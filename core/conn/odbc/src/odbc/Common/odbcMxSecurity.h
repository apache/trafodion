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
#ifndef _SECDRVR_DEFINED
#define _SECDRVR_DEFINED

#include <sys/timeb.h>
#include "cee.h"
#include "odbcCommon.h"
//#include "odbcsrvrcommon.h"

#define MAX_MRU 5
#define MAX_SID_LEN		68
#define	MAX_SQL_ID_LEN	128
#define MAX_TEXT_SID_LEN	186	// Check the comments in SecDrvr.cpp
								// When you change this make sure you change this value in common.idl

#define UNLEN 256
#define DNLEN 15

#endif
