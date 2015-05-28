/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1998-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
********************************************************************/
/*************************************************************************
**************************************************************************/
//

//#include "stdafx.h"
#include "transport.h"
#include "TCPIPUnixDrvr.h"
#include "swap.h"
#include "TCPIPV4.h"
#include "drvrglobal.h"

#ifdef MXOSS
#include <sys/socket.h>
#elif defined(MXLINUX) || defined(MXIA64LINUX)
#include <bits/socket.h>
#elif (defined(MXHPUX) || defined(MXSUNSPARC) ) && !defined MXOSS
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#elif defined MXAIX
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#endif

#include <assert.h>
FPTCPIPSET_ERROR fpTCPIPSET_ERROR = &SET_ERROR;
#ifdef unixcli
#include <errno.h>
#include <sys/time.h>
#endif

#if !defined(INADDR_NONE) && defined(MXSUNSPARC)
// Solaris does not define this
#define INADDR_NONE             ((in_addr_t)0xffffffff) /* -1 return */ 
#endif

// This function if for TCPIPv4 windows dll... not needed for this.
/*BOOL APIENTRY DllMain( HANDLE hInst, 
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
*/

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
	int i=1;
	char rbuf;

	CTCPIPUnixDrvr* pTCPIPSystem = (CTCPIPUnixDrvr*)ipTCPIPSystem;

	if (pTCPIPSystem->m_hSocket == INVALID_SOCKET)
		return;
	shutdown(pTCPIPSystem->m_hSocket,SHUT_WR);
#ifndef unixcli	
	closesocket(pTCPIPSystem->m_hSocket);
#else

	close(pTCPIPSystem->m_hSocket);
#endif
	memset(pTCPIPSystem->m_object_ref,0,sizeof(pTCPIPSystem->m_object_ref));
	pTCPIPSystem->m_hSocket = INVALID_SOCKET;

	if (pTCPIPSystem->m_hEvents[0] != NULL)
		CloseHandle(pTCPIPSystem->m_hEvents[0]);
	pTCPIPSystem->m_hEvents[0] = NULL;
	
	if (pTCPIPSystem->m_hEvents[1] != NULL)
		CloseHandle(pTCPIPSystem->m_hEvents[1]);
	pTCPIPSystem->m_hEvents[1] = NULL;
}

bool WINAPI TCPIPOpenSession(void* ipTCPIPSystem, char* object_ref)
{
	in_addr_t haddr = 0;
	struct hostent *hostEntry = NULL;
	unsigned short ushPort=0;
	char pchIP[MAX_OBJECT_REF + 1];
	short retryCount = 0;
	int     dwSleep =500;
#ifndef unixcli
	struct sockaddr_in addr, *paddr;
#else
	struct timeval tv;
	sockaddr_in addr, *paddr;
#endif
	bool bConnected = false;


	CTCPIPUnixDrvr* pTCPIPSystem = (CTCPIPUnixDrvr*)ipTCPIPSystem;

	if (pTCPIPSystem->m_hSocket != INVALID_SOCKET)
		return true;

	if (pTCPIPSystem->odbcAPI != AS_API_GETOBJREF &&
		pTCPIPSystem->odbcAPI != AS_API_UPDATESRVRSTATE &&
		pTCPIPSystem->odbcAPI != AS_API_STOPSRVR &&
		pTCPIPSystem->odbcAPI != AS_API_STATUSDSALL &&
		pTCPIPSystem->odbcAPI != SRVR_API_SQLCONNECT)
	{
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, UNX, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, object_ref, O_OPEN_SESSION, F_SOCKET, DRVR_ERR_INVALID_CODE_PATH, (int)0, "Connection lost, invalid code path");
		return false;
	}

	if ((pTCPIPSystem->m_hSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, UNX, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, object_ref, O_OPEN_SESSION, F_SOCKET, errno, (int)0, strerror(errno));
		pTCPIPSystem->m_hSocket = INVALID_SOCKET;
		return false;
	}
