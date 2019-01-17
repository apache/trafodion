///////////////////////////////////////////////////////////////////////////////
//
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
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "reqqueue.h"
#include "montrace.h"
#include "monsonar.h"
#include "monlogging.h"
#include "ptpclient.h"

extern CMonStats *MonStats;
extern CNode *MyNode;
extern CNodeContainer *Nodes;
extern bool NameServerEnabled;
extern CPtpClient *PtpClient;

extern _TM_Txid_External invalid_trans( void );

CExtNotifyReq::CExtNotifyReq (reqQueueMsg_t msgType, int pid,
                              struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQEM", 4);
}

CExtNotifyReq::~CExtNotifyReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqem", 4);
}

void CExtNotifyReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld "
              "requester(name=%s/nid=%d/pid=%d/os_pid=%d/verifier=%d) "
              "target(name=%s/nid=%d/pid=%d/verifier=%d) (cancel=%d)"
            , CReqQueue::svcReqType[reqType_], getId()
            , msg_->u.request.u.notify.process_name
            , msg_->u.request.u.notify.nid
            , msg_->u.request.u.notify.pid
            , pid_
            , msg_->u.request.u.notify.verifier
            , msg_->u.request.u.notify.target_process_name
            , msg_->u.request.u.notify.target_nid
            , msg_->u.request.u.notify.target_pid
            , msg_->u.request.u.notify.target_verifier
            , msg_->u.request.u.notify.cancel );
    requestString_.assign( strBuf );
}

