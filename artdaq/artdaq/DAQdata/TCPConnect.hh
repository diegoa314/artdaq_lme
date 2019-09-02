#ifndef TCPConnect_hh
#define TCPConnect_hh
#include <netinet/in.h>

// This file (TCPConnect.hh) was created by Ron Rechenmacher <ron@fnal.gov> on
// Sep 15, 2016. "TERMS AND CONDITIONS" governing this file are in the README
// or COPYING file. If you do not have such a file, one can be obtained by
// contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
// $RCSfile: .emacs.gnu,v $
// rev="$Revision: 1.30 $$Date: 2016/03/01 14:27:27 $";

/**
 * \file TCPConnect.hh
 * Provides utility functions for connecting TCP sockets
 */

/**
 * \brief Convert a string hostname to a in_addr suitable for socket communication
 * \param host_in Name or IP of host to resolve
 * \param[out] addr in_addr object populated with resolved host
 * \return 0 if success, -1 if gethostbyname fails
 */
int ResolveHost(char const* host_in, in_addr& addr);

/**
* \brief Get the IP address associated with a given interface name
* \param interface_name Name of the interface to resolve
* \param[out] addr in_addr object populated with interface IP
* \return 0 if success, -1 if gethostbyname fails, 2 if defaulted to 0.0.0.0 (No matching interfaces)
*/
int GetIPOfInterface(std::string interface_name, in_addr& addr);
/**
* \brief Pick a private IP address on this host
* \param[out] addr in_addr object populated with resolved host
* \return 0 if success, -1 if gethostbyname fails, 2 if defaulted to 0.0.0.0 (No matching interfaces)
*
* The following preference order is used:
* 1. 192.168.0.0/16
* 2. 172.16.0.0/12
* 3. 10.0.0.0/8
* 4. 131.225.0.0/16
* 5. 0.0.0.0 (returns 2)
*/
int AutodetectPrivateInterface(in_addr& addr);
/**
 * \brief Convert an IP address to the network address of the interface sharing the subnet mask
 * \param host_in IP to resolve
 * \param[out] addr in_addr object populated with resolved host
 * \return 0 if success, -1 if gethostbyname fails, 2 if defaulted to 0.0.0.0 (No matching interfaces)
 */
int GetInterfaceForNetwork(char const* host_in, in_addr& addr);
/**
 * \brief Convert a string hostname and port to a sockaddr_in suitable for socket communication
 * \param host_in Name or IP of host to resolve
 * \param dflt_port POrt to populate in output
 * \param[out] sin sockaddr_in object populated with resolved host and port
 * \return 0 if success, -1 if gethostbyname fails
 */
int ResolveHost(char const* host_in, int dflt_port, sockaddr_in& sin);
/**
 * \brief Connect to a host on a given port
 * \param host_in Name or IP of the host to connect to
 * \param dflt_port Port to connect to
 * \param flags TCP flags to use for the socket
 * \param sndbufsiz Size of the send buffer. Set to 0 for automatic send buffer management
 * \return File descriptor of connected socket.
 */
int TCPConnect(char const* host_in, int dflt_port, long flags = 0, int sndbufsiz = 0);

#endif	// TCPConnect_hh
