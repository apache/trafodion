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

#ifndef TMINFO_H_
#define TMINFO_H_


#include <limits.h>
#include <string>

#include "tmmap.h"
#include "tmdeque.h"
#include "dtm/tmtransid.h"
#include "tmrecov.h"
#include "tmtx.h"
#include "tmauditobj.h"
#include "tmtimer.h"
#include "tmtxthread.h"
#include "tmpool.h"
//#include "tmlibmsg.h"
#include "tmlogging.h"
#include "tmtxbranches.h"

#include "seabed/trace.h"

#include "tmxatxn.h"


#define MAX_SEQNUM (UINT_MAX-1)

using namespace std;

typedef enum {TM_SYNC_MODE=1, TM_NONSYNC_MODE=2} TM_RUN_MODE;
typedef enum {TM_NORMAL_AUDIT_MODE=1, TM_BUFFER_AUDIT_MODE=2} TM_AUDIT_MODE;
typedef enum {TM_PERF_STATS_ON=1, TM_PERF_STATS_OFF=2} TM_PERF_STATS;
typedef enum {TM_PARTIC_RM_PARTIC_RECOV=1, TM_ALL_RM_PARTIC_RECOV=2} TM_RM_PARTIC_RECOV;

typedef union 
{
   struct
   {
      int32 iv_pid;
      int32 iv_seqnum;
   } k;
   int64 intKey;
} pidKey;

typedef struct _tminfo_h_as_0 {
    TPT_DECL (iv_phandle);
    int32 iv_tag;
    int16 iv_in_use;
    TM_FAIL_RECOV_STATE iv_recov_state;
    CTmTimerEvent * ip_restartTimerEvent;
} Tm_phandle_info;

typedef struct _tminfo_h_as_1 
{
    MESSAGE_HEADER_SQ    iv_msg_hdr;
    int32           iv_sending_tm_nid;
    int32          iv_type;
    bool           iv_startup;
} Tm_Control_Point_Req_Type;

typedef struct _tminfo_h_as_2 
{
    MESSAGE_HEADER_SQ    iv_msg_hdr;
    int32          iv_error;
} Tm_Control_Point_Rsp_Type;

typedef struct _tminfo_h_as_3
{
    MESSAGE_HEADER_SQ    iv_msg_hdr;
    int32                 iv_sending_tm_nid;
} Tm_Perf_Stats_Req_Type;

typedef struct _tminfo_h_as_4
{
    MESSAGE_HEADER_SQ    iv_msg_hdr;
    int32          iv_error;
    int64 iv_tx_count;
    int64 iv_abort_count;
    int64 iv_commit_count;
    int64 iv_tm_initiated_aborts;
    int32 iv_tm_state;
    int32 iv_tx_state;
    int32 iv_hung_tx_count;
    int32 iv_outstanding_tx_count;
    int64 iv_oldest_transid_1;
    int64 iv_oldest_transid_2;
    int64 iv_oldest_transid_3;
    int64 iv_oldest_transid_4;
} Tm_Perf_Stats_Rsp_Type;

typedef struct _tminfo_h_as_5
{
   bool  iv_down_without_sync;
   int32 iv_node_being_recovered;
   bool  iv_list_built;
} Tm_Recov_Sync;

typedef enum 
{
   none = 0,
   transaction,
   worker
} threadModelType;

//
// There is one of these for each TM instance.
//
class TM_Info
{
    private:
       int32            iv_run_mode;
       int32            iv_audit_mode; //1:normal, 2:buffer
       bool             iv_use_tlog;
       int32            iv_perf_stats; //1:ON, 2:OFF
       int16            iv_incarnation_num;
       bool             iv_can_takeover;
       bool             iv_lead_tm;
       bool             iv_lead_tm_takeover;
       int32            iv_lead_tm_nid;
       int32            iv_num_active_txs;
       int32            iv_nid;
       Tm_phandle_info  iv_open_tms[MAX_NODES];
       int32            iv_tms_highest_index_used;
       bool             iv_leadTM_isolated;
       int32            iv_pid;
       Tm_Recov_Sync    iv_recovery[MAX_NODES]; // who is recovering which node
       int32            iv_restarting_tm;
       int32            iv_state;
       int32            iv_sys_recov_state;     //TM_SYS_RECOV_STATE
       int32            iv_sys_recov_lead_tm_nid;
       MS_MON_ShutdownLevel    iv_shutdown_level;
       bool             iv_all_rms_closed;
       bool             iv_shutdown_coordination_started;
       int32            iv_stall_phase_2; // 1 = ABORT, 2 = COMMIT, 3 = ABORT or COMMIT
       int32            iv_rm_wait_time;
       int32            iv_timeout;  //-1 = no timeout, > 0 = timeout in seconds.
       bool             iv_trace_initialized;
       bool             iv_allTMsOpen;
       int32            iv_sync_otag;
       bool             iv_TSE_xa_start;
       // Audit and control points
       bool             iv_write_cp;
       bool             iv_initiate_cp;
       int32            iv_cp_interval;
       int64            iv_audit_seqno;
       int32            iv_stats_interval;
       int32            iv_RMRetry_interval;
       int32            iv_TMRestartRetry_interval;
       int32            iv_cps_in_curr_file;
       int32            iv_trans_hung_retry_interval;
       int64            iv_timerDefaultWaitTime;
       bool             iv_overrideAuditInconsistencyDuringRecovery;
       bool             iv_RMPartic;
       TM_BROADCAST_ROLLBACKS iv_broadcast_rollbacks;
       TS_MODE          iv_TSMode;
       bool             iv_TLOGperTM;
       bool             iv_AllRmParticRecov;
       TM_SYSRECOVERY_MODE iv_SysRecovMode;
       int32            iv_maxRecoveringTxns;

