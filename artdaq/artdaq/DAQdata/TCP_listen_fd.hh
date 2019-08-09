#ifndef TCP_listen_fd_hh
#define TCP_listen_fd_hh

// This file (TCP_listen_fd.hh) was created by Ron Rechenmacher <ron@fnal.gov> on
// Sep 15, 2016. "TERMS AND CONDITIONS" governing this file are in the README
// or COPYING file. If you do not have such a file, one can be obtained by
// contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
// $RCSfile: .emacs.gnu,v $
// rev="$Revision: 1.30 $$Date: 2016/03/01 14:27:27 $";

/**
 * \file TCP_listen_fd.hh
 * Defines a generator function for a TCP listen socket
 */

/**
 * \brief Create a TCP listening socket on the given port and INADDR_ANY, with the given receive buffer
 * \param port Port to listen on
 * \param rcvbuf Receive buffer for socket. Set to 0 for TCP automatic buffer size
 * \return fd of new socket
 */
int TCP_listen_fd(int port, int rcvbuf);

#endif	// TCP_listen_fd_hh
