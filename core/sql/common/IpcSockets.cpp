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
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         IpcSockets.cpp
 * Description:  OS related code for socket-based IPC (see Ipc.h)
 *
 * Created:      10/6/95
 * Language:     C++
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "Platform.h"
#include "copyright.h"


// At this point we don't really want to use sockets on NSK because
// of problems with C/C++ runtime. Set this define to comment out all
// offending code (and maybe a little more than that)



// Uncomment the next line to debug IPC problems (log of client's I/O)
// #define LOG_IPC

#ifdef LOG_IPC
void IpcSockLogTimestamp(Int32 fdesc); // see bottom of file
#endif

#ifndef DISABLE_SOCKET_IPC
// for sockaddr struct and flags passed to socket system calls

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
const Int32 SOCKET_ERROR = -1;
typedef size_t length_t;

#include <errno.h>
#include <sys/time.h>

#include <iostream>
#include <iomanip>
#endif /* DISABLE_SOCKET_IPC */

#include "Collections.h"
#include "Ipc.h"
#include "ComDiags.h"
#include "str.h"

// comment this out once NAHeap.h is in common
// #include "NAHeap.h"
// for now declare overloaded operator new here
#pragma warning (disable : 4273)   //warning elimination
void * operator new(size_t size, CollHeap* h);
#pragma warning (default : 4273)   //warning elimination


#ifdef DISABLE_SOCKET_IPC

// -----------------------------------------------------------------------
// Stubs for configurations where socket IPC is disabled
// -----------------------------------------------------------------------

SockSocket::SockSocket(IpcEnvironment *env)
{
  ABORT("Stub for SockSocket::SockSocket()");
}

SockSocket::~SockSocket()
{
  ABORT("Stub for SockSocket::~SockSocket()");
}

SockIPAddress::SockIPAddress()
{
  ABORT("Stub for SockIPAddress::SockIPAddress()");
}

SockConnection::SockConnection(IpcEnvironment *env,
			       const IpcProcessId &serviceProcId,
			       NABoolean thisIsTheControlConnection,
                               const char *eye) :
     IpcConnection(env,serviceProcId,eye), sock_(env), ioq_(env->getHeap())
{
  ABORT("Stub for SockConnection::SockConnection()");
}

// also need to stub out all virtual methods of SockConnection,
// constructor needs them
SockConnection::~SockConnection()
{
  ABORT("Stub for SockConnection::~SockConnection()");
}

void SockConnection::send(IpcMessageBuffer *buffer)
{
  ABORT("Stub for void SockConnection::send(IpcMessageBuffer *buffer)");
}

void SockConnection::receive(IpcMessageStreamBase *msg)
{
  ABORT("Stub for void SockConnection::receive(IpcMessageStreamBase *msg)");
}

WaitReturnStatus SockConnection::wait(IpcTimeout timeout, UInt32 *eventConsumed, IpcAwaitiox *ipcAwaitiox)
{
  ABORT("Stub for short SockConnection::wait(IpcTimeout timeout)");
  return WAIT_INTERRUPT;
}

SockConnection * SockConnection::castToSockConnection()
{
  ABORT("Stub for SockConnection *SockConnection::castToSockConnection()");
  return NULL;
}

Int32 SockConnection::numQueuedSendMessages()
{
  ABORT("Stub for int SockConnection::numQueuedSendMessages()");
  return 0;
}

Int32 SockConnection::numQueuedReceiveMessages()
{
  ABORT("Stub for int SockConnection::numQueuedReceiveMessages()");
  return 0;
}

void SockConnection::populateDiagsArea(ComDiagsArea *&diags,
				       CollHeap *diagsHeap)
{
  ABORT("Stub for void SockConnection::populateDiagsArea()");
}

SockControlConnection::SockControlConnection(IpcEnvironment *env, const char *eye) :
     IpcControlConnection(IPC_DOM_INTERNET, eye), listnerSocket_(env)
{
  ABORT("Stub for SockControlConnection::SockControlConnection()");
}

IpcConnection * SockControlConnection::getConnection() const
{
  ABORT("Stub for SockControlConnection::getConnection()");
  return NULL;
}

SockControlConnection * SockControlConnection::castToSockControlConnection()
{
  ABORT("Stub for SockControlConnection::castToSockControlConnection()");
  return NULL;
}

void SockControlConnection::acceptNewConnectionRequest(SockConnection *conn)
{
  ABORT("Stub for SockControlConnection::acceptNewConnectionRequest()");
}

IpcConnection * IpcServerClass::createInternetProcess(
     ComDiagsArea **diags,
     CollHeap   *diagsHeap,
     const char *nodeName,
     const char *className,
     IpcCpuNum  /*cpuNum (sorry, no support for SMPs yet)*/,
     NABoolean  /*usesTransactions (sorry, no transactions in cyberspace*/,
     SockPortNumber defaultPortNumber)
{
  ABORT("Stub for IpcServerClass::createInternetProcess()");
  return NULL;
}

IpcConnection * IpcServerClass::forkProcess(ComDiagsArea **diags,
					    CollHeap   *diagsHeap,
					    const char * /*nodeName*/,
					    const char *className,
					    IpcCpuNum  /*cpuNum*/,
					    NABoolean  /*usesTransactions*/)
{
  ABORT("Stub for IpcServerClass::forkProcess()");
  return NULL;
}

SockErrNo SockIPAddress::set(const char *hostName)
{
  ABORT("Stub for SockIPAddress::set()");
  return 0;
}


#else

// -----------------------------------------------------------------------
// The REAL methods
// -----------------------------------------------------------------------


// -----------------------------------------------------------------------
// Methods for class SockErrNo
// -----------------------------------------------------------------------
Int32 SockErrNo::setFromerrno()
{
#ifdef NA_ERRNO_AS_PROCEDURE
  ... insert code for GUARDIAN style errno here
#else
  errno_ = errno;
#endif
  return errno_;
}

// -----------------------------------------------------------------------
// Methods for class SockIPAddress
// -----------------------------------------------------------------------

