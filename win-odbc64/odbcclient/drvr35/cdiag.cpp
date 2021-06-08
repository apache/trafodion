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
#include "CDiag.h"
#include "DrvrGlobal.h"
#include "CConnect.h"
#include "tdm_odbcDrvMsg.h"
#include "DrvrSrvr.h"
#include "diagfunctions.h"

#include <algorithm>  
#define	GETDIAGRECPTR(ret, multiset, index)	{ 	CDIAGDEQUE::iterator i = multiset.begin(); 	advance (i, index); ret = *i; }

DIAG_FUNC_MAP diagFuncMap[] = {
	{"SQL_DIAG_UNKNOWN_STATEMENT", SQL_DIAG_UNKNOWN_STATEMENT},
	{NULL}
};

// This should be in sync with ERROR_COMPONENT in drvrGlobal.h
char *gErrorMsgHeader[] = 
{
	"[TRAF][Trafodion ODBC Driver]",
	"[TRAF][Trafodion ODBC Driver][DCS Server]",
	"[TRAF][Trafodion ODBC Driver][Network Component]",
	"[TRAF][Trafodion ODBC Driver][Trafodion Database]",
	"[TRAF][Trafodion ODBC Driver][DCS Services]",
	"[TRAF][Trafodion ODBC Driver][DCS Cfg Server]"
};


// Implementation for the member functions of CDiagRec
CDiagStatus::CDiagStatus(short diagComponentCode,
				DWORD		diagErrorCode,
				SQLINTEGER	diagNative,
				char		*diagMessageText,
				char		*diagSqlState,
				SQLLEN		diagRowNumber,
				SQLINTEGER	diagColumnNumber,
				short		diagNoParams,
				va_list		diagParams)
{
	short i;

	m_DiagComponentCode = diagComponentCode;
	m_DiagErrorCode	= diagErrorCode;
	m_DiagRowNumber = diagRowNumber;
	m_DiagColumnNumber = diagColumnNumber;
	m_DiagNative = diagNative;
	if (diagMessageText != NULL)
		m_DiagMessageText = (char *)diagMessageText;
	else
		m_DiagMessageText = DIAG_EMPTY_STRING;
	for (i = 0; i < MAX_DIAG_PARAMS && i < diagNoParams ; i++)
		m_DiagParams[i] = va_arg(diagParams, char *);
	for (;i < MAX_DIAG_PARAMS ; i++)
		m_DiagParams[i] = DIAG_EMPTY_STRING;
	if (diagSqlState != NULL)
		strncpy((char *)m_DiagSqlState, diagSqlState,6);
	else
		m_DiagSqlState[0] = '\0';
}


// Implementation for the member functions of CDiagRec

CDiagRec::CDiagRec()
{
	m_DiagCursorRowCount = -1;
	m_DiagRowCount = -1;
	m_DiagDynamicFuncCode = SQL_DIAG_UNKNOWN_STATEMENT;
}
	
CDiagRec::~CDiagRec()
{
	clear();
}

void CDiagRec::clear()
{
	CDIAGDEQUE::iterator i, temp; /*AMR*/

	i=m_DiagStatusCollect.begin();
	unsigned long count=m_DiagStatusCollect.size();
	
	while(count>0 && !m_DiagStatusCollect.empty())
	{
		temp = i;
		i++;
		if(*temp)delete *temp;
		m_DiagStatusCollect.erase(temp);
	}
	m_DiagDynamicFuncCode = 0;
}


void CDiagRec::setDiagRec(short diagComponentCode,
				DWORD		diagErrorCode,
				SQLINTEGER	diagNative,
				char		*diagMessageText,
				char		*diagSqlState,
				SQLLEN		diagRowNumber,
				SQLINTEGER	diagColumnNumber,
				short		diagNoParams,
				va_list		diagParams)
{
	CDiagStatusPtr	diagStatus;

	diagStatus = new CDiagStatus(diagComponentCode, diagErrorCode, diagNative, diagMessageText,	
			diagSqlState, diagRowNumber, diagColumnNumber, diagNoParams, diagParams);
	if (diagStatus != NULL)
	{
		CDIAGDEQUE::iterator iterHint = m_DiagStatusCollect.end();  
		m_DiagStatusCollect.insert(iterHint,diagStatus);
	}
}

