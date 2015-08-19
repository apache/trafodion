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

/* MODULE: ResStatistics.h
   PURPOSE: Defines ResStatistics class
*/

#ifndef RESSTATISTICS_DEFINED
#define RESSTATISTICS_DEFINED

#include <windows.h>
//#include <winbase.h>
#include <sql.h>
#include <sqlExt.h>
#include "global.h"
#include "cee.h"
#include "odbcCommon.h"
#include "odbcsrvrcommon.h"
#include "odbc_sv.h"
#include "DrvrSrvr.h"
#include "srvrCommon.h"
#include "tdm_odbcSrvrMsg.h"
#include "CommonDiags.h"
#include "odbcMxSecurity.h"
#include "CSrvrStmt.h"
#include "EventMsgs.h"
#include "odbceventMsgUtil.h"
#include "srvrCommon.h"

#define STATE_LEN 15
#define MSGATTR_LEN 35
#define MSGINFO_LEN 500
#define RS_COLLECTOR "$ZACCT"
#define USERNAME_LENGTH 20
#define APPLICATIONID_LENGTH 130
#define COLLECTOR_TYPE "Resource Statistics"
#define BUFFERSIZE 3500

struct collect_info
{
    char					        clientId[MAX_COMPUTERNAME_LENGTH + 1];
    char				            userName[USERNAME_LENGTH + 1];
    char					        applicationId[APPLICATIONID_LENGTH + 1];
    char						    nodeName[10];
    char							cpuPin[20];
	char							DSName[MAX_DSOURCE_NAME + 1];
    long							userId;
    short							startPriority;
};              


class ResStatistics {
protected:

       
	struct collect_info                         resCollectinfo;
   // int											resAcctCollectorError;
    char		                                msgAttribute[MSGATTR_LEN];
    char			                            msgInfo[BUFFERSIZE+50];
    int					                        sequenceNumber;
	char                                        sequenceNumberStr[20];

// class for writing to the collector                               
    ODBCMXEventMsg			                    *resAcctLogger;
    char		                                ems_name[ EXT_FILENAME_LEN ];
	char                                        collectorName[EXT_FILENAME_LEN];
	BOOL										resStatCollectorError;		
	char			                            sessionId[35];

                                   
protected:
	    
    void openCollector(char *collector);
    void sendCollector();
    void closeCollector();
       
public:
	ResStatistics();
	~ResStatistics();
	
	  
};

#endif
