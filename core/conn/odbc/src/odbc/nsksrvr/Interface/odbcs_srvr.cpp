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


#include "ceercv.h"

#include <platform_ndcs.h>
#include <platform_utils.h>
#include "Transport.h"
#include "marshaling.h"
#include "FileSystemSrvr.h"
#include "TCPIPSystemSrvr.h"
#include "odbcCommon.h"
#include "odbcs_srvr.h"

extern void logError( short Code, short Severity, short Operation );
extern char errStrBuf1[], errStrBuf2[], errStrBuf3[], errStrBuf4[], errStrBuf5[];
extern void terminateThreads(int status);

#include "sql.h"
#include "sqlext.h"

#include "tdm_odbcSrvrMsg.h"
#include "DrvrSrvr.h"
#include "Global.h"

#include "ceecfg.h"
#include "odbcCommon.h"
#include "odbc_sv.h"
#include "odbcas_cl.h"
#include "SrvrConnect.h"
#include "odbcs_srvr_res.h"


using namespace SRVR;

extern SRVR_GLOBAL_Def *srvrGlobal;


// Added for exit on SQL un-recoverable errors

extern Int32 sqlErrorExit[];
extern short errorIndex;
const short MAX_EXIT_ERRORS = 10;

// Note that for -2034 the exit error processing only gets triggered for FS error 31 and not for others
Int32 exitErrors[MAX_EXIT_ERRORS] = { -8700, -8570, -8571, -8816, -8817, -2015, -2013, -2008, -2034, -8951 };
bool isExitError( Int32 error )
{
	for( int i = 0; i < MAX_EXIT_ERRORS; i++ )
	{
		if( error == exitErrors[i] )
			return true;
	}

	return false;
}

void DISPATCH_PROCDEATH_SMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ CEERCV_SystemMessage_exc_ *exception_
  , /* In    */ const FS_Receiveinfo_Type *receiveInfo
  , /* In    */ const CEERCV_SystemMessage_request_seq_ *request
  , /* Out   */ IDL_short *error
  , /* Out   */ CEERCV_SystemMessage_reply_seq_ *reply
)
{
	SRVRTRACE_ENTER(FILE_IIOM+3);
	CFSystemSrvr* pnode;
	zsys_ddl_smsg_procdeath_def* procdeath;
	procdeath = (zsys_ddl_smsg_procdeath_def*)&request->_buffer[0];

	if ((pnode = GTransport.m_FSystemSrvr_list->find_nodeByProcessHandle((TPT_PTR() )&procdeath->z_phandle)) != NULL)
	{
		exception_->exception_nr = 0;
		GTransport.m_FSystemSrvr_list->del_nodeByProcessHandle((TPT_PTR() )&procdeath->z_phandle);
	}

        if(memcmp(&srvrGlobal->nskASProcessInfo.pHandle,&procdeath->z_phandle,sizeof(PROCESS_HANDLE_def)) == 0)
        {
           if(srvrGlobal->srvrState == SRVR_CONNECTED)
              srvrGlobal->stopTypeFlag = STOP_WHEN_DISCONNECTED;
           else
              exitServerProcess();
        }
	SRVRTRACE_EXIT(FILE_IIOM+3);
}

//LCOV_EXCL_START
void DISPATCH_CPUDOWN_SMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ CEERCV_SystemMessage_exc_ *exception_
  , /* In    */ const FS_Receiveinfo_Type *receiveInfo
  , /* In    */ const CEERCV_SystemMessage_request_seq_ *request
  , /* Out   */ IDL_short *error
  , /* Out   */ CEERCV_SystemMessage_reply_seq_ *reply
)
{
	SRVRTRACE_ENTER(FILE_IIOM+4);
	zsys_ddl_smsg_cpudown_def* cpudown;
	IDL_short cpunumber;
	CFSystemSrvr* pnode;

	cpudown = (zsys_ddl_smsg_cpudown_def* )&request->_buffer[0];
	cpunumber = (IDL_short)cpudown->z_cpunumber;

	if (cpunumber == srvrGlobal->WmsNid){
		exitServerProcess();
	}
	srvrGlobal->bConfigurationChanged = true;
	if((pnode=GTransport.m_FSystemSrvr_list->find_nodeByCpu(cpunumber)) != NULL)
	{
  		GTransport.m_FSystemSrvr_list->del_nodeByCpu((IDL_short)cpudown->z_cpunumber);
	}
	SRVRTRACE_EXIT(FILE_IIOM+4);
}

void DISPATCH_CPUUP_SMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ CEERCV_SystemMessage_exc_ *exception_
  , /* In    */ const FS_Receiveinfo_Type *receiveInfo
  , /* In    */ const CEERCV_SystemMessage_request_seq_ *request
  , /* Out   */ IDL_short *error
  , /* Out   */ CEERCV_SystemMessage_reply_seq_ *reply
)
{
}
//LCOV_EXCL_STOP

void DISPATCH_OPEN_SMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ CEERCV_SystemMessage_exc_ *exception_
  , /* In    */ const FS_Receiveinfo_Type *receiveInfo
  , /* In    */ const CEERCV_SystemMessage_request_seq_ *request
  , /* Out   */ IDL_short *error
  , /* Out   */ CEERCV_SystemMessage_reply_seq_ *reply
)
{
	SRVRTRACE_ENTER(FILE_IIOM+5);
	if (GTransport.m_FSystemSrvr_list->ins_node(receiveInfo, call_id_) != NULL)
	{
		exception_->exception_nr = 0;
	}
	SRVRTRACE_EXIT(FILE_IIOM+5);
}

void DISPATCH_CLOSE_SMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ CEERCV_SystemMessage_exc_ *exception_
  , /* In    */ const FS_Receiveinfo_Type *receiveInfo
  , /* In    */ const CEERCV_SystemMessage_request_seq_ *request
  , /* Out   */ IDL_short *error
  , /* Out   */ CEERCV_SystemMessage_reply_seq_ *reply
)
{
	SRVRTRACE_ENTER(FILE_IIOM+6);
	CFSystemSrvr* pnode;
	short nodeId;
	short processId;

	if ((pnode = GTransport.m_FSystemSrvr_list->find_node(receiveInfo)) != NULL)
	{
		exception_->exception_nr = 0;
		GTransport.m_FSystemSrvr_list->del_node(receiveInfo);
	}
	SRVRTRACE_EXIT(FILE_IIOM+6);
}

