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

#include <sys/types.h>
#include <sys/time.h>
#include <limits.h>

//#include "dumapp.h"

#include "trafconf/trafconfig.h"
#include "dtm/tm.h"
#include "dtm/xa.h"
#include "rm.h"
#include "../../inc/fs/feerrors.h" //legacy error codes for SQL

#define TPT_DECL(name)       SB_Phandle_Type name
#define TPT_DECL2(name,size) SB_Phandle_Type name[size]
#define TPT_PTR(name)        SB_Phandle_Type *name

// The message descriptor table size for the TM.
// Note that this is higher than the Seabed maximums
// to allow for the maximum possible sending mds
// in a TM.  For M5 this is 257 TSEs * 100 transaction
// threads.  A better solution is needed in M6.
#define SEABED_MAX_SETTABLE_RECVLIMIT_TM 25700
#define SEABED_MAX_SETTABLE_SENDLIMIT_TM 25700

// MAXPROCESSNAME set at 16 for no good reason
#define MAXPROCESSNAME 16
// MAX_TRANS_SLOTS is the maximum number of transactions a TMLIB can have active at any time.
#define MAX_TRANS_SLOTS 1024

// MAX_TRANS_PER_SYNC is the maximum number of transactions that can be synchronized in a
// broadcast message packet.
#define MAX_TRANS_PER_SYNC 1024

// Temporary values for testing only
// MAX_NUM_TRANS is the default maximum number of active concurrent transactions per TM
// Once the TM reaches this number of transactions, no new transactions will begin
// until some complete and begintransaction will received error FETOOMANYTRANSBEGINS (83).
// The maximum value for MAX_NUM_TRANS is around 8500 (crash at 8640 which represents around
// 4GB memory (sizeof(TM_TX_Info) ~ MAX_OPEN_RMS * sizeof(RM_Info_TSEBranch)).
// STEADYSTATE_LOW_TRANS is the default transaction pool low water mark
// Below this value, a new transaction object is always instantiated and added to the pool.
// Once the total active transaction count gets above this value, new transactions
// will take an object from the transactionPool freeList.
// STEADYSTATE_HIGH_TRANS is the default transactionPool high water mark
// Above this, transaction objects will be deleted once the transaction completes.
#define MAX_NUM_TRANS           5000
#define STEADYSTATE_LOW_TRANS   5
#define STEADYSTATE_HIGH_TRANS  1000
#define MAX_NODES               TC_NODES_MAX
#define MAX_SYNC_TXS 50
#define MAX_TXN_TAGS UINT_MAX


// Transaction abort timeout default value.
// TM_NEVERABORT (-1) = wait forever
// > 0 = timeout in seconds.
#define TX_ABORT_TIMEOUT TM_NEVERABORT

// MAX_NUM_THREADS is the maximum number of active transaction threads/TM.
// The pool management algorithm is the same as for transaction objects. 
// Looks like we have a memory leak in CTmPool, so
// I've set the STEADYSTATE_HIGH_THREADS to MAX_NUM_THREADS.
#define MAX_NUM_THREADS           100
#define STEADYSTATE_LOW_THREADS   1
#define STEADYSTATE_HIGH_THREADS  100
// Temporary value to reduce the number of concurrent transactions
// during system recovery at startup. The value reflects the 
// lack of message descriptors:
// 257 TSEs * 15 transactions = 3855 send mds max
// Current max mds is 4096.
#define MAX_THREADS_DURING_SYSRECOVERY 15

// RM Message object pool defaults
// TODO set values for production.
// Looks like we have a memory leak in CTmPool, so
// I've set the STEADYSTATE_HIGH_RMMESSAGES to MAX_NUM_RMMESSAGES.
#define MAX_NUM_RMMESSAGES          10000
#define STEADYSTATE_LOW_RMMESSAGES  10
#define STEADYSTATE_HIGH_RMMESSAGES 10000

// Thread Model 
// Possible values are 0 = transaction threads
//                 and 1 = worker threads
#define THREAD_MODEL 1

// Interval timers default in minutes!!!!
#define TM_CP_DEFAULT 2     // Control Point interval (Lead TM only)
#define TM_STATS_DEFAULT 1  
#define TM_RMRETRY_DEFAULT 1 // RM open retry interval  

// TM failure restart retry interval in seconds.
#define TM_TMRESTARTRETRY_DEFAULT 1 

// Shutdown Phase 1 wait loop timer.
#define TM_SHUTDOWNP1WAIT_DEFAULT 100 // .1 sec

// TM thread Pauses for this time when link gets a FENOLCB  or other
// retriable error.
// In some link retry loops we timeout after TM_LINKRETRY_RETRIES retries.
#define TM_LINKRETRY_PAUSE 3000 // 3 second
#define TM_LINKRETRY_RETRIES 100
#define TM_LINKRETRY_WAITFOREVER -1

// Default wait interval for non-lead TMs main loop in 10 msec units.
#define TM_DEFAULT_WAIT_INTERVAL 10 // 1 sec

// TM_STATS != 0 sets internal TM statistics gathering on.
// TM_STATS_INTERVAL is in minutes.
#define TM_STATS 1
#define TM_STATS_INTERVAL 1

