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

#ifndef __RM_H_
#define __RM_H_

#include <stddef.h>

#include "dtm/tm_util.h"
#include "seabed/ms.h"

#include "dtm/xa.h"
#define FORMAT_ID 403
// MAX_OPEN_RMS: 1 is added for $SYSTEM.
#define MAX_OPEN_RMS (1024 + 1)
#define rm_max(a,b) (((a) > (b)) ? (a) : (b))
#define MAX_RECOVER_XIDS 128
#define MAX_RM_CLOSE_INFO_LEN 256
#define XA_CLOSE_SHUTDOWN_CLEAN         "SHUTDOWN_CLEAN"
#define XA_CLOSE_SHUTDOWN_DIRTY         "SHUTDOWN_DIRTY"

// XARM xa_open variable sizes
#define MAXVARNAME 16

enum {
   TM_SQ_MSG_VERSION_JAN07 = 1,  // Original SQ POC 0-8 version
                                 // Note the name has changed from TM_DP2_SQ_VERSION_JAN07!
   TM_SQ_MSG_VERSION_JUN07 = 2,  // POC 9+ version with changes to TM Library
                                 // and TM-TSE Library message formats.
   TM_SQ_MSG_VERSION_APR11 = 3,  // M5 includes additional managment APIs.
   TM_SQ_MSG_VERSION_AUG11 = 4,  // M5 SPR Fail Safe
   TM_SQ_MSG_VERSION_CURRENT = TM_SQ_MSG_VERSION_AUG11,
   // The minimum version supported is Jun07 because the message header
   // format changed with that version.
   TM_SQ_MSG_VERSION_MINIMUM = TM_SQ_MSG_VERSION_JUN07
};
extern xa_switch_t *tm_switch;

void process_msg(MS_SRE *pp_sre); 

#ifndef INTERROGATE_DEF
#define INTERROGATE_DEF
#endif

#if !defined (_DP2_COMPILE) && !defined (_ADP_COMPILE)
enum {
   DIALECT_TM_DP2_SQ       = 118,
   DIALECT_TM_SQ           = 119,
   DIALECT_DP2_TM_SQ_PRIV  = 120,
   DIALECT_TM_AMP_SQ       = 121
}; // will be in fs/ydialect.h otherwise
#endif

// DTM internal XATM interface to TSEs
extern int tm_xa_close (char *, int, int64);
extern int tm_xa_commit (XID *, int, int64);
extern int tm_xa_complete (int *, int *, int, int64);
extern int tm_xa_end (XID *, int, int64);
extern int tm_xa_forget (XID *, int, int64);
extern int tm_xa_open (char *, int, int64);
extern int tm_xa_prepare (XID *, int, int64);
extern int tm_xa_recover (XID *, int64, int, int64);
extern int tm_xa_rollback (XID *, int, int64);
extern int tm_xa_start (XID *, int, int64);
// not part of the xa switch
extern int tm_xa_dtm_recover (XID *, int64, int, int64, int);

// Utility functions
extern int64 XIDtotransid(XID *);

typedef struct _rm_h_as_0 {
   char process_name[MS_MON_MAX_PROCESS_NAME];
   int  incarnation_num;
   unsigned int  seq_num_block_start;
} RM_Open_struct;

typedef struct _rm_h_as_1 {
   short    dialect_type;  // DIALECT_TM_DP2_SQ

   union {
   short request_type;       // one of TM_DP2_SQ_MSG_TYPE or TM_MSG_TYPE
   short reply_type;         // reply is +1 more than request
   } rr_type;

   union {
      short      request_version;
      short      reply_version;
   } version;

   union {
      short      minimum_interpretation_version; // for request
      short      error;                          // for reply:signed 16-bit
   } miv_err;
} MESSAGE_HEADER_SQ;


typedef enum {
    TM_DP2_SQ_XA_INTERROGATE                    = 100,  //dummy
    TM_DP2_SQ_XA_INTERROGATE_REPLY              = 101,  //dummy

    TM_DP2_SQ_XA_START                          = 102,
    TM_DP2_SQ_XA_START_REPLY                    = 103,

    TM_DP2_SQ_XA_END                            = 104,
    TM_DP2_SQ_XA_END_REPLY                      = 105,

    TM_DP2_SQ_XA_COMMIT                         = 106,
    TM_DP2_SQ_XA_COMMIT_REPLY                   = 107,

    TM_DP2_SQ_XA_PREPARE                        = 108,
    TM_DP2_SQ_XA_PREPARE_REPLY                  = 109,

    TM_DP2_SQ_XA_ROLLBACK                       = 110,
    TM_DP2_SQ_XA_ROLLBACK_REPLY                 = 111,

    TM_DP2_SQ_XA_OPEN                           = 112,
    TM_DP2_SQ_XA_OPEN_REPLY                     = 113,

    TM_DP2_SQ_XA_CLOSE                          = 114,
    TM_DP2_SQ_XA_CLOSE_REPLY                    = 115,

    TM_DP2_SQ_XA_RECOVER                        = 116, 
    TM_DP2_SQ_XA_RECOVER_REPLY                  = 117,

    TM_DP2_SQ_XA_FORGET                         = 118,
    TM_DP2_SQ_XA_FORGET_REPLY                   = 119,

    TM_DP2_SQ_XA_COMPLETE                       = 120,
    TM_DP2_SQ_XA_COMPLETE_REPLY                 = 121,

    TM_DP2_SQ_AX_REG                            = 122,
    TM_DP2_SQ_AX_REG_REPLY                      = 123,

    TM_DP2_SQ_AX_UNREG                          = 124,
    TM_DP2_SQ_AX_UNREG_REPLY                    = 125
} TM_DP2_SQ_MSG_TYPE;

