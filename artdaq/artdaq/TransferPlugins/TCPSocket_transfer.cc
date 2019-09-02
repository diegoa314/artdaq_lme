// Sep 14, 2016. "TERMS AND CONDITIONS" governing this file are in the README
// or COPYING file. If you do not have such a file, one can be obtained by
// contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
// $RCSfile: .emacs.gnu,v $
// rev="$Revision: 1.30 $$Date: 2016/03/01 14:27:27 $";

// C Includes
#include <stdlib.h>				// atoi, strtoul
#include <sys/socket.h>         // socket, socklen_t
#include <sys/un.h>				// sockaddr_un
#include <arpa/inet.h>			// ntohl, ntohs
#include <sys/types.h>			// size_t
#include <poll.h>				// struct pollfd

// C++ Includes
#include <string>
#include <fstream>
#include <stdexcept>

// product Includes
#define TRACE_NAME (app_name + "_TCPSocketTransfer").c_str()
#include "artdaq/DAQdata/Globals.hh"

// artdaq Includes
#include "artdaq/TransferPlugins/TCPSocketTransfer.hh"
#include "artdaq/DAQdata/TCP_listen_fd.hh"
#include "artdaq/DAQdata/TCPConnect.hh"
#include "artdaq/TransferPlugins/detail/Timeout.hh"
#include "artdaq/TransferPlugins/detail/SRSockets.hh"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Utilities/TimeUtils.hh"
#include <iomanip>

std::atomic<int> artdaq::TCPSocketTransfer::listen_thread_refcount_(0);
std::unique_ptr<boost::thread> artdaq::TCPSocketTransfer::listen_thread_ = nullptr;
std::map<int, std::set<int>> artdaq::TCPSocketTransfer::connected_fds_ = std::map<int, std::set<int>>();
std::mutex artdaq::TCPSocketTransfer::listen_thread_mutex_;
std::mutex artdaq::TCPSocketTransfer::connected_fd_mutex_;

artdaq::TCPSocketTransfer::
TCPSocketTransfer(fhicl::ParameterSet const& pset, TransferInterface::Role role)
	: TransferInterface(pset, role)
	, send_fd_(-1)
	, active_receive_fd_(-1)
	, last_active_receive_fd_(-1)
	, rcvbuf_(pset.get<size_t>("tcp_receive_buffer_size", 0))
	, sndbuf_(pset.get<size_t>("tcp_send_buffer_size", max_fragment_size_words_ * sizeof(artdaq::RawDataType) * buffer_count_))
	, send_retry_timeout_us_(pset.get<size_t>("send_retry_timeout_us", 1000000))
	, timeoutMessageArmed_(true)
	, last_recv_time_()
	, receive_disconnected_wait_s_(pset.get<double>("receive_socket_disconnected_wait_s", 10.0))
	, receive_err_wait_us_(pset.get<size_t>("receive_socket_disconnected_wait_us", 10000))
	, receive_socket_has_been_connected_(false)
	, send_ack_diff_(0)
{
	TLOG(TLVL_DEBUG) << GetTraceName() << " Constructor: pset=" << pset.to_string() << ", role=" << (role == TransferInterface::Role::kReceive ? "kReceive" : "kSend");

	if (role == TransferInterface::Role::kReceive)
	{
		// Wait for sender to connect...
		TLOG(TLVL_DEBUG) << GetTraceName() << ": Listening for connections";
		start_listen_thread_();
		TLOG(TLVL_DEBUG) << GetTraceName() << ": Done Listening";
	}
	else
	{
		hostMap_ = MakeHostMap(pset);
		TLOG(TLVL_DEBUG) << GetTraceName() << ": Connecting to destination";
		connect_();
		TLOG(TLVL_DEBUG) << GetTraceName() << ": Done Connecting";
	}
	TLOG(TLVL_DEBUG) << GetTraceName() << ": End of Constructor";
}

artdaq::TCPSocketTransfer::~TCPSocketTransfer() noexcept
{
	TLOG(TLVL_DEBUG) << GetTraceName() << ": Shutting down TCPSocketTransfer";

	if (role() == TransferInterface::Role::kSend)
	{
		// close all open connections (send stop_v0) first
		MessHead mh = { 0,MessHead::stop_v0,htons(TransferInterface::source_rank()),{0} };
		if (send_fd_ != -1)
		{
			// should be blocking with modest timeo
			timeval tv = { 0,100000 };
			socklen_t len = sizeof(tv);
			setsockopt(send_fd_, SOL_SOCKET, SO_SNDTIMEO, &tv, len);
			write(send_fd_, &mh, sizeof(mh));
		}
		close(send_fd_);
		send_fd_ = -1;
	}
	else
	{
		{
			std::unique_lock<std::mutex> fd_lock(connected_fd_mutex_);
			if (connected_fds_.count(source_rank()))
			{
				auto it = connected_fds_[source_rank()].begin();
				while (it != connected_fds_[source_rank()].end())
				{
					close(*it);
					it = connected_fds_[source_rank()].erase(it);
				}
				connected_fds_.erase(source_rank());
			}
			if (ack_listen_thread_ && ack_listen_thread_->joinable()) ack_listen_thread_->join();
		}

		std::unique_lock<std::mutex> lk(listen_thread_mutex_);
		listen_thread_refcount_--;
		if (listen_thread_refcount_ <= 0 && listen_thread_ && listen_thread_->joinable())
		{
			listen_thread_->join();
		}
	}

	TLOG(TLVL_DEBUG) << GetTraceName() << ": End of Destructor";
}

