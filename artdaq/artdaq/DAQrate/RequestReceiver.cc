#define TRACE_NAME (app_name + "_RequestReceiver").c_str()
#include "artdaq/DAQdata/Globals.hh"

#include "artdaq/DAQrate/RequestReceiver.hh"
#include "artdaq/DAQdata/Globals.hh"
#include "artdaq/DAQrate/detail/RequestMessage.hh"

#include <boost/exception/all.hpp>
#include <boost/throw_exception.hpp>

#include <limits>
#include <iterator>

#include "canvas/Utilities/Exception.h"
#include "cetlib_except/exception.h"
#include "fhiclcpp/ParameterSet.h"

#include "artdaq-core/Utilities/SimpleLookupPolicy.hh"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Data/ContainerFragmentLoader.hh"
#include "artdaq-core/Utilities/ExceptionHandler.hh"
#include "artdaq-core/Utilities/TimeUtils.hh"

#include <fstream>
#include <iomanip>
#include <iterator>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "artdaq/DAQdata/TCPConnect.hh"

artdaq::RequestReceiver::RequestReceiver()
	: request_port_(3001)
	, request_addr_("227.128.12.26")
	, running_(false)
	, requests_()
	, request_timing_()
	, request_stop_requested_(false)
	, request_received_(false)
	, end_of_run_timeout_ms_(1000)
	, should_stop_(false)
	, highest_seen_request_(0)
	, out_of_order_requests_()
	, request_increment_(1)
{}

artdaq::RequestReceiver::RequestReceiver(const fhicl::ParameterSet& ps)
	: request_port_(ps.get<int>("request_port", 3001))
	, request_addr_(ps.get<std::string>("request_address", "227.128.12.26"))
	, multicast_out_addr_(ps.get<std::string>("multicast_interface_ip", "0.0.0.0"))
	, running_(false)
	, requests_()
	, request_timing_()
	, request_stop_requested_(false)
	, request_received_(false)
	, end_of_run_timeout_ms_(ps.get<size_t>("end_of_run_quiet_timeout_ms", 1000))
	, should_stop_(false)
	, highest_seen_request_(0)
	, out_of_order_requests_()
	, request_increment_(ps.get<artdaq::Fragment::sequence_id_t>("request_increment", 1))
{
	setupRequestListener();
}