void CDiagRec::setDiagRec(short diagComponentCode,
				DWORD		diagErrorCode,
				SQLINTEGER	diagNative,
				char		*diagMessageText,
				char		*diagSqlState,
				SQLLEN		diagRowNumber,
				SQLINTEGER	diagColumnNumber,
				short		diagNoParams,...)
{
	va_list marker;

	va_start( marker, diagNoParams); 
	
	setDiagRec(diagComponentCode, diagErrorCode, diagNative,
			diagMessageText, diagSqlState, diagRowNumber, diagColumnNumber, diagNoParams, marker);
}

void CDiagRec::setDiagRec(const odbc_SQLSvc_SQLError *SQLError)
{

	IDL_unsigned_long curErrorNo;
	ERROR_DESC_def *error_desc_def;
	
	for (curErrorNo = 0, error_desc_def = SQLError->errorList._buffer;
						curErrorNo < SQLError->errorList._length ; curErrorNo++, error_desc_def++)
	{
		
		if( error_desc_def->errorCodeType == SQLERRWARN && error_desc_def->sqlcode == 0 && error_desc_def->errorText == NULL ) 
			continue;

		// Resource Governing
		if (error_desc_def->errorCodeType == ESTIMATEDCOSTRGERRWARN)
		{
			// Actual value: error_desc_def->Param4 
			// Limit: error_desc_def->Param2
			setDiagRec(SERVER_ERROR, IDS_S1_000_07, ESTIMATEDCOSTRGERRWARN, NULL, NULL, 
				SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 2, 
				error_desc_def->Param4, error_desc_def->Param2);
				
		}
		else if (error_desc_def->errorCodeType == INFOSTATSERR || error_desc_def->errorCodeType == CONFIGERR)
		{
			// Infostats and CONFIG cmd
			setDiagRec(SERVER_ERROR, IDS_HY_000, error_desc_def->sqlcode, error_desc_def->errorText,
				error_desc_def->sqlstate);
		}
		else if(error_desc_def->sqlcode == SQL_PASSWORD_EXPIRING || error_desc_def->sqlcode == SQL_PASSWORD_GRACEPERIOD)
			setDiagRec(SQLMX_ERROR, IDS_SQL_WARNING, error_desc_def->sqlcode, error_desc_def->errorText,
				error_desc_def->sqlstate);
		else
			setDiagRec(SQLMX_ERROR, IDS_SQL_ERROR, error_desc_def->sqlcode, error_desc_def->errorText,
				error_desc_def->sqlstate);
	}

}

void CDiagRec::setDiagRec(const ERROR_DESC_LIST_def *sqlWarning)
{
	IDL_unsigned_long curErrorNo;
	ERROR_DESC_def *error_desc_def;
	
	for (curErrorNo = 0, error_desc_def = sqlWarning->_buffer;
						curErrorNo < sqlWarning->_length ; curErrorNo++, error_desc_def++)
	{
		
		if( error_desc_def->errorCodeType == SQLERRWARN && error_desc_def->sqlcode == 0 && error_desc_def->errorText == NULL ) 
			continue;

		// Resource Governing
		if (error_desc_def->errorCodeType == ESTIMATEDCOSTRGERRWARN)
		{
			// Actual value: error_desc_def->Param4 
			// Limit: error_desc_def->Param2
			setDiagRec(SERVER_ERROR, IDS_S1_000_08, ESTIMATEDCOSTRGERRWARN, NULL, NULL, 
				SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 2, 
				error_desc_def->Param4, error_desc_def->Param2);
				
		}
		else
		{
			if (error_desc_def->rowId > 0)
				setDiagRec(SQLMX_ERROR, IDS_SQL_WARNING, error_desc_def->sqlcode, error_desc_def->errorText,
					error_desc_def->sqlstate, error_desc_def->rowId);
			else
				setDiagRec(SQLMX_ERROR, IDS_SQL_WARNING, error_desc_def->sqlcode, error_desc_def->errorText,
					error_desc_def->sqlstate);
		}
	}
}
	
