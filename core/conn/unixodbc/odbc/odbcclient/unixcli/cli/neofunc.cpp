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

#include <stdio.h>
#include <stdlib.h>
#include "drvrglobal.h"
#include "sqlhandle.h"
#include "sqlconnect.h"
#include "sqlenv.h"
#include "sqlstmt.h"
#include "sqldesc.h"
#include "diagfunctions.h"
#include "DrvrSrvr.h"
#include "cstmt.h"
#include <math.h>
#include "charsetconv.h"
#include "neofunc.h"

using namespace ODBC;

#define TRACE_RETURN(retHandle, rc)\
if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable)\
{\
    if (retHandle)\
    {\
        if (gTraceFlags & TR_ODBC_INFO)\
        {\
            if (fpTracePrintMarker)\
                (fpTracePrintMarker)();\
            if (fpTraceReturn)\
                (fpTraceReturn)(retHandle, rc);\
        }\
    }\
    if ((gTraceFlags & TR_ODBC_WARN) && (rc == SQL_SUCCESS_WITH_INFO))\
        (fpTraceError)(retHandle, rc);\
    else\
    if ((gTraceFlags & TR_ODBC_ERROR) && (rc == SQL_ERROR))\
        (fpTraceError)(retHandle, rc);\
}

#define TRACE_RETURN_CONFIG(retHandle, rc)\
if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable)\
{\
    if (retHandle)\
    {\
        if (gTraceFlags & TR_ODBC_CONFIG)\
        {\
            if (fpTracePrintMarker)\
                (fpTracePrintMarker)();\
            if (fpTraceReturn)\
                (fpTraceReturn)(retHandle, rc);\
        }\
    }\
    if ((gTraceFlags & TR_ODBC_WARN) && (rc == SQL_SUCCESS_WITH_INFO))\
        (fpTraceError)(retHandle, rc);\
    else\
    if ((gTraceFlags & TR_ODBC_ERROR) && (rc == SQL_ERROR))\
        (fpTraceError)(retHandle, rc);\
}

SQLRETURN  SQL_API NeoAllocHandle(SQLSMALLINT HandleType,
				SQLHANDLE InputHandle, 
				SQLHANDLE *OutputHandle)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLAllocHandle)
				retHandle = (fpTraceSQLAllocHandle)(HandleType, InputHandle, OutputHandle);
		}
	}
	else
		RESET_TRACE();

	rc = AllocHandle(HandleType, InputHandle, OutputHandle);

	TRACE_RETURN(retHandle, rc);
	return rc;
}


SQLRETURN  SQL_API NeoFreeHandle(SQLSMALLINT HandleType, 
				SQLHANDLE Handle)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLFreeHandle)
				retHandle = (fpTraceSQLFreeHandle)(HandleType, Handle);
		}
	}
	else
		RESET_TRACE();

	rc = FreeHandle(HandleType, Handle);

	TRACE_RETURN(retHandle, rc);
	return rc;
}


SQLRETURN SQL_API NeoGetDiagRec(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT RecNumber,
				SQLCHAR *Sqlstate,
				SQLINTEGER *NativeError, 
				SQLCHAR *MessageText,
				SQLSMALLINT BufferLength, 
				SQLSMALLINT *TextLength,
				bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLGetDiagRec)
				retHandle = (fpTraceSQLGetDiagRec)(HandleType, Handle, RecNumber, Sqlstate,
							NativeError, MessageText, BufferLength, TextLength);
		}
	}
	else
		RESET_TRACE();

	rc = GetDiagRec(HandleType, Handle, RecNumber, Sqlstate, NativeError, MessageText, BufferLength,TextLength);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API NeoGetDiagField(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT RecNumber, 
				SQLSMALLINT DiagIdentifier,
				SQLPOINTER DiagInfo, 
				SQLSMALLINT BufferLength,
				SQLSMALLINT *StringLength,
				bool isWideCall)
{	
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLGetDiagField)
				retHandle = (fpTraceSQLGetDiagField)(HandleType, Handle, RecNumber, DiagIdentifier,
							DiagInfo, BufferLength, StringLength);
		}
	}
	else
		RESET_TRACE();

	rc = GetDiagField(HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength, StringLength);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API NeoEndTran(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT CompletionType)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;
	DWORD       convError;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLEndTran)
				retHandle = (fpTraceSQLEndTran)(HandleType, Handle, CompletionType);
		}
	}
	else
		RESET_TRACE();

	rc = EndTran(HandleType, Handle, CompletionType);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API NeoConnect(SQLHDBC ConnectionHandle,
           SQLCHAR *ServerName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *UserName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *Authentication, 
		   SQLSMALLINT NameLength3,
		   bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

    SQLCHAR *u8_ServerName = NULL;
    SQLINTEGER ServerNameLen_inBytes, u8_ServerNameLen, u8_ServerNameTransLen;
    SQLCHAR *u8_UserName = NULL;
	SQLINTEGER UserNameLen_inBytes, u8_UserNameLen, u8_UserNameTransLen;
    SQLCHAR *u8_Authentication = NULL; 
	SQLINTEGER AuthenticationLen_inBytes, u8_AuthenticationLen, u8_AuthenticationTransLen;
	char errorMsg[MAX_TRANSLATE_ERROR_MSG_LEN];

    SQLSMALLINT u16_len;
	DWORD convError;

	if(isWideCall)
	{
		if(!((CConnect*)ConnectionHandle)->setAppType(APP_TYPE_UNICODE))
		{
			((CHandle*)ConnectionHandle)->setDiagRec(DRIVER_ERROR, IDS_HY_090, 0, "Cannot mix Ansi calls and W calls");
			return SQL_ERROR;
		}
	}
	else
	{
		if(!((CConnect*)ConnectionHandle)->setAppType(APP_TYPE_ANSI))
		{
			((CHandle*)ConnectionHandle)->setDiagRec(DRIVER_ERROR, IDS_HY_090, 0, "Cannot mix Ansi calls and W calls");
			return SQL_ERROR;
		}
	}

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLConnect)
				retHandle = (fpTraceSQLConnect)(ConnectionHandle, ServerName, NameLength1, UserName, 
						NameLength2, Authentication, NameLength3);
		}
	}
	else
		RESET_TRACE();
//Find the lengths
	ServerNameLen_inBytes = gDrvrGlobal.ICUConv.FindStrLength((const char*)ServerName, (SQLINTEGER)NameLength1);	
	UserNameLen_inBytes = gDrvrGlobal.ICUConv.FindStrLength((const char*)UserName, (SQLINTEGER)NameLength2);
	AuthenticationLen_inBytes = gDrvrGlobal.ICUConv.FindStrLength((const char*)Authentication, (SQLINTEGER)NameLength3);		