       // Transaction object management:
       CTmPool<class CTmTxBase> *ip_transactionPool;
       TM_MAP           iv_txPidList;
       TM_DEQUE         iv_txdisassociatedQ; // Queue of transactions not associated with a thread.

       // Transaction thread object management:
       CTmPool<class CTxThread> *ip_threadPool;
       int64 iv_txThreadNum;

       int32            iv_lock_count;
       long             iv_lock_owner; //Only set while the calling thread holds the lock
       TM_Mutex        *ip_mutex;  // Semaphore to serialize updates to the object.
      // TM_Mutex         iv_msg_mon_mutex;  // Semaphore to serialize access to msg_mon_ procs
       bool             iv_multithreaded;// true if threading is configured via TM_MULTITHREAD or DTM_SEATRANS_SINGLETHREAD
                                         // environment variable.
       threadModelType  iv_threadModel;         // Threading model.  Could be transaction or worker threads.
       bool             iv_globalUniqueSeqNum;  // true => request global unique
                                                // sequence number from Monitor.
                                                // false => TM generates sequence
                                                // number.
       // Next sequence number is the next transaction sequence number that will be allocated 
       // by this TM when using TM local sequence number generation.
       // The sequence number interval determines how often the TM must allocate a new batch
       // of sequence numbers.
       // SeqNumBlockStart is the first sequence number in the current block.
       // nextSeqNumBlockStart is the first sequence number of the next block.
       unsigned int     iv_nextSeqNum;
       int32            iv_SeqNumInterval;
       unsigned int     iv_SeqNumBlockStart;
       unsigned int     iv_nextSeqNumBlockStart;

       // Timestamps and Control Point states
       Ctimeval         iv_lastAuditCPTime;          // Time of last control point write.
       Ctimeval         iv_lastAuditRolloverTime;    // Time of last audit rollover.
       Ctimeval         iv_lastAuditThresholdTime;   // Time of last audit threshold notice.

       // Recovery object. Used by the Lead TM to recover transactions after a Cluster failure.
       TM_Recov        *ip_ClusterRecov;

       //Recovery objects.  Used by the Lead TM to recover transactions after a node failure.
       TM_Recov        *ip_NodeRecov[MAX_NODES];

       TM_Mutex         iv_recovery_mutex;          // mutex to serialize xa_recover to TSEs.

       TM_DEQUE         iv_TMUP_wait_list;
       Tm_Rsp_Msg_Type  iv_TMUP_Wait_reply;  // Reply buffer. Used to reply to the client.

       // Timer thread
       CTmTimer *ip_tmTimer; 
       CTmAuditObj *ip_tmAuditObj; 

       // iv_syncDataList is an array of TM_MAPs. Each entry is a Tm_Tx_Sync_Data.
       TM_MAP           iv_syncDataList[MAX_NODES]; 
       TM_MAP           iv_synctags;
       // Broadcast sequence numbers are used to distingush the packets
       // for a single logical broadcast sync message.
       // The iv_sendingBroadcastSeqNum is used by the lead TM when
       // packing the sync data into packets to send.
       // The iv_receivingBroadcastSeqNum is used by the receiving TM
       // to decide when a new logical broadcast sync message is arriving.
       int32            iv_sendingBroadcastSeqNum;
       int32            iv_receivingBroadcastSeqNum;

       CTmStats         iv_stats;
       TMCOUNTS         iv_counts;

