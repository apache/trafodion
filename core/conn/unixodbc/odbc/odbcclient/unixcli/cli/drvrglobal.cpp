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
/*************************************************************************
**************************************************************************/
//
//

#ifdef MXHPUX
/* 
   hpux defines their own version of UINT64 in dlfcn.h without checking if its already defined.
   we define it in sqltypes.h if its not already defined - this causes a 'redefinition'
   error at compile time on HPUX
   since we check if UINT64 before trying to create a new typedef, this hack will
   prevent the redefinition error.
*/
#define UINT64 UINT64
#endif
#include <dlfcn.h>

#ifdef MXOSS
#include <spthread.h>
#endif

#include <errno.h>
#include "windows.h"
#include "drvrglobal.h"
#include "mxomsg.h"
#include "diagfunctions.h" //10-080124-0030 
#include <cstmt.h>
#include <signal.h>
#include <odbcinst.h>
#include "stubtrace.h"
#ifndef MXOSS
#include "StaticLocking.h"
#endif

// Declare the global variable

typedef unsigned long*  (SQL_API *FPGetODBCSharedData) ();
extern FPGetODBCSharedData pGetODBCSharedData = NULL;

CDrvrGlobal	 gDrvrGlobal;
extern DWORD	*pdwGlobalTraceVariable = NULL;

HMODULE g_hOdbcDM = NULL;
HMODULE	g_hTraceDLL	= NULL;

DATATYPE_TABLE gSQLDatatypeMap[] = 
{
//   conciseType,					verboseType,		datetimeIntervalCode		
	{SQL_CHAR,						SQL_CHAR,			0,							SQL_DESC_LENGTH,	0,					SQL_DESC_LENGTH,	SQL_DESC_LENGTH,				SQL_C_CHAR,				"CHAR"},
	{SQL_VARCHAR,					SQL_VARCHAR,		0,							SQL_DESC_LENGTH,	0,					SQL_DESC_LENGTH,	SQL_DESC_LENGTH,				SQL_C_CHAR,				"VARCHAR"},
	{SQL_LONGVARCHAR,				SQL_LONGVARCHAR,	0,							SQL_DESC_LENGTH,	0,					SQL_DESC_LENGTH,	SQL_DESC_LENGTH,				SQL_C_CHAR,				"LONG VARCHAR"},
	{SQL_DECIMAL,					SQL_DECIMAL,		0,							SQL_DESC_PRECISION,	SQL_DESC_SCALE,		SQL_DESC_PRECISION, SQL_DESC_PRECISION,				SQL_C_CHAR,				"DECIMAL"},
	{SQL_NUMERIC,					SQL_NUMERIC,		0,							SQL_DESC_PRECISION,	SQL_DESC_SCALE,		SQL_DESC_PRECISION, SQL_DESC_PRECISION,				SQL_C_CHAR,				"NUMERIC"},
	{SQL_SMALLINT,					SQL_SMALLINT,		0,							SQL_DESC_PRECISION,	SQL_DESC_SCALE,		6,					sizeof(SHORT),					SQL_C_SHORT,			"SMALLINT"},
	{SQL_INTEGER,					SQL_INTEGER,		0,							SQL_DESC_PRECISION,	SQL_DESC_SCALE,		11,					sizeof(INT),					SQL_C_LONG,				"INTEGER"},
	{SQL_REAL,						SQL_REAL,			0,							SQL_DESC_PRECISION,	0,					13,					sizeof(FLOAT),					SQL_C_FLOAT,			"REAL"},
	{SQL_FLOAT,						SQL_FLOAT,			0,							SQL_DESC_PRECISION,	0,					22,					sizeof(DOUBLE),					SQL_C_DOUBLE,			"FLOAT"},
	{SQL_DOUBLE,					SQL_DOUBLE,			0,							SQL_DESC_PRECISION,	0,					22,					sizeof(DOUBLE),					SQL_C_DOUBLE,			"DOUBLE PRECISION"},
	{SQL_BIT,						SQL_BIT,			0,							SQL_DESC_LENGTH,	0,					1,					sizeof(SCHAR),					SQL_C_BIT,				"BIT"},
	{SQL_TINYINT,					SQL_TINYINT,		0,							SQL_DESC_PRECISION,	SQL_DESC_SCALE,		4,					sizeof(CHAR),					SQL_C_TINYINT,			"TINYINT"},
	{SQL_BIGINT,					SQL_BIGINT,			0,							SQL_DESC_PRECISION,	SQL_DESC_SCALE,		20,					20,								SQL_C_SBIGINT,			"BIGINT"},
	{SQL_BINARY,					SQL_BINARY,			0,							SQL_DESC_LENGTH,	0,					-1,					SQL_DESC_LENGTH,				SQL_C_BINARY,			"BINARY"},
	{SQL_VARBINARY,					SQL_VARBINARY,		0,							SQL_DESC_LENGTH,	0,					-1,					SQL_DESC_LENGTH,				SQL_C_BINARY,			"VARBINARY"},
	{SQL_LONGVARBINARY,				SQL_LONGVARBINARY,	0,							SQL_DESC_LENGTH,	0,					-1,					SQL_DESC_LENGTH,				SQL_C_BINARY,			"LONG VARBINARY"},
	{SQL_WCHAR,						SQL_WCHAR,			0,							SQL_DESC_LENGTH,	0,					SQL_DESC_LENGTH,	SQL_DESC_LENGTH,				SQL_C_CHAR,				"NCHAR"},
	{SQL_WVARCHAR,					SQL_WVARCHAR,		0,							SQL_DESC_LENGTH,	0,					SQL_DESC_LENGTH,	SQL_DESC_LENGTH,				SQL_C_CHAR,				"NVARCHAR"},
	{SQL_WLONGVARCHAR,				SQL_WLONGVARCHAR,	0,							SQL_DESC_LENGTH,	0,					SQL_DESC_LENGTH,	SQL_DESC_LENGTH,				SQL_C_CHAR,				"LONG NVARCHAR"},
	{SQL_DATE,						SQL_DATETIME,		SQL_CODE_DATE,				SQL_DESC_LENGTH,	SQL_DESC_PRECISION, SQL_DESC_LENGTH,	sizeof(SQL_DATE_STRUCT),		SQL_C_TYPE_DATE,		"DATE"},
	{SQL_TIME,						SQL_DATETIME,		SQL_CODE_TIME,				SQL_DESC_LENGTH,	SQL_DESC_PRECISION,	SQL_DESC_LENGTH,	sizeof(SQL_TIME_STRUCT),		SQL_C_TYPE_TIME,		"TIME"},
	{SQL_TIMESTAMP,					SQL_DATETIME,		SQL_CODE_TIMESTAMP,			SQL_DESC_LENGTH,	SQL_DESC_PRECISION,	SQL_DESC_LENGTH,	sizeof(SQL_TIMESTAMP_STRUCT),	SQL_C_TYPE_TIMESTAMP,	"TIMESTAMP"},
	{SQL_TYPE_DATE,					SQL_DATETIME,		SQL_CODE_DATE,				SQL_DESC_LENGTH,	SQL_DESC_PRECISION, SQL_DESC_LENGTH,	sizeof(SQL_DATE_STRUCT),		SQL_C_TYPE_DATE,		"DATE"},
	{SQL_TYPE_TIME,					SQL_DATETIME,		SQL_CODE_TIME,				SQL_DESC_LENGTH,	SQL_DESC_PRECISION,	SQL_DESC_LENGTH,	sizeof(SQL_TIME_STRUCT),		SQL_C_TYPE_TIME,		"TIME"},
	{SQL_TYPE_TIMESTAMP,			SQL_DATETIME,		SQL_CODE_TIMESTAMP,			SQL_DESC_LENGTH,	SQL_DESC_PRECISION,	SQL_DESC_LENGTH,	sizeof(SQL_TIMESTAMP_STRUCT),	SQL_C_TYPE_TIMESTAMP,	"TIMESTAMP"},
	// Make sure you check the values before uncommenting, This is not kept in sync with the structure changes
	{SQL_INTERVAL_MONTH,			SQL_INTERVAL,		SQL_CODE_MONTH,				SQL_DESC_LENGTH,	0,					SQL_DESC_LENGTH,	sizeof(SQL_INTERVAL_STRUCT),	SQL_C_INTERVAL_MONTH,			"INTERVAL MONTH"},
	{SQL_INTERVAL_YEAR,				SQL_INTERVAL,		SQL_CODE_YEAR,				SQL_DESC_LENGTH,	0,					SQL_DESC_LENGTH,	sizeof(SQL_INTERVAL_STRUCT),	SQL_C_INTERVAL_YEAR,			"INTERVAL YEAR"},
	{SQL_INTERVAL_YEAR_TO_MONTH,	SQL_INTERVAL,		SQL_CODE_YEAR_TO_MONTH,		SQL_DESC_LENGTH,	0,					SQL_DESC_LENGTH,	sizeof(SQL_INTERVAL_STRUCT),	SQL_C_INTERVAL_YEAR_TO_MONTH,	"INTERVAL YEAR TO MONTH"},
	{SQL_INTERVAL_DAY,				SQL_INTERVAL,		SQL_CODE_DAY,				SQL_DESC_LENGTH,	0,					SQL_DESC_LENGTH,	sizeof(SQL_INTERVAL_STRUCT),	SQL_C_INTERVAL_DAY,				"INTERVAL DAY"},
	{SQL_INTERVAL_HOUR,				SQL_INTERVAL,		SQL_CODE_HOUR,				SQL_DESC_LENGTH,	0,					SQL_DESC_LENGTH,	sizeof(SQL_INTERVAL_STRUCT),	SQL_C_INTERVAL_HOUR,			"INTERVAL HOUR"},
	{SQL_INTERVAL_MINUTE,			SQL_INTERVAL,		SQL_CODE_MINUTE,			SQL_DESC_LENGTH,	0,					SQL_DESC_LENGTH,	sizeof(SQL_INTERVAL_STRUCT),	SQL_C_INTERVAL_MINUTE,			"INTERVAL MINUTE"},
	{SQL_INTERVAL_SECOND,			SQL_INTERVAL,		SQL_CODE_SECOND,			SQL_DESC_LENGTH,	SQL_DESC_PRECISION, SQL_DESC_LENGTH,	sizeof(SQL_INTERVAL_STRUCT),	SQL_C_INTERVAL_SECOND,			"INTERVAL SECOND"},
	{SQL_INTERVAL_DAY_TO_HOUR,		SQL_INTERVAL,		SQL_CODE_DAY_TO_HOUR,		SQL_DESC_LENGTH,	0,					SQL_DESC_LENGTH,	sizeof(SQL_INTERVAL_STRUCT),	SQL_C_INTERVAL_DAY_TO_HOUR,		"INTERVAL DAY TO HOUR"},
	{SQL_INTERVAL_DAY_TO_MINUTE,	SQL_INTERVAL,		SQL_CODE_DAY_TO_MINUTE,		SQL_DESC_LENGTH,	0,					SQL_DESC_LENGTH,	sizeof(SQL_INTERVAL_STRUCT),	SQL_C_INTERVAL_DAY_TO_MINUTE,	"INTERVAL DAY TO MINUTE"},
	{SQL_INTERVAL_DAY_TO_SECOND,	SQL_INTERVAL,		SQL_CODE_DAY_TO_SECOND,		SQL_DESC_LENGTH,	SQL_DESC_PRECISION, SQL_DESC_LENGTH,	sizeof(SQL_INTERVAL_STRUCT),	SQL_C_INTERVAL_DAY_TO_SECOND,	"INTERVAL DAY TO SECOND"},
	{SQL_INTERVAL_HOUR_TO_MINUTE,	SQL_INTERVAL,		SQL_CODE_HOUR_TO_MINUTE,	SQL_DESC_LENGTH,	0,					SQL_DESC_LENGTH,	sizeof(SQL_INTERVAL_STRUCT),	SQL_C_INTERVAL_HOUR_TO_MINUTE,	"INTERVAL HOUR TO MINUTE"},
	{SQL_INTERVAL_HOUR_TO_SECOND,	SQL_INTERVAL,		SQL_CODE_HOUR_TO_SECOND,	SQL_DESC_LENGTH,	SQL_DESC_PRECISION, SQL_DESC_LENGTH,	sizeof(SQL_INTERVAL_STRUCT),	SQL_C_INTERVAL_HOUR_TO_SECOND,	"INTERVAL HOUR TO SECOND"},
	{SQL_INTERVAL_MINUTE_TO_SECOND,	SQL_INTERVAL,		SQL_CODE_MINUTE_TO_SECOND,	SQL_DESC_LENGTH,	SQL_DESC_PRECISION, SQL_DESC_LENGTH,	sizeof(SQL_INTERVAL_STRUCT),	SQL_C_INTERVAL_MINUTE_TO_SECOND,"INTERVAL MINUTE TO SECOND"},
	{SQL_DEFAULT,					SQL_DEFAULT,		0,							0,					0}
};

