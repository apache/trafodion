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

#ifndef TMRECOVSTATE_H_
#define TMRECOVSTATE_H_

#include "tmtx.h"
#include "tmtxkey.h"

class CTmRmTxRecoveryState
{
private:
    TM_TX_STATE iv_state;
    TM_Txid_Internal iv_transid;
    int32 iv_abort_flags;
    int32 iv_rms_to_recover;
    // iv_rmList is the list of participating subordinate RMs.
    // Elements are pointers to the RM_Info_TSEBranch object in 
    // gv_RMs.TSE().
    TM_MAP iv_rmList; // List of RMs participating

public:
    CTmRmTxRecoveryState(TM_Txid_Internal *pp_transid, TM_TX_STATE pv_state);
    ~CTmRmTxRecoveryState() {iv_rmList.clear();}
    void state(TM_TX_STATE pv_state) {iv_state=pv_state;}
    TM_TX_STATE state() {return iv_state;}
    void abort_flags(int32 pv_flags) {iv_abort_flags=pv_flags;}
    int32 abort_flags() {return iv_abort_flags;}
    TM_Txid_Internal *transid() {return &iv_transid;}
    int32 rms_to_recover() {return iv_rms_to_recover;}
    void inc_rms_to_recover() {iv_rms_to_recover++;}
    void dec_rms_to_recover() {iv_rms_to_recover--;}
    TM_MAP *rmList() {return &iv_rmList;}
    void add_partic(int32 pv_rmid);
    void remove_partic(int32 pv_rmid);
}; //CTmRmTxRecoveryState

#endif //TMRECOVSTATE_H_
