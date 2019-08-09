
#define TRACE_NAME (app_name + "_CommandableInterface").c_str()

#include "artdaq/Application/Commandable.hh"
#include "artdaq/DAQdata/Globals.hh"

// ELF 3/22/18:
// We may want to separate these onto different levels later, 
// but I think the TRACE_NAME separation is enough for now...
#define TLVL_INIT TLVL_INFO
#define TLVL_STOP TLVL_INFO
#define TLVL_STATUS TLVL_TRACE
#define TLVL_START TLVL_INFO
#define TLVL_PAUSE TLVL_INFO
#define TLVL_RESUME TLVL_INFO
#define TLVL_SHUTDOWN TLVL_INFO
#define TLVL_ROLLOVER TLVL_INFO
#define TLVL_SOFT_INIT TLVL_INFO
#define TLVL_REINIT TLVL_INFO
#define TLVL_INRUN_FAILURE TLVL_INFO
#define TLVL_LEGAL_COMMANDS TLVL_INFO

artdaq::Commandable::Commandable() : fsm_(*this)
, primary_mutex_()
{}

// **********************************************************************
// *** The following methods implement the externally available commands.
// **********************************************************************

bool artdaq::Commandable::initialize(fhicl::ParameterSet const& pset, uint64_t timeout, uint64_t timestamp)
{
	std::lock_guard<std::mutex> lk(primary_mutex_);
	external_request_status_ = true;
	report_string_ = "All is OK.";


	SetMFModuleName("Initializing");

	TLOG(TLVL_INIT) << "Initialize transition started";
	auto start_time = std::chrono::steady_clock::now();
	std::string initialState = fsm_.getState().getName();
	fsm_.init(pset, timeout, timestamp);
	if (external_request_status_)
	{
		std::string finalState = fsm_.getState().getName();

		SetMFModuleName(finalState);
		TLOG(TLVL_DEBUG)
			<< "States before and after an init transition: "
			<< initialState << " and " << finalState << ". Transition Duration: " << TimeUtils::GetElapsedTime(start_time) << " s.";
	}
	if (metricMan) metricMan->sendMetric("DAQ Transition Time", TimeUtils::GetElapsedTime(start_time), "s", 4, artdaq::MetricMode::Accumulate);

	TLOG(TLVL_INIT) << "Initialize transition complete";
	return (external_request_status_);
}

bool artdaq::Commandable::start(art::RunID id, uint64_t timeout, uint64_t timestamp)
{
	std::lock_guard<std::mutex> lk(primary_mutex_);
	external_request_status_ = true;
	report_string_ = "All is OK.";

	SetMFModuleName("Starting");

	TLOG(TLVL_START) << "Start transition started";
	auto start_time = std::chrono::steady_clock::now();
	std::string initialState = fsm_.getState().getName();
	fsm_.start(id, timeout, timestamp);
	if (external_request_status_)
	{
		std::string finalState = fsm_.getState().getName();

		SetMFModuleName(finalState);
		TLOG(TLVL_DEBUG)
			<< "States before and after a start transition: "
			<< initialState << " and " << finalState << ". Transition Duration: " << TimeUtils::GetElapsedTime(start_time) << " s.";
	}
	if (metricMan) metricMan->sendMetric("DAQ Transition Time", TimeUtils::GetElapsedTime(start_time), "s", 4, artdaq::MetricMode::Accumulate);

	TLOG(TLVL_START) << "Start transition complete";
	return (external_request_status_);
}

bool artdaq::Commandable::stop(uint64_t timeout, uint64_t timestamp)
{
	std::lock_guard<std::mutex> lk(primary_mutex_);
	external_request_status_ = true;
	report_string_ = "All is OK.";

	SetMFModuleName("Stopping");

	TLOG(TLVL_STOP) << "Stop transition started";
	auto start_time = std::chrono::steady_clock::now();
	std::string initialState = fsm_.getState().getName();
	fsm_.stop(timeout, timestamp);
	if (external_request_status_)
	{
		std::string finalState = fsm_.getState().getName();

		SetMFModuleName(finalState);
		TLOG(TLVL_DEBUG)
			<< "States before and after a stop transition: "
			<< initialState << " and " << finalState << ". Transition Duration: " << TimeUtils::GetElapsedTime(start_time) << " s.";
	}
	if (metricMan) metricMan->sendMetric("DAQ Transition Time", TimeUtils::GetElapsedTime(start_time), "s", 4, artdaq::MetricMode::Accumulate);

	TLOG(TLVL_STOP) << "Stop transition complete";
	return (external_request_status_);
}

