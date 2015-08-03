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

//#include <platform_ndcs.h>
#include "ResStatistics.h"

// +++ T2_REPO
//using namespace SRVR;
//extern BOOL   resStatCollectorError;

ResStatistics::ResStatistics()
{
    memset(msgAttribute,'\0',MSGATTR_LEN);
    memset(msgInfo,'\0',BUFFERSIZE+50);
    memset(resCollectinfo.clientId,'\0',MAX_COMPUTER_NAME_LEN + 1);
    memset(resCollectinfo.cpuPin,'\0',20);
    memset(resCollectinfo.nodeName,'\0',10);
    // +++ T2_REPO - ToDo - Remove DSName if not needed
    memset(resCollectinfo.DSName,'\0',MAX_DSOURCE_NAME + 1);
    memset(resCollectinfo.userName,'\0',USERNAME_LENGTH + 1);
    memset(resCollectinfo.applicationId,'\0',APPLICATIONID_LENGTH*4 + 1);
    memset(resCollectinfo.clientUserName,'\0', MAX_SQL_IDENTIFIER_LEN + 1);
    resCollectinfo.startPriority = 0;
    resCollectinfo.currentPriority = 0;
    resCollectinfo.totalLoginTime = 0;
    resCollectinfo.ldapLoginTime = 0;
    sequenceNumber = 0;
    totalmsgNumber = 0;
    memset(sequenceNumberStr,'\0',20);
}

ResStatistics::~ResStatistics()
{
}
