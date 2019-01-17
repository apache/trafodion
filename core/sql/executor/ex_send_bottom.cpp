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
 * File:         ex_send_bottom.C
 * Description:  Send bottom node (server part of a point to point connection)
 *               
 * Created:      1/1/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "BaseTypes.h"
#include "ex_stdh.h"
#include "ex_exe_stmt_globals.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ex_expr.h"
#include "str.h"
#include "ex_send_bottom.h"
#include "ex_send_top.h"
#include "ex_esp_frag_dir.h"
#include "sql_buffer.h"
#include "ex_io_control.h"
#include "ComDiags.h"
#include "ExStats.h"
#include "sql_buffer_size.h"
#include "ExSMTrace.h"

#include "cli_stdh.h"
#include "SMConnection.h"
#include "ExSMCommon.h"

#define ex_assert_both_sides( assert_test, assert_msg )                   \
  if (!(assert_test))                                                     \
  {                                                                       \
    if (routeMsgStream_ && routeMsgStream_->getClient())                  \
      routeMsgStream_->getClient()->dumpAndStopOtherEnd(true, false);     \
    ex_assert(0, assert_msg);                                             \
  }

// -----------------------------------------------------------------------
// Methods for class ex_send_bottom_tdb
// -----------------------------------------------------------------------

  
ex_tcb * ex_send_bottom_tdb::build(ex_globals *)
{
  ex_assert(FALSE,"send bottom node must be built using buildInstance()");
  return NULL;
}

ex_send_bottom_tcb * ex_send_bottom_tdb::buildInstance(
     ExExeStmtGlobals * glob,
     ExEspFragInstanceDir *espInstanceDir,
     const ExFragKey &myKey,
     const ExFragKey &parentKey,
     ExFragInstanceHandle myHandle,
     Lng32 parentInstanceNum,
     NABoolean isLocal)
{
  ex_send_bottom_tcb *result =
    new(glob->getSpace()) ex_send_bottom_tcb(*this,
                                             glob,
                                             espInstanceDir,
                                             myKey,
                                             parentKey,
                                             myHandle,
                                             parentInstanceNum);
  result->registerSubtasks();

  return result;
}

ExOperStats * ex_send_bottom_tcb::doAllocateStatsEntry(CollHeap *heap, 
                                                       ComTdb *tdb)
{
  ExOperStats * stat = NULL;
  ComTdb::CollectStatsType statsType = getGlobals()->getStatsArea()->getCollectStatsType();
  if (statsType == ComTdb::OPERATOR_STATS)
  {
    // check to see if entry for this node exists. This could happen if
    // multiple send bottom tcbs are constructed for a given exchange,
    // as is normal for an producer ESP below multiple consumer ESPs.
    stat =
      getGlobals()->getStatsArea()->get(ExOperStats::EX_OPER_STATS,
					tdb->getTdbId());
    if (stat)
    {
      setStatsEntry(stat);
      stat->incDop();
      // return NULL so that caller will see no new ExOperStats was
      // allocated and therefore need not do any more setup tasks for
      // it.
      return NULL;
    }

    stat = ex_tcb::doAllocateStatsEntry(heap, tdb);
    
  }
  else
  {
    stat = new(heap) ExESPStats(heap,
			      sendBottomTdb().getReplyBufferSize(),
                              sendBottomTdb().getRequestBufferSize(),
			      getParentInstanceNum(),
			      this,
                              tdb);
  }
  if (stat)
  {
    // Assuming parent Tdb is the one above this Tdb
    stat->setParentTdbId(tdb->getTdbId()+1);
    // Set SplitBottom as the left child TdbId
    stat->setLeftChildTdbId(tdb->getTdbId()-1);
  }
  return stat;
}

// -----------------------------------------------------------------------
// Methods for class ex_send_bottom_tcb
// -----------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// constructor
ex_send_bottom_tcb::ex_send_bottom_tcb(
     const ex_send_bottom_tdb& sendBottomTdb,
     ExExeStmtGlobals *glob,
     ExEspFragInstanceDir *espInstanceDir,
     const ExFragKey &myKey,
     const ExFragKey &parentKey,
     ExFragInstanceHandle myHandle,
     Lng32 parentInstanceNum)
