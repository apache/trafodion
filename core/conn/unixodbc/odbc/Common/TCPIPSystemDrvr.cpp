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

#define WIN32_LEAN_AND_MEAN

#include "windows.h"
#include <winsock2.h>
#include <Ws2tcpip.h>
#include "Transport.h"
#include "TCPIPSystemDrvr.h"
#include "swapdrvr_drvr.h"
#include "../drvrglobal.h"

CTCPIPSystemDrvr::CTCPIPSystemDrvr()
{
	odbcAPI = 0;
	dialogueId = 0;

	RESET_ERRORS((long)this);

	m_wbuffer = NULL;
	m_rbuffer = NULL;
	m_rbuffer_length = 0;
	m_wbuffer_length = 0;
	m_object_ref[0]=0;
	m_IObuffer = NULL;
	m_IObuffer = new char[MAX_BUFFER_LENGTH];
	if (m_IObuffer == NULL)
		exit(0);
	m_hSocket = NULL;
	m_DoCompression = true;
	m_hEvents[0] = NULL;
	m_hEvents[1] = NULL;
}

CTCPIPSystemDrvr::~CTCPIPSystemDrvr()
{
	RESET_ERRORS((long)this);
	cleanup();

	if (m_IObuffer != NULL)
		delete m_IObuffer;
	if (m_hSocket != NULL)
	{
		closesocket(m_hSocket);
	}
	if (m_hEvents[0] != NULL)
		CloseHandle(m_hEvents[0]);
	if (m_hEvents[1] != NULL)
		CloseHandle(m_hEvents[1]);
}

void CTCPIPSystemDrvr::cleanup(void)
{
	w_release();
	r_release();
}

void CTCPIPSystemDrvr::process_swap( char* buffer )
{
	PROCESS_swap(buffer, odbcAPI);
}

char* CTCPIPSystemDrvr::w_allocate(int size)
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

char* CTCPIPSystemDrvr::r_allocate( int size)
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

void CTCPIPSystemDrvr::r_assign(char* buffer, long length)
{
	if (m_rbuffer != NULL)
		delete m_rbuffer;
	m_rbuffer = buffer;
	m_rbuffer_length = length;
}

void CTCPIPSystemDrvr::w_assign(char* buffer, long length)
{
	if (m_wbuffer != NULL)
		delete m_wbuffer;
	m_wbuffer = buffer;
	m_wbuffer_length = length;
}

void CTCPIPSystemDrvr::w_release()
{
	if (m_wbuffer != NULL)
		delete m_wbuffer;
	m_wbuffer = NULL;
	m_wbuffer_length = 0;
}

void CTCPIPSystemDrvr::r_release()
{
	if (m_rbuffer != NULL)
		delete m_rbuffer;
	m_rbuffer = NULL;
	m_rbuffer_length = 0;
}

char* CTCPIPSystemDrvr::w_buffer()
{
	return m_wbuffer;
}

char* CTCPIPSystemDrvr::r_buffer()
{
	return m_rbuffer;
}

long CTCPIPSystemDrvr::w_buffer_length()
{
	return m_wbuffer_length;
}

long CTCPIPSystemDrvr::r_buffer_length()
{
	return m_rbuffer_length;
}

void CTCPIPSystemDrvr::send_error(short error, short error_detail, const CEE_handle_def *call_id_)
{
}

CEE_status CTCPIPSystemDrvr::send_response(char* buffer, unsigned long message_length, const CEE_handle_def *call_id_)
{
	return CEE_SUCCESS;
}

void CTCPIPSystemDrvr::CloseSessionWithCleanup()
{
	(gDrvrGlobal.fpTCPIPCloseSession)(this);
	cleanup();
}

char CTCPIPSystemDrvr::swap()
{
	return SWAP_YES;
}

char CTCPIPSystemDrvr::transport()
{
	return TCPIP;
}

