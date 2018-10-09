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
 * File:         ex_root.cpp
 * Description:  Root TCB for master executor. Interface with CLI.
 *               The root node also drives the scheduler and the
 *               fragment directory. It has execute() and fetch() methods.
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include  "Platform.h"
#include <stdio.h>


#include  "cli_stdh.h"
#include  "ex_stdh.h"
#include  "ex_exe_stmt_globals.h"
#include  "ComTdb.h"
#include  "ex_tcb.h"
#include  "ex_root.h"
#include  "ex_expr.h"
#include  "ex_frag_rt.h"
#include  "ComDiags.h"
#include  "ExStats.h"
#include  "NAError.h"
#include  "LateBindInfo.h"
#include  "ExecuteIdTrig.h"
#include  "TriggerEnable.h"
#include  "ComRtUtils.h"
#include  "PortProcessCalls.h"
#include  "ExpLOB.h"

#include "ex_transaction.h"
#include "ComSqlId.h"
#include "ExCextdecs.h"

#include "ExSMTrace.h"
#include "ExSMGlobals.h"
#include "ExSMCommon.h"
#include "ExpHbaseInterface.h"

////////////////////////////////////////////////////////////////////////
//  TDB procedures

//
// Build a root tcb
//
ex_tcb * ex_root_tdb::build(CliGlobals *cliGlobals, ex_globals * glob)
{
  ExExeStmtGlobals * exe_glob = glob->castToExExeStmtGlobals();
  ExMasterStmtGlobals *master_glob = exe_glob->castToExMasterStmtGlobals();

  if (getQueryUsesSM() && cliGlobals->getEnvironment()->smEnabled())
  {
    // For SeaMonster if reader thread terminated, since there is no
    // reader thread we cannot continue with the execution of the
    // query. So raise the error as to why the reader thread
    // terminated and add the error to statement globals.
    ExSMGlobals *smGlobals = ExSMGlobals::GetExSMGlobals();
    if (smGlobals && smGlobals->getReaderThreadState() ==
        ExSMGlobals::TERMINATED_DUE_TO_ERROR)
    {
      smGlobals->addReaderThreadError(exe_glob);
      return NULL;
    }
  }

  // set this plan version in the statement globals.
  glob->setPlanVersion(planVersion_);

  // set the fragment directory in glob. This will be passed
  // to the build of all tdb's and used by them, if needed.
  master_glob->setFragDir(fragDir_);
  glob->setNumOfTemps(tempTableCount_);

  // ---------------------------------------------------------------------
  // build the run time fragment instance table
  // ---------------------------------------------------------------------
  ExRtFragTable *rtFragTable = NULL;

  // make a run time fragment directory identical to compiled one
  rtFragTable = new(master_glob->getSpace())
    ExRtFragTable(master_glob, fragDir_, (char *) this);

  // if this tdb is displayed in the GUI then enable GUI display for ESPs
  if (displayExecution() > 0)
    rtFragTable->displayGuiForESPs(TRUE);

  // remember the fragment table in the globals
  master_glob->setRtFragTable(rtFragTable);

  // Acquire ESPs (do this before child is built, as some decisions in
  // children depend on the location of fragment instances)

  // If measure stats are being collected, find out the number of new
  // esps started and the time it took to start them.
  UInt32 numOfNewEspsStarted = 0; // num of new esps started by this operator.
  UInt32 numOfTotalEspsUsed = 0;  // num of total esps used by this operator.
  Int64 newprocessTime = 0;
  if ((getCollectStatsType() == ComTdb::ACCUMULATED_STATS) ||
      (getCollectStatsType() == ComTdb::OPERATOR_STATS) ||
      (getCollectStatsType() == ComTdb::PERTABLE_STATS))
    {
      newprocessTime = JULIANTIMESTAMP(OMIT,OMIT,OMIT,OMIT);
 
    }

  rtFragTable->assignEsps(TRUE, numOfTotalEspsUsed, numOfNewEspsStarted 
                          );

  if (((getCollectStatsType() == ComTdb::ACCUMULATED_STATS) ||
       (getCollectStatsType() == ComTdb::OPERATOR_STATS) ||
       (getCollectStatsType() == ComTdb::PERTABLE_STATS)) &&
      (numOfNewEspsStarted > 0))
    {
      newprocessTime = JULIANTIMESTAMP(OMIT,OMIT,OMIT,OMIT) - newprocessTime;
    }
  else
    newprocessTime = 0;

  // return if we couldn't allocate ESPs
  if (rtFragTable->getState() == ExRtFragTable::ERROR)
  {
    return NULL;
  }

  if (getQueryUsesSM() && cliGlobals->getEnvironment()->smEnabled())
  {
    // Assign a SeaMonster ID to the query
    Int64 smQueryID = ExSMGlobals::reserveSMID();
    master_glob->setSMQueryID(smQueryID);
    
    // Initialize SeaMonster. Return early if errors where encountered.
    exe_glob->initSMGlobals();
    ComDiagsArea *diags = exe_glob->getDiagsArea();
    if (diags && diags->getNumber(DgSqlCode::ERROR_) > 0)
      return NULL;
  }
  else
  {
    // Turn off tracing if it once was enabled and now we turned off
    // the SEAMONSTER CQD, which disables the SeaMonster trace.
    ExSMGlobals *smGlobals = ExSMGlobals::GetExSMGlobals();
    if (smGlobals && smGlobals->getTraceEnabled())
      smGlobals->setTraceEnabled(false);
  }

  // build the child
  ex_tcb * child_tcb = childTdb->build(exe_glob);
  
  ex_root_tcb * root_tcb = 
    new(exe_glob->getSpace()) ex_root_tcb(*this,
					     *child_tcb,
					     exe_glob);
  root_tcb->registerSubtasks();

  // if we collect stats set up the stats area. 
  ExStatisticsArea* statsArea = NULL;
  if (getCollectStats())
    {
      StmtStats *stmtStats = master_glob->getStatement()->getStmtStats();
      if (getCollectStatsType() == ALL_STATS || 
          stmtStats == NULL || stmtStats->getQueryId() == NULL)
        {
          // These ALL stats are too numerous to store in the 
          // shared segment. 
          statsArea = new(master_glob->getDefaultHeap())
            ExStatisticsArea(master_glob->getDefaultHeap(), 0, 
                             getCollectStatsType());
          master_glob->setStatsArea(statsArea);
          root_tcb->allocateStatsEntry();
        }
      else
        { 
          StatsGlobals *statsGlobals = cliGlobals->getStatsGlobals();
          if (statsGlobals == NULL)
            {
              statsArea = new(master_glob->getDefaultHeap())
                      ExStatisticsArea(master_glob->getDefaultHeap(), 0, 
                      getCollectStatsType());
              master_glob->setStatsArea(statsArea);
              root_tcb->allocateStatsEntry();
	      if (stmtStats != NULL)
		stmtStats->setStatsArea(statsArea);
            }
          else
            {
              int error = 
                statsGlobals->getStatsSemaphore(cliGlobals->getSemId(),
                  cliGlobals->myPin());
              statsArea = new(cliGlobals->getStatsHeap())
                              ExStatisticsArea(cliGlobals->getStatsHeap(), 0, 
                              getCollectStatsType());
              if (stmtStats != NULL)
              {
                // If the Context Stats is pointing to this statement statsArea
                // make the Context stats as a copy so that it gets deleted when 
                // ContextCli::setStatsArea called again. StmtStats::setStatsArea will not
                // delete this statistics area since context has a reference to it.
                ExStatisticsArea *ctxStats = 
                    master_glob->getStatement()->getContext()->getStats();
                ExStatisticsArea *prevStats = stmtStats->getStatsArea();
                if (ctxStats == prevStats)
                   cliGlobals->currContext()->setStatsArea(NULL,FALSE, FALSE,FALSE);
                stmtStats->setStatsArea(statsArea);
              }
              master_glob->setStatsArea(statsArea);
              statsArea->setRtsStatsCollectEnabled(getCollectRtsStats());
              root_tcb->allocateStatsEntry();
              statsGlobals->releaseStatsSemaphore(cliGlobals->getSemId(), 
                                 cliGlobals->myPin());
            }
        }
      statsArea->setExplainPlanId(explainPlanId_);

      ExMasterStats *masterStats = (stmtStats ? stmtStats->getMasterStats() :
                                                NULL);
      if (masterStats)
        {
          masterStats->numOfNewEspsStarted() = numOfNewEspsStarted;
          masterStats->setNumCpus(rtFragTable->
                                  countSQLNodes(cliGlobals->myCpu()));
        }
    }


  // set the EMS Experience level
  if (isEMSEventExperienceLevelBeginner())
  {
     cliGlobals->setEMSBeginnerExperienceLevel();
  }
  if (getUncProcess())
    cliGlobals->setUncProcess();

  // nobody can claim me as a dependent, so I'm setting my parent TDB id
  // to NULL.
  if (root_tcb->getStatsEntry())
    {
      root_tcb->getStatsEntry()->setParentTdbId(-1);

      if (root_tcb->getStatsEntry()->castToExFragRootOperStats())
	root_tcb->getStatsEntry()->castToExFragRootOperStats()->
	  setStmtIndex(master_glob->getStatement()->getStatementIndex());
    }

  if (root_tcb->getStatsEntry()) 
  {
    if (root_tcb->getStatsEntry()->castToExMeasStats())
    {
      root_tcb->getStatsEntry()->castToExMeasStats()->incNewprocess(numOfNewEspsStarted);
      root_tcb->getStatsEntry()->castToExMeasStats()->incNewprocessTime(newprocessTime);
    }
    else
    if (root_tcb->getStatsEntry()->castToExFragRootOperStats())
    {
      root_tcb->getStatsEntry()->castToExFragRootOperStats()->incNewprocess(numOfNewEspsStarted);
      root_tcb->getStatsEntry()->castToExFragRootOperStats()->incNewprocessTime(newprocessTime);
    }
  }
  // download plans and build them in the ESPs
  if (NOT noEspsFixup())
    rtFragTable->downloadAndFixup();

  // assign initial partition ranges to ESPs
  rtFragTable->assignPartRangesAndTA(TRUE);

  if (cpuLimit_ > 0)
  {
    glob->getScheduler()->setMaxCpuTime(cpuLimit_);
    glob->getScheduler()->setCpuLimitCheckFreq(cpuLimitCheckFreq_);
  }

  if (processLOB())
    {
      glob->initLOBglobal(cliGlobals->currContext(), useLibHdfs());
    }

  return (root_tcb);

} // ex_root_tdb::build



///////////////////////////////////////////////////////////////////////////
//
//  TCB procedures
///////////////////////////////////////////////////////////////////////////
ex_root_tcb::ex_root_tcb(
     const ex_root_tdb & root_tdb,
     const ex_tcb & child_tcb,
     ExExeStmtGlobals *glob) :
      ex_tcb( root_tdb, 1, glob),
      workAtp_(0),
      asyncCancelSubtask_(NULL),
      cancelStarted_(FALSE),
      fatalError_(FALSE),
      cpuLimitExceeded_(FALSE),
      pkeyAtp_(NULL),
      lastQueueSize_(0),  // used by rowsets
      triggerStatusVector_(NULL),
     queryStartedStream_(NULL),
     queryFinishedStream_(NULL),
     cbServer_(NULL),
     cbCommStatus_(0),
      mayPinAudit_(false),
      mayLock_(false)
{
  tcbChild_ = &child_tcb;

  // get the queues that my child uses to communicate with me
  qchild  = child_tcb.getParentQueue();

  // unlike most other nodes, the root allocates ATPs for its child
  // down queue, since it creates the down queue entries rather
  // than passing them
  qchild.down->allocateAtps(glob->getSpace());

  if (root_tdb.inputExpr_)
    {
      (root_tdb.inputExpr_)->fixup(0, getExpressionMode(), this,
				   glob->getSpace(), glob->getDefaultHeap(), FALSE, glob);
    }

  if (root_tdb.outputExpr_)
    {
      (root_tdb.outputExpr_)->fixup(0, getExpressionMode(), this,
				    glob->getSpace(), glob->getDefaultHeap(),
				    FALSE, glob);
    }

  if (root_tdb.pkeyExpr_)
    {
      (root_tdb.pkeyExpr_)->fixup(0, getExpressionMode(), this, 
				  glob->getSpace(), glob->getDefaultHeap(), FALSE, glob);
    }

 if (root_tdb.predExpr_)
    {
      (root_tdb.predExpr_)->fixup(0, getExpressionMode(), this, 
				  glob->getSpace(), glob->getDefaultHeap(), FALSE, glob);
    }

  workAtp_ = allocateAtp(root_tdb.criDesc_, glob->getSpace());

  Int32 numTuples = 2; // temps and consts
  
  if (root_tdb.inputVarsSize_ > 0)
    {
      // allocate space to hold input params/hostvars
      tupp_descriptor *tp = new(glob->getSpace()) tupp_descriptor;
      char * dataPtr =
	(char *)glob->getSpace()->allocateMemory(root_tdb.inputVarsSize_);
      tp->init(root_tdb.inputVarsSize_,0,dataPtr);
      workAtp_->getTupp(numTuples++) = tp;
   }

  if (root_tdb.updateCurrentOfQuery())
    {
      // allocate space to hold input pkey row
      tupp_descriptor *tp = new(glob->getSpace()) tupp_descriptor;
      char * dataPtr =
	(char *)glob->getSpace()->allocateMemory(root_tdb.pkeyLen_);
      tp->init( root_tdb.pkeyLen_,0,dataPtr);
      workAtp_->getTupp(numTuples++) = tp;
    }
  else if (pkeyExpr())
    {
      pkeyAtp_ = allocateAtp(root_tdb.workCriDesc_, glob->getSpace());

      // allocate space to hold the row of primary keys
      tupp_descriptor *tp = new(glob->getSpace()) tupp_descriptor;
      char * dataPtr =
	(char *)glob->getSpace()->allocateMemory(root_tdb.pkeyLen_);
      tp->init( root_tdb.pkeyLen_,0,dataPtr);
      pkeyAtp_->getTupp(2) = tp;
    }
  
  // set the stream timeout value to be used by this TCB
  if ( ! glob->getStreamTimeout( streamTimeout_ ) ) // dynamic not found ?
    streamTimeout_ = root_tdb.streamTimeout_ ; // then use the static value
 

  // Used by Query Caching.
  if (root_tdb.cacheVarsSize_ > 0) 
  {
    tupp_descriptor *tp = new(glob->getSpace()) tupp_descriptor;
    tp->init(root_tdb.cacheVarsSize_,0, root_tdb.getParameterBuffer());
    workAtp_->getTupp(numTuples++) = tp;
  }
  ex_assert(numTuples==root_tdb.criDesc_->noTuples(), 
            "conflicting tuple numbers in root_tdb");

  getGlobals()->getScheduler()->setRootTcb(this);

#ifdef NA_DEBUG_GUI
  //-------------------------------------------------------------
  // if the user has requested use of the executor GUI 
  // display then load the dll and set it up
  //-----------------------------------------------------------
  if (root_tdb.displayExecution() == 1)
    getGlobals()->getScheduler()->startGui();
#endif

  rwrsBuffer_ = NULL;
  if (root_tdb.getRWRSInfo())
    {
      rwrsBuffer_ = 
	new(glob->getDefaultHeap()) char[
	     root_tdb.getRWRSInfo()->rwrsMaxSize() *
	     root_tdb.getRWRSInfo()->rwrsMaxInternalRowlen()];
      root_tdb.getRWRSInfo()->setRWRSInternalBufferAddr(rwrsBuffer_);
    }

}

