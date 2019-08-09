#ifndef artdaq_Application_Commandable_hh
#define artdaq_Application_Commandable_hh

#include <string>
#include <vector>
#include <mutex>

#include "fhiclcpp/ParameterSet.h"
#include "canvas/Persistency/Provenance/RunID.h"
#include "artdaq/Application/Commandable_sm.h"  // must be included after others

namespace artdaq
{
	class Commandable;
}

/**
 * \brief Commandable is the base class for all artdaq components which implement the artdaq state machine
 */
class artdaq::Commandable
{
public:
	/**
	* Default constructor.
	*/
	Commandable();

	/**
	 * \brief Copy Constructor is deleted
	 */
	Commandable(Commandable const&) = delete;

	/**
	 * \brief Default Destructor
	 */
	virtual ~Commandable() = default;

	/**
	 * \brief Copy Assignment operator is deleted
	 * \return Commandable copy
	 */
	Commandable& operator=(Commandable const&) = delete;

	/**
	* \brief Processes the initialize request
	* \param pset ParameterSet used to configure the Commandable
	* \param timeout Timeout for init step
	* \param timestamp Timestamp of init step
	* \return Whether the transition was successful
	*/
	bool initialize(fhicl::ParameterSet const& pset, uint64_t timeout, uint64_t timestamp);

	/**
	 * \brief Processes the start transition
	 * \param id Run number of new run
	 * \param timeout Timeout for transition
	 * \param timestamp Timestamp of transition
	 * \return Whether the transition was successful
	 */
	bool start(art::RunID id, uint64_t timeout, uint64_t timestamp);

	/**
	* \brief Processes the stop transition
	* \param timeout Timeout for transition
	* \param timestamp Timestamp of transition
	* \return Whether the transition was successful
	*/
	bool stop(uint64_t timeout, uint64_t timestamp);

	/**
	* \brief Processes the pause transition
	* \param timeout Timeout for transition
	* \param timestamp Timestamp of transition
	* \return Whether the transition was successful
	*/
	bool pause(uint64_t timeout, uint64_t timestamp);

	/**
	* \brief Processes the resume transition
	* \param timeout Timeout for transition
	* \param timestamp Timestamp of transition
	* \return Whether the transition was successful
	*/
	bool resume(uint64_t timeout, uint64_t timestamp);

	/**
	* \brief Processes the shutdown transition
	* \param timeout Timeout for transition
	* \return Whether the transition was successful
	*/
	bool shutdown(uint64_t timeout);
	
	/**
	* \brief Processes the soft-initialize request
	* \param pset ParameterSet used to configure the Commandable
	* \param timeout Timeout for init step
	* \param timestamp Timestamp of init step
	* \return Whether the transition was successful
	*/
	bool soft_initialize(fhicl::ParameterSet const& pset, uint64_t timeout, uint64_t timestamp);

	/**
	* \brief Processes the reinitialize request
	* \param pset ParameterSet used to configure the Commandable
	* \param timeout Timeout for init step
	* \param timestamp Timestamp of init step
	* \return Whether the transition was successful
	*/
	bool reinitialize(fhicl::ParameterSet const& pset, uint64_t timeout, uint64_t timestamp);

	/**
	 * \brief Actions taken when the in_run_failure state is set
	 * \return Whether the notification was properly processed
	 */
	bool in_run_failure();

	/**
	 * \brief Default report implementation returns current report_string
	 * \return Current report_string (may be set by derived classes)
	 */
	virtual std::string report(std::string const&) const
	{
		std::lock_guard<std::mutex> lk(primary_mutex_);
		return report_string_;
	}

	/**
	 * \brief Returns the current state of the Commandable
	 * \return The current state of the Commandable
	 */
	std::string status() const;

	/**
	* \brief Perform the register_monitor action.
	* \return A report on the status of the command
	*
	* This function is a No-Op. Derived classes should override it.
	*/
	virtual std::string register_monitor(fhicl::ParameterSet const&)
	{
		return "This string is returned from Commandable::register_monitor; register_monitor should either be overridden in a derived class or this process should not have been sent the register_monitor call";
	}

	/**
	* \brief Perform the unregister_monitor action.
	* \return A report on the status of the command
	*
	* This function is a No-Op. Derived classes should override it.
	*/
	virtual std::string unregister_monitor(std::string const&)
	{
		return "This string is returned from Commandable::unregister_monitor; unregister_monitor should either be overridden in a derived class or this process should not have been sent the unregister_monitor call";
	}

	/**
	 * \brief Get the legal transition commands from the current state
	 * \return A list of legal transitions from the current state
	 */
	std::vector<std::string> legal_commands() const;

	// these methods provide the operations that are used by the state machine
	/**
	 * \brief Perform the initialize transition.
	 * \return Whether the transition succeeded
	 * 
	 * This function is a No-Op. Derived classes should override it.
	 */
	virtual bool do_initialize(fhicl::ParameterSet const&, uint64_t, uint64_t);

