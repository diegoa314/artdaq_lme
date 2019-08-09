#define TRACE_NAME (app_name + "_DataSenderManager").c_str()
#include "artdaq/DAQdata/Globals.hh"
#include "artdaq/DAQrate/DataSenderManager.hh"
#include "artdaq/TransferPlugins/MakeTransferPlugin.hh"
#include "artdaq/TransferPlugins/detail/HostMap.hh"

#include <chrono>
#include "canvas/Utilities/Exception.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <poll.h>
#include <sys/socket.h>
#include "artdaq/DAQdata/TCPConnect.hh"

artdaq::DataSenderManager::DataSenderManager(const fhicl::ParameterSet& pset)
	: destinations_()
	, destination_metric_data_()
	, destination_metric_send_time_()
	, enabled_destinations_()
	, sent_frag_count_()
	, broadcast_sends_(pset.get<bool>("broadcast_sends", false))
	, non_blocking_mode_(pset.get<bool>("nonblocking_sends", false))
	, send_timeout_us_(pset.get<size_t>("send_timeout_usec", 5000000))
	, send_retry_count_(pset.get<size_t>("send_retry_count", 2))
	, routing_master_mode_(detail::RoutingMasterMode::INVALID)
	, should_stop_(false)
	, ack_socket_(-1)
	, table_socket_(-1)
	, routing_table_last_(0)
	, routing_table_max_size_(pset.get<size_t>("routing_table_max_size", 1000))
	, highest_sequence_id_routed_(0)
{
	TLOG(TLVL_DEBUG) << "Received pset: " << pset.to_string();

	// Validate parameters
	if (send_timeout_us_ == 0) send_timeout_us_ = std::numeric_limits<size_t>::max();

	auto rmConfig = pset.get<fhicl::ParameterSet>("routing_table_config", fhicl::ParameterSet());
	use_routing_master_ = rmConfig.get<bool>("use_routing_master", false);
	table_port_ = rmConfig.get<int>("table_update_port", 35556);
	table_address_ = rmConfig.get<std::string>("table_update_address", "227.128.12.28");
	ack_port_ = rmConfig.get<int>("table_acknowledge_port", 35557);
	ack_address_ = rmConfig.get<std::string>("routing_master_hostname", "localhost");
	routing_timeout_ms_ = (rmConfig.get<int>("routing_timeout_ms", 1000));
	routing_retry_count_ = rmConfig.get<int>("routing_retry_count", 5);

	hostMap_t host_map = MakeHostMap(pset);
	size_t tcp_send_buffer_size = pset.get<size_t>("tcp_send_buffer_size", 0);
	size_t max_fragment_size_words = pset.get<size_t>("max_fragment_size_words", 0);

	auto dests = pset.get<fhicl::ParameterSet>("destinations", fhicl::ParameterSet());
	for (auto& d : dests.get_pset_names())
	{
		auto dest_pset = dests.get<fhicl::ParameterSet>(d);
		host_map = MakeHostMap(dest_pset, host_map);
	}
	auto host_map_pset = MakeHostMapPset(host_map);
	fhicl::ParameterSet dests_mod;
	for (auto& d : dests.get_pset_names())
	{
		auto dest_pset = dests.get<fhicl::ParameterSet>(d);
		dest_pset.erase("host_map");
		dest_pset.put<std::vector<fhicl::ParameterSet>>("host_map", host_map_pset);

		if (tcp_send_buffer_size != 0 && !dest_pset.has_key("tcp_send_buffer_size"))
		{
			dest_pset.put<size_t>("tcp_send_buffer_size", tcp_send_buffer_size);
		}
		if (max_fragment_size_words != 0 && !dest_pset.has_key("max_fragment_size_words"))
		{
			dest_pset.put<size_t>("max_fragment_size_words", max_fragment_size_words);
		}

		dests_mod.put<fhicl::ParameterSet>(d, dest_pset);
	}

	for (auto& d : dests_mod.get_pset_names())
	{
		try
		{
			auto transfer = MakeTransferPlugin(dests_mod, d, TransferInterface::Role::kSend);
			auto destination_rank = transfer->destination_rank();
			destinations_.emplace(destination_rank, std::move(transfer));
			destination_metric_data_[destination_rank] = std::pair<size_t, double>();
			destination_metric_send_time_[destination_rank] = std::chrono::steady_clock::now();
		}
		catch (std::invalid_argument)
		{
			TLOG(TLVL_DEBUG) << "Invalid destination specification: " << d;
		}
		catch (cet::exception ex)
		{
			TLOG(TLVL_WARNING) << "Caught cet::exception: " << ex.what();
		}
		catch (...)
		{
			TLOG(TLVL_WARNING) << "Non-cet exception while setting up TransferPlugin: " << d << ".";
		}
	}
	if (destinations_.size() == 0)
	{
		TLOG(TLVL_ERROR) << "No destinations specified!";
	}
	else
	{
		auto enabled_dests = pset.get<std::vector<size_t>>("enabled_destinations", std::vector<size_t>());
		if (enabled_dests.size() == 0)
		{
			TLOG(TLVL_INFO) << "enabled_destinations not specified, assuming all destinations enabled.";
			for (auto& d : destinations_)
			{
				enabled_destinations_.insert(d.first);
			}
		}
		else
		{
			for (auto& d : enabled_dests)
			{
				enabled_destinations_.insert(d);
			}
		}
	}
	if (use_routing_master_) startTableReceiverThread_();
}

