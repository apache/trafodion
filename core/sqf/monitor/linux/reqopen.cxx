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

#ifndef NAMESERVER_PROCESS
#include "ptpclient.h"
extern bool NameServerEnabled;
extern CPtpClient *PtpClient;
#endif

extern CMonStats *MonStats;
extern CNodeContainer *Nodes;

CExtOpenReq::CExtOpenReq(reqQueueMsg_t msgType, int pid,
                           struct message_def *msg)
    : CExternalReq(msgType, pid, msg), prepared_(false)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQEN", 4);
}

CExtOpenReq::~CExtOpenReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqen", 4);
}

void CExtOpenReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld "
              "requester(name=%s/nid=%d/pid=%d/os_pid=%d/verifier=%d) "
              "target(name=%s/nid=%d/pid=%d/verifier=%d)"
            , CReqQueue::svcReqType[reqType_], getId()
            , msg_->u.request.u.open.process_name
            , msg_->u.request.u.open.nid
            , msg_->u.request.u.open.pid
            , pid_
            , msg_->u.request.u.open.verifier
            , msg_->u.request.u.open.target_process_name
            , msg_->u.request.u.open.target_nid
            , msg_->u.request.u.open.target_pid
            , msg_->u.request.u.open.target_verifier );
    requestString_.assign( strBuf );
}

void CExtOpenReq::performRequest()
{
    const char method_name[] = "CExtOpenReq::performRequest";
    TRACE_ENTRY;

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_type_open_Incr();

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d request #%ld: Open process: opener %s (%d, %d:%d), "
                      "open target %s (%d, %d:%d), death notify=%s\n"
                    , method_name, __LINE__, id_
                    , msg_->u.request.u.open.process_name
                    , msg_->u.request.u.open.nid
                    , msg_->u.request.u.open.pid
                    , msg_->u.request.u.open.verifier
                    , msg_->u.request.u.open.target_process_name
                    , msg_->u.request.u.open.target_nid
                    , msg_->u.request.u.open.target_pid
                    , msg_->u.request.u.open.target_verifier
                    , msg_->u.request.u.open.death_notification ? "true" : "false");
    }
    
    bool status;
    CProcess *opener = ((CReqResourceProc *) resources_[0])->getProcess();
    CProcess *opened = ((CReqResourceProc *) resources_[1])->getProcess();

    // check for the process object as it could have been deleted by the time this request gets to perform.
    if (opened == NULL) 
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
              trace_printf("%s@%d request #%ld: Open process failed. Process already exited.",
                       method_name, __LINE__, id_);
        }
        errorReply( MPI_ERR_NAME );
        TRACE_EXIT;
        return;
    }

    // check the verifier
    int verifier = msg_->u.request.u.open.verifier;
    if ( (verifier != -1) && (verifier != opener->GetVerifier()) )
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
           trace_printf( "%s@%d - Opener %s (%d, %d:%d) lookup failed -- "
                         "verifier mismatch %s (%d, %d:%d)\n"
                       , method_name, __LINE__
                       , msg_->u.request.u.open.process_name
                       , msg_->u.request.u.open.nid
                       , msg_->u.request.u.open.pid
                       , msg_->u.request.u.open.verifier
                       , opener->GetName()
                       , opener->GetNid()
                       , opener->GetPid()
                       , opener->GetVerifier());
        }            
        errorReply( MPI_ERR_NAME );
        TRACE_EXIT;
        return;
    }

    status = opener->Open (opened, msg_->u.request.u.open.death_notification);

    if (status == SUCCESS)
    {
        msg_->noreply = false;
        msg_->u.reply.type = ReplyType_Open;
        msg_->u.reply.u.open.nid = opened->GetNid();
        msg_->u.reply.u.open.pid = opened->GetPid();
        msg_->u.reply.u.open.verifier = opened->GetVerifier();
        STRCPY (msg_->u.reply.u.open.port, opened->GetPort());
        msg_->u.reply.u.open.type = opened->GetType();
        msg_->u.reply.u.open.return_code = MPI_SUCCESS;
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf( "%s@%d - Open successful, opened %s (%d, %d:%d), "
                          "port=%s\n"
                        , method_name, __LINE__
                        , opened->GetName()
                        , msg_->u.reply.u.open.nid
                        , msg_->u.reply.u.open.pid
                        , msg_->u.reply.u.open.verifier
                        , msg_->u.reply.u.open.port );
        }

        // Send reply to requester
        lioreply(msg_, pid_);
    }
    else
    {
        // Send error reply to requester
        errorReply( MPI_ERR_NAME );
    }

    TRACE_EXIT;
}



