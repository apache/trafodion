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

#include <platform_ndcs.h>
#include "Transport.h"
#include "Listener_srvr.h"
#include "TCPIPSystemSrvr.h"
#include "FileSystemSrvr.h"

#include "Global.h"

extern SRVR_GLOBAL_Def *srvrGlobal;  // needed in the platform specific implementation file
//extern void flushCollectors();       // needed in the platform specific implementation file

CNSKListenerSrvr::CNSKListenerSrvr()
{
	SRVRTRACE_ENTER(FILE_LSNS+1);
	m_port = 0;
	m_TraceCount = 0;
	m_tcpip_operation = CURR_UNDEFINED;
	m_bIPv4 = false;
	m_TcpProcessName[0] = 0;
	doingRequest = new SB_Thread::Errorcheck_Mutex(true);
	pipefd[0] = 0; //read fd
	pipefd[1] = 0; // write fd
	SRVRTRACE_EXIT(FILE_LSNS+1);
}

CNSKListenerSrvr::~CNSKListenerSrvr()
{
	SRVRTRACE_ENTER(FILE_LSNS+2);
	if (m_nListenSocketFnum > 0)
	{
	    closeTCPIPSession(m_nListenSocketFnum);
	}
	delete doingRequest;
	SRVRTRACE_EXIT(FILE_LSNS+2);
}

//void* CNSKListenerSrvr::OpenTCPIPSession()
//moved to platform specific implementation file
void* CNSKListenerSrvr::CheckTCPIPRequest(void* ipnode)
{
	SRVRTRACE_ENTER(FILE_LSNS+4);
	CTCPIPSystemSrvr* pnode = (CTCPIPSystemSrvr*) ipnode;
	pnode = (CTCPIPSystemSrvr*)PROCESS_TCPIP_REQUEST(pnode);
	SRVRTRACE_EXIT(FILE_LSNS+4);
	return (void*)pnode;
}

//bool CNSKListenerSrvr::ListenToPort(long port)
//moved to platform specific implementation file
//int CNSKListenerSrvr::runProgram(char* TcpProcessName, long port, int TransportTrace)
//moved to platform specific implementation file

//================= Trace ===========================

//void CNSKListenerSrvr::SYSTEM_SNAMP(FILE* fp)
//moved to platform specific implementation file

void CNSKListenerSrvr::TCP_PROCESSNAME_PORT(FILE* fp)
{
	fprintf(fp,"<==========TCP/PORT (%s/%d)==========>\n",m_TcpProcessName, m_port);
}

void CNSKListenerSrvr::TCP_SetKeepalive(int socketnum,
                                        bool keepaliveStatus,
                                        int idleTime,
                                        int intervalTime,
                                        int retryCount)
{
    //all need to be configured
    if(!keepaliveStatus){
        keepaliveOpt.isKeepalive = DEFAULT_KEEPALIVE;
        keepaliveOpt.keepaliveIdle = DEFAULT_KEEPALIVE_TIMESEC;
        keepaliveOpt.keepaliveInterval = DEFAULT_KEEPALIVE_INTVL;
        keepaliveOpt.keepCount = DEFAULT_KEEPALIVE_COUNT;
    }else{
        keepaliveOpt.isKeepalive = 1;
        keepaliveOpt.keepaliveIdle = idleTime;
        keepaliveOpt.keepaliveInterval = intervalTime;
        keepaliveOpt.keepCount = retryCount;
    }

    int error;
    error += setsockopt(socketnum, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepaliveOpt.isKeepalive , sizeof(keepaliveOpt.isKeepalive));
    error += setsockopt(socketnum, SOL_TCP, TCP_KEEPIDLE, (void*)&keepaliveOpt.keepaliveIdle , sizeof(keepaliveOpt.keepaliveIdle ));
    error += setsockopt(socketnum, SOL_TCP, TCP_KEEPINTVL, (void *)&keepaliveOpt.keepaliveInterval , sizeof(keepaliveOpt.keepaliveInterval ));
    error += setsockopt(socketnum, SOL_TCP, TCP_KEEPCNT, (void *)&keepaliveOpt.keepCount , sizeof(keepaliveOpt.keepCount ));

    if (error != 0){
        SET_WARNING((long)0, NSK, TCPIP, UNKNOWN_API, errorType_,
                                  "set socket keepalive opt error", O_INIT_PROCESS, F_SOCKET, 0, 0);
    }
}