//Log the given strings, if trace is on
	if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
	{
		if (ServerNameLen_inBytes != 0)
			HexOut(TR_ODBC_DEBUG, ServerNameLen_inBytes, ServerName, "ServerName");
		if(UserNameLen_inBytes != 0)
			HexOut(TR_ODBC_DEBUG, UserNameLen_inBytes, UserName, "Username");
	}
//Allocate temp utf8 buffers	   	
	u8_ServerName = (SQLCHAR*)malloc(ServerNameLen_inBytes * 4); 
	u8_ServerNameLen = ServerNameLen_inBytes * 4;	
	if (SQL_SUCCESS != gDrvrGlobal.ICUConv.InputArgToUTF8Helper(ServerName, ServerNameLen_inBytes, u8_ServerName,
												 u8_ServerNameLen,&u8_ServerNameTransLen, isWideCall, errorMsg))
	{
		//An error during input aeg translation, exit right away
		((CConnect*)ConnectionHandle)->setDiagRec(DRIVER_ERROR, IDS_HY_090, 0, (char*)errorMsg);
		free(u8_ServerName);
		return SQL_ERROR;		   
	}
	u8_UserName = (SQLCHAR*)malloc(UserNameLen_inBytes * 4); 
	u8_UserNameLen = UserNameLen_inBytes * 4;	
	if (SQL_SUCCESS != gDrvrGlobal.ICUConv.InputArgToUTF8Helper(UserName, UserNameLen_inBytes, u8_UserName,
												 u8_UserNameLen,&u8_UserNameTransLen, isWideCall, errorMsg))
	{
		//An error during input aeg translation, exit right away
		((CConnect*)ConnectionHandle)->setDiagRec(DRIVER_ERROR, IDS_HY_090, 0, (char*)errorMsg);
		free(u8_ServerName);
		free(u8_UserName);
		return SQL_ERROR;		   
	}
	u8_Authentication = (SQLCHAR*)malloc(AuthenticationLen_inBytes * 4); 
	u8_AuthenticationLen = AuthenticationLen_inBytes * 4;	
	if (SQL_SUCCESS != gDrvrGlobal.ICUConv.InputArgToUTF8Helper(Authentication, AuthenticationLen_inBytes, u8_Authentication,
												 u8_AuthenticationLen, &u8_AuthenticationTransLen, isWideCall, errorMsg))
	{
		//An error during input aeg translation, exit right away
		((CConnect*)ConnectionHandle)->setDiagRec(DRIVER_ERROR, IDS_HY_090, 0, (char*)errorMsg);
		free(u8_ServerName);
		free(u8_UserName);
		free(u8_Authentication);
		return SQL_ERROR;		   
	}

	rc = Connect(ConnectionHandle, 
					u8_ServerName, u8_ServerNameTransLen, 
					u8_UserName, u8_UserNameTransLen,
					u8_Authentication, u8_AuthenticationTransLen);
	TRACE_RETURN(retHandle, rc);

    if(u8_ServerName != NULL)
	{
       free(u8_ServerName);
       free(u8_UserName);
       free(u8_Authentication);
	}

	return rc;

} // NeoConnect

SQLRETURN SQL_API NeoDisconnect(SQLHDBC ConnectionHandle)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLDisconnect)
				retHandle = (fpTraceSQLDisconnect)(ConnectionHandle);
		}
	}
	else
		RESET_TRACE();

	rc = Disconnect(ConnectionHandle);

	TRACE_RETURN(retHandle, rc);
	return rc;
}
 
SQLRETURN  SQL_API NeoSetConnectAttr(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength,
		   bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;
	if(isWideCall)
	{
		if(!((CConnect*)ConnectionHandle)->setAppType(APP_TYPE_UNICODE))
		{
			((CHandle*)ConnectionHandle)->setDiagRec(DRIVER_ERROR, IDS_HY_090, 0, "Cannot mix Ansi calls and W calls");
			return SQL_ERROR;
		}
	}
	else
	{
		if(!((CConnect*)ConnectionHandle)->setAppType(APP_TYPE_ANSI))
		{
			((CHandle*)ConnectionHandle)->setDiagRec(DRIVER_ERROR, IDS_HY_090, 0, "Cannot mix Ansi calls and W calls");
			return SQL_ERROR;
		}
	}

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_CONFIG))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLSetConnectAttr)
				retHandle = (fpTraceSQLSetConnectAttr)(ConnectionHandle, Attribute, Value, StringLength);
		}
	}
	else
		RESET_TRACE();

	rc = SetConnectAttr(ConnectionHandle, Attribute, Value, StringLength);

	TRACE_RETURN_CONFIG(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API NeoGetConnectAttr(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength,
		   bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLGetConnectAttr)
				retHandle = (fpTraceSQLGetConnectAttr)(ConnectionHandle, Attribute, Value, BufferLength,
							StringLength);
		}
	}
	else
		RESET_TRACE();

	rc = GetConnectAttr(ConnectionHandle, Attribute, Value, BufferLength, StringLength);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API NeoSetEnvAttr(SQLHENV EnvironmentHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_CONFIG))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLSetEnvAttr)
				retHandle = (fpTraceSQLSetEnvAttr)(EnvironmentHandle, Attribute, Value, StringLength);
		}
	}
	else
		RESET_TRACE();

	rc = SetEnvAttr(EnvironmentHandle, Attribute, Value, StringLength);

	TRACE_RETURN_CONFIG(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API NeoGetEnvAttr(SQLHENV EnvironmentHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLGetEnvAttr)
				retHandle = (fpTraceSQLGetEnvAttr)(EnvironmentHandle, Attribute, Value, BufferLength, StringLength);
		}
	}
	else
		RESET_TRACE();

	rc = GetEnvAttr(EnvironmentHandle, Attribute, Value, BufferLength, StringLength);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API NeoSetStmtAttr(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength,
		   bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_CONFIG))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLSetStmtAttr)
				retHandle = (fpTraceSQLSetStmtAttr)(StatementHandle, Attribute, Value, StringLength);
		}
	}
	else
		RESET_TRACE();

	rc = SetStmtAttr(StatementHandle, Attribute, Value, StringLength);

	TRACE_RETURN_CONFIG(retHandle, rc);
	return rc;

}

SQLRETURN  SQL_API NeoGetStmtAttr(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLength,
		   bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLGetStmtAttr)
				retHandle = (fpTraceSQLGetStmtAttr)(StatementHandle, Attribute, Value, BufferLength, 
				StringLength);
		}
	}
	else
		RESET_TRACE();

	rc = GetStmtAttr(StatementHandle, Attribute, Value, BufferLength, StringLength);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API NeoGetInfo(SQLHDBC ConnectionHandle,
           SQLUSMALLINT InfoType, 
		   SQLPOINTER InfoValuePtr,
           SQLSMALLINT BufferLength,
		   SQLSMALLINT *StringLengthPtr,
		   bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLGetInfo)
				retHandle = (fpTraceSQLGetInfo)(ConnectionHandle, InfoType, InfoValuePtr, BufferLength, StringLengthPtr);
		}
	}
	else
		RESET_TRACE();

    rc = GetInfo(ConnectionHandle, InfoType, InfoValuePtr, BufferLength, StringLengthPtr);

	TRACE_RETURN(retHandle, rc);
	return rc;
}