       TM_DEQUE         iv_tx_start_list;     
       // DTM stats - end

       // Private member functions

    public:
 
       TM_Info();
       ~TM_Info();
       void initialize();

       int32 init_slot (MS_Mon_Process_Info_Type *pp_info, 
                        int32 pv_rmid, bool pv_is_ax_reg);
       void init_and_recover_rms();
       int32 recover_tm(int32 pv_nid);
       int32 restart_tm_process(int32 pv_nid);
       void restart_tm_process_helper(int32 pv_nid);
       void reset_restartTimerEvent (int32 pv_nid);
       CTmTimerEvent* get_restartTimerEvent (int32 pv_nid);
       void schedule_init_and_recover_rms();
       void schedule_recover_system();
       void send_tm_state_information();

       char * txStatetoa(int32 pv_state);
       char * tmStatetoa(int32 pv_state);

       int32 take_over_abort(TM_Txid_Internal *pv_transid, bool pv_do_phase1, 
                             TM_TX_STATE pv_state);
       int32 take_over_commit(TM_Txid_Internal *pv_transid, TM_TX_STATE pv_state);
       bool do_take_over(TM_MAP *pp_dataList);
       void set_recovery_start(int32 pv_nid);
       void set_recovery_end(int32 pv_nid);

       // Locking for serialization
       void lock();
       void unlock();

       void recovery_lock();
       void recovery_unlock();

       // methods
       void send_system_status(TM_STATUSSYS *pp_system_status);
       bool perf_stats_on() {if (iv_perf_stats == TM_PERF_STATS_ON) return true; return false;}
       void inc_tx_count() {iv_counts.iv_tx_count++; iv_counts.iv_current_tx_count++;}
       void inc_abort_count() {iv_counts.iv_abort_count++; iv_counts.iv_current_tx_count--;}
       void inc_begin_count() {iv_counts.iv_begin_count++;}
       void inc_commit_count() {iv_counts.iv_commit_count++; iv_counts.iv_current_tx_count--;}
       void inc_tm_initiated_aborts() {iv_counts.iv_tm_initiated_aborts++;}
       void inc_tx_hung_count() 
       {iv_counts.iv_tx_hung_count++; iv_counts.iv_current_tx_hung_count++;}
       void dec_tx_hung_count() {iv_counts.iv_current_tx_hung_count--;}
       void clearCounts() {
          iv_counts.iv_abort_count = iv_counts.iv_commit_count = iv_counts.iv_tm_initiated_aborts = 0;
          iv_counts.iv_tx_hung_count = iv_counts.iv_current_tx_hung_count;
          iv_counts.iv_tx_count = iv_counts.iv_begin_count = iv_counts.iv_current_tx_count;
       }


       int64 tx_count () { return iv_counts.iv_tx_count;}
       int64 begin_count() { return iv_counts.iv_begin_count; }
       int64 abort_count() { return iv_counts.iv_abort_count; }
       int64 commit_count() { return iv_counts.iv_commit_count; }
       int32 current_tx_count () { return iv_counts.iv_current_tx_count; }
       int64 tm_initiated_aborts () { return iv_counts.iv_tm_initiated_aborts; }
       int32 tx_hung_count () { return iv_counts.iv_tx_hung_count; }
       int32 current_tx_hung_count() { return iv_counts.iv_current_tx_hung_count; }
       void remove_tx_from_oldest_list(CTmTxBase *pp_tx) { iv_tx_start_list.remove(pp_tx->transid()); }
       bool oldest_timestamp(TM_Txid_Internal *pp_tx) 
                             { TM_Txid_Internal *lp_tx = NULL;
                               iv_tx_start_list.lock();
                               lp_tx = (TM_Txid_Internal*)iv_tx_start_list.get_firstFIFO();
                               if (lp_tx)
                               {
                                  memcpy (pp_tx, lp_tx, sizeof (TM_Txid_Internal));
                                  iv_tx_start_list.unlock();
                                  return true;
                               }
                               iv_tx_start_list.unlock();
                               return false;  }
       void convert_tx_to_str(std::string &pp_str, TM_Txid_Internal &pr_tx, bool pv_empty = false);

                      

