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
 * File:         ExUdrClientIpc.cpp
 * Description:  IPC streams and message objects for the client-side
 *               of a UDR server connection
 *               
 * Created:      08/20/2000
 * Language:     C++
 *
 *
 *****************************************************************************
 */

#include "ex_stdh.h"
#include "ExUdrClientIpc.h"
#include "ExUdr.h"
#include "ExUdrServer.h"
#include "ExpError.h"
#include "ex_exe_stmt_globals.h"
#include "ExRsInfo.h"

#ifdef UDR_DEBUG
#include "ErrorMessage.h"

#define UdrDebug0(s) \
  ( UdrPrintf(traceFile_,(s)) )
#define UdrDebug1(s,a1) \
  ( UdrPrintf(traceFile_,(s),(a1)) )
#define UdrDebug2(s,a1,a2) \
  ( UdrPrintf(traceFile_,(s),(a1),(a2)) )
#define UdrDebug3(s,a1,a2,a3) \
  ( UdrPrintf(traceFile_,(s),(a1),(a2),(a3)) )
#define UdrDebug4(s,a1,a2,a3,a4) \
  ( UdrPrintf(traceFile_,(s),(a1),(a2),(a3),(a4)) )
#define UdrDebug5(s,a1,a2,a3,a4,a5) \
  ( UdrPrintf(traceFile_,(s),(a1),(a2),(a3),(a4),(a5)) )

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

#endif // UDR_DEBUG

//
// Notes on the UDR IPC callback framework
//
// UDR control streams implement a PUSH model where the stream
// callback functions push info into the TCB by calling TCB methods
// and the TCB never retains pointers to the reply message objects.
//
// UDR data streams implement a PULL model where the stream callback
// functions do not push UDR output data into the TCB. Instead the
// stream simply notifies the TCB when output data arrives and later
// during a TCB work method the TCB will extract reply objects from
// the stream.
//

//---------------------------------------------------------------------------
// class UdrClientControlStream
//---------------------------------------------------------------------------
UdrClientControlStream::UdrClientControlStream(IpcEnvironment *env,
                                               ExUdrTcb *tcb,
                                               ExExeStmtGlobals *stmtGlobals,
                                               NABoolean keepUdrDiagsForCaller,
                                               NABoolean isTransactional)
  : UdrControlStream(env,
                     UDR_STREAM_CLIENT_CONTROL,
                     UdrClientControlStreamVersionNumber),
    tcb_(tcb), stmtGlobals_(stmtGlobals),
    exRsInfo_(NULL), messageType_(UDR_IPC_INVALID),
    keepUdrDiagsForCaller_(keepUdrDiagsForCaller), udrDiagsForCaller_(NULL),
    isTransactional_(isTransactional)
#ifdef UDR_DEBUG
    , traceFile_(NULL), trustReplies_(FALSE)
#endif
{
}

UdrClientControlStream::~UdrClientControlStream()
{
  UdrDebug1("  UdrClientControlStream destructor %p", this);
  UdrDebug2("    %u message%s sent",
            sendCount_, PLURAL_SUFFIX(sendCount_));
  UdrDebug2("    %u message%s received",
            recvCount_, PLURAL_SUFFIX(recvCount_));
  UdrDebug2("    tcb_ = %p, stmtGlobals_ %p", tcb_, stmtGlobals_);
  ex_assert(!tcb_,
            "UDR control stream going away before TCB called delinkTcb()");

  if (udrDiagsForCaller_)
    udrDiagsForCaller_->decrRefCount();

  // If the stream going away was for an ENTER, EXIT, or SUSPEND TX
  // message, make sure the ExRsInfo does not hold a pointer to this
  // stream
  if (exRsInfo_)
  {
    exRsInfo_->reportCompletion(this);
    exRsInfo_ = NULL;
  }
}

