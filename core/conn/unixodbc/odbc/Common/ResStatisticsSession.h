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

/* MODULE: ResStatisticsSession.h
   PURPOSE: Defines ResStatisticsSession class
*/

 
#ifndef RESSTATSESSION_DEFINED
#define RESSTATSESSION_DEFINED

#include "ResStatistics.h"
#include "srvrCommon.h"

#define STATE_LEN 15


 struct passSession
        {
         time_t                                  sqlExecutionTime;
         time_t                                  odbcElapseTime;
         time_t                                  sqlElapseTime;
         time_t                                  odbcExecutionTime;
         char                                    state[STATE_LEN];
         char                                    statementId[MAX_STMT_LABEL_LEN + 1];
         short                                   stmtType;
         long                                    errorStatement;
		 long                                    warningStatement;
        };
 

 class ResStatisticsSession:public ResStatistics 
 {
 
private:

 //  Session variables
        char                                    startTime[25];
        char                                    endTime[25];
		struct tm                               * startTimeInfo;
		struct tm                               * endTimeInfo;
        time_t                                  totalSqlExecutionTime;
        time_t                                  totalOdbcElapseTime;
        time_t                                  totalSqlElapseTime;
        time_t                                  totalOdbcExecutionTime;
        long                                    totalInsertStatements;
        long                                    totalDeleteStatements;
        long                                    totalUpdateStatements;
        long                                    totalSelectStatements;
        long                                    totalErrors;
        long                                    totalWarnings;
        long                                    totalPrepares;
        long                                    totalExecutes;
        long                                    totalFetches;
        long                                    totalCloses;
		long                                    totalExecDirects;
	
public:
	    BOOL                                    logonOn;
	    BOOL                                    summaryOn;
        BOOL                                    resSessionOn;
		BOOL                                    logonFlag;
        int										totalCatalogStatements ;
		long									totalCatalogErrors ;
		long									totalCatalogWarnings ;
                                       
public:
        void start(struct collect_info  *setInit);
        void end();
        void accumulateStatistics(passSession *ps);
        void init();
		ResStatisticsSession();
		~ResStatisticsSession();
	
   };

#endif