CTCPIPSystemDrvr_list::CTCPIPSystemDrvr_list()
{
	list=NULL;
	InitializeCriticalSection(&CSTCPIP);
}
CTCPIPSystemDrvr_list::~CTCPIPSystemDrvr_list()
{
	DeleteCriticalSection(&CSTCPIP);
}
void CTCPIPSystemDrvr_list::cleanup()
{
	EnterCriticalSection (&GTransport.m_TransportCSObject);


	CTCPIPSystemDrvr* cnode = list;
	CTCPIPSystemDrvr* nnode;
	while( cnode != NULL ){
		nnode = cnode->next;
		delete cnode;
		cnode = nnode;
	}
	list=NULL;
	LeaveCriticalSection (&GTransport.m_TransportCSObject);
}
CTCPIPSystemDrvr* CTCPIPSystemDrvr_list::ins_node()
{
	EnterCriticalSection (&GTransport.m_TransportCSObject);

	CTCPIPSystemDrvr* cnode = list;
	CTCPIPSystemDrvr* pnode = list;
	CTCPIPSystemDrvr* nnode;
	while(cnode!=NULL ){
		pnode=cnode;
		cnode=cnode->next;
	}
	if((nnode = new CTCPIPSystemDrvr())!=NULL){
		nnode->next = cnode;
		if(pnode!=NULL) 
			pnode->next = nnode;
		else
			list = nnode;
	}
	LeaveCriticalSection (&GTransport.m_TransportCSObject);
	return nnode ;
}
bool CTCPIPSystemDrvr_list::del_node(CTCPIPSystemDrvr* p )
{
	EnterCriticalSection (&GTransport.m_TransportCSObject);
	bool bret = true;

	CTCPIPSystemDrvr* cnode = list;
	CTCPIPSystemDrvr* pnode = list;
	while( cnode!= NULL )
	{
		if( cnode == p )
			break;
		pnode = cnode;
		cnode = cnode->next;
	}
	if( cnode==NULL )
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
	LeaveCriticalSection (&GTransport.m_TransportCSObject);
	return bret;
}
bool CTCPIPSystemDrvr_list::find_node(CTCPIPSystemDrvr* p )
{
	EnterCriticalSection (&GTransport.m_TransportCSObject);
	CTCPIPSystemDrvr* cnode = list;
	bool bfound = false;
	while( cnode != NULL )
	{
		if (cnode == p ){
			bfound = true;
			break;
		}
		cnode = cnode->next;
	}
	LeaveCriticalSection (&GTransport.m_TransportCSObject);
	return bfound;
}

CTCPIPSystemDrvr* CTCPIPSystemDrvr_list::find_node(char* object_ref )
{
	EnterCriticalSection (&GTransport.m_TransportCSObject);
	CTCPIPSystemDrvr* cnode = list;
	while( cnode != NULL )
	{
		if (strcmp(cnode->m_object_ref, object_ref) == 0 ){
			break;
		}
		cnode = cnode->next;
	}
	LeaveCriticalSection (&GTransport.m_TransportCSObject);
	return cnode;
}

//=====================================================================

bool OpenIO (CTCPIPSystemDrvr* pTCPIPSystem, char* object_ref)
{

	bool sts = false;
	printf("%s %d\n", __FILE__, __LINE__);
	RESET_ERRORS((long)pTCPIPSystem);
	if (gDrvrGlobal.gTCPIPLoadError != 0)
	{
		SET_ERROR((long)pTCPIPSystem, PC, TCPIP, UNKNOWN_API, E_DRIVER, gDrvrGlobal.gTCPIPLibrary, O_INIT_PROCESS, F_LOAD_DLL, gDrvrGlobal.gTCPIPLoadError, (int)0);
		sts = false;
	}
	else
	{
		RESET_ERRORS((long)pTCPIPSystem);
		sts = (gDrvrGlobal.fpTCPIPOpenSession)((void*)pTCPIPSystem, object_ref);
	}

	return sts;
}

