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
#include "tminfo.h"
#include "tmstats.h"


//----------------------------------------------------------------------------
// CTmTxStats methods
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CTmTxStats Constructor 1
//----------------------------------------------------------------------------
CTmTxStats::CTmTxStats()
{
   initialize(gv_tm_info.stats()->collectStats(), 
              gv_tm_info.stats()->collectInterval());
   clearCounters();
   iv_RMParticCount = 0;
   iv_RMNonParticCount = 0;
}


//----------------------------------------------------------------------------
// CTmTxStats Constructor 2
//----------------------------------------------------------------------------
CTmTxStats::CTmTxStats(bool pv_collectStats, int32 pv_collectInterval)
{
   initialize(pv_collectStats, pv_collectInterval);
   clearCounters();
   iv_RMParticCount = 0;
   iv_RMNonParticCount = 0;
}


//----------------------------------------------------------------------------
// CTmTxStats Destructor
CTmTxStats::~CTmTxStats()
{
}


//----------------------------------------------------------------------------
// CTmTxStats::initialize
//----------------------------------------------------------------------------
void CTmTxStats::initialize(bool pv_collectStats, int32 pv_collectInterval)
{
   iv_collectStats = pv_collectStats;
   iv_collectInterval = pv_collectInterval;
} //CTmTxStats::initialize


//----------------------------------------------------------------------------
// CTmTxStats::clearCounters
//----------------------------------------------------------------------------
void CTmTxStats::clearCounters()
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

   iv_RMSendTotal.reset();
} //CTmTxStats::clearCounters


//----------------------------------------------------------------------------
// CTmTxStats::inc_RMParticCount
//----------------------------------------------------------------------------
void CTmTxStats::inc_RMParticCount() 
{
   if (TM_STATS)
      iv_RMParticCount++;
}


//----------------------------------------------------------------------------
// CTmTxStats::inc_RMNonParticCount
//----------------------------------------------------------------------------
void CTmTxStats::inc_RMNonParticCount()
{
   if (TM_STATS)
      iv_RMNonParticCount++;
}