SQLRETURN  SQL_API NeoSetDescField(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr, 
		   SQLINTEGER BufferLength,
		   bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLSetDescField)
				retHandle = (fpTraceSQLSetDescField)(DescriptorHandle, RecNumber, FieldIdentifier, ValuePtr, BufferLength);
		}
	}
	else
		RESET_TRACE();


	rc = SetDescField(DescriptorHandle, RecNumber, FieldIdentifier, ValuePtr, BufferLength);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API NeoSetDescRec(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber,
		   SQLSMALLINT Type,
           SQLSMALLINT SubType, 
		   SQLLEN Length,
           SQLSMALLINT Precision, 
		   SQLSMALLINT Scale,
           SQLPOINTER Data, 
		   SQLLEN *StringLengthPtr,
           SQLLEN *IndicatorPtr)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLSetDescRec)
				retHandle = (fpTraceSQLSetDescRec)(DescriptorHandle, RecNumber, Type, SubType, Length, 
							Precision, Scale, Data, StringLengthPtr, IndicatorPtr);
		}
	}
	else
		RESET_TRACE();

	rc = SetDescRec(DescriptorHandle, RecNumber, Type, SubType, Length, Precision, Scale, Data,
			StringLengthPtr, IndicatorPtr);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API NeoGetDescField(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLSMALLINT FieldIdentifier,
           SQLPOINTER ValuePtr, 
		   SQLINTEGER BufferLength,
           SQLINTEGER *StringLengthPtr,
		   bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLGetDescField)
				retHandle = (fpTraceSQLGetDescField)(DescriptorHandle, RecNumber, FieldIdentifier,
						ValuePtr, BufferLength, StringLengthPtr);
		}
	}
	else
		RESET_TRACE();

	rc = GetDescField(DescriptorHandle, RecNumber, FieldIdentifier, ValuePtr, BufferLength, 
				StringLengthPtr);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API NeoGetDescRec(SQLHDESC DescriptorHandle,
           SQLSMALLINT RecNumber, 
		   SQLCHAR *Name,
           SQLSMALLINT BufferLength, 
		   SQLSMALLINT *StringLengthPtr,
           SQLSMALLINT *TypePtr, 
		   SQLSMALLINT *SubTypePtr, 
           SQLLEN     *LengthPtr, 
		   SQLSMALLINT *PrecisionPtr, 
           SQLSMALLINT *ScalePtr, 
		   SQLSMALLINT *NullablePtr,
		   bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLGetDescRec)
				retHandle = (fpTraceSQLGetDescRec)(DescriptorHandle, RecNumber, Name, BufferLength, StringLengthPtr, TypePtr, 
						SubTypePtr, LengthPtr, PrecisionPtr, ScalePtr, NullablePtr);
		}
	}
	else
		RESET_TRACE();

	rc = GetDescRec(DescriptorHandle, RecNumber, Name, BufferLength, StringLengthPtr, TypePtr, 
				SubTypePtr, LengthPtr, PrecisionPtr, ScalePtr, NullablePtr);

	TRACE_RETURN(retHandle, rc);
	return rc;
}


SQLRETURN  SQL_API NeoBindCol(SQLHSTMT StatementHandle, 
		   SQLUSMALLINT ColumnNumber, 
		   SQLSMALLINT TargetType, 
		   SQLPOINTER TargetValue, 
		   SQLLEN BufferLength, 
	   	   SQLLEN *StrLen_or_IndPtr)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLBindCol)
				retHandle = (fpTraceSQLBindCol)(StatementHandle, ColumnNumber, TargetType, 
							TargetValue, BufferLength, StrLen_or_IndPtr);
		}
	}
	else
		RESET_TRACE();

	rc = BindCol(StatementHandle, ColumnNumber, TargetType, TargetValue, BufferLength, StrLen_or_IndPtr);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API NeoBindParameter(SQLHSTMT StatementHandle,
			SQLUSMALLINT ParameterNumber, 
			SQLSMALLINT InputOutputType,
			SQLSMALLINT ValueType,
			SQLSMALLINT ParameterType, 
			SQLULEN 	ColumnSize,
			SQLSMALLINT DecimalDigits,
			SQLPOINTER  ParameterValuePtr,
			SQLLEN   	BufferLength,
			SQLLEN     *StrLen_or_IndPtr)		   
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLBindParameter)
				retHandle = (fpTraceSQLBindParameter)(StatementHandle, ParameterNumber, InputOutputType, ValueType, ParameterType,
				ColumnSize, DecimalDigits,ParameterValuePtr, BufferLength, StrLen_or_IndPtr);
		}
	}
	else
		RESET_TRACE();

	rc = BindParameter(StatementHandle, ParameterNumber, InputOutputType, ValueType, ParameterType,
				ColumnSize, DecimalDigits,ParameterValuePtr, BufferLength, StrLen_or_IndPtr);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API NeoBrowseConnect(
    SQLHDBC            ConnectionHandle,
    SQLCHAR 		  *InConnectionString,
    SQLSMALLINT        StringLength1,
    SQLCHAR 		  *OutConnectionString,
    SQLSMALLINT        BufferLength,
    SQLSMALLINT       *StringLength2Ptr,
    bool               isWideCall)
{
	SQLRETURN	rc, rc1;
	RETCODE		retHandle = 0;
 	SQLINTEGER StringLength1_inBytes = 0;
    SQLCHAR *u8_InConnectionString = NULL;
    SQLINTEGER u8_InConnectionStringLength = 0;
    SQLINTEGER u8_InConnectionStringTransLength = 0;
    
    SQLCHAR *u8_OutConnectionString = NULL;
    SQLINTEGER u8_OutConnectionStringLength = 0;
    SQLSMALLINT u8_len = 0;
    
	char errorMsg[MAX_TRANSLATE_ERROR_MSG_LEN];
    
    SQLINTEGER transBufferLength = 0;
	DWORD convError;
		
	if(isWideCall)
	{
		if(!((CConnect*)ConnectionHandle)->setAppType(APP_TYPE_UNICODE))
		{
			((CHandle*)ConnectionHandle)->setDiagRec(DRIVER_ERROR, IDS_HY_090, 0, "Cannot mix Ansi calls and W calls");
			return SQL_ERROR;
		}
	}
	else
	{
		if(!((CConnect*)ConnectionHandle)->setAppType(APP_TYPE_ANSI))
		{
			((CHandle*)ConnectionHandle)->setDiagRec(DRIVER_ERROR, IDS_HY_090, 0, "Cannot mix Ansi calls and W calls");
			return SQL_ERROR;
		}
	}
	
	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLBrowseConnect)
				retHandle = (fpTraceSQLBrowseConnect)(ConnectionHandle, InConnectionString, StringLength1,
					OutConnectionString, BufferLength, StringLength2Ptr);
		}
	}
	else
		RESET_TRACE();