void UdrClientControlStream::delinkTcb(const ExUdrTcb *tcb)
{
  UdrDebug0("  UdrClientControlStream::delinkTcb()");
  UdrDebug2("    this %p, tcb_ %p", this, tcb_);
  ex_assert(tcb == tcb_, "The wrong TCB called delinkTcb()");
  if (tcb == tcb_)
  {
    tcb_ = NULL;
  }
}

//
// This function returns TRUE if the tcb_ pointer is valid
// and that TCB expects to be notified when a reply message
// arrives.
//
NABoolean UdrClientControlStream::tcbExpectsReply() const
{
  //
  // For now, as long as tcb_ is not NULL we will assume that
  // it expects a reply
  //
  NABoolean result = (tcb_ != NULL);
  return result;
}

//
// This function returns TRUE if the stmtGlobals_ pointer is valid and
// that object expects to be notified when replies arrive.
//
NABoolean UdrClientControlStream::stmtGlobalsExpectsReply() const
{
  //
  // For now, as long as stmtGlobals_ is not NULL we will assume that
  // it expects a reply
  //
  NABoolean result = (stmtGlobals_ != NULL);
  return result;
}

void UdrClientControlStream::actOnSend(IpcConnection *conn)
{
  sendCount_++;
  UdrDebug2("[BEGIN SEND CALLBACK %u] client control, this %p",
            sendCount_, this);
  UdrDebug2("   tcb_ %p, stmtGlobals_ %p", tcb_, stmtGlobals_);

  UdrDebug2("[END SEND CALLBACK %lu] client control, this 0x%08x",
            sendCount_, this);
  if (tcb_)
    tcb_->incReqMsg(conn->getLastSentMsg()->getMessageLength());
}

void UdrClientControlStream::actOnSendAllComplete()
{
  //
  // Once all sends have completed, initiate a nowait receive from all
  // servers that we sent this message to.
  //
  // IpcMessageStream semantics are such that a single send or receive
  // can be in progress at one time. To make sure the stream does not
  // get itself into an inconsistent state we call clearAllObjects()
  // before initiating either a send or receive.
  //
  clearAllObjects();
  receive(FALSE);  // no-waited
}

