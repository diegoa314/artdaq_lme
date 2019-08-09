#define TRACE_NAME (app_name + "_CommandableFragmentGenerator").c_str() // include these 2 first -
#include "artdaq/DAQdata/Globals.hh"

#include "artdaq/Application/CommandableFragmentGenerator.hh"

#include <boost/exception/all.hpp>
#include <boost/throw_exception.hpp>

#include <limits>
#include <iterator>

#include "canvas/Utilities/Exception.h"
#include "cetlib_except/exception.h"
#include "fhiclcpp/ParameterSet.h"

#include "artdaq-core/Utilities/SimpleLookupPolicy.hh"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Data/ContainerFragmentLoader.hh"
#include "artdaq-core/Utilities/ExceptionHandler.hh"
#include "artdaq-core/Utilities/TimeUtils.hh"

#include <fstream>
#include <iomanip>
#include <iterator>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sys/poll.h>
#include "artdaq/DAQdata/TCPConnect.hh"

#define TLVL_GETNEXT 10
#define TLVL_GETNEXT_VERBOSE 20
#define TLVL_CHECKSTOP 11
#define TLVL_EVCOUNTERINC 12
#define TLVL_GETDATALOOP 13
#define TLVL_GETDATALOOP_DATABUFFWAIT 21
#define TLVL_GETDATALOOP_VERBOSE 20
#define TLVL_WAITFORBUFFERREADY 15
#define TLVL_GETBUFFERSTATS 16
#define TLVL_CHECKDATABUFFER 17
#define TLVL_GETMONITORINGDATA 18
#define TLVL_APPLYREQUESTS 9
#define TLVL_SENDEMPTYFRAGMENTS 19
#define TLVL_CHECKWINDOWS 14

artdaq::CommandableFragmentGenerator::CommandableFragmentGenerator()
	: mutex_()
	, requestReceiver_(nullptr)
	, windowOffset_(0)
	, windowWidth_(0)
	, staleTimeout_(Fragment::InvalidTimestamp)
	, expectedType_(Fragment::EmptyFragmentType)
	, maxFragmentCount_(std::numeric_limits<size_t>::max())
	, uniqueWindows_(true)
	, windows_sent_ooo_()
	, missing_request_window_timeout_us_(1000000)
	, window_close_timeout_us_(2000000)
	, useDataThread_(false)
	, circularDataBufferMode_(false)
	, sleep_on_no_data_us_(0)
	, data_thread_running_(false)
	, dataBufferDepthFragments_(0)
	, dataBufferDepthBytes_(0)
	, maxDataBufferDepthFragments_(1000)
	, maxDataBufferDepthBytes_(1000)
	, useMonitoringThread_(false)
	, monitoringInterval_(0)
	, lastMonitoringCall_()
	, isHardwareOK_(true)
	, dataBuffer_()
	, newDataBuffer_()
	, run_number_(-1)
	, subrun_number_(-1)
	, timeout_(std::numeric_limits<uint64_t>::max())
	, timestamp_(std::numeric_limits<uint64_t>::max())
	, should_stop_(false)
	, exception_(false)
	, force_stop_(false)
	, latest_exception_report_("none")
	, ev_counter_(1)
	, board_id_(-1)
	, instance_name_for_metrics_("FragmentGenerator")
	, sleep_on_stop_us_(0)
{}

artdaq::CommandableFragmentGenerator::CommandableFragmentGenerator(const fhicl::ParameterSet& ps)
	: mutex_()
	, requestReceiver_(nullptr)
	, windowOffset_(ps.get<Fragment::timestamp_t>("request_window_offset", 0))
	, windowWidth_(ps.get<Fragment::timestamp_t>("request_window_width", 0))
	, staleTimeout_(ps.get<Fragment::timestamp_t>("stale_request_timeout", 0xFFFFFFFF))
	, expectedType_(ps.get<Fragment::type_t>("expected_fragment_type", Fragment::type_t(Fragment::EmptyFragmentType)))
	, uniqueWindows_(ps.get<bool>("request_windows_are_unique", true))
	, windows_sent_ooo_()
	, missing_request_window_timeout_us_(ps.get<size_t>("missing_request_window_timeout_us", 5000000))
	, window_close_timeout_us_(ps.get<size_t>("window_close_timeout_us", 2000000))
	, useDataThread_(ps.get<bool>("separate_data_thread", false))
	, circularDataBufferMode_(ps.get<bool>("circular_buffer_mode", false))
	, sleep_on_no_data_us_(ps.get<size_t>("sleep_on_no_data_us", 0))
	, data_thread_running_(false)
	, dataBufferDepthFragments_(0)
	, dataBufferDepthBytes_(0)
	, maxDataBufferDepthFragments_(ps.get<int>("data_buffer_depth_fragments", 1000))
	, maxDataBufferDepthBytes_(ps.get<size_t>("data_buffer_depth_mb", 1000) * 1024 * 1024)
	, useMonitoringThread_(ps.get<bool>("separate_monitoring_thread", false))
	, monitoringInterval_(ps.get<int64_t>("hardware_poll_interval_us", 0))
	, lastMonitoringCall_()
	, isHardwareOK_(true)
	, dataBuffer_()
	, newDataBuffer_()
	, run_number_(-1)
	, subrun_number_(-1)
	, timeout_(std::numeric_limits<uint64_t>::max())
	, timestamp_(std::numeric_limits<uint64_t>::max())
	, should_stop_(false)
	, exception_(false)
	, force_stop_(false)
	, latest_exception_report_("none")
	, ev_counter_(1)
	, board_id_(-1)
	, sleep_on_stop_us_(0)
{
	board_id_ = ps.get<int>("board_id");
	instance_name_for_metrics_ = "BoardReader." + boost::lexical_cast<std::string>(board_id_);

	fragment_ids_ = ps.get<std::vector<artdaq::Fragment::fragment_id_t>>("fragment_ids", std::vector<artdaq::Fragment::fragment_id_t>());

	TLOG(TLVL_TRACE) << "artdaq::CommandableFragmentGenerator::CommandableFragmentGenerator(ps)";
	int fragment_id = ps.get<int>("fragment_id", -99);

	if (fragment_id != -99)
	{
		if (fragment_ids_.size() != 0)
		{
			latest_exception_report_ = "Error in CommandableFragmentGenerator: can't both define \"fragment_id\" and \"fragment_ids\" in FHiCL document";
			throw cet::exception(latest_exception_report_);
		}
		else
		{
			fragment_ids_.emplace_back(fragment_id);
		}
	}

	sleep_on_stop_us_ = ps.get<int>("sleep_on_stop_us", 0);

	dataBuffer_.emplace_back(FragmentPtr(new Fragment()));
	(*dataBuffer_.begin())->setSystemType(Fragment::EmptyFragmentType);

	std::string modeString = ps.get<std::string>("request_mode", "ignored");
	if (modeString == "single" || modeString == "Single")
	{
		mode_ = RequestMode::Single;
	}
	else if (modeString.find("buffer") != std::string::npos || modeString.find("Buffer") != std::string::npos)
	{
		mode_ = RequestMode::Buffer;
	}
	else if (modeString == "window" || modeString == "Window")
	{
		mode_ = RequestMode::Window;
	}
	else if (modeString.find("ignore") != std::string::npos || modeString.find("Ignore") != std::string::npos)
	{
		mode_ = RequestMode::Ignored;
	}
	TLOG(TLVL_DEBUG) << "Request mode is " << printMode_();

	if (mode_ != RequestMode::Ignored)
	{
		if (!useDataThread_)
		{
			latest_exception_report_ = "Error in CommandableFragmentGenerator: use_data_thread must be true when request_mode is not \"Ignored\"!";
			throw cet::exception(latest_exception_report_);
		}
		requestReceiver_.reset(new RequestReceiver(ps));
	}
}

