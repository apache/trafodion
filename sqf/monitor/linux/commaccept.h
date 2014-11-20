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

#ifndef COMMACCEPT_H
#define COMMACCEPT_H

#include <pthread.h>
#include <mpi.h>

class CCommAccept
{
public:

    CCommAccept();
    virtual ~CCommAccept();

    void commAcceptor( void );
    void processNewComm( MPI_Comm interComm );
//    void processNewSock( int sockFd );
    void start( void );
    void shutdownWork( void );

private:
    struct message_def *Notice( const char *msgText );

    void commAcceptorIB( void );
    void commAcceptorSock( void );
    bool sendNodeInfo ( MPI_Comm interComm );

    bool shutdown_;

    // commAccept thread's id
    pthread_t                      thread_id_;

};

#endif
