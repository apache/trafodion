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
 * File:         ex_exe_stmt_globals.h
 * Description:  statement globals for non-DP2 environments (master, ESP)
 *
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
#include "Platform.h"

#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ex_exe_stmt_globals.h"
#include "ex_frag_rt.h"
#include "Ex_esp_msg.h"
#include "ex_esp_frag_dir.h"
#include "LateBindInfo.h"
#include "cli_stdh.h"
#include "ExUdrServer.h"
#include "UdrExeIpc.h"
#include "ExRsInfo.h"
#include "ex_send_top.h"
#include "SqlStats.h"
#include "Globals.h"
#include "ExSMTrace.h"
#include "ExSMCommon.h"
#include "ExSMGlobals.h"
#include "ExSMEvent.h"

class ComTdbRoot;


#if defined(_DEBUG) && defined(TRACE_ESP_ACCESS)

#include "ComCextdecs.h"
// Comment in and build to trace an ESPAccess ESP process
//#define TRACE_ESP_ACCESS 1


ESPTraceEntry::ESPTraceEntry(ex_globals *globals,
			     Lng32 espNum, 
			     Lng32 pid,
			     Int64 currentTS,
			     char* t1)
: espNum_(espNum),
  pid_(pid),
  timestamp_(currentTS),
  globals_(globals)
{ 
  Int32 len = str_len(t1);
  msgtext1_ = new(globals_->getDefaultHeap()) char[len+1];
  str_cpy_all(msgtext1_, t1, len + 1);
}

ESPTraceEntry::ESPTraceEntry(ex_globals *glob, char *t1) 
  : globals_(glob)
{ 
  ExExeStmtGlobals *exeGlob = globals_->castToExExeStmtGlobals();
  ExEspStmtGlobals *espGlob = exeGlob->castToExEspStmtGlobals();
  
  espNum_ = espGlob->getMyInstanceNumber();

  timestamp_ = CONVERTTIMESTAMP(JULIANTIMESTAMP(0,0,0,-1),0,-1,0);
  
  ULng32 pid =-1;
  pid_ = exeGlob->getPid();

  Int32 len = str_len(t1);
  msgtext1_ = new(globals_->getDefaultHeap()) char[len+1];
  str_cpy_all(msgtext1_, t1, len + 1);
    
}

ESPTraceEntry::~ESPTraceEntry()
{
  pid_ = 0;
  espNum_ = 0;
  timestamp_ = 0;
  if(msgtext1_){
    NADELETEBASIC(msgtext1_, globals_->getDefaultHeap());
    msgtext1_ = NULL;
  }
}

void ESPTraceEntry::createMessage(char *message)
{
  short timestamp[8];
  char timeBuf[100];

  INTERPRETTIMESTAMP(timestamp_, timestamp);

  short year    = timestamp[0];
  char month    = (char) timestamp[1];
  char day      = (char) timestamp[2];
  char hour     = (char) timestamp[3];
  char minute   = (char) timestamp[4];
  char second   = (char) timestamp[5];
  Lng32 fraction = timestamp[6] * 1000 + timestamp[7];

  str_sprintf (timeBuf,  "%04u-%02u-%02u %02u:%02u:%02u.%03u",
               year, month, day, hour, minute, second, fraction);

  sprintf(message, "%s PID: %d ESP#: %d %s ",
	  timeBuf,
	  pid_,
	  espNum_,
	  msgtext1_);

}

ESPTraceList::~ESPTraceList() 
{ 
  clearAndDestroy(); 
}

// Remove all entries the list and call their destructors
void ESPTraceList::clearAndDestroy()
{
  for (ULng32 i = 0; i < entries(); i++) {
    ESPTraceEntry *entry = (ESPTraceEntry *) at(i);
    remove(entry);
    NADELETE(entry, ESPTraceEntry, globals_->getDefaultHeap());
  }
  clear();
}

 
void ESPTraceList::insertNewTraceEntry(char *msg)
{
  ExExeStmtGlobals *exeGlob = globals_->castToExExeStmtGlobals();
  ExEspStmtGlobals *espGlob = exeGlob->castToExEspStmtGlobals();

  if (NOT espGlob->isAnESPAccess())
    return;

  if (!traceOn_)
    return;
  
  ESPTraceEntry *traceEntry = new (globals_->getDefaultHeap()) ESPTraceEntry(globals_, msg);

  insert(traceEntry);
}

void ESPTraceList::logESPTraceToFile(char *fn, char *signature, ESPTraceList &traceList)
{
  // Do not trace unless the user specifically
  // set the tracing on for an ESP access process

#ifdef TRACE_ESP_ACCESS
    traceOn_ = TRUE;
#endif
  
  // Do not open the log unless
  // tracing has been set on.

  if (!traceOn_)
    return;

  // For now hardcode the log file name
  FILE *traceFile = NULL;
  char *sgTraceFileName = NULL;
  // get env doesn't work on ESP process

  short fnum;
  sgTraceFileName = (fn ? fn : "SYSTEM.SYSTEM.SGLOG");
  Int32 ferr = FILE_OPEN_(sgTraceFileName, 
			strlen(sgTraceFileName), 
			&fnum,
			(UInt16)2, // Write Access
			(UInt16)1, // Exclusive
			0 //OMIT // waited
			);

  char msg[200];
  sprintf(msg, "Unable to open ESPTrace file - %s", sgTraceFileName);
  ex_assert((ferr==0), msg);

 // Do this for every ESPTraceEntry in the ESPTrace
  for (CollIndex i=0; i<traceList.entries(); i++)
  {
    ESPTraceEntry *entry = traceList[i];
    char entryMsg[256];
    entry->createMessage(entryMsg);
  
    char msg[300];
    sprintf(msg, " %s %s \n", signature, entryMsg);

    WRITEX(fnum,
	   msg,
	   sizeof(msg)
	   );    
  }
  FILE_CLOSE_(fnum);




}

#endif


// -----------------------------------------------------------------------
// Methods for class ExExeStmtGlobals
// -----------------------------------------------------------------------

