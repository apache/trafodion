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
// File:         ExCancel.h
// Description:  Class declaration for ExCancelTdb and ExCancelTcb.
//
// Created:      Oct 15, 2009
// **********************************************************************
#include "Platform.h"

#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ExpError.h"
#include "ExCancel.h"
#include "SqlStats.h"
#include "Globals.h"
#include "Context.h"
#include "ex_exe_stmt_globals.h"
#include "ComCextdecs.h"
#include "seabed/ms.h"                       // msg_mon_get_process_info

/////////////////////////////////////////////////////////////////////////
// Methods for ExCancelTdb
/////////////////////////////////////////////////////////////////////////

ex_tcb * ExCancelTdb::build(ex_globals * glob)
{
  ExMasterStmtGlobals * exe_glob = glob->castToExExeStmtGlobals()->
                                        castToExMasterStmtGlobals();
  
  ex_assert(exe_glob,"Cancel operator must be in master.");

  ExCancelTcb *cancel_tcb = 
    new(exe_glob->getSpace()) ExCancelTcb(*this, exe_glob);

  ex_assert(cancel_tcb, "Error building ExCancelTcb.");

  // Add subtasks to the scheduler.
  cancel_tcb->registerSubtasks();

  return (cancel_tcb);
}


/////////////////////////////////////////////////////////////////////////
// Methods for ExCancelTcb
/////////////////////////////////////////////////////////////////////////

ExCancelTcb::ExCancelTcb(const ExCancelTdb & cancel_tdb, ex_globals *glob) :
    ex_tcb(cancel_tdb, 1, glob)
  , step_(NOT_STARTED)
  , ioSubtask_(NULL)
  , cbServer_(NULL)
  , cancelStream_(NULL)
  , cpu_ (-1)
  , pid_ (-1)
  , retryQidNotActive_(false)
  , retryCount_(0)
{
  nodeName_[0] = '\0';

  allocateParentQueues(qparent_,
                       FALSE );     // don't alloc pstate.

  retryQidNotActive_ = (getenv("SQLMX_REGRESS") != NULL);
}
  
/////////////////////////////////////////////////////////////////////////////
// register tcb for work
  
void ExCancelTcb::registerSubtasks()
{
  ex_tcb::registerSubtasks();

  // register a non-queue event for the IPC with the send top node
  ioSubtask_ =
    getGlobals()->getScheduler()->registerNonQueueSubtask(sWork,this);
}
  
ExCancelTcb::~ExCancelTcb()
{
  freeResources();
}
  
void ExCancelTcb::freeResources() 
{
  if (qparent_.up)
  {
    delete qparent_.up;
    qparent_.up = NULL;
  }
  if (qparent_.down)
  {
    delete qparent_.down;
    qparent_.down = NULL;
  }
  ex_assert(cancelStream_ == NULL, "freeResources called before step_ DONE.");
}

