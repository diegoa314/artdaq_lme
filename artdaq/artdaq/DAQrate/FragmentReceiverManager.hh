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

namespace artdaq
{
	class FragmentReceiverManager;
	class FragmentStoreElement;
}

/**
 * \brief Receives Fragment objects from one or more DataSenderManager instances using TransferInterface plugins
 * DataReceiverMaanger runs a reception thread for each source, and can automatically suppress reception from
 * sources which are going faster than the others.
 */
class artdaq::FragmentReceiverManager
{
public:

	/**
	 * \brief FragmentReceiverManager Constructor
	 * \param ps ParameterSet used to configure the FragmentReceiverManager
	 *
	 * \verbatim
	 * FragmentReceiverManager accepts the following Parameters:
	 * "auto_suppression_enabled" (Default: true): Whether to suppress a source that gets too far ahead
	 * "max_receive_difference" (Default: 50): Threshold (in sequence ID) for suppressing a source
	 * "receive_timeout_usec" (Default: 100000): The timeout for receive operations
	 * "enabled_sources" (OPTIONAL): List of sources which are enabled. If not specified, all sources are assumed enabled
	 * "sources" (Default: blank table): FHiCL table containing TransferInterface configurations for each source.
	 *   NOTE: "source_rank" MUST be specified (and unique) for each source!
	 * \endverbatim
	 */
	explicit FragmentReceiverManager(const fhicl::ParameterSet& ps);

	/**
	 * \brief FragmentReceiverManager Destructor
	 */
	virtual ~FragmentReceiverManager();

	/**
	 * \brief Receive a Fragment
	 * \param[out] rank Rank of sender that sent the Fragment, or RECV_TIMEOUT
	 * \param timeout_usec Timeout to wait for a Fragment to become ready
	 * \return Pointer to received Fragment. May be nullptr if no Fragments are ready
	 */
	FragmentPtr recvFragment(int& rank, size_t timeout_usec = 0);

	/**
	 * \brief Return the count of Fragment objects received by this FragmentReceiverManager
	 * \return The count of Fragment objects received by this FragmentReceiverManager
	 */
	size_t count() const;

	/**
	 * \brief Get the count of Fragment objects received by this FragmentReceiverManager from a given source
	 * \param rank Source rank to get count for
	 * \return The  count of Fragment objects received by this FragmentReceiverManager from the source
	 */
	size_t slotCount(size_t rank) const;

	/**
	 * \brief Get the total size of all data recieved by this FragmentReceiverManager
	 * \return The total size of all data received by this FragmentReceiverManager
	 */
	size_t byteCount() const;

	/**
	 * \brief Start receiver threads for all enabled sources
	 */
	void start_threads();

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

private:
	void runReceiver_(int);

	bool fragments_ready_() const;

	int get_next_source_() const;

	std::atomic<bool> stop_requested_;

	std::map<int, boost::thread> source_threads_;
	std::map<int, std::unique_ptr<TransferInterface>> source_plugins_;
	std::unordered_map<int, std::pair<size_t, double>> source_metric_data_;
	std::unordered_map<int, std::chrono::steady_clock::time_point> source_metric_send_time_;
	std::unordered_map<int, std::atomic<bool>> enabled_sources_;
	std::unordered_map<int, std::atomic<bool>> running_sources_;

	std::map<int, FragmentStoreElement> fragment_store_;

	std::mutex input_cv_mutex_;
	std::condition_variable input_cv_;
	std::mutex output_cv_mutex_;
	std::condition_variable output_cv_;

	detail::FragCounter recv_frag_count_; // Number of frags received per source.
	detail::FragCounter recv_frag_size_; // Number of bytes received per source.
	detail::FragCounter recv_seq_count_; // For counting sequence IDs
	bool suppress_noisy_senders_;
	size_t suppression_threshold_;

	size_t receive_timeout_;
	mutable int last_source_;
};

/**
 * \brief This class contains tracking information for all Fragment objects which have been received from a specific source
 *
 * This class was designed so that there could be a mutex for each source, instead of locking all sources whenever a
 * Fragment had to be retrieved. FragmentStoreElement is itself a container type, sorted by Fragment arrival time. It is a
 * modified queue, with only the first element accessible, but it allows elements to be added to either end (for rejected Fragments).
 */
class artdaq::FragmentStoreElement
{
public:
	/**
	 * \brief FragmentStoreElement Constructor
	 */
	FragmentStoreElement()
		: frags_()
		, empty_(true)
		, eod_marker_(-1)
	{
		std::cout << "FragmentStoreElement CONSTRUCTOR" << std::endl;
	}

	/**
	 * \brief Are any Fragment objects contained in this FragmentStoreElement?
	 * \return Whether any Fragment objects are contained in this FragmentStoreElement
	 */
	bool empty() const
	{
		return empty_;
	}

	/**
	 * \brief Add a Fragment to the front of the FragmentStoreElement
	 * \param frag Fragment to add
	 */
	void emplace_front(FragmentPtr&& frag)
	{
		std::unique_lock<std::mutex> lk(mutex_);
		frags_.emplace_front(std::move(frag));
		empty_ = false;
	}

	/**
	 * \brief Add a Fragment to the end of the FragmentStoreElement
	 * \param frag Fragment to add
	 */
	void emplace_back(FragmentPtr&& frag)
	{
		std::unique_lock<std::mutex> lk(mutex_);
		frags_.emplace_back(std::move(frag));
		empty_ = false;
	}

	/**
	 * \brief Remove the first Fragment from the FragmentStoreElement and return it
	 * \return The first Fragment in the FragmentStoreElement
	 */
	FragmentPtr front()
	{
		std::unique_lock<std::mutex> lk(mutex_);
		auto current_fragment = std::move(frags_.front());
		frags_.pop_front();
		empty_ = frags_.size() == 0;
		return current_fragment;
	}

	/**
	 * \brief Set the End-Of-Data marker value for this Receiver
	 * \param eod Number of Receives expected for this receiver
	 */
	void SetEndOfData(size_t eod) { eod_marker_ = eod; }
	/**
	 * \brief Get the value of the End-Of-Data marker for this Receiver
	 * \return The value of the End-Of-Data marker. Returns -1 (0xFFFFFFFFFFFFFFFF) if no EndOfData Fragments received
	 */
	size_t GetEndOfData() const { return eod_marker_; }

	/**
	 * \brief Get the number of Fragments stored in this FragmentStoreElement
	 * \return The number of Fragments stored in this FragmentStoreElement
	 */
	size_t size() const { return frags_.size(); }

private:
	mutable std::mutex mutex_;
	FragmentPtrs frags_;
	std::atomic<bool> empty_;
	size_t eod_marker_;
};

inline
size_t
artdaq::FragmentReceiverManager::
count() const
{
	return recv_frag_count_.count();
}

inline
size_t
artdaq::FragmentReceiverManager::
slotCount(size_t rank) const
{
	return recv_frag_count_.slotCount(rank);
}

inline
size_t
artdaq::FragmentReceiverManager::
byteCount() const
{
	return recv_frag_size_.count();
}
#endif //ARTDAQ_DAQRATE_DATATRANSFERMANAGER_HH
