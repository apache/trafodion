/*************************************************************************
*
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
**************************************************************************/

#pragma pack(4)

#ifdef _AFXDLL
#include "stdafx.h"
#include <winsock2.h>
#else
#include <windows.h>
#endif

#include "ping.h"
#include "iputils.h"

long ping( char* ObjRef )
{
	char *pSendBuf = "CHAOS is a good thing!";
	WORD wSendLen = (WORD)strlen(pSendBuf);
	DWORD dwReplyLen = sizeof(ICMP_ECHO_REPLY) + max(wSendLen, 8);
	char pReplyBuf[ sizeof(ICMP_ECHO_REPLY) + 30 ];
	HANDLE hIcmp;
	BOOL bResult;
	char IpAddress[100];
	ULONG PingIpAddress;
	int Timeout = PING_TIMEOUT; 
	WSADATA wsaData; 
	int nCount = 0;
	icmp_echo_reply	*icmpReplyBuf;

	IpAddress[0] = '\0';
	for( unsigned int i=0; i < strlen( ObjRef ); i++ )
	{
		if( ObjRef[ 4 + i ] == '/'){
			IpAddress[ i ] = 0;
			break;
		}
		else
			IpAddress[ i ] = ObjRef[ 4 + i ];
	}

	if( strspn( IpAddress, "0123456789." ) != strlen( IpAddress) )
	{
		char IpHost[100];
		strcpy(IpHost,IpAddress);
		if(MapNetBiosNm2IpAddress(IpHost,IpAddress)!=0)
			return INADDR_NONE;
	}

	PingIpAddress = inet_addr( IpAddress );

	if (PingIpAddress == INADDR_NONE) {
		return INADDR_NONE;	// cannot ping an invalid address
	}

	if (WSAStartup(MAKEWORD(1,1),&wsaData) != 0) 
		return GetLastError(); 

	// Open ICMP channel
	hIcmp = IcmpCreateFile();

	if (hIcmp == INVALID_HANDLE_VALUE) { // Kijken: not really a problem
		WSACleanup( );
		return ERROR_INVALID_HANDLE;	// Kijken: with the NIC. should this be TRUE.
	}
	
	while(1) {
  
		if( nCount++==4) break;
 
		// Send echo
		bResult = IcmpSendEcho(hIcmp, PingIpAddress, pSendBuf, wSendLen, NULL, pReplyBuf, 
						   dwReplyLen, Timeout);

		if (bResult)  // If reply comes back, let us check the fist reply
		{
			icmpReplyBuf = (icmp_echo_reply *)pReplyBuf;
			if ((icmpReplyBuf->Status != 0) || 
					(icmpReplyBuf->Status == 0 && (icmpReplyBuf->DataSize != wSendLen 
						|| strncmp((char *)icmpReplyBuf->Data, pSendBuf, wSendLen) != 0)))
			{
				Sleep(PING_SLEEP);
				continue;
			}
			else
			{
				(void) IcmpCloseHandle(hIcmp);
				WSACleanup( );
				return ERROR_SUCCESS;
			}
		}
		else
		{
			Sleep(PING_SLEEP);
			continue;
		}
	}

	// Bookkeeping
	(void) IcmpCloseHandle(hIcmp);
	WSACleanup( );
	return WSAETIMEDOUT;
}

#ifdef AFXDLL
#define _AFXDLL
#undef AFXDLL
#endif
 
