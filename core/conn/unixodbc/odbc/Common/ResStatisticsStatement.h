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

/* MODULE: ResStatisticsStatement.h
   PURPOSE: Defines ResStatisticsStatement class
*/


#ifndef RESSTATSTATEMENT_DEFINED
#define RESSTATSTATEMENT_DEFINED

  
#include "ResStatisticsSession.h"
#include "srvrCommon.h"

#define STATE_LEN 15
#define MAX_LENGTH 1000


class ResStatisticsStatement:public ResStatistics 
{
private:

         //   statement variables
          
         char                                    *sqlStatement;
         long                                    estimatedCost;
         char                                    rowsAccessed[MAX_LENGTH];
         char                                    rowsUsed[MAX_LENGTH];
         char                                    discReads[MAX_LENGTH];
         char                                    msgsToDisc[MAX_LENGTH];
         char                                    msgByteToDisc[MAX_LENGTH];
         char                                    lockWaits[MAX_LENGTH];
         char                                    lockEscalation[MAX_LENGTH];
		 char                                    strmsgsToDisc[30];
		 char                                    strmsgByteToDisc[30];
		 char                                    strrowsAccessed[30];
		 char                                    strrowsUsed[30];
		 char                                    strdiscReads[30];
		 char                                    strlockWaits[30];
		 char                                    strlockEscalation[30];
         time_t                                  odbcElapseTime;
         time_t                                  odbcExecutionTime;
         char                                    state[STATE_LEN];
         char                                    statementId[30];
         short                                   stmtType;
		 int                                     flag;
		 long                                    numberOfRows;
		 long                                    errorCode;
		 long                                    longmsgsToDisc;
		 long                                    longmsgByteToDisc;
		 long                                    longrowsAccessed;
		 long                                    longrowsUsed;
		 long                                    longdiscReads;
		 long                                    longlockWaits;
		 long                                    longlockEscalation;                                    

 // colecting time
        long long                               statementStartTime;
        long long                               statementEndTime;
        long									statementStartCpuTime;
        long                                    statementEndCpuTime;
		long                                    totalStatementOdbcElapseTime;
		long                                    totalStatementOdbcExecutionTime;
        struct  passSession                     ps;
		ResStatisticsSession                    *tempStatSession;
    
//  To fetch results

        SRVR_STMT_HDL                           *resSrvrStmt;
        SQLValueList_def                        outputValueList;
        SQLValue_def                            *SQLValue;
		char                                    tmpString[4000];
	    char                                    sqlString[256];
        long                                    rowsAffected; 
        long                                    maxRowCnt;
        long                                    maxRowLen;
        long                                    retcode;
        bool                                    statStatisticsFlag;
		long                                    totalStatementExecutes;
        char		                            msgBuffer[BUFFERSIZE+10];
		char									typeOfStatement[10];
		BOOL                                    prepareFlag;
public:
		BOOL                                    resStatementOn;
		BOOL                                    sqlOn;
		BOOL                                    prepareOn;
		BOOL                                    executeOn;
		BOOL                                    execdirectOn;
		BOOL                                    fetchOn; 
		BOOL                                    catFlagOn;
		BOOL									tmpFlag;
		char									stmtLabel[MAX_STMT_LABEL_LEN+1];

public:
        void start(char *inState);
        void end(char *inState,short inStmtType,long inEstimatedCost,char *inSqlStatement,long inErrorStatement,long inWarningStatement,long inRowCount,long inErrorCode);
      	void setStatistics();
		void setStatisticsFlag(bool setStatisticsFlag);
        void prepareQuery();
		void setStatementId();
		void setMaxRowCnt(long maxRowCnt);
		void setMaxRowLen(long maxRowLen);
		ResStatisticsStatement();
		ResStatisticsStatement(ResStatisticsSession *resStatSession);
		~ResStatisticsStatement();
		
protected:
	    void sendPrepare();
		void sendExecute();
		char *printLongString(char *buffer,char *longStr,int sequenceNumber);
		void init();
		long long getCpuTime();
		void splitString();

		
   };
#endif
   
