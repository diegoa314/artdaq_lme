#define TRACE_NAME "requestReceiver"

#include <boost/program_options.hpp>
#include "fhiclcpp/make_ParameterSet.h"
namespace bpo = boost::program_options;

#include "artdaq/DAQrate/RequestReceiver.hh"
#include "artdaq-core/Utilities/configureMessageFacility.hh"
#include "artdaq/Application/LoadParameterSet.hh"

int main(int argc, char* argv[])
{
	artdaq::configureMessageFacility("requestReceiver");

	auto pset = LoadParameterSet<artdaq::RequestReceiver::Config>(argc, argv, "receiver", "This is a simple application which listens for Data Request messages and prints their contents");

	int rc = 0;

	fhicl::ParameterSet tempPset;
	if (pset.has_key("request_receiver"))
	{
		tempPset = pset.get<fhicl::ParameterSet>("request_receiver");
	}
	else
	{
		tempPset = pset;
	}

	artdaq::RequestReceiver recvr(tempPset);
	recvr.startRequestReceiverThread();

	while (true)
	{
		for (auto req : recvr.GetAndClearRequests())
		{
			TLOG(TLVL_INFO) << "Received Request for Sequence ID " << req.first << ", timestamp " << req.second ;
		}
		usleep(10000);
	}

	return rc;
}