DATATYPE_TABLE gCDatatypeMap[] = 
{
//   conciseType,						verboseType,		datetimeIntervalCode
	{SQL_C_CHAR,						SQL_C_CHAR,			0, 0, 0, 0,	SQL_DESC_LENGTH,			SQL_C_CHAR,			"CHAR"},
	{SQL_C_SHORT,						SQL_C_SHORT,		0, 0, 0, 0,	sizeof(short),				SQL_C_SHORT,		"SMALLINT"},
	{SQL_C_SSHORT,						SQL_C_SSHORT,		0, 0, 0, 0,	sizeof(short),				SQL_C_SSHORT,		"SMALLINT"},	
	{SQL_C_USHORT,						SQL_C_USHORT,		0, 0, 0, 0,	sizeof(unsigned short),		SQL_C_USHORT,		"SMALLINT"},
	{SQL_C_LONG,						SQL_C_LONG,			0, 0, 0, 0,	sizeof(int),				SQL_C_LONG,			"INTEGER"},
	{SQL_C_SLONG,						SQL_C_SLONG,		0, 0, 0, 0,	sizeof(int),				SQL_C_SLONG,		"INTEGER"},
	{SQL_C_ULONG,						SQL_C_ULONG,		0, 0, 0, 0,	sizeof(unsigned int),		SQL_C_ULONG,		"INTEGER"},
	{SQL_C_FLOAT,						SQL_C_FLOAT,		0, 0, 0, 0,	sizeof(float),				SQL_C_FLOAT,		"FLOAT"},
	{SQL_C_DOUBLE,						SQL_C_DOUBLE,		0, 0, 0, 0,	sizeof(double),				SQL_C_DOUBLE,		"DOUBLE"},
	{SQL_C_BIT,							SQL_C_BIT,			0, 0, 0, 0,	sizeof(unsigned char),		SQL_C_BIT,			"BIT"},
	{SQL_C_TINYINT,						SQL_C_TINYINT,		0, 0, 0, 0,	sizeof(signed char),		SQL_C_TINYINT,		"TINYINT"},	
	{SQL_C_STINYINT,					SQL_C_STINYINT,		0, 0, 0, 0,	sizeof(signed char),		SQL_C_STINYINT,		"TINYINT"},
	{SQL_C_UTINYINT,					SQL_C_UTINYINT,		0, 0, 0, 0,	sizeof(unsigned char),		SQL_C_UTINYINT,		"TINYINT"},
	{SQL_C_SBIGINT,						SQL_C_SBIGINT,		0, 0, 0, 0,	sizeof(__int64),			SQL_C_SBIGINT,		"BIGINT"},
#if defined MXHPUX || defined MXOSS || defined MXAIX
	{SQL_C_UBIGINT,						SQL_C_UBIGINT,		0, 0, 0, 0,	sizeof(__int64),	        SQL_C_UBIGINT,		"BIGINT"},
#else
    {SQL_C_UBIGINT,                     SQL_C_UBIGINT,      0, 0, 0, 0, sizeof(unsigned __int64),   SQL_C_UBIGINT,      "BIGINT"},
#endif
	{SQL_C_BINARY,						SQL_C_BINARY,		0, 0, 0, 0,	SQL_DESC_LENGTH,			SQL_C_BINARY,		"BINARY"},
	{SQL_C_BOOKMARK,					SQL_C_BOOKMARK,		0, 0, 0, 0,	sizeof(unsigned long),		SQL_C_BOOKMARK,		"BOOKMARK"},
	{SQL_C_VARBOOKMARK,					SQL_C_VARBOOKMARK,	0, 0, 0, 0,	SQL_DESC_LENGTH,			SQL_C_VARBOOKMARK,	"VARBOOKMARK"},
	{SQL_C_NUMERIC,						SQL_C_NUMERIC,		0, 0, 0, 0,	sizeof(SQL_NUMERIC_STRUCT),	SQL_C_NUMERIC,		"NUMERIC"},
	{SQL_C_WCHAR,						SQL_C_WCHAR,		0, 0, 0, 0,	SQL_DESC_LENGTH,		SQL_C_WCHAR,		"NCHAR"},
	{SQL_C_DATE,						SQL_DATETIME,		SQL_CODE_DATE,				0, 0, 0,	sizeof(DATE_STRUCT),		SQL_TYPE_DATE,		"DATE"},
	{SQL_C_TIME,						SQL_DATETIME,		SQL_CODE_TIME,				0, 0, 0,	sizeof(TIME_STRUCT),		SQL_TYPE_TIME,		"TIME"},
	{SQL_C_TIMESTAMP,					SQL_DATETIME,		SQL_CODE_TIMESTAMP,			0, 0, 0,	sizeof(TIMESTAMP_STRUCT),	SQL_TYPE_TIMESTAMP,		"TIMESTAMP"},
	{SQL_C_TYPE_DATE,					SQL_DATETIME,		SQL_CODE_DATE,				0, 0, 0,	sizeof(DATE_STRUCT),		SQL_TYPE_DATE,		"DATE"},
	{SQL_C_TYPE_TIME,					SQL_DATETIME,		SQL_CODE_TIME,				0, 0, 0,	sizeof(TIME_STRUCT),		SQL_TYPE_TIME,		"TIME"},
	{SQL_C_TYPE_TIMESTAMP,				SQL_DATETIME,		SQL_CODE_TIMESTAMP,			0, 0, 0,	sizeof(TIMESTAMP_STRUCT),	SQL_TYPE_TIMESTAMP,		"TIMESTAMP"},
	{SQL_C_INTERVAL_MONTH,				SQL_INTERVAL,		SQL_CODE_MONTH,				0, 0, 0,	sizeof(SQL_INTERVAL_STRUCT),SQL_INTERVAL_MONTH,		"INTERVAL MONTH"},
	{SQL_C_INTERVAL_YEAR,				SQL_INTERVAL,		SQL_CODE_YEAR,				0, 0, 0,	sizeof(SQL_INTERVAL_STRUCT),SQL_INTERVAL_YEAR,		"INTERVAL YEAR"},
	{SQL_C_INTERVAL_YEAR_TO_MONTH,		SQL_INTERVAL,		SQL_CODE_YEAR_TO_MONTH,		0, 0, 0,	sizeof(SQL_INTERVAL_STRUCT),SQL_INTERVAL_YEAR_TO_MONTH,		"INTERVAL YEAR TO MONTH"},
	{SQL_C_INTERVAL_DAY,				SQL_INTERVAL,		SQL_CODE_DAY,				0, 0, 0,	sizeof(SQL_INTERVAL_STRUCT),SQL_INTERVAL_DAY,		"INTERVAL DAY"},
	{SQL_C_INTERVAL_HOUR,				SQL_INTERVAL,		SQL_CODE_HOUR,				0, 0, 0,	sizeof(SQL_INTERVAL_STRUCT),SQL_INTERVAL_HOUR,		"INTERVAL HOUR"},
	{SQL_C_INTERVAL_MINUTE,				SQL_INTERVAL,		SQL_CODE_MINUTE,			0, 0, 0,	sizeof(SQL_INTERVAL_STRUCT),SQL_INTERVAL_MINUTE,		"INTERVAL MINUTE"},
	{SQL_C_INTERVAL_SECOND,				SQL_INTERVAL,		SQL_CODE_SECOND,			0, 0, 0,	sizeof(SQL_INTERVAL_STRUCT),SQL_INTERVAL_SECOND,		"INTERVAL SECOND"},
	{SQL_C_INTERVAL_DAY_TO_HOUR,		SQL_INTERVAL,		SQL_CODE_DAY_TO_HOUR,		0, 0, 0,	sizeof(SQL_INTERVAL_STRUCT),SQL_INTERVAL_DAY_TO_HOUR,		"INTERVAL DAY TO HOUR"},
	{SQL_C_INTERVAL_DAY_TO_MINUTE,		SQL_INTERVAL,		SQL_CODE_DAY_TO_MINUTE,		0, 0, 0,	sizeof(SQL_INTERVAL_STRUCT),SQL_INTERVAL_DAY_TO_MINUTE,		"INTERVAL DAY TO MINUTE"},
	{SQL_C_INTERVAL_DAY_TO_SECOND,		SQL_INTERVAL,		SQL_CODE_DAY_TO_SECOND,		0, 0, 0,	sizeof(SQL_INTERVAL_STRUCT),SQL_INTERVAL_DAY_TO_SECOND,		"INTERVAL DAY TO SECOND"},
	{SQL_C_INTERVAL_HOUR_TO_MINUTE,		SQL_INTERVAL,		SQL_CODE_HOUR_TO_MINUTE,	0, 0, 0,	sizeof(SQL_INTERVAL_STRUCT),SQL_INTERVAL_HOUR_TO_MINUTE,		"INTERVAL HOUR TO MINUTE"},
	{SQL_C_INTERVAL_HOUR_TO_SECOND,		SQL_INTERVAL,		SQL_CODE_HOUR_TO_SECOND,	0, 0, 0,	sizeof(SQL_INTERVAL_STRUCT),SQL_INTERVAL_HOUR_TO_SECOND,		"INTERVAL HOUR TO SECOND"},
	{SQL_C_INTERVAL_MINUTE_TO_SECOND,	SQL_INTERVAL,		SQL_CODE_MINUTE_TO_SECOND,	0, 0, 0,	sizeof(SQL_INTERVAL_STRUCT),SQL_INTERVAL_MINUTE_TO_SECOND,		"INTERVAL MINUTE TO SECOND"},
	{SQL_C_DEFAULT}
};

