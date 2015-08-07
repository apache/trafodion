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

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "seabed/ms.h"
#include "tminfo.h"
#include "tmtx.h"
#include "seabed/trace.h"
#include "tmlogging.h"
#include "tminfo.h"
#include "tmtimer.h"


//----------------------------------------------------------------------------
// CTmStatsTimer methods
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CStatsTimer::start
//----------------------------------------------------------------------------
void CStatsTimer::start()
{
   if (TMSTATS_ON)
      iv_startTime = Ctimeval::now();
}

//----------------------------------------------------------------------------
// CStatsTimer::stop
//----------------------------------------------------------------------------
void CStatsTimer::stop()
{
   if (TMSTATS_ON)
      iv_stopTime = Ctimeval::now();
}


//----------------------------------------------------------------------------
// CTmStatsTimerTotal methods
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CStatsTimerTotal::add
// Purpose : Add a timer to this total
//----------------------------------------------------------------------------
void CStatsTimerTotal::add(CStatsTimer *pp_timer)
{
   if (!TMSTATS_ON)
      return;

   if (pp_timer->read() != 0)
   {
      iv_count++;
      iv_total += pp_timer->read();
      iv_totalSq += (pp_timer->read() * pp_timer->read());
   }
} //CStatsTimerTotal::add


//----------------------------------------------------------------------------
// CStatsTimerTotal::add (2)
// Purpose : Add a two totals together
//----------------------------------------------------------------------------
void CStatsTimerTotal::add(CStatsTimerTotal *pp_timerTotal)
{
   if (!TMSTATS_ON)
      return;

   iv_count += pp_timerTotal->iv_count;
   iv_total += pp_timerTotal->iv_total;
   iv_totalSq += pp_timerTotal->iv_totalSq;
} //CStatsTimerTotal::add (2)


//----------------------------------------------------------------------------
// CTmStats methods
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CTmStats Constructor
//----------------------------------------------------------------------------
CTmStats::CTmStats(bool pv_collectStats, int32 pv_collectInterval) : iv_RMParticCount(0), 
                                                                     iv_RMNonParticCount(0)
{
   initialize(pv_collectStats, pv_collectInterval);
}


//----------------------------------------------------------------------------
// CTmStats Destructor
CTmStats::~CTmStats()
{
}


//----------------------------------------------------------------------------
// CTmStats::initialize
//----------------------------------------------------------------------------
void CTmStats::initialize(bool pv_collectStats, int32 pv_collectInterval)
{
   iv_collectStats = pv_collectStats;
   iv_collectInterval = pv_collectInterval;
} //CTmStats::initialize


//----------------------------------------------------------------------------
// CTmStats::clearCounters
//----------------------------------------------------------------------------
void CTmStats::clearCounters()
{
   iv_lastReadTime = Ctimeval::now();
   iv_txnTotal.reset();
   iv_txnBegin.reset();
   iv_txnAbort.reset();
   iv_txnCommit.reset();
   iv_RMSend.reset();
   iv_ax_reg.reset();
   iv_xa_start.reset();
   iv_xa_end.reset();
   iv_xa_prepare.reset();
   iv_xa_commit.reset();
   iv_xa_rollback.reset();
   iv_RMParticCount = 0;
   iv_RMNonParticCount = 0;

   gv_tm_info.threadPool()->resetCounters();
   gv_tm_info.transactionPool()->resetCounters();
   xaTM_RMMessagePool()->resetCounters();
} //CTmStats::clearCounters


//----------------------------------------------------------------------------
// CTmStats::readStats
// Read the current stats counters
//----------------------------------------------------------------------------
void CTmStats::readStats(TM_TMSTATS *pp_stats)
{
   pp_stats->iv_node = gv_tm_info.nid();
   CdblTime lv_start(gv_tm_info.tmTimer()->startTime());
   CdblTime lv_sent(Ctimeval::now());
   pp_stats->iv_tmStartTime = lv_start.get();
   pp_stats->iv_statsSentTime = lv_sent.get();

   pp_stats->iv_counts.iv_tx_count = gv_tm_info.tx_count();
   pp_stats->iv_counts.iv_begin_count = gv_tm_info.begin_count();
   pp_stats->iv_counts.iv_abort_count = gv_tm_info.abort_count();
   pp_stats->iv_counts.iv_commit_count = gv_tm_info.commit_count();
   pp_stats->iv_counts.iv_current_tx_count = gv_tm_info.current_tx_count();
   pp_stats->iv_counts.iv_tm_initiated_aborts = gv_tm_info.tm_initiated_aborts();
   pp_stats->iv_counts.iv_tx_hung_count = gv_tm_info.tx_hung_count();
   pp_stats->iv_counts.iv_current_tx_hung_count = gv_tm_info.current_tx_hung_count();

   pp_stats->iv_txn.iv_txnTotal = iv_txnTotal.read();
   pp_stats->iv_txn.iv_RMSend = iv_RMSend.read();
   pp_stats->iv_txn.iv_txnBegin = iv_txnBegin.read();
   pp_stats->iv_txn.iv_txnCommit = iv_txnCommit.read();
   pp_stats->iv_txn.iv_txnAbort = iv_txnAbort.read();
   pp_stats->iv_txn.iv_ax_reg = iv_ax_reg.read();
   pp_stats->iv_txn.iv_xa_start = iv_xa_start.read();
   pp_stats->iv_txn.iv_xa_end = iv_xa_end.read();
   pp_stats->iv_txn.iv_xa_prepare = iv_xa_prepare.read();
   pp_stats->iv_txn.iv_xa_commit = iv_xa_commit.read();
   pp_stats->iv_txn.iv_xa_rollback = iv_xa_rollback.read();
   pp_stats->iv_txn.iv_RMParticCount = iv_RMParticCount;
   pp_stats->iv_txn.iv_RMNonParticCount = iv_RMNonParticCount;

   gv_tm_info.threadPool()->getPoolStats(&pp_stats->iv_threadPool_stats);
   gv_tm_info.transactionPool()->getPoolStats(&pp_stats->iv_transactionPool_stats);
   xaTM_RMMessagePool()->getPoolStats(&pp_stats->iv_RMMessagePool_stats);
} //CTmStats::readStats


//----------------------------------------------------------------------------
// CTmStats::addTxnCounters
// Add the counters for a transaction into the transaction totals and clear
// the values.
//----------------------------------------------------------------------------
void CTmStats::addTxnCounters(CTmTxStats *pp_txnStats)
{
   iv_txnTotal.add(pp_txnStats->txnTotal());
   iv_txnBegin.add(pp_txnStats->txnBegin());
   iv_txnAbort.add(pp_txnStats->txnAbort());
   iv_txnCommit.add(pp_txnStats->txnCommit());
   iv_RMSend.add(pp_txnStats->RMSendTotal());
   iv_ax_reg.add(pp_txnStats->ax_regTotal());
   iv_xa_start.add(pp_txnStats->xa_start());
   iv_xa_end.add(pp_txnStats->xa_end());
   iv_xa_prepare.add(pp_txnStats->xa_prepare());
   iv_xa_commit.add(pp_txnStats->xa_commit());
   iv_xa_rollback.add(pp_txnStats->xa_rollback());
   iv_RMParticCount += pp_txnStats->RMParticCount();
   iv_RMNonParticCount += pp_txnStats->RMNonParticCount();

   // Clear the counters
   pp_txnStats->clearCounters();
} //CTmStats::addTxnCounters
