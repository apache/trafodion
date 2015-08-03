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
#include <cextdecs.h(SETMODE)>

#include <windows.h>
#include "Transport.h"
#include "FileSystemDrvr.h"
#include "tal.h"

CFSystemDrvr::CFSystemDrvr()
{
	odbcAPI = 0;
	dialogueId = 0;
	next = NULL;

	RESET_ERRORS((long)this);

	m_wbuffer = NULL;
	m_rbuffer = NULL;
	m_rbuffer_length = 0;
	m_wbuffer_length = 0;
	m_IObuffer = NULL;
	m_process_name[0] = 0;
	PROCESSHANDLE_NULLIT_(m_pHandle);
	m_filenum = -1;
	m_IObuffer = new char[MAX_BUFFER_LENGTH];
	if (m_IObuffer == NULL)
		exit(0);
}

CFSystemDrvr::~CFSystemDrvr()
{
	RESET_ERRORS((long)this);
	cleanup();

	if (m_IObuffer != NULL)
		delete m_IObuffer;
}

void CFSystemDrvr::cleanup(void)
{
	w_release();
	r_release();
}

char CFSystemDrvr::swap()
{
	return SWAP_NO;
}

char CFSystemDrvr::transport()
{
	return FILE_SYSTEM;
}

void CFSystemDrvr::process_swap( char* buffer )
{
}

char* CFSystemDrvr::w_allocate(int size)
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

char* CFSystemDrvr::r_allocate( int size)
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

void CFSystemDrvr::w_release()
{
	if (m_wbuffer != NULL)
		delete m_wbuffer;
	m_wbuffer = NULL;
	m_wbuffer_length = 0;
}

void CFSystemDrvr::r_release()
{
	if (m_rbuffer != NULL)
		delete m_rbuffer;
	m_rbuffer = NULL;
	m_rbuffer_length = 0;
}

void CFSystemDrvr::r_assign(char* buffer,long length)
{
	if (m_rbuffer != NULL)
		delete m_rbuffer;
	m_rbuffer = buffer;
	m_rbuffer_length = length;
}

void CFSystemDrvr::w_assign(char* buffer,long length)
{
	if (m_wbuffer != NULL)
		delete m_wbuffer;
	m_wbuffer = buffer;
	m_wbuffer_length = length;
}

char* CFSystemDrvr::w_buffer()
{
	return m_wbuffer;
}

char* CFSystemDrvr::r_buffer()
{
	return m_rbuffer;
}

long CFSystemDrvr::w_buffer_length()
{
	return m_wbuffer_length;
}

long CFSystemDrvr::r_buffer_length()
{
	return m_rbuffer_length;
}

void CFSystemDrvr::send_error(short error, short error_detail, const CEE_handle_def *call_id_)
{
}

CEE_status CFSystemDrvr::send_response(char* buffer, unsigned long message_length, const CEE_handle_def *call_id_)
{
	return CEE_SUCCESS;
}

bool CFSystemDrvr::OpenSession(char* process)
{
	_cc_status cc;
	short error;
	short errorDetail;
	short proctype;
	long  timeout = OPEN_SESSION_TIMEOUT;

	if ( timeout != -1 ) timeout *= 100;

	if ((error = FILENAME_TO_PROCESSHANDLE_(process
					,strlen(process)
					,&m_pHandle[0])) != 0)
	{
		CloseSessionWithCleanup();
		SET_ERROR((long)this, NSK, FILE_SYSTEM, odbcAPI, E_DRIVER, process, O_OPEN_SESSION, F_FILENAME_TO_PROCESSHANDLE_, error, 0);
		return false;
	}

	error = PROCESS_GETINFO_(m_pHandle,
			OMITREF, OMITSHORT,OMITREF,		// proc string,max buf len,act len
			OMITREF,						// priority
			OMITREF,						// Mom's proc handle 
			OMITREF, OMITSHORT,OMITREF,		// home term,max buf len,act len  
			OMITREF,						// Process execution time 
			OMITREF,						// Creator Access Id 
			OMITREF,						// Process Access Id 
			OMITREF,						// Grand Mom's proc handle 
			OMITREF,						// Job Id 
			OMITREF, OMITSHORT,OMITREF,		// Program file,max buf len,act len  
			OMITREF, OMITSHORT,OMITREF,		// Swap file,max buf len,act len 
			&errorDetail,
			&proctype,						// Process type 
			OMITREF);						// OSS or NT process Id

	if( error != 0 )
	{
		CloseSessionWithCleanup();
		SET_ERROR((long)this, NSK, FILE_SYSTEM, odbcAPI, E_DRIVER, process, O_OPEN_SESSION, F_PROCESS_GETINFO_, error, errorDetail);
		return false;
	}

	if (strcmp(m_process_name,process) == 0 && m_filenum > -1)
		return true;

	if ((error = FILE_OPEN_(process
					, strlen(process)
					, &m_filenum
					, 0
					, 0
					, 1
					, 0
					, 0x4000)) == 0) // nowait open and nowait io
	{
		cc = AWAITIOX(&m_filenum,,,, timeout);
		if (_status_lt(cc))
			FILE_GETINFO_ (m_filenum, &error);
		else
			error = 0;
		if (error != 0)
		{
			// appeares to be hung
			CloseSessionWithCleanup();
			SET_ERROR((long)this, NSK, FILE_SYSTEM, odbcAPI, E_DRIVER, process, O_OPEN_SESSION, F_AWAITIOX, cc, error);
		}
		else // open succeeded
		{
			strncpy(m_process_name,process, sizeof(m_process_name));
			m_process_name[sizeof(m_process_name)-1] = 0;
			
			//I/O operations to finish in the order chosen by the operating system to be most efficient.

			cc = SETMODE ( m_filenum,30,1 );
			if (_status_lt(cc))
				FILE_GETINFO_ (m_filenum, &error);
			else
				error = 0;
			if (error != 0)
			{
				CloseSessionWithCleanup();
				SET_ERROR((long)this, NSK, FILE_SYSTEM, odbcAPI, E_DRIVER, process, O_OPEN_SESSION, F_SETMODE, cc, error);
			}
		}
	}
	else
	{
		CloseSessionWithCleanup();
		SET_ERROR((long)this, NSK, FILE_SYSTEM, odbcAPI, E_DRIVER, process, O_OPEN_SESSION, F_FILE_OPEN_, error, 0);
	}

	return m_filenum != -1;
}

