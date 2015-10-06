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
#ifndef __STUBTRACE_H__
#define __STUBTRACE_H__

#include "windows.h"
#include <sql.h>


#define UNALIGNED

class CEnvironment {

public: 
	CEnvironment();
	~CEnvironment();
	void SetEnvironment();
public :
	short	cpu;
	short	pin;
	short	trace_pin;
	long	nodenumber;
	char	nodename[17];
	char	volume[17];
	char	subvolume[17];
	char	progname[17];
};

extern char* versionString;

extern CEnvironment g_Environment;

// TRACE FLAGS
#define TR_ODBC_RANGE_MIN	0x00000000
#define TR_ODBC_ERROR       0x00000002
#define TR_ODBC_WARN        0x00000004
#define TR_ODBC_CONFIG      0x00000008
#define TR_ODBC_INFO        0x00000010
#define TR_ODBC_DEBUG       0x00000020
#define TR_ODBC_TRANSPORT	0x00000040
#define TR_ODBC_RANGE_MAX	0x0FFFFFFF


#endif