// this is the default, for now we will change it to the max value
	int nBlockSIze = TCPI_DRVR_SEND_BUFFER;
	if (setsockopt(pTCPIPSystem->m_hSocket, SOL_SOCKET, SO_SNDBUF, (char*)&nBlockSIze, sizeof(int)) != 0)
	{
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, UNX, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, object_ref, O_OPEN_SESSION, F_SETSOCOPT, errno, (int)SO_SNDBUF, strerror(errno));
		TCPIPCloseSession (ipTCPIPSystem);
		return false;
	}

	nBlockSIze = TCPI_DRVR_RECV_BUFFER;
	if (setsockopt(pTCPIPSystem->m_hSocket, SOL_SOCKET, SO_RCVBUF, (char*)&nBlockSIze, sizeof(int)) != 0)
	{
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, UNX, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, object_ref, O_OPEN_SESSION, F_SETSOCOPT, errno, (int)SO_RCVBUF, strerror(errno));
		TCPIPCloseSession (ipTCPIPSystem);
		return false;
	}

	int optval = true;
	if (setsockopt(pTCPIPSystem->m_hSocket, SOL_SOCKET,SO_KEEPALIVE, (char*)&optval, sizeof(optval)) != 0)
	{
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, UNX, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, object_ref, O_OPEN_SESSION, F_SETSOCOPT, errno, (int)SO_KEEPALIVE, strerror(errno));
		TCPIPCloseSession (ipTCPIPSystem);
		return false;
	};

	ParseObjectRef(object_ref, pchIP, ushPort);
	strcpy(pTCPIPSystem->m_object_ref, object_ref);
	paddr = &addr;
	memset((char *)paddr, 0, sizeof(struct sockaddr_in));
	paddr->sin_family = AF_INET;
	paddr->sin_port = htons(ushPort);
#if defined MXHPUX || defined MXOSS || MXAIX
	if(( haddr = inet_addr(pchIP)) == -1 )
#else
	if(( haddr = inet_addr(pchIP)) == INADDR_NONE )
#endif
	{
		int count = 0;
		hostEntry = gethostbyname(pchIP );
		if (hostEntry)
		{
			while ((bConnected == false) && (retryCount < 4)) //retry four times
			{
				while( hostEntry->h_addr_list[count] )
				{
					memcpy((char *)&paddr->sin_addr.s_addr, hostEntry->h_addr_list[count], hostEntry->h_length );
					if (connect(pTCPIPSystem->m_hSocket, (const struct sockaddr *)&addr, sizeof(struct sockaddr)) != SOCKET_ERROR)
					{
						bConnected = true;
						break;
					}
					else
						count++;
				}
				if ((pTCPIPSystem->odbcAPI == AS_API_GETOBJREF) && (bConnected == false) && (
								(errno == ETIMEDOUT) || (errno == EHOSTUNREACH)))
				{
					Sleep(dwSleep);
					retryCount++;
					count = 0;
				}
				else
					retryCount = 4; // Do not retry if not AS_API_GETOBJREF
			}
		}//if
	}
	else
	{
		paddr->sin_addr.s_addr = haddr;
		while ((bConnected == false) && (retryCount < 4)) //retry four times
		{
			if (connect(pTCPIPSystem->m_hSocket, (const struct sockaddr *)&addr, sizeof(struct sockaddr)) != SOCKET_ERROR)
			{
				bConnected = true;
			}
			else 
				if ((pTCPIPSystem->odbcAPI == AS_API_GETOBJREF) && ((errno == ETIMEDOUT) || errno == EHOSTUNREACH))
				{
					Sleep(dwSleep);
					retryCount++;
				}
				else
					retryCount = 4; // Do not retry if not AS_API_GETOBJREF

		}
	}
	if (bConnected == false)
	{
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, UNX, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, object_ref, O_OPEN_SESSION, F_CONNECT, errno, (int)0, strerror(errno));
		TCPIPCloseSession (ipTCPIPSystem);
		return false;
	}