SockIPAddress::SockIPAddress()
{
  // IP address 0.0.0.0 is an invalid (wildcard) IP address
  a_.ipAddress_[0] = 0;
  a_.ipAddress_[1] = 0;
  a_.ipAddress_[2] = 0;
  a_.ipAddress_[3] = 0;
}

SockIPAddress::SockIPAddress(const struct sockaddr &sa)
{
  a_.ipAddress_[0] = sa.sa_data[2];
  a_.ipAddress_[1] = sa.sa_data[3];
  a_.ipAddress_[2] = sa.sa_data[4];
  a_.ipAddress_[3] = sa.sa_data[5];
}

SockErrNo SockIPAddress::set(const char *hostName)
{
  SockErrNo result;

  struct hostent *hostEnt = NULL;

  // initialize the address to something
  a_.ipAddress_[0] = 0;
  a_.ipAddress_[1] = 0;
  a_.ipAddress_[2] = 0;
  a_.ipAddress_[3] = 0;

  if (hostName != NULL)
    hostEnt = gethostbyname(hostName);
  else
    {
      char me[IpcNodeNameMaxLength];

      if (gethostname(me,IpcNodeNameMaxLength) < 0)
	result.setFromerrno();
      else
	hostEnt = gethostbyname(me);
    }

  // could use errno_h to get better error reporting
  if (hostEnt == NULL)
    result = ENOENT;

  if (NOT result.hasError())
    {
      // hostEnt->h_addr_list points to an array of pointers to addresses,
      // where each address is an array of chars (in network order)
      char *addr = hostEnt->h_addr_list[0];

      if (addr == NULL)
	  result = ENXIO; // addr pointer was bad
      else
	{
	  a_.ipAddress_[0] = addr[0];
	  a_.ipAddress_[1] = addr[1];
	  a_.ipAddress_[2] = addr[2];
	  a_.ipAddress_[3] = addr[3];
	}
    }

  return result;
}

// -----------------------------------------------------------------------
// Methods for class SockService
// -----------------------------------------------------------------------

SockService::SockService(const char *serviceName,
			 SockPortNumber defaultPortNumber)
{
  struct servent *servEnt;

  servEnt = getservbyname(serviceName,NULL);
  if (servEnt == NULL)
    {
      // Service not found, using default port number
      assert(defaultPortNumber != NoSockPortNumber);
      portNum_ = defaultPortNumber;
    }
  else
    portNum_ = servEnt->s_port;
  // don't delete servEnt, it is owned by the system
}

// -----------------------------------------------------------------------
// Methods for class SockSocket
// -----------------------------------------------------------------------

SockSocket::SockSocket(IpcEnvironment *env)
{
  environment_             = env;
  expectedBytes_           =
  receivedBytesSoFar_      = 0;
  partiallyReceivedBuffer_ = NULL;

  // create an unbound socket for the TCP/IP protocol
  fdesc_ = socket(AF_INET,          // domain = IP
		  SOCK_STREAM,      // type = stream
		  SockTCPProtocol); // protocol = TCP
  if (fdesc_ < 0)
    setFromerrno("creating an unbound socket with socket()");
}

SockSocket::SockSocket(SockFdesc fd, IpcEnvironment *env)
{
  environment_             = env;
  expectedBytes_           =
  receivedBytesSoFar_      = 0;
  partiallyReceivedBuffer_ = NULL;

  fdesc_ = fd;
}


SockSocket::~SockSocket()
{
  if (fdesc_ >= 0)
   close(fdesc_);
}

SockPortNumber SockSocket::bindOrConnect(const SockIPAddress &ipAddr,
					 SockPortNumber port,
					 NABoolean bindOnly)
{
  struct sockaddr targetSockAddr;
  SockPortNumber result = port;
  Int32 retcode;

  lastError_.clear();

  // doing everything byte by byte takes care of endianness problems
  // (just in case you don't like this code)
  targetSockAddr.sa_family  = AF_INET;
  targetSockAddr.sa_data[0] = ((UInt32)port) / 256;
  targetSockAddr.sa_data[1] = ((UInt32)port) % 256;
  targetSockAddr.sa_data[2] = ipAddr.a_.ipAddress_[0];
  targetSockAddr.sa_data[3] = ipAddr.a_.ipAddress_[1];
  targetSockAddr.sa_data[4] = ipAddr.a_.ipAddress_[2];
  targetSockAddr.sa_data[5] = ipAddr.a_.ipAddress_[3];
  for (Int32 i = 6; i < 14; i++)
    targetSockAddr.sa_data[i] = 0;

  // this code depends on the definition of sockaddr, make sure
  // we don't do the wrong thing
  assert(sizeof(targetSockAddr) == 16);

  // if we want to accept incoming connections, then the bind system
  // call is the right thing to do now, if we want to connect to a server
  // then the connect system call is what we want
  if (bindOnly)
    {
      // the bind call just binds the socket to the specified port,
      // it doesn't wait for a connection to come in (see listen and accept)

      // NOTE: we want to call the system calls bind and connect, not
      // the member functions with the same names
      // NOTE: the port number of the socket gets altered by the call
      retcode = ::bind(fdesc_,&targetSockAddr,sizeof(targetSockAddr));
    }
  else
    {
      // the connect system call actually starts up a connection; once
      // the call succeeds we can send and receive
      // NOTE: the port number of the socket gets altered by the call
      retcode = ::connect(fdesc_,&targetSockAddr,sizeof(targetSockAddr));
    }

	 if (retcode == SOCKET_ERROR)
	  setFromerrno("::bind() or ::connect()");

  // return the port that the socket is now bound to

  socklen_t namelen = sizeof(targetSockAddr);
  retcode = ::getsockname(fdesc_,&targetSockAddr,&namelen);
  if (retcode == SOCKET_ERROR)
    setFromerrno("::getsockname()");

  return (unsigned char)targetSockAddr.sa_data[0] * 256 + (unsigned char)targetSockAddr.sa_data[1];

}

