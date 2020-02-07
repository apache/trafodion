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

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "monlogging.h"
#include "montrace.h"
#include "comm.h"

const char *EpollEventString( __uint32_t events );
const char *EpollOpString( int op );

CComm::CComm( void )
      :epollFd_(-1)
{
    const char method_name[] = "CComm::CComm";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "COMM", 4);

    epollFd_ = epoll_create1( EPOLL_CLOEXEC );
    if ( epollFd_ < 0 )
    {
        char ebuff[256];
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s@%d] epoll_create1(sendrecv) error: %s\n",
            method_name, __LINE__, strerror_r( errno, ebuff, 256 ) );
        mon_log_write( COMM_COMM_1, SQ_LOG_CRIT, buf );

        mon_failure_exit();
    }

    TRACE_EXIT;
}

CComm::~CComm( void )
{
    const char method_name[] = "CComm::~CComm";
    TRACE_ENTRY;

    if (epollFd_ != -1)
    {
        close( epollFd_ );
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "comm", 4);

    TRACE_EXIT;
}

int CComm::Accept( int listenSock )
{
    const char method_name[] = "CComm::Accept";
    TRACE_ENTRY;

#if defined(_XOPEN_SOURCE_EXTENDED)
#ifdef __LP64__
    socklen_t  size;    // size of socket address
#else
    size_t   size;      // size of socket address
#endif
#else
    int    size;        // size of socket address
#endif
    int csock; // connected socket
    struct sockaddr_in  sockinfo;   // socket address info

    size = sizeof(struct sockaddr *);
    if ( getsockname( listenSock, (struct sockaddr *) &sockinfo, &size ) )
    {
        char buf[MON_STRING_BUF_SIZE];
        int err = errno;
        snprintf(buf, sizeof(buf), "[%s], getsockname() failed, errno=%d (%s).\n",
                 method_name, err, strerror(err));
        mon_log_write(COMM_ACCEPT_1, SQ_LOG_ERR, buf);
        return ( -1 );
    }

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        unsigned char *addrp = (unsigned char *) &sockinfo.sin_addr.s_addr;
        trace_printf( "%s@%d - Accepting socket on addr=%d.%d.%d.%d,  port=%d\n"
                    , method_name, __LINE__
                    , addrp[0]
                    , addrp[1]
                    , addrp[2]
                    , addrp[3]
                    , (int) ntohs( sockinfo.sin_port ) );
    }

    while ( ((csock = accept( listenSock
                            , (struct sockaddr *) 0
                            , (socklen_t *) 0 ) ) < 0) && (errno == EINTR) );

    if ( csock > 0 )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            unsigned char *addrp = (unsigned char *) &sockinfo.sin_addr.s_addr;
            trace_printf( "%s@%d - Accepted socket on addr=%d.%d.%d.%d, "
                          "port=%d, listenSock=%d, csock=%d\n"
                        , method_name, __LINE__
                        , addrp[0]
                        , addrp[1]
                        , addrp[2]
                        , addrp[3]
                        , (int) ntohs( sockinfo.sin_port )
                        , listenSock
                        , csock );
        }

        int nodelay = 1;
        if ( setsockopt( csock
                       , IPPROTO_TCP
                       , TCP_NODELAY
                       , (char *) &nodelay
                       , sizeof(int) ) )
        {
            char buf[MON_STRING_BUF_SIZE];
            int err = errno;
            snprintf(buf, sizeof(buf), "[%s], setsockopt() failed, errno=%d (%s).\n",
                     method_name, err, strerror(err));
            mon_log_write(COMM_ACCEPT_2, SQ_LOG_ERR, buf);
            return ( -2 );
        }

        int reuse = 1;
        if ( setsockopt( csock
                       , SOL_SOCKET
                       , SO_REUSEADDR
                       , (char *) &reuse
                       , sizeof(int) ) )
        {
            char buf[MON_STRING_BUF_SIZE];
            int err = errno;
            snprintf(buf, sizeof(buf), "[%s], setsockopt() failed, errno=%d (%s).\n",
                     method_name, err, strerror(err));
            mon_log_write(COMM_ACCEPT_3, SQ_LOG_ERR, buf);
            return ( -2 );
        }
    }

    TRACE_EXIT;
    return ( csock );
}

void CComm::ConnectLocal( int port )
{
    const char method_name[] = "CComm::ConnectLocal";
    TRACE_ENTRY;

    int  sock;     // socket
    int  ret;      // returned value
#if defined(_XOPEN_SOURCE_EXTENDED)
#ifdef __LP64__
    socklen_t  size;    // size of socket address
#else
    size_t   size;      // size of socket address
#endif
#else
    int    size;        // size of socket address
#endif
    static int retries = 0;       // # times to retry connect
    int     connect_failures = 0; // # failed connects
    char   *p;     // getenv results
    struct sockaddr_in  sockinfo; // socket address info
    struct hostent *he;

    size = sizeof(sockinfo);

    if ( !retries )
    {
        p = getenv( "HPMP_CONNECT_RETRIES" );
        if ( p ) retries = atoi( p );
        else retries = 5;
    }

    sock = socket( AF_INET, SOCK_STREAM, 0 );
    if ( sock < 0 )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf( la_buf, "[%s], socket() failed! errno=%d (%s)\n"
               , method_name, err, strerror( err ));
        mon_log_write( COMM_CONNECTLOCAL_1, SQ_LOG_CRIT, la_buf );

        mon_failure_exit();
    }

    he = gethostbyname( "localhost" );
    if ( !he )
    {
        char ebuff[256];
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s@%d] gethostbyname(%s) error: %s\n",
            method_name, __LINE__, "localhost", strerror_r( h_errno, ebuff, 256 ) );
        mon_log_write( COMM_CONNECTLOCAL_2, SQ_LOG_CRIT, buf );

        mon_failure_exit();
    }

    // Connect socket.
    memset( (char *) &sockinfo, 0, size );
    memcpy( (char *) &sockinfo.sin_addr, (char *) he->h_addr, 4 );
    sockinfo.sin_family = AF_INET;
    sockinfo.sin_port = htons( (unsigned short) port );

    connect_failures = 0;
    ret = 1;
    while ( ret != 0 && connect_failures <= 10 )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - Connecting to localhost addr=%d.%d.%d.%d, port=%d, connect_failures=%d\n"
                        , method_name, __LINE__
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
            mon_log_write(COMM_CONNECTLOCAL_3, SQ_LOG_CRIT, la_buf);

            mon_failure_exit();
        }
    }

    close( sock );

    TRACE_EXIT;
}

