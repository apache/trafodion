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

#ifndef PTPSERVER_H_
#define PTPSERVER_H_
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

    int  InitializePtpClient( char * mon2monPort );
    int  NewProcess(CProcess* proces, int receiveNode, const char *hostName);
    int  ProcessInit(CProcess *process, void *tag, int result, int receiveNode, const char *hostName);
/* //TRK-TODO 
 Need methods for these message types:
            InternalType_Clone
            InternalType_Open
            InternalType_Notify
            InternalType_Exit
*/
private:
    int  basePort_;
    char mon2monPortBase_[MAX_PROCESSOR_NAME+100];
    int  mon2monSock_;
    int  seqNum_;

    int  MkCltSock( const char *portName );
    int  ReceiveSock(char *buf, int size, int sockFd);
    int  SendSock(char *buf, int size, int sockFd);
    int  SendToMon(const char *reqType, internal_msg_def *msg, int size, int receiveNode, const char *hostName);
};

#endif
#endif /*PTPSERVER_H_*/