void CExtNotifyReq::performRequest()
{
    const char method_name[] = "CExtNotifyReq::performRequest";
    TRACE_ENTRY;

    bool status = FAILURE;
    CProcess *requester;

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_type_notify_Incr();

    int pid = -1;
    int target_nid = -1;
    int target_pid = -1;
    Verifier_t target_verifier = -1;
    string target_process_name;

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d request #%ld: Notify, requester %s (%d, %d:%d), "
                      "target %s (%d, %d:%d), cancel=%d\n"
                    , method_name, __LINE__, id_
                    , msg_->u.request.u.notify.process_name
                    , msg_->u.request.u.notify.nid
                    , msg_->u.request.u.notify.pid
                    , msg_->u.request.u.notify.verifier
                    , msg_->u.request.u.notify.target_process_name
                    , msg_->u.request.u.notify.target_nid
                    , msg_->u.request.u.notify.target_pid
                    , msg_->u.request.u.notify.target_verifier
                    , msg_->u.request.u.notify.cancel );
    }

    nid_ = msg_->u.request.u.notify.nid;
    pid = msg_->u.request.u.notify.pid;
    verifier_ = msg_->u.request.u.notify.verifier;
    processName_ = msg_->u.request.u.notify.process_name;
    target_nid = msg_->u.request.u.notify.target_nid;
    target_pid = msg_->u.request.u.notify.target_pid;
    target_process_name = (const char *) msg_->u.request.u.notify.target_process_name;
    target_verifier  = msg_->u.request.u.notify.target_verifier;
    
    if ( processName_.size() )
    { // find by name
        requester = MyNode->GetProcess( processName_.c_str()
                                      , verifier_ );
    }
    else
    { // find by pid
        requester = MyNode->GetProcess( pid_
                                      , verifier_ );
    }

    if ( requester )
    {
        CProcess *sourceProcess = requester;
        if ( sourceProcess )
        {
            if (trace_settings & TRACE_REQUEST)
            {
                trace_printf( "%s@%d - Found sourceProcess %s (%d,%d:%d), clone=%d\n"
                            , method_name, __LINE__
                            , sourceProcess->GetName()
                            , sourceProcess->GetNid()
                            , sourceProcess->GetPid()
                            , sourceProcess->GetVerifier()
                            , sourceProcess->IsClone() );
            }
        }
        else
        {
            if (trace_settings & TRACE_REQUEST)
            {
                trace_printf( "%s@%d - Can't find sourceProcess %s (%d,%d:%d)\n"
                            , method_name, __LINE__
                            , processName_.c_str()
                            , nid_
                            , pid
                            , verifier_ );
            }
        }

        if ( msg_->u.request.u.notify.cancel )
        {
            if ( sourceProcess &&
                 msg_->u.request.u.notify.cancel)
            {   // Cancel all death notification requests for the source
                // process (considering transaction id)
                sourceProcess->procExitUnregAll(msg_->u.request.u.notify.trans_id);
                status = SUCCESS;
            }
        }
        else
        {
            CProcess *targetProcess = NULL;

            if ( target_process_name.size() )
            { // find by name (check node state, don't check process state, not backup)
                if (trace_settings & TRACE_REQUEST)
                {
                    trace_printf( "%s@%d" " - Finding targetProcess (%s:%d)" "\n"
                                , method_name, __LINE__
                                , target_process_name.c_str()
                                , target_verifier );
                }
                if (msg_->u.request.u.notify.target_process_name[0] == '$' )
                {
                    targetProcess = Nodes->GetProcess( target_process_name.c_str()
                                                     , target_verifier
                                                     , true, false, false );
                }
            }     
            else
            { // find by nid (check node state, don't check process state, backup is Ok)
                if (trace_settings & TRACE_REQUEST)
                {
                    trace_printf( "%s@%d" " - Finding targetProcess (%d,%d:%d)\n"
                                , method_name, __LINE__
                                , target_nid
                                , target_pid
                                , target_verifier );
                }
                targetProcess = Nodes->GetProcess( target_nid
                                                 , target_pid
                                                 , target_verifier
                                                 , true, false, true );
            }
            
            if ( targetProcess )
            {
                if (trace_settings & TRACE_REQUEST)
                {
                    trace_printf( "%s@%d - Found targetProcess %s (%d,%d:%d), clone=%d\n"
                                , method_name, __LINE__
                                , targetProcess->GetName()
                                , targetProcess->GetNid()
                                , targetProcess->GetPid()
                                , targetProcess->GetVerifier()
                                , targetProcess->IsClone() );
                }
            }
            else
            {
                if (NameServerEnabled)
                {
                    if ( target_process_name.size() )
                    { // Name Server find by name:verifier
                        if (trace_settings & TRACE_REQUEST)
                        {
                            trace_printf( "%s@%d" " - Getting targetProcess from Name Server (%s:%d)" "\n"
                                        , method_name, __LINE__
                                        , target_process_name.c_str()
                                        , target_verifier );
                        }
                        if (msg_->u.request.u.notify.target_process_name[0] == '$' )
                        {
                            targetProcess = Nodes->CloneProcessNs( target_process_name.c_str()
                                                                 , target_verifier );
                        }
                    }     
                    else
                    { // Name Server find by nid,pid:verifier
                        if (trace_settings & TRACE_REQUEST)
                        {
                            trace_printf( "%s@%d" " - Getting targetProcess from Name Server (%d,%d:%d)\n"
                                        , method_name, __LINE__
                                        , target_nid
                                        , target_pid
                                        , target_verifier );
                        }
                        targetProcess = Nodes->CloneProcessNs( target_nid
                                                             , target_pid
                                                             , target_verifier );
                    }
                    if (targetProcess)
                    {
                        if (trace_settings & TRACE_REQUEST)
                            trace_printf( "%s@%d - Found targetProcess %s (%d,%d:%d), clone=%d\n"
                                        , method_name, __LINE__
                                        , targetProcess->GetName()
                                        , targetProcess->GetNid()
                                        , targetProcess->GetPid()
                                        , targetProcess->GetVerifier()
                                        , targetProcess->IsClone() );
                    }
                }
            }
            
            if ( targetProcess )
            { 
                int verifier = msg_->u.request.u.notify.target_verifier;
                if ( (verifier != -1) && (verifier != targetProcess->GetVerifier()) )
                {
                    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                    {
                       trace_printf("%s@%d - Notify, target (%d, %d:%d) failed -- verifier mismatch (%d)\n",
                                    method_name, __LINE__,
                                    target_nid,
                                    target_pid,
                                    target_verifier,
                                    targetProcess->GetVerifier());
                    }            
                } 
                else
                {
                    if (sourceProcess)
                    {   // Register interest in death of target process 
                        if (NameServerEnabled && targetProcess->IsClone())
                        {
                            CLNode *targetLNode = Nodes->GetLNode( targetProcess->GetNid() );
                        
                            _TM_Txid_External transId = msg_->u.request.u.notify.trans_id;
                            int rc = -1;

                            // Forward the process cancel death notification to the target node
                            rc = PtpClient->ProcessNotify( sourceProcess->GetNid()
                                                         , sourceProcess->GetPid()
                                                         , sourceProcess->GetVerifier()
                                                         , transId
                                                         , false
                                                         , targetProcess
                                                         , targetLNode->GetNid()
                                                         , targetLNode->GetNode()->GetName() );
                            if (rc)
                            {
                                char la_buf[MON_STRING_BUF_SIZE];
                                snprintf( la_buf, sizeof(la_buf)
                                        , "[%s] - Can't send process notify request "
                                          "for process %s (%d, %d) "
                                          "to target node %s, nid=%d\n"
                                        , method_name
                                        , sourceProcess->GetName()
                                        , sourceProcess->GetNid()
                                        , sourceProcess->GetPid()
                                        , targetLNode->GetNode()->GetName()
                                        , targetLNode->GetNid() );
                                mon_log_write(MON_REQ_NOTIFY_1, SQ_LOG_ERR, la_buf);
                            }
                        }
                        
                        sourceProcess->procExitReg( targetProcess,
                                                    msg_->u.request.u.notify.trans_id);
                        status = SUCCESS;
                    }
                }
            }
            else
            {
                if (trace_settings & TRACE_REQUEST)
                {
                    trace_printf("%s@%d" " - Can't find targetProcess" "\n", method_name, __LINE__);
                }
            }
        }

        msg_->u.reply.type = ReplyType_Generic;
        msg_->u.reply.u.generic.nid = requester->GetNid();
        msg_->u.reply.u.generic.pid = requester->GetPid();
        msg_->u.reply.u.generic.verifier = requester->GetVerifier();
        strcpy (msg_->u.reply.u.generic.process_name, requester->GetName());
        if (status == SUCCESS)
        {
            msg_->u.reply.u.generic.return_code = MPI_SUCCESS;
            if (trace_settings & TRACE_REQUEST)
            {
                trace_printf("%s@%d" " - Successful" "\n", method_name, __LINE__);
            }
        }
        else
        {
            msg_->u.reply.u.generic.return_code = MPI_ERR_NAME;
            if (trace_settings & TRACE_REQUEST)
            {
                trace_printf("%s@%d" " - Unsuccessful" "\n", method_name, __LINE__);
            }
        }

        // Send reply to requester
        lioreply(msg_, pid_);
    }
    else
    {   // Reply to requester so it can release the buffer.  
        // We don't know about this process.
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf("%s@%d - Can't find requester, rc=%d\n", method_name, __LINE__, MPI_ERR_NAME);
        }
        errorReply( MPI_ERR_EXITED );
    }

    TRACE_EXIT;
}
