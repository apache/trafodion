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
 * File:         ex_split_bottom.C
 * Description:  Split bottom tdb and tcb (for parallel execution)
 *
 * Created:      12/11/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include  "ex_stdh.h"
#include  "ex_exe_stmt_globals.h"
#include  "ComTdb.h"
#include  "ex_tcb.h"
#include  "ex_split_bottom.h"
#include  "ex_send_bottom.h"
#include  "ex_send_top.h"
#include  "ex_esp_frag_dir.h"
#include  "ex_expr.h"
#include  "ExStats.h"
#include  "str.h"
#include  "exp_clause_derived.h"
#include "ExSMGlobals.h"
#include "ExpLOBaccess.h"

// -----------------------------------------------------------------------
// Methods for class ex_split_bottom_tdb
// -----------------------------------------------------------------------

ex_tcb * ex_split_bottom_tdb::build(ex_globals *)
{
  ex_assert(FALSE,"Don't use ex_split_bottom_tdb::build()");
  return NULL;
}

ex_split_bottom_tcb * ex_split_bottom_tdb::buildESPTcbTree(
     ExExeStmtGlobals * glob,
     ExEspFragInstanceDir *espInstanceDir,
     const ExFragKey &myKey,
     const ExFragKey &parentKey,
     ExFragInstanceHandle myHandle,
     Lng32 numOfParentInstances)
{
  ex_split_bottom_tcb *result;


  // set this plan version in the statement globals.
  glob->setPlanVersion(planVersion_);

  if (getQueryUsesSM() && espInstanceDir->getEnvironment()->smEnabled())
  {
    // Initialize SeaMonster. Return early if errors where encountered.
    glob->initSMGlobals();
    ComDiagsArea *diags = glob->getDiagsArea();
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

  // build the children
  ex_tcb *tcbChild = child_->build(glob);

  // create my own tcb
  result = new(glob->getSpace()) ex_split_bottom_tcb(*this,
						     tcbChild,
						     glob,
						     espInstanceDir,
						     myKey,
						     parentKey,
						     myHandle,
						     numOfParentInstances);

  // add the new tcb to the schedule
  result->registerSubtasks();

  glob->castToExEspStmtGlobals()->setIsAnESPAccess(isAnESPAccess());

  // If detailed statistics are to be collected, allocate the
  // ExStatisticsArea.
  if (getCollectStats()) 
  {
    glob->setStatsEnabled(TRUE);
#ifdef _DEBUG
    if (getenv("DISABLE_STATS"))
       glob->setStatsEnabled(FALSE);
#endif

    ExStatisticsArea* statsArea;
    StatsGlobals *statsGlobals = espInstanceDir->getStatsGlobals();
    if (statsGlobals == NULL || getCollectStatsType() == ALL_STATS || 
      glob->castToExEspStmtGlobals()->getQueryId() == NULL)
    {
      statsArea = new(espInstanceDir->getLocalStatsHeap())
        ExStatisticsArea(espInstanceDir->getLocalStatsHeap(), 
              numOfParentInstances, getCollectStatsType());
      statsArea->setStatsEnabled(glob->statsEnabled());
      glob->setStatsArea(statsArea);
      result->allocateStatsEntry();
    }
    else
    {
      int error = statsGlobals->getStatsSemaphore(espInstanceDir->getSemId(),
                      espInstanceDir->getPid());
      statsArea = new(espInstanceDir->getStatsHeap())
        ExStatisticsArea(espInstanceDir->getStatsHeap(), numOfParentInstances,
		       getCollectStatsType());
      statsArea->setStatsEnabled(glob->statsEnabled());
      StmtStats *ss = glob->castToExEspStmtGlobals()->setStmtStats();
      if ( ss != NULL)
        ss->setStatsArea(statsArea);

      statsArea->setRtsStatsCollectEnabled(getCollectRtsStats());
      glob->setStatsArea(statsArea);
      result->allocateStatsEntry();
      statsGlobals->releaseStatsSemaphore(espInstanceDir->getSemId(), espInstanceDir->getPid());
    }
  }
  else
    glob->setStatsEnabled(FALSE);
         

  if (cpuLimit_ > 0)
  {
    glob->getScheduler()->setMaxCpuTime(cpuLimit_);
    glob->getScheduler()->setCpuLimitCheckFreq(cpuLimitCheckFreq_);
  }

  if (processLOB())
    {
      glob->initLOBglobal(glob->getCliGlobals()->currContext(), useLibHdfs());
    }

  return result;
}


// -----------------------------------------------------------------------
// Methods for class ex_split_bottom_tcb
// -----------------------------------------------------------------------

ex_split_bottom_tcb::ex_split_bottom_tcb (
     const ex_split_bottom_tdb & splitBottomTdb,
     const ex_tcb * child_tcb,
     ExExeStmtGlobals * glob,
     ExEspFragInstanceDir *espInstanceDir,
     const ExFragKey &myKey,
     const ExFragKey &parentKey,
     ExFragInstanceHandle myHandle,
     Lng32 numOfParentInstances) : 
     ex_tcb(splitBottomTdb,1,glob),
     sendNodes_(glob->getSpace()),
     sendNodesUpQ_(glob->getSpace()),
     sendNodesDownQ_(glob->getSpace()),
     workAtp_(NULL),
     numCanceledPartitions_(0),
     bugCatcher3041_(UINT_MAX - 2)
{
  // build the child tcb
  tcbChild_                = child_tcb;
  qChild_                  = child_tcb->getParentQueue();
  espInstanceDir_          = espInstanceDir;
  myHandle_                = myHandle;
  numOfParentInstances_    = numOfParentInstances;
  ioHandler_               = NULL;
  currRequestor_           = 0;
  glob_                    = glob->castToExEspStmtGlobals();
  staticInputData_         = FALSE;
  workMessageSaved_        = FALSE;
  inputDataMessageSaved_   = FALSE;
  releaseMessageSaved_     = FALSE;
  haveTransaction_         = FALSE;
  calcPartNumIsValid_      = FALSE;

  ex_assert(glob_,"Using split bottom node outside of an ESP");

  // we allocate the ATPs for the down queue to the child, since
  // we want to make new ATPs for each input that we send to the
  // child and add the partition input values
  qChild_.down->allocateAtps(glob->getSpace());

  // for table M-Way repartition, recalculate number of parent instances
  if (splitBottomTdb.isMWayRepartition())
    {
      numOfParentInstances_ = numOfTargetESPs(splitBottomTdb.topNumESPs_,
                                              splitBottomTdb.bottomNumESPs_,
                                              glob_->getMyInstanceNumber());
      firstParentNum_ = myFirstTargetESP(splitBottomTdb.topNumESPs_,
                                         splitBottomTdb.bottomNumESPs_,
                                         glob_->getMyInstanceNumber());
    }
  else
    {
      firstParentNum_ = 0;
    }


  // build the send nodes, one for each parent fragment instance
  for (Lng32 i = 0; i < numOfParentInstances_; i++)
    {
      ex_send_bottom_tcb *sendTcb =
	splitBottomTdb.sendTdb_->buildInstance(glob,
					       espInstanceDir,
                                               myKey,
                                               parentKey,
					       myHandle,
					       firstParentNum_ + i,
					       FALSE);
      sendNodes_.insertAt(i, sendTcb);
      sendNodesUpQ_.insertAt(i, sendTcb->getParentQueueForSendBottom().up);
      sendNodesDownQ_.insertAt(i, sendTcb->getParentQueueForSendBottom().down);
    }

  // If this is a parallel extract operation then we need one
  // additional send node to connect to the extract consumer query
  if (splitBottomTdb.getExtractProducerFlag())
    {
      Lng32 i = numOfParentInstances_;
      ex_send_bottom_tcb *sendTcb =
        splitBottomTdb.sendTdb_->buildInstance(glob,
                                               espInstanceDir,
                                               myKey,
                                               parentKey,
                                               myHandle,
                                               i,
                                               FALSE);
      sendTcb->setExtractConsumerFlag(TRUE);
      sendNodes_.insertAt(i, sendTcb);
      sendNodesUpQ_.insertAt(i, sendTcb->getParentQueueForSendBottom().up);
      sendNodesDownQ_.insertAt(i, sendTcb->getParentQueueForSendBottom().down);
    }

  // initialize work atp
  workAtp_ = allocateAtp(splitBottomTdb.workCriDesc_, glob->getSpace());
  partNumTupp_.init(sizeof(partNumInfo_),
		    NULL,
		    (char *) (&partNumInfo_));
  workAtp_->getTupp(splitBottomTdb.partNoATPIndex_) = &partNumTupp_;
  convErrTupp_.init(sizeof(conversionErrorFlg_),
		    NULL,
		    (char *) (&conversionErrorFlg_));
  workAtp_->getTupp(splitBottomTdb.convErrorATPIndex_) = &convErrTupp_;
  partInputDataTupp_.init(
       (unsigned short) splitBottomTdb.partInputDataLen_,
       NULL,
       new(glob->getSpace()) char[splitBottomTdb.partInputDataLen_]);
  workAtp_->getTupp(splitBottomTdb.partInputATPIndex_) = &partInputDataTupp_;

  // fixup expression
  if (partFunction())
    {
      unsigned short expMode = getExpressionMode();
      if (splitBottomTdb.partFuncUsesNarrow_)
	expMode = 0; // don't use PCODE with Narrow
      partFunction()->fixup(0, expMode, this,
			    glob->getSpace(), glob->getDefaultHeap(), FALSE, glob);
    }

  // allocate message stream to receive partition input data
  newMessage_ = new(glob->getSpace()) SplitBottomRequestMessage(
       glob->getIpcEnvironment(),
       this);
  savedDataMessage_ = new(glob->getSpace()) SplitBottomSavedMessage(
       glob->getIpcEnvironment());
  savedWorkMessage_ = new(glob->getSpace()) SplitBottomSavedMessage(
       glob->getIpcEnvironment());
  savedReleaseMessage_ = new(glob->getSpace()) SplitBottomSavedMessage(
       glob->getIpcEnvironment());

  // initialize state
  if (splitBottomTdb.partInputDataLen_)
    {
      setWorkState(WAIT_FOR_PARTITION_INPUT_DATA);
    }
  else
    {
      setWorkState(IDLE); // other data members are undefined for state IDLE
      staticInputData_ = TRUE;
    }

  glob->getScheduler()->setRootTcb(this);

  const SplitBottomSkewInfoPtr sbSkewInfoP = splitBottomTdb.skewInfo_;
  if (sbSkewInfoP)
    {
      // Allocate the collision chain skewLinks_.
      skewLinks_ = new(glob->getSpace())
        Int32[  sbSkewInfoP->getNumSkewHashValues() ];

      // Initialize all collision links to noLink_ (-1 or 0xffffffff),
      // to indicate the end of the chain.
      memset((char *) skewLinks_, 0xff,
             sizeof(Int32) * sbSkewInfoP->getNumSkewHashValues());

      // Initialize all indexes in skewHdrs_ to noLink_ (-1 or 0xffffffff),
      // to indicate that there is no skew key yet on the
      // collision chain.
      memset((char *) skewHdrs_, 0xff, sizeof(skewHdrs_));

      // Populate the skewHdrs_ and skewLinks_ with the skew keys.
      for (Int32 skewArrayIdx=0;
           skewArrayIdx < sbSkewInfoP->getNumSkewHashValues();
           skewArrayIdx++)
      {
        Int64 sv = sbSkewInfoP->getSkewHashValues()[skewArrayIdx];
        Int32 slot = (Int32) (sv % numSkewHdrs_);
        if (skewHdrs_[slot] == noLink_)
        {
          skewHdrs_[slot] = skewArrayIdx;
        }
        else
        {
          skewLinks_[skewArrayIdx] = skewHdrs_[slot];
          skewHdrs_[slot] = skewArrayIdx;
        }
      }
    }
  else
    skewLinks_ = NULL;

  uniformDistributionType_ = RoundRobin_;
  localSendBotttom_ = NULL_COLL_INDEX;
  roundRobinRecipient_ = splitBottomTdb.getInitialRoundRobin();   

  broadcastOneRow_ = splitBottomTdb.getBroadcastOneRow();
}
        
ex_split_bottom_tcb::~ex_split_bottom_tcb()
{
  freeResources();
}

void ex_split_bottom_tcb::freeResources()
{
  // we allocated ATPs for the down queue for the child, which is not
  // the normal way
  if (qChild_.down)
    qChild_.down->deallocateAtps();

//  These tcbs are now deleted by glob->deleteMe().
//  for (CollIndex i = 0; i < sendNodes_.entries(); i++)
//    delete sendNodes_[i];

  if (workAtp_)
    {
    deallocateAtp(workAtp_, getSpace());
    workAtp_ = NULL;
    }
  // delete [] partInputDataTupp_.getTupleAddress();

  // message streams are allocated from the Space
  if (newMessage_)
    {
      newMessage_->~SplitBottomRequestMessage();
      newMessage_ = NULL;
    }
  if (savedDataMessage_)
    {
      savedDataMessage_->~SplitBottomSavedMessage();
      savedDataMessage_ = NULL;
    }
  if (savedWorkMessage_)
    {
      savedWorkMessage_->~SplitBottomSavedMessage();
      savedWorkMessage_ = NULL;
    }
  if (savedReleaseMessage_)
    {
      savedReleaseMessage_->~SplitBottomSavedMessage();
      savedReleaseMessage_ = NULL;
    }
  if (skewLinks_ )
  {
    NADELETEBASIC(skewLinks_, getSpace());
    skewLinks_ = NULL;
  }
}
  
void ex_split_bottom_tcb::registerSubtasks()
{
  ExScheduler *sched = glob_->getScheduler();

  // normal mode for the tcb child
  sched->registerUnblockSubtask(sWork,this, qChild_.down);
  sched->registerInsertSubtask(sWork, this, qChild_.up);

  // for each send node we are sitting on the queue's bottom end,
  // receiving down requests and sending data back up
  for (Int32 i = 0; i < (Int32)sendNodes_.entries(); i++)
    {
      sched->registerInsertSubtask(sWork, this, sendNodesDownQ_[i]);
      sched->registerCancelSubtask(sCancel, this, sendNodesDownQ_[i], "CN");
      sched->registerUnblockSubtask(sWork,this, sendNodesUpQ_[i]);
    }

  // finally, make a separate event through which the IPC message
  // with partition input values and/or transaction can wake us up
  ioHandler_ = sched->registerNonQueueSubtask(sWork, this);
}

NABoolean ex_split_bottom_tcb::needStatsEntry()
{
  return TRUE;
}

ExOperStats * ex_split_bottom_tcb::doAllocateStatsEntry(CollHeap *heap,
							ComTdb *tdb)
{
  ExOperStats * stat = NULL;
  ComTdb::CollectStatsType statsType = getGlobals()->getStatsArea()->getCollectStatsType();
  if ((statsType == ComTdb::ALL_STATS) ||
      (statsType == ComTdb::OPERATOR_STATS) ||
      (statsType == ComTdb::PERTABLE_STATS))
    {
      stat = new(getGlobals()->getStatsArea()->getHeap())
                ExFragRootOperStats(getGlobals()->getStatsArea()->getHeap(),
					   this,
					   tdb);
      stat->setStatsInEsp(TRUE);
      getGlobals()->getStatsArea()->setStatsInEsp(TRUE);
      StmtStats *ss = getGlobals()->castToExExeStmtGlobals()->castToExEspStmtGlobals()->getStmtStats();
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
      // TDB or TCB.
      stat = new(getGlobals()->getStatsArea()->getHeap()) 
                  ExMeasStats(getGlobals()->getStatsArea()->getHeap(), 
				   NULL,
				   tdb); 
      stat->setStatsInEsp(TRUE);
      getGlobals()->getStatsArea()->setStatsInEsp(TRUE);
      StmtStats *ss = getGlobals()->castToExExeStmtGlobals()->castToExEspStmtGlobals()->getStmtStats();
      if (ss != NULL)
        ((ExMeasStats *)stat)->setQueryId(ss->getQueryId(), ss->getQueryIdLen());
    }
    if (stat)
    {
      // Assumue parent Tdb is the one above this Tdb
      stat->setParentTdbId(tdb->getTdbId()+1);
      stat->setLeftChildTdbId(getChild(0)->getTdb()->getTdbId());
      getGlobals()->getStatsArea()->setRootStats(stat);
    }
    return stat;
}

ExWorkProcRetcode ex_split_bottom_tcb::processInactiveSendTopsBelow(
                                                          NABoolean &foundOne)
{
  ExWorkProcRetcode retcode = WORK_OK;
  foundOne = FALSE;

  // Check whether there are any send top nodes below us that haven't
  // been exercised by this request. If that happened (because of an
  // early cancel operation, for example), there is a possibility that
  // the corresponding producer ESP might wait forever. Avoid that by
  // sending it a message saying that we lost interest.  In other
  // words, we guarantee that for every request the split bottom works
  // on, a corresponding request (real one or cancel) goes out to all
  // producer ESPs.

  CollIndex orphanIx = 0;
  ex_send_top_tcb *orphan = glob_->getFirstNonActivatedSendTop(orphanIx);
  
  while (orphan)
    {
      // make the send top TCB send the message
      if (orphan->notifyProducerThatWeCanceled() == 0)
        foundOne = TRUE;
      else
        {
        // The send top is telling us that there was an error in 
        // using its IpcConnection.  Propagate this information 
        // back toward the master.
        retcode = WORK_BAD_ERROR;
        break;
        }

      orphanIx++;
      orphan = glob_->getNextNonActivatedSendTop(orphanIx);
    }

  return retcode;
}
  
ex_queue_pair ex_split_bottom_tcb::getParentQueue() const
{
  // there is no parent queue for this node, return none
  ex_queue_pair temp;
  temp.down = NULL;
  temp.up   = NULL;
  return temp; 
}

void ex_split_bottom_tcb::releaseWorkRequest()
{
    if (releaseMessageSaved_)
    {
      // reply to the previously received release work work msg
      releaseMessageSaved_ = FALSE;
      replyToMessage(savedReleaseMessage_);
    }
  if (workMessageSaved_)
    {
      // the reply tag loses its meaning wrt. its transid
      glob_->setReplyTag(ExInvalidInt64Transid,GuaInvalidReplyTag);

      // reply to the previously received transaction work request
      workMessageSaved_ = FALSE;
      replyToMessage(savedWorkMessage_);
    }
}

ExWorkProcRetcode ex_split_bottom_tcb::work()
{
  // shortcut for the most common case
  if (workState_ == WORK_ON_REQUEST)
    // move some data up
    return workUp();

  // -----------------------------------------------------------------
  // next, try the waiting states
  // -----------------------------------------------------------------
  if (workState_ == WAIT_FOR_PARTITION_INPUT_DATA)
    {
      // this state gets changed when a new part input data request arrives
      return WORK_OK;
    }

  if (workState_ == WAIT_TO_REPLY)
    {
      if (sendNodesUpQ_[fullUpQueue_]->isFull())
	// no change, give up
	return WORK_OK;
      
      // the up queue became empty, we can give it another try to send
      // data up to all send nodes that need it
      setWorkState(WORK_ON_REQUEST);
    }
  
  if (workState_ == WAIT_FOR_MORE_REQUESTORS)
    {
      if (sendNodesDownQ_[emptyDownQueue_]->isEmpty())
	// still missing a request from one of the send/receive nodes
	// give up
	return WORK_OK;
      else
	// we got the request we were waiting for, now give it another try
	setWorkState(WORK_ON_REQUEST);
    }
  
  if (workState_ == IDLE)
  {
    // -------------------------------------------------------------
    // If work state is IDLE, wait for a new request to come in
    // -------------------------------------------------------------
    CollIndex numSendNodes = sendNodes_.entries();
    CollIndex i = 0;
    CollIndex numCanceledRequests = 0;

    // try to find a new request and send it down
    // NOTE: the split bottom node never sends down more than one
    // request per send node (for now)
    while (workState_ == IDLE AND i < numSendNodes)
    {
      NABoolean isExtractConsumer = FALSE;
      if (splitBottomTdb().getExtractProducerFlag() &&
          i == (numSendNodes - 1))
        isExtractConsumer = TRUE;

      ex_queue* squeueDown = sendNodesDownQ_[i];

      if (NOT squeueDown->isEmpty())
      {
        ex_queue_entry *sentry = squeueDown->getHeadEntry();
        
        if (isExtractConsumer && numCanceledRequests > 0)
        {
          // If this send node is for a parallel extract consumer
          // and we have already seen at least one cancel then we
          // need to make sure a cancel is issued for this send
          // node.
          if (sentry->downState.request != ex_queue::GET_NOMORE)
            squeueDown->cancelRequest();
        }

        // check if it is canceled.
        if (sentry->downState.request == ex_queue::GET_NOMORE)
        {
          // note that the send bottoms may send us a GET_NOMORE
          // request that does not have any tupps associated with it,
          // so make sure we don't look at the tupps of a cancel

          numCanceledRequests++;
          bool doWorkCancelBeforeSent = false;
          if (!splitBottomTdb().combineRequests_ ||
              numCanceledRequests >= numSendNodes)
            doWorkCancelBeforeSent = true;
          else if (glob_->noNewRequest())
          {
            // see 10-090326-0318 comments below
            doWorkCancelBeforeSent = true;
            bugCatcher3041_ = squeueDown->getHeadIndex();
          }

          if (doWorkCancelBeforeSent) 
          {
            // we've got nothing but GET_NOMOREs, send
            // EODs back without forwarding a child request
            //
            // - see solution 10-090326-0318 for more details
            // If noNewRequest flag is set, it means master process has issued
            // release work msg to all esps. In that case it's possible that
            // some of the parent esps may not send cancel msg to this esp.
            // A parent esp will send the cancel msg to me only if it had
            // received cancel request from its parent esp before it received
            // release work msg from master. So this esp should reply right
            // away to any cancel msg it receives instead of waiting for
            // cancel msg from all of its parent esps.
            //
            // Fix for sol 10-110209-6041
            // During a MWay Repartition case the requestor (send node)
            // does not have to start with 0 but the first send node
            // will be fistParentNum_ and so need to consider firstParentNum_
            // when setting currRequestor_
            currRequestor_ = i + firstParentNum_;
            setWorkState(WORK_ON_REQUEST);
            glob_->clearAllActivatedSendTopTcbs();
            return workCancelBeforeSent();
          }

          // some, but not all requestors have sent a
          // GET_NOMORE, just wait and see
          
        } // if (sentry->downState.request == ex_queue::GET_NOMORE)
        
        else
        {
          // We have a request to push down but do not push anything
          // down under these conditions:
          // 
          // a. The child queue is full
          // 
          // b. This is a parallel extract consumer. Requests from
          //    extract consumers do not get sent below this split
          //    bottom.
          //
          // c. Do not work on queues before the work message has
          //    arrived. This prevents a problem where PA work methods
          //    are called before the transaction has arrived. When PA
          //    work methods run without a transaction, the SqlTable
          //    constructor calls the arkfs createSession() function
          //    which returns error 75 (no active transaction). This
          //    change was made by the SeaMonster project in March
          //    2013.

          bool okToPush = true;
          if (tcbChild_->getParentQueue().down->isFull() ||
              isExtractConsumer ||
              !hasTransaction())
            okToPush = false;

          if (okToPush)
          {
            // copy queue entry down
            ex_queue_entry *centry = qChild_.down->getTailEntry();
            
            centry->downState = sentry->downState;
            // no need to change parent index, we only use one entry
            // and since we deal with multiple parent indexes anyway
            
            // copy the atp into the already allocated atp of the child's
            // input queue
            centry->copyAtp(sentry);
            
            // add the partition input tupp to the down queue entry
            centry->getAtp()->copyPartialAtp(
                     // copy a tupp from the work atp
                     workAtp_,
                     // to position 3 (after const, temps, input, part input)
                     3, // $$$$ make this a const, too
                     // from the position where the part input data is stored
                     (short) splitBottomTdb().partInputATPIndex_,
                     // for one tupp (last is inclusive)
                     (short) splitBottomTdb().partInputATPIndex_);
              
            qChild_.down->insert();
            
            currRequestor_ = i;
            
            setWorkState(WORK_ON_REQUEST);
            glob_->clearAllActivatedSendTopTcbs();
            glob_->resetMemoryQuota();
            glob_->incExecutionCount();

          } // if (okToPush)
        } // if (GET_NOMORE) else ...
      } // if (NOT squeueDown->isEmpty())
      
      i++;
      
    } // while (workState_ == IDLE AND i < numSendNodes)
    
    // still idle?
    if (workState_ == IDLE)
      return WORK_OK;
      
    // inject events to test log4cxx
    if (splitBottomTdb().getAbendType() == ComTdbSplitBottom::TEST_LOG4CXX)
    {
        Int32 LEN = 100;
        char espInfo[LEN];
        snprintf(espInfo, LEN, "Test ESP Event on ESP with instance number (%d)", glob_->getMyInstanceNumber());
        SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, espInfo, 0);
    }

    // Following code is test for soln 10-081104-7061.  A CQD
    // COMP_INT_39 can be used to force various kinds of abends
    // in the ESP.  We would like to limit the # of ESPs 
    // abending to only one, so we assume that there will always
    // be a fragId of 2 and instance # of 0 in any ESP plan.
    if (splitBottomTdb().getAbendType() != ComTdbSplitBottom::NO_ABEND &&
        glob_->getMyFragId() == 2 &&
        glob_->getMyInstanceNumber() == 0)
    {
      switch (splitBottomTdb().getAbendType())
        {
        case ComTdbSplitBottom::SIGNED_OVERFLOW:
          {
        // Don't expect this to work. b/c this code is compiled
        // with overlow trapping set to off.
            signed short willOverflow = 0x7ffc;
            ULong loopCnt = (ULong) this;
            while (loopCnt--)
              willOverflow++;
            break;
          }
        case ComTdbSplitBottom::ASSERT:
          {
            ex_assert( splitBottomTdb().getAbendType() != 
                        ComTdbSplitBottom::ASSERT, 
                      "Abend/assertTest passed.");
            break;

          }
        case ComTdbSplitBottom::INVALID_MEMORY:
          {
            return *(Int32 *) WORK_OK;
          }
        case ComTdbSplitBottom::SLEEP180:
          {
	    timespec nanoDelayTime;
	    nanoDelayTime.tv_sec = 180;
	    nanoDelayTime.tv_nsec = 0;
	    nanosleep(&nanoDelayTime, NULL);
            break;
          }
        case ComTdbSplitBottom::INTERNAL_ERROR:
          {
            // Test handling of internal error.  On Seaquest, we 
            // should make a core file, but survive and return
            // the error to the user.  Also, a Seapilot event
            // should be logged.  On other platforms we skip the 
            // the core-file and Seapilot event.  The Seaquest
            // logic is encapsulated in ComDiags.cpp and ComRtUtils.cpp.
            // I tested this code with regress/executor/TEST082.
            // However, that test is disabled.
            for (CollIndex i = 0; i < sendNodes_.entries(); i++)
            {
              ex_queue *squeueDown = sendNodesDownQ_[i];
              if (!squeueDown->isEmpty())
              {
                squeueDown->cancelRequest();
                ex_queue *squeueUp = sendNodesUpQ_[i];
                if (!squeueUp->isFull())
                {
                  ex_queue_entry *upEntry = squeueUp->getTailEntry();
                  upEntry->upState.downIndex = squeueDown->getHeadIndex();
	          upEntry->upState.parentIndex =
	            squeueDown->getHeadEntry()->downState.parentIndex;
                  upEntry->upState.status = ex_queue::Q_SQLERROR;
                  upEntry->upState.setMatchNo(0);
                  ComDiagsArea *da = ComDiagsArea::allocate(
                                       getGlobals()->getDefaultHeap());
                  *da << DgSqlCode(-CLI_TCB_EXECUTE_ERROR);
                  upEntry->setDiagsArea(da);
                  squeueUp->insert();

                  qChild_.down->cancelRequest();
                  setWorkState(CLEANUP);
                  break;
                }
              }
            }
            break;
          }
        case ComTdbSplitBottom::TEST_LOG4CXX:
          {
            // nothing done here - should have been handled above
            break;
          }
        default:
          {
            ex_assert( 0, "invalid value for getAbendType (COMP_INT_39).");
            break;
          }
        }
    }
  } // if (workState_ == IDLE)

  else if (workState_ == WAIT_FOR_LATE_CANCELS)
  {
    // wait until late cancel messages have been processed
    if (espInstanceDir_->numLateCancelRequests(myHandle_) <= 0)
    {
      setWorkState(IDLE);
      return WORK_CALL_AGAIN; // maybe there are new requests in the queue
    }
    else
    {
      // wait for the cancel messages to come back, but they will wake
      // up the send tops and not this TCB, so return WORK_POOL_BLOCKED
      return WORK_POOL_BLOCKED;
    }      
  }

  if (workState_ == LIMITS_EXCEEDED_ERROR)
  {
    // Find an active send bottom with room in the up queue.
    for (CollIndex i = 0; i < sendNodes_.entries(); i++)
    {
      ex_queue *squeueDown = sendNodesDownQ_[i];
      if (!squeueDown->isEmpty())
      {
        squeueDown->cancelRequest();
        ex_queue *squeueUp = sendNodesUpQ_[i];
        if (!squeueUp->isFull())
        {
          ex_queue_entry *upEntry = squeueUp->getTailEntry();
          upEntry->upState.downIndex = squeueDown->getHeadIndex();
	  upEntry->upState.parentIndex =
	    squeueDown->getHeadEntry()->downState.parentIndex;
          upEntry->upState.status = ex_queue::Q_SQLERROR;
          upEntry->upState.setMatchNo(0);
          ComDiagsArea *da = ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
          *da << DgSqlCode(-EXE_QUERY_LIMITS_CPU);
          *da << DgInt0((Lng32) splitBottomTdb().cpuLimit_);
          *da << DgInt1((Int32) getGlobals()->getMyFragId());

          if (splitBottomTdb().getQueryLimitDebug())
          {
            *da << DgSqlCode(EXE_QUERY_LIMITS_CPU_DEBUG);
            *da << DgInt0((Lng32)splitBottomTdb().cpuLimit_);
            *da << DgInt1((Int32) getGlobals()->getMyFragId());

            Int64 localCpuTime = 0;
            ExFragRootOperStats *fragRootStats;
            ExMeasStats *measRootStats;
            ExOperStats *rootStats = glob_->getStatsArea()->getRootStats();

            if ((fragRootStats = rootStats->castToExFragRootOperStats()) != NULL)
              localCpuTime = fragRootStats->getLocalCpuTime();
            else if ((measRootStats = rootStats->castToExMeasStats()) != NULL)
              localCpuTime = measRootStats->getLocalCpuTime();
            else 
              ex_assert(0, "Cpu limit evaluated without runtime stats.");

            *da << DgInt2((Lng32) localCpuTime / 1000);

          }

          upEntry->setDiagsArea(da);
          squeueUp->insert();

          qChild_.down->cancelRequest();
          setWorkState(CLEANUP);
          break;
        }
      }
    }
    if (workState_ != CLEANUP)
    {
      // No ready send bottom has room, so come back later.
      return WORK_OK;
    }
  }  // end if LIMITS_EXCEEDED_ERROR

  if (workState_ == CLEANUP)
  {
    for (;;)
    {
      if (qChild_.up->isEmpty())
        return WORK_OK;
      if (qChild_.up->getHeadEntry()->upState.status == ex_queue::Q_NO_DATA)
      {
        // Let workUp deal with this.
        setWorkState(WORK_ON_REQUEST);
        break;
      }
      else
        qChild_.up->removeHead();
    }
  }
  // now we should be working on a request
  ex_assert(workState_ == WORK_ON_REQUEST,"Internal err, splb work state");

  // move some data up
  return workUp();
}

