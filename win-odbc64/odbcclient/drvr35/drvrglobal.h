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
//
#ifndef DRVRGLOBAL_H
#define DRVRGLOBAL_H

#include "windows.h"
#include "Transport.h"
#include "CHandle.h"
#include "OdbcMsg.h"
#include "trafsqlext.h"
#include "charsetconv.h"

extern char* VprocString;

#define	SQL_ATTR_ROWSET_RECOVERY	2000
#define	SQL_ATTR_FETCH_BUFFER_SIZE	2001

//wms_mapping 
#define SQL_MAX_SERVICENAME_LEN		512
#define MAX_SQL_IDENTIFIER_LEN		512
#define DRVR_PENDING				99999

#define UNKNOWN_EXCEPTION						97
#define COMM_LINK_FAIL_EXCEPTION				98
#define TIMEOUT_EXCEPTION						99
#define TRANSPORT_ERROR						101
#define MAX_PING_RETRY_ATTEMPTS					3

// Set values ranges from 0x00000001 thru 0x000.......
// Add this defines in driver code in drvrglobal.h
#define MXO_ODBC_35					1
#define MXO_MSACCESS_1997			2
#define MXO_MSACCESS_2000			4
#define MXO_BIGINT_NUMERIC			8
// end of add define in drvrglobal.h

// Literals used in the VERSION_def - same as Global.h 
// Make sure changes are in sync with global.h
#define	NT_ODBCAS_COMPONENT		1
#define NSK_ODBCAS_COMPONENT	2
#define	SQL_COMPONENT			3
#define ODBC_SRVR_COMPONENT		4
#define ODBC_CFGSRVR_COMPONENT	5
#define CFGCL_COMPONENT			6
#define	DRVR_COMPONENT			7
#define APP_COMPONENT			8
#define CLPS_SRVR_COMPONENT		9
#define	JDBC_DRVR_COMPONENT		20
#define	LINUX_DRVR_COMPONENT	21
#define	HPUX_DRVR_COMPONENT		22
#define	AIX_DRVR_COMPONENT		23
#define OLEDB_DRVR_COMPONENT		24
#define DOT_NET_DRVR_COMPONENT		25
#define WIN_UNICODE_DRVR_COMPONENT		26
#define SUNSPARC32_DRVR_COMPONENT 30
#define SUNSPARC64_DRVR_COMPONENT 31


#define	NT_VERSION_MAJOR_1	1
#define	NT_VERSION_MINOR_0	0
#define NT_BUILD_1			1

#define	NSK_VERSION_MAJOR_1	NT_VERSION_MAJOR_1+2
#define	NSK_VERSION_MINOR_0	0
#define NSK_BUILD_1			1

#define NSK_ENDIAN				256	// Same as literal in Global.h

#define	SQLERRWARN 1
#define	ESTIMATEDCOSTRGERRWARN	2
#define INFOSTATSERR 3
#define CONFIGERR 4

//#define CODEPAGE_JAPANESE                 932
//#define CODEPAGE_SIMPLIFIED_CHINESE       936
//#define CODEPAGE_TRADITIONAL_CHINESE      950

#define ODBC_RESOURCE_DLL     "traf_ores0100.dll"

#define STRICT_SCHEMA_ENV_VAL_SIZE 100

#define MAX_DOUBLE_TO_CHAR_LEN (DBL_DIG + 13)  // sign(1) + integer(1) + point(1) + fraction(DBL_DIG + 2) + e(1) + sign(1) + exp(3) + '\0' + align(2)

typedef enum SRVR_TYPE
{
	SRVR_UNKNOWN = 0,
	AS_SRVR,
	CORE_SRVR,
	CFG_SRVR
} SRVR_TYPE;