SockPortNumber SockSocket::listen(SockPortNumber port)
{
  lastError_.clear();

  // bind the socket to the given port number or, if NoSockPortNumber is
  // specified, to any port
  SockPortNumber result = bind(SockIPAddress(),port);
  if (NOT lastError_.hasError())
    {
      // prepare the socket for ::accept() calls by calling ::listen()
      if (::listen(fdesc_,5) < 0)

	setFromerrno("::listen()");
    }
  else
    {
      result = NoSockPortNumber;
    }
  return result;
}

SockPortNumber SockSocket::receiveListnerPortNum()
{
  // read the message "Listening to port xxxxxxxxxxx\n" from the
  // socket to the newly created server and decode the xxxxxxxxxxx to
  // ascii

  // Listening to port xxxxxxxxxxx\n
  // 012345678901234567890123456789

  const Int32 numCharsInListnerMsg = 30;
  const Int32 numCharsInPrefix = 18;

  char listnerMsg[numCharsInListnerMsg];

  // wait for the server to send the port number and read it into a buffer
  Int32 receivedBytes = recv(fdesc_,listnerMsg,numCharsInListnerMsg,0);

  // make sure we got the right message
  short temp = str_cmp(listnerMsg,"Listening to port ",numCharsInPrefix);
  assert(receivedBytes == numCharsInListnerMsg AND
	 temp == 0);

  // decode the ascii number
  SockPortNumber result = atoi(&listnerMsg[numCharsInPrefix]);

# ifdef LOG_IPC
  IpcSockLogTimestamp(fdesc_);
  cerr << "server listens to port " << result << endl;
# endif

  return result;
}

void SockSocket::assignToStdInOut()
{
  lastError_.clear();

  // assign the socket to stdin
 ABORT ("Not Implemented or Needed in current milestone");

  // now set the new file descriptor number in the
  // object to stdin (Note: server program MUST NOT use stdin
  // for other purposes than IPC procedures)
  fdesc_ = SockStdin;
# ifdef LOG_IPC
  IpcSockLogTimestamp(SockStdin);
  cerr << "Assigned stdin to client conn." << endl;
# endif
}

NABoolean SockSocket::send(IpcMessageBuffer *message,
			   IpcTimeout timeout)
{
  IpcMessageObjSize bytesToSend = message->getMessageLength();
  IpcMessageObjSize bytesSentSoFar = 0;
  Lng32              bytesSentThisTime;

  lastError_.clear();

  // ---------------------------------------------------------------------
  // If a timeout is specified, do a select first before receiving data
  // NOTE: we wait <timeout> 10ms intervals each time we go through
  // the loop, although we should wait only a total or <timeout> intervals
  // (usually, once a message head is coming in, the tail is not far away)
  // ---------------------------------------------------------------------
  if (timeout != IpcInfiniteTimeout)
    {
      // two structs from sys/time.h and sys/types.h to describe the
      // timeout in seconds/microseconds to wait and a bitmap of file
      // descriptors
      timeval timeOutVal;
      fd_set readyDescriptors;

      // make a bitmap with the bit for fdesc_ set to 1
      FD_ZERO(&readyDescriptors);
      FD_SET(fdesc_,&readyDescriptors);

      // store the timeout value in timeOutVal
      timeOutVal.tv_sec = timeout / 100;
      timeOutVal.tv_usec = timeout % 100;

      Int32 retcode =
	select(FD_SETSIZE,NULL,&readyDescriptors,NULL,&timeOutVal);

      if (retcode < 0)
	{
	  // indicate an error by returning TRUE and setting the error status
	  setFromerrno("::select() for send");
	  return TRUE;
	}

      if (retcode == 0)
	{
	  // the timeout expired, return without receiving something
	  return FALSE;
	}
    } // non-infinite timeout

  // ---------------------------------------------------------------------
  // call ::send until all of the data is sent (do it waited from here)
  // ---------------------------------------------------------------------
  while (bytesToSend > 0 AND NOT lastError_.hasError())
    {
      bytesSentThisTime = ::send(fdesc_,
				 message->data(bytesSentSoFar),
				 (Int32) bytesToSend,
				 0);
      if (bytesSentThisTime <= 0)
	{
	  setFromerrno("::send()");
	}
      else
	{
	  // prepare for sending the next chunk or for leaving the loop
	  bytesToSend -= bytesSentThisTime;
	  bytesSentSoFar += (IpcMessageObjSize) bytesSentThisTime;
	}
    }

# ifdef LOG_IPC
  IpcSockLogTimestamp(fdesc_);
  IpcMessageObj *firstUserObj = (IpcMessageObj *)
    (message->data(sizeof(InternalMsgHdrInfoStruct)));
  cerr << "sent " << bytesSentSoFar << " bytes, objtype "
       << firstUserObj->getType() << endl;
# endif

  // we did send it
  return TRUE;
}