void DISPATCH_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ CEERCV_IOMessage_exc_ *exception_
  , /* In    */ const FS_Receiveinfo_Type *receiveInfo
  , /* In    */ IDL_short dialogInfo
  , /* In    */ const CEERCV_IOMessage_request_seq_ *request
  , /* Out   */ IDL_short *error
  , /* Out   */ CEERCV_IOMessage_reply_seq_ *reply
  ,	short operation_id
)
{
	SRVRTRACE_ENTER(FILE_IIOM+2);
	switch(operation_id)
	{
	case SRVR_API_STOPSRVR:
		STOPSRVR_IOMessage(objtag_, call_id_);
		break;
	case SRVR_API_ENABLETRACE:
		ENABLETRACE_IOMessage(objtag_, call_id_);
		break;
	case SRVR_API_DISABLETRACE:
		DISABLETRACE_IOMessage(objtag_, call_id_);
		break;
	case SRVR_API_ENABLE_SERVER_STATISTICS:
		ENABLESTATISTICS_IOMessage(objtag_, call_id_);
		break;
	case SRVR_API_DISABLE_SERVER_STATISTICS:
		DISABLESTATISTICS_IOMessage(objtag_, call_id_);
		break;
	case SRVR_API_UPDATE_SERVER_CONTEXT:
		UPDATECONTEXT_IOMessage(objtag_, call_id_);
		break;
	case SRVR_API_MONITORCALL:
		MONITORCALL_IOMessage(objtag_, call_id_);
		break;
	default:
		break;
	}
	SRVRTRACE_EXIT(FILE_IIOM+2);

//LCOV_EXCL_START
	// Exit on an un-recoverable SQL error
	// The sqlErrorExit array and errorIndex will be set in SqlWrapper.cpp
	// whenever SQL returns an error.
	// *** The below code is also duplicated in DISPATCH_TCPIPRequest() so any changes
	// below should also be reflected in that method.
	if( errorIndex > 0 ) {
		for( int i = 0; i < errorIndex; i++ ) {
			if( isExitError(sqlErrorExit[i]) )
			{
				char tmpString[100];
				sprintf(tmpString, "SQL returned an unrecoverable fatal error (%d). Server Exiting.", sqlErrorExit[i]);
				SendEventMsg(MSG_ODBC_NSK_ERROR
											, EVENTLOG_ERROR_TYPE
											, GetCurrentProcessId()
											, ODBCMX_SERVER
											, srvrGlobal->srvrObjRef
											, 1
											, tmpString);
      			terminateThreads(sqlErrorExit[i]);
				exit( sqlErrorExit[i] );
			}
			sqlErrorExit[i] = 0;
		}
	}
	errorIndex = 0;
//LCOV_EXCL_STOP
}

