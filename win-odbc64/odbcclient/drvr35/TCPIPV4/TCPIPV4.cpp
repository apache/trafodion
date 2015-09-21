/**********************************************************************
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
********************************************************************/
/*************************************************************************
**************************************************************************/
//

#include "stdafx.h"
#include "Transport.h"
#include "TCPIPSystemDrvr.h"
#include "swap.h"
#include "TCPIPV4.h"
#include "../drvrglobal.h"

FPTCPIPSET_ERROR fpTCPIPSET_ERROR = NULL;

BOOL APIENTRY DllMain( HANDLE hInst, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls((HINSTANCE)hInst);
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}

int WINAPI TCPIPInitIO(HMODULE Handle)
{
	HMODULE gModuleHandle;
	if (Handle == NULL)
		return DRVR_ERR_INVALID_DLLHANDLE;
	gModuleHandle = Handle;
	if ((fpTCPIPSET_ERROR = (FPTCPIPSET_ERROR)GetProcAddress( gModuleHandle, TCPIPSETERROR_PROCNAME)) == (FPTCPIPSET_ERROR)NULL)
		return DRVR_ERR_CANNOTLOAD_PROCADDRESS;

	return 0;
}

void WINAPI TCPIPExitIO()
{
}

void WINAPI TCPIPCloseSession (void* ipTCPIPSystem)
{
	CTCPIPSystemDrvr* pTCPIPSystem = (CTCPIPSystemDrvr*)ipTCPIPSystem;

	if (pTCPIPSystem->m_hSocket == NULL)
		return;
	shutdown(pTCPIPSystem->m_hSocket,SD_BOTH);
	closesocket(pTCPIPSystem->m_hSocket);
	memset(pTCPIPSystem->m_object_ref,0,sizeof(pTCPIPSystem->m_object_ref));
	pTCPIPSystem->m_hSocket = NULL;

	if (pTCPIPSystem->m_hEvents[0] != NULL)
		CloseHandle(pTCPIPSystem->m_hEvents[0]);
	pTCPIPSystem->m_hEvents[0] = NULL;
	
	if (pTCPIPSystem->m_hEvents[1] != NULL)
		CloseHandle(pTCPIPSystem->m_hEvents[1]);
	pTCPIPSystem->m_hEvents[1] = NULL;
}