char *ConnectKeywords[] =
{
	"DRIVER",
	"DSN",
	"FILEDSN",
	"SERVER",
	"SAVEFILE",
	"UID",
	"PWD",
	"CATALOG",
	"SCHEMA",
	"FETCHBUFFERSIZE",
	"SQL_ATTR_CONNECTION_TIMEOUT",
	"SQL_LOGIN_TIMEOUT",
	"SQL_QUERY_TIMEOUT",
	"DATALANG",
	"ERRORMSGLANG",
	"CTRLINFERNCHAR",
	"TRANSLATIONDLL",
	"TRANSLATIONOPTION",
	"REPLACEMENTCHAR",
	"PCAPP",
	"ROWSETERRORRECOVERY",
	"INTSECURITYLVL",
	"SERVERDSN",
	"SERVICE",
	"SESSION",
	"APPLICATION",
	"ROLENAME",
	"CERTIFICATEDIR",
	"CERTIFICATEFILE",
	"CERTIFICATEFILE_ACTIVE",
	"COMPRESSION",
	"COMPRESSIONTHRESHOLD",
	NULL
};

char *ConnectLocalizedIdentifier[] =
{
	"Driver",
	"Data Source Name",
	"File Data Source Name",
	"Server ID",
	"Save File",
	"Login ID",
	"Password",
	"Catalog",
	"Schema",
	"Fetch Buffer Size (kbytes)",
	"Connection Timeout (seconds)",
	"Login Timeout (seconds)",
	"Query Timeout (seconds)",
	"Client/Server Character Set",
	"Client Error Message Language",
	"Infer NCHAR",
	"Translate DLL Name",
	"Translate Option",
	"Replacement Character",
	"PCAPP",
	"Rowset Error Recovery",
	"Integrated Security Lvl",
	"Server Data Source Name",
	"Service Name",
	"Session Name",
	"Application Name",
	"Role Name",
	"Certificate Directory",
	"Certificate File",
	"Certificate File Active",
	"Compression",
	"CompressionThreshold",
	NULL,
};