       // methods to access sync array 
       void  add_sync_data (int32 pv_nid, Tm_Tx_Sync_Data *pp_data);
       int32 broadcast_sync_data (int32 pv_nid);
       int32 broadcast_sync_packet(TPT_PTR( pp_TMphandle),
                                   int32 pv_node,
                                   Tm_Broadcast_Req_Type *pp_req,
                                   Tm_Broadcast_Rsp_Type *pp_rsp,
                                   int32 pv_start);
       TM_MAP * get_node_syncDataList (int32 pv_nid);
       Tm_Tx_Sync_Data * get_sync_data(Tm_Tx_Sync_Data *pp_data);
       void  pack_sync_buffer (Tm_Broadcast_Req_Type *pp_data, int32 pv_node, int32 pv_startAt);
       void  remove_sync_data(Tm_Tx_Sync_Data *pp_data);
       int32 size (int32 pv_nid);
       void  unpack_sync_buffer (Tm_Broadcast_Req_Type *pp_data, int32 pv_nid);
 
       // methods to access transaction pool
       CTmPool<CTmTxBase> *transactionPool() {return ip_transactionPool;}
       void * new_tx(int32 pv_creator_nid, int32 pv_creator_pid, int32 pv_node = -1, int32 pv_seqnum = -1,
                     void * (*constructPoolElement)(int64) = NULL);
       void * import_tx (TM_Txid_Internal *pv_transid, TM_TX_STATE pv_state=TM_TX_STATE_NOTX, TM_TX_TYPE pv_txnType=TM_TX_TYPE_DTM);
       int32        add_tx(CTmTxBase *pp_tx);
       void      ** get_all_txs(int64 *pv_count);
       void * get_tx(int32 pv_node, int32 pv_seq);
       void * get_tx(TM_Txid_Internal *pv_transid);
       void * get_tx(int64 pv_txnId);
       void * getFirst_tx();
       void * getNext_tx();
       void         getEnd_tx();
       void * getFirst_tx_byPid(int32 pv_pid);
       void * getNext_tx_byPid(int32 pv_pid);
       void         getEnd_tx_byPid();
       void         remove_tx (CTmTxBase * pp_tx);
       void cleanup(void * pp_txn);
       TM_DEQUE * txdisassociatedQ() {return &iv_txdisassociatedQ;}
       bool check_for_queued_requests(CTxThread * pp_thread);

       // Methods to access sync tags.  Note that sync tags replace the use of sync handles.
       int32 add_sync_otag(Tm_Sync_Type_Transid *pp_data);
       Tm_Sync_Type_Transid *get_sync_otag(int32 pv_tag);
       void remove_sync_otag(int32 pv_tag);

       // helper methods for managing transaction threads
       CTmPool<CTxThread> *threadPool() {return ip_threadPool;}
       CTxThread * new_thread(CTmTxBase *pp_Txn);
       bool release_thread(CTxThread * pp_Thread);
       CTxThread * get_thread(char * pp_name);
       void terminate_all_threads();
       void stopTimerEvent();
       void stopAuditThread();

       // audit and control points
       void  start_backwards_scan();
       void  end_backwards_scan();
       Addr  read_audit_rec();
       int64 audit_seqno() { return iv_audit_seqno; }
       void audit_seqno(int64 pv_audit_seqno) { iv_audit_seqno = pv_audit_seqno; }
       void  release_audit_rec();
       void  initialize_adp() {ip_tmAuditObj->initialize_adp();}
       unsigned int tm_new_seqNum();        // Allocate next txn sequence number
       unsigned int  setNextSeqNumBlock();  // Get/set the next sequence number block in
                                            // registry.
       unsigned int  getSeqNumBlock(int32 pv_blockSize);  // Get/set the next sequence number block for local Transactions.
       bool  tm_active_seqNum(int32 pv_seqNum); // Check whether this seqnum is in use
              
       void  write_control_point(bool pv_cp_only, bool pv_startup = false);
       int32 write_rollover_control_point();
       void  addControlPointEvent();
       int32 startup_read_audit();
       void  write_trans_state(TM_Txid_Internal *pv_transid, TM_TX_STATE pv_state, 
                               int32 pv_abort_flags, bool pv_hurry);
       void  write_all_trans_state();
       void check_for_rollover(int32 pv_notification);

       // write shutdown audit record for clean shutdown
       void  write_shutdown();

       // general get and set
       void  can_takeover(bool pv_can_takeover);
       bool  can_takeover( );
       void  close_tm( int32 pv_nid) 
       {  lock();
          iv_open_tms[pv_nid].iv_in_use = 0;
          iv_allTMsOpen = false;
          unlock();}
       bool  tm_is_up(int32 pv_nid) {return (iv_open_tms[pv_nid].iv_in_use != 0);}
       void  cp_interval(int32 pv_interval);
       int32 cp_interval();
       void  stats_interval(int32 pv_interval);
       int32 stats_interval();
       int32 tms_highest_index_used() { return iv_tms_highest_index_used;}
       int16 incarnation_num() {return iv_incarnation_num;} 
       int32 SeqNumBlockStart () {return iv_SeqNumBlockStart;}
       void tm_new_seqNumBlock(int pv_blockSize, unsigned int *pp_start, int *pp_count);