artdaq::DataSenderManager::~DataSenderManager()
{
	TLOG(TLVL_DEBUG) << "Shutting down DataSenderManager BEGIN";
	should_stop_ = true;
	for (auto& dest : enabled_destinations_)
	{
		if (destinations_.count(dest))
		{
			auto sts = destinations_[dest]->transfer_fragment_reliable_mode(std::move(*Fragment::eodFrag(sent_frag_count_.slotCount(dest))));
			if (sts != TransferInterface::CopyStatus::kSuccess) TLOG(TLVL_ERROR) << "Error sending EOD Fragment to sender rank " << dest;
			//  sendFragTo(std::move(*Fragment::eodFrag(nFragments)), dest, true);
		}
	}
	if (routing_thread_.joinable()) routing_thread_.join();
	TLOG(TLVL_DEBUG) << "Shutting down DataSenderManager END. Sent " << count() << " fragments.";
}


void artdaq::DataSenderManager::setupTableListener_()
{
	int sts;
	table_socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (table_socket_ < 0)
	{
		TLOG(TLVL_ERROR) << "Error creating socket for receiving table updates!";
		exit(1);
	}

	struct sockaddr_in si_me_request;

	int yes = 1;
	if (setsockopt(table_socket_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
	{
		TLOG(TLVL_ERROR) << " Unable to enable port reuse on request socket";
		exit(1);
	}
	memset(&si_me_request, 0, sizeof(si_me_request));
	si_me_request.sin_family = AF_INET;
	si_me_request.sin_port = htons(table_port_);
	//si_me_request.sin_addr.s_addr = htonl(INADDR_ANY);
	struct in_addr          in_addr_s;
	sts = inet_aton(table_address_.c_str(), &in_addr_s );
	if (sts == 0)
	{
		TLOG(TLVL_ERROR) << "inet_aton says table_address " << table_address_ << " is invalid";
	}
	si_me_request.sin_addr.s_addr = in_addr_s.s_addr;
	if (bind(table_socket_, (struct sockaddr *)&si_me_request, sizeof(si_me_request)) == -1)
	{
		TLOG(TLVL_ERROR) << "Cannot bind request socket to port " << table_port_;
		exit(1);
	}

	struct ip_mreq mreq;
	sts = ResolveHost(table_address_.c_str(), mreq.imr_multiaddr);
	if (sts == -1)
	{
		TLOG(TLVL_ERROR) << "Unable to resolve multicast address for table updates";
		exit(1);
	}
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(table_socket_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
	{
		TLOG(TLVL_ERROR) << "Unable to join multicast group";
		exit(1);
	}
}
void artdaq::DataSenderManager::startTableReceiverThread_()
{
	if (routing_thread_.joinable()) routing_thread_.join();
	TLOG(TLVL_INFO) << "Starting Routing Thread";
	try {
		routing_thread_ = boost::thread(&DataSenderManager::receiveTableUpdatesLoop_, this);
	}
	catch (const boost::exception& e)
	{
		TLOG(TLVL_ERROR) << "Caught boost::exception starting Routing Table Receive thread: " << boost::diagnostic_information(e) << ", errno=" << errno;
		std::cerr << "Caught boost::exception starting Routing Table Receive thread: " << boost::diagnostic_information(e) << ", errno=" << errno << std::endl;
		exit(5);
	}
}
void artdaq::DataSenderManager::receiveTableUpdatesLoop_()
{
	while (true)
	{
		if (should_stop_)
		{
			TLOG(TLVL_DEBUG) << __func__ << ": should_stop is " << std::boolalpha << should_stop_ << ", stopping";
			return;
		}

		TLOG(TLVL_TRACE) << __func__ << ": Polling table socket for new routes";
		if (table_socket_ == -1)
		{
			TLOG(TLVL_DEBUG) << __func__ << ": Opening table listener socket";
			setupTableListener_();
		}
		if (table_socket_ == -1)
		{
			TLOG(TLVL_DEBUG) << __func__ << ": The listen socket was not opened successfully.";
			return;
		}
		if (ack_socket_ == -1)
		{
			ack_socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			auto sts = ResolveHost(ack_address_.c_str(), ack_port_, ack_addr_);
			if (sts == -1)
			{
				TLOG(TLVL_ERROR) << __func__ << ": Unable to resolve routing_master_address";
				exit(1);
			}
			TLOG(TLVL_DEBUG) << __func__ << ": Ack socket is fd " << ack_socket_;
		}

		struct pollfd fd;
		fd.fd = table_socket_;
		fd.events = POLLIN | POLLPRI;

		auto res = poll(&fd, 1, 1000);
		if (res > 0)
		{
			auto first = artdaq::Fragment::InvalidSequenceID;
			auto last = artdaq::Fragment::InvalidSequenceID;
			artdaq::detail::RoutingPacketHeader hdr;

			TLOG(TLVL_DEBUG) << __func__ << ": Going to receive RoutingPacketHeader";
			struct sockaddr_in from;
			socklen_t          len=sizeof(from);
			auto stss = recvfrom(table_socket_, &hdr, sizeof(artdaq::detail::RoutingPacketHeader), 0, (struct sockaddr*)&from, &len );
			TLOG(TLVL_DEBUG) << __func__ << ": Received " << stss << " hdr bytes. (sizeof(RoutingPacketHeader) == " << sizeof(detail::RoutingPacketHeader)
							 << " from " << inet_ntoa(from.sin_addr) << ":" << from.sin_port;

			TRACE(TLVL_DEBUG,"receiveTableUpdatesLoop_: Checking for valid header with nEntries=%lu headerData:0x%016lx%016lx",hdr.nEntries,((unsigned long*)&hdr)[0],((unsigned long*)&hdr)[1]);
			if (hdr.header != ROUTING_MAGIC)
			{
				TLOG(TLVL_TRACE) << __func__ << ": non-RoutingPacket received. No ROUTING_MAGIC. size(bytes)="<<stss;
			}
			else if (stss != sizeof(artdaq::detail::RoutingPacketHeader))
			{
				TLOG(TLVL_TRACE) << __func__ << ": non-RoutingPacket received. size(bytes)="<<stss;
			}
			else
			{
				if (routing_master_mode_ != detail::RoutingMasterMode::INVALID && routing_master_mode_ != hdr.mode)
				{
					TLOG(TLVL_ERROR) << __func__ << ": Received table has different RoutingMasterMode than expected!";
					exit(1);
				}
				routing_master_mode_ = hdr.mode;

				artdaq::detail::RoutingPacket buffer(hdr.nEntries);
				TLOG(TLVL_DEBUG) << __func__ << ": Receiving data buffer";
				auto sts = recvfrom(table_socket_, &buffer[0], sizeof(artdaq::detail::RoutingPacketEntry) * hdr.nEntries, 0, (struct sockaddr*)&from, &len );
				assert(static_cast<size_t>(sts) == sizeof(artdaq::detail::RoutingPacketEntry) * hdr.nEntries);
				TLOG(TLVL_DEBUG) << __func__ << ": Received " << sts << " pkt bytes from " << inet_ntoa(from.sin_addr) << ":" << from.sin_port;
				TRACE(6,"receiveTableUpdatesLoop_: Received a packet of %ld bytes. 1st 16 bytes: 0x%016lx%016lx",sts,((unsigned long*)&buffer[0])[0],((unsigned long*)&buffer[0])[1]);

				first = buffer[0].sequence_id;
				last = buffer[buffer.size() - 1].sequence_id;

				if (first + hdr.nEntries - 1 != last)
				{
					TLOG(TLVL_ERROR) << __func__ << ": Skipping this RoutingPacket because the first (" << first << ") and last (" << last << ") entries are inconsistent (sz=" << hdr.nEntries << ")!";
					continue;
				}
				auto thisSeqID = first;

				{
				std::unique_lock<std::mutex> lck(routing_mutex_);
				if (routing_table_.count(last) == 0)
				{
					for (auto entry : buffer)
					{
						if (thisSeqID != entry.sequence_id)
						{
							TLOG(TLVL_ERROR) << __func__ << ": Aborting processing of this RoutingPacket because I encountered an inconsistent entry (seqid=" << entry.sequence_id << ", expected=" << thisSeqID << ")!";
							last = thisSeqID - 1;
							break;
						}
						thisSeqID++;
						if (routing_table_.count(entry.sequence_id))
						{
							if (routing_table_[entry.sequence_id] != entry.destination_rank)
							{
								TLOG(TLVL_ERROR) << __func__ << ": Detected routing table corruption! Recevied update specifying that sequence ID " << entry.sequence_id
									<< " should go to rank " << entry.destination_rank << ", but I had already been told to send it to " << routing_table_[entry.sequence_id] << "!"
									<< " I will use the original value!";
							}
							continue;
						}
						if (entry.sequence_id < routing_table_last_) continue;
						routing_table_[entry.sequence_id] = entry.destination_rank;
						TLOG(TLVL_DEBUG) << __func__ << ": (my_rank=" << my_rank << ") received update: SeqID " << entry.sequence_id
										 << " -> Rank " << entry.destination_rank;
					}
				}
					
					TLOG(TLVL_DEBUG) << __func__ << ": There are now " << routing_table_.size() << " entries in the Routing Table";
					if (routing_table_.size() > 0) TLOG(TLVL_DEBUG) << __func__ << ": Last routing table entry is seqID=" << routing_table_.rbegin()->first;

					auto counter = 0;
					for (auto& entry : routing_table_)
					{
						TLOG(45) << "Routing Table Entry" << counter << ": " << entry.first << " -> " << entry.second;
						counter++;
					}
				}

				artdaq::detail::RoutingAckPacket ack;
				ack.rank = my_rank;
				ack.first_sequence_id = first;
				ack.last_sequence_id = last;

				if (last > routing_table_last_) routing_table_last_ = last;

				TLOG(TLVL_DEBUG) << __func__ << ": Sending RoutingAckPacket with first= " << first << " and last= " << last << " to " << ack_address_ << ", port " << ack_port_ << " (my_rank = " << my_rank << ")";
				sendto(ack_socket_, &ack, sizeof(artdaq::detail::RoutingAckPacket), 0, (struct sockaddr *)&ack_addr_, sizeof(ack_addr_));
			}
		}
	}
}

size_t artdaq::DataSenderManager::GetRoutingTableEntryCount() const
{
	std::unique_lock<std::mutex> lck(routing_mutex_);
	return routing_table_.size();
}

size_t artdaq::DataSenderManager::GetRemainingRoutingTableEntries() const
{
	std::unique_lock<std::mutex> lck(routing_mutex_);
	// Find the distance from the next highest sequence ID to the end of the list
	size_t dist = std::distance(routing_table_.upper_bound(highest_sequence_id_routed_), routing_table_.end());
	return dist; // If dist == 1, there is one entry left.
}

int artdaq::DataSenderManager::calcDest_(Fragment::sequence_id_t sequence_id) const
{
	if (enabled_destinations_.size() == 0) return TransferInterface::RECV_TIMEOUT; // No destinations configured.
	if (!use_routing_master_ && enabled_destinations_.size() == 1) return *enabled_destinations_.begin(); // Trivial case

	if (use_routing_master_)
	{
		auto start = std::chrono::steady_clock::now();
		TLOG(15) << "calcDest_ use_routing_master check for routing info for seqID="<<sequence_id<<" routing_timeout_ms="<<routing_timeout_ms_<<" should_stop_="<<should_stop_;
		while (!should_stop_ && (routing_timeout_ms_ <= 0 || TimeUtils::GetElapsedTimeMilliseconds(start) < static_cast<size_t>(routing_timeout_ms_)))
		{
		  {
			std::unique_lock<std::mutex> lck(routing_mutex_);
			if (routing_master_mode_ == detail::RoutingMasterMode::RouteBySequenceID && routing_table_.count(sequence_id))
			{
				if (sequence_id > highest_sequence_id_routed_) highest_sequence_id_routed_ = sequence_id;
				routing_wait_time_.fetch_add(TimeUtils::GetElapsedTimeMicroseconds(start));
				return routing_table_.at(sequence_id);
			}
			else if (routing_master_mode_ == detail::RoutingMasterMode::RouteBySendCount && routing_table_.count(sent_frag_count_.count() + 1))
			{
				if (sent_frag_count_.count() + 1 > highest_sequence_id_routed_) highest_sequence_id_routed_ = sent_frag_count_.count() + 1;
				routing_wait_time_.fetch_add(TimeUtils::GetElapsedTimeMicroseconds(start));
				return routing_table_.at(sent_frag_count_.count() + 1);
			}
		  }
			usleep(routing_timeout_ms_ * 10);
		}
		routing_wait_time_.fetch_add(TimeUtils::GetElapsedTimeMicroseconds(start));
		if (routing_master_mode_ == detail::RoutingMasterMode::RouteBySequenceID)
		{
			TLOG(TLVL_WARNING) << "Bad Omen: I don't have routing information for seqID " << sequence_id
				<< " and the Routing Master did not send a table update in routing_timeout_ms window (" << routing_timeout_ms_ << " ms)!";
		}
		else
		{
			TLOG(TLVL_WARNING) << "Bad Omen: I don't have routing information for send number " << sent_frag_count_.count()
				<< " and the Routing Master did not send a table update in routing_timeout_ms window (" << routing_timeout_ms_ << " ms)!";
		}
	}
	else
	{
		auto index = sequence_id % enabled_destinations_.size();
		auto it = enabled_destinations_.begin();
		for (; index > 0; --index)
		{
			++it;
			if (it == enabled_destinations_.end()) it = enabled_destinations_.begin();
		}
		return *it;
	}
	return TransferInterface::RECV_TIMEOUT;
}

std::pair<int, artdaq::TransferInterface::CopyStatus> artdaq::DataSenderManager::sendFragment(Fragment&& frag)
{
	// Precondition: Fragment must be complete and consistent (including
	// header information).
	auto start_time = std::chrono::steady_clock::now();
	if (frag.type() == Fragment::EndOfDataFragmentType)
	{
		throw cet::exception("LogicError")
			<< "EOD fragments should not be sent on as received: "
			<< "use sendEODFrag() instead.";
	}
	size_t seqID = frag.sequenceID();
	size_t fragSize = frag.sizeBytes();
	TLOG(13) << "sendFragment start frag.fragmentHeader()=" << std::hex << (void*)(frag.headerBeginBytes()) << ", szB=" << std::dec << fragSize
		<< ", seqID=" << seqID << ", type=" << frag.typeString();
	int dest = TransferInterface::RECV_TIMEOUT;
	auto outsts = TransferInterface::CopyStatus::kSuccess;
	if (broadcast_sends_ || frag.type() == Fragment::EndOfRunFragmentType || frag.type() == Fragment::EndOfSubrunFragmentType || frag.type() == Fragment::InitFragmentType)
	{
		for (auto& bdest : enabled_destinations_)
		{
			TLOG(TLVL_TRACE) << "sendFragment: Sending fragment with seqId " << seqID << " to destination " << bdest << " (broadcast)";
			// Gross, we have to copy.
			auto sts = TransferInterface::CopyStatus::kTimeout;
			size_t retries = 0; // Tried once, so retries < send_retry_count_ will have it retry send_retry_count_ times
			while (sts == TransferInterface::CopyStatus::kTimeout && retries < send_retry_count_)
			{
				if (!non_blocking_mode_)
				{
					sts = destinations_[bdest]->transfer_fragment_reliable_mode(Fragment(frag));
				}
				else
				{
					sts = destinations_[bdest]->transfer_fragment_min_blocking_mode(frag, send_timeout_us_);
				}
				retries++;
			}
			if (sts != TransferInterface::CopyStatus::kSuccess) outsts = sts;
			sent_frag_count_.incSlot(bdest);
		}
	}
	else if (non_blocking_mode_)
	{
		auto count = routing_retry_count_;
		while (dest == TransferInterface::RECV_TIMEOUT && count > 0)
		{
			dest = calcDest_(seqID);
			if (dest == TransferInterface::RECV_TIMEOUT)
			{
				count--;
				TLOG(TLVL_WARNING) << "Could not get destination for seqID " << seqID << (count > 0 ? ", retrying." : ".");
			}
		}
		if (dest != TransferInterface::RECV_TIMEOUT && destinations_.count(dest) && enabled_destinations_.count(dest))
		{
			TLOG(TLVL_TRACE) << "sendFragment: Sending fragment with seqId " << seqID << " to destination " << dest;
			TransferInterface::CopyStatus sts = TransferInterface::CopyStatus::kErrorNotRequiringException;
			auto lastWarnTime = std::chrono::steady_clock::now();
			size_t retries = 0; // Have NOT yet tried, so retries <= send_retry_count_ will have it RETRY send_retry_count_ times
			while (sts != TransferInterface::CopyStatus::kSuccess && retries <= send_retry_count_)
			{
				sts = destinations_[dest]->transfer_fragment_min_blocking_mode(frag, send_timeout_us_);
				if (sts != TransferInterface::CopyStatus::kSuccess && TimeUtils::GetElapsedTime(lastWarnTime) >= 1)
				{
					TLOG(TLVL_WARNING) << "sendFragment: Sending fragment " << seqID << " to destination " << dest << " failed! Retrying...";
					lastWarnTime = std::chrono::steady_clock::now();
				}
			}
			if (sts != TransferInterface::CopyStatus::kSuccess) outsts = sts;
			//sendFragTo(std::move(frag), dest);
			sent_frag_count_.incSlot(dest);
		}
		else if (!should_stop_)
			TLOG(TLVL_ERROR) << "(in non_blocking) calcDest returned invalid destination rank " << dest << "! This event has been lost: " << seqID
							 << ". enabled_destinantions_.size()="<<enabled_destinations_.size();
	}
	else
	{
		auto start = std::chrono::steady_clock::now();
		while (!should_stop_ && dest == TransferInterface::RECV_TIMEOUT)
		{
			dest = calcDest_(seqID);
			if (dest == TransferInterface::RECV_TIMEOUT)
			{
				TLOG(TLVL_WARNING) << "Could not get destination for seqID " << seqID << ", send number " << sent_frag_count_.count() << ", retrying. Waited " << TimeUtils::GetElapsedTime(start) << " s for routing information.";
				usleep(10000);
			}
		}
		if (dest != TransferInterface::RECV_TIMEOUT && destinations_.count(dest) && enabled_destinations_.count(dest))
		{
			TLOG(5) << "DataSenderManager::sendFragment: Sending fragment with seqId " << seqID << " to destination " << dest;
			TransferInterface::CopyStatus sts = TransferInterface::CopyStatus::kErrorNotRequiringException;

			sts = destinations_[dest]->transfer_fragment_reliable_mode(std::move(frag));
			if (sts != TransferInterface::CopyStatus::kSuccess)
				TLOG(TLVL_ERROR) << "sendFragment: Sending fragment " << seqID << " to destination "
				<< dest << " failed! Data has been lost!";

			//sendFragTo(std::move(frag), dest);
			sent_frag_count_.incSlot(dest);
			outsts = sts;
		}
		else if (!should_stop_)
			TLOG(TLVL_ERROR) << "calcDest returned invalid destination rank " << dest << "! This event has been lost: " << seqID
							 << ". enabled_destinantions_.size()="<<enabled_destinations_.size();
	}

	{
		std::unique_lock<std::mutex> lck(routing_mutex_);
	//	while (routing_table_.size() > routing_table_max_size_)
	//	{
	//		routing_table_.erase(routing_table_.begin());
	//	}
	if(routing_master_mode_ == detail::RoutingMasterMode::RouteBySequenceID && routing_table_.find(seqID) != routing_table_.end())
		routing_table_.erase(routing_table_.find(seqID));
	else if(routing_table_.find(sent_frag_count_.count()) != routing_table_.end())
	  routing_table_.erase(routing_table_.find(sent_frag_count_.count()));
	}


	auto delta_t = TimeUtils::GetElapsedTime(start_time);
	destination_metric_data_[dest].first += fragSize;
	destination_metric_data_[dest].second += delta_t;

	if (metricMan && TimeUtils::GetElapsedTime(destination_metric_send_time_[dest]) > 1)
	{
		TLOG(5) << "sendFragment: sending metrics";
		metricMan->sendMetric("Data Send Time to Rank " + std::to_string(dest), destination_metric_data_[dest].second, "s", 5, MetricMode::Accumulate);
		metricMan->sendMetric("Data Send Size to Rank " + std::to_string(dest), destination_metric_data_[dest].first, "B", 5, MetricMode::Accumulate);
		metricMan->sendMetric("Data Send Rate to Rank " + std::to_string(dest), destination_metric_data_[dest].first / destination_metric_data_[dest].second, "B/s", 5, MetricMode::Average);
		metricMan->sendMetric("Data Send Count to Rank " + std::to_string(dest), sent_frag_count_.slotCount(dest), "fragments", 3, MetricMode::LastPoint);

		destination_metric_send_time_[dest] = std::chrono::steady_clock::now();
		destination_metric_data_[dest].first = 0;
		destination_metric_data_[dest].second = 0.0;

		if (use_routing_master_)
		{
			metricMan->sendMetric("Routing Table Size", GetRoutingTableEntryCount(), "events", 2, MetricMode::LastPoint);
			if (routing_wait_time_ > 0)
			{
				metricMan->sendMetric("Routing Wait Time", static_cast<double>(routing_wait_time_.load()) / 1000000, "s", 2, MetricMode::Average);
				routing_wait_time_ = 0;
			}
		}
	}
	TLOG(5) << "sendFragment: Done sending fragment " << seqID;
	return std::make_pair(dest, outsts);
}   // artdaq::DataSenderManager::sendFragment
