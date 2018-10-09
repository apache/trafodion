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
 * File:         ex_send_top.C
 * Description:  Send top node (client part of client-server connection)
 *               
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

#include "BaseTypes.h"
#include "ex_stdh.h"
#include "ex_exe_stmt_globals.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ex_expr.h"
#include "str.h"
#include "ex_send_top.h"
#include "ex_send_bottom.h"
#include "ex_split_bottom.h"
#include "ex_io_control.h"
#include "ex_frag_rt.h"
#include "Ex_esp_msg.h"
#include "ComDiags.h"
#include "ExStats.h"
#include "ComTdb.h"
#include "logmxevent.h"
#include "ExCextdecs.h"
#include "Context.h"
#include "seabed/int/opts.h"

static THREAD_P short sv_max_parallel_opens = 0;

#include "ExSMTrace.h"
#include "SMConnection.h"
#include "ExSMQueue.h"

#define ex_assert_both_sides( assert_test, assert_msg )                   \
  if (!(assert_test))                                                     \
  {                                                                       \
    if (connection_)                                                      \
      connection_->dumpAndStopOtherEnd(true, false);                      \
    ex_assert(0, assert_msg);                                             \
  }
  

// -----------------------------------------------------------------------
// Methods for class ex_send_top_tdb
// -----------------------------------------------------------------------

  
ex_tcb * ex_send_top_tdb::build(ex_globals * glob)
{
  // seems like no split top node is above us, assume that we
  // are building only a single instance
  ex_assert(0,"send top w/o split top not used at this time");
  return buildInstance(glob->castToExExeStmtGlobals(),
		       0,
		       0);
}

ex_tcb * ex_send_top_tdb::buildInstance(ExExeStmtGlobals * glob,
					Lng32 myInstanceNum,
					Lng32 childInstanceNum)
{
  ex_tcb * result;

  // find out what type of connection is needed to communicate with the
  // corresponding send bottom node
  // do a lookup in the fragment table for the process id that
  // has the send bottom node

  // if this is extract consumer, create a processId from the esp phandle stored in tdb

  IpcProcessId dummyProcId;
  IpcProcessId &sendBottomProcId = dummyProcId;

  if (getExtractConsumerFlag()) 
  {
    sendBottomProcId = *(IpcProcessId *) new (glob->getSpace()) IpcProcessId(getExtractEsp());
  }
  else // normal case
  {
    sendBottomProcId = glob->getInstanceProcessId(childFragId_, childInstanceNum);
  }

  result = new(glob->getSpace()) ex_send_top_tcb(*this,
                                                 glob,
                                                 sendBottomProcId,
                                                 myInstanceNum,
                                                 childInstanceNum);
  result->registerSubtasks();

  return result;
}

// -----------------------------------------------------------------------
// Methods for class ex_send_top_tcb
// -----------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// Constructor
ex_send_top_tcb::ex_send_top_tcb(const ex_send_top_tdb& sendTopTdb,
                                 ExExeStmtGlobals* glob,
                                 const IpcProcessId& sendBottomProcId,
                                 Lng32 myInstanceNum,
                                 Lng32 childInstanceNum)
: ex_tcb(sendTopTdb,1,glob),
  ipcBroken_(FALSE),
  workAtp_(NULL),
  myInstanceNum_(myInstanceNum),
  childInstanceNum_(childInstanceNum),
  currentBufferNumber_(1),
  currentSendBuffer_(NULL),
  currentReceiveBuffer_(NULL),
  ioSubtask_(NULL),
  ioCancelSubtask_(NULL),
  connection_(NULL),
  msgStream_(NULL),
  cancelMessageStream_(NULL),
  mySendTopTcbIndex_(NULL_COLL_INDEX),
  childFragHandle_(NullFragInstanceHandle),
  bottomProcId_(sendBottomProcId)
{
  CollHeap * space = glob->getSpace();
  ExMasterStmtGlobals *masterGlob = glob->castToExMasterStmtGlobals();
  ExEspStmtGlobals *espGlob = glob->castToExEspStmtGlobals();

  // We cannot build the SeaMonster target structure until fixup. In
  // the master executor The target process ID and tag is not yet
  // known. We will zero out the structure for now and populate it
  // later inside fixup().
  smTarget_.node = 0;
  smTarget_.pid = 0;
  smTarget_.tag = 0;
  smTarget_.id = 0;

  // for non master ESPs, the trace cannot be initialized until fixup
  // if tracing is turned on using set session default.
  // trace file and level are set in the ms.env file will enable us to 
  // trace from this point..


  // If we are in the master executor and this is a parallel extract
  // producer query then we need to register the top-level ESP
  if (masterGlob != NULL && sendTopTdb.getExtractProducerFlag())
    masterGlob->insertExtractEsp(sendBottomProcId);

  // Allocate the queues to communicate with parent
  qParent_.down = new(space) ex_queue(ex_queue::DOWN_QUEUE,
				      sendTopTdb.queueSizeDown_,
				      sendTopTdb.criDescDown_,
				      space);

  // Allocate the private state in each entry of the down queue
  ex_send_top_private_state *p = 
    new(space) ex_send_top_private_state(this);
  qParent_.down->allocatePstate(p, this);
  delete p;
  
  qParent_.up = new(space) ex_queue(ex_queue::UP_QUEUE,
				    sendTopTdb.queueSizeUp_,
				    sendTopTdb.criDescUp_,
				    space);

  workAtp_ = allocateAtp(sendTopTdb.workCriDesc_, space);

  // fixup expressions
  if (moveInputValues())
    (void) moveInputValues()->fixup(0, getExpressionMode(), this,
				    glob->getSpace(), glob->getDefaultHeap(), FALSE, glob);

  nextToSendDown_ = qParent_.down->getHeadIndex();

  stTidx_ = 0;
  for (int idx = 0; idx < NumSendTopTraceElements; idx++)
  {
    // initialize the trace buffer.
    setStState(INVALID, __LINE__);
  }
  setStState(NOT_OPENED, __LINE__);

  sendBufferSize_ = sendTopTdb.getSendBufferSize();
  receiveBufferSize_ = sendTopTdb.getRecvBufferSize();
  IpcMessageObjSize maxBufSize = sendBufferSize_ > receiveBufferSize_ ? 
                                        sendBufferSize_ : receiveBufferSize_;
  maxBufSize += (sizeof(TupMsgBuffer) + sizeof(ExEspInputDataReqHeader) +
                   sizeof(ExEspOpenReqHeader));

  // temporary: ??? Larry Schumacher ???
  // Old class IpcMessageStream cannot route multi-buffer messages originating
  // from new class IpcBufferedMsgStream so we add padding to ensure that the
  // message fits in one buffer. When routing message streams have been updated
  // to class IpcBufferedMsgStream we can remove the pad. The size of the
  // message sent to send bottom is a fixed length except for a variable 
  // portion consisting of ComDiagsArea entries. Hopefully they won't exceed
  // the amount of padding added (1000 bytes)! Response messages from send
  // bottom are not routed through an old IpcMessageStream so they do not need
  // padding (unless ComDiagsArea responses become normal traffic in which case
  // we should optimize by expanding buffer size to include them).
  // Note also that a few bytes of padding will be needed because the
  // SqlBuffer inside the TupMsgBuffer gets aligned on an 8 byte boundary.
  // If class TupMsgBuffer has a size that is not a multiple of 8, then
  // the size of that space is 8 - (sizeof(TupMsgBuffer) mod 8).
  maxBufSize += 1000;

  // allocate a message stream to talk to send_bottom
  msgStream_ = new(glob->getSpace()) 
    ExSendTopMsgStream(glob,
                       getNumSendBuffers(),
                       getNumRecvBuffers(),
                       maxBufSize,
                       this);

  if (sendTopTdb.getExchangeUsesSM())
    msgStream_->setSMContinueProtocol(TRUE);

  // are the send bottom and send top nodes in the same process?
  if (masterGlob && masterGlob->getRtFragTable()->isLocal(sendBottomProcId))
    // || espGlob && $$$$ TBD: check for ESP whether it's local
    {
      // get child tdb from global fragment instance map
      //ex_tdb *childTdb = (ex_tdb *)glob->getInstanceMap()->childPtr_;
      
      // build the child (for now $$$)
      //guiChildTcb_ = childTdb->build(glob);
      
      // get the send bottom node
      // (assume only one local send bottom node $$$$)
      //localChild_ = (ex_local_send_bottom_tcb *)
      //              ((ex_split_bottom_tcb *)guiChildTcb_)->getSendNode(0);
      
      //msgStream_->setRecipient(localChild_->msgStream_);
      setStState(OPEN_COMPLETE, __LINE__);
      ABORT("local send bottom not implemented yet!"); 
    }

  if (espGlob)
    {
      // the ESP keeps track of all send tops in a given fragment instance
      // for deadlock detection related to cancels
      mySendTopTcbIndex_ = espGlob->registerSendTopTcb(this);
    }
}

/////////////////////////////////////////////////////////////////////////////
// destructor
ex_send_top_tcb::~ex_send_top_tcb()
{
  freeResources();
}

/////////////////////////////////////////////////////////////////////////////
// free all resources
void ex_send_top_tcb::freeResources()
{
  delete qParent_.up;
  qParent_.up = NULL;
  delete qParent_.down;
  qParent_.down = NULL;
  if (workAtp_)
    {
      deallocateAtp(workAtp_, getSpace());
      workAtp_ = NULL;
    }

  // We must delete the connection before deleting the message
  // stream. This will guarantee that any outstanding I/O 
  // associated with the message stream over the connection
  // will be cleaned up. If we do it in the reverse order,
  // and there is an I/O outstanding, all sorts of dangling
  // pointer scenarios are possible, resulting in abends or
  // other unpredictable behavior.

  delete connection_;
  delete msgStream_;
  delete cancelMessageStream_;
}

/////////////////////////////////////////////////////////////////////////////