artdaq::CommandableFragmentGenerator::~CommandableFragmentGenerator()
{
	joinThreads();
	requestReceiver_.reset(nullptr);
}

void artdaq::CommandableFragmentGenerator::joinThreads()
{
	should_stop_ = true;
	force_stop_ = true;
	TLOG(TLVL_DEBUG) << "Joining dataThread";
	if (dataThread_.joinable()) dataThread_.join();
	TLOG(TLVL_DEBUG) << "Joining monitoringThread";
	if (monitoringThread_.joinable()) monitoringThread_.join();
	TLOG(TLVL_DEBUG) << "joinThreads complete";
}

bool artdaq::CommandableFragmentGenerator::getNext(FragmentPtrs& output)
{
	bool result = true;

	if (check_stop()) usleep(sleep_on_stop_us_);
	if (exception() || force_stop_) return false;

	if (!useMonitoringThread_ && monitoringInterval_ > 0)
	{
		TLOG(TLVL_GETNEXT) << "getNext: Checking whether to collect Monitoring Data";
		auto now = std::chrono::steady_clock::now();

		if (TimeUtils::GetElapsedTimeMicroseconds(lastMonitoringCall_, now) >= static_cast<size_t>(monitoringInterval_))
		{
			TLOG(TLVL_GETNEXT) << "getNext: Collecting Monitoring Data";
			isHardwareOK_ = checkHWStatus_();
			TLOG(TLVL_GETNEXT) << "getNext: isHardwareOK_ is now " << std::boolalpha << isHardwareOK_;
			lastMonitoringCall_ = now;
		}
	}

	try
	{
		std::lock_guard<std::mutex> lk(mutex_);
		if (useDataThread_)
		{
			TLOG(TLVL_TRACE) << "getNext: Calling applyRequests";
			result = applyRequests(output);
			TLOG(TLVL_TRACE) << "getNext: Done with applyRequests result=" << std::boolalpha << result;
			for (auto dataIter = output.begin(); dataIter != output.end(); ++dataIter)
			{
				TLOG(20) << "getNext: applyRequests() returned fragment with sequenceID = " << (*dataIter)->sequenceID()
					<< ", timestamp = " << (*dataIter)->timestamp() << ", and sizeBytes = " << (*dataIter)->sizeBytes();
			}

			if (exception())
			{
				TLOG(TLVL_ERROR) << "Exception found in BoardReader with board ID " << board_id() << "; BoardReader will now return error status when queried";
				throw cet::exception("CommandableFragmentGenerator") << "Exception found in BoardReader with board ID " << board_id() << "; BoardReader will now return error status when queried";
			}
		}
		else
		{
			if (!isHardwareOK_)
			{
				TLOG(TLVL_ERROR) << "Stopping CFG because the hardware reports bad status!";
				return false;
			}
			TLOG(TLVL_TRACE) << "getNext: Calling getNext_ w/ ev_counter()=" << ev_counter();
			try
			{
				result = getNext_(output);
			}
			catch (...)
			{
				throw;
			}
			TLOG(TLVL_TRACE) << "getNext: Done with getNext_ - ev_counter() now " << ev_counter();
			for (auto dataIter = output.begin(); dataIter != output.end(); ++dataIter)
			{
				TLOG(TLVL_GETNEXT_VERBOSE) << "getNext: getNext_() returned fragment with sequenceID = " << (*dataIter)->sequenceID()
					<< ", timestamp = " << (*dataIter)->timestamp() << ", and sizeBytes = " << (*dataIter)->sizeBytes();
			}
		}
	}
	catch (const cet::exception& e)
	{
		latest_exception_report_ = "cet::exception caught in getNext(): ";
		latest_exception_report_.append(e.what());
		TLOG(TLVL_ERROR) << "getNext: cet::exception caught: " << e;
		set_exception(true);
		return false;
	}
	catch (const boost::exception& e)
	{
		latest_exception_report_ = "boost::exception caught in getNext(): ";
		latest_exception_report_.append(boost::diagnostic_information(e));
		TLOG(TLVL_ERROR) << "getNext: boost::exception caught: " << boost::diagnostic_information(e);
		set_exception(true);
		return false;
	}
	catch (const std::exception& e)
	{
		latest_exception_report_ = "std::exception caught in getNext(): ";
		latest_exception_report_.append(e.what());
		TLOG(TLVL_ERROR) << "getNext: std::exception caught: " << e.what();
		set_exception(true);
		return false;
	}
	catch (...)
	{
		latest_exception_report_ = "Unknown exception caught in getNext().";
		TLOG(TLVL_ERROR) << "getNext: unknown exception caught";
		set_exception(true);
		return false;
	}

	if (!result)
	{
		TLOG(TLVL_DEBUG) << "getNext: Either getNext_ or applyRequests returned false, stopping";
	}

	if (metricMan && !output.empty())
	{
		auto timestamp = output.front()->timestamp();

		if (output.size() > 1)
		{ // Only bother sorting if >1 entry                                            
			for (auto& outputfrag : output)
			{
				if (outputfrag->timestamp() > timestamp)
				{
					timestamp = outputfrag->timestamp();
				}
			}
		}

		metricMan->sendMetric("Last Timestamp", timestamp, "Ticks", 1,
			MetricMode::LastPoint, app_name);
	}

	return result;
}