int artdaq::TCPSocketTransfer::receiveFragmentHeader(detail::RawFragmentHeader& header, size_t timeout_usec)
{
	TLOG(5) << GetTraceName() << ": receiveFragmentHeader: BEGIN";
	int ret_rank = RECV_TIMEOUT;

	// Don't bomb out until received at least one connection...
	if (getConnectedFDCount(source_rank()) == 0)
	{ 	// what if just listen_fd??? 
		//	if (receive_socket_has_been_connected_ && TimeUtils::GetElapsedTime(last_recv_time_) > receive_disconnected_wait_s_) 
		//	{ 
		//			TLOG(TLVL_ERROR) << GetTraceName() << ": receiveFragmentHeader: senders have been disconnected for "
		//				<< TimeUtils::GetElapsedTime(last_recv_time_) << " s (receive_socket_disconnected_wait_s = " << receive_disconnected_wait_s_ << " s). RETURNING DATA_END!";
		//			return DATA_END; 
		//		}
		//if (++not_connected_count_ > receive_err_threshold_) { return DATA_END; }
		TLOG(7) << GetTraceName() << ": receiveFragmentHeader: Receive socket not connected, returning RECV_TIMEOUT";
		usleep(receive_err_wait_us_);
		return RECV_TIMEOUT;
	}
	receive_socket_has_been_connected_ = true;
	last_recv_time_ = std::chrono::steady_clock::now();

	TLOG(5) << GetTraceName() << ": receiveFragmentHeader timeout_usec=" << timeout_usec;
	//void* buff=alloca(max_fragment_size_words_*8);
	size_t byte_cnt = 0;
	int sts;
	int offset = 0;
	SocketState state = SocketState::Metadata;
	int target_bytes = sizeof(MessHead);
	uint64_t start_time_us = TimeUtils::gettimeofday_us();

	//while (active_receive_fd_ != -1) 
	//{
	//	TLOG(TLVL_TRACE) << GetTraceName() << ": Currently receiving from fd " << active_receive_fd_ << ", waiting!";
	//	usleep(1000);
	//}


	uint8_t* buff;

	int timeout_ms;
	if (timeout_usec == 0)
		timeout_ms = 0;
	else
		timeout_ms = (timeout_usec + 999) / 1000; // want at least 1 ms

	bool done = false;
	bool noDataWarningSent = false;
	int loop_guard = 0;

	while (!done && getConnectedFDCount(source_rank()) > 0)
	{
		if (active_receive_fd_ == -1)
		{
			loop_guard = 0;
			size_t fd_count = 0;
			std::vector<pollfd> pollfds;
			{
				std::unique_lock<std::mutex> lk(connected_fd_mutex_);
				fd_count = connected_fds_[source_rank()].size();
				pollfds.resize(fd_count);
				auto iter = connected_fds_[source_rank()].begin();
				for (size_t ii = 0; ii < fd_count; ++ii)
				{
					pollfds[ii].events = POLLIN | POLLPRI | POLLERR;
					pollfds[ii].fd = *iter;
					++iter;
				}
			}
			//TLOG(TLVL_DEBUG) << GetTraceName() << ": receiveFragment: Polling fd to see if there's data" ;
			int num_fds_ready = poll(&pollfds[0], fd_count, timeout_ms);
			if (num_fds_ready <= 0)
			{
				TLOG(5) << GetTraceName() << ": receiveFragmentHeader: No data on receive socket, returning RECV_TIMEOUT";
				return RECV_TIMEOUT;
			}

			size_t index = 0;
			if (last_active_receive_fd_ != -1)
			{
				for (auto& pollfd : pollfds)
				{
					index++;
					if (pollfd.fd == last_active_receive_fd_)
					{
						break;
					}
				}
			}

			int active_index = -1;
			short anomolous_events = 0;
			for (size_t ii = index; ii < index + pollfds.size(); ++ii)
			{
				auto pollfd_index = (ii + index) % pollfds.size();
				if (pollfds[pollfd_index].revents & (POLLIN | POLLPRI))
				{
					active_index = pollfd_index;
					active_receive_fd_ = pollfds[active_index].fd;
					active_revents_ = pollfds[active_index].revents;;
					break;
				}
				else if (pollfds[pollfd_index].revents & (POLLHUP | POLLERR))
				{
					disconnect_receive_socket_(pollfds[pollfd_index].fd, "Poll returned POLLHUP or POLLERR, indicating problems with the sender.");
					continue;
				}
				else if (pollfds[pollfd_index].revents & (POLLNVAL))
				{
					TLOG(TLVL_DEBUG) << GetTraceName() << ": receiveFragmentHeader: FD is closed, most likely because the peer went away. Removing from fd list.";
					disconnect_receive_socket_(pollfds[pollfd_index].fd, "FD is closed, most likely because the peer went away.");
					continue;
				}
				else if (pollfds[pollfd_index].revents)
				{
					anomolous_events |= pollfds[pollfd_index].revents;
				}
			}

			if (active_index == -1)
			{
				if (anomolous_events)
					TLOG(TLVL_DEBUG) << GetTraceName() << ": receiveFragmentHeader: Wrong event received from a pollfd. Mask: " << static_cast<int>(anomolous_events);
				active_receive_fd_ = -1;
				continue;
			}

			if (!done && timeout_usec > 0)
			{
				// calc next timeout_ms (unless timed out)
				size_t delta_us = TimeUtils::gettimeofday_us() - start_time_us;
				if (delta_us > timeout_usec)
				{
					return RECV_TIMEOUT;
				}
				timeout_ms = ((timeout_usec - delta_us) + 999) / 1000; // want at least 1 ms
			}
		}
		if (loop_guard > 10) { usleep(1000); }
		if (++loop_guard > 10010)
		{
			TLOG(TLVL_WARNING) << GetTraceName() << ": receiveFragmentHeader: loop guard triggered, returning RECV_TIMEOUT";
			usleep(receive_err_wait_us_);
			active_receive_fd_ = -1;
			return RECV_TIMEOUT;
		}

		if (state == SocketState::Metadata)
		{
			//TLOG(TLVL_DEBUG) << GetTraceName() << ": receiveFragmentHeader: Reading Message Header" ;
			buff = &(mha[offset]);
			byte_cnt = sizeof(MessHead) - offset;
		}
		else
		{
			//TLOG(TLVL_DEBUG) << GetTraceName() << ": receiveFragmentHeader: Reading data" ;
			buff = reinterpret_cast<uint8_t*>(&header) + offset;
			byte_cnt = target_bytes - offset;
		}
		//if (byte_cnt > sizeof(MessHead))
		//	{
		//	TLOG(TLVL_ERROR) << "Invalid byte count for read (count=" << byte_cnt
		//			 << ",offset=" << offset << ",mh.byte_count=" << mh.byte_count
		//			 << "), skipping read and returning RECV_TIMEOUT";
		//	return RECV_TIMEOUT;
		//}

		if (byte_cnt > 0)
		{
			TLOG(6) << GetTraceName() << ": receiveFragmentHeader: Reading " << byte_cnt << " bytes from socket";
			sts = read(active_receive_fd_, buff, byte_cnt);
			TLOG(6) << GetTraceName() << ": receiveFragmentHeader: Done with read";
		}
		if (sts > 0) { 
			loop_guard = 0;
			last_recv_time_ = std::chrono::steady_clock::now();
		}

		TLOG(7) << GetTraceName() << ": receiveFragmentHeader state=" << static_cast<int>(state) << " read=" << sts;
		if (sts < 0)
		{
			TLOG(TLVL_WARNING) << GetTraceName() << ": receiveFragmentHeader: Error on receive, closing socket " << " (errno=" << errno << ": " << strerror(errno) << ")";
			active_receive_fd_ = disconnect_receive_socket_(active_receive_fd_);
		}
		else if (sts == 0)
		{
			if (!noDataWarningSent) {
				TLOG(TLVL_WARNING) << GetTraceName() << ": receiveFragmentHeader: No data received, is the sender still sending?!?";
				noDataWarningSent = true;
			}
			if (TimeUtils::GetElapsedTime(last_recv_time_) > receive_disconnected_wait_s_)
			{
				TLOG(TLVL_ERROR) << GetTraceName() << ": receiveFragmentHeader: No data received within timeout, aborting!";
				return RECV_TIMEOUT;
			}
		}
		else
		{
			// see if we're done (with this state)
			sts = offset += sts;
			if (sts >= target_bytes)
			{
				TLOG(7) << GetTraceName() << ": receiveFragmentHeader: Target read bytes reached. Changing state";
				offset = 0;
				if (state == SocketState::Metadata)
				{
					state = SocketState::Data;
					mh.byte_count = ntohl(mh.byte_count);
					mh.source_id = ntohs(mh.source_id);
					target_bytes = mh.byte_count;
					TLOG(7) << GetTraceName() << ": receiveFragmentHeader: Expected header size = " << target_bytes << ", sizeof(RawFragmentHeader) = " << sizeof(artdaq::detail::RawFragmentHeader);
					//assert(target_bytes == sizeof(artdaq::detail::RawFragmentHeader) || target_bytes == 0);

					if (mh.message_type == MessHead::stop_v0)
					{
						active_receive_fd_ = disconnect_receive_socket_(active_receive_fd_, "Stop Message received.");
					}
					else if (mh.message_type == MessHead::data_v0 || mh.message_type == MessHead::data_more_v0)
					{
						TLOG(TLVL_WARNING) << GetTraceName() << ": receiveFragmentHeader: Message header indicates that Fragment data follows when I was expecting a Fragment header!";
						active_receive_fd_ = disconnect_receive_socket_(active_receive_fd_, "Desync detected");
					}

					if (target_bytes == 0)
					{
						//Probably a stop_v0, return timeout so we can try again.
						return RECV_TIMEOUT;
					}
				}
				else
				{
					ret_rank = source_rank();
					TLOG(8) << GetTraceName() << ": receiveFragmentHeader done sts=" << sts << " src=" << ret_rank;
					TLOG(7) << GetTraceName() << ": receiveFragmentHeader: Done receiving fragment header. Moving into output.";

					done = true; // no more polls
					//break; // no more read of ready fds
				}
			}
		}

	} // while(!done)...poll

	TLOG(5) << GetTraceName() << ": receiveFragmentHeader: Returning " << ret_rank;
	return ret_rank;
}

