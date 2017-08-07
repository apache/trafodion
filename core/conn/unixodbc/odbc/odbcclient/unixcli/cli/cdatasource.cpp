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

// Implements the member functions of CDataSource

#define MAX_REGKEY_PATH_LEN			512
#define SYSTEM_DEFAULT				"SYSTEM_DEFAULT"
#define NO_TIMEOUT					"NO_TIMEOUT"
#define CONNECTION_TIMEOUT_DEFAULT	60
#define LOGIN_TIMEOUT_DEFAULT		60
#define	QUERY_TIMEOUT_DEFAULT		0
#define IOCOMPRESSION_DEFAULT       1000
#define FETCH_BUFFER_SIZE_DEFAULT	512 * 1024
#define TCP_DEFAULT_PROCESS			"$ZTC0"
// we are using the hpodbc driver manager
extern int hpodbc_dmanager = 1;

// Daniel - DFA state transfer table. no compressed.
static char state_table[14][5]={
    {2,3,3,5,0},
    {'N','N','N','N',1},
    {4,5,5,5,0},
    {'N','N','N','N',1},
    {6,7,7,9,0},
    {'N','N','N','N',1},
    {8,9,9,9,0},
    {'N','N','N','N',1},
    {10,11,11,13,0},
    {'N','N','N','N',1},
    {12,13,13,13,0},
    {'N','N','N','N',1},
    {'N','N','N','N',1},
    {14,14,14,14,1}
};

CDataSource::CDataSource()
{
	char	keyValueBuf[256];
	DWORD	keyValueLength;

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
    m_DSIOCompressionThreshold = 0;
    //
    // Read the ODBC section to get Certificate Directory Location
    //
    char	path[DS_MAX_PATH_SIZE];

    DWORD	len;
    char	searchKey[MAX_REGKEY_PATH_LEN+1];
    CHAR*	szODBC = "ODBC";
    short retprofile;
    strncpy(searchKey, (const char *)szODBC, sizeof(searchKey));
    searchKey[sizeof(searchKey)-1]='\0';

    retprofile=getAnyProfileString((LPCTSTR)searchKey,
				   "CertificateDir",
				   "SYSTEM_DEFAULT",
				   m_DSCertificateDir,
				   sizeof(m_DSCertificateDir),
				   &m_DSLocation,
				   DS_MAX_PATH_SIZE,
				   path	);

    //
    // Read the ODBC section to get Compression Type
    //

    keyValueLength = sizeof(keyValueBuf);
    len = GetMyPrivateProfileString(
	searchKey,
	"Compression",
	"SYSTEM_DEFAULT",
	keyValueBuf,
	keyValueLength,
	path);

    if (len > 0)
    {	
		if(strcasecmp((const char *)keyValueBuf,SYSTEM_DEFAULT)==0){
				m_DSIOCompression = 0;
		}
		else if(strcasecmp((const char *)keyValueBuf,"no compression")==0){
			m_DSIOCompression = COMP_NO_COMPRESSION;
		}
		else if(strcasecmp((const char *)keyValueBuf,"best speed")==0){
			m_DSIOCompression = COMP_BEST_SPEED;
		}
		else if(strcasecmp((const char *)keyValueBuf,"best compression")==0){
			m_DSIOCompression = COMP_BEST_COMPRESSION;
		}
		else if(strcasecmp((const char *)keyValueBuf,"balance")==0){
			m_DSIOCompression = COMP_DEFAULT;
		}
		else{
			m_DSIOCompression = atol((const char *)keyValueBuf);
		}
    }
    else
	m_DSIOCompression = 0;


}
// Daniel - read key from FILE_DSN > USER_DSN > SYSTEM_DSN, also considering default section in each FILE. it might be very complex, Using DFA to simply it.
short CDataSource::getAnyProfileString(
    LPCTSTR lpAppName,        // section name
    LPCTSTR lpKeyName,        // key name
    LPCTSTR lpDefault,        // default string
    LPTSTR lpReturnedString,  // out destination buffer
    DWORD nSize,              // size of destination buffer
    short* lpDSLocation,      // out DS location
    DWORD nSizePath,	      // size of path buffer
    LPTSTR lpReturnedPath)    // out DSN Path