ex_root_tcb::~ex_root_tcb()
{
  freeResources();
};

void ex_root_tcb::freeResources()
{
  if (workAtp_)
    {
    workAtp_->release();
    deallocateAtp(workAtp_, getGlobals()->getSpace());
    workAtp_ = NULL;
    }

  ExMasterStmtGlobals *glob =
    getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals();

  ExRtFragTable *rtFragTable = glob->getRtFragTable();

  if (rtFragTable) 
    {
    delete rtFragTable;
    glob->setRtFragTable(NULL);
    }

  if (root_tdb().logRetriedIpcErrors())
  {
    // The IpcEnvironment object is global to the process, and shared
    // by all of its statements.  Although it is possible these errors 
    // (if any) were accumulated on some other concurrent Statement, 
    // we will log and clear the counter now.  As a result, we will not 
    // attempt to correlate retried errors with any particular Statement.
    glob->getIpcEnvironment()->logRetriedMessages();
  }
  if (queryStartedStream_)
  {
    queryStartedStream_->removeRootTcb();
    queryStartedStream_->addToCompletedList();
    queryStartedStream_ = NULL;
  }

  if (queryFinishedStream_)
  {
    queryFinishedStream_->removeRootTcb();
    queryFinishedStream_->addToCompletedList();
    queryFinishedStream_ = NULL;
  }

};

void ex_root_tcb::populateCancelDiags(ComDiagsArea &diags)
{

  diags.clearErrorConditionsOnly();
  diags << DgSqlCode(-EXE_CANCELED); 

  ExMasterStmtGlobals *master_glob = getGlobals()->
	            castToExExeStmtGlobals()->castToExMasterStmtGlobals(); 

  ExStatisticsArea *statsArea = master_glob->getStatsArea();
  CliGlobals *cliGlobals = master_glob->getCliGlobals();
  if (statsArea)
  {
    ExMasterStats *masterStats = statsArea->getMasterStats();
    if (masterStats && masterStats->getCancelComment())
      diags << DgString0(masterStats->getCancelComment());
  }
}

void ex_root_tcb::registerSubtasks()
{
  ExScheduler *sched = getGlobals()->getScheduler();

  // there is only one queue pair to the child, no parent queue, and
  // the work procedure does actually no work except interrupting
  // the work procedure immediately when a row can be returned
  sched->registerUnblockSubtask(sWork,this,qchild.down,"WK");
  sched->registerInsertSubtask(sWork,this,qchild.up);
  asyncCancelSubtask_ = sched->registerNonQueueSubtask(sWork,this);
   
  // the frag table has its own event and work procedure, but its
  // work procedure is called through a method of the root TCB
  getGlobals()->
    castToExExeStmtGlobals()->
    castToExMasterStmtGlobals()->getRtFragTable()->setSchedulerEvent(
       getGlobals()->getScheduler()->registerNonQueueSubtask(sWorkOnFragDir,
							  this,"FD"));
}

char * ex_root_tcb::getPkeyRow()
{
  if ((pkeyExpr()) &&
      (pkeyAtp_))
    return pkeyAtp_->getTupp(2).getDataPointer();
  else
    { 
      // either cursor is not updatable or the pkeyExpr was
      // not setup correctly.
      return NULL; // error case.
    }
}

NABoolean ex_root_tcb::needStatsEntry()
{
  return TRUE;
}

ExOperStats * ex_root_tcb::doAllocateStatsEntry(CollHeap *heap, ComTdb *tdb)
{
  ExOperStats * stat = NULL;
  ComTdb::CollectStatsType statsType = getGlobals()->getStatsArea()->getCollectStatsType();
  if ((statsType == ComTdb::ALL_STATS) ||
      (statsType == ComTdb::OPERATOR_STATS) ||
      (statsType == ComTdb::PERTABLE_STATS))
  {
    stat = new(heap) ExFragRootOperStats(heap,
					  this,
					  tdb);
    StmtStats *ss = getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->getStatement()->getStmtStats();
    if (ss != NULL)
      ((ExFragRootOperStats *)stat)->setQueryId(ss->getQueryId(), ss->getQueryIdLen());
  }
  else if (statsType == ComTdb::ACCUMULATED_STATS)
  {
    // if accumulated statistics are to be collected, allocate
    // one stats entry and insert it into the queue.
    // All executor operators that collect stats will use this
    // entry to update stats.
    // These stats are not associated with any particular
    // TDB or TCB. We pass tdb as argument just to propagate
    // overflow_mode settings to stats.
    stat = new(heap) ExMeasStats(heap, 
				  NULL,
				  tdb); 
    StmtStats *ss = getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->getStatement()->getStmtStats();
    if (ss != NULL)
      ((ExMeasStats *)stat)->setQueryId(ss->getQueryId(), ss->getQueryIdLen());
  }
  if (stat)
    getGlobals()->getStatsArea()->setRootStats(stat);
  return stat;
}

// this method receives the row of primary key values for an update
// where current of query. The row is returned from the select cursor
// thru the getPkeyRow() method.
void ex_root_tcb::inputPkeyRow(char * pkey_row)
{
  char * dataPtr =
    workAtp_->getTupp(root_tdb().criDesc_->noTuples()-1).getDataPointer();
  str_cpy_all(dataPtr, pkey_row, root_tdb().pkeyLen_);
}

///////////////////////////////////////////////////////////////////////////
// this method returns the inputRow. If no input values (hostvars, param,
// etc), it returns NULL. Used for reompilation purpose. At recomp time,
// the input descriptor is not available to the fetch methods. So before
// recompilation,
// the 'original' inputRow is retrieved and then passed back again
// after recompilation. 
// Caller should copy the inputdata into its own space.
///////////////////////////////////////////////////////////////////////////
void ex_root_tcb::getInputData(char* &inputData, ULng32 &inputDatalen)
{
  inputData = NULL;
  inputDatalen = 0;
  if (root_tdb().inputVarsSize_ > 0)
    {
      inputData = workAtp_->getTupp(2).getDataPointer();
      inputDatalen = root_tdb().inputVarsSize_;
    }
}

void ex_root_tcb::setInputData(char* inputData)
{
  if (root_tdb().inputVarsSize_ > 0)
    {
      str_cpy_all(workAtp_->getTupp(2).getDataPointer(), inputData,
		  root_tdb().inputVarsSize_);
    }
}

///////////////////////////////////
// fixup.
//////////////////////////////////
Int32 ex_root_tcb::fixup()
{
  return ex_tcb::fixup();
}

///////////////////////////
// execute.
///////////////////////////
Int32 ex_root_tcb::execute(CliGlobals *cliGlobals,
			 ExExeStmtGlobals * glob, 
                         Descriptor * input_desc,
			 ComDiagsArea*& diagsArea,
			 NABoolean reExecute)
{
  Int32 jmpRc = 0;


  ExMasterStmtGlobals *master_glob = glob->castToExMasterStmtGlobals();

  if (fatalError_)
    {
      // Can't fetch. I'm dead.
      diagsArea = ComDiagsArea::allocate(getHeap());
      *diagsArea << DgSqlCode(-EXE_CANNOT_CONTINUE);
      return -EXE_CANNOT_CONTINUE;
    }

#ifdef _DEBUG
// Do not need to cover debug code since customers and QA do no receive it.
  char *testCancelFreq  = getenv("TEST_ERROR_AT_QUEUE");
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
      getGlobals()->setInjectErrorAtQueue(freq);
    }
  else
      getGlobals()->setInjectErrorAtQueue(0);