ExWorkProcRetcode ex_split_bottom_tcb::workUp()
{
  // If this is a parallel extract operation and we have already seen
  // at least one cancel then we need to make sure all send queues are
  // notified of the cancel.
  if (splitBottomTdb().getExtractProducerFlag() && numCanceledPartitions_ > 0)
  {
    CollIndex numSendNodes = sendNodes_.entries();
    for (CollIndex i = 0; i < numSendNodes; i++)
    {
      ex_queue* squeueDown = sendNodesDownQ_[i];
      if (!squeueDown->isEmpty())
      {
        if (squeueDown->getHeadEntry()->downState.request !=
            ex_queue::GET_NOMORE)
          squeueDown->cancelRequest();
      }
    }
  }

  ExOperStats *statsEntry = getStatsEntry();
  // -----------------------------------------------------------------
  // Try to pass result entries back to send/receive node(s)
  // -----------------------------------------------------------------
  while (NOT qChild_.up->isEmpty())  // may exit via return
    {

      ex_queue_entry *centry = qChild_.up->getHeadEntry();
      NABoolean isEofEntry = centry->upState.status == ex_queue::Q_NO_DATA;
      NABoolean isErrorEntry = (centry->getDiagsArea() AND
				centry->getDiagsArea()->mainSQLCODE() < 0);
      CollIndex partNumLow;
      CollIndex partNumHigh;

      // ---------------------------------------------------------
      // calculate the output partition number(s)
      // ---------------------------------------------------------
      if (partFunction() AND NOT isEofEntry)
	{
	  if (isErrorEntry)
	    {
	      // Entries other than Q_OK_MMORE (should be a Q_SQLERROR)
	      // are simply sent to one of the requestors to be
	      //
	      partNumInfo_.calculatedPartNum_ = 0;
	      calcPartNumIsValid_ = TRUE;
	    }
	  
	  // evaluate the partitioning function only if we haven't
	  // come here before for the same row
	  if (!calcPartNumIsValid_)
	    {
              broadcastThisRow_ = FALSE;

	      conversionErrorFlg_ = ex_conv_clause::CONV_RESULT_OK;
	      
	      // evaluate the actual expression that evaluates an integer
	      // partition number and stores it in 
              // partNumInfo_.calculatedPartNum_.  The hash value of the
              // partitioning key is also stored in partNumInfo_.partHash_
              // in the case of skew buster.
	      if (partFunction()->
		  eval(centry->getAtp(),workAtp_) == ex_expr::EXPR_ERROR)
		{
		  // this should not usually happen, but if it does,
		  // for example for a complex partitioning expression,
		  // then propagate the error to one of the target ESPs
		  isErrorEntry = TRUE;
		  partNumInfo_.calculatedPartNum_ = 0;
		}
	      
	      if (partFuncUsesNarrow() &&
		  conversionErrorFlg_ != ex_conv_clause::CONV_RESULT_OK)
		{
		  // A data conversion error occurred while trying
		  // to compute the partitioning function. This
		  // means that the result of the partitioning
		  // function is either rounded or invalid.  Such
		  // conversion errors usually mean that the
		  // result row won't find a match on the other
		  // side of a join, for example, but since we
		  // don't want to make such speculations at this
		  // point, we just set the partition number to a
		  // fixed value in case the expression produced
		  // an invalid result.  NOTE: several
		  // optimizations are possible if this is a
		  // problem in the future: make expressions that
		  // can differentiate between rounding and
		  // invalid results (needs if-then-else stmts for
		  // multiple narrow operators) or just throw the
		  // row away if we reach here.
		  partNumInfo_.calculatedPartNum_ = 0;
		}

              if (splitBottomTdb().useSkewBuster() )
                {
                  ex_assert (splitBottomTdb().combineRequests_, 
                             "Skew buster used without combining requests.");
                  // Apply the skew-buster.  The partNumInfo_.partHash_ 
                  // has been side-effected by partFunc->eval.  See if this
                  // partitioning key's hash key is in our hash table.

                  // Start off assuming that partNumInfo_.partHash_ is not
                  // in the hash table.
                  NABoolean skewedValueFound = FALSE;

                  // Find the collision chain.
                  Int32 slot = (Int32) (partNumInfo_.partHash_ % numSkewHdrs_);
                  Int32 skewArrayIdx = skewHdrs_[slot];

                  while (skewArrayIdx != noLink_)
                    {
                      if ( partNumInfo_.partHash_ ==
                           splitBottomTdb().skewInfo_->
                               getSkewHashValues()[skewArrayIdx] )
                        {
                          skewedValueFound = TRUE;
                          break;
                        }
                      // Get next on collision chain.
                      skewArrayIdx = skewLinks_[skewArrayIdx];
                    }

                  if (skewedValueFound)
                    {
                      if (splitBottomTdb().doBroadcastSkew())
                        broadcastThisRow_ = TRUE;
                      else
                        {
                          if (uniformDistributionType_ == RoundRobin_)
                            {
                              partNumInfo_.calculatedPartNum_ = roundRobinRecipient_;
                              roundRobinRecipient_ += 1;

                              const UInt32 initialRoundRobin = 
                                        splitBottomTdb().getInitialRoundRobin();
                              const UInt32 finalRoundRobin =
                                        splitBottomTdb().getFinalRoundRobin();

                              if (initialRoundRobin <= finalRoundRobin)
                              {
                                if (roundRobinRecipient_ > finalRoundRobin)
                                  roundRobinRecipient_ = initialRoundRobin;
                              }
                              else
                              {
                                if (roundRobinRecipient_ >= 
                                                           sendNodes_.entries())
                                  roundRobinRecipient_ = 0;
                                else
                                if (roundRobinRecipient_ == finalRoundRobin + 1)
                                  roundRobinRecipient_ = initialRoundRobin;
                              }
                            }
                          else
                              partNumInfo_.calculatedPartNum_ = 
                                  localSendBotttom_;
                        }
                    }
                  
                  if (splitBottomTdb().doBroadcastSkew() &&
                      broadcastOneRow_)
                  {
                    broadcastThisRow_ = TRUE;
                    // stop brodcasting after first one
                    broadcastOneRow_ = FALSE;
                  }
                }

              if (splitBottomTdb().isMWayRepartition())
                // calculated parent number has to be in the parent number range
                ex_assert((partNumInfo_.calculatedPartNum_ >=
                           (ULng32) firstParentNum_) &&
                          (partNumInfo_.calculatedPartNum_ <
                           (ULng32) firstParentNum_ +
                           (ULng32) numOfParentInstances_),
                          "M-Way repartition sends data outside of the range!");
                
	      // data member partNumInfo_.calculatedPartNum_ now contains 
              // the result of the expression (the output partition number)
	      calcPartNumIsValid_ = TRUE;
	    }
	}
      else
	{
          broadcastOneRow_ = splitBottomTdb().getBroadcastOneRow();
	  // ---------------------------------------------------------
	  // no partitioning function or EOD, broadcast up queue entry
	  // to all send/receive nodes
	  // ---------------------------------------------------------
	  broadcastThisRow_ = TRUE;
	  
	  // if this is EOD and we are doing dynamic load balancing and
	  // we have a saved partition input data request, then reply to
	  // the saved partition input data request to see whether there
	  // is more work to be done
	  
	  if (isEofEntry AND NOT staticInputData_)
	    {
	      // reply to the partition input data request and set the
	      // state to WAIT_FOR_PARTITION_INPUT_DATA
	      
	      ex_assert(savedDataMessage_ AND
			savedDataMessage_->getState() ==
			IpcMessageStream::RECEIVED,
			"working without a dynamic data request");
	      inputDataMessageSaved_ = FALSE;
	      replyToMessage(savedDataMessage_);
	      
	      // Leave the EOD up entry from the child there, this is
	      // either removed when the next part input data request comes
	      // or it is given up to the parent when that request indicates
	      // that there is no more work to be done.
	      setWorkState(WAIT_FOR_PARTITION_INPUT_DATA);
	      return WORK_OK;
	    }
	  
	}
      
      // ---------------------------------------------------------
      // check to which requestors to reply
      // ---------------------------------------------------------
      if (splitBottomTdb().combineRequests_)
	{
	  if (broadcastThisRow_)
	    {
              if (splitBottomTdb().isMWayRepartition())
                {
                  // we only need to send EOD to real consumers (parents)
                  ex_assert(isEofEntry || splitBottomTdb().isAnESPAccess(),
                            "M-Way repartition broadcasts non EOD entry!");
                  partNumLow  = firstParentNum_;
                  partNumHigh = firstParentNum_ + numOfParentInstances_;
                }
              else
                {
	          partNumLow  = 0;
	          partNumHigh = sendNodes_.entries();
                }
	    }
	  else
	    {
	      ex_assert(calcPartNumIsValid_,
			"Negative or no partition number calculated");
              partNumLow  = partNumInfo_.calculatedPartNum_;
              partNumHigh = partNumLow + 1;
	    }
	}
      else
	{
	  // reply to the single requestor currRequestor_, if it matches
	  // the calculated partition number or if we broadcast
	  if (broadcastThisRow_ OR partNumInfo_.calculatedPartNum_ == currRequestor_)
	    {
	      partNumLow  = currRequestor_;
	      partNumHigh = partNumLow + 1;
	    }
	  else
	    {
	      // set an empty range for the partition numbers
	      partNumLow  = 0;
	      partNumHigh = 0;
	    }
	}
      
      // Check whether we have requests for the output queues (we don't
      // want to reply to a request that's not even there yet) and
      // check whether the output queues aren't full
      for (CollIndex i = partNumLow; i < partNumHigh; i++)
	{
          ex_queue* squeueDown = sendNodesDownQ_[i - firstParentNum_];
          ex_queue* squeueUp = sendNodesUpQ_[i - firstParentNum_];
	  
	  if (squeueDown->isEmpty())
	    {
	      setWorkState(WAIT_FOR_MORE_REQUESTORS);
	      emptyDownQueue_ = i - firstParentNum_;
	      return WORK_OK;
	    }
	  
	  if (squeueUp->isFull() &&
	      (isEofEntry ||
	       squeueDown->getHeadEntry()->downState.request !=
	       ex_queue::GET_NOMORE))
	    {
	      // we would need to put an entry into a full up queue,
	      // wait until the up queue frees up
	      setWorkState(WAIT_TO_REPLY);
	      fullUpQueue_ = i - firstParentNum_;
	      return WORK_OK;
	    }
	}         // for partNumLo to partNumHigh
      
      // ---------------------------------------------------------
      // We got the space, now send the up entries.
      // ---------------------------------------------------------
      for (CollIndex p = partNumLow; p < partNumHigh; p++)
	{
	  // copy the up entry from the child to one send/receive node
          ex_queue* squeueDown = sendNodesDownQ_[p - firstParentNum_];
          ex_queue* squeueUp = sendNodesUpQ_[p - firstParentNum_];
	  
	  // Send a reply, unless the request has been canceled, 
	  // and it is not eof.
	  
	  if (isEofEntry ||
	      (squeueDown->getHeadEntry()->downState.request != 
	                                           ex_queue::GET_NOMORE))
	    {
	      ex_queue_entry *sentry = squeueUp->getTailEntry();
	      
	      sentry->upState = centry->upState;
	      
	      if (isErrorEntry AND
		  sentry->upState.status == ex_queue::Q_OK_MMORE)
		sentry->upState.status = ex_queue::Q_SQLERROR;
	      
              if (splitBottomTdb().getExtractProducerFlag())
              {
                if (sentry->upState.status == ex_queue::Q_OK_MMORE &&
                    p != (partNumHigh - 1))
                {
                  // This is a data row and we're doing a parallel
                  // extract. We only want to send it to the last
                  // queue. If this is not the last queue then go back
                  // to the top of the for loop.
                  continue;
                }
              }
          
	      // the down request may not have come from this queue, make
	      // sure the up request looks like a reply to the down request
	      // of the queue
	      sentry->upState.downIndex = squeueDown->getHeadIndex();
	      sentry->upState.parentIndex =
		squeueDown->getHeadEntry()->downState.parentIndex;
	      
	      // we take the liberty not to set the match # here
	      // since we know that the send top doesn't mind that
	      
	      sentry->copyAtp(centry);
	      
	      if (isEofEntry)
		{
		  if (centry->getAtp()->getDiagsArea())
		    {
		      // Don't multiply the diagnostics area that
		      // comes with a Q_NO_DATA entry, send it only to
		      // one parent. This is especially important for
		      // diags areas that have the number of rows
		      // modified in them. Skip this step for a
		      // parallel extract query and broadcast the
		      // diags area to all send bottoms.
                      if (!splitBottomTdb().getExtractProducerFlag())
                        centry->getAtp()->setDiagsArea(NULL);
		    }
		}
	      else if (statsEntry)
		{
		  statsEntry->incActualRowsReturned();
		}
	      
	      squeueUp->insert();
	      
	    }     // if !GET_NOMORE
	  
	  // if this was EOF, remove the corresponding entry from
	  // the down queue
	  if (isEofEntry)
	    {
	      squeueDown->removeHead();
	    } 
	} // for each send queue that we have to send the entry to
      
      // we're done with all send bottoms, remove request from the
      // child queue and invalidate the calculated partition number
      qChild_.up->removeHead();
      calcPartNumIsValid_ = FALSE;
      
      // We return to the idle state if this was the EOF entry.
      if (isEofEntry)
	{
          NABoolean foundOne;
          ExWorkProcRetcode retcode = processInactiveSendTopsBelow(foundOne);
          if (retcode != WORK_OK)
            {
              ex_assert(retcode == WORK_BAD_ERROR, 
                        "Unexpected return from processInactiveSendTopsBelow");
              return retcode;
            }
	  if (foundOne)
	    setWorkState(WAIT_FOR_LATE_CANCELS);
	  else
            setWorkState(IDLE);
	  numCanceledPartitions_ = 0;
	  // The split bottom node handles only one down request at a
	  // time. There may be no queue event that notifies us of
	  // additional down requests, so call the work procedure again.
	  return WORK_CALL_AGAIN;
	}

    } // while a child q entry is available

  return WORK_OK;
}

