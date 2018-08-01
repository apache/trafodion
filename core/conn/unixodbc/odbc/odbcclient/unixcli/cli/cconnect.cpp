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
//
//

#include "drvrglobal.h"
#include "cconnect.h"
#include "mxomsg.h"
#include "drvrnet.h"
//#include "nlsFunctions.h"
#include "resource.h"
#include "cstmt.h"
#include "security.h"
#include "diagfunctions.h"
#include "bucket.h"
#include "assert.h"
#include "csconvert.h"
#include <locale.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>

#ifdef MXOSS
#include <netdb.h>
#endif

#define UNKNOWN_WINDOW		"UNKNOWN"
extern void SetRowsetErrorRecovery(BOOL m_RowsetErrorRecovery);

#define 	SQL_START_NODE			   4000 // connection attribute define for starting node
#define	    SQL_MODE_LOADER		       4001 // connection attribute define to specify mode as loader
#define 	SQL_STREAMS_PER_SEG 	   4002 // connection attribute define to specify number of streams per segment
#define 	SQL_ATTR_SUG_ROWSET_SIZE   4003 // connectction attribute for suggested rowset size
#define     SQL_LOADER_DELAYERROR      4004 // delay error
#define 	SQL_END_NODE               4005 // connection attribute define for ending node (DBT - rule based engine)
#define 	STREAM_START_NODE		 15
#define	    STREAMS_PER_NODE		 1

// Implements the member functions of CConnect 

CConnect::CConnect(SQLHANDLE InputHandle) : CHandle(SQL_HANDLE_DBC, InputHandle)
{
	m_TranslateLibHandle = NULL;
	userStreams = 0;
	maxUserStreams = 0;
	m_StandardConfig = 0;

	m_SecPwd = NULL;
	m_ICUConv = NULL;

#ifdef ASYNCIO
	m_async_IOthread = NULL;
	m_CSTransmision = new Async_Mutex();
	if(m_CSTransmision == NULL || !m_CSTransmision->IsInitialized())
	{
		delete m_CSTransmision;
		m_CSTransmision = NULL;
	}
#endif /* ASYNCIO */

	reset();
	m_ConnectEvent = NULL;
	m_EnvHandle = (CEnv *)InputHandle;

#ifdef VERSION3
		m_EnvHandle->m_ConnectCollect.push_back(this);
#else
		m_EnvHandle->m_ConnectCollect.append(this);
#endif

#ifndef unixcli
	m_asFileSystem = GTransport.m_FSystemDrvr_list->ins_node();
	m_srvrFileSystem = GTransport.m_FSystemDrvr_list->ins_node();
#else
	m_asTCPIPSystem = GTransport.m_TCPIPUnixDrvr_list->ins_node();
	m_srvrTCPIPSystem = GTransport.m_TCPIPUnixDrvr_list->ins_node();
#endif

//#ifdef HASH_DRIVER
	// Initialize Streaming Variables
	m_srvrTCPIPSystemStream = NULL;
	m_SQLSvc_ObjRefStream = NULL;
	m_DialogueIdStream = NULL;
	m_hashInfo.hashType = 0;
	m_hashInfo.hashPart = 0;
	m_hashInfo.segNum = NULL;
	m_hashInfo.partNum = NULL;
	m_hashInfo.hashCol = NULL;
	m_hashInfo.colCount = 0;
	m_ModeLoader = 0;
	m_StreamDelayedError = 0;
	m_StartNode = -1;
	m_EndNode   = -1;
	m_StreamsPerSeg = STREAMS_PER_NODE;
	m_SugRowsetSize = 0;

    m_hashInfo.HashColKeys = NULL;
    m_hashInfo.HashColLens = NULL;
	m_hashInfo.HashedVarBuffer = NULL;
	m_hashInfo.HashedmemPtr = NULL;
	m_hashInfo.oHashedVarLocation = NULL;
	m_hashInfo.oHashedIndLocation = NULL;
	m_hashInfo.HashedinputRowCnt = NULL;
	m_hashInfo.HashedrowNumber = NULL;
    m_hashInfo.HashedVarBufferLength = NULL;
	m_hashInfo.HashedCurRowCnt = NULL;
	m_hashInfo.ErrorWarning = NULL;
	m_hashInfo.PreviousHashMapTable = NULL;
	m_hashInfo.CurrentHashMapTable = NULL;
	m_hashInfo.statusArray = NULL;
	m_hashInfo.ErrorWarningLength = 0;
	initHashValuesComplete = false;
	m_IgnoreCancel = false;
	m_RetryEncryption = false;

	// Initialize the converter class
	m_ICUConv = new ICUConverter();
	if(gDrvrGlobal.gDisablePreFetch == false)
	{
	   m_preFetchThread = new preFetchThread(this);
	   if(gDrvrGlobal.gDisablePreFetch == true)
	   {
	      delete m_preFetchThread;
		  m_preFetchThread = NULL;
	   }
	   else
	   {
	      m_preFetchThread->createThread();
		  while(m_preFetchThread->m_InitializationComplete == false)
		  {
		     sleep(0);
		     sched_yield();
		  }
		  
	   }
	}
	else 
	   m_preFetchThread = NULL;
}

CConnect::~CConnect()
{

#ifdef VERSION3 
	std::list<CHandlePtr>::iterator i = m_EnvHandle->m_ConnectCollect.begin();
	CConnect* pConnect;

	for (; i !=  m_EnvHandle->m_ConnectCollect.end() ; ++i)
	{
		if ((pConnect = (CConnect*)(*i)) == this)
		{
			m_EnvHandle->m_ConnectCollect.erase(i);
			break;
		}
	}
#else
	RWTPtrSlistIterator<CHandle> i(m_EnvHandle->m_ConnectCollect);
	CConnect* pConnect;
	while ((pConnect = (CConnect*)i()) != NULL)
	{
		if (pConnect == this)
		{
			i.remove();
			break;
		}
	}
#endif

	if (m_ConnectEvent != NULL) 
		CloseHandle(m_ConnectEvent);

	if(m_preFetchThread != NULL)
          delete m_preFetchThread;

#ifndef unixcli
	if (m_asFileSystem != NULL)
		GTransport.m_FSystemDrvr_list->del_node(m_asFileSystem);
	if (m_srvrFileSystem != NULL)
		GTransport.m_FSystemDrvr_list->del_node(m_srvrFileSystem);
#else
        if (m_asTCPIPSystem != NULL)
                GTransport.m_TCPIPUnixDrvr_list->del_node(m_asTCPIPSystem);
        if (m_srvrTCPIPSystem != NULL)
                GTransport.m_TCPIPUnixDrvr_list->del_node(m_srvrTCPIPSystem);
#endif

#ifdef ASYNCIO
	if(m_async_IOthread != NULL)
		delete m_async_IOthread;
#endif /* ASYNCIO */

#ifndef MXOSS
	if (m_SecPwd != NULL)
		delete m_SecPwd;
#endif
	if (m_ICUConv != NULL)
	{
		delete m_ICUConv;
		m_ICUConv = NULL;
	}

	delete m_CSTransmision;
} // CConnect::~CConnect()


SQLRETURN CConnect::initialize()
{
	if (m_ConnectEvent == NULL) 
	{
		m_ConnectEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (m_ConnectEvent == NULL)
		{
//			setNTError(getErrorMsgLang(), "initialize - CreateEvent()");
//			return SQL_ERROR;
		}
	}
	return SQL_SUCCESS;
}


void CConnect::reset(bool clError)
{

	if(clError)
		clearError();
	m_HeartBeatEnable = TRUE;
		
	m_RetSQLSvc_ObjRef[0] = '\0';
	m_ASSvc_ObjRef[0] = '\0';
	m_SQLSvc_ObjRef[0] = '\0';
	m_DialogueId = -1;
	m_SrvrDSName[0] ='\0';
	m_SrvrDSReadType = 0;
	m_SrvrIdentity[0] = '\0';
	
	m_AccessMode = SQL_MODE_DEFAULT;
	m_AutoIPD = SQL_TRUE;			// Automatic population of IPD is supported
	m_AutoCommit = SQL_AUTOCOMMIT_DEFAULT;
	m_ConnectionTimeout = DRVR_PENDING;
	m_CurrentCatalog[0] = '\0';
	m_CurrentSchema[0] = '\0';
	m_LoginTimeout	= DRVR_PENDING;
	m_ODBCCursors = SQL_CUR_USE_DRIVER;
	m_PacketSize = 0;	// Not Used
	m_QuietMode = NULL;
	m_Trace = SQL_OPT_TRACE_OFF;
	m_TraceFile[0] = '\0';
	m_TranslateLib[0] = '\0';
	m_TranslateOption = DRVR_PENDING;
	m_TxnIsolation = gDrvrGlobal.gDefaultTxnIsolation;
	m_ConnectionDead = SQL_CD_TRUE;
	m_CDInfoSent	= TRUE;

	m_AsyncEnable = SQL_ASYNC_ENABLE_DEFAULT;
	m_MetadataId = SQL_FALSE;		// Catalog functions arguments are not treated as identifiers
	
	m_Concurrency = SQL_CONCUR_DEFAULT;
	m_CursorType = SQL_CURSOR_TYPE_DEFAULT;
	m_MaxLength = SQL_MAX_LENGTH_DEFAULT;
	m_MaxRows = SQL_MAX_ROWS_DEFAULT;
	m_Noscan = SQL_NOSCAN_DEFAULT;
	m_QueryTimeout = SQL_QUERY_TIMEOUT_DEFAULT;
	m_SimulateCursor = SQL_SC_NON_UNIQUE;
	m_UseBookmarks = SQL_UB_DEFAULT;
	if (m_TranslateLibHandle)
	{
		FreeLibrary(m_TranslateLibHandle);
		m_TranslateLibHandle = NULL;
	}
	m_TransportType = TR_UNKNOWN;

	memset(&m_ASVersion,0,sizeof(m_ASVersion));
	memset(&m_SrvrVersion,0,sizeof(m_ASVersion));
	memset(&m_SqlVersion,0,sizeof(m_ASVersion));

	m_RowsetErrorRecovery = 0;
	m_FetchBufferSize = 0;
	m_StartNode = -1;
	m_EndNode   = -1;

	//wms_mapping
	memset(m_QueryID_SessionName, 0, sizeof(m_QueryID_SessionName));
	memset(m_applName, 0, sizeof(m_applName));
	
	m_IgnoreCancel = false;
	m_FetchAhead = 0;
	m_UserRole[0] = '\0';
	clearUserName();

	m_CertificateDir[0] = '\0';
	m_CertificateFile[0] = '\0';
	m_CertificateFileActive[0] = '\0';
	m_SecurityMode = 1; // security mode should be 1 by default
	memset(&m_SecInfo, 0, sizeof(m_SecInfo));
#ifndef MXOSS
	if (m_SecPwd != NULL)
	{
		delete m_SecPwd;
		m_SecPwd = NULL;
	}
#endif
	m_RetryEncryption = false;

} /* CConnect::reset() */

void CConnect::setVersion(const VERSION_LIST_def *versionList,int component)
{
	unsigned long i;
	VERSION_def		*version;

	for ( i = 0; i < versionList->_length ; i++)
	{
		version = versionList->_buffer + i;
		if ((version->componentId) == NT_ODBCAS_COMPONENT || version->componentId == NSK_ODBCAS_COMPONENT) // NT or NSK
		{
			m_ASVersion.componentId = version->componentId;
			m_ASVersion.majorVersion = version->majorVersion;
			m_ASVersion.minorVersion = version->minorVersion;
			m_ASVersion.buildId = version->buildId;
		}
		if ((version->componentId & 0x00FF) == ODBC_SRVR_COMPONENT) //odbcSrvr
		{
			m_SrvrVersion.componentId = version->componentId;
			m_SrvrVersion.majorVersion = version->majorVersion;
			m_SrvrVersion.minorVersion = version->minorVersion;
			m_SrvrVersion.buildId = version->buildId;// & 0x00ff;

            if((component == ODBC_SRVR_COMPONENT) && 
               (version->buildId & MXO_SPECIAL_1_MODE))
		    {
		      gDrvrGlobal.gSpecial_1 = true;
		    }
			// set the number of streams as sent from the association server
			if ((component == NSK_ODBCAS_COMPONENT) && (m_ModeLoader != 0)	&& (userStreams == 0))
			{
			    // buildId is used to set mode and also used to
				// indicate the number of segments
				userStreams = version->buildId  * m_StreamsPerSeg; // & 0xff00;
				maxUserStreams = userStreams; // save it in case we need to turn
				                              // streaming off for a particular 
											  // table
				if(userStreams == 1)
				{
					// No point going thru the pre-hash part for a one segment, one stream per segment configuration
					// turn off pre-hash
					userStreams = 0;
					maxUserStreams = 0;
				}
			}
		}
		if ((version->componentId) == SQL_COMPONENT) // sql
		{
			m_SqlVersion.componentId = version->componentId;
			m_SqlVersion.majorVersion = version->majorVersion;
			m_SqlVersion.minorVersion = version->minorVersion;
			m_SqlVersion.buildId = version->buildId;
			m_SQLRowsetSupported = m_SqlVersion.minorVersion;
		}

	}
}

void CConnect::getVersion(VERSION_def *version, int componentId)
{
	if (componentId == NT_ODBCAS_COMPONENT || componentId == NSK_ODBCAS_COMPONENT) // NT or NSK
	{
		version->componentId = m_ASVersion.componentId;
		version->majorVersion = m_ASVersion.majorVersion;
		version->minorVersion = m_ASVersion.minorVersion;
		version->buildId = m_ASVersion.buildId;
	}
	else if (componentId == ODBC_SRVR_COMPONENT) //odbcSrvr
	{
		version->componentId = m_SrvrVersion.componentId;
		version->majorVersion = m_SrvrVersion.majorVersion;
		version->minorVersion = m_SrvrVersion.minorVersion;
		version->buildId = m_SrvrVersion.buildId;
	}
	else if (componentId == SQL_COMPONENT) // sql
	{
		version->componentId = m_SqlVersion.componentId;
		version->majorVersion = m_SqlVersion.majorVersion;
		version->minorVersion = m_SqlVersion.minorVersion;
		version->buildId = m_SqlVersion.buildId;
	}
	else
	{
		version->componentId = 0;
		version->majorVersion = 0;
		version->minorVersion = 0;
		version->buildId = 0;
	}
}

void CConnect::setOutContext(const OUT_CONNECTION_CONTEXT_def *outContext)
{
	setVersion(&outContext->versionList,ODBC_SRVR_COMPONENT);
	// Fix for solution #10-070402-3746. Replacing server cpu,pin with process name,
	// if available, otherwise we'll stick with cpu,pin.
	// First extract the process name
	IDL_OBJECT_def tmpStr;
	char *pToken = NULL;
	char *lasts;

	strcpy (tmpStr, m_RetSQLSvc_ObjRef);
#ifndef unixcli
	pToken = strtok(tmpStr, "$");
	if( pToken != NULL )
		pToken = strtok(NULL, ",");
#else
	pToken = strtok_r(tmpStr, "$", &lasts);
	if( pToken != NULL )
		pToken = strtok_r(NULL, ",", &lasts);
#endif

	if( pToken != NULL ) {
		sprintf (m_SrvrIdentity, "%s($%s){%s}", 
				outContext->computerName,
				pToken,
				m_SQLSvc_ObjRef);
	}
	else
		sprintf (m_SrvrIdentity, "%s(%d,%d){%s}", 
				outContext->computerName,
				outContext->nodeId,
				outContext->processId,
				m_SQLSvc_ObjRef);

	if (outContext->outContextOptions1 & OUTCONTEXT_OPT1_ENFORCE_ISO88591) // SQL_ATTR_ENFORCE_ISO88591 is 'ON'
		m_EnforceISO88591 = TRUE;
	if(outContext->outContextOptions1 & OUTCONTEXT_OPT1_IGNORE_SQLCANCEL)
		m_IgnoreCancel = true;

	strcpy(m_CurrentCatalog, (char*)outContext->catalog);
	
	// if schema was updated by server, it is returned in ISO_MAPPING, need to translate to UTF8
	SQLINTEGER translen=0;
	char textError[128];
	char transError[128];
	if (stricmp(m_DSValue.m_DSSchema, outContext->schema) != 0)
		m_ICUConv->TranslateUTF8(FALSE, (char*)outContext->schema, strlen(outContext->schema), m_DSValue.m_DSSchema, strlen(m_DSValue.m_DSSchema), &translen, transError);
	strcpy(m_CurrentSchema,m_DSValue.m_DSSchema); //  Added for DataDirect	
	
	if (outContext->outContextOptions1 & OUTCONTEXT_OPT1_ROLENAME) // outContext->outContextOptionString = "RN=<ROLE_NAME>;"
	{
		if(outContext->outContextOptionStringLen > 0)
		{
			pToken = strstr(outContext->outContextOptionString, "RN=");
			if (pToken != NULL)
#ifndef unixcli
				strcpy(m_UserRole, strtok(pToken+3,";"));
#else
			   strcpy(m_UserRole, strtok_r(pToken+3,";",&lasts));
#endif
		}
	}

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			LogInfo(&m_ASVersion,&m_SrvrVersion,&m_SqlVersion);
	}
}
		
