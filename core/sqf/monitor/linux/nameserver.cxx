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

#include "lnode.h"
#include "pnode.h"
#include "nameserver.h"
#include "monlogging.h"
#include "montrace.h"

extern CNode *MyNode;
extern CProcess *NameServerProcess;

CNameServer::CNameServer( void )
: mon2nsSock_(-1)
, nsStartupComplete_(false)
, seqNum_(0)
{
    const char method_name[] = "CNameServer::CNameServer";
    TRACE_ENTRY;

    gethostname( mon2nsPort_, MAX_PROCESSOR_NAME);
    strcat( mon2nsPort_, ":");
    char *p = getenv( "NS_M2N_COMM_PORT" );
    if ( p )
        strcat( mon2nsPort_, p );
    else
        strcat( mon2nsPort_, "0" );

    TRACE_EXIT;
}

CNameServer::~CNameServer( void )
{
    const char method_name[] = "CNameServer::~CNameServer";
    TRACE_ENTRY;

    TRACE_EXIT;
}

int CNameServer::ConnectToNs( void )
{
    const char method_name[] = "CNameServer::ConnectToNs";
    TRACE_ENTRY;
    int err = 0;

    int sock = SockCreate();
    if ( sock < 0 )
        err = sock;
    if ( err == 0)
    {
        mon2nsSock_ = sock;
        if ( trace_settings & (TRACE_INIT | TRACE_PROCESS) )
        {
            trace_printf( "%s@%d - connected to nameserver=%s, sock=%d\n"
                        , method_name, __LINE__
                        , mon2nsPort_
                        , mon2nsSock_ );
        }
    }

    if ( err == 0)
    {
        nodeId_t msg;
        strcpy( msg.nodeName, MyNode->GetName() );
        strcpy( msg.commPort, MyNode->GetCommPort() );
        strcpy( msg.syncPort, MyNode->GetSyncPort() );
        msg.pid = -1;
        msg.pnid = MyNode->GetPNid();
        msg.creatorPNid = -1;
        msg.creatorShellPid = -1;
        msg.creatorShellVerifier = -1;
        msg.creator = false;
        msg.ping = false;
        if ( trace_settings & (TRACE_INIT | TRACE_PROCESS) )
        {
            trace_printf( "%s@%d - sending node-info to nameserver=%s, sock=%d\n"
                        , method_name, __LINE__
                        , mon2nsPort_
                        , mon2nsSock_ );
        }
        err = SockSend( ( char *) &msg, sizeof(msg) );
        if ( err == 0 )
        {
            if ( trace_settings & (TRACE_INIT | TRACE_PROCESS) )
            {
                trace_printf( "%s@%d - OK send to nameserver=%s, sock=%d, error=%d, waiting receive\n"
                            , method_name, __LINE__
                            , mon2nsPort_
                            , mon2nsSock_
                            , err );
            }
            err = SockReceive( (char *) &msg, sizeof(msg ) );
            if ( err )
            {
                if ( trace_settings & (TRACE_INIT | TRACE_PROCESS) )
                {
                    trace_printf( "%s@%d - error receiving from nameserver=%s, sock=%d, error=%d\n"
                                , method_name, __LINE__
                                , mon2nsPort_
                                , mon2nsSock_
                                , err );
                }
            }
            else
            {
                if ( !nsStartupComplete_ )
                {
                    nsStartupComplete_ = true;
                    NameServerProcess->CompleteProcessStartup( (char *) ""
                                                             , msg.pid
                                                             , false
                                                             , false
                                                             , false
                                                             , NULL
                                                             , -1 );
                }
            }
        }
    }

    TRACE_EXIT;
    return err;
}

void CNameServer::SockClose( void )
{
    const char method_name[] = "CNameServer::SockClose";
    TRACE_ENTRY;

    close( mon2nsSock_ );
    mon2nsSock_ = -1;
    TRACE_EXIT;
}

