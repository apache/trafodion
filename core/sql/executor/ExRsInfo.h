/******************************************************************************
*
* File:         ExRsInfo.h
* Description:  A container class for Statment globals Result Set info
*
* Created:      October 2005
* Language:     C++
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
******************************************************************************
*/
#ifndef EX_RS_INFO_H
#define EX_RS_INFO_H

#include "Platform.h"
#include "NABoolean.h"
#include "NABasicObject.h"
#include "Collections.h"
#include "ComSmallDefs.h"
#include "Ipc.h"
#include "ComRtUtils.h"

// Contents of this file
class RsInfo;
class ExRsInfo;

// Forward declarations
class NAMemory;
class ExUdrServer;
class Statement;
class ExExeStmtGlobals;
class UdrClientControlStream;

// Some data members such as the NAArray come from outside this
// DLL. The Windows compiler generates a warning about them requiring
// a DLL interface in order to be used by ExRsInfo clients. We
// will suppress such warnings.
#pragma warning ( disable : 4251 )

//------------------------------------------------------------------------
// class RsInfo
//
// This class is a container for Result Set info 
// and is instantiated by ExRsInfo
//
//------------------------------------------------------------------------
class RsInfo : public NABasicObject
{
  friend class ExRsInfo;

private:
  RsInfo();
  virtual ~RsInfo();

  //Accessors
  Statement * getStatement() const {return statement_;} 
  NABoolean getOpenAttempted() const {return openAttempted_;} 
  NABoolean getCloseAttempted() const {return closeAttempted_;} 
  NABoolean isPrepared() const {return prepared_;}
  char * getProxySyntax() const { return proxySyntax_;} 
  
  //Mutators
  void setStatement(Statement *statement) {statement_ = statement;}
  void setOpenAttempted(NABoolean flag) {openAttempted_ = flag;}
  void setCloseAttempted(NABoolean flag) {closeAttempted_ = flag;}
  void setPrepared(NABoolean flag) {prepared_ = flag;}
  void setProxySyntax(char *proxySyntax) {proxySyntax_ = proxySyntax;}  

  //Class data members
  Statement *statement_;
  NABoolean openAttempted_;
  NABoolean closeAttempted_;
  NABoolean prepared_;
  char *proxySyntax_;
}; //RsInfo

//------------------------------------------------------------------------
// class ExRsInfo
//
// This class is a wrapper around a collection of RsInfo objects. 
//
//------------------------------------------------------------------------
class ExRsInfo : public NABasicObject
{
public:
  ExRsInfo();
  virtual ~ExRsInfo();

  void populate(ULng32 index,   
                const char * proxySyntax);   
  void bind(ULng32 index,       
            Statement * statement);   
  void unbind(Statement *statement);   
  void setOpenAttempted(ULng32 index);  
  void setCloseAttempted(ULng32 index);  
  void setPrepared(ULng32 index); 
  void reset();
  NABoolean statementExists(ULng32 index) const; 
  NABoolean openAttempted(ULng32 index) const;
  NABoolean closeAttempted(ULng32 index) const;
  NABoolean isPrepared(ULng32 index) const;
  NABoolean getRsInfo(ULng32 position,    // IN
                      Statement *&statement,     // OUT
                      const char *&proxySyntax,  // OUT
                      NABoolean &openAttempted,  // OUT
                      NABoolean &closeAttempted, // OUT
                      NABoolean &isPrepared)     // OUT
    const;
  ULng32 getIndex(Statement * statement) const;

  ExUdrServer * getUdrServer() const {return udrServer_;}
  const IpcProcessId & getIpcProcessId() const {return ipcProcessId_;} 
  Int64 getUdrHandle() const {return udrHandle_;}
  void setUdrServer(ExUdrServer *udrServer) {udrServer_ = udrServer;}
  void setIpcProcessId(const IpcProcessId &ipcProcessId);
  void setUdrHandle(Int64 udrHandle) {udrHandle_ = udrHandle;}