typedef enum SRVR_STATE
{
	SRVR_UNINITIALIZED = 0,
	SRVR_STARTING,
	SRVR_STARTED,
	SRVR_AVAILABLE,
	SRVR_CONNECTING,
	SRVR_CONNECTED,
	SRVR_DISCONNECTING,
	SRVR_DISCONNECTED,
	SRVR_STOPPING,
	SRVR_STOPPED,
	SRVR_ABENDED,
	SRVR_CONNECT_REJECTED,
	SRVR_CONNECT_FAILED,
	SRVR_CLIENT_DISAPPEARED,
	SRVR_ABENDED_WHEN_CONNECTED, // This combination state is used to update stat of DS
	SRVR_STATE_FAULT,
	SRVR_STOP_WHEN_DISCONNECTED
} SRVR_STATE;

typedef enum ERROR_COMPONENT
{
	 DRIVER_ERROR =	0,
	 SERVER_ERROR,
	 NETWORK_ERROR,
	 SQLMX_ERROR,
	 ASSOC_SERVER_ERROR,
	 CONFIG_SERVER_ERROR
} ERROR_COMPONENT;

// End of literals in Global.h

#define		SQL_API_GETOBJREF					-1		// Hope ODBC doesn't use this value 
// Literals used in PostODBCError
#define CANCEL_PROCNAME							"CANCEL"
#define CONVERT_ARK_TO_CTYPE_PROCNAME			"ConvertArkToCType"
#define CONVERT_C_CHAR_TO_NUMERIC_PROCNAME		"ConvertCCharToNumeric"
#define CONVERT_SQL_CHAR_TO_NUMERIC_PROCNAME	"ConvertSQLCharToNumeric"
#define ENDTRANSACT_PROCNAME					"ENDTRANSACT"
#define EXECBINDPARAM_PROCNAME					"ExecBindParam"
#define EXECDIRECT_PROCNAME						"EXECDIRECT"
#define EXECPARAMDATA_PROCNAME					"ExecParamData"
#define EXECPREPARE_PROCNAME					"ExecPrepare"
#define EXECUTE_PROCNAME						"EXECUTE"
#define FETCH_PROCNAME							"FETCH"
#define FREESTATEMENT_PROCNAME					"FREESTATEMENT"
#define GET_OBJECT_REF_PROCNAME					"GET_OBJECT_REF"
#define INITIALIZE_DIALOG_PROCNAME				"INITIALIZE_DIALOG"
#define PREPARE_PROCNAME						"PREPARE"
#define SETCONNECT_PROCNAME						"SETCONNECT"
#define SETDIAGINFO_PROCNAME					"SETDIAGINFO"
#define THREAD_CONTROL_PROCNAME					"ThreadControlProc"
#define TERMINATE_DIALOG_PROCNAME				"TERMINATE_DIALOG"
#define WAIT_FOR_INITIALIZE_DIALOG_PROCNAME		"WaitForInitializeDialogue"
#define WAIT_FOR_SERVER_PROCNAME				"WaitForServer"
#define SMDCATALOGS_PROCNAME					"SMDCATALOGS" 
#define SQLALLOCSTMT_PROCNAME					"SQLAllocStmt"
#define SQLBINDCOL_PROCNAME						"SQLBindCol"
#define SQLBINDPARAMETER_PROCNAME				"SQLBindParameter"
#define SQLBROWSECONNECT_PROCNAME				"SQLBrowseConnect"
#define SQLCANCEL_PROCNAME						"SQLCancel"
#define SQLCOLATTRIBUTES_PROCNAME				"SQLColAttributes"
#define SQLCOLUMNPRIVILEGES_PROCNAME			"SQLColumnPrivileges"
#define SQLCOLUMNS_PROCNAME						"SQLColumns"
#define SQLCONNECT_PROCNAME						"SQLConnect"
#define SQLDRIVERCONNECT_PROCNAME				"SQLDriverConnect"
#define SQLDESCRIBECOL_PROCNAME					"SQLDescribeCol"
#define SQLDESCRIBEPARAM_PROCNAME				"SQLDescribeParam"
#define SQLFETCH_PROCNAME						"SQLFetch"
#define SQLFREESTMT_PROCNAME					"SQLFreeStmt"
#define SQLGETCONNECTOPTION_PROCNAME			"SQLGetConnectOption"
#define SQLGETCURSORNAME_PROCNAME				"SQLGetCursorName"
#define SQLGETDATA_PROCNAME						"SQLGetData"
#define SQLGETINFO_PROCNAME						"SQLGetInfo"
#define SQLGETSTMTOPTION_PROCNAME				"SQLGetStmtOption"
#define SQLGETTYPEINFO_PROCNAME					"SQLGetTypeInfo"
#define SQLERROR_PROCNAME						"SQLError"
#define SQLEXECDIRECT_PROCNAME					"SQLExecDirect"
#define SQLEXECUTE_PROCNAME						"SQLExecute"
#define SQLFOREIGNKEYS_PROCNAME					"SQLForeignKeys"
#define SQLMORERESULTS_PROCNAME					"SQLMoreResults"
#define SQLNATIVESQL_PROCNAME					"SQLNativeSql"
#define SQLNUMPARAMS_PROCNAME					"SQLNumParams"
#define SQLNUMRESULTCOLS_PROCNAME				"SQLNumResultCols"
#define SQLPARAMDATA_PROCNAME					"SQLParamData"
#define SQLPREPARE_PROCNAME						"SQLPrepare"
#define SQLPRIMARYKEYS_PROCNAME					"SQLPrimaryKeys"
#define SQLPUTDATA_PROCNAME						"SQLPutData"
#define SQLROWCOUNT_PROCNAME					"SQLRowCount"
#define SQLSETCONNECTOPTION_PROCNAME			"SQLSetConnectOption"
#define SQLSETCURSORNAME_PROCNAME				"SQLSetCursorName"
#define SQLSETSCROLLOPTIONS_PROCNAME			"SQLSetScrollOptions"
#define SQLSETSTMTOPTION_PROCNAME				"SQLSetStmtOption"
#define SQLSPECIALCOLUMNS_PROCNAME				"SQLSpecialColumns"
#define SQLSTATISTICS_PROCNAME					"SQLStatistics"
#define SQLTABLEPRIVILEGES_PROCNAME				"SQLTablePrivileges"
#define SQLTABLES_PROCNAME						"SQLTables"
#define SQLTRANSACT_PROCNAME					"SQLTransact"
#define SQLDISCONNECT_PROCNAME					"SQLDisconnect"
#define SQLFREECONNECT_PROCNAME					"SQLFreeConnect"
#define SQLCATALOGS_PROCNAME					"SQLCatalogs" 

