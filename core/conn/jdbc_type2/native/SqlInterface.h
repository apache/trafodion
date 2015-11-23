/**************************************************************************
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
// MODULE: sqlInterface.h
//
// PURPOSE: function prototype for sqlInterface.cpp
//
//
 /*Change Log
 * Methods Changed: Removed setOfCQD
 */

#ifndef _SQLINTERFACE_DEFINED
#define _SQLINTERFACE_DEFINED
#include "CDesc.h"
#include "CSrvrConnect.h"

//--------------- DATETIME MP datatypes ----------------------------

#define SQLDTCODE_YEAR					4
#define SQLDTCODE_YEAR_TO_MONTH			5
//defineSQLDTCODE_YEAR_TO_DAY			1 //SQL DATE
#define SQLDTCODE_YEAR_TO_HOUR			7 //ODBC TIMESTAMP(0)
#define SQLDTCODE_YEAR_TO_MINUTE		8
//defineSQLDTCODE_YEAR_TO_SECOND		3 //SQL TIMESTAMP(0)
//defineSQLDTCODE_YEAR_TO_FRACTION		3 //SQL TIMESTAMP(1 - 5)
#define SQLDTCODE_MONTH					10
#define SQLDTCODE_MONTH_TO_DAY			11
#define SQLDTCODE_MONTH_TO_HOUR			12
#define SQLDTCODE_MONTH_TO_MINUTE		13
#define SQLDTCODE_MONTH_TO_SECOND		14
#define SQLDTCODE_MONTH_TO_FRACTION		14
#define SQLDTCODE_DAY					15
#define SQLDTCODE_DAY_TO_HOUR			16
#define SQLDTCODE_DAY_TO_MINUTE			17
#define SQLDTCODE_DAY_TO_SECOND			18
#define SQLDTCODE_DAY_TO_FRACTION		18
#define SQLDTCODE_HOUR					19
#define SQLDTCODE_HOUR_TO_MINUTE		20
//define SQLDTCODE_HOUR_TO_SECOND		2  //SQL TIME(0)
//define SQLDTCODE_HOUR_TO_FRACTION		2  //SQL TIME(1 - 6)
#define SQLDTCODE_MINUTE				22
#define SQLDTCODE_MINUTE_TO_SECOND		23
#define SQLDTCODE_MINUTE_TO_FRACTION	23
#define SQLDTCODE_SECOND				24
#define SQLDTCODE_SECOND_TO_FRACTION	24
#define SQLDTCODE_FRACTION_TO_FRACTION	29

#define HANDLE_ERROR(error_code, warning_flag) \
{\
    if (error_code != 0) \
    {\
        if (error_code < 0) \
        CLI_DEBUG_RETURN_SQL(SQL_ERROR); \
        warning_flag = TRUE; \
    }\
}

#define HANDLE_ERROR2(x, y) \
{\
    if (x != 0) \
    {\
        if (x < 0) \
        CLI_DEBUG_RETURN_SQL(x); \
        else \
        y = TRUE; \
    }\
}

#define THREAD_RETURN(pSrvrStmt, return_code) \
{ \
    pSrvrStmt->threadReturnCode = return_code; \
    if (pSrvrStmt->threadReturnCode!=SQL_SUCCESS) pSrvrStmt->processThreadReturnCode(); \
    CLI_DEBUG_RETURN_SQL(pSrvrStmt->threadReturnCode); \
}

#define HANDLE_THREAD_ERROR(error_code, warning_flag, pSrvrStmt) \
{\
    if (error_code != 0) \
    {\
        if (error_code < 0) \
        THREAD_RETURN(pSrvrStmt,SQL_ERROR); \
        warning_flag = TRUE; \
    }\
}

// Macro to to ignore the SQL_NO_DATA_FOUND warning for certain statement types.
// Parameters:
// 1. x - sqlStmtType
// 2. y - sqlcode
#define IGNORE_NODATAFOUND(x, y) \
    ((y == SQL_NO_DATA_FOUND && \
      (x == TYPE_INSERT	|| \
       x == TYPE_UPDATE	|| \
       x == TYPE_DELETE || \
       x == TYPE_INSERT_PARAM)) \
     ? TRUE : FALSE)