int CComm::Connect( const char *portName, bool doRetries )
{
    const char method_name[] = "CComm::Connect";
    TRACE_ENTRY;

    int  sock;      // socket
    int  ret;       // returned value
    int  nodelay = 1; // sockopt reuse option
    int  reuse = 1; // sockopt reuse option
#if defined(_XOPEN_SOURCE_EXTENDED)
#ifdef __LP64__
    socklen_t  size;    // size of socket address
#else
    size_t   size;      // size of socket address
#endif
#else
    int    size;        // size of socket address
#endif
    static int retries = 0;      // # times to retry connect
    int    outer_failures = 0;   // # failed connect loops
    int    connect_failures = 0; // # failed connects
    char   *p;     // getenv results
    struct sockaddr_in  sockinfo; // socket address info
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

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - Connecting to %s:%d\n"
                    , method_name, __LINE__
                    , host
                    , port );
    }

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
            sprintf( la_buf, "[%s], socket() failed! errno=%d (%s)\n"
                   , method_name, err, strerror( err ));
            mon_log_write( COMM_CONNECT_1, SQ_LOG_ERR, la_buf );
            return ( -1 );
        }

        he = gethostbyname( host );
        if ( !he )
        {
            char ebuff[256];
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s@%d] gethostbyname(%s) error: %s\n",
                method_name, __LINE__, host, strerror_r( h_errno, ebuff, 256 ) );
            mon_log_write( COMM_CONNECT_2, SQ_LOG_ERR, buf );
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
                trace_printf( "%s@%d - Connecting to %s, addr=%d.%d.%d.%d, "
                              "port=%d, doRetries=%d, connect_failures=%d\n"
                            , method_name, __LINE__
                            , portName
                            , (int)((unsigned char *)he->h_addr)[0]
                            , (int)((unsigned char *)he->h_addr)[1]
                            , (int)((unsigned char *)he->h_addr)[2]
                            , (int)((unsigned char *)he->h_addr)[3]
                            , port
                            , doRetries
                            , connect_failures );
            }

            ret = connect( sock, (struct sockaddr *) &sockinfo, size );
            if ( ret == 0 ) break;

            ++connect_failures;

#ifdef NAMESERVER_PROCESS
            if ( errno == ECONNREFUSED )
            {
                ++connect_failures;
                sleep( 1 );
            }
            else
#endif
            if ( errno != EINTR )
            {
                if (doRetries)
                {
                    char la_buf[MON_STRING_BUF_SIZE];
                    int err = errno;
                    sprintf( la_buf, "[%s], connect(%s) failed! errno=%d (%s)\n"
                           , method_name, portName, err, strerror( err ));
                    mon_log_write( COMM_CONNECT_3, SQ_LOG_ERR, la_buf );
                    struct timespec req, rem;
                    req.tv_sec = 0;
                    req.tv_nsec = 500000000L; // 500,000,000
                    nanosleep( &req, &rem );
                }
                else
                {
                    char la_buf[MON_STRING_BUF_SIZE];
                    int err = errno;
                    sprintf( la_buf, "[%s], connect(%s) failed! errno=%d (%s)\n"
                           , method_name, portName, err, strerror( err ));
                    mon_log_write( COMM_CONNECT_4, SQ_LOG_ERR, la_buf );
                    close(sock);
                    return ( -1 );
                }
            }
        } // while

        if ( ret == 0 ) break;

        if (doRetries == false)
        {
            close( sock );
            TRACE_EXIT;
            return( -1 );
        }

        // For large clusters, the connect/accept calls seem to fail occasionally,
        // no doubt do to the large number (1000's) of simultaneous connect packets
        // flooding the network at once.  So, we retry up to HPMP_CONNECT_RETRIES
        // number of times.
        if ( errno != EINTR )
        {
            if ( ++outer_failures > retries )
            {
                char la_buf[MON_STRING_BUF_SIZE];
                sprintf( la_buf, "[%s], connect(%s) exceeded retries! count=%d\n"
                       , method_name, portName, retries);
                mon_log_write( COMM_CONNECT_5, SQ_LOG_ERR, la_buf );
                close( sock );
                TRACE_EXIT;
                return ( -1 );
            }
            struct timespec req, rem;
            req.tv_sec = 0;
            req.tv_nsec = 500000; // 500,000
            nanosleep( &req, &rem );
        }
        close( sock );
        sock = -1;
    } // for

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - Connected to %s addr=%d.%d.%d.%d, port=%d, sock=%d\n"
                    , method_name, __LINE__
                    , host
                    , (int)((unsigned char *)he->h_addr)[0]
                    , (int)((unsigned char *)he->h_addr)[1]
                    , (int)((unsigned char *)he->h_addr)[2]
                    , (int)((unsigned char *)he->h_addr)[3]
                    , port
                    , sock );
    }

    if ( setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, (char *) &nodelay, sizeof(int) ) )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf( la_buf, "[%s], setsockopt() failed! errno=%d (%s)\n"
               , method_name, err, strerror( err ));
        mon_log_write( COMM_CONNECT_6, SQ_LOG_ERR, la_buf );
        close( sock );
        TRACE_EXIT;
        return ( -2 );
    }

    if ( setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(int) ) )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf( la_buf, "[%s], setsockopt() failed! errno=%d (%s)\n"
               , method_name, err, strerror( err ));
        mon_log_write( COMM_CONNECT_7, SQ_LOG_ERR, la_buf );
        close( sock );
        TRACE_EXIT;
        return ( -2 );
    }

    TRACE_EXIT;
    return ( sock );
}

