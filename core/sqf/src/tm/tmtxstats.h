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

#ifndef TMTXSTATS_H_
#define TMTXSTATS_H_

#include <sys/types.h>
#include <sys/time.h>
#include "tmtimer.h"
#include "tmstats.h"

// CTmTxStats class definition
// This class encapsluates Transaction specific statistics.
class CTmTxStats
{
private:
   bool iv_collectStats;
   int32 iv_collectInterval;
   CdblTime iv_lastReadTime;

   CStatsTimer iv_txnTotal;
   CStatsTimer iv_txnBegin;
   CStatsTimer iv_txnAbort;
   CStatsTimer iv_txnCommit;
   CStatsTimer iv_RMSend;
   CStatsTimer iv_ax_reg;
   CStatsTimer iv_xa_start;
   CStatsTimer iv_xa_end;
   CStatsTimer iv_xa_prepare;
   CStatsTimer iv_xa_commit;
   CStatsTimer iv_xa_rollback;

   CStatsTimerTotal iv_RMSendTotal;
   CStatsTimerTotal iv_ax_regTotal;
   int32 iv_RMParticCount;
   int32 iv_RMNonParticCount;

public:
   CTmTxStats();
   CTmTxStats(bool pv_collectStats, int32 pv_collectInterval);
   ~CTmTxStats();

   bool collectStats() {return iv_collectStats;}
   CStatsTimer *txnTotal() {return &iv_txnTotal;}
   CStatsTimer *txnBegin() {return &iv_txnBegin;}
   CStatsTimer *txnAbort() {return &iv_txnAbort;}
   CStatsTimer *txnCommit() {return &iv_txnCommit;}
   CStatsTimer *RMSend() {return &iv_RMSend;}
   CStatsTimer *ax_reg() {return &iv_ax_reg;}
   CStatsTimer *xa_start() {return &iv_xa_start;}
   CStatsTimer *xa_end() {return &iv_xa_end;}
   CStatsTimer *xa_prepare() {return &iv_xa_prepare;}
   CStatsTimer *xa_commit() {return &iv_xa_commit;}
   CStatsTimer *xa_rollback() {return &iv_xa_rollback;}

   CStatsTimerTotal *RMSendTotal() {return &iv_RMSendTotal;}
   CStatsTimerTotal *ax_regTotal() {return &iv_ax_regTotal;}
   int32 RMParticCount() {return iv_RMParticCount;}
   int32 RMNonParticCount() {return iv_RMNonParticCount;}
   void inc_RMParticCount();
   void inc_RMNonParticCount();

   void initialize(bool pv_collectStats, int32 pv_collectInterval);
   void clearCounters();
   void RMSend_stop()
   {
      iv_RMSend.stop();
      iv_RMSendTotal.add(&iv_RMSend);
   }
   void ax_reg_stop()
   {
      iv_ax_reg.stop();
      iv_ax_regTotal.add(&iv_ax_reg);
   }

}; //class CTmStats

#endif //TMTXSTATS_H_