int CNameServer::SockCreate( void )
{
    const char method_name[] = "CNameServer::SockCreate";
    TRACE_ENTRY;

    int    sock;        // socket
    int    ret;         // returned value
    int    reuse = 1;   // sockopt reuse option
    socklen_t  size;    // size of socket address
    static int retries = 0;      // # times to retry connect
    int    outer_failures = 0;   // # failed connect loops
    int    connect_failures = 0; // # failed connects
    char   *p;     // getenv results 
    struct sockaddr_in  sockinfo;    // socket address info 
    struct hostent *he;
    char   host[1000];
    const char *colon;
    unsigned int port;

    colon = strstr( mon2nsPort_, ":" );
    strcpy( host, mon2nsPort_ );
    int len = colon - mon2nsPort_;
    host[len] = '\0';
    port = atoi(&colon[1]);
    
    size = sizeof(sockinfo );

    if ( !retries )
    {
        p = getenv( "HPMP_CONNECT_RETRIES" );
        if ( p ) retries = atoi( p );
        else retries = 5;
    }

    for ( ;; )
    {
        sock = socket( AF_INET, SOCK_STREAM, 0 );
        if ( sock < 0 )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            int err = errno;
            snprintf( la_buf, sizeof(la_buf) 
                    , "[%s], socket() failed! errno=%d (%s)\n"
                    , method_name, err, strerror(err) );
            mon_log_write( MON_CLUSTER_MKCLTSOCK_1, SQ_LOG_ERR, la_buf ); 
            return ( -1 );
        }

        he = gethostbyname( host );
        if ( !he )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            int err = errno;
            snprintf( la_buf, sizeof(la_buf ), 
                      "[%s] gethostbyname(%s) failed! errno=%d (%s)\n"
                    , method_name, host, err, strerror(err) );
            mon_log_write(MON_CLUSTER_MKCLTSOCK_2, SQ_LOG_ERR, la_buf ); 
            close( sock );
            return ( -1 );
        }

        // Connect socket.
        memset( (char *) &sockinfo, 0, size );
        memcpy( (char *) &sockinfo.sin_addr, (char *) he->h_addr, 4 );
        sockinfo.sin_family = AF_INET;
        sockinfo.sin_port = htons( (unsigned short) port );

        // Note the outer loop uses "retries" from HPMP_CONNECT_RETRIES,
        // and has a yield between each retry, since it's more oriented
        // toward failures from network overload and putting a pause
        // between retries.  This inner loop should only iterate when
        // a signal interrupts the local process, so it doesn't pause
        // or use the same HPMP_CONNECT_RETRIES count.
        connect_failures = 0;
        ret = 1;
        while ( ret != 0 && connect_failures <= 10 )
        {
            if ( trace_settings & (TRACE_INIT | TRACE_PROCESS) )
            {
                trace_printf( "%s@%d - Connecting to %s addr=%d.%d.%d.%d, port=%d, connect_failures=%d\n"
                            , method_name, __LINE__
                            , host
                            , (int)((unsigned char *)he->h_addr)[0]
                            , (int)((unsigned char *)he->h_addr)[1]
                            , (int)((unsigned char *)he->h_addr)[2]
                            , (int)((unsigned char *)he->h_addr)[3]
                            , port
                            , connect_failures );
            }
    
            ret = connect( sock, (struct sockaddr *) &sockinfo, size );
            if ( ret == 0 ) break;
            ++connect_failures;
            if ( errno != EINTR )
            {
                char la_buf[MON_STRING_BUF_SIZE];
                int err = errno;
                sprintf( la_buf, "[%s], connect() failed! errno=%d (%s)\n"
                       , method_name, err, strerror(err) );
                mon_log_write(MON_CLUSTER_MKCLTSOCK_3, SQ_LOG_ERR, la_buf ); 
                struct timespec req, rem;
                req.tv_sec = 0;
                req.tv_nsec = 500000000L; // 500,000,000
                nanosleep( &req, &rem );
            }
        }

        if ( ret == 0 ) break;

        // For large clusters, the connect/accept calls seem to fail occasionally,
        // no doubt do to the large number (1000's) of simultaneous connect packets
        // flooding the network at once.  So, we retry up to HPMP_CONNECT_RETRIES
        // number of times.
        if ( errno != EINTR )
        {
            if ( ++outer_failures > retries )
            {
                char la_buf[MON_STRING_BUF_SIZE];
                sprintf( la_buf, "[%s], connect() exceeded retries! count=%d\n"
                       , method_name, retries );
                mon_log_write(MON_CLUSTER_MKCLTSOCK_4, SQ_LOG_ERR, la_buf ); 
                close( sock );
                return ( -1 );
            }
            struct timespec req, rem;
            req.tv_sec = 0;
            req.tv_nsec = 500000;
            nanosleep( &req, &rem );
        }
        close( sock );
    }

    if ( setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(int) ) )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf( la_buf, "[%s], setsockopt() failed! errno=%d (%s)\n"
               , method_name, err, strerror(err) );
        mon_log_write(MON_CLUSTER_MKCLTSOCK_5, SQ_LOG_ERR, la_buf ); 
        close( sock );
        return ( -2 );
    }

    TRACE_EXIT;
    return ( sock );
}