void UdrClientControlStream::actOnReceive(IpcConnection *conn)
{
  recvCount_++;
  UdrDebug3("[BEGIN RECV CALLBACK %u] client control, this %p, type %s",
            recvCount_, this, GetUdrIpcTypeString(messageType_));
  UdrDebug3("  tcb_ %p, stmtGlobals_ %p, exRsInfo_ %p",
            tcb_, stmtGlobals_, exRsInfo_);

  if (tcb_)
    tcb_->incReplyMsg(conn->getLastReceivedMsg()->getMessageLength());

  if (stmtGlobalsExpectsReply())
  {
    if (isTransactional_)
      stmtGlobals_->decrementUdrTxMsgsOut();
    else
      stmtGlobals_->decrementUdrNonTxMsgsOut();
  }
  
  if (exRsInfo_)
  {
    exRsInfo_->reportCompletion(this);
    exRsInfo_ = NULL;
  }

  // All memory allocations performed by this method will be done on
  // the IPC heap
  NAMemory *ipcHeap = environment_->getHeap();
  
  if (conn->getErrorInfo() != 0)
  {
    UdrDebug0("  *** A receive callback detected an IPC error");

    // Our job now is to report diagnostics to either the TCB or if
    // this stream is not attached to a TCB, then to the statement
    // globals. And if the stream happens to not even be attached to
    // the statement globals, then keep a copy of the diags if the
    // keepUdrDiagsForCaller_ flag is TRUE.
    if (tcbExpectsReply())
    {
      tcb_->reportIpcError(this, conn);
    }
    else if (stmtGlobalsExpectsReply())
    {
      if (stmtGlobals_->getUdrServer())
        // Flag the UdrServer with error
        stmtGlobals_->getUdrServer()->setState(ExUdrServer::EX_UDR_BROKEN);

      ComDiagsArea *d = stmtGlobals_->getDiagsArea();
      if (d && d->contains(-EXE_UDR_REPLY_ERROR))
      {
        // Do nothing
      }
      else
      {
        if (d == NULL)
        {
          d = ComDiagsArea::allocate(ipcHeap);
          stmtGlobals_->setGlobDiagsArea(d);
          d->decrRefCount();
        }
        if (conn)
          conn->populateDiagsArea(d, ipcHeap);
        *d << DgSqlCode(-EXE_UDR_REPLY_ERROR);
      }
    }
    else if (keepUdrDiagsForCaller_)
    {
      if (!udrDiagsForCaller_)
        udrDiagsForCaller_ = ComDiagsArea::allocate(ipcHeap);
      if (conn)
        conn->populateDiagsArea(udrDiagsForCaller_, ipcHeap);
      *udrDiagsForCaller_ << DgSqlCode(-EXE_UDR_REPLY_ERROR);
    }
    
    if (exRsInfo_ && (messageType_ == UDR_MSG_EXIT_TX ||
                      messageType_ == UDR_MSG_SUSPEND_TX))
    {
      UdrClientControlStream *s = exRsInfo_->getEnterTxStream();
      UdrDebug2("*** exRsInfo_ %p, ENTER TX stream %p", exRsInfo_, s);

      // If s is NULL we do nothing. This is possible if the ENTER
      // interaction already completed before the EXIT/SUSPEND
      // callback was issued.
      if (s)
      {
        UdrDebug0("*** About to abandon I/O on the ENTER TX stream");
        s->abandonPendingIOs();
      }
    }

  } // if (conn->getErrorInfo() != 0)

  else
  {
    UdrHandle udrHandle = INVALID_UDR_HANDLE;
    NABoolean loadReplyArrived = FALSE;
    NABoolean diagsAddedToStmt = FALSE;
    NABoolean doIntegrityChecks = TRUE;
    NABoolean integrityCheckFailed = FALSE;

#ifdef UDR_DEBUG
    doIntegrityChecks = !trustReplies_;
#endif

    //
    // The following loop iterates one time for each message object in
    // this reply.
    // 
    // Notes
    //
    //   For some message types no action is required. We simply
    //   remove the object from the stream and do nothing else. For
    //   example, the UdrSessionReply object (the reply to a session
    //   message in which we downloaded JVM startup options) contains
    //   no information and we don't do any special processing when
    //   that object arrives.
    //
    //   When the server has error conditions to report, the error
    //   information is in a ComDiagsArea object that arrives after
    //   the control message reply objects. For example, errors
    //   encounted during processing of a LOAD message get reported in
    //   the ComDiagsArea objects that follows the UdrLoadReply
    //   object. The UdrLoadReply object itself contains no error
    //   information.
    // 
    while (!integrityCheckFailed && moreObjects())
    {
      IpcMessageObjType t = getNextObjType();
      switch (t)
      {
        case UDR_MSG_SESSION_REPLY:
        {
          UdrDebug1("  UDR_MSG_SESSION_REPLY arrived on stream %p", this);
          UdrSessionReply *reply = new (ipcHeap) UdrSessionReply(ipcHeap);
          
          if (!extractNextObj(*reply, doIntegrityChecks))
          {
            UdrDebug0("*** ERROR: Integrity check failed for SESSION reply");
            integrityCheckFailed = TRUE;
          }
          
          reply->decrRefCount();
        }
        break;
        
        case UDR_MSG_LOAD_REPLY:
        {
          UdrDebug1("  UDR_MSG_LOAD_REPLY arrived on stream %p", this);
          UdrLoadReply *reply = new (ipcHeap) UdrLoadReply(ipcHeap);
          
          if (!extractNextObj(*reply, doIntegrityChecks))
          {
            UdrDebug0("*** ERROR: Integrity check failed for LOAD reply");
            integrityCheckFailed = TRUE;
          }
          else
          {
            udrHandle = reply->getHandle();
            UdrDebug1("    UDR handle is " INT64_PRINTF_SPEC, udrHandle);
            loadReplyArrived = TRUE;
          }

          reply->decrRefCount();
        }
        break;
        
        case UDR_MSG_RS_LOAD_REPLY:
        {
          UdrDebug1("  UDR_MSG_RS_LOAD_REPLY arrived on stream %p", this);
          UdrRSLoadReply *reply = new (ipcHeap) UdrRSLoadReply(ipcHeap);
          
          if (!extractNextObj(*reply, doIntegrityChecks))
          {
            UdrDebug0("*** ERROR: Integrity check failed for RS LOAD reply");
            integrityCheckFailed = TRUE;
          }
          else
          {
            udrHandle = reply->getHandle();
            UdrDebug1("    UDR handle is " INT64_PRINTF_SPEC, udrHandle);
            loadReplyArrived = TRUE;
          }

          reply->decrRefCount();
        }
        break;
        
        case UDR_MSG_UNLOAD_REPLY:
        {
          UdrDebug1("  UDR_MSG_UNLOAD_REPLY arrived on stream %p", this);
          UdrUnloadReply *reply = new (ipcHeap) UdrUnloadReply(ipcHeap);

          if (!extractNextObj(*reply, doIntegrityChecks))
          {
            UdrDebug0("*** ERROR: Integrity check failed for UNLOAD reply");
            integrityCheckFailed = TRUE;
          }

          reply->decrRefCount();
        }
        break;
        
        case UDR_MSG_RS_UNLOAD_REPLY:
        {
          UdrDebug1("  UDR_MSG_RS_UNLOAD_REPLY arrived on stream %p",
                    this);
          UdrRSUnloadReply *reply = new (ipcHeap) UdrRSUnloadReply(ipcHeap);

          if (!extractNextObj(*reply, doIntegrityChecks))
          {
            UdrDebug0("*** ERROR: Integrity check failed for RS UNLOAD reply");
            integrityCheckFailed = TRUE;
          }

          reply->decrRefCount();
        }
        break;
        
        case UDR_MSG_ENTER_TX_REPLY:
        {
          UdrDebug1("  UDR_MSG_ENTER_TX_REPLY arrived on stream %p", this);
          UdrEnterTxReply *reply = new (ipcHeap) UdrEnterTxReply(ipcHeap);

          if (!extractNextObj(*reply, doIntegrityChecks))
          {
            UdrDebug0("*** ERROR: Integrity check failed for ENTER TX reply");
            integrityCheckFailed = TRUE;
          }

          reply->decrRefCount();
        }
        break;
        
        case UDR_MSG_SUSPEND_TX_REPLY:
        {
          UdrDebug1("  UDR_MSG_SUSPEND_TX_REPLY arrived on stream %p",
                    this);
          UdrSuspendTxReply *reply = new (ipcHeap) UdrSuspendTxReply(ipcHeap);

          if (!extractNextObj(*reply, doIntegrityChecks))
          {
            UdrDebug0("*** ERROR: Integrity check failed for SUSPEND TX reply");
            integrityCheckFailed = TRUE;
          }

          reply->decrRefCount();
        }
        break;
        
        case UDR_MSG_EXIT_TX_REPLY:
        {
          UdrDebug1("  UDR_MSG_EXIT_TX_REPLY arrived on stream %p", this);
          UdrExitTxReply *reply = new (ipcHeap) UdrExitTxReply(ipcHeap);

          if (!extractNextObj(*reply, doIntegrityChecks))
          {
            UdrDebug0("*** ERROR: Integrity check failed for EXIT TX reply");
            integrityCheckFailed = TRUE;
          }

          reply->decrRefCount();
        }
        break;
        
        case UDR_MSG_RS_CLOSE_REPLY:
        {
          UdrDebug1("  UDR_MSG_RS_CLOSE_REPLY arrived on stream %p", this);
          UdrRSCloseReply *reply = new (ipcHeap) UdrRSCloseReply(ipcHeap);

          if (!extractNextObj(*reply, doIntegrityChecks))
          {
            UdrDebug0("*** ERROR: Integrity check failed for CLOSE reply");
            integrityCheckFailed = TRUE;
          }

          reply->decrRefCount();
        }
        break;
        
        case UDR_MSG_ERROR_REPLY:
        {
          UdrDebug1("  UDR_MSG_ERROR_REPLY arrived on stream %p", this);
          UdrErrorReply *reply = new (ipcHeap) UdrErrorReply(ipcHeap);

          if (!extractNextObj(*reply, doIntegrityChecks))
          {
            UdrDebug0("*** ERROR: Integrity check failed for ERROR reply");
            integrityCheckFailed = TRUE;
          }

          reply->decrRefCount();
        }
        break;
        
        case IPC_SQL_DIAG_AREA:
        {
          UdrDebug1("  IPC_SQL_DIAG_AREA arrived on stream %p", this);

          CollHeap *diagsHeap =
            (tcbExpectsReply() ? tcb_->getHeap() : ipcHeap);

          if (udrDiagsForCaller_)
          {
            // If we are currently holding any diags for our caller we
            // release them now. They will be replaced by the diags
            // that just arrived.
            udrDiagsForCaller_->decrRefCount();
          }
          udrDiagsForCaller_ = ComDiagsArea::allocate(diagsHeap);

          if (!extractNextObj(*udrDiagsForCaller_, doIntegrityChecks))
          {
            UdrDebug0("*** ERROR: Integrity check failed for ComDiagsArea");
            integrityCheckFailed = TRUE;
          }
          else
          {
            UdrDebug1("    Diags count is %d",
                      (Lng32) udrDiagsForCaller_->getNumber());
            
            if (tcbExpectsReply())
            {
              // If the TCB wants replies, give these diags to the TCB
              ComDiagsArea *mainDiags = tcb_->getStatementDiags();

              if (mainDiags)
                mainDiags->mergeAfter(*udrDiagsForCaller_);
              else
              {
                tcb_->setStatementDiags(udrDiagsForCaller_);
              }

              diagsAddedToStmt = TRUE;
            }
            else if (stmtGlobalsExpectsReply())
            {
              // Otherwise if the stmt globals wants replies, give
              // these diags to the stmt globals
              ComDiagsArea *mainDiags = stmtGlobals_->getDiagsArea();

              if (mainDiags)
                mainDiags->mergeAfter(*udrDiagsForCaller_);
              else
              {
                stmtGlobals_->setGlobDiagsArea(udrDiagsForCaller_);
              }

              diagsAddedToStmt = TRUE;
            }
            else if (!keepUdrDiagsForCaller_)
            {
              // Otherwise diags arrived but we have no place to put
              // them. That's not an error but in the debug build we
              // will print warning messages in case a coding mistake
              // has been made.
#ifdef UDR_DEBUG
              FILE *f = (traceFile_ ? traceFile_ : stdout);
              UdrPrintf(f, "***");
              UdrPrintf(f, "*** WARNING: UDR diags arrived on stream %p",
                        this);
              UdrPrintf(f, "***          but the stream is not currently in");
              UdrPrintf(f, "***          a state where it can process diags");
              UdrPrintf(f, "***");
              UdrPrintf(f, "[BEGIN UDR diagnostics]");
              FILE *debugFile = (f == stdout ? NULL : f);
              ostream &os = cout;
              NADumpDiags(os,                 // ostream &
                          udrDiagsForCaller_, // ComDiagsArea *
                          TRUE,               // add newline after condition?
                          FALSE,              // to avoid "--" comment prefix
                          debugFile,          // FILE *
                          1);                 // for verbose output
              UdrPrintf(f, "[END UDR diagnostics]");
#endif // UDR_DEBUG

            } // if (tcbExpectsReply()) ... else ...
          } // if integrity check did not fail

          if (!keepUdrDiagsForCaller_)
          {
            // Release the incoming diags if our caller is not
            // interested in them
            udrDiagsForCaller_->decrRefCount();
            udrDiagsForCaller_ = NULL;
          }

        } // case IPC_SQL_DIAG_AREA
        break;
        
        default:
        {
          UdrDebug2("*** ERROR: Unrecognized message of type %d arrived",
                    (Lng32) t, this);
          integrityCheckFailed = TRUE;
        }
        break;
        
      } // switch (getNextObjType(msgType))
    } // while (!integrityCheckFailed && moreObjects())

    if (integrityCheckFailed)
    {
      ComDiagsArea *targetDiags = NULL;

      if (tcbExpectsReply())
      {
        targetDiags = tcb_->getOrCreateStmtDiags();
      }
      else if (stmtGlobalsExpectsReply())
      {
        targetDiags = stmtGlobals_->getDiagsArea();
        if (targetDiags == NULL)
        {
          targetDiags = ComDiagsArea::allocate(ipcHeap);
          stmtGlobals_->setGlobDiagsArea(targetDiags);
          targetDiags->decrRefCount();
        }
      }

      if (targetDiags == NULL)
      {
        if (!udrDiagsForCaller_)
          udrDiagsForCaller_ = ComDiagsArea::allocate(ipcHeap);
        targetDiags = udrDiagsForCaller_;
      }

      if (targetDiags)
      {
        if (conn)
        {
          *targetDiags << DgSqlCode(-2037);
          environment_->getMyOwnProcessId(IPC_DOM_GUA_PHANDLE).
            addProcIdToDiagsArea(*targetDiags, 0);
          conn->getOtherEnd().addProcIdToDiagsArea(*targetDiags, 1);
        }
        *targetDiags << DgSqlCode(-EXE_UDR_INVALID_OR_CORRUPT_REPLY);
        
        if (!tcbExpectsReply() && !stmtGlobalsExpectsReply() &&
            !keepUdrDiagsForCaller_)
        {
          // Diags arrived but we have no place to put
          // them. That's not an error but in the debug build we
          // will print warning messages in case a coding mistake
          // has been made.
#ifdef UDR_DEBUG
          FILE *f = (traceFile_ ? traceFile_ : stdout);
          UdrPrintf(f, "***");
          UdrPrintf(f, "*** WARNING: UDR diags arrived on stream %p",
                    this);
          UdrPrintf(f, "***          but the stream is not currently in");
          UdrPrintf(f, "***          a state where it can process diags");
          UdrPrintf(f, "***");
          UdrPrintf(f, "[BEGIN UDR diagnostics]");
          FILE *debugFile = (f == stdout ? NULL : f);
          ostream &os = cout;
          NADumpDiags(os,          // ostream &
                      targetDiags, // ComDiagsArea *
                      TRUE,        // add newline after each condition?
                      FALSE,       // to avoid "--" comment prefix
                      debugFile,   // FILE *
                      1);          // for verbose output
          UdrPrintf(f, "[END UDR diagnostics]");
#endif // UDR_DEBUG
        }
      } // if (targetDiags)

      if (!keepUdrDiagsForCaller_)
      {
        // Release any newly generated diags if our caller is not
        // interested in them
        if (udrDiagsForCaller_)
        {
          udrDiagsForCaller_->decrRefCount();
          udrDiagsForCaller_ = NULL;
        }
      }

    } // if (integrityCheckFailed)
    
    //
    // The UDR server is supposed to return diags whenever it is
    // returning an invalid handle. Just in case we get an invalid
    // handle without any diags we generate a generic, uninformative
    // "Invalid UDR handle" diagnostic here.
    //    
    if (loadReplyArrived && !UdrHandleIsValid(udrHandle)
        && !diagsAddedToStmt && tcbExpectsReply())
    {
      UdrDebug0("  *** WARNING: Invalid handle arrived without diags");
      ComDiagsArea *d = tcb_->getOrCreateStmtDiags();
      *d << DgSqlCode(-EXE_UDR_INVALID_HANDLE);
      diagsAddedToStmt = TRUE;
    }

    //
    // Cases to consider for UDR control replies:
    // a) Valid handle arrived, TCB expects reply. This is the common case
    //    for a LOAD reply. Notify the TCB.
    // b) Valid handle arrived, TCB does not expect reply. Nothing to do.
    //    The TCB destructor is responsible for sending the UNLOAD message.
    // c) No valid reply data arrived except possibly diags, and the TCB
    //    expects a reply. This will happen when a LOAD reply is reporting
    //    errors. Notify the TCB.
    // d) No valid reply data arrived, TCB does not expect reply.
    //    This is the common case for an UNLOAD reply. Nothing to do.
    //
    // Note: Currently a LOAD reply never contains a valid handle plus
    // warnings. The code in this method may work fine if a LOAD reply
    // contains warnings but that has not been verified.
    //
    if (loadReplyArrived && UdrHandleIsValid(udrHandle))
    {
      if (tcbExpectsReply())
      {
        //
        // Case a)
        //
        tcb_->reportLoadReply(TRUE);
      }
      else
      {
        //
        // Case b)
        //
        UdrDebug0("  *** WARNING: Valid handle arrived, no TCB expects reply");
      }
    } // if (loadReplyArrived && UdrHandleIsValid(udrHandle))

    else if (tcbExpectsReply())
    {
      //
      // Case c)
      //
      tcb_->reportLoadReply(FALSE);

    } // if (loadReplyArrived && UdrHandleIsValid(udrHandle)) else ...
    
  } // if (conn->getErrorInfo() != 0) else ...

  //
  // UDR control streams are only being used for a single message and
  // reply. In the code above when this method calls a tcb_->reportX()
  // method, inside that TCB method the TCB will detach itself from
  // this stream. To catch programming errors where the TCB does not
  // detach we issue the following assertion.
  //
  ex_assert(!tcb_,
            "UDR control stream going away before TCB called delinkTcb()");

  UdrDebug2("[END RECV CALLBACK %lu] client control, this %p",
            recvCount_, this);
} // UdrClientControlStream::actOnReceive()

