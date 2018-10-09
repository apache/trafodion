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
#ifndef EX_EXE_STMT_GLOBALS_H
#define EX_EXE_STMT_GLOBALS_H


/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ex_exe_stmt_globals.h
 * Description:  Statement globals for stmts executed in master exe or esp.
 *               
 *               
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include <sys/types.h>
#include <sys/syscall.h>
#include "Platform.h"

#include "Globals.h"
#include "ex_globals.h"
#include "FragDir.h"
#include "ex_frag_inst.h"
#include "Int64.h"
#include "ExCollections.h"
#include "timeout_data.h"
#include "ex_esp_frag_dir.h"

// forward
class SequenceValueGenerator;

// Comment in and build to trace an ESPAccess ESP process
// 
//#define TRACE_ESP_ACCESS 1

#if defined(_DEBUG) && defined(TRACE_ESP_ACCESS)

class ESPTraceEntry
{
public:
  ESPTraceEntry(ex_globals *glob, char *t1); 

  ESPTraceEntry(ex_globals *glob, Lng32 espNum, 
		Lng32 pid,
		Int64 currentTS,
		char* t1);

  ~ESPTraceEntry();
 
  void createMessage(char *msg);

  // get the espNum from (int) espGlob->getMyInstanceNumber();
  ULng32 espNum_;

  // from  ExEspStmtGlobals->getPid()
  pid_t pid_;

  // gotten from doing NA_JulianTimestamp()
  Int64 timestamp_;

  // A message
  char *msgtext1_;

  ex_globals *globals_;
};

// List of ESPTraceEntry
class ESPTraceList : public LIST(ESPTraceEntry *)
{
public:

  ESPTraceList(ex_globals *globals,
	       CollHeap* h/*=0*/) 
    : LIST(ESPTraceEntry *)(h),
    globals_(globals),
    traceOn_(FALSE)
    {
    }
 
  ~ESPTraceList();
 
  void insertNewTraceEntry(char *msg);

  void logESPTraceToFile(char *fn, char *signature, ESPTraceList &traceList);

  // Remove all entries the list and call their destructors
  void clearAndDestroy();
  
  ex_globals *globals_;
  NABoolean traceOn_;

}; // class ESPTraceList


#endif

// -----------------------------------------------------------------------
// Contents of this file
// -----------------------------------------------------------------------

class ExExeStmtGlobals;
class ExMasterStmtGlobals;
class ExEspStmtGlobals;

// forward references
class ExRtFragTable;
class ExProcessIdsOfFragList;
class ExEspFragInstanceDir;
class ComDiagsArea;
class IpcEnvironment;
class IpcProcessId;
class ExEspManager;
class ExFragDir;
class ContextCli;
class CliGlobals;
class ResolvedNameList;
class MemoryMonitor;
class ExMsgResourceInfo;
class ExScratchFileOptions;
class ComTdbRoot;
class Statement;
class ExUdrServer;
class ExRsInfo;
class ex_send_top_tcb;
class StmtStats;
class SMConnection;
class ExSMDownloadInfo;

class ExExeStmtGlobals : public ex_globals
{
public:

  ExExeStmtGlobals(short num_temps,
                   CliGlobals *cliGlobals,
		   short create_gui_sched = 0,
		   Space * space = NULL,
		   CollHeap * heap = NULL);

  // Deletes objects this object points to... does NOT destroy
  // this object
  virtual void deleteMe(NABoolean fatalError);
  
  virtual ExExeStmtGlobals * castToExExeStmtGlobals();
  virtual ExMasterStmtGlobals * castToExMasterStmtGlobals();
  virtual ExEspStmtGlobals * castToExEspStmtGlobals();

  // ---------------------------------------------------------------------
  // Information about fragments belonging to this statement
  // ---------------------------------------------------------------------

  // get the pointer to a fragment of the statement (or NULL if the
  // fragment isn't available) and the fragment length
  virtual char * getFragmentPtr(ExFragId fragId) const = 0;
  virtual IpcMessageObjSize getFragmentLength(ExFragId fragId) const = 0;

  // get the fragment key for a given fragment of this statement
  virtual ExFragKey getFragmentKey(ExFragId fragId) const = 0;