int CNameServer::ProcessDelete(CProcess* process )
{
    const char method_name[] = "CNameServer::ProcessDelete";
    TRACE_ENTRY;

    struct message_def msg;
    memset(&msg, 0, sizeof(msg) ); // TODO: remove!
    msg.type = MsgType_Service;
    msg.noreply = false;
    msg.reply_tag = seqNum_++;
    msg.u.request.type = ReqType_DelProcessNs;
    struct DelProcessNs_def *msgdel = &msg.u.request.u.del_process_ns;
    msgdel->nid = process->GetNid();
    msgdel->pid = process->GetPid();
    msgdel->verifier = process->GetVerifier();
    strcpy( msgdel->process_name, process->GetName() );
    msgdel->target_nid = msgdel->nid;
    msgdel->target_pid = msgdel->pid;
    msgdel->target_verifier = msgdel->verifier;
    strcpy( msgdel->target_process_name, msgdel->process_name );

    int error = SendReceive(&msg );

    TRACE_EXIT;
    return error;
}

int CNameServer::ProcessInfo( struct message_def* msg )
{
    const char method_name[] = "CNameServer::ProcessInfo";
    TRACE_ENTRY;

    int error = SendReceive( msg );

    TRACE_EXIT;
    return error;
}

int CNameServer::ProcessInfoCont( struct message_def* msg )
{
    const char method_name[] = "CNameServer::ProcessInfoCont";
    TRACE_ENTRY;

    int error = SendReceive( msg );

    TRACE_EXIT;
    return error;
}

int CNameServer::ProcessNew(CProcess* process )
{
    const char method_name[] = "CNameServer::ProcessNew";
    TRACE_ENTRY;

    struct message_def msg;
    memset(&msg, 0, sizeof(msg) ); // TODO: remove!
    msg.type = MsgType_Service;
    msg.noreply = false;
    msg.reply_tag = seqNum_++;
    msg.u.request.type = ReqType_NewProcessNs;
    struct NewProcessNs_def *msgnew = &msg.u.request.u.new_process_ns;
    CProcess* parent = process->GetParent();
    if ( parent )
    {
        msgnew->parent_nid = parent->GetNid();
        msgnew->parent_pid = parent->GetPid();
        msgnew->parent_verifier = parent->GetVerifier();
    }
    else
    {
        msgnew->parent_nid = -1;
        msgnew->parent_pid = -1;
        msgnew->parent_verifier = -1;
    }
    msgnew->nid = process->GetNid();
    msgnew->pid = process->GetPid();
    msgnew->verifier = process->GetVerifier();
    msgnew->type = process->GetType();
    msgnew->priority = process->GetPriority();
    strcpy( msgnew->process_name, process->GetName() );

    int error = SendReceive(&msg );

    TRACE_EXIT;
    return error;
}

