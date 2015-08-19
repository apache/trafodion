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
#include <fs\feerrors.h>
#include "zspic.h"
#include "zemsc.h"
#include "zinic.h"
#include "zmxoc.h"
#include "zcomc.h"
#include "ztcic.h"

#include "ceercv.h"
#include "zsysc.h"

#include <windows.h>
#include "tal.h"
#include "global.h"
#include "odbcCommon.h"
#include "Transport.h"
#include "Listener.h"

#include "FileSystemSrvr.h"

CFSystemSrvr::CFSystemSrvr()
{
}
CFSystemSrvr::CFSystemSrvr(const CEE_handle_def call_id)
{
}

CFSystemSrvr::CFSystemSrvr(const IDL_short *receiveInfo)
{
	short procname_len;

	memset(&m_call_id,0,sizeof(CEE_handle_def));

	m_wbuffer = NULL;
	m_rbuffer = NULL;
	m_curptr = NULL;
	m_curlength = 0;
	m_max_reply_count = 0;
	m_reply_count = 0;
	m_filenum = -1;
	memset(m_processHandle,0,20);
	m_trans_begin_tag = 0;

	m_filenum = receiveInfo[3];
	memcpy(m_processHandle,&receiveInfo[6],20);
	m_message_tag = receiveInfo[2];

	short error = PROCESSHANDLE_DECOMPOSE_ (
					m_processHandle
					,&m_cpu				/*cpu*/
					,&m_pin			    /*pin*/
					,&m_nodenumer	    /*nodenumber*/
					,				    /*nodename:nmax*/
					,
					,				    /*nlen*/
					,m_procname
					,MAX_PROCESS_NAME
					,&procname_len);
	m_procname[procname_len] = 0;

	next = NULL;
}

CFSystemSrvr::~CFSystemSrvr()
{
	RESET_ERRORS((long)this);
	cleanup();
}

char CFSystemSrvr::swap()
{
	return SWAP_NO;
}

char CFSystemSrvr::transport()
{
	return FILE_SYSTEM;
}

void CFSystemSrvr::cleanup()
{
	w_release();
	r_release();
}

char* CFSystemSrvr::w_allocate( int size)
{
	if (m_wbuffer != NULL)
		delete m_wbuffer;
	m_wbuffer = new char[size];
	if (m_wbuffer != NULL)
		m_wbuffer_length = size;
	else
		m_wbuffer_length = 0;
	return m_wbuffer;
}

char* CFSystemSrvr::r_allocate( int size)
{
	if (m_rbuffer != NULL)
		delete m_rbuffer;
	m_rbuffer = new char[size];
	if (m_rbuffer != NULL)
		m_rbuffer_length = size;
	else
		m_rbuffer_length = 0;
	return m_rbuffer;
}

void CFSystemSrvr::w_release()
{
	if (m_wbuffer != NULL)
		delete m_wbuffer;
	m_wbuffer = NULL;
	m_wbuffer_length = 0;
}

void CFSystemSrvr::r_release()
{
	if (m_rbuffer != NULL)
		delete m_rbuffer;
	m_rbuffer = NULL;
	m_rbuffer_length = 0;
}

void CFSystemSrvr::r_assign(char* buffer, long length)
{
	if (m_rbuffer != NULL)
		delete m_rbuffer;
	m_rbuffer = buffer;
	m_rbuffer_length = length;
}

void CFSystemSrvr::w_assign(char* buffer, long length)
{
	if (m_wbuffer != NULL)
		delete m_wbuffer;
	m_wbuffer = buffer;
	m_wbuffer_length = length;
}

char* CFSystemSrvr::w_buffer()
{
	return m_wbuffer;
}

char* CFSystemSrvr::r_buffer()
{
	return m_rbuffer;
}

long CFSystemSrvr::w_buffer_length()
{
	return m_wbuffer_length;
}

long CFSystemSrvr::r_buffer_length()
{
	return m_rbuffer_length;
}

void CFSystemSrvr::process_swap( char* buffer )
{
}

void CFSystemSrvr::send_error(short ierror, short ierror_detail, const CEE_handle_def *call_id_)
{
	IDL_short error;
	short reply_count;
	CEERCV_IOMessage_reply_seq_ reply;
	CEERCV_IOMessage_exc_ RCVexception_;

	RCVexception_.exception_nr = CEERCV_IOMessage_decline_exn_;
	reply._buffer = NULL;
	reply._length = 0;
	error=0;
	IOMessage_short_res_( m_message_tag, call_id_, &RCVexception_, error, &reply);
}

