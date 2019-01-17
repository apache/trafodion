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
**********************************************************************/

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ExExeUtilGetStats.cpp
 * Description:  
 *               
 *               
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ComCextdecs.h"
#include  "cli_stdh.h"
#include  "ex_stdh.h"
#include  "sql_id.h"
#include  "ex_transaction.h"
#include  "ComTdb.h"
#include  "ex_tcb.h"
#include  "ComSqlId.h"
#include  "ExExeUtil.h"
#include  "ex_exe_stmt_globals.h"
#include  "exp_expr.h"
#include  "exp_clause_derived.h"
#include  "ComRtUtils.h"
#include  "ExStats.h"
#include  "ComSizeDefs.h"

///////////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilGetStatisticsTdb::build(ex_globals * glob)
{
  ex_tcb * exe_util_tcb = NULL;

  switch (getStatsReqType())
  {
  case SQLCLI_STATS_REQ_STMT:
    if (getStmtName() == NULL)
       exe_util_tcb = new(glob->getSpace()) ExExeUtilGetStatisticsTcb(*this, glob);
    else
    {
       if (compilerStats() || executorStats() || otherStats() || detailedStats() ||
               oldFormat() || shortFormat() || tokenizedFormat())
          exe_util_tcb = new(glob->getSpace()) ExExeUtilGetStatisticsTcb(*this, glob);
       else
          exe_util_tcb = new(glob->getSpace()) ExExeUtilGetRTSStatisticsTcb(*this, glob);
    }
    break;
  case SQLCLI_STATS_REQ_QID:
  case SQLCLI_STATS_REQ_QID_INTERNAL:
  case SQLCLI_STATS_REQ_CPU:
  case SQLCLI_STATS_REQ_PID:
  case SQLCLI_STATS_REQ_QID_CURRENT:
  case SQLCLI_STATS_REQ_RMS_INFO:
    exe_util_tcb = new(glob->getSpace()) ExExeUtilGetRTSStatisticsTcb(*this, glob);
    break;
  default:
    ex_assert(0, "Stats Request type not yet supported");
    break;
  }
  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilGetStatisticsTcb
///////////////////////////////////////////////////////////////
ExExeUtilGetStatisticsTcb::ExExeUtilGetStatisticsTcb(
     const ComTdbExeUtilGetStatistics & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilTcb( exe_util_tdb, NULL, glob)
{
  // Allocate the private state in each entry of the down queue
  qparent_.down->allocatePstate(this);

  step_ = INITIAL_;

  // allocate space to hold the stats query that will be used to retrieve
  // statistics. 6K is big enough for it.
  statsQuery_ = new(glob->getDefaultHeap()) char[6144];

  // buffer where output will be formatted
  statsBuf_ = new(glob->getDefaultHeap()) char[4096];

  statsRow_ = NULL;
  statsMergeType_ = SQLCLI_SAME_STATS;
}

ExExeUtilGetStatisticsTcb::~ExExeUtilGetStatisticsTcb()
{
  NADELETEBASIC(statsQuery_, getGlobals()->getDefaultHeap());
  NADELETEBASIC(statsBuf_, getGlobals()->getDefaultHeap());
}

// This method searches for the token string in str.
// It returns start position and length of the next string delimited by
// spaces following the token string except spaces inside quotes.
static short getSubstrInfo(char * str,   // IN
			   short maxLen, // IN
			   const char * token, // IN
			   char * sstrbuf) // OUT
{
  Lng32 startPos, currPos, length;
  const char space = ' ';
  const char quote = '\"';
  char * ptr = str_str(str, token);
  if (! ptr)
    {
      sstrbuf[0] = '\0';
      return -1;
    }
  
  startPos = ptr - str;
  startPos += strlen(token) + 1;
  if (startPos >= maxLen)
    {
      sstrbuf[0] = '\0';
      return -1;
    }  
  
  currPos = startPos;

  // terminate at space
  while(currPos < maxLen && str[currPos] != space)
  {
    // check for quote
    if(str[currPos] == quote)
    {
      currPos++;
      // find end quote
      while(currPos < maxLen && str[currPos] != quote)
          currPos++;
      if (currPos < maxLen)
        currPos++;
    }
    else
      currPos++;
  }

  length = currPos - startPos;

  strncpy(sstrbuf, &str[startPos], length);
  sstrbuf[length] = 0;

  return 0;
}

static const QueryString getStatsAllDefaultViewQuery[] =
{
  {" select a from ( "},
{"    select  "},
{"        trim( "},
{"        case when tdb_id is null then '.  ' "},
{"             else cast(cast(tdb_id as numeric(3)) as char(3)) "},
{"        end || ' ' || "},
{"        case when lc_tdb_id is null then '.  ' "},
{"             else cast(cast(lc_tdb_id as numeric(3)) as char(3)) "},
{"        end || ' ' || "},
{"        case when rc_tdb_id is null then '.  ' "},
{"             else cast(cast(rc_tdb_id as numeric(3)) as char(3)) "},
{"        end || ' ' || "},
{"        case when seq_num is null then '.  ' "},
{"             else cast(cast(seq_num as numeric(3)) as char(3)) "},
{"        end || ' ' || "},
{"        cast(tdb_name as char(18)) || ' ' || "},
{"  "},
{"        substring(cast(cast(sum(est_rows)/count(*) as real) as char(15)), 1, 4) || 'E' ||  "},
{"        substring(cast(cast(sum(est_rows)/count(*) as real) as char(15)), 13, 4) || ' ' || "},
{"  "},
{"        substring(cast(cast(sum(act_rows) as real) as char(15)), 1, 4) || 'E' ||  "},
{"        substring(cast(cast(sum(act_rows) as real) as char(15)), 13, 4) || ' ' || "},
{"        cast(sum(work_calls) as char(8)) || ' ' || "},
{"        ltrim(cast(cast(cast(sum(val1)/1000000 as interval second(12, 6)) "},
{"           as interval hour(2) to second(6)) as char(20))) || ' ' || "},
{"        cast (	 "},
{"          case  "},
{"            when (tdb_name = 'EX_ROOT' or tdb_name = 'EX_EID_ROOT' or  "},
{"                  tdb_name = 'EX_SPLIT_BOTTOM') "},
{"              then case when sum(val2) >= 0 then "},
{"                   ltrim(cast(cast(cast(sum(val2)/1000000 as interval second(12, 6))  "},
{"                        as interval hour(2) to second(6)) as char(20))) "},
{"                 else ' ' "},
{"                 end "},
{"            when tdb_name = 'EX_PARTN_ACCESS'  "},
{"              then trim(cast(min( "},
{"                  substring(substring(text, position('.' in text)+1), "},
{"                     1+ position('.' in substring(text, position('.' in text)+1))) "},
{"                  ) as char(20))) || '|' || "},
{"                trim(cast(sum(zeroifnull(val2)) as char(20))) || '|' || "},
{"                trim(cast(sum(zeroifnull(val3)) as char(20))) || '|' || "},
{"                trim(cast(sum(zeroifnull(val4)) as char(20))) "},
{"            when tdb_name = 'EX_SEND_TOP'  or tdb_name = 'EX_SEND_BOTTOM' "},
{"                 or tdb_name = 'EX_HASHJ' or tdb_name = 'EX_HASH_GRBY'  "},
{"                 or tdb_name = 'EX_SORT'  "},
{"                 or tdb_name = 'EX_SPLIT_TOP'     "},
{"                 or tdb_name = 'EX_FAST_EXTRACT'     "},
{"            then  "},
{"                trim(cast(sum(zeroifnull(val2)) as char(20))) || '|' || "},
{"                trim(cast(sum(zeroifnull(val3)) as char(20))) || '|' || "},
{"                trim(cast(sum(zeroifnull(val4)) as char(20))) "},
{"            else ' ' "},
{"          end "},
{"        as char(45))), "},
{"        tdb_id "},
{"    from table(statistics(null,'STMT=%s,MERGE=%d')) "},
{"  group by tdb_id, lc_tdb_id, rc_tdb_id, seq_num, tdb_name ) x(a,b) order by b desc "},
{" ; "},
};

//
// Used for internal testing to put CompilationStats data into row to be returned
//
void 
ExExeUtilGetStatisticsTcb::moveCompilationStatsToUpQueue(CompilationStatsData *cmpStats)
{
  if( NULL != cmpStats )
  {

    char startTime[26], endTime[26];
    short timestamp[8];

    INTERPRETTIMESTAMP(cmpStats->compileStartTime(), timestamp);
    sprintf(startTime, "%04d/%02d/%02d %02d:%02d:%02d.%03u%03u",                        
		                    timestamp[0], // year
                        timestamp[1], // month
                        timestamp[2], // day
			                  timestamp[3], // hour
                        timestamp[4], // minute
                        timestamp[5], // second
			                  timestamp[6], // fraction
                        timestamp[7]);// fraction

    sprintf(statsBuf_,
				  "Compile Start Time\t: %s", 
				  startTime);
	  moveRowToUpQueue(statsBuf_);

    INTERPRETTIMESTAMP(cmpStats->compileEndTime(), timestamp);
    sprintf(endTime, "%04d/%02d/%02d %02d:%02d:%02d.%03u%03u",                        
		                    timestamp[0], // year
                        timestamp[1], // month
                        timestamp[2], // day
			                  timestamp[3], // hour
                        timestamp[4], // minute
                        timestamp[5], // second
			                  timestamp[6], // fraction
                        timestamp[7]);// fraction

	  sprintf(statsBuf_,
				  "Compile End Time\t: %s", 
				  endTime);
	  moveRowToUpQueue(statsBuf_);

	  sprintf(statsBuf_,
				  "Compiler ID\t\t: %s", 
				  cmpStats->compilerId());
	  moveRowToUpQueue(statsBuf_);

	  sprintf(statsBuf_,
				  "CPU Total\t\t: %d", 
				  cmpStats->cmpCpuTotal());
	  moveRowToUpQueue(statsBuf_);

	  sprintf(statsBuf_,
				  "CPU Binder\t\t: %d", 
				  cmpStats->cmpCpuBinder());
	  moveRowToUpQueue(statsBuf_);

	  sprintf(statsBuf_,
				  "CPU Normalizer\t\t: %d", 
				  cmpStats->cmpCpuNormalizer());
	  moveRowToUpQueue(statsBuf_);

	  sprintf(statsBuf_,
				  "CPU Analyzer\t\t: %d", 
				  cmpStats->cmpCpuAnalyzer());
	  moveRowToUpQueue(statsBuf_);

	  sprintf(statsBuf_,
				  "CPU Optimizer\t\t: %d", 
				  cmpStats->cmpCpuOptimizer());
	  moveRowToUpQueue(statsBuf_);

	  sprintf(statsBuf_,
				  "CPU Generator\t\t: %d", 
				  cmpStats->cmpCpuGenerator());
	  moveRowToUpQueue(statsBuf_);

	  sprintf(statsBuf_,
				  "Metadata Cache Hits\t: %d", 
				  cmpStats->metadataCacheHits());
	  moveRowToUpQueue(statsBuf_);

    sprintf(statsBuf_,
				  "Metadata Cache Lookups\t: %d", 
				  cmpStats->metadataCacheLookups());
	  moveRowToUpQueue(statsBuf_);

    char queryCacheStateBuf[20];

    // See CompilationStats.h for QCacheState values
    switch(cmpStats->queryCacheState())
    {
    case 0:
      sprintf(queryCacheStateBuf,"TEXT");
    break;
    case 1:
      sprintf(queryCacheStateBuf,"TEMPLATE");
    break;
    case 2:
      sprintf(queryCacheStateBuf,"MISS NON-CACHEABLE");
    break;
    case 3:
      sprintf(queryCacheStateBuf,"MISS CACHEABLE");
    break;
    default:
      sprintf(queryCacheStateBuf,"UNKNOWN");
    break;
    };

    sprintf(statsBuf_,
				  "Query Cache State\t: %s", 
				  queryCacheStateBuf);
    moveRowToUpQueue(statsBuf_);

    sprintf(statsBuf_,
				  "Histogram Cache Hits\t: %d", 
				  cmpStats->histogramCacheHits());
	  moveRowToUpQueue(statsBuf_);

    sprintf(statsBuf_,
				  "Histogram Cache Lookups\t: %d", 
				  cmpStats->histogramCacheLookups());
	  moveRowToUpQueue(statsBuf_);

    sprintf(statsBuf_,
				  "Statement Heap Size\t: %d", 
				  cmpStats->stmtHeapSize());
	  moveRowToUpQueue(statsBuf_);

    sprintf(statsBuf_,
				  "Context Heap Size\t: %d", 
				  cmpStats->contextHeapSize());
	  moveRowToUpQueue(statsBuf_);

    sprintf(statsBuf_,
				  "Optimization Tasks\t: %d", 
				  cmpStats->optTasks());
	  moveRowToUpQueue(statsBuf_);

    sprintf(statsBuf_,
				  "Optimization Contexts\t: %d", 
				  cmpStats->optContexts());
	  moveRowToUpQueue(statsBuf_);

    if( cmpStats->isRecompile() )
    {
      sprintf(statsBuf_, "Is Recompile\t\t: YES");
    }
    else
    {
      sprintf(statsBuf_, "Is Recompile\t\t: NO");
    }
	  moveRowToUpQueue(statsBuf_);

    sprintf(statsBuf_,
				  "Compile Info\t\t: %s", 
				  cmpStats->compileInfo());
	  moveRowToUpQueue(statsBuf_);

    sprintf(statsBuf_,
				  "Compile Info Length\t: %d", 
				  cmpStats->compileInfoLen());
	  moveRowToUpQueue(statsBuf_);
  }
}

//////////////////////////////////////////////////////
// work() for ExExeUtilGetStatisticsTcb
//////////////////////////////////////////////////////
short ExExeUtilGetStatisticsTcb::work()
{
  //  short rc = 0;
  Lng32 cliRC = 0;
  char sstrbuf[ComMAX_ANSI_NAME_EXTERNAL_LEN+1];

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;
  
  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;
  
  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ContextCli * currContext = masterGlob->getStatement()->getContext();

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
	    // find the stats area.
	    // If statement name is specified, look for it in the statement
	    // list.
	    // Otherwise, get the current stats area from context.

	    HashQueue * stmtList = currContext->getStatementList();
	    stmtList->position();
	    char * inputStmtName = getStatsTdb().stmtName_;
	    stats_ = NULL;
	    if (inputStmtName != NULL)
            {
		Statement * stmt = NULL;
		NABoolean found = FALSE;
		while ((NOT found) &&
		       (stmt = (Statement *)stmtList->getNext()))
                {
		  const char *ident = stmt->getIdentifier();
		    
		  if ((ident) &&
			(str_len(inputStmtName) == str_len(ident)) &&
			(str_cmp(inputStmtName, ident, str_len(ident)) == 0)) // matches
		  {
		    stats_ = stmt->getStatsArea();
                    if (stats_ == NULL)
                      stats_ = stmt->getCompileStatsArea();
                    found = TRUE;
                  }
                } // while
	    }
	    else
	    {
		stats_ = currContext->getStats();
	    }

	    hdfsAccess_ = 0;

	    if (stats_)
            {
	      step_ = RETURN_COMPILER_STATS_;
              if (getStatsTdb().statsMergeType_ == SQLCLI_DEFAULT_STATS)
              {
                SessionDefaults *sd = currContext->getSessionDefaults();
                if (sd)
                  statsMergeType_ = (short)sd->getStatisticsViewType();
                else
                  statsMergeType_ = stats_->getOrigCollectStatsType();
              }
              else
                statsMergeType_ = getStatsTdb().statsMergeType_;
              if (statsMergeType_ == SQLCLI_SAME_STATS || 
                // If the collection stats type is ALL_STATS, ignore the statsMergeType
                stats_->getOrigCollectStatsType() == ComTdb::ALL_STATS)
                  statsMergeType_ = stats_->getOrigCollectStatsType();
            }
	    else
	      step_ = DONE_;
	  }
	break;

	case RETURN_COMPILER_STATS_:
	  {
	    if (NOT getStatsTdb().compilerStats())
	      {
		step_ = RETURN_EXECUTOR_STATS_;
		break;
	      }

	    // make sure there is enough space to move master stats
	    if ((qparent_.up->getSize() - qparent_.up->getLength()) < 20)
	      return WORK_CALL_AGAIN;	//come back later
	    
	    moveRowToUpQueue(" ");
	    
	    ExMasterStats * masterStats = stats_->getMasterStats();
	    QueryCostInfo &qci = masterStats->queryCostInfo();
	    CompilerStatsInfo &csi = masterStats->compilerStatsInfo();

	    moveRowToUpQueue("Compiler Statistics");
	    moveRowToUpQueue("===================");
	    moveRowToUpQueue(" ");

	    char formattedFloatVal[25];
	    Lng32 intSize = 0;
	    Lng32 valSize = 0;

            strcpy(statsBuf_, "Stats Collection Type  ");
            strcat(statsBuf_, ExStatisticsArea::getStatsTypeText(csi.collectStatsType()));
            moveRowToUpQueue(statsBuf_);
	    strcpy(statsBuf_, "Cost(units)            ");
	    FormatFloat(formattedFloatVal, intSize, valSize, qci.cpuTime(),
			FALSE, TRUE);
	    strcat(statsBuf_, "CPU: ");
	    strcat(statsBuf_, formattedFloatVal);

	    FormatFloat(formattedFloatVal, intSize, valSize, qci.ioTime(),
			FALSE, TRUE);
	    strcat(statsBuf_, "  IO: ");
	    strcat(statsBuf_, formattedFloatVal);

	    FormatFloat(formattedFloatVal, intSize, valSize, qci.msgTime(),
			FALSE, TRUE);
	    strcat(statsBuf_, "  Msg: ");
	    strcat(statsBuf_, formattedFloatVal);

	    FormatFloat(formattedFloatVal, intSize, valSize, qci.idleTime(),
			FALSE, TRUE);
	    strcat(statsBuf_, "  Idle: ");
	    strcat(statsBuf_, formattedFloatVal);
	    moveRowToUpQueue(statsBuf_);

	    FormatFloat(formattedFloatVal, intSize, valSize, qci.numSeqIOs(),
			FALSE, TRUE);
	    strcat(statsBuf_, "  SeqIOs: ");
	    strcat(statsBuf_, formattedFloatVal);
	    moveRowToUpQueue(statsBuf_);

	    FormatFloat(formattedFloatVal, intSize, valSize, qci.numRandIOs(),
			FALSE, TRUE);
	    strcat(statsBuf_, "  RandIOs: ");
	    strcat(statsBuf_, formattedFloatVal);
	    moveRowToUpQueue(statsBuf_);

	    FormatFloat(formattedFloatVal, intSize, valSize, qci.totalTime(),
			FALSE, TRUE);
	    strcpy(statsBuf_,      "                       TotalTime: ");
	    strcat(statsBuf_, formattedFloatVal);

	    FormatFloat(formattedFloatVal, intSize, valSize, qci.totalMem(),
			FALSE, TRUE);
	    strcat(statsBuf_,      "  TotalMem: ");
	    strcat(statsBuf_, formattedFloatVal);

	    FormatFloat(formattedFloatVal, intSize, valSize, qci.totalMem(),
			FALSE, TRUE);
	    strcat(statsBuf_,      "  EstTotalMem: ");
	    strcat(statsBuf_, formattedFloatVal);
	    strcat(statsBuf_, " bytes");

	    FormatFloat(formattedFloatVal, intSize, valSize, qci.maxCpuUsage(),
	    			FALSE, TRUE);
	    strcat(statsBuf_,      "  MaxCpuUse: ");
	    strcat(statsBuf_, formattedFloatVal);
	    strcat(statsBuf_, "%");

	    moveRowToUpQueue(statsBuf_);

	    FormatFloat(formattedFloatVal, intSize, valSize, qci.cardinality(),
			FALSE, TRUE);
	    strcpy(statsBuf_, "Rows                   ReturnedToUser: ");
	    strcat(statsBuf_, formattedFloatVal);
	    moveRowToUpQueue(statsBuf_);

	    sprintf(statsBuf_, "Fragment Size(Kb)      Total: %-4d  Master: %-4d  ESP: %-4d  ",
			csi.totalFragmentSize(),
			csi.masterFragmentSize(),
	        	csi.espFragmentSize());
	    moveRowToUpQueue(statsBuf_);

	    sprintf(statsBuf_, "Operators              Total: %-3d",
			csi.totalOps());
	    moveRowToUpQueue(statsBuf_);
	    
            sprintf(statsBuf_, "  Joins                HJ: %-3d  MJ: %-3d  NJ: %-3d   Total: %-3d",
			csi.hj(), csi.mj(), csi.nj(),  csi.totalJoins());
	    moveRowToUpQueue(statsBuf_);
            sprintf(statsBuf_, "  Others               ESPExchange: %-3d  UDR: %-3d  BMO: %-3d",  
			csi.exchangeOps(), csi.udr(), csi.bmo());

	    moveRowToUpQueue(statsBuf_);
            
	    if (masterStats->numOfRootEsps() > 0)
	      {
		sprintf(statsBuf_, "Parallelism            Root(%d)  DOP(%d)",
			    masterStats->numOfRootEsps(), csi.dop());
	      }
	    else
	      {
		sprintf(statsBuf_, "Parallelism            Root(0)  DOP(%d)",
			    csi.dop());
	      }
	    moveRowToUpQueue(statsBuf_);
	      
	    sprintf(statsBuf_,   "Query Characteristics  MandatoryXP: %s  MissingStats: %s  FullScanOnTable: %s ",
			(csi.mandatoryCrossProduct() ? "Yes" : "No"),
			(csi.missingStats() ? "Yes" : "No"),
			(csi.fullScanOnTable() ? "Yes" : "No"));
	    moveRowToUpQueue(statsBuf_);

	    if (csi.fullScanOnTable())
	      {
		sprintf(statsBuf_,"                       RowsAccessedByFullScan %f",
			    csi.dp2RowsAccessedForFullScan());
		moveRowToUpQueue(statsBuf_);
	      }

	    sprintf(statsBuf_,   "Query Execution        DOP: %d  Affinity: %d  XnReqd: %s",
			csi.dop(), csi.affinityNumber(), 
			(masterStats->xnReqd() ? "Yes" : "No"));
	    moveRowToUpQueue(statsBuf_);
            if (csi.bmo() > 0)
            {
              sprintf(statsBuf_, "                       OverFlowMode: %s  OverFlowSize: %f",
			ExBMOStats::getScratchOverflowMode(csi.ofMode()),
                        csi.ofSize());
	      moveRowToUpQueue(statsBuf_);
            }

	    moveRowToUpQueue(" ");

	    step_ = RETURN_EXECUTOR_STATS_;
	  }
	break;

	case RETURN_EXECUTOR_STATS_:
	  {
	    if (NOT getStatsTdb().executorStats())
	      {
		step_ = RETURN_OTHER_STATS_;
		break;
	      }

	    // make sure there is enough space to move master stats
	    if ((qparent_.up->getSize() - qparent_.up->getLength()) < 20)
	      return WORK_CALL_AGAIN;	//come back later
	    
	    moveRowToUpQueue(" ");
	    
	    ExMasterStats * masterStats = stats_->getMasterStats();

	    if (((! masterStats) ||
		 (masterStats->getElapsedStartTime() == -1) ||
		 (masterStats->getElapsedEndTime() == -1)))
	      {
		step_ = DONE_;
		break;
	      }

	    if ((NOT getStatsTdb().oldFormat()) &&
		(NOT getStatsTdb().shortFormat()))
	      {
		moveRowToUpQueue("Executor Statistics");
		moveRowToUpQueue("===================");
		moveRowToUpQueue(" ");
	      }

	    // Display Start Time
	    short timestamp[8];
	    Int64 juliantimestamp = 
	      CONVERTTIMESTAMP(masterStats->getElapsedStartTime(),0,-1,0);
	    
	    INTERPRETTIMESTAMP(juliantimestamp, timestamp);
	    sprintf(statsBuf_, "Start Time             %04d/%02d/%02d %02d:%02d:%02d.%03u%03u",
			timestamp[0], timestamp[1], timestamp[2],
			timestamp[3], timestamp[4], timestamp[5],
			timestamp[6], timestamp[7]);
	    moveRowToUpQueue(statsBuf_);
	    
	    // Display first row return time
	    if ((masterStats->getFirstRowReturnTime() != -1) &&
		(NOT getStatsTdb().oldFormat()) &&
		(NOT getStatsTdb().shortFormat()))
	      {
		juliantimestamp = 
		  CONVERTTIMESTAMP(masterStats->getFirstRowReturnTime(),0,-1,0);
		
		INTERPRETTIMESTAMP(juliantimestamp, timestamp);
		sprintf(statsBuf_, "First Row Returned     %04d/%02d/%02d %02d:%02d:%02d.%03u%03u",
			    timestamp[0], timestamp[1], timestamp[2],
			    timestamp[3], timestamp[4], timestamp[5],
			    timestamp[6], timestamp[7]);
		moveRowToUpQueue(statsBuf_);
	      }

	    // Display End Time
	    juliantimestamp = 
	      CONVERTTIMESTAMP(masterStats->getElapsedEndTime(),0,-1,0);

	    INTERPRETTIMESTAMP(juliantimestamp, timestamp);
	    sprintf(statsBuf_, "End Time               %04d/%02d/%02d %02d:%02d:%02d.%03u%03u",
			timestamp[0], timestamp[1], timestamp[2],
			timestamp[3], timestamp[4], timestamp[5],
			timestamp[6], timestamp[7]);
	    moveRowToUpQueue(statsBuf_);
	    
	    // Display Elapsed Time
	    Int64 elapsedTime = masterStats->getElapsedEndTime() -
	      masterStats->getElapsedStartTime();
	    if (elapsedTime < 0)
	      elapsedTime = 0;
	    ULng32 sec = (ULng32) (elapsedTime / 1000000);
	    ULng32 usec = (ULng32) (elapsedTime % 1000000);
	    ULng32 min = sec/60;
	    sec = sec % 60;
	    ULng32 hour = min/60;
	    min = min % 60;
	    sprintf (statsBuf_,  "Elapsed Time                      %02u:%02u:%02u.%06u",
			 hour, min, sec, usec);
	    moveRowToUpQueue(statsBuf_);
	    
	    // Display Compile Time
	    Int64 compileTime = 
	      (((masterStats->isPrepare() || masterStats->isPrepAndExec()) &&
		(masterStats->getCompEndTime() != -1)) ?
	       masterStats->getCompEndTime() -
	       masterStats->getCompStartTime() : 0);
	    if (compileTime < 0)
	      compileTime = 0;
	    sec = (ULng32) (compileTime / 1000000);
	    usec = (ULng32) (compileTime % 1000000);
	    min = sec/60;
	    sec = sec % 60;
	    hour = min/60;
	    min = min % 60;
	    sprintf (statsBuf_,  "Compile Time                      %02u:%02u:%02u.%06u",
			 hour, min, sec, usec);
	    moveRowToUpQueue(statsBuf_);

	    if ((NOT getStatsTdb().oldFormat()) &&
		(NOT getStatsTdb().shortFormat()))
	      {
		// Display Fixup Time
		Int64 fixupTime = 
		  ((masterStats->getFixupEndTime() != -1) ?
		   (masterStats->getFixupEndTime() -
		    masterStats->getFixupStartTime()) : 0);
		if (fixupTime < 0)
		  fixupTime = 0;
		sec = (ULng32) (fixupTime / 1000000);
		usec = (ULng32) (fixupTime % 1000000);
		min = sec/60;
		sec = sec % 60;
		hour = min/60;
		min = min % 60;
		sprintf (statsBuf_,  "Fixup Time                        %02u:%02u:%02u.%06u",
			     hour, min, sec, usec);
		moveRowToUpQueue(statsBuf_);
		
		// Display Freeup Time
		Int64 freeupTime = masterStats->getFreeupEndTime() -
		  masterStats->getFreeupStartTime();
		if (freeupTime < 0)
		  freeupTime = 0;
		sec = (ULng32) (freeupTime / 1000000);
		usec = (ULng32) (freeupTime % 1000000);
		min = sec/60;
		sec = sec % 60;
		hour = min/60;
		min = min % 60;
		sprintf (statsBuf_,  "Freeup Time                       %02u:%02u:%02u.%06u",
			     hour, min, sec, usec);
		moveRowToUpQueue(statsBuf_);
	      }

	    // Display Execution Time
	    Int64 executionTime = masterStats->getExeEndTime() -
	      masterStats->getExeStartTime();
	    if (executionTime < 0)
	      executionTime = 0;
	    sec = (ULng32) (executionTime / 1000000);
	    usec = (ULng32) (executionTime % 1000000);
	    min = sec/60;
	    sec = sec % 60;
	    hour = min/60;
	    min = min % 60;
	    sprintf (statsBuf_,  "Execution Time                    %02u:%02u:%02u.%06u",
			 hour, min, sec, usec);
	    moveRowToUpQueue(statsBuf_);

	    if ((NOT getStatsTdb().oldFormat()) &&
		(NOT getStatsTdb().shortFormat()))
	      {
		sprintf (statsBuf_,  "Rows Affected          %ld",
			     masterStats->getRowsAffected());
		moveRowToUpQueue(statsBuf_);
	      }

#if defined(_DEBUG)
                    if( getenv("DISPLAY_COMPILATION_STATS"))
                    {			
                      ComTdbRoot *rootTdb = 
                        (ComTdbRoot*) masterGlob->getStatement()->getRootTdb();			
                      CompilationStatsData *cmpStats = rootTdb->getCompilationStatsData();	
                      if (cmpStats != NULL)
                      {
                        moveRowToUpQueue(" ");
                        moveRowToUpQueue("Compilation Stats");
                        moveRowToUpQueue("===================");
                        moveRowToUpQueue(" ");
                    	
                        moveCompilationStatsToUpQueue(cmpStats);
                      }
                    }
#endif // _DEBUG

	    moveRowToUpQueue(" ");

	    
	    step_ = RETURN_OTHER_STATS_;
	  }
	break;

	case RETURN_OTHER_STATS_:
	  {
	    if (NOT getStatsTdb().otherStats())
	      {
		step_ = SETUP_DETAILED_STATS_;
		break;
	      }

	    // make sure there is enough space to move master stats
	    if ((qparent_.up->getSize() - qparent_.up->getLength()) < 22)
	      return WORK_CALL_AGAIN;	//come back later
	    
	    moveRowToUpQueue(" ");
	    
	    ExMasterStats * masterStats = stats_->getMasterStats();

	    moveRowToUpQueue("Other Statistics");
	    moveRowToUpQueue("================");
	    moveRowToUpQueue(" ");
	    
	    if (masterStats->getQueryId())
	      {
		// split queryId into multiple lines
		char line[80];

		strcpy(line, "UniqueQueryId          ");
		strncat(line, masterStats->getQueryId(), 43);
		sprintf(statsBuf_, line);
		moveRowToUpQueue(statsBuf_);

		strcpy(line, "                       ");
		strcat(line, &masterStats->getQueryId()[43]);
		sprintf(statsBuf_, line);
		moveRowToUpQueue(statsBuf_);

		//sprintf (statsBuf_,    "UniqueQueryId          %s",
		//	     masterStats->getQueryId());
	      }
	    else
	      {
		sprintf (statsBuf_,    "UniqueQueryId          NULL");
		moveRowToUpQueue(statsBuf_);
	      }

	    if (masterStats->compilerCacheHit())
	      sprintf (statsBuf_,  "Compiler Cache Hit     Yes");
	    else
	      sprintf (statsBuf_,  "Compiler Cache Hit     No");
	    moveRowToUpQueue(statsBuf_);
	    sprintf (statsBuf_,    "Executor Cache Hit     No");
	    moveRowToUpQueue(statsBuf_);

	    sprintf (statsBuf_,    "ESPs                   Total: %-4d  Reused: %-4d  New: %-4d",
			 (masterStats->numOfTotalEspsUsed() > 0 ? masterStats->numOfTotalEspsUsed() : 0),
			 (masterStats->numOfTotalEspsUsed() > 0 ? masterStats->numOfTotalEspsUsed() : 0) -
			 (masterStats->numOfNewEspsStarted() > 0 ? masterStats->numOfNewEspsStarted() : 0),
			 (masterStats->numOfNewEspsStarted() > 0 ? masterStats->numOfNewEspsStarted() : 0)
			 );
	    moveRowToUpQueue(statsBuf_);


	    moveRowToUpQueue(" ");

	    step_ = SETUP_DETAILED_STATS_;
	  }
	break;

	case SETUP_DETAILED_STATS_:
	  {
	    if (NOT getStatsTdb().detailedStats())
	      {
		step_ = DONE_;
		break;
	      }

	    if (statsMergeType_ == SQLCLIDEV_NO_STATS)
	      {
		step_ = DONE_;
		break;
	      }

	    // set sqlparserflags to disable stats collection. We don't want
	    // to collect stats on these internal statements as that will
	    // override stats collected for the actual user statement.
	    currContext->setSqlParserFlags(0x00080000); //DISABLE_RUNTIME_STATS

	    // disable any CQS in affect
	    cliRC = cliInterface()->
	      executeImmediate("control query shape hold;");
	    if (cliRC < 0)
	      {
		ExHandleErrors(qparent_,
			       pentry_down,
			       0,
			       getGlobals(),
			       NULL,
			       (ExeErrorCode)cliRC,
			       NULL,
			       NULL
			       );
		step_ = HANDLE_ERROR_;
		break;
	      }

            if ((statsMergeType_ == SQLCLIDEV_ACCUMULATED_STATS) ||
		(statsMergeType_ == SQLCLIDEV_PERTABLE_STATS) ||
                (statsMergeType_ == SQLCLI_PROGRESS_STATS))
	      {
                char * s = getStatsTdb().stmtName_;
                // Needs to use the getStatsTdb.statsMergeType_ since MERGE can't take operator stats as MERGE token
                // value and str_parse_stmt_name will convert it to the relevant stats type
                if (s == NULL)
		  sprintf(statsQuery_, "select variable_info from table(statistics(null, 'STMT=CURRENT,MERGE=%d'));",
                              getStatsTdb().statsMergeType_);
		else
		  sprintf(statsQuery_, "select variable_info from table(statistics(null, 'STMT=%s,MERGE=%d'));",
				s, getStatsTdb().statsMergeType_);
	      }
	    else
	      {
		Int32 stats_qry_array_size = 
		  sizeof(getStatsAllDefaultViewQuery) / sizeof(QueryString);
		const QueryString * getStatsQueryString =
		  getStatsAllDefaultViewQuery;

		char * gluedQuery;
		Lng32 gluedQuerySize;
		glueQueryFragments(stats_qry_array_size, 
				   getStatsQueryString,
				   gluedQuery, gluedQuerySize);
		
                const char *s = getStatsTdb().stmtName_;
                if (s == NULL)
                  s = "CURRENT";
                sprintf(statsQuery_, gluedQuery, s, getStatsTdb().statsMergeType_);
                NADELETEBASIC(gluedQuery, getMyHeap());
            }
	    step_ = FETCH_PROLOGUE_;
	  }
	break;

	case FETCH_PROLOGUE_:
	  {
	    cliRC = cliInterface()->fetchRowsPrologue(statsQuery_);
	    if (cliRC < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		step_ = HANDLE_ERROR_;
		break;
	      }

	    step_ = FETCH_FIRST_STATS_ROW_;
	    
	  }
	break;

	case FETCH_FIRST_STATS_ROW_:
	  {
	    cliRC = cliInterface()->fetch();
	    if (cliRC < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		step_ = HANDLE_ERROR_;
		break;
	      }
            if (cliRC == 100) // EOD
	      	step_ = FETCH_EPILOGUE_;
            else
            {
	      // Skip ROOT_OPER_STATS_ROW or BMO_STATS row
              if (statsMergeType_ == SQLCLIDEV_PERTABLE_STATS || statsMergeType_ == SQLCLI_PROGRESS_STATS) 
              {
                cliInterface()->getPtrAndLen(1, statsRow_, statsRowlen_);
                getSubstrInfo(statsRow_, statsRowlen_, "statsRowType:", sstrbuf);
                short statsRowType = (short)str_atoi(sstrbuf, str_len(sstrbuf));
                if (statsRowType == ExOperStats::ROOT_OPER_STATS || statsRowType == ExOperStats::BMO_STATS ||
                  statsRowType == ExOperStats::UDR_BASE_STATS)
		  {
		    if (statsRowType == ExOperStats::ROOT_OPER_STATS)
		      {
			hdfsAccess_ = 0;
			if (getSubstrInfo(statsRow_, statsRowlen_, "hdfsAccess:", sstrbuf) == 0)
			  {
			    hdfsAccess_ = (short)str_atoi(sstrbuf, str_len(sstrbuf));
			  }
		      }

		    step_ = FETCH_FIRST_STATS_ROW_;
		  }
                else
                  step_ = DISPLAY_HEADING_;
              }
              else
                step_ = DISPLAY_HEADING_;
            }           
          }
	  break;
	case DISPLAY_HEADING_:
	  {
	    // make sure there is enough space to move header
	    if ((qparent_.up->getSize() - qparent_.up->getLength()) < 10)
	      return WORK_CALL_AGAIN;	//come back later

	    moveRowToUpQueue(" ");

	    char statsType[50];

	    if ((NOT getStatsTdb().oldFormat()) &&
		(NOT getStatsTdb().shortFormat()))
	      {
		if (statsMergeType_ == SQLCLIDEV_ACCUMULATED_STATS)
		  strcpy(statsType, "ACCUMULATED");
		else if (statsMergeType_ == SQLCLIDEV_PERTABLE_STATS)
		  strcpy(statsType, "PERTABLE");
                else if(statsMergeType_ == SQLCLI_PROGRESS_STATS)
                  strcpy(statsType, "PROGRESS");
		else if (statsMergeType_ == SQLCLIDEV_ALL_STATS)
		  strcpy(statsType, "ALL");
		else if (statsMergeType_ == SQLCLIDEV_OPERATOR_STATS)
		  strcpy(statsType, "OPERATOR");
		
		strcpy(statsBuf_, "Detailed Statistics (");
		strcat(statsBuf_, statsType);
		strcat(statsBuf_, ")");
		moveRowToUpQueue(statsBuf_);

		str_pad(statsBuf_, (Int32)strlen(statsBuf_), '=');
		moveRowToUpQueue(statsBuf_);
		moveRowToUpQueue(" ");
	      }

	    if ((statsMergeType_ == SQLCLIDEV_ALL_STATS) ||
		(statsMergeType_ == SQLCLIDEV_PERTABLE_STATS) ||
		(statsMergeType_ == SQLCLIDEV_OPERATOR_STATS) ||
                (statsMergeType_ == SQLCLI_PROGRESS_STATS))
	      {
		//the fetch was successful
		if (statsMergeType_ == SQLCLIDEV_PERTABLE_STATS || statsMergeType_ == SQLCLI_PROGRESS_STATS)
		  {
		    if (hdfsAccess_)
		      {
			sprintf(statsBuf_, "%-15s%15s%15s%10s%15s%15s",
				    "Table Name", "Records", "Records", "Hdfs",  "Hdfs I/O", "Hdfs Access");
			moveRowToUpQueue(statsBuf_);
			
			sprintf(statsBuf_, "%15s%15s%15s%10s%15s%15s",
				" ", "Accessed", "Used", "I/Os", "Bytes", "Time(usec)");

		      }
		    else if (getStatsTdb().oldFormat())
		      {
			sprintf(statsBuf_, "%-15s%15s%12s%8s%10s%13s%6s",
				    "Table Name", "Records", "Records", "Disk", "Message", "Message", "Lock");
			moveRowToUpQueue(statsBuf_);
			
			sprintf(statsBuf_, "%15s%15s%12s%8s%10s%13s%6s",
				    " ", "Accessed", "Used", "I/Os", "Count", "Bytes", "");
		      }
		    else
		      {
			sprintf(statsBuf_, "%-15s%15s%15s%10s%10s%15s%6s%6s%15s",
				    "Table Name", "Records", "Records", "Disk", "Message", "Message", "Lock", "Lock", "Disk Process");
			moveRowToUpQueue(statsBuf_);
			
			sprintf(statsBuf_, "%15s%15s%15s%10s%10s%15s%6s%6s%15s",
				" ", "Accessed", "Used", "I/Os", "Count", "Bytes", "Escl", "Wait", "Busy Time");
		      }

		    moveRowToUpQueue(statsBuf_);
		  }
		else if ((statsMergeType_ == SQLCLIDEV_ALL_STATS) ||
			 (statsMergeType_ == SQLCLIDEV_OPERATOR_STATS))
		  {
		    // Construct heading.
		    sprintf(statsBuf_, "%4s%4s%4s%4s%19s%9s%9s%9s%20s%20s",
				"OP", "LC", "RC", "EX", "TDB_NAME", "EstRows", "ActRows", "WC", "Oper Cpu Time", "Details");
		    moveRowToUpQueue(statsBuf_);
		    sprintf(statsBuf_, "%4s%4s%4s%4s%19s%9s%9s%9s%20s%20s",
				"--", "--", "--", "--", "--------", "-------", "-------", "--", "-------------", "-------");
		    moveRowToUpQueue(statsBuf_);
		    moveRowToUpQueue(" ");
		  }
	      }
	    else
	      {
		moveRowToUpQueue(" ");
	      }

	    step_ = RETURN_STATS_ROW_;
	  }
	break;
	
	case FETCH_STATS_ROW_:
	  {
	    cliRC = cliInterface()->fetch();
	    if (cliRC < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		step_ = HANDLE_ERROR_;
		break;
	      }
	    
	    if (cliRC == 100) //no more data
              step_ = FETCH_EPILOGUE_;
	    else
            {
              // Skip ROOT_OPER_STATS_ROW or BMO_STATS row
              if (statsMergeType_ == SQLCLIDEV_PERTABLE_STATS || statsMergeType_ == SQLCLI_PROGRESS_STATS) 
              {
                cliInterface()->getPtrAndLen(1, statsRow_, statsRowlen_);
                getSubstrInfo(statsRow_, statsRowlen_, "statsRowType:", sstrbuf);
                short statsRowType = (short)str_atoi(sstrbuf, str_len(sstrbuf));
                if (statsRowType == ExOperStats::ROOT_OPER_STATS || statsRowType == ExOperStats::BMO_STATS
                  || statsRowType == ExOperStats::UDR_BASE_STATS)
                  step_ = FETCH_STATS_ROW_;
                else
              	  step_ = RETURN_STATS_ROW_;
              }
              else
                  step_ = RETURN_STATS_ROW_;
            }
	  }
	break;
	
	case RETURN_STATS_ROW_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    cliInterface()->getPtrAndLen(1, statsRow_, statsRowlen_);

	    if ((statsMergeType_ == SQLCLIDEV_ALL_STATS) ||
		(statsMergeType_ == SQLCLIDEV_OPERATOR_STATS))
	      step_ = FORMAT_AND_RETURN_ALL_STATS_;
	    else if (statsMergeType_ == SQLCLIDEV_PERTABLE_STATS || statsMergeType_ == SQLCLI_PROGRESS_STATS)
	      step_ = FORMAT_AND_RETURN_PERTABLE_STATS_;
	    else if (statsMergeType_ == SQLCLIDEV_ACCUMULATED_STATS)
	      step_ = FORMAT_AND_RETURN_ACCUMULATED_STATS_;
	    else
	      step_ = HANDLE_ERROR_;
	  }
	break;
	
	case FORMAT_AND_RETURN_PERTABLE_STATS_:
	  {
	    if ((qparent_.up->getSize() - qparent_.up->getLength()) < 4)
	      return WORK_CALL_AGAIN;	//come back later

	    getSubstrInfo(statsRow_, statsRowlen_, "AnsiName:", sstrbuf);
	    sprintf(statsBuf_, "%15s", sstrbuf);
	    moveRowToUpQueue(statsBuf_);

	    if (hdfsAccess_)
            {
	      sprintf(statsBuf_, "%15s", " ");
	      getSubstrInfo(statsRow_, statsRowlen_, "AccessedRows:", sstrbuf);
	      sprintf(&statsBuf_[strlen(statsBuf_)], "%15s", sstrbuf);

	      getSubstrInfo(statsRow_, statsRowlen_, "UsedRows:", sstrbuf);
	      sprintf(&statsBuf_[strlen(statsBuf_)], "%15s", sstrbuf);

	      getSubstrInfo(statsRow_, statsRowlen_, "HbaseSumIOCalls:", sstrbuf);
	      sprintf(&statsBuf_[strlen(statsBuf_)], "%10s", sstrbuf);

	      getSubstrInfo(statsRow_, statsRowlen_, "MessagesBytes:", sstrbuf);
	      sprintf(&statsBuf_[strlen(statsBuf_)], "%15s", sstrbuf);

	      getSubstrInfo(statsRow_, statsRowlen_, "HbaseSumIOTime:", sstrbuf);
	      sprintf(&statsBuf_[strlen(statsBuf_)], "%15s", sstrbuf);
	    }
            else if (getStatsTdb().oldFormat())
            {
              sprintf(statsBuf_, "%15s", " ");
	      getSubstrInfo(statsRow_, statsRowlen_, "AccessedRows:", sstrbuf);
	      sprintf(&statsBuf_[strlen(statsBuf_)], "%15s", sstrbuf);

	      getSubstrInfo(statsRow_, statsRowlen_, "UsedRows:", sstrbuf);
	      sprintf(&statsBuf_[strlen(statsBuf_)], "%12s", sstrbuf);

	      getSubstrInfo(statsRow_, statsRowlen_, "DiskIOs:", sstrbuf);
	      sprintf(&statsBuf_[strlen(statsBuf_)], "%8s", sstrbuf);

	      getSubstrInfo(statsRow_, statsRowlen_, "NumMessages:", sstrbuf);
	      sprintf(&statsBuf_[strlen(statsBuf_)], "%10s", sstrbuf);

	      getSubstrInfo(statsRow_, statsRowlen_, "MessagesBytes:", sstrbuf);
	      sprintf(&statsBuf_[strlen(statsBuf_)], "%13s", sstrbuf);

	      getSubstrInfo(statsRow_, statsRowlen_, "Escalations:", sstrbuf);
	      sprintf(&statsBuf_[strlen(statsBuf_)], "%6s", sstrbuf);
            }
            else
            {
	      sprintf(statsBuf_, "%15s", " ");
	      getSubstrInfo(statsRow_, statsRowlen_, "AccessedRows:", sstrbuf);
	      sprintf(&statsBuf_[strlen(statsBuf_)], "%15s", sstrbuf);

	      getSubstrInfo(statsRow_, statsRowlen_, "UsedRows:", sstrbuf);
	      sprintf(&statsBuf_[strlen(statsBuf_)], "%15s", sstrbuf);

	      getSubstrInfo(statsRow_, statsRowlen_, "DiskIOs:", sstrbuf);
	      sprintf(&statsBuf_[strlen(statsBuf_)], "%10s", sstrbuf);

	      getSubstrInfo(statsRow_, statsRowlen_, "NumMessages:", sstrbuf);
	      sprintf(&statsBuf_[strlen(statsBuf_)], "%10s", sstrbuf);

	      getSubstrInfo(statsRow_, statsRowlen_, "MessagesBytes:", sstrbuf);
	      sprintf(&statsBuf_[strlen(statsBuf_)], "%15s", sstrbuf);

	      getSubstrInfo(statsRow_, statsRowlen_, "Escalations:", sstrbuf);
	      sprintf(&statsBuf_[strlen(statsBuf_)], "%6s", sstrbuf);

	      getSubstrInfo(statsRow_, statsRowlen_, "LockWaits:", sstrbuf);
	      sprintf(&statsBuf_[strlen(statsBuf_)], "%6s", sstrbuf);
  	    
	      getSubstrInfo(statsRow_, statsRowlen_, "ProcessBusyTime:", sstrbuf);
	      sprintf(&statsBuf_[strlen(statsBuf_)], "%15s", sstrbuf);
	    }

	    moveRowToUpQueue(statsBuf_);

	    step_ = FETCH_STATS_ROW_;
	  }
	break;

	case FORMAT_AND_RETURN_ACCUMULATED_STATS_:
	  {
	    // make sure there is enough space to move header
	    if ((qparent_.up->getSize() - qparent_.up->getLength()) < 31)
	      return WORK_CALL_AGAIN;	//come back later
	   
	    getSubstrInfo(statsRow_, statsRowlen_, "AccessedRows:", sstrbuf);
	    sprintf(statsBuf_, "%-25s%10s", "Accessed Rows", sstrbuf);
	    moveRowToUpQueue(statsBuf_);
	
	    getSubstrInfo(statsRow_, statsRowlen_, "UsedRows:", sstrbuf);
	    sprintf(statsBuf_, "%-25s%10s", "Used Rows", sstrbuf);
	    moveRowToUpQueue(statsBuf_);

	    getSubstrInfo(statsRow_, statsRowlen_, "NumMessages:", sstrbuf);
	    sprintf(statsBuf_, "%-25s%10s", "Message Count", sstrbuf);
	    moveRowToUpQueue(statsBuf_);

	    getSubstrInfo(statsRow_, statsRowlen_, "MessagesBytes:", sstrbuf);
	    sprintf(statsBuf_, "%-25s%10s", "Message Bytes", sstrbuf);
	    moveRowToUpQueue(statsBuf_);

	    getSubstrInfo(statsRow_, statsRowlen_, "StatsBytes:", sstrbuf);
	    sprintf(statsBuf_, "%-25s%10s", "Stats Bytes", sstrbuf);
	    moveRowToUpQueue(statsBuf_);

	    getSubstrInfo(statsRow_, statsRowlen_, "DiskIOs:", sstrbuf);
	    sprintf(statsBuf_, "%-25s%10s", "Disk IOs", sstrbuf);
	    moveRowToUpQueue(statsBuf_);

	    getSubstrInfo(statsRow_, statsRowlen_, "LockWaits:", sstrbuf);
	    sprintf(statsBuf_, "%-25s%10s", "Lock Waits", sstrbuf);
	    moveRowToUpQueue(statsBuf_);

	    getSubstrInfo(statsRow_, statsRowlen_, "Escalations:", sstrbuf);
	    sprintf(statsBuf_, "%-25s%10s", "Lock Escalations", sstrbuf);
	    moveRowToUpQueue(statsBuf_);
		
	
	    getSubstrInfo(statsRow_, statsRowlen_, "CpuTime:", sstrbuf);
	    sprintf(statsBuf_, "%-25s%10s", "SQL Process Busy Time", sstrbuf);
	    moveRowToUpQueue(statsBuf_);
	    
	    getSubstrInfo(statsRow_, statsRowlen_, "SpaceTotal:", sstrbuf);
	    sprintf(statsBuf_, "%-25s%10sKB", "SQL Space Allocated", sstrbuf);
	    moveRowToUpQueue(statsBuf_);
	    
	    getSubstrInfo(statsRow_, statsRowlen_, "SpaceUsed:", sstrbuf);
	    sprintf(statsBuf_, "%-25s%10sKB", "SQL Space Used", sstrbuf);
	    moveRowToUpQueue(statsBuf_);
	    
	    getSubstrInfo(statsRow_, statsRowlen_, "HeapTotal:", sstrbuf);
	    sprintf(statsBuf_, "%-25s%10sKB", "SQL Heap Allocated", sstrbuf);
	    moveRowToUpQueue(statsBuf_);
	    
	    getSubstrInfo(statsRow_, statsRowlen_, "HeapUsed:", sstrbuf);
	    sprintf(statsBuf_, "%-25s%10sKB", "SQL Heap Used", sstrbuf);
	    moveRowToUpQueue(statsBuf_);
	    
	    getSubstrInfo(statsRow_, statsRowlen_, "Opens:", sstrbuf);
	    sprintf(statsBuf_, "%-25s%10s", "Opens", sstrbuf);
	    moveRowToUpQueue(statsBuf_);
    
	    getSubstrInfo(statsRow_, statsRowlen_, "OpenTime:", sstrbuf);
	    sprintf(statsBuf_, "%-25s%10s", "Open Time", sstrbuf);
	    moveRowToUpQueue(statsBuf_);

	    getSubstrInfo(statsRow_, statsRowlen_, "Newprocess:", sstrbuf);
	    sprintf(statsBuf_, "%-25s%10s", "Processes Created", sstrbuf);
	    moveRowToUpQueue(statsBuf_);

	    getSubstrInfo(statsRow_, statsRowlen_, "NewprocessTime:", sstrbuf);
	    sprintf(statsBuf_, "%-25s%10s", "Process Create Time", sstrbuf);
	    moveRowToUpQueue(statsBuf_);

	    step_ = FETCH_STATS_ROW_;
	  }
	break;

	case FORMAT_AND_RETURN_ALL_STATS_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    str_cpy_all(statsBuf_, statsRow_, statsRowlen_);
	    moveRowToUpQueue(statsBuf_, statsRowlen_);

	    step_ = FETCH_STATS_ROW_;
	  }
	break;

	case HANDLE_ERROR_:
	  {
	    step_ = FETCH_EPILOGUE_;
	  }
	break;

	case FETCH_EPILOGUE_:
	  {
	    cliRC = cliInterface()->fetchRowsEpilogue(statsQuery_);

	    // restore the original shape before return.
	    cliRC = cliInterface()->
	      executeImmediate("control query shape restore;");

	    // reset stats collection to the original value
	    currContext->resetSqlParserFlags(0x00080000); //DISABLE_RUNTIME_STATS

	    step_ = DONE_;
	  }
	break;

	case DONE_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    // Return EOF.
	    ex_queue_entry * up_entry = qparent_.up->getTailEntry();
	    
	    up_entry->upState.parentIndex = 
	      pentry_down->downState.parentIndex;
	    
	    up_entry->upState.setMatchNo(0);
	    up_entry->upState.status = ex_queue::Q_NO_DATA;
	    
	    // insert into parent
	    qparent_.up->insert();
	    
	    //	    pstate.matches_ = 0;
	    qparent_.down->removeHead();
	    
	    return WORK_OK;
	  }
	break;

	}
    }
}

