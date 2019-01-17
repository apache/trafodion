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
// MODULE: NetConnect.cpp
//
//
// PURPOSE: Krypton - Client Module 
//
//
#include "process.h"

#include "DrvrGlobal.h"
#include "DrvrNet.h"
#include "CStmt.h"
#include "DiagFunctions.h"
#include "tdm_odbcDrvMsg.h"
#include "odbc_cl.h"
#include "ceecfg.h"
#include "ping.h"
#include "nskieee.h"
#include "sqlcli.h"
#include "odbcs_drvr.h"

using namespace ODBC;

SQLRETURN SQLPREPARE_(SRVR_CALL_CONTEXT *srvrCallContext)
{
	CEE_status	sts;
	SQLRETURN	rc;

	IDL_long	returnCode = 0;
	BYTE       *sqlWarningOrError = NULL;
	IDL_long	sqlStmtQueryType;
	IDL_long	stmtHandle;
	IDL_long	estimatedCost = 0;
	IDL_char   *stmtOptions = NULL;
	BYTE       *inputParams = NULL;
	SQLItemDescList_def inputDesc = {0,0};
	BYTE       *outputColumns = NULL;
	SQLItemDescList_def outputDesc = {0,0};

	CStmt *pStatement = (CStmt *)srvrCallContext->sqlHandle;

	sts = odbc_SQLDrvr_Prepare_pst_(
					srvrCallContext,
					stmtOptions,
					&returnCode,
					sqlWarningOrError,
					&sqlStmtQueryType,
					&stmtHandle,
					&estimatedCost,
					inputParams,
					&inputDesc,
					outputColumns,
					&outputDesc);

	if (sts != CEE_SUCCESS)
	{
		if (sts == CEE_INTERNALFAIL)
			pStatement->setDiagRec(DRIVER_ERROR, IDS_EXCEPTION_MSG,0,"SQL SERVER",
				NULL,SQL_ROW_NUMBER_UNKNOWN,SQL_COLUMN_NUMBER_UNKNOWN,2,"Internal Error","SQLPREPARE_");
		else if (sts == TIMEOUT_EXCEPTION)
			pStatement->setDiagRec(DRIVER_ERROR, IDS_S1_T00, sts, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem())); 
		else if (sts == COMM_LINK_FAIL_EXCEPTION)
			pStatement->setDiagRec(DRIVER_ERROR, IDS_08_S01, sts, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem()));
		else if (sts == TRANSPORT_ERROR)
			pStatement->setDiagRec(DRIVER_ERROR, IDS_08_S02, sts, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem()));
		else
			pStatement->setDiagRec(DRIVER_ERROR, IDS_S1_000, sts, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem()), NULL, 
				SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 1,"SQLPREPARE_ failed");
		return SQL_ERROR;
	}

	pStatement->setExceptionErrors(returnCode, 0);
	switch (returnCode) 
	{
		case SQL_SUCCESS:
		case SQL_SUCCESS_WITH_INFO:

			if(sqlStmtQueryType == SQL_CALL_NO_RESULT_SETS ||
		       sqlStmtQueryType == SQL_CALL_WITH_RESULT_SETS ||
			   sqlStmtQueryType == SQL_SP_RESULT_SET )
			{
				//
				// Need to save the input/output params descriptors for SPJ calls
				// since users can prepare once and execute multiple times
				//
				if(inputDesc._length > 0)
				{
					pStatement->m_spjInputParamDesc._length = inputDesc._length;
					pStatement->m_spjInputParamDesc._buffer = new SQLItemDesc_def[pStatement->m_spjInputParamDesc._length];
					memcpy(pStatement->m_spjInputParamDesc._buffer,inputDesc._buffer, inputDesc._length * sizeof(SQLItemDesc_def));
				}
				else
				{
					if(pStatement->m_spjInputParamDesc._length &&
						pStatement->m_spjInputParamDesc._buffer != NULL)
						delete[] pStatement->m_spjInputParamDesc._buffer;

					pStatement->m_spjInputParamDesc._length = 0;
					pStatement->m_spjInputParamDesc._buffer = NULL;
				}

				if(outputDesc._length > 0)
				{
					pStatement->m_spjOutputParamDesc._length = outputDesc._length;
					pStatement->m_spjOutputParamDesc._buffer = new SQLItemDesc_def[pStatement->m_spjOutputParamDesc._length];
					memcpy(pStatement->m_spjOutputParamDesc._buffer,outputDesc._buffer, outputDesc._length * sizeof(SQLItemDesc_def));
				}
				else
				{
					if(pStatement->m_spjOutputParamDesc._length &&
						pStatement->m_spjOutputParamDesc._buffer != NULL)
						delete[] pStatement->m_spjOutputParamDesc._buffer;

					pStatement->m_spjOutputParamDesc._length = 0;
					pStatement->m_spjOutputParamDesc._buffer = NULL;
				}


				rc = pStatement->setDescRec(&pStatement->m_spjInputParamDesc, &pStatement->m_spjOutputParamDesc);
			}
			else
				rc = pStatement->setDescRec(&inputDesc, &outputDesc);


		   pStatement->setExceptionErrors(rc);
		   pStatement->setStmtQueryType(sqlStmtQueryType);
		   pStatement->setStmtHandle(stmtHandle);
		   pStatement->setStmtData(inputParams, outputColumns);
		   if (returnCode != SQL_SUCCESS)
		      pStatement->setDiagRec(sqlWarningOrError, returnCode);
		   break;

		case SQL_STILL_EXECUTING:
   		   break;

		case SQL_ERROR:
		   pStatement->setDiagRec(sqlWarningOrError, returnCode);
		   break;	

		case SQL_INVALID_HANDLE:
		   pStatement->setDiagRec(SERVER_ERROR, IDS_08_S01);
		   break;

		default:
		   pStatement->sendCDInfo(returnCode);
		   pStatement->setDiagRec(returnCode, PREPARE_PROCNAME, pStatement->getSrvrIdentity());
		   break;
	}


	if (sqlWarningOrError != NULL)
		delete sqlWarningOrError;

	if (inputParams != NULL)
		delete inputParams;

	if (inputDesc._buffer != NULL)
	{
		delete inputDesc._buffer;
		inputDesc._length = 0;
		inputDesc._buffer = NULL;
	}
	if (outputColumns != NULL)
		delete outputColumns;

	if (outputDesc._buffer != NULL)
	{
		delete outputDesc._buffer;
		outputDesc._length = 0;
		outputDesc._buffer = NULL;
	}
	
	return (SQLRETURN)returnCode;

} /* SQLPREPARE_() */

