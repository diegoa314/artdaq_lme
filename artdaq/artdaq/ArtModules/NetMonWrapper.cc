#define TRACE_NAME "NetMonWrapper"

#include "artdaq/ArtModules/NetMonWrapper.hh"

#include <TBufferFile.h>

void art::NetMonWrapper::receiveMessage(std::unique_ptr<TBufferFile>& msg)
{
	TLOG(5) << "Receiving Fragment from NetMonTransportService" ;
	TBufferFile* msg_ptr(nullptr);

	ServiceHandle<NetMonTransportService> transport;
	transport->receiveMessage(msg_ptr);

	msg.reset(msg_ptr);
	TLOG(5) << "Done Receiving Fragment from NetMonTransportService" ;
}

void art::NetMonWrapper::receiveInitMessage(std::unique_ptr<TBufferFile>& msg)
{
	TLOG(5) << "Receiving Init Fragment from NetMonTransportService" ;
	TBufferFile* msg_ptr(nullptr);

	ServiceHandle<NetMonTransportService> transport;
	transport->receiveInitMessage(msg_ptr);

	msg.reset(msg_ptr);
	TLOG(5) << "Done Receiving Init Fragment from NetMonTransportService" ;
}