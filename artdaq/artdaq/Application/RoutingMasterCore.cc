#include <sys/un.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <sched.h>
#include <algorithm>

#include "canvas/Utilities/Exception.h"
#include "cetlib_except/exception.h"

#define TRACE_NAME (app_name + "_RoutingMasterCore").c_str() // include these 2 first -
#include "artdaq/DAQdata/Globals.hh"   // to get tracemf.h before trace.h
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Utilities/ExceptionHandler.hh"

#include "artdaq/Application/RoutingMasterCore.hh"
#include "artdaq/Application/Routing/makeRoutingMasterPolicy.hh"
#include "artdaq/DAQdata/TCP_listen_fd.hh"
#include "artdaq/DAQdata/TCPConnect.hh"

const std::string artdaq::RoutingMasterCore::
TABLE_UPDATES_STAT_KEY("RoutingMasterCoreTableUpdates");
const std::string artdaq::RoutingMasterCore::
TOKENS_RECEIVED_STAT_KEY("RoutingMasterCoreTokensReceived");

artdaq::RoutingMasterCore::RoutingMasterCore() 
	: received_token_counter_()
	, shutdown_requested_(false)
	, stop_requested_(false)
	, pause_requested_(false)
	, token_socket_(-1)
	, table_socket_(-1)
	, ack_socket_(-1)
{
	TLOG(TLVL_DEBUG) << "Constructor" ;
	statsHelper_.addMonitoredQuantityName(TABLE_UPDATES_STAT_KEY);
	statsHelper_.addMonitoredQuantityName(TOKENS_RECEIVED_STAT_KEY);
}

artdaq::RoutingMasterCore::~RoutingMasterCore()
{
	TLOG(TLVL_DEBUG) << "Destructor" ;
	if (ev_token_receive_thread_.joinable()) ev_token_receive_thread_.join();
}

