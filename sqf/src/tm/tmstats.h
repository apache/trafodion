// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@

#ifndef TMSTATS_H_
#define TMSTATS_H_

#include <sys/types.h>
#include <sys/time.h>
#include "tmtimer.h"


#ifdef XATM_LIB
#define TMSTATS_ON (gv_xaTM.tm_stats())
#define TMSTATS gv_xaTM.stats()
#else
#define TMSTATS_ON (gv_tm_info.tm_stats())
#define TMSTATS gv_tm_info.stats()
#endif


// Foward declarations
class CTmTxStats;

// CStatsTimer class definition
// This encapsulates the timer start and stop times
// for recording timed statistics.
class CStatsTimer
{
private:
   CdblTime iv_startTime;
   CdblTime iv_stopTime;

public:
   CStatsTimer() {reset();}
   ~CStatsTimer() {}

   void start();
   void stop();
   CdblTime read()
   {
      return iv_stopTime - iv_startTime;
   }
   void reset()
   {
      iv_startTime = 0;
      iv_stopTime = 0;
   }
}; // CStatsTimer


// CStatsTimerTotal class definition
// This class encapsulates the methods and data for totaling
// or accumulating timed values
class CStatsTimerTotal
{
private:
   int32 iv_count;
   CdblTime iv_total;
   CdblTime iv_totalSq;

public:
   CStatsTimerTotal()
   {
      reset();
   }
   ~CStatsTimerTotal() {}

   void reset()
   {
      iv_count = 0;
      iv_total = 0;
      iv_totalSq = 0;
   }

   TIMEDSTATS read()
   {
      TIMEDSTATS iv_stats;
      iv_stats.iv_count = iv_count;
      iv_stats.iv_total = iv_total.get();
      iv_stats.iv_totalSq = iv_totalSq.get();
      return iv_stats;
   }
   void add(CStatsTimer *pp_timer);
   void add(CStatsTimerTotal *pp_timerTotal);
}; //CStatsTimerTotal


// CTmStats class definition
// This class encapsluates the TMs internal statistics and methods for setting, 
// clearing and retrieving values.
class CTmStats
{
private:
   bool  iv_collectStats;        // True = collect stats.
                                 // Set by DTM_TM_STATS=1 registry value, default is TM_STATS.
   int32 iv_collectInterval;     // Interval for stats gathering. 
                                 // Registry value DTM_TM_STATS_INTERVAL in minutes.
   CdblTime iv_lastReadTime;
   CStatsTimerTotal iv_txnTotal;
   CStatsTimerTotal iv_txnBegin;
   CStatsTimerTotal iv_txnAbort;
   CStatsTimerTotal iv_txnCommit;
   CStatsTimerTotal iv_RMSend;
   CStatsTimerTotal iv_ax_reg;
   CStatsTimerTotal iv_xa_start;
   CStatsTimerTotal iv_xa_end;
   CStatsTimerTotal iv_xa_prepare;
   CStatsTimerTotal iv_xa_commit;
   CStatsTimerTotal iv_xa_rollback;
   int32 iv_RMParticCount;
   int32 iv_RMNonParticCount;

public:
   CTmStats(bool pv_collectStats, int32 pv_collectInterval);
   ~CTmStats();

   bool collectStats() {return iv_collectStats;}
   int32 collectInterval() {return iv_collectInterval;}

   CStatsTimerTotal *txnTotal() {return &iv_txnTotal;}
   CStatsTimerTotal *txnBegin() {return &iv_txnBegin;}
   CStatsTimerTotal *txnAbort() {return &iv_txnAbort;}
   CStatsTimerTotal *txnCommit() {return &iv_txnCommit;}
   CStatsTimerTotal *RMSend() {return &iv_RMSend;}
   CStatsTimerTotal *ax_reg() {return &iv_ax_reg;}
   CStatsTimerTotal *xa_start() {return &iv_xa_start;}
   CStatsTimerTotal *xa_end() {return &iv_xa_end;}
   CStatsTimerTotal *xa_prepare() {return &iv_xa_prepare;}
   CStatsTimerTotal *xa_commit() {return &iv_xa_commit;}
   CStatsTimerTotal *xa_rollback() {return &iv_xa_rollback;}
   int32 RMParticCount() {return iv_RMParticCount;}
   int32 RMNonParticCount() {return iv_RMNonParticCount;}
   
   void initialize(bool pv_collectStats, int32 pv_collectInterval);
   void clearCounters();
   void readStats(TM_TMSTATS *pp_stats);
   void addTxnCounters(CTmTxStats *pp_txnStats);

}; //class CTmStats

#endif //TMSTATS_H_
