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

#include <iostream>

using namespace std;

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include "lnode.h"
#include "pnode.h"
#include "ptpclient.h"
#include "monitor.h"
#include "monlogging.h"
#include "montrace.h"
#include "meas.h"

extern CMonitor *Monitor;
extern CNode *MyNode;
extern CNodeContainer *Nodes;
extern bool IsRealCluster;
extern CMeas Meas;
extern int MyPNID;

#define MON2MON_IO_RETRIES 3

CPtpClient::CPtpClient (void)
          : ptpCommPort_(0)
          , ptpClusterSocks_(NULL)
          , seqNum_(0)
{
    const char method_name[] = "CPtpClient::CPtpClient";
    TRACE_ENTRY;

    ptpHost_[0] = '\0';
    ptpPortBase_[0] = '\0';
    if ( !IsRealCluster )
    {
        SetLocalHost();
    }
    
    char * env  = getenv( "MON2MON_COMM_PORT" );
    if ( env  ) 
    {
        ptpCommPort_ = atoi( env  );
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s@%d] MON2MON_COMM_PORT environment variable is not set!\n"
                , method_name, __LINE__ );
        mon_log_write( PTPCLIENT_PTPCLIENT_1, SQ_LOG_CRIT, buf );

        mon_failure_exit();
    }

    ptpClusterSocks_ = new int[MAX_NODES];
    for (int i=0; i < MAX_NODES; ++i)
    {
        ptpClusterSocks_[i] = -1;
    }

    TRACE_EXIT;
}

CPtpClient::~CPtpClient (void)
{
    const char method_name[] = "CPtpClient::~CPtpClient";
    TRACE_ENTRY;

    delete [] ptpClusterSocks_;

    TRACE_EXIT;
}

int CPtpClient::InitializePtpClient( int pnid, char * ptpPort )
{
    const char method_name[] = "CPtpClient::InitializePtpClient";
    TRACE_ENTRY;
    int err = 0;

    if (ptpClusterSocks_[pnid] == -1)
    {
        int sock = Monitor->MkCltSock( ptpPort );                
        if (sock < 0)
        {
            err = sock;
            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            {
                trace_printf( "%s@%d - MkCltSock failed with error %d\n"
                            , method_name, __LINE__, err );
            }
        }
        else
        {
            ptpClusterSocks_[pnid] = sock;
            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            {
                trace_printf( "%s@%d - connected to monitor node=%d(%s), sock=%d, "
                              "ptpClusterSocks_[%d]=%d\n"
                            , method_name, __LINE__
                            , pnid
                            , ptpPort
                            , sock
                            , pnid
                            , ptpClusterSocks_[pnid] );
            }
        }
    }

    TRACE_EXIT;
    return err;
}

bool CPtpClient::IsTargetRemote( int targetNid )
{
    const char method_name[] = "CPtpClient::IsTargetRemote";
    TRACE_ENTRY;

    CLNode *targetLNode = Nodes->GetLNode( targetNid );
    CNode *targetNode = targetLNode->GetNode();
    bool rs = (targetNode && targetNode->GetPNid() == MyPNID) ? false : true ;

    TRACE_EXIT;
    return(rs);
}

int  CPtpClient::ProcessAddUniqStr( int nid
                                  , int id
                                  , const char *stringValue
                                  , int targetNid
                                  , const char *targetNodeName )
{
    const char method_name[] = "CPtpClient::ProcessAddUniqStr";
    TRACE_ENTRY;

    if (!IsTargetRemote( targetNid ))
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf( "%s@%d - Not Sending InternalType_UniqStr request to "
                          "local nid=%d\n"
                        , method_name, __LINE__
                        , targetNid );
        }
        return(0);
    }

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d - Sending InternalType_UniqStr request to %s, "
                      "targetNid=%d\n"
                    , method_name, __LINE__
                    , targetNodeName
                    , targetNid );
    }

    struct internal_msg_def msg;
    memset(&msg, 0, sizeof(msg)); 
    msg.type = InternalType_UniqStr;
    msg.u.uniqstr.nid = nid;
    msg.u.uniqstr.id  = id;

    char *stringData = & msg.u.uniqstr.valueData;
    int  stringDataLen = strlen(stringValue) + 1;

    // Copy the string
    memcpy( stringData, stringValue, stringDataLen );

    ptpMsgInfo_t myInfo;
    myInfo.pnid = MyPNID;
    myInfo.size = offsetof(struct internal_msg_def, u);
    myInfo.size += sizeof(msg.u.uniqstr);
    myInfo.size += stringDataLen;

    if (trace_settings & TRACE_PROCESS_DETAIL)
    {
        trace_printf( "%s@%d - size_=%d, forwarding unique string [%d, %d] (%s)\n"
                    , method_name, __LINE__
                    , myInfo.size
                    , msg.u.uniqstr.nid
                    , msg.u.uniqstr.id
                    , &msg.u.uniqstr.valueData  );
    }

    int error = SendToMon( "process-add-unique-string"
                         , &msg
                         , myInfo
                         , targetNid
                         , targetNodeName);
    
    TRACE_EXIT;
    return error;
}