bool artdaq::CommandableFragmentGenerator::check_stop()
{
	TLOG(TLVL_CHECKSTOP) << "CFG::check_stop: should_stop=" << should_stop() << ", useDataThread_=" << useDataThread_ << ", exception status =" << int(exception());

	if (!should_stop()) return false;
	if (!useDataThread_ || mode_ == RequestMode::Ignored) return true;
	if (force_stop_) return true;

	// check_stop returns true if the CFG should stop. We should wait for the RequestReceiver to stop before stopping.
	TLOG(TLVL_DEBUG) << "should_stop is true, force_stop_ is false, requestReceiver_->isRunning() is " << std::boolalpha << requestReceiver_->isRunning();
	return !requestReceiver_->isRunning();
}

int artdaq::CommandableFragmentGenerator::fragment_id() const
{
	if (fragment_ids_.size() != 1)
	{
		throw cet::exception("Error in CommandableFragmentGenerator: can't call fragment_id() unless member fragment_ids_ vector is length 1");
	}
	else
	{
		return fragment_ids_[0];
	}
}

size_t artdaq::CommandableFragmentGenerator::ev_counter_inc(size_t step, bool force)
{
	if (force || mode_ == RequestMode::Ignored)
	{
		TLOG(TLVL_EVCOUNTERINC) << "ev_counter_inc: Incrementing ev_counter from " << ev_counter() << " by " << step;
		return ev_counter_.fetch_add(step);
	}
	return ev_counter_.load();
} // returns the prev value

void artdaq::CommandableFragmentGenerator::StartCmd(int run, uint64_t timeout, uint64_t timestamp)
{
	TLOG(TLVL_TRACE) << "Start Command received.";
	if (run < 0) throw cet::exception("CommandableFragmentGenerator") << "negative run number";

	timeout_ = timeout;
	timestamp_ = timestamp;
	ev_counter_.store(1);
    windows_sent_ooo_.clear();
	dataBuffer_.clear();
	should_stop_.store(false);
	force_stop_.store(false);
	exception_.store(false);
	run_number_ = run;
	subrun_number_ = 1;
	latest_exception_report_ = "none";

	start();

	std::unique_lock<std::mutex> lk(mutex_);
	if (useDataThread_) startDataThread();
	if (useMonitoringThread_) startMonitoringThread();
	if (mode_ != RequestMode::Ignored && !requestReceiver_->isRunning()) requestReceiver_->startRequestReceiverThread();
	TLOG(TLVL_TRACE) << "Start Command complete.";
}

void artdaq::CommandableFragmentGenerator::StopCmd(uint64_t timeout, uint64_t timestamp)
{
	TLOG(TLVL_TRACE) << "Stop Command received.";

	timeout_ = timeout;
	timestamp_ = timestamp;
	if (requestReceiver_ && requestReceiver_->isRunning()) {
		TLOG(TLVL_DEBUG) << "Stopping Request receiver thread BEGIN";
		requestReceiver_->stopRequestReceiverThread();
		TLOG(TLVL_DEBUG) << "Stopping Request receiver thread END";
	}

	stopNoMutex();
	should_stop_.store(true);
	std::unique_lock<std::mutex> lk(mutex_);
	stop();

	joinThreads();
	TLOG(TLVL_TRACE) << "Stop Command complete.";
}

void artdaq::CommandableFragmentGenerator::PauseCmd(uint64_t timeout, uint64_t timestamp)
{
	TLOG(TLVL_TRACE) << "Pause Command received.";
	timeout_ = timeout;
	timestamp_ = timestamp;
	//if (requestReceiver_->isRunning()) requestReceiver_->stopRequestReceiverThread();

	pauseNoMutex();
	should_stop_.store(true);
	std::unique_lock<std::mutex> lk(mutex_);

	pause();
}

void artdaq::CommandableFragmentGenerator::ResumeCmd(uint64_t timeout, uint64_t timestamp)
{
	TLOG(TLVL_TRACE) << "Resume Command received.";
	timeout_ = timeout;
	timestamp_ = timestamp;

	subrun_number_ += 1;
	should_stop_ = false;
    {
        std::unique_lock<std::mutex> lk(dataBufferMutex_);
	dataBuffer_.clear();
    }
	// no lock required: thread not started yet
	resume();

	std::unique_lock<std::mutex> lk(mutex_);
	//if (useDataThread_) startDataThread();
	//if (useMonitoringThread_) startMonitoringThread();
	//if (mode_ != RequestMode::Ignored && !requestReceiver_->isRunning()) requestReceiver_->startRequestReceiverThread();
	TLOG(TLVL_TRACE) << "Resume Command complete.";
}

