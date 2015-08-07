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

#ifdef VERSION3
	#include <list>
#else
	#include <rw/tpslist.h>
#endif

#include "cdiag.h"
#include "nix.h"
#include "charsetconv.h"


class CHandleGlobal;

class CHandle {

public:
	CHandle(SQLSMALLINT HandleType, SQLHANDLE InputHandle);
	virtual ~CHandle();
	virtual void clearError();
	void purgeError();
	void setDiagRec(short diagComponentCode,
				DWORD		diagErrorCode,
				SQLINTEGER	diagNative = 0,
				char		*diagMessageText = NULL,
				char		*diagSqlState = NULL,
				SQLINTEGER	diagRowNumber = SQL_ROW_NUMBER_UNKNOWN,
				SQLINTEGER	diagColumnNumber = SQL_COLUMN_NUMBER_UNKNOWN,
				short		diagNoParams = 0,...);
	//10-080201-0266
	SQLRETURN GetDiagRec(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT	RecNumber, 
				DWORD	ErrorMsgLang,
				SQLCHAR *SqlState,
				SQLINTEGER	*NativeErrPtr, 
				SQLCHAR *MessageText,
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
	inline void setDiagRowCount(long diagRowCount, long diagCursorRowCount) 
	{
		EnterCriticalSection2(&m_CSObject);
		m_DiagRec.setDiagRowCount(diagRowCount, diagCursorRowCount); 
		LeaveCriticalSection2(&m_CSObject);
	};
	inline IDL_long_long getDiagRowCount() 
	{ 
		IDL_long_long retCode;
		
		EnterCriticalSection2(&m_CSObject);
		retCode = m_DiagRec.getDiagRowCount();
		LeaveCriticalSection2(&m_CSObject);
		return retCode;
	};
	inline void setDiagRec(const odbc_SQLSvc_SQLError *SQLError) 
	{ 
		EnterCriticalSection2(&m_CSObject);
		m_DiagRec.setDiagRec(SQLError);
		LeaveCriticalSection2(&m_CSObject);
	};
	inline void setDiagRec(const ERROR_DESC_LIST_def *sqlWarning) 
	{	
		EnterCriticalSection2(&m_CSObject);
		m_SqlWarning = TRUE; 
		m_DiagRec.setDiagRec(sqlWarning); 
		LeaveCriticalSection2(&m_CSObject);
	};
	inline void setDiagRec(BYTE *&WarningOrError, long returnCode)
	{	
		EnterCriticalSection2(&m_CSObject);
		if (returnCode == SQL_SUCCESS_WITH_INFO)
			m_SqlWarning = TRUE; 
		m_DiagRec.setDiagRec(WarningOrError, returnCode); 
		LeaveCriticalSection2(&m_CSObject);
	};
	inline void setDiagRec(UINT nativeError, LPSTR funcName, char *srvrIdentity) 
	{ 
		EnterCriticalSection2(&m_CSObject);
		m_DiagRec.setDiagRec(nativeError, funcName, srvrIdentity); 
		LeaveCriticalSection2(&m_CSObject);
	};
	inline void setNTError(DWORD errorMsgLang, const char *FuncName)
	{
		EnterCriticalSection2(&m_CSObject);
		m_DiagRec.setNTError(errorMsgLang, FuncName);
		LeaveCriticalSection2(&m_CSObject);
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
	DWORD getTranslateOption();
	DWORD getTranslateOption(DWORD inputCharSet, DWORD outputCharSet, BOOL passthru=TRUE);
	DWORD getSqlCharSet(long inSqlCharSet);
	void structExceptionHandling(long exceptionCode); 

	inline short getCurrentOdbcAPI(){return m_CurrentOdbcAPI;};
	inline void removeDiagRec()
	{
		EnterCriticalSection2(&m_CSObject);
		m_DiagRec.removeDiagRec();
		LeaveCriticalSection2(&m_CSObject);
	};
	inline void setClearDiagRec(BOOL clearDiagRec){m_DiagRec.setClearDiagRec(clearDiagRec);};
	inline BOOL getClearDiagRec(){return m_DiagRec.getClearDiagRec();};
	inline SQLSMALLINT getHandleType(){ return m_HandleType; }

	inline void EnterCriticalCode()
	{
		EnterCriticalSection2(&m_CSObject);
	};

	inline void LeaveCriticalCode()
	{
		LeaveCriticalSection2(&m_CSObject);
	};
	bool setAppType(int value);

public:
	int			type;
	int			state;
	int			cursor_state;
	int			prep_state;
    int			asyn_on;		/* async executing which odbc call */
    int			need_on;		/* which call return SQL_NEED_DATA */

    short		stmt_cip;		/* Call in progress on this handle */
	short		dbc_cip;
	short		desc_cip;

	SQLRETURN	returncode;		// for SQLGetDiagField

	// for charset support
	ICUConverter*	m_ICUConv;

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
	CEE_handle_def		m_SrvrCallProxy;
	//CRITICAL_SECTION	m_CSObject;
	CRITICAL_SECTION2	m_CSObject;

//	bool			m_IsUnicodeApp; // If an application calls SQLConnectW or SQLDriverConnectW 
	
	friend class CHandleGlobal;
};


typedef CHandle	*CHandlePtr;

#ifdef VERSION3
typedef std::list<CHandlePtr> CHANDLECOLLECT;
#else
typedef RWTPtrSlist<CHandle> CHANDLECOLLECT;
#endif

class CHandleGlobal	
{

public:
	CHandleGlobal();
	BOOL validateHandle(SQLSMALLINT HandleType, SQLHANDLE Handle);
	inline CHANDLECOLLECT getHandles() { return m_HandleCollect; }
private:
	CHANDLECOLLECT	m_HandleCollect;
	long			m_HandleSeq;
	friend class CHandle;
};

#endif
