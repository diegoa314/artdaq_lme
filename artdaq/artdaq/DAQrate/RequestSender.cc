#define TRACE_NAME (app_name + "_RequestSender").c_str()
#include "artdaq/DAQdata/Globals.hh" // Before trace.h gets included in ConcurrentQueue (from GlobalQueue)
#include "artdaq/DAQrate/RequestSender.hh"
#include <utility>
#include <cstring>
#include <dlfcn.h>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <chrono>

#include "cetlib_except/exception.h"
#include "artdaq-core/Core/StatisticsCollection.hh"
#include "artdaq-core/Core/SimpleMemoryReader.hh"
#include "artdaq/DAQrate/detail/RoutingPacket.hh"
#include "artdaq/DAQdata/TCPConnect.hh"

namespace artdaq
{
	RequestSender::RequestSender(const fhicl::ParameterSet& pset)
		: send_requests_(pset.get<bool>("send_requests", false))
		, initialized_(false)
		, active_requests_()
		, request_address_(pset.get<std::string>("request_address", "227.128.12.26"))
		, request_port_(pset.get<int>("request_port", 3001))
		, request_delay_(pset.get<size_t>("request_delay_ms", 0) * 1000)
		, request_shutdown_timeout_us_(pset.get<size_t>("request_shutdown_timeout_us", 100000))
		, multicast_out_addr_(pset.get<std::string>("multicast_interface_ip", pset.get<std::string>("output_address", "0.0.0.0")))
		, request_mode_(detail::RequestMessageMode::Normal)
		, token_socket_(-1)
		, request_sending_(0)
	        , tokens_sent_(0)
	{
		TLOG(TLVL_DEBUG) << "RequestSender CONSTRUCTOR";
		setup_requests_();

		auto rmConfig = pset.get<fhicl::ParameterSet>("routing_token_config", fhicl::ParameterSet());
		send_routing_tokens_ = rmConfig.get<bool>("use_routing_master", false);
		token_port_ = rmConfig.get<int>("routing_token_port", 35555);
		token_address_ = rmConfig.get<std::string>("routing_master_hostname", "localhost");
		setup_tokens_();
		TLOG(12) << "artdaq::RequestSender::RequestSender ctor - reader_thread_ initialized";
		initialized_ = true;
	}


	RequestSender::~RequestSender()
	{
		TLOG(TLVL_INFO) << "Shutting down RequestSender: Waiting for " << request_sending_.load() << " requests to be sent";

		auto start_time = std::chrono::steady_clock::now();

		while (request_sending_.load() > 0 && request_shutdown_timeout_us_ + request_delay_ > TimeUtils::GetElapsedTimeMicroseconds(start_time))
		{
			usleep(1000);
		}
		{
			std::unique_lock<std::mutex> lk(request_mutex_);
			std::unique_lock<std::mutex> lk2(request_send_mutex_);
		}
		TLOG(TLVL_INFO) << "Shutting down RequestSender";
		if (request_socket_ > 0)
		{
			shutdown(request_socket_, 2);
			close(request_socket_);
		}
		if (token_socket_ > 0)
		{
			shutdown(token_socket_, 2);
			close(token_socket_);
		}
	}


	void RequestSender::SetRequestMode(detail::RequestMessageMode mode)
	{
		request_mode_ = mode;
		SendRequest(true);
	}

