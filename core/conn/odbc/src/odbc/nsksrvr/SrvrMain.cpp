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


#include <dlfcn.h>

#include <platform_ndcs.h>
#include <platform_utils.h>
#include "Global.h"
#include "tdm_odbcSrvrMsg.h"
#include "commonFunctions.h"
#include "signal.h"
#include "ODBCMXTraceMsgs.h"
#include <errno.h>
#include "Transport.h"
#include "Listener_srvr.h"
#include "ResStatisticsSession.h"
#include "ResStatisticsStatement.h"

// Version helpers
#include "ndcsversion.h"

#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <ifaddrs.h>


#include <new>
#include <stdio.h>

#include <list>

#include "CommonLogger.h"
#include "zookeeper/zookeeper.h"
#include "PubInterface.h"

// +++ Move below to srvrGlobal if needed

//ZK_GLOBAL_Def zkGlobals;

zhandle_t *zh;

clientid_t myid;
pthread_cond_t cond;
pthread_mutex_t lock;
int shutdownThisThing;

stringstream availSrvrNode;
string dcsRegisteredNode;
string availSrvrData;
string regSrvrData;
char regZnodeName[256];
char hostname[256];
char instanceId[8];
char childId[8];
char zkHost[256];
int myNid;
int myPid;
string myProcName;
int sdconn;
char zkRootNode[256];
int clientConnTimeOut;
int zkSessionTimeout;
short stopOnDisconnect;
char trafIPAddr[20];
int aggrInterval;
int statisticsCacheSize;
int queryPubThreshold;
statistics_type statisticsPubType;
bool bStatisticsEnabled;
long maxHeapPctExit;
long initSessMemSize;
int portMapToSecs = -1;
int portBindToSecs = -1;
bool bPlanEnabled = false;
bool keepaliveStatus = false;
int keepaliveIdletime;
int keepaliveIntervaltime;
int keepaliveRetrycount;
long epoch = -1;
void watcher(zhandle_t *zzh, int type, int state, const char *path, void *watcherCtx);
bool verifyPortAvailable(const char * idForPort, int portNumber);
BOOL getInitParamSrvr(int argc, char *argv[], SRVR_INIT_PARAM_Def &initParam, char* strName, char* strValue);
long  getEpoch(zhandle_t *zh);

//only support positive number
BOOL getNumberTemp( char* strValue, int& nValue )
{
	if( strspn( strValue, "0123456789" ) != strlen( strValue) )
	   return FALSE;
	nValue = atol(strValue);
	return TRUE;
}

void initTcpProcessNameTemp(char *tcpName )
{
	strcpy(tcpName, DEFAULT_TCP_PROCESS);
}

static void free_String_vector(struct String_vector *v)
{
    if (v->data)
    {
        for (int32_t i=0; i < v->count; i++)
        {
            free(v->data[i]);
        }
        free(v->data);
        v->data = NULL;
        v->count = 0;
    }
}

void SQL_EXECDIRECT(SRVR_INIT_PARAM_Def* initParam);
short SQL_EXECDIRECT_FETCH(SRVR_INIT_PARAM_Def* initParam);

// void T7970N29_18JUN2010_SRV_0422 (void){}

/******************* SLICE Header Fragment *******************/
#include  <stdio.h>
// void T7970N27_19FEB2010_ALF_SRV_0208 (void){}

char errStrBuf1[141];
char errStrBuf2[141];
char errStrBuf3[141];
char errStrBuf4[141];
char errStrBuf5[141];

char *sCapsuleName = "ODBC/MX Server";
extern SRVR_GLOBAL_Def *srvrGlobal;
extern ResStatisticsSession    *resStatSession;
extern ResStatisticsStatement  *resStatStatement;

dlHandle hzCmdDll = 0;

char PrimarySystemNm[128];
char PrimaryCatalog[MAX_SQL_IDENTIFIER_LEN+1];
char errorMsg[256];
void* ssn;
char tempSqlString[1500];
const char* tempSqlStringPtr ;
IDL_short tempSqlStmtType;
bool informas = true;
bool sqlflag = false;

extern "C" void
ImplInit (
	const CEE_handle_def  *objectHandle,
	const char            *initParam,
	long                  initParamLen,
	CEE_status            *returnSts,
	CEE_tag_def           *objTag,
	CEE_handle_def        *implementationHandle);

CEE_status runCEE(char* TcpProcessName, long portNumber, int TransportTrace)
{
	return (GTransport.m_listener->runProgram(TcpProcessName, portNumber, TransportTrace));
}

void mxosrvr_atexit_function(void)
{
  file_mon_process_shutdown();
}