#define MAX_RECEIVE_BUFFER 200000
// low number for testing
#define MAX_TRANS_FOR_CP 100 
// wake up interval is 30 seconds for polling
#define LEAD_DTM_WAKEUP_INTERVAL 5000 

// MAX_RM_WAIT_TIME is the maximum time the TM will wait for an RM (TSE) to respond
// to an xa request.  This had better be long enough to ensure that the RM is really
// non-responsive because if this timer expires for ANY RM request the transaction
// will be aborted or placed in a hung state!
//#define MAX_RM_WAIT_TIME 24000 //4 minutes in 10ms intervals
//#define MAX_RM_WAIT_TIME 100000 //1000 seconds (16.6 min) in 10ms intervals
#define MAX_RM_WAIT_TIME -1 //Wait forever for the TSE to respond
// MAX_TMTIMER_WAIT_TIME is the maximum time 
//#define MAX_TMTIMER_WAIT_TIME 6000 //60 seconds in 10ms intervals
//#define MAX_TMTIMER_WAIT_TIME 30000 //5 min/300 seconds in 10ms intervals
#define MAX_TMTIMER_WAIT_TIME 20*6000 //OSB: 20 min in 10ms intervals !! to allow for xa_recover replies to arrive
//TODO: We need to either move recovery, or CPs out of the timer thread 
// as we hit a conflict in the wait times, or allow the wait timeout to trickle 
// up to the time thread procedure to allow it to be rescheduled.

// TRANS_HUNG_RETRY_INTERVAL is the time the TM will wait between retrying completion
// for hung transactions.
#define TRANS_HUNG_RETRY_INTERVAL 120000 // 2 minutes in msecs.

// TM_RM_DOWN_LOGEVENT_INTERVAL is the interval between event log writes for the
// DTM_XATM_RETRY_RM_SEND event which otherwise would be written 10 times for 
// each transaction every 2 minutes when an RM is down.
#define TM_RM_DOWN_LOGEVENT_INTERVAL 1000 // 10 retries*100 transactions ~ every 2 minutes.

// TIMERTHREAD_WAIT is the time the timer thread waits when the timer list is empty.
// This can be quite long as it will be woken when a timer event is posted.
// -1 = wait forever.
#define TIMERTHREAD_WAIT -1 // Wait forever. Time in 10 msec intervals.

// Maximum number of times a message can be resent to a TSE.
// A value of 2 means original attempt + 1 retry.
#define MAX_TSE_SEND_RETRIES 100
// Resend initial sleep time. This sets the sleep time for 
// retries of sends to the TSE.
#define XATM_MSG_INIT_SLEEPTIME 3000 // 3 second

// TSE_XA_START_DEFAULT determines whether the tm will send
// xa_start messages to TSEs.  false = don't send, true = send xa_start.
#define TSE_XA_START_DEFAULT true

// earlyCommitReply allows the TM to reply to commit requests
// early and not wait for phase 2 to complete.  This is implicit for TSE branches,
// so the value only applies to HBase branches.
#define TM_DEFAULT_EARLY_COMMIT_REPLY false

// The original implementation had only a single TLOG per environment.  Set
// DEFAULT_TLOG_PER_TM or the DTM_TLOG_PER_TM registry value or environment variable
// to true if configuring a TLOG/TM process.
#define DEFAULT_TLOG_PER_TM false

// Set DEFAULT_ALL_RM_PARTIC_RECOV or the DTM_ALL_RM_PARTIC_RECOV registry value or
// environment variable to true to have all TSE marked as participating in all transactions
// during node recovery the current behavior because of a bug.  Once proven fixed, will
// change the default behavior back to false
#define DEFAULT_ALL_RM_PARTIC_RECOV true

// TM_TEST_PAUSE uses this interval to pause a transaction thread at specific
// points for testing failure id specific states.  To set this build with TM_DEBUG=1, 
// and specify the TM_TEST_PAUSE_STATE environemnt variable in ms.env. Eg:
// TM_TEST_PAUSE_STATE=1  - active transactions
//#define TM_TEST_PAUSE_INTERVAL 30000 // 30 seconds
#define TM_TEST_PAUSE_INTERVAL  (60000*5) // 5 minutes


// Error codes for HBase JNI errors returned
typedef enum {
    COMMIT_CONFLICT = 5
} HBASE_ERRCODE;

// Participation: All TSEs participate in the transaction by default before
// revision M6.  
typedef enum {
    PARTIC_NONE = 0,    // ax_reg used to enlist an RM explicitly
    PARTIC_ALL_RMS = 1  // xa_start used to enlist all RMs in every transaction
} RM_PARTIC;
#define DEFAULT_RM_PARTIC PARTIC_NONE;

// Timestamps: Because gettimeofday() is a very expensive call, we provide a
// way to avoid calling it as much as possible.
typedef enum {
    TS_MINIMUM = 1,
    TS_BEGINONLY,   // Timestamp only on BEGINTRANSACTION
    TS_FAST,        // Not implemented yet - probably will be a course grained timer
    TS_DETAIL       // Required for detailed statistics.
} TS_MODE;
#define DEFAULT_TS_MODE TS_BEGINONLY;

