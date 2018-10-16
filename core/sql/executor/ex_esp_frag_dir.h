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
#ifndef EX_ESP_FRAG_DIR_H
#define EX_ESP_FRAG_DIR_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ex_esp_frag_dir.h
 * Description:  Fragment instance directory in the ESP
 *               
 *               
 * Created:      1/22/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "Ex_esp_msg.h"
#include "Globals.h"

// -----------------------------------------------------------------------
// Contents of this file
// -----------------------------------------------------------------------

class ExEspFragInstanceDir;

// -----------------------------------------------------------------------
// Forward references
// -----------------------------------------------------------------------

class ex_split_bottom_tdb;
class ex_split_bottom_tcb;
class ExEspStmtGlobals;
class MemoryMonitor;
class StatsGlobals;

// -----------------------------------------------------------------------
// Fragment instance directory in the ESP (multiple instances of multiple
// fragments from multiple owners)
// -----------------------------------------------------------------------

const Int32               NumFiTraceElements = 32;

// state name strings, must match with the enums below
static const char * FragmentInstanceStateName[] =
  {
    "UNUSED         ",
    "DOWNLOADED     ",
    "UNPACKED       ",
    "BUILT          ",
    "FIXED_UP       ",
    "READY_INACTIVE ",
    "ACTIVE         ",
    "WAIT_TO_RELEASE",
    "RELEASING_WORK ",
    "RELEASING      ",
    "GOING_FATAL    ",
    "BROKEN         "
  };

class ExEspFragInstanceDir
{
  friend class ExEspControlMessage;

public:
  
  enum FragmentInstanceState
    {
      UNUSED,            // unused entry, no ExEspFragInstance is allocated
      DOWNLOADED,        // fragment downloaded, globals but no tcb allocated
      UNPACKED,          // pointers relocated
      BUILT,             // tcb is now allocated (built)
      FIXED_UP,          // fixup method is now called, files are open
      READY_INACTIVE,    // ready to work but there is currently no work
      ACTIVE,            // tcb has current requests and its transid is active
      WAIT_TO_RELEASE,   // waiting to get replies from msgs to producer ESPs 
      RELEASING_WORK,    // need to release work requests, tcb may be active
      RELEASING,         // release request for this instance has been received
      GOING_FATAL,       // frag's scheduler has received WORK_BAD_ERROR
      BROKEN             // error state, don't use statement

      // Possible state transitions (all others should be considered illegal):
      //
      // UNUSED -> DOWNLOADED             load message received, instance is
      //                                  initialized
      // DOWNLOADED -> UNPACKED           TDB tree got unpacked from load message
      // UNPACKED -> BUILT                TCB tree got built after unpack
      // BUILT -> FIXED_UP                first fixup message received
      // READY_INACTIVE -> FIXED_UP       second or later fixup message received
      //
      // FIXED_UP -> READY_INACTIVE       first send top has opened send bottom
      // 
      // READY_INACTIVE -> ACTIVE         send bottom receives first request
      //
      // ACTIVE ->WAIT_TO_RELEASE         Master sends release message. Frag
      //                                  dir now switches among frag instances
      //                                  while waiting for replies to msgs
      //                                  to producer ESPs.
      // ACTIVE -> RELEASING_WORK         last send bottom replies to last
      //                                  request
      // WAIT_TO_RELEASE -> RELEASING_WORK all messages from producer ESPs 
      //                                  replied
      // RELEASING_WORK -> ACTIVE         work message is released and
      //                                  there are still active requests
      // RELEASING_WORK -> READY_INACTIVE work message is released and
      //                                  there are no more requests
      // 
      // any state -> GOING_FATAL         bad things happen
      //
      // (DOWNLOADED, UNPACKED, BUILT,
      //  FIXED_UP, READY_INACTIVE,
      //  RELEASING, GOING_FATAL,
      //  BROKEN) -> RELEASING            release message received
      //
      // RELEASING -> UNUSED              release message processed
      //

    };

  ExEspFragInstanceDir(CliGlobals *cliGlobals,
		       NAHeap *heap,
                       StatsGlobals *statsGlobals);
  ~ExEspFragInstanceDir();

