#define TRACE_NAME (app_name + "_DataLoggerApp").c_str() // include these 2 first -
#include "artdaq/DAQdata/Globals.hh"

#include "artdaq/Application/DataLoggerApp.hh"
#include "artdaq/Application/DataLoggerCore.hh"
#include "artdaq-core/Utilities/ExceptionHandler.hh"

#include <iostream>

artdaq::DataLoggerApp::DataLoggerApp()
{
}

// *******************************************************************
// *** The following methods implement the state machine operations.
// *******************************************************************

bool artdaq::DataLoggerApp::do_initialize(fhicl::ParameterSet const& pset, uint64_t, uint64_t)
{
	report_string_ = "";

	//DataLogger_ptr_.reset(nullptr);
	if (DataLogger_ptr_.get() == 0)
	{
		DataLogger_ptr_.reset(new DataLoggerCore());
	}
	external_request_status_ = DataLogger_ptr_->initialize(pset);
	if (!external_request_status_)
	{
		report_string_ = "Error initializing ";
		report_string_.append(app_name + " ");
		report_string_.append("with ParameterSet = \"" + pset.to_string() + "\".");
	}

	return external_request_status_;
}

bool artdaq::DataLoggerApp::do_start(art::RunID id, uint64_t, uint64_t)
{
	report_string_ = "";
	external_request_status_ = DataLogger_ptr_->start(id);
	if (!external_request_status_)
	{
		report_string_ = "Error starting ";
		report_string_.append(app_name + " ");
		report_string_.append("for run number ");
		report_string_.append(boost::lexical_cast<std::string>(id.run()));
		report_string_.append(".");
	}
	
	return external_request_status_;
}

bool artdaq::DataLoggerApp::do_stop(uint64_t, uint64_t)
{
	report_string_ = "";
	external_request_status_ = DataLogger_ptr_->stop();
	if (!external_request_status_)
	{
		report_string_ = "Error stopping ";
		report_string_.append(app_name + ".");
	}

	return external_request_status_;
}

bool artdaq::DataLoggerApp::do_pause(uint64_t, uint64_t)
{
	report_string_ = "";
	external_request_status_ = DataLogger_ptr_->pause();
	if (!external_request_status_)
	{
		report_string_ = "Error pausing ";
		report_string_.append(app_name + ".");
	}

	return external_request_status_;
}

bool artdaq::DataLoggerApp::do_resume(uint64_t, uint64_t)
{
	report_string_ = "";
	external_request_status_ = DataLogger_ptr_->resume();
	if (!external_request_status_)
	{
		report_string_ = "Error resuming ";
		report_string_.append(app_name + ".");
	}

	return external_request_status_;
}

bool artdaq::DataLoggerApp::do_shutdown(uint64_t)
{
	report_string_ = "";
	external_request_status_ = DataLogger_ptr_->shutdown();
	if (!external_request_status_)
	{
		report_string_ = "Error shutting down ";
		report_string_.append(app_name + ".");
	}

	return external_request_status_;
}

bool artdaq::DataLoggerApp::do_soft_initialize(fhicl::ParameterSet const&, uint64_t, uint64_t)
{
	return true;
}

bool artdaq::DataLoggerApp::do_reinitialize(fhicl::ParameterSet const&, uint64_t, uint64_t)
{
	return true;
}

std::string artdaq::DataLoggerApp::report(std::string const& which) const
{
	std::string resultString;

	// if all that is requested is the latest state change result, return it
	if (which == "transition_status")
	{
		if (report_string_.length() > 0) { return report_string_; }
		else { return "Success"; }
	}

	//// if there is an outstanding report/message at the Commandable/Application
	//// level, prepend that
	//if (report_string_.length() > 0) {
	//  resultString.append("*** Overall status message:\r\n");
	//  resultString.append(report_string_ + "\r\n");
	//  resultString.append("*** Requested report response:\r\n");
	//}

	// pass the request to the DataLoggerCore instance, if it's available
	if (DataLogger_ptr_.get() != 0)
	{
		resultString.append(DataLogger_ptr_->report(which));
	}
	else
	{
		resultString.append("This DataLogger has not yet been initialized and ");
		resultString.append("therefore can not provide reporting.");
	}

	return resultString;
}

bool artdaq::DataLoggerApp::do_add_config_archive_entry(std::string const& key, std::string const& value)
{
	report_string_ = "";
	external_request_status_ = DataLogger_ptr_->add_config_archive_entry(key, value);
	if (!external_request_status_)
	{
		report_string_ = "Error adding config entry with key ";
		report_string_.append(key + " and value \"");
		report_string_.append(value + "\" in");
		report_string_.append(app_name + ".");
	}

	return external_request_status_;
}

bool artdaq::DataLoggerApp::do_clear_config_archive()
{
	report_string_ = "";
	external_request_status_ = DataLogger_ptr_->clear_config_archive();
	if (!external_request_status_)
	{
		report_string_ = "Error clearing the configuration archive in ";
		report_string_.append(app_name + ".");
	}

	return external_request_status_;
}
