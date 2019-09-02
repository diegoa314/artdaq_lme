#ifndef artdaq_Application_MPI2_DispatcherApp_hh
#define artdaq_Application_MPI2_DispatcherApp_hh

#include <future>

#include "artdaq/Application/DispatcherCore.hh"
#include "artdaq/Application/Commandable.hh"

namespace artdaq
{
	class DispatcherApp;
}

/**
 * \brief DispatcherApp is an artdaq::Commandable derived class which controls the DispatcherCore
 */
class artdaq::DispatcherApp : public artdaq::Commandable
{
public:
	/**
	 * \brief DispatcherApp Constructor
	 */
	DispatcherApp();

	/**
	 * \brief Copy Constructor is Deleted
	 */
	DispatcherApp(DispatcherApp const&) = delete;

	/**
	 * \brief Default virtual destructor
	 */
	virtual ~DispatcherApp() = default;

	/**
	 * \brief Copy Assignment operator is Deleted
	 * \return DispatcherApp copy
	 */
	DispatcherApp& operator=(DispatcherApp const&) = delete;

	// these methods provide the operations that are used by the state machine
	/**
	 * \brief Initialize the DispatcherCore
	 * \param pset ParameterSet used to initialize the DispatcherCore
	 * \return Whether the initialize transition succeeded
	 */
	bool do_initialize(fhicl::ParameterSet const& pset, uint64_t, uint64_t) override;

	/**
	 * \brief Start the DispatcherCore
	 * \param id Run number of the new run
	 * \return Whether the start transition succeeded
	 */
	bool do_start(art::RunID id, uint64_t, uint64_t) override;

	/**
	 * \brief Stop the DispatcherCore
	 * \return Whether the stop transition succeeded
	 */
	bool do_stop(uint64_t, uint64_t) override;

	/**
	* \brief Pause the DispatcherCore
	* \return Whether the pause transition succeeded
	*/
	bool do_pause(uint64_t, uint64_t) override;

	/**
	* \brief Resume the DispatcherCore
	* \return Whether the resume transition succeeded
	*/
	bool do_resume(uint64_t, uint64_t) override;

	/**
	* \brief Shutdown the DispatcherCore
	* \return Whether the shutdown transition succeeded
	*/
	bool do_shutdown(uint64_t) override;

	/**
	 * \brief Soft-initialize the DispatcherCore. No-Op
	 * \return This function always returns true
	 */
	bool do_soft_initialize(fhicl::ParameterSet const&, uint64_t, uint64_t) override;

	/**
	* \brief Reinitialize the DispatcherCore. No-Op
	* \return This function always returns true
	*/
	bool do_reinitialize(fhicl::ParameterSet const&, uint64_t, uint64_t) override;

	/**
	 * \brief If which is "transition_status", report the status of the last transition. Otherwise pass through to DispatcherCore
	 * \param which What to report on
	 * \return Report string. Empty for unknown "which" parameter
	 */
	std::string report(std::string const& which) const override;

	/**
	 * \brief Register an art Online Monitor to the DispatcherCore
	 * \param info ParameterSet containing information about the monitor
	 * \return String detailing result status
	 */
	std::string register_monitor(fhicl::ParameterSet const& info) override;

	/**
	 * \brief Remove an art Online Monitor from the DispatcherCore
	 * \param label Name of the monitor to remove
	 * \return String detailing result status
	 */
	std::string unregister_monitor(std::string const& label) override;

private:
	std::unique_ptr<DispatcherCore> Dispatcher_ptr_;
};

#endif