void CConnect::setGetObjRefHdlOutput(const IDL_char *srvrObjRef,
		DIALOGUE_ID_def dialogueId,
		const IDL_char *dataSource, 
		const USER_SID_def *userSid,
		const VERSION_LIST_def *versionList,
		const IDL_unsigned_long isoMapping,
		const IDL_long srvrNodeId,
		const IDL_long srvrProcessId,
		const IDL_long_long timestamp)
{
	strncpy(m_SQLSvc_ObjRef, srvrObjRef, sizeof(m_SQLSvc_ObjRef));
	m_SQLSvc_ObjRef[sizeof(m_SQLSvc_ObjRef)-1] = 0;
	m_DialogueId = dialogueId;
	strncpy(m_SrvrDSName, dataSource,sizeof(m_SrvrDSName));
	m_SrvrDSName[sizeof(m_SrvrDSName)-1] = 0;
	m_SrvrDSReadType = 1;
	if (userSid->_length > 0)
	{
		strncpy(m_UserSid, (const char *)userSid->_buffer, userSid->_length);
		m_UserSid[userSid->_length] = '\0';
	}
	else
		m_UserSid[0] = '\0';
	setVersion(versionList,NSK_ODBCAS_COMPONENT);

	if (srvrNodeId >= 0) 
	{
		m_SecInfo.cpu = srvrNodeId;
		m_SecInfo.pin = srvrProcessId;
		m_SecInfo.timestamp = timestamp;
		m_SecurityMode = 1;
	}
	else
		m_SecurityMode = 0;
}

void CConnect::setObjRefHdlStream(const IDL_char *srvrObjRef,
								short streamNumber,
								const IDL_short srvrNodeId,
								const IDL_short srvrProcessId,
								const IDL_long_long timestamp)
{
	strcpy(m_srvrTCPIPSystemStream[streamNumber].SQLSvc_ObjRef, srvrObjRef);
	if (srvrNodeId >= 0) 
	{
		m_srvrTCPIPSystemStream[streamNumber].m_SecInfo.cpu = srvrNodeId;
		m_srvrTCPIPSystemStream[streamNumber].m_SecInfo.pin = srvrProcessId;
		m_srvrTCPIPSystemStream[streamNumber].m_SecInfo.timestamp = timestamp;
		m_srvrTCPIPSystemStream[streamNumber].m_SecurityMode = 1;
	}
	else
		m_srvrTCPIPSystemStream[streamNumber].m_SecurityMode = 0;
}

void CConnect::copyObjRefHdl(IDL_char *srvrObjRef)
{
	strcpy(srvrObjRef, m_SQLSvc_ObjRef);
}

void CConnect::copyObjRefHdlStream(IDL_char *srvrObjRef, short streamNumber)
{
	strcpy(srvrObjRef, m_srvrTCPIPSystemStream[streamNumber].SQLSvc_ObjRef);
}

void CConnect::resetGetObjRefHdlOutput()
{
	memset(m_SQLSvc_ObjRef, 0, sizeof(m_SQLSvc_ObjRef));
	m_DialogueId = -1;
	memset(m_SrvrDSName, 0, sizeof(m_SrvrDSName));
	m_SrvrDSReadType = 0;
	m_UserSid[0] = '\0';
}
		
typedef int (WINAPI* GetDomainUser)( LPSTR , LPSTR );

BOOL CConnect::getUserDesc(char *userNameNTS, char *AuthenticationNTS, USER_DESC_def *userDesc)
{

	strncpy(m_UserName, userNameNTS, sizeof(m_UserName));
	m_UserName[sizeof(m_UserName)-1] = 0;
	m_UserDomain[0] = '\0';

	userDesc->userDescType = UNAUTHENTICATED_USER_TYPE;
	userDesc->userName = m_UserName;
	userDesc->domainName = m_UserDomain;
	userDesc->password._length = 0;
	userDesc->password._buffer = NULL;
	userDesc->userSid._length = 0;
	userDesc->userSid._buffer = NULL;

	return TRUE;
}	

long CConnect::sendCDInfo(long exception_nr)
{
	long retcode = CEE_SUCCESS;
//	SRVR_CALL_CONTEXT	srvrCallContext;
	// populate the SrvrCallContext
	if (exception_nr == -27 || exception_nr == -29 || exception_nr == -39 || 
		exception_nr == COMM_LINK_FAIL_EXCEPTION)
	{
		m_ConnectionDead = SQL_CD_TRUE;
		if (! m_CDInfoSent)
		{
/*			
			srvrCallContext.sqlHandle = this;
			srvrCallContext.ASSvc_ObjRef = m_ASSvc_ObjRef;
			srvrCallContext.SQLSvc_ObjRef = m_SQLSvc_ObjRef;
			srvrCallContext.srvrState = SRVR_ABENDED;
			retcode = resetSrvrState(&srvrCallContext);
			retcode = sendStopServer();
*/
			reset();
		}
	}
	else
		retcode = exception_nr;
	return retcode;
}

long CConnect::sendStopServer(odbcas_ASSvc_StopSrvr_exc_ *stopSrvrException)
{
	long retcode=CEE_SUCCESS;

	m_SrvrCallContext.sqlHandle = this;
	m_SrvrCallContext.ASSvc_ObjRef = m_ASSvc_ObjRef;
	m_SrvrCallContext.SQLSvc_ObjRef = m_SQLSvc_ObjRef;
	m_SrvrCallContext.dialogueId = m_DialogueId;
	retcode = stopSrvrAbrupt(&m_SrvrCallContext,stopSrvrException);

	return retcode;
}

SQLRETURN CConnect::Connect(SQLCHAR *ServerName, 
		   SQLSMALLINT	NameLength1,
           SQLCHAR		*UserName, 
		   SQLSMALLINT	NameLength2,
           SQLCHAR		*Authentication, 
		   SQLSMALLINT	NameLength3,
		   BOOL			readDSN)
{
	char					buffer[256];
    char                    pwdbuffer[256] = {0};
	char                    *lasts;
	short					retCode = SQL_SUCCESS;
	HWND					currentWindow;
	TCHAR					windowText[128];
	CONNECTION_CONTEXT_def	inContext;
	USER_DESC_def			userDesc;
	SQLRETURN				rc, rc1;
	char					ServerNameNTS[SQL_MAX_DSN_LENGTH+1];
	char					UserNameNTS[MAX_SQL_IDENTIFIER_LEN+1];
//Arvind: Modified to hold password of "128 Chars,128 Chars, 128 Chars" (two ","s + one '\0')
	char					AuthenticationNTS[3*MAX_SQL_IDENTIFIER_LEN+3];
	VERSION_def version[2];
	VERSION_def* versionPtr = &version[0];
	char tmpUserName[UNLEN+1];

	SQLINTEGER translen=0;
	char textError[MAX_TRANSLATE_ERROR_MSG_LEN];
	char transError[MAX_TRANSLATE_ERROR_MSG_LEN];
	char errorMsg[MAX_TRANSLATE_ERROR_MSG_LEN/4];
	char tmpBuf[MAX_SQL_IDENTIFIER_LEN*4+1];

	clearError();

	m_CurrentOdbcAPI = SQL_API_SQLCONNECT;

	if (NameLength1 == SQL_NTS)
	{
		strncpy(ServerNameNTS, (const char *)ServerName,sizeof(ServerNameNTS));
		ServerNameNTS[sizeof(ServerNameNTS)-1] = '\0';
	}
	else
	if (NameLength1 < sizeof(ServerNameNTS))
	{
		strncpy(ServerNameNTS, (const char *)ServerName, NameLength1);
		ServerNameNTS[NameLength1] = '\0';
	}
	else
	{
		setDiagRec(DRIVER_ERROR, IDS_IM_010);
		return SQL_ERROR;
	}

	if (UserName != NULL)
	{
		if (NameLength2 == SQL_NTS)
		{
			strncpy(UserNameNTS, (const char *)UserName,sizeof(UserNameNTS));
			UserNameNTS[sizeof(UserNameNTS)-1] = '\0';
		}
		else
		if (NameLength2 < sizeof(UserNameNTS))
		{
			strncpy(UserNameNTS, (const char *)UserName, NameLength2);
			UserNameNTS[NameLength2] = '\0';
		}
		else
		{
			setDiagRec(DRIVER_ERROR, IDS_HY_090);
			return SQL_ERROR;
		}
	}
	else
		UserNameNTS[0] = '\0';

	if (Authentication != NULL)
	{
		if (NameLength3 == SQL_NTS)
		{
			strncpy(AuthenticationNTS, (const char *)Authentication,sizeof(AuthenticationNTS));
			AuthenticationNTS[sizeof(AuthenticationNTS)-1] = '\0';
		}
		else
		if (NameLength3 < sizeof(AuthenticationNTS))
		{
			strncpy(AuthenticationNTS, (const char *)Authentication, NameLength3);
			AuthenticationNTS[NameLength3] = '\0';
		}
		else
		{
			setDiagRec(DRIVER_ERROR, IDS_HY_090);
			return SQL_ERROR;
		}
	}
	else
		AuthenticationNTS[0] = '\0';
	
	if (readDSN)
	{
		// Read DS Values (DS can now be only System DSN or User DSN)
		retCode = m_DSValue.readDSValues(ServerNameNTS,this);

		switch (retCode)
		{
		case ERROR_SUCCESS:
			clearError();
			break;
		case DS_AS_KEY_NOT_FOUND:
			sprintf(buffer," %s",getODBCDsnFileName());
			setDiagRec(DRIVER_ERROR, IDS_08_001_01, 0, buffer);
			return SQL_ERROR;
		case DS_NOT_FOUND:
			sprintf(buffer," %s",getODBCDsnFileName());
			setDiagRec(DRIVER_ERROR, IDS_IM_002, 0, buffer);
			return SQL_ERROR;
		case DS_AS_PROCESS_NAME_INCORRECT:
			sprintf(buffer," Incorrect Format of AS Process Name in: %s",getODBCDsnFileName());
			setDiagRec(DRIVER_ERROR, IDS_08_001, 0, buffer);
			return SQL_ERROR;
		case DS_FILE_SYSTEM_ERROR:
			sprintf(buffer," %s (%d): %s", DecodeERRNOErrors(getODBCDsnError()), getODBCDsnError(), getODBCDsnFileName()); 
			setDiagRec(DRIVER_ERROR, IDS_08_001, 0, buffer);
			return SQL_ERROR;
		case DS_INTERNAL_ERROR:
			sprintf(buffer," DSN parser internal error: %s", getODBCDsnFileName()); 
			setDiagRec(DRIVER_ERROR, IDS_08_001, 0, buffer);
			return SQL_ERROR;
		default:
			sprintf(buffer," unknown retcode=%d %s (%d) %s", retCode, DecodeERRNOErrors(getODBCDsnError()), getODBCDsnError(), getODBCDsnFileName()); 
			setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);
			return SQL_ERROR;
		}
		//set the client locale or the active codepage
		m_ICUConv->setClientLocale(m_DSValue.m_DSCharSet);
	}
	m_ICUConv->setUCS2Translation(m_DSValue.m_DS_UCS2Translation);

	purgeError();
	// Update the connection attributes from DSValues if it is not set already 
	if (m_CurrentCatalog[0] == '\0')
		strcpy(m_CurrentCatalog, m_DSValue.m_DSCatalog);
	if (m_CurrentSchema[0] == '\0')
		strcpy(m_CurrentSchema, m_DSValue.m_DSSchema);
	if (m_ConnectionTimeout == DRVR_PENDING)
		m_ConnectionTimeout = m_DSValue.m_DSConnectionTimeout;
	if (m_LoginTimeout == DRVR_PENDING)
		m_LoginTimeout = m_DSValue.m_DSLoginTimeout;
	if (m_TranslateLib[0] == '\0')
		strcpy(m_TranslateLib, m_DSValue.m_DSTranslationDLL);
	if (m_TranslateOption == DRVR_PENDING)
		m_TranslateOption = m_DSValue.m_DSTranslationOption;
	m_QueryTimeout = m_DSValue.m_DSQueryTimeout;
	m_SelectRowsets = m_DSValue.m_DSSelectRowsets;

	m_IOCompression = m_DSValue.m_DSIOCompression;
	if (m_asTCPIPSystem != NULL)
		m_asTCPIPSystem->m_IOCompression = m_IOCompression;
	if (m_srvrTCPIPSystem != NULL)
		m_srvrTCPIPSystem->m_IOCompression = m_IOCompression;

	if (m_asTCPIPSystem != NULL)
		m_asTCPIPSystem->m_IOCompressionThreshold = m_DSValue.m_DSIOCompressionThreshold;
	if (m_srvrTCPIPSystem != NULL)
		m_srvrTCPIPSystem->m_IOCompressionThreshold= m_DSValue.m_DSIOCompressionThreshold;
	// Set the Assocation Service Object Reference
	strcpy(m_ASSvc_ObjRef, m_DSValue.m_DSServer);
	strcpy(buffer,m_DSValue.m_DSServer);
#ifndef unixcli
	if (strtok(buffer, ":") != NULL) 
#else
  	if (strtok_r(buffer, ":", &lasts) != NULL)
#endif
	{
		if (stricmp(buffer,"file")==0)
			m_TransportType = TR_FILE_SYSTEM;
#ifndef unixcli
		else if (stricmp(buffer,"local")==0)
			m_TransportType = TR_LOCAL;
#else
		else if (stricmp(buffer,"TCP")==0)		
		{
			m_TransportType = TR_FILE_SYSTEM;
		}
#endif
		else
		{
			setDiagRec(DRIVER_ERROR, IDS_08_001, 0, " Incorrect Format of AS Process Name.");
			return SQL_ERROR;
		}
	}
	else
	{
		setDiagRec(DRIVER_ERROR, IDS_08_001, 0, " Incorrect Format of AS Process Name.");
		return SQL_ERROR;
	}
	m_RowsetErrorRecovery = m_DSValue.m_DSRowsetErrorRecovery;
	m_FetchBufferSize = m_DSValue.m_DSFetchBufferSize;
	m_FlushFetchData = m_DSValue.m_DSFlushFetchData;

	// populate the inContext
	inContext.location[0] = '\0';
	inContext.userRole[0] = '\0';
    inContext.sessionName[0] = '\0';

	// we need to keep the following in UTF8 internally
	// m_DSValue.m_DSSchema, m_DSValue.m_DSServerDSName, m_DSValue.m_DSName
	
	strcpy(inContext.datasource, m_DSValue.m_DSServerDSName);
	strcpy(inContext.datasource, m_DSValue.m_DSName);
	strcpy( inContext.schema, m_DSValue.m_DSSchema);				
		
	strcpy(inContext.catalog, m_CurrentCatalog);

	inContext.accessMode = m_AccessMode;
	inContext.autoCommit = m_AutoCommit; 
	inContext.queryTimeoutSec = m_ConnectionTimeout;
	inContext.idleTimeoutSec = 0;
	inContext.loginTimeoutSec = m_LoginTimeout;
	inContext.txnIsolationLevel = m_TxnIsolation;
	if (m_FetchBufferSize < 0)
		inContext.rowSetSize = 0;
	else
		inContext.rowSetSize = (short)(m_FetchBufferSize/1024);
	inContext.diagnosticFlag = 0;
// it might be a good idea to move these things into the g_Environment stuct! 
	inContext.processId = (long)getpid();

//10-080313-1379
	int result = gethostname(gDrvrGlobal.gComputerName, sizeof(gDrvrGlobal.gComputerName));
	if (result == -1) 
	{
		if (errno == EINVAL || errno == ENAMETOOLONG) 
			gDrvrGlobal.gComputerName[60] = '\0';	
		else
		{
			strncpy(inContext.computerName, "UNKNOWN", sizeof(inContext.computerName));
			inContext.computerName[sizeof(inContext.computerName) - 1] = '\0';
		}
	}
	strncpy(inContext.computerName, gDrvrGlobal.gComputerName, sizeof(inContext.computerName));
	inContext.computerName[sizeof(inContext.computerName) - 1] = '\0';


	if (GetWindowText(currentWindow, windowText, sizeof(windowText)) == 0)
#ifdef MXHPUX
		strcpy(windowText, "HPUXODBC");
#elif defined MXOSS
		strcpy(windowText, "OSSODBC");
#elif defined MXAIX
		strcpy(windowText, "AIXODBC");
#elif defined MXSUNSPARC
	#if defined __LP64__
		strcpy(windowText, "SUNSPARCODBC64");
	#else
		strcpy(windowText, "SUNSPARCODBC32");
	#endif
#elif defined (MXLINUX) || defined(MXIA64LINUX)
		strcpy(windowText,"LINUXODBC");
#else
		strcpy(windowText,"UNKNOWN");
#endif

	//wms_mapping
	if(m_applName[0] != '\0') 
	{
		inContext.inContextOptions2 |= MAPPING_APPL;
		strcpy(windowText, m_applName); 
	}
	else if(m_DSValue.m_DSApplication[0] != '\0') 
	{
		inContext.inContextOptions2 |= MAPPING_APPL;
		strcpy(windowText, m_DSValue.m_DSApplication); 
	}
	inContext.windowText = windowText;

	ODBCNLS_GetCodePage ((unsigned int*)&inContext.ctxACP);
	inContext.ctxACP = -1;

	inContext.ctxDataLang = SQLCHARSETCODE_UTF8;
	inContext.ctxErrorLang = SQLCHARSETCODE_ISO88591;

// this check needs to be done so that the first server gets the correct
// starting node!  If it is not in streaming mode we should send -1 to the server!
	if (m_ModeLoader == 0)
	{
		if(m_StartNode == -1)
			inContext.cpuToUse = -1;
		else
			inContext.cpuToUse = m_StartNode;
	}
	else
	{
		if(m_StartNode == -1)
			m_StartNode = STREAM_START_NODE;
		inContext.cpuToUse = m_StartNode;
	}
	inContext.cpuToUseEnd = m_EndNode;

	inContext.connectOptions = NULL;
	inContext.inContextOptions1 = 0;
	inContext.inContextOptions2 = 0;

	//wms_mapping
	if(m_QueryID_SessionName[0] != '\0')
	{
		inContext.inContextOptions1 = inContext.inContextOptions1 | INCONTEXT_OPT1_SESSIONNAME;
		inContext.inContextOptions2 |= MAPPING_SESSION;
		strcpy(inContext.sessionName,m_QueryID_SessionName);
	}
	else if(m_DSValue.m_DSSession[0] != '\0') 
	{
		inContext.inContextOptions1 = inContext.inContextOptions1 | INCONTEXT_OPT1_SESSIONNAME;
		inContext.inContextOptions2 |= MAPPING_SESSION;
		strcpy(inContext.sessionName,m_DSValue.m_DSSession);
	}

    if(m_FetchAhead == 1)
       inContext.inContextOptions1 = inContext.inContextOptions1 | INCONTEXT_OPT1_FETCHAHEAD;

	if(strlen(m_UserRole) > 0)
	{
		strcpy(inContext.userRole,m_UserRole);
	}
	// Set the version no of ODBC Driver
	inContext.clientVersionList._length = 2; 
	inContext.clientVersionList._buffer = versionPtr; 
	// Set the componentId to UNICODE_DRVR_COMPONENT if the app type is UTF16
	//if(m_ICUConv->m_AppUnicodeType == APP_UNICODE_TYPE_UTF16)