NABoolean SockSocket::receive(IpcMessageBuffer * &message,
			      IpcTimeout timeout)
{
  // receive buffer for socket (may be stack variable or alloc. buffer)
  IpcMessageBufferPtr sockReceiveBuffer;

  // copy of the message header on the stack
  char header[sizeof(InternalMsgHdrInfoStruct)];
  // variable that remembers how many bytes came in the message
  Lng32 receivedBytesThisTime;

  // the actual message length (initialize to header length until we know it)
  if (receivedBytesSoFar_ == 0)
    {
      // start over with a fresh receive operation
      expectedBytes_ = sizeof(InternalMsgHdrInfoStruct);
    }

  // the user may specify a special receive buffer, but not multiple
  // receive buffers for the same message
  if (partiallyReceivedBuffer_ == NULL AND message != NULL)
    partiallyReceivedBuffer_ = message;
  assert(message == NULL OR partiallyReceivedBuffer_ == message);

  // if no buffer is allocated by the caller, use the on-stack header
  // for the initial chunk
  if (partiallyReceivedBuffer_)
    sockReceiveBuffer = partiallyReceivedBuffer_->data();
  else
    sockReceiveBuffer = header;

  // each call clears the error info and a length is only returned when
  // the message has arrived completely and the return value is TRUE
  lastError_.clear();

  // ---------------------------------------------------------------------
  // Keep on receiving in a loop until we  have read all of the message
  // (and no more than the message).
  // ---------------------------------------------------------------------

  while (expectedBytes_ > receivedBytesSoFar_ AND NOT lastError_.hasError())
    {

      // ---------------------------------------------------------------------
      // If a timeout is specified, do a select first before receiving data
      // NOTE: we wait <timeout> 10ms intervals each time we go through
      // the loop, although we should wait only a total or <timeout> intervals
      // (usually, once a message head is coming in, the tail is not far away)
      // ---------------------------------------------------------------------
      if (timeout != IpcInfiniteTimeout)
	{
	  // two structs from sys/time.h and sys/types.h to describe the
	  // timeout in seconds/microseconds to wait and a bitmap of file
	  // descriptors
	  timeval timeOutVal;
	  fd_set readyDescriptors;

	  // make a bitmap with the bit for fdesc_ set to 1
	  FD_ZERO(&readyDescriptors);
	  FD_SET(fdesc_,&readyDescriptors);

	  // store the timeout value in timeOutVal
	  timeOutVal.tv_sec = timeout / 100;
	  timeOutVal.tv_usec = timeout % 100;

	  timeval *tt = (timeval*)&timeOutVal;
	  Int32 retcode =
	    select(FD_SETSIZE,&readyDescriptors,NULL,NULL,tt);

	  if (retcode < 0)
	    {
	      // indicate an error by returning TRUE and length 0
	      setFromerrno("::select for receive");
	      // indicate the error by returning a zero-length message
	      if (partiallyReceivedBuffer_ != NULL)
		{
		  partiallyReceivedBuffer_->setMessageLength(0);
		  message = partiallyReceivedBuffer_;
		}
	      partiallyReceivedBuffer_ = NULL;
	      expectedBytes_ = 0;
	      receivedBytesSoFar_ = 0;
	      return TRUE;
	    }

	  if (retcode == 0)
	    {
	      // the timeout expired, return without receiving something
	      return FALSE;
	    }
	} // non-infinite timeout

      receivedBytesThisTime = recv(
	   fdesc_,
	   &sockReceiveBuffer[receivedBytesSoFar_],
	   (Int32) (expectedBytes_ - receivedBytesSoFar_),
	   0);

      if (receivedBytesThisTime <= 0)
	{
	  setFromerrno("::recv()");
	}
      else
	{
	  // if we completely received the header then increase the expected
	  // message length to the actual value sent in the header
	  if (receivedBytesSoFar_ + receivedBytesThisTime ==
	      sizeof(InternalMsgHdrInfoStruct))
	    {
	      // a pointer to the message header
	      InternalMsgHdrInfoStruct *headerPtr =
		(InternalMsgHdrInfoStruct *) sockReceiveBuffer;

#ifdef NA_LITTLE_ENDIAN
	      // need to turn around bytes on a little-endian machine,
	      // since the header info is always sent in big-endian format
	      headerPtr->turnByteOrder();
#endif

	      // read the total message length out of the received header
	      expectedBytes_ = headerPtr->totalLength_;

	      // allocate a message buffer of maxLength unless one was
	      // provided already by the caller (caller becomes new owner
	      // of the buffer)
	      if (sockReceiveBuffer == header)
		{
                  CollHeap *heap =
                    (environment_ ? environment_->getHeap() : NULL);

		  // we used the provisional buffer for the receive,
		  // go allocate the real buffer with the correct length
		  partiallyReceivedBuffer_ = IpcMessageBuffer::allocate(
		       expectedBytes_,
		       NULL, // don't know the IpcMessageStream object here
		       heap,
                       0);

		  // the actual receive buffer starts a few bytes further up
		  sockReceiveBuffer = partiallyReceivedBuffer_->data();

		  // copy the received header into the allocated buffer
		  str_cpy_all(sockReceiveBuffer,
			      header,
			      sizeof(InternalMsgHdrInfoStruct));
		}
	      else
		{
		  // if a buffer was supplied then it better be big enough
		  assert(partiallyReceivedBuffer_->getBufferLength() >=
			 expectedBytes_);
		}
	    } // message header received

	  // prepare for receiving the next chunk or for leaving the loop
	  receivedBytesSoFar_ += receivedBytesThisTime;
	} // no receive error
    } // while expectedBytes_ > receivedBytesSoFar_

  // at this point we are done with a message or we got a bad error
  if (lastError_.hasError())
    {
      // indicate the error by returning a zero-length message
      if (partiallyReceivedBuffer_ != NULL)
	partiallyReceivedBuffer_->setMessageLength(0);
    }
  else
    {
      // set message length in the buffer
      partiallyReceivedBuffer_->setMessageLength(expectedBytes_);
    }

# ifdef LOG_IPC
  IpcSockLogTimestamp(fdesc_);
  IpcMessageObj *firstUserObj = (IpcMessageObj *)
    (partiallyReceivedBuffer_->data(sizeof(InternalMsgHdrInfoStruct)));
  cerr << "received " << receivedBytesSoFar_ << " bytes, objtype "
       << firstUserObj->getType() << endl;
# endif

  // indicate we are ready for the next message
  expectedBytes_           = 0;
  receivedBytesSoFar_      = 0;
  message                  = partiallyReceivedBuffer_;
  partiallyReceivedBuffer_ = NULL;

  return TRUE;
}