void UdrClientControlStream::actOnReceiveAllComplete()
{
  //
  // Add this stream to the global list of completed requests.
  // The destructor will be called at a time when it is safe
  // to do so. See comments for IpcMessageStreamBase::addToCompletedList()
  // in common/Ipc.h.
  //
  addToCompletedList();
  UdrDebug1("  UdrClientControlStream %p added to completed list",
            this);
} // UdrClientControlStream::actOnReceive()

ComDiagsArea *UdrClientControlStream::extractUdrDiags()
{
  ComDiagsArea *result = NULL;
  if (keepUdrDiagsForCaller_ && udrDiagsForCaller_)
  {
    result = udrDiagsForCaller_;
    udrDiagsForCaller_ = NULL;
  }
  return result;
}

//---------------------------------------------------------------------------
// class UdrClientDataStream
//---------------------------------------------------------------------------
UdrClientDataStream::UdrClientDataStream(IpcEnvironment *env,
                                         Lng32 sendBufferLimit,
                                         Lng32 inUseBufferLimit,
                                         IpcMessageObjSize bufferSize,
                                         ExUdrTcb *tcb,
                                         ExExeStmtGlobals *stmtGlobals,
                                         NABoolean isTransactional)
  : IpcClientMsgStream(env,
                       UDR_STREAM_CLIENT_DATA,
                       UdrClientDataStreamVersionNumber,
                       sendBufferLimit,
                       inUseBufferLimit,
                       bufferSize),
    tcb_(tcb),
    stmtGlobals_(stmtGlobals),
    sendCount_(0),
    recvCount_(0),
    isTransactional_(isTransactional)
