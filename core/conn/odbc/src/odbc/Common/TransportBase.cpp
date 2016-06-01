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
#include <platform_ndcs.h>

#include <idltype.h>
#include <stdio.h>
#include <errno.h>

#include "Transport.h"

#ifndef NSK_PLATFORM
#define SRVRTRACE_ENTER(name)
#define SRVRTRACE_EXIT(name)
#endif

void
ADD_ONE_TO_HANDLE(CEE_handle_def *handle)
{
	SRVRTRACE_ENTER(FILE_TNSPTB+1);
	handle->contents[0] = handle->contents[0] + 1;
	if (handle->contents[0] == 0)
	{
		handle->contents[1] = handle->contents[1] + 1;
		if (handle->contents[1] == 0)
		{
			handle->contents[2] = handle->contents[2] + 1;
			if (handle->contents[2] == 0)
				handle->contents[3] = handle->contents[3] + 1;
		}
	}
	SRVRTRACE_EXIT(FILE_TNSPTB+1);
}

#ifndef NSK_CFGSRVR

extern IDL_boolean
CEE_HANDLE_IS_NIL( const CEE_handle_def *handle)
{
	SRVRTRACE_ENTER(FILE_TNSPTB+2);
	if (handle->contents[0] == 0 &&
			handle->contents[1] == 0 &&
				handle->contents[2] == 0 &&
					handle->contents[3] == 0)
	{
		SRVRTRACE_EXIT(FILE_TNSPT+1);
		return IDL_TRUE;
	}
	SRVRTRACE_EXIT(FILE_TNSPTB+2);
	return IDL_FALSE;
}

extern void
CEE_HANDLE_SET_NIL( CEE_handle_def *handle)
{
	SRVRTRACE_ENTER(FILE_TNSPTB+3);
	memset(handle, 0, sizeof(CEE_handle_def));
	SRVRTRACE_EXIT(FILE_TNSPTB+3);
}

#endif

//===================== CTempMemory =======================================
CTempMemory::CTempMemory( const CEE_handle_def* call_id, void* p_mem, CTempMemory* cnext)
{
	SRVRTRACE_ENTER(FILE_TNSPTB+4);
	memcpy(&m_call_id, call_id,sizeof(CEE_handle_def));
	p = p_mem;
	next = cnext;
	GTransport.m_TempMemory_list.m_list_length++;
	SRVRTRACE_EXIT(FILE_TNSPTB+4);
}
CTempMemory::~CTempMemory()
{
	SRVRTRACE_ENTER(FILE_TNSPTB+5);
	if(p != NULL) free(p);
	GTransport.m_TempMemory_list.m_list_length--;
	SRVRTRACE_EXIT(FILE_TNSPTB+5);
}
//===================== CTempMemory_list ==================================
CTempMemory_list::CTempMemory_list()
{
	SRVRTRACE_ENTER(FILE_TNSPTB+6);
	list=NULL;
	m_list_length = 0;
	SRVRTRACE_EXIT(FILE_TNSPTB+6);
}
CTempMemory_list::~CTempMemory_list()
{
	SRVRTRACE_ENTER(FILE_TNSPTB+7);
	CTempMemory* cnode = list;
	CTempMemory* nnode;
	while( cnode != NULL )
	{
		nnode = cnode->next;
		delete cnode;
		cnode = nnode;
	}
	list=NULL;
	SRVRTRACE_EXIT(FILE_TNSPTB+7);
}
bool CTempMemory_list::add_mem( const CEE_handle_def* call_id, void* p )
{
	SRVRTRACE_ENTER(FILE_TNSPTB+8);
	CTempMemory* cnode = list;
	CTempMemory* pnode = list;
	CTempMemory* nnode;

	while(cnode!=NULL )
	{
		pnode=cnode;
		cnode=cnode->next;
	}
	if((nnode = new CTempMemory(call_id,p,cnode))!=NULL)
	{
		if(pnode!=NULL)
			pnode->next = nnode;
		else
			list = nnode;
	}
	SRVRTRACE_EXIT(FILE_TNSPTB+8);
	return (nnode == NULL)?false:true;
}
void CTempMemory_list::del_mem()
{
	SRVRTRACE_ENTER(FILE_TNSPTB+9);
	CTempMemory* cnode = list;
	CTempMemory* nnode;
	while( cnode != NULL )
	{
		nnode = cnode->next;
		delete cnode;
		cnode = nnode;
	}
	list=NULL;
	SRVRTRACE_EXIT(FILE_TNSPTB+9);
}

bool CTempMemory_list::del_mem( CTempMemory* node)
{
	SRVRTRACE_ENTER(FILE_TNSPTB+10);
	CTempMemory* cnode = list;
	CTempMemory* pnode = list;
	while( cnode!= NULL && cnode != node )
	{
		pnode = cnode;
		cnode = cnode->next;
	}
	if( cnode==NULL)
	{
		SRVRTRACE_EXIT(FILE_TNSPTB+10);
		return false;
	}
	if (pnode == list && cnode == list)
		list = cnode->next;
	else
		pnode->next = cnode->next;
	delete cnode;
	SRVRTRACE_EXIT(FILE_TNSPTB+10);
	return true;
}

void CTempMemory_list::del_mem( const CEE_handle_def* call_id)
{
	SRVRTRACE_ENTER(FILE_TNSPTB+11);
	CTempMemory* cnode = list;
	CTempMemory* nnode;
	while( cnode != NULL )
	{
		nnode = cnode->next;
		if (memcmp(&cnode->m_call_id, call_id,sizeof(CEE_handle_def)) == 0)
		{
			del_mem(cnode);
			del_mem(call_id);
			SRVRTRACE_EXIT(FILE_TNSPTB+11);
			return;
		}
		cnode = nnode;
	}
	SRVRTRACE_EXIT(FILE_TNSPTB+11);
}

bool CTempMemory_list::add_tmp_allocate(const CEE_handle_def* call_id,void*ptr)
{
	return add_mem( call_id, ptr );
}

void CTempMemory_list::del_tmp_allocate(const CEE_handle_def* call_id)
{
	del_mem(call_id);
}

bool CTempMemory_list::del_tmp_allocate(void* ptr)
{
	SRVRTRACE_ENTER(FILE_TNSPTB+12);
	CTempMemory* tcnode = list;
	while( tcnode != NULL )
	{
		if( tcnode->p == ptr)
		{
			SRVRTRACE_EXIT(FILE_TNSPTB+12);
			return del_mem(tcnode);
		}
		else
			tcnode = tcnode->next;
	}
	SRVRTRACE_EXIT(FILE_TNSPTB+12);
	return false;
}

void CTempMemory_list::del_tmp_allocate()
{
	del_mem();
}