NABoolean SockSocket::accept(SockFdesc &fdesc, IpcTimeout timeout)
{
  // ---------------------------------------------------------------------
  // If a timeout is specified, do a select first before receiving data
  // NOTE: we wait <timeout> 10ms intervals each time we go through
  // the loop, although we should wait only a total or <timeout> intervals
  // (usually, once a message head is coming in, the tail is not far away)
  // ---------------------------------------------------------------------
  if (timeout != IpcInfiniteTimeout)
    {
      // two structs from sys/time.h and sys/types.h to describe the
      // timeout in seconds/microseconds to wait and a bitmap of file
      // descriptors
      timeval timeOutVal;
      fd_set readyDescriptors;

      // make a bitmap with the bit for fdesc_ set to 1
      FD_ZERO(&readyDescriptors);
      FD_SET(fdesc_,&readyDescriptors);

      // store the timeout value in timeOutVal
      timeOutVal.tv_sec = timeout / 100;
      timeOutVal.tv_usec = timeout % 100;

      Int32 retcode =
	select(FD_SETSIZE,&readyDescriptors,NULL,NULL,&timeOutVal);

      if (retcode < 0)
	{
	  // indicate an error by returning TRUE and setting the error
	  setFromerrno("::select for accept");
	  fdesc = InvalidFdesc;
	  return TRUE;
	}

      if (retcode == 0)
	{
	  // the timeout expired, return without receiving something
	  return FALSE;
	}
    } // non-infinite timeout

  struct sockaddr clientSockAddr;
  socklen_t addrlen = sizeof(clientSockAddr);

  // call the accept() system call, waiting until a client wants to connect
  Int32 retcode = ::accept(fdesc_,&clientSockAddr,&addrlen);

  if (retcode >= 0)
    {
      // for successful calls, accept() returns the file descriptor
      // of a newly created socket that can be used to communicate to
      // the client (the existing socket continues to be the lisner
      // port, no real messages are ever sent on it)
      fdesc = retcode;

#     ifdef LOG_IPC
      IpcSockLogTimestamp(fdesc);

      SockIPAddress clientIPAddr(clientSockAddr);
      SockPortNumber clientPort = clientSockAddr.sa_data[0] * 256 +
	clientSockAddr.sa_data[1];
      IpcProcessId clientPid(clientIPAddr,clientPort);
      char clientPidAsString[200];

      clientPid.toAscii(clientPidAsString,200);
      cerr << "accepted connection from " << clientPidAsString << endl;
#     endif

    }
  else
    {
      // a negative returncode indicates an error
      fdesc = InvalidFdesc;
      setFromerrno("::accept()");
    }

  // something happened, either an error or a new client request
  return TRUE;
}

SockErrNo SockSocket::setFromerrno(const char *msg)
{
  lastError_.setFromerrno();

# ifdef LOG_IPC
  IpcSockLogTimestamp(fdesc_);
  cerr << "socket error " << lastError_.geterrno() << ", msg: " << msg << endl;
# endif

  return lastError_;
}

// -----------------------------------------------------------------------
//  Methods for class SockConnection
// -----------------------------------------------------------------------

SockConnection::SockConnection(IpcEnvironment *env,
			       const IpcProcessId &serviceProcId,
			       NABoolean thisIsTheControlConnection,
                               const char *eye) :
     IpcConnection(env,serviceProcId, eye), sock_(env),
     isClient_(TRUE), lastReplyTag_(0), ioq_(env->getHeap())
{
  port_ = sock_.connect(serviceProcId.getIPAddress(),
			serviceProcId.getPortNumber());

  if (thisIsTheControlConnection)
    {
      // The process id that was passed in specified a service for
      // inetd. A new process got created and opened a new port on
      // which it listens for new requests. Catch the message of
      // the new process where it announces its listner port and
      // set that listner port in the server process id. This makes
      // the server process id unique and allows other processes to
      // connect to the server by calling the same constructor, but
      // with the thisIsTheControlConnection parameter set to FALSE.
      setOtherEnd(IpcProcessId(serviceProcId.getIPAddress(),
			       sock_.receiveListnerPortNum()));
    }
  else
    {
      // we already got the server process id with the listner port
      // set; no new process got created, use the original process id
    }

  if (NOT sock_.hasError())
    {
      // a connection was established to the specified port,
      // this is the client side
      setState(ESTABLISHED);

#     ifdef LOG_IPC
      IpcSockLogTimestamp(sock_.getFdesc());
      char serverPidAsString[200];

      serviceProcId.toAscii(serverPidAsString,200);
      cerr << "connected to server " << serverPidAsString
	   << " on port " << port_ << endl;
#     endif

    }
  else
    {
      // it blew
      setErrorInfo(sock_.getError().geterrno());
      setState(ERROR_STATE);
    }
}

SockConnection::SockConnection(
     IpcEnvironment *env,
     SockFdesc fdesc,
     NABoolean isClient,
     const char *eye)    : IpcConnection(env,IpcProcessId(), eye),
			   sock_(fdesc,env), port_(NoSockPortNumber),
			   isClient_(isClient), lastReplyTag_(0),
			   ioq_(env->getHeap())
{
  if (fdesc != InvalidFdesc)
    {
      if (isClient)
	setState(ESTABLISHED);    // client may send right now
      else
	setState(REPLY_PENDING);  // sender needs to wait for client's message
    }
}

SockConnection::~SockConnection()
{
  // deallocate ioq entries
  for (Int32 i = 0; i < (Int32) ioq_.entries(); i++)
    {
      getEnvironment()->getHeap()->deallocateMemory(ioq_[i]);
    }
}

