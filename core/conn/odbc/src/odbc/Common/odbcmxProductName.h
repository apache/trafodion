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
#ifndef _ODBCMXPRODUCTNAME_DEFINED
#define _ODBCMXPRODUCTNAME_DEFINED

#define ODBCMX_SERVICE "ODBC/MX Service"
#define ODBCMX_SERVERWRAPPER "ODBC/MX Server Wrapper"
#define ODBCMX_SERVER "ODBC/MX Server"
#define ODBCMX_CONFIGSERVER "ODBC/MX Configuration Server"
#define ODBCMX_CONFIGCLIENT "ODBC/MX Configuration Client"
#define ODBCMX_ASSERVER "ODBC/MX Association Server"
#define ODBCMX_INIT "ODBC/MX Initialization Program"
#define ODBCMX_OLESERVER "ODBC/MX OLE Server"
// tja 5/24/98
// added for FailOver.  
#define ODBCMX_CASERVICE "ODBC/MX Cluster Administrator Service"
#define ODBCMX_CASERVER  "ODBC/MX Cluster Administrator Server" 

// AS :: Association Services
#ifdef WIN32
#define ODBCMX_AS_EXE_NM		"tdm_odbcService.exe" 
#else
#define ODBCMX_AS_EXE_NM		"mxoas" 
#endif

#define ODBCMX_AS_DLL_NM		"tdm_odbcASSrvr.dll"

// CA :: Cluster Administrator Services
#ifdef WIN32
#define ODBCMX_CA_EXE_NM		"tdm_odbcCASrvr.exe"
#else
#define ODBCMX_CA_EXE_NM		"mxoca"
#endif

#define ODBCMX_CA_DLL_NM		"tdm_odbcCASvc.dll"

// CFG :: Configuration Services
#ifdef WIN32
#define ODBCMX_CFG_EXE_NM		"tdm_odbcCfgServer.exe"
#else
#define ODBCMX_CFG_EXE_NM		"mxocfg"
#endif

#define ODBCMX_CFG_SV_DLL_NM	"tdm_odbcCfgSv.dll"
#define	ODBCMX_CFG_CL_DLL_NM	"tdm_odbcCfgCl.dll"

// INIT :: Initialization Services
#ifdef WIN32
#define ODBCMX_INIT_EXE_NM		"tdm_odbcInit.exe"
#else
#define ODBCMX_INIT_EXE_NM		"mxoinit"
#endif

#define ODBCMX_INIT_DLL_NM		"tdm_odbcInitSv.dll"

// ODBC :: ODBC/SQL Services
#ifdef WIN32
#define ODBCMX_ODBC_EXE_NM		"tdm_odbcServer.exe"
#else
#define ODBCMX_ODBC_EXE_NM		"mxosrvr"
#define ODBCMX_QS_MGR_NM		"qsmgr"
#define ODBCMX_QS_STATS_NM		"qsstats"
#define ODBCMX_QS_COM_NM		"qscom"
#define ODBCMX_QS_SYNC_NM		"qssync"
#define ODBCMX_QS_RULE_NM		"qsrule"
#define ODBCMX_QS_OFFND_NM		"qsoffndr"
#define ODBCMX_QS_RECEV_NM		"qsrecev"
#define ODBCMX_QS_SEND_NM		"qssend"
#define ODBCMX_QS_MGR2_NM		"qsmgr2"
#define ODBCMX_HQS_STATS_EXE_NM "hqsstats"
#endif

#define ODBCMX_ODBC_DLL_NM		"tdm_odbcCore.dll"

// EVT  :: ODBC Event Message Services
#define ODBCMX_EVT_DLL_NM		"tdm_odbcEvent.dll"

// MGMT :: OLE Management Services
#ifdef WIN32
#define ODBCMX_MGMT_EXE_NM		"tdm_odbcOLE.exe"
#else
#define ODBCMX_MGMT_EXE_NM		"mxoole"
#endif

#define ODBCMX_MGMT_SET_EXE_NM	"tdm_odbcOLE_SetPath.exe"

// MSGS :: Message Services
#define	ODBCMX_MSG_CL_DLL_NM	"tdm_odbcDrvrMsg_Intl.dll"

#endif
