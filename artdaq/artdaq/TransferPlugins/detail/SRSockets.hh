#ifndef SRSockets_hh
#define SRSockets_hh
#include <cstdint>

// This file (SRSockets.hh) was created by Ron Rechenmacher <ron@fnal.gov> on
// Sep 14, 2016. "TERMS AND CONDITIONS" governing this file are in the README
// or COPYING file. If you do not have such a file, one can be obtained by
// contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
// $RCSfile: .emacs.gnu,v $
// rev="$Revision: 1.30 $$Date: 2016/03/01 14:27:27 $";

/**
 * \brief This header is sent by the TCPSocket_transfer to allow for more efficient writev calls
 */
struct MessHead
{
	uint8_t endian; ///< 0=little(intel), 1=big
	
	/**
	 * \brief The Message Type
	 * 
	 * Only add to the end!
	 */
	enum MessType
	{
		connect_v0 = 0,
		data_v0,
		data_more_v0,
		stop_v0,
		routing_v0,
		ack_v0,
		header_v0
	};

	MessType message_type; ///< Message Type
	int64_t source_id; ///< Rank of the source

	union
	{
		uint32_t conn_magic; ///< unsigned first is better for MessHead initializer: {0,0,my_node_idx_,CONN_MAGIC}
		int32_t byte_count; ///< use CONN_MAGIC for connect_v0, data that follow for data_v0 (and 0 lenght data)
	};
};

/**
 * \brief Magic Bytes to put in header
 */
#define CONN_MAGIC 0xcafefeca
#define ACK_MAGIC 0xbeeffeed
#endif // SRSockets_hh