       void  lead_tm(bool pv_lead_tm);
       bool  lead_tm();
       void  lead_tm_takeover(bool pv_takeover);
       bool  lead_tm_takeover();
       void  lead_tm_nid(int32 pv_nid);
       int32 lead_tm_nid();
      
       int32 mode() {return iv_run_mode;}
       void  nid (int32 pv_nid);
       int32 nid ();
       bool  leadTM_isolated() {return iv_leadTM_isolated;}
       int32 trans_hung_retry_interval() {return iv_trans_hung_retry_interval;}
       void  trans_hung_retry_interval(int32 pv_interval) {iv_trans_hung_retry_interval = pv_interval;}
       int64 timerDefaultWaitTime() {return iv_timerDefaultWaitTime;}
       void  timerDefaultWaitTime(int64 pv_timer) {iv_timerDefaultWaitTime = pv_timer;}
       bool overrideAuditInconsistencyDuringRecovery() {return iv_overrideAuditInconsistencyDuringRecovery;}
       void overrideAuditInconsistencyDuringRecovery(bool flag) {iv_overrideAuditInconsistencyDuringRecovery=flag;}
       TM_BROADCAST_ROLLBACKS broadcast_rollbacks() {return iv_broadcast_rollbacks;}
       void broadcast_rollbacks(TM_BROADCAST_ROLLBACKS flag) {iv_broadcast_rollbacks=flag;}
       int32 restarting_tm();
       void  restarting_tm(int32 pv_nid);
       bool  down_without_sync (int32 pv_down_node);
       void  down_without_sync (int32 pv_down_node, bool pv_value);
       int32 node_being_recovered (int32 pv_down_node);
       void  node_being_recovered (int32 pv_down_node, int32 pv_take_over_tm);
       void  num_active_txs_inc ();
       void  num_active_txs_dec ();
       int32 num_active_txs ();
       void  open_other_tms();
       int32  open_restarted_tm(int32  pv_nid);
       void dummy_link_to_refresh_phandle(int32 pv_nid);     
       SB_Phandle_Type *
       get_opened_tm_phandle(int32 pv_index);
       void  pid (int32 pv_pid);
       int32 pid ();
       void recovery_list_built (int32 pv_down_node, bool pv_list_built);
       bool recovery_list_built (int32 pv_down_node);
       void  state (int32 pv_state);
       int32 state ();
       bool state_shutdown();
       void  set_trace (int32 pv_detail);
       void  set_xa_trace (char *pp_string);
       void  init_tracing(bool pv_unique, const char *pp_trace_file, int32 pv_detail);
       void rm_wait_time (int32 pv_rm_wait_time) {iv_rm_wait_time = pv_rm_wait_time;}
       int32 rm_wait_time () {return iv_rm_wait_time;}
       void stall_phase_2 (int32 pv_stall_phase_2) {iv_stall_phase_2 = pv_stall_phase_2;}
       int32 stall_phase_2 () {return iv_stall_phase_2;}
       void  all_rms_closed(bool pv_all_closed);
       bool  all_rms_closed();
       void  shutdown_level(MS_MON_ShutdownLevel pv_shutdown_level);
       MS_MON_ShutdownLevel shutdown_level();
       void  shutdown_coordination_started(bool pv_shutdown_coordination_started);
       bool  shutdown_coordination_started();
       void set_sys_recov_status(int32 pv_sys_recov_state, int32 pv_sys_recov_lead_tm_nid);
       int32 sys_recov_state();
       
       void tm_fail_recov_state(int32 pv_nid, TM_FAIL_RECOV_STATE pv_state)
       {
           lock();
           iv_open_tms[pv_nid].iv_recov_state = pv_state;
           unlock();
       }
       TM_FAIL_RECOV_STATE tm_fail_recov_state(int32 pv_nid) {return iv_open_tms[pv_nid].iv_recov_state;}
       bool all_tms_recovered();
       bool recover_failed_tm(int32 pv_nid, int32 pv_rm_wait_time);
       void error_shutdown_abrupt(int32 pv_error);

