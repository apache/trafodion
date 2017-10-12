// **********************************************************************
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
// **********************************************************************
// -*-C++-*-
// **********************************************************************
// *
// * File:         ex_mj.cpp
// * Description:  Implementation of merge join operator
// *
// *
// * Created:      7/10/95
// * Language:     C++
// *
// *
// *
// *
// ***************************************************************************

#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ComQueue.h"
#include "ex_mj.h"
#include "ex_exe_stmt_globals.h"
#include "ex_expr.h"
#include "ExStats.h"
#include "logmxevent.h"
#include "sql_buffer_size.h"
#include "CommonStructs.h"



// TODO: The merge join operator only works on one request at at time,
// TODO: so it doesn really need to use a private state object.
// TODO: Replacing the ex_mj_private_state pstate object by ex_mj_tcb
// TODO: member variables would enable queue resizing logic to be simplified.

// TODO: Periodically check for a next left child row in MJ_SAVE_DUP_RIGHT
// TODO: state when opportunistic look ahead didn't have a next left
// TODO: child row to check.  If a next left child row that differs from
// TODO: the current left child row is found, join the duplicate right child
// TODO: rows that have been saved with the current left child row and then
// TODO: iterate between MJ_GET_RIGHT and MJ_RETURN_ONE_ROW to process any
// TODO: more duplicate right child rows.


/////////////////////////////////////////////////////////////
// Methods for class ex_mj_tdb
/////////////////////////////////////////////////////////////


/////////////////////////////
// Build a mj tcb
/////////////////////////////
ex_tcb * ex_mj_tdb::build(ex_globals * glob)
{
  // first build the children
  ex_tcb * left_tcb  = tdbLeft_->build(glob);
  ex_tcb * right_tcb = tdbRight_->build(glob);

  ex_mj_tcb * mj_tcb = NULL;

  if (isLeftUnique() || isRightUnique())
  {
    mj_tcb =
      new(glob->getSpace()) ex_mj_unique_tcb(*this, *left_tcb, *right_tcb, glob);
  }
  else
  {
    mj_tcb = new(glob->getSpace()) ex_mj_tcb(*this, *left_tcb, *right_tcb, glob);
    mj_tcb->checkInit();
  }

  // add the mj_tcb to the schedule
  mj_tcb->registerSubtasks();

  return (mj_tcb);
}

/////////////////////////////////////////////////////////////
// Methods for class ex_mj_tcb
/////////////////////////////////////////////////////////////

ex_mj_tcb::ex_mj_tcb(const ex_mj_tdb & mj_tdb,
		     const ex_tcb & left_tcb,
		     const ex_tcb & right_tcb,
		     ex_globals *glob)
  : ex_tcb(mj_tdb, 1, glob),
    dupPool_(NULL), leftEncodedKeyPtr_(NULL), rightEncodedKeyPtr_(NULL),
    lookAheadState_(LA_LEFT_UNCHECKED),
    postIoStep_(ex_mj_tcb::MJ_CANCEL), prevRightAtp_(NULL),
    savedRetCode_(ex_expr::EXPR_OK), tspace_(NULL), workAtp_(NULL),
    nullPool_(NULL)
{
  CollHeap * space = glob->getSpace();
  NAMemory * heap = glob->getDefaultHeap();

  // Unique merge join doesn't need tuple storage, since it doesn't
  // perform special null handling or save duplicate right rows.
  if (!mj_tdb.isLeftUnique() && !mj_tdb.isRightUnique())
  {
    pool_ = NULL;
    Int32 nBuffers = mj_tdb.numBuffers_;
    Lng32 bufSize = static_cast<Lng32>(mj_tdb.bufferSize_);

    if (isLeftJoin() && ljExpr())
    {
      // Allocate a bufSize sql_buffer_pool for ljExpr results.
      // If the number of buffers has been specified (is not
      // the default value of one), then decrement the number
      // of buffers to account for the sql_buffer_pool.
      pool_ = new(space) sql_buffer_pool(1, bufSize, space);
      if (nBuffers > 1)
      {
        nBuffers--;
      }

      // Allocate a NULL tuple for use in null instantiation.
      if (mjTdb().ljRecLen_ > 0) {
        ULng32 nullLength = mjTdb().ljRecLen_;
      
        Lng32 neededBufferSize = 
          (Lng32) SqlBufferNeededSize( 1, nullLength);
        nullPool_ = new(space) sql_buffer_pool(1, neededBufferSize, space);
        nullPool_->get_free_tuple(nullData_, nullLength);

        // Fill tuple with NULL values.
        str_pad(nullData_.getDataPointer(), nullLength, '\377');
      }
    }

    // Use a lower overhead pool for storing and returning duplicate right
    // child rows.  Reserve a number of entries equal to the max parent
    // up queue size for returning values.  The remaining entries in the
    // pool (if any) are used to store duplicates.
    UInt32 nReserveEntries = mj_tdb.getMaxQueueSizeUp();
    dupPool_ = new(space) ExDupSqlBuffer(nBuffers, bufSize, nReserveEntries,
                                         mj_tdb.rightDupRecLen_, space);

    // Support a smaller TupleSpace buffer size for regression tests.
    ByteCount tsBufferSize = TupleSpace::OVERFLOW_BUFFER_SIZE;
#if defined(_DEBUG)
    if (getenv("MJ_TEST_OVERFLOW"))
    {
      tsBufferSize = TupleSpace::OVERFLOW_TEST_BUFFER_SIZE;
    }
#endif

    ExExeStmtGlobals* stmtGlobals = glob->castToExExeStmtGlobals();

    // Yield BMO quota that merge join is unlikely to use.  Use the lesser
    // of (quotaMB * MJ_BMO_QUOTA_PERCENT) and the memory required to
    // store duplicate rows for 1/4 of the total rows estimated to be
    // returned by this merge join operator.
    // 
    // The "1/4 of the total rows" is an arbitrary value based on the
    // assumption that a given duplicate right join key value will
    // account for less than half of the joined rows returned to merge
    // join's parent operator.  At most half of these rows (one
    // quarter of the total) will be from saved duplicate right child
    // rows, since rows are saved only when at least two left child
    // rows match.  Note: A zero quota implies no quota is enforced.
    UInt32 quotaMB = mj_tdb.getQuotaMB();
    if (quotaMB)
    {
      Float32 estRows = mj_tdb.getEstRowsUsed();
      Int32 recLen = mj_tdb.rightDupRecLen_;
      UInt32 assumedMaxMB
        = static_cast<UInt32>(
          (recLen * estRows) / (4 * ExOverflow::ONE_MEGABYTE));
      UInt32 pctBasedQuotaMB
        = static_cast<UInt32>(quotaMB * (mj_tdb.getQuotaPct()/100.0));
      if (pctBasedQuotaMB && (pctBasedQuotaMB < assumedMaxMB))
      {
        assumedMaxMB = pctBasedQuotaMB;
      }

      if (assumedMaxMB < quotaMB)
      {
        stmtGlobals->yieldMemoryQuota(quotaMB - assumedMaxMB);
        quotaMB = assumedMaxMB;
      }
    }

    
    
    // cast mj tdb  to non-const
    ex_mj_tdb * mjtdb = (ex_mj_tdb *)&mj_tdb;
    ScratchOverflowMode ovMode;

    switch(mjtdb->getOverFlowMode())
    {
      case SQLCLI_OFM_SSD_TYPE: 
        ovMode = SCRATCH_SSD;
        break;
      case SQLCLI_OFM_MMAP_TYPE: 
        ovMode = SCRATCH_MMAP;
        break;
      default:
      case SQLCLI_OFM_DISK_TYPE:
        ovMode =SCRATCH_DISK;
        break;
    }

    tspace_ = new(space) TupleSpace(mj_tdb.rightDupRecLen_,
                                    TupleSpace::MIN_INITIAL_BUFFERS,
                                    tsBufferSize, quotaMB,
                                    rightCopyDupExpr(), mj_tdb.workCriDesc_,
                                    heap, isOverflowEnabled(), stmtGlobals,
                                    mj_tdb.getScratchThresholdPct(),
                                    ovMode,
                                    mj_tdb.getYieldQuota());
  
    
  }

  tcbLeft_ = &left_tcb;
  tcbRight_ = &right_tcb;

  // Queues that left and right children use to communicate with merge join
  qleft  = left_tcb.getParentQueue();
  qright  = right_tcb.getParentQueue();
  ex_cri_desc * from_parent_cri = mj_tdb.criDescDown_;
  ex_cri_desc * to_parent_cri = mj_tdb.criDescUp_;

  // Queue merge join uses to communicate with its parent
  allocateParentQueues(qparent);

  // Note: dupTupp_ is used to avoid getTupp() method calls, because
  // getTupp() calls weren't getting inlined.
  dupAtp_ = allocateAtp(mj_tdb.workCriDesc_, space);
  dupTupp_ = &dupAtp_->getTupp(DUP_ATP_INDEX);

  if (mj_tdb.encodedKeyCompOpt())
    {
      // The workAtp_ is used for generating encoded keys.
      // Since workAtp_ is only accessed by expression evaluation
      // it's okay that its descriptor::allocatedSize_ is zero.
      workAtp_ = allocateAtp(mj_tdb.workCriDesc_, space);
      workAtp_->getTupp(mj_tdb.encodedKeyWorkAtpIndex_) =
	new(space) tupp_descriptor();
      leftEncodedKeyPtr_ = new(space) char[mj_tdb.encodedKeyLen_];
      rightEncodedKeyPtr_ = new(space) char[mj_tdb.encodedKeyLen_];
    }

  saveFirstDupAtp_ = false;

  // fixup expressions
  if (mergeExpr())
    (void) mergeExpr()->fixup(0, getExpressionMode(), this,
			      glob->getSpace(), glob->getDefaultHeap(), FALSE, glob);

  if (compExpr())
    (void) compExpr()->fixup(0, getExpressionMode(), this,
			     glob->getSpace(), glob->getDefaultHeap(), FALSE, glob);

  if (leftEncodedKeyExpr())
    (void) leftEncodedKeyExpr()->fixup(0, getExpressionMode(), this,
				       glob->getSpace(),
				       glob->getDefaultHeap(), FALSE, glob);

  if (rightEncodedKeyExpr())
    (void) rightEncodedKeyExpr()->fixup(0, getExpressionMode(), this,
					glob->getSpace(),
					glob->getDefaultHeap(), FALSE, glob);

  if (postJoinExpr())
    (void) postJoinExpr()->fixup(0, getExpressionMode(), this,
				 glob->getSpace(), glob->getDefaultHeap(), FALSE, glob);

  if (preJoinExpr())
    (void) preJoinExpr()->fixup(0, getExpressionMode(), this,
				glob->getSpace(), glob->getDefaultHeap(), FALSE, glob);

  if (leftCheckDupExpr())
    (void) leftCheckDupExpr()->fixup(0, getExpressionMode(), this,
				     glob->getSpace(), glob->getDefaultHeap(), FALSE, glob);

  if (rightCheckDupExpr())
    (void) rightCheckDupExpr()->fixup(0, getExpressionMode(), this,
				      glob->getSpace(),
				      glob->getDefaultHeap(), FALSE, glob);

  if (ljExpr())
    (void) ljExpr()->fixup(0, getExpressionMode(), this,
			   glob->getSpace(), glob->getDefaultHeap(), FALSE, glob);

  if (rightCopyDupExpr())
    (void) rightCopyDupExpr()->fixup(0, getExpressionMode(), this,
				     glob->getSpace(),
				     glob->getDefaultHeap(), FALSE, glob);

}