CEE_status CFSystemSrvr::send_response(char* buffer, unsigned long message_length, const CEE_handle_def *call_id_)
{
	return RCV_IOMessage_res_(this, buffer, message_length, call_id_);
}

CFSystemSrvr_list::CFSystemSrvr_list()
{
	list=NULL;
}
CFSystemSrvr_list::~CFSystemSrvr_list()
{
	cleanup();
}

bool CFSystemSrvr_list::isListEmpty()
{
	return list==NULL;
}

void CFSystemSrvr_list::cleanup() 
{
	CFSystemSrvr* cnode = list;
	CFSystemSrvr* nnode;
	while( cnode != NULL ){
		nnode = cnode->next;
		delete cnode;
		cnode = nnode;
	}
	list=NULL;
}

CFSystemSrvr* CFSystemSrvr_list::ins_node( const IDL_short *receiveInfo, const CEE_handle_def* call_id )
{
	CFSystemSrvr* cnode = list;
	CFSystemSrvr* pnode = list;
	CFSystemSrvr* nnode;

	del_node(receiveInfo);

	while(cnode!=NULL )
	{
		pnode=cnode;
		cnode=cnode->next;
	}
	if((nnode = (CFSystemSrvr*) new CFSystemSrvr(receiveInfo))!=NULL)
	{
		nnode->next = cnode;
		if(pnode!=NULL) 
			pnode->next = nnode;
		else
			list = nnode;
		memcpy(&nnode->m_call_id, call_id, sizeof(CEE_handle_def));
	}
	return nnode;
}

bool CFSystemSrvr_list::del_node(const IDL_short *receiveInfo)
{
	CFSystemSrvr* cnode = list;
	CFSystemSrvr* pnode = list;
	while( cnode!= NULL )
	{
		if ( receiveInfo[3] == cnode->m_filenum && memcmp(cnode->m_processHandle,&receiveInfo[6],20) == 0 )
			break;

		pnode = cnode;
		cnode = cnode->next;
	}
	if( cnode==NULL )
		return false;
	if (pnode == list && cnode == list)
		list = cnode->next;
	else
		pnode->next = cnode->next;
	delete cnode;
	return true;
}

CFSystemSrvr* CFSystemSrvr_list::find_node(const IDL_short *receiveInfo)
{
	CFSystemSrvr* cnode = list;

	while( cnode != NULL )
	{
		if (receiveInfo[3] == cnode->m_filenum && 
				memcmp(cnode->m_processHandle,&receiveInfo[6],20) == 0 )
			break;
		cnode = cnode->next;
	}
	return cnode;
}

bool CFSystemSrvr_list::cleanup_node(const IDL_short *receiveInfo)
{
	CFSystemSrvr* pnode;

	if ((pnode = find_node(receiveInfo)) != NULL)
		pnode->cleanup();
	return pnode != NULL;
}

bool CFSystemSrvr_list::del_nodeByProcessHandle(const IDL_short *processHandle)
{
	CFSystemSrvr* cnode = list;
	CFSystemSrvr* pnode = list;
	while( cnode != NULL )
	{
		if (memcmp(cnode->m_processHandle,processHandle,20) == 0 )
			break;
		pnode = cnode;
		cnode = cnode->next;
	}
	if(cnode==NULL)
		return false;
	if (pnode == list && cnode == list)
		list = cnode->next;
	else
		pnode->next = cnode->next;
	delete cnode;
	return true;
}

CFSystemSrvr* CFSystemSrvr_list::find_nodeByProcessHandle(const IDL_short *processHandle)
{
	CFSystemSrvr* cnode = list;
	CFSystemSrvr* pnode = list;
	while( cnode != NULL )
	{
		if (memcmp(cnode->m_processHandle,processHandle,20) == 0 )
			break;
		pnode = cnode;
		cnode = cnode->next;
	}
	return cnode;
}

bool CFSystemSrvr_list::del_nodeByCpu(const IDL_short cpu)
{
	CFSystemSrvr* cnode = list;
	CFSystemSrvr* pnode = list;
	while( cnode != NULL )
	{
		if ( cnode->m_cpu == cpu )
			break;
		pnode = cnode;
		cnode = cnode->next;
	}
	if(cnode==NULL)
		return false;
	if (pnode == list && cnode == list)
		list = cnode->next;
	else
		pnode->next = cnode->next;
	delete cnode;
	return true;
}

CFSystemSrvr* CFSystemSrvr_list::find_nodeByCpu(const IDL_short cpu)
{
	CFSystemSrvr* cnode = list;

	while( cnode!= NULL  )
	{
		if (cnode->m_cpu == cpu)
			break;
		cnode = cnode->next;
	}
	return cnode;
}

