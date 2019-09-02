#define TRACE_NAME (app_name + "_BoardReaderApp").c_str()
#include "artdaq/DAQdata/Globals.hh"
#include "artdaq/Application/BoardReaderApp.hh"

artdaq::BoardReaderApp::BoardReaderApp()
	: fragment_receiver_ptr_(nullptr)
{
}

// *******************************************************************
// *** The following methods implement the state machine operations.
// *******************************************************************

bool artdaq::BoardReaderApp::do_initialize(fhicl::ParameterSet const& pset, uint64_t timeout, uint64_t timestamp)
{
	report_string_ = "";
	external_request_status_ = true;

	// in the following block, we first destroy the existing BoardReader
	// instance, then create a new one.  Doing it in one step does not
	// produce the desired result since that creates a new instance and
	// then deletes the old one, and we need the opposite order.
	TLOG(TLVL_DEBUG) << "Initializing first deleting old instance " << (void*)fragment_receiver_ptr_.get();
	fragment_receiver_ptr_.reset(nullptr);
	fragment_receiver_ptr_.reset(new BoardReaderCore(*this));
	TLOG(TLVL_DEBUG) << "Initializing new BoardReaderCore at " << (void*)fragment_receiver_ptr_.get() << " with pset " << pset.to_string();
	external_request_status_ = fragment_receiver_ptr_->initialize(pset, timeout, timestamp);
	if (!external_request_status_)
	{
		report_string_ = "Error initializing ";
		report_string_.append(app_name + " ");
		report_string_.append("with ParameterSet = \"" + pset.to_string() + "\".");
	}

	TLOG(TLVL_DEBUG) << "do_initialize(fhicl::ParameterSet, uint64_t, uint64_t): "
		<< "Done initializing.";
	return external_request_status_;
}

bool artdaq::BoardReaderApp::do_start(art::RunID id, uint64_t timeout, uint64_t timestamp)
{
	report_string_ = "";
	external_request_status_ = fragment_receiver_ptr_->start(id, timeout, timestamp);
	if (!external_request_status_)
	{
		report_string_ = "Error starting ";
		report_string_.append(app_name + " ");
		report_string_.append("for run number ");
		report_string_.append(boost::lexical_cast<std::string>(id.run()));
		report_string_.append(", timeout ");
		report_string_.append(boost::lexical_cast<std::string>(timeout));
		report_string_.append(", timestamp ");
		report_string_.append(boost::lexical_cast<std::string>(timestamp));
		report_string_.append(".");
	}

	boost::thread::attributes attrs;
	attrs.set_stack_size(4096 * 2000); // 8 MB
	try {
		fragment_processing_thread_ = boost::thread(attrs, boost::bind(&BoardReaderCore::process_fragments, fragment_receiver_ptr_.get()));
	}
	catch (const boost::exception& e)
	{
		TLOG(TLVL_ERROR) << "Caught boost::exception starting Fragment Processing thread: " << boost::diagnostic_information(e) << ", errno=" << errno;
		std::cerr << "Caught boost::exception starting Fragment Processing thread: " << boost::diagnostic_information(e) << ", errno=" << errno << std::endl;
		exit(5);
	}
	/*
	fragment_processing_future_ =
	std::async(std::launch::async, &BoardReaderCore::process_fragments,
	fragment_receiver_ptr_.get());*/

	return external_request_status_;
}

bool artdaq::BoardReaderApp::do_stop(uint64_t timeout, uint64_t timestamp)
{
	report_string_ = "";
	external_request_status_ = fragment_receiver_ptr_->stop(timeout, timestamp);
	if (!external_request_status_)
	{
		report_string_ = "Error stopping ";
		report_string_.append(app_name + ".");
		return false;
	}
	if (fragment_processing_thread_.joinable())
	{
		TLOG(TLVL_DEBUG) << "Joining fragment processing thread";
		fragment_processing_thread_.join();
	}

	TLOG(TLVL_DEBUG) << "BoardReader Stopped. Getting run statistics";
	int number_of_fragments_sent = -1;
	if(fragment_receiver_ptr_) number_of_fragments_sent = fragment_receiver_ptr_->GetFragmentsProcessed();
	TLOG(TLVL_DEBUG) << "do_stop(uint64_t, uint64_t): "
		<< "Number of fragments sent = " << number_of_fragments_sent
		<< ".";

	return external_request_status_;
}