extern SQLRETURN GETSQLWARNINGORERROR2(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN GETSQLWARNINGORERROR2forRowsets(SRVR_STMT_HDL* pSrvrStmt);

SQLRETURN GetODBCValues(Int32 DataType, Int32 DateTimeCode, Int32 &Length, Int32 Precision,
        Int32 &ODBCDataType, Int32 &ODBCPrecision, BOOL &SignType,
        Int32 Nullable, Int32 &totalMemLen, Int32 SQLCharset, Int32 &ODBCCharset,
        Int32 IntLeadPrec, char *ColHeading);

SQLRETURN SetDataPtr(SQLDESC_ID *pDesc, SQLItemDescList_def *SQLDesc, Int32 &totalMemLen,
        BYTE *&VarBuffer, SRVR_DESC_HDL *implDesc);

SQLRETURN SetRowsetDataPtr(SQLDESC_ID *pDesc, SQLItemDescList_def *SQLDesc, Int32 sqlStmtType, Int32 maxRowsetSize,
        Int32 numEntries, Int32 &indMemLen, Int32 &dataMemLen, BYTE *&VarBuffer);

SQLRETURN BuildSQLDesc(SQLDESC_ID *pDesc,
        short numEntries,
        SQLItemDescList_def *SQLDesc,
        BYTE *&varBuffer,
        Int32   &totalMemLen,
        SRVR_DESC_HDL   *&implDesc);

SQLRETURN BuildSQLDesc2(SQLDESC_ID *pDesc,
        Int32 sqlQueryType,
        Int32 maxRowsetSize,
        bool &sqlBulkFetchPossible,
        Int32 numEntries,
        BYTE *&SQLDesc,
        Int32 &SQLDescLength,
        BYTE *&varBuffer,
        Int32 &totalMemLen,
        SRVR_DESC_HDL *&implDesc,DESC_HDL_LISTSTMT *&SqlDescInfo);

SQLRETURN BuildSQLDesc2withRowsets(SQLDESC_ID *pDesc,
        Int32 sqlQueryType,
        Int32 maxRowsetSize,
        bool &sqlBulkFetchPossible,
        Int32 numEntries,
        BYTE *&SQLDesc,
        Int32 &SQLDescLength,
        BYTE *&varBuffer,
        Int32 &totalMemLen,
        SRVR_DESC_HDL *&implDesc,
        SQLCLI_QUAD_FIELDS *&inputQuadList_recover,
        SQLCLI_QUAD_FIELDS *&inputQuadList);

SQLRETURN SetIndandVarPtr(SQLDESC_ID *pDesc,
        bool &bRWRS,
        Int32 numEntries,
        BYTE *&SQLDesc,
        BYTE *&varBuffer,
        Int32 &totalMemLen,
        SRVR_DESC_HDL *&implDesc,
        DESC_HDL_LISTSTMT *&SqlDescInfo);

extern SQLRETURN AllocAssignValueBuffer(SQLItemDescList_def *SQLDesc,  SQLValueList_def *SQLValueList,
										long totalMemLen, 	long maxRowCount, BYTE *&VarBuffer);
extern SQLRETURN CopyValueList(SQLValueList_def *outValueList, const SQLValueList_def *inValueList);

extern SQLRETURN EXECUTE(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN EXECUTE_R(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN EXECUTE2(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN FREESTATEMENT(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN PREPARE(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN PREPARE_R(SRVR_STMT_HDL* pSrvrStmt, bool isFromExecDirect = false);

extern SQLRETURN PREPARE2withRowsets(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN EXECUTE2withRowsets(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN FETCH(SRVR_STMT_HDL *pSrvrStmt);

extern SQLRETURN FETCHPERF(SRVR_STMT_HDL *pSrvrStmt, 
        SQL_DataValue_def *outputDataValue);

extern SQLRETURN FETCH2bulk(SRVR_STMT_HDL *pSrvrStmt);

extern SQLRETURN GETSQLERROR(SRVR_STMT_HDL *pSrvrStmt,
        odbc_SQLSvc_SQLError *SQLError);

extern SQLRETURN GETSQLERROR2(bool& bSQLMessageSet, 
        odbc_SQLSvc_SQLError *SQLError);

extern SQLRETURN EXECDIRECT(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN EXECUTESPJRS(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN GETSQLWARNING(SRVR_STMT_HDL *pSrvrStmt,
							   ERROR_DESC_LIST_def *sqlWarning);

extern SQLRETURN CANCEL(SRVR_STMT_HDL *pSrvrStmt);

extern SQLRETURN CLEARDIAGNOSTICS(SRVR_STMT_HDL *pSrvrStmt);

extern SQLRETURN PREPARE_FROM_MODULE(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN ALLOCSQLMXHDLS(SRVR_STMT_HDL *pSrvrStmt);

extern SQLRETURN REALLOCSQLMXHDLS(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN ALLOCSQLMXHDLS_SPJRS(SRVR_STMT_HDL *pSrvrStmt, SQLSTMT_ID *callpStmt, const char *RSstmtName);

extern SQLRETURN EXECUTECALL(SRVR_STMT_HDL *pSrvrStmt);

extern SQLRETURN CONNECT(SRVR_CONNECT_HDL *pSrvrConnect);

extern SQLRETURN DISCONNECT(SRVR_CONNECT_HDL *pSrvrConnect);

extern SQLRETURN SWITCHCONTEXT(SRVR_CONNECT_HDL *pSrvrConnect, long *sqlcode);

extern SQLRETURN GETSQLCODE(SRVR_STMT_HDL *pSrvrStmt);

extern SQLRETURN GetRowsAffected(SRVR_STMT_HDL *pSrvrStmt);

extern "C" 
SQLRETURN GETNOTATOMICROWSET2(bool& bSQLMessageSet, ERROR_DESC_LIST_def *sqlWarning, SRVR_STMT_HDL* pSrvrStmt);

SQLRETURN RECOVERY_FOR_SURROGATE_ERROR2(
        SRVR_STMT_HDL* pSrvrStmt,
        SQLSTMT_ID *pStmt,
        SQLDESC_ID	*pDesc,
        Int32 currRowcnt);

SQLRETURN RECOVERY_FROM_ROWSET_ERROR2(
        SRVR_STMT_HDL *pSrvrStmt
        , SQLDESC_ID	   *pDesc
        , SQLSTMT_ID	   *pStmt
        , Int32       inputRowCnt
        , Int64      *totalrowsAffected);

SQLRETURN GETSQLERROR_AND_ROWCOUNT(
        bool& bSQLMessageSet, 
        odbc_SQLSvc_SQLError *SQLError,
        Int32 RowsetSize,
        Int32 currentRowCount,
        Int32* errorRowCount);

void COPYSQLERROR_LIST_TO_SRVRSTMT(SRVR_STMT_HDL* pSrvrStmt);

void ADDSQLERROR_TO_LIST(
        SRVR_STMT_HDL* pSrvrStmt,
        odbc_SQLSvc_SQLError *SQLError,
        Int32 rowCount);

void INSERT_NODE_TO_LIST(
        SRVR_STMT_HDL* pSrvrStmt,
        ROWSET_ERROR_NODE* pNode,
        Int32 rowCount);

SQLRETURN COMMIT_ROWSET(
        long dialogueId,
        bool& bSQLMessageSet,
        odbc_SQLSvc_SQLError* SQLError,
        Int32 currentRowCount);

bool fatalSQLError(Int32 sqlcode);

SQLRETURN SET_DATA_PTR(SRVR_STMT_HDL *pSrvrStmt, SRVR_STMT_HDL::DESC_TYPE descType);

SQLRETURN _SQL_WaitCloseOnErr (SRVR_STMT_HDL *pSrvrStmt, long int *retcode );

SQLRETURN _SQL_Wait (SRVR_STMT_HDL *pSrvrStmt, long int *retcode );

extern SQLRETURN PREPAREFORMFC(SRVR_STMT_HDL* pSrvrStmt); // New method for MFC - PREPARE + createModule


//MFC  New structure and methods to get datatypes from descriptors
typedef struct InputDescInfo {
	int CountPosition;
	long DataType;
	char DataTypeString[50];
	long Length;
	long DateTimeCode;
	long Precision;
	long SQLCharset;
	long ODBCPrecision;
	long ODBCDataType;
	long Scale;
	long Nullable;
	long IntLeadPrec;

	void setData(int countPosition, long dataType, long length, long scale,long Nullable,
		long dateTimeCode, long precision,long IntLeadPrec, long sQLCharset, SRVR_GLOBAL_Def *srvrGlobal);




	InputDescInfo();


	~InputDescInfo();

	InputDescInfo(const InputDescInfo &rval);


	void operator = (const InputDescInfo &rval);

};

SQLRETURN BuildSQLDesc2ForModFile(SQLDESC_ID pDesc,long numEntries,InputDescInfo *&pInputDescInfo);

SQLRETURN BuildSQLDesc(SRVR_STMT_HDL*pSrvrStmt, SRVR_STMT_HDL::DESC_TYPE descType,InputDescInfo *pInputDescInfo);
void CreateModulePlan(long inputParamCount, InputDescInfo *inputDescInfo, char *inputsqlString,long dialogueId,const char *resMD5);



#endif