void
SQLCONNECT_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  )
{

	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;
	CEE_status retcode;

	char *curptr;

	CONNECTION_CONTEXT_def inContext;
	USER_DESC_def userDesc;
	DIALOGUE_ID_def dialogueId;

	VERSION_def version[4];
	VERSION_def* versionPtr = &version[0];

	IDL_long inputPosition = 0;

	IDL_long		datasourceLength = 0;     // includes null terminator
	IDL_long		catalogLength = 0;        // includes null terminator
	IDL_long	 	schemaLength = 0;         // includes null terminator
	IDL_long		locationLength = 0;       // includes null terminator
	IDL_long		userRoleLength = 0;       // includes null terminator
	IDL_long		computerNameLength = 0;   // includes null terminator
	IDL_long		windowTextLength = 0;     // includes null terminator
	IDL_long        connectOptionsLength = 0; // includes null terminator
	IDL_long		sessionNameLength = 0;	  // includes null terminator
	IDL_long		clientUserNameLength = 0; // includes null terminator
	IDL_long		maxCopyLen;

	IDL_unsigned_long userSidLength;
	IDL_long domainNameLength;
	IDL_long userNameLength;
	IDL_long passwordLength;

	curptr = pnode->r_buffer();


	// Copy values

	// Copy 1st Parameter

	// copy userDesc Type
	userDesc.userDescType = *(IDL_long *)(curptr + inputPosition);
   /*
    *  TBD after finalizing security arch
	*/
//    userDesc.userDescType = AUTHENTICATED_USER_TYPE;
	inputPosition += sizeof(userDesc.userDescType);

	// copy userSidLength
	userDesc.userSid._length = *(IDL_unsigned_long *)(curptr + inputPosition);
	inputPosition += sizeof(userDesc.userSid._length);


	// copy userSid
	if (userDesc.userSid._length > 0)
	{
		userDesc.userSid._buffer = (IDL_octet *)(curptr + inputPosition);
		inputPosition += userDesc.userSid._length;
	}
	else
		userDesc.userSid._buffer = NULL;


	// copy domainNameLength
	domainNameLength = *(IDL_long *)(curptr + inputPosition);
	inputPosition += sizeof(domainNameLength);
	if (domainNameLength > 0)
	{
		userDesc.domainName = (IDL_char *)(curptr + inputPosition);
		inputPosition += domainNameLength;
	}
	else
		userDesc.domainName = NULL;

	// copy userNameLength
	userNameLength = *(IDL_long *)(curptr + inputPosition);
	inputPosition += sizeof(userNameLength);
	if (userNameLength > 0)
	{
		userDesc.userName = (IDL_char *)(curptr + inputPosition);
		inputPosition += userNameLength;
	}
	else
		userDesc.userName = NULL;

	// copy passwordLength
	passwordLength = *(IDL_unsigned_long *)(curptr + inputPosition);
	inputPosition += sizeof(passwordLength);
	if (passwordLength > 0)
	{
		userDesc.password._buffer = (IDL_octet *)(curptr + inputPosition);
		inputPosition += passwordLength;
		userDesc.password._length = passwordLength - 1; // The authentication functions expect the non-null terminated length
	}
	else
	{
		userDesc.password._buffer = NULL;
		userDesc.password._length = 0;
	}


	// Copy 2nd Parameter
	datasourceLength = *(IDL_long *)(curptr + inputPosition);
	inputPosition += sizeof(datasourceLength);
	if (datasourceLength > 0)
	{
		maxCopyLen = _min(sizeof(inContext.datasource),datasourceLength);
		strncpy(inContext.datasource,(curptr+inputPosition),maxCopyLen);
		inContext.datasource[maxCopyLen-1] = '\0';
		inputPosition += datasourceLength;
	}
	else
		inContext.datasource[0] = '\0';

	catalogLength = *(IDL_long *)(curptr + inputPosition);
	inputPosition += sizeof(catalogLength);
	if (catalogLength > 0)
	{
		maxCopyLen = _min(sizeof(inContext.catalog),catalogLength);
		strncpy(inContext.catalog,(curptr+inputPosition),maxCopyLen);
		inContext.catalog[maxCopyLen-1] = '\0';
		inputPosition += catalogLength;
	}
	else
		inContext.catalog[0] = '\0';

	schemaLength = *(IDL_long *)(curptr + inputPosition);
	inputPosition += sizeof(schemaLength);
	if (schemaLength > 0)
	{
		maxCopyLen = _min(sizeof(inContext.schema),schemaLength);
		strncpy(inContext.schema, (curptr + inputPosition), maxCopyLen);
		inContext.schema[maxCopyLen-1] = '\0';
		inputPosition += schemaLength;
	}
	else
		inContext.schema[0] = '\0';

	locationLength = *(IDL_long *)(curptr + inputPosition);
	inputPosition += sizeof(locationLength);
	if (locationLength > 0)
	{
		maxCopyLen = _min(sizeof(inContext.location),locationLength);
		strncpy(inContext.location,(curptr + inputPosition),maxCopyLen);
		inContext.location[maxCopyLen-1] = '\0';
		inputPosition += locationLength;
	}
	else
		inContext.location[0] = '\0';

	userRoleLength = *(IDL_long *)(curptr + inputPosition);
	inputPosition += sizeof(userRoleLength);
	if (userRoleLength > 0)
	{
		maxCopyLen = _min(sizeof(inContext.userRole),userRoleLength);
		strncpy(inContext.userRole, (curptr + inputPosition), maxCopyLen);
		inContext.userRole[maxCopyLen-1] = '\0';
		inputPosition += userRoleLength;
	}
	else
		inContext.userRole[0] = '\0';

	// copy accessMode
	inContext.accessMode = *(IDL_short *)(curptr+inputPosition);
	inputPosition += sizeof(inContext.accessMode);

	// copy autoCommit
	inContext.autoCommit = *(IDL_short *)(curptr+inputPosition);
	inputPosition += sizeof(inContext.autoCommit);

	// copy queryTimeoutSec
	inContext.queryTimeoutSec = *(IDL_long *)(curptr+inputPosition);
	inputPosition += sizeof(inContext.queryTimeoutSec);

	// copy idleTimeoutSec
	inContext.idleTimeoutSec = *(IDL_long *)(curptr+inputPosition);
	inputPosition += sizeof(inContext.idleTimeoutSec);

	// copy loginTimeoutSec
	inContext.loginTimeoutSec = *(IDL_long *)(curptr+inputPosition);
	inputPosition += sizeof(inContext.loginTimeoutSec);

	// copy txnIsolationLevel
	inContext.txnIsolationLevel = *(IDL_short *)(curptr+inputPosition);
	inputPosition += sizeof(inContext.txnIsolationLevel);

	//copy rowSetSize
	inContext.rowSetSize = *(IDL_short *)(curptr+inputPosition);
	inputPosition += sizeof(inContext.rowSetSize);

	// copy diagnosticFlag
	inContext.diagnosticFlag = *(IDL_short *)(curptr+inputPosition);
	inputPosition += sizeof(inContext.diagnosticFlag);

	// copy processId
	inContext.processId = *(IDL_unsigned_long *)(curptr+inputPosition);
	inputPosition += sizeof(inContext.processId);

	// copy computerName
	computerNameLength = *(IDL_long *)(curptr + inputPosition);
	inputPosition += sizeof(computerNameLength);
	if (computerNameLength > 0)
	{
		maxCopyLen = _min(sizeof(inContext.computerName),computerNameLength);
		strncpy(inContext.computerName, (curptr + inputPosition), maxCopyLen);
		inContext.computerName[maxCopyLen-1] = '\0';
		inputPosition += computerNameLength;
	}
	else
		inContext.computerName[0] = '\0';

	// copy windowTextLength
	windowTextLength = *(IDL_long *)(curptr + inputPosition);
	inputPosition += sizeof(windowTextLength);
	if (windowTextLength > 0)
	{
		inContext.windowText = (IDL_char *)(curptr+inputPosition);
		inputPosition += windowTextLength;
	}
	else
		inContext.windowText = NULL;


	// copy ctxACP
	inContext.ctxACP = *(IDL_unsigned_long *)(curptr + inputPosition);
	inputPosition += sizeof(inContext.ctxACP);

	// copy ctxDataLang
	inContext.ctxDataLang = *(IDL_unsigned_long *)(curptr + inputPosition);
	inputPosition += sizeof(inContext.ctxDataLang);

	// copy ctxErrorLang
	inContext.ctxErrorLang = *(IDL_unsigned_long *)(curptr + inputPosition);
	inputPosition += sizeof(inContext.ctxErrorLang);

	// copy ctxCtrlInferNCHAR
	inContext.ctxCtrlInferNCHAR = *(IDL_short *)(curptr + inputPosition);
	inputPosition += sizeof(inContext.ctxCtrlInferNCHAR);

	// copy cpuToUse
	inContext.cpuToUse = *(IDL_short *)(curptr + inputPosition);
	inputPosition += sizeof(inContext.cpuToUse);

	// copy cpuToUseEnd
	inContext.cpuToUseEnd = *(IDL_short *)(curptr + inputPosition);
	inputPosition += sizeof(inContext.cpuToUseEnd);


	// copy connectOptions
	connectOptionsLength = *(IDL_long *)(curptr + inputPosition);
	inputPosition += sizeof(connectOptionsLength);
	if (connectOptionsLength > 0)
	{
		inContext.connectOptions = (IDL_char *)(curptr+inputPosition);
		inputPosition += connectOptionsLength;
	}
	else
		inContext.connectOptions = NULL;

	// copy versionList Length
	inContext.clientVersionList._length = *(IDL_unsigned_long *)(curptr + inputPosition);
	inputPosition += sizeof(inContext.clientVersionList._length);

	if(inContext.clientVersionList._length > 0)
	{

		sts = CEE_TMP_ALLOCATE(call_id_, sizeof(VERSION_def) * inContext.clientVersionList._length, (void **)&inContext.clientVersionList._buffer);

		if(sts != CEE_SUCCESS)
		{
//LCOV_EXCL_START
		   strcpy( errStrBuf2, "odbcs_srvr.cpp");
		   strcpy( errStrBuf3, "SQLCONNECT_IOMessage");
		   strcpy( errStrBuf4, "CEE_TMP_ALLOCATE");
		   sprintf( errStrBuf5, "Failed to get <%d> bytes", sizeof(VERSION_def) * inContext.clientVersionList._length);
		   logError( NO_MEMORY, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
//LCOV_EXCL_STOP
		}

		memset(inContext.clientVersionList._buffer, 0, inContext.clientVersionList._length*sizeof(VERSION_def));
		versionPtr = inContext.clientVersionList._buffer;

		for (int i=0; i < inContext.clientVersionList._length; i++)
		{
			// copy componentId
			versionPtr->componentId = *(IDL_short *)(curptr + inputPosition);
			inputPosition += sizeof(versionPtr->componentId);

			// copy majorVersion
			versionPtr->majorVersion = *(IDL_short *)(curptr + inputPosition);
			inputPosition += sizeof(versionPtr->majorVersion);

			// copy minorVersion
			versionPtr->minorVersion = *(IDL_short *)(curptr + inputPosition);
			inputPosition += sizeof(versionPtr->minorVersion);

			// copy buildId
			versionPtr->buildId = *(IDL_unsigned_long *)(curptr + inputPosition);
			inputPosition += sizeof(versionPtr->buildId);

			// Get the next versionlist values
			versionPtr++;

		}
	}

 	// Copy 3rd Parameter
	// copy dailogueId
	dialogueId = *(IDL_long *)(curptr + inputPosition);
	inputPosition += sizeof(dialogueId);

    inContext.inContextOptions1 = *(IDL_unsigned_long *)(curptr + inputPosition);
	inputPosition += sizeof(inContext.inContextOptions1);

    inContext.inContextOptions2 = *(IDL_unsigned_long *)(curptr + inputPosition);
	inputPosition += sizeof(inContext.inContextOptions2);


	inContext.sessionName[0] = '\0';
	if(inContext.inContextOptions1 & INCONTEXT_OPT1_SESSIONNAME)
	{
		sessionNameLength = *(IDL_long *)(curptr + inputPosition);
		inputPosition += sizeof(sessionNameLength);

		maxCopyLen = _min(sizeof(inContext.sessionName),sessionNameLength);
		if(maxCopyLen > 0)
		{
			strncpy(inContext.sessionName,(IDL_char *)(curptr+inputPosition),maxCopyLen);
			inContext.sessionName[maxCopyLen -1] = '\0';
			inputPosition += sessionNameLength;
		}

	}

	if(inContext.inContextOptions1 & INCONTEXT_OPT1_CLIENT_USERNAME)
	{
		clientUserNameLength = *(IDL_long *)(curptr + inputPosition);
		inputPosition += sizeof(clientUserNameLength);
		if (clientUserNameLength > 0)
		{
			inContext.clientUserName = (IDL_char *)(curptr + inputPosition);
			inputPosition += clientUserNameLength;
		}
		else
			inContext.clientUserName = NULL;

	}
	else
		inContext.clientUserName = NULL;


	odbc_SQLSvc_InitializeDialogue_ame_(
		  objtag_
		, call_id_
		, &userDesc
		, &inContext
		, dialogueId
	  );
}

void
SQLDISCONNECT_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  )
{
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;
	CEE_status retcode;

	char *curptr;

	DIALOGUE_ID_def dialogueId;

	curptr = pnode->r_buffer();

	// Copy 1st Parameter
	// copy dailogueId
	dialogueId = *(IDL_long *)(curptr);

	odbc_SQLSvc_TerminateDialogue_ame_(
		  objtag_
		, call_id_
		, dialogueId
	  );
}

