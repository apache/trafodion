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

#ifndef TMRMTSE_H_
#define TMRMTSE_H_

#include "dtm/tm.h"
#include "tmrm.h"
#include "tmrmtsebranch.h"
//#include "tmtxbase.h"
//#include "tmtxmsg.h"

typedef int (*xa_entry)(XID *xid, int rmid, int64 flags);

class CTmTxBase;
class CTmTxMessage;
//class RM_Info_TSEBranch;

// TSE Branch implementation
class RM_Info_TSE :public virtual RM_Info
{
    private:
      RM_Info_TSEBranch ia_TSEBranches[MAX_OPEN_RMS];
      int        iv_high_index_used;

    public:

      RM_Info_TSE();
      ~RM_Info_TSE(){}

      int32 init (int32 pv_pid, int32 pv_nid, char *pp_name, int32 pv_rmid, 
                  bool pv_is_ax_reg, TSEBranch_state pv_state=TSEBranch_DOWN,
                  bool pv_clean_init = false);
      int32 reinit (int32 pv_pid, int32 pv_nid, char *pp_name, int32 pv_rmid, 
                    bool pv_is_ax_reg, bool pv_clean_init = false, 
                    TSEBranch_state pv_state= TSEBranch_RECOVERING);
      int set_partic_and_transid(TM_Txid_Internal pv_transid, int32 pv_rmid);
      void remove_rm_by_rmid(int32 pv_rmid);
      void remove_rm_by_index (int32 pv_index);
      int  return_highest_index_used() {return iv_high_index_used;}
      RM_Info_TSEBranch *return_slot(int32 pv_rmid);
      int32 return_slot_index(int32 pv_rmid);
      RM_Info_TSEBranch *return_slot(char * pp_pname);
      RM_Info_TSEBranch *return_slot_by_index(int32 pv_index);
      int return_rmid(int32 pv_nid, int32 pv_pid);
      RM_Info_TSEBranch *get_slot(int32 pv_slotIndex)
         { return (RM_Info_TSEBranch *) &ia_TSEBranches[pv_slotIndex];}
      RM_Info_TSEBranch *return_rms() {return ia_TSEBranches;}
      void fail_rm(int32 pv_rmid);
      void fail_rm(int32 pv_nid, int32 pv_pid);
      void setBranchState(int32 pv_rmid, TSEBranch_state pv_state);
      virtual int32 xa_send (xa_entry *pp_fnc_ptr, RM_Info_TSEBranch *pa_rms, 
                             int64 pv_flags);

      // Common functions
      virtual int32 num_rm_partic(CTmTxBase *pp_txn=NULL);
      virtual int32 num_rms_unresolved(CTmTxBase *pp_txn=NULL);
      virtual void reset_resolved(CTmTxBase *pp_txn=NULL);
      virtual int32 num_rm_failed(CTmTxBase *pp_txn=NULL);
      virtual void init_rms(CTmTxBase *pp_txn, bool pv_partic);
      virtual int32 shutdown_branches(bool pv_leadTM, bool pv_clean);
      
      // TSE  branch interface
      virtual int32 commit_branches (CTmTxBase *pp_txn, int64 pv_flags, CTmTxMessage * pp_msg);    
      virtual int32 end_branches (CTmTxBase *pp_txn, int64 pv_flags);  
      virtual int32 forget_branches (CTmTxBase *pp_txn, int64 pv_flags);    
      virtual int32 forget_heur_branches (CTmTxBase *pp_txn, int64 pv_flags); 
      virtual int32 prepare_branches (CTmTxBase *pp_txn, int64 pv_flags, CTmTxMessage * pp_msg);
      virtual int32 rollback_branches (CTmTxBase *pp_txn, int64 pv_flags, CTmTxMessage * pp_msg,
                                       bool lv_error_condition = false);       
      virtual int32 start_branches (CTmTxBase *pp_txn, int64 pv_flags, CTmTxMessage * pp_msg);
      virtual int32 registerRegion (CTmTxBase *pp_txn, int64 pv_flags, CTmTxMessage * pp_msg);

      virtual int32 prepare_branches_single_pass (CTmTxBase *pp_txn, int64 pv_flags);
      virtual int32 rollback_branches_single_pass (CTmTxBase *pp_txn, int64 pv_flags, CTmTxMessage * pp_msg,
                                       bool lv_error_condition = false);       
};

struct TM_RM_Responses
{
    int32 iv_rmid;
    int32 iv_error;
    bool  iv_partic;
    bool  iv_ax_reg;
    bool  iv_drive_recover;

    TM_RM_Responses() : iv_rmid(-1), iv_error(XA_OK), iv_partic(false), 
                        iv_ax_reg(false), iv_drive_recover(false) {};
};

// used to manage rm information in the TM
typedef struct _tmrm_h_as_0 {
    SB_Phandle_Type iv_phandle;
    int32 iv_rmid;
    int16 iv_in_use;
    int32 iv_msgid;
    RM_Rsp_Msg_Type *iv_rsp;
    RM_Req_Msg_Type *iv_req;
} Rm_Rmid_Info;

// XATM Library global functions
int32 complete_all(int32 pv_num, TM_RM_Responses *pa_resp, int32 pv_rm_wait_time, int64 pv_transid=0);
int complete_one(int rmid, int *handle);
bool tm_xa_rmType_TSE(int pv_rmid);
bool tm_XARM_generic_library();
void xaTM_setTrace(unsigned long pv_mask);
 
#endif // TMRMTSE_H_
