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

#include <windows.h>
#include <stdio.h>
#include "EventMsgs.h"
#ifdef NSK_PLATFORM
#include <cextdecs.h>
#include <fs\feerrors.h>
#include "zspic.h"
#include "zemsc.h"
#include "zinic.h"
#include "zmxoc.h"
#include "tdm_odbcsrvrmsg.h"
#include "odbcmxProductName.h"
#endif

#define EVENTMSG_ERROR_MSGBOX_TITLE	"ODBC/MX Event Message Handling Error"

// These defines are extracted from regvalues.h in order to eliminate
// unnecessary dependencies.
#define MAX_REGKEY_PATH_LEN		1000
#define	MAX_SQL_ID_LEN			128
#define MAX_NODE_CNT			16
#define DEFAULTNODECOUNT		-1


// strings used to build RegValues lookup keys 
#define _NONSTOP_PATH_KEY "SOFTWARE\\TANDEM\\NonStop-Cluster"
#define _CLUSTER_NAME_VAL_NM	"Cluster-Name"
#define _TANDEM_ID_VAL_NM		"Tandem-Id"
#define _BOOT_CONFIG_VAL_NM		"Boot-Config"
#define _CLUSTER_KEY			"CLUSTER"
#define _NODE_KEY				"NODE"
#define _NETBIOS_VAL_NM			"NetBiosName"

long FindClusterName(char *);
int FindMyNodeNum ( void );

		
ODBCMXEventMsg::ODBCMXEventMsg()
{

	PVOID lpMsgBuf;
	hModule = NULL;
	sendEventMsgPtr = NULL;
	ClusterName[0] = '\0';
	NodeId = -1;

#ifdef WIN32
	if ((hModule = LoadLibrary("tdm_odbcEvent.dll")) == NULL)
	{	
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL);
			/////////////////////////////////////////////////////////////////////////
			// Note: MessageBox call is a temporary solution for now.
			// We need to change this call later when ODBC is starting as a service.
			/////////////////////////////////////////////////////////////////////////
			MessageBox(NULL, (const char*) lpMsgBuf, EVENTMSG_ERROR_MSGBOX_TITLE, MB_OK);		
			LocalFree(lpMsgBuf);
	}
	else
	if ((sendEventMsgPtr = (SENDMSG_FNPTR)GetProcAddress(hModule,    // handle to DLL module
			"SendODBCEventMsg")) == NULL)
	{
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL);
			/////////////////////////////////////////////////////////////////////////
			// Note: MessageBox call is a temporary solution for now.
			// We need to change this call later when ODBC is starting as a service.
			/////////////////////////////////////////////////////////////////////////
			MessageBox(NULL, (const char*) lpMsgBuf, EVENTMSG_ERROR_MSGBOX_TITLE, MB_OK);		
			LocalFree(lpMsgBuf);
	}

	
	// Get Cluster Name and Node ID
	long retcode = FindClusterName(ClusterName);

	NodeId = FindMyNodeNum();
#endif
}

ODBCMXEventMsg::~ODBCMXEventMsg()
{
	// Unload the DLL
}

//////////////////////////////////////////////////////////////////////////
//
// Name:    SendEventMsg
//
// Purpose: Add a specified type of parameter to an event.
//          This logs a message with (any) token parameters previously 
//          specified by the Add* methods.
//
// Note:    Parameters in event messages are specified in the message 
//          template by using the form "%n", where "n" indicates the 
//          relative call to AddToken which supplies the value for the 
//          parameter.  Thus, the first AddToken call will provide the 
//          value for token %1, the second for token %2, etc.
//          The maximum number of tokens is defined in teventlog.h.
//          Token is a null-terminated string that becomes the parameter 
//          value.
//
// Input:   EventId - numeric value for the event.  This corresponds
//                    to the value defined by the message compiler for 
//                    the event.
//          EventLogType - The type of the event being logged
//                         (e.g., EVENTLOG_INFORMATION_TYPE, 
//                         EVENTLOG_WARNING_TYPE, and
//                         EVENTLOG_ERROR_TYPE)
//          Pid - Process ID
//          ComponentName - ODBC/MX Component (e.g., Association Server)
//          ObjectRef - Object Reference
//          nToken - Number of additional tokens
//
// Output:  None
//
//////////////////////////////////////////////////////////////////////////
#ifdef NSK_PLATFORM
#include <stdio.h>
#endif