bool artdaq::RoutingMasterCore::initialize(fhicl::ParameterSet const& pset, uint64_t, uint64_t)
{
	TLOG(TLVL_DEBUG) << "initialize method called with "
		<< "ParameterSet = \"" << pset.to_string()
		<< "\"." ;

	// pull out the relevant parts of the ParameterSet
	fhicl::ParameterSet daq_pset;
	try
	{
		daq_pset = pset.get<fhicl::ParameterSet>("daq");
	}
	catch (...)
	{
		TLOG(TLVL_ERROR)
			<< "Unable to find the DAQ parameters in the initialization "
			<< "ParameterSet: \"" + pset.to_string() + "\"." ;
		return false;
	}

	if (daq_pset.has_key("rank"))
	{
		if (my_rank >= 0 && daq_pset.get<int>("rank") != my_rank) {
			TLOG(TLVL_WARNING) << "Routing Master rank specified at startup is different than rank specified at configure! Using rank received at configure!";
		}
		my_rank = daq_pset.get<int>("rank");
	}
	if (my_rank == -1)
	{
		TLOG(TLVL_ERROR) << "Routing Master rank not specified at startup or in configuration! Aborting";
		exit(1);
	}

	try
	{
		policy_pset_ = daq_pset.get<fhicl::ParameterSet>("policy");
	}
	catch (...)
	{
		TLOG(TLVL_ERROR)
			<< "Unable to find the policy parameters in the DAQ initialization ParameterSet: \"" + daq_pset.to_string() + "\"." ;
		return false;
	}

	// pull out the Metric part of the ParameterSet
	fhicl::ParameterSet metric_pset;
	try
	{
		metric_pset = daq_pset.get<fhicl::ParameterSet>("metrics");
	}
	catch (...) {} // OK if there's no metrics table defined in the FHiCL 

	if (metric_pset.is_empty())
	{
		TLOG(TLVL_INFO) << "No metric plugins appear to be defined" ;
	}
	try
	{
		metricMan->initialize(metric_pset, app_name);
	}
	catch (...)
	{
		ExceptionHandler(ExceptionHandlerRethrow::no,
						 "Error loading metrics in RoutingMasterCore::initialize()");
	}

	// create the requested CommandableFragmentGenerator
	auto policy_plugin_spec = policy_pset_.get<std::string>("policy", "");
	if (policy_plugin_spec.length() == 0)
	{
		TLOG(TLVL_ERROR)
			<< "No fragment generator (parameter name = \"policy\") was "
			<< "specified in the policy ParameterSet.  The "
			<< "DAQ initialization PSet was \"" << daq_pset.to_string() << "\"." ;
		return false;
	}
	try
	{
		policy_ = artdaq::makeRoutingMasterPolicy(policy_plugin_spec, policy_pset_);
	}
	catch (...)
	{
		std::stringstream exception_string;
		exception_string << "Exception thrown during initialization of policy of type \""
			<< policy_plugin_spec << "\"";

		ExceptionHandler(ExceptionHandlerRethrow::no, exception_string.str());

		TLOG(TLVL_DEBUG) << "FHiCL parameter set used to initialize the policy which threw an exception: " << policy_pset_.to_string() ;

		return false;
	}

	rt_priority_ = daq_pset.get<int>("rt_priority", 0);
	sender_ranks_ = daq_pset.get<std::vector<int>>("sender_ranks");
	num_receivers_ = policy_->GetReceiverCount();

	receive_ack_events_ = std::vector<epoll_event>(sender_ranks_.size());
	receive_token_events_ = std::vector<epoll_event>(num_receivers_ + 1);

	auto mode = daq_pset.get<bool>("senders_send_by_send_count", false);
	routing_mode_ = mode ? detail::RoutingMasterMode::RouteBySendCount : detail::RoutingMasterMode::RouteBySequenceID;
	max_table_update_interval_ms_ = daq_pset.get<size_t>("table_update_interval_ms", 1000);
	current_table_interval_ms_ = max_table_update_interval_ms_;
	max_ack_cycle_count_ = daq_pset.get<size_t>("table_ack_retry_count", 5);
	receive_token_port_ = daq_pset.get<int>("routing_token_port", 35555);
	send_tables_port_ = daq_pset.get<int>("table_update_port", 35556);
	receive_acks_port_ = daq_pset.get<int>("table_acknowledge_port", 35557);
	send_tables_address_ = daq_pset.get<std::string>("table_update_address", "227.128.12.28");
	receive_address_ = daq_pset.get<std::string>("routing_master_hostname", "localhost");

	// fetch the monitoring parameters and create the MonitoredQuantity instances
	statsHelper_.createCollectors(daq_pset, 100, 30.0, 60.0, TABLE_UPDATES_STAT_KEY);

	shutdown_requested_.store(false);
	start_recieve_token_thread_();
	return true;
}

bool artdaq::RoutingMasterCore::start(art::RunID id, uint64_t, uint64_t)
{
	stop_requested_.store(false);
	pause_requested_.store(false);

	statsHelper_.resetStatistics();
	policy_->Reset();

	metricMan->do_start();
	run_id_ = id;
	table_update_count_ = 0;
	received_token_count_ = 0;

	TLOG(TLVL_INFO) << "Started run " << run_id_.run() ;
	return true;
}

bool artdaq::RoutingMasterCore::stop(uint64_t, uint64_t)
{
	TLOG(TLVL_INFO) << "Stopping run " << run_id_.run()
		<< " after " << table_update_count_ << " table updates."
		<< " and " << received_token_count_ << " received tokens." ;
	stop_requested_.store(true);
	return true;
}

bool artdaq::RoutingMasterCore::pause(uint64_t, uint64_t)
{
	TLOG(TLVL_INFO) << "Pausing run " << run_id_.run()
		<< " after " << table_update_count_ << " table updates."
		<< " and " << received_token_count_ << " received tokens." ;
	pause_requested_.store(true);
	return true;
}

bool artdaq::RoutingMasterCore::resume(uint64_t, uint64_t)
{
	TLOG(TLVL_INFO) << "Resuming run " << run_id_.run() ;
	policy_->Reset();
	pause_requested_.store(false);
	metricMan->do_start();
	return true;
}

