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

// it was picking up the limits in windows directory.  should probably rename that file and not pick it up
// WHY IS THIS NOT WORKIGN
#include <limits.h>
#include "process.h"
#include "cstmt.h"
#include "mxomsg.h"
#include "drvrnet.h"
#include "DrvrSrvr.h"
#include "drvrglobal.h"
#include "nskieee.h"
#include "sqlcli.h"
#include "nskieee.h"
#include "sqlcli.h"
#include "swap.h"
#include "csconvert.h"
#include "diagfunctions.h" //10-080124-0030 

#include <asyncIO.h>
#include <assert.h>

using namespace ODBC;

// Implements the member functions of CStmt

CStmt::CStmt(SQLHANDLE InputHandle) : CHandle(SQL_HANDLE_STMT, InputHandle)
{
	m_ThreadStatus	= SQL_SUCCESS;
	m_StmtEvent		= NULL;

	m_StmtName[0]	= '\0';
	m_CursorName[0] = '\0';
	sprintf(m_StmtLabel, "SQL_CUR_%ld", m_HandleNumber);
	strcpy(m_StmtLabelOrg, m_StmtLabel);
	m_ConnectHandle = (CConnect *)m_InputHandle;
	m_ICUConv = m_ConnectHandle->m_ICUConv;
	m_ARDDesc = NULL;
	m_IRDDesc = NULL;
	m_APDDesc = NULL;
	m_IPDDesc = NULL;

	m_AppParamDesc = m_APDDesc;
	m_AppRowDesc = m_ARDDesc;
	m_ImpParamDesc = m_IPDDesc;
	m_ImpRowDesc = m_IRDDesc;
	
	m_AsyncEnable = m_ConnectHandle->m_AsyncEnable;
	m_Concurrency =  m_ConnectHandle->m_Concurrency;
	m_CursorType = m_ConnectHandle->m_CursorType;
	m_CursorScrollable = SQL_NONSCROLLABLE;
	m_CursorSensitivity = SQL_UNSPECIFIED;
	m_EnableAutoIPD = m_ConnectHandle->m_AutoIPD;
	m_FetchBookmarkPtr = NULL;
	m_CursorHoldable = SQL_NONHOLDABLE; // non-holdable is default
	m_KeysetSize = SQL_KEYSET_SIZE_DEFAULT;
	m_MaxLength = m_ConnectHandle->m_MaxLength;
	m_MaxRows = m_ConnectHandle->m_MaxRows; // Return all rows
	m_MetadataId = m_ConnectHandle->m_MetadataId;
	m_Noscan = m_ConnectHandle->m_Noscan;
	m_QueryTimeout = m_ConnectHandle->m_QueryTimeout;
	m_RetrieveData = SQL_RD_DEFAULT;
	m_RowNumber = 0;
	m_SimulateCursor = m_ConnectHandle->m_SimulateCursor;
	m_UseBookmarks = m_ConnectHandle->m_UseBookmarks;
	m_CurrentOdbcAPI = SQL_API_SQLALLOCHANDLE;
	m_CurrentParam = -1;
	m_CurrentRow = -1;
	m_CurrentParamStatus = 0;
	m_InputValueList._buffer = 0;
	m_InputValueList._length = 0;
	m_DataAtExecData.dataValue._length = 0;
	m_DataAtExecData.dataValue._buffer = NULL;
	m_DataAtExecDataBufferSize = 0;
	m_ParamBuffer = NULL;
	m_RowsFetched = -1;
	m_CurrentRowFetched = 0;
	m_CurrentRowInRowset = 0;
	m_RowsetSize = 0;
	m_ResultsetRowsFetched = 0;
	m_NumResultCols = 0;
	m_NumParams = 0;
	m_StmtState	= STMT_ALLOCATED;
	m_StmtPrepared	= FALSE;
	EnterCriticalSection2(&m_ConnectHandle->m_CSObject);
	m_ConnectHandle->m_StmtCollect.push_back(this);
	LeaveCriticalSection2(&m_ConnectHandle->m_CSObject);
	m_RowStatusArray = NULL;
	m_CurrentFetchType = 0;
		
	m_OutputDataValue = NULL;
	m_FetchDataValue.numberOfElements = 0;
	m_FetchDataValue.numberOfRows = 0;
	m_FetchDataValue.rowAddress = NULL;
	m_SelectRowsets = m_ConnectHandle->m_SelectRowsets;
	m_FetchBufferSize = m_ConnectHandle->m_FetchBufferSize;

	m_RowCount = -1;
	m_CatalogName[0] = '\0';
	m_SchemaName[0] = '\0';
	m_TableName[0] = '\0';
	m_ColumnName[0] = '\0';
	m_TableType[0] = '\0';
	m_FKCatalogName[0] = '\0';
	m_FKSchemaName[0] = '\0';
	m_FKTableName[0] = '\0';
	m_outputDataValue._length = 0;
	m_outputDataValue._buffer = 0;

	m_StmtQueryType = 0;
	m_StmtHandle = 0;
	m_APIDecision = true;
	m_InputParams = NULL;
	m_OutputColumns = NULL;

	m_isClosed = false;

	m_BT = false;
	m_WMStmtPrepared = false;
	m_token = WM_INIT;

	m_ColumnIndexes = NULL;
	m_SwapInfo =  NULL;
	m_SwapInfo_NumRows = 0;

	m_spjNumResultSets = 0;
	m_spjResultSets = NULL;
	m_spjCurrentResultSetIndex = 0;
	m_spjInputParamDesc._length = 0;
	m_spjInputParamDesc._buffer = NULL;
	m_spjOutputParamDesc._length = 0;
	m_spjOutputParamDesc._buffer = NULL;

	//#ifdef ASYNCIO
	m_StmtExecState = STMT_EXECUTION_NONE;
	m_AsyncCanceled = FALSE;
	//#endif

}


CStmt::~CStmt()
{
	std::list<CHandlePtr>::iterator i;
	CStmt* pStmt;
	//Close(SQL_DROP);
	clearError();
	if (m_ARDDesc != NULL)
		delete m_ARDDesc;
	if (m_IRDDesc != NULL)
		delete m_IRDDesc;
	if (m_APDDesc != NULL)
		delete m_APDDesc;
	if (m_IPDDesc != NULL)
		delete m_IPDDesc;
	if (m_StmtEvent != NULL)
		CloseHandle(m_StmtEvent);
	if (m_InputValueList._buffer != NULL)
#ifndef unixcli
		delete m_InputValueList._buffer;
#else
		delete []m_InputValueList._buffer;
#endif
	if (m_FetchDataValue.rowAddress != NULL )
		delete[] m_FetchDataValue.rowAddress;
	if (m_outputDataValue._length != 0 &&
		m_outputDataValue._buffer != NULL)
		delete[] m_outputDataValue._buffer;
	if (m_ParamBuffer != NULL)
 		delete []m_ParamBuffer;
	if (m_InputParams != NULL)
		delete []m_InputParams;
	if (m_OutputColumns != NULL)
		delete m_OutputColumns;

	if (m_ColumnIndexes != NULL)
		delete[] m_ColumnIndexes;

	if (m_SwapInfo != NULL)
	{
		for (int r = 0; r < m_SwapInfo_NumRows; r++) 
		{
			if (m_SwapInfo[r] != NULL)
				delete[] (m_SwapInfo[r]);
		}
		delete[] m_SwapInfo;
		m_SwapInfo = NULL;
		m_SwapInfo_NumRows = 0;
	}

	if (m_spjResultSets != NULL)
	   delete[] m_spjResultSets;

	if( m_spjInputParamDesc._length != 0 &&
		m_spjInputParamDesc._buffer != NULL)
		delete [] m_spjInputParamDesc._buffer;

	if( m_spjOutputParamDesc._length != 0 &&
		m_spjOutputParamDesc._buffer != NULL)
		delete [] m_spjOutputParamDesc._buffer;


	// Remove this from StmtCollection in Connection
	EnterCriticalSection2(&m_ConnectHandle->m_CSObject);

	for (i = m_ConnectHandle->m_StmtCollect.begin() ; i !=  m_ConnectHandle->m_StmtCollect.end() ; ++i)
	{
		pStmt = (CStmt*)(*i); 
		if ((pStmt != NULL ) && (pStmt == this))
		{
			m_ConnectHandle->m_StmtCollect.erase(i);
			break;
		}
	}

	LeaveCriticalSection2(&m_ConnectHandle->m_CSObject);

}

void CStmt::InitInputValueList()
{
	if (m_ParamBuffer != NULL)
		delete []m_ParamBuffer;
	m_ParamBuffer = NULL;
	if (m_InputValueList._buffer != NULL)
#ifndef unixcli
		delete m_InputValueList._buffer;
#else
		delete []m_InputValueList._buffer;
#endif
	m_InputValueList._buffer = 0;
	m_InputValueList._length = 0;
	m_CurrentParam = -1;
	m_CurrentRow = -1;
	m_CurrentParamStatus = 0;
	m_DataAtExecData.dataValue._length = 0;
	if (m_DataAtExecData.dataValue._buffer != NULL)
		delete m_DataAtExecData.dataValue._buffer;
	m_DataAtExecData.dataValue._buffer = NULL;
	m_DataAtExecDataBufferSize = 0;
	if (m_FetchDataValue.rowAddress != NULL )
		delete[] m_FetchDataValue.rowAddress;
	m_FetchDataValue.numberOfElements = 0;
	m_FetchDataValue.numberOfRows = 0;
	m_FetchDataValue.rowAddress = NULL;
	if (m_ColumnIndexes != NULL)
	{
		delete[] m_ColumnIndexes;
		m_ColumnIndexes = NULL;
	}
	if (m_SwapInfo != NULL)
	{
		for (int r = 0; r < m_SwapInfo_NumRows; r++) 
		{
			if (m_SwapInfo[r] != NULL)
				delete[] (m_SwapInfo[r]);
		}
		delete[] m_SwapInfo;
		m_SwapInfo = NULL;
		m_SwapInfo_NumRows = 0;
	}
}

void CStmt::InitParamColumnList()
{
	m_StmtQueryType = -1;
	m_StmtHandle  = 0;
	if (m_InputParams != NULL)
		delete []m_InputParams;
	m_InputParams = NULL;
	if (m_OutputColumns != NULL)
		delete m_OutputColumns;
	m_OutputColumns = NULL;
}

SQLRETURN CStmt::Close(SQLUSMALLINT Option)
{
	SQLRETURN			rc = SQL_SUCCESS;

	//#ifdef ASYNCIO
	//
	// If asynchronous processing is going on in a statement handle
	// the client has to call SQLCancel before closing this
	//if(m_ConnectHandle->m_async_IOthread != NULL &&
	//	m_ConnectHandle->m_async_IOthread->getStatementHandle() == this)
	//	return SQL_STILL_EXECUTING;
	//#endif /* ASYNCIO */
	
	if (m_ConnectHandle->getConnectionStatus() == SQL_CD_TRUE)
	{
		clearError();
		setDiagRec(DRIVER_ERROR, IDS_08_S01, -20005);
		rc = SQL_SUCCESS_WITH_INFO;
	}
	else if(!(m_isClosed && Option == SQL_CLOSE))
	{
		m_SrvrCallContext.initContext();
		m_SrvrCallContext.odbcAPI = SQL_API_SQLFREESTMT;
		m_SrvrCallContext.sqlHandle = this;
		m_SrvrCallContext.SQLSvc_ObjRef = m_ConnectHandle->getSrvrObjRef();
		m_SrvrCallContext.ASSvc_ObjRef = NULL;
		m_SrvrCallContext.eventHandle = m_StmtEvent;
		m_SrvrCallContext.dialogueId = m_ConnectHandle->getDialogueId();
		m_SrvrCallContext.statementTimeout = m_QueryTimeout;
		m_SrvrCallContext.u.closeParams.stmtLabel = m_StmtLabel;
		m_SrvrCallContext.u.closeParams.option = Option;
		rc = ThreadControlProc(&m_SrvrCallContext);
	}

	m_ThreadStatus = SQL_SUCCESS;
	m_RowsFetched = -1;
	m_CurrentRowFetched = 0;
	m_CurrentRowInRowset = 0;
	m_RowsetSize = 0;
	m_ResultsetRowsFetched = 0;
	m_RowNumber = 0;
	m_RowStatusArray = NULL;
	m_CurrentFetchType = 0;

	m_isClosed = true;

	//#ifdef ASYNCIO
	//m_StmtExecState = STMT_EXECUTION_NONE;
	//m_AsyncCanceled = FALSE;
	//#endif

	return rc;
}

SQLRETURN CStmt::initialize()
{
	m_ARDDesc = new CDesc(m_InputHandle, SQL_ATTR_APP_ROW_DESC, SQL_DESC_ALLOC_AUTO);
	m_IRDDesc	= new CDesc(m_InputHandle, SQL_ATTR_IMP_ROW_DESC, SQL_DESC_ALLOC_AUTO, this);
	m_APDDesc = new CDesc(m_InputHandle, SQL_ATTR_APP_PARAM_DESC, SQL_DESC_ALLOC_AUTO);
	m_IPDDesc = new CDesc(m_InputHandle, SQL_ATTR_IMP_PARAM_DESC, SQL_DESC_ALLOC_AUTO, this);

	m_AppParamDesc = m_APDDesc;
	m_AppRowDesc = m_ARDDesc;
	m_ImpParamDesc = m_IPDDesc;
	m_ImpRowDesc = m_IRDDesc;
	
	if (m_ARDDesc == NULL || m_IRDDesc == NULL || m_APDDesc == NULL || m_IPDDesc == NULL)
	{
		setDiagRec(DRIVER_ERROR, IDS_HY_001);
		return SQL_ERROR;
	}
//	if (m_StmtEvent == NULL) 
//	{
//		m_StmtEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
//		if (m_StmtEvent == NULL)
//		{
//			setNTError(m_ConnectHandle->getErrorMsgLang(), "initialize - CreateEvent()");
//			return SQL_ERROR;
//		}
//	}
	return SQL_SUCCESS;
}
	
SQLRETURN CStmt::SetStmtAttr(SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength)
{
	SQLRETURN	rc = SQL_SUCCESS;
	unsigned long	retCode = SQL_SUCCESS;
	SQLUINTEGER		Value;
//#if defined(_WIN64) || defined(__LP64__)
	unsigned long ULongValuePtr = (unsigned long)ValuePtr;;
//#endif
	#ifdef ASYNCIO
	char buffer[256];    // Hold the error text
	int iterations = 0;  // number of times we will wait for the async IO thread to initialize
	#endif

	
	clearError();
	m_CurrentOdbcAPI = SQL_API_SQLSETSTMTATTR;
	switch (Attribute)
	{
	case SQL_ATTR_APP_PARAM_DESC:
		if (ValuePtr == SQL_NULL_HDESC)
			m_AppParamDesc = m_APDDesc;
		else
			m_AppParamDesc = (CDesc *)ValuePtr;
		break;
	case SQL_ATTR_APP_ROW_DESC:
		if (ValuePtr == SQL_NULL_HDESC)
			m_AppRowDesc = m_ARDDesc;
		else
			m_AppRowDesc = (CDesc *)ValuePtr;
		ValuePtr = (SQLPOINTER)m_AppRowDesc;
		break;
	case SQL_ATTR_ASYNC_ENABLE:
		m_AsyncEnable = (SQLUINTEGER)ULongValuePtr;

		#ifdef ASYNCIO
	
		if(m_AsyncEnable == SQL_ASYNC_ENABLE_ON && m_ConnectHandle-> m_async_IOthread == NULL)
		{
			m_ConnectHandle->m_async_IOthread = new AsyncIO_Thread(m_ConnectHandle);
			if(m_ConnectHandle->m_async_IOthread == NULL)
			{
				m_AsyncEnable = SQL_ASYNC_ENABLE_OFF;
				rc = SQL_ERROR;
			}
			else
			{
				m_ConnectHandle->m_async_IOthread->Create(AsyncIOThread);

				if(m_ConnectHandle->m_async_IOthread->IsInitialized() == false)
				{
					while( (m_ConnectHandle->m_async_IOthread->IsInitialized() == false) 
						&& iterations++ < 3)
					{
						#ifdef __DEBUGASYNCIO
						printf("SQLSetStmtAttr: Statement %x, Connection %x not initialized\n",this,m_ConnectHandle);
						#endif
						sleep(1);
						sched_yield(); // allow the other thread a chance to run
					} // while iterations < 3 && !m_async_IOthread->IsInitialized() 

					if(m_ConnectHandle->m_async_IOthread->IsInitialized() == false)
					{
						// Something seriously wrong
						#ifdef __DEBUGASYNCIO
						printf("SQLSetStmtAttr: Statement %x, Connection %x not initialized after 3 attempts\n",this,m_ConnectHandle);
						#endif

						sprintf(buffer,"SQLSetStmtAttr SQL_ASYNC_ENABLE_ON failed for statement handle %x. Async thread was not initialized for connection handle %x",
							this,m_ConnectHandle); 
						this->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);
						rc = SQL_ERROR;

						m_AsyncEnable = SQL_ASYNC_ENABLE_OFF; // turn ASYNC OFF

					} // if(m_ConnectHandle->m_async_IOthread->IsInitialized() == false)
				} // if(m_ConnectHandle->m_async_IOthread->IsInitialized() == false)
			}  // if(m_ConnectHandle->m_async_IOthread != NULL)
		} // if(m_AsyncEnable == SQL_ASYNC_ENABLE_ON && m_ConnectHandle-> m_async_IOthread == NULL)
		#endif // ASYNCIO
		break;
	case SQL_ATTR_CONCURRENCY:
		// Added code to send to server about the SQL_ATTR_CONCURRENCY to perform
		// "CONTROL QUERY DEFAULT CURSOR_READONLY 'TRUE or FALSE'" based on
		// SQL_CONCUR_READ_ONLY or SQL_CONCUR_LOCK attributes.
		Value = (SQLUINTEGER)ULongValuePtr;
		if (m_ConnectHandle->getConnectionStatus() == SQL_CD_TRUE)
		{
			clearError();
			setDiagRec(DRIVER_ERROR, IDS_08_S01, -20005);
			return SQL_ERROR;
		}
		else
			rc = m_ConnectHandle->SetConnectAttr(Attribute, (SQLUINTEGER)ULongValuePtr, NULL);
		if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
		{
			switch (Value)
			{
			case SQL_CONCUR_READ_ONLY:
				m_CursorSensitivity = SQL_INSENSITIVE;
				break;
			case SQL_CONCUR_LOCK:
				m_CursorSensitivity = SQL_SENSITIVE;
				break;
			case SQL_CONCUR_ROWVER:
			case SQL_CONCUR_VALUES:
				m_CursorSensitivity = SQL_SENSITIVE;
				retCode = IDS_01_S02;
				Value = SQL_CONCUR_LOCK;
				rc = SQL_SUCCESS_WITH_INFO;
				break;
			default:
				retCode = IDS_HY_024;
				rc = SQL_ERROR;
			}
			if (rc != SQL_ERROR)
				m_Concurrency = Value;
		}
		break;
	case SQL_ATTR_CURSOR_HOLDABLE:
		m_CursorHoldable = (SQLUINTEGER)ULongValuePtr; 
		break;
	case SQL_ATTR_CURSOR_SCROLLABLE:
		if ((SQLUINTEGER)ULongValuePtr == SQL_NONSCROLLABLE)
			m_CursorScrollable = (SQLUINTEGER)ULongValuePtr;
		else
		{
			m_CursorScrollable = SQL_NONSCROLLABLE;
			retCode = IDS_01_S02;
			rc = SQL_SUCCESS_WITH_INFO;
		}
		break;
	case SQL_ATTR_CURSOR_SENSITIVITY:
		Value = (SQLUINTEGER)ULongValuePtr;
		switch (Value)
		{
		case SQL_UNSPECIFIED:
		case SQL_SENSITIVE:
			m_Concurrency = SQL_CONCUR_LOCK;
			break;
		case SQL_INSENSITIVE:
			m_Concurrency = SQL_CONCUR_READ_ONLY;
			break;
		default:
			retCode = IDS_HY_024;
			rc = SQL_ERROR;
			break;
		}
		if (rc != SQL_ERROR)
			m_CursorSensitivity = Value;
		break;
	case SQL_ATTR_CURSOR_TYPE:
		Value = (SQLUINTEGER)ULongValuePtr;
		if (Value == SQL_CURSOR_FORWARD_ONLY)
			m_CursorType = Value;
		else
		{
			m_CursorType = SQL_CURSOR_FORWARD_ONLY;
			retCode = IDS_01_S02;
			rc = SQL_SUCCESS_WITH_INFO;
		}
		break;
	case SQL_ATTR_ENABLE_AUTO_IPD:
		m_EnableAutoIPD = (SQLUINTEGER)ULongValuePtr;
		break;
	case SQL_ATTR_FETCH_BOOKMARK_PTR:
		m_FetchBookmarkPtr = ValuePtr;
		break;
	case SQL_ATTR_KEYSET_SIZE:
		m_KeysetSize = (SQLULEN)ValuePtr;
		break;
	case SQL_ATTR_MAX_LENGTH:
		m_MaxLength = (SQLULEN)ValuePtr;
		break;
	case SQL_ATTR_MAX_ROWS:
		m_MaxRows = (SQLULEN)ValuePtr;
		break;
	case SQL_ATTR_METADATA_ID:
		m_MetadataId = (SQLUINTEGER)ULongValuePtr;
		break;
	case SQL_ATTR_NOSCAN:
		m_Noscan = (SQLUINTEGER)ULongValuePtr;
		break;
	case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
		rc = m_AppParamDesc->SetDescField(0, SQL_DESC_BIND_OFFSET_PTR, ValuePtr, StringLength);
		break;
	case SQL_ATTR_PARAM_BIND_TYPE:
		rc = m_AppParamDesc->SetDescField(0, SQL_DESC_BIND_TYPE, ValuePtr, StringLength);
		break;
	case SQL_ATTR_PARAM_OPERATION_PTR:
		rc = m_AppParamDesc->SetDescField(0, SQL_DESC_ARRAY_STATUS_PTR, ValuePtr, StringLength);
		break;
	case SQL_ATTR_PARAM_STATUS_PTR:
		rc = m_ImpParamDesc->SetDescField(0, SQL_DESC_ARRAY_STATUS_PTR, ValuePtr, StringLength);
		break;
	case SQL_ATTR_PARAMS_PROCESSED_PTR:
		rc = m_ImpParamDesc->SetDescField(0, SQL_DESC_ROWS_PROCESSED_PTR, ValuePtr, StringLength);
		break;
	case SQL_ATTR_PARAMSET_SIZE:
		rc = m_AppParamDesc->SetDescField(0, SQL_DESC_ARRAY_SIZE, ValuePtr, StringLength);
		break;
	case SQL_ATTR_QUERY_TIMEOUT:
		m_QueryTimeout = (SQLUINTEGER)ULongValuePtr;
		if(m_QueryTimeout != 0 && m_QueryTimeout < 30)
				m_QueryTimeout = 30;
		break;
	case SQL_ATTR_RETRIEVE_DATA:
		m_RetrieveData = (SQLUINTEGER)ULongValuePtr;
		break;
	case SQL_ATTR_ROW_ARRAY_SIZE:
	case SQL_ROWSET_SIZE:
		rc = m_AppRowDesc->SetDescField(0, SQL_DESC_ARRAY_SIZE, ValuePtr, StringLength);
		break;
	case SQL_ATTR_ROW_BIND_OFFSET_PTR:
		rc = m_AppRowDesc->SetDescField(0, SQL_DESC_BIND_OFFSET_PTR, ValuePtr, StringLength);
		break;
	case SQL_ATTR_ROW_BIND_TYPE:
		rc = m_AppRowDesc->SetDescField(0, SQL_DESC_BIND_TYPE, ValuePtr, StringLength);
		break;
	case SQL_ATTR_ROW_OPERATION_PTR:
		rc = m_AppRowDesc->SetDescField(0, SQL_DESC_ARRAY_STATUS_PTR, ValuePtr, StringLength);
		break;
	case SQL_ATTR_ROW_STATUS_PTR:
		rc = m_ImpRowDesc->SetDescField(0, SQL_DESC_ARRAY_STATUS_PTR, ValuePtr, StringLength);
		break;
	case SQL_ATTR_ROWS_FETCHED_PTR:
		rc = m_ImpRowDesc->SetDescField(0, SQL_DESC_ROWS_PROCESSED_PTR, ValuePtr, StringLength);
		break;
	case SQL_ATTR_SIMULATE_CURSOR:
		m_SimulateCursor = (SQLUINTEGER)ULongValuePtr;
		break;
	case SQL_ATTR_USE_BOOKMARKS:
		Value = (SQLUINTEGER)ULongValuePtr;
		if (Value == SQL_UB_ON)
		{
			m_UseBookmarks = SQL_UB_OFF;
			retCode = IDS_01_S02;
			rc = SQL_SUCCESS_WITH_INFO;
		}
		else
			m_UseBookmarks = Value;
		break;
	case SQL_ATTR_FETCH_BUFFER_SIZE:
		m_FetchBufferSize = (SQLINTEGER)ULongValuePtr;
		if (m_FetchBufferSize < 0)
			m_FetchBufferSize = 0;
		m_FetchBufferSize *= 1024;
		break;
	case SQL_ATTR_ROW_NUMBER:
	case SQL_ATTR_IMP_PARAM_DESC:
	case SQL_ATTR_IMP_ROW_DESC:
	default:
		retCode = IDS_HY_092;
		rc = SQL_ERROR;
		break;
	}
	if (retCode != SQL_SUCCESS)
		setDiagRec(DRIVER_ERROR, retCode);
	return rc;
}