ExExeStmtGlobals::ExExeStmtGlobals(short num_temps,
                                   CliGlobals *cliGlobals,
				   short create_gui_sched,
				   Space * space,
				   CollHeap * heap)
  : ex_globals(num_temps, create_gui_sched, space, heap),
    cliGlobals_(cliGlobals),
    udrServer_(NULL),
    udrIpcConnection_(NULL),
    numSendTopMsgesOut_(0),
    numCancelMsgesOut_(0),
    numUdrTxMsgsOut_(0),
    numUdrNonTxMsgsOut_(0),
    timeouts_( NULL ), 
    transid_(-1),
    savepointId_(-1),
    stmtType_(DYNAMIC),
    unusedBMOsMemoryQuota_(0),    
    noNewRequest_(FALSE),
    closeAllOpens_(FALSE),
    executionCount_(0),
    udrServersD_(getDefaultHeap()),
    smQueryIDRegistered_(false)
{
  if (cliGlobals)
    setEventConsumed(cliGlobals->getEventConsumed());
  diagsArea_ = NULL;
  resolvedNameList_ = NULL;

#if defined(_DEBUG) && defined (TRACE_ESP_ACCESS)
  espTraceList_ = new(getDefaultHeap()) ESPTraceList(this, getDefaultHeap());
#endif
  
  
}

// Warning! Despite the name, the following method does NOT delete
// this object. Rather, it cleans up things this object points to...

void ExExeStmtGlobals::deleteMe(NABoolean fatalError)
{
  while (!fatalError && anyCancelMsgesOut())
    {
      // work may have finished before a cancel request was answered
      // by some ESP or exe-in-dp2.  However, this little loop will
      // not ensure that work requests are answered before we proceed
      // to deleteMe.  The insurance that work msgs are answered first
      // comes from our requirement that the root_tcb has gotten its
      // Q_NO_DATA before the statement is dealloc'd.

      if (getSMQueryID() > 0)
        EXSM_TRACE(EXSM_TRACE_CANCEL, 
                   "ExExeStmtGlobals::deleteMe: outstanding %d - waiting", 
                   (int) numCancelMsgesOut());
      
      getIpcEnvironment()->getAllConnections()->waitOnAll();
    }

  // Note about cleanup after fatalError: When a fatalError happens,
  // we can no longer user the data connections and ex_queues. The 
  // query must be deallocated; it cannot be simply reexecuted. So 
  // it is unreliable to depend on ESPs to reply to data messages.
  // Therefore we skip it. What happens to the connections to ESPs?
  // Any pending messages are subjected to BCANCELREQ when they
  // timeout (from Statement::releaseTransaction or ex_root_tcb::cancel).
  // The data connections are closed as part of the destructor of 
  // the send top tcbs.  The control connections are closed as 
  // part of ExEspManager::releaseEsp and this will cause any
  // functioning ESP to exit when it get the system close message.
  // Any hanging ESP will stick around. If it ever comes out of the
  // hang, it will check for the system close message and exit.


  // Release the SeaMonster query ID. It is important that this step
  // happens before TCB destructors run. If TCB destructors run before
  // the ID is released, the reader thread could see arrivals for a
  // connection or buffer that has already gone away.
  Int64 smQueryID = getSMQueryID();
  if (smQueryID > 0 && smQueryIDRegistered_)
  {
    ExSM_Cancel(smQueryID);
    smQueryIDRegistered_ = false;
  }

  // This deleteMe() call will trigger the TCB destructors
  ex_globals::deleteMe(fatalError);

  // After TCB destructors run, wait for UDR messages to complete
  // before releasing this statement globals instance. If we don't
  // wait for UDR messages, UDR IPC callbacks might take place after
  // this statement globals instance is released. A UDR callback might
  // dereference its statement globals pointer, which now points to an
  // invalid address.
  if (numUdrMsgsOut() > 0)
  {
    while (numUdrMsgsOut() > 0)
    {
      // First attempt to complete requests on scalar udr server
      ExUdrServer *udrServ = getUdrServer();
      IpcConnection *conn = getUdrConnection();
      if(conn != NULL && udrServ != NULL)
      {
        udrServ->completeUdrRequests(conn, FALSE);
      }

      // Next, attempt to complete requests on dedicated udr servers
      // if there are any.
      for (CollIndex i = 0; i < udrServersD_.entries(); i++)
      {
        ExUdrServer *udrServ = udrServersD_[i];
        IpcConnection *conn = udrServ->getUdrControlConnection();
        udrServ->completeUdrRequests(conn, FALSE);
      }
    }
  }

  // Next, attempt to reset inUse flag of each dedicated udr server
  // that was acquired by this statement. Resetting this flag will allow
  // contextCli to reuse the server for subsequent statements
  for (CollIndex i = 0; i < udrServersD_.entries(); i++)
  {
    ExUdrServer *udrServ = udrServersD_[i];
    udrServ->setInUse(FALSE);
  }

  if (diagsArea_)
    {
      // decrRefCount methods deallocates the diagsArea_ if
      // its ref count goes down to 0.
      if (diagsArea_->decrRefCount() == 0)
	diagsArea_ = NULL;
    }

  //  if (resolvedNameList())
  //  getDefaultHeap()->deallocateMemory(resolvedNameList());

  // don't be fooled! The next statement does nothing... (don't know why)
  deleteMemory(this);

#if defined(_DEBUG) && defined (TRACE_ESP_ACCESS)
  if (espTraceList_)
    {
      NADELETE(espTraceList_, ESPTraceList, getDefaultHeap());
      espTraceList_ = NULL;
    }
#endif 
}

NABoolean ExExeStmtGlobals::closeTables()
{
  CollIndex numTcbs = tcbList().entries();
  
  for (CollIndex i = 0; i < numTcbs; i++ )
    {
      // Check for returned errors. TBD.
      tcbList()[i]->closeTables();
    }

  return FALSE;
}

NABoolean ExExeStmtGlobals::reOpenTables()
{
  CollIndex numTcbs = tcbList().entries();
  
  for (CollIndex i = 0; i < numTcbs; i++ )
    {
      // Check for returned errors. TBD.
      tcbList()[i]->reOpenTables();
    }

  return FALSE;
}

ExExeStmtGlobals * ExExeStmtGlobals::castToExExeStmtGlobals()
{
  return this;
}

ExMasterStmtGlobals * ExExeStmtGlobals::castToExMasterStmtGlobals()
{
  return NULL;
}

ExEspStmtGlobals * ExExeStmtGlobals::castToExEspStmtGlobals()
{
  return NULL;
}