#ifndef unixcli
	if (NULL == (pTCPIPSystem->m_hEvents[0] = CreateEvent(NULL, FALSE, FALSE,"")))
	{
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, object_ref, O_OPEN_SESSION, F_WSACREATEEVENT, WSAGetLastError(), (int)0);
		TCPIPCloseSession (ipTCPIPSystem);
		return false;
	}
	if (NULL == (pTCPIPSystem->m_hEvents[1] = CreateEvent(NULL, FALSE, FALSE,"")))
	{
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, object_ref, O_OPEN_SESSION, F_WSACREATEEVENT, WSAGetLastError(), (int)1, NULL);
		TCPIPCloseSession (ipTCPIPSystem);
		return false;
	}
#endif	
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
	CTCPIPUnixDrvr* pTCPIPSystem = (CTCPIPUnixDrvr*)ipTCPIPSystem;

	if (pTCPIPSystem->m_hSocket == INVALID_SOCKET)
	{
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, UNX, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_CHECKCONNECTION, 999, (int)0, "Invalid Socket");
		return false;
	}
	
	switch(hdr->hdr_type)
	{
	default:	
	case WRITE_REQUEST_FIRST:
		memcpy(pTCPIPSystem->m_IObuffer,hdr,sizeof(HEADER));
		if(pTCPIPSystem->swap() == SWAP_YES)
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
			(fpTCPIPSET_ERROR)((long)pTCPIPSystem, UNX, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_SEND, 0, wcount, "WRITE_REQUEST_FIRST/NEXT Error");
			TCPIPCloseSession (ipTCPIPSystem);
			return false;
		}
		break;
	case READ_RESPONSE_FIRST:
	case READ_RESPONSE_NEXT:
		wcount = recv_nblk(pTCPIPSystem->m_hSocket, 
						   pTCPIPSystem->m_IObuffer, 
						   MAX_TCP_BUFFER_LENGTH, 
						   0,
						   timeout,
						   pTCPIPSystem); 
		/*
		 * recv_nblk returns a negative -ETIMEDOUT value in case of a timeout
		 * for anything other than a connect/disconnect/getobject reference or a stop server request, we need
		 * to send a Cancel request and wait for either a graceful cancel completion or a communication link failure 
		 * because of the process_stop on the MXOSRVR
		 * A negative ETIMEDOUT value is returned -- otherwise the caller will interpret it as that many bytes read (all *NIX has a +ve value for ETIMEDOUT)
		 */

		if(wcount == -ETIMEDOUT)
		   return false;

		if(wcount == SOCKET_ERROR || wcount == 0)
		{
			(fpTCPIPSET_ERROR)((long)pTCPIPSystem, UNX, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_RECV, 0, wcount, "READ_RESPONSE_FIRST/NEXT Error");
			TCPIPCloseSession (ipTCPIPSystem);
			return false;
		}
		if ((hdr->hdr_type == READ_RESPONSE_FIRST) && (wcount < sizeof(HEADER)))
		{
			if ((wcount = recv_header(pTCPIPSystem->m_hSocket, 
									pTCPIPSystem->m_IObuffer, 
									wcount,
									MAX_TCP_BUFFER_LENGTH,
									sizeof(HEADER),
									0,
									timeout,
									pTCPIPSystem)) == SOCKET_ERROR || wcount == 0)
			{
				(fpTCPIPSET_ERROR)((long)pTCPIPSystem, UNX, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_RECV, 0, wcount, "READ_RESPONSE_FIRST/NEXT recv_header() Error");
				TCPIPCloseSession (ipTCPIPSystem);
				return false;
			}
		}
		break;
	case CLOSE_TCPIP_SESSION:
		memcpy(&close_hdr,hdr,sizeof(HEADER));
		if(pTCPIPSystem->swap() == SWAP_YES)
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
			(fpTCPIPSET_ERROR)((long)pTCPIPSystem, UNX, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_SEND, error, bufcount, "Driver Incorrect Length Error");
			TCPIPCloseSession (ipTCPIPSystem);
			return false;
		}
		break;
	case READ_RESPONSE_FIRST:
		bufcount = wcount - sizeof(HEADER);
		if (bufcount < 0)
		{
			error = DRVR_ERR_INCORRECT_LENGTH;
			(fpTCPIPSET_ERROR)((long)pTCPIPSystem, UNX, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_RECV, error, bufcount, "Driver Incorrect Length Error");
			TCPIPCloseSession (ipTCPIPSystem);
			return false;
		}
		buffer = pTCPIPSystem->m_IObuffer + sizeof(HEADER);
		hdr = (HEADER*)pTCPIPSystem->m_IObuffer;
		if (hdr->signature != SIGNATURE)
		{
			error = DRVR_ERR_WRONGSIGNATURE;
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, UNX, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_RECV, error, (int)0, "Wrong Socket Signature");
			TCPIPCloseSession (ipTCPIPSystem);
			return false;
		}
		if (hdr->error != 0 )
		{
			error = DRVR_ERR_ERROR_FROM_SERVER;
			(fpTCPIPSET_ERROR)((long)pTCPIPSystem, UNX, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_RECV, hdr->error, hdr->error_detail, "Error From Server");
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
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, UNX, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_SRVR_TRANSPORT_ERROR, error, error_detail, "Server Transport Error");
		TCPIPCloseSession (ipTCPIPSystem);
		break;
	default:
		error = 1000;
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_HDR_TYPE, error, (int)0, "Unknown Header Type" );
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
	char *lasts;

	pchIP[0] = 0;
	ushPort = 0;