  // simple method to get number of instances for my fragment id
  // (this method is rooted in the base class, ex_globals)
  virtual Lng32 getNumOfInstances() const;

  // get the number of instances for a given fragment and get the process
  // id for one of the instances, or find out how many processes are
  // in the local node and which of them I am
  virtual Lng32 getNumOfInstances(ExFragId fragId) const = 0;
  virtual const IpcProcessId & getInstanceProcessId(
       ExFragId fragId,
       Lng32 instanceNum) const = 0;
  virtual Lng32 getMyInstanceNumber() const = 0;
  virtual void getMyNodeLocalInstanceNumber(
       Lng32 &myNodeLocalInstanceNumber,
       Lng32 &numOfLocalInstances) const = 0;

  // Virtual methods to retrieve SeaMonster settings. These methods
  // will be implemented in the master and ESP subclasses.
  virtual Int64 getSMQueryID() const = 0;
  virtual Int32 getSMTraceLevel() const = 0;
  virtual const char *getSMTraceFilePrefix() const = 0;

  inline IpcPriority getMyProcessPriority()
    { return cliGlobals_->myPriority(); }

  // get resource info
  virtual const ExScratchFileOptions *getScratchFileOptions() const = 0;

  // there is one SQL diagnostics area for the statement; to be used only
  // for fatal errors that aren't related to a single row
  inline ComDiagsArea *getDiagsArea() const           { return diagsArea_; }
  void setGlobDiagsArea(ComDiagsArea *da);

#if defined(_DEBUG) && defined (TRACE_ESP_ACCESS)
  inline ESPTraceList *getESPTraceList() const {return espTraceList_; }
  void setESPTraceList(ESPTraceList *traceList) {espTraceList_ = traceList;}
#endif

  // update the input parameter diags area with the (non-queue) error
  // info from globals, if there is any
  void takeGlobalDiagsArea(ComDiagsArea &cliDA);

  void setCliGlobals(CliGlobals * cg)                  { cliGlobals_ = cg; }
  inline CliGlobals * getCliGlobals()                { return cliGlobals_; }

  // return of TRUE means error, FALSE means all Ok.
  virtual NABoolean closeTables();
  virtual NABoolean reOpenTables();

  // This method adds a Condition to the statement's diags area
  // to complain about memory alloc errors.
  void makeMemoryCondition(Lng32 errCode);

  inline ContextCli *getContext() const
    { return cliGlobals_->currContext(); }

  SequenceValueGenerator * seqGen();

  // the IPC environment is used by the IPC routines and contains useful
  // information (such as a dynamic heap) for other code as well
  inline IpcEnvironment * getIpcEnvironment() const
    { return cliGlobals_->getEnvironment(); }

  inline MemoryMonitor * getMemoryMonitor () const
    { return cliGlobals_->getMemoryMonitor(); }

  ExUdrServer *acquireUdrServer(const char *runtimeOptions,
                                const char *optionDelimiters,
				NABoolean dedicated = FALSE);

  inline ExUdrServer * getUdrServer() const { return udrServer_; }

  inline LIST(ExUdrServer *) getUdrServersD() const { return udrServersD_; }

  void setUdrConnection(IpcConnection *conn) { udrIpcConnection_ = conn; }
  IpcConnection *getUdrConnection();

  void incrementUdrTxMsgsOut();
  void decrementUdrTxMsgsOut();

  void incrementUdrNonTxMsgsOut();
  void decrementUdrNonTxMsgsOut();

  Lng32 numUdrMsgsOut() const { return numUdrTxMsgsOut_ + numUdrNonTxMsgsOut_; }
  Lng32 numUdrTxMsgsOut() const { return numUdrTxMsgsOut_; }
  Lng32 numUdrNonTxMsgsOut() const { return numUdrNonTxMsgsOut_; }

  inline Int64 &getTransid()                         { return transid_; }

  inline Int64 &getSavepointId()                     { return savepointId_; }

  ResolvedNameList* &resolvedNameList() { return resolvedNameList_; };
  // for accounting for unanswered send top messages sent.
  virtual void decrementSendTopMsgesOut();
  inline void incrementSendTopMsgesOut()   { numSendTopMsgesOut_++; };
  inline NABoolean  anySendTopMsgesOut()   { return numSendTopMsgesOut_ != 0; }
  inline Lng32 numSendTopMsgesOut()        { return numSendTopMsgesOut_; }

