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
// RegValues.h
#ifndef RegValues_H_DEFINED_
#define RegValues_H_DEFINED_

// NOTE: There is a problem compiling this file under the
//       driver project (which is an MFC DLL) and any server
//       project (which in NOT an MFC DLL).  The following
//       #ifdef takes care of those problems.
//
#ifdef _AFXDLL
#include <stdafx.h>
#else
#include <platform_ndcs.h>
#endif

#include <stdio.h>

#include "Global.h"

//#include "OMXTypes.h"
#define MAX_IPADDRESS_LEN		100
#define	MAX_FILENM_LEN			1024
#define MAX_NETBIOS_NAME_LEN    256
#define MAX_SID_LEN				68
#define	MAX_SQL_ID_LEN			128
#define MAX_NODE_CNT			16
#define MAX_ANON_LOGON_DS_LST   2048
#define LOCAL_HOST_IP_ADDRESS	"127.0.0.0"


// strings used to build RegValues lookup keys
#define MAX_REGKEY_PATH_LEN		1000
#define _CLUSTER_NAME_VAL_NM	"Cluster-Name"
#define _IMAGE_PATH_VAL_NM		"ImagePath"
#define _BOOT_CONFIG_VAL_NM		"Boot-Config"
#define _CLUSTER_KEY			"CLUSTER"
#define _NODE_KEY				"NODE"
#define _NETBIOS_VAL_NM			"NetBiosName"
#define _DNSNM_VAL_NM			"DNSName"
#define _ODBC_KEY				"ODBC"
#define _VERSION_VAL_NM			"Version"
#define _ANCHOR_FILE_VAL_NM		"Anchor-File"
#define _AUTOMATIC_START_VAL_NM	"AutomaticStart"
#define _FAILOVER_VAL_NM		"FailOver"
#define _PRIMARY_VAL_NM			"PrimaryNode"
#define _EXTERNAL_IPADDRESS_VAL_NM	"ExternalIPAddress"
#define _ODBC_ENABLED4NODE		"ODBCEnabled4Node"
#define _IPADDRESS_VAL_NM	    "IPAddress"
#define _IPPORT_ID_VAL_NM		"IpPortId"
// obsoleted Registry Value as of SQL/MX Install, 5/30/98
#define _IPPORT_RANGE_VAL_NM	"IpPortRange"
#define _IPPORT_MAX_VAL_NM		"IpPortIdMax"
#define _DEF_AVAIL_CNT			"DefAvailCnt"
#define _DEF_INIT_CNT			"DefInitCnt"
#define _DEF_MAX_CNT			"DefMaxCnt"
//#define _DBG_PRESTART_CFG_SVC	"DbgPreStartCfgSvc"
//#define _DBG_PRESTART_SQL_SVC	"DbgPreStartSQLSvc"
#define _DBG_FLAG				"DebugFlag"
#define _EVENT_LEVEL			"EventLevel"
// tja 10/15/98
// added to detect location of the SQL system catalog
// so that determination of SQL installation completion
// possible
#define _SQL_KEY				"SQL"
#define _SQL_SYSTEM_VOLUME_NM	"System-Volume"
#define _DP2_KEY				"DP2"
#define _DIRECTORY_VAL_NM		"Directory"


#define ANON_LOGON_NOT_ALLOWED 0
#define ANON_LOGON_ALLOWED	   1
	// not RW implies read only access
#define ANON_LOGON_RW_ALLOWED  2


typedef struct	RGNodeDesc_t {
		char	IpAddress[MAX_IPADDRESS_LEN + 1];
		char	NetBiosName[MAX_NETBIOS_NAME_LEN + 1];
		char	ExternalIpAddress[MAX_IPADDRESS_LEN + 1];
		BOOL	ODBCEnabled4Node;
	} RGNodeDesc_def;

#define MAX_NODES_PER_CLUSTER	16


class RegValues
{
public:

	void	SET_BootCfgVal ( char* pNewValue );
	void	SET_ImagePathVal ( char* pNewValue );
	void	SET_TandemIdVal ( char* pNewValue );