//==========================================================================

CError::CError()
{
	SRVRTRACE_ENTER(FILE_TNSPTB+13);
	platform = ' ';
	transport = ' ';
	error_type = E_UNKNOWN;
	api = 0;
	process_name[0] = 0;
	operation = O_UNDEFINED;
	function = F_UNDEFINED;
	error = 0;
	errordetail = 0;
	signature = 0;
	next = NULL;
	GTransport.m_error_list.m_list_length++;
	SRVRTRACE_EXIT(FILE_TNSPTB+13);

};
CError::~CError()
{
	SRVRTRACE_ENTER(FILE_TNSPTB+14);
	GTransport.m_error_list.m_list_length--;
	SRVRTRACE_EXIT(FILE_TNSPTB+14);
};

CError_list::CError_list()
{
	list = NULL;
	m_list_length = 0;
	InitializeCriticalSection(&m_ErrorCSObject);
}
CError_list::~CError_list()
{
	cleanup();
	DeleteCriticalSection(&m_ErrorCSObject);
}
void CError_list::cleanup()
{
	EnterCriticalSection (&m_ErrorCSObject);
	CError* cnode = list;
	CError* nnode;
	while( cnode != NULL )
	{
		nnode = cnode->next;
		delete cnode;
		cnode = nnode;
	}
	list=NULL;
	LeaveCriticalSection (&m_ErrorCSObject);
}
CError* CError_list::ins_error(long signature)
{
	EnterCriticalSection (&m_ErrorCSObject);
	CError* cnode = list;
	CError* pnode = list;
	CError* nnode;

	while(cnode!=NULL )
	{
		pnode=cnode;
		cnode=cnode->next;
	}

	if((nnode = new CError())!=NULL)
	{
		nnode->signature = signature;
		if(pnode!=NULL)
			pnode->next = nnode;
		else
			list = nnode;
	}
	LeaveCriticalSection (&m_ErrorCSObject);
	return nnode;
}
bool CError_list::del_error(long signature)
{
	EnterCriticalSection (&m_ErrorCSObject);
	CError* cnode = list;
	CError* pnode = list;
	bool bret = true;
	while( cnode!= NULL  && cnode->signature != signature )
	{
		pnode = cnode;
		cnode = cnode->next;
	}
	if( cnode==NULL)
	{
		bret = false;
		goto out;
	}
	if (pnode == list && cnode == list)
		list = cnode->next;
	else
		pnode->next = cnode->next;
	delete cnode;
out:
	LeaveCriticalSection (&m_ErrorCSObject);
	return bret;
}

CError* CError_list::find_error(long signature)
{
	EnterCriticalSection (&m_ErrorCSObject);
	CError* cnode = list;
	CError* pnode = list;
	while( cnode!= NULL && cnode->signature != signature )
	{
		pnode = cnode;
		cnode = cnode->next;
	}
	LeaveCriticalSection (&m_ErrorCSObject);
	return cnode;
}

CError* CError_list::find_last_error()
{
	EnterCriticalSection (&m_ErrorCSObject);
	CError* cnode = list;
	CError* pnode = list;
	while( cnode!= NULL )
	{
		pnode = cnode;
		cnode = cnode->next;
	}
	LeaveCriticalSection (&m_ErrorCSObject);
	return pnode;
}

CTransportBase::CTransportBase()
{
	bMapErrors = true;
}
CTransportBase::~CTransportBase()
{
}

void CTransportBase::log_error(CError* ierror)
{
}

void CTransportBase::log_info(CError* ierror)
{
}

void CTransportBase::log_warning(CError* ierror)
{
}

short CTransportBase::AWAITIOX(short* filenum,short* wcount, long* tag, long wtimeout)
{
#ifndef NSK_PLATFORM
	return 0;
#else
	return ::AWAITIOX(filenum,,wcount,tag,wtimeout);
#endif
}

void WINAPI
SET_ERROR(long signature, char platform, char transport, int api, ERROR_TYPE error_type, char* process, OPERATION operation, FUNCTION function, int error, int errordetail)
{
	CError* ierror;
#ifdef NSK_ODBC_SRVR
	if(signature != 0)
	EnterCriticalSection2(&GTransport.m_TransportCSObject);
#else
	EnterCriticalSection (&GTransport.m_TransportCSObject);
#endif

	if(signature == 0)
		// when signatue is 0, the intent is to log an error - no need to insert into a list
       ierror = new CError;
	else
	   ierror = GTransport.m_error_list.ins_error(signature);

	if (ierror != NULL)
	{
		ierror->platform = platform;
		ierror->transport = transport;
		ierror->api = api;
		ierror->error_type = error_type;
		strncpy(ierror->process_name,process,MAX_PROCESS_NAME);
		ierror->process_name[MAX_PROCESS_NAME-1] = 0;
		ierror->operation = operation;
		ierror->function = function;
		ierror->error = error;
		ierror->errordetail = errordetail;
		GTransport.log_error(ierror);
	}
#ifdef NSK_ODBC_SRVR
	if(signature != 0)
	LeaveCriticalSection2(&GTransport.m_TransportCSObject);
#else
	LeaveCriticalSection (&GTransport.m_TransportCSObject);
#endif
    if(signature == 0)
	   delete ierror;
}

void WINAPI
SET_INFO(long signature, char platform, char transport, int api, ERROR_TYPE error_type, char* process, OPERATION operation, FUNCTION function, int error, int errordetail)
{
#ifdef NSK_ODBC_SRVR
	EnterCriticalSection2(&GTransport.m_TransportCSObject);
#else
	EnterCriticalSection (&GTransport.m_TransportCSObject);
#endif

	CError* ierror = GTransport.m_error_list.ins_error(signature);
	;
	if (ierror != NULL)
	{
		ierror->platform = platform;
		ierror->transport = transport;
		ierror->api = api;
		ierror->error_type = error_type;
		strncpy(ierror->process_name,process,MAX_PROCESS_NAME);
		ierror->process_name[MAX_PROCESS_NAME-1] = 0;
		ierror->operation = operation;
		ierror->function = function;
		ierror->error = error;
		ierror->errordetail = errordetail;
		GTransport.log_info(ierror);
	}

#ifdef NSK_ODBC_SRVR
	LeaveCriticalSection2(&GTransport.m_TransportCSObject);
#else
	LeaveCriticalSection (&GTransport.m_TransportCSObject);
#endif
}