SQLRETURN CStmt::GetStmtAttr(SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER BufferLength,
					SQLINTEGER *StringLengthPtr)
{
	SQLRETURN				rc = SQL_SUCCESS;
	RETURN_VALUE_STRUCT		retValue;
	BOOL					DescAttr;
	SQLUSMALLINT			*ArrayStatusPtr;

	DescAttr = FALSE;
	retValue.dataType = DRVR_PENDING;
	retValue.u.strPtr = NULL;
	
	clearError();
	m_CurrentOdbcAPI = SQL_API_SQLGETSTMTATTR;
	switch (Attribute)
	{
	case SQL_ATTR_APP_PARAM_DESC:
		retValue.u.pValue = m_AppParamDesc;
		retValue.dataType = SQL_IS_POINTER;
		break;
	case SQL_ATTR_APP_ROW_DESC:
		retValue.u.pValue = m_AppRowDesc;
		retValue.dataType = SQL_IS_POINTER;
		break;
	case SQL_ATTR_ASYNC_ENABLE:
		retValue.u.u32Value = m_AsyncEnable;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_CONCURRENCY:
		retValue.u.u32Value = m_Concurrency;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_CURSOR_HOLDABLE:
		retValue.u.u32Value = m_CursorHoldable;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_CURSOR_SCROLLABLE:
		retValue.u.u32Value = m_CursorScrollable;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_CURSOR_SENSITIVITY:
		retValue.u.u32Value = m_CursorSensitivity;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_CURSOR_TYPE:
		retValue.u.u32Value = m_CursorType;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_ENABLE_AUTO_IPD:
		retValue.u.u32Value = m_EnableAutoIPD;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_FETCH_BOOKMARK_PTR:
		retValue.u.pValue = m_FetchBookmarkPtr;
		retValue.dataType = SQL_IS_POINTER;
		break;
	case SQL_ATTR_IMP_PARAM_DESC:
		retValue.u.pValue = m_ImpParamDesc;
		retValue.dataType = SQL_IS_POINTER;
		break;
	case SQL_ATTR_IMP_ROW_DESC:
		retValue.u.pValue = m_ImpRowDesc;
		retValue.dataType = SQL_IS_POINTER;
		break;
	case SQL_ATTR_KEYSET_SIZE:
#ifdef __LP64__
		retValue.u.pValue = (SQLPOINTER)m_KeysetSize;
		retValue.dataType = SQL_IS_POINTER;
#else
		retValue.u.u32Value = m_KeysetSize;
		retValue.dataType = SQL_IS_UINTEGER;
#endif
		break;
	case SQL_ATTR_MAX_LENGTH:
#ifdef __LP64__
		retValue.u.pValue = (SQLPOINTER)m_MaxLength;
		retValue.dataType = SQL_IS_POINTER;
#else
		retValue.u.u32Value = m_MaxLength;
		retValue.dataType = SQL_IS_UINTEGER;
#endif
		break;
	case SQL_ATTR_MAX_ROWS:
#ifdef __LP64__
		retValue.u.pValue = (SQLPOINTER)m_MaxRows;
		retValue.dataType = SQL_IS_POINTER;
#else
		retValue.u.u32Value = m_MaxRows;
		retValue.dataType = SQL_IS_UINTEGER;
#endif
		break;
	case SQL_ATTR_METADATA_ID:
		retValue.u.u32Value = m_MetadataId;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_NOSCAN:
		retValue.u.u32Value = m_Noscan;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
		rc = m_AppParamDesc->GetDescField(0, SQL_DESC_BIND_OFFSET_PTR, ValuePtr, BufferLength,
			StringLengthPtr);
		DescAttr = TRUE;
		break;
	case SQL_ATTR_PARAM_BIND_TYPE:
		rc = m_AppParamDesc->GetDescField(0, SQL_DESC_BIND_TYPE, ValuePtr, BufferLength,
			StringLengthPtr);
		DescAttr = TRUE;
		break;
	case SQL_ATTR_PARAM_OPERATION_PTR:
		rc = m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_STATUS_PTR, ValuePtr, BufferLength,
			StringLengthPtr);
		DescAttr = TRUE;
		break;
	case SQL_ATTR_PARAM_STATUS_PTR:
		rc = m_ImpParamDesc->GetDescField(0, SQL_DESC_ARRAY_STATUS_PTR, ValuePtr, BufferLength,
			StringLengthPtr);
		DescAttr = TRUE;
		break;
	case SQL_ATTR_PARAMS_PROCESSED_PTR:
		rc = m_ImpParamDesc->GetDescField(0, SQL_DESC_ROWS_PROCESSED_PTR, ValuePtr, BufferLength,
			StringLengthPtr);
		DescAttr = TRUE;
		break;
	case SQL_ATTR_PARAMSET_SIZE:
		rc = m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, ValuePtr, BufferLength,
			StringLengthPtr);
		DescAttr = TRUE;
		break;
	case SQL_ATTR_QUERY_TIMEOUT:
		retValue.u.u32Value = m_QueryTimeout;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_RETRIEVE_DATA:
		retValue.u.u32Value = m_RetrieveData;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_ROW_ARRAY_SIZE:
	case SQL_ROWSET_SIZE:
		rc = m_AppRowDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, ValuePtr, BufferLength,
			StringLengthPtr);
		DescAttr = TRUE;
		break;
	case SQL_ATTR_ROW_BIND_OFFSET_PTR:
		rc = m_AppRowDesc->GetDescField(0, SQL_DESC_BIND_OFFSET_PTR, ValuePtr, BufferLength,
			StringLengthPtr);
		DescAttr = TRUE;
		break;
	case SQL_ATTR_ROW_BIND_TYPE:
		rc = m_AppRowDesc->GetDescField(0, SQL_DESC_BIND_TYPE, ValuePtr, BufferLength,
			StringLengthPtr);
		DescAttr = TRUE;
		break;
	case SQL_ATTR_ROW_NUMBER:
		if (m_StmtState == STMT_EXECUTED || m_StmtState == STMT_FETCHED_TO_END)
		{
			setDiagRec(DRIVER_ERROR, IDS_24_000);
			return SQL_ERROR;
		}
		rc = m_ImpRowDesc->GetDescField(0, SQL_DESC_ARRAY_STATUS_PTR, &ArrayStatusPtr, 
			sizeof(ArrayStatusPtr), NULL);
		if (ArrayStatusPtr != NULL)
		{
			if (ArrayStatusPtr[m_RowNumber] == SQL_ROW_DELETED || 
				ArrayStatusPtr[m_RowNumber] == SQL_ROW_ERROR)
			{
				setDiagRec(DRIVER_ERROR, IDS_HY_109);
				return SQL_ERROR;
			}
		}
#ifdef __LP64__
		retValue.u.pValue = (SQLPOINTER)m_RowNumber;
		retValue.dataType = SQL_IS_POINTER;
#else
		retValue.u.u32Value = m_RowNumber;
		retValue.dataType = SQL_IS_UINTEGER;
#endif
		break;
	case SQL_ATTR_ROW_OPERATION_PTR:
		rc = m_AppRowDesc->GetDescField(0, SQL_DESC_ARRAY_STATUS_PTR, ValuePtr, BufferLength,
			StringLengthPtr);
		DescAttr = TRUE;
		break;
	case SQL_ATTR_ROW_STATUS_PTR:
		rc = m_ImpRowDesc->GetDescField(0, SQL_DESC_ARRAY_STATUS_PTR, ValuePtr, BufferLength,
			StringLengthPtr);
		DescAttr = TRUE;
		break;
	case SQL_ATTR_ROWS_FETCHED_PTR:
		rc = m_ImpRowDesc->GetDescField(0, SQL_DESC_ROWS_PROCESSED_PTR, ValuePtr, BufferLength,
			StringLengthPtr);
		DescAttr = TRUE;
		break;
	case SQL_ATTR_SIMULATE_CURSOR:
		retValue.u.u32Value = m_SimulateCursor;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_USE_BOOKMARKS:
		retValue.u.u32Value = m_UseBookmarks;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_FETCH_BUFFER_SIZE:
		retValue.u.u32Value = m_FetchBufferSize/1024;
		retValue.dataType = SQL_IS_INTEGER;
		break;
	case SQL_ATTR_ROWCOUNT64_PTR:
		retValue.u.s64Value = m_RowCount;
		retValue.dataType = SQL_C_SBIGINT;
		break;
	default:
		setDiagRec(DRIVER_ERROR, IDS_HY_092);
		rc = SQL_ERROR;
		break;
	}
	if (rc == SQL_SUCCESS && (!DescAttr))
		rc = returnAttrValue(TRUE, this, &retValue, ValuePtr, BufferLength, StringLengthPtr);
	return rc;
}

SQLRETURN CStmt::BindCol(SQLUSMALLINT ColumnNumber,
				SQLSMALLINT TargetType,
				SQLPOINTER	TargetValuePtr,
				SQLINTEGER	BufferLength,
				SQLLEN     *StrLen_or_IndPtr)
{

	CDesc	*pDesc;
	unsigned long	retCode;

	clearError();
	m_CurrentOdbcAPI = SQL_API_SQLBINDCOL;
	pDesc = m_AppRowDesc;	
	if (pDesc == SQL_NULL_HDESC)
	{
		setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, "ARD is NULL");
		return SQL_ERROR;
	}
	if (ColumnNumber == 0)
	{
		setDiagRec(DRIVER_ERROR, IDS_07_009);
		return SQL_ERROR;
	}
	retCode = pDesc->BindCol(ColumnNumber,TargetType, TargetValuePtr, BufferLength, StrLen_or_IndPtr);
	if (retCode != SQL_SUCCESS)
	{
		setDiagRec(DRIVER_ERROR, retCode);
		return SQL_ERROR;
	}
	return SQL_SUCCESS;
}

SQLRETURN CStmt::BindParameter(SQLUSMALLINT ParameterNumber, 
			SQLSMALLINT InputOutputType,
			SQLSMALLINT ValueType,
			SQLSMALLINT ParameterType, 
			SQLULEN 	ColumnSize,
			SQLSMALLINT DecimalDigits,
			SQLPOINTER  ParameterValuePtr,
			SQLLEN   	BufferLength,
			SQLLEN     *StrLen_or_IndPtr)
{
	CDesc *pDesc;
	unsigned long retCode;
//	SQLINTEGER ODBCAppVersion;

	clearError();
	m_CurrentOdbcAPI = SQL_API_SQLBINDPARAMETER;
	pDesc = m_AppParamDesc;
	if (pDesc == SQL_NULL_HDESC)
	{
		setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, "APD is NULL");
		return SQL_ERROR;
	}
//	ODBCAppVersion = getODBCAppVersion();
//	if(ValueType==SQL_C_DEFAULT)
//		if ((retCode = getCDefault(ParameterType, ODBCAppVersion, ValueType)) != SQL_SUCCESS)
//			return retCode;

	retCode = pDesc->BindParameter(ParameterNumber,
			InputOutputType, ValueType, ParameterValuePtr, BufferLength, StrLen_or_IndPtr);
	if (retCode != SQL_SUCCESS)
	{
		setDiagRec(DRIVER_ERROR, retCode);
		return SQL_ERROR;
	}
	pDesc = m_ImpParamDesc;
	if (pDesc == SQL_NULL_HDESC)
		{
		setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, "IPD is NULL");
		return SQL_ERROR;
	}
	retCode = pDesc->BindParameter(ParameterNumber, ParameterType, ColumnSize, DecimalDigits);
	if (retCode != SQL_SUCCESS)
	{
		setDiagRec(DRIVER_ERROR, retCode);
		return SQL_ERROR;
	}
	
	return SQL_SUCCESS;
}

SQLRETURN CStmt::SendSQLCommand(BOOL SkipProcess, SQLCHAR *StatementText,
					SQLINTEGER TextLength)
{
	char				tempStr[2048];
	char				*token;
	char				*delimiters = " {(\t\r\n";
	unsigned char*		sqlString;
	unsigned long		sqlStringLen;
	unsigned long		len;
	SQLINTEGER			translen=0;
	char*				sqlStringDrvrLocale;
	char				errorMsg[64];
	SQLULEN 			MaxRowsetSize;
	SQLULEN 			AppArrayStatusSize;
	int					status;

	BYTE	*inputParams = 0;
	BYTE	*outputColumns = 0;
	DWORD	translateOption = 0;
	SQLRETURN	rc;
	char *lasts;

	setStmtLabel(m_StmtLabelOrg); // need to revert it back to original because of catalog API

	if (m_ConnectHandle->getConnectionStatus() == SQL_CD_TRUE)
	{
		clearError();
		setDiagRec(DRIVER_ERROR, IDS_08_S01, -20005);
		return SQL_ERROR;
	}
	if (! SkipProcess)
	{
		clearError();
		setDiagRowCount(-1, -1);
		// Caffeine Changes
		InitParamColumnList();
		// Caffeine Changes
		m_NumResultCols = 0;

		if (m_StmtState == STMT_EXECUTED)
		{
			setDiagRec(DRIVER_ERROR, IDS_24_000);
			return SQL_ERROR;
		}
		if (TextLength == SQL_NTS)
			sqlStringLen = strlen((const char *)StatementText);
		else
			sqlStringLen = TextLength;
		
		sqlString = (unsigned char*)new BYTE[sqlStringLen + 1];
		if (sqlString == 0)
		{
			setDiagRec(DRIVER_ERROR, IDS_HY_001);
			return SQL_ERROR;	
		}
		
		if (m_intStmtType != TYPE_QS)
			trimSqlString(StatementText, sqlString, sqlStringLen);
		else
		{
			strcpy((char*)sqlString, (char*)StatementText);
			wmstrim((char*)sqlString);
		}		
		
#ifndef unixcli		
		if (sqlStringLen == 0 || sqlString > INT_MAX)
#else
		if (sqlStringLen == 0 || sqlStringLen > INT_MAX)
#endif
		{
			setDiagRec(DRIVER_ERROR, IDS_S1_090);
			delete[] sqlString;
			return SQL_ERROR;	
		}

		m_SqlString = "";

		//10-080124-0030 
		TraceOut(TR_ODBC_DEBUG, "CStmt::SendSQLCommand sqlString \"%s\", sqlStringLen %d", sqlString, sqlStringLen);
		if (m_ConnectHandle->m_StandardConfig)
		{
			// send sqlString in DrvrLocale
			// translate from UTF8 to DrvrLocale
			sqlStringDrvrLocale = new char[sqlStringLen*4+1];
			errorMsg[0] = '\0';

			if (m_ConnectHandle->m_ICUConv->TranslateUTF8(TRUE, (char*)sqlString, sqlStringLen, sqlStringDrvrLocale, sqlStringLen*4+1, &translen, errorMsg) != SQL_SUCCESS)
			{
				if (errorMsg[0] != '\0')
					setDiagRec(DRIVER_ERROR, IDS_193_DRVTODS_ERROR, 0, (char *)errorMsg);
				else
					setDiagRec(DRIVER_ERROR, IDS_193_DRVTODS_ERROR);
				delete[] sqlStringDrvrLocale;
				delete sqlString;
				return SQL_ERROR;
			}
			else
			{
				m_SqlString.append((const char *)sqlStringDrvrLocale, translen);
				delete[] sqlStringDrvrLocale;
			}
		}
		else // sqlString is already in UTF8
		{
			m_SqlString.append((const char *)sqlString, sqlStringLen);
		}
		
		if (sqlStringLen < sizeof(tempStr))
			len = sqlStringLen;
		else
			len = sizeof(tempStr)-1;
		strncpy(tempStr, (const char *)m_SqlString.data(), len);
		tempStr[len] = '\0';
#ifndef unixcli		
		token = strtok(tempStr, delimiters);
#else
		token = strtok_r(tempStr, delimiters,&lasts);
#endif
		if (token == NULL)
		{
			setDiagRec(DRIVER_ERROR, IDS_S1_009);
			delete sqlString;
			return SQL_ERROR;
		}

		if (_strnicmp(token, "WMSOPEN", 7) == 0)
		{
			m_intStmtType = TYPE_QS;
			m_StmtType = TYPE_QS_OPEN;
		}
		else if (_strnicmp(token, "WMSCLOSE", 8) == 0)
		{
			m_intStmtType = TYPE_UNKNOWN;
			m_StmtType = TYPE_QS_CLOSE;
		}
		else if (m_intStmtType == TYPE_QS)
		{
			if (_strnicmp(token, "STATUS", 6) == 0)
				m_StmtType = TYPE_SELECT;
			else if (_strnicmp(token, "CANCEL", 6) == 0)
				m_StmtType = TYPE_UNKNOWN;
			else if (_strnicmp(token, "SUSPEND", 7) == 0)
				m_StmtType = TYPE_UNKNOWN;
			else if (_strnicmp(token, "RESUME", 6) == 0)
				m_StmtType = TYPE_UNKNOWN;
			else if (_strnicmp(token, "ALTPRI", 6) == 0)
				m_StmtType = TYPE_UNKNOWN;
			else if (_strnicmp(token, "ADD", 3) == 0)
				m_StmtType = TYPE_UNKNOWN;
			else if (_strnicmp(token, "ALTER", 5) == 0)
				m_StmtType = TYPE_UNKNOWN;
			else if (strnicmp(token, "DELETE", 6) == 0)
				m_StmtType = TYPE_UNKNOWN;
			else if (_strnicmp(token, "INFO", 4) == 0)
				m_StmtType = TYPE_SELECT;
			else
				m_StmtType = TYPE_UNKNOWN;
		}
		else if (_strnicmp(token, "CMDOPEN", 7) == 0)
		{
			m_intStmtType = TYPE_CMD;
			m_StmtType = TYPE_CMD_OPEN;
		}
		else if (_strnicmp(token, "CMDCLOSE", 8) == 0)
		{
			m_intStmtType = TYPE_UNKNOWN;
			m_StmtType = TYPE_CMD_CLOSE;
		}
		else if (m_intStmtType == TYPE_CMD)
		{
			if ((_strnicmp(token, "LIST", 4) == 0) ||
				(_strnicmp(token, "STATUS", 6) == 0) ||
				(_strnicmp(token, "INFO", 4) == 0))
					m_StmtType = TYPE_SELECT;
			else if ((_strnicmp(token, "ADD", 3) == 0) ||
				(_strnicmp(token, "ALTER", 5) == 0))
					m_StmtType = TYPE_INSERT;
			else if (_strnicmp(token, "DELETE", 6) == 0)
					m_StmtType = TYPE_DELETE;
			else if ((_strnicmp(token, "START", 5) == 0) ||
					 (_strnicmp(token, "STOP", 4) == 0))
					 	m_StmtType = TYPE_DELETE;
		}		
		else if (strcmp(token, "SELECT") == 0)
			m_StmtType = TYPE_SELECT;
		else if ((strcmp(token, "INSERT") == 0) || (strcmp(token, "INS") == 0) ||
				 (strcmp(token, "UPSERT") == 0)) // added for "UPSERT" support
			m_StmtType = TYPE_INSERT;
		else if ((strcmp(token, "UPDATE") == 0) || (strcmp(token, "MERGE") == 0))
			m_StmtType = TYPE_UPDATE;
		else if ((strcmp(token, "DELETE") == 0))
			m_StmtType = TYPE_DELETE;
		else if (strcmp(token, "SMD") == 0) //  added for user module support
			m_StmtType = TYPE_SMD;
		else if ((strcmp(token, "CALL") == 0) || (strcmp(token, "?=CALL") == 0)) // added for stored proc call support
			m_StmtType = TYPE_CALL;
		//else if (strcmp(token, "INFOSTATS") == 0) //  added for INFOSTATS support
		else if (_strnicmp(token, "INFOSTATS", 9) == 0)
			m_StmtType = TYPE_STATS;
		else if (strcmp(token, "?=") == 0)
		{
#ifndef unixcli
			token = strtok(NULL, delimiters);
#else
			token = strtok_r(NULL, delimiters,&lasts);
#endif
			if (token == NULL)
			{
				setDiagRec(DRIVER_ERROR, IDS_S1_009);
				delete sqlString;
				return SQL_ERROR;
			}
			if (strcmp(token, "CALL") == 0)
				m_StmtType = TYPE_CALL;	
			else 
				m_StmtType = TYPE_UNKNOWN;	
		}
		else if (strcmp(token, "?") == 0)
		{
#ifndef unixcli
			token = strtok(NULL, delimiters);
#else
			token = strtok_r(NULL, delimiters,&lasts);
#endif
			if (token == NULL)
			{
				setDiagRec(DRIVER_ERROR, IDS_S1_009);
				delete sqlString;
				return SQL_ERROR;
			}
#ifndef unixcli
			if ((strcmp(token, "=") == 0) && (strcmp(strtok(NULL, delimiters), "CALL") == 0))
#else
			if ((strcmp(token, "=") == 0) && (strcmp(strtok_r(NULL, delimiters,&lasts), "CALL") == 0))
#endif
				m_StmtType = TYPE_CALL;
			else 
				m_StmtType = TYPE_UNKNOWN;	
		}

		else
		{
			m_StmtType = TYPE_UNKNOWN;
		}

		//  Need to check if SQL_ATTR_CONCURRENCY attribute is set to SQL_CONCUR_READ_ONLY then return error if
		// WHERE CURRENT OF is used in UPDATE or DELETE statements. The reason is if SQL_CONCUR_READ_ONLY is set
		// and SQL_ATTR_FETCH_BUFFER_SIZE is more than ZERO OR m_RowsFetched returned by server is greater than
		// ONE which is used to do bulk fetches then we may go head and update or delete a ROW which is not
		// the ROW application wants to update or delete. Inspite of SQL_CONCUR_READ_ONLY
		// is set SQL/MX allows to update or delete since FOR UPDATE OF syntax in SELECT statement takes higher
		// priority. If SQL_ATTR_CONCURRENCY attribute is NOT set to SQL_CONCUR_READ_ONLY then we only do one ROW
		// at a time then we are OK in update or delete.
		if (m_StmtType == TYPE_UPDATE || m_StmtType == TYPE_DELETE) 
		{
			//  Last FOUR tokens in ANSI SQL syntax for positioned update/delete are 
			// always "WHERE", "CURRENT", "OF", "{cursor name | ext_cursor-name | iterator-name}".
			char	whereCurrectof[] = "WHERE CURRENT OF";
			char	*pwhereCurrentof;
			int		rwhereCurrentof = 0;
	
			sqlString[0] = '\0';
			if (TextLength == SQL_NTS)
				sqlStringLen = strlen((const char *)StatementText);
			else
				sqlStringLen = TextLength;

			if (m_intStmtType != TYPE_QS)
				trimSqlString(StatementText, sqlString, sqlStringLen, TRUE);
			else
			{
				strcpy((char*)sqlString, (char*)StatementText);
				wmstrim((char*)sqlString);
			}

			pwhereCurrentof = strstr((char *)sqlString, whereCurrectof);
			if (pwhereCurrentof != NULL)
			{
				char		tCursor[MAX_CURSOR_NAME_LEN+1];
				SQLUINTEGER	tConcurrency = -1;
				SQLUINTEGER	tRowsFetched = -1;

				rwhereCurrentof = (int) (pwhereCurrentof - (char *)sqlString + 17); // 17 = 16 (len of WHERE CURRENT OF) + 1 (space)

				sqlString[0] = '\0';
				tCursor[0] = '\0';
				if (TextLength == SQL_NTS)
					sqlStringLen = strlen((const char *)StatementText);
				else
					sqlStringLen = TextLength;

				if (m_intStmtType != TYPE_QS)
					trimSqlString(StatementText, sqlString, sqlStringLen);
				else
				{
					strcpy((char*)sqlString, (char*)StatementText);
					wmstrim((char*)sqlString);
				}

				strncpy(tCursor, (char *)sqlString + rwhereCurrentof, sqlStringLen - rwhereCurrentof);
				tCursor[sqlStringLen - rwhereCurrentof] = '\0';
				if (tCursor != NULL)
				{
#ifdef VERSION3
					std::list<CHandlePtr>::iterator i;
					CStmt* pStmt;
					for (i = m_ConnectHandle->m_StmtCollect.begin() ; i !=  m_ConnectHandle->m_StmtCollect.end() ; ++i)
					{
						pStmt = (CStmt*)(*i); 
						if ((pStmt != NULL ) && (strcmp(tCursor, pStmt->m_CursorName) == 0))
						{
							tConcurrency = pStmt->m_Concurrency;
							tRowsFetched = pStmt->m_RowsFetched;
							break;
						}
					}
#else
					RWTPtrSlistIterator<CHandle> i(m_ConnectHandle->m_StmtCollect);
					CStmt* pStmt;
					while ((pStmt = (CStmt *)i()) != NULL)
					{
						if(strcmp(tCursor, pStmt->m_CursorName) == 0)
						{
							tConcurrency = pStmt->m_Concurrency;
							tRowsFetched = pStmt->m_RowsFetched;
							break;
						}
					}
#endif
					if (tConcurrency != -1)
					{
						if (tConcurrency == SQL_CONCUR_READ_ONLY)
						{
							setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, "Cursor is Read-only. No updates are allowed. To update set Statement attribute SQL_ATTR_CONCURRENCY to SQL_CONCUR_LOCK for SELECT statement handle.");
							delete sqlString;
							return SQL_ERROR;
						}
						if (tRowsFetched > 1)
						{
							setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, "Maximum rows fetched greater than ONE. To update set Statement attribute SQL_ATTR_MAX_ROWS to 1 for SELECT statement handle.");
							delete sqlString;
							return SQL_ERROR;
						}
					}
				}
			}
		}

		delete[] sqlString;

		MaxRowsetSize = 0;
		m_SrvrCallContext.initContext();

		if (m_intStmtType != TYPE_QS && checkInputParam(m_SqlString))
		{
#ifdef __LP64__
				m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &MaxRowsetSize, SQL_IS_POINTER, NULL);