       int32 sys_recov_lead_tm_nid();
       void  multithreaded(bool pv_threaded);
       bool  multithreaded();
       void threadModel(threadModelType pv_threadModel);
       void  use_tlog(bool pv_use_tlog);
       bool  use_tlog();
       threadModelType  threadModel();
       int32 max_trans();
       void inc_broadcastSeqNum();
       bool write_cp() { return iv_write_cp;}
       void initiate_cp(bool pv_initiate_cp) {iv_initiate_cp = pv_initiate_cp;}
       void RMRetry_interval(int32 pv_interval);
       int32 RMRetry_interval();
       void TMRestartRetry_interval(int32 pv_interval);
       int32 TMRestartRetry_interval();

       bool tm_test_verify (int32 pv_nid);

       TM_Recov * ClusterRecov();
       void ClusterRecov(TM_Recov * pv_recov);

       TM_Recov * NodeRecov(int32 pv_node);
       void NodeRecov(TM_Recov * pv_recov, int32 pv_node);

       TM_DEQUE * TMUP_wait_list();
       void tm_up();
       void wake_TMUP_waiters(short pv_error);
       bool TSE_xa_start() {return iv_TSE_xa_start;}
       
       bool tmTrace(int level) {if (iv_trace_level >= level) return true; return false;}
       int32    iv_trace_level;

       CTmStats * stats() {return &iv_stats;}
       bool tm_stats() {return iv_stats.collectStats();}

       int32 timeout() {return iv_timeout;}
       void timeout(int32 pv_timeout)
       {
          lock();
          iv_timeout = pv_timeout;
          unlock();
       }
       bool allTMsOpen() {return iv_allTMsOpen;}

       Ctimeval *lastAuditCPTime() {return &iv_lastAuditCPTime;}
       Ctimeval *lastAuditRollbackTime() {return &iv_lastAuditRolloverTime;}
       Ctimeval *lastAuditThreasholdTime() {return &iv_lastAuditThresholdTime;}

       bool RMPartic() {return iv_RMPartic;}
       void RMPartic(bool pv_RMpartic) {iv_RMPartic=pv_RMpartic;}
       TS_MODE TSMode() { return iv_TSMode;}
       void TSMode(TS_MODE pv_TSMode) {iv_TSMode=pv_TSMode;}
       bool TLOGperTM() {return iv_TLOGperTM;}
       void TLOGperTM(bool pv_TLOGperTM) {iv_TLOGperTM=pv_TLOGperTM;}
       bool AllRmParticRecov() {return iv_AllRmParticRecov;}
       void AllRmParticRecov(bool pv_AllRmParticRecov) {iv_AllRmParticRecov=pv_AllRmParticRecov;}
       TM_SYSRECOVERY_MODE SysRecovMode() {return iv_SysRecovMode;}
       void SysRecovMode(TM_SYSRECOVERY_MODE pv_recovMode) {iv_SysRecovMode = pv_recovMode;}
       int32 maxRecoveringTxns() {return iv_maxRecoveringTxns;}
       void maxRecoveringTxns(int32 pv_max) {iv_maxRecoveringTxns = pv_max;}

       // Timer thread related
       CTmTimer *tmTimer() {return ip_tmTimer;}
       void tmTimer(CTmTimer *pp_timer) {ip_tmTimer = pp_timer;}
       void CheckFailed_RMs(char *pp_rmname = NULL);
       void abort_all_active_txns();
       void abort_active_txns(int32 pv_rmid);

       CTmTimerEvent * addTimerEvent(CTmTxMessage *pp_msg, int pv_delayInterval);
       CTmTimerEvent * addTMRestartRetry(int32 pv_nid, int32 pv_waitTime);
       void cancelTMRestartEvent(int32 pv_nid);
       CTmTimerEvent * addTMRecoveryWait(int32 pvnid, int32 pv_delay);

       int32 sendAllTMs(CTmTxMessage * pp_msg);
       int32 attachRm(CTmTxMessage * pp_msg);
       void set_txnsvc_ready(int32 pv_ready);
       int32 enableTrans(CTmTxMessage * pp_msg);
       int32 disableTrans(CTmTxMessage * pp_msg);
       int32 drainTrans(CTmTxMessage * pp_msg);
       void addShutdownPhase1WaitEvent(CTmTxMessage * pp_msg);
       void cancelShutdownPhase1WaitEvent();
       void ShutdownPhase1Wait(CTmTxMessage *pp_msg);

       // Audit thread related
       CTmAuditObj *tmAuditObj() {return ip_tmAuditObj;}
       void tmAuditObj(CTmAuditObj *pp_AuditObj) {ip_tmAuditObj = pp_AuditObj;}
       short link(SB_Phandle_Type    *pp_phandle,
                  int                *pp_msgid,
                  // short              *reqctrl,            Unused
                  // int                 reqctrlsize,
                  // short              *replyctrl,
                  // int                 replyctrlmax,
                  char               *pp_reqdata,
                  int                 pv_reqdatasize,
                  char               *pp_replydata,
                  int                 pv_replydatamax,
                  long                pv_linkertag,
                  short               pv_pri,
                  // short               xmitclass,      Unused
                  short               pv_linkopts,
                  int32               pv_maxretries = TM_LINKRETRY_WAITFOREVER,
                  TM_Transid         *pv_transid = NULL);
};