  inline CollIndex getNumEntries() const  { return instances_.entries(); }
  inline Lng32 getNumMasters() const                { return numMasters_; }
  inline IpcEnvironment * getEnvironment() const
    { return cliGlobals_->getEnvironment(); }
  inline CliGlobals *getCliGlobals() const
    {return cliGlobals_; }

  // convert key into handle and vice versa
  ExFragInstanceHandle findHandle(const ExFragKey &key) const;
  inline const ExFragKey & findKey(ExFragInstanceHandle handle) const
                                      { return instances_[handle]->key_; }

  // get the top level tcb for an entry
  inline ex_split_bottom_tcb * getTcb(ExFragInstanceHandle h) const
                                  { return instances_[h]->localRootTcb_; }
  inline ExEspStmtGlobals *getGlobals(ExFragInstanceHandle h) const
  { return instances_[h]->globals_; }

  ex_split_bottom_tcb *getExtractTop(const char *securityKey);

  ExFragInstanceHandle addEntry(ExMsgFragment *msgFragment,
				IpcConnection *connection);
  void fixupEntry(ExFragInstanceHandle handle,
		  Lng32 numOfParentInstances,
		  ComDiagsArea &da);

  // keep track of the activities that go on in the send bottom nodes,
  // record open operation and arrival and finishing of requests
  void openedSendBottom(ExFragInstanceHandle handle);
  void startedSendBottomRequest(ExFragInstanceHandle handle);
  void finishedSendBottomRequest(ExFragInstanceHandle handle);
  void startedSendBottomCancel(ExFragInstanceHandle handle);
  void finishedSendBottomCancel(ExFragInstanceHandle handle);
  void startedLateCancelRequest(ExFragInstanceHandle handle);
  void finishedLateCancelRequest(ExFragInstanceHandle handle);
  Lng32 numLateCancelRequests(ExFragInstanceHandle handle);

  //  Do the ACTIVE->RELEASING_WORK transition
  void finishedRequest(ExFragInstanceHandle handle,
                       NABoolean testAllQueues = FALSE);

  // Do the ACTIVE->WAIT_TO_RELEASE transition
  void hasReleaseRequest(ExFragInstanceHandle handle);

  // release the entry and all its dependent entries
  void releaseEntry(ExFragInstanceHandle handle);

  // release the entries that lost their parent
  void releaseOrphanEntries();

  // mark in the entry that it no longer has a transid
  void hasTransidReleaseRequest(ExFragInstanceHandle handle);

  inline Lng32 getNumActiveInstances()      { return numActiveInstances_; }
  enum FragmentInstanceState getEntryState(ExFragInstanceHandle handle) const
                                    { return instances_[handle]->fiState_; }
  ExMsgFragment * getFragment(ExFragInstanceHandle handle) const
                             { return instances_[handle]->msgFragment_; }
  ex_split_bottom_tcb *getTopTcb(ExFragInstanceHandle handle) const
                             { return instances_[handle]->localRootTcb_; }

  Lng32 getNumSendBottomRequests(ExFragInstanceHandle handle)
  { return instances_[handle]->numSendBottomRequests_; }
  Lng32 getNumSendBottomCancels(ExFragInstanceHandle handle)
  { return instances_[handle]->numSendBottomCancels_; }

  // work on all active fragment instances
  void work(Int64 prevWaitTime);

  class ExEspFragInstance
    {
    public:
      ExFragKey               key_;           // pid,stmthandle,fragid triplet
      ExFragInstanceHandle    handle_;        // index of the owning array
      FragmentInstanceState   fiState_;         // state of this entry
      ExFragDir::ExFragEntryType fragType_; // ESP or DP2 fragment
      ExFragKey               parentKey_;     // parent fragment id
      IpcConnection           *controlConn_;  // control connection used
      Lng32                    topNodeOffset_; // offset of top tcb in frag
      ExMsgFragment           *msgFragment_;  // downloaded fragment buffer
      ex_split_bottom_tdb     *localRootTdb_; // root of this fragment
      ex_split_bottom_tcb     *localRootTcb_; // root of this fragment
      ExEspStmtGlobals        *globals_;      // global statement vars
      Lng32                    numSendBottomRequests_; // # client work requests
      Lng32                    numSendBottomCancels_;  // # client cancel requests
      Lng32                    numLateCancelRequests_; // # late cancels below
      NABoolean               displayInGui_;  // enable executor GUI
      unsigned short          mxvOfOriginator_;// plan version of the plan fragment
      unsigned short          planVersion_;   // plan version of the plan fragment
      char                    *queryId_;
      Lng32                    queryIdLen_;
    };

