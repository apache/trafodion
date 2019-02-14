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
// MODULE: SrvrCommon.cpp
//
// PURPOSE: Implements the common functions used by Srvr 
//
//

#include <platform_ndcs.h>
#include <sql.h>
#include <sqlext.h>
#include "srvrcommon.h"
#include "srvrfunctions.h"
#include "srvrkds.h"
#include "sqlinterface.h"
#include "CommonDiags.h"
#include "tdm_odbcSrvrMsg.h"
#include "NskUtil.h"
#include "ResStatisticsSession.h"
#include "ResStatisticsStatement.h"

// #ifdef _TMP_SQ_SECURITY
#include "secsrvrmxo.h"
// #endif

extern ResStatisticsSession   *resStatSession;
extern ResStatisticsStatement *resStatStatement;

// Fix for bug 1410928
extern SRVR_STMT_HDL * pQueryStmt;
using namespace SRVR; 

short qrysrvcExecuteFinished(
				  const char *stmtLabel
				, const Long stmtHandle
				, const bool bCheckSqlQueryType
				, const short error_code
				, const bool bFetch 
				, const bool bException
				, const bool bErase);


// Global Variables

// mutex for maintaining SRVR_SESSION_HDL list
static pthread_mutex_t 	pSrvrSession_mutex =  PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

SRVR_SESSION_HDL	*pSrvrSession = NULL;  
SRVR_GLOBAL_Def		*srvrGlobal = NULL;	
Int32				*TestPointArray = NULL;
SQLMODULE_ID		nullModule;
SQLDESC_ITEM		gDescItems[NO_OF_DESC_ITEMS];
char				CatalogName[MAX_ANSI_NAME_LEN+1];
char				SchemaName[MAX_ANSI_NAME_LEN+1];
char				TableName[MAX_ANSI_NAME_LEN+1];
char				DescName[MAX_ANSI_NAME_LEN+1];
char				ColumnHeading[MAX_ANSI_NAME_LEN+1];		

extern "C" BYTE* allocGlobalBuffer(Int32 size);

void SRVR::allocSrvrSessionHdl()
{
	SRVRTRACE_ENTER(FILE_COMMON+1);

	markNewOperator,pSrvrSession = new SRVR_SESSION_HDL;
	if (pSrvrSession == NULL)
	{
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
				srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
				srvrGlobal->srvrObjRef, 1, "allocSrvrSessionHdl");
		exit(0);
	}

	pSrvrSession->pSrvrStmtListHead = NULL; 
	pSrvrSession->pCurrentSrvrStmt = NULL;
	pSrvrSession->count	= 0;
	SRVRTRACE_EXIT(FILE_COMMON+1);
}

// added this for metadata sql module
void SRVR::initSqlCore()
{
	SRVRTRACE_ENTER(FILE_COMMON+2);

	if (pSrvrSession == NULL)
   	   allocSrvrSessionHdl();
	// Assign the module Information
	nullModule.version = SQLCLI_ODBC_MODULE_VERSION;
	nullModule.module_name = NULL;
	nullModule.module_name_len = 0;
	nullModule.charset = "ISO88591";
	nullModule.creation_timestamp = 0;

	gDescItems[0].item_id = SQLDESC_TYPE;
	gDescItems[1].item_id = SQLDESC_OCTET_LENGTH;
	gDescItems[2].item_id = SQLDESC_PRECISION;
	gDescItems[3].item_id = SQLDESC_SCALE;
	gDescItems[4].item_id = SQLDESC_NULLABLE;
	gDescItems[5].item_id = SQLDESC_PARAMETER_MODE;
	gDescItems[6].item_id = SQLDESC_INT_LEAD_PREC;
	gDescItems[7].item_id = SQLDESC_DATETIME_CODE;
	gDescItems[8].item_id = SQLDESC_CHAR_SET;
	gDescItems[9].item_id = SQLDESC_TYPE_FS;
	gDescItems[10].item_id = SQLDESC_CATALOG_NAME;
	gDescItems[10].string_val = CatalogName;
	gDescItems[10].num_val_or_len = MAX_ANSI_NAME_LEN;
	gDescItems[11].item_id = SQLDESC_SCHEMA_NAME;
	gDescItems[11].string_val = SchemaName;
	gDescItems[11].num_val_or_len = MAX_ANSI_NAME_LEN;
	gDescItems[12].item_id = SQLDESC_TABLE_NAME;
	gDescItems[12].string_val = TableName;
	gDescItems[12].num_val_or_len = MAX_ANSI_NAME_LEN;
	gDescItems[13].item_id = SQLDESC_NAME;
	gDescItems[13].string_val = DescName;
	gDescItems[13].num_val_or_len = MAX_ANSI_NAME_LEN;
	gDescItems[14].item_id = SQLDESC_HEADING;
	gDescItems[14].string_val = ColumnHeading;
	gDescItems[14].num_val_or_len = MAX_ANSI_NAME_LEN;
	// Make sure you change NO_OF_DESC_ITEMS if you add any more items

	SRVRTRACE_EXIT(FILE_COMMON+2);
}

// Add this for seting query state information to send to event log message repository
void SRVR::formatQueryStateMsg( char *queryStateMsg
							, Int32 retcode
							, const char *queryId
							, const char *sqlText
							)
{
	char tempString[51];

	if(queryStateMsg != NULL)
	{
		sprintf(queryStateMsg, "Query Unique ID: %s. ",queryId);
		snprintf(tempString,51,"Return Code: %d.", retcode);
		strcat(queryStateMsg, tempString);

		if(sqlText != NULL)
		{
			snprintf(tempString,51," SQL Statement Text: %s. ", sqlText);
			strcat(queryStateMsg, tempString);
		}
	}
}

SRVR_STMT_HDL_LIST *SRVR::allocSrvrStmtHdlList()
{
	SRVRTRACE_ENTER(FILE_COMMON+3);

	SRVR_STMT_HDL_LIST *pSrvrStmtList;

	markNewOperator,pSrvrStmtList = new SRVR_STMT_HDL_LIST;
	if (pSrvrStmtList == NULL)
	{
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
				srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
				srvrGlobal->srvrObjRef, 1, "allocSrvrStmtHdlList");
		exit(0);
	}

	pSrvrStmtList->pSrvrStmt = NULL;
	pSrvrStmtList->next = NULL;
	pSrvrStmtList->pre = NULL;

	SRVRTRACE_EXIT(FILE_COMMON+3);

	return pSrvrStmtList;
}

void SRVR::addSrvrStmt(SRVR_STMT_HDL *pSrvrStmt)
{
	SRVRTRACE_ENTER(FILE_COMMON+4);

	SRVR_STMT_HDL_LIST *pSrvrStmtList;
	char tmpString[128];

	int rc = pthread_mutex_lock(&pSrvrSession_mutex);
	if (rc != 0)
	{
		sprintf(tmpString, "Failed to acquire mutex lock in addSrvrStmt: error code %d", rc);
		SendEventMsg(MSG_ODBC_NSK_ERROR,
		             EVENTLOG_ERROR_TYPE,
		             0,
		             ODBCMX_SERVER,
		             srvrGlobal->srvrObjRef,
		             1,
		             tmpString);
		abort();
	}

	pSrvrStmtList = allocSrvrStmtHdlList();
	pSrvrStmtList->pSrvrStmt = pSrvrStmt;
	
	if (pSrvrSession->pSrvrStmtListHead == NULL) 
	{
		pSrvrSession->pSrvrStmtListHead = pSrvrStmtList;
	}
	else
	{
		pSrvrSession->pSrvrStmtListHead->pre = pSrvrStmtList;
		pSrvrStmtList->next = pSrvrSession->pSrvrStmtListHead;
		pSrvrSession->pSrvrStmtListHead = pSrvrStmtList;
	}
	pSrvrSession->pCurrentSrvrStmt = pSrvrStmt;
	pSrvrSession->count++;

	pthread_mutex_unlock(&pSrvrSession_mutex);

	SRVRTRACE_EXIT(FILE_COMMON+4);
	return;
}

void SRVR::removeSrvrStmt(SRVR_STMT_HDL *pSrvrStmt)
{
	SRVRTRACE_ENTER(FILE_COMMON+5);

	SRVR_STMT_HDL_LIST *pSrvrStmtList;
	SRVR_STMT_HDL *lpSrvrStmt;
	char tmpString[128];

	int rc = pthread_mutex_lock(&pSrvrSession_mutex);
	if (rc != 0)
	{
		sprintf(tmpString, "Failed to acquire mutex lock in removeSrvrStmt: error code %d", rc);
		SendEventMsg(MSG_ODBC_NSK_ERROR,
		             EVENTLOG_ERROR_TYPE,
		             0,
		             ODBCMX_SERVER,
		             srvrGlobal->srvrObjRef,
		             1,
		             tmpString);
		abort();
	}

	if (pSrvrSession->pSrvrStmtListHead != NULL)
	{

		for (pSrvrStmtList = pSrvrSession->pSrvrStmtListHead ; pSrvrStmtList != NULL ;)
		{
			lpSrvrStmt = pSrvrStmtList->pSrvrStmt;
			if (lpSrvrStmt == pSrvrStmt)
			{
				if (pSrvrStmtList->pre != NULL)
					pSrvrStmtList->pre->next = pSrvrStmtList->next;
				else // Head
					pSrvrSession->pSrvrStmtListHead = pSrvrStmtList->next;

				if (pSrvrStmtList->next != NULL)
					pSrvrStmtList->next->pre = pSrvrStmtList->pre;
				// If the statement being deleted is current statement, reset the current statement
				if (pSrvrSession->pCurrentSrvrStmt == lpSrvrStmt)
					pSrvrSession->pCurrentSrvrStmt = NULL;
				delete lpSrvrStmt;
				// Fix for bug 1410928. Set the global pQueryStmt to NULL so that the statistics timer thread is aware that the
				// statement no longer exists.
				pQueryStmt = NULL;
				if( trace_memory ) LogDelete("delete lpSrvrStmt;",(void**)&lpSrvrStmt,lpSrvrStmt);
				delete pSrvrStmtList;
				if( trace_memory ) LogDelete("delete pSrvrStmtList;",(void**)&pSrvrStmtList,pSrvrStmtList);
				pSrvrSession->count--;
				pthread_mutex_unlock(&pSrvrSession_mutex);
				return;
			}
			pSrvrStmtList = pSrvrStmtList->next;
		}
	}

	pthread_mutex_unlock(&pSrvrSession_mutex);

	SRVRTRACE_EXIT(FILE_COMMON+5);
	return;
}