#ifdef UDR_DEBUG
    , traceFile_(NULL), trustReplies_(FALSE)
#endif
{
}

UdrClientDataStream::~UdrClientDataStream()
{
  UdrDebug1("  UdrClientDataStream destructor %p", this);
  UdrDebug2("    %u message%s sent",
    sendCount_, PLURAL_SUFFIX(sendCount_));
  UdrDebug2("    %u message%s received",
    recvCount_, PLURAL_SUFFIX(recvCount_));
  UdrDebug2("    tcb_ %p, stmtGlobals_ %p", tcb_, stmtGlobals_);
  ex_assert(!tcb_, "UDR data stream going away before TCB called delinkTcb()");
}

void UdrClientDataStream::delinkTcb(const ExUdrTcb *tcb)
{
  UdrDebug0("  UdrClientDataStream::delinkTcb()");
  UdrDebug2("    this %p, tcb_ %p", this, tcb_);
  ex_assert(tcb == tcb_, "The wrong TCB called delinkTcb()");

  if (tcb == tcb_)
  {
    tcb_ = NULL;

    // abort any outstanding I/Os on the stream
    if (numOfResponsesPending() > 0)
      abandonPendingIOs();

    //
    // Add this stream to the global list of completed requests.
    // The destructor will be called at a time when it is safe
    // to do so. See comments for IpcMessageStreamBase::addToCompletedList()
    // in common/Ipc.h.
    //
    addToCompletedList();
    UdrDebug1("  UdrClientDataStream %p added to the completed list", this);
  }
}

