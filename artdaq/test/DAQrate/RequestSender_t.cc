#define TRACE_NAME "RequestSender_t"

#include "artdaq/DAQrate/RequestSender.hh"

#define BOOST_TEST_MODULE RequestSender_t
#include "cetlib/quiet_unit_test.hpp"
#include "cetlib_except/exception.h"
#include "artdaq/DAQdata/TCP_listen_fd.hh"
#include "artdaq/DAQrate/detail/RoutingPacket.hh"
#include "artdaq/DAQdata/TCPConnect.hh"
#include <sys/poll.h>


BOOST_AUTO_TEST_SUITE(RequestSender_test)

#define TRACE_REQUIRE_EQUAL(l,r) do { \
if(l == r) { \
TLOG(TLVL_DEBUG) << __LINE__ << ": Checking if " << #l << " (" << l << ") equals " << #r << " (" << r << ")...YES!" ; \
} \
else \
{ \
	TLOG(TLVL_ERROR) << __LINE__ << ": Checking if " << #l << " (" << l << ") equals " << #r << " (" << r << ")...NO!" ; \
} \
  BOOST_REQUIRE_EQUAL(l, r); \
} while(0)

BOOST_AUTO_TEST_CASE(Construct)
{
	artdaq::configureMessageFacility("RequestSender_t", true, true);
	metricMan->initialize(fhicl::ParameterSet());
	metricMan->do_start();
	TLOG(TLVL_INFO) << "Construct Test Case BEGIN" ;
	fhicl::ParameterSet pset;
	artdaq::RequestSender t(pset);
	BOOST_REQUIRE_EQUAL(t.GetRequestMode(), artdaq::detail::RequestMessageMode::Normal);
	TLOG(TLVL_INFO) << "Construct Test Case END";
}

BOOST_AUTO_TEST_CASE(Tokens)
{
	artdaq::configureMessageFacility("RequestSender_t", true, true);
	metricMan->initialize(fhicl::ParameterSet());
	metricMan->do_start();
	TLOG(TLVL_INFO) << "Tokens Test Case BEGIN" ;
	const int TOKEN_PORT = (seedAndRandom() % (32768 - 1024)) + 1024;
	TLOG(TLVL_DEBUG) << "Opening token listener socket" ;
	auto token_socket = TCP_listen_fd(TOKEN_PORT, 3 * sizeof(artdaq::detail::RoutingToken));

	fhicl::ParameterSet token_pset;
	token_pset.put("routing_token_port", TOKEN_PORT);
	token_pset.put("use_routing_master", true);
	fhicl::ParameterSet pset;
	pset.put("routing_token_config", token_pset);
	artdaq::RequestSender t(pset);

	my_rank = 0;


	BOOST_REQUIRE(token_socket != -1);
	if (token_socket == -1)
	{
		TLOG(TLVL_ERROR) << "Token listener socket was not opened successfully." ;
		BOOST_REQUIRE_EQUAL(false, true);
		return;
	}

	TLOG(TLVL_DEBUG) << "Accepting new connection on token_socket" ;
	sockaddr_in addr;
	socklen_t arglen = sizeof(addr);
	auto conn_sock = accept(token_socket, (struct sockaddr*)&addr, &arglen);

	t.SendRoutingToken(120);

	artdaq::detail::RoutingToken buff;
	auto sts = read(conn_sock, &buff, sizeof(artdaq::detail::RoutingToken));

	TRACE_REQUIRE_EQUAL(sts, sizeof(artdaq::detail::RoutingToken));
	TRACE_REQUIRE_EQUAL(buff.header, TOKEN_MAGIC);
	TRACE_REQUIRE_EQUAL(buff.new_slots_free, 120);
	TRACE_REQUIRE_EQUAL(buff.rank, 0);

	my_rank = 13;
	t.SendRoutingToken(335);

	sts = read(conn_sock, &buff, sizeof(artdaq::detail::RoutingToken));

	TRACE_REQUIRE_EQUAL(sts, sizeof(artdaq::detail::RoutingToken));
	TRACE_REQUIRE_EQUAL(buff.header, TOKEN_MAGIC);
	TRACE_REQUIRE_EQUAL(buff.new_slots_free, 335);
	TRACE_REQUIRE_EQUAL(buff.rank, 13);

	close(conn_sock);
	close(token_socket);
	TLOG(TLVL_INFO) << "Tokens Test Case END";
}