#ifndef unixcli
	if ( strtok(buffer, ":") != NULL)
		if ((pIp = strtok(NULL,"/")) != NULL )
			if ((pPort = strtok(NULL, ":")) != NULL)
			{
				strcpy (pchIP, pIp);
				ushPort = atoi(pPort);
			}

#else
	if ( (pIp = strtok_r(buffer, ":",&lasts)) != NULL)
	{
	   if(strcmp(pIp,"TCP") == 0)
	      pIp = strtok_r(NULL,"/",&lasts);

	   if ( pIp != NULL && (pPort = strtok_r(NULL,":",&lasts)) != NULL )
	   {
	      strcpy (pchIP, pIp);
		  ushPort = atoi(pPort);
	   }
	}
#endif
}
#ifndef unixcli
int send_nblk( SOCKET s, char *buf, int len, int flags, long timeout, CTCPIPUnixDrvr* pTCPIPSystem)
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

#else
int send_nblk( SOCKET s, char *buf, int len, int flags, unsigned int timeout, CTCPIPUnixDrvr* pTCPIPSystem)
{
	unsigned long sflags = 0;
	int nLeft = len;
	int idx = 0;
	int retcode;
	long lastResult; 
	int result;
	char * sbuf;
	sbuf = buf;
	int retries;

	while (nLeft > 0)
	{
		retries = 0;
		do {
			result = send(s, sbuf, nLeft , 0);

		} while( (result == -1) && (errno == EINTR) && (retries++ < 3));

		if (result == -1)
		{	
			(fpTCPIPSET_ERROR)((long)pTCPIPSystem, UNX, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_SEND, errno, (int)0, strerror(errno));
			return false; // this needs to be better developed
		}
		sbuf += result;
		nLeft -= result;
		idx += result;
	}
	
	// return the total number of bytes written
	return idx;
}
#endif

#ifndef unixcli
// the windows implmentation for recv_nblk
// left here for the time being for reference purposes....
int recv_nblk( SOCKET s, char *buf, int len, int flags, unsigned int timeout, CTCPIPUnixDrvr* pTCPIPSystem)
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

