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

// +++ T2_REPO
#include <platform_ndcs.h>
//

#include "ResStatistics.h"
//#include "srvrcommon.h"   +++ T2_REPO
#include <tr1/memory>

struct passSession
{
    long long                               sqlExecutionTime;
    long long                               odbcElapseTime;
    long long                               sqlElapseTime;
    long long                               odbcExecutionTime;
    long                                    state;
    char                                    statementId[MAX_STMT_LABEL_LEN + 1];
    short                                   stmtType;
    Int32                                   sqlNewQueryType;
    long                                    errorStatement;
    long                                    warningStatement;
};

typedef struct SESSION_STATS
{
      SESSION_STATS() :
              EstimatedRowsAccessed(0.0)
            , EstimatedRowsUsed(0.0)
            , StatsBytes(0)
            , NumRowsIUD(0)
            , RowsReturned(0)
            , SQLProcessBusyTime(0)
            , AQRnumRetries(0)
            , AQRdelayBeforeRetry(0)
            , NumberOfRows(0)
            , OpenBusyTime(0)
            , NumOpens(0)
            , ProcessesCreated(0)
            , ProcessCreateBusyTime(0)
            , RowsAccessed(0)
            , RowsRetrieved(0)
            , DiscProcessBusyTime(0)
            , DiscReads(0)
            , SpaceTotal(0)
            , SpaceUsed(0)
            , HeapTotal(0)
            , HeapUsed(0)
            , TotalMemory(0)
            , Dp2SpaceTotal(0)
            , Dp2SpaceUsed(0)
            , Dp2HeapTotal(0)
            , Dp2HeapUsed(0)
            , MsgsToDisc(0)
            , MsgsBytesToDisc(0)
            , NumRqstMsgs(0)
            , NumRqstMsgBytes(0)
            , NumRplyMsgs(0)
            , NumRplyMsgBytes(0)
            , LockWaits(0)
            , LockEscalation(0)
            , TotalExecutes(0)
            , TotalAggregates(0)
            , totalSelects(0)
            , totalInserts(0)
            , totalUpdates(0)
            , totalDeletes(0)
            , totalDDLs(0)
            , totalUtils(0)
            , totalCatalogs(0)
            , totalOthers(0)
            , totalInsertErrors(0)
            , totalUpdateErrors(0)
            , totalDeleteErrors(0)
            , totalSelectErrors(0)
            , totalDDLErrors(0)
            , totalUtilErrors(0)
            , totalCatalogErrors(0)
            , totalOtherErrors(0)
              {}

    void reset()
    {
        EstimatedRowsAccessed   = 0.0;
        EstimatedRowsUsed       = 0.0;
        StatsBytes              = 0;
        NumRowsIUD              = 0;
        RowsReturned            = 0;
        SQLProcessBusyTime      = 0;
        AQRnumRetries           = 0;
        AQRdelayBeforeRetry     = 0;
        NumberOfRows            = 0;
        OpenBusyTime            = 0;
        NumOpens                = 0;
        ProcessesCreated        = 0;
        ProcessCreateBusyTime   = 0;
        RowsAccessed            = 0;
        RowsRetrieved           = 0;
        DiscProcessBusyTime     = 0;
        DiscReads               = 0;
        SpaceTotal              = 0;
        SpaceUsed               = 0;
        HeapTotal               = 0;
        HeapUsed                = 0;
        TotalMemory             = 0;
        Dp2SpaceTotal           = 0;
        Dp2SpaceUsed            = 0;
        Dp2HeapTotal            = 0;
        Dp2HeapUsed             = 0;
        MsgsToDisc              = 0;
        MsgsBytesToDisc         = 0;
        NumRqstMsgs             = 0;
        NumRqstMsgBytes         = 0;
        NumRplyMsgs             = 0;
        NumRplyMsgBytes         = 0;
        LockWaits               = 0;
        LockEscalation          = 0;
        TotalExecutes           = 0;
        TotalAggregates         = 0;
        totalSelects            = 0;
        totalInserts            = 0;
        totalUpdates            = 0;
        totalDeletes            = 0;
        totalDDLs               = 0;
        totalUtils              = 0;
        totalCatalogs           = 0;
        totalOthers             = 0;
        totalInsertErrors       = 0;
        totalUpdateErrors       = 0;
        totalDeleteErrors       = 0;
        totalSelectErrors       = 0;
        totalDDLErrors          = 0;
        totalUtilErrors         = 0;
        totalCatalogErrors      = 0;
        totalOtherErrors        = 0;
    }

    SESSION_STATS& operator=(const SESSION_STATS& rhs)
    {
        if (this == &rhs)
            return *this;

        EstimatedRowsAccessed   = rhs.EstimatedRowsAccessed;
        EstimatedRowsUsed       = rhs.EstimatedRowsUsed;
        StatsBytes              = rhs.StatsBytes;
        NumRowsIUD              = rhs.NumRowsIUD;
        RowsReturned            = rhs.RowsReturned;
        SQLProcessBusyTime      = rhs.SQLProcessBusyTime;
        AQRnumRetries           = rhs.AQRnumRetries;
        AQRdelayBeforeRetry     = rhs.AQRdelayBeforeRetry;
        NumberOfRows            = rhs.NumberOfRows;
        OpenBusyTime            = rhs.OpenBusyTime;
        NumOpens                = rhs.NumOpens;
        ProcessesCreated        = rhs.ProcessesCreated;
        ProcessCreateBusyTime   = rhs.ProcessCreateBusyTime;
        RowsAccessed            = rhs.RowsAccessed;
        RowsRetrieved           = rhs.RowsRetrieved;
        DiscProcessBusyTime     = rhs.DiscProcessBusyTime;
        DiscReads               = rhs.DiscReads;
        SpaceTotal              = rhs.SpaceTotal;
        SpaceUsed               = rhs.SpaceUsed;
        HeapTotal               = rhs.HeapTotal;
        HeapUsed                = rhs.HeapUsed;
        TotalMemory             = rhs.TotalMemory;
        Dp2SpaceTotal           = rhs.Dp2SpaceTotal;
        Dp2SpaceUsed            = rhs.Dp2SpaceUsed;
        Dp2HeapTotal            = rhs.Dp2HeapTotal;
        Dp2HeapUsed             = rhs.Dp2HeapUsed;
        MsgsToDisc              = rhs.MsgsToDisc;
        MsgsBytesToDisc         = rhs.MsgsBytesToDisc;
        NumRqstMsgs             = rhs.NumRqstMsgs;
        NumRqstMsgBytes         = rhs.NumRqstMsgBytes;
        NumRplyMsgs             = rhs.NumRplyMsgs;
        NumRplyMsgBytes         = rhs.NumRplyMsgBytes;
        LockWaits               = rhs.LockWaits;
        LockEscalation          = rhs.LockEscalation;
        TotalExecutes           = rhs.TotalExecutes;
        TotalAggregates         = rhs.TotalAggregates;
        totalSelects            = rhs.totalSelects;
        totalInserts            = rhs.totalInserts;
        totalUpdates            = rhs.totalUpdates;
        totalDeletes            = rhs.totalDeletes;
        totalDDLs               = rhs.totalDDLs;
        totalUtils              = rhs.totalUtils;
        totalCatalogs           = rhs.totalCatalogs;
        totalOthers             = rhs.totalOthers;
        totalInsertErrors       = rhs.totalInsertErrors;
        totalUpdateErrors       = rhs.totalUpdateErrors;
        totalDeleteErrors       = rhs.totalDeleteErrors;
        totalSelectErrors       = rhs.totalSelectErrors;
        totalDDLErrors          = rhs.totalDDLErrors;
        totalUtilErrors         = rhs.totalUtilErrors;
        totalCatalogErrors      = rhs.totalCatalogErrors;
        totalOtherErrors        = rhs.totalOtherErrors;

        return *this;
    }



    SESSION_STATS& computeDelta(const SESSION_STATS& cur, const SESSION_STATS& last)
    {
        EstimatedRowsAccessed   = cur.EstimatedRowsAccessed - last.EstimatedRowsAccessed;
        EstimatedRowsUsed       = cur.EstimatedRowsUsed - last.EstimatedRowsUsed;
        StatsBytes              = cur.StatsBytes - last.StatsBytes;
        NumRowsIUD              = cur.NumRowsIUD - last.NumRowsIUD;
        RowsReturned            = cur.RowsReturned - last.RowsReturned;
        SQLProcessBusyTime      = cur.SQLProcessBusyTime - last.SQLProcessBusyTime;
        AQRnumRetries           = cur.AQRnumRetries - last.AQRnumRetries;
        AQRdelayBeforeRetry     = cur.AQRdelayBeforeRetry - last.AQRdelayBeforeRetry;
        NumberOfRows            = cur.NumberOfRows - last.NumberOfRows;
        OpenBusyTime            = cur.OpenBusyTime - last.OpenBusyTime;
        NumOpens                = cur.NumOpens - last.NumOpens;
        ProcessesCreated        = cur.ProcessesCreated - last.ProcessesCreated;
        ProcessCreateBusyTime   = cur.ProcessCreateBusyTime - last.ProcessCreateBusyTime;
        RowsAccessed            = cur.RowsAccessed - last.RowsAccessed;
        RowsRetrieved           = cur.RowsRetrieved - last.RowsRetrieved;
        DiscProcessBusyTime     = cur.DiscProcessBusyTime - last.DiscProcessBusyTime;
        DiscReads               = cur.DiscReads - last.DiscReads;
        SpaceTotal              = cur.SpaceTotal - last.SpaceTotal;
        SpaceUsed               = cur.SpaceUsed - last.SpaceUsed;
        HeapTotal               = cur.HeapTotal - last.HeapTotal;
        HeapUsed                = cur.HeapUsed - last.HeapUsed;
        TotalMemory             = cur.TotalMemory - last.TotalMemory;
        Dp2SpaceTotal           = cur.Dp2SpaceTotal - last.Dp2SpaceTotal;
        Dp2SpaceUsed            = cur.Dp2SpaceUsed - last.Dp2SpaceUsed;
        Dp2HeapTotal            = cur.Dp2HeapTotal - last.Dp2HeapTotal;
        Dp2HeapUsed             = cur.Dp2HeapUsed - last.Dp2HeapUsed;
        MsgsToDisc              = cur.MsgsToDisc - last.MsgsToDisc;
        MsgsBytesToDisc         = cur.MsgsBytesToDisc - last.MsgsBytesToDisc;
        NumRqstMsgs             = cur.NumRqstMsgs - last.NumRqstMsgs;
        NumRqstMsgBytes         = cur.NumRqstMsgBytes - last.NumRqstMsgBytes;
        NumRplyMsgs             = cur.NumRplyMsgs - last.NumRplyMsgs;
        NumRplyMsgBytes         = cur.NumRplyMsgBytes - last.NumRplyMsgBytes;
        LockWaits               = cur.LockWaits - last.LockWaits;
        LockEscalation          = cur.LockEscalation - last.LockEscalation;
        TotalExecutes           = cur.TotalExecutes - last.TotalExecutes;
        TotalAggregates         = cur.TotalAggregates - last.TotalAggregates;
        totalSelects            = cur.totalSelects - last.totalSelects;
        totalInserts            = cur.totalInserts - last.totalInserts;
        totalUpdates            = cur.totalUpdates - last.totalUpdates;
        totalDeletes            = cur.totalDeletes - last.totalDeletes;
        totalDDLs               = cur.totalDDLs - last.totalDDLs;
        totalUtils              = cur.totalUtils - last.totalUtils;
        totalCatalogs           = cur.totalCatalogs - last.totalCatalogs;
        totalOthers             = cur.totalOthers - last.totalOthers;
        totalInsertErrors       = cur.totalInsertErrors - last.totalInsertErrors;
        totalUpdateErrors       = cur.totalUpdateErrors - last.totalUpdateErrors;
        totalDeleteErrors       = cur.totalDeleteErrors - last.totalDeleteErrors;
        totalSelectErrors       = cur.totalSelectErrors - last.totalSelectErrors;
        totalDDLErrors          = cur.totalDDLErrors - last.totalDDLErrors;
        totalUtilErrors         = cur.totalUtilErrors - last.totalUtilErrors;
        totalCatalogErrors      = cur.totalCatalogErrors - last.totalCatalogErrors;
        totalOtherErrors        = cur.totalOtherErrors - last.totalOtherErrors;

        return *this;
    }

    int64 LastUpdate;

    double EstimatedRowsAccessed;   //AGGREG    estRowsAccessed
    double EstimatedRowsUsed;       //AGGREG    estRowsUsed
    int64 StatsBytes;               //AGGREG    statsBytes
    int64 NumRowsIUD;               //AGGREG
    int64 RowsReturned;             //AGGREG    rowsReturned
    int64 SQLProcessBusyTime;       //AGGREG    ProcessBusyTime
    int64 AQRnumRetries;            //AGGREG    AQRnumRetries
    int64 AQRdelayBeforeRetry;      //AGGREG    AQRdelayBeforeRetry
    int64 NumberOfRows;             //AGGREG    numberOfRows
    int64 OpenBusyTime;             //AGGREG    OpenTime
    int64 NumOpens;                 //AGGREG    Opens
    int64 ProcessesCreated;         //AGGREG    NewProcess
    int64 ProcessCreateBusyTime;    //AGGREG    NewProcessTime
    int64 RowsAccessed;             //AGGREG    AccessedRows
    int64 RowsRetrieved;            //AGGREG    UsedRows
    int64 DiscProcessBusyTime;      //AGGREG    DiskProcessBusyTime
    int64 DiscReads;                //AGGREG    DiskIOs
    int64 SpaceTotal;               //AGGREG    SpaceTotal
    int64 SpaceUsed;                //AGGREG    SpaceUsed
    int64 HeapTotal;                //AGGREG    HeapTotal
    int64 HeapUsed;                 //AGGREG    HeapUsed
    int64 TotalMemory;              //AGGREG    TotalMemAlloc
    int64 Dp2SpaceTotal;            //AGGREG    Dp2SpaceTotal
    int64 Dp2SpaceUsed;             //AGGREG    Dp2SpaceUsed
    int64 Dp2HeapTotal;             //AGGREG    Dp2HeapTotal
    int64 Dp2HeapUsed;              //AGGREG    Dp2HeapUsed
    int64 MsgsToDisc;               //AGGREG    NumMessages
    int64 MsgsBytesToDisc;          //AGGREG    MessagesBytes
    int64 NumRqstMsgs;              //AGGREG    reqMsgCnt
    int64 NumRqstMsgBytes;          //AGGREG    reqMsgBytes
    int64 NumRplyMsgs;              //AGGREG    replyMsgCnt
    int64 NumRplyMsgBytes;          //AGGREG    replyMsgBytes
    int64 LockWaits;                //AGGREG    LockWaits
    int64 LockEscalation;           //AGGREG    Escalations
    int64 TotalExecutes;            //AGGREG    totalStatementExecutes
    int64 TotalAggregates;
    int64 totalSelects;
    int64 totalInserts;
    int64 totalUpdates;
    int64 totalDeletes;
    int64 totalDDLs;
    int64 totalUtils;
    int64 totalCatalogs;
    int64 totalOthers;
    int64 totalInsertErrors;
    int64 totalUpdateErrors;
    int64 totalDeleteErrors;
    int64 totalSelectErrors;
    int64 totalDDLErrors;
    int64 totalUtilErrors;
    int64 totalCatalogErrors;
    int64 totalOtherErrors;

} SessionStats;