	BOOL	_NODEOn( long NodeNum );
	BOOL	_NODEOn( long NodeNum , long CPUList );
	long    _NODEMask( void );
	void	SET_NODEExternalIpAddr ( long NodeNum
						  ,char *pODBCIpAddress );
	char*   _CfgNODEExternalIpAddr ( long NodeNum );
	char*	_CfgNODENetBiosNm ( long NodeNum );
	char*	_NodeNm ( long NodeNum );
	short   _NodeNum ( char* pzNodeNm );
	long	SET_ODBCCfgVals ();
    char*	_ODBCCfgAnchorFileNm ( void );
	char*	_ODBCCfgASIpAddress ( void );
	long    _ODBCCfgFailOver ( void );
	void	SET_FailOverDisabled ( void );
	void    SET_NodeDisabled ( long NodeNum );
	long	_ODBCCfgPrimaryNode ( void );
	long    _ODBCCfgASIpPortId ( void );
	void    SET_ODBCASObjRef ( void );
	char *  _ODBCASObjRef ( void );
	void    SET_ODBCCAObjRef ( void );
    void	SET_ODBCCAObjRef ( char* pzNewObjRefValue );
	char *  _ODBCCAObjRef ( void );
	short	_MyNodeNum ( void );
	char *	_LocalComputerName ( void );
	long    _ODBCOleCfgSrvrIpPortId ( void );
    long    _ODBCCfgIpPortRange ( void );
	long	_ODBCCfgIpPortIdMax ( void );
	long    _ODBCCfgVersion ( void );
	long    _ODBCCfgDefInitCnt ( void );
	long    _ODBCCfgDefMaxCnt ( void );
	long    _ODBCCfgDefAvailCnt ( void );
	char*   _CfgNODENm (long NodeNum ) ;
	BOOL	_CfgODBCEnabled4Node ( long NodeNum );
	long	_ODBCDbgDetail ( void );
	long    _ODBCEventLevel (void);
	BOOL	_bIsPrimary ( void );
	char*	_SQLSystemVolQualifier ( void );
	char*	_SQLSystemVolumeNmVal ( void );
	char *	_ImagePathVal () { return ImagePathVal; };
    void	_SetIpPortRange( long IpPortRange ) {ODBCCfgIpPortRange=IpPortRange;}
	long	_CPUCnt (char *CPUList, short neoMaxNode);
	bool	_CPUspawnOn(short& nextCPUspawn, long& cpuNumber, short& segmentIndex, NEO_SEGMENT* neoSegment, short segmentMax);
	RegValues();
	~RegValues();

private:

	char	BootCfgPath[MAX_REGKEY_PATH_LEN + 1] ;
	char	TandemIdVal[MAX_SID_LEN + 1] ;
	char	ImagePathVal[MAX_FILENM_LEN + 1];
	char	ClusterCfgPath[MAX_REGKEY_PATH_LEN + 1];
	char	ODBCCfgPath[MAX_REGKEY_PATH_LEN + 1] ;
    char    ODBCCfgAnchorFileNm[MAX_FILENM_LEN + 1];
	char	ODBCCfgVersion[10];
	long	ODBCCfgAutomaticStart;
	long	ODBCCfgFailOver;
	long	ODBCCfgPrimaryNode;
	char    ODBCCfgASIpAddress[MAX_IPADDRESS_LEN + 1];
	long    ODBCCfgASIpPortId ;
	char	ODBCASObjRef[128];
	char    LocalComputerName[128];
	long	ODBCOleCfgSrvrIpPortId;
	long    ODBCCfgIpPortRange;
	long	ODBCCfgIpPortIdMax;
	long    ODBCCfgDefInitCnt;
	long    ODBCCfgDefMaxCnt;
	long    ODBCCfgDefAvailCnt;
	long	ODBCDbgDetail;
	long	ODBCEventLevel;
	long	NODECnt;
	long	NODEMask;
	  RGNodeDesc_def	NodeDescList[MAX_NODE_CNT];
	char	SQLSystemVolQualifier[MAX_FILENM_LEN + 1];
	char	SQLSystemVolumeNmVal[MAX_FILENM_LEN + 1]; // Used by tdm_Odbcinit.exe

	long	CPUCnt;
	char	*CPUMask; // In expand, nodes can go upto 256 and 16 CPUs per node
};

extern long InstantiateRGObject();
extern RegValues *RG;
#endif