void
SQLSETCONNECTATTR_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  )
{
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;
	CEE_status retcode;

	IDL_char	*curptr;
	IDL_long inputPosition = 0;

	DIALOGUE_ID_def dialogueId;
	IDL_short  connectionOption;
	IDL_long   optionValueNum;
    IDL_long   optionValueStrLen = 0;
	IDL_string optionValueStr = NULL;

	curptr = pnode->r_buffer();

	dialogueId = *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(dialogueId);

	connectionOption = *(IDL_short*)(curptr+inputPosition);
	inputPosition += sizeof(connectionOption);

	optionValueNum	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(optionValueNum);

	optionValueStrLen = *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(optionValueStrLen);

	if(optionValueStrLen > 0)
	{
		optionValueStr = curptr+inputPosition;
		inputPosition += optionValueStrLen;
	}


	odbc_SQLSrvr_SetConnectionOption_ame_(
			objtag_
		  , call_id_
		  , dialogueId
		  , connectionOption
		  , optionValueNum
		  , optionValueStr);

} // SQLSETCONNECTATTR_IOMessage()

void
SQLENDTRAN_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  )
{

	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;
	CEE_status retcode;

	IDL_char	*curptr;
	IDL_long inputPosition = 0;

	DIALOGUE_ID_def dialogueId;
	IDL_unsigned_short transactionOpt;

	curptr = pnode->r_buffer();

	dialogueId = *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(dialogueId);

	transactionOpt = *(IDL_unsigned_short*)(curptr+inputPosition);
	inputPosition += sizeof(transactionOpt);

	odbc_SQLSrvr_EndTransaction_ame_(
			objtag_
		  , call_id_
		  , dialogueId
		  , transactionOpt);

} // SQLENDTRAN_IOMessage()

void
SQLPREPARE_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  )
{
	CInterface* pnode = (CInterface*)objtag_;

	IDL_char   *curptr = NULL;

	DIALOGUE_ID_def dialogueId = 0;
	IDL_long      sqlAsyncEnable = 0;
	IDL_long      queryTimeout = 0;
	IDL_short     stmtType = 0;
	IDL_long      sqlStmtType = 0;
	IDL_long      stmtLength = 0;
	IDL_char      *stmtLabel = NULL;
	IDL_long      stmtLabelCharset = 0;
	IDL_long      cursorLength = 0;
	IDL_string    cursorName = NULL;
	IDL_long      cursorCharset = 0;
	IDL_long      moduleNameLength = 0;
	IDL_char      *moduleName = NULL;
	IDL_long      moduleCharset = 0;
	IDL_long_long moduleTimestamp = 0;
	IDL_long      sqlStringLength = 0;
	IDL_string    sqlString = NULL;
	IDL_long      sqlStringCharset = 0;
	IDL_long      setStmtOptionsLength = 0;
	IDL_string    setStmtOptions = NULL;
    IDL_long      stmtExplainLabelLength = 0;
	IDL_string    stmtExplainLabel = NULL;
	IDL_long      maxRowsetSize = 0;
	IDL_long	  transactionIDLength = 0; // JDBC is the only one that will use this to join a transaction
	IDL_long_long transactionID = 0;     // JDBC is the only one that will use this to join a transaction
	IDL_long      holdableCursor = SQL_NONHOLDABLE; // default
	IDL_short	  *extTransId = NULL;
	IDL_short	ix;
	IDL_char	*temp = NULL;
	bool		all_zero=true;

	IDL_long inputPosition = 0;

	curptr = pnode->r_buffer();

	dialogueId = *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(dialogueId);

	// to support SAP holdable cursor, the driver overloads this field with the value of the holdable cursor
	// because currently sqlAsyncEnable is not used.
	//sqlAsyncEnable	= *(IDL_long*)(curptr+inputPosition);
	holdableCursor	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(sqlAsyncEnable);

	queryTimeout	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(queryTimeout);

	stmtType		= *(IDL_short*)(curptr+inputPosition);
	inputPosition += sizeof(stmtType);

	sqlStmtType		= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(sqlStmtType);

	stmtLength		= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(stmtLength);
	if (stmtLength > 0)
	{
		stmtLabel = curptr+inputPosition;
		inputPosition += stmtLength;
		stmtLabelCharset = *(IDL_long*)(curptr+inputPosition);
		inputPosition += sizeof(stmtLabelCharset);
	}

	cursorLength	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(cursorLength);
	if (cursorLength > 0)
	{
		cursorName = curptr+inputPosition;
		inputPosition += cursorLength;
		cursorCharset = *(IDL_long*)(curptr+inputPosition);
		inputPosition += sizeof(cursorCharset);
	}

	moduleNameLength	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(moduleNameLength);
	if (moduleNameLength > 0)
	{
		moduleName = curptr+inputPosition;
		inputPosition += moduleNameLength;
		moduleCharset = *(IDL_long*)(curptr+inputPosition);
		inputPosition += sizeof(moduleCharset);
		moduleTimestamp = *(IDL_long_long*)(curptr+inputPosition);
		inputPosition += sizeof(moduleTimestamp);
	}

	sqlStringLength	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(sqlStringLength);
	if (sqlStringLength > 0)
	{
		sqlString = curptr+inputPosition;
		inputPosition += sqlStringLength;
		sqlStringCharset	= *(IDL_long*)(curptr+inputPosition);
		inputPosition += sizeof(sqlStringCharset);
	}

	setStmtOptionsLength	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(setStmtOptionsLength);
	if (setStmtOptionsLength > 0)
	{
		setStmtOptions = curptr+inputPosition;
		inputPosition += setStmtOptionsLength;
	}

	stmtExplainLabelLength	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(stmtExplainLabelLength);
	if (stmtExplainLabelLength > 0)
	{
		stmtExplainLabel = curptr+inputPosition;
		inputPosition += stmtExplainLabelLength;
	}

	maxRowsetSize  = *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(maxRowsetSize);

	transactionIDLength = *(IDL_unsigned_long*)(curptr+inputPosition);
	inputPosition += sizeof(transactionIDLength);

	if(transactionIDLength > 0)
	{
//LCOV_EXCL_START
		if (transactionIDLength == 17){
			extTransId = (IDL_short*)(curptr+inputPosition);
			temp = (IDL_char*)(curptr+inputPosition);
			inputPosition += transactionIDLength;
			// need to check the extTransId, if it contains all 0 (17 bytes), change the address to NULL
			// this will prevent TMF_JOIN_EXT_ to be called at the odbc_SQLSrvr_..._ame_
			for (ix=0; ix<transactionIDLength; ix++)
			{
				if (*temp != 0) {
					all_zero=false;
					break;
				}
				temp++;
			}
			if (all_zero)
				extTransId = NULL;
		}
		else{
                   if(transactionIDLength == 5)
			transactionID = *(IDL_long*)(curptr+inputPosition);
                   if(transactionIDLength == 9)
			transactionID = *(IDL_long_long*)(curptr+inputPosition);
		   inputPosition += transactionIDLength;
		}
//LCOV_EXCL_STOP
	}


	odbc_SQLSrvr_Prepare_ame_(
			  objtag_
			, call_id_
			, dialogueId
			, sqlAsyncEnable
			, queryTimeout
			, stmtType
			, sqlStmtType
			, stmtLength
			, stmtLabel
			, stmtLabelCharset
			, cursorLength
			, cursorName
			, cursorCharset
			, moduleNameLength
			, moduleName
			, moduleCharset
			, moduleTimestamp
			, sqlStringLength
			, sqlString
			, sqlStringCharset
			, setStmtOptionsLength
			, setStmtOptions
            , stmtExplainLabelLength
			, stmtExplainLabel
			, maxRowsetSize
			, transactionID
			, extTransId
			, holdableCursor
		  );
} /* SQLPREPARE_IOMessage() */