bool artdaq::Commandable::pause(uint64_t timeout, uint64_t timestamp)
{
	std::lock_guard<std::mutex> lk(primary_mutex_);
	external_request_status_ = true;
	report_string_ = "All is OK.";

	SetMFModuleName("Pausing");

	TLOG(TLVL_PAUSE) << "Pause transition started";
	auto start_time = std::chrono::steady_clock::now();
	std::string initialState = fsm_.getState().getName();
	fsm_.pause(timeout, timestamp);
	if (external_request_status_)
	{
		std::string finalState = fsm_.getState().getName();

		SetMFModuleName(finalState);
		TLOG(TLVL_DEBUG)
			<< "States before and after a pause transition: "
			<< initialState << " and " << finalState << ". Transition Duration: " << TimeUtils::GetElapsedTime(start_time) << " s.";
	}
	if (metricMan) metricMan->sendMetric("DAQ Transition Time", TimeUtils::GetElapsedTime(start_time), "s", 4, artdaq::MetricMode::Accumulate);

	TLOG(TLVL_PAUSE) << "Pause transition complete";
	return (external_request_status_);
}

bool artdaq::Commandable::resume(uint64_t timeout, uint64_t timestamp)
{
	std::lock_guard<std::mutex> lk(primary_mutex_);
	external_request_status_ = true;
	report_string_ = "All is OK.";

	SetMFModuleName("Resuming");

	TLOG(TLVL_RESUME) << "Resume transition started";
	auto start_time = std::chrono::steady_clock::now();
	std::string initialState = fsm_.getState().getName();
	fsm_.resume(timeout, timestamp);
	if (external_request_status_)
	{
		std::string finalState = fsm_.getState().getName();
		SetMFModuleName(finalState);
		TLOG(TLVL_DEBUG)
			<< "States before and after a resume transition: "
			<< initialState << " and " << finalState << ". Transition Duration: " << TimeUtils::GetElapsedTime(start_time) << " s.";
	}
	if (metricMan) metricMan->sendMetric("DAQ Transition Time", TimeUtils::GetElapsedTime(start_time), "s", 4, artdaq::MetricMode::Accumulate);

	TLOG(TLVL_RESUME) << "Resume transition complete";
	return (external_request_status_);
}

bool artdaq::Commandable::shutdown(uint64_t timeout)
{
	std::lock_guard<std::mutex> lk(primary_mutex_);
	external_request_status_ = true;
	report_string_ = "All is OK.";
	SetMFModuleName("Shutting Down");

	TLOG(TLVL_SHUTDOWN) << "Shutdown transition started";
	auto start_time = std::chrono::steady_clock::now();
	std::string initialState = fsm_.getState().getName();
	fsm_.shutdown(timeout);
	if (external_request_status_)
	{
		std::string finalState = fsm_.getState().getName();
		SetMFModuleName(finalState);
		TLOG(TLVL_DEBUG)
			<< "States before and after a shutdown transition: "
			<< initialState << " and " << finalState << ". Transition Duration: " << TimeUtils::GetElapsedTime(start_time) << " s.";
	}
	if (metricMan) metricMan->sendMetric("DAQ Transition Time", TimeUtils::GetElapsedTime(start_time), "s", 4, artdaq::MetricMode::Accumulate);

	TLOG(TLVL_SHUTDOWN) << "Shutdown transition complete";
	return (external_request_status_);
}

bool artdaq::Commandable::soft_initialize(fhicl::ParameterSet const& pset, uint64_t timeout, uint64_t timestamp)
{
	std::lock_guard<std::mutex> lk(primary_mutex_);
	external_request_status_ = true;
	report_string_ = "All is OK.";

	SetMFModuleName("Soft_initializing");

	TLOG(TLVL_SOFT_INIT) << "Soft_initialize transition started";
	auto start_time = std::chrono::steady_clock::now();
	std::string initialState = fsm_.getState().getName();
	fsm_.soft_init(pset, timeout, timestamp);
	if (external_request_status_)
	{
		std::string finalState = fsm_.getState().getName();

		SetMFModuleName(finalState);
		TLOG(TLVL_DEBUG)
			<< "States before and after a soft_init transition: "
			<< initialState << " and " << finalState << ". Transition Duration: " << TimeUtils::GetElapsedTime(start_time) << " s.";
	}
	if (metricMan) metricMan->sendMetric("DAQ Transition Time", TimeUtils::GetElapsedTime(start_time), "s", 4, artdaq::MetricMode::Accumulate);

	TLOG(TLVL_SOFT_INIT) << "Soft_initialize transition complete";
	return (external_request_status_);
}