#endif
  
  ex_queue_entry *entry = qchild.down->getTailEntry();

  // Initialize rownum.
  getGlobals()->rowNum() = 1;

  if (root_tdb().isEmbeddedUpdateOrDelete())
    {
      // This is a destructive select, we need the GET_NEXT_N protocol.
      // The program must be using a cursor for "delete access" that probably
      // will remain open across transactions.  In order to "bunch"
      // destructively read rows, we need the GET_NEXT_N protocol where N is
      // the number of rows the program intents to fetch (destructively) in
      // a transaction.
      entry->downState.request = (root_tdb().isStreamScan() ? ex_queue::GET_NEXT_N_MAYBE_WAIT : ex_queue::GET_NEXT_N);

      // nothing to do yet (fetch will start this)
      entry->downState.requestValue = 0;
      // this is the first GET_NEXT_N request
      entry->downState.numGetNextsIssued = 0;
    }
  else
  if (root_tdb().getFirstNRows() < 0)
    {
      entry->downState.request = ex_queue::GET_ALL;
      entry->downState.requestValue = 11;
    }
  else
    {
      // Coverage note - this is probably dead code.
      entry->downState.request = ex_queue::GET_N;
      entry->downState.requestValue = (Lng32)root_tdb().getFirstNRows();
    }

  entry->downState.parentIndex = 20;

  entry->copyAtp(workAtp_);

  // In case we are dealing with compound statements
  if (input_desc) {
    input_desc->setCompoundStmtsInfo(((ComTdbRoot *) &tdb)->getCompoundStmtsInfo());
  }
  // 
  //  This entry may not have a diags area yet.
  //  If diags area is not available, allocate 
  //  one for the entry.
  //
  if (root_tdb().isNonFatalErrorTolerated()) 
  {
    if (!(entry->getDiagsArea())) 
    {
      ComDiagsArea * da = ComDiagsArea::allocate(getHeap());
      entry->setDiagsArea(da);
    }
    entry->getDiagsArea()->setLengthLimit(root_tdb().getNotAtomicFailureLimit());
  }
  // do not evaulate the inputExpr, if this execute is being done
  // after automatic recompilation. The original input tupp will
  // be used at this time.
  if ((! reExecute) && (inputExpr()))
    {        
      UInt32 flags = 0;
      inputExpr()->setIsODBC(flags, root_tdb().odbcQuery());
      inputExpr()->setIsRWRS(flags, root_tdb().rowwiseRowsetInput());
      inputExpr()->setInternalFormatIO
	(flags,
	 master_glob->getStatement()->getContext()->getSessionDefaults()->
	 getInternalFormatIO());
      inputExpr()->setIsDBTR
	(flags,
	 master_glob->getStatement()->getContext()->getSessionDefaults()->
	 getDbtrProcess());
					     
      // move the values from user area to internal area
      ex_expr::exp_return_type rc;
      if ((input_desc) &&
	  (root_tdb().singleRowInput()) &&
	  (NOT input_desc->isDescTypeWide()) &&
	  (NOT input_desc->thereIsACompoundStatement()))
	{
	  // fast single row input
	  rc = inputExpr()->inputSingleRowValue
	    (entry->getAtp(),
	     input_desc,
	     NULL,
	     glob->getDefaultHeap(),
	     flags);
	}
      else if (root_tdb().rowwiseRowsetInput())
	{
	  // rowwise rowset input
	  rc = inputExpr()->inputRowwiseRowsetValues
	    (entry->getAtp(),
	     input_desc,
	     root_tdb().getRWRSInfo(),
	     root_tdb().isNonFatalErrorTolerated(),
	     glob->getDefaultHeap(),
	     flags);
	}
      else
	{
	  // columnwise rowset or fastsingle row input disabled.
	  rc = inputExpr()->inputValues(entry->getAtp(),
					input_desc,
					root_tdb().isNonFatalErrorTolerated(),
					glob->getDefaultHeap(),
					flags);
	}
      if (rc == ex_expr::EXPR_ERROR) 
	{
	  diagsArea = ex_root_tcb::moveDiagsAreaFromEntry(entry);
	  if (root_tdb().isNonFatalErrorTolerated() && !(diagsArea->canAcceptMoreErrors())) 
	    {
	      diagsArea->removeLastErrorCondition();
	      *diagsArea << DgSqlCode(-EXE_NONATOMIC_FAILURE_LIMIT_EXCEEDED) 
			 << DgInt0(root_tdb().getNotAtomicFailureLimit());
	    }
	  
	  return -1;
	};
      
      // and evaluate any expression on those input values
      if ((NOT inputExpr()->noEval()) &&
	  (inputExpr()->eval(entry->getAtp(), 0) == ex_expr::EXPR_ERROR))
        {
          diagsArea = ex_root_tcb::moveDiagsAreaFromEntry(entry);
          return -1;
        }
    }
  else 
    if (!reExecute && !inputExpr() && input_desc &&
	input_desc->getUsedEntryCount() &&
	glob->getStmtType() != ExExeStmtGlobals::STATIC) {
      NABoolean byPassCheck = input_desc->isDescTypeWide();
      if (!byPassCheck && (input_desc->getCompoundStmtsInfo() == 0) 
	  && (diagsArea == NULL)) {
	diagsArea = ComDiagsArea::allocate(getHeap());
	*diagsArea << DgSqlCode(-CLI_STMT_DESC_COUNT_MISMATCH);
	return -1; 
      }
    }
  
  ExFragRootOperStats *rootStats = NULL;
  if (getStatsEntry())
  {
    rootStats = getStatsEntry()->castToExFragRootOperStats();
    if (rootStats)
      rootStats->setTimestamp(NA_JulianTimestamp());
  }

  ExMasterStats *mStats = NULL;
  StmtStats *sStats = master_glob->getStatement()->getStmtStats();
  if(sStats)
    mStats = sStats->getMasterStats();

  if (checkTransBeforeExecute(
         master_glob->getStatement()->getContext()->getTransaction(), 
	 master_glob,
	 mStats, diagsArea)) 
    return -1;
            

  // Set cancelState to indicate ex_root_tcb is valid 
  // for the cancel thread to reference.
  // Also check if there is an old cancel request.
  // This code block must be placed right before insert().
  // ------------------------------------------------------------
  CancelState old = master_glob->setCancelState(CLI_CANCEL_TCB_READY);
  if (old == CLI_CANCEL_REQUESTED)
    {
      // Don't bother to continue if async cancel has been requested.
      master_glob->clearCancelState();
      if (diagsArea == NULL)
	 diagsArea = ComDiagsArea::allocate(getHeap());
      else
         diagsArea->clearErrorConditionsOnly();
      populateCancelDiags(*diagsArea);
      return -1; 
    }

	// ++Trigger,
	//
	// UniqueExecueId should be able to distinguish between every two query trees 
	// that are executed in a cluster simultaneously.  
	// UniqueExecueId is an evaluate once function in the compiler, but its value   
	// is calculated once here, and propagate like external parameter to every 
	// node in the query tree. 
	// The unique execute id is a combination of CPU number in the cluster, 
    // process id and root tcb address.
	//


    if (root_tdb().getUniqueExecuteIdOffset() >= 0
		// noTuples() > 2 is checked temporarily, entered to prevent 
		// root_tdb version related problems during INIT_SQL 
		&& entry->getAtp()->getCriDesc()->noTuples() > 2)
	{
	  
	  ExecuteId *uniqueExecuteId = (ExecuteId *)
		  (entry->getAtp()->getTupp(2).getDataPointer() + root_tdb().getUniqueExecuteIdOffset());

	  uniqueExecuteId->cpuNum = glob->castToExExeStmtGlobals()->getIpcEnvironment()->getMyOwnProcessId(IPC_DOM_GUA_PHANDLE).getCpuNum();
	  uniqueExecuteId->pid = getpid();
	  uniqueExecuteId->rootTcbAddress = this;

	}

	if ((root_tdb().getTriggersCount() > 0) 
		// noTuples() > 2 is checked to prevent 
		// root_tdb version related problems during INIT_SQL 
		&& (entry->getAtp()->getCriDesc()->noTuples() > 2)
                && (getTriggerStatusVector() != NULL))
                // In case one or more triggers are dropped during execution and 
                // precisely after fixup and immediately before populating the root
                // tcb trigger status vector array (before calling getTriggersStatus 
                // in Statement::execute) a blown away open error will be raised
                // getTriggerStatusVector() returns NULL when there are no triggers
                // in this statement
	{
	  unsigned char *triggersStatus = 
		  (unsigned char *)(entry->getAtp()->getTupp(2).getDataPointer() + 
		  root_tdb().getTriggersStatusOffset());
	  // copy the TriggerStatusVector to its atp
	  memcpy(triggersStatus, getTriggerStatusVector(), TRIGGERS_STATUS_VECTOR_SIZE);
	}
	// --Trigger,

  qchild.down->insert();

  // indicate to fragment table that we have a new request
  // (ignore return code for now, since we never process multiple requests)
  getGlobals()->
    castToExExeStmtGlobals()->
    castToExMasterStmtGlobals()->getRtFragTable()->setActiveState();

  // Let cancel broker know about this query,
  registerCB(diagsArea);

  cpuLimitExceeded_ = FALSE;

  master_glob->incExecutionCount();

  if (root_tdb().getQueryUsesSM() && cliGlobals->getEnvironment()->smEnabled())
  {
    EXSM_TRACE(EXSM_TRACE_MAIN_THR, "query [%s]", 
               master_glob->getStatement()->getSrcStr());
    
    EXSM_TRACE(EXSM_TRACE_MAIN_THR, "root tcb %p exec count %d", this,
               (int) master_glob->getExecutionCount());
  }

  // do some work, but not necessarily all of it
  if (glob->getScheduler()->work() == WORK_BAD_ERROR)
    {
      completeOutstandingCancelMsgs();
      return fatal_error(glob, diagsArea);
    }

  // Following code is test for soln 10-081104-7061.  A CQD
  // COMP_INT_38 can be used to force various kinds of abends
  // in the master.
#if 0
  // re-enable this to test abend creation.
  switch (root_tdb().getAbendType())
  {
    case ComTdbRoot::NO_ABEND:
      {
        break;
      }
    case ComTdbRoot::SIGNED_OVERFLOW:
      {
        // Don't expect this to work. b/c this code is compiled
        // with overlow trapping set to off.
        signed short willOverflow = 0x7ffc;
        ULng32 loopCnt = (ULng32) this;
        while (loopCnt--)
          willOverflow++;
        break;
      }
    case ComTdbRoot::ASSERT:
      {
        ex_assert( root_tdb().getAbendType() != 
                    ComTdbRoot::ASSERT, 
                  "Abend/assertTest passed.");
        break;

      }
    case ComTdbRoot::INVALID_MEMORY:
      {
        return *(Int32 *) WORK_OK;
      }
    default:
      {
        ex_assert( 0, "invalid value for getAbendType (COMP_INT_38).");
        break;
      }
  }
#endif
  Int32 retcode = 0;
// The lines below were added to return proper value
// for retcode at the time of execute. But it seems to
// open up more issues. So commented out for now
/*
  if (qchild.up->isEmpty())
     return 0;  
  ex_queue_entry *centry = qchild.up->getHeadEntry();
  if (centry == NULL)
     return 0;
  if (centry->getDiagsArea()) {
     if (diagsArea == NULL)     
        diagsArea = ex_root_tcb::moveDiagsAreaFromEntry (centry);
     else
        diagsArea->mergeAfter(*centry->getDiagsArea());
  }
  // Copied the diagsArea to be returned as part of the SQL_EXEC_Fetch
  // But SQL_EXEC_Exec will continue to return 0 to avoid
  // breaking the implied protocol.
  if (diagsArea != NULL) {
     if (retcode == 0 && diagsArea->mainSQLCODE() > 0)
        // It's a warning. So return 1. That's what the cli expects.
        retcode = 1;
     else if (diagsArea->mainSQLCODE() < 0)
        // It's an error. Return the negative value.
        retcode = -1;
     else
        // It's a Diags Area w/o any Conditions.
        retcode = 0;
  }
*/
  return retcode;
}

void ex_root_tcb::setupWarning(Lng32 retcode, const char * str,
    const char * str2, ComDiagsArea* & diagsArea)
{
  ContextCli *currContext = GetCliGlobals()->currContext();
  // Make sure retcode is positive.
  if (retcode < 0)
    retcode = -retcode;

  if ((ABS(retcode) >= HBASE_MIN_ERROR_NUM)
      && (ABS(retcode) <= HBASE_MAX_ERROR_NUM))
  {
    if (diagsArea == NULL)
      diagsArea = ComDiagsArea::allocate(getHeap());
    Lng32 cliError = 0;

    Lng32 intParam1 = retcode;
    ComDiagsArea * newDiags = NULL;
    ExRaiseSqlWarning(getHeap(), &newDiags, (ExeErrorCode) (8448), NULL,
        &intParam1, &cliError, NULL, (str ? (char*) str : (char*) " "),
        getHbaseErrStr(retcode),
        (str2 ? (char*) str2 : (char *) GetCliGlobals()->getJniErrorStr()));
    diagsArea->mergeAfter(*newDiags);
  }
  ex_assert( 0, "invalid return code value");
}