ExWorkProcRetcode ex_split_bottom_tcb::processCancel()
{
  // If this is a parallel extract producer and any cancel activity is
  // taking place, make sure the down queue entry from the consumer
  // gets marked as GET_NOMORE. This prevents the split bottom node
  // from sending data rows to the consumer even though all other send
  // bottoms have cancelled.
  if (splitBottomTdb().getExtractProducerFlag())
  {
    CollIndex i = sendNodes_.entries();
    ex_assert(i == 2, "Extract producer should always have 2 send nodes");
    ex_queue *squeueDown = sendNodesDownQ_[i - 1];
    if (!squeueDown->isEmpty())
    {
      if (squeueDown->getHeadEntry()->downState.request !=
          ex_queue::GET_NOMORE)
        squeueDown->cancelRequest();
    }
  }

   // Don't process cancels unless we are working on a
   // request. Cancels in the IDLE work state or before we receive
   // partition input data are handled by the regular work method.
   if (workState_ == IDLE || workState_ == WAIT_FOR_PARTITION_INPUT_DATA)
    return WORK_OK;

  // Could get here if another ESP cancels at about the same time that this
  // ESP has entered the LIMITS_EXCEEDED_ERROR or CLEANUP state.  If so,
  // we do not need to do anything special for that ESP's cancel. 
  if (workState_ == CLEANUP || workState_ == LIMITS_EXCEEDED_ERROR)
  {
    return WORK_OK;
  }

  // Has everyone canceled?  If so, propagate cancel to child and
  // remember that all have canceled.

  CollIndex sendNodeLo;
  CollIndex sendNodeHi;
  if (splitBottomTdb().combineRequests_)
    {
      if (splitBottomTdb().isMWayRepartition())
        {
          // we only send reply to actual consumers
          sendNodeLo = firstParentNum_;
          sendNodeHi = firstParentNum_ + numOfParentInstances_;
        }
      else
        {
          sendNodeLo = 0;
          sendNodeHi = sendNodes_.entries();
        }
    }
  else
    {
      sendNodeLo = currRequestor_;
      sendNodeHi = sendNodeLo + 1;
    }

  if ((CollIndex) numCanceledPartitions_ == (sendNodeHi - sendNodeLo))
    // we have already done all the work in a previous call
    return WORK_OK;
  
  // recompute the number of canceled requests that we are actually working on
  numCanceledPartitions_ = 0;

  for (CollIndex i = sendNodeLo; i < sendNodeHi; i++)
    {
      ex_queue* squeueDown = sendNodesDownQ_[i - firstParentNum_];
      if (!squeueDown->isEmpty())
	{
	  ex_queue_entry *sentry = squeueDown->getHeadEntry();
	  if (sentry->downState.request == ex_queue::GET_NOMORE)
	    numCanceledPartitions_++;
	} // if isEmpty ... else ...
    } // for all sendNodes.        

  // if all interested requestors canceled then cancel the request to the child
  if ((CollIndex) numCanceledPartitions_ == (sendNodeHi - sendNodeLo))
    {
      switch (workState_)
	{
	case WAIT_TO_REPLY:
	  // Some of the full up queues that prevented us from working
	  // may now have a GET_NOMORE request and possibly we don't
	  // need to insert an entry into them anymore. Go back to the
	  // regular work state and give this a try.
	  setWorkState(WORK_ON_REQUEST);
	  // I must be sure that work() will be scheduled again, 
	  // because no other scheduling event is guaranteed.
	  ioHandler_->schedule();
                    
	  // fall thru....
	case WORK_ON_REQUEST:
	case WAIT_FOR_PARTITION_INPUT_DATA:
	  qChild_.down->cancelRequest();
          setWorkState(CLEANUP);
	  break;

        case WAIT_FOR_LATE_CANCELS:
        case IDLE:
          // the work method is either still busy with late
          // cancels of the previous request or it has not
          // yet seen this new cancel request, give it a
          // chance to handle this
          return WORK_POOL_BLOCKED;

	case WAIT_FOR_MORE_REQUESTORS:
	default:
	  // when we have a GET_NOMORE request from all of our
	  // consumer ESPs, then we must be actively working on
	  // a request
	  ex_assert(0, "All requestors canceled w/invalid work state");
	}
    }

  return WORK_OK;
}

