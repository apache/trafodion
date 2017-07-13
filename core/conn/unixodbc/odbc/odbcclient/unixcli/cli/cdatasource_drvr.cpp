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
#include "cdatasource.h"
//#include "NLSFunctions.h"
#include "cconnect.h"
#include "errno.h"
#include "transport.h"
#include "odbcinst.h"

// Implements the member functions of CDataSource

#define MAX_REGKEY_PATH_LEN			512
#define SYSTEM_DEFAULT				"SYSTEM_DEFAULT"
#define NO_TIMEOUT					"NO_TIMEOUT"
#define CONNECTION_TIMEOUT_DEFAULT	60
#define LOGIN_TIMEOUT_DEFAULT		60
#define	QUERY_TIMEOUT_DEFAULT		0
#define FETCH_BUFFER_SIZE_DEFAULT	512 * 1024
#define TCP_DEFAULT_PROCESS			"$ZTC0"

extern int hpodbc_dmanager = 0;

CDataSource::CDataSource()
{
	m_DSName[0] = '\0';
	m_DSLocation = 0;
	m_DSServer[0] = '\0';
	m_DSCatalog[0] = '\0';
	m_DSCtrlInferNCHAR = FALSE;
	m_DSCharSet[0] = '\0';
	m_DSDataLang = MAKELCID (MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), SORT_DEFAULT);
	m_DSErrorMsgLang = MAKELCID (MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), SORT_DEFAULT);
	m_DSFetchBufferSize = FETCH_BUFFER_SIZE_DEFAULT;
	m_DSSchema[0] = '\0';
	m_DSConnectionTimeout = CONNECTION_TIMEOUT_DEFAULT;
	m_DSLoginTimeout = LOGIN_TIMEOUT_DEFAULT;
	m_DSQueryTimeout = QUERY_TIMEOUT_DEFAULT;
	m_DSTranslationDLL[0] = '\0';
	m_DSTranslationOption = 0;
	m_DSReplacementChar[0] = '\0';
	m_DSSelectRowsets = TRUE;
	m_DSTcpProcessName[0] = '\0';
	m_DSRowsetErrorRecovery = FALSE;
	m_DSServerDSName[0] = 0;
	m_DSServiceName[0] = 0;
	m_DS_UCS2Translation = TRUE;
	m_DSSession[0] = '\0';
	m_DSApplication[0] = '\0';
	m_DSRoleName[0] = '\0';
	m_DSCertificateDir[0] = '\0';
	m_DSCertificateFile[0] = '\0';
	m_DSCertificateFileActive[0] = '\0';
	strcpy(m_DSCertificateDir,"SYSTEM_DEFAULT");
	m_DSIOCompression = 0;

	if(gDrvrGlobal.fpSQLGetPrivateProfileString != NULL)
	{
		DWORD	len;
		char	searchKey[MAX_REGKEY_PATH_LEN+1];
		CHAR*	szODBC = "ODBC";
		char *OdbcIniEnv = getenv("ODBCINI");
		//
		// Get Certificate Directory Location
		// Search for it first in the Datasource section, then try the odbc section
		//

		strncpy(searchKey, (const char *)m_DSName, sizeof(searchKey));
		searchKey[sizeof(searchKey)-1] = 0;

		len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
							searchKey,
							"CertificateDir",
							"SYSTEM_DEFAULT",
							m_DSCertificateDir,
							sizeof(m_DSCertificateDir),
							OdbcIniEnv);

		if(strcmp(m_DSCertificateDir,"SYSTEM_DEFAULT") == 0)
		{
			strncpy(searchKey, (const char *)szODBC, sizeof(searchKey));
			len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
								searchKey,
								"CertificateDir",
								"SYSTEM_DEFAULT",
								m_DSCertificateDir,
								sizeof(m_DSCertificateDir),
								OdbcIniEnv);
		}
	}
}

