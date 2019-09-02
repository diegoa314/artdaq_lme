#ifndef ARTDAQ_DAQRATE_DATASENDERMANAGER_HH
#define ARTDAQ_DAQRATE_DATASENDERMANAGER_HH

#include <map>
#include <set>
#include <memory>
#include <netinet/in.h>

#include "fhiclcpp/fwd.h"

#include "artdaq-core/Data/Fragment.hh"
#include "artdaq/TransferPlugins/TransferInterface.hh"
#include "artdaq/DAQrate/detail/FragCounter.hh"
#include "artdaq-utilities/Plugins/MetricManager.hh"
#include "artdaq/DAQrate/detail/RoutingPacket.hh"
#include "artdaq/TransferPlugins/detail/HostMap.hh"
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/types/OptionalTable.h"
#include "fhiclcpp/types/TableFragment.h"

namespace artdaq
{
	class DataSenderManager;
}

/**
 * \brief Sends Fragment objects using TransferInterface plugins. Uses Routing Tables if confgiured,
 * otherwise will Round-Robin Fragments to the destinations.
 */
class artdaq::DataSenderManager
{
public:
	/// <summary>
	/// Configuration for Routing table reception
	/// 
	/// This configuration should be the same for all processes receiving routing tables from a given RoutingMaster.
	/// </summary>
	struct RoutingTableConfig
	{
		///   "use_routing_master" (Default: false): True if using the Routing Master
		fhicl::Atom<bool> use_routing_master{ fhicl::Name{ "use_routing_master"}, fhicl::Comment{ "True if using the Routing Master"}, false };
		///   "table_update_port" (Default: 35556): Port that table updates should arrive on
		fhicl::Atom<int> table_port{ fhicl::Name{ "table_update_port"}, fhicl::Comment{ "Port that table updates should arrive on" },35556 };
		///   "table_update_address" (Default: "227.128.12.28"): Address that table updates should arrive on
		fhicl::Atom<std::string> table_address{ fhicl::Name{ "table_update_address"}, fhicl::Comment{ "Address that table updates should arrive on" }, "227.128.12.28" };
		///   "table_acknowledge_port" (Default: 35557): Port that acknowledgements should be sent to
		fhicl::Atom<int> ack_port{ fhicl::Name{ "table_acknowledge_port" },fhicl::Comment{ "Port that acknowledgements should be sent to" },35557 };
		///   "routing_master_hostname" (Default: "localhost"): Host that acknowledgements should be sent to
		fhicl::Atom<std::string> ack_address{ fhicl::Name{ "routing_master_hostname"}, fhicl::Comment{ "Host that acknowledgements should be sent to" },"localhost" };
		///   "routing_timeout_ms" (Default: 1000): Time to wait for a routing table update if the table is exhausted
		fhicl::Atom<int> routing_timeout_ms{ fhicl::Name{"routing_timeout_ms"}, fhicl::Comment{"Time to wait (in ms) for a routing table update if the table is exhausted"}, 1000 };
		///   "routing_retry_count" (Default: 5): Number of times to retry calculating destination before giving up (DROPPING DATA!)
		fhicl::Atom<int> routing_retry_count{ fhicl::Name{"routing_retry_count"}, fhicl::Comment{"Number of times to retry getting destination from routing table"}, 5 };
		///   "routing_table_max_size" (Default: 1000): Maximum number of entries in the routing table
		fhicl::Atom<size_t> routing_table_max_size{ fhicl::Name{"routing_table_max_size"}, fhicl::Comment{"Maximum number of entries in the routing table"}, 1000 };
	};

	/// <summary>
	/// Configuration for transfers to destinations
	/// </summary>
	struct DestinationsConfig
	{
		/// Example Configuration for transfer to destination. See artdaq::TransferInterface::Config
		fhicl::OptionalTable<artdaq::TransferInterface::Config> dest{ fhicl::Name{"d1"}, fhicl::Comment{"Configuration for transfer to destination"} };
	};

