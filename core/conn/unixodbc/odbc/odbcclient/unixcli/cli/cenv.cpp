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

#include "cenv.h"
#include "drvrglobal.h"
#include "cconnect.h"

// Implements the member functions of CEnv

CEnv::CEnv(SQLHANDLE InputHandle) : CHandle(SQL_HANDLE_ENV, InputHandle)
{
	m_ODBCVersion    = SQL_OV_ODBC3;


   /*
    * Check the environment variable "AppUnicodeType"
	* for the application unicode type
	*/
	const char *envAppUnicodeType = getenv("AppUnicodeType");

    if(envAppUnicodeType != NULL)
	{
	   if(strcmp(envAppUnicodeType,"utf16") == 0)
          gDrvrGlobal.ICUConv.m_AppUnicodeType = APP_UNICODE_TYPE_UTF16;
	   else
	      gDrvrGlobal.ICUConv.m_AppUnicodeType = APP_UNICODE_TYPE_UTF8;
	}
   /*
    * check the mxodsn/odbc.ini file to get the application unicode type
	*/
	else if(gDrvrGlobal.fpSQLGetPrivateProfileString != NULL)
	{
		DWORD	len;
		char	searchKey[512];
		CHAR*	szODBC = "ODBC";
		char    *OdbcIniEnv = getenv("ODBCINI");
		char 	szAppUnicodeType[128];
		//
		// Get Certificate Directory Location
		// Search for it first in the Datasource section, then try the odbc section
		//
		strncpy(searchKey, (const char *)szODBC, sizeof(searchKey));
		len = (gDrvrGlobal.fpSQLGetPrivateProfileString)(
							searchKey,
							"AppUnicodeType",
							"SYSTEM_DEFAULT",
							szAppUnicodeType,
							sizeof(szAppUnicodeType),
							OdbcIniEnv);
		if(strcmp(szAppUnicodeType,"utf16") == 0)
		   gDrvrGlobal.ICUConv.m_AppUnicodeType = APP_UNICODE_TYPE_UTF16;
		else
		   gDrvrGlobal.ICUConv.m_AppUnicodeType = APP_UNICODE_TYPE_UTF8;
	}     
   /*
    * defaults to utf8
	*/
	else 
	   gDrvrGlobal.ICUConv.m_AppUnicodeType = APP_UNICODE_TYPE_UTF8;

}

CEnv::~CEnv()
{
}

SQLRETURN CEnv::SetEnvAttr(SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength)
{
	
	SQLRETURN rc = SQL_SUCCESS;
	SQLINTEGER intValue;
	char buffer[256];

	switch (Attribute)
	{
	case SQL_ATTR_CONNECTION_POOLING:			// Implemented by DM
		break;
	case SQL_ATTR_CP_MATCH:						// Implemented by DM
		break;
	case SQL_ATTR_ODBC_VERSION:
		m_ODBCVersion = (long)ValuePtr;
		break;
	case SQL_ATTR_OUTPUT_NTS:					// Implemented by DM
		break;			
#if defined(ASYNCIO) && defined(DBSELECT)
	case SQL_ATTR_DBSELECT:
		if(gDrvrGlobal.gdbSelect == NULL)
		{
			gDrvrGlobal.gdbSelect = new AsyncSelect(this,SQL_DBSELECT_QUEUE_LEN_DEFAULT);

			if(gDrvrGlobal.gdbSelect == NULL || !gDrvrGlobal.gdbSelect->IsInitialized())
			{
				if(gDrvrGlobal.gdbSelect != NULL)
					delete gDrvrGlobal.gdbSelect;
				gDrvrGlobal.gdbSelect = NULL;
				return SQL_ERROR;
			}
		}
		intValue = (SQLINTEGER)(SQLLEN)ValuePtr;
		if(intValue == SQL_DBSELECT_ENABLE_ON)
			gDrvrGlobal.gdbSelect->SetEnabled(true);
		else
			gDrvrGlobal.gdbSelect->SetEnabled(false);

		break;				

	case SQL_ATTR_DBSELECT_QUEUE_LEN:
		if(gDrvrGlobal.gdbSelect == NULL)
		{
			gDrvrGlobal.gdbSelect = new AsyncSelect(this,(int)(SQLLEN)ValuePtr);

			if(gDrvrGlobal.gdbSelect == NULL || !gDrvrGlobal.gdbSelect->IsInitialized())
			{
				if(gDrvrGlobal.gdbSelect != NULL)
					delete gDrvrGlobal.gdbSelect;
				gDrvrGlobal.gdbSelect = NULL;
				return SQL_ERROR;
			}
		}

		intValue = (SQLINTEGER)(SQLLEN)ValuePtr;
		if(gDrvrGlobal.gdbSelect->GetQueueLen() != intValue)
		{
			if(gDrvrGlobal.gdbSelect->SetQueueLen(intValue) == SQL_SUCCESS)
				gDrvrGlobal.gdbSelect->SetEnabled(true);
			else
				return SQL_ERROR;
		}

		break;				
#endif /* ASYNCIO && DBSELECT */
	}
	return rc;
}

