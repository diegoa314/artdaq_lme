#define TRACE_NAME "NetMonTransportService"

#include "artdaq/DAQdata/Globals.hh"
#include "artdaq/ArtModules/NetMonTransportService.h"
#include "artdaq/DAQrate/DataSenderManager.hh"
#include "artdaq-core/Core/SharedMemoryEventReceiver.hh"

#include "artdaq-core/Data/Fragment.hh"
#include "artdaq/DAQdata/NetMonHeader.hh"
#include "artdaq-core/Data/RawEvent.hh"
#include "artdaq-core/Utilities/TimeUtils.hh"

#include "art/Framework/Services/Registry/ActivityRegistry.h"
#include "canvas/Utilities/Exception.h"
#include "cetlib/container_algorithms.h"
#include "cetlib_except/exception.h"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/ParameterSetRegistry.h"

#include <TClass.h>
#include <TBufferFile.h>

#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>


#define DUMP_SEND_MESSAGE 0
#define DUMP_RECEIVE_MESSAGE 0

static fhicl::ParameterSet empty_pset;


NetMonTransportService::
NetMonTransportService(fhicl::ParameterSet const& pset, art::ActivityRegistry&)
	: NetMonTransportServiceInterface()
	, data_pset_(pset)
	, init_received_(false)
	, sender_ptr_(nullptr)
	, incoming_events_(nullptr)
	, recvd_fragments_(nullptr)
{
	TLOG(TLVL_TRACE) << "NetMonTransportService CONSTRUCTOR" ;
	if (pset.has_key("rank")) my_rank = pset.get<int>("rank");

	init_timeout_s_ = pset.get<double>("init_fragment_timeout_seconds", 1.0);
}

NetMonTransportService::
~NetMonTransportService()
{
	NetMonTransportService::disconnect();
}

void
NetMonTransportService::
connect()
{
	sender_ptr_.reset(new artdaq::DataSenderManager(data_pset_));
}

void
NetMonTransportService::
listen()
{
	if (!incoming_events_)
	{
		incoming_events_.reset(new artdaq::SharedMemoryEventReceiver(data_pset_.get<int>("shared_memory_key", 0xBEE70000 + getppid()), data_pset_.get<int>("broadcast_shared_memory_key", 0xCEE70000 + getppid())));
		if (data_pset_.has_key("rank")) my_rank = data_pset_.get<int>("rank");
		else my_rank = incoming_events_->GetRank();
	}
	return;
}

void
NetMonTransportService::
disconnect()
{
	if (sender_ptr_) sender_ptr_.reset(nullptr);
}

void
NetMonTransportService::
sendMessage(uint64_t sequenceId, uint8_t messageType, TBufferFile& msg)
{
	if (sender_ptr_ == nullptr)
	{
		TLOG(TLVL_DEBUG) << "Reconnecting DataSenderManager" ;
		connect();
	}

#if DUMP_SEND_MESSAGE
	std::string fileName = "sendMessage_" + std::to_string(my_rank) + "_" + std::to_string(getpid()) + "_" + std::to_string(sequenceId) + ".bin";
	std::fstream ostream(fileName, std::ios::out | std::ios::binary);
	ostream.write(msg.Buffer(), msg.Length());
	ostream.close();
#endif

	TLOG(TLVL_DEBUG) << "Sending message with sequenceID=" << sequenceId << ", type=" << (int)messageType << ", length=" << msg.Length() ;
	artdaq::NetMonHeader header;
	header.data_length = static_cast<uint64_t>(msg.Length());
	artdaq::Fragment
		fragment(std::ceil(msg.Length() /
			static_cast<double>(sizeof(artdaq::RawDataType))),
			sequenceId, 0, messageType, header);

	memcpy(&*fragment.dataBegin(), msg.Buffer(), msg.Length());
	sender_ptr_->sendFragment(std::move(fragment));
}