void ex_root_tcb::snapshotScanCleanup(ComDiagsArea* & diagsArea)
{
  const char * tmpLoc = root_tdb().getSnapshotScanTempLocation();
  if (tmpLoc == NULL)
    return;

  ExpHbaseInterface* ehi = ExpHbaseInterface::newInstance
                           (STMTHEAP, "", "");

  ex_assert(ehi != NULL, "cannot connect to HBase");
  Int32 retcode = ehi->init(NULL);
  if (retcode != 0)
  {
    setupWarning(retcode, "ExpHbaseInterface::init", "", diagsArea);
  } 
  else
  {
    retcode = ehi->cleanSnpTmpLocation(tmpLoc);
    if (retcode != 0)
      setupWarning(retcode, "ExpHbaseInterface::cleanSnpTmpLocation", "", diagsArea);
    if (root_tdb().getListOfSnapshotScanTables())
      root_tdb().getListOfSnapshotScanTables()->position();
    for (int i = 0; i < root_tdb().getListOfSnapshotScanTables()->entries(); i++)
    {
      char * tbl = (char*) root_tdb().getListOfSnapshotScanTables()->getCurr();
      retcode = ehi->setArchivePermissions((const char *) tbl);
      if (retcode != 0)
      {
        setupWarning(retcode, "ExpHbaseInterface::setArchivePermissions", "", diagsArea);
      }
      root_tdb().getListOfSnapshotScanTables()->advance();

    }
  }
  delete ehi;
  return;
}
////////////////////////////////////////////////////////
// RETURNS: 0, success. 100, EOF. -1, error. 1, warning
////////////////////////////////////////////////////////
Int32 ex_root_tcb::fetch(CliGlobals *cliGlobals,
		       ExExeStmtGlobals * glob, 
		       Descriptor * output_desc,
		       ComDiagsArea* & diagsArea,
                       Lng32 timeLimit,
		       NABoolean newOperation,
		       NABoolean &closeCursorOnError)
{
  // see details of this param in ex_root.h::fetch() declaration.
  closeCursorOnError = TRUE;
  //
  // This procedure retrieves the row returned from the child's up
  // queue, evaluates the output expression, and returns the result
  // into the output host variables. All the entries from the child's
  // up queue are returned first. The scheduler is invoked to do more
  // processing once the queue becomes empty.
  //

  // For the GET_NEXT_N protocol, we should only return when a Q_GET_DONE is
  //  received.  In addition, due to the incomplete implementation of the 
  //  GET_NEXT_N protocol, it is possible to receive a Q_GET_DONE without
  //  receiving any rows.  To work around that, we re-issue a GET_NEXT_N if
  //  that happens.
  // For the non-streaming destructive cursor (which also uses the GET_NEXT_N
  //  protocol).  A Q_NO_DATA is returned instead of a Q_GET_DONE is there
  //  were less than 'N' rows to be returned.
  Int32 rowsReturned = 0;
  NABoolean nextIsQNoData         = FALSE;
  if (newOperation)
    time_of_fetch_call_usec_      = NA_JulianTimestamp();

  ExMasterStmtGlobals *master_glob = glob->castToExMasterStmtGlobals();

  // start off by calling the scheduler (again)
  ExWorkProcRetcode schedRetcode = WORK_CALL_AGAIN;
  

// mjh -- I'd like to move this test down to after the scheduler 
//  has returned (consolidate it w/ the other new stuff), and
// only do this code if WORK_BAD_ERROR has been returned.  After I 
// have built browse info, check and see if places that set globals
// da could also return WORK_BAD_ERROR???  (Build-time is okay exception).

  if (glob->getDiagsArea() && glob->getDiagsArea()->mainSQLCODE() < 0)
    {
      // fatal error coming back in the globals, don't fetch
      
      // $$$$ need to do any cleanup, like close cursor?
      diagsArea = glob->getDiagsArea();
      diagsArea->incrRefCount();
      glob->setGlobDiagsArea(NULL);
      return -1;
    }

  if (fatalError_)
    {
      // Can't fetch. I'm dead.
      diagsArea = ComDiagsArea::allocate(getHeap());
      *diagsArea << DgSqlCode(-EXE_CANNOT_CONTINUE);
      return -1;
    }
  
  Lng32 numLoopFetches    = 1;
  Lng32 outputRowsetSize  = 0;
  Lng32 compileRowsetSize = -1;
  NABoolean doneWithRowsets = TRUE;
  
  if (output_desc != NULL) {
    if ((newOperation) && 
	(NOT output_desc->rowwiseRowsetEnabled()))
    {
      output_desc->setDescItem(0, SQLDESC_ROWSET_NUM_PROCESSED, 0, 0);
      output_desc->setDescItem(0, SQLDESC_ROWSET_HANDLE, 0, 0);
    }

    output_desc->getDescItem(0, SQLDESC_ROWSET_SIZE, &outputRowsetSize, 
                             0, 0, 0, 0);
    doneWithRowsets = (outputRowsetSize == 0);
    // In case we are dealing with compound statements
    output_desc->setCompoundStmtsInfo(((ComTdbRoot *) &tdb)->getCompoundStmtsInfo());
  }

  // For an active destructive select, a new GET_NEXT_N-type command needs
  //  to be issued for every new fetch.
  if (newOperation && root_tdb().isEmbeddedUpdateOrDelete() &&
      !qchild.down->isEmpty())
    {
      ex_queue_entry * dentry = qchild.down->getHeadEntry();
      if (dentry->downState.numGetNextsIssued >5)
	{
	  if (root_tdb().isStreamScan())
	    // either 1 or output rowset size
	    qchild.down->getNextNMaybeWaitRequestInit((outputRowsetSize?outputRowsetSize:1));
	  else
	    // either 1 or output rowset size
	    qchild.down->getNextNRequestInit((outputRowsetSize?outputRowsetSize:1));
	}
      else
	{
	  if (root_tdb().isStreamScan())
	    // either 1 or output rowset size
	    qchild.down->getNextNMaybeWaitRequest((outputRowsetSize?outputRowsetSize:1));
	  else
	    // either 1 or output rowset size
	    qchild.down->getNextNRequest((outputRowsetSize?outputRowsetSize:1));
	}
    }

  ContextCli   & currContext = *(master_glob->getStatement()->getContext());
  ComDiagsArea & diags       = currContext.diags();
  Lng32 numOfCliCalls = currContext.getNumOfCliCalls(); 

  Lng32 queueSize = 0;

  // Indicates if Cli has converted a rowset from executor format to user
  // space format
  NABoolean rowsetConvertedInCli = FALSE; 

  ExMasterStats* mStats = NULL;
  StmtStats* sStats = master_glob->getStatement()->getStmtStats();
  if(sStats)
    mStats = sStats ->getMasterStats();

  ExOperStats *statsEntry = getStatsEntry();
  IpcEnvironment * ipcEnv =
                glob->castToExExeStmtGlobals()->getIpcEnvironment();
  
  while (1) // return 
    {
      queueSize = (Lng32)qchild.up->getLength();
      Lng32 retcode = 0;

      // The first condition in this if statement means that there is 
      // are number of rows in the queue that do not fit in the rowset, so
      // we retrieve only those that fit. The second condition means that we 
      // have no more rows to retrieve than those in the up queue. Note that
      // if none of these conditions are met, the else part of this statement
      // will cause us to call the scheduler to retrieve more rows
      if ((queueSize > outputRowsetSize) ||
          ((outputRowsetSize > 0) && (queueSize > 0) && (lastQueueSize_ == queueSize))) {

        lastQueueSize_ = queueSize;

        ex_queue_entry * centry = qchild.up->getHeadEntry();
        // if getFirstNRows() is set to -2, then don't output rows
        // and don't return after each fetch. Only return at EOD.
	// Do this only for user query invocation at the top cli level.
	// Don't do this for internal queries.
        NABoolean dontReturn = 
	  ((root_tdb().getFirstNRows() == -2) &&
	   (numOfCliCalls == 1) &&
               NOT (centry->upState.status == ex_queue::Q_NO_DATA));

        if (compileRowsetSize == -1) {
          // Find out if the SQL statement was compiled with output 
          // Rowsets
//          ex_queue_entry * centry = qchild.up->getHeadEntry();
          if (centry->upState.status ==  ex_queue::Q_OK_MMORE) {
            if (outputExpr()) {
              compileRowsetSize = 
                outputExpr()->getCompiledOutputRowsetSize(centry->getAtp());
            }

            if (compileRowsetSize < 0)
              compileRowsetSize = 0;

            if (compileRowsetSize == 0 && outputRowsetSize > 0) {
              // The SQL statement is likely a cursor that was compiled
              // without rowset output manipulation. In this case we need to fetch
              // one row at a time and put it in the output descriptor.
              // This code is also exercised for dynamic rowset selects where
              // we fetch multiple rows at a time and put it in the output descriptor.
              numLoopFetches = outputRowsetSize;
            }
          }
        }

        if (queueSize > numLoopFetches) {
          queueSize = numLoopFetches;
        }

        for(Lng32 i = 0; i < queueSize && (retcode == 0 || retcode ==1); i++) {

	    if (outputRowsetSize > 0 && doneWithRowsets) {
	      break;
	    }

            centry = qchild.up->getHeadEntry();

            if ((centry->upState.status != ex_queue::Q_OK_MMORE) &&
                (centry->upState.status != ex_queue::Q_GET_DONE)) {

              doneWithRowsets = TRUE;

              //  if we encounter a Q_NO_DATA
	      //  and we already have accumulated rows to be returned, then
	      //  return the Q_NO_DATA on the next fetch.
	      nextIsQNoData
		= (centry->upState.status ==  ex_queue::Q_NO_DATA);

		if (rowsReturned && nextIsQNoData) {
		break;
		}

	      // If we get here then status = Q_SQLERROR. With the
	      // new behaviour of fetch all we want to do is return the
	      // error info along with data. We do not want to break here.
	    }

            switch (centry->upState.status)
              {
              case ex_queue::Q_OK_MMORE:
                {
                  if (predExpr())
                    {
                      ex_expr::exp_return_type exprRetCode =
                        predExpr()->eval(centry->getAtp(), NULL) ;
                          
                      if (exprRetCode == ex_expr::EXPR_ERROR)
                        {
                          retcode = -1;
                          break;
                        }
                      else if (exprRetCode == ex_expr::EXPR_FALSE)
                        {
                          dontReturn = TRUE;
                          break;
                        }
                    }

		  // Remember how many rows have been returned before a
		  // Q_GET_DONE or Q_NO_DATA tuple is encountered.  Needed for
		  // destructive cursors and dynamic rowset selects .
		  rowsReturned++;      
		  
		  // update operator stats
		  if (statsEntry)
		    statsEntry-> incActualRowsReturned();	
                  
                  // update master stats also
                  if(mStats)
                    mStats->incRowsReturned();
		    		  
                  // row was returned from child.Evaluate output expression.
                  if (outputExpr())
                    {
                      if ((NOT outputExpr()->noEval()) &&
			  (outputExpr()->eval(centry->getAtp(), 0)
                          == ex_expr::EXPR_ERROR)) {
                        retcode = -1;
                        break;
                      }

                      if (output_desc)
			{
			  UInt32 flags = 0;
			  outputExpr()->setIsODBC(flags, root_tdb().odbcQuery());
			  outputExpr()->setInternalFormatIO
			    (flags,
			     currContext.getSessionDefaults()->
			     getInternalFormatIO());

			  outputExpr()->setIsDBTR
			    (flags,
			     master_glob->getStatement()->getContext()->getSessionDefaults()->
			     getDbtrProcess());
			  
                          if (outputExpr()->outputValues(centry->getAtp(), 
							 output_desc, 
							 getHeap(),
							 flags)
			    == ex_expr::EXPR_ERROR)
			    {
			      closeCursorOnError = FALSE;
			      retcode = -1;
			      break;
			    }
			}

                      // if primary key is to be returned, create a tuple
                      // containing the pkey row.
                      if (pkeyExpr())
                        {
                          if (pkeyExpr()->eval(centry->getAtp(), pkeyAtp_) 
                              == ex_expr::EXPR_ERROR)
                            {
                              retcode = -1;
                              break;
                            }
                        }
                    }

                  if (output_desc != NULL) {
		    if (outputRowsetSize > 0)
		    {
			
		      Lng32 numProcessed;
                      output_desc->getDescItem(0, 
                                               SQLDESC_ROWSET_NUM_PROCESSED, 
                                               &numProcessed, 0, 0, 0, 0);
		      if (numLoopFetches > 1 && (numProcessed < outputRowsetSize)) {
		        // This section gets executed if we are in a FETCH
		        // stmt. for rowsets. It counts the number of rows fetched
                        output_desc->setDescItem(0, 
                                                 SQLDESC_ROWSET_ADD_NUM_PROCESSED,
                                                 0, 0);
		      }

		      // Special case when rowset size is 1
		      if ((numLoopFetches == 1) && (numProcessed == 0) &&
                          (outputRowsetSize == 1)) {
                        output_desc->setDescItem(0, 
                                                 SQLDESC_ROWSET_ADD_NUM_PROCESSED,
                                                 0, 0);
		      }

                      // Update the total number of rows processed in the rowset.
                      output_desc->getDescItem(0, 
                                               SQLDESC_ROWSET_NUM_PROCESSED, 
                                               &numProcessed, 0, 0, 0, 0);
                      diags.setRowCount(numProcessed);
		      //master_glob->setRowsAffected(numProcessed);

                      // Set row number in row's diags if any
                      ComDiagsArea *rowDiags = centry->getAtp()->getDiagsArea();
                      if (rowDiags != NULL) {
                        Int32 index;
                        for (index=1;
                             index <= rowDiags->getNumber(DgSqlCode::WARNING_);
                             index++) {
                          rowDiags->getWarningEntry(index)->setRowNumber(numProcessed);
                        }
                        for (index=1;
                             index <= rowDiags->getNumber(DgSqlCode::ERROR_);
                             index++) {
                          rowDiags->getErrorEntry(index)->setRowNumber(numProcessed);
                        }
                      }

		      // Do not return if the rowset is not full yet and there is
		      // data to be processed
                      // ------
                      // the condition on dontRetun ensures that when processing under LAST0_MODE cqd
                      // we do not return from fetch even when the number of rowsProcessed exceeds 
                      // rowset size. Since no rows are returned when last0_mode is set we do not return
                      // from fetch till we see Q_NODATA. This if block allows one to return from fetch
                      // after an OK_MMORE, so we are disallowing it when LAST0_MODE is set.
		      if (((numProcessed >= outputRowsetSize) || 
                           (numLoopFetches <= 1)) && 
                          (!dontReturn)) {
                        doneWithRowsets = TRUE; 
		      }
                    }
                  } 
                  break;
                }
                
              case ex_queue::Q_NO_DATA:
                {
                  dontReturn = FALSE; // i.e., do return
		  
                  retcode = 100;
		  
                  // done with this request, indicate this to frag dir
		  
		  getGlobals()->
                    castToExExeStmtGlobals()->
                    castToExMasterStmtGlobals()->
                    getRtFragTable()->setInactiveState();
		  
		  // mark the stats area to indicate if stats
		  // were collected. This method will get this
		  // information from globals and set it in the
		  // statsArea that is pointed to by stmt globals.
		  ExStatisticsArea *stats = getGlobals()->getOrigStatsArea();
                  if (stats)
		    stats->setStatsEnabled(getGlobals()->statsEnabled());

#ifdef NA_DEBUG_GUI
                  if (root_tdb().displayExecution() > 0)
                    getGlobals()->getScheduler()->stopGui();
                  ex_tcb::objectCount = 0;
#endif
                  // Make the rowset handle available
                  if (output_desc != NULL)
                    output_desc->setDescItem(0, 
                                             SQLDESC_ROWSET_HANDLE,
                                             0, 0);
                }
              break;

              case ex_queue::Q_SQLERROR:
		{
		  // Converts error -EXE_CS_EOD to warning EXE_CS_EOD and sets
		  // retcode to 0 if such a warning is found and no other error 
		  // is present, else retcode is -1. Setting retcode to 0 allows
		  // processing to continue till we get to Q_N0_DATA
		  ComDiagsArea * da = centry->getAtp()->getDiagsArea();
		  if ( da && da->contains(-EXE_CS_EOD)) {
                    da->negateCondition(da->returnIndex(-EXE_CS_EOD));
                    CollIndex index;
		    while ((index = da->returnIndex(-EXE_CS_EOD)) != NULL_COLL_INDEX ) {
		      da->deleteError(index);  
		    }
		    if (da->getNumber(DgSqlCode::ERROR_) == 0) {
                      retcode = 0;
		    }
		    else {
		      retcode = -1;
		    }
                  }
		  else {
		    retcode = -1;
                  }
		  
		  if (retcode == -1)
                  {
                    // mark the stats area to indicate if stats
                    // were collected. This method will get this
                    // information from globals and set it in the
                    // statsArea that is pointed to by stmt globals.
                    ExStatisticsArea *stats = getGlobals()->getOrigStatsArea();
                    if (stats)
                      stats->setStatsEnabled(getGlobals()->statsEnabled());
                  }
	        }
	      break;
	      
	      case ex_queue::Q_GET_DONE:
		{
		  // mark the stats area to indicate if stats
		  // were collected. This method will get this
		  // information from globals and set it in the
		  // statsArea that is pointed to by stmt globals.
		  ExStatisticsArea *stats = getGlobals()->getOrigStatsArea();
                  if (stats)
		    stats->setStatsEnabled(getGlobals()->statsEnabled());

		  // Only the destructive select will get Q_GET_DONE signals.
		  //  This is in response to GET_NEXT_N and
		  //  GET_NEXT_N_MAYBE_WAIT commands.  This Q_GET_DONE signals
		  //  that there are no more rows in this "bunch".
		  // Note that this for-loop only loops for the number of row
		  //  expected and we expect here the number of rows PLUS a
		  //  Q_GET_DONE.  We will fall out of the loop and get back
		  //  in it via the while loop.  This is a little inefficient
		  //  but the good thing is that I don't need to touch the
		  //  fragile for loop.
		  if (rowsReturned == 0)
		    {
		      // This Q_GET_DONE finishes the current bunch, this means
		      //  that we have not received a row to return to the
		      //  fetch.
		      // This is a destructive select.  No rows were returned
		      //  because the full GET_NEXT protocol is not yet
		      //  implemeted. Work around it by issueing a new command.
		      if ((streamTimeout_ >= 0) &&   
			  ((time_of_fetch_call_usec_ + streamTimeout_*10000) < NA_JulianTimestamp()))
			{
			  // We have spend enough time in fetch().
			  // It is time to give back control to the program.
			  // Note that this cursor is a read-only cursor, so
			  //  there is not problem with outstanding requests
			  //  (which there probably are) to DP2 damaging
			  //  things while the program is given the chance to
			  //  commit.
			  // Note that streamTimeout_ is in .01 seconds and
			  //  NA_JulianTimestamp is in micro seconds
                          if (diagsArea == NULL)
                            {
			      diagsArea = ComDiagsArea::allocate(getHeap());
                            }
			  *diagsArea << DgSqlCode(-EXE_STREAM_TIMEOUT);
			  // This is not a tuple that needs to be returned.
			  qchild.up->removeHead();
			  return 1; // warning  -- we return 1 (a warning) here
                                    // , because we want Statement::fetch to behave a little differently
                                    // than when other errors are raised: usually, Statement::fetch 
                                    // would rollback the transaction and cancel the statment (i.e., close 
                                    // the cursor).  Instead, when this Condition is raised, we 
                                    // want it to leave the trans active and let the cursor be used 
                                    // for more fetches if the client desires.
			}
#ifdef TRACE_PAPA_DEQUEUE
                      cout << "ROOT empty Q_GET_DONE, no timeout" << endl;
#endif
		      if (root_tdb().isStreamScan())
			qchild.down->getNextNMaybeWaitRequest((outputRowsetSize?outputRowsetSize:1)); 
		      else
			qchild.down->getNextNRequest((outputRowsetSize?outputRowsetSize:1)); 
		    }
		  else
		    {
		      // This Q_GET_DONE finishes the current bunch.  We have
		      //  moved at least one row to the caller;  return now.
		      // This is not a tuple that needs to be returned.
		      qchild.up->removeHead();

		    // Note that the retcode is already set since we have
		    //  already moved rows.
		      return retcode;
		    }
		}
	      break;	      

              case ex_queue::Q_INVALID:
                ex_assert(0, "invalid state returned from child");
                break;
              } // end of switch
	  
            // Set output parameter diagsArea to the diagnostics area
            // associated with the current entry, if there is one
            // associated with it.  Be careful not to leak diags areas
            // by merging into diagsArea if it has already been init'd,
            // as can happen with rowsets.
	  
	    /* if (root_tdb().isNonFatalErrorTolerated())
	      {
		// remove dup NF errors if any
		if (centry->getDiagsArea())
		  (centry->getDiagsArea())->removeDupNFConditions();
		  }*/

            if (diagsArea == NULL)
              {
                diagsArea = ex_root_tcb::moveDiagsAreaFromEntry (centry);
              }
            else 
              {
                if (centry->getDiagsArea())
                  {
                    diagsArea->mergeAfter(*centry->getDiagsArea());
                  }
              }

            if (diagsArea != NULL)
              {
                if (retcode == 0 && diagsArea->mainSQLCODE() > 0)
                  // It's a warning. So return 1. That's what the cli expects.
                  retcode = 1;
                else if (diagsArea->mainSQLCODE() < 0)
                  {
                    // It's an error. Return the negative value.
                    retcode = -1; 
                  }
                else
                  ; // It's a Diags Area w/o any Conditions.
              }

	  
            qchild.up->removeHead();
	  
            // this may have woken up the scheduler, make sure we call it
            schedRetcode = WORK_CALL_AGAIN;
          }//loop for(Lng32 i = 0; i < queueSize && (retcode == 0 || retcode ==1); i++)

	if (retcode == 100)
	  {
	  completeOutstandingCancelMsgs();
	  glob->testAllQueues();
	  }

	if (dontReturn)
	  rowsReturned = 0;
	else if (retcode)
	{
	  if ((retcode < 0) || (retcode == 100) || ((retcode > 0) && doneWithRowsets))
	  {
            ipcEnv->deleteCompletedMessages(); // cleanup messages
            if (root_tdb().getSnapshotScanTempLocation())
            {
              snapshotScanCleanup(diagsArea);
            }
	    return retcode;
	  }
	}
	else if (root_tdb().isEmbeddedUpdateOrDelete())
	  {
	    // We get here in the following scenarios:
	    //  - we have received not tuples yet => go wait for more
	    //  - we have received data tuples => go wait for a Q_GET_DONE
	    //  - we have received data tuples and the next tuple is a
	    //	  Q_NO_DATA => no Q_GET_DONE will be coming, return the 
	    //	  data tuples and return the Q_NO_DATA on the next fetch.
	    // In the case of a destructive cursor, the GET_NEXT_N protocol
	    //  is used and we can only return when a complete 'bunch' or rows
	    //  is received, and that is identified by a Q_GET_DONE tuple.
	    // Consequently, only after reception of a Q_GET_DONE can we
	    //  return (which the Q_GET_DONE code above does).
	    // Note that a non-streaming destructive cursor returns
	    //  Q_NO_DATA when no more rows are available (just like any
	    //  other cursor).
	    if (nextIsQNoData)
	      {
                // We have encountered a Q_NO_DATA.  Earlier, if we encountered a non-Q_OK_MMORE, we
                // set doneWithRowsets to TRUE.  We assert that no other developer has changed this 
                // logic.
                // 
                ex_assert(doneWithRowsets, "Invalid logic in ex_root_tcb::fetch");
                ipcEnv->deleteCompletedMessages(); // cleanup messages
		// This non-streaming destructive cursor encountered
		//  Q_NO_DATA but there are rows to be returned.  Return them
		//  now.
		return retcode;
	      }
	    else
	      {
		// This destructive cursor is still waiting for a
		//  Q_GET_DONE or a Q_NO_DATA.
                // Don't let the rowset code make us return before processing
                // the Q_GET_DONE (genesis CR 10-000517-0131).  It is safe to
                // reset doneWithRowsets to FALSE, because the embeddedUpdateOrDelete
                // statements use the GET_NEXT_N protocol, limiting the N (
                // (# rows requested) to the size of the rowset.
		doneWithRowsets = FALSE;
		;
	      }
	  }
	else if (doneWithRowsets) 
          {
            ipcEnv->deleteCompletedMessages(); // cleanup messages

            // row is being returned now.
            // Increment rownum.
            getGlobals()->rowNum()++;

            return retcode;
          } 
	}
	else
        {

          lastQueueSize_ = queueSize;

	  // make sure we wake our ESPs if this is the first fetch
	  // after quiescing the executor
	  master_glob->getRtFragTable()->continueWithTransaction();

          NABoolean earliestTimeToCancel = FALSE;
          // wait for external events if the scheduler indicated
          // last time that it had nothing to do
          NABoolean timedout;
          Int64 prevWaitTime = 0;
          if (schedRetcode == WORK_OK || schedRetcode == WORK_POOL_BLOCKED)
            {
	      IpcTimeout wait_timeout = -1; // default is wait forever
	      // We are about to block. If this is a streaming non-destructive
	      //  cursor, then make sure we don't get stuck for too long.
	      if (root_tdb().isStreamScan() && !root_tdb().isEmbeddedUpdateOrDelete())
		{

		  // If we are not waiting forever, then chech if we waited
		  //  long enough.
		  if (streamTimeout_ >= 0)
                    {
                    wait_timeout =  streamTimeout_;
		    // Note that NA_JulianTimestamp is in micro seconds and that
		    //  streamTimeout_ is in .01 seconds
                    Int64 wait64 = streamTimeout_; // to avoid timeout overflow
                    // Extra time may be needed
                    IpcTimeout extraTime = 0;
                    if (getGlobals()->statsEnabled())
                        extraTime = 100;
                  
                    if ((time_of_fetch_call_usec_ + (wait64+extraTime)*10000) < NA_JulianTimestamp())
	
		      {
		        // We have spend enough time in fetch().
		        // It is time to give back control to the program.
		        // Note that this cursor is a read-only cursor, so there
		        //  is not problem with outstanding requests (which
		        //  there probably are) to DP2 damaging things while
		        //  the program is given the chance to commit.
                        if (diagsArea == NULL)
                          {
		            diagsArea = ComDiagsArea::allocate(getHeap());
                          }
		        *diagsArea << DgSqlCode(-EXE_STREAM_TIMEOUT);
		        return 1; // warning  -- we return 1 (a warning) here
                                  // , because we want Statement::fetch to behave a little differently
                                  // than when other errors are raised: usually, Statement::fetch 
                                  // would rollback the transaction and cancel the statment (i.e., close 
                                  // the cursor).  Instead, when this Condition is raised, we 
                                  // want it to leave the trans active and let the cursor be used 
                                  // for more fetches if the client desires.
		      }
		   }
		}

              // $$$ no-wait CLI prototype VV
// Obsolete in SQ.
              if (timeLimit == 0)
                {
                // the fetch is nowaited and time has expired, so return to caller now
                // note that no diagnostic is generated
                // $$$ need to check that we don't lose any outstanding
                // diagnostics in this way...
                return NOT_FINISHED;
                }

              // $$$ no-wait CLI prototype ^^

              // Wait for any I/O to complete, this includes both
              // ARKFS and ESP connections. Use an infinite timeout.
              ipcEnv->getAllConnections()->waitOnAll((Lng32) wait_timeout, FALSE, &timedout, &prevWaitTime);
              
              // clean up the completed MasterEspMessages
              ipcEnv->deleteCompletedMessages();
              earliestTimeToCancel = TRUE;
           }
	  
          // redrive the scheduler
          schedRetcode = glob->getScheduler()->work(prevWaitTime);

#ifdef _DEBUG
// We do not need coverage for DEBUG builds since QA or customers do no recieve.
          if (earliestTimeToCancel && getenv("TEST_CANCEL_ABORT"))
            {
              if (diagsArea == NULL)
                {
                  diagsArea = ComDiagsArea::allocate(getHeap());
                }
              *diagsArea << DgSqlCode(-EXE_CANCEL_INJECTED);
              return -1;
            }
#endif

          if (schedRetcode == WORK_BAD_ERROR)
            {
              return fatal_error(glob, diagsArea);
            }
          else if (schedRetcode == WORK_NOBODY_WORKED)
            {
              // for now, map this back to WORK_OK.
              // I think we'll be able to detect infinite loops
              // in the master after we add code to test that there
              // are no IPC connections waiting.  It may also be
              // necessary to handle cases where all the work was
              // done in root::execute's call to scheduler::work
              // as can happen, perhaps, with waited I/O (or 
              // statements w/ no I/O).
              schedRetcode = WORK_OK;
            }
        }

    } // while
}