  ULng32 getNumReturnedByLastCall() const
  { return numReturnedByLastCall_; }
  ULng32 getNumClosedSinceLastCall() const
  { return numClosedSinceLastCall_; }

  NABoolean allResultsAreClosed() const
  {
    return (numClosedSinceLastCall_ >= numReturnedByLastCall_ ? TRUE : FALSE);
  }

  // Methods to get and set the maximum number of RS entries this
  // object can hold. 
  // 
  // If the user of this object calls setNumEntries(N) to set the max,
  // the object may actually raise the limit internally and
  // getNumEntries() may return a value higher than N.
  //
  // Users of this class should not rely on this "non-exact"
  // behavior. It is a side-effect of using NAArray in the
  // implementation of this class, and may change in the future.
  void setNumEntries(ComUInt32 maxEntries);
  const ULng32 getNumEntries() const
  {
    ULng32 n = rsInfoArray_.getSize();
    // We return (n-1) to account for the never-used entry in slot 0
    return (n > 0 ? n - 1 : 0);
  }

  // Methods to bring the UDR server into and out of a transaction for
  // a single CALL operation that can potentially return result
  // sets. To bring the server into a transaction we will send an
  // ENTER TX request. To temporarily bring the server out we send a
  // SUSPEND TX request. To bring the server fully out we send an EXIT
  // TX request. The stmtGlobals object passed in will have its diags
  // area populated if IPC errors are encountered.
  void enterUdrTx(ExExeStmtGlobals &stmtGlobals);
  void suspendUdrTx(ExExeStmtGlobals &stmtGlobals);
  void exitUdrTx(ExExeStmtGlobals &stmtGlobals);

  UdrClientControlStream *getEnterTxStream() const { return enterTxStream_; }
  UdrClientControlStream *getExitOrSuspendStream() const
  { return exitOrSuspendStream_; }

  // Message streams for ENTER, EXIT, and SUSPEND TX messages will use
  // this method to inform the ExRsInfo that the message interaction
  // has completed.
  void reportCompletion(UdrClientControlStream *);

private:

  enum TxMsgType { ENTER, SUSPEND, EXIT };

  enum TxState { IDLE, ACTIVE, SUSPENDED };

  static const char *getTxMsgTypeString(TxMsgType t)
  {
    switch (t)
    {
      case ENTER: return "ENTER";
      case SUSPEND: return "SUSPEND";
      case EXIT: return "EXIT";
      default: return ComRtGetUnknownString((Int32) t);
    }
  }

  static const char *getTxStateString(TxState s)
  {
    switch (s)
    {
      case IDLE: return "IDLE";
      case ACTIVE: return "ACTIVE";
      case SUSPENDED: return "SUSPENDED";
      default: return ComRtGetUnknownString((Int32) s);
    }
  }

  // A private method to do IPC for TX requests
  void sendTxMessage(ExExeStmtGlobals &, TxMsgType);

  ARRAY(RsInfo*) rsInfoArray_;
  ExUdrServer *udrServer_;
  IpcProcessId ipcProcessId_;
  Int64 udrHandle_;
  TxState txState_;

  // When an ENTER TX message is sent we need to keep a pointer to the
  // stream. This allows the ENTER TX to later be abandoned if the
  // EXIT TX or SUSPEND TX cannot be successfully sent.
  UdrClientControlStream *enterTxStream_;

  // We also need a pointer to the EXIT TX or SUSPEND TX stream. This
  // allows the ExRsInfo destructor to inform the stream when the
  // ExRsInfo instance is going away.
  UdrClientControlStream *exitOrSuspendStream_;

  ULng32 numReturnedByLastCall_;
  ULng32 numClosedSinceLastCall_;

#ifdef _DEBUG
public:
  void ExRsPrintf(const char *formatString, ...) const;
  NABoolean debugEnabled() const { return debug_; }
private:
  NABoolean debug_;
  NABoolean txDebug_;
  void displayList() const;
#endif
}; // class ExRsInfo

#pragma warning ( default : 4251 )

#endif // EX_RS_INFO_H