ExWorkProcRetcode ExCancelTcb::work()
{

  ExMasterStmtGlobals *masterGlobals = 
     getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals();

  CliGlobals *cliGlobals = masterGlobals->getCliGlobals();

  while ((qparent_.down->isEmpty() == FALSE) && 
         (qparent_.up->isFull() == FALSE))
  {
    ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
  
    switch (step_)
    {
      case NOT_STARTED:
      {
        if (pentry_down->downState.request == ex_queue::GET_NOMORE)
          step_ = DONE;
        else
        {
          retryCount_ = 0;
          // Priv checking is done during compilation. To support 
          // REVOKE, prevent a prepared CANCEL/SUSPEND/ACTIVATE
          // that was compiled more than 1 second ago from executing 
          // by raising the 8734 error to force an AQR. 
          Int64 microSecondsSinceCompile = NA_JulianTimestamp() - 
              masterGlobals->getStatement()->getCompileEndTime();

          if (microSecondsSinceCompile > 1000*1000)
          {

            ComDiagsArea *diagsArea =
              ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
            *diagsArea << DgSqlCode(-CLI_INVALID_QUERY_PRIVS);
            reportError(diagsArea);
            step_ = DONE;
            break;
          }
          
          // Figure out which MXSSMP broker to use.
          if (cancelTdb().getAction() == ComTdbCancel::CancelByPname)
          {
            int nid = -1;
            int rc = msg_mon_get_process_info(cancelTdb().getCancelPname(),
                                &nid, &pid_);
            switch (rc)
            {
              case XZFIL_ERR_OK:
                cpu_ = (short) nid;
                break;
              case XZFIL_ERR_NOTFOUND:
              case XZFIL_ERR_BADNAME:
              case XZFIL_ERR_NOSUCHDEV:
                {
                  ComDiagsArea *diagsArea =
                    ComDiagsArea::allocate(getGlobals()->getDefaultHeap());

                  *diagsArea << DgSqlCode(-EXE_CANCEL_PROCESS_NOT_FOUND);
                  *diagsArea << DgString0(cancelTdb().getCancelPname());
                  reportError(diagsArea);

                  step_ = DONE;
                  break;
                }
              default:
                {
                  char buf[200];
                  str_sprintf(buf, "Unexpected error %d returned from "
                                   "msg_mon_get_process_info", rc);
                  ex_assert(0, buf);
                }
            }
            if (step_ != NOT_STARTED)
              break;
          }
          else if  (cancelTdb().getAction() == ComTdbCancel::CancelByNidPid)
          {
            cpu_ = (short) cancelTdb().getCancelNid();
            pid_ = cancelTdb().getCancelPid();

            // check that process exists, if not report error.
            char processName[MS_MON_MAX_PROCESS_NAME];
            int rc = msg_mon_get_process_name(cpu_, pid_, processName);
            if (XZFIL_ERR_OK == rc)
              ; // good. nid & pid are valid.
            else
            {
              if ((XZFIL_ERR_NOTFOUND  != rc) &&
                  (XZFIL_ERR_BADNAME   != rc) &&
                  (XZFIL_ERR_NOSUCHDEV != rc))
              {
                // Log rc in case it needs investigation later.
               char buf[200];
               str_sprintf(buf, "Unexpected error %d returned from "
                                "msg_mon_get_process_name", rc);
               SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__,
                                               buf, 0);
              }
              char nid_pid_str[32];
              str_sprintf(nid_pid_str, "%d, %d", cpu_, pid_);
              ComDiagsArea *diagsArea =
                    ComDiagsArea::allocate(getGlobals()->getDefaultHeap());

              *diagsArea << DgSqlCode(-EXE_CANCEL_PROCESS_NOT_FOUND);
              *diagsArea << DgString0(nid_pid_str);
              reportError(diagsArea);

              step_ = DONE;
              break;
            }
          }
          else
          {
            char * qid = cancelTdb().qid_;
            Lng32 qid_len = str_len(qid);

            // This static method is defined in SqlStats.cpp.  It side-effects
            // the nodeName and cpu_ according to the input qid.
            if (getMasterCpu(
                  qid, qid_len, nodeName_, sizeof(nodeName_) - 1, cpu_) == -1)
            {
              ComDiagsArea *diagsArea =
                ComDiagsArea::allocate(getGlobals()->getDefaultHeap());

              *diagsArea << DgSqlCode(-EXE_RTS_INVALID_QID);

              reportError(diagsArea);

              step_ = DONE;
              break;
            }
          }
          
          ComDiagsArea *tempDiagsArea = NULL;
 
          ContextCli *context = getGlobals()->castToExExeStmtGlobals()->
                castToExMasterStmtGlobals()->getStatement()->getContext();
          ExSsmpManager *ssmpManager = context->getSsmpManager(); 
          cbServer_ = ssmpManager->getSsmpServer((NAHeap *)getGlobals()->getDefaultHeap(),
                                 nodeName_,
                                 cpu_, tempDiagsArea);
          if (cbServer_ == NULL) {
             reportError(tempDiagsArea, true, EXE_CANCEL_PROCESS_NOT_FOUND, 
                          nodeName_, cpu_);

             step_ = DONE;
             break;
          }

          //Create the stream on the IpcHeap, since we don't dispose 
          // of it immediately.  We just add it to the list of completed 
          // messages in the IpcEnv, and it is disposed of later.

          cancelStream_  = new (cliGlobals->getIpcHeap())
                CancelMsgStream(cliGlobals->getEnvironment(), this, ssmpManager);

          cancelStream_->addRecipient(cbServer_->getControlConnection());

        }

        step_ = SEND_MESSAGE;

        break;
      }  // end case NOT_STARTED


      case SEND_MESSAGE:
      {
        RtsHandle rtsHandle = (RtsHandle) this;

        if (cancelTdb().actionIsCancel())
        {
          Int64 cancelStartTime = JULIANTIMESTAMP();

          Lng32 firstEscalationInterval = cliGlobals->currContext()->
                    getSessionDefaults()->getCancelEscalationInterval();

          Lng32 secondEscalationInterval = cliGlobals->currContext()->
                    getSessionDefaults()->getCancelEscalationMxosrvrInterval();

          NABoolean cancelEscalationSaveabend = cliGlobals->currContext()->
                    getSessionDefaults()->getCancelEscalationSaveabend();

          bool cancelLogging = (TRUE == cliGlobals->currContext()->
                    getSessionDefaults()->getCancelLogging());

          CancelQueryRequest *cancelMsg = new (cliGlobals->getIpcHeap()) 
            CancelQueryRequest(rtsHandle, cliGlobals->getIpcHeap(), 
                      cancelStartTime,
                      firstEscalationInterval,
                      secondEscalationInterval,
                      cancelEscalationSaveabend,
                      cancelTdb().getCommentText(),
                      str_len(cancelTdb().getCommentText()),
                      cancelLogging,
                      cancelTdb().action_ != ComTdbCancel::CancelByQid,
                      pid_,
                      cancelTdb().getCancelPidBlockThreshold());


          *cancelStream_ << *cancelMsg;

          cancelMsg->decrRefCount();
        }
        else if (ComTdbCancel::Suspend == cancelTdb().action_)
        {

          bool suspendLogging = (TRUE == cliGlobals->currContext()->
                    getSessionDefaults()->getSuspendLogging());

          SuspendQueryRequest * suspendMsg = new (cliGlobals->getIpcHeap()) 
            SuspendQueryRequest(rtsHandle, cliGlobals->getIpcHeap(),
                                ComTdbCancel::Force ==
                                cancelTdb().forced_,
                                suspendLogging);

          *cancelStream_ << *suspendMsg;

          suspendMsg->decrRefCount();
        }
        else
        {
          ex_assert(
            ComTdbCancel::Activate == cancelTdb().action_,
            "invalid action for ExCancelTcb");

          bool suspendLogging = (TRUE == cliGlobals->currContext()->
                    getSessionDefaults()->getSuspendLogging());

          ActivateQueryRequest * activateMsg = new (cliGlobals->getIpcHeap()) 
            ActivateQueryRequest(rtsHandle, cliGlobals->getIpcHeap(),
                                 suspendLogging);

          *cancelStream_ << *activateMsg;

          activateMsg->decrRefCount();
        }

        if ((cancelTdb().getAction() != ComTdbCancel::CancelByPname) &&
            (cancelTdb().getAction() != ComTdbCancel::CancelByNidPid))
        {
          char * qid = cancelTdb().qid_;
          Lng32 qid_len = str_len(qid);

          RtsQueryId *rtsQueryId = new (cliGlobals->getIpcHeap())
                           RtsQueryId( cliGlobals->getIpcHeap(), qid, qid_len);

          *cancelStream_ << *rtsQueryId;
          rtsQueryId->decrRefCount();
        }

        // send a no-wait request to the cancel broker.
        cancelStream_->send(FALSE);

        step_ = GET_REPLY;    
        // Come back when I/O completes.
        return WORK_OK; 

        break;
      }  // end case SEND_MESSAGE

      case GET_REPLY:
      {

        // Handle general IPC error.
        bool fakeError201 = false;
        fakeError201 = (getenv("HP_FAKE_ERROR_201") != NULL);
        if ((cbServer_->getControlConnection()->getErrorInfo() != 0) ||
            fakeError201)
        {
          ComDiagsArea *diagsArea = 
            ComDiagsArea::allocate(getGlobals()->getDefaultHeap());

          cbServer_->getControlConnection()->
              populateDiagsArea( diagsArea, getGlobals()->getDefaultHeap());

          if (fakeError201)
          {
            *diagsArea << DgSqlCode(-2034) << DgInt0(201)
                       << DgString0("I say") << DgString1("control broker");
          }

          if (diagsArea->contains(-8921))
          {
            // Should not get timeout error 8921. Get a core-file
            // of the SSMP and this process too so that this can be
            // debugged.
            cbServer_->getControlConnection()->
              dumpAndStopOtherEnd(true, false);
            genLinuxCorefile("Unexpected timeout error");
          }

          reportError(diagsArea);

          step_ = DONE;
          break;
        }

        // See if stream has the reply yet.
        if (!cancelStream_->moreObjects())
          return WORK_OK; 


        ControlQueryReply *reply = new (cliGlobals->getIpcHeap()) 
              ControlQueryReply(INVALID_RTS_HANDLE, cliGlobals->getIpcHeap());


        *cancelStream_ >> *reply;

        if (reply->didAttemptControl())
        {
          // yeaah!
          cancelStream_->clearAllObjects();
        }
        else
        {
          if (cancelStream_->moreObjects() &&
              cancelStream_->getNextObjType() == IPC_SQL_DIAG_AREA)
          {
            ComDiagsArea *diagsArea = 
              ComDiagsArea::allocate(getGlobals()->getDefaultHeap());

            *cancelStream_ >> *diagsArea;
            cancelStream_->clearAllObjects();

            if ( retryQidNotActive_ &&
                (diagsArea->mainSQLCODE() == -EXE_SUSPEND_QID_NOT_ACTIVE) &&
                (++retryCount_ <= 60))
            {
              SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, 
                                             "Retrying error 8672.", 0);
              DELAY(500);
              diagsArea->decrRefCount();
              step_ = SEND_MESSAGE;
              break;
            }

            reportError(diagsArea);
          }
          else 
            ex_assert(0, "Control failed, but no diagnostics.");
        }

        step_ = DONE;
        break;
      }
      case DONE: 
      {
        if (cancelStream_)
        {
          cancelStream_->addToCompletedList();
          cancelStream_ = NULL;
        }
        ex_queue_entry * up_entry = qparent_.up->getTailEntry();
        up_entry->copyAtp(pentry_down);
        up_entry->upState.parentIndex = pentry_down->downState.parentIndex;
        up_entry->upState.downIndex = qparent_.down->getHeadIndex();     
        up_entry->upState.setMatchNo(1);
        up_entry->upState.status = ex_queue::Q_NO_DATA;
        qparent_.up->insert();

        qparent_.down->removeHead();

        step_ = NOT_STARTED;
        break;
      }
      default:
        ex_assert( 0, "Unknown step_.");
    }
  }  // while have a request and have room for a reply.
  return WORK_OK;
}

