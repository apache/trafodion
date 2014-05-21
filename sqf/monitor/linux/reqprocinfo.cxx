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
extern CNode *MyNode;
extern CNodeContainer *Nodes;


// Copy information for a specific process into the reply message buffer.
void CExtProcInfoBase::ProcessInfo_CopyData(CProcess *process, ProcessInfoState &procState)
{
    CProcess *parent;

    procState.nid = process->GetNid();
    procState.pid = process->GetPid();
    procState.type = process->GetType();
    strncpy (procState.process_name, process->GetName(), MAX_PROCESS_NAME);
    strncpy (procState.program, process->program(), MAX_PROCESS_PATH);
    procState.os_pid = process->GetPid();
    procState.priority = process->GetPriority();
    procState.pending_delete = process->IsDeletePending();
    procState.pending_replication = false; // obsolete field
    procState.state = process->GetState();
    procState.event_messages = process->IsEventMessages();
    procState.system_messages = process->IsSystemMessages();
    procState.paired = process->IsPaired();
    procState.waiting_startup = !process->IsStartupCompleted();
    procState.opened = process->IsOpened();
    procState.backup = process->IsBackup();
    procState.creation_time = process->GetCreationTime();
    parent = (process->GetParentNid() == -1 ? 
              NULL : 
              Nodes->GetLNode(process->GetParentNid())
                 ->GetProcessL(process->GetParentPid()));
    if (parent)
    {
        procState.parent_nid = parent->GetNid();
        procState.parent_pid = parent->GetPid();
        strcpy (procState.parent_name, parent->GetName());
    }
    else
    {
        procState.parent_nid = -1;
        procState.parent_pid = -1;
        procState.parent_name[0] = '\0';
    }
}


// Get a pointer to the first process object for node specified by
// "nid".  If there are no processes on that node and
// "getDataForAllNodes" is true, advance to subsequent nodes until
// find a process or run out of nodes.   Note thar parameter "nid"
// is a reference parameter because this method may change its value
// and the caller will need the new value.
CProcess * CExtProcInfoBase::ProcessInfo_GetProcess (int &nid, bool getDataForAllNodes)
{
    CProcess * process;
    CLNode *lnode = NULL;

    do
    {
        lnode = Nodes->GetLNode (nid);
        if ( lnode && 
             (lnode->GetState() == State_Up ||
              lnode->GetState() == State_Shutdown) )
        {
            process = lnode->GetFirstProcess();
            if (process != 0) return process;
        }

    } while (getDataForAllNodes && ++nid < Nodes->NumberLNodes);

    return 0;
}


// Information for more than one process is being requested.  Iterate
// through the process list and return process information for processes
// meeting the requested criteria.
int CExtProcInfoBase::ProcessInfo_BuildReply(CProcess *process,
                                     struct message_def * msg,
                                     PROCESSTYPE type,
                                     bool getDataForAllNodes)
{
    int currentNode = (process != 0) ? process->GetNid() : Nodes->NumberLNodes;
    bool moreToRetrieve;
    int count = 0;

    do
    {
        // Retrieve process data for processes on current node
        while ( process )
        {
            if (type == ProcessType_Undefined || type == process->GetType())
            {
                ProcessInfo_CopyData(process,
                                     msg->u.reply.u.process_info.process[count]);
                count++;

            }
            process = process->GetNextL();
            if ( count == MAX_PROCINFO_LIST )
            {
                // Ran out of room to store data.  Give caller and indication
                // of whether there is more data remaining.
                msg->u.reply.u.process_info.more_data
                    = (process != 0)
                    || (++currentNode < Nodes->NumberLNodes);
                return count;
            }
        }

        moreToRetrieve = false;
        if (getDataForAllNodes && ++currentNode < Nodes->NumberLNodes)
        {   // Start retrieving process data for next node.  We ask
            // ProcessInfo_GetProcess for the first process on
            // "currentNode" which has just been incremented.  Note
            // that it is possible there are no processes on that node
            // so ProcessInfo_GetProcess will return a process on the
            // first node it finds and "currentNode" will be updated
            // to be the node number where the process resides.

            process = ProcessInfo_GetProcess(currentNode, getDataForAllNodes);
            moreToRetrieve = true;
        }
    } while (moreToRetrieve);

    msg->u.reply.u.process_info.more_data = false;
    return count;
}

