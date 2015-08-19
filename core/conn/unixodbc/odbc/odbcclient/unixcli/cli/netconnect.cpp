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
***************************************************************************/
//
// MODULE: NetConnect.cpp
//
//
// PURPOSE: Krypton - Client Module for Connection .
//
//

#include "drvrglobal.h"
#include "odbcsrvrcommon.h"
#include "drvrnet.h"
#include "cconnect.h"
#include "diagfunctions.h"
#include "mxomsg.h"
#include "odbcas_cl.h"
#include "ceecfg.h"
#include "ping.h"
#include "resource.h"
#include "security.h"
#include "DrvrSrvr.h"
#include "odbcas_drvr.h"
#include "odbcs_drvr.h"

SQLRETURN GETOBJREF(SRVR_CALL_CONTEXT *srvrCallContext)
{
	DWORD			dwTimeout = 0;
	DWORD			curTimeout = 0;
	_timeb			time_start;
	_timeb			time_curr;

	SQLRETURN		rc = SQL_SUCCESS;
	CEE_status		sts;
	short			noOfRetries = 0;
	CConnect		*pConnection;
	VERSION_def		version;
	bool			bloop = true;
	int				dwSleep;

	odbcas_ASSvc_GetObjRefHdl_exc_ exception_ = {0,0,0};
	IDL_OBJECT_def		srvrObjRef;
	DIALOGUE_ID_def 	dialogueId = 0;
	SQL_IDENTIFIER_def	dataSource;
    USER_SID_def 		userSid = {0,0};
    VERSION_LIST_def 	versionList = {0,0};
	IDL_long  isoMapping = 0;
	IDL_long  srvrNodeId = -1;
	IDL_long  srvrProcessId = -1;
	long long timestamp = 0;
	const char *_debugFlagForMxosrvrName = NULL;

	IDL_OBJECT_def fwsrvrObjRef;
	char* pTCP;
	char* pIpAddress;
	char* pPortNumber;
	char* pObjectName;
	IDL_OBJECT_def objRef;
	char* pCheckComma;
	char* pCheck;
	char* srvrSegName=NULL;
	char* lasts;
	pConnection = (CConnect *)srvrCallContext->sqlHandle;
        int getObjRefRetryCnt = 0;

	dwTimeout = srvrCallContext->u.connectParams.loginTimeout;

	if (dwTimeout != 0){
		dwSleep = (dwTimeout / 3) * 1000;
		dwSleep = dwSleep > 5000 ? 5000 : dwSleep;
		dwSleep = dwSleep < 1000 ? 1000 : dwSleep;
	}
	else
		dwSleep = 5000;


	_ftime(&time_start);

TryAgain:

	pConnection->clearError();
		
	sts = odbcas_ASSvc_GetObjRefHdl_(NULL,
								srvrCallContext,
								srvrCallContext->u.connectParams.inContext,
								srvrCallContext->u.connectParams.userDesc,
								CORE_SRVR,
								getObjRefRetryCnt,
								&exception_,
				 				srvrObjRef,
								&dialogueId,
				 				dataSource,
								&userSid,
								&versionList,
								&isoMapping,
								&srvrNodeId,
								&srvrProcessId,
								&timestamp);

	if (sts != CEE_SUCCESS)
	{
		if (sts == CEE_INTERNALFAIL)
			pConnection->setDiagRec(ASSOC_SERVER_ERROR, IDS_EXCEPTION_MSG,0,"ASSOCIATION SERVICE",
				NULL,SQL_ROW_NUMBER_UNKNOWN,SQL_COLUMN_NUMBER_UNKNOWN,2,"Internal Error","GETOBJREF");
		else if (sts == TIMEOUT_EXCEPTION)
			pConnection->setDiagRec(DRIVER_ERROR, IDS_S1_T00, 0, FORMAT_ERROR((long)pConnection->m_asTCPIPSystem));
		else if ((sts == COMM_LINK_FAIL_EXCEPTION) || (sts == TRANSPORT_ERROR))
			pConnection->setDiagRec(ASSOC_SERVER_ERROR, IDS_ASSOC_SRVR_NOT_AVAILABLE,-20005, FORMAT_ERROR("ASSOCIATION SERVICE",(long)pConnection->m_asTCPIPSystem),
				NULL,SQL_ROW_NUMBER_UNKNOWN,SQL_COLUMN_NUMBER_UNKNOWN,1,"");
		else
			pConnection->setDiagRec(DRIVER_ERROR, IDS_ASSOC_SRVR_NOT_AVAILABLE, sts, FORMAT_ERROR((long)pConnection->m_asTCPIPSystem), 
				NULL, SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 1,LAST_ERROR_TO_TEXT());

			if(versionList._buffer != NULL)
				delete versionList._buffer;

		return SQL_ERROR;
	}
	
	/* Starts CCF */
	pConnection->setExceptionErrors(exception_.exception_nr, exception_.exception_detail);

	switch (exception_.exception_nr)
	{
	case CEE_SUCCESS:
		if (srvrObjRef[0] == 0)
		{
			pConnection->setExceptionErrors(odbcas_ASSvc_GetObjRefHdl_ASParamError_exn_,0);
			pConnection->setDiagRec(ASSOC_SERVER_ERROR, IDS_PROGRAM_ERROR, exception_.exception_nr,
					"Invalid Object Reference");
			break;
		}

		pConnection->m_srvrTCPIPSystem->setSwap(pConnection->m_asTCPIPSystem->swap());
		// During debugging, it helps to know what server the client is connected to
 		_debugFlagForMxosrvrName = getenv("__DEBUG_MXOSRVR");
		if((_debugFlagForMxosrvrName != NULL) && (strcmp(_debugFlagForMxosrvrName, "TRUE") ==  0))
			printf("Connected mxosrvr is: %s\n", srvrObjRef);
		
		pConnection->setRetSrvrObjRef(srvrObjRef);
		strcpy(fwsrvrObjRef, srvrObjRef);

		strcpy(objRef, srvrObjRef);
#ifndef unixcli
		if ((pTCP = strtok(objRef, ":")) != NULL)
		{
			pCheckComma = strchr( srvrObjRef, ',' );
			if (pCheckComma != NULL)
			{
//
// New object ref tcp:\neo0001.$z123,1.2.3.4/18650:ObjectName
//
				pCheck = strtok(NULL,",");
				if (pCheck != NULL)
				{
					if ((pIpAddress = strtok(NULL, "/"))  != NULL)
					{
						if ((pPortNumber = strtok(NULL, ":")) != NULL)
						{
							if ((pObjectName = strtok(NULL, ":")) != NULL)
							{
								sprintf( fwsrvrObjRef, "%s:%s/%s:%s", pTCP,pIpAddress,pPortNumber,pObjectName);
							}
						}
					}
				}
				srvrSegName = strtok(pCheck, ".");
			}
			else
			{
//
// Old object ref tcp:\neo0001.$z123/18650:ObjectName
//
				strcpy(objRef, srvrObjRef);
				pCheck = objRef + 5;
				if ((pIpAddress = strtok(pCheck, ".")) != NULL)
				{
					strtok_r(NULL, "/", &lasts);
					if ((pPortNumber = strtok(NULL, ":")) != NULL)
					{
						if ((pObjectName = strtok(NULL, ":")) != NULL)
						{
							sprintf( fwsrvrObjRef, "tcp:%s/%s:%s", pIpAddress,pPortNumber,pObjectName);
						}
					}
					srvrSegName = pIpAddress;
				}
			}
		}
#else
		if ((pTCP = strtok_r(objRef, ":", &lasts)) != NULL)
		{
			pCheckComma = strchr( srvrObjRef, ',' );
			if (pCheckComma != NULL)
			{
//
// New object ref tcp:\neo0001.$z123,1.2.3.4/18650:ObjectName
//
				pCheck = strtok_r(NULL,",",&lasts);
				if (pCheck != NULL)
				{
					if ((pIpAddress = strtok_r(NULL, "/",&lasts))  != NULL)
					{
						if ((pPortNumber = strtok_r(NULL, ":", &lasts)) != NULL)
						{
							if ((pObjectName = strtok_r(NULL, ":", &lasts)) != NULL)
							{
								sprintf( fwsrvrObjRef, "%s:%s/%s:%s", pTCP,pIpAddress,pPortNumber,pObjectName);
							}
						}
					}
				}
				srvrSegName = strtok_r(pCheck, ".",&lasts);
			}
			else
			{
//
// Old object ref tcp:\neo0001.$z123/18650:ObjectName
//
				strcpy(objRef, srvrObjRef);
				pCheck = objRef + 5;
				if ((pIpAddress = strtok_r(pCheck, ".", &lasts)) != NULL)
				{
					strtok_r(NULL, "/", &lasts);
					if ((pPortNumber = strtok_r(NULL, ":", &lasts)) != NULL)
					{
						if ((pObjectName = strtok_r(NULL, ":", &lasts)) != NULL)
						{
							sprintf( fwsrvrObjRef, "tcp:%s/%s:%s", pIpAddress,pPortNumber,pObjectName);
						}
					}
					srvrSegName = pIpAddress;
				}
			}
		}
#endif
		pConnection->setGetObjRefHdlOutput(fwsrvrObjRef, dialogueId, dataSource, &userSid, &versionList, isoMapping, srvrNodeId, srvrProcessId, timestamp);

		break;
	case odbcas_ASSvc_GetObjRefHdl_ASParamError_exn_ :
	//  Added check to see if No CPUs or Invalid CPU list are set for MXCS server to start then return 
	// error back to client as param error then parse the error in client to return proper error message.
		pCheck = strstr(exception_.u.ASParamError.ErrorText, "CPU" );
		if (pCheck == NULL)
			pConnection->setDiagRec(ASSOC_SERVER_ERROR, IDS_PROGRAM_ERROR, exception_.exception_nr,
				exception_.u.ASParamError.ErrorText);
		else
		{

			pConnection->setDiagRec(ASSOC_SERVER_ERROR,IDS_NO_SRVR_AVAILABLE, 0,
				exception_.u.ASNotAvailable.ErrorText);
		}
		break;	
	case odbcas_ASSvc_GetObjRefHdl_LogonUserFailure_exn_ :
		PVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				exception_.u.LogonUserFailure.errorCode,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL);
		pConnection->setDiagRec(ASSOC_SERVER_ERROR, IDS_UNABLE_TO_LOGON,
			exception_.u.LogonUserFailure.errorCode,
			 (char *)lpMsgBuf);
		LocalFree((HLOCAL)lpMsgBuf);
		break;
	case odbcas_ASSvc_GetObjRefHdl_ASNotAvailable_exn_ :
		pConnection->setDiagRec(ASSOC_SERVER_ERROR,IDS_ASSOC_SRVR_NOT_AVAILABLE, 0,
			exception_.u.ASNotAvailable.ErrorText);
		break;
	case odbcas_ASSvc_GetObjRefHdl_DSNotAvailable_exn_:
		pConnection->setDiagRec(ASSOC_SERVER_ERROR,IDS_DS_NOT_AVAILABLE,0L,	
			exception_.u.DSNotAvailable.ErrorText);
		break;
	case odbcas_ASSvc_GetObjRefHdl_PortNotAvailable_exn_:
		pConnection->setDiagRec(ASSOC_SERVER_ERROR, IDS_PORT_NOT_AVAILABLE);
		break;
	case odbcas_ASSvc_GetObjRefHdl_InvalidUser_exn_:
		pConnection->setDiagRec(ASSOC_SERVER_ERROR, IDS_28_000);
		break;
	case odbcas_ASSvc_GetObjRefHdl_ASTimeout_exn_ :
		pConnection->setDiagRec(ASSOC_SERVER_ERROR, IDS_S1_T00);
		break;
	case odbcas_ASSvc_GetObjRefHdl_ASNoSrvrHdl_exn_ :
		pConnection->setDiagRec(ASSOC_SERVER_ERROR,IDS_NO_SRVR_AVAILABLE, 0,
			exception_.u.ASNotAvailable.ErrorText);
		break;
	case odbcas_ASSvc_GetObjRefHdl_ASTryAgain_exn_ :
	case -27:
		break;
	case -29:
		break;
	default:
		pConnection->setDiagRec(exception_.exception_nr, GET_OBJECT_REF_PROCNAME,
				pConnection->getSrvrIdentity());
		break;
	}	/* Ends CCF */

	
	switch (pConnection->getExceptionNr())
	{
	case CEE_SUCCESS:
		pConnection->getVersion(&version, NSK_ODBCAS_COMPONENT);
		if (version.componentId != NSK_ODBCAS_COMPONENT ||
			version.majorVersion != NSK_VERSION_MAJOR_1 ||
			version.minorVersion != NSK_VERSION_MINOR_0 )
		{
			char tmp[100];
			IDL_short majorVersion = version.majorVersion;
			IDL_short minorVersion = version.minorVersion;
			majorVersion = (majorVersion > 0)? majorVersion - 1: majorVersion;
			sprintf(tmp,"Incorrect AS version: %d.%d, expected: %d.%d",majorVersion,minorVersion,NSK_VERSION_MAJOR_1 - 1,NSK_VERSION_MINOR_0);
			pConnection->setDiagRec(ASSOC_SERVER_ERROR, IDS_S1_000, 0, tmp,NULL, 0, 0, 1, tmp);
			rc = SQL_ERROR;
		}
		break;
	case odbcas_ASSvc_GetObjRefHdl_ASTryAgain_exn_:
		_ftime(&time_curr);
		curTimeout = (long)(time_curr.time - time_start.time);

		if ((dwTimeout != 0) && (dwTimeout <= curTimeout))
		{
			pConnection->setDiagRec(ASSOC_SERVER_ERROR, IDS_S1_T00);
			rc = SQL_ERROR;
			break;
		}
		else
		{
			if(dwTimeout != 0)
				srvrCallContext->u.connectParams.loginTimeout = dwTimeout - curTimeout;
			Sleep(dwSleep); 
                        getObjRefRetryCnt++;
			goto TryAgain;
		}
		break;
	default:
		rc = SQL_ERROR;
		break;
	}

	if(versionList._buffer != NULL)
		delete versionList._buffer;

	return rc;
}


