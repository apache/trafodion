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

#include "RegValues.h"
#include <platform_utils.h>


RegValues *RG = NULL;

RegValues::RegValues() {
    // set all the base level defaults, prior to reading RegValues
	// or reading configuration data
	BootCfgPath[0] = '\0';
	TandemIdVal[0] = '\0';
	ImagePathVal[0] = '\0';
	ClusterCfgPath[0] = '\0';
	ODBCCfgFailOver = 0;
    ODBCCfgPath[0] = '\0';
    ODBCCfgAnchorFileNm[0] = '\0';
	ODBCCfgVersion[0] = '\0';
	ODBCCfgASIpAddress[0] = '\0';
	ODBCCfgASIpPortId = -1;
	ODBCASObjRef[0] = '\0';
	LocalComputerName[0] = '\0';
	ODBCOleCfgSrvrIpPortId = -1;
	ODBCCfgIpPortRange = -1;
	ODBCCfgDefInitCnt = -1;
	ODBCCfgDefMaxCnt = -1;
	ODBCCfgDefAvailCnt = -1;

	NODECnt = -1;
	NODEMask = 0;
	SQLSystemVolQualifier[0] = '\0';
	SQLSystemVolumeNmVal[0] = '\0';

	CPUCnt = 0;
	return;
}

RegValues::~RegValues() {
	// nothing to do
	return;
}


long RegValues::_CPUCnt (char *CPUList, short maxNode)
{
	CPUCnt = maxNode;
	if (CPUCnt == 0)
		return -1;

	return CPUCnt;
}

bool RegValues::_CPUspawnOn(short& nextCPUspawn, long& cpuNumber, short& segmentIndex, NEO_SEGMENT* neoSegment, short neoSegmentMax)
{
	short	cpuNum;
	short	cpuServerTobeSpawned = -1;
	short	cpuNumWithinSegment = 0;
	short	segmentNum = -1;
	bool	cpuFound = FALSE;

	if (CPUCnt == 0)
		return FALSE;

	for (cpuNum = nextCPUspawn; cpuNum < CPUCnt; cpuNum++)
	{
		if(nextCPUspawn%(CPUCnt+1) < CPUCnt)
		{
			cpuNumber = cpuNum;
			cpuFound = TRUE;
			nextCPUspawn = cpuNum;
			segmentIndex= cpuNum;
			break;
		}
	 }

	if (!cpuFound )
	{
	   for (cpuNum = nextCPUspawn; cpuNum < CPUCnt; cpuNum++)
	   {
			if(nextCPUspawn%(CPUCnt+1) < CPUCnt)
			{
				cpuNumber = cpuNum;
				cpuFound = TRUE;
				nextCPUspawn = cpuNum;
				segmentIndex= cpuNum;
				break;
			}
	 	}
	}

	if (!cpuFound )
		nextCPUspawn = 0;
	else
	{
		nextCPUspawn++;
		nextCPUspawn = nextCPUspawn % CPUCnt;
	}

	return (cpuFound);
}

BOOL RegValues::_NODEOn( long NodeNum )
{
	if ((NodeNum < 0) || (NodeNum > MAX_NODE_CNT))
		return FALSE;
//	return ( (NODEMask & ( 1 << NodeNum )) != 0 );
	return (NODEMask >> (15 - NodeNum) & 1);

}

BOOL RegValues::_NODEOn( long NodeNum, long CPUList )
{
	if ((NodeNum < 0) || (NodeNum > MAX_NODE_CNT))
		return FALSE;
//	return ( (NODEMask & ( 1 << NodeNum )) != 0 );
	return ((NODEMask & CPUList) >> (15 - NodeNum) & 1);

}


long RegValues::_NODEMask()
{
	return NODEMask;
}

BOOL RegValues::_CfgODBCEnabled4Node ( long NodeNum )
{
	if (_NODEOn( NodeNum ) == FALSE )
	    return FALSE;

	return (BOOL) NodeDescList[NodeNum].ODBCEnabled4Node;
}