int CPtpClient::ProcessClone( CProcess *process )
{
    const char method_name[] = "CPtpClient::ProcessClone";
    TRACE_ENTRY;

    CLNode *parentLNode = NULL;
    if (process->GetParentNid() != -1)
    {
        parentLNode = Nodes->GetLNode( process->GetParentNid() );
    }

    if (parentLNode == NULL)
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf( "%s@%d - Not Sending InternalType_Clone request to parentNid=%d"
                          ", process=%s (%d:%d:%d)\n"
                        , method_name, __LINE__
                        , process->GetParentNid()
                        , process->GetName()
                        , process->GetNid()
                        , process->GetPid()
                        , process->GetVerifier() );
        }
        return(0);
    }

    if (!IsTargetRemote( process->GetParentNid() ))
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf( "%s@%d - Not Sending InternalType_Clone request to "
                          "local nid=%d\n"
                        , method_name, __LINE__
                        , process->GetParentNid() );
        }
        return(0);
    }

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d - Sending InternalType_Clone request to %s, parentNid=%d"
                      ", process=%s (%d:%d:%d)\n"
                    , method_name, __LINE__
                    , parentLNode->GetNode()->GetName()
                    , process->GetParentNid()
                    , process->GetName()
                    , process->GetNid()
                    , process->GetPid()
                    , process->GetVerifier() );
    }

    struct internal_msg_def msg;
    memset(&msg, 0, sizeof(msg)); 
    msg.type = InternalType_Clone;
    msg.u.clone.backup = process->IsBackup();
    msg.u.clone.unhooked = process->IsUnhooked();
    msg.u.clone.event_messages = process->IsEventMessages();
    msg.u.clone.system_messages = process->IsSystemMessages();
    msg.u.clone.nid = process->GetNid();
    msg.u.clone.verifier = process->GetVerifier();
    msg.u.clone.type = process->GetType();
    msg.u.clone.priority = process->GetPriority();
    msg.u.clone.parent_nid = process->GetParentNid();
    msg.u.clone.parent_pid = process->GetParentPid();
    msg.u.clone.parent_verifier = process->GetParentVerifier();
    msg.u.clone.os_pid = process->GetPid();
    msg.u.clone.persistent = process->IsPersistent();
    msg.u.clone.persistent_retries = process->GetPersistentRetries();
    msg.u.clone.origPNidNs= -1;
    msg.u.clone.argc = process->argc();
    msg.u.clone.creation_time = process->GetCreationTime();
    msg.u.clone.pathStrId = process->pathStrId();
    msg.u.clone.ldpathStrId = process->ldPathStrId();
    msg.u.clone.programStrId = process->programStrId();

    msg.u.clone.prior_pid = process->GetPriorPid ();
    process->SetPriorPid ( 0 );
    msg.u.clone.creation_time = process->GetCreationTime();

    char *stringData = & msg.u.clone.stringData;
    int  nameLen = strlen(process->GetName()) + 1;
    int  portLen = strlen(process->GetPort()) + 1;
    int  infileLen = strlen(process->infile()) + 1;
    int  outfileLen = strlen(process->outfile()) + 1;
    int  argvLen = process->userArgvLen();

    // Copy the process name
    msg.u.clone.nameLen = nameLen;
    memcpy( stringData, process->GetName(), nameLen );
    stringData += nameLen;

    // Copy the port
    msg.u.clone.portLen = portLen;
    memcpy(stringData, process->GetPort(),  portLen );
    stringData += portLen;

    // Copy the standard in file name
    msg.u.clone.infileLen = infileLen;
    memcpy( stringData, process->infile(), infileLen );
    stringData += infileLen ;

    // Copy the standard out file name
    msg.u.clone.outfileLen = outfileLen;
    memcpy( stringData, process->outfile(), outfileLen  );
    stringData += outfileLen ;

    // Copy the program argument strings
    msg.u.clone.argvLen =  argvLen;
    memcpy( stringData, process->userArgv(), argvLen );

    ptpMsgInfo_t myInfo;
    myInfo.pnid = MyPNID;
    myInfo.size = offsetof(struct internal_msg_def, u);
    myInfo.size += sizeof(msg.u.clone);
    myInfo.size += nameLen ;
    myInfo.size += portLen ;
    myInfo.size += infileLen ;
    myInfo.size += outfileLen ;
    myInfo.size += argvLen ;
    
    if (trace_settings & TRACE_PROCESS_DETAIL)
    {
        trace_printf( "%s@%d - size_=%d, programStrId=(%d,%d), "
                      "pathStrId=(%d,%d), ldPathStrId=(%d,%d), "
                      "name=%s, strlen(name)=%d, "
                      "port=%s, strlen(port)=%d, "
                      "infile=%s, strlen(infile)=%d, "
                      "outfile=%s, strlen(outfile)=%d, "
                      "argc=%d, strlen(total argv)=%d, args=[%.*s]\n"
                    , method_name, __LINE__
                    , myInfo.size
                    , msg.u.clone.programStrId.nid
                    , msg.u.clone.programStrId.id
                    , msg.u.clone.pathStrId.nid
                    , msg.u.clone.pathStrId.id
                    , msg.u.clone.ldpathStrId.nid
                    , msg.u.clone.ldpathStrId.id
                    , &msg.u.clone.stringData
                    , nameLen
                    , &msg.u.clone.stringData+nameLen
                    , portLen
                    , &msg.u.clone.stringData+nameLen
                    , infileLen
                    , &msg.u.clone.stringData+nameLen+infileLen
                    , outfileLen 
                    , msg.u.clone.argc
                    , argvLen
                    , argvLen
                    , &msg.u.clone.stringData+nameLen+infileLen+outfileLen);
    }

    int error = SendToMon( "process-clone"
                         , &msg
                         , myInfo
                         , process->GetParentNid()
                         , parentLNode->GetNode()->GetName());
    
    TRACE_EXIT;
    return error;
}