// DEFAULT_TRANSACTION_DISTRIBUTION determines where new transactions are begun.
typedef enum {
    DIST_NOT_SET = 0,
    NODE_LOCAL_BEGINS = 1,
    CLUSTER_WIDE_BEGINS = 2
} TRANSACTION_DISTRIBUTION;
#define DEFAULT_TRANSACTION_DISTRIBUTION NODE_LOCAL_BEGINS

// MSG_LINK_ priorities
#define TSE_LINK_PRIORITY 200
#define TM_TM_LINK_PRIORITY 210
#define TM_BROADCAST_LINK_PRIORITY 180
#define TMLIB_LINK_PRIORITY 0
#define XARMLIB_LINK_PRIORITY 0

// EID is used to identify different pool elements and whether they are inuse, 
// on the free list, or deleted.
#define EID_SIZE 12

// TM_DEFAULT_SEQ_NUM_INTERVAL is the default value for the number of 
// sequence numbers a TM grabs at a time.  The TM grabs a number of 
// sequence numbers based in this interval at a time.  Each time it 
// gets a new batch, it increments the registry value DTM_NEXT_SEQNUM_BLOCK
// for this TM process.
// You may override this value by setting the DTM_SEQ_NUM_INTERVAL
// environment variable.
#define TM_DEFAULT_SEQ_NUM_INTERVAL 10000

// OVERRIDE_AUDIT_INCONSISTENCY is the default value for DTM_OVERRIDE_AUDIT_INCONSISTENCY.
// This determines whether the Lead TM will ignore additional trans state records
// found during the backward scan of the TLOG during recovery.  If true, the most
// recent trans state record will always be actioned and any additional trans state
// records found will be ignored.  This must not be set as the default but used only
// to recover an audit trail containing inconsistencies.  By default we want recovery
// to check the audit trail for consistency back to the second control point.
#define OVERRIDE_AUDIT_INCONSISTENCY false

// TM_DEFAULT_SYSRECOVERY_MODE
// This environment variable indicates whether system recovery will optimise the recovery
// are a clean shutdown.  If mode is CLEAN_SHUTDOWN_OPTIMIZE then system recovery will
// terminate once a TM_Shutdown record is encountered and NOT send xa_recover requests to
// the RMs.  Note that this can only be set as an environment variable at Seaquest startup.
typedef enum {
    CLEAN_SHUTDOWN_OPTIMIZE = 0,
    ALWAYS_SEND_XA_RECOVER  = 1
} TM_SYSRECOVERY_MODE;
const TM_SYSRECOVERY_MODE TM_DEFAULT_SYSRECOVERY_MODE = ALWAYS_SEND_XA_RECOVER;

// TM_DEFAULT_MAXRECOVERINGTXNS
// This environment variable provides the maximum number of transactions that will
// be scheduled for recovery at any point in time.  Any indoubt transactions in excess of 
// this number will be queued for resolution in the TM_Recov objects txnStateList.
// Currently we allow 10000 RM messages in the pool and 1025 RMs. During system recovery
// we will run out of RM message elements if the number of indoubt transactions being processed
// at any point exceeds 10000 / 1025 = 9.
#define TM_DEFAULT_MAXRECOVERINGTXNS 9

// TM_DEFAULT_BROADCAST_ROLLBACKS
// If DTM_BROADCAST_ROLLBACKS is set, the TM will broadcast all xa_rollback requests to
// all available RMs (TSEs) regardless of which TSEs participated in the transaction.
// This may be necessary to avoid producing orphan transaction branches in TSEs after
// a failure/recovery, but we don't understand the window where this occurs yet.
typedef enum {
   TM_BROADCAST_ROLLBACKS_NO  = 0,
   TM_BROADCAST_ROLLBACKS_YES,      // Log an event if an RM responded that wasn't a participant
   TM_BROADCAST_ROLLBACKS_DEBUG     // As for _YES, but also abort to produce a core.
} TM_BROADCAST_ROLLBACKS;
const TM_BROADCAST_ROLLBACKS TM_DEFAULT_BROADCAST_ROLLBACKS = TM_BROADCAST_ROLLBACKS_NO;

// These thresholds avoid problems with looping too much during the 
// multi-pass prepare and rollback processing.
#define TM_MULTIPASS_REPORT_THRESHOLD 3
#define TM_MULTIPASS_LOOP_THRESHOLD 10

#define TM_NO_ERR      0
#define TM_ERR        -1

// Transaction Abort flag offsets
#define TM_TX_ABORTFLAGS_OFFSET_TM    0x0
#define TM_TX_ABORTFLAGS_OFFSET_TSE   0x2
#define TM_TX_ABORTFLAGS_OFFSET_APPL  0x4
#define TM_TX_ABORTFLAGS_OFFSET_HEUR  0x8