ExWorkProcRetcode ex_split_bottom_tcb::workCancelBeforeSent()
{
  // -----------------------------------------------------------------
  // Figure out who the sendNodes are, make sure they got room
  // in the up queues, and then send a Q_NO_DATA to each.
  // -----------------------------------------------------------------
  CollIndex partNumLow;
  CollIndex partNumHigh;

  // If we are doing dynamic load balancing and
  // we have a saved partition input data request, then reply to
  // the saved partition input data request to see whether there
  // is more work to be done

  if (NOT staticInputData_)
    {
      // reply to the partition input data request and set the
      // state to WAIT_FOR_PARTITION_INPUT_DATA

      ex_assert(savedDataMessage_ AND
		savedDataMessage_->getState() ==
		IpcMessageStream::RECEIVED,
		"working without a dynamic data request");
      inputDataMessageSaved_ = FALSE;
      replyToMessage(savedDataMessage_);

      // Leave the EOD up entry from the child there, this is
      // either removed when the next part input data request comes
      // or it is given up to the parent when that request indicates
      // that there is no more work to be done.
      setWorkState(WAIT_FOR_PARTITION_INPUT_DATA);
      return WORK_OK;
    }

  // check to which requestors to reply
  //
  // - see solution 10-090326-0318 for more details
  // If noNewRequest flag is set, it means master process has issued
  // release work msg to all esps. In that case this esp should reply
  // right away to any cancel msg it receives instead of waiting for
  // cancel msg from all of its parent esps.
  //
  bool useFix0318 = false;
  bool replySingleRequestor = false;
  if (!splitBottomTdb().combineRequests_)
    replySingleRequestor = true;
  else if (glob_->noNewRequest())
  {
    useFix0318 = true;
    replySingleRequestor = true;
  }
  if (replySingleRequestor)
    {
      // reply to the single requestor currRequestor_.
      partNumLow  = currRequestor_;
      partNumHigh = partNumLow + 1;
    }
  else
    {
      if (splitBottomTdb().isMWayRepartition())
        {
          // we only send reply to actual consumers
          partNumLow = firstParentNum_;
          partNumHigh = firstParentNum_ + numOfParentInstances_;
        }
      else
        {
          partNumLow = 0;
          partNumHigh = sendNodes_.entries();
        }
    }

  //
  // Now send the up q_no_data, to anybody ready.
  //
  for (CollIndex p = partNumLow; p < partNumHigh; p++)
    {
      // Make a Q_NO_DATA up entry and send it to one send/receive node
      ex_queue* squeueDown = sendNodesDownQ_[p - firstParentNum_];
      ex_queue* squeueUp = sendNodesUpQ_[p - firstParentNum_];

      // this method should be invoked only when all send bottoms have
      // a GET_NOMORE and when no activity has started yet
      // (once we support multiple simultaneous requests to ESPs there
      // might be a slight chance that the up queue is full)
      ex_assert(!squeueDown->isEmpty(),
		"Should have a GET_NOMORE request");
      ex_assert(!squeueUp->isFull(),
		"Up queue should be empty at this point");
      if (useFix0318)
        bugCatcher3041_ = squeueDown->getHeadIndex();

      ex_queue_entry *sentry = squeueUp->getTailEntry();

      sentry->upState.status = ex_queue::Q_NO_DATA;
      sentry->upState.setMatchNo(0);
      sentry->upState.downIndex = squeueDown->getHeadIndex();
      sentry->upState.parentIndex =
	squeueDown->getHeadEntry()->downState.parentIndex;

      squeueUp->insert();

      squeueDown->removeHead();
    } // for each send queue that we have to send the entry to

  NABoolean foundOne;
  ExWorkProcRetcode retcode = processInactiveSendTopsBelow(foundOne);
  if (retcode != WORK_OK)
    {
      ex_assert(retcode == WORK_BAD_ERROR, 
                "Unexpected return from processInactiveSendTopsBelow");
      return retcode;
    }
  if (foundOne)
    setWorkState(WAIT_FOR_LATE_CANCELS);
  else
    setWorkState(IDLE);
  numCanceledPartitions_ = 0;
	
  // The split bottom node handles only one down request at a
  // time. There may be no queue event that notifies us of
  // additional down requests, so call the work procedure again.
  return WORK_CALL_AGAIN;

}
  