BOOST_AUTO_TEST_CASE(Requests)
{
	artdaq::configureMessageFacility("RequestSender_t", true, true);
	metricMan->initialize(fhicl::ParameterSet());
	metricMan->do_start();
	TLOG(TLVL_INFO) << "Requests Test Case BEGIN" ;
	const int REQUEST_PORT = (seedAndRandom() % (32768 - 1024)) + 1024;
	const int DELAY_TIME = 100;
#if 0
	const std::string MULTICAST_IP = "227.28.12.28";
#else
	const std::string MULTICAST_IP = "localhost";
#endif
	fhicl::ParameterSet pset;
	pset.put("request_port", REQUEST_PORT);
	pset.put("request_delay_ms", DELAY_TIME);
	pset.put("send_requests", true);
	pset.put("request_address", MULTICAST_IP);
	artdaq::RequestSender t(pset);


	TLOG(TLVL_DEBUG) << "Opening request listener socket" ;
	auto request_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	struct sockaddr_in si_me_request;

	int yes = 1;
	if (setsockopt(request_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
	{
		TLOG(TLVL_ERROR) << "Unable to set reuse on request socket" ;
		BOOST_REQUIRE_EQUAL(true, false);
		return;
	}
	memset(&si_me_request, 0, sizeof(si_me_request));
	si_me_request.sin_family = AF_INET;
	si_me_request.sin_port = htons(REQUEST_PORT);
	si_me_request.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(request_socket, (struct sockaddr *)&si_me_request, sizeof(si_me_request)) == -1)
	{
		TLOG(TLVL_ERROR) << "Cannot bind request socket to port " << REQUEST_PORT ;
		BOOST_REQUIRE_EQUAL(true, false);
		return;
	}

	if (MULTICAST_IP != "localhost")
	{
		struct ip_mreq mreq;
		int sts = ResolveHost(MULTICAST_IP.c_str(), mreq.imr_multiaddr);
		if (sts == -1)
		{
			TLOG(TLVL_ERROR) << "Unable to resolve multicast request address" ;
			BOOST_REQUIRE_EQUAL(true, false);
			return;
		}
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);
		if (setsockopt(request_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
		{
			TLOG(TLVL_ERROR) << "Unable to join multicast group" ;
			BOOST_REQUIRE_EQUAL(true, false);
			return;
		}
	}

	TLOG(TLVL_DEBUG) << "Sending request" ;
	auto start_time = std::chrono::steady_clock::now();
	t.AddRequest(0, 0x10);
	struct pollfd ufds[1];

	TLOG(TLVL_DEBUG) << "Receiving Request" ;
	ufds[0].fd = request_socket;
	ufds[0].events = POLLIN | POLLPRI;
	int rv = poll(ufds, 1, 10000);
	if (rv > 0)
	{
		if (ufds[0].revents == POLLIN || ufds[0].revents == POLLPRI)
		{
			auto delay_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();
			BOOST_REQUIRE_GE(delay_time, DELAY_TIME);
			TLOG(TLVL_TRACE) << "Recieved packet on Request channel";
			std::vector<uint8_t> buffer(MAX_REQUEST_MESSAGE_SIZE);
			artdaq::detail::RequestHeader hdr_buffer;
			recv(request_socket, &buffer[0], buffer.size(), 0);
			memcpy(&hdr_buffer, &buffer[0], sizeof(artdaq::detail::RequestHeader));
			TRACE_REQUIRE_EQUAL(hdr_buffer.isValid(), true);
			TRACE_REQUIRE_EQUAL(static_cast<uint8_t>(hdr_buffer.mode),
								static_cast<uint8_t>(artdaq::detail::RequestMessageMode::Normal));
			TRACE_REQUIRE_EQUAL(hdr_buffer.packet_count, 1);
			if (hdr_buffer.isValid())
			{

				std::vector<artdaq::detail::RequestPacket> pkt_buffer(hdr_buffer.packet_count);
				memcpy(&pkt_buffer[0], &buffer[sizeof(artdaq::detail::RequestHeader)], sizeof(artdaq::detail::RequestPacket) * hdr_buffer.packet_count);

				for (auto& buffer : pkt_buffer)
				{
					TRACE_REQUIRE_EQUAL(buffer.isValid(), true);
					TRACE_REQUIRE_EQUAL(buffer.sequence_id, 0);
					TRACE_REQUIRE_EQUAL(buffer.timestamp, 0x10);
				}
			}
			else
			{
				TLOG(TLVL_ERROR) << "Invalid header received" ;
				BOOST_REQUIRE_EQUAL(false, true);
				return;
			}
		}
		else
		{
			TLOG(TLVL_ERROR) << "Wrong event type from poll" ;
			BOOST_REQUIRE_EQUAL(false, true);
			return;
		}
	}
	else
	{
		TLOG(TLVL_ERROR) << "Timeout occured waiting for request" ;
		BOOST_REQUIRE_EQUAL(false, true);
		return;
	}

	// SetRequestMode and AddRequest BOTH send requests...
	start_time = std::chrono::steady_clock::now();
	t.SetRequestMode(artdaq::detail::RequestMessageMode::EndOfRun);
	t.AddRequest(2, 0x20);
	rv = poll(ufds, 1, 1000);
	if (rv > 0)
	{
		if (ufds[0].revents == POLLIN || ufds[0].revents == POLLPRI)
		{
			auto delay_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();
			BOOST_REQUIRE_GE(delay_time, DELAY_TIME);
			TLOG(TLVL_TRACE) << "Recieved packet on Request channel";
			std::vector<uint8_t> buffer(MAX_REQUEST_MESSAGE_SIZE);
			artdaq::detail::RequestHeader hdr_buffer;
			recv(request_socket, &buffer[0], buffer.size(), 0);
			memcpy(&hdr_buffer, &buffer[0], sizeof(artdaq::detail::RequestHeader));
			TRACE_REQUIRE_EQUAL(hdr_buffer.isValid(), true);
			TRACE_REQUIRE_EQUAL(static_cast<uint8_t>(hdr_buffer.mode),
								static_cast<uint8_t>(artdaq::detail::RequestMessageMode::EndOfRun));
			TRACE_REQUIRE_EQUAL(hdr_buffer.packet_count, 2);
			if (hdr_buffer.isValid())
			{

				std::vector<artdaq::detail::RequestPacket> pkt_buffer(hdr_buffer.packet_count);
				memcpy(&pkt_buffer[0], &buffer[sizeof(artdaq::detail::RequestHeader)], sizeof(artdaq::detail::RequestPacket) * hdr_buffer.packet_count);

				TRACE_REQUIRE_EQUAL(pkt_buffer[0].isValid(), true);
				TRACE_REQUIRE_EQUAL(pkt_buffer[0].sequence_id, 0);
				TRACE_REQUIRE_EQUAL(pkt_buffer[0].timestamp, 0x10);
				TRACE_REQUIRE_EQUAL(pkt_buffer[1].isValid(), true);
				TRACE_REQUIRE_EQUAL(pkt_buffer[1].sequence_id, 2);
				TRACE_REQUIRE_EQUAL(pkt_buffer[1].timestamp, 0x20);

			}
			else
			{
				TLOG(TLVL_ERROR) << "Invalid header received" ;
				BOOST_REQUIRE_EQUAL(false, true);
				return;
			}
		}
		else
		{
			TLOG(TLVL_ERROR) << "Wrong event type from poll" ;
			BOOST_REQUIRE_EQUAL(false, true);
			return;
		}
	}
	else
	{
		TLOG(TLVL_ERROR) << "Timeout occured waiting for request" ;
		BOOST_REQUIRE_EQUAL(false, true);
		return;
	}
	rv = poll(ufds, 1, 1000);
	if (rv > 0)
	{
		if (ufds[0].revents == POLLIN || ufds[0].revents == POLLPRI)
		{
			auto delay_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();
			BOOST_REQUIRE_GE(delay_time, DELAY_TIME);
			TLOG(TLVL_TRACE) << "Recieved packet on Request channel";
			std::vector<uint8_t> buffer(MAX_REQUEST_MESSAGE_SIZE);
			artdaq::detail::RequestHeader hdr_buffer;
			recv(request_socket, &buffer[0], buffer.size(), 0);
			memcpy(&hdr_buffer, &buffer[0], sizeof(artdaq::detail::RequestHeader));
			TRACE_REQUIRE_EQUAL(hdr_buffer.isValid(), true);
			TRACE_REQUIRE_EQUAL(static_cast<uint8_t>(hdr_buffer.mode),
								static_cast<uint8_t>(artdaq::detail::RequestMessageMode::EndOfRun));
			TRACE_REQUIRE_EQUAL(hdr_buffer.packet_count, 2);
			if (hdr_buffer.isValid())
			{

				std::vector<artdaq::detail::RequestPacket> pkt_buffer(hdr_buffer.packet_count);
				memcpy(&pkt_buffer[0], &buffer[sizeof(artdaq::detail::RequestHeader)], sizeof(artdaq::detail::RequestPacket) * hdr_buffer.packet_count);

				TRACE_REQUIRE_EQUAL(pkt_buffer[0].isValid(), true);
				TRACE_REQUIRE_EQUAL(pkt_buffer[0].sequence_id, 0);
				TRACE_REQUIRE_EQUAL(pkt_buffer[0].timestamp, 0x10);
				TRACE_REQUIRE_EQUAL(pkt_buffer[1].isValid(), true);
				TRACE_REQUIRE_EQUAL(pkt_buffer[1].sequence_id, 2);
				TRACE_REQUIRE_EQUAL(pkt_buffer[1].timestamp, 0x20);

			}
			else
			{
				TLOG(TLVL_ERROR) << "Invalid header received" ;
				BOOST_REQUIRE_EQUAL(false, true);
				return;
			}
		}
		else
		{
			TLOG(TLVL_ERROR) << "Wrong event type from poll" ;
			BOOST_REQUIRE_EQUAL(false, true);
			return;
		}
	}
	else
	{
		TLOG(TLVL_ERROR) << "Timeout occured waiting for request" ;
		BOOST_REQUIRE_EQUAL(false, true);
		return;
	}

	t.RemoveRequest(0);
	t.RemoveRequest(2);
	t.AddRequest(3, 0x30);
	start_time = std::chrono::steady_clock::now();
	rv = poll(ufds, 1, 1000);
	if (rv > 0)
	{
		if (ufds[0].revents == POLLIN || ufds[0].revents == POLLPRI)
		{
			auto delay_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();
			BOOST_REQUIRE_GE(delay_time, DELAY_TIME);
			TLOG(TLVL_TRACE) << "Recieved packet on Request channel";
			std::vector<uint8_t> buffer(MAX_REQUEST_MESSAGE_SIZE);
			artdaq::detail::RequestHeader hdr_buffer;
			recv(request_socket, &buffer[0], buffer.size(), 0);
			memcpy(&hdr_buffer, &buffer[0], sizeof(artdaq::detail::RequestHeader));
			TRACE_REQUIRE_EQUAL(hdr_buffer.isValid(), true);
			TRACE_REQUIRE_EQUAL(static_cast<uint8_t>(hdr_buffer.mode),
								static_cast<uint8_t>(artdaq::detail::RequestMessageMode::EndOfRun));
			TRACE_REQUIRE_EQUAL(hdr_buffer.packet_count, 1);
			if (hdr_buffer.isValid())
			{

				std::vector<artdaq::detail::RequestPacket> pkt_buffer(hdr_buffer.packet_count);
				memcpy(&pkt_buffer[0], &buffer[sizeof(artdaq::detail::RequestHeader)], sizeof(artdaq::detail::RequestPacket) * hdr_buffer.packet_count);

				TRACE_REQUIRE_EQUAL(pkt_buffer[0].isValid(), true);
				TRACE_REQUIRE_EQUAL(pkt_buffer[0].sequence_id, 3);
				TRACE_REQUIRE_EQUAL(pkt_buffer[0].timestamp, 0x30);

			}
			else
			{
				TLOG(TLVL_ERROR) << "Invalid header received" ;
				BOOST_REQUIRE_EQUAL(false, true);
				return;
			}
		}
		else
		{
			TLOG(TLVL_ERROR) << "Wrong event type from poll" ;
			BOOST_REQUIRE_EQUAL(false, true);
			return;
		}
	}
	else
	{
		TLOG(TLVL_ERROR) << "Timeout occured waiting for request" ;
		BOOST_REQUIRE_EQUAL(false, true);
		return;
	}

	close(request_socket);
	TLOG(TLVL_INFO) << "Requests Test Case END";
	artdaq::Globals::CleanUpGlobals();
}

BOOST_AUTO_TEST_SUITE_END()
