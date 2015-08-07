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
#include "drvrglobal.h"
#include "drvrnet.h"
#include "cconnect.h"
#include "cstmt.h"
#include "diagfunctions.h"
#include "mxomsg.h"
#include "drvrglobal.h"
#include "DrvrSrvr.h"


UINT ThreadControlProc(LPVOID pParam)
{
	SQLRETURN			rc = SQL_SUCCESS;
	SRVR_CALL_CONTEXT	*srvrCallContext;
	CConnect			*pConnection=NULL;
	CStmt				*pStatement=NULL;
	long				sqlStmtType;
	long				intStmtType;
	VERSION_def			version;

	srvrCallContext = (SRVR_CALL_CONTEXT *)pParam;

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
		pStatement = (CStmt*)srvrCallContext->sqlHandle;
		pConnection = pStatement->getConnectHandle();
		break;
	default:
		srvrCallContext->sqlHandle->setDiagRec(DRIVER_ERROR, IDS_IM_001);
		return SQL_ERROR;
		break;
	}

#ifdef unixcli
#ifdef ASYNCIO
		#ifdef __DEBUGASYNCIO
		printf("ThreadControlProc: Acquiring Trasnmission Lock for connection %x, statement %x\n",pConnection,pStatement);
		#endif	

		if(pConnection->m_CSTransmision != NULL)
			pConnection->m_CSTransmision->Lock();

		#ifdef __DEBUGASYNCIO
		printf("ThreadControlProc: Acquired Trasnmission Lock for connection %x, statement %x\n",pConnection,pStatement);
		#endif
#endif
#else
	EnterCriticalSection(&pConnection->m_CSTransmision);
#endif
//	if (gDrvrGlobal.gWaitCursor)
//		hPrevCur = SetCursor(gDrvrGlobal.gWaitCursor);

	sqlStmtType = -1;

	if(pStatement != NULL && pStatement->getAsyncEnable() != SQL_ASYNC_ENABLE_ON)
		if(pStatement->getStmtExecState() != STMT_EXECUTION_EXECUTING)
			pStatement->setStmtExecState(STMT_EXECUTION_EXECUTING);

	if (pStatement != NULL )
	{
		pConnection->getVersion(&version, ODBC_SRVR_COMPONENT);
		
		if (srvrCallContext->odbcAPI != SQL_API_SQLFETCH && srvrCallContext->odbcAPI != SQL_API_SQLEXTENDEDFETCH)
			pStatement->setFetchCatalog(FALSE);

		sqlStmtType = pStatement->getStmtType();

		intStmtType = pStatement->m_intStmtType;

		if (srvrCallContext->odbcAPI != SQL_API_SQLFETCH && srvrCallContext->odbcAPI != SQL_API_SQLEXTENDEDFETCH)
			pStatement->deleteValueList();
	}
//	else
//	{
//		pConnection->deleteValueList();
//	}

//	__try
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

	if(pStatement != NULL && pStatement->getAsyncEnable() != SQL_ASYNC_ENABLE_ON)
		pStatement->setStmtExecState(STMT_EXECUTION_NONE);

	{
#ifdef unixcli
#ifdef ASYNCIO
                #ifdef __DEBUGASYNCIO
                printf("ThreadControlProc: Freeing  Trasnmission Lock for connection %x, statement %x\n",pConnection,pStatement);
                #endif

		if(pConnection->m_CSTransmision != NULL)
			pConnection->m_CSTransmision->UnLock();
		#ifdef __DEBUGASYNCIO
		printf("ThreadControlProc: Freed Trasnmission Lock for connection %x, statement %x\n",pConnection,pStatement);
		#endif
#endif
#else
		LeaveCriticalSection(&pConnection->m_CSTransmision);
#endif
	}

	return rc;
}

