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
#include <stdlib.h>
#include <string.h>
#include "commonFunctions.h"
#include "odbcCommon.h"
#include "odbcsrvrcommon.h"
#ifdef NSK_PLATFORM
#include "cextdecs.h"
#include <fs\feerrors.h>
#include <stdlib.h>
#endif

BOOL getNumber( char* strValue, int& nValue );

#ifdef NSK_PLATFORM
BOOL checkProcess( char* strProcessName );
void initTcpProcessName(char *tcpName );
#endif

BOOL checkPortNumber( long portNumber );

CEECFG_Transport getTransport(const IDL_Object srvrObjRef)
{
	if (strncmp(srvrObjRef, "TCP", 3) == 0)
		return CEE_TRANSPORT_TCP;
	else
		return CEE_TRANSPORT_INVALID;
}

long getPortNumber(const IDL_Object srvrObjRef)
{
	IDL_OBJECT_def objRef;
	char	*portNumber;
	char *saveptr=NULL;

	strncpy(objRef, srvrObjRef, sizeof(objRef));
	objRef[sizeof(objRef)-1] = 0;
#ifndef unixcli
	if (strtok(objRef, ":") == NULL)
		return 0;
	if (strtok(NULL, "/")  == NULL)
		return 0;
	if ((portNumber = strtok(NULL, ":")) == NULL)
		return 0;
	return(atol(portNumber));
#else
	if (strtok_r(objRef, ":",&saveptr) == NULL)
		return 0;
	if (strtok_r(NULL, "/",&saveptr)  == NULL)
		return 0;
	if ((portNumber = strtok_r(NULL, ":",&saveptr)) == NULL)
		return 0;
	return(atol(portNumber));

#endif
}