DESC_SET_TYPE	gDescSetTypes[] =
{	
	// FieldIdentifier				ARD					APD					IRD					IPD
	{SQL_DESC_ALLOC_TYPE,			READ_DESC_TYPE,		READ_DESC_TYPE,		READ_DESC_TYPE,		READ_DESC_TYPE},
	{SQL_DESC_ARRAY_SIZE,			RW_DESC_TYPE,		RW_DESC_TYPE,		UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE},
	{SQL_DESC_ARRAY_STATUS_PTR,		RW_DESC_TYPE,		RW_DESC_TYPE,		RW_DESC_TYPE,		RW_DESC_TYPE},
	{SQL_DESC_BIND_OFFSET_PTR,		RW_DESC_TYPE,		RW_DESC_TYPE,		UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE},
	{SQL_DESC_BIND_TYPE,			RW_DESC_TYPE,		RW_DESC_TYPE,		UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE},
	{SQL_DESC_COUNT,				RW_DESC_TYPE,		RW_DESC_TYPE,		READ_DESC_TYPE,		RW_DESC_TYPE},
	{SQL_DESC_ROWS_PROCESSED_PTR,	UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE,	RW_DESC_TYPE,		RW_DESC_TYPE},
	{SQL_DESC_AUTO_UNIQUE_VALUE,	UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE,	READ_DESC_TYPE,		UNUSED_DESC_TYPE},
	{SQL_DESC_BASE_COLUMN_NAME,		UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE,	READ_DESC_TYPE,		UNUSED_DESC_TYPE},
	{SQL_DESC_BASE_TABLE_NAME,		UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE,	READ_DESC_TYPE,		UNUSED_DESC_TYPE},
	{SQL_DESC_CASE_SENSITIVE,		UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE,	READ_DESC_TYPE,		READ_DESC_TYPE},
	{SQL_DESC_CATALOG_NAME,			UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE,	READ_DESC_TYPE,		UNUSED_DESC_TYPE},
	{SQL_DESC_CONCISE_TYPE,			RW_DESC_TYPE,		RW_DESC_TYPE,		READ_DESC_TYPE,		RW_DESC_TYPE},
	{SQL_DESC_DATA_PTR,				RW_DESC_TYPE,		RW_DESC_TYPE,		UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE},
	{SQL_DESC_DATETIME_INTERVAL_CODE,	RW_DESC_TYPE,	RW_DESC_TYPE,		READ_DESC_TYPE,		RW_DESC_TYPE},
	{SQL_DESC_DATETIME_INTERVAL_PRECISION,	RW_DESC_TYPE,	RW_DESC_TYPE,	READ_DESC_TYPE,		RW_DESC_TYPE},
	{SQL_DESC_DISPLAY_SIZE,			UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE,	READ_DESC_TYPE,		UNUSED_DESC_TYPE},
	{SQL_DESC_FIXED_PREC_SCALE,		UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE,	READ_DESC_TYPE,		READ_DESC_TYPE},
	{SQL_DESC_INDICATOR_PTR,		RW_DESC_TYPE,		RW_DESC_TYPE,		UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE},
	{SQL_DESC_LABEL,				UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE,	READ_DESC_TYPE,		UNUSED_DESC_TYPE},
	{SQL_DESC_LENGTH,				RW_DESC_TYPE,		RW_DESC_TYPE,		READ_DESC_TYPE,		RW_DESC_TYPE},
	{SQL_DESC_LITERAL_PREFIX,		UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE,	READ_DESC_TYPE,		UNUSED_DESC_TYPE},
	{SQL_DESC_LITERAL_SUFFIX,		UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE,	READ_DESC_TYPE,		UNUSED_DESC_TYPE},
	{SQL_DESC_LOCAL_TYPE_NAME,		UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE,	READ_DESC_TYPE,		READ_DESC_TYPE},
	{SQL_DESC_NAME,					UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE,	READ_DESC_TYPE,		RW_DESC_TYPE},
	{SQL_DESC_NULLABLE,				UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE,	READ_DESC_TYPE,		READ_DESC_TYPE},
	{SQL_DESC_NUM_PREC_RADIX,		RW_DESC_TYPE,		RW_DESC_TYPE,		READ_DESC_TYPE,		READ_DESC_TYPE},
	{SQL_DESC_OCTET_LENGTH,			RW_DESC_TYPE,		RW_DESC_TYPE,		READ_DESC_TYPE,		RW_DESC_TYPE},
	{SQL_DESC_OCTET_LENGTH_PTR,		RW_DESC_TYPE,		RW_DESC_TYPE,		UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE},
	{SQL_DESC_PARAMETER_TYPE,		UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE,	RW_DESC_TYPE},
	{SQL_DESC_PRECISION,			RW_DESC_TYPE,		RW_DESC_TYPE,		READ_DESC_TYPE,		RW_DESC_TYPE},
	{SQL_DESC_SCALE,				RW_DESC_TYPE,		RW_DESC_TYPE,		READ_DESC_TYPE,		RW_DESC_TYPE},
	{SQL_DESC_SCHEMA_NAME,			UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE,	READ_DESC_TYPE,		UNUSED_DESC_TYPE},
	{SQL_DESC_SEARCHABLE,			UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE,	READ_DESC_TYPE,		UNUSED_DESC_TYPE},
	{SQL_DESC_TABLE_NAME,			UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE,	READ_DESC_TYPE,		UNUSED_DESC_TYPE},
	{SQL_DESC_TYPE,					RW_DESC_TYPE,		RW_DESC_TYPE,		READ_DESC_TYPE,		RW_DESC_TYPE},
	{SQL_DESC_TYPE_NAME,			UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE,	READ_DESC_TYPE,		READ_DESC_TYPE},
	{SQL_DESC_UNNAMED,				UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE,	READ_DESC_TYPE,		RW_DESC_TYPE},
	{SQL_DESC_UNSIGNED,				UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE,	READ_DESC_TYPE,		READ_DESC_TYPE},
	{SQL_DESC_UPDATABLE,			UNUSED_DESC_TYPE,	UNUSED_DESC_TYPE,	READ_DESC_TYPE,		UNUSED_DESC_TYPE},
	{9999}
};
//	Global Handle Initialization