char* RegValues::_CfgNODEExternalIpAddr ( long NodeNum )
{
	if (_NODEOn( NodeNum ) == FALSE )
	    return (char*) NULL;
	else
		return NodeDescList[NodeNum].ExternalIpAddress;
}

void  RegValues::SET_NODEExternalIpAddr ( long NodeNum
										,char *pExternalIpAddress ) {
	if (_NODEOn( NodeNum ) == FALSE )
	    return ;
	if (pExternalIpAddress != NULL)
		strcpy(NodeDescList[NodeNum].ExternalIpAddress, pExternalIpAddress);
	else
		NodeDescList[NodeNum].ExternalIpAddress[0] = '\0';
	return;
}

char* RegValues::_CfgNODENetBiosNm ( long NodeNum )
{
	if (_NODEOn( NodeNum ) == FALSE )
	    return (char*) NULL;

	if (strcmp(NodeDescList[NodeNum].NetBiosName, "-1") == 0)
		// if this node in the cluster is made not available to ODBC
		// by giving it an invalid IP address, disable its use by setting
		// to null
		return (char*) NULL;
	else
		// if this node has a specific address for ODBC services, use it
		// instead of the machine name
		return NodeDescList[NodeNum].NetBiosName;
}

long RegValues::SET_ODBCCfgVals () {
	HKEY	kCurrKeyValue;
	HKEY	hKeySystem;
	long	error;
	unsigned long	length;
	DWORD	valueType;
	char	ValueData[MAX_SQL_ID_LEN + 1];
	char	KeySearchPath[MAX_REGKEY_PATH_LEN + 1];

#ifdef WIN32
	length = MAX_REGKEY_PATH_LEN;

	hKeySystem = HKEY_LOCAL_MACHINE;

	sprintf(KeySearchPath, "%s", _NONSTOP_PATH_KEY);

	error = RegOpenKeyEx(	hKeySystem
						   ,KeySearchPath
						   ,0
						   ,KEY_READ
						   ,&kCurrKeyValue );
	if (error != ERROR_SUCCESS) {
		return error;
	}
	length = sizeof(ValueData);
	error = RegQueryValueEx ( kCurrKeyValue
	                       ,_BOOT_CONFIG_VAL_NM
						   ,NULL
						   ,&valueType
						   ,(LPBYTE)ValueData
						   ,(LPDWORD)&length );

    if (error == ERROR_SUCCESS)
		sprintf( BootCfgPath, "%s\\%s", _NONSTOP_PATH_KEY ,ValueData );
	else
		if (error == ERROR_FILE_NOT_FOUND)
			sprintf( BootCfgPath, "%s\\%s", _NONSTOP_PATH_KEY ,"Default-Config" );
		else
			return error;

	length = sizeof(ValueData);
	error = RegQueryValueEx ( kCurrKeyValue
							,_TANDEM_ID_VAL_NM
							,NULL
							,&valueType
							,(LPBYTE)ValueData
							,(LPDWORD)&length );

	if (error != ERROR_SUCCESS)  {
		if (error == ERROR_FILE_NOT_FOUND)
			TandemIdVal[0] = 0;
		else
			return error;
	}
	else
		strcpy(TandemIdVal, ValueData);

	length = sizeof(ValueData);
	error = RegQueryValueEx ( kCurrKeyValue
							,_IMAGE_PATH_VAL_NM
							,NULL
							,&valueType
							,(LPBYTE)ValueData
							,(LPDWORD)&length );

	if (error != ERROR_SUCCESS) {
		if (error == ERROR_FILE_NOT_FOUND)
			ImagePathVal[0] = 0;
		else
			return error;
	}
	else
		strcpy(ImagePathVal, ValueData);

	RegCloseKey( kCurrKeyValue );

	// now retrieve cluster definition values
	if (SET_NODECfgVals() != ERROR_SUCCESS)
		return error;

	// retrieve SQL Cluster Configuration values
	sprintf(KeySearchPath, "%s\\%s", BootCfgPath, _SQL_KEY);
	error = RegOpenKeyEx(	hKeySystem
						   ,KeySearchPath
						   ,0
						   ,KEY_READ
						   ,&kCurrKeyValue );
	if (error != ERROR_SUCCESS) {
		return error;
	}

	length = sizeof(SQLSystemVolumeNmVal);
	error = RegQueryValueEx ( kCurrKeyValue
	                       ,_SQL_SYSTEM_VOLUME_NM
						   ,NULL
						   ,&valueType
						   ,(LPBYTE)SQLSystemVolumeNmVal
						   ,(LPDWORD)&length );

	if(error != ERROR_SUCCESS) {
		if (error == ERROR_FILE_NOT_FOUND)
			SQLSystemVolumeNmVal[0] = '\0';
		else
			return error;
	}
	else {
		RegCloseKey( kCurrKeyValue );
		sprintf(KeySearchPath, "%s\\%s\\%s"
								, BootCfgPath
								, _DP2_KEY
								, SQLSystemVolumeNmVal);
		error = RegOpenKeyEx(	hKeySystem
							   ,KeySearchPath
							   ,0
							   ,KEY_READ
							   ,&kCurrKeyValue );
		if (error == ERROR_SUCCESS) {
			length = sizeof(SQLSystemVolQualifier);
			error = RegQueryValueEx ( kCurrKeyValue
	                       ,_DIRECTORY_VAL_NM
						   ,NULL
						   ,&valueType
						   ,(LPBYTE)SQLSystemVolQualifier
						   ,(LPDWORD)&length );
			if (error != ERROR_SUCCESS)
				SQLSystemVolQualifier[0] = '\0';
		}
		RegCloseKey( kCurrKeyValue );
	}

	// retrieve ODBC Cluster Configuration values
	sprintf(KeySearchPath, "%s\\%s", BootCfgPath, _ODBC_KEY);

	error = RegOpenKeyEx(	hKeySystem
						   ,KeySearchPath
						   ,0
						   ,KEY_READ
						   ,&kCurrKeyValue );
	if (error != ERROR_SUCCESS) {
		return error;
	}

	char ODBCCfgAutomaticStartVal[4];
	length = sizeof(ODBCCfgAutomaticStartVal);
	error = RegQueryValueEx ( kCurrKeyValue
	                       ,_AUTOMATIC_START_VAL_NM
						   ,NULL
						   ,&valueType
						   ,(LPBYTE)ODBCCfgAutomaticStartVal
						   ,(LPDWORD)&length );
	if (error != ERROR_SUCCESS) {
		if ((strcmp(ODBCCfgAutomaticStartVal, "YES") == 0) ||
			(strcmp(ODBCCfgAutomaticStartVal, "yes") == 0)    )
			ODBCCfgAutomaticStart = 1;
		else
			ODBCCfgAutomaticStart = 0;
	}
	else
		ODBCCfgAutomaticStart = 0;

	ODBCCfgFailOver = 0;

	length = sizeof(ODBCCfgASIpAddress);
	error = RegQueryValueEx ( kCurrKeyValue
							   ,_IPADDRESS_VAL_NM
							   ,NULL
							   ,&valueType
							   ,(LPBYTE)ODBCCfgASIpAddress
							   ,&length );
	if (error != ERROR_SUCCESS) {
		if (error == ERROR_FILE_NOT_FOUND)
			strcpy(ODBCCfgASIpAddress, "127.0.0.1");
		else
			return error;
	}

	if (strspn(ODBCCfgASIpAddress, " ") == strlen(ODBCCfgASIpAddress))
		strcpy(ODBCCfgASIpAddress, "127.0.0.1");

	length = sizeof(ODBCDbgDetail);
	error = RegQueryValueEx ( kCurrKeyValue
							   ,_DBG_FLAG
							   ,NULL
							   ,&valueType
							   , (LPBYTE) &ODBCDbgDetail
							   ,&length );
	if (error != ERROR_SUCCESS) {
		if (error == ERROR_FILE_NOT_FOUND)
			ODBCDbgDetail = 0;
		else
			return error;
	}

	length = sizeof(ODBCEventLevel);
	error = RegQueryValueEx ( kCurrKeyValue
							   ,_EVENT_LEVEL
							   ,NULL
							   ,&valueType
							   , (LPBYTE) &ODBCEventLevel
							   ,&length );
	if (error != ERROR_SUCCESS) {
		if (error == ERROR_FILE_NOT_FOUND)
			ODBCEventLevel = 0;
		else
			return error;
	}


	// tja 6/8/98
	// added, as SQL/MX install changed from IpPort, IpPortRange
	// to IpPort and IpPortMax

	length = sizeof(ODBCCfgASIpPortId);
	error = RegQueryValueEx ( kCurrKeyValue
	                       ,_IPPORT_ID_VAL_NM
						   ,NULL
						   ,&valueType
						   ,(LPBYTE)&ODBCCfgASIpPortId
						   ,&length );
	if (error != ERROR_SUCCESS) {
		if (error == ERROR_FILE_NOT_FOUND)
			ODBCCfgASIpPortId  = 18650;
		else
			return error;
	}

	length = sizeof(ODBCCfgIpPortIdMax);
	error = RegQueryValueEx ( kCurrKeyValue
							   ,_IPPORT_MAX_VAL_NM
							   ,NULL
							   ,&valueType
							   ,(LPBYTE)&ODBCCfgIpPortIdMax
							   ,&length );
	if (error != ERROR_SUCCESS) {
		if (error == ERROR_FILE_NOT_FOUND)
			ODBCCfgIpPortIdMax = ODBCCfgASIpPortId + 100;
		else
			return error;
	}

	ODBCCfgIpPortRange = ODBCCfgIpPortIdMax - ODBCCfgASIpPortId - 1; // 1 for CfgSever of OLE server

  	char ODBCCfgPrimaryNodeVal[100];
	length = sizeof(ODBCCfgPrimaryNodeVal);
	error = RegQueryValueEx ( kCurrKeyValue
	                       ,_PRIMARY_VAL_NM
						   ,NULL
						   ,&valueType
						   ,(LPBYTE)&ODBCCfgPrimaryNodeVal
						   ,&length );
	if (error != ERROR_SUCCESS) {
		if (error == ERROR_FILE_NOT_FOUND)
			ODBCCfgPrimaryNode  = 0;
		else
			return error;
	}
	else
		ODBCCfgPrimaryNode = ConvertPrimaryNodeVal(ODBCCfgPrimaryNodeVal);

	length = sizeof(ODBCCfgVersion);
	error = RegQueryValueEx ( kCurrKeyValue
	                       ,_VERSION_VAL_NM
						   ,NULL
						   ,&valueType
						   ,(LPBYTE)ODBCCfgVersion
						   ,(LPDWORD)&length );
	if (error != ERROR_SUCCESS) {
		if (error == ERROR_FILE_NOT_FOUND)
			ODBCCfgVersion[0] = '\0';
		else
			return error;
	}



	length = sizeof(ODBCCfgAnchorFileNm);
	error = RegQueryValueEx ( kCurrKeyValue
						   ,_ANCHOR_FILE_VAL_NM
						   ,NULL
						   ,&valueType
						   ,(LPBYTE)ODBCCfgAnchorFileNm
						   ,(LPDWORD)&length );
	if (error != ERROR_SUCCESS) {
		if (error == ERROR_FILE_NOT_FOUND)
			ODBCCfgAnchorFileNm[0] = '0';
		else
			return error;
	}




	// added to accomodate no Cfg
	length = sizeof(ODBCCfgDefInitCnt);
	error = RegQueryValueEx ( kCurrKeyValue
						   ,_DEF_INIT_CNT
						   ,NULL
						   ,&valueType
						   ,(LPBYTE)&ODBCCfgDefInitCnt
						   ,&length );

	if (error != ERROR_SUCCESS) {
		if (error == ERROR_FILE_NOT_FOUND)
			ODBCCfgDefInitCnt = 1;
		else
			return error;
	}

	length = sizeof(ODBCCfgDefAvailCnt);
	error = RegQueryValueEx ( kCurrKeyValue
						   ,_DEF_AVAIL_CNT
						   ,NULL
						   ,&valueType
						   ,(LPBYTE)&ODBCCfgDefAvailCnt
						   ,&length );

	if (error != ERROR_SUCCESS) {
		if (error == ERROR_FILE_NOT_FOUND)
			ODBCCfgDefAvailCnt = 1;
		else
			return error;
	}

	length = sizeof(ODBCCfgDefMaxCnt);
	error = RegQueryValueEx ( kCurrKeyValue
						   ,_DEF_MAX_CNT
						   ,NULL
						   ,&valueType
						   ,(LPBYTE)&ODBCCfgDefMaxCnt
						   ,&length );

	if (error != ERROR_SUCCESS) {
		if (error == ERROR_FILE_NOT_FOUND)
			ODBCCfgDefMaxCnt = NO_MAX_SRVR_LIMIT;
		else
			return error;
	}


	ODBCOleCfgSrvrIpPortId = ODBCCfgASIpPortId + ODBCCfgIpPortRange;
	if (ODBCCfgIpPortRange < ODBCCfgDefMaxCnt+1) // 1 is for Assoc Server
		ODBCCfgDefMaxCnt = ODBCCfgIpPortRange-1;
	if (ODBCCfgDefMaxCnt != NO_MAX_SRVR_LIMIT)
	{
		if (ODBCCfgDefMaxCnt < ODBCCfgDefInitCnt)
			ODBCCfgDefInitCnt = ODBCCfgDefMaxCnt;
		if (ODBCCfgDefMaxCnt <= ODBCCfgDefAvailCnt)
			ODBCCfgDefAvailCnt = ODBCCfgDefMaxCnt;
	}


	RegCloseKey( kCurrKeyValue );
#else
	TandemIdVal[0] = 0;
	ImagePathVal[0] = 0;
	SQLSystemVolumeNmVal[0] = '\0';
	ODBCCfgAutomaticStart = 0;
	strcpy(ODBCCfgASIpAddress, "127.0.0.1");
	ODBCDbgDetail = 0;
	ODBCCfgASIpPortId  = 18650;
	ODBCCfgIpPortRange = 250;
	ODBCCfgIpPortIdMax = ODBCCfgASIpPortId + 100;
	ODBCCfgPrimaryNode  = 0;
	ODBCCfgVersion[0] = '\0';
	ODBCCfgAnchorFileNm[0] = '0';
	ODBCCfgDefInitCnt = 1;
	ODBCCfgDefAvailCnt = 1;
	ODBCCfgDefMaxCnt = NO_MAX_SRVR_LIMIT;
	ODBCOleCfgSrvrIpPortId = ODBCCfgASIpPortId + ODBCCfgIpPortRange;
	if (ODBCCfgIpPortRange < ODBCCfgDefMaxCnt+1) // 1 is for Assoc Server
		ODBCCfgDefMaxCnt = ODBCCfgIpPortRange-1;
	if (ODBCCfgDefMaxCnt != NO_MAX_SRVR_LIMIT)
	{
		if (ODBCCfgDefMaxCnt < ODBCCfgDefInitCnt)
			ODBCCfgDefInitCnt = ODBCCfgDefMaxCnt;
		if (ODBCCfgDefMaxCnt <= ODBCCfgDefAvailCnt)
			ODBCCfgDefAvailCnt = ODBCCfgDefMaxCnt;
	}
#endif
	return ERROR_SUCCESS;

}