int CComm::Connect( unsigned char srcip[4]
                  , unsigned char dstip[4]
                  , int port )
{
    const char method_name[] = "CComm::Connect";
    TRACE_ENTRY;

    int    sock;        // socket
    int    ret;         // returned value
    int    reuse = 1;   // sockopt reuse option
    int    nodelay = 1; // sockopt nodelay option
#if defined(_XOPEN_SOURCE_EXTENDED)
#ifdef __LP64__
    socklen_t  size;    // size of socket address
#else
    size_t   size;      // size of socket address
#endif
#else
    int    size;        // size of socket address
#endif
    static int retries = 0;      // # times to retry connect
    int    outer_failures = 0;   // # failed connect loops
    int    connect_failures = 0; // # failed connects
    char   *p;     // getenv results
    struct sockaddr_in  sockinfo;    // socket address info

    size = sizeof(sockinfo);
    const char * size_srcip = (const char *) srcip;

    if ( !retries )
    {
        p = getenv( "HPMP_CONNECT_RETRIES" );
        if ( p ) retries = atoi( p );
        else retries = 5;
    }

    for ( ;; )
    {
        sock = socket( AF_INET, SOCK_STREAM, 0 );
        if ( sock < 0 ) return ( -1 );

        // Bind local address if specified.
        if ( srcip )
        {
            memset( (char *) &sockinfo, 0, size );
            memcpy( (char *) &sockinfo.sin_addr,
                (unsigned char *) srcip, strlen(size_srcip));

            sockinfo.sin_family = AF_INET;
            sockinfo.sin_port = 0;
            if ( bind( sock, (struct sockaddr *) &sockinfo, size ) )
            {
                char la_buf[MON_STRING_BUF_SIZE];
                int err = errno;
                sprintf( la_buf, "[%s], bind() failed! errno=%d (%s)\n"
                       , method_name, err, strerror( err ));
                mon_log_write( COMM_CONNECT_13, SQ_LOG_ERR, la_buf );
                close( sock );
                return ( -1 );
            }
        }

        // Connect socket.
        memset( (char *) &sockinfo, 0, size );
        memcpy( (char *) &sockinfo.sin_addr, (char *) dstip, 4 );
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
                trace_printf( "%s@%d - Connecting to addr=%d.%d.%d.%d, port=%d, connect_failures=%d\n"
                            , method_name, __LINE__
                            , (int)dstip[0]
                            , (int)dstip[1]
                            , (int)dstip[2]
                            , (int)dstip[3]
                            , port
                            , connect_failures );
            }

            ret = connect( sock, (struct sockaddr *) &sockinfo,
                size );
            if ( ret == 0 ) break;
            if ( errno == EINTR )
            {
                ++connect_failures;
            }
#ifdef NAMESERVER_PROCESS
            else if ( errno == ECONNREFUSED )
            {
                ++connect_failures;
                sleep( 1 );
            }
#endif
            else
            {
                char la_buf[MON_STRING_BUF_SIZE];
                int err = errno;
                sprintf( la_buf, "[%s], connect(%d.%d.%d.%d:%d) failed! errno=%d (%s)\n"
                       , method_name
                       , (int)((unsigned char *)dstip)[0]
                       , (int)((unsigned char *)dstip)[1]
                       , (int)((unsigned char *)dstip)[2]
                       , (int)((unsigned char *)dstip)[3]
                       , port
                       , err, strerror( err ));
                mon_log_write( COMM_CONNECT_14, SQ_LOG_ERR, la_buf );
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
                mon_log_write( COMM_CONNECT_15, SQ_LOG_ERR, la_buf );
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

    if ( setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, (char *) &nodelay, sizeof(int) ) )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf( la_buf, "[%s], setsockopt() failed! errno=%d (%s)\n"
               , method_name, err, strerror( err ));
        mon_log_write( COMM_CONNECT_16, SQ_LOG_ERR, la_buf );
        close( sock );
        return ( -2 );
    }

    if ( setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(int) ) )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf( la_buf, "[%s], setsockopt() failed! errno=%d (%s)\n"
               , method_name, err, strerror( err ));
        mon_log_write( COMM_CONNECT_17, SQ_LOG_ERR, la_buf );
        close( sock );
        return ( -2 );
    }

    int lv_retcode = SetKeepAliveSockOpt( sock );
    if ( lv_retcode != 0 )
    {
        return lv_retcode;
    }

    TRACE_EXIT;
    return ( sock );
}

int CComm::Close( int sock )
{
    const char method_name[] = "CComm::Close";
    TRACE_ENTRY;

    int rc = 0;
    if (sock != -1)
    {
        rc = close( sock );
    }

    TRACE_EXIT;
    return( rc );
}

