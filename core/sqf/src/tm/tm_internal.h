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

#ifndef __TMLIBMSG_H_
#define __TMLIBMSG_H_
/***********************************************************************
   tmlibmsg.h
   TM Library / TM-TSE Library Message Header file
   The TM Library provides the generic TM API interfaces for manipulating
   global transactions such as BEGINTRANSACTION.
   The TM - TSE Library provides TM interfaces specific to the TSE.
   This header contains definitions used by both the TM Library and 
   TM-TSE Library when communicating with the TM.  These are not
   externalized to callers.
   This replaces tm_internal.h.  The new message format uses
   MSG_HEADER_SQ to define the message header found in rm.h and allows
   consistent message versioning with the XATM Library.  The dialect
   for all TM Library messages is DIALECT_TM_SQ.
   The TM-TSE Library message dialect is DIALECT_DP2_TM_SQ_PRIV.
***********************************************************************/


#include "dtm/tm.h"

#include "xa.h"
#include "rm.h"
#include "../../inc/fs/feerrors.h" //legacy error codes for SQL

#define MAX_FILE_NAME 64
#define MAX_NUM_TRANS   1024
#define MAX_SYNC_TXS 50
#define MAX_RECEIVE_BUFFER 200000
// low number for testing
#define MAX_TRANS_FOR_CP 100 
// wake up interval is 30 seconds for polling
#define LEAD_DTM_WAKEUP_INTERVAL 5000 

#define TM_NO_ERR      0
#define TM_ERR        -1

// Request and Reply types
typedef enum {
    TM_MSG_TYPE_BEGINTRANSACTION         = 201,
    TM_MSG_TYPE_BEGINTRANSACTION_REPLY   = 202,
    TM_MSG_TYPE_ENDTRANSACTION           = 203,
    TM_MSG_TYPE_ENDTRANSACTION_REPLY     = 204,
    TM_MSG_TYPE_ABORTTRANSACTION         = 205,
    TM_MSG_TYPE_ABORTTRANSACTION_REPLY   = 206,
    TM_MSG_TYPE_STATUSTRANSACTION        = 207,
    TM_MSG_TYPE_STATUSTRANSACTION_REPLY  = 208,
    TM_MSG_TYPE_GETTRANSID               = 209,
    TM_MSG_TYPE_GETTRANSID_REPLY         = 210,
    TM_MSG_TYPE_RESUMETRANSACTION        = 211,
    TM_MSG_TYPE_RESUMETRANSACTION_REPLY  = 212,
    TM_MSG_TYPE_AX_REG                   = 213,
    TM_MSG_TYPE_AX_REG_REPLY             = 214,
    TM_MSG_TYPE_AX_UNREG                 = 215,
    TM_MSG_TYPE_AX_UNREG_REPLY           = 216,
    TM_MSG_TYPE_TEST_TX_COUNT            = 217,
    TM_MSG_TYPE_JOINTRANSACTION          = 218,
    TM_MSG_TYPE_JOINTRANSACTION_REPLY    = 219,
    TM_MSG_TYPE_SUSPENDTRANSACTION       = 220,
    TM_MSG_TYPE_SUSPENDTRANSACTION_REPLY = 221,
    TM_MSG_TYPE_BROADCAST                = 291,
    TM_MSG_TYPE_BROADCAST_REPLY          = 292,
    TM_MSG_TYPE_CP                       = 293,
    TM_MSG_TYPE_CP_REPLY                 = 294
} TM_MSG_TYPE;

// TM system states 
typedef enum {
    TM_STATE_INITIAL                 = 100,// Initial state, before startup commences.
    TM_STATE_UP,                     
    TM_STATE_DOWN,                         // initial state at startup time.
    TM_STATE_SHUTTING_DOWN,                // state after receiving shutdown msg from monitor
    TM_STATE_SHUTDOWN_FAILED,
    TM_STATE_SHUTDOWN_COMPLETED,           // final state after Shutdown operation
    TM_STATE_TX_DISABLED,
    TM_STATE_TX_DISABLED_SHUTDOWN_PHASE1,  // In phase 1 of shutdown. Transactions disabled.
    TM_STATE_QUIESCE,                      // Received a NodeQuiesce from Monitor.
    TM_STATE_DRAIN,                        // Received a drain.
    TM_STATE_WAITING_RM_OPEN               // Waiting for RM opens to complete before declaring the TM up.
} TM_STATE;

// ------------------------
// request structs
// ------------------------

typedef struct _tm_internal_h_as_0 {
    TM_Txid_External    iv_transid;
    int32               iv_tag;
} Abort_Trans_Req_Type;