#if defined(MXLINUX)
		versionPtr->componentId = LINUX_UNICODE_DRVR_COMPONENT;
#elif defined(MXHPUX)
		versionPtr->componentId = HPUX_UNICODE_DRVR_COMPONENT;
#endif
	//else
	//	versionPtr->componentId = gDrvrGlobal.gClientVersion.componentId; 

	versionPtr->majorVersion = gDrvrGlobal.gClientVersion.majorVersion;
	versionPtr->minorVersion = gDrvrGlobal.gClientVersion.minorVersion;
	if(m_ModeLoader > 0) {
       if(m_StreamDelayedError > 0)
	      versionPtr->buildId = STREAMING_DELAYEDERROR_MODE | ROWWISE_ROWSET | CHARSET | PASSWORD_SECURITY;
	   else
	      versionPtr->buildId = STREAMING_MODE | ROWWISE_ROWSET | CHARSET | PASSWORD_SECURITY;
	   gDrvrGlobal.gClientVersion.buildId = versionPtr->buildId; // drvrnet.cpp uses this hack
	}
	else 
	{
	   gDrvrGlobal.gClientVersion.buildId = 0;
#ifndef MXOSS
	   versionPtr->buildId = gDrvrGlobal.gClientVersion.buildId | ROWWISE_ROWSET | CHARSET | PASSWORD_SECURITY;
#else
	   versionPtr->buildId = gDrvrGlobal.gClientVersion.buildId | ROWWISE_ROWSET | CHARSET;
#endif
	}
	versionPtr++;
	versionPtr->componentId = APP_COMPONENT; 
	versionPtr->majorVersion = m_EnvHandle->getODBCAppVersion();
	versionPtr->minorVersion = 0;
    versionPtr->buildId = 0;
	memcpy(inContext.clientVproc,gDrvrGlobal.gClientVproc,sizeof(gDrvrGlobal.gClientVproc));

	// populate the userDesc
	if (! getUserDesc(UserNameNTS, AuthenticationNTS, &userDesc))
		return SQL_ERROR;

	// Get client user name.
    struct passwd passwd;           /* man getpwuid */
    memset(&passwd,'\0',sizeof(struct passwd));
    struct passwd* tempPwdPtr;
    if((getpwuid_r(getuid(),&passwd,pwdbuffer,sizeof(pwdbuffer),&tempPwdPtr)) == 0)
	{
		inContext.clientUserName = passwd.pw_name;
		inContext.inContextOptions1 = inContext.inContextOptions1 | INCONTEXT_OPT1_CLIENT_USERNAME;
	}
	else
		inContext.clientUserName = NULL;
		
	m_ConnectionDead = SQL_CD_TRUE;
	m_CDInfoSent = TRUE;				// May need to investigate

	// populate the SrvrCallContext
	m_SrvrCallContext.initContext();
	m_SrvrCallContext.sqlHandle = this;
	m_SrvrCallContext.ASSvc_ObjRef = m_ASSvc_ObjRef;
	m_SrvrCallContext.eventHandle = m_ConnectEvent;
	m_SrvrCallContext.u.connectParams.loginTimeout = m_LoginTimeout;
	m_SrvrCallContext.u.connectParams.inContext = &inContext;
	m_SrvrCallContext.u.connectParams.userDesc = &userDesc;

retryGetObjRef:

	m_SrvrCallContext.odbcAPI = SQL_API_GETOBJREF;
	m_SrvrCallContext.SQLSvc_ObjRef = NULL;
	
	rc = ThreadControlProc(&m_SrvrCallContext);
	if (rc == SQL_ERROR)
		return rc;

	//
	//	Initialize Dialog
	//

	m_SrvrCallContext.odbcAPI = SQL_API_SQLCONNECT;
	m_SrvrCallContext.SQLSvc_ObjRef = m_SQLSvc_ObjRef;
	m_SrvrCallContext.dialogueId = m_DialogueId;

	if (m_SecurityMode == 1) // for pwd security
	{
		DoEncryption(m_SecPwd,m_SecInfo,userDesc,AuthenticationNTS,inContext);
	}
	

	rc = ThreadControlProc(&m_SrvrCallContext);

	if (m_RetryEncryption  && m_SecurityMode == 1)
	{
		m_RetryEncryption = false;
		retCode = SQL_SUCCESS;
		userDesc.userName = m_UserName;
		if(userDesc.password._buffer != NULL)
		{
			delete[] userDesc.password._buffer;
			userDesc.password._buffer=NULL;
		}
		DoEncryption(m_SecPwd,m_SecInfo,userDesc,AuthenticationNTS,inContext);
		rc = ThreadControlProc(&m_SrvrCallContext);
		if(userDesc.password._buffer != NULL)
		{
			delete[] userDesc.password._buffer;
			userDesc.password._buffer=NULL;
		}
	}

	switch (rc)
	{
	case SQL_STILL_EXECUTING:
		goto retryGetObjRef;
		break;
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:

		// Return SQL_SUCCESS_WITH_INFO if the datasource names are not same
		if (m_DSValue.m_DSServerDSName[0] != 0)
		{
			if (strcmp(m_DSValue.m_DSServerDSName, m_SrvrDSName) != 0)
			{
				setDiagRec(DRIVER_ERROR, IDS_01_000_01, 0, NULL, NULL, SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 1, m_SrvrDSName);
				rc = SQL_SUCCESS_WITH_INFO;
			}
		}
		else
		if (strcmp(m_DSValue.m_DSName, m_SrvrDSName) != 0)
		{
			setDiagRec(DRIVER_ERROR, IDS_01_000_01, 0, NULL, NULL, SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 1, m_SrvrDSName);
			rc = SQL_SUCCESS_WITH_INFO;
		}
		// update the ConnectionDead values
		m_ConnectionDead = SQL_CD_FALSE;
		m_CDInfoSent	= FALSE;
		break;
	default:
		break;
	}


	if(rc1 == SQL_ERROR)
		rc1 = SQL_SUCCESS_WITH_INFO;

	if (rc != SQL_ERROR)
	{
		rc1 = SetConnectAttr(SQL_ATTR_ROWSET_RECOVERY, (SQLUINTEGER)m_RowsetErrorRecovery, NULL, false);
		if(rc1 != SQL_SUCCESS)
			rc = rc1;
	}

	if(rc == SQL_ERROR)
		clearUserName();
	if(userDesc.password._buffer  != NULL)
	{
	    if(m_SecurityMode == 1)
			delete[] userDesc.password._buffer;
		userDesc.password._length = 0;
		userDesc.password._buffer = NULL;
	}
	
	return rc;
}


SQLRETURN CConnect::Disconnect()
{
	SQLRETURN				rc = SQL_SUCCESS;

	clearError();
// If SQLBrowseConnect returns SQL_NEED_DATA
	if ( m_CurrentOdbcAPI == SQL_API_SQLBROWSECONNECT)
	{
		m_CurrentOdbcAPI = SQL_API_SQLDISCONNECT;
	}
	else
	{
		m_CurrentOdbcAPI = SQL_API_SQLDISCONNECT;
		if (m_ConnectionDead == SQL_CD_TRUE)
		{
			setDiagRec(DRIVER_ERROR, IDS_08_S01, -20005);
			rc = SQL_SUCCESS_WITH_INFO;
		}
		else
		{
#ifdef ASYNCIO
			if(m_async_IOthread != NULL && m_async_IOthread->getStatementHandle() != NULL)
			{
				setDiagRec(DRIVER_ERROR, IDS_HY_010);
				return SQL_ERROR;
			}
#endif
			m_SrvrCallContext.initContext();
			m_SrvrCallContext.odbcAPI = SQL_API_SQLDISCONNECT;
			m_SrvrCallContext.sqlHandle = this;
			m_SrvrCallContext.SQLSvc_ObjRef = m_SQLSvc_ObjRef;
			m_SrvrCallContext.ASSvc_ObjRef = m_ASSvc_ObjRef;
			m_SrvrCallContext.eventHandle = m_ConnectEvent;
			m_SrvrCallContext.dialogueId = m_DialogueId;
			m_SrvrCallContext.connectionTimeout = m_ConnectionTimeout;
			rc = ThreadControlProc(&m_SrvrCallContext);
			if (rc == -25000)
				return SQL_ERROR;
			rc = SQL_SUCCESS;

#ifdef VERSION3
			while (!m_StmtCollect.empty())
				delete (CStmt *)m_StmtCollect.front();
			while (!m_DescCollect.empty())
				delete (CDesc *)m_DescCollect.front();
#else
			while (!m_StmtCollect.isEmpty())
				delete (CStmt *)m_StmtCollect.first();

			while (!m_DescCollect.isEmpty())
				delete (CDesc *)m_DescCollect.first();
#endif
		}
	}
	reset(rc == SQL_SUCCESS );
	m_ConnectionDead = SQL_CD_TRUE;
	m_CDInfoSent = TRUE;

	m_asTCPIPSystem->resetSwap();
	m_srvrTCPIPSystem->resetSwap();

	return rc;
} // CConnect::Disconnect()

SQLRETURN CConnect::generateConnectionString(CONNECT_KEYWORD_TREE *KeywordTree, 
				CONNECT_FIELD_ITEMS	*ConnectFieldItems,
				short				DSNAttrNo, 
				SQLCHAR				*OutConnectionString,
				SQLSMALLINT			BufferLength,
				SQLSMALLINT 		*StringLength2Ptr)
{
	SQLRETURN		rc = SQL_SUCCESS;
	char			tempBuffer[512];
	char			*equalString = "=";
	char			*colonString = ";";
	SQLSMALLINT		strLen;
	SQLSMALLINT		lengthCopied;
	SQLSMALLINT		totalLength;
	short			i;

	tempBuffer[0] = 0;
	// Write DSN or DRIVER name
	if (DSNAttrNo < KEY_MAX)
	{
		strcpy(tempBuffer, ConnectKeywords[DSNAttrNo]);
		strcat(tempBuffer, equalString);
		strncat(tempBuffer, KeywordTree[DSNAttrNo].AttrValue, KeywordTree[DSNAttrNo].AttrLength);
		strcat(tempBuffer, colonString);
	}
	
	// set the index to i = KEY_SERVER to start copying the attributes from then onwards till KEY_MAX
	i = KEY_SERVER;
	lengthCopied = 0;
	totalLength = 0;
	do
	{
		strLen = strlen(tempBuffer);
		totalLength += strLen;
		if (strLen != 0 && BufferLength != 0 && lengthCopied < BufferLength-1)
		{
			if (BufferLength == SQL_NTS || (BufferLength > 0 && strLen+lengthCopied < BufferLength))
			{
				strcpy((char *)(OutConnectionString+lengthCopied), tempBuffer);
				lengthCopied += strLen;
			}
			else
			{
				strLen = BufferLength-lengthCopied-1;
				strncpy((char *)(OutConnectionString+lengthCopied), tempBuffer, strLen);
				((char *)OutConnectionString)[BufferLength-1] = '\0';
				lengthCopied = BufferLength-1;
				setDiagRec(DRIVER_ERROR, IDS_01_004);
				rc = SQL_SUCCESS_WITH_INFO;
			}
		}
		tempBuffer[0] = '\0';
		switch (i)
		{
		case KEY_SAVEFILE:
			if (KeywordTree[i].AttrRank != 0)
			{
				strcpy(tempBuffer, ConnectKeywords[i]);
				strcat(tempBuffer, equalString);
				strncat(tempBuffer, KeywordTree[i].AttrValue, KeywordTree[DSNAttrNo].AttrLength);
				strcat(tempBuffer, colonString);
			}
			break;
		case KEY_UID:
			sprintf(tempBuffer, "%s=%s;", ConnectKeywords[i], ConnectFieldItems->loginId);
			break;
		case KEY_PWD:
			sprintf(tempBuffer, "%s=%s;", ConnectKeywords[i], ConnectFieldItems->password);
			break;
		case KEY_CATALOG:
			sprintf(tempBuffer, "%s=%s;", ConnectKeywords[i], ConnectFieldItems->catalog);
			break;
		case KEY_SCHEMA:
			sprintf(tempBuffer, "%s=%s;", ConnectKeywords[i], ConnectFieldItems->schema);
			break;
		case KEY_SERVER:
			if (ConnectFieldItems->server[0] != '\0')
				sprintf(tempBuffer, "%s=%s;", ConnectKeywords[i], ConnectFieldItems->server);
			break;
		default:
			break;
		}
	}
	while (i++ < KEY_MAX) ;
	if (StringLength2Ptr != NULL)
		*StringLength2Ptr = totalLength;
	return rc;
}

SQLRETURN CConnect::prepareForConnect(CONNECT_KEYWORD_TREE *KeywordTree, 
									  CONNECT_FIELD_ITEMS &connectFieldItems, 
									  short DSNAttrNo,
									  short	DSNType,
									  char	*ServerNameNTS)
{
	char	buffer[256];
	short	retCode;
	char   *lasts;

	// Initialize the Keyword Attributes
	connectFieldItems.loginId[0] = '\0';
	connectFieldItems.password[0] = '\0';
	connectFieldItems.schema[0] = '\0';
	connectFieldItems.catalog[0] = '\0';
	connectFieldItems.server[0] = '\0';
	switch (DSNType)
	{
	case UNKNOWN_DSN:
		if (DSNAttrNo != KEY_MAX)
		{
			if (KeywordTree[DSNAttrNo].AttrLength <= SQL_MAX_DSN_LENGTH)
			{
				strncpy(ServerNameNTS, (const char *)KeywordTree[DSNAttrNo].AttrValue, KeywordTree[DSNAttrNo].AttrLength);
				ServerNameNTS[KeywordTree[DSNAttrNo].AttrLength] = '\0';
			}
			else
			{
				setDiagRec(DRIVER_ERROR, IDS_IM_010);
				return SQL_ERROR;
			}
		}
		// Read DS Values (DS can now be only System DSN or User DSN)
		retCode = m_DSValue.readDSValues(ServerNameNTS, this);

		switch (retCode)
		{
		case ERROR_SUCCESS:
			clearError();
			break;
		case DS_AS_KEY_NOT_FOUND:
			sprintf(buffer," %s",getODBCDsnFileName());
			setDiagRec(DRIVER_ERROR, IDS_08_001_01, 0, buffer);
			return SQL_ERROR;
		case DS_NOT_FOUND:
			sprintf(buffer," %s",getODBCDsnFileName());
			setDiagRec(DRIVER_ERROR, IDS_IM_002, 0, buffer);
			return SQL_ERROR;
		case DS_AS_PROCESS_NAME_INCORRECT:
			sprintf(buffer," Incorrect Format of AS Process Name in: %s",getODBCDsnFileName());
			setDiagRec(DRIVER_ERROR, IDS_08_001, 0, buffer);
			return SQL_ERROR;
		case DS_FILE_SYSTEM_ERROR:
			sprintf(buffer," %s (%d): %s", DecodeERRNOErrors(getODBCDsnError()), getODBCDsnError(), getODBCDsnFileName()); 
			setDiagRec(DRIVER_ERROR, IDS_08_001, 0, buffer);
			return SQL_ERROR;
		default:
			sprintf(buffer," unknown retcode=%d %s (%d) %s", retCode, DecodeERRNOErrors(getODBCDsnError()), getODBCDsnError(), getODBCDsnFileName()); 
			setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);
			return SQL_ERROR;
		}
		//set the client locale or active code page
		m_ICUConv->setClientLocale(m_DSValue.m_DSCharSet);
		break;
	case DRIVER_KW_DSN:
		break;
	default:
		return SQL_ERROR;
	}
	if (KeywordTree[KEY_SERVER].AttrRank != 0)
	{
		strncpy(connectFieldItems.server, KeywordTree[KEY_SERVER].AttrValue,
					KeywordTree[KEY_SERVER].AttrLength);
		connectFieldItems.server[KeywordTree[KEY_SERVER].AttrLength] = '\0';
	}
	else
		strcpy(connectFieldItems.server, m_DSValue.m_DSServer);

	strcpy(buffer,connectFieldItems.server);
#ifndef unixcli
	if (strtok(buffer, ":") != NULL)
#else
	if (strtok_r(buffer, ":", &lasts) != NULL)
#endif
	{
		if (stricmp(buffer,"file")==0)
			m_TransportType = TR_FILE_SYSTEM;
#ifndef unixcli
		else if (stricmp(buffer,"local")==0)
			m_TransportType = TR_LOCAL;
#else
		else if (stricmp(buffer,"TCP")==0)
			m_TransportType = TR_TCPIP;
#endif
		else
		{
			setDiagRec(DRIVER_ERROR, IDS_08_001, 0, " Incorrect Format of AS Process Name.");
			return SQL_ERROR;
		}
	}
	else
	{
		setDiagRec(DRIVER_ERROR, IDS_08_001, 0, " Incorrect Format of AS Process Name.");
		return SQL_ERROR;
	}
	if (KeywordTree[KEY_ROWSET_ERROR_RECOVERY].AttrRank != 0)
	{
		strncpy(buffer, KeywordTree[KEY_ROWSET_ERROR_RECOVERY].AttrValue, KeywordTree[KEY_ROWSET_ERROR_RECOVERY].AttrLength);
		buffer[KeywordTree[KEY_ROWSET_ERROR_RECOVERY].AttrLength] = '\0';
		if (atol((const char *)buffer))
			m_RowsetErrorRecovery = TRUE;
		else
			m_RowsetErrorRecovery = FALSE;
	}
	else
		m_RowsetErrorRecovery = m_DSValue.m_DSRowsetErrorRecovery;

	if (KeywordTree[KEY_FETCHBUFFERSIZE].AttrRank != 0)
	{
		strncpy(buffer, KeywordTree[KEY_FETCHBUFFERSIZE].AttrValue, KeywordTree[KEY_FETCHBUFFERSIZE].AttrLength);
		buffer[KeywordTree[KEY_FETCHBUFFERSIZE].AttrLength] = '\0';
		m_FetchBufferSize = atol((const char *)buffer);
	}
	else
		m_FetchBufferSize = m_DSValue.m_DSFetchBufferSize;