void WINAPI
SET_WARNING(long signature, char platform, char transport, int api, ERROR_TYPE error_type, char* process, OPERATION operation, FUNCTION function, int error, int errordetail)
{
#ifdef NSK_ODBC_SRVR
	EnterCriticalSection2(&GTransport.m_TransportCSObject);
#else
	EnterCriticalSection (&GTransport.m_TransportCSObject);
#endif

	CError* ierror = GTransport.m_error_list.ins_error(signature);
	;
	if (ierror != NULL)
	{
		ierror->platform = platform;
		ierror->transport = transport;
		ierror->api = api;
		ierror->error_type = error_type;
		strncpy(ierror->process_name,process,MAX_PROCESS_NAME);
		ierror->process_name[MAX_PROCESS_NAME-1] = 0;
		ierror->operation = operation;
		ierror->function = function;
		ierror->error = error;
		ierror->errordetail = errordetail;
		GTransport.log_warning(ierror);
	}

#ifdef NSK_ODBC_SRVR
	LeaveCriticalSection2(&GTransport.m_TransportCSObject);
#else
	LeaveCriticalSection (&GTransport.m_TransportCSObject);
#endif
}

void WINAPI
RESET_ERRORS(long signature)
{
    if(signature == 0) return;
#ifdef NSK_ODBC_SRVR
	EnterCriticalSection2(&GTransport.m_TransportCSObject);
#else
	EnterCriticalSection (&GTransport.m_TransportCSObject);
#endif
	GTransport.m_error_list.del_error(signature);
#ifdef NSK_ODBC_SRVR
	LeaveCriticalSection2(&GTransport.m_TransportCSObject);
#else
	LeaveCriticalSection (&GTransport.m_TransportCSObject);
#endif
}

int WINAPI
GET_ERROR(long signature)
{
#ifdef NSK_ODBC_SRVR
	EnterCriticalSection2(&GTransport.m_TransportCSObject);
#else
	EnterCriticalSection (&GTransport.m_TransportCSObject);
#endif
	CError* ierror = GTransport.m_error_list.find_error(signature);
#ifdef NSK_ODBC_SRVR
	LeaveCriticalSection2(&GTransport.m_TransportCSObject);
#else
	LeaveCriticalSection (&GTransport.m_TransportCSObject);
#endif
	if (ierror != NULL)
		return ierror->error;
	else
		return 0;
}
//LCOV_EXCL_START
int WINAPI
GET_ERROR_DETAIL(long signature)
{
#ifdef NSK_ODBC_SRVR
	EnterCriticalSection2(&GTransport.m_TransportCSObject);
#else
	EnterCriticalSection (&GTransport.m_TransportCSObject);
#endif
	CError* ierror = GTransport.m_error_list.find_error(signature);
#ifdef NSK_ODBC_SRVR
	LeaveCriticalSection2(&GTransport.m_TransportCSObject);
#else
	LeaveCriticalSection (&GTransport.m_TransportCSObject);
#endif
	if (ierror != NULL)
		return ierror->errordetail;
	else
		return 0;
}

void
FORMAT_AS_APIS(int api, char* buffer)
{
	switch(api)
	{
	case AS_API_INIT:
		strcat(buffer,"INIT");
		break;
	case AS_API_GETOBJREF:
		strcat(buffer,"GETOBJREF");
		break;
	case AS_API_GETOBJREF_PRE_R22:
		strcat(buffer,"PRE R2.2 GETOBJREF");
		break;
	case AS_API_REGPROCESS:
		strcat(buffer,"REGPROCESS");
		break;
	case AS_API_UPDATESRVRSTATE:
		strcat(buffer,"UPDATESRVRSTATE");
		break;
	case AS_API_WOULDLIKETOLIVE:
		strcat(buffer,"WOULDLIKETOLIVE");
		break;
	case AS_API_STARTAS:
		strcat(buffer,"STARTAS");
		break;
	case AS_API_STOPAS:
		strcat(buffer,"STOPAS");
		break;
	case AS_API_STARTDS:
		strcat(buffer,"STARTDS");
		break;
	case AS_API_STOPDS:
		strcat(buffer,"STOPDS");
		break;
	case AS_API_STATUSAS:
		strcat(buffer,"STATUSAS");
		break;
	case AS_API_STATUSDS:
		strcat(buffer,"STATUSDS");
		break;
	case AS_API_STATUSDSDETAIL:
		strcat(buffer,"STATUSDSDETAIL");
		break;
	case AS_API_STATUSSRVRALL:
		strcat(buffer,"STATUSSRVRALL");
		break;
	case AS_API_STOPSRVR:
		strcat(buffer,"STOPSRVR");
		break;
	case AS_API_STATUSDSALL:
		strcat(buffer,"STATUSDSALL");
		break;
	case AS_API_DATASOURCECONFIGCHANGED:
		strcat(buffer,"DATASOURCECONFIGCHANGED");
		break;
	case AS_API_ENABLETRACE:
		strcat(buffer,"ENABLETRACE");
		break;
	case AS_API_DISABLETRACE:
		strcat(buffer,"DISABLETRACE");
		break;
	case AS_API_GETVERSIONAS:
		strcat(buffer,"GETVERSIONAS");
		break;
	default:
		sprintf(buffer,"UNKNOWN_API %d",api);
	}
}

void
FORMAT_CFG_APIS(int api, char* buffer)
{
	switch(api)
	{
	case CFG_API_INIT:
		strcat(buffer,"INIT");
		break;
	case CFG_API_GETOBJECTNAMELIST:
		strcat(buffer,"GETOBJECTNAMELIST");
		break;
	case CFG_API_GETDATASOURCE:
		strcat(buffer,"GETDATASOURCE");
		break;
	case CFG_API_DROPDATASOURCE:
		strcat(buffer,"DROPDATASOURCE");
		break;
	case CFG_API_SETDATASOURCE:
		strcat(buffer,"SETDATASOURCE");
		break;
	case CFG_API_ADDNEWDATASOURCE:
		strcat(buffer,"ADDNEWDATASOURCE");
		break;
	case CFG_API_CHECKDATASOURCENAME:
		strcat(buffer,"CHECKDATASOURCENAME");
		break;
	case CFG_API_GETDSNCONTROL:
		strcat(buffer,"GETDSNCONTROL");
		break;
	case CFG_API_SETDSNCONTROL:
		strcat(buffer,"SETDSNCONTROL");
		break;
	case CFG_API_GETRESOURCEVALUES:
		strcat(buffer,"GETRESOURCEVALUES");
		break;
	case CFG_API_SETRESOURCEVALUES:
		strcat(buffer,"SETRESOURCEVALUES");
		break;
	case CFG_API_GETENVIRONMENTVALUES:
		strcat(buffer,"GETENVIRONMENTVALUES");
		break;
	case CFG_API_SETENVIRONMENTVALUES:
		strcat(buffer,"SETENVIRONMENTVALUES");
		break;
	case CFG_API_GETSTARTUPCONFIGVALUES:
		strcat(buffer,"GETSTARTUPCONFIGVALUES");
		break;
	case CFG_API_GETDATASOURCEVALUES:
		strcat(buffer,"GETDATASOURCEVALUES");
		break;
	case CFG_API_SETDSSTATUS:
		strcat(buffer,"SETDSSTATUS");
		break;
	case CFG_API_SETASSTATUS:
		strcat(buffer,"SETASSTATUS");
		break;
	default:
		sprintf(buffer,"UNKNOWN_API %d",api);
	}
}

