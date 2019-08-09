/* DarkSide 50 DAQ program
 * This file add the xmlrpc commander as a client to the SC
 * Author: Alessandro Razeto <Alessandro.Razeto@ge.infn.it>
 */
#ifndef artdaq_ExternalComms_xmlrpc_commander_hh
#define artdaq_ExternalComms_xmlrpc_commander_hh

#include <mutex>
#include "artdaq/ExternalComms/CommanderInterface.hh"

namespace artdaq
{

/**
 * \brief The xmlrpc_commander class serves as the XMLRPC server run in each artdaq application
 */
class xmlrpc_commander : public CommanderInterface
{
public:
	/**
	 * \brief xmlrpc_commander Constructor
	 * \param ps ParameterSet used for configuring xmlrpc_commander
	 * \param commandable artdaq::Commandable object to send transition commands to
	 *
	 * \verbatim
	  xmlrpc_commander accepts the following Parameters:
	   id: For XMLRPC, the ID should be the port to listen on
	   server_url: When sending, location of XMLRPC server
	 * \endverbatim
	 */
	xmlrpc_commander(fhicl::ParameterSet ps, artdaq::Commandable& commandable);

	/**
	 * \brief Run the XMLRPC server
	 */
	void run_server() override;

	/// <summary>
	/// Send a register_monitor command over XMLRPC
	/// </summary>
	/// <param name="monitor_fhicl">FHiCL string contianing monitor configuration</param>
	/// <returns>Return status from XMLRPC</returns>
	std::string send_register_monitor(std::string monitor_fhicl) override;

	/// <summary>
	/// Send an unregister_monitor command over XMLRPC
	/// </summary>
	/// <param name="monitor_label">Label of the monitor to unregister</param>
	/// <returns>Return status from XMLRPC</returns>
	std::string send_unregister_monitor(std::string monitor_label) override;
	
	/// <summary>
	/// Send an init command over XMLRPC
	/// 
	/// The init command is accepted by all artdaq processes that are in the booted state.
	/// It expects a ParameterSet for configuration, a timeout, and a timestamp.
	/// </summary>
	/// <returns>Command result: "SUCCESS" if succeeded</returns>
	std::string send_init(fhicl::ParameterSet, uint64_t, uint64_t) override;

	/// <summary>
	/// Send a soft_init command over XMLRPC
	/// 
	/// The soft_init command is accepted by all artdaq processes that are in the booted state.
	/// It expects a ParameterSet for configuration, a timeout, and a timestamp.
	/// </summary>
	/// <returns>Command result: "SUCCESS" if succeeded</returns>
	std::string send_soft_init(fhicl::ParameterSet, uint64_t, uint64_t) override;

	/// <summary>
	/// Send a reinit command over XMLRPC
	/// 
	/// The reinit command is accepted by all artdaq processes.
	/// It expects a ParameterSet for configuration, a timeout, and a timestamp.
	/// </summary>
	/// <returns>Command result: "SUCCESS" if succeeded</returns>
	std::string send_reinit(fhicl::ParameterSet, uint64_t, uint64_t) override;

	/// <summary>
	/// Send a start command over XMLRPC
	/// 
	/// The start command starts a Run using the given run number.
	/// This command also accepts a timeout parameter and a timestamp parameter.
	/// </summary>
	/// <returns>Command result: "SUCCESS" if succeeded</returns>
	std::string send_start(art::RunID, uint64_t, uint64_t) override;

	/// <summary>
	/// Send a pause command over XMLRPC
	/// 
	/// The pause command pauses a Run. When the run resumes, the subrun number will be incremented.
	/// This command accepts a timeout parameter and a timestamp parameter.
	/// </summary>
	/// <returns>Command result: "SUCCESS" if succeeded</returns>
	std::string send_pause(uint64_t, uint64_t) override;

	/// <summary>
	/// Send a resume command over XMLRPC
	/// 
	/// The resume command resumes a paused Run. When the run resumes, the subrun number will be incremented.
	/// This command accepts a timeout parameter and a timestamp parameter.
	/// </summary>
	/// <returns>Command result: "SUCCESS" if succeeded</returns>
	std::string send_resume(uint64_t, uint64_t) override;