void ODBCMXEventMsg::SendEventMsg(DWORD EventId, short EventLogType, DWORD Pid, char *ComponentName,
			char *ObjectRef, short nToken, ...)
{
	va_list marker;

	va_start( marker, nToken);
	
#ifdef WIN32
	if (hModule != NULL && sendEventMsgPtr != NULL)
	{
		(*sendEventMsgPtr)(EventId, EventLogType, Pid, ComponentName,
			ObjectRef, ClusterName, NodeId, nToken, marker);
	}
#else

	send_to_ems( EventId & 0x0FFFF, EventLogType, ComponentName, ObjectRef, nToken, marker);

#endif

}



//////////////////////////////////////////////////////////////////////////
//
// Name:    FindClusterName
//
// Purpose: Get the cluster name from the registry.
//
//////////////////////////////////////////////////////////////////////////

long FindClusterName(char *ClusterName)
{
#ifdef WIN32
	HKEY	hGlobalHandle;
	long	error;
	unsigned long	length;
	DWORD	valueType;
	
	ClusterName[0] = '\0';
	
	error = RegOpenKeyEx(	HKEY_LOCAL_MACHINE
						   ,_NONSTOP_PATH_KEY
						   ,0
						   ,KEY_READ
						   ,&hGlobalHandle );
	if (error != ERROR_SUCCESS) {
		return error;
	}
	
	// Get cluster name
	length = MAX_COMPUTERNAME_LENGTH + 1;
	error = RegQueryValueEx ( hGlobalHandle
							,_CLUSTER_NAME_VAL_NM
							,NULL
							,&valueType
							,(LPBYTE)ClusterName
							,(LPDWORD)&length );

	RegCloseKey( hGlobalHandle );
	
	return error;
#else
	short errorCode;
	char  sysNm[8];
	short sysNmLen;
	short processHandle[10];
	for(int i=0;i<10;i++)
		processHandle[i]=0;

	ClusterName[0] = '\0';
	
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
		memcpy(ClusterName, sysNm, sysNmLen);
		ClusterName[sysNmLen] = '\0';
	}
	else
	{
		strcpy(ClusterName, "UNKNOWN");
	}
	return ERROR_SUCCESS;
#endif
}


//////////////////////////////////////////////////////////////////////////
//
// Name:    ReadNodeNumInRegistry
//
// Purpose: Use the local computer name and search in the registry to
//          find its node number.
//
// Note:    The default node ID is -1 which will be shown as
//				Node ID: Unknown 
//          in the event message.
//
//////////////////////////////////////////////////////////////////////////