void SockConnection::send(IpcMessageBuffer *buffer)
{
  socketIOQueueEntry *sendEntry = NULL;
  short replyTag;

  if (isClient_)
    {
      // if the client sends, add a new entry to the I/O queue since
      // the send operation starts a new round trip

      sendEntry = new(getEnvironment()->getHeap()) socketIOQueueEntry;
      ioq_.insert(sendEntry);

      // increment reply tag
      // (wrap around at some arbitrary number that is large enough)
      lastReplyTag_++;
      if (lastReplyTag_ > 4711)
	lastReplyTag_ = 0;
      replyTag = lastReplyTag_;

      sendEntry->msg_        = buffer->getMessageStream();
      sendEntry->recvBuffer_ = NULL;
      sendEntry->replyTag_   = lastReplyTag_;
      sendEntry->receiving_  = FALSE;
    }
  else
    {
      // for the server, a send operation is actually a reply and it
      // completes the operation; find the request with the correct
      // reply tag

      replyTag = buffer->getReplyTag();

      // find the correct reply tag in the I/O queue
      for (Int32 i = 0; i < (Int32) ioq_.entries() AND sendEntry == NULL; i++)
	{
	  if (ioq_[i]->replyTag_ == replyTag)
	    sendEntry = ioq_[i];
	}
      assert(sendEntry AND sendEntry->recvBuffer_ == NULL AND
	     NOT sendEntry->receiving_);
    }

  // Put the reply tag into the message itself. A little note: there are
  // three places where we store reply tags: in the I/O queue entry,
  // in the IpcMessageBuffer header (the part that doesn't get sent in
  // a message) and in the message header that does get sent. In NSK there
  // is no need to send a reply tag, it is maintained by NSK, so we
  // need only two places in NSK. We always keep the IpcMessageBuffer
  // reply tag consistent with the one in the message header, and we
  // use the reply tag to find the correct I/O queue slot when we
  // receive in the client or when we send (reply) in the server. For
  // a socket connection, a reply tag is generated when the client sends,
  // it is then sent to the server and finally sent back to the client
  // where it is used to match the reply with the original send.

  ((InternalMsgHdrInfoStruct *) buffer->data())->sockReplyTag_ = replyTag;

  // set I/O queue entry
  sendEntry->sent_       = FALSE;
  sendEntry->sendBuffer_ = buffer;

  // indicate that I/O operations are pending, connection is active
  setState(SENDING);

  // try to send this message or another, earlier, message
  tryToSendMore();

  // NOTE: send queue of the base class is not used
}

void SockConnection::receive(IpcMessageStreamBase *msg)
{
  socketIOQueueEntry *receiveEntry = NULL;

  if (isClient_)
    {
      // in the client, find the send I/O that was issued by this message
      // stream
      for (Int32 i = 0; i < (Int32)ioq_.entries() AND receiveEntry == NULL; i++)
	{
	  if (ioq_[i]->msg_ == msg)
	    receiveEntry = ioq_[i];
	}
      // make sure there is an entry that has been sent already
      assert(receiveEntry AND receiveEntry->sent_ AND
	     NOT receiveEntry->receiving_ AND
	     receiveEntry->sendBuffer_ == NULL);
    }
  else
    {
      // in the server, add a new entry to the I/O queue
      receiveEntry = new(getEnvironment()->getHeap()) socketIOQueueEntry;
      ioq_.insert(receiveEntry);

      // set IO queue entry
      receiveEntry->sent_       = FALSE;
      receiveEntry->msg_        = msg;
      receiveEntry->sendBuffer_ = NULL;
      receiveEntry->recvBuffer_ = NULL;
    }

  // set rest of IO queue entry
  receiveEntry->receiving_ = TRUE;

  setState(RECEIVING);

  // we are now at a state where wait() will be able to complete the
  // receive I/O
}

WaitReturnStatus SockConnection::wait(IpcTimeout timeout, UInt32 *eventConsumed, IpcAwaitiox *ipcAwaitiox)
{
  // try to send some more messages
  tryToSendMore(); // $$$$ use of zero timeout is not ideal

  // loop over the I/O queue and try to receive some messages and try
  // to call callbacks
  for (Int32 i = 0; i < (Int32)ioq_.entries(); i++)
    {
      socketIOQueueEntry *e = ioq_[i];

      if (e->receiving_)
	{
	  // this is an entry that is receiving, try to get a message
	  // buffer from the socket (but in the client this will not
	  // necessarily be the one that is associated with this IO
	  // queue entry)
	  IpcMessageBuffer *buffer = NULL;
	  NABoolean completed = sock_.receive(buffer,timeout);

	  if (sock_.hasError())
	    {
	      // it blew
	      setErrorInfo(sock_.getError().geterrno());
	      setState(ERROR_STATE);
	      ABORT("Can't handle this error via ComDiags");
	    }

	  if (completed)
	    {
	      // find the IO queue entry that is associated with the
	      // received buffer

	      // get the sent reply tag from the message header
	      short replyTag =
		((InternalMsgHdrInfoStruct *) buffer->data())->sockReplyTag_;

	      if (isClient_)
		{
		  // in the client, use the reply tag to find the associated
		  // entry
		  NABoolean found = FALSE;
		  for (Int32 j = 0; j < (Int32)ioq_.entries() AND NOT found; j++)
		    {
		      if (ioq_[j]->replyTag_ == replyTag)
			{
			  e = ioq_[j];
			  found = TRUE;
			}
		    }
		  assert(found AND e->sent_ AND
			 e->recvBuffer_ == NULL);
		}
	      else
		{
		  // in the server the buffer is matched with the current
		  // I/O queue entry; we know it is in the receiving state
		  assert(e->receiving_ AND
			 NOT e->sent_ AND
			 e->sendBuffer_ == NULL AND
			 e->recvBuffer_ == NULL);

		  // store the sent reply tag with the buffer and in the IOQ
		  // (IpcMessageStream will maintain it there and
		  // SocketConnection::send() will match in up later)
		  buffer->setReplyTag(replyTag);
		  e->replyTag_ = replyTag;
		}

	      // e now has a buffer associated with it
	      e->recvBuffer_ = buffer;
	    }

	  // let the timeout expire only once
	  timeout = IpcImmediately;
	}

      // if a message was sent/received and the callback wasn't called yet
      // then do this now
      if (e->sent_ AND e->sendBuffer_)
	{
	  // can call a send callback
	  // save the message buffer on the stack and reset the IO queue entry
	  IpcMessageBuffer *buffer = e->sendBuffer_;

	  if (isClient_)
	    {
	      // setting the buffer pointer back to NULL indicates that
	      // the callback has been called
	      e->sendBuffer_ = NULL;
	    }
	  else
	    {
	      // in the server, calling the send callback is the last
	      // thing to do in the request-reply cycle
	      ioq_.removeAt(i);

	      // if this was the last active I/O then set state to inactive
	      if (ioq_.entries() == 0)
		setState(REPLY_PENDING);
	    }

#         ifdef LOG_IPC
	  IpcSockLogTimestamp(sock_.getFdesc());
	  cerr << "calling send callback" << endl;
#         endif

	  // finally, call the callback, which may have a lot of side-effects
	  buffer->callSendCallback(this);
	  buffer->decrRefCount();

	  // bail out, don't trust environment after calling a callback
	  return WAIT_OK;
	}
      else if (e->receiving_ AND e->recvBuffer_)
	{
	  // can call a receive callback
	  // save the message buffer on the stack and reset the IO queue entry
	  IpcMessageBuffer *buffer = e->recvBuffer_;

	  if (isClient_)
	    {
	      // finished send/receive cycle if receive callback is called
	      // in the client
	      ioq_.removeAt(i);

	      // if this was the last active I/O then set state to inactive
	      if (ioq_.entries() == 0)
		setState(ESTABLISHED);
	    }
	  else
	    {
	      // in the server we'll keep the I/O queue entry until the
	      // reply comes
	      e->recvBuffer_ = NULL;
	      e->receiving_ = FALSE;
	    }

#         ifdef LOG_IPC
	  IpcSockLogTimestamp(sock_.getFdesc());
	  cerr << "calling receive callback" << endl;
#         endif

	  // finally, call the callback, which may have a lot of side-effects
	  buffer->addCallback(e->msg_);
	  buffer->callReceiveCallback(this);

	  // bail out, don't trust environment after calling a callback
	  return WAIT_OK;
	}
    } // for each ioq_ entry

  return WAIT_OK;
}