ExWorkProcRetcode ex_send_top_tcb::sCancel(ex_tcb *tcb)
                      { return ((ex_send_top_tcb *) tcb)->processCancel(); }

/////////////////////////////////////////////////////////////////////////////
// register tcb for work
  
void ex_send_top_tcb::registerSubtasks()
{
  ExScheduler *sched = getGlobals()->getScheduler();

  // register events for parent queues
  ex_assert(qParent_.down && qParent_.up,"Parent queues must exist");
  sched->registerInsertSubtask(ex_tcb::sWork, this, qParent_.down, "WK");
  sched->registerCancelSubtask(sCancel, this, qParent_.down,"CN");
  sched->registerUnblockSubtask(ex_tcb::sWork,this, qParent_.up);

  // register a non-queue event for the IPC with the send top node
  ioSubtask_ = sched->registerNonQueueSubtask(sWork,this);
  ioCancelSubtask_ = sched->registerNonQueueSubtask(sCancel,this);
}

/////////////////////////////////////////////////////////////////////////////
// TCB fixup
Int32 ex_send_top_tcb::fixup()
{
  if (sendTopTdb().getExchangeUsesSM())
  {
    // The purpose of this block is to populate the SeaMonster target
    // structure. In the master executor the process ID is not known
    // during the TCB constructor so we discover the value here.

    ExExeStmtGlobals *glob = getGlobals()->castToExExeStmtGlobals();
    ExEspStmtGlobals *espGlob = glob->castToExEspStmtGlobals();
    
    // Find the SeaMonster query ID and tag. A lookup is performed
    // with the child fragment number and the send top's instance
    // number
    int myFrag = (espGlob ? (int) espGlob->getMyFragId() : 0);
    int myInstNum = (int) myInstanceNum_;
    int childFrag = (int) sendTopTdb().getChildFragId();
    int childInstNum = (int) childInstanceNum_;
    int smTag = (int) sendTopTdb().getSMTag();
    Int64 smQueryID = glob->getSMQueryID();

    // Find the send bottom's node and pid    
    const GuaProcessHandle &phandle = bottomProcId_.getPhandle();
    Int32 otherCPU, otherPID, otherNode;
    SB_Int64_Type seqNum = 0;
    phandle.decompose2(otherCPU, otherPID, otherNode
                       , seqNum
                      );

    // Store SeaMonster information in the TCB and in IPC objects
    // Seaquest NodeId == NV CpuNum
    smTarget_.node = ExSM_GetNodeID(otherCPU); 
    smTarget_.pid = otherPID;
    smTarget_.tag = smTag;
    smTarget_.id = smQueryID;
    ex_assert(smTarget_.id > 0, "Invalid SeaMonster query ID");

    int downRowLen = (int) sendTopTdb().getDownRecordLength();
    int upRowLen = (int) sendTopTdb().getUpRecordLength();
    
    int downQueueLen = (int) sendTopTdb().queueSizeDown_;
    int upQueueLen = (int) sendTopTdb().queueSizeUp_;
    
    EXSM_TRACE(EXSM_TRACE_INIT|EXSM_TRACE_TAG, "SNDT FIXUP %p", this);
    EXSM_TRACE(EXSM_TRACE_INIT|EXSM_TRACE_TAG, "SNDT  frag %d inst %d",
               myFrag, myInstNum);
    EXSM_TRACE(EXSM_TRACE_INIT|EXSM_TRACE_TAG, "SNDT  child frag %d inst %d",
               childFrag, childInstNum);

    // regress/executor/TEST123 expects this line of output when
    // tracing is reduced to tags only
    EXSM_TRACE(EXSM_TRACE_INIT|EXSM_TRACE_TAG,
               "SNDT  tgt %d:%d:%" PRId64 ":%d",
               (int) smTarget_.node, (int) smTarget_.pid, smTarget_.id,
               (int) smTarget_.tag);

    EXSM_TRACE(EXSM_TRACE_INIT, "SNDT  work stream %p", msgStream_);
    EXSM_TRACE(EXSM_TRACE_INIT, "SNDT  cancel stream %p",
               cancelMessageStream_);
    EXSM_TRACE(EXSM_TRACE_INIT, "SNDT  send limit %d, in use limit %d",
               (int) msgStream_->getSendBufferLimit(),
               (int) msgStream_->getInUseBufferLimit());
    EXSM_TRACE(EXSM_TRACE_INIT,
               "SNDT  send size %d", (int) sendBufferSize_);
    EXSM_TRACE(EXSM_TRACE_INIT,
               "SNDT  recv size %d", (int) receiveBufferSize_);
    EXSM_TRACE(EXSM_TRACE_INIT,
               "SNDT  stream max buf %d", (int) msgStream_->getBufferSize());
    EXSM_TRACE(EXSM_TRACE_INIT, "SNDT  send bufs %d recv bufs %d",
               (int) getNumSendBuffers(), (int) getNumRecvBuffers());
    EXSM_TRACE(EXSM_TRACE_INIT, "SNDT  row len %d down, %d up", 
               downRowLen, upRowLen);
    EXSM_TRACE(EXSM_TRACE_INIT, "SNDT  down queue %d, up queue %d",
               downQueueLen, upQueueLen);

  } // if exchange uses SM

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// tcb work method for queue processing
short ex_send_top_tcb::work()
{
  EXSM_TRACE(EXSM_TRACE_WORK,"SNDT %p BEGIN WORK", this);

  short result = WORK_OK;

  if (ipcBroken_)
  {
    result = WORK_BAD_ERROR;
  }
  else
  {
    result = checkReceive();
    EXSM_TRACE(EXSM_TRACE_WORK,"SNDT %p checkReceive rc %s",
               this, ExWorkProcRetcodeToString(result));
    
    if (result == WORK_OK)
    {
      result = checkSend();
      EXSM_TRACE(EXSM_TRACE_WORK,"SNDT %p checkSend rc %s",
                 this, ExWorkProcRetcodeToString(result));
    }

    if (sendTopState_ != WAITING_FOR_OPEN_COMPLETION)
    {
      if (result == WORK_OK)
      {
        result = continueRequest();
        EXSM_TRACE(EXSM_TRACE_WORK,"SNDT %p continueRequest rc %s",
                   this, ExWorkProcRetcodeToString(result));
      }

      if (qParent_.down->isEmpty())
      {
        msgStream_->releaseBuffers();  // do final garbage collection

        if (sendTopTdb().getExchangeUsesSM())
        {
          ExExeStmtGlobals *glob = getGlobals()->castToExExeStmtGlobals();
          EXSM_TRACE(EXSM_TRACE_WORK,"SNDT %p down queue empty", this);
          EXSM_TRACE(EXSM_TRACE_WORK,"SNDT %p glob msgs %d cancels %d", this,
                     (int) glob->numSendTopMsgesOut(),
                     (int) glob->numCancelMsgesOut());
          if (msgStream_)
            EXSM_TRACE(EXSM_TRACE_WORK,"SNDT %p data resp pend %d", this,
                       (int) msgStream_->numOfResponsesPending());
          if (cancelMessageStream_)
            EXSM_TRACE(EXSM_TRACE_WORK,"SNDT %p cancel resp pend %d", this,
                       (int) cancelMessageStream_->numOfResponsesPending());
        }
      }
    }
  }
  
  EXSM_TRACE(EXSM_TRACE_WORK, "SNDT %p END WORK %s %s", this,
             ExWorkProcRetcodeToString(result),
             getExSendTopStateString(sendTopState_));

  return result;
}

/////////////////////////////////////////////////////////////////////////////
// check for response data from send bottom and put in up queue
short ex_send_top_tcb::checkReceive()
{
  ExOperStats *statsEntry;
  ExESPStats *espStats;
  
         
  while (NOT qParent_.up->isFull())
    {  // process data from send bottom
      if (ipcBroken_)
	return WORK_BAD_ERROR;

      if (qParent_.down->isEmpty())
	{
	  // No more requests from the parent. This should mean
	  // that there are no more data from the send bottom.
	  // Clean up any remaining exhausted or empty receive buffers.
	  do
	    {
	      if (currentReceiveBuffer_)
		{
		  SqlBuffer* sqlBuf =
		    currentReceiveBuffer_->get_sql_buffer();
                  ex_assert_both_sides(sqlBuf->atEOTD(),
			    "Receiving extra rows from send bottom");
		}
	      currentReceiveBuffer_ = getReceiveBuffer();
	    }
	  while (currentReceiveBuffer_);

	  // done, we might be called again if more empty receive buffers or
	  // new requests arrive
	  return WORK_OK;
	}

      // the corresponding request entry in the down queue
      queue_index rindex     = qParent_.down->getHeadIndex();
      ex_queue_entry *rentry = qParent_.down->getQueueEntry(rindex);
      ex_send_top_private_state & pstate =
	*((ex_send_top_private_state *) rentry->pstate);

      // get sql buffer from message stream
      if (currentReceiveBuffer_ == NULL)
	{
	  currentReceiveBuffer_ = getReceiveBuffer();
	  if (currentReceiveBuffer_ == NULL)
          {
            if (ipcBroken_)
              return WORK_BAD_ERROR;
            else
	      return WORK_OK;
          }

          SqlBuffer *sb = currentReceiveBuffer_->get_sql_buffer();
          ex_assert(sb, "Invalid SqlBuffer pointer");
          sb->driveUnpack();

          EXSM_TRACE(EXSM_TRACE_WORK, "SNDT %p tupps arrived %d", this,
                     (int) sb->getTotalTuppDescs());
          
          statsEntry = getStatsEntry();
          if (statsEntry)
            espStats = statsEntry->castToExESPStats();
          else
            espStats = NULL;
          
	  // we got a buffer. Update statistics
	  if (espStats)
          {
            espStats->bufferStats()->totalRecdBytes() +=
              sb->getSendBufferSize();
            espStats->bufferStats()->recdBuffers().addEntry(
                sb->get_used_size());
          }
	}

      int numRowsReturned = 0;

      SqlBuffer* currSqlBuffer = currentReceiveBuffer_->get_sql_buffer();
      currSqlBuffer->driveUnpack();

      statsEntry = getStatsEntry();
      
      while (NOT qParent_.up->isFull())
	{  // move data from sql buffer to up queue

	  // data describing the row coming back
	  tupp nextTupp;
	  ControlInfo *ci;
	  NABoolean currBufferIsEmpty;
	  ComDiagsArea* diagsArea;

	  // the entry in the up-queue to be filled
	  ex_queue_entry* pentry = qParent_.up->getTailEntry();

	  // get the next row out of currSqlBuffer
	  currBufferIsEmpty = currSqlBuffer->moveOutSendOrReplyData(
	       FALSE,
	       &(pentry->upState),
	       nextTupp,
	       &ci,
	       &diagsArea, // will never be set not a non-NULL value
	       NULL);
	     
	  if (currBufferIsEmpty)
	    {
	      currentReceiveBuffer_ = NULL;
              if (sendTopTdb().logDiagnostics())
                {
                  // 2^18 = about .25M, just needs to be 
                  // very infrequent and efficient
                  if (((pstate.matchCount_++) % 2^18) == 0)
                    {
                      char msg[1024];
                      str_sprintf(msg, "Send top returning row # %d.", 
                                 (Lng32) pstate.matchCount_);
                      SQLMXLoggingArea::logExecRtInfo(NULL, 0, msg, 
                                          sendTopTdb().getExplainNodeId());
                    }
                }
	      break;
	    }

	  // check for a diags area coming back with this row
	  if (ci->getIsExtDiagsAreaPresent())
	    {
	      IpcMessageObjType msgType;
               ex_assert_both_sides(msgStream_->getNextObjType(msgType)  AND
			msgType == IPC_SQL_DIAG_AREA,
			"no diags areas in message");
	      // construct a copy of diags area from message object
	      // and put it in the up queue entry
	      diagsArea = 
		ComDiagsArea::allocate (getGlobals()->getDefaultHeap());

	      *msgStream_ >> *diagsArea;
	    }
	  else
	    diagsArea = NULL;

	  if ((rentry->downState.request == ex_queue::GET_NOMORE) &&
	      (pentry->upState.status != ex_queue::Q_NO_DATA))
	    {
	      // Don't reply to up-queue. This request has been canceled.
	      // Ignore replies until Q_NO_DATA.
	      continue;
	    }

	  // fix index fields in the up state (state has already been set)
	  pentry->upState.downIndex = rindex;
	  pentry->upState.parentIndex = rentry->downState.parentIndex;
	  // no need to fiddle with matchNo, the sender of the message
	  // has done the right thing already

	  // pass the Tupp up to the parent queue: copy request ATP and
	  // append the tupp that we got from the message
	  pentry->copyAtp(rentry);
	  pentry->getTupp(rentry->criDesc()->noTuples()) = nextTupp;
          pentry->setDiagsArea(diagsArea);
	  nextTupp.release();

	  // if stats were returned, unpack and merge them to the 
	  // stat area.
	  if (ci->getIsExtStatsAreaPresent())
	    {
	      IpcMessageObjType msgType;
              ex_assert_both_sides(msgStream_->getNextObjType(msgType)  AND
			msgType == IPC_SQL_STATS_AREA,
			"no stats area in message");
	      
              CollHeap * defaultHeap = getGlobals()->getDefaultHeap();

	      ExStatisticsArea* statArea = new(defaultHeap)
		ExStatisticsArea(defaultHeap, 0,
				 ((ComTdb*)getTdb())->getCollectStatsType());
	      
	      *msgStream_ >> *statArea;
	      if ( getGlobals()->statsEnabled() )
		mergeStats(statArea);

              EXSM_TRACE(EXSM_TRACE_WORK,"SNDT %p merged stats", this);

              // we can get rid of the received area now
              NADELETE(statArea, ExStatisticsArea, defaultHeap);
	    }
	  
	  // If statistics are being collected, update the number of
	  // rows returned now.
	  if(pentry->upState.status != ex_queue::Q_NO_DATA)
          {
            if(statsEntry)
	      statsEntry->incActualRowsReturned();
	  }

	  qParent_.up->insert();
          numRowsReturned++;

	  // if this is EOF then we are done with the request
	  if (pentry->upState.status == ex_queue::Q_NO_DATA)
	    {
              EXSM_TRACE(EXSM_TRACE_WORK,"SNDT %p returned Q_NO_DATA", this);

	      ex_assert_both_sides( (pstate.step_ == STARTED_) ||
			 (pstate.step_ == CANCELED_AFTER_SENT_), 
			 "Send top responding incorrectly!");

	      pstate.step_ = NOT_STARTED_;

	      qParent_.down->removeHead();

	      // rentry and pstate have become invalid, exit to outer loop
	      break;
	    }
	}   // while parent up queue not full

        EXSM_TRACE(EXSM_TRACE_WORK,
                   "SNDT %p queue entries returned: %d", 
                   this, numRowsReturned);
      if (qParent_.up->isFull())
        EXSM_TRACE(EXSM_TRACE_WORK, "SNDT %p up queue is full", this);

    }   // while parent down queue not empty and parent up queue not full

  // At this point we are either waiting for another buffer from the
  // send bottom node or the up queue is full. In both cases an event
  // will wake us up if there is more work.
  return WORK_OK;
}

/////////////////////////////////////////////////////////////////////////////
// get a receive buffer from the message stream
TupMsgBuffer* ex_send_top_tcb::getReceiveBuffer()
{
  IpcMessageObjType msgType;
  while (msgStream_->getNextReceiveMsg(msgType))
    {
      ex_assert_both_sides(msgType == IPC_MSG_SQLESP_DATA_REPLY,
		"received message from unknown message stream");
    
      while (msgStream_->getNextObjType(msgType))
	{
	  if (msgType == ESP_RETURN_STATUS_HDR)
	    {  // reply to open message, save child ExFragInstanceHandle 
	      
	      // only a reply to an OPEN can be of this message type
	      ex_assert_both_sides(sendTopState_ == WAITING_FOR_OPEN_REPLY,
			"received status reply after opening send bottom");
	      
	      ExEspReturnStatusReplyHeader* statHdr =
		new(msgStream_->receiveMsgObj())
		ExEspReturnStatusReplyHeader(msgStream_);
	      
	      childFragHandle_ = statHdr->handle_;
	      setStState(OPEN_COMPLETE, __LINE__);
            }
          // check for a diagnostics area, returned from server during OPEN
          else if (msgType == ESP_DIAGNOSTICS_AREA)
            {
              // construct a copy of diags area from message object
	      ComDiagsArea* diagsArea = 
	          ComDiagsArea::allocate (getGlobals()->getDefaultHeap());

              Lng32 objSize = msgStream_->getNextObjSize();

              // The diagsArea could have come packed from buffered or unbuffered stream.
	      // If the sender used unbuffered stream, it will send only the header first, 
	      // followed by the actual diagsArea. If the sender used buffered stream,
	      // it will only send the actual diags area. 
	      if (objSize == sizeof(IpcMessageObj)) 
	        {
	          IpcMessageObj* packedObj = msgStream_->receiveMsgObj(); // ignore the header
	          packedObj = msgStream_->receiveMsgObj();
                  ex_assert_both_sides(packedObj, 
                    "error receiving diags area from unbuffered stream");
	          diagsArea->unpackObj(packedObj->getType(),
	                              packedObj->getVersion(),
			              TRUE,
			              packedObj->getObjLength(),
			              (const char *)packedObj);		
	        } 
	      else 
	        {
	          *msgStream_ >> *diagsArea;  
	        }

              // check for connection errors
	      if (diagsArea->getNumber(DgSqlCode::ERROR_) > 0) 
	        {
	          ipcBroken_ = TRUE;
	        }

	      // merge the returned diagnostics area with the main one
	      if (getGlobals()->castToExExeStmtGlobals()->getDiagsArea())
	        {
		  getGlobals()->castToExExeStmtGlobals()->
		    getDiagsArea()->mergeAfter(*diagsArea);
		  // clean up
		  diagsArea->deAllocate();
		}
	      else
		{
	          getGlobals()->castToExExeStmtGlobals()->
		    setGlobDiagsArea(diagsArea);
                  diagsArea->decrRefCount();
                }
	    }
	  else if (msgType == ESP_RETURN_DATA_HDR)
	    {  // reply data header, get sql buffer and return for processing
	      ExEspReturnDataReplyHeader* dataHdr =
		new(msgStream_->receiveMsgObj())
		ExEspReturnDataReplyHeader(msgStream_);
	      
	      ex_assert_both_sides(msgStream_->getNextObjType(msgType)  AND
			msgType == ESP_OUTPUT_SQL_BUFFER,
			"output data header w/o output data buffer received");
	      
	      if (dataHdr->stopSendingData_)
		setStState(SERVER_SATURATED, __LINE__);
	      else
		setStState(OPEN_COMPLETE, __LINE__);
	      
	      TupMsgBuffer *buffer = 
		new(msgStream_->receiveMsgObj()) TupMsgBuffer(msgStream_);
	      
	      return buffer;
	    }
	  else
	    {
	      ex_assert_both_sides(0, "Unexpected reply received from ESP");
	    }
	}
    }
  return NULL;
}

/////////////////////////////////////////////////////////////////////////////
// check for data in the down queue to send to send bottom
short ex_send_top_tcb::checkSend()
{
  ExEspStmtGlobals *espGlobals =
    getGlobals()->castToExExeStmtGlobals()->castToExEspStmtGlobals();

  // free up any receive buffers no longer in use
  msgStream_->cleanupBuffers();

  if (sendTopState_ == SERVER_SATURATED OR 
      sendTopState_ == WAITING_FOR_OPEN_REPLY)
    return WORK_OK; // send continue messages only

  while (qParent_.down->entryExists(nextToSendDown_))
    {  // build sql buffers and send to send bottom
      if (ipcBroken_)
	return WORK_BAD_ERROR;

      // Need to handle canceled entries here in the outer loop
      // (which executes on the first queue entry, and whenever
      // we get a new buffer), as well as in the inner loop (which
      // handles all the queue entries that fit in one buffer) below.
      ex_queue_entry* pentry = qParent_.down->getQueueEntry(nextToSendDown_);

      ex_send_top_private_state & pstate 
	= *((ex_send_top_private_state *) pentry->pstate);

      ex_assert(pstate.step_ == NOT_STARTED_, 
              "Message shoulda been NOT_STARTED_");

      if (espGlobals)
	espGlobals->setSendTopTcbActivated(mySendTopTcbIndex_);

      // NOTE: the down request we are working on may be a GET_NOMORE
      // request.  We could optimize the logic here by not sending a
      // GET_NOMORE request down. However, that would just trigger
      // some "late cancel" logic in the split bottom node above us
      // (if applicable). Cancels in this parallel data stream are
      // best dealt with in the split bottom node below us, therefore
      // we'll send this request down even if it is a cancel.
      
      // flow control: don't send any more requests down if
      // there are too many outstanding requests or if we
      // have too many buffers in use
      if (msgStream_->sendLimitReached())
	return WORK_OK;
      if (msgStream_->inUseLimitReached())
	return WORK_POOL_BLOCKED;
      
      // Am I being asked to refrain from new I/O?
      if (getGlobals()->castToExExeStmtGlobals()->noNewRequest())
      {
        // If my root is split_bottom, it will use its list of 
        // activated send tops reschedule me when I/O can resume.
        // See the call to setSendTopTcbActivated above.
        return WORK_OK;
      }

      // get sql buffer from message stream
      currentSendBuffer_ = getSendBuffer();

      if (currentSendBuffer_ == NULL)
      {
        if (ipcBroken_)
        {
          return WORK_BAD_ERROR;
        }
        else
        {
          if (sendTopState_ == WAITING_FOR_OPEN_COMPLETION)
	    return WORK_OK;
          else
	    return WORK_POOL_BLOCKED;
        }
      }

      SqlBuffer* sqlBuf = currentSendBuffer_->get_sql_buffer();
      
      NABoolean sendSpaceAvailable = TRUE;
      do  // fill sql buffer with data from down queue
	{
	  ex_queue_entry* pentry =
	    qParent_.down->getQueueEntry(nextToSendDown_);
	  
	  ex_send_top_private_state & pstate 
	    = *((ex_send_top_private_state *) pentry->pstate);
	  
	  ex_assert(pstate.step_ == NOT_STARTED_, 
		    "Message shoulda been NOT_STARTED_");
	  
	  //
	  // Allocate space in buffer for control info (down state) and
	  // input row, if one is present
	  //
	  tupp_descriptor* controlInfoAsTupp = 
	    sqlBuf->add_tuple_desc((unsigned short) sizeof(ControlInfo));
	  
	  NABoolean isInputRowToBeSent = (moveInputValues() != NULL);
	  
	  tupp_descriptor* rowToSend =
	    isInputRowToBeSent ?
	    sqlBuf->add_tuple_desc(sendTopTdb().getDownRecordLength()) :
	    NULL;
	  
	  NABoolean isDiagsAreaToBeSent = pentry->getDiagsArea() != NULL;
	  //
	  // Check that there was sufficient space in buffer.
	  //
	  sendSpaceAvailable  = (controlInfoAsTupp != NULL)    AND 
	    (!isInputRowToBeSent OR rowToSend != NULL);
	  if (!sendSpaceAvailable)
	    {
	      //
	      // Sufficient space is not available in buffer.  Deallocate
	      // control info tupp_descriptor, if non-null.
	      //
	      if (controlInfoAsTupp) sqlBuf->remove_tuple_desc();
	      if (rowToSend)         sqlBuf->remove_tuple_desc();
	    }
	  else
	    {
	      //
	      // There was space in the send buffer --- copy the current input
	      // row into it and advance to the next input queue entry.
	      //
	      ControlInfo msgControlInfo;
	      // 
	      // Copy the down state and flags into first tupp allocated
	      // in the message buffer.
	      //
	      msgControlInfo.getDownState() = pentry->downState;
	      msgControlInfo.getDownState().parentIndex = nextToSendDown_;
	      msgControlInfo.setIsDataRowPresent(isInputRowToBeSent);
	      msgControlInfo.setIsExtDiagsAreaPresent(isDiagsAreaToBeSent);
	      
	      str_cpy_all(controlInfoAsTupp->getTupleAddress(),
			  (char *) &msgControlInfo,
			  sizeof(ControlInfo));
	      
	      // SqlBuffer::findAndCancel may need this.
	      controlInfoAsTupp->setControlTupleType();
	      
	      if (isInputRowToBeSent)
		{
		  
		  // SqlBuffer::findAndCancel may need this.
		  controlInfoAsTupp->setDataTupleType();
		  //
		  // Evaluate the move input expression, copying the
		  // actual input row into the second tupp allocated
		  // in the message buffer.
		  // 
		  workAtp_->getTupp((unsigned short)
			    sendTopTdb().moveExprTuppIndex_) = rowToSend;
		  
		  ex_expr::exp_return_type retval =
		    moveInputValues()->eval(pentry->getAtp(),workAtp_);
		  ex_assert(retval != ex_expr::EXPR_ERROR,
			    "Add error handling");
		  
		  // release the tupp_descriptor in the buffer
		  workAtp_->getTupp((unsigned short)
                            sendTopTdb().moveExprTuppIndex_) = NULL;
		}
	      //
	      // If there is a diagnostics area to send, then add
	      // it to the message following the sql buffer.
	      //
	      if (isDiagsAreaToBeSent)
		{  // construct a copy of ComDiagsArea in message
		  *msgStream_ << *(pentry->getDiagsArea());
		}
	      
	      // this one is sent down (ok, packed into the buffer),
	      // so remember its state and buffer number (in case 
	      // we cancel later) and advance to the next down queue entry
	      
	      pstate.step_ = STARTED_;
	      pstate.bufferNumber_ = currentBufferNumber_;
              pstate.matchCount_ = 0;
	      nextToSendDown_++;
	    }
	} while (sendSpaceAvailable AND
		 qParent_.down->entryExists(nextToSendDown_));
      
      // send sql buffer even if not completely full  
      ex_assert(NOT currentSendBuffer_->get_sql_buffer()->isEmpty(),
                "Send top's sql buffer is empty");
      
      // indicate if stats are enabled or not
      sqlBuf->setStatsEnabled(getGlobals()->statsEnabled());

      currentSendBuffer_ = NULL;
      
      ExOperStats *statsEntry;

      if ((statsEntry = getStatsEntry()) != NULL)
      {
	ExESPStats *stats = statsEntry->castToExESPStats();
        if (stats)
        {
	  stats->bufferStats()->totalSentBytes() +=
	  sqlBuf->getSendBufferSize();
          stats->bufferStats()->sentBuffers().addEntry(sqlBuf->get_used_size());
        }
      }
      
      // pack sql buffer
      sqlBuf->drivePack();
      
      EXSM_TRACE(EXSM_TRACE_WORK,"SNDT %p SEND data %d tupps",
                 this, (int) sqlBuf->getTotalTuppDescs());

      msgStream_->sendRequest();
      currentBufferNumber_++;
    }
  
  return WORK_OK;
}

/////////////////////////////////////////////////////////////////////////////
// get a send buffer from the message stream
TupMsgBuffer* ex_send_top_tcb::getSendBuffer()
{
  if (sendTopState_ == NOT_OPENED || 
      sendTopState_ == WAITING_FOR_OPEN_COMPLETION)
    {
      // Don't even try to open the connection before the child process
      // isn't ready. If this is the master, check for the run-time fragment
      // directory to be in the "READY" state. If this is an ESP, it's ok
      // to go ahead and open, the other ESP must have received the load
      // message for its fragment instance by now, since the master has
      // already sent us data via its send top nodes.

      // The above argument does not apply for parallel extract consumer.
      ExExeStmtGlobals* glob = getGlobals()->castToExExeStmtGlobals();
      ExMasterStmtGlobals* masterGlob = glob->castToExMasterStmtGlobals();

      if (sendTopState_ == NOT_OPENED)
      {
	if (!sendTopTdb().getExtractConsumerFlag()) 
	{
	  if (masterGlob AND
	      masterGlob->getRtFragTable() AND
	      masterGlob->getRtFragTable()->getState() != ExRtFragTable::READY)
	    return NULL;     // retry later
	}

	// the connection may already be created thru late cancel.
	// if so, there is no need to create another one.
	if (connection_ == NULL)
	  if (createConnectionToSendBottom() != 0 ||
#ifdef NA_LINUX_DISABLE // multi fragment esp
	      sendTopState_ == NOT_OPENED ||
#endif
	      sendTopState_ == WAITING_FOR_OPEN_COMPLETION)
            return NULL;
      }
      if (sendTopState_ == WAITING_FOR_OPEN_COMPLETION)
      {
	if (createConnectionToSendBottom() != 0) // Finish creating connection
	  return NULL;
      }

      EXSM_TRACE(EXSM_TRACE_BUFFER,
                 "SNDT %p about to allocate msg buffer", this);

      msgStream_->addRecipient(connection_);

      // construct open request in message, 
      // ask child ESP to respond with ExFragInstanceHandle
      ExEspOpenReqHeader *openReq = 
	new(*msgStream_) ExEspOpenReqHeader((NAMemory *) NULL);
      if (openReq == NULL)
	return NULL;

      // if this is parallel extract consumer sendTop, send the security info too
      if (sendTopTdb().getExtractConsumerFlag()) 
      {
        openReq->myInstanceNum_ = 0;
        openReq->statID_ = 0;
        openReq->setOpenType(ExEspOpenReqHeader::PARALLEL_EXTRACT);

        // Send the securityInfo object along with the OPEN
        // message. The user name we put in the message must not have
        // a ",<password>" suffix. If the user name we get from the
        // context does have the password suffix, we strip it off.
        IpcEnvironment *ipcEnv = 
          getGlobals()->castToExExeStmtGlobals()->getIpcEnvironment();
        CollHeap *ipcHeap = ipcEnv->getHeap();

        ExMsgSecurityInfo *secInfo = new (ipcHeap) ExMsgSecurityInfo(ipcHeap);

        const char *key = sendTopTdb().getExtractSecurityKey();
        Int32 len = str_len(key);
        char *copyOfKey = (char *) ipcHeap->allocateMemory(len + 1);
        str_cpy_all(copyOfKey, key, len + 1);
        secInfo->setSecurityKey(copyOfKey);

        ContextCli *context = masterGlob->getContext();
        ex_assert(context, "Invalid ContextCli pointer");

        // We need to retrieve a user identifier from ContextCli. On
        // Linux it will be the 32-bit integer and we will send the
        // string representation of that integer. On other platforms
        // it will be a user name.

        // NOTE: The user ID for the extract security check is
        // currently sent and compared as a C string. On Linux it is
        // possible to send and compare integers which would lead to
        // simpler code. The code to send/compare strings is still
        // used because it works on all platforms.

        const char *idToSend = NULL;

        Int32 *userID = context->getDatabaseUserID();
        Int32 userAsInt = *((Int32 *) userID);
        char userIDBuf[32];
        sprintf(userIDBuf, "%d", (int) userAsInt);
        idToSend = &(userIDBuf[0]);

        ex_assert(idToSend, "Could not retrieve user ID from ContextCli");

        char *copyOfId = NULL;

        // On platforms other than Linux the user identifer can be a
        // "username,password" string. We only want to send the
        // characters before the comma.
        char *locationOfComma = NULL;

        if (locationOfComma == NULL)
        {
          len = str_len(idToSend);
          copyOfId = (char *) ipcHeap->allocateMemory(len + 1);
          str_cpy_all(copyOfId, idToSend, len + 1);
          secInfo->setAuthID(copyOfId);
        }
        else
        {
          len = locationOfComma - idToSend;
          copyOfId = (char *) ipcHeap->allocateMemory(len + 1);
          str_cpy_all(copyOfId, idToSend, len);
          copyOfId[len] = 0;
          secInfo->setAuthID(copyOfId);
        }

        *msgStream_ << *secInfo;
        secInfo->decrRefCount();
        secInfo = NULL;
      }
      else // normal case, this is not an extract consumer
      {
        openReq->key_ = glob->getFragmentKey(sendTopTdb().getChildFragId());
        openReq->myInstanceNum_ = getMyInstanceNum();
        openReq->statID_ = 0; // not used at this point
      }

      setStState(WAITING_FOR_OPEN_REPLY, __LINE__);
    }
  else if ((sendTopState_ == OPEN_COMPLETE) &&
           (msgStream_->getRecipients().isEmpty()))
    {
      // This can happen if query has been executed once before and this 
      // send_top did a late cancel.
      msgStream_->addRecipient(connection_);
    }

  // construct Input data request header in message,
  // include child ExFragInstanceHandle for fast routing
  ExEspInputDataReqHeader *hdr = 
    new(*msgStream_) ExEspInputDataReqHeader((NAMemory *) NULL);
  if (hdr == NULL)
    return NULL;

  hdr->handle_ = childFragHandle_;
  hdr->myInstanceNum_ = getMyInstanceNum();
  hdr->injectErrorAtQueueFreq_ = getGlobals()->getInjectErrorAtQueue();
#ifdef _DEBUG
  if (getenv("TEST_ERROR_AT_QUEUE_NO_ESP"))
    hdr->injectErrorAtQueueFreq_  = 0;
#endif

  // construct sql buffer(TupMsgBuffer) directly in message to avoid copy
  TupMsgBuffer *result =
    new(*msgStream_, sendBufferSize_) TupMsgBuffer(sendBufferSize_,
                                                   TupMsgBuffer::MSG_IN,
				                   msgStream_);
  if (result == NULL)
    ABORT("mismatched send top buffer size");

  return result;
}

/////////////////////////////////////////////////////////////////////////////
// pre-send continue requests in anticipation of many reply data messages
short ex_send_top_tcb::continueRequest()
{
  if (qParent_.down->isEmpty() OR
      sendTopState_ == WAITING_FOR_OPEN_REPLY OR
      sendTopState_ == NOT_OPENED OR
      sendTopState_ == WAITING_FOR_OPEN_COMPLETION)
    return WORK_OK;

  if (msgStream_->sendLimitReached())
    EXSM_TRACE(EXSM_TRACE_CONTINUE,"SNDT %p send limit reached, rp %d", this,
               (int) msgStream_->numOfResponsesPending());
  
  // send continue requests until the limit for outstanding requests
  // or for incoming and used buffers is reached
  while (NOT msgStream_->sendLimitReached())
    {
      if (ipcBroken_)
	return WORK_BAD_ERROR;

      if (msgStream_->inUseLimitReached())
	{
          EXSM_TRACE(EXSM_TRACE_CONTINUE,
                     "SNDT %p CREQ in use limit reached, rp %d", this,
                     (int) msgStream_->numOfResponsesPending());

	  // We can't send another continue request down because there
	  // is no room up here to take the reply.  Return a status that
	  // causes this task to be rescheduled. Next time, methods
	  // cleanupBuffers() and checkReceive() can reduce the number
	  // of in-use buffers and of unread buffers in the receive
	  // queue.
	  return WORK_POOL_BLOCKED;
	}

        // Am I being asked to refrain from new I/O?
        if (getGlobals()->castToExExeStmtGlobals()->noNewRequest())
          {
            ExEspStmtGlobals *espGlobals = getGlobals()->
                            castToExExeStmtGlobals()->castToExEspStmtGlobals();
            if (espGlobals)
              {
                // Make sure my root (split_bottom) knows to schedule me 
                // when I/O can resume.
                espGlobals->setSendTopTcbActivated(mySendTopTcbIndex_);
              }
            return WORK_OK;
          }

      // construct continue request header in message,
      // include child ExFragInstanceHandle for fast routing
      ExEspContinueReqHeader *hdr = 
	new(*msgStream_) ExEspContinueReqHeader((NAMemory *) NULL);
      if (hdr == NULL)
	return WORK_POOL_BLOCKED;
      hdr->handle_ = childFragHandle_;
      hdr->myInstanceNum_ = getMyInstanceNum();

      EXSM_TRACE(EXSM_TRACE_CONTINUE,
                 "SNDT %p CREQ sending continue", this);

      msgStream_->sendRequest();
    }

  return WORK_OK;
}

void ex_send_top_tcb::checkCancelReply()
{
  if (!cancelMessageStream_)
    {
      // no replies yet if no message stream yet.
    }
  else
    {
      IpcMessageObjType msgType;
      while (cancelMessageStream_->getNextReceiveMsg(msgType))
	{
	  ex_assert_both_sides(msgType == IPC_MSG_SQLESP_CANCEL_REPLY,
		    "received message from unknown message stream");
  
	  while (cancelMessageStream_->getNextObjType(msgType))
	    {
	      ex_assert_both_sides(msgType == ESP_RETURN_CANCEL_HDR, 
		   "Wrong type of message received by cancelMessageStream!");

              EXSM_TRACE(EXSM_TRACE_CANCEL,
                         "SNDT %p recv ESP_RETURN_CANCEL_HDR", this);
              EXSM_TRACE(EXSM_TRACE_CANCEL,"SNDT %p state %s", this,
                         getExSendTopStateString(sendTopState_));

	      ExEspCancelReplyHeader *dataHdr =
		new(cancelMessageStream_->receiveMsgObj())
		ExEspCancelReplyHeader(cancelMessageStream_);

	      // if CANCELED_BEFORE_OPENED, only the late cancel message
	      // was sent, not the open message. Go back to NOT_OPENED
              // state so that the open message could be sent.
              // See the state transition diagram in the header file.
	      if (sendTopState_ == CANCELED_BEFORE_OPENED)
                {
		   setStState(NOT_OPENED, __LINE__);
                }
	    }
	}
      // free up any receive buffers no longer in use
      cancelMessageStream_->cleanupBuffers(); 
    }   // if cancelMessageStream_ ... else ...
}

/////////////////////////////////////////////////////////////////////////////
// 
ExWorkProcRetcode ex_send_top_tcb::processCancel()
{
  EXSM_TRACE(EXSM_TRACE_CANCEL,"SNDT %p BEGIN processCancel", this);

  ExWorkProcRetcode result = processCancelHelper();

  if (sendTopTdb().getExchangeUsesSM())
  {
    ExExeStmtGlobals *glob = getGlobals()->castToExExeStmtGlobals();
    EXSM_TRACE(EXSM_TRACE_CANCEL,"SNDT %p glob msgs %d cancels %d", this,
               (int) glob->numSendTopMsgesOut(),
               (int) glob->numCancelMsgesOut());
    if (cancelMessageStream_)
      EXSM_TRACE(EXSM_TRACE_CANCEL,"SNDT %p cancel resp pend %d", this,
                 (int) cancelMessageStream_->numOfResponsesPending());
    EXSM_TRACE(EXSM_TRACE_CANCEL,"SNDT %p END processCancel rc %s", this,
               ExWorkProcRetcodeToString(result));
  }

  return result;
}

ExWorkProcRetcode ex_send_top_tcb::processCancelHelper()
{
  if (ipcBroken_)
    return WORK_BAD_ERROR;

  checkCancelReply();

  // Are there any messages still waiting to be canceled?  Am I too late?
  if (qParent_.down->isEmpty())
  {
    EXSM_TRACE(EXSM_TRACE_CANCEL,"SNDT %p down queue is empty", this);
    return WORK_OK;
  }

  TupMsgBuffer * cancelSendBuffer = NULL;
  queue_index pindex = qParent_.down->getHeadIndex();
 
  while (pindex != nextToSendDown_)
    {
      ex_queue_entry *pentry = qParent_.down->getQueueEntry(pindex);
      ex_send_top_private_state & pstate 
        = *((ex_send_top_private_state *) pentry->pstate);
      
      SqlBuffer *sqlBuf = NULL;
      
      if ((pentry->downState.request == ex_queue::GET_NOMORE) &&
          (pstate.step_ == STARTED_))
        {
          // Am I being asked to refrain from new I/O?
          if (getGlobals()->castToExExeStmtGlobals()->noNewRequest())
            {
              ExEspStmtGlobals *espGlobals = getGlobals()->
                              castToExExeStmtGlobals()->castToExEspStmtGlobals();
              if (espGlobals)
                {
                  // Make sure my root (split_bottom) knows to schedule me 
                  // when I/O can resume.
                  espGlobals->setSendTopTcbActivated(mySendTopTcbIndex_);
                }

              EXSM_TRACE(EXSM_TRACE_CANCEL,
                         "SNDT %p must refrain from new I/O", this);

              return WORK_OK;
            }

	  if (cancelSendBuffer == NULL)
            {
	      if (cancelMessageStream_)
		if (cancelMessageStream_->sendLimitReached()  ||
		    cancelMessageStream_->inUseLimitReached())
                {
                  EXSM_TRACE(EXSM_TRACE_CANCEL,
                             "SNDT %p stream limits prevent new I/O", this);

		  return WORK_OK;
                }

              // cancelMessageStream_ may be NULL, but it's OK
              // getCancelSendBuffer can establish the stream and
              // connection_ should be already created.
	      cancelSendBuffer = getCancelSendBuffer(FALSE);
	      
	      if (cancelSendBuffer == NULL)
              {
                EXSM_TRACE(EXSM_TRACE_CANCEL,
                           "SNDT %p send buffer not available", this);

		return WORK_POOL_BLOCKED;
	      }

	      sqlBuf = cancelSendBuffer->get_sql_buffer();
            }
	  
          // Allocate space in buffer for control info (down state).
          //
          tupp_descriptor *controlInfoAsTupp = 
	    sqlBuf->add_tuple_desc((unsigned short) sizeof(ControlInfo));
	  
          // Check that there was sufficient space in buffer.
          //
          if (controlInfoAsTupp == NULL)
            {
	      //
	      // Sufficient space is not available in buffer.
              // Will cancel more later.  Will rely on actOnReceive
              // to schedule me, after reply from ESP.
              EXSM_TRACE(EXSM_TRACE_CANCEL,
                         "SNDT %p no room in buffer for ControlInfo", this);

              break;
	    }
          else
	    {
	      //
	      // There was space in the send buffer --- copy the current input
	      // row into it and advance to the next input queue entry.
	      //
	      ControlInfo msgControlInfo;
	      
	      // 
	      // Copy the down state and flags into first tupp allocated
	      // in the message buffer.
	      //
	      msgControlInfo.getDownState() = pentry->downState;
	      msgControlInfo.getDownState().parentIndex = pindex;
              msgControlInfo.setIsDataRowPresent(FALSE);
	      msgControlInfo.setIsExtDiagsAreaPresent(FALSE);
              msgControlInfo.setBufferSequenceNumber(pstate.bufferNumber_);
	      
	      str_cpy_all(controlInfoAsTupp->getTupleAddress(),
		          (char *) &msgControlInfo,
		          sizeof(ControlInfo));
	      
              // Now the pentry's pstate.step_ does its transition....
              pstate.step_ = CANCELED_AFTER_SENT_;
	    }
        }   // if GET_NOMORE && STARTED_
      pindex++;
    }     // while pindex != nextToSendDown_
  
  if (cancelSendBuffer)  
  {
    SqlBuffer *sqlBuf = cancelSendBuffer->get_sql_buffer();
    sqlBuf->drivePack();
    
    EXSM_TRACE(EXSM_TRACE_CANCEL,"SNDT %p SEND cancel %d tupps", this,
               (int) sqlBuf->getTotalTuppDescs());
    
    cancelMessageStream_->sendRequest();
  }
  else
  {
    if (sendTopTdb().getExchangeUsesSM() && pindex == nextToSendDown_)
      EXSM_TRACE(EXSM_TRACE_CANCEL,
                 "SNDT %p no outstanding entries to cancel", this);
  }

  return WORK_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
short ex_send_top_tcb::createConnectionToSendBottom(NABoolean nowaitedCompleted)
{
  short retcode = 0;
  
  if (sendTopTdb().getExchangeUsesSM())
    retcode = createSMConnection();
  else 
    retcode = createIpcGuardianConnection(nowaitedCompleted);

  return retcode;
}

short ex_send_top_tcb::createSMConnection()
{
  short retcode = 0;

  IpcEnvironment *env =
    getGlobals()->castToExExeStmtGlobals()->getIpcEnvironment();

  // The numBufs variable represents the number of pre-allocated
  // receive buffers. The connection will need N batches of M buffers
  // where:
  //   N = getNumSendBuffers()
  //   M = getNumRecvBuffers()
  // 
  // And we add one for cancel replies

  UInt32 numBufs = (getNumSendBuffers() * getNumRecvBuffers()) + 1;

  ExMasterStmtGlobals *masterGlobals =
    getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals();

  connection_ = new (env->getHeap())
    SMConnection(env,
                 smTarget_,
                 numBufs,
                 msgStream_->getBufferSize(),
                 this,
                 masterGlobals);
  
  EXSM_TRACE(EXSM_TRACE_PROTOCOL,"SNDT %p created conn %p", 
             this, connection_);
  ex_assert(connection_, "SM connection allocation failed");
  
  SMConnection *smConn = (SMConnection *)
    connection_->castToSMConnection();
  ex_assert(smConn, "Invalid SM connection pointer");
  smConn->setDataStream(msgStream_);
  smConn->setCancelStream(cancelMessageStream_);
  
  return retcode;                  
}

/////////////////////////////////////////////////////////////////////////////
//
short ex_send_top_tcb::createIpcGuardianConnection(NABoolean nowaitedCompleted)
{
  short retcode;
  IpcEnvironment *env = getGlobals()->castToExExeStmtGlobals()->getIpcEnvironment();
  static THREAD_P bool sv_env_multiple_fragments_checked = false;
  static THREAD_P bool sv_multiple_fragments = true;

  Lng32 nowaitDepth = 4;

  if (getStatsEntry() && sendTopTdb().getUseOldStatsNoWaitDepth())
    nowaitDepth = 2;

  if (sendTopState_ == NOT_OPENED)//added
  {
    NABoolean espParallelDcOpens;
    char * espParallelDcOpensEnvvar = getenv("ESP_PARALLEL_DC_OPENS");
    if (espParallelDcOpensEnvvar != NULL && *espParallelDcOpensEnvvar == '0')
      espParallelDcOpens = FALSE;
    else { // multi fragment esp
#ifdef USE_SB_FILE_DYN_THREADS
      sv_max_parallel_opens = FS_MAX_CONCUR_NOWAIT_OPENS;
#else
      sv_max_parallel_opens = 8;
#endif
      if (espParallelDcOpensEnvvar) {
	sv_max_parallel_opens = atoi(espParallelDcOpensEnvvar);
      }
      espParallelDcOpens = TRUE;
    }
      if (!sv_env_multiple_fragments_checked) {
	char *lv_espMultipleFragmentsEnvvar = getenv("ESP_MULTI_FRAGMENTS");
	if ((lv_espMultipleFragmentsEnvvar != NULL) && 
	    (*lv_espMultipleFragmentsEnvvar == '0')) {
	  sv_multiple_fragments = false;
	}
	sv_env_multiple_fragments_checked = true;
      }

      if (sv_multiple_fragments) {
	if ((env->getNumOpensInProgress() >= sv_max_parallel_opens) && nowaitedCompleted == false) {
	  Int32             lv_err;
	  struct timespec lv_tv;

	  lv_tv.tv_sec = 0;
	  lv_tv.tv_nsec = 100 * 1000;
	  lv_err = nanosleep(&lv_tv, NULL);
	  return 1;
	}
      }
    
    // Open the bottom ESP by creating a connection to
    // it. createConnectionToServer() can return NULL if bottomProcId_
    // does not refer to a running server.
    connection_ = bottomProcId_.createConnectionToServer(
	 env,
	 FALSE, // transactions are sent via the control connection
	 nowaitDepth,
	 espParallelDcOpens,
	 ioSubtask_->getScheduledAddr()
         ,
         TRUE // data connection to ESP
         );

    if (connection_ && connection_->getState() == IpcConnection::OPENING)
    {
      if (nowaitedCompleted)
      {
        connection_->openPhandle(NULL, TRUE);
        if (connection_->getState() == IpcConnection::ESTABLISHED)
        {
          setStState(OPEN_COMPLETE, __LINE__);
          return 0;
        }
      }
      else
      {
        //connection_->openPhandle(NULL, FALSE); // Temporarily
        setStState(WAITING_FOR_OPEN_COMPLETION, __LINE__);
        return 0;
      }
    }
    if (connection_ && connection_->getState() == IpcConnection::ESTABLISHED)
    {
#ifdef NA_LINUX_DISABLE // multi fragment esp experiment
      setStState(OPEN_COMPLETE, __LINE__);
#endif
      return 0;
    }

  } // if (state == NOT_OPENED)

  else
  {
    ex_assert(sendTopState_ == WAITING_FOR_OPEN_COMPLETION,
              "ex_send_top_tcb::createConnectionToSendBottom was called in an invalid sendTopState_");
  }

  // If the connection is NULL or the connection encountered errors,
  // add diagnostics to the statement globals diags area. Also set the
  // TCB's ipcBroken flag and set retcode to indicate an error.
  if (connection_ == NULL ||
      (connection_ && connection_->getErrorInfo()))
  {
    ComDiagsArea* da =
      getGlobals()->castToExExeStmtGlobals()->getDiagsArea();
    
    // If da is NULL, allocate a new diags area and attach to
    // statement globals
    if (da == NULL)
    {
      da = ComDiagsArea::allocate(getHeap());
      getGlobals()->castToExExeStmtGlobals()->setGlobDiagsArea(da);
      da->decrRefCount();
    }

    // Two cases to consider
    //
    //  a. connection_ is NULL. This can happen when the ESP phandle
    //  does not represent a running server. For an extract consumer
    //  query the phandle comes from query text so we report an error
    //  to the application. Otherwise the phandle is managed
    //  internally and should always be valid, so it's a bug if we
    //  come here and we raise an assertion failure.
    //
    //  b. connection is not NULL and contains error information. Add
    //  the error information to the statement globals diags area.

    if (connection_ == NULL)
    {
      if (sendTopTdb().getExtractConsumerFlag())
      {
        const char *stringForDiags = sendTopTdb().getExtractEsp();
        *da << DgSqlCode(-EXE_PARALLEL_EXTRACT_CONNECT_ERROR)
            << DgString0(stringForDiags);
      }
      else
      {
        ex_assert(FALSE, "createConnectionToSendBottom failed to connect");
      }
    }
    else
    {
      connection_->populateDiagsArea(da, getHeap());
    }

    setIpcBroken();
    retcode = -1;
  }
  else
  {
    retcode = 0;
  }

  return retcode;
}


/////////////////////////////////////////////////////////////////////////////
//  
ex_queue_pair  ex_send_top_tcb::getParentQueue() const
{
  return qParent_;
}

/////////////////////////////////////////////////////////////////////////////
//
const ex_tcb* ex_send_top_tcb::getChild(Int32 pos) const
{
  ex_assert((pos >= 0), "");
  return NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
Int32 ex_send_top_tcb::numChildren() const
{
  return 0; 
}   

ExOperStats * ex_send_top_tcb::doAllocateStatsEntry(CollHeap *heap,
                                                    ComTdb *tdb)
{
  ExOperStats * stat = NULL;
  if (tdb->getCollectStatsType() == ComTdb::OPERATOR_STATS)
  {
    // check to see if entry for this node exists. This could happen if
    // multuiple send top tcbs are constructed for the same table(like, by
    // a split top node).
    stat =
      getGlobals()->getStatsArea()->get(ExOperStats::EX_OPER_STATS,
					tdb->getTdbId());
    if (stat)
    {
      setStatsEntry(stat);
      stat->incDop();
      return NULL;
    }
    stat = ex_tcb::doAllocateStatsEntry(heap, tdb);
  }
  else
  {
    stat = new(heap) ExESPStats(heap,
			        sendTopTdb().getSendBufferSize(),
			        sendTopTdb().getRecvBufferSize(),
			        getChildInstanceNum(),
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

/////////////////////////////////////////////////////////////////////////////
//
short ex_send_top_tcb::notifyProducerThatWeCanceled()
{
  ExEspStmtGlobals *espGlobals =
    getGlobals()->castToExExeStmtGlobals()->castToExEspStmtGlobals();

  if (espGlobals->noNewRequest())
    {
      // sol 10-090220-9443. after esp receives release work msg, it sets the
      // no-new-request flag that prevents this esp from propagating any msgs
      // further down the esp chain. the msg can be any of the following: 
      //
      //   - data request: see checkSend()
      //   - continue msg: see continueRequest()
      //   - cancel msg: see processCancel()
      //   - late cancel msg: see notifyProducerThatWeCanceled()
      //
      // Make sure my root (split_bottom) knows to schedule me 
      // when I/O can resume.
      espGlobals->setSendTopTcbActivated(mySendTopTcbIndex_);

      return WORK_OK;
    }

  if (connection_ == NULL)
    if (createConnectionToSendBottom(TRUE) != 0)
      return -1;

  TupMsgBuffer *tmb = getCancelSendBuffer(TRUE);
  SqlBuffer *sqlBuf = tmb->get_sql_buffer();
  tupp_descriptor *controlInfoAsTupp = 
    sqlBuf->add_tuple_desc((unsigned short) sizeof(ControlInfo));
  ControlInfo msgControlInfo;

  msgControlInfo.getDownState().request = ex_queue::GET_NOMORE;
  msgControlInfo.setBufferSequenceNumber(9999999);
  msgControlInfo.getDownState().parentIndex = 9999999; // manage this later
  msgControlInfo.setIsDataRowPresent(FALSE);
  msgControlInfo.setIsExtDiagsAreaPresent(FALSE);

  str_cpy_all(controlInfoAsTupp->getTupleAddress(),
	      (char *) &msgControlInfo,
	      sizeof(ControlInfo));

  ex_assert(espGlobals,
	    "Master does not use notifyProducerThatWeCanceled() right now");

  // make sure we wait for the late cancel to complete
  espGlobals->setSendTopTcbLateCancelling();

  if (sendTopState_ == NOT_OPENED  OR 
      sendTopState_ == WAITING_FOR_OPEN_COMPLETION)
    setStState(CANCELED_BEFORE_OPENED, __LINE__);

  EXSM_TRACE(EXSM_TRACE_CANCEL,"SNDT %p SEND late cancel", this);

  cancelMessageStream_->setLateCancel();
  cancelMessageStream_->sendRequest();

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
TupMsgBuffer * ex_send_top_tcb::getCancelSendBuffer(NABoolean lateCancel)
{
  ex_assert((sendTopState_ != NOT_OPENED AND sendTopState_ != WAITING_FOR_OPEN_COMPLETION) || lateCancel,
	    "Canceling too soon!");

  if (!cancelMessageStream_)
    {
      IpcMessageObjSize maxBufSize = MAXOF(sendBufferSize_,500);
      IpcMessageBuffer::alignOffset(maxBufSize);

      maxBufSize += sizeof(TupMsgBuffer);
      IpcMessageBuffer::alignOffset(maxBufSize);

      maxBufSize += MAXOF(sizeof(ExEspCancelReqHeader),
			  sizeof(ExEspLateCancelReqHeader));
      IpcMessageBuffer::alignOffset(maxBufSize);

      cancelMessageStream_ = new(getGlobals()->getSpace()) 
        ExSendTopCancelMessageStream(
	  getGlobals()->castToExExeStmtGlobals(),
          1,      //sendBufferLimit,
          1,      //inUseBufferLimit,
          maxBufSize,
          this); 

      ex_assert(connection_ != NULL, "Connection not created!");
      cancelMessageStream_->addRecipient(connection_);

      if (sendTopTdb().getExchangeUsesSM())
      {
        SMConnection *smConn = (SMConnection *)
          connection_->castToSMConnection();
        ex_assert(smConn, "Invalid SM connection pointer");

        smConn->setCancelStream(cancelMessageStream_);
      }
    }  
  else
    {
      ex_assert( !cancelMessageStream_->sendLimitReached()  &&
		 !cancelMessageStream_->inUseLimitReached(), 
		 "getCancelSendBuffer called at the wrong time!");
    }

  // construct cancel request header in message,
  if (lateCancel)
    {
      ExEspLateCancelReqHeader *hdr = 
	new(*cancelMessageStream_) ExEspLateCancelReqHeader((NAMemory *) NULL);
      if (hdr == NULL)
	return NULL;
      // a late cancel message may not know the ExFragInstanceHandle yet
      hdr->key_ = getGlobals()->castToExExeStmtGlobals()->getFragmentKey(
	   sendTopTdb().getChildFragId());
      hdr->myInstanceNum_ = getMyInstanceNum();
    }
  else
    {
      // include child ExFragInstanceHandle for fast routing
      ExEspCancelReqHeader *hdr = 
	new(*cancelMessageStream_) ExEspCancelReqHeader((NAMemory *) NULL);
      if (hdr == NULL)
	return NULL;
      hdr->handle_ = childFragHandle_;
      hdr->myInstanceNum_ = getMyInstanceNum();
    }

  // construct sql buffer(TupMsgBuffer) directly in message to avoid copy
  Lng32 sqlBufferLen = sendBufferSize_;

  // The TupMsgBuffer for a late cancel is for only one queue entry and
  // doesn't have a data record in it. Note that it may be routed through
  // a non-buffered message stream at the producer ESP (the new incoming
  // message stream in case we also send an open request). Therefore
  // we must not send a multi-buffer message, because only buffered message
  // streams know how to route that. Therefore we keep the buffer length of
  // the TupMsgBuffer small. Sorry for the arbitrary choice of 500. It
  // includes a very generous reserve as long as we work with single
  // cancel requests with no records.
  if (lateCancel)
    sqlBufferLen = MAXOF(sqlBufferLen,500);
  TupMsgBuffer *result = new(*cancelMessageStream_, sqlBufferLen)
			   TupMsgBuffer(sqlBufferLen,
					TupMsgBuffer::MSG_IN,
					cancelMessageStream_);
  if (result == NULL)
    ABORT("mismatched send top buffer size");

  return result;
}

void ex_send_top_tcb::incReqMsg(Int64 msgBytes)
{
  ExStatisticsArea *statsArea;
  if ((statsArea = getGlobals()->getStatsArea()) != NULL)
    statsArea->incReqMsg(msgBytes);
}

/////////////////////////////////////////////////////////////////////////////
void ex_send_top_tcb::setStState(
                   enum ExSendTopState newState,
                   int linenum)
{
  if (newState != sendTopState_)
  {
    if (stTidx_ >= NumSendTopTraceElements)
      stTidx_ = 0;
    sendTopState_ = newState;
    stStateTrace_[stTidx_].stState_ = sendTopState_;
    stStateTrace_[stTidx_].lineNum_ = linenum;
    stTidx_++;
  }
}

const char *ex_send_top_tcb::getExSendTopStateString(ExSendTopState s)
{
  switch (s)
  {
    case INVALID: return "INVALID";
    case NOT_OPENED: return "NOT_OPENED";
    case WAITING_FOR_OPEN_COMPLETION: return "WAITING_FOR_OPEN_COMPLETION";
    case WAITING_FOR_OPEN_REPLY: return "WAITING_FOR_OPEN_REPLY";
    case CANCELED_BEFORE_OPENED: return "CANCELED_BEFORE_OPENED";
    case OPEN_COMPLETE: return "OPEN_COMPLETE";
    case SERVER_SATURATED: return "SERVER_SATURATED";
    default: return ComRtGetUnknownString((Int32) s);
  }
}

// -----------------------------------------------------------------------
// Methods for class ExSendTopMessageStream
// -----------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// constructor
ExSendTopMsgStream::ExSendTopMsgStream(ExExeStmtGlobals* glob,
                                       Lng32 sendBufferLimit,
                                       Lng32 inUseBufferLimit,
                                       IpcMessageObjSize bufferSize,
                                       ex_send_top_tcb* sendTopTcb)

: IpcClientMsgStream(glob->getIpcEnvironment(),
                     IPC_MSG_SQLESP_DATA_REQUEST,
                     CurrEspRequestMessageVersion,
                     sendBufferLimit,
                     inUseBufferLimit,
                     bufferSize),
  sendTopTcb_(sendTopTcb)
  { }

///////////////////////////////////////////////////////////////////////////////
// method called upon send complete
void ExSendTopMsgStream::actOnSend(IpcConnection* connection)
{
  EXSM_TRACE(EXSM_TRACE_PROTOCOL, "SNDT %p ACTS D s %p c %p", 
             sendTopTcb_, this, connection);

  if (connection)
  {
    // we always use this message with the same connection
    //    ex_assert(connection == sendTopTcb_->connection_,
    //       "A send top message is associated with the wrong connection");
    
    if (getErrorInfo())
    {
      ComDiagsArea* da = sendTopTcb_->getGlobals()->
        castToExExeStmtGlobals()->getDiagsArea();
      ComDiagsArea* oldDa =  da;
      connection->populateDiagsArea( da, sendTopTcb_->getHeap());
      if ((!oldDa) && (da))
      {
        sendTopTcb_->getGlobals()->castToExExeStmtGlobals()->
          setGlobDiagsArea(da);
        da->decrRefCount();
      }
      sendTopTcb_->setIpcBroken();
      sendTopTcb_->tickleSchedulerWork();

      EXSM_TRACE(EXSM_TRACE_PROTOCOL,"SNDT %p IPC broken", sendTopTcb_);
      EXSM_TRACE(EXSM_TRACE_PROTOCOL,"SNDT %p tcb scheduled", sendTopTcb_);
    }
    else
    {
      sendTopTcb_->incReqMsg(connection->getLastSentMsg()->getMessageLength());
    }
  }
  
  ExExeStmtGlobals *stmtGlob = 
    sendTopTcb_->getGlobals()->castToExExeStmtGlobals();
  stmtGlob->incrementSendTopMsgesOut();

  EXSM_TRACE(EXSM_TRACE_PROTOCOL, "SNDT %p ACTS D glob msgs %d", 
             sendTopTcb_,
             (int) stmtGlob->numSendTopMsgesOut());
}

/////////////////////////////////////////////////////////////////////////////
// method called upon receive complete
void ExSendTopMsgStream::actOnReceive(IpcConnection* connection)
{
  EXSM_TRACE(EXSM_TRACE_PROTOCOL, "SNDT %p ACTR D s %p c %p", 
             sendTopTcb_, this, connection);

  if (connection)
  {
    // we always use this message with the same connection
    // ex_assert(connection == sendTopTcb_->connection_,
    //	"A send top message is associated with the wrong connection");
    
    if (getErrorInfo())
    {
      ComDiagsArea* da = sendTopTcb_->getGlobals()->
        castToExExeStmtGlobals()->getDiagsArea();
      ComDiagsArea* oldDa =  da;
      connection->populateDiagsArea( da, sendTopTcb_->getHeap());
      if ((!oldDa) && (da))
      {
        sendTopTcb_->getGlobals()->castToExExeStmtGlobals()->
          setGlobDiagsArea(da);
        da->decrRefCount();
      }
      sendTopTcb_->setIpcBroken();
    }
  }
  
  ExExeStmtGlobals *stmtGlob =
    sendTopTcb_->getGlobals()->castToExExeStmtGlobals();

  if (getSMContinueProtocol())
  {
    if (getSMBatchIsComplete())
      stmtGlob->decrementSendTopMsgesOut();
  }
  else
  {
    stmtGlob->decrementSendTopMsgesOut();
  }

  EXSM_TRACE(EXSM_TRACE_PROTOCOL, "SNDT %p ACTR D glob msgs %d rp %d", 
             sendTopTcb_,
             (int) stmtGlob->numSendTopMsgesOut(),
             (int) numOfResponsesPending());

  // wake up the work procedure
  sendTopTcb_->tickleSchedulerWork(TRUE);
  EXSM_TRACE(EXSM_TRACE_PROTOCOL,"SNDT %p tcb scheduled", sendTopTcb_);
}

// -----------------------------------------------------------------------
// Methods for class ExSendTopCancelMessageStream
// -----------------------------------------------------------------------

ExSendTopCancelMessageStream::ExSendTopCancelMessageStream(
     ExExeStmtGlobals *glob,
     Lng32 sendBufferLimit,
     Lng32 inUseBufferLimit,
     IpcMessageObjSize bufferSize,
     ex_send_top_tcb *sendTopTcb) 

: IpcClientMsgStream(glob->getIpcEnvironment(),
                     IPC_MSG_SQLESP_CANCEL_REQUEST,
                     CurrEspRequestMessageVersion,
                     sendBufferLimit,
                     inUseBufferLimit,
                     bufferSize),
  sendTopTcb_(sendTopTcb),
  lateCancel_(FALSE)
  {}

/////////////////////////////////////////////////////////////////////////////
// method called upon send complete

void ExSendTopCancelMessageStream::actOnSend(IpcConnection *connection)
{
  EXSM_TRACE(EXSM_TRACE_PROTOCOL, "SNDT %p ACTS C %p c %p", 
             sendTopTcb_, this, connection);

  if (connection)
    {
      if (getErrorInfo())
	{
	  ComDiagsArea* da = sendTopTcb_->getGlobals()->
	    castToExExeStmtGlobals()->getDiagsArea();
	  ComDiagsArea* oldDa =  da;
	  connection->populateDiagsArea( da, sendTopTcb_->getHeap());
	  if ((!oldDa) && (da))
	    {
	      sendTopTcb_->getGlobals()->castToExExeStmtGlobals()->
		setGlobDiagsArea(da);
	      da->decrRefCount();
	    }
	  sendTopTcb_->setIpcBroken();
	  sendTopTcb_->tickleSchedulerCancel();

          EXSM_TRACE(EXSM_TRACE_PROTOCOL,"SNDT %p IPC broken", sendTopTcb_);
          EXSM_TRACE(EXSM_TRACE_PROTOCOL,"SNDT %p tcb scheduled", sendTopTcb_);
	}
    }

  ExExeStmtGlobals *stmtGlob = 
    sendTopTcb_->getGlobals()->castToExExeStmtGlobals();
  stmtGlob->incrementCancelMsgesOut();

  EXSM_TRACE(EXSM_TRACE_PROTOCOL, "SNDT %p ACTS C glob cancel %d", 
             sendTopTcb_,
             (int) stmtGlob->numCancelMsgesOut());
}


void ExSendTopCancelMessageStream::actOnReceive(IpcConnection *connection)
{
  EXSM_TRACE(EXSM_TRACE_PROTOCOL, "SNDT %p ACTR %s %p c %p", sendTopTcb_,
             (lateCancel_ ? "LC" : "C"), this, connection);

  if (connection)
    {
      if (getErrorInfo())
	{
	  ComDiagsArea *da = sendTopTcb_->getGlobals()->
	    castToExExeStmtGlobals()->getDiagsArea();
	  ComDiagsArea *oldDa =  da;
	  connection->populateDiagsArea( da, 
				         sendTopTcb_->getHeap());
	  if ((!oldDa) && (da))
	    {
	      sendTopTcb_->getGlobals()->castToExExeStmtGlobals()->
		setGlobDiagsArea(da);
	      da->decrRefCount();
	    }
	  sendTopTcb_->setIpcBroken();
	}
      
    }

  ExExeStmtGlobals *stmtGlob = 
    sendTopTcb_->getGlobals()->castToExExeStmtGlobals();

  stmtGlob->decrementCancelMsgesOut();

  if (lateCancel_)
    {
      // do the bookkeeping here and let the cancel method clean
      // up the message stream
      lateCancel_ = FALSE;
      ExEspStmtGlobals *espGlobals = sendTopTcb_->getGlobals()->
	castToExExeStmtGlobals()->castToExEspStmtGlobals();
      if (espGlobals)
	espGlobals->resetSendTopTcbLateCancelling();
    }
  
  EXSM_TRACE(EXSM_TRACE_PROTOCOL, "SNDT %p ACTR %s glob cancel %d rp %d", 
             sendTopTcb_,
             (lateCancel_ ? "LC" : "C"),
             (int) stmtGlob->numCancelMsgesOut(),
             (int) numOfResponsesPending());

  // wake up the cancel procedure.  It might be awaiting this 
  // receive so it can send somemore.
  sendTopTcb_->tickleSchedulerCancel();
  EXSM_TRACE(EXSM_TRACE_PROTOCOL,"SNDT %p tcb scheduled", sendTopTcb_);
}

///////////////////////////////////////////////
// class ex_send_top_private_state
///////////////////////////////////////////////
ex_send_top_private_state::ex_send_top_private_state(
                                            const ex_send_top_tcb *  /*tcb*/)
{
  step_ = ex_send_top_tcb::NOT_STARTED_;
}

ex_send_top_private_state::~ex_send_top_private_state()
{
}

ex_tcb_private_state * ex_send_top_private_state::allocate_new(
                                            const ex_tcb *tcb)
{
  return new(((ex_tcb *)tcb)->getSpace()) 
          ex_send_top_private_state((ex_send_top_tcb *) tcb);
}