std::string artdaq::CommandableFragmentGenerator::ReportCmd(std::string const& which)
{
	TLOG(TLVL_TRACE) << "Report Command received.";
	std::lock_guard<std::mutex> lk(mutex_);

	// 14-May-2015, KAB: please see the comments associated with the report()
	// methods in the CommandableFragmentGenerator.hh file for more information
	// on the use of those methods in this method.

	// check if the child class has something meaningful for this request
	std::string childReport = reportSpecific(which);
	if (childReport.length() > 0) { return childReport; }

	// handle the requests that we can take care of at this level
	if (which == "latest_exception")
	{
		return latest_exception_report_;
	}

	// check if the child class has provided a catch-all report function
	childReport = report();
	if (childReport.length() > 0) { return childReport; }

	// if we haven't been able to come up with any report so far, say so
	std::string tmpString = "The \"" + which + "\" command is not ";
	tmpString.append("currently supported by the ");
	tmpString.append(metricsReportingInstanceName());
	tmpString.append(" fragment generator.");
	TLOG(TLVL_TRACE) << "Report Command complete.";
	return tmpString;
}

// Default implemenetations of state functions
void artdaq::CommandableFragmentGenerator::pauseNoMutex()
{
#pragma message "Using default implementation of CommandableFragmentGenerator::pauseNoMutex()"
}

void artdaq::CommandableFragmentGenerator::pause()
{
#pragma message "Using default implementation of CommandableFragmentGenerator::pause()"
}

void artdaq::CommandableFragmentGenerator::resume()
{
#pragma message "Using default implementation of CommandableFragmentGenerator::resume()"
}

std::string artdaq::CommandableFragmentGenerator::report()
{
#pragma message "Using default implementation of CommandableFragmentGenerator::report()"
	return "";
}

std::string artdaq::CommandableFragmentGenerator::reportSpecific(std::string const&)
{
#pragma message "Using default implementation of CommandableFragmentGenerator::reportSpecific(std::string)"
	return "";
}

bool artdaq::CommandableFragmentGenerator::checkHWStatus_()
{
#pragma message "Using default implementation of CommandableFragmentGenerator::checkHWStatus_()"
	return true;
}

bool artdaq::CommandableFragmentGenerator::metaCommand(std::string const&, std::string const&)
{
#pragma message "Using default implementation of CommandableFragmentGenerator::metaCommand(std::string, std::string)"
	return true;
}

void artdaq::CommandableFragmentGenerator::startDataThread()
{
	if (dataThread_.joinable()) dataThread_.join();
	TLOG(TLVL_INFO) << "Starting Data Receiver Thread";
	try {
		dataThread_ = boost::thread(&CommandableFragmentGenerator::getDataLoop, this);
	}
	catch (const boost::exception& e)
	{
		TLOG(TLVL_ERROR) << "Caught boost::exception starting Data Receiver thread: " << boost::diagnostic_information(e) << ", errno=" << errno;
		std::cerr << "Caught boost::exception starting Data Receiver thread: " << boost::diagnostic_information(e) << ", errno=" << errno << std::endl;
		exit(5);
	}
}

void artdaq::CommandableFragmentGenerator::startMonitoringThread()
{
	if (monitoringThread_.joinable()) monitoringThread_.join();
	TLOG(TLVL_INFO) << "Starting Hardware Monitoring Thread";
	try {
		monitoringThread_ = boost::thread(&CommandableFragmentGenerator::getMonitoringDataLoop, this);
	}
	catch (const boost::exception& e)
	{
		TLOG(TLVL_ERROR) << "Caught boost::exception starting Hardware Monitoring thread: " << boost::diagnostic_information(e) << ", errno=" << errno;
		std::cerr << "Caught boost::exception starting Hardware Monitoring thread: " << boost::diagnostic_information(e) << ", errno=" << errno << std::endl;
		exit(5);
	}
}

std::string artdaq::CommandableFragmentGenerator::printMode_()
{
	switch (mode_)
	{
	case RequestMode::Single:
		return "Single";
	case RequestMode::Buffer:
		return "Buffer";
	case RequestMode::Window:
		return "Window";
	case RequestMode::Ignored:
		return "Ignored";
	}

	return "ERROR";
}


