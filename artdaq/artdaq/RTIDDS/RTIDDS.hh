#ifndef artdaq_RTIDDS_RTIDDS_hh
#define artdaq_RTIDDS_RTIDDS_hh

#include "artdaq-core/Data/Fragment.hh"

#include <ndds/ndds_cpp.h>

#include <string>
#include <queue>
#include <mutex>


namespace artdaq
{
	class RTIDDS;
}

/**
 * \brief DDS Transport Implementation
 */
class artdaq::RTIDDS
{
public:

	/**
	 * \brief Whether this DDS instance is a reader or a writer
	 */
	enum class IOType
	{
		reader,
		writer
	};

	/**
	 * \brief Construct a RTIDDS transmitter
	 * \param name Name of the module
	 * \param iotype Direction of transmission
	 * \param max_size Maximum size to transmit
	 */
	RTIDDS(std::string name, IOType iotype, std::string max_size = "1000000");

	/**
	 * \brief Default virtrual Destructor
	 */
	virtual ~RTIDDS() = default;

	// JCF, Apr-7-2016
	// Are copy constructor, assignment operators, etc., logical absurdities?

	/**
	 * \brief Copy a Fragment to DDS
	 * \param fragment Fragment to copy
	 * 
	 * This function may be non-reliable, and induces a memcpy of the Fragment
	 */
	void transfer_fragment_min_blocking_mode_via_DDS_(artdaq::Fragment const& fragment);

	/**
	 * \brief Move a Fragment to DDS
	 * \param fragment Fragment to move
	 * 
	 * This function should be reliable, and minimize copies.
	 * Currently implemented via transfer_fragment_min_blocking_mode_via_DDS_
	 */
	void transfer_fragment_reliable_mode_via_DDS_(artdaq::Fragment&& fragment);

	/**
	 * \brief A class that reads data from DDS
	 */
	class OctetsListener: public DDSDataReaderListener
	{
	public:

		/**
		 * \brief Action to perform when data is available
		 * \param reader Reader reference to read data from
		 */
		void on_data_available(DDSDataReader* reader);

		/**
		 * \brief Receive a Fragment from DDS
		 * \param[out] fragment Received Fragment
		 * \param receiveTimeout Timeout for receive operation
		 * \return Whether the receive succeeded in receiveTimeout
		 */
		bool receiveFragmentFromDDS(artdaq::Fragment& fragment,
		                            const size_t receiveTimeout);

	private:

		DDS_Octets dds_octets_;
		std::queue<DDS_Octets> dds_octets_queue_;

		std::mutex queue_mutex_;
	};

	OctetsListener octets_listener_; ///< The receiver

private:

	std::string name_;
	IOType iotype_;
	std::string max_size_;

	std::unique_ptr<DDSDomainParticipant, std::function<void(DDSDomainParticipant * )>> participant_;

	DDSTopic* topic_octets_;
	DDSOctetsDataWriter* octets_writer_;
	DDSDataReader* octets_reader_;

	static void participantDeleter(DDSDomainParticipant* participant);
};

#endif