//Find all lengths
	StringLength1_inBytes = gDrvrGlobal.ICUConv.FindStrLength((const char*)InConnectionString, StringLength1);
	if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
		if (StringLength1_inBytes != 0)
		{
			int tmp_StringLength1_inBytes = StringLength1_inBytes;
                        //Remove the passwd part of it from the trace string
                        char* pwdStart = (char*)strstr((const char*)InConnectionString, "PWD=");
			if(pwdStart != NULL)
	                        tmp_StringLength1_inBytes = (pwdStart+4) - (char*)InConnectionString;
			HexOut(TR_ODBC_DEBUG, tmp_StringLength1_inBytes, InConnectionString, "InConnectionString");
		}
	
	//Allocate temp utf8 buffers	   	
	StringLength1_inBytes = gDrvrGlobal.ICUConv.FindStrLength((const char*)InConnectionString, StringLength1);
	u8_InConnectionString = (SQLCHAR*)malloc(StringLength1_inBytes * 4); 
	u8_InConnectionStringLength = StringLength1_inBytes * 4;
	
	u8_OutConnectionString = (SQLCHAR*)malloc(u8_InConnectionStringLength*2); 
	u8_OutConnectionStringLength = (SQLINTEGER)u8_InConnectionStringLength*2;

	 //Converts arguments to utf8
	if (SQL_SUCCESS != gDrvrGlobal.ICUConv.InputArgToUTF8Helper(InConnectionString, StringLength1_inBytes, u8_InConnectionString,
												 u8_InConnectionStringLength,&u8_InConnectionStringTransLength, isWideCall, errorMsg))
	{
		//An error during input aeg translation, exit right away
		((CConnect*)ConnectionHandle)->setDiagRec(DRIVER_ERROR, IDS_HY_090, 0, (char*)errorMsg);
		free(u8_InConnectionString);
		free(u8_OutConnectionString);
		return SQL_ERROR;		   
	}
	
	rc = BrowseConnect(ConnectionHandle, u8_InConnectionString, u8_InConnectionStringTransLength,
	                       u8_OutConnectionString, u8_OutConnectionStringLength, &u8_len);
	                       

	TRACE_RETURN(retHandle, rc); //call TRACE_RETURN when everything is in  UTF-8	
	
	if(rc != SQL_ERROR)
	{
		if (OutConnectionString != NULL && BufferLength != 0)  // App is interested in getting an output ptr
		{
			if (SQL_SUCCESS != (rc1 = gDrvrGlobal.ICUConv.OutputArgFromUTF8Helper(u8_OutConnectionString, u8_len, OutConnectionString, 
															BufferLength, &transBufferLength, isWideCall, errorMsg)))
			{
				//An error/truncation during output arg translation, log it and return a SQL_SUCCESS_WITH_INFO
				((CConnect*)ConnectionHandle)->setDiagRec(DRIVER_ERROR, IDS_HY_090, 0, (char*)errorMsg);
			}
			
			if (StringLength2Ptr != NULL)
				*StringLength2Ptr = transBufferLength;                   
		}
		else
			*StringLength2Ptr = 0;
	}
    if(u8_InConnectionString != NULL)
	{
       free(u8_InConnectionString);
       free(u8_OutConnectionString);
	}
	if((rc == SQL_SUCCESS) && (rc1 != SQL_SUCCESS))
		rc = SQL_SUCCESS_WITH_INFO;

	return rc;
}

SQLRETURN SQL_API NeoDriverConnect(SQLHDBC  ConnectionHandle,
    SQLHWND            WindowHandle,
    SQLCHAR 		  *InConnectionString,
    SQLSMALLINT        StringLength1,
    SQLCHAR           *OutConnectionString,
    SQLSMALLINT        BufferLength,
    SQLSMALLINT 	  *StringLength2Ptr,
    SQLUSMALLINT       DriverCompletion,
    bool               isWideCall)
{
	SQLRETURN	rc, rc1;
	RETCODE		retHandle = 0;
	
	SQLINTEGER StringLength1_inBytes = 0;
    SQLCHAR *u8_InConnectionString = NULL;
    SQLINTEGER u8_InConnectionStringLength = 0;
    SQLINTEGER u8_InConnectionStringTransLength = 0;
    
    SQLCHAR *u8_OutConnectionString = NULL;
    SQLINTEGER u8_OutConnectionStringLength = 0;
    SQLSMALLINT u8_len = 0;
    
	char errorMsg[MAX_TRANSLATE_ERROR_MSG_LEN];
    
    SQLINTEGER transBufferLength = 0;

	DWORD convError;

	
	if(isWideCall)
	{
		if(!(((CConnect*)ConnectionHandle)->setAppType(APP_TYPE_UNICODE)))
		{
			((CHandle*)ConnectionHandle)->setDiagRec(DRIVER_ERROR, IDS_HY_090, 0, "Cannot mix Ansi calls and W calls");
			return SQL_ERROR;
		}
	}
	else
	{
		if(!(((CConnect*)ConnectionHandle)->setAppType(APP_TYPE_ANSI)))
		{
			((CHandle*)ConnectionHandle)->setDiagRec(DRIVER_ERROR, IDS_HY_090, 0, "Cannot mix Ansi calls and W calls");
			return SQL_ERROR;
		}
	}

	if (IsTraceLibrary()) //Trace can handle UTF8 or UTF16 or Locale
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLDriverConnect)
				retHandle = (fpTraceSQLDriverConnect)(ConnectionHandle, WindowHandle, InConnectionString, StringLength1,
					OutConnectionString, BufferLength, StringLength2Ptr, DriverCompletion);

		}
	}
	else
		RESET_TRACE();
		
//Find all String lengths
	StringLength1_inBytes = gDrvrGlobal.ICUConv.FindStrLength((const char*)InConnectionString, StringLength1);
	if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
		if (StringLength1_inBytes != 0)
		{
			int tmp_StringLength1_inBytes = StringLength1_inBytes;
			//Remove the passwd part of it from the trace string
			char * pwdStart = (char*)strstr((const char*)InConnectionString, "PWD=");
			if(pwdStart != NULL)
				tmp_StringLength1_inBytes = (pwdStart+4) - (char*)InConnectionString;
			HexOut(TR_ODBC_DEBUG, tmp_StringLength1_inBytes, InConnectionString, "InConnectionString");
		}
