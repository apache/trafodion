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
#ifndef _UDRSTREAMS_H_
#define _UDRSTREAMS_H_

/* -*-C++-*-
*****************************************************************************
*
* File:         UdrStreams.h
* Description:  Server-side message stream classes for UDRs
* Created:      01/01/2001
* Language:     C++
*
*
*****************************************************************************
*/

#include "UdrExeIpc.h"

//
// Forward declarations
//
class UdrGlobals;
class SPInfo;

//
// Classes defined in this file
//
class UdrServerControlStream;
class UdrServerDataStream;
class UdrGuaControlConnection;
class UdrServerReplyStream;

const Int32 UdrServerControlStreamVersionNumber = 1;
const Int32 UdrServerDataStreamVersionNumber = 1;
const Int32 UdrServerReplyStreamVersionNumber = 100;

//
// A non-buffered server-side stream for UDR control messages
//
class UdrServerControlStream : public UdrControlStream
{
public:
  UdrServerControlStream(IpcEnvironment *env,
                         UdrGlobals *udrGlob,
                         IpcMessageType msgType,
                         IpcMessageObjVersion version)
    : UdrControlStream(env, msgType, version),
      udrGlob_(udrGlob)
  {
  }
  virtual ~UdrServerControlStream()
  {
  }

  void actOnReceive(IpcConnection *);

private:
  UdrGlobals *udrGlob_;

}; // class UdrServerControlStream

//
// A buffered server-side stream for UDR data
//
class UdrServerDataStream : public IpcServerMsgStream
{
friend class UdrServerReplyStream;

public:
  typedef IpcServerMsgStream super;

  UdrServerDataStream(IpcEnvironment *env,
                      Lng32 sendBufferLimit,
                      Lng32 inUseBufferLimit,
                      IpcMessageObjSize bufferSize,
                      UdrGlobals *UdrGlob,
                      SPInfo *spinfo)
    : IpcServerMsgStream(env,
                         UDR_STREAM_SERVER_DATA,
                         UdrServerDataStreamVersionNumber,
                         sendBufferLimit,
                         inUseBufferLimit,
                         bufferSize),
      udrGlob_(UdrGlob),
      replyTag_(GuaInvalidReplyTag),
      spinfo_(spinfo)
  {}

  virtual ~UdrServerDataStream()
  {
    releaseBuffers();
  }

  void actOnSend(IpcConnection *)
  {}

  void actOnReceive(IpcConnection *);

  void activateCurrentMsgTransaction();

protected:
  void setReplyTag(short tag) { replyTag_ = tag; }
  short getReplyTag() const { return replyTag_; }

private:
  UdrGlobals *udrGlob_;

  // replyTag of the request. There is no easy of getting reply tag
  // from IPC in case of buffered streams. So we store the tag here.
  // This tag will be set by the stream which routes messages to
  // this stream.
  short replyTag_;

  // Owner of this stream
  SPInfo *spinfo_;
}; // class UdrServerDataStream

// UdrGuaContolConnection
//   UDR Server needs to have different behavior for actOnSystemMessage()
//   because GuaReceiveControlConnection::actOnSystemMessage() does not
//   allow multiple requestors for a server process.
class UdrGuaControlConnection : public GuaReceiveControlConnection
{
public:

  UdrGuaControlConnection(IpcEnvironment *env,
                          UdrGlobals *udrGlob,
                          short receiveDepth = 4000) :
    GuaReceiveControlConnection(env, receiveDepth),
    udrGlob_(udrGlob)
  { }

  virtual void actOnSystemMessage(
       short                  messageNum,
       IpcMessageBufferPtr    sysMsg,
       IpcMessageObjSize      sysMsgLen,
       short                  clientFileNumber,
       const GuaProcessHandle &clientPhandle,
       GuaConnectionToClient  *connection);

private:
  UdrGlobals *udrGlob_;
}; // class UdrGuaControlConnection

// A stream that is used to reply to messages
class UdrServerReplyStream : public UdrControlStream
{
public:
  UdrServerReplyStream(IpcEnvironment *env,
                       UdrGlobals *udrGlob,
                       IpcMessageType msgType,
                       IpcMessageObjVersion version)
    : UdrControlStream(env, msgType, version),
      udrGlob_(udrGlob)
  { }

  virtual ~UdrServerReplyStream()
  { }

  void routeMessage(UdrServerDataStream &other);

  void activateCurrentMsgTransaction();

  void actOnReceive(IpcConnection *)
  { }

private:
  UdrGlobals *udrGlob_;
}; // class UdrServerReplyStream

#endif // _UDRSTREAMS_H_