void CloseIO (CTCPIPSystemDrvr* pTCPIPSystem)
{
	pTCPIPSystem->w_release(); //release write buffer
	(gDrvrGlobal.fpTCPIPCloseSession)((void*)pTCPIPSystem);
}

bool DoIO (CTCPIPSystemDrvr* pTCPIPSystem, char* wbuffer, long write_count, char*& rbuffer, long& read_count)
{
	bool bok = true;

// write count, read count and temporary count
	long wcount;
	long rcount = 0;
	short tcount = 0;

	char* buffer;
	rbuffer = NULL;
//
	HEADER wheader = {0,0,0,0,'N',COMP_12,WRITE_REQUEST_FIRST,SIGNATURE,VERSION,PC,TCPIP,SWAP_YES,0,0};
	HEADER rheader = {0,0,0,0,'N',COMP_12,READ_RESPONSE_FIRST,SIGNATURE,VERSION,PC,TCPIP,SWAP_YES,0,0};
	HEADER* prheader,*pwheader;

	wheader.operation_id	= pTCPIPSystem->odbcAPI;
	rheader.operation_id	= pTCPIPSystem->odbcAPI;
	wheader.dialogueId		= pTCPIPSystem->dialogueId;
	rheader.dialogueId		= pTCPIPSystem->dialogueId;
	long timeout			= pTCPIPSystem->dwTimeout;
	memset(&pTCPIPSystem->m_rheader, 0, sizeof(HEADER));

	RESET_ERRORS((long)pTCPIPSystem);

	if (pTCPIPSystem->m_DoCompression)
	{
		wheader.compress_ind = COMP_YES;
		wheader.compress_type = COMP_12;
		rheader.compress_ind = COMP_YES;
		rheader.compress_type = COMP_12;
	}
	else
	{
		wheader.compress_ind = COMP_NO;
		wheader.compress_type = 0;
		rheader.compress_ind = COMP_NO;
		rheader.compress_type = 0;
	}

// send to the server

	wheader.total_length = write_count;
	wheader.hdr_type = WRITE_REQUEST_FIRST;
	if (wheader.compress_ind == COMP_YES  && write_count > MIN_LENGTH_FOR_COMPRESSION)
		DoCompression(pTCPIPSystem, wheader, (unsigned char*)wbuffer, (unsigned long&)write_count);
	else
		wheader.compress_type = 0;

	wcount = write_count;

	while (wcount > 0)
	{
		if (wheader.hdr_type == WRITE_REQUEST_FIRST)
		{
			if (wcount > TCPI_DRVR_SEND_BUFFER - sizeof(HEADER))
				tcount = TCPI_DRVR_SEND_BUFFER - sizeof(HEADER);
			else
				tcount = (short)wcount;
		}
		else
		{
			if (wcount > TCPI_DRVR_SEND_BUFFER)
				tcount = TCPI_DRVR_SEND_BUFFER;
			else
				tcount = (short)wcount;
		}

		pwheader = &wheader;
		bok = (gDrvrGlobal.fpTCPIPDoWriteRead)((void*)pTCPIPSystem, pwheader, wbuffer, tcount, timeout);
		if (bok == false)
			goto out;
		wheader.hdr_type = WRITE_REQUEST_NEXT;
		wcount  -= tcount;
		wbuffer += tcount;
	}
// receive from the server

	pTCPIPSystem->cleanup(); //release all temp memory

// read for READ_RESPONSE_FIRST

	tcount = 0;
	rbuffer = NULL;
	rheader.hdr_type = READ_RESPONSE_FIRST;

	prheader = &rheader;
	bok = (gDrvrGlobal.fpTCPIPDoWriteRead)((void*)pTCPIPSystem, prheader, rbuffer, tcount, timeout);
	if (bok == false)
		goto out;

// the server returns total length in the header

	memcpy(&pTCPIPSystem->m_rheader, prheader, sizeof(HEADER));
	if (prheader->compress_ind == COMP_YES && prheader->compress_type != 0)
		rcount = prheader->cmp_length;
	else
		rcount = prheader->total_length;
	read_count = prheader->total_length;

	buffer = (char*)pTCPIPSystem->r_allocate(rcount);
	if (buffer == NULL)
	{
		SET_ERROR((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_OPERATOR_NEW, F_DO_IO, DRVR_ERR_MEMORY_ALLOCATE, rcount);
		bok = false;
		goto out;
	}

// if there is something beside the header in the IO buffer

	if (tcount > 0)
	{
		memcpy(buffer, rbuffer, tcount);
		rcount -= tcount;
	}

	rbuffer = buffer+ tcount;

// read for READ_RESPONSE_NEXT

	while (rcount > 0){
// we send only a header
		tcount = 0;
		rheader.hdr_type = READ_RESPONSE_NEXT;
		prheader = &rheader;
		bok = (gDrvrGlobal.fpTCPIPDoWriteRead)((void*)pTCPIPSystem, prheader, rbuffer, tcount, timeout);
		if (bok == false)
			goto out;
		rcount  -= tcount;
		rbuffer += tcount;
	}

	rbuffer = buffer;

	if (pTCPIPSystem->m_rheader.compress_ind == COMP_YES && pTCPIPSystem->m_rheader.compress_type != 0)
	{
		bok = DoExpand(pTCPIPSystem, pTCPIPSystem->m_rheader, (unsigned char*)rbuffer, (unsigned long&)read_count);
		if (bok)
			rbuffer = pTCPIPSystem->r_buffer();
	}
out:
	return bok;
}

void DoCompression(CTCPIPSystemDrvr* pTCPIPSystem, HEADER& wheader, unsigned char* wbuffer, unsigned long& write_count)
{
	bool retcode = true;
	unsigned long uncmp_count = write_count;

	unsigned char* cmp_buf = (unsigned char*)new char[write_count];
	if (cmp_buf == NULL)
	{
		wheader.compress_type = 0;
		wheader.cmp_length = 0;
		return;
	}

	retcode = pTCPIPSystem->m_compression.compress((unsigned char*)wbuffer, uncmp_count, cmp_buf, write_count);
	if (retcode == false || uncmp_count <= write_count)
	{
		delete cmp_buf;
		wheader.compress_type = 0;
		wheader.cmp_length = 0;
		write_count = uncmp_count;
		return;
	}
	memcpy(wbuffer, cmp_buf, write_count);
	delete cmp_buf;
	wheader.compress_type = COMP_12;
	wheader.cmp_length = write_count;
}

bool DoExpand(CTCPIPSystemDrvr* pTCPIPSystem, HEADER& rheader, unsigned char* ibuffer, unsigned long& output_size)
{
	bool retcode;
	unsigned char* obuffer = (unsigned char*)new char[rheader.total_length];
	if (obuffer == NULL)
	{
		SET_ERROR((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_OPERATOR_NEW, F_DO_IO, DRVR_ERR_MEMORY_ALLOCATE, rheader.total_length);
		return false;
	}
	retcode = pTCPIPSystem->m_compression.expand(ibuffer, obuffer, output_size);
	if (retcode == false || output_size != rheader.total_length)
	{
		delete obuffer;
		SET_ERROR((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_EXPAND, F_DO_IO, DRVR_ERR_MEMORY_ALLOCATE, rheader.total_length);
		return false;
	}
	pTCPIPSystem->r_assign((char*)obuffer, rheader.total_length);
	rheader.compress_ind = COMP_NO;
	rheader.cmp_length = 0;
	return true;
}

void WINAPI 
TCPIP_SET_ERROR(long signature, char platform, char transport, int api, ERROR_TYPE error_type, char* process, OPERATION operation, FUNCTION function, int error, int errordetail)
{
	SET_ERROR(signature, platform, transport, api, error_type, process, operation, function, error, errordetail);
}
