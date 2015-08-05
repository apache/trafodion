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

#ifndef CXaTxn_H_
#define CXaTxn_H_

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "seabed/atomic.h"

#include "dtm/tmtransid.h"
#include "xa.h"
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
const char EID_CTmXaTxn[] = {"CTmXaTxn  "}; 

//
// XARM Subordinate branch Representation.  There will be one of these for
// each XA branch created by an external superior TM.  There is an array 
// in the TM_Info that stores 
// these.  Everything is public since this instance is stored 
// privately in the TM_Info class and that class can safely access this.
//
class CTmXaTxn :public CTmTxBase
{
   protected:
       // rmid and XID need to map to sequence number
       XID iv_XID;
       int64 iv_rmid;

   public:
       CTmXaTxn(int32 pv_nid, int64 pv_flags, int32 pv_trace_level, int32 pv_seq, 
                  int32 pv_pid, int32 pv_rm_wait_time);
       CTmXaTxn(){iv_in_use = false;}
       ~CTmXaTxn();

       static CTmXaTxn *constructPoolElement(int64 pv_seqnum);
       virtual void cleanup();
       
       virtual void initialize(int32 pv_nid, int64 pv_flags, int32 pv_trace_level, 
                               int32 pv_seq, int32 pv_creator_nid, int32 pv_creator_pid, 
                               int32 pv_rm_wait_time);

       // CTmXaTxn specific functions
       void xid(XID *pp_XID) {memcpy(&iv_XID, pp_XID, sizeof(iv_XID));}
       XID *xid() {return &iv_XID;}
       bool xid_eq(XID *pp_XID);
       void rmid(int64 pv_rmid) {iv_rmid = pv_rmid;}
       int64 rmid() {return iv_rmid;}

       // Derived functions
       virtual int32 schedule_abort();

       // trans state related stuff here
       virtual int32 state_change (TX_EVENT pv_event, CTmTxMessage *pp_msg);
       TM_TX_STATE state_change_abort_helper(CTmTxMessage * pp_msg);
       TM_TX_STATE state_change_abort_set( CTmTxMessage * pp_msg, short pv_error);
       TM_TX_STATE state_change_commit_helper(CTmTxMessage * pp_msg, bool pv_read_only = false);
       TM_TX_STATE state_change_prepare_helper(CTmTxMessage * pp_msg);
       TM_TX_STATE state_change_forget_helper(CTmTxMessage * pp_msg);

       // DTM standard event specific methods
       virtual bool req_begin(CTmTxMessage *pp_msg);
       virtual bool req_end(CTmTxMessage *pp_msg);
       virtual bool req_end_complete(CTmTxMessage *pp_msg = NULL);
       virtual bool req_forget(CTmTxMessage *pp_msg = NULL);
       virtual bool req_abort(CTmTxMessage *pp_msg);
       virtual bool req_abort_complete(CTmTxMessage *pp_msg = NULL);
       //virtual bool req_status(CTmTxMessage *pp_msg);
       //virtual bool req_ax_reg(CTmTxMessage *pp_msg);
       //virtual bool req_join(CTmTxMessage *pp_msg);
       //virtual bool req_suspend(CTmTxMessage *pp_msg);
       virtual bool rollback_txn(CTmTxMessage *pp_msg);
       virtual bool redriverollback_txn(CTmTxMessage *pp_msg);
       virtual bool redrivecommit_txn(CTmTxMessage *pp_msg);

       //  Map to XA error code
       virtual int mapErr(short pv_tmError);

       // XARM event specific methods
       bool req_xa_start(CTmTxMessage *pp_msg);
       bool req_xa_join(CTmTxMessage *pp_msg);
       bool req_xa_resume(CTmTxMessage *pp_msg);
       bool req_xa_end(CTmTxMessage *pp_msg);
       bool req_xa_prepare(CTmTxMessage *pp_msg);
       bool req_xa_commit(CTmTxMessage *pp_msg);
       bool req_xa_rollback(CTmTxMessage *pp_msg);
       bool req_xa_forget(CTmTxMessage *pp_msg);
       //  bool req_xa_recover(CTmTxMessage *pp_msg);
       // bool req_xa_complete(CTmTxMessage *pp_msg);
       // bool req_ax_reg(CTmTxMessage *pp_msg);
       // bool req_ax_unreg(CTmTxMessage *pp_msg);
       void req_xa_notImplemented(CTmTxMessage *pp_msg);
       bool releaseTxnObj(bool pv_terminate);
       
       virtual void process_eventQ(); 
       virtual void schedule_eventQ();
       virtual short doom_txn();
};
#endif //CXaTxn_H_
