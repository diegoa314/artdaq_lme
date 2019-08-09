#ifndef artdaq_Application_MPI2_StatisticsHelper_hh
#define artdaq_Application_MPI2_StatisticsHelper_hh

#include "artdaq-core/Core/StatisticsCollection.hh"
#include "fhiclcpp/ParameterSet.h"
#include <vector>

namespace artdaq
{
	class StatisticsHelper;
}

/**
 * \brief This class manages MonitoredQuantity instances for the *Core classes.
 */
class artdaq::StatisticsHelper
{
public:
	/**
	 * \brief StatisticsHelper default constructor
	 */
	StatisticsHelper();

	/**
	 * \brief Copy Constructor is deleted
	 */
	StatisticsHelper(StatisticsHelper const&) = delete;

	/**
	 * \brief Default Destructor
	 */
	virtual ~StatisticsHelper() = default;

	/**
	 * \brief Copy Assignment operator is deleted
	 * \return StatisticsHelper copy
	 */
	StatisticsHelper& operator=(StatisticsHelper const&) = delete;

	/**
	 * \brief Add a MonitoredQuantity name to the list
	 * \param statKey Name of the MonitoredQuantity to be added
	 */
	void addMonitoredQuantityName(std::string const& statKey);

	/**
	 * \brief Add a sample to the MonitoredQuantity with the given name
	 * \param statKey Name of the MonitoredQuantity
	 * \param value Value to record in the MonitoredQuantity
	 */
	void addSample(std::string const& statKey, double value) const;

	/**
	 * \brief Create MonitoredQuantity objects for all names registered with the StatisticsHelper
	 * \param pset ParameterSet used to configure reporting
	 * \param defaultReportIntervalFragments Default reporting interval in Fragments
	 * \param defaultReportIntervalSeconds Default reporting interval in Seconds
	 * \param defaultMonitorWindow Default monitoring window
	 * \param primaryStatKeyName The primary (default) MonitoredQuantity
	 * \return Whether the primary MonitoredQuantity exists
	 * 
	 * StatisitcsHelper accpets the following Parameters:
	 * "reporting_interval_fragments" (Default given above): The reporting interval in Fragments
	 * "reporting_interval_seconds" (Default given above): The reporting interval in Seconds
	 * "monitor_window" (Default given above): The monitoring window for the MonitoredQuantity
	 * "monitor_binsize" (Default: 1 + ((monitorWindow - 1) / 100)): The monitoring bin size for the MonitoredQuantity
	 */
	bool createCollectors(fhicl::ParameterSet const& pset,
	                      int defaultReportIntervalFragments,
	                      double defaultReportIntervalSeconds,
	                      double defaultMonitorWindow,
	                      std::string const& primaryStatKeyName);

	/**
	 * \brief Reset all MonitoredQuantity instances
	 */
	void resetStatistics();

	/**
	 * \brief Determine if the reporting interval conditions have been met
	 * \param currentCount Current Fragment count
	 * \return Whether the StatisticsHelper is ready to report
	 */
	bool readyToReport(size_t currentCount);

	/**
	 * \brief Determine if the MonitoredQuantity "recent" window has changed since the last time this function was called
	 * \return Whether the MonitoredQuantity "recent" window has changed
	 */
	bool statsRollingWindowHasMoved();

private:
	std::vector<std::string> monitored_quantity_name_list_;
	artdaq::MonitoredQuantityPtr primary_stat_ptr_;

	int reporting_interval_fragments_;
	double reporting_interval_seconds_;
	size_t previous_reporting_index_;
	MonitoredQuantityStats::TIME_POINT_T previous_stats_calc_time_;
};

#endif /* artdaq_Application_MPI2_StatisticsHelper_hh */