{
    
    char	path[DS_MAX_PATH_SIZE];
    path[0]='\0';
    
    DWORD	len;
    char	searchKey[MAX_REGKEY_PATH_LEN+1];
    short       ret;

    char cur_state=1; // initial state
    char index=0;
    char *p=NULL;
    int n;
    while(!state_table[cur_state-1][4])
    {
	// action 
	switch(cur_state)
	{
	case 1:
	    strncpy(searchKey, (const char *)lpAppName, sizeof(searchKey));
	    searchKey[sizeof(searchKey)-1] = 0;
	    if((p=getEnvFilePath(ENV_DSNVAR))!=NULL)
		strcpy(path,p);
	    else
		path[0]='\0';
	    break;
	case 3:
	case 7:
	case 11:
	    strcpy(searchKey,"DEFAULT");
	    break;
	case 5:
	    // transition 3 -> 5 need re-copy searchKey.
	    strncpy(searchKey, (const char *)lpAppName, sizeof(searchKey));
	    searchKey[sizeof(searchKey)-1] = '\0';
	    if((p=getUserFilePath(USER_DSNFILE))!=NULL)
		strcpy(path,p);
	    else
		path[0]='\0';
	    break;
	case 9:
	    // transition 7 -> 9 need re-copy searchKey.
	    strncpy(searchKey, (const char *)lpAppName, sizeof(searchKey));
	    searchKey[sizeof(searchKey)-1] = '\0';
	    strcpy(path,SYSTEM_DSNFILE);
	    break;
	default: // DFA error , return
	    cur_state=14;
	    continue;
	}

	    
	len = GetMyPrivateProfileString(
	    searchKey,
	    lpKeyName,
	    lpDefault,
	    lpReturnedString,
	    nSize,
	    path);
	if(len>0){
	    index=1;
	}
	else if(len==0){
	    index=2;
	}
	else if(len==-1){
	    index=3;
	}
	else if(len==-2){
	    index=4;
	}
	else{
	    index=1;
	    cur_state=14;
	}
	cur_state=state_table[cur_state-1][index-1];
    };
    switch(cur_state)
    {
    case 2:
    case 4:
	*lpDSLocation=FILE_DSN;
	ret=1;
	break;
    case 6:
    case 8:
	*lpDSLocation=HKCU_DSN;
	ret=1;
	break;
    case 10:
    case 12:  // successfully get key value.
	*lpDSLocation=HKLM_DSN;
	ret=1;
	break;
    case 13: // can't get key from any DSN file
	*lpDSLocation=UNKNOWN_DSN;
	if(lpDefault)
	    ret=0; // get default value
	else
	    ret=-1; // not get value
	break;
    default: // DFA error
	ret=-2;
	break;
    }
    strncpy(lpReturnedPath,path,nSizePath);
    lpReturnedPath[nSizePath-1]='\0';
    return ret;
}

