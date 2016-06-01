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
#include <sstream>
#include <string>
using namespace std;
//#include <winbase.h>

/* +++ T2_REPO
#include <platform_ndcs.h>
#include <errno.h>
#include <sql.h>
#include <sqlext.h>
#include "Global.h"
#include "cee.h"
#include "odbcCommon.h"
#include "odbcsrvrcommon.h"
#include "odbc_sv.h"
#include "DrvrSrvr.h"
#include "srvrcommon.h"
#include "tdm_odbcSrvrMsg.h"
#include "CommonDiags.h"
#include "odbcMxSecurity.h"
#include "CSrvrStmt.h"
#include "odbceventmsgutil.h"
#include "srvrcommon.h"
*/

#include "PubInterface.h"

#define STATE_LEN 15
#define MSGATTR_LEN 35
#define MSGINFO_LEN 500
#define RS_COLLECTOR "$ZACCT"
#define USERNAME_LENGTH 128
#define APPLICATIONID_LENGTH 128
#define COLLECTOR_TYPE "Resource Statistics"
#define BUFFERSIZE 16000

class ResStatistics {
protected:


    struct collect_info     resCollectinfo;
    char                    msgAttribute[MSGATTR_LEN];
    char                    msgInfo[BUFFERSIZE+50];
    int                     sequenceNumber;
    int                     totalmsgNumber;
    char                    sequenceNumberStr[20];

// class for writing to the collector
    char                    sessionId[35];

protected:

public:
    ResStatistics();
    ~ResStatistics();

};

#endif