#else
				m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &MaxRowsetSize, SQL_IS_UINTEGER, NULL);
#endif
			// SQL does not support result sets for select statements so need to set rowsetsize to 0 or 1
			//if(m_StmtType == TYPE_SELECT)
			//	MaxRowsetSize = 0; 

			if (m_StmtType == TYPE_INSERT)
			{
				m_StmtType = TYPE_INSERT_PARAM;
			}
			if (m_CurrentOdbcAPI == SQL_API_SQLEXECDIRECT)
				m_SrvrCallContext.odbcAPI = SQL_API_SQLPREPARE;
			else
				m_SrvrCallContext.odbcAPI = m_CurrentOdbcAPI;

		}
		else
			m_SrvrCallContext.odbcAPI = m_CurrentOdbcAPI;
 
		m_SrvrCallContext.sqlHandle = this;
		m_SrvrCallContext.SQLSvc_ObjRef = m_ConnectHandle->getSrvrObjRef();
		m_SrvrCallContext.ASSvc_ObjRef = NULL;
		m_SrvrCallContext.eventHandle = m_StmtEvent;
		m_SrvrCallContext.dialogueId = m_ConnectHandle->getDialogueId();
		m_SrvrCallContext.connectionTimeout = m_ConnectHandle->getConnectionTimeout();
		m_SrvrCallContext.u.sendSQLcommandParams.queryTimeout = m_QueryTimeout;
#ifdef VERSION3
		m_SrvrCallContext.u.sendSQLcommandParams.sqlString = m_SqlString.c_str();
#else	
		m_SrvrCallContext.u.sendSQLcommandParams.sqlString = m_SqlString.data();
#endif
		m_SrvrCallContext.u.sendSQLcommandParams.stmtLabel = m_StmtLabel;
		m_SrvrCallContext.u.sendSQLcommandParams.stmtName = m_StmtName;
		m_SrvrCallContext.u.sendSQLcommandParams.cursorName = m_CursorName;
        m_SrvrCallContext.u.sendSQLcommandParams.moduleName = NULL;
		m_SrvrCallContext.u.sendSQLcommandParams.sqlStmtType = m_StmtType;
		m_SrvrCallContext.u.sendSQLcommandParams.asyncEnable = m_AsyncEnable;
		m_SrvrCallContext.u.sendSQLcommandParams.holdableCursor = m_CursorHoldable;
		m_SrvrCallContext.maxRowsetSize = MaxRowsetSize;
	}
#ifdef ASYNCIO
	if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
	{
		rc  = StartAsyncOperation(m_ConnectHandle, this /* statement handle */);
		if(rc == SQL_SUCCESS)
			rc = SQL_STILL_EXECUTING;
		else
			rc = SQL_ERROR;
	}
	else
		rc = ThreadControlProc(&m_SrvrCallContext);
#else
	rc = ThreadControlProc(&m_SrvrCallContext);
#endif /* ASYNCIO */

	if (m_CurrentOdbcAPI == SQL_API_SQLEXECDIRECT && m_SrvrCallContext.odbcAPI == SQL_API_SQLPREPARE)
	{
		if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
			rc = SendExecute(FALSE);
	}
	else if (m_StmtType == TYPE_INSERT_PARAM)
	{
#ifdef __LP64__
		m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &AppArrayStatusSize, SQL_IS_POINTER, NULL);
#else
		m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &AppArrayStatusSize, SQL_IS_UINTEGER, NULL);
#endif
		m_ImpParamDesc->setDescArrayStatus(AppArrayStatusSize,SQL_PARAM_UNUSED);
	}

	if (rc != SQL_SUCCESS && ((m_StmtType == TYPE_QS_OPEN) || (m_StmtType == TYPE_CMD_OPEN))) m_intStmtType = TYPE_UNKNOWN;	
	return rc;
}

SQLRETURN CStmt::Prepare(SQLCHAR *StatementText,
					SQLINTEGER TextLength)
{
	SQLRETURN	rc;
	BOOL		SkipProcess = FALSE;

	m_CurrentOdbcAPI = SQL_API_SQLPREPARE;
	m_NumResultCols = 0;
#ifdef ASYNCIO
	if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
	{
		if (m_AsyncCanceled == TRUE)
		{
			#ifdef __DEBUGASYNCIO
			printf("CStmt::Prepare: Statement %x on Connection %x was cancelled\n",this,m_ConnectHandle);
			#endif
			rc = SQL_ERROR;
		}
		else
		{
			if(m_StmtExecState == STMT_EXECUTION_EXECUTING)
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::Prepare: Statement %x still executing, connection = %x\n",this,m_ConnectHandle);
				#endif

				rc = SQL_STILL_EXECUTING;
			}
			else if( m_StmtExecState == STMT_EXECUTION_DONE)
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::Prepare: Statement %x,Connection %x: Execution completed with returnCode %d\n",this,m_ConnectHandle,m_AsyncOperationReturnCode);
				#endif

				m_StmtExecState = STMT_EXECUTION_NONE;
				rc = m_AsyncOperationReturnCode;
			}
			else if( m_StmtExecState == STMT_EXECUTION_NONE)
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::Prepare: Statement %x,Connection %x: No Statement currently being executed on this statement handle\n",this,m_ConnectHandle);
				#endif

				rc = SendSQLCommand(FALSE, StatementText, TextLength);
			}
			else
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::Prepare: Unexpected Async Execution State: %d\n",m_StmtExecState);
				#endif
				assert(0);
			}
		}
	} // if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
	else
#endif /* ASYNCIO */
		rc = SendSQLCommand(SkipProcess, StatementText, TextLength);

	if(m_ConnectHandle->userStreams==0)
		m_ConnectHandle->m_StreamDelayedError = 0;
		
	switch (rc)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
		if (m_StmtType == TYPE_SELECT )
			m_StmtState = STMT_PREPARED;
		else
			m_StmtState = STMT_PREPARED_NO_RESULT;
		m_StmtPrepared = TRUE;
		setRowCount(-1);
		m_isClosed = false;
		break;
	case SQL_STILL_EXECUTING:
		m_StmtState = STMT_STILL_EXECUTING;
		break;
	case SQL_ERROR:
		if (m_AsyncCanceled == TRUE)
		{
			setDiagRec(DRIVER_ERROR, IDS_S1_008);
			m_AsyncCanceled = FALSE;
		}
		m_StmtState = STMT_ALLOCATED;
		setRowCount(-1);
		break;
	case SQL_NEED_DATA:
	case SQL_NO_DATA:
	case SQL_INVALID_HANDLE:
	default:
		break;
	}
	return rc;
}

SQLRETURN CStmt::ExecDirect(SQLCHAR *StatementText,
					SQLINTEGER TextLength)
{
	SQLRETURN	rc;
	BOOL		SkipProcess = FALSE;

	m_CurrentOdbcAPI = SQL_API_SQLEXECDIRECT;
#ifdef ASYNCIO
	if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
	{
		if (m_AsyncCanceled == TRUE)
		{
			rc = SQL_ERROR;
		}
		else
		{
			if(m_StmtExecState == STMT_EXECUTION_EXECUTING)
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::ExecDirect: Statement %x still executing, connection = %x\n",this,m_ConnectHandle);
				#endif

				rc = SQL_STILL_EXECUTING;
			}
			else if( m_StmtExecState == STMT_EXECUTION_DONE)
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::ExecDirect: Statement %x,Connection %x: Execution completed with returnCode %d\n",this,m_ConnectHandle,m_AsyncOperationReturnCode);
				#endif

				m_StmtExecState = STMT_EXECUTION_NONE;
				rc = m_AsyncOperationReturnCode;
			}
			else if( m_StmtExecState == STMT_EXECUTION_NONE)
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::ExecDirect: Statement %x,Connection %x: No Statement currently being executed on this statement handle\n",this,m_ConnectHandle);
				#endif

				rc = SendSQLCommand(FALSE, StatementText, TextLength);
			}
			else
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::ExecDirect: Unexpected Async Execution State: %d\n",m_StmtExecState);
				#endif
				assert(0);
			}
		}
	}
	else
		rc = SendSQLCommand(FALSE, StatementText, TextLength);
#else
	rc = SendSQLCommand(FALSE, StatementText, TextLength);
#endif /* ASYNCIO */

	switch (rc)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
		// If it is SQLExecute or SQLExecDirect, then SQLExecDirect is done
		if (m_SrvrCallContext.odbcAPI != SQL_API_SQLPREPARE)
		{
			if (m_StmtType == TYPE_SELECT || m_StmtType == TYPE_STATS || m_StmtType == TYPE_CALL) //  added for INFOSTATS support
				m_StmtState = STMT_EXECUTED;
			else
				m_StmtState = STMT_EXECUTED_NO_RESULT;
		}
		m_StmtPrepared = FALSE;
		m_isClosed = false;
		if (m_StmtQueryType == SQL_SELECT_UNIQUE && m_RowCount == 0)
			m_StmtState = STMT_EXECUTED_NO_RESULT;
		break;
	case SQL_STILL_EXECUTING:
		m_StmtState = STMT_STILL_EXECUTING;
		break;
	case SQL_NEED_DATA:
		m_StmtState = STMT_PARAM_DATA_NOT_CALLED;
		break;
	case SQL_ERROR:
		if (m_AsyncCanceled == TRUE)
		{
			setDiagRec(DRIVER_ERROR, IDS_S1_008);
			m_AsyncCanceled = FALSE;
		}
		if (m_StmtState != STMT_EXECUTED)
			m_StmtState = STMT_ALLOCATED;
		setRowCount(-1);
		break;
	case SQL_NO_DATA:
		if (m_StmtQueryType == SQL_SELECT_UNIQUE)
		{
			m_StmtState = STMT_EXECUTED_NO_RESULT;
			rc = SQL_SUCCESS;
		}
	case SQL_INVALID_HANDLE:
	default:
		break;
	}
	return rc;
}

SQLRETURN CStmt::setDescRec(const SQLItemDescList_def *IPDDescList, const SQLItemDescList_def *IRDDescList)
{

	CDesc			*pDesc;
	unsigned long	retCode = SQL_SUCCESS;
	unsigned long	retCode1;
	
	if (IPDDescList != NULL)
	{
		pDesc = m_ImpParamDesc;
		if (pDesc != SQL_NULL_HDESC)
		{
			retCode = pDesc->setDescRec(IPDDescList);
			if (retCode != SQL_SUCCESS)
				setDiagRec(DRIVER_ERROR, retCode);
		}
		m_NumParams = IPDDescList->_length;
	}
	else
		m_NumParams = 0;

	if (IRDDescList != NULL)
	{
		pDesc = m_ImpRowDesc;
		if (pDesc != SQL_NULL_HDESC)
		{
			retCode1 = pDesc->setDescRec(IRDDescList);
			if (retCode1 != SQL_SUCCESS)
			{
				setDiagRec(DRIVER_ERROR, retCode1);
				retCode = retCode1;
			}
		}
		m_NumResultCols = IRDDescList->_length;
	}
	else
		m_NumResultCols = 0;
	// Reset the FetchRelated members, 
	m_ResultsetRowsFetched = 0;
	m_RowsFetched = -1;
	m_CurrentRowFetched = 0;
	m_CurrentRowInRowset = 0;
	m_RowsetSize = 0;
	m_RowNumber = 0;

	if(this->getStmtQueryType() == SQL_INSERT_RWRS)
	   this->m_RWRS_Descriptor = new RWRS_Descriptor(m_ImpParamDesc);

	return retCode;
}

SQLRETURN CStmt::setStmtData(BYTE *&inputParams, BYTE *&outputColumns)
{
	unsigned long retCode = SQL_SUCCESS;
	long tmpLen = 0;

	if (inputParams != NULL)
	{
		tmpLen = *(IDL_long *)(inputParams);

		if ((m_InputParams = new BYTE[tmpLen]) == NULL)
		{
			m_InputParams = 0;
			return false;
		}
		else
			memcpy(m_InputParams, inputParams, tmpLen);
	}
	else
		m_InputParams = 0;

	if (outputColumns != NULL)
	{
		tmpLen = *(IDL_long *)(outputColumns);
		if ((m_OutputColumns = new BYTE[tmpLen]) == NULL)
		{
			m_OutputColumns = 0;
			return false;
		}
		else
			memcpy(m_OutputColumns, outputColumns, tmpLen);
	}
	else
		m_OutputColumns = 0;

	return retCode;
}

SQLRETURN CStmt::getDescRec(short odbcAPI,
			SQLUSMALLINT ColumnNumber, 
			SQLCHAR *ColumnName,
			SQLSMALLINT BufferLength, 
			SQLSMALLINT *NameLengthPtr,
		    SQLSMALLINT *DataTypePtr, 
			SQLULEN *ColumnSizePtr,
			SQLSMALLINT *DecimalDigitsPtr,
			SQLSMALLINT *NullablePtr)
{
	CDesc			*pDesc;
	unsigned long	retCode;
	SQLRETURN		rc = SQL_SUCCESS;
	UCHAR			errorMsg[MAX_TRANSLATE_ERROR_MSG_LEN];

	clearError();
	m_CurrentOdbcAPI = odbcAPI;
	if (ColumnNumber == 0)
	{
		setDiagRec(DRIVER_ERROR, IDS_07_009);
		return SQL_ERROR;
	}

	switch (odbcAPI)
	{
	case SQL_API_SQLDESCRIBECOL:
		if ((m_StmtState == STMT_EXECUTED_NO_RESULT) &&
                    (m_StmtQueryType != SQL_SELECT_UNIQUE))
		{
			setDiagRec(DRIVER_ERROR, IDS_24_000);
			return SQL_ERROR;
		}
		if (m_StmtState == STMT_PREPARED_NO_RESULT)
		{
			setDiagRec(DRIVER_ERROR, IDS_07_005);
			return SQL_ERROR;
		}
		pDesc = m_ImpRowDesc;
		break;
	case SQL_API_SQLDESCRIBEPARAM :
		pDesc = m_ImpParamDesc;
		break;
	default:
		setDiagRec(DRIVER_ERROR, IDS_HY_000);
		return SQL_ERROR;
	}
	retCode = pDesc->getDescRec(ColumnNumber, ColumnName, BufferLength, NameLengthPtr,
			DataTypePtr, ColumnSizePtr, DecimalDigitsPtr, NullablePtr, errorMsg, sizeof(errorMsg));
	if (retCode != SQL_SUCCESS)
	{
		if (errorMsg[0] != '\0')
			setDiagRec(DRIVER_ERROR, retCode, 0, (char *)errorMsg);
		else
			setDiagRec(DRIVER_ERROR, retCode);
		switch (retCode)
		{
			case IDS_01_004:
			case IDS_186_DSTODRV_TRUNC:
				rc = SQL_SUCCESS_WITH_INFO;
				break;
			default:
				rc = SQL_ERROR;
				break;
		}
	}
	return rc;
}

void CStmt::getStmtData(BYTE *&inputParams, BYTE *&outputColumns)
{
	inputParams = m_InputParams;
	outputColumns = m_OutputColumns;
}
	
SQLRETURN CStmt::getDescSize(short odbcAPI, SQLSMALLINT *CountPtr)
{
	SQLRETURN		rc = SQL_SUCCESS;
	
	clearError();
	m_CurrentOdbcAPI = odbcAPI;
	switch (odbcAPI)
	{
	case SQL_API_SQLNUMRESULTCOLS:
		if (CountPtr != NULL)
			*CountPtr = m_NumResultCols;
		break;
	case SQL_API_SQLNUMPARAMS:
		if (CountPtr != NULL)
			*CountPtr = m_NumParams;
		break;
	default:
		setDiagRec(DRIVER_ERROR, IDS_HY_000);
		return SQL_ERROR;
	}
	return rc;
}

SQLRETURN CStmt::FreeStmt(short odbcAPI, SQLUSMALLINT Option)
{
	CDesc			*pDesc;
	SQLRETURN		rc = SQL_SUCCESS;

	clearError();
	m_CurrentOdbcAPI = odbcAPI;
	if (odbcAPI == SQL_API_SQLCLOSECURSOR || Option == SQL_CLOSE || Option == SQL_DROP)
		setRowCount(-1);
	switch (Option)
	{
	case SQL_UNBIND:
		pDesc = m_AppRowDesc;
		if (pDesc != NULL)
			pDesc->clear();
		break;
	case SQL_RESET_PARAMS:
		pDesc = m_AppParamDesc;
		if (pDesc != NULL)
			pDesc->clear();
		InitInputValueList();
		break;
	case SQL_CLOSE:
		switch (odbcAPI)
		{
		case SQL_API_SQLFREESTMT:
			switch (m_StmtState)
			{
			case STMT_ALLOCATED:
			case STMT_PREPARED:
			case STMT_PREPARED_NO_RESULT:
			case STMT_EXECUTED_NO_RESULT:
				if(m_spjNumResultSets > 0)
				{
					m_spjNumResultSets = 0;
					m_spjCurrentResultSetIndex = 0;
					delete[] m_spjResultSets;
					m_spjResultSets = NULL;
				}
				return SQL_SUCCESS;
			default:
				break;
			}
			break;
		case SQL_API_SQLCLOSECURSOR:
			switch (m_StmtState)
			{
			case STMT_ALLOCATED:
			case STMT_PREPARED:
			case STMT_PREPARED_NO_RESULT:
			case STMT_EXECUTED_NO_RESULT:
				if(m_spjNumResultSets > 0)
				{
					m_spjNumResultSets = 0;
					m_spjCurrentResultSetIndex = 0;
					delete[] m_spjResultSets;
					m_spjResultSets = NULL;
				}
				if(m_StmtQueryType != SQL_SELECT_UNIQUE)
				{
				   setDiagRec(DRIVER_ERROR, IDS_24_000);
				   return SQL_ERROR;
				}
			default:
				break;
			}
			break;
		case SQL_API_SQLMORERESULTS:
			switch (m_StmtState)
			{
			case STMT_ALLOCATED:
			case STMT_PREPARED:
			case STMT_PREPARED_NO_RESULT:
				return SQL_NO_DATA;
			case STMT_EXECUTED_NO_RESULT:
				return SQL_NO_DATA;
			case STMT_FETCHED_TO_END:
				if((m_spjNumResultSets > 0) && (++m_spjCurrentResultSetIndex < m_spjNumResultSets))
				{
					setDescRec(&m_spjInputParamDesc, &m_spjResultSets[m_spjCurrentResultSetIndex].spjOutputItemDesc);
					return SQL_SUCCESS;
				}
				else
					rc = SQL_NO_DATA;
			default:
				break;
			}
		default:				// Want to fall thru
			break;
		}
		rc = Close(Option);
		switch (rc)
		{
		case SQL_SUCCESS:
		case SQL_SUCCESS_WITH_INFO:
			revertStmtState();
			rc = SQL_SUCCESS;
			break;
		case SQL_ERROR:
		case SQL_STILL_EXECUTING:
		case SQL_NEED_DATA:
		case SQL_NO_DATA:
		case SQL_INVALID_HANDLE:
		default:
			break;
		}
		if (odbcAPI == SQL_API_SQLMORERESULTS)
			rc = SQL_NO_DATA;
		break;
	default:
		setDiagRec(DRIVER_ERROR, IDS_HY_092);
		rc = SQL_ERROR;
	}
	return rc;
}


