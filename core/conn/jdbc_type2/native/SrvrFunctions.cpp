/**************************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1998-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**************************************************************************/
//
// MODULE: SrvrFunctions.cpp
//
// PURPOSE: Implements System Meta-Data API's Querry Formation


#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include "srvrCommon.h"
#include "SrvrFunctions.h"
#include "Debug.h"

void AssembleSqlString(SMD_SELECT_TABLE *smdSelectTable, const char *catalogNm, 
										const char *locationNm, char *sqlString)
{
	FUNCTION_ENTRY("AssembleSqlString",("..."));

	SMD_SELECT_TABLE *lc_smdSelectTable;

	lc_smdSelectTable = smdSelectTable;

	sqlString[0] = '\0';
	while(lc_smdSelectTable->lineID != END_OF_TABLE )
	{
		switch (lc_smdSelectTable->lineID) {
		case STRING_TYPE:
			strcat(sqlString, lc_smdSelectTable->textLine);
			break;
		case TABLE_TYPE:
			strcat(sqlString, smdTablesList[lc_smdSelectTable->index]);
			break;
		case SCHEMA_TYPE:
			strcat(sqlString, smdSchemaList[lc_smdSelectTable->index]);
			break;
		case CATALOG_TYPE:
			if( catalogNm != NULL && catalogNm[ 0 ] != '\0' )
			{
				strcat(sqlString, "trim('");
				strcat(sqlString, catalogNm);
				//strcat(sqlString, "'),"); // shirley 11/13/97
				strcat(sqlString, "') TABLE_QUALIFIER,");  // shirley 11/13/97
			}
			else
				//strcat(sqlString, "'',"); // shirley 11/13/97
				strcat(sqlString, "'' TABLE_QUALIFIER,"); // shirley 11/13/97
			break;
		case CATALOG_TYPE_DOT:
			if( catalogNm != NULL && catalogNm[ 0 ] != '\0' )
			{
				strcat(sqlString, catalogNm);
				strcat(sqlString, ".");
			}
			break;
		case LOCATION_TYPE:
			if( locationNm != NULL && locationNm[ 0 ] != '\0' )
			{
				strcat(sqlString, " LOCATION "); 
				strcat(sqlString, locationNm);
			}
			break;
#ifdef NSK_PLATFORM
		case NSK_CLUSTER_TYPE:
			if( catalogNm != NULL && catalogNm[ 0 ] != '\0' )
			{
				strcat(sqlString, catalogNm);
				strcat(sqlString, ".");
			}
			break;
		case NSK_SHORTANSI_CATALOG_NAME:
			catalogsTableNm[0] = '\0';
			translateNSKtoShortAnsiFormat(&catalogsTableNm[0], (char *)catalogNm);
			strcat(sqlString,catalogsTableNm);
			break;
		case NSK_CATALOGS_TABLE_LOCATION:
			char	temp[MAX_DBNAME_LEN+1];
			unsigned int i,j,k;
			catalogsTableNm[0] = '\0';
			temp[0] = '\0';
			envGetCatalogsTable(temp);
		
			j = 0;
			k = 0;
			for (i = 0; i < strlen(temp); i++)
			{
				if (temp[i] == '.')
				{
					if (k == 1)
					{
						catalogsTableNm[j++] = temp[i];
						catalogsTableNm[j++] = '"';
					}
					else if (k == 2)
					{
						catalogsTableNm[j++] = '"';
						catalogsTableNm[j++] = temp[i];
					}
					else
					{
						catalogsTableNm[j++] = temp[i];
					}
					k++;
				}
				else
				{
					catalogsTableNm[j++] = temp[i];
				}
			}
			catalogsTableNm[j] = '\0';
			
			strcat(sqlString, catalogsTableNm);
			break;
#endif
		default:
			break;
		}
		lc_smdSelectTable++;
	}
	FUNCTION_RETURN_VOID((NULL));
} // End of Function


SMD_QUERY_TABLE *getSmdSelectTable (SMD_QUERY_TABLE *queryTable, const char *stmtLabel, short *sqlStmtType)
{
	FUNCTION_ENTRY("getSmdSelectTable",("queryTable=%08x, stmtLabel=%s, sqlStmtType=0x%08x",
		queryTable,
		DebugString(stmtLabel),
		sqlStmtType));
	short i;

	for (i = 0; ; i++)
	{
		if (queryTable[i].stmtLabel == NULL)
			break;
		if (strcmp(queryTable[i].stmtLabel, stmtLabel) == 0)
		{
			*sqlStmtType = queryTable[i].sqlStmtType;
			FUNCTION_RETURN_PTR(&(queryTable[i]),("queryTable[%d]",i));
		}
	}
	FUNCTION_RETURN_PTR(NULL,(NULL));
} 