//
// The "useDataThread_" thread
//
void artdaq::CommandableFragmentGenerator::getDataLoop()
{
	data_thread_running_ = true;
	while (!force_stop_)
	{
		if (!isHardwareOK_)
		{
			TLOG(TLVL_DEBUG) << "getDataLoop: isHardwareOK is " << isHardwareOK_ << ", aborting data thread";
			data_thread_running_ = false;
			return;
		}

		TLOG(TLVL_GETDATALOOP) << "getDataLoop: calling getNext_";

		bool data = false;
		auto startdata = std::chrono::steady_clock::now();

		try
		{
			data = getNext_(newDataBuffer_);
		}
		catch (...)
		{
			ExceptionHandler(ExceptionHandlerRethrow::no,
				"Exception thrown by fragment generator in CommandableFragmentGenerator::getDataLoop; setting exception state to \"true\"");
			set_exception(true);

			data_thread_running_ = false;
			return;
		}
		for (auto dataIter = newDataBuffer_.begin(); dataIter != newDataBuffer_.end(); ++dataIter)
		{
			TLOG(TLVL_GETDATALOOP_VERBOSE) << "getDataLoop: getNext_() returned fragment with timestamp = " << (*dataIter)->timestamp() << ", and sizeBytes = " << (*dataIter)->sizeBytes();
		}

		if (metricMan)
		{
			metricMan->sendMetric("Avg Data Acquisition Time", TimeUtils::GetElapsedTime(startdata), "s", 3, artdaq::MetricMode::Average);
		}

		if (newDataBuffer_.size() == 0 && sleep_on_no_data_us_ > 0)
		{
			usleep(sleep_on_no_data_us_);
		}

		TLOG(TLVL_GETDATALOOP_DATABUFFWAIT) << "Waiting for data buffer ready";
		if (!waitForDataBufferReady()) return;
		TLOG(TLVL_GETDATALOOP_DATABUFFWAIT) << "Done waiting for data buffer ready";

		TLOG(TLVL_GETDATALOOP) << "getDataLoop: processing data";
		if (data && !force_stop_)
		{
			std::unique_lock<std::mutex> lock(dataBufferMutex_);
			switch (mode_)
			{
			case RequestMode::Single:
				// While here, if for some strange reason more than one event's worth of data is returned from getNext_...
				while (newDataBuffer_.size() >= fragment_ids_.size())
				{
					dataBuffer_.clear();
					auto it = newDataBuffer_.begin();
					std::advance(it, fragment_ids_.size());
					dataBuffer_.splice(dataBuffer_.end(), newDataBuffer_, newDataBuffer_.begin(), it);
				}
				break;
			case RequestMode::Buffer:
			case RequestMode::Ignored:
			case RequestMode::Window:
			default:
				//dataBuffer_.reserve(dataBuffer_.size() + newDataBuffer_.size());
				dataBuffer_.splice(dataBuffer_.end(), newDataBuffer_);
				break;
			}
			getDataBufferStats();
		}

		{
			std::unique_lock<std::mutex> lock(dataBufferMutex_);
			if (dataBuffer_.size() > 0)
			{
				dataCondition_.notify_all();
			}
		}
		if (!data || force_stop_)
		{
			TLOG(TLVL_INFO) << "Data flow has stopped. Ending data collection thread";
			std::unique_lock<std::mutex> lock(dataBufferMutex_);
			data_thread_running_ = false;
			if (requestReceiver_) requestReceiver_->ClearRequests();
			newDataBuffer_.clear();
			TLOG(TLVL_INFO) << "getDataLoop: Ending thread";
			return;
		}
	}
}

bool artdaq::CommandableFragmentGenerator::waitForDataBufferReady()
{
	auto startwait = std::chrono::steady_clock::now();
	auto first = true;
	auto lastwaittime = 0ULL;

	{
		std::unique_lock<std::mutex> lock(dataBufferMutex_);
		getDataBufferStats();
	}

	while (dataBufferIsTooLarge())
	{
		if (!circularDataBufferMode_)
		{
			if (should_stop())
			{
				TLOG(TLVL_DEBUG) << "Run ended while waiting for buffer to shrink!";
				std::unique_lock<std::mutex> lock(dataBufferMutex_);
				getDataBufferStats();
				dataCondition_.notify_all();
				data_thread_running_ = false;
				return false;
			}
			auto waittime = TimeUtils::GetElapsedTimeMilliseconds(startwait);

			if (first || (waittime != lastwaittime && waittime % 1000 == 0))
			{
				TLOG(TLVL_WARNING) << "Bad Omen: Data Buffer has exceeded its size limits. "
					<< "(seq_id=" << ev_counter()
					<< ", frags=" << dataBufferDepthFragments_ << "/" << maxDataBufferDepthFragments_
					<< ", szB=" << dataBufferDepthBytes_ << "/" << maxDataBufferDepthBytes_ << ")";
				TLOG(TLVL_TRACE) << "Bad Omen: Possible causes include requests not getting through or Ignored-mode BR issues";
				first = false;
			}
			if (waittime % 5 && waittime != lastwaittime)
			{
				TLOG(TLVL_WAITFORBUFFERREADY) << "getDataLoop: Data Retreival paused for " << waittime << " ms waiting for data buffer to drain";
			}
			lastwaittime = waittime;
			usleep(1000);
		}
		else
		{
			std::unique_lock<std::mutex> lock(dataBufferMutex_);
			getDataBufferStats(); // Re-check under lock
			if (dataBufferIsTooLarge())
			{
				if (dataBuffer_.begin() == dataBuffer_.end())
				{
					TLOG(TLVL_WARNING) << "Data buffer is reported as too large, but doesn't contain any Fragments! Possible corrupt memory!";
					continue;
				}
				if (*dataBuffer_.begin())
				{
					TLOG(TLVL_WAITFORBUFFERREADY) << "waitForDataBufferReady: Dropping Fragment with timestamp " << (*dataBuffer_.begin())->timestamp() << " from data buffer (Buffer over-size, circular data buffer mode)";
				}
				dataBuffer_.erase(dataBuffer_.begin());
				getDataBufferStats();
			}

		}
	}
	return true;
}

bool artdaq::CommandableFragmentGenerator::dataBufferIsTooLarge()
{
	return (maxDataBufferDepthFragments_ > 0 && dataBufferDepthFragments_ > maxDataBufferDepthFragments_) || (maxDataBufferDepthBytes_ > 0 && dataBufferDepthBytes_ > maxDataBufferDepthBytes_);
}

void artdaq::CommandableFragmentGenerator::getDataBufferStats()
{
	/// dataBufferMutex must be owned by the calling thread!
	dataBufferDepthFragments_ = dataBuffer_.size();
	size_t acc = 0;
	TLOG(TLVL_GETBUFFERSTATS) << "getDataBufferStats: Calculating buffer size";
	for (auto i = dataBuffer_.begin(); i != dataBuffer_.end(); ++i)
	{
		if (i->get() != nullptr)
		{
			acc += (*i)->sizeBytes();
		}
	}
	dataBufferDepthBytes_ = acc;

	if (metricMan)
	{
		TLOG(TLVL_GETBUFFERSTATS) << "getDataBufferStats: Sending Metrics";
		metricMan->sendMetric("Buffer Depth Fragments", dataBufferDepthFragments_.load(), "fragments", 1, MetricMode::LastPoint);
		metricMan->sendMetric("Buffer Depth Bytes", dataBufferDepthBytes_.load(), "bytes", 1, MetricMode::LastPoint);
	}
	TLOG(TLVL_GETBUFFERSTATS) << "getDataBufferStats: frags=" << dataBufferDepthFragments_.load() << "/" << maxDataBufferDepthFragments_
		<< ", sz=" << dataBufferDepthBytes_.load() << "/" << maxDataBufferDepthBytes_;
}

