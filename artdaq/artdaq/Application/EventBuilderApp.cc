#define TRACE_NAME (app_name + "_DataLoggerApp").c_str() // include these 2 first -
#include "artdaq/DAQdata/Globals.hh"

#include "artdaq/Application/EventBuilderApp.hh"

artdaq::EventBuilderApp::EventBuilderApp() 
{
}

// *******************************************************************
// *** The following methods implement the state machine operations.
// *******************************************************************

bool artdaq::EventBuilderApp::do_initialize(fhicl::ParameterSet const& pset, uint64_t, uint64_t)
{
	report_string_ = "";
	external_request_status_ = true;

	// in the following block, we first destroy the existing EventBuilder
	// instance, then create a new one.  Doing it in one step does not
	// produce the desired result since that creates a new instance and
	// then deletes the old one, and we need the opposite order.
	//event_builder_ptr_.reset(nullptr);
	if (event_builder_ptr_.get() == 0)
	{
		event_builder_ptr_.reset(new EventBuilderCore());
		external_request_status_ = event_builder_ptr_->initialize(pset);
	}
	if (! external_request_status_)
	{
		report_string_ = "Error initializing an EventBuilderCore named";
		report_string_.append(app_name + " with ");
		report_string_.append("ParameterSet = \"" + pset.to_string() + "\".");
	}

	return external_request_status_;
}

bool artdaq::EventBuilderApp::do_start(art::RunID id, uint64_t, uint64_t)
{
	report_string_ = "";
	external_request_status_ = event_builder_ptr_->start(id);
	if (! external_request_status_)
	{
		report_string_ = "Error starting ";
		report_string_.append(app_name + " for run ");
		report_string_.append("number ");
		report_string_.append(boost::lexical_cast<std::string>(id.run()));
		report_string_.append(".");
	}

	return external_request_status_;
}

bool artdaq::EventBuilderApp::do_stop(uint64_t, uint64_t)
{
	report_string_ = "";
	external_request_status_ = event_builder_ptr_->stop();
	if (! external_request_status_)
	{
		report_string_ = "Error stopping ";
		report_string_.append(app_name + ".");
	}
	return external_request_status_;
}

bool artdaq::EventBuilderApp::do_pause(uint64_t, uint64_t)
{
	report_string_ = "";
	external_request_status_ = event_builder_ptr_->pause();
	if (! external_request_status_)
	{
		report_string_ = "Error pausing ";
		report_string_.append(app_name + ".");
	}

	return external_request_status_;
}

bool artdaq::EventBuilderApp::do_resume(uint64_t, uint64_t)
{
	report_string_ = "";
	external_request_status_ = event_builder_ptr_->resume();
	if (! external_request_status_)
	{
		report_string_ = "Error resuming ";
		report_string_.append(app_name + ".");
	}
	
	return external_request_status_;
}

bool artdaq::EventBuilderApp::do_shutdown(uint64_t)
{
	report_string_ = "";
	external_request_status_ = event_builder_ptr_->shutdown();
	if (! external_request_status_)
	{
		report_string_ = "Error shutting down ";
		report_string_.append(app_name + ".");
	}
	return external_request_status_;
}

bool artdaq::EventBuilderApp::do_soft_initialize(fhicl::ParameterSet const& pset, uint64_t, uint64_t)
{
	report_string_ = "";
	external_request_status_ = event_builder_ptr_->soft_initialize(pset);
	if (! external_request_status_)
	{
		report_string_ = "Error soft-initializing ";
		report_string_.append(app_name + " with ");
		report_string_.append("ParameterSet = \"" + pset.to_string() + "\".");
	}
	return external_request_status_;
}

bool artdaq::EventBuilderApp::do_reinitialize(fhicl::ParameterSet const& pset, uint64_t, uint64_t)
{
	report_string_ = "";
	external_request_status_ = event_builder_ptr_->reinitialize(pset);
	if (! external_request_status_)
	{
		report_string_ = "Error reinitializing ";
		report_string_.append(app_name + " with ");
		report_string_.append("ParameterSet = \"" + pset.to_string() + "\".");
	}
	return external_request_status_;
}

bool artdaq::EventBuilderApp::do_rollover_subrun(uint64_t boundary)
{
	TLOG(TLVL_DEBUG) << "do_rollover_subrun BEGIN boundary=" << boundary;
	report_string_ = "";
	external_request_status_ = event_builder_ptr_->rollover_subrun(boundary);
	if (!external_request_status_)
	{
		report_string_ = "Error rolling over subrun in ";
		report_string_.append(app_name + "!");
	}
	TLOG(TLVL_DEBUG) << "do_rollover_subrun END sts=" << std::boolalpha << external_request_status_;
	return external_request_status_;
}

void artdaq::EventBuilderApp::BootedEnter()
{
	TLOG_DEBUG(app_name + "App") << "Booted state entry action called." ;

	// the destruction of any existing EventBuilderCore has to happen in the
	// Booted Entry action rather than the Initialized Exit action because the
	// Initialized Exit action is only called after the "init" transition guard
	// condition is executed.
	//event_builder_ptr_.reset(nullptr);
}

std::string artdaq::EventBuilderApp::report(std::string const& which) const
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

	// pass the request to the EventBuilderCore instance, if it's available
	if (event_builder_ptr_.get() != nullptr)
	{
		resultString.append(event_builder_ptr_->report(which));
	}
	else
	{
		resultString.append("This EventBuilder has not yet been initialized and ");
		resultString.append("therefore can not provide reporting.");
	}

	return resultString;
}

bool artdaq::EventBuilderApp::do_add_config_archive_entry(std::string const& key, std::string const& value)
{
	report_string_ = "";
	external_request_status_ = event_builder_ptr_->add_config_archive_entry(key, value);
	if (!external_request_status_)
	{
		report_string_ = "Error adding config entry with key ";
		report_string_.append(key + " and value \"");
		report_string_.append(value + "\" in");
		report_string_.append(app_name + ".");
	}

	return external_request_status_;
}

bool artdaq::EventBuilderApp::do_clear_config_archive()
{
	report_string_ = "";
	external_request_status_ = event_builder_ptr_->clear_config_archive();
	if (!external_request_status_)
	{
		report_string_ = "Error clearing the configuration archive in ";
		report_string_.append(app_name + ".");
	}

	return external_request_status_;
}