int artdaq::TCPSocketTransfer::disconnect_receive_socket_(int fd, std::string msg)
{
	TLOG(TLVL_WARNING) << GetTraceName() << ": disconnect_receive_socket_: " << msg << " Closing socket " << fd;
	close(fd);
	std::unique_lock<std::mutex> lk(connected_fd_mutex_);
	if (connected_fds_.count(source_rank()))
		connected_fds_[source_rank()].erase(fd);
	fd = -1;
	TLOG(TLVL_DEBUG) << GetTraceName() << ": disconnect_receive_socket_: There are now " << connected_fds_[source_rank()].size() << " active senders.";
	return fd;
}

int artdaq::TCPSocketTransfer::receiveFragmentData(RawDataType* destination, size_t)
{
	TLOG(9) << GetTraceName() << ": receiveFragmentData: BEGIN";
	int ret_rank = RECV_TIMEOUT;
	if (active_receive_fd_ == -1)
	{ // what if just listen_fd??? 
		TLOG(TLVL_ERROR) << GetTraceName() << ": receiveFragmentData: Receive socket not connected, returning RECV_TIMEOUT (Will result in \"Unexpected return code error\")";
		return RECV_TIMEOUT;
	}

	//void* buff=alloca(max_fragment_size_words_*8);
	uint8_t* buff;
	size_t byte_cnt = 0;
	int sts;
	int offset = 0;
	SocketState state = SocketState::Metadata;
	int target_bytes = sizeof(MessHead);

	pollfd pollfd_s;
	pollfd_s.events = POLLIN | POLLPRI | POLLERR;
	pollfd_s.fd = active_receive_fd_;

	int loop_guard = 0;
	bool done = false;
	bool noDataWarningSent = false;
	last_recv_time_ = std::chrono::steady_clock::now();
	while (!done)
	{
		TLOG(9) << GetTraceName() << ": receiveFragmentData: Polling fd to see if there's data";
		int num_fds_ready = poll(&pollfd_s, 1, 1000);
		TLOG(TLVL_TRACE) << GetTraceName() << ": receiveFragmentData: Polled fd to see if there's data"
			<< ", num_fds_ready = " << num_fds_ready;
		if (num_fds_ready <= 0)
		{
			if (num_fds_ready == 0)
			{
				TLOG(TLVL_WARNING) << GetTraceName() << ": receiveFragmentData: No data from " << source_rank() << " in " << TimeUtils::GetElapsedTimeMilliseconds(last_recv_time_) << " ms!"
				                   << " State = " << (state == SocketState::Metadata ? "Metadata" : "Data") << ", recvd/total=" << offset << "/" << target_bytes << " (delta=" << target_bytes - offset << ")";
				
				if (TimeUtils::GetElapsedTime(last_recv_time_) > receive_disconnected_wait_s_)
				{
					TLOG(TLVL_WARNING) << GetTraceName() << ": receiveFragmentData: No data received within timeout (" << TimeUtils::GetElapsedTime(last_recv_time_) << " / " << receive_disconnected_wait_s_ << " ), returning RECV_TIMEOUT";
					disconnect_receive_socket_(active_receive_fd_, "No data on this socket within timeout");
					active_receive_fd_ = -1;
					return RECV_TIMEOUT;
				}
				continue;
			}

			TLOG(TLVL_ERROR) << "Error in poll: errno=" << errno;
			active_receive_fd_ = -1;
			break;
		}
		else { last_recv_time_ = std::chrono::steady_clock::now(); }

		if (pollfd_s.revents & (POLLIN | POLLPRI))
		{
			// Expected, don't have to check revents any further
		}
		else if (pollfd_s.revents & (POLLNVAL))
		{
			disconnect_receive_socket_(pollfd_s.fd, "FD is closed, most likely because the peer went away.");
			break;
		}
		else if (pollfd_s.revents & (POLLHUP | POLLERR))
		{
			disconnect_receive_socket_(pollfd_s.fd, "Poll returned POLLHUP or POLLERR, indicating problems with the sender.");
			break;
		}
		else
		{
			TLOG(TLVL_WARNING) << GetTraceName() << ": receiveFragmentData: Wrong event received from pollfd: " << pollfd_s.revents;
			disconnect_receive_socket_(pollfd_s.fd);
			break;
		}

		if (state == SocketState::Metadata)
		{
			//TLOG(TLVL_DEBUG) << GetTraceName() << ": receiveFragmentData: Reading Message Header" ;
			buff = &(mha[offset]);
			byte_cnt = sizeof(MessHead) - offset;
		}
		else
		{
			//TLOG(TLVL_DEBUG) << GetTraceName() << ": receiveFragmentData: Reading data" ;
			buff = reinterpret_cast<uint8_t*>(destination) + offset;
			byte_cnt = mh.byte_count - offset;
		}

		TLOG(10) << GetTraceName() << ": receiveFragmentData: Reading " << byte_cnt << " bytes from socket into " << (void*)buff;
		sts = read(active_receive_fd_, buff, byte_cnt);
		//TLOG(TLVL_DEBUG) << GetTraceName() << ": receiveFragmentData: Done with read" ;

		TLOG(10) << GetTraceName() << ": recvFragment state=" << static_cast<int>(state) << " read=" << sts;

		if (sts == 0)
		{
			if (loop_guard > 10) { usleep(1000); }
			if (++loop_guard > 10010)
			{
				TLOG(TLVL_WARNING) << GetTraceName() << ": receiveFragmentData: loop guard triggered, returning RECV_TIMEOUT";
				active_receive_fd_ = -1;
				return RECV_TIMEOUT;
			}
		}
		else if(sts > 0)
		{
			loop_guard = 0;
			last_recv_time_ = std::chrono::steady_clock::now();
		 }

		if (sts < 0)
		{
			TLOG(TLVL_WARNING) << GetTraceName() << ": receiveFragmentData: Error on receive, closing socket"
				<< " (errno=" << errno << ": " << strerror(errno) << ")";
			disconnect_receive_socket_(pollfd_s.fd);
		}
		else if (sts == 0)
		{
			if (!noDataWarningSent) {
				TLOG(TLVL_WARNING) << GetTraceName() << ": receiveFragmentData: No data received, is the sender still sending?!?";
				noDataWarningSent = true;
			}
			if (TimeUtils::GetElapsedTime(last_recv_time_) > receive_disconnected_wait_s_)
			{
				TLOG(TLVL_ERROR) << GetTraceName() << ": receiveFragmentData: No data received within timeout, aborting!";
				return RECV_TIMEOUT;
			}
		}
		else
		{
			// see if we're done (with this state)
			sts = offset += sts;
			if (sts >= target_bytes)
			{
				TLOG(9) << GetTraceName() << ": receiveFragmentData: Target read bytes reached. Changing state";
				offset = 0;
				if (state == SocketState::Metadata)
				{
					state = SocketState::Data;
					mh.byte_count = ntohl(mh.byte_count);
					mh.source_id = ntohs(mh.source_id);
					target_bytes = mh.byte_count;


					if (mh.message_type == MessHead::header_v0)
					{
						TLOG(TLVL_WARNING) << GetTraceName() << ": receiveFragmentData: Message header indicates that a Fragment header follows when I was expecting Fragment data!";
						active_receive_fd_ = disconnect_receive_socket_(active_receive_fd_, "Desync detected");
				}
			}
				else
				{
					ret_rank = source_rank();
					TLOG(11) << GetTraceName() << ": receiveFragmentData done sts=" << sts << " src=" << ret_rank;
					TLOG(9) << GetTraceName() << ": receiveFragmentData: Done receiving fragment. Moving into output.";

#if USE_ACKS
					send_ack_(active_receive_fd_);
#endif

					done = true; // no more polls
					//break; // no more read of ready fds
				}
				}
		}

		// Check if we were asked to do a 0-size receive
		if (target_bytes == 0 && state == SocketState::Data)
		{
			ret_rank = source_rank();
			TLOG(11) << GetTraceName() << ": receiveFragmentData done sts=" << sts << " src=" << ret_rank;
			TLOG(9) << GetTraceName() << ": receiveFragmentData: Done receiving fragment. Moving into output.";

#if USE_ACKS
			send_ack_(active_receive_fd_);
#endif

			done = true; // no more polls
		}

	} // while(!done)...poll

	last_active_receive_fd_ = active_receive_fd_;
	active_receive_fd_ = -1;

	TLOG(9) << GetTraceName() << ": receiveFragmentData: Returning " << ret_rank;
	return ret_rank;
}