struct sigaction gOldSigTermActionHandler;

extern "C" void ODBC_SIGTERM_handler(int signal)
{
	struct sigaction newActionHandler;

	CStmt *pStmt;
	std::list<CHandlePtr>::iterator i;
	CHandlePtr		pHandle;
	CHANDLECOLLECT handleCollection = 	gDrvrGlobal.gHandle.getHandles();

	for(i = handleCollection.begin(); i !=  handleCollection.end() ; ++i)
	{
		if ((pHandle = (CHandlePtr)(*i)) != NULL)		
		{
			if (pHandle->getHandleType() == SQL_HANDLE_STMT)
				SQLCancel((SQLHSTMT)pHandle);
		}

	} // for all handles

   sigaction(SIGTERM,&gOldSigTermActionHandler,NULL);
   raise(SIGTERM);
} // ODBC_SIGTERM_handler()

CDrvrGlobal::CDrvrGlobal()// : gOdbcMsg(DRVRMSG_DLL)
{
   char *envDisablePreFetch = getenv("HPODBC_DISABLE_PREFETCH");

   if(envDisablePreFetch != NULL && (strcmp(envDisablePreFetch,"1") == 0))
      gDisablePreFetch = true;
   else
      gDisablePreFetch = false;

#ifdef TRACE_COMPRESSION
	char *envTraceCompression=NULL;
	gTraceCompression=false;
	if( (envTraceCompression = getenv("TRAFODBC_TRACE_COMPRESSION")) != NULL)
	{
		if(strcmp(envTraceCompression,"1")==0)
		{
			gTraceCompression=true;
		}
	}
#endif


	InitializeCriticalSection(&gCSObject);
	InitializeCriticalSection(&gHandleCSObject);

	struct sigaction SigTermActionHandler;
    sigaction(SIGTERM,NULL,&SigTermActionHandler);

	if(SigTermActionHandler.sa_handler == SIG_DFL)
	{
		/* 
		   If the application has not arm'ed a handler for SIGTERM, then
		   we'll setup a trap handler for that so that we can call SQLCancel
		   on all the queries that are still running. Otherwise
		   it is the application's responsibility to call SQLCancel() from
		   their SIGTERM handler
		*/
		SigTermActionHandler.sa_handler = ODBC_SIGTERM_handler;
	    sigaction(SIGTERM,&SigTermActionHandler,&gOldSigTermActionHandler);
	}
	strcpy(gCapsuleName, "TRAF ODBC Driver");
	strcpy(gProcName, "$ZTCP0");

#ifdef MXAIX
	// AIX behaves a bit differently. On other unix systems when libhpodbc.so exports SQLGetPrivateProfileString,
	// a call to dlsym can get the address of the function. On AIX it does not unless we do an explicit dlopen
	void *phandle = dlopen(0,RTLD_LAZY|RTLD_GLOBAL);
#endif

	// for some unknown reason fpSQLGetPrivateProfileString = (FPSQLGetPrivateProfileString) SQLGetPrivateProfileString
	// does not work for the Informatica/DataDirect/libhpodbc_drvr.so combination
	// (perhaps its because this is an unresolved reference - but it does not explain the fact that it works from
	// a stand alone application like our connect_test.cpp/DataDirect/libhpodbc_drvr combintation

	// fpSQLGetPrivateProfileString = (FPSQLGetPrivateProfileString) SQLGetPrivateProfileString;

#if defined(MXHPUX) && !defined(MXOSS) || defined(MXSUNSPARC)
   fpSQLGetPrivateProfileString = (FPSQLGetPrivateProfileString) dlsym(RTLD_SELF,"SQLGetPrivateProfileString");
#elif defined(MXOSS)
    // OSS did not implement psedo handle so we must use
    dlHandle dlhdl_search = dlopen(0,RTLD_LAZY|RTLD_GLOBAL);
      if(dlhdl_search != NULL)
          fpSQLGetPrivateProfileString = (FPSQLGetPrivateProfileString) dlsym(dlhdl_search,"SQLGetPrivateProfileString");
#else
   fpSQLGetPrivateProfileString = (FPSQLGetPrivateProfileString) dlsym(RTLD_DEFAULT,"SQLGetPrivateProfileString");
#endif
#ifndef MXOSS
   if(fpSQLGetPrivateProfileString == NULL)
   {
	  #if defined(MXHPUXPA)
	  dlhdl_odbcinst = dlopen("libodbcinst.sl",RTLD_LAZY|RTLD_GLOBAL);
	  #else
	  dlhdl_odbcinst = dlopen("libodbcinst.so",RTLD_LAZY|RTLD_GLOBAL);
	  #endif

      if(dlhdl_odbcinst != NULL)
         fpSQLGetPrivateProfileString = (FPSQLGetPrivateProfileString) dlsym(RTLD_DEFAULT,"SQLGetPrivateProfileString");
   }
#endif
#ifdef MXOSS
    dlclose(dlhdl_search);
#endif
#ifdef MXAIX
	dlclose(phandle);
#endif


	if (gClientVersion.componentId == 0)
	{
#ifdef MXHPUX
		gClientVersion.componentId = HPUX_DRVR_COMPONENT; 
#elif MXSUNSPARC
	#ifdef __LP64__
		gClientVersion.componentId = SUNSPARC64_DRVR_COMPONENT;
	#else
		gClientVersion.componentId = SUNSPARC32_DRVR_COMPONENT;
	#endif
#elif MXAIX
		gClientVersion.componentId = AIX_DRVR_COMPONENT;
#elif unixcli
		gClientVersion.componentId = LINUX_DRVR_COMPONENT; 
#else
		gClientVersion.componentId = DRVR_COMPONENT;
#endif

		gClientVersion.majorVersion = 3;
		gClientVersion.minorVersion = 0;
		gClientVersion.buildId = 0;
		strcpy(gClientVproc, versionString);
		//strcpy(gClientVproc,"T0775N29_SQ");
	}
//	gCEEInitialized = FALSE;
//	gCEESyncEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
//	gCEEExecuteThread = NULL;

	gDefaultTxnIsolation = SQL_TXN_READ_COMMITTED;
//	gWaitCursor = LoadCursor(NULL, IDC_WAIT); 

#ifdef OSS_PTHREAD
#ifdef OSS_DRIVER
	gMutex.field1 = (void*)-1;
	gMutex.field2 = -1;
	gMutex.field3 = -1;
	pthread_mutex_init(&gMutex, NULL);
#endif
#endif

	gSpecial_1 = false;  //  default is false. CQD
	                        // has to be set to be in special mode
#ifndef MXOSS
	SecurityInitialize();
#endif
}