int CPtpClient::ProcessDump( CProcess *process )
{
    const char method_name[] = "CPtpClient::ProcessDump";
    TRACE_ENTRY;

    if (!IsTargetRemote( process->GetNid() ))
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf( "%s@%d - Not Sending InternalType_Dump request to "
                          "local nid=%d\n"
                        , method_name, __LINE__
                        , process->GetNid() );
        }
        return(0);
    }

    int targetNid = process->GetNid();
    CNode *targetNode = Nodes->GetLNode(targetNid)->GetNode();

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d - Sending InternalType_Dump request to %s, targetPNid=%d"
                      ", target process=%s (%d,%d:%d)\n"
                    , method_name, __LINE__
                    , targetNode?targetNode->GetName():""
                    , targetNode?targetNode->GetPNid():-1
                    , process->GetName()
                    , process->GetNid()
                    , process->GetPid()
                    , process->GetVerifier() );
    }

    struct internal_msg_def msg;
    memset(&msg, 0, sizeof(msg)); 
    msg.type = InternalType_Dump;
    msg.u.dump.nid = process->GetNid();
    msg.u.dump.pid = process->GetPid();
    msg.u.dump.verifier = process->GetVerifier();
    msg.u.dump.dumper_nid = process->GetDumperNid();
    msg.u.dump.dumper_pid = process->GetDumperPid();
    msg.u.dump.dumper_verifier = process->GetDumperVerifier();
    strcpy(msg.u.dump.core_file, process->GetDumpFile());

    ptpMsgInfo_t myInfo;
    myInfo.pnid = MyPNID;
    myInfo.size = offsetof(struct internal_msg_def, u);
    myInfo.size += sizeof(msg.u.dump);
    
    if (trace_settings & TRACE_PROCESS_DETAIL)
    {
        trace_printf( "%s@%d - size_=%d, process %s (%d,%d:%d), "
                      "dumper (%d,%d:%d), core_file=%s\n"
                    , method_name, __LINE__
                    , myInfo.size
                    , process->GetName()
                    , msg.u.dump.nid
                    , msg.u.dump.pid
                    , msg.u.dump.verifier
                    , msg.u.dump.dumper_nid
                    , msg.u.dump.dumper_pid
                    , msg.u.dump.dumper_verifier
                    , msg.u.dump.core_file );
    }

    int error = SendToMon( "process-dump"
                         , &msg
                         , myInfo
                         , targetNid
                         , targetNode->GetName() );
    
    TRACE_EXIT;
    return error;
}

int CPtpClient::ProcessDumpComplete( CProcess *process )
{
    const char method_name[] = "CPtpClient::ProcessDumpComplete";
    TRACE_ENTRY;

    if (!IsTargetRemote( process->GetDumperNid() ))
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf( "%s@%d - Not Sending InternalType_Dump request to "
                          "local nid=%d\n"
                        , method_name, __LINE__
                        , process->GetDumperNid() );
        }
        return(0);
    }

    int targetNid = process->GetDumperNid();
    CNode *targetNode = Nodes->GetLNode(targetNid)->GetNode();

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d - Sending InternalType_DumpComplete reply to %s, targetPNid=%d"
                      ", dumper process (%d,%d:%d)\n"
                    , method_name, __LINE__
                    , targetNode?targetNode->GetName():""
                    , targetNode?targetNode->GetPNid():-1
                    , process->GetDumperNid()
                    , process->GetDumperPid()
                    , process->GetDumperVerifier() );
    }

    struct internal_msg_def msg;
    memset(&msg, 0, sizeof(msg)); 
    msg.type = InternalType_DumpComplete;
    msg.u.dump.nid = process->GetNid();
    msg.u.dump.pid = process->GetPid();
    msg.u.dump.verifier = process->GetVerifier();
    msg.u.dump.dumper_nid = process->GetDumperNid();
    msg.u.dump.dumper_pid = process->GetDumperPid();
    msg.u.dump.dumper_verifier = process->GetDumperVerifier();
    strcpy(msg.u.dump.core_file, process->GetDumpFile());

    ptpMsgInfo_t myInfo;
    myInfo.pnid = MyPNID;
    myInfo.size = offsetof(struct internal_msg_def, u);
    myInfo.size += sizeof(msg.u.dump);
    
    if (trace_settings & TRACE_PROCESS_DETAIL)
    {
        trace_printf( "%s@%d - size_=%d, process %s (%d,%d:%d), "
                      "dumper (%d,%d:%d), core_file=%s\n"
                    , method_name, __LINE__
                    , myInfo.size
                    , process->GetName()
                    , msg.u.dump.nid
                    , msg.u.dump.pid
                    , msg.u.dump.verifier
                    , msg.u.dump.dumper_nid
                    , msg.u.dump.dumper_pid
                    , msg.u.dump.dumper_verifier
                    , msg.u.dump.core_file );
    }

    int error = SendToMon( "process-dump-complete"
                         , &msg
                         , myInfo
                         , targetNid
                         , targetNode->GetName() );
    
    TRACE_EXIT;
    return error;
}