SQLRETURN INITIALIZE_DIALOG(SRVR_CALL_CONTEXT *srvrCallContext)
{
	CEE_status							sts;
	CConnect							*pConnection;
	SQLRETURN							rc = SQL_SUCCESS;
	// RAJANI - for password expiry
	CONNECT_FIELD_ITEMS					connectFieldItems;
	bool								bChangePwd = false;

    odbc_SQLSvc_InitializeDialogue_exc_ exception_ = {0,0,0};
    OUT_CONNECTION_CONTEXT_def outContext;

	pConnection = (CConnect *)srvrCallContext->sqlHandle;
	short retry = 0;

	retryInitializeDialogue:

	do
	{
		sts = odbc_SQLSvc_InitializeDialogue_(NULL,
									srvrCallContext,
									srvrCallContext->u.connectParams.userDesc,
									srvrCallContext->u.connectParams.inContext,
									srvrCallContext->dialogueId,
									&exception_,
									&outContext);
		if (sts == CEE_SUCCESS) break;
		Sleep(100);
	}
	while (sts == COMM_LINK_FAIL_EXCEPTION && retry++ < 3);

	if (sts != CEE_SUCCESS)
	{
		if (sts == CEE_INTERNALFAIL)
			pConnection->setDiagRec(DRIVER_ERROR, IDS_EXCEPTION_MSG,0,"SQL SERVER",
				NULL,SQL_ROW_NUMBER_UNKNOWN,SQL_COLUMN_NUMBER_UNKNOWN,2,"Internal Error","INITIALIZE_DIALOG");
		else if (sts == TIMEOUT_EXCEPTION)
			pConnection->setDiagRec(DRIVER_ERROR, IDS_S1_T00, 0, FORMAT_ERROR((long)pConnection->m_srvrTCPIPSystem));
		else if (sts == COMM_LINK_FAIL_EXCEPTION)
			pConnection->setDiagRec(SERVER_ERROR, IDS_08_S01, 0, FORMAT_ERROR((long)pConnection->m_srvrTCPIPSystem));
		else if (sts == TRANSPORT_ERROR)
			pConnection->setDiagRec(SERVER_ERROR, IDS_08_S02, 0, FORMAT_ERROR((long)pConnection->m_srvrTCPIPSystem));
		else
			pConnection->setDiagRec(DRIVER_ERROR, IDS_S1_000, sts, FORMAT_ERROR((long)pConnection->m_srvrTCPIPSystem), 
				NULL, SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 1,"INITIALIZE_DIALOG failed");
		return SQL_ERROR;
	}

	// Start CCF
	pConnection->setExceptionErrors(exception_.exception_nr, exception_.exception_detail);
	
	switch ( exception_.exception_nr) 
	{
	case CEE_SUCCESS:
		pConnection->setOutContext(&outContext);
		break;
	case odbc_SQLSvc_InitializeDialogue_SQLError_exn_:
#ifndef MXOSS
		if (exception_.exception_detail == 4415) // SECMXO_NO_CERTIFICATE
		{
			try{
				if (pConnection->m_SecPwd->switchCertificate()==SQL_SUCCESS) // successfully switched to the new certificate
					pConnection->setRetryEncryption();
				else // there is no certificate to switch to
					pConnection->setDiagRec(&exception_.u.SQLError);
			}
			catch (SecurityException se) {
				rc= se.getErrCode();
				pConnection->setSecurityError(rc, se.getSQLState(), se.getMsg());
			}
		}
		else
#endif
			pConnection->setDiagRec(&exception_.u.SQLError);
		break;
	case odbc_SQLSvc_InitializeDialogue_InvalidUser_exn_:
		if (outContext.outContextOptions1 & OUTCONTEXT_OPT1_DOWNLOAD_CERTIFICATE)
		{
			try {
				pConnection->m_SecPwd->switchCertificate(outContext.outContextOptionString, outContext.outContextOptionStringLen);
			} 
			catch (SecurityException se) {
				rc = se.getErrCode();
				pConnection->setSecurityError(rc, se.getSQLState(), se.getMsg());
			}
			if(rc == SQL_SUCCESS)
				pConnection->setRetryEncryption();
		}
		else if (srvrCallContext->u.connectParams.userDesc->userName != NULL && srvrCallContext->u.connectParams.userDesc->password._buffer != NULL)
			pConnection->setDiagRec(SQLMX_ERROR, IDS_28_000, SQL_INVALID_USER_CODE);
		break;
	case odbc_SQLSvc_InitializeDialogue_ParamError_exn_:
		pConnection->setDiagRec(SQLMX_ERROR, IDS_28_000, exception_.exception_nr,
			(char*)(LPCTSTR)exception_.u.ParamError.ParamDesc, NULL, 
			SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 1, INITIALIZE_DIALOG_PROCNAME);
		break;
	case odbc_SQLSvc_InitializeDialogue_InvalidConnection_exn_:
		pConnection->setDiagRec(SERVER_ERROR, IDS_08_004_01);
		break;
	case odbc_SQLSvc_InitializeDialogue_SQLInvalidHandle_exn_:
		break;
	case odbc_SQLSvc_InitializeDialogue_SQLNeedData_exn_:
		break;
	default:
		pConnection->setDiagRec(exception_.exception_nr, INITIALIZE_DIALOG_PROCNAME,
				pConnection->getSrvrIdentity());
		break;
	}
	// End CCF
	
	switch (pConnection->getExceptionNr())
	{
		case CEE_SUCCESS:
			break;
		case odbc_SQLSvc_InitializeDialogue_SQLInvalidHandle_exn_:
			rc = SQL_INVALID_HANDLE;
			break;
		case odbc_SQLSvc_InitializeDialogue_SQLError_exn_:
			/*
			 * RAJANI KANTH
			 * The following is the code, asusming the error numbers as
			 * Password to expire 	: 8857
			 * Grace Period			: 8837
			 * RAJANI END
			 */
#ifdef unixcli
		    if(pConnection->getExceptionDetail() == SQL_PASSWORD_EXPIRING || 
		       pConnection->getExceptionDetail() == SQL_PASSWORD_GRACEPERIOD)
			{
			   pConnection->setOutContext(&outContext);
			   rc = SQL_SUCCESS_WITH_INFO;
			}
		    else
		       rc = SQL_ERROR;
		    break;
#else
			if(pConnection->getExceptionDetail() == SQL_PASSWORD_EXPIRING && !bChangePwd )
			{
				if (! pConnection->getChangePasswordStatus() )
				{
					int resp = MessageBox(gDrvrGlobal.ghWindow," Password about to Expire - Do you want to change? ","HP ODBC",MB_YESNO);
					if ( resp == IDYES )
					{

						strcpy(connectFieldItems.catalog,pConnection->getCurrentCatalog());
						strcpy(connectFieldItems.schema,pConnection->getCurrentSchema());
						strcpy(connectFieldItems.server,pConnection->getSrvrDSName());
						strcpy(connectFieldItems.loginId,srvrCallContext->u.connectParams.userDesc->userName);
						if (srvrCallContext->u.connectParams.userDesc->password._buffer != NULL && srvrCallContext->u.connectParams.userDesc->password._length > 0)
						{
							memcpy (connectFieldItems.password, srvrCallContext->u.connectParams.userDesc->password._buffer, srvrCallContext->u.connectParams.userDesc->password._length);
							connectFieldItems.password[srvrCallContext->u.connectParams.userDesc->password._length] = '\0';
							Decryption( connectFieldItems.password, connectFieldItems.password, srvrCallContext->u.connectParams.userDesc->password._length );
						}
																
						int DialogRetCode = DialogBoxParam(gDrvrGlobal.gModuleHandle,
										MAKEINTRESOURCE(IDD_CHANGE_PWD),
										gDrvrGlobal.ghWindow, ChangePwdProc, (long)&connectFieldItems);
						if ( strchr(connectFieldItems.password,',')!=NULL )
						{
							pConnection->clearError();
							pConnection->getUserDesc(connectFieldItems.loginId,connectFieldItems.password,srvrCallContext->u.connectParams.userDesc);
							goto retryInitializeDialogue;
						}
						else
						{
							bChangePwd = true;
							rc = SQL_SUCCESS_WITH_INFO;
						}
					}
					else
					{
							bChangePwd = true;
							rc = SQL_SUCCESS_WITH_INFO;
					}
				}
				else
				{
					bChangePwd = true;
					rc = SQL_SUCCESS_WITH_INFO;
				}
			}
			else if (pConnection->getExceptionDetail() == SQL_PASSWORD_GRACEPERIOD && !bChangePwd )
			{
				int resp = MessageBox(gDrvrGlobal.ghWindow," Password Expired - In Grace Period ","HP ODBC",MB_YESNO);
				if ( resp == IDYES )
				{
					strcpy(connectFieldItems.catalog,pConnection->getCurrentCatalog());
					strcpy(connectFieldItems.schema,pConnection->getCurrentSchema());
					strcpy(connectFieldItems.server,pConnection->getSrvrDSName());
					strcpy(connectFieldItems.loginId,(const char*)srvrCallContext->u.connectParams.userDesc->userName);
						
					int DialogRetCode = DialogBoxParam(gDrvrGlobal.gModuleHandle,
										MAKEINTRESOURCE(IDD_CHANGE_PWD),
										gDrvrGlobal.ghWindow, ChangePwdProc, (long)&connectFieldItems);
						if ( strchr(connectFieldItems.password,',')!=NULL )
						{
							pConnection->clearError();
							pConnection->getUserDesc(connectFieldItems.loginId,connectFieldItems.password,srvrCallContext->u.connectParams.userDesc);
							goto retryInitializeDialogue;
							
						}
						else
						{
							bChangePwd = true;
							rc = SQL_SUCCESS_WITH_INFO;
						}
				}
				else 
				{
						bChangePwd = true;
						rc = SQL_SUCCESS_WITH_INFO;
				}
			}
			else
			rc = SQL_ERROR;
			break;
#endif /* unixcli */
		case odbc_SQLSvc_InitializeDialogue_SQLNeedData_exn_:
			rc = SQL_NEED_DATA;
			break;
		default:
			rc = SQL_ERROR;
			break;
	}

	//
	// cleanup
	//
	if(exception_.exception_nr == odbc_SQLSvc_InitializeDialogue_SQLError_exn_ &&
	   exception_.u.SQLError.errorList._length > 0 )
  	      delete[] exception_.u.SQLError.errorList._buffer;

	if(exception_.exception_nr == odbc_SQLSvc_InitializeDialogue_InvalidUser_exn_ &&
	   exception_.u.InvalidUser.errorList._length > 0 )
  	      delete[] exception_.u.InvalidUser.errorList._buffer;

	if(outContext.versionList._length > 0)
		delete [] outContext.versionList._buffer;

	if((outContext.outContextOptions1 & OUTCONTEXT_OPT1_ROLENAME || outContext.outContextOptions1 & OUTCONTEXT_OPT1_DOWNLOAD_CERTIFICATE)
		&& outContext.outContextOptionStringLen > 0)
		delete [] outContext.outContextOptionString;

	return rc;

} // INITIALIZE_DIALOG()