#define ODBCMX_ERROR_MSGBOX_TITLE				"TRF ODBC Error"
#define MSG_SERVERID_EMPTY						"Server ID empty, Must be entered"

/**********************************************************************
* Rajani - Adding the Message defines which will be used in ChangePwdProc
*
***********************************************************************/
#define MSG_USERNAME_EMPTY						"UserName empty, Must be entered"
#define MSG_OLDPASSWORD_EMPTY					"Password empty, Must be entered"
#define MSG_NEWPASSWORD_EMPTY					"New Password empty, Must be entered"
#define MSG_VERIFYPASSWORD_EMPTY				"Please re-enter password"
#define MSG_VERIFYPASSWORD_EQUAL				"New Passwords verification failed"
#define MSG_OLDPASSWORD_EQUAL					"Old Password verification failed"

#define EMPTY_STRING							""
#define YES_STRING								"Y"
#define NO_STRING								"N"
#define MAX_CAPSULE_NAME_LENGTH					20
#define	DRIVER_DLL_NAME							"trfodbc1.dll"
#define TRANSLATE_DLL_NAME						"traf_translation01.dll"
#define TRACE_DLL_NAME							"traf_odbctrace0100.dll"
#define TCPIPV4_DLL_NAME						"traf_tcpipv40100.dll"
#define TCPIPV6_DLL_NAME						"traf_tcpipv60100.dll"
#define TCPIPINITIO_PROCNAME					"TCPIPInitIO"
#define TCPIPEXITIO_PROCNAME					"TCPIPExitIO"
#define TCPIPOPENSESSION_PROCNAME				"TCPIPOpenSession"
#define TCPIPDOWRITEREAD_PROCNAME				"TCPIPDoWriteRead"
#define TCPIPCLOSESESSION_PROCNAME				"TCPIPCloseSession"
#define SQL_DRIVER_TO_DATASOURCE				"SQLDriverToDataSource"
#define SQL_DATASOURCE_TO_DRIVER				"SQLDataSourceToDriver"
typedef bool (WINAPI* FPTCPIPInitIO)(HMODULE );
typedef bool (WINAPI* FPTCPIPExitIO)();
typedef bool (WINAPI* FPTCPIPOpenSession)(void*, char* );
typedef void (WINAPI* FPTCPIPCloseSession)(void* );
typedef bool (WINAPI* FPTCPIPDoWriteRead)(void* pTCPIPSystem, HEADER*& hdr, char*& buffer, long& bufcount, unsigned int timeout);
typedef BOOL (SQL_API* FPSQLDriverToDataSource) (UDWORD fOption, SWORD fSqlType, PTR rgbValueIn, SDWORD cbValueIn, PTR rgbValueOut, SDWORD cbValueOutMax, SDWORD *pcbValueOut, UCHAR *szErrorMsg, SWORD cbErrorMsgMax, SWORD *pcbErrorMsg);
typedef BOOL (SQL_API* FPSQLDataSourceToDriver) (UDWORD fOption, SWORD fSqlType, PTR rgbValueIn, SDWORD cbValueIn, PTR rgbValueOut, SDWORD cbValueOutMax, SDWORD *pcbValueOut, UCHAR *szErrorMsg, SWORD cbErrorMsgMax, SWORD *pcbErrorMsg, PTR	replacementChar );

