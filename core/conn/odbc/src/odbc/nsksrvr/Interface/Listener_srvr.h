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

#ifndef NSK_LISTENER_SRVR_H
#define NSK_LISTENER_SRVR_H

#include "Listener.h"

#define LISTEN_TAG 1900


class CNSKListenerSrvr: public CNSKListener
{

public:
	CNSKListenerSrvr();
	~CNSKListenerSrvr();

	virtual int runProgram(char* TcpProcessName, long port, int TransportTrace=0);
	bool ListenToPort(int port);
	bool verifyPortAvailable(const char * idForPort, int portNumber);
	int getListenSocketFnum(){return m_nListenSocketFnum;};

	void SYSTEM_SNAMP(FILE* fp);
	void TCP_PROCESSNAME_PORT(FILE* fp);

	// SQ_PORT : Added for a compile error in QSMGR
	char* getTcpProcessName() { return m_TcpProcessName; };
	long getPort() { return m_port; };

	void closeTCPIPSession(int fnum);
       KEEPALIVE_OPT keepaliveOpt;
       void TCP_SetKeepalive(int socketnum, bool keepaliveStatus, int idleTime, int intervalTime, int retryCount);
protected:
	long m_port;
	CURR_TCPIP_OPER m_tcpip_operation;
	char m_TcpProcessName[50]; 
	void* OpenTCPIPSession();
	void* CheckTCPIPRequest(void* pnode);



    fd_set read_fds_;  // read  fds monitored by "select"
	fd_set error_fds_; // error fds monitored by "select"
    int max_read_fd_;  // max value of fd
	SB_Thread::Errorcheck_Mutex *doingRequest; // mutex 
    ERROR_TYPE errorType_; // Token used in calls to SET_ERROR to identify error source.
    
    int pipefd[2]; // used for a dummy write on tcp/ip thread to stop executing (used instead of a thread_cancel())

public:        
    SB_Thread::Sthr tcpip_listener_thr;
    void* tcpip_tid;
        
    static void * tcpip_listener(void *arg); // tcp/ip listener thread
    void terminateThreads(int status);

};

#endif