void artdaq::CommandableFragmentGenerator::checkDataBuffer()
{
	std::unique_lock<std::mutex> lock(dataBufferMutex_);
	dataCondition_.wait_for(lock, std::chrono::milliseconds(10));
	if (dataBufferDepthFragments_ > 0)
	{
		if ((mode_ == RequestMode::Buffer || mode_ == RequestMode::Window))
		{
			// Eliminate extra fragments
			while (dataBufferIsTooLarge())
			{
				TLOG(TLVL_CHECKDATABUFFER) << "checkDataBuffer: Dropping Fragment with timestamp " << (*dataBuffer_.begin())->timestamp() << " from data buffer (Buffer over-size)";
				dataBuffer_.erase(dataBuffer_.begin());
				getDataBufferStats();
			}
			if (dataBuffer_.size() > 0)
			{
				TLOG(TLVL_CHECKDATABUFFER) << "Determining if Fragments can be dropped from data buffer";
				Fragment::timestamp_t last = dataBuffer_.back()->timestamp();
				Fragment::timestamp_t min = last > staleTimeout_ ? last - staleTimeout_ : 0;
				for (auto it = dataBuffer_.begin(); it != dataBuffer_.end();)
				{
					if ((*it)->timestamp() < min)
					{
						TLOG(TLVL_CHECKDATABUFFER) << "checkDataBuffer: Dropping Fragment with timestamp " << (*it)->timestamp() << " from data buffer (timeout=" << staleTimeout_ << ", min=" << min << ")";
						it = dataBuffer_.erase(it);
					}
					else
					{
						++it;
					}
				}
				getDataBufferStats();
			}
		}
		else if (mode_ == RequestMode::Single && dataBuffer_.size() > fragment_ids_.size())
		{
			// Eliminate extra fragments
			while (dataBuffer_.size() > fragment_ids_.size())
			{
				dataBuffer_.erase(dataBuffer_.begin());
			}
		}
	}
}

void artdaq::CommandableFragmentGenerator::getMonitoringDataLoop()
{
	while (!force_stop_)
	{
		if (should_stop() || monitoringInterval_ <= 0)
		{
			TLOG(TLVL_DEBUG) << "getMonitoringDataLoop: should_stop() is " << std::boolalpha << should_stop()
				<< " and monitoringInterval is " << monitoringInterval_ << ", returning";
			return;
		}
		TLOG(TLVL_GETMONITORINGDATA) << "getMonitoringDataLoop: Determining whether to call checkHWStatus_";

		auto now = std::chrono::steady_clock::now();
		if (TimeUtils::GetElapsedTimeMicroseconds(lastMonitoringCall_, now) >= static_cast<size_t>(monitoringInterval_))
		{
			isHardwareOK_ = checkHWStatus_();
			TLOG(TLVL_GETMONITORINGDATA) << "getMonitoringDataLoop: isHardwareOK_ is now " << std::boolalpha << isHardwareOK_;
			lastMonitoringCall_ = now;
		}
		usleep(monitoringInterval_ / 10);
	}
}

void artdaq::CommandableFragmentGenerator::applyRequestsIgnoredMode(artdaq::FragmentPtrs& frags)
{
	// We just copy everything that's here into the output.
	TLOG(TLVL_APPLYREQUESTS) << "Mode is Ignored; Copying data to output";
	std::move(dataBuffer_.begin(), dataBuffer_.end(), std::inserter(frags, frags.end()));
	dataBuffer_.clear();
}

void artdaq::CommandableFragmentGenerator::applyRequestsSingleMode(artdaq::FragmentPtrs& frags)
{
	// We only care about the latest request received. Send empties for all others.
	auto requests = requestReceiver_->GetRequests();
	while (requests.size() > 1)
	{
		// std::map is ordered by key => Last sequence ID in the map is the one we care about
		requestReceiver_->RemoveRequest(requests.begin()->first);
		requests.erase(requests.begin());
	}
	sendEmptyFragments(frags, requests);

	// If no requests remain after sendEmptyFragments, return
	if (requests.size() == 0 || !requests.count(ev_counter())) return;

	if (dataBuffer_.size() > 0)
	{
		TLOG(TLVL_APPLYREQUESTS) << "Mode is Single; Sending copy of last event";
		for (auto& fragptr : dataBuffer_)
		{
			// Return the latest data point
			auto frag = fragptr.get();
			auto newfrag = std::unique_ptr<artdaq::Fragment>(new Fragment(ev_counter(), frag->fragmentID()));
			newfrag->resize(frag->size() - detail::RawFragmentHeader::num_words());
			memcpy(newfrag->headerAddress(), frag->headerAddress(), frag->sizeBytes());
			newfrag->setTimestamp(requests[ev_counter()]);
			newfrag->setSequenceID(ev_counter());
			frags.push_back(std::move(newfrag));
		}
	}
	else
	{
		sendEmptyFragment(frags, ev_counter(), "No data for");
	}
	requestReceiver_->RemoveRequest(ev_counter());
	ev_counter_inc(1, true);
}