SRVR_STMT_HDL *SRVR::allocSrvrStmtHdl(const IDL_char	*stmtLabel,
								const char	    *moduleName,
								Int32			moduleVersion,
								int64		moduleTimestamp,
								const char	    *inputDescName,
								const char	    *outputDescName,
 				     			short	        sqlStmtType,
                                short           sqlQueryType,
				     			Int32            resultSetIndex,
	 			     			SQLSTMT_ID      *callStmtId,
				     			Int32            *sqlReturn
                                     )
{
	SRVRTRACE_ENTER(FILE_COMMON+6);

	SRVR_STMT_HDL *pSrvrStmt;
	SQLRETURN rc;

	markNewOperator,pSrvrStmt = new SRVR_STMT_HDL();
	if (pSrvrStmt == NULL)
	{
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
				srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
				srvrGlobal->srvrObjRef, 1, "allocSrvrStmtHdl");
		exit(0);
	}

     if (sqlQueryType == SQL_SP_RESULT_SET)
     {
	  pSrvrStmt->sqlQueryType   = SQL_SP_RESULT_SET;
       pSrvrStmt->callStmtId     = callStmtId;
	  pSrvrStmt->resultSetIndex = resultSetIndex;
    }

	rc = pSrvrStmt->allocSqlmxHdls(stmtLabel, moduleName, moduleTimestamp, moduleVersion, inputDescName, outputDescName, sqlStmtType);
	if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
	{
                char errorBuf[512];
		int num_errors = pSrvrStmt->sqlError.errorList._length;
		ERROR_DESC_def *error_buffer = (ERROR_DESC_def *)pSrvrStmt->sqlError.errorList._buffer;

                if(num_errors > 0)
                {
                   if(error_buffer->errorText)
                      snprintf(errorBuf,sizeof(errorBuf),"SQL statement allocate failed, sqlcode = %d, sql error = %s",error_buffer->sqlcode,error_buffer->errorText);
                   else
                      snprintf(errorBuf,sizeof(errorBuf),"SQL statement allocate failed, sqlcode = %d",error_buffer->sqlcode);
                }
                else
                   snprintf(errorBuf,sizeof(errorBuf),"SQL statement allocate failed, no sql diagnostics were returned");

		SendEventMsg(MSG_SQL_ERROR,
                                              EVENTLOG_ERROR_TYPE,		
		                              srvrGlobal->nskProcessInfo.processId, 
                                              ODBCMX_SERVER, 
				              srvrGlobal->srvrObjRef, 
                                              1, errorBuf);

		delete pSrvrStmt;
		pSrvrStmt = NULL;
	}
	else
		addSrvrStmt(pSrvrStmt);

	SRVRTRACE_EXIT(FILE_COMMON+6);

     if (sqlReturn != NULL)
         *sqlReturn = rc;

	 return pSrvrStmt;
}

SRVR_STMT_HDL *SRVR::getSrvrStmt( const IDL_char *stmtLabel
				, BOOL	         canAddStmt
				, const char     *moduleName
				, Int32	         moduleVersion
				, int64      moduleTimestamp
				, const char     *inputDescName
				, const char     *outputDescName
			    , short	         sqlStmtType
				, short          sqlQueryType
				, Int32           resultSetIndex
				, SQLSTMT_ID     *callStmtId
				, Int32           *sqlReturn
                    )
{
	SRVRTRACE_ENTER(FILE_COMMON+7);

	SRVR_STMT_HDL *pSrvrStmt;
	SRVR_STMT_HDL_LIST *pSrvrStmtList;
	char tmpString[128];

	int rc = pthread_mutex_lock(&pSrvrSession_mutex);
	if (rc != 0)
	{
		sprintf(tmpString, "Failed to acquire mutex lock in getSrvrStmt: error code %d", rc);
		SendEventMsg(MSG_ODBC_NSK_ERROR,
		             EVENTLOG_ERROR_TYPE,
		             0,
		             ODBCMX_SERVER,
		             srvrGlobal->srvrObjRef,
		             1,
		             tmpString);
		return NULL;
	}

	// Check in the currentSrvrStmt
	if (pSrvrSession->pSrvrStmtListHead != NULL)
	{
		int moduleNameLen = moduleName == NULL? 0 : strlen(moduleName);
		int stmtLabelLen = strlen(stmtLabel);
		int stmtLabelCmp;
		char lastChar = stmtLabel[stmtLabelLen-1];

		if (pSrvrSession->pCurrentSrvrStmt != NULL 
			&& pSrvrSession->pCurrentSrvrStmt->stmtNameLen == stmtLabelLen
			&& lastChar == pSrvrSession->pCurrentSrvrStmt->stmtName[stmtLabelLen-1])
		{
			stmtLabelCmp = memcmp(pSrvrSession->pCurrentSrvrStmt->stmtName, stmtLabel, stmtLabelLen);
			if (moduleName == NULL)
			{
				if (stmtLabelCmp == 0)
				{
					pthread_mutex_unlock(&pSrvrSession_mutex);
					SRVRTRACE_EXIT(FILE_COMMON+7);
					return pSrvrSession->pCurrentSrvrStmt;
				}
			}
			else
			{
				if (stmtLabelCmp == 0
					&& (strcmp(pSrvrSession->pCurrentSrvrStmt->moduleName, moduleName) == 0))
				{
					pthread_mutex_unlock(&pSrvrSession_mutex);
					SRVRTRACE_EXIT(FILE_COMMON+7);
					return pSrvrSession->pCurrentSrvrStmt;
				}
			}
		}
		// Go thru the list

		for (pSrvrStmtList = pSrvrSession->pSrvrStmtListHead ; pSrvrStmtList != NULL ;)
		{
			pSrvrStmt = pSrvrStmtList->pSrvrStmt;
			if (pSrvrStmt->stmtNameLen == stmtLabelLen 
				&& lastChar == pSrvrStmt->stmtName[stmtLabelLen-1])
			{
				stmtLabelCmp = memcmp(pSrvrStmt->stmtName, stmtLabel, stmtLabelLen);
				if (moduleName == NULL)
				{
					if (stmtLabelCmp == 0)
					{
						pSrvrSession->pCurrentSrvrStmt = pSrvrStmt;
						pthread_mutex_unlock(&pSrvrSession_mutex);
						SRVRTRACE_EXIT(FILE_COMMON+7);
						return pSrvrStmt;
					}
				}
				else
				{
					if (stmtLabelCmp == 0 
						&& moduleNameLen == pSrvrStmt->moduleId.module_name_len
						&& strcmp(pSrvrStmt->moduleName, moduleName) == 0)
					{
						pSrvrSession->pCurrentSrvrStmt = pSrvrStmt;
						pthread_mutex_unlock(&pSrvrSession_mutex);
						SRVRTRACE_EXIT(FILE_COMMON+7);
						return pSrvrStmt;
					}
				}
			}
			pSrvrStmtList = pSrvrStmtList->next;
		}
	}
	// Not Found, Hence alloc SrvrStmt
	if (canAddStmt)
		pSrvrStmt = allocSrvrStmtHdl( stmtLabel
                        , moduleName
                        , moduleVersion
                        , moduleTimestamp
                        , inputDescName
                        , outputDescName
                        , sqlStmtType
					    , sqlQueryType
					    , resultSetIndex 
					    , callStmtId
					    , sqlReturn 
                        );
	else
		pSrvrStmt = NULL;

	pthread_mutex_unlock(&pSrvrSession_mutex);

	SRVRTRACE_EXIT(FILE_COMMON+7);

	return pSrvrStmt;
}

SRVR_STMT_HDL *SRVR::getSrvrStmtByCursorName(const IDL_char	*stmtLabel,
											 BOOL			canAddStmt)
{
	SRVRTRACE_ENTER(FILE_COMMON+7);

	SRVR_STMT_HDL *pSrvrStmt;
	SRVR_STMT_HDL_LIST *pSrvrStmtList;
	char tmpString[128];

	int rc = pthread_mutex_lock(&pSrvrSession_mutex);
	if (rc != 0)
	{
		sprintf(tmpString, "Failed to acquire mutex lock in getSrvrStmtByCursorName: error code %d", rc);
		SendEventMsg(MSG_ODBC_NSK_ERROR,
		             EVENTLOG_ERROR_TYPE,
		             0,
		             ODBCMX_SERVER,
		             srvrGlobal->srvrObjRef,
		             1,
		             tmpString);
		return NULL;
	}

	// Check in the currentSrvrStmt
	if (pSrvrSession->pSrvrStmtListHead != NULL)
	{
		int stmtLabelLen = strlen(stmtLabel);
		int stmtLabelCmp;
		char lastChar = stmtLabel[stmtLabelLen-1];

		if (pSrvrSession->pCurrentSrvrStmt != NULL 
			&& pSrvrSession->pCurrentSrvrStmt->stmtNameLen == stmtLabelLen
			&& lastChar == pSrvrSession->pCurrentSrvrStmt->stmtName[stmtLabelLen-1])
		{
			stmtLabelCmp = memcmp(pSrvrSession->pCurrentSrvrStmt->stmtName, stmtLabel, stmtLabelLen);
			if (stmtLabelCmp == 0)
			{
				pthread_mutex_unlock(&pSrvrSession_mutex);
				SRVRTRACE_EXIT(FILE_COMMON+7);
				return pSrvrSession->pCurrentSrvrStmt;
			}
		}
		else if (pSrvrSession->pCurrentSrvrStmt != NULL 
			&& pSrvrSession->pCurrentSrvrStmt->cursorNameLen == stmtLabelLen
			&& lastChar == pSrvrSession->pCurrentSrvrStmt->cursorName[stmtLabelLen-1])
		{
			stmtLabelCmp = memcmp(pSrvrSession->pCurrentSrvrStmt->cursorName, stmtLabel, stmtLabelLen);
			if (stmtLabelCmp == 0)
			{
				pthread_mutex_unlock(&pSrvrSession_mutex);
				SRVRTRACE_EXIT(FILE_COMMON+7);
				return pSrvrSession->pCurrentSrvrStmt;
			}
		}

		// Go thru the list
		for (pSrvrStmtList = pSrvrSession->pSrvrStmtListHead ; pSrvrStmtList != NULL ;)
		{
			pSrvrStmt = pSrvrStmtList->pSrvrStmt;
			if (pSrvrStmt->stmtNameLen == stmtLabelLen 
				&& lastChar == pSrvrStmt->stmtName[stmtLabelLen-1])
			{
				stmtLabelCmp = memcmp(pSrvrStmt->stmtName, stmtLabel, stmtLabelLen);
				if (stmtLabelCmp == 0)
				{
					pSrvrSession->pCurrentSrvrStmt = pSrvrStmt;
					pthread_mutex_unlock(&pSrvrSession_mutex);
					SRVRTRACE_EXIT(FILE_COMMON+7);
					return pSrvrStmt;
				}
				
			}
			else if ((pSrvrStmt->cursorNameLen == stmtLabelLen || pSrvrStmt->cursorNameLen == stmtLabelLen+1) 
				&& lastChar == pSrvrStmt->cursorName[stmtLabelLen-1])
			{
				stmtLabelCmp = memcmp(pSrvrStmt->cursorName, stmtLabel, stmtLabelLen);
				if (stmtLabelCmp == 0)
				{
					pSrvrSession->pCurrentSrvrStmt = pSrvrStmt;
					pthread_mutex_unlock(&pSrvrSession_mutex);
					SRVRTRACE_EXIT(FILE_COMMON+7);
					return pSrvrStmt;
				}
				
			}
			pSrvrStmtList = pSrvrStmtList->next;
		}
	}

	// Not Found, return NULL
	pSrvrStmt = NULL;
	pthread_mutex_unlock(&pSrvrSession_mutex);
	SRVRTRACE_EXIT(FILE_COMMON+7);
	return pSrvrStmt;
}

