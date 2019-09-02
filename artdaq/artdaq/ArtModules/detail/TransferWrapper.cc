#define TRACE_NAME "TransferWrapper"

#include "artdaq/ArtModules/detail/TransferWrapper.hh"
#include "artdaq/TransferPlugins/MakeTransferPlugin.hh"
#include "artdaq/ExternalComms/MakeCommanderPlugin.hh"
#include "artdaq/DAQdata/NetMonHeader.hh"
#include "artdaq/DAQdata/Globals.hh"
#include "artdaq-core/Utilities/ExceptionHandler.hh"
#include "artdaq-core/Data/Fragment.hh"

#include "cetlib/BasicPluginFactory.h"
#include "cetlib_except/exception.h"
#include "fhiclcpp/ParameterSet.h"

#include <TBufferFile.h>

#include <limits>
#include <iostream>
#include <string>
#include <sstream>
#include <csignal>

namespace
{
	volatile std::sig_atomic_t gSignalStatus = 0; ///< Stores singal from signal handler
}

/**
 * \brief Handle a Unix signal
 * \param signal Signal to handle
 */
void signal_handler(int signal)
{
	gSignalStatus = signal;
}

artdaq::TransferWrapper::TransferWrapper(const fhicl::ParameterSet& pset) :
	timeoutInUsecs_(pset.get<std::size_t>("timeoutInUsecs", 100000))
	, dispatcherHost_(pset.get<std::string>("dispatcherHost", "localhost"))
	, dispatcherPort_(pset.get<std::string>("dispatcherPort", "5266"))
	, serverUrl_(pset.get<std::string>("server_url", "http://" + dispatcherHost_ + ":" + dispatcherPort_ + "/RPC2"))
	, maxEventsBeforeInit_(pset.get<std::size_t>("maxEventsBeforeInit", 5))
	, allowedFragmentTypes_(pset.get<std::vector<int>>("allowedFragmentTypes", { 226, 227, 229 }))
	, quitOnFragmentIntegrityProblem_(pset.get<bool>("quitOnFragmentIntegrityProblem", true))
	, monitorRegistered_(false)
{
	std::signal(SIGINT, signal_handler);

	try
	{
		transfer_ = MakeTransferPlugin(pset, "transfer_plugin", TransferInterface::Role::kReceive);
	}
	catch (...)
	{
		ExceptionHandler(ExceptionHandlerRethrow::yes,
						 "TransferWrapper: failure in call to MakeTransferPlugin");
	}

	try {
		if (metricMan) {
			metricMan->initialize(pset.get<fhicl::ParameterSet>("metrics", fhicl::ParameterSet()), "Online Monitor");
			metricMan->do_start();
		}
	}
	catch (...)
	{
		ExceptionHandler(ExceptionHandlerRethrow::no,			"TransferWrapper: could not configure metrics");
	}


	fhicl::ParameterSet new_pset(pset);
	if (!new_pset.has_key("server_url")) {
		new_pset.put<std::string>("server_url", serverUrl_);
	}

	auto dispatcherConfig = pset.get<fhicl::ParameterSet>("dispatcher_config");
	artdaq::Commandable c;
	commander_ = MakeCommanderPlugin(new_pset, c);

	int retry = 3;

	while (retry > 0) {
		TLOG(TLVL_INFO) << "Attempting to register this monitor (\"" << transfer_->uniqueLabel()
			<< "\") with the dispatcher aggregator" ;

		auto status = commander_->send_register_monitor(dispatcherConfig.to_string());

		TLOG(TLVL_INFO) << "Response from dispatcher is \"" << status << "\"" ;

		if (status == "Success")
		{
			monitorRegistered_ = true;
			break;
		}
		else
		{
			TLOG(TLVL_WARNING) << "Error in TransferWrapper: attempt to register with dispatcher did not result in the \"Success\" response" ;
			usleep(100000);
		}
		retry--;
	}
}