CExtProcInfoReq::CExtProcInfoReq (reqQueueMsg_t msgType, int pid,
                                  struct message_def *msg )
    : CExtProcInfoBase(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQEO", 4);
}

CExtProcInfoReq::~CExtProcInfoReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqeo", 4);
}

void CExtProcInfoReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf),
              "ExtReq(%s) req #=%ld requester(pid=%d) (name=%s/nid=%d/pid=%d)"
              , CReqQueue::svcReqType[reqType_], getId(), pid_,
              msg_->u.request.u.process_info.process_name,
              msg_->u.request.u.process_info.target_nid,
              msg_->u.request.u.process_info.target_pid );
    requestString_.assign( strBuf );
}

void CExtProcInfoReq::performRequest()
{
    const char method_name[] = "CExtProcInfoReq::performRequest";
    TRACE_ENTRY;

    CProcess *requester;
    int count = 0;

    // Record statistics (sonar counters)
       MonStats->req_type_processinfo_Incr();

    requester = MyNode->GetProcess( pid_ );
    if ( requester )
    {

#ifdef DELAY_TP
        Delay_TP("REQUESTDELAY_TP");
#endif

        msg_->u.reply.u.process_info.more_data = false;

        // setup for type of status request
        if ((*msg_->u.request.u.process_info.process_name) &&
            (msg_->u.request.u.process_info.target_pid == -1))
        {
            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            {
                trace_printf("%s@%d request #%ld: ProcessInfo, for %s, "
                             "process type=%d\n", method_name, __LINE__, id_,
                             msg_->u.request.u.process_info.process_name,
                             msg_->u.request.u.process_info.type);
            }

            //if the requester is tse or ASE, and it is requesting info for itself. monitor will always return local process info
            if ((requester->GetType() == ProcessType_TSE || requester->GetType() == ProcessType_ASE ||
                 requester->GetType() == ProcessType_AMP) &&
                strcmp(requester->GetName(),  msg_->u.request.u.process_info.process_name) == 0 )
            {
                ProcessInfo_CopyData(requester,
                                     msg_->u.reply.u.process_info.process[0]);
                count = 1;
            }
            else
            {  // get info for single named process
                CProcess *process;

                Nodes->GetLNode(msg_->u.request.u.process_info.process_name, &process, false);
                if (process)
                {
                    ProcessInfo_CopyData(process,
                                         msg_->u.reply.u.process_info.process[0]);
                    count = 1;
                }
            }
        }
        else if (msg_->u.request.u.process_info.target_nid == -1)
        {
            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            {
                trace_printf("%s@%d request #%ld: ProcessInfo, for all "
                             "processes\n", method_name, __LINE__, id_);
            }

            // get info for all processes in all nodes
            int nid = 0;
            count = ProcessInfo_BuildReply(ProcessInfo_GetProcess (nid, true),
                                           msg_,
                                           msg_->u.request.u.process_info.type,
                                           true);
        }
        else
        {
            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            {
                trace_printf("%s@%d request #%ld: ProcessInfo, for (%d, %d), "
                             "process type=%d\n", method_name, __LINE__, id_,
                             msg_->u.request.u.process_info.target_nid,
                             msg_->u.request.u.process_info.target_pid,
                             msg_->u.request.u.process_info.type);
            }

            int nid = msg_->u.request.u.process_info.target_nid;
            int pid = msg_->u.request.u.process_info.target_pid;
            if (pid == -1)
            {
                // get info for all processes in node
                if (nid >= 0 && nid < Nodes->NumberLNodes)
                {
                    count = ProcessInfo_BuildReply(Nodes->GetNode(nid)->GetFirstProcess(), 
                                                   msg_,
                                                   msg_->u.request.u.process_info.type,
                                                   false);
                }
            }
            else
            {
                // get info for single process in node
                if ((requester->GetType() == ProcessType_TSE
                     || requester->GetType() == ProcessType_ASE
                     || requester->GetType() == ProcessType_AMP)
                    &&
                    (requester->GetNid() == msg_->u.request.u.process_info.target_nid &&
                     requester->GetPid() == msg_->u.request.u.process_info.target_pid))
                {
                    ProcessInfo_CopyData(requester,
                                         msg_->u.reply.u.process_info.process[0]);
                    count = 1;
                }
                else if (nid >= 0 && nid < Nodes->NumberLNodes)
                {
                    CProcess *process;
                    process = Nodes->GetLNode(nid)->GetProcessL(pid);
                    if (process)
                    {
                        ProcessInfo_CopyData(process,
                                             msg_->u.reply.u.process_info.process[0]);
                        count = 1;
                    }
                }
            }
        }

        msg_->u.reply.type = ReplyType_ProcessInfo;
        msg_->u.reply.u.process_info.num_processes = count;
        msg_->u.reply.u.process_info.return_code = MPI_SUCCESS;

        // Send reply to requester
        lioreply(msg_, pid_);
    }
    else
    {   // Reply to requester so it can release the buffer.  
        // We don't know about this process.
        errorReply( MPI_ERR_EXITED );
    }

    TRACE_EXIT;
}

