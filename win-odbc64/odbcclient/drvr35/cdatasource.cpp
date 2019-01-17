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
#include "CDataSource.h"
#include "NLSFunctions.h"
#include "Transport.h"

// Implements the member functions of CDataSource

#define MAX_REGKEY_PATH_LEN			512
#define	ODBC_DS_KEY					"SOFTWARE\\ODBC\\ODBC.INI\\"
#define ODBC_CERTIFICATE_KEY		"SOFTWARE\\ODBC\\ODBCINST.INI\\TRAF ODBC 2.3"
#define SYSTEM_DEFAULT				"SYSTEM_DEFAULT"
#define NO_TIMEOUT					"NO_TIMEOUT"
#define CONNECTION_TIMEOUT_DEFAULT	60
#define LOGIN_TIMEOUT_DEFAULT		60
#define	QUERY_TIMEOUT_DEFAULT		0
#define IOCOMPRESSION_DEFAULT       1000
#define FETCH_BUFFER_SIZE_DEFAULT	512 * 1024
//#define FETCH_BUFFER_SIZE_DEFAULT   0


CDataSource::CDataSource()
{
	m_DSName[0] = '\0';
	m_DSLocation = 0;
	m_DSServer[0] = '\0';
	m_DSCatalog[0] = '\0';
	m_DSCtrlInferNCHAR = FALSE;
	m_DSCharSet = 0;
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
	m_DSRowsetErrorRecovery = FALSE;
	m_DSServerDSName[0] = 0;
	m_DSServiceName[0] = 0;
	m_DSSession[0] = '\0';
	m_DSApplication[0] = '\0';
	m_DSRoleName[0] = '\0';
	m_DSCertificateDir[0] = '\0';
	m_DSCertificateFile[0] = '\0';
	m_DSCertificateFileActive[0] = '\0';
	m_DSIOCompression = 0;

	DWORD	error;
	HKEY	keyHandle;
	DWORD	keyValueLength;
	DWORD	keyValueType;
	error = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
						 ODBC_CERTIFICATE_KEY,
						 0,
						 KEY_READ,
						 &keyHandle);
	if (error == ERROR_SUCCESS)
	{
		// Read CertificateDir	Value
		keyValueLength = sizeof(m_DSCertificateDir);
		error = RegQueryValueEx (keyHandle,
						   "CertificateDir",
						   NULL,				// Reserved
						   &keyValueType,
						   (LPBYTE)m_DSCertificateDir,
						   &keyValueLength);
		if (error != ERROR_SUCCESS)
			m_DSCertificateDir[0] = '\0';
		RegCloseKey(keyHandle);
	}
	else 
		m_DSCertificateDir[0] = '\0';
}