short CDataSource::readDSValues(char *DSName,CConnect* pConnection)
{
//	char	path[EXT_FILENAME_LEN+1];
	char	path[MAX_DSN_NAME_LEN];

	DWORD	len;
	char	searchKey[MAX_REGKEY_PATH_LEN+1];
//	HKEY	keyHandle;
	char	keyValueBuf[256];
	DWORD	keyValueLength;
	DWORD	keyValueType;
	long	tempValue;
	short	i = 0;
	DWORD	retcode = ERROR_SUCCESS;
	char    errorbuffer[128];
	CHAR*	szODBC = "ODBC";
	char* pSlash=NULL;

  // fpSQLGetPrivateProfileString should already be initialized in drvrglobal

	if(gDrvrGlobal.fpSQLGetPrivateProfileString == NULL)
	{
		pConnection->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0,"Unable to find exported symbol SQLGetPrivateProfileString");
		return SQL_ERROR;
	}

	/*
	 * SQLGetPrivateProfileString for the DataDirect Driver manager expects the 
	 * last param to be passed in, unlike iODBC & unixODBC 
	 * So, we'll set the last param to the env pointed to by ODBCINI (defaults to NULL)
	 */
	char *OdbcIniEnv = getenv("ODBCINI");

	//
	//	Read the ODBC sections
	//


	//
	//	Read the Datasource sections
	//


	strncpy(m_DSName, (const char *)DSName, sizeof(m_DSName));
	m_DSName[sizeof(m_DSName)-1] = 0;

	strncpy(searchKey, (const char *)DSName, sizeof(searchKey));
	searchKey[sizeof(searchKey)-1] = 0;

	path[0] = NULL;
	// Read AssociationService	Value
	keyValueLength = sizeof(m_DSServer);
	len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
			searchKey,
			"Server",
			"",
			m_DSServer,
			keyValueLength,
			OdbcIniEnv);

	if (len == 0)
		return DS_AS_KEY_NOT_FOUND;
	if (strncmp(m_DSServer, "TCP:", 4) != 0 )
		return DS_AS_PROCESS_NAME_INCORRECT;
	if((pSlash=strchr(m_DSServer,'/'))!=NULL)
		*pSlash=':';


	// Read Catalog	Value
	keyValueLength = sizeof(m_DSCatalog);
	len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
						searchKey,
						"Catalog",
						"",
						m_DSCatalog,
						keyValueLength,
						OdbcIniEnv);
	if (len <= 0)
		m_DSCatalog[0] = '\0';

	// Read CtrlInferNCHAR Value
	keyValueLength = sizeof(keyValueBuf);
	len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
						searchKey,
						"CtrlInferNCHAR",
						"",
						keyValueBuf,
						keyValueLength,
						OdbcIniEnv);
	if (len > 0)
	{
		tempValue = atol((const char *)keyValueBuf);
		if (tempValue)
			m_DSCtrlInferNCHAR = TRUE;
		else
			m_DSCtrlInferNCHAR = FALSE;
	}
	else
		m_DSCtrlInferNCHAR = TRUE;

	// Read FetchBufferSize
	keyValueLength = sizeof(keyValueBuf);
	len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
						searchKey,
						"FetchBufferSize",
						"",
						keyValueBuf,
						keyValueLength,
						OdbcIniEnv);
	if (len > 0)
	{	
		if (strcmp((const char *)keyValueBuf, SYSTEM_DEFAULT) == 0)
			m_DSFetchBufferSize = FETCH_BUFFER_SIZE_DEFAULT;
		else
			m_DSFetchBufferSize = atol((const char *)keyValueBuf) * 1024;
	}
	else
		m_DSFetchBufferSize = FETCH_BUFFER_SIZE_DEFAULT;

	// Read Schema Value
	keyValueLength = sizeof(m_DSSchema);
	len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
						searchKey,
						"Schema",
						"",
						m_DSSchema,
						keyValueLength,
						OdbcIniEnv);
	if (len <= 0)
		m_DSSchema[0] = '\0';

	// Read SQL_ATTR_CONNECTION_TIMEOUT value
	keyValueLength = sizeof(keyValueBuf);
	len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
						searchKey,
						"SQL_ATTR_CONNECTION_TIMEOUT",
						"",
						keyValueBuf,
						keyValueLength,
						OdbcIniEnv);
	if (len > 0)
	{	
		if (strcmp((const char *)keyValueBuf, SYSTEM_DEFAULT) == 0)
			m_DSConnectionTimeout = CONNECTION_TIMEOUT_DEFAULT;
		else
		if (strcmp((const char *)keyValueBuf, NO_TIMEOUT) == 0)
			m_DSConnectionTimeout = NO_LOGIN_TIMEOUT;
		else
			m_DSConnectionTimeout = atol((const char *)keyValueBuf);
	}
	else
		m_DSConnectionTimeout = CONNECTION_TIMEOUT_DEFAULT;
		
	// Read SQL_LOGIN_TIMEOUT value
	keyValueLength = sizeof(keyValueBuf);
	len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
						searchKey,
						"SQL_LOGIN_TIMEOUT",
						"",
						keyValueBuf,
						keyValueLength,
						OdbcIniEnv);
	if (len > 0)
	{	
		if (strcmp((const char *)keyValueBuf, SYSTEM_DEFAULT) == 0)
			m_DSLoginTimeout = LOGIN_TIMEOUT_DEFAULT;
		else
		if (strcmp((const char *)keyValueBuf, NO_TIMEOUT) == 0)
			m_DSLoginTimeout = NO_LOGIN_TIMEOUT;
		else
			m_DSLoginTimeout = atol((const char *)keyValueBuf);
	}
	else
		m_DSLoginTimeout = LOGIN_TIMEOUT_DEFAULT;
	
	// Read SQL_QURRY_TIMEOUT value
	keyValueLength = sizeof(keyValueBuf);
	len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
						searchKey,
						"SQL_QUERY_TIMEOUT",
						"",
						keyValueBuf,
						keyValueLength,
						OdbcIniEnv);
	if (len > 0)
	{	
		if (strcmp((const char *)keyValueBuf, SYSTEM_DEFAULT) == 0)
			m_DSQueryTimeout = QUERY_TIMEOUT_DEFAULT;
		else
		if (strcmp((const char *)keyValueBuf, NO_TIMEOUT) == 0)
			m_DSQueryTimeout = NO_LOGIN_TIMEOUT;
		else
		{
			m_DSQueryTimeout = atol((const char *)keyValueBuf);
			if(m_DSQueryTimeout != 0 && m_DSQueryTimeout < 30)
				m_DSQueryTimeout = 30;
		}
	}
	else
		m_DSQueryTimeout = QUERY_TIMEOUT_DEFAULT;
	
	// Read TranslationDLL Value
	keyValueLength = sizeof(m_DSTranslationDLL);
	len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
						searchKey,
						"TranslationDLL",
						"",
						m_DSTranslationDLL,
						keyValueLength,
						OdbcIniEnv);
	if (len <= 0)
		m_DSTranslationDLL[0] = '\0';
	if (stricmp(m_DSTranslationDLL,"null")==0 || stricmp(m_DSTranslationDLL,"0")==0)
		m_DSTranslationDLL[0] = '\0';

	// Read TranslationOption Value
	keyValueLength = sizeof(keyValueBuf);
	len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
						searchKey,
						"TranslationOption",
						"",
						keyValueBuf,
						keyValueLength,
						OdbcIniEnv);
	if (len > 0)
		m_DSTranslationOption = atol((const char *)keyValueBuf);
	else
		m_DSTranslationOption = 0;

