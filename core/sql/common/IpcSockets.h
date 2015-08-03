/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
#ifndef IPCSOCKETS_H
#define IPCSOCKETS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         IpcSockets.h
 * Description:  IPC code using the socket interface
 *
 * Created:      2/5/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "IpcMessageObj.h"

// -----------------------------------------------------------------------
// Contents of this file
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// Forward references
// -----------------------------------------------------------------------
class IpcEnvironment;
class IpcMessageBuffer;
struct sockaddr; // OS-specific

// -----------------------------------------------------------------------
// A typedef for a port number as defined in /etc/services and a literal
// to indicate an unspecified port number
// -----------------------------------------------------------------------
typedef ULng32 SockPortNumber;
const SockPortNumber NoSockPortNumber = 0;

// -----------------------------------------------------------------------
// An error number for a socket operation
// -----------------------------------------------------------------------

class SockErrNo
{
public:
  // initialize to a specific error number (values from errno.h)
  SockErrNo(Int32 e = 0) : errno_(e) {}

  // check whether an error is set
  inline NABoolean hasError() const                { return errno_ != 0; }

  // get the error number
  inline Int32 geterrno() const                           { return errno_; }

  // set the error number from the global variable "errno" or its
  // replacement in GUARDIAN
  Int32 setFromerrno();

  // clear the error
  inline void clear()                                      { errno_ = 0; }


private:
  Int32 errno_;
};

// -----------------------------------------------------------------------
// An Internet address of the form a.b.c.d (4 bytes).
// -----------------------------------------------------------------------
struct SockRawIPAddress
{
  // binary form of the address
  unsigned char ipAddress_[4];
};

class SockIPAddress
{
  friend class SockSocket;

public:
  SockIPAddress();
  SockIPAddress(const SockRawIPAddress a)               { a_ = a; }
  SockIPAddress(const struct sockaddr &);
  // set address from a node name, NULL means local host
  // resolves the host name and stores the result in the object
  SockErrNo set(const char *hostName = NULL);
  inline const SockRawIPAddress & getRawAddress() const     { return a_; }

private:
  // binary form of the address
  SockRawIPAddress a_;
};

// -----------------------------------------------------------------------
// Description of a service as maintained by inetd and /etc/services
// -----------------------------------------------------------------------
class SockService
{
public:
  SockService(const char *serviceName,
	      SockPortNumber defaultPortNumber = NoSockPortNumber);
  inline SockPortNumber getPortNumber() const         { return portNum_; }
  inline SockErrNo getError() const                 { return lastError_; }

private:
  // each service is assigned a port number which should be the same
  // for all nodes in the network
  SockPortNumber portNum_;
  SockErrNo lastError_;
};

// -----------------------------------------------------------------------
// The TCP protocol as defined in /etc/protocols (always 6)
// -----------------------------------------------------------------------
const Int32 SockTCPProtocol = 6;

// -----------------------------------------------------------------------
// A socket datatype (UNIX uses int for a socket)
// -----------------------------------------------------------------------
typedef Int32 SockFdesc;
const SockFdesc InvalidFdesc = -1;

const SockFdesc SockStdin = 0;
const SockFdesc SockStdout = 1;

class SockSocket
{
public:

  // default constructor for a TCP/IP socket (creates it)
  SockSocket(IpcEnvironment *env);

  // make a socket from an existing file descriptor or SockSocket
  SockSocket(SockFdesc fd,IpcEnvironment *env);

  // the destructor closes the socket
  ~SockSocket();

  // set this socket from an existing file descriptor, but only if there
  // is no real socket created yet for this object
  void setFdesc(SockFdesc fd)
                         { assert(fdesc_ == InvalidFdesc); fdesc_ = fd; }
  SockFdesc getFdesc() const                           { return fdesc_; }

  //   creates a duplicate handle for fdesc_
  SockFdesc getDuplicateFdesc_();
  // bind or connect the socket to the specified port on the specified
  // IP address
  inline SockPortNumber bind(const SockIPAddress &ipAddr,SockPortNumber port)
                              { return bindOrConnect(ipAddr,port,TRUE); }
  inline SockPortNumber connect(const SockIPAddress &ipAddr,
				SockPortNumber port)
                             { return bindOrConnect(ipAddr,port,FALSE); }

  // listen at the port
  SockPortNumber listen(SockPortNumber port = NoSockPortNumber);

  // receive a message from a newly created server what the listner port is
  SockPortNumber receiveListnerPortNum();

  // assign this socket to standard input/output and close the existing
  // file descriptor (this assignment stays beyond fork() and exec() calls)
  void assignToStdInOut();

  // try to send a message within the specified time interval (call has to be
  // repeated with the same parameters until it returns TRUE)
  NABoolean send(IpcMessageBuffer *message,
		 IpcTimeout timeout = IpcInfiniteTimeout);

  // Try to receive data within a given timeout period and create a
  // new buffer if none is passed in. The call returns FALSE if it times
  // out before an entire message could be received. It then has to be
  // retried, passing the same parameters in that were returned the
  // previous time. The returned message buffer (may be returned even
  // on a FALSE return value) is owned by the caller. Check the error
  // status of the socket (getError() and hasError() methods) after each call.
  // In an error case the method returns TRUE and sets the message length to 0.
  // If a buffer is passed in, it has to be long enough to hold the entire
  // message. The receive call interprets the incoming message header in
  // order to find out the message length.
  NABoolean receive(IpcMessageBuffer *&message,
		    IpcTimeout timeout);

  // Try to accept a new connection request from a client, try for
  // the given timeout. Return whether a new client request was accepted
  // and if the return value is TRUE, fill in the file descriptor of
  // the newly created socket.
  NABoolean accept(SockFdesc &fdesc, IpcTimeout timeout);

  // error information
  SockErrNo setFromerrno(const char *msg);
  SockErrNo getError() const                       { return lastError_; }
  NABoolean hasError() const            { return lastError_.hasError(); }

private:

  SockPortNumber bindOrConnect(const SockIPAddress &ipAddr,
			       SockPortNumber port,
			       NABoolean bindOnly);
  SockFdesc          fdesc_;
  SockErrNo          lastError_;

  // remember partially completed receive operations
  IpcMessageObjSize   expectedBytes_;
  IpcMessageObjSize   receivedBytesSoFar_;
  IpcMessageBuffer    *partiallyReceivedBuffer_;

  // environment information, needed to allocate memory from the heap
  IpcEnvironment *environment_;
};

#endif /* IPCSOCKETS_H */
