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

#ifndef NSCOMMACCEPTMON_H
#define NSCOMMACCEPTMON_H

#include <pthread.h>
#include "lock.h"
#include "reqqueue.h"

class CCommAcceptMon : public CLock
{
public:

    CCommAcceptMon();
    virtual ~CCommAcceptMon();

    void commAcceptor( void );
    bool isAccepting( void ) { CAutoLock lock(getLocker()); return( accepting_ ); }
    void processNewSock( int sockFd );
    void processMonReqs( int sockFd );
    int  processMonReqsGetBestNs( void );
    void monReqDeleteProcess( struct message_def* msg, int sockFd );
    void monReqExec( CExternalReq * request );
    void monReqNameServerStop( struct message_def* msg, int sockFd );
    void monReqNewProcess( struct message_def* msg, int sockFd );
    void monReqNodeDown( struct message_def* msg, int sockFd );
    void monReqProcessInfo( struct message_def* msg, int sockFd );
    void monReqProcessInfoCont( struct message_def* msg, int sockFd );
    void monReqProcessInfoNs( struct message_def* msg, int sockFd );
    void monReqShutdown( struct message_def* msg, int sockFd );
    void monReqUnknown( struct message_def* msg, int sockFd );
    void startAccepting( void );
    void stopAccepting( void );
    void start( void );
    void shutdownWork( void );

    typedef struct
    {
        CCommAcceptMon *this_;
        int             pendingFd_;
    } Context;

private:
    void commAcceptorSock( void );

    bool accepting_;
    bool shutdown_;

    // mon2nsAcceptMon thread's id
    pthread_t                      thread_id_;
    // mon2nsProcess thread's id
    pthread_t                      process_thread_id_;

    enum { HEURISTIC_COUNT = 10 };
};

#endif
