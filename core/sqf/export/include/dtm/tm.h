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
#ifndef __TM_H_
#define __TM_H_

#include "dtm/tm_util.h"
#include "dtm/tmtransid.h"
#include "seabed/ms.h"

#define FENOTENDERSQ 7001
#define FESTILLJOINERS 7002
#define FETMSHUTDOWN_NOTREADY 7003
#define FETMSHUTDOWN_FATAL_ERR 7004
#define FETMRECOVERY_NOT_NEEDED 7005

#define TM_TT_NOFLAGS           0x00
#define TM_TT_READ_ONLY         0x01  
#define TM_TT_NO_UNDO           0x02  
#define TM_TT_NO_CAPACITY_ABORT 0x04  // unsupported
#define TM_TT_NO_AUTO_ABORT     0x08  // default for DTM
#define TM_TT_LOAD_BALANCING    0x10  // unsupported
#define TM_TT_FORCE_CONSISTENCY 0x100 // Overrides NO_UNDO!
                                      // Note the Force Consistency flag is passthrough 
                                      //  - the TM does nothing with this.
#define TM_MAX_LIST_TRANS 128

// Auto abort values.
#define TM_NEVERABORT            -1
#define TM_AUTOABORT_DEFAULT      0

// Disable Transaction shutdown levels.
// These are input values for DTM_DISABLETRANSACTIONS
#define TM_DISABLE_NORMAL                0    // No shutdown
#define TM_DISABLE_SHUTDOWN_NORMAL       1    // Disable transactions and allow current txns to complete
#define TM_DISABLE_SHUTDOWN_IMMEDIATE    2    // Disable transactions and abort all current txns
#define TM_DISABLE_SHUTDOWN_ABRUPT       3    // Monitor brings down processes directly - should never see this

// Transaction states
typedef enum {                            //XA equivalent states
    TM_TX_STATE_NOTX              = 0,    //S0 - NOTX
    TM_TX_STATE_ACTIVE            = 1,    //S1 - ACTIVE
    TM_TX_STATE_IDLE              = 14,   //S2 - IDLE XARM Branches only!
    TM_TX_STATE_FORGOTTEN         = 2,    //N/A
    TM_TX_STATE_COMMITTED         = 3,    //N/A
    TM_TX_STATE_ABORTING          = 4,    //S4 - ROLLBACK
    TM_TX_STATE_ABORTED           = 5,    //S4 - ROLLBACK
    TM_TX_STATE_COMMITTING        = 6,    //S3 - PREPARED
    TM_TX_STATE_PREPARING         = 7,    //S2 - IDLE
    TM_TX_STATE_PREPARED          = 9,    //S3 - PREPARED XARM Branches only!
    TM_TX_STATE_FORGETTING        = 8,    //N/A
    TM_TX_STATE_FORGETTING_HEUR   = 10,   //S5 - HEURISTIC
    TM_TX_STATE_FORGOTTEN_HEUR    = 15,   //S5 - HEURISTIC - Waiting Superior TM xa_forget request
    TM_TX_STATE_BEGINNING         = 11,   //S1 - ACTIVE
    TM_TX_STATE_HUNGCOMMITTED     = 12,   //N/A
    TM_TX_STATE_HUNGABORTED       = 13,   //S4 - ROLLBACK
    TM_TX_STATE_ABORTING_PART2    = 16,   // Internal State
    TM_TX_STATE_TERMINATING       = 17,
    TM_TX_STATE_LAST              = 17
} TM_TX_STATE;

// RM states
typedef enum 
{
   TSEBranch_UP            = 200,
   TSEBranch_DOWN,
   TSEBranch_FAILED,
   TSEBranch_RECOVERING,
   TSEBranch_FAILOVER
} TSEBranch_state;

typedef union RMID {
    struct {
        uint16 iv_num;
        uint16 iv_nid;
    } s;
    uint32 iv_rmid;
} RMID;