  // for accounting for unanswered cancel messages sent thru IPC & ArkFs.
  void decrementCancelMsgesOut();
  inline void incrementCancelMsgesOut()    { numCancelMsgesOut_++; };
  inline NABoolean  anyCancelMsgesOut()    { return numCancelMsgesOut_ != 0; }
  inline Lng32 numCancelMsgesOut()         { return numCancelMsgesOut_; }

  inline NABoolean noNewRequest() { return noNewRequest_; }
  inline virtual void setNoNewRequest(NABoolean n) { noNewRequest_ = n; }

  void setCloseAllOpens(NABoolean v)         { closeAllOpens_ = v; }
  NABoolean closeAllOpens()                  { return closeAllOpens_; }

  // return TRUE iff this timeout was set, and then put value in timeoutValue
  inline NABoolean getLockTimeout( char * tableName, Lng32 & timeoutValue )
  { 
    if ( NULL == timeouts_ ) return FALSE;
    return timeouts_->getLockTimeout( tableName, timeoutValue ); 
  } ;

  // returns a reference to the internal pointer (used inter alia for setting)
  TimeoutData  ** getTimeoutData() { return & timeouts_; } ;

  inline NABoolean grabMemoryQuotaIfAvailable(ULng32 size)
  { 
    CliGlobals *cli_globals = GetCliGlobals();
    if (cli_globals->isEspProcess())
       return cli_globals->grabMemoryQuotaIfAvailable(size);
    if ( unusedBMOsMemoryQuota_ < size ) return FALSE;
    unusedBMOsMemoryQuota_ -= size ;
    return TRUE;
  }

  inline void resetMemoryQuota() 
  {
    CliGlobals *cli_globals = GetCliGlobals();
    if (cli_globals->isEspProcess())
       return cli_globals->resetMemoryQuota();
    unusedBMOsMemoryQuota_ = 0 ; 
  }

  inline ULng32 unusedMemoryQuota() 
  { 
    CliGlobals *cli_globals = GetCliGlobals();
    if (cli_globals->isEspProcess())
       return cli_globals->unusedMemoryQuota();
    return unusedBMOsMemoryQuota_;
  }

  inline void yieldMemoryQuota(ULng32 size) 
  { 
    CliGlobals *cli_globals = GetCliGlobals();
    if (cli_globals->isEspProcess())
       return cli_globals->yieldMemoryQuota(size);
    unusedBMOsMemoryQuota_ += size; 
  }
  
  // getStreamTimeout: return TRUE (FALSE) if the stream-timeout was set (was
  // not set). If set, the timeoutValue parameter would return that value
  virtual NABoolean getStreamTimeout( Lng32 & timeoutValue );

  // Whenever a (prepared) statement is re-executed, the executionCount_
  // is incremented. Right now this counter is used only by the
  // partition access to let the EID session know if a new input buffer
  // represents a continuation of an earlier execution of the statement.
  // This helps dp2 CPU limits work properly if the statement in both 
  // scenarios: 1) the statement is re-exec'd; and 2) the statement has 
  // multiple input buffers to DP2.
  // The increment is done by the root and by the split bottom.
  // NOTE: This is another place where the code assume that an ESP is never
  // on the right-hand side of a flow node (TF, or ONLJ).  Some would say the 
  // correct solution would be to maintain the exec count in the master, 
  // and then pass it to ESPs as dataflow input.

  void incExecutionCount() { executionCount_++; }

  ULng32 getExecutionCount() { return executionCount_; }


  virtual void initSMGlobals();

private:

  // SQL Diagnostics area for this statement
  ComDiagsArea *diagsArea_;

  // Pointer to shared udr server used by scalar udfs
  ExUdrServer *udrServer_;

  // List of dedicated UDR servers associated with this statement
  // Note that this list does not contain the shared udr server
  // pointed to by udrServer_ defined above.This set of dedicated
  // udr servers is used by tmudfs and accessed in statement.cpp
  // when a statement want to assertain that all IO with these servers
  // are completed.
  LIST(ExUdrServer *) udrServersD_;

  // IpcConnection to the UDR Server
  IpcConnection *udrIpcConnection_;