int CPtpClient::ProcessExit( CProcess *process
                           , int targetNid
                           , const char *targetNodeName )
{
    const char method_name[] = "CPtpClient::ProcessExit";
    TRACE_ENTRY;

    if (!IsTargetRemote( targetNid ))
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf( "%s@%d - Not Sending InternalType_Exit request to "
                          "local nid=%d\n"
                        , method_name, __LINE__
                        , targetNid );
        }
        return(0);
    }

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d - Sending InternalType_Exit request to %s, targetNid=%d"
                      ", process=%s (%d,%d:%d) is exiting\n"
                    , method_name, __LINE__
                    , targetNodeName
                    , targetNid
                    , process->GetName()
                    , process->GetNid()
                    , process->GetPid()
                    , process->GetVerifier() );
    }

    struct internal_msg_def msg;
    memset(&msg, 0, sizeof(msg)); 
    msg.type = InternalType_Exit;
    msg.u.exit.nid = process->GetNid();
    msg.u.exit.pid = process->GetPid();
    msg.u.exit.verifier = process->GetVerifier();
    strcpy(msg.u.exit.name, process->GetName());
    msg.u.exit.abended = process->IsAbended();

    ptpMsgInfo_t myInfo;
    myInfo.pnid = MyPNID;
    myInfo.size = offsetof(struct internal_msg_def, u);
    myInfo.size += sizeof(msg.u.exit);
    
    if (trace_settings & TRACE_PROCESS_DETAIL)
    {
        trace_printf( "%s@%d - size_=%d, process %s (%d,%d:%d) "
                      "abended=%d\n"
                    , method_name, __LINE__
                    , myInfo.size
                    , msg.u.exit.name
                    , msg.u.exit.nid
                    , msg.u.exit.pid
                    , msg.u.exit.verifier
                    , msg.u.exit.abended );
    }

    int error = SendToMon( "process-exit"
                         , &msg
                         , myInfo
                         , targetNid
                         , targetNodeName);
    
    TRACE_EXIT;
    return error;
}

int CPtpClient::ProcessInit( CProcess *process
                           , void *tag
                           , int result
                           , int parentNid )
{
    const char method_name[] = "CPtpClient::ProcessInit";
    TRACE_ENTRY;  
    
    CLNode *parentLNode = NULL;
    if (process->GetParentNid() != -1)
    {
        parentLNode = Nodes->GetLNode( process->GetParentNid() );
    }

    if (parentLNode == NULL)
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf( "%s@%d - Not Sending InternalType_ProcessInit request to parentNid=%d"
                          ", process=%s (%d,%d:%d)\n"
                        , method_name, __LINE__
                        , process->GetParentNid()
                        , process->GetName()
                        , process->GetNid()
                        , process->GetPid()
                        , process->GetVerifier() );
        }
        return(0);
    }

    if (!IsTargetRemote( process->GetParentNid() ))
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf( "%s@%d - Not Sending InternalType_ProcessInit request to "
                          "local nid=%d\n"
                        , method_name, __LINE__
                        , process->GetParentNid() );
        }
        return(0);
    }

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d" " - Sending InternalType_ProcessInit to parent node %s, parentNid=%d"
                    ", for process %s (%d,%d:%d), result=%d, tag=%p\n"
                    , method_name, __LINE__
                    , parentLNode->GetNode()->GetName()
                    , parentNid
                    , process->GetName()
                    , process->GetNid()
                    , process->GetPid()
                    , process->GetVerifier()
                    , result
                    , tag );
    }

    struct internal_msg_def msg;
    memset(&msg, 0, sizeof(msg)); 
    msg.type = InternalType_ProcessInit;
    msg.u.processInit.nid = process->GetNid();
    msg.u.processInit.pid = process->GetPid();
    msg.u.processInit.verifier = process->GetVerifier();
    strcpy (msg.u.processInit.name, process->GetName());
    msg.u.processInit.state = process->GetState();
    msg.u.processInit.result = result;
    msg.u.processInit.tag = tag;
    msg.u.processInit.origNid = process->GetParentNid();
    
    ptpMsgInfo_t myInfo;
    myInfo.pnid = MyPNID;
    myInfo.size = offsetof(struct internal_msg_def, u);
    myInfo.size += sizeof(msg.u.processInit);
    
    int error = SendToMon( "process-init"
                         , &msg
                         , myInfo
                         , parentNid
                         , parentLNode->GetNode()->GetName() );
    
    TRACE_EXIT;
    return error;
    
}