void SRVR::releaseCachedObject(BOOL internalStmt, NDCS_SUBSTATE mxsrvr_substate)
{
	SRVRTRACE_ENTER(FILE_COMMON+8);

	SRVR_STMT_HDL *pSrvrStmt;
	SRVR_STMT_HDL_LIST *pSrvrStmtList;
	SRVR_STMT_HDL_LIST *nextSrvrStmtList;
	char				*inSqlString = NULL;
	short				inSqlStmtType = TYPE_UNKNOWN;
	double				inEstimatedCost = 0;
	Int32				inErrorCode = 0;
	Int32				inErrorStatement = 0;
	Int32				inSqlQueryType = 0;
	Int32				inNewSqlQueryType = 0;
	Int32				inWarningStatement = 0;
	int64				inRowCount = 0;
	char				*inSqlError = NULL;
	Int32				inSqlErrorLength = 0;
	

//	SQL_DROP is a process hop, hence we should not take a mutex on 
//	pSrvrSession_mutex. Also releaseCachedObject() should not be called
//	by two different threads.

	for (pSrvrStmtList = pSrvrSession->pSrvrStmtListHead ; pSrvrStmtList != NULL ;)
	{
		pSrvrStmt = pSrvrStmtList->pSrvrStmt;
		// If you remove the pSrvrStmt. pSrvrStmtList will also be deleted and hence become undefined.
		// so store the next entry before pSrvrStmtList is deleted
 		nextSrvrStmtList = pSrvrStmtList->next;
		// generate resStatStatement end message if flag is true
		if (pSrvrStmt == NULL) continue;

                // the statement handles for result sets are cleaned up when the call statement gets cleaned up.
                // allowing it to be cleaned up here leads to double deletes and invalid memory references
                // when the parent call stmt handle is being cleaned up.
                if(pSrvrStmt->sqlQueryType == SQL_SP_RESULT_SET)
                {
                   pSrvrStmtList = nextSrvrStmtList;
                   continue;
                }

		if (pSrvrStmt->m_need_21036_end_msg == true && resStatStatement != NULL) 
		{
			// Initialize the counters by calling start() , remove when setStatistics() can handle specific qids.
			
			pSrvrStmt->inState = STMTSTAT_CLOSE;
			inSqlStmtType = TYPE_UNKNOWN;
			inEstimatedCost = 0;
			inSqlString = NULL;
			inErrorStatement = 0;
			inWarningStatement = 0;
			inRowCount = 0;
			inErrorCode = 0;
			inSqlError = NULL;
			inSqlErrorLength = 0;
			/*resStatStatement->start(inState, 
						pSrvrStmt->sqlQueryType, 
						stmtLabel, 
						pSrvrStmt->sqlUniqueQueryID, 
						pSrvrStmt->cost_info,
						pSrvrStmt->comp_stats_info,
						inEstimatedCost, 
						&pSrvrStmt->m_need_21036_end_msg);*/
			resStatStatement->start(pSrvrStmt->inState,
									pSrvrStmt->sqlQueryType, 
									pSrvrStmt->stmtName, 
									pSrvrStmt, 
									inEstimatedCost,
									&pSrvrStmt->m_need_21036_end_msg);

			if (pSrvrStmt->sqlWarningOrError != NULL)
			{
				inErrorCode = *(Int32 *)(pSrvrStmt->sqlWarningOrError+8);
				inErrorStatement ++;
				inSqlError = (char*)pSrvrStmt->sqlWarningOrError + 16;
				inSqlErrorLength =*(Int32 *)(pSrvrStmt->sqlWarningOrError + 12);
			}

			resStatStatement->end(pSrvrStmt->inState,
								  inSqlQueryType,
								  inSqlStmtType,
								  pSrvrStmt->sqlUniqueQueryID,
								  inEstimatedCost,
								  inSqlString,
								  inErrorStatement,
								  inWarningStatement,
								  inRowCount,
								  inErrorCode,
								  resStatSession,
								  inSqlErrorLength,
								  inSqlError,
								  pSrvrStmt,
								  &pSrvrStmt->m_need_21036_end_msg,
								  inNewSqlQueryType,
								  pSrvrStmt->isClosed);

				pSrvrStmt->m_mxsrvr_substate = mxsrvr_substate;

				qrysrvcExecuteFinished(NULL, (Long)pSrvrStmt, false, inErrorCode, false, false, true);

				SQLSTMT_ID	*pStmt = &pSrvrStmt->stmt;

				SQL_EXEC_ClearDiagnostics(pStmt);

				SQL_EXEC_CloseStmt(pStmt);

				resStatStatement->setStatistics(pSrvrStmt);

				if (resStatStatement->getSqlErrorCode())
				{
					GETSQLWARNINGORERROR2(pSrvrStmt);
				}

				if (pSrvrStmt->sqlWarningOrError != NULL)
				{
					inErrorCode = *(Int32 *)(pSrvrStmt->sqlWarningOrError+8);
					inErrorStatement ++;
					inSqlError = (char*)pSrvrStmt->sqlWarningOrError + 16;
					inSqlErrorLength =*(Int32 *)(pSrvrStmt->sqlWarningOrError + 12);
				}

				resStatStatement->endRepository(pSrvrStmt,
					inSqlErrorLength,
					(BYTE*)inSqlError,
					true);
		}
			
		// Ignore any exception thrown and proceed to the next
		if (pSrvrStmt->stmtType == EXTERNAL_STMT)
			pSrvrStmt->Close(SQL_DROP);
		else
			pSrvrStmt->InternalStmtClose(SQL_CLOSE);
		pSrvrStmtList = nextSrvrStmtList;
	}

#ifdef _TMP_SQ_SECURITY
	// security clean-up
	if (!internalStmt)
		NEO_SECURITY_RESET_();
#endif

	SRVRTRACE_EXIT(FILE_COMMON+8);

	return;
}

short SRVR::do_ExecSMD( 
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ odbc_SQLSvc_ExecuteN_exc_ *executeException
  , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_short sqlStmtType
  , /* In    */ char *tableParam[]
  , /* In    */ char *inputParam[]
  , /* Out   */ SQLItemDescList_def *outputDesc)
{
	SRVRTRACE_ENTER(FILE_COMMON+10);

	SRVR_STMT_HDL		*pSrvrStmt;
	IDL_long			rowsAffected;
	SQLItemDesc_def		*SQLItemDesc;
	IDL_unsigned_long	curParamNo;
	Int32				allocLength;
	Int32				retcode;
	SQLRETURN			rc;
	IDL_short			indValue;
	BOOL				tableParamDone;
	IDL_unsigned_long	index;
		
	pSrvrStmt = getSrvrStmt(stmtLabel, TRUE, "HP_SYSTEM_CATALOG.MXCS_SCHEMA.CATANSIMX", 0, 1234567890);
 	if (pSrvrStmt == NULL)
	{
		executeException->exception_nr = odbc_SQLSvc_ExecuteN_ParamError_exn_;
		executeException->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_UNABLE_TO_ALLOCATE_SQL_STMT;
		return EXECUTE_EXCEPTION;
	}
	rc = pSrvrStmt->PrepareFromModule(INTERNAL_STMT);
	if (rc == SQL_ERROR)
	{
		executeException->exception_nr = odbc_SQLSvc_ExecuteN_ParamError_exn_;
		executeException->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_CATSMD_MODULE_ERROR;
		return EXECUTE_EXCEPTION;
	}
	if ((rc = AllocAssignValueBuffer(pSrvrStmt->bSQLValueListSet,&pSrvrStmt->inputDescList, 
		&pSrvrStmt->inputValueList, pSrvrStmt->inputDescVarBufferLen, 1, 
		pSrvrStmt->inputValueVarBuffer)) != SQL_SUCCESS)
	{
		executeException->exception_nr = odbc_SQLSvc_ExecuteN_ParamError_exn_;
		executeException->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_BUFFER_ALLOC_FAILED;
		return EXECUTE_EXCEPTION;
	}
	pSrvrStmt->InternalStmtClose(SQL_CLOSE);
	outputDesc->_length = pSrvrStmt->outputDescList._length;
	outputDesc->_buffer = pSrvrStmt->outputDescList._buffer;
	
	for (curParamNo = 0, index = 0, tableParamDone = FALSE, pSrvrStmt->inputValueList._length = 0; 
			curParamNo < pSrvrStmt->inputDescList._length ; curParamNo++, index++) 
	{
		SQLItemDesc = (SQLItemDesc_def *)pSrvrStmt->inputDescList._buffer + curParamNo;
		allocLength = getAllocLength(SQLItemDesc->dataType, SQLItemDesc->maxLen);
		if (! tableParamDone)
		{
			if (tableParam[index] == NULL)
			{
				tableParamDone = TRUE;
				index = 0;
			}
			else
			{
				retcode = kdsCopyToSMDSQLValueSeq(&pSrvrStmt->inputValueList, 
					SQLItemDesc->dataType, 0, tableParam[index], allocLength, SQLItemDesc->ODBCCharset);
			}
		}
		if (tableParamDone) 
		{
			if  (inputParam[index] == NULL)
				indValue = -1;
			else
				indValue = 0;
			retcode = kdsCopyToSMDSQLValueSeq(&pSrvrStmt->inputValueList, 
					SQLItemDesc->dataType, indValue, inputParam[index], allocLength, SQLItemDesc->ODBCCharset);
		}
		if (retcode != 0)
			return ((short) retcode);
	}
	executeException->exception_nr = 0;
	// sqlStmtType has value of types like TYPE_SELECT, TYPE_DELETE etc.
	odbc_SQLSvc_ExecuteN_sme_(objtag_, call_id_, executeException, dialogueId, stmtLabel, 
			(IDL_string)stmtLabel, 
			sqlStmtType, 1, &pSrvrStmt->inputValueList, SQL_ASYNC_ENABLE_OFF, 0, &rowsAffected, sqlWarning);
	if (executeException->exception_nr != CEE_SUCCESS)
		return EXECUTE_EXCEPTION;

	SRVRTRACE_EXIT(FILE_COMMON+10);

	return 0;
}

