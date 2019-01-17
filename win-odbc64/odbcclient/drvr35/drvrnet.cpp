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
//
//
#include "process.h"
#include "DrvrGlobal.h"
#include "DrvrNet.h"
#include "CConnect.h"
#include "CStmt.h"
#include "DiagFunctions.h"
#include "tdm_odbcDrvMsg.h"

UINT __stdcall ThreadControlProc(LPVOID pParam)
{
	SQLRETURN			rc = SQL_SUCCESS;
	SRVR_CALL_CONTEXT	*srvrCallContext;
	CConnect			*pConnection=NULL;
	CStmt				*pStatement=NULL;
	HCURSOR				hPrevCur = NULL;
	long				intStmtType;
	
	VERSION_def			version;

	srvrCallContext = (SRVR_CALL_CONTEXT *)pParam;
 	gDrvrGlobal.gCEEMessageFilter = FALSE;


	switch (srvrCallContext->odbcAPI)
	{
	case SQL_API_GETOBJREF:
	case SQL_API_SQLCONNECT:
	case SQL_API_SQLDISCONNECT:
	case SQL_API_SQLSETCONNECTATTR:
	case SQL_API_SQLENDTRAN:
		pStatement = NULL;
		pConnection = (CConnect *)srvrCallContext->sqlHandle;
		break;
	case SQL_API_SQLPREPARE:
	case SQL_API_SQLEXECDIRECT:
	case SQL_API_SQLEXECUTE:
	case SQL_API_SQLEXTENDEDFETCH:
	case SQL_API_SQLPARAMDATA:
	case SQL_API_SQLFETCH:
	case SQL_API_SQLFREESTMT:
	case SQL_API_SQLTABLES:
	case SQL_API_SQLCOLUMNS:
	case SQL_API_SQLPRIMARYKEYS:
	case SQL_API_SQLSPECIALCOLUMNS:
	case SQL_API_SQLSTATISTICS:
	case SQL_API_SQLGETTYPEINFO:
	case SQL_API_SQLPROCEDURES:
	case SQL_API_SQLPROCEDURECOLUMNS:
	case SQL_API_SQLCOLUMNPRIVILEGES:
	case SQL_API_SQLTABLEPRIVILEGES:
	case SQL_API_SQLFOREIGNKEYS:
    case SRVR_API_EXTRACTLOB:
    case SRVR_API_UPDATELOB:
		pStatement = (CStmt*)srvrCallContext->sqlHandle;
		pConnection = pStatement->getConnectHandle();
		break;
	default:
		srvrCallContext->sqlHandle->setDiagRec(DRIVER_ERROR, IDS_IM_001);
		return SQL_ERROR;
		break;
	}
	EnterCriticalSection(&pConnection->m_CSTransmision);
	if (gDrvrGlobal.gWaitCursor)
		hPrevCur = SetCursor(gDrvrGlobal.gWaitCursor);


	if (pStatement != NULL )
	{
		pConnection->getVersion(&version, ODBC_SRVR_COMPONENT);
		
		if (srvrCallContext->odbcAPI != SQL_API_SQLFETCH && srvrCallContext->odbcAPI != SQL_API_SQLEXTENDEDFETCH)
			pStatement->setFetchCatalog(FALSE);

		intStmtType = pStatement->m_intStmtType;

		if (srvrCallContext->odbcAPI != SQL_API_SQLFETCH && srvrCallContext->odbcAPI != SQL_API_SQLEXTENDEDFETCH)
			pStatement->deleteValueList();
	}
	else
	{
		pConnection->deleteValueList();
	}

	__try
	{
		switch (srvrCallContext->odbcAPI)
		{
		case SQL_API_SQLPREPARE:
			rc = SQLPREPARE_(srvrCallContext);
			break;

		case SQL_API_SQLEXECDIRECT:
			pConnection->m_srvrTCPIPSystem->odbcAPI = SRVR_API_SQLEXECDIRECT;
			pStatement->setAPIDecision(false);
			rc = SQLEXECUTE_(srvrCallContext);

			break;

		case SQL_API_SQLEXECUTE:
		case SQL_API_SQLPARAMDATA:
			pStatement->setAPIDecision(false);
			pConnection->m_srvrTCPIPSystem->odbcAPI = SRVR_API_SQLEXECUTE2;
			rc = SQLEXECUTE_(srvrCallContext);
			break;

		case SQL_API_SQLFETCH:
        case SQL_API_SQLEXTENDEDFETCH:
			rc = SQLFETCH_(srvrCallContext);
			break;

        case SRVR_API_EXTRACTLOB:
            pConnection->m_srvrTCPIPSystem->odbcAPI = SRVR_API_EXTRACTLOB;
            rc = EXTRACTLOB(srvrCallContext);
            break;

        case SRVR_API_UPDATELOB:
            pConnection->m_srvrTCPIPSystem->odbcAPI = SRVR_API_UPDATELOB;
            rc = UPDATELOB(srvrCallContext);
            break;

		case SQL_API_SQLFREESTMT:
			rc = FREESTATEMENT(srvrCallContext);
			break;

		case SQL_API_SQLENDTRAN:
			rc = ENDTRANSACT(srvrCallContext);
			break;
		case SQL_API_GETOBJREF:
			rc = GETOBJREF(srvrCallContext);
			break;
		case SQL_API_SQLCONNECT:
			rc = INITIALIZE_DIALOG(srvrCallContext);
			break; 
		case SQL_API_SQLDISCONNECT:
			rc = TERMINATE_DIALOG(srvrCallContext);
			break;
		case SQL_API_SQLSETCONNECTATTR:
			rc = SETCONNECT(srvrCallContext);
			break;
		case SQL_API_SQLTABLES:
		case SQL_API_SQLCOLUMNS:
		case SQL_API_SQLPRIMARYKEYS:
		case SQL_API_SQLSPECIALCOLUMNS:
		case SQL_API_SQLSTATISTICS:
		case SQL_API_SQLGETTYPEINFO:
		case SQL_API_SQLPROCEDURES:
		case SQL_API_SQLPROCEDURECOLUMNS:
		case SQL_API_SQLCOLUMNPRIVILEGES:
		case SQL_API_SQLTABLEPRIVILEGES:
		case SQL_API_SQLFOREIGNKEYS:
			pStatement->setFetchCatalog(TRUE);
			rc = SMDCATALOGS(srvrCallContext);
			break;
		default:
			srvrCallContext->sqlHandle->setDiagRec(DRIVER_ERROR, IDS_IM_001);
			rc = SQL_ERROR;
			break;
		}
	} 
	__except ( EXCEPTION_EXECUTE_HANDLER ){
		if (pStatement) ((CHandle*)pStatement)->structExceptionHandling(GetExceptionCode());
		if (pConnection)((CHandle*)pConnection)->structExceptionHandling(GetExceptionCode());
		rc = SQL_ERROR;
	}

	LeaveCriticalSection(&pConnection->m_CSTransmision);
	gDrvrGlobal.gCEEMessageFilter=TRUE;
	if (hPrevCur)
		SetCursor(hPrevCur);

	if((pConnection != NULL) && pConnection->m_IgnoreCancel && pStatement != NULL)
		pStatement->m_CancelCalled = false;

	return rc;
}