//Allocate temp utf8 buffers	   	
    if(StringLength1_inBytes>0){
        u8_InConnectionString = (SQLCHAR*)malloc(StringLength1_inBytes * 4); 
        u8_InConnectionStringLength = StringLength1_inBytes * 4;
	
        u8_OutConnectionString = (SQLCHAR*)malloc(u8_InConnectionStringLength*2); 
        u8_OutConnectionStringLength = (SQLINTEGER)u8_InConnectionStringLength*2;
	
        //Converts arguments to utf8
        if (SQL_SUCCESS != gDrvrGlobal.ICUConv.InputArgToUTF8Helper(InConnectionString, StringLength1_inBytes, u8_InConnectionString,
                                                                    u8_InConnectionStringLength,&u8_InConnectionStringTransLength, isWideCall, errorMsg))
        {
            //An error during input aeg translation, exit right away
            ((CConnect*)ConnectionHandle)->setDiagRec(DRIVER_ERROR, IDS_HY_090, 0, (char*)errorMsg);
            free(u8_InConnectionString);
            free(u8_OutConnectionString);
            return SQL_ERROR;		   
        }
    }
	   
	rc = DriverConnect(ConnectionHandle, WindowHandle, u8_InConnectionString, u8_InConnectionStringTransLength,
                       u8_OutConnectionString, (SQLSMALLINT)u8_OutConnectionStringLength, &u8_len, DriverCompletion);
		
	TRACE_RETURN(retHandle, rc); //call TRACE_RETURN when everything is in  UTF-8
	
	if(rc != SQL_ERROR)
	{
		if (OutConnectionString != NULL && BufferLength != 0)  // App is interested in getting an output ptr
		{
			if (SQL_SUCCESS != (rc1 = gDrvrGlobal.ICUConv.OutputArgFromUTF8Helper(u8_OutConnectionString, u8_len, OutConnectionString, 
															BufferLength, &transBufferLength, isWideCall, errorMsg)))
			{
				//An error/truncation during output arg translation, log it and return a SQL_SUCCESS_WITH_INFO
				((CConnect*)ConnectionHandle)->setDiagRec(DRIVER_ERROR, IDS_HY_090, 0, (char*)errorMsg);
			}
			
			if (StringLength2Ptr != NULL)
				*StringLength2Ptr = transBufferLength;                   
		}
		else if(StringLength2Ptr != NULL)
		{
			if(StringLength2Ptr != 0)
				*StringLength2Ptr = 0;
		}
	}
    if(u8_InConnectionString != NULL)
	{
       free(u8_InConnectionString);
       free(u8_OutConnectionString);
	}
	if((rc == SQL_SUCCESS) && (rc1 != SQL_SUCCESS))
		rc = SQL_SUCCESS_WITH_INFO;
	
	return rc;
}

SQLRETURN SQL_API NeoPrepare(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText,
		   SQLINTEGER TextLength,
		   bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;
    SQLCHAR *u8_StatementText = NULL;
    SQLINTEGER u8_StatementTextLen,textLength_inBytes, u8_StatementTextTransLen;
	char errorMsg[MAX_TRANSLATE_ERROR_MSG_LEN];
	
	if(isWideCall)
	{
		if(!(((CStmt*)StatementHandle)->setAppType(APP_TYPE_UNICODE)))
		{
			((CHandle*)StatementHandle)->setDiagRec(DRIVER_ERROR, IDS_HY_090, 0, "Cannot mix Ansi calls and W calls");
			return SQL_ERROR;
		}
	}
	else
	{
		if(!(((CStmt*)StatementHandle)->setAppType(APP_TYPE_ANSI)))
		{
			((CHandle*)StatementHandle)->setDiagRec(DRIVER_ERROR, IDS_HY_090, 0, "Cannot mix Ansi calls and W calls");
			return SQL_ERROR;
		}
	}
	
	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLPrepare)
				retHandle = (fpTraceSQLPrepare)(StatementHandle, StatementText, TextLength);
		}
	}
	else
		RESET_TRACE();
		
//Find String length
	textLength_inBytes = gDrvrGlobal.ICUConv.FindStrLength((const char*)StatementText, TextLength);
	if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
		if (textLength_inBytes != 0)
			HexOut(TR_ODBC_DEBUG, textLength_inBytes, StatementText, "StatementText");

//Allocate temp utf8 buffers	   	
	u8_StatementTextLen = textLength_inBytes * 4;
	u8_StatementText = (SQLCHAR*)malloc(u8_StatementTextLen); 
	
	  //Converts arguments to utf8
	if (SQL_SUCCESS !=  ((CHandle*)StatementHandle)->m_ICUConv->InputArgToUTF8Helper(StatementText, textLength_inBytes, u8_StatementText,
												 u8_StatementTextLen,&u8_StatementTextTransLen, isWideCall, errorMsg))
	{
		//An error during input aeg translation, exit right away
		((CConnect*)StatementHandle)->setDiagRec(DRIVER_ERROR, IDS_HY_090, 0, (char*)errorMsg);
		free(u8_StatementText);
		return SQL_ERROR;		   
	}

	rc = Prepare(StatementHandle, u8_StatementText, u8_StatementTextTransLen);

    if(u8_StatementText != NULL)
       free(u8_StatementText);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API NeoExecDirect(SQLHSTMT StatementHandle,
           SQLCHAR *StatementText,
		   SQLINTEGER TextLength,
		   bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;
    SQLCHAR *u8_StatementText = NULL;
    SQLINTEGER u8_StatementTextLen,textLength_inBytes, u8_StatementTextTransLen;
	char errorMsg[MAX_TRANSLATE_ERROR_MSG_LEN];

	if(isWideCall)
	{
		if(!((CHandle*)StatementHandle)->setAppType(APP_TYPE_UNICODE))
		{
			((CHandle*)StatementHandle)->setDiagRec(DRIVER_ERROR, IDS_HY_090, 0, "Cannot mix Ansi calls and W calls");
			return SQL_ERROR;
		}
	}
	else
	{
		if(!((CHandle*)StatementHandle)->setAppType(APP_TYPE_ANSI))
		{
			((CHandle*)StatementHandle)->setDiagRec(DRIVER_ERROR, IDS_HY_090, 0, "Cannot mix Ansi calls and W calls");
			return SQL_ERROR;
		}
	}

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLExecDirect)
				retHandle = (fpTraceSQLExecDirect)(StatementHandle, StatementText, TextLength);
		}
	}
	else
		RESET_TRACE();
		
//Find the string length
	textLength_inBytes = gDrvrGlobal.ICUConv.FindStrLength((const char*)StatementText, TextLength);		
	if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
		if (textLength_inBytes != 0)
			HexOut(TR_ODBC_DEBUG, textLength_inBytes, StatementText, "StatementText");
		