SQLRETURN TERMINATE_DIALOG(SRVR_CALL_CONTEXT *srvrCallContext)
{
	CEE_status	sts;
	long timerTimeout;

	odbc_SQLSvc_TerminateDialogue_exc_ exception_;

	CConnect *pConnection = (CConnect *)srvrCallContext->sqlHandle;
	timerTimeout = srvrCallContext->connectionTimeout > 10 ? srvrCallContext->connectionTimeout : 10;
	
	sts = odbc_SQLSvc_TerminateDialogue_(NULL,
		srvrCallContext,
		srvrCallContext->dialogueId,
		&exception_);

	if (sts != CEE_SUCCESS)
	{
		if (sts == CEE_INTERNALFAIL)
		{
			pConnection->setDiagRec(DRIVER_ERROR, IDS_EXCEPTION_MSG,0,"SQL SERVER",
				NULL,SQL_ROW_NUMBER_UNKNOWN,SQL_COLUMN_NUMBER_UNKNOWN,2,"Internal Error","TERMINATE_DIALOG");
			return SQL_ERROR;
		}
		pConnection->setDiagRec(DRIVER_ERROR, IDS_S1_000, sts, FORMAT_ERROR((long)pConnection->m_srvrTCPIPSystem),
			NULL, SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 1, "TERMINATE_DIALOG failed");
		return SQL_SUCCESS_WITH_INFO;
	}

	// Start CCF
	pConnection->setExceptionErrors(exception_.exception_nr, exception_.exception_detail);
	switch (exception_.exception_nr) {
	case CEE_SUCCESS:
		pConnection->resetGetObjRefHdlOutput();
		break;
	case odbc_SQLSvc_TerminateDialogue_SQLError_exn_:
		if (exception_.exception_detail == 25000)
			pConnection->setDiagRec(DRIVER_ERROR, IDS_25_000);
		else
			pConnection->setDiagRec(&exception_.u.SQLError);
		break;
	case odbc_SQLSvc_TerminateDialogue_ParamError_exn_:
		pConnection->setDiagRec(SERVER_ERROR, IDS_PROGRAM_ERROR, exception_.exception_nr, 
							exception_.u.ParamError.ParamDesc);
		break;
	case odbc_SQLSvc_TerminateDialogue_InvalidConnection_exn_:
		pConnection->sendCDInfo(exception_.exception_nr);
		pConnection->setDiagRec(SERVER_ERROR, IDS_08_S01, -20005);
		break;
	default:
		pConnection->sendCDInfo(exception_.exception_nr);
		pConnection->setDiagRec(exception_.exception_nr, TERMINATE_DIALOG_PROCNAME,
				pConnection->getSrvrIdentity());
		break;
	}
	if (exception_.exception_detail != 25000)
		CloseIO (pConnection->m_srvrTCPIPSystem);
	// Close CCF

	//
	// cleanup
	//
	if(exception_.exception_nr == odbc_SQLSvc_TerminateDialogue_SQLError_exn_ &&
	   exception_.u.SQLError.errorList._length > 0 )
  	      delete[] exception_.u.SQLError.errorList._buffer;



	switch (pConnection->getExceptionNr())
	{
		case CEE_SUCCESS:
			return SQL_SUCCESS;
		default:
	// if transaction is open return SQL_ERROR
			if (pConnection->getExceptionDetail() == 25000)
				return -25000;
			else
	// Any other errors treat them as if it has been disconnected
				return SQL_SUCCESS_WITH_INFO;
	}

} // TERMINATE_DIALOG()