void
FORMAT_SRVR_APIS(int api, char* buffer)
{
	switch(api)
	{
	case SRVR_API_INIT:
		strcat(buffer,"INIT");
		break;
	case SRVR_API_SQLCONNECT:
		strcat(buffer,"SQLCONNECT");
		break;
	case SRVR_API_SQLDISCONNECT:
		strcat(buffer,"SQLDISCONNECT");
		break;
	case SRVR_API_SQLSETCONNECTATTR:
		strcat(buffer,"SQLSETCONNECTATTR");
		break;
	case SRVR_API_SQLENDTRAN:
		strcat(buffer,"SQLENDTRAN");
		break;
	case SRVR_API_SQLPREPARE:
		strcat(buffer,"SQLPREPARE");
		break;
	case SRVR_API_SQLEXECUTE_ROWSET:
		strcat(buffer,"SQLEXECUTE_ROWSET");
		break;
	case SRVR_API_SQLEXECDIRECT_ROWSET:
		strcat(buffer,"SQLEXECDIRECT_ROWSET");
		break;
	case SRVR_API_SQLEXECDIRECT:
		strcat(buffer,"SQLEXECDIRECT");
		break;
	case SRVR_API_SQLEXECUTE:
		strcat(buffer,"SQLEXECUTE");
		break;
	case SRVR_API_SQLEXECUTECALL:
		strcat(buffer,"SQLEXECUTECALL");
		break;
	case SRVR_API_SQLEXECUTE2:
		strcat(buffer,"SQLEXECUTE2");
		break;
	case SRVR_API_SQLFETCH:
		strcat(buffer,"SQLFETCH");
		break;
	case SRVR_API_SQLFREESTMT:
		strcat(buffer,"SQLFREESTMT");
		break;
	case SRVR_API_GETCATALOGS:
		strcat(buffer,"GETCATALOGS");
		break;
	case SRVR_API_STOPSRVR:
		strcat(buffer,"STOPSRVR");
		break;
	case SRVR_API_ENABLETRACE:
		strcat(buffer,"ENABLETRACE");
		break;
	case SRVR_API_DISABLETRACE:
		strcat(buffer,"DISABLETRACE");
		break;
	case SRVR_API_ENABLE_SERVER_STATISTICS:
		strcat(buffer,"ENABLE_SERVER_STATISTICS");
		break;
	case SRVR_API_DISABLE_SERVER_STATISTICS:
		strcat(buffer,"DISABLE_SERVER_STATISTICS");
		break;
	case SRVR_API_UPDATE_SERVER_CONTEXT:
		strcat(buffer,"UPDATE_SERVER_CONTEXT");
		break;
	default:
		sprintf(buffer,"UNKNOWN_API %d",api);
	}
}