void
SQLFETCH_IOMessage(
    /* In    */       CEE_tag_def     objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ IDL_short operation_id
                   )
{
  CInterface    *pnode = (CInterface*)objtag_;

  IDL_char		*curptr;

  DIALOGUE_ID_def dialogueId           = 0;
  IDL_long        sqlAsyncEnable       = 0;
  IDL_long        queryTimeout         = 0;
  Long 			stmtHandle           = 0;
  IDL_long 		stmtHandleKey           = 0;
  IDL_long        stmtLength           = 0;
  IDL_string      stmtLabel            = NULL;
  IDL_long        stmtLabelCharset     = 0;
  IDL_unsigned_long_long   maxRowCnt            = 0;
  IDL_unsigned_long_long   maxRowLen            = 0;
  IDL_long        cursorLength         = 0;
  IDL_string      cursorName           = NULL;
  IDL_long        cursorCharset        = 0;
  IDL_long        setStmtOptionsLength = 0;
  IDL_string      setStmtOptions       = NULL;


  IDL_long inputPosition = 0;

  curptr = pnode->r_buffer();

  dialogueId     = *(IDL_long*)(curptr+inputPosition);
  inputPosition += sizeof(dialogueId);

  sqlAsyncEnable = *(IDL_long*)(curptr+inputPosition);
  inputPosition += sizeof(sqlAsyncEnable);

  queryTimeout	 = *(IDL_long*)(curptr+inputPosition);
  inputPosition += sizeof(queryTimeout);

  stmtHandleKey	 = *(IDL_long*)(curptr+inputPosition);
  inputPosition += sizeof(IDL_long);
  stmtHandle = 0;
  if( stmtHandleKey > 0 )
  	stmtHandle = srvrGlobal->stmtHandleMap[stmtHandleKey];

  stmtLength	 = *(IDL_long*)(curptr+inputPosition);
  inputPosition += sizeof(stmtLength);
  if (stmtLength > 0)
  {
    stmtLabel     = curptr+inputPosition;
    inputPosition += stmtLength;
    stmtLabelCharset  = *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(stmtLabelCharset);
  }

  maxRowCnt	 = *(IDL_unsigned_long_long*)(curptr+inputPosition);
  inputPosition += sizeof(maxRowCnt);

  maxRowLen	 = *(IDL_unsigned_long_long*)(curptr+inputPosition);
  inputPosition += sizeof(maxRowLen);

  /* Unused for now */
  cursorLength	 = *(IDL_long*)(curptr+inputPosition);
  inputPosition += sizeof(cursorLength);
  if (cursorLength > 0)
  {
    cursorName     = curptr+inputPosition;
    inputPosition += cursorLength;
    cursorCharset  = *(IDL_long*)(curptr+inputPosition);
    inputPosition += sizeof(cursorCharset);
  }

  /* Unused for now */
  setStmtOptionsLength	= *(IDL_long*)(curptr+inputPosition);
  inputPosition += sizeof(setStmtOptionsLength);
  if (setStmtOptionsLength > 0)
  {
     setStmtOptions = curptr+inputPosition;
	 inputPosition += setStmtOptionsLength;
  }

  odbc_SQLSrvr_Fetch_ame_(
	  objtag_
	, call_id_
	, dialogueId
	, operation_id
	, sqlAsyncEnable
	, queryTimeout
	, stmtHandle
	, stmtLabel
	, maxRowCnt
	, maxRowLen);

} /* SQLFETCH_IOMessage() */

void
SQLFREESTMT_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  )
{
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;
	CEE_status retcode;

	IDL_char *stmtLabel = NULL;
	IDL_long  stmtLabelLength = 0;
	DIALOGUE_ID_def dialogueId = 0;
	IDL_unsigned_short freeResourceOpt = 0;

	IDL_char   *curptr = NULL;
	IDL_long   inputPosition = 0;

	curptr = pnode->r_buffer();

	// 1st Parameter: Dialogue Id
	dialogueId = *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(dialogueId);

	// 2nd Param: Statement label
    stmtLabelLength = *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(stmtLabelLength);

	if(stmtLabelLength > 0)
	{
        stmtLabel = curptr + inputPosition;
		inputPosition += stmtLabelLength;
	}

	// 3rd Parameter: Free resource options
    freeResourceOpt = *(IDL_unsigned_short*)(curptr+inputPosition);
	inputPosition += sizeof(freeResourceOpt);


	odbc_SQLSrvr_Close_ame_(
			objtag_
		  , call_id_
		  , dialogueId
		  , stmtLabel
		  , freeResourceOpt
	);

} /* SQLFREESTMT_IOMessage() */

void
SQLGETCATALOGS_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  )
{
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;
	CEE_status retcode;

	IDL_char	*curptr;
	IDL_long inputPosition = 0;

	DIALOGUE_ID_def dialogueId;
	IDL_char *stmtLabel = NULL;
	IDL_short APIType;
	IDL_char *catalogNm = NULL;
	IDL_char *schemaNm  = NULL;
	IDL_char *tableNm   = NULL;
	IDL_char *tableTypeList = NULL;
	IDL_char *columnNm  = NULL;
	IDL_long columnType;
	IDL_long rowIdScope;
	IDL_long nullable;
	IDL_long uniqueness;
	IDL_long accuracy;
	IDL_short sqlType;
	IDL_unsigned_long metadataId;
	IDL_char *fkcatalogNm = NULL;
	IDL_char *fkschemaNm  = NULL;
	IDL_char *fktableNm   = NULL;

	IDL_long stmtLabelLen =0;
	IDL_long catalogNmLen =0;
	IDL_long schemaNmLen =0;
	IDL_long tableNmLen =0;
	IDL_long tableTypeListLen =0;
	IDL_long columnNmLen =0;
	IDL_long fkcatalogNmLen =0;
	IDL_long fkschemaNmLen = 0;
	IDL_long fktableNmLen = 0;


	curptr = pnode->r_buffer();

	dialogueId = *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(dialogueId);

	stmtLabelLen	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(stmtLabelLen);
	if (stmtLabelLen > 0)
	{
		stmtLabel = curptr+inputPosition;
		inputPosition += stmtLabelLen;
	}

	APIType	= *(IDL_short*)(curptr+inputPosition);
	inputPosition += sizeof(APIType);


	catalogNmLen	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(catalogNmLen);
	if (catalogNmLen > 0)
	{
		catalogNm = curptr+inputPosition;
		inputPosition += catalogNmLen;
	}

	schemaNmLen	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(schemaNmLen);
	if (schemaNmLen > 0)
	{
		schemaNm = curptr+inputPosition;
		inputPosition += schemaNmLen;
	}

	tableNmLen	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(tableNmLen);
	if (tableNmLen > 0)
	{
		tableNm = curptr+inputPosition;
		inputPosition += tableNmLen;
	}

	tableTypeListLen = *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(tableTypeListLen);
	if (tableTypeListLen > 0)
	{
		tableTypeList = curptr+inputPosition;
		inputPosition += tableTypeListLen;
	}

	columnNmLen	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(columnNmLen);
	if (columnNmLen > 0)
	{
		columnNm = curptr+inputPosition;
		inputPosition += columnNmLen;
	}

	columnType		= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(columnType);

	rowIdScope	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(rowIdScope);

	nullable		= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(nullable);

	uniqueness		= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(uniqueness);

	accuracy		= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(accuracy);

	sqlType	= *(IDL_short*)(curptr+inputPosition);
	inputPosition += sizeof(sqlType);

	metadataId		= *(IDL_unsigned_long*)(curptr+inputPosition);
	inputPosition += sizeof(metadataId);

	fkcatalogNmLen	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(fkcatalogNmLen);
	if (fkcatalogNmLen > 0)
	{
		fkcatalogNm = curptr+inputPosition;
		inputPosition += fkcatalogNmLen;
	}

	fkschemaNmLen	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(fkschemaNmLen);
	if (fkschemaNmLen > 0)
	{
		fkschemaNm = curptr+inputPosition;
		inputPosition += fkschemaNmLen;
	}

	fktableNmLen	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(fktableNmLen);
	if (fktableNmLen > 0)
	{
		fktableNm = curptr+inputPosition;
		inputPosition += fktableNmLen;
	}


	odbc_SQLSrvr_GetSQLCatalogs_ame_(
		 objtag_
	  ,  call_id_
	  ,  dialogueId
	  ,  stmtLabel
	  ,  APIType
	  ,  catalogNm
	  ,  schemaNm
	  ,  tableNm
	  ,  tableTypeList
	  ,  columnNm
	  ,  columnType
	  ,  rowIdScope
	  ,  nullable
	  ,  uniqueness
	  ,  accuracy
	  ,  sqlType
	  ,  metadataId
	  ,  fkcatalogNm
	  ,  fkschemaNm
	  ,  fktableNm
	  );
}

