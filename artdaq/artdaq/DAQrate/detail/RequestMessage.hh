#ifndef artdaq_DAQrate_detail_RequestMessage_hh
#define artdaq_DAQrate_detail_RequestMessage_hh

#include "artdaq-core/Data/Fragment.hh"
#define MAX_REQUEST_MESSAGE_SIZE 65000

namespace artdaq
{
	namespace detail
	{
		struct RequestPacket;
		struct RequestHeader;
		class RequestMessage;

		/**
		 * \brief Mode used to indicate current run conditions to the request receiver
		 */
		enum class RequestMessageMode : uint8_t
		{
			Normal = 0, ///< Normal running
			EndOfRun = 1, ///< End of Run mode (Used to end request processing on receiver)
		};

		/**
		 * \brief Converts the RequestMessageMode to a string and sends it to the output stream
		 * \param o Stream to send string to
		 * \param m RequestMessageMode to convert to string
		 * \return o with string sent to it
		 */
		inline std::ostream& operator<<(std::ostream& o, RequestMessageMode m)
		{
			switch (m)
			{
			case RequestMessageMode::Normal:
				o << "Normal";
				break;
			case RequestMessageMode::EndOfRun:
				o << "EndOfRun";
				break;
			}
			return o;
		}

	}
}

/**
 * \brief The RequestPacket contains information about a single data request
 */
struct artdaq::detail::RequestPacket
{
public:
	/** The magic bytes for the request packet */
	uint32_t header; //TRIG, or 0x54524947
	Fragment::sequence_id_t sequence_id; ///< The sequence ID that responses to this request should use
	Fragment::timestamp_t timestamp; ///< The timestamp of the request

	/**
	 * \brief Default Constructor
	 */
	RequestPacket()
		: header(0)
		, sequence_id(Fragment::InvalidSequenceID)
		, timestamp(Fragment::InvalidTimestamp)
	{}

	/**
	 * \brief Create a RequestPacket using the given sequence ID and timestmap
	 * \param seq Sequence ID of RequestPacket
	 * \param ts Timestamp of RequestPAcket
	 */
	RequestPacket(const Fragment::sequence_id_t& seq, const Fragment::timestamp_t& ts)
		: header(0x54524947)
		, sequence_id(seq)
		, timestamp(ts)
	{}

	/**
	 * \brief Check the magic bytes of the packet
	 * \return Whether the correct magic bytes were found
	 */
	bool isValid() const { return header == 0x54524947; }
};

/**
 * \brief Header of a RequestMessage. Contains magic bytes for validation and a count of expected RequestPackets
 */
struct artdaq::detail::RequestHeader
{
	/** The magic bytes for the request header */
	uint32_t header; //HEDR, or 0x48454452
	uint32_t packet_count; ///< The number of RequestPackets in this Request message
	int      rank; ///< Rank of the sender
	RequestMessageMode mode; ///< Communicates additional information to the Request receiver

	/**
	 * \brief Default Constructor
	 */
	RequestHeader() : header(0x48454452)
		, packet_count(0)
		, rank(my_rank)
		, mode(RequestMessageMode::Normal)
	{}

	/**
	* \brief Check the magic bytes of the packet
	* \return Whether the correct magic bytes were found
	*/
	bool isValid() const { return header == 0x48454452; }
};

/**
 * \brief A RequestMessage consists of a RequestHeader and zero or more RequestPackets. They will usually be sent in two calls to send()
 */
class artdaq::detail::RequestMessage
{
public:
	/**
	 * \brief Default Constructor
	 */
	RequestMessage() : header_()
		, packets_()
	{}

	std::vector<uint8_t> GetMessage()
	{
		auto size = sizeof(RequestHeader) + packets_.size() * sizeof(RequestPacket);
		header_.packet_count = packets_.size();
		assert(size < MAX_REQUEST_MESSAGE_SIZE);
		auto output = std::vector<uint8_t>(size);
		memcpy(&output[0], &header_, sizeof(RequestHeader));
		memcpy(&output[sizeof(RequestHeader)], &packets_[0], packets_.size() * sizeof(RequestPacket));
		
		return output;
	}

	/**
	 * \brief Set the Request Message Mode for this request
	 * \param mode Mode for this Request Message
	 */
	void setMode(RequestMessageMode mode)
	{
		header_.mode = mode;
	}

	/**
	 * \brief Get the number of RequestPackets in the RequestMessage
	 * \return The number of RequestPackets in the RequestMessage
	 */
	size_t size() const { return packets_.size(); }

	/**
	 * \brief Add a request for a sequence ID and timestamp combination
	 * \param seq Sequence ID to request
	 * \param time Timestamp of request
	 */
	void addRequest(const Fragment::sequence_id_t& seq, const Fragment::timestamp_t& time)
	{
		packets_.emplace_back(RequestPacket(seq, time));
	}

private:
	RequestHeader header_;
	std::vector<RequestPacket> packets_;
};

#endif // artdaq_DAQrate_detail_RequestMessage