BOOL getInitParam(int argc, char *argv[], SRVR_INIT_PARAM_Def &initParam, char* strName, char* strValue)
{
	int		count;
	char*	arg = NULL;
	int		number;
	BOOL	retcode;
	BOOL	argEmpty = FALSE;
	BOOL	argWrong = FALSE;

	initParam.debugFlag			= 0;
	initParam.eventFlag			= 0;
	initParam.asSrvrObjRef[0]	= '\0';
	initParam.srvrType			= SRVR_UNKNOWN;
	initParam.startType			= NON_INTERACTIVE;
	initParam.noOfServers		= DEFAULT_NUMBER_OF_SERVERS;
	initParam.cfgSrvrTimeout	= DEFAULT_CFGSRVR_TIMEOUT;
	initParam.DSId				= PUBLIC_DSID; 
	initParam.transport			= CEE_TRANSPORT_TCP;
	initParam.portNumber		= DEFAULT_PORT_NUMBER;
	initParam.majorVersion		= DEFAULT_MAJOR_VERSION;
	initParam.IpPortRange		= DEFAULT_PORT_RANGE;
	initParam.DSName[0]			= '\0';
	count = 1;

	strName[0] = 0;
	strValue[0] = 0;

#ifdef NSK_PLATFORM
	initTcpProcessName(initParam.TcpProcessName);
	initParam.ASProcessName[0]	= '\0';
	strcpy( initParam.EmsName, DEFAULT_EMS );

	initParam.EmsTimeout		= DEFAULT_EMS_TIMEOUT;
	initParam.initIncSrvr		= DEFAULT_INIT_SRVR;
	initParam.initIncTime		= DEFAULT_INIT_TIME;
	initParam.DSG =DEFAULT_DSG;
	initParam.srvrTrace = false;
	initParam.TraceCollector[0]	= '\0';
	initParam.RSCollector[0]	= '\0';
#else
	initParam.IpAddress[0]		= '\0';
#endif

	while ( count < argc)
	{
		if( (arg = (char*)realloc( arg, strlen( argv[count]) + 1) ) == NULL)
		{
			strcpy( strName, "realloc");
			sprintf( strValue, "%d",strlen( argv[count]) + 1);
			return FALSE;
		}

		strcpy(arg, argv[count]);
		
		strupr(arg);

		if (strcmp(arg, "-A") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				if (strlen(argv[count]) < sizeof(initParam.asSrvrObjRef)-1)
					strcpy(initParam.asSrvrObjRef, argv[count]);
				else
				{
					argWrong = TRUE;
					break;
				}
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
		else
		if (strcmp(arg, "-D") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				if( getNumber( argv[count], number ) == TRUE )
					initParam.debugFlag = number;
				else
				{
					argWrong = TRUE;
					break;
				}
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
		else
		if (strcmp(arg, "-E") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				if( getNumber( argv[count], number ) == TRUE )
				{
					if( number != 0 && number != 1 && number !=2 )
					{
						argWrong = TRUE;
						break;
					}

					initParam.eventFlag = number;
				}
				else
				{
					argWrong = TRUE;
					break;
				}
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
		else
		if (strcmp(arg, "-DS") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				if( getNumber( argv[count], number ) == TRUE )
				{
					initParam.DSId = number;
				}
				else
				{
					argWrong = TRUE;
					break;
				}
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
		else
		if (strcmp(arg, "-DSN") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				if (strlen(argv[count]) < sizeof(initParam.DSName)-1)
					strcpy(initParam.DSName, argv[count]);
				else
				{
					argWrong = TRUE;
					break;
				}
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
		else
		if (strcmp(arg, "-PN") == 0)
		{
			if(++count < argc && argv[count][0] != '-' )
			{
				if( getNumber( argv[count], number ) == TRUE && checkPortNumber(number) == TRUE )
				{
					initParam.portNumber = number;
				}
				else
				{
					argWrong = TRUE;
					break;
				}
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
		else
		if (strcmp(arg, "-PR") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				if( getNumber( argv[count], number ) == TRUE && number >= 3)
				{
					initParam.IpPortRange = number;
				}
				else
				{
					argWrong = TRUE;
					break;
				}
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
#ifdef NSK_PLATFORM
		else
		if (strcmp(arg, "-TCP") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				if (strlen(argv[count]) < sizeof(initParam.TcpProcessName)-1)
				{
					if( checkProcess( argv[count]) != TRUE )
					{
						argWrong = TRUE;
						break;
					}
					strcpy(initParam.TcpProcessName, argv[count]);
				}
				else
				{
					argWrong = TRUE;
					break;
				}
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
		else
		if (strcmp(arg, "-AS") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				if (strlen(argv[count]) < sizeof(initParam.ASProcessName)-1)
				{
					if( checkProcess( argv[count]) != TRUE )
					{
						argWrong = TRUE;
						break;
					}
					strcpy(initParam.ASProcessName, argv[count]);
				}
				else
				{
					argWrong = TRUE;
					break;
				}
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
		else
		if (strcmp(arg, "-EMS") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				if (strlen(argv[count]) < sizeof(initParam.EmsName)-1)
				{
					strcpy(initParam.EmsName, argv[count]);
				}
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
		else
		if (strcmp(arg, "-TMC") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				if (strlen(argv[count]) < sizeof(initParam.TraceCollector)-1)
				{
					strcpy(initParam.TraceCollector, argv[count]);
				}
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
		else
		if (strcmp(arg, "-RMC") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				if (strlen(argv[count]) < sizeof(initParam.RSCollector)-1)
				{
					strcpy(initParam.RSCollector, argv[count]);
				}
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
		else
		if (strcmp(arg, "-TE") == 0)
		{
			if(++count < argc && argv[count][0] != '-' )
			{
				if( getNumber( argv[count], number ) == TRUE )
				{
					initParam.EmsTimeout = number;
				}
				else
				{
					argWrong = TRUE;
					break;
				}
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
		else
		if (strcmp(arg, "-ISRVR") == 0)
		{
			if(++count < argc && argv[count][0] != '-' )
			{
				if( getNumber( argv[count], number ) == TRUE )
				{
					initParam.initIncSrvr = number;
				}
				else
				{
					argWrong = TRUE;
					break;
				}
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
		else
		if (strcmp(arg, "-ITIME") == 0)
		{
			if(++count < argc && argv[count][0] != '-' )
			{
				if( getNumber( argv[count], number ) == TRUE )
				{
					initParam.initIncTime = number;
				}
				else
				{
					argWrong = TRUE;
					break;
				}
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
		else
		if (strcmp(arg, "-DSG") == 0)
		{
			initParam.DSG =true;
		}
		else
		if (strcmp(arg, "-STC") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				if (strlen(argv[count]) < 2)
				{
					if (strcmp(argv[count], "Y") == 0)
						initParam.srvrTrace = true;
					else
						initParam.srvrTrace = false;
				}
				else
				{
					argWrong = TRUE;
					break;
				}
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
#else
		else
		if (strcmp(arg, "-IP") == 0)
		{
			if(++count < argc && argv[count][0] != '-' )
			{
				if (strlen(argv[count]) < sizeof(initParam.IpAddress)-1)
					strcpy(initParam.IpAddress, argv[count]);
				else
				{
					argWrong = TRUE;
					break;
				}
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
#endif
		else
		if (strcmp(arg, "-T") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				if( getNumber( argv[count], number ) == TRUE )
				{
					initParam.transport = number;
				}
				else
				{
					argWrong = TRUE;
					break;
				}
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
		else
		if (strcmp(arg, "-N") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				if( getNumber( argv[count], number ) == TRUE )
				{
					initParam.noOfServers = number;
				}
				else
				{
					argWrong = TRUE;
					break;
				}
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
		else
		if (strcmp(arg, "-TO") == 0)
		{
			if(++count < argc && argv[count][0] != '-' )
			{
				if( getNumber( argv[count], number ) == TRUE )
				{
					initParam.cfgSrvrTimeout = number;
				}
				else
				{
					argWrong = TRUE;
					break;
				}
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
		else
		if (strcmp(arg, "-V") == 0)
		{
			if(++count < argc && argv[count][0] != '-' )
			{
				if( getNumber( argv[count], number ) == TRUE )
				{
					initParam.majorVersion = number;
				}
				else
				{
					argWrong = TRUE;
					break;
				}
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
		else
		if (strcmp(arg, "-I") == 0)
			initParam.startType = INTERACTIVE;
		else
		if (strcmp(arg, "-FO") == 0)
			initParam.startType = FAILOVER;
		else
		{
			argWrong = TRUE;
			count = argc;
			break;
		}

		count++;
	}

#ifdef NSK_PLATFORM

	if (initParam.TraceCollector[0]	== '\0')
		strcpy(initParam.TraceCollector, initParam.EmsName);
	if (initParam.RSCollector[0] == '\0')
		strcpy(initParam.RSCollector, initParam.EmsName);
#endif

	if( argEmpty == TRUE || argWrong == TRUE )
	{
		strcpy( strName, arg);
		if(count < argc)
			strcpy( strValue, argv[count]);
		else
			strcpy( strValue, "");
		retcode = FALSE;
	}
	else
		retcode = TRUE;

	if( arg != NULL ) free(arg);

	return retcode;
}

BOOL getNumber( char* strValue, int& nValue )
{
	if( strspn( strValue, "0123456789" ) != strlen( strValue) )
	   return FALSE;
	nValue = atol(strValue);
	return TRUE;
}

#ifdef NSK_PLATFORM

BOOL checkProcess( char* strProcessName )
{
	NSK_PROCESS_HANDLE processHandle;

	short errorDetail;
	short proctype;
	short retcode;

	if( strProcessName == NULL ) return FALSE;

	PROCESSHANDLE_NULLIT_( processHandle ); 

	retcode = FILENAME_TO_PROCESSHANDLE_( strProcessName, 
								strlen( strProcessName),
								 processHandle );

	if( retcode != 0 ) return FALSE;


	retcode = PROCESS_GETINFO_(processHandle,
			OMITREF, OMITSHORT,OMITREF,		// proc string,max buf len,act len
			OMITREF,						// priority
			OMITREF,						// Mom's proc handle 
			OMITREF, OMITSHORT,OMITREF,		// home term,max buf len,act len  
			OMITREF,						// Process execution time 
			OMITREF,						// Creator Access Id 
			OMITREF,						// Process Access Id 
			OMITREF,						// Grand Mom's proc handle 
			OMITREF,						// Job Id 
			OMITREF, OMITSHORT,OMITREF,		// Program file,max buf len,act len  
			OMITREF, OMITSHORT,OMITREF,		// Swap file,max buf len,act len 
			&errorDetail,
			&proctype,						// Process type 
			OMITREF);						// OSS or NT process Id

	if( retcode != 0 ) return FALSE;
	
	return TRUE;
}

#endif
/*
Port Numbers for Network Services

	Port Number		Protocol		C Name(s)of Service or Function
	7				TCP,UDP			echo
	9				UDP				discard, sink null
	11				TCP				systat
	13				TCP				daytime
	15				TCP				netstat
	20				TCP				ftp-data
	21				TCP				ftp
	23				TCP				telnet
	25				TCP				smtp, mail
	37				TCP,UDP			time, timserver
	42				UDP				name, nameserver
	43				TCP				whois, nicname (usually tosri-nic)
	53				TCP,UDP			domain
	101				TCP				hostnames, hostname (usually tosri-nic)
	111				TCP,UDP			sunrpc

Port Numbers for Host-Specific Functions

	Port Number		Protocol		C Name(s) of Service or Function

	69				UDP				tftp
	77				TCP				rje
	79				TCP				finger
	87				TCP				link, ttylink
	95				TCP				supdup
	105				TCP				csnet-ns
	117				TCP				uucp-path
	119				TCP				untp, usenet
	123				TCP				ntp
	1524			TCP				ingreslock

Port Numbers for UNIX-Specific Services

	Port Number		Protocol		C Name(s) of Service or Function

	512				TCP				exec
					UDP				biff, comsat
	513				TCP				login
					UDP				who, whod
	514				TCP				shell, cmd (no passwords used)
					UDP				syslog
	515				TCP				printer, spooler (experimental)
	517				UDP				talk
	520				UDP				route, router, routed
	530				TCP				courier, rpc (experimental)
	550				UDP				new-rwho, new-who (experimental)
	560				UDP				rmonitor, rmonitord (experimental)
	561				UDP				monitor (experimental)

*/

BOOL checkWellKnownPortNumber( long portNumber )
{
	if( portNumber != 1524 && portNumber > 561 ) return TRUE;

	switch( portNumber )
	{
	case 7:case 9:case 11:case 13:case 15:case 20:case 21:case 23:case 25:case 37:
	case 42:case 43:case 53:case 101:case 111:
	case 69:case 77:case 79:case 87:case 95:case 105:case 117:case 119:case 123:case 1524:
	case 512:case 513:case 514:case 515:case 517:case 520:case 530:case 550:case 560:case 561:
		return FALSE;
	default:
		return TRUE;
	}
}

BOOL checkPortNumber( long portNumber )
{
	if( portNumber > MAX_PORT_NUMBER ) return FALSE;
	if( portNumber <= 0 ) return FALSE;
	if( MAX_PORT_NUMBER + 1 - portNumber < 3 ) return FALSE;

	return checkWellKnownPortNumber( portNumber );
}

int setBuildId(char* vProcName)
{
	int i;
	for (i = strlen(vProcName); i > 0; i--)
		if (vProcName[i] == '_') break;
	if (i == 0)
		return NSK_BUILD_1;
	else
		return atoi(&vProcName[i+1]);
}

/******************************************************* 
 *                                                     * 
 * Get a value for a param of TCPIP^PROCESS^NAME or    * 
 * for a define of =TCPIP^PROCESS^NAME or              *
 * a default process name $ZTC0                        * 
 *                                                     * 
 *******************************************************/
#ifdef _GUARDIAN_TARGET 
void initTcpProcessName(char *tcpName )
{
	short error;
	short bufflen = MAX_TCP_PROCESS_NAME_LEN;
	short len;
	char attrname[17] = "FILE            ";

	tcpName[0] = 0;

	error = get_param_by_name("TCPIP^PROCESS^NAME"
		,tcpName
		,bufflen);
	if (!error)
		return;
		
	error = DEFINEREADATTR( 
		"=TCPIP^PROCESS^NAME     "				/* IN OPTIONAL  */
		,attrname								/* IN/OUT  */
		,										/* IN/OUT OPTIONAL  */
		,tcpName								/* OUT  */
		,bufflen								/* IN  */
		,&len									/* OUT  */
		,										/* IN OPTIONAL  */
		,										/* OUT OPTIONAL  */
		);
	if (!error)
		tcpName[len]=0;
	if (tcpName[0] == 0)
		strcpy(tcpName, DEFAULT_TCP_PROCESS);
}
#else
void initTcpProcessName(char *tcpName )
{
	strcpy(tcpName, DEFAULT_TCP_PROCESS);
}
#endif