SockConnection * SockConnection::castToSockConnection()
{
  return this;
}

Int32 SockConnection::numQueuedSendMessages()
{
  // the current implementation assumes that no messages can
  // be queued and that only one message can be sent nowait
  assert(sendQueueEntries() == 0 OR
	 sendQueueEntries() == 1 AND sendIOPending());
  return sendQueueEntries();
}

Int32 SockConnection::numQueuedReceiveMessages()
{
  // the current implementation assumes that no messages can
  // be queued and that only one message can be received nowait
  assert(receiveQueueEntries() == 0 OR
	 receiveQueueEntries() == 1 AND receiveIOPending());
  return receiveQueueEntries();
}

void SockConnection::populateDiagsArea(ComDiagsArea *&diags,
				       CollHeap *diagsHeap)
{
  if (sock_.hasError())
    {
      IpcAllocateDiagsArea(diags,diagsHeap);

      *diags << DgSqlCode(-2035) << DgInt0(sock_.getError().geterrno());
      getEnvironment()->getMyOwnProcessId(IPC_DOM_INTERNET).
	addProcIdToDiagsArea(*diags,0);
      getOtherEnd().addProcIdToDiagsArea(*diags,1);
    }
}

void SockConnection::setFdesc(SockFdesc fdesc, NABoolean isClient)
{
  assert(getState() == INITIAL);
  sock_.setFdesc(fdesc);
  if (isClient)
    setState(ESTABLISHED);
  else
    setState(REPLY_PENDING);
}

SockPortNumber SockConnection::connect(const SockIPAddress &ipAddr, SockPortNumber port)
{ SockPortNumber port_ = sock_.connect(ipAddr,port);
  setOtherEnd(IpcProcessId(ipAddr,sock_.receiveListnerPortNum()));  // (3/6/97)
  if (NOT sock_.hasError())
   {
     // a connection was established to the specified port,
     // this is the client side
     setState(ESTABLISHED);
   }
  else
   {
     // it blew
     setErrorInfo(sock_.getError().geterrno());
     setState(ERROR_STATE);
   };
  return port_;
}

void SockConnection::tryToSendMore()
{
  // while there are unsent entries left, try to send them if the socket
  // is ready
  for (Int32 i = 0; i < (Int32) ioq_.entries(); i++)
    {
      // can we send the message in this io queue entry?
      if (NOT ioq_[i]->sent_ AND ioq_[i]->sendBuffer_)
	{
	  // try to send the next message

	  if (sock_.send(ioq_[i]->sendBuffer_,IpcImmediately))
	    {
	      // message has been sent or an error has occurred during send
	      ioq_[i]->sent_ = TRUE;

	      if (sock_.hasError())
		{
		  // it blew
		  setErrorInfo(sock_.getError().geterrno());
		  setState(ERROR_STATE);
		  return;
		}
	    }
	  else
	    return; // socket is busy, don't try another one
	}
    }
}

// -----------------------------------------------------------------------
//  Methods for class SockPairConnection
// -----------------------------------------------------------------------

SockPairConnection::SockPairConnection(
     IpcEnvironment *env, const char *eye) :
  SockConnection(env,InvalidFdesc,TRUE,eye)
{
ABORT ("Not Implemented milestone 1");

}


SockPairConnection::SockPairConnection(
     IpcEnvironment *env, SockFdesc fd) : SockConnection(env,fd,FALSE)
{
  otherEnd_ = NULL;
}

SockPairConnection::~SockPairConnection() {}

SockPairConnection *SockPairConnection::otherEnd()
{
  // make sure to delete the pointer to the other end before returning
  // it, since the other end of the connection will go to a different process
  SockPairConnection *temp = otherEnd_;

  otherEnd_ = NULL;
  return temp;
}

void SockPairConnection::doConnectNow()
  {port_ = connect(ipAddr_,port_);
  }



// -----------------------------------------------------------------------
// Methods for class SockListnerPort
// -----------------------------------------------------------------------

SockListnerPort::SockListnerPort(IpcEnvironment *env,
				 SockFdesc fdesc,
				 SockControlConnection *cc,
                                 const char *eye) :
     SockConnection(env,fdesc,FALSE,eye)
{
  cc_ = cc;
  // we should always be listening to new requests, the state never changes
  setState(RECEIVING);
}