int ReadNodeNumInRegistry(char *LocalComputerName)
{	

	int		NodeCount = DEFAULTNODECOUNT;	
#ifdef WIN32
	DWORD	valueType;		
	HKEY	hGlobalHandle;		
	char	ValueData[MAX_SQL_ID_LEN + 1];	
	char	NetBiosName[MAX_COMPUTERNAME_LENGTH + 1];
	char	KeySearchPath[MAX_REGKEY_PATH_LEN + 1];
	char	tmpKeySearchPath[MAX_REGKEY_PATH_LEN + 1];
	unsigned long	length;
	long	error;
	

	error = RegOpenKeyEx(	HKEY_LOCAL_MACHINE
						   ,_NONSTOP_PATH_KEY
						   ,0
						   ,KEY_READ
						   ,&hGlobalHandle );
	if (error != ERROR_SUCCESS) {		
		return DEFAULTNODECOUNT;
	}
	length = sizeof(ValueData);
	error = RegQueryValueEx ( hGlobalHandle
	                       ,_BOOT_CONFIG_VAL_NM
						   ,NULL
						   ,&valueType
						   ,(LPBYTE)ValueData
						   ,(LPDWORD)&length );

	RegCloseKey( hGlobalHandle );

    if (error == ERROR_SUCCESS) 
		sprintf( KeySearchPath, "%s\\%s\\%s", _NONSTOP_PATH_KEY ,ValueData,_CLUSTER_KEY );
	else
		if (error == ERROR_FILE_NOT_FOUND)
			sprintf( KeySearchPath, "%s\\%s\\%s", _NONSTOP_PATH_KEY ,"Default-Config",_CLUSTER_KEY );
		else
			return DEFAULTNODECOUNT;

	error = RegOpenKeyEx(	HKEY_LOCAL_MACHINE
						   ,KeySearchPath
						   ,0
						   ,KEY_READ
						   ,&hGlobalHandle );

	if (error != ERROR_SUCCESS)		
		return DEFAULTNODECOUNT;

	RegCloseKey( hGlobalHandle );

	for (int i = 0; i < MAX_NODE_CNT ; i++) 
	{

		sprintf(tmpKeySearchPath, "%s\\Node%d", KeySearchPath,i );
		error = RegOpenKeyEx ( HKEY_LOCAL_MACHINE
	                       ,tmpKeySearchPath
						   ,0
						   ,KEY_READ
						   ,&hGlobalHandle );

		if (error != ERROR_SUCCESS) 
			if (error == ERROR_FILE_NOT_FOUND)
				continue;
			else
				return DEFAULTNODECOUNT;

		length = sizeof(NetBiosName);
	    error = RegQueryValueEx ( hGlobalHandle
	                       ,_NETBIOS_VAL_NM
						   ,NULL
						   ,&valueType
						   ,(LPBYTE)NetBiosName
						   ,&length );
		
		RegCloseKey( hGlobalHandle );

		if (error != ERROR_SUCCESS) 
			if (error == ERROR_FILE_NOT_FOUND)	
				NetBiosName[0] = '\0';
			else
				return DEFAULTNODECOUNT;
		

		if (strcmp((char*)NetBiosName,
			       (char*)LocalComputerName) == 0)   
		{
			NodeCount = i;
			break;
		}

	}
#else
	NodeCount = PROCESSORSTATUS() >> 16;
#endif
	return NodeCount;
}


//////////////////////////////////////////////////////////////////////////
//
// Name:    FindMyNodeNum
//
// Purpose: Get the local computer name and search in the registry to
//          find its node number.
//
//////////////////////////////////////////////////////////////////////////

int FindMyNodeNum ( void )
{
	char    LocalComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	unsigned long	dwMyNodeNmLen = MAX_COMPUTERNAME_LENGTH ;
	

	LocalComputerName[0] = '\0';	
	GetComputerName(LocalComputerName, &dwMyNodeNmLen);

	return ReadNodeNumInRegistry(LocalComputerName);
}

#ifdef NSK_PLATFORM
#pragma page "T8666 NOS : open_ems_name"
                                       
/*----------------------------------------------------------------------*/      
/* open_ems_name                                                  */      
/*----------------------------------------------------------------------*/      
void ODBCMXEventMsg::open_ems_name( char* collector)
{
	strncpy(ems_name, collector, sizeof(ems_name));
	ems_name[sizeof(ems_name)-1] = 0;
	open_ems();
}
#pragma page "T8666 NOS : open_ems"
                                       
/*----------------------------------------------------------------------*/      
/* Open ems                                                  */      
/*----------------------------------------------------------------------*/      
void ODBCMXEventMsg::open_ems()                                                         
{                                                                               
	short error;                                                                    

	if( FILE_OPEN_(ems_name, strlen(ems_name), &ems_fnum) != FEOK )                             
	{
		FILE_GETINFO_(ems_fnum, &error);
		exit (error);

	}
                                                                         
} /* open_ems */                                                            
                                                                                
#pragma page "T8666 NOS : close_ems"                                        
                                                          
/*----------------------------------------------------------------------*/      
/* Close ems                                                 */      
/*----------------------------------------------------------------------*/      
void ODBCMXEventMsg::close_ems()                                                       
{                                                                               
	if (ems_fnum != -1)                                                         
	{                                                                            
		FILE_CLOSE_(ems_fnum);                                           
		ems_fnum = -1;                                                           
	}                                                                            
} /* close_ems */                                                           
 