int main(int argc, char *argv[], char *envp[])
{
	INITSRVRTRC

	CEE_status				sts = CEE_SUCCESS;
	SRVR_INIT_PARAM_Def		initParam;
	DWORD					processId;
	char					tmpString[128];
	char					tmpString2[32];
	char					tmpString3[512];
	CEECFG_Transport		transport;
	CEECFG_TcpPortNumber	portNumber;
	BOOL					retcode;
	IDL_OBJECT_def			srvrObjRef;
	CEECFG_TcpProcessName	TcpProcessName;
	int						TransportTrace = 0;

	CALL_COMP_DOVERS(ndcs,argc,argv);

try
{
	regZnodeName[0] = '\x0';
	zkHost[0] = '\x0';
	zkRootNode[0] = '\x0';

	// Initialize seabed
	int	sbResult;
	char buffer[FILENAME_MAX] = {0};
	bzero(buffer, sizeof(buffer));

	sbResult = file_init_attach(&argc, &argv, true, buffer);
	if(sbResult != XZFIL_ERR_OK){
		exit(3);
	}
	sbResult = file_mon_process_startup(true);
	if(sbResult != XZFIL_ERR_OK){
		exit(3);
	}
	msg_mon_enable_mon_messages(true);
}
catch(SB_Fatal_Excep sbfe)
{
	exit(3);
}

	sigset_t newset, oldset;
	sigemptyset(&newset);
	sigaddset(&newset,SIGQUIT);
	sigaddset(&newset,SIGTERM);
	sigprocmask(SIG_BLOCK,&newset,&oldset);

	processId = GetCurrentProcessId();

	 retcode = getInitParamSrvr(argc, argv, initParam, tmpString, tmpString3);
	retcode = TRUE;

	mxosrvr_init_seabed_trace_dll();
	atexit(mxosrvr_atexit_function);

	// +++ Todo: Duplicating calls here. Should try to persist in srvrGlobal
	MS_Mon_Process_Info_Type  proc_info;
	msg_mon_get_process_info_detail(NULL, &proc_info);
	myNid = proc_info.nid;
	myPid = proc_info.pid;
	myProcName = proc_info.process_name;

	char logNameSuffix[32];
        const char *lv_configFileName = "log4cxx.trafodion.sql.config";
        bool singleSqlLogFile = TRUE;
        if (getenv("TRAF_MULTIPLE_SQL_LOG_FILE"))
           singleSqlLogFile = FALSE;
        if (singleSqlLogFile) {
	   sprintf( logNameSuffix, "_%d.log", myNid );
           lv_configFileName = "log4cxx.trafodion.sql.config";
        }
        else 
        {
	   sprintf( logNameSuffix, "_%d_%d.log", myNid, myPid );
           lv_configFileName = "log4cxx.trafodion.masterexe.config";
        }
	CommonLogger::instance().initLog4cxx(lv_configFileName, logNameSuffix);

    if(retcode == FALSE )
   {
//LCOV_EXCL_START
      SendEventMsg(  MSG_SET_SRVR_CONTEXT_FAILED,
                           EVENTLOG_ERROR_TYPE,
                           processId,
                           ODBCMX_SERVER,
                           srvrObjRef,
                           2,
                           tmpString,
                           tmpString3);
      exit(0);
//LCOV_EXCL_STOP
   }

   GTransport.initialize();
   if(GTransport.error != 0 )
   {
//LCOV_EXCL_START
      SendEventMsg(  MSG_SET_SRVR_CONTEXT_FAILED,
                           EVENTLOG_ERROR_TYPE,
                           processId,
                           ODBCMX_SERVER,
                           srvrObjRef,
                           1,
                           GTransport.error_message);
      exit(0);
//LCOV_EXCL_STOP
   }
   chdir(GTransport.myPathname);

   initParam.srvrType = CORE_SRVR;

//LCOV_EXCL_START
   if (initParam.debugFlag & SRVR_DEBUG_BREAK)
   {
        volatile int done = 0;
        while (!done) {
          sleep(10);
        }
   }
//LCOV_EXCL_STOP

	char zkErrStr[2048];
	stringstream zk_ip_port;
//	zoo_set_debug_level(ZOO_LOG_LEVEL_DEBUG);
	if( zkHost[0] == '\x0' && regZnodeName[0] == '\x0' )
	{
		sprintf(zkErrStr, "***** Cannot get Zookeeper properties or registered znode info from startup params");
		SendEventMsg(  MSG_SET_SRVR_CONTEXT_FAILED,
						   EVENTLOG_ERROR_TYPE,
						   processId,
						   ODBCMX_SERVER,
						   srvrObjRef,
						   1,
						   zkErrStr);
		// exit(1);

	}
	else
	{
		zk_ip_port << zkHost;
		sprintf(zkErrStr, "zk_ip_port is: %s", zk_ip_port.str().c_str());
		SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE,
					processId, ODBCMX_SERVER,
					srvrObjRef, 1, zkErrStr);
	}

	if (initParam.debugFlag & SRVR_DEBUG_BREAK)
		zkSessionTimeout = 600;

	zoo_deterministic_conn_order(1); // enable deterministic order
	zh = zookeeper_init(zk_ip_port.str().c_str(), watcher, zkSessionTimeout * 1000, &myid, 0, 0);
	if (zh == 0){
		sprintf(zkErrStr, "***** zookeeper_init() failed for host:port %s",zk_ip_port.str().c_str());
		SendEventMsg(  MSG_SET_SRVR_CONTEXT_FAILED,
						   EVENTLOG_ERROR_TYPE,
						   processId,
						   ODBCMX_SERVER,
						   srvrObjRef,
						   1,
						   zkErrStr);
		// exit(1);
	}

	bool found = false;
	int rc;
	stringstream ss;
	ss.str("");
	ss << zkRootNode << "/dcs/master";
	string dcsMaster(ss.str());
	Stat stat;
	int startPortNum = 0, portRangeNum;
	char masterHostName[MAX_HOST_NAME_LEN];
	char startPort[12], portRange[12], masterTS[24];
	struct String_vector children;
	children.count = 0;
	children.data = NULL;

	// Get the instance ID from registered node
	char *tkn;
	char tmpStr[256];
	strcpy( tmpStr,  regZnodeName );
	tkn = strtok(tmpStr, ":" );			
	if(tkn!=NULL)
		strcpy(hostname,tkn);
	tkn = strtok(NULL, ":" );
	if( tkn != NULL )
		strcpy( instanceId, tkn );
	tkn = strtok(NULL, ":" );
	if( tkn != NULL )
		strcpy( childId, tkn );
	else
		;	// +++ Todo handle error

	while(!found)
	{
		rc = zoo_exists(zh, dcsMaster.c_str(), 0, &stat);
		if( rc == ZNONODE )
			continue;
		else
		if( rc == ZOK )
		{
			rc = zoo_get_children(zh, dcsMaster.c_str(), 0, &children);
			if( children.count > 0 )
			{
				char zknodeName[2048];
				strcpy(zknodeName, children.data[0]);
				tkn = strtok(zknodeName, ":" );
				if( tkn != NULL )
					strcpy( masterHostName, tkn );

				tkn = strtok(NULL, ":" );
				if( tkn != NULL ) {
					strcpy( startPort, tkn );
					startPortNum = atoi(tkn);
				}

				tkn = strtok(NULL, ":" );
				if( tkn != NULL ) {
					strcpy( portRange, tkn );
					portRangeNum = atoi(tkn);
				}

				tkn = strtok(NULL, ":" );
				if( tkn != NULL )
					strcpy( masterTS, tkn );

				free_String_vector(&children);
				found = true;
			}
			else
				continue;
		}
		else	// error
		{
			sprintf(zkErrStr, "***** zoo_exists() for %s failed with error %d",dcsMaster.c_str(), rc);
			SendEventMsg(  MSG_SET_SRVR_CONTEXT_FAILED,
							   EVENTLOG_ERROR_TYPE,
							   processId,
							   ODBCMX_SERVER,
							   srvrObjRef,
							   1,
							   zkErrStr);
			break;
		}
	}

	// Initialize initparam to defaults
	initParam.transport = CEE_TRANSPORT_TCP;	// -T 3
	initParam.majorVersion = 3; 				// -V 3
	// Will need to remove $ZTC0 and NonStopODBC from below
	sprintf( initParam.asSrvrObjRef, "TCP:$ZTC0/%s:NonStopODBC", startPort);	// -A TCP:$ZTC0/52500:NonStopODBC
	// Will need to remove this after we get rid off all existing AS related processing
	sprintf( initParam.ASProcessName, "$MXOAS" );	// -AS $MXOAS
	// Will need to remove this after we get rid off all existing WMS related processing
	sprintf( initParam.QSProcessName, "$ZWMGR" );	// -QS $ZWMGR

	// moved this here from begining of the function
	BUILD_OBJECTREF(initParam.asSrvrObjRef, srvrObjRef, "NonStopODBC", initParam.portNumber);

	ss.str("");
	ss << zkRootNode << "/dcs/servers/registered";
	string dcsRegistered(ss.str());

	char realpath[1024];
	bool zk_error = false;

 	if( found )
	{
		sprintf(zkErrStr, "Found master node in Zookeeper");
		SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE,
					processId, ODBCMX_SERVER,
					srvrObjRef, 1, zkErrStr);

		found = false;
		while(!found)
		{
			rc = zoo_exists(zh, dcsRegistered.c_str(), 0, &stat);
			if( rc == ZNONODE )
				continue;
			else
			if( rc == ZOK )
			{
				int i;
				//This section is the original port finding mechanism.
				//All servers (the herd) start looking for any available port
				//between starting port number+2 through port range max.
				//This is mainly for backward compatability for DcsServers
				//that don't pass PORTMAPTOSECS and PORTBINDTOSECS param
				if(portMapToSecs == -1 && portBindToSecs == -1) {
					for(i = startPortNum+2; i < startPortNum+portRangeNum; i++) {
						if (GTransport.m_listener->verifyPortAvailable("SRVR", i))
							break;
					}

					if( i == startPortNum+portRangeNum )
					{
						zk_error = true;
						sprintf(zkErrStr, "***** No ports free");
						break;
					}
				} else {
					//This section is for new port map params, PORTMAPTOSECS and PORTBINDTOSECS,
					//passed in by DcsServer. DcsMaster writes the port map to data portion of
					//<username>/dcs/servers/registered znode. Wait PORTMAPTOSECS for port map
					//to appear in registered znode. When it appears read it and scan looking for
					//match of instance and child Id.
					long retryTimeout = 500;//.5 second
					long long timeout = JULIANTIMESTAMP();
					bool isPortsMapped = false;
					char *zkData = new char[1000000];
					int zkDataLen = 1000000;
					while(! isPortsMapped) {
						memset(zkData,0,1000000);
						rc = zoo_get(zh, dcsRegistered.c_str(), false, zkData, &zkDataLen, &stat);
						if( rc == ZOK && zkDataLen > 0 ) {
							sprintf(zkErrStr, "DCS port map = %s", zkData);
							SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE,
									processId, ODBCMX_SERVER,
									srvrObjRef, 1, zkErrStr);

							int myInstanceId = atoi(instanceId);
							int myChildId = atoi(childId);

							sprintf(zkErrStr, "Searching for my id (%d:%d) in port map",myInstanceId,myChildId);
							SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE,
									processId, ODBCMX_SERVER,
									srvrObjRef, 1, zkErrStr);

							char portMapInstanceId[8];
							char portMapChildId[8];
							char portMapPortNum[8];
							char* saveptr;
							char* token = strtok_r (zkData,":",&saveptr);
							while (token != NULL)
							{
								if( token != NULL )//instance Id
									strcpy( portMapInstanceId, token );
								token = strtok_r(NULL, ":",&saveptr);
								if( token != NULL )//child id
									strcpy( portMapChildId, token );
								token = strtok_r(NULL, ":",&saveptr);
								if( token != NULL )//port number
									strcpy( portMapPortNum, token );

								int currPortMapInstanceId = atoi(portMapInstanceId);
								int currPortMapChildId = atoi(portMapChildId);
								int currPortMapPortNum = atoi(portMapPortNum);

								if(myInstanceId == currPortMapInstanceId && myChildId == currPortMapChildId) {
									i = currPortMapPortNum;
									sprintf(zkErrStr, "Found my port number = %d in port map", i);
									SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE,
											processId, ODBCMX_SERVER,
											srvrObjRef, 1, zkErrStr);
									break;
								} else {
									token = strtok_r (NULL, ":",&saveptr);
								}
							}

							timeout = JULIANTIMESTAMP();
							bool isAvailable = false;
							while ( isAvailable == false ) {
								if (GTransport.m_listener->verifyPortAvailable("SRVR", i)) {
									isAvailable = true;
								} else {
									if((JULIANTIMESTAMP() - timeout) > (portBindToSecs * 1000000)) {
										sprintf(zkErrStr, "Port bind timeout...exiting");
										zk_error = true;
										break;
									} else {
										sprintf(zkErrStr, "Port = %d is already in use...retrying", i);
										SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE,
												processId, ODBCMX_SERVER,
												srvrObjRef, 1, zkErrStr);
										DELAY(retryTimeout);
									}
								}
							}

							isPortsMapped = true;

						} else {
							if((JULIANTIMESTAMP() - timeout) > (portMapToSecs * 1000000)) {
								sprintf(zkErrStr, "Port map read timeout...exiting");
								zk_error = true;
								break;
							} else {
								sprintf(zkErrStr, "Waiting for port map");
								SendEventMsg(MSG_SERVER_TRACE_INFO, EVENTLOG_INFORMATION_TYPE,
										processId, ODBCMX_SERVER,
										srvrObjRef, 1, zkErrStr);
								DELAY(retryTimeout);
								rc = zoo_exists(zh, dcsRegistered.c_str(), 0, &stat);
							}
						}
					}

					delete[] zkData;
				}

				initParam.portNumber = i;

				stringstream newpath;
				newpath.str("");
				newpath << dcsRegistered.c_str() << "/" << regZnodeName;