bool artdaq::BoardReaderApp::do_pause(uint64_t timeout, uint64_t timestamp)
{
	report_string_ = "";
	external_request_status_ = fragment_receiver_ptr_->pause(timeout, timestamp);
	if (!external_request_status_)
	{
		report_string_ = "Error pausing ";
		report_string_.append(app_name + ".");
	}

	if(fragment_processing_thread_.joinable()) fragment_processing_thread_.join();
	int number_of_fragments_sent = fragment_receiver_ptr_->GetFragmentsProcessed();
	TLOG(TLVL_DEBUG) << "do_pause(uint64_t, uint64_t): "
		<< "Number of fragments sent = " << number_of_fragments_sent
		<< ".";

	return external_request_status_;
}

bool artdaq::BoardReaderApp::do_resume(uint64_t timeout, uint64_t timestamp)
{
	report_string_ = "";
	external_request_status_ = fragment_receiver_ptr_->resume(timeout, timestamp);
	if (!external_request_status_)
	{
		report_string_ = "Error resuming ";
		report_string_.append(app_name + ".");
	}

	boost::thread::attributes attrs;
	attrs.set_stack_size(4096 * 2000); // 8 MB
	fragment_processing_thread_ = boost::thread(attrs, boost::bind(&BoardReaderCore::process_fragments, fragment_receiver_ptr_.get()));
	/*
		fragment_processing_future_ =
			std::async(std::launch::async, &BoardReaderCore::process_fragments,
					   fragment_receiver_ptr_.get());*/

	return external_request_status_;
}

bool artdaq::BoardReaderApp::do_shutdown(uint64_t timeout)
{
	report_string_ = "";
	external_request_status_ = fragment_receiver_ptr_->shutdown(timeout);
        // 02-Jun-2018, ELF & KAB: it's very, very unlikely that the following call is needed,
        // but just in case...
	if (fragment_processing_thread_.joinable()) fragment_processing_thread_.join();
	if (!external_request_status_)
	{
		report_string_ = "Error shutting down ";
		report_string_.append(app_name + ".");
	}
	return external_request_status_;
}

bool artdaq::BoardReaderApp::do_soft_initialize(fhicl::ParameterSet const& pset, uint64_t timeout, uint64_t timestamp)
{
	report_string_ = "";
	external_request_status_ = fragment_receiver_ptr_->soft_initialize(pset, timeout, timestamp);
	if (!external_request_status_)
	{
		report_string_ = "Error soft-initializing ";
		report_string_.append(app_name + " ");
		report_string_.append("with ParameterSet = \"" + pset.to_string() + "\".");
	}
	return external_request_status_;
}

bool artdaq::BoardReaderApp::do_reinitialize(fhicl::ParameterSet const& pset, uint64_t timeout, uint64_t timestamp)
{
	external_request_status_ = fragment_receiver_ptr_->reinitialize(pset, timeout, timestamp);
	if (!external_request_status_)
	{
		report_string_ = "Error reinitializing ";
		report_string_.append(app_name + " ");
		report_string_.append("with ParameterSet = \"" + pset.to_string() + "\".");
	}
	return external_request_status_;
}

void artdaq::BoardReaderApp::BootedEnter()
{
	TLOG(TLVL_DEBUG) << "Booted state entry action called.";

	// the destruction of any existing BoardReaderCore has to happen in the
	// Booted Entry action rather than the Initialized Exit action because the
	// Initialized Exit action is only called after the "init" transition guard
	// condition is executed.
	fragment_receiver_ptr_.reset(nullptr);
}

bool artdaq::BoardReaderApp::do_meta_command(std::string const& command, std::string const& arg)
{
	external_request_status_ = fragment_receiver_ptr_->metaCommand(command, arg);
	if (!external_request_status_)
	{
		report_string_ = "Error running meta-command on ";
		report_string_.append(app_name + " ");
		report_string_.append("with command = \"" + command + "\", arg = \"" + arg + "\".");
	}
	return external_request_status_;
}

std::string artdaq::BoardReaderApp::report(std::string const& which) const
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

	// pass the request to the BoardReaderCore instance, if it's available
	if (fragment_receiver_ptr_.get() != 0)
	{
		resultString.append(fragment_receiver_ptr_->report(which));
	}
	else
	{
		resultString.append("This BoardReader has not yet been initialized and ");
		resultString.append("therefore can not provide reporting.");
	}

	return resultString;
}