SQLRETURN SETCONNECT (SRVR_CALL_CONTEXT *srvrCallContext)
{

	CEE_status	sts;
    struct odbc_SQLSvc_SetConnectionOption_exc_ exception_ = {0,0,0};
    ERROR_DESC_LIST_def sqlWarning = {0,0};
	
	UDWORD_P timerTimeout = srvrCallContext->connectionTimeout;
	CConnect *pConnection = (CConnect *)srvrCallContext->sqlHandle;

	sts = odbc_SQLDrvr_SetConnectionOption_pst_(
								srvrCallContext,
								srvrCallContext->dialogueId,
								srvrCallContext->u.setConnectParams.attribute,
								srvrCallContext->u.setConnectParams.valueNum,
								(IDL_char *)srvrCallContext->u.setConnectParams.valueStr,
								&exception_,
								&sqlWarning);


	if (sts != CEE_SUCCESS)
	{
		if (sts == CEE_INTERNALFAIL)
			pConnection->setDiagRec(DRIVER_ERROR, IDS_EXCEPTION_MSG,0,"SQL SERVER",
				NULL,SQL_ROW_NUMBER_UNKNOWN,SQL_COLUMN_NUMBER_UNKNOWN,2,"Internal Error","SETCONNECT");
		else if (sts == TIMEOUT_EXCEPTION)
				pConnection->setDiagRec(DRIVER_ERROR, IDS_S1_T00, 0, FORMAT_ERROR((long)pConnection->m_srvrTCPIPSystem));
		else if (sts == COMM_LINK_FAIL_EXCEPTION)
				pConnection->setDiagRec(DRIVER_ERROR, IDS_08_S01, 0, FORMAT_ERROR((long)pConnection->m_srvrTCPIPSystem));
		else if (sts == TRANSPORT_ERROR)
				pConnection->setDiagRec(DRIVER_ERROR, IDS_08_S02, 0, FORMAT_ERROR((long)pConnection->m_srvrTCPIPSystem));
		else
			pConnection->setDiagRec(DRIVER_ERROR, IDS_S1_000, sts, FORMAT_ERROR((long)pConnection->m_srvrTCPIPSystem), 
				NULL, SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 1,"SETCONNECT failed");
		return SQL_ERROR;
	}

    odbc_SQLSvc_SetConnectionOption_ccf_ (srvrCallContext,
                                          &exception_,
                                          &sqlWarning);


	// 
	// cleanup
	//

	if(exception_.exception_nr == odbc_SQLSvc_SetConnectionOption_SQLError_exn_ &&
	   exception_.u.SQLError.errorList._length > 0 )
  	      delete[] exception_.u.SQLError.errorList._buffer;

	if(sqlWarning._length > 0)
		delete[] sqlWarning._buffer;



	switch (pConnection->getExceptionNr())
	{
		case CEE_SUCCESS:
			return SQL_SUCCESS;
		case odbc_SQLSvc_SetConnectionOption_SQLInvalidHandle_exn_:
			return SQL_INVALID_HANDLE;
		default:
			return SQL_ERROR;
		}

} // SETCONNECT()

