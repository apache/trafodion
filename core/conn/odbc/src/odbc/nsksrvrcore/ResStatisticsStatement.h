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
#include "srvrcommon.h"

#define MAX_LENGTH 				1000
#define MAX_RMS_COLS			32
#define MAX_PERTABLE_STATS_DESC	30
#define MAX_ROWBUF_SIZE			4000
#define MAX_ERROR_TEXT_LENGTH	200

// Temporary till available from sqlcli.h
#ifndef SQLSTATS_ROWS_RETURNED
#define SQLSTATS_ROWS_RETURNED	23
#endif

typedef short (* pEndAggWMS) (SRVR_STMT_HDL *pSrvrStmt);
typedef short (* pUpdAggWMS) (SRVR_STMT_HDL *pSrvrStmt);
 
/*
typedef struct _SELEMENT {
	char* keyword;
	int64* dest;
	bool found;
	short index;
} SELEMENT;
*/

// Structure to contain QS supported RTS columns
typedef struct _NDCS_SuppRTSCols {
	char colName[30];
	char colValue[MAX_LENGTH];
	bool exists;
} NDCS_SuppRTSCols;

// Structure to contain all RTS columns returned by the stored procedure
typedef struct _RTS_Columns {
	char colName[30];
	short index;
	_RTS_Columns *next;
} RTS_Col;

typedef struct _PerTableStats {
	short  statsItem_id;
	char	tblName[(128*3)+1];
	double	estAccessedRows;
	double	estUsedRows;
	int64	accessedRows;
	int64	usedRows;
	int64	diskIOs;
	int64	numMessages;
	int64	messagesBytes;
	int64	statsBytes;
	int64	escalations;
	int64	lockWaits;
	int64	dp2BusyTime;
	int64	opens;
	int64	openTime;
} PerTableStats;

// the following struct is a kludge to get us to the length of the sqlWarningOrError,
// the actual text follows 'Int32 length'
typedef struct _Error_Desc_def {
	Int32 rowId;
	Int32 errorCode;
	Int32 sqlCode;
	Int32 length;
} err_desc_def;


class ResStatisticsStatement:public ResStatistics 
{
private:
	friend class ResStatisticsSession;
         //   statement variables
          
         char                                    *sqlStatement;
	 // row 1 of RMS stats 
	 char	queryId[MAX_QUERY_NAME_LEN+1];
	 char	parentQID[MAX_QUERY_ID_LEN+1];     // start of new items
	 char	childQID[MAX_QUERY_ID_LEN+1];
	 int64   compStartTime;
	 int64   compEndTime;
	 int64   compTime;
	 int64   exeStartTime;
	 int64   exeEndTime;
	 int64   exeTime;
	 int64   rowsAffected;
	 short       sqlErrorCode;
	 short       statsErrorCode;
	 short       state;
	 short       statsType;
	 short       queryType;
	 double      estRowsAccessed;
	 double      estRowsUsed;
	 int64   exeElapsedTime;
	 Int32        numSqlProcs;
	 Int32        numCpus;
	 char        sqlSrc[RMS_STORE_SQL_SOURCE_LEN+1];
	 Int32        sqlSrcLen;
	 Int32        exePriority;
	 char        transID[MAX_TXN_STR_LEN+1];
	 int64	 transIDnum;
	 int64   rowsReturned;
	 short       AQRlastError;
	 short       AQRnumRetries;
	 short       AQRdelayBeforeRetry;
	 int64   firstRowReturnTime;               // end of new item
	 int64		 NumRowsIUD;
	 int64		ScratchFileCount;
	 int64		ScratchBufferBlockSize;
	 int64		ScratchBufferBlocksRead;
	 int64		ScratchBufferBlocksWritten;
	 short		ScratchOverflowMode;
	 int64		ScratchBufferReadCount;
	 int64		ScratchBufferWriteCount;
	 int32		ScratchIOSize;
	 int32		ScratchIOMaxTime;
	 int32		bmoSpaceBufferSize;
	 int64		bmoSpaceBufferCount;
	 int64		bmoInterimRowCount;
	 int32		topN;
	 char        subQryType[SUB_QRY_TYPE_LEN+1];
	 char        parentSysName[PAR_SYS_NAME_LEN+1];

	 // row 1 or RMS stats - end

