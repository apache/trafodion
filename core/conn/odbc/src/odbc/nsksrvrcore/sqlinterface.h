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
// MODULE: sqlInterface.h
//
// PURPOSE: function prototype for sqlInterface.cpp
//
//


#ifndef _SQLINTERFACE_DEFINED
#define _SQLINTERFACE_DEFINED
#include "odbcCommon.h"
#include "odbcsrvrcommon.h"

#include "odbc_sv.h"
#include "DrvrSrvr.h"

//
// This macro returns SQL_ERROR if there was an error.
//
#define HANDLE_ERROR(x, y) \
	{\
		if (x != 0) \
		{\
			if (x < 0) \
				return SQL_ERROR;\
			else \
				y = TRUE; \
		}\
	}

//
// This macro returns the original error if there was an error.
//
#define HANDLE_ERROR2(x, y) \
	{\
		if (x != 0) \
		{\
			if (x < 0) \
				return x;\
			else \
				y = TRUE; \
		}\
	}

// Added for fix to SQL returning sqlcode=SQL_NO_DATA_FOUND for non-select
// stmts when no rows get affected - Tharak 10/03/06
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

namespace SRVR {

SQLRETURN GetODBCValues(Int32 DataType, Int32 DateTimeCode, Int32 &Length, Int32 Precision, 
						Int32 &ODBCDataType, Int32 &ODBCPrecision, BOOL &SignType, 
						Int32 Nullable, Int32 &totalMemLen, Int32 SQLCharset, Int32 &ODBCCharset,
						Int32 IntLeadPrec, char *ColHeading);
SQLRETURN SetDataPtr(SQLDESC_ID *pDesc, SQLItemDescList_def *SQLDesc, Int32 &totalMemLen,
						BYTE *&VarBuffer, SRVR_DESC_HDL	*implDesc);
SQLRETURN SetRowsetDataPtr(SQLDESC_ID *pDesc, SQLItemDescList_def *SQLDesc, Int32 sqlStmtType, Int32 maxRowsetSize, 
						Int32 numEntries, Int32 &indMemLen, Int32 &dataMemLen, BYTE *&VarBuffer);
SQLRETURN BuildSQLDesc(SQLDESC_ID *pDesc,
						short numEntries,
						SQLItemDescList_def *SQLDesc,
						BYTE *&varBuffer,
						Int32	&totalMemLen,
						SRVR_DESC_HDL	*&implDesc);
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

SQLRETURN RESOURCEGOV(SRVR_STMT_HDL* pSrvrStmt, 				   
						char *pSqlStr,
						double estimated_cost);

extern SQLRETURN AllocAssignValueBuffer(
						bool& bSQLValueListSet,
						SQLItemDescList_def *SQLDesc,  
						SQLValueList_def *SQLValueList, 
						Int32 totalMemLen, 	
						Int32 maxRowCount, 
						BYTE *&VarBuffer);

extern SQLRETURN CopyValueList(SQLValueList_def *outValueList, const SQLValueList_def *inValueList);

extern SQLRETURN EXECUTE(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN FREESTATEMENT(SRVR_STMT_HDL* pSrvrStmt);

SQLRETURN RESOURCEGOV(SRVR_STMT_HDL* pSrvrStmt, 				   
				  char *pSqlStr,
				  double *estimated_cost,
				   RES_HIT_DESC_def *rgPolicyHit);

extern SQLRETURN PREPARE(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN FETCH(SRVR_STMT_HDL *pSrvrStmt);

extern SQLRETURN FETCH2( SRVR_STMT_HDL *pSrvrStmt
                       , Int32 *outValuesFormat
                       , Int32 *outValuesLength
                       , BYTE *&outValues
                       );

extern SQLRETURN GETSQLERROR(bool& bSQLMessageSet,
							 odbc_SQLSvc_SQLError *SQLError);

extern SQLRETURN GETSQLWARNINGORERROR2(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN GETSQLWARNINGORERROR2forRowsets(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN EXECDIRECT(char *pSqlStr, BOOL WriteError=FALSE);

extern SQLRETURN EXECDIRECT(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN GETSQLWARNING(bool& bSQLMessageSet, 
					ERROR_DESC_LIST_def *sqlWarning);

extern SQLRETURN CANCEL(SRVR_STMT_HDL *pSrvrStmt);

//extern SQLRETURN CLEARDIAGNOSTICS(SRVR_STMT_HDL *pSrvrStmt);

extern SQLRETURN FETCHPERF(SRVR_STMT_HDL *pSrvrStmt, 
					   SQL_DataValue_def *outputDataValue);

extern SQLRETURN FETCHROWSET(SRVR_STMT_HDL *pSrvrStmt);

extern SQLRETURN PREPAREROWSET(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN EXECDIRECTROWSET(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN EXECUTEROWSET(SRVR_STMT_HDL* pSrvrStmt);

SQLRETURN RECOVERY_FROM_ROWSET_ERROR(
		SRVR_STMT_HDL* pSrvrStmt,
		SQLDESC_ID	*pDesc,
		SQLSTMT_ID	*pStmt,
		Int32 inputRowCnt,
		Int64 *rowsAffected);

SQLRETURN RECOVERY_FROM_ROWSET_ERROR2(
		SRVR_STMT_HDL* pSrvrStmt,
		SQLDESC_ID	*pDesc,
		SQLSTMT_ID	*pStmt,
		Int32 inputRowCnt,
		Int64 *rowsAffected);

SQLRETURN GETSQLERROR_AND_ROWCOUNT(
		bool& bSQLMessageSet,
		odbc_SQLSvc_SQLError *SQLError,
		Int32 RowsetSize,
		Int32 currentRowCount,
		Int32* errorRowCount);

SQLRETURN COMMIT_ROWSET(
		bool& bSQLMessageSet, 
		odbc_SQLSvc_SQLError* SQLError, 
		Int32 currentRowCount);

void INSERT_NODE_TO_LIST(
		SRVR_STMT_HDL *pSrvrStmt,
		ROWSET_ERROR_NODE* pNode, 
		Int32 rowCount);

void ADDSQLERROR_TO_LIST(
		SRVR_STMT_HDL *pSrvrStmt,
		odbc_SQLSvc_SQLError *SQLError, 
		Int32 rowCount);

void COPYSQLERROR_LIST_TO_SRVRSTMT(
		SRVR_STMT_HDL* pSrvrStmt);

Int32 sqlMaxDataLength(SQLSMALLINT SQLDataType, 
				   SQLINTEGER SQLMaxLength);

Int32 dataLength(SQLSMALLINT SQLDataType, 
				SQLINTEGER SQLOctetLength, 
				void* buffer);

extern void checkIfRowsetSupported();

SQLRETURN GetRowsAffected(SRVR_STMT_HDL *pSrvrStmt);

extern SQLRETURN PREPARE_FROM_MODULE(SRVR_STMT_HDL* pSrvrStmt, 
						 SQLItemDescList_def *inputSQLDesc,
						 SQLItemDescList_def *outputSQLDesc);

extern SQLRETURN ALLOCSQLMXHDLS(SRVR_STMT_HDL *pSrvrStmt);

extern SQLRETURN EXECUTECALL(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN PREPARE2(SRVR_STMT_HDL* pSrvrStmt,bool isFromExecDirect = false);
extern SQLRETURN PREPARE2withRowsets(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN EXECUTE2(SRVR_STMT_HDL* pSrvrStmt);
extern SQLRETURN EXECUTE2withRowsets(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN FETCH2bulk(SRVR_STMT_HDL* pSrvrStmt);

extern SQLRETURN FETCHCATALOGPERF(SRVR_STMT_HDL *pSrvrStmt, 
				Int32 maxRowCnt,
				Int32 maxRowLen,
				Int32 *rowsAffected, 
				SQL_DataValue_def *outputDataValue);

// No differences between bottom two functions. But the RECOVERY_FOR_SURROGATE_ERROR caused regression in some cases
// so added RECOVERY_FOR_SURROGATE_ERROR2. From 2.2 ODBC/JDBC driver uses functions ending with 2. Also
// GETNOTATOMICROWSET2 
extern "C" SQLRETURN GETNOTATOMICROWSET(bool& bSQLMessageSet, ERROR_DESC_LIST_def *sqlWarning, SRVR_STMT_HDL *pSrvrStmt = NULL);
extern "C" SQLRETURN GETNOTATOMICROWSET2(bool& bSQLMessageSet, ERROR_DESC_LIST_def *sqlWarning, SRVR_STMT_HDL *pSrvrStmt = NULL);

//NAR
SQLRETURN RECOVERY_FOR_SURROGATE_ERROR(SRVR_STMT_HDL* pSrvrStmt,
		SQLSTMT_ID	*pStmt,
		SQLDESC_ID	*pDesc,
		Int32 currRowcnt);
SQLRETURN RECOVERY_FOR_SURROGATE_ERROR2(SRVR_STMT_HDL* pSrvrStmt,
		SQLSTMT_ID	*pStmt,
		SQLDESC_ID	*pDesc,
		Int32 currRowcnt);

//Indicator and var Pointer
SQLRETURN SetIndandVarPtr(SQLDESC_ID *pDesc,
						bool &bRWRS,
						Int32 numEntries,
						BYTE *&SQLDesc,
						BYTE *&varBuffer,
						Int32 &totalMemLen,
						SRVR_DESC_HDL *&implDesc,
						DESC_HDL_LISTSTMT *&SqlDescInfo);

extern SQLRETURN REALLOCSQLMXHDLS(SRVR_STMT_HDL *pSrvrStmt);
}


#endif