// Assuming outName is of sufficient size for efficiency
void SRVR::convertWildcard(UInt32 metadataId, BOOL isPV, const IDL_char *inName, IDL_char *outName, BOOL isCatalog)
{
	SRVRTRACE_ENTER(FILE_COMMON+11);

	char *in = (char *)inName;
	char *out = (char *)outName;
	BOOL	quoted = FALSE;
	BOOL	isThisEndQuote = FALSE;
	BOOL 	leadingBlank = TRUE;
	char	*trailingBlankPtr = NULL;

	while (*in != '\0' && *in == ' ') // Remove the leading blanks
		in++;
	if (metadataId) // Treat the argument as Identifier Argument
	{
		if (*in == '"')
		{
			if (!isCatalog)
				in++;
			else
				*out++ = *in++;
			quoted = TRUE;
		}
		while (*in != '\0')
		{
			if (*in == ' ')
			{
				if (! leadingBlank)
				{
					if (! trailingBlankPtr)
						trailingBlankPtr = out;	// Store the pointer to begining of the blank
				}
				*out++ = *in++;
			}
			else
			{
				leadingBlank = FALSE;
				if (quoted)
				{
					if (isThisEndQuote)
						*out++ = '"';
					if (*in != '"')
					{
						trailingBlankPtr = NULL;
						*out++ = *in++;
						isThisEndQuote = FALSE;
					}
					else
					{
						if (!isCatalog)
						{
							in++;
							if(!isThisEndQuote) // This is needed to make two double quotes as single quote
								isThisEndQuote = TRUE;
							else
								isThisEndQuote = FALSE;
						}
						else
							*out++ = *in++;
					}
				}
				else
				{
					trailingBlankPtr = NULL;	
					switch (*in)
					{
						case '\\':		// Escape Character
							*out++ = *in++;
							break;
						case '_':
						case '%':
							*out++ = '\\';
							break;
						default:
							break;
					}
					if (*in != '\0')
						*out++ = toupper(*in++);
				}
			}
		}
		if (trailingBlankPtr)
			*trailingBlankPtr = '\0';
		else
			*out = '\0';
	}
	else
	{
		if (isPV)
		{
			if (*in == '"')
			{
				in++;
				while (*in != '\0')
				{
					if (isThisEndQuote)
						*out++ = '"';
					if (*in != '"')
					{
						switch (*in)
						{
							case '\\':		// Escape Character
								*out++ = *in++;
								break;
							case '_':
							case '%':
								*out++ = '\\';
								break;
							default:
								break;
						}
						if (*in != '\0')
						{
							*out++ = *in++;
						}
						isThisEndQuote = FALSE;
					}
					else
					{
						in++;
						if(!isThisEndQuote) // This is needed to make two double quotes as single quote
							isThisEndQuote = TRUE;
						else
							isThisEndQuote = FALSE;
					}
				}
				*out = '\0';
			}
			else
				strcpy(outName, inName);
		}
		else
		{
			if (*in == '"')
			{
				if (!isCatalog)
					in++;
				else
					*out++ = *in++;
				quoted = TRUE;
			}
			while (*in != '\0')
			{
				if (quoted)
				{
					if (isThisEndQuote)
						*out++ = '"';
					if (*in != '"')
					{
						*out++ = *in++;
						isThisEndQuote = FALSE;
					}
					else
					{
						if (!isCatalog)
						{
							in++;
							if(!isThisEndQuote) // This is needed to make two double quotes as single quote
								isThisEndQuote = TRUE;
							else
								isThisEndQuote = FALSE;
						}
						else
							*out++ = *in++;
					}
				}
				else
				{
					switch (*in)
					{
						case '\\':		// Escape Character
							*out++ = *in++;
							break;
						case '_':
						case '%':
							*out++ = '\\';
							break;
						default:
							break;
					}
					if (*in != '\0')
						*out++ = *in++;
				}
			}
			*out = '\0';
		}
	}
	SRVRTRACE_EXIT(FILE_COMMON+11);

}

void SRVR::convertWildcardNoEsc(UInt32 metadataId, BOOL isPV, const IDL_char *inName, IDL_char *outName, BOOL isCatalog)
{
	SRVRTRACE_ENTER(FILE_COMMON+12);

	char *in = (char *)inName;
	char *out = (char *)outName;
	BOOL	quoted = FALSE;
	BOOL	isThisEndQuote = FALSE;
	BOOL 	leadingBlank = TRUE;
	char	*trailingBlankPtr = NULL;

	while (*in != '\0' && *in == ' ') // Remove the leading blanks
		in++;
	if (metadataId) // Treat the argument as Identifier Argument
	{
		if (*in == '"')
		{
			if (!isCatalog)
				in++;
			else
				*out++ = *in++;
			quoted = TRUE;
		}
		while (*in != '\0')
		{
			if (*in == ' ')
			{
				if (! leadingBlank)
				{
					if (! trailingBlankPtr)
						trailingBlankPtr = out;	// Store the pointer to begining of the blank
				}
				*out++ = *in++;
			}
			else
			{
				leadingBlank = FALSE;
				if (quoted)
				{
					if (isThisEndQuote)
						*out++ = '"';
					if (*in != '"')
					{
						trailingBlankPtr = NULL;
						*out++ = *in++;
						isThisEndQuote = FALSE;
					}
					else
					{
						if (!isCatalog)
						{
							in++;
							if(!isThisEndQuote) // This is needed to make two double quotes as single quote
								isThisEndQuote = TRUE;
							else
								isThisEndQuote = FALSE;
						}
						else
							*out++ = *in++;
					}
				}
				else
				{
					trailingBlankPtr = NULL;	
					*out++ = toupper(*in++);
				}
			}
		}
		if (trailingBlankPtr)
			*trailingBlankPtr = '\0';
		else
			*out = '\0';
	}
	else
	{
		if (isPV)
		{
			if (*in == '"')
			{
				in++;
				while (*in != '\0')
				{
					if (isThisEndQuote)
						*out++ = '"';
					if (*in != '"')
					{
						*out++ = *in++;
						isThisEndQuote = FALSE;
					}
					else
					{
						in++;
						if(!isThisEndQuote) // This is needed to make two double quotes as single quote
							isThisEndQuote = TRUE;
						else
							isThisEndQuote = FALSE;
					}
				}
				*out = '\0';
			}
			else
				strcpy(outName, inName);
		}
		else
		{
			if (*in == '"')
			{
				if (!isCatalog)
					in++;
				else
					*out++ = *in++;
				quoted = TRUE;
			}
			while (*in != '\0')
			{
				if (quoted)
				{
					if (isThisEndQuote)
						*out++ = '"';
					if (*in != '"')
					{
						*out++ = *in++;
						isThisEndQuote = FALSE;
					}
					else
					{
						if (!isCatalog)
						{
							in++;
							if(!isThisEndQuote) // This is needed to make two double quotes as single quote
								isThisEndQuote = TRUE;
							else
								isThisEndQuote = FALSE;
						}
						else
							*out++ = *in++;
					}
				}
				else
				{
					if (*in != '\\') // Skip the Escape character since application is escapes with \. 
						*out++ = *in++;
					else
						in++;
				}
			}
			*out = '\0';
		}
	}
	SRVRTRACE_EXIT(FILE_COMMON+12);

}

/* This function is used to suppress wildcard escape sequence since we don't support wild card characters 
in CatalogNames except in SQLTables (for certain conditions). 
This function returns an error when the wildcard character % is in the input string. The wildcard character
'_' is ignored and treated like an ordinary character
*/

// Assuming outName is of sufficient size for efficiency
BOOL SRVR::checkIfWildCard(const IDL_char *inName, IDL_char *outName)
{
	SRVRTRACE_ENTER(FILE_COMMON+13);

	char *in = (char *)inName;
	char *out = (char *)outName;

	BOOL	rc = TRUE;

	if (*in != '\0')
	{
		while (*in == ' ')
			in++;
	}

	if (*in == '"')
	{
		in++;
		while (*in != '\0')
		{
			*out++ = *in++;
			if (*in == '"')
			{
				in++;
				break;
			}
		}
	}
	else
	{
		while (*in != '\0')
		{
			switch(*in)
			{
			case '%':
				rc = FALSE;
			case '\\':
				if ((*(in+1) == '_') || (*(in+1) == '%')) // Dont copy '\'
					in++;	
				break;
			default:
				break;
			}
			*out++ = *in++;
		}
	}
	*out = '\0';
	SRVRTRACE_EXIT(FILE_COMMON+13);

	// Because we need descriptors from SQL, we have to block this function and later map an SQL error to an empty result set 
	return TRUE;
	return rc;
}