SQLRETURN SQLFETCH_(SRVR_CALL_CONTEXT *srvrCallContext)
{
	CEE_status	sts;
	IDL_long    stmtHandle = 0;
	IDL_long    returnCode = 0;
	IDL_long	outValuesFormat = 0;	
	IDL_long    rowsReturned= 0;
	BYTE       *sqlWarningOrError = 0;
	IDL_char   *stmtOptions = NULL;
	IDL_long			stmtQueryType=-1;
	SQL_DataValue_def	outputDataValue={0,0};

	IDL_unsigned_long Index=0;
	IDL_unsigned_long Items=0;
	IDL_unsigned_long rowLength=0;
	IDL_long count;
	IDL_long colnumber;
	IDL_long rowNumber=0;

	SQLINTEGER              SQLCharset=0;
	SQLSMALLINT             SQLDataType=0;
	SQLSMALLINT             SQLDatetimeCode=0;
	SQLINTEGER              SQLOctetLength=0;
	SQLSMALLINT             SQLPrecision=0;
	SQLSMALLINT             SQLUnsigned=0;

	IDL_short               SQLDataInd=0;
	IDL_short               SQLDataLength=0;
	BYTE*                   SQLDataValue;
	BOOL                    byteSwap;

	CStmt *pStatement = (CStmt *)srvrCallContext->sqlHandle;
	if (pStatement->m_isClosed)
		return SQL_NO_DATA_FOUND;


	DWORD preFetchThreadStatus;
	if (pStatement->m_preFetchThread.m_Thread != NULL)
	{
		if(pStatement->m_preFetchThread.m_State == PREFETCH_STATE_WRK_STARTED ||
		   pStatement->m_preFetchThread.m_State == PREFETCH_STATE_WRK_DONE)
		   // the prefetch thread is fetching or is done
		{
			GetExitCodeThread(pStatement->m_preFetchThread.m_Thread, &preFetchThreadStatus);
			if (preFetchThreadStatus == STILL_ACTIVE)
			   WaitForSingleObject(pStatement->m_preFetchThread.m_Thread,INFINITE);
			GetExitCodeThread(pStatement->m_preFetchThread.m_Thread, &preFetchThreadStatus);
			sts = preFetchThreadStatus;
			CloseHandle(pStatement->m_preFetchThread.m_Thread);
			pStatement->m_preFetchThread.m_Thread = NULL;
			pStatement->m_preFetchThread.m_State = PREFETCH_STATE_WRK_UNASSIGNED;

			returnCode = pStatement->m_preFetchThread.returnCode;
			sqlWarningOrError = pStatement->m_preFetchThread.sqlWarningOrError;
			rowsReturned = pStatement->m_preFetchThread.rowsReturned;
			outValuesFormat = pStatement->m_preFetchThread.outValuesFormat;
			outputDataValue._length = pStatement->m_preFetchThread.outputDataValue._length;
			outputDataValue._buffer = pStatement->m_preFetchThread.outputDataValue._buffer;
			pStatement->m_preFetchThread.initializeResults();
		}
		else
		{
			// fetch data synchronously
			sts = odbc_SQLDrvr_Fetch_pst_(
					srvrCallContext,
					stmtOptions,
					&returnCode,
					sqlWarningOrError,
					&rowsReturned,
					&outValuesFormat,
					&outputDataValue);
		}
	}
	else
	{
		pStatement->m_preFetchThread.initializeResults();

		sts = odbc_SQLDrvr_Fetch_pst_(
				srvrCallContext,
				stmtOptions,
				&returnCode,
				sqlWarningOrError,
				&rowsReturned,
				&outValuesFormat,
				&outputDataValue);
	}

	SQLUINTEGER maxLength = pStatement->getMaxLength();
	SQL_DataValue_def       *DataValue = &outputDataValue;
	stmtHandle = pStatement->getStmtHandle();

	if (sts != CEE_SUCCESS)
	{
		if (sts == CEE_INTERNALFAIL)
			pStatement->setDiagRec(DRIVER_ERROR, IDS_EXCEPTION_MSG,0,"SQL SERVER",
				NULL,SQL_ROW_NUMBER_UNKNOWN,SQL_COLUMN_NUMBER_UNKNOWN,2,"Internal Error","SQLFETCH_");
		else if (sts == TIMEOUT_EXCEPTION)
			pStatement->setDiagRec(DRIVER_ERROR, IDS_S1_T00, sts, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem())); 
		else if (sts == COMM_LINK_FAIL_EXCEPTION)
			pStatement->setDiagRec(DRIVER_ERROR, IDS_08_S01, sts, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem()));
		else if (sts == TRANSPORT_ERROR)
			pStatement->setDiagRec(DRIVER_ERROR, IDS_08_S02, sts, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem()));
		else
			pStatement->setDiagRec(DRIVER_ERROR, IDS_S1_000, sts, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem()), NULL, 
				SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 1,"SQLFETCH_ failed");
		return SQL_ERROR;
	}


	
	pStatement->setExceptionErrors(returnCode, 0);
	CConnect *pConnection = pStatement->getConnectHandle();

	switch (returnCode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
		
		pStatement->setDiagRowCount(rowsReturned, -1);
		pStatement->setRowCount(rowsReturned);

		if (returnCode != SQL_SUCCESS)
			pStatement->setDiagRec(sqlWarningOrError, returnCode);

		stmtQueryType = pStatement->getStmtQueryType();

		if(!pStatement->getFetchCatalog()) //if (stmtQueryType == 10000)
		{
			if (!pStatement->setFetchOutputPerf(DataValue, rowsReturned))
			{
				pStatement->setDiagRec(DRIVER_ERROR, IDS_S1_000_06);
				pStatement->clearFetchDataValue();
				return SQL_ERROR;
			}
		}
		else // Hack! To take care of the catalog APIs, should be removed once refactored code is in 
		{
			if (!pStatement->setFetchOutputPerf(rowsReturned, DataValue))
			{
				pStatement->setDiagRec(DRIVER_ERROR, IDS_S1_000_06);
				pStatement->clearFetchDataValue();
				break;
			}
		}

		byteSwap = pStatement->getByteSwap();
		count = pStatement->getImpDescCount();

		if(!pStatement->getFetchCatalog()) // if (stmtQueryType == 10000)
		{
			
			if(rowsReturned > 0)
				rowLength = DataValue->_length / rowsReturned;
			
			for (int rowOffset=0, row =0; row < rowsReturned; row++)
			{
				pStatement->setRowAddress(row,(DataValue->_buffer + rowOffset));
				rowOffset += rowLength;
			}
		}
		else	//With RWRS, only catalog API data takes this path
		{
			while (Index < DataValue->_length)
			{
				if (Items % count == 0)
				{
					rowNumber++;
					pStatement->setRowAddress(Items/count,DataValue->_buffer + Index);
				}
				SQLDataInd = (short)*(unsigned char*)(DataValue->_buffer + Index);
				if (SQLDataInd == 0)
				{
					colnumber = Items % count + 1;
					pStatement->getImpSQLData(colnumber, SQLCharset, SQLDataType, SQLDatetimeCode, SQLOctetLength, SQLPrecision,SQLUnsigned);
					SQLDataValue = DataValue->_buffer + Index + 1;
					if (byteSwap)
						SQLDatatype_Dependent_Swap(SQLDataValue, SQLDataType, SQLCharset, SQLOctetLength);
					
					Index = Index + dataLengthFetchPerf(SQLDataType, SQLOctetLength, maxLength, SQLCharset, SQLDataValue);
				}
				Index = Index + 1;
				Items++;
			}
		}

		break;

	case SQL_STILL_EXECUTING:
		break;

	case SQL_NO_DATA_FOUND:
		break;

	case SQL_ERROR:
		pStatement->setDiagRec(sqlWarningOrError, returnCode);
		break;

	case SQL_INVALID_HANDLE:
		pStatement->setDiagRec(SERVER_ERROR, IDS_08_S01);
		break;

	default:
		pStatement->sendCDInfo(returnCode);
		pStatement->setDiagRec(returnCode, EXECUTE_PROCNAME, pStatement->getSrvrIdentity());
		break;
	}

	if (sqlWarningOrError != NULL)
	    delete sqlWarningOrError;

	if ( returnCode == SQL_SUCCESS_WITH_INFO || returnCode == SQL_SUCCESS )
	{
		if((pStatement->m_Concurrency == SQL_CONCUR_READ_ONLY) && (pStatement->m_AsyncEnable != SQL_ASYNC_ENABLE_ON))
		{
			if (pStatement->m_preFetchThread.m_Thread == NULL)
			{
				if ((pStatement->m_preFetchThread.m_Thread = (HANDLE)_beginthreadex(NULL, 0, odbc_SQLDrvr_PreFetch_pst_, 
							pStatement, CREATE_SUSPENDED, NULL)) > 0)
				{
					ResumeThread(pStatement->m_preFetchThread.m_Thread);
					pStatement->m_preFetchThread.m_State = PREFETCH_STATE_WRK_ASSIGNED;
				}
			}
		}
	}
	else if (pStatement->m_preFetchThread.m_Thread != NULL)
	{
		if( !(pStatement->m_preFetchThread.m_State == PREFETCH_STATE_WRK_STARTED ||
		      pStatement->m_preFetchThread.m_State == PREFETCH_STATE_WRK_DONE) )
		   pStatement->m_preFetchThread.m_State = PREFETCH_STATE_WRK_CANCELLED;
	}

	return (SQLRETURN)returnCode;

} /* SQLFETCH_() */


SQLRETURN FREESTATEMENT(SRVR_CALL_CONTEXT *srvrCallContext)
{
	CEE_status	sts;

	CStmt    *pStatement = (CStmt *)srvrCallContext->sqlHandle;
	CConnect *pConnection = pStatement->getConnectHandle();
	IDL_long returnCode = 0;
	IDL_long rowsAffected = 0;
	BYTE     *sqlWarningOrError = 0;

	if (srvrCallContext->SQLSvc_ObjRef[0] == 0)
		return SQL_SUCCESS;

	DWORD preFetchThreadStatus;
	if (pStatement->m_preFetchThread.m_Thread != NULL)
	{
		GetExitCodeThread(pStatement->m_preFetchThread.m_Thread, &preFetchThreadStatus);
		if (preFetchThreadStatus == STILL_ACTIVE)
		{
		   LeaveCriticalSection(&pConnection->m_CSTransmision);
		   WaitForSingleObject(pStatement->m_preFetchThread.m_Thread,INFINITE);
	       EnterCriticalSection(&pConnection->m_CSTransmision);
		}
		GetExitCodeThread(pStatement->m_preFetchThread.m_Thread, &preFetchThreadStatus);
		CloseHandle(pStatement->m_preFetchThread.m_Thread);
		pStatement->m_preFetchThread.m_Thread = NULL;
	}
	pStatement->m_preFetchThread.initializeResults();


	// Destroy the previous proxy if needed

	sts = odbc_SQLDrvr_Close_pst_(srvrCallContext,
							      &returnCode,
							      &rowsAffected,
							      sqlWarningOrError);

	
	if (sts != CEE_SUCCESS)
	{
		if (sts == CEE_INTERNALFAIL)
			pStatement->setDiagRec(DRIVER_ERROR, IDS_EXCEPTION_MSG,0,"SQL SERVER",
				NULL,SQL_ROW_NUMBER_UNKNOWN,SQL_COLUMN_NUMBER_UNKNOWN,2,"Internal Error","FREESTATEMENT");
		else if (sts == TIMEOUT_EXCEPTION)
			pStatement->setDiagRec(DRIVER_ERROR, IDS_S1_T00, sts, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem())); 
		else if (sts == COMM_LINK_FAIL_EXCEPTION)
			pStatement->setDiagRec(DRIVER_ERROR, IDS_08_S01, sts, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem()));
		else if (sts == TRANSPORT_ERROR)
			pStatement->setDiagRec(DRIVER_ERROR, IDS_08_S02, sts, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem()));
		else
			pStatement->setDiagRec(DRIVER_ERROR, IDS_S1_000, sts, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem()), NULL, 
				SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 1,"FREESTATEMENT failed");
		return SQL_ERROR;
	}
	
	pStatement->setExceptionErrors(returnCode, 0);

	// Start CCF
	switch (returnCode) 
	{
	   case SQL_SUCCESS:
       case SQL_SUCCESS_WITH_INFO:
	      pStatement->setDiagRowCount(rowsAffected, -1);
		  pStatement->setRowCount(rowsAffected);
          if(returnCode == SQL_SUCCESS_WITH_INFO)
 	         pStatement->setDiagRec(sqlWarningOrError, returnCode);
		  break;
		  
	   case SQL_ERROR:
	      pStatement->setDiagRec(sqlWarningOrError, returnCode);
		  break;

	   default:
	      pStatement->sendCDInfo(returnCode);
		  pStatement->setDiagRec(returnCode, FREESTATEMENT_PROCNAME, pStatement->getSrvrIdentity());
		  break;
	}
    // End of CCF

	
	if (sqlWarningOrError != NULL)
		delete sqlWarningOrError;
	
	return (SQLRETURN)returnCode;
}


SQLRETURN SMDCATALOGS(SRVR_CALL_CONTEXT *srvrCallContext)
{
	CEE_status	sts;
	SQLRETURN	rc;

    struct odbc_SQLSvc_GetSQLCatalogs_exc_ exception_ = {0,0,0};
    SQLItemDescList_def outputItemDescList = {0,0};
	ERROR_DESC_LIST_def sqlWarning = {0,0};
	IDL_string catStmtLabel = NULL;

	CStmt *pStatement = (CStmt *)srvrCallContext->sqlHandle;

	sts = odbc_SQLDrvr_GetSQLCatalogs_pst_(
									srvrCallContext,
									&catStmtLabel,
									&outputItemDescList,
                                    &exception_,
									&sqlWarning);
    if (sts != CEE_SUCCESS)
	{
		if (sts == CEE_INTERNALFAIL)
			pStatement->setDiagRec(DRIVER_ERROR, IDS_EXCEPTION_MSG,0,"SQL SERVER",
				NULL,SQL_ROW_NUMBER_UNKNOWN,SQL_COLUMN_NUMBER_UNKNOWN,2,"Internal Error","FREESTATEMENT");
		else if (sts == TIMEOUT_EXCEPTION)
			pStatement->setDiagRec(DRIVER_ERROR, IDS_S1_T00, sts, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem())); 
		else if (sts == COMM_LINK_FAIL_EXCEPTION)
			pStatement->setDiagRec(DRIVER_ERROR, IDS_08_S01, sts, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem()));
		else if (sts == TRANSPORT_ERROR)
			pStatement->setDiagRec(DRIVER_ERROR, IDS_08_S02, sts, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem()));
		else
			pStatement->setDiagRec(DRIVER_ERROR, IDS_S1_000, sts, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem()), NULL, 
				SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 1,"FREESTATEMENT failed");
		return SQL_ERROR;
	}

    odbc_SQLSvc_GetSQLCatalogs_ccf_ (srvrCallContext,
                                     &exception_,
                                     catStmtLabel,
                                     &outputItemDescList,
                                     &sqlWarning);


	//
	// Cleanup
	//
	if(outputItemDescList._length > 0)
		delete[] outputItemDescList._buffer;


	if(exception_.exception_nr == odbc_SQLSvc_GetSQLCatalogs_SQLError_exn_ &&
	   exception_.u.SQLError.errorList._length > 0 )
  	      delete[] exception_.u.SQLError.errorList._buffer;

	if(sqlWarning._length > 0)
		delete[] sqlWarning._buffer;


	rc = checkNetStmt(srvrCallContext,sts,"SMDCATALOGS");
	return rc;

} // SMDCATALOGS()

// Call Completion function for odbc_SQLSvc_GetSQLCatalogs

extern "C" void 
odbc_SQLSvc_GetSQLCatalogs_ccf_ (
	CEE_tag_def cmptag_
  , const odbc_SQLSvc_GetSQLCatalogs_exc_ *exception_
  , const IDL_char *catStmtLabel
  , const SQLItemDescList_def *outputDesc
  , const ERROR_DESC_LIST_def *sqlWarning)
{

	SRVR_CALL_CONTEXT	*srvrCallContext = (SRVR_CALL_CONTEXT *)cmptag_;
	CStmt				*pStatement = (CStmt *)srvrCallContext->sqlHandle;
	SQLRETURN			rc;

	pStatement->setExceptionErrors(exception_->exception_nr, exception_->exception_detail);
	switch (exception_->exception_nr) 
	{
	case CEE_SUCCESS:
		pStatement->setStmtLabel(catStmtLabel);
		rc = pStatement->setDescRec(NULL, outputDesc);
		pStatement->setExceptionErrors(rc);
		if (sqlWarning->_length > 0)
			pStatement->setDiagRec(sqlWarning);
		break;
	case odbc_SQLSvc_GetSQLCatalogs_SQLError_exn_:
		pStatement->setDiagRec(&exception_->u.SQLError);
		break;
	case odbc_SQLSvc_GetSQLCatalogs_InvalidConnection_exn_:
		pStatement->setDiagRec(SERVER_ERROR, IDS_08_S01);
		break;
	case odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_:
		pStatement->setDiagRec(SERVER_ERROR, IDS_HY_C00, exception_->exception_nr, 
				exception_->u.ParamError.ParamDesc, NULL, 
				SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 1, pStatement->getSrvrIdentity());
		break;
	case odbc_SQLSvc_GetSQLCatalogs_SQLInvalidHandle_exn_:
		break;
	default:
		pStatement->sendCDInfo(exception_->exception_nr);
		pStatement->setDiagRec(exception_->exception_nr, SMDCATALOGS_PROCNAME, pStatement->getSrvrIdentity());
		break;
	}
	
}


// Call Completion function for a ExecDirect or ExecDirect rowset