  // transaction identifier, if this statement is running under
  // a transaction. -1, if it is not.
  Int64 transid_;

  // savepoint id, if dp2 savepoints are enabled and in use.
  // -1, if not.
  Int64 savepointId_;

  ResolvedNameList * resolvedNameList_;

  // Keep a count of unanswered send top messages.
  Lng32  numSendTopMsgesOut_;

  // Keep a count of unanswered cancel messages sent thru IPC & ArkFs.
  Lng32  numCancelMsgesOut_;

  // Keep a count of unanswered transactional UDR messages
  Lng32 numUdrTxMsgsOut_;

  // Keep a count of unanswered non-transactional UDR messages
  Lng32 numUdrNonTxMsgsOut_;

  // Set when split bottom gets release transaction
  // Reset when split bottom gets work messge
  NABoolean noNewRequest_;

  NABoolean closeAllOpens_;

  // Hold all the dynamicly set timeout data (relevant to this statement)
  // (Note: This pointer is NULL when there are no relevant timeouts set.)
  TimeoutData  * timeouts_; 

  // memory quota allocation given back by BMOs to be used by other BMOs
  ULng32 unusedBMOsMemoryQuota_;

#if defined(_DEBUG) && defined (TRACE_ESP_ACCESS)
  ESPTraceList *espTraceList_;
#endif

  ULng32 executionCount_;

protected:
  inline void setUdrServer(ExUdrServer *udrServ)      { udrServer_ = udrServ; }

  // points to the executor global object. There is only one
  // executor global for the whole executor session.
  CliGlobals * cliGlobals_;

  // Keep track of whether the SeaMonster query ID was registered
  bool smQueryIDRegistered_;

public:
  enum StmtType {
    DYNAMIC = 0, STATIC = 1};

private:
  StmtType stmtType_;

public:
  StmtType getStmtType();
  void setStmtType(StmtType stmtType);
};

// -----------------------------------------------------------------------
// Both the main thread and the cancel thread can set the state.
// The only state that can be set to by the cancel thread is 
// CLI_CANCEL_REQUESTED.
// The main thread can set the remaining 3 states.
//
// statement::cancel acts based on the state.
// -----------------------------------------------------------------------

typedef enum CancelStateEnum
{
  CLI_CANCEL_TCB_INVALID = 1,    // cancel enable but no tcb.
  CLI_CANCEL_TCB_READY   = 2,    // cancel enable and tcb built.
  CLI_CANCEL_REQUESTED   = 3,    // set by cancel thread to request cancel.
  CLI_CANCEL_DISABLE     = 4     // reject async cancel.  Not retryable.
} CancelState;

class ExMasterStmtGlobals : public ExExeStmtGlobals
{
public:

  ExMasterStmtGlobals(short num_temps,
		      CliGlobals *cliGlobals,
                      Statement *statement,
		      short create_gui_sched = 0,
		      Space * space = NULL,
		      CollHeap * heap = NULL);

  // Deletes objects this object points to... does NOT destroy
  // this object
  virtual void deleteMe(NABoolean fatalError);
  
  virtual ExMasterStmtGlobals * castToExMasterStmtGlobals();

  virtual char * getFragmentPtr(ExFragId fragId) const;
  virtual IpcMessageObjSize getFragmentLength(ExFragId fragId) const;
  virtual ExFragKey getFragmentKey(ExFragId fragId) const;
  virtual ExFragId getMyFragId() const;
  virtual Lng32 getNumOfInstances() const // avoid warning - hidden virtual func
  { return ExExeStmtGlobals::getNumOfInstances(); } 
  virtual Lng32 getNumOfInstances(ExFragId fragId) const;
  virtual const IpcProcessId & getInstanceProcessId(ExFragId fragId,
						    Lng32 instanceNum) const;
  virtual Lng32 getMyInstanceNumber() const;
  virtual void getMyNodeLocalInstanceNumber(
                                       Lng32 &myNodeLocalInstanceNumber,
                                       Lng32 &numOfLocalInstances) const;

