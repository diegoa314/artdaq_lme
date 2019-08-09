#ifndef ARTDAQ_DAQRATE_DATATRANSFERMANAGER_HH
#define ARTDAQ_DAQRATE_DATATRANSFERMANAGER_HH

#include <map>
#include <set>
#include <memory>
#include <condition_variable>

#include "fhiclcpp/fwd.h"

#include "artdaq-core/Data/Fragment.hh"
#include "artdaq/TransferPlugins/TransferInterface.hh"
#include "artdaq/DAQrate/detail/FragCounter.hh"
#include "artdaq-utilities/Plugins/MetricManager.hh"
#include "artdaq/DAQrate/SharedMemoryEventManager.hh"

namespace artdaq
{
	class DataReceiverManager;
}

/**
 * \brief Receives Fragment objects from one or more DataSenderManager instances using TransferInterface plugins
 * DataReceiverMaanger runs a reception thread for each source, and can automatically suppress reception from
 * sources which are going faster than the others.
 */
class artdaq::DataReceiverManager
{
public:

	/**
	 * \brief DataReceiverManager Constructor
	 * \param ps ParameterSet used to configure the DataReceiverManager
	 * \param shm Pointer to SharedMemoryEventManager instance (destination for received data)
	 *
	 * \verbatim
	 * DataReceiverManager accepts the following Parameters:
	 * "auto_suppression_enabled" (Default: true): Whether to suppress a source that gets too far ahead
	 * "max_receive_difference" (Default: 50): Threshold (in sequence ID) for suppressing a source
	 * "receive_timeout_usec" (Default: 100000): The timeout for receive operations
	 * "enabled_sources" (OPTIONAL): List of sources which are enabled. If not specified, all sources are assumed enabled
	 * "sources" (Default: blank table): FHiCL table containing TransferInterface configurations for each source.
	 *   NOTE: "source_rank" MUST be specified (and unique) for each source!
	 * \endverbatim
	 */
	explicit DataReceiverManager(const fhicl::ParameterSet& ps, std::shared_ptr<SharedMemoryEventManager> shm);

	/**
	 * \brief DataReceiverManager Destructor
	 */
	virtual ~DataReceiverManager();
	
	/**
	 * \brief Return the count of Fragment objects received by this DataReceiverManager
	 * \return The count of Fragment objects received by this DataReceiverManager
	 */
	size_t count() const;

	/**
	 * \brief Get the count of Fragment objects received by this DataReceiverManager from a given source
	 * \param rank Source rank to get count for
	 * \return The  count of Fragment objects received by this DataReceiverManager from the source
	 */
	size_t slotCount(size_t rank) const;

	/**
	 * \brief Get the total size of all data recieved by this DataReceiverManager
	 * \return The total size of all data received by this DataReceiverManager
	 */
	size_t byteCount() const;

	/**
	 * \brief Start receiver threads for all enabled sources
	 */
	void start_threads();

	/**
	 * \brief Stop receiver threads 
	 */
	void stop_threads();

	/**
	 * \brief Get the list of enabled sources
	 * \return The list of enabled sources
	 */
	std::set<int> enabled_sources() const;

	/**
	 * \brief Get the list of sources which are still receiving data
	 * \return std::set containing ranks of sources which are still receiving data
	 */
	std::set<int> running_sources() const;

	/**
	 * \brief Get a handle to the SharedMemoryEventManager connected to this DataReceiverManager
	 * \return shared_ptr to SharedMemoryEventManager instance
	 */
	std::shared_ptr<SharedMemoryEventManager> getSharedMemoryEventManager() const { return shm_manager_; }


	/**
	 * \brief Get a pointer to the FragCounter instance tracking the number of received Fragments
	 * \return Pointer to the FragCounter instance tracking the number of recevied Fragments
	 */
	std::shared_ptr<detail::FragCounter> GetReceivedFragmentCount() { return std::shared_ptr<detail::FragCounter>(&recv_frag_count_); }

private:
	void runReceiver_(int);
		
	std::atomic<bool> stop_requested_;
	std::atomic<size_t> stop_requested_time_;

	std::map<int, boost::thread> source_threads_;
	std::map<int, std::unique_ptr<TransferInterface>> source_plugins_;

	struct source_metric_data
	{
		source_metric_data() : delta_t(0), hdr_delta_t(0), store_delta_t(0), data_delta_t(0),dead_t(0), data_point_count(0), data_size(0), header_size(0) {}
		double delta_t;
		double hdr_delta_t;
		double store_delta_t;
		double data_delta_t;
		double dead_t;
		size_t data_point_count;
		size_t data_size;
		size_t header_size;
	};

	std::unordered_map<int, source_metric_data> source_metric_data_;
	std::unordered_map<int, std::chrono::steady_clock::time_point> source_metric_send_time_;
	std::unordered_map<int, std::atomic<bool>> enabled_sources_;
	std::unordered_map<int, std::atomic<bool>> running_sources_;

	detail::FragCounter recv_frag_count_; // Number of frags received per source.
	detail::FragCounter recv_frag_size_; // Number of bytes received per source.
	detail::FragCounter recv_seq_count_; // For counting sequence IDs

	size_t receive_timeout_;
	size_t stop_timeout_ms_;
	std::shared_ptr<SharedMemoryEventManager> shm_manager_;

	bool non_reliable_mode_enabled_;
	size_t non_reliable_mode_retry_count_;
};

inline
size_t
artdaq::DataReceiverManager::
count() const
{
	return recv_frag_count_.count();
}

inline
size_t
artdaq::DataReceiverManager::
slotCount(size_t rank) const
{
	return recv_frag_count_.slotCount(rank);
}

inline
size_t
artdaq::DataReceiverManager::
byteCount() const
{
	return recv_frag_size_.count();
}
#endif //ARTDAQ_DAQRATE_DATATRANSFERMANAGER_HH