ex_mj_tcb::~ex_mj_tcb()
{
  delete qparent.up;
  delete qparent.down;

  freeResources();
}

void ex_mj_tcb::freeResources()
{
  delete pool_;
  pool_ = NULL;

  delete dupPool_;
  dupPool_ = NULL;

  delete tspace_;
  tspace_ = NULL;

  if (nullPool_) {
    delete nullPool_;
    nullPool_ = NULL;
  }
}

////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ex_mj_tcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ex_mj_private_state> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

void
ex_mj_tcb::checkInit(void)
{
  ex_assert((rightCopyDupExpr()),
            "Regular merge join should always have a rightCopyDupExpr");
}

void
ex_mj_tcb::createDiags(Int16 sqlCode)
{
  Int16 sysError = 0;

  ex_queue_entry * downEntry = qparent.down->getHeadEntry();
  ComDiagsArea *diags = downEntry->getDiagsArea();
  if (!diags)
    {
      NAMemory * heap = this->getGlobals()->getDefaultHeap();
      diags = ComDiagsArea::allocate(heap);
    }
  else
    {
      diags->incrRefCount();
    }

  if (!sqlCode)
  {
    // A sqlCode argument wasn't passed in, so get the last SQLCODE value.
    sqlCode = tspace_->getLastSqlCode();
    ex_assert(sqlCode, "ex_mj_tcb::createDiags() getLastSqlCode returned zero");
    sysError = tspace_->getLastError();
  }
  *diags << DgSqlCode(-sqlCode) << DgInt0(sysError) << DgString0("Merge join");
  downEntry->setDiagsArea(diags);

  // The Q_SQLERROR entry that will return the diagnostics information doesn't
  // match any rows, so reset matchCount_.
  ex_mj_private_state & pstate = *((ex_mj_private_state*) downEntry->pstate);
  pstate.matchCount_ = 0;
}

bool
ex_mj_tcb::processError(atp_struct* entryAtp)
{
  if (qparent.up->isFull())
    {
      return false;
    }

  ex_queue_entry *downEntry = qparent.down->getHeadEntry();
  ex_mj_private_state &  pstate = *((ex_mj_private_state*) downEntry->pstate);

  ex_queue_entry *upEntry = qparent.up->getTailEntry();

  qleft.down->cancelRequestWithParentIndex(qparent.down->getHeadIndex());
  qright.down->cancelRequestWithParentIndex(qparent.down->getHeadIndex());

  if (entryAtp)
    upEntry->copyAtp(entryAtp);

  ComDiagsArea *prevDiagsArea = downEntry->getDiagsArea();
  if (prevDiagsArea)
    {
      ComDiagsArea *diagsArea = upEntry->getDiagsArea();
      if (!diagsArea)
        {
          diagsArea = ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
        }
      else
        {
          diagsArea->incrRefCount();   // to offset the decrRefCount done on setDiagsArea call
        }
      diagsArea->mergeAfter(*prevDiagsArea);
      upEntry->setDiagsArea(diagsArea);
    }

  upEntry->upState.status = ex_queue::Q_SQLERROR;
  upEntry->upState.downIndex = qparent.down->getHeadIndex();
  upEntry->upState.parentIndex = downEntry->downState.parentIndex;
  upEntry->upState.setMatchNo(pstate.matchCount_);
  qparent.up->insert();

  pstate.step_ = ex_mj_tcb::MJ_CANCEL;

  return true;
}

bool ex_mj_tcb::reacquireResources(void)
{
  if (tspace_)
    {

      
      tspace_->reacquireResources();  // failure will invoke longjmp handler


      if (mjTdb().getLogDiagnostics())
        {
          char msg[128];
          UInt32 memorySize = static_cast<UInt32>(tspace_->getMemory());
          str_sprintf(msg, "Merge join initial TupleSpace memory is %d bytes",
                      memorySize);
          SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, msg,
                                          mjTdb().getExplainNodeId());
        }
    }  // if tspace_

  return true;
}

void ex_mj_tcb::registerSubtasks()
{
  ex_tcb::registerSubtasks();

  // NSK-specific ScratchFileConnection requires scheduler subtask
  if (tspace_)
  {
    ExScheduler * sched = getGlobals()->getScheduler();
    ExSubtask* ioEventHandler
      = sched->registerNonQueueSubtask(sWork, this, "WK");
    tspace_->setIoEventHandler(ioEventHandler);
  }

  // Parent queues will be resizable, so register a resize subtask
  this->registerResizeSubtasks();
}

void ex_mj_tcb::start(ex_mj_private_state &pstate)
{
  if (reacquireResources())
    {
      ex_queue_entry * pentry_down = qparent.down->getHeadEntry();

      ex_queue_entry * lentry = qleft.down->getTailEntry();
      ex_queue_entry * rentry = qright.down->getTailEntry();

      // pass GET ALL request to both children. This is done
      // because all rows are needed(in worst case) to find even
      // one matching row.
      lentry->downState.request = ex_queue::GET_ALL;
      lentry->downState.requestValue = pentry_down->downState.requestValue;
      lentry->downState.parentIndex = qparent.down->getHeadIndex();
      lentry->passAtp(pentry_down);

      rentry->downState.request = ex_queue::GET_ALL;
      rentry->downState.requestValue = pentry_down->downState.requestValue;
      rentry->downState.parentIndex = qparent.down->getHeadIndex();
      rentry->passAtp(pentry_down);

      qleft.down->insert();
      qright.down->insert();

      pstate.step_ = ex_mj_tcb::MJ_GET_LEFT;
    }
  else
    {
      // Failed to reacquire resources.  Release any resources that were acquired.
      if (tspace_)
      {
        tspace_->releaseResources();
      }
      processError();
      pstate.step_ = ex_mj_tcb::MJ_DONE_NEVER_STARTED;
    }
}

short ex_mj_tcb::stop(ex_mj_private_state& pstate)
{
  ex_queue_entry * pentry_down = qparent.down->getHeadEntry();
  ex_queue_entry * pentry = qparent.up->getTailEntry();

  // check if we've got room in the up queue
  if (qparent.up->isFull())
    return WORK_OK;

  // all rows have been returned.
  pentry->upState.status = ex_queue::Q_NO_DATA;
  pentry->upState.downIndex = qparent.down->getHeadIndex();
  pentry->upState.parentIndex = pentry_down->downState.parentIndex;
  pentry->upState.setMatchNo(pstate.matchCount_);

  // insert into parent up queue
  qparent.up->insert();

  // remove the Q_NO_DATA rows from children.
  if (pstate.step_ != ex_mj_tcb::MJ_DONE_NEVER_STARTED)
    {
      qleft.up->removeHead();
      qright.up->removeHead();
    }

  pstate.step_ = ex_mj_tcb::MJ_EMPTY;
  pstate.matchCount_ = 0;
  pstate.outerMatched_ = false;

  prevRightAtp_ = NULL;
  saveFirstDupAtp_ = false;

  qparent.down->removeHead();
  lookAheadState_ = LA_LEFT_UNCHECKED; // reset to initial state

  if (qparent.down->isEmpty())
    {
      if (tspace_)
      {
        tspace_->releaseResources();
        if (mjTdb().getLogDiagnostics())
        {
          char msg[64];
          UInt32 maxMemory = static_cast<UInt32>(tspace_->getMaxMemory());
          str_sprintf(msg, "Merge join released resources, max memory %d",
                      maxMemory);
          SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, msg,
                                          mjTdb().getExplainNodeId());
        }
      }
      return WORK_OK;
    }
  else
    return WORK_CALL_AGAIN; // check for more parent requests next time
}

short ex_mj_tcb::cancel(ex_mj_private_state& pstate)
{
  // request was cancelled. Both children were sent cancel
  // requests. Ignore all up rows from children.
  // Wait for Q_NO_DATA.

  // consume rows from left child
  bool done     = false;
  bool leftEOD  = false;
  bool rightEOD = false;

  while (!done)
    {
      if (qleft.up->isEmpty())
	done = true;
      else
	{
	  ex_queue_entry * lentry = qleft.up->getHeadEntry();

	  ex_queue::up_status left_status = lentry->upState.status;
	  switch(left_status)
	    {
	    case ex_queue::Q_OK_MMORE:
	    case ex_queue::Q_SQLERROR:
	      {
		// just consume the child row
		qleft.up->removeHead();
	      }
	    break;

	    case ex_queue::Q_NO_DATA:
	      {
		// Done with left child.
		leftEOD = true;
		done = true;
	      }
	    break;

	    case ex_queue::Q_INVALID:
	      ex_assert(0, "ex_mj_tcb::cancel() "
                           "Invalid state returned by left child");
	      break;

	    }; // end of switch on status of child queue
	} // left queue is not empty
    } // while not done

  // consume rows from right child
  done = false;
  while (!done)
    {
      if (qright.up->isEmpty())
	done = true;
      else
	{
	  ex_queue_entry * rentry = qright.up->getHeadEntry();

	  ex_queue::up_status right_status = rentry->upState.status;
	  switch(right_status)
	    {
	    case ex_queue::Q_OK_MMORE:
	    case ex_queue::Q_SQLERROR:
	      {
		// just consume the child row
		qright.up->removeHead();
	      }
	    break;

	    case ex_queue::Q_NO_DATA:
	      {
		// Done with right child.
		rightEOD = true;
		done = true;
	      }
	    break;

	    case ex_queue::Q_INVALID:
	      ex_assert(0, "ex_mj_tcb::cancel() "
                           "Invalid state returned by right child");
	      break;

	    }; // end of switch on status of child queue
	} // right queue is not empty
    } // while not done

  if (leftEOD && rightEOD)
    {
      // we will only reach here if both children have returned Q_NO_DATA.
      pstate.step_ = ex_mj_tcb::MJ_DONE;
      // we still need to do the cleanup
      return WORK_CALL_AGAIN;
    }

  // wait for more rows or EOD to arrive
  return WORK_OK;
} // request was cancelled


ex_mj_tcb::Comparison
ex_mj_tcb::compareTuples(ex_queue_entry* lentry,
                         ex_queue_entry* rentry,
                         bool doFullComparison)
{
  Comparison cmpResult = CMP_ERROR;

  ex_expr::exp_return_type retCode = ex_expr::EXPR_FALSE;

  if (mjTdb().encodedKeyCompOpt())
  {
    workAtp_->getTupp(mjTdb().encodedKeyWorkAtpIndex_).
      setDataPointer(leftEncodedKeyPtr_);
    retCode = leftEncodedKeyExpr()->eval(lentry->getAtp(), workAtp_);
    if (retCode != ex_expr::EXPR_ERROR)
    {
      workAtp_->getTupp(mjTdb().encodedKeyWorkAtpIndex_).
        setDataPointer(rightEncodedKeyPtr_);
      retCode = rightEncodedKeyExpr()->eval(workAtp_, rentry->getAtp());
    }
    if (retCode != ex_expr::EXPR_ERROR)
    {
      Int32 compareCode = memcmp(leftEncodedKeyPtr_, rightEncodedKeyPtr_,
                               mjTdb().encodedKeyLen_);
      if (compareCode == 0)
        cmpResult = CMP_EQUAL;
      else if (compareCode < 0)
        cmpResult = CMP_LESS;
      else
        cmpResult = CMP_GREATER;
    }
  }
  else
  { // Not using encoded key comparison optimization
    retCode = mergeExpr()->eval(lentry->getAtp(), rentry->getAtp());
    if (retCode == ex_expr::EXPR_TRUE)
    {
      cmpResult = CMP_EQUAL;
    }
    else if (retCode != ex_expr::EXPR_ERROR)
    {
      if (doFullComparison)
      {
        retCode = compExpr()->eval(lentry->getAtp(), rentry->getAtp());
        if (retCode == ex_expr::EXPR_TRUE)
        {
          cmpResult = CMP_LESS;    // left tuple < right tuple
        }
        else if (retCode != ex_expr::EXPR_ERROR)
        {
          cmpResult = CMP_GREATER;    // left tuple > right tuple
        }
      }
      else
      {
        cmpResult = CMP_NOT_EQUAL;  // partial comparison determined "not equal"
      }
    }
  }

  return cmpResult;
}

// Join a row and return it via the parent up queue.  The caller is
// responsible for ensuring the parent up queue has an available entry.
ex_expr::exp_return_type
ex_mj_tcb::returnRow(atp_struct* leftAtp,
                     atp_struct* rightAtp,
                     ex_mj_private_state& pstate,
                     bool isUniqueMj,
                     ExOperStats *statsEntry)
{
  // This code relies on the fact that the SEMI_JOIN, ANTI_JOIN, and
  // LEFT_JOIN flags are always zero for a unique merge join operator
  // (see MergeJoin::codeGen).

  ex_expr::exp_return_type retCode = ex_expr::EXPR_TRUE;

  ex_assert((!qparent.up->isFull()),
            "returnRow() called when parent up queue was full");

  ex_queue_entry* pentry = qparent.up->getTailEntry();
  atp_struct* parentAtp = pentry->getAtp();

  // Evaluate the pre-join expression
  if (!isUniqueMj && preJoinExpr())
  {
    // TODO: fix defect caused by preJoinExpr assumption that rightAtp
    // always has a contiguous tuple in tupp2.  Requires a code generation
    // change to create a second pre-join predicate
    retCode = preJoinExpr()->eval(leftAtp, rightAtp);
  }

  if (retCode == ex_expr::EXPR_TRUE)
  {  // satisfied pre-join predicate

    // Remember that a match has been found for the current left row.
    // This knowledge is used by left outer join, semi-join, and anti-semi-join.
    // Left outer join will avoid null instantiating the left (outer) row.
    // Semi-join will return the left row.  Anti-semi-join will avoid returning
    // the left row.
    pstate.outerMatched_ = true;

    if (!isAntiJoin())
    {  // not anti-semi-join

      // Return left row portion of join.  Semi-join returns just the
      // left row.  Semi-joins are handled here rather than in the
      // MJ_FINISH_LEFT.
      pentry->copyAtp(leftAtp);

      if (!isSemiJoin())
      {
        // Return right row portion of join.
        short nLeftTuples = static_cast<short>(leftAtp->numTuples());
        short lastSrcAtpIndex = static_cast<short>(rightAtp->numTuples() - 1);
        if (!isUniqueMj)
        {
          // Join flattened (contiguous) saved duplicate right row
          parentAtp->copyPartialAtp(rightAtp,          // src ATP
                                    nLeftTuples,       // first tgt ATP index
                                    DUP_ATP_INDEX,     // first src ATP index
                                    DUP_ATP_INDEX);    // last src ATP index
        }
        else
        {
          // Join unflattened right row
          parentAtp->copyPartialAtp(rightAtp,          // src ATP
                                    nLeftTuples,       // first tgt ATP index
                                    nLeftTuples,       // first src ATP index
                                    lastSrcAtpIndex);  // last src ATP index
        }
      }

      // Left outer join special null handling for the case where
      // output of the right child depends on its input.  (see
      // Join::instantiateValuesForLeftJoin).  Instantiating the right
      // value requires ljRecLen_ space to be allocated at the end of
      // the row being returned in pentry.
      if (isLeftJoin() && ljExpr())
      {
        if (pool_->get_free_tuple(parentAtp->getTupp(pentry->numTuples()-1),
                                  mjTdb().ljRecLen_))
        {
          // Couldn't allocate; try to add a new buffer.
          Lng32 bufSize = static_cast<Lng32>(mjTdb().bufferSize_);

          if (!pool_->addBuffer(bufSize, false))
          {
            createDiags(static_cast<Int16>(EXE_NO_MEM_TO_EXEC));
            retCode = ex_expr::EXPR_ERROR;
          }
          else if (pool_->get_free_tuple(parentAtp->getTupp(pentry->numTuples()-1),
                                    mjTdb().ljRecLen_))
          {
            ex_assert(0, "ex_mj_tcb::returnRow() - Could not allocate pool_ buffer");
          }
        }

        // Instantiate right value
        if (retCode == ex_expr::EXPR_TRUE)
        {
          retCode = ljExpr()->eval(parentAtp, 0);
        }

        if (retCode != ex_expr::EXPR_ERROR)
        {
          retCode = ex_expr::EXPR_TRUE;
        }
      }

      if ((retCode == ex_expr::EXPR_TRUE) && postJoinExpr())
        {
          retCode = postJoinExpr()->eval(parentAtp, 0);
        }

      if (retCode == ex_expr::EXPR_TRUE)
        {
          pentry->upState.status = ex_queue::Q_OK_MMORE;
          pentry->upState.downIndex = qparent.down->getHeadIndex();
          ex_queue_entry* pentryDown = qparent.down->getHeadEntry();
          pentry->upState.parentIndex = pentryDown->downState.parentIndex;
          pstate.matchCount_++;
          pentry->upState.setMatchNo(pstate.matchCount_);

          if (statsEntry)
            statsEntry->incActualRowsReturned();

          // insert into parent up queue
          qparent.up->insert();
        }
      else
        {
          if (retCode == ex_expr::EXPR_ERROR)
          {
            processError();
          }
	  else 
          {
            // retCode is not EXPR_TRUE nor EXPR_ERROR, so nothing was send up
            parentAtp->release();
          }
        }
    }  // not anti-semi-join
  }  // satisfied pre-join predicate
  else if (retCode == ex_expr::EXPR_ERROR)
    {
      processError(leftAtp);
    }

  return retCode;
}

///////////////////////////////////////////////////////////////////////////////
// work()                                                                    //
// This is where all the action is.                                          //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Here is a schematic view of the TCB's state machine:                      //
//                                                                           //
// UNINITIALIZED:  not yet initialized, ptrs are NULL                        //
//                                                                           //
// EMPTY:          initial state, no request                                 //
//                                                                           //
// GET LEFT:       read next row from the left table                         //
//                                                                           //
// GET RIGHT:      read rows from the right table until right >= left or     //
//                 until we reach EOD on the right table                     //
//                                                                           //
// RETURN ONE ROW: join the left child row with current right child row and  //
//                 put the joined row in the parent up queue.                //
//                                                                           //
// SAVE DUP RIGHT: save matching duplicate right child rows in dupPool_.     //
//                                                                           //
// SAVE DUP RIGHT TSPACE: "aliased" state sharing code with SAVE DUP RIGHT.  //
//                 Save matching duplicate right child rows in TupleSpace.   //
//                                                                           //
// RETURN SAVED DUP ROWS: return the left child row joined with the          //
//                 duplicate right child rows saved in dupPool_.             //
//                                                                           //
// RETURN OVERFLOW ROWS: return the left child row joined with the duplicate //
//                 right child rows saved in TupleSpace.                     //
//                                                                           //
// FINISH LEFT:    handle null instantiation of left outer joins             //
//                                                                           //
// REWIND OVERFLOW: reposition current pointer to first TupleSpace tuple.    //
//                                                                           //
// ERROR:          indicate to our parent an error that is not associated    //
//                 with a specific child row, such as an I/O error or a      //
//                 memory allocation error, occurred.                        //
//                                                                           //
// CANCEL:         consume all rows until EOD is reached on left and         //
//                 right tables                                              //
//                                                                           //
//             +---------------+                                             //
//             | UNINITIALIZED |                                             //
//             +------+--------+                                             //
//                    |                                                      //
//                    | pstate is used for the first time                    //
//                    |                                                      //
//             +---------------+                                             //
//             |    EMPTY      |                                             //
//             +------+--------+                                             //
//                    |                                                      //
//                    | get a request                                        //
//                    | from parent queue                                    //
//                    |                                                      //
//                    |      +-------------------<---------------------+     //
//                    |      |                                         |     //
//                    V      V                                         |     //
//             +---------------+   duplicate row on left               |     //
//  +-----<----|  GET LEFT     |----------------->-----+               |     //
//  |      EOD +------+--------+                       |               |     //
//  |      on         |                                |               |     //
//  |      left       | non-duplicate left row         |               |     //
//  |      table      |                                V               |     //
//  |                 V                                |               |     //
//  |          +---------------+                       |               |     //
//  |          |               |-----------------<----------------+    |     //
//  |          |               |                       |          |    |     //
//  |          |     GET       |   left < right        |          |    |     //
//  |     <----+    RIGHT      |----------------->----------+     |    |     //
//  |      EOD |               |<----+                 |    |     |    |     //
//  |      on  +------+----+---+     |                 |    |     |    |     //
//  |      right      |    |         |                 |    |     |    |     //
//  |      table      |    |         |                 |    |     |    |     //
//  |                 |    |   +-----+----------+      |    |     |    |     //
//  |                 |    +-->| RETURN ONE ROW |      |    |     |    |     //
//  |                 |        +----------------+      |    |     |    |     //
//  |                 |   (opportunistic look ahead)   |    |     |    |     //
//  |                 |   next left != current left    |    |     |    |     //
//  |                 |      and left == right         |    |     |    |     //
//  |                 |                                |    |     |    |     //
//  |                 |                                |    |     |    |     //
//  |                 | left == right and              V    V     ^    ^     //
//  |                 | next left is duplicate         |    |     |    |     //
//  |                 | or unavailable                 |    |     |    |     //
//  |                 V                                |    |     |    |     //
//  |          +----------------+                      |    |     |    |     //
//  |          | SAVE DUP RIGHT |------+               |    |     |    |     //
//  |          +------+---------+      |               |    |     |    |     //
//  |                 |                |               |    |     |    |     //
//  |                 |                V               |    |     |    |     //
//  |                 |        +-------+---------+     |    |     |    |     //
//  |                 |        | REWIND OVERFLOW |     |    |     |    |     //
//  |                 |        +-------+---------+     |    |     |    |     //
//  |                 |      (complete pending write)  |    |     |    |     //
//  |                 |                |               |    |     |    |     //
//  |                 +<---------------+               |    |     |    |     //
//  |                 |                                V    V     ^    ^     //
//  |                 | last duplicate right seen;     |    |     |    |     //
//  |                 | right > left or EOD on right   |    |     |    |     //
//  |                 V                                |    |     |    |     //
//  |          +----------------+                      |    |     |    |     //
//  |    +---->|  RETURN SAVED  |<---------------------+    |     |    |     //
//  |    |     |    DUP ROWS    |-------+                   |     |    |     //
//  |    |     +------+---------+       |                   |     |    |     //
//  |    |            |                 |                   |     |    |     //
//  |    |            |         if duplicate right          |     |    |     //
//  |    |            |       rows saved in TupleSpace      |     |    |     //
//  |    |            |                 |                   |     |    |     //
//  |    ^            V                 V                   |     |    |     //
//  |    |            |        +--------+-------------+     |     |    |     //
//  |    |            |        | RETURN OVERFLOW ROWS |     V     ^    ^     //
//  |    |            |        +--------+-------------+     |     |    |     //
//  |    |            |        join TupleSpace dup rows     |     |    |     //
//  |    |            |                 |                   |     |    |     //
//  |    |            +--->---+----<----+                   |     |    |     //
//  |    |                    |                             |     |    |     //
//  |    |                    | semi-join or                |     |    |     //
//  |    |                    | end of saved                |     |    |     //
//  |    | next left          | duplicate                   |     |    |     //
//  |    | duplicate;         | right rows                  |     |    |     //
//  |    | no dups in         V                             |     |    |     //
//  |    | TupleSpace +---------------+                     |     |    |     //
//  |    +-----<------|  FINISH LEFT  |<--------------------+     |    |     //
//  |    |            +------+--------+                           |    |     //
//  |    |                   |                                    |    |     //
//  |    |                   |              next left different   |    |     //
//  |    |                   +------------------->----------------+    |     //
//  |    |                   |                                         |     //
//  |    |                   V                                         |     //
//  |    |            saved duplicate                                  |     //
//  |    |              right rows                                     |     //
//  |    |                   |           TupleSpace empty              |     //
//  |    |                   +------------------->-------------------->+     //
//  |    |                   |                                         ^     //
//  V    ^                   V                                         |     //
//  |    |                   |                                         |     //
//  |    |                   |  TupleSpace   +-----------------+       |     //
//  |    |                   +-------------->| REWIND OVERFLOW |--->---+     //
//  |    |                      not empty    +--------+--------+             //
//  |    |                                            |                      //
//  |    |                                            V                      //
//  |    |                    next left is duplicate  |                      //
//  |    +-------------------------------<------------+                      //
//  |                                                                        //
//  |     +---------------+                                                  //
//  +---->|    CANCEL     |                                                  //
//        +------+--------+                                                  //
//               |                                                           //
//               | seen EOD on left and right table                          //
//               |                                                           //
//               V                                                           //
//        +---------------+                                                  //
//        |    EMPTY      |                                                  //
//        +---------------+                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

short ex_mj_tcb::work()
{
  // if no parent request, return
  if (qparent.down->isEmpty())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent.down->getHeadEntry();
  ex_mj_private_state &  pstate = *((ex_mj_private_state*) pentry_down->pstate);
  const ex_queue::down_request & request = pentry_down->downState.request;

  ExOperStats *statsEntry = getStatsEntry();

  while (true)
    {
      ex_expr::exp_return_type retCode;
      // if we have already given to the parent all the rows needed cancel the
      // parent's request. Also cancel it if the parent cancelled
      if ((pstate.step_ != ex_mj_tcb::MJ_CANCEL) &&
	  (pstate.step_ != ex_mj_tcb::MJ_DONE) &&
	  (pstate.step_ != ex_mj_tcb::MJ_DONE_NEVER_STARTED) &&
	  ((request == ex_queue::GET_NOMORE) ||
	   ((request == ex_queue::GET_N) &&
	    (pentry_down->downState.requestValue <= (Lng32)pstate.matchCount_))))
	{
	  if (pstate.step_ == ex_mj_tcb::MJ_UNINITIALIZED ||
              pstate.step_ == ex_mj_tcb::MJ_EMPTY)
            pstate.step_ = ex_mj_tcb::MJ_DONE_NEVER_STARTED;
          else
            {
	      qleft.down->cancelRequestWithParentIndex(
		   qparent.down->getHeadIndex());
	      qright.down->cancelRequestWithParentIndex(
		   qparent.down->getHeadIndex());
	      pstate.step_ = ex_mj_tcb::MJ_CANCEL;
            }
	}

      switch (pstate.step_)
	{
        case ex_mj_tcb::MJ_UNINITIALIZED:
          {
            pstate.init(this);
          }
	  break;  // case ex_mj_tcb::MJ_UNINITIALIZED:

        case ex_mj_tcb::MJ_EMPTY:
	  {
	    start(pstate);
	  }
	  break;  // case ex_mj_tcb::MJ_EMPTY:

       	case ex_mj_tcb::MJ_GET_LEFT:
	  {
	    if (qleft.up->isEmpty())
              {
                return WORK_OK;
              }

	    ex_queue_entry * lentry = qleft.up->getHeadEntry();
	    switch(lentry->upState.status)
	      {
	      case ex_queue::Q_OK_MMORE:
		{
		  if (noSavedDups())
		    {
		      // New row from left side. Get all matching rows
                      // from right.
                      pstate.step_ = ex_mj_tcb::MJ_GET_RIGHT;
		    }
		  else
                    {  // check for duplicate left row
                      atp_struct* leftAtp = lentry->getAtp();
                      retCode = ex_expr::EXPR_FALSE;
                      if (dupPool_->hasDups())
                      {
                        if (!dupPool_->current(*dupTupp_))
                          {
                            ex_assert(false,
                                      "No current duplicate right row "
                                      "in dupPool_");
                          }
                        retCode = leftCheckDupExpr()->eval(leftAtp, dupAtp_);
                      }
                      else
                      {  // get duplicate from TupleSpace
                        atp_struct* currentAtp = NULL;
                        IoStatus status = tspace_->current(currentAtp);
                        ex_assert((status != END_OF_DATA),
                                  "Duplicate right row not in TupleSpace");
                        if (status == OK)
                        {
                          retCode = leftCheckDupExpr()->eval(leftAtp,
                                                             currentAtp);
                        }
                        else if (status == IO_PENDING)
                        {
                          return WORK_OK;
                        }
                        else
                        {
                          // status is IO_ERROR or INTERNAL_ERROR
                          createDiags();
                          pstate.step_ = ex_mj_tcb::MJ_ERROR;
                          continue;
                        }
                      }  // get duplicate from TupleSpace

                      if (retCode == ex_expr::EXPR_TRUE)
			{
			  // Left row is a duplicate of the previous left row.
                          // Join left row with the saved right rows.
			  pstate.step_ = ex_mj_tcb::MJ_RETURN_SAVED_DUP_ROWS;
			}
		      else if (retCode == ex_expr::EXPR_ERROR)
                        {
                          if (!processError(leftAtp))
                            {
                              return WORK_OK;
                            }
                        }
                      else
                      {
                        // Left row differs from previous left row.
                        // Start new group of join output rows.
                        dupPool_->finishDups();
                        tspace_->discard();
                        pstate.step_ = MJ_GET_RIGHT;
                      }
		    }  // check for duplicate left row
		}
		break;

	      case ex_queue::Q_NO_DATA:
		{
		  // we're done, cancel the request for the right child if
		  // this is not done in parallel (with today's cancel logic,
		  // the cancel would apply to the other, parallel ESPs which
		  // is something we don't want)
		  pstate.step_ = ex_mj_tcb::MJ_CANCEL;
		  if (getGlobals()->getNumOfInstances() == 1)
		    qright.down->cancelRequestWithParentIndex(
			 qparent.down->getHeadIndex());
		}
		break;

	      case ex_queue::Q_SQLERROR:
                {
                  if (!processError(lentry->getAtp()))
                    {
                      return WORK_OK;
                    }
                }
                break;

	      case ex_queue::Q_INVALID:
		ex_assert(0, "ex_mj_tcb::work() "
                             "Invalid state returned by left child");
		break;
              }

	  }
	  break; // case ex_mj_tcb::MJ_GET_LEFT:

        case ex_mj_tcb::MJ_GET_RIGHT:
	  {
	    if (qright.up->isEmpty())
              {
	        return WORK_OK;
              }

	    ex_queue_entry * lentry = qleft.up->getHeadEntry();
	    ex_queue_entry * rentry = qright.up->getHeadEntry();
	    switch(rentry->upState.status)
	      {
              case ex_queue::Q_OK_MMORE:
               	{
                  Comparison compare = compareTuples(lentry, rentry);
                  if (compare == CMP_EQUAL)
		    {  // left and right rows matched

                      // Opportunistic look ahead:
                      // If it can be determined that the next left row will be
                      // different than the current left row, there is no need
                      // to save right duplicates.
                      if (lookAheadState_ == LA_LEFT_UNCHECKED)
                      {  // next left not checked
                        queue_index nextIndex = qleft.up->getHeadIndex() + 1;
                        if (qleft.up->entryExists(nextIndex))
                        {
                          ex_queue_entry* nextLeft
                            = qleft.up->getQueueEntry(nextIndex);
                          if (nextLeft->upState.status == ex_queue::Q_OK_MMORE)
                          {
                            compare = compareTuples(nextLeft, rentry, false);
                            if ((compare == CMP_EQUAL) || (compare == CMP_ERROR))
                            {
                              lookAheadState_ = LA_LEFT_DUPLICATE;
                            }
                            else
                            {
                              lookAheadState_ = LA_LEFT_DIFFERENT;
                            }
                          }
                          else if (nextLeft->upState.status == ex_queue::Q_NO_DATA)
                          {
                            lookAheadState_ = LA_LEFT_NO_DATA;
                          }
                          else
                          {
                            lookAheadState_ = LA_LEFT_ERROR;
                          }
                        }
                      }  // next left not checked
                      if ((lookAheadState_ == LA_LEFT_DUPLICATE)
                          ||(lookAheadState_ == LA_LEFT_UNCHECKED))
                      {
                        // Need to save duplicate right child rows.  Either the
                        // next left row is a duplicate of the current left row
                        // or the next left row is not yet available.
                        saveFirstDupAtp_ = true;
                        savedRetCode_ = ex_expr::EXPR_TRUE;  // skip self compare
                        pstate.step_ = ex_mj_tcb::MJ_SAVE_DUP_RIGHT;
                      }
                      else
                      {
                        // We don't need to save duplicate right child rows,
                        // since the next left child row either differs from
                        // the current left row or won't generate join results.
                        pstate.step_ = ex_mj_tcb::MJ_RETURN_ONE_ROW;
                      }
		    }  // left and right rows matched
                  else if (compare == CMP_LESS)
                    {
                      // Left row less than right row.
                      pstate.step_ = ex_mj_tcb::MJ_FINISH_LEFT;
                    }
                  else if (compare == CMP_GREATER)
                    {
                      // Consume the right row. It doesn't qualify.
                      qright.up->removeHead();
                    }
		  else
                    {
                      // Evaluation error
                      if (!processError(lentry->getAtp()))
                        {
                          return WORK_OK;
                        }
                    }
                }
                break;

              case ex_queue::Q_NO_DATA:
		{
		  // no more rows coming from right
		  if (isLeftJoin() || isAntiJoin())
		    {
		      // no more rows coming from right but we need to
		      // do left join or anti-semijoin processing
		      pstate.step_ = ex_mj_tcb::MJ_FINISH_LEFT;
		    }
		  else
		    {
		      // no need to look at more rows from the left,
		      // so cancel the left down queue request
		      pstate.step_ = ex_mj_tcb::MJ_CANCEL;
		      if (getGlobals()->getNumOfInstances() == 1)
			qleft.down->cancelRequestWithParentIndex(
			     qparent.down->getHeadIndex());
		    }
		}
		break;

              case ex_queue::Q_SQLERROR:
                {
                  if (!processError(rentry->getAtp()))
                    {
                      return WORK_OK;
                    }
                }
                break;

	      case ex_queue::Q_INVALID:
		ex_assert(0, "ex_mj_tcb::work() "
                             "Invalid state returned by right child");
		break;
	      }
	  }
	  break;  // case ex_mj_tcb::MJ_GET_RIGHT:

        case ex_mj_tcb::MJ_RETURN_ONE_ROW:
	  {
	    if (qparent.up->isFull())
              {
	        return WORK_OK;
              }

            // Semi-joins return the left row if it matches a right row
            // and the pre-join predicate is satisfied.  Once a left row
            // is returned, additional matching right rows are ignored.
            if (!(isSemiJoin() && pstate.outerMatched_))
            {  // semi-join test

              // Join left and right child row.  The left row is known
              // not to be followed by a duplicate left row so there is
              // no need to "replay" duplicate right rows, but the right
              // row still needs to be flattened to conform to the tuple
              // layout expected by our parent operator.
              atp_struct* leftAtp  = qleft.up->getHeadEntry()->getAtp();
              atp_struct* rightAtp = qright.up->getHeadEntry()->getAtp();
              if (dupPool_->getTuple(*dupTupp_))
              {
                retCode = rightCopyDupExpr()->eval(rightAtp,
                                                   dupAtp_);
                if (retCode == ex_expr::EXPR_ERROR)
                {
                  processError(rightAtp);  // qparent.up is not full
                }
              }
              else
              {
                return WORK_POOL_BLOCKED;
              }

              ex_expr::exp_return_type retCode = returnRow(leftAtp, dupAtp_,
                                                           pstate, false, statsEntry);
              if (retCode == ex_expr::EXPR_ERROR)
              {
                continue;
              }

            }  // semi-join test
            qright.up->removeHead();
            pstate.step_ = ex_mj_tcb::MJ_GET_RIGHT;
          }
          break;

        case ex_mj_tcb::MJ_SAVE_DUP_RIGHT:
        case ex_mj_tcb::MJ_SAVE_DUP_RIGHT_TSPACE:
	  {
	    if (qright.up->isEmpty())
              {
	        return WORK_OK;
              }

	    ex_queue_entry * rentry = qright.up->getHeadEntry();
            atp_struct* rightAtp = rentry->getAtp();
	    switch(rentry->upState.status)
	      {
	      case ex_queue::Q_OK_MMORE:
		{
                  if (savedRetCode_ == ex_expr::EXPR_OK)
                  {
                    retCode = rightCheckDupExpr()->eval(rightAtp,
                                                        prevRightAtp_);
                  }
                  else
                  {
                    // Use result of comparison done in MJ_GET_RIGHT
                    retCode = savedRetCode_;
                    savedRetCode_ = ex_expr::EXPR_OK;
                  }
                  if (retCode == ex_expr::EXPR_TRUE)
                    { // retCode == EXPR_TRUE
                      // Save the duplicate right row fragment.  Save a single
                      // right row fragment for a semi-join that lacks a
                      // pre-join predicate.  A semi-join verifies that at least
                      // one right row matches the left row, so a semi-join that
                      // lacks a pre-join predicate doesn't need to return right
                      // row fragments; however, the left row returned for a
                      // semi-join is generated by the MJ_RETURN_SAVED_DUP_ROWS
                      // state and its current implementation requires a saved
                      // duplicate right row.
		      if (!isSemiJoin() || preJoinExpr() || noSavedDups())
                      {
                        if (pstate.step_ == MJ_SAVE_DUP_RIGHT_TSPACE)
                        {
                          atp_struct *rtAtp;
                          IoStatus status = tspace_->insert(rightAtp, &rtAtp);
                          if (status == IO_PENDING)
                          {
                            return WORK_OK;  // insert didn't happen
                          }
                          else if (status == IO_ERROR)
                          {
                            createDiags();
                            pstate.step_ = ex_mj_tcb::MJ_ERROR;
                            continue;
                          }
                          else if ((status != OK) && (status != END_OF_DATA))
                          {
                            if (!processError(rightAtp))
                            {
                              return WORK_OK;
                            }
                            continue;
                          }
                          if (saveFirstDupAtp_ == true)
                          {
                            // save the first dup tuple now
                            //
                            // We would normally use copyAtp() call, but it's
                            // safe here to just keep the pointer as the dup
                            // tuple is only refernced within this tdb
                            prevRightAtp_ = rtAtp;
                            saveFirstDupAtp_ = false;
                          }
                        }
                        else if (dupPool_->getDupTuple(*dupTupp_))
                        {
                          retCode = rightCopyDupExpr()->eval(rightAtp,
                                                             dupAtp_);
                          if (retCode == ex_expr::EXPR_ERROR)
                          {
                            if (!processError(rightAtp))
                              {
                                return WORK_OK;
                              }
                          }
                          else if (saveFirstDupAtp_ == true)
                          {
                            // save the first dup tuple now
                            //
                            // We would normally use copyAtp() call, but it's
                            // safe here to just keep the pointer as the dup
                            // tuple is only refernced within this tdb
                            prevRightAtp_ = dupAtp_;
                            saveFirstDupAtp_ = false;
                          }
                        }
                        else
                        {
                          // Store remaining duplicate rows in TupleSpace.
                          pstate.step_ = MJ_SAVE_DUP_RIGHT_TSPACE;
                          savedRetCode_ = ex_expr::EXPR_TRUE;
                          continue;
                        }
                      }
		      qright.up->removeHead();
		    }  // retCode == EXPR_TRUE
		  else if (retCode == ex_expr::EXPR_ERROR)
                    {
                      if (!processError(rentry->getAtp()))
                      {
                        return WORK_OK;
                      }
                    }
		  else
                  {
                    // Found a non-duplicate right row.  Join the current
                    // left row with the saved right rows.
                    // prevRightAtp_->release();
                    dupPool_->rewind();
                    if (!tspace_->empty())
                    {
                      postIoStep_  = ex_mj_tcb::MJ_RETURN_SAVED_DUP_ROWS;
                      pstate.step_ = ex_mj_tcb::MJ_REWIND_OVERFLOW;
                    }
                    else
                    {
                      pstate.step_ = ex_mj_tcb::MJ_RETURN_SAVED_DUP_ROWS;
                    }
                  }
		}
		break;

	      case ex_queue::Q_NO_DATA:
		{
                  prevRightAtp_ = NULL;
                  dupPool_->rewind();
                  if (!tspace_->empty())
                  {
                    postIoStep_  = ex_mj_tcb::MJ_RETURN_SAVED_DUP_ROWS;
                    pstate.step_ = ex_mj_tcb::MJ_REWIND_OVERFLOW;
                  }
                  else
                  {
                    pstate.step_ = ex_mj_tcb::MJ_RETURN_SAVED_DUP_ROWS;
                  }
		}
		break;

	      case ex_queue::Q_SQLERROR:
                {
                  if (!processError(rightAtp))
                    {
                      return WORK_OK;
                    }
                }
                break;

	      case ex_queue::Q_INVALID:
		ex_assert(0, "ex_mj_tcb::work() "
                             "Invalid state returned by right child");
		break;

	      }
	  }
	  break; // case ex_mj_tcb::MJ_SAVE_DUP_RIGHT

        case ex_mj_tcb::MJ_REWIND_OVERFLOW:
        {
          IoStatus status = tspace_->rewind();
          if (status == IO_PENDING)
          {
            return WORK_OK;  // rewind didn't happen
          }
          else if (status == OK)
          {
            pstate.step_ = postIoStep_;
          }
          else
          {
            if (status == IO_ERROR)
            {
              createDiags();
            }
            pstate.step_ = ex_mj_tcb::MJ_ERROR;
            continue;
          }
        }
        break; // case ex_mj_tcb::MJ_REWIND_OVERFLOW

       	case ex_mj_tcb::MJ_RETURN_SAVED_DUP_ROWS:
        {
          // Left and right rows have matched and duplicate right rows
          // have been saved.  Remain in this state returning joined
          // rows until all saved duplicate right rows from dupPool_
          // have been joined with the left row.

          if (qparent.up->isFull())
            {
              return WORK_OK;
            }

          if (dupPool_->current(*dupTupp_))
          {
            ex_queue_entry * lentry = qleft.up->getHeadEntry();
            atp_struct* leftAtp = lentry->getAtp();
            retCode = returnRow(leftAtp, dupAtp_, pstate, false, statsEntry);
            if (retCode == ex_expr::EXPR_ERROR)
            {
              continue;
            }

            if (isSemiJoin() && pstate.outerMatched_)
            {
              // Semi-join only needs to match the left row once
              pstate.step_ = ex_mj_tcb::MJ_FINISH_LEFT;
              continue;
            }

            dupPool_->advance();
          }
          else
          {
            pstate.step_ = !tspace_->empty() ? MJ_RETURN_OVERFLOW_ROWS
                                             : MJ_FINISH_LEFT;
          }
        }
	  break; // case ex_mj_tcb::MJ_RETURN_SAVED_DUP_ROWS

       	case ex_mj_tcb::MJ_RETURN_OVERFLOW_ROWS:
        {
          // Remain in this state joining the left row with overflowed
          // duplicate right rows until all overflowed right row
          // fragments have been processed.  Duplicate right rows must
          // be copied into dupPool_ before they can be referenced by
          // a parent up queue entry.

          if (qparent.up->isFull())
          {
            return WORK_OK;
          }

          atp_struct* currentAtp = NULL;
          IoStatus status = tspace_->current(currentAtp);
          if (status == OK)
          {
            if (dupPool_->getTuple(*dupTupp_))
            {
              memcpy(dupTupp_->getDataPointer(),
                     currentAtp->getTupp(DUP_ATP_INDEX).getDataPointer(),
                     mjTdb().rightDupRecLen_);
            }
            else
            {
              // Failed to allocate dupPool_ entry
              return WORK_POOL_BLOCKED;
            }
          }
          else if (status == IO_PENDING)
          {
            return WORK_OK;
          }
          else if (status != END_OF_DATA)
          {
            if (status == IO_ERROR)
            {
              createDiags();
            }
            processError();  // qparent.up is not full
            continue;
          }

          if (status == OK)
          {  // have a duplicate right row
            ex_queue_entry * lentry = qleft.up->getHeadEntry();
            atp_struct* leftAtp = lentry->getAtp();
            retCode = returnRow(leftAtp, dupAtp_, pstate, false, statsEntry);
            if (retCode == ex_expr::EXPR_ERROR)
            {
              continue;
            }

            if (isSemiJoin() && pstate.outerMatched_)
            {
              // Semi-join only needs to match the left row once
              pstate.step_ = ex_mj_tcb::MJ_FINISH_LEFT;
              continue;
            }

            status = tspace_->advance();
            if (status != OK)
              {  // advance error
                if (status == END_OF_DATA)
                  {
                    pstate.step_ = ex_mj_tcb::MJ_FINISH_LEFT;
                  }
                else
                  {
                    if (status == IO_ERROR)
                    {
                      createDiags();
                    }
                    processError();  // qparent.up is not full
                    continue;
                  }
              }  // advance error
          }  // have a duplicate right row
          else
          {
            pstate.step_ = ex_mj_tcb::MJ_FINISH_LEFT;
          }
        }
	  break; // case ex_mj_tcb::MJ_RETURN_OVERFLOW_ROWS

        case ex_mj_tcb::MJ_FINISH_LEFT:
	  {
	    // check if we've got room in the up queue
	    if (qparent.up->isFull())
            {
	      return WORK_OK;
            }

	    ex_queue_entry * lentry = qleft.up->getHeadEntry();
	    ex_queue_entry * pentry = qparent.up->getTailEntry();

            if (!pstate.outerMatched_ && (isLeftJoin() || isAntiJoin()))
              {  // return left row to parent

                // For left joins, the parent up queue ATPs are
                // allocated with null tuple pointers for the
                // null-extended columns.
		pentry->copyAtp(lentry->getAtp());

                // If we have a nullPool, use the pre-allocated nullData.
                if(isLeftJoin() && ljExpr() && nullPool_) {
                  pentry->getAtp()->getTupp(pentry->numTuples() - 1) = nullData_;
                }

                retCode = ex_expr::EXPR_TRUE;
		if (postJoinExpr())
		  {
		    retCode = postJoinExpr()->eval(pentry->getAtp(), 0);
                  }

                if (retCode == ex_expr::EXPR_TRUE)
		  {
		    pentry->upState.status = ex_queue::Q_OK_MMORE;
                    pentry->upState.downIndex = qparent.down->getHeadIndex();
		    pentry->upState.parentIndex = pentry_down->downState.parentIndex;

		    pstate.matchCount_++;
                    pentry->upState.setMatchNo(pstate.matchCount_);

		    // insert into parent up queue
		    qparent.up->insert();
		  }
		else if (retCode == ex_expr::EXPR_FALSE)
                  {
                    // release the copied entry in parent's queue
                    pentry->getAtp()->release();
		  }
		else
                  {
                    processError();  // qparent.up is not full
                    continue;
                  }
	      }  // return left row to parent

            // consume the left row, we are done with it.
            qleft.up->removeHead();
            dupTupp_->release();
            pstate.outerMatched_ = false;

            switch(lookAheadState_)
            { // switch on lookAheadState_
              case LA_LEFT_DIFFERENT:
                pstate.step_ = MJ_GET_RIGHT;
                break;

              case LA_LEFT_NO_DATA:
                // We're done, cancel the request for the right child if
                // this is not done in parallel (with today's cancel logic,
                // the cancel would apply to the other, parallel ESPs which
                // is something we don't want).
                pstate.step_ = MJ_CANCEL;
                if (getGlobals()->getNumOfInstances() == 1)
                  qright.down->cancelRequestWithParentIndex(
                       qparent.down->getHeadIndex());
                break;

              case LA_LEFT_DUPLICATE:
              case LA_LEFT_UNCHECKED:
                {
                  ex_mj_tcb::mj_step tgtStep = MJ_GET_LEFT;
                  if (lookAheadState_ == LA_LEFT_DUPLICATE)
                  {
                    tgtStep = MJ_RETURN_SAVED_DUP_ROWS;
                  }
                  dupPool_->rewind();
                  if (!tspace_->empty())
                  {
                    postIoStep_ = tgtStep;
                    pstate.step_ = MJ_REWIND_OVERFLOW;
                  }
                  else
                  {
                    pstate.step_ = tgtStep;
                  }
                }
                break;

              case LA_LEFT_ERROR:
              default:
                pstate.step_ = MJ_GET_LEFT;
                break;
            }  // switch on lookAheadState_
            lookAheadState_ = LA_LEFT_UNCHECKED;
          }
	break; // case ex_mj_tcb::MJ_FINISH_LEFT:

        case ex_mj_tcb::MJ_ERROR:
          {
	    if (!processError())
            {
	      return WORK_OK;
            }
            continue;
          }
        break;

        case ex_mj_tcb::MJ_CANCEL:
	  {
            if (!noSavedDups())
            {
              dupPool_->finishDups();
              tspace_->discard();
            }

	    if (cancel(pstate) == WORK_OK)
            {
	      return WORK_OK;
            }
	  }
	break;

        case ex_mj_tcb::MJ_DONE:
        case ex_mj_tcb::MJ_DONE_NEVER_STARTED:
	  {
	    return stop(pstate);
	  }
	  break;
	} // switch on pstate.step_
    } // while
}

///////////////////////////////////////////////////////////////////////
// methods for class:  ex_mj_unique_tcb
///////////////////////////////////////////////////////////////////////
ex_mj_unique_tcb::ex_mj_unique_tcb(
     const ex_mj_tdb &  mj_tdb,  //
     const ex_tcb &    left_tcb,    // left queue pair
     const ex_tcb &    right_tcb,   // right queue pair
     ex_globals *glob
     ) : ex_mj_tcb( mj_tdb, left_tcb, right_tcb, glob)
{
}

short ex_mj_unique_tcb::work()
{
  // if no parent request, return
  if (qparent.down->isEmpty())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent.down->getHeadEntry();
  ex_mj_private_state &  pstate = *((ex_mj_private_state*) pentry_down->pstate);
  const ex_queue::down_request & request = pentry_down->downState.request;

  ExOperStats *statsEntry = getStatsEntry();

  while (true)
    {
      ex_expr::exp_return_type retCode = ex_expr::EXPR_OK;
      // if we have already given to the parent all the rows needed cancel the
      // parent's request. Also cancel it if the parent cancelled
      if ((pstate.step_ != ex_mj_tcb::MJ_CANCEL) &&
	  (pstate.step_ != ex_mj_tcb::MJ_DONE) &&
	  (pstate.step_ != ex_mj_tcb::MJ_DONE_NEVER_STARTED) &&
	  ((request == ex_queue::GET_NOMORE) ||
	   ((request == ex_queue::GET_N) &&
	    (pentry_down->downState.requestValue <= (Lng32)pstate.matchCount_))))
	{
	  if (pstate.step_ == ex_mj_tcb::MJ_UNINITIALIZED ||
              pstate.step_ == ex_mj_tcb::MJ_EMPTY)
            pstate.step_ = ex_mj_tcb::MJ_DONE_NEVER_STARTED;
          else
            {
	      qleft.down->cancelRequestWithParentIndex(qparent.down->getHeadIndex());
	      qright.down->cancelRequestWithParentIndex(qparent.down->getHeadIndex());
	      pstate.step_ = ex_mj_tcb::MJ_CANCEL;
            }
	}

      switch (pstate.step_)
	{
        case ex_mj_tcb::MJ_UNINITIALIZED:
          {
            pstate.init(this);
          }
	  break;  // case ex_mj_tcb::MJ_UNINITIALIZED:

        case ex_mj_tcb::MJ_EMPTY:
	  {
	    start(pstate);
	  }
	  break;

       	case ex_mj_tcb::MJ_GET_LEFT:
	  {
	    if (qleft.up->isEmpty())
	      return WORK_OK;

	    ex_queue_entry * lentry = qleft.up->getHeadEntry();
	    switch(lentry->upState.status)
	      {
	      case ex_queue::Q_OK_MMORE:
		{
		  pstate.step_ = ex_mj_tcb::MJ_GET_RIGHT;
		}
		break;

	      case ex_queue::Q_NO_DATA:
		{
		  if (getGlobals()->getNumOfInstances() == 1)
		    qright.down->cancelRequestWithParentIndex(
			 qparent.down->getHeadIndex());
		  pstate.step_ = ex_mj_tcb::MJ_CANCEL;
		}
		break;

	      case ex_queue::Q_SQLERROR:
                {
                  if (!processError(lentry->getAtp()))
                    return WORK_OK;
                }
                break;

	      case ex_queue::Q_INVALID:
		ex_assert(0, "ex_mj_unique_tcb::work() "
                             "Invalid state returned by left child");
		break;
              }
	  }
	  break;

	case ex_mj_tcb::MJ_GET_RIGHT:
	  {
	    if (qright.up->isEmpty())
	      return WORK_OK;

	    ex_queue_entry * lentry = qleft.up->getHeadEntry();
	    ex_queue_entry * rentry = qright.up->getHeadEntry();
	    switch(rentry->upState.status)
              {
                case ex_queue::Q_OK_MMORE:
                {
                  Comparison compare = compareTuples(lentry, rentry);
                  if (compare == CMP_EQUAL)
                    {
                      // the left and right rows have matched.
                      // Return this row.
                      pstate.step_ = ex_mj_tcb::MJ_RETURN_ROW;
                    }
                  else if (compare == CMP_LESS)
                    {
                      // Left row < right row.
                      // Consume left child and get next left row.
                      qleft.up->removeHead();
                      pstate.outerMatched_ = false;
                      pstate.step_ = ex_mj_tcb::MJ_GET_LEFT;
                    }
                  else if (compare == CMP_GREATER)
                    {
                      // Left row > right row.
                      // Consume the right row. It doesn't qualify.
                      qright.up->removeHead();
                    }
                  else
                    {
                      // An eval error occurred, since compare is CMP_ERROR.
                      if (!processError(rentry->getAtp()))
                        return WORK_OK;
                    }
                }
                break;

              case ex_queue::Q_NO_DATA:
		{
		  if (getGlobals()->getNumOfInstances() == 1)
		    qleft.down->cancelRequestWithParentIndex(
			 qparent.down->getHeadIndex());
		  pstate.step_ = ex_mj_tcb::MJ_CANCEL;
		}
		break;

              case ex_queue::Q_SQLERROR:
                {
                  if (!processError(rentry->getAtp()))
                    return WORK_OK;
                }
                break;

	      case ex_queue::Q_INVALID:
		ex_assert(0, "ex_mj_unique_tcb::work() "
                             "Invalid state returned by right child");
		break;
	      }
	  }
	break;

       	case ex_mj_tcb::MJ_RETURN_ROW:
	  {
	    // At this point, left and right rows have matched (equi-join).

	    if (qparent.up->isFull())
	      return WORK_OK;

            atp_struct* leftAtp  = qleft.up->getHeadEntry()->getAtp();
            atp_struct* rightAtp = qright.up->getHeadEntry()->getAtp();
            retCode = returnRow(leftAtp, rightAtp, pstate, true, statsEntry);

            if (retCode == ex_expr::EXPR_ERROR)
              {
                continue;
              }

	    // consume left and/or right rows
	    if (mjTdb().isLeftUnique())
	      {
		qright.up->removeHead();
		pstate.step_ = ex_mj_tcb::MJ_GET_RIGHT;
	      }

	    if (mjTdb().isRightUnique())
	      {
		qleft.up->removeHead();
                pstate.outerMatched_ = false;
		pstate.step_ = ex_mj_tcb::MJ_GET_LEFT;
	      }
	  }
	break;

        case ex_mj_tcb::MJ_CANCEL:
	  {
	    if (cancel(pstate) == WORK_OK)
	      return WORK_OK;
	  }
	break;

        case ex_mj_tcb::MJ_DONE:
        case ex_mj_tcb::MJ_DONE_NEVER_STARTED:
	  {
	    return stop(pstate);
	  }
	break;

	} // switch pstate.step
    } // while

  return WORK_OK;
}

///////////////////////////////////////////////////////////////////////////
//  Private state procedures
///////////////////////////////////////////////////////////////////////////

// Constructor and destructor for mj_private_state
//
ex_mj_private_state::ex_mj_private_state() :
  matchCount_(0),
  step_(ex_mj_tcb::MJ_UNINITIALIZED),
  outerMatched_ (false)
{}

// Init is called just once
void ex_mj_private_state::init(const ex_mj_tcb * tcb)
{
  matchCount_ = 0;  // number of rows returned for this parent row
  step_ = ex_mj_tcb::MJ_EMPTY;
  outerMatched_ = false;
}

ex_mj_private_state::~ex_mj_private_state()
{
};

const char*
ex_mj_private_state::currentState(void) const
{
  return stateName(step_);
}

const char*
ex_mj_private_state::stateName(ex_mj_tcb::mj_step mjStep)
{
  Int32 i = static_cast<Int32>(mjStep);

  return stateNames[i];
}

// stateNames must be in the same order as ex_mj_tcb::mj_step
const char* const
ex_mj_private_state::stateNames[]
    = {"MJ_UNINITIALIZED",
       "MJ_EMPTY",
       "MJ_GET_LEFT",
       "MJ_GET_RIGHT",
       "MJ_RETURN_ROW",
       "MJ_RETURN_ONE_ROW",
       "MJ_SAVE_DUP_RIGHT",
       "MJ_SAVE_DUP_RIGHT_TSPACE",
       "MJ_REWIND_OVERFLOW",
       "MJ_RETURN_SAVED_DUP_ROWS",
       "MJ_RETURN_OVERFLOW_ROWS",
       "MJ_FINISH_LEFT",
       "MJ_ERROR",
       "MJ_CANCEL",
       "MJ_DONE",
       "MJ_DONE_NEVER_STARTED"
      };