// Following functions are used in Configuration Server and InitSv.
// executeSQLQuery(), executeAndFetchSQLQuery(), completeTransaction()
// SetAutoCommitOff().
// CatalogNm = NONSTOP_SQLMX_NSK fixed.
// SqlWarning is ignored at present.
// 

short SRVR::executeAndFetchSMDQuery(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ DIALOGUE_ID_def dialogueId
  , /* In    */ const IDL_char *stmtLabel
  , /* In    */ IDL_long maxRowCnt
  , /* In    */ IDL_short sqlStmtType
  , /* In    */ char *tableParam[]
  , /* In    */ char *inputParam[]
  , /* Out   */ SQLItemDescList_def *outputDesc
  , /* Out   */ odbc_SQLSvc_ExecuteN_exc_ *executeException
  , /* Out   */ odbc_SQLSvc_FetchN_exc_ *fetchException
  , /* Out   */ ERROR_DESC_LIST_def	*sqlWarning
  , /* Out   */ IDL_long *rowsAffected
  , /* Out   */ SQLValueList_def *outputValueList)
{
	SRVRTRACE_ENTER(FILE_COMMON+16);

	short retCode;
	
	retCode = do_ExecSMD(objtag_, call_id_, executeException, sqlWarning, dialogueId,
				stmtLabel, sqlStmtType, tableParam, inputParam, outputDesc);
	if(retCode != CEE_SUCCESS)
		return retCode;
	
	odbc_SQLSvc_FetchN_sme_(objtag_, call_id_, fetchException, dialogueId, stmtLabel, maxRowCnt, 0,
			SQL_ASYNC_ENABLE_OFF, 0, rowsAffected, outputValueList, sqlWarning);
	if (fetchException->exception_nr != CEE_SUCCESS) 
		return FETCH_EXCEPTION;

	SRVRTRACE_EXIT(FILE_COMMON+16);

	return CEE_SUCCESS;
}

short SRVR::completeTransaction( CEE_tag_def objtag_
						, const CEE_handle_def *call_id_
						, DIALOGUE_ID_def dialogueId
						, IDL_unsigned_short transactionOpt
						, odbc_SQLSvc_EndTransaction_exc_ *transactionException
						, ERROR_DESC_LIST_def *sqlWarning)
{
	SRVRTRACE_ENTER(FILE_COMMON+17);

	transactionException->exception_nr =0;
	sqlWarning->_length=0;
	sqlWarning->_buffer=NULL;

	odbc_SQLSvc_EndTransaction_sme_( objtag_, call_id_, transactionException
										, dialogueId, transactionOpt, sqlWarning );

	if (transactionException->exception_nr != CEE_SUCCESS) 
		return TRANSACTION_EXCEPTION;
	
	SRVRTRACE_EXIT(FILE_COMMON+17);

	return CEE_SUCCESS;
}

short SRVR::SetAutoCommitOff()
{
	SRVRTRACE_ENTER(FILE_COMMON+18);

	static BOOL autoCommitSet;
	short retcode = 0;
	
	if (autoCommitSet)
		return CEE_SUCCESS;
	
	retcode = EXECDIRECT("SET TRANSACTION AUTOCOMMIT OFF");
	
	if(retcode != CEE_SUCCESS)
	{
		retcode = EXECDIRECT("COMMIT WORK");
		retcode = EXECDIRECT("SET TRANSACTION AUTOCOMMIT OFF");
	}

	TEST_POINT( SET_AUTOCOMMIT_OFF, NULL )

	if(retcode != CEE_SUCCESS)
		return retcode;

	autoCommitSet = TRUE;	

	SRVRTRACE_EXIT(FILE_COMMON+18);

	return CEE_SUCCESS;
}

short SRVR::execDirectSQLQuery(SRVR_STMT_HDL *pSrvrStmt, char *pSqlStr,
							   odbc_SQLSvc_ExecuteN_exc_ *executeException)
{
	SRVRTRACE_ENTER(FILE_COMMON+19);

	short retcode = 0;

	if (pSrvrStmt == NULL)
	{
		executeException->exception_nr = odbc_SQLSvc_ExecDirect_SQLError_exn_;
		executeException->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_UNABLE_TO_ALLOCATE_SQL_STMT;
		return EXECUTE_EXCEPTION;
	}
	retcode = pSrvrStmt->ExecDirect(NULL, pSqlStr, EXTERNAL_STMT, TYPE_UNKNOWN, SQL_ASYNC_ENABLE_OFF, 0);

	if (retcode == SQL_ERROR)
	{
		executeException->exception_nr = odbc_SQLSvc_ExecDirect_SQLError_exn_;
		executeException->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
		executeException->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
		retcode = EXECUTE_EXCEPTION;
	}

	SRVRTRACE_EXIT(FILE_COMMON+19);

	return retcode;
}

BOOL SRVR::writeServerException( short retCode 
			,	odbc_SQLSvc_GetSQLCatalogs_exc_ *exception_
			,	odbc_SQLSvc_Prepare_exc_ *prepareException
			,	odbc_SQLSvc_ExecuteN_exc_ *executeException
			,	odbc_SQLSvc_FetchN_exc_ *fetchException)
{
	SRVRTRACE_ENTER(FILE_COMMON+20);

	switch (retCode) {
	case STMT_LABEL_NOT_FOUND:
		exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
		exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_SMD_STMT_LABEL_NOT_FOUND;
		if(diagnostic_flags){
				TraceOut(TR_SRVR, SQLSVC_EXCEPTION_SMD_STMT_LABEL_NOT_FOUND);
				} 	
		return FALSE;
	case DATA_TYPE_ERROR:
		exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
		exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_UNSUPPORTED_SMD_DATA_TYPE;
		if(diagnostic_flags){
				TraceOut(TR_SRVR, SQLSVC_EXCEPTION_UNSUPPORTED_SMD_DATA_TYPE);
				} 			
		return FALSE;
	case DATA_ERROR:
		exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
		exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_DATA_ERROR;
		return FALSE;
	case PROGRAM_ERROR:
		exception_->exception_nr = odbc_SQLSvc_Prepare_ParamError_exn_;
		exception_->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_PREPARE_FAILED;
		return FALSE;
	case PREPARE_EXCEPTION:
		switch (prepareException->exception_nr) {
		case CEE_SUCCESS:
			break;
		case odbc_SQLSvc_Prepare_SQLError_exn_:
			exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_SQLError_exn_;
			exception_->u.SQLError = prepareException->u.SQLError;
			return FALSE;
		case odbc_SQLSvc_Prepare_ParamError_exn_:
			exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
			exception_->u.ParamError.ParamDesc = prepareException->u.ParamError.ParamDesc;
			if(diagnostic_flags){
				TraceOut(TR_SRVR,"PREPARE_EXCEPTION: %s", prepareException->u.ParamError.ParamDesc);
				}			
			return FALSE;
		default:
			exception_->exception_nr = prepareException->exception_nr;
			exception_->exception_detail = prepareException->exception_detail;
			return FALSE;
		}
		break;
	case EXECUTE_EXCEPTION:
		switch (executeException->exception_nr) {
		case CEE_SUCCESS:
			break;
		case odbc_SQLSvc_ExecuteN_SQLError_exn_:
			exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_SQLError_exn_;
			exception_->u.SQLError = executeException->u.SQLError;
			return FALSE;
		case odbc_SQLSvc_ExecuteN_ParamError_exn_:
			exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
			exception_->u.ParamError.ParamDesc = executeException->u.ParamError.ParamDesc;
			if(diagnostic_flags){
				TraceOut(TR_SRVR,"EXECUTE_EXCEPTION: %s", executeException->u.ParamError.ParamDesc);
				}			
			return FALSE;
		case odbc_SQLSvc_ExecuteN_SQLInvalidHandle_exn_:
			exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_SQLInvalidHandle_exn_;
			return FALSE;
		default:
			exception_->exception_nr = executeException->exception_nr;
			exception_->exception_detail = executeException->exception_detail;
			return FALSE;
		}
		break;
	case FETCH_EXCEPTION:
		switch (fetchException->exception_nr) {
		case CEE_SUCCESS:
			break;
		case odbc_SQLSvc_FetchN_SQLNoDataFound_exn_:
			break;
		case odbc_SQLSvc_FetchN_SQLError_exn_:
			exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_SQLError_exn_;
			exception_->u.SQLError = fetchException->u.SQLError;
			return FALSE;
		case odbc_SQLSvc_FetchN_ParamError_exn_:
			exception_->exception_nr = odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_;
			exception_->u.ParamError.ParamDesc = fetchException->u.ParamError.ParamDesc;
			if(diagnostic_flags){
				TraceOut(TR_SRVR,"FETCH_EXCEPTION: %s", fetchException->u.ParamError.ParamDesc);
				}			
			return FALSE;
		default:
			exception_->exception_nr = fetchException->exception_nr;
			exception_->exception_detail = fetchException->exception_detail;
			return FALSE;
		}
		break;
	default:
		return FALSE;
		break;
	}
	SRVRTRACE_EXIT(FILE_COMMON+20);

	return TRUE;
}

void SRVR::getSessionId(char* sessionId)
{
	SRVRTRACE_ENTER(FILE_COMMON+21);

	char timeStamp[25];
	char currentTime[25];
	char procName[20];
	short pNameLen;
	short error;
	Int32 DSId;
	char ASName[9];
	
	char tmpString[50];
	getJulianTime(timeStamp);
	strcpy(currentTime,timeStamp);
    memset (procName, '\0', sizeof (procName));

	if ((error = PROCESSHANDLE_DECOMPOSE_(TPT_REF(srvrGlobal->nskProcessInfo.pHandle),
				 &srvrGlobal->nskProcessInfo.nodeId,
			     OMITREF, 
				 OMITREF, OMITREF,
			     OMITSHORT, OMITREF, procName, sizeof (procName),
			     &pNameLen, OMITREF)) != 0)
	{
		sprintf(tmpString, "%d", error);
		SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
			0, ODBCMX_SERVER, srvrGlobal->srvrObjRef, 
			1, tmpString);	
		return;
	}
	strncpy(ASName,srvrGlobal->ASProcessName, sizeof(ASName));
	ASName[sizeof(ASName)-1] = 0;
	DSId = srvrGlobal->DSId;
	sprintf(sessionId,"%0.20s:%d:%0.20s:%0.25s",ASName,DSId,procName,currentTime);

	SRVRTRACE_EXIT(FILE_COMMON+21);
}


