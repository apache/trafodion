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
//#include <winsock2.h>
//#include <Ws2tcpip.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "transport.h"
#include "TCPIPUnixDrvr.h"
#include "swapdrvr_drvr.h"
#include "drvrglobal.h"
#include "diagfunctions.h"
#include "TCPIPV4.h"
#include <assert.h>
#include "cconnect.h"
#include "cstmt.h"


extern CEE_status MAP_SRVR_ERRORS(CConnect *pConnection);


CTCPIPUnixDrvr::CTCPIPUnixDrvr()
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
	m_IObuffer = new char[MAX_TCP_BUFFER_LENGTH];
	if (m_IObuffer == NULL)
		exit(0);
	m_hSocket = INVALID_SOCKET;

	m_IOCompression = 0;
	m_IOCompressionlimits = 1000;
	m_hEvents[0] = NULL;
	m_hEvents[1] = NULL;

// we need to initalize the function pointers!
//        FPTCPIPInitIO           fpTCPIPInitIO;
	gDrvrGlobal.fpTCPIPInitIO = &TCPIPInitIO;
//        FPTCPIPExitIO           fpTCPIPExitIO;
	gDrvrGlobal.fpTCPIPExitIO = &TCPIPExitIO; 
//        FPTCPIPOpenSession      fpTCPIPOpenSession;
	gDrvrGlobal.fpTCPIPOpenSession = &TCPIPOpenSession;
//        FPTCPIPCloseSession     fpTCPIPCloseSession;
	gDrvrGlobal.fpTCPIPCloseSession = &TCPIPCloseSession;
//        FPTCPIPDoWriteRead      fpTCPIPDoWriteRead;
	gDrvrGlobal.fpTCPIPDoWriteRead = &TCPIPDoWriteRead;
//
#ifdef BIGE
	m_swap = SWAP_NO;
#else
	m_swap = SWAP_YES;
#endif
}

CTCPIPUnixDrvr::~CTCPIPUnixDrvr()
{
	RESET_ERRORS((long)this);
	cleanup();
	if (m_IObuffer != NULL)
#ifndef unixcli
		delete m_IObuffer;
#else
		delete []m_IObuffer;
#endif
	if (m_hSocket != INVALID_SOCKET)
	{
#ifndef unixcli
		closesocket(m_hSocket);
#else
		close(m_hSocket);
#endif
	}
	if (m_hEvents[0] != NULL)
		CloseHandle(m_hEvents[0]);
	if (m_hEvents[1] != NULL)
		CloseHandle(m_hEvents[1]);
}

void CTCPIPUnixDrvr::cleanup(void)
{
	w_release();
	r_release();
}

void CTCPIPUnixDrvr::process_swap( char* buffer )
{
//#ifndef BIGE
//	PROCESS_swap(buffer, odbcAPI);
//#endif
}

char* CTCPIPUnixDrvr::w_allocate(int size)
{
	if (m_wbuffer != NULL)
#ifndef unixcli
		delete m_wbuffer;
#else
		delete []m_wbuffer;
#endif
	m_wbuffer = new char[size];
	if (m_wbuffer != NULL)
		m_wbuffer_length = size;
	else
		m_wbuffer_length = 0;
	return m_wbuffer;
}

char* CTCPIPUnixDrvr::r_allocate( int size)
{
	if (m_rbuffer != NULL)
#ifndef unixcli
		delete m_rbuffer;
#else
		delete []m_rbuffer;
#endif
	m_rbuffer = new char[size];
	if (m_rbuffer != NULL)
		m_rbuffer_length = size;
	else
		m_rbuffer_length = 0;
	return m_rbuffer;
}

void CTCPIPUnixDrvr::r_assign(char* buffer, long length)
{
	if (m_rbuffer != NULL)
#ifndef unixcli
		delete m_rbuffer;
#else
		delete []m_rbuffer;
#endif
	m_rbuffer = buffer;
	m_rbuffer_length = length;
}

void CTCPIPUnixDrvr::w_assign(char* buffer, long length)
{
	if (m_wbuffer != NULL)
#ifndef unixcli
		delete m_wbuffer;
#else
		delete []m_wbuffer;
#endif
	m_wbuffer = buffer;
	m_wbuffer_length = length;
}

