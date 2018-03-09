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

#ifndef NAMESERVER_H_
#define NAMESERVER_H_
#ifndef NAMESERVER_PROCESS

#include "process.h"


class CNameServer 
{
protected:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:

    CNameServer( void );
    virtual ~CNameServer( void );

    int  ProcessDelete(CProcess* process );
    int  ProcessInfo( struct message_def* msg );
    int  ProcessInfoCont( struct message_def* msg );
    int  ProcessNew(CProcess* process );

private:
    char mon2nsPort_[MAX_PROCESSOR_NAME+100];
    int  mon2nsSock_;
    bool nsStartupComplete_;
    int  seqNum_;

    int  ConnectToNs( void );
    int  SendReceive( struct message_def* msg );
    int  SendToNs( const char *reqType, struct message_def *msg, int size );
    void SockClose( void );
    int  SockCreate();
    int  SockReceive( char *buf, int size );
    int  SockSend( char *buf, int size );
};

#endif
#endif /*NAMESERVER_H_*/
