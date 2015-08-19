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

/* MODULE: ResStatistics.cpp
   PURPOSE: Implements the member functions of ResStatistics class
*/

 
#include <windows.h>
#include "ResStatistics.h"
#include <fs\feerrors.h>
#include "cextdecs.h"
 
using namespace SRVR;

ResStatistics::ResStatistics()
{
	memset(msgAttribute,'\0',MSGATTR_LEN);
	memset(msgInfo,'\0',BUFFERSIZE);
	memset(resCollectinfo.clientId,'\0',MAX_COMPUTERNAME_LENGTH + 1);
	memset(resCollectinfo.cpuPin,'\0',20);
	memset(resCollectinfo.nodeName,'\0',10);
	memset(resCollectinfo.DSName,'\0',MAX_DSOURCE_NAME + 1);
	memset(resCollectinfo.userName,'\0',USERNAME_LENGTH + 1);
	memset(resCollectinfo.applicationId,'\0',APPLICATIONID_LENGTH + 1);
	ODBCMXEventMsg *resAcctLogger = NULL;
	resCollectinfo.startPriority = 0;
	sequenceNumber = 0;
	memset(sequenceNumberStr,'\0',20);
	memset( ems_name,'\0',EXT_FILENAME_LEN);
	resStatCollectorError = FALSE;
	strcpy(collectorName,srvrGlobal->RSCollector);
}

ResStatistics::~ResStatistics()
{
	
}

// Open the collector	
void ResStatistics::openCollector(char *collector)
{
    short error = 0;
	char errorstr[10];
	short retems;
	short typeinfo[5];
	

	memset(errorstr, NULL, 10);
	sprintf(errorstr,"%d",error);

	if (collector != NULL)
	{
		strncpy(ems_name, collector, sizeof(ems_name));
		ems_name[sizeof(ems_name)-1]=0;
	}
	else
	{
		if (srvrEventLogger != NULL && resStatCollectorError == false) 
		{
				srvrEventLogger->SendEventMsg(	MSG_SERVER_COLLECTOR_ERROR, 
												EVENTLOG_WARNING_TYPE,
												srvrGlobal->nskProcessInfo.processId,
												ODBCMX_SERVER,
												srvrGlobal->srvrObjRef,
												5,
												srvrGlobal->sessionId,
												errorstr,
												"Invalid Collector NULL",
												COLLECTOR_TYPE,
												collector);
											
			resStatCollectorError = TRUE;
		}
		return;		
	}
	

	markNewOperator,resAcctLogger = new ODBCMXEventMsg();
	if (resAcctLogger == NULL) 
	{
			srvrEventLogger->SendEventMsg(	MSG_SERVER_COLLECTOR_ERROR, 
											EVENTLOG_WARNING_TYPE,
											srvrGlobal->nskProcessInfo.processId,
											ODBCMX_SERVER,
											srvrGlobal->srvrObjRef,
											5,
											srvrGlobal->sessionId,
											"resAcctLogger",
											"Out of Memory Error",
											COLLECTOR_TYPE,
											collector);
											
		resStatCollectorError = TRUE;
		return;
	}
	
	// check if collector name is a valid collector name
	retems = FILE_GETINFOBYNAME_(collector,
				     (short)strlen(collector),
				     typeinfo);
	// Device Type must be 1 (Operator console ) and
	// Subtype must be 0 ($0 operator process or alternate collector)
	if(retems != 0 || !(typeinfo[0] == 1 && typeinfo[1] == 0))
	{
		if (srvrEventLogger != NULL && resStatCollectorError == false) 
		{
				srvrEventLogger->SendEventMsg(	MSG_SERVER_COLLECTOR_ERROR, 
												EVENTLOG_WARNING_TYPE,
												srvrGlobal->nskProcessInfo.processId,
												ODBCMX_SERVER,
												srvrGlobal->srvrObjRef,
												5,
												srvrGlobal->sessionId,
												errorstr,
												"Invalid Collector or Collector Not Running",
												COLLECTOR_TYPE,
												collector);
											
			resStatCollectorError = TRUE;
			
		}
        delete resAcctLogger;
		resAcctLogger = NULL;
		return;		
	}

	
	// Open collector for Resource Statistics
	if (FILE_OPEN_(ems_name, strlen(ems_name) ,&resAcctLogger->ems_fnum) != FEOK)
	{
		FILE_GETINFO_(resAcctLogger->ems_fnum, &error);

		if (srvrEventLogger != NULL && resStatCollectorError == FALSE )
		{
	       srvrEventLogger->SendEventMsg(	MSG_SERVER_COLLECTOR_ERROR, 
											EVENTLOG_WARNING_TYPE,
											srvrGlobal->nskProcessInfo.processId,
											ODBCMX_SERVER,
											srvrGlobal->srvrObjRef,
											5,
											srvrGlobal->sessionId,
											errorstr,
											"ODBC/MX server failed to write to statistics collector",
											COLLECTOR_TYPE,
											collector);
			
		   resStatCollectorError = TRUE;
		   
		}  
		delete resAcctLogger;
		resAcctLogger = NULL;
	}
	else
	{
		resStatCollectorError = FALSE;
	}

}  

// Write information to the collector
void ResStatistics::sendCollector()
{

	if (resAcctLogger != NULL)
	{
		resAcctLogger->SendEventMsg(	MSG_RES_STAT_INFO, 
										EVENTLOG_INFORMATION_TYPE,
										srvrGlobal->nskProcessInfo.processId,
										ODBCMX_SERVER,
										srvrGlobal->srvrObjRef,
										4,
										srvrGlobal->sessionId,
										msgAttribute,
										sequenceNumberStr,
										msgInfo);
	}
}

// Close the collector 
void ResStatistics::closeCollector()
{ 
	if (resAcctLogger != NULL)
    {
	resAcctLogger->close_ems();
	delete resAcctLogger;
	resAcctLogger = NULL;
	}
	                                                                         
} 


