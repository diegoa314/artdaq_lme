#ifndef artdaq_DAQrate_detail_RoutingPacket_hh
#define artdaq_DAQrate_detail_RoutingPacket_hh

#include "artdaq-core/Data/Fragment.hh"

namespace artdaq
{
	namespace detail
	{
		struct RoutingPacketEntry;
		/**
		 * \brief A RoutingPacket is simply a vector of RoutingPacketEntry objects.
		 * It is not suitable for network transmission, rather a RoutingPacketHeader
		 * should be sent, followed by &RoutingPacket.at(0) (the physical storage of the vector)
		 */
		using RoutingPacket = std::vector<RoutingPacketEntry>;
		struct RoutingPacketHeader;
		struct RoutingAckPacket;
		struct RoutingToken;

		/**
		 * \brief Mode indicating whether the RoutingMaster is routing events by Sequence ID or by Send Count
		 */
		enum class RoutingMasterMode : uint8_t
		{
			RouteBySequenceID, ///< Events should be routed by sequence ID (BR -> EB)
			RouteBySendCount, ///< Events should be routed by send count (EB -> Agg)
			INVALID
		};
	}
}
/**
 * \brief A row of the Routing Table
 */
struct artdaq::detail::RoutingPacketEntry
{
	/**
	 * \brief Default Constructor
	 */
	RoutingPacketEntry() : sequence_id(Fragment::InvalidSequenceID), destination_rank(-1) {}
	/**
	 * \brief Construct a RoutingPacketEntry with the given sequence ID and destination rank
	 * \param seq The sequence ID of the RoutingPacketEntry
	 * \param rank The destination rank for this sequence ID
	 */
	RoutingPacketEntry(Fragment::sequence_id_t seq, int rank) : sequence_id(seq), destination_rank(rank) {}
	Fragment::sequence_id_t sequence_id; ///< The sequence ID of the RoutingPacketEntry
	int destination_rank; ///< The destination rank for this sequence ID
};


/**
 * \brief Magic bytes expected in every RoutingPacketHeader
 */
#define ROUTING_MAGIC 0x1337beef

/**
 * \brief The header of the Routing Table, containing the magic bytes and the number of entries
 */
struct artdaq::detail::RoutingPacketHeader
{
	uint32_t header; ///< Magic bytes to make sure the packet wasn't garbled
	RoutingMasterMode mode; ///< The current mode of the RoutingMaster
	size_t nEntries; ///< The number of RoutingPacketEntries in the RoutingPacket

	/**
	 * \brief Construct a RoutingPacketHeader declaring a given number of entries
	 * \param m The RoutingMasterMode that senders are supposed to be operating in
	 * \param n The number of RoutingPacketEntries in the associated RoutingPacket
	 */
	explicit RoutingPacketHeader(RoutingMasterMode m, size_t n) : header(ROUTING_MAGIC), mode(m), nEntries(n) {}
	/**
	 * \brief Default Constructor
	 */
	RoutingPacketHeader() : header(0), mode(RoutingMasterMode::INVALID), nEntries(0) {}
};

/**
 * \brief A RoutingAckPacket contains the rank of the table receiver, plus the first and last sequence IDs in the Routing Table (for verification)
 */
struct artdaq::detail::RoutingAckPacket
{
	int rank; ///< The rank from which the RoutingAckPacket came
	Fragment::sequence_id_t first_sequence_id; ///< The first sequence ID in the received RoutingPacket
	Fragment::sequence_id_t last_sequence_id; ///< The last sequence ID in the received RoutingPacket
};


/**
 * \brief Magic bytes expected in every RoutingToken
 */
#define TOKEN_MAGIC 0xbeefcafe

/**
 * \brief The RoutingToken contains the magic bytes, the rank of the token sender, and the number of slots free. This is 
 * a TCP message, so additional verification is not necessary.
 */
struct artdaq::detail::RoutingToken
{
	uint32_t header; ///< The magic bytes that help validate the RoutingToken
	int rank; ///< The rank from which the RoutingToken came
	unsigned new_slots_free; ///< The number of slots free in the token sender (usually 1)
};

#endif //artdaq_Application_Routing_RoutingPacket_hh