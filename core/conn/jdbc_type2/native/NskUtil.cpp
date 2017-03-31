/*************************************************************************
*
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
**************************************************************************/
//
// MODULE: NskUtil.cpp
//
//
// PURPOSE: Implements the common functions 
//
/**************************************************************************/

#include <windows.h>
#ifdef NSK_PLATFORM
	#include <sqlWin.h>
#else
	#include <sql.h>
#endif
#include <sqlext.h>
#include "SrvrCommon.h"
#include "SrvrFunctions.h"
#include "SrvrKds.h"
#include "SqlInterface.h"
#include "CommonDiags.h"
#include "Debug.h"

/* Code for NSK */

#ifdef NSK_PLATFORM

#include <feerrors.h>
#include "NskUtil.h"

char systemNm[10] = {0};
char systemCatalogNm[MAX_DBNAME_LEN+1] = {0};

BOOL envGetCatalogName (
				  char *guardianNm //In
				, char *catalogNm //Out
				)
{
	FUNCTION_ENTRY("envGetCatalogName",("guardianNm=%s, catalogNm=0x%08x",
		DebugString(guardianNm),
		catalogNm));
	short	type_info[5];
	short	item_list[2] = {84,85};        //84 SQL Catalog Name length & 85 SQL Catalog name.
	short	result[2 + (EXT_FILENAME_LEN / 2) + 1];
	short	fileCode;
	short	errorCode;
	char	temp[MAX_DBNAME_LEN+1];
	unsigned int i,j,k;

	errorCode = FILE_GETINFOLISTBYNAME_(guardianNm,(short)strlen(guardianNm),item_list,
					(short)(sizeof(item_list)/sizeof(short)),result,sizeof(result));

	if(errorCode != FEOK)
		FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - FILE_GETINFOLISTBYNAME_() failed"));
	j = 0;
	k = 0;
	temp[j] = '\0';
	strncpy(temp, (char *)&result[1], result[0]);
	temp[result[0]] = '\0';
	for (i = 0; i < strlen(temp); i++)
	{
		catalogNm[j++] = temp[i];
		if (temp[i] == '.')
		{
			if (k == 1) catalogNm[j++] = '"';
			k++;
		}
	}
	catalogNm[j++] = '"';
	catalogNm[j] = '\0';
	FUNCTION_RETURN_NUMERIC(TRUE,("TRUE - catalogNm=%s",catalogNm));
}