typedef struct _rm_h_as_2 {
   XID iv_xid;
   int iv_rmid;
   int64 iv_flags;
   int iv_nid;
   int iv_pid;
} RM_Start_Req_Type;

typedef struct _rm_h_as_3 {
   XID iv_xid;
   int iv_rmid;
   int64 iv_flags;
   int iv_nid;
   int iv_pid;
} RM_End_Req_Type;

typedef struct _rm_h_as_4 {
   XID iv_xid;
   int iv_rmid;
   int64 iv_flags;
   int iv_nid;
   int iv_pid;
} RM_Rollback_Req_Type;

typedef struct _rm_h_as_5 {
   XID iv_xid;
   int iv_rmid;
   int64 iv_flags;
   int iv_nid;
   int iv_pid;
} RM_Commit_Req_Type;

typedef struct _rm_h_as_6 {
   XID iv_xid;
   int iv_rmid;
   int64 iv_flags;
   int iv_nid;
   int iv_pid;
} RM_Forget_Req_Type;

typedef struct _rm_h_as_7 {
   XID iv_xid;
   int iv_rmid;
   int64 iv_flags;
   int iv_nid;
   int iv_pid;
} RM_Prepare_Req_Type;

typedef struct _rm_h_as_8 {
   int iv_rmid;
   int iv_tm_nid;
   int iv_incarnation_num;
   unsigned int iv_seq_num_minimum;
   int64 iv_flags;
   char iv_info[MAXINFOSIZE];
   int iv_nid;
   int iv_pid;
} RM_Open_Req_Type;

typedef struct _rm_h_as_9 {
   int iv_rmid;
   char  iv_info[MAX_RM_CLOSE_INFO_LEN];
   int64 iv_flags;
   int iv_nid;
   int iv_pid;
} RM_Close_Req_Type;

typedef struct _rm_h_as_10 {
   int iv_seqnum;
   XID iv_xid;
   int64 iv_txid;
   int iv_nid;
   int iv_pid;
} RM_Test_Req_Type;

typedef struct _rm_h_as_11 {
   int32 iv_rmid;
   int64 iv_flags;
   int64 iv_count;
   int32 iv_recovery_index;
   bool  iv_dtm_death;
   int32 iv_dtm_node;
   int iv_nid;
   int iv_pid;
} RM_Recover_Req_Type;


typedef struct _rm_h_as_12 {
      MESSAGE_HEADER_SQ iv_msg_hdr;
   //INTERROGATE_DEF
   union {
     RM_Start_Req_Type    iv_start;
     RM_End_Req_Type      iv_end;
     RM_Rollback_Req_Type iv_rollback;
     RM_Commit_Req_Type   iv_commit;
     RM_Forget_Req_Type   iv_forget;
     RM_Prepare_Req_Type  iv_prepare;
     RM_Open_Req_Type     iv_open;
     RM_Close_Req_Type    iv_close;
     RM_Test_Req_Type     iv_test;
     RM_Recover_Req_Type  iv_recover;
  } u;
} RM_Req_Msg_Type;

//responses

typedef enum {
    RM_RSP_ERROR_OK = 0,
    RM_RSP_ERROR_NOT_OK = 1
} RM_RSP_ERROR_TYPE;

typedef struct _rm_h_as_13 {
} RM_Start_Rsp_Type;

typedef struct _rm_h_as_14 {
} RM_End_Rsp_Type;

typedef struct _rm_h_as_15 {
} RM_Rollback_Rsp_Type;

typedef struct _rm_h_as_16 {
} RM_Commit_Rsp_Type;

typedef struct _rm_h_as_17 {
} RM_Forget_Rsp_Type;

typedef struct _rm_h_as_18 {
  int iv_partic;
} RM_Prepare_Rsp_Type;

typedef struct _rm_h_as_19 {
   int iv_opener;
   bool  iv_ax_reg;
} RM_Open_Rsp_Type;

typedef struct _rm_h_as_20 {
} RM_Close_Rsp_Type;

typedef struct _rm_h_as_21 {
   bool  iv_end;
   int64 iv_count;
   int32 iv_recovery_index;
   XID   iv_xid[MAX_RECOVER_XIDS];
} RM_Recover_Rsp_Type;


typedef struct _rm_h_as_22 {
      MESSAGE_HEADER_SQ iv_msg_hdr;
 // INTERROGATE_DEF
  union {
     RM_Start_Rsp_Type    iv_start;
     RM_End_Rsp_Type      iv_end;
     RM_Rollback_Rsp_Type iv_rollback;
     RM_Forget_Rsp_Type   iv_forget;
     RM_Commit_Rsp_Type   iv_commit;
     RM_Prepare_Rsp_Type  iv_prepare;
     RM_Open_Rsp_Type     iv_open;
     RM_Close_Rsp_Type    iv_close;
     RM_Recover_Rsp_Type  iv_recover;
  } u;
} RM_Rsp_Msg_Type;

enum { RM_MAX_DATA = rm_max(sizeof(RM_Req_Msg_Type), sizeof(RM_Rsp_Msg_Type)) };

#define RM_MsgSize(MsgType) \
      (sizeof(MESSAGE_HEADER_SQ) + sizeof(MsgType))

//other stuff

// branch information used for the RM
struct TM_RM_Branch_Data
{
    int       iv_branchid;
    int       iv_branchtype;
};

struct TM_RM_MAP_Data
{
    TM_RM_Branch_Data   iv_data;
    bool                iv_checked_in;
};

#endif // __RM_H_