bool artdaq::Commandable::reinitialize(fhicl::ParameterSet const& pset, uint64_t timeout, uint64_t timestamp)
{
	std::lock_guard<std::mutex> lk(primary_mutex_);
	external_request_status_ = true;
	report_string_ = "All is OK.";

	SetMFModuleName("Reinitializing");

	TLOG(TLVL_REINIT) << "Reinitialize transition started";
	auto start_time = std::chrono::steady_clock::now();
	std::string initialState = fsm_.getState().getName();
	fsm_.reinit(pset, timeout, timestamp);
	if (external_request_status_)
	{
		std::string finalState = fsm_.getState().getName();

		SetMFModuleName(finalState);
		TLOG(TLVL_DEBUG)
			<< "States before and after a reinit transition: "
			<< initialState << " and " << finalState << ". Transition Duration: " << TimeUtils::GetElapsedTime(start_time) << " s.";
	}
	if (metricMan) metricMan->sendMetric("DAQ Transition Time", TimeUtils::GetElapsedTime(start_time), "s", 4, artdaq::MetricMode::Accumulate);

	TLOG(TLVL_REINIT) << "Reinitialize transition complete";
	return (external_request_status_);
}

bool artdaq::Commandable::in_run_failure()
{
	std::lock_guard<std::mutex> lk(primary_mutex_);
	external_request_status_ = true;
	report_string_ = "An error condition was reported while running.";

	SetMFModuleName("Failing");

	TLOG(TLVL_INRUN_FAILURE) << "In_Run_Failure transition started";
	auto start_time = std::chrono::steady_clock::now();
	std::string initialState = fsm_.getState().getName();
	fsm_.in_run_failure();
	if (external_request_status_)
	{
		std::string finalState = fsm_.getState().getName();

		SetMFModuleName(finalState);
		TLOG(TLVL_DEBUG)
			<< "States before and after an in_run_failure transition: "
			<< initialState << " and " << finalState << ". Transition Duration: " << TimeUtils::GetElapsedTime(start_time) << " s.";
	}
	if (metricMan) metricMan->sendMetric("DAQ Transition Time", TimeUtils::GetElapsedTime(start_time), "s", 4, artdaq::MetricMode::Accumulate);

	TLOG(TLVL_INRUN_FAILURE) << "in_run_failure complete";
	return (external_request_status_);
}

std::string artdaq::Commandable::status() const
{
	std::lock_guard<std::mutex> lk(primary_mutex_);
	TLOG(TLVL_STATUS) << "Status command started";
	std::string currentState = this->current_state();
	if (currentState == "InRunError")
	{
		return "Error";
	}
	TLOG(TLVL_STATUS) << "Status command complete";
	return currentState;
}

std::vector<std::string> artdaq::Commandable::legal_commands() const
{
	std::lock_guard<std::mutex> lk(primary_mutex_);
	TLOG(TLVL_LEGAL_COMMANDS) << "legal_commands started";
	std::string currentState = this->current_state();

	if (currentState == "Ready")
	{
		return { "init", "soft_init", "start", "shutdown" };
	}
	if (currentState == "Running")
	{
		// 12-May-2015, KAB: in_run_failure is also possible, but it
		// shouldn't be requested externally.
		return { "pause", "stop" };
	}
	if (currentState == "Paused")
	{
		return { "resume", "stop" };
	}
	if (currentState == "InRunError")
	{
		return { "pause", "stop" };
	}

	// Booted and Error
	TLOG(TLVL_LEGAL_COMMANDS) << "legal_commands complete";
	return { "init", "shutdown" };
}

// *******************************************************************
// *** The following methods implement the state machine operations.
// *******************************************************************

bool artdaq::Commandable::do_initialize(fhicl::ParameterSet const&, uint64_t, uint64_t)
{
	TLOG(TLVL_DEBUG) << "do_initialize called.";
	external_request_status_ = true;
	return external_request_status_;
}

bool artdaq::Commandable::do_start(art::RunID, uint64_t, uint64_t)
{
	TLOG(TLVL_DEBUG) << "do_start called.";
	external_request_status_ = true;
	return external_request_status_;
}

bool artdaq::Commandable::do_stop(uint64_t, uint64_t)
{
	TLOG(TLVL_DEBUG) << "do_stop called.";
	external_request_status_ = true;
	return external_request_status_;
}

bool artdaq::Commandable::do_pause(uint64_t, uint64_t)
{
	TLOG(TLVL_DEBUG) << "do_pause called.";
	external_request_status_ = true;
	return external_request_status_;
}

bool artdaq::Commandable::do_resume(uint64_t, uint64_t)
{
	TLOG(TLVL_DEBUG) << "do_resume called.";
	external_request_status_ = true;
	return external_request_status_;
}

bool artdaq::Commandable::do_shutdown(uint64_t)
{
	TLOG(TLVL_DEBUG) << "do_shutdown called.";
	external_request_status_ = true;
	return external_request_status_;
}

bool artdaq::Commandable::do_reinitialize(fhicl::ParameterSet const&, uint64_t, uint64_t)
{
	TLOG(TLVL_DEBUG) << "do_reinitialize called.";
	external_request_status_ = true;
	return external_request_status_;
}