char*
FORMAT_ERROR(CError* ierror)
{
	char static s_buffer[500];
	char buffer[10];

	strcpy(s_buffer," Platform: ");
	switch(ierror->platform)
	{
	case NSK:
		strcat(s_buffer,"NSK");
		break;
	case PC:
		strcat(s_buffer,"PC");
		break;
	default:
		strcpy(s_buffer,"UNKNOWN_PLATFORM");
	}
	strcat(s_buffer,", Transport: ");
	switch(ierror->transport)
	{
	case FILE_SYSTEM:
		strcat(s_buffer,"FILE_SYSTEM");
		break;
	case TCPIP:
		strcat(s_buffer,"TCPIP");
		break;
	default:
		sprintf(s_buffer,"UNKNOWN_TRANSPORT %c",ierror->transport);
	}
	strcat(s_buffer,", Api: ");
	if (ierror->api < AS_API_START )
		strcat(s_buffer,"UNKNOWN_API");
	else if (ierror->api < CFG_API_START )
		FORMAT_AS_APIS(ierror->api, s_buffer);
	else if (ierror->api < SRVR_API_START )
		FORMAT_CFG_APIS(ierror->api, s_buffer);
	else
		FORMAT_SRVR_APIS(ierror->api, s_buffer);

	strcat(s_buffer,", Error type: ");
	switch(ierror->error_type)
	{
	case E_DRIVER:
		strcat(s_buffer,"DRIVER");
		break;
	case E_SERVER:
		strcat(s_buffer,"SERVER");
		break;
	case E_ASSERVER:
		strcat(s_buffer,"ASSERVER");
		break;
	case E_CFGSERVER:
		strcat(s_buffer,"CFGSERVER");
		break;
	case E_TEMP_MEMORY:
		strcat(s_buffer,"TEMP_MEMORY");
		break;
	case E_TIMER:
		strcat(s_buffer,"TIMER");
		break;
	case E_LISTENER:
		strcat(s_buffer,"LISTENER");
		break;
	case E_TCPIPROCESS:
		strcat(s_buffer,"TCPIPROCESS");
		break;
	default:
		sprintf(s_buffer,"UNKNOWN_ERROR_TYPE %d",ierror->error_type);
	}
	strcat(s_buffer,", Process: ");
	strcat(s_buffer,ierror->process_name);
	strcat(s_buffer,", Operation: ");
	switch(ierror->operation)
	{
	case O_INIT_PROCESS:
		strcat(s_buffer,"INIT_PROCESS");
		break;
	case O_OPEN_SESSION:
		strcat(s_buffer,"OPEN_SESSION");
		break;
	case O_DO_WRITE_READ:
		strcat(s_buffer,"DO_WRITE_READ");
		break;
	case O_DO_OPERATOR_NEW:
		strcat(s_buffer,"DO_OPERATOR_NEW");
		break;
	case O_DO_EXPAND:
		strcat(s_buffer,"DO_EXPAND");
		break;
	case O_NEW_CONNECTION:
		strcat(s_buffer,"NEW_CONNECTION");
		break;
	case O_SELECT:
		strcat(s_buffer,"SOCKET_SELECT");
		break;
	case O_PIPE:
		strcat(s_buffer,"PIPE");
		break;
	default:
		sprintf(s_buffer,"UNKNOWN_OPERATION %d",ierror->operation);
	}

	strcat(s_buffer,", function: ");
	switch(ierror->function)
	{
	case F_AWAITIOX:
		strcat(s_buffer,"AWAITIOX");
		break;
	case F_CHECK_IF_ASSVC_LIVES:
		strcat(s_buffer,"CHECK_IF_ASSVC_LIVES");
		break;
	case F_ENV_GET_SYSTEM_CATALOG_NAME:
		strcat(s_buffer,"ENV_GET_SYSTEM_CATALOG_NAME");
		break;
	case F_ENV_GET_MX_SYSTEM_CATALOG_NAME:
		strcat(s_buffer,"ENV_GET_MX_SYSTEM_CATALOG_NAME");
		break;
	case F_FILE_GETINFO_:
		strcat(s_buffer,"FILE_GETINFO_");
		break;
	case F_FILE_OPEN_:
		strcat(s_buffer,"FILE_OPEN_");
		break;
	case F_FILENAME_TO_PROCESSHANDLE_:
		strcat(s_buffer,"FILENAME_TO_PROCESSHANDLE_");
		break;
	case F_HDR_TYPE:
		strcat(s_buffer,"HDR_TYPE");
		break;
	case F_INS_NODE:
		strcpy(s_buffer,"INS_NODE");
		break;
	case F_INSTANTIATE_RG_OBJECT:
		strcpy(s_buffer,"INSTANTIATE_RG_OBJECT");
		break;
	case F_NEW:
		strcat(s_buffer,"NEW");
		break;
	case F_PROCESS_GETINFO_:
		strcat(s_buffer,"PROCESS_GETINFO_");
		break;
	case F_PROCESSHANDLE_GETMINE_:
		strcat(s_buffer,"PROCESSHANDLE_GETMINE_");
		break;
	case F_LOAD_DLL:
		strcat(s_buffer,"LOAD_DLL");
		break;
	case F_SETMODE:
		strcat(s_buffer,"SETMODE");
		break;
	case F_SOCKET:
		strcat(s_buffer,"SOCKET");
		break;
	case F_SOCKET_GET_LEN:
		strcat(s_buffer,"SOCKET_GET_LEN");
		break;
	case F_CONNECT:
		strcat(s_buffer,"CONNECT");
		break;
	case F_RESOLVE_IP_ADDRESS:
		strcat(s_buffer,"RESOLVE_IP_ADDRESS");
		break;
	case F_SETSOCOPT:
		strcat(s_buffer,"SETSOCOPT");
		break;
	case F_WSACREATEEVENT:
		strcat(s_buffer,"WSACREATE_EVENT");
		break;
	case F_WSAEVENTSELECT:
		strcat(s_buffer,"WSAEVENT_SELECT");
		break;
	case F_WSAWAITFORMULTIPLEEVENTS:
		strcat(s_buffer,"WSAWAIT_FOR_MULTIPLE_EVENTS");
		break;
	case F_CHECKCONNECTION:
		strcat(s_buffer,"CHECKCONNECTION");
		break;
	case F_CHECKSOCKET:
		strcat(s_buffer,"CHECKSOCKET");
		break;
	case F_SELECT:
		strcat(s_buffer,"SELECT");
		break;
	case F_SEND:
		strcat(s_buffer,"SEND");
		break;
	case F_SEND_GETOVERLAPPEDRESULTS:
		strcat(s_buffer,"SEND_GETOVERLAPPEDRESULTS");
		break;
	case F_RECV:
		strcat(s_buffer,"RECV");
		break;
	case F_RECV_GETOVERLAPPEDRESULTS:
		strcat(s_buffer,"RECV_GETOVERLAPPEDRESULTS");
		break;
	case F_LISTEN:
		strcat(s_buffer,"LISTEN");
		break;
	case F_ACCEPT:
		strcat(s_buffer,"ACCEPT");
		break;
	case F_BIND:
		strcat(s_buffer,"BIND");
		break;
	case F_SRVR_TRANSPORT_ERROR:
		strcat(s_buffer,"SRVR_TRANSPORT_ERROR");
		break;
	case F_FILE_COMPLETE_SET:
		strcat(s_buffer,"FILE_COMPLETE_SET");
		break;
	case F_FILE_COMPLETE:
		strcat(s_buffer,"FILE_COMPLETE");
		break;
	case F_FILE_COMPLETE_GETINFO:
		strcat(s_buffer,"FILE_COMPLETE_GETINFO");
		break;
	case F_FD_ISSET:
		strcat(s_buffer,"FD_ISSET");
		break;
	case F_INIT_PIPE:
		strcat(s_buffer,"PIPE");
		break;
	default:
		sprintf(s_buffer,"UNKNOWN_FUNCTION %d",ierror->function);
	}
	strcat(s_buffer,", error: ");
	_itoa( ierror->error, buffer, 10 );
	strcat(s_buffer, buffer);
	strcat(s_buffer,", error_detail: ");
	_itoa( ierror->errordetail, buffer, 10 );
	strcat(s_buffer, buffer);
	strcat(s_buffer, ". ");

//	GTransport.m_error_list.del_error(ierror->signature);

	return s_buffer;
}

char*
FORMAT_ERROR(long signature)
{
	CError* ierror = GTransport.m_error_list.find_error(signature);
	if (ierror == NULL)
		return "";
	return FORMAT_ERROR(ierror);
}

char* FORMAT_ERROR(char* text, long signature)
{
	char static s_buffer[600];
	CError* ierror = GTransport.m_error_list.find_error(signature);
	if (ierror == NULL)
		return "";
	strcpy(s_buffer, text);
	strcat(s_buffer, " ");
	return strcat(s_buffer, FORMAT_ERROR(ierror));
}


char*
FORMAT_LAST_ERROR()
{
	CError* ierror = GTransport.m_error_list.find_last_error();
	if (ierror == NULL)
		return "";
	return FORMAT_ERROR(ierror);
}

char*
ERROR_TO_TEXT(CError* ierror)
{
	char* s_buffer = NULL;
	if (ierror->error == 0) return "";
	s_buffer = ALLOC_ERROR_BUFFER();
	if (s_buffer == NULL) return "";
	s_buffer[0]=0;
	if (ierror->error >= NT_SOCKET_ERR)							// 10000
		strcpy(s_buffer,DecodeNTSocketErrors(ierror->error));
	else if (ierror->error >= DRVR_ERR_)						// 6000
		strcpy(s_buffer,DecodeDRVRErrors(ierror->error));
	else if (ierror->error >= SRVR_ERR_)						// 5000
		strcpy(s_buffer,DecodeSRVRErrors(ierror->error));
	else if (ierror->error >= NSK_SOCKET_ERR)					// 4000
		strcpy(s_buffer,DecodeNSKSocketErrors(ierror->error));
	else
		strcpy(s_buffer,DecodeSocketErrors(ierror->error));		// Probably standard POSIX socket errors
	return s_buffer;
}