void CDiagRec::setDiagRec(BYTE *&WarningOrError, long returnCode)
{
	long msg_total_len = 0;
	
	long numConditions = 0;
	char sqlState[6];
	long sqlCode;
	long errorTextLen = 0;
	char *errorText;
	long rowId = 0;

	sqlState[0] = '\0';

	unsigned char *curptr;
	int i;
		
	curptr = WarningOrError;
	
	numConditions = *(IDL_long*)(curptr+msg_total_len);
	msg_total_len +=4;

	if (numConditions > 0)
	{
		DWORD IDS_SQL_temp = 0;
		if (returnCode == SQL_SUCCESS_WITH_INFO)
			IDS_SQL_temp = IDS_SQL_WARNING;
		else
			IDS_SQL_temp = IDS_SQL_ERROR;

		for (i = 0; i < numConditions; i++)
		{
			rowId= *(IDL_long*)(curptr+msg_total_len);
			msg_total_len +=4;

			sqlCode= *(IDL_long*)(curptr+msg_total_len);
			msg_total_len +=4;

			errorTextLen= *(IDL_long*)(curptr+msg_total_len);
			msg_total_len +=4;

			if (errorTextLen > 0)
			{
				errorText = new char[errorTextLen];
				memcpy(errorText, curptr+msg_total_len, errorTextLen);
				msg_total_len +=errorTextLen;
			}
			else 
			{
				errorText = new char[1]; 
				errorText[0] = '\0';
			}

			memcpy(sqlState, curptr+msg_total_len, sizeof(sqlState));
			sqlState[5] = '\0';
			msg_total_len +=sizeof(sqlState);
			if (rowId > 0)
				setDiagRec(SQLMX_ERROR, IDS_SQL_temp, sqlCode, errorText, sqlState, rowId); 
			else if (rowId == 0)
				setDiagRec(SQLMX_ERROR, IDS_SQL_temp, sqlCode, errorText, sqlState, SQL_NO_ROW_NUMBER); 
			else
				setDiagRec(SQLMX_ERROR, IDS_SQL_temp, sqlCode, errorText, sqlState);
			delete[] errorText;
		}
	}
}

void CDiagRec::setDiagRec(UINT nativeError, LPSTR funcName, char *srvrIdentity)
{
	switch (nativeError)
	{
	case -29:
		setDiagRec(NETWORK_ERROR, IDS_KRYPTON_SRVR_GONE, nativeError, NULL, NULL, SQL_ROW_NUMBER_UNKNOWN,
			SQL_COLUMN_NUMBER_UNKNOWN, 2, funcName, srvrIdentity);
		break;
	case -27:
		setDiagRec(NETWORK_ERROR, IDS_KRYPTON_NO_SRVR, nativeError, NULL, NULL, SQL_ROW_NUMBER_UNKNOWN,
			SQL_COLUMN_NUMBER_UNKNOWN, 2, funcName, srvrIdentity);
		break;
	default:
		setDiagRec(NETWORK_ERROR, IDS_KRYPTON_ERROR, nativeError, NULL, NULL, SQL_ROW_NUMBER_UNKNOWN,
			SQL_COLUMN_NUMBER_UNKNOWN, 2, funcName, FORMAT_LAST_ERROR());
	}
}