#ifndef unixcli
	if (m_TransportType == TR_LOCAL)
		SetRowsetErrorRecovery(m_RowsetErrorRecovery);
#endif
	if (KeywordTree[KEY_CATALOG].AttrRank != 0)
	{
		strncpy(connectFieldItems.catalog, KeywordTree[KEY_CATALOG].AttrValue,
					KeywordTree[KEY_CATALOG].AttrLength);
		connectFieldItems.catalog[KeywordTree[KEY_CATALOG].AttrLength] = '\0';
	}
	else
		strcpy(connectFieldItems.catalog, m_DSValue.m_DSCatalog);
	
	if (KeywordTree[KEY_SCHEMA].AttrRank != 0)
	{
		if (KeywordTree[KEY_SCHEMA].AttrValue[0] == '{' && KeywordTree[KEY_SCHEMA].AttrValue[KeywordTree[KEY_SCHEMA].AttrLength-1] == '}')
		{
			int i,j;
			for (i=0,j=1; j<KeywordTree[KEY_SCHEMA].AttrLength-1; i++, j++)
			{
				connectFieldItems.schema[i] = KeywordTree[KEY_SCHEMA].AttrValue[j];
				if(KeywordTree[KEY_SCHEMA].AttrValue[j] == '}' && KeywordTree[KEY_PWD].AttrValue[j+1] == '}')
					j++; // The '}' character is escaped as '}}'
			}
			connectFieldItems.schema[i] = '\0';
		}
		else
		{
			strncpy(connectFieldItems.schema, KeywordTree[KEY_SCHEMA].AttrValue,
						KeywordTree[KEY_SCHEMA].AttrLength);
			connectFieldItems.schema[KeywordTree[KEY_SCHEMA].AttrLength] = '\0';
		}
	}
	else
		strcpy(connectFieldItems.schema, m_DSValue.m_DSSchema);
		
	if (KeywordTree[KEY_UID].AttrRank != 0)
	{
		if (KeywordTree[KEY_UID].AttrValue[0] == '{' && KeywordTree[KEY_UID].AttrValue[KeywordTree[KEY_UID].AttrLength-1] == '}')
		{
			int i,j;
			for (i=0, j=1; j<KeywordTree[KEY_UID].AttrLength-1; i++, j++)
			{
				connectFieldItems.loginId[i] = KeywordTree[KEY_UID].AttrValue[j];
				if(KeywordTree[KEY_UID].AttrValue[j] == '}' && KeywordTree[KEY_UID].AttrValue[j+1] == '}')
					j++; // The '}' character is escaped as '}}'	
			}
			connectFieldItems.loginId[i] = '\0';
		}
		else
		{		
			strncpy(connectFieldItems.loginId, KeywordTree[KEY_UID].AttrValue,
						KeywordTree[KEY_UID].AttrLength);
			connectFieldItems.loginId[KeywordTree[KEY_UID].AttrLength] = '\0';
		}
	}
	else
		connectFieldItems.loginId[0] = '\0';
	if (KeywordTree[KEY_PWD].AttrRank != 0)
	{
		if (KeywordTree[KEY_PWD].AttrValue[0] == '{' && KeywordTree[KEY_PWD].AttrValue[KeywordTree[KEY_PWD].AttrLength-1] == '}')
		{
			int i,j;
			for (i=0, j=1; j<KeywordTree[KEY_PWD].AttrLength-1; i++, j++)
			{
				connectFieldItems.password[i] = KeywordTree[KEY_PWD].AttrValue[j];
				if(KeywordTree[KEY_PWD].AttrValue[j] == '}' && KeywordTree[KEY_PWD].AttrValue[j+1] == '}')
					j++; // The '}' character is escaped as '}}'	
			}
			connectFieldItems.password[i] = '\0';
		}
		else
		{
			strncpy(connectFieldItems.password, KeywordTree[KEY_PWD].AttrValue,
						KeywordTree[KEY_PWD].AttrLength);
			connectFieldItems.password[KeywordTree[KEY_PWD].AttrLength] = '\0';
		}
	}
	else
		connectFieldItems.password[0] = '\0';
	return SQL_SUCCESS;
}

SQLRETURN CConnect::AllocHandle(SQLSMALLINT HandleType, 
			SQLHANDLE InputHandle, 
			SQLHANDLE *OutputHandle)
{
	SQLRETURN				rc = SQL_SUCCESS;
	CStmt* pStmt;

	switch (HandleType)
	{
	case SQL_HANDLE_STMT:
		pStmt = new CStmt(InputHandle);
		if (pStmt != NULL)
		{
			if ((rc = pStmt->initialize()) == SQL_SUCCESS)
			{
				//10-080909-5745 hack: need the connection state definition for en_dbc_hstmt, from drvrmanager.h
				#define en_dbc_hstmt 3
				state = en_dbc_hstmt;
				*OutputHandle = (SQLHANDLE)pStmt;
			}
			else
			{
				delete pStmt;
				rc = SQL_ERROR;
			}
		}
		else
		{
			setDiagRec(DRIVER_ERROR, IDS_HY_001);
			rc = SQL_ERROR;
		}
		break;
	case SQL_HANDLE_DESC:
		*OutputHandle = (SQLHANDLE)new CDesc(InputHandle, SQL_DESC_MODE_UNKNOWN, SQL_DESC_ALLOC_USER);
		if (*OutputHandle != NULL)
		{

#ifdef VERSION3
			m_DescCollect.push_back((CDesc*)(*OutputHandle));
#else
			m_DescCollect.append((CDesc*)(*OutputHandle));
#endif

		}
		else
		{
			setDiagRec(DRIVER_ERROR, IDS_HY_001);
			rc = SQL_ERROR;
		}
		break;
	default:
		rc = SQL_ERROR;
		break;
	}
	return rc;
}
	
SQLRETURN CConnect::BrowseConnect(SQLCHAR *InConnectionString,
			SQLSMALLINT        StringLength1,
			SQLCHAR 		  *OutConnectionString,
			SQLSMALLINT        BufferLength,
			SQLSMALLINT       *StringLength2Ptr)
{

	SQLRETURN				rc = SQL_SUCCESS;
	SQLRETURN				rc1;
	CONNECT_KEYWORD_TREE	KeywordTree[KEY_MAX+1];
	CONNECT_FIELD_ITEMS		connectFieldItems;
	short					outputKeyword[KEY_MAX];
	short					i;
	short					DSNAttrNo;					
	short					DSNType;
	short					inStrLen;
	char					lcConnectionString[2048+1];
	char					ServerNameNTS[SQL_MAX_DSN_LENGTH+1];
	char					tempBuffer[512];
	SQLSMALLINT				lengthCopied;
	SQLSMALLINT				totalLength;
	SQLSMALLINT				strLen;
		
	clearError();
	if (OutConnectionString != NULL)
		*OutConnectionString = 0;
	if (StringLength2Ptr != NULL)
		*StringLength2Ptr = 0;

	if (InConnectionString != NULL)
	{
		if (StringLength1 == SQL_NTS)
			inStrLen = strlen((const char *)InConnectionString);
		else
			inStrLen = StringLength1;
		if (inStrLen > sizeof(lcConnectionString)-1)
		{
			setDiagRec(DRIVER_ERROR, IDS_HY_013);
			return SQL_ERROR;
		}
		strncpy(lcConnectionString, (const char *)InConnectionString, inStrLen);
		lcConnectionString[inStrLen] = '\0';

	}
	else
	{
		strcpy(lcConnectionString, "DSN=DEFAULT;");
		inStrLen = strlen(lcConnectionString);
	}	
	
	ScanConnectString(lcConnectionString, inStrLen, KeywordTree);
	
	gDrvrGlobal.gClientApplication[0] = 0;
	if (KeywordTree[KEY_PCAPP].AttrRank != 0)
		strncpy(gDrvrGlobal.gClientApplication,KeywordTree[KEY_PCAPP].AttrValue,KeywordTree[KEY_PCAPP].AttrLength);
	if (KeywordTree[KEY_DSN].AttrRank != 0)
	{
		DSNAttrNo = KEY_DSN;
		DSNType = UNKNOWN_DSN;
	}
	else
	if (KeywordTree[KEY_DRIVER].AttrRank != 0)
	{
		DSNAttrNo = KEY_DRIVER;		
		DSNType = DRIVER_KW_DSN;
	}
	else
	{
		DSNType = UNKNOWN_DSN;
		strcpy(ServerNameNTS, "DEFAULT");
		DSNAttrNo = KEY_MAX;
	}
	for (i = 0; i < KEY_MAX ; i++)
		outputKeyword[i] = KEY_MAX;
	i = 0;
	if (KeywordTree[KEY_UID].AttrRank == 0)
	{
		outputKeyword[i++] = KEY_UID;
		rc = SQL_NEED_DATA;
	}
	if (KeywordTree[KEY_PWD].AttrRank == 0)
	{
		outputKeyword[i++] = KEY_PWD;
		rc = SQL_NEED_DATA;
	}
	if (KeywordTree[KEY_DRIVER].AttrRank != 0)
	{
		if (KeywordTree[KEY_SERVER].AttrRank == 0)
		{
			outputKeyword[i++] = KEY_SERVER;
			rc = SQL_NEED_DATA;
		}
		if (KeywordTree[KEY_CATALOG].AttrRank == 0)
		{
			outputKeyword[i++] = KEY_CATALOG;
			rc = SQL_NEED_DATA;
		}
		if (KeywordTree[KEY_SCHEMA].AttrRank == 0)
		{
			outputKeyword[i++] = KEY_SCHEMA;
			rc = SQL_NEED_DATA;
		}
	}
	if (rc != SQL_NEED_DATA)
	{
		if ((rc = prepareForConnect(KeywordTree, connectFieldItems, DSNAttrNo, DSNType, ServerNameNTS)) != SQL_ERROR)
		{
			rc = Connect((SQLCHAR *)ServerNameNTS,
				SQL_NTS,
				(SQLCHAR *)connectFieldItems.loginId,
				SQL_NTS,
				(SQLCHAR *)connectFieldItems.password,
				SQL_NTS,
				FALSE);
		}
		if (rc != SQL_ERROR)
		{
			rc1 = generateConnectionString((CONNECT_KEYWORD_TREE *)&KeywordTree, &connectFieldItems, DSNAttrNo, OutConnectionString, 
				BufferLength, StringLength2Ptr);
			if (rc1 == SQL_SUCCESS_WITH_INFO)
				rc = rc1;
		}
	}
	else
	{
		lengthCopied = 0;
		totalLength = 0;

		for (i = 0; i < KEY_MAX && outputKeyword[i] != KEY_MAX ; i++)
		{
			sprintf(tempBuffer, "%s:%s=?;", ConnectKeywords[outputKeyword[i]],
											ConnectLocalizedIdentifier[outputKeyword[i]]);
			strLen = strlen(tempBuffer);
			totalLength += strLen;
			if (strLen != 0 && BufferLength != 0 && lengthCopied < BufferLength-1)
			{
				if (BufferLength == SQL_NTS || (BufferLength > 0 && strLen+lengthCopied < BufferLength))
				{
					if (OutConnectionString != NULL)
						strcpy((char *)(OutConnectionString+lengthCopied), tempBuffer);
					lengthCopied += strLen;
				}
				else
				{
					strLen = BufferLength-lengthCopied-1;
					if (OutConnectionString != NULL)
					{
						strncpy((char *)(OutConnectionString+lengthCopied), tempBuffer, strLen);
						((char *)OutConnectionString)[BufferLength-1] = '\0';
					}
					lengthCopied = BufferLength-1;
					setDiagRec(DRIVER_ERROR, IDS_01_004);
				}
			}
		}
		if (StringLength2Ptr != NULL)
			*StringLength2Ptr = totalLength;
	}
	if (rc == SQL_NEED_DATA) 
		m_CurrentOdbcAPI = SQL_API_SQLBROWSECONNECT;
	return rc;
}

SQLRETURN CConnect::DriverConnect(SQLHWND WindowHandle,
			SQLCHAR			*InConnectionString,
			SQLSMALLINT     StringLength1,
			SQLCHAR         *OutConnectionString,
			SQLSMALLINT     BufferLength,
			SQLSMALLINT 	*StringLength2Ptr,
			SQLUSMALLINT    DriverCompletion)
{
	CONNECT_KEYWORD_TREE	KeywordTree[KEY_MAX+1];
	CONNECT_FIELD_ITEMS		connectFieldItems;
	BOOL					reqAttrGiven = TRUE;
	short					DSNAttrNo;					
	short					DSNType;
	SQLRETURN				rc;
	SQLRETURN				rc1;
	short					inStrLen;
	char					lcConnectionString[2048+1];
	int						DialogRetCode;
	char					ServerNameNTS[SQL_MAX_DSN_LENGTH+1];
	clearError();

	if (OutConnectionString != NULL)
		*OutConnectionString = 0;
	if (StringLength2Ptr != NULL)
		*StringLength2Ptr = 0;
	
	// FILEDSN is implemented by DM, DM reads the file datasource and gives it us either with DRIVER 
	// keyword or DSN keyword or both. Refer Driver Manager Guidelines under SQLDriverConnect
	// The code below attempts to follow Driver Guidelines under SQLDriverConnect
	if (InConnectionString != NULL)
	{
		if (StringLength1 == SQL_NTS)
			inStrLen = strlen((const char *)InConnectionString);
		else
			inStrLen = StringLength1;
		if (inStrLen > sizeof(lcConnectionString)-1)
		{
			setDiagRec(DRIVER_ERROR, IDS_HY_013);
			return SQL_ERROR;
		}
		strncpy(lcConnectionString, (const char *)InConnectionString, inStrLen);
		lcConnectionString[inStrLen] = '\0';

	}
	else
	{
		strcpy(lcConnectionString, "DSN=DEFAULT;");
		inStrLen = strlen(lcConnectionString);
	}
	ScanConnectString(lcConnectionString, inStrLen, KeywordTree);

	gDrvrGlobal.gClientApplication[0] = 0;
	if (KeywordTree[KEY_PCAPP].AttrRank != 0)
		strncpy(gDrvrGlobal.gClientApplication,KeywordTree[KEY_PCAPP].AttrValue,KeywordTree[KEY_PCAPP].AttrLength);
	if (KeywordTree[KEY_DSN].AttrRank != 0)
	{
		DSNAttrNo = KEY_DSN;
		DSNType = UNKNOWN_DSN;
		if ((KeywordTree[KEY_UID].AttrRank == 0)/* || (KeywordTree[KEY_PWD].AttrRank == 0)*/)
			reqAttrGiven = FALSE;
	}
	else
	if (KeywordTree[KEY_DRIVER].AttrRank != 0)
	{
		DSNAttrNo = KEY_DRIVER;		
		DSNType = DRIVER_KW_DSN;
		if ((KeywordTree[KEY_UID].AttrRank == 0)/* || (KeywordTree[KEY_PWD].AttrRank == 0)*/ ||
					(KeywordTree[KEY_SERVER].AttrRank == 0))
			reqAttrGiven = FALSE;
	}
	else
	{
		DSNType = UNKNOWN_DSN;
		strcpy(ServerNameNTS, "DEFAULT");
		DSNAttrNo = KEY_MAX;
	}
	if ((rc = prepareForConnect(KeywordTree, connectFieldItems, DSNAttrNo, DSNType, ServerNameNTS)) == SQL_ERROR)
		return SQL_ERROR;
	switch (DriverCompletion)
	{
	case SQL_DRIVER_NOPROMPT:
		if (! reqAttrGiven )
		{
			setDiagRec(DRIVER_ERROR, IDS_28_000);
			return SQL_ERROR;
		}
		break;
	// Don't want the clutter the dialog box with all the optional fields
	// Hence SQL_DRIVER_COMPLETE_REQUIRED is same as SQL_DRIVER_COMPLETE
	case SQL_DRIVER_COMPLETE_REQUIRED:	
	case SQL_DRIVER_COMPLETE:	
		if (reqAttrGiven)			
			break;
	case SQL_DRIVER_PROMPT:				// No break, should fall thru
/*
		if (WindowHandle == NULL)
		{
			setDiagRec(DRIVER_ERROR, IDS_IM_008);
			return SQL_ERROR;
		}
		if(IsWindow(WindowHandle) == 0)
		{
			setDiagRec(DRIVER_ERROR, IDS_IM_008);
			return SQL_ERROR;
		}
		// Synchronize, since DialogProc uses a static variable
		EnterCriticalSection(&m_CSObject);
		if (DSNType == DRIVER_KW_DSN)
			DialogRetCode = DialogBoxParam(gDrvrGlobal.gModuleHandle,
				MAKEINTRESOURCE(IDD_CONNECT_DRIVER_KW_DIALOG),
				WindowHandle, ConnectDriverKWDialogProc, (long)&connectFieldItems);
		else
			DialogRetCode = DialogBoxParam(gDrvrGlobal.gModuleHandle,
				MAKEINTRESOURCE(IDD_CONNECT_DIALOG),
				WindowHandle, ConnectDialogProc, (long)&connectFieldItems);
		LeaveCriticalSection(&m_CSObject);
		if (DialogRetCode == FALSE)
		{
			setDiagRec(DRIVER_ERROR, IDS_IM_008);
			return SQL_ERROR;
		}
		break;
*/
	default:
		setDiagRec(DRIVER_ERROR, IDS_HY_110);
		return SQL_ERROR;
	}
	// Update the m_DSValue with what is in the connectionString and dialogbox 
	m_DSValue.updateDSValues(DSNType, &connectFieldItems, (CONNECT_KEYWORD_TREE *)&KeywordTree);
		
	rc = Connect((SQLCHAR *)ServerNameNTS,
			SQL_NTS,
			(SQLCHAR *)connectFieldItems.loginId,
			SQL_NTS,
			(SQLCHAR *)connectFieldItems.password,
			SQL_NTS,
			FALSE);
	if (rc != SQL_ERROR)
	{
		rc1 = generateConnectionString((CONNECT_KEYWORD_TREE *)&KeywordTree, &connectFieldItems, DSNAttrNo, OutConnectionString, 
				BufferLength, StringLength2Ptr);
		if (rc1 == SQL_SUCCESS_WITH_INFO)
			rc = rc1;
	}
	return rc;
}
	
	
SQLRETURN CConnect::SetConnectAttr(SQLINTEGER Attribute, SQLUINTEGER ValueNum, SQLPOINTER ValueStr, bool bClearError)
{
	SQLRETURN			rc;

	if(bClearError)
		clearError();
	m_CurrentOdbcAPI = SQL_API_SQLSETCONNECTATTR;

	m_SrvrCallContext.initContext();
	m_SrvrCallContext.odbcAPI = SQL_API_SQLSETCONNECTATTR;
	m_SrvrCallContext.sqlHandle = this;
	m_SrvrCallContext.SQLSvc_ObjRef = m_SQLSvc_ObjRef;
	m_SrvrCallContext.ASSvc_ObjRef = m_ASSvc_ObjRef;
	m_SrvrCallContext.eventHandle = m_ConnectEvent;
	m_SrvrCallContext.dialogueId = m_DialogueId;
	m_SrvrCallContext.connectionTimeout = m_ConnectionTimeout;
	m_SrvrCallContext.u.setConnectParams.attribute = Attribute;
	m_SrvrCallContext.u.setConnectParams.valueNum = ValueNum;
	m_SrvrCallContext.u.setConnectParams.valueStr = ValueStr;
	rc = ThreadControlProc(&m_SrvrCallContext);
	if (rc == SQL_SUCCESS)
		if (m_SqlWarning)
			rc = SQL_SUCCESS_WITH_INFO;
	return rc;
}