void CTCPIPUnixDrvr::w_release()
{
#ifndef unixcli
	if (m_wbuffer != NULL)
		delete m_wbuffer;
#else
	if (m_wbuffer != NULL)
		delete []m_wbuffer;
#endif
	m_wbuffer = NULL;
	m_wbuffer_length = 0;
}

void CTCPIPUnixDrvr::r_release()
{
#ifndef unixcli
	if (m_rbuffer != NULL)
		delete m_rbuffer;
#else
	if (m_rbuffer != NULL)		
		delete []m_rbuffer;
#endif
	m_rbuffer = NULL;
	m_rbuffer_length = 0;
}

char* CTCPIPUnixDrvr::w_buffer()
{
	return m_wbuffer;
}

char* CTCPIPUnixDrvr::r_buffer()
{
	return m_rbuffer;
}

long CTCPIPUnixDrvr::w_buffer_length()
{
	return m_wbuffer_length;
}

long CTCPIPUnixDrvr::r_buffer_length()
{
	return m_rbuffer_length;
}

void CTCPIPUnixDrvr::send_error(short error, short error_detail, const CEE_handle_def *call_id_)
{
}

CEE_status CTCPIPUnixDrvr::send_response(char* buffer, unsigned long message_length, const CEE_handle_def *call_id_)
{
	return CEE_SUCCESS;
}

void CTCPIPUnixDrvr::CloseSessionWithCleanup()
{
	(gDrvrGlobal.fpTCPIPCloseSession)(this);
	cleanup();
}

char CTCPIPUnixDrvr::swap()
{
	return m_swap;
}
void CTCPIPUnixDrvr::setSwap(char swap)
{
	m_swap = swap;
}

void CTCPIPUnixDrvr::resetSwap()
{
#ifdef BIGE
	m_swap = SWAP_NO;
#else
	m_swap = SWAP_YES;
#endif
}

char CTCPIPUnixDrvr::transport()
{
	return TCPIP;
}

CTCPIPUnixDrvr_list::CTCPIPUnixDrvr_list()
{
	list=NULL;
	InitializeCriticalSection(&CSTCPIP);
}
CTCPIPUnixDrvr_list::~CTCPIPUnixDrvr_list()
{
	DeleteCriticalSection(&CSTCPIP);
}
void CTCPIPUnixDrvr_list::cleanup()
{
	EnterCriticalSection2 (&GTransport.m_TransportCSObject);


	CTCPIPUnixDrvr* cnode = list;
	CTCPIPUnixDrvr* nnode;
	while( cnode != NULL ){
		nnode = cnode->next;
		delete cnode;
		cnode = nnode;
	}
	list=NULL;
	LeaveCriticalSection2 (&GTransport.m_TransportCSObject);
}
CTCPIPUnixDrvr* CTCPIPUnixDrvr_list::ins_node()
{
	EnterCriticalSection2 (&GTransport.m_TransportCSObject);

	CTCPIPUnixDrvr* cnode = list;
	CTCPIPUnixDrvr* pnode = list;
	CTCPIPUnixDrvr* nnode;
	while(cnode!=NULL ){
		pnode=cnode;
		cnode=cnode->next;
	}
	if((nnode = new CTCPIPUnixDrvr())!=NULL){
		nnode->next = cnode;
		if(pnode!=NULL) 
			pnode->next = nnode;
		else
			list = nnode;
	}
	LeaveCriticalSection2 (&GTransport.m_TransportCSObject);
	return nnode ;
}
bool CTCPIPUnixDrvr_list::del_node(CTCPIPUnixDrvr* p )
{
	EnterCriticalSection2 (&GTransport.m_TransportCSObject);
	bool bret = true;

	CTCPIPUnixDrvr* cnode = list;
	CTCPIPUnixDrvr* pnode = list;
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
	LeaveCriticalSection2 (&GTransport.m_TransportCSObject);
	return bret;
}
bool CTCPIPUnixDrvr_list::find_node(CTCPIPUnixDrvr* p )
{
	EnterCriticalSection2 (&GTransport.m_TransportCSObject);
	CTCPIPUnixDrvr* cnode = list;
	bool bfound = false;
	while( cnode != NULL )
	{
		if (cnode == p ){
			bfound = true;
			break;
		}
		cnode = cnode->next;
	}
	LeaveCriticalSection2 (&GTransport.m_TransportCSObject);
	return bfound;
}

