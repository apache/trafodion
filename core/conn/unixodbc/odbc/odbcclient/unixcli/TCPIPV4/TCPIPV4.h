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

#ifndef TCPIPV4_H
#define TCPIPV4_H

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "unix_extra.h"

typedef struct _SEND_INFO
{
	SOCKET		s;
#ifndef unixcli
	WSAEVENT	hEvent;
#endif
	unsigned int timeout;
} SEND_INFO;

#define RESIPADDR_FIRST	1
#define RESIPADDR_NEXT	2

#define TCPIPSETERROR_PROCNAME		"TCPIP_SET_ERROR"
typedef void	(WINAPI* FPTCPIPSET_ERROR)(long signature, char platform, char transport, int api, ERROR_TYPE error_type, char* process, OPERATION operation, FUNCTION function, int error, int errordetail, char* error_text);

extern "C"{
int WINAPI TCPIPInitIO(HMODULE Handle);
void WINAPI TCPIPExitIO();
bool WINAPI TCPIPOpenSession(void* pTCPIPSystem, char* object_ref);
void WINAPI TCPIPCloseSession (void* pTCPIPSystem);
bool WINAPI TCPIPDoWriteRead(void* ipTCPIPSystem, HEADER*& hdr, char*& buffer, int& bufcount, unsigned int timeout);

//bool WINAPI TCPIPDoWriteRead (void* pTCPIPSystem, char* wbuffer, long write_count, char*& rbuffer, long& read_count);
}

void ParseObjectRef(char* object_ref, char* pchIP, unsigned short& ushPort);

int send_nblk( SOCKET s, char *buf, int len, int flags, unsigned int timeout, CTCPIPUnixDrvr* pTCPIPSystem);
int recv_nblk( SOCKET s, char *buf, int len, int flags, unsigned int timeout, CTCPIPUnixDrvr* pTCPIPSystem);
void send_close_session( SOCKET s, char *buf, int len, int flags, unsigned int timeout, CTCPIPUnixDrvr* pTCPIPSystem);
int recv_header(SOCKET s, char* buf, int wcount, int bufSize, int headerSize, int flags, unsigned int timeout, CTCPIPUnixDrvr* pTCPIPSystem);
int wait_for_event(SEND_INFO* si, long operation, CTCPIPUnixDrvr* pTCPIPSystem);

#endif


