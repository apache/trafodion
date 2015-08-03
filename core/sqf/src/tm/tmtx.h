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

#ifndef TMTX_H_
#define TMTX_H_

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "seabed/atomic.h"

#include "dtm/tmtransid.h"
#include "tmaudit.h"
#include "tmlibmsg.h"
#include "tmrm.h"
#include "tmmap.h"
#include "tmdeque.h"
#include "tmsync.h"
#include "tmtxmsg.h"
#include "tmtxkey.h"
#include "tmtimer.h"
#include "tmpoolelement.h"
#include "tmpool.h"
#include "tmtxstats.h"
#include "tmtxthread.h"
#include "tmtxbase.h"

// EID must be exactly 9 characters
const char EID_TM_TX_INFO[] = {"TM_TX_Info"}; 

//
// Transaction Representation.  There will be one of these for
// each transaction.  There is an array in the TM_Info that will store
// these.  Everything is public since this instance is stored 
// privately in the TM_Info class and that class can safely access this.
//
// For the XARM prototype this class is mostly implemented by CTmTxBase
//
class TM_TX_Info :public CTmTxBase
{
    protected:
       int t;

    public:
       TM_TX_Info(int32 pv_nid, int64 pv_flags, int32 pv_trace_level, int32 pv_seq, 
                  int32 pv_pid, int32 pv_rm_wait_time);
       TM_TX_Info() {}
       ~TM_TX_Info();

       static TM_TX_Info *constructPoolElement(int64 pv_seqnum);
       virtual void cleanup();

       virtual void initialize(int32 pv_nid, int64 pv_flags, int32 pv_trace_level, 
                               int32 pv_seq, int32 pv_creator_nid, int32 pv_creator_pid, 
                               int32 pv_rm_wait_time);


       virtual int32 state_change (TX_EVENT pv_event, 
                          int32 pv_nid, int32 pv_pid, CTmTxMessage * pp_msg);
       bool state_change_abort_helper(CTmTxMessage * pp_msg);
       void state_change_abort_set( CTmTxMessage * pp_msg, short pv_error);
       bool state_change_commit_helper(CTmTxMessage * pp_msg, bool pv_read_only = false);
       bool state_change_prepare_helper(CTmTxMessage * pp_msg);

       virtual int32 schedule_abort();
       virtual short doom_txn();

       // event specific methods
       virtual bool req_begin(CTmTxMessage *pp_msg);
       virtual bool req_begin_complete(CTmTxMessage *pp_msg = NULL);
       virtual bool req_end(CTmTxMessage *pp_msg);
       virtual bool req_end_complete(CTmTxMessage *pp_msg = NULL);
       virtual bool req_forget(CTmTxMessage *pp_msg = NULL);
       virtual bool req_abort(CTmTxMessage *pp_msg);
       virtual bool req_abort_complete(CTmTxMessage *pp_msg = NULL);
       virtual bool req_ddloperation(CTmTxMessage *pp_msg);
       //virtual bool req_status(CTmTxMessage *pp_msg);
       //virtual bool req_ax_reg(CTmTxMessage *pp_msg);
       //virtual bool req_join(CTmTxMessage *pp_msg);
       //virtual bool req_suspend(CTmTxMessage *pp_msg);
       virtual bool rollback_txn(CTmTxMessage *pp_msg);
       virtual bool redriverollback_txn(CTmTxMessage *pp_msg);
       virtual bool redrivecommit_txn(CTmTxMessage *pp_msg);
       bool redrive_sync(CTmTxMessage *pp_msg);

       // No mapping for TM Transactions
       virtual int mapErr(short pv_tmError) {return pv_tmError;}

       virtual int32 internal_abortTrans(bool pv_takeover = false);
       virtual int32 redrive_rollback();
       virtual int32 redrive_commit();

       virtual void process_eventQ(); 
       virtual void schedule_eventQ();
};

#endif //TMTX_H_