void
NetMonTransportService::
receiveMessage(TBufferFile*& msg)
{
	listen();
	TLOG(TLVL_TRACE) << "receiveMessage BEGIN" ;
	while (recvd_fragments_ == nullptr)
	{
		TLOG(TLVL_TRACE) << "receiveMessage: Waiting for available buffer" ;
		bool keep_looping = true;
		bool got_event = false;
		while (keep_looping)
		{
			keep_looping = false;
			got_event = incoming_events_->ReadyForRead();
			if (!got_event)
			{
				keep_looping = true;
			}
		}

		TLOG(TLVL_TRACE) << "receiveMessage: Reading buffer header" ;
		auto errflag = false;
		incoming_events_->ReadHeader(errflag);
		if (errflag) { // Buffer was changed out from under reader!
			msg = nullptr;
			return;
		}
		TLOG(TLVL_TRACE) << "receiveMessage: Getting Fragment types" ;
		auto fragmentTypes = incoming_events_->GetFragmentTypes(errflag);
		if (errflag) { // Buffer was changed out from under reader!
			incoming_events_->ReleaseBuffer();
			msg = nullptr;
			return;
		}
		if (fragmentTypes.size() == 0)
		{
			TLOG(TLVL_ERROR) << "Event has no Fragments! Aborting!" ;
			incoming_events_->ReleaseBuffer();
			msg = nullptr;
			return;
		}
		TLOG(TLVL_TRACE) << "receiveMessage: Checking first Fragment type" ;
		auto firstFragmentType = *fragmentTypes.begin();

		// We return false, indicating we're done reading, if:
		//   1) we did not obtain an event, because we timed out and were
		//      configured NOT to keep trying after a timeout, or
		//   2) the event we read was the end-of-data marker: a null
		//      pointer
		if (!got_event || firstFragmentType == artdaq::Fragment::EndOfDataFragmentType)
		{
			TLOG(TLVL_DEBUG) << "Received shutdown message, returning from receiveMessage "
					 << "(debug: got_event=" << got_event << ",fragType=" << (int)firstFragmentType
					 << ",EODFragType=" << (int)artdaq::Fragment::EndOfDataFragmentType << ")";
			incoming_events_->ReleaseBuffer();
			msg = nullptr;
			return;
		}
		if (firstFragmentType == artdaq::Fragment::InitFragmentType)
		{
			TLOG(TLVL_DEBUG) << "Cannot receive InitFragments here, retrying" ;
			incoming_events_->ReleaseBuffer();
			continue;
		}
		// EndOfRun and EndOfSubrun Fragments are ignored in NetMonTransportService
		else if (firstFragmentType == artdaq::Fragment::EndOfRunFragmentType || firstFragmentType == artdaq::Fragment::EndOfSubrunFragmentType)
		{
			TLOG(TLVL_DEBUG) << "Ignoring EndOfRun or EndOfSubrun Fragment" ;
			incoming_events_->ReleaseBuffer();
			continue;
		}

		TLOG(TLVL_TRACE) << "receiveMessage: Getting all Fragments" ;
		recvd_fragments_ = incoming_events_->GetFragmentsByType(errflag, artdaq::Fragment::InvalidFragmentType);
		if (!recvd_fragments_)
		{
			TLOG(TLVL_ERROR) << "Error retrieving Fragments from shared memory! Aborting!";
			incoming_events_->ReleaseBuffer();
			msg = nullptr;
			return;

		}
		/* Events coming out of the EventStore are not sorted but need to be
		   sorted by sequence ID before they can be passed to art.
		*/
		std::sort(recvd_fragments_->begin(), recvd_fragments_->end(),
			artdaq::fragmentSequenceIDCompare);

		TLOG(TLVL_TRACE) << "receiveMessage: Releasing buffer" ;
		incoming_events_->ReleaseBuffer();
	}

	// Do not process data until Init Fragment received!
	auto start = std::chrono::steady_clock::now();
	while (!init_received_ && artdaq::TimeUtils::GetElapsedTime(start) < init_timeout_s_)
	{
		usleep(init_timeout_s_ * 1000000 / 100); // Check 100 times
	}
	if (!init_received_) {
		TLOG(TLVL_ERROR) << "Received data but no Init Fragment after " << init_timeout_s_ << " seconds. Art will crash." ;
	}

	TLOG(TLVL_TRACE) << "receiveMessage: Returning top Fragment" ;
	artdaq::Fragment topFrag = std::move(recvd_fragments_->at(0));
	recvd_fragments_->erase(recvd_fragments_->begin());
	if (recvd_fragments_->size() == 0)
	{
		recvd_fragments_.reset(nullptr);
	}

	TLOG(TLVL_TRACE) << "receiveMessage: Copying Fragment into TBufferFile, length=" << topFrag.metadata<artdaq::NetMonHeader>()->data_length ;
	auto header = topFrag.metadata<artdaq::NetMonHeader>();
	auto buffer = static_cast<char *>(malloc(header->data_length));
	memcpy(buffer, &*topFrag.dataBegin(), header->data_length);
	msg = new TBufferFile(TBuffer::kRead, header->data_length, buffer, kTRUE, 0);

#if DUMP_RECEIVE_MESSAGE
	std::string fileName = "receiveMessage_" + std::to_string(my_rank) + "_" + std::to_string(getpid()) + "_" + std::to_string(topFrag.sequenceID()) + ".bin";
	std::fstream ostream(fileName.c_str(), std::ios::out | std::ios::binary);
	ostream.write(buffer, header->data_length);
	ostream.close();
#endif

	TLOG(TLVL_TRACE) << "receiveMessage END" ;
}