	/// <summary>
	/// Send a stop command over XMLRPC
	/// 
	/// The stop command stops the current Run.
	/// This command accepts a timeout parameter and a timestamp parameter.
	/// </summary>
	/// <returns>Command result: "SUCCESS" if succeeded</returns>
	std::string send_stop(uint64_t, uint64_t) override;

	/// <summary>
	/// Send a shutdown command over XMLRPC
	/// 
	/// The shutdown command shuts down the artdaq process.
	/// This command accepts a timeout parameter.
	/// </summary>
	/// <returns>Command result: "SUCCESS" if succeeded</returns>
	std::string send_shutdown(uint64_t) override;

	/// <summary>
	/// Send a status command over XMLRPC
	/// 
	/// The status command returns the current status of the artdaq process.
	/// </summary>
	/// <returns>Command result: current status of the artdaq process</returns>
	std::string send_status() override;

	/// <summary>
	/// Send a report command over XMLRPC
	/// 
	/// The report command returns the current value of the requested reportable quantity.
	/// </summary>
	/// <returns>Command result: current value of the requested reportable quantity</returns>
	std::string send_report(std::string) override;

	/// <summary>
	/// Send a legal_commands command over XMLRPC
	/// 
	/// This will query the artdaq process, and it will return the list of allowed transition commands from its current state.
	/// </summary>
	/// <returns>Command result: a list of allowed transition commands from its current state</returns>
	std::string send_legal_commands() override;
	
	/// <summary>
	/// Send an send_trace_get command over XMLRPC
	/// 
	/// This will cause the receiver to get the TRACE level masks for the given name
	/// Use name == "ALL" to get ALL names
	/// </summary>
	/// <returns>Command result: "SUCCESS" if succeeded</returns>
	std::string send_trace_get(std::string) override;

	/// <summary>
	/// Send an send_trace_msgfacility_set command over XMLRPC
	/// 
	/// This will cause the receiver to set the given TRACE level mask for the given name to the given mask.
	/// Only the first character of the mask selection will be parsed, dial 'M' for Memory, or 'S' for Slow.
	/// Use name == "ALL" to set ALL names
	///
	/// EXAMPLE: xmlrpc http://localhost:5235/RPC2 daq.trace_msgfacility_set TraceLock i/$((0x1234)) # Use Bash to convert hex to dec
	/// </summary>
	/// <returns>Command result: "SUCCESS" if succeeded</returns>
	std::string send_trace_set(std::string, std::string, uint64_t) override;

	/// <summary>
	/// Send an send_meta_command command over XMLRPC
	/// 
	/// This will cause the receiver to run the given command with the given argument in user code
	/// </summary>
	/// <returns>Command result: "SUCCESS" if succeeded</returns>
	std::string send_meta_command(std::string, std::string) override;

	/// <summary>
	/// Send a send_rollover_subrun command over XMLRPC
	///
	/// This will cause the receiver to rollover the subrun number at the given event. (Event with seqID == boundary will be in new subrun.)
	/// Should be sent to all EventBuilders before the given event is processed.
	/// </summary>
	/// <returns>Command result: "SUCCESS" if succeeded</returns>
	std::string send_rollover_subrun(uint64_t) override;


private:
	xmlrpc_commander(const xmlrpc_commander&) = delete;

	xmlrpc_commander(xmlrpc_commander&&) = delete;

	int port_;
	std::string serverUrl_;

	std::string send_command_(std::string command);
	std::string send_command_(std::string command, std::string arg);
	std::string send_command_(std::string command, fhicl::ParameterSet pset, uint64_t a, uint64_t b);
	std::string send_command_(std::string command, uint64_t a, uint64_t b);
	std::string send_command_(std::string command, art::RunID r, uint64_t a, uint64_t b);
	std::string send_command_(std::string, uint64_t);
	std::string send_command_(std::string, std::string, std::string);
	std::string send_command_(std::string, std::string, std::string, uint64_t);

public:
	std::timed_mutex mutex_; ///< XMLRPC mutex
	std::unique_ptr<xmlrpc_c::serverAbyss> server; ///< XMLRPC server
};

}

#endif /* artdaq_ExternalComms_xmlrpc_commander_hh */