void CExtOpenReq::errorReply( int rc )
{
    const char method_name[] = "CExtOpenReq::errorReply";
    TRACE_ENTRY;

    msg_->u.reply.type = ReplyType_Open;
    msg_->u.reply.u.open.nid = -1;
    msg_->u.reply.u.open.pid = -1;
    strcpy (msg_->u.reply.u.open.port, "");
    msg_->u.reply.u.open.type = ProcessType_Undefined;
    msg_->u.reply.u.open.return_code = rc;

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        trace_printf("%s@%d - Unsuccessful\n", method_name, __LINE__);


    lioreply(msg_, pid_);
    TRACE_EXIT;
}


bool CExtOpenReq::prepare()
{
    const char method_name[] = "CExtOpenReq::prepare";
    TRACE_ENTRY;

    int target_nid = -1;
    int target_pid = -1;
    Verifier_t target_verifier = -1;
    string target_process_name;
    CLNode *target_lnode = NULL;

    if ( prepared_ == true )
    {   // Already did the prepare work earlier.
        return true;
    }

    target_nid = msg_->u.request.u.open.nid;
    target_lnode = Nodes->GetLNode( target_nid );
    if ( target_lnode == NULL )
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "%s, Invalid Node ID (%d)\n", method_name,
                msg_->u.request.u.open.nid);
        mon_log_write(MON_REQQUEUE_PREP_REQ_1, SQ_LOG_ERR, buf);

        errorReply( MPI_ERR_NAME );

        TRACE_EXIT;
        return false;
    }

    CProcess * openerProcess = NULL;
    CProcess * openedProcess = NULL;

    // Get process object for opener process
    if ( msg_->u.request.u.open.process_name[0] )
    { // find by name (check node state, don't check process state, not backup)
        openerProcess = Nodes->GetProcess( msg_->u.request.u.open.process_name
                                         , msg_->u.request.u.open.verifier
                                         , true, false, false );
    }
    else
    { // find by pid (check node state, don't check process state, backup is Ok)
        openerProcess = Nodes->GetProcess( msg_->u.request.u.open.nid
                                         , msg_->u.request.u.open.pid
                                         , msg_->u.request.u.open.verifier
                                         , true, false, true );
    }
    if (!openerProcess)
    {  // Could not find the process
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            trace_printf( "%s@%d - Process %s (%d,%d:%d) not found\n"
                        , method_name, __LINE__
                        , msg_->u.request.u.open.process_name
                        , msg_->u.request.u.open.nid
                        , msg_->u.request.u.open.pid
                        , msg_->u.request.u.open.verifier);
        errorReply( MPI_ERR_NAME );
        TRACE_EXIT;
        return false;
    }

    // Get process object for process to open
    if ( msg_->u.request.u.open.target_process_name[0] ) 
    { // find by name (check node state, don't check process state, backup is NOT Ok)
        if (msg_->u.request.u.open.target_process_name[0] == '$' )
        {
            openedProcess = Nodes->GetProcess( msg_->u.request.u.open.target_process_name
                                             , msg_->u.request.u.open.target_verifier
                                             , true, false, false );
        }
    }
    else
    { // find by pid (check node state, don't check process state, backup is Ok)
        openedProcess = Nodes->GetProcess( msg_->u.request.u.open.target_nid
                                         , msg_->u.request.u.open.target_pid
                                         , msg_->u.request.u.open.target_verifier
                                         , true, false, true );
    }

    if ( openedProcess == NULL )
    {
        if (NameServerEnabled)
        {
            target_nid = msg_->u.request.u.open.target_nid;
            target_pid = msg_->u.request.u.open.target_pid;
            target_verifier  = msg_->u.request.u.open.target_verifier;
            target_process_name = (const char *) msg_->u.request.u.open.target_process_name;

            if ( target_process_name.size() )
            { // Name Server find by name:verifier
                if (trace_settings & TRACE_REQUEST)
                    trace_printf( "%s@%d" " - Getting targetProcess from Name Server (%s:%d)" "\n"
                                , method_name, __LINE__
                                , target_process_name.c_str()
                                , target_verifier );
                if (msg_->u.request.u.open.target_process_name[0] == '$' )
                {
                    openedProcess = Nodes->CloneProcessNs( target_process_name.c_str()
                                                         , target_verifier );
                }
            }     
            else
            { // Name Server find by nid,pid:verifier
                if (trace_settings & TRACE_REQUEST)
                    trace_printf( "%s@%d" " - Getting targetProcess from Name Server (%d,%d:%d)\n"
                                , method_name, __LINE__
                                , target_nid
                                , target_pid
                                , target_verifier );
                openedProcess = Nodes->CloneProcessNs( target_nid
                                                     , target_pid
                                                     , target_verifier );
            }
        }
        
    }

    if ( openedProcess )
    {
        if ( openedProcess->IsBackup() )
        {
            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                trace_printf("%s@%d - Process %s (%d,%d) is still the backup\n",
                             method_name, __LINE__, openedProcess->GetName(),
                             openedProcess->GetNid(),
                             openedProcess->GetPid());
            // We're in the middle of a process pair takeover
            errorReply( MPI_ERR_EXITED );
            TRACE_EXIT;
            return false;
        }

        CProcess *backup;

        // Check if we are opening our backup process
        backup = openedProcess->GetBackup ();
        if (backup)
        {
            // We are opening our peer of a process pair
            if ((msg_->u.request.u.open.nid == openedProcess->GetNid()) &&
                (msg_->u.request.u.open.pid == openedProcess->GetPid()))
            {
                if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                    trace_printf("%s@%d - Process %s(%d, %d) opening its "
                                 "backup.\n",
                                 method_name, __LINE__, backup->GetName(),
                                 msg_->u.request.u.open.nid,
                                 msg_->u.request.u.open.pid);
                // backup is the peer to open
                openedProcess = backup;
            }
            // else
            //    "openedProcess" is already the peer to open
        }
        else if ((msg_->u.request.u.open.nid == openedProcess->GetNid()) &&
                 (msg_->u.request.u.open.pid == openedProcess->GetPid()))
        {
            // if not backup then it's invalid to open itself.
            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                trace_printf("%s@%d - Process (%d, %d) attempting to open "
                             "itself.  Sending error reply.\n",
                             method_name, __LINE__, msg_->u.request.u.open.nid,
                             msg_->u.request.u.open.pid);
            errorReply( MPI_ERR_NAME );
            TRACE_EXIT;
            return false;
        }
        
        if ( !   (openedProcess->GetState() == State_Up
               || openedProcess->GetState() == State_Initializing))
        {
            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                trace_printf("%s@%d - Bad state (%d) for process %s\n",
                             method_name, __LINE__, openedProcess->GetState(),
                             msg_->u.request.u.open.target_process_name);

            errorReply( MPI_ERR_NAME );
            TRACE_EXIT;
            return false;
        }
    }
    else
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Can't find target process %s\n", method_name,
                         __LINE__, msg_->u.request.u.open.target_process_name);

        errorReply( MPI_ERR_NAME );
        TRACE_EXIT;
        return false;
    }

    // Add the opener process and the opened process to the resource
    // list for this request.
    addResource(new CReqResourceProc( openerProcess->GetNid()
                                    , openerProcess->GetPid()
                                    , openerProcess->GetName()
                                    , openerProcess->GetVerifier()));
    addResource(new CReqResourceProc( openedProcess->GetNid()
                                    , openedProcess->GetPid()
                                    , openedProcess->GetName()
                                    , openedProcess->GetVerifier()));

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        trace_printf( "%s@%d - opener= %s (%d, %d:%d), opened= %s (%d, %d:%d)\n"
                    , method_name, __LINE__
                    , openerProcess->GetName()
                    , openerProcess->GetNid()
                    , openerProcess->GetPid()
                    , openerProcess->GetVerifier()
                    , openedProcess->GetName()
                    , openedProcess->GetNid()
                    , openedProcess->GetPid()
                    , openedProcess->GetVerifier() );

    prepared_ = true;

    TRACE_EXIT;
    return true;
}