void
STOPSRVR_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  )
{
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;
	CEE_status retcode;

	DIALOGUE_ID_def dialogueId;
	IDL_long StopType;
	IDL_string ReasonText;

	long* param[3];
	retcode = decodeParameters(3, param, pnode->r_buffer(), pnode->r_buffer_length());
	if (retcode != CEE_SUCCESS)
	{
//LCOV_EXCL_START
		strcpy( errStrBuf2, "odbcs_srvr.cpp");
		strcpy( errStrBuf3, "SRVR-STOPSRVR_IOMessage");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "retcode <%d>", retcode);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
//LCOV_EXCL_STOP
	}

	dialogueId	= *(IDL_long*)param[0];
	StopType = *(IDL_long*)param[1];
	ReasonText = (IDL_string)param[2];

	odbc_SQLSvc_StopServer_ame_(
		 objtag_
	  ,  call_id_
	  ,  dialogueId
	  ,  StopType
	  ,  ReasonText
	  );
}

void
ENABLETRACE_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  )
{
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;
	CEE_status retcode;

	DIALOGUE_ID_def dialogueId;
	IDL_long TraceType;

	long* param[2];
	retcode = decodeParameters(2, param, pnode->r_buffer(), pnode->r_buffer_length());
	if (retcode != CEE_SUCCESS)
	{
//LCOV_EXCL_START
		strcpy( errStrBuf2, "odbcs_srvr.cpp");
		strcpy( errStrBuf3, "SRVR-ENABLETRACE_IOMessage");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "retcode <%d>", retcode);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
//LCOV_EXCL_STOP
	}

	dialogueId	= *(IDL_long*)param[0];
	TraceType = *(IDL_long*)param[1];

	odbc_SQLSvc_EnableServerTrace_ame_(
		 objtag_
	  ,  call_id_
	  ,  dialogueId
	  ,  TraceType
	  );
}

void
DISABLETRACE_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  )
{
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;
	CEE_status retcode;

	DIALOGUE_ID_def dialogueId;
	IDL_long TraceType;

	long* param[2];
	retcode = decodeParameters(2, param, pnode->r_buffer(), pnode->r_buffer_length());
	if (retcode != CEE_SUCCESS)
	{
//LCOV_EXCL_START
		strcpy( errStrBuf2, "odbcs_srvr.cpp");
		strcpy( errStrBuf3, "SRVR-DISABLETRACE_IOMessage");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "retcode <%d>", retcode);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
//LCOV_EXCL_STOP
	}

	dialogueId	= *(IDL_long*)param[0];
	TraceType = *(IDL_long*)param[1];

	odbc_SQLSvc_DisableServerTrace_ame_(
		 objtag_
	  ,  call_id_
	  ,  dialogueId
	  ,  TraceType
	  );
}

void
ENABLESTATISTICS_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  )
{
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;
	CEE_status retcode;

	DIALOGUE_ID_def dialogueId;
	IDL_long StatisticsType;

	long* param[2];
	retcode = decodeParameters(2, param, pnode->r_buffer(), pnode->r_buffer_length());
	if (retcode != CEE_SUCCESS)
	{
//LCOV_EXCL_START
		strcpy( errStrBuf2, "odbcs_srvr.cpp");
		strcpy( errStrBuf3, "SRVR-ENABLESTATISTICS_IOMessage");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "retcode <%d>", retcode);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
//LCOV_EXCL_STOP
	}

	dialogueId	= *(IDL_long*)param[0];
	StatisticsType = *(IDL_long*)param[1];

	odbc_SQLSvc_EnableServerStatistics_ame_(
		 objtag_
	  ,  call_id_
	  ,  dialogueId
	  ,  StatisticsType
	  );
}

void
DISABLESTATISTICS_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  )
{
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;
	CEE_status retcode;

	DIALOGUE_ID_def dialogueId;

	long* param[1];
	retcode = decodeParameters(1, param, pnode->r_buffer(), pnode->r_buffer_length());
	if (retcode != CEE_SUCCESS)
	{
//LCOV_EXCL_START
		strcpy( errStrBuf2, "odbcs_srvr.cpp");
		strcpy( errStrBuf3, "SRVR-DISABLESTATISTICS_IOMessage");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "retcode <%d>", retcode);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
//LCOV_EXCL_STOP
	}

	dialogueId	= *(IDL_long*)param[0];

	odbc_SQLSvc_DisableServerStatistics_ame_(
		 objtag_
	  ,  call_id_
	  ,  dialogueId
	  );
}

void
UPDATECONTEXT_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  )
{
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;
	CEE_status retcode;

	SRVR_CONTEXT_def *srvrContext;

	long* param[1];
	retcode = decodeParameters(1, param, pnode->r_buffer(), pnode->r_buffer_length());
	if (retcode != CEE_SUCCESS)
	{
//LCOV_EXCL_START
		strcpy( errStrBuf2, "odbcs_srvr.cpp");
		strcpy( errStrBuf3, "SRVR-UPDATECONTEXT_IOMessage");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "retcode <%d>", retcode);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
//LCOV_EXCL_STOP
	}

	srvrContext	= (SRVR_CONTEXT_def*)param[0];

	odbc_SQLSvc_UpdateServerContext_ame_(
		 objtag_
	  ,  call_id_
	  ,  srvrContext
	  );
}

void
MONITORCALL_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  )
{
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;
	CEE_status retcode;

	DIALOGUE_ID_def dialogueId;

	long* param[1];
	retcode = decodeParameters(1, param, pnode->r_buffer(), pnode->r_buffer_length());
	if (retcode != CEE_SUCCESS)
	{
//LCOV_EXCL_START
		strcpy( errStrBuf2, "odbcs_srvr.cpp");
		strcpy( errStrBuf3, "SRVR-MONITORCALL_IOMessage");
		strcpy( errStrBuf4, "buffer overflow");
		sprintf( errStrBuf5, "retcode <%d>", retcode);
		logError( PROGRAM_ERROR, SEVERITY_MAJOR, CAPTURE_ALL + PROCESS_STOP );
		exit(1000);
//LCOV_EXCL_STOP
	}

	dialogueId	= *(IDL_long*)param[0];

	odbc_SQLSvc_MonitorCall_ame_(
		 objtag_
	  ,  call_id_
	  ,  dialogueId
	  );
}