extern "C" void
odbc_SQLSvc_ExecDirect_ccf_(
    CEE_tag_def cmptag_
  , const odbc_SQLSvc_ExecDirect_exc_ *exception_
  , IDL_long estimatedCost
  , const SQLItemDescList_def *outputDesc
  , IDL_long rowsAffected
  , const ERROR_DESC_LIST_def *sqlWarning
  )
{
	
	SRVR_CALL_CONTEXT	*srvrCallContext = (SRVR_CALL_CONTEXT *)cmptag_;
	CStmt				*pStatement = (CStmt *)srvrCallContext->sqlHandle;
	SQLRETURN			rc;

	pStatement->setExceptionErrors(exception_->exception_nr, exception_->exception_detail);
	switch (exception_->exception_nr) 
	{
	case CEE_SUCCESS:
		rc = pStatement->setDescRec(NULL, outputDesc);
		pStatement->setExceptionErrors(rc);
		if (sqlWarning->_length > 0)
			pStatement->setDiagRec(sqlWarning);
		pStatement->setDiagRowCount(rowsAffected, -1);
		pStatement->setRowCount(rowsAffected);
		if (estimatedCost == SQL_SELECT_UNIQUE)
		   estimatedCost = SQL_SELECT_NON_UNIQUE;
		// Vijay - Set the statement type to SELECT using estimatedCost to get the QueryType from SQL/Server
		pStatement->setStmtQueryType(estimatedCost);
		break;
	case odbc_SQLSvc_ExecDirect_SQLStillExecuting_exn_:
		break;
	case odbc_SQLSvc_ExecDirect_SQLQueryCancelled_exn_:
		pStatement->setDiagRec(DRIVER_ERROR, IDS_S1_008, exception_->u.SQLQueryCancelled.sqlcode);
		break;
	case odbc_SQLSvc_ExecDirect_SQLError_exn_:
		pStatement->setDiagRec(&exception_->u.SQLError);
		break;	
	case odbc_SQLSvc_ExecDirect_ParamError_exn_:
		if ((pStatement->m_intStmtType == TYPE_QS) || (pStatement->m_intStmtType == TYPE_CMD))
			pStatement->setDiagRec(SERVER_ERROR, IDS_S1_000, 0, exception_->u.ParamError.ParamDesc,NULL, 0, 0, 1, exception_->u.ParamError.ParamDesc);
		else
			pStatement->setDiagRec(SERVER_ERROR, IDS_PROGRAM_ERROR, exception_->exception_nr, 
				exception_->u.ParamError.ParamDesc, NULL, 
				SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 1, pStatement->getSrvrIdentity());
		break;
	case odbc_SQLSvc_ExecDirect_InvalidConnection_exn_:
		pStatement->setDiagRec(SERVER_ERROR, IDS_08_S01);
		break;
	case odbc_SQLSvc_ExecDirect_TransactionError_exn_:
		char tmpNumBuffer[16];
		_itoa (exception_->exception_detail, tmpNumBuffer, 10);
		pStatement->setDiagRec(SERVER_ERROR, IDS_TRANSACTION_ERROR, exception_->exception_nr, 
				tmpNumBuffer, NULL, 
				SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 1, pStatement->getSrvrIdentity());
	default:
		pStatement->sendCDInfo(exception_->exception_nr);
		pStatement->setDiagRec(exception_->exception_nr, EXECDIRECT_PROCNAME, pStatement->getSrvrIdentity());
		break;
	}
}


SQLRETURN checkNetStmt(SRVR_CALL_CONTEXT *srvrCallContext,CEE_status sts, char* procname)
{
	SQLRETURN	rc;

	CStmt *pStatement = (CStmt *)srvrCallContext->sqlHandle;
	if (sts != CEE_SUCCESS)
	{
		if (sts == CEE_INTERNALFAIL)
			pStatement->setDiagRec(DRIVER_ERROR, IDS_EXCEPTION_MSG,0,"SQL SERVER",
				NULL,SQL_ROW_NUMBER_UNKNOWN,SQL_COLUMN_NUMBER_UNKNOWN,2,"Internal Error",procname);
		else if (sts == TIMEOUT_EXCEPTION)
		{
			pStatement->setDiagRec(DRIVER_ERROR, IDS_S1_T00, 0, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem()));
			pStatement->Cancel();
		}
		else if (sts == COMM_LINK_FAIL_EXCEPTION)
			pStatement->setDiagRec(SERVER_ERROR, IDS_08_S01, 0, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem()));
		else if (sts == TRANSPORT_ERROR)
			pStatement->setDiagRec(SERVER_ERROR, IDS_08_S02, 0, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem()));
		else
			pStatement->setDiagRec(DRIVER_ERROR, IDS_S1_000, sts, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem()), NULL, 
				SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 1,procname);

		return SQL_ERROR;
	}

	switch (srvrCallContext->odbcAPI)
	{
	case SQL_API_SQLPREPARE:
	case SQL_API_SQLEXECDIRECT:
	case SQL_API_SQLPARAMDATA:

	default:
		switch (pStatement->getExceptionNr())
		{	
		case CEE_SUCCESS:
			if (pStatement->getSQLWarning())
				rc = SQL_SUCCESS_WITH_INFO;
			else
				rc = SQL_SUCCESS;
			break;
		default:
			rc = SQL_ERROR;
			break;
		}
		break;
	case SQL_API_SQLEXECUTE:
		switch (pStatement->getExceptionNr())
		{	
		case CEE_SUCCESS:
			if (pStatement->getSQLWarning())
				rc = SQL_SUCCESS_WITH_INFO;
			else
				rc = SQL_SUCCESS;
			break;
		case odbc_SQLSvc_ExecuteRowset_SQLInvalidHandle_exn_:
			rc = SQL_INVALID_HANDLE;
			break;
		case odbc_SQLSvc_ExecuteRowset_SQLStillExecuting_exn_:
			rc = SQL_STILL_EXECUTING;
			break;
		case odbc_SQLSvc_ExecuteRowset_SQLNeedData_exn_:
			rc = SQL_NEED_DATA;
			break;
		case SQL_NO_DATA_FOUND:
			rc = SQL_NO_DATA;
			break;
		default:
			rc = SQL_ERROR;
			break;
		}
		break;
	case SQL_API_SQLEXTENDEDFETCH:
	case SQL_API_SQLFETCH:
		switch (pStatement->getExceptionNr())
		{	
		case CEE_SUCCESS:
			if (pStatement->getNumberOfRows() == 0 )
				rc = SQL_ERROR;
			else
			{
				if (pStatement->getSQLWarning())
					rc = SQL_SUCCESS_WITH_INFO;
				else
					rc = SQL_SUCCESS;
			}
			break;
		case odbc_SQLSvc_FetchRowset_SQLInvalidHandle_exn_:
			rc = SQL_INVALID_HANDLE;
			break;
		case odbc_SQLSvc_FetchRowset_SQLStillExecuting_exn_:
			rc = SQL_STILL_EXECUTING;
			break;
		case odbc_SQLSvc_FetchRowset_SQLNoDataFound_exn_:
			rc = SQL_NO_DATA;
			break;
		case SQL_NO_DATA_FOUND:
			rc = SQL_NO_DATA;
			break;
		default:
			rc = SQL_ERROR;
			break;
		}
		break;
	case SQL_API_SQLTABLES:
	case SQL_API_SQLCOLUMNS:
	case SQL_API_SQLPRIMARYKEYS:
	case SQL_API_SQLSPECIALCOLUMNS:
	case SQL_API_SQLSTATISTICS:
	case SQL_API_SQLGETTYPEINFO:
	case SQL_API_SQLPROCEDURES:
	case SQL_API_SQLPROCEDURECOLUMNS:
		switch (pStatement->getExceptionNr())
		{	
		case CEE_SUCCESS:
			if (pStatement->getSQLWarning())
				rc = SQL_SUCCESS_WITH_INFO;
			else
				rc = SQL_SUCCESS;
			break;
		case odbc_SQLSvc_GetSQLCatalogs_SQLInvalidHandle_exn_:
			rc = SQL_INVALID_HANDLE;
			break;
		default:
			rc = SQL_ERROR;
			break;
		}
		break;
	}
	return rc;
}



SQLRETURN SQLEXECUTE_(SRVR_CALL_CONTEXT *srvrCallContext)
{

	CEE_status			sts;
	SQLRETURN			rc = SQL_ERROR;
	IDL_long			stmtHandle = 0;
	IDL_long			returnCode = 0;
	IDL_long_long		rowsAffected = 0;
	IDL_long            sqlStmtQueryType = 0;
	IDL_long            estimatedCost = 0;
	BYTE				*sqlWarningOrError = 0;


	SQL_DataValue_def	outputDataValue = {0,0};   // output from execute and execute with rowset
	SQLItemDescList_def outputItemDescList = {0,0};// output from execdirect and execdirect rowset
	SQLValueList_def    inputValueList = {0,0};    // input for ExecuteCall

	IDL_unsigned_long rowLength=0;

	IDL_unsigned_long	Index=0;
	IDL_unsigned_long	Items=0;


	SQLINTEGER		SQLCharset=0;
	SQLSMALLINT		SQLDataType=0; 
	SQLSMALLINT		SQLDatetimeCode=0;
	SQLINTEGER		SQLOctetLength=0; 
	SQLSMALLINT		SQLPrecision=0;
	SQLSMALLINT		SQLUnsigned=0;
	SQLINTEGER		SQLMaxLength=0; 
	SQLSMALLINT		SQLNullable=0;

	IDL_short		SQLDataInd=0;
	IDL_short		SQLDataLength=0;
	BYTE*			outputColumns = NULL; // ExecDirect on the server calls Prepare/Execute


	CStmt *pStatement = (CStmt *)srvrCallContext->sqlHandle;
	CConnect *pConnection = pStatement->getConnectHandle();

	sqlStmtQueryType = pStatement->getStmtQueryType();

	SQLUINTEGER maxLength = pStatement->getMaxLength();
	SQL_DataValue_def	*DataValue = &outputDataValue;
	SQLUINTEGER maxRowCount = srvrCallContext->u.executeParams.rowCount;

	stmtHandle = pStatement->getStmtHandle();

	

	if (pConnection->m_srvrTCPIPSystem->odbcAPI == SRVR_API_SQLEXECDIRECT)
	{

		sts = odbc_SQLDrvr_Execute_pst_(srvrCallContext,
						srvrCallContext->dialogueId,
						srvrCallContext->u.sendSQLcommandParams.asyncEnable,
						srvrCallContext->u.sendSQLcommandParams.holdableCursor,
						srvrCallContext->u.sendSQLcommandParams.queryTimeout,
						0, // input rowCount
						srvrCallContext->maxRowsetSize,
						srvrCallContext->u.sendSQLcommandParams.sqlStmtType,
						stmtHandle,
						srvrCallContext->u.sendSQLcommandParams.sqlStmtType, // statement type
						(char *)srvrCallContext->u.sendSQLcommandParams.sqlString,
						srvrCallContext->u.sendSQLcommandParams.cursorName,
						srvrCallContext->u.sendSQLcommandParams.stmtLabel,
						srvrCallContext->u.sendSQLcommandParams.stmtName, // stmt explain label
                       		                &inputValueList,
			                        &returnCode,
			                        sqlWarningOrError,
			                        &rowsAffected,
						&sqlStmtQueryType,
                                        	&estimatedCost,
			                        &outputDataValue,
                                        	&outputItemDescList,
						outputColumns // for ExecDirect (prepare returns this)
						);
		if (sts == CEE_SUCCESS)
		{
			pStatement->setStmtQueryType(sqlStmtQueryType);
			switch (returnCode) 
			{
				case SQL_SUCCESS:
				case SQL_SUCCESS_WITH_INFO:
					pStatement->setDiagRowCount(rowsAffected, -1);
					pStatement->setRowCount(rowsAffected);

					if (sqlStmtQueryType == SQL_SELECT_UNIQUE && rowsAffected > 0)
					{
						if (!pStatement->setFetchOutputPerf(DataValue, rowsAffected))
						{
							pStatement->setDiagRec(DRIVER_ERROR, IDS_S1_000_06);
							pStatement->clearFetchDataValue();
							break;
						}
						

						pStatement->setRowAddress(0,(DataValue->_buffer)); // only one row

					} // Unique Select

					if( sqlStmtQueryType == SQL_CALL_NO_RESULT_SETS ||
						sqlStmtQueryType == SQL_CALL_WITH_RESULT_SETS ||
						sqlStmtQueryType == SQL_SP_RESULT_SET )
					{

						pStatement->setDescRec(&pStatement->m_spjInputParamDesc, &pStatement->m_spjOutputParamDesc);

						if(DataValue->_buffer != NULL)
						{
							pStatement->setFetchOutputPerf(DataValue, rowsAffected);
							pStatement->setRowAddress(0,(DataValue->_buffer));
							pStatement->setExecuteCallOutputs();
						}

						if(pStatement->m_spjNumResultSets > 0)
						{
							// set the descriptors for the first result set. We'll set the other ones when the
							// user calls SQLMoreResults
							pStatement->setDescRec(&pStatement->m_spjInputParamDesc, &pStatement->m_spjResultSets[0].spjOutputItemDesc);
						}

					} // Execute Call With Outut Params

					if(returnCode == SQL_SUCCESS_WITH_INFO)
						pStatement->setDiagRec(sqlWarningOrError, returnCode);

					break;
				case SQL_STILL_EXECUTING:
					break;
				case SQL_NO_DATA_FOUND:
					break;
				case SQL_ERROR:
					pStatement->setDiagRec(sqlWarningOrError, returnCode);
					break;
				case SQL_INVALID_HANDLE:
					pStatement->setDiagRec(SERVER_ERROR, IDS_08_S01);
					break;
				default:
					pStatement->sendCDInfo(returnCode);
					pStatement->setDiagRec(returnCode, EXECDIRECT_PROCNAME, pStatement->getSrvrIdentity());
					break;
			} // switch on returnCode
		} // sts == CEE_SUCCESS

		rc = (SQLRETURN)returnCode;

	} // SRVR_API_SQLEXECDIRECT



    if (pConnection->m_srvrTCPIPSystem->odbcAPI == SRVR_API_SQLEXECUTE2)
	{

		sts = odbc_SQLDrvr_Execute_pst_(srvrCallContext,
						srvrCallContext->dialogueId,
						srvrCallContext->u.executeParams.asyncEnable,
						srvrCallContext->u.executeParams.holdableCursor,				
						srvrCallContext->u.executeParams.queryTimeout,
						srvrCallContext->u.executeParams.rowCount, // input rowCount
						srvrCallContext->maxRowsetSize,
						srvrCallContext->u.executeParams.sqlStmtType,
						stmtHandle,
						srvrCallContext->u.executeParams.sqlStmtType, // statement type
						NULL, // sql string
						srvrCallContext->u.executeParams.cursorName,
						NULL, // stmt Label
						NULL, // stmt explain label
                                        	srvrCallContext->u.executeParams.inputValueList,
			                        &returnCode,
			                        sqlWarningOrError,
			                        &rowsAffected,
						&sqlStmtQueryType,
                                        	&estimatedCost,
			                        &outputDataValue,
                                        	&outputItemDescList,
						outputColumns // for ExecDirect (prepare returns this)
						);

		if (sts == CEE_SUCCESS)
		{
			pStatement->setExceptionErrors(returnCode, 0);
			switch (returnCode) 
			{
				case SQL_SUCCESS:
				case SQL_SUCCESS_WITH_INFO:
					
					pStatement->setDiagRowCount(rowsAffected, -1);
					pStatement->setRowCount(rowsAffected);
					sqlStmtQueryType = pStatement->getStmtQueryType();
					if (returnCode != SQL_SUCCESS)
					{
						if( pStatement->rowsetWasFiltered() )
							pStatement->mapSqlRowIdsToDrvrRowIds(sqlWarningOrError);/*AMR */

						pStatement->setDiagRec(sqlWarningOrError, returnCode);
					}
					
					if (sqlStmtQueryType == SQL_SELECT_UNIQUE && rowsAffected > 0)
					{

						if (!pStatement->setFetchOutputPerf(DataValue, rowsAffected))
						{
							pStatement->setDiagRec(DRIVER_ERROR, IDS_S1_000_06);
							pStatement->clearFetchDataValue();
							break;
						}

						pStatement->setRowAddress(0,(DataValue->_buffer)); // only one row
					

					} // Unique Select

					if( sqlStmtQueryType == SQL_CALL_NO_RESULT_SETS ||
						sqlStmtQueryType == SQL_CALL_WITH_RESULT_SETS ||
						sqlStmtQueryType == SQL_SP_RESULT_SET )
					{

						pStatement->setDescRec(&pStatement->m_spjInputParamDesc, &pStatement->m_spjOutputParamDesc);

						if(DataValue->_buffer != NULL)
						{
							pStatement->setFetchOutputPerf(DataValue, rowsAffected);
							pStatement->setRowAddress(0,(DataValue->_buffer));
							pStatement->setExecuteCallOutputs();
						}

						if(pStatement->m_spjNumResultSets > 0)
						{
							// set the descriptors for the first result set. We'll set the other ones when the
							// user calls SQLMoreResults
							pStatement->setDescRec(&pStatement->m_spjInputParamDesc, &pStatement->m_spjResultSets[0].spjOutputItemDesc);
						}

					} // Execute Call With Outut Params

			  		if (pStatement->getStmtType() == TYPE_INSERT_PARAM ||
					    pStatement->getStmtType() == TYPE_UPDATE ||
					    pStatement->getStmtType() == TYPE_DELETE) 
					{
						if (pStatement->rowsetErrorRecovery())
						{
							if (rowsAffected == -1)
								pStatement->setExceptionErrors(odbc_SQLSvc_ExecuteRowset_SQLError_exn_,0);
			//				else
			//					pStatement->setExceptionErrors(returnCode, 0);

							if (returnCode != SQL_SUCCESS)
								pStatement->setRowsetArrayStatus(sqlWarningOrError, rowsAffected);
						}
						else
						{
							pStatement->setExceptionErrors (returnCode, 0);
							if (returnCode != SQL_SUCCESS)  /*AMR*/
							{
								if( pStatement->rowsetWasFiltered() )							
									pStatement->mapSqlRowIdsToDrvrRowIds(sqlWarningOrError);/*AMR */

								pStatement->setRowsetArrayStatus(sqlWarningOrError, rowsAffected);
							}
						}
					}
					break;
				case SQL_STILL_EXECUTING:
					break;
				case SQL_NO_DATA_FOUND:
					break;
				case SQL_ERROR:
					if( pStatement->rowsetWasFiltered() )							
						pStatement->mapSqlRowIdsToDrvrRowIds(sqlWarningOrError);/*AMR*/

					if (pStatement->getStmtType() == TYPE_INSERT_PARAM ||
					    pStatement->getStmtType() == TYPE_UPDATE ||
					    pStatement->getStmtType() == TYPE_DELETE) 
						   pStatement->setRowsetArrayStatus(sqlWarningOrError, -1);

					pStatement->setDiagRec(sqlWarningOrError, returnCode);
					break;
				case SQL_INVALID_HANDLE:
					pStatement->setDiagRec(SERVER_ERROR, IDS_08_S01);
					break;
				default:
					pStatement->sendCDInfo(returnCode);
					pStatement->setDiagRec(returnCode, EXECUTE_PROCNAME, pStatement->getSrvrIdentity());
					break;
			} // switch on returnCode
		} // sts == CEE_SUCCESS

		rc = (SQLRETURN)returnCode;

	} // SRVR_API_SQLEXECUTE2




	if (sts != CEE_SUCCESS)
	{
		if (sts == CEE_INTERNALFAIL)
			pStatement->setDiagRec(DRIVER_ERROR, IDS_EXCEPTION_MSG,0,"SQL SERVER",
				NULL,SQL_ROW_NUMBER_UNKNOWN,SQL_COLUMN_NUMBER_UNKNOWN,2,"Internal Error","SQLEXECUTE_");
		else if (sts == TIMEOUT_EXCEPTION)
			pStatement->setDiagRec(DRIVER_ERROR, IDS_S1_T00, sts, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem())); 
		else if (sts == COMM_LINK_FAIL_EXCEPTION)
			pStatement->setDiagRec(DRIVER_ERROR, IDS_08_S01, sts, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem()));
		else if (sts == TRANSPORT_ERROR)
			pStatement->setDiagRec(DRIVER_ERROR, IDS_08_S02, sts, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem()));
		else
			pStatement->setDiagRec(DRIVER_ERROR, IDS_S1_000, sts, FORMAT_ERROR((long)pStatement->getSrvrTCPIPSystem()), NULL, 
				SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 1,"SQLEXECUTE_ failed");
		return SQL_ERROR;
	}



	//
	// Cleanup
	//
	if (sqlWarningOrError != NULL)
		delete sqlWarningOrError;

	if(outputItemDescList._length > 0)
		delete[] outputItemDescList._buffer;

	pStatement->clearInputValueList();
	
	return rc;

} /* SQLEXECUTE_() */


SQLRETURN EXTRACTLOB(SRVR_CALL_CONTEXT *srvrCallContext)
{
    CEE_status    sts;
    SQLRETURN     rc = SQL_SUCCESS;

    struct odbc_SQLsvc_ExtractLob_exc_ exception_ = { 0, 0, 0 };
    BYTE   *sqlWarningOrError = 0;

    CStmt   * pStatement = (CStmt *)srvrCallContext->sqlHandle;

    sts = odbc_SQLDrvr_ExtractLOB_pst_(
        srvrCallContext,
        srvrCallContext->u.extractLobParams.extractType,
        srvrCallContext->u.extractLobParams.lobHandle,
        srvrCallContext->u.extractLobParams.lobHandleLen,
        srvrCallContext->u.extractLobParams.extractLen,
        &exception_,
        srvrCallContext->u.extractLobParams.extractData
        );

    pStatement->setExceptionErrors(exception_.exception_nr, exception_.exception_detail);
    switch (exception_.exception_nr)
    {
    case CEE_SUCCESS:
        break;
    case odbc_SQLSvc_ExtractLob_SQLError_exn_:
        pStatement->setDiagRec(&exception_.u.SQLError);
        break;
    case odbc_SQLSvc_ExtractLob_InvalidConnection_exn_:
        pStatement->setDiagRec(SERVER_ERROR, IDS_08_S01);
        break;
    case odbc_SQLSvc_ExtractLob_ParamError_exn_:
        pStatement->setDiagRec(SERVER_ERROR, IDS_HY_C00, exception_.exception_nr,
            exception_.u.ParamError.ParamDesc, NULL,
            SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 1, pStatement->getSrvrIdentity());
        break;
    case odbc_SQLSvc_ExtractLob_SQLInvalidhandle_exn_:
        break;
    default:
        pStatement->sendCDInfo(exception_.exception_nr);
        pStatement->setDiagRec(exception_.exception_nr, "EXTRACTLOB", pStatement->getSrvrIdentity());
        break;
    }
    return rc;
}