  // Virtual methods to retrieve SeaMonster settings
  //
  // In the master executor these settings come from:
  //  Query ID: from statement globals
  //  Trace level: from session defaults
  //  Trace file prefix: from session defaults
  virtual Int64 getSMQueryID() const { return smQueryID_; }
  virtual Int32 getSMTraceLevel() const;
  virtual const char *getSMTraceFilePrefix() const;

  // In the master we allow root_tdb::build() to assign a SeaMonster
  // ID to the query
  void setSMQueryID(Int64 id) { smQueryID_ = id; }

  virtual const ExScratchFileOptions *getScratchFileOptions() const;

  // the frag table is only set in the master executor
  inline void setFragDir(ExFragDir * frag_dir)        {fragDir_ = frag_dir;}
  inline ExFragDir * getFragDir() const                   {return fragDir_;}

  inline void setStartAddr(void * start_addr)       {startAddr_ = start_addr;}
  inline void * getStartAddr() const                      {return startAddr_;}

  inline ExRtFragTable *getRtFragTable() const        { return fragTable_; }
  inline void setRtFragTable(ExRtFragTable *m)           { fragTable_ = m; }

  inline ExEspManager * getEspManager() const {
    return cliGlobals_->getEspManager();
  }

  NABoolean udrRuntimeOptionsChanged() const;

  Int64 getRowsAffected() const           {return rowsAffected_;}
  void setRowsAffected(Int64 newRows)             {rowsAffected_ = newRows;}

  inline Statement *getStatement()                 { return statement_; }

  // For asynchronous CLI cancel.
  inline CancelState getCancelState() const           {return cancelState_;}
  CancelState setCancelState(CancelState newState);
  inline void clearCancelState()   {setCancelState(CLI_CANCEL_TCB_INVALID);}
  void resetCancelState();

  // The following two methods are called in /cli/Statement.cpp :
  // copy timeout data relevant to this stmt (from the global CLI context)
  // (This method is called after the statement was fixed up)
  void      setLocalTimeoutData(ComTdbRoot * rootTdb);
  // check if a previous SET TIMEOUT statement affects this fixedup statement
  // (This method is called before executing a previously fixedup statement)
  NABoolean timeoutSettingChanged();

  ExRsInfo *getResultSetInfo(NABoolean createIfNecessary = FALSE);
  void deleteResultSetInfo();
  void acquireRSInfoFromParent(ULng32 &rsIndex,   // OUT
                               Int64 &udrHandle,         // OUT
                               ExUdrServer *&udrServer,  // OUT
                               IpcProcessId &pid,        // OUT
                               ExRsInfo *&rsInfo);       // OUT

 StatsGlobals *getStatsGlobals() 
 { return (cliGlobals_ ?  cliGlobals_->getStatsGlobals() : NULL); }
 Long getSemId() 
 { return (cliGlobals_ ?  cliGlobals_->getSemId() : (short)0);}
 pid_t getPid() 
 { return (cliGlobals_ ?  cliGlobals_->myPin() : (short)0);}
 pid_t getTid() 
 {
    return syscall(SYS_gettid);
 }

 Lng32 myNodeNumber() { return (cliGlobals_ ?  cliGlobals_->myNodeNumber() : (short)0);}

  inline NABoolean verifyESP() { return verifyESP_; }
  inline void setVerifyESP() { verifyESP_ = TRUE; }
  inline void resetVerifyESP() { verifyESP_ = FALSE; }

  // Methods to manage information related to the top-level ESPs
  // involved in a parallel extract operation
  void insertExtractEsp(const IpcProcessId &);
  void insertExtractSecurityKey(const char *key);
  short getExtractEspCpu(ULng32 index) const;
  Lng32 getExtractEspNodeNumber(ULng32 index) const;
  const char *getExtractEspPhandleText(ULng32 index) const;
  const char *getExtractSecurityKey() const;

  inline void addSMConnection(SMConnection *conn)
            { allSMConnections_.insert(conn); }
  inline void removeSMConnection(SMConnection *conn)
            { allSMConnections_.remove(conn); }
  const LIST(SMConnection *) &allSMConnections() const
            { return allSMConnections_; }
  void setAqrWnrInsertCleanedup() { aqrWnrCleanedup_ = true; }
  void resetAqrWnrInsertCleanedup() { aqrWnrCleanedup_ = false; }
  bool getAqrWnrInsertCleanedup() const { return aqrWnrCleanedup_; }

private:


  // directory containing offsets and types of all fragments.
  ExFragDir * fragDir_;
  
  // pointer to the generated code for all fragments
  void * startAddr_;

  // fragment instance directory for the current statement
  ExRtFragTable *fragTable_;

  // Current statement
  Statement *statement_;

  // rows affected during the execution of this statement.
  // Applies to rows updated/deleted/inserted ONLY.
  Int64 rowsAffected_;

  // Used exclusively by asynchronous CLI cancel.
  CancelState cancelState_;
  
  // local snapshot of the global timeout-change-counter at the time this
  // stmt was fixed up (speeds up checking that timeout values are up-to-date)
  ULng32  localSnapshotOfTimeoutChangeCounter_;

  // Store Procedure Result Set Info
  // will always be NULL except for CALL statements that produce result sets.
  ExRsInfo *resultSetInfo_; 

  // If TRUE, check every existing ESP before reassigning them
  NABoolean verifyESP_;

  // For a parallel extract producer query we need to manage
  // information that describes the collection of top-level ESPs. The
  // collection remains empty if this is not an extract producer query
  struct ExExtractEspInfo
  {
    // Each ESP will be described by one instance of this struct
    short cpu_;
    Lng32 nodeNumber_;
    char *phandleText_;
  };
  struct ExExtractProducerInfo
  {
    // One instance of this struct represents the entire collection of ESPs
    ARRAY(ExExtractEspInfo*) *esps_;
    char *securityKey_;
  };
  ExExtractProducerInfo *extractInfo_;

  // A list of all SM connections used by this master executor. Used
  // for error reporting when an ESP control connection is lost. Each
  // SM connection will be informed of the error and will go into an
  // error state rather than continuing to wait for arrivals.
  LIST(SMConnection *) allSMConnections_;

  // In the master we store the SeaMonster query ID here
  Int64 smQueryID_;

  // Let cli layer know that an NO ROLLBACK insert can be AQR'd.
  bool aqrWnrCleanedup_;
};

class ExEspStmtGlobals : public ExExeStmtGlobals
{
public:

  ExEspStmtGlobals(short num_temps,
                   CliGlobals *cliGlobals,
		   short create_gui_sched,
		   Space * space,
		   CollHeap * heap,
		   ExEspFragInstanceDir *espFragInstanceDir,
		   ExFragInstanceHandle handle,
                   ULng32 injectErrorAtExprFreq,
                   char *queryId = NULL,
                   Lng32 queryIdLen = 0);
  
  virtual void deleteMe(NABoolean fatalError);
  virtual ExEspStmtGlobals * castToExEspStmtGlobals();

  virtual char * getFragmentPtr(ExFragId fragId) const;
  virtual IpcMessageObjSize getFragmentLength(ExFragId fragId) const;
  virtual ExFragKey getFragmentKey(ExFragId fragId) const;
  virtual ExFragId getMyFragId() const;
  virtual Lng32 getNumOfInstances() const // avoid warning - hidden virtual func
  { return ExExeStmtGlobals::getNumOfInstances(); } 
  virtual Lng32 getNumOfInstances(ExFragId fragId) const;
  virtual const IpcProcessId & getInstanceProcessId(ExFragId fragId,
						    Lng32 instanceNum) const;
  virtual Lng32 getMyInstanceNumber() const;
  virtual void getMyNodeLocalInstanceNumber(
       Lng32 &myNodeLocalInstanceNumber,
       Lng32 &numOfLocalInstances) const;
  ExFragInstanceHandle getMyFragInstanceHandle() const { return myHandle_; }

  // Virtual methods to retrieve SeaMonster settings
  // 
  // In an ESP these settings come from an ExSMDownloadInfo object
  // that was sent with the fragment download message and is pointed
  // to by statement globals
  virtual Int64 getSMQueryID() const;
  virtual Int32 getSMTraceLevel() const;
  virtual const char *getSMTraceFilePrefix() const;

  // This method will store a pointer to an object containing
  // SeaMonster info for this query (e.g., query ID, trace level,
  // ...). The object is sent from the master executor as part of the
  // download message.
  void setSMDownloadInfo(ExSMDownloadInfo *info)
  { smDownloadInfo_ = info; }

