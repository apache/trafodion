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

#ifndef _SRVRCOMMON_DEFINED
#define _SRVRCOMMON_DEFINED
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


struct SRVR_STMT_HDL_LIST {
	SRVR_STMT_HDL	*pSrvrStmt;
	struct SRVR_STMT_HDL_LIST *next;
	struct SRVR_STMT_HDL_LIST *pre;
};

struct SRVR_SESSION_HDL {
	SRVR_STMT_HDL_LIST *pSrvrStmtListHead;
	SRVR_STMT_HDL	   *pCurrentSrvrStmt;
	Int32				count;
};


// Following are the global variables

#define	NO_OF_DESC_ITEMS		15
#define TIMEBUFSIZE				22  //" [2008-04-21 11:23:59]"
// Note that space for '\0' is not considered here. This buffer will be strcat'ed with another null terminated 
// buffer, and hence the space for '\0' will be available from that buffer.
																
extern SRVR_SESSION_HDL			*pSrvrSession;  // Allocation at the time of ImplInit if it is not allocated
										// already
extern SRVR_GLOBAL_Def			*srvrGlobal;
extern Int32					*TestPointArray;
extern SQLMODULE_ID				nullModule;
extern SQLDESC_ITEM				gDescItems[];
extern char						CatalogName[MAX_ANSI_NAME_LEN+1];
extern char						SchemaName[MAX_ANSI_NAME_LEN+1];
extern char						TableName[MAX_ANSI_NAME_LEN+1];
extern char						DescName[MAX_ANSI_NAME_LEN+1];
extern char						ColumnHeading[MAX_ANSI_NAME_LEN+1];

// Added for exit on SQL un-recoverable errors - Tharak
extern Int32 sqlErrorExit[];
extern short errorIndex;

extern char primaryNodeName[10];


#define NUMERIC_NULL			-32767
#define NUMEIRC_NULL_IN_CHAR	"-32767"

