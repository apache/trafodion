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
// File:         CancelBroker.h
// Description:  Class declaration for Cancel Broker role of SSMP.
//
// Created:      Aug 17, 2009
**********************************************************************/

#ifndef _CANCELBROKER_H_
#define _CANCELBROKER_H_
#include "Ipc.h"
#include "ComQueue.h"
#include "Int64.h"
#include "ssmpipc.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ActiveQueryMgr;
class ActiveQueryEntry;
class ActiveQueryStream;
class PendingQueryMgr;
class PendingQueryEntry;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class SsmpNewIncomingConnectionStream;
class SsmpGlobals;

// -----------------------------------------------------------------------
// API for some methods.
// -----------------------------------------------------------------------

enum NextActionForSubject {
  CB_COMPLETE,
  CB_CANCEL,
  CB_DONT_CARE
};

/////////////////////////////////////////////////////////////
// class ActiveQueryStream
//
// Since the Start Query message is not replied to until the Finished
// Query or Cancel Query message arrives, an ActiveQueryEntry needs 
// its own message stream to allow the SsmpNewIncomingConnectionStream
// to continue receiving and replying to unrelated messages during
// this interval.  The method IpcMessageStream::giveMessageTo is used
// to transfer the message from SsmpNewIncomingConnectionStream to 
// the ActiveQueryStream.  The model here is similar to that which 
// is used by ex_split_bottom_tcb's SplitBottomSavedMessage. 
/////////////////////////////////////////////////////////////

class ActiveQueryStream : public IpcMessageStream
{
public:

  ActiveQueryStream(IpcEnvironment *env, SsmpGlobals * ssmpG,
                    ActiveQueryEntry *myAq);

  virtual void actOnSend(IpcConnection* connection);

private:
  ActiveQueryEntry *myAq_;
  SsmpGlobals *ssmpGlobals_;

};

/////////////////////////////////////////////////////////////
// class ActiveQueryEntry
/////////////////////////////////////////////////////////////

class ActiveQueryEntry : public NABasicObject
{
public:

  ActiveQueryEntry(char *qid, Lng32 qidLen, 
                   Int64 startTime,
                   GuaProcessHandle master,
                   short masterFileNum,
                   Int32 executionCount,
                   IpcEnvironment *ipcEnv,
                   SsmpGlobals *ssmpG
                   );

  ~ActiveQueryEntry();

  void replyToQueryStarted(NAHeap *ipcHeap, NextActionForSubject nxtA,
                           bool cancelLogging);

  char *getQid() const { return qid_; }

  Lng32 getQidLen() const { return qidLen_; }

  Int64 getQueryStartTime() const { return queryStartTime_; }

  GuaProcessHandle getMasterPhandle() const {return master_;}

  short getMasterFileNum() const {return masterFileNum_;}

  ActiveQueryStream *getReplyStartedStream () { return replyStartedStream_; }

  Int32 getExecutionCount() const { return executionCount_; }

  void releaseStream();

private:

  char * qid_;
  Lng32 qidLen_;
  ActiveQueryStream *replyStartedStream_;
  Int64 queryStartTime_;
  GuaProcessHandle master_;
  short masterFileNum_;
  Int32 executionCount_;
}; // ActiveQueryEntry

/////////////////////////////////////////////////////////////
// class ActiveQueryMgr
/////////////////////////////////////////////////////////////

class ActiveQueryMgr : public NABasicObject
{
public:

  ActiveQueryMgr(IpcEnvironment *ipcEnv, NAHeap *heap) :
        activeQueries_(heap) 
      , heap_(heap)
      , ipcEnv_(ipcEnv)
        {}

  ~ActiveQueryMgr() 
    {};

  // Called from actOnCancelQueryReq for the Cancel Query msg.
  ActiveQueryEntry *getActiveQuery(char *qid, Lng32 qidLen);

  // Called from actOnQueryStartedReq for the Begin Query msg
  void addActiveQuery(char *qid, Lng32 qidLen, Int64 startTime, 
                 GuaProcessHandle masterPhandle, Int32 executionCount,
                 SsmpNewIncomingConnectionStream *cStream, 
                 IpcConnection *conn );

  // Called from actOnQueryFinishedReq for the Finished Query msg.  This call 
  // will send a reply to the QueryStarted message.  The caller is responsible
  // for replying to the FinishedQuery msg.
  void rmActiveQuery(char *qid, Lng32 qidLen, NAHeap *ipcHeap,
                     NextActionForSubject nxtA, bool cancelLogging);

  // Called from actOnSystemMessage for the close message, if the master
  // process ends before the Finished Query message is sent.
  void clientIsGone(const GuaProcessHandle &client, short fnum);

private:

  NAHeap *heap_;

  IpcEnvironment *ipcEnv_;
  
  HashQueue activeQueries_;
}; // ActiveQueryMgr


/////////////////////////////////////////////////////////////
// class PendingQueryEntry
/////////////////////////////////////////////////////////////

class PendingQueryEntry : public NABasicObject
{
public:

  PendingQueryEntry(char *qid, Lng32 qidLen, Int32 executionCount,
                    GuaProcessHandle master, short masterFileNum,
                    Int64 escalateTime1, Int64 escalateTime2,
                    bool cancelEscalationSaveabend, bool cancelLogging);

  ~PendingQueryEntry();

  char *getQid() const { return qid_; }

  Lng32 getQidLen() const { return qidLen_; }

  Int32 getExecutionCount() const  { return executionCount_; }

  Int64 getEscalateTime1() const  { return escalateTime1_; }

  Int64 getEscalateTime2() const  { return escalateTime2_; }

  bool getCancelEscalationSaveabend() const 
              { return cancelEscalationSaveabend_; }

  GuaProcessHandle getMasterPhandle() const { return master_; }

  short getMasterFileNum() const {return masterFileNum_;}

  bool getHaveEscalated1() const {return haveEscalated1_; }

  void setHaveEscalated1() {haveEscalated1_ = true; }

  bool getCancelLogging() const { return cancelLogging_; }

private:

  char * qid_;
  Lng32 qidLen_;
  Int32   executionCount_;
  Int64 escalateTime1_;
  Int64 escalateTime2_;
  bool cancelEscalationSaveabend_;
  bool haveEscalated1_;
  GuaProcessHandle master_;
  short masterFileNum_;
  bool cancelLogging_;
}; // PendingQueryEntry

class PendingQueryMgr : public NABasicObject
{
public:

  PendingQueryMgr(SsmpGlobals *ssmpGlobals, NAHeap *heap);

  ~PendingQueryMgr() 
    {};

  // Called from actOnCancelRequest 
  void addPendingQuery(ActiveQueryEntry *aq, Int32 ceFirstInterval, 
                       Int32 ceSecondInterval, NABoolean ceSaveabend,
                       NABoolean cancelLogging);

  // Called from SsmpGlobals::work every time it awakes.
  void killPendingCanceled();

  // Called from actOnSystemMessage for the close message, if the master
  // process ends before the Finished Query message is sent.
  void clientIsGone(const GuaProcessHandle &client, short fnum);

private:

  void removePendingQuery(PendingQueryEntry *pq);

  // Called from this->killPendingCanceled(), to stop server 
  // processes of one pending query.
  void askSscpsToStopServers(PendingQueryEntry *pq);

  SsmpGlobals *ssmpGlobals_;
  
  HashQueue pendingQueries_;
}; // PendingQueryMgr



#endif // _CANCELBROKER_H_