bool artdaq::RoutingMasterCore::shutdown(uint64_t)
{
    shutdown_requested_.store(true);
	if (ev_token_receive_thread_.joinable()) ev_token_receive_thread_.join();
	policy_.reset(nullptr);
	metricMan->shutdown();
	return true;
}

bool artdaq::RoutingMasterCore::soft_initialize(fhicl::ParameterSet const& pset, uint64_t e, uint64_t f)
{
	TLOG(TLVL_INFO) << "soft_initialize method called with "
		<< "ParameterSet = \"" << pset.to_string()
		<< "\"." ;
	return initialize(pset, e, f);
}

bool artdaq::RoutingMasterCore::reinitialize(fhicl::ParameterSet const& pset, uint64_t e, uint64_t f)
{
	TLOG(TLVL_INFO) << "reinitialize method called with "
		<< "ParameterSet = \"" << pset.to_string()
		<< "\"." ;
	return initialize(pset, e, f);
}

void artdaq::RoutingMasterCore::process_event_table()
{
	if (rt_priority_ > 0)
	{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
		sched_param s_param = {};
		s_param.sched_priority = rt_priority_;
		if (pthread_setschedparam(pthread_self(), SCHED_RR, &s_param))
			TLOG(TLVL_WARNING) << "setting realtime priority failed" ;
#pragma GCC diagnostic pop
	}

	// try-catch block here?

	// how to turn RT PRI off?
	if (rt_priority_ > 0)
	{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
		sched_param s_param = {};
		s_param.sched_priority = rt_priority_;
		int status = pthread_setschedparam(pthread_self(), SCHED_RR, &s_param);
		if (status != 0)
		{
			TLOG(TLVL_ERROR)
				<< "Failed to set realtime priority to " << rt_priority_
				<< ", return code = " << status ;
		}
#pragma GCC diagnostic pop
	}

	//MPI_Barrier(local_group_comm_);

	TLOG(TLVL_DEBUG) << "Sending initial table." ;
	auto startTime = artdaq::MonitoredQuantity::getCurrentTime();
	auto nextSendTime = startTime;
	double delta_time;
	while (!stop_requested_ && !pause_requested_)
	{
		startTime = artdaq::MonitoredQuantity::getCurrentTime();

		if (startTime >= nextSendTime)
		{
			auto table = policy_->GetCurrentTable();
			if (table.size() > 0)
			{
				send_event_table(table);
				++table_update_count_;
				delta_time = artdaq::MonitoredQuantity::getCurrentTime() - startTime;
				statsHelper_.addSample(TABLE_UPDATES_STAT_KEY, delta_time);
				TLOG(16) << "process_fragments TABLE_UPDATES_STAT_KEY=" << delta_time ;
			}
			else
			{
			  TLOG(TLVL_DEBUG) << "No tokens received in this update interval (" << current_table_interval_ms_ << " ms)! This most likely means that the receivers are not keeping up!" ;
			}
			auto max_tokens = policy_->GetMaxNumberOfTokens();
			if (max_tokens > 0)
			{
				auto frac = table.size() / static_cast<double>(max_tokens);
				if (frac > 0.75) current_table_interval_ms_ = 9 * current_table_interval_ms_ / 10;
				if (frac < 0.5) current_table_interval_ms_ = 11 * current_table_interval_ms_ / 10;
				if (current_table_interval_ms_ > max_table_update_interval_ms_) current_table_interval_ms_ = max_table_update_interval_ms_;
				if (current_table_interval_ms_ < 1) current_table_interval_ms_ = 1;
			}
			nextSendTime = startTime + current_table_interval_ms_ / 1000.0;
			TLOG(TLVL_DEBUG) << "current_table_interval_ms is now " << current_table_interval_ms_ ;
		}
		else
		{
			usleep(current_table_interval_ms_ * 10); // 1/100 of the table update interval
		}
	}

	metricMan->do_stop();
}