	 // row 2 of RMS stats 
	 int64   NumMessages;			//YES
	 int64   MessagesBytes;			//YES
	 int64   AccessedRows;			//YES
	 int64   UsedRows;				//YES
	 int64   DiskIOs;				//YES
	 int64   Escalations;			//YES
	 int64   LockWaits;				//YES
	 int64   Opens;				//YES
	 int64   OpenTime;				//YES
	 int64   StatsBytes;			//YES
	 int64   ProcessBusyTime;			//YES
	 int64   NewProcess;			//YES
	 int64   NewProcessTime;			//YES
	 Int64        SpaceTotal;			//YES
	 Int64        SpaceUsed;				//YES
	 Int64        HeapTotal;				//YES
	 Int64        HeapUsed;				//YES
         Int64        HeapWM;
	 int64   CpuTime;
	 Int64        Dp2SpaceTotal;			//YES
	 Int64        Dp2SpaceUsed;			//YES
	 Int64        Dp2HeapTotal;			//YES
	 Int64        Dp2HeapUsed;			//YES

	 // Currently not in R2.5. To be enabled post R2.5/SQ
	 int64		 UdrCpuTime;			//YES

	 // the following 5 are new RMS stats 
	 int64   DiskProcessBusyTime;
	 Int64        reqMsgCnt;                         
	 int64   reqMsgBytes;
	 Int64        replyMsgCnt;
	 int64   replyMsgBytes;
	 // row 2 of RMS stats - end

	 // Field to hold Total Memory Allocated
	 UInt32	TotalMemAlloc;

	 // Field to hold Max Memory Used
     UInt32	MaxMemUsed;

	 // Field to hold Estimated Total Memory ( compiler estimated total memory * dop )
	 double			estTotalMem;

         double                                  estimatedCost;
         int64                                  odbcElapseTime;
         int64                                  odbcExecutionTime;
		 int64	 prepareTime;
         char                                    statementId[MAX_STMT_LABEL_LEN+1];
         short                                   stmtType;
		 int                                     flag;
		 int64                                   numberOfRows;
		 Int32                                    errorCode;


		int64 StatsRowType;
		int64 Timeouts;
		int64 NumSorts;
		int64 SortElapsedTime;

	 static      NDCS_SuppRTSCols rowOne[];
	 static      NDCS_SuppRTSCols rowTwo[];

	 RTS_Col     *rtsExeCols;
	 RTS_Col     *rtsDp2Cols;
/*
	 SELEMENT    stats0[MAX_RMS_COLS];
	 SELEMENT    stats1[MAX_RMS_COLS];
	 short       stats0_index[MAX_RMS_COLS];
	 short       stats1_index[MAX_RMS_COLS];		
*/
 // collecting time
        int64				statementStartTime;
        int64				statementEndTime;
		int64				queryStartTime;
        int64				statementStartCpuTime;
        int64				statementEndCpuTime;
		int64				queryStartCpuTime;
		int64				prepareStartTime;
		int64				prepareEndTime;
	int64				totalStatementOdbcElapseTime;
	int64				totalStatementOdbcExecutionTime;
	int64				queryElapseTime;
	int64				queryExecutionTime;
        struct  passSession                     ps;
	ResStatisticsSession                    *tempStatSession;
    
//  To fetch results

        SRVR_STMT_HDL                           *resSrvrStmt;
        SQLValueList_def                        outputValueList;
        SQLValue_def                            *SQLValue;
		char                                    tmpString[4000];
	    char                                    sqlString[256];
        //int64                                   rowsAffected; 
        Int32                                    maxRowCnt;
        Int32                                    maxRowLen;
        Int32                                    retcode;
        bool                                    statStatisticsFlag;
		Int32                                    totalStatementExecutes;
        char		                            msgBuffer[BUFFERSIZE+70];
		char									typeOfStatement[10];
		bool									useCLI_;
//
// Connection, Compilation, Execution rules
//
		char									con_rule_name[MAX_RULE_NAME_LEN + 1];
		char									cmp_rule_name[MAX_RULE_NAME_LEN + 1];
		char									exe_rule_name[MAX_RULE_NAME_LEN + 1];
public:
		BOOL                                    catFlagOn;
		BOOL									tmpFlag;
		char									stmtLabel[MAX_STMT_LABEL_LEN+1];
		PerTableStats							perTableStats[MAX_PERTABLE_STATS_DESC];
		short									perTableRowSize;