void
EXTRACTLOB_IOMessage(
    /* In    */CEE_tag_def objtag_
  , /* In    */const CEE_handle_def *call_id_
  )
{
    CInterface* pnode = (CInterface *)objtag_;
    CEE_status sts = CEE_SUCCESS;
    CEE_status retcode;

    IDL_char     *curptr;
    IDL_long inputPosition = 0;

    IDL_short extractLobAPI = 0;
    IDL_long extractLen = 0;
    IDL_long lobHandleLen = 0;
    IDL_string lobHandle = NULL;
    IDL_long lobHandleCharset = 0;

    curptr = pnode->r_buffer();

    extractLobAPI = *(IDL_short *)(curptr + inputPosition);
    inputPosition += sizeof(extractLobAPI);

    lobHandleLen = *(IDL_long*)(curptr + inputPosition);
    inputPosition += sizeof(lobHandleLen);

    if (lobHandleLen > 0)
    {
        lobHandle = curptr + inputPosition;
        inputPosition += lobHandleLen;
        lobHandleCharset = *(IDL_long *)(curptr + inputPosition);
        inputPosition += sizeof(lobHandleCharset);
    }

    extractLen = *(IDL_long *)(curptr + inputPosition);
    inputPosition += sizeof(extractLen);

    odbc_SQLSrvr_ExtractLob_ame_(
            objtag_,
            call_id_,
            extractLobAPI,
            lobHandle,
            extractLen
            );
}

void UPDATELOB_IOMessage(
          /* In  */ CEE_tag_def objtag_
        , /* In  */ const CEE_handle_def *call_id_
        )
{
    CInterface * pnode = (CInterface *)objtag_;
    CEE_status  sts = CEE_SUCCESS;

    IDL_char   *curptr;
    IDL_long   inputPosition = 0;

    IDL_long   lobUpdateType = 0;
    IDL_long   lobHandleLen = 0;
    IDL_string lobHandle = NULL;
    IDL_long   lobHandleCharset = 0;

    IDL_long_long   totalLength = 0;
    IDL_long_long   offset = 0;
    IDL_long_long   length = 0;
    BYTE     * data = NULL;

    curptr = pnode->r_buffer();

    lobUpdateType = *(IDL_long *)(curptr + inputPosition);
    inputPosition += sizeof(lobUpdateType);

    lobHandleLen = *(IDL_long *)(curptr + inputPosition);
    inputPosition += sizeof(lobHandleLen);

    if (lobHandleLen > 0)
    {
        lobHandle = curptr + inputPosition;
        inputPosition += lobHandleLen;
        lobHandleCharset = *(IDL_long *)(curptr + inputPosition);
        inputPosition += sizeof(lobHandleCharset);
    }

    totalLength = *(IDL_long_long *)(curptr + inputPosition);
    inputPosition += sizeof(IDL_long_long);

    offset = *(IDL_long_long *)(curptr + inputPosition);
    inputPosition += sizeof(IDL_long_long);

    length = *(IDL_long_long *)(curptr + inputPosition);
    inputPosition += sizeof(IDL_long_long);

    if (length > 0)
    {
        data = (BYTE *)curptr + inputPosition;
        inputPosition += length;
    }

    odbc_SQLSrvr_UpdateLob_ame_(
            objtag_,
            call_id_,
            lobUpdateType,
            lobHandle,
            totalLength,
            offset,
            length,
            data);
}

void LOG_MSG(CError* ierror, short level)
{
	char buffer[500];

	IDL_OBJECT_def objRef;
	memset(&objRef, 0, sizeof(IDL_OBJECT_def));
	if (srvrGlobal != NULL)
		memcpy(&objRef,&srvrGlobal->srvrObjRef,sizeof(IDL_OBJECT_def));

	strcpy(buffer,ERROR_TO_TEXT(ierror));

	SendEventMsg( MSG_KRYPTON_ERROR,
				level,
				GetCurrentProcessId(),
				ODBCMX_SERVER,
				objRef,
				2, FORMAT_ERROR(ierror), buffer);
}

void LOG_ERROR(CError* ierror)
{
	LOG_MSG( ierror, EVENTLOG_ERROR_TYPE);
}

void LOG_WARNING(CError* ierror)
{
	LOG_MSG( ierror, EVENTLOG_WARNING_TYPE);
}

void LOG_INFO(CError* ierror)
{
	LOG_MSG( ierror, EVENTLOG_INFORMATION_TYPE);
}

void
SQLEXECUTE_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ short operation_id
  )
{
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status	sts = CEE_SUCCESS;
	CEE_status	retcode;
	IDL_char	*curptr;
	IDL_unsigned_long i;


	DIALOGUE_ID_def dialogueId = 0;
	IDL_long   sqlAsyncEnable = 0;
    IDL_long   queryTimeout = 0;
	IDL_long   inputRowCnt = 0;
	IDL_long   maxRowsetSize = 0;
	IDL_long   sqlStmtType = 0;
	Long   stmtHandle = 0;
	IDL_long   stmtHandleKey = 0;
	IDL_long   stmtType = 0;
	IDL_long   sqlStringLength = 0;
	IDL_string sqlString = NULL;
	IDL_long   sqlStringCharset = 0;
	IDL_long   cursorLength = 0;
	IDL_string cursorName = NULL;
	IDL_long   cursorCharset = 0;
	IDL_long   stmtLength = 0;
	IDL_char  *stmtLabel = NULL;
	IDL_long   stmtLabelCharset = 0;
    IDL_long   stmtExplainLabelLength = 0;
	IDL_string stmtExplainLabel = NULL;
	IDL_long   inValuesLength = 0;
	BYTE      *inValues = NULL;
	IDL_long	transactionIDLength = 0; // JDBC is the only one that will use this to join a transaction
	IDL_long_long transactionID = 0;     // JDBC is the only one that will use this to join a transaction


	IDL_long  holdableCursor = SQL_NONHOLDABLE; //default
	IDL_long inputPosition = 0;
	IDL_short ix;

	curptr = pnode->r_buffer();

	dialogueId = *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(dialogueId);

	// to support SAP holdable cursor, the driver overload this field with the value of holdableCursor
	// currently the sqlAsyncEnable is not used.
	//sqlAsyncEnable	= *(IDL_long*)(curptr+inputPosition);
	holdableCursor	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(sqlAsyncEnable);

	queryTimeout	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(queryTimeout);

	inputRowCnt		= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(inputRowCnt);

	maxRowsetSize	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(maxRowsetSize);

	sqlStmtType		= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(sqlStmtType);

	stmtHandleKey	 = *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(IDL_long);
	stmtHandle = 0;
	if( stmtHandleKey > 0 )
		stmtHandle = srvrGlobal->stmtHandleMap[stmtHandleKey];

	stmtType		= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(stmtType);

	sqlStringLength	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(sqlStringLength);
	if (sqlStringLength > 0)
	{
		sqlString = curptr+inputPosition;
		inputPosition += sqlStringLength;
		sqlStringCharset	= *(IDL_long*)(curptr+inputPosition);
		inputPosition += sizeof(sqlStringCharset);
	}

	cursorLength	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(cursorLength);
	if (cursorLength > 0)
	{
		cursorName = curptr+inputPosition;
		inputPosition += cursorLength;
		cursorCharset = *(IDL_long*)(curptr+inputPosition);
		inputPosition += sizeof(cursorCharset);
	}

	stmtLength		= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(stmtLength);
	if (stmtLength > 0)
	{
		stmtLabel = curptr+inputPosition;
		inputPosition += stmtLength;
		stmtLabelCharset = *(IDL_long*)(curptr+inputPosition);
		inputPosition += sizeof(stmtLabelCharset);
	}

	stmtExplainLabelLength	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(stmtExplainLabelLength);
	if (stmtExplainLabelLength > 0)
	{
		stmtExplainLabel = curptr+inputPosition;
		inputPosition += stmtExplainLabelLength;
	}

	inValuesLength	= *(IDL_long*)(curptr+inputPosition);
	inputPosition += sizeof(inValuesLength);
	if (inValuesLength > 0)
	{
		inValues = (BYTE *) curptr+inputPosition;
		inputPosition += inValuesLength;
	}

	transactionIDLength = *(IDL_unsigned_long*)(curptr+inputPosition);
	inputPosition += sizeof(transactionIDLength);

	if(transactionIDLength > 0)
	{
//LCOV_EXCL_START
//LCOV_EXCL_STOP
                   if(transactionIDLength == 5)
		      transactionID = *(IDL_long*)(curptr+inputPosition);
                   if(transactionIDLength == 9)
		      transactionID = *(IDL_long_long*)(curptr+inputPosition);

		   inputPosition += transactionIDLength;

	}


   if( operation_id == SRVR_API_SQLEXECUTE2)
   {
	   odbc_SQLSrvr_Execute2_ame_(
         objtag_,
         call_id_,
         dialogueId,
         sqlAsyncEnable,
         queryTimeout,
         inputRowCnt,
         sqlStmtType,
         stmtHandle,
         cursorName,
         cursorCharset,
         inValuesLength,
         inValues,
		 0,        // Sql Query Type (used for execdirect calls)
		 0,        // output Descriptor Length (used for execdirect calls)
		 NULL,     // output Descriptor (used for execdirect calls)
		 maxRowsetSize, //For DBT to obtain the Rowlength from Driver
		 transactionID, // JDBC sends this to join an existing transaction for SPJ calls
		 holdableCursor
		 );
   } /* if operation_id == SRVR_API_SQLEXECUTE2 */
   else if( operation_id == SRVR_API_SQLEXECDIRECT)
   {

	   odbc_SQLSrvr_ExecDirect_ame_(
         objtag_,
         call_id_,
         dialogueId,
         stmtLabel,
         cursorName,
         stmtExplainLabel,
         stmtType,
         sqlStmtType,
         sqlString,
         sqlAsyncEnable,
         queryTimeout,
	 inputRowCnt,
	 	transactionID, // JDBC sends this to join an existing transaction for SPJ calls
		holdableCursor
	 );

   } /* if operation_id == SRVR_API_SQLEXECDIRECT */


} // SQLEXECUTE_IOMessage()