bool artdaq::TCPSocketTransfer::isRunning()
{
	switch (role())
	{
	case TransferInterface::Role::kSend:
		return send_fd_ != -1;
	case TransferInterface::Role::kReceive:
		TLOG(TLVL_DEBUG) << GetTraceName() << ": isRunning: There are " << getConnectedFDCount(source_rank()) << " fds connected.";
		return getConnectedFDCount(source_rank()) > 0;
	}
	return false;
}

// Send the given Fragment. Return the rank of the destination to which
// the Fragment was sent OR -1 if to none.
artdaq::TransferInterface::CopyStatus artdaq::TCPSocketTransfer::sendFragment_(Fragment&& frag, size_t send_timeout_usec)
{
	TLOG(12) << GetTraceName() << ": sendFragment begin send of fragment with sequenceID="<<frag.sequenceID();
	artdaq::Fragment grab_ownership_frag = std::move(frag);

	reconnect_();
	// Send Fragment Header

#if USE_ACKS
	// Wait for fragments to be received
	while (static_cast<size_t>(send_ack_diff_) > buffer_count_) usleep(10000);
#endif

	iovec iov = { reinterpret_cast<void*>(grab_ownership_frag.headerAddress()),
		detail::RawFragmentHeader::num_words() * sizeof(RawDataType) };

	auto sts = sendData_(&iov, 1, send_retry_timeout_us_, true);
	auto start_time = std::chrono::steady_clock::now();
	//If it takes more than 10 seconds to write a Fragment header, give up
	while (sts == CopyStatus::kTimeout && (send_timeout_usec == 0 || TimeUtils::GetElapsedTimeMicroseconds(start_time) < send_timeout_usec) && TimeUtils::GetElapsedTimeMicroseconds(start_time) < 10000000)
	{
		TLOG(13) << GetTraceName() << ": sendFragment: Timeout sending fragment";
		sts = sendData_(&iov, 1, send_retry_timeout_us_, true);
		usleep(1000);
	}
	if (sts != CopyStatus::kSuccess) return sts;

	// Send Fragment Data

	iov = { reinterpret_cast<void*>(grab_ownership_frag.headerAddress() + detail::RawFragmentHeader::num_words()),
		grab_ownership_frag.sizeBytes() - detail::RawFragmentHeader::num_words() * sizeof(RawDataType)
};
	sts = sendData_(&iov, 1, send_retry_timeout_us_);
	start_time = std::chrono::steady_clock::now();
	while (sts == CopyStatus::kTimeout && (send_timeout_usec == 0 || TimeUtils::GetElapsedTimeMicroseconds(start_time) < send_timeout_usec) && TimeUtils::GetElapsedTimeMicroseconds(start_time) < 10000000)
	{
		TLOG(13) << GetTraceName() << ": sendFragment: Timeout sending fragment";
		sts = sendData_(&iov, 1, send_retry_timeout_us_);
		usleep(1000);
	}

#if USE_ACKS
	send_ack_diff_++;
#endif

	TLOG(12) << GetTraceName() << ": sendFragment returning " << CopyStatusToString(sts);
	return sts;
}