void CComm::EpollCtl( int efd
                    , int op
                    , int fd
                    , struct epoll_event *event
                    , char *remoteName )
{
    const char method_name[] = "CComm::EpollCtl";
    TRACE_ENTRY;
#if 0
    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d epoll_ctl( efd=%d,%s, fd=%d(%s), %s )\n"
                    , method_name, __LINE__
                    , efd
                    , EpollOpString(op)
                    , fd
                    , remoteName
                    , EpollEventString(event->events) );
    }
#endif
    int rc = epoll_ctl( efd, op, fd, event );
    if ( rc == -1 )
    {
        char ebuff[256];
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s@%d] epoll_ctl(efd=%d, %s, fd=%d(%s), %s) error: %s\n"
                , method_name, __LINE__
                , efd
                , EpollOpString(op)
                , fd
                , remoteName
                , EpollEventString(event->events)
                , strerror_r( errno, ebuff, 256 ) );
        mon_log_write( COMM_EPOLLCTL_1, SQ_LOG_CRIT, buf );

        mon_failure_exit();
    }

    TRACE_EXIT;
    return;
}

void CComm::EpollCtlDelete( int efd
                          , int fd
                          , struct epoll_event *event
                          , char *remoteName )
{
    const char method_name[] = "CComm::EpollCtlDelete";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d epoll_ctl( efd=%d,%s, fd=%d(%s), %s )\n"
                    , method_name, __LINE__
                    , efd
                    , EpollOpString(EPOLL_CTL_DEL)
                    , fd
                    , remoteName
                    , EpollEventString(event->events) );
    }

    // Remove old socket from epoll set, it may not be there
    int rc = epoll_ctl( efd, EPOLL_CTL_DEL, fd, event  );
    if ( rc == -1 )
    {
        int err = errno;
        if (err != ENOENT)
        {
            char ebuff[256];
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s@%d] epoll_ctl(efd=%d, %s, fd=%d, %s) error: %s\n"
                    , method_name, __LINE__
                    , efd
                    , EpollOpString(EPOLL_CTL_DEL)
                    , fd
                    , EpollEventString(event->events)
                    , strerror_r( err, ebuff, 256 ) );
            mon_log_write( COMM_EPOLLCTLDELETE_1, SQ_LOG_CRIT, buf );

            mon_failure_exit();
        }
    }

    TRACE_EXIT;
    return;
}

int CComm::Listen( int *port )
{
    const char method_name[] = "CComm::Listen";
    TRACE_ENTRY;

    int  sock;     // socket
    int  err;      // return code
#if defined(_XOPEN_SOURCE_EXTENDED)
#ifdef __LP64__
    socklen_t size; // size of socket address
#else
    size_t    size; // size of socket address
#endif
#else
    unsigned int size; // size of socket address
#endif
    struct sockaddr_in  sockinfo;   // socket address info
    sock = socket( AF_INET, SOCK_STREAM, 0 );
    if ( sock < 0 )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf( la_buf, "[%s], socket() failed! errno=%d (%s)\n"
               , method_name, err, strerror( err ));
        mon_log_write( COMM_LISTEN_1, SQ_LOG_CRIT, la_buf );
        return ( -1 );
    }

    int    nodelay = 1;   // sockopt nodelay option
    if ( setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, (char *) &nodelay, sizeof(int) ) )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf( la_buf, "[%s], setsockopt() failed! errno=%d (%s)\n"
               , method_name, err, strerror( err ));
        mon_log_write( COMM_LISTEN_2, SQ_LOG_ERR, la_buf );
        close( sock );
        return ( -2 );
    }

    int    reuse = 1;   // sockopt reuse option
    if ( setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(int) ) )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf( la_buf, "[%s], setsockopt(SO_REUSEADDR) failed! errno=%d (%s)\n"
               , method_name, err, strerror( err ));
        mon_log_write( COMM_LISTEN_3, SQ_LOG_ERR, la_buf );
        close( sock );
        return ( -1 );
    }

    // Bind socket.
    size = sizeof(sockinfo);
    memset( (char *) &sockinfo, 0, size );
    sockinfo.sin_family = AF_INET;
    sockinfo.sin_addr.s_addr = htonl( INADDR_ANY );
    sockinfo.sin_port = htons( *port );
    int lv_bind_tries = 0;
    do
    {
        if (lv_bind_tries > 0)
        {
            sleep(5);
        }
        err = bind( sock, (struct sockaddr *) &sockinfo, size );
        sched_yield( );
    } while ( err &&
             (errno == EADDRINUSE) &&
             (++lv_bind_tries < 4) );
    if ( err )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf( la_buf, "[%s], bind() failed! port=%d, errno=%d (%s)\n"
               , method_name, *port, err, strerror( err ));
        mon_log_write( COMM_LISTEN_4, SQ_LOG_CRIT, la_buf );
        close( sock );
        return ( -1 );
    }
    if ( port )
    {
        if ( getsockname( sock, (struct sockaddr *) &sockinfo, &size ) )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            int err = errno;
            sprintf( la_buf, "[%s], getsockname() failed! errno=%d (%s)\n"
                   , method_name, err, strerror( err ));
            mon_log_write( COMM_LISTEN_5, SQ_LOG_CRIT, la_buf );
            close( sock );
            return ( -1 );
        }

        *port = (int) ntohs( sockinfo.sin_port );
    }
    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        unsigned char *addrp = (unsigned char *) &sockinfo.sin_addr.s_addr;
        trace_printf( "%s@%d listening on addr=%d.%d.%d.%d, port=%d\n"
                    , method_name, __LINE__
                    , addrp[0]
                    , addrp[1]
                    , addrp[2]
                    , addrp[3]
                    , port?*port:0);
    }

    int lv_retcode = SetKeepAliveSockOpt( sock );
    if ( lv_retcode != 0 )
    {
        return lv_retcode;
    }

    // Listen
    if ( listen( sock, SOMAXCONN ) )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf( la_buf, "[%s], listen() failed! errno=%d (%s)\n"
               , method_name, err, strerror( err ));
        mon_log_write( COMM_LISTEN_6, SQ_LOG_CRIT, la_buf );
        close( sock );
        return ( -1 );
    }

    TRACE_EXIT;
    return ( sock );
}