void SockListnerPort::send(IpcMessageBuffer *)
{
  ABORT("SockListnerPort::send()");
}

void SockListnerPort::receive(IpcMessageStreamBase *)
{
  ABORT("SockListnerPort::receive()");
}

WaitReturnStatus SockListnerPort::wait(IpcTimeout timeout, UInt32 *eventConsumed, IpcAwaitiox *ipcAwaitiox)
{
  // new file descriptor in case a connection request comes
  SockFdesc newFdesc;

  // call the accept system call to wait for client connection requests
  if (socket().accept(newFdesc,timeout))
    {
      // It worked, we have a new client and a socket to it in newFdesc.
      // Create a new IPC socket connection to the client.
      SockConnection *newConn =
	new(getEnvironment()->getHeap()) SockConnection(
	     getEnvironment(),newFdesc,FALSE);

      // call the virtual method of the control connection that is
      // supposed to know what to do with a new client connection
      cc_->acceptNewConnectionRequest(newConn);

      // the state remains "RECEIVING", listen for more connection requests
    }
  return WAIT_OK;
}

// -----------------------------------------------------------------------
// Methods for class SockControlConnection
// -----------------------------------------------------------------------

SockControlConnection::SockControlConnection(
     IpcEnvironment *env, const char *eye)
     : IpcControlConnection(IPC_DOM_INTERNET, eye),
       listnerSocket_(env)
{
  // This process was forked by the internet demon inetd or by the master ESP
  // Stdin and stdout are one socket that is already connected to the client.
  controlConnection_ = new(env->getHeap()) SockConnection(env,SockStdin,FALSE);
  incrNumRequestors();

  // bind the new listner socket to any port for communication with any process
  // and listen on the socket
  listnerPortNum_ = listnerSocket_.listen();

  // create a connection object that will listen on the listner port
  // and stay active forever
  listnerPort_ = new(env->getHeap()) SockListnerPort(
       env,listnerSocket_.getFdesc(),this);

  // the listener port has some arbitrary port number; send the listener port
  // number back to the client as a string "Listening to port xxxxxxxxxxx\n"
  cout << "Listening to port " << setw(11) << listnerPortNum_ << "\n" << flush;
}

SockControlConnection::SockControlConnection(
     IpcEnvironment *env, Int32 inheritedSocket, Int32 passedPort, const char *eye)
     : IpcControlConnection(IPC_DOM_INTERNET),
       listnerSocket_(env)
{
    assert(0);
}

IpcConnection *SockControlConnection::getConnection() const
{
  return controlConnection_;
}

SockControlConnection * SockControlConnection::castToSockControlConnection()
{
  return this;
}

void SockControlConnection::acceptNewConnectionRequest(SockConnection *conn)
{
  // the default implementation is to reject new connection requests
  // should have a deleteMe() method for IpcConnection $$$$
  conn->~SockConnection();
}

// -----------------------------------------------------------------------
// Some methods for class IpcServerClass
// -----------------------------------------------------------------------

IpcConnection * IpcServerClass::createInternetProcess(
     ComDiagsArea **diags,
     CollHeap   *diagsHeap,
     const char *nodeName,
     const char *className,
     IpcCpuNum  /*cpuNum (sorry, no support for SMPs yet)*/,
     NABoolean  /*usesTransactions (sorry, no transactions in cyberspace*/,
     SockPortNumber defaultPortNumber)
{
  IpcNodeName theNode(IPC_DOM_INTERNET,nodeName);

  // the inetd service identified by className_
  SockService service(className,defaultPortNumber);

  // Establish a connection to the service, which will cause
  // a new server process to be spawned
  IpcConnection *result = new(environment_->getHeap()) SockConnection(
       environment_,
       IpcProcessId(theNode.getIPAddress(),service.getPortNumber()),
       TRUE);

  // do an error check and set diags area if it's used
  if (diags)
    result->populateDiagsArea(*diags,diagsHeap);

  return result;
}



IpcConnection * IpcServerClass::forkProcess(ComDiagsArea **diags,
					    CollHeap   *diagsHeap,
					    const char * /*nodeName*/,
					    const char *className,
					    IpcCpuNum  /*cpuNum*/,
					    NABoolean  /*usesTransactions*/)
{
  IpcConnection *result = NULL;
  Int32 serverPid;
  SockPairConnection *sockPairServerConn;
  SockPairConnection *clientConn;

  sockPairServerConn = new SockPairConnection(environment_);
  clientConn = sockPairServerConn->otherEnd();
  result = sockPairServerConn;

  serverPid = fork();

  if (serverPid < 0)
    ABORT("Couldn't fork the server process");

  if (serverPid != 0)
    {
      // I'm the master

      // the connection to the client is not needed here
      delete clientConn;
      clientConn = NULL;
    }
  else
    {
      // delete (close) the connection to the server, I AM the server
      delete result;
      result = NULL;

      // assign the connection to the client to stdin/stdout, this
      // is the convention for a socket-based server to receive data
      clientConn->assignToStdInOut();

      // access the process' environment and propagate it
      extern char **environ;
      // the command line looks like "<className> -fork"
      const char *argv[3];

      argv[0] = (char *)className;
      argv[1] = "-fork";
      argv[2] = 0;

      // add "-debug" to the command line arguments if an environment
      // variable is set
      if (getenv("DEBUG_SERVER"))
	argv[1] = "-fork -debug";
      if (getenv("DEBUG_FORK"))
	NADebug();

      // I'm the server, call exec() on the server program file
    ABORT("Not implemented for milestone 1");

      // won't ever come out of here
    }

  return result;
}

#ifdef LOG_IPC
void IpcSockLogTimestamp(Int32 fdesc)
{
  cerr << getpid() << "(" << fdesc << ") ";
}
#endif

#endif /* DISABLE_SOCKET_IPC */