// Status Values used for SQL_DATA_AT_EXEC Values
#define SQL_WAITING_FOR_DATA -10
#define SQL_RECEIVING_DATA	 -11
#define SQL_DATA_COPIED		 -12

// Statement States							Equivalient States in ODBC Manual
#define STMT_ALLOCATED				1		//	S1
#define STMT_PREPARED_NO_RESULT		2		//	S2 
#define STMT_PREPARED				3		//	S3
#define STMT_EXECUTED_NO_RESULT		4		//	S4
#define	STMT_EXECUTED				5		//	S5
#define STMT_FETCHED				6		//	S6 or S7
#define STMT_FETCHED_TO_END			7		//	S6 or S7, but returned SQL_NO_DATA
#define STMT_PARAM_DATA_NOT_CALLED	8		//  S8
#define STMT_PUT_DATA_NOT_CALLED	9		//	S9
#define STMT_PUT_DATA_CALLED		10		//	S10
#define STMT_STILL_EXECUTING		11		//	S11
#define STMT_CANCELLED				12		//	S12

// DSN Types
#define UNKNOWN_DSN			0
#define HKCU_DSN			1
#define HKLM_DSN			2
#define FILE_DSN			3
#define DRIVER_KW_DSN		4

#define MAX_TRANSLATE_ERROR_MSG_LEN		512

#define GETOUTCHARSET(translateOption)	(translateOption & 0x0000ffff)
#define GETINCHARSET(translateOption)	(translateOption >> 16)

#define NO_LOCALE_TRANS 0
#define TWO_STEP_LOCALE_TRANS 1

class CDrvrGlobal {

public: 
	CDrvrGlobal();
	~CDrvrGlobal();
	
public :
	
	CHandleGlobal		gHandle;
	OdbcMsg				gOdbcMsg;
	
	CRITICAL_SECTION	gCSObject;
	CRITICAL_SECTION	gHandleCSObject;
	char				gCapsuleName[MAX_CAPSULE_NAME_LENGTH+1];
	TCHAR				gComputerName[MAX_COMPUTERNAME_LENGTH+1];
	DWORD				gProcessId;
	HMODULE				gModuleHandle;
	DWORD				gPlatformId;		// OS related
	DWORD				gMinorVersion;		// OS related
	VERSION_def			gClientVersion;
	char				gDriverDLLName[256];
	char				gClientVproc[100+1]; // Driver vproc
	CEE_handle_def		gSQLSvc_ifch;
	CEE_handle_def		gASSvc_ifch;
	BOOL				gCEEInitialized;
	HANDLE				gCEESyncEvent;
	HANDLE				gCEEExecuteThread;
	unsigned int		gCEEThreadaddr;
	HCURSOR				gWaitCursor;
	unsigned short		gCEEMessageFilter;
	HWND				ghWindow; // Rajani - Added to store the WindowHandle of parent window
	
	SQLUINTEGER			gDefaultTxnIsolation;

	void				loadTransportAndTranslateDll();
	int					gTCPIPLoadError;
	int					gTranslateLoadError;
	char				gTCPIPLibrary[256];
	char				gTranslateLibrary[256];
	HMODULE				gTCPIPHandle;
	HMODULE				gTranslateLibHandle;
	FPTCPIPInitIO		fpTCPIPInitIO;
	FPTCPIPExitIO		fpTCPIPExitIO;
	FPTCPIPOpenSession	fpTCPIPOpenSession;
	FPTCPIPCloseSession	fpTCPIPCloseSession;
	FPTCPIPDoWriteRead	fpTCPIPDoWriteRead;
	FPSQLDriverToDataSource	fpSQLDriverToDataSource;
	FPSQLDataSourceToDriver	fpSQLDataSourceToDriver;
	BOOL                gSpecial_1;
	DWORD	gUTF8_To_UTF16_TranslateOption;
	DWORD	gUTF16_To_UTF8_TranslateOption;
	bool RestrictSchema;
	bool noSchemaInDSN;
	BOOL gTraceCompression;

};

typedef struct RETURN_VALUE_STRUCT
{
	SQLINTEGER dataType;
	union
	{
		SQLINTEGER		s32Value;
		SQLUINTEGER		u32Value;
		SQLSMALLINT		s16Value;
		SQLUSMALLINT	u16Value;
		SQLPOINTER		pValue;
		IDL_long_long	s64Value;
		IDL_unsigned_long_long	u64Value;
		char			*strPtr;
	} u;
} RETURN_VALUE_STRUCT;

// Please do not change the order of the CONNECT_KEYWORDS
typedef enum CONNECT_KEYWORDS
{
	KEY_DRIVER = 0,
	KEY_DSN,
	KEY_FILEDSN,
	KEY_SERVER,
	KEY_SAVEFILE,
	KEY_UID,
	KEY_PWD,
	KEY_CATALOG,
	KEY_SCHEMA,
	KEY_FETCHBUFFERSIZE,
	KEY_SQL_ATTR_CONNECTION_TIMEOUT,
	KEY_SQL_LOGIN_TIMEOUT,
	KEY_SQL_QUERY_TIMEOUT,
	KEY_DATALANG,
	KEY_ERRORMSGLANG,
	KEY_CTRLINFERNCHAR,
	KEY_TRANSLATIONDLL,
	KEY_TRANSLATIONOPTION,
	KEY_REPLACEMENTCHAR,
	KEY_SDSN,
	KEY_SN,
	KEY_SESSION,
	KEY_APPLICATION,
	KEY_ROLENAME,
	KEY_CERTIFICATEDIR,
	KEY_CERTIFICATEFILE,
	KEY_CERTIFICATEFILE_ACTIVE,
	KEY_COMPRESSION,
	KEY_COMPRESSIONTHRESHOLD,
	KEY_MAX
};