CFSystemSrvr* CFSystemSrvr_list::find_nodeBycall_id(const CEE_handle_def* call_id)
{
	CFSystemSrvr* cnode = list;

	while( cnode!= NULL )
	{
		if (memcmp(&cnode->m_call_id,call_id,sizeof(CEE_handle_def)) == 0 )
			break;
		cnode = cnode->next;
	}
	return cnode;
}

char* CFSystemSrvr_list::enum_nodes(char* obuffer, FILE* fp)
{
	CFSystemSrvr* cnode = list;
	char* pbuffer = obuffer;
	int	ip;

	ip=sprintf(pbuffer,"\t%s\n","<FS NODES>");
	while( cnode != NULL )
	{
		pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\t(%02d,%03d) %s\n","Process",cnode->m_cpu,cnode->m_pin, cnode->m_procname);
		cnode = cnode->next;
	}
	return pbuffer+ip;
}

//=========================================================================

void
BUILD_TIMER_MSG_CALL(
	   const CEE_handle_def *call_id_ 
	,  void *request 
	,  short countRead
	,  short* receive_info
)
{
	CEE_handle_def handle;
	typedef struct
	{
		short request_code;
		short param1;
		long  param2;
	} SignalTimeoutMsg_def;

	SignalTimeoutMsg_def* SignalTimeoutMsg = (SignalTimeoutMsg_def*)request;
	CEE_handle_def *thandle = (CEE_handle_def *)SignalTimeoutMsg->param2;

	REPLYX(,,,*(receive_info+2));

	CTimer* ptimer = GTransport.m_Timer_list->find_timerByhandle(thandle);
	if (ptimer == NULL)
		return;

	memcpy(&handle,thandle,sizeof(handle));

	CEE_timer_expiration_ptr ptimer_expiration = ptimer->m_expiration_routine;
	CEE_tag_def tag = ptimer->m_user_tag;
	(ptimer_expiration)(tag);

	ptimer = GTransport.m_Timer_list->find_timerByhandle(&handle);
	if (ptimer != NULL)
		ptimer->timer_restart();
}

void
BUILD_SYSTEM_MSG_CALL(
	   const CEE_handle_def *call_id_ 
	,  void *request_ 
	,  short countRead
	,  short* receiveInfo
)
{
	CEE_status sts = CEE_SUCCESS;
	CEE_tag_def objtag_ = 0;
	CEERCV_SystemMessage_exc_ exception_;
	IDL_short error = 0;
	CEERCV_SystemMessage_reply_seq_ reply;
	CEERCV_SystemMessage_request_seq_ request;
	request._buffer = (IDL_short *)request_;
	request._length = countRead;

	CEERCV_SystemMessage_sme_(
		  NULL
		, call_id_
		, &exception_
		, (const IDL_short *)receiveInfo
		, (const CEERCV_SystemMessage_request_seq_ *)&request
		, &error
		, (CEERCV_SystemMessage_reply_seq_ *)&reply);

	sts = REPLYX ( (const char*)reply._buffer, reply._length,,*(receiveInfo+2),error);
	DEALLOCATE_TEMP_MEMORY((CEE_handle_def *)call_id_);
}

void
BUILD_USER_MSG_CALL(
	   const CEE_handle_def *call_id_ 
	,  void *request_ 
	,  short countRead
	,  short* receive_info
)
{
	IDL_short dialogInfo = 0;
	CEERCV_IOMessage_request_seq_ request;
	request._buffer = (IDL_octet*)request_;
	request._length = countRead;

	CEERCV_IOMessage_ame_(
	    NULL
	  , call_id_
	  , (const IDL_short *)receive_info
	  , dialogInfo
	  , (const CEERCV_IOMessage_request_seq_ *)&request
	  );
}

/*
 * Asynchronous method function for
 * operation 'CEERCV_IOMessage'
 */
extern "C" void
CEERCV_IOMessage_ame_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const IDL_short *receiveInfo
  , /* In    */ IDL_short dialogInfo
  , /* In    */ const CEERCV_IOMessage_request_seq_ *request
  )
{
	CEERCV_IOMessage_exc_ exception_;
	IDL_short error;
	CEERCV_IOMessage_reply_seq_ reply;

	exception_.exception_nr = 0;
	error = 0;
	if (request->_length >= sizeof(HEADER) && ((HEADER*)(request->_buffer))->signature == SIGNATURE)
		IOMessage(objtag_, call_id_, &exception_, receiveInfo, dialogInfo, request, &error, &reply);
	else
	{
		error = SRVR_ERR_WRONG_MESSAGE_FORMAT;
		reply._length = 0;
		reply._buffer = NULL;
		IOMessage_short_res_( receiveInfo[2], call_id_, &exception_, error, &reply);
	}
}