bool artdaq::Commandable::do_soft_initialize(fhicl::ParameterSet const&, uint64_t, uint64_t)
{
	TLOG(TLVL_DEBUG) << "do_soft_initialize called.";
	external_request_status_ = true;
	return external_request_status_;
}

void artdaq::Commandable::badTransition(const std::string& trans)
{
	std::string currentState = this->current_state();
	if (currentState == "InRunError")
	{
		currentState = "Error";
	}

	report_string_ = "An invalid transition (";
	report_string_.append(trans);
	report_string_.append(") was requested; transition not allowed from this process's current state of \"");
	report_string_.append(currentState);
	report_string_.append("\"");

	TLOG(TLVL_WARNING) << report_string_;

	external_request_status_ = false;
}

void artdaq::Commandable::BootedEnter()
{
	TLOG(TLVL_DEBUG) << "BootedEnter called.";
}

void artdaq::Commandable::InRunExit()
{
	TLOG(TLVL_DEBUG) << "InRunExit called.";
}


std::string artdaq::Commandable::do_trace_get(std::string const& name)
{
	TLOG(TLVL_DEBUG) << "Getting masks for name " << name;
	std::ostringstream ss;
	if (name == "ALL")
	{
		unsigned ii = 0;
		unsigned ee = traceControl_p->num_namLvlTblEnts;
		for (ii = 0; ii < ee; ++ii)
		{
			if (traceNamLvls_p[ii].name[0])
				ss << traceNamLvls_p[ii].name << " " << std::hex << std::showbase << traceNamLvls_p[ii].M << " " << traceNamLvls_p[ii].S << " " << traceNamLvls_p[ii].T << " " << std::endl;
		}
	}
	else
	{
		unsigned ii = 0;
		unsigned ee = traceControl_p->num_namLvlTblEnts;
		for (ii = 0; ii < ee; ++ii)
		{
			if (traceNamLvls_p[ii].name[0] && TMATCHCMP(name.c_str(), traceNamLvls_p[ii].name)) break;
		}
		if (ii == ee) return "";

		ss << std::hex << traceNamLvls_p[ii].M << " " << traceNamLvls_p[ii].S << " " << traceNamLvls_p[ii].T;
	}
	return ss.str();
}

bool artdaq::Commandable::do_trace_set(std::string const& type, std::string const& name, uint64_t mask)
{
	TLOG(TLVL_DEBUG) << "Setting msk " << type << " for name " << name << " to " << std::hex << std::showbase << mask;
	if (name != "ALL")
	{
		if (type.size() > 0)
		{
			switch (type[0])
			{
			case 'M':
				TRACE_CNTL("lvlmsknM", name.c_str(), mask);
				break;
			case 'S':
				TRACE_CNTL("lvlmsknS", name.c_str(), mask);
				break;
			case 'T':
				TRACE_CNTL("lvlmsknT", name.c_str(), mask);
				break;
			}
		}
		else
		{
			TLOG(TLVL_ERROR) << "Cannot set mask: no type specified!";
		}
	}
	else
	{
		if (type.size() > 0)
		{
			switch (type[0])
			{
			case 'M':
				TRACE_CNTL("lvlmskMg", mask);
				break;
			case 'S':
				TRACE_CNTL("lvlmskSg", mask);
				break;
			case 'T':
				TRACE_CNTL("lvlmskTg", mask);
				break;
			}
		}
		else
		{
			TLOG(TLVL_ERROR) << "Cannot set mask: no type specified!";
		}
	}
	return true;
}

bool artdaq::Commandable::do_meta_command(std::string const& cmd, std::string const& arg)
{
	TLOG(TLVL_DEBUG) << "Meta-Command called: cmd=" << cmd << ", arg=" << arg;
	return true;
}

bool artdaq::Commandable::do_rollover_subrun(uint64_t)
{
	TLOG(TLVL_DEBUG) << "do_rollover_subrun called.";
	external_request_status_ = true;
	return external_request_status_;
}

bool artdaq::Commandable::do_add_config_archive_entry(std::string const& key, std::string const& value)
{
	TLOG(TLVL_DEBUG) << "do_add_config_archive_entry called: key=" << key << ", value=" << value;
	return true;
}

bool artdaq::Commandable::do_clear_config_archive()
{
	TLOG(TLVL_DEBUG) << "do_clear_config_archive called.";
	external_request_status_ = true;
	return external_request_status_;
}

// *********************
// *** Utility methods. 
// *********************

std::string artdaq::Commandable::current_state() const
{
	std::string fullStateName = (const_cast<Commandable*>(this))->fsm_.getState().getName();
	size_t pos = fullStateName.rfind("::");
	if (pos != std::string::npos)
	{
		return fullStateName.substr(pos + 2);
	}
	else
	{
		return fullStateName;
	}
}