void ex_split_bottom_tcb::replyToMessage(IpcMessageStream *sm)
{
  // prepare message for a reply, then do it
  ExEspReturnStatusReplyHeader *repHdr =
    new(getGlobals()->getDefaultHeap()) ExEspReturnStatusReplyHeader(getGlobals()->getDefaultHeap());

  ex_assert(sm->getState() == IpcMessageStream::EXTRACTING OR
	    sm->getState() == IpcMessageStream::RECEIVED,
	    "Saved msg. not ready for reply");
  sm->clearAllObjects();
  repHdr->key_           = espInstanceDir_->findKey(myHandle_);
  repHdr->handle_        = myHandle_;
  repHdr->instanceState_ = ExEspReturnStatusReplyHeader::INSTANCE_ACTIVE;
  *sm << *repHdr;
  repHdr->decrRefCount();
  sm->send(FALSE);
}

NABoolean ex_split_bottom_tcb::reportErrorToMaster()
{
  if (workMessageSaved_)
    {
      // prepare message for a reply, then do it
      ExEspReturnStatusReplyHeader *repHdr =
	new(getGlobals()->getDefaultHeap()) ExEspReturnStatusReplyHeader(getGlobals()->getDefaultHeap());
      
      savedWorkMessage_->clearAllObjects();
      repHdr->key_           = espInstanceDir_->findKey(myHandle_);
      repHdr->handle_        = myHandle_;
      repHdr->instanceState_ = ExEspReturnStatusReplyHeader::INSTANCE_BROKEN;
      *savedWorkMessage_ << *repHdr;
      *savedWorkMessage_ << *glob_->castToExExeStmtGlobals()->getDiagsArea();
      workMessageSaved_ = FALSE;
      repHdr->decrRefCount();
      savedWorkMessage_->send(FALSE);
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

void ex_split_bottom_tcb::acceptNewPartInputValues(NABoolean moreWork)
{
  // for a static assignment this must be called exactly once with
  // a TRUE input parameter
  ex_assert(NOT staticInputData_ OR moreWork,
	    "Static input data assignment must have part input values");

  if (workState_ != WAIT_FOR_PARTITION_INPUT_DATA)
    return;

  if (NOT qChild_.up->isEmpty() AND
      qChild_.up->getHeadEntry()->upState.status == ex_queue::Q_NO_DATA)
    {
      // There was an EOD request stuck in the child's up queue

      if (moreWork)
	{
	  // simply send down a new request and remove the stuck up entry
	  qChild_.up->removeHead();
	  calcPartNumIsValid_ = FALSE;
	  // by resetting the work state to idle the work procedure will send
	  // the same request down again
	  setWorkState(IDLE);
	}
      else
	{
	  // by resetting the work request to WORK_ON_REQUEST the work
	  // procedure will finish what it was doing (which is sending
	  // and EOD indicator back to its clients)
	  setWorkState(WORK_ON_REQUEST);
	}
    }
  else
    {
      // If there was no EOD entry stuck in the up queue then
      // this means we got partition input data before the data
      // flow begun. State "IDLE" indicates that we have partition
      // input data and can start working immediately after we get
      // the first request.
      setWorkState(IDLE);
    }
}

Int32 ex_split_bottom_tcb::numChildren() const
{
  return 1 + (Int32) sendNodes_.entries();
}   

const ex_tcb* ex_split_bottom_tcb::getChild(Int32 pos) const
{
  ex_assert((pos >= 0), "");
  if (pos == 0)
    return tcbChild_;
  else if (pos > 0 AND pos <= (Int32)sendNodes_.entries())
    return (ex_tcb *) sendNodes_[pos-1];
  else
    return NULL;
}

void ex_split_bottom_tcb::testAllQueues()
{
  getGlobals()->testAllQueues();

  // Must test my own queues, because split bottom doesn't report having 
  // a (note that "a" implies "one") parent queue when getParentQueue() is 
  // called.
  CollIndex numSendNodes = sendNodes_.entries();
  for (CollIndex i = 0; i < numSendNodes; i++ )
    {
      if (NOT sendNodesUpQ_[i]->isEmpty())
        ex_assert(0, "Orphan entries in split bottom's up queue!");

      if (NOT sendNodesDownQ_[i]->isEmpty())
        ex_assert(0, "Orphan entries in split bottom's down queue!");
    }
}

// This method is called for each "open message" of
// the data connections in the fragment instance.
// All of the calls must come before any rows are 
// returned (tbd - figure an efficient way to 
// test this).
void ex_split_bottom_tcb::setLocalSendBottom(CollIndex s)
{
  if (splitBottomTdb().useSkewBuster() &&
      splitBottomTdb().doBroadcastSkew() == FALSE &&
      splitBottomTdb().forceSkewRoundRobin() == FALSE)
    {
      if (localSendBotttom_ != NULL_COLL_INDEX)
        {
          // If we have >1 co-located ESP, then just use
          // round-robin.  If >1 consumer ESPs per CPU
          // were an important scenario, then in the future
          // we could round-robin among them.
          uniformDistributionType_ = RoundRobin_;
          localSendBotttom_ = NULL_COLL_INDEX;
        }
      else if (splitBottomTdb().bottomNumESPs_ >=
               splitBottomTdb().topNumESPs_)
        {
          uniformDistributionType_ = CoLocated_;
          localSendBotttom_ = s;
        }
    }
}

void ex_split_bottom_tcb::cpuLimitExceeded()
{
  if ((workState_ == WORK_ON_REQUEST         ) ||
      (workState_ == WAIT_FOR_MORE_REQUESTORS) ||
      (workState_ == WAIT_TO_REPLY           ))
  {
    setWorkState(LIMITS_EXCEEDED_ERROR);
    ioHandler_->schedule();
  }
  else
  {
    // CLEANUP is valid for this call, but nothing needs to
    // be done, i.e., we already started canceling, and the 
    // scheduler might determine again, repeatedly, that we've
    // exceeded our time. Should be harmless.
    // Same for LIMITS_EXCEEDED_ERROR and IDLE as for CLEANUP.
    // Should be okay to be here for WAIT_FOR_LATE_CANCELS, but
    // don't need to do anything since cancel has begun.
    // I don't think we could get here for WAIT_FOR_PARTITION_INPUT_DATA,
    // but shouldn't be necessary to do anything, since 
    // that workState_ is basically a wait state.
  }
}

void ex_split_bottom_tcb::setWorkState(SplitBottomWorkState s)
{
  workState_ = s;
}

// -----------------------------------------------------------------------
// Methods for class SplitBottomRequestMessage
// -----------------------------------------------------------------------

#define ex_assert_both_sides( assert_test, assert_msg )               \
  if (!(assert_test))                                                 \
  {                                                                   \
    if(environment_ &&                                                \
       environment_->getControlConnection() &&                        \
       environment_->getControlConnection()->                         \
               castToGuaReceiveControlConnection() &&                 \
       environment_->getControlConnection()->                         \
               castToGuaReceiveControlConnection()->getConnection())  \
      environment_->getControlConnection()->                          \
               castToGuaReceiveControlConnection()->getConnection()-> \
               dumpAndStopOtherEnd(true, false);                      \
    ex_assert(0, assert_msg);                                         \
  }

SplitBottomRequestMessage::SplitBottomRequestMessage(
     IpcEnvironment *env,
     ex_split_bottom_tcb *splitBottom) :
     IpcMessageStream(env,
		      IPC_MSG_SQLESP_CONTROL_REPLY,
		      CurrEspReplyMessageVersion,
		      0,
		      TRUE)
{
  splitBottom_ = splitBottom;
}

void SplitBottomRequestMessage::actOnSend(IpcConnection * /*connection*/)
{
  // nothing to be done
  if (getState() == ERROR_STATE)
    {
      ex_assert(FALSE,"Error while replying to a part input data msg");
    }
}

void SplitBottomRequestMessage::actOnReceive(IpcConnection * connection)
{
  if (getState() == ERROR_STATE)
    {
      ex_assert(FALSE,"Error occurred while receiving split bottom msg");
    }
  else
    {
      NABoolean saveDataMsg  = FALSE;
      NABoolean saveWorkMsg = FALSE;
      NABoolean saveReleaseMsg = FALSE;

      NAMemory *heap = splitBottom_->getGlobals()->getDefaultHeap();

      while (moreObjects())
	{
	  switch (getNextObjType())
	    {
	    case ESP_PARTITION_INPUT_DATA_HDR:
	      {
		ExEspPartInputDataReqHeader receivedRequest(heap);

		*this >> receivedRequest;
		splitBottom_->staticInputData_ =
		  (NABoolean) receivedRequest.staticAssignment_;

		// does data follow?
		if (moreObjects() AND
		    getNextObjType() == ESP_INPUT_SQL_BUFFER)
		  {
		    TupMsgBuffer recBuffer( 
			 (Lng32) getNextObjSize() + 1000, 
			 TupMsgBuffer::MSG_IN,
			 heap);

		    *this >> recBuffer;

		    recBuffer.get_sql_buffer()->position();
		    tupp inputTuple = recBuffer.get_sql_buffer()->getNext();
		    ex_assert_both_sides(inputTuple.getDataPointer(),
			      "Empty sql_buffer received for part input data");

		    if (splitBottom_->splitBottomTdb().getPartInputDataLen() > 0)
		      {
			ex_assert_both_sides(
			     splitBottom_->workState_ ==
			     ex_split_bottom_tcb::WAIT_FOR_PARTITION_INPUT_DATA,
			     "Got part input data although not waiting for it");
			str_cpy_all(
			     splitBottom_->partInputDataTupp_.getTupleAddress(),
			     inputTuple.getDataPointer(),
			     splitBottom_->splitBottomTdb().getPartInputDataLen());
		      }
		  }

		// let the split bottom handle the arrival of more
		// work or no more work (more work maybe with a new
		// partition input value vector)
		splitBottom_->acceptNewPartInputValues(
		     receivedRequest.staticAssignment_ OR
		     receivedRequest.askForMoreWorkWhenDone_);

		if (receivedRequest.askForMoreWorkWhenDone_)
		  {
		    // keep part input data message until we are done
		    // processing the data identified by it
		    ex_assert_both_sides(
                              NOT splitBottom_->inputDataMessageSaved_,
			      "Received multiple part input data requests");
		    ex_assert_both_sides(
                          NOT receivedRequest.staticAssignment_,
			  "Cant ask for more work with static assignment");
		    saveDataMsg = TRUE;
		  }
	      } // case partition input data
	      break;

	    case ESP_WORK_TRANSACTION_HDR:
	      {
		splitBottom_->getGlobals()->castToExExeStmtGlobals()->
                    castToExEspStmtGlobals()->setNoNewRequest(FALSE);
		
		ExEspWorkReqHeader receivedRequest(heap);
		ExMsgTransId transid(heap);

		*this >> receivedRequest;

		if (moreObjects() AND getNextObjType() == ESP_TRANSID)
		  {
		    *this >> transid;
		  }

		// we know we must be in an ESP, set the reply tag of this
		// request in the statement globals, it is used later to
		// restore the transaction id that is associated with this
		// request
		splitBottom_->glob_->setReplyTag(
		     transid.getTransIdAsInt(),getReplyTag());

		// don't reply now, save the message for a later reply
		ex_assert_both_sides(NOT splitBottom_->workMessageSaved_,
			  "Received multiple transaction/work requests");
		saveWorkMsg = TRUE;
	      }
	      break;

	    case ESP_RELEASE_TRANSACTION_HDR:
	      {
		ExExeStmtGlobals *stmtGlobals = splitBottom_->getGlobals()->
                     castToExExeStmtGlobals();

		ExEspReleaseWorkReqHeader receivedRequest(heap);

		*this >> receivedRequest;

                // Get all messages replied to, and prevent any new messages.
                // tbd - Although this is necessary for "release work", it 
                // might be overkill for a "release transaction".   But it
                // does not appear that these two requests are distint here
                // in this split bottom code.
		stmtGlobals->castToExEspStmtGlobals()->setNoNewRequest(TRUE);

                // Change frag instance state.  Want to let other active
                // frag instances work until all msgs to producer ESPs get 
                // replies.
                splitBottom_->espInstanceDir_->hasReleaseRequest(
                                                   splitBottom_->myHandle_);
		// Reply to this message later.
                saveReleaseMsg = TRUE;
               
	      }
	      break;

	    default:
	      ex_assert_both_sides(0,
                  "Received invalid request for split bottom node");
	    } // switch on next object type
	} // while more objects

      if (saveWorkMsg)
	{
	  ex_assert_both_sides(NOT saveDataMsg,
		"Trans/work requests can't come with load balancing msg");

	  // keep this message elsewhere so we can reply to it
	  // later and keep this message stream free for further
	  // receive operations
	  splitBottom_->workMessageSaved_ = TRUE;
	  giveMessageTo(*splitBottom_->savedWorkMessage_,connection);
	}
      else if (saveDataMsg)
	{
	  splitBottom_->inputDataMessageSaved_ = TRUE;
	  giveMessageTo(*splitBottom_->savedDataMessage_,connection);
	}
      else if (saveReleaseMsg)
        {
	  splitBottom_->releaseMessageSaved_ = TRUE;
	  giveMessageTo(*splitBottom_->savedReleaseMessage_,connection);
        }
      else
	{
	  // don't save any parts of the message, reply now
	  splitBottom_->replyToMessage(this);
	}

      // tickle the scheduler
      splitBottom_->ioHandler_->schedule();

    } // no error
}

// -----------------------------------------------------------------------
// Methods for class SplitBottomSavedMessage
// -----------------------------------------------------------------------

SplitBottomSavedMessage::SplitBottomSavedMessage(IpcEnvironment *env) :
     IpcMessageStream(env,
		      IPC_MSG_SQLESP_CONTROL_REPLY,
		      CurrEspReplyMessageVersion,
		      0,
		      TRUE)
{}

void SplitBottomSavedMessage::actOnSend(IpcConnection *)
{
  ex_assert(getState() != ERROR_STATE,
	    "Error during reply to a saved split bottom message");
}

void SplitBottomSavedMessage::actOnReceive(IpcConnection *)
{
  ex_assert(getState() != ERROR_STATE,
	    "Error while saving a split bottom message");
}

// This function returns the sendBottom tcb that connects
// to the consumer query. It is currently the last one in the 
// sendBottom array. If it finds one that already has a client, 
// it returns NULL. 
ex_send_bottom_tcb *ex_split_bottom_tcb::getConsumerSendBottom() const
{
  ex_send_bottom_tcb *sendTCB = NULL;

  if (splitBottomTdb().getExtractProducerFlag())
  {
    CollIndex i = sendNodes_.entries();
    if (i > 0) 
    {
      sendTCB = sendNodes_[i - 1];
      if (!sendTCB->getClient()) // not already connected
        return sendTCB;
    }
  }

  return NULL;
}