int CPtpClient::ProcessKill( CProcess *process
                           , bool abort
                           , int targetNid
                           , const char *targetNodeName )
{
    const char method_name[] = "CPtpClient::ProcessKill";
    TRACE_ENTRY;

    if (!IsTargetRemote( targetNid ))
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf( "%s@%d - Not Sending InternalType_Kill request to "
                          "local nid=%d\n"
                        , method_name, __LINE__
                        , targetNid );
        }
        return(0);
    }

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d - Sending InternalType_Kill request to %s, targetNid=%d"
                      ", killing process (%d,%d:%d)\n"
                    , method_name, __LINE__
                    , targetNodeName
                    , targetNid
                    , process->GetNid()
                    , process->GetPid()
                    , process->GetVerifier() );
    }

    struct internal_msg_def msg;
    memset(&msg, 0, sizeof(msg)); 
    msg.type = InternalType_Kill;
    msg.u.kill.nid = process->GetNid();
    msg.u.kill.pid = process->GetPid();
    msg.u.kill.verifier = process->GetVerifier();
    msg.u.kill.persistent_abort = abort;

    ptpMsgInfo_t myInfo;
    myInfo.pnid = MyPNID;
    myInfo.size = offsetof(struct internal_msg_def, u);
    myInfo.size += sizeof(msg.u.exit);
    
    if (trace_settings & TRACE_PROCESS_DETAIL)
    {
        trace_printf( "%s@%d - size_=%d, process (%d,%d:%d) "
                      "persistent_abort=%d\n"
                    , method_name, __LINE__
                    , myInfo.size
                    , msg.u.kill.nid
                    , msg.u.kill.pid
                    , msg.u.kill.verifier
                    , msg.u.kill.persistent_abort );
    }

    int error = SendToMon( "process-kill"
                         , &msg
                         , myInfo
                         , targetNid
                         , targetNodeName);
    
    TRACE_EXIT;
    return error;
}

int CPtpClient::ProcessNew( CProcess *process
                          , int targetNid
                          , const char *targetNodeName )
{
    const char method_name[] = "CPtpClient::ProcessNew";
    TRACE_ENTRY;

    if (!IsTargetRemote( targetNid ))
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf( "%s@%d - Not Sending InternalType_Process request to "
                          "local nid=%d\n"
                        , method_name, __LINE__
                        , targetNid );
        }
        return(0);
    }

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d - Sending InternalType_Process request to %s, targetNid=%d"
                      ", program=%s, parent=(%d,%d:%d)\n"
                    , method_name, __LINE__
                    , targetNodeName
                    , targetNid
                    , process->program()
                    , process->GetParentNid()
                    , process->GetParentPid()
                    , process->GetParentVerifier() );
    }

    struct internal_msg_def msg;
    memset(&msg, 0, sizeof(msg)); 
    msg.type = InternalType_Process;
    msg.u.process.nid = process->GetNid();
    msg.u.process.pid = process->GetPid();
    msg.u.process.type = process->GetType();
    msg.u.process.priority = process->GetPriority();
    msg.u.process.backup = process->IsBackup();
    msg.u.process.unhooked = process->IsUnhooked();
    msg.u.process.tag = process;
    msg.u.process.parent_nid = process->GetParentNid();
    msg.u.process.parent_pid = process->GetParentPid();
    msg.u.process.parent_verifier = process->GetParentVerifier();
    msg.u.process.pair_parent_nid = process->GetPairParentNid();
    msg.u.process.pair_parent_pid = process->GetPairParentPid();
    msg.u.process.pair_parent_verifier = process->GetPairParentVerifier();
    msg.u.process.pathStrId =  process->pathStrId();
    msg.u.process.ldpathStrId = process->ldPathStrId();
    msg.u.process.programStrId = process->programStrId();
    msg.u.process.argc = process->argc();

    char *stringData = & msg.u.process.stringData;
    int  nameLen = strlen(process->GetName()) + 1;
    int  infileLen = strlen(process->infile()) + 1;
    int  outfileLen = strlen(process->outfile()) + 1;
    int  argvLen = process->userArgvLen();

    // Copy the process name
    msg.u.process.nameLen = nameLen;
    memcpy( stringData, process->GetName(), nameLen );
    stringData += nameLen;

    // Copy the standard in file name
    msg.u.process.infileLen = infileLen;
    memcpy( stringData, process->infile(), infileLen );
    stringData += infileLen;

    // Copy the standard out file name
    msg.u.process.outfileLen = outfileLen;
    memcpy( stringData, process->outfile(), outfileLen  );
    stringData += outfileLen;

    // Copy the program argument strings
    msg.u.process.argvLen =  argvLen;
    memcpy( stringData, process->userArgv(), argvLen );

    ptpMsgInfo_t myInfo;
    myInfo.pnid = MyPNID;
    myInfo.size = offsetof(struct internal_msg_def, u);
    myInfo.size += sizeof(msg.u.process);
    myInfo.size += nameLen ;
    myInfo.size += infileLen ;
    myInfo.size += outfileLen ;
    myInfo.size += argvLen ;
    
    if (trace_settings & TRACE_PROCESS_DETAIL)
    {
        trace_printf( "%s@%d - size_=%d, programStrId=(%d,%d), "
                      "pathStrId=(%d,%d), ldPathStrId=(%d,%d), "
                      "name=%s, strlen(name)=%d, "
                      "infile=%s, strlen(infile)=%d, "
                      "outfile=%s, strlen(outfile)=%d, "
                      "argc=%d, strlen(total argv)=%d, args=[%.*s]\n"
                    , method_name, __LINE__
                    , myInfo.size
                    , msg.u.process.programStrId.nid
                    , msg.u.process.programStrId.id
                    , msg.u.process.pathStrId.nid
                    , msg.u.process.pathStrId.id
                    , msg.u.process.ldpathStrId.nid
                    , msg.u.process.ldpathStrId.id
                    , &msg.u.process.stringData
                    , nameLen
                    , &msg.u.process.stringData+nameLen
                    , infileLen
                    , &msg.u.process.stringData+nameLen+infileLen
                    , outfileLen 
                    , msg.u.process.argc
                    , argvLen
                    , argvLen
                    , &msg.u.process.stringData+nameLen+infileLen+outfileLen);
    }

    int error = SendToMon( "process-new"
                         , &msg
                         , myInfo
                         , targetNid
                         , targetNodeName);
    
    TRACE_EXIT;
    return error;
}