//Allocate temp utf8 buffers	   	
	u8_StatementTextLen = textLength_inBytes * 4;
	u8_StatementText = (SQLCHAR*)malloc(u8_StatementTextLen); 

	if (SQL_SUCCESS !=  ((CHandle*)StatementHandle)->m_ICUConv->InputArgToUTF8Helper(StatementText, textLength_inBytes, u8_StatementText,
												 u8_StatementTextLen,&u8_StatementTextTransLen, isWideCall, errorMsg))
	{
		//An error during input aeg translation, exit right away
		((CConnect*)StatementHandle)->setDiagRec(DRIVER_ERROR, IDS_HY_090, 0, (char*)errorMsg);
		free(u8_StatementText);
		return SQL_ERROR;		   
	}
	
	rc = ExecDirect(StatementHandle, u8_StatementText, u8_StatementTextTransLen);

    if(u8_StatementText != NULL)
       free(u8_StatementText);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API NeoDescribeCol(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLCHAR *ColumnName,
           SQLSMALLINT BufferLength, 
		   SQLSMALLINT *NameLengthPtr,
           SQLSMALLINT *DataTypePtr, 
		   SQLULEN *ColumnSizePtr,
           SQLSMALLINT *DecimalDigitsPtr,
		   SQLSMALLINT *NullablePtr,
		   bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLDescribeCol)
				retHandle = (fpTraceSQLDescribeCol)(StatementHandle, ColumnNumber, ColumnName, 
						BufferLength, NameLengthPtr, DataTypePtr, ColumnSizePtr, DecimalDigitsPtr, NullablePtr);
		}
	}
	else
		RESET_TRACE();

	rc = getDescRec(StatementHandle, SQL_API_SQLDESCRIBECOL, ColumnNumber, ColumnName, BufferLength, NameLengthPtr,
			DataTypePtr, ColumnSizePtr, DecimalDigitsPtr, NullablePtr);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API NeoNumResultCols(SQLHSTMT StatementHandle,
           SQLSMALLINT *ColumnCountPtr)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLNumResultCols)
				retHandle = (fpTraceSQLNumResultCols)(StatementHandle, ColumnCountPtr);		
		}
	}
	else
		RESET_TRACE();

	rc = getDescSize(StatementHandle, SQL_API_SQLNUMRESULTCOLS, ColumnCountPtr);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API NeoNumParams(SQLHSTMT StatementHandle,
           SQLSMALLINT *ParameterCountPtr)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLNumParams)
				retHandle = (fpTraceSQLNumParams)(StatementHandle, ParameterCountPtr);
		}
	}
	else
		RESET_TRACE();

	rc = getDescSize(StatementHandle, SQL_API_SQLNUMPARAMS, ParameterCountPtr);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API NeoDescribeParam(
	SQLHSTMT           StatementHandle,
	SQLUSMALLINT       ParameterNumber,
    SQLSMALLINT 	  *DataTypePtr,
    SQLULEN      	  *ParameterSizePtr,
    SQLSMALLINT 	  *DecimalDigitsPtr,
    SQLSMALLINT 	  *NullablePtr)
{
	SQLRETURN	rc = SQL_ERROR;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLDescribeParam)
				retHandle = (fpTraceSQLDescribeParam)(StatementHandle, ParameterNumber, DataTypePtr,
						ParameterSizePtr, DecimalDigitsPtr, NullablePtr);
		}
	}
	else
		RESET_TRACE();

	rc = getDescRec(StatementHandle, SQL_API_SQLDESCRIBEPARAM, ParameterNumber, NULL, 0, NULL,
			DataTypePtr, ParameterSizePtr, DecimalDigitsPtr, NullablePtr);

	TRACE_RETURN(retHandle, rc);
	return rc;

}

SQLRETURN SQL_API NeoFreeStmt(SQLHSTMT StatementHandle,
        SQLUSMALLINT Option)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLFreeStmt)
				retHandle = (fpTraceSQLFreeStmt)(StatementHandle, Option);
		}
	}
	else
		RESET_TRACE();

	rc = FreeStmt(StatementHandle, SQL_API_SQLFREESTMT, Option);

	TRACE_RETURN(retHandle, rc);
	return rc;
}


SQLRETURN SQL_API NeoCloseCursor(SQLHSTMT StatementHandle)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLCloseCursor)
				retHandle = (fpTraceSQLCloseCursor)(StatementHandle);
		}
	}
	else
		RESET_TRACE();

	rc = FreeStmt(StatementHandle, SQL_API_SQLCLOSECURSOR, SQL_CLOSE);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API NeoTables(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
           SQLCHAR *TableType, 
		   SQLSMALLINT NameLength4,
		   bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLTables)
				retHandle = (fpTraceSQLTables)(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, 
						TableName, NameLength3, TableType, NameLength4);
		}
	}
	else
		RESET_TRACE();

	rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLTABLES, CatalogName, NameLength1, SchemaName, NameLength2, 
				TableName, NameLength3, NULL, SQL_NTS, TableType, NameLength4);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API NeoColumns(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
           SQLCHAR *ColumnName, 
		   SQLSMALLINT NameLength4,
		   bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLColumns)
				retHandle = (fpTraceSQLColumns)(StatementHandle, CatalogName, NameLength1, SchemaName, 
						NameLength2, TableName, NameLength3,  ColumnName, NameLength4);
		}
	}
	else
		RESET_TRACE();

	rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLCOLUMNS, CatalogName, 
				NameLength1, SchemaName, NameLength2, 
				TableName, NameLength3,  ColumnName, 
				NameLength4);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API NeoSpecialColumns(SQLHSTMT StatementHandle,
		   SQLUSMALLINT IdentifierType,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
		   SQLUSMALLINT Scope,
		   SQLUSMALLINT Nullable,
		   bool isWideCall)
{
 	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLSpecialColumns)
				retHandle = (fpTraceSQLSpecialColumns)(StatementHandle, IdentifierType, CatalogName, NameLength1, SchemaName, 
						NameLength2, TableName, NameLength3,  Scope, Nullable);
		}
	}
	else
		RESET_TRACE();

	rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLSPECIALCOLUMNS, CatalogName, 
				NameLength1, SchemaName, NameLength2, 
				TableName, NameLength3,  NULL, SQL_NTS, NULL, SQL_NTS, IdentifierType, Scope, Nullable);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API NeoGetTypeInfo(SQLHSTMT StatementHandle,
           SQLSMALLINT DataType,
		   bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLGetTypeInfo)
				retHandle = (fpTraceSQLGetTypeInfo)(StatementHandle, DataType);
		}
	}
	else
		RESET_TRACE();

	rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLGETTYPEINFO, NULL, 0, 
				NULL, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, 0, DataType);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API NeoPrimaryKeys(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
		   bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLPrimaryKeys)
				retHandle = (fpTraceSQLPrimaryKeys)(StatementHandle, CatalogName, NameLength1, SchemaName, 
						NameLength2, TableName, NameLength3);
		}
	}
	else
		RESET_TRACE();

	rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLPRIMARYKEYS, CatalogName, 
				NameLength1, SchemaName, NameLength2, 
				TableName, NameLength3);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API NeoStatistics(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *TableName, 
		   SQLSMALLINT NameLength3,
		   SQLUSMALLINT Unique,
		   SQLUSMALLINT Reserved,
		   bool isWideCall)
{
 	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLStatistics)
				retHandle = (fpTraceSQLStatistics)(StatementHandle, CatalogName, NameLength1, SchemaName, 
						NameLength2, TableName, NameLength3, Unique, Reserved);
		}
	}
	else
		RESET_TRACE();

	rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLSTATISTICS, CatalogName, 
				NameLength1, SchemaName, NameLength2, 
				TableName, NameLength3,  NULL, SQL_NTS, NULL, SQL_NTS, 0, 0, 0, 0, Unique, Reserved);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API NeoGetCursorName(SQLHSTMT StatementHandle,
           SQLCHAR *CursorName, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *NameLengthPtr,
		   bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLGetCursorName)
				retHandle = (fpTraceSQLGetCursorName)(StatementHandle, CursorName, BufferLength, 
						NameLengthPtr);
		}
	}
	else
		RESET_TRACE();

	rc = GetCursorName(StatementHandle, CursorName, BufferLength, NameLengthPtr);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API NeoSetCursorName(SQLHSTMT StatementHandle,
           SQLCHAR *CursorName, 
		   SQLSMALLINT NameLength,
		   bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLSetCursorName)
				retHandle = (fpTraceSQLSetCursorName)(StatementHandle, CursorName,  NameLength);
		}
	}
	else
		RESET_TRACE();

	rc = SetCursorName(StatementHandle, CursorName,  NameLength);

	TRACE_RETURN(retHandle, rc);
	return rc;
}
		