/*----------------------------------------------------------------------*/      
/* Close ems                                                 */      
/*----------------------------------------------------------------------*/      
char* ODBCMXEventMsg::get_ems_name( void )
{ 
	return ems_name;
}
                                                                              

/* The following should go to some other files - later */


#pragma page "T8666 NOS format_spi_token"
void format_spi_token (char *buf, int spi_tkn)
{
   strcpy (buf, "\n");
   switch (spi_tkn)
   {
      case ZSPI_TKN_COMMAND:          strcat (buf, "(COMMAND)");        break;
      case ZSPI_TKN_COMMENT:          strcat (buf, "(COMMENT)");        break;
      case ZSPI_TKN_CONTEXT:          strcat (buf, "(CONTEXT)");        break;
      case ZSPI_TKN_DATALIST:         strcat (buf, "(DATALIST)");       break;
      case ZSPI_TKN_ENDLIST:          strcat (buf, "(ENDLIST)");        break;
      case ZSPI_TKN_ERRLIST:          strcat (buf, "(ERRLIST)");        break;
      case ZSPI_TKN_ERROR:            strcat (buf, "(ERROR)");          break;
      case ZSPI_TKN_HDRTYPE:          strcat (buf, "(HDRTYPE)");        break;
      case ZSPI_TKN_INITIAL_POSITION: strcat (buf, "(COMMENT)");        break;
      case ZSPI_TKN_MANAGER:          strcat (buf, "(MANAGER)");        break;
      case ZSPI_TKN_MAXRESP:          strcat (buf, "(MAXRESP)");        break;
      case ZSPI_TKN_NEXTCODE:         strcat (buf, "(NEXTCODE)");       break;
      case ZSPI_TKN_NEXTTOKEN:        strcat (buf, "(NEXTTOKEN)");      break;
      case ZSPI_TKN_OBJECT_TYPE:      strcat (buf, "(OBJECT TYPE)");    break;
      case ZSPI_TKN_PARM_ERR:         strcat (buf, "(PARM ERR)");       break;
      case ZSPI_TKN_PROC_ERR:         strcat (buf, "(PROC ERR)");       break;
      case ZSPI_TKN_RESET_BUFFER:     strcat (buf, "(RESET BUFFER)");   break;
      case ZSPI_TKN_RETCODE:          strcat (buf, "(RETCODE)");        break;
      case ZSPI_TKN_SERVER_BANNER:    strcat (buf, "(SERVER BANNER)");  break;
      case ZSPI_TKN_SERVER_VERSION:   strcat (buf, "(SERVER VERSION)"); break;
      case ZSPI_TKN_SSID:             strcat (buf, "(SSID)");           break;
	  case ZINI_TKN_SERVER_NAME:    strcat (buf, "(SERVER NAME)");      break;
      
            /* From SPI Programming E-49:50 */
      /* also fill in for message-specific tokens (ZINI_TKN_....) */
      default:
      strcat (buf, " Unknown token = ");
      sprintf(&buf[strlen(buf)], "%ld", spi_tkn);
      /* This the catch all for any token that isn't explain above */
      break;
   } /* end of switch (spi_tkn) */
} /* end of format_spi_token */