SQLRETURN UPDATELOB(SRVR_CALL_CONTEXT *srvrCallContext)
{
    CEE_status    sts;
    SQLRETURN     rc = SQL_SUCCESS;

    struct odbc_SQLSvc_UpdateLob_exc_ exception_ = { 0, 0, 0 };
    BYTE   *sqlWarningOrError = 0;

    CStmt   * pStatement = (CStmt *)srvrCallContext->sqlHandle;

    sts = odbc_SQLDrvr_UpdateLob_pst_(
        srvrCallContext,
        srvrCallContext->u.updateLobParams.updateType,
        srvrCallContext->u.updateLobParams.lobHandle,
        srvrCallContext->u.updateLobParams.lobHandleLen,
        srvrCallContext->u.updateLobParams.totalLength,
        srvrCallContext->u.updateLobParams.offset,
        srvrCallContext->u.updateLobParams.pos,
        srvrCallContext->u.updateLobParams.length,
        srvrCallContext->u.updateLobParams.data,
        &exception_
        );

    pStatement->setExceptionErrors(exception_.exception_nr, exception_.exception_detail);
    switch (exception_.exception_nr)
    {
    case CEE_SUCCESS:
        break;
    case odbc_SQLSvc_UpdateLob_SQLError_exn_:
        pStatement->setDiagRec(&exception_.u.SQLError);
        break;
    case odbc_SQLSvc_UpdateLob_InvalidConnect_exn_:
        pStatement->setDiagRec(SERVER_ERROR, IDS_08_S01);
        break;
    case odbc_SQLSvc_UpdateLob_ParamError_exn_:
        pStatement->setDiagRec(SERVER_ERROR, IDS_HY_C00, exception_.exception_nr,
            exception_.u.ParamError.ParamDesc, NULL,
            SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 1, pStatement->getSrvrIdentity());
        break;
    case odbc_SQLSvc_UpdateLob_SQLInvalidhandle_exn_:
        break;
    default:
        pStatement->sendCDInfo(exception_.exception_nr);
        pStatement->setDiagRec(exception_.exception_nr, "UPDATELOB", pStatement->getSrvrIdentity());
        break;
    }
    return rc;
}