BOOL envGetCatalogsTable(char *catalogsTableNm)
{
	FUNCTION_ENTRY("envGetCatalogsTable",("catalogsTableNm=0x%08x",
		catalogsTableNm));
	char	tableNm[EXT_FILENAME_LEN];
	short	tableNmLen;
	short	type_info[5];
	short	fileCode;
	short	errorCode, errorCodeFinish;
	short	searchId, resolveLvl;
	short	deviceType = 3;		/* restricts search to actual diskfiles */
	char	searchPatternStr[EXT_FILENAME_LEN];
	short	searchPatternLen;
	short	returnLen;
	short	entity[5];	// for FILENAME_FINDNEXT_

	if (systemNm[0] == '\0')
		GetSystemNm(systemNm);

	tableNmLen = sprintf(catalogsTableNm, "%s.$SYSTEM.SQL.CATALOGS", systemNm);

	// See if CATALOGS exists and has filecode 571. If so, we're done.

	errorCode = FILE_GETINFOBYNAME_(catalogsTableNm, tableNmLen, type_info);
	if (errorCode == FEOK &&		// call went okay
		type_info[0] == 3 &&		// device is disk
		type_info[2] >  0 &&		// file is a SQL file
		type_info[4] == 571)		// correct filecode for SQL table CATALOGS
	{
		catalogsTableNm[tableNmLen] = '\0';
		DEBUG_OUT(DEBUG_LEVEL_DATA,("MP Catalog does exist on system"));

		// found SQL system catalog table Nm
		FUNCTION_RETURN_NUMERIC(TRUE,("TRUE - catalogsTableNm=%s",catalogsTableNm));
	}

	// Since the default \n.$SYSTEM.SQL.CATALOGS failed to be the SQL catalog
	// need to scan all volumes.
	searchPatternLen = sprintf(searchPatternStr, "%s.$*.SQL.CATALOGS", systemNm);
	resolveLvl = -1; // VolumeNm

	errorCode = FILENAME_FINDSTART_(&searchId,
									searchPatternStr,
									searchPatternLen,
									resolveLvl,
									deviceType);	// disk device only

	if(errorCode == FEEOF)
	{
		FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - FILENAME_FINDSTART_() failed"));
	}

	fileCode = 0;
	errorCode = 0;

#ifdef _LP64
	while((errorCode = FILENAME_FINDNEXT64_(searchId, tableNm, tableNmLen, &returnLen, entity)) != 1)
#else
		while((errorCode = FILENAME_FINDNEXT_(searchId, tableNm, tableNmLen, &returnLen, entity)) != 1)
#endif
	{
		fileCode = entity[4];
		if (errorCode == FEOK && fileCode == 571)
		{
			tableNm[returnLen] = '\0';
			strcpy(catalogsTableNm,tableNm);
			errorCodeFinish = FILENAME_FINDFINISH_(searchId);
			FUNCTION_RETURN_NUMERIC(TRUE,("TRUE - catalogsTableNm=%s",catalogsTableNm));
		}
	}

	errorCodeFinish = FILENAME_FINDFINISH_(searchId);

	// if FILENAME_FINDNEXT_() read to: End-of-File (FEEOF)
	// without finding MP Catalog, MP is not present on system.
	// Return TRUE so as to not throw an unwanted exception.
	// Error code check on FINDNEXT_ has to be performed after FINDFINISH_.
	// FILENAME_FINDFINISH_ releases resources that were reserved
	// for a search that was previously initiated by FILENAME_FINDSTART_
	// BTW: 'MP not installed' is signified by string '\0' in
	// the first character position in array catalogsTableNm[].
	// catalogsTableNm[] is stored srvrGlobal->NskSystemCatalogName[]
	// See Java_org_apache_trafodion_jdbc_t2_SQLMXDriver_SQLMXInitialize().
	if(errorCode == FEEOF)
	{
		catalogsTableNm[0] = '\0';
		DEBUG_OUT(DEBUG_LEVEL_DATA,("MP Catalog does NOT exist on system"));
		FUNCTION_RETURN_NUMERIC(TRUE,("TRUE - MP Catalog Table does not exist"));
	}
	FUNCTION_RETURN_NUMERIC(FALSE,("FALSE"));
}

// Capture MP system catalog name
BOOL envGetSystemCatalogName (char *catalogNm)
{
	FUNCTION_ENTRY("envGetSystemCatalogName",("catalogNm=0x%08x",
		catalogNm));

	char catalogsTableNm[EXT_FILENAME_LEN+1];
	char *mpCatalogNm;
	if (systemCatalogNm[0] == '\0')
	{
		// Get the fully-qualified CATALOGS table name
		if(envGetCatalogsTable(catalogsTableNm) != TRUE)
		{
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - envGetCatalogsTable() failed"));
		}

		if ((mpCatalogNm = getenv("MP_SYSTEM_CATALOG")) == NULL)
		{
			// if MP env variable == NULL
			// and MP table was not detected
			// then set systemCatlogNm to '\0'
			// else strip catalog name and assign to systemCatalogNm.
			if(catalogsTableNm[0] == '\0')
			{
				systemCatalogNm[0] = '\0';
			}
			else if (envGetCatalogName(catalogsTableNm, systemCatalogNm) != TRUE)
			{
				FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - envGetCatalogName() failed"));
			}
		}
		else
		{
			strcpy(systemCatalogNm, mpCatalogNm);
		}
	}

	strcpy (catalogNm, systemCatalogNm);
	FUNCTION_RETURN_NUMERIC(TRUE,("TRUE - catalogNm=%s",catalogNm));
}