void CFSystemDrvr::CloseSession()
{
	if (m_filenum != -1)
	{
		FILE_CLOSE_(m_filenum);
	}
	m_filenum = -1;
	memset(m_process_name,0,MAX_PROCESS_NAME + 1);
	memset(m_pHandle,0,20);
}

void CFSystemDrvr::CloseSessionWithCleanup()
{
	CloseSession();
	cleanup();
}


bool CFSystemDrvr::DoWriteRead(HEADER*& hdr, char*& buffer, short& bufcount, long timeout)
{
	_cc_status cc;
	short error = 0;
	long tag;
	short wcount;
	long wtimeout = timeout > 0 ? timeout * 100 : -1;

	memcpy(m_IObuffer,hdr,sizeof(HEADER));
	if (buffer != NULL)
		memcpy(m_IObuffer + sizeof(HEADER),buffer,bufcount);

	switch(hdr->hdr_type)
	{
	default:
	case WRITE_REQUEST_FIRST:
	case WRITE_REQUEST_NEXT:
		WRITEX( m_filenum, 
				m_IObuffer,
				bufcount + sizeof(HEADER),
				,
				SIGNATURE);
		break;
	case READ_RESPONSE_FIRST:
	case READ_RESPONSE_NEXT:
		WRITEREADX( m_filenum, 
				m_IObuffer,
				bufcount + sizeof(HEADER), 
				MAX_BUFFER_LENGTH,
				&wcount,
				SIGNATURE);
		break;
	}

	cc = AWAITIOX(&m_filenum,,&wcount,&tag,wtimeout);
	if (!_status_eq(cc))
	{
		FILE_GETINFO_(m_filenum,&error);
		CloseSessionWithCleanup();
		SET_ERROR((long)this, NSK, FILE_SYSTEM, odbcAPI, E_DRIVER, m_process_name, O_DO_WRITE_READ, F_AWAITIOX, cc, error);
	}
	else
	{
		switch(hdr->hdr_type)
		{
		case WRITE_REQUEST_FIRST:
		case WRITE_REQUEST_NEXT:
			bufcount = wcount - sizeof(HEADER);
			break;
		case READ_RESPONSE_FIRST:
			bufcount = wcount - sizeof(HEADER);
			buffer = m_IObuffer + sizeof(HEADER);
			hdr = (HEADER*)m_IObuffer;
			break;
		case READ_RESPONSE_NEXT:
			bufcount = wcount;
			memcpy(buffer, m_IObuffer, bufcount); 
			break;
		default:
			CloseSessionWithCleanup();
			error = 1000;
			SET_ERROR((long)this, NSK, FILE_SYSTEM, odbcAPI, E_DRIVER, m_process_name, O_DO_WRITE_READ, F_HDR_TYPE, error, 0);
		}
	}

	return error == 0;
}

