#ifndef artdaq_ArtModules_NetMonTransportServiceInterface_h
#define artdaq_ArtModules_NetMonTransportServiceInterface_h

#include "art/Framework/Services/Registry/ServiceMacros.h"

class TBufferFile;

/**
 * \brief Interface for NetMonTranportService. This interface is declared to art as part of the required registration of an art Service
 */
class NetMonTransportServiceInterface
{
public:
	/**
	 * \brief Default virtual destructor
	 */
	virtual ~NetMonTransportServiceInterface() = default;

	/**
	 * \brief Connect the NetMonTransportService
	 * 
	 * This is a pure virtual function, derived classes must reimplement it
	 */
	virtual void connect() = 0;

	/**
	* \brief Disconnect the NetMonTransportService
	*
	* This is a pure virtual function, derived classes must reimplement it
	*/
	virtual void disconnect() = 0;

	/**
	* \brief Listen for new connections
	*
	* This is a pure virtual function, derived classes must reimplement it
	*/
	virtual void listen() = 0;

	/**
	 * \brief Send a message
	 * \param sequenceId Sequence ID of Fragment wrapping the message
	 * \param messageType Fragment type of Fragment wrapping the message
	 * \param msg ROOT data to send
	 */
	virtual void sendMessage(uint64_t sequenceId, uint8_t messageType, TBufferFile& msg) = 0;

	/**
	 * \brief Receive a message
	 * \param[out] msg ROOT message data
	 */
	virtual void receiveMessage(TBufferFile*& msg) = 0;

	/**
	* \brief Receive the init message
	* \param[out] msg ROOT message data
	*/
	virtual void receiveInitMessage(TBufferFile*& msg) = 0;
};

DECLARE_ART_SERVICE_INTERFACE(NetMonTransportServiceInterface, LEGACY)
#endif /* artdaq_ArtModules_NetMonTransportServiceInterface_h */

// Local Variables:
// mode: c++
// End:
