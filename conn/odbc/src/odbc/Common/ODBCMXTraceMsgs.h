/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1998-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
********************************************************************/
/**************************************************************************
**************************************************************************/

#include <platform_ndcs.h>
#include <sql.h>
#include <sqlext.h>
#include "Global.h"
#include "cee.h"
#include "odbcCommon.h"
#include "odbcsrvrcommon.h"
#include "odbc_sv.h"
#include "DrvrSrvr.h"
#include "srvrcommon.h"
#include "tdm_odbcSrvrMsg.h"
#include "CommonDiags.h"
#include "odbcMxSecurity.h"
#include "CSrvrStmt.h"
#include "odbceventmsgutil.h"

#define ODBCMX_TRACE	  "ODBCMX TRACE"
#define MAX_MSG_LENGTH	  3500
#define SESSIONID_LENGTH  64
// Following causes problems when /usr/include/search.h is included 
// because it has an enum value ENTER
//#define	ENTER 		  "ENTER"
#define EXIT 		  "EXIT"
#define ODBC 		  "ODBC"
#define KRYPTON 	  "KRYPTON"
#define SQL 		  "SQL"
#define COLLECTORTYPE	  "MXCS server trace"


class ODBCMXTraceMsg
{
  public:
	int  seqNumber;
	char seqNumStr[25];
	char funcName[50];

  protected:
	bool		collectorOpenError;
	DWORD 		pid;
	IDL_OBJECT_def 	objectRef;
	char		collector[EXT_FILENAME_LEN];
	char		*msgBuffer;
	char		*temp;

  public:
	ODBCMXTraceMsg(DWORD processId, IDL_OBJECT_def objRef);
	~ODBCMXTraceMsg();
	void OpenTraceCollector(char* collectorName);
	void CloseTraceCollector();

	void TraceConnectEnter(const USER_DESC_def	    *userDesc,
			       const CONNECTION_CONTEXT_def *inContext,
			       DIALOGUE_ID_def		     dialogueId);
	void TraceConnectExit(odbc_SQLSvc_InitializeDialogue_exc_ exception, 
			      OUT_CONNECTION_CONTEXT_def	  outContext);
	void TraceDisconnectEnter(DIALOGUE_ID_def dialogueId);
	void TraceDisconnectExit(odbc_SQLSvc_TerminateDialogue_exc_ exception);
	void TraceConnectOptionEnter(DIALOGUE_ID_def dialogueId, 
				     IDL_short	     connectionOption,
				     IDL_long	     optionValueNum, 
				     IDL_string	     optionValueStr);
	void TraceConnectOptionExit(odbc_SQLSvc_SetConnectionOption_exc_ exception,
				    ERROR_DESC_LIST_def			 sqlWarning);
	void TraceEndTransactEnter(DIALOGUE_ID_def    dialogueId,
				   IDL_unsigned_short transactionOpt);
	void TraceEndTransactExit(odbc_SQLSvc_EndTransaction_exc_ exception,
				  ERROR_DESC_LIST_def		  sqlWarning);
	void TracePrepareEnter(DIALOGUE_ID_def dialogueId,
			       const IDL_char *stmtLabel,
			       const IDL_char *stmtExplainLabel,
			       IDL_short       stmtType,
			       IDL_string      sqlString,
			       IDL_short       sqlAsyncEnable,
			       IDL_long	       queryTimeout);
	void TracePrepareExit(odbc_SQLSvc_Prepare_exc_ exception, 
			      IDL_long		       estimatedCost, 
			      SQLItemDescList_def      inputDesc, 
			      SQLItemDescList_def      outputDesc, 
			      ERROR_DESC_LIST_def      sqlWarning);
	void TracePrepare2Enter(DIALOGUE_ID_def dialogueId,
				IDL_long sqlAsyncEnable,
				IDL_long queryTimeout,
				IDL_long inputRowCnt,
				IDL_long sqlStmtType,
				IDL_long stmtLength,	
				const IDL_char *stmtLabel,
				IDL_long stmtLabelCharset,
				IDL_long cursorLength,
				IDL_string cursorName,
				IDL_long cursorCharset,
				IDL_long moduleNameLength,
				const IDL_char *moduleName,
				IDL_long moduleCharset,
				IDL_long_long moduleTimestamp,
				IDL_long sqlStringLength,
				IDL_string sqlString,
				IDL_long sqlStringCharset,
				IDL_long setStmtOptionsLength,
				IDL_string setStmtOptions,
				IDL_long_long txnId,
				IDL_long holdableCursor);
	void TracePrepare2Exit(IDL_long returnCode,
				IDL_long sqlWarningOrErrorLength,
				BYTE *sqlWarningOrError,
				IDL_long sqlQueryType,
				Long stmtHandle,
				IDL_long estimatedCost,
				IDL_long inputDescLength,
				BYTE *inputDesc,
				IDL_long outputDescLength,
				BYTE *outputDesc);
	void TraceExecDirectEnter(DIALOGUE_ID_def dialogueId,
				  const IDL_char *stmtLabel,
				  IDL_string	  cursorName,
				  const IDL_char *stmtExplainLabel,
				  IDL_short	  stmtType,
				  IDL_short	  sqlStmtType,
				  IDL_string	  sqlString,
				  IDL_short	  sqlAsyncEnable,
				  IDL_long	  queryTimeout);
	void TraceExecDirectExit(odbc_SQLSvc_ExecDirect_exc_ exception, 
				 IDL_long		     estimatedCost, 
				 SQLItemDescList_def	     outputDesc, 
				 IDL_long		     rowsAffected,
				 ERROR_DESC_LIST_def	     sqlWarning);
	void TraceExecuteEnter(DIALOGUE_ID_def	       dialogueId,
			       const IDL_char	      *stmtLabel,
			       IDL_string	       cursorName,
			       IDL_short	       sqlStmtType,
			       IDL_long		       inputRowCnt,
			       const SQLValueList_def *inputValueList,
			       IDL_short	       sqlAsyncEnable,
			       IDL_long		       queryTimeout);
	void TraceExecuteExit(odbc_SQLSvc_ExecuteN_exc_ exception, 
			      IDL_long		        rowsAffected, 
			      ERROR_DESC_LIST_def	sqlWarning);
	void TraceExecute2Enter(DIALOGUE_ID_def dialogueId,
				IDL_long sqlAsyncEnable,
				IDL_long queryTimeout,
				IDL_long inputRowCnt,
				IDL_long sqlStmtType,
				Long stmtHandle,
				IDL_long cursorLength,
				IDL_string cursorName,
				IDL_long cursorCharset,
				IDL_long inValuesLength,
				BYTE *inValues);
	void TraceExecute2Exit(IDL_long returnCode,
				IDL_long sqlWarningOrErrorLength,
				BYTE *sqlWarningOrError,
				IDL_long rowsAffected,
				IDL_long outValuesLength,
				BYTE *outValues);
	void TraceExecuteCallEnter(DIALOGUE_ID_def	       dialogueId,
			       const IDL_char	      *stmtLabel,
			       IDL_string	       cursorName,
			       IDL_short	       sqlStmtType,
			       IDL_long		       inputRowCnt,
			       const SQLValueList_def *inputValueList,
			       IDL_short	       sqlAsyncEnable,
			       IDL_long		       queryTimeout);
	void TraceExecuteCallExit(odbc_SQLSvc_ExecuteCall_exc_ exception, 
			      IDL_long		        rowsAffected, 
				  SQLValueList_def	    *outputValueList, 
			      ERROR_DESC_LIST_def	sqlWarning);
	void TraceFetchPerfEnter(DIALOGUE_ID_def dialogueId,
				 const IDL_char *stmtLabel,
				 IDL_long	     maxRowCnt,
				 IDL_long	     maxRowLen,
				 IDL_short	     sqlAsyncEnable,
				 IDL_long	     queryTimeout);
	void TraceFetchPerfExit(odbc_SQLSvc_FetchPerf_exc_ exception, 
				IDL_long		       rowsAffected, 
				SQL_DataValue_def	       outputDataValue, 
				ERROR_DESC_LIST_def	       sqlWarning);
	void TraceFetchEnter(DIALOGUE_ID_def dialogueId,
			     const IDL_char *stmtLabel,
			     IDL_long	     maxRowCnt,
			     IDL_long	     maxRowLen,
			     IDL_short	     sqlAsyncEnable,
			     IDL_long	     queryTimeout);
	void TraceFetch2Enter(DIALOGUE_ID_def dialogueId,
			      IDL_long	      sqlAsyncEnable,
			      IDL_long	      queryTimeout,
                           Long        stmtHandle,			     
			      IDL_long	      maxRowCnt,
			      IDL_long        cursorLength,
			      IDL_string      cursorName,
			      IDL_long        cursorCharset);
	void TraceFetch2Exit(IDL_long  returnCode,
			     IDL_long  sqlWarningOrErrorLength,
			     BYTE     *sqlWarningOrError,
			     IDL_long  rowsAffected,
			     IDL_long  outValuesLength,
			     BYTE     *outValues);
	void TraceFetchExit(odbc_SQLSvc_FetchN_exc_    exception, 
			    IDL_long		       rowsAffected, 
			    SQLValueList_def	      *outputValueList, 
			    ERROR_DESC_LIST_def	       sqlWarning);
	void TraceCloseEnter(DIALOGUE_ID_def	 dialogueId,
			     const IDL_char     *stmtLabel,
			     IDL_unsigned_short  freeResourceOpt);
	void TraceCloseExit(odbc_SQLSvc_Close_exc_ exception, 
			    IDL_long		   rowsAffected, 
			    ERROR_DESC_LIST_def	   sqlWarning);
	void TraceClose2Exit(IDL_long returnCode, 
			    IDL_long		   rowsAffected, 
				IDL_long sqlWarningOrErrorLength,
				BYTE *sqlWarningOrError);
	void TraceGetSQLCatalogsEnter(DIALOGUE_ID_def   dialogueId,
				      const IDL_char   *stmtLabel,
				      IDL_short         APIType,
				      const IDL_char   *catalogNm,
				      const IDL_char   *schemaNm,
				      const IDL_char   *tableNm,
				      const IDL_char   *tableTypeList,
				      const IDL_char   *columnNm,
				      IDL_long		columnType,
				      IDL_long		rowIdScope,
				      IDL_long		nullable,
				      IDL_long		uniqueness,
				      IDL_long		accuracy,
				      IDL_short		sqlType,
				      IDL_unsigned_long metadataId,
				      const IDL_char   *fkcatalogNm,
				      const IDL_char   *fkschemaNm,
				      const IDL_char   *fktableNm);
	void TraceGetSQLCatalogsExit(odbc_SQLSvc_GetSQLCatalogs_exc_ exception, 
				     STMT_NAME_def		     catStmtLabel,
				     SQLItemDescList_def	     outputDesc,
				     ERROR_DESC_LIST_def	     sqlWarning);
	void TraceCancelStatementEnter(DIALOGUE_ID_def  dialogueId,
				       const IDL_char  *stmtLabel);
	void TraceCancelStatementExit(odbc_SQLSvc_CancelStatement_exc_ exception, 
				      ERROR_DESC_LIST_def	       sqlWarning);
	void TraceStopServerEnter(DIALOGUE_ID_def dialogueId,
				  IDL_long	  StopType,
				  IDL_string	  ReasonText);
	void TraceStopServerExit(odbc_SQLSvc_StopServer_exc_ exception);
	void TraceFetchRowsetEnter(DIALOGUE_ID_def dialogueId,
				   const IDL_char *stmtLabel,
				   IDL_long	   maxRowCnt,
				   IDL_long	   maxRowLen,
				   IDL_short	   sqlAsyncEnable,
				   IDL_long	   queryTimeout);
	void TraceFetchRowsetExit(odbc_SQLSvc_FetchRowset_exc_ exception, 
				  IDL_long		       rowsAffected, 
				  SQL_DataValue_def	       outputDataValue, 
				  ERROR_DESC_LIST_def	       sqlWarning);

	void TraceSrvrFetchEnter(DIALOGUE_ID_def dialogueId,
			                 IDL_long	     sqlAsyncEnable,
			                 IDL_long	     queryTimeout,
                             	    Long        stmtHandle,			     
			                 IDL_long	     maxRowCnt,
			                 IDL_long        maxRowLen,
							 IDL_long		 fetchAhead=0);

	void TraceSrvrFetchExit(IDL_long         returnCode,
			                IDL_long         sqlWarningOrErrorLength,
			                BYTE             *sqlWarningOrError,
			                IDL_long         rowsAffected,
			                IDL_long         outValuesLength,
			                BYTE             *outValues);

	void TracePrepareRowsetEnter(DIALOGUE_ID_def dialogueId,
				     const IDL_char *stmtLabel,
				     const IDL_char *stmtExplainLabel,
				     IDL_short	     stmtType,
				     IDL_short	     sqlStmtType,
				     IDL_string	     sqlString,
				     IDL_short	     sqlAsyncEnable,
				     IDL_long	     queryTimeout,
				     IDL_long	     maxRowCnt);
	void TracePrepareRowsetExit(odbc_SQLSvc_PrepareRowset_exc_ exception, 
				    IDL_long			   estimatedCost, 
				    SQLItemDescList_def		   inputDesc, 
				    SQLItemDescList_def		   outputDesc, 
				    ERROR_DESC_LIST_def		   sqlWarning);
	void TraceExecDirectRowsetEnter(DIALOGUE_ID_def dialogueId,
					const IDL_char *stmtLabel,
					IDL_string	cursorName,
					const IDL_char *stmtExplainLabel,
					IDL_short	stmtType,
					IDL_short	sqlStmtType,
					IDL_string	sqlString,
					IDL_short	sqlAsyncEnable,
					IDL_long	queryTimeout,
					IDL_long	maxRowCnt);
	void TraceExecDirectRowsetExit(odbc_SQLSvc_ExecDirectRowset_exc_ exception,
				       IDL_long				 estimatedCost, 
				       SQLItemDescList_def		 outputDesc, 
				       IDL_long				 rowsAffected,
				       ERROR_DESC_LIST_def		 sqlWarning);
	void TraceExecuteRowsetEnter(DIALOGUE_ID_def	      dialogueId,
				     const IDL_char	     *stmtLabel,
				     IDL_string		      cursorName,
				     IDL_short		      sqlStmtType,
				     IDL_long		      inputRowCnt,
				     const SQL_DataValue_def *inputDataValue,
				     IDL_short		      sqlAsyncEnable,
				     IDL_long		      queryTimeout);
	void TraceExecuteRowsetExit(odbc_SQLSvc_ExecuteRowset_exc_ exception,
				    IDL_long			   rowsAffected, 
				    ERROR_DESC_LIST_def		   sqlWarning);
	void TraceImplInitEnter(SRVR_INIT_PARAM_Def *initParam, long initParamLen);
	void TraceImplInitExit(CEE_status returnSts);

	char *printLongString(char *buffer, char *longStr);
	char *printUSER_DESC_def(const USER_DESC_def *value, char *buffer);
	char *printCONNECTION_CONTEXT_def(const CONNECTION_CONTEXT_def *value, char *buffer);
	char *printSQLWarning (ERROR_DESC_LIST_def sqlWarning, char *buffer);
	char *printSQLItemDesc(SQLItemDescList_def list, char *buffer);
	char *hexOut(char *buffer, unsigned char *data, int size);
	char *printHex(char *buffer, unsigned char *data, int strSize);
	char *writeTraceMsg(char *buffer, char *data, int dataSize);
	void CheckCollectorError();
};