void SRVR::getJulianTime(char* timestamp)
{
	SRVRTRACE_ENTER(FILE_COMMON+22);

	short currentTime[20];
	TIME(currentTime);
	snprintf(timestamp,25,"%d%d%d%d%d%d",currentTime[0],currentTime[1],currentTime[2],currentTime[3],currentTime[4],currentTime[5],currentTime[6]);
	SRVRTRACE_EXIT(FILE_COMMON+22);
}

void SRVR::getCurrentTime(char* timestamp)
{
	SRVRTRACE_ENTER(FILE_COMMON+23);

	short currentTime[20];
	TIME(currentTime);
	snprintf(timestamp,25,"%d/%d/%d %d:%d:%d.%d",currentTime[0],currentTime[1],currentTime[2],currentTime[3],currentTime[4],currentTime[5],currentTime[6]);
	SRVRTRACE_EXIT(FILE_COMMON+23);
}

short SRVR::do_ExecFetchSMD( 
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
  , /* Out   */ SQLItemDescList_def *outputDesc)
{
	SRVRTRACE_ENTER(FILE_COMMON+24);

	SRVR_STMT_HDL		*pSrvrStmt;
	SQLItemDesc_def		*SQLItemDesc;
	IDL_unsigned_long	curParamNo;
	Int32				allocLength;
	Int32				retcode;
	SQLRETURN			rc;
	IDL_short			indValue;
	BOOL				tableParamDone;
	IDL_unsigned_long	index;
	IDL_long			tempRowsAffected = 0;
	Int32				estLength=0;
	Int32				estRowLength=0;
	Int32				initestTotalLength=0;
	Int32				prevestTotalLength=0;
	SRVR_STMT_HDL		*pPrevSrvrStmt;

	if (previousstmtLabel[0] != '\0')
	{
		pPrevSrvrStmt = getSrvrStmt(previousstmtLabel, FALSE);
 		if (pPrevSrvrStmt == NULL)
		{
			executeException->exception_nr = odbc_SQLSvc_ExecuteN_ParamError_exn_;
			executeException->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_UNABLE_TO_ALLOCATE_SQL_STMT;
			return EXECUTE_EXCEPTION;
		}


		if (pPrevSrvrStmt->outputDataValue._length != 0)
		{
			for (Int32 curColumnNo = 0; curColumnNo < pPrevSrvrStmt->columnCount ; curColumnNo++)
			{
				SRVR_DESC_HDL *IRD;

				IRD = pPrevSrvrStmt->IRD;
				estLength = getAllocLength(IRD[curColumnNo].dataType, IRD[curColumnNo].length);
				estLength += 1;
				estRowLength += estLength;
			}
			initestTotalLength = estRowLength * 100;
			prevestTotalLength = ((pPrevSrvrStmt->outputDataValue._length / initestTotalLength) + 1) * initestTotalLength;
		}
		tempRowsAffected = pPrevSrvrStmt->rowsAffected;
	}

	
	if (strncmp(stmtLabel, "SQL_GETTYPEINFO",15) == 0)
		pSrvrStmt = getSrvrStmt(stmtLabel, TRUE, "HP_SYSTEM_CATALOG.MXCS_SCHEMA.CATANSIMXGTI", 0, 1234567890);
	else if (strncmp(stmtLabel, "SQL_JAVA_",9) == 0)
		pSrvrStmt = getSrvrStmt(stmtLabel, TRUE, "HP_SYSTEM_CATALOG.MXCS_SCHEMA.CATANSIMXJAVA", 0, 1234567890);
	else
		pSrvrStmt = getSrvrStmt(stmtLabel, TRUE, "HP_SYSTEM_CATALOG.MXCS_SCHEMA.CATANSIMX", 0, 1234567890);
 	if (pSrvrStmt == NULL)
	{
		executeException->exception_nr = odbc_SQLSvc_ExecuteN_ParamError_exn_;
		executeException->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_UNABLE_TO_ALLOCATE_SQL_STMT;
		return EXECUTE_EXCEPTION;
	}


	if (pSrvrStmt->rowsAffected != -1)
		tempRowsAffected += pSrvrStmt->rowsAffected;

	rc = pSrvrStmt->PrepareFromModule(INTERNAL_STMT);
	if (rc == SQL_ERROR)
	{
		executeException->exception_nr = odbc_SQLSvc_ExecuteN_ParamError_exn_;
		executeException->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_CATSMD_MODULE_ERROR;
		return EXECUTE_EXCEPTION;
	}
	if ((rc = AllocAssignValueBuffer(pSrvrStmt->bSQLValueListSet,&pSrvrStmt->inputDescList, 
		&pSrvrStmt->inputValueList, pSrvrStmt->inputDescVarBufferLen, 1, 
		pSrvrStmt->inputValueVarBuffer)) != SQL_SUCCESS)
	{
		executeException->exception_nr = odbc_SQLSvc_ExecuteN_ParamError_exn_;
		executeException->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_BUFFER_ALLOC_FAILED;
		return EXECUTE_EXCEPTION;
	}


	pSrvrStmt->InternalStmtClose(SQL_CLOSE);
	outputDesc->_length = pSrvrStmt->outputDescList._length;
	outputDesc->_buffer = pSrvrStmt->outputDescList._buffer;
	
	for (curParamNo = 0, index = 0, tableParamDone = FALSE, pSrvrStmt->inputValueList._length = 0; 
			curParamNo < pSrvrStmt->inputDescList._length ; curParamNo++, index++) 
	{
		SQLItemDesc = (SQLItemDesc_def *)pSrvrStmt->inputDescList._buffer + curParamNo;
		allocLength = getAllocLength(SQLItemDesc->dataType, SQLItemDesc->maxLen);
		if (! tableParamDone)
		{
			if (tableParam[index] == NULL)
			{
				tableParamDone = TRUE;
				index = 0;
			}
			else
			{
				retcode = kdsCopyToSMDSQLValueSeq(&pSrvrStmt->inputValueList, 
					SQLItemDesc->dataType, 0, tableParam[index], allocLength, SQLItemDesc->ODBCCharset);
			}
		}
		if (tableParamDone) 
		{
			if  (inputParam[index] == NULL)
				indValue = -1;
			else
				indValue = 0;
			retcode = kdsCopyToSMDSQLValueSeq(&pSrvrStmt->inputValueList, 
					SQLItemDesc->dataType, indValue, inputParam[index], allocLength, SQLItemDesc->ODBCCharset);
		}
		if (retcode != 0)
			return ((short) retcode);
	}
	executeException->exception_nr = 0;

	// sqlStmtType has value of types like TYPE_SELECT, TYPE_DELETE etc.
	odbc_SQLSvc_ExecuteN_sme_(objtag_, call_id_, executeException, dialogueId, stmtLabel, 
			(IDL_string)stmtLabel, 
			sqlStmtType, 1, &pSrvrStmt->inputValueList, SQL_ASYNC_ENABLE_OFF, 0, rowsAffected, sqlWarning);
	if (executeException->exception_nr != CEE_SUCCESS)
		return EXECUTE_EXCEPTION;
	
	if (previousstmtLabel[0] != '\0')
	{
		if (pSrvrStmt->outputDataValue._buffer != NULL)
			delete pSrvrStmt->outputDataValue._buffer;
		pSrvrStmt->outputDataValue._length = 0;
		pSrvrStmt->outputDataValue._buffer = NULL;
		if (pPrevSrvrStmt->outputDataValue._length != 0)
		{
			markNewOperator,pSrvrStmt->outputDataValue._buffer = new BYTE[prevestTotalLength + 1];
			if (pSrvrStmt->outputDataValue._buffer == 0)
			{
				// Handle Memory Overflow execption here
				SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
							srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
							srvrGlobal->srvrObjRef, 1, "_buffer:FETCHCATALOGPERF");
				exit(0);
			}
			memset(pSrvrStmt->outputDataValue._buffer,0,prevestTotalLength + 1);
			pSrvrStmt->outputDataValue._length = pPrevSrvrStmt->outputDataValue._length;
			memcpy(pSrvrStmt->outputDataValue._buffer,pPrevSrvrStmt->outputDataValue._buffer,pPrevSrvrStmt->outputDataValue._length);
			pPrevSrvrStmt->InternalStmtClose(SQL_CLOSE);
		}
		if (pPrevSrvrStmt->outputDataValue._buffer != NULL)
			delete pPrevSrvrStmt->outputDataValue._buffer;
		pPrevSrvrStmt->outputDataValue._length = 0;
		pPrevSrvrStmt->outputDataValue._buffer = NULL;
	}

	pSrvrStmt->rowsAffected = tempRowsAffected;

	rc = pSrvrStmt->FetchCatalogRowset(100, 0, SQL_ASYNC_ENABLE_OFF, 0, resetValues);
	switch (rc)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
		executeException->exception_nr = 0;
		*rowsAffected = pSrvrStmt->rowsAffected;
		sqlWarning->_length = pSrvrStmt->sqlWarning._length;
		sqlWarning->_buffer = pSrvrStmt->sqlWarning._buffer;
		break;
	case SQL_STILL_EXECUTING:
		executeException->exception_nr = odbc_SQLSvc_FetchRowset_SQLStillExecuting_exn_;
		break;
	case SQL_INVALID_HANDLE:
		executeException->exception_nr = odbc_SQLSvc_FetchRowset_SQLInvalidHandle_exn_;
		break;
	case SQL_NO_DATA_FOUND:
		executeException->exception_nr = odbc_SQLSvc_FetchRowset_SQLNoDataFound_exn_;
		break;
	case SQL_ERROR:
		ERROR_DESC_def *error_desc_def;
		error_desc_def = pSrvrStmt->sqlError.errorList._buffer;
		if (pSrvrStmt->sqlError.errorList._length != 0 && 
			(error_desc_def->sqlcode == -8007 || error_desc_def->sqlcode == -8007))
		{
			executeException->exception_nr = odbc_SQLSvc_FetchRowset_SQLQueryCancelled_exn_;
			executeException->u.SQLQueryCancelled.sqlcode = error_desc_def->sqlcode;
		}
		else
		{
			executeException->exception_nr = odbc_SQLSvc_FetchRowset_SQLError_exn_;
			executeException->u.SQLError.errorList._length = pSrvrStmt->sqlError.errorList._length;
			executeException->u.SQLError.errorList._buffer = pSrvrStmt->sqlError.errorList._buffer;
		}
		break;
	case PROGRAM_ERROR:
		executeException->exception_nr = odbc_SQLSvc_FetchRowset_ParamError_exn_;
		executeException->u.ParamError.ParamDesc = SQLSVC_EXCEPTION_FETCH_FAILED;
	default:
		break;
	}

	if (executeException->exception_nr != 0 && executeException->exception_nr != odbc_SQLSvc_FetchRowset_SQLNoDataFound_exn_)
	{
		if (pSrvrStmt->outputDataValue._buffer != NULL)
			delete pSrvrStmt->outputDataValue._buffer;
		pSrvrStmt->outputDataValue._length = 0;
		pSrvrStmt->outputDataValue._buffer = NULL;
		pSrvrStmt->rowsAffected = 0;
	}

	SRVRTRACE_EXIT(FILE_COMMON+24);

	return 0;
}