artdaq::TransferInterface::CopyStatus artdaq::TCPSocketTransfer::sendData_(const void* buf, size_t bytes, size_t send_timeout_usec, bool isHeader)
{
	TLOG(TLVL_DEBUG) << GetTraceName() << ": sendData_ Converting buf to iovec";
	iovec iov = { (void*)buf, bytes };
	return sendData_(&iov, 1, send_timeout_usec, isHeader);
}

artdaq::TransferInterface::CopyStatus artdaq::TCPSocketTransfer::sendData_(const struct iovec* iov, int iovcnt, size_t send_timeout_usec, bool isHeader)
{
	// check all connected??? -- currently just check fd!=-1
	if (send_fd_ == -1)
	{
		if (timeoutMessageArmed_)
		{
			TLOG(TLVL_DEBUG) << GetTraceName() << ": sendData_: Send fd is not open. Returning kTimeout";
			timeoutMessageArmed_ = false;
		}
		return CopyStatus::kTimeout;
	}
	timeoutMessageArmed_ = true;
	TLOG(14) << GetTraceName() << ": send_timeout_usec is " << send_timeout_usec << ", currently unused.";

	//TLOG(TLVL_DEBUG) << GetTraceName() << ": sendData_: Determining write size" ;
	uint32_t total_to_write_bytes = 0;
	std::vector<iovec> iov_in(iovcnt + 1); // need contiguous (for the unlike case that only partial MH
	std::vector<iovec> iovv(iovcnt + 2); // 1 more for mh and another one for any partial
	int ii;
	for (ii = 0; ii < iovcnt; ++ii)
	{
		iov_in[ii + 1] = iov[ii];
		total_to_write_bytes += iov[ii].iov_len;
	}
	//TLOG(TLVL_DEBUG) << GetTraceName() << ": sendData_: Constructing Message Header" ;
	MessHead mh = { 0,isHeader ? MessHead::header_v0 : MessHead::data_v0,htons(source_rank()),{htonl(total_to_write_bytes)} };
	iov_in[0].iov_base = &mh;
	iov_in[0].iov_len = sizeof(mh);
	total_to_write_bytes += sizeof(mh);

	ssize_t sts = 0;
	ssize_t total_written_bytes = 0;
	ssize_t per_write_max_bytes = (32 * 1024);

	size_t in_iov_idx = 0; // only increment this when we know the associated data has been xferred
	size_t out_iov_idx = 0;
	ssize_t this_write_bytes = 0;

	do
	{
		// The first out_iov may be set at the end of the previous loop.
		// iov looping from below (b/c of the latter, we need to check this_write_bytes)
		for (;
			(in_iov_idx + out_iov_idx) < iov_in.size() && this_write_bytes < per_write_max_bytes;
			++out_iov_idx)
		{
			this_write_bytes += iov_in[in_iov_idx + out_iov_idx].iov_len;
			iovv[out_iov_idx] = iov_in[in_iov_idx + out_iov_idx];
		}
		if (this_write_bytes > per_write_max_bytes)
		{
			iovv[out_iov_idx - 1].iov_len -= this_write_bytes - per_write_max_bytes;
			this_write_bytes = per_write_max_bytes;
		}

		// need to do blocking algorithm -- including throttled block notifications
	do_again:
#ifndef __OPTIMIZE__ // This can be an expensive TRACE call (even if disabled) due to multiplicity of calls
		TLOG(14) << GetTraceName() << ": sendFragment b4 writev " << std::setw(7) << total_written_bytes << " total_written_bytes send_fd_=" << send_fd_ << " in_idx=" << in_iov_idx
			<< " iovcnt=" << out_iov_idx << " 1st.len=" << iovv[0].iov_len;
#endif
		//TLOG(TLVL_DEBUG) << GetTraceName() << " calling writev" ;
		sts = writev(send_fd_, &(iovv[0]), out_iov_idx);
		//TLOG(TLVL_DEBUG) << GetTraceName() << " done with writev" ;

		if (sts == -1)
		{
			if (errno == EAGAIN /* same as EWOULDBLOCK */)
			{
				TLOG(TLVL_DEBUG) << GetTraceName() << ": sendFragment EWOULDBLOCK";
				fcntl(send_fd_, F_SETFL, 0); // clear O_NONBLOCK
				blocking = true;
				// NOTE: YES -- could drop here
				goto do_again;
			}
			TLOG(TLVL_WARNING) << GetTraceName() << ": sendFragment_: WRITE ERROR: " << strerror(errno);
			connect_state = 0; // any write error closes
			close(send_fd_);
			send_fd_ = -1;
			return TransferInterface::CopyStatus::kErrorNotRequiringException;
		}
		else if (sts != this_write_bytes)
		{
			// we'll loop around -- with
			TLOG(TLVL_DEBUG) << GetTraceName() << ": sendFragment writev sts(" << sts << ")!=requested_send_bytes(" << this_write_bytes << ")";
			total_written_bytes += sts; // add sts to total_written_bytes now as sts is adjusted next
			// find which iovs are done
			for (ii = 0; (size_t)sts >= iovv[ii].iov_len; ++ii)
				sts -= iovv[ii].iov_len;
			in_iov_idx += ii; // done with these in_iovs
			iovv[ii].iov_len -= sts; // adjust partial iov
			iovv[ii].iov_base = (uint8_t*)(iovv[ii].iov_base) + sts; // adjust partial iov

			// add more to get up to per_write_max_bytes
			out_iov_idx = 0;
			if (ii != 0)
				iovv[out_iov_idx] = iovv[ii];
			// starting over
			this_write_bytes = iovv[out_iov_idx].iov_len;
			// add any left over from appropriate in_iov_idx --
			// i.e. match this out_iov with the in_iov that was used to
			// initialize it; see how close the out base+len is to in base+len
			// check !>per_write_max_bytes
			unsigned long additional = ((unsigned long)iov_in[in_iov_idx].iov_base + iov_in[in_iov_idx].iov_len)
				- ((unsigned long)iovv[out_iov_idx].iov_base + iovv[out_iov_idx].iov_len);
			if (additional)
			{
				iovv[out_iov_idx].iov_len += additional;
				this_write_bytes += additional;
				if (this_write_bytes > per_write_max_bytes)
				{
					iovv[out_iov_idx].iov_len -= this_write_bytes - per_write_max_bytes;
					this_write_bytes = per_write_max_bytes;
				}
			}
			++out_iov_idx; // done with
			TLOG(TLVL_TRACE) << GetTraceName() << ": sendFragment writev sts!=: this_write_bytes=" << this_write_bytes
				<< " out_iov_idx=" << out_iov_idx
				<< " additional=" << additional
				<< " ii=" << ii;
		}
		else
		{
#ifndef __OPTIMIZE__ // This can be an expensive TRACE call (even if disabled) due to multiplicity of calls
			TLOG(TLVL_TRACE) << GetTraceName() << ": sendFragment writev sts(" << sts << ")==requested_send_bytes(" << this_write_bytes << ")";
#endif
			total_written_bytes += sts;
			--out_iov_idx; // make it the index of the last iovv
			iovv[out_iov_idx].iov_base = (uint8_t*)(iovv[out_iov_idx].iov_base) + iovv[out_iov_idx].iov_len;
			iovv[out_iov_idx].iov_len = 0;
			in_iov_idx += out_iov_idx; // at least this many complete (one more if "last iovv" is complete
			this_write_bytes = 0;
			// need to check last iovv against appropriate iov_in
			unsigned long additional = ((unsigned long)iov_in[in_iov_idx].iov_base + iov_in[in_iov_idx].iov_len)
				- ((unsigned long)iovv[out_iov_idx].iov_base + iovv[out_iov_idx].iov_len);
			if (additional)
			{
				iovv[out_iov_idx].iov_len += additional;
				this_write_bytes += additional;
				if (this_write_bytes > per_write_max_bytes)
				{
					iovv[out_iov_idx].iov_len -= this_write_bytes - per_write_max_bytes;
					this_write_bytes = per_write_max_bytes;
				}
				if (out_iov_idx != 0)
					iovv[0] = iovv[out_iov_idx];
				out_iov_idx = 1;
			}
			else
			{
				++in_iov_idx;
				out_iov_idx = 0;
			}
		}
	} while (total_written_bytes < total_to_write_bytes);
	if (total_written_bytes > total_to_write_bytes)
		TLOG(TLVL_ERROR) << GetTraceName() << ": sendFragment program error: too many bytes transferred";

	if (blocking)
	{
		blocking = false;
		fcntl(send_fd_, F_SETFL, O_NONBLOCK); // set O_NONBLOCK
	}
	sts = total_written_bytes - sizeof(MessHead);

	TLOG(14) << GetTraceName() << ": sendFragment sts=" << sts;
	return TransferInterface::CopyStatus::kSuccess;
}

