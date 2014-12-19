/**********************************************************************
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
********************************************************************/
/**************************************************************************
**************************************************************************/
#include <platform_ndcs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commonFunctions.h"
#include "odbcCommon.h"
#include "odbcsrvrcommon.h"
#include "platform_utils.h"

BOOL getNumber( char* strValue, int& nValue );

//BOOL checkProcess( char* strProcessName ); // defined in the header
void initTcpProcessName(char *tcpName );


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
	char	*saveptr;

	strncpy(objRef, srvrObjRef, sizeof(objRef));
	objRef[sizeof(objRef)-1] = 0;
	if (strtok_r(objRef, ":", &saveptr) == NULL)
		return 0;
	if (strtok_r(NULL, "/", &saveptr)  == NULL)
		return 0;
	if ((portNumber = strtok_r(NULL, ":", &saveptr)) == NULL)
		return 0;
	return(atol(portNumber));
}


BOOL getNumber( char* strValue, int& nValue )
{
	if( strspn( strValue, "0123456789" ) != strlen( strValue) )
	   return FALSE;
	nValue = atol(strValue);
	return TRUE;
}



/*******************************************************
 *                                                     *
 * Get a value for a param of TCPIP^PROCESS^NAME or    *
 * for a define of =TCPIP^PROCESS^NAME or              *
 * a default process name $ZTC0                        *
 *                                                     *
 *******************************************************/
void initTcpProcessName(char *tcpName )
{
	strcpy(tcpName, DEFAULT_TCP_PROCESS);
}

void insertIpAddressAndHostNameInObjRef(char* srvrObjRef, char* iIpAddress, char* iHostName)
{
	char* pTCP;
	char* pNodeName;
	char* pPortNumber;
	char* pObjectName;
	char* pIpAddress;
	char* pHostName;
	IDL_OBJECT_def objRef;
	int len;
	char	*saveptr;

	(iIpAddress != NULL)? pIpAddress = iIpAddress : pIpAddress = "IP_IS_NULL";
	(iHostName != NULL)? pHostName = iHostName : pHostName = "HOST_IS_NULL";

	strcpy(objRef, srvrObjRef);

	len = strlen(srvrObjRef) + strlen(pIpAddress) + 1 + 1;

	if (len + strlen(pHostName) > 128)
		pHostName = TOO_LONG_HOST_NAME;

	if ((pTCP = strtok_r(objRef, ":", &saveptr)) != NULL)
	{
		if ((pNodeName = strtok_r(NULL, "/", &saveptr))  != NULL)
		{
			if ((pPortNumber = strtok_r(NULL, ":", &saveptr)) != NULL)
			{
				if ((pObjectName = strtok_r(NULL, ":", &saveptr)) != NULL)
				{
					sprintf( srvrObjRef, "%s:%s,%s,%s/%s:%s", pTCP,pNodeName,pIpAddress,pHostName,pPortNumber,pObjectName);
				}
			}
		}
	}

}

void insertInObjRef(char* srvrObjRef, char* IpAddress, char* HostName, char* SegmentName, TCP_ADDRESS tcp_address, char* process_name )
{
	char* pTCP;
	char* pNodeName;
	char* pPortNumber;
	char* pObjectName;
	char* pPtr;
	IDL_OBJECT_def objRef;
	char	*saveptr;

	switch(tcp_address)
	{
		default:
		case IP_ADDRESS:
			pPtr = IpAddress;
			break;
		case HOST_NAME:
			pPtr = HostName;
			break;
		case SEGMENT_NAME:
			pPtr = SegmentName;
			break;
	}

	strcpy(objRef, srvrObjRef);

	if ((pTCP = strtok_r(objRef, ":", &saveptr)) != NULL)
	{
		if ((pNodeName = strtok_r(NULL, "/", &saveptr))  != NULL)
		{
			if ((pPortNumber = strtok_r(NULL, ":", &saveptr)) != NULL)
			{
				if ((pObjectName = strtok_r(NULL, ":", &saveptr)) != NULL)
				{
					sprintf( srvrObjRef, "%s:%s,%s/%s:%s", pTCP, process_name, pPtr, pPortNumber,pObjectName);
				}
			}
		}
	}
}

