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
/**************************************************************************
**************************************************************************/
//
//
#ifndef CDATASOURCE_H
#define CDATASOURCE_H

#include "drvrglobal.h"

#define DS_NOT_FOUND					1
#define DS_AS_KEY_NOT_FOUND				2
#define DS_INIT_FILE_NOT_FOUND			3
#define DS_AS_PROCESS_NAME_INCORRECT	4
#define DS_FILE_SYSTEM_ERROR			5
#define DS_INTERNAL_ERROR			6

#define DS_MAX_PATH_SIZE	2048

class CConnect;

class CDataSource {

public:
	CDataSource();
	short getAnyProfileString(
	    LPCTSTR lpAppName,        // section name
	    LPCTSTR lpKeyName,        // key name
	    LPCTSTR lpDefault,        // default string
	    LPTSTR lpReturnedString,  // destination buffer
	    DWORD nSize,              // size of destination buffer
	    short* lpDSLocation,      // out DS location
	    DWORD nSizePath,	      // size of path buffer
	    LPTSTR lpReturnedPath     // out DSN Path
	    );
	short readDSValues(char *DSName,CConnect* pConnection);
	void updateDSValues(short DSNType, CONNECT_FIELD_ITEMS *connectFieldItems,
								 CONNECT_KEYWORD_TREE *keywordTree);
private:
	short		m_DSLocation;						
	char		m_DSName[128+1];
	char		m_DSServer[128];
	char		m_DSCatalog[MAX_SQL_IDENTIFIER_LEN + 1];
	BOOL		m_DSCtrlInferNCHAR;
	DWORD		m_DSDataLang;
	char		m_DSCharSet[32];
	DWORD		m_DSErrorMsgLang;
	long		m_DSFetchBufferSize;					// in Bytes
	char		m_DSSchema[MAX_SQL_IDENTIFIER_LEN + 1];
	SQLUINTEGER m_DSConnectionTimeout;
	SQLUINTEGER	m_DSLoginTimeout;
	SQLUINTEGER m_DSQueryTimeout;
	char		m_DSTranslationDLL[128];
	DWORD		m_DSTranslationOption;
	char		m_DSReplacementChar[5];
	BOOL		m_DSSelectRowsets;
	char		m_DSTcpProcessName[50];
	BOOL		m_DSRowsetErrorRecovery;
	char		m_DSServerDSName[129];
	char		m_DSServiceName[50];
	BOOL		m_DSFlushFetchData;
	BOOL		m_DS_UCS2Translation;
	char		m_DSSession[SQL_MAX_SESSIONNAME_LEN];
	char		m_DSApplication[SQL_MAX_APPLNAME_LEN];
	char		m_DSRoleName[SQL_MAX_ROLENAME_LEN+1];
	char		m_DSCertificateDir[MAX_SQL_IDENTIFIER_LEN + 1];
	char		m_DSCertificateFile[MAX_SQL_IDENTIFIER_LEN + 1];
	char		m_DSCertificateFileActive[MAX_SQL_IDENTIFIER_LEN + 1];
	SQLUINTEGER	m_DSIOCompression;
       int         m_DSIOCompressionThreshold;
	friend class CConnect;
};

#endif