void artdaq::TCPSocketTransfer::connect_()
{
	auto start_time = std::chrono::steady_clock::now();

	// Retry a few times if we can't connect
	while (send_fd_ == -1 && TimeUtils::GetElapsedTimeMicroseconds(start_time) < send_retry_timeout_us_ * 10)
	{
		TLOG(TLVL_DEBUG) << GetTraceName() << ": Connecting sender socket";
		int sndbuf_bytes = static_cast<int>(sndbuf_);
		if (sndbuf_ > INT_MAX)
		{
			sndbuf_bytes = INT_MAX;
			TLOG(TLVL_WARNING) << "Requested SNDBUF " << sndbuf_ << " too large, setting to INT_MAX: " << INT_MAX;
		}
        TLOG(TLVL_DEBUG) << "Requested SNDBUF is " << sndbuf_bytes;

		send_fd_ = TCPConnect(hostMap_[destination_rank()].c_str()
			, portMan->GetTCPSocketTransferPort(destination_rank())
			, O_NONBLOCK
			, sndbuf_bytes);
		if (send_fd_ == -1)
			usleep(send_retry_timeout_us_);
	}
	connect_state = 0;
	blocking = 0;
	TLOG(TLVL_DEBUG) << GetTraceName() << ": connect_ " + hostMap_[destination_rank()] + ":" << portMan->GetTCPSocketTransferPort(destination_rank()) << " send_fd_=" << send_fd_;
	if (send_fd_ != -1)
	{
		// write connect msg
		TLOG(TLVL_DEBUG) << GetTraceName() << ": connect_: Writing connect message";
		MessHead mh = { 0,MessHead::connect_v0,htons(source_rank()),{htonl(CONN_MAGIC)} };
		ssize_t sts = write(send_fd_, &mh, sizeof(mh));
		if (sts == -1)
		{
			TLOG(TLVL_ERROR) << GetTraceName() << ": connect_: Error writing connect message!";
			// a write error here is completely unexpected!
			connect_state = 0;
			close(send_fd_);
			send_fd_ = -1;
		}
		else
		{
			TLOG(TLVL_INFO) << GetTraceName() << ": connect_: Successfully connected";
			// consider it all connected/established
			connect_state = 1;
		}

#if USE_ACKS
		if (ack_listen_thread_ && ack_listen_thread_->joinable()) ack_listen_thread_->join();
		TLOG(TLVL_INFO) << GetTraceName() << ": Starting Ack Listener Thread";

		try {
			ack_listen_thread_ = std::make_unique<boost::thread>(&TCPSocketTransfer::receive_acks_, this);
		}
		catch (const boost::exception& e)
		{
			TLOG(TLVL_ERROR) << "Caught boost::exception starting TCP Socket Ack Listen thread: " << boost::diagnostic_information(e) << ", errno=" << errno;
			std::cerr << "Caught boost::exception starting TCP Socket Ack Listen thread: " << boost::diagnostic_information(e) << ", errno=" << errno << std::endl;
			exit(5);
		}
#endif
		}
	}