inline CTmTimerEvent* TM_Info::get_restartTimerEvent (int32 pv_nid)
{
    return iv_open_tms[pv_nid].ip_restartTimerEvent;
}

inline void TM_Info::reset_restartTimerEvent (int32 pv_nid)
{
    iv_open_tms[pv_nid].ip_restartTimerEvent = NULL;
}

// ----------------------------------------------------------
// General get and set inline methods
// ----------------------------------------------------------
inline void TM_Info::can_takeover(bool pv_can_takeover)
{ 
    lock();
    iv_can_takeover =  pv_can_takeover; 
    unlock();
}

inline bool TM_Info::can_takeover( )
{ 
    return iv_can_takeover; 
}

inline void  TM_Info::cp_interval(int32 pv_interval)
{ 
    lock();
    iv_cp_interval = pv_interval; 
    unlock();
}

inline int32 TM_Info::cp_interval()
{
    return iv_cp_interval;
}

inline void  TM_Info::stats_interval(int32 pv_interval)
{ 
    lock();
    iv_stats_interval = pv_interval; 
    unlock();
}

inline int32 TM_Info::stats_interval()
{
    return iv_stats_interval;
}

inline void  TM_Info::RMRetry_interval(int32 pv_interval)
{ 
    lock();
    iv_RMRetry_interval = pv_interval; 
    unlock();
}

inline int32 TM_Info::RMRetry_interval()
{
    return iv_RMRetry_interval;
}

inline void  TM_Info::TMRestartRetry_interval(int32 pv_interval)
{ 
    lock();
    iv_TMRestartRetry_interval = pv_interval; 
    unlock();
}

inline int32 TM_Info::TMRestartRetry_interval()
{
    return iv_TMRestartRetry_interval;
}

inline void TM_Info::lead_tm(bool pv_lead_tm)
{ 
    lock();
    iv_lead_tm = pv_lead_tm; 
    unlock();
}

inline bool TM_Info::lead_tm()
{ 
    return iv_lead_tm; 
}

inline void TM_Info::lead_tm_takeover(bool pv_takeover)
{ 
    lock();
    iv_lead_tm_takeover = pv_takeover; 
    unlock();
}

inline bool TM_Info::lead_tm_takeover()
{ 
    return iv_lead_tm_takeover; 
}

inline void TM_Info::lead_tm_nid(int32 pv_nid)
{ 
    lock();
    iv_lead_tm_nid = pv_nid; 
    unlock();
}

inline int32 TM_Info::lead_tm_nid()
{ 
    return iv_lead_tm_nid; 
}

inline void TM_Info::nid (int32 pv_nid)
{ 
    lock();
    iv_nid = pv_nid; 
    unlock();
}

inline int32 TM_Info::nid ()
{ 
    return iv_nid; 
}

inline int32 TM_Info::restarting_tm() {return iv_restarting_tm;}
inline void  TM_Info::restarting_tm(int32 pv_nid) {iv_restarting_tm = pv_nid;}

inline int32 TM_Info::node_being_recovered (int32 pv_down_node)
{ 
    return iv_recovery[pv_down_node].iv_node_being_recovered; 
}

inline void  TM_Info::node_being_recovered (int32 pv_down_node, int32 pv_take_over_tm)
{ 
    lock();
    iv_recovery[pv_down_node].iv_node_being_recovered = pv_take_over_tm;
    unlock();
}

inline bool TM_Info::down_without_sync (int32 pv_down_node)
{ 
    return iv_recovery[pv_down_node].iv_down_without_sync; 
}

inline void  TM_Info::down_without_sync (int32 pv_down_node, bool pv_value)
{ 
    lock();
    iv_recovery[pv_down_node].iv_down_without_sync = pv_value;
    unlock();
}

inline bool TM_Info::recovery_list_built (int32 pv_down_node)
{
   return iv_recovery[pv_down_node].iv_list_built;
}

inline void TM_Info::recovery_list_built (int32 pv_down_node, bool pv_list_built)
{
    lock();
    iv_recovery[pv_down_node].iv_list_built = pv_list_built;
    unlock();
}

inline void  TM_Info::num_active_txs_inc ()
{ 
    //lock();
    iv_num_active_txs++; 
    //unlock();
}