char*
LAST_ERROR_TO_TEXT()
{
	CError* ierror = GTransport.m_error_list.find_last_error();
	if (ierror == NULL)
		return "";
	return ERROR_TO_TEXT(ierror);
}

char*
ERROR_TO_TEXT(long signature)
{
	CError* ierror = GTransport.m_error_list.find_error(signature);
	if (ierror == NULL)
		return "";
	return ERROR_TO_TEXT(ierror);
}

char*
DecodeNSKSocketErrors(int error)
{
	char* msg = "";
	switch(error)
	{
	case 4001:
		msg = "The specified I/O control operation cannot be performed by a nonprivileged user";break;
	case 4003:
		msg = "An accept_nw2 call was issued on a socket that had been shut down or closed";break;
	case 4004:
		msg = "Process received an unexpected signal";break;
	case 4005:
		msg = "I/O error";break;
	case 4006:
		msg = "The call specified an unknown device or the request was outside of the device capabilities";break;
	case 4009:
		msg = "Invalid file descriptor";break;
	case 4012:
		msg = "Insufficient memory";break;
	case 4013:
		msg = "Permission denied";break;
	case 4014:
		msg = "Memory access fault";break;
	case 4017:
		msg = "Object exists";break;
	case 4022:
		msg = "Invalid argument";break;
	case 4024:
		msg = "The network manager attempted to add too many routes";break;
	case 4028:
		msg = "Adapter does not have sufficient memory to complete the request";break;
	case 4032:
		msg = "Write/Send call attempted on a closed (shutdown) socket";break;
	case 4034:
		msg = "A numeric specification in the call is not within the allowable range";break;
	case 4101:
		msg = "no out-of-band data to read";break;
	case 4102:
		msg = "Operation now in progress";break;
	case 4103:
		msg = "Operation already in progress";break;
	case 4104:
		msg = "A socket operation was attempted on an object that is not a socket";break;
	case 4105:
		msg = "Destination address required";break;
	case 4106:
		msg = "The message is too large";break;
	case 4107:
		msg = "Incorrect protocol";break;
	case 4108:
		msg = "Incorrect option";break;
	case 4109:
		msg = "The protocol is not supported by NonStop TCP/IP software";break;
	case 4110:
		msg = "The socket type not supported by the NonStop TCP/IP";break;
	case 4111:
		msg = "The operation is not supported on a transport end point";break;
	case 4112:
		msg = "The specified protocol family is not supported";break;
	case 4113:
		msg = "Cannot assign requested address";break;
	case 4114:
		msg = "Address already in use";break;
	case 4115:
		msg = "Cannot assign requested address";break;
	case 4116:
		msg = "The network is down";break;
	case 4117:
		msg = "Remote network is unreachable";break;
	case 4118:
		msg = "Host crashed and rebooted";break;
	case 4119:
		msg = "Connection aborted";break;
	case 4120:
		msg = "Peer process reset connection before operation completed";break;
	case 4121:
		msg = "Insufficient buffer space";break;
	case 4122:
		msg = "Incorrect call on connected socket";break;
	case 4123:
		msg = "Socket not connected";break;
	case 4124:
		msg = "The operation could not be performed because the specified socket was already shut down";break;
	case 4126:
		msg = "The connection timed out before the operation completed";break;
	case 4127:
		msg = "Remote host rejected connection request";break;
	case 4128:
		msg = "The destination host is present, but it is not responding";break;
	case 4129:
		msg = "No route to host";break;
	case 4131:
		msg = "Process or file name exceeds maximum allowable name length";break;
	case 4195:
		msg = "Out-of-band data is pending";break;
	case 4196:
		msg = "Internal error occurred";break;
	default:
		msg = DecodeERRNOErrors(error);
	}
	return msg;
}

char*
DecodeSocketErrors(int error)
{
	char* msg = "";
	switch(error)
	{
		case ENFILE:
			msg = "No more file descriptors are available for the system";break;
		case EMFILE:
			msg = "No more file descriptors are available for this process";break;
		case EBADF:
			msg = "The argument is not a valid socket file descriptor";break;
		case EADDRINUSE:
			msg = "The specified address is already in use";break;
		case EAFNOSUPPORT:
			msg = "Implementation or the address does not support the address family";break;
		case EPROTONOSUPPORT:
			msg = "The protocol is not supported by the address family or the implementation";break;
		case EPROTOTYPE:
			msg = "The socket type is not supported by the protocol";break;
		case EADDRNOTAVAIL:
			msg = "The specified address is not available";break;
		case EINVAL:
			msg = "The socket has been shut down, or is already bound or connected. Or no data is available";break;
		case EACCES:
			msg = "Permission denied";break;
		case ENOTSOCK:
			msg = "The socket argument does not refer to a socket";break;
		case EOPNOTSUPP:
			msg = "The type of the socket or the socket protocol does not support this operation";break;
		case EDESTADDRREQ:
			msg = "The socket is not bound to a local address";break;
		case ECONNRESET:
			msg = "A connection was forcibly closed or reset by peer";break;
		case EINTR:
			msg = "The function was interrupted by a signal before any data is available";break;
		case ENOTCONN:
			msg = "The socket is not connected";break;
		case ETIMEDOUT:
			msg = "The connection timed out during establishment, or a transmission timeout on active connection";break;
		case EIO:
			msg = "An I/O error occurred while reading or writing data";break;
		case EFAULT:
			msg = "Invalid user space or process address";break;
		case EISCONN:
			msg = "The connection-mode socket was connected already but a recipient was specified";break;
		case EAGAIN:
			msg = "The socket is marked O_NONBLOCK and no conn or data is waiting to be accepted or received";break;
		case ENOMEM:
			msg = "Insufficient memory was available to fulfill the request";break;
		case ENOBUFS:
			msg = "Insufficient resources are available in the system to complete the call";break;
		default:
			msg = DecodeERRNOErrors(error);
	}
	return msg;
}

