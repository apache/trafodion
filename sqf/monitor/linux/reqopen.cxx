///////////////////////////////////////////////////////////////////////////////
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "reqqueue.h"
#include "montrace.h"
#include "monsonar.h"
#include "monlogging.h"

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
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf),
              "ExtReq(%s) req #=%ld requester (%d, %d), opening %s"
              , CReqQueue::svcReqType[reqType_], getId(), 
              msg_->u.request.u.open.nid, msg_->u.request.u.open.pid,
              msg_->u.request.u.open.process_name );
    requestString_.assign( strBuf );
}

void CExtOpenReq::performRequest()
{
    const char method_name[] = "CExtOpenReq::performRequest";
    TRACE_ENTRY;


    // Record statistics (sonar counters)
       MonStats->req_type_open_Incr();

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

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf("%s@%d request #%ld: open process: opener=%s (%d, %d), "
                     "opened=%s (%d, %d), notify=%d\n",
                     method_name, __LINE__, id_,
                     opener->GetName(), opener->GetNid(), opener->GetPid(),
                     opened->GetName(), opened->GetNid(), opened->GetPid(),
                     msg_->u.request.u.open.death_notification);
    }

    status = opener->Open (opened, msg_->u.request.u.open.death_notification);

    if (status == SUCCESS)
    {
        msg_->noreply = false;
        msg_->u.reply.type = ReplyType_Open;
        msg_->u.reply.u.open.nid = opened->GetNid();
        msg_->u.reply.u.open.pid = opened->GetPid();
        STRCPY (msg_->u.reply.u.open.port, opened->GetPort());
        msg_->u.reply.u.open.type = opened->GetType();
        msg_->u.reply.u.open.return_code = MPI_SUCCESS;
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Successful\n", method_name, __LINE__);

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

    if ( prepared_ == true )
    {   // Already did the prepare work earlier.
        return true;
    }

    if ((msg_->u.request.u.open.nid < 0) ||
        (msg_->u.request.u.open.nid >= Nodes->NumberLNodes))
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "%s, Invalid Node ID (%d)\n", method_name,
                msg_->u.request.u.open.nid);
        mon_log_write(MON_REQQUEUE_PREP_REQ_1, SQ_LOG_ERR, buf);

        errorReply( MPI_ERR_NAME );

        TRACE_EXIT;
        return false;
    }

    // Get process object for opener process
    CProcess *openerProcess = Nodes->GetProcess( msg_->u.request.u.open.nid, 
                                                 msg_->u.request.u.open.pid );

    if (!openerProcess)
    {  // Could not find the process
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Process (%d,%d) not found\n",
                         method_name, __LINE__,
                         msg_->u.request.u.open.nid,
                         msg_->u.request.u.open.pid);
        errorReply( MPI_ERR_NAME );
        TRACE_EXIT;
        return false;
    }

    // Get process object for process to open
    CProcess * openedProcess;

    if (Nodes->GetLNode( msg_->u.request.u.open.process_name, &openedProcess,
                        false, true ))
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
                             msg_->u.request.u.open.process_name);

            errorReply( MPI_ERR_NAME );
            TRACE_EXIT;
            return false;
        }
    }
    else
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Can't find process %s\n", method_name,
                         __LINE__, msg_->u.request.u.open.process_name);

        errorReply( MPI_ERR_NAME );
        TRACE_EXIT;
        return false;
    }

    // Add the opener process and the opened process to the resource
    // list for this request.
    addResource(new CReqResourceProc(openerProcess->GetNid(),
                                     openerProcess->GetPid()));
    addResource(new CReqResourceProc(openedProcess->GetNid(),
                                     openedProcess->GetPid()));

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        trace_printf("%s@%d - opener=%s (%d, %d), opened=%s (%d, %d)\n",
                     method_name, __LINE__,
                     openerProcess->GetName(), openerProcess->GetNid(),
                     openerProcess->GetPid(),
                     openedProcess->GetName(), openedProcess->GetNid(),
                     openedProcess->GetPid());

    prepared_ = true;

    TRACE_EXIT;
    return true;
}