int CComm::Listen( const char *portName, int *port )
{
    const char method_name[] = "CComm::Listen";
    TRACE_ENTRY;

    *port = atoi(portName);

    TRACE_EXIT;
    return( CComm::Listen( port ) );
}

int CComm::Receive( int sockFd
                  , char *buf
                  , int size
                  , char *remoteName
                  , const char *desc )
{
    const char method_name[] = "CComm::Receive";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS | TRACE_NS))
    {
        trace_printf( "%s@%d (%s) - read from %s, sockFd=%d, size=%d\n"
                    , method_name, __LINE__, desc
                    , remoteName
                    , sockFd
                    , size );
    }

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
        //if ( readCount > 0 ) Meas.addSockNsRcvdBytes( readCount );
    
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS | TRACE_NS))
        {
            trace_printf( "%s@%d (%s) - read from %s, sockFd=%d, "
                          "count read=%d, sizeCount=%d\n"
                        , method_name, __LINE__, desc
                        , remoteName, sockFd
                        , readCount, sizeCount );
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

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS | TRACE_NS))
    {
        trace_printf( "%s@%d - recv(), received=%d, error=%d(%s)\n"
                    , method_name, __LINE__
                    , received
                    , error, strerror(error) );
    }

    if (error)
    {
        int err = error;
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s](%s), unable to receive request from %s, size %d,"
                  "sockFd=%d, error: %d(%s)\n"
                , method_name, desc, remoteName, size
                , sockFd, err, strerror(err) );
        mon_log_write( COMM_RECEIVE_1, SQ_LOG_ERR, buf );    
    }

    TRACE_EXIT;
    return error;
}

int CComm::ReceiveWait( int sockFd
                      , char *buf
                      , int size
                      , int timeout
                      , int retries
                      , char *remoteName
                      , const char *desc)
{
    const char method_name[] = "CComm::ReceiveWait";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_SYNC))
    {
        trace_printf( "%s@%d (%s) - read from %s, sockFd=%d, size=%d\n"
                    , method_name, __LINE__, desc
                    , remoteName
                    , sockFd
                    , size );
    }

    int error = SendRecvWait( sockFd
                            , NULL // char *sendbuf
                            , 0    // int sendsize
                            , buf
                            , size
                            , timeout
                            , retries
                            , remoteName
                            , desc);

    TRACE_EXIT;
    return error;
}

int CComm::Send( int sockFd
               , char *buf
               , int size
               , char *remoteName
               , const char *desc )
{
    const char method_name[] = "CComm::Send";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS | TRACE_NS))
    {
        trace_printf( "%s@%d (%s) - read from %s, sockFd=%d, size=%d\n"
                    , method_name, __LINE__, desc
                    , remoteName
                    , sockFd
                    , size );
    }

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
        //if ( sendCount > 0 ) Meas.addSockNsSentBytes( sendCount );
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
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS | TRACE_NS))
        {
            trace_printf( "%s@%d (%s) - send to %s, sockFd=%d, "
                          "count size=%d, sendCount=%d, sent=%d\n"
                        , method_name, __LINE__, desc
                        , remoteName, sockFd
                        , size, sendCount, sent );
        }
    }
    while( sendAgain );

    if ( trace_settings & TRACE_NS )
    {
        trace_printf( "%s@%d - send(), sent=%d, error=%d(%s)\n"
                    , method_name, __LINE__
                    , sent
                    , error, strerror(error) );
    }

    if (error)
    {
        int err = error;
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s](%s), unable to send request to %s, size %d,"
                  "sockFd=%d, error: %d(%s)\n"
                , method_name, desc, remoteName, size
                , sockFd, err, strerror(err) );
        mon_log_write( COMM_SEND_1, SQ_LOG_ERR, buf );
    }

    TRACE_EXIT;
    return error;
}

int CComm::SendWait(int sockFd
                   , char *buf
                   , int size
                   , int timeout
                   , int retries
                   , char *remoteName
                   , const char *desc)
{
    const char method_name[] = "CComm::SendWait";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_SYNC))
    {
        trace_printf( "%s@%d (%s) - write to %s, sockFd=%d, size=%d\n"
                    , method_name, __LINE__, desc
                    , remoteName
                    , sockFd
                    , size );
    }

    int error = SendRecvWait( sockFd
                            , buf
                            , size
                            , NULL // char *recvbuf
                            , 0    // int recvsize
                            , timeout
                            , retries
                            , remoteName
                            , desc);

    TRACE_EXIT;
    return error;
}

int CComm::SendRecv( int sockFd
                   , char *sendbuf
                   , int sendsize
                   , char *recvbuf
                   , int recvsize
                   , char *remoteName
                   , const char *desc)
{
    const char method_name[] = "CComm::SendRecv";
    TRACE_ENTRY;

    int error = Send( sockFd
                    , sendbuf
                    , sendsize
                    , remoteName
                    , desc);
    if (error != 0)
    {
        TRACE_EXIT;
        return error;
    }

    error = Receive( sockFd
                   , recvbuf
                   , recvsize
                   , remoteName
                   , desc);

    TRACE_EXIT;
    return error;
}