void artdaq::TCPSocketTransfer::reconnect_()
{
	if (send_fd_ == -1 && role() == TransferInterface::Role::kSend)
	{
		TLOG(TLVL_TRACE) << GetTraceName() << ": check/reconnect";
		return connect_();
	}
}

void artdaq::TCPSocketTransfer::start_listen_thread_()
{
	std::unique_lock<std::mutex> start_lock(listen_thread_mutex_);
	if (listen_thread_refcount_ == 0)
	{
		if (listen_thread_ && listen_thread_->joinable()) listen_thread_->join();
		listen_thread_refcount_ = 1;
		TLOG(TLVL_INFO) << GetTraceName() << ": Starting Listener Thread";

		try {
			listen_thread_ = std::make_unique<boost::thread>(&TCPSocketTransfer::listen_, portMan->GetTCPSocketTransferPort(destination_rank()), rcvbuf_);
		}
		catch (const boost::exception& e)
		{
			TLOG(TLVL_ERROR) << "Caught boost::exception starting TCP Socket Listen thread: " << boost::diagnostic_information(e) << ", errno=" << errno;
			std::cerr << "Caught boost::exception starting TCP Socket Listen thread: " << boost::diagnostic_information(e) << ", errno=" << errno << std::endl;
			exit(5);
		}
	}
	else
	{
		listen_thread_refcount_++;
	}
}