SQLRETURN CDiagRec::GetDiagRec(SQLSMALLINT	RecNumber,
						DWORD	ErrorMsgLang,
						SQLWCHAR *SqlState,
						SQLINTEGER	*NativeErrPtr, 
						SQLWCHAR *MessageText,
						SQLSMALLINT	BufferLength,
						SQLSMALLINT *TextLengthPtr)
{
	CDiagStatusPtr	diagStatusPtr;
	short			strLen;
	short			tmpStrLen;
	ODBCMXMSG_Def	MsgStruct;
	SQLRETURN		rc = SQL_SUCCESS;
	SQLINTEGER		translateLength;
	SQLINTEGER		translateLengthMax;
	UCHAR			errorMsg[MAX_TRANSLATE_ERROR_MSG_LEN];
	char			cTmpBuf[132];
	
	if (RecNumber <= (SQLSMALLINT)m_DiagStatusCollect.size())
	{	
		GETDIAGRECPTR(diagStatusPtr, m_DiagStatusCollect, RecNumber-1);
		
		// Get the Formatted Message Text from mc file
		if (MAX_DIAG_PARAMS >= 4) // if MAX_DIAG_PARAMS is changed you may need to change here
		{
			gDrvrGlobal.gOdbcMsg.GetOdbcMessage(ErrorMsgLang, diagStatusPtr->m_DiagErrorCode, &MsgStruct,
				diagStatusPtr->m_DiagParams[0].c_str(), diagStatusPtr->m_DiagParams[1].c_str(), 
				diagStatusPtr->m_DiagParams[2].c_str(), diagStatusPtr->m_DiagParams[3].c_str());
		}
		// Append the Message Text
		MsgStruct.lpsMsgText.append(diagStatusPtr->m_DiagMessageText);
		// Insert the Message Header
		MsgStruct.lpsMsgText.insert(0, gErrorMsgHeader[diagStatusPtr->m_DiagComponentCode]);
		// Append row id
		if(diagStatusPtr->m_DiagRowNumber>SQL_NO_ROW_NUMBER)
		{
			MsgStruct.lpsMsgText.append(" Row: ");
			MsgStruct.lpsMsgText.append(itoa(diagStatusPtr->m_DiagRowNumber,cTmpBuf,10));
			// Append column id
			if(diagStatusPtr->m_DiagColumnNumber>SQL_COLUMN_NUMBER_UNKNOWN)
			{
				MsgStruct.lpsMsgText.append(" Column: ");
				MsgStruct.lpsMsgText.append(itoa(diagStatusPtr->m_DiagColumnNumber,cTmpBuf,10));
			}
		}
		int transLen = 0;
		if (SqlState != NULL)
		{
			char tmpSqlState[6];
			char error[50];
			if (diagStatusPtr->m_DiagSqlState[0] != '\0')
				strncpy((char *)tmpSqlState, (const char *)diagStatusPtr->m_DiagSqlState, 5);
			else
				strncpy((char *)tmpSqlState, MsgStruct.lpsSQLState, 5);
			tmpSqlState[5] = 0;
			SqlState[0] = 0;
			//convert to UTF-16
			rc = UTF8ToWChar(tmpSqlState, 5, SqlState, 6, &transLen, error);
			if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable && rc == SQL_ERROR)
				TraceOut(TR_ODBC_API, "CDiagRec::GetDiagRec: SqlState Error: tmpSqlState \"%s\"", tmpSqlState);
		}
		if (NativeErrPtr != NULL)
			*NativeErrPtr = diagStatusPtr->m_DiagNative;
		tmpStrLen = MsgStruct.lpsMsgText.size();

		//Double strLen to circumvent the bug in driver manager, that requires us to give 
		//the NO. OF BYTES instead of no. of characters
		strLen = tmpStrLen;

		translateLengthMax = (BufferLength == SQL_NTS) ? strLen : BufferLength; 
		
		if (MessageText != NULL)
		{
			// translate from UTF8 to WChar
			if ( (rc = UTF8ToWChar((char *)MsgStruct.lpsMsgText.c_str(), tmpStrLen, MessageText, 
					translateLengthMax, (int *)&translateLength, (char *)errorMsg)) != SQL_SUCCESS )
				rc = SQL_SUCCESS_WITH_INFO; //ERROR;

			((wchar_t *)MessageText)[translateLength] = L'\0';
			strLen = tmpStrLen;
		}
		else
		{
			rc = SQL_SUCCESS_WITH_INFO;
			strLen = tmpStrLen;
		}

		if (TextLengthPtr != NULL)
			*TextLengthPtr = strLen;
	}
	else
	{
		if (SqlState != NULL)
			wcsncpy(SqlState, L"00000", 5);
		if (MessageText != NULL)
			MessageText[0] =L'\0';
		if (TextLengthPtr != NULL)
			*TextLengthPtr = 0;
		rc = SQL_NO_DATA;
	}
	return rc;
}	

