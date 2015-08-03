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

#ifndef CDIAG_H
#define CDIAG_H

#include <windows.h>
#include <sql.h>
#include <sqlext.h>

#ifdef VERSION3
	#include <string>
	#include <set>
#else
	#include <rw/cstring.h>
	#include <rw/tpdlist.h>
#endif

#include "odbccommon.h"
#include "odbc_cl.h"
#include "charsetconv.h"


#define MAX_DIAG_PARAMS		4
#define DIAG_EMPTY_STRING	""

typedef struct DIAG_FUNC_MAP
{
	char		*diagFuncName;
	SQLINTEGER	diagFuncCode;
} DIAG_FUNC_MAP;

class CDiagRec;

class CDiagStatus {

public:	
	CDiagStatus(short		diagComponentCode,
				DWORD		diagErrorCode,
				SQLINTEGER	diagNative,
				char		*diagMessageText,
				char		*diagSqlState,
				SQLLEN  	diagRowNumber,
				SQLINTEGER	diagColumnNumber,
				short		diagNoParams,
				va_list		diagParams);
#ifdef VERSION3
friend struct DiagRecCompareClass;
#endif
private:
	short		m_DiagComponentCode;
	DWORD		m_DiagErrorCode;
	SQLLEN  	m_DiagRowNumber;
	SQLINTEGER	m_DiagColumnNumber;
	SQLINTEGER	m_DiagNative;
	SQLCHAR		m_DiagSqlState[6];
	std::string	m_DiagMessageText;	
	char*	m_DiagParams[MAX_DIAG_PARAMS]; 	
	friend class CDiagRec;				
};

typedef CDiagStatus *CDiagStatusPtr;

#ifdef VERSION3
struct DiagRecCompareClass {
  bool operator() (const CDiagStatus* lhs, const CDiagStatus* rhs) const
  {
  //STEP 1: sort by row id
	if(lhs->m_DiagRowNumber<rhs->m_DiagRowNumber)
		return true;
	if(lhs->m_DiagRowNumber>rhs->m_DiagRowNumber)
		return false;
	
	//row number is the same, now check error code
	
	//STEP 2; sort by diagErrorCode, where transaction failures or possible transaction failures take precedence
	if(lhs->m_DiagErrorCode<rhs->m_DiagErrorCode)
		return true;
	if(lhs->m_DiagErrorCode>rhs->m_DiagErrorCode)
		return false;
	
	//diag error code is the same, now check sqlState
	//STEP 3: sort by sqlState; (classes 03 through HZ) outrank ODBC-defined and driver-defined SQLSTATEs
	if(lhs->m_DiagSqlState[0]>rhs->m_DiagSqlState[0])
		return true;
	if(lhs->m_DiagSqlState[0]<rhs->m_DiagSqlState[0])
		return false;
	if(lhs->m_DiagSqlState[1]>rhs->m_DiagSqlState[1])
		return true;
	if(lhs->m_DiagSqlState[1]<rhs->m_DiagSqlState[1])
		return false;
		
	//STEP 4: sort by column number; NOTE: this is not part of odbc spec
	if(lhs->m_DiagColumnNumber<rhs->m_DiagColumnNumber)
		return true;
	if(lhs->m_DiagColumnNumber>rhs->m_DiagColumnNumber)
		return false;

	return false;
	}
};
typedef std::multiset<CDiagStatusPtr,DiagRecCompareClass>	CDIAGDEQUE; /*AMR - the compare function is DiagRecCompareClass*/
#else
typedef RWTPtrDlist<CDiagStatus>	CDIAGDEQUE;
#endif

class CDiagRec	{

public:
	CDiagRec();
	~CDiagRec();
	void clear();
	void setDiagRec(short diagComponentCode,
				DWORD		diagErrorCode,
				SQLINTEGER	diagNative,
				char		*diagMessageText,
				char		*diagSqlState,
				SQLLEN  	diagRowNumber,
				SQLINTEGER	diagColumnNumber,
				short		diagNoParams,
				va_list		diagParams);
	void setDiagRec(short diagComponentCode,
				DWORD		diagErrorCode,
				SQLINTEGER	diagNative = 0,
				char		*diagMessageText = NULL,
				char		*diagSqlState = NULL,
				SQLLEN  	diagRowNumber = SQL_ROW_NUMBER_UNKNOWN,
				SQLINTEGER	diagColumnNumber = SQL_COLUMN_NUMBER_UNKNOWN,
				short		diagNoParams = 0,...);
	void setDiagRec(const odbc_SQLSvc_SQLError *SQLError);
	void setDiagRec(const ERROR_DESC_LIST_def *sqlWarning);
	void setDiagRec(BYTE *&WarningOrError, long returnCode);
	void setDiagRec(UINT nativeError, LPSTR funcName, char *srvrIdentity);
	inline void setDiagRowCount(IDL_long_long diagRowCount, SQLLEN diagCursorRowCount)
			{ m_DiagRowCount = diagRowCount; m_DiagCursorRowCount = diagCursorRowCount; };
	inline IDL_long_long getDiagRowCount() { return m_DiagRowCount;};
	SQLRETURN GetDiagRec(SQLSMALLINT	RecNumber,
				DWORD	ErrorMsgLang,
				SQLCHAR		*SqlState,
				SQLINTEGER	*NativeErrPtr,
				SQLCHAR		*MessageText,
				SQLSMALLINT	BufferLength,
				SQLSMALLINT *TextLengthPtr,
				ICUConverter* icuConv);
	SQLRETURN GetDiagField(SQLSMALLINT HandleType,
						SQLHANDLE Handle,
						SQLSMALLINT RecNumber,
						DWORD	ErrorMsgLang,
						SQLSMALLINT	DiagIdentifier,
						SQLPOINTER DiagInfoPtr,
						SQLSMALLINT BufferLength,
						SQLSMALLINT *StringLengthPtr);
	void setNTError(DWORD errorMsgLang, const char *FuncName);
	void removeDiagRec();
	inline void setClearDiagRec(BOOL clearDiagRec){m_ClearDiagRec = clearDiagRec;};
	inline BOOL getClearDiagRec(){return m_ClearDiagRec;};

private:
	SQLLEN  	m_DiagCursorRowCount;
	IDL_long_long m_DiagRowCount;
	SQLINTEGER	m_DiagDynamicFuncCode;
	CDIAGDEQUE	m_DiagStatusCollect;
	BOOL		m_ClearDiagRec;


};


#endif


