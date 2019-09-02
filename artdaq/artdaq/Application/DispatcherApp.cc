#define TRACE_NAME "DispatcherApp"

#include "artdaq/Application/DispatcherApp.hh"
#include "artdaq/Application/DispatcherCore.hh"
#include "artdaq-core/Utilities/ExceptionHandler.hh"

#include <iostream>

artdaq::DispatcherApp::DispatcherApp()
{
}

// *******************************************************************
// *** The following methods implement the state machine operations.
// *******************************************************************

bool artdaq::DispatcherApp::do_initialize(fhicl::ParameterSet const& pset, uint64_t, uint64_t)
{
	report_string_ = "";

	//Dispatcher_ptr_.reset(nullptr);
	if (Dispatcher_ptr_.get() == 0)
	{
		Dispatcher_ptr_.reset(new DispatcherCore());
	}
	external_request_status_ = Dispatcher_ptr_->initialize(pset);
	if (!external_request_status_)
	{
		report_string_ = "Error initializing ";
		report_string_.append(app_name + " ");
		report_string_.append("with ParameterSet = \"" + pset.to_string() + "\".");
	}

	return external_request_status_;
}

bool artdaq::DispatcherApp::do_start(art::RunID id, uint64_t, uint64_t)
{
	report_string_ = "";
	external_request_status_ = Dispatcher_ptr_->start(id);
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

bool artdaq::DispatcherApp::do_stop(uint64_t, uint64_t)
{
	report_string_ = "";
	external_request_status_ = Dispatcher_ptr_->stop();
	if (!external_request_status_)
	{
		report_string_ = "Error stopping ";
		report_string_.append(app_name + ".");
	}

	return external_request_status_;
}

bool artdaq::DispatcherApp::do_pause(uint64_t, uint64_t)
{
	report_string_ = "";
	external_request_status_ = Dispatcher_ptr_->pause();
	if (!external_request_status_)
	{
		report_string_ = "Error pausing ";
		report_string_.append(app_name + ".");
	}
	return external_request_status_;
}

bool artdaq::DispatcherApp::do_resume(uint64_t, uint64_t)
{
	report_string_ = "";
	external_request_status_ = Dispatcher_ptr_->resume();
	if (!external_request_status_)
	{
		report_string_ = "Error resuming ";
		report_string_.append(app_name + ".");
	}

	return external_request_status_;
}

bool artdaq::DispatcherApp::do_shutdown(uint64_t)
{
	report_string_ = "";
	external_request_status_ = Dispatcher_ptr_->shutdown();
	if (!external_request_status_)
	{
		report_string_ = "Error shutting down ";
		report_string_.append(app_name + ".");
	}

	return external_request_status_;
}

bool artdaq::DispatcherApp::do_soft_initialize(fhicl::ParameterSet const&, uint64_t, uint64_t)
{
	return true;
}

bool artdaq::DispatcherApp::do_reinitialize(fhicl::ParameterSet const&, uint64_t, uint64_t)
{
	return true;
}

std::string artdaq::DispatcherApp::report(std::string const& which) const
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

	// pass the request to the DispatcherCore instance, if it's available
	if (Dispatcher_ptr_.get() != 0)
	{
		resultString.append(Dispatcher_ptr_->report(which));
	}
	else
	{
		resultString.append("This Dispatcher has not yet been initialized and ");
		resultString.append("therefore can not provide reporting.");
	}

	return resultString;
}

std::string artdaq::DispatcherApp::register_monitor(fhicl::ParameterSet const& info)
{
	TLOG(TLVL_DEBUG) << "DispatcherApp::register_monitor called with argument \"" << info.to_string() << "\"" ;

	if (Dispatcher_ptr_)
	{
		try
		{
			return Dispatcher_ptr_->register_monitor(info);
		}
		catch (...)
		{
			ExceptionHandler(ExceptionHandlerRethrow::no,
							 "Error in call to DispatcherCore's register_monitor function");

			return "Error in artdaq::DispatcherApp::register_monitor: an exception was thrown in the call to DispatcherCore::register_monitor, possibly due to a problem with the argument";
		}
	}
	else
	{
		return "Error in artdaq::DispatcherApp::register_monitor: DispatcherCore object wasn't initialized";
	}
}


std::string artdaq::DispatcherApp::unregister_monitor(std::string const& label)
{
	TLOG(TLVL_DEBUG) << "DispatcherApp::unregister_monitor called with argument \"" << label << "\"" ;

	if (Dispatcher_ptr_)
	{
		try
		{
			return Dispatcher_ptr_->unregister_monitor(label);
		}
		catch (...)
		{
			ExceptionHandler(ExceptionHandlerRethrow::no,
							 "Error in call to DispatcherCore's unregister_monitor function");

			return "Error in artdaq::DispatcherApp::unregister_monitor: an exception was thrown in the call to DispatcherCore::unregister_monitor, possibly due to a problem with the argument";
		}
	}
	else
	{
		return "Error in artdaq::DispatcherApp::unregister_monitor: DispatcherCore object wasn't initialized";
	}
}