// Request and Reply types
typedef enum {
    TM_MSG_TYPE_NULL                     = 0,
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
    TM_MSG_TYPE_TEST_TX_COUNT_REPLY      = 218,
    TM_MSG_TYPE_JOINTRANSACTION          = 219,
    TM_MSG_TYPE_JOINTRANSACTION_REPLY    = 220,
    TM_MSG_TYPE_SUSPENDTRANSACTION       = 221,
    TM_MSG_TYPE_SUSPENDTRANSACTION_REPLY = 222,

    TM_MSG_TYPE_ROLLOVER_CP              = 289,
    TM_MSG_TYPE_ROLLOVER_CP_REPLY        = 290,
    TM_MSG_TYPE_BROADCAST                = 291,
    TM_MSG_TYPE_BROADCAST_REPLY          = 292,
    TM_MSG_TYPE_CP                       = 293,
    TM_MSG_TYPE_CP_REPLY                 = 294,

    TM_MSG_TYPE_SHUTDOWN_COMPLETE        = 295,
    TM_MSG_TYPE_SHUTDOWN_COMPLETE_REPLY  = 296,
    TM_MSG_TYPE_SHUTDOWN_EXIT            = 297,
    TM_MSG_TYPE_SHUTDOWN_EXIT_REPLY      = 298,
    TM_MSG_TYPE_SHUTDOWN_AUDIT           = 299,
    TM_MSG_TYPE_SHUTDOWN_AUDIT_REPLY     = 300,

    TM_MSG_TYPE_DOOMTX                   = 301,
    TM_MSG_TYPE_DOOMTX_REPLY             = 302,

    TM_MSG_TYPE_WAIT_TMUP                = 303,
    TM_MSG_TYPE_WAIT_TMUP_REPLY          = 304,

    TM_MSG_TYPE_TSE_DOOMTX               = 305,
    TM_MSG_TYPE_TSE_DOOMTX_REPLY         = 306,

    TM_MSG_TYPE_LISTTRANSACTION          = 307,
    TM_MSG_TYPE_LISTTRANSACTION_REPLY    = 308,

    TM_MSG_TYPE_TMSTATS                  = 309,
    TM_MSG_TYPE_TMSTATS_REPLY            = 310,

    TM_MSG_TYPE_TMPERFSTATS              = 311,
    TM_MSG_TYPE_TMPERFSTATS_REPLY        = 312,

    TM_MSG_TYPE_STATUSTM                 = 313,
    TM_MSG_TYPE_STATUSTM_REPLY           = 314,

    TM_MSG_TYPE_LEADTM                   = 315,
    TM_MSG_TYPE_LEADTM_REPLY             = 316,

    TM_MSG_TYPE_ENABLETRANS              = 317,
    TM_MSG_TYPE_ENABLETRANS_REPLY        = 318,

    TM_MSG_TYPE_DISABLETRANS             = 319,
    TM_MSG_TYPE_DISABLETRANS_REPLY       = 320,

    TM_MSG_TYPE_DRAINTRANS               = 321,
    TM_MSG_TYPE_DRAINTRANS_REPLY         = 322,

    TM_MSG_TYPE_STATUSTRANSMGMT          = 323,
    TM_MSG_TYPE_STATUSTRANSMGMT_REPLY    = 324,

    TM_MSG_TYPE_STATUSSYSTEM             = 325,
    TM_MSG_TYPE_STATUSSYSTEM_REPLY       = 326,

    TM_MSG_TYPE_CALLSTATUSSYSTEM         = 327,
    TM_MSG_TYPE_CALLSTATUSSYSTEM_REPLY   = 328,

    TM_MSG_TYPE_GETTRANSINFO             = 329,
    TM_MSG_TYPE_GETTRANSINFO_REPLY       = 330,

    TM_MSG_TYPE_ATTACHRM                 = 331,
    TM_MSG_TYPE_ATTACHRM_REPLY           = 332,

    TM_MSG_TYPE_STATUSALLTRANSMGT        = 333,
    TM_MSG_TYPE_STATUSALLTRANSMGT_REPLY  = 334,

    TM_MSG_TYPE_REGISTERREGION           = 335, // TOPL
    TM_MSG_TYPE_REGISTERREGION_REPLY     = 336, // TOPL

    TM_MSG_TYPE_REQUESTREGIONINFO        = 337, // TOPL
    TM_MSG_TYPE_REQUESTREGIONINFO_REPLY  = 338, // TOPL

    TM_MSG_TYPE_GETNEXTSEQNUMBLOCK       = 339, // TOPL
    TM_MSG_TYPE_GETNEXTSEQNUMBLOCK_REPLY = 340, // TOPL

    TM_MSG_TYPE_DDLREQUEST               = 341,
    TM_MSG_TYPE_DDLREQUEST_REPLY         = 342,

    TM_MSG_TYPE_QUIESCE                  = 9001, // Testing only!
    TM_MSG_TYPE_QUIESCE_REPLY            = 9002,

    TM_MSG_TXTHREAD_INITIALIZE           = 1001,
    TM_MSG_TXTHREAD_RELEASE              = 1002,
    TM_MSG_TXTHREAD_TERMINATE            = 1003,

    TM_MSG_TXINTERNAL_ROLLBACK           = 1011,
    TM_MSG_TXINTERNAL_REDRIVEROLLBACK    = 1012,
    TM_MSG_TXINTERNAL_REDRIVECOMMIT      = 1013,
    TM_MSG_TXINTERNAL_ABORTCOMPLETE      = 1014,
    TM_MSG_TXINTERNAL_ENDCOMPLETE        = 1015,
    TM_MSG_TXINTERNAL_ENDFORGET          = 1016,
    TM_MSG_TXINTERNAL_REDRIVESYNC        = 1017,
    TM_MSG_TXINTERNAL_BEGINCOMPLETE      = 1018,
    TM_MSG_TXINTERNAL_ALLRMSREPLIED      = 1019,

    TM_MSG_TXINTERNAL_CONTROLPOINT       = 1020,
    TM_MSG_TXINTERNAL_STATS              = 1021,
    TM_MSG_TXINTERNAL_RMRETRY            = 1022, 
    TM_MSG_TXINTERNAL_SHUTDOWNP1_WAIT    = 1023,
    TM_MSG_TXINTERNAL_TMRESTART_RETRY    = 1024,
    TM_MSG_TXINTERNAL_RECOVERY_WAIT      = 1025,
    TM_MSG_TXINTERNAL_INITIALIZE_RMS     = 1026,
    TM_MSG_TXINTERNAL_SYSTEM_RECOVERY    = 1027,
    TM_MSG_TXINTERNAL_SONAR_HEARTBEAT    = 1028
} TM_MSG_TYPE;