SQLRETURN CStmt::SendGetSQLCatalogs(short odbcAPI,
				BOOL SkipProcess,
			SQLCHAR *CatalogName, 
			SQLSMALLINT NameLength1,
			SQLCHAR *SchemaName, 
			SQLSMALLINT NameLength2,
			SQLCHAR *TableName, 
			SQLSMALLINT NameLength3,
			SQLCHAR *ColumnName, 
			SQLSMALLINT NameLength4,
			SQLCHAR *TableType, 
			SQLSMALLINT NameLength5,
			SQLUSMALLINT IdentifierType,
		   	SQLUSMALLINT Scope,
			SQLUSMALLINT Nullable,
			SQLSMALLINT SqlType,
			SQLUSMALLINT Unique,
			SQLUSMALLINT Reserved,
		    SQLCHAR *FKCatalogName, 
		    SQLSMALLINT NameLength6,
            SQLCHAR *FKSchemaName, 
		    SQLSMALLINT NameLength7,
            SQLCHAR *FKTableName, 
		    SQLSMALLINT NameLength8)
{
	SQLRETURN			rc;
	char				*cEmpty = "";
	char				*cAll = '\0';
	SQLCHAR				*substCatalogName;
	SQLSMALLINT			catalogNameLen;
	SQLCHAR				*substSchemaName;
	SQLSMALLINT			schemaNameLen;
	SQLCHAR				*substTableName;
	SQLSMALLINT			tableNameLen;
	SQLCHAR				*substColumnName;
	SQLSMALLINT			columnNameLen;
	SQLCHAR				*substTableType;
	SQLSMALLINT			tableTypeLen;
	SQLINTEGER			translateLength;
	SQLCHAR				*FKsubstCatalogName;
	SQLSMALLINT			FKcatalogNameLen;
	SQLCHAR				*FKsubstSchemaName;
	SQLSMALLINT			FKschemaNameLen;
	SQLCHAR				*FKsubstTableName;
	SQLSMALLINT			FKtableNameLen;
	bool				IsCase = FALSE;
	DWORD				translateOption;
	bool				internalData = false;
	int					caseSensitivity = CASE_NONE;
	int 				transLen = 0;
	char				errorMsg[MAX_TRANSLATE_ERROR_MSG_LEN];
	int					allowedIdentifierLen;

	if (m_ICUConv->m_AppType == APP_TYPE_ANSI)
		allowedIdentifierLen = MAX_SQL_IDENTIFIER_LEN+1;
	else
		if (m_ICUConv->m_AppUnicodeType == APP_UNICODE_TYPE_UTF16)
			allowedIdentifierLen = (MAX_SQL_IDENTIFIER_LEN+1)*2;
		else
			allowedIdentifierLen = (MAX_SQL_IDENTIFIER_LEN+1)*4;

	if (m_ConnectHandle->getConnectionStatus() == SQL_CD_TRUE)
	{
		clearError();
		setDiagRec(DRIVER_ERROR, IDS_08_S01, -20005);
		return SQL_ERROR;
	}

	if (odbcAPI == SQL_API_SQLGETTYPEINFO)
	{
		switch (SqlType)
		{
			case SQL_BIGINT:
//			case SQL_BIT:
//			case SQL_BINARY:
			case SQL_CHAR:
			case SQL_WCHAR:
			case SQL_DATE:
			case SQL_DECIMAL:
			case SQL_DOUBLE:
			case SQL_FLOAT:
			case SQL_INTEGER:
//			case SQL_LONGVARBINARY:
			case SQL_LONGVARCHAR:
			case SQL_NUMERIC:
			case SQL_REAL:
			case SQL_SMALLINT:
			case SQL_TINYINT:
			case SQL_TIME:
			case SQL_TIMESTAMP:
			case SQL_TYPE_DATE:
			case SQL_TYPE_TIME:
			case SQL_TYPE_TIMESTAMP:
//			case SQL_VARBINARY:
			case SQL_VARCHAR:
			case SQL_WVARCHAR:
			case SQL_INTERVAL_YEAR:
			case SQL_INTERVAL_MONTH:
			case SQL_INTERVAL_DAY:
			case SQL_INTERVAL_HOUR:
			case SQL_INTERVAL_MINUTE:
			case SQL_INTERVAL_SECOND:
			case SQL_INTERVAL_YEAR_TO_MONTH:
			case SQL_INTERVAL_DAY_TO_HOUR:
			case SQL_INTERVAL_DAY_TO_MINUTE:
			case SQL_INTERVAL_DAY_TO_SECOND:
			case SQL_INTERVAL_HOUR_TO_MINUTE:
			case SQL_INTERVAL_HOUR_TO_SECOND:
			case SQL_INTERVAL_MINUTE_TO_SECOND:
			case SQL_ALL_TYPES:
				break;
			default:
				setDiagRec(DRIVER_ERROR, IDS_HY_004);
				return SQL_ERROR;
		}
	}

	if (! SkipProcess)
	{
		clearError();
		setDiagRowCount(-1, -1);
	
		if (m_StmtState == STMT_FETCHED)
		{
			setDiagRec(DRIVER_ERROR, IDS_24_000);
			return SQL_ERROR;
		}
		
		// Passing a null pointer to a search pattern argument does not constrain
		// the search for that argument; that is, a null pointer and the search pattern % 
		// (any characters) are equivalent. However, a zero-length search pattern that is, 
		// a valid pointer to a string of length zero matches only the empty string (""). - ODBC Manual
		
		//Check the m_MetaDataId to see whether case sensitivity has to be preserved
		caseSensitivity = m_ConnectHandle->getCaseSensitivity();
		
		if (CatalogName == NULL)
		{
			if (true)
				substCatalogName = (SQLCHAR *)getCurrentCatalog();
			else
				substCatalogName = (SQLCHAR *)SQL_ALL_CATALOGS;
			catalogNameLen = strlen((const char *)substCatalogName);
			internalData = true;  
		}
		else
		{
			catalogNameLen = m_ConnectHandle->m_ICUConv->FindStrLength((const char*)CatalogName, NameLength1);
			substCatalogName = (SQLCHAR *)CatalogName;
			internalData = false;
		}
		
		if (m_ConnectHandle->m_ICUConv->InArgTranslationHelper(
												substCatalogName,
												catalogNameLen,
												m_CatalogName,
												allowedIdentifierLen,
												&transLen,
												errorMsg,
												internalData,
												caseSensitivity) != SQL_SUCCESS)
			return SQL_ERROR; 

		if (SchemaName == NULL)
		{
			substSchemaName = (SQLCHAR *)SQL_ALL_SCHEMAS;
			schemaNameLen = strlen((const char *)substSchemaName);
			internalData = true;  
		}
		else
		{
			substSchemaName = SchemaName;
			schemaNameLen = m_ConnectHandle->m_ICUConv->FindStrLength((const char*)SchemaName, NameLength2);
			internalData = false;
		}
		if (m_ConnectHandle->m_ICUConv->InArgTranslationHelper(
														substSchemaName,
														schemaNameLen,
														m_SchemaName,
														allowedIdentifierLen,
														&transLen,
														errorMsg,
														internalData,
														caseSensitivity) != SQL_SUCCESS)
			return SQL_ERROR;

		if (TableName == NULL)
		{
			if (odbcAPI != SQL_API_SQLFOREIGNKEYS)
				substTableName = (SQLCHAR *)cAll;
			else
				substTableName = (SQLCHAR *)cEmpty;
//			tableNameLen = strlen((const char *)substTableName);
			tableNameLen = 0; 
			internalData = true;  
		}
		else
		{
			substTableName = TableName;
			tableNameLen = m_ConnectHandle->m_ICUConv->FindStrLength((const char*)TableName, NameLength3);
			internalData = false;
		}

		if (m_ConnectHandle->m_ICUConv->InArgTranslationHelper(
													substTableName,
													tableNameLen,
													m_TableName,
													allowedIdentifierLen,
													&transLen,
													errorMsg,
													internalData,
													caseSensitivity) != SQL_SUCCESS)
			return SQL_ERROR;

		if (ColumnName == NULL)
		{
			substColumnName = (SQLCHAR *)cAll;
//			columnNameLen = strlen((const char *)cAll);
			columnNameLen = 0;
			internalData = true; 
		}
		else
		{			
			substColumnName = ColumnName;
			columnNameLen = m_ConnectHandle->m_ICUConv->FindStrLength((const char*)ColumnName, NameLength4);
			internalData = false; 
		}

		if (m_ConnectHandle->m_ICUConv->InArgTranslationHelper(
											substColumnName,
											columnNameLen,
											m_ColumnName,
											allowedIdentifierLen,
											&transLen,
											errorMsg,
											internalData,
											caseSensitivity) != SQL_SUCCESS)
			return SQL_ERROR;

		if (TableType == NULL)
		{
			substTableType = (SQLCHAR *)SQL_ALL_TABLE_TYPES;
			tableTypeLen = strlen((const char *)substTableType);
			internalData = true;
		}
		else
		{
			substTableType = TableType;
			tableTypeLen = m_ConnectHandle->m_ICUConv->FindStrLength((const char*)TableType, NameLength5);
			internalData = false;
		}

		if (m_ConnectHandle->m_ICUConv->InArgTranslationHelper(
											substTableType,
											tableTypeLen,
											m_TableType,
											allowedIdentifierLen,
											&transLen,
											errorMsg,
											internalData,
											caseSensitivity) != SQL_SUCCESS)
			return SQL_ERROR;

		//  Added to support SQLForeignKeys
		if (FKCatalogName == NULL)
		{
			if (isNskVersion())
				FKsubstCatalogName = (SQLCHAR *)getCurrentCatalog();
			else
				FKsubstCatalogName = (SQLCHAR *)SQL_ALL_CATALOGS;
			FKcatalogNameLen = strlen((const char *)FKsubstCatalogName);
			internalData = true;
		}
		else
		{
			FKsubstCatalogName = FKCatalogName;
			FKcatalogNameLen = m_ConnectHandle->m_ICUConv->FindStrLength((const char*)FKsubstCatalogName, NameLength6);
			internalData = false;
		}

		if (m_ConnectHandle->m_ICUConv->InArgTranslationHelper(
											FKsubstCatalogName,
											FKcatalogNameLen,
											m_FKCatalogName,
											allowedIdentifierLen,
											&transLen,
											errorMsg,
											internalData,
											caseSensitivity) != SQL_SUCCESS)
			return SQL_ERROR;

		if (FKSchemaName == NULL)
		{
			if (isNskVersion())
				FKsubstSchemaName = (SQLCHAR *)getCurrentSchema();
			else
				FKsubstSchemaName = (SQLCHAR *)SQL_ALL_SCHEMAS;
			FKschemaNameLen = strlen((const char *)FKsubstSchemaName);
			internalData = true;
		}
		else
		{
			FKsubstSchemaName = FKSchemaName;
			FKschemaNameLen = m_ConnectHandle->m_ICUConv->FindStrLength((const char*)FKSchemaName, NameLength7);
			internalData = false;
		}

		if (m_ConnectHandle->m_ICUConv->InArgTranslationHelper(
											FKsubstSchemaName,
											FKschemaNameLen,
											m_FKSchemaName,
											allowedIdentifierLen,
											&transLen,
											errorMsg,
											internalData,
											caseSensitivity) != SQL_SUCCESS)
		return SQL_ERROR;

		if (FKTableName == NULL)
		{
			FKsubstTableName = (SQLCHAR *)cEmpty;
			FKtableNameLen = strlen((const char *)FKsubstTableName);
		}
		else
		{
			FKsubstTableName = FKTableName;
			FKtableNameLen = m_ConnectHandle->m_ICUConv->FindStrLength((const char*)FKTableName, NameLength8);
		}

		if (m_ConnectHandle->m_ICUConv->InArgTranslationHelper(
											FKsubstTableName,
											FKtableNameLen,
											m_FKTableName,
											allowedIdentifierLen,
											&transLen,
											errorMsg,
											internalData,
											caseSensitivity) != SQL_SUCCESS)
		return SQL_ERROR;

		m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.catalogNm = m_CatalogName;
		m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.schemaNm = m_SchemaName;
		m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.tableNm = m_TableName;
		m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.columnNm =  m_ColumnName;
		m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.tableTypeList = m_TableType;
		m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.columnType = IdentifierType;
		m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.rowIdScope = Scope;
		m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.nullable = Nullable;
		m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.uniqueness = Unique;
		m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.accuracy = Reserved;
		m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.fkcatalogNm = m_FKCatalogName;
		m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.fkschemaNm = m_FKSchemaName;
		m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.fktableNm = m_FKTableName;

#if (ODBCVER < 0x0300)
			switch (SqlType)
			{
				case SQL_TYPE_DATE:
					m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.sqlType = SQL_DATE;
					break;
				case SQL_TYPE_TIME:
					m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.sqlType = SQL_TIME;
					break;
				case SQL_TYPE_TIMESTAMP:
					m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.sqlType = SQL_TIMESTAMP;
					break;
				case SQL_INTERVAL_YEAR:
					m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.sqlType = -80;
					break;
				case SQL_INTERVAL_MONTH:
					m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.sqlType = -81;
					break;
				case SQL_INTERVAL_YEAR_TO_MONTH:
					m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.sqlType = -82;
					break;
				case SQL_INTERVAL_DAY:
					m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.sqlType = -83;
					break;
				case SQL_INTERVAL_HOUR:
					m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.sqlType = -84;
					break;
				case SQL_INTERVAL_MINUTE:
					m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.sqlType = -85;
					break;
				case SQL_INTERVAL_SECOND:
					m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.sqlType = -86;
					break;
				case SQL_INTERVAL_DAY_TO_HOUR:
					m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.sqlType = -87;
					break;
				case SQL_INTERVAL_DAY_TO_MINUTE:
					m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.sqlType = -88;
					break;
				case SQL_INTERVAL_DAY_TO_SECOND:
					m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.sqlType = -89;
					break;
				case SQL_INTERVAL_HOUR_TO_MINUTE:
					m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.sqlType = -90;
					break;
				case SQL_INTERVAL_HOUR_TO_SECOND:
					m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.sqlType = -91;
					break;
				case SQL_INTERVAL_MINUTE_TO_SECOND:
					m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.sqlType = -92;
					break;
				default:
					m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.sqlType = SqlType;
					break;
			}
#else
			m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.sqlType = SqlType;
#endif
		m_SrvrCallContext.u.getSQLCatalogsParams.catalogAPIParams.metadataId = m_MetadataId;
		m_SrvrCallContext.u.getSQLCatalogsParams.stmtLabel = m_StmtLabel;
		m_SrvrCallContext.odbcAPI = odbcAPI;
		m_SrvrCallContext.sqlHandle = this;
		m_SrvrCallContext.SQLSvc_ObjRef = m_ConnectHandle->getSrvrObjRef();
		m_SrvrCallContext.ASSvc_ObjRef = NULL;
		m_SrvrCallContext.eventHandle = m_StmtEvent;
		m_SrvrCallContext.dialogueId = m_ConnectHandle->getDialogueId();
		m_SrvrCallContext.connectionTimeout = m_ConnectHandle->getConnectionTimeout();
		m_SrvrCallContext.statementTimeout  = m_QueryTimeout;
		m_StmtType = TYPE_SELECT;			// Since catalog APIs produce result set
		
	}	
#ifdef ASYNCIO
	if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
	{
		rc = StartAsyncOperation(m_ConnectHandle, this /* statement handle */);

		if(rc == SQL_SUCCESS)
			rc = SQL_STILL_EXECUTING;
		else
			rc = SQL_ERROR;
	}
	else
		rc = ThreadControlProc(&m_SrvrCallContext);
#else
	rc = ThreadControlProc(&m_SrvrCallContext);
#endif
	return rc;
}

SQLRETURN CStmt::GetSQLCatalogs(short odbcAPI, 
			SQLCHAR *CatalogName, 
			SQLSMALLINT NameLength1,
			SQLCHAR *SchemaName, 
			SQLSMALLINT NameLength2,
			SQLCHAR *TableName, 
			SQLSMALLINT NameLength3,
			SQLCHAR *ColumnName, 
			SQLSMALLINT NameLength4,
			SQLCHAR *TableType, 
			SQLSMALLINT NameLength5,
			SQLUSMALLINT IdentifierType,
		   	SQLUSMALLINT Scope,
			SQLUSMALLINT Nullable,
			SQLSMALLINT SqlType,
			SQLUSMALLINT Unique,
			SQLUSMALLINT Reserved,
		    SQLCHAR *FKCatalogName, 
		    SQLSMALLINT NameLength6,
            SQLCHAR *FKSchemaName, 
		    SQLSMALLINT NameLength7,
            SQLCHAR *FKTableName, 
		    SQLSMALLINT NameLength8)
{
	SQLRETURN	rc;
	BOOL		SkipProcess = FALSE;

	m_CurrentOdbcAPI = odbcAPI;

#ifdef ASYNCIO
	if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
	{
		if (m_AsyncCanceled == TRUE)
		{
			rc = SQL_ERROR;
		}
		else
		{
			if(m_StmtExecState == STMT_EXECUTION_EXECUTING)
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::GetSQLCatalogs: Statement %x still executing, connection = %x\n",this,m_ConnectHandle);
				#endif

				rc = SQL_STILL_EXECUTING;
			}
			else if( m_StmtExecState == STMT_EXECUTION_DONE)
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::GetSQLCatalogs: Statement %x,Connection %x: Execution completed with returnCode %d\n",this,m_ConnectHandle,m_AsyncOperationReturnCode);
				#endif

				m_StmtExecState = STMT_EXECUTION_NONE;
				rc = m_AsyncOperationReturnCode;
			}
			else if( m_StmtExecState == STMT_EXECUTION_NONE)
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::GetSQLCatalogs: Statement %x,Connection %x: No Statement currently being executed on this statement handle\n",this,m_ConnectHandle);
				#endif

				rc = SendGetSQLCatalogs(odbcAPI, SkipProcess, CatalogName, NameLength1, SchemaName, 
						NameLength2, TableName, NameLength3, ColumnName, NameLength4,
						TableType, NameLength5, IdentifierType, Scope, Nullable, SqlType, Unique, Reserved,
						FKCatalogName, NameLength6, FKSchemaName, NameLength7, FKTableName, NameLength8);

			}
			else
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::GetSQLCatalogs: Unexpected Async Execution State: %d\n",m_StmtExecState);
				#endif
				assert(0);
			}
		}
	}
	else
		rc = SendGetSQLCatalogs(odbcAPI, SkipProcess, CatalogName, NameLength1, SchemaName, 
				NameLength2, TableName, NameLength3, ColumnName, NameLength4,
				TableType, NameLength5, IdentifierType, Scope, Nullable, SqlType, Unique, Reserved,
				FKCatalogName, NameLength6, FKSchemaName, NameLength7, FKTableName, NameLength8);
#else
		rc = SendGetSQLCatalogs(odbcAPI, SkipProcess, CatalogName, NameLength1, SchemaName, 
				NameLength2, TableName, NameLength3, ColumnName, NameLength4,
				TableType, NameLength5, IdentifierType, Scope, Nullable, SqlType, Unique, Reserved,
				FKCatalogName, NameLength6, FKSchemaName, NameLength7, FKTableName, NameLength8);
#endif /* ASYNCIO */
	switch (rc)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
		m_StmtState = STMT_EXECUTED;
		m_isClosed = false;
		break;
	case SQL_STILL_EXECUTING:
		m_StmtState = STMT_STILL_EXECUTING;
		break;
	case SQL_ERROR:
		if (m_AsyncCanceled == TRUE)
		{
			setDiagRec(DRIVER_ERROR, IDS_S1_008);
			m_AsyncCanceled = FALSE;
		}
		m_StmtState = STMT_ALLOCATED;
		setRowCount(-1);
		break;
	case SQL_NEED_DATA:
	case SQL_NO_DATA:
	case SQL_INVALID_HANDLE:
	default:
		break;
	}
	return rc;
}

SQLRETURN  CStmt::GetCursorName(SQLCHAR *CursorName, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *NameLengthPtr)
{

	SQLRETURN	rc = SQL_SUCCESS;
	short		strLen =0;
	SQLINTEGER	translateLength;
	char		errorMsg[MAX_TRANSLATE_ERROR_MSG_LEN];
	unsigned long	retCode;

	clearError();
	m_CurrentOdbcAPI = SQL_API_SQLGETCURSORNAME;
	if(m_CursorName[0] != '\0')
		strLen = strlen((const char *)m_CursorName);
	else
		strLen = strlen((const char *)m_StmtLabel);
	if (CursorName != NULL)
	{
		if(m_CursorName[0] != '\0')
		{
			// BufferLength [Input] Length of *CursorName, in bytes. It must be an even number according to spec
			if((rc = m_ICUConv->OutArgTranslationHelper( (SQLCHAR *)m_CursorName, strLen, (char*)CursorName, BufferLength,
								&translateLength, NULL, errorMsg)) != SQL_SUCCESS)
			{
				setDiagRec(DRIVER_ERROR, IDS_190_DSTODRV_ERROR, 0, (char *)errorMsg);
			}

		}
		else
		{
			// BufferLength [Input] Length of *CursorName, in bytes. It must be an even number according to spec
			if((rc = m_ICUConv->OutArgTranslationHelper((SQLCHAR *)m_StmtLabel, strLen, (char*)CursorName, BufferLength,
										&translateLength,NULL, errorMsg)) != SQL_SUCCESS)
			{
				setDiagRec(DRIVER_ERROR, IDS_190_DSTODRV_ERROR, 0, (char *)errorMsg);
			}
		}
		strLen = translateLength;
	}
	else
		if(m_ICUConv->isAppUTF16())
			strLen *= 2; 

	if (NameLengthPtr != NULL)
		*NameLengthPtr = strLen;
	return rc ;
}
	

SQLRETURN CStmt::SetCursorName(SQLCHAR *CursorName, 
		   SQLSMALLINT NameLength)
{
	SQLRETURN	rc = SQL_SUCCESS;
	SQLINTEGER	translateLength;
	std::list<CHandlePtr>::iterator i;
	char		errorMsg[MAX_TRANSLATE_ERROR_MSG_LEN];
	CStmt* pStmt;
	int transLen = 0;

	clearError();
	m_CurrentOdbcAPI = SQL_API_SQLSETCURSORNAME;
	int strLen = m_ICUConv->FindStrLength((const char *)CursorName, NameLength);
	rc = m_ICUConv->InArgTranslationHelper((SQLCHAR *)CursorName, strLen, m_CursorName, sizeof(m_CursorName),
											&transLen, errorMsg, false, CASE_NONE, true);
	if (rc != SQL_SUCCESS)
	{
		setDiagRec(DRIVER_ERROR, IDS_01_004, 0, (char *)errorMsg);
		return rc;
	}
	if((strncmp(m_CursorName, "SQLCUR", 6) == 0) || (strncmp(m_CursorName, "SQL_CUR", 7) == 0))
	{
		setDiagRec(DRIVER_ERROR, IDS_34_000);
		return SQL_ERROR;
	}
	for (i = m_ConnectHandle->m_StmtCollect.begin() ; i !=  m_ConnectHandle->m_StmtCollect.end() ; ++i)
	{
		pStmt = (CStmt*)(*i); 
		if ((pStmt != NULL ) &&
			((strncmp(pStmt->m_CursorName, m_CursorName, strlen(m_CursorName))) == 0) && (pStmt != this))
		{
			setDiagRec(DRIVER_ERROR, IDS_3C_000);
			return SQL_ERROR;
		}
	}
	return rc;
}