// Read select rowsets value
// It is internal parameters not exposed through the administrator, where we define the behavior for rowset or non-rowset

	keyValueLength = sizeof(keyValueBuf);
	len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
						searchKey,
						"SelectRowsets",
						"",
						keyValueBuf,
						keyValueLength,
						OdbcIniEnv);

	if (len > 0)
	{
		if (atol((const char *)keyValueBuf))
			m_DSSelectRowsets = TRUE;
		else
			m_DSSelectRowsets = FALSE;
	}
	else
		m_DSSelectRowsets = TRUE;

// Read tcp process name
	keyValueLength = sizeof(m_DSTcpProcessName);
	len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
						searchKey,
						"TcpProcessName",
						"",
						m_DSTcpProcessName,
						keyValueLength,
						OdbcIniEnv);
	if (len <= 0)
		strcpy(m_DSTcpProcessName,TCP_DEFAULT_PROCESS);
//
// Patch for Informatica to recover from insert rowset error
//
	keyValueLength = sizeof(keyValueBuf);
	len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
						searchKey,
						"RowsetErrorRecovery",
						"",
						keyValueBuf,
						keyValueLength,
						OdbcIniEnv);

	if (len > 0)
	{
		if (atol((const char *)keyValueBuf))
			m_DSRowsetErrorRecovery = TRUE;
		else
			m_DSRowsetErrorRecovery = FALSE;
	}
	else
		m_DSRowsetErrorRecovery = TRUE;

	keyValueLength = sizeof(m_DSServerDSName);
	len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
						searchKey,
						"ServerDSN",
						"",				// Reserved
						m_DSServerDSName,
						keyValueLength,
						OdbcIniEnv);
	if (len <= 0)
		m_DSServerDSName[0] = '\0';

	keyValueLength = sizeof(keyValueBuf);
	len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
						searchKey,
						"MapErrors",
						"",
						keyValueBuf,
						keyValueLength,
						OdbcIniEnv);

	if (len > 0)
	{
		if (atol((const char *)keyValueBuf))
			GTransport.bMapErrors = TRUE;
		else
			GTransport.bMapErrors = FALSE;
	}
	else
		GTransport.bMapErrors = TRUE;

