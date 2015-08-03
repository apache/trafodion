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

#ifndef TMSYNC_H_
#define TMSYNC_H_

#include "tmlibmsg.h"

// ------------------------------
// types of sync messages
// ------------------------------

typedef enum {
    TM_BEGIN_SYNC     = 1,
    TM_END_SYNC       = 2,
    TM_FORGET_SYNC    = 3,
    TM_RECOVERY_START = 5,
    TM_RECOVERY_END   = 6,
    TM_LEAD_TM        = 7,
    TM_UP             = 8,
    TM_SYS_RECOV_START_SYNC     = 9,
    TM_SYS_RECOV_END_SYNC       = 10,
    TM_TAKEOVER_TM_START_SYNC   = 11,
    TM_TAKEOVER_TM_END_SYNC     = 12, 
    TM_PROCESS_RESTART          = 13,
    TM_LISTBUILT_SYNC           = 14,
    TM_STATE_RESYNC               = 15,

} TM_SYNC_TYPE;

// ----------------------------------------
// Sync data, request and reply structs
// ----------------------------------------

typedef struct _tmsync_h_as_0 
{
    int32            iv_sync_otag;
    int32            iv_num_tries;
    int32            iv_sync_type;
    union
    {
        int32            iv_seqnum;
        int32            iv_node_to_takeover;
    } u;
} Tm_Sync_Type_Transid;

typedef struct _tmsync_h_as_1 {
   int32        iv_type;
   int32        iv_nid;
} Tm_Sync_Header;

typedef struct _tmsync_h_as_2 {
   int32            iv_pid;
   TM_Transid_Type  iv_transid;
   TM_TX_STATE      iv_state;
   bool             iv_is_valid;
} Tm_Tx_Sync_Data;

typedef struct _tmsync_h_as_3 {
   int32 iv_my_node;
   int32 iv_down_node;
} Tm_Sync_TakeOver;

typedef struct _tmsync_h_as_4 {
   int32 iv_sys_recov_state;
   int32 iv_sys_recov_lead_tm_node;
} Tm_Sync_Sys_Recovery;

typedef struct _tmsync_h_as_5 {
   TM_Transid_Type  iv_transid;
   int32            iv_my_nid;
} Tm_Sync_TakeOver_Trans;

typedef struct _tmsync_h_as_6 {
   int32 iv_proc_restart_lead_tm_node;
   int32 iv_proc_restart_node;
} Tm_Sync_Process_Restart;

typedef struct _tmsync_h_as_7 {
   int32 iv_down_node;
} Tm_Sync_ListBuilt;

typedef struct _tmsync_h_as_8 {
   int32 iv_index;
   bool  iv_down_without_sync;
   int32 iv_node_being_recovered;
   bool  iv_list_built;
} Tm_Sync_State_Resync;

typedef struct _tmsync_h_as_9 {
    Tm_Sync_Header iv_hdr;
    union
    {
        Tm_Tx_Sync_Data iv_tx_data;
        Tm_Sync_TakeOver iv_to_data;
        Tm_Sync_Sys_Recovery iv_sys_recov_data;
        Tm_Sync_Process_Restart iv_proc_restart_data;
        Tm_Sync_TakeOver_Trans iv_takeover_trans;
        Tm_Sync_ListBuilt iv_list_built;
        Tm_Sync_State_Resync iv_state_resync;
    } u;
} Tm_Sync_Data;

typedef struct _tmsync_h_as_10 {
   MESSAGE_HEADER_SQ    iv_msg_hdr;
   int32                iv_error;
   int32                iv_node;
   bool                 iv_state_up;
   int32                iv_BroadcastSeqNum;
   int32                iv_DataStartAddr;
   Tm_Sync_Sys_Recovery iv_sys_recov_data; 
   Tm_Tx_Sync_Data      iv_data[MAX_TRANS_PER_SYNC];
} Tm_Broadcast_Req_Type;

typedef struct _tmsync_h_as_11 
{
    MESSAGE_HEADER_SQ    iv_msg_hdr;
    int32          iv_error;
} Tm_Broadcast_Rsp_Type;


// -----------------------------------------------------------
// Sync helper methods
// -----------------------------------------------------------

void init_and_send_tx_sync_data( TM_SYNC_TYPE pv_type, TM_TX_STATE pv_state,
                              TM_Transid_Type *pp_transid,
                              int32 pv_nid, int32 pv_pid);
void send_recov_listbuilt_sync(int32 pv_nid, int32 pv_dead_tm);
void send_sync_data (void *pv_buffer, int32 pv_length,
                     TM_SYNC_TYPE pv_type, int32 pv_nid, 
                     Tm_Sync_Type_Transid *pp_data = NULL);
void send_state_up_sync(int32 pv_nid);
void send_state_resync(int32 pv_nid, bool  iv_down_without_sync, int32 iv_node_being_recovered,
          bool  iv_list_built, int32 pv_index); 
void send_sys_recov_start_sync(int32 pv_nid);
void send_sys_recov_end_sync(int32 pv_nid);
void send_takeover_tm_sync(TM_SYNC_TYPE pv_type, int32 pv_nid, int32 pv_dead_tm);
void send_tm_process_restart_sync(int32 pv_lead_nid, int32 pv_restart_nid);

#endif