	/// <summary>
	/// Configuration of DataSenderManager. May be used for parameter validation
	/// </summary>
	struct Config
	{
		/// "broadcast_sends" (Default: false): Send all Fragments to all destinations
		fhicl::Atom<bool> broadcast_sends{ fhicl::Name{"broadcast_sends"}, fhicl::Comment{"Send all Fragments to all destinations"}, false };
		/// "nonblocking_sends" (Default: false): If true, will use non-reliable mode of TransferInterface plugins
		fhicl::Atom<bool> nonblocking_sends{ fhicl::Name{"nonblocking_sends"}, fhicl::Comment{"Whether sends should block. Used for DL->DISP connection."}, false };
		/// "send_timeout_usec" (Default: 5000000 (5 seconds): Timeout for sends in non-reliable modes (broadcast and nonblocking)
		fhicl::Atom<size_t> send_timeout_us{ fhicl::Name{"send_timeout_usec"}, fhicl::Comment{"Timeout for sends in non-reliable modes (broadcast and nonblocking)"},5000000 };
		/// "send_retry_count" (Default: 2): Number of times to retry a send in non-reliable mode
		fhicl::Atom<size_t> send_retry_count{ fhicl::Name{"send_retry_count"}, fhicl::Comment{"Number of times to retry a send in non-reliable mode"}, 2 };
		fhicl::OptionalTable<RoutingTableConfig> routing_table_config{ fhicl::Name{"routing_table_config"} }; ///< Configuration for Routing Table reception. See artdaq::DataSenderManager::RoutingTableConfig
		/// "destinations" (Default: Empty ParameterSet): FHiCL table for TransferInterface configurations for each destaintion. See artdaq::DataSenderManager::DestinationsConfig
	    ///   NOTE: "destination_rank" MUST be specified (and unique) for each destination!
		fhicl::OptionalTable<DestinationsConfig> destinations{ fhicl::Name{"destinations"} };
		fhicl::TableFragment<artdaq::HostMap::Config> host_map; ///< Optional host_map configuration (Can also be specified in each DestinationsConfig entry. See artdaq::HostMap::Config
	    /// enabled_destinations" (OPTIONAL): If specified, only the destination ranks listed will be enabled. If not specified, all destinations will be enabled.
		fhicl::Sequence<size_t> enabled_destinations{ fhicl::Name{"enabled_destinations"}, fhicl::Comment{"List of destiantion ranks to activate (must be defined in destinations block)"}, std::vector<size_t>() };
	};
	using Parameters = fhicl::WrappedTable<Config>;

	/**
	 * \brief DataSenderManager Constructor
	 * \param ps ParameterSet used to configure the DataSenderManager. See artdaq::DataSenderManager::Config
	 */
	explicit DataSenderManager(const fhicl::ParameterSet& ps);

	/**
	 * \brief DataSenderManager Destructor
	 */
	virtual ~DataSenderManager();

	/**
	 * \brief Send the given Fragment. Return the rank of the destination to which the Fragment was sent.
	 * \param frag Fragment to sent
	 * \return Pair containing Rank of destination for Fragment and the CopyStatus from the send call
	 */
	std::pair<int, TransferInterface::CopyStatus> sendFragment(Fragment&& frag);

	/**
	* \brief Return the count of Fragment objects sent by this DataSenderManagerq
	* \return The count of Fragment objects sent by this DataSenderManager
	*/
	size_t count() const;

	/**
	* \brief Get the count of Fragment objects sent by this DataSenderManager to a given destination
	* \param rank Destination rank to get count for
	* \return The  count of Fragment objects sent by this DataSenderManager to the destination
	*/
	size_t slotCount(size_t rank) const;

	/**
	 * \brief Get the number of configured destinations
	 * \return The number of configured destinations
	 */
	size_t destinationCount() const { return destinations_.size(); }

	/**
	 * \brief Get the list of enabled destinations
	 * \return The list of enabled destiantion ranks
	 */
	std::set<int> enabled_destinations() const { return enabled_destinations_; }

	/**
	 * \brief Gets the current size of the Routing Table, in case other parts of the system want to use this information
	 * \return The current size of the Routing Table.
	 */
	size_t GetRoutingTableEntryCount() const;

	/**
	 * \brief Gets the number of sends remaining in the routing table, in case other parts of the system want to use this information
	 * \return The number of sends remaining in the routing table
	 */
	size_t GetRemainingRoutingTableEntries() const;

	/**
	 * \brief Stop the DataSenderManager, aborting any sends in progress
	 */
	void StopSender() { should_stop_ = true; }

private:

	// Calculate where the fragment with this sequenceID should go.
	int calcDest_(Fragment::sequence_id_t) const;

	void setupTableListener_();

	void startTableReceiverThread_();

	void receiveTableUpdatesLoop_();
private:

	std::map<int, std::unique_ptr<artdaq::TransferInterface>> destinations_;
	std::unordered_map<int, std::pair<size_t, double>> destination_metric_data_;
	std::unordered_map<int, std::chrono::steady_clock::time_point> destination_metric_send_time_;
	std::set<int> enabled_destinations_;

	detail::FragCounter sent_frag_count_;

	bool broadcast_sends_;
	bool non_blocking_mode_;
	size_t send_timeout_us_;
	size_t send_retry_count_;

	bool use_routing_master_;
	detail::RoutingMasterMode routing_master_mode_;
	std::atomic<bool> should_stop_;
	int table_port_;
	std::string table_address_;
	int ack_port_;
	std::string ack_address_;
	struct sockaddr_in ack_addr_;
	int ack_socket_;
	int table_socket_;
	std::map<Fragment::sequence_id_t, int> routing_table_;
	Fragment::sequence_id_t routing_table_last_;
	size_t routing_table_max_size_;
	mutable std::mutex routing_mutex_;
	boost::thread routing_thread_;
	mutable std::atomic<size_t> routing_wait_time_;

	int routing_timeout_ms_;
	int routing_retry_count_;

	mutable std::atomic<uint64_t> highest_sequence_id_routed_;

};

inline
size_t
artdaq::DataSenderManager::
count() const
{
	return sent_frag_count_.count();
}

inline
size_t
artdaq::DataSenderManager::
slotCount(size_t rank) const
{
	return sent_frag_count_.slotCount(rank);
}
#endif //ARTDAQ_DAQRATE_DATASENDERMANAGER_HH