////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExExeUtilGetStatisticsTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilGetStatisticsPrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeUtilGetStatisticsPrivateState::ExExeUtilGetStatisticsPrivateState()
{
}

ExExeUtilGetStatisticsPrivateState::~ExExeUtilGetStatisticsPrivateState()
{
};

ExExeUtilGetRTSStatisticsTcb::ExExeUtilGetRTSStatisticsTcb(
     const ComTdbExeUtilGetStatistics & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilTcb( exe_util_tdb, NULL, glob)
{
   // Allocate the private state in each entry of the down queue
  qparent_.down->allocatePstate(this);

  step_ = INITIAL_;
   
  // buffer where output will be formatted
  statsBuf_ = new(glob->getDefaultHeap()) char[4096];
  sqlStatsDesc_ = NULL;
  currStatsDescEntry_ = -1;
  currStatsItemEntry_ = -1;
  masterStatsItems_ = NULL;
  measStatsItems_ = NULL;
  operatorStatsItems_ = NULL;
  rootOperStatsItems_ = NULL;
  partitionAccessStatsItems_ = NULL;
  pertableStatsItems_ = NULL;
  rmsStatsItems_ = NULL;
  bmoStatsItems_ = NULL;
  udrbaseStatsItems_ = NULL;
  replicateStatsItems_ = NULL;
  replicatorStatsItems_ = NULL;
  hbaseStatsItems_ = NULL;
  hiveStatsItems_ = NULL;
  isHeadingDisplayed_ = FALSE;
  isBMOHeadingDisplayed_ = FALSE;
  isUDRBaseHeadingDisplayed_ = FALSE;
  isHbaseHeadingDisplayed_ = FALSE;
  isHiveHeadingDisplayed_ = FALSE;
  maxMasterStatsItems_ = 0;
  maxMeasStatsItems_ = 0;
  maxOperatorStatsItems_ = 0;
  maxRootOperStatsItems_ = 0;
  maxPartitionAccessStatsItems_ = 0;
  maxPertableStatsItems_ = 0;
  maxRMSStatsItems_ = 0;
  maxBMOStatsItems_ = 0;
  maxUDRBaseStatsItems_ = 0;
  maxReplicateStatsItems_ = 0;
  maxReplicatorStatsItems_ = 0;
  maxHbaseStatsItems_ = 0;
  maxHiveStatsItems_ = 0;
  singleLineFormat_ = ((ComTdbExeUtilGetStatistics &)exe_util_tdb).singleLineFormat();
}

ExExeUtilGetRTSStatisticsTcb::~ExExeUtilGetRTSStatisticsTcb()
{
  NADELETEBASIC(statsBuf_, getGlobals()->getDefaultHeap());
  if (sqlStatsDesc_ != NULL)
    NADELETEBASIC(sqlStatsDesc_, getGlobals()->getDefaultHeap());
  if (masterStatsItems_ != NULL)
  {
    deleteSqlStatItems(masterStatsItems_, maxMasterStatsItems_);
    masterStatsItems_ = NULL;
  }
  if (measStatsItems_ != NULL)
  {
    deleteSqlStatItems(measStatsItems_, maxMeasStatsItems_);
    measStatsItems_ = NULL;
  }
  if (operatorStatsItems_ != NULL)
  {
    deleteSqlStatItems(operatorStatsItems_, maxOperatorStatsItems_);
    operatorStatsItems_ = NULL;
  }
  if (rootOperStatsItems_ != NULL)
  {
    deleteSqlStatItems(rootOperStatsItems_, maxRootOperStatsItems_);
    rootOperStatsItems_ = NULL;
  }
  if (partitionAccessStatsItems_ != NULL)
  {
    deleteSqlStatItems(partitionAccessStatsItems_, maxPartitionAccessStatsItems_);
    partitionAccessStatsItems_ = NULL;
  }
  if (pertableStatsItems_ != NULL)
  {
    deleteSqlStatItems(pertableStatsItems_, maxPertableStatsItems_);
    pertableStatsItems_ = NULL;
  }
  if (rmsStatsItems_ != NULL)
  {
    deleteSqlStatItems(rmsStatsItems_, maxRMSStatsItems_);
    rmsStatsItems_ = NULL;
  }
  if (bmoStatsItems_ != NULL)
  {
    deleteSqlStatItems(bmoStatsItems_, maxBMOStatsItems_);
    bmoStatsItems_ = NULL;
  }
  if (udrbaseStatsItems_ != NULL)
  {
    deleteSqlStatItems(udrbaseStatsItems_, maxUDRBaseStatsItems_);
    udrbaseStatsItems_ = NULL;
  }
  if (replicateStatsItems_ != NULL)
  {
    deleteSqlStatItems(replicateStatsItems_, maxReplicateStatsItems_);
    replicateStatsItems_ = NULL;
  }
  if (replicatorStatsItems_ != NULL)
  {
    deleteSqlStatItems(replicatorStatsItems_, maxReplicatorStatsItems_);
    replicatorStatsItems_ = NULL;
  }
  if (hbaseStatsItems_ != NULL)
  {
    deleteSqlStatItems(hbaseStatsItems_, maxHbaseStatsItems_);
    hbaseStatsItems_ = NULL;
  }
  if (hiveStatsItems_ != NULL)
  {
    deleteSqlStatItems(hiveStatsItems_, maxHiveStatsItems_);
    hiveStatsItems_ = NULL;
  }
}

void ExExeUtilGetRTSStatisticsTcb::formatInt64(SQLSTATS_ITEM stat, char* targetString)
{
  Int64 value = stat.int64_value;
  sprintf(targetString, "%ld", value);
  if (value >= 1000)
  {
    Lng32 intSize = str_len(targetString);
    AddCommas(targetString,intSize);
  }
}


void ExExeUtilGetRTSStatisticsTcb::formatWInt64(SQLSTATS_ITEM stat, char* targetString)
{
  if (stat.error_code)
    strcpy(targetString,"");   
  else
  {
    Int64 value = stat.int64_value;
    sprintf(targetString, "%ld", value);
    Lng32 intSize = str_len(targetString);
    AddCommas(targetString,intSize);
  }
}


void ExExeUtilGetRTSStatisticsTcb::formatOperStatItems(SQLSTATS_ITEM operStatsItems[])
{
  operStatsItems[0].statsItem_id = SQLSTATS_TDB_ID;
  operStatsItems[1].statsItem_id = SQLSTATS_LEFT_CHILD;
  operStatsItems[2].statsItem_id = SQLSTATS_RIGHT_CHILD;
  operStatsItems[3].statsItem_id = SQLSTATS_PARENT_TDB_ID;
  operStatsItems[4].statsItem_id = SQLSTATS_EXPLAIN_NODE_ID;
  operStatsItems[5].statsItem_id = SQLSTATS_FRAG_NUM;
  operStatsItems[6].statsItem_id = SQLSTATS_TDB_NAME;
  operStatsItems[6].str_value    = new (getGlobals()->getDefaultHeap())
                                                char[MAX_TDB_NAME_LEN+1];
  operStatsItems[6].str_max_len  = MAX_TDB_NAME_LEN;
  operStatsItems[7].statsItem_id = SQLSTATS_NUM_CALLS;  
  operStatsItems[8].statsItem_id = SQLSTATS_OPER_CPU_TIME;                  
  operStatsItems[9].statsItem_id = SQLSTATS_EST_ROWS_USED;
  operStatsItems[10].statsItem_id = SQLSTATS_ACT_ROWS_USED;
  operStatsItems[11].statsItem_id = SQLSTATS_DETAIL;
  operStatsItems[11].str_value    = new (getGlobals()->getDefaultHeap())
                                              char[1001];
  operStatsItems[11].str_max_len = 1000;
  operStatsItems[12].statsItem_id = SQLSTATS_DOP;
}
                
// operStatsItems must be in the following order for formatOperStats()
// Tdb Id, LeftChild, RightChild, Explain Node Id, TdbName,  NumCalls, SQL CPU Busy Time,
// Est Rows Accessed, Est Rows Used, Act Rows Used,
// SQL Space Alloc, SQL Space Used, SQL Heap Alloc, SQL Heap used

void ExExeUtilGetRTSStatisticsTcb::formatOperStats(SQLSTATS_ITEM* operStatsItems)
{
  char valString[25];
  Lng32 intSize=0;
  Lng32 valSize=0;
  if(!isHeadingDisplayed_ )
  {
    moveRowToUpQueue("");
    isHeadingDisplayed_ = TRUE;
    sprintf(statsBuf_, "%5s%5s%5s%5s%5s%5s %-25s%5s%13s%19s%19s%19s %s", 
      "LC","RC","Id","PaId", "ExId","Frag","TDBName","DOP", "Dispatches","OperCPUTime","EstRowsUsed", 
      "ActRowsUsed","Details");
    moveRowToUpQueue(statsBuf_);
    moveRowToUpQueue("");
  } 

  // Left Child TdbId
  if(operStatsItems[1].int64_value > 0)
    formatInt64(operStatsItems[1], valString);
  else
    strcpy(valString, "."); 
  sprintf(statsBuf_, "%5s", valString);        
 
  // Right Child TdbId
  if(operStatsItems[2].int64_value > 0)
    formatInt64(operStatsItems[2], valString);
  else
    strcpy(valString, ".");       
  sprintf(&statsBuf_[strlen(statsBuf_)], "%5s", valString);      

  // Id
  formatInt64(operStatsItems[0], valString);
  sprintf(&statsBuf_[strlen(statsBuf_)], "%5s", valString);        

  // Parent Tdb Id
  if(operStatsItems[3].int64_value > 0)
    formatInt64(operStatsItems[3], valString);
  else
    strcpy(valString, ".");
  sprintf(&statsBuf_[strlen(statsBuf_)], "%5s", valString);      

  // Explain Node Id
  formatInt64(operStatsItems[4], valString);
  sprintf(&statsBuf_[strlen(statsBuf_)], "%5s", valString);
  // Frag Num 
  formatInt64(operStatsItems[5], valString);
  sprintf(&statsBuf_[strlen(statsBuf_)], "%5s", valString);
   
  // Tdb Name
  if (operStatsItems[6].error_code)
    sprintf(&statsBuf_[strlen(statsBuf_)], " %-25s", "");   
  else
  {
    operStatsItems[6].str_value[operStatsItems[6].str_ret_len] = '\0'; 
    sprintf(&statsBuf_[strlen(statsBuf_)], " %-25s", operStatsItems[6].str_value);
  }

  // dop 
  formatInt64(operStatsItems[12], valString);
  sprintf(&statsBuf_[strlen(statsBuf_)], "%5s", valString);
  
  // Number of Work Calls
  formatInt64(operStatsItems[7], valString);
  sprintf(&statsBuf_[strlen(statsBuf_)], "%13s", valString);
  
  // CPU Time
  formatInt64(operStatsItems[8], valString);
  sprintf(&statsBuf_[strlen(statsBuf_)], "%19s", valString);
  
  // Estimated Rows Used
  sprintf(valString, "%.6g", operStatsItems[9].double_value);
  sprintf(&statsBuf_[strlen(statsBuf_)], "%19s", valString); 
  
  // Actual Rows Used 
  formatInt64(operStatsItems[10], valString);
  sprintf(&statsBuf_[strlen(statsBuf_)], "%19s", valString);

  // Detail
  if (operStatsItems[11].error_code)
    sprintf(&statsBuf_[strlen(statsBuf_)], "%s", "");   
  else
  {
    operStatsItems[11].str_value[operStatsItems[11].str_ret_len] = '\0'; 
    sprintf(&statsBuf_[strlen(statsBuf_)], " %s", operStatsItems[11].str_value);
  }
}


void ExExeUtilGetRTSStatisticsTcb::deleteSqlStatItems(SQLSTATS_ITEM *sqlStatsItem, 
                              ULng32  noOfStatsItem)
{
  for (ULng32 i= 0; i < noOfStatsItem; i++)
  {
    if (sqlStatsItem[i].str_value != NULL)
      NADELETEBASIC(sqlStatsItem[i].str_value, getGlobals()->getDefaultHeap());
  }
  NADELETEBASIC(sqlStatsItem, getGlobals()->getDefaultHeap()); 
}

void ExExeUtilGetRTSStatisticsTcb::initSqlStatsItems(SQLSTATS_ITEM *sqlStatsItem,
                                  ULng32  noOfStatsItem, NABoolean initTdbIdOnly)
{
  for (ULng32 i = 0; i < noOfStatsItem; i++)
  {
    if (initTdbIdOnly)
    {
      sqlStatsItem[i].tdb_id = sqlStatsDesc_[currStatsDescEntry_].tdb_id;
    }
    else
    {
      sqlStatsItem[i].stats_type = sqlStatsDesc_[currStatsDescEntry_].stats_type;
      sqlStatsItem[i].tdb_id = sqlStatsDesc_[currStatsDescEntry_].tdb_id;
      sqlStatsItem[i].str_value = NULL;
    }
  }
}

//////////////////////////////////////////////////////
// work() for ExExeUtilGetRTSStatisticsTcb
//////////////////////////////////////////////////////
short ExExeUtilGetRTSStatisticsTcb::work()
{
  //  short rc = 0;
  Lng32 cliRC = 0;
  ULng32 sec;
  ULng32 usec;
  ULng32 min;
  ULng32 hour;
  ULng32 i;
  short rc;
  Int64 jtime;
  
  short stmtState;
  short queryType;
  short timestamp[8];
  short subqueryType;

  char formattedFloatVal[25];
  char Int64Val[50];
  Lng32 intSize = 0;
  Lng32 valSize = 0;
  char timestampVal[50];
  Lng32 microSecs;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;
  
  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;
  
  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ContextCli * currContext = masterGlob->getStatement()->getContext();
  SessionDefaults *sd = currContext->getSessionDefaults();
  short statsMergeType = 0;

  while (1)
  {
    switch (step_)
    {
    case INITIAL_:
      {
        switch (getStatsTdb().statsMergeType_)
        {
        case SQLCLI_ACCUMULATED_STATS:
          sqlStatsDesc_ = new (getGlobals()->getDefaultHeap()) SQLSTATS_DESC[MAX_ACCUMULATED_STATS_DESC];
          maxStatsDescEntries_ = MAX_ACCUMULATED_STATS_DESC;
          break;
        case SQLCLI_PERTABLE_STATS:
          sqlStatsDesc_ = new (getGlobals()->getDefaultHeap()) SQLSTATS_DESC[MAX_PERTABLE_STATS_DESC];
          maxStatsDescEntries_ = MAX_PERTABLE_STATS_DESC;
          break;
        case SQLCLI_PROGRESS_STATS:
          sqlStatsDesc_ = new (getGlobals()->getDefaultHeap()) SQLSTATS_DESC[MAX_PROGRESS_STATS_DESC];
          maxStatsDescEntries_ = MAX_PROGRESS_STATS_DESC;
          break;
        case SQLCLI_OPERATOR_STATS:
          sqlStatsDesc_ = new (getGlobals()->getDefaultHeap()) SQLSTATS_DESC[MAX_OPERATOR_STATS_DESC];
          maxStatsDescEntries_ = MAX_OPERATOR_STATS_DESC;
          break;
        case SQLCLI_RMS_INFO_STATS:
          sqlStatsDesc_ = new (getGlobals()->getDefaultHeap()) SQLSTATS_DESC[MAX_RMS_STATS_DESC];
          maxStatsDescEntries_ = MAX_RMS_STATS_DESC;
          break;
        default:
          sqlStatsDesc_ = new (getGlobals()->getDefaultHeap()) SQLSTATS_DESC[MAX_OPERATOR_STATS_DESC];
          maxStatsDescEntries_ = MAX_OPERATOR_STATS_DESC;
          break;
        }
        Lng32 diagMarkValue = currContext ->getDiagsArea() -> mark();
        cliRC = SQL_EXEC_GetStatistics2(getStatsTdb().statsReqType_,
                getStatsTdb().stmtName_,
                getStatsTdb().stmtName_ ? str_len(getStatsTdb().stmtName_) : 0,
                getStatsTdb().activeQueryNum_,
                getStatsTdb().statsMergeType_,
                &statsCollectType_,
                sqlStatsDesc_,
                maxStatsDescEntries_,
                &retStatsDescEntries_);
        
        // if not enough stats desc array, make a larger one
        if( cliRC == -CLI_INSUFFICIENT_STATS_DESC)
        {
          currContext ->getDiagsArea() -> rewind(diagMarkValue);
          step_ = EXPAND_STATS_ARRAY_;        
        }
        else
        if (cliRC < 0)
        {
          step_ = HANDLE_ERROR_;
        }
        else
        {
          currStatsDescEntry_ = -1;
          currStatsItemEntry_ = -1;
          isHeadingDisplayed_ = FALSE;
          isBMOHeadingDisplayed_ = FALSE;
          isUDRBaseHeadingDisplayed_ = FALSE;
          if (getStatsTdb().statsMergeType_ == SQLCLI_DEFAULT_STATS)
          {
            if (sd)
              statsMergeType = (short)sd->getStatisticsViewType();
            else
              statsMergeType = statsCollectType_;
          }
          else
            statsMergeType = getStatsTdb().statsMergeType_;
          if (statsMergeType == SQLCLI_SAME_STATS)
            statsMergeType = statsCollectType_;
          step_ = GET_NEXT_STATS_DESC_ENTRY_;
        }
      }
      break;
    case EXPAND_STATS_ARRAY_:
      {
         NADELETEBASIC(sqlStatsDesc_, getGlobals()->getDefaultHeap());
         maxStatsDescEntries_ = retStatsDescEntries_;
         sqlStatsDesc_ = new (getGlobals()->getDefaultHeap()) SQLSTATS_DESC[maxStatsDescEntries_];
         cliRC = SQL_EXEC_GetStatistics2(getStatsTdb().statsReqType_,
                getStatsTdb().stmtName_,
                getStatsTdb().stmtName_ ? str_len(getStatsTdb().stmtName_) : 0,
                getStatsTdb().activeQueryNum_,
                getStatsTdb().statsMergeType_,
                &statsCollectType_,
                sqlStatsDesc_,
                maxStatsDescEntries_,
                &retStatsDescEntries_);
        
        if (cliRC < 0)
        {
          step_ = HANDLE_ERROR_;
        }
        else
        {
          currStatsDescEntry_ = -1;
          currStatsItemEntry_ = -1;
          isHeadingDisplayed_ = FALSE;
          isBMOHeadingDisplayed_ = FALSE;
          isUDRBaseHeadingDisplayed_ = FALSE;
          if (getStatsTdb().statsMergeType_ == SQLCLI_DEFAULT_STATS)
          {
            if (sd)
              statsMergeType = (short)sd->getStatisticsViewType();
            else
              statsMergeType = statsCollectType_;
          }
          else
            statsMergeType = getStatsTdb().statsMergeType_;
          if (statsMergeType == SQLCLI_SAME_STATS)
            statsMergeType = statsCollectType_;
          step_ = GET_NEXT_STATS_DESC_ENTRY_;
        }
      }
      break;
    case GET_NEXT_STATS_DESC_ENTRY_:
      {
        currStatsDescEntry_++;
        currStatsItemEntry_ = 0;
        if (currStatsDescEntry_ >= retStatsDescEntries_)
          step_ = DONE_;
        else
        {
          if (statsMergeType == SQLCLI_OPERATOR_STATS)
          {
            if (sqlStatsDesc_[currStatsDescEntry_].stats_type == SQLSTATS_DESC_MASTER_STATS)
              step_ = GET_MASTER_STATS_ENTRY_;
            else
              step_ = GET_OPER_STATS_ENTRY_;
          }
          else
          {
            switch (sqlStatsDesc_[currStatsDescEntry_].stats_type)
            {
            case SQLSTATS_DESC_MASTER_STATS:
              step_ = GET_MASTER_STATS_ENTRY_;
              break;
            case SQLSTATS_DESC_ROOT_OPER_STATS:
              step_ = GET_ROOTOPER_STATS_ENTRY_;
              break;
            case SQLSTATS_DESC_BMO_STATS:
              step_ = GET_BMO_STATS_ENTRY_;
              break;
            case SQLSTATS_DESC_REPLICATE_STATS:
              step_ = GET_REPLICATE_STATS_ENTRY_;
              break;
            case SQLSTATS_DESC_REPLICATOR_STATS:
              step_ = GET_REPLICATOR_STATS_ENTRY_;
              break;
            case SQLSTATS_DESC_MEAS_STATS:
              step_ = GET_MEAS_STATS_ENTRY_;
              break;
            case SQLSTATS_DESC_RMS_STATS:
              step_ = GET_RMS_STATS_ENTRY_;
              break;
            case SQLSTATS_DESC_UDR_BASE_STATS:
              step_ = GET_UDR_BASE_STATS_ENTRY_;
              break;
            case SQLSTATS_DESC_SE_STATS:
              step_ = GET_HBASE_STATS_ENTRY_;
              break;
            default:
              step_ = GET_NEXT_STATS_DESC_ENTRY_;
              break;
            }
          }
        }
      }
      break;
    case GET_MASTER_STATS_ENTRY_:
      {
        if (masterStatsItems_ == NULL)
        {
          maxMasterStatsItems_ = 36;
          masterStatsItems_ = new (getGlobals()->getDefaultHeap()) 
                  SQLSTATS_ITEM[maxMasterStatsItems_];
          initSqlStatsItems(masterStatsItems_, maxMasterStatsItems_, FALSE);
          masterStatsItems_[0].statsItem_id = SQLSTATS_QUERY_ID;
          masterStatsItems_[1].statsItem_id = SQLSTATS_COMP_START_TIME;
          masterStatsItems_[2].statsItem_id = SQLSTATS_COMP_END_TIME;
          masterStatsItems_[3].statsItem_id = SQLSTATS_COMP_TIME;
          masterStatsItems_[4].statsItem_id = SQLSTATS_EXECUTE_START_TIME;
          masterStatsItems_[5].statsItem_id = SQLSTATS_EXECUTE_END_TIME;
          masterStatsItems_[6].statsItem_id = SQLSTATS_EXECUTE_TIME;
          masterStatsItems_[7].statsItem_id = SQLSTATS_STMT_STATE;
          masterStatsItems_[8].statsItem_id = SQLSTATS_ROWS_AFFECTED;
          masterStatsItems_[9].statsItem_id = SQLSTATS_SQL_ERROR_CODE;
          masterStatsItems_[10].statsItem_id = SQLSTATS_STATS_ERROR_CODE;
          masterStatsItems_[11].statsItem_id = SQLSTATS_QUERY_TYPE;
          masterStatsItems_[12].statsItem_id = SQLSTATS_SUBQUERY_TYPE;
          masterStatsItems_[13].statsItem_id = SQLSTATS_EST_ROWS_ACCESSED;
          masterStatsItems_[14].statsItem_id = SQLSTATS_EST_ROWS_USED;
          masterStatsItems_[15].statsItem_id = SQLSTATS_PARENT_QUERY_ID;   
          masterStatsItems_[16].statsItem_id = SQLSTATS_PARENT_QUERY_SYSTEM;   
          masterStatsItems_[17].statsItem_id = SQLSTATS_CHILD_QUERY_ID;
          masterStatsItems_[18].statsItem_id = SQLSTATS_NUM_SQLPROCS;
          masterStatsItems_[19].statsItem_id = SQLSTATS_NUM_CPUS;
          masterStatsItems_[20].statsItem_id  = SQLSTATS_MASTER_PRIORITY;
          masterStatsItems_[21].statsItem_id  = SQLSTATS_TRANSID;
          masterStatsItems_[22].statsItem_id = SQLSTATS_SOURCE_STR;
          masterStatsItems_[23].statsItem_id = SQLSTATS_SOURCE_STR_LEN;
          masterStatsItems_[24].statsItem_id = SQLSTATS_ROWS_RETURNED;
          masterStatsItems_[25].statsItem_id = SQLSTATS_FIRST_ROW_RET_TIME;
          masterStatsItems_[26].statsItem_id = SQLSTATS_AQR_LAST_ERROR;
          masterStatsItems_[27].statsItem_id = SQLSTATS_AQR_NUM_RETRIES;
          masterStatsItems_[28].statsItem_id = SQLSTATS_AQR_DELAY_BEFORE_RETRY;
          masterStatsItems_[29].statsItem_id = SQLSTATS_RECLAIM_SPACE_COUNT;
          masterStatsItems_[30].statsItem_id = SQLSTATS_CANCEL_TIME_ID;
          masterStatsItems_[31].statsItem_id = SQLSTATS_SUSPEND_TIME_ID;
          masterStatsItems_[32].statsItem_id = SQLSTATS_EXECUTE_COUNT;
          masterStatsItems_[33].statsItem_id = SQLSTATS_EXECUTE_TIME_MIN;
          masterStatsItems_[34].statsItem_id = SQLSTATS_EXECUTE_TIME_MAX;
          masterStatsItems_[35].statsItem_id = SQLSTATS_EXECUTE_TIME_AVG;
          // maxMasterStatsItems_ is set to 36
          masterStatsItems_[0].str_value = new (getGlobals()->getDefaultHeap())
                          char[ComSqlId::MAX_QUERY_ID_LEN+1];
          masterStatsItems_[0].str_max_len = ComSqlId::MAX_QUERY_ID_LEN;
          // Parent Qid
          masterStatsItems_[15].str_value = new (getGlobals()->getDefaultHeap())
                          char[ComSqlId::MAX_QUERY_ID_LEN+1];
          masterStatsItems_[15].str_max_len = ComSqlId::MAX_QUERY_ID_LEN;
          // Parent Qid System
          masterStatsItems_[16].str_value = new (getGlobals()->getDefaultHeap())
                          char[24];
          masterStatsItems_[16].str_max_len = 23;
          // Child Qid
          masterStatsItems_[17].str_value = new (getGlobals()->getDefaultHeap())
                          char[ComSqlId::MAX_QUERY_ID_LEN+1];
          masterStatsItems_[17].str_max_len = ComSqlId::MAX_QUERY_ID_LEN;
          // Source Str
          masterStatsItems_[22].str_value = new (getGlobals()->getDefaultHeap())
                          char[RMS_STORE_SQL_SOURCE_LEN+2];
          masterStatsItems_[22].str_max_len = RMS_STORE_SQL_SOURCE_LEN;
        }
        else
          initSqlStatsItems(masterStatsItems_, maxMasterStatsItems_, TRUE);
        cliRC = SQL_EXEC_GetStatisticsItems(getStatsTdb().statsReqType_,
                getStatsTdb().stmtName_,
                getStatsTdb().stmtName_ ? str_len(getStatsTdb().stmtName_) : 0,
                maxMasterStatsItems_,
                masterStatsItems_);
        if (cliRC < 0)
        {
          step_ = HANDLE_ERROR_;
        }
        else
          step_ = FORMAT_AND_RETURN_MASTER_STATS_;
      }
      break;
    case FORMAT_AND_RETURN_MASTER_STATS_:
      {
        for (; currStatsItemEntry_ < maxMasterStatsItems_; currStatsItemEntry_++)
        {
          i = (short)currStatsItemEntry_;
          statsBuf_[0] = '\0';
          if (masterStatsItems_[i].error_code != 0)
            continue;
          switch (masterStatsItems_[i].statsItem_id)
          {
          case SQLSTATS_QUERY_ID:
            masterStatsItems_[i].str_value[masterStatsItems_[i].str_ret_len] = '\0';
            sprintf(statsBuf_, "%-25s%s", "Qid", masterStatsItems_[i].str_value);
            break;
          case SQLSTATS_SOURCE_STR:
            masterStatsItems_[i].str_value[masterStatsItems_[i].str_ret_len] = '\0';
            masterStatsItems_[i].str_value[masterStatsItems_[i].str_ret_len+1] = '\0';
            sprintf(statsBuf_, "%-25s%s", "Source String ", masterStatsItems_[i].str_value);
            break;
          case SQLSTATS_SOURCE_STR_LEN:
            sprintf(Int64Val, "%ld", masterStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "SQL Source Length", Int64Val);

            break;
          case SQLSTATS_COMP_START_TIME:
	    jtime = CONVERTTIMESTAMP(masterStatsItems_[i].int64_value,0,-1,NULL);
	    INTERPRETTIMESTAMP(jtime, timestamp);
	    sprintf(statsBuf_, "%-25s%04d/%02d/%02d %02d:%02d:%02d.%03u%03u",
                        "Compile Start Time",
			timestamp[0], timestamp[1], timestamp[2],
			timestamp[3], timestamp[4], timestamp[5],
			timestamp[6], timestamp[7]);
	    break;
          case SQLSTATS_COMP_END_TIME:
            if (masterStatsItems_[i].int64_value != -1)
            {
	      jtime = CONVERTTIMESTAMP(masterStatsItems_[i].int64_value,0,-1,NULL);
	      INTERPRETTIMESTAMP(jtime, timestamp);
	      sprintf(statsBuf_, "%-25s%04d/%02d/%02d %02d:%02d:%02d.%03u%03u",
                          "Compile End Time",
			  timestamp[0], timestamp[1], timestamp[2],
			  timestamp[3], timestamp[4], timestamp[5],
			  timestamp[6], timestamp[7]);
            }
            else
              sprintf(statsBuf_, "%-25s%s", "Compile End Time", "-1");
	    break;
          case SQLSTATS_COMP_TIME:
            sec = (ULng32) (masterStatsItems_[i].int64_value / 1000000);
            usec = (ULng32) (masterStatsItems_[i].int64_value % 1000000);
            min = sec/60;
            sec = sec % 60;
            hour = min/60;
            min = min % 60;

            sprintf (statsBuf_,  "%-34s%4u:%02u:%02u.%06u", 
                          "Compile Elapsed Time",hour, min, sec,usec);   
            break;
          case SQLSTATS_EXECUTE_START_TIME:
            if (masterStatsItems_[i].int64_value != -1)
            {
	      jtime = CONVERTTIMESTAMP(masterStatsItems_[i].int64_value,0,-1,NULL);
	      INTERPRETTIMESTAMP(jtime, timestamp);
	      sprintf(statsBuf_, "%-25s%04d/%02d/%02d %02d:%02d:%02d.%03u%03u",
                          "Execute Start Time",
			  timestamp[0], timestamp[1], timestamp[2],
			  timestamp[3], timestamp[4], timestamp[5],
			  timestamp[6], timestamp[7]);
            }
            else
              sprintf(statsBuf_, "%-25s%s", "Execute Start Time", "-1");
	    break;
          case SQLSTATS_FIRST_ROW_RET_TIME:
            if (masterStatsItems_[i].int64_value != -1)
            {
	      jtime =  CONVERTTIMESTAMP(masterStatsItems_[i].int64_value,0,-1,NULL);
    	    
              INTERPRETTIMESTAMP(jtime, timestamp);
	      sprintf(statsBuf_, "%-25s%04d/%02d/%02d %02d:%02d:%02d.%03u%03u",
                          "First Row Returned Time",
			  timestamp[0], timestamp[1], timestamp[2],
			  timestamp[3], timestamp[4], timestamp[5],
			  timestamp[6], timestamp[7]);
            }
            else
            {
              sprintf(statsBuf_, "%-25s%s",
                            "First Row Returned Time", "-1");
            }
            break;
         case SQLSTATS_CANCEL_TIME_ID:
            if (masterStatsItems_[i].int64_value != -1)
            {
	      jtime = CONVERTTIMESTAMP(masterStatsItems_[i].int64_value,0,-1,NULL);
	      INTERPRETTIMESTAMP(jtime, timestamp);
	      sprintf(statsBuf_, "%-25s%04d/%02d/%02d %02d:%02d:%02d.%03u%03u",
                          "Cancel Time",
			  timestamp[0], timestamp[1], timestamp[2],
			  timestamp[3], timestamp[4], timestamp[5],
			  timestamp[6], timestamp[7]);
            }
            else
              sprintf(statsBuf_, "%-25s%s", "Cancel Time", "-1");
            break;
         case SQLSTATS_EXECUTE_END_TIME:
            if (masterStatsItems_[i].int64_value != -1)
            {
	      jtime = CONVERTTIMESTAMP(masterStatsItems_[i].int64_value,0,-1,NULL);
	      INTERPRETTIMESTAMP(jtime, timestamp);
	      sprintf(statsBuf_, "%-25s%04d/%02d/%02d %02d:%02d:%02d.%03u%03u",
                          "Execute End Time",
			  timestamp[0], timestamp[1], timestamp[2],
			  timestamp[3], timestamp[4], timestamp[5],
			  timestamp[6], timestamp[7]);
            }
            else
              sprintf(statsBuf_, "%-25s%s", "Execute End Time", "-1");
	    break;
          case SQLSTATS_EXECUTE_TIME:
            sec = (ULng32) (masterStatsItems_[i].int64_value / 1000000);
            usec = (ULng32) (masterStatsItems_[i].int64_value % 1000000);
            min = sec/60;
            sec = sec % 60;
            hour = min/60;
            min = min % 60;

            sprintf (statsBuf_,  "%-34s%4u:%02u:%02u.%06u", "Execute Elapsed Time",
  	                  hour, min, sec,usec);   
            break;
          case SQLSTATS_FIXUP_TIME:
            sec = (ULng32) (masterStatsItems_[i].int64_value / 1000000);
            usec = (ULng32) (masterStatsItems_[i].int64_value % 1000000);
            min = sec/60;
            sec = sec % 60;
            hour = min/60;
            min = min % 60;
            sprintf (statsBuf_,  "%34s%4u:%02u:%02u.%06u", "Fixup Elapsed Time",
  	                  hour, min, sec,usec);   
            break;
          case SQLSTATS_STMT_STATE:
            stmtState  = (short)masterStatsItems_[i].int64_value;
            if (stmtState >= SQLSTMT_STATE_UNKNOWN)
              stmtState = SQLSTMT_STATE_UNKNOWN;
            sprintf(statsBuf_, "%-25s%s", "State", 
                  Statement::stmtState((Statement::State)stmtState));
            break;
          case SQLSTATS_ROWS_AFFECTED:
            sprintf(Int64Val, "%ld", masterStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "Rows Affected", Int64Val);
            break;
          case SQLSTATS_ROWS_RETURNED:
            sprintf(Int64Val, "%ld", masterStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "Rows Returned", Int64Val);
            break;
          case SQLSTATS_SQL_ERROR_CODE:
            sprintf(statsBuf_, "%-25s%-d", "SQL Error Code", 
                      (Lng32)masterStatsItems_[i].int64_value);
            break;
          case SQLSTATS_STATS_ERROR_CODE:
            sprintf(statsBuf_, "%-25s%-d", "Stats Error Code", 
                      (Lng32)masterStatsItems_[i].int64_value);
            break;
          case SQLSTATS_QUERY_TYPE:
            queryType = (short)masterStatsItems_[i].int64_value;
            sprintf(statsBuf_, "%-25s%s", "Query Type",
                    ComTdbRoot::getQueryTypeText(queryType));
            break;
          case SQLSTATS_SUBQUERY_TYPE:
            subqueryType = (short)masterStatsItems_[i].int64_value;
            sprintf(statsBuf_, "%-25s%s", "Sub Query Type",
                    ComTdbRoot::getSubqueryTypeText(subqueryType));
            break;
          case SQLSTATS_EST_ROWS_ACCESSED:
            sprintf(statsBuf_, "%-25s%.6g", "Estimated Accessed Rows", masterStatsItems_[i].double_value);
            break;
          case SQLSTATS_EST_ROWS_USED:
            sprintf(statsBuf_, "%-25s%.6g", "Estimated Used Rows", masterStatsItems_[i].double_value);
            break;
          case SQLSTATS_PARENT_QUERY_ID:
            masterStatsItems_[i].str_value[masterStatsItems_[i].str_ret_len] = '\0';
            sprintf(statsBuf_, "%-25s%s", "Parent Qid", masterStatsItems_[i].str_value);
            break;
          case SQLSTATS_PARENT_QUERY_SYSTEM:
            masterStatsItems_[i].str_value[masterStatsItems_[i].str_ret_len] = '\0';
            sprintf(statsBuf_, "%-25s%s", "Parent Query System", masterStatsItems_[i].str_value);
            break;
          case SQLSTATS_CHILD_QUERY_ID:
            masterStatsItems_[i].str_value[masterStatsItems_[i].str_ret_len] = '\0';
            sprintf(statsBuf_, "%-25s%s", "Child Qid", masterStatsItems_[i].str_value);
            break;
          case SQLSTATS_NUM_SQLPROCS:
            sprintf(Int64Val, "%ld", masterStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "Number of SQL Processes", Int64Val);
            break;
          case SQLSTATS_NUM_CPUS:
            sprintf(Int64Val, "%ld", masterStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "Number of Cpus", Int64Val);
            break;
          case SQLSTATS_TRANSID:
            sprintf(Int64Val, "%ld", masterStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            sprintf(statsBuf_, "%-25s%s", "Transaction Id", 
                              Int64Val);
            break;
          case SQLSTATS_SUSPEND_TIME_ID:
            if (masterStatsItems_[i].int64_value != -1)
            {
	      jtime = CONVERTTIMESTAMP(masterStatsItems_[i].int64_value,0,-1,NULL);
	      INTERPRETTIMESTAMP(jtime, timestamp);
	      sprintf(statsBuf_, "%-25s%04d/%02d/%02d %02d:%02d:%02d.%03u%03u",
                          "Last Suspend Time",
			  timestamp[0], timestamp[1], timestamp[2],
			  timestamp[3], timestamp[4], timestamp[5],
			  timestamp[6], timestamp[7]);
            }
            else
              sprintf(statsBuf_, "%-25s%s", "Last Suspend Time", "-1");
            break;
          case SQLSTATS_AQR_LAST_ERROR:
            sprintf(statsBuf_, "%-25s%-d", "Last Error before AQR", 
		 (Lng32)masterStatsItems_[i].int64_value);         
            break;
          case SQLSTATS_AQR_NUM_RETRIES:
            sprintf(Int64Val, "%ld", masterStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "Number of AQR retries", Int64Val);
            break;
          case SQLSTATS_AQR_DELAY_BEFORE_RETRY:
            sprintf(Int64Val, "%ld", masterStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "Delay before AQR", Int64Val);
            break;
          case SQLSTATS_RECLAIM_SPACE_COUNT:
            sprintf(statsBuf_, "%-25s%-d", "No. of times reclaimed", 
		 (Lng32)masterStatsItems_[i].int64_value);         
            break;
          case SQLSTATS_EXECUTE_COUNT:
            sprintf(Int64Val, "%ld", masterStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "No. of times executed", Int64Val);
            break;
          case SQLSTATS_EXECUTE_TIME_MIN:
            microSecs = masterStatsItems_[i].int64_value % 1000000L;
            sprintf(Int64Val, "%ld", masterStatsItems_[i].int64_value/1000000);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s.%06d secs", "Min. Execute Time", Int64Val,microSecs);
            break;
          case SQLSTATS_EXECUTE_TIME_MAX:
            microSecs = masterStatsItems_[i].int64_value % 1000000L;
            sprintf(Int64Val, "%ld", masterStatsItems_[i].int64_value/1000000);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s.%06d secs", "Max. Execute Time", Int64Val,microSecs);
            break;
          case SQLSTATS_EXECUTE_TIME_AVG:
            microSecs = masterStatsItems_[i].int64_value % 1000000L;
            sprintf(Int64Val, "%ld", masterStatsItems_[i].int64_value/1000000);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s.%06d secs", "Avg. Execute Time", Int64Val,microSecs);
            break;
          default:
            statsBuf_[0] = '\0';
            break;
          }
          if (strlen(statsBuf_) > 0)
            if (moveRowToUpQueue(statsBuf_, strlen(statsBuf_), &rc) == -1)
              return rc;
        }
        sprintf(statsBuf_, "%-25s%s", "Stats Collection Type", 
                    ExStatisticsArea::getStatsTypeText(statsCollectType_));
        if (moveRowToUpQueue(statsBuf_, strlen(statsBuf_), &rc) == -1)
          return rc;
        step_ = GET_NEXT_STATS_DESC_ENTRY_;
      }
      break;
    case GET_MEAS_STATS_ENTRY_:
      {
        if (measStatsItems_ == NULL)
        {
          maxMeasStatsItems_ = 30;
          measStatsItems_ = new (getGlobals()->getDefaultHeap()) 
                  SQLSTATS_ITEM[maxMeasStatsItems_];
          initSqlStatsItems(measStatsItems_, maxMeasStatsItems_, FALSE);
          measStatsItems_[0].statsItem_id = SQLSTATS_ACT_ROWS_ACCESSED;
          measStatsItems_[1].statsItem_id = SQLSTATS_ACT_ROWS_USED;
          measStatsItems_[2].statsItem_id = SQLSTATS_SE_IOS;
          measStatsItems_[3].statsItem_id = SQLSTATS_SE_IO_BYTES;
          measStatsItems_[4].statsItem_id = SQLSTATS_SE_IO_MAX_TIME;
          measStatsItems_[5].statsItem_id = SQLSTATS_SQL_CPU_BUSY_TIME;
          measStatsItems_[6].statsItem_id = SQLSTATS_UDR_CPU_BUSY_TIME;
          measStatsItems_[7].statsItem_id = SQLSTATS_SQL_SPACE_ALLOC;
          measStatsItems_[8].statsItem_id = SQLSTATS_SQL_SPACE_USED;
          measStatsItems_[9].statsItem_id = SQLSTATS_SQL_HEAP_ALLOC;
          measStatsItems_[10].statsItem_id = SQLSTATS_SQL_HEAP_USED;
          measStatsItems_[11].statsItem_id = SQLSTATS_SQL_HEAP_WM;
          measStatsItems_[12].statsItem_id = SQLSTATS_OPENS;
          measStatsItems_[13].statsItem_id = SQLSTATS_OPEN_TIME;
          measStatsItems_[14].statsItem_id = SQLSTATS_PROCESS_CREATED;
          measStatsItems_[15].statsItem_id = SQLSTATS_PROCESS_CREATE_TIME;
          measStatsItems_[16].statsItem_id = SQLSTATS_REQ_MSG_CNT;
          measStatsItems_[17].statsItem_id = SQLSTATS_REQ_MSG_BYTES;
          measStatsItems_[18].statsItem_id = SQLSTATS_REPLY_MSG_CNT;
          measStatsItems_[19].statsItem_id = SQLSTATS_REPLY_MSG_BYTES;
          measStatsItems_[20].statsItem_id = SQLSTATS_BMO_SPACE_BUFFER_SIZE;
          measStatsItems_[21].statsItem_id = SQLSTATS_BMO_SPACE_BUFFER_COUNT;
          measStatsItems_[22].statsItem_id = SQLSTATS_INTERIM_ROW_COUNT;
          measStatsItems_[23].statsItem_id = SQLSTATS_SCRATCH_OVERFLOW_MODE;
          measStatsItems_[24].statsItem_id = SQLSTATS_SCRATCH_FILE_COUNT;
          measStatsItems_[25].statsItem_id = SQLSTATS_SCRATCH_IO_SIZE;
          measStatsItems_[26].statsItem_id = SQLSTATS_SCRATCH_READ_COUNT;
          measStatsItems_[27].statsItem_id = SQLSTATS_SCRATCH_WRITE_COUNT;
          measStatsItems_[28].statsItem_id = SQLSTATS_SCRATCH_IO_MAX_TIME;
          measStatsItems_[29].statsItem_id = SQLSTATS_TOPN;
          // maxMeasStatsItems_ is set to  30
        }
        else
          initSqlStatsItems(measStatsItems_, maxMeasStatsItems_, TRUE);
        cliRC = SQL_EXEC_GetStatisticsItems(getStatsTdb().statsReqType_,
                getStatsTdb().stmtName_,
                getStatsTdb().stmtName_ ? str_len(getStatsTdb().stmtName_) : 0,
                maxMeasStatsItems_,
                measStatsItems_);
        if (cliRC < 0)
        {
          step_ = HANDLE_ERROR_;
        }
        else
          step_ = FORMAT_AND_RETURN_MEAS_STATS_;
      }
      break;
    case FORMAT_AND_RETURN_MEAS_STATS_:
      {
        for (; currStatsItemEntry_ < maxMeasStatsItems_; currStatsItemEntry_++)
        {
          i = (short)currStatsItemEntry_;
          if (measStatsItems_[i].error_code != 0)
            continue;
          switch (measStatsItems_[i].statsItem_id)
          {
          case SQLSTATS_ACT_ROWS_ACCESSED:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "Accessed Rows", Int64Val);
            break;          
          case SQLSTATS_ACT_ROWS_USED:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "Used Rows", Int64Val);
            break;
          case SQLSTATS_SE_IOS:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "SE IOs", Int64Val);
            break;
          case SQLSTATS_SE_IO_BYTES:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "SE IO Bytes", Int64Val);
            break;
          case SQLSTATS_SE_IO_MAX_TIME:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "SE IO MAX Time", Int64Val);
            break;
          case SQLSTATS_SQL_CPU_BUSY_TIME:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "SQL Process Busy Time", Int64Val);
            break;
          case SQLSTATS_UDR_CPU_BUSY_TIME:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "UDR Process Busy Time", Int64Val);
            break;
          case SQLSTATS_SQL_SPACE_ALLOC:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s KB", "SQL Space Allocated", Int64Val); 
            break;
          case SQLSTATS_SQL_SPACE_USED:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s KB", "SQL Space Used", Int64Val); 
            break;
          case SQLSTATS_SQL_HEAP_ALLOC:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s KB", "SQL Heap Allocated", Int64Val);
            break;
          case SQLSTATS_SQL_HEAP_USED:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s KB", "SQL Heap Used", Int64Val);
            break;
          case SQLSTATS_SQL_HEAP_WM:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s KB", "SQL Heap WM", Int64Val);
            break;
          case SQLSTATS_OPENS:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "Opens", Int64Val);
            break;
          case SQLSTATS_OPEN_TIME:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "Open Time", Int64Val);
            break;
          case SQLSTATS_PROCESS_CREATED:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "Processes Created", Int64Val);
            break;
          case SQLSTATS_PROCESS_CREATE_TIME:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "Process Create Time", Int64Val);
            break;
          case SQLSTATS_REQ_MSG_CNT:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "Request Message Count", Int64Val);
            break;
          case SQLSTATS_REQ_MSG_BYTES:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "Request Message Bytes", Int64Val);
            break;
          case SQLSTATS_REPLY_MSG_CNT:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "Reply Message Count", Int64Val);
            break;
          case SQLSTATS_REPLY_MSG_BYTES:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "Reply Message Bytes", Int64Val);
            break;
          case SQLSTATS_SCRATCH_OVERFLOW_MODE:
            sprintf(statsBuf_, "%-25s%s", "Scr. Overflow Mode", 
              ExBMOStats::getScratchOverflowMode((Int16)measStatsItems_[i].int64_value));
            break;
          case SQLSTATS_TOPN:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "Sort TopN", Int64Val);
            break;
          case SQLSTATS_SCRATCH_FILE_COUNT:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "Scr. File Count", Int64Val);
            break;
          case SQLSTATS_BMO_SPACE_BUFFER_SIZE:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "BMO Space Buffer Size", Int64Val);
            break;
          case SQLSTATS_BMO_SPACE_BUFFER_COUNT:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "BMO Space Buffer Count", Int64Val);
            break;
          case SQLSTATS_SCRATCH_READ_COUNT:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "Scr. Read Count", Int64Val);
            break;
          case SQLSTATS_SCRATCH_WRITE_COUNT:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "Scr. Write Count", Int64Val);
            break;
          case SQLSTATS_SCRATCH_IO_SIZE:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "Scr. IO Size", Int64Val);
            break;
          case SQLSTATS_SCRATCH_IO_MAX_TIME:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "Scr. IO Max Time", Int64Val);
            break;
          case SQLSTATS_INTERIM_ROW_COUNT:
            sprintf(Int64Val, "%ld", measStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-25s%s", "BMO Interim Row Count", Int64Val);
            break;
          default:
            statsBuf_[0] = '\0';
            break;
          }
          if (moveRowToUpQueue(statsBuf_, strlen(statsBuf_), &rc) == -1)
            return rc;
        }
        step_ = GET_NEXT_STATS_DESC_ENTRY_;
      }
      break;
    case GET_ROOTOPER_STATS_ENTRY_:
      {
        if (rootOperStatsItems_ == NULL)
        {
          maxRootOperStatsItems_ = 27;
          rootOperStatsItems_ = new (getGlobals()->getDefaultHeap()) 
                  SQLSTATS_ITEM[maxRootOperStatsItems_];
          initSqlStatsItems(rootOperStatsItems_, maxRootOperStatsItems_, FALSE);
          rootOperStatsItems_[0].statsItem_id = SQLSTATS_SQL_CPU_BUSY_TIME;
          rootOperStatsItems_[1].statsItem_id = SQLSTATS_SQL_MAX_WAIT_TIME;
          rootOperStatsItems_[2].statsItem_id = SQLSTATS_SQL_AVG_WAIT_TIME;
          rootOperStatsItems_[3].statsItem_id = SQLSTATS_UDR_CPU_BUSY_TIME;
          rootOperStatsItems_[4].statsItem_id = SQLSTATS_SQL_SPACE_ALLOC;
          rootOperStatsItems_[5].statsItem_id = SQLSTATS_SQL_SPACE_USED;
          rootOperStatsItems_[6].statsItem_id = SQLSTATS_SQL_HEAP_ALLOC;
          rootOperStatsItems_[7].statsItem_id = SQLSTATS_SQL_HEAP_USED;
          rootOperStatsItems_[8].statsItem_id = SQLSTATS_SQL_HEAP_WM;
          rootOperStatsItems_[9].statsItem_id = SQLSTATS_OPENS;
          rootOperStatsItems_[10].statsItem_id = SQLSTATS_OPEN_TIME;
          rootOperStatsItems_[11].statsItem_id = SQLSTATS_PROCESS_CREATED;
          rootOperStatsItems_[12].statsItem_id = SQLSTATS_PROCESS_CREATE_TIME;
          rootOperStatsItems_[13].statsItem_id = SQLSTATS_REQ_MSG_CNT;
          rootOperStatsItems_[14].statsItem_id = SQLSTATS_REQ_MSG_BYTES;
          rootOperStatsItems_[15].statsItem_id = SQLSTATS_REPLY_MSG_CNT;
          rootOperStatsItems_[16].statsItem_id = SQLSTATS_REPLY_MSG_BYTES;
          rootOperStatsItems_[17].statsItem_id = SQLSTATS_BMO_SPACE_BUFFER_SIZE;
          rootOperStatsItems_[18].statsItem_id = SQLSTATS_BMO_SPACE_BUFFER_COUNT;
          rootOperStatsItems_[19].statsItem_id = SQLSTATS_INTERIM_ROW_COUNT;
          rootOperStatsItems_[20].statsItem_id = SQLSTATS_SCRATCH_OVERFLOW_MODE;
          rootOperStatsItems_[21].statsItem_id = SQLSTATS_SCRATCH_FILE_COUNT;
          rootOperStatsItems_[22].statsItem_id = SQLSTATS_SCRATCH_IO_SIZE;
          rootOperStatsItems_[23].statsItem_id = SQLSTATS_SCRATCH_READ_COUNT;
          rootOperStatsItems_[24].statsItem_id = SQLSTATS_SCRATCH_WRITE_COUNT;
          rootOperStatsItems_[25].statsItem_id = SQLSTATS_SCRATCH_IO_MAX_TIME;
          rootOperStatsItems_[26].statsItem_id = SQLSTATS_TOPN;
          // maxRootOperStatsItems_ is set to 27
        }
        else
          initSqlStatsItems(rootOperStatsItems_, maxRootOperStatsItems_, TRUE);

        cliRC = SQL_EXEC_GetStatisticsItems(getStatsTdb().statsReqType_,
                getStatsTdb().stmtName_,
                getStatsTdb().stmtName_ ? str_len(getStatsTdb().stmtName_) : 0,
                maxRootOperStatsItems_,
                rootOperStatsItems_);
        if (cliRC < 0)
        {
          step_ = HANDLE_ERROR_;
        }
        else
          step_ = FORMAT_AND_RETURN_ROOTOPER_STATS_;
      }
      break;
    case FORMAT_AND_RETURN_ROOTOPER_STATS_:
      {
        for (; currStatsItemEntry_ < maxRootOperStatsItems_; currStatsItemEntry_++)
        {
          i = (short)currStatsItemEntry_;
          if (rootOperStatsItems_[i].error_code != 0)
            continue;
          switch (rootOperStatsItems_[i].statsItem_id)
          {         
          case SQLSTATS_SQL_CPU_BUSY_TIME:
            formatWInt64( rootOperStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-25s%s", "SQL Process Busy Time", Int64Val);
            break;
          case SQLSTATS_SQL_MAX_WAIT_TIME:
            if (getenv("SQL_DISPLAY_WAIT_TIME"))
            {
               // Convert it into microseconds
               sprintf(statsBuf_, "%-25s%s", "SQL Max Wait Time",
                 formatElapsedTime(timestampVal, rootOperStatsItems_[i].int64_value/1000));
            }
            else
               continue;
            break;
          case SQLSTATS_SQL_AVG_WAIT_TIME:
            if (getenv("SQL_DISPLAY_WAIT_TIME"))
            {
               // Convert it into microseconds
               sprintf(statsBuf_, "%-25s%s", "SQL Avg Wait Time",
                 formatElapsedTime(timestampVal, rootOperStatsItems_[i].int64_value/1000));
            }
            else
               continue;
            break;
          case SQLSTATS_UDR_CPU_BUSY_TIME:
            if (statsMergeType != SQLCLI_PROGRESS_STATS)
            {
              formatWInt64( rootOperStatsItems_[i], Int64Val);
              sprintf(statsBuf_, "%-25s%s", "UDR Process Busy Time", Int64Val);
            }
            else
              continue;
            break;
          case SQLSTATS_SQL_SPACE_ALLOC:
            formatWInt64( rootOperStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-25s%s KB", "SQL Space Allocated", Int64Val);
            break;
          case SQLSTATS_SQL_SPACE_USED:
            formatWInt64( rootOperStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-25s%s KB", "SQL Space Used", Int64Val);
            break;
          case SQLSTATS_SQL_HEAP_ALLOC:
            formatWInt64( rootOperStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-25s%s KB", "SQL Heap Allocated", Int64Val);
            break;
          case SQLSTATS_SQL_HEAP_USED:
            formatWInt64( rootOperStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-25s%s KB", "SQL Heap Used", Int64Val);
            break;
          case SQLSTATS_SQL_HEAP_WM:
            formatWInt64( rootOperStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-25s%s KB", "SQL Heap WM", Int64Val);
            break;
          case SQLSTATS_OPENS:
            formatWInt64( rootOperStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-25s%s", "Opens", Int64Val);
            break;
          case SQLSTATS_OPEN_TIME:
            formatWInt64( rootOperStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-25s%s", "Open Time", Int64Val);
            break;
          case SQLSTATS_PROCESS_CREATED:
            formatWInt64( rootOperStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-25s%s", "Processes Created", Int64Val);
            break;
          case SQLSTATS_PROCESS_CREATE_TIME:
            formatWInt64( rootOperStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-25s%s", "Process Create Time", Int64Val);
            break;
          case SQLSTATS_REQ_MSG_CNT:
            formatWInt64( rootOperStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-25s%s", "Request Message Count", Int64Val);
            break;
          case SQLSTATS_REQ_MSG_BYTES:
            formatWInt64( rootOperStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-25s%s", "Request Message Bytes", Int64Val);
            break;
          case SQLSTATS_REPLY_MSG_CNT:
            formatWInt64( rootOperStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-25s%s", "Reply Message Count", Int64Val);
            break;
          case SQLSTATS_REPLY_MSG_BYTES:
            formatWInt64( rootOperStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-25s%s", "Reply Message Bytes", Int64Val);
            break;
          case SQLSTATS_SCRATCH_OVERFLOW_MODE:
            if (statsMergeType != SQLCLI_PROGRESS_STATS)
            {
              sprintf(statsBuf_, "%-25s%s", "Scr. Overflow Mode", 
                ExBMOStats::getScratchOverflowMode((Int16)rootOperStatsItems_[i].int64_value));
            }
            else
              continue;
            break;
          case SQLSTATS_BMO_SPACE_BUFFER_SIZE:
            if (statsMergeType != SQLCLI_PROGRESS_STATS)
            {
              formatWInt64( rootOperStatsItems_[i], Int64Val);
              sprintf(statsBuf_, "%-25s%s", "BMO Space Buffer Size", Int64Val);
            }
            else
              continue;
            break;
          case SQLSTATS_BMO_SPACE_BUFFER_COUNT:
            if (statsMergeType != SQLCLI_PROGRESS_STATS)
            {
              formatWInt64( rootOperStatsItems_[i], Int64Val);
              sprintf(statsBuf_, "%-25s%s", "BMO Space Buffer Count", Int64Val);
            }
            else
              continue;
            break;
          case SQLSTATS_TOPN:
            if (statsMergeType != SQLCLI_PROGRESS_STATS)
            {
              formatWInt64( rootOperStatsItems_[i], Int64Val);
              sprintf(statsBuf_, "%-25s%s", "Sort TopN", Int64Val);
            }
            else
              continue;
            break;
          case SQLSTATS_SCRATCH_FILE_COUNT:
            if (statsMergeType != SQLCLI_PROGRESS_STATS)
            {
              formatWInt64( rootOperStatsItems_[i], Int64Val);
              sprintf(statsBuf_, "%-25s%s", "Scr. File Count", Int64Val);
            }
            else
              continue;
            break;
          case SQLSTATS_SCRATCH_IO_SIZE:
            if (statsMergeType != SQLCLI_PROGRESS_STATS)
            {
              formatWInt64( rootOperStatsItems_[i], Int64Val);
              sprintf(statsBuf_, "%-25s%s", "Scr. IO Size", Int64Val);
            }
            else
              continue;
            break;
          case SQLSTATS_SCRATCH_READ_COUNT:
            if (statsMergeType != SQLCLI_PROGRESS_STATS)
            {
              formatWInt64( rootOperStatsItems_[i], Int64Val);
              sprintf(statsBuf_, "%-25s%s", "Scr. Read Count", Int64Val);
            }
            else
              continue;
            break;
          case SQLSTATS_SCRATCH_WRITE_COUNT:
            if (statsMergeType != SQLCLI_PROGRESS_STATS)
            {
              formatWInt64( rootOperStatsItems_[i], Int64Val);
              sprintf(statsBuf_, "%-25s%s", "Scr. Write Count", Int64Val);
            }
            else
              continue;
            break;
          case SQLSTATS_SCRATCH_IO_MAX_TIME:
            if (statsMergeType != SQLCLI_PROGRESS_STATS)
            {
              formatWInt64( rootOperStatsItems_[i], Int64Val);
              sprintf(statsBuf_, "%-25s%s", "Scr. IO Max Time", Int64Val);
            }
            else
              continue;
            break;
          case SQLSTATS_INTERIM_ROW_COUNT:
            if (statsMergeType != SQLCLI_PROGRESS_STATS)
            {
              formatWInt64( rootOperStatsItems_[i], Int64Val);
              sprintf(statsBuf_, "%-25s%s", "BMO Interim Row Count", Int64Val);
            }
            else
              continue;
            break;
          default:
            statsBuf_[0] = '\0';
            break;
          }
          if (moveRowToUpQueue(statsBuf_, strlen(statsBuf_), &rc) == -1)
            return rc;
        }
        step_ = GET_NEXT_STATS_DESC_ENTRY_;
      }
      break;
    case GET_HBASE_STATS_ENTRY_:
      {
        if (hbaseStatsItems_ == NULL)
        {
          maxHbaseStatsItems_ = 11;
          hbaseStatsItems_ = new (getGlobals()->getDefaultHeap()) 
                  SQLSTATS_ITEM[maxHbaseStatsItems_];
          initSqlStatsItems(hbaseStatsItems_, maxHbaseStatsItems_, FALSE);
          hbaseStatsItems_[0].statsItem_id = SQLSTATS_TDB_ID;
          hbaseStatsItems_[1].statsItem_id = SQLSTATS_DOP;
          hbaseStatsItems_[2].statsItem_id = SQLSTATS_TABLE_ANSI_NAME;
          hbaseStatsItems_[3].statsItem_id = SQLSTATS_EST_ROWS_ACCESSED;
          hbaseStatsItems_[4].statsItem_id = SQLSTATS_ACT_ROWS_ACCESSED;
          hbaseStatsItems_[5].statsItem_id = SQLSTATS_EST_ROWS_USED;
          hbaseStatsItems_[6].statsItem_id = SQLSTATS_ACT_ROWS_USED;
          hbaseStatsItems_[7].statsItem_id = SQLSTATS_HBASE_IOS;
          hbaseStatsItems_[8].statsItem_id = SQLSTATS_HBASE_IO_BYTES;
          hbaseStatsItems_[9].statsItem_id = SQLSTATS_HBASE_IO_ELAPSED_TIME;
          hbaseStatsItems_[10].statsItem_id = SQLSTATS_HBASE_IO_MAX_TIME;
          // maxHbaseStatsItems_ is set to 11
          // SQLSTATS_TABLE_ANSI_NAM
          hbaseStatsItems_[2].str_value = new (getGlobals()->getDefaultHeap())
            char[ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES+1];
          hbaseStatsItems_[2].str_max_len = ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES;
        }
        else
          initSqlStatsItems(hbaseStatsItems_, maxHbaseStatsItems_, TRUE);
        cliRC = SQL_EXEC_GetStatisticsItems(getStatsTdb().statsReqType_,
                getStatsTdb().stmtName_,
                getStatsTdb().stmtName_ ? str_len(getStatsTdb().stmtName_) : 0,
                maxHbaseStatsItems_,
                hbaseStatsItems_);
        if (cliRC < 0)
        {
          step_ = HANDLE_ERROR_;
        }
        else
        {
          if (! isHeadingDisplayed_)
            step_ = DISPLAY_HBASE_STATS_HEADING_;
          else
            step_ = FORMAT_AND_RETURN_HBASE_STATS_;
        }
      }
      break;
    case DISPLAY_HBASE_STATS_HEADING_:
    case DISPLAY_HIVE_STATS_HEADING_:
      {
        if ((qparent_.up->getSize() - qparent_.up->getLength()) < 5)
	      return WORK_CALL_AGAIN;
        moveRowToUpQueue(" ");
        if (singleLineFormat_) {
           sprintf(statsBuf_, "%5s%10s%15s%20s%15s%20s%20s%20s%20s%20s%10s",
                "Id", "DOP",
                "EstRowsAccess", "ActRowsAccess", "EstRowUsed", "ActRowsUsed", "SE_IOs",
                "SE_IO_KBytes", "SE_IO_SumTime", "SE_IO_MaxTime", "TableName");
           moveRowToUpQueue(statsBuf_);
        }
        else {
           sprintf(statsBuf_, "%5s%10s %-20s", 
                "Id", "DOP","Table Name");
	   moveRowToUpQueue(statsBuf_);
           sprintf(statsBuf_, "%15s%20s%15s%20s%20s%20s%20s%20s",
		"EstRowsAccess", "ActRowsAccess", "EstRowsUsed", "ActRowsUsed", "SE_IOs", "SE_IO_KBytes", "SE_IO_SumTime", "SE_IO_MaxTime");
           moveRowToUpQueue(statsBuf_);
        }
        isHeadingDisplayed_ = TRUE;
        if (step_ == DISPLAY_HBASE_STATS_HEADING_)
           step_ = FORMAT_AND_RETURN_HBASE_STATS_;
        else
           step_ = FORMAT_AND_RETURN_HIVE_STATS_;
      }
      break;
    case FORMAT_AND_RETURN_HBASE_STATS_:
    case FORMAT_AND_RETURN_HIVE_STATS_:
      {
        short tableNameIndex = 2;
        SQLSTATS_ITEM *statsItems;
        Lng32 maxStatsItems;

        if (step_ == FORMAT_AND_RETURN_HBASE_STATS_) {
           statsItems = hbaseStatsItems_;
           maxStatsItems = maxHbaseStatsItems_;
        }
        else {
           statsItems = hiveStatsItems_;
           maxStatsItems = maxHiveStatsItems_;
        } 
        for (; currStatsItemEntry_ < maxStatsItems; currStatsItemEntry_++)
        {
          i = (short)currStatsItemEntry_;
          if (statsItems[i].error_code != 0)
            continue;
          switch (statsItems[i].statsItem_id)
          {
          case SQLSTATS_TDB_ID:
            sprintf(statsBuf_, "%5ld", statsItems[i].int64_value);
            break;
          case SQLSTATS_DOP:
            sprintf(&statsBuf_[strlen(statsBuf_)], "%10ld", statsItems[i].int64_value);
            break;
          case SQLSTATS_TABLE_ANSI_NAME:
            statsItems[i].str_value[statsItems[i].str_ret_len] = '\0';
            if (singleLineFormat_)
               tableNameIndex = i;
            else {
               sprintf(&statsBuf_[strlen(statsBuf_)], " %s", statsItems[i].str_value);
               if (moveRowToUpQueue(statsBuf_, strlen(statsBuf_), &rc) == -1)
                  return rc;
            }
            break;
          case SQLSTATS_EST_ROWS_ACCESSED:
            sprintf(formattedFloatVal, "%.6g", statsItems[i].double_value);
            if (singleLineFormat_)
               sprintf(&statsBuf_[strlen(statsBuf_)], "%15s", formattedFloatVal);
            else
               sprintf(statsBuf_, "%15s", formattedFloatVal);
            break;
          case SQLSTATS_EST_ROWS_USED:
            sprintf(formattedFloatVal, "%.6g", statsItems[i].double_value);
            sprintf(&statsBuf_[strlen(statsBuf_)], "%15s", formattedFloatVal);
            break;
          case SQLSTATS_ACT_ROWS_ACCESSED:
            sprintf(Int64Val, "%ld", statsItems[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(&statsBuf_[strlen(statsBuf_)], "%20s", Int64Val);
            break;          
          case SQLSTATS_ACT_ROWS_USED:
            sprintf(Int64Val, "%ld", statsItems[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(&statsBuf_[strlen(statsBuf_)], "%20s", Int64Val);
            break;
          case SQLSTATS_HBASE_IOS:
          case SQLSTATS_HIVE_IOS:
           sprintf(Int64Val, "%ld", statsItems[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(&statsBuf_[strlen(statsBuf_)], "%20s", Int64Val);
            break;
          case SQLSTATS_HBASE_IO_BYTES:
          case SQLSTATS_HIVE_IO_BYTES:
            sprintf(Int64Val, "%ld", statsItems[i].int64_value/1024);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(&statsBuf_[strlen(statsBuf_)], "%20s", Int64Val);
            break;
          case SQLSTATS_HBASE_IO_ELAPSED_TIME:
          case SQLSTATS_HIVE_IO_ELAPSED_TIME:
            sprintf(Int64Val, "%ld", statsItems[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(&statsBuf_[strlen(statsBuf_)], "%20s", Int64Val);
            break;
          case SQLSTATS_HBASE_IO_MAX_TIME:
          case SQLSTATS_HIVE_IO_MAX_TIME:
            sprintf(Int64Val, "%ld", statsItems[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(&statsBuf_[strlen(statsBuf_)], "%20s", Int64Val);
            break;
          default:
            break;
          }
        }
        if (singleLineFormat_)
           sprintf(&statsBuf_[strlen(statsBuf_)], " %s", statsItems[2].str_value);
        if (moveRowToUpQueue(statsBuf_, strlen(statsBuf_), &rc) == -1)
          return rc;
        step_ = GET_NEXT_STATS_DESC_ENTRY_;
      }
      break;
    case GET_HIVE_STATS_ENTRY_:
      {
        if (hiveStatsItems_ == NULL)
        {
          maxHiveStatsItems_ = 11;
          hiveStatsItems_ = new (getGlobals()->getDefaultHeap()) 
                  SQLSTATS_ITEM[maxHiveStatsItems_];
          initSqlStatsItems(hiveStatsItems_, maxHiveStatsItems_, FALSE);
          hiveStatsItems_[0].statsItem_id = SQLSTATS_TDB_ID;
          hiveStatsItems_[1].statsItem_id = SQLSTATS_DOP;
          hiveStatsItems_[2].statsItem_id = SQLSTATS_TABLE_ANSI_NAME;
          hiveStatsItems_[3].statsItem_id = SQLSTATS_EST_ROWS_ACCESSED;
          hiveStatsItems_[4].statsItem_id = SQLSTATS_ACT_ROWS_ACCESSED;
          hiveStatsItems_[5].statsItem_id = SQLSTATS_EST_ROWS_USED;
          hiveStatsItems_[6].statsItem_id = SQLSTATS_ACT_ROWS_USED;
          hiveStatsItems_[7].statsItem_id = SQLSTATS_HIVE_IOS;
          hiveStatsItems_[8].statsItem_id = SQLSTATS_HIVE_IO_BYTES;
          hiveStatsItems_[9].statsItem_id = SQLSTATS_HIVE_IO_ELAPSED_TIME;
          hiveStatsItems_[10].statsItem_id = SQLSTATS_HIVE_IO_MAX_TIME;
          // maxHiveStatsItems_ is set to 11 
          // SQLSTATS_TABLE_ANSI_NAME
          hiveStatsItems_[2].str_value = new (getGlobals()->getDefaultHeap())
            char[ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES+1];
          hiveStatsItems_[2].str_max_len = ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES;
        }
        else
          initSqlStatsItems(hiveStatsItems_, maxHiveStatsItems_, TRUE);
        cliRC = SQL_EXEC_GetStatisticsItems(getStatsTdb().statsReqType_,
                getStatsTdb().stmtName_,
                getStatsTdb().stmtName_ ? str_len(getStatsTdb().stmtName_) : 0,
                maxHiveStatsItems_,
                hiveStatsItems_);
        if (cliRC < 0)
        {
          step_ = HANDLE_ERROR_;
        }
        else
        {
          if (! isHeadingDisplayed_)
            step_ = DISPLAY_HIVE_STATS_HEADING_;
          else
            step_ = FORMAT_AND_RETURN_HIVE_STATS_;
        }
      }
      break;
    case GET_BMO_STATS_ENTRY_:
      {
        if (bmoStatsItems_ == NULL)
        {
          maxBMOStatsItems_ = 20;
          bmoStatsItems_ = new (getGlobals()->getDefaultHeap()) 
                  SQLSTATS_ITEM[maxBMOStatsItems_];
          initSqlStatsItems(bmoStatsItems_, maxBMOStatsItems_, FALSE);
          bmoStatsItems_[0].statsItem_id = SQLSTATS_TDB_ID;
          bmoStatsItems_[1].statsItem_id = SQLSTATS_TDB_NAME;
          bmoStatsItems_[2].statsItem_id = SQLSTATS_SCRATCH_OVERFLOW_MODE;
          bmoStatsItems_[3].statsItem_id = SQLSTATS_DOP;
          bmoStatsItems_[4].statsItem_id = SQLSTATS_TOPN;
          bmoStatsItems_[5].statsItem_id = SQLSTATS_BMO_PHASE;
          bmoStatsItems_[6].statsItem_id = SQLSTATS_INTERIM_ROW_COUNT;
          bmoStatsItems_[7].statsItem_id = SQLSTATS_OPER_CPU_TIME;
          bmoStatsItems_[8].statsItem_id = SQLSTATS_BMO_HEAP_USED;
          bmoStatsItems_[9].statsItem_id = SQLSTATS_BMO_HEAP_ALLOC;
          bmoStatsItems_[10].statsItem_id = SQLSTATS_BMO_HEAP_WM;
          bmoStatsItems_[11].statsItem_id = SQLSTATS_BMO_EST_MEMORY;
          bmoStatsItems_[12].statsItem_id = SQLSTATS_BMO_SPACE_BUFFER_SIZE;
          bmoStatsItems_[13].statsItem_id = SQLSTATS_BMO_SPACE_BUFFER_COUNT;
          bmoStatsItems_[14].statsItem_id = SQLSTATS_SCRATCH_FILE_COUNT;
          bmoStatsItems_[15].statsItem_id = SQLSTATS_SCRATCH_IO_SIZE;
          bmoStatsItems_[16].statsItem_id = SQLSTATS_SCRATCH_READ_COUNT;
          bmoStatsItems_[17].statsItem_id = SQLSTATS_SCRATCH_WRITE_COUNT;
          bmoStatsItems_[18].statsItem_id = SQLSTATS_SCRATCH_IO_TIME;
          bmoStatsItems_[19].statsItem_id = SQLSTATS_SCRATCH_IO_MAX_TIME;
                  
          // maxBMOStatsItems_ is set to 20 
          // TDB_NAME
          bmoStatsItems_[1].str_value = new (getGlobals()->getDefaultHeap())
                      char[MAX_TDB_NAME_LEN+1];
          bmoStatsItems_[1].str_max_len = MAX_TDB_NAME_LEN;
          // OVERFLOW_MODE
          bmoStatsItems_[2].str_value = new (getGlobals()->getDefaultHeap())
                      char[13];
          bmoStatsItems_[2].str_max_len = 12;
          // BMO_PHASE
          bmoStatsItems_[5].str_value = new (getGlobals()->getDefaultHeap())
                      char[12];
          bmoStatsItems_[5].str_max_len = 11;
        }
        else
          initSqlStatsItems(bmoStatsItems_, maxBMOStatsItems_, TRUE);
  
        cliRC = SQL_EXEC_GetStatisticsItems(getStatsTdb().statsReqType_,
                getStatsTdb().stmtName_,
                getStatsTdb().stmtName_ ? str_len(getStatsTdb().stmtName_) : 0,
                maxBMOStatsItems_,
                bmoStatsItems_);
        if (cliRC < 0)
        {
          step_ = HANDLE_ERROR_;
        }
        else
        {
          if (! isBMOHeadingDisplayed_)
            step_ = DISPLAY_BMO_STATS_HEADING_;
          else
            step_ = FORMAT_AND_RETURN_BMO_STATS_;
        }
      }
      break;
    case DISPLAY_BMO_STATS_HEADING_:
      {
       if ((qparent_.up->getSize() - qparent_.up->getLength()) < 4)
	      return WORK_CALL_AGAIN;
       moveRowToUpQueue(" ");
       if (singleLineFormat()) {       
          sprintf(statsBuf_, "%5s%20s%5s%10s%10s%12s%20s%20s%20s%20s%20s%20s%20s%20s%10s%10s%20s%20s%20s%20s",
               "Id", "TDBName", "Mode", "DOP", "TopN", "BMOPhase", "InterimRowCount", "CPUTime",
                "BMOHeapUsed", "BMOHeapAllocated", "BMOHeapWM", "EstMemory", 
                "BMOSpaceBufSz","BMOSpaceBufCnt", "FileCnt", "ScrIOSize",
                "ScrIORead", "ScrIOWritten", "ScrIOTime", "ScrIOMaxTime");
          moveRowToUpQueue(statsBuf_);
       }
       else {
          sprintf(statsBuf_, "%5s%20s%20s%10s%10s%20s%20s%20s",
               "Id", "TDBName", "Mode", "DOP", "TopN", "BMOPhase", "InterimRowCount", "CPUTime");
          moveRowToUpQueue(statsBuf_);
        
          sprintf(statsBuf_, "%25s%20s%20s%20s%20s%20s",
                "BMOHeapUsed", "BMOHeapAllocated", "BMOHeapWM", "EstMemory",
                "BMOSpaceBufSz","BMOSpaceBufCnt");
          moveRowToUpQueue(statsBuf_);
       
          sprintf(statsBuf_, "%25s%20s%20s%20s%20s%20s",
                "ScrFileCnt", "ScrIOSize", "ScrIORead", "ScrIOWritten", "ScrIOTime", "ScrIOMaxTime");
          moveRowToUpQueue(statsBuf_);
        }
        isBMOHeadingDisplayed_ = TRUE;
        step_ = FORMAT_AND_RETURN_BMO_STATS_;
      }
      break;
    case FORMAT_AND_RETURN_BMO_STATS_:
      {
        const char *ofMode;
        Int32 dop = 1;
        for (; currStatsItemEntry_ < maxBMOStatsItems_; currStatsItemEntry_++)
        {
          i = (short)currStatsItemEntry_;
          if (bmoStatsItems_[i].error_code != 0)
            continue;
          switch (bmoStatsItems_[i].statsItem_id)
          {
          case SQLSTATS_TDB_ID:
            sprintf(statsBuf_, "%5ld", bmoStatsItems_[i].int64_value);
            break;
          case SQLSTATS_TDB_NAME:
            bmoStatsItems_[i].str_value[bmoStatsItems_[i].str_ret_len] = '\0';
            if (singleLineFormat_) 
               sprintf(&statsBuf_[strlen(statsBuf_)], " %19s", bmoStatsItems_[i].str_value);
            else 
               sprintf(&statsBuf_[strlen(statsBuf_)], "%20s", bmoStatsItems_[i].str_value);
            break;
          case SQLSTATS_SCRATCH_OVERFLOW_MODE:
            ofMode = ExBMOStats::getScratchOverflowMode((Int16) bmoStatsItems_[i].int64_value);
            if (singleLineFormat_) 
               sprintf(&statsBuf_[strlen(statsBuf_)], "%5s", ofMode);
            else
               sprintf(&statsBuf_[strlen(statsBuf_)], "%20s", ofMode);
            break;
          case SQLSTATS_DOP:
            dop = (Int32)bmoStatsItems_[i].int64_value;
            sprintf(&statsBuf_[strlen(statsBuf_)], "%10ld", bmoStatsItems_[i].int64_value);
            break;
          case SQLSTATS_TOPN:
            sprintf(Int64Val, "%ld", bmoStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(&statsBuf_[strlen(statsBuf_)], "%10s", Int64Val);
            break;
          case SQLSTATS_BMO_PHASE:
            bmoStatsItems_[i].str_value[bmoStatsItems_[i].str_ret_len] = '\0';
            if (singleLineFormat_) 
               sprintf(&statsBuf_[strlen(statsBuf_)], "%12s", bmoStatsItems_[i].str_value);
            else
               sprintf(&statsBuf_[strlen(statsBuf_)], "%20s", bmoStatsItems_[i].str_value);
            break;
          case SQLSTATS_INTERIM_ROW_COUNT:
            sprintf(Int64Val, "%ld", bmoStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(&statsBuf_[strlen(statsBuf_)], "%20s", Int64Val);
            break;
          case SQLSTATS_OPER_CPU_TIME:
            sprintf(Int64Val, "%ld", bmoStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(&statsBuf_[strlen(statsBuf_)], "%20s", Int64Val);
            if (! singleLineFormat_) {
               if (moveRowToUpQueue(statsBuf_, strlen(statsBuf_), &rc) == -1)
                  return rc;
            }
            break;
          case SQLSTATS_BMO_HEAP_USED:
            sprintf(Int64Val, "%ld", bmoStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            if (singleLineFormat_) 
               sprintf(&statsBuf_[strlen(statsBuf_)], "%20s", Int64Val);
            else
               sprintf(statsBuf_, "%25s", Int64Val);
            break;
          case SQLSTATS_BMO_HEAP_ALLOC:
          case SQLSTATS_BMO_HEAP_WM:
            sprintf(Int64Val, "%ld", bmoStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(&statsBuf_[strlen(statsBuf_)], "%20s", Int64Val);
            break;
          case SQLSTATS_BMO_EST_MEMORY:
            sprintf(formattedFloatVal, "%.6g",  bmoStatsItems_[i].double_value * dop);
            str_sprintf(&statsBuf_[strlen(statsBuf_)], "%-20s", formattedFloatVal);
            break;
          case SQLSTATS_BMO_SPACE_BUFFER_SIZE:
            sprintf(Int64Val, "%ld", bmoStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(&statsBuf_[strlen(statsBuf_)], "%20s", Int64Val);
            break;
          case SQLSTATS_BMO_SPACE_BUFFER_COUNT:
            sprintf(Int64Val, "%ld", bmoStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(&statsBuf_[strlen(statsBuf_)], "%20s", Int64Val);
            if (! singleLineFormat_) {
               if (moveRowToUpQueue(statsBuf_, strlen(statsBuf_), &rc) == -1)
                  return rc;
            }
            break;
          case SQLSTATS_SCRATCH_FILE_COUNT:
            sprintf(Int64Val, "%ld", bmoStatsItems_[i].int64_value);
            if (singleLineFormat_) 
               sprintf(&statsBuf_[strlen(statsBuf_)], "%10s", Int64Val);
            else
               sprintf(statsBuf_, "%25s", Int64Val);
            break;
          case SQLSTATS_SCRATCH_IO_SIZE:
            sprintf(Int64Val, "%ld", bmoStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            if (singleLineFormat_) 
               sprintf(&statsBuf_[strlen(statsBuf_)], "%10s", Int64Val);
            else
               sprintf(&statsBuf_[strlen(statsBuf_)], "%20s", Int64Val);
            break;
          case SQLSTATS_SCRATCH_READ_COUNT:
            sprintf(Int64Val, "%ld", bmoStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(&statsBuf_[strlen(statsBuf_)], "%20s", Int64Val);
            break;
          case SQLSTATS_SCRATCH_WRITE_COUNT:
            sprintf(Int64Val, "%ld", bmoStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(&statsBuf_[strlen(statsBuf_)], "%20s", Int64Val);
            break;
          case SQLSTATS_SCRATCH_IO_TIME:
            sprintf(Int64Val, "%ld", bmoStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(&statsBuf_[strlen(statsBuf_)], "%20s", Int64Val);
            break;
          case SQLSTATS_SCRATCH_IO_MAX_TIME:
            sprintf(Int64Val, "%ld", bmoStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(&statsBuf_[strlen(statsBuf_)], "%20s", Int64Val);
            if (moveRowToUpQueue(statsBuf_, strlen(statsBuf_), &rc) == -1)
              return rc;
            break;
          default:
            break;
          }
        }
        step_ = GET_NEXT_STATS_DESC_ENTRY_;
      }
      break;
    case GET_REPLICATE_STATS_ENTRY_:
      {
        if (replicateStatsItems_ == NULL)
        {
          maxReplicateStatsItems_ = 31;
          replicateStatsItems_ = new (getGlobals()->getDefaultHeap())
                  SQLSTATS_ITEM[maxReplicateStatsItems_];
          initSqlStatsItems(replicateStatsItems_, maxReplicateStatsItems_, FALSE);
          replicateStatsItems_[0].statsItem_id = SQLSTATS_REPL_ANSI_NAME;
          replicateStatsItems_[1].statsItem_id = SQLSTATS_REPL_SOURCE_SYSTEM;
          replicateStatsItems_[2].statsItem_id = SQLSTATS_REPL_TARGET_SYSTEM;
          replicateStatsItems_[3].statsItem_id = SQLSTATS_REPL_OBJECT_TYPE;
          replicateStatsItems_[4].statsItem_id = SQLSTATS_REPL_TYPE;
          replicateStatsItems_[5].statsItem_id = SQLSTATS_REPL_NUM_PARTNS;
          replicateStatsItems_[6].statsItem_id = SQLSTATS_REPL_CONCURRENCY;
          replicateStatsItems_[7].statsItem_id = SQLSTATS_REPL_PHASE0_START_TIME;
          replicateStatsItems_[8].statsItem_id = SQLSTATS_REPL_PHASE0_END_TIME;
          replicateStatsItems_[9].statsItem_id = SQLSTATS_REPL_PHASE0_ELAPSED_TIME;
          replicateStatsItems_[10].statsItem_id = SQLSTATS_REPL_PHASE1_START_TIME;
          replicateStatsItems_[11].statsItem_id = SQLSTATS_REPL_PHASE1_END_TIME;
          replicateStatsItems_[12].statsItem_id = SQLSTATS_REPL_PHASE1_ELAPSED_TIME;
          replicateStatsItems_[13].statsItem_id = SQLSTATS_REPL_PHASE2_START_TIME;
          replicateStatsItems_[14].statsItem_id = SQLSTATS_REPL_PHASE2_END_TIME;
          replicateStatsItems_[15].statsItem_id = SQLSTATS_REPL_PHASE2_ELAPSED_TIME;
          replicateStatsItems_[16].statsItem_id = SQLSTATS_REPL_PHASE3_START_TIME;
          replicateStatsItems_[17].statsItem_id = SQLSTATS_REPL_PHASE3_END_TIME;
          replicateStatsItems_[18].statsItem_id = SQLSTATS_REPL_PHASE3_ELAPSED_TIME;
          replicateStatsItems_[19].statsItem_id = SQLSTATS_REPL_PHASE4_START_TIME;
          replicateStatsItems_[20].statsItem_id = SQLSTATS_REPL_PHASE4_END_TIME;
          replicateStatsItems_[21].statsItem_id = SQLSTATS_REPL_PHASE4_ELAPSED_TIME;
          replicateStatsItems_[22].statsItem_id = SQLSTATS_REPL_PHASE5_START_TIME;
          replicateStatsItems_[23].statsItem_id = SQLSTATS_REPL_PHASE5_END_TIME;
          replicateStatsItems_[24].statsItem_id = SQLSTATS_REPL_PHASE5_ELAPSED_TIME;
          replicateStatsItems_[25].statsItem_id = SQLSTATS_REPL_PHASE6_START_TIME;
          replicateStatsItems_[26].statsItem_id = SQLSTATS_REPL_PHASE6_END_TIME;
          replicateStatsItems_[27].statsItem_id = SQLSTATS_REPL_PHASE6_ELAPSED_TIME;
          replicateStatsItems_[28].statsItem_id = SQLSTATS_REPL_PERCENT_DONE;
          replicateStatsItems_[29].statsItem_id = SQLSTATS_REPL_STATUS;
          replicateStatsItems_[30].statsItem_id = SQLSTATS_REPL_COMPLETED_PARTNS;

          // maxReplicateStatsItems_ is set to 31
          // ANSI_NAME
          replicateStatsItems_[0].str_value = new (getGlobals()->getDefaultHeap())
                      char[ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES+1];
          replicateStatsItems_[0].str_max_len = ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES;
          // SOURCE_SYSTEM_NAME
          replicateStatsItems_[1].str_value = new (getGlobals()->getDefaultHeap())
                      char[24];
          replicateStatsItems_[1].str_max_len = 23;
          // TARGET_SYSTEM_NAME
          replicateStatsItems_[2].str_value = new (getGlobals()->getDefaultHeap())
                      char[24];
          replicateStatsItems_[2].str_max_len = 23;
          // OBJECT_TYPE
          replicateStatsItems_[3].str_value = new (getGlobals()->getDefaultHeap())
                      char[51];
          replicateStatsItems_[3].str_max_len = 50;
          // REPL_TYPE
          replicateStatsItems_[4].str_value = new (getGlobals()->getDefaultHeap())
                      char[51];
          replicateStatsItems_[4].str_max_len = 50;
          // STATUS
          replicateStatsItems_[29].str_value = new (getGlobals()->getDefaultHeap())
                      char[51];
          replicateStatsItems_[29].str_max_len = 50;
        }
        else
          initSqlStatsItems(replicateStatsItems_, maxReplicateStatsItems_, TRUE);

        cliRC = SQL_EXEC_GetStatisticsItems(getStatsTdb().statsReqType_,
                getStatsTdb().stmtName_,
                getStatsTdb().stmtName_ ? str_len(getStatsTdb().stmtName_) : 0,
                maxReplicateStatsItems_,
                replicateStatsItems_);
        if (cliRC < 0)
          step_ = HANDLE_ERROR_;
        else
          step_ = FORMAT_AND_RETURN_REPLICATE_STATS_;
      }
      break;
   case FORMAT_AND_RETURN_REPLICATE_STATS_:
      {
        for (; currStatsItemEntry_ < maxReplicateStatsItems_; currStatsItemEntry_++)
        {
          i = (short)currStatsItemEntry_;
          if (replicateStatsItems_[i].error_code != 0)
            continue;
          switch (replicateStatsItems_[i].statsItem_id)
          {
          case SQLSTATS_REPL_ANSI_NAME:
            replicateStatsItems_[i].str_value[replicateStatsItems_[i].str_ret_len] = '\0';
            sprintf(statsBuf_, "%-25s%s", "Object Ansi Name", replicateStatsItems_[i].str_value);
            break;
          case SQLSTATS_REPL_SOURCE_SYSTEM:
            replicateStatsItems_[i].str_value[replicateStatsItems_[i].str_ret_len] = '\0';
            sprintf(statsBuf_, "%-25s%s", "Source System ", replicateStatsItems_[i].str_value);
            break;
          case SQLSTATS_REPL_TARGET_SYSTEM:
            replicateStatsItems_[i].str_value[replicateStatsItems_[i].str_ret_len] = '\0';
            sprintf(statsBuf_, "%-25s%s", "Target System ", replicateStatsItems_[i].str_value);
            break;
          case SQLSTATS_REPL_OBJECT_TYPE:
            replicateStatsItems_[i].str_value[replicateStatsItems_[i].str_ret_len] = '\0';
            sprintf(statsBuf_, "%-25s%s", "Object Type ", replicateStatsItems_[i].str_value);
            break;
          case SQLSTATS_REPL_TYPE:
            replicateStatsItems_[i].str_value[replicateStatsItems_[i].str_ret_len] = '\0';
            sprintf(statsBuf_, "%-25s%s", "Replicate Type ", replicateStatsItems_[i].str_value);
            break;
          case SQLSTATS_REPL_NUM_PARTNS:
            sprintf(statsBuf_, "%-25s%-ld", "Number of Partitions ", replicateStatsItems_[i].int64_value);
            break;
          case SQLSTATS_REPL_CONCURRENCY:
            sprintf(statsBuf_, "%-25s%-ld", "Replication Concurrency ", replicateStatsItems_[i].int64_value);
            break;
          case SQLSTATS_REPL_PHASE0_START_TIME:
            if (replicateStatsItems_[i].int64_value == -1)
               continue;
            sprintf(statsBuf_, "%-25s%s", "Phase0 Start Time", 
                      formatTimestamp(timestampVal, replicateStatsItems_[i].int64_value));
            break;
          case SQLSTATS_REPL_PHASE0_END_TIME:
            if (replicateStatsItems_[i].int64_value == -1)
               continue;
            sprintf(statsBuf_, "%-25s%s", "Phase0 End Time", 
                      formatTimestamp(timestampVal, replicateStatsItems_[i].int64_value));
            break;
          case SQLSTATS_REPL_PHASE0_ELAPSED_TIME:
            if (replicateStatsItems_[i].int64_value == -1)
               continue;
            sprintf(statsBuf_, "%-25s%s", "Phase0 Elapsed Time", 
                      formatElapsedTime(timestampVal, replicateStatsItems_[i].int64_value));
            break;
          case SQLSTATS_REPL_PHASE1_START_TIME:
            if (replicateStatsItems_[i].int64_value == -1)
               continue;
            sprintf(statsBuf_, "%-25s%s", "Phase1 Start Time", 
                      formatTimestamp(timestampVal, replicateStatsItems_[i].int64_value));
            break;
          case SQLSTATS_REPL_PHASE1_END_TIME:
            if (replicateStatsItems_[i].int64_value == -1)
               continue;
            sprintf(statsBuf_, "%-25s%s", "Phase1 End Time", 
                      formatTimestamp(timestampVal, replicateStatsItems_[i].int64_value));
            break;
          case SQLSTATS_REPL_PHASE1_ELAPSED_TIME:
            if (replicateStatsItems_[i].int64_value == -1)
               continue;
            sprintf(statsBuf_, "%-25s%s", "Phase1 Elapsed Time", 
                      formatElapsedTime(timestampVal, replicateStatsItems_[i].int64_value));
            break;
          case SQLSTATS_REPL_PHASE2_START_TIME:
            if (replicateStatsItems_[i].int64_value == -1)
               continue;
            sprintf(statsBuf_, "%-25s%s", "Phase2 Start Time", 
                      formatTimestamp(timestampVal, replicateStatsItems_[i].int64_value));
            break;
          case SQLSTATS_REPL_PHASE2_END_TIME:
            if (replicateStatsItems_[i].int64_value == -1)
               continue;
            sprintf(statsBuf_, "%-25s%s", "Phase2 End Time", 
                      formatTimestamp(timestampVal, replicateStatsItems_[i].int64_value));
            break;
          case SQLSTATS_REPL_PHASE2_ELAPSED_TIME:
            if (replicateStatsItems_[i].int64_value == -1)
               continue;
            sprintf(statsBuf_, "%-25s%s", "Phase2 Elapsed Time", 
                      formatElapsedTime(timestampVal, replicateStatsItems_[i].int64_value));
            break;
          case SQLSTATS_REPL_PHASE3_START_TIME:
            if (replicateStatsItems_[i].int64_value == -1)
               continue;
            sprintf(statsBuf_, "%-25s%s", "Phase3 Start Time", 
                      formatTimestamp(timestampVal, replicateStatsItems_[i].int64_value));
            break;
          case SQLSTATS_REPL_PHASE3_END_TIME:
            if (replicateStatsItems_[i].int64_value == -1)
               continue;
            sprintf(statsBuf_, "%-25s%s", "Phase3 End Time", 
                      formatTimestamp(timestampVal, replicateStatsItems_[i].int64_value));
            break;
          case SQLSTATS_REPL_PHASE3_ELAPSED_TIME:
            if (replicateStatsItems_[i].int64_value == -1)
               continue;
            sprintf(statsBuf_, "%-25s%s", "Phase3 Elapsed Time", 
                      formatElapsedTime(timestampVal, replicateStatsItems_[i].int64_value));
            break;
          case SQLSTATS_REPL_PHASE4_START_TIME:
            if (replicateStatsItems_[i].int64_value == -1)
               continue;
            sprintf(statsBuf_, "%-25s%s", "Phase4 Start Time", 
                      formatTimestamp(timestampVal, replicateStatsItems_[i].int64_value));
            break;
          case SQLSTATS_REPL_PHASE4_END_TIME:
            if (replicateStatsItems_[i].int64_value == -1)
               continue;
            sprintf(statsBuf_, "%-25s%s", "Phase4 End Time", 
                      formatTimestamp(timestampVal, replicateStatsItems_[i].int64_value));
            break;
          case SQLSTATS_REPL_PHASE4_ELAPSED_TIME:
            if (replicateStatsItems_[i].int64_value == -1)
               continue;
            sprintf(statsBuf_, "%-25s%s", "Phase4 Elapsed Time", 
                      formatElapsedTime(timestampVal, replicateStatsItems_[i].int64_value));
            break;
          case SQLSTATS_REPL_PHASE5_START_TIME:
            if (replicateStatsItems_[i].int64_value == -1)
               continue;
            sprintf(statsBuf_, "%-25s%s", "Phase5 Start Time", 
                      formatTimestamp(timestampVal, replicateStatsItems_[i].int64_value));
            break;
          case SQLSTATS_REPL_PHASE5_END_TIME:
            if (replicateStatsItems_[i].int64_value == -1)
               continue;
            sprintf(statsBuf_, "%-25s%s", "Phase5 End Time", 
                      formatTimestamp(timestampVal, replicateStatsItems_[i].int64_value));
            break;
          case SQLSTATS_REPL_PHASE5_ELAPSED_TIME:
            if (replicateStatsItems_[i].int64_value == -1)
               continue;
            sprintf(statsBuf_, "%-25s%s", "Phase5 Elapsed Time", 
                      formatElapsedTime(timestampVal, replicateStatsItems_[i].int64_value));
            break;
          case SQLSTATS_REPL_PHASE6_START_TIME:
            if (replicateStatsItems_[i].int64_value == -1)
               continue;
            sprintf(statsBuf_, "%-25s%s", "Phase6 Start Time", 
                      formatTimestamp(timestampVal, replicateStatsItems_[i].int64_value));
            break;
          case SQLSTATS_REPL_PHASE6_END_TIME:
            if (replicateStatsItems_[i].int64_value == -1)
               continue;
            sprintf(statsBuf_, "%-25s%s", "Phase6 End Time", 
                      formatTimestamp(timestampVal, replicateStatsItems_[i].int64_value));
            break;
          case SQLSTATS_REPL_PHASE6_ELAPSED_TIME:
            if (replicateStatsItems_[i].int64_value == -1)
               continue;
            sprintf(statsBuf_, "%-25s%s", "Phase6 Elapsed Time", 
                      formatElapsedTime(timestampVal, replicateStatsItems_[i].int64_value));
            break;
          case SQLSTATS_REPL_PERCENT_DONE:
            formatWInt64(replicateStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-25s%s ", "Percent Done", Int64Val);
            break;
          case SQLSTATS_REPL_STATUS:
            replicateStatsItems_[i].str_value[replicateStatsItems_[i].str_ret_len] = '\0';
            sprintf(statsBuf_, "%-25s%s", "Replicate Status", replicateStatsItems_[i].str_value);
            break;
          case SQLSTATS_REPL_COMPLETED_PARTNS:
            sprintf(statsBuf_, "%-25s%-ld", "Replicated Partitions ", replicateStatsItems_[i].int64_value);
            break;
          default:
            statsBuf_[0] = '\0';
            break;
          }
          if (moveRowToUpQueue(statsBuf_, strlen(statsBuf_), &rc) == -1)
            return rc;
        }
        step_ = GET_NEXT_STATS_DESC_ENTRY_;
      }
      break;
    case GET_REPLICATOR_STATS_ENTRY_:
      {
        if (replicatorStatsItems_ == NULL)
        {
          maxReplicatorStatsItems_ = 8;
          replicatorStatsItems_ = new (getGlobals()->getDefaultHeap())
                  SQLSTATS_ITEM[maxReplicatorStatsItems_];
          initSqlStatsItems(replicatorStatsItems_, maxReplicatorStatsItems_, FALSE);
          replicatorStatsItems_[0].statsItem_id = SQLSTATS_REPL_BLOCK_LEN;
          replicatorStatsItems_[1].statsItem_id = SQLSTATS_REPL_TOTAL_BLOCKS;
          replicatorStatsItems_[2].statsItem_id = SQLSTATS_REPL_BLOCKS_READ;
          replicatorStatsItems_[3].statsItem_id = SQLSTATS_REPL_ROWS_READ;
          replicatorStatsItems_[4].statsItem_id = SQLSTATS_REPL_BLOCKS_REPLICATED;
          replicatorStatsItems_[5].statsItem_id = SQLSTATS_REPL_TOTAL_COMPRESS_TIME;
          replicatorStatsItems_[6].statsItem_id = SQLSTATS_REPL_TOTAL_COMPRESS_BYTES;
          replicatorStatsItems_[7].statsItem_id = SQLSTATS_REPL_TOTAL_UNCOMPRESS_TIME;
        }
        else
          initSqlStatsItems(replicatorStatsItems_, maxReplicatorStatsItems_, TRUE);

        cliRC = SQL_EXEC_GetStatisticsItems(getStatsTdb().statsReqType_,
                getStatsTdb().stmtName_,
                getStatsTdb().stmtName_ ? str_len(getStatsTdb().stmtName_) : 0,
                maxReplicatorStatsItems_,
                replicatorStatsItems_);
        if (cliRC < 0)
          step_ = HANDLE_ERROR_;
        else
          step_ = FORMAT_AND_RETURN_REPLICATOR_STATS_;
      }
      break;
    case FORMAT_AND_RETURN_REPLICATOR_STATS_:
      {
        for (; currStatsItemEntry_ < maxReplicatorStatsItems_; currStatsItemEntry_++)
        {
          i = (short)currStatsItemEntry_;
          if (replicatorStatsItems_[i].error_code != 0)
            continue;
          switch (replicatorStatsItems_[i].statsItem_id)
          {
          case SQLSTATS_REPL_BLOCK_LEN:
            formatWInt64(replicatorStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-25s%s ", "Block Length", Int64Val);
            break;
          case SQLSTATS_REPL_TOTAL_BLOCKS:
            formatWInt64(replicatorStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-25s%s ", "Total Blocks", Int64Val);
            break;
          case SQLSTATS_REPL_BLOCKS_READ:
            formatWInt64(replicatorStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-25s%s ", "Blocks Read", Int64Val);
            break;
          case SQLSTATS_REPL_ROWS_READ:
            formatWInt64(replicatorStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-25s%s ", "Rows Read", Int64Val);
            break;
          case SQLSTATS_REPL_BLOCKS_REPLICATED:
            formatWInt64(replicatorStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-25s%s ", "Blocks Replicated", Int64Val);
            break;
          case SQLSTATS_REPL_TOTAL_COMPRESS_TIME:
            formatWInt64(replicatorStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-25s%s ", "Total Compress Time", Int64Val);
            break;
          case SQLSTATS_REPL_TOTAL_COMPRESS_BYTES:
            formatWInt64(replicatorStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-25s%s ", "Total Compress Bytes", Int64Val);
            break;
          case SQLSTATS_REPL_TOTAL_UNCOMPRESS_TIME:
            formatWInt64(replicatorStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-25s%s ", "Total Uncompress Time", Int64Val);
            break;
          default:
            statsBuf_[0] = '\0';
            break;
          }
          if (moveRowToUpQueue(statsBuf_, strlen(statsBuf_), &rc) == -1)
            return rc;
        }
        step_ = GET_NEXT_STATS_DESC_ENTRY_;
      }
      break;
    case GET_OPER_STATS_ENTRY_:
      {
       maxOperatorStatsItems_ = 13;
       if (operatorStatsItems_ == NULL)
        {
          operatorStatsItems_ = new (getGlobals()->getDefaultHeap()) 
                                            SQLSTATS_ITEM[maxOperatorStatsItems_];
          initSqlStatsItems(operatorStatsItems_, maxOperatorStatsItems_, FALSE);

          formatOperStatItems(operatorStatsItems_);                
        }
        else
        {
          // reset tdb_id and stats type
          for (i = 0; i < maxOperatorStatsItems_; i++)
          {
            operatorStatsItems_[i].tdb_id = sqlStatsDesc_[currStatsDescEntry_].tdb_id;
            operatorStatsItems_[i].stats_type = sqlStatsDesc_[currStatsDescEntry_].stats_type;            
          }
        }
        cliRC = SQL_EXEC_GetStatisticsItems(getStatsTdb().statsReqType_,
                getStatsTdb().stmtName_,
                getStatsTdb().stmtName_ ? str_len(getStatsTdb().stmtName_) : 0,
                maxOperatorStatsItems_,
                operatorStatsItems_);
        if (cliRC < 0)
	{
	  step_ = HANDLE_ERROR_;
	}
        else
        {
          step_ = FORMAT_AND_RETURN_OPER_STATS_;
        }
      }
      break;

    case FORMAT_AND_RETURN_OPER_STATS_:
      {
      formatOperStats(operatorStatsItems_);	            
      if (moveRowToUpQueue(statsBuf_, strlen(statsBuf_), &rc) == -1)
        return rc;
      step_ = GET_NEXT_STATS_DESC_ENTRY_;
      }
      break;
    case GET_RMS_STATS_ENTRY_:
      {
        if (rmsStatsItems_ == NULL)
        {
          maxRMSStatsItems_ = 34;
          rmsStatsItems_ = new (getGlobals()->getDefaultHeap()) 
                  SQLSTATS_ITEM[maxRMSStatsItems_];
          initSqlStatsItems(rmsStatsItems_, maxRMSStatsItems_, FALSE);
          rmsStatsItems_[0].statsItem_id = SQLSTATS_NODE_NAME;
          rmsStatsItems_[1].statsItem_id = SQLSTATS_CPU;
          rmsStatsItems_[2].statsItem_id = SQLSTATS_RMS_VER;
          rmsStatsItems_[3].statsItem_id = SQLSTATS_RMS_ENV_TYPE;
          rmsStatsItems_[4].statsItem_id = SQLSTATS_SSCP_PID;
          rmsStatsItems_[5].statsItem_id = SQLSTATS_SSCP_PRIORITY;
          rmsStatsItems_[6].statsItem_id = SQLSTATS_SSCP_TIMESTAMP;
          rmsStatsItems_[7].statsItem_id = SQLSTATS_SSMP_PID;
          rmsStatsItems_[8].statsItem_id = SQLSTATS_SSMP_PRIORITY;
          rmsStatsItems_[9].statsItem_id = SQLSTATS_SSMP_TIMESTAMP;
          rmsStatsItems_[10].statsItem_id = SQLSTATS_STORE_SRC_LEN;
          rmsStatsItems_[11].statsItem_id = SQLSTATS_STATS_HEAP_ALLOC;
          rmsStatsItems_[12].statsItem_id = SQLSTATS_STATS_HEAP_USED;
          rmsStatsItems_[13].statsItem_id = SQLSTATS_STATS_HEAP_HIGH_WM;
          rmsStatsItems_[14].statsItem_id = SQLSTATS_PROCESS_STATS_HEAPS;
          rmsStatsItems_[15].statsItem_id = SQLSTATS_PROCESSES_REGD;
          rmsStatsItems_[16].statsItem_id = SQLSTATS_QUERIES_REGD;
          rmsStatsItems_[17].statsItem_id = SQLSTATS_RMS_SEMAPHORE_PID;
          rmsStatsItems_[18].statsItem_id = SQLSTATS_SSCPS_OPENED;
          rmsStatsItems_[19].statsItem_id = SQLSTATS_SSCPS_DELETED_OPENS;
          rmsStatsItems_[20].statsItem_id = SQLSTATS_LAST_GC_TIME;
          rmsStatsItems_[21].statsItem_id = SQLSTATS_QUERIES_GCED_IN_LAST_RUN;
          rmsStatsItems_[22].statsItem_id = SQLSTATS_TOTAL_QUERIES_GCED;
          rmsStatsItems_[23].statsItem_id = SQLSTATS_SSMP_REQ_MSG_CNT;
          rmsStatsItems_[24].statsItem_id = SQLSTATS_SSMP_REQ_MSG_BYTES;
          rmsStatsItems_[25].statsItem_id = SQLSTATS_SSMP_REPLY_MSG_CNT;
          rmsStatsItems_[26].statsItem_id = SQLSTATS_SSMP_REPLY_MSG_BYTES;
          rmsStatsItems_[27].statsItem_id = SQLSTATS_SSCP_REQ_MSG_CNT;
          rmsStatsItems_[28].statsItem_id = SQLSTATS_SSCP_REQ_MSG_BYTES;
          rmsStatsItems_[29].statsItem_id = SQLSTATS_SSCP_REPLY_MSG_CNT;
          rmsStatsItems_[30].statsItem_id = SQLSTATS_SSCP_REPLY_MSG_BYTES;
          rmsStatsItems_[31].statsItem_id = SQLSTATS_RMS_STATS_RESET_TIMESTAMP;
          rmsStatsItems_[32].statsItem_id = SQLSTATS_RMS_STATS_NUM_SQL_SIK;
          rmsStatsItems_[33].statsItem_id = SQLSTATS_RMS_CONFIGURED_PID_MAX;
          // maxRMSStatsItems_ is set to 34
          rmsStatsItems_[0].str_value = new (getGlobals()->getDefaultHeap())
            char[MAX_SEGMENT_NAME_LEN+1];
          rmsStatsItems_[0].str_max_len = MAX_SEGMENT_NAME_LEN;
        }
        else
          initSqlStatsItems(rmsStatsItems_, maxRMSStatsItems_, TRUE);

        cliRC = SQL_EXEC_GetStatisticsItems(getStatsTdb().statsReqType_,
                getStatsTdb().stmtName_,
                getStatsTdb().stmtName_ ? str_len(getStatsTdb().stmtName_) : 0,
                maxRMSStatsItems_,
                rmsStatsItems_);
        if (cliRC < 0)
        {
          step_ = HANDLE_ERROR_;
        }
        else
          step_ = FORMAT_AND_RETURN_RMS_STATS_;
      }
      break;
    case FORMAT_AND_RETURN_RMS_STATS_:
      {
        for (; currStatsItemEntry_ < maxRMSStatsItems_; currStatsItemEntry_++)
        {
          i = (short)currStatsItemEntry_;
          statsBuf_[0] = '\0';
          if (rmsStatsItems_[i].error_code != 0)
            continue;
          switch (rmsStatsItems_[i].statsItem_id)
          {
          case SQLSTATS_NODE_NAME:
            rmsStatsItems_[i].str_value[rmsStatsItems_[i].str_ret_len] = '\0';
            sprintf(statsBuf_, "%-30s%s", "Node name", rmsStatsItems_[i].str_value);
            break;
          case SQLSTATS_CPU:
            sprintf(statsBuf_, "%-30s%-ld", "Node Id",  rmsStatsItems_[i].int64_value);
            break;  
          case SQLSTATS_RMS_VER:
            sprintf(statsBuf_, "%-30s%-ld", "RMS Version",  rmsStatsItems_[i].int64_value);
            break;  
          case SQLSTATS_RMS_ENV_TYPE:
            if ((StatsGlobals::RTSEnvType)rmsStatsItems_[i].int64_value == StatsGlobals::RTS_PRIVATE_ENV)
              sprintf(statsBuf_, "%-30s%s", "RMS Env Type",  
                StatsGlobals::rmsEnvType((StatsGlobals::RTSEnvType)rmsStatsItems_[i].int64_value));
            else
              continue;
            break;          
          case SQLSTATS_SSCP_PID:
            sprintf(statsBuf_, "%-30s%-ld", "SSCP PID",  rmsStatsItems_[i].int64_value);
            break;          
          case SQLSTATS_SSCP_TIMESTAMP:
	    jtime = CONVERTTIMESTAMP(rmsStatsItems_[i].int64_value,0,-1,NULL);
	    INTERPRETTIMESTAMP(jtime, timestamp);
	    sprintf(statsBuf_, "%-30s%04d/%02d/%02d %02d:%02d:%02d.%03u%03u",
                        "SSCP Creation Timestamp ",
			timestamp[0], timestamp[1], timestamp[2],
			timestamp[3], timestamp[4], timestamp[5],
			timestamp[6], timestamp[7]);
	    break;
          case SQLSTATS_SSMP_PID:
            sprintf(statsBuf_, "%-30s%-ld", "SSMP PID",  rmsStatsItems_[i].int64_value);
            break;          
          case SQLSTATS_SSMP_TIMESTAMP:
	    jtime = CONVERTTIMESTAMP(rmsStatsItems_[i].int64_value,0,-1,NULL);
	    INTERPRETTIMESTAMP(jtime, timestamp);
	    sprintf(statsBuf_, "%-30s%04d/%02d/%02d %02d:%02d:%02d.%03u%03u",
                        "SSMP Creation Timestamp ",
			timestamp[0], timestamp[1], timestamp[2],
			timestamp[3], timestamp[4], timestamp[5],
			timestamp[6], timestamp[7]);
	    break;
          case SQLSTATS_STORE_SRC_LEN:
            sprintf(statsBuf_, "%-30s%-ld", "Source String Store Len",  rmsStatsItems_[i].int64_value);
            break;          
          case SQLSTATS_STATS_HEAP_ALLOC:
            sprintf(Int64Val, "%ld", rmsStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-30s%s", "Stats Heap Allocated", Int64Val);
            break;
          case SQLSTATS_STATS_HEAP_USED:
            sprintf(Int64Val, "%ld", rmsStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-30s%s", "Stats Heap Used", Int64Val);
            break;
          case SQLSTATS_STATS_HEAP_HIGH_WM:
            sprintf(Int64Val, "%ld", rmsStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-30s%s", "Stats Heap High WM", Int64Val);
            break;
          case SQLSTATS_PROCESS_STATS_HEAPS:
            sprintf(statsBuf_, "%-30s%-ld", "No.of Process Stats Heaps",  rmsStatsItems_[i].int64_value);
            break;
          case SQLSTATS_PROCESSES_REGD:
            sprintf(statsBuf_, "%-30s%-ld", "No.of Process Regd.",  rmsStatsItems_[i].int64_value);
            break;
          case SQLSTATS_QUERIES_REGD:
            sprintf(statsBuf_, "%-30s%-ld", "No.of Query Fragments Regd.",  rmsStatsItems_[i].int64_value);
            break;
          case SQLSTATS_RMS_SEMAPHORE_PID:
            sprintf(statsBuf_, "%-30s%-ld", "RMS Semaphore Owner",  rmsStatsItems_[i].int64_value);
            break;
          case SQLSTATS_RMS_STATS_NUM_SQL_SIK:
            sprintf(statsBuf_, "%-30s%-ld", "No. Query Invalidation Keys", rmsStatsItems_[i].int64_value);
            break;
          case SQLSTATS_SSCPS_OPENED:
            sprintf(statsBuf_, "%-30s%-ld", "No.of SSCPs Opened",  rmsStatsItems_[i].int64_value);
            break;  
          case SQLSTATS_SSCPS_DELETED_OPENS:
            sprintf(statsBuf_, "%-30s%-ld", "No.of SSCPs Open Deleted",  rmsStatsItems_[i].int64_value);
            break; 
          case SQLSTATS_LAST_GC_TIME:
	    jtime = CONVERTTIMESTAMP(rmsStatsItems_[i].int64_value,0,-1,NULL);
	    INTERPRETTIMESTAMP(jtime, timestamp);
	    sprintf(statsBuf_, "%-30s%04d/%02d/%02d %02d:%02d:%02d.%03u%03u",
                        "Last GC Time ",
			timestamp[0], timestamp[1], timestamp[2],
			timestamp[3], timestamp[4], timestamp[5],
			timestamp[6], timestamp[7]);
	    break;
          case SQLSTATS_QUERIES_GCED_IN_LAST_RUN:
            sprintf(statsBuf_, "%-30s%-ld", "Queries GCed in Last Run",  rmsStatsItems_[i].int64_value);
            break; 
          case SQLSTATS_TOTAL_QUERIES_GCED:
            sprintf(Int64Val, "%ld", rmsStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-30s%s", "Total Queries GCed ",  Int64Val);
            break; 
          case SQLSTATS_SSMP_REQ_MSG_CNT:
            sprintf(Int64Val, "%ld", rmsStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-30s%s", "SSMP Request Message Count", Int64Val);
            break;
          case SQLSTATS_SSMP_REQ_MSG_BYTES:
            sprintf(Int64Val, "%ld", rmsStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-30s%s", "SSMP Request Message Bytes", Int64Val);
            break;
          case SQLSTATS_SSMP_REPLY_MSG_CNT:
            sprintf(Int64Val, "%ld", rmsStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-30s%s", "SSMP Reply Message Count", Int64Val);
            break;
          case SQLSTATS_SSMP_REPLY_MSG_BYTES:
            sprintf(Int64Val, "%ld", rmsStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-30s%s", "SSMP Reply Message Bytes", Int64Val);
            break;
          case SQLSTATS_SSCP_REQ_MSG_CNT:
            sprintf(Int64Val, "%ld", rmsStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-30s%s", "SSCP Request Message Count", Int64Val);
            break;
          case SQLSTATS_SSCP_REQ_MSG_BYTES:
            sprintf(Int64Val, "%ld", rmsStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-30s%s", "SSCP Request Message Bytes", Int64Val);
            break;
          case SQLSTATS_SSCP_REPLY_MSG_CNT:
            sprintf(Int64Val, "%ld", rmsStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-30s%s", "SSCP Reply Message Count", Int64Val);
            break;
          case SQLSTATS_SSCP_REPLY_MSG_BYTES:
            sprintf(Int64Val, "%ld", rmsStatsItems_[i].int64_value);
            intSize = str_len(Int64Val);
            AddCommas(Int64Val,intSize); 
            sprintf(statsBuf_, "%-30s%s", "SSCP Reply Message Bytes", Int64Val);
            break;
          case SQLSTATS_RMS_STATS_RESET_TIMESTAMP:
	    jtime = CONVERTTIMESTAMP(rmsStatsItems_[i].int64_value,0,-1,NULL);
	    INTERPRETTIMESTAMP(jtime, timestamp);
	    sprintf(statsBuf_, "%-30s%04d/%02d/%02d %02d:%02d:%02d.%03u%03u",
                        "RMS Stats Reset Timestamp ",
			timestamp[0], timestamp[1], timestamp[2],
			timestamp[3], timestamp[4], timestamp[5],
			timestamp[6], timestamp[7]);
	    break;
          case SQLSTATS_RMS_CONFIGURED_PID_MAX:
            sprintf(Int64Val, "%ld", rmsStatsItems_[i].int64_value);
            sprintf(statsBuf_, "%-30s%s", "Configured Pid Max", Int64Val);
            break;
          }
          if (strlen(statsBuf_) > 0)
            if (moveRowToUpQueue(statsBuf_, strlen(statsBuf_), &rc) == -1)
              return rc;
        }
        // Leave a blank line between two CPUs RMS stats
        if (moveRowToUpQueue(" ", strlen(" "), &rc) == -1)
          return rc;
        step_ = GET_NEXT_STATS_DESC_ENTRY_;
      }
      break;
    case GET_UDR_BASE_STATS_ENTRY_:
     {
        if (udrbaseStatsItems_ == NULL)
        {
          maxUDRBaseStatsItems_ = 8;
          udrbaseStatsItems_ = new (getGlobals()->getDefaultHeap()) 
                  SQLSTATS_ITEM[maxUDRBaseStatsItems_];
          initSqlStatsItems(udrbaseStatsItems_, maxUDRBaseStatsItems_, FALSE);
          udrbaseStatsItems_[0].statsItem_id = SQLSTATS_TDB_ID;
          udrbaseStatsItems_[1].statsItem_id = SQLSTATS_UDR_CPU_BUSY_TIME;
          udrbaseStatsItems_[2].statsItem_id = SQLSTATS_RECENT_REQ_TS;
          udrbaseStatsItems_[3].statsItem_id = SQLSTATS_RECENT_REPLY_TS;
          udrbaseStatsItems_[4].statsItem_id = SQLSTATS_REQ_MSG_CNT;
          udrbaseStatsItems_[5].statsItem_id = SQLSTATS_REQ_MSG_BYTES;
          udrbaseStatsItems_[6].statsItem_id = SQLSTATS_REPLY_MSG_CNT;
          udrbaseStatsItems_[7].statsItem_id = SQLSTATS_REPLY_MSG_BYTES;
          // maxUDRBaseStatsItems_ is set to 8
        }
        else
          initSqlStatsItems(udrbaseStatsItems_, maxUDRBaseStatsItems_, TRUE);
  
        cliRC = SQL_EXEC_GetStatisticsItems(getStatsTdb().statsReqType_,
                getStatsTdb().stmtName_,
                getStatsTdb().stmtName_ ? str_len(getStatsTdb().stmtName_) : 0,
                maxUDRBaseStatsItems_,
                udrbaseStatsItems_);
        if (cliRC < 0)
        {
          step_ = HANDLE_ERROR_;
        }
        else
        {
          if (! isUDRBaseHeadingDisplayed_)
            step_ = DISPLAY_UDR_BASE_STATS_HEADING_;
          else
            step_ = FORMAT_AND_RETURN_UDR_BASE_STATS_;
        }
      }
      break;
    case DISPLAY_UDR_BASE_STATS_HEADING_:
      {
        if ((qparent_.up->getSize() - qparent_.up->getLength()) < 3)
	      return WORK_CALL_AGAIN;
        moveRowToUpQueue(" ");
        sprintf(statsBuf_, "%5s%-21s%-27s%-27s",
                "   Id", "UDR CPU Busy Time", "Recent Req.Timestamp", "Recent Reply Timestamp");
        moveRowToUpQueue(statsBuf_);
	sprintf(statsBuf_, "%-20s%-20s%-20s%-20s",
		"Req. Msg count", "Req. Msg Bytes", "Reply Msg Count", "Reply Msg Bytes");
        moveRowToUpQueue(statsBuf_);
        isUDRBaseHeadingDisplayed_ = TRUE;
        step_ = FORMAT_AND_RETURN_UDR_BASE_STATS_;
      }
      break;
    case FORMAT_AND_RETURN_UDR_BASE_STATS_:
      {
        for (; currStatsItemEntry_ < maxUDRBaseStatsItems_; currStatsItemEntry_++)
        {
          i = (short)currStatsItemEntry_;
          if (udrbaseStatsItems_[i].error_code != 0)
            continue;
          switch (udrbaseStatsItems_[i].statsItem_id)
          {
          case SQLSTATS_TDB_ID:
            sprintf(statsBuf_, "%5ld", udrbaseStatsItems_[i].int64_value);
            break;
          case SQLSTATS_UDR_CPU_BUSY_TIME:
            formatWInt64( udrbaseStatsItems_[i], Int64Val);
            sprintf(&statsBuf_[strlen(statsBuf_)], "%-21s", Int64Val);
            break;
          case SQLSTATS_RECENT_REQ_TS:
            if (udrbaseStatsItems_[i].int64_value == -1)
            {
              sprintf(&statsBuf_[strlen(statsBuf_)], "%-27s", " ");
            }
            else
            {
	      jtime = CONVERTTIMESTAMP(udrbaseStatsItems_[i].int64_value,0,-1,NULL);
	      INTERPRETTIMESTAMP(jtime, timestamp);
	      sprintf(&statsBuf_[strlen(statsBuf_)], " %04d/%02d/%02d %02d:%02d:%02d.%03u%03u",
                          timestamp[0], timestamp[1], timestamp[2],
			  timestamp[3], timestamp[4], timestamp[5],
                          timestamp[6], timestamp[7]);
            }
            break;
          case SQLSTATS_RECENT_REPLY_TS:
            if (udrbaseStatsItems_[i].int64_value == -1)
            {
              sprintf(&statsBuf_[strlen(statsBuf_)], "%-27s", " ");
            }
            else
            {
	      jtime = CONVERTTIMESTAMP(udrbaseStatsItems_[i].int64_value,0,-1,NULL);
	      INTERPRETTIMESTAMP(jtime, timestamp);
	      sprintf(&statsBuf_[strlen(statsBuf_)], " %04d/%02d/%02d %02d:%02d:%02d.%03u%03u",
                          timestamp[0], timestamp[1], timestamp[2],
			  timestamp[3], timestamp[4], timestamp[5],
                          timestamp[6], timestamp[7]);
            }
            if (moveRowToUpQueue(statsBuf_, strlen(statsBuf_), &rc) == -1)
              return rc;
            break;
          case SQLSTATS_REQ_MSG_CNT:
            formatWInt64( udrbaseStatsItems_[i], Int64Val);
            sprintf(statsBuf_, "%-20s", Int64Val);
            break;
          case SQLSTATS_REQ_MSG_BYTES:
            formatWInt64( udrbaseStatsItems_[i], Int64Val);
            sprintf(&statsBuf_[strlen(statsBuf_)], "%-20s", Int64Val);
            break;
          case SQLSTATS_REPLY_MSG_CNT:
            formatWInt64( udrbaseStatsItems_[i], Int64Val);
            sprintf(&statsBuf_[strlen(statsBuf_)], "%-20s", Int64Val);
            break;
          case SQLSTATS_REPLY_MSG_BYTES:
            formatWInt64( udrbaseStatsItems_[i], Int64Val);
            sprintf(&statsBuf_[strlen(statsBuf_)], "%-20s", Int64Val);
            if (moveRowToUpQueue(statsBuf_, strlen(statsBuf_), &rc) == -1)
              return rc;
            break;
          default:
            break;
          }
        }
        step_ = GET_NEXT_STATS_DESC_ENTRY_;
      }
      break;
    case HANDLE_ERROR_:
      {
        // SQL_EXEC_GetStatistics2 CLI call populates the diagnostics area
        // in context directly. However, ExHandleErrors will push this
        // into queue entry. CLI layer populates from queue into context
        // causing the errors to be displayed twice. Hence clear
        // Context diagnostics area here
        ComDiagsArea *diagsArea = currContext->getDiagsArea();
        ExHandleErrors(qparent_,
			        pentry_down,
			        0,
			        getGlobals(),
			        (diagsArea->getNumber() > 0 ? diagsArea : NULL),
			        (ExeErrorCode)cliRC,
			        NULL,
			        NULL
			        );
        if (diagsArea->getNumber() > 0)
           diagsArea->clear();
        step_ = DONE_;
      }
      break;
    case DONE_:
      {
	if (qparent_.up->isFull())
	  return WORK_OK;

	// Return EOF.
	ex_queue_entry * up_entry = qparent_.up->getTailEntry();
	
	up_entry->upState.parentIndex = 
	  pentry_down->downState.parentIndex;
	
	up_entry->upState.setMatchNo(0);
	up_entry->upState.status = ex_queue::Q_NO_DATA;
	
	// insert into parent
	qparent_.up->insert();
	
	//	    pstate.matches_ = 0;
	qparent_.down->removeHead();
	
        step_ = INITIAL_;
	return WORK_OK;
      }
      break;
    default:
      break;
    }
  } // while
}

char *ExExeUtilGetRTSStatisticsTcb::formatTimestamp(char *buf, Int64 inTime)
{
    Int64 jtime;
    short timestamp[8];

    if (inTime == -1)
       sprintf(buf, "%s", "-1");
    else if (inTime == 0)
       sprintf(buf, "%ld", inTime);
    else
    {
       jtime = CONVERTTIMESTAMP(inTime,0,-1,NULL);
       INTERPRETTIMESTAMP(jtime, timestamp);
       sprintf(buf, "%04d/%02d/%02d %02d:%02d:%02d.%03u%03u",
               timestamp[0], timestamp[1], timestamp[2],
               timestamp[3], timestamp[4], timestamp[5],
               timestamp[6], timestamp[7]);
    }
    return buf;
}

char *ExExeUtilGetRTSStatisticsTcb::formatElapsedTime(char *buf, Int64 inTime)
{
   ULng32 sec = (ULng32) (inTime / 1000000);
   ULng32 usec = (ULng32) (inTime  % 1000000);
   ULng32 min = sec/60;
   sec = sec % 60;
   ULng32 hour = min/60;
   min = min % 60;

   sprintf (buf, "%4u:%02u:%02u.%06u",
                          hour, min, sec,usec);
   return buf;
}


////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExExeUtilGetRTSStatisticsTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilGetRTSStatisticsPrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExExeUtilGetRTSStatisticsPrivateState
/////////////////////////////////////////////////////////////////////////////
ExExeUtilGetRTSStatisticsPrivateState::ExExeUtilGetRTSStatisticsPrivateState()
{
}

ExExeUtilGetRTSStatisticsPrivateState::~ExExeUtilGetRTSStatisticsPrivateState()
{
};


ex_tcb * ExExeUtilGetProcessStatisticsTdb::build(ex_globals * glob)
{
  ex_tcb * exe_util_tcb = NULL;

  exe_util_tcb = new(glob->getSpace()) ExExeUtilGetProcessStatisticsTcb(*this, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

////////////////////////////////////////////////////////////////
//// Constructor for class ExExeUtilGetProcessStatisticsTcb
/////////////////////////////////////////////////////////////////
ExExeUtilGetProcessStatisticsTcb::ExExeUtilGetProcessStatisticsTcb(
     const ComTdbExeUtilGetProcessStatistics & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilGetStatisticsTcb( exe_util_tdb, glob)
{
  step_ = INITIAL_;

  statsArea_ = NULL;
  processStats_ = NULL;
}

short ExExeUtilGetProcessStatisticsTcb::work()
{
  Lng32 cliRC = 0;
  char outBuf[1024];
  short timestamp[8];
  Int64 juliantimestamp;

  // if no parent request, return
  if (qparent_.down->isEmpty())
     return WORK_OK;

  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
     return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ContextCli * currContext = masterGlob->getCliGlobals()->currContext();

  while (1)
  {
    switch (step_)
    {
      case INITIAL_:
       {
         statsArea_ = NULL;
         processStats_ = NULL;
         step_ = GET_PROCESS_STATS_AREA_;
       }
       break;
      case GET_PROCESS_STATS_AREA_:
       {
         cliRC = SQL_EXEC_GetStatisticsArea_Internal
            (getStatsTdb().statsReqType_,
             getStatsTdb().stmtName_,
             getStatsTdb().stmtName_ ? str_len(getStatsTdb().stmtName_) : 0,
             getStatsTdb().activeQueryNum_,
             getStatsTdb().statsMergeType_,
             statsArea_);

         if (cliRC < 0)
            step_ = HANDLE_ERROR_;
         else
            step_ = GET_PROCESS_STATS_ENTRY_;
       }
       break;
      case GET_PROCESS_STATS_ENTRY_:
       {
         processStats_ = (ExProcessStats *)
            ((ExStatisticsArea *)statsArea_)->get(ExOperStats::PROCESS_STATS,
                    _UNINITIALIZED_TDB_ID);
         if (processStats_ == NULL)
         {
            cliRC = -EXE_STAT_NOT_FOUND;
            step_ = HANDLE_ERROR_;
         }
         else
           step_ = FORMAT_AND_RETURN_PROCESS_STATS_;
       }
       break;
      case FORMAT_AND_RETURN_PROCESS_STATS_: 
       {
          if ((qparent_.up->getSize() - qparent_.up->getLength()) < 30)
             return -1;
          sprintf(outBuf, "Node Id:                      %d", 
             processStats_->getNid());
          moveRowToUpQueue(outBuf);

          sprintf(outBuf, "PID                           %d", 
             processStats_->getPid());
          moveRowToUpQueue(outBuf);

          if (processStats_->getStartTime() == -1)
           strcpy(outBuf, "Start Time                    -1");
          else
          {
	     juliantimestamp = 
	         CONVERTTIMESTAMP(processStats_->getStartTime(),0,-1,0);

	     INTERPRETTIMESTAMP(juliantimestamp, timestamp);
	  sprintf(outBuf, "Start Time                    %04d/%02d/%02d %02d:%02d:%02d.%03u%03u",
			timestamp[0], timestamp[1], timestamp[2],
			timestamp[3], timestamp[4], timestamp[5],
			timestamp[6], timestamp[7]);
          }
	  moveRowToUpQueue(outBuf);

          sprintf(outBuf, "EXE Memory Allocated          %ld MB", 
               (processStats_->getExeMemAlloc() >> 20));
          moveRowToUpQueue(outBuf);

          sprintf(outBuf, "EXE Memory Used High WM       %ld MB", 
               (processStats_->getExeMemHighWM() >> 20));
          moveRowToUpQueue(outBuf);

          sprintf(outBuf, "EXE Memory Used               %ld MB", 
               (processStats_->getExeMemUsed() >> 20));
          moveRowToUpQueue(outBuf);

          sprintf(outBuf, "IPC Memory Allocated          %ld MB", 
               (processStats_->getIpcMemAlloc() >> 20));
          moveRowToUpQueue(outBuf);

          sprintf(outBuf, "IPC Memory Used High WM       %ld MB", 
               (processStats_->getIpcMemHighWM() >> 20));
          moveRowToUpQueue(outBuf);

          sprintf(outBuf, "IPC Memory Used               %ld MB", 
               (processStats_->getIpcMemUsed() >> 20));
          moveRowToUpQueue(outBuf);

          sprintf(outBuf, "Static Stmt Count             %d", 
               processStats_->getStaticStmtCount());
          moveRowToUpQueue(outBuf);

          sprintf(outBuf, "Dynamic Stmt Count            %d", 
               processStats_->getDynamicStmtCount());
          moveRowToUpQueue(outBuf);

          sprintf(outBuf, "Open Stmt Count               %d", 
               processStats_->getOpenStmtCount());
          moveRowToUpQueue(outBuf);

          sprintf(outBuf, "Reclaimable Stmt Count        %d", 
               processStats_->getCloseStmtCount());
          moveRowToUpQueue(outBuf);

          sprintf(outBuf, "Reclaimed Stmt Count          %d", 
               processStats_->getReclaimStmtCount());
          moveRowToUpQueue(outBuf);

          sprintf(outBuf, "Total ESPs Started            %d", 
               processStats_->getNumESPsStarted());
          moveRowToUpQueue(outBuf);

          sprintf(outBuf, "Total ESPs Startup Completed  %d", 
               processStats_->getNumESPsStartupCompleted());
          moveRowToUpQueue(outBuf);

          sprintf(outBuf, "Total ESPs Error in Startup   %d", 
               processStats_->getNumESPsBad());
          moveRowToUpQueue(outBuf);

          sprintf(outBuf, "Total ESPs Deleted            %d", 
               processStats_->getNumESPsDeleted());
          moveRowToUpQueue(outBuf);

          sprintf(outBuf, "Num ESPs In Use               %d", 
               processStats_->getNumESPsInUse());
          moveRowToUpQueue(outBuf);

          sprintf(outBuf, "Num ESPs Free                 %d", 
               processStats_->getNumESPsFree());
          moveRowToUpQueue(outBuf);

          sprintf(outBuf, "Recent Qid                    %s", 
              ( processStats_->getRecentQid() ?
                processStats_->getRecentQid() : "NULL"));
          moveRowToUpQueue(outBuf);

          step_ = DONE_;
       }
       break;
     case HANDLE_ERROR_:         
       {
         ExHandleErrors(qparent_,
               pentry_down,
               0,
               getGlobals(),
               NULL,
               (ExeErrorCode)cliRC,
               NULL,
               NULL
             );
         step_ = DONE_;
       }
       break;
     case DONE_:
       {
         if (qparent_.up->isFull())
            return WORK_OK;
	 // Return EOF.
	 ex_queue_entry * up_entry = qparent_.up->getTailEntry();
	 up_entry->upState.parentIndex = 
	      pentry_down->downState.parentIndex;
	 up_entry->upState.setMatchNo(0);
	 up_entry->upState.status = ex_queue::Q_NO_DATA;
	 // insert into parent
	 qparent_.up->insert();
	 qparent_.down->removeHead();
	 step_ = INITIAL_;
	 return WORK_OK;
       }
       break;
     default:
       break;
    }
  }
}

     