//
// This function returns TRUE if the tcb_ pointer is valid
// and that TCB expects to be notified when a reply message
// arrives.
//
NABoolean UdrClientDataStream::tcbExpectsReply() const
{
  //
  // For now, as long as tcb_ is not NULL we will assume that
  // it expects a reply. In the future we may 
  //
  NABoolean result = (tcb_ != NULL);
  return result;
}

//
// This function returns TRUE if the stmtGlobals_ pointer is valid and
// that object expects to be notified when replies arrive.
//
NABoolean UdrClientDataStream::stmtGlobalsExpectsReply() const
{
  //
  // For now, as long as stmtGlobals_ is not NULL we will assume that
  // it expects a reply
  //
  NABoolean result = (stmtGlobals_ != NULL);
  return result;
}

void UdrClientDataStream::actOnSend(IpcConnection *conn)
{
  ex_assert(conn, "Connection is NULL in send callback");

  sendCount_++;
  UdrDebug2("[BEGIN SEND CALLBACK %u] client data, this %p",
            sendCount_, this);
  UdrDebug2("  tcb_ %p, stmtGlobals_ %p", tcb_, stmtGlobals_);

  UdrDebug2("[END SEND CALLBACK %u] client data, this %p",
            sendCount_, this);
  if (tcb_)
    tcb_->incReqMsg(conn->getLastSentMsg()->getMessageLength());
}

void UdrClientDataStream::actOnReceive(IpcConnection *conn)
{
  recvCount_++;
  
  UdrDebug2("[BEGIN RECV CALLBACK %u] client data, this %p",
            recvCount_, this);
  UdrDebug2("  tcb_ %p, stmtGlobals_ %p", tcb_, stmtGlobals_);
  if (tcb_)
    tcb_->incReplyMsg(conn->getLastReceivedMsg()->getMessageLength());

#ifdef UDR_DEBUG
  if (getErrorInfo() != 0)
  {
    UdrDebug0("  *** A receive callback detected an IPC error");
  }
#endif // UDR_DEBUG

  if (stmtGlobalsExpectsReply())
  {
    if (isTransactional_)
      stmtGlobals_->decrementUdrTxMsgsOut();
    else
      stmtGlobals_->decrementUdrNonTxMsgsOut();
  }

  if (tcbExpectsReply())
  {
    if (getErrorInfo() != 0)
    {
      tcb_->reportIpcError(this, conn);
    }
    else
    {
      tcb_->reportDataArrival();
    }
  } // if (tcbExpectsReply())

  UdrDebug2("[END RECV CALLBACK %u] client data, this %p",
            recvCount_, this);
}