SQLRETURN CStmt::Cancel()
{
	int status;
	SQLRETURN rc = SQL_SUCCESS;
	odbcas_ASSvc_StopSrvr_exc_ stopSrvrException;
    stopSrvrException.exception_nr=SQL_ERROR;
	// Check if we are cancelling SQLParamData or SQLPutData API
	m_CurrentOdbcAPI = SQL_API_SQLCANCEL;
	switch (m_StmtState)
	{
	case STMT_PARAM_DATA_NOT_CALLED:
	case STMT_PUT_DATA_NOT_CALLED:
	case STMT_PUT_DATA_CALLED:
		revertStmtState();
		break;
	default:
		break;
	}

	#ifdef __DEBUGASYNCIO
	printf("Cancel: Statement %x on Connection %x\n",this,m_ConnectHandle);
	#endif

	if (m_AsyncCanceled == FALSE)
	{
		// If the Stmt is not currently executing, the behaviour depends on the ODBC version
		//    - For ODBC 2.x, SQLCancel has the same effect as SQLFreeStmt
		//    - For ODBC 3.x  SQLCancel is a NO-OP
		// send a sendStopServer request only is the SQL_ATTR_IGNORE_CANCEL is 0 (default)
		if(m_StmtExecState == STMT_EXECUTION_EXECUTING)
		{
		   if(m_ConnectHandle->m_IgnoreCancel == false)
		   {
				#ifdef ASYNCIO
				if(m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
				{
					// We're in asynchronous mode, we'll hand off the Cancel processing to the appropriate method
					rc = m_ConnectHandle->m_async_IOthread->Cancel(this);
				}
				else
				#endif
				{
					#ifdef __DEBUGASYNCIO
					printf("Cancel: Statement %x still executing synchronously on Connection %x, Killing Server\n",this,m_ConnectHandle);
					#endif
					m_AsyncCanceled = TRUE;
					sendStopServer(&stopSrvrException);
					m_StmtExecState = STMT_EXECUTION_NONE;
					rc = SQL_SUCCESS;
				}
		   }
		   else
		   {
		      setDiagRec(DRIVER_ERROR, IDS_HY_018);
		      rc = SQL_ERROR;
		   }
		}
		else if (getODBCAppVersion()== SQL_OV_ODBC2)
			rc = Close(SQL_CLOSE);
	}

	return rc;

} // CStmt::Cancel()

SQLRETURN CStmt::ColAttribute(SQLUSMALLINT ColumnNumber, 
			SQLUSMALLINT FieldIdentifier,
			SQLPOINTER CharacterAttributePtr, 
			SQLSMALLINT BufferLength,
			SQLSMALLINT *StringLengthPtr, 
			SQLPOINTER NumericAttributePtr)
{
	CDesc			*pDesc;
	SQLRETURN		rc = SQL_SUCCESS;
	SQLPOINTER		ValuePtr;
	SQLINTEGER		lStringLength;
	SQLSMALLINT		sTmp;
	SQLINTEGER		DataType;
	
	clearError();
	DataType = DRVR_PENDING;

	m_CurrentOdbcAPI = SQL_API_SQLCOLATTRIBUTE;
	if ((m_StmtState == STMT_EXECUTED_NO_RESULT) &&
            (m_StmtQueryType != SQL_SELECT_UNIQUE))
	{
		setDiagRec(DRIVER_ERROR, IDS_24_000);
		return SQL_ERROR;
	}
	if (m_StmtState == STMT_PREPARED_NO_RESULT && FieldIdentifier != SQL_DESC_COUNT)
	{
		setDiagRec(DRIVER_ERROR, IDS_07_005);
		return SQL_ERROR;
	}
	if (ColumnNumber == 0)
	{
		setDiagRec(DRIVER_ERROR, IDS_07_009);
		return SQL_ERROR;
	}

	
	pDesc = m_ImpRowDesc;
	switch (FieldIdentifier)
	{
	case SQL_DESC_AUTO_UNIQUE_VALUE:
	case SQL_DESC_CASE_SENSITIVE:
	case SQL_DESC_NUM_PREC_RADIX:
	case SQL_DESC_DISPLAY_SIZE:
	case SQL_DESC_OCTET_LENGTH:
		DataType = SQL_IS_INTEGER;
		ValuePtr = NumericAttributePtr;
		break;
	case SQL_DESC_LENGTH:
	case SQL_COLUMN_LENGTH:
	case SQL_COLUMN_PRECISION:
		DataType = SQL_IS_UINTEGER;
		ValuePtr = NumericAttributePtr;
		break;
    case SQL_DESC_COUNT: 
	case SQL_DESC_CONCISE_TYPE:
	case SQL_DESC_FIXED_PREC_SCALE:
	case SQL_DESC_PRECISION:
	case SQL_DESC_SCALE:
	case SQL_DESC_NULLABLE:
	case SQL_DESC_SEARCHABLE:
	case SQL_DESC_TYPE:
	case SQL_DESC_UNNAMED:
	case SQL_DESC_UNSIGNED:
	case SQL_DESC_UPDATABLE:
	case SQL_COLUMN_SCALE:
		DataType = SQL_IS_SMALLINT;
		ValuePtr = &sTmp;
		break;
	case SQL_DESC_BASE_COLUMN_NAME:
	case SQL_DESC_BASE_TABLE_NAME:
	case SQL_DESC_CATALOG_NAME:
	case SQL_DESC_LABEL:
	case SQL_DESC_LITERAL_PREFIX:
	case SQL_DESC_LITERAL_SUFFIX:
	case SQL_DESC_LOCAL_TYPE_NAME:
	case SQL_DESC_NAME:
	case SQL_DESC_SCHEMA_NAME:
	case SQL_DESC_TYPE_NAME:
	case SQL_DESC_TABLE_NAME:
		ValuePtr = CharacterAttributePtr;
		break;
	default:
		setDiagRec(DRIVER_ERROR, IDS_HY_091);
		return SQL_ERROR;
	}
	rc = pDesc->GetDescField(ColumnNumber, FieldIdentifier, ValuePtr, BufferLength, &lStringLength);

#if defined(__LP64__) && defined(BIGE)
   if(FieldIdentifier == SQL_DESC_DISPLAY_SIZE ||
      FieldIdentifier == SQL_DESC_OCTET_LENGTH ||
      FieldIdentifier == SQL_DESC_LENGTH ||
      FieldIdentifier == SQL_COLUMN_LENGTH)
   {
    // For these Fields a 8 byte container will be passed in, so 32 bit -> 64 bit 
	   if (NumericAttributePtr != NULL)
          *(SQLLEN *)NumericAttributePtr = *(SQLLEN *)NumericAttributePtr >> 32;
   }
#endif

	if (DataType == SQL_IS_SMALLINT)
	{
		if (NumericAttributePtr != NULL)
		{
#if defined(__LP64__) && defined(BIGE)
            if(FieldIdentifier != SQL_DESC_COUNT)
 		         *(SQLINTEGER *)NumericAttributePtr = sTmp;
			else
 		       *(SQLLEN *)NumericAttributePtr = sTmp;
#else
		    *(SQLINTEGER *)NumericAttributePtr = sTmp;
#endif

        }
	}
	if (DataType == DRVR_PENDING)
		if (StringLengthPtr != NULL)
			*StringLengthPtr = lStringLength;
	return rc;
}

SQLRETURN CStmt::SendExecute(BOOL SkipProcess)
{

	SQLRETURN		rc;
	SQLRETURN		rctrack = 0; //  To keep track of SUCCESS_WITH_INFO in case of data truncation.
	SQLSMALLINT		AppParamCount;
	SQLUINTEGER		AppArrayStatusSize;
	SQLSMALLINT		ParamNumber;
	SQLULEN 		RowNumber;
	SQLINTEGER		ODBCAppVersion;
	
	if (m_ConnectHandle->getConnectionStatus() == SQL_CD_TRUE)
	{
		clearError();
		setDiagRec(DRIVER_ERROR, IDS_08_S01, -20005);
		return SQL_ERROR;
	}
	if (! SkipProcess)
	{
		clearError();
		setDiagRowCount(-1, -1);
		if (m_StmtState == STMT_FETCHED)
		{
			setDiagRec(DRIVER_ERROR, IDS_24_000);
			return SQL_ERROR;
		}

		m_AppParamDesc->GetDescField(0, SQL_DESC_COUNT, &AppParamCount, SQL_IS_SMALLINT, NULL);
		if (AppParamCount < m_NumParams)
		{
			setDiagRec(DRIVER_ERROR, IDS_07_002);
			return SQL_ERROR;
		}

		InitInputValueList();
		ParamNumber = 0;
		RowNumber = 1;
		ODBCAppVersion = getODBCAppVersion();
		if (m_NumParams > 0)
		{
            if (m_StmtType == TYPE_INSERT_PARAM && !rowsetErrorRecovery())
            {       
#ifdef __LP64__
                m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &AppArrayStatusSize, SQL_IS_POINTER, NULL);
#else
                m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &AppArrayStatusSize, SQL_IS_UINTEGER, NULL);
#endif
                m_ImpParamDesc->setDescArrayStatus(AppArrayStatusSize,SQL_PARAM_UNUSED);
            }
			rc = m_AppParamDesc->BuildValueList(this, m_NumParams, m_ImpParamDesc,
						ODBCAppVersion, ParamNumber, RowNumber, &m_InputValueList, m_ParamBuffer);
			switch (rc)
			{
			case SQL_SUCCESS:
				break;
			case SQL_SUCCESS_WITH_INFO:
				rctrack = SQL_SUCCESS_WITH_INFO;
				break;
			case SQL_NEED_DATA:
				m_CurrentParam = ParamNumber;
				m_CurrentRow = RowNumber;
				m_CurrentParamStatus = SQL_DATA_AT_EXEC;
				return SQL_NEED_DATA;
			default:
				return SQL_ERROR;
			}
		}
		m_ImpParamDesc->setRowsProcessed(RowNumber);
		//  Added for Caffeine
		if (m_StmtQueryType == SQL_SELECT_UNIQUE)
		{
			// Reset the Fetch related member elements
			m_RowsFetched = -1;
			m_ImpRowDesc->setRowsProcessed(0);
			m_ImpRowDesc->setDescArrayStatus(1); 
		}
		if(m_NumParams > 0)
		{
			if(m_RowIdMap.size()==0)//all rows contained ERRORS
			{
				RowNumber = 0;
			}
			else if(RowNumber == m_RowIdMap.size())
				m_RowIdMap.clear();//don't need the row id map since each client side row id mathces the server side row id
			else //there are both, successful and failed rows
				RowNumber = m_RowIdMap.size();
		}
		if(RowNumber>0 || this->m_StmtQueryType == SQL_INSERT_RWRS)
		{
			m_SrvrCallContext.initContext();
			m_SrvrCallContext.odbcAPI = SQL_API_SQLEXECUTE;
			m_SrvrCallContext.sqlHandle = this;
			m_SrvrCallContext.SQLSvc_ObjRef = m_ConnectHandle->getSrvrObjRef();
			m_SrvrCallContext.ASSvc_ObjRef = NULL;
			m_SrvrCallContext.eventHandle = m_StmtEvent;
			m_SrvrCallContext.dialogueId = m_ConnectHandle->getDialogueId();
			m_SrvrCallContext.connectionTimeout = m_ConnectHandle->getConnectionTimeout();
			m_SrvrCallContext.u.executeParams.queryTimeout = m_QueryTimeout;
			m_SrvrCallContext.u.executeParams.stmtLabel = m_StmtLabel;
			m_SrvrCallContext.u.executeParams.cursorName = m_CursorName;
			m_SrvrCallContext.u.executeParams.sqlStmtType = m_StmtType;
			m_SrvrCallContext.u.executeParams.rowCount = RowNumber;
			m_SrvrCallContext.u.executeParams.inputValueList = &m_InputValueList;
			m_SrvrCallContext.u.executeParams.asyncEnable = m_AsyncEnable;
			m_SrvrCallContext.u.executeParams.holdableCursor = m_CursorHoldable;
		}
		else
		{
			setDiagRowCount(0, -1);
			setRowCount(0);
			return rc;
		}
	}
#ifdef ASYNCIO
	if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
	{
		rc = StartAsyncOperation(m_ConnectHandle, this /* statement handle */);

		if(rc == SQL_SUCCESS)
			rc = SQL_STILL_EXECUTING;
		else
			rc = SQL_ERROR;
	}
	else
		rc = ThreadControlProc(&m_SrvrCallContext);
#else
	rc = ThreadControlProc(&m_SrvrCallContext);
#endif /* ASYNCIO */


#if ASYNCIO
	if(m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
	{ 
		if(rctrack == SQL_SUCCESS_WITH_INFO)
			m_WarningSetBeforeAsync = true;
		else
		    m_WarningSetBeforeAsync = false;
	}
	else
#endif
	if (rctrack == SQL_SUCCESS_WITH_INFO  && rc != SQL_ERROR )
		rc = rctrack;


	return rc;
}

SQLRETURN CStmt::Execute()
{
	SQLRETURN	rc;
	BOOL		SkipProcess = FALSE;

	m_CurrentOdbcAPI = SQL_API_SQLEXECUTE;
#ifdef ASYNCIO
	if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
	{
		if (m_AsyncCanceled == TRUE)
		{
			rc = SQL_ERROR;
		}
		else
		{
			if(m_StmtExecState == STMT_EXECUTION_EXECUTING)
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::Execute: Statement %x still executing, connection = %x\n",this,m_ConnectHandle);
				#endif

				rc = SQL_STILL_EXECUTING;
			}
			else if( m_StmtExecState == STMT_EXECUTION_DONE)
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::Execute: Statement %x,Connection %x: Execution completed with returnCode %d\n",this,m_ConnectHandle,m_AsyncOperationReturnCode);
				#endif

				m_StmtExecState = STMT_EXECUTION_NONE;
				rc = m_AsyncOperationReturnCode;

				// this is to handle the case where a data truncation occurs when formating the buffers and a warning is set there
				if(m_WarningSetBeforeAsync && rc == SQL_SUCCESS)
					rc = SQL_SUCCESS_WITH_INFO;
			}
			else if( m_StmtExecState == STMT_EXECUTION_NONE)
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::Execute: Statement %x,Connection %x: No Statement currently being executed on this statement handle\n",this,m_ConnectHandle);
				#endif

				rc = SendExecute(FALSE);
			}
			else
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::Execute: Unexpected Async Execution State: %d\n",m_StmtExecState);
				#endif
				assert(0);
			}
		}
	}
	else
		rc = SendExecute(SkipProcess);
#else
	rc = SendExecute(SkipProcess);
#endif /* ASYNCIO */

	switch (rc)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
		if (m_StmtType == TYPE_SELECT || m_StmtType == TYPE_CALL)
			m_StmtState = STMT_EXECUTED;
		else
			m_StmtState = STMT_EXECUTED_NO_RESULT;
			m_isClosed = false;
		if (m_StmtQueryType == SQL_SELECT_UNIQUE && m_RowCount == 0) // Added for Caffeine
			m_StmtState = STMT_EXECUTED_NO_RESULT;
		break;
	case SQL_STILL_EXECUTING:
		m_StmtState = STMT_STILL_EXECUTING;
		break;
	case SQL_NEED_DATA:
		m_StmtState = STMT_PARAM_DATA_NOT_CALLED;
		break;
	case SQL_ERROR:
		if (m_AsyncCanceled == TRUE)
		{
			setDiagRec(DRIVER_ERROR, IDS_S1_008);
			m_AsyncCanceled = FALSE;
		}
		if (m_StmtType == TYPE_SELECT )
			m_StmtState = STMT_PREPARED;
		else
			m_StmtState = STMT_PREPARED_NO_RESULT;
		setRowCount(-1);
		break;
	case SQL_NO_DATA:
		if (m_StmtQueryType == SQL_SELECT_UNIQUE) // Added for Caffeine
		{
			m_StmtState = STMT_EXECUTED_NO_RESULT;
			rc = SQL_SUCCESS;
		}
		break;
	case SQL_INVALID_HANDLE:
	default:
		break;
	}
	return rc;
}

SQLRETURN CStmt::SendParamData(BOOL SkipProcess, SQLPOINTER *ValuePtrPtr)
{

	SQLRETURN		rc = SQL_SUCCESS;
	SQLSMALLINT		ParamNumber;
	SQLULEN 		RowNumber;
	SQLULEN 		AppArrayStatusSize;
	SQLINTEGER		ODBCAppVersion;
	unsigned long	retCode;
	UCHAR			errorMsg[MAX_TRANSLATE_ERROR_MSG_LEN];

	
	if (m_ConnectHandle->getConnectionStatus() == SQL_CD_TRUE)
	{
		clearError();
		setDiagRec(DRIVER_ERROR, IDS_08_S01, -20005);
		return SQL_ERROR;
	}
	if (! SkipProcess)
	{
		clearError();
		if (m_CurrentParam != -1)
		{
			switch (m_CurrentParamStatus)
			{
			case SQL_DATA_AT_EXEC:
				retCode = m_AppParamDesc->ParamData(m_CurrentRow, m_CurrentParam, ValuePtrPtr);
				if (retCode != SQL_SUCCESS)
				{
					setDiagRec(DRIVER_ERROR, retCode);
					rc = SQL_ERROR;
				}
				else
				{
					m_CurrentParamStatus = SQL_WAITING_FOR_DATA;
					rc = SQL_NEED_DATA;
				}
				return rc;
			case SQL_WAITING_FOR_DATA:
				// SQLParamData is called without calling SQLPutData
				setDiagRec(DRIVER_ERROR, IDS_HY_010);
				return SQL_ERROR;
			case SQL_RECEIVING_DATA:
				ODBCAppVersion = getODBCAppVersion();
				ParamNumber = m_CurrentParam;
				RowNumber = m_CurrentRow;
				// Update the current DataAtExecData Parameter value in m_InputValueList
				// and then proceed to Build the inputList
				retCode = m_ImpParamDesc->AssignDataAtExecValue(&m_DataAtExecData, m_AppParamDesc, 
								ODBCAppVersion, ParamNumber, RowNumber, &m_InputValueList,
								errorMsg,
								sizeof(errorMsg));
				if (retCode != SQL_SUCCESS)
				{
					if (errorMsg[0] != '\0')
						setDiagRec(DRIVER_ERROR, retCode, 0, (char *)errorMsg, NULL, RowNumber, ParamNumber);
					else
						setDiagRec(DRIVER_ERROR, retCode, 0, NULL, NULL, RowNumber, ParamNumber);
					switch (retCode)
					{
					case IDS_01_004:
					case IDS_01_S07:
					case IDS_188_DRVTODS_TRUNC:
						rc = SQL_SUCCESS_WITH_INFO;
						break;
					case IDS_22_001:						
					default:
						rc = SQL_ERROR;
						break;
					}
				}
				else
					rc = SQL_SUCCESS;
				if (rc == SQL_ERROR)
					return rc;
				rc = m_AppParamDesc->BuildValueList(this, m_NumParams, m_ImpParamDesc, ODBCAppVersion, ParamNumber, RowNumber,
						&m_InputValueList, m_ParamBuffer);
				switch (rc)
				{
				case SQL_SUCCESS:
				case SQL_SUCCESS_WITH_INFO:
					break;
				case SQL_NEED_DATA:
					m_CurrentParam = ParamNumber;
					m_CurrentRow = RowNumber;
					retCode = m_AppParamDesc->ParamData(m_CurrentRow, m_CurrentParam, ValuePtrPtr);
					if (retCode != SQL_SUCCESS)
					{
						setDiagRec(DRIVER_ERROR, retCode);
						rc = SQL_ERROR;
					}
					else
					{
						m_CurrentParamStatus = SQL_WAITING_FOR_DATA;
						rc = SQL_NEED_DATA;
					}
					return rc;
				default:
					setDiagRec(DRIVER_ERROR, rc);
					return rc;
				}
				break;
			default:
				setDiagRec(DRIVER_ERROR, IDS_HY_000);
				return SQL_ERROR;
			}
		}
		else
		{
			setDiagRec(DRIVER_ERROR, IDS_HY_010);
			return SQL_ERROR;
		}
		m_SrvrCallContext.initContext();
		m_SrvrCallContext.odbcAPI = SQL_API_SQLPARAMDATA;
		m_SrvrCallContext.sqlHandle = this;
		m_SrvrCallContext.SQLSvc_ObjRef = m_ConnectHandle->getSrvrObjRef();
		m_SrvrCallContext.ASSvc_ObjRef = NULL;
		m_SrvrCallContext.eventHandle = m_StmtEvent;
		m_SrvrCallContext.dialogueId = m_ConnectHandle->getDialogueId();
		m_SrvrCallContext.connectionTimeout = m_ConnectHandle->getConnectionTimeout();
		m_SrvrCallContext.u.executeParams.queryTimeout = m_QueryTimeout;
		m_SrvrCallContext.u.executeParams.stmtLabel = m_StmtLabel;
		m_SrvrCallContext.u.executeParams.cursorName = m_CursorName;
		m_SrvrCallContext.u.executeParams.sqlStmtType = m_StmtType;
		m_SrvrCallContext.u.executeParams.rowCount = RowNumber;
		m_SrvrCallContext.u.executeParams.inputValueList = &m_InputValueList;
		m_SrvrCallContext.u.executeParams.asyncEnable = m_AsyncEnable;
		m_ImpParamDesc->setRowsProcessed(RowNumber);
	
	}

#ifdef ASYNCIO
	if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
	{
		rc = StartAsyncOperation(m_ConnectHandle, this /* statement handle */);

		if(rc == SQL_SUCCESS)
			rc = SQL_STILL_EXECUTING;
		else
			rc = SQL_ERROR;
	}
	else
		rc = ThreadControlProc(&m_SrvrCallContext);
#else
	rc = ThreadControlProc(&m_SrvrCallContext);
#endif /* ASYNCIO */

	if (rc == SQL_ERROR)
	{
#ifdef __LP64__
		m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &AppArrayStatusSize, SQL_IS_POINTER, NULL);
#else
		m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &AppArrayStatusSize, SQL_IS_UINTEGER, NULL);
#endif
		m_ImpParamDesc->setDescArrayStatus(AppArrayStatusSize,SQL_PARAM_ERROR);
	}
	return rc;
}