#else
// the unix implemenation of the recv_nblk
int recv_nblk( SOCKET s, char *buf, int len, int flags, unsigned int timeout, CTCPIPUnixDrvr* pTCPIPSystem)
{
	unsigned long lastResult;
	unsigned long rflags = 0;
	int retcode;
	char * rbuf;
	int lrecv;
	fd_set rset;
	int retries;

	struct timeval tv;

	// we use select to deal timeout scenarios	
	if (timeout != 0)
	{
		tv.tv_usec = 0;
		#ifndef __LP64__
		// timeouts per ODBC spec are unsigned 32 bit values
		// the tv_sec param is a signed long
		// so for the 64-bit driver, the timeout param can be accomodated.
		// however for the 32-bit driver, we need to set it to INT_MAX, otherwise
		// it will be treated as a negative timeout value which is incorrect
		// (On the Win32 driver, the timeout value in WaitForMulitpleObject is 
		// a DWWORD which is also a unsigned 32 bit entity)
		if(timeout > INT_MAX)
		   tv.tv_sec = INT_MAX;
		else
		   tv.tv_sec = timeout;
		#else
		tv.tv_sec = timeout;
		#endif
		retries = 0;
		do {
			FD_ZERO(&rset);
			FD_SET(s, &rset);
			retcode = select(s + 1, &rset, NULL, NULL, &tv);
		} while( (retcode == -1) && (errno == EINTR) && (retries++ < 3));

	    if (retcode == -1)
    	{
		   (fpTCPIPSET_ERROR)((long)pTCPIPSystem, UNX, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_SELECT, errno, (int)0, strerror(errno));
		   return 0; // hmmm this needs to be better completed as well... errno should be set
	    } 
   	    else if (retcode == 0)
     	{
			  (fpTCPIPSET_ERROR)((long)pTCPIPSystem, UNX, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_SELECT, ETIMEDOUT, (unsigned int)timeout, strerror(ETIMEDOUT));
			/*
			 * for anything other than a connect/disconnect/getobject reference or a stop server request, we need
			 * to send a Cancel request and wait for either a graceful cancel completion or a communication link failure 
			 * because of the process_stop on the MXOSRVR
			 * We will return a negative ETIMEDOUT -- otherwise the caller will interpret it as that many bytes read (all *NIX has a +ve value for ETIMEDOUT)
			 */

			if( pTCPIPSystem->odbcAPI != AS_API_GETOBJREF &&
				pTCPIPSystem->odbcAPI != AS_API_STOPSRVR &&
				pTCPIPSystem->odbcAPI != SRVR_API_SQLCONNECT &&
				pTCPIPSystem->odbcAPI != SRVR_API_SQLDISCONNECT)
				return -ETIMEDOUT;
			else
				return 0;
		}
	}

	retries = 0;
	do {
		retcode = recv (s, buf, len, 0);
	} while( (retcode == -1) && (errno == EINTR) && (retries++ < 3));


	if (retcode == -1)
	{
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, UNX, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_RECV, errno, (int)0, strerror(errno));
		return 0; // hmmm this needs to be better completed as well... errno should be set
	}
    if (retcode == 0) // The peer has performed an orderly shutdown
	{
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, UNX, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_RECV, ECONNRESET, int(0), "recv() returned zero bytes");
		return 0;
	}
	return retcode;
}

#endif

void send_close_session( SOCKET s, char *buf, int len, int flags, unsigned int timeout, CTCPIPUnixDrvr* pTCPIPSystem)
{
	send_nblk(s, buf, len, flags, timeout, pTCPIPSystem);
}

int recv_header(SOCKET s, char* buf, int wcount, int bufSize, int headerSize, int flags, unsigned int timeout, CTCPIPUnixDrvr* pTCPIPSystem)
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
#ifndef unixcli
int wait_for_event(SEND_INFO* si, long operation, CTCPIPUnixDrvr* pTCPIPSystem)
{
	DWORD dwEvent;
	long wtimeout = (si->timeout == 0)? INFINITE: si->timeout * 1000;

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
		(fpTCPIPSET_ERROR)((long)pTCPIPSystem, PC, TCPIP, pTCPIPSystem->odbcAPI, E_DRIVER, pTCPIPSystem->m_object_ref, O_DO_WRITE_READ, F_WSAWAITFORMULTIPLEEVENTS, TIMEOUT_EXCEPTION, (int)wtimeout);
		TCPIPCloseSession (pTCPIPSystem);
		return false;
	}

	return true;
}
#endif



