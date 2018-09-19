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


#include "process.h"
#include "CStmt.h"
#include "tdm_odbcDrvMsg.h"
#include "DrvrNet.h"
#include "DrvrSrvr.h"
#include "DrvrGlobal.h"
#include "nskieee.h"
#include "sqlcli.h"
#include "swap.h"
#include "csconvert.h"
#include "DiagFunctions.h"

using namespace ODBC;

DWORD wait_for_event(HANDLE hEvent,long timeout);

// Implements the member functions of CStmt

CStmt::CStmt(SQLHANDLE InputHandle) : CHandle(SQL_HANDLE_STMT, InputHandle)
{
    m_AsyncThread	= NULL;
    m_ThreadStatus	= SQL_SUCCESS;
    m_StmtEvent		= NULL;
    m_SyncThread	= NULL;

    m_StmtName[0]	= '\0';
    m_CursorName[0] = '\0';
    sprintf(m_StmtLabel, "SQL_CUR_%ld", m_HandleNumber);
    strcpy(m_StmtLabelOrg, m_StmtLabel);
    m_ConnectHandle = (CConnect *)m_InputHandle;
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
    EnterCriticalSection(&m_ConnectHandle->m_CSObject);
    m_ConnectHandle->m_StmtCollect.push_back(this);
    LeaveCriticalSection(&m_ConnectHandle->m_CSObject);
    m_RowStatusArray = NULL;
    m_CurrentFetchType = 0;
    m_AsyncCanceled = FALSE;
    m_RowCount = -1;

    m_FetchDataValue.numberOfElements = 0;
    m_FetchDataValue.numberOfRows = 0;
    m_FetchDataValue.rowAddress = NULL;
    m_SelectRowsets = m_ConnectHandle->m_SelectRowsets;
    m_FetchBufferSize = m_ConnectHandle->m_FetchBufferSize;

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
    m_intStmtType = TYPE_UNKNOWN;

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

    m_CancelCalled = false;
}


CStmt::~CStmt()
{
    CHANDLECOLLECT::iterator i;

    DWORD preFetchThreadStatus;

    if (this->m_preFetchThread.m_Thread != NULL)
    {
        GetExitCodeThread(this->m_preFetchThread.m_Thread, &preFetchThreadStatus);
        if (preFetchThreadStatus == STILL_ACTIVE)
            WaitForSingleObject(this->m_preFetchThread.m_Thread,INFINITE);

        GetExitCodeThread(this->m_preFetchThread.m_Thread, &preFetchThreadStatus);
        CloseHandle(this->m_preFetchThread.m_Thread);
        this->m_preFetchThread.m_Thread = NULL;
    }

    Close(SQL_DROP);
    clearError();
    if (m_ARDDesc != NULL)
        delete m_ARDDesc;
    if (m_IRDDesc != NULL)
        delete m_IRDDesc;
    if (m_APDDesc != NULL)
        delete m_APDDesc;
    if (m_IPDDesc != NULL)
        delete m_IPDDesc;
    if (m_AsyncThread != NULL)
        CloseHandle(m_AsyncThread);
    if (m_StmtEvent != NULL)
        CloseHandle(m_StmtEvent);
    if (m_InputValueList._buffer != NULL)
        delete m_InputValueList._buffer;
    if (m_FetchDataValue.rowAddress != NULL )
        delete[] m_FetchDataValue.rowAddress;

    if (m_outputDataValue._length != 0 &&
            m_outputDataValue._buffer != NULL)
        delete m_outputDataValue._buffer;

    if (m_InputParams != NULL)
        delete m_InputParams;
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

    if(m_spjResultSets != NULL)
        delete[] m_spjResultSets;

    if( m_spjInputParamDesc._length != 0 &&
            m_spjInputParamDesc._buffer != NULL)
        delete [] m_spjInputParamDesc._buffer;

    if( m_spjOutputParamDesc._length != 0 &&
            m_spjOutputParamDesc._buffer != NULL)
        delete [] m_spjOutputParamDesc._buffer;


    // Remove this from StmtCollection in Connection
    EnterCriticalSection(&m_ConnectHandle->m_CSObject);
    for (i = m_ConnectHandle->m_StmtCollect.begin() ; i !=  m_ConnectHandle->m_StmtCollect.end() ; ++i)
    {
        if ((*i) == this)
        {
            m_ConnectHandle->m_StmtCollect.erase(i);
            break;
        }
    }
    LeaveCriticalSection(&m_ConnectHandle->m_CSObject);
}

void CStmt::InitInputValueList()
{
    if (m_ParamBuffer != NULL)
        delete m_ParamBuffer;
    m_ParamBuffer = NULL;
    if (m_InputValueList._buffer != NULL)
        delete m_InputValueList._buffer;
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
    if (m_SwapInfo !=  NULL)
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
        delete m_InputParams;
    m_InputParams = NULL;
    if (m_OutputColumns != NULL)
        delete m_OutputColumns;
    m_OutputColumns = NULL;
}