int CNameServer::SendReceive( struct message_def* msg )
{
    const char method_name[] = "CNameServer::SendReceive";
    char desc[100];
    char* descp;
    struct DelProcessNs_def *msgdel;
    struct NewProcessNs_def *msgnew;

    TRACE_ENTRY;

    descp = desc;
    int size = offsetof(struct message_def, u.request.u);
    switch ( msg->u.request.type )
    {
    case ReqType_DelProcessNs:
        msgdel = &msg->u.request.u.del_process_ns;
        sprintf( desc, "delete-process (nid=%d, pid=%d, verifier=%d, name=%s)",
                 msgdel->nid, msgdel->pid, msgdel->verifier, msgdel->process_name );
        size += sizeof(msg->u.request.u.del_process_ns);
        break;
    case ReqType_NewProcessNs:
        msgnew = &msg->u.request.u.new_process_ns;
        sprintf( desc, "new-process (nid=%d, pid=%d, verifier=%d, name=%s)",
                msgnew->nid, msgnew->pid, msgnew->verifier, msgnew->process_name );
        size += sizeof(msg->u.request.u.new_process_ns);
        break;
    case ReqType_ProcessInfo:
        descp = (char *) "process-info";
        size += sizeof(msg->u.request.u.process_info);
        break;
    case ReqType_ProcessInfoCont:
        descp = (char *) "process-info-cont";
        size += sizeof(msg->u.request.u.process_info_cont);
        break;
    default:
        abort(); // TODO change
        break;
    }

    int error = SendToNs( descp, msg, size );
    if ( error == 0 )
        error = SockReceive( (char *) &size, sizeof(size ) );
    if ( error == 0 )
        error = SockReceive( (char *) msg, size );
    if ( error == 0 )
    {
        if ( trace_settings & (TRACE_REQUEST | TRACE_PROCESS) )
        {
            char desc[200];
            char* descp = desc;
            switch ( msg->u.reply.type )
            {
            case ReplyType_DelProcessNs:
                sprintf( desc, "DelProcessNs, nid=%d, pid=%d, verifier=%d, name=%s, rc=%d\n",
                         msg->u.reply.u.del_process_ns.nid,
                         msg->u.reply.u.del_process_ns.pid,
                         msg->u.reply.u.del_process_ns.verifier,
                         msg->u.reply.u.del_process_ns.process_name,
                         msg->u.reply.u.del_process_ns.return_code );
                break;
            case ReplyType_Generic:
                sprintf( desc, "Generic, nid=%d, pid=%d, verifier=%d, name=%s, rc=%d\n",
                         msg->u.reply.u.generic.nid,
                         msg->u.reply.u.generic.pid,
                         msg->u.reply.u.generic.verifier,
                         msg->u.reply.u.generic.process_name,
                         msg->u.reply.u.generic.return_code );
                break;
            case ReplyType_NewProcessNs:
                sprintf( desc, "NewProcessNs, nid=%d, pid=%d, verifier=%d, name=%s, rc=%d\n",
                         msg->u.reply.u.new_process_ns.nid,
                         msg->u.reply.u.new_process_ns.pid,
                         msg->u.reply.u.new_process_ns.verifier,
                         msg->u.reply.u.new_process_ns.process_name,
                         msg->u.reply.u.new_process_ns.return_code );
                break;
            case ReplyType_ProcessInfo:
                sprintf( desc, "ProcessInfo, num_processes=%d, rc=%d, more_data=%d\n",
                         msg->u.reply.u.process_info.num_processes,
                         msg->u.reply.u.process_info.return_code,
                         msg->u.reply.u.process_info.more_data );
                break;
            default:
                descp = (char *) "UNKNOWN";
                break;
            }
            trace_printf( "%s@%d - msgType=%d, replyType=%d, ReplyType=%s\n"
                        , method_name, __LINE__
                        , msg->type, msg->u.reply.type
                        , descp
                        );
        }
    }

    TRACE_EXIT;
    return error;
}

int CNameServer::SendToNs( const char *reqType, struct message_def *msg, int size )
{
    const char method_name[] = "CNameServer::SendToNs";
    TRACE_ENTRY;

    if ( trace_settings & (TRACE_REQUEST | TRACE_PROCESS) )
    {
        trace_printf( "%s@%d - sending %s REQ to nameserver=%s, sock=%d\n"
                    , method_name, __LINE__
                    , reqType
                    , mon2nsPort_
                    , mon2nsSock_ );
    }

    int error = 0;
    if ( mon2nsSock_ < 0 )
        error = ConnectToNs();
    if ( error == 0 )
        error = SockSend( (char *) &size, sizeof(size) );
    if ( error == 0 )
        error = SockSend( (char *) msg, size );

    TRACE_EXIT;
    return error;
}

int CNameServer::SockReceive( char *buf, int size )
{
    const char method_name[] = "CNameServer::SockReceive";
    TRACE_ENTRY;

    bool    readAgain = false;
    int     error = 0;
    int     readCount = 0;
    int     received = 0;
    int     sizeCount = size;
    
    do
    {
        readCount = (int) recv( mon2nsSock_
                              , buf
                              , sizeCount
                              , 0 );
    
        if ( trace_settings & (TRACE_REQUEST | TRACE_PROCESS) )
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

    if ( trace_settings & (TRACE_REQUEST | TRACE_PROCESS) )
    {
        trace_printf( "%s@%d - recv(), received=%d, error=%d(%s)\n"
                    , method_name, __LINE__
                    , received
                    , error, strerror(error) );
    }

    if ( error )
        SockClose();

    TRACE_EXIT;
    return error;
}

int CNameServer::SockSend( char *buf, int size )
{
    const char method_name[] = "CNameServer::SockSend";
    TRACE_ENTRY;

    bool    sendAgain = false;
    int     error = 0;
    int     sendCount = 0;
    int     sent = 0;
    
    do
    {
        sendCount = (int) send( mon2nsSock_
                              , buf
                              , size
                              , 0 );
    
        if ( trace_settings & (TRACE_REQUEST | TRACE_PROCESS) )
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

    if ( trace_settings & (TRACE_REQUEST | TRACE_PROCESS) )
    {
        trace_printf( "%s@%d - send(), sent=%d, error=%d(%s)\n"
                    , method_name, __LINE__
                    , sent
                    , error, strerror(error) );
    }

    if ( error )
        SockClose();

    TRACE_EXIT;
    return error;
}