CDrvrGlobal::~CDrvrGlobal()
{
	CloseHandle(gCEESyncEvent);
	DeleteCriticalSection(&gCSObject);
	DeleteCriticalSection(&gHandleCSObject);

#ifdef OSS_PTHREAD
#ifdef OSS_DRIVER
	pthread_mutex_destroy(&gMutex);
#endif
#endif
#if defined(ASYNCIO) && defined(DBSELECT)
	if(gdbSelect != NULL)
		delete gdbSelect;
	gdbSelect = NULL;
#endif
}

SQLRETURN returnAttrValue(BOOL reportError, 
						  CHandle *pHandle,
						  RETURN_VALUE_STRUCT *retValues,
						  SQLPOINTER ValuePtr,
						  SQLINTEGER BufferLength,
						  SQLINTEGER *StringLengthPtr)
{
	SQLINTEGER strLen = DRVR_PENDING;
	SQLRETURN rc = SQL_SUCCESS;
	SQLINTEGER	translateLength;
	SQLINTEGER	translateLengthMax;
	UCHAR		errorMsg[MAX_TRANSLATE_ERROR_MSG_LEN];
	ICUConverter* iconv;
	
	
	if (pHandle == NULL ) // and check whether a connection has happened. 
		iconv = &gDrvrGlobal.ICUConv;
	else
		iconv = pHandle->m_ICUConv;
	
	errorMsg[0] = '\0';
	if (ValuePtr != NULL)
	{
		switch (retValues->dataType)
		{
		case SQL_C_SBIGINT:
			*(IDL_long_long*)ValuePtr = retValues->u.s64Value;
			strLen = sizeof(IDL_long_long);
			break;
		case SQL_C_UBIGINT:
			*(IDL_unsigned_long_long*)ValuePtr = retValues->u.u64Value;
			strLen = sizeof(IDL_unsigned_long_long);
			break;
		case SQL_IS_POINTER:
			*(SQLPOINTER *)ValuePtr = retValues->u.pValue;
			strLen = sizeof(SQLPOINTER);
			break;
		case SQL_IS_INTEGER:
			*(SQLINTEGER *)ValuePtr = retValues->u.s32Value;
			strLen = sizeof(SQLINTEGER);
			break;
		case SQL_IS_UINTEGER:
			*(SQLUINTEGER *)ValuePtr = retValues->u.u32Value;
			strLen = sizeof(SQLUINTEGER);
			break;
		case SQL_IS_SMALLINT:
			*(SQLSMALLINT *)ValuePtr = retValues->u.s16Value;
			strLen = sizeof(SQLSMALLINT);
			break;
		case SQL_IS_USMALLINT:
			*(SQLUSMALLINT *)ValuePtr = retValues->u.u16Value;
			strLen = sizeof(SQLUSMALLINT);
			break;
		default:
			if (retValues->u.strPtr != NULL)
			{
				strLen = strlen(retValues->u.strPtr);
				translateLengthMax = (BufferLength == SQL_NTS) ? strLen : BufferLength;

				if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
						TraceOut(TR_ODBC_ERROR, "UTF8ToWChar retValues->u.strPtr \"%s\", strLen %d ",
																					retValues->u.strPtr, strLen);
																					
				rc = iconv->OutArgTranslationHelper((SQLCHAR*)retValues->u.strPtr, strLen, (char*)ValuePtr, translateLengthMax,
													 (int *)&translateLength, NULL, (char*)errorMsg);
				// For ALM CR 5228. The returned length of the attribute value should be counted in bytes for buffer size as ODBC
				// standard specification explains. Thus, translateLength should be used to guarantee the length is correctly
				// returned not matter the application type is ANSI or UNICODE.
				strLen = translateLength;

				if(rc != SQL_SUCCESS)
				{
					if (errorMsg && errorMsg[0] != '\0')
					{
						if (reportError)
							pHandle->setDiagRec(DRIVER_ERROR, IDS_186_DSTODRV_TRUNC, 0, (char *)errorMsg);
					}
					else
					{
						if (reportError)
							pHandle->setDiagRec(DRIVER_ERROR, IDS_186_DSTODRV_TRUNC);
					}
				}
			}
			else
				iconv->NullTerminate((char*) ValuePtr);
			break;
		}
	}
	else
	{
		switch (retValues->dataType)
		{
		case SQL_IS_POINTER:
			strLen = sizeof(SQLPOINTER);
			break;
		case SQL_IS_INTEGER:
			strLen = sizeof(SQLINTEGER);
			break;
		case SQL_IS_UINTEGER:
			strLen = sizeof(SQLUINTEGER);
			break;
		case SQL_IS_SMALLINT:
			strLen = sizeof(SQLSMALLINT);
			break;
		case SQL_IS_USMALLINT:
			strLen = sizeof(SQLUSMALLINT);
			break;
		default:
			if (retValues->u.strPtr != NULL)
				strLen = gDrvrGlobal.ICUConv.FindStrLength((const char*)retValues->u.strPtr, BufferLength);
			break;
		}
	}
	if (strLen != DRVR_PENDING)
		if (StringLengthPtr != NULL)
			*StringLengthPtr = strLen;
	return rc;

}