// TM_TM_MSG_OFFSET is used to distingush messages arriving
// from another TM to client requests.
#define TM_TM_MSG_OFFSET 10000 

// Transaction branch states
typedef enum {
    TMBR_S0 = 0, TM_BR_STATE_NOTX             = 0,
    TMBR_S1 = 1, TM_BR_STATE_ACTIVE           = 1,
    TMBR_S11= 2, TM_BR_STATE_ACTIVE_RETRY     = 2,
    TMBR_S2 = 3, TM_BR_STATE_IDLE             = 3,
    TMBR_S3 = 4, TM_BR_STATE_PREPARED         = 4,
    TMBR_S31= 5, TM_BR_STATE_PREPARED_RETRY   = 5,
    TMBR_S4 = 6, TM_BR_STATE_ROLLBACK         = 6,
    TMBR_S41= 7, TM_BR_STATE_ROLLBACK_HUNG    = 7,
    TMBR_S5 = 8, TM_BR_STATE_HEURISTIC        = 8,
    TMBR_T  = 9, TM_BR_STATE_TERMINATE        = 9
} TM_BR_STATE;
#define TMBR_FSM_NUM_STATES 10

// Transaction thread states
typedef enum {
   TM_TX_TH_STATE_IDLE           = 0,
   TM_TX_TH_STATE_ACTIVE         = 1
} TM_TX_TH_STATE;

// TM system states 
typedef enum {
    TM_STATE_NOTRUNNING              = -1, // Tm not up or not running
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
    TM_STATE_WAITING_RM_OPEN,              // Waiting for RM opens to complete before declaring the TM up.
} TM_STATE;

// TM system recovery state
typedef enum {
    TM_SYS_RECOV_STATE_INIT          = 200,
    TM_SYS_RECOV_STATE_START,
    TM_SYS_RECOV_STATE_END
} TM_SYS_RECOV_STATE;

// TM failure recovery state
typedef enum {
    TM_FAIL_RECOV_STATE_INITIAL      = 300,
    TM_FAIL_RECOV_STATE_RUNNING,
    TM_FAIL_RECOV_STATE_COMPLETE
} TM_FAIL_RECOV_STATE;

// Values for the SQ_TXNSVC_READY registry value
#define TXNSVC_DOWN        0        // the transaction service is unavailable.
#define TXNSVC_UP          1        // the transaction service is available.
#define TXNSVC_SHUTDOWN    2        // transaction service is unavailable, shutdown phase 2.

// Transaction types
typedef enum {
      TM_TX_TYPE_DTM    = 901,
      TM_TX_TYPE_XARM
} TM_TX_TYPE;

// Resource Manager types
typedef enum {
      TM_RM_TYPE_TSE    = 1901,
      TM_RM_TYPE_HBASE
} TM_RM_TYPE;

// ------------------------
// request structs
// ------------------------

typedef struct registerregion_req {
    TM_Transid_Type     iv_transid;
    int                 iv_pid;
    int                 iv_nid;
    int                 iv_port;
    char ia_hostname[TM_MAX_REGIONSERVER_STRING];
    int                 iv_hostname_length;
    long                iv_startcode;
    long                iv_startid;
    char ia_regioninfo2[TM_MAX_REGIONSERVER_STRING];
    int                 iv_regioninfo_length;
} Register_Region_Req_Type;

typedef enum {
    TM_DDL_CREATE     = 0,
    TM_DDL_DROP       = 1,
    TM_DDL_TRUNCATE   = 2,
    TM_DDL_ALTER      = 3
} TM_DDL_REQ;

typedef struct ddlmessage_req {
    TM_Transid_Type     iv_transid;
    char                ddlreq[TM_MAX_DDLREQUEST_STRING];
    int                 ddlreq_len;
    int                 crt_numsplits;
    int                 crt_keylen;
    int                 alt_numopts;
    int                 alt_optslen;
    TM_DDL_REQ          ddlreq_type;
    char                key_eyecatch[10];
} Ddl_Message_Req_Type;

typedef struct hbaseregioninfo_req {
} HbaseRegionInfo_Req_Type;

typedef struct _tmlibmsg_h_as_0 {
    TM_Transid_Type     iv_transid;
    int                 iv_tag;
    int                 iv_pid;
    int                 iv_nid;
} Abort_Trans_Req_Type;