  virtual const ExScratchFileOptions *getScratchFileOptions() const;

  // a Process Ids of Fragments list gets only sent to ESP fragment instances
  inline ExProcessIdsOfFragList *getPidFragList() const
                                           { return processIdsOfFragList_; }
  inline void setPidFragList(ExProcessIdsOfFragList *p)
                                              { processIdsOfFragList_ = p; }

  inline const ExMsgResourceInfo * getResourceInfo() const
                                                   { return resourceInfo_; }
  inline void setResourceInfo(ExMsgResourceInfo *r)   { resourceInfo_ = r; }

  // deal with the transaction ids that come in with client requests

  // set the reply tag that is associate with the transaction work request
  // for this instance
  void setReplyTag(Int64 transid, short replyTag);

  // restore the transid associated with this instance and return whether
  // this was possible (not possible if no current transaction work request)
  NABoolean restoreTransaction();

  // manage a list of the send top TCBs in this fragment instance, so that
  // dependent ESPs can be notified if we lose interest in this statement
  // because of a cancel operation
  CollIndex registerSendTopTcb(ex_send_top_tcb *st);
  void setSendTopTcbActivated(CollIndex id) { activatedSendTopTcbs_ += id; }
  void setSendTopTcbLateCancelling();
  void clearAllActivatedSendTopTcbs()     { activatedSendTopTcbs_.clear(); }
  void resetSendTopTcbLateCancelling();

  ex_send_top_tcb * getFirstNonActivatedSendTop(CollIndex &i)
                            { i = 0; return getNextNonActivatedSendTop(i); }
  ex_send_top_tcb * getNextNonActivatedSendTop(CollIndex &i);

  virtual void setNoNewRequest(NABoolean n);

  virtual void decrementSendTopMsgesOut();
  
 StatsGlobals *getStatsGlobals() 
      { return espFragInstanceDir_->getStatsGlobals(); }
 Long getSemId() 
      { return espFragInstanceDir_->getSemId();}
 pid_t getPid() 
      { return espFragInstanceDir_->getPid();}
pid_t getTid() 
      { return espFragInstanceDir_->getTid();}

  NABoolean isAnESPAccess() {return isAnESPAccess_;}
  void setIsAnESPAccess(NABoolean a) { isAnESPAccess_ = a;}

  char *getQueryId() { return queryId_; }
  Lng32 getQueryIdLen() { return queryIdLen_; }
  inline StmtStats *getStmtStats() { return stmtStats_; }
  StmtStats *setStmtStats();
  void setMyFixupPriority(IpcPriority v)     { myFixupPriority_ = v; }
  IpcPriority getMyFixupPriority()           { return myFixupPriority_; }

private:


  // my own fragment instance handle
  // (using a long here to avoid exposing the entire ESP frag instance
  // dir class to this header file)
  ExFragInstanceHandle myHandle_;

  // pointer back the the ESP's fragment instance directory
  // (use this only to get pointers to fragments)
  ExEspFragInstanceDir *espFragInstanceDir_;

  // fragment instances of input fragments to this ESP (used in ESPs)
  ExProcessIdsOfFragList *processIdsOfFragList_;

  // downloaded resource info
  ExMsgResourceInfo *resourceInfo_;

  // reply tag is used to restore the transid for this reply tag
  short replyTag_;

  // list of the send top TCBs in this fragment instance, needed for
  // cancel propagation to producer ESPs
  ARRAY(ex_send_top_tcb *) sendTopTcbs_;
  // the subset of the send top TCBs that got a request
  SUBARRAY(ex_send_top_tcb *) activatedSendTopTcbs_;

  // This is the priority ESPs run at when waiting for a 'fixup' message.
  // After fixup message is received, the priority is changed to its
  // 'execute' priority which is sent as part of the fixup message.
  // Once ESPs are done executing and the release transaction msg is
  // received, priority is changed back to the 'fixup' priority value.
  IpcPriority myFixupPriority_;

  char *queryId_;
  Lng32 queryIdLen_;
  StmtStats *stmtStats_;
  NAHeap *heap_;

  NABoolean isAnESPAccess_;

  // A pointer to an object containing SeaMonster properties for the
  // query
  ExSMDownloadInfo *smDownloadInfo_;
};

#endif