SQLRETURN CStmt::ParamData(SQLPOINTER *ValuePtrPtr)
{
	
	BOOL		SkipProcess = FALSE;
	SQLRETURN	rc;
	
	m_CurrentOdbcAPI = SQL_API_SQLPARAMDATA;

#ifdef ASYNCIO
	if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
	{
		if (m_AsyncCanceled == TRUE)
		{
			rc = SQL_ERROR;
		}
		else
		{
			if(m_StmtExecState == STMT_EXECUTION_EXECUTING)
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::ParamData: Statement %x still executing, connection = %x\n",this,m_ConnectHandle);
				#endif

				rc = SQL_STILL_EXECUTING;
			}
			else if( m_StmtExecState == STMT_EXECUTION_DONE)
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::ParamData: Statement %x,Connection %x: Execution completed with returnCode %d\n",this,m_ConnectHandle,m_AsyncOperationReturnCode);
				#endif

				m_StmtExecState = STMT_EXECUTION_NONE;
				rc = m_AsyncOperationReturnCode;
			}
			else if( m_StmtExecState == STMT_EXECUTION_NONE)
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::ParamData: Statement %x,Connection %x: No Statement currently being executed on this statement handle\n",this,m_ConnectHandle);
				#endif

				rc = SendParamData(SkipProcess, ValuePtrPtr);
			}
			else
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::ParamData: Unexpected Async Execution State: %d\n",m_StmtExecState);
				#endif
				assert(0);
			}
		}
	}
	else
		rc = SendParamData(SkipProcess, ValuePtrPtr);
#else
	rc = SendParamData(SkipProcess, ValuePtrPtr);
#endif /* ASYNCIO */

	switch (rc)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
		if (m_StmtType == TYPE_SELECT)
			m_StmtState = STMT_EXECUTED;
		else
			m_StmtState = STMT_EXECUTED_NO_RESULT;
		break;
	case SQL_STILL_EXECUTING:
		m_StmtState = STMT_STILL_EXECUTING;
		break;
	case SQL_NEED_DATA:
		m_StmtState = STMT_PUT_DATA_NOT_CALLED;
		break;
	case SQL_ERROR:
		if (m_AsyncCanceled == TRUE)
		{
			setDiagRec(DRIVER_ERROR, IDS_S1_008);
			m_AsyncCanceled = FALSE;
		}
		revertStmtState();
		break;
	case SQL_NO_DATA:
		if (m_StmtQueryType == SQL_SELECT_UNIQUE) //  Added for Caffeine
			m_StmtState = STMT_EXECUTED_NO_RESULT;
		break;
	case SQL_INVALID_HANDLE:
	default:
		break;
	}
	return rc;
}

SQLRETURN CStmt::PutData(SQLPOINTER DataPtr, 
		   SQLLEN StrLen_or_Ind)
{
	SQLRETURN		rc = SQL_SUCCESS;
	SQLSMALLINT		DataType;
	SQLSMALLINT		ODBCDataType;
	SQLINTEGER		OctetLength;
	SQLPOINTER		AllocDataPtr;
	unsigned long	retCode;
	SQLLEN 		   *StrLenPtr;
	SQLINTEGER		StrLen;
	SQLSMALLINT		ParameterType;
	// For ALM CR 5228. Add these 2 variables to get the parameter charset from APD.
	// The charset will be used for determine the mapped paramter type via getCDefault();
	SQLINTEGER		ParamCharSet;
	CDescRec		*descRecPtr = NULL;

	m_CurrentOdbcAPI = SQL_API_SQLPUTDATA;
	clearError();
	if (m_CurrentParam == -1)
	{
		setDiagRec(DRIVER_ERROR, IDS_HY_010);
		rc = SQL_ERROR;
		goto updateStmtState;
	}
	switch (m_CurrentParamStatus)
	{
	case SQL_WAITING_FOR_DATA:	// First time SQLPutData is called for this parameter
		if ((retCode = m_AppParamDesc->GetParamType(m_CurrentRow, m_CurrentParam, DataType, 
				StrLenPtr)) != SQL_SUCCESS)
		{
			setDiagRec(DRIVER_ERROR, retCode);
			rc = SQL_ERROR;
			goto updateStmtState;
		}
		if (StrLenPtr == NULL)
		{
			setDiagRec(DRIVER_ERROR, IDS_HY_000);
			rc = SQL_ERROR;
			goto updateStmtState;
		}
		// For ALM CR8225. Added argument ParamCharSet to get the charset of the parameter to determine the mapped Data Type.
		if ((retCode = m_ImpParamDesc->GetParamSQLInfo(m_CurrentRow, m_CurrentParam, ParameterType,
				ODBCDataType, ParamCharSet)) != SQL_SUCCESS)
		{
			setDiagRec(DRIVER_ERROR, retCode);
			rc = SQL_ERROR;
			goto updateStmtState;
		}
		if (DataType == SQL_C_DEFAULT)
		{
			if ((retCode = getCDefault(ParameterType, getODBCAppVersion(), ParamCharSet, DataType)) != SQL_SUCCESS)
			{
				setDiagRec(DRIVER_ERROR, retCode);
				rc = SQL_ERROR;
				goto updateStmtState;
			}
		}
		m_DataAtExecData.dataType = DataType;
		m_DataAtExecData.dataCharset = ParamCharSet;
		StrLen = StrLen_or_Ind;
		switch (StrLen_or_Ind)
		{
		case SQL_DEFAULT_PARAM:
			setDiagRec(DRIVER_ERROR, IDS_07_S01);
			rc = SQL_ERROR;
			goto updateStmtState;
			break;
		case SQL_NULL_DATA:
			m_DataAtExecData.dataInd = -1;
			if (m_DataAtExecData.dataValue._buffer != NULL)
				delete m_DataAtExecData.dataValue._buffer;
			m_DataAtExecData.dataValue._buffer = NULL;
			m_DataAtExecData.dataValue._length = 0;
			m_DataAtExecDataBufferSize = 0;
			break;
		case SQL_NTS:
			//StrLen = strlen((const char *)DataPtr);		// Note No Break
			//ALM CR ID 5228: Use icu converter function 
			//ICUConverter::FindStrLength() to determine
			//the length of the input data length.
			StrLen = gDrvrGlobal.ICUConv.FindStrLength((const char *)DataPtr, StrLen_or_Ind);
		default:
			if (StrLen < 0)
			{
				setDiagRec(DRIVER_ERROR, IDS_HY_090);
				rc = SQL_ERROR;
				goto updateStmtState;
			}
			switch (DataType)
			{
			case SQL_C_CHAR:
			case SQL_C_BINARY:
			case SQL_C_WCHAR:
				// Get the Length Specified at exec Time
				if (*StrLenPtr == SQL_DATA_AT_EXEC)
					OctetLength = 4096+2;
				else
					OctetLength = (*StrLenPtr * -1) + SQL_LEN_DATA_AT_EXEC_OFFSET+2;
				if (StrLen > OctetLength)
				{
					setDiagRec(DRIVER_ERROR, IDS_22_001);
					rc = SQL_ERROR;
					goto updateStmtState;
				}
				break;
			default:
				if (retCode = getOctetLength(ODBCDataType, 
								getODBCAppVersion(), 
								DataPtr,
								StrLen_or_Ind,
								DataType,
								OctetLength) != SQL_SUCCESS)
				{
					setDiagRec(DRIVER_ERROR, retCode);
					rc = SQL_ERROR;
					goto updateStmtState;
				}
				StrLen = OctetLength;
				break;
			}
			AllocDataPtr = new BYTE[OctetLength];
			if (AllocDataPtr == NULL)
			{
				setDiagRec(DRIVER_ERROR, IDS_HY_001);
				rc = SQL_ERROR;
				goto updateStmtState;
			}
			m_DataAtExecDataBufferSize = OctetLength;
			m_DataAtExecData.dataInd = 0;
			if (m_DataAtExecData.dataValue._buffer != NULL)
				delete m_DataAtExecData.dataValue._buffer;
			m_DataAtExecData.dataValue._buffer = (unsigned char *)AllocDataPtr;
			memcpy(AllocDataPtr, DataPtr, StrLen);
			m_DataAtExecData.dataValue._length = StrLen;
			break;
		}
		m_CurrentParamStatus = SQL_RECEIVING_DATA;
		break;
	case SQL_RECEIVING_DATA:
		switch (m_DataAtExecData.dataType)
		{
		case SQL_C_CHAR:
		case SQL_C_BINARY:
		case SQL_C_WCHAR:
			StrLen = StrLen_or_Ind;
			switch (StrLen_or_Ind)
			{
			case SQL_NULL_DATA:		// Null Data being concatenated, 
			case SQL_DEFAULT_PARAM:
				setDiagRec(DRIVER_ERROR, IDS_HY_020);
				rc = SQL_ERROR;
				goto updateStmtState;
			case SQL_NTS:
				//StrLen = strlen((const char *)DataPtr);
				//ALM CR ID 5228: Use icu converter function 
				//ICUConverter::FindStrLength() to determine
				//the length of the input data length.
				StrLen = gDrvrGlobal.ICUConv.FindStrLength((const char *)DataPtr, StrLen_or_Ind);
			default:
				if (StrLen < 0)
				{
					setDiagRec(DRIVER_ERROR, IDS_HY_090);
					rc = SQL_ERROR;
					goto updateStmtState;
				}
				if (m_DataAtExecData.dataValue._length + StrLen > m_DataAtExecDataBufferSize)
				{
					setDiagRec(DRIVER_ERROR, IDS_22_001);
					rc = SQL_ERROR;
					goto updateStmtState;
				}
				memcpy(m_DataAtExecData.dataValue._buffer + m_DataAtExecData.dataValue._length,
					DataPtr, StrLen);
				m_DataAtExecData.dataValue._length +=  StrLen;
				break;
			}
			break;
		default: // Non-character or non-Binary data being sent in pieces
			setDiagRec(DRIVER_ERROR, IDS_HY_019);
			rc = SQL_ERROR;
			break;
		}
		break;
	default:
		setDiagRec(DRIVER_ERROR, IDS_HY_010);
		rc = SQL_ERROR;
	}
updateStmtState:
	switch (rc)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
		m_StmtState = STMT_PUT_DATA_CALLED;
		break;
	case SQL_ERROR:
		revertStmtState();
		break;
	case SQL_STILL_EXECUTING:
	case SQL_NEED_DATA:
	case SQL_NO_DATA:
	case SQL_INVALID_HANDLE:
	default:
		break;
	}
	return rc;
}

void CStmt::setFetchOutput(long rowsFetched, const SQLValueList_def *outputValueList, CEE_handle_def srvrProxy)
{
        m_RowsFetched = rowsFetched;
        m_ResultsetRowsFetched += rowsFetched;
//      m_OutputValueList = (SQLValueList_def *)outputValueList;
        m_CurrentRowFetched = 0;
        m_CurrentRowInRowset = 0;
        m_RowsetSize = 0;
        m_ImpRowDesc->setRowsProcessed(rowsFetched);
}

BOOL CStmt::setFetchOutputPerf(long rowsFetched, SQL_DataValue_def*& outputDataValue)
{
	long columnOffsets = 0;
	long totalLength;

	m_RowsFetched = rowsFetched;
	m_ResultsetRowsFetched += rowsFetched;
	m_CurrentRowFetched = 0;
	m_CurrentRowInRowset = 0;
	m_RowsetSize = 0;
	m_ImpRowDesc->setRowsProcessed(rowsFetched);

	if (m_FetchDataValue.rowAddress != NULL )
	{
		delete[] m_FetchDataValue.rowAddress;
		m_FetchDataValue.rowAddress = NULL;
	}

	if (m_outputDataValue._length != 0 &&
		m_outputDataValue._buffer != NULL)
	{
		delete [] m_outputDataValue._buffer;
		m_outputDataValue._buffer = NULL;
		m_outputDataValue._length = 0;
	}
		
	totalLength = outputDataValue->_length;
	totalLength += columnOffsets;

	if(totalLength > 0)
	{
		m_outputDataValue._buffer = (unsigned char*)new char[totalLength+1];
		if (m_outputDataValue._buffer == NULL)
			return FALSE;
		memset(m_outputDataValue._buffer, 0, totalLength+1);
	}
	m_outputDataValue._length = outputDataValue->_length;

	if(outputDataValue->_length > 0)
	{
		memcpy(m_outputDataValue._buffer,outputDataValue->_buffer,outputDataValue->_length);
		m_outputDataValue._buffer[totalLength] = '\0';
	}

	if (m_StmtQueryType == SQL_SELECT_UNIQUE)
	{
		if (outputDataValue->_buffer != NULL)
		{
			delete outputDataValue->_buffer;
			outputDataValue->_length = 0;
			outputDataValue->_buffer = NULL;
		}
	}	
	outputDataValue = &m_outputDataValue;

	m_FetchDataValue.numberOfElements = m_outputDataValue._length;
	m_FetchDataValue.numberOfRows = rowsFetched;
	if(rowsFetched > 0)
	{
		m_FetchDataValue.rowAddress = new unsigned long[rowsFetched];
		if (m_FetchDataValue.rowAddress != NULL)
			return TRUE;
		else
		{
			delete m_outputDataValue._buffer;
			m_outputDataValue._buffer =NULL;
			m_outputDataValue._length = 0;
			return FALSE;
		}
	}
	return TRUE;
}
//This function has been overloaded to use in RWRS binding scenario
BOOL CStmt::setFetchOutputPerf(SQL_DataValue_def*& outputDataValue, long rowsFetched)
{
	IDL_long *columnOffsets = NULL;
	IDL_long memOffSet = 0;
	IDL_long VarOffSet = 0;
	IDL_long IndOffSet = 0;
	BYTE  *memPtr = NULL;
	BYTE  *IndicatorPtr = NULL;
	IDL_long count = 0;
	int ColumnCount = 0;
	int index=0;
	SQLINTEGER		SQLCharset=0;
	SQLSMALLINT		SQLDataType=0; 
	SQLSMALLINT		SQLDatetimeCode=0;
	SQLINTEGER		SQLMaxLength=0;
	SQLINTEGER		SQLOctetLength=0; 
	SQLSMALLINT		SQLPrecision=0;
	SQLSMALLINT		SQLUnsigned=0;
	SQLSMALLINT	SQLNullable=0;
 	
	m_RowsFetched = rowsFetched;
	m_ResultsetRowsFetched += rowsFetched;
	m_CurrentRowFetched = 0;
	m_CurrentRowInRowset = 0;
	m_RowsetSize = 0;
	
	m_ImpRowDesc->setRowsProcessed(rowsFetched);

	if (m_FetchDataValue.rowAddress != NULL )
	{
		delete[] m_FetchDataValue.rowAddress;
		m_FetchDataValue.rowAddress = NULL;
	}

	if (m_outputDataValue._length != 0 &&
		m_outputDataValue._buffer != NULL)
	{
		delete[] m_outputDataValue._buffer;
		m_outputDataValue._buffer = NULL;
		m_outputDataValue._length = 0;
	}
	
	count = getImpDescCount();
	
	if (m_ColumnIndexes != NULL)
	{
		delete[] m_ColumnIndexes;
		m_ColumnIndexes=NULL;
	}
	if (m_SwapInfo != NULL)
	{
		for (int r = 0; r < m_SwapInfo_NumRows; r++) 
		{
			if (m_SwapInfo[r] != NULL)
				delete[] (m_SwapInfo[r]);
		}
		delete[] m_SwapInfo;
		m_SwapInfo = NULL;
		m_SwapInfo_NumRows = 0;
	}

	m_ColumnIndexes = new long[count*2]; // Two indexes for each column
	
	memset((void *)m_ColumnIndexes, 0, sizeof(long)*count*2);

	memPtr = (BYTE*)outputDataValue->_buffer;

	for (ColumnCount = 0; ColumnCount < count ; ColumnCount++)
	{
		getImpRowData(ColumnCount+1, SQLDataType, SQLMaxLength, SQLNullable);
		
		index = 2*ColumnCount;
		
		if (SQLNullable)
		{
			memOffSet = ((memOffSet + 2 - 1) >> 1) << 1;
			IndOffSet = memOffSet;
			IndicatorPtr = memPtr + IndOffSet;
			memOffSet += 2 ;
			m_ColumnIndexes[index] = IndOffSet;

		}
		else
			m_ColumnIndexes[index] = -1;
		switch (SQLDataType)
		{
			case SQLTYPECODE_CHAR:
			case SQLTYPECODE_VARCHAR:
				VarOffSet = memOffSet;					
				memOffSet += SQLMaxLength;
				break;
			case SQLTYPECODE_VARCHAR_WITH_LENGTH:
				if(SQLMaxLength > SHRT_MAX)
				{
					memOffSet = ((memOffSet + 4 - 1) >> 2) << 2; 
					VarOffSet = memOffSet;
					memOffSet += SQLMaxLength + 4;
				}
				else
				{
					memOffSet = ((memOffSet + 2 - 1) >> 1) << 1; 
					VarOffSet = memOffSet;
					memOffSet += SQLMaxLength + 2;
				}
				break;
			case SQLTYPECODE_INTERVAL: 
				VarOffSet = memOffSet;					
				memOffSet += SQLMaxLength;
				break;
			case SQLTYPECODE_VARCHAR_LONG:
				memOffSet = ((memOffSet + 2 - 1) >> 1) << 1; 
				VarOffSet = memOffSet;					
				memOffSet += SQLMaxLength + 2;
				break;
			case SQLTYPECODE_SMALLINT:
				memOffSet = ((memOffSet + 2 - 1) >> 1) << 1; 
				VarOffSet = memOffSet;					
				memOffSet += SQLMaxLength;
				break;
			case SQLTYPECODE_SMALLINT_UNSIGNED:
				memOffSet = ((memOffSet + 2 - 1) >> 1) << 1; 
				VarOffSet = memOffSet;					
				memOffSet += SQLMaxLength ;
				break;
			case SQLTYPECODE_INTEGER:
				memOffSet = ((memOffSet + 4 - 1) >> 2) << 2; 
				VarOffSet = memOffSet;					
				memOffSet += SQLMaxLength ;
				break;
			case SQLTYPECODE_INTEGER_UNSIGNED:
				memOffSet = ((memOffSet + 4 - 1) >> 2) << 2; 
				VarOffSet = memOffSet;					
				memOffSet += SQLMaxLength ;
				break;
			case SQLTYPECODE_LARGEINT:
			case SQLTYPECODE_LARGEINT_UNSIGNED:
				memOffSet = ((memOffSet + 8 - 1) >> 3) << 3; 
				VarOffSet = memOffSet;					
				memOffSet += SQLMaxLength ;
				break;
			case SQLTYPECODE_IEEE_REAL:
				//memOffSet = ((memOffSet + 4 - 1) >> 2) << 2;  
				memOffSet = ((memOffSet + 8 - 1) >> 3) << 3;
				VarOffSet = memOffSet;					
				memOffSet += SQLMaxLength ;
				break;
			case SQLTYPECODE_IEEE_FLOAT:
			case SQLTYPECODE_IEEE_DOUBLE:
				memOffSet = ((memOffSet + 8 - 1) >> 3) << 3; 
				VarOffSet = memOffSet;					
				memOffSet += SQLMaxLength ;
				break;
			case SQLTYPECODE_DATETIME:
				VarOffSet = memOffSet;					
				memOffSet += SQLMaxLength ;
				break; 
			case SQLTYPECODE_DECIMAL_UNSIGNED:
				VarOffSet = memOffSet;
				memOffSet += SQLMaxLength;
				break;
			case SQLTYPECODE_DECIMAL:
				VarOffSet = memOffSet;
				memOffSet += SQLMaxLength;
				break; 
			case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED: // Tandem extension
				VarOffSet = memOffSet;
				memOffSet += SQLMaxLength;
				break;
			case SQLTYPECODE_DECIMAL_LARGE: // Tandem extension
				VarOffSet = memOffSet;
				memOffSet += SQLMaxLength;
				break;
                        case SQLTYPECODE_NUMERIC:              //2
			case SQLTYPECODE_NUMERIC_UNSIGNED:    //-201
				memOffSet = ((memOffSet + 8 - 1) >> 3) << 3; 
				VarOffSet = memOffSet;					
				memOffSet += SQLMaxLength ;
                                break;
			default:
//						memOffSet = ((memOffSet + 8 - 1) >> 3) << 3; 
				VarOffSet = memOffSet;
				memOffSet += SQLMaxLength ;
				break;
			}

		m_ColumnIndexes[++index] = VarOffSet;
	}
	
  	if ((rowsFetched > 0) && (memOffSet != outputDataValue->_length / rowsFetched))
	{
		setDiagRec(DRIVER_ERROR, IDS_21_002, 0, "Inconsistent memory offsets, data length, and rows fetched");	
		return FALSE;
	}

	if(outputDataValue->_length > 0)
	{
		m_outputDataValue._buffer = (unsigned char*)new char[outputDataValue->_length];
		if (m_outputDataValue._buffer == NULL)
		{
			setDiagRec(DRIVER_ERROR, IDS_S1_001, 0, "Not enough memory to hold data");
			return FALSE;
		}
	}

	m_outputDataValue._length = outputDataValue->_length;
	if(outputDataValue->_length > 0)
		memcpy(m_outputDataValue._buffer,outputDataValue->_buffer,outputDataValue->_length);
	outputDataValue = &m_outputDataValue;

	m_FetchDataValue.numberOfElements = outputDataValue->_length;
	m_FetchDataValue.numberOfRows = rowsFetched;

	if (rowsFetched > 0)
	{
		m_FetchDataValue.rowAddress = new unsigned long[rowsFetched];
		m_SwapInfo = new char*[rowsFetched];
		m_SwapInfo_NumRows = rowsFetched;
		for (int i = 0; i < rowsFetched; i++)
			m_SwapInfo[i] = new char[count];
		if (m_FetchDataValue.rowAddress == NULL || m_SwapInfo == NULL)
		{
			setDiagRec(DRIVER_ERROR, IDS_S1_001, 0, "Not enough memory");
			delete m_outputDataValue._buffer;
			m_outputDataValue._buffer =NULL;
			m_outputDataValue._length = 0;
			return FALSE;
		}
		//memset(m_SwapInfo,'N', rowsFetched);
		for (int r = 0; r < rowsFetched; r++)
		{
			for (int c = 0; c < count; c++)
				m_SwapInfo[r][c] = 'N';
		}
	}

	return TRUE;
} // Rowwise rowset version of setFetchOutputPerf()