//				dcsRegisteredNode.str("");
//				dcsRegisteredNode << dcsRegistered.c_str() << "/" << regZnodeName;
				dcsRegisteredNode = newpath.str();

				ss.str("");
				ss << myPid;
				string pid(ss.str());

				ss.str("");
				ss << "STARTING"
				   << ":"
				   << JULIANTIMESTAMP()
				   << ":"
				   << ":"				// Dialogue ID
				   << myNid
				   << ":"
				   << myPid
				   << ":"
				   << myProcName.c_str()
				   << ":"			   		// Server IP address
				   << ":"					// Server Port
				   << ":"					// Client computer name
				   << ":"					// Client address
				   << ":"					// Client port
				   << ":"					// Client Appl name
				   << ":";

				regSrvrData = ss.str();

				rc = zoo_create(zh, dcsRegisteredNode.c_str(), regSrvrData.c_str(), regSrvrData.length(), &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, realpath, sizeof(realpath)-1);
				if( rc != ZOK )
				{
					zk_error = true;
					sprintf(zkErrStr, "***** zoo_create() failed with error %d", rc);
					break;
				}
				found = true;
			}
			else	// error
			{
				zk_error = true;
				sprintf(zkErrStr, "***** zoo_exists() for %s failed with error %d",dcsRegistered.c_str(), rc);
				break;
			}
		}
	}

	if( zk_error ) {
		SendEventMsg(  MSG_SET_SRVR_CONTEXT_FAILED,
					   EVENTLOG_ERROR_TYPE,
					   processId,
					   ODBCMX_SERVER,
					   srvrObjRef,
					   1,
					   zkErrStr);
		exit(1);
	}

        // get the current epoch from zookeeper and also put a watch on it
        // (to be even safer, take epoch as a command line arg)
        epoch = getEpoch(zh);