/*
--
--   Calls to this method should not be confused with calls
--   to method atp_struct::setDiagsArea. Callers of this 
--   method should make sure to decrement the reference 
--   counter of the Diags if the latter was just created
--   prior to calling this method. The code fragment below
--   presents an example:
--
--     da = ComDiagsArea::allocate(glob_->getDefaultHeap());
--         << refCount is 1 after this call
--     glob_->setGlobDiagsArea(da);
--         << refCount is 2 after this call
--     da->decrRefCount();
--         << refCount is back to 1 after this call
--
--   while in this example you don't need to decrement the
--   reference count.
--
--     cliDA = glob_->getGlobDiagsArea();
--         << refCount is N before and after this call
--     cliDA.mergeAfter(*diagsArea);
--         << refCount is N before and after this call
--     glob_setGlobDiagsArea(cliDA);
--         << refCount is N before and after this call
--
*/
void ExExeStmtGlobals::setGlobDiagsArea(ComDiagsArea *da)
{
  // first secure new diags area, then give up the old one
  if (da)
    da->incrRefCount();

  if (diagsArea_)
    diagsArea_->decrRefCount();

  diagsArea_ = da;
}

void ExExeStmtGlobals::takeGlobalDiagsArea(ComDiagsArea &cliDA)
{
  // take my DA, if any, and merge it into
  // to the diags area passed in.  Then release my DA.
  if (diagsArea_)
    {
      cliDA.mergeAfter(*diagsArea_);
      setGlobDiagsArea(NULL);
    }

}

Lng32 ExExeStmtGlobals::getNumOfInstances() const
{
  // define the simple method by calling the more general method
  return getNumOfInstances(getMyFragId());
}

NABoolean ExExeStmtGlobals::getStreamTimeout( Lng32 & timeoutValue )
{ 
  if ( NULL == timeouts_ || ! timeouts_->isStreamTimeoutSet() ) return FALSE; 
  timeoutValue = timeouts_->getStreamTimeout();
  return TRUE;
};

// Ask the SQL context for an ExUdrServer pointer and store it in this
// instance, if an ExUdrServer is not already associated with this
// instance.
ExUdrServer * ExExeStmtGlobals::acquireUdrServer(const char *runtimeOptions,
                                                 const char *optionDelimiters,
						 NABoolean dedicated)
{
#ifdef UDR_DEBUG
  NABoolean doDebug =
    (getenv("UDR_SERVER_MGR_DEBUG") || getenv("UDR_DEBUG")) ? TRUE : FALSE;
  FILE *f = stdout;
  if (doDebug)
  {
    UdrPrintf(f, "[BEGIN ExExeStmtGlobals::acquireUdrServer()]");
    UdrPrintf(f, "  this %p, udrServer_ %p", this, getUdrServer());
    UdrPrintf(f, "  options '%s'", runtimeOptions);
    if (getUdrServer())
    {
      UdrPrintf(f, "*** WARNING: Re-acquiring a server for this statement");
    }
  }
#endif

  // This method gets called by the UDR TCB constructor. We should not
  // be constructing a new TCB tree if the statement currently has
  // outstanding UDR messages. This is guaranteed by the fact that TCB
  // teardown logic in the Statement class always waits for completion
  // of UDR messages. To be safe we also add an assertion here.
  ex_assert(numUdrMsgsOut() == 0,
            "Cannot acquire a new UDR server while messages are outstanding");
 
  ContextCli *context = getContext();
  ExUdrServer *udrServ =
    context->acquireUdrServer(runtimeOptions, optionDelimiters,dedicated);

#ifdef UDR_DEBUG
  if (doDebug)
  {
    UdrPrintf(f, "  Acquired udrServ now is %p", udrServ);
    UdrPrintf(f, "[END ExExeStmtGlobals::acquireUdrServer()]");
  }
#endif
  // Dedicated servers are currently used only by tmudfs. We keep
  // tmudf servers in a separate list and not change the behavior
  // of keeping a pointer to a shared server as in the case of scalar
  // udfs.
  if(dedicated)
  {
    udrServ->setInUse(TRUE);
    udrServersD_.insert(udrServ);
  }
  else
  {
    setUdrServer(udrServ);
  }

  return udrServ;
}

IpcConnection * ExExeStmtGlobals::getUdrConnection()
{
  if (udrIpcConnection_)
    return udrIpcConnection_;
  else
  {
    ExUdrServer *udrServ = getUdrServer();
    if(udrServ)
      return udrServ->getUdrControlConnection();
    else 
      return NULL;
  }
}

void ExExeStmtGlobals::decrementSendTopMsgesOut()
{
  ex_assert(numSendTopMsgesOut_ > 0,
            "Send top message counter should not drop below zero");
  numSendTopMsgesOut_--;
}

void ExExeStmtGlobals::decrementCancelMsgesOut()
{
  ex_assert(numCancelMsgesOut_ > 0,
            "Cancel message counter should not drop below zero");
  numCancelMsgesOut_--;
}

// -----------------------------------------------------------------------
// Methods for class ExMasterStmtGlobals
// -----------------------------------------------------------------------

ExMasterStmtGlobals::ExMasterStmtGlobals(
     short num_temps,
     CliGlobals *cliGlobals,
     Statement *statement,
     short create_gui_sched,
     Space * space,
     CollHeap * heap) : ExExeStmtGlobals(num_temps,
                                         cliGlobals,
                                         create_gui_sched,
                                         space,
                                         heap)
                      , allSMConnections_(heap)
                      , smQueryID_(0)
                      , aqrWnrCleanedup_(false)
{
  fragDir_   = NULL;
  startAddr_ = 0;
  fragTable_ = NULL;
  statement_ = statement;
  rowsAffected_ = 0;
  cancelState_ = CLI_CANCEL_TCB_INVALID;
  resultSetInfo_ = NULL;
  extractInfo_ = NULL;
  verifyESP_ = FALSE;
 
#ifdef _DEBUG
  char *testCancelFreq  = getenv("TEST_ERROR_AT_EXPR");
  if (testCancelFreq)
    {
      Int32 freq = atoi(testCancelFreq);
      if (freq < 0)
        freq = 0;
      if (freq  != 0)
        {
        Int32 i = 1;
        while ( i <= freq)
          i = i << 1;  
        freq = i >> 1;
        }
      setInjectErrorAtExpr(freq);
    }
#endif

  localSnapshotOfTimeoutChangeCounter_ = (ULng32) -1 ;
}


