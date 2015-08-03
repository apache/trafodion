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
#include <windows.h>
#include <idltype.h>
#include "transport.h"
//#include "FileSystemDrvr.h"
#include "TCPIPUnixDrvr.h"

CTransport::CTransport()
{
	m_TCPIPUnixDrvr_list = new CTCPIPUnixDrvr_list();
}
CTransport::~CTransport()
{
	if (m_TCPIPUnixDrvr_list != NULL)
		delete m_TCPIPUnixDrvr_list;
	m_TCPIPUnixDrvr_list = NULL;
}

CTransport GTransport;

extern char*
ALLOC_ERROR_BUFFER()
{
	static char buffer[500];
	return buffer;
}