typedef struct _tmlibmsg_h_as_1 {
   TM_Transid_Type      iv_txid;
   int                  iv_rmid;
   int64                iv_flags;
   int                  iv_tm_nid;
} Ax_Reg_Req_Type;

typedef struct _tmlibmsg_h_as_2 {
    int iv_pid;
    int iv_nid;
    int iv_abort_timeout;
    int64 iv_transactiontype_bits;
} Begin_Trans_Req_Type;

typedef struct _tmlibmsg_h_as_3 {
    TM_Transid_Type  iv_transid;
    int              iv_tag;
    int              iv_pid;
    int              iv_nid;
} End_Trans_Req_Type;

typedef struct _tmlibmsg_h_as_4 {
    TM_Transid_Type iv_transid;
} Get_Transid_Req_Type;

typedef struct _tmlibmsg_h_as_5 {
    TM_Transid_Type iv_transid;
    int             iv_pid;
    int             iv_nid;
    bool            iv_coord_role;
} Join_Trans_Req_Type;

typedef struct _tmlibmsg_h_as_6 {
    TM_Transid_Type iv_transid;
} Status_Trans_Req_Type;

typedef struct list_trans_req {
} List_Trans_Req_Type;

typedef struct status_alltrans_req{
}StatusAllTrans_Req_Type;

typedef struct tmstats_req  {
   bool iv_reset;
} Tmstats_Req_Type;

typedef struct statustm_req {
} Statustm_Req_Type;

typedef struct attachrm_req {
    char ia_rmname[10];
} Attachrm_Req_Type;

typedef struct statustransmgmt_req {
   TM_Transid_Type iv_transid;
} Status_TransM_Req_Type;

typedef struct gettransinfo_req {
        int64 iv_transid;
} GetTransInfo_Req_Type;


typedef struct enabletrans_req {
} Enabletrans_Req_Type;

typedef struct disabletrans_req {
    int iv_shutdown_level;
} Disabletrans_Req_Type;

typedef struct draintrans_req {
    bool iv_immediate;
} Draintrans_Req_Type;

typedef struct quiesce_req {
    bool iv_stop;
} Quiesce_Req_Type;

typedef struct leadtm_req {
} Leadtm_Req_Type;

typedef struct Tm_Sys_Status_Req_Type
{
    MESSAGE_HEADER_SQ    iv_msg_hdr;
    int32                iv_sending_tm_nid;
} Tm_Sys_Status_Req_Type;

typedef struct Tm_CSys_Status_Req_Type
{
    MESSAGE_HEADER_SQ    iv_msg_hdr;
    int32                iv_sending_tm_nid;
} Tm_CSys_Status_Req_Type;

typedef struct _tmlibmsg_h_as_7 {
    TM_Transid_Type iv_transid;
    int             iv_pid;
    int             iv_nid;
    bool            iv_coord_role;
} Suspend_Trans_Req_Type;

typedef struct _tmlibmsg_h_as_8 {
} Wait_TmUp_Req_Type;

typedef struct _tmlibmsg_h_as_9 {
} Test_Tx_Count;

typedef struct _tmlibmsg_h_as_10 {
    void *ip_txObject;
} TxTh_Initialize_Req_Type;

typedef struct _tmlibmsg_h_as_11 {
} TxTh_Release_Req_Type;

typedef struct _tmlibmsg_h_as_12 {
} TxTh_Terminate_Req_Type;

typedef struct _tmlibmsg_h_as_13 {
    bool iv_takeover_or_shutdown;
} TxInternal_Rollback_Req_Type;

typedef struct _tmlibmsg_h_as_14 {
} TxInternal_RedriveRollback_Req_Type;

typedef struct _tmlibmsg_h_as_15 {
} TxInternal_RedriveCommit_Req_Type;

typedef struct _tmlibmsg_h_as_16 {
} TxInternal_AbortComplete_Req_Type;

typedef struct _tmlibmsg_h_as_17 {
} TxInternal_EndComplete_Req_Type;

typedef struct _tmlibmsg_h_as_18 {
} TxInternal_EndForget_Req_Type;

typedef struct _tmlibmsg_h_as_19 {
} TxInternal_BeginComplete_Req_Type;

typedef struct _tmlibmsg_h_as_20 {
    int32 iv_nid;
} TxInternal_TMRestart_Req_Type;

typedef struct _tmlibmsg_h_as_21 {
    int32 iv_nid;
} TxInternal_TMRecovery_Req_Type;

typedef struct _tmlibmsg_h_as_22 {
    int32 iv_block_size;
} TM_GetNextSeqNum_Req_Type;

typedef struct Tm_RolloverCP_Req_Type {
    MESSAGE_HEADER_SQ    iv_msg_hdr;
    int32 iv_nid;
    int64 iv_sequence_no;
} Tm_RolloverCP_Req_Type;