int CPtpClient::ProcessNotify( int nid
                             , int pid
                             , Verifier_t verifier
                             , _TM_Txid_External transId
                             , bool canceled
                             , CProcess *targetProcess
                             , int targetNid
                             , const char *targetNodeName )
{
    const char method_name[] = "CPtpClient::ProcessNotify";
    TRACE_ENTRY;

    if (!IsTargetRemote( targetNid ))
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf( "%s@%d - Not Sending InternalType_Notify request to "
                          "local nid=%d\n"
                        , method_name, __LINE__
                        , targetNid );
        }
        return(0);
    }

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d - Sending InternalType_Notify request to %s"
                      ", nid=%d, canceled=%d\n"
                    , method_name, __LINE__
                    , targetNodeName
                    , targetNid
                    , canceled );
    }

    struct internal_msg_def msg;
    memset(&msg, 0, sizeof(msg)); 
    msg.type = InternalType_Notify;
    msg.u.notify.nid = nid;
    msg.u.notify.pid = pid;
    msg.u.notify.verifier = verifier;
    msg.u.notify.canceled = canceled;
    msg.u.notify.target_nid = targetProcess->GetNid();
    msg.u.notify.target_pid = targetProcess->GetPid();
    msg.u.notify.target_verifier = targetProcess->GetVerifier();
    msg.u.notify.trans_id = transId;

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        if (canceled)
        {
            trace_printf( "%s@%d - Process (%d, %d:%d) deleting death "
                          "notice interest for %s (%d, %d:%d), "
                          "trans_id=%lld.%lld.%lld.%lld\n"
                        , method_name, __LINE__
                        , msg.u.notify.nid
                        , msg.u.notify.pid
                        , msg.u.notify.verifier
                        , targetProcess->GetName()
                        , msg.u.notify.target_nid
                        , msg.u.notify.target_pid
                        , msg.u.notify.target_verifier
                        , msg.u.notify.trans_id.txid[0]
                        , msg.u.notify.trans_id.txid[1]
                        , msg.u.notify.trans_id.txid[2]
                        , msg.u.notify.trans_id.txid[3] );
        }
        else
        {
            trace_printf("%s@%d - Process (%d, %d:%d) registering interest "
                         "in death of process %s (%d, %d:%d), "
                         "trans_id=%lld.%lld.%lld.%lld\n"
                        , method_name, __LINE__
                        , msg.u.notify.nid
                        , msg.u.notify.pid
                        , msg.u.notify.verifier
                        , targetProcess->GetName()
                        , msg.u.notify.target_nid
                        , msg.u.notify.target_pid
                        , msg.u.notify.target_verifier
                        , msg.u.notify.trans_id.txid[0]
                        , msg.u.notify.trans_id.txid[1]
                        , msg.u.notify.trans_id.txid[2]
                        , msg.u.notify.trans_id.txid[3] );
        }
    }

    ptpMsgInfo_t myInfo;
    myInfo.pnid = MyPNID;
    myInfo.size = offsetof(struct internal_msg_def, u);
    myInfo.size += sizeof(msg.u.notify);

    int error = SendToMon( "process-notify"
                         , &msg
                         , myInfo
                         , targetNid
                         , targetNodeName);
    
    TRACE_EXIT;
    return error;
}