////////////////////////////////////////////////////////
// RETURNS: 0, success. 100, EOF. -1, error. 1, warning
////////////////////////////////////////////////////////
Int32 ex_root_tcb::fetchMultiple(CliGlobals *cliGlobals,
			       ExExeStmtGlobals * glob, 
			       Descriptor * output_desc,
			       ComDiagsArea* & diagsArea,
			       Lng32 timeLimit,
			       NABoolean newOperation,
			       NABoolean &closeCursorOnError,
			       NABoolean &eodSeen)
{
  Int32 retcode = 0;
  NABoolean keepFetching = TRUE;

  Lng32 numTotalRows = 0;
  Lng32 numRowsProcessed = 0;
  output_desc->getDescItem(0, SQLDESC_ROWWISE_ROWSET_SIZE,
			   &numTotalRows, NULL, 0, NULL, 0);

  output_desc->setDescItem(0, SQLDESC_ROWSET_NUM_PROCESSED,
			   numRowsProcessed, NULL);

  while ((output_desc->rowwiseRowsetEnabled()) &&
         keepFetching &&
	 (numRowsProcessed < numTotalRows))
    {
      retcode = fetch(cliGlobals, glob, output_desc, diagsArea, timeLimit,
		      newOperation, closeCursorOnError);
        
      if (retcode < 0)  
	keepFetching = FALSE;
           
      if( retcode == 100)

	{
	  // If there are warnings  accompanying this EOD, then change 
	  //the retcode to WARNING. MXCS will retrieve diagnostic warnings
	  // only if the retcode from this method is a warning. If we return
	  // EOD then they ignore the warnings.
	  if (diagsArea && diagsArea->getNumber(DgSqlCode::WARNING_)>0)
	    retcode =1;
	  // we will set the statement state to EOF_ after this call so 
	  // on a redrive of a fetch, we will return EOD right away. 
	  keepFetching = FALSE;
	  eodSeen = TRUE;
	}
          
      else if (retcode == 1 && diagsArea && 
	  diagsArea->containsError(-EXE_STREAM_TIMEOUT))
	{
	  if (numRowsProcessed > 0)
	    {
	      // Here we have a stream timeout (-8006) ComCondition in the 
	      // diagsArea, but some rows have already been placed into
	      // the rowset.  To make sure that the ODBC and JDBC clients
	      // get these rows, we need to convert the error into a 
	      // warning.  There is probably only one such ComCondition,
	      // but to be safe we'll convert any that we find.
	      CollIndex ste = diagsArea->returnIndex(-EXE_STREAM_TIMEOUT);
	      do {
		diagsArea->negateCondition(ste);
		ste = diagsArea->returnIndex(-EXE_STREAM_TIMEOUT);
	      } while (ste != NULL_COLL_INDEX);
	    }
	  else
	    {
	      // There weren't any rows in the rowset when the stream
	      // timed out.  So leave the ComCondition as an error, so
	      // that the caller will stop fetching.
	    }
	  // Regardless of whether the rowset was partially populated,
	  // this method wil return 1 (warning), not -1 (error) so that
	  // the Statement will not rollback the transaction and thereby
	  // also close the cursor.  At a higher level of the CLI, the 
	  // DiagsArea::mainSQLCode is used to return the -8006 or 8006
	  // to the caller.
	  keepFetching = FALSE;
	}
      else
	if (eodSeen != TRUE)
	  numRowsProcessed++;

      output_desc->setDescItem(0, SQLDESC_ROWSET_NUM_PROCESSED,
			       numRowsProcessed, NULL);
    }

  return retcode;
}