SQLRETURN CStmt::Close(SQLUSMALLINT Option)
{
    SQLRETURN			rc = SQL_SUCCESS;

    if (m_ConnectHandle->getConnectionStatus() == SQL_CD_TRUE)
    {
        clearError();
        setDiagRec(DRIVER_ERROR, IDS_08_S01);
        rc = SQL_SUCCESS_WITH_INFO;
    }
    else if(!(m_isClosed && Option == SQL_CLOSE))
    {
        m_SrvrCallContext.odbcAPI = SQL_API_SQLFREESTMT;
        m_SrvrCallContext.sqlHandle = this;
        m_SrvrCallContext.SQLSvc_ObjRef = m_ConnectHandle->getSrvrObjRef();
        m_SrvrCallContext.ASSvc_ObjRef = NULL;
        m_SrvrCallContext.eventHandle = m_StmtEvent;
        m_SrvrCallContext.dialogueId = m_ConnectHandle->getDialogueId();
        m_SrvrCallContext.statementTimeout  = m_QueryTimeout;
        m_SrvrCallContext.u.closeParams.stmtLabel = m_StmtLabel;
        m_SrvrCallContext.u.closeParams.option = Option;
        rc = ThreadControlProc(&m_SrvrCallContext);
    }
    if (m_AsyncThread != NULL)
    {
        // TerminateThread
        CloseHandle(m_AsyncThread);
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
    m_AsyncCanceled = FALSE;
    m_isClosed = true;
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
    if (m_StmtEvent == NULL)
    {
        m_StmtEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (m_StmtEvent == NULL)
        {
            setNTError(m_ConnectHandle->getErrorMsgLang(), "initialize - CreateEvent()");
            return SQL_ERROR;
        }
    }
    return SQL_SUCCESS;
}

SQLRETURN CStmt::SetStmtAttr(SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength)
{
    SQLRETURN	rc = SQL_SUCCESS;
    unsigned long	retCode = SQL_SUCCESS;
    SQLUINTEGER		Value;

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
            m_AsyncEnable = (SQLUINTEGER)ValuePtr;
            break;
        case SQL_ATTR_CONCURRENCY:
            //  Added code to send to server about the SQL_ATTR_CONCURRENCY to perform
            // "CONTROL QUERY DEFAULT CURSOR_READONLY 'TRUE or FALSE'" based on
            // SQL_CONCUR_READ_ONLY or SQL_CONCUR_LOCK attributes.
            Value = (SQLUINTEGER)ValuePtr;
            if (m_ConnectHandle->getConnectionStatus() == SQL_CD_TRUE)
            {
                clearError();
                setDiagRec(DRIVER_ERROR, IDS_08_S01, -20005);
                return SQL_ERROR;
            }
            else
                rc = m_ConnectHandle->SetConnectAttr(Attribute, (SQLUINTEGER)ValuePtr, NULL);
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
            m_CursorHoldable = (SQLUINTEGER)ValuePtr;
            break;
        case SQL_ATTR_CURSOR_SCROLLABLE:
            if ((SQLUINTEGER)ValuePtr == SQL_NONSCROLLABLE)
                m_CursorScrollable = (SQLUINTEGER)ValuePtr;
            else
            {
                m_CursorScrollable = SQL_NONSCROLLABLE;
                retCode = IDS_01_S02;
                rc = SQL_SUCCESS_WITH_INFO;
            }
            break;
        case SQL_ATTR_CURSOR_SENSITIVITY:
            Value = (SQLUINTEGER)ValuePtr;
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
            Value = (SQLUINTEGER)ValuePtr;
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
            m_EnableAutoIPD = (SQLUINTEGER)ValuePtr;
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
            m_MetadataId = (SQLUINTEGER)ValuePtr;
            break;
        case SQL_ATTR_NOSCAN:
            m_Noscan = (SQLUINTEGER)ValuePtr;
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
            m_QueryTimeout = (SQLUINTEGER)ValuePtr;
            if(m_QueryTimeout != 0 && m_QueryTimeout < 30)
                m_QueryTimeout = 30;
            break;
        case SQL_ATTR_RETRIEVE_DATA:
            m_RetrieveData = (SQLUINTEGER)ValuePtr;
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
            m_SimulateCursor = (SQLUINTEGER)ValuePtr;
            break;
        case SQL_ATTR_USE_BOOKMARKS:
            Value = (SQLUINTEGER)ValuePtr;
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
            m_FetchBufferSize = (SQLINTEGER)ValuePtr;
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
#ifdef _WIN64
            retValue.u.s64Value = m_KeysetSize;
            retValue.dataType = SQL_C_UBIGINT;
#else
            retValue.u.u32Value = m_KeysetSize;
            retValue.dataType = SQL_IS_UINTEGER;
#endif
            break;
        case SQL_ATTR_MAX_LENGTH:
#ifdef _WIN64
            retValue.u.s64Value = m_MaxLength;
            retValue.dataType = SQL_C_UBIGINT;
#else
            retValue.u.u32Value = m_MaxLength;
            retValue.dataType = SQL_IS_UINTEGER;
#endif
            break;
        case SQL_ATTR_MAX_ROWS:
#ifdef _WIN64
            retValue.u.s64Value = m_MaxRows;
            retValue.dataType = SQL_C_UBIGINT;
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
#ifdef _WIN64
            retValue.u.s64Value = m_RowNumber;
            retValue.dataType = SQL_C_UBIGINT;
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
        SQLLEN	BufferLength,
        SQLLEN *StrLen_or_IndPtr)
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
        SQLULEN		ColumnSize,
        SQLSMALLINT DecimalDigits,
        SQLPOINTER  ParameterValuePtr,
        SQLLEN		BufferLength,
        SQLLEN		*StrLen_or_IndPtr)
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
    SQLULEN				MaxRowsetSize;
    SQLULEN				AppArrayStatusSize;

    BYTE	*inputParams = 0;
    BYTE	*outputColumns = 0;
    SQLRETURN	rc;

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

        InitParamColumnList();
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

        sqlString = new BYTE[sqlStringLen + 1];
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

        //		if (sqlStringLen == 0 || sqlStringLen > USHRT_MAX)
        if (sqlStringLen <= 0 )
        {
            setDiagRec(DRIVER_ERROR, IDS_S1_090);
            delete[] sqlString;
            return SQL_ERROR;
        }

        if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
            TraceOut(TR_ODBC_API, "CStmt::SendSQLCommand sqlString \"%s\", sqlStringLen %d",
                    sqlString, sqlStringLen);

        // sqlString is already in UTF8
        m_SqlString.assign((const char *)sqlString, sqlStringLen);

        if (sqlStringLen < sizeof(tempStr))
            len = sqlStringLen;
        else
            len = sizeof(tempStr)-1;
        strcpyUTF8(tempStr, (const char *)m_SqlString.c_str(), sizeof(tempStr), len);

        token = strtok(tempStr, delimiters);
        if (token == NULL)
        {
            setDiagRec(DRIVER_ERROR, IDS_S1_009);
            delete sqlString;
            return SQL_ERROR;
        }

        /* Vijay - Changes not to block statements of type SELECT
           if ((strcmp(token, "SELECT") == 0) || (strcmp(token, "SEL") == 0)
           || (strcmp(token, "LOCK") == 0) || (strcmp(token, "LOCKING") == 0)
           || (strcmp(token, "INVOKE") == 0) || (strcmp(token, "EXPLAIN") == 0)
           || (strcmp(token, "SHOWCONTROL") == 0) || (strcmp(token, "SHOWDDL") == 0)
           || (strcmp(token, "SHOWSHAPE") == 0) || (strcmp(token, "SHOWPLAN") == 0)
           || (strcmp(token, "SHOWLABEL") == 0) || (strcmp(token, "MAINTAIN") == 0)
           || (strcmp(token, "REORG") == 0) || (strcmp(token, "REORGANIZE") == 0)
           || (strcmp(token, "PURGEDATA") == 0)) // JoyJ - Added for PURGEDATA
           m_StmtType = TYPE_SELECT;
           */
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
            else if (_strnicmp(token, "DELETE", 6) == 0)
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
                (strcmp(token, "UPSERT") == 0))  // Add for "UPSERT" support
            m_StmtType = TYPE_INSERT;
        else if ((strcmp(token, "UPDATE") == 0) || (strcmp(token, "MERGE") == 0))
            m_StmtType = TYPE_UPDATE;
        else if ((strcmp(token, "DELETE") == 0))
            m_StmtType = TYPE_DELETE;
        else if (strcmp(token, "SMD") == 0) // added for user module support
            m_StmtType = TYPE_SMD;
        else if ((strcmp(token, "CALL") == 0) || (strcmp(token, "?=CALL") == 0)) //  added for stored proc call support
            m_StmtType = TYPE_CALL;
        else if (_strnicmp(token, "INFOSTATS", 9) == 0)
            m_StmtType = TYPE_STATS;
        else if (strcmp(token, "?=") == 0)
        {
            token = strtok(NULL, delimiters);
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
            token = strtok(NULL, delimiters);
            if (token == NULL)
            {
                setDiagRec(DRIVER_ERROR, IDS_S1_009);
                delete sqlString;
                return SQL_ERROR;
            }
            if ((strcmp(token, "=") == 0) && (strcmp(strtok(NULL, delimiters), "CALL") == 0))
                m_StmtType = TYPE_CALL;
            else
                m_StmtType = TYPE_UNKNOWN;
        }
        else if (_strnicmp(token, "LOBUPDATE", 9) == 0)
        {
            if (m_SrvrCallContext.u.extractLobParams.lobHandle == NULL)
            {
                return SQL_ERROR;
            }
            m_CurrentOdbcAPI = SRVR_API_UPDATELOB;
            return SQL_NEED_DATA;
        }

        else
        {
            /*
            // In case we decide to support "SET CATALOG and SET SCHEMA" through EXECUTE, Strongly this
            // should not be allowed.
            if (strcmp(token, "SET") == 0)
            {
            char	settempStr[300];
            char	*settoken;
            char	*setdelimiters = ".\n";
            char	catTempStr[128+1];
            char	schTempStr[128+1];
            catTempStr[0] = '\0';
            schTempStr[0] = '\0';
            if (sqlStringLen < sizeof(settempStr))
            len = sqlStringLen;
            else
            len = sizeof(settempStr)-1;
            token = strtok(NULL, delimiters);
            if (strcmp(token, "CATALOG") == 0)
            {
            token = strtok(NULL, delimiters);
            strncpy(settempStr, (const char *)m_SqlString.c_str() + (token-(const char *)m_SqlString.c_str()), len);
            settempStr[len] = '\0';
            settoken = strtok(settempStr, setdelimiters); // ignore first token
            if (settoken != NULL)
            //						Do catalog change here
            }
            else if (strcmp(token, "SCHEMA") == 0)
            {
            token = strtok(NULL, delimiters);
            strncpy(settempStr, (const char *)m_SqlString.c_str() + (token-(const char *)m_SqlString.c_str()), len);
            settempStr[len] = '\0';
            settoken = strtok(settempStr, setdelimiters); // ignore first token
            if (settoken != NULL)
            {
            strcpy(schTempStr,settoken);
            settoken = strtok(NULL, setdelimiters);
            if (settoken != NULL)
            {
            strcpy(catTempStr,schTempStr);
            strcpy(schTempStr,settoken);
            }
            if (catTempStr[0] != '\0')
            //								Do catalog change here
            if (schTempStr[0] != '\0')
            //							Do schema change here
            }
            }
            }
            */
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

                strcpyUTF8(tCursor, (char *)sqlString + rwhereCurrentof, sizeof(tCursor), sqlStringLen - rwhereCurrentof);
                if (tCursor != NULL)
                {
                    CHANDLECOLLECT::iterator itr;
                    for (itr = m_ConnectHandle->m_StmtCollect.begin() ; itr !=  m_ConnectHandle->m_StmtCollect.end() ; ++itr)
                    {
                        if(strcmp(tCursor, ((CStmt *)(*itr))->m_CursorName) == 0)
                        {
                            tConcurrency = ((CStmt *)(*itr))->m_Concurrency;
                            tRowsFetched = ((CStmt *)(*itr))->m_RowsFetched;
                            break;
                        }
                    }
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

        delete sqlString;

        MaxRowsetSize = 0;

        if (m_intStmtType != TYPE_QS && checkInputParam(m_SqlString))
        {
#ifdef _WIN64
            m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &MaxRowsetSize, SQL_C_UBIGINT, NULL);
#else
            m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &MaxRowsetSize, SQL_IS_UINTEGER, NULL);
#endif

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
        m_SrvrCallContext.u.sendSQLcommandParams.sqlString = m_SqlString.c_str();
        m_SrvrCallContext.u.sendSQLcommandParams.stmtLabel = m_StmtLabel;
        m_SrvrCallContext.u.sendSQLcommandParams.stmtName = m_StmtName;
        m_SrvrCallContext.u.sendSQLcommandParams.cursorName = m_CursorName;
        m_SrvrCallContext.u.sendSQLcommandParams.moduleName = NULL;
        m_SrvrCallContext.u.sendSQLcommandParams.sqlStmtType = m_StmtType;
        m_SrvrCallContext.u.sendSQLcommandParams.asyncEnable = m_AsyncEnable;
        m_SrvrCallContext.u.sendSQLcommandParams.holdableCursor = m_CursorHoldable;
        m_SrvrCallContext.maxRowsetSize = MaxRowsetSize;
    }
    if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
    {
        if ((m_AsyncThread = (HANDLE)_beginthreadex(NULL, 0, ThreadControlProc,
                        &m_SrvrCallContext, CREATE_SUSPENDED, &m_ThreadAddr)) == 0)
        {
            setNTError(m_ConnectHandle->getErrorMsgLang(), "SendPrepare - _beginthreadex()");
            rc = SQL_ERROR;
        }
        ResumeThread(m_AsyncThread);
        rc = SQL_STILL_EXECUTING;
        Sleep(0);

    }
    else
    {
        if ((m_SyncThread = (HANDLE)_beginthreadex(NULL, 0, ThreadControlProc,
                        &m_SrvrCallContext, CREATE_SUSPENDED, &m_ThreadAddr)) == 0)
        {
            setNTError(m_ConnectHandle->getErrorMsgLang(), "SendPrepare - _beginthreadex()");
            rc = SQL_ERROR;
        }
        else
        {
            ResumeThread(m_SyncThread);
            WaitForSingleObject(m_SyncThread,INFINITE);
            GetExitCodeThread(m_SyncThread, &m_ThreadStatus);
            rc = m_ThreadStatus;
            CloseHandle(m_SyncThread);
            m_SyncThread = NULL;

            //			rc = ThreadControlProc(&m_SrvrCallContext);
            if (m_CurrentOdbcAPI == SQL_API_SQLEXECDIRECT && m_SrvrCallContext.odbcAPI == SQL_API_SQLPREPARE)
            {
                if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
                    rc = SendExecute(FALSE);
                else if (m_StmtType == TYPE_INSERT_PARAM)
                {
#ifdef _WIN64
                    m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &AppArrayStatusSize, SQL_C_UBIGINT, NULL);
#else
                    m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &AppArrayStatusSize, SQL_IS_UINTEGER, NULL);
#endif
                    m_ImpParamDesc->setDescArrayStatus(AppArrayStatusSize,SQL_PARAM_ERROR);
                }
            }
        }
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

    if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
    {
        if (m_AsyncThread != NULL)
        {
            if (GetExitCodeThread(m_AsyncThread, &m_ThreadStatus))
            {
                if (m_ThreadStatus == STILL_ACTIVE)
                    rc = SQL_STILL_EXECUTING;
                else
                {
                    CloseHandle(m_AsyncThread);
                    m_AsyncThread = NULL;
                    if (m_AsyncCanceled == TRUE)
                        rc = SQL_ERROR;
                    else
                        rc = m_ThreadStatus;
                }
            }
            else
            {
                CloseHandle(m_AsyncThread);
                m_AsyncThread = NULL;
                setNTError(m_ConnectHandle->getErrorMsgLang(), "SQLPrepare - GetExitCodeThread()");
                rc = SQL_ERROR;
            }
        }
        else
        {
            if (m_ThreadStatus == SQL_STILL_EXECUTING)
                SkipProcess = TRUE;
            rc = SendSQLCommand(SkipProcess, StatementText, TextLength);
        }
    }
    else
        rc = SendSQLCommand(SkipProcess, StatementText, TextLength);
    switch (rc)
    {
        case SQL_SUCCESS:
        case SQL_SUCCESS_WITH_INFO:
            if (m_StmtType == TYPE_SELECT)
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
                m_AsyncCanceled = FALSE;
                setDiagRec(DRIVER_ERROR, IDS_S1_008);
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

    if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
    {
        if (m_AsyncThread != NULL)
        {
            if (GetExitCodeThread(m_AsyncThread, &m_ThreadStatus))
            {
                if (m_ThreadStatus == STILL_ACTIVE)
                    rc = SQL_STILL_EXECUTING;
                else
                {
                    CloseHandle(m_AsyncThread);
                    m_AsyncThread = NULL;
                    if (m_AsyncCanceled == TRUE)
                        rc = SQL_ERROR;
                    else
                    {
                        if (m_ThreadStatus == SQL_SUCCESS || m_ThreadStatus == SQL_SUCCESS_WITH_INFO)
                        {
                            if (m_SrvrCallContext.odbcAPI == SQL_API_SQLPREPARE)
                                rc = SendExecute(FALSE);
                        }
                        rc = m_ThreadStatus;
                    }
                }
                //return rc;
            }
            else
            {
                CloseHandle(m_AsyncThread);
                m_AsyncThread = NULL;
                setNTError(m_ConnectHandle->getErrorMsgLang(), "SQLPrepare - GetExitCodeThread()");
                rc = SQL_ERROR;
            }
        }
        else
        {
            if (m_ThreadStatus == SQL_STILL_EXECUTING)
            {
                if (m_SrvrCallContext.odbcAPI == SQL_API_SQLPREPARE)
                    rc = SendSQLCommand(TRUE, StatementText, TextLength);
                else
                    rc = SendExecute(TRUE);
            }
            else
                rc = SendSQLCommand(FALSE, StatementText, TextLength);
        }
    }
    else
        rc = SendSQLCommand(FALSE, StatementText, TextLength);

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
            if (m_AsyncCanceled)
            {
                m_AsyncCanceled = FALSE;
                setDiagRec(DRIVER_ERROR, IDS_S1_008);
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
    return retCode;
}

SQLRETURN CStmt::setStmtData(BYTE *&inputParams, BYTE *&outputColumns)
{
    unsigned long retCode = SQL_SUCCESS;
    long tmpLen = 0;

    if (inputParams != NULL)
    {
        tmpLen = *(long *)(inputParams);

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
        tmpLen = *(long *)(outputColumns);
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
        SQLWCHAR *ColumnName,
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

SQLRETURN CStmt::SendGetSQLCatalogsArgsHlpr(bool WCharArg, SQLCHAR* arg, int argLen, char * dest, int destLen, SQLUINTEGER metaDataId)
{
    SQLRETURN	rc = SQL_SUCCESS;
    int	 translen = 0;
    char transError[128];
    BOOL isCase = FALSE;

    if(argLen > 0)
    {
        if (!WCharArg)  // DriverLocale to UTF8 translation here
        {
            if ((arg[0] == '"') && (arg[argLen-1] == '"'))
                isCase = TRUE;

            if (TranslateUTF8(FALSE, (char*)arg, argLen, dest, destLen, &translen, transError) != SQL_SUCCESS)
            {
                setDiagRec(DRIVER_ERROR, IDS_193_DRVTODS_ERROR, 0, transError);
                return SQL_ERROR;
            }
        }
        else //WChar
        {
            if (*(wchar_t *)arg == L'"' && *((wchar_t *)arg + (argLen/2)-1) == L'"')
                isCase = TRUE;

            if(WCharToUTF8((wchar_t *)arg, argLen/2, dest, destLen, &translen, transError) != SQL_SUCCESS)
            {
                setDiagRec(DRIVER_ERROR, IDS_193_DRVTODS_ERROR, 0, transError);
                return SQL_ERROR;
            }
        }
    }
    else if(dest != NULL)
        *dest = '\0';
    if(!isCase && metaDataId)
        _strupr(dest);
    return rc;
}

SQLRETURN CStmt::SendGetSQLCatalogs(short odbcAPI,
        BOOL SkipProcess,
        SQLWCHAR *CatalogNameW,
        SQLSMALLINT NameLength1,
        SQLWCHAR *SchemaNameW,
        SQLSMALLINT NameLength2,
        SQLWCHAR *TableNameW,
        SQLSMALLINT NameLength3,
        SQLWCHAR *ColumnNameW,
        SQLSMALLINT NameLength4,
        SQLWCHAR *TableTypeW,
        SQLSMALLINT NameLength5,
        SQLUSMALLINT IdentifierType,
        SQLUSMALLINT Scope,
        SQLUSMALLINT Nullable,
        SQLSMALLINT SqlType,
        SQLUSMALLINT Unique,
        SQLUSMALLINT Reserved,
        SQLWCHAR *FKCatalogNameW,
        SQLSMALLINT NameLength6,
        SQLWCHAR *FKSchemaNameW,
        SQLSMALLINT NameLength7,
        SQLWCHAR *FKTableNameW,
        SQLSMALLINT NameLength8)
{
    SQLRETURN			rc;
    char				*cEmpty = "";
    char				*cAll = 0;
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
    SQLCHAR				*FKsubstCatalogName;
    SQLSMALLINT			FKcatalogNameLen;
    SQLCHAR				*FKsubstSchemaName;
    SQLSMALLINT			FKschemaNameLen;
    SQLCHAR				*FKsubstTableName;
    SQLSMALLINT			FKtableNameLen;
    bool				IsCase = FALSE;
    bool				isWCharData = false;

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
            case SQL_WLONGVARCHAR:
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
        // (any characters) are equivalent. However, a zero-length search patternthat is,
        // a valid pointer to a string of length zeromatches only the empty string (""). - ODBC Manual

        if (CatalogNameW == NULL)
        {
            //substCatalogName = (SQLCHAR *)SQL_ALL_CATALOGS;
            substCatalogName = (SQLCHAR *)getCurrentCatalog();
            catalogNameLen = strlen((const char *)substCatalogName);
            isWCharData = false;
        }
        else
        {
            substCatalogName = (SQLCHAR *)CatalogNameW;
            if(NameLength1 == SQL_NTS)
                catalogNameLen = wcslen(CatalogNameW) * 2;
            else
                catalogNameLen = NameLength1 * 2;
            isWCharData = true;
        }

        if (SendGetSQLCatalogsArgsHlpr(isWCharData,
                    substCatalogName,
                    catalogNameLen,
                    m_CatalogName,
                    sizeof(m_CatalogName), m_MetadataId) != SQL_SUCCESS)
            return SQL_ERROR;

        if (SchemaNameW == NULL)
        {
            if(gDrvrGlobal.RestrictSchema)
            {
                substSchemaName = (SQLCHAR *)getCurrentSchema();
                if(gDrvrGlobal.noSchemaInDSN) //but no schema was specified in DSN
                    substSchemaName = (SQLCHAR *)SQL_ALL_SCHEMAS;
            }
            else
                substSchemaName = (SQLCHAR *)SQL_ALL_SCHEMAS;
            schemaNameLen = strlen((const char *)substSchemaName);
            isWCharData = false;
        }
        else
        {
            substSchemaName = (SQLCHAR *)SchemaNameW;
            if(NameLength2 == SQL_NTS)
                schemaNameLen = wcslen(SchemaNameW) * 2;
            else
                schemaNameLen = NameLength2 * 2;
            isWCharData = true;
        }

        if (SendGetSQLCatalogsArgsHlpr(isWCharData,
                    substSchemaName,
                    schemaNameLen,
                    m_SchemaName,
                    sizeof(m_SchemaName),m_MetadataId) != SQL_SUCCESS)
            return SQL_ERROR;

        if (TableNameW == NULL)
        {
            if (odbcAPI != SQL_API_SQLFOREIGNKEYS)
                substTableName = (SQLCHAR *)cAll;
            else
                substTableName = (SQLCHAR *)cEmpty;
            if(substTableName != NULL)
                tableNameLen = strlen((const char *)substTableName);
            else
                tableNameLen = 0;
            isWCharData = false;
        }
        else
        {
            substTableName = (SQLCHAR *)TableNameW;
            if(NameLength3 == SQL_NTS)
                tableNameLen = wcslen(TableNameW) * 2;
            else
                tableNameLen = NameLength3 * 2;
            isWCharData = true;
        }
        if (SendGetSQLCatalogsArgsHlpr(isWCharData,
                    substTableName,
                    tableNameLen,
                    m_TableName,
                    sizeof(m_TableName),m_MetadataId) != SQL_SUCCESS)
            return SQL_ERROR;

        if (ColumnNameW == NULL)
        {
            substColumnName = (SQLCHAR *)cAll;
            if(substColumnName != NULL)
                columnNameLen = strlen((const char *)substColumnName);
            else
                columnNameLen = 0;
            isWCharData = false;
        }
        else
        {
            substColumnName = (SQLCHAR *)ColumnNameW;
            if(NameLength4 ==  SQL_NTS)
                columnNameLen = wcslen(ColumnNameW) * 2;
            else
                columnNameLen = NameLength4 * 2;
            isWCharData = true;
        }
        if (SendGetSQLCatalogsArgsHlpr(isWCharData,
                    substColumnName,
                    columnNameLen,
                    m_ColumnName,
                    sizeof(m_ColumnName),m_MetadataId) != SQL_SUCCESS)
            return SQL_ERROR;

        if (TableTypeW == NULL)
        {
            substTableType = (SQLCHAR *)SQL_ALL_TABLE_TYPES;
            tableTypeLen = strlen((const char *)substTableType);
            isWCharData = false;
        }
        else
        {
            substTableType = (SQLCHAR *)TableTypeW;
            if(NameLength5 == SQL_NTS)
                tableTypeLen = wcslen(TableTypeW) * 2;
            else
                tableTypeLen = NameLength5 * 2;
            isWCharData = true;
        }
        if (SendGetSQLCatalogsArgsHlpr(isWCharData,
                    substTableType,
                    tableTypeLen,
                    m_TableType,
                    sizeof(m_TableType),m_MetadataId) != SQL_SUCCESS)
            return SQL_ERROR;

        //  Added to support SQLForeignKeys
        if (FKCatalogNameW == NULL)
        {
            if (isNskVersion())
                FKsubstCatalogName = (SQLCHAR *)getCurrentCatalog();
            else
                FKsubstCatalogName = (SQLCHAR *)SQL_ALL_CATALOGS;
            FKcatalogNameLen = strlen((const char *)FKsubstCatalogName);
            isWCharData = false;
        }
        else
        {
            FKsubstCatalogName = (SQLCHAR *)FKCatalogNameW;
            if(NameLength6 == SQL_NTS)
                FKcatalogNameLen = wcslen(FKCatalogNameW) * 2;
            else
                FKcatalogNameLen = NameLength6 * 2;
            isWCharData = true;
        }
        if (SendGetSQLCatalogsArgsHlpr(isWCharData,
                    FKsubstCatalogName,
                    FKcatalogNameLen,
                    m_FKCatalogName,
                    sizeof(m_FKCatalogName),m_MetadataId)!= SQL_SUCCESS)
            return SQL_ERROR;

        if (FKSchemaNameW == NULL)
        {
            if (isNskVersion())
                FKsubstSchemaName = (SQLCHAR *)getCurrentSchema();
            else
                FKsubstSchemaName = (SQLCHAR *)SQL_ALL_SCHEMAS;
            FKschemaNameLen = strlen((const char *)FKsubstSchemaName);
            isWCharData = false;
        }
        else
        {
            FKsubstSchemaName = (SQLCHAR *)FKSchemaNameW;
            if(NameLength7 ==  SQL_NTS)
                FKschemaNameLen = wcslen(FKSchemaNameW) * 2;
            else
                FKschemaNameLen = NameLength7 * 2;
            isWCharData = true;
        }
        if (SendGetSQLCatalogsArgsHlpr(isWCharData,
                    FKsubstSchemaName,
                    FKschemaNameLen,
                    m_FKSchemaName,
                    sizeof(m_FKSchemaName),m_MetadataId)!= SQL_SUCCESS)
            return SQL_ERROR;

        if (FKTableNameW == NULL)
        {
            FKsubstTableName = (SQLCHAR *)cEmpty;
            FKtableNameLen = strlen((const char *)FKsubstTableName);
            isWCharData = false;
        }
        else
        {
            FKsubstTableName = (SQLCHAR *)FKTableNameW;
            if(NameLength8 == SQL_NTS)
                FKtableNameLen = wcslen(FKTableNameW) * 2;
            else
                FKtableNameLen = NameLength8 * 2;
            isWCharData = true;
        }
        if (SendGetSQLCatalogsArgsHlpr(isWCharData,
                    FKsubstTableName,
                    FKtableNameLen,
                    m_FKTableName,
                    sizeof(m_FKTableName),m_MetadataId)!= SQL_SUCCESS)
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
        m_SrvrCallContext.statementTimeout = m_QueryTimeout;
        m_StmtType = TYPE_SELECT;			// Since catalog APIs produce result set

    }
    if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
    {
        if ((m_AsyncThread = (HANDLE)_beginthreadex(NULL, 0, ThreadControlProc,
                        &m_SrvrCallContext, CREATE_SUSPENDED, &m_ThreadAddr)) == 0)
        {
            setNTError(m_ConnectHandle->getErrorMsgLang(), "SendGetSQLCatalogs - _beginthreadex()");
            rc = SQL_ERROR;
        }
        ResumeThread(m_AsyncThread);
        rc = SQL_STILL_EXECUTING;
        Sleep(0);
    }
    else
    {
        if ((m_SyncThread = (HANDLE)_beginthreadex(NULL, 0, ThreadControlProc,
                        &m_SrvrCallContext, CREATE_SUSPENDED, &m_ThreadAddr)) == 0)
        {
            setNTError(m_ConnectHandle->getErrorMsgLang(), "SendPrepare - _beginthreadex()");
            rc = SQL_ERROR;
        }
        else
        {
            ResumeThread(m_SyncThread);
            WaitForSingleObject(m_SyncThread,INFINITE);
            GetExitCodeThread(m_SyncThread, &m_ThreadStatus);
            rc = m_ThreadStatus;
            CloseHandle(m_SyncThread);
            m_SyncThread = NULL;
        }
        //		rc = ThreadControlProc(&m_SrvrCallContext);
    }
    return rc;
}

SQLRETURN CStmt::GetSQLCatalogs(short odbcAPI,
        SQLWCHAR *CatalogNameW,
        SQLSMALLINT NameLength1,
        SQLWCHAR *SchemaNameW,
        SQLSMALLINT NameLength2,
        SQLWCHAR *TableNameW,
        SQLSMALLINT NameLength3,
        SQLWCHAR *ColumnNameW,
        SQLSMALLINT NameLength4,
        SQLWCHAR *TableTypeW,
        SQLSMALLINT NameLength5,
        SQLUSMALLINT IdentifierType,
        SQLUSMALLINT Scope,
        SQLUSMALLINT Nullable,
        SQLSMALLINT SqlType,
        SQLUSMALLINT Unique,
        SQLUSMALLINT Reserved,
        SQLWCHAR *FKCatalogNameW,
        SQLSMALLINT NameLength6,
        SQLWCHAR *FKSchemaNameW,
        SQLSMALLINT NameLength7,
        SQLWCHAR *FKTableNameW,
        SQLSMALLINT NameLength8)
{
    SQLRETURN	rc;
    BOOL		SkipProcess = FALSE;

    m_CurrentOdbcAPI = odbcAPI;
    if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
    {
        if (m_AsyncThread != NULL)
        {
            if (GetExitCodeThread(m_AsyncThread, &m_ThreadStatus))
            {
                if (m_ThreadStatus == STILL_ACTIVE)
                    rc = SQL_STILL_EXECUTING;
                else
                {
                    CloseHandle(m_AsyncThread);
                    m_AsyncThread = NULL;
                    if (m_AsyncCanceled == TRUE)
                        rc = SQL_ERROR;
                    else
                        rc = m_ThreadStatus;
                }
            }
            else
            {
                CloseHandle(m_AsyncThread);
                m_AsyncThread = NULL;
                setNTError(m_ConnectHandle->getErrorMsgLang(), "GetSQLCatalogs - GetExitCodeThread()");
                rc = SQL_ERROR;
            }
        }
        else
        {
            if (m_ThreadStatus == SQL_STILL_EXECUTING)
                SkipProcess = TRUE;
            rc = SendGetSQLCatalogs(odbcAPI, SkipProcess, CatalogNameW, NameLength1, SchemaNameW,
                    NameLength2, TableNameW, NameLength3, ColumnNameW, NameLength4,
                    TableTypeW, NameLength5, IdentifierType, Scope, Nullable, SqlType, Unique, Reserved,
                    FKCatalogNameW, NameLength6, FKSchemaNameW, NameLength7, FKTableNameW, NameLength8);
        }
    }
    else
        rc = SendGetSQLCatalogs(odbcAPI, SkipProcess, CatalogNameW, NameLength1, SchemaNameW,
                NameLength2, TableNameW, NameLength3, ColumnNameW, NameLength4,
                TableTypeW, NameLength5, IdentifierType, Scope, Nullable, SqlType, Unique, Reserved,
                FKCatalogNameW, NameLength6, FKSchemaNameW, NameLength7, FKTableNameW, NameLength8);
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
                m_AsyncCanceled = FALSE;
                setDiagRec(DRIVER_ERROR, IDS_S1_008);
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

SQLRETURN  CStmt::GetCursorName(SQLWCHAR *CursorNameW,
        SQLSMALLINT BufferLength,
        SQLSMALLINT *NameLengthPtr)
{

    SQLRETURN	rc = SQL_SUCCESS;
    short		strLen = 0;
    int			translen = 0;
    char		transError[MAX_TRANSLATE_ERROR_MSG_LEN];
    unsigned long	retCode;

    clearError();
    m_CurrentOdbcAPI = SQL_API_SQLGETCURSORNAME;
    if(m_CursorName[0] != '\0')
        strLen = strlen((const char *)m_CursorName);
    else
        strLen = strlen((const char *)m_StmtLabel);
    if (CursorNameW != NULL)
    {
        if(m_CursorName[0] != '\0')
        {
            // translate from UTF8 to WCHAR
            rc = UTF8ToWChar((char*)m_CursorName, strLen, (wchar_t *)CursorNameW, BufferLength, &translen, transError);
        }
        else
        {
            // translate from UTF8 to WCHAR
            rc = UTF8ToWChar((char*)m_StmtLabel, strLen, (wchar_t *)CursorNameW, BufferLength, &translen, transError);
        }
        if (rc == SQL_SUCCESS)
            strLen = translen;
        else if (rc == SQL_SUCCESS_WITH_INFO)
            setDiagRec(DRIVER_ERROR, IDS_01_004);
    }

    if (NameLengthPtr != NULL)
        *NameLengthPtr = strLen*2;
    return rc ;
}


SQLRETURN CStmt::SetCursorName(SQLWCHAR *CursorNameW,
        SQLSMALLINT NameLength)
{
    SQLRETURN	rc = SQL_SUCCESS;
    int			translen = 0;
    char		transError[MAX_TRANSLATE_ERROR_MSG_LEN];
    CHANDLECOLLECT::iterator i;

    clearError();

    m_CurrentOdbcAPI = SQL_API_SQLSETCURSORNAME;
    if(NameLength == SQL_NTS)
        NameLength = wcslen(CursorNameW);

    NameLength = min(NameLength, 128); // max cursorname length is 128

    // translate from WCHAR to UTF8
    rc = WCharToUTF8((wchar_t*)CursorNameW, NameLength, (char*)m_CursorName, sizeof(m_CursorName), &translen, transError);
    if (rc == SQL_SUCCESS_WITH_INFO)
        setDiagRec(DRIVER_ERROR, IDS_01_004);

    if ((strncmp(m_CursorName, "SQLCUR", 6) == 0) || (strncmp(m_CursorName, "SQL_CUR", 7) == 0))
    {
        setDiagRec(DRIVER_ERROR, IDS_34_000);
        return SQL_ERROR;
    }

    for (i = m_ConnectHandle->m_StmtCollect.begin() ; i !=  m_ConnectHandle->m_StmtCollect.end() ; ++i)
    {
        CStmt*	pStmt = ((CStmt *)(*i));
        if ((strncmp(pStmt->m_CursorName, m_CursorName, strlen(m_CursorName)) == 0) && (pStmt != this))
        {
            setDiagRec(DRIVER_ERROR, IDS_3C_000);
            return SQL_ERROR;
        }
    }
    return rc;
}

SQLRETURN CStmt::Cancel()
{
    SQLRETURN rc = SQL_SUCCESS;
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

    if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
    {
        if (m_AsyncThread != NULL)
        {
            if (GetExitCodeThread(m_AsyncThread, &m_ThreadStatus))
            {
                if (m_ThreadStatus == STILL_ACTIVE )
                {
                    if(m_ConnectHandle->m_IgnoreCancel == false)
                    {
                        m_AsyncCanceled = TRUE;
                        sendStopServer();
                        rc = SQL_SUCCESS;
                    }
                    else
                    {
                        m_CancelCalled = true;
                        setDiagRec(DRIVER_ERROR, IDS_HY_018);
                        rc = SQL_ERROR;
                    }
                }
                else if (getODBCAppVersion()== SQL_OV_ODBC2)
                {
                    m_AsyncEnable = SQL_ASYNC_ENABLE_OFF;
                    rc = Close(SQL_CLOSE);
                    m_AsyncEnable = SQL_ASYNC_ENABLE_ON;
                }
            }
            else
            {
                CloseHandle(m_AsyncThread);
                m_AsyncThread = NULL;
                setNTError(m_ConnectHandle->getErrorMsgLang(), "Cancel - GetExitCodeThread()");
                return SQL_ERROR;
            }
        }
    }
    else if (m_SyncThread != NULL)
    {
        if (GetExitCodeThread(m_SyncThread, &m_ThreadStatus))
        {
            if (m_ThreadStatus == STILL_ACTIVE)
            {
                if(m_ConnectHandle->m_IgnoreCancel == false)
                {

                    m_AsyncCanceled = TRUE;
                    sendStopServer();
                    rc = SQL_SUCCESS;
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
        else
        {
            CloseHandle(m_SyncThread);
            m_SyncThread = NULL;
            setNTError(m_ConnectHandle->getErrorMsgLang(), "Cancel - GetExitCodeThread()");
            return SQL_ERROR;
        }
    }

    return rc;
}

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
        case SQL_DESC_DISPLAY_SIZE:
        case SQL_DESC_NUM_PREC_RADIX:
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
        case SQL_DESC_CONCISE_TYPE:
        case SQL_DESC_COUNT:
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
    if (DataType == SQL_IS_SMALLINT)
    {
        if (NumericAttributePtr != NULL)
            *(SQLSMALLINT *)NumericAttributePtr = (SQLSMALLINT)sTmp;
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
    SQLSMALLINT		ParamNumber;
    SQLULEN			RowNumber;
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
        if(RowNumber>0)
        {
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
    if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
    {
        if ((m_AsyncThread = (HANDLE)_beginthreadex(NULL, 0, ThreadControlProc,
                        &m_SrvrCallContext, CREATE_SUSPENDED, &m_ThreadAddr)) == 0)
        {
            setNTError(m_ConnectHandle->getErrorMsgLang(), "SendExecute - _beginthreadex()");
            rc = SQL_ERROR;
        }
        ResumeThread(m_AsyncThread);
        rc = SQL_STILL_EXECUTING;
        Sleep(0);
    }
    else
    {
        if ((m_SyncThread = (HANDLE)_beginthreadex(NULL, 0, ThreadControlProc,
                        &m_SrvrCallContext, CREATE_SUSPENDED, &m_ThreadAddr)) == 0)
        {
            setNTError(m_ConnectHandle->getErrorMsgLang(), "SendPrepare - _beginthreadex()");
            rc = SQL_ERROR;
        }
        else
        {
            ResumeThread(m_SyncThread);
            WaitForSingleObject(m_SyncThread,INFINITE);
            GetExitCodeThread(m_SyncThread, &m_ThreadStatus);
            rc = m_ThreadStatus;
            CloseHandle(m_SyncThread);
            m_SyncThread = NULL;
        }
        //		rc = ThreadControlProc(&m_SrvrCallContext);
    }

    if(m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
    {
        if(rctrack == SQL_SUCCESS_WITH_INFO)
            m_WarningSetBeforeAsync = true;
        else
            m_WarningSetBeforeAsync = false;
    }
    else if (rctrack == SQL_SUCCESS_WITH_INFO  && rc != SQL_ERROR)
        rc = rctrack;

    return rc;
}

SQLRETURN CStmt::Execute()
{
    SQLRETURN	rc;
    BOOL		SkipProcess = FALSE;

    m_CurrentOdbcAPI = SQL_API_SQLEXECUTE;
    if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
    {
        if (m_AsyncThread != NULL)
        {
            if (GetExitCodeThread(m_AsyncThread, &m_ThreadStatus))
            {
                if (m_ThreadStatus == STILL_ACTIVE)
                    rc = SQL_STILL_EXECUTING;
                else
                {
                    CloseHandle(m_AsyncThread);
                    m_AsyncThread = NULL;
                    if (m_AsyncCanceled == TRUE)
                        rc = SQL_ERROR;
                    else
                    {
                        rc = m_ThreadStatus;
                        if(rc == SQL_SUCCESS && m_WarningSetBeforeAsync)
                            rc = SQL_SUCCESS_WITH_INFO;
                    }
                }
            }
            else
            {
                CloseHandle(m_AsyncThread);
                m_AsyncThread = NULL;
                setNTError(m_ConnectHandle->getErrorMsgLang(), "Execute - GetExitCodeThread()");
                rc = SQL_ERROR;
            }
        }
        else
        {
            if (m_ThreadStatus == SQL_STILL_EXECUTING)
                SkipProcess = TRUE;
            rc = SendExecute(SkipProcess);

        }
    }
    else
        rc = SendExecute(SkipProcess);
    switch (rc)
    {
        case SQL_SUCCESS:
        case SQL_SUCCESS_WITH_INFO:
            if (m_StmtType == TYPE_SELECT || m_StmtType == TYPE_CALL)
                m_StmtState = STMT_EXECUTED;
            else
                m_StmtState = STMT_EXECUTED_NO_RESULT;
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
                m_AsyncCanceled = FALSE;
                setDiagRec(DRIVER_ERROR, IDS_S1_008);
            }
            if (m_StmtType == TYPE_SELECT)
                m_StmtState = STMT_PREPARED;
            else
                m_StmtState = STMT_PREPARED_NO_RESULT;
            setRowCount(-1);
            break;
        case SQL_NO_DATA:
            if (m_StmtQueryType == SQL_SELECT_UNIQUE)
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
    SQLULEN			RowNumber;
    SQLULEN			AppArrayStatusSize;
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
    if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
    {
        if ((m_AsyncThread = (HANDLE)_beginthreadex(NULL, 0, ThreadControlProc,
                        &m_SrvrCallContext, CREATE_SUSPENDED, &m_ThreadAddr)) == 0)
        {
            setNTError(m_ConnectHandle->getErrorMsgLang(), "SendParamData - _beginthreadex()");
            rc = SQL_ERROR;
        }
        ResumeThread(m_AsyncThread);
        rc = SQL_STILL_EXECUTING;
        Sleep(0);
    }
    else
    {
        if ((m_SyncThread = (HANDLE)_beginthreadex(NULL, 0, ThreadControlProc,
                        &m_SrvrCallContext, CREATE_SUSPENDED, &m_ThreadAddr)) == 0)
        {
            setNTError(m_ConnectHandle->getErrorMsgLang(), "SendPrepare - _beginthreadex()");
            rc = SQL_ERROR;
        }
        else
        {
            ResumeThread(m_SyncThread);
            WaitForSingleObject(m_SyncThread,INFINITE);
            GetExitCodeThread(m_SyncThread, &m_ThreadStatus);
            rc = m_ThreadStatus;
            CloseHandle(m_SyncThread);
            m_SyncThread = NULL;
        }
        //		rc = ThreadControlProc(&m_SrvrCallContext);
    }
    if (rc == SQL_ERROR)
    {
#ifdef _WIN64
        m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &AppArrayStatusSize, SQL_C_UBIGINT, NULL);
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

    //insert >16m lob
    if (m_CurrentOdbcAPI == SRVR_API_UPDATELOB)
    {
        if (m_ConnectHandle->lobHandleLenSave == 0)
            return SQL_ERROR;

        if (m_StmtState == STMT_PARAM_DATA_NOT_CALLED)
        {
            m_CurrentParamStatus = SQL_WAITING_FOR_DATA;
            return SQL_NEED_DATA;
        }
        else
        {
            rc = UpdateLob(0, m_ConnectHandle->lobHandleSave, m_ConnectHandle->lobHandleLenSave, 0, 0, 0, m_DataAtExecData.dataValue._length, (BYTE *)m_DataAtExecData.dataValue._buffer);
            m_ConnectHandle->lobHandleLenSave == 0;
            free(m_ConnectHandle->lobHandleSave);
            m_ConnectHandle->lobHandleSave = NULL;
            return rc;
        }

    }

    m_CurrentOdbcAPI = SQL_API_SQLPARAMDATA;
    if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
    {
        if (m_AsyncThread != NULL)
        {
            if (GetExitCodeThread(m_AsyncThread, &m_ThreadStatus))
            {
                if (m_ThreadStatus == STILL_ACTIVE)
                    rc = SQL_STILL_EXECUTING;
                else
                {
                    CloseHandle(m_AsyncThread);
                    m_AsyncThread = NULL;
                    if (m_AsyncCanceled == TRUE)
                        rc = SQL_ERROR;
                    else
                        rc = m_ThreadStatus;
                }
            }
            else
            {
                CloseHandle(m_AsyncThread);
                m_AsyncThread = NULL;
                setNTError(m_ConnectHandle->getErrorMsgLang(), "Execute - GetExitCodeThread()");
                rc = SQL_ERROR;
            }
        }
        else
        {
            if (m_ThreadStatus == SQL_STILL_EXECUTING)
                SkipProcess = TRUE;
            rc = SendParamData(SkipProcess, ValuePtrPtr);

        }
    }
    else
        rc = SendParamData(SkipProcess, ValuePtrPtr);
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
                m_AsyncCanceled = FALSE;
                setDiagRec(DRIVER_ERROR, IDS_S1_008);
            }
            revertStmtState();
            break;
        case SQL_NO_DATA:
            if (m_StmtQueryType == SQL_SELECT_UNIQUE)
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
    SQLLEN			*StrLenPtr;
    SQLINTEGER		StrLen;
    SQLSMALLINT		ParameterType;

    if (m_CurrentOdbcAPI == SRVR_API_UPDATELOB)
    {
        if (m_CurrentParamStatus == SQL_WAITING_FOR_DATA)
        {
            if (m_DataAtExecData.dataValue._buffer != NULL)
            {
                delete m_DataAtExecData.dataValue._buffer;
                m_DataAtExecData.dataValue._buffer = NULL;
                m_DataAtExecData.dataValue._length = 0;
            }
            m_CurrentParamStatus = SQL_RECEIVING_DATA;
        }
        else
        {
            OctetLength = m_DataAtExecData.dataValue._length;
        }

        if (StrLen_or_Ind == SQL_NTS)
            StrLen = strlen((const char *)DataPtr);
        else
            StrLen = StrLen_or_Ind;

        OctetLength += StrLen;
        AllocDataPtr = new BYTE[OctetLength];

        if (m_DataAtExecData.dataValue._buffer != NULL)
        {
            memcpy((unsigned char *)AllocDataPtr, (unsigned char*)m_DataAtExecData.dataValue._buffer, m_DataAtExecData.dataValue._length);
            delete m_DataAtExecData.dataValue._buffer;
            m_DataAtExecData.dataValue._buffer = NULL;
        }

        m_DataAtExecData.dataValue._buffer = (unsigned char *)AllocDataPtr;
        memcpy(m_DataAtExecData.dataValue._buffer + m_DataAtExecData.dataValue._length, DataPtr, StrLen);

        m_DataAtExecData.dataValue._length = OctetLength;
        m_StmtState = STMT_PUT_DATA_CALLED;

        return rc;
    }

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
            if ((retCode = m_ImpParamDesc->GetParamSQLInfo(m_CurrentRow, m_CurrentParam, ParameterType,
                            ODBCDataType)) != SQL_SUCCESS)
            {
                setDiagRec(DRIVER_ERROR, retCode);
                rc = SQL_ERROR;
                goto updateStmtState;
            }
            if (DataType == SQL_C_DEFAULT)
            {
                if ((retCode = getCDefault(ParameterType, getODBCAppVersion(), DataType)) != SQL_SUCCESS)
                {
                    setDiagRec(DRIVER_ERROR, retCode);
                    rc = SQL_ERROR;
                    goto updateStmtState;
                }
            }
            m_DataAtExecData.dataType = DataType;
            m_DataAtExecData.dataCharset = 0;
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
                    if (DataType == SQL_C_WCHAR)
                    {
                        StrLen = wcslen((const wchar_t *)DataPtr);
                        StrLen = StrLen * 2;
                    }
                    else
                        StrLen = strlen((const char *)DataPtr);		// Note No Break
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
                            if(m_DataAtExecData.dataType == SQL_C_WCHAR)
                            {
                                StrLen = wcslen((const wchar_t *)DataPtr);
                                StrLen = StrLen * 2;
                            }
                            else
                                StrLen = strlen((const char *)DataPtr);
                        default:
                            if (StrLen < 0)
                            {
                                setDiagRec(DRIVER_ERROR, IDS_HY_090);
                                rc = SQL_ERROR;
                                goto updateStmtState;
                            }
                            if(m_DataAtExecData.dataType == SQL_C_WCHAR)
                            {
                                if (m_DataAtExecData.dataValue._length + StrLen > m_DataAtExecDataBufferSize)
                                {
                                    setDiagRec(DRIVER_ERROR, IDS_22_001);
                                    rc = SQL_ERROR;
                                    goto updateStmtState;
                                }
                            }
                            else
                            {
                                if (m_DataAtExecData.dataValue._length + StrLen > m_DataAtExecDataBufferSize)
                                {
                                    setDiagRec(DRIVER_ERROR, IDS_22_001);
                                    rc = SQL_ERROR;
                                    goto updateStmtState;
                                }
                            }
                            memcpy(m_DataAtExecData.dataValue._buffer + m_DataAtExecData.dataValue._length,	DataPtr, StrLen);
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

BOOL CStmt::setFetchOutputPerf(long rowsFetched, SQL_DataValue_def*& outputDataValue)
{
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
        delete m_outputDataValue._buffer;
        m_outputDataValue._buffer = NULL;
        m_outputDataValue._length = 0;
    }

    totalLength = outputDataValue->_length;

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
        m_FetchDataValue.rowAddress = new LONG_PTR[rowsFetched];
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

// This function has been overloaded to use in RWRS binding scenario
BOOL CStmt::setFetchOutputPerf(SQL_DataValue_def*& outputDataValue, long rowsFetched)
{
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
        delete m_outputDataValue._buffer;
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

    memPtr = outputDataValue->_buffer;

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
                if(SQLMaxLength>SHRT_MAX)
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
            case SQLTYPECODE_BLOB:
            case SQLTYPECODE_CLOB:
                memOffSet = ((memOffSet + 2 - 1) >> 1) << 1;
                VarOffSet = memOffSet;
                memOffSet += SQLMaxLength + 2;
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
            case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED:
                VarOffSet = memOffSet;
                memOffSet += SQLMaxLength;
                break;
            case SQLTYPECODE_DECIMAL_LARGE:
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
                //memOffSet = ((memOffSet + 8 - 1) >> 3) << 3;
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
        m_FetchDataValue.rowAddress = new LONG_PTR[rowsFetched];
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
        //memset(m_SwapInfo,'N', rowsFetched * count);
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
    SQLULEN			MaxRowCnt;
    SQLINTEGER		RecLength;

    if (m_ConnectHandle->getConnectionStatus() == SQL_CD_TRUE)
    {
        clearError();
        setDiagRec(DRIVER_ERROR, IDS_08_S01, -20005);
        return SQL_ERROR;
    }

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
#ifdef _WIN64
            m_AppRowDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &MaxRowCnt, SQL_C_SBIGINT, NULL);
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

            //  Added this code for future enabling. We can enable this for ABD, Do this does
            // is if rows fetched by SQL is less the MAXROWCNT set then we can assume it as
            // END OF DATA. Ando no need to send again FETCH call to Server. So that we can
            // avoid one message transmission. On the Server side we internally call Fetch to
            // make sure we reach END OF DATA.
            // if (m_RowsFetched < MaxRowCnt)
            //	return SQL_NO_DATA;

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
#ifdef _WIN64
            m_AppRowDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &MaxRowCnt, SQL_C_UBIGINT, NULL);
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
        this->m_preFetchThread.setSrvrCallContext(&m_SrvrCallContext);
        if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
        {
            if ((m_AsyncThread = (HANDLE)_beginthreadex(NULL, 0, ThreadControlProc,
                            &m_SrvrCallContext, CREATE_SUSPENDED, &m_ThreadAddr)) == 0)
            {
                setNTError(m_ConnectHandle->getErrorMsgLang(), "SendFetch - _beginthreadex()");
                rc = SQL_ERROR;
            }
            ResumeThread(m_AsyncThread);
            rc = SQL_STILL_EXECUTING;
            Sleep(0);
        }
        else
        {
            if ((m_SyncThread = (HANDLE)_beginthreadex(NULL, 0, ThreadControlProc,
                            &m_SrvrCallContext, CREATE_SUSPENDED, &m_ThreadAddr)) == 0)
            {
                setNTError(m_ConnectHandle->getErrorMsgLang(), "SendPrepare - _beginthreadex()");
                rc = SQL_ERROR;
            }
            else
            {
                ResumeThread(m_SyncThread);
                WaitForSingleObject(m_SyncThread,INFINITE);
                GetExitCodeThread(m_SyncThread, &m_ThreadStatus);
                rc = m_ThreadStatus;
                CloseHandle(m_SyncThread);
                m_SyncThread = NULL;
            }
            //rc = ThreadControlProc(&m_SrvrCallContext);
        }
    }
    return rc;
}

SQLRETURN CStmt::Fetch()
{
    SQLRETURN	rc, temprc;
    SQLRETURN   	rcCopyData = SQL_SUCCESS;
    BOOL		SkipProcess = FALSE;

    // Following variable and code is added because if application is 2.0 and is using
    // SQL_ROWSET_SIZE option with SQLFetch instead of SQLExtendedFetch. So we are suppose to
    // return one row at a time instead of 10 rows we fetch from server and move them
    // into application buffer. Ex: Suppose we have 20 catalogs (like CAT1, CAT2.. Till
    // CAT20) configured and application calls SQLTables for catalogs then we move
    // 2 sets of 10 row chucks when application call SQLFetch we should return ONE ROW
    // at a time NOT in set of 10 when SQLGetData or SQLBindCol is called. So set the
    // m_DescArraySize variable to 1 and set it back to original before exiting the function.
    SQLULEN		tempMaxRowCnt;
    SQLUINTEGER	setMaxRowCntToOne = 1;
    if (getODBCAppVersion() == SQL_OV_ODBC2)
    {
#ifdef _WIN64
        m_AppRowDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &tempMaxRowCnt, SQL_C_UBIGINT, NULL);
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
    if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
    {
        if (m_AsyncThread != NULL)
        {
            if (GetExitCodeThread(m_AsyncThread, &m_ThreadStatus))
            {
                if (m_ThreadStatus == STILL_ACTIVE)
                    rc = SQL_STILL_EXECUTING;
                else
                {
                    CloseHandle(m_AsyncThread);
                    m_AsyncThread = NULL;
                    if (m_AsyncCanceled == TRUE)
                        rc = SQL_ERROR;
                    else
                        rc = m_ThreadStatus;
                }
            }
            else
            {
                CloseHandle(m_AsyncThread);
                m_AsyncThread = NULL;
                setNTError(m_ConnectHandle->getErrorMsgLang(), "Fetch - GetExitCodeThread()");
                rc = SQL_ERROR;
            }
        }
        else
        {
            if (m_ThreadStatus == SQL_STILL_EXECUTING)
                SkipProcess = TRUE;
            rc = SendFetch(SkipProcess);
        }
    }
    else
        rc = SendFetch(SkipProcess);

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
                m_AsyncCanceled = FALSE;
                setDiagRec(DRIVER_ERROR, IDS_S1_008);
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
    SQLULEN		MaxRowCnt;
    SQLINTEGER		RecLength;

    if (m_ConnectHandle->getConnectionStatus() == SQL_CD_TRUE)
    {
        clearError();
        setDiagRec(DRIVER_ERROR, IDS_08_S01, -20005);
        return SQL_ERROR;
    }

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
#ifdef _WIN64
            m_AppRowDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &MaxRowCnt, SQL_C_UBIGINT, NULL);
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
        this->m_preFetchThread.setSrvrCallContext(&m_SrvrCallContext);
        if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
        {
            if ((m_AsyncThread = (HANDLE)_beginthreadex(NULL, 0, ThreadControlProc,
                            &m_SrvrCallContext, CREATE_SUSPENDED, &m_ThreadAddr)) == 0)
            {
                setNTError(m_ConnectHandle->getErrorMsgLang(), "SendFetch - _beginthreadex()");
                rc = SQL_ERROR;
            }
            ResumeThread(m_AsyncThread);
            rc = SQL_STILL_EXECUTING;
            Sleep(0);
        }
        else
        {
            if ((m_SyncThread = (HANDLE)_beginthreadex(NULL, 0, ThreadControlProc,
                            &m_SrvrCallContext, CREATE_SUSPENDED, &m_ThreadAddr)) == 0)
            {
                setNTError(m_ConnectHandle->getErrorMsgLang(), "SendPrepare - _beginthreadex()");
                rc = SQL_ERROR;
            }
            else
            {
                ResumeThread(m_SyncThread);
                WaitForSingleObject(m_SyncThread,INFINITE);
                GetExitCodeThread(m_SyncThread, &m_ThreadStatus);
                rc = m_ThreadStatus;
                CloseHandle(m_SyncThread);
                m_SyncThread = NULL;
            }
            //			rc = ThreadControlProc(&m_SrvrCallContext);
        }
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
    if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
    {
        if (m_AsyncThread != NULL)
        {
            if (GetExitCodeThread(m_AsyncThread, &m_ThreadStatus))
            {
                if (m_ThreadStatus == STILL_ACTIVE)
                    rc = SQL_STILL_EXECUTING;
                else
                {
                    CloseHandle(m_AsyncThread);
                    m_AsyncThread = NULL;
                    if (m_AsyncCanceled == TRUE)
                        rc = SQL_ERROR;
                    else
                        rc = m_ThreadStatus;
                }
            }
            else
            {
                CloseHandle(m_AsyncThread);
                m_AsyncThread = NULL;
                setNTError(m_ConnectHandle->getErrorMsgLang(), "ExtendedFetch - GetExitCodeThread()");
                rc = SQL_ERROR;
            }
        }
        else
        {
            if (m_ThreadStatus == SQL_STILL_EXECUTING)
                SkipProcess = TRUE;
            rc = SendExtendedFetch(SkipProcess,FetchOrientation,FetchOffset,RowCountPtr,RowStatusArray);
        }
    }
    else
        rc = SendExtendedFetch(SkipProcess,FetchOrientation,FetchOffset,RowCountPtr,RowStatusArray);
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
                m_AsyncCanceled = FALSE;
                setDiagRec(DRIVER_ERROR, IDS_S1_008);
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
        SQLLEN		BufferLength,
        SQLLEN		*StrLen_or_IndPtr)
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
    BYTE*			SQLDataValue;
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
        setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, "The value specified for the argument ColumnNumber was greater than the number of columns in the result set.",
                "07009", m_CurrentRowInRowset, ColumnNumber);
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


    /*
     *  In the case of fetch returning no data plus warnings, the application could potentially
     *   call getdata - handle this condition here
     */
    if (m_RowsFetched == 0)
        return SQL_NO_DATA;


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
            // called SQLBindCol() before (a SQLFetch() and) SQLGetData(). Note that the problem does
            // happen with Catalog APIs where we are still getting the data in column-wise format.
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
        if (SQLDataInd != -1 && SQLCharset != SQLCHARSETCODE_UCS2 &&
                ((SQLDataType == SQLTYPECODE_CHAR) || (SQLDataType == SQLTYPECODE_BIT) || (SQLDataType == SQLTYPECODE_VARCHAR)))
            SQLDataLength++;

        if(m_MaxLength > 0)
        {
            switch (SQLDataType)
            {
                case SQLTYPECODE_CHAR:
                case SQLTYPECODE_BIT:
                case SQLTYPECODE_VARCHAR:
                    {
                        bool increment = false;
                        if (m_MaxLength > 0)
                        {
                            if (SQLDataLength - 1 > m_MaxLength)
                            {
                                SQLDataLength = m_MaxLength + 1;
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
                        if (m_MaxLength > 0 && SQLDataValue != NULL)
                        {
                            SQLDataLength = *(USHORT *)SQLDataValue;
                            if (SQLDataLength > m_MaxLength)
                                *(USHORT *)SQLDataValue = m_MaxLength;
                            //SQLDataLength-3 to account for length(2 bytes) and null terminator(1 byte)
                            //SQLDataLength = ((SQLDataLength-3)>maxLength)?maxLength + 3:SQLDataLength;
                            //*(USHORT *)SQLDataValue = SQLDataLength - 3;
                            break;
                        }
                    }
                    break;
            }
        } // if (m_MaxLength > 0)

        SQLValue.dataType = SQLDataType;
        SQLValue.dataInd = SQLDataInd;
        SQLValue.dataCharset = SQLCharset;
        SQLValue.dataValue._length = SQLDataLength;
        SQLValue.dataValue._buffer = SQLDataValue;
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
                SQLDataLength = dataLengthFetchPerf(SQLDataType, SQLOctetLength, getMaxLength(), SQLCharset, SQLDataValue);
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
        SQLValue.dataValue._buffer = SQLDataValue;
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

BOOL CStmt::checkInputParam(string SqlString)
{
    char del;
    char ch;
    int state = 0;

    unsigned int size = SqlString.size();

    for (unsigned int i=0; i < size; i++)
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
                else if ((ch) == ' ' || (ch) == '\t')
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
        m_FetchDataValue.rowAddress[row] = (LONG_PTR)address;
}

BYTE* CStmt::getRowAddress( unsigned long row)
{
    if (row <= m_FetchDataValue.numberOfRows)
        return (unsigned char*)m_FetchDataValue.rowAddress[row];
    else
        return 0;
}

void CStmt::clearFetchDataValue(void)
{
    if (m_outputDataValue._length != 0 &&
            m_outputDataValue._buffer != NULL)
        delete m_outputDataValue._buffer;
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
        delete m_InputValueList._buffer;
    m_InputValueList._buffer = 0;
    m_InputValueList._length = 0;
}

SQLRETURN CStmt::mapSqlRowIdsToDrvrRowIds(BYTE *&WarningOrError)
{
    SQLULEN AppArrayStatusSize;
    IDL_long sqlRowId = -1, drvrRowId = -1;  //sql row id starts counting at 1, not 0
    unsigned char *curptr = (unsigned char*)WarningOrError;
    long msg_total_len = 0;

    IDL_long numConditions = 0;
    IDL_long errorTextLen = 0;
    int i;

    if (curptr == NULL)
        return SQL_ERROR;

    numConditions = *(IDL_long*)(curptr+msg_total_len);
    msg_total_len +=4;

    if (numConditions > 0)
    {
        m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &AppArrayStatusSize, SQL_IS_UINTEGER, NULL);

        for (i = 0; i < numConditions; i++)
        {
            sqlRowId= *(IDL_long*)(curptr+msg_total_len);

            if (AppArrayStatusSize > sqlRowId  && sqlRowId > 0)
            {
                //find trueRowId
                drvrRowId = m_RowIdMap[sqlRowId-1];
                memcpy(curptr+msg_total_len,&drvrRowId,sizeof(IDL_long));
            }
            msg_total_len +=4;  //move past rowId
            msg_total_len +=4;  //move past sqlCode

            errorTextLen= *(IDL_long*)(curptr+msg_total_len);
            msg_total_len +=4;  //move past errorTextLen
            msg_total_len +=errorTextLen;
            msg_total_len += 6 * (sizeof(char));//sizeof(sqlState);
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

// for tcpip transport

CTCPIPSystemDrvr* CStmt::getSrvrTCPIPSystem()
{
    return m_ConnectHandle->m_srvrTCPIPSystem;
}

// obsolete - get rid of it once the collapsed driver goes away for genus
void CStmt::setRowsetArrayStatus(const ERROR_DESC_LIST_def *sqlWarning, long rowsAffected)
{
    SQLUINTEGER AppArrayStatusPtr;
    SQLUINTEGER AppArrayStatusSize;
    ERROR_DESC_def *_buffer;
    IDL_long rowId;
    unsigned long i,count;

    m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &AppArrayStatusSize, SQL_IS_UINTEGER, NULL);
    m_ImpParamDesc->GetDescField(0, SQL_DESC_ROWS_PROCESSED_PTR, &AppArrayStatusPtr, SQL_IS_UINTEGER, NULL);

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

    long numConditions = 0;
    char sqlState[6];
    long errorTextLen = 0;
    long rowId = 0;

    sqlState[0] = '\0';

    unsigned char *curptr;
    int i;

#ifdef _WIN64
    m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &AppArrayStatusSize, SQL_IS_POINTER, NULL);
    m_ImpParamDesc->GetDescField(0, SQL_DESC_ARRAY_STATUS_PTR, &AppArrayStatusPtr, SQL_C_UBIGINT, NULL);
#else
    m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &AppArrayStatusSize, SQL_IS_UINTEGER, NULL);
    m_ImpParamDesc->GetDescField(0, SQL_DESC_ARRAY_STATUS_PTR, &AppArrayStatusPtr, SQL_IS_UINTEGER, NULL);
#endif

    if (AppArrayStatusPtr == NULL)
        return;

    if (rowsAffected == -1)
    {
        m_ImpParamDesc->setDescArrayStatus(AppArrayStatusSize, SQL_PARAM_ERROR);
        return;
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

            m_ImpParamDesc->setDescArrayStatusAt(rowId, SQL_PARAM_ERROR);
        }
    }

} /* setRowsetArrayStatus() */

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

SQLRETURN CStmt::SendExtractLob(IDL_short extractType,
    IDL_string lobHandle,
    IDL_long   lobHandleLen,
    IDL_long &extractLen,
    BYTE * &extractData)
{
    SQLRETURN         rc = SQL_SUCCESS;

    m_SrvrCallContext.odbcAPI = SRVR_API_EXTRACTLOB;
    m_SrvrCallContext.sqlHandle = this;
    m_SrvrCallContext.SQLSvc_ObjRef = m_ConnectHandle->getSrvrObjRef();
    m_SrvrCallContext.ASSvc_ObjRef = NULL;
    m_SrvrCallContext.eventHandle = m_StmtEvent;
    m_SrvrCallContext.dialogueId = m_ConnectHandle->getDialogueId();
    m_SrvrCallContext.connectionTimeout = m_ConnectHandle->getConnectionTimeout();
    m_SrvrCallContext.u.fetchParams.queryTimeout = m_QueryTimeout;
    m_SrvrCallContext.u.extractLobParams.extractType = extractType;
    m_SrvrCallContext.u.extractLobParams.lobHandle = lobHandle;
    m_SrvrCallContext.u.extractLobParams.lobHandleLen = lobHandleLen;
    m_SrvrCallContext.u.extractLobParams.extractLen = extractLen;
    m_SrvrCallContext.u.extractLobParams.extractData = extractData;

    if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
    {
        if ((m_AsyncThread = (HANDLE)_beginthreadex(NULL, 0, ThreadControlProc,
            &m_SrvrCallContext, CREATE_SUSPENDED, &m_ThreadAddr)) == 0)
        {
            setNTError(m_ConnectHandle->getErrorMsgLang(), "SendExtractLob - _beginthreadex()");
            rc = SQL_ERROR;
        }
        ResumeThread(m_AsyncThread);
        rc = SQL_STILL_EXECUTING;
        Sleep(0);
    }
    else
    {
        if ((m_SyncThread = (HANDLE)_beginthreadex(NULL, 0, ThreadControlProc,
            &m_SrvrCallContext, CREATE_SUSPENDED, &m_ThreadAddr)) == 0)
        {
            setNTError(m_ConnectHandle->getErrorMsgLang(), "SendExtractLob - _beginthreadex()");
            rc = SQL_ERROR;
        }
        else
        {
            ResumeThread(m_SyncThread);
            WaitForSingleObject(m_SyncThread, INFINITE);
            GetExitCodeThread(m_SyncThread, &m_ThreadStatus);
            rc = m_ThreadStatus;
            CloseHandle(m_SyncThread);
            m_SyncThread = NULL;
        }
    }

    if (rc == SQL_SUCCESS)
    {
        extractLen = m_SrvrCallContext.u.extractLobParams.extractLen;
        extractData = m_SrvrCallContext.u.extractLobParams.extractData;

        if (m_ConnectHandle->lobHandleSave != NULL)
            free(m_ConnectHandle->lobHandleSave);

        m_ConnectHandle->lobHandleSave = (IDL_string)malloc(lobHandleLen);
        memcpy(m_ConnectHandle->lobHandleSave, lobHandle, lobHandleLen);
        m_ConnectHandle->lobHandleLenSave = lobHandleLen;
    }

    return rc;
}

SQLRETURN CStmt::ExtractLob(IDL_short extractType,
    IDL_string lobHandle,
    IDL_long   lobHandleLen,
    IDL_long   &extractLen,
    BYTE * &extractData)
{
    SQLRETURN   rc = SQL_SUCCESS;
    BOOL		SkipProcess = FALSE;

    m_CurrentOdbcAPI = SRVR_API_EXTRACTLOB;
    if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
    {
        if (m_AsyncThread != NULL)
        {
            if (GetExitCodeThread(m_AsyncThread, &m_ThreadStatus))
            {
                if (m_ThreadStatus == STILL_ACTIVE)
                    rc = SQL_STILL_EXECUTING;
                else
                {
                    CloseHandle(m_AsyncThread);
                    m_AsyncThread = NULL;
                    if (m_AsyncCanceled == TRUE)
                        rc = SQL_ERROR;
                    else
                        rc = m_ThreadStatus;
                }
            }
            else
            {
                CloseHandle(m_AsyncThread);
                m_AsyncThread = NULL;
                setNTError(m_ConnectHandle->getErrorMsgLang(), "Fetch - GetExitCodeThread()");
                rc = SQL_ERROR;
            }
        }
        else
        {
            if (m_ThreadStatus == SQL_STILL_EXECUTING)
                SkipProcess = TRUE;
            rc = SendExtractLob(extractType, lobHandle, lobHandleLen, extractLen, extractData);
        }
    }
    else
        rc = SendExtractLob(extractType, lobHandle, lobHandleLen, extractLen, extractData);

    return rc;
}

SQLRETURN CStmt::SendUpdateLob(IDL_long updateType,
    IDL_string lobHandle,
    IDL_long   lobHandleLen,
    IDL_long_long totalLength,
    IDL_long_long offset,
    IDL_long_long pos,
    IDL_long_long length,
    BYTE *        data)
{
    SQLRETURN         rc = SQL_SUCCESS;

    m_SrvrCallContext.odbcAPI = SRVR_API_UPDATELOB;
    m_SrvrCallContext.sqlHandle = this;
    m_SrvrCallContext.SQLSvc_ObjRef = m_ConnectHandle->getSrvrObjRef();
    m_SrvrCallContext.ASSvc_ObjRef = NULL;
    m_SrvrCallContext.eventHandle = m_StmtEvent;
    m_SrvrCallContext.dialogueId = m_ConnectHandle->getDialogueId();
    m_SrvrCallContext.connectionTimeout = m_ConnectHandle->getConnectionTimeout();
    m_SrvrCallContext.u.fetchParams.queryTimeout = m_QueryTimeout;
    m_SrvrCallContext.u.updateLobParams.updateType = updateType;
    m_SrvrCallContext.u.updateLobParams.lobHandle = lobHandle;
    m_SrvrCallContext.u.updateLobParams.lobHandleLen = lobHandleLen;
    m_SrvrCallContext.u.updateLobParams.totalLength = totalLength;
    m_SrvrCallContext.u.updateLobParams.offset = offset;
    m_SrvrCallContext.u.updateLobParams.pos = pos;
    m_SrvrCallContext.u.updateLobParams.length = length;
    m_SrvrCallContext.u.updateLobParams.data = data;

    if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
    {
        if ((m_AsyncThread = (HANDLE)_beginthreadex(NULL, 0, ThreadControlProc,
            &m_SrvrCallContext, CREATE_SUSPENDED, &m_ThreadAddr)) == 0)
        {
            setNTError(m_ConnectHandle->getErrorMsgLang(), "SendExtractLob - _beginthreadex()");
            rc = SQL_ERROR;
        }
        ResumeThread(m_AsyncThread);
        rc = SQL_STILL_EXECUTING;
        Sleep(0);
    }
    else
    {
        if ((m_SyncThread = (HANDLE)_beginthreadex(NULL, 0, ThreadControlProc,
            &m_SrvrCallContext, CREATE_SUSPENDED, &m_ThreadAddr)) == 0)
        {
            setNTError(m_ConnectHandle->getErrorMsgLang(), "SendExtractLob - _beginthreadex()");
            rc = SQL_ERROR;
        }
        else
        {
            ResumeThread(m_SyncThread);
            WaitForSingleObject(m_SyncThread, INFINITE);
            GetExitCodeThread(m_SyncThread, &m_ThreadStatus);
            rc = m_ThreadStatus;
            CloseHandle(m_SyncThread);
            m_SyncThread = NULL;
        }
    }

    return rc;
}

SQLRETURN CStmt::UpdateLob(IDL_long updateType,
    IDL_string lobHandle,
    IDL_long   lobHandleLen,
    IDL_long_long totalLength,
    IDL_long_long offset,
    IDL_long_long pos,
    IDL_long_long length,
    BYTE *        data)
{
    SQLRETURN   rc = SQL_SUCCESS;
    BOOL		SkipProcess = FALSE;

    if (m_AsyncEnable == SQL_ASYNC_ENABLE_ON)
    {
        if (m_AsyncThread != NULL)
        {
            if (GetExitCodeThread(m_AsyncThread, &m_ThreadStatus))
            {
                if (m_ThreadStatus == STILL_ACTIVE)
                    rc = SQL_STILL_EXECUTING;
                else
                {
                    CloseHandle(m_AsyncThread);
                    m_AsyncThread = NULL;
                    if (m_AsyncCanceled == TRUE)
                        rc = SQL_ERROR;
                    else
                        rc = m_ThreadStatus;
                }
            }
            else
            {
                CloseHandle(m_AsyncThread);
                m_AsyncThread = NULL;
                setNTError(m_ConnectHandle->getErrorMsgLang(), "Fetch - GetExitCodeThread()");
                rc = SQL_ERROR;
            }
        }
        else
        {
            if (m_ThreadStatus == SQL_STILL_EXECUTING)
                SkipProcess = TRUE;
            rc = SendUpdateLob(updateType, lobHandle, lobHandleLen, totalLength, offset, pos, length, data);
        }
    }
    else
        rc = SendUpdateLob(updateType, lobHandle, lobHandleLen, totalLength, offset, pos, length, data);

    return rc;
}