bool WINAPI TCPIPOpenSession(void* ipTCPIPSystem, char* object_ref)
{
	long haddr = 0;
	struct hostent *hostEntry = NULL;
	unsigned short ushPort=0;
	char pchIP[MAX_OBJECT_REF + 1];
	struct sockaddr_in addr,* paddr;
	bool bConnected = false;

	CTCPIPSystemDrvr* pTCPIPSystem = (CTCPIPSystemDrvr*)ipTCPIPSystem;

	if (pTCPIPSystem->m_hSocket != NULL)
		return true;

	if (pTCPIPSystem->odbcAPI != AS_API_GETOBJREF &&
		pTCPIPSystem->odbcAPI != AS_API_UPDATESRVRSTATE &&
		pTCPIPSystem->odbcAPI != AS_API_STOPSRVR &&
		pTCPIPSystem->odbcAPI != AS_API_STATUSDSALL &&
		pTCPIPSystem->odbcAPI != SRVR_API_SQLCONNECT)
	{
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, object_ref, O_OPEN_SESSION, F_SOCKET, 10054, (int)0);
		return false;
	}

	if ((pTCPIPSystem->m_hSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, object_ref, O_OPEN_SESSION, F_SOCKET, WSAGetLastError(), (int)0);
		pTCPIPSystem->m_hSocket = NULL;
		return false;
	}

	ParseObjectRef(object_ref, pchIP, ushPort);
	strcpy(pTCPIPSystem->m_object_ref, object_ref);

	int nBlockSIze = MAX_TCP_BUFFER_LENGTH;
	if (setsockopt(pTCPIPSystem->m_hSocket, SOL_SOCKET, SO_SNDBUF, (char*)&nBlockSIze, sizeof(int)) != 0)
	{
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, object_ref, O_OPEN_SESSION, F_SETSOCOPT, WSAGetLastError(), (int)SO_SNDBUF);
		TCPIPCloseSession (ipTCPIPSystem);
		return false;
	};
	nBlockSIze = MAX_TCP_BUFFER_LENGTH;
	if (setsockopt(pTCPIPSystem->m_hSocket, SOL_SOCKET, SO_RCVBUF, (char*)&nBlockSIze, sizeof(int)) != 0)
	{
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, object_ref, O_OPEN_SESSION, F_SETSOCOPT, WSAGetLastError(), (int)SO_RCVBUF);
		TCPIPCloseSession (ipTCPIPSystem);
		return false;
	};

	paddr = &addr;
	memset((char *)paddr, 0, sizeof(struct sockaddr_in));
	paddr->sin_family = AF_INET;
	paddr->sin_port = htons(ushPort);

	if(( haddr = inet_addr(pchIP)) == INADDR_NONE )
	{
		int count = 0;
		hostEntry = gethostbyname(pchIP );
		if (hostEntry)
			while( hostEntry->h_addr_list[count] )
			{
				memcpy((char *)&paddr->sin_addr.s_addr, hostEntry->h_addr_list[count], hostEntry->h_length );
				if (connect(pTCPIPSystem->m_hSocket, (const struct sockaddr *)&addr, sizeof(SOCKADDR)) != SOCKET_ERROR)
				{
					bConnected = true;
					break;
				}
				else
					count++;
			}
	}
	else
	{
		paddr->sin_addr.s_addr = haddr;
		if (connect(pTCPIPSystem->m_hSocket, (const struct sockaddr *)&addr, sizeof(SOCKADDR)) != SOCKET_ERROR)
		{
			bConnected = true;
		}
	}
	if (bConnected == false)
	{
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, object_ref, O_OPEN_SESSION, F_CONNECT, DRVR_ERR_WRONG_IP_ADDRESS, (int)0);
		TCPIPCloseSession (ipTCPIPSystem);
		return false;
	}

	struct linger_def
	{
		u_short l_onoff;
		u_short l_linger;
	} linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;
	if (setsockopt(pTCPIPSystem->m_hSocket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger)) != 0)
	{
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, object_ref, O_OPEN_SESSION, F_SETSOCOPT, WSAGetLastError(), (int)SO_LINGER);
		TCPIPCloseSession (ipTCPIPSystem);
		return false;
	};

	if (NULL == (pTCPIPSystem->m_hEvents[0] = CreateEvent(NULL, FALSE, FALSE,"")))
	{
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, object_ref, O_OPEN_SESSION, F_WSACREATEEVENT, WSAGetLastError(), (int)0);
		TCPIPCloseSession (ipTCPIPSystem);
		return false;
	}
	if (NULL == (pTCPIPSystem->m_hEvents[1] = CreateEvent(NULL, FALSE, FALSE,"")))
	{
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, object_ref, O_OPEN_SESSION, F_WSACREATEEVENT, WSAGetLastError(), (int)1);
		TCPIPCloseSession (ipTCPIPSystem);
		return false;
	}
	return true;
}