typedef struct _tmlibmsg_h_as_23 {
    MESSAGE_HEADER_SQ iv_msg_hdr;
    union {
        Abort_Trans_Req_Type    iv_abort_trans;
        Ax_Reg_Req_Type         iv_ax_reg;
        Begin_Trans_Req_Type    iv_begin_trans;
        End_Trans_Req_Type      iv_end_trans;
        Get_Transid_Req_Type    iv_get_transid;
        Join_Trans_Req_Type     iv_join_trans;
        List_Trans_Req_Type     iv_list_trans;
        StatusAllTrans_Req_Type iv_status_alltrans;
        Tmstats_Req_Type        iv_tmstats;
        Tm_Sys_Status_Req_Type  iv_status_sysm;
        Tm_CSys_Status_Req_Type iv_call_status_sysm;
        Statustm_Req_Type       iv_statustm;
        Attachrm_Req_Type       iv_attachrm;
        Status_TransM_Req_Type  iv_status_transm;
        GetTransInfo_Req_Type   iv_gettransinfo;
        Leadtm_Req_Type         iv_leadtm;
        Enabletrans_Req_Type    iv_enabletrans;
        Disabletrans_Req_Type   iv_disabletrans;
        Draintrans_Req_Type     iv_draintrans;
        Quiesce_Req_Type        iv_quiesce;
        Status_Trans_Req_Type   iv_status_trans;
        Suspend_Trans_Req_Type  iv_suspend_trans;
        Wait_TmUp_Req_Type      iv_wait_tmup;
        Test_Tx_Count           iv_count;
        Tm_RolloverCP_Req_Type  iv_control_point;
        Register_Region_Req_Type iv_register_region;
        Ddl_Message_Req_Type    iv_ddl_request;
        HbaseRegionInfo_Req_Type iv_hbase_regioninfo;
        TM_GetNextSeqNum_Req_Type iv_GetNextSeqNum;

        // TM internal Tx Thread events
        TxTh_Initialize_Req_Type iv_init_txthread;
        TxTh_Release_Req_Type  iv_release_txthread;
        TxTh_Terminate_Req_Type  iv_term_txthread;

        // TM internal Tx events
        TxInternal_Rollback_Req_Type iv_rollback_internal;
        TxInternal_RedriveRollback_Req_Type iv_redriverollback_internal;
        TxInternal_RedriveCommit_Req_Type iv_redrivecommit_internal;
        TxInternal_AbortComplete_Req_Type iv_abort_complete_internal;
        TxInternal_EndComplete_Req_Type iv_end_complete_internal;
        TxInternal_EndForget_Req_Type iv_end_forget_internal;
        TxInternal_BeginComplete_Req_Type iv_begin_complete_internal;
        TxInternal_TMRestart_Req_Type iv_tmrestart_internal;
        TxInternal_TMRecovery_Req_Type iv_tmrecovery_internal;

        // XARM requests
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
} Tm_Req_Msg_Type;


// Used for sending shutdown message by Lead TM
// to other TMs, all TSEs and the AMP.
typedef struct _tmlibmsg_h_as_31 
{
   MESSAGE_HEADER_SQ    iv_msg_hdr;
   int                  iv_type;
} Tm_Shutdown_Req_Type;

typedef struct _tmlibmsg_h_as_32 
{
   MESSAGE_HEADER_SQ    iv_msg_hdr;
   int                  iv_error;
} Tm_Shutdown_Rsp_Type;

// ------------------------
// reply structs
// ------------------------

typedef enum {
    TM_RSP_ERROR_OK     = 0,
    TM_RSP_ERROR_NOT_OK = 1
} TM_RSP_ERROR_TYPE;

//TOPL
typedef struct registerregion_rsp {
} Register_Region_Rsp_Type;

typedef struct ddlrequest_rsp {
    char	iv_err_str[TM_MAX_ERROR_STRING];
    int     iv_err_str_len;
} Ddl_Request_Rsp_Type;

typedef struct hbaseregioninfo_rsp {
  TM_HBASEREGIONINFO iv_status;
  int                iv_count;
  TM_HBASEREGIONINFO iv_trans[TM_MAX_LIST_TRANS];
} HbaseRegionInfo_Rsp_Type;

typedef struct _tmlibmsg_h_as_33 {
} Abort_Trans_Rsp_Type;

typedef struct _tmlibmsg_h_as_34 {
   XID     iv_xid;
   int64   iv_flags;
   int16   iv_TM_incarnation_num;
   int32   iv_LeadTM_nid;
} Ax_Reg_Rsp_Type;

typedef struct _tmlibmsg_h_as_35 {
    TM_Transid_Type iv_transid;
    int             iv_tag; // ignored
} Begin_Trans_Rsp_Type;

typedef struct _tmlibmsg_h_as_36 {
  char      iv_err_str[TM_MAX_ERROR_STRING];
  int       iv_err_str_len;
} End_Trans_Rsp_Type;

typedef struct _tmlibmsg_h_as_37 {
   TM_Transid_Type iv_transid;
} Get_Transid_Rsp_Type;

typedef struct _tmlibmsg_h_as_38 {
   int  iv_tm_pid;
} Join_Trans_Rsp_Type;

typedef struct _tmlibmsg_h_as_39 {
   short iv_status;
} Status_Trans_Rsp_Type;

typedef struct list_trans_rsp {
   int             iv_count;
   TM_LIST_TRANS   iv_trans[TM_MAX_LIST_TRANS];
} List_Trans_Rsp_Type;

typedef struct status_all_trans_rsp {
   int             iv_count;
   TM_STATUS_ALL_TRANS iv_trans[TM_MAX_LIST_TRANS];
} StatusAllTrans_Rsp_Type;

typedef struct tmstats_rsp {
   TM_TMSTATS iv_stats;
} Tmstats_Rsp_Type;

typedef struct statustm_rsp {
   TMSTATUS iv_status;
} Statustm_Rsp_Type;

typedef struct attachrm_rsp {
} Attachrm_Rsp_Type;

typedef struct statustransmgmt_rsp {
   TM_STATUS_TRANS iv_status_trans;
} Status_TransM_Rsp_Type;

typedef struct gettransinfo_rsp {
    int32 iv_seqnum;
    int32 iv_node;
    int16 iv_incarnation_num;
    int16 iv_tx_flags;
    TM_TT_Flags iv_tt_flags;
    int16 iv_version;
    int16 iv_checksum;
    int64 iv_timestamp;
} GetTransInfo_Rsp_Type;

typedef struct leadtm_rsp {
    int32 iv_node;
} Leadtm_Rsp_Type;

typedef struct Tm_Sys_Status_Rsp_Type
{
    TM_STATUSSYS iv_status_system;
} Tm_Sys_Status_Rsp_Type;

typedef struct Tm_CSys_Status_Rsp_Type
{
    TM_STATUSSYS iv_status_system;
} Tm_CSys_Status_Rsp_Type;

typedef struct enabletrans_rsp {
} Enabletrans_Rsp_Type;

typedef struct disabletrans_rsp {
} Disabletrans_Rsp_Type;

typedef struct draintrans_rsp {
} Draintrans_Rsp_Type;

typedef struct quiesce_rsp {
} Quiesce_Rsp_Type;

typedef struct _tmlibmsg_h_as_40 {
} Suspend_Trans_Rsp_Type;

typedef struct _tmlibmsg_h_as_41 {
   int iv_count;
} Test_Tx_Count_Rsp_Type;

typedef struct _tmlibmsg_h_as_42 {
} Wait_TmUp_Rsp_Type;

typedef struct _tmlibmsg_h_as_43 {
   uint32 iv_seqNumBlock_start;
   int32 iv_seqNumBlock_count;
} TM_GetNextSeqNum_Rsp_Type;

typedef struct Tm_RolloverCP_Rsp_Type {
   
} Tm_RolloverCP_Rsp_Type;

typedef struct _tmlibmsg_h_as_44 {
    MESSAGE_HEADER_SQ  iv_msg_hdr;
    union {
        Abort_Trans_Rsp_Type    iv_abort_trans;
        Ax_Reg_Rsp_Type         iv_ax_reg;
        Begin_Trans_Rsp_Type    iv_begin_trans;
        End_Trans_Rsp_Type      iv_end_trans;
        Get_Transid_Rsp_Type    iv_get_transid;
        Join_Trans_Rsp_Type     iv_join_trans;
        List_Trans_Rsp_Type     iv_list_trans;
        StatusAllTrans_Rsp_Type iv_status_alltrans;
        Tmstats_Rsp_Type        iv_tmstats;
        Leadtm_Rsp_Type         iv_leadtm;
        Tm_Sys_Status_Rsp_Type  iv_status_sysm;
        Tm_CSys_Status_Rsp_Type iv_cstatus_sysm;
        Statustm_Rsp_Type       iv_statustm;
        Attachrm_Rsp_Type       iv_attachrm;
        Status_TransM_Rsp_Type  iv_status_transm;
        GetTransInfo_Rsp_Type   iv_gettransinfo;
        Enabletrans_Rsp_Type    iv_enabletrans;
        Disabletrans_Rsp_Type   iv_disabletrans;
        Draintrans_Rsp_Type     iv_draintrans;
        Quiesce_Rsp_Type        iv_quiesce;
        Status_Trans_Rsp_Type   iv_status_trans;
        Suspend_Trans_Rsp_Type  iv_suspend_trans;
        Wait_TmUp_Rsp_Type      iv_wait_tmup;
        Test_Tx_Count_Rsp_Type  iv_count;
        Tm_RolloverCP_Rsp_Type  iv_control_point;
        Ddl_Request_Rsp_Type    iv_ddl_response;
#ifndef HP_CLOSED_SOURCE_1
        HbaseRegionInfo_Rsp_Type iv_hbaseregion_info;
        TM_GetNextSeqNum_Rsp_Type iv_GetNextSeqNum;
#endif

        // XARM Responses
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
} Tm_Rsp_Msg_Type;

#define tm_max(a,b) (((a) > (b)) ? (a) : (b))
enum { TM_MAX_DATA = tm_max(sizeof(Tm_Req_Msg_Type), sizeof(Tm_Rsp_Msg_Type)) };

#define TM_MsgSize(MsgType) \
      (sizeof(MESSAGE_HEADER_SQ) + sizeof(MsgType))

#define rsp_type(req_type) (TM_MSG_TYPE) (req_type+1)

#define new_req(MsgLength) \
    (Tm_Req_Msg_Type *) new char[MsgLength]
#define new_rsp(MsgLength) \
    (Tm_Rsp_Msg_Type *) new char[MsgLength]

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#endif //__TMLIBMSG_H_