void artdaq::RequestReceiver::setupRequestListener()
{
	TLOG(TLVL_INFO) << "Setting up request listen socket, rank=" << my_rank << ", address=" << request_addr_ << ":" << request_port_;
	request_socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (request_socket_ < 0)
	{
		TLOG(TLVL_ERROR) << "Error creating socket for receiving data requests! err=" << strerror(errno);
		exit(1);
	}

	struct sockaddr_in si_me_request;

	int yes = 1;
	if (setsockopt(request_socket_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
	{
		TLOG(TLVL_ERROR) << "Unable to enable port reuse on request socket, err=" << strerror(errno);
		exit(1);
	}
	memset(&si_me_request, 0, sizeof(si_me_request));
	si_me_request.sin_family = AF_INET;
	si_me_request.sin_port = htons(request_port_);
	si_me_request.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(request_socket_, (struct sockaddr *)&si_me_request, sizeof(si_me_request)) == -1)
	{
		TLOG(TLVL_ERROR) << "Cannot bind request socket to port " << request_port_ << ", err=" << strerror(errno);
		exit(1);
	}

	if (request_addr_ != "localhost")
	{
		struct ip_mreq mreq;
		int sts = ResolveHost(request_addr_.c_str(), mreq.imr_multiaddr);
		if (sts == -1)
		{
			TLOG(TLVL_ERROR) << "Unable to resolve multicast request address, err=" << strerror(errno);
			exit(1);
		}
		sts = GetInterfaceForNetwork(multicast_out_addr_.c_str(), mreq.imr_interface);
		if (sts == -1)
		{
			TLOG(TLVL_ERROR) << "Unable to resolve hostname for " << multicast_out_addr_;
			exit(1);
		}
		if (setsockopt(request_socket_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
		{
			TLOG(TLVL_ERROR) << "Unable to join multicast group, err=" << strerror(errno);
			exit(1);
		}
	}
	TLOG(TLVL_INFO) << "Done setting up request socket, rank=" << my_rank;
}

artdaq::RequestReceiver::~RequestReceiver()
{
	stopRequestReceiverThread(true);
}

void artdaq::RequestReceiver::stopRequestReceiverThread(bool force)
{
	std::unique_lock<std::mutex> lk(state_mutex_);
	if (!request_received_ && !force)
	{
		TLOG(TLVL_ERROR) << "Stop request received by RequestReceiver, but no requests have ever been received." << std::endl
			<< "Check that UDP port " << request_port_ << " is open in the firewall config.";
	}
	should_stop_ = true;
	if (running_) {
		TLOG(TLVL_DEBUG) << "Joining requestThread";
		if (requestThread_.joinable()) requestThread_.join();
		bool once = true;
		while (running_) {
			if (once) TLOG(TLVL_ERROR) << "running_ is true after thread join! Should NOT happen";
			once = false;
			usleep(10000);
		}
	}

	if (request_socket_ != -1)
	{
		close(request_socket_);
		request_socket_ = -1;
	}
	request_received_ = false;
	highest_seen_request_ = 0;
}

void artdaq::RequestReceiver::startRequestReceiverThread()
{
	std::unique_lock<std::mutex> lk(state_mutex_);
	if (requestThread_.joinable()) requestThread_.join();
	should_stop_ = false;
	request_stop_requested_ = false;

	if (request_socket_ == -1)
	{
		TLOG(TLVL_INFO) << "Connecting Request Reception socket";
		setupRequestListener();
	}

	TLOG(TLVL_INFO) << "Starting Request Reception Thread";
	try {
		requestThread_ = boost::thread(&RequestReceiver::receiveRequestsLoop, this);
	}
	catch (const boost::exception& e)
	{
		TLOG(TLVL_ERROR) << "Caught boost::exception starting Request Receiver thread: " << boost::diagnostic_information(e) << ", errno=" << errno;
		std::cerr << "Caught boost::exception starting Request Receiver thread: " << boost::diagnostic_information(e) << ", errno=" << errno << std::endl;
		exit(5);
	}
	running_ = true;
}

void artdaq::RequestReceiver::receiveRequestsLoop()
{
	while (!should_stop_)
	{
		TLOG(16) << "receiveRequestsLoop: Polling Request socket for new requests";

		if (request_socket_ == -1)
		{
			setupRequestListener();
		}

		int ms_to_wait = 10;
		struct pollfd ufds[1];
		ufds[0].fd = request_socket_;
		ufds[0].events = POLLIN | POLLPRI | POLLERR;
		int rv = poll(ufds, 1, ms_to_wait);

		// Continue loop if no message received or message does not have correct event ID
		if (rv <= 0 || (ufds[0].revents != POLLIN && ufds[0].revents != POLLPRI))
		{
			if (rv == 1 && (ufds[0].revents & (POLLNVAL | POLLERR | POLLHUP)))
			{
				close(request_socket_);
				request_socket_ = -1;
			}
			if (request_stop_requested_ && TimeUtils::GetElapsedTimeMilliseconds(request_stop_timeout_) > end_of_run_timeout_ms_)
			{
				break;
			}
			continue;
		}

		TLOG(11) << "Received packet on Request channel";
		std::vector<uint8_t> buffer(MAX_REQUEST_MESSAGE_SIZE);
		struct sockaddr_in from;
		socklen_t          len = sizeof(from);
		auto sts = recvfrom(request_socket_, &buffer[0], MAX_REQUEST_MESSAGE_SIZE, 0, (struct sockaddr*)&from, &len);
		if (sts < 0)
		{
			TLOG(TLVL_ERROR) << "Error receiving request message header err=" << strerror(errno);
			close(request_socket_);
			request_socket_ = -1;
			continue;
		}

		auto hdr_buffer = reinterpret_cast<artdaq::detail::RequestHeader*>(&buffer[0]);
		TLOG(11) << "Request header word: 0x" << std::hex << hdr_buffer->header /*<< std::dec*/ << ", packet_count: " << hdr_buffer->packet_count << " from rank " << hdr_buffer->rank << ", " << inet_ntoa(from.sin_addr) << ":" << from.sin_port;
		if (!hdr_buffer->isValid()) continue;

		request_received_ = true;
		if (hdr_buffer->mode == artdaq::detail::RequestMessageMode::EndOfRun)
		{
			TLOG(TLVL_INFO) << "Received Request Message with the EndOfRun marker. (Re)Starting 1-second timeout for receiving all outstanding requests...";
			request_stop_timeout_ = std::chrono::steady_clock::now();
			request_stop_requested_ = true;
		}

		std::vector<artdaq::detail::RequestPacket> pkt_buffer(hdr_buffer->packet_count);
		memcpy(&pkt_buffer[0], &buffer[sizeof(artdaq::detail::RequestHeader)], sizeof(artdaq::detail::RequestPacket) * hdr_buffer->packet_count);
		bool anyNew = false;

		if (should_stop_) break;

		for (auto& buffer : pkt_buffer)
		{
			TLOG(20) << "Request Packet: hdr=" << /*std::dec <<*/ buffer.header << ", seq=" << buffer.sequence_id << ", ts=" << buffer.timestamp;
			if (!buffer.isValid()) continue;
			std::unique_lock<std::mutex> tlk(request_mutex_);
			if (requests_.count(buffer.sequence_id) && requests_[buffer.sequence_id] != buffer.timestamp)
			{
				TLOG(TLVL_ERROR) << "Received conflicting request for SeqID "
					<< buffer.sequence_id << "!"
					<< " Old ts=" << requests_[buffer.sequence_id]
					<< ", new ts=" << buffer.timestamp << ". Keeping OLD!";
			}
			else if (!requests_.count(buffer.sequence_id))
			{
				int delta = buffer.sequence_id - highest_seen_request_;
				TLOG(11) << "Received request for sequence ID " << buffer.sequence_id
					<< " and timestamp " << buffer.timestamp << " (delta: " << delta << ")";
				if (delta <= 0 || out_of_order_requests_.count(buffer.sequence_id))
				{
					TLOG(11) << "Already serviced this request ( sequence ID " << buffer.sequence_id << ")! Ignoring...";
				}
				else
				{
					requests_[buffer.sequence_id] = buffer.timestamp;
					request_timing_[buffer.sequence_id] = std::chrono::steady_clock::now();
					anyNew = true;
				}
			}
		}
		if (anyNew)
		{
			request_cv_.notify_all();
		}
	}
	TLOG(TLVL_DEBUG) << "Ending Request Thread";
	running_ = false;
}

void artdaq::RequestReceiver::RemoveRequest(artdaq::Fragment::sequence_id_t reqID)
{
	TLOG(10) << "RemoveRequest: Removing request for id " << reqID;
	std::unique_lock<std::mutex> lk(request_mutex_);
	requests_.erase(reqID);

	if (reqID > highest_seen_request_)
	{
		TLOG(10) << "RemoveRequest: out_of_order_requests_.size() == " << out_of_order_requests_.size() << ", reqID=" << reqID << ", expected=" << highest_seen_request_ + request_increment_;
		if (out_of_order_requests_.size() || reqID != highest_seen_request_ + request_increment_)
		{
			out_of_order_requests_.insert(reqID);

			auto it = out_of_order_requests_.begin();
			while (it != out_of_order_requests_.end() && !should_stop_) // Stop accounting for requests after stop
			{
				if (*it == highest_seen_request_ + request_increment_)
				{
					highest_seen_request_ = *it;
					it = out_of_order_requests_.erase(it);
				}
				else
				{
					break;
				}
			}
		}
		else // no out-of-order requests and this request is highest seen + request_increment_
		{
			highest_seen_request_ = reqID;
		}
		TLOG(10) << "RemoveRequest: reqID=" << reqID << " Setting highest_seen_request_ to " << highest_seen_request_;
	}
	if (metricMan && request_timing_.count(reqID))
	{
		metricMan->sendMetric("Request Response Time", TimeUtils::GetElapsedTime(request_timing_[reqID]), "seconds", 2, MetricMode::Average);
	}
	request_timing_.erase(reqID);
}
