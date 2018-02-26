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

CNameServer::CNameServer (void)
: mon2nsSock_(0)
, seqNum_(0)
{
    const char method_name[] = "CNameServer::CNameServer";
    TRACE_ENTRY;

    TRACE_EXIT;
}

CNameServer::~CNameServer (void)
{
    const char method_name[] = "CNameServer::~CNameServer";
    TRACE_ENTRY;

    TRACE_EXIT;
}

int CNameServer::MkCltSock( const char *portName )
{
    const char method_name[] = "CNameServer::MkCltSock";
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

    colon = strstr(portName, ":");
    strcpy(host, portName);
    int len = colon - portName;
    host[len] = '\0';
    port = atoi(&colon[1]);
    
    size = sizeof(sockinfo);

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
                    , method_name, err, strerror( err ));
            mon_log_write(MON_CLUSTER_MKCLTSOCK_1, SQ_LOG_ERR, la_buf); 
            return ( -1 );
        }

        he = gethostbyname( host );
        if ( !he )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            int err = errno;
            snprintf( la_buf, sizeof(la_buf), 
                      "[%s] gethostbyname(%s) failed! errno=%d (%s)\n"
                    , method_name, host, err, strerror( err ));
            mon_log_write(MON_CLUSTER_MKCLTSOCK_2, SQ_LOG_ERR, la_buf); 
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
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
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
            if ( errno == EINTR )
            {
                ++connect_failures;
            }
            else
            {
                char la_buf[MON_STRING_BUF_SIZE];
                int err = errno;
                sprintf( la_buf, "[%s], connect() failed! errno=%d (%s)\n"
                       , method_name, err, strerror( err ));
                mon_log_write(MON_CLUSTER_MKCLTSOCK_3, SQ_LOG_ERR, la_buf); 
                close(sock);
                return ( -1 );
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
                       , method_name, retries);
                mon_log_write(MON_CLUSTER_MKCLTSOCK_4, SQ_LOG_ERR, la_buf); 
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
               , method_name, err, strerror( err ));
        mon_log_write(MON_CLUSTER_MKCLTSOCK_5, SQ_LOG_ERR, la_buf); 
        close( (int)sock );
        return ( -2 );
    }

    TRACE_EXIT;
    return ( sock );
}

int CNameServer::InitializeNameServer( void )
{
    const char method_name[] = "CNameServer::InitializeNameServer";
    TRACE_ENTRY;
    int err = 0;
    char   *p;     // getenv results 

    gethostname(mon2nsPort_, MAX_PROCESSOR_NAME);
    strcat(mon2nsPort_, ":");
    p = getenv( "MON2NAMESERVER_COMM_PORT" );
    if (p)
        strcat(mon2nsPort_, p);
    else
        strcat(mon2nsPort_, "3000");

    int sock = MkCltSock(mon2nsPort_);
    if (sock < 0)
        err = sock;
    else
    {
        mon2nsSock_ = sock;
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - connected to nameserver=%s, sock=%d\n"
                        , method_name, __LINE__
                        , mon2nsPort_
                        , mon2nsSock_ );
        }
    }

    if (err == 0)
    {
        nodeId_t msg;
        strcpy(msg.nodeName, MyNode->GetName());
        strcpy(msg.commPort, MyNode->GetCommPort());
        strcpy(msg.syncPort, MyNode->GetSyncPort());
        msg.pnid = MyNode->GetPNid();
        msg.creatorPNid = -1;
        msg.creatorShellPid = -1;
        msg.creatorShellVerifier = -1;
        msg.creator = false;
        msg.ping = false;
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - sending node-info to nameserver=%s, sock=%d\n"
                        , method_name, __LINE__
                        , mon2nsPort_
                        , mon2nsSock_);
        }
        err = SendSock((char *) &msg, sizeof(msg), mon2nsSock_);
        if (err)
        {
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d - error sending to nameserver=%s, sock=%d, error=%d\n"
                            , method_name, __LINE__
                            , mon2nsPort_
                            , mon2nsSock_
                            , err );
            }
        }
    }

    TRACE_EXIT;
    return err;
}