: ex_tcb(sendBottomTdb,1,glob), 
  workAtp_(0),
  myFragId_(myKey.getFragId()),
  myHandle_(myHandle),
  parentInstanceNum_(parentInstanceNum),
  espInstanceDir_(espInstanceDir),
  ioSubtask_(NULL),
  cancelMsgStream_(NULL),
  cancelOnSight_(glob->getDefaultHeap()),
  currentRequestBuffer_(NULL),
  currentReplyBuffer_(NULL),
  currentBufferNumber_(0),
  routeMsgStream_ (NULL),
  workMsgStream_(NULL),
  cancelReplyPending_(FALSE),
  lateCancel_(FALSE),
  isActive_(FALSE),
  isExtractConsumer_(FALSE),
  isExtractWorkDone_(FALSE),
  defragPool_(NULL),
  defragTd_(NULL),
  connection_(NULL)
{
  smTarget_.node = 0;
  smTarget_.pid = 0;
  smTarget_.tag = 0;
  smTarget_.id = 0;
  
  if (sendBottomTdb.getExchangeUsesSM())
  {
    Int32 parentFrag = (Int32) parentKey.getFragId();
    Int32 parentInst = (Int32) parentInstanceNum_;
    
    const IpcProcessId &otherEnd =
      glob->getInstanceProcessId((ExFragId) parentFrag,
                                 (Lng32) parentInst);

    const GuaProcessHandle &phandle = otherEnd.getPhandle();
    Int32 otherCPU, otherPID, otherNode;
    SB_Int64_Type seqNum = 0;
    phandle.decompose2(otherCPU, otherPID, otherNode
                      , seqNum
                      );
    
    // Seaquest Node num == old cpuNum
    smTarget_.node = ExSM_GetNodeID(otherCPU);
    smTarget_.pid = otherPID;
    smTarget_.tag = sendBottomTdb.getSMTag();
    smTarget_.id = glob->getSMQueryID();
    ex_assert(smTarget_.id > 0, "Invalid SeaMonster query ID");
    
  } // isSMEnabled()

  CollHeap * space = glob->getSpace();

  // Allocate the queue to communicate with the split bottom node
  // (here is the exception where we allocate ATPs for the down entry
  // because we get tupp_descriptors from the buffer that need to be
  // put into ATPs)
  qSplit_.down = new(space) ex_queue(ex_queue::DOWN_QUEUE,
				     sendBottomTdb.queueSizeDown_,
				     sendBottomTdb.criDescDown_,
				     space,
				     ex_queue::ALLOC_ATP);

  // there is no private state for send bottom nodes

  qSplit_.up = new(space) ex_queue(ex_queue::UP_QUEUE,
				   sendBottomTdb.queueSizeUp_,
				   sendBottomTdb.criDescUp_,
				   space);
			    
  // allocate work atp
  workAtp_ = allocateAtp(sendBottomTdb.workCriDesc_, space);

  // fixup expression
  if (moveOutputValues())
    (void) moveOutputValues()->fixup(0, getExpressionMode(), this,
				     glob->getSpace(),
				     glob->getDefaultHeap(), FALSE, glob);

  nextToSend_ = qSplit_.down->getHeadIndex();

  requestBufferSize_ = sendBottomTdb.getRequestBufferSize();
  replyBufferSize_ = sendBottomTdb.getReplyBufferSize();

  IpcMessageObjSize maxBufSize = requestBufferSize_ > replyBufferSize_ ? 
    requestBufferSize_ : replyBufferSize_;
  maxBufSize += (sizeof(TupMsgBuffer) + sizeof(ExEspReturnDataReplyHeader));
  
  // pad for alignment and possible ComDiags objects ??? Larry Schumacher ???
  // see a longer comment with detailed explanation in file ex_send_top.cpp.
  maxBufSize += 1000;

  UInt32 numReplyBuffers = sendBottomTdb.getNumReplyBuffers();
  
  // The default value for GEN_SNDT_NUM_BUFFERS is 2. During
  // SeaMonster testing we noticed that a value of 1 can cause
  // hangs. We bump the value up to a minimum of 2.
  UInt32 numRequestBuffers = sendBottomTdb.getNumRequestBuffers();
  if (numRequestBuffers < 2)
    numRequestBuffers = 2;

  // create a msg stream object to route msgs to my other two msg streams.
  routeMsgStream_ = new(glob->getSpace())
    ExSendBottomRouteMessageStream(glob, this);

  workMsgStream_ = new(glob->getSpace())
    ExSendBottomWorkMessageStream(glob,
                                  numReplyBuffers,
                                  numRequestBuffers,
                                  maxBufSize,
                                  this);
  
  // create a cancel message stream object, just in case.
  cancelMsgStream_ = new(glob->getSpace())
    ExSendBottomCancelMessageStream(glob,
                                    this);

  if (sendBottomTdb.getExchangeUsesSM())
  {
    // Without SeaMonster, the IPC connection to send top is created
    // on demand by IPC infrastructure when send top sends an OPEN
    // request. With SeaMonster, the connection to send top is not a
    // physical IPC connection and we can create the C++ object now.

    // When we call the connection constructor we choose the number of
    // pre-allocated receive buffers. The number will be:
    // * number of request buffers (from the plan)
    // * plus one for cancel requests
    
    connection_ = new (glob->getSpace())
      SMConnection(glob->getIpcEnvironment(),
                   smTarget_,
                   numRequestBuffers + 1,
                   maxBufSize,
                   this,
                   NULL,
                   TRUE);
    
    routeMsgStream_->setClient(connection_);
    workMsgStream_->setClient(connection_, FALSE);
    cancelMsgStream_->setClient(connection_, FALSE);

    routeMsgStream_->setSMContinueProtocol(TRUE);
    workMsgStream_->setSMContinueProtocol(TRUE);

    SMConnection *smConn = (SMConnection *)
      connection_->castToSMConnection();
    ex_assert(smConn, "Invalid SM connection pointer");
    smConn->setDataStream(workMsgStream_);

    int myFrag = (int) myFragId_;
    int parentFrag = (int) parentKey.getFragId();
    int parentInst = (int) parentInstanceNum_;

    int downQueueLen = (int) sendBottomTdb.queueSizeDown_;
    int upQueueLen = (int) sendBottomTdb.queueSizeUp_;
        
    EXSM_TRACE(EXSM_TRACE_INIT|EXSM_TRACE_TAG, "SNDB CTOR %p", this);
    EXSM_TRACE(EXSM_TRACE_INIT|EXSM_TRACE_TAG, "SNDB  frag %d inst %d",
               myFrag, (int) glob->getMyInstanceNumber());
    EXSM_TRACE(EXSM_TRACE_INIT|EXSM_TRACE_TAG,
               "SNDB  parent frag %d inst %d", parentFrag, parentInst);

    // regress/executor/TEST123 expects this line of output when
    // tracing is reduced to tags only
    EXSM_TRACE(EXSM_TRACE_INIT|EXSM_TRACE_TAG,
               "SNDB  tgt %d:%d:%" PRId64 ":%d",
               (int) smTarget_.node, (int) smTarget_.pid, smTarget_.id,
               (int) smTarget_.tag);

    EXSM_TRACE(EXSM_TRACE_INIT, "SNDB  route stream %p", routeMsgStream_);
    EXSM_TRACE(EXSM_TRACE_INIT, "SNDB  work stream %p", workMsgStream_);
    EXSM_TRACE(EXSM_TRACE_INIT, "SNDB  cancel stream %p", cancelMsgStream_);
    EXSM_TRACE(EXSM_TRACE_INIT,
               "SNDB  request size %d", (int) requestBufferSize_);
    EXSM_TRACE(EXSM_TRACE_INIT,
               "SNDB  reply size %d", (int) replyBufferSize_);
    EXSM_TRACE(EXSM_TRACE_INIT,
               "SNDB  stream max buf %d",
               (int) workMsgStream_->getBufferSize());
    EXSM_TRACE(EXSM_TRACE_INIT, "SNDB  conn %p", connection_);
    EXSM_TRACE(EXSM_TRACE_INIT, "SNDB  req bufs %d reply bufs %d",
               (int) numRequestBuffers, (int) numReplyBuffers);
    EXSM_TRACE(EXSM_TRACE_INIT, "SNDB  down queue %d, up queue %d",
               downQueueLen, upQueueLen);

  } // if exchanges uses SM

  defragPool_ = NULL;
  defragTd_ = NULL;

  if (sendBottomTdb.considerBufferDefrag())
  {
	 //allocate the deframentation space 
   // if (resizeCifRecord())
    {
      Lng32 neededBufferSize =
      (Lng32) SqlBufferNeededSize( 1,
                                   sendBottomTdb.getUpRecordLength(),
                                   SqlBuffer::NORMAL_ );

      defragPool_ = new(space) sql_buffer_pool(1, neededBufferSize *2, space);// *2 not necesary here

      defragTd_ = defragPool_->addDefragTuppDescriptor(sendBottomTdb.getUpRecordLength());
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
// destructor
ex_send_bottom_tcb::~ex_send_bottom_tcb()
{
  ex_send_bottom_tcb::freeResources();
}

/////////////////////////////////////////////////////////////////////////////
// free all resources
void ex_send_bottom_tcb::freeResources()
{
  delete qSplit_.up;
  delete qSplit_.down;
  if (workAtp_)
    deallocateAtp(workAtp_, getSpace());

  delete routeMsgStream_;
  delete workMsgStream_; 
  delete cancelMsgStream_;

  delete connection_;
}

/////////////////////////////////////////////////////////////////////////////
// register tcb for work
void ex_send_bottom_tcb::registerSubtasks()
{
  // The send bottom node handles the top end of the queue to the split
  // top node.
  getGlobals()->getScheduler()->registerUnblockSubtask(sWork,
						       this,
						       qSplit_.down,
                                                       "WK");
  getGlobals()->getScheduler()->registerInsertSubtask(sWork,
						      this,
						      qSplit_.up);

  // register a non-queue event for the IPC with the send top node
  ioSubtask_ =
    getGlobals()->getScheduler()->registerNonQueueSubtask(sWork,this);
  // need another non-queue event for the cancel message stream
  ioCancelSubtask_ =
    getGlobals()->getScheduler()->registerNonQueueSubtask(sCancel,this,"CN");
}

/////////////////////////////////////////////////////////////////////////////
// tcb work method for queue processing
short ex_send_bottom_tcb::work()
{
  EXSM_TRACE(EXSM_TRACE_WORK, "SNDB %p BEGIN WORK", this);

  short result = checkRequest();
  EXSM_TRACE(EXSM_TRACE_WORK, "SNDB %p checkRequest rc %s", this,
             ExWorkProcRetcodeToString(result));

  if (result == WORK_OK)
  {
    result = checkReply();
    EXSM_TRACE(EXSM_TRACE_WORK, "SNDB %p checkReply rc %s", this,
               ExWorkProcRetcodeToString(result));
  }

  EXSM_TRACE(EXSM_TRACE_WORK, "SNDB %p END WORK %s", this,
             ExWorkProcRetcodeToString(result));

  return result;
}

/////////////////////////////////////////////////////////////////////////////
// check for request message from send top and put data in queue
short ex_send_bottom_tcb::checkRequest()
{
  ExOperStats *statsEntry;
  ExESPStats *espStats;
  
  while (NOT qSplit_.down->isFull())
    {  // process data from send top

      // get sql buffer from message stream
      if (currentRequestBuffer_ == NULL)
	{
	  currentRequestBuffer_ = getRequestBuffer();
	  if (currentRequestBuffer_ == NULL)
            return WORK_OK;

	  currentBufferNumber_++;
          statsEntry = getStatsEntry();
          if (statsEntry)
            espStats = statsEntry->castToExESPStats();
          else
            espStats = NULL;
	  // we got a buffer. Update statistics
	  if (espStats)
          {
	    SqlBuffer* sb = currentRequestBuffer_->get_sql_buffer();
	    sb->driveUnpack();
	    espStats->bufferStats()->totalRecdBytes() += sb->getSendBufferSize();
	    espStats->bufferStats()->recdBuffers().addEntry(sb->get_used_size());
          }
	}
      SqlBuffer* sb = currentRequestBuffer_->get_sql_buffer();
      sb->driveUnpack();
     
      statsEntry = getStatsEntry();
      // Notify statement globals if stats is enabled for this
      // buffer. Skip this step for a parallel extract consumer. For
      // an extract consumer we allow stats to be enabled only by the
      // initiating master executor.
      if (!isExtractConsumer_)
        getGlobals()->setStatsEnabled(sb->statsEnabled());

      while (NOT qSplit_.down->isFull())
	{
	  tupp_descriptor* controlInfoAsTupp = sb->getNext();
	  tupp_descriptor* rowReceived;
	  ControlInfo msgControlInfo;

	  if (controlInfoAsTupp == NULL)
	    {
	      currentRequestBuffer_ = NULL;
	      break;
	    }

	  controlInfoAsTupp->setRelocatedAddress(0);
	  controlInfoAsTupp->resetCommFlags();

	  str_cpy_all((char*) &msgControlInfo,
		      controlInfoAsTupp->getTupleAddress(),
		      sizeof(ControlInfo));

	  rowReceived = 
	    msgControlInfo.getIsDataRowPresent() ? sb->getNext() : NULL;
	  //
	  // Copy the down state that came in the message buffer into
	  // the queue entry.
	  // The unique identifier of a request is the parentIndex
	  // field which is set by the remote node. We know that the send
	  // top node can't have two entries with the same parentIndex
	  // outstanding.
	  //
	  ex_queue_entry* sentry = qSplit_.down->getTailEntry();

	  sentry->downState = msgControlInfo.getDownState();

	  // We always send a single tupp down, if any (tupp index = 2)
	  if (sendBottomTdb().getDownRecordLength() > 0)
          {
            // The TDB has a non-zero input record length. If this is
            // not a parallel extract consumer request then an input
            // row must be present.
            if (!isExtractConsumer_)
            {
              ex_assert_both_sides(rowReceived!=NULL,"no input data row");
              rowReceived->setRelocatedAddress(0);
              sentry->getTupp(2) = rowReceived;
            }
          }

	  //
	  // Attach diagnostics area, one is associated with
	  // this entry.
	  //
	  if (msgControlInfo.getIsExtDiagsAreaPresent())
	    {
	      IpcMessageObjType msgType;
	      ex_assert_both_sides(
                        workMsgStream_->getNextObjType(msgType)  AND
			msgType == IPC_SQL_DIAG_AREA,
			"no diags areas in message");
	      // construct a copy of diags area from message object
	      // and put it in the queue
	      ComDiagsArea* diagsArea = 
		ComDiagsArea::allocate (getGlobals()->getDefaultHeap());

	      *workMsgStream_ >> *diagsArea;
        
	      sentry->setDiagsArea(diagsArea);
	    }

	  // See if the queue entry is on the cancelOnSight_ list, and if so,
	  // mark it as canceled and remove the entry from the list.

	  if (cancelOnSight_.contains(sentry->downState.parentIndex))
	    {
	      cancelOnSight_.remove(sentry->downState.parentIndex);
	      sentry->downState.request = ex_queue::GET_NOMORE;
	    }

	  // if extract work is already done, do not honor any more 
          // data requests from consumer.
	  if (isExtractWorkDone_) 
            {
	      // construct reply message 
	      if (currentReplyBuffer_ == NULL)
              {
                getReplyBuffer();
	        ex_assert_both_sides(currentReplyBuffer_,
			  "Could not get reply buffer for "
                          "parallel extract operation");
	      }

  	      // prepare proper error message
	      char phandle[100];
	      MyGuaProcessHandle myHandle;
              myHandle.toAscii(phandle, 100);

              ComDiagsArea *diags = ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
              *diags << DgSqlCode(-EXE_PARALLEL_EXTRACT_OPEN_ERROR)
                     << DgString0(phandle)
                     << DgString1("Producer ESP work is already complete");
              *workMsgStream_ << *diags;
              diags->decrRefCount();

	      workMsgStream_->sendResponse();

	      return WORK_OK;
	    }
            StatsGlobals *statsGlobals = getGlobals()->getStatsGlobals();
            ExStatisticsArea *statsArea = getGlobals()->getStatsArea();
            if (statsArea != NULL)
            {
               if (statsGlobals != NULL)
               {
                  Long semId = getGlobals()->getSemId();
                  int error = statsGlobals->getStatsSemaphore(semId,
                                       getGlobals()->getPid());
                  statsArea->setDonotUpdateCounters(FALSE);
                  statsArea->restoreDop();
                  statsGlobals->releaseStatsSemaphore(semId, 
                      getGlobals()->getPid());
               }
               else
               {
                  statsArea->setDonotUpdateCounters(FALSE);
                  statsArea->restoreDop();
               }
           }
	  qSplit_.down->insert();

          EXSM_TRACE(EXSM_TRACE_WORK, "SNDB %p down queue insert", this);
	} 
    }

  return WORK_OK;
}

/////////////////////////////////////////////////////////////////////////////
// get a request buffer from the message stream
TupMsgBuffer* ex_send_bottom_tcb::getRequestBuffer()
{
  IpcMessageObjType msgType;
  while (workMsgStream_->getNextReceiveMsg(msgType))
    {
      ex_assert_both_sides(msgType == IPC_MSG_SQLESP_DATA_REQUEST,
		"received message from unknown message stream");

      while (workMsgStream_->getNextObjType(msgType))
	{
          if (sendBottomTdb().getExchangeUsesSM())
          {
            ESPMessageObjTypeEnum t = (ESPMessageObjTypeEnum) msgType;
            EXSM_TRACE(EXSM_TRACE_BUFFER, "SNDB %p RECV %s", this,
                       getESPMessageObjTypeString(t));
          }

	  if (msgType == ESP_OPEN_HDR)
	    {  // reply to open message, return ExFragInstanceHandle 
	      ExEspOpenReqHeader* reqHdr =
		new(workMsgStream_->receiveMsgObj()) 
		ExEspOpenReqHeader(workMsgStream_);
	      // the statID we get from the send-top is used in the stats as
	      // parentStatID. The real parent is of course the split bottom
	      // of this ESP. But for stats purposes we want our send top
	      // as parent.
	      if (getStatsEntry() && getStatsEntry()->castToExESPStats())
		getStatsEntry()->castToExESPStats()->setSendTopStatID(
		     reqHdr->statID_);

	      // construct reply message and send back with
	      // ExFragInstanceHandle
	      ExEspReturnStatusReplyHeader* replyHdr = new(*workMsgStream_)
		ExEspReturnStatusReplyHeader((NAMemory *) NULL);
	      replyHdr->key_ = reqHdr->key_;
	      replyHdr->handle_ = getMyHandle();
	      replyHdr->instanceState_ =
		ExEspReturnStatusReplyHeader::INSTANCE_ACTIVE;

	      workMsgStream_->sendResponse();
	    }
	  else if (msgType == ESP_CONTINUE_HDR)
	    {
	      ExEspContinueReqHeader* reqHdr =
		new(workMsgStream_->receiveMsgObj()) 
                ExEspContinueReqHeader(workMsgStream_);
	      // continue request is only needed to trigger IPC response
	    }
	  else  if (msgType == ESP_INPUT_DATA_HDR)
	    {
	      // request data header, get sql buffer and return for processing
	      ExEspInputDataReqHeader* reqHdr =
		new(workMsgStream_->receiveMsgObj()) 
                ExEspInputDataReqHeader(workMsgStream_);

	      ex_assert_both_sides(workMsgStream_->getNextObjType(msgType)  AND
			msgType == ESP_INPUT_SQL_BUFFER,
			"input data header w/o input data buffer received");
	      ex_assert_both_sides(!lateCancel_,
			"late cancel overlaps following request");

	      getGlobals()->setInjectErrorAtQueue(
		   reqHdr->injectErrorAtQueueFreq_);

	      IpcMessageObj *msgHeap = workMsgStream_->receiveMsgObj();
	      if (!msgHeap) 
		return NULL;
	      
	      TupMsgBuffer* buffer = new(msgHeap) TupMsgBuffer(workMsgStream_);
	      return buffer;

	    }
	  else if (msgType == ESP_SECURITY_INFO)
	    {
	      // ignore this message object
	      IpcMessageObj* packedObj = workMsgStream_->receiveMsgObj();
	    }
	  else
	    {
	      ex_assert_both_sides(0, "unexpected message object received");
	    }
	}
    }

  return NULL;
}

/////////////////////////////////////////////////////////////////////////////
// check for reply data in the queue and send reply message to send top
short ex_send_bottom_tcb::checkReply()
{
  ControlInfo *msgControlInfo;
  Int64 finalReplyBytes = 0;

  workMsgStream_->tickleOutputIo(); // send any buffers in the output queue
  workMsgStream_->cleanupBuffers(); // free up rec. buffers no longer in use

  ExOperStats *statsEntry;
  ExESPStats *espStats;
  
  while (NOT qSplit_.up->isEmpty() || currentReplyBuffer_)
    {
      if (currentReplyBuffer_ == NULL)
	{  // get sql buffer from message stream
	  if (workMsgStream_->sendLimitReached())
          {
            EXSM_TRACE(EXSM_TRACE_PROTOCOL, 
                       "SNDB %p send limit reached", this);
	    return WORK_OK;
          }

	  if (lateCancel_)
	    {
	      // consume replies from a late cancel request
	      ex_queue_entry* lcentry = qSplit_.up->getHeadEntry();
	      ex_assert_both_sides(
                        lcentry->upState.status == ex_queue::Q_NO_DATA,
			"late cancel should not produce data");

              ExWorkProcRetcode result;
              // Reply the cancel message
              result = replyCancel();
              if (result != WORK_OK)
                return result;   // actually WORK_POOL_BLOCKED

              cancelReplyPending_ = FALSE;

	      qSplit_.up->removeHead();
	      // no need to reply, reply has already been sent
	      lateCancel_ = FALSE;
	      finish();
	      return WORK_CALL_AGAIN;
	    }

          getReplyBuffer();
	  if (currentReplyBuffer_ == NULL)
          {
            EXSM_TRACE(EXSM_TRACE_PROTOCOL, 
                       "SNDB %p checkReply buffer not available", this);
            return WORK_POOL_BLOCKED;
          }
	}

      SqlBuffer* sqlBuf = currentReplyBuffer_->get_sql_buffer();
      sqlBuf->driveUnpack();
      statsEntry = getStatsEntry();

      NABoolean replySpaceAvailable = TRUE;
      NABoolean isEOFEntry = FALSE;
      NABoolean isStatsToBeSent = FALSE;
      const ULng32 controlInfoLen = sizeof(ControlInfo);

      while (replySpaceAvailable AND NOT qSplit_.up->isEmpty()) 
	// fill sql buffer with data from queue
	{
	  ex_queue_entry* sentry = qSplit_.up->getHeadEntry();

	  NABoolean isRowToBeSent = 
	    (sentry->upState.status == ex_queue::Q_OK_MMORE);
	  NABoolean isDiagsAreaToBeSent = sentry->getDiagsArea() != NULL;

	  // Use a method to pack ControlInfo and row into SqlBuffer.
	  // Don't let that method pack diags or stats. If we do want
	  // to send a diags area with this row, do force the method
	  // to add a specific ControlInfo for this row. Ignore stats
	  // completely, since they are not tied to a particular row.
	  Int32 moveRetcode = sqlBuf->moveInSendOrReplyData(
	       FALSE,               // we are replying with an up state
	       FALSE,               // do not force ControlInfo.
	       isRowToBeSent,
	       &(sentry->upState),
	       controlInfoLen,
	       &msgControlInfo,
	       sendBottomTdb().getUpRecordLength(),
	       NULL,
	       NULL,                // don't let this method pack a diags area
	       NULL,
	       moveOutputValues(),
	       sentry->getAtp(),
	       workAtp_,
	       workAtp_,            // work atp is destination of move
	       (unsigned short) sendBottomTdb().moveExprTuppIndex_, //dest. ix
	       FALSE, NULL,                // Stats are sent separately
	       NULL,
	       TRUE,                // diags areas are external.
	       isDiagsAreaToBeSent, //);// Already have a DA.
	       defragTd_
#if (defined(_DEBUG))
	       ,this
#endif
	       );
      
	  if (moveRetcode == SqlBuffer::BUFFER_FULL)
	    {
	      replySpaceAvailable = FALSE;
	    }
	  else
	    {
	      //
	      // If there is a diagnostics area to send, then add
	      // it to the message following the sql buffer.  Note 
	      // this test catches DAs added by moveInSendOrReplyData.
	      //
	      if (sentry->getDiagsArea())
              {
                UInt32 diagsBytes = (UInt32)
                  sentry->getDiagsArea()->packedLength();
                finalReplyBytes += diagsBytes;
                // pack a copy of ComDiagsArea into the message
                *workMsgStream_ << *(sentry->getDiagsArea());
              }
              
	      if (sentry->upState.status == ex_queue::Q_NO_DATA)
		{
                  EXSM_TRACE(EXSM_TRACE_PROTOCOL, 
                             "SNDB %p Placed EOD in reply buf", this);
		  isEOFEntry = TRUE;

                  // if this is Extract Consumer, remember that
                  // EOD is being sent and the work is done. 
                  if (isExtractConsumer_) 
		  {
		    isExtractWorkDone_ = TRUE;
		  }

		  // Notify statement globals that we reached EOD and
		  // are ready to send stats. Skip this step for send
		  // nodes servicing a parallel extract consumer
		  // query. We do not yet support stats propagation to
		  // consumer queries.
		  if (getGlobals()->getStatsArea() && !isExtractConsumer_)
                  {
                    // NOTE: the sendStats() call will increment a
                    // counter in the stats area. The counter
                    // represents the number of send bottoms that have
                    // reached EOD. The stats area also knows the
                    // total number of send bottoms excluding parallel
                    // extract consumers. sendStats() only returns
                    // TRUE to the last send bottom to reach EOD.
		    isStatsToBeSent =
                      getGlobals()->getStatsArea()->sendStats();
                  }
		}
	      else
		{
		  isEOFEntry = FALSE;
	  
		  // If detailed statistics are being collected, update
		  // the return row count now.
		  if(statsEntry)
		    statsEntry->incActualRowsReturned();
		}

	      // done with this row
	      qSplit_.up->removeHead();

	    }
	} // while (replySpaceAvailable AND NOT qSplit_.up->isEmpty()) 

      if (replySpaceAvailable AND NOT isEOFEntry)
	return WORK_OK;

      // send the buffer now. It is either full, or we reached EOF
      statsEntry = getStatsEntry();
      if (statsEntry)
        espStats = statsEntry->castToExESPStats();
      else
        espStats = NULL;

      if(espStats)
	{
	  SqlBuffer* sb = currentReplyBuffer_->get_sql_buffer();
	  espStats->bufferStats()->totalSentBytes() += sb->getSendBufferSize();
	  espStats->bufferStats()->sentBuffers().addEntry(sb->get_used_size());
     	}

      // send the stats (only if we are the last send-bottom to reach EOF

      if (isStatsToBeSent)
	{
	  ex_assert_both_sides(msgControlInfo,
		    "Should have added a new ControlInfo for stats!");
	  msgControlInfo->setIsExtStatsAreaPresent(isStatsToBeSent);

          // Special handling of reply stats for final reply -- bugzilla
          // 1586, Genesis soln 10-101116-4536.
          finalReplyBytes += currentReplyBuffer_->packedLength();
          UInt32 statsBytes = (UInt32)
            getGlobals()->getStatsArea()->packedLength();
          finalReplyBytes += statsBytes;
          getGlobals()->getStatsArea()->incReplyMsg(finalReplyBytes);
          
          StatsGlobals *statsGlobals = getGlobals()->getStatsGlobals();
	  if (statsGlobals != NULL)
          {
            Long semId = getGlobals()->getSemId();
            int error = statsGlobals->getStatsSemaphore(semId, 
                                                    getGlobals()->getPid());
	    *workMsgStream_ << *(getGlobals()->getStatsArea());
	    getGlobals()->getStatsArea()->initEntries();
            getGlobals()->getStatsArea()->setDonotUpdateCounters(TRUE);
            statsGlobals->releaseStatsSemaphore(semId, getGlobals()->getPid());
          }
          else
          {
            *workMsgStream_ << *(getGlobals()->getStatsArea());
	    getGlobals()->getStatsArea()->initEntries();
            getGlobals()->getStatsArea()->setDonotUpdateCounters(TRUE);
          }
	  isStatsToBeSent = FALSE;

          EXSM_TRACE(EXSM_TRACE_PROTOCOL, 
                     "SNDB %p stats bytes %d final %d",
                     this,
                     (int) statsBytes, (int) finalReplyBytes);
	}

      // For SeaMonster, mark the next reply buffer as "last in batch" if
      // the send bottom queues are empty
      if (sendBottomTdb().getExchangeUsesSM() &&
          qSplit_.up->isEmpty() && qSplit_.down->isEmpty())
        workMsgStream_->setSMLastInBatch();

      EXSM_TRACE(EXSM_TRACE_PROTOCOL, "SNDB %p SEND reply tupps %d",
                 this, (int) sqlBuf->getTotalTuppDescs());

      currentReplyBuffer_ = NULL;

      workMsgStream_->sendResponse();

    } // while (NOT qSplit_.up->isEmpty() || currentReplyBuffer_)
  
  if (qSplit_.up->isEmpty()   AND
      qSplit_.down->isEmpty() AND
      (workMsgStream_->numOfInputBuffers() == 0) )
    {
      EXSM_TRACE(EXSM_TRACE_PROTOCOL, "SNDB %p queues are empty", this);

      // all work is done and no more messages, lazy release of reply tags
      EXSM_TRACE(EXSM_TRACE_PROTOCOL, "SNDB %p calling responseDone", this);
      workMsgStream_->responseDone();

      EXSM_TRACE(EXSM_TRACE_PROTOCOL, 
                 "SNDB %p calling releaseBuffers", this);
      workMsgStream_->releaseBuffers();  // do final garbage collection

      if (workMsgStream_->numOfOutputBuffers() == 0 AND
	  workMsgStream_->numOfReplyTagBuffers() == 0)
      {
	finish();
      }
      else
      {
        EXSM_TRACE(EXSM_TRACE_PROTOCOL, 
                   "SNDB %p buffers stuck in data stream", this);
        EXSM_TRACE(EXSM_TRACE_PROTOCOL, 
                   "SNDB %p out %d reply tag %d", this,
                   (int) workMsgStream_->numOfOutputBuffers(),
                   (int) workMsgStream_->numOfReplyTagBuffers());

	return WORK_POOL_BLOCKED; // buffers are stuck in this msg stream
      }
    }
  
  return WORK_OK;

} // checkReply()

/////////////////////////////////////////////////////////////////////////////
// get a reply buffer from the message stream
void ex_send_bottom_tcb::getReplyBuffer()
{
  if (currentReplyBuffer_)
    return;

  // construct return data reply header in message
  ExEspReturnDataReplyHeader* hdr = 
    new(*workMsgStream_) ExEspReturnDataReplyHeader((NAMemory *) NULL);
  if (hdr == NULL)
    return;

  // construct sql buffer(TupMsgBuffer) directly in message to avoid copy
  TupMsgBuffer* result =
    new(*workMsgStream_, replyBufferSize_) TupMsgBuffer(replyBufferSize_,
							TupMsgBuffer::MSG_OUT,
							workMsgStream_);
  if (result == NULL)
    ABORT("mismatched send bottom buffer size");

  //  if (workMsgStream_->inUseLimitReached())
  //    hdr->stopSendingData_ = TRUE; // tell send top to quit sending data
    
  currentReplyBuffer_ = result;
}

/////////////////////////////////////////////////////////////////////////////
// Propagate cancel to child queue.
ExWorkProcRetcode ex_send_bottom_tcb::cancel()
{
  EXSM_TRACE(EXSM_TRACE_CANCEL, "SNDB %p BEGIN CANCEL", this);

  ExWorkProcRetcode result = WORK_OK;
  TupMsgBuffer *cancelReceiveBuffer = getCancelRequestBuffer();
   
  EXSM_TRACE(EXSM_TRACE_CANCEL, "SNDB %p cancelReceiveBuffer %p late %d", 
             this, cancelReceiveBuffer, (int) lateCancel_);
  
  if (cancelReceiveBuffer != NULL)
    {
      cancelReplyPending_ = TRUE;
      EXSM_TRACE(EXSM_TRACE_CANCEL, 
                 "SNDB %p cancelReplyPending_ TRUE", this);
  
      SqlBuffer *sb = cancelReceiveBuffer->get_sql_buffer();
      tupp_descriptor *controlInfoAsTupp;
      
      while ((controlInfoAsTupp = sb->getCurr()))
	{
	  ControlInfo msgControlInfo;
	  
	  str_cpy_all((char *) &msgControlInfo,
		      controlInfoAsTupp->getTupleAddress(),
		      sizeof(ControlInfo));

	  ex_assert_both_sides(msgControlInfo.getDownState().request == 
		    ex_queue::GET_NOMORE, 
		    "this is supposed to be a cancel buffer!");
	  sb->advance();

	  ULng32 canceledRequestsBufferNum = 
            msgControlInfo.getBufferSequenceNumber();

	  NABoolean cancelInDownQueue = FALSE;	// Soln 10-041117-1848
	  if (lateCancel_)
	    {
	      ex_assert_both_sides(qSplit_.down->isEmpty(),
			"late cancel should see empty queue in send bottom");
	      ex_queue_entry* sentry = qSplit_.down->getTailEntry();
	      
	      sentry->downState.parentIndex  = 9999999;
	      sentry->downState.requestValue = 0;
	      sentry->downState.request      = ex_queue::GET_NOMORE;
	      // Note that this GET_NOMORE request does not have
	      // a tuple to go with it. We know that the split bottom
	      // node below us will not try to use the tupps in this ATP
	      // when it gets a GET_NOMORE request.
	      qSplit_.down->insert();
	    }
	  else if (canceledRequestsBufferNum < currentBufferNumber_)
	    {
	      // work finished w/ the canceled request's buffer.
	      // Only hope to cancel is through down queue's method.
	      cancelInDownQueue = TRUE;
	    }
	  else if (canceledRequestsBufferNum > currentBufferNumber_)
	    {
	      // work has not seen this buffer yet, so we cannot use
	      // down queue's method.  Instead, add this entry to the 
	      // cancelOnSight_ list.
	      cancelOnSight_.insert(
		   msgControlInfo.getDownState().parentIndex);
	      cancelInDownQueue = FALSE;
	    }
	  else if (currentRequestBuffer_ == NULL)
	    {
	      // work()'s most recent buffer is the right one, but work()
	      // is finished with it now, so message must be in the down
	      // queue.
	      cancelInDownQueue = TRUE;
	    }
	  else 
	    {
	      // work() is currently working on this buffer.  Search it
	      // to see if the canceled request has been placed in the 
	      // down queue.  If not, calling SqlBuffer::findAndCancel
	      // will cancel it by simply changing its downstate.request to 
	      // GET_NOMORE.
	      if (currentRequestBuffer_->get_sql_buffer()->findAndCancel(
		   msgControlInfo.getDownState().parentIndex, FALSE))
		cancelInDownQueue = FALSE;
	      else 
		cancelInDownQueue = TRUE;
	    }

	  if (cancelInDownQueue)
	    qSplit_.down->cancelRequestWithParentIndex(
		 msgControlInfo.getDownState().parentIndex);
	}
    }     // if cancelReceiveBuffer 

  if (!lateCancel_ && cancelReplyPending_)
    {
      result = replyCancel();

      if (result == WORK_OK)
	cancelReplyPending_ = FALSE;
    }

  EXSM_TRACE(EXSM_TRACE_CANCEL, "SNDB %p END CANCEL rc %s", this,
             ExWorkProcRetcodeToString(result));
  return result;
}

/////////////////////////////////////////////////////////////////////////////
// get a request buffer from the cancel message stream
TupMsgBuffer* ex_send_bottom_tcb::getCancelRequestBuffer()
{
  IpcMessageObjType msgType;

  while (cancelMsgStream_->getNextReceiveMsg(msgType))
    {
      ex_assert_both_sides(msgType == IPC_MSG_SQLESP_CANCEL_REQUEST,
		"received message from unknown message stream");

      while (cancelMsgStream_->getNextObjType(msgType))
	{
	  if (msgType == ESP_CANCEL_HDR)
	    {
	      // cancel data header, get sql buffer and return for processing
	      ExEspCancelReqHeader* reqHdr =
		new(cancelMsgStream_->receiveMsgObj()) 
                ExEspCancelReqHeader(cancelMsgStream_);
	    }
	  else if (msgType == ESP_LATE_CANCEL_HDR)
	    {
	      ExEspLateCancelReqHeader* reqHdr =
		new(cancelMsgStream_->receiveMsgObj()) 
                ExEspLateCancelReqHeader(cancelMsgStream_);
	      
	      lateCancel_ = TRUE;
	    }
	  else
	    {
	      ABORT("unexpected message object received");
	    }
	  
	  ex_assert_both_sides(cancelMsgStream_->getNextObjType(msgType)  AND
		    msgType == ESP_INPUT_SQL_BUFFER,
		    "cancel header w/o input data buffer received");
	  
	  TupMsgBuffer* buffer = new(cancelMsgStream_->receiveMsgObj()) 
	    TupMsgBuffer(cancelMsgStream_);
	  
	  return buffer;
	}
    }
  
  return NULL;
}

/////////////////////////////////////////////////////////////////////////////
// make a reply to the cancel message stream
ExWorkProcRetcode ex_send_bottom_tcb::replyCancel()
{
  ExWorkProcRetcode result = WORK_OK;

  EXSM_TRACE(EXSM_TRACE_CANCEL, "SNDB %p BEGIN replyCancel", this);
  
  // send any buffers in the output queue
  cancelMsgStream_->tickleOutputIo(); 
  // free up any receive buffers no longer in use
  cancelMsgStream_->cleanupBuffers();

  // construct return data reply header in message
  ExEspCancelReplyHeader* hdr = 
    new(*cancelMsgStream_) ExEspCancelReplyHeader((NAMemory *) NULL);

  if (hdr == NULL)
  {
    EXSM_TRACE(EXSM_TRACE_CANCEL, 
               "SNDB %p replyCancel buffer not available", this);
    
    // Let caller come back later. (should this be an assert?)
    result = WORK_POOL_BLOCKED;
  }

  if (result == WORK_OK)
  {
    cancelMsgStream_->setSMLastInBatch();
    cancelMsgStream_->sendResponse();
    
    if (cancelMsgStream_->numOfInputBuffers() == 0)
    {
      EXSM_TRACE(EXSM_TRACE_CANCEL, "SNDB %p calling responseDone", this);
      cancelMsgStream_->responseDone();
    }
    
    finishCancel();
  }

  EXSM_TRACE(EXSM_TRACE_CANCEL, "SNDB %p END replyCancel rc %s", this,
             ExWorkProcRetcodeToString(result));
  return result;
}

/////////////////////////////////////////////////////////////////////////////
// Notify ESP fragment directory that the send bottom node is now active
void ex_send_bottom_tcb::start()
{
  if (!isActive_)
  {
    espInstanceDir_->startedSendBottomRequest(myHandle_);
    isActive_ = TRUE;
    EXSM_TRACE(EXSM_TRACE_PROTOCOL, 
               "SNDB %p started request, instDir reqs %d", this,
               (int) espInstanceDir_->getNumSendBottomRequests(myHandle_));
  }
}

/////////////////////////////////////////////////////////////////////////////
// Notify ESP fragment directory that the send bottom node is no longer active
void ex_send_bottom_tcb::finish()
{
  if (isActive_)
  {
    espInstanceDir_->finishedSendBottomRequest(myHandle_);
    isActive_ = FALSE;
    EXSM_TRACE(EXSM_TRACE_PROTOCOL, 
               "SNDB %p finished request, instDir reqs %d", this,
               (int) espInstanceDir_->getNumSendBottomRequests(myHandle_));
  }
}

/////////////////////////////////////////////////////////////////////////////
// Notify ESP fragment directory that the send bottom node has cancel requests
void ex_send_bottom_tcb::startCancel()
{
  espInstanceDir_->startedSendBottomCancel(myHandle_);
  EXSM_TRACE(EXSM_TRACE_CANCEL, 
             "SNDB %p started cancel, instDir cancels %d", this,
             (int) espInstanceDir_->getNumSendBottomCancels(myHandle_));
}

/////////////////////////////////////////////////////////////////////////////
// Notify ESP frag dir that the send bottom node is finished w/ cancel request
void ex_send_bottom_tcb::finishCancel()
{
  espInstanceDir_->finishedSendBottomCancel(myHandle_);
  EXSM_TRACE(EXSM_TRACE_CANCEL, 
             "SNDB %p finished cancel, instDir cancels %d", this,
             (int) espInstanceDir_->getNumSendBottomCancels(myHandle_));
}

/////////////////////////////////////////////////////////////////////////////
ex_queue_pair ex_send_bottom_tcb::getParentQueue() const
{
  // if we look at the tcb tree as a tree, then pretend that the
  // send bottom node is actually a child of the split bottom node
  return qSplit_;
}

/////////////////////////////////////////////////////////////////////////////
const ex_tcb* ex_send_bottom_tcb::getChild(Int32 pos) const
{
  ex_assert((pos >= 0), "");
  return NULL;
}

/////////////////////////////////////////////////////////////////////////////
Int32 ex_send_bottom_tcb::numChildren() const
{
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
void ex_send_bottom_tcb::routeMsg(IpcMessageStream& msgStream)
{
  msgStream.giveReceiveMsgTo(*routeMsgStream_);
}

/////////////////////////////////////////////////////////////////////////////
void ex_send_bottom_tcb::setClient(IpcConnection* connection)
{
  if (routeMsgStream_->getClient() && 
      (routeMsgStream_->getClient() != connection))
  {
    connection->dumpAndStopOtherEnd(true, false);
    ex_assert_both_sides(false, 
     "Attempt to switch send bottom's client.");
  }
  routeMsgStream_->setClient(connection);
}

/////////////////////////////////////////////////////////////////////////////
IpcConnection* ex_send_bottom_tcb::getClient()
{
  return routeMsgStream_->getClient();
}

void ex_send_bottom_tcb::incReplyMsg(Int64 msgBytes)
{
  ExStatisticsArea *statsArea;
 
  if ((statsArea = getGlobals()->getStatsArea()) != NULL)
  {
    if (statsArea->donotUpdateCounters())
      // too late to update stats -- they've already been shipped to master.
      // See bugzilla 1586, Genesis soln 10-101116-4536.
      ;
    else
      statsArea->incReplyMsg(msgBytes);
  }
 }

/////////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// Methods for class ExSendBottomRouteMessageStream
// -----------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// constructor
ExSendBottomRouteMessageStream::ExSendBottomRouteMessageStream(
                                   ExExeStmtGlobals *glob,
                                   ex_send_bottom_tcb *sendBottomTcb)
: IpcServerMsgStream(glob->getIpcEnvironment(),
                     IPC_MSG_SQLESP_SERVER_ROUTING,
                     CurrEspReplyMessageVersion,
                     0,       // sendBufferLimit
                     0,       // inUseBufferLimit
                     0        // bufferSize  (not needed, b/c doesn't send.)
                     ),
  sendBottomTcb_(sendBottomTcb)
  { }

/////////////////////////////////////////////////////////////////////////////
// method called upon send complete
void ExSendBottomRouteMessageStream::actOnSend(IpcConnection* connection)
  {
    ABORT("ExSendBottomRouteMessageStream::actOnSend should not be called!");
  }

/////////////////////////////////////////////////////////////////////////////
// method called upon receive complete
void ExSendBottomRouteMessageStream::actOnReceive(IpcConnection* connection)
{
  if (sendBottomTcb_->sendBottomTdb().getExchangeUsesSM())
    EXSM_TRACE(EXSM_TRACE_PROTOCOL, "SNDB %p ROUTE s %p c %p",
               sendBottomTcb_, this, connection);
  
  if (connection AND getErrorInfo())
    {
      ABORT("Error while receiving data");
    }


  IpcMessageObjType msgType;
  if (!getNextReceiveMsg(msgType))
    {
      EXSM_TRACE(EXSM_TRACE_PROTOCOL, 
                 "SNDB %p ROUTE no data available", sendBottomTcb_);

      // False alarm, only have some of the message.  Will be called again,
      // when the rest of the message arrives.
      return;
    }
    
  if (!getNextObjType(msgType))
    {
      ABORT("Empty message received in send_bottom");
    }

  EXSM_TRACE(EXSM_TRACE_PROTOCOL, "SNDB %p ROUTE %s", sendBottomTcb_,
             getESPMessageObjTypeString((ESPMessageObjTypeEnum) msgType));

  switch (msgType)
    {
    case ESP_OPEN_HDR:
      sendBottomTcb_->getEspFragInstanceDir()->openedSendBottom(
	   sendBottomTcb_->getMyHandle());
      // fall through to next case

    case ESP_INPUT_DATA_HDR:
      // fall through to next case

    case ESP_CONTINUE_HDR:
      // it is possible to receive a continue message when the instance
      // is inactive as the instance has done the assigned work. In this
      // case, we wake it up so that the instance could reply to this
      // message. If the instance is currenly active, the start is no-op
      // See CR 10-030922-2720 for detail.
      sendBottomTcb_->start();
      giveReceiveMsgTo(*sendBottomTcb_->workMsgStream_);        
      break;

    case ESP_LATE_CANCEL_HDR:
      // in case this was the first request that came down
      sendBottomTcb_->getEspFragInstanceDir()->openedSendBottom(
          sendBottomTcb_->getMyHandle());
      // fall through to next case

    case ESP_CANCEL_HDR:
      // the send bottom might be in the inactive state right now,
      // start scheduling it
      sendBottomTcb_->startCancel();
      giveReceiveMsgTo(*sendBottomTcb_->cancelMsgStream_);        
      break;

    default:
      ABORT("Unknown message type received in send_bottom");
    }
}

// -----------------------------------------------------------------------
// Methods for class ExSendBottomWorkMessageStream
// -----------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// constructor
ExSendBottomWorkMessageStream::ExSendBottomWorkMessageStream(
                                   ExExeStmtGlobals *glob,
                                   Lng32 sendBufferLimit,
                                   Lng32 inUseBufferLimit,
                                   IpcMessageObjSize bufferSize,
                                   ex_send_bottom_tcb *sendBottomTcb)
: IpcServerMsgStream(glob->getIpcEnvironment(),
                     IPC_MSG_SQLESP_DATA_REPLY,
                     CurrEspReplyMessageVersion,
                     sendBufferLimit,
                     inUseBufferLimit,
                     bufferSize),
  sendBottomTcb_(sendBottomTcb)
  { }

/////////////////////////////////////////////////////////////////////////////
// method called upon send complete
void ExSendBottomWorkMessageStream::actOnSend(IpcConnection* connection)
{
  if (sendBottomTcb_->sendBottomTdb().getExchangeUsesSM())
    EXSM_TRACE(EXSM_TRACE_PROTOCOL, "SNDB %p ACTS D %p c %p",
               sendBottomTcb_, this, connection);

  if (connection)
  {
    if (getErrorInfo())
    {
      ABORT("Error while replying to a data, continue, or open msg");
    }
    else
    {
      sendBottomTcb_->incReplyMsg(connection->getLastSentMsg()->getMessageLength());
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
// method called upon receive complete
void ExSendBottomWorkMessageStream::actOnReceive(IpcConnection* connection)
{
  if (sendBottomTcb_->sendBottomTdb().getExchangeUsesSM())
    EXSM_TRACE(EXSM_TRACE_PROTOCOL, "SNDB %p ACTR D %p c %p",
               sendBottomTcb_, this, connection);

  if (connection AND getErrorInfo())
    {
      ABORT("Error while receiving data");
    }

  sendBottomTcb_->tickleScheduler();
  EXSM_TRACE(EXSM_TRACE_PROTOCOL, "SNDB %p tcb scheduled", sendBottomTcb_);
}

// -----------------------------------------------------------------------
// Methods for class ExSendBottomCancelMessageStream
// -----------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// constructor
ExSendBottomCancelMessageStream::ExSendBottomCancelMessageStream(
                                   ExExeStmtGlobals *glob,
                                   ex_send_bottom_tcb *sendBottomTcb)
: IpcServerMsgStream(glob->getIpcEnvironment(),
                     IPC_MSG_SQLESP_CANCEL_REPLY,
                     CurrEspReplyMessageVersion,
                     0,       // sendBufferLimit
                     0,       // inUseBufferLimit
                     sizeof(ExEspCancelReplyHeader)        // bufferSize
                     ),
  sendBottomTcb_(sendBottomTcb)
  { }
 
/////////////////////////////////////////////////////////////////////////////
// method called upon send complete
void ExSendBottomCancelMessageStream::actOnSend(IpcConnection* connection)
{
  EXSM_TRACE(EXSM_TRACE_PROTOCOL, "SNDB %p ACTS C %p c %p",
             sendBottomTcb_, this, connection);
  
  if (connection)
  {
    if (getErrorInfo())
    {
      ABORT("Error while replying to a cancel message");
    }
    else
    {
      EXSM_TRACE(EXSM_TRACE_PROTOCOL, 
                 "ExSendBottomCancelMessageStream::actSend(): incReplyMsg" );
      if (connection->getLastSentMsg())
        sendBottomTcb_->incReplyMsg(connection->getLastSentMsg()->getMessageLength());
      else
        sendBottomTcb_->incReplyMsg(0);
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
// method called upon receive complete
void ExSendBottomCancelMessageStream::actOnReceive(IpcConnection* connection)
{
  EXSM_TRACE(EXSM_TRACE_CANCEL, "SNDB %p ACTR C %p c %p",
             sendBottomTcb_, this, connection);

  if (connection AND getErrorInfo())
    {
      ABORT("Error while receiving data");
    }

  EXSM_TRACE(EXSM_TRACE_CANCEL, 
             "ExSendBottomCancelMessageStream::actOnReceive()" );
  sendBottomTcb_->tickleSchedulerCancel();
  EXSM_TRACE(EXSM_TRACE_CANCEL, "SNDB %p tcb scheduled", sendBottomTcb_);
}