void ScanConnectString(char *InConnectionString, SQLSMALLINT StringLength1, CONNECT_KEYWORD_TREE *KeywordTree)
{
	short		i;		
	short		iStartPos;
	short		KeyAttrNo;
	short		AttrRank;
	char		delimiter;
	BOOL		found;

	for (i = 0 ; i <= KEY_MAX ; i++)
	{
		KeywordTree[i].AttrValue = NULL;
		KeywordTree[i].AttrLength = 0;
		KeywordTree[i].AttrRank = 0;
	}
		
	AttrRank = KEY_MAX;
	for (i =0; i < StringLength1 ; i++ )
	{
		// skip leading noise characters; if any
		for ( ; i < StringLength1 && ( InConnectionString[i] == ';' || InConnectionString[i] == ' ' ); i++ );
		if( i>=StringLength1) break;
		// get the key name; is terminated by an '='
		for( iStartPos = i; i < StringLength1 && InConnectionString[i] != '=' ; i++ );
		if( i>=StringLength1) break;
		//
		for (KeyAttrNo = 0, found = FALSE; ConnectKeywords[KeyAttrNo] != NULL ; KeyAttrNo++)
		{
			if (strnicmp(ConnectKeywords[KeyAttrNo], (const char *)(InConnectionString + iStartPos), (i-iStartPos)) == 0)
			{
				found = TRUE;
				break;
			}
		}
		delimiter = ';';
		if (found)
		{
			if (InConnectionString[i+1] == '{')
				delimiter = '}';
		}
		else
			KeyAttrNo = KEY_MAX;
		if( delimiter != '}' )
			for (iStartPos = i ; i < StringLength1 && InConnectionString[i] != delimiter ; i++ );
		else if (KeyAttrNo == KEY_PWD || KeyAttrNo == KEY_SCHEMA || KeyAttrNo == KEY_UID) {
			for (iStartPos = i ; i < StringLength1; i++ )
				if(InConnectionString[i] == delimiter && InConnectionString[i+1] == ';') break;	
		}
		else {
			int level = 0;
			for (iStartPos = i ; i < StringLength1; i++ ){
				 if(InConnectionString[i] == delimiter && --level==0) break;
				 if(InConnectionString[i] == '{') level++;
			}
		}
		KeywordTree[KeyAttrNo].AttrValue = (char *)(InConnectionString + iStartPos+1); //+1 Skip '='
		if( delimiter != '}' )
			KeywordTree[KeyAttrNo].AttrLength = i-iStartPos-1;	//-1 skip the delimiter
		else
			KeywordTree[KeyAttrNo].AttrLength = i-iStartPos;	//do not skip the delimiter
		KeywordTree[KeyAttrNo].AttrRank = AttrRank--;
	} 
	return;
}	

unsigned long getCDefault(SQLSMALLINT SQLDataType, SQLINTEGER ODBCAppVersion, SQLINTEGER SQLCharSet, SQLSMALLINT &DataType)
{
	short	i;
	BOOL	found = FALSE;

	if(gDrvrGlobal.ICUConv.m_AppUnicodeType == APP_UNICODE_TYPE_UTF16)
	{
		switch (SQLDataType)
		{
			case SQL_CHAR:
			case SQL_VARCHAR:
			case SQL_LONGVARCHAR:
			case SQL_WCHAR:
			case SQL_WVARCHAR:
			case SQL_WLONGVARCHAR:
				DataType = SQL_C_WCHAR;
				return SQL_SUCCESS;
			default:
				goto mapdef;
		}
	}
	else
	{
		switch (SQLDataType)
		{
			case SQL_WCHAR:
			case SQL_WVARCHAR:
			case SQL_WLONGVARCHAR:
				DataType = SQL_C_CHAR;
				return SQL_SUCCESS;
			default:
				goto mapdef;
		}
	}

mapdef:
	switch (SQLDataType)
	{
		case SQL_BIGINT:
			if (ODBCAppVersion >= SQL_OV_ODBC3)
				DataType = SQL_C_SBIGINT;
			else
				DataType = SQL_C_CHAR;
			break;
		default:
			for (i = 0, found = FALSE; gSQLDatatypeMap[i].conciseType != SQL_DEFAULT ; i++)
			{
				// Compare with SQL data type given in SQLBindParameter
				if (gSQLDatatypeMap[i].conciseType == SQLDataType) 
				{
					found = TRUE;
					DataType = gSQLDatatypeMap[i].defaultType;
					break;
				}
			}
			if (! found)
				return IDS_HY_003;
			break;
	}

	return SQL_SUCCESS;
}

unsigned long getOctetLength(SQLSMALLINT	SQLDataType, 
							 SQLINTEGER		ODBCAppVersion, 
							 SQLPOINTER		DataPtr,
							 SQLINTEGER		StrLen, 
							 SQLSMALLINT	&DataType,
							 SQLINTEGER		&OctetLength)
{

	unsigned long	retCode;
	short			i;
	BOOL			found;

	switch (DataType)
	{
	case SQL_C_DEFAULT:
		// Get the corresponding C DataType and fall thru 
		// No Break;
		if ((retCode = getCDefault(SQLDataType, ODBCAppVersion, SQLCHARSETCODE_UNKNOWN, DataType)) != SQL_SUCCESS)
			return retCode;
		// Note No Break
	case SQL_C_CHAR:
	case SQL_C_BINARY:
		if (StrLen == SQL_NTS)
			OctetLength = strlen((const char *)DataPtr);
		else
			OctetLength = StrLen;
		break;
	default:
		for (i = 0, found = FALSE; gCDatatypeMap[i].conciseType != SQL_C_DEFAULT ; i++)
		{
			if (gCDatatypeMap[i].conciseType == DataType)
			{
				found = TRUE;
				OctetLength = gCDatatypeMap[i].octetLength;
				break;
			}
		}
		if (! found)
			return IDS_HY_003;
	}
	return SQL_SUCCESS;
}

char *rTrim(char *string)                     
{                                             
   char *strPtr;                              
                                              
   for (strPtr = string + strlen(string) - 1; 
        strPtr >= string && (*strPtr == ' ' || *strPtr == '\t') ;  
        *(strPtr--) = '\0');                  
   return(string);                            
} 

char* wmstrim(char *string)
{
	char *tmp;
	char *p;                              
                                              
	for (p = string + strlen(string) - 1; p >= string && isspace(*p); *(p--) = '\0');
		
	tmp = (char*)malloc( strlen(string) + 1);
	if (tmp == NULL ) return string;
	strcpy(tmp,string);

	for (p = tmp; *p != 0 && isspace(*p); p++);

	strcpy( string, p);
	free( tmp);
	return string;
}

char* trimInterval(char *string)
{
	char sep[] = " ";
	char *token;
	char *assembledStr;
	char *saveptr=NULL;

	assembledStr = (char*)malloc( strlen(string) + 1);
	if (assembledStr == NULL ) return string;
	assembledStr[0]=0;
#ifndef unixcli
	token = strtok( string, sep );
#else
	token = strtok_r( string, sep, &saveptr );
#endif
	if (strcmp(token, "-") == 0) {	
	  strcat(assembledStr, token);
#ifndef unixcli
	  token = strtok( NULL, sep );
#else
	  token = strtok_r( NULL, sep, &saveptr );
#endif
	}
	while( token != NULL )   {
	  strcat(assembledStr, token); 
#ifndef unixcli
	  token = strtok( NULL, sep );
#else
	  token = strtok_r( NULL, sep, &saveptr );
#endif
	  if(token != NULL)
		strcat(assembledStr, sep);
	}
	strcpy( string, assembledStr);
	free( assembledStr);
	return string;
}

