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

#ifndef COMM_H_
#define COMM_H_

class CComm
{
protected:
    int     eyecatcher_;    // Debuggging aid -- leave as first
                            // member variable of the class
public:

    CComm( void );
    ~CComm( void );

    int  Accept( int listenSock );
    void ConnectLocal( int port );
    int  Connect( const char *portName, bool doRetries = true );
    int  Connect( unsigned char srcip[4], unsigned char dstip[4], int port );
    int  Close( int sock );
    void EpollCtl( int efd
                 , int op
                 , int fd
                 , struct epoll_event *event
                 , char *remoteName );
    void EpollCtlDelete( int efd
                       , int fd
                       , struct epoll_event *event
                       , char *remoteName );
    int  Listen( int *pport );
    int  Listen( const char *portName, int *port );
    int  Receive( int sockFd
                , char *buf
                , int size
                , char *remoteName
                , const char *desc );
    int  ReceiveWait(int sockFd
                    , char *buf
                    , int size
                    , int timeout
                    , int retries
                    , char *remoteName
                    , const char *desc );
    int  Send( int sockFd
             , char *buf
             , int size
             , char *remoteName
             , const char *desc );
    int  SendWait(int sockFd
                 , char *buf
                 , int size
                 , int timeout
                 , int retries
                 , char *remoteName
                 , const char *desc );
    int  SendRecv( int sockFd
                  , char *sendbuf
                  , int sendsize
                  , char *recvbuf
                  , int recvsize
                  , char *remoteName
                  , const char *desc );
    int  SendRecvWait( int sockFd
                     , char *sendbuf
                     , int sendsize
                     , char *recvbuf
                     , int recvsize
                     , int timeout
                     , int retries
                     , char *remoteName
                     , const char *desc );

protected:

private:

    int  SetKeepAliveSockOpt( int sock );

    int epollFd_;
    int listenSock_;
};

#endif /*COMM_H_*/