// Call Completion function for operation 'odbc_SQLSvc_SetConnectionOption'
extern "C" void odbc_SQLSvc_SetConnectionOption_ccf_ (
	CEE_tag_def cmptag_
  , const struct odbc_SQLSvc_SetConnectionOption_exc_ *exception_
  , const ERROR_DESC_LIST_def *sqlWarning
  )
{
	
	SRVR_CALL_CONTEXT	*srvrCallContext = (SRVR_CALL_CONTEXT *)cmptag_;
	CConnect			*pConnection = (CConnect *)srvrCallContext->sqlHandle;

	pConnection->setExceptionErrors(exception_->exception_nr, exception_->exception_detail);
	switch (exception_->exception_nr)
	{
	case CEE_SUCCESS:
		if (sqlWarning->_length > 0)
			pConnection->setDiagRec(sqlWarning);
		break;
	case odbc_SQLSvc_SetConnectionOption_SQLError_exn_:
		pConnection->setDiagRec(&exception_->u.SQLError);
		break;
	case odbc_SQLSvc_SetConnectionOption_ParamError_exn_:
		pConnection->setDiagRec(SERVER_ERROR, IDS_PROGRAM_ERROR, exception_->exception_nr, 
				exception_->u.ParamError.ParamDesc, NULL, 
				SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 1, pConnection->getSrvrIdentity());
		break;
	case odbc_SQLSvc_SetConnectionOption_SQLInvalidHandle_exn_: //This error code is used tempororily
		pConnection->setDiagRec(SERVER_ERROR, IDS_HY_000, 0, " SQL_ATTR_AUTOCOMMIT can not be changed when transaction is in progress");
		break;
	case odbc_SQLSvc_SetConnectionOption_InvalidConnection_exn_:
		pConnection->setDiagRec(SERVER_ERROR, IDS_08_S01, -20005);
		break;
	default:
		pConnection->sendCDInfo(exception_->exception_nr);
		pConnection->setDiagRec(exception_->exception_nr, SETCONNECT_PROCNAME,
					pConnection->getSrvrIdentity());
		break;
	}
}