void
NetMonTransportService::
receiveInitMessage(TBufferFile*& msg)
{
	listen();
	TLOG(TLVL_TRACE) << "receiveInitMessage BEGIN" ;
	if (recvd_fragments_ == nullptr)
	{
		TLOG(TLVL_TRACE) << "receiveInitMessage: Waiting for available buffer" ;

		bool got_init = false;
		auto errflag = false;
		while (!got_init) {

			bool got_event = false;
			while (!got_event)
			{
				got_event = incoming_events_->ReadyForRead(true);
			}

			TLOG(TLVL_TRACE) << "receiveInitMessage: Reading buffer header" ;
			incoming_events_->ReadHeader(errflag);
			if (errflag) { // Buffer was changed out from under reader!
				TLOG(TLVL_ERROR) << "receiveInitMessage: Error receiving message!" ;
				incoming_events_->ReleaseBuffer();
				msg = nullptr;
				return;
			}
			TLOG(TLVL_TRACE) << "receiveInitMessage: Getting Fragment types" ;
			auto fragmentTypes = incoming_events_->GetFragmentTypes(errflag);
			if (errflag) { // Buffer was changed out from under reader!
				incoming_events_->ReleaseBuffer();
				msg = nullptr;
				TLOG(TLVL_ERROR) << "receiveInitMessage: Error receiving message!" ;
				return;
			}
			if (fragmentTypes.size() == 0)
			{
				TLOG(TLVL_ERROR) << "Event has no Fragments! Aborting!" ;
				incoming_events_->ReleaseBuffer();
				msg = nullptr;
				return;
			}
			TLOG(TLVL_TRACE) << "receiveInitMessage: Checking first Fragment type" ;
			auto firstFragmentType = *fragmentTypes.begin();

			// We return false, indicating we're done reading, if:
			//   1) we did not obtain an event, because we timed out and were
			//      configured NOT to keep trying after a timeout, or
			//   2) the event we read was the end-of-data marker: a null
			//      pointer
			if (!got_event || firstFragmentType == artdaq::Fragment::EndOfDataFragmentType)
			{
				TLOG(TLVL_DEBUG) << "Received shutdown message, returning" ;
				incoming_events_->ReleaseBuffer();
				msg = nullptr;
				return;
			}
			if (firstFragmentType != artdaq::Fragment::InitFragmentType)
			{
				TLOG(TLVL_WARNING) << "Did NOT receive Init Fragment as first broadcast! Type=" << artdaq::detail::RawFragmentHeader::SystemTypeToString(firstFragmentType) ;
				incoming_events_->ReleaseBuffer();
			}
			got_init = true;
		}
		TLOG(TLVL_TRACE) << "receiveInitMessage: Getting all Fragments" ;
		recvd_fragments_ = incoming_events_->GetFragmentsByType(errflag, artdaq::Fragment::InvalidFragmentType);
		/* Events coming out of the EventStore are not sorted but need to be
		sorted by sequence ID before they can be passed to art.
		*/
		std::sort(recvd_fragments_->begin(), recvd_fragments_->end(),
			artdaq::fragmentSequenceIDCompare);

		incoming_events_->ReleaseBuffer();
	}

	TLOG(TLVL_TRACE) << "receiveInitMessage: Returning top Fragment" ;
	artdaq::Fragment topFrag = std::move(recvd_fragments_->at(0));
	recvd_fragments_->erase(recvd_fragments_->begin());
	if (recvd_fragments_->size() == 0)
	{
		recvd_fragments_.reset(nullptr);
	}

	auto header = topFrag.metadata<artdaq::NetMonHeader>();
	TLOG(TLVL_TRACE) << "receiveInitMessage: Copying Fragment into TBufferFile: message length: " << header->data_length ;
	auto buffer = new char[header->data_length];
	//auto buffer = static_cast<char *>(malloc(header->data_length)); // Fix alloc-dealloc-mismatch
	memcpy(buffer, &*topFrag.dataBegin(), header->data_length);

#if DUMP_RECEIVE_MESSAGE
	std::string fileName = "receiveInitMessage_" + std::to_string(getpid()) + ".bin";
	std::fstream ostream(fileName.c_str(), std::ios::out | std::ios::binary);
	ostream.write(buffer, header->data_length);
	ostream.close();
#endif

	msg = new TBufferFile(TBuffer::kRead, header->data_length, buffer, kTRUE, 0);

	TLOG(TLVL_TRACE) << "receiveInitMessage END" ;
	init_received_ = true;
}
DEFINE_ART_SERVICE_INTERFACE_IMPL(NetMonTransportService, NetMonTransportServiceInterface)
