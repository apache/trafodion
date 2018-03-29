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

#include "DrvrGlobal.h"
#include "charsetconv.h"

#define DS_NOT_FOUND			1
#define DS_AS_KEY_NOT_FOUND		2
#define	DS_TRANSLATION_ERROR	3


class CConnect;

class CDataSource {

public:
	CDataSource();
	short readDSValues(char *DSName, char* TransError);
	void CDataSource::updateDSValues(short DSNType, CONNECT_FIELD_ITEMS *connectFieldItems,
								 CONNECT_KEYWORD_TREE *keywordTree);
    void setDSCharSet(DWORD charset){ m_DSCharSet = charset; };
private:
	short		m_DSLocation;						
	char		m_DSName[MAX_SQL_IDENTIFIER_LEN + 1];
	char		m_DSServer[MAX_SQL_IDENTIFIER_LEN + 1];
	char		m_DSCatalog[MAX_SQL_IDENTIFIER_LEN + 1];
	BOOL		m_DSCtrlInferNCHAR;
	DWORD		m_DSDataLang;
	DWORD		m_DSCharSet;
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
	SQLUINTEGER	m_DSIOCompression;
	int             m_DSIOCompressionThreshold;
	BOOL		m_DSRowsetErrorRecovery;
	char		m_DSServerDSName[MAX_SQL_IDENTIFIER_LEN + 1];
	char		m_DSServiceName[SQL_MAX_SERVICENAME_LEN + 1];
	char		m_DSSession[SQL_MAX_SESSIONNAME_LEN*4+1];
	char		m_DSApplication[SQL_MAX_APPLNAME_LEN*4+1];
	char		m_DSRoleName[SQL_MAX_ROLENAME_LEN+1];
	char		m_DSCertificateDir[MAX_SQL_IDENTIFIER_LEN + 1];
	char		m_DSCertificateFile[MAX_SQL_IDENTIFIER_LEN + 1];
	char		m_DSCertificateFileActive[MAX_SQL_IDENTIFIER_LEN + 1];
	BOOL		m_DSFlushFetchData;
	friend class CConnect;
};

#endif
