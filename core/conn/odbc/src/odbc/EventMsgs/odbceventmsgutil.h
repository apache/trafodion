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
//
// FILE: odbcEventMsgUtil.h
//
#ifndef ODBCEVENTMSGUTIL_DEFINED
#define ODBCEVENTMSGUTIL_DEFINED

//#include <platform_ndcs.h>
#include <platform_ndcs.h>

//BOOL StartODBCEventMsg();
extern "C"
void SendODBCEventMsg(DWORD EventId, short EventLogType, DWORD Pid, char *ComponentName,
					  char *ObjectRef, char *ClusterName, int  NodeId, short nToken, va_list vl);
// void StopODBCEventMsg();
typedef void (* SENDMSG_FNPTR)(DWORD EventId, short EventLogType, DWORD Pid, char *ComponentName,
			char *ObjectRef, char *ClusterName, int  NodeId, short nToken, va_list vl);
#endif