// Warning! Despite the name, this method does NOT destroy this
// object... rather it cleans up the things it points to

void ExMasterStmtGlobals::deleteMe(NABoolean fatalError)
{
  
   // Deallocate all the parallel extract bookkeeping structures
  if (extractInfo_)
  {
    NAMemory *h = getDefaultHeap();

    if (extractInfo_->securityKey_)
      h->deallocateMemory(extractInfo_->securityKey_);

    if (extractInfo_->esps_ != NULL)
    {
      ARRAY(ExExtractEspInfo*) *espList = extractInfo_->esps_;
      CollIndex lastEntry = espList->getSize();
      CollIndex i;
      for (i = 0; i < lastEntry; i++)
      {
        if (espList->used(i))
        {
          ExExtractEspInfo *esp = (*espList)[i];
          if (esp)
          {
            if (esp->phandleText_)
              h->deallocateMemory(esp->phandleText_);
            h->deallocateMemory(esp);
          }
        }
      }

      delete extractInfo_->esps_;
    }
    h->deallocateMemory(extractInfo_);

    // reset extractInfo_ so in case this ExMasterStmtGlobals is reused,
    // we know to create a new extractInfo_ object.
    extractInfo_ = NULL;
  }

  // Warning! Despite the name, the following call does NOT delete
  // this object.
  ExExeStmtGlobals::deleteMe(fatalError);  

  // $$$$ Note: the base class actually calls the destructor for the
  // object hanging off fragTable_. Two things should be done:
  // a) reset the pointer and delete the object from the same
  // place, and, b) don't modify data members after calling deleteMe().
  fragDir_ = NULL;    // clean up pointers to objects that will
  fragTable_ = NULL;  // be deleted below 
}

ExMasterStmtGlobals * ExMasterStmtGlobals::castToExMasterStmtGlobals()
{
  return this;
}

char * ExMasterStmtGlobals::getFragmentPtr(ExFragId fragId) const
{
  ex_assert(getFragDir() AND getStartAddr(),
	    "Frag dir and starting address must be set first");
  return (char *) ((char *)getStartAddr() + getFragDir()->getGlobalOffset(fragId));
}

IpcMessageObjSize ExMasterStmtGlobals::getFragmentLength(
     ExFragId fragId) const
{
  ex_assert(getFragDir(),"Frag dir must be set first");
  return getFragDir()->getFragmentLength(fragId);
}

ExFragKey ExMasterStmtGlobals::getFragmentKey(ExFragId fragId) const
{
  // get the fragment key for this fragment
  ExFragKey result = fragTable_->getMasterFragKey();

  // then change the fragment id to the one we are looking for
  result.setFragId(fragId);

  return result;
}

ExFragId ExMasterStmtGlobals::getMyFragId() const
{
  return 0; // master is always fragment id 0
}

Lng32 ExMasterStmtGlobals::getNumOfInstances(ExFragId fragId) const
{
  return fragTable_->getNumOfInstances(fragId);
}

const IpcProcessId & ExMasterStmtGlobals::getInstanceProcessId(
     ExFragId fragId,
     Lng32 instanceNum) const
{
  return fragTable_->getInstanceProcessId(fragId,instanceNum);
}

Lng32 ExMasterStmtGlobals::getMyInstanceNumber() const
{
  // there is only one master and it's instance number is therefore 0
  return 0;
}

void ExMasterStmtGlobals::getMyNodeLocalInstanceNumber(
       Lng32 &myNodeLocalInstanceNumber,
       Lng32 &numOfLocalInstances) const
{
  // I'm number one (zero in geek-speak) and there is only one master
  myNodeLocalInstanceNumber = 0;
  numOfLocalInstances = 1;
}

const ExScratchFileOptions *ExMasterStmtGlobals::getScratchFileOptions() const
{
  return fragDir_->getScratchFileOptions();
}

// -----------------------------------------------------------------------
// Both the main thread and the cancel thread call this.  
// Rules for consistent access:
// - The only state that can be set to by the cancel thread is 
//   CLI_CANCEL_REQUESTED.
// - The main thread can set the remaining 3 states.
// - The cancel thread reads and writes the state within the same
//   critical section set up in the caller.
// - Ready to take a quiz? [EL]
// -----------------------------------------------------------------------
CancelState ExMasterStmtGlobals::setCancelState(CancelState newState)
{
  CancelState old = cancelState_;

  if (old == CLI_CANCEL_DISABLE)
    return old;

  switch(newState)
    {
    case CLI_CANCEL_REQUESTED:
      cancelState_ = newState;
      return old;

    case CLI_CANCEL_TCB_READY:
      if (old == CLI_CANCEL_REQUESTED)
	{
	  cancelState_ = newState;
	  return old;
	}
      break;

    case CLI_CANCEL_TCB_INVALID:
      // If the current state is not CLI_CANCEL_TCB_INVALID, 
      // acquire a critical section to exclude possible 
      // reference of ex_root_tcb by the cancel thread.
      // ---------------------------------------------------------
      if (old == CLI_CANCEL_TCB_INVALID)
	return old;  

      break;

    case CLI_CANCEL_DISABLE:
      break;
    }
  
  // Acquire a critical section in all other cases.
  getContext()->semaphoreLock();           
  old = cancelState_;
  cancelState_ = newState;
  getContext()->semaphoreRelease();  
       
  return old;
}

void ExMasterStmtGlobals::resetCancelState()
{
  if ((cancelState_ == CLI_CANCEL_TCB_INVALID ||
       cancelState_ == CLI_CANCEL_DISABLE))
    return;

  getContext()->semaphoreLock();  
  if (cancelState_ != CLI_CANCEL_REQUESTED)
    cancelState_ = CLI_CANCEL_TCB_INVALID;
  getContext()->semaphoreRelease();  
}