//
// Compression types to be used
//
	keyValueLength = sizeof(keyValueBuf);
	len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
						searchKey,
						"Compression",
						SYSTEM_DEFAULT,
						keyValueBuf,
						keyValueLength,
						OdbcIniEnv);

	if (len > 0)
	{
		if (stricmp(keyValueBuf, SYSTEM_DEFAULT) == 0){
			m_DSIOCompression = 0;
		}
		else if(strcasecmp(keyValueBuf,"no compression")==0){
			m_DSIOCompression = COMP_NO_COMPRESSION;
		}
		else if(strcasecmp(keyValueBuf,"best speed")==0){
			m_DSIOCompression = COMP_BEST_SPEED;
		}
		else if(strcasecmp(keyValueBuf,"best compression")==0){
			m_DSIOCompression = COMP_BEST_COMPRESSION;
		}
		else if(strcasecmp(keyValueBuf,"balance")==0){
			m_DSIOCompression = COMP_DEFAULT;
		}
		else{
			m_DSIOCompression = atol((const char *)keyValueBuf);
		}
	}
	else
		m_DSIOCompression = 0;


	keyValueLength = sizeof(m_DSServiceName);
	len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
						searchKey,
						"ServiceName",
						"",
						m_DSServiceName,
						keyValueLength,
						OdbcIniEnv);

	if (len <= 0)
		m_DSServiceName[0] = '\0';

// Read ClientCharSet Value
	keyValueLength = sizeof(m_DSCharSet);
	len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
						searchKey,
						"ClientCharSet",
						"ISO-8859-1",
						m_DSCharSet,
						sizeof(m_DSCharSet),
						OdbcIniEnv);
	if (len <= 0)
		m_DSCharSet[0] = '\0';

// Read ReplacementCharacter Value
	keyValueLength = sizeof(m_DSReplacementChar);
	len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
						searchKey,
						"ReplacementCharacter",
						"",
						m_DSReplacementChar,
						keyValueLength,
						OdbcIniEnv);
	if (len <= 0)
		m_DSReplacementChar[0] = '\0';
	
// UCS2Translation : An internal parameter not exposed through administrator.
// It is used for to insert/retrieve an application-translated UCS2 string.
// If an application needs no Translation UCS2Translation should be set to "0"
// It is valid only if server ISOMapping is ISO88591, otherwise ignored. 
// UCS2Translation is by default is ON (means either no Ucs2Translation attribute 
// specified in the datasource or it is present and has a value of 1.
// Read UCS2Translation Value
	keyValueLength = sizeof(keyValueBuf);
	len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
						searchKey,
						"UCS2Translation",
						"",
						keyValueBuf,
						keyValueLength,
						OdbcIniEnv);
	if (len > 0)
	{
		int val =  atol((const char *)keyValueBuf);
		if ( val == 0 )
			m_DS_UCS2Translation = FALSE;
		else
			m_DS_UCS2Translation = TRUE;
	}
	else
		m_DS_UCS2Translation = TRUE;

    // Read AppUnicodeType Value
    keyValueLength = sizeof(keyValueBuf);
    len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
                        searchKey,
                        "AppUnicodeType",
                        "",
                        keyValueBuf,
                        keyValueLength,
                        OdbcIniEnv);
	if (len > 0)
	{
        if( strcmp(keyValueBuf,"utf8") == 0 )
            gDrvrGlobal.ICUConv.m_AppUnicodeType == APP_UNICODE_TYPE_UTF8;
        else if( strcmp(keyValueBuf,"utf16") == 0 )
            gDrvrGlobal.ICUConv.m_AppUnicodeType == APP_UNICODE_TYPE_UTF16;
    }

	return ERROR_SUCCESS;
}