//LCOV_EXCL_START
// when a server dies, the MXOAS sends message to CFG. CFG creates the MXOSRVR process
// and passess only one command line atribute: -SQL CLEANUP OBSOLETE VOLATILE TABLES
// It is for cleanup resources (volatile tables).
// Newly created MXOSRVR process executes CLEANUP OBSOLETE VOLATILE TABLES and exits.
   // (This process is not managed by AS!. It is only a helper.

	if (initParam.sql != NULL)
	{
      if (strncmp(initParam.sql, "SELECT COUNT", 12) == 0)
      {
         //You can specify a completion code with any positive value in a PROCESS_STOP_.
         //Negative completion codes are reserved for HP use.
         //Therefore negative codes will return as 1000 + abs(completionCode)
         short completionCode = -1;
         completionCode = SQL_EXECDIRECT_FETCH(&initParam);

         if (completionCode < 0)
            completionCode = 1000 + abs(completionCode);

#ifdef NSK_PLATFORM
         PROCESS_STOP_(,,,completionCode,,,,);
#else
		 /*
		  * TODO:
		  * need to revisit this logic to return a value via exit code
		  *
		  */
#endif
      }
      else
      {
		  sprintf(tmpString, "Server dies, see previous event."
				  "New temp MXOSRVR is cleaning up obsolete volatile tables and exit.");
		  SendEventMsg(	MSG_ODBC_SERVICE_STOPPED_WITH_INFO,
								  EVENTLOG_INFORMATION_TYPE,
								  processId,
								  ODBCMX_SERVER,
								  srvrObjRef,
								  1,
								  tmpString);
		  SQL_EXECDIRECT(&initParam);
		  exit(0);
	  }
   }