SQLRETURN CStmt::SendFetch(BOOL SkipProcess)
{

	SQLRETURN		rc = SQL_SUCCESS;
	BOOL			DispatchFetch = FALSE;
	SQLULEN 		MaxRowCnt;
	SQLINTEGER		RecLength;
		
	if (m_ConnectHandle->getConnectionStatus() == SQL_CD_TRUE)
	{
		clearError();
		setDiagRec(DRIVER_ERROR, IDS_08_S01, -20005);
		return SQL_ERROR;
	}
	//  Added for Caffeine
	if (m_StmtQueryType == SQL_SELECT_UNIQUE)
	{
		switch (m_StmtState)
		{
		case STMT_EXECUTED:
			return SQL_SUCCESS;
		case STMT_EXECUTED_NO_RESULT:
		case STMT_FETCHED:
			return SQL_NO_DATA;
		default:
			break;
		}
	}

	if (! SkipProcess)
	{
		clearError();
		if (m_StmtState == STMT_EXECUTED_NO_RESULT)
		{
			setDiagRec(DRIVER_ERROR, IDS_24_000);
			return SQL_ERROR;
		}
		if (m_MaxRows != 0)
		{
			if (m_RowNumber >= m_MaxRows)
				return SQL_NO_DATA;
		}
		if (m_RowsFetched == -1 || m_RowsFetched == m_CurrentRowFetched)
		{
			// Calculate the Number of Rows to be fetched
#ifdef __LP64__
			m_AppRowDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &MaxRowCnt, SQL_IS_POINTER, NULL);
#else
			m_AppRowDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &MaxRowCnt, SQL_IS_UINTEGER, NULL);
#endif
			// Adjust MaxRowCnt to implement MaxRowValues
			if (m_MaxRows != 0)
			{
				if (m_RowNumber + MaxRowCnt > m_MaxRows)
				{
					MaxRowCnt = m_MaxRows - m_RowNumber;
					if (MaxRowCnt == 0)
						return SQL_NO_DATA;
				}
			}
			// If MaxRowCnt is more than 1, the application is using Block Cursor
			// If MaxRowCnt is 1, the driver tries to do BulkFetch based on SQL_ATTR_CONCURRENCY
			if (MaxRowCnt == 1)
			{
				if (m_Concurrency == SQL_CONCUR_READ_ONLY)
				{
					RecLength = m_ImpRowDesc->getRecLength();
					if (RecLength > 0)		// m_FetchBufferSize = 0 is equivalent to BulkFetch is disabled.
						if (m_FetchBufferSize > RecLength)		
							MaxRowCnt = m_FetchBufferSize / RecLength;
					
					if (m_MaxRows != 0)
					{
						if (m_RowNumber + MaxRowCnt > m_MaxRows)
						{
							MaxRowCnt = m_MaxRows - m_RowNumber;
							if (MaxRowCnt == 0)
								return SQL_NO_DATA;
						}
					}
				}
			}
			
			// Added this code for future enabling. We can enable this for ABD, Do this does
			// is if rows fetched by SQL is less the MAXROWCNT set then we can assume it as
			// END OF DATA. Ando no need to send again FETCH call to Server. So that we can
			// avoid one message transmission. On the Server side we internally call Fetch to
			// make sure we reach END OF DATA.
			// if (m_RowsFetched < MaxRowCnt)
			//	return SQL_NO_DATA;

			m_SrvrCallContext.initContext();
			m_SrvrCallContext.odbcAPI = SQL_API_SQLFETCH;
			m_SrvrCallContext.sqlHandle = this;
			m_SrvrCallContext.SQLSvc_ObjRef = m_ConnectHandle->getSrvrObjRef();
			m_SrvrCallContext.ASSvc_ObjRef = NULL;
			m_SrvrCallContext.eventHandle = m_StmtEvent;
			m_SrvrCallContext.dialogueId = m_ConnectHandle->getDialogueId();
			m_SrvrCallContext.connectionTimeout = m_ConnectHandle->getConnectionTimeout();
			m_SrvrCallContext.u.fetchParams.queryTimeout = m_QueryTimeout;
			if(m_spjNumResultSets > 0)
				m_SrvrCallContext.u.fetchParams.stmtLabel = m_spjResultSets[m_spjCurrentResultSetIndex].spjStmtLabelName;
			else
				m_SrvrCallContext.u.fetchParams.stmtLabel = m_StmtLabel;
			m_SrvrCallContext.u.fetchParams.maxRowCount = MaxRowCnt;
			m_SrvrCallContext.u.fetchParams.maxRowLen = m_MaxLength;
			m_SrvrCallContext.u.fetchParams.asyncEnable = m_AsyncEnable;

			// Reset the Fetch related member elements
			m_RowsFetched = -1;
			DispatchFetch = TRUE;
			m_ImpRowDesc->setRowsProcessed(0);
#ifdef __LP64__
			m_AppRowDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &MaxRowCnt, SQL_IS_POINTER, NULL);
#else
			m_AppRowDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &MaxRowCnt, SQL_IS_UINTEGER, NULL);
#endif
			m_ImpRowDesc->setDescArrayStatus(MaxRowCnt); 
		}
		else
			DispatchFetch = FALSE;
	}
	if (DispatchFetch)
	{
		m_ConnectHandle->setSrvrCallContext(&m_SrvrCallContext);
	#ifdef ASYNCIO
		if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
		{
			rc = StartAsyncOperation(m_ConnectHandle, this /* statement handle */);

			if(rc == SQL_SUCCESS)
				rc = SQL_STILL_EXECUTING;
			else
				rc = SQL_ERROR;
		}
		else
			rc = ThreadControlProc(&m_SrvrCallContext);
	#else
		rc = ThreadControlProc(&m_SrvrCallContext);
	#endif /* ASYNCIO */
	}
	return rc;
}

SQLRETURN CStmt::Fetch()
{
	SQLRETURN	rc, temprc;
	SQLRETURN   rcCopyData = SQL_SUCCESS;
	BOOL		SkipProcess = FALSE;

	// Following variable and code is added because if application is 2.0 and is using
	// SQL_ROWSET_SIZE option with SQLFetch instead of SQLExtendedFetch. So we are suppose to
	// return one row at a time instead of 10 rows we fetch from server and move them
	// into application buffer. Ex: Suppose we have 20 catalogs (like CAT1, CAT2.. Till
	// CAT20) configured and application calls SQLTables for catalogs then we move
	// 2 sets of 10 row chucks when application call SQLFetch we should return ONE ROW
	// at a time NOT in set of 10 when SQLGetData or SQLBindCol is called. So set the 
	// m_DescArraySize variable to 1 and set it back to original before exiting the function.
	SQLULEN	tempMaxRowCnt;
	SQLULEN	setMaxRowCntToOne = 1;
	if (getODBCAppVersion() == SQL_OV_ODBC2)
	{
#ifdef __LP64__
		m_AppRowDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &tempMaxRowCnt, SQL_IS_POINTER, NULL);
#else
		m_AppRowDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &tempMaxRowCnt, SQL_IS_UINTEGER, NULL);
#endif
		if (tempMaxRowCnt > 1)
		{
			temprc = m_AppRowDesc->SetDescField(0, SQL_DESC_ARRAY_SIZE, (SQLPOINTER)setMaxRowCntToOne, 0);
			if (temprc == SQL_ERROR)
				return temprc;
		}
	}

	m_CurrentOdbcAPI = SQL_API_SQLFETCH;
	m_CurrentFetchType = SQL_API_SQLFETCH;

#ifdef ASYNCIO
	if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
	{
		if (m_AsyncCanceled == TRUE)
		{
			rc = SQL_ERROR;
		}
		else
		{
			if(m_StmtExecState == STMT_EXECUTION_EXECUTING)
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::Fetch: Statement %x still executing, connection = %x\n",this,m_ConnectHandle);
				#endif

				rc = SQL_STILL_EXECUTING;
			}
			else if( m_StmtExecState == STMT_EXECUTION_DONE)
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::Fetch: Statement %x,Connection %x: Execution completed with returnCode %d\n",this,m_ConnectHandle,m_AsyncOperationReturnCode);
				#endif

				m_StmtExecState = STMT_EXECUTION_NONE;
				rc = m_AsyncOperationReturnCode;
			}
			else if( m_StmtExecState == STMT_EXECUTION_NONE)
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::Fetch: Statement %x,Connection %x: No Statement currently being executed on this statement handle\n",this,m_ConnectHandle);
				#endif

				rc = SendFetch(FALSE);
			}
			else
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::Fetch: Unexpected Async Execution State: %d\n",m_StmtExecState);
				#endif
				assert(0);
			}
		}
	}
	else
		rc = SendFetch(SkipProcess);
#else
	rc = SendFetch(SkipProcess);
#endif /* ASYNCIO */

	switch (rc)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
		rcCopyData = m_AppRowDesc->CopyData(this, m_NumResultCols, m_ImpRowDesc,
			m_RowsFetched, m_CurrentRowFetched, m_RowNumber, m_RowsetSize);
		m_ImpRowDesc->setRowsProcessed(m_RowsetSize);
		m_StmtState = STMT_FETCHED;
		m_CurrentRowInRowset = 1;
		break;
	case SQL_STILL_EXECUTING:
		m_StmtState = STMT_STILL_EXECUTING;
		break;
	case SQL_NO_DATA:
		m_StmtState = STMT_FETCHED_TO_END;
		break;
	case SQL_ERROR:
		if (m_AsyncCanceled == TRUE)
		{
			setDiagRec(DRIVER_ERROR, IDS_S1_008);
			m_AsyncCanceled = FALSE;
		}
		break;
	case SQL_NEED_DATA:
	case SQL_INVALID_HANDLE:
	default:
		break;
	}
	// Following code is will set it back to original.
	if (getODBCAppVersion() == SQL_OV_ODBC2)
	{
		if (tempMaxRowCnt > 1)
		{
			temprc = m_AppRowDesc->SetDescField(0, SQL_DESC_ARRAY_SIZE, (SQLPOINTER) tempMaxRowCnt, 0);
			if (temprc == SQL_ERROR)
				return temprc;
		}
	}
	if(rcCopyData == SQL_ERROR || rc == SQL_ERROR)
		return SQL_ERROR;
	else if(rcCopyData == SQL_SUCCESS_WITH_INFO || rc == SQL_SUCCESS_WITH_INFO)
		return SQL_SUCCESS_WITH_INFO;
	else
		return rc;
}

SQLRETURN CStmt::SendExtendedFetch(BOOL SkipProcess,
			SQLUSMALLINT FetchOrientation,
			SQLLEN FetchOffset,
			SQLULEN* RowCountPtr,
			SQLUSMALLINT* RowStatusArray)
{

	SQLRETURN		rc = SQL_SUCCESS;
	BOOL			DispatchFetch = FALSE;
	SQLULEN 		MaxRowCnt;
	SQLINTEGER		RecLength;
		
	if (m_ConnectHandle->getConnectionStatus() == SQL_CD_TRUE)
	{
		clearError();
		setDiagRec(DRIVER_ERROR, IDS_08_S01, -20005);
		return SQL_ERROR;
	}

	//  Added for Caffeine
	if (m_StmtQueryType == SQL_SELECT_UNIQUE)
	{
		switch (m_StmtState)
		{
		case STMT_EXECUTED:
			return SQL_SUCCESS;
		case STMT_EXECUTED_NO_RESULT:
		case STMT_FETCHED:
			return SQL_NO_DATA;
		default:
			break;
		}
	}

	if (! SkipProcess)
	{
		clearError();
		if (m_StmtState == STMT_EXECUTED_NO_RESULT)
		{
			setDiagRec(DRIVER_ERROR, IDS_24_000);
			return SQL_ERROR;
		}
		if (m_MaxRows != 0)
		{
			if (m_RowNumber >= m_MaxRows)
				return SQL_NO_DATA;
		}
		if (m_RowsFetched == -1 || m_RowsFetched == m_CurrentRowFetched)
		{
			// Calculate the Number of Rows to be fetched
#ifdef __LP64__
			m_AppRowDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &MaxRowCnt, SQL_IS_POINTER, NULL);
#else
			m_AppRowDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &MaxRowCnt, SQL_IS_UINTEGER, NULL);
#endif
			// Adjust MaxRowCnt to implement MaxRowValues
			if (m_MaxRows != 0)
			{
				if (m_RowNumber + MaxRowCnt > m_MaxRows)
				{
					MaxRowCnt = m_MaxRows - m_RowNumber;
					if (MaxRowCnt == 0)
						return SQL_NO_DATA;
				}
			}
			// If MaxRowCnt is more than 1, the application is using Block Cursor
			// If MaxRowCnt is 1, the driver tries to do BulkFetch based on SQL_ATTR_CONCURRENCY
			if (MaxRowCnt == 1)
			{
				if (m_Concurrency == SQL_CONCUR_READ_ONLY)
				{
					RecLength = m_ImpRowDesc->getRecLength();
					if (RecLength > 0)		// FetchBufferSize = 0 is equivalent to BulkFetch is disabled.
						if (m_FetchBufferSize > RecLength)		
							MaxRowCnt = m_FetchBufferSize / RecLength;
					
					if (m_MaxRows != 0)
					{
						if (m_RowNumber + MaxRowCnt > m_MaxRows)
						{
							MaxRowCnt = m_MaxRows - m_RowNumber;
							if (MaxRowCnt == 0)
								return SQL_NO_DATA;
						}
					}
				}
			}
			
			m_SrvrCallContext.initContext();
			m_SrvrCallContext.odbcAPI = SQL_API_SQLEXTENDEDFETCH;
			m_SrvrCallContext.sqlHandle = this;
			m_SrvrCallContext.SQLSvc_ObjRef = m_ConnectHandle->getSrvrObjRef();
			m_SrvrCallContext.ASSvc_ObjRef = NULL;
			m_SrvrCallContext.eventHandle = m_StmtEvent;
			m_SrvrCallContext.dialogueId = m_ConnectHandle->getDialogueId();
			m_SrvrCallContext.connectionTimeout = m_ConnectHandle->getConnectionTimeout();
			m_SrvrCallContext.u.fetchParams.queryTimeout = m_QueryTimeout;
			m_SrvrCallContext.u.fetchParams.stmtLabel = m_StmtLabel;
			m_SrvrCallContext.u.fetchParams.maxRowCount = MaxRowCnt;
			m_SrvrCallContext.u.fetchParams.maxRowLen = m_MaxLength;
			m_SrvrCallContext.u.fetchParams.asyncEnable = m_AsyncEnable;

			// Reset the Fetch related member elements
			m_RowsFetched = -1;
			DispatchFetch = TRUE;
			m_ImpRowDesc->setRowsProcessed(0);
			m_ImpRowDesc->setDescArrayStatus(MaxRowCnt); 
		}
		else
			DispatchFetch = FALSE;
	}
	if (DispatchFetch)
	{
		m_ConnectHandle->setSrvrCallContext(&m_SrvrCallContext);
	#ifdef ASYNCIO
		if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
		{
			rc = StartAsyncOperation(m_ConnectHandle, this /* statement handle */);

			if(rc == SQL_SUCCESS)
				rc = SQL_STILL_EXECUTING;
			else
				rc = SQL_ERROR;
		}
		else
			rc = ThreadControlProc(&m_SrvrCallContext);
	#else
		rc = ThreadControlProc(&m_SrvrCallContext);
	#endif /* ASYNCIO */
	}
	return rc;
}

SQLRETURN CStmt::ExtendedFetch(SQLUSMALLINT FetchOrientation,
			SQLLEN FetchOffset,
			SQLULEN* RowCountPtr,
			SQLUSMALLINT* RowStatusArray)
{
	SQLRETURN	rc;
	SQLRETURN   rcCopyData = SQL_SUCCESS;
	BOOL		SkipProcess = FALSE;

	m_CurrentOdbcAPI = SQL_API_SQLEXTENDEDFETCH;
	m_CurrentFetchType = SQL_API_SQLEXTENDEDFETCH;
	m_RowStatusArray = RowStatusArray; 

#ifdef ASYNCIO
	if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
	{
		if (m_AsyncCanceled == TRUE)
		{
			rc = SQL_ERROR;
		}
		else
		{
			if(m_StmtExecState == STMT_EXECUTION_EXECUTING)
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::ExtendedFetch: Statement %x still executing, connection = %x\n",this,m_ConnectHandle);
				#endif

				rc = SQL_STILL_EXECUTING;
			}
			else if( m_StmtExecState == STMT_EXECUTION_DONE)
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::ExtendedFetch: Statement %x,Connection %x: Execution completed with returnCode %d\n",this,m_ConnectHandle,m_AsyncOperationReturnCode);
				#endif

				m_StmtExecState = STMT_EXECUTION_NONE;
				rc = m_AsyncOperationReturnCode;
			}
			else if( m_StmtExecState == STMT_EXECUTION_NONE)
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::ExtendedFetch: Statement %x,Connection %x: No Statement currently being executed on this statement handle\n",this,m_ConnectHandle);
				#endif

				rc = SendExtendedFetch(SkipProcess,FetchOrientation,FetchOffset,RowCountPtr,RowStatusArray);
			}
			else
			{
				#ifdef __DEBUGASYNCIO
				printf("CStmt::ExtendedFetch: Unexpected Async Execution State: %d\n",m_StmtExecState);
				#endif
				assert(0);
			}
		}
	}
	else
		rc = SendExtendedFetch(SkipProcess,FetchOrientation,FetchOffset,RowCountPtr,RowStatusArray);
#else
	rc = SendExtendedFetch(SkipProcess,FetchOrientation,FetchOffset,RowCountPtr,RowStatusArray);
#endif /* ASYNCIO */

	switch (rc)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
		rcCopyData = m_AppRowDesc->ExtendedCopyData(this, m_NumResultCols, m_ImpRowDesc,
			m_RowsFetched, m_CurrentRowFetched, m_RowNumber, m_RowsetSize, RowStatusArray);
		if (RowCountPtr != NULL) *RowCountPtr = m_RowsetSize;
		m_StmtState = STMT_FETCHED;
		m_CurrentRowInRowset = 1;
		break;
	case SQL_STILL_EXECUTING:
		m_StmtState = STMT_STILL_EXECUTING;
		break;
	case SQL_NO_DATA:
		m_StmtState = STMT_FETCHED_TO_END;
		break;
	case SQL_ERROR:
		if (m_AsyncCanceled == TRUE)
		{
			setDiagRec(DRIVER_ERROR, IDS_S1_008);
			m_AsyncCanceled = FALSE;
		}
		break;
	case SQL_NEED_DATA:
	case SQL_INVALID_HANDLE:
	default:
		break;
	}
	if(rcCopyData == SQL_ERROR || rc == SQL_ERROR)
		return SQL_ERROR;
	else if(rcCopyData == SQL_SUCCESS_WITH_INFO || rc == SQL_SUCCESS_WITH_INFO)
		return SQL_SUCCESS_WITH_INFO;
	else
		return rc;
}

SQLRETURN CStmt::FetchScroll(SQLUSMALLINT FetchOrientation,
			SQLINTEGER FetchOffset)
{
	if (FetchOrientation != SQL_FETCH_NEXT)
	{
		setDiagRec(DRIVER_ERROR, IDS_HY_105);
		return SQL_ERROR;
	}
	return Fetch();
}

SQLRETURN CStmt::GetData(SQLUSMALLINT ColumnNumber, 
			SQLSMALLINT TargetType,
			SQLPOINTER	TargetValuePtr, 
			SQLLEN	BufferLength,
			SQLLEN *StrLen_or_IndPtr)
{
	SQLRETURN rc = SQL_SUCCESS;
	unsigned long	retCode;
	SQLValue_def	SQLValue;
	unsigned long	Index;
	unsigned long	swapInfoIndex=0; 
	SQLUSMALLINT	*ArrayStatusPtr = NULL;
	UCHAR			errorMsg[MAX_TRANSLATE_ERROR_MSG_LEN];

	SQLINTEGER		SQLCharset=0;
	SQLSMALLINT		SQLDataType=0; 
	SQLSMALLINT		SQLDatetimeCode=0;
	SQLINTEGER		SQLOctetLength=0; 
	SQLSMALLINT		SQLPrecision=0;
	SQLSMALLINT		SQLUnsigned=0;

	short			SQLDataInd=0;
	int			SQLDataLength=0;
	BYTE*			SQLDataRow;
	BYTE*			SQLDataValue=0;

	unsigned long	offset=0;
	long stmtQueryType = getStmtQueryType();

	unsigned long columnIndexCounter=0;
	unsigned long indIndex=0;
	unsigned long dataIndex=0;
	BYTE		*IndicatorPtr;

	clearError();
	m_CurrentOdbcAPI = SQL_API_SQLGETDATA;
	if(ColumnNumber > m_NumResultCols)
	{
		setDiagRec(DRIVER_ERROR, IDS_07_009_02, 0, NULL, NULL, m_CurrentRowInRowset, ColumnNumber);
		return SQL_ERROR;
	}
	if (m_StmtState == STMT_FETCHED_TO_END || m_CurrentRowFetched > m_RowsFetched)
	{
		setDiagRec(DRIVER_ERROR, IDS_24_000);
		return SQL_ERROR;
	}
	if (m_CurrentFetchType == SQL_API_SQLFETCH)
		rc = m_ImpRowDesc->GetDescField(0, SQL_DESC_ARRAY_STATUS_PTR, &ArrayStatusPtr, 
			sizeof(ArrayStatusPtr), NULL);
	else if (m_CurrentFetchType == SQL_API_SQLEXTENDEDFETCH)
		ArrayStatusPtr = m_RowStatusArray;
	if (ArrayStatusPtr != NULL)
	{
		if (ArrayStatusPtr[m_CurrentRowInRowset] == SQL_ROW_DELETED || 
			ArrayStatusPtr[m_CurrentRowInRowset] == SQL_ROW_ERROR)
		{
			setDiagRec(DRIVER_ERROR, IDS_HY_109);
			return SQL_ERROR;
		}
	}
	Index = (m_CurrentRowFetched -1 - m_RowsetSize + m_CurrentRowInRowset ) * m_NumResultCols + ColumnNumber -1;
	swapInfoIndex = m_CurrentRowFetched -1 - m_RowsetSize + m_CurrentRowInRowset; 
	if (Index >= m_FetchDataValue.numberOfElements)
	{
		setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, NULL, NULL, 
							m_CurrentRowInRowset, ColumnNumber);
		return SQL_ERROR;
	}
	SQLDataRow = (BYTE*)m_FetchDataValue.rowAddress[m_CurrentRowFetched -1 - m_RowsetSize + m_CurrentRowInRowset];
	offset = 0;
	SQLDataValue = 0;

	if(!getFetchCatalog()) // if (stmtQueryType == 10000)
	{
		BOOL byteSwap = getByteSwap(); 
		SQLDataInd = 0;
		columnIndexCounter = (ColumnNumber-1)*2;
		getImpBulkSQLData(ColumnNumber, SQLCharset, SQLDataType, SQLDatetimeCode, SQLOctetLength, SQLPrecision,SQLUnsigned);
		
		if ((indIndex=m_ColumnIndexes[columnIndexCounter]) != -1)
		{	
			IndicatorPtr = SQLDataRow + indIndex;
		}
		else
			IndicatorPtr = NULL;
				
		dataIndex = m_ColumnIndexes[++columnIndexCounter];
		SQLDataValue = SQLDataRow + dataIndex;
	
		if ((IndicatorPtr == NULL) || (IndicatorPtr != NULL && *((short *)IndicatorPtr) != -1))
		{
			// Swapping is done only once, the first time. The returnedLength() function prevents 
			// additional swap of column values incase data is returned by multiple calls of GetData().
			// m_SwapInfo information can not be used for this purpose because 1, it keeps track only
			// row level swap information 2, it contains useful values only when the application had
			// called SQLBindCol() before (a SQLFetch() and) SQLGetData()
			//if ((byteSwap) && (m_ImpRowDesc->returnedLength(ColumnNumber) == 0) && (m_SwapInfo[swapInfoIndex] == 'N'))
			
			int returnedLength = m_ImpRowDesc->returnedLength(ColumnNumber);
			if ((byteSwap) && (returnedLength == 0) && (m_SwapInfo[m_CurrentRowFetched - 1][ColumnNumber - 1] == 'N'))
				SQLDatatype_Dependent_Swap(SQLDataValue, SQLDataType, SQLCharset, SQLOctetLength, SQLDatetimeCode);

			SQLDataLength = SQLOctetLength;
		}
		else 
		{
			SQLDataValue = 0;
			SQLDataLength = 0;
			SQLDataInd = -1;
		}
		
		switch (SQLDataType)
		{
			case SQLTYPECODE_CHAR:
			case SQLTYPECODE_BIT:
			case SQLTYPECODE_VARCHAR:
			{
				//SQLDataLength-1, to take care of null terminator
				//10-071209-9343 
				//if (m_MaxLength > 0)
				//	SQLDataLength = ((SQLDataLength-1)>m_MaxLength)?m_MaxLength + 1:SQLDataLength;
				//if (SQLDataInd != -1) SQLDataLength++;
				bool increment = false;
				//10-080311-1283
				SQLUINTEGER maxLength = m_MaxLength;
				if (SQLCharset == SQLCHARSETCODE_UCS2) 
					maxLength = m_MaxLength * 2;
				if (maxLength > 0) 
				{
					if (SQLDataLength - 1 > maxLength) 
					{
						SQLDataLength = maxLength + 1;
						increment = true;
					}
				}
				if (SQLDataInd != -1 && !increment) 
					SQLDataLength++;
			}
			break;
			case SQLTYPECODE_VARCHAR_WITH_LENGTH:
			case SQLTYPECODE_VARCHAR_LONG:
			case SQLTYPECODE_BITVAR:
			{	
				//10-080311-1283
				SQLUINTEGER maxLength = m_MaxLength;
				if ((SQLCharset == SQLCHARSETCODE_UCS2) && (m_ICUConv->m_AppUnicodeType == APP_UNICODE_TYPE_UTF16)) 
					maxLength = m_MaxLength * 2;
				if (maxLength > 0)
				{
					SQLDataLength = *(USHORT *)SQLDataValue;
					if (SQLDataLength > maxLength)
						*(USHORT *)SQLDataValue = maxLength;
					//SQLDataLength-3 to account for length(2 bytes) and null terminator(1 byte)
					//SQLDataLength = ((SQLDataLength-3)>maxLength)?maxLength + 3:SQLDataLength;
					//*(USHORT *)SQLDataValue = SQLDataLength - 3;
					break;
				}
			}
			break;
		}
		SQLValue.dataType = SQLDataType;
		SQLValue.dataInd = SQLDataInd;
		SQLValue.dataCharset = SQLCharset;
		SQLValue.dataValue._length = SQLDataLength;
		SQLValue.dataValue._buffer = (IDL_octet *)SQLDataValue;

	}
	else
	{
		for (int ColumnCount=0; ColumnCount < ColumnNumber; ColumnCount++)
		{
			SQLDataValue = SQLDataRow;
			SQLDataInd = (short)*(unsigned char*)(SQLDataValue);
			if (SQLDataInd == 0)
			{
				getImpSQLData(ColumnCount+1, SQLCharset, SQLDataType, SQLDatetimeCode, SQLOctetLength, SQLPrecision,SQLUnsigned);
				SQLDataValue = SQLDataValue + 1;
				SQLDataLength = dataLengthFetchPerf(SQLDataType, SQLOctetLength, getMaxLength(), (unsigned char* )SQLDataValue);
				SQLDataRow = SQLDataRow + 1 + SQLDataLength;
			}
			else
			{
				SQLDataValue = 0;
				SQLDataLength = 0;
				SQLDataInd = -1;
				SQLDataRow = SQLDataRow + 1;
			}
		}
		if (getAPIDecision() && !getFetchCatalog())
			switch (SQLDataType)
			{
			case SQLTYPECODE_CHAR:
			case SQLTYPECODE_BIT:
			case SQLTYPECODE_VARCHAR:
				if (SQLDataInd != -1) SQLDataLength++;
				break;
			}
		SQLValue.dataType = SQLDataType;
		SQLValue.dataInd = SQLDataInd;
		SQLValue.dataCharset = SQLCharset;
		SQLValue.dataValue._length = SQLDataLength;
		SQLValue.dataValue._buffer = (IDL_octet *)SQLDataValue;
	}

	retCode = m_ImpRowDesc->GetData(&SQLValue, ColumnNumber,
				TargetType, TargetValuePtr, BufferLength, StrLen_or_IndPtr, 
				errorMsg, sizeof(errorMsg), m_bFetchCatalog);
	if (retCode != SQL_SUCCESS && retCode != SQL_NO_DATA)
	{
		if (errorMsg[0] != '\0')
			setDiagRec(DRIVER_ERROR, retCode, 0, (char *)errorMsg, NULL, m_CurrentRowFetched, ColumnNumber);
		else
			setDiagRec(DRIVER_ERROR, retCode, 0, NULL, NULL,  m_CurrentRowInRowset, ColumnNumber);
		switch (retCode)
		{
		case IDS_01_004:
		case IDS_01_S07:
		case IDS_22_003:
		case IDS_186_DSTODRV_TRUNC:
			rc = SQL_SUCCESS_WITH_INFO;
			break;
		default:
			rc = SQL_ERROR;
		}
	}
	else
		rc = retCode;
	return rc;
}