int CNameServer::ProcessInfo(struct ProcessInfo_def* infoReq)
{
    const char method_name[] = "CNameServer::ProcessInfo";
    TRACE_ENTRY;
    struct message_def msg;
    memset(&msg, 0, sizeof(msg)); // TODO: remove!
    msg.type = MsgType_Service;
    msg.noreply = false;
    msg.reply_tag = seqNum_++;
    msg.u.request.type = ReqType_ProcessInfo;
    struct ProcessInfo_def *msginfo = &msg.u.request.u.process_info;
    msginfo->nid = infoReq->nid;
    msginfo->pid = infoReq->pid;
    msginfo->verifier = infoReq->verifier;
    strcpy(msginfo->process_name, infoReq->process_name);
    msginfo->target_nid = infoReq->target_nid;
    msginfo->target_pid = infoReq->target_pid;
    msginfo->target_verifier = infoReq->target_verifier;
    strcpy(msginfo->target_process_name, infoReq->target_process_name);
    strcpy(msginfo->target_process_pattern, infoReq->target_process_pattern);
    msginfo->type = infoReq->type;
    int size = offsetof(struct message_def, u) +
               sizeof(REQTYPE) +
               sizeof(msg.u.request.u.process_info);

    int error = SendToNs("process-info", &msg, size);
    if (error == 0)
    {
        size = offsetof(struct message_def, u) +
               sizeof(REPLYTYPE);
        error = ReceiveSock((char *) &msg, size, mon2nsSock_);
        if (error)
        {
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d - error receiving (hdr) from nameserver=%s, sock=%d, error=%d\n"
                            , method_name, __LINE__
                            , mon2nsPort_
                            , mon2nsSock_
                            , error );
            }
        }
        else
        {
            size = sizeof( struct ProcessInfo_reply_def );
            error = ReceiveSock((char *) &msg.u.reply.u.process_info, size, mon2nsSock_);
            if (error)
            {
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d - error receiving (data) from nameserver=%s, sock=%d, error=%d\n"
                                , method_name, __LINE__
                                , mon2nsPort_
                                , mon2nsSock_
                                , error );
                }
            }
        }
    }

    TRACE_EXIT;
    return error;
}

int CNameServer::NewProcess(CProcess* process)
{
    const char method_name[] = "CNameServer::NewProcess";
    TRACE_ENTRY;

    struct message_def msg;
    memset(&msg, 0, sizeof(msg)); // TODO: remove!
    msg.type = MsgType_Service;
    msg.noreply = false;
    msg.reply_tag = seqNum_++;
    msg.u.request.type = ReqType_NewProcessNs;
    struct NewProcessNs_def *msgnew = &msg.u.request.u.new_process_ns;
    CProcess* parent = process->GetParent();
    if (parent)
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
    strcpy(msgnew->process_name, process->GetName());
    int size = offsetof(struct message_def, u) +
               sizeof(REQTYPE) +
               sizeof(msg.u.request.u.new_process_ns);

    int error = SendToNs("new-process", &msg, size);

    TRACE_EXIT;
    return error;
}

int CNameServer::ReceiveSock(char *buf, int size, int sockFd)
{
    const char method_name[] = "CNameServer::ReceiveSock";
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

int CNameServer::SendSock(char *buf, int size, int sockFd)
{
    const char method_name[] = "CNameServer::SendSock";
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

int CNameServer::SendToNs(const char *reqType, struct message_def *msg, int size)
{
    const char method_name[] = "CNameServer::SendToNs";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - sending %s REQ to nameserver=%s, sock=%d\n"
                    , method_name, __LINE__
                    , reqType
                    , mon2nsPort_
                    , mon2nsSock_);
    }
    int error = SendSock((char *) &size, sizeof(size), mon2nsSock_);
    if (error)
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - error sending to nameserver=%s, sock=%d, error=%d\n"
                        , method_name, __LINE__
                        , mon2nsPort_
                        , mon2nsSock_
                        , error );
        }
    }
    error = SendSock((char *) msg, size, mon2nsSock_);
    if (error)
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - error sending to nameserver=%s, sock=%d, error=%d\n"
                        , method_name, __LINE__
                        , mon2nsPort_
                        , mon2nsSock_
                        , error );
        }
    }
    TRACE_EXIT;
    return error;
}