SQLRETURN  SQL_API NeoRowCount(SQLHSTMT StatementHandle, 
	   SQLLEN	*RowCountPtr)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLRowCount)
				retHandle = (fpTraceSQLRowCount)(StatementHandle, RowCountPtr);
		}
	}
	else
		RESET_TRACE();

	rc = RowCount(StatementHandle, RowCountPtr);

	TRACE_RETURN(retHandle, rc);
	return rc;
}


SQLRETURN SQL_API NeoMoreResults(
    SQLHSTMT           StatementHandle)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLMoreResults)
				retHandle = (fpTraceSQLMoreResults)(StatementHandle);
		}
	}
	else
		RESET_TRACE();

	rc = FreeStmt(StatementHandle, SQL_API_SQLMORERESULTS, SQL_CLOSE);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API NeoNativeSql(
    SQLHDBC            ConnectionHandle,
    SQLCHAR 		  *InStatementText,
    SQLINTEGER         TextLength1,
    SQLCHAR 		  *OutStatementText,
    SQLINTEGER         BufferLength,
    SQLINTEGER 		  *TextLength2Ptr,
	bool              isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLNativeSql)
				retHandle = (fpTraceSQLNativeSql)(ConnectionHandle, InStatementText, TextLength1, OutStatementText, 
					BufferLength, TextLength2Ptr);
		}
	}
	else
		RESET_TRACE();

	rc = NativeSql(ConnectionHandle, InStatementText, TextLength1, OutStatementText, 
				BufferLength, TextLength2Ptr);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API NeoCancel(SQLHSTMT StatementHandle)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLCancel)
				retHandle = (fpTraceSQLCancel)(StatementHandle);
		}
	}
	else
		RESET_TRACE();

	rc = Cancel(StatementHandle);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API NeoColAttribute(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLUSMALLINT FieldIdentifier,
           SQLPOINTER CharacterAttributePtr, 
		   SQLSMALLINT BufferLength,
           SQLSMALLINT *StringLengthPtr, 
		   SQLPOINTER NumericAttributePtr,
		   bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLColAttribute)
				retHandle = (fpTraceSQLColAttribute)(StatementHandle, ColumnNumber, FieldIdentifier,
						CharacterAttributePtr, BufferLength, StringLengthPtr, NumericAttributePtr);
		}
	}
	else
		RESET_TRACE();

	rc = ColAttribute(StatementHandle, ColumnNumber, FieldIdentifier, CharacterAttributePtr, BufferLength,
					StringLengthPtr, NumericAttributePtr);

	TRACE_RETURN(retHandle, rc);
	return rc;
}


SQLRETURN SQL_API NeoExecute(SQLHSTMT StatementHandle)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLExecute)
				retHandle = (fpTraceSQLExecute)(StatementHandle);
		}
	}
	else
		RESET_TRACE();

	rc = Execute(StatementHandle);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API NeoParamData(SQLHSTMT StatementHandle,
           SQLPOINTER *ValuePtrPtr)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLParamData)
				retHandle = (fpTraceSQLParamData)(StatementHandle, ValuePtrPtr);
		}
	}
	else
		RESET_TRACE();

	rc = ParamData(StatementHandle, ValuePtrPtr);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API NeoPutData(SQLHSTMT StatementHandle,
           SQLPOINTER DataPtr, 
		   SQLLEN StrLen_or_Ind)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLPutData)
				retHandle = (fpTraceSQLPutData)(StatementHandle, DataPtr, StrLen_or_Ind);
		}
	}
	else
		RESET_TRACE();

	rc = PutData(StatementHandle, DataPtr, StrLen_or_Ind);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API NeoFetch(SQLHSTMT StatementHandle)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLFetch)
				retHandle = (fpTraceSQLFetch)(StatementHandle);
		}
	}
	else
		RESET_TRACE();

	rc = Fetch(StatementHandle);

	TRACE_RETURN(retHandle, rc);
	return rc;
}