inline void TM_Info::num_active_txs_dec ()
{ 
    // Note the caller must lock
    //lock();
    iv_num_active_txs--; 
    //unlock();
}

inline int32 TM_Info::num_active_txs ()
{ 
    return iv_num_active_txs; 
}

inline void  TM_Info::pid (int32 pv_pid)
{ 
    lock();
    iv_pid = pv_pid; 
    unlock();
}

inline int32 TM_Info::pid ()
{ 
    return iv_pid; 
}

inline int32 TM_Info::state ()
{ 
    return iv_state; 
}

inline bool TM_Info::state_shutdown()
{
   if (iv_state == TM_STATE_SHUTTING_DOWN ||
        iv_state == TM_STATE_SHUTDOWN_FAILED ||
        iv_state == TM_STATE_SHUTDOWN_COMPLETED)
      return true;
   else
      return false;
}

inline void TM_Info::all_rms_closed(bool pv_all_closed)
{
    lock();
    iv_all_rms_closed = pv_all_closed;
    unlock();
}

inline bool TM_Info::all_rms_closed()
{
    return iv_all_rms_closed;
}

inline void TM_Info::shutdown_level(MS_MON_ShutdownLevel pv_shutdown_level)
{
    lock();
    iv_shutdown_level = pv_shutdown_level;
    unlock();
}

inline MS_MON_ShutdownLevel TM_Info::shutdown_level()
{
    return iv_shutdown_level;
}

inline void TM_Info::shutdown_coordination_started(bool pv_shutdown_coordination_started)
{
   lock();
   iv_shutdown_coordination_started = pv_shutdown_coordination_started;
   unlock();
}

inline bool TM_Info::shutdown_coordination_started()
{
   return iv_shutdown_coordination_started;
}

inline int32 TM_Info::sys_recov_state()
{
   return iv_sys_recov_state;
}

inline int32 TM_Info::sys_recov_lead_tm_nid()
{
   return iv_sys_recov_lead_tm_nid;
}

inline bool TM_Info::use_tlog()
{
   return iv_use_tlog;
}

inline void TM_Info::use_tlog(bool pv_use_tlog)
{
   lock();
   iv_use_tlog = pv_use_tlog;
   unlock();
}

inline bool TM_Info::multithreaded()
{
   return iv_multithreaded;
}

inline void TM_Info::multithreaded(bool pv_threaded)
{
   lock();
   iv_multithreaded = pv_threaded;
   unlock();
}

inline threadModelType TM_Info::threadModel()
{
   return iv_threadModel;
}

inline void TM_Info::threadModel(threadModelType pv_threadModel)
{
   lock();
   iv_threadModel = pv_threadModel;
   unlock();
}

inline int32 TM_Info::max_trans() { return transactionPool()->get_maxPoolSize(); }

inline void TM_Info::inc_broadcastSeqNum()
{
   if (iv_sendingBroadcastSeqNum >= (int)0xffffffff)
      iv_sendingBroadcastSeqNum = 1;
   else
      iv_sendingBroadcastSeqNum++;
}

inline TM_Recov *TM_Info::ClusterRecov() { return ip_ClusterRecov; }
inline void TM_Info::ClusterRecov(TM_Recov * pv_recov) { ip_ClusterRecov = pv_recov; }

inline TM_Recov *TM_Info::NodeRecov(int32 pv_node) { return ip_NodeRecov[pv_node]; }
inline void TM_Info::NodeRecov(TM_Recov * pv_recov, int32 pv_node) { ip_NodeRecov[pv_node] = pv_recov; }

inline TM_DEQUE * TM_Info::TMUP_wait_list() { return &iv_TMUP_wait_list; }

//----------------------------------------------------------------------------
// TM_Info::tm_active_seqNum
// Purpose : Checks the active transactions to see if this sequence number
// is already in use.  If there is an active transaction with this sequence
// number then return true, otherwise return false.
//----------------------------------------------------------------------------
inline bool TM_Info::tm_active_seqNum(int32 pv_seqNum)
{
   CTmTxBase *lp_tx = (CTmTxBase *) transactionPool()->get(pv_seqNum);

   if (lp_tx == NULL)
      return false;
   else
      return true;
} //tm_active_seqNum

inline void TM_Info::recovery_lock()
{
   iv_recovery_mutex.lock();
}

inline void TM_Info::recovery_unlock()
{
   iv_recovery_mutex.unlock();
}

// Externals
extern TM_Info gv_tm_info;
extern CTmTxBranches gv_RMs;
extern int32  gv_system_tx_count;         
#endif