SQLRETURN CStmt::SetPos(SQLUSMALLINT RowNumber,
			SQLUSMALLINT Operation,
			SQLUSMALLINT LockType)
{
	SQLRETURN rc = SQL_SUCCESS;
	SQLUSMALLINT	*ArrayStatusPtr = NULL;

	clearError();
	m_CurrentOdbcAPI = SQL_API_SQLSETPOS;
	switch (Operation)
	{
	case SQL_POSITION:
		if (m_StmtState == STMT_EXECUTED || m_StmtState == STMT_FETCHED_TO_END)
		{
			setDiagRec(DRIVER_ERROR, IDS_24_000);
			return SQL_ERROR;
		}
		if (RowNumber == 0 || RowNumber > m_RowsetSize)
		{
			setDiagRec(DRIVER_ERROR, IDS_HY_107);
			return SQL_ERROR;
		}
		if (m_CurrentFetchType == SQL_API_SQLFETCH)
			rc = m_ImpRowDesc->GetDescField(0, SQL_DESC_ARRAY_STATUS_PTR, &ArrayStatusPtr, 
				sizeof(ArrayStatusPtr), NULL);
		else if (m_CurrentFetchType == SQL_API_SQLEXTENDEDFETCH)
			ArrayStatusPtr = m_RowStatusArray;
		if (ArrayStatusPtr != NULL)
		{
			if (ArrayStatusPtr[RowNumber] == SQL_ROW_DELETED || 
				ArrayStatusPtr[RowNumber] == SQL_ROW_ERROR)
			{
				setDiagRec(DRIVER_ERROR, IDS_HY_109);
				return SQL_ERROR;
			}
		}
		m_CurrentRowInRowset = RowNumber;
		m_ImpRowDesc->SetPos(m_NumResultCols);
		break;
	default:
		setDiagRec(DRIVER_ERROR, IDS_HY_C00);
		break;
	}
	return rc;
}

long CStmt::getImpDescCount(){	return m_ImpRowDesc->getDescCount();}

BOOL CStmt::getImpSQLData(long colnumber,
	SQLINTEGER& SQLCharset, SQLSMALLINT& SQLDataType, 
	SQLSMALLINT& SQLDatetimeCode, SQLINTEGER& SQLOctetLength, 
	SQLSMALLINT& SQLPrecision, SQLSMALLINT& SQLUnsigned)
{
	return m_ImpRowDesc->getDescRecSQLData(colnumber,
			SQLCharset, SQLDataType, 
			SQLDatetimeCode, SQLOctetLength, 
			SQLPrecision,	SQLUnsigned);
}

BOOL CStmt::getImpBulkSQLData(long colnumber,
	SQLINTEGER& SQLCharset, SQLSMALLINT& SQLDataType, 
	SQLSMALLINT& SQLDatetimeCode, SQLINTEGER& SQLOctetLength, 
	SQLSMALLINT& SQLPrecision, SQLSMALLINT& SQLUnsigned)
{

	BOOL rc = m_ImpRowDesc->getDescRecSQLData(colnumber,
			SQLCharset, SQLDataType, 
			SQLDatetimeCode, SQLOctetLength, 
			SQLPrecision,	SQLUnsigned);
	if (SQLDataType == SQL_CHAR)
		SQLOctetLength = SQLOctetLength -1;
	return rc;
}

long CStmt::getImpRowDescCount(){	return m_ImpRowDesc->getDescCount();}

BOOL CStmt::getImpRowData(long colnumber,
	SQLINTEGER& SQLCharset, SQLSMALLINT& SQLDataType, 
	SQLSMALLINT& SQLDatetimeCode, SQLINTEGER& SQLMaxLength, 
	SQLSMALLINT& SQLPrecision, SQLSMALLINT& SQLUnsigned,
	SQLSMALLINT& SQLNullable)
{
	return m_ImpRowDesc->getDescRowRecData(colnumber,
			SQLCharset, SQLDataType, 
			SQLDatetimeCode, SQLMaxLength, 
			SQLPrecision,	SQLUnsigned,
			SQLNullable);
}

BOOL CStmt::getImpRowData(long colnumber, SQLSMALLINT& SQLDataType, 
			  SQLINTEGER& SQLMaxLength, SQLSMALLINT& SQLNullable)
{
	return m_ImpRowDesc->getDescRowRecData(colnumber,
			SQLDataType, SQLMaxLength, SQLNullable);
}

long CStmt::getImpParamDescCount(){	return m_ImpParamDesc->getDescCount();}

BOOL CStmt::getImpParamData(long colnumber,
	SQLINTEGER& SQLCharset, SQLSMALLINT& SQLDataType, 
	SQLSMALLINT& SQLDatetimeCode, SQLINTEGER& SQLMaxLength, 
	SQLSMALLINT& SQLPrecision, SQLSMALLINT& SQLUnsigned,
	SQLSMALLINT& SQLNullable)
{
	return m_ImpParamDesc->getDescParamRecData(colnumber,
			SQLCharset, SQLDataType, 
			SQLDatetimeCode, SQLMaxLength, 
			SQLPrecision,	SQLUnsigned,
			SQLNullable);
}


BOOL CStmt::checkInputParam( std::string SqlString )
{
	char del;
	char ch;
	int state = 0;

	unsigned int length = SqlString.length();

	for (unsigned int i=0; i < length; i++)
	{
		ch = SqlString[i];
		switch (state)
		{
		case 0:
			if (ch == '?' ) return TRUE;
			else if (ch == '\'' || ch == '"' ) { state = 1; del = ch; }
			break;
		case 1:
			if (ch == del ) { state = 2; }
			break;
		case 2:
			if (ch == del ) { state = 1; }
			else { i--; state = 0; }
			break;
		}
	}
	return FALSE;
}

bool CStmt::isInfoStats(char *inpString)
{
	bool result = false;

	char *ptr = inpString;
	while (*ptr != '\0' && isspace(*ptr))
		ptr++;
	if (_strnicmp(ptr, "INFOSTATS", 9) == 0)  
		result = true;

	return result;
}

// Passing TRUE for padDelimitedIdentifiers while substitute 'X' as character for all delimited identifiers. So that in POSITIONED
// UPDATE/DELETE for a table can be searched easily for "WHERE CURRENT OF" string.
void CStmt::trimSqlString(unsigned char* inpString, unsigned char* outString, unsigned long& sqlStringLen, BOOL padDelimitedIdentifiers)
{
	char del;
	char ch;
	int state = 0;
	BOOL blank_added = FALSE;
	BOOL copy = FALSE;
	unsigned long len = 0;


	if (isInfoStats((char *)inpString))
	{
		strncpy((char *)outString, (const char *)inpString, sqlStringLen);
		outString[sqlStringLen] = '\0';
		return;
	}

	for (unsigned long i=0; i < sqlStringLen; i++)
	{
		ch = inpString[i];
		copy = TRUE;

		switch (state)
		{
		case 0:
			if (ch == '\'' || ch == '"' ) { state = 1; del = ch; blank_added = FALSE; }
			//else if (isspace(ch))
			else if (isblank(ch)) // Replaced with isblank() so we don't strip the EOL characters
			{
				if (blank_added == FALSE)
				{
					blank_added = TRUE;
					ch = ' ';
				}
				else
					copy = FALSE;
			}
			else if (blank_added)
				blank_added = FALSE;
			break;
		case 1:
			if (ch == del ) { state = 2; }
			if (padDelimitedIdentifiers)
				ch = 'X';
			break;
		case 2:
			if (ch == del ) { state = 1; }
			else { i--; state = 0; copy = FALSE;}
			if (padDelimitedIdentifiers)
				ch = 'X';
			break;
		}
		if (copy)
		{
			if ( state == 0)
				outString[len++] = toupper(ch);
			else
				outString[len++] = ch;
		}
			
	}
	if (outString[len-1] == ' ')
		outString[--len] = '\0';
	else
		outString[len] = '\0';
	sqlStringLen = len;
}

void CStmt::setNumberOfElements(unsigned long count)
{
	m_FetchDataValue.numberOfElements = count;
}

unsigned long CStmt::getNumberOfElements(void)
{
	return m_FetchDataValue.numberOfElements;
}

void CStmt::setNumberOfRows(unsigned long count)
{
	m_FetchDataValue.numberOfRows = count;
}

unsigned long CStmt::getNumberOfRows(void)
{
	return m_FetchDataValue.numberOfRows;
}

void CStmt::setRowAddress( unsigned long row, BYTE* address)
{
	if (row <= m_FetchDataValue.numberOfRows)
		m_FetchDataValue.rowAddress[row] = (unsigned long)address;
}

BYTE* CStmt::getRowAddress( unsigned long row)
{
	if (row <= m_FetchDataValue.numberOfRows)
		return (BYTE*)m_FetchDataValue.rowAddress[row];
	else
		return 0;
}

void CStmt::clearFetchDataValue(void)
{
	if (m_outputDataValue._length != 0 &&
		m_outputDataValue._buffer != NULL)
		delete[] m_outputDataValue._buffer;
	m_outputDataValue._length = 0;
	m_outputDataValue._buffer = NULL;
	if (m_FetchDataValue.rowAddress != NULL)
		delete[] m_FetchDataValue.rowAddress;
	m_FetchDataValue.numberOfElements = 0;
	m_FetchDataValue.numberOfRows = 0;
	m_FetchDataValue.rowAddress = NULL;
	if (m_ColumnIndexes != NULL)
	{
		delete[] m_ColumnIndexes;
		m_ColumnIndexes =  NULL;
	}
	if (m_SwapInfo != NULL)
	{
		for (int r = 0; r < m_SwapInfo_NumRows; r++) 
		{
			if (m_SwapInfo[r] != NULL)
				delete[] (m_SwapInfo[r]);
		}
		delete[] m_SwapInfo;
		m_SwapInfo = NULL;
		m_SwapInfo_NumRows = 0;
	}
}

void CStmt::clearInputValueList(void)
{
	if (m_InputValueList._buffer != NULL)
	   delete[] m_InputValueList._buffer;

	m_InputValueList._buffer = 0;
	m_InputValueList._length = 0;
}

void CStmt::restoreDefaultDescriptor(CDesc* desc)
{ 
	if (m_AppParamDesc == desc)
		m_AppParamDesc = m_APDDesc;
	else if (m_AppRowDesc == desc)
		m_AppRowDesc = m_ARDDesc;
}

SQLRETURN CStmt::mapSqlRowIdsToDrvrRowIds(BYTE *&WarningOrError)
{
	SQLULEN AppArrayStatusSize;
	IDL_long sqlRowId = -1, drvrRowId = -1;  //sql row id starts counting at 1, not 0
	unsigned char *curptr = (unsigned char*)WarningOrError;
	long msg_total_len = 0;

	IDL_long numConditions = 0;
	char sqlState[6];
	IDL_long errorTextLen = 0;
	int i;
	
	if (curptr == NULL)
		return SQL_ERROR;
		
	numConditions = *(IDL_long*)(curptr+msg_total_len);
	msg_total_len +=4;

	if (numConditions > 0)
	{
#ifdef __LP64__
		m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &AppArrayStatusSize, SQL_IS_POINTER, NULL);
#else
		m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &AppArrayStatusSize, SQL_IS_UINTEGER, NULL);
#endif

		for (i = 0; i < numConditions; i++)
		{
			sqlRowId= *(IDL_long*)(curptr+msg_total_len);
			
			if (AppArrayStatusSize > sqlRowId  && sqlRowId > 0) 
			{
				//find trueRowId:
				drvrRowId = m_RowIdMap[sqlRowId-1];
				memcpy(curptr+msg_total_len,&drvrRowId,sizeof(IDL_long)); 
			}	
			msg_total_len +=4;  //move past rowId
			msg_total_len +=4;  //move past sqlCode

			errorTextLen= *(IDL_long*)(curptr+msg_total_len);
			msg_total_len +=4;  //move past errorTextLen
			msg_total_len +=errorTextLen;
			msg_total_len +=sizeof(sqlState);
		}
	}

	return SQL_SUCCESS;
}

// For Stored Proceudre Call support
SQLRETURN CStmt::setExecuteCallOutputs()
{
	m_AppParamDesc->CopyData(this, m_NumResultCols, m_ImpParamDesc,
		m_RowsFetched, m_CurrentRowFetched, m_RowNumber, m_RowsetSize);
	return SQL_SUCCESS;
}

CTCPIPUnixDrvr* CStmt::getSrvrTCPIPSystem()
{
	return m_ConnectHandle->m_srvrTCPIPSystem; 
}

void CStmt::setRowsetArrayStatus(const ERROR_DESC_LIST_def *sqlWarning, long rowsAffected)
{
	SQLULEN AppArrayStatusPtr;
	SQLULEN     AppArrayStatusSize;
	ERROR_DESC_def *_buffer;
	IDL_long rowId;
	unsigned long i,count;

#ifdef __LP64__
	m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &AppArrayStatusSize, SQL_IS_POINTER, NULL);
	m_ImpParamDesc->GetDescField(0, SQL_DESC_ROWS_PROCESSED_PTR, &AppArrayStatusPtr, SQL_IS_POINTER, NULL);
#else
	m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &AppArrayStatusSize, SQL_IS_UINTEGER, NULL);
	m_ImpParamDesc->GetDescField(0, SQL_DESC_ROWS_PROCESSED_PTR, &AppArrayStatusPtr, SQL_IS_UINTEGER, NULL);
#endif

	if (AppArrayStatusPtr == NULL)
		return;
	
	if (rowsAffected == -1)
	{
		m_ImpParamDesc->setDescArrayStatus(AppArrayStatusSize, SQL_PARAM_ERROR);
		return;
	}
	else
		m_ImpParamDesc->setDescArrayStatus(AppArrayStatusSize, SQL_PARAM_SUCCESS);

	if (rowsAffected == AppArrayStatusSize)
		return;

	for (i=0; i < sqlWarning->_length; i++)
	{
		_buffer = sqlWarning->_buffer + i;
		rowId = _buffer->rowId - 1;
		if (rowId > AppArrayStatusSize) break;
		if (rowId < 0) continue;
		m_ImpParamDesc->setDescArrayStatusAt(rowId, SQL_PARAM_ERROR);
	}

	for (i=0,count=1; i <AppArrayStatusSize; i++)
	{
		if (m_ImpParamDesc->getDescArrayStatusFrom(i) == SQL_PARAM_SUCCESS)
		{
			if (count++ > rowsAffected)
			{
				m_ImpParamDesc->setDescArrayStatusAt(i, SQL_PARAM_ERROR);
				setDiagRec(SERVER_ERROR, IDS_SQL_ERROR, 0, NULL, NULL, i+1);
				continue;
			}
		}
	}

}

void CStmt::setRowsetArrayStatus(BYTE *&WarningOrError, long rowsAffected)
{
	SQLULEN AppArrayStatusPtr;
	SQLULEN AppArrayStatusSize;

	long msg_total_len = 0;

	IDL_long numConditions = 0;
	char sqlState[6];
	IDL_long errorTextLen = 0;
	IDL_long rowId = 0;
	IDL_long rowIdSaved = 0;

	sqlState[0] = '\0';

	unsigned char *curptr;
	int i;

#ifdef __LP64__
	m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &AppArrayStatusSize, SQL_IS_POINTER, NULL);
	m_ImpParamDesc->GetDescField(0, SQL_DESC_ARRAY_STATUS_PTR , &AppArrayStatusPtr, SQL_IS_POINTER, NULL);
#else
	m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &AppArrayStatusSize, SQL_IS_UINTEGER, NULL);
	m_ImpParamDesc->GetDescField(0, SQL_DESC_ARRAY_STATUS_PTR , &AppArrayStatusPtr, SQL_IS_UINTEGER, NULL);
#endif

	if (AppArrayStatusPtr == NULL)
		return;
	
	if (rowsAffected == -1)
	{
		if(!rowsetErrorRecovery())
		   m_ImpParamDesc->setDescArrayStatus(AppArrayStatusSize, SQL_PARAM_UNUSED);
		else
		{
		m_ImpParamDesc->setDescArrayStatus(AppArrayStatusSize, SQL_PARAM_ERROR);
		return;
	}
	}

	if (rowsAffected == AppArrayStatusSize)
		return;

	curptr = (unsigned char *)WarningOrError;

	if (curptr == NULL)
		return;

	numConditions = *(IDL_long*)(curptr+msg_total_len);
	msg_total_len +=4;

	if (numConditions > 0)
	{
		for (i = 0; i < numConditions; i++)
		{
			rowId= *(IDL_long*)(curptr+msg_total_len);
			msg_total_len +=4;  //move past rowId
			msg_total_len +=4;  //move past sqlCode

			errorTextLen= *(IDL_long*)(curptr+msg_total_len);
			msg_total_len +=4;  //move past errorTextLen
			msg_total_len +=errorTextLen;
			msg_total_len +=sizeof(sqlState);

			rowId = rowId -1; // RowId index starts from 0
			if (rowId > AppArrayStatusSize) break;

			if (rowId < 0) continue;

			rowIdSaved = rowId;
			m_ImpParamDesc->setDescArrayStatusAt(rowId, SQL_PARAM_ERROR);
		}
	}
	// for Atomic, set the status of all the ones before to SQL_PARAM_SUCCESS
	if(!rowsetErrorRecovery())
	for(i = 0; i < rowIdSaved; i++)
	   m_ImpParamDesc->setDescArrayStatusAt(i, SQL_PARAM_SUCCESS);
}

void CStmt::setColumnOffset(short columnNumber, long offset)
{
	long totalLength = m_outputDataValue._length;
	long columnOffset = sizeof(long) * (columnNumber - 1);
	totalLength = ((totalLength + 4 - 1) >> 2) << 2;
	*(long*)(&m_outputDataValue._buffer[totalLength + columnOffset]) = offset;
}

long CStmt::getColumnOffset(short columnNumber)
{
	long totalLength = m_outputDataValue._length;
	long columnOffset = sizeof(long) * (columnNumber - 1);
	totalLength = ((totalLength + 4 - 1) >> 2) << 2;
	return *(long*)(&m_outputDataValue._buffer[totalLength + columnOffset]);
}

// overriden base class method CHandle::clearError()
void CStmt::clearError()
{
	EnterCriticalSection2(&m_CSObject);
	if(m_AsyncEnable == SQL_ASYNC_ENABLE_ON &&
	  (m_AsyncCanceled == TRUE || m_StmtExecState == STMT_EXECUTION_EXECUTING || m_StmtExecState == STMT_EXECUTION_DONE))
	{
	  LeaveCriticalSection2(&m_CSObject);
	  return;
	}
	   
	if (m_DiagRec.getClearDiagRec())
	{
		m_DiagRec.clear();
		m_SqlWarning = FALSE;
		m_ExceptionNr = CEE_SUCCESS;
	}
	LeaveCriticalSection2(&m_CSObject);
}