short CDataSource::readDSValues(char *DSName,CConnect* pConnection)
{
//	char	path[EXT_FILENAME_LEN+1];
	char	path[DS_MAX_PATH_SIZE];

	DWORD	len;
	char	searchKey[MAX_REGKEY_PATH_LEN+1];
//	HKEY	keyHandle;
	char	keyValueBuf[256];
	DWORD	keyValueLength;
	DWORD	keyValueType;
	long	tempValue;
	short	retprofile = 0;
	DWORD	retcode = ERROR_SUCCESS;
	strncpy(m_DSName, (const char *)DSName, sizeof(m_DSName));
	m_DSName[sizeof(m_DSName)-1] = 0;
	CHAR*	szODBC = "ODBC";
	char *pSlash=NULL;

	//
	//	Read the ODBC sections
	//

	strncpy(searchKey, (const char *)DSName, sizeof(searchKey));
	searchKey[sizeof(searchKey)-1] = 0;
	keyValueLength=128;
	retprofile=getAnyProfileString(searchKey,
				       "Server",
				       NULL,
				       m_DSServer,
				       keyValueLength,				       
				       &m_DSLocation,
				       DS_MAX_PATH_SIZE,
				       path);
	if(retprofile==1)
	{
	    retcode=ERROR_SUCCESS;
	}
	else if(retprofile==0||retprofile==-1)
	{
	    retprofile=getAnyProfileString(searchKey,
					   "AssociationService",
					   NULL,
					   m_DSServer,
					   keyValueLength,
					   &m_DSLocation,
					   DS_MAX_PATH_SIZE,
					   path);
	    if(retprofile==1)
	    {
		retcode=ERROR_SUCCESS;
	    }
	    else if(retprofile==0||retprofile==-1)
	    {
		retcode=DS_AS_KEY_NOT_FOUND;
	    }
	    else if(retprofile==-2)
	    {
		retcode=DS_INTERNAL_ERROR;
	    }

	}
	else if(retprofile==-2)
	{
	    retcode=DS_INTERNAL_ERROR;
	}
/* 
  
	for (i = 0; i < 2 ; i++)
	{
		if ( i == 0)
		{
			strncpy(searchKey, (const char *)DSName, sizeof(searchKey));
			searchKey[sizeof(searchKey)-1] = 0;
		}
		else
			strcpy(searchKey, "DEFAULT");

		strcpy(path,getUserFilePath(USER_DSNFILE));
		if (path[0]!='\0')
		{
			m_DSLocation = HKCU_DSN; // HKCU
		}
		else
		{
			m_DSLocation = HKLM_DSN; // HKLM
			strcpy(path, SYSTEM_DSNFILE);
		}

		// Read AssociationService	Value
		keyValueLength = sizeof(m_DSServer);
		len = GetMyPrivateProfileString(
							searchKey,
							"Server",
							NULL,
							m_DSServer,
							keyValueLength,
							path);

		if (len < 0 ) // File not found or other File error or DS not found
		{
			if (len == -2)							//open error
			{
				if (getODBCDsnError() != ENOENT)	//other than not found
					return DS_FILE_SYSTEM_ERROR;
				else
				{
					char buffer[256];
					sprintf(buffer," %s",getODBCDsnFileName());
					pConnection->setDiagRec(DRIVER_ERROR, IDS_IM_002, 0, buffer);
				}
			}
			if (m_DSLocation == HKCU_DSN)
			{
				m_DSLocation = HKLM_DSN; // HKLM
				strcpy(path, SYSTEM_DSNFILE);
				len = GetMyPrivateProfileString(
									searchKey,
									"Server",
									NULL,
									m_DSServer,
									keyValueLength,
									path);
			}
			if (len < 0 )
			{
				retcode = DS_NOT_FOUND;
				if (len == -2)							//open error
				{
					if (getODBCDsnError() != ENOENT)	//other than not found
						return DS_FILE_SYSTEM_ERROR;
					break;
				}
				else
					continue; // try DEFAULT data source
			}
		}
		if (len == 0) // section not found - try AssociationService
		{
			len = GetMyPrivateProfileString(
								searchKey,
								"AssociationService",
								NULL,
								m_DSServer,
								keyValueLength,
								path);

			if (len <= 0) // section not found
			{
				if (m_DSLocation == HKLM_DSN) // System DS
				{
					if (retcode == ERROR_SUCCESS)
						retcode = DS_AS_KEY_NOT_FOUND;
					continue;		// try DEFAULT data source
				}
				else
				{
					m_DSLocation = HKLM_DSN; // HKLM
					strcpy(path, SYSTEM_DSNFILE);
					len = GetMyPrivateProfileString(
										searchKey,
										"Server",
										NULL,
										m_DSServer,
										keyValueLength,
										path);
				}
				if (len < 0 ) // File not found or DS not found
				{
					retcode = DS_NOT_FOUND;
					if (len == -2) // File not found
						break;
					else
						continue; // DS not found - try DEFAULT
				}
				else if (len == 0) // section not found
				{
					len = GetMyPrivateProfileString(
										searchKey,
										"AssociationService",
										NULL,
										m_DSServer,
										keyValueLength,
										path);
					if (len <= 0)
					{
						if (retcode == ERROR_SUCCESS)
							retcode = DS_AS_KEY_NOT_FOUND;
					}
					else
					{
						retcode = ERROR_SUCCESS;
						break;
					}
				}
				else
				{
					retcode = ERROR_SUCCESS;
					break;
				}
			}
			else
			{
				retcode = ERROR_SUCCESS;
				break;
			}
		}
		else
		{
			retcode = ERROR_SUCCESS;
			break;
		}
	}
*/
	if (retcode != ERROR_SUCCESS)
		return retcode;
	if (strncmp(m_DSServer, "TCP:", 4) != 0 )
		return DS_AS_PROCESS_NAME_INCORRECT;
	if((pSlash=strchr(m_DSServer,'/'))!=NULL)
		*pSlash=':';

	// Read Catalog	Value
	keyValueLength = sizeof(m_DSCatalog);
	len = GetMyPrivateProfileString(
						searchKey,
						"Catalog",
						NULL,
						m_DSCatalog,
						keyValueLength,
						path);
	if (len <= 0)
		m_DSCatalog[0] = '\0';

	// Read CtrlInferNCHAR Value
	keyValueLength = sizeof(keyValueBuf);
	len = GetMyPrivateProfileString(
						searchKey,
						"CtrlInferNCHAR",
						NULL,
						keyValueBuf,
						keyValueLength,
						path);
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
	len = GetMyPrivateProfileString(
						searchKey,
						"FetchBufferSize",
						NULL,
						keyValueBuf,
						keyValueLength,
						path);
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
	len = GetMyPrivateProfileString(
						searchKey,
						"Schema",
						NULL,
						m_DSSchema,
						keyValueLength,
						path);
	if (len <= 0)
		m_DSSchema[0] = '\0';

	// Read SQL_ATTR_CONNECTION_TIMEOUT value
	keyValueLength = sizeof(keyValueBuf);
	len = GetMyPrivateProfileString(
						searchKey,
						"SQL_ATTR_CONNECTION_TIMEOUT",
						NULL,
						keyValueBuf,
						keyValueLength,
						path);
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
	len = GetMyPrivateProfileString(
						searchKey,
						"SQL_LOGIN_TIMEOUT",
						NULL,
						keyValueBuf,
						keyValueLength,
						path);
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
	len = GetMyPrivateProfileString(
						searchKey,
						"SQL_QUERY_TIMEOUT",
						NULL,
						keyValueBuf,
						keyValueLength,
						path);
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
	len = GetMyPrivateProfileString(
						searchKey,
						"TranslationDLL",
						NULL,
						m_DSTranslationDLL,
						keyValueLength,
						path);
	if (len <= 0)
		m_DSTranslationDLL[0] = '\0';
	if (stricmp(m_DSTranslationDLL,"null")==0 || stricmp(m_DSTranslationDLL,"0")==0)
		m_DSTranslationDLL[0] = '\0';

	// Read TranslationOption Value
	keyValueLength = sizeof(keyValueBuf);
	len = GetMyPrivateProfileString(
						searchKey,
						"TranslationOption",
						NULL,
						keyValueBuf,
						keyValueLength,
						path);
	if (len > 0)
		m_DSTranslationOption = atol((const char *)keyValueBuf);
	else
		m_DSTranslationOption = 0;

// Read select rowsets value
// It is internal parameters not exposed through the administrator, where we define the behavior for rowset or non-rowset

	keyValueLength = sizeof(keyValueBuf);
	len = GetMyPrivateProfileString(
						searchKey,
						"SelectRowsets",
						NULL,
						keyValueBuf,
						keyValueLength,
						path);

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
	len = GetMyPrivateProfileString(
						searchKey,
						"TcpProcessName",
						NULL,
						m_DSTcpProcessName,
						keyValueLength,
						path);
	if (len <= 0)
		strcpy(m_DSTcpProcessName,TCP_DEFAULT_PROCESS);
//
// Patch for Informatica to recover from insert rowset error
//
	keyValueLength = sizeof(keyValueBuf);
	len = GetMyPrivateProfileString(
						searchKey,
						"RowsetErrorRecovery",
						NULL,
						keyValueBuf,
						keyValueLength,
						path);

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
	len = GetMyPrivateProfileString(
						searchKey,
						"ServerDSN",
						NULL,				// Reserved
						m_DSServerDSName,
						keyValueLength,
						path);
	if (len <= 0)
		m_DSServerDSName[0] = '\0';

	keyValueLength = sizeof(keyValueBuf);
	len = GetMyPrivateProfileString(
						searchKey,
						"MapErrors",
						NULL,
						keyValueBuf,
						keyValueLength,
						path);

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
	// Certificate Directory location
	//
	len = GetMyPrivateProfileString(
						searchKey,
						"CertificateDir",
						"SYSTEM_DEFAULT",
						m_DSCertificateDir,
						sizeof(m_DSCertificateDir),
						path);

//
// Compression types to be used
//
	keyValueLength = sizeof(keyValueBuf);
	len = GetMyPrivateProfileString(
						searchKey,
						"Compression",
						NULL,
						keyValueBuf,
						keyValueLength,
						path);

	if (len > 0)
	{
		if(strcasecmp((const char *)keyValueBuf,SYSTEM_DEFAULT)==0){
				m_DSIOCompression = 0;
		}
		else if(strcasecmp((const char *)keyValueBuf,"no compression")==0){
			m_DSIOCompression = COMP_NO_COMPRESSION;
		}
		else if(strcasecmp((const char *)keyValueBuf,"best speed")==0){
			m_DSIOCompression = COMP_BEST_SPEED;
		}
		else if(strcasecmp((const char *)keyValueBuf,"best compression")==0){
			m_DSIOCompression = COMP_BEST_COMPRESSION;
		}
		else if(strcasecmp((const char *)keyValueBuf,"balance")==0){
			m_DSIOCompression = COMP_DEFAULT;
		}
		else{
			m_DSIOCompression = atol((const char *)keyValueBuf);
		}
	}

//get threshold of compression ,default is 1000
	keyValueLength = sizeof(keyValueBuf);
	len = GetMyPrivateProfileString(
						searchKey,
						"CompressionThreshold",
						NULL,
						keyValueBuf,
						keyValueLength,
						path);

	if (len > 0)
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

	keyValueLength = sizeof(m_DSServiceName);
	len = GetMyPrivateProfileString(
						searchKey,
						"ServiceName",
						NULL,
						m_DSServiceName,
						keyValueLength,
						path);

	if (len <= 0)
		m_DSServiceName[0] = '\0';

// Read ClientCharSet Value
	len = GetMyPrivateProfileString(
						searchKey,
						"ClientCharSet",
						"ISO-8859-1",
						m_DSCharSet,
						sizeof(m_DSCharSet),
						path);

// Read ReplacementCharacter Value
	keyValueLength = sizeof(m_DSReplacementChar);
	len = GetMyPrivateProfileString(
						searchKey,
						"ReplacementCharacter",
						NULL,
						m_DSReplacementChar,
						keyValueLength,
						path);
	if (len <= 0)
		m_DSReplacementChar[0] = '\0';

//UCS2Translation : An internal parameter not exposed through administrator.
//It is used for to insert/retrieve an application-translated UCS2 string.
//If an application needs no Translation UCS2Translation should be set to "0"
//This is valid only if server ISOMapping is ISO88591, otherwise ignored. 
//UCS2Translation is by default is ON (means either 
//no Ucs2Translation attribute specified in the datasource or it is present and 
//has a value of 1).
//Read UCS2Translation Value
	keyValueLength = sizeof(keyValueBuf);
	len = GetMyPrivateProfileString(
						searchKey,
						"UCS2Translation",
						NULL,
						keyValueBuf,
						keyValueLength,
						path);
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



	if(strcmp(m_DSCertificateDir,"SYSTEM_DEFAULT") == 0)
	{
		strncpy(searchKey, (const char *)szODBC, sizeof(searchKey));
		len = GetMyPrivateProfileString(
							searchKey,
							"CertificateDir",
							"SYSTEM_DEFAULT",
							m_DSCertificateDir,
							sizeof(m_DSCertificateDir),
							path);
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
				else if (stricmp(AttrValue, NO_TIMEOUT) == 0)
				    m_DSConnectionTimeout = 0; // no timeout
				else
					m_DSConnectionTimeout = atol(AttrValue);
				break;
			case KEY_SQL_LOGIN_TIMEOUT:
				if (stricmp(AttrValue, SYSTEM_DEFAULT) == 0)
					m_DSLoginTimeout = LOGIN_TIMEOUT_DEFAULT;
				else if (stricmp(AttrValue, NO_TIMEOUT) == 0)
				    m_DSLoginTimeout = 0; // no timeout
				else
					m_DSLoginTimeout = atol(AttrValue);
				break;
			case KEY_SQL_QUERY_TIMEOUT:
				if (stricmp(AttrValue, SYSTEM_DEFAULT) == 0)
					m_DSQueryTimeout = QUERY_TIMEOUT_DEFAULT;
				else if (stricmp(AttrValue, NO_TIMEOUT) == 0)
				    m_DSQueryTimeout = 0; // no timeout
				else
					m_DSQueryTimeout = atol(AttrValue);
				break;
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
			case KEY_COMPRESSION:
				if (stricmp(AttrValue, SYSTEM_DEFAULT) == 0){
					m_DSIOCompression = 0;
				}
				else if(strcasecmp(AttrValue,"no compression")==0){
					m_DSIOCompression = COMP_NO_COMPRESSION;
				}
				else if(strcasecmp(AttrValue,"best speed")==0){
					m_DSIOCompression = COMP_BEST_SPEED;
				}
				else if(strcasecmp(AttrValue,"best compression")==0){
					m_DSIOCompression = COMP_BEST_COMPRESSION;
				}
				else if(strcasecmp(AttrValue,"balance")==0){
					m_DSIOCompression = COMP_DEFAULT;
				}
				else{
			        m_DSIOCompression = atol((const char *)AttrValue);
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

