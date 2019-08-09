#ifndef artdaq_Application_MPI2_RoutingMasterApp_hh
#define artdaq_Application_MPI2_RoutingMasterApp_hh

#include "artdaq/Application/Commandable.hh"
#include "artdaq/Application/RoutingMasterCore.hh"

namespace artdaq
{
	class RoutingMasterApp;
}

/**
* \brief RoutingMasterApp is an artdaq::Commandable derived class which controls the RoutingMasterCore state machine
*/
class artdaq::RoutingMasterApp : public artdaq::Commandable
{
public:
	/**
	* \brief RoutingMasterApp Constructor
	*/
	RoutingMasterApp();

	/**
	* \brief Copy Constructor is deleted
	*/
	RoutingMasterApp(RoutingMasterApp const&) = delete;

	/**
	* \brief Default Destructor
	*/
	virtual ~RoutingMasterApp() = default;

	/**
	* \brief Copy Assignment Operator is deleted
	* \return RoutingMasterApp copy
	*/
	RoutingMasterApp& operator=(RoutingMasterApp const&) = delete;

	// these methods provide the operations that are used by the state machine
	/**
	* \brief Initialize the RoutingMasterCore
	* \param pset ParameterSet used to configure the RoutingMasterCore
	* \param timeout Timeout for transition
	* \param timestamp Timestamp of transition
	* \return Whether the transition succeeded
	*/
	bool do_initialize(fhicl::ParameterSet const& pset, uint64_t timeout, uint64_t timestamp) override;

	/**
	* \brief Start the RoutingMasterCore
	* \param id Run ID of new run
	* \param timeout Timeout for transition
	* \param timestamp Timestamp of transition
	* \return Whether the transition succeeded
	*/
	bool do_start(art::RunID id, uint64_t timeout, uint64_t timestamp) override;

	/**
	* \brief Stop the RoutingMasterCore
	* \param timeout Timeout for transition
	* \param timestamp Timestamp of transition
	* \return Whether the transition succeeded
	*/
	bool do_stop(uint64_t timeout, uint64_t timestamp) override;

	/**
	* \brief Pause the RoutingMasterCore
	* \param timeout Timeout for transition
	* \param timestamp Timestamp of transition
	* \return Whether the transition succeeded
	*/
	bool do_pause(uint64_t timeout, uint64_t timestamp) override;

	/**
	* \brief Resume the RoutingMasterCore
	* \param timeout Timeout for transition
	* \param timestamp Timestamp of transition
	* \return Whether the transition succeeded
	*/
	bool do_resume(uint64_t timeout, uint64_t timestamp) override;

	/**
	* \brief Shutdown the RoutingMasterCore
	* \param timeout Timeout for transition
	* \return Whether the transition succeeded
	*/
	bool do_shutdown(uint64_t timeout) override;

	/**
	* \brief Soft-Initialize the RoutingMasterCore
	* \param pset ParameterSet used to configure the RoutingMasterCore
	* \param timeout Timeout for transition
	* \param timestamp Timestamp of transition
	* \return Whether the transition succeeded
	*/
	bool do_soft_initialize(fhicl::ParameterSet const& pset, uint64_t timeout, uint64_t timestamp) override;

	/**
	* \brief Reinitialize the RoutingMasterCore
	* \param pset ParameterSet used to configure the RoutingMasterCore
	* \param timeout Timeout for transition
	* \param timestamp Timestamp of transition
	* \return Whether the transition succeeded
	*/
	bool do_reinitialize(fhicl::ParameterSet const& pset, uint64_t timeout, uint64_t timestamp) override;

	/**
	* \brief Action taken upon entering the "Booted" state
	*
	* This resets the RoutingMasterCore pointer
	*/
	void BootedEnter() override;

	/* Report_ptr */
	/**
	 * \brief If which is "transition_status", report the status of the last transition. Otherwise pass through to AggregatorCore
	 * \param which What to report on
	 * \return Report string. Empty for unknown "which" parameter
	 */
	std::string report(std::string const&) const override;

private:
	std::unique_ptr<artdaq::RoutingMasterCore> routing_master_ptr_;
	boost::thread routing_master_thread_;
};

#endif /* artdaq_Application_MPI2_RoutingMasterApp_hh */
