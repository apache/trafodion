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

#include "nstype.h"

#include "process.cxx"

CProcess *CProcessContainer::CreateProcess (CProcess * parent,
                                            int nid,
                                            int pid,
                                            Verifier_t verifier,
                                            bool event_messages,
                                            bool system_messages,
                                            PROCESSTYPE type,
                                            int debug,
                                            int priority,
                                            int backup,
                                            bool unhooked,
                                            char *process_name,
                                            char *path, 
                                            char *ldpath, 
                                            char *program, 
//                                            strId_t pathStrId, 
//                                            strId_t ldpathStrId,
//                                            strId_t programStrId,
                                            char *infile,
                                            char *outfile,
                                            void *tag,
                                            int &result)
{
    const char method_name[] = "CProcessContainer::CreateProcess";
    TRACE_ENTRY;

    CProcess *process = NULL;

    tag = tag; // Satisfy compiler
    result = MPI_SUCCESS;

    // load & normalize process name
    if( process_name[0] != '\0' )
    {
        NormalizeName (process_name);
    }

    process =
        new CProcess( parent
                    , nid
                    , pid
                    , verifier
                    , type
                    , priority
                    , backup
                    , debug
                    , unhooked
                    , process_name
                    , path
                    , ldpath
                    , program
//                    , pathStrId
//                    , ldpathStrId
//                    , programStrId
                    , infile
                    , outfile);
    if (process)
    {
        AddToList( process );
        char port[1];
        port[0] = '\0';
        struct timespec creation_time;
        memset(&creation_time, 0, sizeof(creation_time));
        process->SetOrigPNidNs( MyPNID );
        process->CompleteProcessStartup (port, pid, event_messages, system_messages, false, &creation_time, MyPNID);
    }

    TRACE_EXIT;

    return process;
}

void CProcess::CompleteProcessStartup (char *port, int os_pid, bool event_messages,
                                       bool system_messages, bool preclone,
                                       struct timespec *creation_time,
                                       int origPNidNs)
{
    const char method_name[] = "CProcess::CompleteProcessStartup";
    TRACE_ENTRY;

    STRCPY (Port, port);
    Pid = os_pid;
    Event_messages = event_messages;
    System_messages = system_messages;
    origPNidNs_ = origPNidNs;
    State_ = State_Up;

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL | TRACE_REQUEST_DETAIL))
        trace_printf("%s@%d: process %s (%d, %d), preclone=%d"
                     ", clone=%d, origPNidNs=%d, MyPNID=%d\n",
                     method_name, __LINE__, Name,
                     Nid, os_pid, preclone, Clone, origPNidNs, MyPNID );
    StartupCompleted = true;
    if (creation_time != NULL)
        CreationTime = *creation_time;

    if ( MyPNID != GetOrigPNidNs() )
    {
        Clone = true;
    }

    TRACE_EXIT;
}