void artdaq::TransferWrapper::receiveMessage(std::unique_ptr<TBufferFile>& msg)
{
	std::unique_ptr<artdaq::Fragment> fragmentPtr;
	bool receivedFragment = false;
	static bool initialized = false;
	static size_t fragments_received = 0;

	while (true && !gSignalStatus)
	{
		fragmentPtr = std::make_unique<artdaq::Fragment>();

		while (!receivedFragment)
		{
			if (gSignalStatus)
			{
				TLOG(TLVL_INFO) << "Ctrl-C appears to have been hit" ;
				unregisterMonitor();
				return;
			}

			try
			{
				auto result = transfer_->receiveFragment(*fragmentPtr, timeoutInUsecs_);

				if (result >= artdaq::TransferInterface::RECV_SUCCESS)
				{
					receivedFragment = true;
					fragments_received++;

					static size_t cntr = 1;
					auto mod = ++cntr % 10;
					auto suffix = "-th";
					if (mod == 1) suffix = "-st";
					if (mod == 2) suffix = "-nd";
					if (mod == 3) suffix = "-rd";
					TLOG(TLVL_INFO) << "Received " << cntr << suffix << " event, "
						<< "seqID == " << fragmentPtr->sequenceID()
						<< ", type == " << fragmentPtr->typeString() ;
					continue;
				}
				else if (result == artdaq::TransferInterface::DATA_END)
				{
					TLOG(TLVL_ERROR) << "Transfer Plugin disconnected or other unrecoverable error. Shutting down.";
					unregisterMonitor();
					return;
				}
				else
				{
					// 02-Jun-2018, KAB: added status/result printout
					// to-do: add another else clause that explicitly checks for RECV_TIMEOUT
					TLOG(TLVL_WARNING) << "Timeout occurred in call to transfer_->receiveFragmentFrom; will try again"
							   << ", status = " << result;

				}
			}
			catch (...)
			{
				ExceptionHandler(ExceptionHandlerRethrow::yes,
								 "Problem receiving data in TransferWrapper::receiveMessage");
			}
		}

		if (fragmentPtr->type() == artdaq::Fragment::EndOfDataFragmentType)
		{
			//if (monitorRegistered_)
			//{
			//	unregisterMonitor();
			//}
			return;
		}

		try
		{
			extractTBufferFile(*fragmentPtr, msg);
		}
		catch (...)
		{
			ExceptionHandler(ExceptionHandlerRethrow::yes,
							 "Problem extracting TBufferFile from artdaq::Fragment in TransferWrapper::receiveMessage");
		}

		checkIntegrity(*fragmentPtr);

		if (initialized || fragmentPtr->type() == artdaq::Fragment::InitFragmentType)
		{
			initialized = true;
			break;
		}
		else
		{
			receivedFragment = false;

			if (fragments_received > maxEventsBeforeInit_)
			{
				throw cet::exception("TransferWrapper") << "First " << maxEventsBeforeInit_ <<
					" events received did not include the \"Init\" event containing necessary info for art; exiting...";
			}
		}
	}
}


void
artdaq::TransferWrapper::extractTBufferFile(const artdaq::Fragment& fragment,
											std::unique_ptr<TBufferFile>& tbuffer)
{
	const artdaq::NetMonHeader* header = fragment.metadata<artdaq::NetMonHeader>();
	char* buffer = (char *)malloc(header->data_length);
	memcpy(buffer, fragment.dataBeginBytes(), header->data_length);

	// TBufferFile takes ownership of the contents of memory passed to it
	tbuffer.reset(new TBufferFile(TBuffer::kRead, header->data_length, buffer, kTRUE, 0));
}

void
artdaq::TransferWrapper::checkIntegrity(const artdaq::Fragment& fragment) const
{
	const size_t artdaqheader = artdaq::detail::RawFragmentHeader::num_words() *
		sizeof(artdaq::detail::RawFragmentHeader::RawDataType);
	const size_t payload = static_cast<size_t>(fragment.dataEndBytes() - fragment.dataBeginBytes());
	const size_t metadata = sizeof(artdaq::NetMonHeader);
	const size_t totalsize = fragment.sizeBytes();

	const size_t type = static_cast<size_t>(fragment.type());

	if (totalsize != artdaqheader + metadata + payload)
	{
		std::stringstream errmsg;
		errmsg << "Error: artdaq fragment of type " <<
			fragment.typeString() << ", sequence ID " <<
			fragment.sequenceID() <<
			" has internally inconsistent measures of its size, signalling data corruption: in bytes," <<
			" total size = " << totalsize << ", artdaq fragment header = " << artdaqheader <<
			", metadata = " << metadata << ", payload = " << payload;

		TLOG(TLVL_ERROR) << errmsg.str() ;

		if (quitOnFragmentIntegrityProblem_)
		{
			throw cet::exception("TransferWrapper") << errmsg.str();
		}
		else
		{
			return;
		}
	}

	auto findloc = std::find(allowedFragmentTypes_.begin(), allowedFragmentTypes_.end(), static_cast<int>(type));

	if (findloc == allowedFragmentTypes_.end())
	{
		std::stringstream errmsg;
		errmsg << "Error: artdaq fragment appears to have type "
			<< type << ", not found in the allowed fragment types list";

		TLOG(TLVL_ERROR) << errmsg.str() ;
		if (quitOnFragmentIntegrityProblem_)
		{
			throw cet::exception("TransferWrapper") << errmsg.str();
		}
		else
		{
			return;
		}
	}
}

void
artdaq::TransferWrapper::unregisterMonitor()
{
	if (!monitorRegistered_)
	{
		throw cet::exception("TransferWrapper") <<
			"The function to unregister the monitor was called, but the monitor doesn't appear to be registered";
	}

	int retry = 3;
	while (retry > 0) {

		TLOG(TLVL_INFO) << "Requesting that this monitor (" << transfer_->uniqueLabel()
			<< ") be unregistered from the dispatcher aggregator";

		auto status = commander_->send_unregister_monitor(transfer_->uniqueLabel());


		TLOG(TLVL_INFO) << "Response from dispatcher is \""
			<< status << "\"";

		if (status == "Success")
		{
			monitorRegistered_ = false;
			break;
		}
		else if (status == "busy")
		{ }
		else
		{
			throw cet::exception("TransferWrapper") << "Error in TransferWrapper: attempt to unregister with dispatcher did not result in the \"Success\" response";
		}
		retry--;
	}
}


artdaq::TransferWrapper::~TransferWrapper()
{
	if (monitorRegistered_)
	{
		try
		{
			unregisterMonitor();
		}
		catch (...)
		{
			ExceptionHandler(ExceptionHandlerRethrow::no,
							 "An exception occurred when trying to unregister monitor during TransferWrapper's destruction");
		}
	}
	artdaq::Globals::CleanUpGlobals();
}