//LCOV_EXCL_STOP

	TransportTrace = initParam.debugFlag & SRVR_TRANSPORT_TRACE;

	switch (initParam.transport)
	{
	case CEE_TRANSPORT_TCP:
		if (initParam.portNumber == 0)
		{
//LCOV_EXCL_START
			sprintf(tmpString, "Invalid Port Number: %ld", initParam.portNumber);
			SendEventMsg(	MSG_PROGRAMMING_ERROR,
										EVENTLOG_ERROR_TYPE,
										processId,
										ODBCMX_SERVER,
										srvrObjRef,
										1,
										tmpString);
			exit(0);
//LCOV_EXCL_STOP
		}
		transport = initParam.transport;
		portNumber = (CEECFG_TcpPortNumber)initParam.portNumber;
		TcpProcessName = initParam.TcpProcessName;
//LCOV_EXCL_START
		if (initParam.eventFlag >= EVENT_INFO_LEVEL2)
		{
			char tmpStringEnv[512];

			sprintf(tmpStringEnv
				   ,"ODBC Server Initial Parameters: "
				    "AS Object Ref(-A) %s, "
					"ASProcessName(-AS) %s, "
					"DebugFlag(-D) %d, "
					"DsId(-DS) %d, "
					"EventFlag(-E) %d, "
					"EmsName(-EMS) %s, "
					"IpPortNumber(-PN) %d, "
					"Transport(-T) %d, "
					"TcpProcessName(-TCP) %s, "
					"MajorVersion(-V) %d"
				   ,initParam.asSrvrObjRef
				   ,initParam.ASProcessName
				   ,initParam.debugFlag
				   ,initParam.DSId
				   ,initParam.eventFlag
				   ,initParam.EmsName
				   ,initParam.portNumber
				   ,initParam.transport
				   ,initParam.TcpProcessName
				   ,initParam.majorVersion
					);

					SendEventMsg(MSG_SRVR_ENV
										, EVENTLOG_INFORMATION_TYPE
										, processId
										, ODBCMX_SERVICE
										, srvrObjRef
										, 1
										, tmpStringEnv);


		}
		break;
	default:
		sts = CEE_BADTRANSPORT;
		sprintf(tmpString, "%ld", sts);
		SendEventMsg(	MSG_KRYPTON_ERROR,
									EVENTLOG_ERROR_TYPE,
									processId,
									ODBCMX_SERVER,
									srvrObjRef,
									1,
									tmpString);
		exit(0);
		break;
	}
//LCOV_EXCL_STOP

	if ((initParam.majorVersion == NT_VERSION_MAJOR_1) ||
		(initParam.majorVersion == NSK_VERSION_MAJOR_1)) // Major Version == 1
	{
		ImplInit (NULL,(const char *)&initParam, sizeof(initParam), &sts, NULL, NULL);

		if (sts != CEE_SUCCESS)
		{
//LCOV_EXCL_START
			sprintf(tmpString, "%ld", sts);
			SendEventMsg(MSG_KRYPTON_ERROR,
									EVENTLOG_ERROR_TYPE,
									processId,
									ODBCMX_SERVER,
									srvrObjRef,
									1,
									tmpString);
			exit(0);
//LCOV_EXCL_STOP
		}
	}

    srvrGlobal->clientKeepaliveStatus = keepaliveStatus;
    srvrGlobal->clientKeepaliveIntervaltime = keepaliveIntervaltime;
    srvrGlobal->clientKeepaliveIdletime = keepaliveIdletime;
    srvrGlobal->clientKeepaliveRetrycount = keepaliveRetrycount;

    // TCPADD and RZ are required parameters.
	// The address is passed in with TCPADD parameter .
	// The hostname is passed in with RZ parameter.
	if( strlen(trafIPAddr) > 0 && strlen(hostname)>0 )
	{
		strcpy( srvrGlobal->IpAddress, trafIPAddr );
		strcpy( srvrGlobal->HostName, hostname);
        sprintf( errStrBuf1, "Server: IPaddr = %s, hostname = %s",
                 srvrGlobal->IpAddress,srvrGlobal->HostName);
	}
	else
	{
		SendEventMsg(  MSG_SET_SRVR_CONTEXT_FAILED,EVENTLOG_ERROR_TYPE,
                           processId,ODBCMX_SERVER,srvrObjRef,
                           1,"Cannot get TCPADD or RZ information from startup parameters");                           
	    exit(0);
	}	                            
	SendEventMsg(MSG_SRVR_ENV, EVENTLOG_INFORMATION_TYPE,
								  srvrGlobal->nskProcessInfo.processId,
								  ODBCMX_SERVICE, srvrGlobal->srvrObjRef,
								  1, errStrBuf1);
	if( stopOnDisconnect )
		srvrGlobal->stopTypeFlag = STOP_WHEN_DISCONNECTED;
	found = false;
	while(!found)
	{
		srvrGlobal->portNumber = initParam.portNumber;
		rc = zoo_exists(zh, dcsRegisteredNode.c_str(), 0, &stat);
		if( rc == ZOK )
		{
			ss.str("");
			ss << myNid
			   << ":"
			   << myPid
			   << ":"
			   << myProcName.c_str()
			   << ":"
			   << srvrGlobal->IpAddress
			   << ":"
			   << initParam.portNumber;

			regSrvrData = ss.str();

			ss.str("");
			ss << "AVAILABLE"
			   << ":"
			   << JULIANTIMESTAMP()
			   << ":"					// Dialogue ID
			   << ":"
			   << regSrvrData
			   << ":"					// Client computer name
			   << ":"					// Client address
			   << ":"					// Client port
			   << ":"					// Client Appl name
			   << ":";

			string data(ss.str());

			rc = zoo_set(zh, dcsRegisteredNode.c_str(), data.c_str(), data.length(), -1);
			if( rc != ZOK )
			{
				zk_error = true;
				sprintf(zkErrStr, "***** zoo_set() failed for %s with error %d", dcsRegisteredNode.c_str(), rc);
				break;
			}
			found = true;
		}
		else	// error
		{
			zk_error = true;
			sprintf(zkErrStr, "***** zoo_exists() for %s failed with error %d",dcsRegisteredNode.c_str(), rc);
			break;
		}
	}

	if( zk_error )
		SendEventMsg(  MSG_SET_SRVR_CONTEXT_FAILED,
					   EVENTLOG_ERROR_TYPE,
					   processId,
					   ODBCMX_SERVER,
					   srvrObjRef,
					   1,
					   zkErrStr);