typedef struct CONNECT_FIELD_ITEMS
{
	char	loginId[MAX_SQL_IDENTIFIER_LEN+1];
//	char	password[MAX_SQL_IDENTIFIER_LEN+1];
	char	password[3*MAX_SQL_IDENTIFIER_LEN+3]; 
	char	catalog[MAX_SQL_IDENTIFIER_LEN+1];
	char	schema[MAX_SQL_IDENTIFIER_LEN+1];
	char	server[MAX_SQL_IDENTIFIER_LEN+1];
} CONNECT_FIELD_ITEMS;

typedef struct CONNECT_KEYWORD_TREE
{
	char	*AttrValue;
	short	AttrLength;
	short	AttrRank;
} CONNECT_KEYWORD_TREE;

typedef struct DATETYPE_TABLE
{
	SQLSMALLINT	conciseType;
	SQLSMALLINT	verboseType;
	SQLSMALLINT	datetimeIntervalCode;
	short		columnSizeAttr;				// 0 = N/A, SQL_DESC_LENGTH, SQL_DESC_PRECISION
	short		decimalDigitsAttr;			// 0 = N/A, SQL_DESC_PRECISION, SQL_DESC_SCALE
	short		displaySizeAttr;			// 0 = N/A, SQL_DESC_LENGTH, SQL_DESC_PRECISION, 
											//	-1 = SQL_DESC_LENGTH+2,
											//	Any + Number = DisplaySize
	SQLINTEGER	octetLength;
	SQLSMALLINT	defaultType;				
	char		*typeName;
} DATATYPE_TABLE;

typedef struct DESC_SET_TYPE
{
	SQLSMALLINT	FieldIdentifier;
	SQLSMALLINT	ARDDescType;
	SQLSMALLINT	APDDescType;
	SQLSMALLINT	IRDDescType;
	SQLSMALLINT	IPDDescType;
} DESC_SET_TYPE;

#define SQL_DESC_MODE_UNKNOWN		99

typedef enum DESC_TYPES
{
	RW_DESC_TYPE	= 0,
	READ_DESC_TYPE,
	UNUSED_DESC_TYPE
};

extern DESC_SET_TYPE gDescSetTypes[];

extern SQLRETURN returnAttrValue(BOOL reportError, 
						  CHandle *pHandle,
						  RETURN_VALUE_STRUCT *retValues,
						  SQLPOINTER ValuePtr,
						  SQLINTEGER BufferLength,
						  SQLINTEGER *StringLengthPtr);
extern void ScanConnectString(char *InConnectionString, SQLSMALLINT StringLength1, CONNECT_KEYWORD_TREE *KeywordTree);
extern unsigned long getCDefault(SQLSMALLINT SQLDataType, SQLINTEGER ODBCAppVersion, SQLSMALLINT &DataType);
extern unsigned long getOctetLength(SQLSMALLINT	SQLDataType, 
							 SQLINTEGER		ODBCAppVersion, 
							 SQLPOINTER		DataPtr,
							 SQLINTEGER		StrLen, 
							 SQLSMALLINT	&DataType,
							 SQLINTEGER		&OctetLength);
extern char *rTrim(char *string);
extern char* trim(char *string);
extern char* wmstrim(char *string);
extern char* trimInterval(char *string);
extern char* trimInterval(char *string, SQLINTEGER length);
extern char* rSup(char* string);
extern bool double_to_char (double number, int precision, char* string, short size);
extern bool ctoi64(char* buff, __int64& out, bool* truncation);
extern bool ctoi64(char* buff, __int64& out);
extern __int64 pow(short base, short scale);

extern BOOL checkDatetimeValue(short *datetime_parts);
extern void SLEEP(long sec);

extern CDrvrGlobal		gDrvrGlobal;
extern DATATYPE_TABLE	gSQLDatatypeMap[];
extern DATATYPE_TABLE	gCDatatypeMap[];
extern char				*ConnectKeywords[];
extern char				*ConnectLocalizedIdentifier[];
extern DWORD			*pdwGlobalTraceVariable;
extern DWORD			gTraceFlags;
extern HMODULE			g_hTraceDLL;

#endif