	void RequestSender::setup_requests_()
	{
		if (send_requests_)
		{
			request_socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			if (request_socket_ < 0)
			{
				TLOG(TLVL_ERROR) << "I failed to create the socket for sending Data Requests! err=" << strerror(errno);
				exit(1);
			}
			int sts = ResolveHost(request_address_.c_str(), request_port_, request_addr_);
			if (sts == -1)
			{
				TLOG(TLVL_ERROR) << "Unable to resolve Data Request address, err=" << strerror(errno);
				exit(1);
			}

			if (multicast_out_addr_ == "0.0.0.0")
			{
				multicast_out_addr_.reserve(HOST_NAME_MAX);
				sts = gethostname(&multicast_out_addr_[0], HOST_NAME_MAX);
				if (sts < 0)
				{
					TLOG(TLVL_ERROR) << "Could not get current hostname,  err=" << strerror(errno);
					exit(1);
				}
			}

			if (multicast_out_addr_ != "localhost")
			{
				struct in_addr addr;
				sts = GetInterfaceForNetwork(multicast_out_addr_.c_str(), addr);
				//sts = ResolveHost(multicast_out_addr_.c_str(), addr);
				if (sts == -1)
				{
					TLOG(TLVL_ERROR) << "Unable to resolve multicast interface address, err=" << strerror(errno);
					exit(1);
				}

				if (setsockopt(request_socket_, IPPROTO_IP, IP_MULTICAST_IF, &addr, sizeof(addr)) == -1)
				{
					TLOG(TLVL_ERROR) << "Cannot set outgoing interface, err=" << strerror(errno);
					exit(1);
				}
			}
			int yes = 1;
			if (setsockopt(request_socket_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
			{
				TLOG(TLVL_ERROR) << "Unable to enable port reuse on request socket, err=" << strerror(errno);
				exit(1);
			}
			if (setsockopt(request_socket_, IPPROTO_IP, IP_MULTICAST_LOOP, &yes, sizeof(yes)) < 0)
			{
				TLOG(TLVL_ERROR) << "Unable to enable multicast loopback on request socket, err=" << strerror(errno);
				exit(1);
			}
			if (setsockopt(request_socket_, SOL_SOCKET, SO_BROADCAST, (void*)&yes, sizeof(int)) == -1)
			{
				TLOG(TLVL_ERROR) << "Cannot set request socket to broadcast, err=" << strerror(errno);
				exit(1);
			}
		}
	}

	void
		RequestSender::setup_tokens_()
	{
		if (send_routing_tokens_)
		{
			TLOG(TLVL_DEBUG) << "Creating Routing Token sending socket";
			int retry = 5;
			while (retry > 0 && token_socket_ < 0)
			{
				token_socket_ = TCPConnect(token_address_.c_str(), token_port_, 0, sizeof(detail::RoutingToken));
				if (token_socket_ < 0) usleep(100000);
				retry--;
			}
			if (token_socket_ < 0)
			{
				TLOG(TLVL_ERROR) << "I failed to create the socket for sending Routing Tokens! err=" << strerror(errno);
				exit(1);
			}
			TLOG(TLVL_DEBUG) << "Routing Token sending socket created successfully";
		}
	}

	void RequestSender::do_send_request_()
	{
		if (!send_requests_) {
			request_sending_--;
			return;
		}
		if (request_socket_ == -1) setup_requests_();

		TLOG(TLVL_TRACE) << "Waiting for " << request_delay_ << " microseconds.";
		std::this_thread::sleep_for(std::chrono::microseconds(request_delay_));

		TLOG(TLVL_TRACE) << "Creating RequestMessage";
		detail::RequestMessage message;
		{
			std::unique_lock<std::mutex> lk(request_mutex_);
			for (auto& req : active_requests_)
			{
				TLOG(12) << "Adding a request with sequence ID " << req.first << ", timestamp " << req.second << " to request message";
				message.addRequest(req.first, req.second);
			}
		}
		TLOG(TLVL_TRACE) << "Setting mode flag in Message Header";
		message.setMode(request_mode_);
		char str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(request_addr_.sin_addr), str, INET_ADDRSTRLEN);
		std::unique_lock<std::mutex> lk2(request_send_mutex_);
		TLOG(TLVL_TRACE) << "Sending request for " << message.size() << " events to multicast group " << str;
		auto buf = message.GetMessage();
		auto sts=sendto(request_socket_, &buf[0], buf.size(), 0, (struct sockaddr *)&request_addr_, sizeof(request_addr_));
		if (sts < 0 || static_cast<size_t>(sts) != buf.size())
		{
			TLOG(TLVL_ERROR) << "Error sending request message err=" << strerror(errno)<< "sts="<<sts;
			request_socket_ = -1;
			request_sending_--;
			return;
		}
		TLOG(TLVL_TRACE) << "Done sending request sts="<<sts;
		request_sending_--;
	}

	void RequestSender::send_routing_token_(int nSlots)
	{
		TLOG(TLVL_TRACE) << "send_routing_token_ called, send_routing_tokens_=" << std::boolalpha << send_routing_tokens_;
		if (!send_routing_tokens_) return;
		if (token_socket_ == -1) setup_tokens_();
		detail::RoutingToken token;
		token.header = TOKEN_MAGIC;
		token.rank = my_rank;
		token.new_slots_free = nSlots;

		TLOG(TLVL_TRACE) << "Sending RoutingToken to " << token_address_ << ":" << token_port_;
		size_t sts = 0;
		while (sts < sizeof(detail::RoutingToken))
		{
			auto res = send(token_socket_, reinterpret_cast<uint8_t*>(&token) + sts, sizeof(detail::RoutingToken) - sts, 0);
			if (res == -1)
			{
				close(token_socket_);
				token_socket_ = -1;
				sts = 0;
				setup_tokens_();
				continue;
			}
			sts += res;
		}
		tokens_sent_ += nSlots;
		TLOG(TLVL_TRACE) << "Done sending RoutingToken to " << token_address_ << ":" << token_port_;
	}

	void RequestSender::SendRoutingToken(int nSlots)
	{
		while (!initialized_) usleep(1000);
		if (!send_routing_tokens_) return;
		boost::thread token([=] { send_routing_token_(nSlots); });
		token.detach();
		usleep(0); // Give up time slice
	}

	void RequestSender::SendRequest(bool endOfRunOnly)
	{
		while (!initialized_) usleep(1000);

		if (!send_requests_) return;
		if (endOfRunOnly && request_mode_ != detail::RequestMessageMode::EndOfRun) return;
		request_sending_++;
		boost::thread request([=] { do_send_request_(); });
		request.detach();
	}

	void RequestSender::AddRequest(Fragment::sequence_id_t seqID, Fragment::timestamp_t timestamp)
	{
		while (!initialized_) usleep(1000);

		{
			std::lock_guard<std::mutex> lk(request_mutex_);
			if (!active_requests_.count(seqID)) {
				TLOG(12) << "Adding request for sequence ID " << seqID << " and timestamp " << timestamp << " to request list.";
				active_requests_[seqID] = timestamp;
			}
		}
		SendRequest();
	}

	void RequestSender::RemoveRequest(Fragment::sequence_id_t seqID)
	{
		while (!initialized_) usleep(1000);
		std::lock_guard<std::mutex> lk(request_mutex_);
		TLOG(12) << "Removing request for sequence ID " << seqID << " from request list.";
		active_requests_.erase(seqID);
	}
}