class SessionWlStats
{
public:
    SessionStats    aggrStats;
    SessionStats    deltaStats;
    SessionStats    lastStats;

    SessionWlStats()
    {
        deltaStats.reset();
        aggrStats.reset();
        lastStats.reset();
    }

    // Needs to be called before sending delta stats to WMS
    void computeDeltaStats()
    {
        deltaStats.reset();
        deltaStats.computeDelta(aggrStats, lastStats);
        lastStats.reset();
        lastStats = aggrStats;
    }

};

 class ResStatisticsSession:public ResStatistics
 {

private:

 //  Session variables
        char                                    startTime[25];
        char                                    endTime[25];
        struct tm                               * startTimeInfo;
        struct tm                               * endTimeInfo;
        long long                               totalSqlExecutionTime;
        long long                               totalOdbcElapseTime;
        long long                               totalSqlElapseTime;
        long long                               totalOdbcExecutionTime;
        long                                    totalInsertStatements;
        long                                    totalDeleteStatements;
        long                                    totalUpdateStatements;
        long                                    totalSelectStatements;
        long                                    totalDDLStatements;
        long                                    totalUtilStatements;
        long                                    totalOtherStatements;
        long                                    totalInsertErrors;
        long                                    totalUpdateErrors;
        long                                    totalDeleteErrors;
        long                                    totalSelectErrors;
        long                                    totalDDLErrors;
        long                                    totalUtilErrors;
        long                                    totalOtherErrors;

        long                                    totalErrors;
        long                                    totalWarnings;
        long                                    totalPrepares;
        long                                    totalExecutes;
        long                                    totalFetches;
        long                                    totalCloses;
        long                                    totalExecDirects;

        long long                               startTime_ts;
        long long                               entryTime_ts;
        long long                               endTime_ts;
public:
        BOOL                                    logonFlag;
        int                                     totalCatalogStatements ;
        long                                    totalCatalogErrors ;
        long                                    totalCatalogWarnings ;

        SessionWlStats                          sessWlStats;
public:
        void start(struct collect_info  *setInit);
        void end();
        void accumulateStatistics(passSession *ps);
        void accumulateStatistics(const ResStatistics * const pResStats);
        void update();
        void init();
        std::tr1::shared_ptr<SESSION_AGGREGATION> getAggrStats();
        ResStatisticsSession();
        ~ResStatisticsSession();
   };

#endif