SQLRETURN CConnect::SetConnectAttr(SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength)
{
	SQLRETURN	rc = SQL_SUCCESS;
	SQLINTEGER	strLen;
	char		Temp_Catalog[MAX_SQL_IDENTIFIER_LEN+1];
#ifdef VERSION3
	std::list<CHandlePtr>::iterator i;
#else
	RWTPtrSlistIterator<CHandle> i(m_StmtCollect);
#endif
	int j;
	char errorMsg[MAX_TRANSLATE_ERROR_MSG_LEN];

	Temp_Catalog[0] = '\0';
	CStmt* pStmt;
	UINT64 ULongValue;
	#ifdef ASYNCIO
	char buffer[256];    // Hold the error text
	int iterations = 0;  // number of times we will wait for the async IO thread to initialize
	#endif

	switch (Attribute)
	{
	case SQL_ATTR_ACCESS_MODE:
	case SQL_ATTR_ASYNC_ENABLE:
	case SQL_ATTR_AUTOCOMMIT:
	case SQL_ATTR_CONNECTION_TIMEOUT:
	case SQL_ATTR_LOGIN_TIMEOUT:
	case SQL_ATTR_METADATA_ID:
	case SQL_ATTR_ODBC_CURSORS:
	case SQL_ATTR_TRANSLATE_OPTION:
	case SQL_ATTR_TXN_ISOLATION:
	case SQL_MAX_ROWS:
	case SQL_MAX_LENGTH:
	case SQL_NOSCAN:
	case SQL_QUERY_TIMEOUT:
	case SQL_START_NODE:
	case SQL_MODE_LOADER:
	case SQL_STREAMS_PER_SEG:
	case SQL_LOADER_DELAYERROR:
	case SQL_ATTR_SESSIONNAME:
	case SQL_ATTR_ROLENAME:
	case SQL_ATTR_FETCHAHEAD:
	//wms_mapping 
	case SQL_ATTR_APPLNAME:
	case SQL_ATTR_CERTIFICATE_DIR:
	case SQL_ATTR_CERTIFICATE_FILE:
	case SQL_ATTR_CERTIFICATE_FILE_ACTIVE:
		break;
	case SQL_ATTR_QUIET_MODE:
		if (StringLength != 0 && StringLength != SQL_IS_POINTER)
		{
			setDiagRec(DRIVER_ERROR, IDS_HY_090);
			rc = SQL_ERROR;
		}
		break;
	case SQL_ATTR_CURRENT_CATALOG:
	case SQL_ATTR_TRANSLATE_LIB:
		if (StringLength == SQL_NTS)
			strLen = strlen((const char *)ValuePtr);
		else
			strLen = StringLength;
		break;
	case SQL_ATTR_AUTO_IPD:
	case SQL_ATTR_PACKET_SIZE:
		setDiagRec(DRIVER_ERROR, IDS_HY_092);
		rc = SQL_ERROR;
		break;
	case SQL_ATTR_TRACE:			// Implemented by DM
	case SQL_ATTR_TRACEFILE:
		break;
	case SQL_ATTR_ROWSET_RECOVERY:
	case SQL_ATTR_FETCH_BUFFER_SIZE:
		break;
	case SQL_ATTR_CONCURRENCY:
	case SQL_ATTR_CURSOR_TYPE:
	case SQL_ATTR_SIMULATE_CURSOR:
	case SQL_ATTR_USE_BOOKMARKS:
	default:
		setDiagRec(DRIVER_ERROR, IDS_HY_092);
		rc = SQL_ERROR;
	}
	
	if (rc == SQL_ERROR)
		return rc;

	switch (Attribute)
	{
	case SQL_ATTR_ACCESS_MODE:
		if (m_ConnectionDead != SQL_CD_TRUE)
#if defined(_WIN64) || defined(__LP64__)
            rc = SetConnectAttr(Attribute, (UINT64)ValuePtr, NULL);
#else
			rc = SetConnectAttr(Attribute, (SQLUINTEGER)ValuePtr, NULL);
#endif
		if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)	
#if defined(_WIN64) || defined(__LP64__)
            ULongValue = (UINT64)ValuePtr;
			m_AccessMode = (SQLUINTEGER)ULongValue;
#else			 
			m_AccessMode = (SQLUINTEGER)ValuePtr;
#endif
		break;
	case SQL_ATTR_ASYNC_ENABLE:
#if defined(_WIN64) || defined(__LP64__)
            ULongValue = (UINT64)ValuePtr;
			m_AsyncEnable = (SQLUINTEGER)ULongValue;
#else			 
		    m_AsyncEnable = (SQLUINTEGER)ValuePtr;
#endif
		#ifdef ASYNCIO
		if(m_AsyncEnable == SQL_ASYNC_ENABLE_ON && m_async_IOthread == NULL)
		{
			m_async_IOthread = new AsyncIO_Thread(this);
			if(m_async_IOthread == NULL)
			{
				m_AsyncEnable = SQL_ASYNC_ENABLE_OFF;
				rc = SQL_ERROR;
			}
			else
			{
				m_async_IOthread->Create(AsyncIOThread);

				if(m_async_IOthread->IsInitialized() == false)
				{
					while( (m_async_IOthread->IsInitialized() == false) 
						&& iterations++ < 3)
					{
						#ifdef __DEBUGASYNCIO
						printf("SQLSetConnectionAttr: Connection %x not initialized\n",this);
						#endif
						sleep(1);
						sched_yield(); // allow the created thread a chance to run

					} // while iterations < 3 && !m_async_IOthread->IsInitialized() 

					if(m_async_IOthread->IsInitialized() == false)
					{
						// Something is really wrong if we still have not been able to initialize by now
						#ifdef __DEBUGASYNCIO
						printf("SQLConnectionStmtAttr: Connection handle %x not initialized after 3 attempts\n",this);
						#endif

						sprintf(buffer,"SQLSetConnectionAttr SQL_ASYNC_ENABLE_ON failed for connection handle %x. Async thread was not initialized",
							this); 
						this->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, buffer);
						rc = SQL_ERROR;

						m_AsyncEnable = SQL_ASYNC_ENABLE_OFF; // turn ASYNC OFF

					} // if(m_async_IOthread->IsInitialized() == false)
				} // if(m_async_IOthread->IsInitialized() == false)
			} // if m_async_IOthread != NULL
		} // if(m_AsyncEnable == SQL_ASYNC_ENABLE_ON && m_async_IOthread == NULL)
		#endif // ASYNCIO

		for (i = m_StmtCollect.begin() ; i !=  m_StmtCollect.end() ; ++i)
			if((pStmt = (CStmt *) (*i)) != NULL)
			   pStmt->m_AsyncEnable = m_AsyncEnable;
		break;
	case SQL_ATTR_AUTOCOMMIT:
	        ULongValue = (UINT64)ValuePtr;
		if (m_ConnectionDead != SQL_CD_TRUE)
		{
#if defined(_WIN64) || defined(__LP64__) || defined(__64BIT__) || defined(_LP64)
		if( (m_AutoCommit == SQL_AUTOCOMMIT_ON && (SQLUINTEGER)ULongValue != SQL_AUTOCOMMIT_ON)
		    || (m_AutoCommit == SQL_AUTOCOMMIT_OFF && (SQLUINTEGER)ULongValue != SQL_AUTOCOMMIT_OFF) )
			rc = SetConnectAttr(Attribute, (SQLUINTEGER)ULongValue, NULL);
		}
		if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
			m_AutoCommit = (SQLUINTEGER)ULongValue;
#else
		if( (m_AutoCommit == SQL_AUTOCOMMIT_ON && (SQLUINTEGER)ValuePtr != SQL_AUTOCOMMIT_ON)
		    || (m_AutoCommit == SQL_AUTOCOMMIT_OFF && (SQLUINTEGER)ValuePtr != SQL_AUTOCOMMIT_OFF) )
			rc = SetConnectAttr(Attribute, (SQLUINTEGER)ValuePtr, NULL);
		}
		if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
			m_AutoCommit = (SQLUINTEGER)ValuePtr;
#endif
		break;
	case SQL_ATTR_CONNECTION_TIMEOUT:
#if defined(_WIN64) || defined(__LP64__)
        ULongValue = (UINT64)ValuePtr;
        m_ConnectionTimeout = (SQLUINTEGER)ULongValue;
#else
		m_ConnectionTimeout = (SQLUINTEGER)ValuePtr;
#endif
		break;
	case SQL_ATTR_CURRENT_CATALOG:
		if (strLen >= sizeof(m_CurrentCatalog))
		{
			setDiagRec(DRIVER_ERROR, IDS_HY_090);
			rc = SQL_ERROR;
		}
		else
		{
			if(strLen > 0)
			{
				int transLen =0;
				if(SQL_SUCCESS != m_ICUConv->InArgTranslationHelper((SQLCHAR *)ValuePtr, strLen/2, Temp_Catalog, 
									sizeof(Temp_Catalog), &transLen, errorMsg))
				{
					setDiagRec(DRIVER_ERROR, IDS_01_004, 0, (char *)errorMsg);
					return SQL_ERROR; 
				}
			}
			else
				Temp_Catalog[0] = '\0';

//			strncpy(Temp_Catalog, (const char *)ValuePtr, strLen);
			Temp_Catalog[strLen] = '\0';
		
			if (m_ConnectionDead != SQL_CD_TRUE)
				rc = SetConnectAttr(Attribute, 0, Temp_Catalog);
			if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)	

				strcpy(m_CurrentCatalog, Temp_Catalog);
		}
		break;
	case SQL_ATTR_LOGIN_TIMEOUT:
#if defined(_WIN64) || defined(__LP64__)
        ULongValue = (UINT64)ValuePtr;
		m_LoginTimeout = (SQLUINTEGER)ULongValue;
#else
  	    m_LoginTimeout = (SQLUINTEGER)ValuePtr;
#endif
		break;
	case SQL_ATTR_METADATA_ID:
#if defined(_WIN64) || defined(__LP64__)
        ULongValue = (UINT64)ValuePtr;
		m_MetadataId = (SQLUINTEGER)ULongValue;
#else
		m_MetadataId = (SQLUINTEGER)ValuePtr;
#endif
		for (i = m_StmtCollect.begin() ; i !=  m_StmtCollect.end() ; ++i)
			if((pStmt = (CStmt *) (*i)) != NULL)
			pStmt->m_MetadataId = m_MetadataId;
		break;
	case SQL_ATTR_ODBC_CURSORS:
		if (m_ConnectionDead == SQL_CD_FALSE)
#if defined(_WIN64) || defined(__LP64__)
        {
           ULongValue = (UINT64)ValuePtr;
		   m_ODBCCursors = (SQLUINTEGER)ULongValue;
 	    }
#else
		   m_ODBCCursors = (SQLUINTEGER)ValuePtr;
#endif
		else
		{
			setDiagRec(DRIVER_ERROR, IDS_08_002);
			rc = SQL_ERROR;
		}
		break;
	case SQL_ATTR_QUIET_MODE:
		m_QuietMode = (HWND)ValuePtr;
		break;
	case SQL_ATTR_TRANSLATE_LIB:
		if (strLen >= sizeof(m_TranslateLib))
		{
			setDiagRec(DRIVER_ERROR, IDS_HY_090);
			rc = SQL_ERROR;
		}
		else
		{
			strncpy(m_TranslateLib, (const char *)ValuePtr, strLen);
			m_TranslateLib[strLen] = '\0';
		}
		break;
	case SQL_ATTR_TRANSLATE_OPTION:
#if defined(_WIN64) || defined(__LP64__)
        ULongValue = (UINT64)ValuePtr;
		m_TranslateOption = (SQLUINTEGER)ULongValue;
#else
		m_TranslateOption = (SQLUINTEGER)ValuePtr;
#endif
		break;
	case SQL_ATTR_TXN_ISOLATION:
#if defined(_WIN64) || defined(__LP64__)
        ULongValue = (UINT64)ValuePtr;
		if (m_ConnectionDead != SQL_CD_TRUE)
			rc = SetConnectAttr(Attribute, (SQLUINTEGER)ULongValue, NULL);

		if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)	
  		   m_TxnIsolation = (SQLUINTEGER)ULongValue;

#else
		if (m_ConnectionDead != SQL_CD_TRUE)
			rc = SetConnectAttr(Attribute, (SQLUINTEGER)ValuePtr, NULL);

		if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)	
		   m_TxnIsolation = (SQLUINTEGER)ValuePtr;
#endif
		break;
	case SQL_ATTR_TRACE:			// Implemented by DM
	case SQL_ATTR_TRACEFILE:
		break;
	case SQL_ATTR_CONCURRENCY:
		break;
	case SQL_ATTR_CURSOR_TYPE:
		break;
	case SQL_ATTR_SIMULATE_CURSOR:
		break;
	case SQL_ATTR_USE_BOOKMARKS:
#if defined(_WIN64) || defined(__LP64__)
        ULongValue = (UINT64)ValuePtr;
		if ((SQLUINTEGER)ULongValue == SQL_UB_ON)
#else
		if ((SQLUINTEGER)ValuePtr == SQL_UB_ON)
#endif
		{
			setDiagRec(DRIVER_ERROR, IDS_01_S02);
			rc = SQL_SUCCESS_WITH_INFO;
		}
		break;
	case SQL_MAX_ROWS:
		m_MaxRows = (SQLULEN)ValuePtr;
		break;
	case SQL_MAX_LENGTH:
		m_MaxLength = (SQLULEN)ValuePtr;
		break;
	case SQL_NOSCAN:
#if defined(_WIN64) || defined(__LP64__)
        ULongValue = (UINT64)ValuePtr;
		m_Noscan = (SQLUINTEGER)ULongValue;
#else
		m_Noscan = (SQLUINTEGER)ValuePtr;
#endif
		break;
	case SQL_QUERY_TIMEOUT:
#if defined(_WIN64) || defined(__LP64__)
        ULongValue = (UINT64)ValuePtr;
		m_QueryTimeout = (SQLUINTEGER)ULongValue;
#else
		m_QueryTimeout = (SQLUINTEGER)ValuePtr;
#endif
		if(m_QueryTimeout != 0 && m_QueryTimeout < 30)
				m_QueryTimeout = 30;
		break;
	case SQL_ATTR_ROWSET_RECOVERY:
#if defined(_WIN64) || defined(__LP64__)
        ULongValue = (UINT64)ValuePtr;
		if (m_ConnectionDead != SQL_CD_TRUE)
			rc = SetConnectAttr(Attribute, (SQLUINTEGER)ULongValue, NULL);
		else
			rc = SQL_ERROR;
		if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)	
			m_RowsetErrorRecovery = (SQLUINTEGER)ULongValue;
#else
		if (m_ConnectionDead != SQL_CD_TRUE)
			rc = SetConnectAttr(Attribute, (SQLUINTEGER)ValuePtr, NULL);
		else
			rc = SQL_ERROR;
		if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)	
			m_RowsetErrorRecovery = (SQLUINTEGER)ValuePtr;
#endif
		break;
	case SQL_ATTR_FETCH_BUFFER_SIZE:
#if defined(_WIN64) || defined(__LP64__)
        ULongValue = (UINT64)ValuePtr;
		m_FetchBufferSize = (SQLUINTEGER)ULongValue;
#else
		m_FetchBufferSize = (SQLINTEGER)ValuePtr;
#endif
		if (m_FetchBufferSize < 0)
			m_FetchBufferSize = 0;
		m_FetchBufferSize *= 1024;
		break;
	case SQL_MODE_LOADER:
#if defined (MXLINUX) || defined (MXSUNSPARC) || defined (MXAIX)
		m_ModeLoader = (SQLUINTEGER)(SQLULEN)ValuePtr;
#else
        setDiagRec(DRIVER_ERROR, IDS_HY_092);
        rc = SQL_ERROR;
#endif		
		break;
	case SQL_LOADER_DELAYERROR:
#if defined (MXLINUX) || defined (MXSUNSPARC) || defined (MXAIX)
	    m_StreamDelayedError = (SQLUINTEGER)(SQLULEN)ValuePtr;
#else
        setDiagRec(DRIVER_ERROR, IDS_HY_092);
        rc = SQL_ERROR;
#endif		
		break;
	case SQL_START_NODE:
#if defined(_WIN64) || defined(__LP64__)
        ULongValue = (UINT64)ValuePtr;
		m_StartNode = (SQLUINTEGER)ULongValue;
#else
		m_StartNode = (SQLUINTEGER)ValuePtr;
#endif
		break;
	case SQL_STREAMS_PER_SEG:
#if defined(_WIN64) || defined(__LP64__)
        ULongValue = (UINT64)ValuePtr;
		m_StreamsPerSeg = (SQLUINTEGER)ULongValue;
#else
		m_StreamsPerSeg = (SQLUINTEGER)ValuePtr;
#endif
		break;
	// HP Extension to set the Session Name part of Unique Query ID
	case SQL_ATTR_SESSIONNAME:
	// For Unicode drvr , we need a function in translation class to take care of lenth in utf-8/utf16 case
	// [ALM CR5228] use the ICU Converter function FindStringLength() to determine the length of the value
	// which solves utf-8/utf16 case.
		strLen = gDrvrGlobal.ICUConv.FindStrLength((const char*)ValuePtr, (SQLINTEGER)StringLength);
		if(m_ICUConv->isAppUTF16())
		{
			strLen = strLen/2;
			for(j = 0; j < strLen; j++)
			{
				UChar32 UCharHyphen = 0x5F;	// Change the value from 0x2D to 0x5F. 0x2D is dash '-', not a under score.
				UChar32 ch = (*(UChar *)((UChar *)ValuePtr + j));
				/* Why it was using u_isUAlphabetic() here? As investigation on Windows driver, 
				* any alphanumeric characters(a-z, A-Z, 0-9) are considered as legal here.
				* UCHAR_ALPHABETIC property reflect Alphabetic property defined in UCD. Thus numeric character
				* will be considered as illegal as well, not just only non-syllable characters. (Refer to the
				* UNICODE standard doc, chapter 4 character properties).
				*
				* For ALM CR5228. Use u_isalnum() function instead to determine the given character is legal or not.
				** */
//				if(!u_isUAlphabetic(ch) && ch != UCharHyphen)
				if(!u_isalnum(ch) && ch != UCharHyphen)
				{
					setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, "Illegal characters in the Session Name attribute.");
					rc = SQL_ERROR;
					break;
				}
			}

		}
		else
		{	
			for(j = 0; j < strLen; j++)
			{
				//10-080811-5133
				int ch = (*(unsigned char *)((char *)ValuePtr + j));
				//if(!isalnum(*(char *)((char *)ValuePtr + j) ) && (*(char *)((char *)ValuePtr + j) != '_'))
				if(!isalnum(ch) && ch != '_')
				{
					setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, "Illegal characters in the Session Name attribute.");
					rc = SQL_ERROR;
					break;
				}
			}
		}
		if(rc == SQL_SUCCESS)
		{
				if(strLen > 0)
				{
					int transLen=0;
					rc = m_ICUConv->InArgTranslationHelper((SQLCHAR *)ValuePtr, strLen, m_QueryID_SessionName,
						       					sizeof(m_QueryID_SessionName), &transLen, errorMsg, false, CASE_NONE, true);
					if (rc == SQL_ERROR)
					{
						setDiagRec(DRIVER_ERROR, IDS_01_004, 0, (char *)errorMsg);
					}
					break; 
				}
				else
					m_QueryID_SessionName[0] = '\0';
				if(rc == SQL_SUCCESS_WITH_INFO)
				{
					setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, "The maximum length for Session Name, 24 characters, has been exceeded.");
					rc = SQL_ERROR;
					break;
				}
		}
		break;
	case SQL_ATTR_FETCHAHEAD:
#if defined(_WIN64) || defined(__LP64__)
        ULongValue = (UINT64)ValuePtr;
		if ((SQLUINTEGER)ULongValue != 0 && (SQLUINTEGER)ULongValue != 1)
		{
			setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, "Illegal value specified. Valid value is 0 or 1.");
			rc = SQL_ERROR;
			break;
		}
		m_FetchAhead = (SQLUINTEGER)ULongValue;
#else
		if ((SQLINTEGER)ValuePtr != 0 && (SQLINTEGER)ValuePtr != 1)
		{
			setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, "Illegal value specified. Valid value is 0 or 1.");
			rc = SQL_ERROR;
			break;
		}
		m_FetchAhead = (SQLINTEGER)ValuePtr;
#endif
		break;
	case SQL_ATTR_ROLENAME:
	{
		strLen = m_ICUConv->FindStrLength((const char *)ValuePtr, StringLength);

		if (strLen >= sizeof(m_UserRole))
		{
			setDiagRec(DRIVER_ERROR, IDS_HY_090);
			rc = SQL_ERROR;
		}
		if(strLen > 0)
		{		
				int transLen = 0;
				rc = m_ICUConv->InArgTranslationHelper((SQLCHAR *)ValuePtr, strLen, m_UserRole,
					       					sizeof(m_UserRole), &transLen, errorMsg);
				if (rc == SQL_ERROR)
				{
					setDiagRec(DRIVER_ERROR, IDS_01_004, 0, (char *)errorMsg);
				}
				break; 
		}
		else
			m_UserRole[0] = '\0';
		if(rc == SQL_SUCCESS_WITH_INFO)
		{
			setDiagRec(DRIVER_ERROR, IDS_HY_090);
			rc = SQL_ERROR;
			break;
		}
	}
		break;
/* Don't need to stranslate the string values below as they are going to be ASCII values, same in UTF8 */
	//wms_mapping 
	case SQL_ATTR_APPLNAME:
		if (StringLength == SQL_NTS)
			strLen = strlen((const char *)ValuePtr);
		else
			strLen = StringLength;

		strLen = strLen > sizeof(m_applName)-1 ? sizeof(m_applName)-1 : strLen;
		strncpy(m_applName, (char*)ValuePtr, strLen);
		m_applName[strLen] = 0;
		break;
	case SQL_ATTR_CERTIFICATE_DIR:
		if (StringLength == SQL_NTS)
			strLen = strlen((const char *)ValuePtr);
		else
			strLen = StringLength;

		strLen = strLen > sizeof(m_CertificateDir)-1 ? sizeof(m_CertificateDir)-1 : strLen;
		strncpy(m_CertificateDir, (char*)ValuePtr, strLen);
		m_CertificateDir[strLen] = 0;
		break;
	case SQL_ATTR_CERTIFICATE_FILE:
		if (StringLength == SQL_NTS)
			strLen = strlen((const char *)ValuePtr);
		else
			strLen = StringLength;

		strLen = strLen > sizeof(m_CertificateFile)-1 ? sizeof(m_CertificateFile)-1 : strLen;
		strncpy(m_CertificateFile, (char*)ValuePtr, strLen);
		m_CertificateFile[strLen] = 0;
		break;
	case SQL_ATTR_CERTIFICATE_FILE_ACTIVE:
		if (StringLength == SQL_NTS)
			strLen = strlen((const char *)ValuePtr);
		else
			strLen = StringLength;

		strLen = strLen > sizeof(m_CertificateFileActive)-1 ? sizeof(m_CertificateFileActive)-1 : strLen;
		strncpy(m_CertificateFileActive, (char*)ValuePtr, strLen);
		m_CertificateFileActive[strLen] = 0;
		break;
	default:
		setDiagRec(DRIVER_ERROR, IDS_HY_092);
		rc = SQL_ERROR;
		break;
	}
	return rc;
}

SQLRETURN CConnect::GetConnectAttr(SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER BufferLength,
								   SQLINTEGER *StringLengthPtr)
{
	SQLRETURN				rc = SQL_SUCCESS;
	RETURN_VALUE_STRUCT		retValue;
	
	clearError();
	m_CurrentOdbcAPI = SQL_API_SQLGETCONNECTATTR;
	retValue.dataType = DRVR_PENDING;
	retValue.u.strPtr = NULL;
	
	switch (Attribute)
	{
	case SQL_ATTR_ACCESS_MODE:
		retValue.u.u32Value = m_AccessMode;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_ASYNC_ENABLE:
		retValue.u.u32Value = m_AsyncEnable;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_AUTOCOMMIT:
		retValue.u.u32Value = m_AutoCommit;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_AUTO_IPD:
		retValue.u.u32Value = m_AutoIPD;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_CONNECTION_TIMEOUT:
		retValue.u.u32Value = m_ConnectionTimeout;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_CURRENT_CATALOG:
		retValue.u.strPtr = m_CurrentCatalog;
		break;
	case SQL_ATTR_LOGIN_TIMEOUT:
		retValue.u.u32Value = m_LoginTimeout;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_METADATA_ID:
		retValue.u.u32Value = m_MetadataId;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_ODBC_CURSORS:
		retValue.u.u32Value = m_ODBCCursors;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_PACKET_SIZE:
		retValue.u.u32Value = m_PacketSize;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_QUIET_MODE:
		retValue.u.pValue = (SQLPOINTER)m_QuietMode;
		retValue.dataType = SQL_IS_POINTER;
		break;
	case SQL_ATTR_TRANSLATE_LIB:
		retValue.u.strPtr = m_TranslateLib;
		break;
	case SQL_ATTR_TRANSLATE_OPTION:
		retValue.u.u32Value = m_TranslateOption;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_TXN_ISOLATION:
		retValue.u.u32Value = m_TxnIsolation;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_TRACE:			// Implemented by DM
	case SQL_ATTR_TRACEFILE:
		break;
	case SQL_ATTR_CONCURRENCY:
		retValue.u.u32Value = m_Concurrency;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_CURSOR_TYPE:
		retValue.u.u32Value = m_CursorType;
		retValue.dataType = SQL_IS_UINTEGER;
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
	case SQL_ATTR_NOSCAN:
		retValue.u.u32Value = m_Noscan;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_QUERY_TIMEOUT:
		retValue.u.u32Value = m_QueryTimeout;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_SIMULATE_CURSOR:
		retValue.u.u32Value = m_SimulateCursor;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_USE_BOOKMARKS:
		retValue.u.u32Value = m_UseBookmarks;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_ROWSET_RECOVERY:
		retValue.u.u32Value = m_RowsetErrorRecovery;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ATTR_FETCH_BUFFER_SIZE:
		retValue.u.u32Value = m_FetchBufferSize/1024;
		retValue.dataType = SQL_IS_INTEGER;
		break;
	case SQL_ATTR_SESSIONNAME:
		retValue.u.strPtr = m_QueryID_SessionName;
		break;
	case SQL_ATTR_FETCHAHEAD:
		retValue.u.u32Value = m_FetchAhead;
		break;
	case SQL_ATTR_ROLENAME:
		retValue.u.strPtr = m_UserRole;
		break;
	//wms_mapping 
	case SQL_ATTR_APPLNAME:
		retValue.u.strPtr = m_applName;
		break;
	case SQL_ATTR_SUG_ROWSET_SIZE:
		retValue.u.u32Value = m_SugRowsetSize;
		retValue.dataType = SQL_IS_INTEGER;
		break;
	case SQL_ATTR_CERTIFICATE_DIR:
		retValue.u.strPtr = m_CertificateDir;
		break;
	case SQL_ATTR_CERTIFICATE_FILE:
		retValue.u.strPtr = m_CertificateFile;
		break;
	case SQL_ATTR_CERTIFICATE_FILE_ACTIVE:
		retValue.u.strPtr = m_CertificateFileActive;
		break;
	default:
		setDiagRec(DRIVER_ERROR, IDS_HY_092);
		break;
	}
	if (rc == SQL_SUCCESS)
		rc = returnAttrValue(TRUE, this, &retValue, ValuePtr, BufferLength, StringLengthPtr);
	return rc;
}

SQLRETURN CConnect::EndTran(SQLSMALLINT CompletionType)	
{
	SQLRETURN			rc;

#ifdef VERSION3
	std::list<CHandlePtr>::iterator i;
#else
	RWTPtrSlistIterator<CHandle> i(m_StmtCollect);
#endif

	CStmt* pStmt;

	clearError();
	if (m_ConnectionDead == SQL_CD_TRUE)
	{
		clearError();
		setDiagRec(DRIVER_ERROR, IDS_08_S01, -20005);
		rc = SQL_ERROR;
	}
	else
	{
		m_SrvrCallContext.initContext();
		m_SrvrCallContext.odbcAPI = SQL_API_SQLENDTRAN;
		m_SrvrCallContext.sqlHandle = this;
		m_SrvrCallContext.SQLSvc_ObjRef = m_SQLSvc_ObjRef;
		m_SrvrCallContext.ASSvc_ObjRef = m_ASSvc_ObjRef;
		m_SrvrCallContext.eventHandle = m_ConnectEvent;
		m_SrvrCallContext.dialogueId = m_DialogueId;
		m_SrvrCallContext.connectionTimeout = m_ConnectionTimeout;
		m_SrvrCallContext.u.completionType = CompletionType;
		rc = ThreadControlProc(&m_SrvrCallContext);
		if (rc == SQL_SUCCESS)
			if (m_SqlWarning)
				rc = SQL_SUCCESS_WITH_INFO;
	}
	switch (rc)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
		// SQL_CB_CLOSE behavior
#ifdef VERSION3
		for (i = m_StmtCollect.begin() ; i !=  m_StmtCollect.end() ; ++i)
			if((pStmt = (CStmt *) (*i)) != NULL)
#else
		while ((pStmt = (CStmt *)i()) != NULL)
#endif
            if(!pStmt->m_CursorHoldable)
			pStmt->revertStmtState();

		break;
	case SQL_STILL_EXECUTING:
	case SQL_NEED_DATA:
	case SQL_ERROR:
	case SQL_NO_DATA:
	case SQL_INVALID_HANDLE:
	default:
		break;
	}
	return rc;
}