bool WINAPI TCPIPDoWriteRead(void* ipTCPIPSystem, HEADER*& hdr, char*& buffer, int& bufcount, unsigned int timeout)
{
	HEADER close_hdr;
	HEADER* error_hdr;
	short error = 0;
	short error_detail = 0;
	int wcount;
	int data_length = 0;
	CTCPIPSystemDrvr* pTCPIPSystem = (CTCPIPSystemDrvr*)ipTCPIPSystem;

	if (pTCPIPSystem->m_hSocket == NULL)
	{
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_CHECKCONNECTION, 999, (int)0);
		return false;
	}

	switch(hdr->hdr_type)
	{
	default:
	case WRITE_REQUEST_FIRST:
		memcpy(pTCPIPSystem->m_IObuffer,hdr,sizeof(HEADER));
		HEADER_swap((HEADER*)pTCPIPSystem->m_IObuffer);
		data_length = sizeof(HEADER);
	case WRITE_REQUEST_NEXT:
		if (buffer != NULL)
			memcpy(pTCPIPSystem->m_IObuffer + data_length, buffer, bufcount);
		data_length += bufcount;
		if ((wcount = send_nblk(pTCPIPSystem->m_hSocket, 
								pTCPIPSystem->m_IObuffer, 
								data_length, 
								0,
								timeout,
								pTCPIPSystem)) == SOCKET_ERROR|| wcount == 0)
		{
			TCPIPCloseSession (ipTCPIPSystem);
			return false;
		}
		break;
	case READ_RESPONSE_FIRST:
	case READ_RESPONSE_NEXT:
		if ((wcount = recv_nblk(pTCPIPSystem->m_hSocket, 
								pTCPIPSystem->m_IObuffer, 
								MAX_TCP_BUFFER_LENGTH, 
								0,
								timeout,
								pTCPIPSystem)) == SOCKET_ERROR || wcount == 0)
		{
			TCPIPCloseSession (ipTCPIPSystem);
			return false;
		}
		if (hdr->hdr_type == READ_RESPONSE_FIRST && wcount < sizeof HEADER)
		{
			if ((wcount = recv_header(pTCPIPSystem->m_hSocket, 
									pTCPIPSystem->m_IObuffer, 
									wcount,
									MAX_TCP_BUFFER_LENGTH,
									sizeof HEADER,
									0,
									timeout,
									pTCPIPSystem)) == SOCKET_ERROR || wcount == 0)
			{
				TCPIPCloseSession (ipTCPIPSystem);
				return false;
			}
		}
		break;
	case CLOSE_TCPIP_SESSION:
		memcpy(&close_hdr,hdr,sizeof(HEADER));
		HEADER_swap((HEADER*)&close_hdr);
		data_length = sizeof(HEADER);
		send_close_session(pTCPIPSystem->m_hSocket, 
								(char*)&close_hdr, 
								data_length, 
								0,
								30,
								pTCPIPSystem);
		return true;
	}
	bufcount = 0;
	switch(hdr->hdr_type)
	{
	case WRITE_REQUEST_FIRST:
		bufcount -= sizeof(HEADER);
	case WRITE_REQUEST_NEXT:
		bufcount += wcount;
		if (bufcount < 0)
		{
			error = DRVR_ERR_INCORRECT_LENGTH;
			(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_SEND, error, bufcount);
			TCPIPCloseSession (ipTCPIPSystem);
			return false;
		}
		break;
	case READ_RESPONSE_FIRST:
		bufcount = wcount - sizeof(HEADER);
		if (bufcount < 0)
		{
			error = DRVR_ERR_INCORRECT_LENGTH;
			(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_RECV, error, bufcount);
			TCPIPCloseSession (ipTCPIPSystem);
			return false;
		}
		buffer = pTCPIPSystem->m_IObuffer + sizeof(HEADER);
		hdr = (HEADER*)pTCPIPSystem->m_IObuffer;
		if (hdr->signature != SIGNATURE)
		{
			error = DRVR_ERR_WRONGSIGNATURE;
			(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_RECV, error, (int)0);
			TCPIPCloseSession (ipTCPIPSystem);
			return false;
		}
		if (hdr->error != 0 )
		{
			error = DRVR_ERR_ERROR_FROM_SERVER;
			(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_RECV, hdr->error, hdr->error_detail);
			TCPIPCloseSession (ipTCPIPSystem);
			return false;
		}
		break;
	case READ_RESPONSE_NEXT:
		bufcount = wcount;
		memcpy(buffer, pTCPIPSystem->m_IObuffer, bufcount); 
		break;
	case SRVR_TRANSPORT_ERROR:
		error_hdr = (HEADER*)(pTCPIPSystem->m_IObuffer);
		error = error_hdr->error;
		error_detail = error_hdr->error_detail;
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_SRVR_TRANSPORT_ERROR, error, error_detail);
		TCPIPCloseSession (ipTCPIPSystem);
		break;
	default:
		error = 1000;
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_HDR_TYPE, error, (int)0);
		TCPIPCloseSession (ipTCPIPSystem);
		break;
	}
	return error == 0;
}

void ParseObjectRef(char* object_ref, char* pchIP, unsigned short& ushPort)
{
	char buffer[MAX_OBJECT_REF + 1];

	strncpy(buffer, object_ref, MAX_OBJECT_REF);
	buffer[MAX_OBJECT_REF] = 0;
	char* pIp = NULL, * pPort = NULL;

	pchIP[0] = 0;
	ushPort = 0;

	if ( strtok(buffer, ":") != NULL)
		if ((pIp = strtok(NULL,"/")) != NULL )
			if ((pPort = strtok(NULL, ":")) != NULL)
			{
				strcpy (pchIP, pIp);
				ushPort = atoi(pPort);
			}
}