// DO NOT call this function using pSrvrStmt->sqlWarningOrErrorLength and pSrvrStmt->sqlWarningOrError,
// Since the WarningOrError is static and pSrvrStmt->sqlWarningOrError will deallocate this memory. 
extern "C" void GETMXCSWARNINGORERROR(
   /* In    */ Int32 sqlcode
  , /* In    */ char *sqlState
  , /* In    */ char *msg_buf
  , /* Out   */ Int32 *MXCSWarningOrErrorLength
  , /* Out   */ BYTE *&MXCSWarningOrError)
{
	Int32 total_conds = 1;
	Int32 buf_len;
	Int32 curr_cond = 1;
	Int32 msg_buf_len = strlen(msg_buf)+1;
	Int32 time_and_msg_buf_len = 0;
	Int32 msg_total_len = 0;
	Int32 rowId = 0; // use this for rowset recovery.
	char tsqlState[6];
	static BYTE WarningOrError[1024];
	char  strNow[TIMEBUFSIZE + 1];
	char* time_and_msg_buf = NULL;

	memset(tsqlState,0,sizeof(tsqlState));
	memcpy(tsqlState,sqlState,sizeof(tsqlState)-1);
	
	bzero(WarningOrError,sizeof(WarningOrError));

	*MXCSWarningOrErrorLength = 0;
	MXCSWarningOrError = WarningOrError; // Size of internally generated message should be enough

	*(Int32 *)(MXCSWarningOrError+msg_total_len) = total_conds;
	msg_total_len += sizeof(total_conds);
	*(Int32 *)(MXCSWarningOrError+msg_total_len) = rowId;
	msg_total_len += sizeof(rowId);
	*(Int32 *)(MXCSWarningOrError+msg_total_len) = sqlcode;
	msg_total_len += sizeof(sqlcode);
	time_and_msg_buf_len   = msg_buf_len + TIMEBUFSIZE;
	*(Int32 *)(MXCSWarningOrError+msg_total_len) = time_and_msg_buf_len;
	msg_total_len += sizeof(time_and_msg_buf_len);
	//Get the timetsamp
	time_and_msg_buf = new char[time_and_msg_buf_len];
	strncpy(time_and_msg_buf, msg_buf, msg_buf_len);
	time_t  now = time(NULL);
	bzero(strNow, sizeof(strNow));
	strftime(strNow, sizeof(strNow), " [%Y-%m-%d %H:%M:%S]", localtime(&now));
	strcat(time_and_msg_buf, strNow);
	memcpy(MXCSWarningOrError+msg_total_len, time_and_msg_buf, time_and_msg_buf_len);
	msg_total_len += time_and_msg_buf_len;
	delete time_and_msg_buf;
	memcpy(MXCSWarningOrError+msg_total_len, tsqlState, sizeof(tsqlState));
	msg_total_len += sizeof(tsqlState);
	*MXCSWarningOrErrorLength = msg_total_len;
	return;
}

void SRVR::SETSRVRERROR(Int32 srvrErrorCodeType, Int32 srvrErrorCode, char *srvrSqlState, char* srvrErrorTxt, ERROR_DESC_LIST_def *srvrErrorList)
{
	Int32 len=0;
	ERROR_DESC_def *srvrError = NULL;
	markNewOperator,srvrError = new ERROR_DESC_def;
	if (srvrError == NULL)
	{
		// Handle Memory Overflow execption here
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "SETSRVRERROR");
		exit(0);
	}
	memset(srvrError, 0, sizeof(ERROR_DESC_def));

	srvrError->errorCodeType = srvrErrorCodeType;
	srvrError->sqlcode = srvrErrorCode;
	memset(srvrError->sqlstate,0,sizeof(SQLSTATE_def));
	if (srvrSqlState != NULL)
	{
		len = strlen(srvrSqlState);
		if (len > sizeof(SQLSTATE_def)) len = sizeof(SQLSTATE_def)-1;		
		memcpy(srvrError->sqlstate, srvrSqlState, len);
		srvrError->sqlstate[5] = '\0';
	}

	if (srvrErrorTxt != NULL)
	{
		len = strlen(srvrErrorTxt);
		markNewOperator,srvrError->errorText = new char [len+1];
		if (srvrError->errorText == NULL)
		{
			// Handle Memory Overflow execption here
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
						srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
						srvrGlobal->srvrObjRef, 1, "SETSRVRERROR");
			exit(0);
		}
		strcpy(srvrError->errorText, srvrErrorTxt);
	}

	srvrErrorList->_length = 1;
	srvrErrorList->_buffer = srvrError;


	return;
}

// #ifdef _TMP_SQ_SECURITY
void SRVR::SETSECURITYERROR(short securityErrorCode, ERROR_DESC_LIST_def *outError)
{
	char errorMsg[128];

	pwd_get_errortext (securityErrorCode, errorMsg, sizeof(errorMsg));
	SETSRVRERROR(SECURITYERR, securityErrorCode, SECSERV_SQLSTATE_ID, errorMsg, outError);

	return;
}
// #endif

SQLRETURN SRVR::doInfoStats(SRVR_STMT_HDL *pSrvrStmt)
{
	Int32 retcode = SQL_SUCCESS;
	char stmtName[MAX_STMT_NAME_LEN+1];
	short stmtNamelen = 0;
	short quotedStrlen = 0;
	char *openQuote = NULL;
	char *closeQuote = NULL;
	char *temp = NULL;
	char *sqlString = NULL;
	Int32 i = 0;
	char errorMsg[256];
	SRVR_STMT_HDL *pInfoStatsStmt = NULL;
	SRVR_STMT_HDL *pPrepareStmt = NULL;
	Int32 sqlWarningOrErrorLength = 0;
	BYTE *sqlWarningOrError = NULL;

	i = _max(pSrvrStmt->sqlStringLen, 1024);
	sqlString  = new char[i+1];
	if (sqlString == NULL)
	{
		SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
					srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
					srvrGlobal->srvrObjRef, 1, "EXECDIRECT_InfoStats");
		exit(0);
	}

	bzero(sqlString,i+1);
	strcpy(sqlString, pSrvrStmt->sqlString);
	bzero(stmtName,sizeof(stmtName));

	char quote;
	char* psqlString = sqlString;
	while (isspace(*psqlString))psqlString++;
	openQuote = psqlString;
	while (*openQuote != 0 && !isspace(*openQuote))openQuote++;
	while (*openQuote != 0 && isspace(*openQuote))openQuote++;
	if (*openQuote == '\"' || *openQuote == '\'') quote = *openQuote;
	else openQuote = NULL;

	// syntax checking - check for quotes