int CComm::SendRecvWait( int sockFd
                       , char *sendbuf
                       , int sendsize
                       , char *recvbuf
                       , int recvsize
                       , int timeout
                       , int retries
                       , char *remoteName
                       , const char *desc)
{
    const char method_name[] = "CComm::SendRecvWait";
    TRACE_ENTRY;

    bool receiving = (recvsize > 0) ? true : false;
    bool sending = (sendsize > 0) ? true : false;
    bool sendAgain = false;
    int error = 0;
    int nrecv = 0;
    int nsent = 0;
    int num2recv = (receiving) ? 1 : 0;
    int num2send = (sending) ? 1 : 0;
    int received = 0;
    int sent = 0;
    int retry = 0;

    struct epoll_event event;
    event.data.fd = sockFd;
    event.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_SYNC))
    {
        trace_printf( "%s@%d (%s) - EPOLL state change "
                      "to/from %s, efd=%d, op=%s, sockFd=%d, event=%s\n"
                    , method_name, __LINE__, desc
                    , remoteName
                    , epollFd_
                    , EpollOpString(EPOLL_CTL_ADD)
                    , sockFd
                    , EpollEventString(event.events) );

    }
    EpollCtl( epollFd_, EPOLL_CTL_ADD, sockFd, &event, remoteName );

    if (trace_settings & (TRACE_SYNC_DETAIL))
    {
        trace_printf( "%s@%d (%s) - write/read to/from %s, sockFd=%d, "
                      "sending=%d, sendsize=%d, receiving=%d, recvsize=%d, "
                      "wait_timeout=%d, retry_count=%d\n"
                    , method_name, __LINE__, desc
                    , remoteName
                    , sockFd
                    , sending
                    , sendsize
                    , receiving
                    , recvsize
                    , timeout
                    , retries );
    }

    struct epoll_event ioEvents;
    while(1)
    {
        bool stateChange = false;
        int maxEvents = num2recv + num2send - nsent - nrecv;
        int nw;

        if (trace_settings & (TRACE_SYNC_DETAIL))
        {
            trace_printf( "%s@%d (%s) - write/read to/from %s, "
                          "sockFd=%d, maxEvents=%d, nsent=%d, nrecv=%d, "
                          "sending=%d (%d), "
                          "sent=%d, "
                          "receiving=%d (%d), "
                          "received=%d\n"
                        , method_name, __LINE__, desc, remoteName
                        , sockFd, maxEvents, nsent, nrecv
                        , sending, sendsize
                        , sent
                        , receiving, recvsize
                        , received );
        }

do_again:

        if (maxEvents == 0) break;

        while(1)
        {
            nw = epoll_wait( epollFd_, &ioEvents, maxEvents, timeout );
            if ( nw >= 0 || errno != EINTR ) break;
        }

        if ( nw == 0 )
        { // Timeout, no fd's ready
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_SYNC))
            {
                trace_printf( "%s@%d (%s) - IO timedout! (node=%s, retry=%d, "
                              "timeout=%d, retries=%d) - "
                              "sending=%d (%d), "
                              "sent=%d, "
                              "receiving=%d (%d), "
                              "received=%d\n"
                            , method_name, __LINE__
                            , desc, remoteName, retry, timeout, retries
                            , sending, sendsize
                            , sent
                            , receiving, recvsize
                            , received );
            }

            retry++;
            if (retry < retries)
            {
                goto do_again;
            }

            error = ETIMEDOUT;
            if ( sending )
            {
                nsent++;
                sending = false;
            }
            if ( receiving )
            {
                nrecv++;
                receiving = false;
            }
            stateChange = true;
            goto early_exit;
        }  // ( nw == 0 )

        if (nw < 0)
        { // Got an error
            char ebuff[256];
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s@%d] (%s) - epoll_wait(%d, %d) error: %s\n",
                method_name, __LINE__, desc, epollFd_, maxEvents,
                strerror_r( errno, ebuff, 256 ) );
            mon_log_write( COMM_SENDRECVWAIT_1, SQ_LOG_CRIT, buf );

            mon_failure_exit();
        }
        else
        {
            if ((ioEvents.events & EPOLLERR) ||
                (ioEvents.events & EPOLLHUP) ||
                (!(ioEvents.events & (EPOLLIN|EPOLLOUT))))
            {
                // An error has occurred on this fd, or the socket is not
                // ready for reading nor writing
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf)
                        , "[%s@%d] (%s) - Error: node=%s, events.data.fd=%d, event=%s\n"
                        , method_name, __LINE__, desc
                        , remoteName
                        , ioEvents.data.fd
                        , EpollEventString(ioEvents.events) );
                mon_log_write( COMM_SENDRECVWAIT_2, SQ_LOG_CRIT, buf );
                error = -1;
                if ( sending )
                {
                    nsent++;
                    sending = false;
                }
                if ( receiving )
                {
                    nrecv++;
                    receiving = false;
                }
                stateChange = true;
                goto early_exit;
            }
            if (receiving && ioEvents.events & EPOLLIN)
            { // Got receive (read) completion
                int eagain_ok = 0;
                int recverror = 0;
read_again:
                char *r = &((char *)recvbuf)[received];
                int n2get = recvsize - received;
                int nr = 0;
                while ( 1 )
                {
                    if (trace_settings & (TRACE_SYNC_DETAIL))
                    {
                        trace_printf( "%s@%d (%s) - EPOLLIN from %s,"
                                      " receiving=%d (%d)"
                                      " received=%d"
                                      " nr=%d"
                                      " n2get=%d"
                                      " recverror=%d(%s)\n"
                                    , method_name, __LINE__, desc
                                    , remoteName
                                    , receiving, n2get
                                    , received
                                    , nr
                                    , n2get
                                    , recverror, strerror(recverror) );
                    }
                    nr = recv( sockFd, r, n2get, 0 );
                    recverror = errno;
//                    if ( nr > 0 ) Meas.addSockRcvdBytes( (nr<n2get)?nr:0 );
                    if ( nr >= 0 || recverror == EINTR ) break;
                    if ( nr > n2get || nr <= 0 ) break;
                }
                
                if (nr > n2get || nr <= 0)
                {
                    if (recverror == 0)
                    { // Timeout
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_SYNC))
                        {
                            trace_printf( "%s@%d (%s) - IO timeout!\n"
                                        , method_name, __LINE__, desc );
                        }
                    }
                    else
                    {
                        char buf[MON_STRING_BUF_SIZE];
                        snprintf( buf, sizeof(buf)
                                , "[%s@%d] (%s) - recv(%d) from %s error %d (%s)\n"
                                , method_name, __LINE__, desc
                                , nr
                                , remoteName
                                , recverror, strerror(recverror) );
                        mon_log_write( COMM_SENDRECVWAIT_3, SQ_LOG_CRIT, buf );
                    }
                    error = -1;
                    nrecv++;
                    receiving = false;
                    stateChange = true;
                    goto early_exit;
                }

                if ( nr < 0 )
                {
                    if ( nr < 0 && eagain_ok && recverror == EAGAIN )
                    {
                        // do nothing
                    }
                    else
                    {
                        // error, down socket
                        char buf[MON_STRING_BUF_SIZE];
                        snprintf( buf, sizeof(buf)
                                , "[%s@%d] (%s) - recv(%d) from %s error %d (%s)\n"
                                , method_name, __LINE__, desc
                                , nr
                                , remoteName
                                , recverror, strerror(recverror) );
                        mon_log_write( COMM_SENDRECVWAIT_4, SQ_LOG_CRIT, buf );
                        nrecv++;
                        receiving = false;
                        if ( sending )
                        {
                            nsent++;
                            sending = false;
                        }
                        stateChange = true;
                    }
                }
                else
                {
                    received += nr;
                    // reading buffer, update counters
                    n2get -= nr;
                    if ( n2get > 0 )
                    {
                        eagain_ok = 1;
                        goto read_again;
                    }
                    if ( n2get < 0 )
                    {
                        char buf[MON_STRING_BUF_SIZE];
                        snprintf( buf, sizeof(buf),
                            "[%s@%d] (%s) - error n2get=%d\n",
                            method_name, __LINE__, desc, n2get );
                        mon_log_write( COMM_SENDRECVWAIT_5, SQ_LOG_CRIT, buf );

                        mon_failure_exit();
                    }
                    if ( n2get == 0 )
                    {
                        // this buffer is done
                        nrecv++;
                        receiving = false;
                        if (trace_settings & (TRACE_SYNC_DETAIL))
                        {
                            trace_printf( "%s@%d (%s) - EPOLLIN from %s,"
                                          " receiving=%d (%d)"
                                          " received=%d"
                                          " n2get=%d\n"
                                        , method_name, __LINE__, desc
                                        , remoteName
                                        , receiving, n2get
                                        , received
                                        , n2get );
                        }
                        stateChange = true;
                    }
                }
            }
            if (sending && ioEvents.events & EPOLLOUT)
            { // Got send (write) completion
                char *s = &((char *)sendbuf)[sent];
                int n2send = sendsize - sent;
                int ns;
                int senderror = 0;
                while ( 1 )
                {
                    if (trace_settings & (TRACE_SYNC_DETAIL))
                    {
                        trace_printf( "%s@%d (%s) - EPOLLOUT to %s,"
                                      " sending=%d (%d),"
                                      " sent=%d, "
                                      " recverror=%d(%s)\n"
                                    , method_name, __LINE__, desc
                                    , remoteName
                                    , sendsize, n2send
                                    , sent
                                    , senderror, strerror(senderror) );
                    }
                    ns = send( sockFd, s, n2send, 0 );
                    int senderror = errno;
//                    if ( ns > 0 ) Meas.addSockSentBytes( ns );
                    if ( ns >= 0 || senderror != EINTR ) break;
                    if ( ns > n2send || ns <= 0 ) break;
                }

                if (ns > n2send || ns <= 0)
                { // Timeout
                    if (senderror == 0)
                    { // Timeout
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_SYNC))
                        {
                            trace_printf( "%s@%d (%s) - IO timeout!\n"
                                        , method_name, __LINE__, desc );
                        }
                    }
                    else
                    {
                        char buf[MON_STRING_BUF_SIZE];
                        snprintf( buf, sizeof(buf)
                                , "[%s@%d] (%s) - send(%d) to %s, error=%d (%s)\n"
                                , method_name, __LINE__, desc
                                , ns
                                , remoteName
                                , senderror, strerror(senderror) );
                        mon_log_write( COMM_SENDRECVWAIT_6, SQ_LOG_CRIT, buf );
                    }
                    error = -1;
                    nsent++;
                    sending = false;
                    stateChange = true;
                    goto early_exit;
                }

                if ( ns < 0 )
                { // error, down socket
                    char buf[MON_STRING_BUF_SIZE];
                    snprintf( buf, sizeof(buf)
                            , "[%s@%d] (%s) - send(%d) to %s, error=%d (%s)\n"
                            , method_name, __LINE__, desc
                            , ns
                            , remoteName
                            , senderror, strerror(senderror) );
                    mon_log_write( COMM_SENDRECVWAIT_7, SQ_LOG_CRIT, buf );
                    nsent++;
                    sending = false;
                    if ( receiving )
                    {
                        nrecv++;
                        receiving = false;
                    }
                    stateChange = true;
                }
                else
                {
                    sent += ns;
                    if ( sent == sendsize )
                    {
                        nsent++;
                        sending = false;
                        // finished sending to this destination
                        if (trace_settings & (TRACE_SYNC_DETAIL))
                        {
                            trace_printf( "%s@%d (%s) - EPOLLOUT to %s,"
                                          " sending=%d (%d),"
                                          " sent=%d\n"
                                        , method_name, __LINE__, desc
                                        , remoteName
                                        , sendsize, n2send
                                        , sent );
                        }
                        stateChange = true;
                    }
                }
            }