	bool	pubStarted;
	bool	queryFinished;
	int64	wouldLikeToStart_ts;

public:

	int64 getNumMessages() { return NumMessages;}
	int64 getMessagesBytes() {return MessagesBytes;}
	int64 getAccessedRows(){return AccessedRows;}
	int64 getUsedRows(){ return UsedRows;}
	int64 getDiskIOs(){return DiskIOs; }
	int64 getLockWaits(){ return LockWaits;}
	int64 getEscalations(){return Escalations;}
	int64 getOpens(){return Opens;}
	int64 getOpenTime(){return OpenTime;}
	int64 getStatsBytes(){return StatsBytes; }
	int64 getDiskProcessBusyTime() { return DiskProcessBusyTime;}
	int64 getNewProcess(){ return NewProcess; }
	int64 getNewProcessTime() { return NewProcessTime;  }
	Int64 getSpaceTotal(){return SpaceTotal;}
	Int64 getSpaceUsed(){ return SpaceUsed;  }
	Int64 getHeapTotal(){ return HeapTotal;  }
	Int64 getHeapUsed() {return HeapUsed;    }
	int64 getProcessBusyTime(){return  ProcessBusyTime;  }
	Int64 getDp2SpaceTotal(){return Dp2SpaceTotal;}
	Int64 getDp2SpaceUsed(){return Dp2SpaceUsed;}
	Int64 getDp2HeapTotal(){return Dp2HeapTotal;}
	Int64 getDp2HeapUsed(){ return Dp2HeapUsed;}
	double getEstRowsAccessed(){return estRowsAccessed;}
	double getEstRowsUsed(){return estRowsUsed;}

	int64 getCompStartTime(){ return compStartTime;}
	int64 getCompEndTime(){return compEndTime;}
	int64 getExeStartTime(){return exeStartTime;}
	int64 getExeEndTime(){ return exeEndTime;}
	short getState(){return state;}
	short getStatsErrorCode(){return statsErrorCode;}
	short  getSqlErrorCode_(){return sqlErrorCode;}
	int64 getNumRowsIUD(){return NumRowsIUD;}
	int64  getExeElapsedTime(){return exeElapsedTime;}

	char *getParentQID(){return parentQID;}
	Int32 getNumSqlProcs(){return numSqlProcs ;}
	Int32 getNumCpus(){return numCpus;}
	char *getSqlSrc(){return sqlSrc;}
	Int32 getSqlSrcLen(){return sqlSrcLen;}
	Int32 getExePriority(){return exePriority;}
	char *getTransID(){return transID;}
	int64 getRowsReturned(){return rowsReturned;}
	int64 getFirstRowReturnTime(){return firstRowReturnTime;}
	Int64 getReqMsgCnt(){return  reqMsgCnt;}
	int64 getReqMsgBytes(){return reqMsgBytes;}
	Int64 getReplyMsgCnt(){return replyMsgCnt;}
	int64 getReplyMsgBytes(){return replyMsgBytes;}

	short getAQRlastError(){return AQRlastError;}
	short getAQRnumRetries(){return AQRnumRetries;}
	short getAQRdelayBeforeRetry(){return AQRdelayBeforeRetry;}
	int64 getUdrCpuTime(){return UdrCpuTime;}

	short getScratchOverflowMode(){return  ScratchOverflowMode;}
	int64 getScratchFileCount(){return ScratchFileCount;}
	int64 getScratchBufferBlockSize(){return ScratchBufferBlockSize;}
	int64 getScratchBufferBlocksRead(){return ScratchBufferBlocksRead;}
	int64  getScratchBufferBlocksWritten(){return ScratchBufferBlocksWritten;}
	int64 getScratchBufferReadCount(){ return ScratchBufferReadCount;}
	int64 getScratchBufferWriteCount(){return ScratchBufferWriteCount;}
	char *getSubQryType(){return subQryType;}
	char *getParSysName(){return parentSysName;}

