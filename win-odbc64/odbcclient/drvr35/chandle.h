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
#ifndef CHANDLE_H
#define CHANDLE_H

#include <list>
#include "CDiag.h"
//#include "drvrnet.h"
extern CRITICAL_SECTION gCollectionCSObject;

using namespace std;

class CHandleGlobal;

class CHandle {

public:
	CHandle(SQLSMALLINT HandleType, SQLHANDLE InputHandle);
	CHandle::~CHandle();
	void clearError();
	void setDiagRec(short diagComponentCode,
				DWORD		diagErrorCode,
				SQLINTEGER	diagNative = 0,
				char		*diagMessageText = NULL,
				char		*diagSqlState = NULL,
				SQLINTEGER	diagRowNumber = SQL_ROW_NUMBER_UNKNOWN,
				SQLINTEGER	diagColumnNumber = SQL_COLUMN_NUMBER_UNKNOWN,
				short		diagNoParams = 0,...);
	SQLRETURN GetDiagRec(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT	RecNumber, 
				DWORD	ErrorMsgLang,
				SQLWCHAR *SqlState,
				SQLINTEGER	*NativeErrPtr, 
				SQLWCHAR *MessageText,
				SQLSMALLINT	BufferLength,
				SQLSMALLINT *TextLengthPtr);
	SQLRETURN GetDiagField(SQLSMALLINT HandleType,
						SQLHANDLE Handle,
						SQLSMALLINT RecNumber,
						DWORD	ErrorMsgLang,
						SQLSMALLINT	DiagIdentifier,
						SQLPOINTER DiagInfoPtr,
						SQLSMALLINT BufferLength,
						SQLSMALLINT *StringLengthPtr);
	inline void setDiagRowCount(long diagRowCount, long diagCursorRowCount) 
	{
		EnterCriticalSection(&gCollectionCSObject);
		m_DiagRec.setDiagRowCount(diagRowCount, diagCursorRowCount); 
		LeaveCriticalSection(&gCollectionCSObject);
	};
	inline SQLINTEGER getDiagRowCount() 
	{ 
		SQLINTEGER	retCode;
		
		EnterCriticalSection(&gCollectionCSObject);
		retCode = m_DiagRec.getDiagRowCount();
		LeaveCriticalSection(&gCollectionCSObject);
		return retCode;
	};
	inline void setDiagRec(const odbc_SQLSvc_SQLError *SQLError) 
	{ 
		EnterCriticalSection(&gCollectionCSObject);
		m_DiagRec.setDiagRec(SQLError);
		LeaveCriticalSection(&gCollectionCSObject);
	};
	inline void setDiagRec(const ERROR_DESC_LIST_def *sqlWarning) 
	{	
		EnterCriticalSection(&gCollectionCSObject);
		m_SqlWarning = TRUE; 
		m_DiagRec.setDiagRec(sqlWarning); 
		LeaveCriticalSection(&gCollectionCSObject);
	};
	inline void setDiagRec(BYTE *&WarningOrError, long returnCode) 
	{	
		EnterCriticalSection(&gCollectionCSObject);
		if (returnCode == SQL_SUCCESS_WITH_INFO)
			m_SqlWarning = TRUE; 
		m_DiagRec.setDiagRec(WarningOrError, returnCode); 
		LeaveCriticalSection(&gCollectionCSObject);
	};
	inline void setDiagRec(UINT nativeError, LPSTR funcName, char *srvrIdentity) 
	{ 
		EnterCriticalSection(&gCollectionCSObject);
		m_DiagRec.setDiagRec(nativeError, funcName, srvrIdentity); 
		LeaveCriticalSection(&gCollectionCSObject);
	};
	inline void setNTError(DWORD errorMsgLang, const char *FuncName)
	{
		EnterCriticalSection(&gCollectionCSObject);
		m_DiagRec.setNTError(errorMsgLang, FuncName);
		LeaveCriticalSection(&gCollectionCSObject);
	};
	inline void setExceptionErrors(long exceptionNr,long exceptionDetail=0) 
	{ 
		m_ExceptionNr = exceptionNr;
		m_ExceptionDetail = exceptionDetail;
	};
	inline long getExceptionNr() { return m_ExceptionNr; };
	inline long getExceptionDetail() { return m_ExceptionDetail; };
	inline BOOL	getSQLWarning() { return m_SqlWarning; };
	inline void setHeartBeatEnable(BOOL heartBeatEnable) { m_HeartBeatEnable = heartBeatEnable; };
	inline BOOL getHeartBeatEnable() { return m_HeartBeatEnable; };
	virtual long sendCDInfo(long exception_nr);
//	DWORD getTranslateOption();
//	DWORD getTranslateOption(DWORD inputCharSet, DWORD outputCharSet, BOOL passthru=TRUE);
	DWORD getDrvrCharSet();
	DWORD getSqlCharSet(long inSqlCharSet);
	SQLRETURN translateStringToDS(SQLCHAR		*inString,
							SQLSMALLINT	inStringLen,
							char		*outString,
							SDWORD		outStringMax,
							SDWORD		*outStringLen = NULL,
							BOOL		IsTruncError = TRUE,
							BOOL		IsCatalogAPI = FALSE,
							BOOL		TranslateToUTF8 = FALSE,
							DWORD		tranlateOption = 0); 
	unsigned long translateStringToDriver(SQLCHAR		*inString,
							SQLSMALLINT	inStringLen,
							char		*outString,
							SDWORD		outStringMax,
							SDWORD		*outStringLen,
							UCHAR		*errorMsg,
							SWORD		errorMsgMax,
							BOOL		TranslateFromUTF8 = FALSE,
							DWORD		tranlateOption = 0); 
	void structExceptionHandling(long exceptionCode); 
	void setWcharConvError(char* error);

protected:
	long		m_HandleNumber;
	short		m_CurrentOdbcAPI;
	SQLSMALLINT	m_HandleType;
	SQLHANDLE	m_InputHandle;
	CDiagRec	m_DiagRec;
	long		m_ExceptionNr;
	long		m_ExceptionDetail;
	BOOL		m_HeartBeatEnable;
	BOOL		m_SqlWarning;
public:
	CRITICAL_SECTION	m_CSObject;

	friend class CHandleGlobal;
};


typedef CHandle	*CHandlePtr;

typedef list<CHandlePtr> CHANDLECOLLECT;

class CHandleGlobal	
{

public:
	CHandleGlobal();
	BOOL validateHandle(SQLSMALLINT HandleType, SQLHANDLE Handle);
private:
	CHANDLECOLLECT	m_HandleCollect;
	long			m_HandleSeq;
	friend class CHandle;
};

#endif