/*
 * Asynchronous response for
 * operation 'IOMessage_res'
 */
CEE_status
IOMessage_short_res_(
    /* In    */ short message_tag
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const CEERCV_IOMessage_exc_ *exception_
  , /* In    */ IDL_short error
  , /* In    */ const CEERCV_IOMessage_reply_seq_ *reply
  )
{
	CEE_status sts = CEE_SUCCESS;
	IDL_short m_error = error;

	if (exception_->exception_nr == CEERCV_IOMessage_decline_exn_)
		m_error = 2;

	sts = REPLYX ( reply->_buffer, reply->_length,,message_tag,m_error);
	DEALLOCATE_TEMP_MEMORY((CEE_handle_def *)call_id_);
	return sts;
}

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
  )
{
	CEE_status sts = CEE_SUCCESS;

	CEE_handle_def call_id;
	memcpy(&call_id, call_id_, sizeof(CEE_handle_def));

	char* buffer = request->_buffer + sizeof(HEADER);
	long length = request->_length - sizeof(HEADER);

	HEADER* hdr = (HEADER*)request->_buffer;
	CFSystemSrvr* pnode;

	short max_reply_count = receiveInfo[1];
	short message_tag = receiveInfo[2];
	*error = 0;
	exception_->exception_nr = 0;
	reply->_length = 0;
	reply->_buffer = NULL;

	switch(hdr->hdr_type)
	{
	case WRITE_REQUEST_FIRST:
		pnode = GTransport.m_FSystemSrvr_list->find_node(receiveInfo);
		if (pnode == NULL)
		{
			*error = SRVR_ERR_NODE_WRITE_REQUEST_FIRST;
			IOMessage_short_res_( message_tag, &call_id, exception_, *error, reply);
			return;
		}
		RESET_ERRORS((long)pnode);
		memset(&pnode->m_whdr,0,sizeof(HEADER));
		memcpy(&pnode->m_rhdr,hdr,sizeof(HEADER));
		if(pnode->r_allocate(hdr->total_length) == NULL)
		{
			*error = SRVR_ERR_MEMORY_ALLOCATE;
			IOMessage_short_res_( message_tag, &call_id, exception_, *error, reply);
			return;
		}
		memcpy(pnode->m_rbuffer, buffer, length);
		pnode->m_curptr = pnode->m_rbuffer + length;
		IOMessage_short_res_(message_tag, &call_id, exception_, *error, reply);
		break;
	case WRITE_REQUEST_NEXT:
		pnode = GTransport.m_FSystemSrvr_list->find_node(receiveInfo);
		if (pnode == NULL)
		{
			*error = SRVR_ERR_NODE_WRITE_REQUEST_NEXT;
			IOMessage_short_res_( message_tag, &call_id, exception_, *error, reply);
			return;
		}
		RESET_ERRORS((long)pnode);
		memset(&pnode->m_whdr,0,sizeof(HEADER));
		memcpy(pnode->m_curptr, buffer, length);
		pnode->m_curptr += length;
		sts = IOMessage_short_res_( message_tag, &call_id, exception_, *error, reply);
		break;
	case READ_RESPONSE_FIRST:
	case READ_RESPONSE_NEXT:
		pnode = GTransport.m_FSystemSrvr_list->find_node(receiveInfo);
		if (pnode == NULL)
		{ 
			if (hdr->hdr_type == READ_RESPONSE_FIRST)
				*error = SRVR_ERR_NODE_READ_RESPONSE_FIRST;
			else
				*error = SRVR_ERR_NODE_READ_RESPONSE_NEXT;
			IOMessage_short_res_( message_tag, &call_id, exception_, *error, reply);
			return;
		}
		RESET_ERRORS((long)pnode);
		memset(&pnode->m_whdr,0,sizeof(HEADER));
		if (hdr->hdr_type == READ_RESPONSE_FIRST)
		{
			memcpy(&call_id, &pnode->m_call_id, sizeof(CEE_handle_def));
			pnode->m_max_reply_count = max_reply_count;
			if(pnode->m_trans_begin_tag != 0)
				RESUMETRANSACTION(pnode->m_trans_begin_tag);
			DISPATCH_IOMessage( (CEE_tag_def)pnode, &call_id, exception_, receiveInfo, dialogInfo, request, error, reply, hdr->operation_id );
			length = pnode->m_reply_count;
		}
		else
		{
			length = pnode->m_curlength;
			buffer = pnode->m_curptr;
			if (length > max_reply_count)
				length = max_reply_count;
			reply->_length = length;
			reply->_buffer = buffer;
			IOMessage_short_res_( message_tag, &call_id, exception_, *error, reply);
		}
		pnode->m_curlength -= length;
		pnode->m_curptr += length;

		if (pnode->m_curlength == 0)
		{
			if (pnode->m_rbuffer != NULL)
			{
				delete pnode->m_rbuffer;
				pnode->m_rbuffer = NULL;
			}
			if (pnode->m_wbuffer != NULL)
			{
				delete pnode->m_wbuffer;
				pnode->m_wbuffer = NULL;
			}
			DEALLOCATE_TEMP_MEMORY(&call_id);
		}
		break;
	case CLEANUP:
		GTransport.m_FSystemSrvr_list->cleanup_node(receiveInfo);
		IOMessage_short_res_( message_tag, &call_id, exception_, *error, reply);
		break;
	default:
		break;
	}
}