//
//======================= TCPIP =============================================
//
void DISPATCH_TCPIPRequest(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  ,	short operation_id
)
{
	SRVRTRACE_ENTER(FILE_IIOM+1);
	CTCPIPSystemSrvr* pnode = (CTCPIPSystemSrvr*)objtag_;
	switch(operation_id)
	{
	case SRVR_API_SQLCONNECT:
#ifdef __TIME_LOGGER
		if(srvrGlobal->timeLoggerFlag)
		{
			srvrGlobal->timeLogger.start("SQLCONNECT");
		}
#endif
		SQLCONNECT_IOMessage(objtag_, call_id_);
#ifdef __TIME_LOGGER
		if(srvrGlobal->timeLoggerFlag)
		{
			srvrGlobal->timeLogger.end();
		}
#endif
		break;
	case SRVR_API_SQLDISCONNECT:
#ifdef __TIME_LOGGER
		if(srvrGlobal->timeLoggerFlag)
		{
			srvrGlobal->timeLogger.start("SQLDISCONNECT");
		}
#endif
		SQLDISCONNECT_IOMessage(objtag_, call_id_);
#ifdef __TIME_LOGGER
		if(srvrGlobal->timeLoggerFlag)
		{
			srvrGlobal->timeLogger.end();
		}
#endif
		break;
	case SRVR_API_SQLSETCONNECTATTR:
		SQLSETCONNECTATTR_IOMessage(objtag_, call_id_);
		break;
	case SRVR_API_SQLENDTRAN:
		SQLENDTRAN_IOMessage(objtag_, call_id_);
		break;
	case SRVR_API_SQLPREPARE:
		SQLPREPARE_IOMessage(objtag_, call_id_);
		break;
	case SRVR_API_SQLEXECDIRECT_ROWSET:
	case SRVR_API_SQLEXECDIRECT:
	case SRVR_API_SQLEXECUTE2:
	case SRVR_API_SQLEXECUTECALL:
	     SQLEXECUTE_IOMessage(objtag_, call_id_, operation_id);
		break;
	case SRVR_API_SQLFREESTMT:
		SQLFREESTMT_IOMessage(objtag_, call_id_);
		break;
	case SRVR_API_GETCATALOGS:
		SQLGETCATALOGS_IOMessage(objtag_, call_id_);
		break;
	case SRVR_API_SQLFETCH:
	case SRVR_API_SQLFETCH_ROWSET:
		SQLFETCH_IOMessage(objtag_, call_id_, operation_id);
		break;
	case SRVR_API_STOPSRVR:
		STOPSRVR_IOMessage(objtag_, call_id_);
		break;
	case SRVR_API_ENABLETRACE:
		ENABLETRACE_IOMessage(objtag_, call_id_);
		break;
	case SRVR_API_DISABLETRACE:
		DISABLETRACE_IOMessage(objtag_, call_id_);
		break;
	case SRVR_API_ENABLE_SERVER_STATISTICS:
		ENABLESTATISTICS_IOMessage(objtag_, call_id_);
		break;
	case SRVR_API_DISABLE_SERVER_STATISTICS:
		DISABLESTATISTICS_IOMessage(objtag_, call_id_);
		break;
	case SRVR_API_UPDATE_SERVER_CONTEXT:
		UPDATECONTEXT_IOMessage(objtag_, call_id_);
		break;
	case CLOSE_TCPIP_SESSION:
		shutdown(pnode->m_nSocketFnum, 2);
		FILE_CLOSE_(pnode->m_nSocketFnum);
		pnode->m_nSocketFnum = -2;
		break;
    case SRVR_API_EXTRACTLOB:
        EXTRACTLOB_IOMessage(objtag_, call_id_);
        break;
    case SRVR_API_UPDATELOB:
        UPDATELOB_IOMessage(objtag_, call_id_);
        break;
	default:
//LCOV_EXCL_START
		break;
	}
//LCOV_EXCL_STOP
	SRVRTRACE_EXIT(FILE_IIOM+1);

//LCOV_EXCL_START
	// Exit on an un-recoverable SQL error
	// The sqlErrorExit array and errorIndex will be set in SqlWrapper.cpp
	// whenever SQL returns an error

/* Test code - to test terminate thread from TCPIP thread
	if(operation_id == SRVR_API_SQLDISCONNECT)
	{
		sqlErrorExit[0] = -8700;
		errorIndex = 1;
	}
*/

	if( errorIndex > 0 ) {
		for( int i = 0; i < errorIndex; i++ ) {
			if( isExitError(sqlErrorExit[i]) )
			{
				char tmpString[100];
				sprintf(tmpString, "SQL returned an unrecoverable fatal error (%d). Server Exiting.", sqlErrorExit[i]);
				SendEventMsg(MSG_ODBC_NSK_ERROR
											, EVENTLOG_ERROR_TYPE
											, GetCurrentProcessId()
											, ODBCMX_SERVER
											, srvrGlobal->srvrObjRef
											, 1
											, tmpString);
    		    terminateThreads(sqlErrorExit[i]);
    		    file_mon_process_shutdown();
				exit( sqlErrorExit[i] );
			}
			sqlErrorExit[i] = 0;
		}
	}
	errorIndex = 0;
//LCOV_EXCL_STOP
}

void
ReleaseServer()
{
	BreakDialogue(NULL);
}