//	openQuote = strchr(pSrvrStmt->sqlString, quotes); // check for open quote
	if (openQuote != NULL)
	{
//		closeQuote = strrchr(pSrvrStmt->sqlString, '\"'); // check for close quote
		closeQuote = strrchr(psqlString, quote); // check for close quote
		if (closeQuote == openQuote || closeQuote == openQuote+1)
		{
			strcpy(errorMsg, "Syntax error - no open or closing quote");
			retcode = INFOSTATS_SYNTAX_ERROR;
		}
		else
		{
			quotedStrlen = closeQuote - openQuote -1;

			strncpy(sqlString, openQuote+1, quotedStrlen);
			sqlString[quotedStrlen] = '\0';
			trim(sqlString);

			temp = strchr(sqlString, ' ');
			if (temp == NULL)
			{
				// if no spaces in sqlString, it must be a stmtName
				strcpy(stmtName, sqlString);
				sqlString[0] = '\0';
			}
			else
			{
				// sql query is provided (i.e. INFOSTATS "select * from t1")
				stmtName[0] = '\0';
			}
		}
	}
	else
	{
		// no quotes in pSrvrStmt->sqlString - must have stmtName after INFOSTATS
		strcpy(stmtName, pSrvrStmt->sqlString+9);
		trim(stmtName);
		temp = strchr(stmtName, ' ');
		if (temp != NULL)
		{
			strcpy(errorMsg, "Syntax error - cannot have space in the stmtName");
			retcode = INFOSTATS_SYNTAX_ERROR; // cannot have space in the stmtName
		}
		else
		{
			if (strlen(stmtName) > MAX_STMT_NAME_LEN)
			{
				sprintf(errorMsg, "Syntax error - stmtName length exceeds maxlength: %d", MAX_STMT_NAME_LEN);
				retcode = INFOSTATS_SYNTAX_ERROR; // cannot have space in the stmtName
			}
		}
		sqlString[0] = '\0';
	}
	// stmtName should be set now

	if(retcode == SQL_SUCCESS)
	{
		if (stmtName[0] == '\0' && sqlString[0] != '\0')
		{
			strcpy(stmtName, "STMT_INFOSTATS");
			// allocate a srvr stmt handle and prepare sql stmt
			if ((pPrepareStmt = getSrvrStmt(stmtName, TRUE)) == NULL)
			{
				sprintf(errorMsg, "Invalid statement/cursor name %s is specified", stmtName);
				retcode = INFOSTATS_STMT_NOT_FOUND;
			}		
			else
			{
				// cleanup all memory allocated in the previous operations
				pPrepareStmt->cleanupAll();
				pPrepareStmt->sqlStringLen = strlen(sqlString);
				pPrepareStmt->sqlString  = new char[pPrepareStmt->sqlStringLen+1];
				if (pPrepareStmt->sqlString == NULL)
				{
					SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
								srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
								srvrGlobal->srvrObjRef, 1, "EXECDIRECT_InfoStats");
					exit(0);
				}

				strcpy(pPrepareStmt->sqlString, sqlString);
				retcode = PREPARE2(pPrepareStmt);
				if (retcode != SQL_SUCCESS_WITH_INFO && retcode != SQL_SUCCESS && retcode != ODBC_RG_WARNING)
				{
					strcpy(errorMsg, "Syntax error");
					retcode = INFOSTATS_SYNTAX_ERROR;
				}
			}
		}
		else if (stmtName[0] == 0)
		{
			strcpy(errorMsg, "Syntax error");
			retcode = INFOSTATS_SYNTAX_ERROR;
		}

		if (retcode == SQL_SUCCESS)
		{
			if ((pInfoStatsStmt = getSrvrStmtByCursorName(stmtName, FALSE)) == NULL)
			{
				// stmtName not found
				sprintf(errorMsg, "Invalid statement/cursor name %s is specified", stmtName);
				retcode = INFOSTATS_STMT_NOT_FOUND;
			}
			else
			{
				if (pInfoStatsStmt == pSrvrStmt)
				{
					// cannot do INFOSTATS on itself
					sprintf(errorMsg, "INFOSTATS on current statement/cursor name %s is not allowed", stmtName);
					retcode = INFOSTATS_STMT_NOT_FOUND;
				}
			}

			if (retcode == SQL_SUCCESS)
			{
				sprintf(sqlString, "SELECT \"QueryID\", \"CPUTime\", \"IOTime\", \"MsgTime\", \"IdleTime\", \"TotalTime\", \"Cardinality\", \"ResourceUsage\", \"AffinityNumber\", \"Dop\", \"XnNeeded\", \"MandatoryCrossProduct\", \"MissingStats\", \"NumOfJoins\", \"FullScanOnTable\", \"HighDp2MxBufferUsage\", \"RowsAccessedForFullScan\", \"Dp2RowsAccessed\", \"Dp2RowsUsed\" FROM (VALUES (\'%s\', %g, %g, %g, %g, %g, %g, %d, %u, %d, %d, %d, %d, %d, %d, %d, %g, %g, %g )) INFOSTATS_TAB (\"QueryID\", \"CPUTime\", \"IOTime\", \"MsgTime\", \"IdleTime\", \"TotalTime\", \"Cardinality\", \"ResourceUsage\", \"AffinityNumber\", \"Dop\", \"XnNeeded\", \"MandatoryCrossProduct\", \"MissingStats\", \"NumOfJoins\", \"FullScanOnTable\", \"HighDp2MxBufferUsage\", \"RowsAccessedForFullScan\", \"Dp2RowsAccessed\", \"Dp2RowsUsed\")",
			pInfoStatsStmt->sqlUniqueQueryID, pInfoStatsStmt->cost_info.cpuTime, pInfoStatsStmt->cost_info.ioTime, 
			pInfoStatsStmt->cost_info.msgTime, pInfoStatsStmt->cost_info.idleTime, pInfoStatsStmt->cost_info.totalTime,
                           pInfoStatsStmt->cost_info.cardinality,pInfoStatsStmt->cost_info.resourceUsage,
                           pInfoStatsStmt->comp_stats_info.affinityNumber,
                           pInfoStatsStmt->comp_stats_info.dop,
                           pInfoStatsStmt->comp_stats_info.xnNeeded,
                           pInfoStatsStmt->comp_stats_info.mandatoryCrossProduct,
                           pInfoStatsStmt->comp_stats_info.missingStats,
                           pInfoStatsStmt->comp_stats_info.numOfJoins,
                           pInfoStatsStmt->comp_stats_info.fullScanOnTable,
                           pInfoStatsStmt->comp_stats_info.highDp2MxBufferUsage,
                           pInfoStatsStmt->comp_stats_info.rowsAccessedForFullScan,
                           pInfoStatsStmt->comp_stats_info.dp2RowsAccessed,
                           pInfoStatsStmt->comp_stats_info.dp2RowsUsed);

				if (pSrvrStmt->sqlString != NULL)
				{
					delete pSrvrStmt->sqlString;
					pSrvrStmt->sqlStringLen = 0;
				}
				pSrvrStmt->sqlStringLen = strlen(sqlString);
				pSrvrStmt->sqlString  = new char[pSrvrStmt->sqlStringLen+1];
				if (pSrvrStmt->sqlString == NULL)
				{
					SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
								srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
								srvrGlobal->srvrObjRef, 1, "EXECDIRECT_InfoStats");
					exit(0);
				}
				strcpy(pSrvrStmt->sqlString, sqlString);
				pSrvrStmt->sqlStmtType = TYPE_SELECT;
			}
		}
	}

	if (retcode != SQL_SUCCESS)
	{
		SETSRVRERROR(INFOSTATSERR, -1, "HY000", errorMsg, &pSrvrStmt->sqlError.errorList);
		// DO NOT call this function using pSrvrStmt->sqlWarningOrErrorLength and pSrvrStmt->sqlWarningOrError,
		// Since the WarningOrError is static and pSrvrStmt->sqlWarningOrError will deallocate this memory. 
		GETMXCSWARNINGORERROR(-1, "HY000", errorMsg, &sqlWarningOrErrorLength, sqlWarningOrError);
		markNewOperator,pSrvrStmt->sqlWarningOrError = new BYTE[sqlWarningOrErrorLength]; // Size of internally generated message should be enough
		if (pSrvrStmt->sqlWarningOrError == NULL)
		{
			// Handle Memory Overflow execption here
			SendEventMsg(MSG_MEMORY_ALLOCATION_ERROR, EVENTLOG_ERROR_TYPE,
						srvrGlobal->nskProcessInfo.processId, ODBCMX_SERVER, 
						srvrGlobal->srvrObjRef, 1, "EXECDIRECT_InfoStats");
			exit(0);
		}
		pSrvrStmt->sqlWarningOrErrorLength = sqlWarningOrErrorLength;
		bzero(pSrvrStmt->sqlWarningOrError, sqlWarningOrErrorLength);
		memcpy(pSrvrStmt->sqlWarningOrError, sqlWarningOrError, sqlWarningOrErrorLength);
	}
	if (sqlString != NULL)
		delete [] sqlString;

	return retcode;
}

char *SRVR::getCharsetStr(Int32 charset)
{
	switch (charset)
	{
	case SQLCHARSETCODE_ISO88591:
		return SQLCHARSETSTRING_ISO88591;
	case SQLCHARSETCODE_UTF8:
		return SQLCHARSETSTRING_UTF8;
	case SQLCHARSETCODE_UCS2:
		return SQLCHARSETSTRING_UNICODE;
	case SQLCHARSETCODE_SJIS:
		return SQLCHARSETSTRING_SJIS;
	case SQLCHARSETCODE_EUCJP:
		return SQLCHARSETSTRING_EUCJP;
	case SQLCHARSETCODE_KSC5601:
		return SQLCHARSETSTRING_KSC5601;
	case SQLCHARSETCODE_BIG5:
		return SQLCHARSETSTRING_BIG5;
	case SQLCHARSETCODE_GB2312:
		return SQLCHARSETSTRING_GB2312;
	case SQLCHARSETCODE_GB18030:
		return SQLCHARSETSTRING_GB18030;
	default:
		return SQLCHARSETSTRING_ISO88591;
	}
}

// translates inStr in inCharset encoding to UTF8 string and writes it in outStr
// UTF8 string is written in outStr upto outStrMax
// if for some reason, translation fails, inStr is simply copied to outStr
void SRVR::translateToUTF8(Int32 inCharset, char* inStr, Int32 inStrLen, char* outStr, Int32 outStrMax)
{
	Int32 err=0;
	Int32 charset = SQLCHARSETCODE_UNKNOWN;
	Int32 length = _min (inStrLen, outStrMax);
	Int32 outStrLen = 0;
	void *firstUntranslatedChar = 0;
	unsigned int translatedCharCnt = 0;

	if (inStr == NULL || length <= 0)
	{
		if (outStr != NULL) outStr[0] = '\0';		
		return;
	}

	switch (inCharset)
	{
	case SQLCHARSETCODE_ISO88591:
		charset = SQLCONVCHARSETCODE_ISO88591;
		break;
	case SQLCHARSETCODE_KSC5601:
	case SQLCHARSETCODE_MB_KSC5601:
		charset = SQLCONVCHARSETCODE_KSC;
		break;
	case SQLCHARSETCODE_SJIS:
		charset = SQLCONVCHARSETCODE_SJIS;
		break;
	case SQLCHARSETCODE_UCS2:
		charset = SQLCONVCHARSETCODE_UTF16;
		break;
	case SQLCHARSETCODE_EUCJP:
		charset = SQLCONVCHARSETCODE_EUCJP;
		break;
	case SQLCHARSETCODE_BIG5:
		charset = SQLCONVCHARSETCODE_BIG5;
		break;
	case SQLCHARSETCODE_GB18030:
		charset = SQLCONVCHARSETCODE_GB18030;
		break;
	case SQLCHARSETCODE_UTF8:
		charset = SQLCONVCHARSETCODE_UTF8;
		break;
	case SQLCHARSETCODE_GB2312:
		charset = SQLCONVCHARSETCODE_BIG5;
		break;
	default:
		charset = SQLCONVCHARSETCODE_UNKNOWN;
		break;
	};

	if (charset != SQLCONVCHARSETCODE_UNKNOWN)
	{
		err = SQL_EXEC_LocaleToUTF8 (charset,
									 inStr,
									 inStrLen,
									 outStr,
									 outStrMax,
									 &firstUntranslatedChar,
									 &outStrLen,
									 TRUE,
									 (Int32*)&translatedCharCnt); //for 64-bit the Executor is going to change it to a int
		if (err != 0)
		{
			if (err == -2 && outStrMax <= outStrLen) // CNV_ERR_BUFFER_OVERRUN
			{
				outStr[outStrMax-1]= '\0';
			}
			else
			{
				memcpy(outStr, inStr, length-1);
				outStr[length-1] = '\0';
			}
		}
	}
	else
	{
		memcpy(outStr, inStr, length-1);
		outStr[length-1] = '\0';
	}

	return;
}