char*
DecodeNTSocketErrors(int error)
{
	char* msg = "";
	switch(error)
	{
	case 10013:											//WSAEACCES
		msg = "Permission denied";break;
	case 10014:											//WSAEFAULT
		msg = "Bad address";break;
	case 10024:											//WSAEMFILE
		msg = "Too many open sockets";break;
	case 10039:											//WSAEDESTADDRREQ
		msg = "Destination address required";break;
	case 10048:											//WSAEADDRINUSE
		msg = "Address already in use";break;
	case 10049:											//WSAEADDRNOTAVAIL
		msg = "Cannot assign requested address";break;
	case 10050:											//WSAENETDOWN
		msg = "Network is down";break;
	case 10051:											//WSAENETUNREACH
		msg = "Network is unreachable";break;
	case 10052:											//WSAENETRESET
		msg = "Network dropped connection on reset";break;
	case 10053:											//WSAECONNABORTED
		msg = "Software caused connection abort";break;
	case 10054:											//WSAECONNRESET
		msg = "Connection reset by peer";break;
	case 10055:											//WSAENOBUFS
		msg = "No buffer space available";break;
	case 10060:											//WSAETIMEDOUT
		msg = "Connection timed out";break;
	case 10061:											//WSAECONNREFUSED
		msg = "Connection refused";break;
	case 10064:											//WSAEHOSTDOWN
		msg = "Host is down";break;
	case 10065:											//WSAEHOSTUNREACH
		msg = "No route to host";break;
	case 10067:											//WSAEPROCLIM
		msg = "Too many processes using sockets";break;
	case 10091:											//WSASYSNOTREADY
		msg = "Winsock not available";break;
	case 10093:											//WSANOTINITIALISED
		msg = "Successful WSAStartup not yet performed";break;
	case 10094:											//WSAEDISCON
		msg = "Graceful shutdown in progress";break;
	case 11001:											//WSAHOST_NOT_FOUND
		msg = "Host not found";break;
	}
	return msg;
}
char*
DecodeSRVRErrors(int error)
{
	char* msg = "";
	switch(error)
	{
	case SRVR_ERR_WRONG_MESSAGE_FORMAT:
		msg = "WRONG MESSAGE FORMAT";break;
	case SRVR_ERR_NODE_WRITE_REQUEST_FIRST:
		msg = "NODE WRITE REQUEST FIRST FAILED";break;
	case SRVR_ERR_MEMORY_ALLOCATE:
		msg = "MEMORY ALLOCATE";break;
	case SRVR_ERR_NODE_WRITE_REQUEST_NEXT:
		msg = "NODE WRITE REQUEST NEXT FAILED";break;
	case SRVR_ERR_NODE_READ_RESPONSE_FIRST:
		msg = "NODE READ RESPONSE FIRST FAILED";break;
	case SRVR_ERR_NODE_READ_RESPONSE_NEXT:
		msg = "NODE READ RESPONSE NEXT FAILED";break;
	case SRVR_ERR_READ_OPERATION:
		msg = "READ OPERATION FAILED";break;
	case SRVR_ERR_COMPRESS_OPERATION:
		msg = "COMPRESS OPERATION FAILED";break;
	case SRVR_ERR_EXPAND_OPERATION:
		msg = "EXPAND OPERATION FAILED";break;
	case SRVR_ERR_WRITE_OPERATION:
		msg = "WRITE OPERATION FAILED";break;
	case SRVR_ERR_UNKNOWN_REQUEST:
		msg = "UNKNOWN REQUEST";break;
	case SRVR_ERR_LISTENER_ERROR1:
		msg = "LISTENER ERROR1";break;
	case SRVR_ERR_LISTENER_ERROR2:
		msg = "LISTENER ERROR2";break;
	case SRVR_ERR_ZERO_MESSAGE_LENGTH:
		msg = "ZERO MESSAGE LENGTH";break;
	case SRVR_ERR_DECODE_PARAMETERS:
		msg = "DECODE PARAMETERS";break;
	}
	return msg;
}

char*
DecodeDRVRErrors(int error)
{
	char* msg = "";
	switch(error)
	{
	case DRVR_ERR_INVALID_DLLHANDLE:
		msg = "INVALID DLL HANDLE";break;
	case DRVR_ERR_CANNOTLOAD_PROCADDRESS:
		msg = "CANNOT LOAD PROCADDRESS";break;
	case DRVR_ERR_WRONGWINSOCKVERSION:
		msg = "WRONG WINSOCK VERSION";break;
	case DRVR_ERR_WRONGSIGNATURE:
		msg = "WRONG SIGNATURE";break;
	case DRVR_ERR_WRONGVERSION:
		msg = "WRONG VERSION";break;
	case DRVR_ERR_ERROR_FROM_SERVER:
		msg = "ERROR FROM SERVER";break;
	case DRVR_ERR_INCORRECT_LENGTH:
		msg = "INCORRECT LENGTH";break;
	case DRVR_ERR_MEMORY_ALLOCATE:
		msg = "MEMORY ALLOCATE";break;
	case DRVR_ERR_WRONG_IP_ADDRESS:
		msg = "WRONG IP ADDRESS";break;
	}
	return msg;
}