//////////////////////////////////////////////////////
// OLT execute. Does execute/fetch in one shot. Used
// for OLT queries that do OLT optimization.
//////////////////////////////////////////////////////
Int32 ex_root_tcb::oltExecute(ExExeStmtGlobals * glob, 
			    Descriptor * input_desc,
			    Descriptor * output_desc,
			    ComDiagsArea*& diagsArea)
{
  ExMasterStmtGlobals *master_glob = getGlobals()->
	            castToExExeStmtGlobals()->castToExMasterStmtGlobals(); 

  ex_queue_entry *entry = qchild.down->getTailEntry();
  entry->downState.request = ex_queue::GET_ALL;
  entry->downState.requestValue = 11;
  entry->downState.parentIndex = 20;
  entry->copyAtp(workAtp_);
 
  if (inputExpr())
    {   
      // In case we are dealing with compound statements
      if (input_desc) 
	{
	  input_desc->setCompoundStmtsInfo(((ComTdbRoot *) &tdb)->getCompoundStmtsInfo());
	}
      
      UInt32 flags = 0;
      inputExpr()->setIsODBC(flags, root_tdb().odbcQuery());
      inputExpr()->setInternalFormatIO
	(flags,
	 master_glob->getStatement()->getContext()->getSessionDefaults()->
	 getInternalFormatIO());
      inputExpr()->setIsDBTR
	(flags,
	 master_glob->getStatement()->getContext()->getSessionDefaults()->
	 getDbtrProcess());

      // move the values from user area to internal area
      if (inputExpr()->inputValues(entry->getAtp(),
				   input_desc,
				   FALSE, // do not tolerate nonfatal errors
				   glob->getDefaultHeap(),
				   flags) ==
	  ex_expr::EXPR_ERROR) 
	{
	  diagsArea = ex_root_tcb::moveDiagsAreaFromEntry(entry);
	  return -1;
	};
      
      // and evaluate any expression on those input values
      if ((NOT inputExpr()->noEval()) &&
	  (inputExpr()->eval(entry->getAtp(), 0) == ex_expr::EXPR_ERROR))
        {
          diagsArea = ex_root_tcb::moveDiagsAreaFromEntry(entry);
          return -1;
        }
    } // input expr


  ExMasterStats *mStats = NULL;
  StmtStats *sStats = master_glob->getStatement()->getStmtStats();
  if(sStats)
    mStats = sStats ->getMasterStats();

  if ( checkTransBeforeExecute(
          master_glob->getStatement()->getContext()->getTransaction(),
	  master_glob,
          mStats, diagsArea) )
    return -1;

  qchild.down->insert();

  registerCB(diagsArea);

  cpuLimitExceeded_ = FALSE;

  // now check for returned rows from child

  // indicate to fragment table that we have a new request. This would
  // activate ESPs to work, otherwise query will hang. If no ESPs is used
  // this call would like no-op.
  // (ignore return code for now, since we never process multiple requests)
  master_glob->getRtFragTable()->setActiveState();

  while (1)
    {
      // do some work, but not necessarily all of it
      if (glob->getScheduler()->work() == WORK_BAD_ERROR)
	{
	  completeOutstandingCancelMsgs();
	  return fatal_error(glob, diagsArea);
	}
      Lng32 retcode = -1;
      NABoolean setRetcode = TRUE;
      while (!qchild.up->isEmpty()) 
	{
	  ex_queue_entry * centry = qchild.up->getHeadEntry();
	  switch (centry->upState.status)
	    {
	    case ex_queue::Q_OK_MMORE:
	      {
		// row was returned from child.Evaluate output expression.
		if (outputExpr())
		  {
		    if ((NOT outputExpr()->noEval()) &&
			(outputExpr()->eval(centry->getAtp(), 0)
			 == ex_expr::EXPR_ERROR))
		      {
			diagsArea = ex_root_tcb::moveDiagsAreaFromEntry (centry);

			retcode = -1;
			setRetcode = FALSE;
			break;
		      }
		    
		    if (output_desc) 
		      {
			UInt32 flags = 0;
			outputExpr()->setIsODBC(flags, root_tdb().odbcQuery());
			outputExpr()->setInternalFormatIO
			  (flags,
			   master_glob->getStatement()->getContext()->
			   getSessionDefaults()->
			   getInternalFormatIO());
			outputExpr()->setIsDBTR
			  (flags,
			   master_glob->getStatement()->getContext()->
			   getSessionDefaults()->
			   getDbtrProcess());
			if (outputExpr()->outputValues(centry->getAtp(), 
						       output_desc, 
						       getHeap(),
						       flags)
			    == ex_expr::EXPR_ERROR)
			  {
			    diagsArea = ex_root_tcb::moveDiagsAreaFromEntry (centry);
			    
			    retcode = -1;
			    setRetcode = FALSE;
			    break;
			  }
		      }
                  } 

		if (setRetcode)
		  {
		    retcode = 0;
		    diagsArea = ex_root_tcb::moveDiagsAreaFromEntry (centry);
		    setRetcode = FALSE;
		  }

                // update operator stats
                if (getStatsEntry())
                  getStatsEntry() -> incActualRowsReturned();	
		  
                // update master stats rows returned
                if(mStats)
                  mStats->incRowsReturned();

	      }
	    break;
	    
	    case ex_queue::Q_NO_DATA:
	      {
		if (setRetcode)
		  {
		    retcode = 100;
		    diagsArea = ex_root_tcb::moveDiagsAreaFromEntry (centry);
		    setRetcode = FALSE;
		  }
	      }
	    break;
	    
	    case ex_queue::Q_SQLERROR:
	      {
                // Converts error -EXE_CS_EOD to warning EXE_CS_EOD and sets
                // retcode to 0 if such a warning is found and no other error 
                // is present, else retcode is -1. Setting retcode to 0 allows
                // processing to continue till we get to Q_N0_DATA
                ComDiagsArea * da = centry->getAtp()->getDiagsArea();
                if ( da && da->contains(-EXE_CS_EOD)) {
                    da->negateCondition(da->returnIndex(-EXE_CS_EOD));
                    CollIndex index ;
                    while ((index = da->returnIndex(-EXE_CS_EOD)) != NULL_COLL_INDEX ) {
                      da->deleteError(index);  }
                    if (da->getNumber(DgSqlCode::ERROR_) == 0) {
                      if (setRetcode) {
                        retcode = 0;
                      }
                    }
                    else {
                      retcode = -1;
		      setRetcode = FALSE;
                    }   
                }
                else {
                retcode = -1;
		setRetcode = FALSE;
                }
                diagsArea = ex_root_tcb::moveDiagsAreaFromEntry (centry);
                break;
	      }
	    
	    case ex_queue::Q_INVALID:
	      ex_assert(0, "invalid state returned from child");
	      setRetcode = FALSE;
	      break;
	    } // end of switch
	  
	  qchild.up->removeHead();
	    
	} // while child is not empty

      if (NOT setRetcode)
	{
	  if (diagsArea != NULL)
	    {
	      // Its a warning. So return 1. That's what the cli expects.
	      if (retcode == 0)
		retcode = 1;
	      else if (diagsArea->mainSQLCODE() < 0)
		{
		  // its an error. So send in a negative values.
		  // The diagsArea->mainSQLCODE() better be negative!!
		  retcode = (Int32) diagsArea->mainSQLCODE();
		}
	    }
	  return retcode;
	}

      IpcEnvironment * ipcEnv =
	glob->castToExExeStmtGlobals()->getIpcEnvironment();
      // Wait for any I/O to complete, this includes both
      // ARKFS and ESP connections. Use an infinite timeout.
      ipcEnv->getAllConnections()->waitOnAll();
      
    } // while (1)

}

/////////////////////////////////////////////////////////////////////
// Called by the QueryStartedMsgStream::actOnReceive or
// ex_root_tcb::cpuLimitExceeded.  Also, on windows, old prototype
// code called this method from a special cancel thread.
/////////////////////////////////////////////////////////////////////
Int32 ex_root_tcb::requestCancel()
{
  if (qchild.down->isEmpty() && qchild.up->isEmpty())
    ; // Query is already finished.
  else
  {
    ExMasterStmtGlobals *glob = getGlobals()->
             castToExExeStmtGlobals()->castToExMasterStmtGlobals();

    // To guarantee correct concurrent execution, the order of
    // the following three statements must not change.
    glob->castToExMasterStmtGlobals()->setCancelState(CLI_CANCEL_REQUESTED);
    asyncCancelSubtask_->schedule();
    // Call ex_root_tcb::work to start the cancel.
    work();
    // Break the IPC wait loop.
    glob->getIpcEnvironment()->getAllConnections()->cancelWait(TRUE);
  }
  return 0;
}

static IpcTimeout CancelTimeout = -1;
///////////////////////////////////////////
// RETURNS: 0, success. 1, EOF. -1, error.
///////////////////////////////////////////
Int32 ex_root_tcb::cancel(ExExeStmtGlobals * glob, ComDiagsArea *&diagsArea,
			NABoolean getQueueDiags)
{
  if (fatalError_)
    {
      // After fatal error, just ignore cancel, because
      // queues may be unstable and a hang can occur.
    }
  else
    {
      // start off by calling the scheduler (again)
      ExWorkProcRetcode schedRetcode = WORK_CALL_AGAIN;

      qchild.down->cancelRequest();

      // make sure we wake our ESPs if this is the first fetch
      // after quiescing the executor
      glob->castToExMasterStmtGlobals()->getRtFragTable()->
	continueWithTransaction();
      
      // IPC errors (e.g., SQLCODE 2034, error 201 or 14)
      // accumulate on the statement globals. Get rid of
      // them or any other new errors created during cancel.
      ComDiagsArea *globDiagsAreaAtEntry = glob->getDiagsArea();
      Lng32 oldDiagsAreaMark = -1;
      if (globDiagsAreaAtEntry && !getQueueDiags)
        oldDiagsAreaMark = globDiagsAreaAtEntry->mark();

      // while there is anything in the up or down queue
      // between the root node and its child, work to
      // remove whatever is in it.
      while ((!qchild.up->isEmpty() || !qchild.down->isEmpty())
             && (schedRetcode != WORK_BAD_ERROR))
        {
	  // remove all up entries that have accumulated
	  // in the up queue from the child.
	  // Update diagsArea before removing up queue entries from child.
	  while (!qchild.up->isEmpty())
	    {
	      if (getQueueDiags && 
		  qchild.up->getHeadEntry()->getDiagsArea())
		{
		  if (!diagsArea)
		    diagsArea = ComDiagsArea::allocate(glob->getDefaultHeap());
		   
		  diagsArea->mergeAfter((const ComDiagsArea&)*qchild.up->getHeadEntry()->getDiagsArea());
		}
		    
	      qchild.up->removeHead();
	    }

          NABoolean timedOut = FALSE;
          // wait for external events if the scheduler indicated
          // last time that it had nothing to do
          if (schedRetcode == WORK_OK || schedRetcode == WORK_POOL_BLOCKED
              || schedRetcode == WORK_NOBODY_WORKED)
	    {
              if (CancelTimeout < 0)
                {
                  // call getenv once per process
                  char *sct = getenv("SQLMX_CANCEL_TIMEOUT");
                  if (sct)
                  {
                    CancelTimeout = (IpcTimeout) 
                                    str_atoi(sct, str_len(sct)) * 100;
                    if (CancelTimeout < 100)
                      CancelTimeout = 30 * 100;
                  }
                  else
                    CancelTimeout = 30 * 100;
                }
	      // Wait for any I/O to complete, this includes both
	      // ARKFS and ESP connections. Use an infinite timeout.
	      glob->castToExExeStmtGlobals()->getIpcEnvironment()->
	        getAllConnections()->waitOnAll(CancelTimeout, FALSE, 
                                               &timedOut);
	    }
  
          if (timedOut)
            {
            SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, 
                             "IPC timeout in ex_root_tcb::cancel.", 0);
              schedRetcode = WORK_BAD_ERROR;
            }
          else 
            {
              schedRetcode = glob->getScheduler()->work();
            }
        }
      if (!getQueueDiags)
      {
        if (globDiagsAreaAtEntry)
          globDiagsAreaAtEntry->rewind(oldDiagsAreaMark);
        else if (glob->getDiagsArea())
        {
          // newly created diagnostic area in the statement globals.
          // Clear it, since the caller doesn't want new diags.
          glob->getDiagsArea()->clear();
        }
      }

      if (schedRetcode == WORK_BAD_ERROR)
        {
          // deregister to avoid a leak in broker process MXSSMP.
          deregisterCB();
          if (root_tdb().getSnapshotScanTempLocation())
          {
            snapshotScanCleanup(diagsArea);
          }
          return fatal_error(glob, diagsArea, TRUE);
        }

      if (!fatalError_)
        {
          glob->testAllQueues();
        }
    }

  completeOutstandingCancelMsgs();

  // Set cancelState to ensure no more references of 
  // ex_root_tcb by the cancel thread.
  glob->castToExMasterStmtGlobals()->clearCancelState();

  deregisterCB();
  cbMessageWait(0);

  if (root_tdb().getSnapshotScanTempLocation())
  {
    snapshotScanCleanup(diagsArea);
  }
  return 0;
}