#pragma page "T8666 NOS display_spi_error"
void display_spi_error ( short spi_err
                       , short  spi_proc
                       , short  spi_tkn
                       )
{
   /* this is from the tal equivalent in SPI Programming Manual
      E-8 p. E-47 */
   char buf [81] = {0};
   strcpy (buf, "Error from ");
   switch (spi_proc)
   {
      case ZEMS_VAL_EMSINIT:    strcat(buf, "EMSINIT: ");       break;
      case ZEMS_VAL_EMSADDTOKENS: strcat(buf, "EMSADDTOKENS: "); break;
      case ZSPI_VAL_SSGETTKN: strcat(buf, "SSGETTKN: ");         break;
      case ZSPI_VAL_SSMOVE: strcat(buf, "SSMOVE: ");              break;
      case ZSPI_VAL_SSPUT: strcat(buf, "SSPUT: ");                break;
      case ZSPI_VAL_SSPUTTKN: strcat(buf, "SSPUTTKN: ");          break;
      case ZSPI_VAL_BUFFER_FORMATSTART:
      strcat (buf, "FORMATSTART: ");                              break;
      case ZSPI_VAL_BUFFER_FORMATNEXT:
      strcat (buf, "FORMATNEXT: ");                               break;
      case ZSPI_VAL_BUFFER_FORMATFINISH:
      strcat (buf, "FORMATFINISH: ");                             break;
      case ZSPI_VAL_FORMAT_CLOSE:
      strcat (buf, "FORMATCLOSE: ");                              break;
      /* From SPI Programming Manual p. E-47 */
      default:                strcat(buf, "???Unknown function??");   break;
   } /* end of switch (spi_proc) */

   switch (spi_err)
   {
      case ZSPI_ERR_INVBUF:        strcat (buf, "Invalid Buffer");      break;
      case ZSPI_ERR_ILLPARM:       strcat (buf, "Illegal Param");       break;
      case ZSPI_ERR_MISPARM:       strcat (buf, "Missing Param");       break;
      case ZSPI_ERR_BADADDR:       strcat (buf, "Illegal Address");     break;
      case ZSPI_ERR_NOSPACE:       strcat (buf, "Buffer Full");         break;
      case ZSPI_ERR_XSUMERR:       strcat (buf, "Invalid checksum");    break;
      case ZSPI_ERR_INTERR:        strcat (buf, "Internal Error");      break;
      case ZSPI_ERR_MISTKN:        strcat (buf, "Missing Token");       break;
      case ZSPI_ERR_ILLTKN:        strcat (buf, "Illegal Token");       break;
      case ZSPI_ERR_BADSSID:       strcat (buf, "Bad SSID");            break;
      case ZSPI_ERR_NOTIMP:        strcat (buf, "Not implemented");     break;
      case ZSPI_ERR_NOSTACK:       strcat (buf, "Insufficient Stack");  break;
      case ZSPI_ERR_ZFIL_ERR:      strcat (buf, "File System Error");   break;
      case ZSPI_ERR_ZGRD_ERR:      strcat (buf, "OS Kernel Error");     break;
      case ZSPI_ERR_INV_FILE: strcat (buf, "Invalid template file");    break;
      case ZSPI_ERR_CONTINUE:      strcat (buf, "Continue");            break;
      case ZSPI_ERR_NEW_LINE:      strcat (buf, "New line");            break;
      case ZSPI_ERR_NO_MORE:       strcat (buf, "No more");             break;
      case ZSPI_ERR_MISS_NAME:     strcat (buf, "Missing name");        break;
      case ZSPI_ERR_DUP_NAME:      strcat (buf, "Duplicate name");      break;
      case ZSPI_ERR_MISS_ENUM:     strcat (buf, "Invalid checksum");    break;
      case ZSPI_ERR_MISS_STRUCT:   strcat (buf, "Missing struct");      break;
      case ZSPI_ERR_MISS_OFFSET:   strcat (buf, "Invalid checksum");    break;
      case ZSPI_ERR_TOO_LONG:      strcat (buf, "Too Long");            break;
      case ZSPI_ERR_MISS_FIELD:    strcat (buf, "Missing Field");       break;
      case ZSPI_ERR_NO_SCANID:     strcat (buf, "No SCAN ID");          break;
      case ZSPI_ERR_NO_FORMATID:   strcat (buf, "No Format ID");        break;
      case ZSPI_ERR_OCCURS_DEPTH:  strcat (buf, "Occurs Depth");        break;
      case ZSPI_ERR_MISS_LABEL:    strcat (buf, "Missing Label");       break;
      case ZSPI_ERR_BUF_TOO_LARGE: strcat (buf, "Buffer is too big");   break;
      case ZSPI_ERR_OBJFORM:       strcat (buf, "Object Form");         break;
      case ZSPI_ERR_OBJCLASS:      strcat (buf, "Object Class");        break;
      case ZSPI_ERR_BADNAME:       strcat (buf, "Bad name");            break;
      case ZSPI_ERR_TEMPLATE:      strcat (buf, "Template");            break;
      case ZSPI_ERR_ILL_CHAR:      strcat (buf, "Illegal character");   break;
      case ZSPI_ERR_NO_TKNDEFID:   strcat (buf, "No TKNDEF ID");        break;
      case ZSPI_ERR_INCOMP_RESP:   strcat (buf, "Incomplete Response"); break;
      /* From SPI Programming Manual p. E-48 */
      default:                     strcat (buf, "??Unknown err ??");    break;
   }/* end of switch (spi_error) */
   format_spi_token(&buf[strlen(buf)], spi_tkn);
  /* noss_log_error(0, buf, -1, 0, FALSE);*/ /*@@ Should this be 0? */
} /* end of display_spi_error */

