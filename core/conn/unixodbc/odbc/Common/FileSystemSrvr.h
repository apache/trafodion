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

#ifndef FILE_SYSTEM_SRVR_H
#define FILE_SYSTEM_SRVR_H

#include "Transport.h"
#include "ceercv.h"

class CFSystemSrvr;
class CFSystemSrvr_list; 
class CTimer_list;
class CTempMemory_list; 

CEE_status
IOMessage_short_res_(
    /* In    */ short message_tag
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const CEERCV_IOMessage_exc_ *exception_
  , /* In    */ IDL_short error
  , /* In    */ const CEERCV_IOMessage_reply_seq_ *reply
  );

CEE_status 
RCV_IOMessage_res_(
	  CFSystemSrvr* pnode
	, char* buffer 
	, unsigned long message_length
	, /* In    */ const CEE_handle_def *call_id_
  );

void
IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ CEERCV_IOMessage_exc_ *exception_
  , /* In    */ const IDL_short *receiveInfo
  , /* In    */ IDL_short dialogInfo
  , /* In    */ const CEERCV_IOMessage_request_seq_ *request
  , /* Out   */ IDL_short *error
  , /* Out   */ CEERCV_IOMessage_reply_seq_ *reply
  );

void 
DISPATCH_PROCDEATH_SMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ CEERCV_SystemMessage_exc_ *exception_
  , /* In    */ const IDL_short *receiveInfo
  , /* In    */ const CEERCV_SystemMessage_request_seq_ *request
  , /* Out   */ IDL_short *error
  , /* Out   */ CEERCV_SystemMessage_reply_seq_ *reply
);

void DISPATCH_CPUDOWN_SMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ CEERCV_SystemMessage_exc_ *exception_
  , /* In    */ const IDL_short *receiveInfo
  , /* In    */ const CEERCV_SystemMessage_request_seq_ *request
  , /* Out   */ IDL_short *error
  , /* Out   */ CEERCV_SystemMessage_reply_seq_ *reply
);

void DISPATCH_OPEN_SMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ CEERCV_SystemMessage_exc_ *exception_
  , /* In    */ const IDL_short *receiveInfo
  , /* In    */ const CEERCV_SystemMessage_request_seq_ *request
  , /* Out   */ IDL_short *error
  , /* Out   */ CEERCV_SystemMessage_reply_seq_ *reply
);

void DISPATCH_CLOSE_SMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ CEERCV_SystemMessage_exc_ *exception_
  , /* In    */ const IDL_short *receiveInfo
  , /* In    */ const CEERCV_SystemMessage_request_seq_ *request
  , /* Out   */ IDL_short *error
  , /* Out   */ CEERCV_SystemMessage_reply_seq_ *reply
);

void DISPATCH_DEVINFO_SMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ CEERCV_SystemMessage_exc_ *exception_
  , /* In    */ const IDL_short *receiveInfo
  , /* In    */ const CEERCV_SystemMessage_request_seq_ *request
  , /* Out   */ IDL_short *error
  , /* Out   */ CEERCV_SystemMessage_reply_seq_ *reply
);

void DISPATCH_CONFIGINFO_SMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ CEERCV_SystemMessage_exc_ *exception_
  , /* In    */ const IDL_short *receiveInfo
  , /* In    */ const CEERCV_SystemMessage_request_seq_ *request
  , /* Out   */ IDL_short *error
  , /* Out   */ CEERCV_SystemMessage_reply_seq_ *reply
);

void 
DISPATCH_IOMessage( 
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ CEERCV_IOMessage_exc_ *exception_
  , /* In    */ const IDL_short *receiveInfo
  , /* In    */ IDL_short dialogInfo
  , /* In    */ const CEERCV_IOMessage_request_seq_ *request
  , /* Out   */ IDL_short *error
  , /* Out   */ CEERCV_IOMessage_reply_seq_ *reply
  ,	short operation_id
);

//-----------------------------------------------------

class CFSystemSrvr: public CInterface
{
public:
	CFSystemSrvr();
	CFSystemSrvr(const IDL_short *receiveInfo);
	CFSystemSrvr(const CEE_handle_def call_id);
	~CFSystemSrvr();
	void	cleanup();

	virtual	void process_swap( char* buffer );
	virtual	char* w_allocate(int size);
	virtual char* r_allocate(int size);
	virtual void w_release();
	virtual void r_release();
	virtual void w_assign(char* buffer, long length);
	virtual void r_assign(char* buffer, long length);
	virtual char* w_buffer();
	virtual char* r_buffer();
	virtual long w_buffer_length();
	virtual long r_buffer_length();
	virtual	char swap();
	virtual	char transport();
	virtual	void send_error(short error, short error_detail, const CEE_handle_def *call_id_);
	virtual	CEE_status send_response(char* buffer, unsigned long message_length, const CEE_handle_def *call_id_);

	CEE_handle_def m_call_id;

	char*	m_rbuffer;
	long	m_rbuffer_length;
	char*	m_wbuffer;
	long	m_wbuffer_length;
	char*	m_curptr;
	unsigned long m_curlength;
	short	m_max_reply_count;
	short	m_reply_count;
	short	m_filenum;
	short	m_processHandle[10];
	short	m_cpu;
	short	m_pin;
	long	m_nodenumer;
	char	m_procname[MAX_PROCESS_NAME+1];
	short	m_message_tag;
	long	m_trans_begin_tag;
	HEADER	m_rhdr;
	HEADER	m_whdr;
private:
	CFSystemSrvr*	next;

	friend CFSystemSrvr_list;
};

class CFSystemSrvr_list 
{
public:
	CFSystemSrvr_list();
	~CFSystemSrvr_list();
	void	cleanup(); 
	bool	isListEmpty();

	CFSystemSrvr*	ins_node( const IDL_short *receiveInfo, const CEE_handle_def* call_id);
	bool	del_node(const IDL_short *receiveInfo);
	CFSystemSrvr*	find_node(const IDL_short *receiveInfo);
	bool	del_nodeByProcessHandle(const IDL_short *processHandle);
	bool	del_nodeByCpu(const IDL_short cpu);
	CFSystemSrvr*	find_nodeByProcessHandle(const IDL_short *processHandle);
	CFSystemSrvr*	find_nodeByCpu(const IDL_short cpu);
	CFSystemSrvr*	find_nodeBycall_id(const CEE_handle_def* call_id);
	bool	cleanup_node(const IDL_short *receiveInfo);
	char*	enum_nodes(char* obuffer, FILE* fp);

private:
	CFSystemSrvr*	list;
};

//=========================================================================

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

#endif