// this method is called once, after fixup, to copy relevant timeout data
void ExMasterStmtGlobals::setLocalTimeoutData(ComTdbRoot * rootTdb)
{
  // First thing -- keep the current value of the global change counter
  localSnapshotOfTimeoutChangeCounter_ = 
    getContext()->getTimeoutChangeCounter();

  // second -- deallocate local TD (when this stmt was deallocated + refixedup)
  TimeoutData ** localTimeoutData = getTimeoutData() ;
  if ( NULL != *localTimeoutData ) {  // remove a previous TD
    delete *localTimeoutData ;
    *localTimeoutData = NULL ;
  }

  // get the global timeout-data (FALSE: do not allocate it if it's NULL)
  TimeoutData * globalTimeouts = getContext()->getTimeouts( FALSE );
  
  if ( NULL == globalTimeouts ) return;  // a common case: no dynamic timeouts

  // copy the relevant data from CLI context into this statement's globals
  globalTimeouts->copyData( localTimeoutData , getDefaultHeap() , rootTdb );
}

// return TRUE iff some relevant timeout was changed dynamically
// (This method is called before each execution of a fixedup stmt)
NABoolean ExMasterStmtGlobals::timeoutSettingChanged()
{
  // a quick check against the "global change counter" (a logical timestamp)
  if ( localSnapshotOfTimeoutChangeCounter_ == 
       getContext()->getTimeoutChangeCounter() )
    return FALSE ; // this statement must be up to date (the most common case)
  
  // update the local change counter; in case the global change was irrelevant
  // this way subsequent calls may succeed by using the quick check above
  // (if the change was relevant; this statement is deallocated anyhow)
  localSnapshotOfTimeoutChangeCounter_ = 
    getContext()->getTimeoutChangeCounter();

  // get the global timeout-data (FALSE: do not allocate it if it's NULL)
  TimeoutData * globalTimeouts = getContext()->getTimeouts( FALSE );
  TimeoutData *  LocalTimeouts = * getTimeoutData() ;
 
  if ( NULL == globalTimeouts ) // if both NULL -- no change -- return FALSE
    return ( NULL != LocalTimeouts ); // local not NULL -- change -- return TRU

  ComTdbRoot * rootTdb = (ComTdbRoot *) getFragmentPtr(0) ;

  // So the global "change counter" changed; check if the change affects us
  // if we are NOT up-to-date, then the setting has changed.
  return  ! globalTimeouts->isUpToDate( LocalTimeouts , rootTdb );
}

// Return TRUE if UDR runtime options in the context were changed
// dynamically and now differ from those options associated with this
// statement's udrServer_ object (if one exists). This method is
// called before each execution of an already fixed up statement.
NABoolean ExMasterStmtGlobals::udrRuntimeOptionsChanged() const
{
  NABoolean changed = FALSE;
  ExUdrServer *udrServer = getUdrServer();

  if (udrServer)
  {
    ContextCli *context = getContext();

    const char *newOptions = context->getUdrRuntimeOptions();
    if (newOptions)
    {
      const char *newDelims = context->getUdrRuntimeOptionDelimiters();
      const char *oldOptions = udrServer->getOptions();
      const char *oldDelims = udrServer->getOptionDelimiters();

      // The following str_cmp_ne calls tolerate NULL input
      // values. Comparisons fail if one input is NULL and the other
      // is non-NULL. We should never encounter NULL option strings
      // though. This comment is only being made to show that the code
      // below is safe.
      if (str_cmp_ne(oldOptions, newOptions) != 0 ||
          str_cmp_ne(oldDelims, newDelims) != 0)
      {
        changed = TRUE;
      }

    } // if (newOptions)
  } // if (udrServer)

  return changed;
}

ExRsInfo * ExMasterStmtGlobals::getResultSetInfo(NABoolean createIfNecessary)
{
  if(createIfNecessary && resultSetInfo_ == NULL)
      resultSetInfo_ = new (getDefaultHeap()) ExRsInfo();
  return resultSetInfo_;
}

void ExMasterStmtGlobals::deleteResultSetInfo()
{
  if(resultSetInfo_ != NULL)
  {
    delete resultSetInfo_;
    resultSetInfo_ = NULL;
  }
}

void
ExMasterStmtGlobals::acquireRSInfoFromParent(ULng32 &rsIndex,   // OUT
                                             Int64 &udrHandle,         // OUT
                                             ExUdrServer *&udrServer,  // OUT
                                             IpcProcessId &pid,        // OUT
                                             ExRsInfo *&rsInfo)        // OUT
{
  Statement *myStatement = getStatement();
  ex_assert(myStatement, "No Statement available for RS info");

  Statement *parentCall = myStatement->getParentCall();
  ex_assert(parentCall, "No parent CALL available for RS info");

  ExMasterStmtGlobals *otherGlobals = parentCall->getGlobals();
  ex_assert(otherGlobals, "No parent globals available for RS info");

  rsInfo = otherGlobals->getResultSetInfo();
  ex_assert(rsInfo, "No parent RS info available");

  rsIndex = rsInfo->getIndex(myStatement);
  pid = rsInfo->getIpcProcessId();
  udrHandle = rsInfo->getUdrHandle();

  udrServer = rsInfo->getUdrServer();
  ex_assert(udrServer, "No UDR server available in parent");
  setUdrServer(udrServer);

  ex_assert(otherGlobals->getUdrConnection(),
            "No connection to UDR server is available in parent");
  setUdrConnection(otherGlobals->getUdrConnection());
}