SQLRETURN CEnv::GetEnvAttr(SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER BufferLength,
								   SQLINTEGER *StringLengthPtr)
{
	
	SQLRETURN rc = SQL_SUCCESS;
	RETURN_VALUE_STRUCT		retValue;
	
	retValue.dataType = DRVR_PENDING;
	retValue.u.strPtr = NULL;

	switch (Attribute)
	{
	case SQL_ATTR_CONNECTION_POOLING:			// Implemented by DM
		break;
	case SQL_ATTR_CP_MATCH:						// Implemented by DM
		break;
	case SQL_ATTR_ODBC_VERSION:
		retValue.u.s32Value = m_ODBCVersion;
		retValue.dataType = SQL_IS_INTEGER;
		break;
	case SQL_ATTR_OUTPUT_NTS:					// Implemented by DM
		break;									
    case 1065: // DataDirect's SQL_ATTR_DRIVER_UNICODE_TYPE which is defined as:
	           // #define SQL_ATTR_DRIVER_UNICODE_TYPE (SQL_CONOPT_START+25) (#define SQL_CONOPT_START 1040)
	/*
	 * The DataDirect driver manager will query our driver to see what unicode mode we support
	 * We'll reply back based on whatever m_AppUnicodeType is set to
	 */
	    if(gDrvrGlobal.ICUConv.m_AppUnicodeType == APP_UNICODE_TYPE_UTF8)
		{
           retValue.u.s32Value = 2; // DataDirect's SQL_DD_CP_UTF8
           retValue.dataType   = SQL_IS_INTEGER; 
		}
		else
		{
           retValue.u.s32Value = 1; // DataDirect's SQL_DD_CP_UTF16
           retValue.dataType = SQL_IS_INTEGER;
		}
	    break;
	}
	if (rc == SQL_SUCCESS)
		rc = returnAttrValue(TRUE, this, &retValue, ValuePtr, BufferLength, StringLengthPtr);
	return rc;
}

SQLRETURN CEnv::AllocHandle(SQLSMALLINT HandleType,SQLHANDLE InputHandle, SQLHANDLE *OutputHandle)
{
	SQLRETURN rc = SQL_SUCCESS;

	CConnect* pConnect = new CConnect(InputHandle);
	if (pConnect != NULL)
	{
		if ((rc = pConnect->initialize()) == SQL_SUCCESS)
			*OutputHandle = (SQLHANDLE)pConnect;
		else
		{
			delete pConnect;
			rc = SQL_ERROR;
		}
	}
	else
	{
		setDiagRec(DRIVER_ERROR, IDS_HY_001);
		rc = SQL_ERROR;
	}
	return rc;
}