void artdaq::RoutingMasterCore::send_event_table(detail::RoutingPacket packet)
{
	// Reconnect table socket, if necessary
	if (table_socket_ == -1)
	{
		table_socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (table_socket_ < 0)
		{
			TLOG(TLVL_ERROR) << "I failed to create the socket for sending Data Requests! Errno: " << errno ;
			exit(1);
		}
		auto sts = ResolveHost(send_tables_address_.c_str(), send_tables_port_, send_tables_addr_);
		if (sts == -1)
		{
			TLOG(TLVL_ERROR) << "Unable to resolve table_update_address" ;
			exit(1);
		}

		auto yes = 1;
		if (receive_address_ != "localhost")
		{
			TLOG(TLVL_DEBUG) << "Making sure that multicast sending uses the correct interface for hostname " << receive_address_ ;
			struct in_addr addr;
			sts = ResolveHost(receive_address_.c_str(), addr);
			if (sts == -1)
			{
				throw art::Exception(art::errors::Configuration) << "RoutingMasterCore: Unable to resolve routing_master_address" << std::endl;;
			}

			if (setsockopt(table_socket_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
			{
				throw art::Exception(art::errors::Configuration) <<
					"RoutingMasterCore: Unable to enable port reuse on table update socket" << std::endl;
				exit(1);
			}

			if (setsockopt(table_socket_, IPPROTO_IP, IP_MULTICAST_LOOP, &yes, sizeof(yes)) < 0)
			{
				TLOG(TLVL_ERROR) << "Unable to enable multicast loopback on table socket" ;
				exit(1);
			}
			if (setsockopt(table_socket_, IPPROTO_IP, IP_MULTICAST_IF, &addr, sizeof(addr)) == -1)
			{
				TLOG(TLVL_ERROR) << "Cannot set outgoing interface. Errno: " << errno ;
				exit(1);
			}
		}
		if (setsockopt(table_socket_, SOL_SOCKET, SO_BROADCAST, (void*)&yes, sizeof(int)) == -1)
		{
			TLOG(TLVL_ERROR) << "Cannot set request socket to broadcast. Errno: " << errno ;
			exit(1);
		}
	}

	// Reconnect ack socket, if necessary
	if (ack_socket_ == -1)
	{
		ack_socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (ack_socket_ < 0)
		{
			throw art::Exception(art::errors::Configuration) << "RoutingMasterCore: Error creating socket for receiving table update acks!" << std::endl;
			exit(1);
		}

		struct sockaddr_in si_me_request;

		auto yes = 1;
		if (setsockopt(ack_socket_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
		{
			throw art::Exception(art::errors::Configuration) <<
				"RoutingMasterCore: Unable to enable port reuse on ack socket" << std::endl;
			exit(1);
		}
		memset(&si_me_request, 0, sizeof(si_me_request));
		si_me_request.sin_family = AF_INET;
		si_me_request.sin_port = htons(receive_acks_port_);
		si_me_request.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind(ack_socket_, reinterpret_cast<struct sockaddr *>(&si_me_request), sizeof(si_me_request)) == -1)
		{
			throw art::Exception(art::errors::Configuration) <<
				"RoutingMasterCore: Cannot bind request socket to port " << receive_acks_port_ << std::endl;
			exit(1);
		}
		TLOG(TLVL_DEBUG) << "Listening for acks on 0.0.0.0 port " << receive_acks_port_ ;
	}

	auto acks = std::unordered_map<int, bool>();
	for (auto& r : sender_ranks_)
	{
		acks[r] = false;
	}
	auto counter = 0U;
	auto start_time = std::chrono::steady_clock::now();
	while (std::count_if(acks.begin(), acks.end(), [](std::pair<int, bool> p) {return !p.second; }) > 0 && !stop_requested_)
	{
		// Send table update
		auto header = detail::RoutingPacketHeader(routing_mode_, packet.size());
		auto packetSize = sizeof(detail::RoutingPacketEntry) * packet.size();

		TLOG(TLVL_DEBUG) << "Sending table information for " << header.nEntries << " events to multicast group " << send_tables_address_ << ", port " << send_tables_port_ ;
		TRACE(16,"headerData:0x%016lx%016lx packetData:0x%016lx%016lx"
		      ,((unsigned long*)&header)[0],((unsigned long*)&header)[1], ((unsigned long*)&packet[0])[0],((unsigned long*)&packet[0])[1] );
		auto hdrsts = sendto(table_socket_, &header, sizeof(detail::RoutingPacketHeader), 0, reinterpret_cast<struct sockaddr *>(&send_tables_addr_), sizeof(send_tables_addr_));
		if (hdrsts != sizeof(detail::RoutingPacketHeader))
		{
			TLOG(TLVL_ERROR) << "Error sending routing message header. hdrsts=" << hdrsts;
		}
		auto pktsts = sendto(table_socket_, &packet[0], packetSize, 0, reinterpret_cast<struct sockaddr *>(&send_tables_addr_), sizeof(send_tables_addr_));
		if (pktsts != (ssize_t)packetSize)
		{
			TLOG(TLVL_ERROR) << "Error sending routing message data. hdrsts="<<hdrsts<<" pktsts="<<pktsts;
		}

		// Collect acks

		auto first = packet[0].sequence_id;
		auto last = packet.rbegin()->sequence_id;
		TLOG(TLVL_DEBUG) << "Sent " << hdrsts <<"+"<< pktsts << ". Expecting acks to have first= " << first << ", and last= " << last ;


		auto startTime = std::chrono::steady_clock::now();
		while (std::count_if(acks.begin(), acks.end(), [](std::pair<int, bool> p) {return !p.second; }) > 0)
		{
			auto table_ack_wait_time_ms = current_table_interval_ms_ / max_ack_cycle_count_;
			if (TimeUtils::GetElapsedTimeMilliseconds(startTime) > table_ack_wait_time_ms)
			{
				if (counter > max_ack_cycle_count_ && table_update_count_ > 0)
				{
					TLOG(TLVL_ERROR) << "Did not receive acks from all senders after resending table " << counter
						<< " times during the table_update_interval. Check the status of the senders!" ;
					break;
				}
				TLOG(TLVL_WARNING) << "Did not receive acks from all senders within the timeout (" << table_ack_wait_time_ms << " ms). Resending table update" ;
				break;
			}

			TLOG(20) << "send_event_table: Polling Request socket for new requests" ;
			auto ready = true;
			while (ready)
			{
				detail::RoutingAckPacket buffer;
				if (recvfrom(ack_socket_, &buffer, sizeof(detail::RoutingAckPacket), MSG_DONTWAIT, NULL, NULL) < 0)
				{
					if (errno == EWOULDBLOCK || errno == EAGAIN)
					{
						TLOG(20) << "send_event_table: No more ack datagrams on ack socket." ;
						ready = false;
					}
					else
					{
						TLOG(TLVL_ERROR) << "An unexpected error occurred during ack packet receive" ;
						exit(2);
					}
				}
				else
				{
					TLOG(TLVL_DEBUG) << "Ack packet from rank " << buffer.rank << " has first= " << buffer.first_sequence_id
						<< " and last= " << buffer.last_sequence_id ;
					if (acks.count(buffer.rank) && buffer.first_sequence_id == first && buffer.last_sequence_id == last)
					{
						TLOG(TLVL_DEBUG) << "Received table update acknowledgement from sender with rank " << buffer.rank << "." ;
						acks[buffer.rank] = true;
						TLOG(TLVL_DEBUG) << "There are now " << std::count_if(acks.begin(), acks.end(), [](std::pair<int, bool> p) {return !p.second; })
							<< " acks outstanding" ;
					}
					else
					{
						if (!acks.count(buffer.rank))
						{
							TLOG(TLVL_ERROR) << "Received acknowledgement from invalid rank " << buffer.rank << "!"
								<< " Cross-talk between RoutingMasters means there's a configuration error!" ;
						}
						else
						{
							TLOG(TLVL_WARNING) << "Received acknowledgement from rank " << buffer.rank
								<< " that had incorrect sequence ID information. Discarding." ;
						}
					}
				}
			}
			usleep(table_ack_wait_time_ms * 1000 / 10);
		}
	}
	if (metricMan)
	{
		artdaq::TimeUtils::seconds delta = std::chrono::steady_clock::now() - start_time;
		metricMan->sendMetric("Avg Table Acknowledge Time", delta.count(), "seconds", 3, MetricMode::Average);
	}
}

void artdaq::RoutingMasterCore::receive_tokens_()
{
	while (!shutdown_requested_)
	{
		TLOG(TLVL_DEBUG) << "Receive Token loop start" ;
		if (token_socket_ == -1)
		{
			TLOG(TLVL_DEBUG) << "Opening token listener socket" ;
			token_socket_ = TCP_listen_fd(receive_token_port_, 3 * sizeof(detail::RoutingToken));
			fcntl(token_socket_, F_SETFL, O_NONBLOCK); // set O_NONBLOCK

			if (token_epoll_fd_ != -1) close(token_epoll_fd_);
			struct epoll_event ev;
			token_epoll_fd_ = epoll_create1(0);
			ev.events = EPOLLIN | EPOLLPRI;
			ev.data.fd = token_socket_;
			if (epoll_ctl(token_epoll_fd_, EPOLL_CTL_ADD, token_socket_, &ev) == -1)
			{
				TLOG(TLVL_ERROR) << "Could not register listen socket to epoll fd" ;
				exit(3);
			}
		}
		if (token_socket_ == -1 || token_epoll_fd_ == -1)
		{
			TLOG(TLVL_DEBUG) << "One of the listen sockets was not opened successfully." ;
			return;
		}

		auto nfds = epoll_wait(token_epoll_fd_, &receive_token_events_[0], receive_token_events_.size(), current_table_interval_ms_);
		if (nfds == -1)
		{
			perror("epoll_wait");
			exit(EXIT_FAILURE);
		}

		TLOG(TLVL_DEBUG) << "Received " << nfds << " events" ;
		for (auto n = 0; n < nfds; ++n)
		{
			if (receive_token_events_[n].data.fd == token_socket_)
			{
				TLOG(TLVL_DEBUG) << "Accepting new connection on token_socket" ;
				sockaddr_in addr;
				socklen_t arglen = sizeof(addr);
				auto conn_sock = accept(token_socket_, (struct sockaddr *)&addr, &arglen);
				fcntl(conn_sock, F_SETFL, O_NONBLOCK); // set O_NONBLOCK

				if (conn_sock == -1)
				{
					perror("accept");
					exit(EXIT_FAILURE);
				}

				receive_token_addrs_[conn_sock] = std::string(inet_ntoa(addr.sin_addr));
				TLOG(TLVL_DEBUG) << "New fd is " << conn_sock << " for receiver at " << receive_token_addrs_[conn_sock];
				struct epoll_event ev;
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = conn_sock;
				if (epoll_ctl(token_epoll_fd_, EPOLL_CTL_ADD, conn_sock, &ev) == -1)
				{
					perror("epoll_ctl: conn_sock");
					exit(EXIT_FAILURE);
				}
			}
			else
			{
			  /*if (receive_token_events_[n].events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP))
				{
					TLOG(TLVL_DEBUG) << "Closing connection on fd " << receive_token_events_[n].data.fd << " (" << receive_token_addrs_[receive_token_events_[n].data.fd] << ")";
					receive_token_addrs_.erase(receive_token_events_[n].data.fd);
					close(receive_token_events_[n].data.fd);
					epoll_ctl(token_epoll_fd_, EPOLL_CTL_DEL, receive_token_events_[n].data.fd, NULL);
					continue;
					}*/

				auto startTime = artdaq::MonitoredQuantity::getCurrentTime();
				bool reading = true;
				int sts = 0;
				while(reading)
				  {
					detail::RoutingToken buff;
					sts += read(receive_token_events_[n].data.fd, &buff, sizeof(detail::RoutingToken) - sts);
					if (sts == 0)
					  {
						TLOG(TLVL_INFO) << "Received 0-size token from " << receive_token_addrs_[receive_token_events_[n].data.fd];
						reading = false;
					  }
					else if(sts < 0 && errno == EAGAIN)
					  {
						TLOG(TLVL_DEBUG) << "No more tokens from this rank. Continuing poll loop.";
						reading = false;
					  }
					else if(sts < 0)
					  {
						TLOG(TLVL_ERROR) << "Error reading from token socket: sts=" << sts << ", errno=" << errno;
						receive_token_addrs_.erase(receive_token_events_[n].data.fd);
						close(receive_token_events_[n].data.fd);
						epoll_ctl(token_epoll_fd_, EPOLL_CTL_DEL, receive_token_events_[n].data.fd, NULL);
						reading = false;
					  }
					else if (sts == sizeof(detail::RoutingToken) && buff.header != TOKEN_MAGIC)
					  {
						TLOG(TLVL_ERROR) << "Received invalid token from " << receive_token_addrs_[receive_token_events_[n].data.fd] << " sts=" << sts;
						reading = false;
					  }
					else if(sts == sizeof(detail::RoutingToken))
					  {
						sts = 0;
						TLOG(TLVL_DEBUG) << "Received token from " << buff.rank << " indicating " << buff.new_slots_free << " slots are free." ;
						received_token_count_ += buff.new_slots_free;
						if (routing_mode_ == detail::RoutingMasterMode::RouteBySequenceID)
						  {
							policy_->AddReceiverToken(buff.rank, buff.new_slots_free);
						  }
						else if (routing_mode_ == detail::RoutingMasterMode::RouteBySendCount)
						  {
							if (!received_token_counter_.count(buff.rank)) received_token_counter_[buff.rank] = 0;
							received_token_counter_[buff.rank] += buff.new_slots_free;
							TLOG(TLVL_DEBUG) << "RoutingMasterMode is RouteBySendCount. I have " << received_token_counter_[buff.rank] << " tokens for rank " << buff.rank << " and I need " << sender_ranks_.size() << "." ;
							while (received_token_counter_[buff.rank] >= sender_ranks_.size())
							  {
								TLOG(TLVL_DEBUG) << "RoutingMasterMode is RouteBySendCount. I have " << received_token_counter_[buff.rank] << " tokens for rank " << buff.rank << " and I need " << sender_ranks_.size()
												 << "... Sending token to policy" ;
								policy_->AddReceiverToken(buff.rank, 1);
								received_token_counter_[buff.rank] -= sender_ranks_.size();
							  }
						  }
					  }
				  }
				auto delta_time = artdaq::MonitoredQuantity::getCurrentTime() - startTime;
				statsHelper_.addSample(TOKENS_RECEIVED_STAT_KEY, delta_time);
				bool readyToReport = statsHelper_.readyToReport(delta_time);
				if (readyToReport)
				{
				std::string statString = buildStatisticsString_();
					TLOG(TLVL_INFO) << statString;
				sendMetrics_();
			}
	}
}
	}
}

void artdaq::RoutingMasterCore::start_recieve_token_thread_()
{
	if (ev_token_receive_thread_.joinable()) ev_token_receive_thread_.join();
	boost::thread::attributes attrs;
	attrs.set_stack_size(4096 * 2000); // 8000 KB
	
	TLOG(TLVL_INFO) << "Starting Token Reception Thread" ;
	try {
		ev_token_receive_thread_ = boost::thread(attrs, boost::bind(&RoutingMasterCore::receive_tokens_, this));
	}
	catch(boost::exception const& e)
	{
		std::cerr << "Exception encountered starting Token Reception thread: " << boost::diagnostic_information(e) << ", errno=" << errno << std::endl;
		exit(3);
	}
	TLOG(TLVL_INFO) << "Started Token Reception Thread";
}

std::string artdaq::RoutingMasterCore::report(std::string const&) const
{
	std::string resultString;

	// if we haven't been able to come up with any report so far, say so
	auto tmpString = app_name + " run number = " + std::to_string(run_id_.run())
		+ ", table updates sent = " + std::to_string(table_update_count_)
		+ ", Receiver tokens received = " + std::to_string(received_token_count_);
	return tmpString;
}

std::string artdaq::RoutingMasterCore::buildStatisticsString_() const
{
	std::ostringstream oss;
	oss << app_name << " statistics:" << std::endl;

	auto mqPtr = artdaq::StatisticsCollection::getInstance().getMonitoredQuantity(TABLE_UPDATES_STAT_KEY);
	if (mqPtr.get() != nullptr)
	{
		artdaq::MonitoredQuantityStats stats;
		mqPtr->getStats(stats);
		oss << "  Table Update statistics: "
			<< stats.recentSampleCount << " table updates sent at "
			<< stats.recentSampleRate << " table updates/sec, , monitor window = "
			<< stats.recentDuration << " sec" << std::endl;
		oss << "  Average times per table update: ";
		if (stats.recentSampleRate > 0.0)
		{
			oss << " elapsed time = "
				<< (1.0 / stats.recentSampleRate) << " sec";
		}
		oss << ", avg table acknowledgement wait time = "
			<< (mqPtr->getRecentValueSum() / sender_ranks_.size()) << " sec" << std::endl;
	}

	mqPtr = artdaq::StatisticsCollection::getInstance().getMonitoredQuantity(TOKENS_RECEIVED_STAT_KEY);
	if (mqPtr.get() != nullptr)
	{
		artdaq::MonitoredQuantityStats stats;
		mqPtr->getStats(stats);
		oss << "  Received Token statistics: "
			<< stats.recentSampleCount << " tokens received at "
			<< stats.recentSampleRate << " tokens/sec, , monitor window = "
			<< stats.recentDuration << " sec" << std::endl;
		oss << "  Average times per token: ";
		if (stats.recentSampleRate > 0.0)
		{
			oss << " elapsed time = "
				<< (1.0 / stats.recentSampleRate) << " sec";
		}
		oss << ", input token wait time = "
			<< mqPtr->getRecentValueSum() << " sec" << std::endl;
	}

	return oss.str();
}

void artdaq::RoutingMasterCore::sendMetrics_()
{
	auto mqPtr = artdaq::StatisticsCollection::getInstance().getMonitoredQuantity(TABLE_UPDATES_STAT_KEY);
	if (mqPtr.get() != nullptr)
	{
		artdaq::MonitoredQuantityStats stats;
		mqPtr->getStats(stats);
		metricMan->sendMetric("Table Update Count", static_cast<unsigned long>(stats.fullSampleCount), "updates", 1, MetricMode::LastPoint);
		metricMan->sendMetric("Table Update Rate", stats.recentSampleRate, "updates/sec", 1, MetricMode::Average);
		metricMan->sendMetric("Average Sender Acknowledgement Time", (mqPtr->getRecentValueSum() / sender_ranks_.size()), "seconds", 3, MetricMode::Average);
	}

	mqPtr = artdaq::StatisticsCollection::getInstance().getMonitoredQuantity(TOKENS_RECEIVED_STAT_KEY);
	if (mqPtr.get() != nullptr)
	{
		artdaq::MonitoredQuantityStats stats;
		mqPtr->getStats(stats);
		metricMan->sendMetric("Receiver Token Count", static_cast<unsigned long>(stats.fullSampleCount), "updates", 1, MetricMode::LastPoint);
		metricMan->sendMetric("Receiver Token Rate", stats.recentSampleRate, "updates/sec", 1, MetricMode::Average);
		metricMan->sendMetric("Total Receiver Token Wait Time", mqPtr->getRecentValueSum(), "seconds", 3, MetricMode::Average);
	}
}