try
{
	if((sts = runCEE(initParam.TcpProcessName, initParam.portNumber, TransportTrace)) != CEE_SUCCESS )
	{
//LCOV_EXCL_START
		sprintf(tmpString, "%ld", sts);
		SendEventMsg(	MSG_KRYPTON_ERROR,
									EVENTLOG_ERROR_TYPE,
									processId,
									ODBCMX_SERVER,
									srvrObjRef,
									1,
									tmpString);
//LCOV_EXCL_STOP
	}
}
catch (std::bad_alloc)
{
		SendEventMsg(MSG_ODBC_NSK_ERROR, EVENTLOG_ERROR_TYPE,
			0, ODBCMX_SERVER, srvrGlobal->srvrObjRef,
			1, "No more memory is available to allocate, Exiting MXOSRVR server");
}

	if (srvrGlobal->traceLogger != NULL)
	{
		delete srvrGlobal->traceLogger;
		srvrGlobal->traceLogger = NULL;
	}
	if (resStatSession != NULL)
	{
		delete resStatSession;
		resStatSession = NULL;
	}
	if (resStatStatement != NULL)
	{
		delete resStatStatement;
		resStatStatement = NULL;
	}

	exit(0);
}

void logError( short Code, short Severity, short Operation );

void logError( short Code, short Severity, short Operation )
{
	return;
}

static const char* state2String(int state){
  if (state == 0)
    return "CLOSED_STATE";
  if (state == ZOO_CONNECTING_STATE)
    return "CONNECTING_STATE";
  if (state == ZOO_ASSOCIATING_STATE)
    return "ASSOCIATING_STATE";
  if (state == ZOO_CONNECTED_STATE)
    return "CONNECTED_STATE";
  if (state == ZOO_EXPIRED_SESSION_STATE)
    return "EXPIRED_SESSION_STATE";
  if (state == ZOO_AUTH_FAILED_STATE)
    return "AUTH_FAILED_STATE";

  return "INVALID_STATE";
}

void watcher(zhandle_t *zzh, int type, int state, const char *path, void *watcherCtx)
{
    /* Be careful using zh here rather than zzh - as this may be mt code
     * the client lib may call the watcher before zookeeper_init returns */
    if (path && strlen(path) > 0) {
    	fprintf(stderr, "Watcher %d state = %s for path %s\n", type, state2String(state), path);
    }
    else
    	fprintf(stderr, "Watcher %d state = %s\n", type, state2String(state));

    if(type == ZOO_SESSION_EVENT){
        if(state == ZOO_CONNECTED_STATE){
            pthread_mutex_lock(&lock);
            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&lock);
        }
    }

    if (type == ZOO_SESSION_EVENT) {
        if (state == ZOO_CONNECTED_STATE) {
            const clientid_t *id = zoo_client_id(zzh);
            if (myid.client_id == 0 || myid.client_id != id->client_id) {
                myid = *id;
                fprintf(stderr,"Got a new session id: 0x%llx\n", myid.client_id);
             }
        } else if (state == ZOO_AUTH_FAILED_STATE) {
            fprintf(stderr,"Authentication failure. Shutting down...\n");
            zookeeper_close(zzh);
            shutdownThisThing=1;
            zh=0;
        } else if (state == ZOO_EXPIRED_SESSION_STATE) {
            fprintf(stderr,"Session expired. Shutting down...\n");
            zookeeper_close(zzh);
            shutdownThisThing=1;
            zh=0;
        }
    }

    if (type == ZOO_CHANGED_EVENT) {
      string masterNode(zkRootNode);

      masterNode.append("/dcs/master");

      if (masterNode.compare(path) == 0) {
        if (getEpoch(zzh) != epoch) {
          shutdownThisThing=1;
        }
      }
    }
}