	UInt32 getTotalMemAlloc(){return TotalMemAlloc;}
	UInt32 getMaxMemUsed(){return MaxMemUsed;}
        /*void start( Int32 inState,
		Int32 inSqlQueryType,
		const char *inStmtName,
		char *inQueryID,
			SQL_QUERY_COST_INFO cost_info,
			SQL_QUERY_COMPILER_STATS_INFO comp_stats_info,
		double inEstimatedCost,
		bool *flag_21036,
			bool logRepository = false,
			//
			Int32 *returnCode,
			Int32 *sqlWarningOrErrorLength,
			BYTE * &sqlWarningOrError,
			//
			char *inSqlStatement=NULL
			); */
        void start( Int32 inState,
			Int32 inSqlQueryType,
			const char *inStmtName,
			SRVR_STMT_HDL *pSrvrStmt,
			double inEstimatedCost,
			bool *flag_21036,
			/*
			Int32 *returnCode,
			Int32 *sqlWarningOrErrorLength,
			BYTE * &sqlWarningOrError,
			*/
			char *inSqlStatement=NULL
			);
        void end(Int32 inState,
		Int32 inSqlQueryType,
		short inStmtType,
		char *inQueryId,
		double inEstimatedCost,
		char *inSqlStatement,
		Int32 inErrorStatement,
		Int32 inWarningStatement,
		int64 inRowCount,
		Int32 inErrorCode, 
		ResStatisticsSession *resStatSession, 
			Int32 inSqlErrorLength,
			char *inSqlError,
			SRVR_STMT_HDL *pSrvrStmt,
		bool *flag_21036,
			Int32 inSqlNewQueryType,
			char isClosed=TRUE
			); 
	/*
	void endRepository(Int32 inState, 
			Int32 inSqlEuryType, 
			char *inSqlStatement,
			bool isClosed,
			SQL_QUERY_COST_INFO cost_info,
			SQL_QUERY_COMPILER_STATS_INFO comp_stats_info,
			bool *flag_21036,
			Int32 sqlWarningOrErrorLength,
			BYTE *sqlWarningOrError);
			*/
	void endRepository(SRVR_STMT_HDL *pSrvrStmt, 
			Int32 sqlWarningOrErrorLength,
			BYTE *sqlWarningOrError,
			bool bClose_Fetch);
	/*void toRepository(Int32 inState, 
			Int32 inSqlEuryType, 
			char *inSqlStatement,
			SQL_QUERY_COST_INFO cost_info,
			SQL_QUERY_COMPILER_STATS_INFO comp_stats_info,
			bool *flag_21036,
			Int32 sqlWarningOrErrorLength,
			BYTE *sqlWarningOrError);*/
	void toRepository(SRVR_STMT_HDL *pSrvrStmt, 
			Int32 sqlWarningOrErrorLength,
			BYTE *sqlWarningOrError);
	void setStatistics(SRVR_STMT_HDL *pSrvrStmt = NULL, SQLSTATS_TYPE statsType = SQLCLI_ACCUMULATED_STATS, char *qID = NULL, short qIdLen = 0 ,int activeQueryNum = 0);//20111208
	void setStatisticsFlag(bool setStatisticsFlag);
        void prepareQuery(struct collect_info *setinit);
		char* getStatementType(Int32 inStmtType);


		ResStatisticsStatement(bool useCLI=true);
		ResStatisticsStatement(ResStatisticsSession *resStatSession, bool useCLI=true);
		~ResStatisticsStatement();
//
// AGGREGATION----------------procedures defines
//

	const char* mapEmptyToNA(char* input);

	short getSqlErrorCode() { return sqlErrorCode; }
	void setSqlErrorCode(int32 insqlErrorCode){ sqlErrorCode = (short)insqlErrorCode; }
	
	void init_rms_counters(bool resetAll=false);

	void SendQueryStats(bool bStart, SRVR_STMT_HDL *pSrvrStmt, char *inSqlError = NULL, Int32 inSqlErrorLength = 0);

protected:
		char *printLongString(char *buffer,char *longStr, Int32 inState,Int32 inSqlQueryType, SRVR_STMT_HDL *pSrvrStmt);
		void appendErrorText(char *inSqlError, Int32 inSqlErrorLength);
		string getErrorText(char *inSqlError, size_t inSqlErrorLength, size_t inMaxSqlErrorLength);
		void init();
		int64 getCpuTime();
        void identifyRTSRowFormat( short rowNum );
		void splitString(short rowNo);
public:
		short currentPriority();
   };

std::string getSQLStateStringRes(UInt32 state);
std::string getQueryStateStringRes(unsigned short state);
std::string getQuerySubStateStringRes(unsigned short state);
std::string get_WarnLevelStringRes(unsigned short value);

#endif
   
