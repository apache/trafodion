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

CPtpClient::CPtpClient (void)
          : ptpSock_(0)
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
    

    char * p = getenv( "MON2MON_COMM_PORT" );
    if ( p ) 
    {
        basePort_ = atoi( p );
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s@%d] MON2MON_COMM_PORT environment variable is not set!\n"
                , method_name, __LINE__ );
        mon_log_write( PTPCLIENT_PTPCLIENT_1, SQ_LOG_CRIT, buf );
        abort();
    }

    TRACE_EXIT;
}

CPtpClient::~CPtpClient (void)
{
    const char method_name[] = "CPtpClient::~CPtpClient";
    TRACE_ENTRY;

    TRACE_EXIT;
}

int  CPtpClient::AddUniqStr( int nid
                           , int id
                           , const char *stringValue
                           , int targetNid
                           , const char *targetNodeName )
{
    const char method_name[] = "CPtpClient::AddUniqStr";
    TRACE_ENTRY;

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

    int size = offsetof(struct internal_msg_def, u);
    size += sizeof(msg.u.uniqstr);
    size += stringDataLen;
    
    if (trace_settings & TRACE_PROCESS_DETAIL)
    {
        trace_printf( "%s@%d - size_=%d, forwarding unique string [%d, %d] (%s)\n"
                    , method_name, __LINE__
                    , size
                    , msg.u.uniqstr.nid
                    , msg.u.uniqstr.id
                    , &msg.u.uniqstr.valueData  );
    }

    int error = SendToMon("add-unique-string", &msg, size, targetNid, targetNodeName);
    
    TRACE_EXIT;
    return error;
}

int CPtpClient::InitializePtpClient( char * ptpPort )
{
    const char method_name[] = "CPtpClient::InitializePtpClient";
    TRACE_ENTRY;
    int err = 0;
      
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
        ptpSock_ = sock;
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf( "%s@%d - connected to monitor node=%s, sock=%d\n"
                        , method_name, __LINE__
                        , ptpPort
                        , ptpSock_ );
        }
    }

    TRACE_EXIT;
    return err;
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

    int size = offsetof(struct internal_msg_def, u);
    size += sizeof(msg.u.clone);
    size += nameLen ;
    size += portLen ;
    size += infileLen ;
    size += outfileLen ;
    size += argvLen ;
    
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
                    , size
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
                         , size
                         , process->GetParentNid()
                         , parentLNode->GetNode()->GetName());
    
    TRACE_EXIT;
    return error;
}