	/**
	* \brief Perform the start transition.
	* \return Whether the transition succeeded
	*
	* This function is a No-Op. Derived classes should override it.
	*/
	virtual bool do_start(art::RunID, uint64_t, uint64_t);

	/**
	* \brief Perform the stop transition.
	* \return Whether the transition succeeded
	*
	* This function is a No-Op. Derived classes should override it.
	*/
	virtual bool do_stop(uint64_t, uint64_t);

	/**
	* \brief Perform the pause transition.
	* \return Whether the transition succeeded
	*
	* This function is a No-Op. Derived classes should override it.
	*/
	virtual bool do_pause(uint64_t, uint64_t);

	/**
	* \brief Perform the resume transition.
	* \return Whether the transition succeeded
	*
	* This function is a No-Op. Derived classes should override it.
	*/
	virtual bool do_resume(uint64_t, uint64_t);

	/**
	* \brief Perform the shutdown transition.
	* \return Whether the transition succeeded
	*
	* This function is a No-Op. Derived classes should override it.
	*/
	virtual bool do_shutdown(uint64_t);

	/**
	* \brief Perform the reinitialize transition.
	* \return Whether the transition succeeded
	*
	* This function is a No-Op. Derived classes should override it.
	*/
	virtual bool do_reinitialize(fhicl::ParameterSet const&, uint64_t, uint64_t);

	/**
	* \brief Perform the soft_initialize transition.
	* \return Whether the transition succeeded
	*
	* This function is a No-Op. Derived classes should override it.
	*/
	virtual bool do_soft_initialize(fhicl::ParameterSet const&, uint64_t, uint64_t);

	/**
	* \brief Perform the rollover_subrun transition.
	* \param eventNum Sequence ID of boundary
	* \return Whether the transition succeeded
	*/
	virtual bool do_rollover_subrun(uint64_t eventNum);
	
	/**
	 * \brief This function is called when an attempt is made to call an illegal transition
	 * \param trans The transition that was attempted
	 */
	virtual void badTransition(const std::string& trans);

	/**
	 * \brief Perform actions upon entering the Booted state
	 * 
	 * This function is a No-Op. Derived classes should override it.
	 */
	virtual void BootedEnter();

	/**
	* \brief Perform actions upon leaving the InRun state
	*
	* This function is a No-Op. Derived classes should override it.
	*/
	virtual void InRunExit();

	/**
	* \brief Get the TRACE mask for the given TRACE name
	* If name is "ALL", then all TRACE masks will be printed
	*
	* This function is implemented in Commandable, derived classes may override if necessary.
	* \param name TRACE name to print mask for. "ALL" prints all TRACE masks
	* \return TRACE mask of the given TRACE name
	*/
	virtual std::string do_trace_get(std::string const& name);

	/**
	* \brief Set the given TRACE mask for the given TRACE name
	*
	* This function is implemented in Commandable, derived classes may override if necessary.
	* \param type Type of TRACE mask to set (either M, S, or T)
	* \param name Name of the TRACE level to set mask for
	* \param mask Mask to set
	* \return Whether the command succeeded (always true)
	*/
	virtual bool do_trace_set(std::string const& type, std::string const& name, uint64_t mask);

	/**
	* \brief Run a module-defined command with the given parameter string
	*
	* This function is a No-Op. Derived classes should override it.
	* \param command Name of the command to run (implementation-defined)
	* \param args Any arguments for the command (implementation-defined)
	* \return Whether the command succeeded (always true)
	*/
	virtual bool do_meta_command(std::string const& command, std::string const& args);

	/**
	* \brief Add the specified key-value pair to the configuration archive list
	*
	* This function is a No-Op. Derived classes should override it.
	*/
	virtual bool do_add_config_archive_entry(std::string const&, std::string const&);

	/**
	* \brief Clears the configuration archive list
	*
	* This function is a No-Op. Derived classes should override it.
	*/
	virtual bool do_clear_config_archive();

protected:
	/**
	 * \brief Return the name of the current state
	 * \return The name of the current state
	 */
	std::string current_state() const;

	CommandableContext fsm_; ///< The generated State Machine (using smc_compiler)
	bool external_request_status_; ///< Whether the last command succeeded
	std::string report_string_; ///< Status information about the last command

private:
	// 06-May-2015, KAB: added a mutex to be used in avoiding problems when
	// requests are sent to a Commandable object from different threads. The
	// reason that we're doing this now is that we've added the in_run_failure()
	// transition that will generally be called from inside the Application.
	// Prior to this, the only way that transitions were requested was via
	// external XMLRPC commands, and those were presumed to be called one
	// at a time. The use of scoped locks based on the mutex will prevent
	// the in_run_failure() transition from being called at the same time as
	// an externally requested transition. We only lock the methods that
	// are externally called.
	mutable std::mutex primary_mutex_;
};

#endif /* artdaq_Application_Commandable_hh */