SQLRETURN CConnect::GetInfo(SQLUSMALLINT InfoType, 
		   SQLPOINTER InfoValuePtr,
           SQLSMALLINT BufferLength,
		   SQLSMALLINT *StringLengthPtr)
{
	char					*cYes	= "Y";
	char					*cNo	= "N";
	char					*cPartial = "P"; //  Based on 2.0 specification
	char					*cFull = "F";
	char					*cEmpty = "";
	char					tmpStr[32];
	char					*cKeywords = "ABS,AFTER,ALIAS,BEFORE,BOOLEAN,BREADTH,CALL,COMPLETION,"\
								   "CONVERTTIMESTAMP,CYCLE,DATEFORMAT,DAYOFWEEK,DEPTH,DICTIONARY,"\
								   "EACH,EQUALS,GENERAL,IF,IGNORE,INDEX_TABLE,INVOKE,JULIANTIMESTAMP,"\
								   "LEAVE,LESS,LIMIT,LOAD_TABLE,LOOP,MODIFY,NEW,OBJECT,OFF,OID,OLD,"\
								   "OPERATION,OPERATORS,OTHERS,PARAMETERS,PENDANT,PREORDER,PRIVATE,"\
								   "PROTECTED,PROTOTYPE,RECURSIVE,REF,REFERENCING,REPLACE,RESIGNAL,"\
								   "RESOURCE_FORK,RETURN,RETURNS,ROLE,ROUTINE,ROW,SAVEPOINT,SEARCH,"\
								   "SENSITIVE,SHOWDDL,SHOWPLAN,SIGNAL,SIMILAR,SQLEXCEPTION,STDDEV,"\
								   "STRUCTURE,TEST,THERE,TRANSPOSE,TRIGGER,TYPE,UNDER,UPSHIFT,"\
								   "VARIABLE,VARIANCE,VIRTUAL,WAIT,WHILE,WITHOUT";
	SQLRETURN				rc = SQL_SUCCESS;
	RETURN_VALUE_STRUCT		retValue;
	SQLINTEGER				lStringLength;

	clearError();
	m_CurrentOdbcAPI = SQL_API_SQLGETINFO;
	retValue.dataType = DRVR_PENDING;
	retValue.u.strPtr = NULL;

	switch (InfoType)
	{
	case SQL_ACCESSIBLE_PROCEDURES:	
		retValue.u.strPtr = cNo;
		break;
	case SQL_ACCESSIBLE_TABLES:	
		retValue.u.strPtr = cNo;
		break;
	case SQL_ACTIVE_ENVIRONMENTS:
		retValue.u.u16Value = 0;	
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_AGGREGATE_FUNCTIONS:
		retValue.u.u32Value = SQL_AF_AVG | SQL_AF_COUNT | SQL_AF_MAX | SQL_AF_MIN | SQL_AF_SUM;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ALTER_DOMAIN:
		retValue.u.u32Value = 0L; // Not supported
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ALTER_TABLE:
		retValue.u.u32Value = 0;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ASYNC_MODE:
		retValue.u.u32Value = SQL_AM_STATEMENT;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_BATCH_ROW_COUNT:
		retValue.u.u32Value = 0; 
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_BATCH_SUPPORT:
		retValue.u.u32Value = 0; 
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_BOOKMARK_PERSISTENCE:
		retValue.u.u32Value = 0;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_CATALOG_LOCATION:
		retValue.u.u16Value = SQL_CL_START;
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_CATALOG_NAME:
		retValue.u.strPtr = cYes;
		break;
	case SQL_CATALOG_NAME_SEPARATOR:
		retValue.u.strPtr = ".";
		break;
	case SQL_CATALOG_TERM:
		retValue.u.strPtr = "catalog";
		break;
	case SQL_CATALOG_USAGE:
		retValue.u.u32Value = SQL_QU_DML_STATEMENTS | SQL_QU_TABLE_DEFINITION | 
				SQL_QU_INDEX_DEFINITION;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_COLLATION_SEQ:
		retValue.u.strPtr = "ISO 8859-1";
		break;
	case SQL_COLUMN_ALIAS:
		retValue.u.strPtr = cYes;
		break;
	case SQL_CONCAT_NULL_BEHAVIOR:
		retValue.u.u16Value = SQL_CB_NULL;
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_CONVERT_BIGINT:		
	case SQL_CONVERT_DECIMAL:               
	case SQL_CONVERT_DOUBLE:                
	case SQL_CONVERT_FLOAT:                 
	case SQL_CONVERT_INTEGER:               
	case SQL_CONVERT_NUMERIC:               
	case SQL_CONVERT_REAL:                  
	case SQL_CONVERT_SMALLINT:              
		retValue.u.u32Value = SQL_CVT_CHAR | SQL_CVT_NUMERIC | SQL_CVT_DECIMAL
					| SQL_CVT_INTEGER | SQL_CVT_SMALLINT | SQL_CVT_FLOAT
					| SQL_CVT_REAL | SQL_CVT_DOUBLE  | SQL_CVT_VARCHAR
					// | SQL_CVT_LONGVARCHAR
					// | SQL_CVT_TINYINT 
					| SQL_CVT_BIGINT;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_CONVERT_CHAR:                  
	case SQL_CONVERT_VARCHAR: 
		retValue.u.u32Value = SQL_CVT_CHAR | SQL_CVT_NUMERIC | SQL_CVT_DECIMAL
					| SQL_CVT_INTEGER | SQL_CVT_SMALLINT | SQL_CVT_FLOAT
					| SQL_CVT_REAL | SQL_CVT_DOUBLE  | SQL_CVT_VARCHAR
					// | SQL_CVT_LONGVARCHAR 
					// | SQL_CVT_TINYINT 
					| SQL_CVT_BIGINT | SQL_CVT_DATE | SQL_CVT_TIME 
					| SQL_CVT_TIMESTAMP;
					// | SQL_CVT_WCHAR |SQL_CVT_WLONGVARCHAR | SQL_CVT_WVARCHAR;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_CONVERT_LONGVARCHAR:           
	case SQL_CONVERT_WLONGVARCHAR:
		retValue.u.u16Value = 0;	// Comply with Windows driver
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_CONVERT_DATE:                  
		retValue.u.u32Value =  SQL_CVT_CHAR | SQL_CVT_VARCHAR
					| SQL_CVT_DATE 
					| SQL_CVT_TIMESTAMP;
		
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_CONVERT_TIME:                  
		retValue.u.u32Value = SQL_CVT_CHAR | SQL_CVT_VARCHAR
					| SQL_CVT_TIME 
					| SQL_CVT_TIMESTAMP;
		
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_CONVERT_TIMESTAMP:             
		retValue.u.u32Value = SQL_CVT_CHAR | SQL_CVT_VARCHAR
					| SQL_CVT_DATE | SQL_CVT_TIME 
					| SQL_CVT_TIMESTAMP;
		
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_CONVERT_BIT:                   
	case SQL_CONVERT_BINARY:		
	case SQL_CONVERT_VARBINARY:             
	case SQL_CONVERT_LONGVARBINARY:         		
	case SQL_CONVERT_INTERVAL_DAY_TIME:
	case SQL_CONVERT_INTERVAL_YEAR_MONTH:
	case SQL_CONVERT_TINYINT: 
		retValue.u.u32Value = 0;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_CONVERT_WCHAR:
	case SQL_CONVERT_WVARCHAR:
		retValue.u.u32Value = SQL_CVT_CHAR | SQL_CVT_NUMERIC | SQL_CVT_DECIMAL
					| SQL_CVT_INTEGER | SQL_CVT_SMALLINT | SQL_CVT_FLOAT
					| SQL_CVT_REAL | SQL_CVT_DOUBLE  | SQL_CVT_VARCHAR
					
					// | SQL_CVT_TINYINT 
					| SQL_CVT_BIGINT | SQL_CVT_DATE | SQL_CVT_TIME 
					| SQL_CVT_TIMESTAMP
					| SQL_CVT_WCHAR | SQL_CVT_WVARCHAR;
		retValue.dataType = SQL_IS_UINTEGER;
		break;

	case SQL_CONVERT_FUNCTIONS:
		retValue.u.u32Value = SQL_FN_CVT_CONVERT;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_CORRELATION_NAME:
		retValue.u.u16Value = SQL_CN_ANY;
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_CREATE_ASSERTION:
		retValue.u.u32Value = 0;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_CREATE_CHARACTER_SET:
		retValue.u.u32Value = 0;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_CREATE_COLLATION:
		retValue.u.u32Value = 0;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_CREATE_DOMAIN:
		retValue.u.u32Value = 0;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_CREATE_SCHEMA:
		retValue.u.u32Value = 0;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_CREATE_TABLE:
		retValue.u.u32Value = SQL_CT_CREATE_TABLE;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_CREATE_TRANSLATION:
		retValue.u.u32Value = 0;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_CREATE_VIEW:
		retValue.u.u32Value = SQL_CV_CREATE_VIEW | SQL_CV_CHECK_OPTION | SQL_CV_CASCADED;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_CURSOR_COMMIT_BEHAVIOR:
	case SQL_CURSOR_ROLLBACK_BEHAVIOR:
		retValue.u.u16Value = SQL_CB_CLOSE;
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_CURSOR_SENSITIVITY:
		retValue.u.u32Value = SQL_UNSPECIFIED;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_DATA_SOURCE_NAME:
		retValue.u.strPtr = m_DSValue.m_DSName;
		break;
	case SQL_DATA_SOURCE_READ_ONLY:
		if (m_SrvrDSReadType == 0) // Read only
			retValue.u.strPtr = cYes;
		else
			retValue.u.strPtr = cNo;
		break;
	case SQL_DATABASE_NAME:
		retValue.u.strPtr = m_CurrentCatalog;
		break;
	case SQL_DATETIME_LITERALS:
		retValue.u.u32Value = SQL_DL_SQL92_DATE | SQL_DL_SQL92_TIME | SQL_DL_SQL92_TIMESTAMP
				| SQL_DL_SQL92_INTERVAL_YEAR | SQL_DL_SQL92_INTERVAL_MONTH  
				| SQL_DL_SQL92_INTERVAL_DAY | SQL_DL_SQL92_INTERVAL_HOUR
				| SQL_DL_SQL92_INTERVAL_MINUTE | SQL_DL_SQL92_INTERVAL_SECOND
				| SQL_DL_SQL92_INTERVAL_YEAR_TO_MONTH | SQL_DL_SQL92_INTERVAL_DAY_TO_HOUR
				| SQL_DL_SQL92_INTERVAL_DAY_TO_MINUTE
				| SQL_DL_SQL92_INTERVAL_DAY_TO_SECOND | SQL_DL_SQL92_INTERVAL_HOUR_TO_MINUTE
				| SQL_DL_SQL92_INTERVAL_HOUR_TO_SECOND | SQL_DL_SQL92_INTERVAL_MINUTE_TO_SECOND;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_DBMS_NAME:
		retValue.u.strPtr = "Trafodion";
		break;
	case SQL_DBMS_VER:
		sprintf(tmpStr, "%02d.%02d.%04d", m_SqlVersion.majorVersion,
			m_SqlVersion.minorVersion, m_SqlVersion.buildId);
		retValue.u.strPtr = tmpStr;
		break;
	case SQL_DDL_INDEX:
		retValue.u.u32Value = SQL_DI_CREATE_INDEX | SQL_DI_DROP_INDEX;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_DEFAULT_TXN_ISOLATION:
		retValue.u.u32Value = SQL_TXN_READ_COMMITTED;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_DESCRIBE_PARAMETER:
		retValue.u.strPtr = cYes;
		break;
	case SQL_DM_VER: //		Implemented by DM
		break;
	case SQL_DRIVER_HDBC: //		Implemented by DM
	case SQL_DRIVER_HENV:
	case SQL_DRIVER_HDESC:
	case SQL_DRIVER_HLIB:
	case SQL_DRIVER_HSTMT:
		break;
	case SQL_DRIVER_NAME:
//		retValue.u.strPtr = gDrvrGlobal.gDriverDLLName;
		retValue.u.strPtr = DRIVER_DLL_NAME;
		break;
	case SQL_DRIVER_ODBC_VER:
		sprintf(tmpStr, "%02d.%02d",3,51);
		retValue.u.strPtr = tmpStr;
		break;
	case SQL_DRIVER_VER:
		sprintf(tmpStr, "%02d.%02d.%04d", gDrvrGlobal.gClientVersion.majorVersion,
			gDrvrGlobal.gClientVersion.minorVersion, gDrvrGlobal.gClientVersion.buildId);
		retValue.u.strPtr = tmpStr;
		break;
	case SQL_DROP_ASSERTION:
	case SQL_DROP_CHARACTER_SET:		
	case SQL_DROP_COLLATION:			
	case SQL_DROP_DOMAIN:			
	case SQL_DROP_SCHEMA:
	case SQL_DROP_TRANSLATION:
		retValue.u.u32Value = 0;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_DROP_TABLE:
		retValue.u.u32Value = SQL_DT_DROP_TABLE | SQL_DT_RESTRICT | SQL_DT_CASCADE;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_DROP_VIEW:
		retValue.u.u32Value = SQL_DV_DROP_VIEW | SQL_DV_RESTRICT | SQL_DV_CASCADE;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_DYNAMIC_CURSOR_ATTRIBUTES1:	
	case SQL_DYNAMIC_CURSOR_ATTRIBUTES2:	
		retValue.u.u32Value = 0;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_EXPRESSIONS_IN_ORDERBY:
		retValue.u.strPtr = cNo;						 // But we support column Number
		break;
	case SQL_FETCH_DIRECTION:
		retValue.u.u32Value = SQL_FD_FETCH_NEXT;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_FILE_USAGE:
		retValue.u.u16Value = SQL_FILE_NOT_SUPPORTED;
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES1:	
		retValue.u.u32Value = SQL_CA1_NEXT | SQL_CA1_SELECT_FOR_UPDATE;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2:	
		retValue.u.u32Value = SQL_CA2_READ_ONLY_CONCURRENCY | SQL_CA2_LOCK_CONCURRENCY 
					| SQL_CA2_MAX_ROWS_SELECT;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_GETDATA_EXTENSIONS:
		retValue.u.u32Value = SQL_GD_ANY_COLUMN | SQL_GD_ANY_ORDER | SQL_GD_BOUND;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_GROUP_BY:
		retValue.u.u16Value = SQL_GB_GROUP_BY_EQUALS_SELECT;
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_IDENTIFIER_CASE:
		retValue.u.u16Value = SQL_IC_UPPER;
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_IDENTIFIER_QUOTE_CHAR:				
		retValue.u.strPtr = "\"";
		break;
	case SQL_INDEX_KEYWORDS:
		retValue.u.u32Value = SQL_IK_ALL;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_INFO_SCHEMA_VIEWS:
		retValue.u.u32Value = 0;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_INSERT_STATEMENT:
		retValue.u.u32Value = SQL_IS_INSERT_LITERALS | SQL_IS_INSERT_SEARCHED 
									| SQL_IS_SELECT_INTO;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_INTEGRITY:
		retValue.u.strPtr = cNo;
		break;
	case SQL_KEYSET_CURSOR_ATTRIBUTES1:	
	case SQL_KEYSET_CURSOR_ATTRIBUTES2:	
		retValue.u.u32Value = 0;
		retValue.dataType = SQL_IS_UINTEGER;
		break;					
	case SQL_KEYWORDS:
		retValue.u.strPtr = cKeywords;
		break;
	case SQL_LIKE_ESCAPE_CLAUSE:
		retValue.u.strPtr = cYes;
		break;
	case SQL_LOCK_TYPES:							// 2.0
		retValue.u.u32Value = 0;					// Not supported
		retValue.dataType = SQL_IS_UINTEGER;
		break;	
	case SQL_MAX_ASYNC_CONCURRENT_STATEMENTS:
		retValue.u.u32Value = 0;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_MAX_BINARY_LITERAL_LEN:	
		retValue.u.u32Value = 2*4050;		// Must be SQL Version based
		retValue.dataType = SQL_IS_UINTEGER;
		break;	
	case SQL_MAX_CATALOG_NAME_LEN:					
		retValue.u.u16Value = 128;		// Must be SQL Version based
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_MAX_CHAR_LITERAL_LEN:					// Must be SQL Version based
		retValue.u.u32Value = 4050;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_MAX_COLUMN_NAME_LEN:					// Must be SQL Version based
		retValue.u.u16Value = 128;				
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_MAX_COLUMNS_IN_GROUP_BY:
	case SQL_MAX_COLUMNS_IN_INDEX:
	case SQL_MAX_COLUMNS_IN_ORDER_BY:
	case SQL_MAX_COLUMNS_IN_SELECT:
	case SQL_MAX_COLUMNS_IN_TABLE:
		retValue.u.u16Value = 0;
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_MAX_CONCURRENT_ACTIVITIES:
		//retValue.u.u16Value = 1;				
		retValue.u.u16Value = 0;			// 10-070910-7336
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_MAX_CURSOR_NAME_LEN:
		//retValue.u.u16Value = 30;			// Must be SQL Version based
		retValue.u.u16Value = 128;			// 10-070910-7336
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_MAX_DRIVER_CONNECTIONS:
		retValue.u.u16Value = 0;
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_MAX_IDENTIFIER_LEN:
		//retValue.u.u16Value = 30;			// Must be SQL Version based
		retValue.u.u16Value = 128;			// 10-070910-7336
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_MAX_INDEX_SIZE:
		retValue.u.u32Value = 4050;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_MAX_PROCEDURE_NAME_LEN: 
		retValue.u.u16Value = 128;			
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_MAX_ROW_SIZE:
		retValue.u.u32Value = 4050;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
		retValue.u.strPtr = cYes;
		break;
	case SQL_MAX_SCHEMA_NAME_LEN:
		retValue.u.u16Value = 128;			// Must be SQL Version based
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_MAX_STATEMENT_LEN:
		retValue.u.u32Value = 0;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_MAX_TABLE_NAME_LEN:
		retValue.u.u16Value = 128;			
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_MAX_TABLES_IN_SELECT:
		retValue.u.u16Value = 0;			
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_MAX_USER_NAME_LEN:
		retValue.u.u16Value = 32;			// Based on Safeguard alias name
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_MULT_RESULT_SETS:
		retValue.u.strPtr = cNo;
		break;
	case SQL_MULTIPLE_ACTIVE_TXN:
		retValue.u.strPtr = cNo;
		break;
	case SQL_NEED_LONG_DATA_LEN:
		retValue.u.strPtr = cNo;
		break;
	case SQL_NON_NULLABLE_COLUMNS:
		retValue.u.u16Value = SQL_NNC_NON_NULL;
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_NULL_COLLATION:
		retValue.u.u16Value = SQL_NC_HIGH;
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_NUMERIC_FUNCTIONS:
		retValue.u.u32Value = SQL_FN_NUM_ABS | SQL_FN_NUM_ACOS | SQL_FN_NUM_ASIN
				| SQL_FN_NUM_ATAN | SQL_FN_NUM_ATAN2
				| SQL_FN_NUM_CEILING | SQL_FN_NUM_COS | SQL_FN_NUM_DEGREES
				| SQL_FN_NUM_EXP | SQL_FN_NUM_FLOOR | SQL_FN_NUM_LOG
				| SQL_FN_NUM_LOG10 | SQL_FN_NUM_MOD | SQL_FN_NUM_PI | SQL_FN_NUM_POWER
				| SQL_FN_NUM_RADIANS | SQL_FN_NUM_RAND | SQL_FN_NUM_SIGN
				| SQL_FN_NUM_SIN | SQL_FN_NUM_SQRT | SQL_FN_NUM_TAN;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ODBC_API_CONFORMANCE:
		retValue.u.u16Value = SQL_OAC_LEVEL2;
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_ODBC_INTERFACE_CONFORMANCE: // 3.0
		retValue.u.u32Value = SQL_OIC_CORE;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ODBC_SAG_CLI_CONFORMANCE:	// 1.0
		retValue.u.u16Value = SQL_OSCC_COMPLIANT;
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_ODBC_SQL_CONFORMANCE:		// 1.0
		retValue.u.u16Value = SQL_OSC_CORE;
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_ODBC_VER: // Implemented by DM
		break;
	case SQL_OJ_CAPABILITIES:
	case 65003:		// This is interim value used by DM when the ODBCVER >= 0x0201 && ODBCVER < 0x0300 
					// for SQL_OJ_CAPABILITIES
		retValue.u.u32Value = SQL_OJ_LEFT | SQL_OJ_RIGHT | SQL_OJ_FULL | SQL_OJ_NESTED | SQL_OJ_NOT_ORDERED | SQL_OJ_INNER |
				SQL_OJ_ALL_COMPARISON_OPS ;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_ORDER_BY_COLUMNS_IN_SELECT:
		retValue.u.strPtr = cNo;
		break;
	case SQL_OUTER_JOINS:
		retValue.u.strPtr = cYes; 
		break;
	case SQL_PARAM_ARRAY_ROW_COUNTS:
		retValue.u.u32Value = SQL_PARC_NO_BATCH;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_PARAM_ARRAY_SELECTS:
		retValue.u.u32Value = SQL_PAS_NO_SELECT;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_POS_OPERATIONS:						// 2.0
		retValue.u.u32Value = 0;					// SQLSetPos unsupported
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_POSITIONED_STATEMENTS:					// 2.0
		retValue.u.u32Value = SQL_PS_POSITIONED_DELETE | SQL_PS_POSITIONED_UPDATE
					| SQL_PS_SELECT_FOR_UPDATE;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_PROCEDURE_TERM:
		retValue.u.strPtr = "procedure";
		break;
	case SQL_PROCEDURES:
		retValue.u.strPtr = cYes;
		break;
	case SQL_QUOTED_IDENTIFIER_CASE:
		retValue.u.u16Value = SQL_IC_SENSITIVE;
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_ROW_UPDATES:
		retValue.u.strPtr = cNo;
		break;
	case SQL_SCHEMA_TERM:
		retValue.u.strPtr = "schema";
		break;
	case SQL_SCHEMA_USAGE:
		retValue.u.u32Value = SQL_SU_DML_STATEMENTS | SQL_SU_TABLE_DEFINITION | SQL_SU_PRIVILEGE_DEFINITION;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_SCROLL_CONCURRENCY:		// 1.0
		retValue.u.u32Value = SQL_SCCO_LOCK;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_SCROLL_OPTIONS:
		retValue.u.u32Value = SQL_SO_FORWARD_ONLY;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_SEARCH_PATTERN_ESCAPE:
		retValue.u.strPtr = "\\";
		break;
	case SQL_SERVER_NAME:
		retValue.u.strPtr = m_SrvrIdentity;
		break;
	case SQL_SPECIAL_CHARACTERS:
		retValue.u.strPtr = "$\\";
		break;
	case SQL_SQL_CONFORMANCE:
		retValue.u.u32Value = SQL_SC_SQL92_ENTRY;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_SQL92_DATETIME_FUNCTIONS:
		retValue.u.u32Value = SQL_SDF_CURRENT_DATE | SQL_SDF_CURRENT_TIME 
				| SQL_SDF_CURRENT_TIMESTAMP;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_SQL92_FOREIGN_KEY_DELETE_RULE:
	case SQL_SQL92_FOREIGN_KEY_UPDATE_RULE:
	case SQL_SQL92_GRANT:
		retValue.u.u32Value = 0;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_SQL92_NUMERIC_VALUE_FUNCTIONS:
		retValue.u.u32Value = SQL_SNVF_CHAR_LENGTH | SQL_SNVF_CHARACTER_LENGTH
				| SQL_SNVF_EXTRACT | SQL_SNVF_OCTET_LENGTH | SQL_SNVF_POSITION;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_SQL92_PREDICATES:
		retValue.u.u32Value = SQL_SP_BETWEEN | SQL_SP_COMPARISON | SQL_SP_EXISTS | SQL_SP_IN
				| SQL_SP_ISNOTNULL | SQL_SP_ISNULL | SQL_SP_LIKE | SQL_SP_MATCH_FULL
				| SQL_SP_MATCH_PARTIAL | SQL_SP_QUANTIFIED_COMPARISON;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_SQL92_RELATIONAL_JOIN_OPERATORS:
		retValue.u.u32Value = SQL_SRJO_CROSS_JOIN | SQL_SRJO_FULL_OUTER_JOIN | SQL_SRJO_INNER_JOIN
				| SQL_SRJO_LEFT_OUTER_JOIN | SQL_SRJO_NATURAL_JOIN | SQL_SRJO_RIGHT_OUTER_JOIN
				| SQL_SRJO_UNION_JOIN | SQL_SRJO_CORRESPONDING_CLAUSE;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_SQL92_REVOKE:				
	case SQL_SQL92_ROW_VALUE_CONSTRUCTOR:	
		retValue.u.u32Value = 0;	
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_SQL92_STRING_FUNCTIONS:		
		retValue.u.u32Value = SQL_SSF_CONVERT | SQL_SSF_LOWER | SQL_SSF_UPPER | SQL_SSF_SUBSTRING
				| SQL_SSF_TRIM_BOTH | SQL_SSF_TRIM_LEADING | SQL_SSF_TRIM_TRAILING;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_SQL92_VALUE_EXPRESSIONS:		
		retValue.u.u32Value = SQL_SVE_CASE | SQL_SVE_CAST;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_STANDARD_CLI_CONFORMANCE:		
		retValue.u.u32Value = SQL_SCC_XOPEN_CLI_VERSION1 | SQL_SCC_ISO92_CLI;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_STATIC_CURSOR_ATTRIBUTES1:		
	case SQL_STATIC_CURSOR_ATTRIBUTES2:
		retValue.u.u32Value = 0;				
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_STATIC_SENSITIVITY:		// 2.0
		retValue.u.u32Value = SQL_SS_ADDITIONS | SQL_SS_DELETIONS | SQL_SS_UPDATES;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_STRING_FUNCTIONS:
		retValue.u.u32Value = SQL_FN_STR_ASCII | SQL_FN_STR_CHAR | SQL_FN_STR_CONCAT 
					| SQL_FN_STR_INSERT | SQL_FN_STR_LCASE | SQL_FN_STR_LEFT 
					| SQL_FN_STR_LENGTH | SQL_FN_STR_LOCATE_2| SQL_FN_STR_LTRIM
					| SQL_FN_STR_REPEAT | SQL_FN_STR_REPLACE
					| SQL_FN_STR_RIGHT | SQL_FN_STR_RTRIM | SQL_FN_STR_SPACE
					| SQL_FN_STR_SUBSTRING | SQL_FN_STR_UCASE;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_SUBQUERIES:
		retValue.u.u32Value = SQL_SQ_CORRELATED_SUBQUERIES | SQL_SQ_COMPARISON
					| SQL_SQ_EXISTS | SQL_SQ_IN | SQL_SQ_QUANTIFIED;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_SYSTEM_FUNCTIONS:
//		retValue.u.u32Value = SQL_FN_SYS_DBNAME | SQL_FN_SYS_USERNAME;
		retValue.u.u32Value = SQL_FN_SYS_USERNAME;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_TABLE_TERM:
		retValue.u.strPtr = "table";
		break;
	case SQL_TIMEDATE_ADD_INTERVALS:
		retValue.u.u32Value = SQL_FN_TSI_FRAC_SECOND | SQL_FN_TSI_SECOND
					| SQL_FN_TSI_MINUTE | SQL_FN_TSI_HOUR | SQL_FN_TSI_DAY
					| SQL_FN_TSI_WEEK | SQL_FN_TSI_MONTH | SQL_FN_TSI_YEAR;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_TIMEDATE_DIFF_INTERVALS:		
		retValue.u.u32Value = SQL_FN_TSI_FRAC_SECOND | SQL_FN_TSI_SECOND
					| SQL_FN_TSI_MINUTE | SQL_FN_TSI_HOUR | SQL_FN_TSI_DAY
					| SQL_FN_TSI_WEEK | SQL_FN_TSI_MONTH | SQL_FN_TSI_YEAR;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_TIMEDATE_FUNCTIONS:			
//		retValue.u.u32Value = SQL_FN_TD_CURDATE | SQL_FN_TD_CURTIME | SQL_FN_TD_DAYOFMONTH
//					| SQL_FN_TD_NOW | SQL_FN_TD_DAYOFWEEK;
		retValue.u.u32Value = SQL_FN_TD_CURRENT_DATE | SQL_FN_TD_CURRENT_TIME 
				    | SQL_FN_TD_CURRENT_TIMESTAMP | SQL_FN_TD_DAYNAME 
				    | SQL_FN_TD_DAYOFMONTH | SQL_FN_TD_DAYOFWEEK 
				    | SQL_FN_TD_DAYOFYEAR | SQL_FN_TD_HOUR 
				    | SQL_FN_TD_MINUTE | SQL_FN_TD_MONTH 
				    | SQL_FN_TD_MONTHNAME | SQL_FN_TD_QUARTER 
				    | SQL_FN_TD_SECOND | SQL_FN_TD_WEEK 
				    | SQL_FN_TD_YEAR;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_TXN_CAPABLE:
		retValue.u.u16Value = SQL_TC_ALL;
		retValue.dataType = SQL_IS_USMALLINT;
		break;
	case SQL_TXN_ISOLATION_OPTION:
		retValue.u.u32Value = SQL_TXN_READ_UNCOMMITTED | SQL_TXN_READ_COMMITTED
					| SQL_TXN_REPEATABLE_READ  | SQL_TXN_SERIALIZABLE;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_UNION:
		retValue.u.u32Value = SQL_U_UNION;
		retValue.dataType = SQL_IS_UINTEGER;
		break;
	case SQL_USER_NAME:
		retValue.u.strPtr = m_UserName;
		break;
	case SQL_XOPEN_CLI_YEAR: 
		retValue.u.strPtr = "1995";
		break;
	default:
		setDiagRec(DRIVER_ERROR, IDS_HY_096);
		rc = SQL_ERROR;
		break;
	}

	if (rc == SQL_SUCCESS)
	{
		rc = returnAttrValue(TRUE, this, &retValue, InfoValuePtr, BufferLength, &lStringLength);
		if (StringLengthPtr != NULL)
			*StringLengthPtr = (SQLSMALLINT)lStringLength;
	}
	return rc;
}

