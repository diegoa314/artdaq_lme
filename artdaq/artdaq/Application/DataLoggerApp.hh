#ifndef artdaq_Application_MPI2_DataLoggerApp_hh
#define artdaq_Application_MPI2_DataLoggerApp_hh

#include <future>

#include "artdaq/Application/DataLoggerCore.hh"
#include "artdaq/Application/Commandable.hh"

namespace artdaq
{
	class DataLoggerApp;
}

/**
 * \brief DataLoggerApp is an artdaq::Commandable derived class which controls the DataLoggerCore
 */
class artdaq::DataLoggerApp : public artdaq::Commandable
{
public:
	/**
	 * \brief DataLoggerApp Constructor
	 */
	DataLoggerApp();

	/**
	 * \brief Copy Constructor is Deleted
	 */
	DataLoggerApp(DataLoggerApp const&) = delete;

	/**
	 * \brief Default virtual destructor
	 */
	virtual ~DataLoggerApp() = default;

	/**
	 * \brief Copy Assignment operator is Deleted
	 * \return DataLoggerApp copy
	 */
	DataLoggerApp& operator=(DataLoggerApp const&) = delete;

	// these methods provide the operations that are used by the state machine
	/**
	 * \brief Initialize the DataLoggerCore
	 * \param pset ParameterSet used to initialize the DataLoggerCore
	 * \return Whether the initialize transition succeeded
	 */
	bool do_initialize(fhicl::ParameterSet const& pset, uint64_t, uint64_t) override;

	/**
	 * \brief Start the DataLoggerCore
	 * \param id Run number of the new run
	 * \return Whether the start transition succeeded
	 */
	bool do_start(art::RunID id, uint64_t, uint64_t) override;

	/**
	 * \brief Stop the DataLoggerCore
	 * \return Whether the stop transition succeeded
	 */
	bool do_stop(uint64_t, uint64_t) override;

	/**
	* \brief Pause the DataLoggerCore
	* \return Whether the pause transition succeeded
	*/
	bool do_pause(uint64_t, uint64_t) override;

	/**
	* \brief Resume the DataLoggerCore
	* \return Whether the resume transition succeeded
	*/
	bool do_resume(uint64_t, uint64_t) override;

	/**
	* \brief Shutdown the DataLoggerCore
	* \return Whether the shutdown transition succeeded
	*/
	bool do_shutdown(uint64_t) override;

	/**
	 * \brief Soft-initialize the DataLoggerCore. No-Op
	 * \return This function always returns true
	 */
	bool do_soft_initialize(fhicl::ParameterSet const&, uint64_t, uint64_t) override;

	/**
	* \brief Reinitialize the DataLoggerCore. No-Op
	* \return This function always returns true
	*/
	bool do_reinitialize(fhicl::ParameterSet const&, uint64_t, uint64_t) override;

	/**
	 * \brief If which is "transition_status", report the status of the last transition. Otherwise pass through to DataLoggerCore
	 * \param which What to report on
	 * \return Report string. Empty for unknown "which" parameter
	 */
	std::string report(std::string const& which) const override;

	/**
	* \brief Add the specified configuration archive entry to the DataLoggerCore
	* \return Whether the command succeeded
	*/
	bool do_add_config_archive_entry(std::string const&, std::string const&) override;

	/**
	* \brief Clear the configuration archive list in the DataLoggerCore
	* \return Whether the command succeeded
	*/
	bool do_clear_config_archive() override;


private:
	std::unique_ptr<DataLoggerCore> DataLogger_ptr_;
};

#endif