char*	RegValues::_ODBCCfgAnchorFileNm ( void ){
	return (char*)ODBCCfgAnchorFileNm;
}

char*	RegValues::_ODBCCfgASIpAddress ( void ) {
	return (char*)ODBCCfgASIpAddress;
}



long RegValues::_ODBCCfgASIpPortId ( void ) {
	return (long)ODBCCfgASIpPortId;
}

char* RegValues::_SQLSystemVolQualifier ( void ) {
	return (char*)SQLSystemVolQualifier;
}

char* RegValues::_SQLSystemVolumeNmVal ( void ) {
	return (char*)SQLSystemVolumeNmVal;
}

char* RegValues::_NodeNm ( long NodeNum ) {
	if (_NODEOn( NodeNum ) == FALSE )
		NodeNum = 0;

	return (char *)NodeDescList[NodeNum].NetBiosName;
}

short RegValues::_NodeNum ( char*	pzNodeNm )
{
	short i;

	for (i = 0; i < MAX_NODE_CNT; i++)
	{
		if (_NODEOn( i ) == FALSE ) continue;

		if (strcmp((char*)(_NodeNm(i)),
			       (char*)pzNodeNm) == 0)
			return i;
	}
    // not found, so return -1
	return -1;

}


long RegValues::_ODBCCfgFailOver ( void ) {
	return (long)ODBCCfgFailOver;
}