short CConnect::getTransactionNumber(void)
{
	CStmt* pStmt;
	short count=0;
#ifdef VERSION3
	std::list<CHandlePtr>::iterator i;
	for (i = m_StmtCollect.begin() ; i !=  m_StmtCollect.end() ; ++i)
		if ((pStmt = (CStmt *)(*i)) != NULL)
#else	
	RWTPtrSlistIterator<CHandle> i(m_StmtCollect);
	while ((pStmt = (CStmt *)i()) != NULL)
#endif	
			if(pStmt->getStmtState() == STMT_FETCHED)
				count++;
/*
	CHANDLECOLLECT::iterator i;
	for (i = m_StmtCollect.begin() ; i !=  m_StmtCollect.end() ; ++i)
		if(((CStmt *)(*i))->getStmtState() == STMT_FETCHED)
			count++;
*/
	return count;
}

void CConnect::deleteValueList()
{ 
	CStmt* pStmt;
	short count=0;

#ifdef VERSION3
	std::list<CHandlePtr>::iterator i;
	for (i = m_StmtCollect.begin() ; i !=  m_StmtCollect.end() ; ++i)
		if ((pStmt = (CStmt *)(*i)) != NULL)
#else
	RWTPtrSlistIterator<CHandle> i(m_StmtCollect);
	while ((pStmt = (CStmt *)i()) != NULL)
/*
	CHANDLECOLLECT::iterator i;
	for (i = m_StmtCollect.begin() ; i !=  m_StmtCollect.end() ; ++i)
		((CStmt *)(*i))->deleteValueList();
*/
#endif
	
		pStmt->deleteValueList();
}

void CConnect::restoreDefaultDescriptor(CDesc* desc)
{ 
	CStmt* pStmt;
	short count=0;

#ifdef VERSION3
	std::list<CHandlePtr>::iterator i;
	for (i = m_StmtCollect.begin() ; i !=  m_StmtCollect.end() ; ++i)
		if ((pStmt = (CStmt *)(*i)) != NULL)
#else
	RWTPtrSlistIterator<CHandle> i(m_StmtCollect);
	while ((pStmt = (CStmt *)i()) != NULL)
#endif

		pStmt->restoreDefaultDescriptor(desc);
}


/* Rethink about this function - we don't need now, but left it for some history
/* might require a mechanism by which all translations are OFF!
/* This function is used when "UCS2Translation" datasource
 * attribute is set. The function checks whether the default 
 * locale of the system has been changed or not.
 * Return value of this function depends on the functioning of 
 * CConnect::getDrvrCharSet() and the default LC_CTYPE value 
 * of the paltform. For example, on HPUX or SUN or AIX, if the 
 * locale settings were not changed CConnect::getDrvrCharSet()
 * will return cnv_ISO88591.
 * NOTE that for a new supported platform (for driver)
 * or a change in CConnect::getDrvrCharSet() may require
 * changes in this function. 
 */
DWORD CConnect::defaultLocale()
{
#if defined(MXHPUX) || defined(MXSUNSPARC) || defined(MXAIX)
	return cnv_ISO88591;
#elif defined (MXLINUX)	
	return cnv_UTF8;
#else //unknown plarform, this should never be returned 
	return cnv_ISO88591;
#endif
}

DWORD CConnect::getSqlCharSet(long inSqlCharSet)
{
	switch(inSqlCharSet)
	{
	case SQLCHARSETCODE_ISO88591:
		return cnv_ISO88591;
	case SQLCHARSETCODE_SJIS:
		return cnv_SJIS;
	case SQLCHARSETCODE_KSC5601:
		return cnv_KSC;
	case SQLCHARSETCODE_BIG5:
		return cnv_BIG5;
	case SQLCHARSETCODE_GB2312:
		return cnv_GB2312;
	case SQLCHARSETCODE_GB18030:
		return cnv_GB18030;
	case SQLCHARSETCODE_UTF8:
		return cnv_UTF8;
	case SQLCHARSETCODE_UCS2:
		return cnv_UTF16;
	case SQLCHARSETCODE_UNKNOWN:
	case SQLCHARSETCODE_KANJI:
	default:
		return cnv_ISO88591;
	}
}

//01-080313-1379
//UTF8 character identification in the following code
//The most significant bit of a single-byte character is always 0. 
//The most significant bits of the first byte of a multi-byte sequence determine the length of the sequence. 
//These most significant bits are: 110 for two-byte sequences; 1110 for three-byte sequences, and so on. 
//The remaining bytes in a multi-byte sequence have 10 as their two most significant bits. 	
int CConnect::getUTF8CharLength(const char *inputChar, const int inputLength, const int maxLength)
{
	int length = 0;
	int numBytesInChar = 0;
	int idx = 0;
	unsigned char byte;

	while (inputChar[idx])
	{
		byte = inputChar[idx];
		if (byte & 0x80) //multibyte ?
		{    
			if ((byte & 0x40) && (byte & 0x20) && (byte & 0x10))
				numBytesInChar = 4;
			else if ((byte & 0x40) && (byte & 0x20) && !(byte & 0x10))
				numBytesInChar = 3;
			else if ((byte & 0x40) && !(byte & 0x20) && !(byte & 0x10))
				numBytesInChar = 2;
			if ((idx + numBytesInChar) > maxLength)
				break;
		}
		else
		{
			numBytesInChar = 1;
			if ((idx + numBytesInChar) > maxLength)
				break;
		}

		idx += numBytesInChar;
		length += numBytesInChar;
	}

	return length;
}

void preFetchThread::setSrvrCallContext(SRVR_CALL_CONTEXT *srvrCallContext) {
      memcpy(&m_SrvrCallContext,srvrCallContext,sizeof(SRVR_CALL_CONTEXT));
}
#ifndef MXOSS
SQLRETURN CConnect::DoEncryption(SecPwd* &pSecPwd, ProcInfo SecInfo,USER_DESC_def &userDesc,char *AuthenticationNTS,CONNECTION_CONTEXT_def &inContext)
{
	short retCode=SQL_SUCCESS;

	char *dir;
	char *file;
	char *activefile; 
	unsigned char *certificate_ts; 

	dir = NULL;
	file = NULL;
	activefile = NULL;
	certificate_ts = NULL;

	char *pwdKey=NULL;
	int	  pwdKeyLen=0;


	if (m_CertificateDir[0] != '\0')
		dir = m_CertificateDir;
	else if (m_DSValue.m_DSCertificateDir[0] != '\0' && strcmp(m_DSValue.m_DSCertificateDir, "SYSTEM_DEFAULT") != 0)
		dir = m_DSValue.m_DSCertificateDir;
	if (m_CertificateFile[0] != '\0')
		file = m_CertificateFile;
	else if (m_DSValue.m_DSCertificateFile[0] != '\0')
		file = m_DSValue.m_DSCertificateFile;
	if (m_CertificateFileActive[0] != '\0')
		activefile = m_CertificateFileActive;
	else if (m_DSValue.m_DSCertificateFileActive[0] != '\0')
		activefile = m_DSValue.m_DSCertificateFileActive;

	try 
	{
		if(pSecPwd == NULL)
			pSecPwd = new SecPwd(dir, file, activefile, m_ClusterName);
	} 
	catch (SecurityException se) 
	{
			retCode = se.getErrCode();
			setSecurityError(retCode, se.getSQLState(), se.getMsg());
	}

	if (retCode == SQL_SUCCESS)
	{
		try
		{
			pSecPwd->openCertificate();
		} 
		catch (SecurityException se) 
		{
			retCode = se.getErrCode();
			if (retCode == FILE_NOTFOUND) // certificate file not found, need to download
				inContext.inContextOptions1 = inContext.inContextOptions1 | INCONTEXT_OPT1_CERTIFICATE_TIMESTAMP;
			else
				setSecurityError(retCode, se.getSQLState(), se.getMsg());
		}
	}

	if (retCode == SQL_SUCCESS)
	{
		try
		{
			certificate_ts = pSecPwd->getCertExpDate();
			inContext.inContextOptions1 = inContext.inContextOptions1 | INCONTEXT_OPT1_CERTIFICATE_TIMESTAMP;
			inContext.connectOptions = (IDL_string)certificate_ts;
			pwdKeyLen = pSecPwd->getPwdEBufferLen();
			pwdKey = new char [pwdKeyLen]; // caller needs to free this
			pSecPwd->encryptPwd(AuthenticationNTS, strlen(AuthenticationNTS),
								 inContext.userRole, strlen(inContext.userRole),
								 (const char*)&SecInfo, sizeof(SecInfo),
								 pwdKey, &pwdKeyLen);


		} 
		catch (SecurityException se) 
		{
			retCode = se.getErrCode();
			setSecurityError(retCode, se.getSQLState(), se.getMsg());
		}
	}

	if (retCode == SQL_SUCCESS)
	{
		userDesc.password._length = pwdKeyLen;
		userDesc.password._buffer = (IDL_octet *)pwdKey;
	}
	else
	{
		userDesc.userName = NULL;
		userDesc.password._length = 0;
		userDesc.password._buffer = NULL;
	}

	return retCode;

} /* DoEncryption() */

void CConnect::setSecurityError(int error, char* sqlState, char* errorMsg)
{
	setDiagRec(SQLMX_ERROR, IDS_SQL_ERROR, error, errorMsg, sqlState);
	return;
}
#endif