#pragma page "T8666 NOS send_to_ems"

/*************************************************************************
*/
void ODBCMXEventMsg::send_to_ems (short evt_num, short EventLogType, char *ComponentName
								  , char *ObjectRef, short nToken, va_list marker)
{
     #define MAX_EVT_BUF_SIZE 3900

	// Next 2 lines uncommented for testing.
//	 va_list marker;

//	 va_start(marker, nToken);

     short evt_buf [MAX_EVT_BUF_SIZE] = {0};
     short error = FEOK;
     short used_len = 0;
     int token = 0;
     char buf[81];
	 char *TokenPtr;
	 int i;
	 
     /* initialize the struct zini_ssid (SPI Programming in C 6-2) corrected*/
     zini_val_ssid_def zini_ssid = { "TANDEM ", ZSPI_SSN_ZMXO, ZINI_VAL_VERSION};

	 memcpy(&zini_ssid.u_z_filler.z_owner, ZINI_VAL_OWNER, strlen(ZINI_VAL_OWNER)); 
    
	 error = EMSINIT (evt_buf /* buffer */
                     , MAX_EVT_BUF_SIZE  /* buffer length */
                     , (short *)&zini_ssid /*ssid */
                     , (short)evt_num /* event number */
                     ,(int)ZMXO_TKN_EVENTTYPE); /* subject token code */
                    
	 if (error != FEOK)
             display_spi_error(error, ZEMS_VAL_EMSINIT, 0 /* token code */);

	 char eventType[5];

	 itoa(EventLogType, eventType, 10);

	 error = EMSADDTOKENS(evt_buf /* our ssid by default */
					,(short *)&zini_ssid
					,ZMXO_TKN_EVENTTYPE
					,eventType
					,(short)strlen(eventType)
					,ZMXO_TKN_COMPONENT	/* token name */
					,ComponentName			/* token value */
					,(short)strlen(ComponentName)	/* length needed */
					,ZMXO_TKN_OBJECTREF
					,ObjectRef
					,(short)strlen(ObjectRef) 
			);

	 for (i=0; i<nToken; i++) {		
		 TokenPtr = va_arg(marker, char *);

		 error = EMSADDTOKENS(evt_buf
				,(short*)&zini_ssid
				,ZEMS_TKN_TEXT
				,TokenPtr
				,(short)strlen(TokenPtr)
		 );
	 }

	 va_end(marker);

     if (error != FEOK)
     {
        display_spi_error(error, ZEMS_VAL_EMSADDTOKENS
                          ,token /* token code */);
       /* return; */
     }

     error = SSGETTKN (evt_buf, ZSPI_TKN_USEDLEN, (char *)&used_len);

     if (error != FEOK)
     {
        display_spi_error(error, ZSPI_VAL_SSGETTKN, ZSPI_TKN_USEDLEN);
     }

     WRITEREADX(ems_fnum, (char *)evt_buf, used_len, (short) 0);
     /* This is the actual code that finally writes to the collector */
     FILE_GETINFO_(ems_fnum, &error);
     if (error != FEOK)
     {
        sprintf(buf, "WRITEREADX error = %d\n", error);
      /*  noss_log_error(0, buf, -1, 0, FALSE); */ /*@@ Should this be 0? */
     }
//     close_ems ();
} /* end of send_to_ems */


#endif