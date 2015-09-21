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

#include <windows.h>
#include <idltype.h>
#include "Transport.h"
#include "TCPIPSystemDrvr.h"

CTransport::CTransport()
{
	m_TCPIPSystemDrvr_list = new CTCPIPSystemDrvr_list();
	InitializeCriticalSection(&m_TransportCSObject);
}
CTransport::~CTransport()
{
	if (m_TCPIPSystemDrvr_list != NULL)
		delete m_TCPIPSystemDrvr_list;
	m_TCPIPSystemDrvr_list = NULL;
	DeleteCriticalSection(&m_TransportCSObject);
}

CTransport GTransport;

char* ALLOC_ERROR_BUFFER()
{
extern DWORD gTlsIndex_ErrorBuffer;
	void* lpszStr = NULL;
	if (gTlsIndex_ErrorBuffer != TLS_OUT_OF_INDEXES)
	{
		lpszStr = TlsGetValue(gTlsIndex_ErrorBuffer);
		if (lpszStr == NULL)
		{
			lpszStr = HeapAlloc(GetProcessHeap(), 0, 500);
			TlsSetValue(gTlsIndex_ErrorBuffer, lpszStr);
		}
	}
	return (char*)lpszStr;
}