// Capture MX system catalog name
BOOL envGetMXSystemCatalogName (char *catalogNm)
{
	FUNCTION_ENTRY("envGetMXSystemCatalogName",("catalogNm=0x%08x",
		catalogNm));
	if ( (systemNm[0] == '\0') && (!GetSystemNm(systemNm)) )
		FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - systemNm blank or GetSystemNm() failed"));

	strcpy(catalogNm,CLUSTER_NAME);
	strcat(catalogNm,systemNm+1); // skip '\' in the system name
	FUNCTION_RETURN_NUMERIC(TRUE,("TRUE - catalogNm=%s",catalogNm));
}

BOOL envGetSQLType (char *guardianNm, char *SQLType)
{
	FUNCTION_ENTRY("envGetSQLType",("guardianNm=%s, SQLType=0x%08x",
		DebugString(guardianNm),
		SQLType));
	short	item_list[1] = {40};  // 40 SQL types. For disk objects:
								  //	0 Unstructured or Enscribe file
								  //	2 SQL table
								  //	4 SQL index
								  //	5 SQL protection view
								  //	7 SQL shorthand view
	short	result[2 + 1];
	short	fileCode;
	short	errorCode;
	unsigned int i,j,k;

	errorCode = FILE_GETINFOLISTBYNAME_(guardianNm,(short)strlen(guardianNm),item_list,
					(short)(sizeof(item_list)/sizeof(short)),result,sizeof(result));
	if(errorCode != FEOK)
		FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - FILE_GETINFOLISTBYNAME_() failed"));
	_itoa(result[0], SQLType, 10);
	FUNCTION_RETURN_NUMERIC(TRUE,("TRUE - SQLType=%s",SQLType));
}

BOOL GetSystemNm (char *systemNm)
{
	FUNCTION_ENTRY("GetSystemNm",("systemNm=0x%08x",
		systemNm));
	short errorCode;
	char  sysNm[8];
	short sysNmLen;
	short processHandle[10];
	for(int i=0;i<10;i++)
		processHandle[i]=0;
	
	PROCESSHANDLE_GETMINE_(processHandle);
	errorCode = PROCESSHANDLE_DECOMPOSE_(processHandle
									   ,/* cpu */
									   ,/* pin */
									   ,/* nodenumber */
									   ,sysNm
									   ,(short)sizeof(sysNm)
									   ,&sysNmLen);
	if (errorCode == FEOK)
	{
	  memcpy(systemNm, sysNm, sysNmLen);
	  systemNm[sysNmLen] = '\0';
	  FUNCTION_RETURN_NUMERIC(TRUE,("TRUE - systemNm=%s",systemNm));
	}

	FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - PROCESSHANDLE_DECOMPOSE_() failed"));
}

void translateShortAnsitoNSKFormat(
		char *shortansiNm, // In
		char *catalogNm   // Out
	)
{
	unsigned int i,j,k;
	char temp[128];

	j = 0;
	k = 0;
	temp[j] = '\0';
	temp[j] = '\\';
	for (i = 0; i < strlen(shortansiNm); i++)
	{
		if ((shortansiNm[i] == '_') && (k < 2))
		{
			temp[++j] = '.';
			if (k == 0)
				temp[++j] = '$';
			k++;
		}
		else
		{
			temp[++j] = shortansiNm[i];
		}
	}
	temp[++j] = '\0';
	strcpy(catalogNm,temp);
}

void translateNSKtoShortAnsiFormat(
		char *shortansiNm, // In
		char *catalogNm   // Out
	)
{
	unsigned int i,j,k;
	char temp[128];

	j = 0;
	k = 0;
	temp[j] = '\0';
	for (i = 0; i < strlen(catalogNm); i++)
	{
		if ((catalogNm[i] != '\\') && (catalogNm[i] != '$') && (catalogNm[i] != '"'))
		{
			if (catalogNm[i] == '.')
			{
				temp[j++] = '_';
			}
			else
			{
				temp[j++] = catalogNm[i];
			}
		}
	}
	temp[j] = '\0';
	strcpy(shortansiNm,temp);
}

#endif //NSK_PLATFORM
