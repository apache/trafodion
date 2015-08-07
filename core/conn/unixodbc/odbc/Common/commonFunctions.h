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
/**************************************************************************
**************************************************************************/
#ifndef _COMMONFUNCTION_DEFINED
#define _COMMONFUNCTION_DEFINED

#include "cee.h"
#include "ceeCfg.h"
#include "global.h"

extern CEECFG_Transport getTransport(const IDL_Object srvrObjRef);
extern long getPortNumber(const IDL_Object srvrObjRef);
extern BOOL getInitParam(int argc, char *argv[], SRVR_INIT_PARAM_Def &initParam, char* strName, char* strValue);
extern long ping( char* IPAddress );
extern BOOL checkProcess( char* strProcessName );
extern int setBuildId(char* vProcName);

#endif