#if USE_ACKS
void artdaq::TCPSocketTransfer::receive_acks_()
{
	while (send_fd_ >= 0)
	{
		pollfd pollfd_s;
		pollfd_s.events = POLLIN | POLLPRI;
		pollfd_s.fd = send_fd_;

		TLOG(18) << GetTraceName() << ": receive_acks_: Polling fd to see if there's data";
		int num_fds_ready = poll(&pollfd_s, 1, 1000);
		if (num_fds_ready <= 0)
		{
			if (num_fds_ready == 0)
			{
				TLOG(18) << GetTraceName() << ": receive_acks_: No data on receive socket";
				continue;
			}

			TLOG(TLVL_ERROR) << "Error in poll: errno=" << errno;
			break;
		}

		if (pollfd_s.revents & (POLLIN | POLLPRI))
		{
			// Expected, don't have to check revents any further
		}
		else
		{
			TLOG(TLVL_DEBUG) << GetTraceName() << ": receive_acks_: Wrong event received from pollfd: " << pollfd_s.revents;
			break;
		}

		MessHead mh;
		auto sts = read(send_fd_, &mh, sizeof(mh));

		if (sts != sizeof(mh))
		{
			TLOG(TLVL_ERROR) << GetTraceName() << ": receive_ack_: Wrong message header length received! (actual " << sts << " != " << sizeof(mh) << " expected)";
			continue;
		}

		// check for "magic" and valid source_id(aka rank)
		mh.source_id = ntohs(mh.source_id); // convert here as it is reference several times
		if (mh.source_id != my_rank)
		{
			TLOG(TLVL_ERROR) << GetTraceName() << ": receive_ack_: Received ack for different sender! Rank=" << my_rank << ", hdr=" << mh.source_id;
			continue;
		}
		if (ntohl(mh.conn_magic) != ACK_MAGIC || !(mh.message_type == MessHead::ack_v0)) // Allow for future connect message versions
		{
			TLOG(TLVL_ERROR) << GetTraceName() << ": receive_ack_: Wrong magic bytes in header!";
			continue;
	}

		TLOG(17) << GetTraceName() << ": receive_acks_: Received ack message, diff is now " << (send_ack_diff_.load() - 1);
		send_ack_diff_--;
}
}

void artdaq::TCPSocketTransfer::send_ack_(int fd)
{
	MessHead mh = { 0,MessHead::ack_v0,htons(source_rank()),{ htonl(ACK_MAGIC) } };
	write(fd, &mh, sizeof(mh));
}
#endif

void artdaq::TCPSocketTransfer::listen_(int port, size_t rcvbuf)
{
	int listen_fd = -1;
	while (listen_thread_refcount_ > 0)
	{
		TLOG(TLVL_TRACE) << "listen_: Listening/accepting new connections on port " << port;
		if (listen_fd == -1)
		{
			TLOG(TLVL_DEBUG) << "listen_: Opening listener";
			listen_fd = TCP_listen_fd(port, rcvbuf);
		}
		if (listen_fd == -1)
		{
			TLOG(TLVL_DEBUG) << "listen_: Error creating listen_fd!";
			break;
		}

		int res;
		timeval tv = { 2,0 }; // maybe increase of some global "debugging" flag set???
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(listen_fd, &rfds);

		res = select(listen_fd + 1, &rfds, (fd_set *)0, (fd_set *)0, &tv);
		if (res > 0)
		{
			int sts;
			sockaddr_un un;
			socklen_t arglen = sizeof(un);
			int fd;
			TLOG(TLVL_DEBUG) << "listen_: Calling accept";
			fd = accept(listen_fd, (sockaddr *)&un, &arglen);
			TLOG(TLVL_DEBUG) << "listen_: Done with accept";

			TLOG(TLVL_DEBUG) << "listen_: Reading connect message";
			socklen_t lenlen = sizeof(tv);
			/*sts=*/
			setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, lenlen); // see man 7 socket.
			MessHead mh;
			uint64_t mark_us = TimeUtils::gettimeofday_us();
			sts = read(fd, &mh, sizeof(mh));
			uint64_t delta_us = TimeUtils::gettimeofday_us() - mark_us;
			TLOG(TLVL_DEBUG) << "listen_: Read of connect message took " << delta_us << " microseconds.";
			if (sts != sizeof(mh))
			{
				TLOG(TLVL_DEBUG) << "listen_: Wrong message header length received!";
				close(fd);
				continue;
			}

			// check for "magic" and valid source_id(aka rank)
			mh.source_id = ntohs(mh.source_id); // convert here as it is reference several times
			if (ntohl(mh.conn_magic) != CONN_MAGIC || !(mh.message_type == MessHead::connect_v0)) // Allow for future connect message versions
			{
				TLOG(TLVL_DEBUG) << "listen_: Wrong magic bytes in header!";
				close(fd);
				continue;
			}

			// now add (new) connection
			std::unique_lock<std::mutex> lk(connected_fd_mutex_);
			connected_fds_[mh.source_id].insert(fd);

			TLOG(TLVL_INFO) << "listen_: New fd is " << fd << " for source rank " << mh.source_id;
		}
		else
		{
			TLOG(16) << "listen_: No connections in timeout interval!";
		}
	}

	TLOG(TLVL_INFO) << "listen_: Shutting down connection listener";
	if (listen_fd != -1) close(listen_fd);
	std::unique_lock<std::mutex> lk(connected_fd_mutex_);
	auto it = connected_fds_.begin();
	while (it != connected_fds_.end())
	{
		auto& fd_set = it->second;
		auto rank_it = fd_set.begin();
		while (rank_it != fd_set.end())
		{
			close(*rank_it);
			rank_it = fd_set.erase(rank_it);
		}
		it = connected_fds_.erase(it);
	}

} // do_connect_

DEFINE_ARTDAQ_TRANSFER(artdaq::TCPSocketTransfer)