// Populate the parallel extract bookkeeping structures with new
// information describing one of the top-level ESPs
void ExMasterStmtGlobals::insertExtractEsp(const IpcProcessId &pid)
{
  NAMemory *h = getDefaultHeap();

  if (extractInfo_ == NULL)
  {
    extractInfo_ = (ExExtractProducerInfo *)
      h->allocateMemory(sizeof(ExExtractProducerInfo));
    memset(extractInfo_, 0, sizeof(ExExtractProducerInfo));
  }

  if (extractInfo_->esps_ == NULL)
    extractInfo_->esps_ = new (h) ARRAY(ExExtractEspInfo *)(h);

  ExExtractEspInfo *esp = (ExExtractEspInfo *)
    h->allocateMemory(sizeof(ExExtractEspInfo));
  memset(esp, 0, sizeof(ExExtractEspInfo));

  // Here is where we insert the new esp object into the esps_
  // array. We don't store the elements in any particular order. We
  // will find the first unused index in the array and put the new
  // element there.
  CollIndex idx = extractInfo_->esps_->unusedIndex();
  extractInfo_->esps_->insertAt(idx, esp);

  char pidBuf[300];
  pid.toAscii(pidBuf, 300);
  Lng32 len = str_len(pidBuf);

  const GuaProcessHandle &phandle = pid.getPhandle();
  Int32 cpu = -1, pin = -1;
  Int32 nodeNumber = -1;
  SB_Int64_Type seqNum = 0;
  Lng32 guaError = phandle.decompose(cpu, pin, nodeNumber
                                    , seqNum
                                    );
  if (guaError != 0)
  {
    char msg[100];
    str_sprintf(msg, "Unexpected error %d from DECOMPOSE", (Int32) guaError);
    ex_assert(guaError == 0, msg);
  }

  esp->phandleText_ = (char *) h->allocateMemory(len + 1);
  str_cpy_all(esp->phandleText_, pidBuf, len + 1);
  esp->cpu_ = cpu;
  esp->nodeNumber_ = nodeNumber;
  // tbd - parallel extract - extract master executor will need to use 
  // verifier as part of process name. Need to test this and see if it 
  // is happening correctly. Maybe defer until we support parallel extract.
}

void ExMasterStmtGlobals::insertExtractSecurityKey(const char *key)
{
  NAMemory *h = getDefaultHeap();

  if (extractInfo_ == NULL)
  {
    extractInfo_ = (ExExtractProducerInfo *)
      h->allocateMemory(sizeof(ExExtractProducerInfo));
    memset(extractInfo_, 0, sizeof(ExExtractProducerInfo));
  }

  if (extractInfo_->securityKey_ != NULL)
  {
    h->deallocateMemory(extractInfo_->securityKey_);
    extractInfo_->securityKey_ = NULL;
  }

  if (key == NULL)
    key = "";
  Lng32 len = str_len(key);

  extractInfo_->securityKey_ = (char *)
    h->allocateMemory(len + 1);
  str_cpy_all(extractInfo_->securityKey_, key, len + 1);
}

short ExMasterStmtGlobals::getExtractEspCpu(ULng32 index) const
{
  short result = -1;
  if (extractInfo_ && extractInfo_->esps_)
  {
    if (extractInfo_->esps_->used(index))
    {
      ExExtractEspInfo *esp = extractInfo_->esps_->at(index);
      if (esp)
        result = esp->cpu_;
    }
  }
  return result;
}

Lng32 ExMasterStmtGlobals::getExtractEspNodeNumber(ULng32 index) const
{
  Lng32 result = -1;
  if (extractInfo_ && extractInfo_->esps_)
  {
    if (extractInfo_->esps_->used(index))
    {
      ExExtractEspInfo *esp = extractInfo_->esps_->at(index);
      if (esp)
        result = esp->nodeNumber_;
    }
  }
  return result;
}

const char *
ExMasterStmtGlobals::getExtractEspPhandleText(ULng32 index) const
{
  const char *result = NULL;
  if (extractInfo_ && extractInfo_->esps_)
  {
    if (extractInfo_->esps_->used(index))
    {
      ExExtractEspInfo *esp = extractInfo_->esps_->at(index);
      if (esp)
        result = esp->phandleText_;
    }
  }
  return result;
}

const char *ExMasterStmtGlobals::getExtractSecurityKey() const
{
  const char *result = NULL;
  if (extractInfo_)
    result = extractInfo_->securityKey_;
  return result;
}

Int32 ExMasterStmtGlobals::getSMTraceLevel() const
{
  Int32 result = 0;
  if (smQueryID_ > 0)
    result = getContext()->getSessionDefaults()->getExSMTraceLevel();
  return result;
}

const char *ExMasterStmtGlobals::getSMTraceFilePrefix() const
{
  const char *result = NULL;
  if (smQueryID_ > 0)
    result = getContext()->getSessionDefaults()->getExSMTraceFilePrefix();
  return result;
}

// -----------------------------------------------------------------------
// Methods for class ExEspStmtGlobals
// -----------------------------------------------------------------------

ExEspStmtGlobals::ExEspStmtGlobals(short num_temps,
                                   CliGlobals *cliGlobals,
                                   short create_gui_sched,
                                   Space * space,
                                   CollHeap * heap,
                                   ExEspFragInstanceDir *espFragInstanceDir,
                                   ExFragInstanceHandle handle,
                                   ULng32 injectErrorAtExprFreq,
                                   char *queryId,
                                   Lng32 queryIdLen)
  : ExExeStmtGlobals(num_temps,
                     cliGlobals,
                     create_gui_sched,
                     space,
                     heap),
    sendTopTcbs_(heap),
    activatedSendTopTcbs_(&sendTopTcbs_, heap),
    espFragInstanceDir_(espFragInstanceDir),
    queryId_(queryId),
    queryIdLen_(queryIdLen),
    smDownloadInfo_(NULL)
{
  myHandle_               = handle;
  processIdsOfFragList_   = NULL;
  replyTag_               = GuaInvalidReplyTag;
  setInjectErrorAtExpr(injectErrorAtExprFreq);
  heap_  = (NAHeap *)heap;
  stmtStats_ = NULL;      // This is just a temporary initialization --
                          // see ExEspStmtGlobals::getStmtStats().
}

void ExEspStmtGlobals::deleteMe(NABoolean fatalError)
{
  StatsGlobals *statsGlobals;
  statsGlobals = espFragInstanceDir_->getStatsGlobals();
  if (statsGlobals != NULL)
  {
    int error = statsGlobals->getStatsSemaphore(espFragInstanceDir_->getSemId(),
                      espFragInstanceDir_->getPid());
    if (stmtStats_ != NULL)
      statsGlobals->removeQuery(espFragInstanceDir_->getPid(), stmtStats_);
    statsGlobals->releaseStatsSemaphore(espFragInstanceDir_->getSemId(),
            espFragInstanceDir_->getPid());
    stmtStats_ = NULL;

  }
  ExExeStmtGlobals::deleteMe(fatalError);  
}

ExEspStmtGlobals * ExEspStmtGlobals::castToExEspStmtGlobals()
{
  return this;
}

char * ExEspStmtGlobals::getFragmentPtr(ExFragId fragId) const
{
  ExFragInstanceHandle handle =
    espFragInstanceDir_->findHandle(getFragmentKey(fragId));

  return espFragInstanceDir_->getFragment(handle)->getFragment();
}

