#ifndef artdaq_ArtModules_NetMonTransportService_h
#define artdaq_ArtModules_NetMonTransportService_h

#include "art/Framework/Services/Registry/ServiceMacros.h"

#include "artdaq/ArtModules/NetMonTransportServiceInterface.h"
#include "artdaq/DAQrate/DataSenderManager.hh"
#include "artdaq-core/Core/SharedMemoryEventReceiver.hh"

// ----------------------------------------------------------------------

/**
 * \brief NetMonTransportService extends NetMonTransportServiceInterface.
 * It sends events using DataSenderManager and receives events from the GlobalQueue
 */
class NetMonTransportService : public NetMonTransportServiceInterface
{
public:
	/// <summary>
	/// Allowed Configuration parameters of NetMonTransportService. May be used for configuration validation
	/// </summary>
	struct Config
	{
		/// "shared_memory_key" (Default: 0xBEE70000 + pid): Key to use when connecting to shared memory. Will default to 0xBEE70000 + getppid().
		fhicl::Atom<uint32_t> shared_memory_key{ fhicl::Name{"shared_memory_key"},fhicl::Comment{"Key to use when connecting to shared memory. Will default to 0xBEE70000 + getppid()."},0xBEE70000 };
		/// "shared_memory_key" (Default: 0xCEE70000 + pid): Key to use when connecting to broadcast shared memory. Will default to 0xCEE70000 + getppid().
		fhicl::Atom<uint32_t> broadcast_shared_memory_key{ fhicl::Name{ "broadcast_shared_memory_key" },fhicl::Comment{ "Key to use when connecting to broadcast shared memory. Will default to 0xCEE70000 + getppid()."},0xCEE70000 };
		/// "rank" (OPTIONAL) : The rank of this applicaiton, for use by non - artdaq applications running NetMonTransportService
		fhicl::Atom<int> rank{ fhicl::Name{"rank"}, fhicl::Comment{"Rank of this artdaq application. Used for data transfers"} };
		/// "init_fragment_timeout_seconds" (Default: 1.0): Amount of time to wait, in seconds, for init Fragment to arrive
		fhicl::Atom<double> init_fragment_timeout{ fhicl::Name{"init_fragment_timeout_seconds"}, fhicl::Comment{"Amount of time to wait, in seconds, for init Fragment to arrive"}, 1.0 };
		fhicl::TableFragment<artdaq::DataSenderManager::Config> dataSenderConfig; ///< Configuration for DataSenderManager. See artdaq::DataSenderManager::Config
	};
	using Parameters = fhicl::WrappedTable<Config>;


	/**
	 * \brief NetMonTransportService Destructor. Calls disconnect().
	 */
	virtual ~NetMonTransportService();

	/**
	 * \brief NetMonTransportService Constructor
	 * \param pset ParameterSet used to configure NetMonTransportService and DataSenderManager. See NetMonTransportService::Config
	 */
	NetMonTransportService(fhicl::ParameterSet const& pset, art::ActivityRegistry&);

	/**
	 * \brief Reconnect the NetMonTransportService.
	 *
	 * Creates a new instance of DataSenderManager using the stored ParameterSet
	 */
	void connect() override;

	/**
	 * \brief Disconnects the NetMonTranportService
	 *
	 * Destructs the DataSenderManager
	 */
	void disconnect() override;

	/**
	 * \brief Listen for connections. This method is a No-Op.
	 */
	void listen() override;

	/**
	 * \brief Send ROOT data, wrapped in an artdaq::Fragment object
	 * \param sequenceId The sequence id of the Fragment which will wrap the ROOT data
	 * \param messageType The type id of the Fragment which will wrap the ROOT data
	 * \param msg The ROOT data to send
	 */
	void sendMessage(uint64_t sequenceId, uint8_t messageType, TBufferFile& msg) override;

	/**
	 * \brief Receive data from the ConcurrentQueue
	 * \param[out] msg Received data
	 */
	void receiveMessage(TBufferFile*& msg) override;

	void receiveInitMessage(TBufferFile*& msg) override;

	/**
	 * \brief Get the number of data receivers
	 * \return The number of data receivers
	 */
	size_t dataReceiverCount() const { return sender_ptr_->destinationCount(); }
private:
	fhicl::ParameterSet data_pset_;
	bool init_received_;
	double init_timeout_s_;

	std::unique_ptr<artdaq::DataSenderManager> sender_ptr_;
	std::unique_ptr<artdaq::SharedMemoryEventReceiver> incoming_events_;
	std::unique_ptr<std::vector<artdaq::Fragment>> recvd_fragments_;

	void setupEventReceiver_();
};

DECLARE_ART_SERVICE_INTERFACE_IMPL(NetMonTransportService, NetMonTransportServiceInterface, LEGACY)
#endif /* artdaq_ArtModules_NetMonTransportService_h */

// Local Variables:
// mode: c++
// End:
