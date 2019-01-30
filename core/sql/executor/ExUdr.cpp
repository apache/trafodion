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
 * File:         ExUdr.cpp
 * Description:  TDB/TCB for user-defined routines
 *               
 * Created:      2/8/2000
 * Language:     C++
 *
 *****************************************************************************
 */

//
// ***TBD 
// - Reviewers have noted that error handling following IPC errors may
//   be too heavy-handed. The TCB places diagnostics into the statement
//   globals area and often returns WORK_BAD_ERROR. It should be
//   possible to associate the diags with an up queue entry and return
//   all errors and diagnostics via the queues. One reason why this
//   enhancement has not been made is that it is not clear when the
//   server should be restarted following an IPC error. Currently the
//   code lets the Statement object deallocate TCBs and redrive fixup in
//   order to acquire a new UDR server. If work methods become
//   responsible for restarting the server, I'm not sure how a work
//   method would decide to initiate the restart.
//

#include "ex_stdh.h"
#include "ExUdr.h"
#include "ExUdrServer.h"
#include "ExUdrClientIpc.h"
#include "UdrExeIpc.h"
#include "ex_exe_stmt_globals.h"
#include "exp_attrs.h"
#include "ex_error.h"
#include "ExStats.h"
#include "ExCextdecs.h"
#include "UdrFormalParamInfo.h"
#include "udrtabledescinfo.h"
#include "Statement.h"
#include "ExRsInfo.h"
#include "Descriptor.h"
#include "ExExeUtil.h"
#include <sys/stat.h>
#define TF_STRING(x) ((x) ? ("TRUE") : ("FALSE"))
#define YN_STRING(x) ((x) ? ("YES") : ("NO"))

#ifdef UDR_DEBUG
static const char *GetExpStatusString(ex_expr::exp_return_type s)
{
  switch (s)
  {
    case ex_expr::EXPR_OK:         return "EXPR_OK";
    case ex_expr::EXPR_ERROR:      return "EXPR_ERROR";
    case ex_expr::EXPR_TRUE:       return "EXPR_TRUE";
    case ex_expr::EXPR_FALSE:      return "EXPR_FALSE";
    case ex_expr::EXPR_NULL:       return "EXPR_NULL";
    default:                       return ComRtGetUnknownString((Int32) s);
  }
}
static const char *GetWorkRetcodeString(ExWorkProcRetcode r)
{
  switch (r)
  {
    case WORK_OK:                    return "WORK_OK";
    case WORK_CALL_AGAIN:            return "WORK_CALL_AGAIN";
    case WORK_POOL_BLOCKED:          return "WORK_POOL_BLOCKED";
    case WORK_RESCHEDULE_AND_RETURN: return "WORK_RESCHEDULE_AND_RETURN";
    case WORK_BAD_ERROR:             return "WORK_BAD_ERROR";
    default:                         return ComRtGetUnknownString((Int32) r);
  }
}
static const char *GetUpStatusString(ex_queue::up_status s)
{
  switch (s)
  {
    case ex_queue::Q_NO_DATA:        return "Q_NO_DATA";
    case ex_queue::Q_OK_MMORE:       return "Q_OK_MMORE";
    case ex_queue::Q_SQLERROR:       return "Q_SQLERROR";
    case ex_queue::Q_INVALID:        return "Q_INVALID";
    case ex_queue::Q_GET_DONE:       return "Q_GET_DONE";
    default:                         return ComRtGetUnknownString((Int32) s);
  }
}

static void TransIdToText(Int64 transId, char *buf, Int32 len)
{
  ex_assert(len > 100, "Buffer passed to TransIdToText is too small");

  if (transId == -1)
  {
    sprintf(buf, "-1");
    return;
  }

  short actualLen;
  short error;
  error = TRANSIDTOTEXT(transId, buf, (short) (len - 1), &actualLen);
  if (error)
    str_sprintf(buf, "(error %d)", (Int32) error);
  else
    buf[actualLen] = 0;
}

#define UdrDebug0(s) \
 if (doTrace_) UdrPrintf(traceFile_,(s))
#define UdrDebug1(s,a1) \
 if (doTrace_) UdrPrintf(traceFile_,(s),(a1))
#define UdrDebug2(s,a1,a2) \
 if (doTrace_) UdrPrintf(traceFile_,(s),(a1),(a2))
#define UdrDebug3(s,a1,a2,a3) \
 if (doTrace_) UdrPrintf(traceFile_,(s),(a1),(a2),(a3)) 
#define UdrDebug4(s,a1,a2,a3,a4) \
 if (doTrace_) UdrPrintf(traceFile_,(s),(a1),(a2),(a3),(a4))
#define UdrDebug5(s,a1,a2,a3,a4,a5) \
 if (doTrace_) UdrPrintf(traceFile_,(s),(a1),(a2),(a3),(a4),(a5))
#define UdrDebug6(s,a1,a2,a3,a4,a5,a6)	\
  if (doTrace_) UdrPrintf(traceFile_,(s),(a1),(a2),(a3),(a4),(a5),(a6))

#else
//
// Debug macros are no-ops in the release build
//
#define UdrDebug0(s)
#define UdrDebug1(s,a1)
#define UdrDebug2(s,a1,a2)
#define UdrDebug3(s,a1,a2,a3)
#define UdrDebug4(s,a1,a2,a3,a4)
#define UdrDebug5(s,a1,a2,a3,a4,a5)
#define printDataStreamState()

#endif // UDR_DEBUG

//
// UDR TCBs use this function to acquire process-wide unique UDR
// handles. JULIANTIMESTAMP(3) returns the number of microseconds
// since cold load. This value is not affected by setting the system
// clock. On Windows we use a static counter because JULIANTIMESTAMP is
// not always reliable due to occasional clock synchronization
// performed by the NonStop Cluster services.
//
static void InitializeUdrHandle(Int64 &handle)
{
  static Int64 nextHandle = 0;
  nextHandle++;
  handle = nextHandle;
}

//----------------------------------------------------------------------
// ExUdrTdb methods
//----------------------------------------------------------------------
ex_tcb *ExUdrTdb::build(ex_globals *glob)
{
  // first build the child TCBs
  //
  const ex_tcb **childTcbs = (const ex_tcb**) new (glob->getSpace()) ex_tcb* [numChildren()];
  short i;
  for (i=0; i<numChildren(); i++)
    {
      childTcbs[i] = childTdbs_[i]->build(glob);
    }

  ExUdrTcb *tcb = new (glob->getSpace()) ExUdrTcb(*this, childTcbs,glob);
  return tcb;
}

//----------------------------------------------------------------------
// Allocate stats area for this tcb
//----------------------------------------------------------------------
ExOperStats * ExUdrTcb::doAllocateStatsEntry(CollHeap *heap,
                                                    ComTdb *tdb)
{
  ExUDRBaseStats * stat = NULL;
  ComTdb::CollectStatsType statsType = getGlobals()->getStatsArea()->getCollectStatsType();
  ExUdrTdb * udrTdb = (ExUdrTdb *) tdb;
  if (statsType == ComTdb::OPERATOR_STATS)
  {
    stat =  new(heap) ExUDRBaseStats(heap,
                        this,
                        tdb);
  }
  else
  {
    stat = new(heap) ExUDRStats(heap,
                        udrTdb->getRequestSqlBufferSize(),
                        udrTdb->getReplySqlBufferSize(),
                        tdb,
                        this);
    udrStats_ = (ExUDRStats *) stat;
  }
  udrBaseStats_ = stat;
  return stat;
}

//----------------------------------------------------------------------
// ExUdrTcb methods
//----------------------------------------------------------------------
ExUdrTcb::ExUdrTcb(const ExUdrTdb &udrTdb, 
		   const ex_tcb **childTcbs, 
		   ex_globals *glob)
  : ex_tcb(udrTdb, 1, glob),
    state_(BUILD),
    qParent_(),
    workAtp_(NULL),
    outputPool_(NULL),
    inputPool_(NULL),
    replyBuffer_(NULL),
    requestBuffer_(NULL),
    childInputBuffers_(NULL),
    nextToSend_(0),
    udrServer_(NULL),
    dataStream_(NULL),
    outstandingControlStream_(NULL),
    udrHandle_(INVALID_UDR_HANDLE),
    ioSubtask_(NULL),
    serverProcessId_(),
    rsHandle_(INVALID_RS_HANDLE),
    rsIndex_(0),
    rsInfo_(NULL),
    childTcbs_(childTcbs),
    qChild_(0),
    udrBaseStats_(NULL),
    udrStats_(NULL),
    dataMsgsSent_(0),
    continueMsgsSent_(0)
#ifdef UDR_DEBUG
    , doTrace_(FALSE), doStateTrace_(FALSE), doIpcTrace_(FALSE)
    , traceFile_(NULL)
    , trustReplies_(FALSE)
#endif
{
#ifdef UDR_DEBUG
  initializeDebugVariables();
#endif

  UdrDebug1("[BEGIN TCB CONSTRUCTOR] %p", this);

  ex_assert(getGlobals() && getGlobals()->castToExExeStmtGlobals(),
            "Invalid statement globals pointer in ExUdrTcb constructor");
  ExExeStmtGlobals *stmtGlobals = myExeStmtGlobals();
  NABoolean isResultSet = udrTdb.isResultSetProxy();

  if (isResultSet)
  {
    ExMasterStmtGlobals *masterStmtGlobals =
      stmtGlobals->castToExMasterStmtGlobals();
    ex_assert(masterStmtGlobals,
              "Must be ExMasterStmtGlobals to support UDR result set");
    
    masterStmtGlobals->acquireRSInfoFromParent(rsIndex_, udrHandle_,
                                               udrServer_, serverProcessId_,
                                               rsInfo_);

    InitializeUdrHandle(rsHandle_);
    ex_assert(RSHandleIsValid(rsHandle_),
              "Invalid RS handle generated by ExUdrTcb constructor");
    
#ifdef UDR_DEBUG
    char buf[300];
    serverProcessId_.toAscii(buf, 300);
    UdrDebug0("  *** This is a result set proxy");
    UdrDebug2("  *** RS index %u, RS handle " INT64_PRINTF_SPEC,
              rsIndex_, rsHandle_);
    UdrDebug2("  *** ExUdrServer %p, process ID %s", udrServer_, buf);
#endif
  }
  else
  {
    InitializeUdrHandle(udrHandle_);
    ex_assert(UdrHandleIsValid(udrHandle_),
              "Invalid UDR handle generated by ExUdrTcb constructor");
  }

  UdrDebug1("  UDR handle: " INT64_PRINTF_SPEC, udrHandle_);
  UdrDebug5("  %u params, %u input value%s, %u output value%s",
            udrTdb.getNumParams(),
            udrTdb.getNumInputValues(),
            PLURAL_SUFFIX(udrTdb.getNumInputValues()),
            udrTdb.getNumOutputValues(),
            PLURAL_SUFFIX(udrTdb.getNumOutputValues()));
  UdrDebug3("  Row length: %u request, %u reply, %u output",
            udrTdb.getRequestRowLen(), udrTdb.getReplyRowLen(),
            udrTdb.getOutputRowLen());
  UdrDebug2("  SqlBuffer size: %u request, %u reply",
            udrTdb.getRequestSqlBufferSize(), udrTdb.getReplySqlBufferSize());
  UdrDebug1("  Statement globals %p", getGlobals());
  UdrDebug1("  Transaction required: %s",
            udrTdb.getTransactionAttrs() == COM_TRANSACTION_REQUIRED ?
            "YES" : "NO");

  if (!isResultSet)
  {
    UdrDebug1("  SQL name: %s", udrTdb.getSqlName());

    ComRoutineParamStyle paramStyle = udrTdb.getParamStyle();
    switch (paramStyle)
    {
      case COM_STYLE_JAVA_CALL:
        UdrDebug0("  Parameter style JAVA (call)");
        break;
      case COM_STYLE_JAVA_OBJ:
        UdrDebug0("  Parameter style JAVA (object)");
        break;
      case COM_STYLE_SQL:
        UdrDebug0("  Parameter style SQL");
        break;
      case COM_STYLE_SQLROW:
        UdrDebug0("  Parameter style SQLROW");
        break;
      case COM_STYLE_SQLROW_TM:
        UdrDebug0("  Parameter style SQLROW_TM");
        break;
      case COM_STYLE_CPP_OBJ:
        UdrDebug0("  Parameter style C++");
        break;
      default:
        ex_assert(0, "Invalid PARAMETER STYLE");
        break;
    }
    
    ComRoutineLanguage language = udrTdb.getLanguage();
    switch (language)
    {
      case COM_LANGUAGE_JAVA:
        UdrDebug0("  Language JAVA");
        UdrDebug1("  Runtime options '%s'",
                  udrTdb.getRuntimeOptions() ?
                  udrTdb.getRuntimeOptions() : "");
        UdrDebug1("  Option delimiters '%s'",
                  udrTdb.getRuntimeOptionDelimiters() ?
                  udrTdb.getRuntimeOptionDelimiters() : "");
        break;
      case COM_LANGUAGE_C:
        UdrDebug0("  Language C");
        break;
      case COM_LANGUAGE_CPP:
        UdrDebug0("  Language C++");
        break;
      default:
        ex_assert(0, "Invalid LANGUAGE");
        break;
    }
  } // if (!isResultSet)

  Space *globSpace = getSpace();
  CollHeap *globHeap = getHeap();

  // Allocate output buffer pool
  if (udrTdb.getOutputExpression() != NULL)
  {
    UdrDebug2("  Output pool: %u buffers of size %u",
              (ULng32) udrTdb.getNumOutputBuffers(),
              (ULng32) udrTdb.getOutputSqlBufferSize());

    outputPool_ = new (globSpace)
      sql_buffer_pool((Lng32) udrTdb.getNumOutputBuffers(),
                      (Lng32) udrTdb.getOutputSqlBufferSize(),
                      globSpace,
                      SqlBufferBase::NORMAL_);
  }
  
 // Allocate input buffer pool
  if (udrTdb.getInputExpression() != NULL)
  {
    UdrDebug2("  Input pool: %u buffers of size %u",
              (ULng32) udrTdb.getNumInputBuffers(),
              (ULng32) udrTdb.getInputSqlBufferSize());

    inputPool_ = new (globSpace)
      sql_buffer_pool((Lng32) udrTdb.getNumInputBuffers(),
                      (Lng32) udrTdb.getInputSqlBufferSize(),
                      globSpace,
                      SqlBufferBase::NORMAL_);
  }
  if (numChildren())
    {
      childInputBuffers_ = (UdrDataBuffer **) new (globSpace) UdrDataBuffer *[numChildren()];
      // Initialize them all to NULL
      for (Int32 i = 0; i < numChildren(); i++)
	childInputBuffers_[i] = NULL;
    }
  

  // Allocate queues to communicate with parent
  allocateParentQueues(qParent_);
  // if it is a tmudf tcb then allocate the child queues too
  if (myTdb().isTmudf())
    {
      

      // Allocate array of child queue pairs (i.e., queue pairs for
      // communicating with children.
      //
      qChild_ = new (globSpace) ex_queue_pair[numChildren()];
      tmudfStates_ = new (globSpace) TmudfState[numChildren()];

      // Initialize array.
      //
      Int32 i;
      for (i=0; i<numChildren(); i++)
	{
	  qChild_[i] = childTcbs_[i]->getParentQueue();
          tmudfStates_[i] = INITIAL;
	}
    }

  // During the lifetime of this TCB when down queue entries exist
  // that have been sent to the UDR Server but for which not all up
  // queue entries have been generated, qParent_.down->getHeadIndex()
  // will not be equal to nextToSend_. We maintain this relationship
  // by incrementing nextToSend_ each time a down queue entry is
  // processed. An entry is considered "processed" when it gets copied
  // into a message buffer that gets sent to the UDR Server, or when
  // the copy step generates errors, or when we notice the entry has
  // been cancelled before it was sent.
  nextToSend_ = qParent_.down->getHeadIndex();

  // Allocate the work ATP
  if (udrTdb.getWorkCriDesc())
    workAtp_ = allocateAtp(udrTdb.getWorkCriDesc(), globSpace);

  // Fixup expressions
  if (udrTdb.getInputExpression())
    udrTdb.getInputExpression()->fixup(0, getExpressionMode(), this,
                                       globSpace, globHeap, FALSE, glob);
  
  if (udrTdb.getOutputExpression())
    udrTdb.getOutputExpression()->fixup(0, getExpressionMode(), this,
                                        globSpace, globHeap, FALSE, glob);
 
  Int32 i;
  for (i=0; i<numChildren(); i++)
    {
      if (udrTdb.getChildInputExpr(i))
        udrTdb.getChildInputExpr(i)->fixup(0,getExpressionMode(),this,
                                           globSpace, globHeap, FALSE, glob);
    }
  if (udrTdb.getPredicate())
    udrTdb.getPredicate()->fixup(0, getExpressionMode(), this,
                                 globSpace, globHeap, FALSE, glob);
  
  // Register subtasks with the scheduler
  registerSubtasks();
  registerResizeSubtasks();
  
  
  if (!isResultSet)
  {
    // We now create an ExUdrServer instance and store a pointer to it
    // in this TCB. If the TDB does not contain runtime option strings
    // (which would be true for a plan compiled prior to release 2), we
    // establish meaningful default values for those strings here.
    const char *options = udrTdb.getRuntimeOptions();
    const char *delims = udrTdb.getRuntimeOptionDelimiters();
    if (options == NULL || strlen(options) == 0)
      options = "OFF";
    if (delims == NULL || strlen(delims) == 0)
      delims = " ";
    NABoolean dedicated = FALSE;
    if (myTdb().isTmudf())
      dedicated = TRUE;
    udrServer_ = stmtGlobals->acquireUdrServer(options, delims, dedicated);
  }

  ex_assert(udrServer_, "No ExUdrServer available for this TCB");
  UdrDebug2("  This TCB will use ExUdrServer %p, ref count %d",
            udrServer_, udrServer_->getRefCount());

  UdrDebug1("[END TCB CONSTRUCTOR] %p\n", this);

} // ExUdrTcb::ExUdrTcb

ExUdrTcb::~ExUdrTcb()
{
  UdrDebug1("[BEGIN TCB DESTRUCTOR] %p", this);
  
  //
  // Release resources acquired during fixup
  //
  freeResources();
  
  //
  // Release resources allocated by the constructor. The queues were
  // allocated by calling ex_tcb::allocateParentQueues() but there is
  // no deallocateParentQueues() function so we simply call
  // destructors for the queues here.
  //
  delete qParent_.up;
  delete qParent_.down;
  delete outputPool_;
  if (workAtp_)
  {
    workAtp_->release();
    deallocateAtp(workAtp_, getSpace());
  }

  setUdrTcbState(DONE);
  UdrDebug1("[END TCB DESTRUCTOR] %p\n", this);

} // ExUdrTcb::~ExUdrTcb()

//
// This function frees any resources acquired during fixup.
// It should only be called by the TCB destructor.
//
void ExUdrTcb::freeResources()
{
  UdrDebug1("  [BEGIN TCB FREE RESOURCES] %p", this);
  releaseControlStream();
  releaseDataStream();
  releaseServerResources();
  releaseConnectionToServer();
  UdrDebug1("  [END TCB FREE RESOURCES] %p", this);
}

ex_tcb_private_state *ExUdrTcb::allocatePstates(
  Lng32 &numElems,      // [IN/OUT] desired/actual elements
  Lng32 &pstateLength)  // [OUT] length of one element
{
  PstateAllocator<ExUdrPrivateState> pa;
  return pa.allocatePstates(this, numElems, pstateLength);
}

void ExUdrTcb::registerSubtasks()
{
  ExScheduler *sched = getGlobals()->getScheduler();

  // Cancel events are seen by workCancel()
  sched->registerCancelSubtask(sWorkCancel, this, qParent_.down, "Cancel");

  if (myTdb().isTmudf())
    {
      // Up queue events are seen by tmudfWork()
      sched->registerUnblockSubtask(sTmudfWork, this, qParent_.up, "Up");

      // Down queue events are seen by tmudfWork()
      sched->registerInsertSubtask(sTmudfWork, this, qParent_.down, "Down");

      // I/O events are seen by tmudfWork()
      ioSubtask_ = sched->registerNonQueueSubtask(sTmudfWork, this, "I/O Events");
      Int32 i;
      for (i=0; i<numChildren(); i++)
	{
	  sched->registerUnblockSubtask(sTmudfWork, this, qChild_[i].down,"Down");
	  sched->registerInsertSubtask(sTmudfWork, this, qChild_[i].up, " Up");
	}
    }
  else
    { 
      // Up queue events are seen by work()
      sched->registerUnblockSubtask(sWork, this, qParent_.up, "Up");

      // Down queue events are seen by work()
      sched->registerInsertSubtask(sWork, this, qParent_.down, "Down");

      // I/O events are seen by work()
      ioSubtask_ = sched->registerNonQueueSubtask(sWork, this, "I/O Events");
    }
}

//----------------------------------------------------------------------
// TCB accessor methods
//----------------------------------------------------------------------
ExExeStmtGlobals *ExUdrTcb::myExeStmtGlobals() const
{
  return ((ExUdrTcb *) this)->getGlobals()->castToExExeStmtGlobals();
}  

IpcEnvironment *ExUdrTcb::myIpcEnv() const
{
  return myExeStmtGlobals()->getIpcEnvironment();
}

CollHeap *ExUdrTcb::getIpcHeap() const
{
  return myIpcEnv()->getHeap();
}

ComDiagsArea *ExUdrTcb::getStatementDiags() const
{
  return myExeStmtGlobals()->getDiagsArea();
}

void ExUdrTcb::setStatementDiags(ComDiagsArea *d) const
{
  myExeStmtGlobals()->setGlobDiagsArea(d);
}

ComDiagsArea *ExUdrTcb::getOrCreateStmtDiags() const
{
  ComDiagsArea *result = getStatementDiags();
  if (result == NULL)
  {
    result = ComDiagsArea::allocate(((ExUdrTcb *)this)->getHeap());
    setStatementDiags(result);
    result->decrRefCount();
  }
  return result;
}

//----------------------------------------------------------------------
// TCB fixup
// Non-zero return value indicates errors
//----------------------------------------------------------------------
Int32 ExUdrTcb::fixup()
{
#ifdef UDR_DEBUG
  initializeDebugVariables();
#endif // UDR_DEBUG

  UdrDebug1("[BEGIN TCB FIXUP] %p", this);
  setUdrTcbState(FIXUP);
  // CliGlobals *cliGlobals = getGlobals()->castToExExeStmtGlobals()->
  // castToExMasterStmtGlobals()->getCliGlobals();
  CliGlobals *cliGlobals = GetCliGlobals();
  ExeCliInterface cliInterface(getHeap(), 0, cliGlobals->currContext());
  //
  // Non-zero return value indicates an error
  //
  const Int32 FIXUP_SUCCESS = 0;
  const Int32 FIXUP_ERROR = 1;

  // The result variable will have the value FIXUP_ERROR until we are
  // sure the following are true
  // - this TCB has a running server
  // - a nowait LOAD request has been sent to the server
  Int32 result = FIXUP_ERROR;

  NABoolean isResultSet = myTdb().isResultSetProxy();
  

  // The code is written to retry fixup for this TCB some number of
  // times.  Retrying fixup would allow us to send a LOAD message to a
  // server that we think is running, then detect that the server has
  // died, and then start a new server and send it the LOAD
  // message. This sounds like a good idea but there is a catch.  The
  // MX IPC code will only detect that the server died when it calls
  // AWAITIOX to complete IO. We do not detect a dead server when we
  // are sending data. Once we call AWAITIOX to complete IO to a dead
  // server we experience transaction abort (if there was a
  // transaction active) and AWAITIOX returns error 201 which is
  // FEPATHDOWN. Once our transaction is gone it does us no good to
  // start a new server without first starting a new transaction and
  // it is not this TCB's job to start transactions. Thus we are not
  // going to retry fixup in this TCB. If bad things happen during the
  // fixup phase then it is probably true that a transaction was
  // aborted and that fixup should not be retried. 

  // To prevent fixup retries we set the numRetries variable to zero
  // before entering the following for loop.
  UInt32 numRetries = 0;

  for (UInt32 i = 0; i <= numRetries && result != FIXUP_SUCCESS; i++)
  {
    if (state_ != FIXUP)
    {
      setUdrTcbState(FIXUP);
    }

    //
    // We call ExUdrServer::start() to start a new UDR server if one
    // is not already running. start() does nothing and returns
    // EX_UDR_SUCCESS if one is already running.
    //
    ComDiagsArea *da = myExeStmtGlobals()->getDiagsArea();
    CollHeap *diagsHeap = getHeap();
    ComDiagsArea *originalDa = da;

    // Determine whether the UDR control connection should carry
    // transactional requests. Transactional requests are currently
    // only needed in the master executor. UDFs running from ESPs do
    // not send transactional requests.
    NABoolean usesTransactions = TRUE;
    if (myExeStmtGlobals()->castToExEspStmtGlobals())
      usesTransactions = FALSE;
    
    if (!isResultSet && udrStats_ != NULL)
    { 
      Int64 serverInit = NA_JulianTimestamp();
      udrStats_->setUDRServerInit(serverInit);
    }
    
#ifdef UDR_DEBUG
    if (doTrace_ && traceFile_)
      udrServer_->setTraceFile(traceFile_);
#endif
    
    // Determine the transid we should send in the message. We use the
    // statement transid only if the TDB says we require a
    // transaction.
    Int64 &stmtTransId = myExeStmtGlobals()->getTransid();
    Int64 transIdToSend = -1; // -1 is an invalid transid
    if (myTdb().getTransactionAttrs() == COM_TRANSACTION_REQUIRED)
      transIdToSend = stmtTransId;
    
    // For CALL stmts, start UDR Server if it's not already started.
    // For RS stmts, UDR Server must have been started by parent call.
    ExUdrServer::ExUdrServerStatus status = ExUdrServer::EX_UDR_SUCCESS;
    if (!isResultSet)
      status = udrServer_->start(&da, diagsHeap, transIdToSend,
                                 serverProcessId_, usesTransactions);
    
    if (originalDa == NULL && da != NULL)
    {
      //
      // This indicates that before the start() call our statement
      // globals did not have a diags area but during the start() 
      // call one was created. We need a pointer to this new diags
      // area in our statement globals. Note that the diags area can
      // contain warnings only, so we need to do this even if start()
      // was successful.
      //
      myExeStmtGlobals()->setGlobDiagsArea(da);
      da->decrRefCount();
    }
    
    if (status == ExUdrServer::EX_UDR_SUCCESS)
    {
#ifdef UDR_DEBUG
      char buf[300];
      serverProcessId_.toAscii(buf, 300);
      UdrDebug1("  [FIXUP] This TCB will use server process %s", buf);
#endif // UDR_DEBUG

      if (!isResultSet )
      {
        if (udrBaseStats_)
        {
          char bufForServer[300];
          serverProcessId_.toAscii(bufForServer, 300);
          udrBaseStats_->setUDRServerId( (char *) bufForServer, 300);
        }
        if (udrStats_)
        {
          Int64 serverStart = NA_JulianTimestamp();
          udrStats_->castToExUDRStats()->setUDRServerStart(serverStart);
        }
      }

      ExExeStmtGlobals *stmtGlobals = myExeStmtGlobals();

      // If this is a CALL that produces result sets, then store
      // information about this TCB in the statement's ExRsInfo object.
      if (myTdb().getMaxResultSets() > 0)
      {
        ExMasterStmtGlobals *masterStmtGlobals =
          stmtGlobals->castToExMasterStmtGlobals();
        rsInfo_ = masterStmtGlobals->getResultSetInfo(TRUE);
        ex_assert(rsInfo_, "No ExRsInfo object available");

        rsInfo_->setNumEntries(myTdb().getMaxResultSets());
        rsInfo_->setUdrHandle(udrHandle_);
        rsInfo_->setUdrServer(udrServer_);
        rsInfo_->setIpcProcessId(serverProcessId_);
      }

      // Set correct IpcConnection in stmt globals.
      // For SPJ with RS, create a new connection. For SPJ that does
      // not return RS, stmt globals do not store connection pointer, but
      // stmt globals return control connection from udrServer_ on demand.
      if (!isResultSet)
      {
        if (myTdb().getMaxResultSets() > 0)
        {
          IpcConnection *conn = udrServer_->getAnIpcConnection();
          stmtGlobals->setUdrConnection(conn);

          UdrDebug2("    TCB %p will use IPC Connection %p",
                    this, conn);
        }
        else
        {
          UdrDebug2("    TCB %p will use IPC Connection %p",
                     this, udrServer_->getUdrControlConnection());
        }
      }
      
      NAString cachedLibName, cachedLibPath;
      if ((myTdb().getLibraryRedefTime() != -1) && (myTdb().getLibraryRedefTime() != 0))
        {
          // Cache library locally. 
          NAString dummyUser;
          NAString libOrJarName;
        
          if (myTdb().getLanguage() == COM_LANGUAGE_JAVA)
            libOrJarName = myTdb().getPathName();
          else
            libOrJarName = myTdb().getContainerName();
          Int32 err = 0;
          if(err=ComGenerateUdrCachedLibName(libOrJarName.data(),
                                         myTdb().getLibraryRedefTime(),
                                         myTdb().getLibrarySchName(),
                                         dummyUser,
                                         cachedLibName, cachedLibPath))
            {
             
               char errString[200];
               NAString errNAString;
               sprintf(errString , "Error %d creating directory :",err ); 
               errNAString = errString ;   
               errNAString += cachedLibPath;         
               *getOrCreateStmtDiags() <<  DgSqlCode(-4316)
                                       << DgString0(( char *)errNAString.data());
               return FIXUP_ERROR;
            }
           NAString cachedFullName = cachedLibPath+"/"+cachedLibName;
          //If the local copy already exists, don't bother extracting.
          struct stat statbuf;
          if (stat(cachedFullName, &statbuf) != 0)
            {
              ComDiagsArea *returnedDiags = ComDiagsArea::allocate(getHeap());
              if (ExExeUtilLobExtractLibrary(&cliInterface,
                                             (char *)myTdb().getLibraryBlobHandle(), 
                                             ( char *)cachedFullName.data(),returnedDiags))
                {
                  *returnedDiags <<  DgSqlCode(-4316)
                                << DgString0(( char *)cachedFullName.data());
                  getOrCreateStmtDiags()->mergeAfter(*returnedDiags);
                  returnedDiags->decrRefCount();
                  returnedDiags = NULL;
                  return FIXUP_ERROR;
                }
        
              returnedDiags->decrRefCount();
              returnedDiags = NULL;
            }
          
        }
      
      if (sendControlMessage(isResultSet ? UDR_MSG_RS_LOAD : UDR_MSG_LOAD, 
                             TRUE, 
                             cachedLibName.length()? (char *)cachedLibName.data():NULL, 
                             cachedLibPath.length()? (char *)cachedLibPath.data():NULL))
      {
        if (verifyUdrServerProcessId())
        {
          //
          // Everything went as planned. We have a running server, the
          // LOAD message was sent and following the send, the server
          // process ID is still valid which should indicate that
          // during the send the IPC layer did not detect an IPC
          // error. The send() methods in our IPC stream and
          // connection classes return void, therefore errors can
          // occur during a send() without the caller of send() being
          // notified, and that is why we do the
          // verifyUdrServerProcessId() test following the send.
          //
          result = FIXUP_SUCCESS;
        }
        else
        {
          UdrDebug0("  ***");
          UdrDebug0("  *** WARNING: LOAD message successfully sent but");
          UdrDebug0("  ***          an IPC error occurred or perhaps the");
          UdrDebug0("  ***          server is no longer running.");
          UdrDebug0("  ***");
        }
      
      } // if (sendControlMessage())
    } // if (status == ExUdrServer::EX_UDR_SUCCESS)
  } // for i = 0 to numRetries
  
  if (result == FIXUP_SUCCESS)
  {
    allocateDataStream();
    result = super::fixup();
  }

  UdrDebug3("[END TCB FIXUP] %p. Return value is %d (%s)\n",
            this, result, (result == FIXUP_SUCCESS ? "SUCCESS" : "ERROR"));
  return result;

} // ExUdrTcb::fixup()

//----------------------------------------------------------------------
// TCB work methods
//----------------------------------------------------------------------
ExWorkProcRetcode ExUdrTcb::work()
{
#ifdef UDR_DEBUG
  static Lng32 workCount = 0;
  if (doTrace_)
  {
    workCount++;
    FILE *f = traceFile_;
    char text[256];
    TransIdToText(myExeStmtGlobals()->getTransid(), text, 256);
    ex_queue *up = qParent_.up;
    ex_queue *dn = qParent_.down;
    UdrPrintf(f, "[BEGIN WORK %d] %p", workCount, this);
    UdrPrintf(f, "  [WORK] Stmt tx %s", text);
    UdrPrintf(f, "  [WORK] TCB state %s", getUdrTcbStateString(state_));
    UdrPrintf(f, "  [WORK] Down queue: head %d, tail %d, len %d, size %d",
              (Lng32) dn->getHeadIndex(), (Lng32) dn->getTailIndex(),
              (Lng32) dn->getLength(), (Lng32) dn->getSize());
    UdrPrintf(f, "  [WORK] Up queue: head %d, tail %d, len %d, size %d",
              (Lng32) up->getHeadIndex(), (Lng32) up->getTailIndex(),
              (Lng32) up->getLength(), (Lng32) up->getSize());
    UdrPrintf(f, "  [WORK] Next to send: %d", (Lng32) nextToSend_);
    printDataStreamState();
  }
#endif // UDR_DEBUG

  ExWorkProcRetcode result = WORK_OK;

  result = checkReceive();
  UdrDebug1("  [WORK] checkReceive() returned %s",
            GetWorkRetcodeString(result));
  
  if (result == WORK_OK)
  {
    result = checkSend();
    UdrDebug1("  [WORK] checkSend() returned %s",
              GetWorkRetcodeString(result));
  }

  if (result == WORK_OK)
  {
    result = continueRequest();
    UdrDebug1("  [WORK] continueRequest() returned %s",
              GetWorkRetcodeString(result));
  }

  if (qParent_.down->isEmpty())
  {
    UdrDebug0("  [WORK] Down queue is empty");

    // There are no more parent requests to process. We now want to
    // perform garbage collection of buffers in the data stream. 
    // There seems to be a bug in stream message processing code 
    // that moves the message from RECEIVE queue to IN USE queue only 
    // when you try to get the next message from the stream and it 
    // only garbage collects the IN USE queue. 
    // So we ask the stream for one more buffer even though we know
    // there won't be one. By doing this we guarantee that all
    // incoming buffers have moved off the stream's RECEIVE queue and
    // on to the IN USE queue. 
    // If there does happen to be another buffer in the stream, 
    // that is an internal error.
    //
    // This cleanup technique is borrowed from the send top node (code
    // is in ex_send_top.cpp) and fixes the memory leak reported in
    // solution 10-050707-9503.
  
    UdrDebug0("  [WORK] Releasing unused IPC buffers in the data stream...");
    IpcMessageObjType msgType;
  
    if (dataStream_ && dataStream_->getNextReceiveMsg(msgType))
    {
      char buf[256];
      dataStream_->getNextObjType(msgType);
      str_sprintf(buf, "An unexpected message of type %d arrived",
                  (Int32) msgType);
      ex_assert(FALSE, buf);
    }
    dataStream_->releaseBuffers();
    UdrDebug0("  [WORK] Done releasing IPC buffers");
  
  } // if (qParent_.down->isEmpty())


#ifdef UDR_DEBUG
  if (doTrace_)
  {
    FILE *f = traceFile_;
    UdrPrintf(f, "  [WORK] UDR TX messages outstanding: %d",
              (Lng32) myExeStmtGlobals()->numUdrTxMsgsOut());
    UdrPrintf(f, "  [WORK] UDR Non-TX messages outstanding: %d",
              (Lng32) myExeStmtGlobals()->numUdrNonTxMsgsOut());
    UdrPrintf(f, "  [WORK] TCB state: %s", getUdrTcbStateString(state_));
    UdrPrintf(f, "[END WORK %d] %p. Return value is %s\n",
              workCount, this, GetWorkRetcodeString(result));
  }
#endif // UDR_DEBUG

  return result;
}

NABoolean ExUdrTcb::anyOutstandingQueueRequests()
{
  // Return TRUE if any down queue entries exist that have been sent
  // to the UDR server but for which not all up queue entries have
  // been generated.
  return (nextToSend_ != qParent_.down->getHeadIndex());
}

ExWorkProcRetcode ExUdrTcb::checkSend()
{
  ExWorkProcRetcode result = WORK_OK;

  if (getIpcBroken())
  {
    UdrDebug0("  [WORK] IPC is broken");
    return WORK_BAD_ERROR;
  }

  // We should not attempt to send data unless we are sure the server
  // has already loaded this UDR.
  if (!serverResourcesAreLoaded())
  {
    if (state_ == LOAD_FAILED)
    {
      UdrDebug0("  [WORK] Cannot continue. The LOAD request failed.");
      result = WORK_BAD_ERROR;
    }
    else
    {
      UdrDebug0("  [WORK] Cannot continue. Server resources not loaded.");
      result = WORK_OK;
    }
    return result;
  }

  if (!(qParent_.down->isEmpty()))
  {
    result = buildAndSendRequestBuffer();
  }

  return result;

} // ExUdrTcb::checkSend()

ExWorkProcRetcode ExUdrTcb::checkReceive()
{
  // Work method helper function that attempts to process output rows
  ExWorkProcRetcode result = WORK_OK;
  NABoolean done = FALSE;
  
  while (!done && result == WORK_OK
         && anyOutstandingQueueRequests() && !qParent_.up->isFull())
  {
    if (getIpcBroken())
    {
      UdrDebug0("  [WORK] IPC is broken");
      result = WORK_BAD_ERROR;
    }
    else
    {
      ex_queue_entry *downEntry = qParent_.down->getHeadEntry();
      ExUdrPrivateState &down_pstate = 
        *((ExUdrPrivateState *) downEntry->pstate);
      
      switch (down_pstate.step_)
      {
        case CANCEL_BEFORE_SEND:
          insertUpQueueEntry(ex_queue::Q_NO_DATA);
          break;
          
        case PRODUCE_ERROR_REPLY:
          // An error occurred before the entry could be sent, probably
          // during expression evaluation. If any diags were generated
          // they were placed in the down entry ATP and will be copied
          // to the up entry ATP by the following function.
          insertUpQueueEntry(ex_queue::Q_SQLERROR);
          down_pstate.step_ = PRODUCE_EOD_AFTER_ERROR;
          break;

      case PRODUCE_EOD_AFTER_ERROR:
          // finish up error handling by sending an EOD
          insertUpQueueEntry(ex_queue::Q_NO_DATA);
          break;
          
        case STARTED:
        case CANCEL_AFTER_SEND:
        {
          if (getReplyBuffer() != NULL)
          {
            // Now we can return a single up queue entry. If the up
            // queue entry is Q_NO_DATA then inside this call to
            // returnSingleRow() the head of the down queue will be
            // popped.
            result = returnSingleRow();
            if (replyBufferIsEmpty())
            {
              releaseReplyBuffer();

              // It is possible that we are in the WORK_IO_ACTIVE
              // state and conditions to transition back to WORK have
              // now been met. For example we might have just finished
              // processing all down queue entries that have been sent
              // to the server. The following method checks the
              // conditions and transitions to WORK if
              // appropriate. See commentary in the method for more
              // detail.
              attemptTransitionToWorkState();
            }
          }
          else
          {
            // check for errors from getReplyBuffer()
            if (getStatementDiags() &&
                getStatementDiags()->mainSQLCODE() < 0)
              {
                insertUpQueueEntry(ex_queue::Q_SQLERROR, getStatementDiags());
                down_pstate.step_ = PRODUCE_EOD_AFTER_ERROR;
              }
            else
              done = TRUE;
          }
        }
        break;
        
        case NOT_STARTED:
        default:
          ex_assert(FALSE, "Invalid step value in TCB down queue");
          break;
          
      } // switch (down_pstate.step_)
      
    } // if (getIpcBroken()) else
  } // while (!done && result == WORK_OK && outstanding requests ... )
  
  return result;

} // ExUdrTcb::checkReceive()

//
// Insert a single entry into the up queue and optionally
// remove the head of the down queue
//
// Right now this function does not handle data rows, only error
// and end-of-data. It could possibly be extended to handle a data
// row. I have not looked at that closely enough yet.
//
NABoolean ExUdrTcb::insertUpQueueEntry(ex_queue::up_status status,
                                       ComDiagsArea *diags)
{
  if (qParent_.up->isFull())
    return FALSE;

  ex_queue_entry *upEntry = qParent_.up->getTailEntry();
  ex_queue_entry *downEntry = qParent_.down->getHeadEntry();
  ExUdrPrivateState &privateState =
    *((ExUdrPrivateState *) downEntry->pstate);

  // Initialize the up queue entry. 
  //
  // copyAtp() will copy all tuple pointers and the diags area from
  // the down queue entry to the up queue entry.
  //
  // When we return Q_NO_DATA if the match count is > 0:
  // * assume down queue diags were returned with the Q_OK_MMORE entries
  // * release down queue diags before copyAtp()
  //
  if (status == ex_queue::Q_NO_DATA && privateState.matchCount_ > 0)
  {
    downEntry->setDiagsArea(NULL);
    upEntry->copyAtp(downEntry);
  }
  else
  {
    upEntry->copyAtp(downEntry);
    downEntry->setDiagsArea(NULL);
  }

  upEntry->upState.status = status;
  upEntry->upState.parentIndex = downEntry->downState.parentIndex;
  upEntry->upState.downIndex = qParent_.down->getHeadIndex();
  upEntry->upState.setMatchNo(privateState.matchCount_);
  
  // Move any diags to the up queue entry
  if (diags != NULL)
  {
    ComDiagsArea *atpDiags = upEntry->getDiagsArea();
    if (atpDiags == NULL)
    {
      // setDiagsArea() does not increment the reference count
      upEntry->setDiagsArea(diags);
      diags->incrRefCount();
    }
    else if (atpDiags != diags)
    {
      atpDiags->mergeAfter(*diags);
      // errors have been reported, clear the passed-in diags
      diags->clear();
    }
  }

  if (status == ex_queue::Q_SQLERROR &&
      myTdb().isTmudf())
    {
      // cancel child requests, if necessary
      tmudfCancelChildRequests(qParent_.down->getHeadIndex());
    }
  
  // Insert into up queue
  qParent_.up->insert();
 
  // Remove the head of the down queue if we are done
  if (status == ex_queue::Q_NO_DATA)
  {
    privateState.init();
    qParent_.down->removeHead();
  }

  return TRUE;
}

ExWorkProcRetcode ExUdrTcb::buildAndSendRequestBuffer()
{
  UdrDebug1("  [WORK] BEGIN ExUdrTcb::buildAndSendRequestBuffer() 0x%08x",
            this);

  ExWorkProcRetcode result = WORK_OK;
  ULng32 numErrors = 0;
  ULng32 numCancels = 0;
  NABoolean isResultSet = myTdb().isResultSetProxy();

  // Determine the transid we should send in data messages. We
  // use the statement transid if all the following are true
  // a. the TDB says we require a transaction
  // b. this is NOT a CALL that returns result sets
  // c. this is NOT a result set
  // 
  // Note: (b) and (c) are both indicated by rsInfo_ == NULL
  // 
  Int64 &stmtTransId = myExeStmtGlobals()->getTransid();
  Int64 transIdToSend = -1; // -1 is an invalid transid
  if (dataRequestsAreTransactional())
    transIdToSend = stmtTransId;
  
  // Process entries from the down queue. nextToSend_
  // will be incremented inside the loop following successful 
  // insertion of a request row into the request buffer.
  // state_ will be WORK until a request message is sent and
  // then state_ will become WORK_IO_ACTIVE.
  while (result == WORK_OK && state_ == WORK
         && qParent_.down->entryExists(nextToSend_))
  {
    // Two early tests to see if we should get out of the outer while
    // loop. First make sure a server is running then attempt to
    // allocate a request buffer.
    //
    // 1. Make sure we have a data stream and a running server
    if (dataStream_ == NULL || !verifyUdrServerProcessId())
    {
      result = WORK_BAD_ERROR;
      break;
    }

    // 2. Allocate a request buffer on the message stream heap. Make
    // sure stream buffer limits have not been reached. Break out of
    // the outer while loop if the allocation fails.
    if (requestBuffer_ == NULL)
    {
      // Free up any receive buffers no longer in use
      dataStream_->cleanupBuffers();

      // Now test the stream buffer limits. If the send queue is full
      // we return WORK_OK and let the operator wake up again when I/O
      // completes. If other limits have been reached we return
      // WORK_POOL_BLOCKED which causes the operator to be scheduled
      // again right away.
      if (dataStream_->sendLimitReached())
      {
        UdrDebug0("  [WORK] Cannot allocate request buffer");
        UdrDebug0("         sendLimitReached() returned TRUE");
        printDataStreamState();
        result = WORK_OK;
      }
      else if (dataStream_->inUseLimitReached())
      {
        UdrDebug0("  [WORK] Cannot allocate request buffer");
        UdrDebug0("         inUseLimitReached() returned TRUE");
        printDataStreamState();
        result = WORK_POOL_BLOCKED;
      }
      else
      {
        requestBuffer_ = getRequestBuffer();
        if (requestBuffer_ == NULL)
        {
          result = WORK_POOL_BLOCKED;
          printDataStreamState();
        }
      }
    } // if (requestBuffer_ == NULL)

    if (requestBuffer_ == NULL)
      break;

    SqlBuffer *sqlBuf = requestBuffer_->getSqlBuffer();
    ex_assert(sqlBuf, "UDR request buffer is corrupt or contains no data");

    ULng32 numRequestsAdded = 0;

    // Move as many rows as possible into the request buffer
    // 
    // *** IMPORTANT: This inner loop must not do any I/O, must not
    // cause a call to setUdrTcbState(), and must not cause the value
    // of dataStream_ to become non-null. If this is done then the
    // conditions guaranteed by the outer while loop will still be
    // valid following this inner while loop.
    NABoolean sendSpaceAvailable = TRUE;
    while (sendSpaceAvailable && qParent_.down->entryExists(nextToSend_))
    {
      ex_queue_entry *downEntry = qParent_.down->getQueueEntry(nextToSend_);
      const ex_queue::down_request request = downEntry->downState.request;
      const Lng32 value = downEntry->downState.requestValue;
      ExUdrPrivateState &pstate = *((ExUdrPrivateState *) downEntry->pstate);
      NABoolean anyErrors = FALSE;
      
      switch (request)
      {
      case ex_queue::GET_N:
      case ex_queue::GET_ALL:
        {
          // First step is to allocate space in the SqlBuffer. After
          // that a move expression will be evaluated to copy data into
          // the buffer. If the expression results in errors then we need
          // to back-out the buffer allocation, which may have been a
          // single data tupp or possibly control + data. So that the
          // correct number are backed out, we will remember how many 
          // tupps are in the buffer before the allocation takes place.
          Lng32 numTuppsBefore = sqlBuf->getTotalTuppDescs();

          tupp_descriptor *dataDesc =NULL;
          if (sqlBuf->moveInSendOrReplyData(
            TRUE,                        // [IN] sending? (vs. replying)
            FALSE,                       // [IN] force move of control info?
            TRUE,                        // [IN] move data?
            &(downEntry->downState),     // [IN] queue state
            sizeof(ControlInfo),         // [IN] length of ControlInfo area
            NULL,                        // [OUT] new ControlInfo area
            myTdb().getRequestRowLen(),  // [IN] data row length
            &dataDesc,                   // [OUT] new data tupp_descriptor
            NULL,                        // [IN] diags area
            NULL                         // [OUT] new diags tupp_descriptor
            ) == SqlBuffer::BUFFER_FULL)
          {
            // No more space in request buffer. The assertion here says that
            // we have at least inserted one row into the message buffer.
            ex_assert(numRequestsAdded > 0,
              "Request buffer not large enough to hold a single row");
            sendSpaceAvailable = FALSE;
          }
          else
          {
            ex_expr::exp_return_type expStatus = ex_expr::EXPR_OK;
            if (myTdb().getInputExpression())
            {
              workAtp_->getTupp(myTdb().getRequestTuppIndex()) = dataDesc;

              // Evaluate the input expression. If diags are generated they
              // will be left in the down entry ATP.
              expStatus = 
                myTdb().getInputExpression()->eval(downEntry->getAtp(),
                                                   workAtp_);
#ifdef UDR_DEBUG
              if (expStatus != ex_expr::EXPR_OK)
                UdrDebug1("  [WORK] inputExpr.eval() returned %s",
                          GetExpStatusString(expStatus));
#endif
              workAtp_->getTupp(myTdb().getRequestTuppIndex()).release();
              
              if (expStatus == ex_expr::EXPR_ERROR)
              {
                while (sqlBuf->getTotalTuppDescs() > numTuppsBefore)
                {
                  sqlBuf->remove_tuple_desc();
                }
                pstate.step_ = ExUdrTcb::PRODUCE_ERROR_REPLY;
                numErrors++;
              }

            } // if (myTdb().getInputExpression())

            if (expStatus != ex_expr::EXPR_ERROR)
            {
              pstate.step_ = ExUdrTcb::STARTED;
              numRequestsAdded++;
            }
            
            // Move to the next down entry
            nextToSend_++;

          } // if (no space in the request buffer) else
          
        } // case GET_N, GET_ALL
        break;
        
      case ex_queue::GET_NOMORE:
        {
          pstate.step_ = ExUdrTcb::CANCEL_BEFORE_SEND;
          numCancels++;
          nextToSend_++;
        }
        break;
        
      case ex_queue::GET_EMPTY:
      case ex_queue::GET_EOD:
      default:
        {
          ex_assert(FALSE, "Invalid request type in TCB down queue");
          nextToSend_++;
        }
        break;
        
      } // switch (request)
    } // while (sendSpaceAvailable && qParent_.down->entryExists(nextToSend_))

    UdrDebug2("  [WORK] %u request%s added to the request buffer",
      numRequestsAdded, PLURAL_SUFFIX(numRequestsAdded));
    UdrDebug2("  [WORK] %u error%s encountered",
      numErrors, PLURAL_SUFFIX(numErrors));
    UdrDebug2("  [WORK] %u %s cancelled before being sent",
      numCancels, (numCancels == 1 ? "entry" : "entries"));

    if (sqlBuf->getTotalTuppDescs() > 0)
    {
      // If we enter this block then it's time to send a data request

#ifdef UDR_DEBUG
      // Print a message if tracing is enabled or the
      // UDR_CONTINUE_DEBUG environment variable is set
      if (doTrace_ ||
          doIpcTrace_ ||
          (getenv("UDR_CONTINUE_DEBUG") != NULL))
      {
        char stmtTx[256];
        char msgTx[256];
        TransIdToText(stmtTransId, stmtTx, 256);
        TransIdToText(transIdToSend, msgTx, 256);
        UdrPrintf(traceFile_ ? traceFile_ : stdout,
                  "  [WORK] Sending %s, %d tupps, msg tx %s",
                  isResultSet ? "RS INVOKE" : "INVOKE",
                  (Lng32) sqlBuf->getTotalTuppDescs(), msgTx);
      }
#endif
      
      sqlBuf->drivePack();
      setUdrTcbState(WORK_IO_ACTIVE);

      if (udrStats_)
      {
        udrStats_->bufferStats()->sentBuffers().addEntry(sizeof(*requestBuffer_) +
                                                sqlBuf->get_used_size());
        udrStats_->bufferStats()->totalSentBytes() += sqlBuf->getSendBufferSize();
      }

      // If this is a CALL that produces result sets or a result set
      // (both are indicated by a non-NULL rsInfo_ pointer) then we
      // may need to bring the UDR server into the current transaction
      // before sending the data request.
      if (rsInfo_)
        rsInfo_->enterUdrTx(*myExeStmtGlobals());

      if (dataRequestsAreTransactional())
        myExeStmtGlobals()->incrementUdrTxMsgsOut();
      else
        myExeStmtGlobals()->incrementUdrNonTxMsgsOut();

      dataStream_->sendRequest(transIdToSend);
      dataMsgsSent_++;

      UdrDebug0("  [WORK] The data stream has returned control to the TCB");

      // Now that sendRequest() has completed the request buffer is in
      // packed form and owned by the stream. This TCB should not
      // access the request buffer again so we release it now.
      releaseRequestBuffer();

      UdrDebug0("  [WORK] Done sending the request buffer");

    } // if (sqlBuf->getTotalTuppDescs() > 0)

  } // while (result == WORK_OK && state_ == WORK &&
    //        qParent_.down->entryExists(nextToSend_))

  if (result == WORK_OK)
  {
    if (getIpcBroken())
    {
      result = WORK_BAD_ERROR;
    }
  }

  //
  // If errors were encountered then there may be down queue
  // entries that were not sent but still need to be processed.
  // Changing the return code to WORK_CALL_AGAIN makes sure the
  // scheduler redrives this TCB.
  //
  if (result == WORK_OK && (numErrors > 0 || numCancels > 0))
  {
    result = WORK_CALL_AGAIN;
  }

  UdrDebug1("  [WORK] END ExUdrTcb::buildAndSendRequestBuffer() %p",
            this);
  return result;

} // ExUdrTcb::buildAndSendRequestBuffer()





//
// Stream callbacks use this function to report arrival of a reply to
// a LOAD message
//
void ExUdrTcb::reportLoadReply(NABoolean loadWasSuccessful)
{
  UdrDebug2("  TCB %p received a LOAD reply. Handle " INT64_PRINTF_SPEC,
            this, udrHandle_);
  UdrDebug1("    The load %s successful",
            loadWasSuccessful ? "was" : "was not");

  ex_assert(state_ == SENDING_LOAD, "Unexpected arrival of a LOAD reply");
  
  releaseControlStream();

  if (loadWasSuccessful)
  {
    setUdrTcbState(WORK);
  }
  else
  {
    // If we know now that the JVM did not start then we stop the
    // running server now. JVM failures are indicated by error -11202
    // (LME_JVM_INIT_ERROR in langman/LmError.h).
    ComDiagsArea *d = getStatementDiags();

    if (d && d->contains(-11202))
    {
      udrServer_->setState(ExUdrServer::EX_UDR_BROKEN);
      setUdrTcbState(IPC_ERROR);
    }
    else
    {
      setUdrTcbState(LOAD_FAILED);
    }
  }

  tickleSchedulerWork();
}

//
// ExUdrTcb::reportIpcError()
//
// Stream callbacks use this function to report IPC errors.  When conn
// is not NULL then an IPC failure occurred on the connection. If conn
// is NULL then some other type of error occurred that prevents the
// TCB from continuing its work. For example, if a LOAD reply reports
// that the LOAD was not successful then the TCB cannot continue. The
// stream callback will first move diags from the stream into the TCB
// statement globals area and then call this function with conn set to
// NULL.
//
// The technique of putting diags directly into the statement globals
// area may not be a good long-term solution but works fine for
// single-row CALL statements. It may be better to associate diags
// with an up queue entry and avoid returning WORK_BAD_ERROR to the
// scheduler. However, one effect of returning WORK_BAD_ERROR is that
// the statement does not get used again until it is deallocated and
// re-fixed up. This is a desirable effect because it guarantees that
// a new server gets started. All these pros and cons will have to be
// considered if we want to change the error handling strategy for
// this TCB.
// 
void ExUdrTcb::reportIpcError(IpcMessageStreamBase *stream,
                              IpcConnection *conn)
{
  UdrDebug1("  [BEGIN ExUdrTcb::reportIpcError(IpcConnection %p)]",
            conn);

  //
  // Other than lots of debug-build printf calls this method does 
  // the following:
  // - If the TCB is not already in the IPC_ERROR state:
  //     - Create diagnostics
  //     - Kill the server if we received an interrupt signal
  //     - Transition to the IPC_ERROR state
  // - Release all message streams
  // - Schedule the TCB
  //

  if (!getIpcBroken())
  {
    ComDiagsArea *diags = getOrCreateStmtDiags();
    CollHeap *diagsHeap = getHeap();

    NABoolean udrErrorAlreadyInDiags = FALSE;
    if (diags->contains(-EXE_UDR_REPLY_ERROR))
      udrErrorAlreadyInDiags = TRUE;

    if (conn)
    {
#ifdef UDR_DEBUG
      UdrDebug1("    IPC error on connection %p", conn);
      if (conn->castToGuaConnectionToServer())
      {
        GuaErrorNumber guardianError =
          conn->castToGuaConnectionToServer()->getGuardianError();
        UdrDebug1("    Guardian error number is %d", (Lng32) guardianError);
      }
#endif // UDR_DEBUG

      // Merge the connection's diagnostics with the main diags area
      // for this TCB
      if (!udrErrorAlreadyInDiags)
        conn->populateDiagsArea(diags, diagsHeap);

    } // if (conn)

    // Even if a connection was passed in and it has diagnostics, they
    // are generic and do not say anything UDR-specific. Here we add a
    // simple diagnostic that says the error is an MXUDR failure.
    if (!udrErrorAlreadyInDiags)
      *diags << DgSqlCode(-EXE_UDR_REPLY_ERROR);


    // Notify ExUdrServer that we have seen an error if the stored
    // serverProcessId_ matches that of ExUdrServer's. They will be
    // different if ExUdrServer has already started new process after
    // an error.
    if (udrServer_->getServerProcessId() == serverProcessId_)
      udrServer_->setState(ExUdrServer::EX_UDR_BROKEN);

    setUdrTcbState(IPC_ERROR);

  } // if (!getIpcBroken())

  //
  // We can release an outstanding control stream because doing so 
  // does not actually delete the object. Instead it goes on the
  // IpcEnvironment's completed list.
  //
  releaseControlStream();

  tickleSchedulerWork();

  UdrDebug0("  [END ExUdrTcb::reportIpcError()]");

} // ExUdrTcb::reportIpcError()

void ExUdrTcb::deallocateMessage(UdrMessageObj *m)
{
  UdrDebug2("  ExUdrTcb %p deallocating message object %p",
               this, m);
  if (m)
  {
    //
    // UDR messages will be deallocated from a heap when the reference
    // count drops to zero, not by calling a destructor.
    //
    m->decrRefCount();
  }
}

void ExUdrTcb::releaseControlStream()
{
  if (outstandingControlStream_)
  {
    outstandingControlStream_->delinkTcb(this);
    outstandingControlStream_ = NULL;
  }
}

void ExUdrTcb::releaseDataStream()
{
  /* The following code is added for memory leak for RS Continue Reply
     message. The dataStream_->getNextReceiveMsg() function will put
     the RS Continue Reply message in inUseBufList_. So when the
     destructor for datastream_ is called, the cleanupBuffers() will
     delete this message from the inUseBufList_.

     But if the ExUdrTcb is in IPC_ERROR state_, we are not executing
     this while loop. It is possible that there might be a small 
     memory leak for error messages. If we get in the while loop for 
     IPC_ERROR state, we are hanging in the while loop for ever. 
     So we are ignoring that one right now.
  */

  // $$$$ THE FOLLOWING LOOP IS COMMENTED OUT BECAUSE IT CAUSES A HANG
  // FOR UDFS. THIS SHOULD BE CORRECTED BEFORE WE RELEASE UDFS. THE
  // HANG HAPPENED WHEN THERE WERE TWO UDR OPERATORS IN THE SAME
  // FRAGMENT AND FOR ONE OPERATOR THE MXUDR LOAD FAILED. TCB
  // DESTRUCTORS WERE CALLED BEFORE THE "GOOD" UDR OPERATOR PROCESSED
  // ITS REPLY DATA. INTERNALLY IN THE "GOOD" DATA STREAM, A REPLY
  // BUFFER HAD NOT MOVED FROM THE RECV QUEUE TO THE IN INUSE QUEUE
  // AND THIS WHILE LOOP KEPT EXECUTING WHILE THE BUFFER STAYED STUCK
  // ON THE RECV QUEUE.
#if 0
  if (state_ != IPC_ERROR)
  {
    IpcMessageObjType msgType;
    while (dataStream_ && dataStream_->getNextReceiveMsg(msgType))
    {
      ;
    }
  }
#endif

  releaseRequestBuffer();
  releaseReplyBuffer();

  if (dataStream_)
  {
    UdrDebug1("ExUdrTcb %p releasing its data stream", this);
    dataStream_->delinkTcb(this);
    dataStream_ = NULL;
  }
}

void ExUdrTcb::releaseConnectionToServer()
{
  ExExeStmtGlobals *stmtGlobals = myExeStmtGlobals();

  IpcConnection *conn = stmtGlobals->getUdrConnection();

  if(myTdb().isTmudf())
  {
    conn = udrServer_->getUdrControlConnection();
  }

  // RSs share connection with parent CALL statement. So no need to
  // release connection for RSs. Connection will be released in
  // parent CALL's context.
  if (conn && (myTdb().getMaxResultSets() > 0))
    udrServer_->releaseConnection(conn);
}

//
// Function called during fixup to create the data stream.
// Should not be called except during fixup, and only after
// a UDR server is believed to be running.
//
void ExUdrTcb::allocateDataStream()
{
  UdrDebug1("  [BEGIN ExUdrTcb::allocateDataStream()] %p", this);

  ExExeStmtGlobals *stmtGlobals = myExeStmtGlobals();
  IpcConnection *conn = stmtGlobals->getUdrConnection();
  if(myTdb().isTmudf())
  {
    conn = udrServer_->getUdrControlConnection();
  }

  if (!dataStream_ && conn)
  {
    
    IpcMessageObjSize maxSqlBufSize = 0;
    IpcMessageObjSize maxBufSize = 0;
    
    maxSqlBufSize = MAXOF(myTdb().getRequestSqlBufferSize(),
                          myTdb().getReplySqlBufferSize());

    NABoolean isResultSet = myTdb().isResultSetProxy();

    if (isResultSet)
      maxBufSize += sizeof(UdrRSDataHeader);
    else if (myTdb().isTmudf())
      maxBufSize +=sizeof(UdrTmudfDataHeader);
    else
      maxBufSize += sizeof(UdrDataHeader);

    IpcMessageBuffer::alignOffset(maxBufSize);
    
    maxBufSize += sizeof(UdrDataBuffer);
    IpcMessageBuffer::alignOffset(maxBufSize);
    
    maxBufSize += maxSqlBufSize;
    IpcMessageBuffer::alignOffset(maxBufSize);
    
    // maxBufSize now represents the number of bytes in a request or
    // in a reply, whichever is larger. We want the stream to use this
    // size as its default buffer size. But when diags are returned in
    // a reply, the reply can become bigger. We will create the
    // stream's internal buffers with room for the request or reply,
    // and a pad for diags. Doing this does not increase the actual
    // message size. It just allocates additional memory for the
    // target buffers used by the stream for I/O. These buffers are
    // where requests are buffered and where replies are written. By
    // making them large enough we avoid going into chunking mode when
    // diags are returned in a reply.
    IpcMessageObjSize pad = 1000;

#ifdef UDR_DEBUG
    //
    // We can set the pad size with an environment variable in the
    // debug build.
    //
    char *val;
    if ((val = getenv("UDR_BUFFER_PAD")))
    {
      pad = atol(val);
    }
    UdrDebug3("    Stream buffer size: %d data + %d pad = %d total",
                 (Lng32) maxBufSize, (Lng32) pad, (Lng32) maxBufSize + pad);
#endif // UDR_DEBUG
    
    NABoolean isTransactional = dataRequestsAreTransactional();

    dataStream_ = new (getIpcHeap()) UdrClientDataStream (
      myIpcEnv(),
      1,                 // sendBufferLimit
      2,                 // inUseBufferLimit
      maxBufSize + pad,  // default buffer size
      this,
      myExeStmtGlobals(),
      isTransactional);

#ifdef UDR_DEBUG
    if (doIpcTrace_ && traceFile_)
      dataStream_->setTraceFile(traceFile_);
#endif

    dataStream_->addRecipient(conn);
    
#ifdef UDR_DEBUG
    char buf[300];
    serverProcessId_.toAscii(buf, 300);
    UdrDebug2("    Attaching buffered data stream %p to connection %d",
              dataStream_, (Lng32) conn->getId());
    UdrDebug1("    Server process is %s", buf);
    UdrDebug1("  [END ExUdrTcb::allocateDataStream()] %p", this);
#endif // UDR_DEBUG
  }
}

void ExUdrTcb::releaseServerResources()
{
  if (state_ == SENDING_LOAD || state_ == LOAD_FAILED
      || serverResourcesAreLoaded())
  {
    UdrDebug1("  ExUdrTcb %p is releasing server resources", this);

    NABoolean isResultSet = myTdb().isResultSetProxy();

    if (isResultSet)
    {
      UdrDebug0("    An RS UNLOAD message is about to be sent");
      sendControlMessage(UDR_MSG_RS_UNLOAD, FALSE);
    }
    else
    {
      UdrDebug0("    An UNLOAD message is about to be sent");
      sendControlMessage(UDR_MSG_UNLOAD, FALSE);
    }
  }
}

// This function is used to send any control messages. Right now there are
// the following types:
// - LOAD is sent during fixup for UDR invocations
// - RS LOAD is sent during fixup for a stored procedure result set proxy
//   statement
// - UNLOAD or RS UNLOAD, whichever is appropriate, is sent during teardown
//   by freeResources(). 
// - RS CLOSE is sent by the RS operator when down queue entries are
//   cancelled.
NABoolean ExUdrTcb::sendControlMessage(UdrIpcObjectType t,
                                       NABoolean callbackRequired, char *cachedLibName, char *cachedLibPath)
{
  // Determine the transid we should send in the message. We
  // use the statement transid if all the following are true
  // a. the TDB says we require a transaction
  // b. the message type is UDR_MSG_LOAD or UDR_MSG_UNLOAD
  //
  // Note: 
  // * other control messages such as RS LOAD and RS UNLOAD do not
  //   require a transaction since they travel within the scope of
  //   an ENTER/EXIT TX pair.
  // * when the statement is being deallocated after the transaction
  //   ended, UNLOAD will be sent without a transid since there is no
  //   longer a transid in stmt globals.
  Int64 &stmtTransId = myExeStmtGlobals()->getTransid();
  Int64 transIdToSend = -1; // -1 is an invalid transid
  if (myTdb().getTransactionAttrs() == COM_TRANSACTION_REQUIRED &&
      (t == UDR_MSG_LOAD || t == UDR_MSG_UNLOAD))
  {
    transIdToSend = stmtTransId;
  }
 
#ifdef UDR_DEBUG
  {
    char stmtTx[256];
    char msgTx[256];
    TransIdToText(stmtTransId, stmtTx, 256);
    TransIdToText(transIdToSend, msgTx, 256);
    UdrDebug1("  [BEGIN ExUdrTcb::sendControlMessage()] 0x%08x", this);
    UdrDebug2("    stmt tx %s, msg tx %s", stmtTx, msgTx);
  }
#endif
 
 
  ex_assert(outstandingControlStream_ == NULL,
    "Sending a UDR control message while one is already outstanding");

  UdrMessageObj *msg = NULL;
  UdrClientControlStream *s = NULL;
  NAMemory *h = getIpcHeap();
  const ExUdrTdb &udrTdb = myTdb();
  UdrTcbState nextState = state_;

  // This switch statement builds a control message object of
  // the correct type
  switch (t)
    {
    case UDR_MSG_LOAD:
    {
      releaseServerResources();
      setUdrTcbState(SENDING_LOAD);
      const char *parentQid;
      if (myExeStmtGlobals()->castToExMasterStmtGlobals())
        parentQid = myExeStmtGlobals()->castToExMasterStmtGlobals()->
          getStatement()->getUniqueStmtId();
      else if (myExeStmtGlobals()->castToExEspStmtGlobals())
        parentQid = myExeStmtGlobals()->castToExEspStmtGlobals()->
          getQueryId();
      else
        parentQid = "";

      //  set the instance number/numInstances
      Lng32 numInstances = myExeStmtGlobals()->getNumOfInstances();
      Lng32 instanceNum = myExeStmtGlobals()->getMyInstanceNumber();
      if (numInstances == 0)
        numInstances = 1; // I compute, therefore I am.

      
      // Steps to create a LOAD message:
      // - First create a UdrLoadMsg instance on the IPC heap and initialize
      //   it with a UDR handle and UDR metadata
      // - Next put metadata for each formal parameter into the object

      //In the case of java, the path name contains the fully qualified jar file location
      NAString pathName, containerName;
      if (udrTdb.getLanguage() == COM_LANGUAGE_JAVA)
        {
          pathName = cachedLibPath? cachedLibPath :udrTdb.getPathName();
          if (cachedLibName)
            {
            pathName+= "/";
            pathName+=cachedLibName;
            }
          containerName = udrTdb.getContainerName();
        }
      else
        //In the case of C/C++ the DLL name and the path names need to be sent
        //separately so langman can process them properly
        {
          pathName = cachedLibPath? cachedLibPath :udrTdb.getPathName();
          containerName = cachedLibName?cachedLibName: udrTdb.getContainerName();
        }
      UdrLoadMsg *loadMsg = new (h) UdrLoadMsg(
        h,
        udrTdb.getSqlName(),
        udrTdb.getRoutineName(),
        udrTdb.getSignature(),
        containerName,
        pathName,
        udrTdb.getLibrarySqlName(),
        udrTdb.getTransactionAttrs(),
        udrTdb.getSqlAccessMode(),
        udrTdb.getLanguage(),
        udrTdb.getParamStyle(),
        udrTdb.getExternalSecurity(),
        udrTdb.getMaxResultSets(),
        udrTdb.getNumParams(),
        udrTdb.getNumInputValues(),
        udrTdb.getNumOutputValues(),
        udrTdb.getRequestSqlBufferSize(),
        udrTdb.getReplySqlBufferSize(),
        udrTdb.getRequestRowLen(),
        udrTdb.getReplyRowLen(),
        udrTdb.getUdrFlags(),
        udrTdb.getRoutineOwnerId(),
        parentQid,
        udrTdb.udrSerInvocationInfoLen_,
        udrTdb.udrSerInvocationInfo_,
        udrTdb.udrSerPlanInfoLen_,
        udrTdb.udrSerPlanInfo_,
        udrTdb.getJavaDebugPort(),
        udrTdb.getJavaDebugTimeout(),
	instanceNum,
	numInstances
        );

      loadMsg->setHandle(udrHandle_);

      // This loop will insert parameter metadata into the LOAD message.
      // The loop iterates over a single list of parameters. But inside
      // the LOAD message two lists are maintained, one for input and
      // one for output. To keep track of the current position in the
      // input and output lists the inPosition and outPosition counters
      // are used.
      ComUInt32 i;
      ComUInt32 inPosition = 0;
      ComUInt32 outPosition = 0;
      for (i = 0; i < udrTdb.getNumParams(); i++)
      {
        const UdrFormalParamInfo &formal = *udrTdb.getFormalParamInfo(i);
        const char *paramName = formal.getParamName();
        ComUInt16 paramNameLen = (ComUInt16) str_len(paramName);
        ComSInt16 fsType = formal.getType();
        SQLTYPE_CODE ansiType =
          (SQLTYPE_CODE) Descriptor::ansiTypeFromFSType(fsType);

        if (formal.isIn())
        {
          // Cannot use a const reference to the Attributes objects because
          // Attributes accessors are not const methods
          Attributes &actual = *(udrTdb.getRequestAttr(inPosition));
          
          UdrParameterInfo info (
            i,                                // Formal: position
            formal.getFlags(),                //         flags
            fsType,                           //         FS type
            ansiType,                         //         ANSI type
            paramNameLen,                     //         length of param name
            paramName,                        //         param name
            formal.getPrecision(),            //         prec
            formal.getScale(),                //         scale
            formal.getEncodingCharSet(),      //         encoding charset
            formal.getCollation(),            //         collation
            actual.getLength(),               // Actual: dataLength
            actual.getNullIndicatorLength(),  //         nullIndicatorLength
            actual.getNullIndOffset(),        //         nullIndicatorOffset
            actual.getOffset(),               //         dataOffset
            actual.getVCLenIndOffset(),       //         varchar ind offset
            actual.getVCIndicatorLength()     //         varchar ind length
            );

          loadMsg->setInParam(inPosition, info);
          inPosition++;

        } // if (formal.isIn())
      
        if (formal.isOut())
        {
          Attributes &actual = *(udrTdb.getReplyAttr(outPosition));
          
          UdrParameterInfo info (
            i,                                // Formal: position
            formal.getFlags(),                //         flags
            fsType,                           //         FS type
            ansiType,                         //         ANSI type
            paramNameLen,                     //         length of param name
            paramName,                        //         param name
            formal.getPrecision(),            //         prec
            formal.getScale(),                //         scale
            formal.getEncodingCharSet(),      //         encoding charset
            formal.getCollation(),            //         collation
            actual.getLength(),               // Actual: dataLength
            actual.getNullIndicatorLength(),  //         nullIndicatorLength
            actual.getNullIndOffset(),        //         nullIndicatorOffset
            actual.getOffset(),               //         dataOffset
            actual.getVCLenIndOffset(),       //         varchar ind offset
            actual.getVCIndicatorLength()     //         varchar ind length
            );
          
          loadMsg->setOutParam(outPosition, info);
          outPosition++;

        } // if (formal.isOut())
      
      } // for (i = 0; i < udrTdb.getNumParams(); i++)

      // Add optional data buffers to the load message
      Queue *q = udrTdb.getOptionalData();
      if (q)
      {
        Lng32 numEntries = q->numEntries();
        loadMsg->initOptionalDataBufs(numEntries, TRUE);

        char *s = NULL;
        ComUInt32 j = 0;
        q->position();
        while ((s = (char *) q->getNext()) != NULL)
        {
          // Each data element is prefixed by a 4-byte length field
          UInt32 dataLen = 0;
          str_cpy_all((char *)&dataLen, s, 4);
          loadMsg->setOptionalDataBuf(j++, s, (dataLen + 4));

        } // for each queue element
      } // if (q)
      if(udrTdb.isTmudf())
      {
	setTmUdfInfo(loadMsg,udrTdb);
      }
	
      msg = loadMsg;

      UdrDebug2("    TCB %p created UdrLoadMsg %p", this, msg);
    
    } // case UDR_MSG_LOAD
    break;

    case UDR_MSG_RS_LOAD:
    {
      if (rsInfo_)
      {
         // Need to send UdrEnterTxMsg to udrserver if the
         // txState_ is not ACTIVE.
         // This change is added as a precautionary measure.
         // Normally this should not happen.

         rsInfo_->enterUdrTx(*myExeStmtGlobals());
      }
      releaseServerResources();
      setUdrTcbState(SENDING_LOAD);

      // Steps to create an RS LOAD message:
      // - First create a message object on the IPC heap and initialize
      //   it with a UDR handle and RS handle
      // - Next put metadata for each output column into the object

      UdrRSLoadMsg *rsLoadMsg = new (h)
        UdrRSLoadMsg(rsIndex_,
                     udrTdb.getNumOutputValues(),
                     udrTdb.getReplyRowLen(),
                     udrTdb.getReplySqlBufferSize(),
                     udrTdb.getUdrFlags(),
                     h);

      rsLoadMsg->setHandle(udrHandle_);
      rsLoadMsg->setRSHandle(rsHandle_);

      // This loop will insert output column descriptions into the RS
      // LOAD message. Each column is described by a
      // UdrFormalParamInfo instance in the TDB and a UdrParameterInfo
      // instance in the outgoing message.
      ComUInt32 i;
      for (i = 0; i < udrTdb.getNumParams(); i++)
      {
        const UdrFormalParamInfo &formal = *udrTdb.getFormalParamInfo(i);
        Attributes &actual = *(udrTdb.getReplyAttr(i));
        
        ComUInt16 fsType = formal.getType();
        ComUInt32 dataLength = actual.getLength();
        ComUInt32 dataOffset = actual.getOffset();
        ComUInt32 lenIndOffset = actual.getVCLenIndOffset();
        ComUInt16 lenIndLen = actual.getVCIndicatorLength();

        SQLTYPE_CODE ansiType =
          (SQLTYPE_CODE) Descriptor::ansiTypeFromFSType(fsType);

        UdrParameterInfo info (
            i,                                // Formal: position
            formal.getFlags(),                //         flags
            (ComSInt16) fsType,               //         FS type
            ansiType,                         //         ANSI type
            0,                                //         length of param name
            "",                               //         param name
            formal.getPrecision(),            //         prec
            formal.getScale(),                //         scale
            formal.getEncodingCharSet(),      //         encoding charset
            formal.getCollation(),            //         collation
            dataLength,                       // Actual: dataLength
            actual.getNullIndicatorLength(),  //         nullIndicatorLength
            actual.getNullIndOffset(),        //         nullIndicatorOffset
            dataOffset,                       //         dataOffset
            (ComSInt32) lenIndOffset,         //         varchar ind offset
            (ComSInt16) lenIndLen             //         varchar ind length
            );
          
          rsLoadMsg->setColumnDesc(i, info);
      
      } // for (i = 0; i < udrTdb.getNumParams(); i++)

      msg = rsLoadMsg;

      UdrDebug2("    TCB %p created UdrRSLoadMsg %p", this, msg);
    
    } // case UDR_MSG_RS_LOAD
    break;

    case UDR_MSG_UNLOAD:
    {
      setUdrTcbState(SENDING_UNLOAD);
      msg = new (h) UdrUnloadMsg(h);
      msg->setHandle(udrHandle_);
      UdrDebug2("    TCB %p created UdrUnloadMsg %p", this, msg);
    } // case UDR_MSG_UNLOAD
    break;

    case UDR_MSG_RS_UNLOAD:
    {
      setUdrTcbState(SENDING_UNLOAD);
      UdrRSUnloadMsg *m = new (h) UdrRSUnloadMsg(h);
      m->setHandle(udrHandle_);
      m->setRSHandle(rsHandle_);
      UdrDebug2("    TCB %p created UdrRSUnloadMsg %p", this, msg);
      msg = m;
    } // case UDR_MSG_RS_UNLOAD
    break;

    case UDR_MSG_RS_CLOSE:
    {
      UdrRSCloseMsg *m = new (h) UdrRSCloseMsg(h);
      m->setHandle(udrHandle_);
      m->setRSHandle(rsHandle_);
      UdrDebug2("    TCB %p created UdrRSCloseMsg %p", this, msg);
      msg = m;
    } // case UDR_MSG_RS_CLOSE
    break;

  
  default:
    ex_assert(FALSE, "Trying to send an invalid control message type");
    break;

  } // switch (t)
  
  //
  // Send the message object if one was created
  //
  if (msg != NULL && verifyUdrServerProcessId())
  {
    // Now send the control message and keep a pointer to the control
    // stream if callbacks are required
    NABoolean isTransactional = (transIdToSend == -1 ? FALSE : TRUE);
    s = new (getIpcHeap())
      UdrClientControlStream(myIpcEnv(),
                             (callbackRequired ? this : NULL),
                             myExeStmtGlobals(),
                             FALSE,
                             isTransactional);
    
#ifdef UDR_DEBUG
    if (doIpcTrace_ && traceFile_)
      s->setTraceFile(traceFile_);
    s->setTrustReplies(trustReplies_);
#endif
    if(udrTdb.isTmudf())
    {
      ex_assert(udrServer_->getUdrControlConnection(), "Invalid control connection (IPC error?)");
      s->addRecipient(udrServer_->getUdrControlConnection());
    }
    else
    {
      s->addRecipient(myExeStmtGlobals()->getUdrConnection());
    }

    UdrDebug2("    TCB %p created control stream %p", this, s);
    *s << *msg;
    if (callbackRequired)
    {
      UdrDebug2("    TCB %p storing pointer to control stream %p",
                this, s);
      outstandingControlStream_ = s;
    }
    UdrDebug2("    TCB %p sending data on control stream %p",
              this, s);

    if ((t == UDR_MSG_LOAD || t == UDR_MSG_RS_LOAD)
        && udrStats_)
    {
      ComUInt32 sizeOfMsg = msg->packedLength();
      udrStats_->getSentControlBuffers().addEntry(sizeOfMsg);
    }
    
    if (transIdToSend == -1)
      myExeStmtGlobals()->incrementUdrNonTxMsgsOut();
    else
      myExeStmtGlobals()->incrementUdrTxMsgsOut();

    NABoolean sendWaited = FALSE;
    s->send(sendWaited, transIdToSend);

  } // if (msg != NULL && verifyUdrServerProcessId())
  
  NABoolean result = TRUE;
  if (getIpcBroken())
  {
    result = FALSE;
  }
  
  //
  // It is OK for this method to abandon the message now. If the
  // stream is still holding a pointer to the message object then
  // the reference count will not drop to zero and the object will
  // not be destroyed until the stream releases it. The
  // deallocateMessage() call here is basically a wrapper around
  // decrRefCount() and is a no-op if msg is NULL.
  //
  deallocateMessage(msg);
  
  UdrDebug1("  [END ExUdrTcb::sendControlMessage()] %p", this);
  return result;

} // ExUdrTcb::sendControlMessage()

void ExUdrTcb::setTmUdfInfo(UdrLoadMsg *lm, const ExUdrTdb &tdb)
{
// the constructor allocates the needed memory for UdrTableInputInfo
  
  // fill in the tableInputs_ array
  Int16 i = 0;
  lm->setNumInputTables( numChildren());
  // allocate memory to hold table input info
  lm->allocateTableInputInfo();
  
  for (i=0; i < numChildren(); i++)
    {
      // retrieve the info from the tdb
      const UdrTableDescInfo *childtab = tdb.getTableDescInfo(i);
    
     
      UdrTableInputInfo  udrTabInput(i,
				     str_len(childtab->getCorrName()),
				     childtab->getCorrName(),
				     childtab->getNumColumns(),
				     childtab->getRowLength()
				     );
      
      //  construct the column desc info
      short j = 0;
      for (j=0; j < childtab->getNumColumns(); j++)
	{
	  const UdrColumnDescInfo *colinfo =childtab->getColumnDescInfo(j);
	  const char *colName = colinfo->getParamName();
	  ComUInt16 colNameLen = (ComUInt16)str_len(colName);
	  ComSInt16 fsType = colinfo->getType();
	  SQLTYPE_CODE ansiType = (SQLTYPE_CODE)Descriptor::ansiTypeFromFSType(fsType);
	 
	 
	  Attributes  *actual = tdb.getChildTableAttr(i,j);
  
	  UdrParameterInfo udrcolinfo(j,
				   colinfo->getFlags(),
				   fsType,
				   ansiType,
				   colNameLen,
				   colName,
				   colinfo->getPrecision(),
				   colinfo->getScale(),
				   colinfo->getEncodingCharSet(),
				   colinfo->getCollation(),
				   actual->getLength(),
				   actual->getNullIndicatorLength(),
				   actual->getNullIndOffset(),
				   actual->getOffset(),
				   actual->getVCLenIndOffset(),
				   actual->getVCIndicatorLength()
			       );
	  udrTabInput.setInTableColumnDesc(j,udrcolinfo,lm->getHeap());
	}// for j
				  
      lm->setChildTableInput(i, udrTabInput);
				    
    } //for i
  
}
void ExUdrTcb::releaseRequestBuffer()
{
  UdrDebug1("  [BEGIN ExUdrTcb::releaseRequestBuffer() %p]", this);

  //
  // No need to decrement a reference count now that data buffers
  // are managed by a buffered stream rather than on a NAMemory heap.
  //
  if (requestBuffer_)
  {
    UdrDebug1("    Request buffer %p is being released",
              requestBuffer_);
    requestBuffer_ = NULL;
  }

  UdrDebug1("  [END ExUdrTcb::releaseRequestBuffer() %p]", this);
}

void ExUdrTcb::releaseChildInputBuffer(Int32 i)
{
  UdrDebug1("  [BEGIN ExUdrTcb::releaseChildInputBuffer() 0x%08x]", this);

  //
  // No need to decrement a reference count now that data buffers
  // are managed by a buffered stream rather than on a NAMemory heap.
  //
  if (childInputBuffers_[i])
  {
    UdrDebug1("    Child Input buffer %p is being released",
              childInputBuffers_[i]);
    childInputBuffers_[i] = NULL;
  }

  UdrDebug1("  [END ExUdrTcb::releaseChildInputBuffer() %p]", this);
}

void ExUdrTcb::releaseReplyBuffer()
{
  UdrDebug1("  [BEGIN ExUdrTcb::releaseReplyBuffer() %p]", this);

  //
  // No need to decrement a reference count now that data buffers are
  // managed by a buffered stream rather than on a NAMemory heap.  The
  // stream will use the method msgObjectIsFree() to determine when a
  // receive buffer can be reused.
  //
  if (replyBuffer_)
  {
    UdrDebug1("    Reply buffer %p is being released", replyBuffer_);
    
    // We need to put the SqlBuffer into a state where it can be freed
    // once the reference counts of all tuple descriptors drop to
    // zero. This way, when the stream's garbage collection logic
    // calls msgObjIsFree() on this UdrDataBuffer instance, TRUE will
    // be returned if the SqlBuffer is no longer being referenced and
    // the UdrDataBuffer will be released by the stream. We call
    // bufferFull() on the SqlBuffer to accomplish this even though
    // that method name is a bit misleading.
    //
    // If the IPC integrity check failed during the unpacking of this
    // object then the SqlBuffer pointer will be NULL and that in
    // itself puts the UdrDataBuffer in a state where it will
    // eventually be released by the stream because
    // UdrDataBuffer::msgObjIsFree() will return TRUE.

    SqlBuffer *sqlBuf = replyBuffer_->getSqlBuffer();
    
    if (sqlBuf)
    {
      sqlBuf->bufferFull();
    }
    else
      UdrDebug0( "The sql buffer in reply data buffer is NULL");

    replyBuffer_ = NULL;
  }

  printDataStreamState();

  if (dataStream_)
  {
    UdrDebug0("    About to call dataStream_->cleanupBuffers()");
    dataStream_->cleanupBuffers();
    UdrDebug0("    Done");
    printDataStreamState();
  }
  
  UdrDebug1("  [END ExUdrTcb::releaseReplyBuffer() %p]", this);
}

NABoolean ExUdrTcb::replyBufferIsEmpty()
{
  //
  // Returns TRUE if we don't currently have a reply buffer or
  // if there are no more rows available in the reply buffer
  //
  NABoolean result = TRUE;
  if (replyBuffer_ && replyBuffer_->moreRows())
  {
    result = FALSE;
  }
  return result;
}

UdrDataBuffer *ExUdrTcb::getRequestBuffer()
{
  UdrDataBuffer *result = NULL;

  // We now need to allocate two message objects in the buffered data
  // stream. The first is a data header and the second a data
  // buffer. Stream buffer sizes should have been chosen so that both
  // objects fit in a single buffer. If allocation of the header
  // fails, we treat that like a blocked buffer pool and return a NULL
  // pointer. If allocation of the data buffer fails, that means the
  // stream buffer size was set too low and we trigger an assertion
  // failure.
  
  const ULng32 sqlBufferSize = myTdb().getRequestSqlBufferSize();

  NABoolean isResultSet = myTdb().isResultSetProxy();
  UdrMessageObj *header = NULL;

  if (isResultSet)
    header = new (*dataStream_) UdrRSDataHeader(udrHandle_, rsHandle_);
  else if (myTdb().isTmudf())
    header = new (*dataStream_) UdrTmudfDataHeader(udrHandle_);
  else
    header = new (*dataStream_) UdrDataHeader(udrHandle_);

  if (header == NULL)
  {
    UdrDebug0("  [WORK] Cannot allocate the data header");
    UdrDebug0("         Stream heap allocation failed");
    result = NULL;
  }
  else
  {
    result = new (*dataStream_, sqlBufferSize)
      UdrDataBuffer (sqlBufferSize, UdrDataBuffer::UDR_DATA_IN, NULL);
    ex_assert(result,
              "Could not allocate request. Stream buffer size may be low.");
    
    result->setHandle(udrHandle_);
    
    UdrDebug2("  [WORK] Created request buffer %p. SqlBuffer size %u",
              result, sqlBufferSize);
    UdrDebug1("  [WORK] Number of send buffers in the data stream is %d",
              dataStream_->numOfSendBuffers());
    ex_assert(dataStream_->numOfSendBuffers() == 1,
              "Multiple buffers for single request. Stream buf size too low.");
    
#ifdef UDR_DEBUG
    if (header && (getenv("UDR_INVALID_DATA_HANDLE") != NULL))
    {
      UdrDebug0("  [WORK] *** DATA HEADER HANDLE IS BEING INVALIDATED ***");
      header->setHandle(0);
    }
    if (getenv("UDR_INVALID_DATA_MSGTYPE") != NULL)
    {
      UdrDebug0("  [WORK] *** MESSAGE TYPE IS BEING INVALIDATED ***");
      result->setType(0);
    }
#endif // UDR_DEBUG
    
  } // if (header == NULL) else ...

  return result;

} // ExUdrTcb::getRequestBuffer()

UdrDataBuffer *ExUdrTcb::getReplyBuffer()
{
  NABoolean doIntegrityChecks = TRUE;
  NABoolean integrityCheckResult = TRUE;

#ifdef UDR_DEBUG
  // Integrity checks can be disabled in the debug build by an
  // environment setting. trustReplies_ stores the environment
  // setting and is initialized by initializeDebugVariables().
  doIntegrityChecks = !trustReplies_;
#endif

  if (replyBuffer_ == NULL)
  {
    IpcMessageObjType msgType;
    
    if (dataStream_ && dataStream_->getNextReceiveMsg(msgType)
        && dataStream_->getNextObjType(msgType))
    {
      switch (msgType)
      {
        case UDR_MSG_DATA_REPLY:
        {
          if (doIntegrityChecks)
          {
            IpcMessageObjSize objSize = dataStream_->getNextObjSize();
            replyBuffer_ = new (dataStream_->receiveMsgObj())
              UdrDataBuffer(dataStream_, objSize, integrityCheckResult);
          }
          else
          {
            replyBuffer_ = new (dataStream_->receiveMsgObj())
              UdrDataBuffer(dataStream_);
          }

          if (integrityCheckResult)
          {
            if (dataStream_->getNextObjType(msgType))
            {
#ifdef UDR_DEBUG
              UdrIpcObjectType udrType = (UdrIpcObjectType) msgType;
              if (udrType > UDR_IPC_FIRST && udrType < UDR_IPC_LAST)
                UdrDebug1("Found an object of type %s in the stream",
                          GetUdrIpcTypeString(udrType));
              else
                UdrDebug1("Found an object of type %d in the stream",
                          (Lng32) msgType);
#endif

              if (msgType == UDR_MSG_RS_INFO)
              {
                NAMemory *h = getIpcHeap();
                UdrRSInfoMsg *rsInfoMsg = new (h) UdrRSInfoMsg(h);
                
                UdrDebug0("About to extract UDR_MSG_RS_INFO from stream");
                
                integrityCheckResult =
                  dataStream_->extractNextObj(*rsInfoMsg, doIntegrityChecks);
                
                if (integrityCheckResult)
                {
                  ex_assert(rsInfo_,
                            "RS INFO msg received but ExRsInfo ptr is NULL");
                  
                  UdrDebug0("About to push RS info into statement globals");
                  
                  ComUInt32 numRs = rsInfoMsg->getNumResultSets();
                  for (ComUInt32 i=0; i < numRs; i++)
                  {
                    ResultSetInfo *rsInfo = &rsInfoMsg->getRSInfo(i);
                    const char *s = rsInfo->getProxySyntax();
                    rsInfo_->populate(i + 1, s);
                  }
                  
                  UdrDebug0("About to push UDR Server info to stmt globals");
                  rsInfo_->setUdrServer(udrServer_);
                  rsInfo_->setIpcProcessId(serverProcessId_);
                  rsInfo_->setUdrHandle(udrHandle_);
                  //Release the RS info message object
                  rsInfoMsg->decrRefCount();

                } // if (integrityCheckResult)

                else
                {
                  UdrDebug0("*** ERROR: extractNextObj() returned FALSE");
                }

              } // if (msgType == UDR_MSG_RS_INFO)
            } // if (dataStream_->getNextObjType(msgType))

            if (integrityCheckResult)
            {
#ifdef UDR_DEBUG
              if (doTrace_ || doIpcTrace_)
              {
                FILE *f = (traceFile_ ? traceFile_ : stdout);
                UdrPrintf(f,
                          "  [WORK] Received UdrDataBuffer %p, %d tupps",
                          replyBuffer_, 
                          replyBuffer_->getSqlBuffer()->getTotalTuppDescs());
              }
#endif
            }
          }
        }
        break;

      case UDR_MSG_ERROR_REPLY:
        {
          UdrDebug0("About to extract UdrErrorReply from stream");

          UdrErrorReply *reply = new (getIpcHeap()) UdrErrorReply(getIpcHeap());

          integrityCheckResult =
            dataStream_->extractNextObj(*reply, doIntegrityChecks);
          reply->decrRefCount();
          reply = NULL;
          if (!integrityCheckResult)
            {
              UdrDebug0("*** ERROR: extractNextObj() for diags returned FALSE");
            }

          IpcMessageObjType t;
        
          if (dataStream_->getNextObjType(t) && t == IPC_SQL_DIAG_AREA)
            {
              ComDiagsArea *returnedDiags = ComDiagsArea::allocate(getHeap());

              integrityCheckResult =
                dataStream_->extractNextObj(*returnedDiags, doIntegrityChecks);
              if (integrityCheckResult)
                {
                  UdrDebug1("  [WORK]   Diags arrived with this row, count %d",
                            (Lng32) returnedDiags->getNumber());
                  getOrCreateStmtDiags()->mergeAfter(*returnedDiags);
                }
              else
                {
                  UdrDebug0("*** ERROR: extractNextObj() for diags returned FALSE");
                }

              returnedDiags->decrRefCount();
              returnedDiags = NULL;
            }
          else
            {
              UdrDebug0("*** ERROR: Expected diags in the stream but found none");
              integrityCheckResult = FALSE;
            }
        }
        break;
        
      default:
        {
          //
          // An unexpected message type arrived. Treat this the same
          // as an integrity check failure.
          //
          UdrDebug0("  ***");
          UdrDebug0("  *** WARNING: An unexpected message arrived");
          UdrDebug1("  ***          Message type is %d", (Lng32) msgType);
          UdrDebug0("  ***");
          integrityCheckResult = FALSE;
        }
        break;
        
      } // switch (msgType)
    } // if (there are message objects to process)
  } // if (replyBuffer_ == NULL)

  if (!integrityCheckResult)
  {
    UdrDebug0("*** ERROR: Integrity check failed for data reply");
    ComDiagsArea *diags = getOrCreateStmtDiags();
    addIntegrityCheckFailureToDiagsArea(diags);
    releaseReplyBuffer();
    setUdrTcbState(IPC_ERROR);
  }
  
  return replyBuffer_;

} // ExUdrTcb::getReplyBuffer()

ExWorkProcRetcode ExUdrTcb::returnSingleRow()
{

  ExWorkProcRetcode result = WORK_OK;
  ControlInfo *ci = NULL;
  ex_queue_entry *upEntry = qParent_.up->getTailEntry();

  SqlBuffer *sqlBuf = replyBuffer_->getSqlBuffer();
  ex_assert(sqlBuf, "UDR reply buffer is corrupt or contains no data");

  tupp output_row;
  ULng32 numOutputValues = myTdb().getNumOutputValues();
  NABoolean moveOutputValues =
    (myTdb().getOutputExpression() == NULL ? FALSE : TRUE);

  // At this point we do not know if the next entry in the reply
  // buffer is data or a Q_NO_DATA indicator. But we allocate a
  // tupp for our output row ahead of time to avoid the situation
  // where we have extracted a data row from the reply buffer but
  // then our output buffer pool is full. In the case where a 
  // tupp is allocated but never used (because the next reply buffer
  // entry is not data) the tupp will eventually be recycled 
  // because its reference count stays at zero until it gets placed
  // into an ATP. If there was a way to "peek" into the reply buffer
  // and see if there were more data rows, then we could avoid 
  // allocating the output tupp until we were sure there was data
  // to process.
  if (moveOutputValues)
  {
    short ok = outputPool_->
      get_free_tuple(output_row, (Lng32) myTdb().getOutputRowLen());
    if (ok != 0)
      result = WORK_POOL_BLOCKED;
  }

  if (result == WORK_OK)
  {
    // Get a pointer to the next reply row. If there are diags
    // associated with the reply row they are not packed into
    // the SqlBuffer. We will extract a ComDiagsArea from the
    // message stream later if the ControlInfo for this row
    // indicates that there are diags.
    tupp reply_row;
    NABoolean endOfData = sqlBuf->moveOutSendOrReplyData(
      FALSE,                 // [IN] sending? (vs. replying)
      &(upEntry->upState),   // [OUT] queue state
      reply_row,             // [OUT] new data tupp_descriptor
      &ci,                   // [OUT] new ControlInfo area
      NULL,                  // [OUT] new diags area
      NULL                   // [OUT] new stats area
      );
    
    ex_queue::up_status status = upEntry->upState.status;

    UdrDebug2("  [WORK] Next reply row: eod %s, status %s",
              TF_STRING(endOfData), GetUpStatusString(status));
    
    if (!endOfData)
    {
      // Process a row from the reply buffer
      ex_queue_entry *downEntry = qParent_.down->getHeadEntry();
      ExUdrPrivateState &down_pstate =
        *((ExUdrPrivateState *) downEntry->pstate);
      upEntry->upState.downIndex = qParent_.down->getHeadIndex();
      upEntry->upState.parentIndex = downEntry->downState.parentIndex;

      // Check for a diags area coming back with this row
      ComDiagsArea *returnedDiags = NULL;
      NABoolean doIntegrityChecks = TRUE;
      NABoolean integrityCheckResult = TRUE;

#ifdef UDR_DEBUG
      // Integrity checks can be disabled in the debug build by an
      // environment setting. trustReplies_ stores the environment
      // setting and is initialized by initializeDebugVariables().
      doIntegrityChecks = !trustReplies_;
#endif

      if (!ci)
      {
        UdrDebug0("*** ERROR: No ControlInfo available for this reply row");
        integrityCheckResult = FALSE;
      }
      
      if (ci && ci->getIsExtDiagsAreaPresent())
      {
        IpcMessageObjType t;
        returnedDiags = ComDiagsArea::allocate(getHeap());
        
        if (dataStream_->getNextObjType(t) && t == IPC_SQL_DIAG_AREA)
        {
          integrityCheckResult =
            dataStream_->extractNextObj(*returnedDiags, doIntegrityChecks);
          if (!integrityCheckResult)
          {
            UdrDebug0("*** ERROR: extractNextObj() for diags returned FALSE");
          }
        }
        else
        {
          UdrDebug0("*** ERROR: Expected diags in the stream but found none");
          returnedDiags->decrRefCount();
          returnedDiags = NULL;
          integrityCheckResult = FALSE;
        }

        if (integrityCheckResult)
        {
          UdrDebug1("  [WORK]   Diags arrived with this row, count %d",
                    (Lng32) returnedDiags->getNumber());
        }
      }

      if (integrityCheckResult)
      {
        // The server should only return error diagnostics with a
        // Q_SQLERROR entry. But if we happen to receive error
        // diagnostics with a Q_OK_MMORE entry, let's tolerate that
        // and treat it as a Q_SQLERROR entry.
        if (status == ex_queue::Q_OK_MMORE &&
            returnedDiags &&
            returnedDiags->getNumber(DgSqlCode::ERROR_) > 0)
          status = ex_queue::Q_SQLERROR;

        switch (status) 
        {
          case ex_queue::Q_OK_MMORE: 
          {
            if (down_pstate.step_ != CANCEL_AFTER_SEND)
            {
              ComDiagsArea *validationDiags = NULL;
              if (validateDataRow(reply_row, validationDiags) == FALSE)
              {
                ex_assert(validationDiags,
                          "validateDataRow() did not return diagnostics");
                insertUpQueueEntry(ex_queue::Q_SQLERROR, validationDiags);
                validationDiags->decrRefCount();
              }
              else
              {
                // We have a data row to evaluate.
                // 
                // Steps to perform are:
                // 1. Initialize the up queue entry ATP
                // 2. Evaluate the output expression if one is present
                // 3. Evaluate the predicate if one is present
                // 4. Copy server diags to the up queue entry
                // 5. Return an up queue entry
                // 
                // Skip steps 4 and 5 if the predicate is not satisfied.
                // Skip step 3 if errors are encountered in step 2.

                NABoolean outputExprError = FALSE;
                NABoolean predSatisfied = TRUE;

                // 1. Initialize the up entry ATP. copyAtp() moves any
                // diags associated with the down entry ATP to the up
                // entry ATP.
                upEntry->copyAtp(downEntry);
                if (numOutputValues > 0)
                {
                  upEntry->getTupp(myTdb().getNumOutputTupps() - 1) =
                    (moveOutputValues ? output_row : reply_row);
                }

                // 2. Evaluate the output expression
                if (moveOutputValues)
                {
                  workAtp_->getTupp(myTdb().getReplyTuppIndex()) = reply_row;
                  
                  // Evaluate the output expression. If diags are
                  // created they will be stored in the up entry
                  // ATP. If errors occur during expression evaluation
                  // we will go ahead and insert a data row plus diags
                  // in the up queue.
                  ex_expr::exp_return_type expStatus =
                    myTdb().getOutputExpression()->eval(upEntry->getAtp(),
                                                        workAtp_);
                  if (expStatus != ex_expr::EXPR_OK)
                  {
                    outputExprError = TRUE;
                    UdrDebug1("  [WORK]   outputExpr.eval() returned %s",
                              GetExpStatusString(expStatus));
                  }
                  
                  workAtp_->getTupp(myTdb().getReplyTuppIndex()).release();
                }

                // 3. Evaluate the predicate
                ex_expr *predicate = myTdb().getPredicate();
                if (!outputExprError && predicate)
                {
                  ex_expr::exp_return_type expStatus =
                    predicate->eval(upEntry->getAtp(), 0);

                  if (expStatus == ex_expr::EXPR_ERROR)
                  {
                    UdrDebug1("  [WORK]   predicate.eval() returned %s",
                              GetExpStatusString(expStatus));
                  }
                  else if (expStatus == ex_expr::EXPR_FALSE)
                  {
                    // Record the fact that the predicate is not
                    // satisfied. Also decrement the reference count
                    // on any diags area pointed to by this up queue
                    // entry because the entry is about to be
                    // abandoned.
                    predSatisfied = FALSE;
                    upEntry->setDiagsArea(NULL);
                  }
                }
                
                // 4. Move server diags to the up queue entry
                if (predSatisfied && returnedDiags)
                {
                  ComDiagsArea *atpDiags = upEntry->getDiagsArea();
                  if (atpDiags == NULL)
                  {
                    // setDiagsArea() does not increment the reference count
                    upEntry->setDiagsArea(returnedDiags);
                    returnedDiags->incrRefCount();
                  }
                  else
                  {
                    atpDiags->mergeAfter(*returnedDiags);
                  }
                }
                  
                // 5. Return an up queue entry
                if (predSatisfied)
                {
                  down_pstate.matchCount_++;
                  upEntry->upState.setMatchNo(down_pstate.matchCount_);
                  qParent_.up->insert();
                }

                // Increment actual row count (1 by default) in
                // statistics area
                if (udrBaseStats_ != NULL)
                  udrBaseStats_->incActualRowsReturned();

              } // if (validateDataRow()) else ...
            } // if (down_pstate.step_ != CANCEL_AFTER_SEND)
          } // case ex_queue::Q_OK_MMORE
          break;
          
          case ex_queue::Q_NO_DATA:
          {
            insertUpQueueEntry(ex_queue::Q_NO_DATA, returnedDiags);
          } // case ex_queue::Q_NO_DATA
          break;
          
          case ex_queue::Q_SQLERROR:
          {
            insertUpQueueEntry(ex_queue::Q_SQLERROR, returnedDiags);
          } // case ex_queue::Q_SQLERROR
          break;
          
          case ex_queue::Q_INVALID:
          default:
          {
            UdrDebug0("*** ERROR: Invalid queue status for this reply row");
            integrityCheckResult = FALSE;
          } // case ex_queue::Q_INVALID
          break;
          
        } // switch (status)
      } // if (integrityCheckResult)

      if (!integrityCheckResult)
      {
        UdrDebug0("*** ERROR: Integrity check failed for this reply row");

        ComDiagsArea *newDiags = ComDiagsArea::allocate(getHeap());
        addIntegrityCheckFailureToDiagsArea(newDiags);

        // Currently our strategy is to treat an integrity check
        // failure as an IPC error. This has the side-effect of
        // causing the TCB to return WORK_BAD_ERROR to the
        // scheduler. When WORK_BAD_ERROR is returned our up queue
        // entries get ignored and diagnostics associated with up
        // queue entries do not propagate to the application. So even
        // though we are working on a specific row right now, we need
        // to put diagnostics in the statement diags area so that the
        // application is sure to see them. If we changed our error
        // handling strategy so that integrity check failures did not
        // cause the TCB to return WORK_BAD_ERROR, then we could put
        // newDiags in the Q_SQLERROR up queue entry instead of in the
        // statement diags area.
        // insertUpQueueEntry(ex_queue::Q_SQLERROR, newDiags);
        insertUpQueueEntry(ex_queue::Q_NO_DATA);
        ComDiagsArea *stmtDiags = getOrCreateStmtDiags();
        stmtDiags->mergeAfter(*newDiags);

        releaseReplyBuffer();
        setUdrTcbState(IPC_ERROR);
        newDiags->decrRefCount();
      }
      
      if (returnedDiags)
      {
        returnedDiags->decrRefCount();
        returnedDiags = NULL;
      }
      
    } // if (!endOfData)
  } // if (result == WORK_OK)

  return result;

} // ExUdrTcb::returnSingleRow()

//
// Cancel processing
//
ExWorkProcRetcode ExUdrTcb::workCancel()
{
  UdrDebug1("[BEGIN TCB CANCEL] %p", this);

  ExWorkProcRetcode result = WORK_OK;

  // We will keep track of whether any UDR server replies are
  // still required after cancelled entries have been processed.
  // Replies will be required if some, not all, down queue
  // entries have been cancelled and at least one not-cancelled
  // entry has already been sent to the UDR server.
  NABoolean anyEntriesCancelled = FALSE;
  NABoolean repliesRequired = FALSE;
  NABoolean needToSendCancelToServer = FALSE;
  NABoolean isResultSet = myTdb().isResultSetProxy();

  if (!anyOutstandingQueueRequests())
  {
    UdrDebug0("  No outstanding entries to cancel");
  }
  else
  {
    // Down queue entries that are still being processed will have an
    // index i, where i logically falls between the head index and
    // nextToSend_. If any such entry has been cancelled (state was
    // changed to GET_NOMORE), then process the cancel request for
    // that entry.
    for (queue_index i = qParent_.down->getHeadIndex(); i != nextToSend_; i++)
    {
      ex_queue_entry *downEntry = qParent_.down->getQueueEntry(i);
      ex_queue::down_request &requestType = downEntry->downState.request;
      ExUdrPrivateState &down_pstate = 
        *((ExUdrPrivateState *) downEntry->pstate);
      UdrTcbSendStep &sendStep = down_pstate.step_;

      switch (requestType)
      {
      case ex_queue::GET_N:
      case ex_queue::GET_ALL:
        {
          if (sendStep == STARTED)
          {
            repliesRequired = TRUE;
          }
        } // case GET_N, GET_ALL
        break;

      case ex_queue::GET_NOMORE:
        {
          switch (sendStep)
          {
          case NOT_STARTED:
            sendStep = CANCEL_BEFORE_SEND;
            anyEntriesCancelled = TRUE;
            break;
          case STARTED:
            sendStep = CANCEL_AFTER_SEND;
            anyEntriesCancelled = TRUE;
            needToSendCancelToServer = TRUE;
	    for (Int32 j=0; j< numChildren(); j++)
	      {
		qChild_[j].down->cancelRequestWithParentIndex(i);
	      }
            break;
          default:
            break;
          } // switch (sendStep)

        } // case GET_NOMORE
        break;

      default:
        {
          ex_assert(FALSE, "Invalid request type in TCB down queue");
        } // default
        break;

      } // switch (requestType)

    } // for (each queue index i)
  } // if (!anyOutstandingQueueRequests()) else

  if (repliesRequired)
  {
#ifdef UDR_DEBUG
    // Print a message even if the UDR_DEBUG environment variable
    // is not set. This may expose cancel situations that have not
    // yet been considered.
    FILE *f = (traceFile_ ? traceFile_ : stdout);
    UdrPrintf(f, "  ***");
    UdrPrintf(f, "  *** WARNING: Some but not all queue entries cancelled");
    UdrPrintf(f, "  ***");
#endif // UDR_DEBUG
  }

  else if (anyEntriesCancelled)
  {
    UdrDebug0("  ***");
    UdrDebug0("  *** WARNING: All queue entries were cancelled");
    UdrDebug0("  ***");

    if (isResultSet)
    {
      if (needToSendCancelToServer)
      {
        // If this is a result set, we tell the server that it can stop
        // producing rows. Note that rows may still continue to
        // arrive. All we are doing is telling the server to stop
        // producing as soon as possible, we are NOT putting the TCB
        // into a state where it cannot (or will not) process more reply
        // rows.
        
        // First make sure the server is working in the current
        // transaction

        if (rsInfo_)
          rsInfo_->enterUdrTx(*myExeStmtGlobals());
        
#ifdef UDR_DEBUG
        // Print a message if either the UDR_DEBUG or
        // UDR_CONTINUE_DEBUG environment variable is set.
        if (doTrace_ || (getenv("UDR_CONTINUE_DEBUG") != NULL))
        {
          FILE *f = (traceFile_ ? traceFile_ : stdout);
          UdrPrintf(f, "  [CANCEL] Sending RS CLOSE");
        }
#endif
        
        // Now send an RS CLOSE request
        sendControlMessage(UDR_MSG_RS_CLOSE,
                           FALSE); // FALSE means the TCB does not require
                                   // a callback when I/O completes
      }
    }
    else
    {
      // The commentary below applies to CALL statements, not result
      // sets, because for CALL statements we do not currently have any
      // type of cancel protocol with the server.
      //
      // What we would like to do now is tell the server to stop working
      // on our requests. But the UDR server is currently a serial, 
      // single-threaded server and does not support any sort of cancel
      // protocol.
      //
      // Thinking about how we got here, it is likely that the executor
      // is sending out internal cancel requests because on NSK we do not
      // yet support application-initiated cancel. An application can stop
      // a SQL statement with Ctrl-C but that event is detected and handled
      // in the IPC layers. Since it is likely that this cancellation is
      // only being done for internal correctness, not because the application
      // wants to interrupt SQL, we will not take aggressive action like
      // killing the server. We will do nothing and let normal work method
      // processing handle UDR replies.
      //
      // For what it's worth, the following line could be used to kill
      // the UDR server...
      //   udrServer_->kill(getOrCreateStmtDiags());
      // 
    }

  } // else if (anyEntriesCancelled)

  UdrDebug2("[END TCB CANCEL] %p. Return value is %s\n",
            this, GetWorkRetcodeString(result));

  return result;
}

// Returns TRUE if a UDR Server connection exists and the running
// server process ID matches the server process ID stored in this
// TCB. Generates a diagnostic if this TCB was storing a valid
// process ID that does not match the running server.
NABoolean ExUdrTcb::verifyUdrServerProcessId()
{
  NABoolean result = FALSE;
  if (!getIpcBroken() && udrServer_ && !ProcessIdIsNull(serverProcessId_))
  {
    if (udrServer_->getState() != ExUdrServer::EX_UDR_BROKEN &&
        udrServer_->getServerProcessId() == serverProcessId_)
    {
      result = TRUE;
    }
    else
    {
      setUdrTcbState(IPC_ERROR);
      ComDiagsArea *da = getOrCreateStmtDiags();
      *da << DgSqlCode(-EXE_UDR_SERVER_WENT_AWAY);

#ifdef UDR_DEBUG
      char buf1[300];
      serverProcessId_.toAscii(buf1, 300);
      UdrDebug0("***");
      UdrDebug1("*** WARNING: UDR Server for TCB %p is not running", this);
      UdrDebug1("***          TCB state is %s", getUdrTcbStateString(state_));
      UdrDebug1("***          Server for this TCB was %s", buf1);
      UdrDebug0("***");
      if (udrServer_ && udrServer_->getUdrControlConnection())
      {
        char buf2[300];
        const IpcProcessId &runningServer =
          udrServer_->getUdrControlConnection()->getOtherEnd();
        runningServer.toAscii(buf2, 300);
        UdrDebug1("***          Running server is %s", buf2);
      }
      else
      {
        UdrDebug0("***          No server is currently running");
      }
#endif // UDR_DEBUG

    }
  }

  return result;
}

void ExUdrTcb::reportDataArrival()
{
  UdrDebug1("  Data arrived for TCB %p. TCB is being scheduled", this);

  // It is possible that we are in the WORK_IO_ACTIVE state and
  // conditions to transition back to WORK have now been met. For
  // example we might have just completed all outstanding I/O. The
  // following method checks the conditions and transitions to WORK if
  // appropriate. See commentary in the method for more detail.
  attemptTransitionToWorkState();

  printDataStreamState();
  tickleSchedulerWork();
}

void ExUdrTcb::attemptTransitionToWorkState()
{
  // We can transition from WORK_IO_ACTIVE to WORK if the following
  // are true:
  // * No down queue requests being processed by the server
  // * No pending I/O (such as continue requests) in the data stream
  //
  // Things to note:
  //  
  // * Within other methods of this TCB, any time one of these
  //   conditions potentially changes (for example a receive callback
  //   arrives) then this method should be called to see if a transition
  //   from WORK_IO_ACTIVE to WORK is appropriate.
  //
  // * Sending a data request while the TCB is in the WORK_IO_ACTIVE
  //   state can lead to problems. The data stream does not seem to
  //   tolerate this. Callbacks can become unreliable or we encounter
  //   stream limits and never drive I/O to completion, leading to
  //   hangs. The code to send data requests to the server is in method
  //   buildAndSendRequestBuffer() and that method contains logic to
  //   never send a request unless the TCB is in the WORK state.
  //
  if (state_ == WORK_IO_ACTIVE &&
      !anyOutstandingQueueRequests() &&
      dataStream_->numOfResponsesPending() == 0)
    setUdrTcbState(WORK);
}

NABoolean ExUdrTcb::setUdrTcbState(UdrTcbState target)
{
  NABoolean result = TRUE;
  UdrTcbState source = state_;

#ifdef UDR_DEBUG
  if (doTrace_ || doStateTrace_)
  {
    FILE *f = traceFile_;
    UdrPrintf(f, "*** TCB %p state transition: %s -> %s",
              this, getUdrTcbStateString(source),
              getUdrTcbStateString(target));
  }
#endif // UDR_DEBUG

  //
  // This switch statement sets result to FALSE if an unexpected transition
  // is happening. It may be that the unexpected transition is valid, but 
  // happens to be one that the code does not handle correctly or gracefully
  // yet. If that is the case then a new transition should be added to the
  // set of "expected" transitions and changes should be made in ExUdrTcb
  // code to handle the new transition. 
  //
  // Current thinking about expected transitions:
  // - FIXUP should never be skipped
  // - We cannot transition to DONE without sending an UNLOAD if
  //   a LOAD has been sent. But if an IpcError occurred then no
  //   UNLOAD needs to be sent.
  // - We allow a transition from IPC_ERROR back to FIXUP.
  // - We do not transition out of DONE. DONE means the TCB is going away.
  //
  switch (source)
  {
  case BUILD:
    if (target != FIXUP && target != DONE)
    {
      result = FALSE;
    }
    break;
  case FIXUP:
    if (target != SENDING_LOAD && target != DONE)
    {
      result = FALSE;
    }
    break;
  case SENDING_LOAD:
    if (target != WORK && target != LOAD_FAILED && 
        target != SENDING_UNLOAD && target != IPC_ERROR)
    {
      result = FALSE;
    }
    break;
  case LOAD_FAILED:
    if (target != SENDING_UNLOAD)
    {
      result = FALSE;
    }
    break;
  case WORK:
    if (target != WORK_IO_ACTIVE && target != SENDING_UNLOAD
        && target != IPC_ERROR && target != SCALAR_INPUT_READY_TO_SEND)
    {
      result = FALSE;
    }
    break;
  case WORK_IO_ACTIVE:
    if (target != WORK && target != SENDING_UNLOAD && target != IPC_ERROR &&
	target != READ_TABLE_INPUT_FROM_CHILD)
    {
      result = FALSE;
    }
    break;
  case SENDING_UNLOAD:
    if (target != IPC_ERROR && target != DONE)
    {
      result = FALSE;
    }
    break;
  case IPC_ERROR:
    if (target != DONE && target != FIXUP)
    {
      result = FALSE;
    }
    break;
  case DONE:
    result = FALSE;
    break;
  case SCALAR_INPUT_READY_TO_SEND:
    if (target != WORK_IO_ACTIVE)
      {
	return FALSE ;
      }
    break;
  case READ_TABLE_INPUT_FROM_CHILD:
    if (target != RETURN_ROWS_FROM_CHILD)
      {
	return FALSE;
      }
    break;
  case RETURN_ROWS_FROM_CHILD :  
    if (target != CHILD_INPUT_READY_TO_SEND )
      {
	return FALSE;
      }   
    break;
 case CHILD_INPUT_READY_TO_SEND :
   if (target !=  WORK_IO_ACTIVE)
     {
       return FALSE;
     }
   break;
  default:
    result = FALSE;
    break;
  }
  
  if (!result)
  {
    char buf[256];
    str_sprintf(buf, "Invalid UDR state transition: %s -> %s",
                getUdrTcbStateString(source), getUdrTcbStateString(target));
    ex_assert(FALSE, buf);
  }
  else
  {
    state_ = target;
  }

  return result;
}

const char *ExUdrTcb::getUdrTcbStateString(UdrTcbState s)
{
  switch (s)
  {
    case BUILD:           return "BUILD";
    case FIXUP:           return "FIXUP";
    case SENDING_LOAD:    return "SENDING_LOAD";
    case LOAD_FAILED:     return "LOAD_FAILED";
    case WORK:            return "WORK";
    case WORK_IO_ACTIVE:  return "WORK_IO_ACTIVE";
    case SENDING_UNLOAD:  return "SENDING_UNLOAD";
    case IPC_ERROR:       return "IPC_ERROR";
    case DONE:            return "DONE";
    case SCALAR_INPUT_READY_TO_SEND: return "SCALAR_INPUT_READY_TO_SEND";
    case READ_TABLE_INPUT_FROM_CHILD : return "READ_TABLE_INPUT_FROM_CHILD";
    case RETURN_ROWS_FROM_CHILD: return "RETURN_ROWS_FROM_CHILD";
    case CHILD_INPUT_READY_TO_SEND: return "CHILD_INPUT_READY_TO_SEND";
    default:              return ComRtGetUnknownString((Int32) s);
  }
}

ExWorkProcRetcode ExUdrTcb::continueRequest()
{
  ExWorkProcRetcode result = WORK_OK;

  if (anyOutstandingQueueRequests() && state_ == WORK_IO_ACTIVE)
  {
    // Send continue requests until the limit for outstanding requests
    // or for incoming and used buffers is reached
    dataStream_->cleanupBuffers();

    NABoolean done = FALSE;
    while (!done)
    {
      if (getIpcBroken())
      {
        result = WORK_BAD_ERROR;
        done = TRUE;
      }
      else if (dataStream_->sendLimitReached())
      {
        UdrDebug0("  [CONTINUE] Cannot send continue request");
        UdrDebug0("             sendLimitReached() returned TRUE");
        printDataStreamState();
        result = WORK_OK;
        done = TRUE;
      }
      else if (dataStream_->inUseLimitReached())
      {
        // We can't send another continue request down because there
        // is no room up here to take the reply.  Return a status
        // that causes this task to be rescheduled. Next time,
        // methods cleanupBuffers() and checkReceive() can reduce
        // the number of in-use buffers and of unread buffers in the
        // receive queue.
        UdrDebug0("  [CONTINUE] Cannot send continue request");
        UdrDebug0("             inUseLimitReached() returned TRUE");
        printDataStreamState();
        result = WORK_POOL_BLOCKED;
        done = TRUE;
      }
       
      else
      {
	if (myTdb().isTmudf())
	  {
	  
	    UdrDebug0("Check for input buffers");
	    printDataStreamState();
	    if (dataStream_->numOfInputBuffers()> 0)
	      {
		UdrDebug0("Not sending continue request since input buffers is >0");
		result = WORK_OK;
		done = TRUE;
		break;
	      }  
	  
	}
        NABoolean isResultSet = myTdb().isResultSetProxy();
        UdrMessageObj *m = NULL;

        if (isResultSet)
          m = new (*dataStream_) UdrRSContinueMsg(udrHandle_, rsHandle_);
        else
          m = new (*dataStream_) UdrContinueMsg(udrHandle_);

        if (m == NULL)
        {
          UdrDebug0("  [CONTINUE] Cannot send continue request");
          UdrDebug0("             Buffer allocation failed");
          printDataStreamState();
          result = WORK_POOL_BLOCKED;
          done = TRUE;
        }
        else
        {
          // If this is a CALL that produces result sets or a result
          // set (both are indicated by a non-NULL rsInfo_ pointer)
          // then we may need to bring the UDR server into the current
          // transaction before sending the continue request.
          if (rsInfo_)
            rsInfo_->enterUdrTx(*myExeStmtGlobals());

          // Determine the transid we should send in the message. We
          // use the statement transid if all the following are true
          // a. the TDB says we require a transaction
          // b. this is NOT a CALL that returns result sets
          // c. this is NOT a result set
          // 
          // Note: (b) and (c) are both indicated by rsInfo_ == NULL
          // 
          Int64 &stmtTransId = myExeStmtGlobals()->getTransid();
          Int64 transIdToSend = -1; // -1 is an invalid transid
          if (dataRequestsAreTransactional())
            transIdToSend = stmtTransId;

#ifdef UDR_DEBUG
          // Print a message if tracing is enabled or
          // UDR_CONTINUE_DEBUG environment variable is set.
          if (doTrace_ ||
              doIpcTrace_ ||
              (getenv("UDR_CONTINUE_DEBUG") != NULL))
          {
            FILE *f = (traceFile_ ? traceFile_ : stdout);
            char stmtTx[256];
            char msgTx[256];
            TransIdToText(stmtTransId, stmtTx, 256);
            TransIdToText(transIdToSend, msgTx, 256);
            if (isResultSet)
              UdrPrintf(f, "  [CONTINUE] Sending RS CONTINUE %d",
                        (int) continueMsgsSent_ + 1);
            else
              UdrPrintf(f,
                        "  [CONTINUE] Sending CONTINUE %d, msg tx %s",
                        (int) continueMsgsSent_ + 1, msgTx);
          }

          if (getenv("UDR_INVALID_CONTINUE_HANDLE") != NULL)
          {
            UdrDebug0("  [CONTINUE] *** HANDLE IS BEING INVALIDATED ***");
            m->setHandle(0);
          }
#endif // UDR_DEBUG

          if (udrStats_)
          {
            udrStats_->getSentContinueBuffers().addEntry(sizeof(*m));
          }

          if (state_ == WORK)
            setUdrTcbState(WORK_IO_ACTIVE);

          if (dataRequestsAreTransactional())
            myExeStmtGlobals()->incrementUdrTxMsgsOut();
          else
            myExeStmtGlobals()->incrementUdrNonTxMsgsOut();

          dataStream_->sendRequest(transIdToSend);
          continueMsgsSent_++;

          printDataStreamState();
        }
      }

    } // while (!done)
  } // if (anyOutstandingQueueRequests() && state_ == WORK_IO_ACTIVE)
    
  return result;

} // ExUdrTcb::continueRequest()

// work method for handling table mapping udfs.
// This work method will handle scalar inputs from the parent and
// table valued inputs from one or more children.

ExWorkProcRetcode ExUdrTcb::tmudfWork()
{
#ifdef UDR_DEBUG
  static Lng32 workCount = 0;
  if (doTrace_)
  {
    workCount++;
    FILE *f = traceFile_;
    char text[256];
    TransIdToText(myExeStmtGlobals()->getTransid(), text, 256);
    ex_queue *up = qParent_.up;
    ex_queue *dn = qParent_.down;
    UdrPrintf(f, "[BEGIN TMUDF WORK %d] %p", workCount, this);
    UdrPrintf(f, "  [TMUDF WORK] Stmt tx %s", text);
    UdrPrintf(f, "  [TMUDF WORK] TCB state %s", getUdrTcbStateString(state_));
    UdrPrintf(f, "  [TMUDF WORK] Down queue: head %d, tail %d, len %d, size %d",
              (Lng32) dn->getHeadIndex(), (Lng32) dn->getTailIndex(),
              (Lng32) dn->getLength(), (Lng32) dn->getSize());
    UdrPrintf(f, "  [TMUDF WORK] Up queue: head %d, tail %d, len %d, size %d",
              (Lng32) up->getHeadIndex(), (Lng32) up->getTailIndex(),
              (Lng32) up->getLength(), (Lng32) up->getSize());
    UdrPrintf(f, "  [TMUDF WORK] Next to send: %d", (Lng32) nextToSend_);
    printDataStreamState();
  }
#endif // UDR_DEBUG
  ExWorkProcRetcode result = WORK_OK;

  result = tmudfCheckReceive();
  UdrDebug1("  [TMUDF WORK] tmudfCheckReceive() returned %s",
            GetWorkRetcodeString(result));
  if (result == WORK_OK)
  {
  result = tmudfCheckSend();
   UdrDebug1("  [TMUDF WORK] tmudfCheckSend() returned %s",
              GetWorkRetcodeString(result));
  }
  if (result == WORK_OK)
  {
    result = continueRequest();
    UdrDebug1("  [TMUDF WORK] continueRequest() returned %s",
              GetWorkRetcodeString(result));
  }

 if (qParent_.down->isEmpty())
  {
    UdrDebug0("  [TMUDF WORK] Down queue is empty");

    // There are no more parent requests to process. We now want to
    // perform garbage collection of buffers in the data stream. 
    // There seems to be a bug in stream message processing code 
    // that moves the message from RECEIVE queue to IN USE queue only 
    // when you try to get the next message from the stream and it 
    // only garbage collects the IN USE queue. 
    // So we ask the stream for one more buffer even though we know
    // there won't be one. By doing this we guarantee that all
    // incoming buffers have moved off the stream's RECEIVE queue and
    // on to the IN USE queue. 
    // If there does happen to be another buffer in the stream, 
    // that is an internal error.
    //
    // This cleanup technique is borrowed from the send top node (code
    // is in ex_send_top.cpp) and fixes the memory leak reported in
    // solution 10-050707-9503.
  
    UdrDebug0("  [TMUDF WORK] Releasing unused IPC buffers in the data stream...");
    IpcMessageObjType msgType;
  
    if (dataStream_ && dataStream_->getNextReceiveMsg(msgType))
    {
      char buf[256];
      dataStream_->getNextObjType(msgType);
      str_sprintf(buf, "An unexpected message of type %d arrived",
                  (Int32) msgType);
      ex_assert(FALSE, buf);
    }
    dataStream_->releaseBuffers();
    UdrDebug0("  [TMUDF WORK] Done releasing IPC buffers");
  
  } // if (qParent_.down->isEmpty())
#ifdef UDR_DEBUG
  if (doTrace_)
  {
    FILE *f = traceFile_;
    UdrPrintf(f, "  [TMUDF WORK] UDR TX messages outstanding: %d",
              (Lng32) myExeStmtGlobals()->numUdrTxMsgsOut());
    UdrPrintf(f, "  [TMUDF WORK] UDR Non-TX messages outstanding: %d",
              (Lng32) myExeStmtGlobals()->numUdrNonTxMsgsOut());
    UdrPrintf(f, "  [TMUDF WORK] TCB state: %s", getUdrTcbStateString(state_));
    UdrPrintf(f, "[END TMUDF WORK %d] 0x%08x. Return value is %s\n",
              workCount, this, GetWorkRetcodeString(result));
  }
#endif // UDR_DEBUG

  return result;
  
} // ExUdrTdb::tmudfWork()

ExWorkProcRetcode ExUdrTcb::tmudfCheckReceive()
{
  // Work method helper function that attempts to process output rows
  ExWorkProcRetcode result = WORK_OK;
  NABoolean done = FALSE;
  UdrDebug0("  [TMUDF WORK] Inside tmudfCheckReceive");
  while (!done && result == WORK_OK
         && anyOutstandingQueueRequests() 
	 && !qParent_.up->isFull() 
	 && ((state_ != READ_TABLE_INPUT_FROM_CHILD) && 
	     (state_ != RETURN_ROWS_FROM_CHILD)))
    {
      if (getIpcBroken())
	{
	  UdrDebug0("  [TMUDF WORK] IPC is broken");
	  result = WORK_BAD_ERROR;
	}
      else
	{
	  ex_queue_entry *downEntry = qParent_.down->getHeadEntry();
	  ExUdrPrivateState &down_pstate = 
	    *((ExUdrPrivateState *) downEntry->pstate);
      
	  switch (down_pstate.step_)
	    {
	    case CANCEL_BEFORE_SEND:
	      insertUpQueueEntry(ex_queue::Q_NO_DATA);
	      break;
          
	    case PRODUCE_ERROR_REPLY:
	      // An error occurred before the entry could be sent, probably
	      // during expression evaluation. If any diags were generated
	      // they were placed in the down entry ATP and will be copied
	      // to the up entry ATP by the following function.
	      insertUpQueueEntry(ex_queue::Q_SQLERROR);
              down_pstate.step_ = PRODUCE_EOD_AFTER_ERROR;
              break;

	    case PRODUCE_EOD_AFTER_ERROR:
              {
                // consume any remaining rows from the children, before
                // producing the EOD
                for (Int32 i=0; i<numChildren(); i++)
                  if (tmudfStates_[i] == READING_FROM_CHILD)
                    {
                      // remove child up queue entries until we see an EOD
                      while (!qChild_[i].up->isEmpty() &&
                             tmudfStates_[i] == READING_FROM_CHILD)
                        {
                          ex_queue_entry * centry = qChild_[i].up->getHeadEntry();
                          ex_queue::up_status child_status = centry->upState.status;

                          qChild_[i].up->removeHead();
                          if (child_status == ex_queue::Q_NO_DATA)
                            {
                              tmudfStates_[i] = INITIAL;
                              down_pstate.numEodsFromChildTcbs_++;
                            }
                        }
                    }

                if (down_pstate.numEodsFromChildTcbs_ == numChildren())
                  // finish up error with an EOD
                  insertUpQueueEntry(ex_queue::Q_NO_DATA);
                else
                  return WORK_OK;
              }
	      break;
          
	    case STARTED:
	    case CANCEL_AFTER_SEND:
	      {
		UdrDataBuffer *repBuf;
		UdrDebug0("  [TMUDF WORK checkreceive] about to call getReplyBuffer()");
		repBuf = getReplyBuffer();
                
		if (repBuf && replyBufferIsEmpty() &&
		    !(down_pstate.numEodsFromChildTcbs_ == 
		      myTdb().numChildren()))
		  {
		    //check if this is a reply to indicate that we can read 
		    // more from child tcbs. 
		    // if TRUE
		    //  {
		    //    setUdrTcbState(READ_TABLE_INPUT_FROM_CHILD);
		    //    releaseReplyBuffer();
		    //   
		    //    done = TRUE;
		    //  }
		    if (repBuf->sendMoreData())
		      {
			if ( down_pstate.numEodsFromChildTcbs_ ==1)
		    
			  {
			    UdrDebug0(" !!!! ERROR EOD sent already 1 !!!!");
			   
			  }
			//see if udrserver has specified any table index
			if(repBuf->tableIndex() != -1)
			  {
			    down_pstate.currentChildTcbIndex_ = 
			      repBuf->tableIndex();
			    // change state_ from WORK_IO_ACTIVE to 
			    setUdrTcbState(READ_TABLE_INPUT_FROM_CHILD);
			  }
			else
			  {
			    ex_assert(FALSE,"Udrserver has not specified a valid table index");
			  }
			releaseReplyBuffer();				  
			done = TRUE;
		      }
		  }
		else if (repBuf)
		  {
		   
                  		    
		    // Now we can return a single up queue entry. If the up
		    // queue entry is Q_NO_DATA then inside this call to
		    // returnSingleRow() the head of the down queue will be
		    // popped.
		    
		    // the state will remain WORK_IO_ACTIVE at this point
		    // this ensures that we don't send more data yet but will
		    // call continueRequest instead.
                    
		    result = returnSingleRow();
		    if (replyBufferIsEmpty())
		      { 
			//check if this is a reply to indicate that we can read 
			// more from child tcbs. 
			// if TRUE
			//  {
			//    setUdrTcbState(READ_TABLE_INPUT_FROM_CHILD);
			//    releaseReplyBuffer();
			//   
			//    done = TRUE;
			//  }
			if (repBuf->sendMoreData() &&
			    !(down_pstate.numEodsFromChildTcbs_ == 
			      myTdb().numChildren()) )
			  {
		  		    			     
		  	   //see if udrserver has specified any table index
			    if(repBuf->tableIndex() != -1)
			      {
				down_pstate.currentChildTcbIndex_ = 
				  repBuf->tableIndex();
				// change state_ from WORK_IO_ACTIVE to 
				setUdrTcbState(READ_TABLE_INPUT_FROM_CHILD);
			      }
			    else
			      {
				ex_assert(FALSE,"Udrserver has sepcified invalid table index");
			      }			   
			    done = TRUE;
			    releaseReplyBuffer();
			    
			  }
			else
			  {
			  
			    // It is possible that we are in the WORK_IO_ACTIVE
			    // state and conditions to transition back to WORK have
			    // now been met. For example we might have just finished
			    // processing all down queue entries that have been sent
			    // to the server. The following method checks the
			    // conditions and transitions to WORK if
			    // appropriate. See commentary in the method for more
			    // detail.			  
			
			    releaseReplyBuffer();
			   
			    
			    if (dataStream_->numOfInputBuffers() > 0 )
			      {
				done = FALSE;
				UdrDebug0("Received next reply buffer while processing one  stay in this method");
			      } 
			    attemptTransitionToWorkState();
			      
			    
			  }//else 
		      }// if replyBufferIsEmpty
		  }// else if repBuf
	      
		else
		  {
                    // check for errors from getReplyBuffer()
                    if (getStatementDiags() &&
                        getStatementDiags()->mainSQLCODE() < 0)
                      {
                        insertUpQueueEntry(ex_queue::Q_SQLERROR, getStatementDiags());
                        down_pstate.step_ = PRODUCE_EOD_AFTER_ERROR;
                      }
                    else
                      done = TRUE;
		  }
	      }
	      break;
        
	    case NOT_STARTED:
	    default:
	      ex_assert(FALSE, "Invalid step value in TCB down queue");
	      break;
          
	    } // switch (down_pstate.step_)
      
	} // if (getIpcBroken()) else
    } // while (!done && result == WORK_OK && outstanding requests ... )
  // if there another reply buffer already in the data stream schedule this 
  // again
  
    return result;
}
ExWorkProcRetcode ExUdrTcb::tmudfCheckSend()
{
  return buildAndSendTmudfInput();
}


ExWorkProcRetcode ExUdrTcb::buildAndSendTmudfInput()
{
UdrDebug1("  [WORK] BEGIN ExUdrTcb::buildAndSendTmudfInput() 0x%08x",
            this);

  ExWorkProcRetcode result = WORK_OK;
  NABoolean done = FALSE;
  ULng32 numErrors = 0;
  ULng32 numCancels = 0;
  NABoolean sendSpaceAvailable =TRUE; 

  // Determine the transid we should send in data messages. We
  // use the statement transid if all the following are true
  // a. the TDB says we require a transaction
  // b. this is NOT a CALL that returns result sets
  // c. this is NOT a result set
  // 
  // Note: (b) and (c) are both indicated by rsInfo_ == NULL
  // 
  Int64 &stmtTransId = myExeStmtGlobals()->getTransid();
  Int64 transIdToSend = -1; // -1 is an invalid transid
  if (dataRequestsAreTransactional())
    transIdToSend = stmtTransId;
  
  // Process entries from the down queue. nextToSend_
  // will be incremented inside the loop following successful 
  // insertion of a request row into the request buffer.
  // This method will handle a sequence of actions until
  // we reach the WORK_IO_ACTIVE state:
  // 
  // WORK:
  //    - if a down queue entry exists, insert it into request buffer
  //    - state_ ==> SCALAR_INPUT_READY_TO_SEND
  //    - other possible subsequent states: WORK (for a cancel request)
  // SCALAR_INPUT_READY_TO_SEND:
  //    - send a message to the UDR server
  //    - state ==> WORK_IO_ACTIVE
  // ... 
  // READ_TABLE_INPUT_FROM_CHILD:
  //    - send a down queue request to child, if not done already
  //    - make sure we have an allocated buffer to store data
  //      returned from the child, to send to udrserv
  //    - state ==> RETURN_ROWS_FROM_CHILD
  // RETURN_ROWS_FROM_CHILD:
  //    - If we have some rows and child up queue is empty,
  //      send the buffer to udrserv
  //    - Otherwise, try to read more rows
  //    - state_ ==> CHILD_INPUT_READY_TO_SEND
  // CHILD_INPUT_READY_TO_SEND:
  //    - send message to udrserv
  //    - state_ ==> WORK_IO_ACTIVE
  
  while (result == WORK_OK  &&
         !done &&
         state_ != WORK_IO_ACTIVE &&
         (state_ != WORK || qParent_.down->entryExists(nextToSend_)))
  {
    // Two early tests to see if we should get out of the outer while
    // loop. First make sure a server is running then attempt to
    // allocate a request buffer.
    //
    // 1. Make sure we have a data stream and a running server
    if (dataStream_ == NULL || !verifyUdrServerProcessId())
    {
      result = WORK_BAD_ERROR;
      break;
    }
   
    queue_index parentQueueIndex;

    if (state_ == WORK)
      {
        // point to the next entry to be sent
        parentQueueIndex = nextToSend_;
      }
    else
      {
        // point to the head entry we are already working on
        parentQueueIndex = qParent_.down->getHeadIndex();
      }

    ex_queue_entry *downEntry = qParent_.down->getQueueEntry(parentQueueIndex);
    const ex_queue::down_request request = downEntry->downState.request;
    const Lng32 value = downEntry->downState.requestValue;
    ExUdrPrivateState &pstate = *((ExUdrPrivateState *) downEntry->pstate);

    switch (state_)
      {
      case LOAD_FAILED:
        result = WORK_BAD_ERROR;
        break;

      case WORK :
	{
          //  Allocate a request buffer on the message stream heap. Make
	  // sure stream buffer limits have not been reached. Break out of
	  // the outer while loop if the allocation fails.
	  if (requestBuffer_ == NULL)
	    {
	      // Free up any receive buffers no longer in use
	      dataStream_->cleanupBuffers();

	      // Now test the stream buffer limits. If the send queue is full
	      // we return WORK_OK and let the operator wake up again when I/O
	      // completes. If other limits have been reached we return
	      // WORK_POOL_BLOCKED which causes the operator to be scheduled
	      // again right away.
	      if (dataStream_->sendLimitReached())
		{
		  UdrDebug0("  [TMUDF WORK] Cannot allocate request buffer");
		  UdrDebug0("         sendLimitReached() returned TRUE");
		  printDataStreamState();
		  result = WORK_OK;
		}
	      else if (dataStream_->inUseLimitReached())
		{
		  UdrDebug0("  [TMUDF WORK] Cannot allocate request buffer");
		  UdrDebug0("         inUseLimitReached() returned TRUE");
		  printDataStreamState();
		  result = WORK_POOL_BLOCKED;
		}
	      else
		{
		  requestBuffer_ = getRequestBuffer();
		  if (requestBuffer_ == NULL)
		    {
		      result = WORK_POOL_BLOCKED;
		      printDataStreamState();
		    }
		}
	    } // if (requestBuffer_ == NULL)

	  if (requestBuffer_ == NULL)
	    break;
	  // get ready to put scalar values from down request into the request
	  // buffer
	
	  NABoolean anyErrors = FALSE;
	  
	  SqlBuffer *sqlBuf = requestBuffer_->getSqlBuffer();
	  Lng32 numTuppsBefore = sqlBuf->getTotalTuppDescs();
	 
	  ex_assert(sqlBuf, "UDR request buffer is corrupt or contains no data");
	  ULng32 numRequestsAdded = 0;
	  switch (request)
	    {
	    case ex_queue::GET_N:
	    case ex_queue::GET_ALL:
	      {
		// First step is to allocate space in the SqlBuffer. After
		// that a move expression will be evaluated to copy data into
		// the buffer. If the expression results in errors then we need
		// to back-out the buffer allocation, which may have been a
		// single data tupp or possibly control + data. So that the
		// correct number are backed out, we will remember how many 
		// tupps are in the buffer before the allocation takes place.

		tupp_descriptor *scalarDesc;
		if (sqlBuf->moveInSendOrReplyData(
		   TRUE,                        // [IN] sending? (vs. replying)
		   FALSE,                       // [IN] force move of control info?
		   TRUE,                        // [IN] move data?
		   &(downEntry->downState),     // [IN] queue state
		   sizeof(ControlInfo),         // [IN] length of ControlInfo area
		   NULL,                        // [OUT] new ControlInfo area
		   myTdb().getRequestRowLen(),  // [IN] data row length
		   &scalarDesc,                   // [OUT] new data tupp_descriptor
		   NULL,                        // [IN] diags area
		   NULL                         // [OUT] new diags tupp_descriptor
						  ) == SqlBuffer::BUFFER_FULL)
		  {
		    // No more space in request buffer. The assertion here says that
		    // we have at least inserted one row into the message buffer.
		    ex_assert(numRequestsAdded > 0,
			      "Request buffer not large enough to hold a single row");
		    sendSpaceAvailable = FALSE;
		  }
		else
		  {
		    ex_expr::exp_return_type expStatus = ex_expr::EXPR_OK;
		    if (myTdb().getInputExpression())
		      {
			workAtp_->getTupp(myTdb().getRequestTuppIndex()) = scalarDesc;

			// Evaluate the input expression. If diags are generated they
			// will be left in the down entry ATP to be returned to the parent.
			expStatus = 
			  myTdb().getInputExpression()->eval(downEntry->getAtp(),
							     workAtp_);
#ifdef UDR_DEBUG
			if (expStatus != ex_expr::EXPR_OK)
			  UdrDebug1("  [TMUDF WORK] scalar inputExpr.eval() returned %s",
				    GetExpStatusString(expStatus));
#endif
			workAtp_->getTupp(myTdb().getRequestTuppIndex()).release();
              
			if (expStatus == ex_expr::EXPR_ERROR)
			  {
			    while (sqlBuf->getTotalTuppDescs() > numTuppsBefore)
			      {
				sqlBuf->remove_tuple_desc();
			      }
                            if (insertUpQueueEntry(ex_queue::Q_SQLERROR))
                              {
                                pstate.step_ = ExUdrTcb::PRODUCE_EOD_AFTER_ERROR;
                                numErrors++;
                                nextToSend_++;
                              }
			  }

		      } // if (myTdb().getInputExpression())

		    if (expStatus != ex_expr::EXPR_ERROR)
		      {
			pstate.step_ = ExUdrTcb::STARTED;
			numRequestsAdded++;
                        // Note: this will prevent us from adding
                        // a second request from the parent down queue,
                        // so we process those one at a time for now
			setUdrTcbState(SCALAR_INPUT_READY_TO_SEND);
                        nextToSend_++;
		      }		    
		  } // if (no space in the request buffer) else
          
	      } // case GET_N, GET_ALL
	      break;
        
	    case ex_queue::GET_NOMORE:
	      {
		pstate.step_ = ExUdrTcb::CANCEL_BEFORE_SEND;
		numCancels++;
		nextToSend_++;
	      }
	      break;
        
	    case ex_queue::GET_EMPTY:
	    case ex_queue::GET_EOD:
	    default:
	      {
		ex_assert(FALSE, "Invalid request type in TCB down queue");
	       
	      }
	      break;
	   
	    } // switch (request)
	}  // case work
	break;
	case SCALAR_INPUT_READY_TO_SEND:
	  {
	    SqlBuffer *sqlBuf = requestBuffer_->getSqlBuffer();
	    if (sqlBuf->getTotalTuppDescs() > 0)
	      {

#ifdef UDR_DEBUG
		// Print a message if tracing is enabled or the
		// UDR_CONTINUE_DEBUG environment variable is set
		if (doTrace_ ||
		    doIpcTrace_ ||
		    (getenv("UDR_CONTINUE_DEBUG") != NULL))
		  {
		    char stmtTx[256];
		    char msgTx[256];
		    TransIdToText(stmtTransId, stmtTx, 256);
		    TransIdToText(transIdToSend, msgTx, 256);
		    UdrPrintf(traceFile_ ? traceFile_ : stdout,
			      "  [TMUDF WORK] Sending %s, %d tupps, msg tx %s",
			      "INVOKE",
			      (Lng32) sqlBuf->getTotalTuppDescs(), msgTx);
		  }
#endif
      
		// set the scalar flag to indicate we're sending a scalar input
		requestBuffer_->setSendingScalarValues(TRUE);
		sqlBuf->drivePack();
		setUdrTcbState(WORK_IO_ACTIVE);

		if (getStatsEntry() && getStatsEntry()->castToExUDRStats())
		  {
		    getStatsEntry()->castToExUDRStats()->
		      bufferStats()->sentBuffers().addEntry(sizeof(*requestBuffer_) +
							    sqlBuf->get_used_size());
		    getStatsEntry()->castToExUDRStats()->
		      bufferStats()->totalSentBytes() += sqlBuf->getSendBufferSize();
		  }
		

		if (dataRequestsAreTransactional())
		  myExeStmtGlobals()->incrementUdrTxMsgsOut();
		else
		  myExeStmtGlobals()->incrementUdrNonTxMsgsOut();

		dataStream_->sendRequest(transIdToSend);
                dataMsgsSent_++;

		UdrDebug0("  [WORK] The data stream has returned control to the TCB");

		// Now that sendRequest() has completed the request buffer is in
		// packed form and owned by the stream. This TCB should not
		// access the request buffer again so we release it now.
		releaseRequestBuffer();

		UdrDebug0("  [WORK] Done sending the scalarrequest buffer");

	      } // if (sqlBuf->getTotalTuppDescs() > 0)
	    
	   
	  }
	  break;


      case READ_TABLE_INPUT_FROM_CHILD :
	{
	  // read rows from child specified in the reply from server
	  // if nothing returned from any child. Get outta here.
	  Int32 currChild = pstate.currentChildTcbIndex_;
          UdrDebug1("TMUDF READING FROM CHILD  %lu", currChild);

	  // If this table's buffer is empty, allocate it on the stream
	  if (childInputBuffers_[currChild] == NULL)
	    {
	      // Free up any receive buffers no longer in use
	      dataStream_->cleanupBuffers();

	      // Now test the stream buffer limits. If the send queue is full
	      // we return WORK_OK and let the operator wake up again when I/O
	      // completes. If other limits have been reached we return
	      // WORK_POOL_BLOCKED which causes the operator to be scheduled
	      // again right away.
	      if (dataStream_->sendLimitReached())
		{
		  UdrDebug0("  [TMUDF WORK] Cannot allocate request buffer");
		  UdrDebug0("         sendLimitReached() returned TRUE");
		  printDataStreamState();
		  return WORK_OK;
		}
	      else if (dataStream_->inUseLimitReached())
		{
		  UdrDebug0("  [TMUDF WORK] Cannot allocate request buffer");
		  UdrDebug0("         inUseLimitReached() returned TRUE");
		  printDataStreamState();
		  return WORK_POOL_BLOCKED;
		}
	      else
		{
		  childInputBuffers_[currChild] = getRequestBuffer();
		  if (childInputBuffers_[currChild] == NULL)
		    {
		      return WORK_POOL_BLOCKED;
		      printDataStreamState();
		    }
		}
	    }
	   	 		
	  // pass the parent request to the child downqueue, if not already done
          if (tmudfStates_[currChild] != READING_FROM_CHILD)
            if (!qChild_[currChild].down->isFull())
              {
                ex_queue_entry * centry = qChild_[currChild].down->getTailEntry();

                if (request == ex_queue::GET_N)
                  centry->downState.request = ex_queue::GET_ALL;
                else           
                  centry->downState.request = request;

                centry->downState.requestValue = downEntry->downState.requestValue;
                centry->downState.parentIndex = parentQueueIndex;
                // set the child's input atp
                centry->passAtp(downEntry->getAtp());
                qChild_[currChild].down->insert();

                tmudfStates_[currChild] = READING_FROM_CHILD;
              }
            else
              // couldn't pass request to child, return
              return WORK_OK;

          // we are now ready to read (more) rows from this child
          setUdrTcbState(RETURN_ROWS_FROM_CHILD);
	}// case READ_TABLE_INPUT_FROM_CHILD
	break;
	case RETURN_ROWS_FROM_CHILD :
	  {
	    // Each child will fill it's own dedicated buffer
	  
	    // if we fill up a buffer, send it 
	    // and get out of this while loop
	    // OR if we reach EOD on one particular table input, just send 
	    // whatever we have in that buffer. This is because each tables 
	    // input should reach in it's own buffer
	    Int32 currChild = pstate.currentChildTcbIndex_;
	    SqlBuffer *childSqlBuf = childInputBuffers_[currChild]->getSqlBuffer();
	    ex_assert(childSqlBuf, "UDR childinput buffer is missing");

   
	    if ( (qChild_[currChild].up->isEmpty()))
	      {
		// if we have a partially filled buffer send it to the udrserver 
		if (childInputBuffers_[currChild])
		  {
		    if (childInputBuffers_[currChild]->getSqlBuffer()->
			getTotalTuppDescs() > 0)
		      {		  
			childInputBuffers_[currChild]->setTableIndex(currChild);
			setUdrTcbState(CHILD_INPUT_READY_TO_SEND);
			break;
		      }
		  }
		// if the buffer is empty we can't send anything
		// go back to the scheduler
		
		return WORK_OK;
	      }
	    
	     
	      
	    ex_queue_entry * centry = qChild_[currChild].up->getHeadEntry();
	    ex_queue::up_status child_status = centry->upState.status;

	    switch(child_status)
	      {
	      case ex_queue::Q_OK_MMORE:
		{ 
		  // First step is to allocate space in the SqlBuffer. 
		  // After that a move expression will be evaluated to
		  // copy data into the buffer. If the expression results
		  // in errors then we need to back-out the buffer 
		  // allocation, which may have been a single data tupp 
		  // or possibly control + data. So that the correct 
		  // number are backed out, we will remember how many 
		  // tupps are in the buffer before the allocation takes 
		  // place.
		  Lng32 numTuppsBefore = childSqlBuf->getTotalTuppDescs();
		  UdrDebug0(" Received OK_MMORE from child");
		  tupp_descriptor *dataDesc=NULL;
                  NABoolean processedUpEntry = TRUE;

		  if (childSqlBuf->moveInSendOrReplyData(
		      TRUE,     // [IN] sending? (vs. replying)
		      FALSE,    // [IN] force move of control info?
		      TRUE,           // [IN] move data?
		      &(centry->upState),     // [IN] queue state
		      sizeof(ControlInfo), // [IN] length of ControlInfo area
		      NULL,                // [OUT] new ControlInfo area
		      myTdb().getTableDescInfo(currChild)->getRowLength(),  // [IN] data row length
		      &dataDesc,      // [OUT] new data tupp_descriptor
		      NULL,           // [IN] diags area
		      NULL            // [OUT] new diags tupp_descriptor
							 ) 
		      == SqlBuffer::BUFFER_FULL)
		    {
		      // No more space in request buffer. The assertion here 
		      //says that we have at least inserted one row into 
		      // the message buffer.
		      ex_assert(childSqlBuf->getTotalTuppDescs()
				> 0,
				"Request buffer not large enough to hold a single row");
		      UdrDebug0("No more space in send buffer");
		      sendSpaceAvailable = FALSE;
                      processedUpEntry = FALSE;
		    }
		  else
		    {
		      ex_expr::exp_return_type expStatus = ex_expr::EXPR_OK;
		      if (myTdb().getChildInputExpr(currChild))
			{
			
			  UInt32 childTuppIndex = 
			    myTdb().getTableDescInfo(currChild)->
			    getOutputTuppIndex();

			  workAtp_->getTupp(childTuppIndex) 
			    = dataDesc;

			  // Evaluate the input expression.
			  expStatus = 
			    myTdb().getChildInputExpr(currChild)->
			    eval(centry->getAtp(),workAtp_);
#ifdef UDR_DEBUG
			  if (expStatus != ex_expr::EXPR_OK)
			    UdrDebug1("  [TMUDF WORK] childInputExpr.eval() returned %s",
				      GetExpStatusString(expStatus));
#endif
			  // SS TBD change to getChildTuppIndex
			  workAtp_->getTupp(childTuppIndex).release();
              
			  if (expStatus == ex_expr::EXPR_ERROR)
			    {
			      while (childSqlBuf->
				     getTotalTuppDescs() > numTuppsBefore)
				{
				  childSqlBuf->remove_tuple_desc();
				}
                              if (insertUpQueueEntry(ex_queue::Q_SQLERROR,
                                                     centry->getDiagsArea()))
                                {
                                  pstate.step_ = ExUdrTcb::PRODUCE_EOD_AFTER_ERROR;
                                  numErrors++;
                                }
                              else
                                {
                                  processedUpEntry = FALSE;
                                }
			    }

			} // if (myTdb().getChildInputExpression())

		      if (expStatus != ex_expr::EXPR_ERROR)          
			pstate.step_ = ExUdrTcb::STARTED;
             
            	
		    }//else

		  if (sendSpaceAvailable && processedUpEntry)
                    {
                      qChild_[currChild].up->removeHead();
                    }
                  else if (!sendSpaceAvailable)
		    {
		   
		      // set the tableindex in the header
		      UdrDebug1("Ready to send  child input buffer, numtupps = %d", childSqlBuf->getTotalTuppDescs());
		      childInputBuffers_[currChild]->setTableIndex(currChild);
		      setUdrTcbState(CHILD_INPUT_READY_TO_SEND);
		    }
		}
		break;



	      case ex_queue::Q_NO_DATA:
		{
	       
		  // indicate that this is the last buffer for this table
		  childInputBuffers_[currChild]->setLastBuffer(TRUE);
		  UdrDebug1("EOD reached for child input %u", currChild);
		  //set the tableindex in the buffer.		   
		  childInputBuffers_[currChild]->setTableIndex(currChild);
		  pstate.numEodsFromChildTcbs_++;
		  UdrDebug1(" NUm Child EOD's so far = %u",
			    pstate.numEodsFromChildTcbs_);
                  qChild_[currChild].up->removeHead();
                  tmudfStates_[currChild] = INITIAL;
		  setUdrTcbState(CHILD_INPUT_READY_TO_SEND);
		}
		break;
	      case ex_queue::Q_SQLERROR:
		{
                  // pass the diags are on to the parent if there is space in
                  // the up queue
                  if (insertUpQueueEntry(ex_queue::Q_SQLERROR,
                                         centry->getDiagsArea()))
                    {
                      pstate.step_ = ExUdrTcb::PRODUCE_EOD_AFTER_ERROR;
                      qChild_[currChild].up->removeHead();
                      numErrors++;
                    }
                  else
                    {
                      // come back later, when the up queue has room
                      done = TRUE;
                    }
		}
		break;
	      case ex_queue::Q_INVALID:
		ex_assert(0,"ExUdrTcb::tmdudfWork() Invalid state returned by child");
		break;
		
	      } // switch
	  }
	  break;




      case CHILD_INPUT_READY_TO_SEND:
	{
	  Int32 currChild = pstate.currentChildTcbIndex_;
	  //send request in buffer
	  SqlBuffer *sqlBuf = childInputBuffers_[currChild]->getSqlBuffer();
	  if (sqlBuf->getTotalTuppDescs() > 0)
	    {
#ifdef UDR_DEBUG
	      // Print a message if tracing is enabled or the
	      // UDR_CONTINUE_DEBUG environment variable is set
	      if (doTrace_ ||
		  doIpcTrace_ ||
		  (getenv("UDR_CONTINUE_DEBUG") != NULL))
		{
		  char stmtTx[256];
		  char msgTx[256];
		  TransIdToText(stmtTransId, stmtTx, 256);
		  TransIdToText(transIdToSend, msgTx, 256);
		  UdrPrintf(traceFile_ ? traceFile_ : stdout,
			    "  [TMUDF WORK] Sending %s, %d tupps, msg tx %s",
			     "CHILD TABLE INPUT",
			    (Lng32) sqlBuf->getTotalTuppDescs(), msgTx);
		}
#endif
      
	      sqlBuf->drivePack();	     

	      if (getStatsEntry() && getStatsEntry()->castToExUDRStats())
		{
		  getStatsEntry()->castToExUDRStats()->
		    bufferStats()->sentBuffers().
		    addEntry(sizeof(*requestBuffer_) +
			     sqlBuf->get_used_size());
		  getStatsEntry()->castToExUDRStats()->
		    bufferStats()->totalSentBytes() +=
		    sqlBuf->getSendBufferSize();
		}
	      if (dataRequestsAreTransactional())
		myExeStmtGlobals()->incrementUdrTxMsgsOut();
	      else
		myExeStmtGlobals()->incrementUdrNonTxMsgsOut();

	      dataStream_->sendRequest(transIdToSend);
              dataMsgsSent_++;

	      UdrDebug0("  [TMUDF WORK] The data stream has returned control to the TCB");


	      UdrDebug1("  [TMUDF WORK] Done sending the request buffer for child %u", currChild);

	    } // if (sqlBuf->getTotalTuppDescs() > 0)
	      // Now that sendRequest() has completed the request buffer is in
	      // packed form and owned by the stream. This TCB should not
	      // access the request buffer again so we release it now.
	      releaseChildInputBuffer(currChild);

	  setUdrTcbState(WORK_IO_ACTIVE);
	}
	break;


      default :
	{
          // other states cause us to return back to the caller
          done = TRUE;
	}
	  
	break;

      }// switch(state_)
  } // while 
 
 if (result == WORK_OK)
  {
    if (getIpcBroken())
    {
      result = WORK_BAD_ERROR;
    }
  }

  //
  // If errors were encountered then there may be down queue
  // entries that were not sent but still need to be processed.
  // Changing the return code to WORK_CALL_AGAIN makes sure the
  // scheduler redrives this TCB.
  //
  if (result == WORK_OK && (numErrors > 0 || numCancels > 0))
  {
    result = WORK_CALL_AGAIN;
  }
return result;
}

#ifdef UDR_DEBUG
void ExUdrTcb::initializeDebugVariables()
{
  // First close the old trace file. Not sure if it is safe to call
  // fclose on stdout or stderr so we will guard against that.
  if (traceFile_ && (traceFile_ != stdout) && (traceFile_ != stderr))
  {
    fclose(traceFile_);
  }

  doTrace_ = (getenv("UDR_""DEBUG") != NULL);
  doStateTrace_ = (doTrace_ || getenv("UDR_STATE_DEBUG"));
  doIpcTrace_ = (doTrace_ || getenv("UDR_IPC_DEBUG"));
  traceFile_ = NULL;

  // Now see if the name of a trace file is specified in the
  // environment. If not the trace will go to stdout.
  if (doTrace_ || doStateTrace_ || doIpcTrace_)
  {
    FILE *newTraceFile = NULL;
    const char *traceFileName = getenv("UDR_""DEBUG_FILE");
    if (traceFileName && traceFileName[0])
    {
      newTraceFile = fopen(traceFileName, "a");
    }
    traceFile_ = (newTraceFile ? newTraceFile : stdout);
  }

  // ESP debugging . uncomment the following block of code
  /*
  // begin esp debugging
  doTrace_ = TRUE;
  long numInstances = 0;
  long instanceNum =0;
  myExeStmtGlobals()->getMyNodeLocalInstanceNumber(instanceNum,numInstances);
 
  char esptraceFileName[30] = "";
  
  
  char *logdir = "C:/temp"; //change if needed 
  sprintf(esptraceFileName, "%s/espudrtrace.log%d",logdir,instanceNum);
  traceFile_ = fopen(esptraceFileName,"a");
  */
  // end esp debugging
  

  // Integrity checks in incoming reply messages can be disabled in
  // the debug build by an environment setting.
  trustReplies_ = (getenv("UDR_TRUST_REPLIES") ? TRUE : FALSE);
}

void ExUdrTcb::printDataStreamState()
{
  if (dataStream_ == NULL)
  {
    UdrDebug0("    Data stream is NULL");
  }
  else
  {
    UdrDebug6("    Data stream: "
              "inUse %d, in %d, out %d, send %d, replyTag %d, pending %d",
              (Int32) dataStream_->numOfInUseBuffers(),
              (Int32) dataStream_->numOfInputBuffers(),
              (Int32) dataStream_->numOfOutputBuffers(),
              (Int32) dataStream_->numOfSendBuffers(),
              (Int32) dataStream_->numOfReplyTagBuffers(),
              (Int32) dataStream_->numOfResponsesPending());
  }
}
 
#endif

void ExUdrTcb::addIntegrityCheckFailureToDiagsArea(ComDiagsArea *diags) const
{
  ex_assert(diags, "Should not call this function with a NULL diags pointer");
  if (!ProcessIdIsNull(serverProcessId_))
  {
    *diags << DgSqlCode(-2037);
    myIpcEnv()->getMyOwnProcessId(IPC_DOM_GUA_PHANDLE).
      addProcIdToDiagsArea(*diags, 0);
    serverProcessId_.addProcIdToDiagsArea(*diags, 1);
  }
  *diags << DgSqlCode(-EXE_UDR_INVALID_OR_CORRUPT_REPLY);
}

void ExUdrTcb::tmudfCancelChildRequests(queue_index parentIndex)
{
  for (int i=0; i<numChildren(); i++)
    {
      if (tmudfStates_[i] == READING_FROM_CHILD)
        qChild_[i].down->cancelRequestWithParentIndex(parentIndex);
    }
}

NABoolean ExUdrTcb::validateDataRow(const tupp &replyTupp,
                                    ComDiagsArea *&diags)
{
  const ExUdrTdb &tdb = myTdb();
  ComUInt32 numOutParams = tdb.getNumOutputValues();

  if (numOutParams == 0)
    return TRUE;

  NABoolean result = TRUE;
  ComUInt32 numParams = tdb.getNumParams();
  ComUInt32 numInParams = tdb.getNumInputValues();

  char *dataPtr = replyTupp.getDataPointer();
  ComUInt32 dataSize = replyTupp.getAllocatedSize();

  // We walk the parameter array in one of two ways:
  // * if this is a stored procedure, look at every element
  // * otherwise jump past the input parameters

  NABoolean isProcedure =
    (tdb.getUdrType() == COM_PROCEDURE_TYPE ? TRUE : FALSE);

  ComUInt32 outPosition = 0;
  ComUInt32 startIndex = 0;
  if (!isProcedure)
    startIndex = numInParams;
  
  for (ComUInt32 i = startIndex; result && i < numParams; i++)
  {
    const UdrFormalParamInfo &formal = *tdb.getFormalParamInfo(i);
    
    if (formal.isOut())
    {
      Attributes &actual = *(tdb.getReplyAttr(outPosition));
      ComSInt16 fsType = formal.getType();
      ComSInt16 nullValue = 0;

      // Check for a valid null indicator. Null indicators returned
      // from MXUDR are 2 bytes. Valid null indicator values are 0 and
      // -1.
      if (result && actual.getNullFlag())
      {
        const char *nullIndPtr = dataPtr + actual.getNullIndOffset();
        nullValue = *((ComSInt16 *) nullIndPtr);
        if (nullValue != 0 && nullValue != -1)
        {
          if (diags == NULL)
            diags = ComDiagsArea::allocate(getHeap());

          Lng32 pos = (Lng32) (isProcedure ? (i + 1) : (outPosition + 1));

          *diags << DgSqlCode(-EXE_UDR_INVALID_DATA)
                 << DgString0(myTdb().getSqlName())
                 << DgInt0(pos)
                 << DgString1("Invalid null indicator");

          result = FALSE;
        }
      }
      
      // do not check for varchar length when the value is null as the varchar length could be garbage/uninitialized
      if (nullValue != -1 && result && DFS2REC::isAnyVarChar(fsType))
      {
        // Check for valid VARCHAR length
        const char *vcIndPtr = dataPtr + actual.getVCLenIndOffset();
        ComUInt32 vcLen = actual.getLength((char *) vcIndPtr);
        if (vcLen > (ComUInt32) actual.getLength())
        {
          if (diags == NULL)
            diags = ComDiagsArea::allocate(getHeap());

          char msg[100];
          str_sprintf(msg, "VARCHAR length should not exceed %d",
                      (Int32) actual.getLength());

          Lng32 pos = (Lng32) (isProcedure ? (i + 1) : (outPosition + 1));

          *diags << DgSqlCode(-EXE_UDR_INVALID_DATA)
                 << DgString0(myTdb().getSqlName())
                 << DgInt0(pos)
                 << DgString1(msg);

          result = FALSE;
        }
      }

      if (result && DFS2REC::isDoubleCharacter(fsType))
      {
        // Check for valid UCS2 characters
        const char *source = dataPtr + actual.getOffset();
        ComUInt32 sourceLen = actual.getLength();
        if (DFS2REC::isAnyVarChar(fsType))
          sourceLen = actual.getLength(dataPtr + actual.getVCLenIndOffset());

        ComUInt32 sourceChars = sourceLen / 2;
        if (CharInfo::checkCodePoint((wchar_t *) source, (Int32) sourceChars,
                                     CharInfo::UNICODE) == FALSE)
        {
          if (diags == NULL)
            diags = ComDiagsArea::allocate(getHeap());

          Lng32 pos = (Lng32) (isProcedure ? (i + 1) : (outPosition + 1));

          *diags << DgSqlCode(-EXE_UDR_INVALID_DATA)
                 << DgString0(myTdb().getSqlName())
                 << DgInt0(pos)
                 << DgString1("Invalid UCS2 character");

          result = FALSE;
        }
      }
      
      // Advance the output parameter position
      outPosition++;
      
    } // if (formal.isOut())
  } // for each param
  
  return result;
}

NABoolean ExUdrTcb::serverResourcesAreLoaded() const
{
  NABoolean result = FALSE;
  if (state_ == WORK || state_ == WORK_IO_ACTIVE ||
      state_== READ_TABLE_INPUT_FROM_CHILD ||
      state_ == RETURN_ROWS_FROM_CHILD )
  {
    result = TRUE;
  }
  return result;
}

NABoolean ExUdrTcb::dataRequestsAreTransactional() const
{
  // Data requests are transactional if all the following are true
  // a. the TDB says we require a transaction
  // b. this is NOT a CALL that returns result sets
  // c. this is NOT a result set
  // 
  // Note: (b) and (c) are both indicated by rsInfo_ == NULL
  //
  NABoolean result = FALSE;
  if (myTdb().getTransactionAttrs() == COM_TRANSACTION_REQUIRED &&
      rsInfo_ == NULL)
    result = TRUE;
  return result;
}

//----------------------------------------------------------------------
// class ExUdrPrivateState
//----------------------------------------------------------------------
ExUdrPrivateState::ExUdrPrivateState() 
{
  init();
}

void ExUdrPrivateState::init() 
{
  step_ = ExUdrTcb::NOT_STARTED;
  matchCount_ = 0;
  numEodsFromChildTcbs_= 0;
  currentChildTcbIndex_ = -1; // no table input
}

ExUdrPrivateState::~ExUdrPrivateState()
{
}


