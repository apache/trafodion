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
#ifndef _EX_UDR_CLIENT_IPC_H_
#define _EX_UDR_CLIENT_IPC_H_
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ExUdrClientIpc.h
 * Description:  IPC streams and message objects for the client-side
 *               of a UDR server connection
 *               
 * Created:      08/20/2000
 * Language:     C++
 *
 *
 *****************************************************************************
 */

#include "UdrExeIpc.h"

// Forward class references
class ExUdrTcb;
class ExUdrServer;
class ExRsInfo;

// Classes defined in this file
class UdrClientControlStream;
class UdrClientDataStream;

const Int32 UdrClientControlStreamVersionNumber = 1;
const Int32 UdrClientDataStreamVersionNumber = 1;

//
// A non-buffered client-side message stream for UDR control messages.
// A given instance of this class will be used to send one and only
// one message. Once the reply for the message arrives the stream is 
// no longer used. The stream will notify a TCB of the reply if the
// TCB has not yet called delinkTcb(). 
//
class UdrClientControlStream : public UdrControlStream
{
public:
  typedef UdrControlStream super;

  UdrClientControlStream(IpcEnvironment *,
                         ExUdrTcb *,
                         ExExeStmtGlobals *,
                         NABoolean keepUdrDiagsForCaller,
                         NABoolean isTransactional);

  virtual ~UdrClientControlStream();

  // Users of a control stream can mark the stream with a specific
  // message type. This can be useful, for example, in callback
  // functions where different actions are required for different
  // message types.
  UdrIpcObjectType getMessageType() const { return messageType_; }
  void setMessageType(UdrIpcObjectType t) { messageType_ = t; }

  virtual void actOnSend(IpcConnection *);
  virtual void actOnSendAllComplete();
  virtual void actOnReceive(IpcConnection *);
  virtual void actOnReceiveAllComplete();

  // TCBs call this function to inform the stream that
  // they are not interested in any more callbacks
  void delinkTcb(const ExUdrTcb *);

  // If the stream was configured to retain diagnostics that arrive on
  // the stream, then this method can be used to extract them. Once a
  // caller extracts these diags, the stream releases control of them
  // and the caller becomes responsible for eventually decrementing
  // the reference count.
  ComDiagsArea *extractUdrDiags();

#ifdef UDR_DEBUG
  void setTraceFile(FILE *f) { traceFile_ = f; }
  void setTrustReplies(NABoolean trust) { trustReplies_ = trust; }
#endif

  NABoolean isTransactional() const { return isTransactional_; }

  // Control streams used for ENTER/EXIT/SUSPEND TX requests have a
  // pointer back to the ExRsInfo object that sent the request. This
  // way, if EXIT TX or SUSPEND TX encounter an IPC error the
  // associated ENTER TX can be abandoned.
  ExRsInfo *getExRsInfo() { return exRsInfo_; }
  void setExRsInfo(ExRsInfo *exRsInfo) { exRsInfo_ = exRsInfo; }

protected:

  // Returns TRUE if the TCB is expecting callbacks
  NABoolean tcbExpectsReply() const;
  
  // Returns TRUE if the TCB's statement globals is expecting callbacks
  NABoolean stmtGlobalsExpectsReply() const;
  
  ExUdrTcb *tcb_;
  ExExeStmtGlobals *stmtGlobals_;

  // ENTER/EXIT/SUSPEND TX requests have a pointer back to the
  // ExRsInfo
  ExRsInfo *exRsInfo_;

  UdrIpcObjectType messageType_;

  // When a stream instance is being used by a TCB and diags arrive on
  // the stream, the stream callback methods are able to add the diags
  // to the TCB's statement diags area. When an instance is not being
  // used by a TCB, the stream can be configured to hold on to those
  // diags and the stream user can retrieve them later by calling the
  // getUdrDiags() method.
  NABoolean keepUdrDiagsForCaller_;
  ComDiagsArea *udrDiagsForCaller_;

  NABoolean isTransactional_;

#ifdef UDR_DEBUG
  FILE *traceFile_;
  NABoolean trustReplies_;
#endif
};

//
// A buffered client-side message stream for UDR data
//
class UdrClientDataStream : public IpcClientMsgStream
{
public:
  typedef IpcClientMsgStream super;

  UdrClientDataStream(IpcEnvironment *env,
                      Lng32 sendBufferLimit,
                      Lng32 inUseBufferLimit,
                      IpcMessageObjSize bufferSize,
                      ExUdrTcb *tcb,
                      ExExeStmtGlobals *stmtGlobals,
                      NABoolean isTransactional);

  virtual ~UdrClientDataStream();

  virtual void actOnSend(IpcConnection *);
  virtual void actOnReceive(IpcConnection *);

  //
  // TCBs call this function to inform the stream that
  // they are not interested in any more callbacks
  //
  void delinkTcb(const ExUdrTcb *);

#ifdef UDR_DEBUG
  void setTraceFile(FILE *f) { traceFile_ = f; }
  void setTrustReplies(NABoolean trust) { trustReplies_ = trust; }
#endif

  NABoolean isTransactional() const { return isTransactional_; }

protected:

  //
  // Returns TRUE if the TCB is expecting callbacks
  //
  NABoolean tcbExpectsReply() const;

  //
  // Returns TRUE if the TCB's statement globals is expecting callbacks
  //
  NABoolean stmtGlobalsExpectsReply() const;

  ExUdrTcb *tcb_;
  ExExeStmtGlobals *stmtGlobals_;

  ULng32 sendCount_;
  ULng32 recvCount_;

  NABoolean isTransactional_;

#ifdef UDR_DEBUG
  FILE *traceFile_;
  NABoolean trustReplies_;
#endif

};

#endif // _EX_UDR_CLIENT_IPC_H_