int CPtpClient::ProcessExit( CProcess *process
                           , int targetNid
                           , const char *targetNodeName )
{
    const char method_name[] = "CPtpClient::ProcessExit";
    TRACE_ENTRY;

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

    int size = offsetof(struct internal_msg_def, u);
    size += sizeof(msg.u.exit);
    
    if (trace_settings & TRACE_PROCESS_DETAIL)
    {
        trace_printf( "%s@%d - size_=%d, process %s (%d,%d:%d) "
                      "abended=%d\n"
                    , method_name, __LINE__
                    , size
                    , msg.u.exit.name
                    , msg.u.exit.nid
                    , msg.u.exit.pid
                    , msg.u.exit.verifier
                    , msg.u.exit.abended );
    }

    int error = SendToMon("process-exit", &msg, size, targetNid, targetNodeName);
    
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
    
    int size = offsetof(struct internal_msg_def, u);
    size += sizeof(msg.u.processInit);
    
    int error = SendToMon( "process-init"
                         , &msg
                         , size
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

    int size = offsetof(struct internal_msg_def, u);
    size += sizeof(msg.u.exit);
    
    if (trace_settings & TRACE_PROCESS_DETAIL)
    {
        trace_printf( "%s@%d - size_=%d, process (%d,%d:%d) "
                      "persistent_abort=%d\n"
                    , method_name, __LINE__
                    , size
                    , msg.u.kill.nid
                    , msg.u.kill.pid
                    , msg.u.kill.verifier
                    , msg.u.kill.persistent_abort );
    }

    int error = SendToMon("process-kill", &msg, size, targetNid, targetNodeName);
    
    TRACE_EXIT;
    return error;
}

int CPtpClient::ProcessNew( CProcess *process
                          , int targetNid
                          , const char *targetNodeName )
{
    const char method_name[] = "CPtpClient::ProcessNew";
    TRACE_ENTRY;

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

    int size = offsetof(struct internal_msg_def, u);
    size += sizeof(msg.u.process);
    size += nameLen ;
    size += infileLen ;
    size += outfileLen ;
    size += argvLen ;
    
    if (trace_settings & TRACE_PROCESS_DETAIL)
    {
        trace_printf( "%s@%d - size_=%d, programStrId=(%d,%d), "
                      "pathStrId=(%d,%d), ldPathStrId=(%d,%d), "
                      "name=%s, strlen(name)=%d, "
                      "infile=%s, strlen(infile)=%d, "
                      "outfile=%s, strlen(outfile)=%d, "
                      "argc=%d, strlen(total argv)=%d, args=[%.*s]\n"
                    , method_name, __LINE__
                    , size
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

    int error = SendToMon("process-new", &msg, size, targetNid, targetNodeName);
    
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

    int size = offsetof(struct internal_msg_def, u);
    size += sizeof(msg.u.notify);

    int error = SendToMon("process-notify", &msg, size, targetNid, targetNodeName);
    
    TRACE_EXIT;
    return error;
}

int CPtpClient::ReceiveSock(char *buf, int size, int sockFd)
{
    const char method_name[] = "CPtpClient::ReceiveSock";
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

void CPtpClient::SetLocalHost( void )
{
    gethostname( ptpHost_, MAX_PROCESSOR_NAME );
}

int CPtpClient::SendSock(char *buf, int size, int sockFd)
{
    const char method_name[] = "CPtpClient::SendSock";
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

int CPtpClient::SendToMon(const char *reqType, internal_msg_def *msg, int size, 
                           int receiveNode, const char *hostName)
{
    const char method_name[] = "CPtpClient::SendToMon";
    TRACE_ENTRY;
    
    char monPortString[MAX_PROCESSOR_NAME];
    char ptpHost[MAX_PROCESSOR_NAME];
    char ptpPort[MAX_PROCESSOR_NAME];
    int tempPort = basePort_;
    
    ptpHost[0] = '\0';

    // For virtual env
    if (!IsRealCluster)
    {
        tempPort += receiveNode;
        strcat( ptpHost, ptpHost_ );
    }
    else
    {
        strcat( ptpHost, hostName );
    }
    
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d - reqType=%s, hostName=%s, receiveNode=%d, "
                      "ptpHost=%s, tempPort=%d, basePort_=%d\n"
                    , method_name, __LINE__
                    , reqType
                    , hostName
                    , receiveNode
                    , ptpHost
                    , tempPort 
                    , basePort_ );
    }

    memset( &ptpPort, 0, MAX_PROCESSOR_NAME );
    memset( &ptpPortBase_, 0, MAX_PROCESSOR_NAME+100 );

    strcat( ptpPortBase_, ptpHost );
    strcat( ptpPortBase_, ":" );
    sprintf( monPortString,"%d", tempPort );
    strcat( ptpPort, ptpPortBase_ );
    strcat( ptpPort, monPortString ); 

    int error = InitializePtpClient( ptpPort );
    if (error < 0)
    {
        TRACE_EXIT;
        return error;
    }

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d - sending %s REQ to Monitor=%s, sock=%d\n"
                    , method_name, __LINE__
                    , reqType
                    , ptpPort
                    , ptpSock_);
    }

    error = SendSock((char *) &size, sizeof(size), ptpSock_);
    if (error)
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf( "%s@%d - error sending to Monitor=%s, sock=%d, error=%d\n"
                        , method_name, __LINE__
                        , ptpPort
                        , ptpSock_
                        , error );
        }
    }
    
    error = SendSock((char *) msg, size, ptpSock_);
    if (error)
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf( "%s@%d - error sending to nameserver=%s, sock=%d, error=%d\n"
                        , method_name, __LINE__
                        , ptpPort
                        , ptpSock_
                        , error );
        }
    }
    
    close( ptpSock_ );

    TRACE_EXIT;
    return error;
}

int CPtpClient::StdInReq( int nid
                        , int pid
                        , StdinReqType type
                        , int supplierNid
                        , int supplierPid )
{
    const char method_name[] = "CPtpClient::StdInReq";
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

    int size = offsetof(struct internal_msg_def, u);
    size += sizeof(msg.u.stdin_req);
    
    if (trace_settings & (TRACE_REDIRECTION | TRACE_PROCESS_DETAIL))
    {
        trace_printf( "%s@%d - size_=%d, type =%d "
                      "from (%d,%d), for supplier (%d,%d)\n"
                    , method_name, __LINE__
                    , size
                    , msg.u.stdin_req.reqType
                    , msg.u.stdin_req.nid
                    , msg.u.stdin_req.pid
                    , msg.u.stdin_req.supplier_nid
                    , msg.u.stdin_req.supplier_pid );
    }

    int error = SendToMon("stdin"
                         , &msg
                         , size
                         , process->GetNid()
                         , lnode->GetNode()->GetName());
    
    TRACE_EXIT;
    return error;
}

int CPtpClient::StdIoData( int nid
                         , int pid
                         , StdIoType type
                         , ssize_t count
                         , char *data )
{
    const char method_name[] = "CPtpClient::StdIoData";
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

    int size = offsetof(struct internal_msg_def, u);
    size += sizeof(msg.u.iodata);
    
    if (trace_settings & (TRACE_REDIRECTION | TRACE_PROCESS_DETAIL))
    {
        trace_printf( "%s@%d - size_=%d, type =%d "
                      "to (%d,%d), count=%d\n(%s)"
                    , method_name, __LINE__
                    , size
                    , msg.u.iodata.ioType
                    , msg.u.iodata.nid
                    , msg.u.iodata.pid
                    , msg.u.iodata.length
                    , msg.u.iodata.length?msg.u.iodata.data:"\n" );
    }

    int error = SendToMon("stdio-data"
                         , &msg
                         , size
                         , process->GetNid()
                         , lnode->GetNode()->GetName());
    
    TRACE_EXIT;
    return error;
}