CTCPIPUnixDrvr* CTCPIPUnixDrvr_list::find_node(char* object_ref )
{
	EnterCriticalSection2 (&GTransport.m_TransportCSObject);
	CTCPIPUnixDrvr* cnode = list;
	while( cnode != NULL )
	{
		if (strcmp(cnode->m_object_ref, object_ref) == 0 ){
			break;
		}
		cnode = cnode->next;
	}
	LeaveCriticalSection2 (&GTransport.m_TransportCSObject);
	return cnode;
}

//=====================================================================

bool OpenIO (CTCPIPUnixDrvr* pTCPIPSystem, char* object_ref)
{

	bool sts = false;
	RESET_ERRORS((long)pTCPIPSystem);
	if (gDrvrGlobal.gTCPIPLoadError != 0)
	{
		SET_ERROR((long)pTCPIPSystem, PC, TCPIP, UNKNOWN_API, E_DRIVER, gDrvrGlobal.gTCPIPLibrary, O_INIT_PROCESS, F_LOAD_DLL, gDrvrGlobal.gTCPIPLoadError, (int)0);
		sts = false;
	}
	else
	{
		RESET_ERRORS((long)pTCPIPSystem);
		if (gDrvrGlobal.fpTCPIPOpenSession == NULL)
			assert(0);	
		sts = (gDrvrGlobal.fpTCPIPOpenSession)((void*)pTCPIPSystem, object_ref);
	}

	return sts;
}

void CloseIO (CTCPIPUnixDrvr* pTCPIPSystem)
{
	pTCPIPSystem->w_release(); //release write buffer
	(gDrvrGlobal.fpTCPIPCloseSession)((void*)pTCPIPSystem);
}