int CPtpClient::ProcessStdInReq( int nid
                               , int pid
                               , StdinReqType type
                               , int supplierNid
                               , int supplierPid )
{
    const char method_name[] = "CPtpClient::ProcessStdInReq";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_REDIRECTION | TRACE_PROCESS))
    {
        trace_printf( "%s@%d - Sending InternalType_StdinReq request type =%d "
                      "from (%d,%d), for supplier (%d,%d)\n"
                    , method_name, __LINE__
                    , type
                    , nid
                    , pid
                    , supplierNid
                    , supplierPid );
    }

    CLNode  *lnode = Nodes->GetLNode( supplierNid );
    if (lnode == NULL)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], Can't find supplier node nid=%d "
                  "for stdin data request.\n"
                , method_name
                , supplierNid );
        mon_log_write(PTPCLIENT_STDINREQ_1, SQ_LOG_ERR, buf);

        TRACE_EXIT;
        return -1;
    }

    CProcess *process = lnode->GetProcessL( supplierPid );
    if (process == NULL)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], Can't find process nid=%d, "
                  "pid=%d for stdin data request.\n"
                , method_name
                , supplierNid
                , supplierPid );
        mon_log_write(PTPCLIENT_STDINREQ_2, SQ_LOG_ERR, buf);

        TRACE_EXIT;
        return -1;
    }

    struct internal_msg_def msg;
    memset(&msg, 0, sizeof(msg)); 
    msg.type = InternalType_StdinReq;
    msg.u.stdin_req.nid = nid;
    msg.u.stdin_req.pid = pid;
    msg.u.stdin_req.reqType = type;
    msg.u.stdin_req.supplier_nid = supplierNid;
    msg.u.stdin_req.supplier_pid = supplierPid;

    ptpMsgInfo_t myInfo;
    myInfo.pnid = MyPNID;
    myInfo.size = offsetof(struct internal_msg_def, u);
    myInfo.size += sizeof(msg.u.stdin_req);
    
    if (trace_settings & (TRACE_REDIRECTION | TRACE_PROCESS_DETAIL))
    {
        trace_printf( "%s@%d - size_=%d, type =%d "
                      "from (%d,%d), for supplier (%d,%d)\n"
                    , method_name, __LINE__
                    , myInfo.size
                    , msg.u.stdin_req.reqType
                    , msg.u.stdin_req.nid
                    , msg.u.stdin_req.pid
                    , msg.u.stdin_req.supplier_nid
                    , msg.u.stdin_req.supplier_pid );
    }

    int error = SendToMon( "process-stdin"
                         , &msg
                         , myInfo
                         , process->GetNid()
                         , lnode->GetNode()->GetName());
    
    TRACE_EXIT;
    return error;
}

int CPtpClient::ProcessStdIoData( int nid
                                , int pid
                                , StdIoType type
                                , ssize_t count
                                , char *data )
{
    const char method_name[] = "CPtpClient::ProcessStdIoData";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_REDIRECTION | TRACE_PROCESS))
    {
        trace_printf( "%s@%d - Sending InternalType_IoData request type =%d "
                      "to (%d,%d), count=%ld\n"
                    , method_name, __LINE__
                    , type
                    , nid
                    , pid
                    , count );
    }

    CLNode  *lnode = Nodes->GetLNode( nid );
    if (lnode == NULL)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], Can't find supplier node nid=%d "
                  "for stdin data request.\n"
                , method_name
                , nid );
        mon_log_write(PTPCLIENT_STDIODATA_1, SQ_LOG_ERR, buf);

        TRACE_EXIT;
        return -1;
    }

    CProcess *process = lnode->GetProcessL( pid );
    if (process == NULL)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], Can't find process nid=%d, "
                  "pid=%d for stdin data request.\n"
                , method_name
                , nid
                , pid );
        mon_log_write(PTPCLIENT_STDIODATA_2, SQ_LOG_ERR, buf);

        TRACE_EXIT;
        return -1;
    }

    struct internal_msg_def msg;
    memset(&msg, 0, sizeof(msg)); 
    msg.type = InternalType_IoData;
    msg.u.iodata.nid = nid ;
    msg.u.iodata.pid = pid ;
    msg.u.iodata.ioType = type ;
    msg.u.iodata.length = count;
    memcpy(&msg.u.iodata.data, data, count);

    ptpMsgInfo_t myInfo;
    myInfo.pnid = MyPNID;
    myInfo.size = offsetof(struct internal_msg_def, u);
    myInfo.size += sizeof(msg.u.iodata);
    
    if (trace_settings & (TRACE_REDIRECTION | TRACE_PROCESS_DETAIL))
    {
        trace_printf( "%s@%d - size_=%d, type =%d "
                      "to (%d,%d), count=%d\n(%s)"
                    , method_name, __LINE__
                    , myInfo.size
                    , msg.u.iodata.ioType
                    , msg.u.iodata.nid
                    , msg.u.iodata.pid
                    , msg.u.iodata.length
                    , msg.u.iodata.length?msg.u.iodata.data:"\n" );
    }

    int error = SendToMon( "process-stdio-data"
                         , &msg
                         , myInfo
                         , process->GetNid()
                         , lnode->GetNode()->GetName());

    TRACE_EXIT;
    return error;
}

int CPtpClient::SendToMon(const char *reqType, internal_msg_def *msg
                         , ptpMsgInfo_t &myInfo
                         , int targetNid, const char *hostName)
{
    const char method_name[] = "CPtpClient::SendToMon";
    TRACE_ENTRY;
    
    char ptpHost[MAX_PROCESSOR_NAME];
    char ptpPort[MAX_PROCESSOR_NAME];
    int error = 0;
    int tempPort = ptpCommPort_;
    int pnid = 0;
    int sendSock = -1;
    int retryCount = 0;
    CNode *node = NULL;
    CLNode *lnode = NULL;

    ptpHost[0] = '\0';
    lnode = Nodes->GetLNode( targetNid );
    node = lnode->GetNode();
    pnid = node->GetPNid();

    // For virtual env
    if (!IsRealCluster)
    {
        tempPort += targetNid;
        strcat( ptpHost, ptpHost_ );
    }
    else
    {
        strcat( ptpHost, hostName );
    }
    
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d - reqType=%s, hostName=%s, targetNid=%d, "
                      "ptpHost=%s, tempPort=%d, ptpCommPort_=%d\n"
                    , method_name, __LINE__
                    , reqType
                    , hostName
                    , targetNid
                    , ptpHost
                    , tempPort 
                    , ptpCommPort_ );
    }

    memset( &ptpPort, 0, MAX_PROCESSOR_NAME );
    memset( &ptpPortBase_, 0, MAX_PROCESSOR_NAME+100 );
    sprintf( ptpPortBase_,"%s:", ptpHost );
    sprintf( ptpPort,"%s%d", ptpPortBase_, tempPort );

