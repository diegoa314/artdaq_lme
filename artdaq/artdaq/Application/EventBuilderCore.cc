#include "canvas/Utilities/Exception.h"
#include "art/Framework/Art/artapp.h"

#define TRACE_NAME (app_name + "_EventBuilderCore").c_str() // include these 2 first -
#include "artdaq/DAQdata/Globals.hh"
#include "artdaq-core/Core/SimpleMemoryReader.hh"
#include "artdaq-core/Utilities/ExceptionHandler.hh"

#include "artdaq/Application/EventBuilderCore.hh"
#include "artdaq/TransferPlugins/TransferInterface.hh"

#include <iomanip>

artdaq::EventBuilderCore::EventBuilderCore()
	: DataReceiverCore()
{
}

artdaq::EventBuilderCore::~EventBuilderCore()
{
	TLOG(TLVL_DEBUG) << "Destructor" ;
}

bool artdaq::EventBuilderCore::initialize(fhicl::ParameterSet const& pset)
{
	TLOG(TLVL_DEBUG) << "initialize method called with DAQ "
		<< "ParameterSet = \"" << pset.to_string() << "\"." ;

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
	fhicl::ParameterSet evb_pset;
	try
	{
		evb_pset = daq_pset.get<fhicl::ParameterSet>("event_builder");

		// We are going to change the default from true to false for the "send_init_fragments" parameter
		// EventBuilders almost always do NOT receive init Fragments from upstream to send to art
		if (!evb_pset.has_key("send_init_fragments"))
		{
			evb_pset.put<bool>("send_init_fragments", false);
		}
	}
	catch (...)
	{
		TLOG(TLVL_ERROR)
			<< "Unable to find the event_builder parameters in the DAQ "
			<< "initialization ParameterSet: \"" + daq_pset.to_string() + "\"." ;
		return false;
	}

	fhicl::ParameterSet metric_pset;
	try
	{
		metric_pset = daq_pset.get<fhicl::ParameterSet>("metrics");
	}
	catch (...) {} // OK if there's no metrics table defined in the FHiCL

	return initializeDataReceiver(pset,evb_pset, metric_pset);
}
