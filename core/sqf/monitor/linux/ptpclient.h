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

#ifndef PTPCLIENT_H_
#define PTPCLIENT_H_
#ifndef NAMESERVER_PROCESS

#include "process.h"
#include "internal.h"

class CPtpClient 
{
protected:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:

    CPtpClient( void );
    virtual ~CPtpClient( void );

    int  AddUniqStr( int nid
                   , int id
                   , const char *stringValue
                   , int targetNid
                   , const char *targetNodeName );
    int  InitializePtpClient( char * ptpPort );
    int  ProcessClone( CProcess *process );
    int  ProcessExit( CProcess* process
                    , int parentNid
                    , const char *targetNodeName );
    int  ProcessInit( CProcess *process
                    , void *tag
                    , int result
                    , int parentNid );
    int  ProcessKill( CProcess* process
                    , bool abort
                    , int targetNid
                    , const char *targetNodeName );
    int  ProcessNew( CProcess* process
                   , int targetNid
                   , const char *targetNodeName );
    int  ProcessNotify( int nid
                      , int pid
                      , Verifier_t verifier
                      , _TM_Txid_External transId
                      , bool canceled
                      , CProcess *targetProcess
                      , int targetNid
                      , const char *targetNodeName );
    int  StdInReq( int nid
                 , int pid
                 , StdinReqType type
                 , int supplierNid
                 , int supplierPid );
    int  StdIoData( int nid
                  , int pid
                  , StdIoType type
                  , ssize_t count
                  , char *data );

private:

    int  basePort_;
    char ptpHost_[MAX_PROCESSOR_NAME];
    char ptpPortBase_[MAX_PROCESSOR_NAME+100];
    int  ptpSock_;
    int  seqNum_;

    int  ReceiveSock(char *buf, int size, int sockFd);
    int  SendSock(char *buf, int size, int sockFd);
    int  SendToMon(const char *reqType, internal_msg_def *msg, int size, int receiveNode, const char *hostName);
    void SetLocalHost( void );
};

#endif
#endif /*PTPCLIENT_H_*/