bool verifyPortAvailable(const char * idForPort,
                         int portNumber)
{
    int fdesc;
    int result;
	sockaddr_in6	portcheckaddr;
	char sTmp1[80];
	char sTmp2[80];

	if ((fdesc = socket(AF_INET6, SOCK_STREAM, 0)) < 0 )
		fdesc = socket(AF_INET, SOCK_STREAM, 0);

//LCOV_EXCL_START
    if (fdesc < 0) {
        return false;
    }
//LCOV_EXCL_STOP

    bzero((char*)&portcheckaddr, sizeof(portcheckaddr));
    portcheckaddr.sin6_family = AF_INET6;
    portcheckaddr.sin6_addr = in6addr_any;
    portcheckaddr.sin6_port = htons((uint16_t)portNumber);

    result = bind(fdesc, (struct sockaddr *)&portcheckaddr, sizeof(portcheckaddr));
//LCOV_EXCL_START
/*
    if (result < 0)
    {
		sprintf( sTmp1, "Cannot use %s port (%s).", idForPort,
                 strerror(errno));
		sprintf( sTmp2, "%d", portNumber );

		SendEventMsg(MSG_SET_SRVR_CONTEXT_FAILED,
                                    EVENTLOG_ERROR_TYPE,
                                    GetCurrentProcessId(),
                                    ODBCMX_SERVICE,
                                    tcpProcessName, 2,
                                    sTmp1, sTmp2);

        close(fdesc);
        return false;
    }
//LCOV_EXCL_STOP
    else
    {
        close(fdesc);
        return true;
    }
*/
    close(fdesc);
    return (result < 0) ? false : true;
}