SQLRETURN CDiagRec::GetDiagField(SQLSMALLINT HandleType,
						SQLHANDLE Handle,
						SQLSMALLINT RecNumber,
						DWORD	ErrorMsgLang,
						SQLSMALLINT	DiagIdentifier,
						SQLPOINTER DiagInfoPtr,
						SQLSMALLINT BufferLength,
						SQLSMALLINT *StringLengthPtr)
{
	short					i;
	CDiagStatusPtr			diagStatusPtr;
	ODBCMXMSG_Def			MsgStruct;
	SQLINTEGER				lStringLength;					
	SQLRETURN				rc = SQL_SUCCESS;
	RETURN_VALUE_STRUCT		retValue;
	
	retValue.dataType = DRVR_PENDING;
	retValue.u.strPtr = NULL;

	switch (DiagIdentifier)
	{
	case SQL_DIAG_CURSOR_ROW_COUNT:
#ifdef _WIN64
		retValue.u.s64Value = m_DiagCursorRowCount;
		retValue.dataType = SQL_C_SBIGINT;
#else
		retValue.u.s32Value = m_DiagCursorRowCount;
		retValue.dataType = SQL_IS_INTEGER;
#endif
		break;
	case SQL_DIAG_DYNAMIC_FUNCTION:
		for (i = 0; ;i++)
		{
			if (diagFuncMap[i].diagFuncName == NULL)
			{
				retValue.u.strPtr = "SQL_DIAG_UNKNOWN_STATEMENT";
				break;
			}
			if (diagFuncMap[i].diagFuncCode == m_DiagDynamicFuncCode)
			{
				retValue.u.strPtr = diagFuncMap[i].diagFuncName;
				break;
			}
		}
		break;
	case SQL_DIAG_DYNAMIC_FUNCTION_CODE:
		retValue.u.s32Value = m_DiagDynamicFuncCode;
		retValue.dataType = SQL_IS_INTEGER;
		break;
	case SQL_DIAG_NUMBER:
		retValue.u.s32Value = m_DiagStatusCollect.size();
		retValue.dataType = SQL_IS_INTEGER;
		break;
	case SQL_DIAG_RETURNCODE: // Implemented by DM
		break;
	case SQL_DIAG_ROW_COUNT:
#ifdef _WIN64
		retValue.u.s64Value = m_DiagRowCount;
		retValue.dataType = SQL_C_SBIGINT;
#else
		retValue.u.s32Value = m_DiagRowCount;
		retValue.dataType = SQL_IS_INTEGER;
#endif
		break;
	default:
		if (RecNumber <= 0)
		{
			rc = SQL_ERROR;
			break;
		}
		if (RecNumber > (SQLSMALLINT)m_DiagStatusCollect.size())
		{
			rc = SQL_NO_DATA;
			break;
		}
		
		GETDIAGRECPTR(diagStatusPtr, m_DiagStatusCollect, RecNumber-1);
		switch (DiagIdentifier)
		{
		case SQL_DIAG_CLASS_ORIGIN:
			if (strncmp((const char *)diagStatusPtr->m_DiagSqlState, "IM", 2) == 0)
				retValue.u.strPtr = "ODBC 3.0";
			else
				retValue.u.strPtr = "ISO 9075";
			break;
		case SQL_DIAG_CONNECTION_NAME:
			switch (HandleType)
			{
			case SQL_HANDLE_ENV:
				retValue.u.strPtr = EMPTY_STRING;
				break;
			case SQL_HANDLE_DBC:
				retValue.u.strPtr = ((CConnect *)Handle)->getSrvrIdentity();
				break;
			case SQL_HANDLE_STMT:
				break;
			default:
				retValue.u.strPtr = EMPTY_STRING;
				break;
			}
			break;
		case SQL_DIAG_MESSAGE_TEXT:
			// Get the Formatted Message Text from mc file
			if (MAX_DIAG_PARAMS >= 4) // if MAX_DIAG_PARAMS is changed you may need to change here
			{
				gDrvrGlobal.gOdbcMsg.GetOdbcMessage(ErrorMsgLang, diagStatusPtr->m_DiagErrorCode, &MsgStruct,
					diagStatusPtr->m_DiagParams[0].c_str(), diagStatusPtr->m_DiagParams[1].c_str(), 
					diagStatusPtr->m_DiagParams[2].c_str(), diagStatusPtr->m_DiagParams[3].c_str());
			}
			// Append the Message Text
			MsgStruct.lpsMsgText.append(diagStatusPtr->m_DiagMessageText);
			// Insert the Message Header
			MsgStruct.lpsMsgText.insert(0, gErrorMsgHeader[diagStatusPtr->m_DiagComponentCode]);
			retValue.u.strPtr = (char *)MsgStruct.lpsMsgText.c_str();
			break;
		case SQL_DIAG_SERVER_NAME:
			switch (HandleType)
			{
			case SQL_HANDLE_ENV:
				retValue.u.strPtr = EMPTY_STRING;
				break;
			case SQL_HANDLE_DBC:
				retValue.u.strPtr = ((CConnect *)Handle)->getSrvrDSName();
				break;
			default:
				retValue.u.strPtr = EMPTY_STRING;
				break;
			}
			break;
		case SQL_DIAG_SQLSTATE:
			if (diagStatusPtr->m_DiagSqlState[0] != '\0')
				retValue.u.strPtr = (char *)diagStatusPtr->m_DiagSqlState;
			else
			{
				// Get the Formatted Message Text from mc file
				if (MAX_DIAG_PARAMS >= 4) // if MAX_DIAG_PARAMS is changed you may need to change here
				{
					gDrvrGlobal.gOdbcMsg.GetOdbcMessage(ErrorMsgLang, diagStatusPtr->m_DiagErrorCode, &MsgStruct,
						diagStatusPtr->m_DiagParams[0].c_str(), diagStatusPtr->m_DiagParams[1].c_str(), 
						diagStatusPtr->m_DiagParams[2].c_str(), diagStatusPtr->m_DiagParams[3].c_str());
				}
				retValue.u.strPtr = (char *)MsgStruct.lpsSQLState;
			}
			break;
		case SQL_DIAG_SUBCLASS_ORIGIN:
			retValue.u.strPtr = EMPTY_STRING;
			break;
		case SQL_DIAG_COLUMN_NUMBER:
			retValue.u.s32Value = diagStatusPtr->m_DiagColumnNumber;
			retValue.dataType = SQL_IS_INTEGER;
			break;
		case SQL_DIAG_ROW_NUMBER:
#ifdef _WIN64
			retValue.u.s64Value = diagStatusPtr->m_DiagRowNumber;
			retValue.dataType = SQL_C_SBIGINT;
#else
			retValue.u.s32Value = diagStatusPtr->m_DiagRowNumber;
			retValue.dataType = SQL_IS_INTEGER;
#endif
			break;
		case SQL_DIAG_NATIVE:
			retValue.u.s32Value = diagStatusPtr->m_DiagNative;
			retValue.dataType = SQL_IS_INTEGER;
			break;
		default:
			rc = SQL_ERROR;
			break;
		}
	}
	if (rc == SQL_SUCCESS)
	{
		rc = returnAttrValue(FALSE, NULL, &retValue, DiagInfoPtr, BufferLength, &lStringLength);
		if (StringLengthPtr != NULL)
			*StringLengthPtr = (short)lStringLength;
	}
	return rc;
}

void CDiagRec::setNTError(DWORD errorMsgLang, const char *FuncName)
{
	LPVOID	lpMsgBuf;
	DWORD	error;
	char	buffer[10];

	error = GetLastError();
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				error,
				errorMsgLang,
				(LPTSTR) &lpMsgBuf,
				0,
				NULL);
	setDiagRec(DRIVER_ERROR, IDS_NT_ERROR, error, (char *)lpMsgBuf, NULL, 
		SQL_ROW_NUMBER_UNKNOWN, SQL_COLUMN_NUMBER_UNKNOWN, 2, FuncName, _itoa(error,buffer,10));
	LocalFree(lpMsgBuf);
}