short CDataSource::readDSValues(char *DSName, char* transError)
{

	DWORD	error;
	char	searchKey[MAX_REGKEY_PATH_LEN+1];
	HKEY	keyHandle;
	BYTE	keyValueBuf[256];
	DWORD	keyValueLength;
	DWORD	keyValueType;
	long	tempValue;
	short	i = 0;
	SQLRETURN retcode = ERROR_SUCCESS;
	int translen;
	wchar_t	tmpWBuf[MAX_SQL_IDENTIFIER_LEN + 1];

	strcpyUTF8(m_DSName, (const char *)DSName, sizeof(m_DSName));

	for (i = 0; i < 2 ; i++)
	{
		strcpy(searchKey, ODBC_DS_KEY);
		if ( i == 0)
			strcat(searchKey, (const char *)DSName);
		else
			strcat(searchKey, "DEFAULT");

		error = RegOpenKeyEx(HKEY_CURRENT_USER
						   ,searchKey
						   ,0
						   ,KEY_READ
						   ,&keyHandle);
		if (error == ERROR_SUCCESS)
		{
			m_DSLocation = HKCU_DSN; // HKCU
			// Read AssociationService	Value
			tmpWBuf[0] = L'\0';
			keyValueLength = sizeof(tmpWBuf);
			error = RegQueryValueExW (keyHandle,
							   L"Server",
							   NULL,				// Reserved
							   &keyValueType,
							   (LPBYTE)tmpWBuf,
							   &keyValueLength);
			if (error != ERROR_SUCCESS)
			{
				error = RegQueryValueExW (keyHandle,
							   L"AssociationService",
							   NULL,				// Reserved
							   &keyValueType,
							   (LPBYTE)tmpWBuf,
							   &keyValueLength);
				if (error != ERROR_SUCCESS)
				{
					RegCloseKey( keyHandle);
					if (retcode == ERROR_SUCCESS)
						retcode = DS_AS_KEY_NOT_FOUND;
				}
				else
				{
					retcode = ERROR_SUCCESS;
				}
			}
			else
			{
				retcode = ERROR_SUCCESS;
			}
			if (error == ERROR_SUCCESS)
			{
				// translate m_DSServer to UTF8
				transError[0] = '\0';
				if(wcslen((wchar_t*)tmpWBuf) > 0)
				{
					if (WCharToUTF8((wchar_t*) tmpWBuf, wcslen((wchar_t*)tmpWBuf), m_DSServer, sizeof(m_DSServer), &translen, transError)!= SQL_SUCCESS)
					{
						strcat(transError," :DataSource Server Name ");
						return  DS_TRANSLATION_ERROR;
					}
				}
				break;
			}
		}
		else if (retcode == ERROR_SUCCESS)
			retcode = DS_NOT_FOUND;

		error = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					   searchKey,
					   0,
					   KEY_READ,
					   &keyHandle);
		if (error == ERROR_SUCCESS)
		{
			m_DSLocation = HKLM_DSN; //HKLM
			// Read AssociationService	Value
			tmpWBuf[0] = L'\0';
			keyValueLength = sizeof(tmpWBuf);;
			error = RegQueryValueExW (keyHandle,
							   L"Server",
							   NULL,				// Reserved
							   &keyValueType,
							   (LPBYTE)tmpWBuf,
							   &keyValueLength);
			if (error != ERROR_SUCCESS)
			{
				error = RegQueryValueExW (keyHandle,
							   L"AssociationService",
							   NULL,				// Reserved
							   &keyValueType,
							   (LPBYTE)tmpWBuf,
							   &keyValueLength);
				if (error != ERROR_SUCCESS)
				{
					RegCloseKey( keyHandle);
					if (retcode == ERROR_SUCCESS)
						retcode = DS_AS_KEY_NOT_FOUND;
				}
				else
				{
					retcode = ERROR_SUCCESS;
				}
			}
			else
			{
				retcode = ERROR_SUCCESS;
			}
			if (error == ERROR_SUCCESS)
			{
				// translate m_DSServer to UTF8
				transError[0] = '\0';
				if(wcslen((wchar_t*)tmpWBuf) > 0)
				{
					if (WCharToUTF8((wchar_t*) tmpWBuf, wcslen((wchar_t*)tmpWBuf), m_DSServer, sizeof(m_DSServer), &translen, transError)!= SQL_SUCCESS)
					{
						strcat(transError," :DataSource Server Name ");
						return  DS_TRANSLATION_ERROR;
					}
				}
				break;
			}
		}
		else if (retcode == ERROR_SUCCESS)
			retcode = DS_NOT_FOUND;
	}

	if (retcode != ERROR_SUCCESS)
		return retcode;

	//Read the Catalog Value in unicode
	tmpWBuf[0] = L'\0';
	keyValueLength = sizeof(tmpWBuf);
	error = RegQueryValueExW (keyHandle,
	                   L"Catalog",
					   NULL,				// Reserved
					   &keyValueType,
					   (LPBYTE)tmpWBuf,
					   &keyValueLength);
	if (error != ERROR_SUCCESS)
		m_DSCatalog[0] = '\0';
	transError[0] = '\0';
	//Convert Catalog Value to UTF-8
	if(wcslen((wchar_t*)tmpWBuf) > 0)
	{
		if (WCharToUTF8((wchar_t*) tmpWBuf, wcslen((wchar_t*)tmpWBuf), m_DSCatalog, sizeof(m_DSCatalog), &translen, transError)!= SQL_SUCCESS)
		{
			strcat(transError," :DataSource Catalog Name ");
			return  DS_TRANSLATION_ERROR;
		}
	}

	// Read CtrlInferNCHAR Value
	keyValueLength = sizeof(keyValueBuf);
	error = RegQueryValueEx (keyHandle,
	                   "CtrlInferNCHAR",
					   NULL,				// Reserved
					   &keyValueType,
					   keyValueBuf,
					   &keyValueLength);
	if (error == ERROR_SUCCESS)
	{
		tempValue = atol((const char *)keyValueBuf);
		if (tempValue)
			m_DSCtrlInferNCHAR = TRUE;
		else
			m_DSCtrlInferNCHAR = FALSE;
	}
	else
		m_DSCtrlInferNCHAR = TRUE;


	// Read DataLang Value
	keyValueLength = sizeof(keyValueBuf);
	error = RegQueryValueEx (keyHandle,
	                   "DataLang",
					   NULL,				// Reserved
					   &keyValueType,
					   keyValueBuf,
					   &keyValueLength);
	if (error == ERROR_SUCCESS)
	{
		m_DSCharSet = atol((const char *)keyValueBuf);
		m_DSDataLang = m_DSCharSet;
		ODBCNLS_ValidateLanguage(&m_DSDataLang);
	}
	else	
		m_DSDataLang = MAKELCID (MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), SORT_DEFAULT);

	// Read ErrorMsgLang Value
	keyValueLength = sizeof(keyValueBuf);
	error = RegQueryValueEx (keyHandle,
	                   "ErrorMsgLang",
					   NULL,				// Reserved
					   &keyValueType,
					   keyValueBuf,
					   &keyValueLength);
	if (error == ERROR_SUCCESS)
	{
		m_DSErrorMsgLang = atol((const char *)keyValueBuf);
		ODBCNLS_ValidateLanguage(&m_DSErrorMsgLang);
	}
	else	
		m_DSErrorMsgLang = MAKELCID (MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), SORT_DEFAULT);

	// Read FetchBufferSize
	keyValueLength = sizeof(keyValueBuf);
	error = RegQueryValueEx ( keyHandle
	                   ,"FetchBufferSize"
					   ,NULL				// Reserved
					   ,&keyValueType
					   ,keyValueBuf
					   ,&keyValueLength);
	if (error == ERROR_SUCCESS)
	{	
		if (strcmp((const char *)keyValueBuf, SYSTEM_DEFAULT) == 0)
			m_DSFetchBufferSize = FETCH_BUFFER_SIZE_DEFAULT;
		else
			m_DSFetchBufferSize = atol((const char *)keyValueBuf) * 1024;
	}
	else
		m_DSFetchBufferSize = FETCH_BUFFER_SIZE_DEFAULT;

	//Read the Schema Value in unicode
	tmpWBuf[0] = L'\0';
	keyValueLength = sizeof(tmpWBuf);
	error = RegQueryValueExW (keyHandle,
	                   L"Schema",
					   NULL,				// Reserved
					   &keyValueType,
					   (LPBYTE)tmpWBuf,
					   &keyValueLength);
	if (error != ERROR_SUCCESS)
		m_DSSchema[0] = '\0';
	transError[0] = '\0';
	//Convert Schema Value to UTF-8
	if(wcslen((wchar_t*)tmpWBuf) > 0)
	{
		if (WCharToUTF8((wchar_t*) tmpWBuf, wcslen((wchar_t*)tmpWBuf), m_DSSchema, sizeof(m_DSSchema), &translen, transError) != SQL_SUCCESS)
		{
			strcat(transError, " :DataSource Schema Name ");
			return  DS_TRANSLATION_ERROR;
		}
	}
	//customer specific change
	if(m_DSSchema[0] == '\0')
		gDrvrGlobal.noSchemaInDSN = true;

	// Read SQL_ATTR_CONNECTION_TIMEOUT value
	keyValueLength = sizeof(keyValueBuf);
	error = RegQueryValueEx (keyHandle,
	                   "SQL_ATTR_CONNECTION_TIMEOUT",
					   NULL,				// Reserved
					   &keyValueType,
					   keyValueBuf,
					   &keyValueLength);
	if (error == ERROR_SUCCESS)
	{	
		if (strcmp((const char *)keyValueBuf, SYSTEM_DEFAULT) == 0)
			m_DSConnectionTimeout = CONNECTION_TIMEOUT_DEFAULT;
		else
		if (strcmp((const char *)keyValueBuf, NO_TIMEOUT) == 0)
			m_DSConnectionTimeout = 0;
		else
			m_DSConnectionTimeout = atol((const char *)keyValueBuf);
	}
	else
		m_DSConnectionTimeout = CONNECTION_TIMEOUT_DEFAULT;
		
	// Read SQL_LOGIN_TIMEOUT value
	keyValueLength = sizeof(keyValueBuf);
	error = RegQueryValueEx (keyHandle,
	                   "SQL_LOGIN_TIMEOUT",
					   NULL,				// Reserved
					   &keyValueType,
					   keyValueBuf,
					   &keyValueLength);
	if (error == ERROR_SUCCESS)
	{	
		if (strcmp((const char *)keyValueBuf, SYSTEM_DEFAULT) == 0)
			m_DSLoginTimeout = LOGIN_TIMEOUT_DEFAULT;
		else
		if (strcmp((const char *)keyValueBuf, NO_TIMEOUT) == 0)
			m_DSLoginTimeout = 0;
		else
			m_DSLoginTimeout = atol((const char *)keyValueBuf);
	}
	else
		m_DSLoginTimeout = LOGIN_TIMEOUT_DEFAULT;
	
	// Read SQL_QURRY_TIMEOUT value
	keyValueLength = sizeof(keyValueBuf);
	error = RegQueryValueEx (keyHandle,
	                   "SQL_QUERY_TIMEOUT",
					   NULL,				// Reserved
					   &keyValueType,
					   keyValueBuf,
					   &keyValueLength);
	if (error == ERROR_SUCCESS)
	{	
		if (strcmp((const char *)keyValueBuf, SYSTEM_DEFAULT) == 0)
			m_DSQueryTimeout = QUERY_TIMEOUT_DEFAULT;
		else
		if (strcmp((const char *)keyValueBuf, NO_TIMEOUT) == 0)
			m_DSQueryTimeout = 0;
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
	error = RegQueryValueEx (keyHandle,
	                   "TranslationDLL",
					   NULL,				// Reserved
					   &keyValueType,
					   (LPBYTE)m_DSTranslationDLL,
					   &keyValueLength);
	if (error != ERROR_SUCCESS)
		m_DSTranslationDLL[0] = '\0';

	// Read TranslationOption Value
	keyValueLength = sizeof(keyValueBuf);
	error = RegQueryValueEx (keyHandle,
	                   "TranslationOption",
					   NULL,				// Reserved
					   &keyValueType,
					   keyValueBuf,
					   &keyValueLength);
	if (error == ERROR_SUCCESS)
		m_DSTranslationOption = atol((const char *)keyValueBuf);
	else
		m_DSTranslationOption = 0;

	// Read ReplacementChar Value
	keyValueLength = sizeof(m_DSReplacementChar);
	error = RegQueryValueEx (keyHandle,
	                   "ReplacementCharacter",
					   NULL,				// Reserved
					   &keyValueType,
					   (LPBYTE)m_DSReplacementChar,
					   &keyValueLength);
	if (error != ERROR_SUCCESS)
		m_DSReplacementChar[0] = '\0';

// Read select rowsets value
// It is internal parameter not exposed through the administrator, where we define the behavior for rowset or non-rowset

	keyValueLength = sizeof(keyValueBuf);
	error = RegQueryValueEx (keyHandle,
	                   "SelectRowsets",
					   NULL,				// Reserved
					   &keyValueType,
					   keyValueBuf,
					   &keyValueLength);

	if (error == ERROR_SUCCESS)
	{
		tempValue = atol((const char *)keyValueBuf);
		if (tempValue)
			m_DSSelectRowsets = TRUE;
		else
			m_DSSelectRowsets = FALSE;
	}
	else
		m_DSSelectRowsets = TRUE;

// Compression 

	keyValueLength = sizeof(keyValueBuf);
	error = RegQueryValueEx (keyHandle,
	                   "Compression",
					   NULL,				// Reserved
					   &keyValueType,
					   keyValueBuf,
					   &keyValueLength);

	if (error == ERROR_SUCCESS){
		if(_stricmp((const char *)keyValueBuf,SYSTEM_DEFAULT)==0){
				m_DSIOCompression = 0;
		}
		else if(_stricmp((const char *)keyValueBuf,"no compression")==0){
			m_DSIOCompression = COMP_NO_COMPRESSION;
		}
		else if(_stricmp((const char *)keyValueBuf,"best speed")==0){
			m_DSIOCompression = COMP_BEST_SPEED;
		}
		else if(_stricmp((const char *)keyValueBuf,"best compression")==0){
			m_DSIOCompression = COMP_BEST_COMPRESSION;
		}
		else if(_stricmp((const char *)keyValueBuf,"balance")==0){
			m_DSIOCompression = COMP_DEFAULT;
		}
		else{
			m_DSIOCompression = atol((const char *)keyValueBuf);
		}
	}
	else
		m_DSIOCompression = 0;

	//get threshold of compression ,default is 1000
	keyValueLength = sizeof(keyValueBuf);
	error = RegQueryValueEx(keyHandle,
		"CompressionThreshold",
		NULL,				// Reserved
		&keyValueType,
		keyValueBuf,
		&keyValueLength);

	if (error == ERROR_SUCCESS)
	{
		if (strcmp((const char *)keyValueBuf, SYSTEM_DEFAULT) == 0)
			m_DSIOCompressionThreshold = IOCOMPRESSION_DEFAULT;
		else
		{
			m_DSIOCompressionThreshold = atol((const char *)keyValueBuf);
		}

	}
	else
		m_DSIOCompressionThreshold = IOCOMPRESSION_DEFAULT;

	keyValueLength = sizeof(keyValueBuf);
	error = RegQueryValueEx(keyHandle,
						"RowsetErrorRecovery",
						NULL,
						&keyValueType,
						keyValueBuf,
						&keyValueLength);

	if (error == ERROR_SUCCESS)
	{
		if (atol((const char *)keyValueBuf))
			m_DSRowsetErrorRecovery = TRUE;
		else
			m_DSRowsetErrorRecovery = FALSE;
	}
	else
		m_DSRowsetErrorRecovery = TRUE;

	//Read the ServerDSN Value in unicode
	tmpWBuf[0] = L'\0';
	keyValueLength = sizeof(tmpWBuf);
	error = RegQueryValueExW (keyHandle,
	                   L"ServerDSN",
					   NULL,				// Reserved
					   &keyValueType,
					   (LPBYTE)tmpWBuf,
					   &keyValueLength);
	if (error != ERROR_SUCCESS)
		m_DSServerDSName[0] = '\0';
	transError[0] = '\0';
	//Convert ServerDSN Value to UTF-8
	if(wcslen((wchar_t*)tmpWBuf) > 0)
	{
		if (WCharToUTF8((wchar_t*) tmpWBuf, wcslen((wchar_t*)tmpWBuf), m_DSServerDSName, sizeof(m_DSServerDSName), &translen, transError) != SQL_SUCCESS)
		{
			strcat(transError, " :DataSource ServerDSName ");
			return  DS_TRANSLATION_ERROR;
		}
	}

	keyValueLength = sizeof(keyValueBuf);
	error = RegQueryValueEx(keyHandle,
						"MapErrors",
						NULL,
						&keyValueType,
						keyValueBuf,
						&keyValueLength);

	if (error == ERROR_SUCCESS)
	{
		if (atol((const char *)keyValueBuf))
			GTransport.bMapErrors = TRUE;
		else
			GTransport.bMapErrors = FALSE;
	}
	else
		GTransport.bMapErrors = TRUE;

// FlushFetchData Y/N (default is N)
// It is internal parameter not exposed through the administrator,
// where we define the behavior for ATC query driver which cannot handle huge data being returned.

	keyValueLength = sizeof(keyValueBuf);
	error = RegQueryValueEx (keyHandle,
	                   "FlushFetchData",
					   NULL,				// Reserved
					   &keyValueType,
					   keyValueBuf,
					   &keyValueLength);

	if (error == ERROR_SUCCESS)
	{
		if (keyValueBuf[0] == 'N')
			m_DSFlushFetchData = FALSE;
		else
			m_DSFlushFetchData = TRUE;
	}
	else
		m_DSFlushFetchData = FALSE;

	//Read the ServiceName Value in unicode
	tmpWBuf[0] = L'\0';
	keyValueLength = sizeof(tmpWBuf);
	error = RegQueryValueExW (keyHandle,
	                   L"ServiceName",
					   NULL,				// Reserved
					   &keyValueType,
					   (LPBYTE)tmpWBuf,
					   &keyValueLength);
	if (error != ERROR_SUCCESS)
		m_DSServiceName[0] = '\0';
	//Convert ServiceName Value to UTF-8
	if(wcslen((wchar_t*)tmpWBuf) > 0)
	{
		if (WCharToUTF8((wchar_t*) tmpWBuf, wcslen((wchar_t*)tmpWBuf), m_DSServiceName, sizeof(m_DSServiceName), &translen, transError) != SQL_SUCCESS)
		{
			strcat(transError, " :DataSource ServiceName ");
			return  DS_TRANSLATION_ERROR;
		}
	}

	RegCloseKey( keyHandle);
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
			strcpyUTF8(AttrValue, keywordTree[i].AttrValue, sizeof(AttrValue), keywordTree[i].AttrLength);
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
				m_DSCharSet = atol(keywordTree[KEY_DATALANG].AttrValue);
				m_DSDataLang = m_DSCharSet;
				ODBCNLS_ValidateLanguage(&m_DSDataLang);
				break;
			case KEY_ERRORMSGLANG:
				m_DSErrorMsgLang = atol(AttrValue);
				ODBCNLS_ValidateLanguage(&m_DSErrorMsgLang);
				break;
			case KEY_FETCHBUFFERSIZE:
				if (_stricmp(AttrValue, SYSTEM_DEFAULT) == 0)
					m_DSFetchBufferSize = FETCH_BUFFER_SIZE_DEFAULT;
				else
					m_DSFetchBufferSize = atol(AttrValue);
				break;
			case KEY_SQL_ATTR_CONNECTION_TIMEOUT:
				if (_stricmp(AttrValue, SYSTEM_DEFAULT) == 0)
					m_DSConnectionTimeout = CONNECTION_TIMEOUT_DEFAULT;
				else
					m_DSConnectionTimeout = atol(AttrValue);
				break;
			case KEY_SQL_LOGIN_TIMEOUT:
				if (_stricmp(AttrValue, SYSTEM_DEFAULT) == 0)
					m_DSLoginTimeout = LOGIN_TIMEOUT_DEFAULT;
				else
					m_DSLoginTimeout = atol(AttrValue);
				break;
			case KEY_SQL_QUERY_TIMEOUT:
				if (_stricmp(AttrValue, SYSTEM_DEFAULT) == 0)
					m_DSQueryTimeout = QUERY_TIMEOUT_DEFAULT;
				else
					m_DSQueryTimeout = atol(AttrValue);
				break;
			case KEY_TRANSLATIONDLL:
				strcpyUTF8(m_DSTranslationDLL, AttrValue, sizeof(m_DSTranslationDLL));
				break;
			case KEY_TRANSLATIONOPTION:
				m_DSTranslationOption = atol(AttrValue);
				break;
			case KEY_REPLACEMENTCHAR:
				strcpyUTF8(m_DSReplacementChar, AttrValue, sizeof(m_DSReplacementChar));
				break;
			case KEY_SDSN:
				strcpyUTF8(m_DSServerDSName, AttrValue, sizeof(m_DSServerDSName));
				break;
			case KEY_SN:
				strcpyUTF8(m_DSServiceName, AttrValue, sizeof(m_DSServiceName));
				break;
			case KEY_SESSION:
				strcpyUTF8(m_DSSession, AttrValue, sizeof(m_DSSession));
				break;
			case KEY_APPLICATION:
				strcpyUTF8(m_DSApplication, AttrValue, sizeof(m_DSApplication));
				break;
			case KEY_ROLENAME:
				strcpyUTF8(m_DSRoleName, AttrValue, sizeof(m_DSRoleName));
				break;
			case KEY_CERTIFICATEDIR:
				strcpyUTF8(m_DSCertificateDir, AttrValue, sizeof(m_DSCertificateDir));
				break;
			case KEY_CERTIFICATEFILE:
				strcpyUTF8(m_DSCertificateFile, AttrValue, sizeof(m_DSCertificateFile));
				break;
			case KEY_CERTIFICATEFILE_ACTIVE:
				strcpyUTF8(m_DSCertificateFileActive, AttrValue, sizeof(m_DSCertificateFileActive));
				break;
			case KEY_COMPRESSION:
				if(_stricmp(AttrValue,SYSTEM_DEFAULT)==0){
					m_DSIOCompression = 0;
				}
				else if(_stricmp(AttrValue,"no compression")==0){
					m_DSIOCompression = COMP_NO_COMPRESSION;
				}
				else if(_stricmp(AttrValue,"best speed")==0){
					m_DSIOCompression = COMP_BEST_SPEED;
				}
				else if(_stricmp(AttrValue,"best compression")==0){
					m_DSIOCompression = COMP_BEST_COMPRESSION;
				}
				else if(_stricmp(AttrValue,"balance")==0){
					m_DSIOCompression = COMP_DEFAULT;
				}
				else{
					m_DSIOCompression = atol(AttrValue);
				}
				break;
			case KEY_COMPRESSIONTHRESHOLD:
				if (stricmp(AttrValue, SYSTEM_DEFAULT) == 0){
					  m_DSIOCompressionThreshold = IOCOMPRESSION_DEFAULT;
				}
				else
				{
				         m_DSIOCompressionThreshold = atol((const char *)AttrValue);
				}
				break;
			default:
				break;
			}
		}
	}
}
