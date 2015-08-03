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
//  EventMsgs.h - Environment object
//

#ifndef EVENTMSGS_DEFINED
#define EVENTMSGS_DEFINED

#include "odbceventMsgUtil.h"
#include "zsysc.h"

class ODBCMXEventMsg
{

private:
	HMODULE				hModule;
	SENDMSG_FNPTR		sendEventMsgPtr;
	char ClusterName[201];
	int NodeId;

public:
	ODBCMXEventMsg();
	~ODBCMXEventMsg();	
	void SendEventMsg(DWORD EventId, short EventLogType, DWORD Pid, char *ComponentName,
			char *ObjectRef, short nToken, ...);
#ifdef NSK_PLATFORM
#define EXT_FILENAME_LEN ZSYS_VAL_LEN_FILENAME
private:
	char ems_name[ EXT_FILENAME_LEN ];
public:
	void open_ems_name( char* collector );
	short ems_fnum;                                                
    void open_ems();
	void close_ems();
	void send_to_ems (short evt_num, short EventLogType, char *ComponentName
								  , char *ObjectRef, short nToken, va_list vl);
	char* get_ems_name( void );
	
	// use this prototype for testing.
//	void send_to_ems (short evt_num, short EventLogType, char *ComponentName
//								  , char *ObjectRef, short nToken, ...)

#endif

};


#endif