/*************************************************************************
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
// MODULE: SrvrFunctions.h
//
//
// PURPOSE: Defines System Meta-Data API's Querry structures
//
/**************************************************************************/

#ifndef _SERVERFUNCTIONS_DEFINED
#define _SERVERFUNCTIONS_DEFINED

//#include	"DrvrSrvr.h"
#include "CoreCommon.h"

#define		STRING_TYPE					1
#define		TABLE_TYPE					2
#define		CATALOG_TYPE				3
#define		SCHEMA_TYPE					4
#define     CATALOG_TYPE_DOT			5
#define     LOCATION_TYPE				6
#define		NSK_CLUSTER_TYPE			7
#define		NSK_SHORTANSI_CATALOG_NAME	8
#define		NSK_CATALOGS_TABLE_LOCATION	9   
#define     END_OF_TABLE				99

#define		CLUSTER_NAME	"NONSTOP_SQLMX_" // for SQL/MX 2.0 system catalog name

/* Linux port. Temprarily moving this to SrvrCommon.h since this file is not yet made part of the compile list.
#define STMT_LABEL_NOT_FOUND	1
#define DATA_TYPE_ERROR			2
#define PREPARE_EXCEPTION		3
#define EXECUTE_EXCEPTION		4
#define DATA_ERROR				5
#define FETCH_EXCEPTION			6
#define TRANSACTION_EXCEPTION	7
*/

struct SMD_SELECT_TABLE{
	short	lineID;
	char	*textLine;
	short	index;
};

struct SMD_QUERY_TABLE {
	char *stmtLabel;
	SMD_SELECT_TABLE *smdSelectTable;
	short sqlStmtType;
	BOOL catalogDependent;
	BOOL preparedState;
	char catalogNm[MAX_ANSI_NAME_LEN+1];
};


SMD_QUERY_TABLE *getSmdSelectTable (SMD_QUERY_TABLE *queryTable, const char *stmtLabel, 
									short *sqlStmtType);
void AssembleSqlString(SMD_SELECT_TABLE *smdSelectTable, const char *catalogNm, 
					   const char *locationNm, char *sqlString);

extern char *smdSchemaList[];
extern char *smdTablesList[];
//extern enum TABLE_INDEX;
//extern enum SCHEMA_INDEX;
#endif	// _SERVERFUNCTIONS_DEFINED
