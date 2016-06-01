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
//
#ifndef DMINSTALL_H
#define DMINSTALL_H

#include "odbcinst.h"

#ifndef ODBC_ERROR_DRIVER_SPECIFIC
#define ODBC_ERROR_DRIVER_SPECIFIC		23
#endif

extern UWORD configMode;
extern UWORD wSystemDSN;

#define USERDSN_ONLY  0
#define SYSTEMDSN_ONLY  1

/* Definition of the error code array */
#define ERROR_NUM 8

extern DWORD ierror[ERROR_NUM];
extern LPSTR errormsg[ERROR_NUM];
extern SWORD numerrors;

#define CLEAR_ERROR() \
	numerrors = -1;

#define PUSH_ERROR(error) \
	if(numerrors < ERROR_NUM) \
	{ \
		ierror[++numerrors] = (error); \
		errormsg[numerrors] = NULL; \
	}

#define POP_ERROR(error) \
	if(numerrors != -1) \
	{ \
		errormsg[numerrors] = NULL; \
		(error) = ierror[numerrors--]; \
	}

#ifdef IS_ERROR
#  undef IS_ERROR
#endif
#define IS_ERROR() \
	(numerrors != -1) ? 1 : 0

#endif