SQLRETURN ENDTRANSACT(SRVR_CALL_CONTEXT *srvrCallContext)
{
	CEE_status	sts;

    struct odbc_SQLSvc_EndTransaction_exc_ exception_ = {0,0,0};
	ERROR_DESC_LIST_def sqlWarning = {0,0};
	
	UDWORD_P timerTimeout = srvrCallContext->connectionTimeout;
	CConnect *pConnection = (CConnect *)srvrCallContext->sqlHandle;
	
	if (timerTimeout != 0)
		timerTimeout = (timerTimeout < 180)? 180: timerTimeout;

	sts = odbc_SQLDrvr_EndTransaction_pst_(
									srvrCallContext,
									srvrCallContext->dialogueId,
									srvrCallContext->u.completionType,
                                    &exception_,
                                    &sqlWarning);


	if (sts != CEE_SUCCESS)
	{
		if (sts == CEE_INTERNALFAIL)
			pConnection->setDiagRec(DRIVER_ERROR, IDS_EXCEPTION_MSG,0,"SQL SERVER",
				NULL,SQL_ROW_NUMBER_UNKNOWN,SQL_COLUMN_NUMBER_UNKNOWN,2,"Internal Error","ENDTRANSACT");
		else if (sts == TIMEOUT_EXCEPTION)
				pConnection->setDiagRec(DRIVER_ERROR, IDS_S1_T00, 0, FORMAT_ERROR((long)pConnection->m_srvrTCPIPSystem));
		else if (sts == COMM_LINK_FAIL_EXCEPTION)
				pConnection->setDiagRec(DRIVER_ERROR, IDS_08_S01, 0, FORMAT_ERROR((long)pConnection->m_srvrTCPIPSystem));
		else if (sts == TRANSPORT_ERROR)
				pConnection->setDiagRec(DRIVER_ERROR, IDS_08_S02, 0, FORMAT_ERROR((long)pConnection->m_srvrTCPIPSystem));
		else
			pConnection->setDiagRec(DRIVER_ERROR, IDS_S1_000, sts, FORMAT_ERROR((long)pConnection->m_srvrTCPIPSystem), 
				NULL, SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 1,"ENDTRANSACT failed");
		return SQL_ERROR;
	}

    odbc_SQLSvc_EndTransaction_ccf_ (srvrCallContext,
                                     &exception_,
                                     &sqlWarning);

	// 
	// cleanup
	//

	if(exception_.exception_nr == odbc_SQLSvc_EndTransaction_SQLError_exn_ &&
	   exception_.u.SQLError.errorList._length > 0 )
  	      delete[] exception_.u.SQLError.errorList._buffer;

	if(sqlWarning._length > 0)
		delete[] sqlWarning._buffer;


	switch (pConnection->getExceptionNr())
	{
		case CEE_SUCCESS:
			return SQL_SUCCESS;
		case odbc_SQLSvc_EndTransaction_SQLInvalidHandle_exn_:
			return SQL_INVALID_HANDLE;
		default:
			return SQL_ERROR;
	}
} // ENDTRANSACT()

