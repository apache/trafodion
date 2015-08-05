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

#ifndef _NSKUTIL_DEFINED
#define _NSKUTIL_DEFINED
#include <platform_ndcs.h>
#include "Global.h"
#include "sqlcli.h"
#include "DrvrSrvr.h"
#include "odbcCommon.h"
#include "odbcsrvrcommon.h"
#include "odbc_sv.h" 
#include "odbcMxSecurity.h"
#include "odbcas_cl.h"
#include "srvrfunctions.h"
#include "CSrvrStmt.h"

#define EXT_FILENAME_LEN ZSYS_VAL_LEN_FILENAME
#define MAX_DBNAME_LEN 25 /* (1+7) + (1+7) + (1+8) */
						  /* \SYSTEM.$VOLUME.SUBVOLUME */
namespace SRVR {

//extern BOOL envGetSystemCatalogName (CEE_tag_def objtag_, const CEE_handle_def *call_id_, char *systemCatalogNm);
extern BOOL envGetSystemCatalogName (char *systemCatalogNm);

extern BOOL GetSystemNm (char *systemNm);

extern BOOL envGetCatalogsTable(char *catalogsTableNm);

extern BOOL envGetCatalogName (char *guardianNm, char *catalogNm);

extern BOOL envGetCatalogNameWithNoQuote (char *guardianNm, char *catalogNm);

extern BOOL envGetSQLType (char *guardianNm, char *SQLType);

extern BOOL envGetMXSystemCatalogName (char *systemCatalogNm);

extern BOOL envGetMXSystemCatalogName (char *systemCatalogNm, char *systemName);
//LCOV_EXCL_START
extern BOOL getTimeInterval(long long jtimestamp , int* , short* , short* , short* );
//LCOV_EXCL_STOP
char *_i64toa( __int64 value, char *string, int radix );
}

extern int SystemResources(char *&systemString, short Type);

#endif
