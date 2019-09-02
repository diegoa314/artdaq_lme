#ifndef artdaq_Application_MPI2_DispatcherCore_hh
#define artdaq_Application_MPI2_DispatcherCore_hh

#include <string>

#include "fhiclcpp/ParameterSet.h"
#include "canvas/Persistency/Provenance/RunID.h"

#include "artdaq/Application/DataReceiverCore.hh"


namespace artdaq
{
	class DispatcherCore;
}

/**
 * \brief DispatcherCore implements the state machine for the Dispatcher artdaq application.
 * DispatcherCore processes incoming events in one of three roles: Data Logger, Online Monitor, or Dispatcher.
 */
class artdaq::DispatcherCore : public DataReceiverCore
{
public:

	/**
	* \brief DispatcherCore Constructor.
	*/
	DispatcherCore();

	/**
	 * \brief Copy Constructor is deleted
	 */
	DispatcherCore(DispatcherCore const&) = delete;

	/**
	* Destructor.
	*/
	~DispatcherCore();

	/**
	 * \brief Copy Assignment operator is deleted
	 * \return DispatcherCore copy
	 */
	DispatcherCore& operator=(DispatcherCore const&) = delete;

	/**
	* \brief Processes the initialize request.
	* \param pset ParameterSet used to configure the DispatcherCore
	* \return Whether the initialize attempt succeeded
	*
	* \verbatim
	* DispatcherCore accepts the following Parameters:
	* "daq" (REQUIRED): FHiCL table containing DAQ configuration
	*   "Dispatcher" (REQUIRED): FHiCL table containing Dispatcher paramters
	*     "expected_events_per_bunch" (REQUIRED): Number of events to collect before sending them to art
	*     "enq_timeout" (Default: 5.0): Maximum amount of time to wait while enqueueing events to the ConcurrentQueue
	*     "is_data_logger": True if the Dispatcher is a Data Logger
	*     "is_online_monitor": True if the Dispatcher is an Online Monitor. is_data_logger takes precedence
	*     "is_dispatcher": True if the Dispatcher is a Dispatcher. is_data_logger and is_online_monitor take precedence
	*       NOTE: At least ONE of these three parameters must be specified.
	*     "inrun_recv_timeout_usec" (Default: 100000): Amount of time to wait for new events while running
	*     "endrun_recv_timeout_usec" (Default: 20000000): Amount of time to wait for additional events at EndOfRun
	*     "pause_recv_timeout_usec" (Default: 3000000): Amount of time to wait for additional events at PauseRun
	*     "onmon_event_prescale" (Default: 1): Only send 1/N events to art for online monitoring (requires is_data_logger: true)
	*     "verbose" (Default: true): Whether to print transition messages
	*   "metrics": FHiCL table containing configuration for MetricManager
	* "outputs" (REQUIRED): FHiCL table containing output parameters
	*   "normalOutput" (REQUIRED): FHiCL table containing default output parameters
	*     "fileName" (Default: ""): Name template of the output file. Used to determine output directory
	* \endverbatim
	*  Note that the "Dispatcher" ParameterSet is also used to configure the EventStore. See that class' documentation for more information.
	*/
	bool initialize(fhicl::ParameterSet const& pset) override;

	/**
	 * \brief Create a new TransferInterface instance using the given configuration
	 * \param pset ParameterSet used to configure the TransferInterface
	 * \return String detailing any errors encountered or "Success"
	 *
	 * See TransferInterface for details on the expected configuration
	 */
	std::string register_monitor(fhicl::ParameterSet const& pset);

	/**
	 * \brief Delete the TransferInterface having the given unique label
	 * \param label Label of the TransferInterface to delete
	 * \return String detailing any errors encountered or "Success"
	 */
	std::string unregister_monitor(std::string const& label);


private:
	fhicl::ParameterSet generate_filter_fhicl_(); 
	fhicl::ParameterSet merge_parameter_sets_(fhicl::ParameterSet skel, std::string label, fhicl::ParameterSet pset);
	void check_filters_();

	std::mutex dispatcher_transfers_mutex_;
	std::unordered_map<std::string, fhicl::ParameterSet> registered_monitors_;
	std::unordered_map<std::string, pid_t> registered_monitor_pids_;
	fhicl::ParameterSet pset_; // The ParameterSet initially passed to the Dispatcher (contains input info)
	bool broadcast_mode_;
};

#endif

//  LocalWords:  ds