  // Check the plan version of the sent fragment
  Lng32 checkPlanVersion(const ExEspFragInstance * entry,
                        ComDiagsArea& da);

  StatsGlobals *getStatsGlobals() { return statsGlobals_; }
  NAHeap *getStatsHeap() { return statsHeap_; }
  pid_t getPid() { return pid_;};
  pid_t getTid() { return tid_;};
  Int32 getCpu() { return cpu_; }
  Long &getSemId() { return semId_; }; 
  NAHeap *getLocalStatsHeap();

  // A function to establish database user identity for this
  // process. On Linux, ContextCli data members will be updated. On
  // other platforms this call is a no-op.
  void setDatabaseUserID(Int32 userID, const char *userName);

  NABoolean getUserIDEstablished() const { return userIDEstablished_; }

  // For fragment instance trace
  struct FiStateTrace 
  {
    ExFragInstanceHandle fragId_;
    FragmentInstanceState fiState_;
    Int32 lineNum_;
  };

  Int32 printALiner(Int32 lineno, char *buf);
  static Int32 getALine(void * mine, Int32 lineno, char * buf)
           { return ((ExEspFragInstanceDir *) mine)->printALiner(lineno, buf); }

  void initFiStateTrace();
  FragmentInstanceState setFiState(ExFragInstanceHandle fragId,
                                   FragmentInstanceState newState,
                                   Int32 linenum);

private:

  // how many instances need to have their scheduler called?
  Lng32                       numActiveInstances_;

  // no instance has a number >= to this
  CollIndex                  highWaterMark_;

  // an array with pointers to all downloaded fragment instances
  ARRAY(ExEspFragInstance *) instances_;

  // how many master ESPs are we talking to?
  Lng32                       numMasters_;

  CliGlobals                 *cliGlobals_;

  NAHeap                     *heap_;

  StatsGlobals               *statsGlobals_;
  NAHeap                     *statsHeap_;
  pid_t                      pid_;
  pid_t                      tid_;
  Int32                      cpu_;
  Long                       semId_;
  NAHeap                     *localStatsHeap_;

  // A boolean to indicate whether this process has already updated
  // the ContextCli user identity. Only meaningful on Linux.
  NABoolean                  userIDEstablished_;

  // Fragment instance trace
  FiStateTrace fiStateTrace_[NumFiTraceElements];
  Int32 fiTidx_;
  void *traceRef_;
  
  // private methods
  ExEspFragInstance * findEntry(const ExFragKey &key);
  void destroyEntry(ExFragInstanceHandle handle);

  void traceIdleMemoryUsage();
};

// -----------------------------------------------------------------------
// Message stream to receive requests through the control connection
// (such as load, fixup, ...)
// -----------------------------------------------------------------------

class ExEspControlMessage : public IpcMessageStream
{
public:

  ExEspControlMessage(ExEspFragInstanceDir *fragInstanceDir,
		      IpcEnvironment       *ipcEnvironment,
		      CollHeap             *heap);
  virtual ~ExEspControlMessage();

private:

  // private data members

  // fragment instance directory, to find instances referred to in requests
  ExEspFragInstanceDir *fragInstanceDir_;

  CollHeap             *heap_;

  // store info about the received request (key, handle)
  ExFragKey            currKey_;
  ExFragInstanceHandle currHandle_;

  // private methods

  // callback functions
  virtual void actOnSend(IpcConnection *connection);
  virtual void actOnSendAllComplete();
  virtual void actOnReceive(IpcConnection *connection);

  // act on individual requests
  void actOnLoadFragmentReq(IpcConnection *connection,
			    ComDiagsArea & da);
  void actOnFixupFragmentReq(ComDiagsArea & da);
  void actOnReleaseFragmentReq(ComDiagsArea & da);
  void actOnReqForSplitBottom(IpcConnection *connection);
  void incReplyMsg(Int64 msgBytes);
};

#endif /* EX_ESP_FRAG_DIR_H */