CFSystemDrvr_list::CFSystemDrvr_list()
{
	list=NULL;
}
CFSystemDrvr_list::~CFSystemDrvr_list()
{
}
void CFSystemDrvr_list::cleanup()
{
	CFSystemDrvr* cnode = list;
	CFSystemDrvr* nnode;
	while( cnode != NULL ){
		nnode = cnode->next;
		delete cnode;
		cnode = nnode;
	}
	list=NULL;
}
CFSystemDrvr* CFSystemDrvr_list::ins_node()
{
	CFSystemDrvr* cnode = list;
	CFSystemDrvr* pnode = list;
	CFSystemDrvr* nnode;

	while(cnode!=NULL ){
		pnode=cnode;
		cnode=cnode->next;
	}
	if((nnode = new CFSystemDrvr())!=NULL){
		nnode->next = cnode;
		if(pnode!=NULL) 
			pnode->next = nnode;
		else
			list = nnode;
	}
	return nnode ;
}
bool CFSystemDrvr_list::del_node(CFSystemDrvr* p )
{
	CFSystemDrvr* cnode = list;
	CFSystemDrvr* pnode = list;
	while( cnode!= NULL )
	{
		if( cnode == p )
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
bool CFSystemDrvr_list::find_node(CFSystemDrvr* p )
{
	CFSystemDrvr* cnode = list;
	bool bfound = false;

	while( cnode != NULL )
	{
		if (cnode == p ){
			bfound = true;
			break;
		}
		cnode = cnode->next;
	}
	return bfound;
}

CFSystemDrvr* CFSystemDrvr_list::find_node(char* processname )
{
	CFSystemDrvr* cnode = list;

	while( cnode != NULL )
	{
		if (strcmp(cnode->m_process_name, processname) == 0 ){
			break;
		}
		cnode = cnode->next;
	}
	return cnode;
}

//=====================================================================

bool OpenIO (CFSystemDrvr* pFSystem, char* process)
{
	RESET_ERRORS((long)pFSystem);
	return pFSystem->OpenSession(process);
}

void CloseIO (CFSystemDrvr* pFSystem)
{
	RESET_ERRORS((long)pFSystem);
	pFSystem->CloseSession();
}

bool DoIO (CFSystemDrvr* pFSystem, char* wbuffer, long write_count, char*& rbuffer, long& read_count)
{
	bool bok = true;

// write count, read count and temporary count
	long wcount = write_count;
	long rcount = 0;
	short tcount = 0;

	char* buffer;
	rbuffer = NULL;
//
	HEADER wheader = {0,0,0,0,'N',COMP_12,WRITE_REQUEST_FIRST,SIGNATURE,VERSION,NSK,FILE_SYSTEM,SWAP_NO,0,0};
	HEADER rheader = {0,0,0,0,'N',COMP_12,READ_RESPONSE_FIRST,SIGNATURE,VERSION,NSK,FILE_SYSTEM,SWAP_NO,0,0};
	HEADER* prheader,*pwheader;

	wheader.operation_id	= pFSystem->odbcAPI;
	rheader.operation_id	= pFSystem->odbcAPI;
	wheader.dialogueId		= pFSystem->dialogueId;
	rheader.dialogueId		= pFSystem->dialogueId;
	long timeout			= pFSystem->dwTimeout;

	RESET_ERRORS((long)pFSystem);

// send to the server

	wcount = write_count;
	wheader.total_length = write_count;
	wheader.hdr_type = WRITE_REQUEST_FIRST;

	while (wcount > 0)
	{
		if (wcount > MAX_BUFFER_LENGTH - sizeof(HEADER))
			tcount = MAX_BUFFER_LENGTH - sizeof(HEADER);
		else
			tcount = wcount;

		pwheader = &wheader;
		bok = pFSystem->DoWriteRead(pwheader, wbuffer, tcount, timeout);
		if (bok == false)
			goto out;
		wheader.hdr_type = WRITE_REQUEST_NEXT;
		wcount  -= tcount;
		wbuffer += tcount;
	}
// receive from the server

	pFSystem->cleanup(); //release all temp memory

// read for READ_RESPONSE_FIRST ( we send only a header and we use IO buffer)

	tcount = 0;
	rbuffer = NULL;
	rheader.hdr_type = READ_RESPONSE_FIRST;

	prheader = &rheader;
	bok = pFSystem->DoWriteRead(prheader, rbuffer, tcount, timeout);
	if (bok == false)
		goto out;

// the server returns total length in the header

	rcount = prheader->total_length;
	read_count = prheader->total_length;

	buffer = (char*)pFSystem->r_allocate(rcount);
	if (buffer == NULL)
	{
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

// read for READ_HEADER

	while (rcount > 0){
// we send only a header
		tcount = 0;
		rheader.hdr_type = READ_RESPONSE_NEXT;
		prheader = &rheader;
		bok = pFSystem->DoWriteRead(prheader, rbuffer, tcount, timeout);
		if (bok == false)
			goto out;
		rcount  -= tcount;
		rbuffer += tcount;
	}

	rbuffer = buffer;

out:
	return bok;
}