Int32 ex_root_tcb::deallocAndDelete(ExExeStmtGlobals *glob,
                                    ExRtFragTable *fragTable)
{
  // Reset cancelState to ensure no more references of 
  // ex_root_tcb by the cancel thread.
  glob->castToExMasterStmtGlobals()->resetCancelState();
   
  // Warning:  deleteMe() will delete this tcb!!!!
  glob->deleteMe(fatalError_); 
  return 0;
}

Int32 ex_root_tcb::closeTables(ExExeStmtGlobals * glob,
			     ExRtFragTable * fragTable)
{
  glob->closeTables(); 

  return 0;
}

Int32 ex_root_tcb::reOpenTables(ExExeStmtGlobals * glob,
			      ExRtFragTable * fragTable)
{
  glob->reOpenTables(); 

  return 0;
}

short ex_root_tcb::work()
{
  // execute and fetch method handle all the work, but the scheduler
  // call this method when an event on the queues to the child
  // occurs
  ExMasterStmtGlobals *master_glob = getGlobals()->
	            castToExExeStmtGlobals()->castToExMasterStmtGlobals(); 

  // Pick up an asynchronous cancel request if any.
  if ((master_glob->getCancelState() == CLI_CANCEL_REQUESTED) &&
      !cancelStarted_)
    {
      // Process the cancel request and push cancel request 
      // down the tcb tree.
      // It is possible that there is a concurrent ordinary cancel().
      // If so, the ordinary cancel will be superseded by 
      // this async cancel, and get -EXE_CANCELED error from 
      // up child queue in cancel().
      // For now, cancel() does not pass back the -EXE_CANCELED.
      cancelStarted_ = TRUE;
      if (!qchild.down->isEmpty())
        qchild.down->cancelRequest();
    }
   
  if (!qchild.up->isEmpty())
    {
      // A new row is available to the root node. Exit the scheduler
      // immediately and let the user see the row. Remember that the
      // scheduler exits for all positive error codes (NSK error codes
      // when used in DP2). Positive error codes are used to communicate
      // a certain exit status from the work procedure to the root.
      if (!cancelStarted_)
	return 1;
      
      // For async cancel.
      // Remove all entries except Q_NO_DATA for the cancel request.
      while (qchild.up->getLength() > 1)
	 qchild.up->removeHead();

      if (!qchild.down->isEmpty())
	{ // Cancel request has not been removed by child.
	  qchild.up->removeHead();
          // WORK_OK will continue scheduler loop.
	  return WORK_OK;
	}
      else
	{ // Async cancel completes.
          // Attach -EXE_CANCELED error to Q_NO_DATA entry.
          // Return 1 to exit the scheduler loop.
	  ex_queue_entry *centry = qchild.up->getHeadEntry();
	 
	  ComDiagsArea *diagsArea = 
	    ex_root_tcb::moveDiagsAreaFromEntry(centry);
	  if (diagsArea == NULL)
	    {
	      diagsArea = ComDiagsArea::allocate(getHeap());
	      centry->setDiagsArea(diagsArea);
	    }
          if (cpuLimitExceeded_)
            {
              *diagsArea << DgSqlCode(-EXE_QUERY_LIMITS_CPU);
              *diagsArea << DgInt0((Lng32) root_tdb().cpuLimit_);
              *diagsArea << DgInt1((Int32) master_glob->getMyFragId());

              if (root_tdb().getQueryLimitDebug())
              {
                *diagsArea << DgSqlCode(EXE_QUERY_LIMITS_CPU_DEBUG);
                *diagsArea << DgInt0((Lng32) root_tdb().cpuLimit_);
                *diagsArea << DgInt1((Int32) master_glob->getMyFragId());

                Int64 localCpuTime = 0;
                ExFragRootOperStats *fragRootStats;
                ExMeasStats *measRootStats;
                ExOperStats *rootStats = master_glob->getStatsArea()->getRootStats();
                if ((fragRootStats = rootStats->castToExFragRootOperStats()) != NULL)
                  localCpuTime = fragRootStats->getLocalCpuTime();
                else if ((measRootStats = rootStats->castToExMeasStats()) != NULL)
                  localCpuTime = measRootStats->getLocalCpuTime();
                else 
                  ex_assert(0, "Cpu limit evaluated without runtime stats.");
  
                *diagsArea << DgInt2((Lng32) localCpuTime / 1000);
              }
            }
          else
	    populateCancelDiags(*diagsArea); 
          
          // Reset these for the next execution.
	  master_glob->clearCancelState();
	  cancelStarted_ = FALSE;
          cpuLimitExceeded_ = FALSE;
	  return 1;
	}
    }
  else
    {
      // some other reason for reaching here (e.g. unblock of
      // down queue to child), just ignore it
      return WORK_OK;
    }
}

ExWorkProcRetcode ex_root_tcb::workOnFragDir()
{

//called by scheduler.
  return getGlobals()->castToExExeStmtGlobals()->
                       castToExMasterStmtGlobals()->
                       getRtFragTable()->workOnRequests();
}

Int32 ex_root_tcb::fatal_error( ExExeStmtGlobals * glob, 
                              ComDiagsArea*& diagsArea,
                              NABoolean noFatalDiags)
{
  if (diagsArea)
    glob->takeGlobalDiagsArea(*diagsArea);
  else if ((diagsArea = glob->getDiagsArea()) != NULL)
    {
    diagsArea->incrRefCount();
    glob->setGlobDiagsArea(NULL);
    }     
  else if (FALSE == noFatalDiags)
    {
    diagsArea = ComDiagsArea::allocate(getHeap());
    *diagsArea << DgSqlCode(-EXE_CANNOT_CONTINUE);
    }

  fatalError_ = TRUE;

  if (noFatalDiags)
    ; // 
  else if (glob->castToExMasterStmtGlobals()->getCancelState() 
      == CLI_CANCEL_REQUESTED)
     populateCancelDiags(*diagsArea);

  Int32 retCode = 0;
  if (diagsArea) 
     retCode = diagsArea->mainSQLCODE();
  return retCode;
}