void CDataSource::updateDSValues(short DSNType, CONNECT_FIELD_ITEMS *connectFieldItems,
								 CONNECT_KEYWORD_TREE *keywordTree)
{

	long	tempValue;
	char	AttrValue[256];
	short	i;

	if (DSNType == DRIVER_KW_DSN)
		m_DSName[0] = '\0';
	strcpy(m_DSServer, connectFieldItems->server);
	strcpy(m_DSCatalog, connectFieldItems->catalog);
	strcpy(m_DSSchema, connectFieldItems->schema);
	
	for (i = KEY_SCHEMA+1 ; i < KEY_MAX ; i++)
	{
		if (keywordTree[i].AttrRank != 0)
		{
			strncpy(AttrValue, keywordTree[i].AttrValue, keywordTree[i].AttrLength);
			AttrValue[keywordTree[i].AttrLength] = '\0';
			switch (i)
			{
			case KEY_CTRLINFERNCHAR:
				tempValue = atol(AttrValue);
				if (tempValue)
					m_DSCtrlInferNCHAR = TRUE;
				else
					m_DSCtrlInferNCHAR = FALSE;
				break;
			case KEY_DATALANG:
				m_DSDataLang = atol(keywordTree[KEY_DATALANG].AttrValue);
				ODBCNLS_ValidateLanguage((unsigned long*)&m_DSDataLang);
				break;
			case KEY_ERRORMSGLANG:
				m_DSErrorMsgLang = atol(AttrValue);
				ODBCNLS_ValidateLanguage((unsigned long*)&m_DSErrorMsgLang);
				break;
			case KEY_FETCHBUFFERSIZE:
				if (stricmp(AttrValue, SYSTEM_DEFAULT) == 0)
					m_DSFetchBufferSize = FETCH_BUFFER_SIZE_DEFAULT;
				else
					m_DSFetchBufferSize = atol(AttrValue);
				break;
			case KEY_SQL_ATTR_CONNECTION_TIMEOUT:
				if (stricmp(AttrValue, SYSTEM_DEFAULT) == 0)
					m_DSConnectionTimeout = CONNECTION_TIMEOUT_DEFAULT;
				else
					m_DSConnectionTimeout = atol(AttrValue);
				break;
			case KEY_SQL_LOGIN_TIMEOUT:
				if (stricmp(AttrValue, SYSTEM_DEFAULT) == 0)
					m_DSLoginTimeout = LOGIN_TIMEOUT_DEFAULT;
				else
					m_DSLoginTimeout = atol(AttrValue);
				break;
			case KEY_SQL_QUERY_TIMEOUT:
				if (stricmp(AttrValue, SYSTEM_DEFAULT) == 0)
					m_DSQueryTimeout = QUERY_TIMEOUT_DEFAULT;
				else
					m_DSQueryTimeout = atol(AttrValue);
			case KEY_TRANSLATIONDLL:
				strncpy(m_DSTranslationDLL, AttrValue, sizeof(m_DSTranslationDLL));
				m_DSTranslationDLL[sizeof(m_DSTranslationDLL)-1] = 0;
				break;
			case KEY_TRANSLATIONOPTION:
				m_DSTranslationOption = atol(AttrValue);
				break;
			case KEY_SDSN:
				strncpy(m_DSServerDSName, AttrValue, sizeof(m_DSServerDSName));
				m_DSServerDSName[sizeof(m_DSServerDSName)-1] = 0;
				break;
			case KEY_SN:
				strncpy(m_DSServiceName, AttrValue, sizeof(m_DSServiceName));
				m_DSServiceName[sizeof(m_DSServiceName)-1] = 0;
				break;
			case KEY_SESSION:
				strncpy(m_DSSession, AttrValue, sizeof(m_DSSession));
				m_DSSession[sizeof(m_DSSession)-1] = 0;
				break;
			case KEY_APPLICATION:
				strncpy(m_DSApplication, AttrValue, sizeof(m_DSApplication));
				m_DSApplication[sizeof(m_DSApplication)-1] = 0;
				break;
			case KEY_ROLENAME:
				strncpy(m_DSRoleName, AttrValue, sizeof(m_DSRoleName));
				m_DSRoleName[sizeof(m_DSRoleName)-1] = 0;
				break;
			case KEY_CERTIFICATEDIR:
				strncpy(m_DSCertificateDir, AttrValue, sizeof(m_DSCertificateDir));
				m_DSCertificateDir[sizeof(m_DSCertificateDir)-1] = 0;
				break;
			case KEY_CERTIFICATEFILE:
				strncpy(m_DSCertificateFile, AttrValue, sizeof(m_DSCertificateFile));
				m_DSCertificateFile[sizeof(m_DSCertificateFile)-1] = 0;
				break;
			case KEY_CERTIFICATEFILE_ACTIVE:
				strncpy(m_DSCertificateFileActive, AttrValue, sizeof(m_DSCertificateFileActive));
				m_DSCertificateFileActive[sizeof(m_DSCertificateFileActive)-1] = 0;
				break;

			default:
				break;
			}
		}
	}

}