IpcMessageObjSize ExEspStmtGlobals::getFragmentLength(ExFragId fragId) const
{
  ExFragInstanceHandle handle =
    espFragInstanceDir_->findHandle(getFragmentKey(fragId));

  return espFragInstanceDir_->getFragment(handle)->getFragmentLength();
}

ExFragKey ExEspStmtGlobals::getFragmentKey(ExFragId fragId) const
{
  // get the fragment key for this fragment
  ExFragKey result = espFragInstanceDir_->findKey(myHandle_);

  // then change the fragment id to the one we are looking for
  result.setFragId(fragId);

  return result;
}

ExFragId ExEspStmtGlobals::getMyFragId() const
{
  return espFragInstanceDir_->findKey(myHandle_).getFragId();
}

Lng32 ExEspStmtGlobals::getNumOfInstances(ExFragId fragId) const
{
  return processIdsOfFragList_->getNumOfInstances(fragId);
}

const IpcProcessId & ExEspStmtGlobals::getInstanceProcessId(
     ExFragId fragId,
     Lng32 instanceNum) const
{
  return processIdsOfFragList_->getProcessId(fragId,instanceNum);
}

Lng32 ExEspStmtGlobals::getMyInstanceNumber() const
{
  Lng32 myInstanceNum  = -1;
  ExFragId myFragId   = getMyFragId();
  Lng32 numInstances   = getNumOfInstances(myFragId);

  // my own process id, expressed in the same domain as the
  // one we are comparing with
  IpcProcessId myProcId(getIpcEnvironment()->getMyOwnProcessId(
       getInstanceProcessId(myFragId,0).getDomain()));

  // I'm not the master, go through the list of process ids
  // for all instances and compare them with my own process id,
  // this will tell me which instance I'm supposed to be
  // NOTE: another option would be to send a message to each ESP
  // telling it which instance number it is, but this would mean
  // that the master would have to make individual messages instead
  // of being able to broadcast the same load/fixup message to all
  // ESPs.

  for (Int32 i = 0;
       i < numInstances AND myInstanceNum == -1;
       i++)
    {
      if (myProcId == getInstanceProcessId(myFragId,i))
	myInstanceNum = i;
    }
  
  // make sure we found it
  ex_assert(myInstanceNum != -1,
	    "couldn't determine my own instance number");

  return myInstanceNum;
}

void ExEspStmtGlobals::getMyNodeLocalInstanceNumber(
     Lng32 &myNodeLocalInstanceNumber,
     Lng32 &numOfLocalInstances) const
{
  ExFragId myFragId   = getMyFragId();
  Lng32 numInstances   = getNumOfInstances(myFragId);

  numOfLocalInstances = 0;
  myNodeLocalInstanceNumber = -1;

  // my own process id, expressed in the same domain as the
  // one we are comparing with
  IpcProcessId myProcId(getIpcEnvironment()->getMyOwnProcessId(
       getInstanceProcessId(myFragId,0).getDomain()));
  IpcNodeName myNodeName = myProcId.getNodeName();
  IpcCpuNum myCpuNum = myProcId.getCpuNum();

  // I'm not the master, go through the list of process ids
  // for all instances and compare them with my own process id,
  // this will tell me which instance I'm supposed to be
  // NOTE: another option would be to send a message to each ESP
  // telling it which instance number it is, but this would mean
  // that the master would have to make individual messages instead
  // of being able to broadcast the same load/fixup message to all
  // ESPs.

  for (Int32 i = 0; i < numInstances; i++)
    {
      const IpcProcessId &pid = getInstanceProcessId(myFragId,i);

      if (myProcId == pid)
	{
	  myNodeLocalInstanceNumber = numOfLocalInstances;
	  numOfLocalInstances++;
	}
      // In the case of NT, we want to get the total number of ESPs per
      // PE so we add the extra check  to check that the CPU number of the
      // process matched the current ESP s CPU number.
      // In the case of NSK, we can have several CPUs on a system. What we want
      // is the total number of ESPs on the node , so we ignore the check for
      //  CPU number.

      else if (pid.getNodeName() == myNodeName 
          AND pid.getCpuNum() == myCpuNum
           )
	{
	  numOfLocalInstances++;
	}
    }
  
  ex_assert(myNodeLocalInstanceNumber != -1,
	    "couldn't determine my own instance number");
}

Int64 ExEspStmtGlobals::getSMQueryID() const
{
  Int64 smQueryID = 0;
  if (smDownloadInfo_)
    smQueryID = smDownloadInfo_->getQueryID();
  return smQueryID;
}

Int32 ExEspStmtGlobals::getSMTraceLevel() const
{
  Int32 result = 0;
  if (smDownloadInfo_)
    result = smDownloadInfo_->getTraceLevel();
  return result;
}

const char *ExEspStmtGlobals::getSMTraceFilePrefix() const
{
  const char *result = NULL;
  if (smDownloadInfo_)
    result = smDownloadInfo_->getTraceFilePrefix();
  return result;
}

const ExScratchFileOptions *ExEspStmtGlobals::getScratchFileOptions() const
{
  if (resourceInfo_)
    return resourceInfo_->getScratchFileOptions();

  return NULL;
}

void ExEspStmtGlobals::setReplyTag(Int64 transid, short replyTag)
{
  // The transaction id itself is stored in the base table, the reply
  // tag through which we can switch to this transaction id is stored
  // in the derived object since it applied only to ESPs.
  //
  // And do this only if the ESP did NOT start it's own transaction.
  // transTag == -1 means the ESP did not start a transaction.
  if (getIpcEnvironment()->getControlConnection()->castToGuaReceiveControlConnection()->getBeginTransTag() == -1)
    {
      getTransid() = transid;
    }

  replyTag_    = replyTag;
}

NABoolean ExEspStmtGlobals::restoreTransaction()
{
  if (replyTag_ != GuaInvalidReplyTag)
    {
      // we do have a transaction work request, switch to its transaction
      // (the transid in the base class has already been set)
      getIpcEnvironment()->getControlConnection()->
	castToGuaReceiveControlConnection()->setUserTransReplyTag(replyTag_);

      // if this fails we'll catch the error when we try to use the transaction
      return TRUE;
    }
  else
    {
      // we don't have a transaction work request, this may be because
      // we don't need one or because we need to wait for one
      return NOT espFragInstanceDir_->
	getFragment(myHandle_)->getNeedsTransaction();
    }
}