bool DoIO (CTCPIPUnixDrvr* pTCPIPSystem, IDL_char* wbuffer, IDL_long write_count, IDL_char*& rbuffer, IDL_long& read_count,CConnect *pConnection, CStmt *pStatement)
{
	bool bok = true;

// write count, read count and temporary count
	long wcount;
	long rcount = 0;
	int tcount = 0;

	char* buffer;
	rbuffer = NULL;
// should the header specify swap
#ifdef BIGE
	HEADER wheader = {0,0,0,0,'N',COMP_NO_COMPRESSION,WRITE_REQUEST_FIRST,SIGNATURE,CLIENT_HEADER_VERSION_BE,PC,TCPIP,SWAP_NO,0,0};
	HEADER rheader = {0,0,0,0,'N',COMP_NO_COMPRESSION,READ_RESPONSE_FIRST,SIGNATURE,CLIENT_HEADER_VERSION_BE,PC,TCPIP,SWAP_NO,0,0};
#else
	HEADER wheader = {0,0,0,0,'N',COMP_NO_COMPRESSION,WRITE_REQUEST_FIRST,SIGNATURE,CLIENT_HEADER_VERSION_LE,PC,TCPIP,SWAP_YES,0,0};
	HEADER rheader = {0,0,0,0,'N',COMP_NO_COMPRESSION,READ_RESPONSE_FIRST,SIGNATURE,CLIENT_HEADER_VERSION_LE,PC,TCPIP,SWAP_YES,0,0};
#endif
	HEADER* prheader,*pwheader;

	if(pTCPIPSystem->odbcAPI == AS_API_GETOBJREF)
	{
		;
	}
	else
	{
		if(pTCPIPSystem->swap() == SWAP_YES)
		{
			wheader.swap = SWAP_YES;
			rheader.swap = SWAP_YES;
		}
		else
		{
			wheader.swap = SWAP_NO;
			rheader.swap = SWAP_NO;
		}
	}

	wheader.operation_id	= pTCPIPSystem->odbcAPI;
	rheader.operation_id	= pTCPIPSystem->odbcAPI;
	wheader.dialogueId		= pTCPIPSystem->dialogueId;
	rheader.dialogueId		= pTCPIPSystem->dialogueId;
	unsigned int timeout	= pTCPIPSystem->dwTimeout;
	memset(&pTCPIPSystem->m_rheader, 0, sizeof(HEADER));

	RESET_ERRORS((long)pTCPIPSystem);

	if ((pTCPIPSystem->m_IOCompression != 0)&&(pTCPIPSystem->odbcAPI != AS_API_GETOBJREF))
	{
		wheader.compress_ind = COMP_YES;
		wheader.compress_type = pTCPIPSystem->m_IOCompression;
		rheader.compress_ind = COMP_YES;
		rheader.compress_type = pTCPIPSystem->m_IOCompression;
	}
	else
	{
		wheader.compress_ind = COMP_NO;
		wheader.compress_type = COMP_NO_COMPRESSION;
		rheader.compress_ind = COMP_NO;
		rheader.compress_type = COMP_NO_COMPRESSION;
	}

// send to the server

	wheader.total_length = write_count;
	wheader.hdr_type = WRITE_REQUEST_FIRST;
	if (wheader.compress_ind == COMP_YES  && write_count > pTCPIPSystem->m_IOCompressionlimits)
		DoCompression(pTCPIPSystem, wheader, (unsigned char*)wbuffer, (unsigned int&)write_count);
	else
		wheader.compress_type = COMP_NO_COMPRESSION;

#ifdef TRACE_COMPRESSION
	if(gDrvrGlobal.gTraceCompression)
	{
		printf("%d,sending bytes number: %d\n",wheader.compress_type,write_count);
	}
#endif
	wcount = write_count;

	while (wcount > 0)
	{
		if (wheader.hdr_type == WRITE_REQUEST_FIRST)
		{
			if (wcount > MAX_TCP_BUFFER_LENGTH - sizeof(HEADER))
				tcount = MAX_TCP_BUFFER_LENGTH - sizeof(HEADER);
			else
				tcount = wcount;
		}
		else
		{
			if (wcount > MAX_TCP_BUFFER_LENGTH)
				tcount = MAX_TCP_BUFFER_LENGTH;
			else
				tcount = wcount;
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

	if(bok == false && pConnection != NULL)
	{
		if(MAP_SRVR_ERRORS(pConnection) == TIMEOUT_EXCEPTION)
		{
			if( pTCPIPSystem->odbcAPI != AS_API_GETOBJREF &&
				pTCPIPSystem->odbcAPI != AS_API_STOPSRVR &&
				pTCPIPSystem->odbcAPI != SRVR_API_SQLCONNECT &&
				pTCPIPSystem->odbcAPI != SRVR_API_SQLDISCONNECT)
			{
			    	odbcas_ASSvc_StopSrvr_exc_ stopSrvrException;
				stopSrvrException.exception_nr=SQL_ERROR;
				if(pStatement != NULL)
				   pStatement->setDiagRec(DRIVER_ERROR, IDS_S1_T00, TIMEOUT_EXCEPTION, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem()));
				pConnection->sendStopServer(&stopSrvrException);
				bok = (gDrvrGlobal.fpTCPIPDoWriteRead)((void*)pTCPIPSystem, prheader, rbuffer, tcount, timeout);
			}
		}
	}

	if (bok == false)
		goto out;
		
	if(pTCPIPSystem->odbcAPI == AS_API_GETOBJREF)
	{
		if(prheader->version == SERVER_HEADER_VERSION_LE)
			// we're connected to a Seaquest system
			#if defined(BIGE)
			pTCPIPSystem->setSwap(SWAP_YES);
			#else
			pTCPIPSystem->setSwap(SWAP_NO);
			#endif
		else if(prheader->version == SERVER_HEADER_VERSION_BE)
			// we're connected to a older system
			#if defined(BIGE)
			pTCPIPSystem->setSwap(SWAP_NO);
			#else
			pTCPIPSystem->setSwap(SWAP_YES);
			#endif
		else
			// we're connected to an older system (which will just echo back the version we send)
			// (we could combine this else-case with the previous one, this is just to make it clear)
			#if defined(BIGE)
			pTCPIPSystem->setSwap(SWAP_NO);
			#else
			pTCPIPSystem->setSwap(SWAP_YES);
			#endif
	}


// the server returns total length in the header

	memcpy(&pTCPIPSystem->m_rheader, prheader, sizeof(HEADER));
	if (prheader->compress_ind == COMP_YES && prheader->compress_type != COMP_NO_COMPRESSION)
		rcount = prheader->cmp_length;
	else
		rcount = prheader->total_length;
	read_count = rcount;

	buffer = (char*)pTCPIPSystem->r_allocate(rcount);
	if (buffer == NULL)
	{
		SET_ERROR((long)pTCPIPSystem, UNX, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_OPERATOR_NEW, F_DO_IO, DRVR_ERR_MEMORY_ALLOCATE, rcount);
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
		{
			goto out;
		}
		rcount  -= tcount;
		rbuffer += tcount;
	}

	rbuffer = buffer;
#ifdef TRACE_COMPRESSION
	if(gDrvrGlobal.gTraceCompression)
	{
		printf("%d,receiving bytes number: %d\n",pTCPIPSystem->m_rheader.compress_type,read_count);
	}
#endif
	if (pTCPIPSystem->m_rheader.compress_ind == COMP_YES && pTCPIPSystem->m_rheader.compress_type != COMP_NO_COMPRESSION)
	{
		bok = DoExpand(pTCPIPSystem, pTCPIPSystem->m_rheader, (unsigned char*)rbuffer, (unsigned int&)read_count);
		if (bok)
			rbuffer = pTCPIPSystem->r_buffer();
	}
out:
	return bok;
}

void DoCompression(CTCPIPUnixDrvr* pTCPIPSystem, HEADER& wheader, unsigned char* wbuffer, unsigned int& write_count)
{
	bool retcode = true;
	unsigned long inCmpCount = write_count; // // In number of bytes 
	unsigned long output_size = write_count;
	unsigned char* cmp_buf = NULL;

	retcode = pTCPIPSystem->m_compression.compress((unsigned char*)wbuffer,  // input buffer (data to be compressed)
																  (unsigned int)inCmpCount,              // input number of bytes
																  (int)wheader.compress_type,
	                                               (unsigned char**)&cmp_buf,                 // output buffer containing compressed output
																  (unsigned long&)output_size);            // input/output param - input == max size, on output contains compressed size


	if (retcode == false)
	{
		delete[] cmp_buf;
		wheader.compress_type = COMP_NO_COMPRESSION;
		wheader.cmp_length = 0;
		return;
	}
	write_count = output_size;
	memcpy(wbuffer, cmp_buf, write_count);

	delete[] cmp_buf; //allocated in m_compression.compress();
	wheader.cmp_length = write_count;
}

bool DoExpand(CTCPIPUnixDrvr* pTCPIPSystem, HEADER& rheader, unsigned char* ibuffer, unsigned int& output_size)
{
	bool retcode;
	unsigned char* obuffer;
	int error=0;
	char* tcp_obuffer=NULL;

	if(rheader.compress_ind == COMP_YES )
	{
	  /*
	   * check that the compression types is something that the driver understands
	   */ 
	   if((rheader.compress_type < COMP_DEFAULT) ||
		   (rheader.compress_type > COMP_BEST_COMPRESSION))
	   {
		   SET_ERROR((long)pTCPIPSystem, UNX, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_EXPAND, F_DO_IO, DRVR_ERR_COMPRESS_OPERATION, (int)rheader.compress_type, "unsupported compression type");
		   return false;
	   }
	}
	obuffer = (unsigned char*)new char[rheader.total_length +512]; // +512 is just to be safe
	                                                               // (in case something goes wrong in decompression)
	if (obuffer == NULL)
	{
		SET_ERROR((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_OPERATOR_NEW, F_DO_IO, DRVR_ERR_MEMORY_ALLOCATE, rheader.total_length);
		return false;
	}

	output_size = rheader.total_length; 
	retcode = pTCPIPSystem->m_compression.expand((unsigned char*)ibuffer,            // input compressed buffer    
		                                          (unsigned long)rheader.cmp_length, // input compresses length
		                                          (unsigned char**)&obuffer,            // output buffer after decompression
															   (unsigned long&)output_size,
																(int&)error);       // output size after decompression (should be equal to rheader.total_length)
	//output_size could be 0 or rheader.total_length, no other value.
	if (retcode == false || output_size == 0 )
	{
		delete[] obuffer;
		SET_ERROR((long)pTCPIPSystem, UNX, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_EXPAND, F_DO_IO, DRVR_ERR_COMPRESS_OPERATION, (int)error, "decompress operation failed");
		return false;
	}
	pTCPIPSystem->r_assign((char*)obuffer, output_size);
	rheader.compress_type = COMP_NO_COMPRESSION;
	rheader.cmp_length = 0;
	return true;
}

void WINAPI 
TCPIP_SET_ERROR(long signature, char platform, char transport, int api, ERROR_TYPE error_type, char* process, OPERATION operation, FUNCTION function, int error, int errordetail)
{
	SET_ERROR(signature, platform, transport, api, error_type, process, operation, function, error, errordetail);
}