early_exit:
            if (stateChange)
            {
                struct epoll_event event;
                event.data.fd = sockFd;
                int op = 0;
                if ( !sending && !receiving )
                {
                    op = EPOLL_CTL_DEL;
                    event.events = 0;
                }
                else if (sending)
                {
                    op = EPOLL_CTL_MOD;
                    event.events = EPOLLOUT | EPOLLET | EPOLLRDHUP;
                }
                else if (receiving)
                {
                    op = EPOLL_CTL_MOD;
                    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                }
                if ( op == EPOLL_CTL_DEL || op == EPOLL_CTL_MOD )
                {
                    if (trace_settings & (TRACE_SYNC_DETAIL))
                    {
                        trace_printf( "%s@%d (%s) - EPOLL state change "
                                      "to/from %s, efd=%d, op=%s, sockFd=%d, event=%s\n"
                                    , method_name, __LINE__, desc
                                    , remoteName
                                    , epollFd_
                                    , EpollOpString(op)
                                    , sockFd
                                    , EpollEventString(event.events) );
    
                    }
                    EpollCtl( epollFd_, op, sockFd, &event, remoteName );
                    if (op == EPOLL_CTL_DEL)
                    {
                        if (trace_settings & (TRACE_SYNC_DETAIL))
                        {
                            trace_printf( "%s@%d (%s) - write/read to/from %s, "
                                          "removed socket from epoll set, "
                                          "sockFd=%d, event=%s\n"
                                        , method_name, __LINE__, desc
                                        , remoteName
                                        , sockFd
                                        , EpollEventString(event.events) );

                        }
                    }
                }
            }
        }
    }

    TRACE_EXIT;
    return error;
}