typedef struct trans_list {
    int64 iv_transid;
    short iv_status;
    int iv_nid;         // Transaction beginner TM nid
    int iv_seqnum;      // Transaction sequence number
    int iv_tag;         // Transaction tag
    int iv_owner_nid;   // Transaction owner nid
    int iv_owner_pid;   // Transaction owner pid
    int iv_event_count;          // Number of events in eventQ
    int iv_pendingRequest_count; // Number of events in the PendingRequestQ
    int iv_num_active_partic;    // Number of participating processes (joiners)
    int iv_num_partic_RMs;       // Number of secondary RMs (TSEs) participating
    bool iv_XARM_branch;       // transaction is an XARM branch.
    bool iv_transactionBusy;   // transaction is busy with a request
    bool iv_mark_for_rollback; // transaction has been marked for rollback
    bool iv_tm_aborted;        // transaction was aborted by tm or TSE
    bool iv_read_only;         // transaction is read only.
    bool iv_recovering;        // transaction is in recovery.
} TM_LIST_TRANS;


typedef struct status_all_trans {
    int64 iv_transid;
    int64 iv_timestamp;
    short iv_status;
    int iv_nid;         // Transaction beginner TM nid
    int iv_seqnum;      // Transaction sequence number
    int iv_tag;         // Transaction tag
    int iv_owner_nid;   // Transaction owner nid
    int iv_owner_pid;   // Transaction owner pid
    int iv_event_count;          // Number of events in eventQ
    int iv_pendingRequest_count; // Number of events in the PendingRequestQ
    int iv_num_active_partic;    // Number of participating processes (joiners)
    int iv_num_partic_RMs;       // Number of secondary RMs (TSEs) participating.
    int iv_num_unresolved_RMs;   // Number of RMs (TSEs) to be resolved after abort/prepare through forgotten.
    int iv_num_resolved_RMs;     // Number of RMs (TSEs) resolved (received all responses xa_rollback/xa_prepares).
    bool iv_XARM_branch;       // transaction is an XARM branch.
    bool iv_transactionBusy;   // transaction is busy with a request
    bool iv_mark_for_rollback; // transaction has been marked for rollback
    bool iv_tm_aborted;        // transaction was aborted by tm or TSE
    bool iv_read_only;         // transaction is read only.
    bool iv_recovering;        // transaction is in recovery.
} TM_STATUS_ALL_TRANS;

// Pool statistics
typedef struct tmpoolstats {
   double iv_startTime;
   double iv_lastTimeInterval;
   int32 iv_poolThresholdEventCounter;
   int32 iv_poolSizeNow;
   int32 iv_inUseListNow;
   int32 iv_freeListNow;
   int32 iv_steadyStateLow;
   int32 iv_steadyStateHigh;
   int32 iv_max;
   int32 iv_totalAllocs_new;
   int32 iv_totalAllocs_free;
   int32 iv_totalDeallocs_free;
   int32 iv_totalDeallocs_delete;
} TMPOOLSTATS;

typedef struct timedstats {
   int32 iv_count;
   double iv_total;
   double iv_totalSq;
} TIMEDSTATS;

typedef struct txnstats {
   TIMEDSTATS iv_txnTotal;
   TIMEDSTATS iv_txnBegin;
   TIMEDSTATS iv_txnAbort;
   TIMEDSTATS iv_txnCommit;
   TIMEDSTATS iv_RMSend;
   TIMEDSTATS iv_ax_reg;
   TIMEDSTATS iv_xa_start;
   TIMEDSTATS iv_xa_end;
   TIMEDSTATS iv_xa_prepare;
   TIMEDSTATS iv_xa_commit;
   TIMEDSTATS iv_xa_rollback;
   int32 iv_RMParticCount;
   int32 iv_RMNonParticCount;
} TXNSTATS;

typedef struct tmcounts {
   int64 iv_tx_count;
   int64 iv_begin_count;
   int64 iv_abort_count;
   int64 iv_commit_count;
   int32 iv_current_tx_count;
   int64 iv_tm_initiated_aborts;
   int32 iv_tx_hung_count;
   int32 iv_current_tx_hung_count; // Current txn hung count
} TMCOUNTS;

typedef struct tmstats {
   int iv_node;
   double iv_tmStartTime;
   double iv_statsSentTime;
   TMCOUNTS iv_counts;
   TXNSTATS iv_txn;
   TMPOOLSTATS iv_threadPool_stats;
   TMPOOLSTATS iv_transactionPool_stats;
   TMPOOLSTATS iv_RMMessagePool_stats;
} TM_TMSTATS;

typedef struct RM_INFO {
    char ia_name[10];
    TSEBranch_state iv_state;
    bool iv_in_use;
    int32 iv_nid;
    int32 iv_rmid;
    bool iv_partic;
    int32 iv_pid;
    int32 iv_totalBranchesLeftToRecover;
} RM_INFO;