void RegValues::SET_FailOverDisabled ( void ) {
	ODBCCfgFailOver = -1;
}
long RegValues::_ODBCCfgPrimaryNode ( void ) {
	return (long)ODBCCfgPrimaryNode;
}

long RegValues::_ODBCOleCfgSrvrIpPortId  ( void ) {
	return (long)ODBCOleCfgSrvrIpPortId ;
}

long RegValues::_ODBCCfgIpPortRange ( void ) {
	return (long)ODBCCfgIpPortRange;
}

long RegValues::_ODBCCfgIpPortIdMax ( void ) {
	ODBCCfgIpPortIdMax = ODBCCfgASIpPortId + ODBCCfgIpPortRange - 1;
	return (long)ODBCCfgIpPortIdMax;
}

long RegValues::_ODBCCfgVersion ( void ) {
	return (long)atol(ODBCCfgVersion);
}

long RegValues::_ODBCCfgDefInitCnt ( void ) {
	return (long)ODBCCfgDefInitCnt;
}

long RegValues::_ODBCCfgDefAvailCnt ( void ) {
	return (long)ODBCCfgDefAvailCnt;
}

long RegValues::_ODBCCfgDefMaxCnt ( void ) {
	return (long)ODBCCfgDefMaxCnt;
}

void  RegValues::SET_ODBCASObjRef ( void ) {
	sprintf(ODBCASObjRef
			, "TCP:%0.100s/%5d:ODBC"
			, ODBCCfgASIpAddress
			, ODBCCfgASIpPortId) ;
	return;
}

