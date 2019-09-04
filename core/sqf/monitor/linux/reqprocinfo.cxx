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
#include <regex.h>
#include "reqqueue.h"
#include "montrace.h"
#include "monsonar.h"
#include "monlogging.h"
#include "nameserver.h"

extern CMonStats *MonStats;
#ifndef NAMESERVER_PROCESS
extern CNode *MyNode;
#endif
extern CNodeContainer *Nodes;
extern bool NameServerEnabled;
#ifndef NAMESERVER_PROCESS
extern CNameServer *NameServer;
#endif

extern const char *ProcessTypeString( PROCESSTYPE type );

// Copy information for a specific process into the reply message buffer.
void CExtProcInfoBase::ProcessInfo_CopyData(CProcess *process, ProcessInfoState &procState)
{
    const char method_name[] = "CExtProcInfoBase::ProcessInfo_CopyData";
    CProcess *parent;

    TRACE_ENTRY;

    procState.nid = process->GetNid();
    procState.pid = process->GetPid();
    procState.verifier = process->GetVerifier();
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
        procState.parent_verifier = parent->GetVerifier();
        strcpy (procState.parent_name, parent->GetName());
    }
    else
    {
        procState.parent_nid = -1;
        procState.parent_pid = -1;
        procState.parent_verifier = -1;
        procState.parent_name[0] = '\0';
    }

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        char desc[2048];
        char* descp = desc;
        sprintf( desc, 
                 "ProcessInfo reply:\n"
                 "        procState.process_name=%s\n"
                 "        procState.nid=%d\n"
                 "        procState.pid=%d\n"
                 "        procState.verifier=%d\n"
                 "        procState.type=%d\n"
                 "        procState.os_pid=%d\n"
                 "        procState.parent_name=%s\n"
                 "        procState.parent_nid=%d\n"
                 "        procState.parent_pid=%d\n"
                 "        procState.parent_verifier=%d\n"
                 "        procState.priority=%d\n"
                 "        procState.state=%d\n"
                 "        procState.pending_delete=%d\n"
                 "        procState.event_messages=%d\n"
                 "        procState.system_messages=%d\n"
                 "        procState.paired=%d\n"
                 "        procState.waiting_startup=%d\n"
                 "        procState.opened=%d\n"
                 "        procState.backup=%d\n"
                 "        procState.program=%s\n"
                 , procState.process_name
                 , procState.nid
                 , procState.pid
                 , procState.verifier
                 , procState.type
                 , procState.os_pid
                 , procState.parent_name
                 , procState.parent_nid
                 , procState.parent_pid
                 , procState.parent_verifier
                 , procState.priority
                 , procState.state
                 , procState.pending_delete
                 , procState.event_messages
                 , procState.system_messages
                 , procState.paired
                 , procState.waiting_startup
                 , procState.opened
                 , procState.backup
                 , procState.program );
        trace_printf( "%s@%d - %s\n"
                    , method_name, __LINE__, descp );
    }

    TRACE_EXIT;
}