int send_nblk( SOCKET s, char *buf, int len, int flags, unsigned int timeout, CTCPIPSystemDrvr* pTCPIPSystem)
{
	unsigned long sflags = 0;
	int nLeft = len;
	int idx = 0;
	int retcode;
	long lastResult; 

	DWORD written = 0;
	WSABUF wsabuf;
	WSAOVERLAPPED overlapped;

	SEND_INFO si;
	si.hEvent = pTCPIPSystem->m_hEvents[1];
	si.s = s;
	si.timeout = timeout;

	overlapped.hEvent = pTCPIPSystem->m_hEvents[1];

	while (nLeft > 0)
	{
		wsabuf.buf = &buf[idx];
		wsabuf.len = nLeft;
		if (WSASend(s, &wsabuf, 1, &written, 0, &overlapped, NULL) == 0)
		{
			lastResult = written;
		}
		else
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_SEND, WSAGetLastError(), (int)0);
				return false;
			}
			retcode = wait_for_event(&si, FD_WRITE, pTCPIPSystem);
			if (retcode == 0)
				return false;
			if (FALSE==WSAGetOverlappedResult(s, &overlapped, &written, false, &sflags))
			{
				(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_SEND_GETOVERLAPPEDRESULTS, WSAGetLastError(), (int)0);
				return false;
			}
			lastResult = written;
		}

		nLeft -= lastResult;
		idx += lastResult;
	}
	return idx;
}

int recv_nblk( SOCKET s, char *buf, int len, int flags, unsigned int timeout, CTCPIPSystemDrvr* pTCPIPSystem)
{
	unsigned long lastResult;
	unsigned long rflags = 0;
	int retcode;
	WSABUF wsabuf = {len, (char *)buf};
	WSAOVERLAPPED overlapped;
		
	SEND_INFO si;
	si.hEvent = pTCPIPSystem->m_hEvents[0];
	si.s = s;
	si.timeout = timeout;

	overlapped.hEvent = pTCPIPSystem->m_hEvents[0];

	if (WSARecv(s, &wsabuf, 1, &lastResult, &rflags, &overlapped, NULL) == 0)
	{
		if (lastResult == 0)
		{
			(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_RECV, COMM_LINK_FAIL_EXCEPTION, (int)0);
			return 0;
		}
	}
	else
	{
		switch (WSAGetLastError())
		{
		default:
			(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_RECV, WSAGetLastError(), (int)0);
			return SOCKET_ERROR;
		case WSAEDISCON:
			(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_RECV, COMM_LINK_FAIL_EXCEPTION, (int)0);
			return WSAEDISCON;
		case WSA_IO_PENDING:
			retcode = wait_for_event(&si, FD_READ, pTCPIPSystem);
			if (retcode == false)
				return 0;
			if (WSAGetOverlappedResult(s, &overlapped, &lastResult, false, &rflags))
			{
				if (lastResult == 0)
				{
					(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_RECV_GETOVERLAPPEDRESULTS, COMM_LINK_FAIL_EXCEPTION, (int)0);
					return 0;
				}
			}
			else
			{
				switch (WSAGetLastError())
					{
					default:
						(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_RECV_GETOVERLAPPEDRESULTS, WSAGetLastError(), (int)0);
						return SOCKET_ERROR;
					case WSAEDISCON:
						(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_RECV_GETOVERLAPPEDRESULTS, COMM_LINK_FAIL_EXCEPTION, (int)0);
						return WSAEDISCON;
					}
			}
		}
	}
	return lastResult;
}

void send_close_session( SOCKET s, char *buf, int len, int flags, unsigned int timeout, CTCPIPSystemDrvr* pTCPIPSystem)
{
	send_nblk(s, buf, len, flags, timeout, pTCPIPSystem);
}

int recv_header(SOCKET s, char* buf, int wcount, int bufSize, int headerSize, int flags, unsigned int timeout, CTCPIPSystemDrvr* pTCPIPSystem)
{

	int already_read = wcount;
	char* pbuf;
	int len;
	int retcode;

	while (already_read < headerSize )
	{
		pbuf = buf + already_read;
		len = bufSize - already_read;

		retcode = recv_nblk(s, pbuf, len, flags, timeout, pTCPIPSystem);
		if (retcode == 0 || retcode == SOCKET_ERROR)
			return retcode;
		already_read += retcode;
	}
	return already_read;
}
int wait_for_event(SEND_INFO* si, long operation, CTCPIPSystemDrvr* pTCPIPSystem)
{
	DWORD dwEvent;
	unsigned int wtimeout = (si->timeout == 0)? INFINITE: si->timeout * 1000;

	dwEvent = WaitForSingleObject(si->hEvent,wtimeout);
	switch(dwEvent)
	{
	case WAIT_OBJECT_0:
		break;
	case WAIT_FAILED:
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_WSAWAITFORMULTIPLEEVENTS, GetLastError(), (int)operation);
		TCPIPCloseSession (pTCPIPSystem);
		return false;
	case WAIT_TIMEOUT:
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_WSAWAITFORMULTIPLEEVENTS, TIMEOUT_EXCEPTION, (unsigned int)wtimeout);
		TCPIPCloseSession (pTCPIPSystem);
		return false;
	}

	return true;
}



