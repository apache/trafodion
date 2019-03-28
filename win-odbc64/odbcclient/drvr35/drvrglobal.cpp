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

#include "DrvrGlobal.h"
#include "tdm_odbcDrvMsg.h"
#include "DiagFunctions.h" 
#include <errno.h>
#include "StaticLocking.h"
#include "DrvrSrvr.h"

// Declare the global variable

typedef unsigned long*  (SQL_API *FPGetODBCSharedData) ();
extern FPGetODBCSharedData pGetODBCSharedData = NULL;

CRITICAL_SECTION gCollectionCSObject;
CDrvrGlobal	gDrvrGlobal;
DWORD	*pdwGlobalTraceVariable = NULL;
DWORD	gTraceFlags = 0;
HMODULE g_hOdbcDM = NULL;
HMODULE	g_hTraceDLL	= NULL;

DWORD gTlsIndex_ErrorBuffer = TLS_OUT_OF_INDEXES;

DATATYPE_TABLE gSQLDatatypeMap[] = 
{
//   conciseType,					verboseType,		datetimeIntervalCode,		columnSizeAttr,		decimalDigitsAttr,	displaySizeAttr,	octetLength,					defaultType,					typeName		
    {TYPE_BLOB,                     SQL_CHAR,           0,                          SQL_DESC_LENGTH,    0,                  SQL_DESC_LENGTH,    SQL_DESC_LENGTH,                SQL_C_CHAR,                         "BLOB" },
    {TYPE_CLOB,                     SQL_CHAR,           0,                          SQL_DESC_LENGTH,    0,                  SQL_DESC_LENGTH,    SQL_DESC_LENGTH,                SQL_C_CHAR,                         "CLOB" },
    {SQL_CHAR,						SQL_CHAR,			0,							SQL_DESC_LENGTH,	0,					SQL_DESC_LENGTH,	SQL_DESC_LENGTH,				SQL_C_CHAR,						"CHAR"},
	{SQL_VARCHAR,					SQL_VARCHAR,		0,							SQL_DESC_LENGTH,	0,					SQL_DESC_LENGTH,	SQL_DESC_LENGTH,				SQL_C_CHAR,						"VARCHAR"},
	{SQL_LONGVARCHAR,				SQL_LONGVARCHAR,	0,							SQL_DESC_LENGTH,	0,					SQL_DESC_LENGTH,	SQL_DESC_LENGTH,				SQL_C_CHAR,						"LONG VARCHAR"},
	{SQL_DECIMAL,					SQL_DECIMAL,		0,							SQL_DESC_PRECISION,	SQL_DESC_SCALE,		SQL_DESC_PRECISION, SQL_DESC_PRECISION,				SQL_C_CHAR,						"DECIMAL"},
	{SQL_NUMERIC,					SQL_NUMERIC,		0,							SQL_DESC_PRECISION,	SQL_DESC_SCALE,		SQL_DESC_PRECISION, SQL_DESC_PRECISION,				SQL_C_CHAR,					"NUMERIC"},
	{SQL_SMALLINT,					SQL_SMALLINT,		0,							SQL_DESC_PRECISION,	SQL_DESC_SCALE,		6,					sizeof(SHORT),					SQL_C_SHORT,					"SMALLINT"},
	{SQL_INTEGER,					SQL_INTEGER,		0,							SQL_DESC_PRECISION,	SQL_DESC_SCALE,		11,					sizeof(LONG),					SQL_C_LONG,						"INTEGER"},
	{SQL_REAL,						SQL_REAL,			0,							SQL_DESC_PRECISION,	0,					13,					sizeof(FLOAT),					SQL_C_FLOAT,					"REAL"},
	{SQL_FLOAT,						SQL_FLOAT,			0,							SQL_DESC_PRECISION,	0,					22,					sizeof(DOUBLE),					SQL_C_DOUBLE,					"FLOAT"},
	{SQL_DOUBLE,					SQL_DOUBLE,			0,							SQL_DESC_PRECISION,	0,					22,					sizeof(DOUBLE),					SQL_C_DOUBLE,					"DOUBLE PRECISION"},
	{SQL_BIT,						SQL_BIT,			0,							SQL_DESC_LENGTH,	0,					1,					sizeof(SCHAR),					SQL_C_BIT,						"BIT"},
	{SQL_TINYINT,					SQL_TINYINT,		0,							SQL_DESC_PRECISION,	SQL_DESC_SCALE,		4,					sizeof(CHAR),					SQL_C_TINYINT,					"TINYINT"},
	{SQL_BIGINT,					SQL_BIGINT,			0,							SQL_DESC_PRECISION,	SQL_DESC_SCALE,		20,					20,								SQL_C_SBIGINT,					"BIGINT"},
	{SQL_BINARY,					SQL_BINARY,			0,							SQL_DESC_LENGTH,	0,					-1,					SQL_DESC_LENGTH,				SQL_C_BINARY,					"BINARY"},
	{SQL_VARBINARY,					SQL_VARBINARY,		0,							SQL_DESC_LENGTH,	0,					-1,					SQL_DESC_LENGTH,				SQL_C_BINARY,					"VARBINARY"},
	{SQL_LONGVARBINARY,				SQL_LONGVARBINARY,	0,							SQL_DESC_LENGTH,	0,					-1,					SQL_DESC_LENGTH,				SQL_C_BINARY,					"LONG VARBINARY"},
	{SQL_WCHAR,						SQL_WCHAR,			0,							SQL_DESC_LENGTH,	0,					SQL_DESC_LENGTH,	SQL_DESC_LENGTH,				SQL_C_WCHAR,					"NCHAR"},
	{SQL_WVARCHAR,					SQL_WVARCHAR,		0,							SQL_DESC_LENGTH,	0,					SQL_DESC_LENGTH,	SQL_DESC_LENGTH,				SQL_C_WCHAR,					"NCHAR VARYING"},
	{SQL_WLONGVARCHAR,				SQL_WLONGVARCHAR,	0,							SQL_DESC_LENGTH,	0,					SQL_DESC_LENGTH,	SQL_DESC_LENGTH,				SQL_C_WCHAR,					"NCHAR VARYING"},
	{SQL_DATE,						SQL_DATETIME,		SQL_CODE_DATE,				SQL_DESC_LENGTH,	SQL_DESC_PRECISION, SQL_DESC_LENGTH,	sizeof(SQL_DATE_STRUCT),		SQL_C_TYPE_DATE,				"DATE"},
	{SQL_TIME,						SQL_DATETIME,		SQL_CODE_TIME,				SQL_DESC_LENGTH,	SQL_DESC_PRECISION,	SQL_DESC_LENGTH,	sizeof(SQL_TIME_STRUCT),		SQL_C_TYPE_TIME,				"TIME"},
	{SQL_TIMESTAMP,					SQL_DATETIME,		SQL_CODE_TIMESTAMP,			SQL_DESC_LENGTH,	SQL_DESC_PRECISION,	SQL_DESC_LENGTH,	sizeof(SQL_TIMESTAMP_STRUCT),	SQL_C_TYPE_TIMESTAMP,			"TIMESTAMP"},
	{SQL_TYPE_DATE,					SQL_DATETIME,		SQL_CODE_DATE,				SQL_DESC_LENGTH,	SQL_DESC_PRECISION, SQL_DESC_LENGTH,	sizeof(SQL_DATE_STRUCT),		SQL_C_TYPE_DATE,				"DATE"},
	{SQL_TYPE_TIME,					SQL_DATETIME,		SQL_CODE_TIME,				SQL_DESC_LENGTH,	SQL_DESC_PRECISION,	SQL_DESC_LENGTH,	sizeof(SQL_TIME_STRUCT),		SQL_C_TYPE_TIME,				"TIME"},
	{SQL_TYPE_TIMESTAMP,			SQL_DATETIME,		SQL_CODE_TIMESTAMP,			SQL_DESC_LENGTH,	SQL_DESC_PRECISION,	SQL_DESC_LENGTH,	sizeof(SQL_TIMESTAMP_STRUCT),	SQL_C_TYPE_TIMESTAMP,			"TIMESTAMP"},
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
	{SQL_C_CHAR,						SQL_C_CHAR,			0,							0, 0, 0,	SQL_DESC_LENGTH,			SQL_C_CHAR,			"CHAR"},
	{SQL_C_SHORT,						SQL_C_SHORT,		0,							0, 0, 0,	sizeof(short),				SQL_C_SHORT,		"SMALLINT"},
	{SQL_C_SSHORT,						SQL_C_SSHORT,		0,							0, 0, 0,	sizeof(short),				SQL_C_SSHORT,		"SMALLINT"},	
	{SQL_C_USHORT,						SQL_C_USHORT,		0,							0, 0, 0,	sizeof(unsigned short),		SQL_C_USHORT,		"SMALLINT"},
	{SQL_C_LONG,						SQL_C_LONG,			0,							0, 0, 0,	sizeof(long),				SQL_C_LONG,			"INTEGER"},
	{SQL_C_SLONG,						SQL_C_SLONG,		0,							0, 0, 0,	sizeof(long),				SQL_C_SLONG,		"INTEGER"},
	{SQL_C_ULONG,						SQL_C_ULONG,		0,							0, 0, 0,	sizeof(unsigned long),		SQL_C_ULONG,		"INTEGER"},
	{SQL_C_FLOAT,						SQL_C_FLOAT,		0,							0, 0, 0,	sizeof(float),				SQL_C_FLOAT,		"FLOAT"},
	{SQL_C_DOUBLE,						SQL_C_DOUBLE,		0,							0, 0, 0,	sizeof(double),				SQL_C_DOUBLE,		"DOUBLE"},
	{SQL_C_BIT,							SQL_C_BIT,			0,							0, 0, 0,	sizeof(unsigned char),		SQL_C_BIT,			"BIT"},
	{SQL_C_TINYINT,						SQL_C_TINYINT,		0,							0, 0, 0,	sizeof(signed char),		SQL_C_TINYINT,		"TINYINT"},	
	{SQL_C_STINYINT,					SQL_C_STINYINT,		0,							0, 0, 0,	sizeof(signed char),		SQL_C_STINYINT,		"TINYINT"},
	{SQL_C_UTINYINT,					SQL_C_UTINYINT,		0,							0, 0, 0,	sizeof(unsigned char),		SQL_C_UTINYINT,		"TINYINT"},
	{SQL_C_SBIGINT,						SQL_C_SBIGINT,		0,							0, 0, 0,	sizeof(_int64),				SQL_C_SBIGINT,		"BIGINT"},
	{SQL_C_UBIGINT,						SQL_C_UBIGINT,		0,							0, 0, 0,	sizeof(unsigned _int64),	SQL_C_UBIGINT,		"BIGINT"},
	{SQL_C_BINARY,						SQL_C_BINARY,		0,							0, 0, 0,	SQL_DESC_LENGTH,			SQL_C_BINARY,		"BINARY"},
	{SQL_C_BOOKMARK,					SQL_C_BOOKMARK,		0,							0, 0, 0,	sizeof(unsigned long),		SQL_C_BOOKMARK,		"BOOKMARK"},
	{SQL_C_VARBOOKMARK,					SQL_C_VARBOOKMARK,	0,							0, 0, 0,	SQL_DESC_LENGTH,			SQL_C_VARBOOKMARK,	"VARBOOKMARK"},
	{SQL_C_NUMERIC,						SQL_C_NUMERIC,		0,							0, 0, 0,	sizeof(SQL_NUMERIC_STRUCT),	SQL_C_NUMERIC,		"NUMERIC"},
	{SQL_C_WCHAR,						SQL_C_WCHAR,		0,							0, 0, 0,	SQL_DESC_LENGTH,			SQL_C_WCHAR,		"NCHAR"},
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
	NULL
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

CDrvrGlobal::CDrvrGlobal() : gOdbcMsg(DRVRMSG_DLL)
{
	OSVERSIONINFO		VersionInformation;
	void				*pVersionInfo;
	VS_FIXEDFILEINFO	*pInfo;
	UINT				wInfoLen;
	DWORD				dwVersionInfoSz;
	DWORD				hFile;
	unsigned long		len;

	WORD* langInfo;
    UINT cbLang;
	TCHAR tszVerStrName[128];
	LPVOID lpt;

	InitializeCriticalSection(&gCSObject);
	InitializeCriticalSection(&gHandleCSObject);
	InitializeCriticalSection(&gCollectionCSObject);
	strcpy(gCapsuleName, "TRAF ODBC Driver");
	len = sizeof(gComputerName);
	if (GetComputerName(gComputerName, &len) == 0)
		strcpy(gComputerName, "UNKNOWN");
	gProcessId = GetCurrentProcessId();

	//referer to the enum cnv_charset in csconvert.h
	//TranslateOption <4bytes from charset,4 bytes to charset>
	gUTF8_To_UTF16_TranslateOption = ((DWORD)0x00010002L);
	gUTF16_To_UTF8_TranslateOption = ((DWORD)0x00020001L);

	VersionInformation.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
	if (GetVersionEx( &VersionInformation ) == 0)
	{
		gPlatformId = 0;
		gMinorVersion = 0;
	}
	else
	{
		gPlatformId = VersionInformation.dwPlatformId;
		gMinorVersion = VersionInformation.dwMinorVersion;
	}

	gClientVersion.componentId = 0; // Unknown
	strcpy(gDriverDLLName, DRIVER_DLL_NAME);
	if ((gModuleHandle = GetModuleHandle(DRIVER_DLL_NAME)) != NULL)
	{
		if (GetModuleFileName(gModuleHandle, gDriverDLLName, sizeof(gDriverDLLName)))
		{
			if ((dwVersionInfoSz = GetFileVersionInfoSize(gDriverDLLName, &hFile)) != 0)
			{
				pVersionInfo = (void *)new char[dwVersionInfoSz];
				if (GetFileVersionInfo(gDriverDLLName, hFile, dwVersionInfoSz, (LPVOID)pVersionInfo)
					&& VerQueryValue((LPVOID)pVersionInfo, "\\",(LPVOID *)&pInfo, &wInfoLen) )
				{
					gClientVersion.componentId = WIN_UNICODE_DRVR_COMPONENT;
					gClientVersion.majorVersion = HIWORD(pInfo->dwProductVersionMS);
					gClientVersion.minorVersion = LOWORD(pInfo->dwProductVersionMS);
					gClientVersion.buildId = LOWORD(pInfo->dwFileVersionLS);
					// Get the vproc in the Comments field
					strncpy(gClientVproc, VprocString, sizeof(gClientVproc)-1);
					gClientVproc[sizeof(gClientVproc)-1] = '\0';
				}
				delete[] pVersionInfo;
			}
		}
	}
	if (gClientVersion.componentId == 0)
	{
		gClientVersion.componentId = DRVR_COMPONENT; 
		gClientVersion.majorVersion = 3;
		gClientVersion.minorVersion = 51;
		gClientVersion.buildId = 0;
	}

	gCEEInitialized = FALSE;
	gCEESyncEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	gCEEExecuteThread = NULL;

	gDefaultTxnIsolation = SQL_TXN_READ_COMMITTED;
	gWaitCursor = LoadCursor(NULL, IDC_WAIT); 
	
	gCEEMessageFilter = TRUE;
	ghWindow = NULL;
    gSpecial_1 = false;

	loadTransportAndTranslateDll();
	SecurityInitialize();
	noSchemaInDSN = false; // Assume that DSN has a schema value
	char envValue[STRICT_SCHEMA_ENV_VAL_SIZE];
	size_t valueSize = STRICT_SCHEMA_ENV_VAL_SIZE ;
	size_t returnLen=0 ;
	RestrictSchema = false;
	if (getenv_s(&returnLen, envValue, valueSize, "TRAFODBC-SQLTABLES-RESTRICT-DSNSCHEMA") == 0) //success
		if(returnLen != 0)
			RestrictSchema = true;
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

}

CDrvrGlobal::~CDrvrGlobal()
{
	CloseHandle(gCEESyncEvent);
	DeleteCriticalSection(&gCSObject);
	DeleteCriticalSection(&gHandleCSObject);
	DeleteCriticalSection(&gCollectionCSObject);
	if (gTCPIPHandle != NULL && fpTCPIPExitIO != NULL)
	{
		(fpTCPIPExitIO)();
//
// keep dll in the memory as long as posible because of the memory leaks when free/load?
//		FreeLibrary(gTCPIPHandle);
	}
//	if (gTranslateLibrary != NULL)
//		FreeLibrary(gTranslateLibHandle);
}

void CDrvrGlobal::loadTransportAndTranslateDll()
{
	strcpy(gTCPIPLibrary, gDriverDLLName);
	strstr(_strlwr(gTCPIPLibrary),DRIVER_DLL_NAME)[0] = 0;

	if (gPlatformId != VER_PLATFORM_WIN32_NT)
		strcat(gTCPIPLibrary, TCPIPV4_DLL_NAME);
	else
		strcat(gTCPIPLibrary, TCPIPV6_DLL_NAME);

	gTCPIPHandle = NULL;
	gTCPIPLoadError = 0;
	if ((gTCPIPHandle = GetModuleHandle(gTCPIPLibrary)) != NULL || (gTCPIPHandle = LoadLibrary(gTCPIPLibrary)) != NULL )
	{
		fpTCPIPInitIO = (FPTCPIPInitIO)GetProcAddress( gTCPIPHandle, TCPIPINITIO_PROCNAME);
		fpTCPIPExitIO = (FPTCPIPExitIO)GetProcAddress( gTCPIPHandle, TCPIPEXITIO_PROCNAME);
		fpTCPIPOpenSession = (FPTCPIPOpenSession)GetProcAddress( gTCPIPHandle, TCPIPOPENSESSION_PROCNAME);
		fpTCPIPCloseSession = (FPTCPIPCloseSession)GetProcAddress( gTCPIPHandle, TCPIPCLOSESESSION_PROCNAME);
		fpTCPIPDoWriteRead = (FPTCPIPDoWriteRead)GetProcAddress( gTCPIPHandle, TCPIPDOWRITEREAD_PROCNAME);
		if (fpTCPIPInitIO != NULL && 
				fpTCPIPExitIO != NULL &&
					fpTCPIPOpenSession != NULL &&
						fpTCPIPCloseSession != NULL &&
							fpTCPIPDoWriteRead != NULL )
			gTCPIPLoadError = (fpTCPIPInitIO)(gModuleHandle);
		else
			gTCPIPLoadError = 990;
	}
	else
	{
		gTCPIPLoadError = GetLastError();
		fpTCPIPInitIO = NULL;
		fpTCPIPExitIO = NULL;
		fpTCPIPOpenSession = NULL;
		fpTCPIPCloseSession = NULL;
		fpTCPIPDoWriteRead = NULL;
	}
//Load Translation DLL
	//Get the absolute directory of system32 and append the dll name
	strcpy(gTranslateLibrary, gDriverDLLName);
	strstr(_strlwr(gTranslateLibrary),DRIVER_DLL_NAME)[0] = 0;
	strcat(gTranslateLibrary, TRANSLATE_DLL_NAME);
	gTranslateLibHandle = NULL;
	gTranslateLoadError = 0;
	if ((gTranslateLibHandle = GetModuleHandle(gTranslateLibrary)) != NULL || 
			(gTranslateLibHandle = LoadLibrary(gTranslateLibrary)) != NULL )
	{
		fpSQLDriverToDataSource = (FPSQLDriverToDataSource)GetProcAddress( gTranslateLibHandle, SQL_DRIVER_TO_DATASOURCE);
		fpSQLDataSourceToDriver = (FPSQLDataSourceToDriver)GetProcAddress( gTranslateLibHandle, SQL_DATASOURCE_TO_DRIVER);
		if (fpSQLDriverToDataSource != NULL && 
				fpSQLDataSourceToDriver != NULL)
						gTranslateLoadError = 890;
	}
	else
	{
		gTranslateLoadError = GetLastError();
		fpSQLDriverToDataSource = NULL;
		fpSQLDriverToDataSource = NULL;
	}
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
	SQLINTEGER	translateLength = 0;
	SQLINTEGER	translateLengthMax = 0;
	UCHAR		errorMsg[MAX_TRANSLATE_ERROR_MSG_LEN];
	
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
			{
				if (retValues->u.strPtr != NULL)
				{
					strLen = strlen(retValues->u.strPtr);
					if ((BufferLength <= 0 && BufferLength != SQL_NTS) || (BufferLength % 2 != 0))
					{
						if (reportError)
							pHandle->setDiagRec(DRIVER_ERROR, IDS_HY_090);
						rc = SQL_ERROR;
					}
					else
					{
						if(strLen > 0) // && BufferLength > 2)
						{
							translateLengthMax = (BufferLength == SQL_NTS) ? strLen : BufferLength;

							if((rc = UTF8ToWChar(retValues->u.strPtr, strLen, (wchar_t *)ValuePtr, translateLengthMax/2,
								(int *)(&translateLength), (char *)errorMsg)) != SQL_SUCCESS)
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
								strLen = translateLength * 2;
							}
							else
								strLen = translateLength * 2;
						}
						else
							*((wchar_t *)ValuePtr) = L'\0';
					}
				}
			} //default
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
				strLen = strlen(retValues->u.strPtr);
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
			if (_strnicmp(ConnectKeywords[KeyAttrNo], (const char *)(InConnectionString + iStartPos), (i-iStartPos)) == 0)
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
#if 0
		{
		char buff[1024];
		int length = KeywordTree[KeyAttrNo].AttrLength;
		strncpy(buff, KeywordTree[KeyAttrNo].AttrValue, length);
		buff[length] = '\0';
		}
#endif 
	} 
	return;
}	