CollIndex ExEspStmtGlobals::registerSendTopTcb(ex_send_top_tcb *st)
{
  CollIndex result = sendTopTcbs_.unusedIndex();
  sendTopTcbs_.insertAt(result, st);
  return result;
}

void ExEspStmtGlobals::setSendTopTcbLateCancelling()
{
  espFragInstanceDir_->startedLateCancelRequest(myHandle_);
}

void ExEspStmtGlobals::resetSendTopTcbLateCancelling()
{
  espFragInstanceDir_->finishedLateCancelRequest(myHandle_);
}

ex_send_top_tcb * ExEspStmtGlobals::getNextNonActivatedSendTop(CollIndex &i)
{
  // shortcut, if all send tops are marked, which should be very common
  if (sendTopTcbs_.entries() == activatedSendTopTcbs_.entries())
    return NULL;
  else
    {
      CollIndex l = sendTopTcbs_.getSize();
      ex_send_top_tcb *result = NULL;
      
      while (i < l && !result)
	{
	  // we check for send tops that are not marked
	  if (sendTopTcbs_.used(i) && ! activatedSendTopTcbs_.contains(i))
	    result = sendTopTcbs_[i];
	  else
	    i++;
	}
      return result;
    }
}

void ExEspStmtGlobals::setNoNewRequest(NABoolean n)
{ 
  // Okay to make requests now.  
  if (n == FALSE && noNewRequest() == TRUE)
  {
    // Special for transition from TRUE --> FALSE in ESPs.
    // Schedule all activated send tops in case they 
    // had to preempt their work methods when this flag 
    // was set to TRUE.
    
    CollIndex i;
    for (i=0; i<sendTopTcbs_.getSize(); i++)
      {
	// we check for send tops that are activated.
	if (sendTopTcbs_.used(i) && activatedSendTopTcbs_.contains(i))
          {
            ex_send_top_tcb *sendTop = sendTopTcbs_[i];
            sendTop->tickleSchedulerWork(TRUE);
            sendTop->tickleSchedulerCancel();
          }       
      }
  }
  ExExeStmtGlobals::setNoNewRequest(n); 
}

void ExEspStmtGlobals::decrementSendTopMsgesOut()
{
  // Let base class mutate its private member.
  ExExeStmtGlobals::decrementSendTopMsgesOut();

  if (anySendTopMsgesOut() == FALSE)
    { 
      // Let frag instance know that it need not wait for any
      // send top to transition from ACTIVE->RELEASING_WORK.
      espFragInstanceDir_->finishedRequest(myHandle_); 
    }
}

StmtStats *ExEspStmtGlobals::setStmtStats()
{
  StatsGlobals *statsGlobals = espFragInstanceDir_->getStatsGlobals();
  if (statsGlobals != NULL && queryId_ != NULL)
      stmtStats_ = statsGlobals->addQuery(espFragInstanceDir_->getPid(), 
                        queryId_, queryIdLen_, (void *)this, (Lng32)getMyFragId());
  return stmtStats_;
}


void ExExeStmtGlobals::makeMemoryCondition(Lng32 errCode)
{
  if (diagsArea_==NULL)
    diagsArea_ = ComDiagsArea::allocate(getDefaultHeap());
  
  ComCondition *cond = diagsArea_->makeNewCondition();
  cond->setSQLCODE(errCode);
  diagsArea_->acceptNewCondition();
}


ExExeStmtGlobals::StmtType ExExeStmtGlobals::getStmtType() { return stmtType_; }

void ExExeStmtGlobals::setStmtType(StmtType stmtType) { stmtType_ = stmtType; }

void ExExeStmtGlobals::incrementUdrTxMsgsOut()
{
  numUdrTxMsgsOut_++;
  //ex_assert(udrServer_,
  //          "UDR TX message counter used without an ExUdrServer pointer");
}

void ExExeStmtGlobals::decrementUdrTxMsgsOut()
{
  numUdrTxMsgsOut_--;
  ex_assert(numUdrTxMsgsOut_ >= 0,
            "The UDR TX message counter has dropped below zero");
}

void ExExeStmtGlobals::incrementUdrNonTxMsgsOut()
{
  numUdrNonTxMsgsOut_++;
  //ex_assert(udrServer_,
  //          "UDR Non-TX message counter used without an ExUdrServer pointer");
}

void ExExeStmtGlobals::decrementUdrNonTxMsgsOut()
{
  numUdrNonTxMsgsOut_--;
  ex_assert(numUdrNonTxMsgsOut_ >= 0,
            "The UDR Non-TX message counter has dropped below zero");
}

// the caller of this method should check if there are any errors
// encountered during initializing SeaMonster, by looking for errors in
// ExExeStmtGlobals which is side effected by ExSMGlobals::InitSMGlobals
void ExExeStmtGlobals::initSMGlobals()
{
  // Do nothing if this query does not use SeaMonster
  Int64 smQueryID = getSMQueryID();
  if (smQueryID <= 0)
    return;

  // Initialize SM if there are no errors already
  ComDiagsArea *diags = getDiagsArea();
  if (diags == NULL || diags->getNumber(DgSqlCode::ERROR_) == 0)
  {
    ExSMGlobals *smGlobals = ExSMGlobals::InitSMGlobals(this);
  }

  // Return if there are errors in the diags area
  diags = getDiagsArea();
  if (diags && diags->getNumber(DgSqlCode::ERROR_) > 0)
    return;

  // Register the SeaMonster query ID. The ID will be un-registered
  // when this statement globals instance is cleaned up by calling the
  // deleteMe() method.
  int32_t rc = ExSM_Register(smQueryID);
  
  if (rc != 0)
    ExSMGlobals::addDiags("ExSM_Register", rc, this);
  else
    smQueryIDRegistered_ = true;
}

SequenceValueGenerator * ExExeStmtGlobals::seqGen()
{
  if (! getContext())
    return NULL;

  return getContext()->seqGen();
}

SequenceValueGenerator * ex_globals::seqGen()
{
  if (! castToExExeStmtGlobals())
    return NULL;

  return castToExExeStmtGlobals()->seqGen();
}