char* trimInterval(char *string, SQLINTEGER length)
{
	char sep[] = " ";
	char *token;
	char *tmpStr, *assembledStr;
	char *saveptr=NULL;
	
	// We can use *string, as '\0' will overwrite the null indicator for the next field 
	// So make a copy of it.
	tmpStr = (char*)malloc(length+1);
	memset(tmpStr,'\0',length+1);
	strncpy(tmpStr, string, length);
	
	assembledStr = (char*)malloc(length + 1);
	if (assembledStr == NULL ) return string;
	assembledStr[0]=0;
	
#ifndef unixcli
	  token = strtok( tmpStr, sep );
#else
	  token = strtok_r( tmpStr, sep, &saveptr );
#endif

	if (strcmp(token, "-") == 0) {	
	  strcat(assembledStr, token);
#ifndef unixcli
	  token = strtok( NULL, sep );
#else
	  token = strtok_r( NULL, sep, &saveptr );
#endif
	}
	while( token != NULL )   {
	  strcat(assembledStr, token); 
#ifndef unixcli
	  token = strtok( NULL, sep );
#else
	  token = strtok_r( NULL, sep, &saveptr );
#endif
	  if(token != NULL)
		strcat(assembledStr, sep);
	}
	strcpy(string, assembledStr);

	free(assembledStr);
	free(tmpStr);

	return string;
}

 
char* rSup( char* string )
{
	if (*string == 0 ) return string;
	if (strchr(string, '.') == NULL) return string;

	for(int i=strlen(string)-1; i >= 0; i--)
	{
		if (string[i] == '0') {
			if ( i - 1 >= 0 && string[i-1] == '.') break;
			else {
				string[i]=0;
				continue;
			}
		}
		break;
	}
	return string;
}

/* double_to_char:
*  number: the number to be converted to char string.
*  precision: the number of digits after the decimal point(0 <= precision <= DBL_DIG).
*  string: the buffer to receive a null terminated string result on success.
*  size: the buffer size of string.
*
*  return: true on succeeded, false on failed.
*/
bool double_to_char (double number, int precision, char* string, short size)
{
    char format[8] = { '\0' };
    size_t actualLen = 0;

    // make sure any precision of possible double value can be format to the buf.
    char buf[MAX_DOUBLE_TO_CHAR_LEN] = { '\0' };

    // precission should be limit to a reasonable range.
    if ((precision < 0) || (precision > DBL_DIG))
        return false;

    if ((sprintf(format, "%%.%dlg", (precision > FLT_DIG) ? (precision + 2) : (precision + 3)) < 0) ||
        ((actualLen = sprintf(buf, format, number)) < 0) ||
        (actualLen > size)) {
        return false;
    }

    strcpy(string, buf);
    return true;
}

bool ctoi64(char* string, __int64& out, bool* truncation)
{
	int len;
	out = 0;
	char* buff;
	double dTmp;
	char *errorCharPtr = NULL;
	*truncation = false; // initialize anyway

	buff = (char*)malloc(100 + 1);
	if (buff == NULL ) return false;
	memset(buff,0,100 + 1);
	strncpy( buff, string, 100);

	trim(buff);
	len = strlen(buff);
	if (len != (int)strspn(buff, "+-1234567890"))
	{
		if (len != (int)strspn(buff, "+-1234567890.eE"))
		{
			free (buff);
			return false;
		}
		else
		{
			dTmp = strtod(buff, &errorCharPtr);
			if (errno == ERANGE || (strlen(errorCharPtr) > 0))
			{
				free (buff);
				return false; 
			}
			sprintf(buff,"%lf",dTmp);
			rSup( buff );
			char* cptr = strchr(buff,'.');
			if (cptr != NULL) // data truncation: JoyJ
			{   
		           *cptr = 0;
			   if ((++cptr != NULL) && (_atoi64(cptr) > 0)) //No truncation if '0's after '.' are truncated. 
				*truncation = true; 
			}
			len = strlen(buff);
		}
	}

	if (buff[0] == '+')
	{
		if (len - 1 > 19 )
		{
			free (buff);
			return false;
		}
		if (len - 1 == 19)
		{
			if (strcmp( buff, "+9223372036854775807") > 0 )
			{
				free (buff);
				return false;
			}
		}
	} 
	else if (buff[0] == '-')
	{
		if (len - 1 > 19 )
		{
			free (buff);
			return false;
		}
		if (len - 1 == 19)
		{
			if (strcmp( buff, "-9223372036854775808") > 0 )
			{
				free (buff);
				return false;
			}
		}
	}
	else
	{
	//now there should't be any "+-.eE" in "buff"(other than the first byte): JoyJ
	    if ((len-1) != (int)strspn(buff+1, "1234567890"))
			return false; 
		if (len > 19 )
		{
			free (buff);
			return false;
		}
		if (len == 19)
		{
			if (strcmp( buff, "9223372036854775807") > 0 )
			{
				free (buff);
				return false;
			}
		}
	}

	out = _atoi64(buff);
	free (buff);
	return true;
}

bool ctoi64(char* string, __int64& out )
{
	int len;
	out = 0;
	char* buff;
	double dTmp;
	char *errorCharPtr;

	buff = (char*)malloc(100 + 1);
	if (buff == NULL ) return false;
	memset(buff,0,100 + 1);
	strncpy( buff, string, 100);

	trim(buff);
	len = strlen(buff);
	if (len != (int)strspn(buff, "+-1234567890"))
	{
		if (len != (int)strspn(buff, "+-1234567890.eE"))
		{
			free (buff);
			return false;
		}
		else
		{
			dTmp = strtod(buff, &errorCharPtr);
			if (errno == ERANGE)
			{
				free (buff);
				return false;
			}
			sprintf(buff,"%lf",dTmp);
			rSup( buff );
			char* cptr = strchr(buff,'.');
			if (cptr != NULL) *cptr = 0;
			len = strlen(buff);
		}
	}

	if (buff[0] == '+')
	{
		if (len - 1 > 19 )
		{
			free (buff);
			return false;
		}
		if (len - 1 == 19)
		{
			if (strcmp( buff, "+9223372036854775807") > 0 )
			{
				free (buff);
				return false;
			}
		}
	} 
	else if (buff[0] == '-')
	{
		if (len - 1 > 19 )
		{
			free (buff);
			return false;
		}
		if (len - 1 == 19)
		{
			if (strcmp( buff, "-9223372036854775808") > 0 )
			{
				free (buff);
				return false;
			}
		}
	}
	else
	{
		if (len > 19 )
		{
			free (buff);
			return false;
		}
		if (len == 19)
		{
			if (strcmp( buff, "9223372036854775807") > 0 )
			{
				free (buff);
				return false;
			}
		}
	}

	out = _atoi64(buff);
	free (buff);
	return true;
}

BOOL checkDatetimeValue(short *datetime_parts)
{
	short DaysArray[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	
	if (datetime_parts[1] < 1 || datetime_parts[1] > 12)
		return FALSE;
	if (datetime_parts[3] < 0 || datetime_parts[3] > 23)
		return FALSE;
	if (datetime_parts[4] < 0 || datetime_parts[4] > 59)
		return FALSE;
	if (datetime_parts[5] < 0 || datetime_parts[5] > 59)
		return FALSE;
	if (datetime_parts[1] == 2)
	{
		if ((datetime_parts[0] % 4 == 0) && ((datetime_parts[0] % 100 != 0) || (datetime_parts[0] % 400 == 0)))
			DaysArray[1] = 29;
		else
			DaysArray[1] = 28;
	}
	if (datetime_parts[2] < 1 || datetime_parts[2] > DaysArray[datetime_parts[1]-1])
			return FALSE;
	return TRUE;
}

__int64 pow(short base, short scale)
{
	__int64 retValue = 1;

	scale = scale > 18?18:scale;

	for(int i = 0; i < scale; i++)
		retValue *= 10;
	return retValue;
}


 