void artdaq::CommandableFragmentGenerator::applyRequestsBufferMode(artdaq::FragmentPtrs& frags)
{
	// We only care about the latest request received. Send empties for all others.
	auto requests = requestReceiver_->GetRequests();
	while (requests.size() > 1)
	{
		// std::map is ordered by key => Last sequence ID in the map is the one we care about
		requestReceiver_->RemoveRequest(requests.begin()->first);
		requests.erase(requests.begin());
	}
	sendEmptyFragments(frags, requests);

	// If no requests remain after sendEmptyFragments, return
	if (requests.size() == 0 || !requests.count(ev_counter())) return;

	TLOG(TLVL_DEBUG) << "Creating ContainerFragment for Buffered Fragments";
	frags.emplace_back(new artdaq::Fragment(ev_counter(), fragment_id()));
	frags.back()->setTimestamp(requests[ev_counter()]);
	ContainerFragmentLoader cfl(*frags.back());
	cfl.set_missing_data(false); // Buffer mode is never missing data, even if there IS no data.

	// Buffer mode TFGs should simply copy out the whole dataBuffer_ into a ContainerFragment
	// Window mode TFGs must do a little bit more work to decide which fragments to send for a given request
	for (auto it = dataBuffer_.begin(); it != dataBuffer_.end();)
	{
		TLOG(TLVL_APPLYREQUESTS) << "ApplyRequests: Adding Fragment with timestamp " << (*it)->timestamp() << " to Container with sequence ID " << ev_counter();
		cfl.addFragment(*it);
		it = dataBuffer_.erase(it);
	}
	requestReceiver_->RemoveRequest(ev_counter());
	ev_counter_inc(1, true);
}

void artdaq::CommandableFragmentGenerator::applyRequestsWindowMode(artdaq::FragmentPtrs& frags)
{
	TLOG(TLVL_APPLYREQUESTS) << "applyRequestsWindowMode BEGIN";

	auto requests = requestReceiver_->GetRequests();

	TLOG(TLVL_APPLYREQUESTS) << "applyRequestsWindowMode: Starting request processing";
	for (auto req = requests.begin(); req != requests.end();)
	{
		TLOG(TLVL_APPLYREQUESTS) << "applyRequestsWindowMode: processing request with sequence ID " << req->first << ", timestamp " << req->second;


		while (req->first < ev_counter() && requests.size() > 0)
		{
			TLOG(TLVL_APPLYREQUESTS) << "applyRequestsWindowMode: Clearing passed request for sequence ID " << req->first;
			requestReceiver_->RemoveRequest(req->first);
			req = requests.erase(req);
		}
		if (requests.size() == 0) break;

		auto ts = req->second;
		TLOG(TLVL_APPLYREQUESTS) << "applyRequests: Checking that data exists for request window " << req->first;
		Fragment::timestamp_t min = ts > windowOffset_ ? ts - windowOffset_ : 0;
		Fragment::timestamp_t max = min + windowWidth_;
		TLOG(TLVL_APPLYREQUESTS) << "ApplyRequests: min is " << min << ", max is " << max
			<< " and last point in buffer is " << (dataBuffer_.size() > 0 ? dataBuffer_.back()->timestamp() : 0) << " (sz=" << dataBuffer_.size() << ")";
		bool windowClosed = dataBuffer_.size() > 0 && dataBuffer_.back()->timestamp() >= max;
		bool windowTimeout = !windowClosed && TimeUtils::GetElapsedTimeMicroseconds(requestReceiver_->GetRequestTime(req->first)) > window_close_timeout_us_;
		if (windowTimeout)
		{
			TLOG(TLVL_WARNING) << "applyRequests: A timeout occurred waiting for data to close the request window ({" << min << "-" << max
				<< "}, buffer={" << (dataBuffer_.size() > 0 ? dataBuffer_.front()->timestamp() : 0) << "-"
				<< (dataBuffer_.size() > 0 ? dataBuffer_.back()->timestamp() : 0)
				<< "} ). Time waiting: "
				<< TimeUtils::GetElapsedTimeMicroseconds(requestReceiver_->GetRequestTime(req->first)) << " us "
				<< "(> " << window_close_timeout_us_ << " us).";
		}
		if (windowClosed || !data_thread_running_ || windowTimeout)
		{
			TLOG(TLVL_DEBUG) << "applyRequests: Creating ContainerFragment for Window-requested Fragments";
			frags.emplace_back(new artdaq::Fragment(req->first, fragment_id()));
			frags.back()->setTimestamp(ts);
			ContainerFragmentLoader cfl(*frags.back());

			// In the spirit of NOvA's MegaPool: (RS = Request start (min), RE = Request End (max))
			//  --- | Buffer Start | --- | Buffer End | ---
			//1. RS RE |           |     |            | 
			//2. RS |              |  RE |            |   
			//3. RS |              |     |            | RE
			//4.    |              | RS RE |          |
			//5.    |              | RS  |            | RE
			//6.    |              |     |            | RS RE
			//
			// If RE (or RS) is after the end of the buffer, we wait for window_close_timeout_us_. If we're here, then that means that windowClosed is false, and the missing_data flag should be set.
			// If RS (or RE) is before the start of the buffer, then missing_data should be set to true, as data is assumed to arrive in the buffer in timestamp order
			// If the dataBuffer has size 0, then windowClosed will be false
			if (!windowClosed || (dataBuffer_.size() > 0 && dataBuffer_.front()->timestamp() > min))
			{
				TLOG(TLVL_DEBUG) << "applyRequests: Request window starts before and/or ends after the current data buffer, setting ContainerFragment's missing_data flag!"
					<< " (requestWindowRange=[" << min << "," << max << "], "
					<< "buffer={" << (dataBuffer_.size() > 0 ? dataBuffer_.front()->timestamp() : 0) << "-"
					<< (dataBuffer_.size() > 0 ? dataBuffer_.back()->timestamp() : 0) << "}";
				cfl.set_missing_data(true);
			}

			// Buffer mode TFGs should simply copy out the whole dataBuffer_ into a ContainerFragment
			// Window mode TFGs must do a little bit more work to decide which fragments to send for a given request
			for (auto it = dataBuffer_.begin(); it != dataBuffer_.end();)
			{
				Fragment::timestamp_t fragT = (*it)->timestamp();
				if (fragT < min || fragT > max || (fragT == max && windowWidth_ > 0))
				{
					++it;
					continue;
				}

				TLOG(TLVL_APPLYREQUESTS) << "applyRequests: Adding Fragment with timestamp " << (*it)->timestamp() << " to Container";
				cfl.addFragment(*it);

				if (uniqueWindows_)
				{
					it = dataBuffer_.erase(it);
				}
				else
				{
					++it;
				}
			}
			requestReceiver_->RemoveRequest(req->first);
			checkOutOfOrderWindows(req->first);
			requestReceiver_->RemoveRequest(req->first);
			req = requests.erase(req);
		}
		else
		{
			++req;
		}
	}
}

