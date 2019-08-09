#ifndef artdaq_Application_MPI2_BoardReaderApp_hh
#define artdaq_Application_MPI2_BoardReaderApp_hh

#include <future>

#include "artdaq/Application/Commandable.hh"
#include "artdaq/Application/BoardReaderCore.hh"

namespace artdaq
{
	class BoardReaderApp;
}

/**
 * \brief BoardReaderApp is an artdaq::Commandable derived class which controls the BoardReaderCore state machine
 */
class artdaq::BoardReaderApp : public artdaq::Commandable
{
public:
	/**
	 * \brief BoardReaderApp Constructor
	 */
	BoardReaderApp();

	/**
	 * \brief Copy Constructor is deleted
	 */
	BoardReaderApp(BoardReaderApp const&) = delete;

	/**
	 * \brief Default Destructor
	 */
	virtual ~BoardReaderApp() = default;

	/**
	 * \brief Copy Assignment Operator is deleted
	 * \return BoardReaderApp copy
	 */
	BoardReaderApp& operator=(BoardReaderApp const&) = delete;

	// these methods provide the operations that are used by the state machine
	/**
	 * \brief Initialize the BoardReaderCore
	 * \param pset ParameterSet used to configure the BoardReaderCore
	 * \param timeout Timeout for transition
	 * \param timestamp Timestamp of transition
	 * \return Whether the transition succeeded
	 */
	bool do_initialize(fhicl::ParameterSet const& pset, uint64_t timeout, uint64_t timestamp) override;

	/**
	 * \brief Start the BoardReaderCore
	 * \param id Run ID of new run
	 * \param timeout Timeout for transition
	 * \param timestamp Timestamp of transition
	 * \return Whether the transition succeeded
	 */
	bool do_start(art::RunID id, uint64_t timeout, uint64_t timestamp) override;

	/**
	 * \brief Stop the BoardReaderCore
	 * \param timeout Timeout for transition
	 * \param timestamp Timestamp of transition
	 * \return Whether the transition succeeded
	 */
	bool do_stop(uint64_t timeout, uint64_t timestamp) override;

	/**
	* \brief Pause the BoardReaderCore
	* \param timeout Timeout for transition
	* \param timestamp Timestamp of transition
	* \return Whether the transition succeeded
	*/
	bool do_pause(uint64_t timeout, uint64_t timestamp) override;

	/**
	* \brief Resume the BoardReaderCore
	* \param timeout Timeout for transition
	* \param timestamp Timestamp of transition
	* \return Whether the transition succeeded
	*/
	bool do_resume(uint64_t timeout, uint64_t timestamp) override;

	/**
	* \brief Shutdown the BoardReaderCore
	* \param timeout Timeout for transition
	* \return Whether the transition succeeded
	*/
	bool do_shutdown(uint64_t timeout) override;

	/**
	* \brief Soft-Initialize the BoardReaderCore
	* \param pset ParameterSet used to configure the BoardReaderCore
	* \param timeout Timeout for transition
	* \param timestamp Timestamp of transition
	* \return Whether the transition succeeded
	*/
	bool do_soft_initialize(fhicl::ParameterSet const& pset, uint64_t timeout, uint64_t timestamp) override;

	/**
	* \brief Reinitialize the BoardReaderCore
	* \param pset ParameterSet used to configure the BoardReaderCore
	* \param timeout Timeout for transition
	* \param timestamp Timestamp of transition
	* \return Whether the transition succeeded
	*/
	bool do_reinitialize(fhicl::ParameterSet const& pset, uint64_t timeout, uint64_t timestamp) override;

	/**
	 * \brief Action taken upon entering the "Booted" state
	 * 
	 * This resets the BoardReaderCore pointer
	 */
	void BootedEnter() override;

	/**
	* \brief Perform a user-defined command (passed to CommandableFragmentGenerator)
	* \param command Name of the command
	* \param arg Argument for the command
	* \return Whether the command succeeded
	*/
	bool do_meta_command(std::string const& command, std::string const& arg) override;

	/* Report_ptr */
	/**
	 * \brief If which is "transition_status", report the status of the last transition. Otherwise pass through to BoardReaderCore
	 * \param which What to report on
	 * \return Report string. Empty for unknown "which" parameter
	 */
	std::string report(std::string const& which) const override;

private:
	std::unique_ptr<artdaq::BoardReaderCore> fragment_receiver_ptr_;
	boost::thread fragment_processing_thread_;
};

#endif /* artdaq_Application_MPI2_BoardReaderApp_hh */