typedef struct _tm_internal_h_as_1 {
   TM_Txid_External     iv_txid;
   int32                iv_rmid;
   int32                iv_flags;
   int32                iv_tm_nid;
} Ax_Reg_Req_Type;

typedef struct _tm_internal_h_as_2 {
    int32 iv_pid;
} Begin_Trans_Req_Type;

typedef struct _tm_internal_h_as_3 {
    TM_Txid_External iv_transid;
    int32            iv_tag;
    int32            iv_pid;
} End_Trans_Req_Type;

typedef struct _tm_internal_h_as_4 {
    TM_Txid_External iv_transid;
} Get_Transid_Req_Type;

typedef struct _tm_internal_h_as_5 {
    TM_Txid_External iv_transid;
} Join_Trans_Req_Type;

typedef struct _tm_internal_h_as_6 {
    int32 iv_tag;
    int32 iv_pid;
} Resume_Trans_Req_Type;

typedef struct _tm_internal_h_as_7 {
    TM_Txid_External iv_transid;
} Status_Trans_Req_Type;

typedef struct _tm_internal_h_as_8 {
    TM_Txid_External iv_transid;
} Suspend_Trans_Req_Type;

typedef struct _tm_internal_h_as_9 {
} Test_Tx_Count;

typedef struct _tm_internal_h_as_10 {
    MESSAGE_HEADER_SQ iv_msg_hdr;
    union {
        Abort_Trans_Req_Type  iv_abort_trans;
        Ax_Reg_Req_Type       iv_ax_reg;
        Begin_Trans_Req_Type  iv_begin_trans;
        End_Trans_Req_Type    iv_end_trans;
        Get_Transid_Req_Type  iv_get_transid;
        Join_Trans_Req_Type   iv_join_trans;
        Resume_Trans_Req_Type iv_resume_trans;
        Status_Trans_Req_Type iv_status_trans;
        Suspend_Trans_Req_Type iv_suspend_trans;
        Test_Tx_Count         iv_count;
    } u;
} Tm_Req_Msg_Type;


// ------------------------
// reply structs
// ------------------------

typedef enum {
    TM_RSP_ERROR_OK     = 0,
    TM_RSP_ERROR_NOT_OK = 1
} TM_RSP_ERROR_TYPE;

typedef struct _tm_internal_h_as_11 {
} Abort_Trans_Rsp_Type;

typedef struct _tm_internal_h_as_12 {
   XID     iv_xid;
} Ax_Reg_Rsp_Type;

typedef struct _tm_internal_h_as_13 {
    TM_Txid_External iv_transid;
    int32            iv_tag;
} Begin_Trans_Rsp_Type;

typedef struct _tm_internal_h_as_14 {
} End_Trans_Rsp_Type;

typedef struct _tm_internal_h_as_15 {
   TM_Txid_External iv_transid;
} Get_Transid_Rsp_Type;

typedef struct _tm_internal_h_as_16 {
} Join_Trans_Rsp_Type;

typedef struct _tm_internal_h_as_17 {
    TM_Txid_External iv_transid;
} Resume_Trans_Rsp_Type;

typedef struct _tm_internal_h_as_18 {
   int16 iv_status;
} Status_Trans_Rsp_Type;

typedef struct _tm_internal_h_as_19 {
} Suspend_Trans_Rsp_Type;

typedef struct _tm_internal_h_as_20 {
   int32 iv_count;
} Test_Tx_Count_Rsp_Type;

typedef struct _tm_internal_h_as_21 {
    MESSAGE_HEADER_SQ  iv_msg_hdr;
    union {
        Abort_Trans_Rsp_Type  iv_abort_trans;
        Ax_Reg_Rsp_Type       iv_ax_reg;
        Begin_Trans_Rsp_Type  iv_begin_trans;
        End_Trans_Rsp_Type    iv_end_trans;
        Get_Transid_Rsp_Type  iv_get_transid;
        Join_Trans_Rsp_Type   iv_join_trans;
        Resume_Trans_Rsp_Type iv_resume_trans;
        Status_Trans_Rsp_Type iv_status_trans;
        Suspend_Trans_Rsp_Type iv_suspend_trans;
        Test_Tx_Count_Rsp_Type iv_count;
    } u;
} Tm_Rsp_Msg_Type;

#define tm_max(a,b) (((a) > (b)) ? (a) : (b))
enum { TM_MAX_DATA = tm_max(sizeof(Tm_Req_Msg_Type), sizeof(Tm_Rsp_Msg_Type)) };

#define TM_MsgSize(MsgType) \
      (sizeof(MESSAGE_HEADER_SQ) + sizeof(MsgType))

#endif //__TMLIBMSG_H_