CEE_status 
RCV_IOMessage_res_(
	  CFSystemSrvr* pnode
	, char* buffer 
	, unsigned long message_length
	, /* In    */ const CEE_handle_def *call_id_
  )
{
	CEE_status sts = CEE_SUCCESS;
	HEADER* hdr;
	short reply_count;

	short retcode = 0;
	short tx_handle[10];
	long trans_begin_tag = 0;

	memset(&tx_handle[0],0,20);
	retcode = TMF_GETTXHANDLE_(&tx_handle[0]);
	if (retcode == 0)
		TMF_BEGINTAG_FROM_TXHANDLE_(&tx_handle[0], &pnode->m_trans_begin_tag);
	else
		pnode->m_trans_begin_tag = 0;		
//
// save buffer in the node
//
	pnode->m_wbuffer = buffer;
//
// send back message to the caller
//
	hdr = (HEADER*)buffer;
	memcpy(hdr, &pnode->m_rhdr, sizeof(HEADER));
	hdr->hdr_type = READ_RESPONSE_FIRST;
	hdr->total_length = message_length - sizeof(HEADER);

	reply_count = message_length > pnode->m_max_reply_count ? pnode->m_max_reply_count: message_length;
	
	sts = REPLYX ( buffer, reply_count,,pnode->m_message_tag);

	memcpy(&pnode->m_whdr, hdr, sizeof(HEADER));
	pnode->m_reply_count = reply_count;
	pnode->m_curlength = message_length;
	pnode->m_curptr = buffer;

	return sts;
}
//==============================================================================

/*
 * Synchronous method function for
 * operation 'CEERCV_SystemMessage'
 */
extern "C" void
CEERCV_SystemMessage_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ CEERCV_SystemMessage_exc_ *exception_
  , /* In    */ const IDL_short *receiveInfo
  , /* In    */ const CEERCV_SystemMessage_request_seq_ *request
  , /* Out   */ IDL_short *error
  , /* Out   */ CEERCV_SystemMessage_reply_seq_ *reply
  )
{
	exception_->exception_nr = CEERCV_SystemMessage_decline_exn_;
	*error = 0;
	reply->_length = 0;
	reply->_buffer = NULL;

	if( receiveInfo[0] == 0 ) // System messages
	{
		if( request->_buffer[0] == ZSYS_VAL_SMSG_PROCDEATH )
		{
			DISPATCH_PROCDEATH_SMessage( objtag_, call_id_, exception_, receiveInfo, request, error, reply );
		}
		else if( request->_buffer[0] == ZSYS_VAL_SMSG_CPUDOWN )
		{
			DISPATCH_CPUDOWN_SMessage( objtag_, call_id_, exception_, receiveInfo, request, error, reply );
		}
		else if( request->_buffer[0] == ZSYS_VAL_SMSG_OPEN ) 
		{
			DISPATCH_OPEN_SMessage( objtag_, call_id_, exception_, receiveInfo, request, error, reply );
		}
		else if( request->_buffer[0] == ZSYS_VAL_SMSG_CLOSE ) 
		{
			DISPATCH_CLOSE_SMessage( objtag_, call_id_, exception_, receiveInfo, request, error, reply );
		}
		else if( request->_buffer[0] == ZSYS_VAL_SMSG_DEVINFO )
		{
			DISPATCH_DEVINFO_SMessage( objtag_, call_id_, exception_, receiveInfo, request, error, reply );
		}
		else if( request->_buffer[0] == ZSYS_VAL_SMSG_CONFIGINFO )
		{
			DISPATCH_CONFIGINFO_SMessage( objtag_, call_id_, exception_, receiveInfo, request, error, reply );
		}
	}

}