// Call Completion function operation 'odbc_SQLSvc_EndTransaction'
 
extern "C" void odbc_SQLSvc_EndTransaction_ccf_ (
    CEE_tag_def cmptag_
  , const struct odbc_SQLSvc_EndTransaction_exc_ *exception_
  , const ERROR_DESC_LIST_def *sqlWarning
  )
{

	SRVR_CALL_CONTEXT	*srvrCallContext = (SRVR_CALL_CONTEXT *)cmptag_;
	CConnect			*pConnection = (CConnect *)srvrCallContext->sqlHandle;

	pConnection->setExceptionErrors(exception_->exception_nr, exception_->exception_detail);
	switch (exception_->exception_nr)
	{
		case CEE_SUCCESS:
			if (sqlWarning->_length > 0)
				pConnection->setDiagRec(sqlWarning);
			break;
		case odbc_SQLSvc_EndTransaction_SQLError_exn_:
			pConnection->setDiagRec(&exception_->u.SQLError);
			break;
		case odbc_SQLSvc_EndTransaction_ParamError_exn_:
			pConnection->setDiagRec(SERVER_ERROR, IDS_PROGRAM_ERROR, exception_->exception_nr, 
					exception_->u.ParamError.ParamDesc, NULL, 
					SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 1, pConnection->getSrvrIdentity());
			break;
		case odbc_SQLSvc_EndTransaction_SQLInvalidHandle_exn_:
			break;
		case odbc_SQLSvc_EndTransaction_InvalidConnection_exn_:
			pConnection->setDiagRec(SERVER_ERROR, IDS_08_S01, -20005);
			break;
		case odbc_SQLSvc_EndTransaction_TransactionError_exn_:
			char tmpNumBuffer[16];
			itoa (exception_->exception_detail, tmpNumBuffer, 10);
			pConnection->setDiagRec(SERVER_ERROR, IDS_TRANSACTION_ERROR, exception_->exception_nr,
						tmpNumBuffer, NULL, SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 1, pConnection->getSrvrIdentity());
			break;
		default:
			pConnection->sendCDInfo(exception_->exception_nr);
			pConnection->setDiagRec(exception_->exception_nr, ENDTRANSACT_PROCNAME,
					pConnection->getSrvrIdentity());
			break;
	}

} // odbc_SQLSvc_EndTransaction_ccf_()




