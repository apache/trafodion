#ifndef NAIPC_H
#define NAIPC_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         NAIpc.h
 * Description:  Interprocess communication among SQL/ARK processes. Defines
 *               servers, requestors, and messages.
 *               
 * Created:      10/17/95
 * Language:     C++
 *
 *
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
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------
#include "Platform.h"

#include "IpcMessageType.h"
#include "Ipc.h"
#include "NAString.h"

// -----------------------------------------------------------------------
// Contents of this file
// -----------------------------------------------------------------------
class NASingleServer;
class NAMessage;
class NAMessageObj;

// -----------------------------------------------------------------------
// This object represents the connection to a single, possibly context-
// sensitive server, such as the SQL catman, compiler, or ESP servers.
// -----------------------------------------------------------------------
class NASingleServer
{
friend class NAMessage;

public:

  // ---------------------------------------------------------------------
  // A requestor creates a NASingleServer object to start a server process
  // and to open a connection to that server. The method on how to find
  // and start the server process is stored in the defaults tables, so
  // the server name must be one of the well-known servers.
  // A remote server can be started by specifying a node name.
  // ---------------------------------------------------------------------
  NASingleServer(
       ComDiagsArea **diags,
       CollHeap *diagsHeap,
       IpcServerType serverType,
       const char *node = NULL,
       IpcServerAllocationMethod allocationMethod = IPC_ALLOC_DONT_CARE);

  // give access to the ExServer object
  inline IpcServer *getServer()                             { return s_; }

private:

  IpcServerClass *sc_;
  IpcServer *s_;
}; // NASingleServer

// -----------------------------------------------------------------------
// A message
// -----------------------------------------------------------------------
class NAMessage : private IpcMessageStream
{
public:

  // ---------------------------------------------------------------------
  // Constructor to be used in a server: a message that is read from
  // the process' control connection. The control connection can be
  // $RECEIVE (NSK only) or a socket that is assigned to the process'
  // stdin and stdout (both Unix and NSK).
  // ---------------------------------------------------------------------
  NAMessage(IpcNetworkDomain domain
#if (defined(NA_GUARDIAN_IPC))
	    // when compiling on NSK systems the default is $RECEIVE
	    = IPC_DOM_GUA_PHANDLE
#else
	    // on Unix systems there is only one choice
	    = IPC_DOM_INTERNET
#endif
	    );

  // ---------------------------------------------------------------------
  // Constructor to be used in a client: a message to be sent to a
  // server and the returned data from the server
  // ---------------------------------------------------------------------
  NAMessage(NASingleServer *destination);

  // ---------------------------------------------------------------------
  // set and get the header information
  // ---------------------------------------------------------------------
  inline IpcMessageObjType getType() const
                                   { return IpcMessageStream::getType(); }
  inline IpcMessageObjVersion getVersion() const
                                { return IpcMessageStream::getVersion(); }
  inline void setType(IpcMessageObjType t)
                                         { IpcMessageStream::setType(t); }
  inline void setVersion(IpcMessageObjVersion v)
                                      { IpcMessageStream::setVersion(v); }

  // ---------------------------------------------------------------------
  // Check error information
  // ---------------------------------------------------------------------
  inline NABoolean hasError()
                       { return (getState() == IpcMessageStream::ERROR_STATE); }

  // ---------------------------------------------------------------------
  // Include an object into a message
  // ---------------------------------------------------------------------
  inline NAMessage & operator << (IpcMessageObj & toAppend)
     {IpcMessageStream::operator << ((IpcMessageObj &) toAppend);return *this;}

  // ---------------------------------------------------------------------
  // Extract an object of a given type from a message
  // ---------------------------------------------------------------------
  inline NAMessage & operator >> (IpcMessageObj & toRetrieve)
   {IpcMessageStream::operator >> ((IpcMessageObj &) toRetrieve);return *this;}

  // ---------------------------------------------------------------------
  // check whether there are more objects to extract
  // ---------------------------------------------------------------------
  inline NABoolean moreObjects() { return IpcMessageStream::moreObjects(); }

  // ---------------------------------------------------------------------
  // get information about the next object to be retrieved
  // ---------------------------------------------------------------------
  inline IpcMessageObjType getNextObjType()
                            { return IpcMessageStream::getNextObjType(); }
  inline IpcMessageObjVersion getNextObjVersion()
                         { return IpcMessageStream::getNextObjVersion(); }
  inline IpcMessageObjSize getNextObjSize()
                            { return IpcMessageStream::getNextObjSize(); }

  // ---------------------------------------------------------------------
  // reinitialize the message, so it can be used as if it were new
  // ---------------------------------------------------------------------
  void clear();

  // ---------------------------------------------------------------------
  // send the message
  // ---------------------------------------------------------------------
  void send(NABoolean wait = TRUE);

  // ---------------------------------------------------------------------
  // receive a message or a reply
  // ---------------------------------------------------------------------
  void receive(NABoolean wait = TRUE);

  // ---------------------------------------------------------------------
  // send a reply to a message (same as send)
  // ---------------------------------------------------------------------
  inline void reply()                                          { send(); }

}; // NAMessage

// -----------------------------------------------------------------------
// An object that can be added to or retrieved from a message.
// NAMessageObj is an abstract base class, which means that no objects
// of this class can be created, only objects of a derived class.
// All objects must at least override the virtual packedLength() method.
// See files Ipc.h and IpcMessageType.h for more documentation on the
// IpcMessageObj class.
// -----------------------------------------------------------------------
class NAMessageObj : public IpcMessageObj
{
public:

  // ---------------------------------------------------------------------
  // Constructor, to be used by derived classes to specify type and
  // version of the object.
  // ---------------------------------------------------------------------
  inline NAMessageObj(IpcMessageObjType objType,
		      IpcMessageObjVersion version = 100) :
                                          IpcMessageObj(objType,version) {}

}; // NAMessageObj

// -----------------------------------------------------------------------
// A global method to get access to the IPC environment
// (usually not needed for waited communication)
// -----------------------------------------------------------------------
IpcEnvironment *GetIpcEnv();
Lng32 GetNumRequestors();

#endif /* NAIPC_H */