retryIO:

    if (ptpClusterSocks_[pnid] == -1)
    {
        error = InitializePtpClient( pnid, ptpPort );
        if (error < 0)
        {
            ptpClusterSocks_[pnid] = -1;
            TRACE_EXIT;
            return error;
        }
    }

    sendSock = ptpClusterSocks_[pnid];

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d - sending %s REQ to Monitor=%s, sock=%d\n"
                    , method_name, __LINE__
                    , reqType
                    , ptpPort
                    , sendSock );
    }

    error = SockSend((char *) &myInfo, sizeof(ptpMsgInfo_t), sendSock);
    if (error)
    {
        int err = error;
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], unable to send %s request size %ld to "
                  "node %s, error: %d(%s)\n"
                , method_name, reqType, sizeof(ptpMsgInfo_t), ptpHost, err, strerror(err) );
        mon_log_write(PTPCLIENT_SENDTOMON_1, SQ_LOG_ERR, buf);    
    }
    else
    {
        error = SockSend((char *) msg, myInfo.size, sendSock);
        if (error)
        {
            int err = error;
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], unable to send %s request to "
                      "node %s, error: %d(%s)\n"
                    , method_name, reqType, ptpHost, err, strerror(err) );
            mon_log_write(PTPCLIENT_SENDTOMON_2, SQ_LOG_ERR, buf);    
        }
    }
    
    if (error)
    {
        SockClose( pnid );
        if ( retryCount < MON2MON_IO_RETRIES )
        {
            retryCount++;
            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            {
                trace_printf( "%s@%d - retrying IO (%d) to node %s\n"
                            , method_name, __LINE__
                            , retryCount
                            , ptpHost );
            }
            goto retryIO;
        }
    }

    TRACE_EXIT;
    return error;
}

void CPtpClient::SockClose( int pnid )
{
    const char method_name[] = "CPtpClient::SockClose";
    TRACE_ENTRY;

    if (ptpClusterSocks_[pnid] != -1)
    {
        close( ptpClusterSocks_[pnid] );
        ptpClusterSocks_[pnid] = -1;
    }

    TRACE_EXIT;
}

void CPtpClient::SetLocalHost( void )
{
    gethostname( ptpHost_, MAX_PROCESSOR_NAME );
}

int CPtpClient::SockReceive(char *buf, int size, int sockFd)
{
    const char method_name[] = "CPtpClient::SockReceive";
    TRACE_ENTRY;

    bool    readAgain = false;
    int     error = 0;
    int     readCount = 0;
    int     received = 0;
    int     sizeCount = size;
       
    do
    {
        readCount = (int) recv( sockFd
                              , buf
                              , sizeCount
                              , 0 );
        if ( readCount > 0 ) Meas.addSockPtpRcvdBytes( readCount );

        if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - Count read %d = recv(%d)\n"
                        , method_name, __LINE__
                        , readCount
                        , sizeCount );
        }
    
        if ( readCount > 0 )
        { // Got data
            received += readCount;
            buf += readCount;
            if ( received == size )
            {
                readAgain = false;
            }
            else
            {
                sizeCount -= readCount;
                readAgain = true;
            }
        }
        else if ( readCount == 0 )
        { // EOF
             error = ENODATA;
             readAgain = false;
        }
        else
        { // Got an error
            if ( errno != EINTR)
            {
                error = errno;
                readAgain = false;
            }
            else
            {
                readAgain = true;
            }
        }
    }
    while( readAgain );

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - recv(), received=%d, error=%d(%s)\n"
                    , method_name, __LINE__
                    , received
                    , error, strerror(error) );
    }

    TRACE_EXIT;
    return error;
}

int CPtpClient::SockSend(char *buf, int size, int sockFd)
{
    const char method_name[] = "CPtpClient::SockSend";
    TRACE_ENTRY;
    
    bool    sendAgain = false;
    int     error = 0;
    int     sendCount = 0;
    int     sent = 0;
    
    do
    {
        sendCount = (int) send( sockFd
                              , buf
                              , size
                              , 0 );
        if ( sendCount > 0 ) Meas.addSockPtpSentBytes( sendCount );

        if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - send(), sendCount=%d\n"
                        , method_name, __LINE__
                        , sendCount );
        }
    
        if ( sendCount > 0 )
        { // Sent data
            sent += sendCount;
            if ( sendCount == size )
            {
                 sendAgain = false;
            }
            else
            {
                sendAgain = true;
            }
        }
        else
        { // Got an error
            if ( errno != EINTR)
            {
                error = errno;
                sendAgain = false;
            }
            else
            {
                sendAgain = true;
            }
        }
    }
    while( sendAgain );

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - send(), sent=%d, error=%d(%s)\n"
                    , method_name, __LINE__
                    , sent
                    , error, strerror(error) );
    }

    TRACE_EXIT;
    return error;
}