void CExtProcInfoBase::ProcessInfo_CopyPairData( CProcess *process
                                               , ProcessInfoState &procState )
{
    CProcess *parent = (process->GetParentNid() == -1 ? 
                        NULL : 
                        Nodes->GetLNode(process->GetParentNid())
                            ->GetProcessL(process->GetParentPid()));
    if ( parent )
    {
        if ( process->IsBackup() || parent->IsBackup())
        {
            if ( parent->IsBackup())
            {
                ProcessInfo_CopyData( process, procState );
            }
            else
            {
                ProcessInfo_CopyData( parent, procState );
            }
        }
        else
        {
            ProcessInfo_CopyData( process, procState );
        }
    }
    else
    {
        ProcessInfo_CopyData( process, procState );
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
#ifndef NAMESERVER_PROCESS 
    const char method_name[] = "CExtProcInfoBase::ProcessInfo_GetProcess";
#endif

    CProcess * process;
    CLNode *lnode = NULL;

    lnode = Nodes->GetLNode( nid );
    do
    {
        if (lnode)
        {
#ifdef NAMESERVER_PROCESS // ignore node state
            process = lnode->GetFirstProcess();
            if (process != 0)
            {
                return process;
            }
#else
            if (lnode->GetState() == State_Up ||
                lnode->GetState() == State_Shutdown)
            {
                process = lnode->GetFirstProcess();
                if (process != 0)
                {
                    if (trace_settings & TRACE_PROCESS_DETAIL)
                    {
                        trace_printf( "%s@%d allNodes=%d, nid=%d, process: %s (%d,%d:%d)\n"
                                    , method_name, __LINE__
                                    , getDataForAllNodes
                                    , nid
                                    , process->GetName()
                                    , process->GetNid()
                                    , process->GetPid()
                                    , process->GetVerifier() );
                    }
                    return process;
                }
            }
#endif
            lnode = lnode->GetNext();
            nid = lnode ? lnode->GetNid() : nid;
        }
    } while (getDataForAllNodes && lnode);

    return(NULL);
}

// Information for more than one process is being requested.  Iterate
// through the process list and return process information for processes
// meeting the requested criteria.
int CExtProcInfoBase::ProcessInfo_BuildReply(CProcess *process,
                                     struct message_def * msg,
                                     PROCESSTYPE type,
                                     bool getDataForAllNodes,
                                     char *pattern)
{
    const char method_name[] = "CExtProcInfoBase::ProcessInfo_BuildReply";

    int currentIndex = (process != 0) 
            ? Nodes->GetNidIndex( process->GetNid() )
            : Nodes->GetLNodesCount();
    bool moreToRetrieve;
    bool copy = true;
    bool reg = false;
    int count = 0;
    int rerr;
    char *process_pattern = pattern;
    regex_t regex;

    if (strlen( pattern ) > 0)
    {
        if (*process_pattern == '$')
        {
            process_pattern++;
        }
        // Compile pattern regex
        rerr = regcomp( &regex, process_pattern, REG_EXTENDED );
        if (rerr == 0)
        {
            copy = false;
            reg = true;
        }
    }
    do
    {
        // Retrieve process data for processes on current node
        while ( process )
        {
            if (trace_settings & TRACE_PROCESS_DETAIL)
            {
                trace_printf( "%s@%d allNodes=%d, pattern=%s, type=%s, process: %s (%d,%d:%d)\n"
                            , method_name, __LINE__
                            , getDataForAllNodes
                            , (strlen( pattern ))?process_pattern: ""
                            , ProcessTypeString(type)
                            , process->GetName()
                            , process->GetNid()
                            , process->GetPid()
                            , process->GetVerifier() );
            }
            if (type == ProcessType_Undefined || type == process->GetType())
            {
                if (reg)
                {
                    // Check for regex match
                    rerr  = regexec( &regex, process->GetName(), 0 , NULL, 0 );
                    copy = (rerr == 0) ? true : false;
                }
                if (copy)
                {
                    ProcessInfo_CopyData(process,
                                         msg->u.reply.u.process_info.process[count]);
                    count++;
                }
            }
            process = process->GetNextL();
            if ( count == MAX_PROCINFO_LIST )
            {
                // Ran out of room to store data.  Give caller and indication
                // of whether there is more data remaining.
                msg->u.reply.u.process_info.more_data
                    = (process != 0)
                    || (++currentIndex < Nodes->GetLNodesCount());
                return count;
            }
        }

        moreToRetrieve = false;
        if (getDataForAllNodes && ++currentIndex < Nodes->GetLNodesCount())
        {   // Start retrieving process data for next node.  We ask
            // ProcessInfo_GetProcess for the first process on lnode of
            // "currentIndex" which has just been incremented.  Note
            // that it is possible there are no processes on that node
            // so ProcessInfo_GetProcess will return a process on the
            // first node it finds and "currentIndex" will be updated
            // to be the node index number where the process resides.

            int nid = Nodes->GetNidByMap( currentIndex );
            if (trace_settings & TRACE_PROCESS_DETAIL)
            {
                trace_printf( "%s@%d moreToRetrieve=%d, nid=%d\n"
                            , method_name, __LINE__
                            , moreToRetrieve
                            , nid );
            }
            if (nid == -1) break;
            if (trace_settings & TRACE_PROCESS_DETAIL)
            {
                trace_printf( "%s@%d allNodes=%d, nid=%d\n"
                            , method_name, __LINE__
                            , getDataForAllNodes
                            , nid );
            }
            process = ProcessInfo_GetProcess( nid, getDataForAllNodes);
            currentIndex = Nodes->GetNidIndex( nid );
            if (process && trace_settings & TRACE_PROCESS_DETAIL)
            {
                trace_printf( "%s@%d currentIndex=%d, next process: %s (%d,%d:%d)\n"
                            , method_name, __LINE__
                            , currentIndex
                            , process->GetName()
                            , process->GetNid()
                            , process->GetPid()
                            , process->GetVerifier() );
            }
            moreToRetrieve = true;
        }
    } while (moreToRetrieve);

    if (reg)
    {
        regfree( &regex );
    }

    msg->u.reply.u.process_info.more_data = false;
    return count;
}

CExtProcInfoReq::CExtProcInfoReq (reqQueueMsg_t msgType,
                                  int nid, int pid, int sockFd,
                                  struct message_def *msg )
    : CExtProcInfoBase(msgType, nid, pid, sockFd, msg)
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
    char strBuf[MON_STRING_BUF_SIZE] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld "
              "requester(name=%s/nid=%d/pid=%d/os_pid=%d/verifier=%d) "
              "target(name=%s/nid=%d/pid=%d/verifier=%d) pattern(name=%s)"
            , CReqQueue::svcReqType[reqType_], getId()
            , msg_->u.request.u.process_info.process_name
            , msg_->u.request.u.process_info.nid
            , msg_->u.request.u.process_info.pid
            , pid_
            , msg_->u.request.u.process_info.verifier
            , msg_->u.request.u.process_info.target_process_name
            , msg_->u.request.u.process_info.target_nid
            , msg_->u.request.u.process_info.target_pid
            , msg_->u.request.u.process_info.target_verifier
            , msg_->u.request.u.process_info.target_process_pattern );
    requestString_.assign( strBuf );
}