CExtProcInfoContReq::CExtProcInfoContReq (reqQueueMsg_t msgType, int pid,
                                          struct message_def *msg )
    : CExtProcInfoBase(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQEP", 4);
}

CExtProcInfoContReq:: ~CExtProcInfoContReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqep", 4);
}

void CExtProcInfoContReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf),
              "ExtReq(%s) req #=%ld requester(pid=%d)"
              , CReqQueue::svcReqType[reqType_], getId(), pid_ );
}


// An earlier ProcessInfo request returned as much data as would
// fit into the reply message.   Caller is now continuing the request
// to get additional data.void CExtProcInfoContReq::performRequest()
void CExtProcInfoContReq::performRequest()
{
    const char method_name[] = "CExtProcInfoContReq::performRequest";
    TRACE_ENTRY;

    int count = 0;
    int nid;
    int pid;

    // Record statistics (sonar counters)
       MonStats->req_type_processinfocont_Incr();

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf("%s@%d request #%ld: ProcessInfoCont, context (%d, %d), "
                     "process type=%d, allnodes=%d\n", method_name, __LINE__,
                     id_,
                     msg_->u.request.u.process_info_cont.context[0].nid,
                     msg_->u.request.u.process_info_cont.context[0].pid,
                     msg_->u.request.u.process_info_cont.type,
                     msg_->u.request.u.process_info_cont.allNodes);
    }

    msg_->u.reply.u.process_info.more_data = false;

    // Using context from the last reply, locate next process.
    // Generally the final process in the last reply will still exist
    // so we locate its CProcess object for continuation.  If that
    // process no longer exists we try to find other processes in the
    // context list until we find the CProcess object or run out of
    // context.
    int i = -1;
    CProcess *process = 0;

    while (!process && ++i < MAX_PROC_CONTEXT)
    {
        nid = msg_->u.request.u.process_info_cont.context[i].nid;
        pid = msg_->u.request.u.process_info_cont.context[i].pid;
        if (nid >= 0 && nid < Nodes->NumberLNodes)
        {
            process = Nodes->GetLNode(nid)->GetProcessL(pid);
        }
    }


    if (!process)
    {   // Could not locate any process in the context list.  So
        // begin with the first process in the node.
        nid = msg_->u.request.u.process_info_cont.context[0].nid;
        if (trace_settings & TRACE_REQUEST)
           trace_printf("%s@%d" " could not find context process, restarting for node="  "%d" "\n", method_name, __LINE__, nid);
        if (nid >= 0 && nid < Nodes->NumberLNodes)
        {
            process = ProcessInfo_GetProcess (nid, msg_->u.request.u.process_info_cont.allNodes);
        }
    }

    // Assuming we found a CProcess object resume returning data with
    // the subsequent process.
    if (process)
    {
        process = process->GetNextL();
        if (!process)
        {   // We were at the last process on the node.  Get first process
            // on the next node (if there is a next node).
            if (++nid < Nodes->NumberLNodes)
            {
                process = ProcessInfo_GetProcess(nid,
                                msg_->u.request.u.process_info_cont.allNodes);
            }
        }

        if (process)
        {
            count = ProcessInfo_BuildReply(
                                process,
                                msg_,
                                msg_->u.request.u.process_info_cont.type,
                                msg_->u.request.u.process_info_cont.allNodes);

        }
    }

    msg_->u.reply.type = ReplyType_ProcessInfo;
    msg_->u.reply.u.process_info.num_processes = count;
    msg_->u.reply.u.process_info.return_code = MPI_SUCCESS;

    // Send reply to requester
    lioreply(msg_, pid_);

    TRACE_EXIT;
}