unsigned long getCDefault(SQLSMALLINT SQLDataType, SQLINTEGER ODBCAppVersion, SQLSMALLINT &DataType)
{
	short	i;
	BOOL	found = FALSE;

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
		if ((retCode = getCDefault(SQLDataType, ODBCAppVersion, DataType)) != SQL_SUCCESS)
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
 
char* trim(char *string)
{
	char sep[] = " ";
	char *token;
	char *assembledStr;

	assembledStr = (char*)malloc( strlen(string) + 1);
	if (assembledStr == NULL ) return string;
	assembledStr[0]=0;

	token = strtok( string, sep );   
	while( token != NULL )   {
	  strcat(assembledStr, token); 
	  token = strtok( NULL, sep );
	  if(token != NULL)
		strcat(assembledStr, sep);
	  }
	strcpy( string, assembledStr);
	free( assembledStr);
	return string;
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

	assembledStr = (char*)malloc( strlen(string) + 1);
	if (assembledStr == NULL ) return string;
	assembledStr[0]=0;

	token = strtok( string, sep );
	if (strcmp(token, "-") == 0) {	
	  strcat(assembledStr, token);
	  token = strtok( NULL, sep );
	}
	while( token != NULL )   {
	  strcat(assembledStr, token); 
	  token = strtok( NULL, sep );
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
	
	// We can use *string, as '\0' will overwrite the null indicator for the next field 
	// So make a copy of it.
	tmpStr = (char*)malloc(length+1);
	memset(tmpStr,'\0',length+1);
	strncpy(tmpStr, string, length);
	
	assembledStr = (char*)malloc(length + 1);
	if (assembledStr == NULL ) return string;
	assembledStr[0]=0;
	
	token = strtok( tmpStr, sep );
	if (strcmp(token, "-") == 0) {	
	  strcat(assembledStr, token);
	  token = strtok( NULL, sep );
	}
	while( token != NULL )   {
	  strcat(assembledStr, token); 
	  token = strtok( NULL, sep );
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
bool double_to_char(double number, int precision, char* string, short size)
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

bool ctoi64(char* string, __int64& out)
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
			if (errno == ERANGE )
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

BOOL  WINAPI DllMain(HANDLE hInst,
					DWORD ul_reason_being_called,
					LPVOID lpReserved)
{
	int error;
	WORD wVersionRequested = MAKEWORD(2,0);
	WSADATA wsaData;
	void* lpszStr;

	switch (ul_reason_being_called) 
	{
		case DLL_PROCESS_ATTACH:
			if ((error = WSAStartup(wVersionRequested, &wsaData))!=0)
				return false;
			if ( LOBYTE( wsaData.wVersion ) != 2 || HIBYTE( wsaData.wVersion  ) != 0 )
			{
				WSACleanup();
				return false;
			}
			gTlsIndex_ErrorBuffer = TlsAlloc();
			if (gTlsIndex_ErrorBuffer == TLS_OUT_OF_INDEXES)
				return FALSE;
//			DisableThreadLibraryCalls((HINSTANCE)hInst);
			break;
    	case DLL_THREAD_ATTACH:
			break;
    	case DLL_THREAD_DETACH:
			if (gTlsIndex_ErrorBuffer != TLS_OUT_OF_INDEXES)
			{
				lpszStr = TlsGetValue(gTlsIndex_ErrorBuffer);
				if (lpszStr != NULL)
					HeapFree(GetProcessHeap(), 0, lpszStr);
			}
			break;
    	case DLL_PROCESS_DETACH:
			WSACleanup();
			if (gTlsIndex_ErrorBuffer != TLS_OUT_OF_INDEXES)
			{
				lpszStr = TlsGetValue(gTlsIndex_ErrorBuffer);
				if (lpszStr != NULL)
					HeapFree(GetProcessHeap(), 0, lpszStr);
				TlsFree(gTlsIndex_ErrorBuffer);
			}
			break;
		default:
			break;

	}
    return 1;
	UNREFERENCED_PARAMETER(lpReserved);
}