typedef struct  TMSTATUS {
    int32 iv_node;
    bool  iv_isLeadTM;
    int32 iv_state;
    int32 iv_sys_recovery_state;
    MS_MON_ShutdownLevel iv_shutdown_level;
    int16 iv_incarnation_num;
    int32 iv_number_active_txns;
    bool iv_is_isolated;
    int32 iv_rm_count;
    RM_INFO ia_rminfo[512 + 1];
} TMSTATUS;

typedef struct TM_STATUS_TRANS {
     TM_Transid_Type iv_transid;
     short iv_status;
     int32 iv_nid;                // Transaction beginner TM nid
     int32 iv_seqnum;             // Transaction sequence number
     int16 iv_incarnation_num;    // Transaction incarnation number
     int16 iv_tx_flags;           // Transaction flags
     int64 iv_tt_flags;           // Transaction Type flags
     int iv_owner_nid;            // Transaction owner nid
     int iv_owner_pid;            // Transaction owner pid
     int iv_event_count;          // Number of events in eventQ
     int iv_pendingRequest_count; // Number of events in the PendingRequestQ
     int iv_num_active_partic;    // Number of participating processes (joiners)
     int iv_num_partic_RMs;       // Number of subordinate RMs (TSEs) participating
     bool iv_XARM_branch;         // transaction is an XARM branch.
     bool iv_transactionBusy;     // transaction is busy with a request
     bool iv_mark_for_rollback;   // transaction has been marked for rollback
     bool iv_tm_aborted;          // transaction was aborted by tm or TSE
     int32 iv_abort_flags;        // transaction abort flags
     bool iv_read_only;           // transaction is read only.
     bool iv_recovering;          // transaction is in recovery.
} TM_STATUS_TRANS;

typedef struct TM_STATUSSYS {
    int32 iv_up;
    int32 iv_down;
    int32 iv_recovering;
    int32 iv_totaltms;
    int32 iv_activetxns;
    int32 iv_leadtm;
} TM_STATUSSYS;

typedef struct TM_HBASEREGIONINFO {
    int64 iv_transid;
    short iv_status;
    int iv_nid;                 // Transaction beginner TM nid
    int iv_seqnum;              // Transaction sequence number
    int iv_owner_nid;           // Transaction owner nid
    int iv_owner_pid;           // Transaction owner pid
    char iv_tablename[300];     // Tablename
    char iv_enc_regionname[50]; // Encoded region name
    char iv_regionname[300];   // Region name
    char iv_is_offline[20];     // IsOffline
    char iv_region_id[200];     // Region Id
    char iv_hostname[200];      // Hostname
    char iv_port[100];          // Port
} TM_HBASEREGIONINFO;

// Trafodion start //
#define TM_MAX_REGIONSERVER_STRING 2048
#define TM_MAX_DDLREQUEST_STRING 2048
#define TM_MAX_ERROR_STRING 2048
extern "C" short REGISTERREGION(long transid, long startid, int port, char *hostname, int hostname_length, long startcode, char *regionInfo, int regionInfo_Length);
extern "C" short CREATETABLE(char *pa_tbldesc, int pv_tbldesc_length, char *pv_tblname, char** pv_keys, int pv_numsplits, int pv_keylen, long transid,
				char *&pv_err_str, int &pv_err_len);
extern "C" short REGTRUNCATEONABORT(char *pv_tblname, int pv_tblname_len,
                                    long pv_transid, char* &pv_err_str, 
                                    int &pv_err_len);
extern "C" short DROPTABLE(char *pv_tblname, int pv_tblname_len, long transid, 
                          char* &pv_err_str, int &pv_err_len);
extern "C" short ALTERTABLE(char *pv_tblname, int pv_tblname_len, 
                            char ** pv_tbloptions, int pv_tbloptslen, 
                            int pv_tbloptscnt, long pv_transid, 
                            char* &pv_err_str, int &pv_err_len);
extern "C" short HBASETM_REQUESTREGIONINFO(TM_HBASEREGIONINFO pa_trans[], short *pp_count);
extern "C" short DTM_GETNEXTSEQNUMBLOCK(unsigned int &pp_seqNum_start, unsigned int &pp_seqNum_count);
extern "C" bool DTM_LOCALTRANSACTION(int32 *pp_node, int32 *pp_seqnum);
// Trafodion end //
//
extern "C" short ABORTTRANSACTION();
extern "C" short BEGINTRANSACTION(int *tag);
extern "C" short BEGINTX(int *tag, int timeout=0, int64 type_flags=0);
extern "C" short ENDTRANSACTION();