void ExCancelTcb::reportError(ComDiagsArea *da, bool addCondition, 
                              Lng32 SQLCode, char *nodeName, short cpu)
{
  ex_queue_entry *down_entry = qparent_.down->getHeadEntry();
  ex_queue_entry * up_entry = qparent_.up->getTailEntry();

  ComDiagsArea *prevDiagsArea = down_entry->getDiagsArea();
  if (prevDiagsArea)
    da->mergeAfter(*prevDiagsArea);

  if (addCondition)
    {
      ComDiagsArea *diagsArea = 
        ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
      
      *diagsArea << DgSqlCode(-SQLCode);

      if (nodeName)
      {
        char processName[50];
        ContextCli *context = getGlobals()->castToExExeStmtGlobals()->
                castToExMasterStmtGlobals()->getStatement()->getContext();
        context->getSsmpManager()->getServerClass()->getProcessName(cpu, processName);
        *diagsArea << DgString0(processName);
      }

      diagsArea->mergeAfter(*da);
      up_entry->setDiagsArea(diagsArea);
      da->decrRefCount();
    }
  else
    up_entry->setDiagsArea(da);

  up_entry->upState.status = ex_queue::Q_SQLERROR;
  up_entry->upState.downIndex = qparent_.down->getHeadIndex();
  up_entry->upState.parentIndex = down_entry->downState.parentIndex;
  up_entry->upState.setMatchNo(0);
  qparent_.up->insert();
  return;
}


// -----------------------------------------------------------------------
// Methods for CancelMsgStream. 
// -----------------------------------------------------------------------

void CancelMsgStream::actOnSend(IpcConnection *conn)
{
  if (conn->getErrorInfo() != 0)
     delinkConnection(conn);
}

void CancelMsgStream::actOnSendAllComplete()
{
  clearAllObjects();
  receive(FALSE);   // FALSE means no-waited.
  return;
}

void CancelMsgStream::actOnReceive(IpcConnection *conn)
{
  if (conn->getErrorInfo() != 0)
     delinkConnection(conn);
}

void CancelMsgStream::delinkConnection(IpcConnection *conn)
{
  char nodeName[MAX_SEGMENT_NAME_LEN+1];
  IpcCpuNum cpu;

  conn->getOtherEnd().getNodeName().getNodeNameAsString(nodeName);
  cpu = conn->getOtherEnd().getCpuNum();
  ssmpManager_->removeSsmpServer(nodeName, (short)cpu);
}


void CancelMsgStream::actOnReceiveAllComplete()
{
  ExExeStmtGlobals *glob = cancelTcb_->getGlobals()->
                             castToExExeStmtGlobals();
  cancelTcb_->tickleSchedulerWork(TRUE);
}