BOOL getInitParamSrvr(int argc, char *argv[], SRVR_INIT_PARAM_Def &initParam, char* strName, char* strValue)
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
	initParam.tcp_address		= IP_ADDRESS;
	initParam.minSeg			= DEFAULT_MINIMUM_SEGMENTS;
	initParam.initSegTimeout	= DEFAULT_INIT_SEG_TIMEOUT;
	count = 1;

	stopOnDisconnect = 0;
	clientConnTimeOut = 60;	// Default to 60 secs
	zkSessionTimeout = 30;	// Default to 30 secs
	strName[0] = 0;
	strValue[0] = 0;
	maxHeapPctExit = 0;
	initSessMemSize = 0;
	initParam.timeLogger=false;
	initTcpProcessNameTemp(initParam.TcpProcessName);
	initParam.ASProcessName[0]	= '\0';
	strcpy( initParam.EmsName, DEFAULT_EMS );
	strcpy(initParam.QSProcessName, DEFAULT_QS_PROCESS_NAME);
	strcpy(initParam.QSsyncProcessName, DEFAULT_SYNC_PROCESS_NAME);
	memset(trafIPAddr,0,sizeof(trafIPAddr));
	memset(hostname,0,sizeof(hostname));
	aggrInterval = 60;
    statisticsCacheSize = 60;
	queryPubThreshold = 60;
	statisticsPubType = STATISTICS_AGGREGATED;
	bStatisticsEnabled = false;

	initParam.EmsTimeout		= DEFAULT_EMS_TIMEOUT;
	initParam.initIncSrvr		= DEFAULT_INIT_SRVR;
	initParam.initIncTime		= DEFAULT_INIT_TIME;
	initParam.DSG =DEFAULT_DSG;
	initParam.srvrTrace = false;
	initParam.TraceCollector[0]	= '\0';
	initParam.RSCollector[0]	= '\0';
	initParam.sql = NULL;
	initParam.mute = false;//Dashboard testing - no 21036 message
	initParam.ext_21036 = true; // new extended 21036 msg - for SRPQ
    initParam.floatIP = false;  // bool which indicates if the script which binds the floating ip address needs to be run

	memset(initParam.neoODBC,0,sizeof(initParam.neoODBC));

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

		if (strcmp(arg, "-D") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				if( getNumberTemp( argv[count], number ) == TRUE )
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
		if (strcmp(arg, "-RZ") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				if (strlen(argv[count]) < sizeof(regZnodeName)-1)
				{
					strcpy(regZnodeName, argv[count]);
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
		if (strcmp(arg, "-ZKHOST") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				if (strlen(argv[count]) < sizeof(zkHost)-1)
				{
					strcpy(zkHost, argv[count]);
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
		if (strcmp(arg, "-ZKPNODE") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				if (strlen(argv[count]) < sizeof(zkRootNode)-1)
				{
					strcpy(zkRootNode, argv[count]);
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
		if (strcmp(arg, "-CNGTO") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				if( getNumberTemp( argv[count], number ) == TRUE )
					clientConnTimeOut = number;
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
		if (strcmp(arg, "-ZKSTO") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				if( getNumberTemp( argv[count], number ) == TRUE )
					zkSessionTimeout = number;
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
		if (strcmp(arg, "-EADSCO") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				if( getNumberTemp( argv[count], number ) == TRUE )
					stopOnDisconnect = number;
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
		if (strcmp(arg, "-TCPADD") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				if (strlen(argv[count]) < sizeof(trafIPAddr)-1)
				{
					strcpy(trafIPAddr, argv[count]);
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
		if (strcmp(arg, "-MAXHEAPPCT") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				if( getNumberTemp( argv[count], number ) == TRUE )
				{
					if(number > 0)
						maxHeapPctExit = number;
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
		if (strcmp(arg, "-STATISTICSINTERVAL") == 0)
		{
			if (++count < argc )
			{			    
				//support positive & minus
				number=atoi(argv[count]);				
                if(number >= MIN_INTERVAL)
					aggrInterval = number;				
			}	
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
		else
		if (strcmp(arg, "-STATISTICSCACHESIZE") == 0)
		{
			if (++count < argc )
			{
                            if(strspn(argv[count], "0123456789")==strlen(argv[count])){
                                number=atoi(argv[count]);
                                if(number > 0)
                                    statisticsCacheSize = number;
                            }
                            else
                            {
                                argWrong = TRUE;
                            }
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
               else
		if (strcmp(arg, "-STATISTICSLIMIT") == 0)
		{
			if (++count < argc)
			{
				number=atoi(argv[count]);				
                if(!(number >0 && number<MIN_INTERVAL))
					queryPubThreshold = number;				
			}	
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
		else
		if (strcmp(arg, "-STATISTICSTYPE") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				char statisticsOpt[20];
				if (strlen(argv[count]) < sizeof(statisticsOpt)-1)
				{
					memset(statisticsOpt,0,sizeof(statisticsOpt));
					strcpy(statisticsOpt, argv[count]);
					if (stricmp(statisticsOpt, "session") == 0)
						statisticsPubType = STATISTICS_SESSION;						
					else
						statisticsPubType = STATISTICS_AGGREGATED;
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
		if (strcmp(arg, "-STATISTICSENABLE") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				char statisticsEnable[20];
				if (strlen(argv[count]) < sizeof(statisticsEnable)-1)
				{
					memset(statisticsEnable,0,sizeof(statisticsEnable));
					strcpy(statisticsEnable, argv[count]);
					if (stricmp(statisticsEnable, "false") == 0)
						bStatisticsEnabled = false;
					else
						bStatisticsEnabled = true;
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
		if (strcmp(arg, "-PORTMAPTOSECS") == 0)
		{
			if (++count < argc )
			{
				portMapToSecs = atoi(argv[count]);;				
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
		else
		if (strcmp(arg, "-PORTBINDTOSECS") == 0)
		{
			if (++count < argc )
			{
				portBindToSecs = atoi(argv[count]);				
			}
			else
			{
				argEmpty = TRUE;
				break;
			}
		}
		else
		if (strcmp(arg, "-SQLPLAN") == 0)
		{
			if (++count < argc && argv[count][0] != '-' )
			{
				char planEnable[20];
				if (strlen(argv[count]) < sizeof(planEnable)-1)
				{
					memset(planEnable,0,sizeof(planEnable));
					strcpy(planEnable, argv[count]);
					if (stricmp(planEnable, "false") == 0)
						bPlanEnabled = false;
					else
						bPlanEnabled = true;
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
		}else
        if (strcmp(arg, "-TCPKEEPALIVESTATUS") == 0){
            if (++count < argc && argv[count][0] != '-')
            {
                char keepaliveEnable[20];
                if (strlen(argv[count]) < sizeof(keepaliveEnable) - 1)
                {
                    memset(keepaliveEnable, 0, sizeof(keepaliveEnable) - 1);
                    strncpy(keepaliveEnable, argv[count], sizeof(keepaliveEnable) - 1);
                    if(stricmp(keepaliveEnable, "true") == 0)
                        keepaliveStatus = true;
                    else
                        keepaliveStatus = false;
                }
                else
                {
                    argWrong = TRUE;
                }
            }
            else
            {
                argEmpty = TRUE;
                break;
            }
        }else
        if (strcmp(arg, "-TCPKEEPALIVEIDLETIME") == 0){
            if (++count < argc )
            {
                if(strspn(argv[count], "0123456789")==strlen(argv[count])){
                    keepaliveIdletime = atoi(argv[count]);
                }else
                {
                    argWrong = TRUE;
                }
			}
            else
            {
                argEmpty = TRUE;
                break;
            }
        }else
        if (strcmp(arg, "-TCPKEEPALIVEINTERVAL") == 0){
            if (++count < argc )
            {
                if(strspn(argv[count], "0123456789")==strlen(argv[count])){
                    keepaliveIntervaltime = atoi(argv[count]);
                }else
                {
                    argWrong = TRUE;
                }
            }
            else
            {
                argEmpty = TRUE;
                break;
            }
        }else
        if (strcmp(arg, "-TCPKEEPALIVERETRYCOUNT") == 0){
            if (++count < argc )
            {
                if(strspn(argv[count], "0123456789")==strlen(argv[count])){
                    keepaliveRetrycount = atoi(argv[count]);
                }else
                {
                    argWrong = TRUE;
                }
            }
            else
            {
                argEmpty = TRUE;
                break;
            }
        }
		count++;
	}

}

// The "epoch" is a time period between configuration changes in the
// system. When such a configuration change happens (e.g. the
// executable of the mxosrvr is replaced, or a system default is being
// changed), we want to stop all existing mxosrvrs once they become
// idle and replace them with new ones. Therefore, keep a watch on
// this value and exit when it changes and when our state is or
// becomes idle.
long  getEpoch(zhandle_t *zh) {
  char path[2000];
  char zkData[1000];
  int zkDataLen = sizeof(zkData);
  int result = -1;

  snprintf(path, sizeof(path), "%s/dcs/master", zkRootNode);
  int rc = zoo_get(zh, path, 1, zkData, &zkDataLen, NULL);

  if (rc == ZOK && zkDataLen > 0)
    result = atol(zkData);

  return result;
}
