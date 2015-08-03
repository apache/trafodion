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

#ifndef NSK_LISTENER_H
#define NSK_LISTENER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in6.h>
#include <netdb.h>

enum CURR_TCPIP_OPER{
	CURR_UNDEFINED,
	CURR_OPEN,
	CURR_PROCESS,
	CURR_OTHER
};

#define INITIALIZE_TRACE(TransportTrace) \
	m_TransportTrace = TransportTrace; \
	if (m_TransportTrace) { \
		OpenTraceFile(); \
	}

#define TRACE_INPUT(fnum,countRead,tag,cc) \
	if (m_TransportTrace) { \
		TraceInput(fnum,countRead,tag,cc); \
	}

#define TRACE_UNKNOWN_INPUT() \
	if (m_TransportTrace) { \
		TraceUnknownInput(); \
	}

#define FS_TRACE_OUTPUT(cc) \
	if (m_TransportTrace) { \
		FSTraceOutput(cc); \
	}

#define TCP_TRACE_OUTPUT(hdr) \
	if (m_TransportTrace) { \
		TCPIPTraceOutput(hdr); \
	}

#define TCP_TRACE_OUTPUT_R0() \
	if (m_TransportTrace) { \
		TCPIPTraceOutputR0(); \
	}

#define TCP_TRACE_OUTPUT_CC(cc) \
	if (m_TransportTrace) { \
		TCPIPTraceOutputCC(cc); \
	}

#define LISTEN_ON_SOCKET(socket) \
	if (m_TransportTrace) { \
		ListenOnSocket(socket); \
	}

char* frmt_serverstate(long serverstate);

extern void FORMAT_SRVR_APIS(int api, char* buffer);

//================== CTimer ============================

class CTimer 
{
public:

	CTimer( long seconds, long microseconds,CEE_timer_expiration_ptr e_routine, CEE_tag_def u_tag, 
				CEE_handle_def *thandle, short t_tag, CTimer* cnext);
	~CTimer();
public:
	void timer_restart();
	CEE_timer_expiration_ptr m_expiration_routine;
	CEE_tag_def m_user_tag;
	CEE_handle_def m_timer_handle;
	short m_timer_tag;
	long m_seconds;
	long m_microseconds;
private:
	CTimer* next;

	friend CTimer_list;
};

//================== CTimerList ============================

class CTimer_list 
{
public:
	CTimer_list();
	~CTimer_list();
	CEE_status timer_create(long seconds,long microseconds, CEE_timer_expiration_ptr e_routine, CEE_tag_def u_tag, CEE_handle_def *thandle );
	void timer_destroy(const CEE_handle_def *thandle);
	CTimer* find_timerByhandle(const CEE_handle_def* handle);
	void del_timer_node(CTimer* timer_node);
public:
	CEE_handle_def m_timer_handle;
private:
	CTimer* list;
};

extern CEE_status
CEE_TIMER_CREATE(
  /* In  */ long seconds,
  /* In  */ long microseconds,
  /* In  */ CEE_timer_expiration_ptr expiration_routine,
  /* In  */ CEE_tag_def timer_tag,
  /* Out */ CEE_handle_def *timer_handle
);

extern CEE_status
CEE_TIMER_DESTROY(
  /* In  */ const CEE_handle_def *timer_handle
);

void
BUILD_TIMER_MSG_CALL(
	   const CEE_handle_def *call_id_ 
	,  void *request 
	,  short countRead
	,  short* receive_info
);

void
BUILD_SYSTEM_MSG_CALL(
	   const CEE_handle_def *call_id_ 
	,  void *request 
	,  short countRead
	,  short* receive_info
);

void
BUILD_USER_MSG_CALL(
	   const CEE_handle_def *call_id_ 
	,  void *request 
	,  short countRead
	,  short *receive_info
);

class CNSKListener
{

public:
	CNSKListener();
	~CNSKListener();

	virtual int runProgram(char* TcpProcessName, long port, int TransportTrace=0);
public:
	int  m_TraceCount;
	int	 m_TransportTrace;
	char m_TraceFileName[80];

	virtual void SYSTEM_SNAMP(FILE* fp);
	virtual void TCP_PROCESSNAME_PORT(FILE* fp);

	void OpenTraceFile();
	FILE* OpenTraceFileA();
	void TraceInput(short fnum,short countRead,long tag,_cc_status cc);
	void TraceUnknownInput();
	void TCPIPTraceOutput(HEADER* phdr);
	void TCPIPTraceOutputR0();
	void TCPIPTraceOutputCC(_cc_status cc);
	void FSTraceOutput(_cc_status cc);
	void ListenOnSocket(short socket);
	FILE* OnCountReOpenFile();
	void finish_trace(FILE* fp);

protected:
	bool			m_bIPv4;
	short			m_nListenSocketFnum;
	short			m_nSocketFnum;
	sockaddr_in		m_ListenSocketAddr;
	sockaddr_in		m_AcceptFromSocketAddr;
	sockaddr_in6	m_ListenSocketAddr6;
	sockaddr_in6	m_AcceptFromSocketAddr6;
	int				m_nAcceptFromSocketAddrLen;

protected:

	void CheckReceiveMessage(_cc_status &cc, short countRead, CEE_handle_def* call_id);
	CEE_handle_def m_call_id;
	bool	m_bKeepRunning;
	short	m_ReceiveFnum;
	short	m_SignaltimeoutTag;
	char	m_RequestBuf[MAX_BUFFER_LENGTH];
private:
};

#endif