#define SQL_ACCESSMODE_AND_ISOLATION 191 //Arvind
// Function Prototypes
namespace SRVR {

void allocSrvrSessionHdl();
SRVR_STMT_HDL_LIST *allocSrvrStmtHdlList();
SRVR_STMT_HDL *allocSrvrStmtHdl( const IDL_char	 *stmtLabel
	             , const char	*moduleName
	             , Int32			moduleVersion
	             , int64	moduleTimestamp
	             , const char	*inputDescName
	             , const char	*outputDescName
	             , short		sqlStmtType
                 , short		sqlQueryType
                 , Int32			resultSetIndex = 0
                 , SQLSTMT_ID	*callStmtId     = NULL
                 , Int32			*sqlReturn      = NULL
                 );
void addSrvrStmt(SRVR_STMT_HDL *pSrvrStmt);
extern void allocSrvrSessionHdl();
extern void initSqlCore();
extern void formatQueryStateMsg( char *queryStateMsg
							, Int32 retcode
							, const char *queryId
							, const char *sqlText = NULL
							);

extern SRVR_STMT_HDL *getSrvrStmt( const IDL_char *stmtLabel
				 , BOOL            canAddStmt
				 , const char     *moduleName      = NULL
				 , Int32            moduleVersion   = SQLCLI_ODBC_MODULE_VERSION
				 , int64       moduleTimestamp = 0
				 , const char     *inputDescName   = NULL
				 , const char     *outputDescName  = NULL
				 , short           sqlStmtType     = TYPE_UNKNOWN
				 , short           sqlQueryType    = SQL_UNKNOWN
				 , Int32            resultSetIndex  = 0
                 , SQLSTMT_ID     *callStmtId      = NULL
				 , Int32           *sqlReturn       = NULL
				 );
extern SRVR_STMT_HDL *getSrvrStmtByCursorName(const IDL_char	*stmtLabel,
					  	BOOL		canAddStmt);
extern void removeSrvrStmt(SRVR_STMT_HDL *pSrvrStmt);
extern Int32 getAllocLength(Int32 DataType, Int32 Length);
extern void releaseCachedObject(BOOL internalStmt, NDCS_SUBSTATE mx_sub = NDCS_INIT);

extern void convertWildcard(UInt32 metadataId, BOOL isPV, const IDL_char *inName, IDL_char *outName, BOOL isCatalog=FALSE);
extern void convertWildcardNoEsc(UInt32 metadataId, BOOL isPV, const IDL_char *inName, IDL_char *outName, BOOL isCatalog=FALSE);
extern BOOL checkIfWildCard(const IDL_char *inName, IDL_char *outName);
extern short SetAutoCommitOff();
extern short execDirectSQLQuery(SRVR_STMT_HDL *pSrvrStmt, char *pSqlStr,
								odbc_SQLSvc_ExecuteN_exc_ *executeException);

// The following two function protoypes are placed here, though it is not implemented in SrvrCommon.cpp
// Each of the DLL which links with this static LIB, should implement these functions
// in one of their module.
// This is done to minimize the compilcations in project settings and also it is felt that
// each DLL may have different implementation for these functions

extern short executeAndFetchSMDQuery(CEE_tag_def objtag_
						  , const CEE_handle_def *call_id_
						  , DIALOGUE_ID_def dialogueId
						  , const IDL_char *stmtLabel
						  , IDL_long maxRowCnt
						  , IDL_short sqlStmtType
						  , char *tableParam[]
						  , char *inputParam[]
						  , SQLItemDescList_def *outputDesc
						  , odbc_SQLSvc_ExecuteN_exc_ *executeException
						  , odbc_SQLSvc_FetchN_exc_ *fetchException
						  , ERROR_DESC_LIST_def	*sqlWarning
						  , IDL_long *rowsAffected
						  , SQLValueList_def *outputValueList);

extern short completeTransaction( CEE_tag_def objtag_
						, const CEE_handle_def *call_id_
						, DIALOGUE_ID_def dialogueId
						, IDL_unsigned_short transactionOpt
						, odbc_SQLSvc_EndTransaction_exc_ *transactionException
						, ERROR_DESC_LIST_def	*sqlWarning);

extern BOOL writeServerException( short retCode 
							, odbc_SQLSvc_GetSQLCatalogs_exc_ *exception_
							, odbc_SQLSvc_Prepare_exc_ *prepareException
							, odbc_SQLSvc_ExecuteN_exc_ *executeException
							, odbc_SQLSvc_FetchN_exc_ *fetchException);
extern void getSessionId(char*);
extern void getJulianTime(char*);
extern void getCurrentTime(char*);
extern short do_ExecSMD( 
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ odbc_SQLSvc_ExecuteN_exc_ *executeException
  , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_short sqlStmtType
  , /* In    */ char *tableParam[]
  , /* In    */ char *inputParam[]
  , /* Out   */ SQLItemDescList_def *outputDesc);
extern short do_ExecFetchSMD( 
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ odbc_SQLSvc_ExecuteN_exc_ *executeException
  , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ const IDL_char *previousstmtLabel
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_short sqlStmtType
  , /* In    */ char *tableParam[]
  , /* In    */ char *inputParam[]
  , /* In    */ bool resetValues
  , /* Out   */ IDL_long *rowsAffected
  , /* Out   */ SQLItemDescList_def *outputDesc);

extern "C" void
odbc_SQLSvc_Prepare2_sme_(
    /* In    */ IDL_long inputRowCnt
  , /* In    */ IDL_long sqlStmtType
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_string sqlString
  , /* In    */ IDL_long holdableCursor
  , /* In    */ Int32 queryTimeout
  , /* Out   */ IDL_long *returnCode
  , /* Out   */ IDL_long *sqlWarningOrErrorLength
  , /* Out   */ BYTE *&sqlWarningOrError
  , /* Out   */ IDL_long *sqlQueryType
  , /* Out   */ Long *stmtHandle
  , /* Out   */ IDL_long *estimatedCost
  , /* Out   */ IDL_long *inputDescLength
  , /* Out   */ BYTE *&inputDesc
  , /* Out   */ IDL_long *outputDescLength
  , /* Out   */ BYTE *&outputDesc
  , /* In    */ bool isFromExecDirect = false);
  
extern "C" void
odbc_SQLSvc_Execute2_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ IDL_long sqlAsyncEnable
  , /* In    */ IDL_long queryTimeout
  , /* In    */ IDL_long inputRowCnt
  , /* In    */ IDL_long sqlStmtType
  , /* In    */ Long stmtHandle
  , /* In    */ IDL_long cursorLength
  , /* In    */ IDL_string cursorName
  , /* In    */ IDL_long cursorCharset
   , /* In    */ IDL_long holdableCursor
  , /* In    */ IDL_long inValuesLength
  , /* In    */ BYTE *inValues
  , /* Out   */ IDL_long *returnCodex
  , /* Out   */ IDL_long *sqlWarningOrErrorLength
  , /* Out   */ BYTE *&sqlWarningOrError
  , /* Out   */ IDL_long *rowsAffected
  , /* Out   */ IDL_long *outValuesLength
  , /* Out   */ BYTE *&outValues);
extern "C" void
odbc_SQLSvc_Fetch2_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def  *call_id_
  , /* In    */ DIALOGUE_ID_def        dialogueId
  , /* In    */ IDL_long               sqlAsyncEnable
  , /* In    */ IDL_long               queryTimeout
  , /* In    */ Long               stmtHandle
  , /* In    */ IDL_long               maxRowCnt
  , /* In    */ IDL_long               cursorLength
  , /* In    */ IDL_string             cursorName
  , /* In    */ IDL_long               cursorCharset
  , /* Out   */ IDL_long              *returnCode
  , /* Out   */ IDL_long              *sqlWarningOrErrorLength
  , /* Out   */ BYTE                 *&sqlWarningOrError
  , /* Out   */ IDL_long              *rowsAffected
  , /* Out   */ IDL_long              *outValuesFormat
  , /* Out   */ IDL_long              *outValuesLength
  , /* Out   */ BYTE                  *&outValues);
extern "C" void
odbc_SQLSvc_Prepare2withRowsets_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ IDL_long sqlAsyncEnable
  , /* In    */ IDL_long queryTimeout
  , /* In    */ IDL_long inputRowCnt
  , /* In    */ IDL_long sqlStmtType
  , /* In    */ IDL_long stmtLength
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_long stmtLabelCharset
  , /* In    */ IDL_long cursorLength
  , /* In    */ IDL_string cursorName
  , /* In    */ IDL_long cursorCharset
  , /* In    */ IDL_long moduleNameLength
  , /* In    */ const IDL_char *moduleName
  , /* In    */ IDL_long moduleCharset
  , /* In    */ IDL_long_long moduleTimestamp
  , /* In    */ IDL_long sqlStringLength
  , /* In    */ IDL_string sqlString
  , /* In    */ IDL_long sqlStringCharset
  , /* In    */ IDL_long setStmtOptionsLength
  , /* In    */ IDL_string setStmtOptions
   , /* In    */ IDL_long holdableCursor
  , /* Out   */ IDL_long *returnCode
  , /* Out   */ IDL_long *sqlWarningOrErrorLength
  , /* Out   */ BYTE *&sqlWarningOrError
  , /* Out   */ IDL_long *sqlQueryType
  , /* Out   */ Long *stmtHandle
  , /* Out   */ IDL_long *estimatedCost
  , /* Out   */ IDL_long *inputDescLength
  , /* Out   */ BYTE *&inputDesc
  , /* Out   */ IDL_long *outputDescLength
  , /* Out   */ BYTE *&outputDesc);
extern "C" void
odbc_SQLSvc_Execute2withRowsets_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ IDL_long sqlAsyncEnable
  , /* In    */ IDL_long queryTimeout
  , /* In    */ IDL_long inputRowCnt
  , /* In    */ IDL_long sqlStmtType
  , /* In    */ Long stmtHandle
  , /* In    */ IDL_long cursorLength
  , /* In    */ IDL_string cursorName
  , /* In    */ IDL_long cursorCharset
  , /* In    */ IDL_long holdableCursor 
  , /* In    */ IDL_long inValuesLength
  , /* In    */ BYTE *inValues
  , /* Out   */ IDL_long *returnCode
  , /* Out   */ IDL_long *sqlWarningOrErrorLength
  , /* Out   */ BYTE *&sqlWarningOrError
  , /* Out   */ IDL_long *rowsAffected
  , /* Out   */ IDL_long *outValuesLength
  , /* Out   */ BYTE *&outValues);

extern "C" void
odbc_SQLSrvr_FetchPerf_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ IDL_long *returnCode
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_long maxRowCnt
  , /* In    */ IDL_long maxRowLen
  , /* In    */ IDL_short sqlAsyncEnable
  , /* In    */ IDL_long queryTimeout
  , /* Out   */ IDL_long *rowsAffected
  , /* Out   */ IDL_long *outValuesFormat
  , /* Out   */ SQL_DataValue_def *outputDataValue
  , /* Out   */ IDL_long *sqlWarningOrErrorLength
  , /* Out   */ BYTE     *&sqlWarningOrError);

extern "C" void
odbc_SQLSrvr_ExtractLob_sme_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ odbc_SQLsrvr_ExtractLob_exc_ *exception_
  , /* In    */ IDL_long extractLobAPI
  , /* In    */ IDL_string lobHandle
  , /* In    */ IDL_long_long &lobLength
  , /* Out   */ IDL_long_long &extractLen
  , /* Out   */ BYTE *& extractData);

extern "C" void
odbc_SQLSrvr_UpdateLob_sme_(
    /* In   */ CEE_tag_def objtag_
  , /* In   */ const CEE_handle_def * call_id_
  , /* In   */ odbc_SQLSvc_UpdateLob_exc_ * exception_
  , /* In   */ IDL_short lobUpdateType
  , /* In   */ IDL_string lobHandle
  , /* In   */ IDL_long_long totalLength
  , /* In   */ IDL_long_long offset
  , /* In   */ IDL_long_long length
  , /* In   */ BYTE * data);

extern "C" void 
GETMXCSWARNINGORERROR(
    /* In    */ Int32 sqlcode
  , /* In    */ char *sqlState
  , /* In    */ char *msg_buf
  , /* Out   */ Int32 *MXCSWarningOrErrorLength
  , /* Out   */ BYTE *&MXCSWarningOrError);
void SETSRVRERROR(Int32 srvrErrorCodeType, Int32 srvrErrorCode, char *srvrSqlState, char* srvrErrorTxt, ERROR_DESC_LIST_def *srvrError);
void SETSECURITYERROR(short securityErrorCode, ERROR_DESC_LIST_def *outError);
SQLRETURN doInfoStats(SRVR_STMT_HDL *pSrvrStmt);

// charset support
extern char *getCharsetStr(Int32 charset); 
extern void translateToUTF8(Int32 inCharset, char* inStr, Int32 inStrLen, char* outStr, Int32 outStrMax);

extern "C" void 
rePrepare2( SRVR_STMT_HDL *pSrvrStmt
          , Int32           sqlStmtType
	  , Int32           inputRowCnt
       	  , Int32    	    holdableCursor
          , Int32 	    queryTimeout
	  , SQLRETURN     	*rc 
	  , Int32          	*returnCode
          , Int32      		*sqlWarningOrErrorLength
          , BYTE          	*&sqlWarningOrError );

}
#define INFOSTATS_COLUMN_COUNT 7

#endif
