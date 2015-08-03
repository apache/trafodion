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
// MODULE: SrvrFunctions.cpp
//
// PURPOSE: Implements System Meta-Data API's Querry Formation

// MODIFICATION: fixed a catalog API column header problem (11/13/97)

#include <platform_ndcs.h>
#include <sql.h>
#include <sqlext.h>
#include "odbcCommon.h"
#include "odbc_sv.h"  
#include "srvrcommon.h"
#include "srvrfunctions.h"
#include "Global.h"
#include "NskUtil.h"

using namespace SRVR;

char catalogsTableNm[EXT_FILENAME_LEN+1];

void SRVR::AssembleSqlString(SMD_SELECT_TABLE *smdSelectTable, const char *catalogNm, 
										const char *locationNm, char *sqlString)
{	

	SMD_SELECT_TABLE *lc_smdSelectTable;

	lc_smdSelectTable = smdSelectTable;

	sqlString[0] = '\0';
	while(lc_smdSelectTable->lineID != END_OF_TABLE )
	{
		switch (lc_smdSelectTable->lineID) {
		case STRING_TYPE:
			strcat(sqlString, lc_smdSelectTable->textLine);
			break;
		case TABLE_TYPE:
			strcat(sqlString, smdTablesList[lc_smdSelectTable->index]);
			break;
		case SCHEMA_TYPE:
			strcat(sqlString, smdSchemaList[lc_smdSelectTable->index]);
			break;
		case MX_SYSTEM_CATALOG_NAME:
			strcat(sqlString,srvrGlobal->SystemCatalog);
			break;
		case MX_SYSTEM_CATALOG_NAME_DOT:
			strcat(sqlString,srvrGlobal->SystemCatalog);
			strcat(sqlString, ".");
			break;
		default:
			break;
		}
		lc_smdSelectTable++;
	}
} // End of Function


SMD_QUERY_TABLE *SRVR::getSmdSelectTable (SMD_QUERY_TABLE *queryTable, const char *stmtLabel, IDL_short *sqlStmtType)
{
	short i;

	for (i = 0; ; i++)
	{
		if (queryTable[i].stmtLabel == NULL)
			break;
		if (strcmp(queryTable[i].stmtLabel, stmtLabel) == 0)
		{
			*sqlStmtType = queryTable[i].sqlStmtType;
			return &(queryTable[i]);
		}
	}
	return NULL;
}

#ifdef _DEBUG_1 
//LCOV_EXCL_START
extern "C" long   random(void);
extern "C" void   srandom(unsigned int);
//LCOV_EXCL_STOP
#endif