char* RegValues::_ODBCASObjRef ( void ) {
	if (strlen(ODBCASObjRef) == 0) {
		sprintf(ODBCASObjRef
			, "TCP:%0.100s/%5d:ODBC"
			, ODBCCfgASIpAddress
			, ODBCCfgASIpPortId) ;
	}
	return (char*)ODBCASObjRef;
}

char* RegValues::_LocalComputerName ( void ) {
	DWORD    dwMyNodeNmLen;
	DWORD			dwError;

	if (strlen(LocalComputerName) == 0) {
		dwMyNodeNmLen = sizeof (LocalComputerName);
		dwError = GetComputerName(LocalComputerName, &dwMyNodeNmLen);
		if (!dwError)
		{
#ifdef WIN32
			dwError = GetLastError ();
#endif
			LocalComputerName[0] = '\0';
		}
	}

	return (char*) LocalComputerName;
}

short RegValues::_MyNodeNum ( void )
{
	// Get my node number
	for (short myNode = 0; myNode < MAX_NODE_CNT; ++myNode)
	{
		if (_NODEOn( myNode ) == FALSE ) continue;

		if (strcmp((char*)(NodeDescList[myNode].NetBiosName),
			       (char*)(_LocalComputerName())) == 0)
			return myNode;
	}

	return 0;

}


long RegValues::_ODBCDbgDetail ( void ) {
	return (long)ODBCDbgDetail;
}

long RegValues::_ODBCEventLevel ( void ) {
	return (long)ODBCEventLevel;
}
long InstantiateRGObject()
{
	long error;


	if (RG == NULL)
	{
		RG = new RegValues();
		if ((error = RG->SET_ODBCCfgVals()) != ERROR_SUCCESS)
			return error;
	}


	return ERROR_SUCCESS;
}


// added as part of Services addition
BOOL    RegValues::_bIsPrimary (void) {
	if (_ODBCCfgPrimaryNode() == _MyNodeNum())
		return TRUE;
	return FALSE;
}
//LCOV_EXCL_STOP