void removeIpAddressAndHostNameFromObjRef(char* srvrObjRef, char* IpAddress, char* HostName)
{
	char* pTCP;
	char* pNodeName;
	char* pIpAddress;
	char* pHostName;
	char* pPortNumber;
	char* pObjectName;
	IDL_OBJECT_def objRef;
	char	*saveptr;

	strcpy(objRef, srvrObjRef);

	if ((pTCP = strtok_r(objRef, ":", &saveptr)) != NULL)
	{
		if ((pNodeName = strtok_r(NULL, ",", &saveptr))  != NULL)
		{
			if ((pIpAddress = strtok_r(NULL, ",", &saveptr)) != NULL)
			{
				if ((pHostName = strtok_r(NULL, "/", &saveptr)) != NULL)
				{
					if ((pPortNumber = strtok_r(NULL, ":", &saveptr)) != NULL)
					{
						if ((pObjectName = strtok_r(NULL, ":", &saveptr)) != NULL)
						{
							sprintf( srvrObjRef, "%s:%s/%s:%s", pTCP,pNodeName,pPortNumber, pObjectName);
							strcpy(IpAddress, pIpAddress);
							strcpy(HostName, pHostName);
						}
					}
				}
			}
		}
	}

}

bool isUTF8(const char *str)
{
	char c;
	unsigned short byte = 1;
	size_t len = strlen(str);

	for (size_t i=0; i<len; i++)
	{
		c = str[i];

		if (c >= 0x00 && c < 0x80 && byte == 1) // ascii
			continue;
		else if (c >= 0x80 && c < 0xc0 && byte > 1) // second, third, or fourth byte of a multi-byte sequence
			byte--;
		else if (c == 0xc0 || c == 0xc1) // overlong encoding
			return false;
		else if (c >= 0xc2 && c < 0xe0 && byte == 1) // start of 2-byte sequence
			byte = 2;
		else if (c >= 0xe0 && c < 0xf0 && byte == 1) // start of 3-byte sequence
			byte = 3;
		else if (c >= 0xf0 && c < 0xf5 && byte == 1) // start of 4-byte sequence
			byte = 4;
		else
			return false;
	}
	return true;
}

char* strcpyUTF8(char *dest, const char *src, size_t destSize, size_t copySize)
{
	char c;
	size_t len;

	if (copySize == 0)
		len = strlen(src);
	else
		len = copySize;

	if (len >= destSize)
		len = destSize-1; // truncation

	while (len > 0)
	{
		c = src[len-1];
		if (c < 0x80 || c > 0xbf)
			break;
		len--; // in second, third, or fourth byte of a multi-byte sequence
	}
	strncpy((char*)dest, (const char*)src, len);
	dest[len] = 0;

	return dest;
}

// @@@ TBD @@@  Enable once the monitor delivered process death messages for persistent processes to other nodes (bugzilla 2909)
#if 0
/*
 * Registers for process death messages for the specified process name
 * This will be used by the mxosrvrs and mxocfg process'es to register
 * for mxoas death messages, instead of polling every 15 seconds to
 * check if the mxoas process is still around
 */
bool RegisterProcessDeathMessage(char *processName)
{
   int ferr; // file system errors
   int nid;  // node id
   int pid;  // process id

   ferr = msg_mon_get_process_info(processName,&nid,&pid);
   if(ferr != XZFIL_ERR_OK || nid == -1 || pid == -1) return false;

   ferr = msg_mon_register_death_notification(nid, pid);
   if(ferr != XZFIL_ERR_OK) return false;

} /* RegisterProcessDeathMessage() */
#endif /* 0 */



#if defined(NSK_ODBC_SRVR) || defined(NSK_AS)
#include "Transport.h"
#ifdef NSK_ODBC_SRVR
#include "Listener_srvr.h"
#endif
#ifdef NSK_AS
#include "Listener_as.h"
#endif
void terminateThreads(int exitcode)
{
	GTransport.m_listener->terminateThreads(exitcode);
}
#endif // NSK_ODBC_SRVR || NSK_AS