void ex_root_tcb::completeOutstandingCancelMsgs()
{
  ExMasterStmtGlobals *glob = getGlobals()->castToExExeStmtGlobals()->
                               castToExMasterStmtGlobals();
  
  while (!fatalError_ && 
         (glob->getRtFragTable()->getState() != ExRtFragTable::ERROR) && 
         glob->anyCancelMsgesOut())
  {
    if (root_tdb().getQueryUsesSM() && glob->getIpcEnvironment()->smEnabled())
      EXSM_TRACE(EXSM_TRACE_MAIN_THR | EXSM_TRACE_CANCEL,
                 "root tcb %p completeOutstandingCancelMsgs out %d", 
                 this, (int) glob->numCancelMsgesOut());

    // work may have finished before a cancel request 
    // was answered by some ESP. 
    glob->getIpcEnvironment()->getAllConnections()->waitOnAll();
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
}

NABoolean ex_root_tcb::externalEventCompleted(void)
{
  return getGlobals()->getScheduler()->externalEventCompleted();
}

Int32 ex_root_tcb::checkTransBeforeExecute(ExTransaction *myTrans, 
					   ExMasterStmtGlobals *masterGlob,
					   ExMasterStats *masterStats,
					   ComDiagsArea *& diagsArea)
{
 

  if (myTrans && myTrans->xnInProgress())
  {
    // If the statement uses DP2 locks to track already-inserted
    // rows to protect against the Halloween problem, check that
    // it is not executed with autocommit off.  External transactions
    // must also not be allowed, since these aren't committed
    // at the end of a SQL statement.  By "external transaction",
    // we mean one that is started without using BEGIN WORK in this
    // master executor.  For example, application calls TMF 
    // BEGINTRANSACTION directly.
      if ((myTrans->autoCommit() == FALSE) ||
          (myTrans->exeStartedXn() == FALSE))
      {

        NABoolean doAutoCommitTest = TRUE;
  #ifdef _DEBUG
      if (getenv("SKIP_AUTOCOMMIT_TEST")) 
        doAutoCommitTest = FALSE;
  #endif

      if (root_tdb().isCheckAutoCommit() &&
          doAutoCommitTest)
        {
         if (diagsArea == NULL)
            diagsArea = ComDiagsArea::allocate(getHeap());
         *diagsArea << DgSqlCode(-EXE_HALLOWEEN_INSERT_AUTOCOMMIT);
         return -1; 
        }

      // If there is an external trans, we cannot be sure whether audit 
      // or locks are held, so we will make the safest assumption.  
      if (myTrans->exeStartedXn() == FALSE)
      {
        myTrans->setMayAlterDb(TRUE);
        myTrans->setMayHoldLock(TRUE);
      }
    }

    if (root_tdb().getMayAlterDb())
      myTrans->setMayAlterDb(TRUE);

    if (root_tdb().getSuspendMayHoldLock())
      myTrans->setMayHoldLock(TRUE);

    if (masterStats)
    {
      // Will configure runtime statistics to disallow or warn
      // query SUSPEND if encompassing transaction might alter 
      // database or hold locks.
      if (myTrans->mayAlterDb()) 
        masterStats->setSuspendMayHaveAuditPinned(TRUE);
    
      if (myTrans->mayHoldLock())
        masterStats->setSuspendMayHoldLock(TRUE);

    }
    
    mayPinAudit_ = myTrans->mayAlterDb() ? true : false;
    mayLock_ = myTrans->mayHoldLock() ? true : false;

    

  }
  return 0;
}

static const char * const NCRenvvar = 
  getenv("SQL_NO_REGISTER_CANCEL");
static const bool NoRegisterCancel = (NCRenvvar && *NCRenvvar == '1');

void ex_root_tcb::registerCB(ComDiagsArea *&diagsArea)
{
  if (NoRegisterCancel)
    return;

  ExMasterStmtGlobals *glob =
    getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals();

  CliGlobals *cliGlobals = glob->getCliGlobals();
  if (cliGlobals->getStatsGlobals() == NULL)
     return;
  ContextCli *context = cliGlobals->currContext();

  // Let MXSSMP know this query is ready to be suspended, since any ESPs will
  // now have their root oper stats in place and we are guaranteed to go into
  // ExScheduler::work.
  ExMasterStats *mStats = NULL;
  StmtStats *sStats = glob->getStatement()->getStmtStats();
  if(sStats)
    mStats = sStats ->getMasterStats();
  if (mStats)
    mStats->setReadyToSuspend();

  Lng32 qidLen = glob->getStatement()->getUniqueStmtIdLen();
  char *qid = glob->getStatement()->getUniqueStmtId();
  
  // No query id -- no cancel or suspend.
  if (qidLen == 0)
    return;

  // No query id -- no cancel or suspend.
  if (NULL == getStatsEntry())
    return;

  // For executionCount, will need either an ExFragRootOperStats 
  // or else an ExMeasStats, otherwise no cancel or suspend.
  Int32 executionCount;
  ExFragRootOperStats *rootOperStats = 
                      getStatsEntry()->castToExFragRootOperStats();
  ExMeasStats *measStats =
                      getStatsEntry()->castToExMeasStats();
  if (rootOperStats)
    executionCount = rootOperStats->getExecutionCount();
  else if (measStats)
    executionCount = measStats->getExecutionCount();
  else
    return;
    
  // See if query allows cancel or suspend.
  if (root_tdb().mayNotCancel())
    return;

  SessionDefaults *sessionDefaults = context->getSessionDefaults();
  if (sessionDefaults)
  {
  
    // Note that it will be required that if a session does not
    // allow queries to be canceled, then it also will not be 
    // possible to suspend the queries.
    // See if session allows cancel.
    if (!sessionDefaults->getCancelQueryAllowed())
      return;
      
    // See if query is "unique" and session allows cancel of unique.
    if (!sessionDefaults->getCancelUniqueQuery())
    {
      Lng32 qt = root_tdb().getQueryType();
      if ((qt == ComTdbRoot::SQL_SELECT_UNIQUE) ||
          (qt == ComTdbRoot::SQL_INSERT_UNIQUE) ||
          (qt == ComTdbRoot::SQL_UPDATE_UNIQUE) ||
          (qt == ComTdbRoot::SQL_DELETE_UNIQUE))
      return;
    }
  }
  Lng32 fromCond = 0;
  if (diagsArea != NULL)
      fromCond = diagsArea->mark();
  ExSsmpManager *ssmpManager = context->getSsmpManager();
  cbServer_ = ssmpManager->getSsmpServer((NAHeap *)getHeap(),
                                 cliGlobals->myNodeName(), 
                                 cliGlobals->myCpu(), diagsArea);
  if (cbServer_ == NULL || cbServer_->getControlConnection() == NULL)		
  {		
      // We could not get a phandle for the cancel broker.  However,		
      // let the query run (on the assumption that it will not need to 		
      // be canceled) and convert any error conditions to warnings.		
      diagsArea->negateErrors(fromCond); 
      return;
  }

  // The stream's actOnSend method will delete (or call decrRefCount()) 
  // for this object.
  RtsQueryId *rtsQueryId = new (context->getIpcHeap()) 
                         RtsQueryId(context->getIpcHeap(), qid, qidLen); 

  RtsHandle rtsHandle = (RtsHandle) this;

  // Make the message
  QueryStarted *queryStarted = 
    new (context->getIpcHeap()) 
      QueryStarted(rtsHandle, context->getIpcHeap(), 
                   JULIANTIMESTAMP(), executionCount);


  //Create the stream on the IpcHeap, since we don't dispose of it immediately.
  //We just add it to the list of completed messages in the IpcEnv, and it is disposed of later.
  //If we create it in the executor heap, that heap gets deallocated when the statement is 
  //finished, and we can corrupt some other statement's heap later on when we deallocate this stream.

  if (queryStartedStream_ == NULL)
  {
    queryStartedStream_  = new (context->getIpcHeap())
          QueryStartedMsgStream(context->getEnvironment(), this, ssmpManager);

    queryStartedStream_->addRecipient(cbServer_->getControlConnection());
  }

  *queryStartedStream_ << *queryStarted;
  queryStarted->decrRefCount();

  *queryStartedStream_<< *rtsQueryId;
  rtsQueryId->decrRefCount();

  // send the no-wait request so cancel broker knows this query has started.
  queryStartedStream_->send(FALSE);  
  
  setCbStartedMessageSent();
}

void ex_root_tcb::deregisterCB()
{
  ExMasterStmtGlobals *glob =
    getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals();

  CliGlobals *cliGlobals = glob->getCliGlobals();
  ContextCli *context = cliGlobals->currContext();

  // Let MXSSMP know this query can no longer be suspended.  Also, here
  // is where we handle one possibilty:  the query can be suspended after
  // the ExScheduler::work method has made its final check of the root
  // oper stats isSuspended_ flag, but before we get to this code where
  // we change ExMasterStats::readyToSuspend_ from READY to NOT_READY.
  // To handle this, the subject master will obtain the Stats semaphore
  // and test ExMasterStats::isSuspended_.  If set to true, it will
  // release the semaphore, wait a few seconds, and obtain the semaphore
  // and test again.  This will repeat until the ExMasterStats::isSuspended_
  // is false.  Then, before releasing the semaphore, the subject master
  // makes the change from READY->NOT_READY.  By waiting until the SSMP has
  // cleared the ExMasterStats::isSuspended_ flag, we can be sure it has
  // also sent messages to the SSCPs to set all the frag root oper stats
  // isSuspended_ flag to false.

  StatsGlobals *statsGlobals = cliGlobals->getStatsGlobals();
  ExMasterStats *mStats = NULL;
  StmtStats *sStats = glob->getStatement()->getStmtStats();
  if(sStats)
    mStats = sStats->getMasterStats();
  if (statsGlobals && mStats && mStats->isReadyToSuspend())
  {
    int error = statsGlobals->getStatsSemaphore(cliGlobals->getSemId(),
                cliGlobals->myPin());

    while (mStats->isQuerySuspended())
    {
      // See comments around allowUnitTestSuspend above.  This code has
      // been manually unit tested, and it too difficult and costly to 
      // test in an automated script.
      statsGlobals->releaseStatsSemaphore(cliGlobals->getSemId(),
               cliGlobals->myPin());
      DELAY(300);

      int error = statsGlobals->getStatsSemaphore(cliGlobals->getSemId(),
                  cliGlobals->myPin());
    }

    // Now we have the semaphore, and the query is not suspended.  Quick
    // set the flag in the masterStats to prevent it from being suspended.
    mStats->setNotReadyToSuspend();

    // Now it is safe to let MXSSMP process another SUSPEND.
    statsGlobals->releaseStatsSemaphore(cliGlobals->getSemId(),
               cliGlobals->myPin());
  }

  // No started message sent, so no finished message should be sent.
  if (!isCbStartedMessageSent())
    return;

  // Holdable cursor is known to deregister more than once.
  if (isCbFinishedMessageSent())
    return;


  // The stream's actOnSend method will delete (or call decrRefCount()) 
  // for this object.
  RtsQueryId *rtsQueryId = new (context->getIpcHeap()) 
                         RtsQueryId( context->getIpcHeap(), 
                                     glob->getStatement()->getUniqueStmtId(),
                                     glob->getStatement()->getUniqueStmtIdLen()
                                   );

  RtsHandle rtsHandle = (RtsHandle) this;

  // Make the message
  QueryFinished *queryFinished = 
    new (context->getIpcHeap()) QueryFinished( rtsHandle, 
                                                  context->getIpcHeap());

  //Create the stream on the IpcHeap, since we don't dispose of it immediately.
  //We just add it to the list of completed messages in the IpcEnv, and it is disposed of later.
  //If we create it in the executor heap, that heap gets deallocated when the statement is 
  //finished, and we can corrupt some other statement's heap later on when we deallocate this stream.

  if (queryFinishedStream_ == NULL)
  {
    queryFinishedStream_  = new (context->getIpcHeap())
          QueryFinishedMsgStream(context->getEnvironment(), this, context->getSsmpManager());

    queryFinishedStream_->addRecipient(cbServer_->getControlConnection());
  }

  *queryFinishedStream_ << *queryFinished;
  queryFinished->decrRefCount();

  *queryFinishedStream_<< *rtsQueryId;
  rtsQueryId->decrRefCount();

  // send the no-wait request to let the cancel broker know this query has started.
  queryFinishedStream_->send(FALSE);  

  setCbFinishedMessageSent();
}

void ex_root_tcb::setCbFinishedMessageReplied()    
{ 
  ExMasterStmtGlobals *glob =
  getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals();

  ContextCli *context = glob->getCliGlobals()->currContext();

  cbCommStatus_ &= ~FINISHED_PENDING_; 
}

void ex_root_tcb::cbMessageWait(Int64 waitStartTime)
{
  ExMasterStmtGlobals *master_glob = getGlobals()->
                  castToExExeStmtGlobals()->castToExMasterStmtGlobals();
  NABoolean cbDidTimedOut = FALSE;
  while (anyCbMessages())
  {
    // Allow the Control Broker 5 minutes to reply.
    // The time already spent waiting (see caller 
    // Statement::releaseTransaction) is counted in this 5 minutes.
    Int64 timeSinceCBMsgSentMicroSecs = waitStartTime ? 
      NA_JulianTimestamp() - waitStartTime : 0 ;
    IpcTimeout timeSinceCBMsgSentCentiSecs = (IpcTimeout)
      (timeSinceCBMsgSentMicroSecs / 10000);
    IpcTimeout cbTimeout = (5*60*100) - timeSinceCBMsgSentCentiSecs;
    if (cbTimeout <= 0)
      cbTimeout = IpcImmediately;
    master_glob->getIpcEnvironment()->getAllConnections()->
      waitOnAll(cbTimeout, FALSE, &cbDidTimedOut);
    if (cbDidTimedOut)
    {
      SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__,
             "Dumping the MXSSMP after IPC timeout.", 0);
      dumpCb();
      ex_assert(!cbDidTimedOut, "Timeout waiting for control broker.");
    }
  }
}
void ex_root_tcb::dumpCb()
{
  ExMasterStmtGlobals *master_glob = getGlobals()->
                    castToExExeStmtGlobals()->castToExMasterStmtGlobals();
  CliGlobals *cliGlobals = master_glob->getCliGlobals();
  StatsGlobals *statsGlobals = cliGlobals->getStatsGlobals();
  if (statsGlobals == NULL)
    return; 
  int error = statsGlobals->getStatsSemaphore(cliGlobals->getSemId(),
          cliGlobals->myPin());
  bool doDump = false;
  Int64 timenow = NA_JulianTimestamp();
  if ((timenow - statsGlobals->getSsmpDumpTimestamp()) > 
       5*60*1000*1000) // 5 minutes
  {
    doDump = true;
    statsGlobals->setSsmpDumpTimestamp(timenow);
  }
  statsGlobals->releaseStatsSemaphore(cliGlobals->getSemId(),
                      cliGlobals->myPin());
  if (doDump)
    cbServer_->getServerId().getPhandle().dumpAndStop(true, false);
}

// -----------------------------------------------------------------------
// Methods for QueryStartedMsgStream. 
// -----------------------------------------------------------------------

void QueryStartedMsgStream::actOnSend(IpcConnection *conn)
{
  if (conn->getErrorInfo() != 0)
     delinkConnection(conn);
}

void QueryStartedMsgStream::actOnSendAllComplete()
{
  clearAllObjects();
  receive(FALSE);
}

void QueryStartedMsgStream::actOnReceive(IpcConnection *connection)
{
  if (connection->getErrorInfo() == 0)
  {
    CollHeap *ipcHeap = connection->getEnvironment()->getHeap();


    QueryStartedReply *reply = new (ipcHeap) 
            QueryStartedReply(INVALID_RTS_HANDLE, ipcHeap);


    *this >> *reply;

    ex_assert( !moreObjects(), "Unknown extra objects follow started reply");

    if(reply->isNextActionComplete())
      ; // normal completion of a query.  This query requested a reply to its
        // own QueryStarted msg.
    else if (rootTcb_)
    {
      ex_assert(reply->isNextActionCancel(), "Unknown reply to Query Started message.");
      rootTcb_->requestCancel();
      if (reply->cancelLogging())
      {
        ExMasterStmtGlobals *glob = rootTcb_->getGlobals()->
            castToExExeStmtGlobals()->castToExMasterStmtGlobals();
        char *qid = glob->getStatement()->getUniqueStmtId();
        char msg[80 + // the constant text
                 ComSqlId::MAX_QUERY_ID_LEN];

        str_sprintf(msg,
               "Master executor of query %s has begun internal SQL cancel.",
                qid);
        SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, msg, 0);
      }
    }
    reply->decrRefCount();
  }
  else 
     delinkConnection(connection);
}

void QueryStartedMsgStream::delinkConnection(IpcConnection *conn)
{
  char nodeName[MAX_SEGMENT_NAME_LEN+1];
  IpcCpuNum cpu;

  conn->getOtherEnd().getNodeName().getNodeNameAsString(nodeName);
  cpu = conn->getOtherEnd().getCpuNum();
  ssmpManager_->removeSsmpServer(nodeName, (short)cpu);
}


void QueryStartedMsgStream::actOnReceiveAllComplete()
{
  if (rootTcb_)
  {
    // clear i/o pending flag for Statement::releaseTransaction
    rootTcb_->setCbStartedMessageReplied();
    // root's pointer to this stream is no longer valid.
    rootTcb_->setQueryStartedMsgStreamIsInvalid();
    // This stream's pointer to root is no longer valid.
    rootTcb_ = NULL;
  }
  addToCompletedList();
}

// -----------------------------------------------------------------------
// Methods for QueryFinishedMsgStream. 
// -----------------------------------------------------------------------

void QueryFinishedMsgStream::actOnSend(IpcConnection *conn)
{
  if (conn->getErrorInfo() != 0)
     delinkConnection(conn);
}

void QueryFinishedMsgStream::actOnSendAllComplete()
{
  clearAllObjects();
  receive(FALSE);
}

void QueryFinishedMsgStream::actOnReceive(IpcConnection *connection)
{
  if (connection->getErrorInfo() == 0)
  {

    CollHeap *ipcHeap = connection->getEnvironment()->getHeap();


    RmsGenericReply *reply = new (ipcHeap) 
              RmsGenericReply(ipcHeap);


    *this >> *reply;

    ex_assert( !moreObjects(), "Unknown extra objects follow started reply");

    reply->decrRefCount();
  }
  else
      delinkConnection(connection);
}

void QueryFinishedMsgStream::delinkConnection(IpcConnection *conn)
{
  char nodeName[MAX_SEGMENT_NAME_LEN+1];
  IpcCpuNum cpu;

  conn->getOtherEnd().getNodeName().getNodeNameAsString(nodeName);
  cpu = conn->getOtherEnd().getCpuNum();
  ssmpManager_->removeSsmpServer(nodeName, (short)cpu);
}

void QueryFinishedMsgStream::actOnReceiveAllComplete()
{
  if (rootTcb_)
  {
    // clear i/o pending flag for Statement::releaseTransaction
    rootTcb_->setCbFinishedMessageReplied();
    // root's pointer to this stream is no longer valid.
    rootTcb_->setQueryFinishMsgStreamIsInvalid();
    // This stream's pointer to root is no longer valid.
    rootTcb_ = NULL;
  }

  addToCompletedList();
}

void ex_root_tcb::cpuLimitExceeded()
{
  cpuLimitExceeded_ = TRUE; 
  requestCancel();
}