char*
DecodeERRNOErrors(int error)
{
	char* msg = "";
	switch(error)
	{
	case 12:
		msg = "The file is in use";break;
	case 53:
		msg = "File system internal error has occurred";break;
	case 59:
		msg = "File structure is inconsistent or disk file is bad";break;
	case 60:
		msg = "Volume mounted is not correct or device downed and then upped";break;
	case 73:
		msg = "The disk file or record is locked";break;
	case 4001:
		msg = "Not owner, permission denied";break;
	case 4002:
		msg = "No such file or directory";break;
	case 4003:
		msg = "No such process or table entry";break;
	case 4004:
		msg = "Interrupted system call";break;
	case 4005:
		msg = "I/O error";break;
	case 4006:
		msg = "No such device or address";break;
	case 4007:
		msg = "Argument list too long";break;
	case 4008:
		msg = "Exec format error";break;
	case 4009:
		msg = "Bad file descriptor";break;
	case 4010:
		msg = "No children";break;
	case 4011:
		msg = "Resource temporarily unavailable";break;
	case 4012:
		msg = "Insufficient user memory";break;
	case 4013:
		msg = "Permission denied";break;
	case 4014:
		msg = "Bad address";break;
	case 4016:
		msg = "Device or resource busy";break;
	case 4017:
		msg = "File already exists";break;
	case 4018:
		msg = "Cross-device link";break;
	case 4019:
		msg = "No such device";break;
	case 4020:
		msg = "Not a directory";break;
	case 4021:
		msg = "Is a directory";break;
	case 4022:
		msg = "Invalid function argument";break;
	case 4023:
		msg = "File table overflow";break;
	case 4024:
		msg = "Maximum number of files already open";break;
	case 4025:
		msg = "Inappropriate I/O control operation";break;
	case 4026:
		msg = "Object (text) file busy";break;
	case 4027:
		msg = "File too large";break;
	case 4028:
		msg = "No space left on device";break;
	case 4029:
		msg = "Illegal seek";break;
	case 4030:
		msg = "Read only file system";break;
	case 4031:
		msg = "Too many links";break;
	case 4032:
		msg = "Broken pipe or no reader on socket";break;
	case 4033:
		msg = "Argument out of range";break;
	case 4034:
		msg = "Value out of range";break;
	case 4035:
		msg = "No message of desired type";break;
	case 4036:
		msg = "Identifier removed";break;
	case 4045:
		msg = "Deadlock condition";break;
	case 4046:
		msg = "No record locks available";break;
	case 4061:
		msg = "No data sent or received";break;
	case 4099:
		msg = "Function not implemented";break;
	case 4101:
		msg = "Operation would block";break;
	case 4102:
		msg = "Operation now in progress";break;
	case 4103:
		msg = "Operation already in progress";break;
	case 4104:
		msg = "Socket operation on non-socket";break;
	case 4105:
		msg = "Destination address required";break;
	case 4106:
		msg = "Message too long";break;
	case 4107:
		msg = "Protocol wrong type for socket";break;
	case 4108:
		msg = "Protocol not available";break;
	case 4109:
		msg = "Protocol not supported";break;
	case 4110:
		msg = "Socket type not supported";break;
	case 4111:
		msg = "Operation not supported on socket";break;
	case 4112:
		msg = "Protocol family not supported";break;
	case 4113:
		msg = "Address family not supported";break;
	case 4114:
		msg = "Address already in use";break;
	case 4115:
		msg = "Can't assign requested address";break;
	case 4116:
		msg = "Network is down";break;
	case 4117:
		msg = "Network is unreachable";break;
	case 4118:
		msg = "Network dropped connection on reset";break;
	case 4119:
		msg = "Software caused connection abort";break;
	case 4120:
		msg = "Connection reset by remote host";break;
	case 4121:
		msg = "No buffer space available";break;
	case 4122:
		msg = "Socket is already connected";break;
	case 4123:
		msg = "Socket is not connected";break;
	case 4124:
		msg = "Can't send after socket shutdown";break;
	case 4126:
		msg = "Connection timed out";break;
	case 4127:
		msg = "Connection refused";break;
	case 4128:
		msg = "Host is down";break;
	case 4129:
		msg = "No route to host";break;
	case 4131:
		msg = "File name too long";break;
	case 4132:
		msg = "Directory not empty";break;
	case 4180:
		msg = "Invalid data in buffer";break;
	case 4181:
		msg = "No reply in buffer";break;
	case 4182:
		msg = "Partial buffer received";break;
	case 4183:
		msg = "Interface error from SPI";break;
	case 4184:
		msg = "Version mismatch";break;
	case 4185:
		msg = "XDR encoding error";break;
	case 4186:
		msg = "XDR decoding error";break;
	case 4195:
		msg = "Out-of-band data available";break;
	case 4196:
		msg = "Invalid socket call";break;
	case 4197:
		msg = "File type not supported";break;
	case 4198:
		msg = "C file (code 180) not odd-unstructured";break;
	case 4199:
		msg = "Insufficient internal memory";break;
	case 4200:
		msg = "Too many symbolic links during path name resolution";break;
	case 4201:
		msg = "Fileset catalog internal consistency error";break;
	case 4202:
		msg = "Root fileset is not mounted";break;
	case 4203:
		msg = "OSS not running";break;
	case 4204:
		msg = "Illegal byte sequence (from XPG4)";break;
	case 4205:
		msg = "Process not CRE compliant, but requests a service that depends on CRE";break;
	case 4206:
		msg = "Non-OSS process has requested a service available only to OSS processes";break;
	case 4207:
		msg = "CPU unavailable";break;
	case 4208:
		msg = "Something impossible happened";break;
	case 4209:
		msg = "OSS operation attempted on Guardian file descriptor";break;
	case 4210:
		msg = "NonStop logic error";break;
	case 4211:
		msg = "Either cwd or cwd/file name is longer than PATHMAX";break;
	case 4212:
		msg = "A Guardian define error was encountered";break;
	case 4213:
		msg = "An OSS process is trying to do a cross cpu exec, but has an active semundo";break;
	case 4214:
		msg = "An invalid message tag was encountered";break;
	case 4215:
		msg = "Positioning of an OSS directory failed";break;
	case 4216:
		msg = "Operation not supported";break;
	case 4217:
		msg = "Socket Transport Agent not running";break;
	case 4218:
		msg = "Message Queue Server not running";break;
	}
	return msg;
}
//LCOV_EXCL_STOP
bool BUILD_OBJECTREF(char* ObjRef, char* BuildObjRef, char* ObjectName, int portNumber)
{
	char* tcp;
	char* ip_address;
	IDL_OBJECT_def tmp_objref;

	strncpy(tmp_objref, ObjRef, sizeof(tmp_objref));
	tmp_objref[sizeof(tmp_objref)-1]=0;

	char* saveptr;
	if ((tcp = strtok_r(tmp_objref, ":", &saveptr)) != NULL)
	{
		if ((ip_address = strtok_r( NULL, "/", &saveptr))  != NULL)
		{
			sprintf( BuildObjRef, "%s:%s/%d:%s", tcp, ip_address, portNumber,ObjectName);
		}
		else
			return false;
	}
	else
		return false;

	return true;
}

//=========================================================================

extern CEE_status
CEE_TMP_ALLOCATE(
  /* In  */ const CEE_handle_def *call_id,
  /* In  */ long len,
  /* Out */ void **ptr
  )
{
	if (call_id == NULL || ptr == NULL)
		return CEE_ALLOCFAIL;
	if (len < 8) len = 8;

	*ptr = malloc(len);
	if (*ptr == NULL)
		return CEE_ALLOCFAIL;

	if (GTransport.m_TempMemory_list.add_tmp_allocate(call_id,*ptr) == false)
	{
		free(*ptr);
		return CEE_ALLOCFAIL;
	}

	return CEE_SUCCESS;
}

extern CEE_status
CEE_TMP_DEALLOCATE(
  /* In  */ void    *ptr
  )
{
	if (ptr != 0)
		if(GTransport.m_TempMemory_list.del_tmp_allocate(ptr) == false)
			return CEE_BADMEMORYADDR;

	return CEE_SUCCESS;
}

extern void
DEALLOCATE_TEMP_MEMORY(CEE_handle_def *call_id)
{
	if (call_id != NULL && GTransport.m_TempMemory_list.m_list_length )
		GTransport.m_TempMemory_list.del_tmp_allocate(call_id);
}
//LCOV_EXCL_START
extern void
DEALLOCATE_ALL_TEMP_MEMORY()
{
	if (GTransport.m_TempMemory_list.m_list_length )
		GTransport.m_TempMemory_list.del_tmp_allocate();
}

extern void
DEALLOCATE_ALL_TEMP_MEMORY(void* p)
{
	if (GTransport.m_TempMemory_list.m_list_length )
	{
		CTransportBase* pTransportBase = (CTransportBase*)p;
		pTransportBase->m_TempMemory_list.del_tmp_allocate();
	}
}
//LCOV_EXCL_STOP





