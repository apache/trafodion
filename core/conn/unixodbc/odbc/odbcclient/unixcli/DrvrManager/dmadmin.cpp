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
/*************************************************************************
**************************************************************************/
//
#include "neofunc.h"
#include "cenv.h"
#include "cconnect.h"
#include "cstmt.h"
#include "cdesc.h"
#include "dmfunctions.h"
#include "drvrmanager.h"
#include "dmadmin.h"

using namespace ODBC;

#ifndef unixcli
static int
stricmp (const char *s1, const char *s2)
{
  int cmp;

  while (*s1)
    {
      if ((cmp = toupper (*s1) - toupper (*s2)) != 0)
	return cmp;
      s1++;
      s2++;
    }
  return (*s2) ? -1 : 0;
}

#endif
extern "C"
int SectSorter (const void *p1, const void *p2)
{
  char **s1 = (char **) p1;
  char **s2 = (char **) p2;

  return stricmp (*s1, *s2);
}

/* MXODSN File entries are assumed to be kept in UTF8  */

SQLRETURN SQL_API SQLDataSources(
     SQLHENV EnvironmentHandle,
     SQLUSMALLINT Direction,
     SQLCHAR *ServerName,
     SQLSMALLINT BufferLength1,
     SQLSMALLINT *NameLength1Ptr,
     SQLCHAR *Description,
     SQLSMALLINT BufferLength2,
     SQLSMALLINT *NameLength2Ptr)
{
	GENV (genv, EnvironmentHandle);
	char *path;
	char buf[1024];
	FILE *fp = NULL;
	int i;
	SQLUSMALLINT direction = Direction;
	static int cur_entry = -1;
	static int num_entries = 0;
	static char **sect = NULL;

	ODBC_LOCK ();
	if (!IS_VALID_HENV (genv))
    {
		ODBC_UNLOCK ();
		return SQL_INVALID_HANDLE;
    }
	CLEAR_ERRORS (genv);

	/* check argument */
	if (BufferLength1 < 0 || BufferLength2 < 0)
    {
		PUSHSQLERR (genv, IDS_S1_090);
		RETURNCODE (genv, SQL_ERROR);
		ODBC_UNLOCK ();
		return SQL_ERROR;
    }
	switch(direction)
	{
		case SQL_FETCH_FIRST:
		case SQL_FETCH_NEXT:
		case SQL_FETCH_FIRST_USER:
		case SQL_FETCH_FIRST_SYSTEM:
			break;
		default:
			PUSHSQLERR (genv, IDS_S1_103);
			RETURNCODE (genv, SQL_ERROR);
			ODBC_UNLOCK ();
			return SQL_ERROR;
    }

	if (cur_entry < 0 || direction == SQL_FETCH_FIRST || direction == SQL_FETCH_FIRST_USER || direction == SQL_FETCH_FIRST_SYSTEM)
    {
		if (direction == SQL_FETCH_NEXT) direction = SQL_FETCH_FIRST;
		cur_entry = 0;
		num_entries = 0;
		/*
		*  Free old section list
		*/
		if (sect)
		{
			for (i = 0; i < MAX_ENTRIES; i++)
				if (sect[i]) free (sect[i]);
			free (sect);
		}
		if ((sect = (char **) calloc (MAX_ENTRIES, sizeof (char *))) == NULL)
		{
			if (fp)	fclose (fp);
			PUSHSQLERR (genv, IDS_HY_001);
			RETURNCODE (genv, SQL_ERROR);
			ODBC_UNLOCK ();
			return SQL_ERROR;
		}

		for (i = 0; i < 2; i++)
		{
			if (i == 0)
			{
				/* 
				*	Open user odbc.ini file
				*/
				if (direction == SQL_FETCH_FIRST || direction == SQL_FETCH_FIRST_USER)
				{
					strncpy(buf,getUserFilePath(USER_DSNFILE),sizeof(buf));
					path = buf;
				}
				else
					path = (char *) SYSTEM_DSNFILE;

				if (path[0] == '\0') continue;
#ifndef unixcli
				if ((fp = fopen_guardian (path, "r")) == NULL) continue;
#else
				if ((fp = fopen (path, "r")) == NULL) continue;
#endif
			}
			else
			{
				if (Direction == SQL_FETCH_FIRST)
				{
					path = (char *) SYSTEM_DSNFILE;
#ifndef unixcli
					if ((fp = fopen_guardian (path, "r")) == NULL)
						break;
#else
					if ((fp = fopen (path, "r")) == NULL)
						break;
#endif
				}
				else
					break;
			}

			/* Finds the section heading [ODBC Data Sources] */
			while (1)
			{
				char *str;
				str = fgets (buf, sizeof (buf), fp);
				if (str == NULL)
				break;
				if (strncmp (str, SECT1, 19) == 0)
						break;
			}
			/*
			*  Build a dynamic list of sections
			*/
			while (1)
			{
				char *str, *p;
				str = fgets (buf, sizeof (buf), fp);
				if (str == NULL || *str == '[')
					break;
				if (*str == '\n')
					continue;
				for (p = str; *p; p++)
				{
//					if (*p == '=' || *p == '\n')
					if (*p == '\n')
					{
						*p = '\0';
						/*
						*  Trim whitespace from the right
						*/
						for (; p > str && (*(p - 1) == ' ' || *(p - 1) == '\t');)
							*--p = '\0';
						break;
					}
				}
				/* Add this section to the comma separated list */
				if (num_entries >= MAX_ENTRIES)
					break;		/* Skip the rest */
				
				sect[num_entries++] = (char *) _strdup (str);
			}
			fclose (fp);
		}
		/*
		*  Sort all entries so we can present a nice list
		*/
		if (num_entries > 1)
			qsort (sect, num_entries, sizeof (char *), SectSorter);
	}
	/*
	*  Try to get to the next item
	*/
	if (cur_entry >= num_entries)
    {
		cur_entry = 0;		/* Next time, start all over again */
		RETURNCODE (genv, SQL_NO_DATA_FOUND);
		ODBC_UNLOCK ();
		return SQL_NO_DATA_FOUND;
    }
	/*
	*  Copy DSN information 
	*/
	char* desc = strchr(sect[cur_entry], '=');
	if (desc != NULL)
	{
		*desc = '\0';
		// Trim whitespace from the right
		for (char* p=sect[cur_entry]; p < desc; ++p)
		{		
		   if (*p == ' ' || *p == '\t')	
		   {			
	                  *p = '\0';
			  break;
		   }
        	}
		strcpy((char *)ServerName, sect[cur_entry]);
		++desc;
		if ( Description != NULL)
		{
			((char *)Description)[BufferLength2 - 1] = '\0';
			strncpy((char *)Description, desc, BufferLength2-1);
		}
 	}
	else
	{
		strncpy ((char *)ServerName, sect[cur_entry], BufferLength1);
		*((char *)Description) = '\0';
	}
	if (NameLength1Ptr)
		*NameLength1Ptr = strlen (sect[cur_entry]);
	/*
	*  And find the type description that goes with this entry
	*/
	//	fun_getkeyvalbydsn (SECT3, strlen (SECT3), sect[cur_entry], (char *)Description, BufferLength2);

	if (NameLength2Ptr)
		*NameLength2Ptr = strlen((char *)Description);
	/*
	*  Next record
	*/
	cur_entry++;
	RETURNCODE (genv, SQL_SUCCESS);
	ODBC_UNLOCK ();
	return SQL_SUCCESS;
}