SQLRETURN SQL_API NeoGetData(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, 
		   SQLSMALLINT  TargetType,
           SQLPOINTER   TargetValuePtr, 
		   SQLLEN       BufferLength,
           SQLLEN      *StrLen_or_IndPtr)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLGetData)
				retHandle = (fpTraceSQLGetData)(StatementHandle, ColumnNumber, TargetType, 
						TargetValuePtr, BufferLength, StrLen_or_IndPtr);
		}
	}
	else
		RESET_TRACE();

	rc = GetData(StatementHandle, ColumnNumber, TargetType, TargetValuePtr, BufferLength, 
							StrLen_or_IndPtr);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API NeoSetPos(
    SQLHSTMT        StatementHandle,
    SQLSETPOSIROW	RowNumber,
    SQLUSMALLINT    Operation,
    SQLUSMALLINT    LockType)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLSetPos)
				retHandle = (fpTraceSQLSetPos)(StatementHandle, RowNumber, Operation, LockType);
		}
	}
	else
		RESET_TRACE();

	rc = SetPos(StatementHandle, RowNumber, Operation, LockType);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API NeoCopyDesc(SQLHDESC SourceDescHandle,
		SQLHDESC TargetDescHandle)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLCopyDesc)
				retHandle = (fpTraceSQLCopyDesc)(SourceDescHandle, TargetDescHandle);
		}
	}
	else
		RESET_TRACE();

	rc = CopyDesc(SourceDescHandle, TargetDescHandle);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN  SQL_API NeoProcedures(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *ProcName, 
		   SQLSMALLINT NameLength3,
		   bool isWideCall)
           
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLProcedures)
				retHandle = (fpTraceSQLProcedures)(StatementHandle, CatalogName, NameLength1, SchemaName, 
						NameLength2, ProcName, NameLength3);
		}
	}
	else
		RESET_TRACE();
	rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLPROCEDURES, CatalogName, 
				NameLength1, SchemaName, NameLength2, 
				ProcName, NameLength3);

	TRACE_RETURN(retHandle, rc);
	return rc;
}


SQLRETURN  SQL_API NeoProcedureColumns(SQLHSTMT StatementHandle,
           SQLCHAR *CatalogName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *SchemaName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *ProcName, 
		   SQLSMALLINT NameLength3,
           SQLCHAR *ColumnName, 
		   SQLSMALLINT NameLength4,
		   bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLProcedureColumns)
				retHandle = (fpTraceSQLProcedureColumns)(StatementHandle, CatalogName, NameLength1, SchemaName, 
						NameLength2, ProcName, NameLength3,  ColumnName, NameLength4);
		}
	}
	else
		RESET_TRACE();

	rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLPROCEDURECOLUMNS, CatalogName, 
				NameLength1, SchemaName, NameLength2, 
				ProcName, NameLength3,  ColumnName, 
				NameLength4);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API NeoExtendedFetch(SQLHSTMT StatementHandle,
			SQLUSMALLINT FetchOrientation,
			SQLLEN FetchOffset,
			SQLULEN* RowCountPtr,
			SQLUSMALLINT* RowStatusArray)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLFetch)
				retHandle = (fpTraceSQLExtendedFetch)(StatementHandle,FetchOrientation,
						FetchOffset,RowCountPtr,RowStatusArray);
		}
	}
	else
		RESET_TRACE();

	rc = ExtendedFetch(StatementHandle,FetchOrientation,FetchOffset,RowCountPtr,RowStatusArray);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API NeoFetchScroll(
	SQLHSTMT StatementHandle,
    SQLSMALLINT FetchOrientation,
    SQLLEN FetchOffset)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLFetch)
				retHandle = (fpTraceSQLFetchScroll)(StatementHandle,FetchOrientation,FetchOffset);
		}
	}
	else
		RESET_TRACE();

	rc = FetchScroll(StatementHandle,FetchOrientation,FetchOffset);

	TRACE_RETURN(retHandle, rc);
	return rc;
}


SQLRETURN SQL_API NeoColumnPrivileges(SQLHSTMT StatementHandle,
     SQLCHAR* CatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR* SchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR* TableName,
     SQLSMALLINT NameLength3,
     SQLCHAR* ColumnName,
     SQLSMALLINT NameLength4,
	 bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLColumnPrivileges)
				retHandle = (fpTraceSQLColumnPrivileges)(StatementHandle, CatalogName, NameLength1, SchemaName, 
						NameLength2, TableName, NameLength3, ColumnName, NameLength4);
		}
	}
	else
		RESET_TRACE();

	rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLCOLUMNPRIVILEGES, CatalogName, 
				NameLength1, SchemaName, NameLength2, 
				TableName, NameLength3, ColumnName, NameLength4);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API NeoForeignKeys(SQLHSTMT StatementHandle,
     SQLCHAR *PKCatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR *PKSchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR *PKTableName,
     SQLSMALLINT NameLength3,
     SQLCHAR *FKCatalogName,
     SQLSMALLINT NameLength4,
     SQLCHAR *FKSchemaName,
     SQLSMALLINT NameLength5,
     SQLCHAR *FKTableName,
     SQLSMALLINT NameLength6,
	 bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLForeignKeys)
				retHandle = (fpTraceSQLForeignKeys)(StatementHandle, PKCatalogName, NameLength1, PKSchemaName, 
						NameLength2, PKTableName, NameLength3, FKCatalogName, NameLength4, FKSchemaName, 
						NameLength5, FKTableName, NameLength6);
		}
	}
	else
		RESET_TRACE();

	rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLFOREIGNKEYS, PKCatalogName, NameLength1, PKSchemaName, 
						NameLength2, PKTableName, NameLength3, NULL, SQL_NTS, NULL, SQL_NTS, 0, 0, 0, 0, 0, 0,
						FKCatalogName, NameLength4, FKSchemaName, NameLength5, FKTableName, NameLength6);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

SQLRETURN SQL_API NeoTablePrivileges(
	 SQLHSTMT StatementHandle,
     SQLCHAR *CatalogName,
     SQLSMALLINT NameLength1,
     SQLCHAR *SchemaName,
     SQLSMALLINT NameLength2,
     SQLCHAR *TableName,
     SQLSMALLINT NameLength3,
	 bool isWideCall)
{
	SQLRETURN	rc;
	RETCODE		retHandle = 0;

	if (IsTraceLibrary())
	{
		InitializeTrace();
		if (pdwGlobalTraceVariable && *pdwGlobalTraceVariable && (gTraceFlags & TR_ODBC_ERROR))
		{
			if (fpTracePrintMarker)
				(fpTracePrintMarker)();
			if (fpTraceSQLTablePrivileges)
				retHandle = (fpTraceSQLTablePrivileges)(StatementHandle, CatalogName, NameLength1, SchemaName, 
						NameLength2, TableName, NameLength3);
		}
	}
	else
		RESET_TRACE();

	rc = GetSQLCatalogs(StatementHandle, SQL_API_SQLTABLEPRIVILEGES, CatalogName, 
				NameLength1, SchemaName, NameLength2, 
				TableName, NameLength3);

	TRACE_RETURN(retHandle, rc);
	return rc;
}

//**************************************************************************
//================== APIs not implemented ==================================
//**************************************************************************

SQLRETURN SQL_API NeoBulkOperations(
	SQLHSTMT StatementHandle,
    SQLSMALLINT Operation)
{
	((CStmt*)StatementHandle)->setDiagRec( DM_ERROR, IDS_IM_001);
	return SQL_ERROR;
}