void CExtProcInfoReq::performRequest()
{
    const char method_name[] = "CExtProcInfoReq::performRequest";
    TRACE_ENTRY;

#ifndef NAMESERVER_PROCESS
    bool    getMonitorInfo = false;
    if (strcasecmp(msg_->u.request.u.process_info.target_process_name, "MONITOR") == 0)
    {
        getMonitorInfo = true;
        msg_->u.request.u.process_info.target_process_name[0] = 0;
    }

    if ( NameServerEnabled && !getMonitorInfo )
    {
        int rc = NameServer->ProcessInfo(msg_); // in reqQueue thread (CExternalReq)
        if (rc)
        {
            char la_buf[MON_STRING_BUF_SIZE];
            snprintf( la_buf, sizeof(la_buf)
                    , "[%s] - Process info request to Name Server failed\n"
                    , method_name );
            mon_log_write(MON_REQ_PROCINFO_1, SQ_LOG_ERR, la_buf);
        }
    }
#endif

#ifndef NAMESERVER_PROCESS
    if ( NameServerEnabled && !getMonitorInfo )
    {
        // Send reply to requester
        lioreply(msg_, pid_);
    }
    else
    {
#endif
        int count = 0;

        // Record statistics (sonar counters)
        if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
           MonStats->req_type_processinfo_Incr();

        nid_ = msg_->u.request.u.process_info.nid;
        verifier_ = msg_->u.request.u.process_info.verifier;
        processName_ = msg_->u.request.u.process_info.process_name;

        int       target_nid = -1;
        int       target_pid = -1;
        string    target_process_name;
        Verifier_t target_verifier = -1;
        CProcess *requester = NULL;

        target_nid = msg_->u.request.u.process_info.target_nid;
        target_pid = msg_->u.request.u.process_info.target_pid;
        target_process_name = (const char *) msg_->u.request.u.process_info.target_process_name;
        target_verifier  = msg_->u.request.u.process_info.target_verifier;

        if ( processName_.size() )
        { // find by name
#ifdef NAMESERVER_PROCESS
            //  (check node state, check process state, not backup)
            requester = Nodes->GetProcess( processName_.c_str()
                                         , verifier_ );
#else
            requester = MyNode->GetProcess( processName_.c_str()
                                          , verifier_ );
#endif
        }
        else
        { // find by pid
#ifdef NAMESERVER_PROCESS
            //  (don't check node state, don't check process state, backup is Ok)
            requester =
               Nodes->GetProcess( nid_ , pid_ , verifier_
                                , false, false, true );
#else
            requester = MyNode->GetProcess( pid_
                                          , verifier_ );
#endif
        }

#ifdef NAMESERVER_PROCESS
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf( "%s@%d tnid=%d, tpid=%d, tver=%d, tpname=%s, requester=%p\n"
                        , method_name, __LINE__
                        , target_nid, target_pid, target_verifier, target_process_name.c_str()
                        , (void *) requester );
        }
#endif

#ifdef NAMESERVER_PROCESS
        if ( requester || ( nid_ == -1 && pid_ == -1 && verifier_ == -1 ) )
#else
        if ( requester )
#endif
        {
            msg_->u.reply.u.process_info.more_data = false;

            // setup for type of status request
            if ( target_process_name.size() )
            { // find by name 
                if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                {
                    trace_printf( "%s@%d request #%ld: ProcessInfo from %s (%d, %d:%d) "
                                  "by name for %s:%d, process type=%d\n"
                                , method_name, __LINE__, id_
                                , requester?requester->GetName():""
                                , requester?requester->GetNid():-1
                                , requester?requester->GetPid():-1
                                , requester?requester->GetVerifier():-1
                                , target_process_name.c_str(), target_verifier
                                , msg_->u.request.u.process_info.type);
                }

                //if requester is requesting info for itself, return local process info
                if ( requester && strcmp( requester->GetName()
                           , msg_->u.request.u.process_info.target_process_name) == 0 )
                {
                    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                    {
                        trace_printf("%s@%d request #%ld: ProcessInfo, for "
                                     "requester\n", method_name, __LINE__, id_);
                    }
                    ProcessInfo_CopyData(requester,
                                         msg_->u.reply.u.process_info.process[0]);
                    count = 1;
                }
                else
                {
#ifdef NAMESERVER_PROCESS
                    // find by name (don't check node state, don't check process state, 
                    //               if verifier is -1, backup is NOT Ok, else is Ok)
                    CProcess *process = Nodes->GetProcess( target_process_name.c_str()
                                                         , target_verifier
                                                         , false, false
                                                         , target_verifier == -1 ? false : true );
#else
                    CProcess *process = NULL;
                    // find by name (check node state, don't check process state, 
                    //               if verifier is -1, backup is NOT Ok, else is Ok)
                    if (msg_->u.request.u.process_info.target_process_name[0] == '$' )
                    {
                        process = Nodes->GetProcess( target_process_name.c_str()
                                                   , target_verifier
                                                   , true, false
                                                   , target_verifier == -1 ? false : true );
                    }
#endif
                    if (process)
                    {
                        if ( target_verifier == -1 )
                        { // the name may represent process pair, return primary only
                            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                            {
                                trace_printf("%s@%d request #%ld: ProcessInfo, for "
                                             "process pair\n", method_name, __LINE__, id_);
                            }
                            ProcessInfo_CopyPairData( process
                                                    , msg_->u.reply.u.process_info.process[0] );
                            count = 1;

                        }
                        else
                        { 
                            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                            {
                                trace_printf("%s@%d request #%ld: ProcessInfo, for "
                                             "process\n", method_name, __LINE__, id_);
                            }
                            ProcessInfo_CopyData( process
                                                , msg_->u.reply.u.process_info.process[0] );
                            count = 1;
                        }
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
                int nid = Nodes->GetFirstNid();
                count = ProcessInfo_BuildReply( ProcessInfo_GetProcess(nid, true)
                                              , msg_
                                              , msg_->u.request.u.process_info.type
                                              , true
                                              , msg_->u.request.u.process_info.target_process_pattern );
            }
            else
            {
                if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                {
                    trace_printf( "%s@%d request #%ld: ProcessInfo, for (%d, %d:%d), "
                                  "process type=%d\n"
                                , method_name, __LINE__, id_
                                , target_nid, target_pid, target_verifier
                                , msg_->u.request.u.process_info.type);
                }

                if (target_pid == -1)
                {
                    // get info for all processes in node
                    if (target_nid >= 0 && target_nid < Nodes->GetLNodesConfigMax())
                    {
                        count = ProcessInfo_BuildReply(Nodes->GetNode(target_nid)->GetFirstProcess(), 
                                                       msg_,
                                                       msg_->u.request.u.process_info.type,
                                                       false,
                                                       msg_->u.request.u.process_info.target_process_pattern);
                    }
                }
                else
                {
                    // get info for single process in node
                    if (requester &&
                        (requester->GetType() == ProcessType_TSE ||
                         requester->GetType() == ProcessType_ASE ||
                         requester->GetType() == ProcessType_AMP)  &&
                        (requester->GetNid() == target_nid &&
                         requester->GetPid() == target_pid))
                    {
                        ProcessInfo_CopyData(requester,
                                             msg_->u.reply.u.process_info.process[0]);
                        count = 1;
                    }
                    else if (target_nid >= 0 && target_nid < Nodes->GetLNodesConfigMax())
                    { // find by nid/pid (check node state, don't check process state, backup is Ok)
                        CProcess *process = Nodes->GetProcess( target_nid
                                                             , target_pid
                                                             , target_verifier
                                                             , true, false, true );
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

#ifdef NAMESERVER_PROCESS
            monreply(msg_, sockFd_);
#else
            // Send reply to requester
            lioreply(msg_, pid_);
#endif
        }
        else
        {   // Reply to requester so it can release the buffer.  
            // We don't know about this process.
            errorReply( MPI_ERR_EXITED );
        }
#ifndef NAMESERVER_PROCESS
    }
#endif

    TRACE_EXIT;
}

CExtProcInfoContReq::CExtProcInfoContReq (reqQueueMsg_t msgType,
                                          int nid, int pid, int sockFd,
                                          struct message_def *msg )
    : CExtProcInfoBase(msgType, nid, pid, sockFd, msg)
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

#ifndef NAMESERVER_PROCESS
    bool    getMonitorInfo = false;
    if (strcasecmp(msg_->u.request.u.process_info.target_process_name, "MONITOR") == 0)
    {
        getMonitorInfo = true;
        msg_->u.request.u.process_info.target_process_name[0] = 0;
    }

    if ( NameServerEnabled && !getMonitorInfo )
    {
        int rc = NameServer->ProcessInfoCont(msg_); // in reqQueue thread (CExternalReq)
        if (rc)
        {
            char la_buf[MON_STRING_BUF_SIZE];
            snprintf( la_buf, sizeof(la_buf)
                    , "[%s] - Process info continue request to Name Server failed\n"
                    , method_name );
            mon_log_write(MON_REQ_PROCINFOCONT_1, SQ_LOG_ERR, la_buf);
        }
    }
#endif

#ifndef NAMESERVER_PROCESS
    if ( NameServerEnabled && !getMonitorInfo )
    {
        // Send reply to requester
        lioreply(msg_, pid_);
    }
    else
    {
#endif
        int count = 0;
        int nid;
        int pid;

        // Record statistics (sonar counters)
        if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
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
            if (nid >= 0 && nid < Nodes->GetLNodesConfigMax())
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
            if (nid >= 0 && nid < Nodes->GetLNodesConfigMax())
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
                if (++nid < Nodes->GetLNodesConfigMax())
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
                                    msg_->u.request.u.process_info_cont.allNodes,
                                    (char *) "");

            }
        }

        msg_->u.reply.type = ReplyType_ProcessInfo;
        msg_->u.reply.u.process_info.num_processes = count;
        msg_->u.reply.u.process_info.return_code = MPI_SUCCESS;

#ifdef NAMESERVER_PROCESS
        monreply(msg_, sockFd_);
#else
        // Send reply to requester
        lioreply(msg_, pid_);
#endif
#ifndef NAMESERVER_PROCESS
    }
#endif

    TRACE_EXIT;
}