SQLRETURN SQL_API SQLDataSourcesW(
     SQLHENV EnvironmentHandle,
     SQLUSMALLINT Direction,
     SQLWCHAR *ServerName,
     SQLSMALLINT BufferLength1,
     SQLSMALLINT *NameLength1Ptr,
     SQLWCHAR *Description,
     SQLSMALLINT BufferLength2,
     SQLSMALLINT *NameLength2Ptr)
{
	GENV (genv, EnvironmentHandle);
	char *path;
	char buf[1024];
	FILE *fp = NULL;
	int i;
	SQLUSMALLINT direction = Direction;
	static int cur_entry = -1;
	static int num_entries = 0;
	static char **sect = NULL;
	char tmpServerName[1024];
	char tmpDescription[2048];

	tmpDescription[0] = '\0';
	ODBC_LOCK ();
	if (!IS_VALID_HENV (genv))
    {
		ODBC_UNLOCK ();
		return SQL_INVALID_HANDLE;
    }
	CLEAR_ERRORS (genv);

	/* check argument */
	if (BufferLength1 < 0 || BufferLength2 < 0)
    {
		PUSHSQLERR (genv, IDS_S1_090);
		RETURNCODE (genv, SQL_ERROR);
		ODBC_UNLOCK ();
		return SQL_ERROR;
    }
	switch(direction)
	{
		case SQL_FETCH_FIRST:
		case SQL_FETCH_NEXT:
		case SQL_FETCH_FIRST_USER:
		case SQL_FETCH_FIRST_SYSTEM:
			break;
		default:
			PUSHSQLERR (genv, IDS_S1_103);
			RETURNCODE (genv, SQL_ERROR);
			ODBC_UNLOCK ();
			return SQL_ERROR;
    }

	if (cur_entry < 0 || direction == SQL_FETCH_FIRST || direction == SQL_FETCH_FIRST_USER || direction == SQL_FETCH_FIRST_SYSTEM)
    {
		if (direction == SQL_FETCH_NEXT) direction = SQL_FETCH_FIRST;
		cur_entry = 0;
		num_entries = 0;
		/*
		*  Free old section list
		*/
		if (sect)
		{
			for (i = 0; i < MAX_ENTRIES; i++)
				if (sect[i]) free (sect[i]);
			free (sect);
		}
		if ((sect = (char **) calloc (MAX_ENTRIES, sizeof (char *))) == NULL)
		{
			if (fp)	fclose (fp);
			PUSHSQLERR (genv, IDS_HY_001);
			RETURNCODE (genv, SQL_ERROR);
			ODBC_UNLOCK ();
			return SQL_ERROR;
		}

		for (i = 0; i < 2; i++)
		{
			if (i == 0)
			{
				/* 
				*	Open user odbc.ini file
				*/
				if (direction == SQL_FETCH_FIRST || direction == SQL_FETCH_FIRST_USER)
				{
					strncpy(buf,getUserFilePath(USER_DSNFILE),sizeof(buf));
					path = buf;
				}
				else
					path = (char *) SYSTEM_DSNFILE;

				if (path[0] == '\0') continue;
#ifndef unixcli
				if ((fp = fopen_guardian (path, "r")) == NULL) continue;
#else
				if ((fp = fopen (path, "r")) == NULL) continue;
#endif
			}
			else
			{
				if (Direction == SQL_FETCH_FIRST)
				{
					path = (char *) SYSTEM_DSNFILE;
#ifndef unixcli
					if ((fp = fopen_guardian (path, "r")) == NULL)
						break;
#else
					if ((fp = fopen (path, "r")) == NULL)
						break;
#endif
				}
				else
					break;
			}

			/* Finds the section heading [ODBC Data Sources] */
			while (1)
			{
				char *str;
				str = fgets (buf, sizeof (buf), fp);
				if (str == NULL)
				break;
				if (strncmp (str, SECT1, 19) == 0)
						break;
			}
			/*
			*  Build a dynamic list of sections
			*/
			while (1)
			{
				char *str, *p;
				str = fgets (buf, sizeof (buf), fp);
				if (str == NULL || *str == '[')
					break;
				if (*str == '\n')
					continue;
				for (p = str; *p; p++)
				{
//					if (*p == '=' || *p == '\n')
					if (*p == '\n')
					{
						*p = '\0';
						/*
						*  Trim whitespace from the right
						*/
						for (; p > str && (*(p - 1) == ' ' || *(p - 1) == '\t');)
							*--p = '\0';
						break;
					}
				}
				/* Add this section to the comma separated list */
				if (num_entries >= MAX_ENTRIES)
					break;		/* Skip the rest */
				
				sect[num_entries++] = (char *) _strdup (str);
			}
			fclose (fp);
		}
		/*
		*  Sort all entries so we can present a nice list
		*/
		if (num_entries > 1)
			qsort (sect, num_entries, sizeof (char *), SectSorter);
	}
	/*
	*  Try to get to the next item
	*/
	if (cur_entry >= num_entries)
    {
		cur_entry = 0;		/* Next time, start all over again */
		RETURNCODE (genv, SQL_NO_DATA_FOUND);
		ODBC_UNLOCK ();
		return SQL_NO_DATA_FOUND;
    }
	/*
	*  Copy DSN information 
	*/
	char* desc = strchr(sect[cur_entry], '=');
	if (desc != NULL)
	{
		*desc = '\0';
		// Trim whitespace from the right
		for (char* p=sect[cur_entry]; p < desc; ++p)
		{		
		   if (*p == ' ' || *p == '\t')	
		   {			
	                  *p = '\0';
			  break;
		   }
        	}
		strcpy((char *)tmpServerName, sect[cur_entry]);
		++desc;
		if ( Description != NULL)
		{
			((char *)tmpDescription)[BufferLength2 - 1] = '\0';
			strncpy((char *)tmpDescription, desc, BufferLength2-1);
		}
 	}
	else
	{
		strncpy ((char *)tmpServerName, sect[cur_entry], BufferLength1);
		*((char *)tmpDescription) = '\0';
	}
	//Translate the tmpServerName and Description if the application is UTF16
	if(gDrvrGlobal.ICUConv.isAppUTF16())
	{
		if(ServerName != NULL)
		{
			int transLen=0;
			char errorMsg[MAX_TRANSLATE_ERROR_MSG_LEN];
			//convert from utf8 to utf16
			if((gDrvrGlobal.ICUConv.UTF8ToWChar(tmpServerName, strlen(tmpServerName), (UChar*)ServerName, 
									BufferLength1, &transLen, errorMsg)) == SQL_ERROR)
			{
				PUSHSQLERR (genv, IDS_S1_090);
				RETURNCODE (genv, SQL_ERROR);
				ODBC_UNLOCK ();
				return SQL_ERROR;
			}
       		else
	       		*ServerName = UCharNull;
		}
		if(Description != NULL)
		{
			if(tmpDescription[0] != '\0')
			{
				int transLen=0;
				char errorMsg[MAX_TRANSLATE_ERROR_MSG_LEN];
				//convert from utf8 to utf16
				if((gDrvrGlobal.ICUConv.UTF8ToWChar(tmpDescription, strlen(tmpDescription), (UChar*)Description, 
										BufferLength2, &transLen, errorMsg)) == SQL_ERROR)
				{
					PUSHSQLERR (genv, IDS_S1_090);
					RETURNCODE (genv, SQL_ERROR);
					ODBC_UNLOCK ();
					return SQL_ERROR;
				}
			}
	       	else
	       		*Description = UCharNull;
		}
	}
	else
	{
		strcpy((char*)ServerName, tmpServerName);
		if (NameLength1Ptr)
			*NameLength1Ptr = strlen (sect[cur_entry]);
	/*
	*  And find the type description that goes with this entry
	*/
	//	fun_getkeyvalbydsn (SECT3, strlen (SECT3), sect[cur_entry], (char *)Description, BufferLength2);

		strcpy((char*)Description, tmpDescription);
		if (NameLength2Ptr)
			*NameLength2Ptr = strlen((char *)Description);
	}
	/*
	*  Next record
	*/
	cur_entry++;
	RETURNCODE (genv, SQL_SUCCESS);
	ODBC_UNLOCK ();
	return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLDrivers(
     SQLHENV EnvironmentHandle,
     SQLUSMALLINT Direction,
     SQLCHAR *DriverDescription,
     SQLSMALLINT BufferLength1,
     SQLSMALLINT *DescriptionLengthPtr,
     SQLCHAR *DriverAttributes,
     SQLSMALLINT BufferLength2,
     SQLSMALLINT *AttributesLengthPtr)
{
	SQLRETURN rc;
	GENV (genv, EnvironmentHandle);

	ODBC_LOCK ();
	if (!IS_VALID_HENV (genv))
    {
		ODBC_UNLOCK ();
		return SQL_INVALID_HANDLE;
    }
	CLEAR_ERRORS (genv);
	if (BufferLength1 < 0 || BufferLength2 < 0 || BufferLength2 == 1)
    {
		PUSHSQLERR (genv, IDS_S1_090);
		RETURNCODE (genv, SQL_ERROR);
		ODBC_UNLOCK ();
		return SQL_ERROR;
    }
	if (Direction != SQL_FETCH_FIRST && Direction != SQL_FETCH_NEXT)
    {
		PUSHSQLERR (genv, IDS_S1_103);
		RETURNCODE (genv, SQL_ERROR);
		ODBC_UNLOCK ();
		return SQL_ERROR;
    }
	if (!DriverDescription || !DriverAttributes || !BufferLength1 || !BufferLength2)
    {
		PUSHSQLERR (genv, IDS_01_004);
		RETURNCODE (genv, SQL_SUCCESS_WITH_INFO);
		ODBC_UNLOCK ();
		return SQL_SUCCESS_WITH_INFO;
    }
	if (Direction == SQL_FETCH_FIRST)
	{
		strncpy((char *)DriverDescription,ODBC_DRIVER,BufferLength1);
		DriverDescription[BufferLength1 - 1] = '\0';
		if(DescriptionLengthPtr != NULL)
			*DescriptionLengthPtr = strlen((char *)DriverDescription);
		DriverAttributes[0] = '\0';
		if(AttributesLengthPtr != NULL)
			*AttributesLengthPtr = strlen((char *)DriverAttributes);
		rc = SQL_SUCCESS;
	}
	else
	{
		rc = SQL_NO_DATA_FOUND;
	}
	RETURNCODE (genv, rc);
	ODBC_UNLOCK ();
	return rc;
}

SQLRETURN SQL_API SQLDriversW(
     SQLHENV EnvironmentHandle,
     SQLUSMALLINT Direction,
     SQLWCHAR *DriverDescription,
     SQLSMALLINT BufferLength1,
     SQLSMALLINT *DescriptionLengthPtr,
     SQLWCHAR *DriverAttributes,
     SQLSMALLINT BufferLength2,
     SQLSMALLINT *AttributesLengthPtr)
{
	SQLRETURN rc;
	GENV (genv, EnvironmentHandle);

	ODBC_LOCK ();
	if (!IS_VALID_HENV (genv))
    {
		ODBC_UNLOCK ();
		return SQL_INVALID_HANDLE;
    }
	CLEAR_ERRORS (genv);
	if (BufferLength1 < 0 || BufferLength2 < 0 || BufferLength2 == 1)
    {
		PUSHSQLERR (genv, IDS_S1_090);
		RETURNCODE (genv, SQL_ERROR);
		ODBC_UNLOCK ();
		return SQL_ERROR;
    }
	if (Direction != SQL_FETCH_FIRST && Direction != SQL_FETCH_NEXT)
    {
		PUSHSQLERR (genv, IDS_S1_103);
		RETURNCODE (genv, SQL_ERROR);
		ODBC_UNLOCK ();
		return SQL_ERROR;
    }
	if (!DriverDescription || !DriverAttributes || !BufferLength1 || !BufferLength2)
    {
		PUSHSQLERR (genv, IDS_01_004);
		RETURNCODE (genv, SQL_SUCCESS_WITH_INFO);
		ODBC_UNLOCK ();
		return SQL_SUCCESS_WITH_INFO;
    }
	if (Direction == SQL_FETCH_FIRST)
	{
		if(gDrvrGlobal.ICUConv.isAppUTF16())
		{
			int transLen=0;
			char errorMsg[MAX_TRANSLATE_ERROR_MSG_LEN];
			//convert from utf8 to utf16
			if((gDrvrGlobal.ICUConv.UTF8ToWChar(ODBC_DRIVER, strlen(ODBC_DRIVER), (UChar*)DriverDescription, 
									BufferLength1, &transLen, errorMsg)) == SQL_ERROR)
			{
				PUSHSQLERR (genv, IDS_S1_090);
				RETURNCODE (genv, SQL_ERROR);
				ODBC_UNLOCK ();
				return SQL_ERROR;
			}
		}
		else
		{
			strncpy((char *)DriverDescription,ODBC_DRIVER,BufferLength1);
			DriverDescription[BufferLength1 - 1] = '\0';
		}
		if(DescriptionLengthPtr != NULL)
		{
			if(gDrvrGlobal.ICUConv.isAppUTF16())	
				*DescriptionLengthPtr = u_strlen((UChar *)DriverDescription);
			else	
				*DescriptionLengthPtr = strlen((char *)DriverDescription);
		}
		if(gDrvrGlobal.ICUConv.isAppUTF16())
			*DriverAttributes = UCharNull;
		else
			DriverAttributes[0] = '\0';
		if(AttributesLengthPtr != NULL)
			*AttributesLengthPtr = strlen((char *)DriverAttributes);
		rc = SQL_SUCCESS;
	}
	else
	{
		rc = SQL_NO_DATA_FOUND;
	}
	RETURNCODE (genv, rc);
	ODBC_UNLOCK ();
	return rc;
}