int CComm::SetKeepAliveSockOpt( int sock )
{
    const char method_name[] = "CComm::SetKeepAliveSockOpt";
    TRACE_ENTRY;

    static int sv_keepalive = -1;
    static int sv_keepidle  = 120;
    static int sv_keepintvl = 12;
    static int sv_keepcnt   = 5;

    if ( sv_keepalive == -1 )
    {
        char *lv_keepalive_env = getenv( "SQ_MON_KEEPALIVE" );
        if ( lv_keepalive_env )
        {
            sv_keepalive = atoi( lv_keepalive_env );
        }
        if ( sv_keepalive == 1 )
        {
            char *lv_keepidle_env = getenv( "SQ_MON_KEEPIDLE" );
            if ( lv_keepidle_env )
            {
                sv_keepidle = atoi( lv_keepidle_env );
            }
            char *lv_keepintvl_env = getenv( "SQ_MON_KEEPINTVL" );
            if ( lv_keepintvl_env )
            {
                sv_keepintvl = atoi( lv_keepintvl_env );
            }
            char *lv_keepcnt_env = getenv( "SQ_MON_KEEPCNT" );
            if ( lv_keepcnt_env )
            {
                sv_keepcnt = atoi( lv_keepcnt_env );
            }
        }
    }

    if ( sv_keepalive == 1 )
    {
        if ( setsockopt( sock, SOL_SOCKET, SO_KEEPALIVE, &sv_keepalive, sizeof(int) ) )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            int err = errno;
            sprintf( la_buf, "[%s], setsockopt so_keepalive() failed! errno=%d (%s)\n"
                   , method_name, err, strerror( err ) );
            mon_log_write( COMM_SETKEEPALIVESOCKOPT_1, SQ_LOG_ERR, la_buf );
            close( sock );
            return ( -2 );
        }

        if ( setsockopt( sock, SOL_TCP, TCP_KEEPIDLE, &sv_keepidle, sizeof(int) ) )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            int err = errno;
            sprintf( la_buf, "[%s], setsockopt tcp_keepidle() failed! errno=%d (%s)\n"
                   , method_name, err, strerror( err ) );
            mon_log_write( COMM_SETKEEPALIVESOCKOPT_2, SQ_LOG_ERR, la_buf );
            close( sock );
            return ( -2 );
        }

        if ( setsockopt( sock, SOL_TCP, TCP_KEEPINTVL, &sv_keepintvl, sizeof(int) ) )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            int err = errno;
            sprintf( la_buf, "[%s], setsockopt tcp_keepintvl() failed! errno=%d (%s)\n"
                   , method_name, err, strerror( err ) );
            mon_log_write( COMM_SETKEEPALIVESOCKOPT_3, SQ_LOG_ERR, la_buf );
            close( sock );
            return ( -2 );
        }

        if ( setsockopt( sock, SOL_TCP, TCP_KEEPCNT, &sv_keepcnt, sizeof(int) ) )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            int err = errno;
            sprintf( la_buf, "[%s], setsockopt tcp_keepcnt() failed! errno=%d (%s)\n"
                   , method_name, err, strerror( err ) );
            mon_log_write( COMM_SETKEEPALIVESOCKOPT_4, SQ_LOG_ERR, la_buf );
            close( sock );
            return ( -2 );
        }
    }

    TRACE_EXIT;
    return ( 0 );
}