//ENDTRANSACTION_ERR() is same as ENDTRANSACTION(). However,
//errStr is allocated if errlen is not zero. Caller must deallocate errStr
//by calling DELLAOCATE_ERR.
//Rest of functionality same as ENDTRANSACTION.
extern "C" short ENDTRANSACTION_ERR(char *&errStr, int &errlen);
extern "C" void  DEALLOCATE_ERR(char *&errStr);

extern "C" short STATUSTRANSACTION(short *status, int64 transid = 0);
extern "C" short GETTRANSID(short *transid);
extern "C" short GETTRANSINFO(short *transid, int64 *type_flags);
extern "C" short TMF_DOOMTRANSACTION_ ();
extern "C" short SUSPENDTRANSACTION(short *transid);
extern "C" short RESUMETRANSACTION(int tag);
extern "C" short TEST_TX_COUNT();
extern "C" short JOINTRANSACTION(int64 transid);
extern "C" short TMF_GETTXHANDLE_(short *handle);
extern "C" short TMF_SETTXHANDLE_(short *handle);
extern "C" short TMWAIT();
extern "C" short TMCLIENTEXIT();

// Extended API
extern "C" short GETTRANSID_EXT (TM_Transid_Type *transid);
extern "C" short GETTRANSINFO_EXT (TM_Transid_Type *transid, int64 *type_flags);
extern "C" short JOINTRANSACTION_EXT(TM_Transid_Type *transid);
extern "C" short SUSPENDTRANSACTION_EXT (TM_Transid_Type *transid);

// Management API
extern "C" short DTM_ATTACHRM(short pv_node, char *pp_rmname);
extern "C" short DTM_STATUSSYSTEM( TM_STATUSSYS *pp_status);
extern "C" short DTM_STATUSTM(short pv_node, TMSTATUS *pp_tmstatus);
extern "C" short DTM_STATUSTRANSACTION(int64 pv_transid, TM_STATUS_TRANS *pp_trans);
extern "C" short DTM_STATUSALLTRANS(TM_STATUS_ALL_TRANS pa_trans[], short *pp_count, int pv_node);
extern "C" short DTM_ENABLETRANSACTIONS();
extern "C" short DTM_DISABLETRANSACTIONS(int32 pv_shutdown_level);
extern "C" short DTM_DRAINTRANSACTIONS(int32 pv_node, bool pv_immediate);
extern "C" short LISTTRANSACTION(TM_LIST_TRANS pa_trans[], short *pp_count, int pv_node);
extern "C" short TMSTATS(int pv_node, TM_TMSTATS *pp_tmstats, bool pv_reset = false);
extern "C" short DTM_GETTRANSINFO(int64 pv_transid, int32 *pp_seq_num, int32 *pp_node,
                                  int16 *pp_incarnation_num, int16 *pp_tx_flags,
                                  TM_TT_Flags *pp_tt_flags, int16 *pp_version,
                                  int16 *pp_checksum, int64 *pp_timestamp);
extern "C" short DTM_GETTRANSINFO_EXT(TM_Transid_Type pv_transid, int32 *pp_seq_num, int32 *pp_node,
                                      int16 *pp_incarnation_num, int16 *pp_tx_flags,
                                      TM_TT_Flags *pp_tt_flags, int16 *pp_version,
                                      int16 *pp_checksum, int64 *pp_timestamp);
extern "C" short DTM_GETTRANSIDSTR(int64 pv_transid, char *pp_transidstr);
extern "C" short DTM_GETTRANSIDSTR_EXT(TM_Transid_Type pv_transid, char *pp_transidstr);

// Internal use only!!
extern "C" short DTM_QUIESCE(int32 pv_node); //Use for testing only
extern "C" short DTM_UNQUIESCE(int32 pv_node); //Use for testing only

//Transaction Typing API
// extern "C" short STATUSTRANSACTION (short *status, TM_Transid transid = 0, 
//                                     TM_TT_Flags *flags = NULL);
// extern "C" short SETTRANSACTIONTYPE (TM_Transid transid, TM_TT_Flags flags);

#endif // __TM_H_