bool artdaq::CommandableFragmentGenerator::applyRequests(artdaq::FragmentPtrs& frags)
{
	if (check_stop() || exception())
	{
		return false;
	}

	// Wait for data, if in ignored mode, or a request otherwise
	if (mode_ == RequestMode::Ignored)
	{
		while (dataBufferDepthFragments_ <= 0)
		{
			if (check_stop() || exception() || !isHardwareOK_) return false;
			std::unique_lock<std::mutex> lock(dataBufferMutex_);
			dataCondition_.wait_for(lock, std::chrono::milliseconds(10), [this]() { return dataBufferDepthFragments_ > 0; });
		}
	}
	else
	{
		if ((check_stop() && requestReceiver_->size() == 0) || exception()) return false;
		checkDataBuffer();

		// Wait up to 1000 ms for a request...
		auto counter = 0;

		while (requestReceiver_->size() == 0 && counter < 100)
		{
			if (check_stop() || exception()) return false;

			checkDataBuffer();

			requestReceiver_->WaitForRequests(10); // milliseconds
			counter++;
		}
	}

	{
		std::unique_lock<std::mutex> dlk(dataBufferMutex_);

		switch (mode_)
		{
		case RequestMode::Single:
			applyRequestsSingleMode(frags);
			break;
		case RequestMode::Window:
			applyRequestsWindowMode(frags);
			break;
		case RequestMode::Buffer:
			applyRequestsBufferMode(frags);
			break;
		case RequestMode::Ignored:
		default:
			applyRequestsIgnoredMode(frags);
			break;
		}

		if (!data_thread_running_ || force_stop_)
		{
			TLOG(TLVL_INFO) << "Data thread has stopped; Clearing data buffer";
			dataBuffer_.clear();
		}

		getDataBufferStats();
	}

	if (frags.size() > 0)
		TLOG(TLVL_APPLYREQUESTS) << "Finished Processing Event " << (*frags.begin())->sequenceID() << " for fragment_id " << fragment_id() << ".";
	return true;
}

bool artdaq::CommandableFragmentGenerator::sendEmptyFragment(artdaq::FragmentPtrs& frags, size_t seqId, std::string desc)
{
	TLOG(TLVL_WARNING) << desc << " sequence ID " << seqId << ", sending empty fragment";
	for (auto fid : fragment_ids_)
	{
		auto frag = new Fragment();
		frag->setSequenceID(seqId);
		frag->setFragmentID(fid);
		frag->setSystemType(Fragment::EmptyFragmentType);
		frags.emplace_back(FragmentPtr(frag));
	}
	return true;
}

void artdaq::CommandableFragmentGenerator::sendEmptyFragments(artdaq::FragmentPtrs& frags, std::map<Fragment::sequence_id_t, Fragment::timestamp_t>& requests)
{
	if (requests.size() > 0)
	{
		TLOG(TLVL_SENDEMPTYFRAGMENTS) << "Sending Empty Fragments for Sequence IDs from " << ev_counter() << " up to but not including " << requests.begin()->first;
		while (requests.begin()->first > ev_counter())
		{
			sendEmptyFragment(frags, ev_counter(), "Missed request for");
			ev_counter_inc(1, true);
		}
	}
}

void artdaq::CommandableFragmentGenerator::checkOutOfOrderWindows(artdaq::Fragment::sequence_id_t seq)
{
	windows_sent_ooo_[seq] = std::chrono::steady_clock::now();

	auto it = windows_sent_ooo_.begin();
	while (it != windows_sent_ooo_.end())
	{
		if (seq == it->first && it->first == ev_counter())
		{
			TLOG(TLVL_CHECKWINDOWS) << "checkOutOfOrderWindows: Sequence ID matches ev_counter, incrementing ev_counter (" << ev_counter() << ")";
			ev_counter_inc(1, true);
			it = windows_sent_ooo_.erase(it);
		}
		else if (it->first <= ev_counter())
		{
			TLOG(TLVL_CHECKWINDOWS) << "checkOutOfOrderWindows: Data-taking has caught up to out-of-order window request " << it->first << ", removing from list. ev_counter=" << ev_counter();
			requestReceiver_->RemoveRequest(ev_counter());
			if (it->first == ev_counter()) ev_counter_inc(1, true);
			it = windows_sent_ooo_.erase(it);
		}
		else if (TimeUtils::GetElapsedTimeMicroseconds(it->second) > missing_request_window_timeout_us_)
		{
			TLOG(TLVL_CHECKWINDOWS) << "checkOutOfOrderWindows: Out-of-order window " << it->first << " has timed out, setting current sequence ID and removing from list";
			while (ev_counter() <= it->first)
			{
				if (ev_counter() < it->first) TLOG(TLVL_WARNING) << "Missed request for sequence ID " << ev_counter() << "! Will not send any data for this sequence ID!";
				requestReceiver_->RemoveRequest(ev_counter());
				ev_counter_inc(1, true);
			}
			windows_sent_ooo_.erase(windows_sent_ooo_.begin(), it);
			it = windows_sent_ooo_.erase(it);
		}
		else
		{
			TLOG(TLVL_CHECKWINDOWS) << "checkOutOfOrderWindows: Out-of-order window " << it->first << " waiting. Current event counter = " << ev_counter();
			++it;
		}
	}
}

