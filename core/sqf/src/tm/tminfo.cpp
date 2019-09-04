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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// TM includes
#include "tminfo.h"
#include "tmlogging.h"
#include "tmregistry.h"
#include "tmtxthread.h"
#include "tmrecov.h"
#include "tmtxbase.h"
#include "hbasetm.h"

// Seabed includes
#include "seabed/pctl.h"
#include "seabed/pevents.h"
#include "seabed/trace.h"

#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <iostream>
#include <sstream>

using std::stringstream;
using std::string;

// for use only in this file
typedef struct _tminfo_cpp_as_0 
{
   int32 iv_tag;
   int32 iv_msgid;
   int32 iv_nid;
} pid_msgid_struct;

// -------------------------------------------------------------
// TM_Info
// Purpose : Initialize the TM info.
//           This is a single TM entity instance
// -------------------------------------------------------------
       
TM_Info::TM_Info()
   :iv_stats(TM_STATS, TM_STATS_INTERVAL)
{

   // Mutex attributes: Recursive = true, ErrorCheck=false
   ip_mutex = new TM_Mutex(true, false);
   iv_trace_level = 0;

   iv_state   = TM_STATE_INITIAL;
   iv_lock_count = iv_cps_in_curr_file = 0;
   iv_lock_owner = -1;

   lock();

   iv_all_rms_closed = true;
   iv_counts.iv_tx_count = iv_counts.iv_abort_count =
   iv_counts.iv_commit_count = iv_counts.iv_tm_initiated_aborts = 
   iv_counts.iv_tx_hung_count = iv_rm_wait_time = iv_stall_phase_2 = 0;

   iv_run_mode = iv_incarnation_num = iv_lead_tm_nid = iv_sys_recov_state = 
   iv_sys_recov_lead_tm_nid = iv_stats_interval = iv_RMRetry_interval =  
   iv_TMRestartRetry_interval = iv_SeqNumBlockStart = iv_nextSeqNumBlockStart = 
   iv_counts.iv_current_tx_count = iv_counts.iv_current_tx_hung_count = 0;
   iv_shutdown_coordination_started = iv_TSE_xa_start = false; 
   iv_threadModel = none;
   iv_shutdown_level = MS_Mon_ShutdownLevel_Undefined;

   iv_audit_seqno = 0;
   iv_audit_mode = TM_BUFFER_AUDIT_MODE; // M10+ default.
   iv_perf_stats = TM_PERF_STATS_OFF;
   iv_num_active_txs = 0;
   iv_timeout = TX_ABORT_TIMEOUT;
   iv_allTMsOpen = false;
   iv_lead_tm = iv_can_takeover = iv_leadTM_isolated = iv_lead_tm_takeover = false;
   iv_nid = iv_pid = -1;
   iv_cp_interval = -1;
   iv_sync_otag = 0;
   iv_sendingBroadcastSeqNum = 1;
   iv_receivingBroadcastSeqNum = 0;
   iv_write_cp = iv_initiate_cp = iv_trace_initialized = false;
   iv_RMPartic = DEFAULT_RM_PARTIC;
   iv_TSMode = DEFAULT_TS_MODE;
   iv_TLOGperTM = DEFAULT_TLOG_PER_TM;
   iv_SysRecovMode = TM_DEFAULT_SYSRECOVERY_MODE;
   iv_maxRecoveringTxns = TM_DEFAULT_MAXRECOVERINGTXNS;
   iv_AllRmParticRecov = DEFAULT_ALL_RM_PARTIC_RECOV;

   memset(iv_open_tms, 0, sizeof(Tm_phandle_info) * MAX_NODES);

   restarting_tm(-1);
   for (int lv_idx = 0; lv_idx < MAX_NODES; lv_idx++)
   {
       iv_recovery[lv_idx].iv_node_being_recovered = -1;
       iv_recovery[lv_idx].iv_list_built = false;
       iv_recovery[lv_idx].iv_down_without_sync = false;
       iv_open_tms[lv_idx].iv_in_use = false;
       iv_open_tms[lv_idx].iv_recov_state = TM_FAIL_RECOV_STATE_INITIAL;
       NodeRecov(NULL, lv_idx);
   }
   
   iv_tms_highest_index_used = 0;

   for (int i=0; i<MAX_NODES; i++)
      iv_syncDataList[i].clear();

   ClusterRecov(NULL);

   memset(&iv_TMUP_Wait_reply, 0, sizeof(Tm_Rsp_Msg_Type));

   // Initialize transactionPool
   ip_transactionPool = new CTmPool<CTmTxBase>(tm_stats(), MAX_NUM_TRANS,
                                 STEADYSTATE_LOW_TRANS, STEADYSTATE_HIGH_TRANS);
   iv_globalUniqueSeqNum = false; // By default use local TM seq number generation
   iv_SeqNumInterval = TM_DEFAULT_SEQ_NUM_INTERVAL;
   iv_nextSeqNum = 1;

   // Initialize threadPool
   ip_threadPool = new CTmPool<CTxThread>(tm_stats(), MAX_NUM_THREADS, 
                               STEADYSTATE_LOW_THREADS, STEADYSTATE_HIGH_THREADS);
   iv_txThreadNum = 1;

   ip_tmAuditObj = NULL;
   ip_tmTimer = NULL;

   iv_trans_hung_retry_interval = TRANS_HUNG_RETRY_INTERVAL;
   iv_timerDefaultWaitTime = TIMERTHREAD_WAIT;
   iv_overrideAuditInconsistencyDuringRecovery = OVERRIDE_AUDIT_INCONSISTENCY;
   iv_broadcast_rollbacks = TM_DEFAULT_BROADCAST_ROLLBACKS;
   iv_stats.clearCounters();

   // Default no pause
   gv_pause_state = -1; 
   gv_pause_state_type = TX_PAUSE_STATE_TYPE_FIXED;
   ms_getenv_int("TM_TEST_PAUSE_STATE", &gv_pause_state); 
   ms_getenv_int("TM_TEST_PAUSE_STATE_TYPE", (int *) &gv_pause_state_type); 
   unlock();
}

// ------------------------------------
// ~TM_Info
// Purpose : not much to do here
// ------------------------------------
TM_Info::~TM_Info() 
{
   TMTrace (2, ("TM_Info::~TM_Info ENTRY: lock count = %d.\n", iv_lock_count));

   char la_tm_name[8];
   sprintf(la_tm_name, "$tm%d", iv_nid);

   while (ip_mutex->lock_count() > 0)
       SB_Thread::Sthr::sleep(10); // 1/100th of a second

   //if (iv_lock_count > 0)
   //   unlock();


   if (iv_state != TM_STATE_INITIAL)
      terminate_all_threads();
   delete ip_transactionPool;
   delete ip_threadPool;
   delete ip_mutex;

   TMTrace (2, ("TM_Info::~TM_Info EXIT\n"));
   //12/8/2010 Removed exit here as exit() is calling this destructor
   //exit(0);
}

// ------------------------------------------------------------------
// initialize
// Purpose : initialize the global TM structure.  Includes things like
//           setting the state, starting adp, etc...
// -------------------------------------------------------------------
void TM_Info::initialize()
{
    char  la_tm_name[8];
    char  la_value[9];
    int32 lv_error = 0;
    int32 lv_trace_detail = 0;
    bool  lv_unique = false;
    bool  lv_success = false;

    //initialize pool limits
    int32 lv_max_num_trans = 0;
    int32 lv_ss_low_trans = 0;
    int32 lv_ss_high_trans = 0;
    int32 lv_max_num_threads = 0;
    int32 lv_ss_low_threads = 0;
    int32 lv_ss_high_threads = 0;

    // TM Stats
    bool lv_tm_stats = ((TM_STATS==0)?false:true);
    int32 lv_tm_stats_interval = TM_STATS_INTERVAL;

    sprintf(la_tm_name, "$tm%d", iv_nid);

    lock();
    // intialize the tm info
    iv_state   = TM_STATE_DOWN;

    // initialize system recovery state and recovering
    // lead TM nid.
    iv_sys_recov_state = TM_SYS_RECOV_STATE_INIT;
    iv_sys_recov_lead_tm_nid = -1;
    iv_shutdown_level = MS_Mon_ShutdownLevel_Undefined;
    
    // start up the adp
   // iv_mat.initialize_adp();
    iv_lead_tm = false;
    iv_lead_tm_takeover = false;

    // get stall directive from registry
    lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                  (char *) CLUSTER_GROUP, (char *) DTM_STALL_PHASE_2, la_value);
           
    if (lv_error == 0)
    {
       iv_stall_phase_2 = atoi (la_value);
       if (iv_stall_phase_2 < 0)
          iv_stall_phase_2 = 0; // Turn it off
    }
    else
        iv_stall_phase_2 = 0;   // Turn it off

    // get interval from registry
   lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                  (char *) CLUSTER_GROUP, (char *) DTM_RM_WAIT_TIME, la_value);
           
    if (lv_error == 0)
    {
       iv_rm_wait_time = atoi (la_value);
       if (iv_rm_wait_time <= 0)
          iv_rm_wait_time = MAX_RM_WAIT_TIME;
    }
    else
        iv_rm_wait_time = MAX_RM_WAIT_TIME;

    // get trans hung retry from registry
   lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                  (char *) CLUSTER_GROUP, (char *) DTM_TRANS_HUNG_RETRY_INTERVAL, la_value);
           
    if (lv_error == 0)
    {
       iv_trans_hung_retry_interval = atoi (la_value);
       if (iv_trans_hung_retry_interval == 0)
          iv_trans_hung_retry_interval = TRANS_HUNG_RETRY_INTERVAL;
    }
    else
        iv_trans_hung_retry_interval = TRANS_HUNG_RETRY_INTERVAL;


    // get default timer thread wait time from registry (msec)
   lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                  (char *) CLUSTER_GROUP, (char *) DTM_TIMERTHREAD_WAIT, la_value);
           
    if (lv_error == 0)
    {
       iv_timerDefaultWaitTime = atoi (la_value);
       if (iv_timerDefaultWaitTime == 0)
          iv_timerDefaultWaitTime = TIMERTHREAD_WAIT;
    }
    else
        iv_timerDefaultWaitTime = TIMERTHREAD_WAIT;


    lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                  (char *) CLUSTER_GROUP, (char *) DTM_CP_INTERVAL, la_value);
           
    if (lv_error == 0)
    {
       iv_cp_interval = atoi (la_value);
       if (iv_cp_interval > 0)
          iv_cp_interval *= 60000; // 60 (mins to secs) * 1000 (secs to msecs)
       else
          iv_cp_interval = TM_CP_DEFAULT * 60000;
    }
    else
    {
       iv_cp_interval = TM_CP_DEFAULT * 60000;
    }

    lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                  (char *) CLUSTER_GROUP, (char *) DTM_STATS_INTERVAL, la_value);
           
    if (lv_error == 0)
    {
       iv_stats_interval = atoi (la_value);
       if (iv_stats_interval > 0)
          iv_stats_interval *= 60000; // 60 (mins to secs) * 1000 (secs to msecs)
       else
          iv_stats_interval = TM_STATS_DEFAULT * 60000;
    }
    else
    {
       iv_stats_interval = TM_STATS_DEFAULT * 60000;
    }

    lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                  (char *) CLUSTER_GROUP, (char *) DTM_TM_RMRETRY_INTERVAL, la_value);
           
    if (lv_error == 0)
    {
       iv_RMRetry_interval = atoi (la_value);
       if (iv_RMRetry_interval > 0)
          iv_RMRetry_interval *= 60000; // 60 (mins to secs) * 1000 (secs to msecs)
       else
          iv_RMRetry_interval = TM_RMRETRY_DEFAULT * 60000;
    }
    else
    {
       iv_RMRetry_interval = TM_RMRETRY_DEFAULT * 60000;
    }

    lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                  (char *) CLUSTER_GROUP, (char *) DTM_TM_TMRESTARTRETRY_INTERVAL, la_value);
           
    if (lv_error == 0)
    {
       iv_TMRestartRetry_interval = atoi (la_value);
       if (iv_TMRestartRetry_interval > 0)
          iv_TMRestartRetry_interval *= 1000; // * 1000 (secs to msecs)
       else
          iv_TMRestartRetry_interval = TM_TMRESTARTRETRY_DEFAULT * 1000; //secs
    }
    else
    {
       iv_TMRestartRetry_interval = TM_TMRESTARTRETRY_DEFAULT * 1000; //secs
    }

    lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                  (char *) CLUSTER_GROUP, (char *) DTM_TX_ABORT_TIMEOUT, la_value);
           
    if (lv_error == 0)
    {
       iv_timeout = atoi (la_value);
       if (iv_timeout != -1 && iv_timeout <= 0)
          iv_timeout = TX_ABORT_TIMEOUT; // Reset to default
    }
    else
    {
       iv_timeout = TX_ABORT_TIMEOUT; // Default
    }

    //initialize trace file
    iv_trace_level = 0;
    lv_unique = false;
    ms_getenv_int("TM_TRACE", &iv_trace_level);
    if (!iv_trace_level)
    {
        lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                 (char *) CLUSTER_GROUP, (char *) DTM_TM_TRACE, la_value);
        if (lv_error == 0)
        {
            iv_trace_level = atoi (la_value);
            init_tracing (true, (char *) "tm_trace", iv_trace_level);
        }
    }
    else //if (iv_trace_level)
    {
       ms_getenv_int("TM_TRACE_DETAIL", &lv_trace_detail);
       if (lv_trace_detail)
          iv_trace_level = lv_trace_detail;

       ms_getenv_bool ("TM_TRACE_UNIQUE", &lv_unique);

       const char *lp_file = ms_getenv_str("TM_TRACE_FILE");
       if (lp_file != NULL)
       {
          char *lp_trace_file = (char*)lp_file;
          init_tracing (lv_unique, lp_trace_file, iv_trace_level);
       }
       else
          init_tracing (lv_unique, (char *) "tm_trace", iv_trace_level);
 
      TMTrace (1, ("TM Tracing is on, trace level %d.\n", iv_trace_level));
    }

    // Get DTM_TM_STATS
    lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                          (char *) CLUSTER_GROUP, (char *) DTM_TM_STATS, 
                          la_value);
    if (lv_error == 0)
      lv_tm_stats = ((atoi(la_value) == 0)?false:true);

    lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                          (char *) CLUSTER_GROUP, (char *) DTM_TM_STATS_INTERVAL, 
                          la_value);
    if (lv_error == 0)
      lv_tm_stats_interval = atoi(la_value);

    gv_tm_info.stats()->initialize(lv_tm_stats, lv_tm_stats_interval);

    // Check for global unique sequence number generation
     lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                     (char *) CLUSTER_GROUP, (char *) DTM_GLOBAL_UNIQUE_SEQ_NUM, la_value);
           
    if (lv_error == 0)
    {
       int lv_globalUniqueSeqNum = atoi (la_value);
       if (lv_globalUniqueSeqNum == 0)
         iv_globalUniqueSeqNum = false;
    }
    if (iv_trace_level)
    {
       if (iv_globalUniqueSeqNum)
         trace_printf("Monitor to be used for sequence number generation.\n");
       else
         trace_printf("TM will generate local sequence numbers.\n");
     }
#ifdef MULTITHREADED_TM
    lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                  (char *) CLUSTER_GROUP, (char *) DTM_THREAD_MODEL, la_value);
           
    if (lv_error == 0)
    {
       int lv_threadModel = atoi (la_value);
      if (lv_threadModel == 1)
         iv_threadModel = worker;
      else 
         iv_threadModel = transaction;
    }
    else // Use default
    {
      if (THREAD_MODEL == 1)
         iv_threadModel = worker;
      else
         iv_threadModel = transaction;
    }
    if (iv_trace_level)
    {
       if (iv_threadModel == worker)
         trace_printf("Multithreading enabled for TM using worker threads.\n");
       else
         trace_printf("Multithreading enabled for TM using transaction threads.\n");
    }
#else
    iv_threadModel = none;
    TMTrace (1, ("Multithreading disabled for TM.\n"));
#endif


    // Configure transactionPool
    lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                          (char *) CLUSTER_GROUP, (char *) DTM_MAX_NUM_TRANS, 
                          la_value);
    lv_max_num_trans = ((lv_error == 0)?atoi(la_value):-1);
    lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                          (char *) CLUSTER_GROUP, (char *) DTM_STEADYSTATE_LOW_TRANS, 
                          la_value);
    lv_ss_low_trans = ((lv_error == 0)?atoi(la_value):-1);
    lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                          (char *) CLUSTER_GROUP, (char *) DTM_STEADYSTATE_HIGH_TRANS, 
                          la_value);
    lv_ss_high_trans = ((lv_error == 0)?atoi(la_value):-1);
    lv_success = ip_transactionPool->setConfig(tm_stats(), lv_max_num_trans, 
                                          lv_ss_low_trans, lv_ss_high_trans);
    if (lv_success)
    {
      TMTrace (1, ("Transaction pool parameters set: "
                     "Max %d, steady state low %d, steady state high %d.\n",
                     lv_max_num_trans, lv_ss_low_trans, lv_ss_high_trans));
    }
    else
    {
      TMTrace (1, ("Attempt to set tranasction pool parameters failed: "
                     "Max %d, steady state low %d, steady state high %d.\n",
                     lv_max_num_trans, lv_ss_low_trans, lv_ss_high_trans));
    }

    // Configure threadPool
    ms_getenv_int ("DTM_MAX_NUM_THREADS", &lv_max_num_threads);
    if (lv_max_num_threads)
    {   
      TMTrace (1, ("Enabling DTM_MAX_NUM_THREADS from env variable\n"));
    }
    else 
    {
      lv_max_num_threads = MAX_NUM_THREADS;
    }
    ms_getenv_int ("DTM_STEADYSTATE_LOW_THREADS", &lv_ss_low_threads);
    if (lv_ss_low_threads)
    {
      TMTrace (1, ("Enabling DTM_STEADYSTATE_LOW_THREADS from env variable\n"));
    }
    else 
    {
      lv_ss_low_threads = STEADYSTATE_LOW_THREADS;
    }
    ms_getenv_int ("DTM_STEADYSTATE_HIGH_THREADS", &lv_ss_high_threads);
    if (lv_ss_high_threads)
    {
      TMTrace (1, ("Enabling DTM_STEADYSTATE_HIGH_THREADS from env variable\n"));
    }
    else 
    {
      lv_ss_high_threads = STEADYSTATE_HIGH_THREADS;
    }
    
    lv_success = ip_threadPool->setConfig(tm_stats(), lv_max_num_threads, 
                                          lv_ss_low_threads, lv_ss_high_threads);
    if (lv_success)
    {
      TMTrace (1, ("Thread pool parameters set: "
                     "Max %d, steady state low %d, steady state high %d.\n",
                     lv_max_num_threads, lv_ss_low_threads, lv_ss_high_threads));
    }
    else
    {
      TMTrace (1, ("Attempt to set thread pool parameters failed: "
                     "Max %d, steady state low %d, steady state high %d.\n",
                     lv_max_num_threads, lv_ss_low_threads, lv_ss_high_threads));
    }

    // get incarnation num
    char la_incarnation[9];
    lv_error = tm_reg_get(MS_Mon_ConfigType_Process,
                  (char *) la_tm_name, (char *) DTM_INCARNATION_NUM, la_incarnation);
    if (lv_error) // NOT FOUND
       iv_incarnation_num = 0;
    else
       iv_incarnation_num = (short) (atoi (la_incarnation) + 1);

    TMTrace (1, ("Current incarnation for %s is %d: with error : %d \n",
                    la_tm_name, iv_incarnation_num, lv_error));

    sprintf(la_incarnation, "%d", iv_incarnation_num);
    lv_error = tm_reg_set(MS_Mon_ConfigType_Process,
                  la_tm_name, (char *) DTM_INCARNATION_NUM, la_incarnation);

    if (lv_error)
    {
        tm_log_event (DTM_TM_REGISTRY_SET_ERROR, SQ_LOG_CRIT, "DTM_TM_REGISTRY_SET_ERROR", lv_error);
        TMTrace (1, ("Failed to write value into the registry.  Error %d\n",lv_error));
        abort (); 
    }

    // get running mode
    lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                          (char *) CLUSTER_GROUP, (char *) DTM_RUN_MODE, la_value);

    if (lv_error == 0)
    {
        int lv_run_mode =  atoi(la_value);
        switch (lv_run_mode)
        {
            case TM_SYNC_MODE:
            {
                TMTrace (1, ("Setting RUNTIME mode to SYNC_MODE\n"));
                 iv_run_mode = TM_SYNC_MODE;
                break;
            }
            case TM_NONSYNC_MODE:
            default:
            {
                 TMTrace (1, ("Setting RUNTIME mode to TM_NONSYNC_MODE\n"));
                 iv_run_mode = TM_NONSYNC_MODE;
                break;
            }
        }
    }
    else
    {
        TMTrace (1, ("Setting RUNTIME mode to TM_NONSYNC_MODE\n"));
        iv_run_mode = TM_NONSYNC_MODE;
    }
 
    int lv_use_tlog;
    ms_getenv_int ("TM_ENABLE_TLOG_WRITES", &lv_use_tlog);

    if (lv_use_tlog)
        TMTrace (1, ("Enabling TLOG use from env variable\n"));
    use_tlog( lv_use_tlog != 0 );

    int lv_audit_mode;
    ms_getenv_int ("DTM_AUDIT_MODE", &lv_audit_mode);

    if (lv_audit_mode)
        TMTrace (1, ("Setting AUDIT mode from env variable\n"));
            
    if (!lv_audit_mode)
    {   
        // get audit mode DTM_AUDIT_MODE
        lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                              (char *) CLUSTER_GROUP, (char *) DTM_AUDIT_MODE, la_value);

        if (lv_error == 0)
            lv_audit_mode =  atoi(la_value);
     }

     switch (lv_audit_mode)
     {
         case TM_NORMAL_AUDIT_MODE:
         {
             TMTrace (1, ("Setting AUDIT mode to BUFFERING OFF\n"));
             iv_audit_mode = lv_audit_mode;
             break;
         }
         case TM_BUFFER_AUDIT_MODE:
         {
             TMTrace (1, ("Setting AUDIT mode to BUFFERING ON\n"));
             iv_audit_mode = lv_audit_mode;
             break;
         }
         default:
         {
             TMTrace (1, ("Setting AUDIT mode to BUFFERING OFF as default\n"));
             iv_audit_mode = TM_NORMAL_AUDIT_MODE;
             break;
         }
     }
       
  // get perf stats mode DTM_PERF_STATS
    iv_perf_stats = TM_PERF_STATS_OFF;

    TMTrace (1, ("Setting PERF STATS mode to %d\n",iv_perf_stats ));

    // Get the next sequence number block and set the next block range in
    // the registry. SeqNumInterval determines the block size.
    iv_nextSeqNum = setNextSeqNumBlock();

    iv_TSE_xa_start = TSE_XA_START_DEFAULT;
    ms_getenv_bool ("TM_TSE_XA_START", &iv_TSE_xa_start);
    if (iv_trace_level)
    {
       if (iv_TSE_xa_start)
         trace_printf("TM_TSE_XA_START on, but no longer used - remove!\n");
       else
         trace_printf("TM_TSE_XA_START off, but no longer used - remove!\n");
    }


    // RM Participation
    // We can't distingush between the DTM_RM_PARTIC environment
    // variable =0 or missing.  So, you must omit both the environment
    // variable and registry value or include only the registry value
    // (omitting the environment variable) for the defaulting to work.
    // A non-zero environment variable takes precidence, followed by the 
    // registry value, then the default.
    bool lv_RMPartic;
    ms_getenv_bool ("DTM_RM_PARTIC", &lv_RMPartic);
    if (lv_RMPartic)
        iv_RMPartic = lv_RMPartic;
    else
    {   
       // Check for registry value
       lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                          (char *) CLUSTER_GROUP, (char *) DTM_RM_PARTIC, la_value);

       if (lv_error == 0)
       {
          lv_RMPartic =  atoi(la_value);
          switch (lv_RMPartic)
          {
          case PARTIC_NONE:
             iv_RMPartic = false;
             break;
          case PARTIC_ALL_RMS:
             iv_RMPartic = true;
             break;
          default:
             iv_RMPartic = (bool) DEFAULT_RM_PARTIC;
          }
       }
       else
          iv_RMPartic = lv_RMPartic;
    }
    if (iv_trace_level)
    {
       if (iv_RMPartic)
         trace_printf("TM will use xa_start to register transactions with all TSEs.\n");
       else
         trace_printf("RMs will use ax_reg to register transaction participation.\n");
    }

    // Timestamp mode
    TS_MODE lv_TSMode;
    ms_getenv_bool ("DTM_TM_TS_MODE", (bool *) &iv_TSMode);
    if (!iv_TSMode)
    {   
       // Check for registry value
       lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                          (char *) CLUSTER_GROUP, (char *) DTM_TM_TS_MODE, la_value);

       if (lv_error == 0)
       {
          lv_TSMode =  (TS_MODE) atoi(la_value);
          if (lv_TSMode)
             iv_TSMode = lv_TSMode;
          else
             iv_TSMode = DEFAULT_TS_MODE;
       }
       else
          iv_TSMode = DEFAULT_TS_MODE;
    }
    if (iv_perf_stats == TM_PERF_STATS_ON && iv_TSMode < TS_FAST)
    {
       iv_TSMode = TS_DETAIL;
       if (iv_trace_level)
          trace_printf("Warning Timestamp mode changed to %d because TM_PERF_STATS is on.\n", iv_TSMode);
    }
       
    if (iv_trace_level)
       trace_printf("Timestamp mode %d.\n", iv_TSMode);

    iv_overrideAuditInconsistencyDuringRecovery = OVERRIDE_AUDIT_INCONSISTENCY;
    ms_getenv_bool ("DTM_OVERRIDE_AUDIT_INCONSISTENTCY", &iv_overrideAuditInconsistencyDuringRecovery);
    if (iv_overrideAuditInconsistencyDuringRecovery)
    {
       tm_log_event (DTM_OVERRIDE_AUDIT_INCONSISTENCY_WARN, SQ_LOG_INFO,"DTM_OVERRIDE_AUDIT_INCONSISTENCY_WARN"); 
       if (iv_trace_level)
          trace_printf("DTM_OVERRIDE_AUDIT_INCONSISTENCY set.  Only the first trans state record will be used on scan back.\n");
    }

    TM_BROADCAST_ROLLBACKS lv_broadcast_rollbacks = TM_DEFAULT_BROADCAST_ROLLBACKS;
    ms_getenv_int (DTM_BROADCAST_ROLLBACKS, (int *) &lv_broadcast_rollbacks);

    if (lv_broadcast_rollbacks == 0)
    {
       // Check for registry value
       lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                             (char *) CLUSTER_GROUP, (char *) DTM_BROADCAST_ROLLBACKS, la_value);

       if (lv_error == 0)
       {
          lv_broadcast_rollbacks = (TM_BROADCAST_ROLLBACKS) atoi(la_value);
          switch (lv_broadcast_rollbacks)
          {
          case TM_BROADCAST_ROLLBACKS_NO:
          case TM_BROADCAST_ROLLBACKS_YES:
          case TM_BROADCAST_ROLLBACKS_DEBUG:
             broadcast_rollbacks(lv_broadcast_rollbacks);
             break;
          default:
             broadcast_rollbacks(TM_DEFAULT_BROADCAST_ROLLBACKS);
          }
       }
       else
          broadcast_rollbacks(lv_broadcast_rollbacks);
    }
    else
       broadcast_rollbacks(lv_broadcast_rollbacks);

    tm_log_event (DTM_BROADCAST_ROLLBACKS_INFO, SQ_LOG_INFO,"DTM_BROADCAST_ROLLBACKS_INFO",\
                  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,iv_broadcast_rollbacks); 
    if (iv_trace_level)
    {
       if (iv_trace_level)
          trace_printf("DTM_BROADCAST_ROLLBACKS set to %d.  All aborts will cause xa_rollback to be broadcast "
                       "to all TSEs regardless of branch participation.\n", iv_broadcast_rollbacks);
    }

    TM_SYSRECOVERY_MODE lv_sysrecovery_mode = iv_SysRecovMode;
    ms_getenv_int("DTM_TM_SYSRECOVERY_MODE", (int *) &lv_sysrecovery_mode);

    switch (lv_sysrecovery_mode)
    {
    case CLEAN_SHUTDOWN_OPTIMIZE:
    case ALWAYS_SEND_XA_RECOVER:
        TMTrace(1, ("Setting System Recovery mode to %d\n", lv_sysrecovery_mode));
        iv_SysRecovMode = lv_sysrecovery_mode;
        break;
    default:
        TMTrace(1, ("** Invalid System Recovery mode %d, default of %d used.\n", 
                lv_sysrecovery_mode, iv_SysRecovMode));
    }

    // Setting a TLOG per TM allows greater scale than the standard one TLOG/environment.
    // We can't distingush between the DTM_TLOG_PER_TM environment
    // variable =0 or missing.  So, you must omit both the environment
    // variable and registry value or include only the registry value
    // (omitting the environment variable) for the defaulting to work.
    // A non-zero environment variable takes precidence, followed by the 
    // registry value, then the default.
    bool lv_TLOGperTM;
    ms_getenv_bool ("DTM_TLOG_PER_TM", &lv_TLOGperTM);
    if (lv_TLOGperTM)
        iv_TLOGperTM = lv_TLOGperTM;
    else
    {   
       // Check for registry value
       lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                          (char *) CLUSTER_GROUP, (char *) DTM_TLOG_PER_TM, la_value);

       if (lv_error == 0)
       {
          lv_TLOGperTM =  atoi(la_value);
          switch (lv_TLOGperTM)
          {
          case false:
             iv_TLOGperTM = false;
             break;
          case true:
             iv_TLOGperTM = true;
             break;
          default:
             iv_TLOGperTM = (bool) DEFAULT_TLOG_PER_TM;
          }
       }
       else
          iv_TLOGperTM = lv_TLOGperTM;
    }

    if (iv_trace_level)
    {
       if (iv_TLOGperTM)
         trace_printf("TM will configure one TLOG per TM process for audit.\n");
       else
         trace_printf("TM will use a single TLOG for all audit.\n");
    }

    int lv_maxRecoveringTxns = 0;
    ms_getenv_int ("DTM_MAXRECOVERINGTXNS", &lv_maxRecoveringTxns);

    if (lv_maxRecoveringTxns)
    {
        TMTrace (1, ("Setting maximum recovering transactions from env variable "
                 "DTM_MAXRECOVERINGTXNS to %d, default was %d.\n",
                 lv_maxRecoveringTxns, iv_maxRecoveringTxns));
        iv_maxRecoveringTxns = lv_maxRecoveringTxns;
    }
 
    int lv_rm_partic_recov;
    ms_getenv_int ("DTM_RM_PARTIC_RECOV", &lv_rm_partic_recov);

    if (lv_rm_partic_recov)
        TMTrace (1, ("Setting RM_PARTIC_RECOV from env variable\n"));
            
    if (!lv_rm_partic_recov)
    {   
        // get rm_partic_recov DTM_RM_PARTIC_RECOV registry
        lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                              (char *) CLUSTER_GROUP, (char *) DTM_RM_PARTIC_RECOV, la_value);

        if (lv_error == 0)
            lv_rm_partic_recov =  atoi(la_value);
     }

     switch (lv_rm_partic_recov)
     {
         case TM_PARTIC_RM_PARTIC_RECOV:
         {
             TMTrace (1, ("Setting ALL_RM_PARTIC_RECOV to false\n"));
             iv_AllRmParticRecov = false;
             break;
         }
         case TM_ALL_RM_PARTIC_RECOV:
         {
             TMTrace (1, ("Setting ALL_RM_PARTIC_RECOV to true\n"));
             iv_AllRmParticRecov = true;
             break;
         }
         default:
         {
             TMTrace (1, ("Setting RM_PARTIC_RECOV to default\n"));
             iv_AllRmParticRecov = DEFAULT_ALL_RM_PARTIC_RECOV;
             break;
         }
     }

     // get SeaTrans multithread
     bool lv_seatransSingleThreaded = false;
     ms_getenv_bool("TM_SEATRANS_SINGLETHREAD", &lv_seatransSingleThreaded);

     if (lv_seatransSingleThreaded == false)
     {
         cout << "SeaTrans multithread mode set." << endl;
         multithreaded(true);
     }
     else
     {
         cout << "SeaTrans singlethread mode set." << endl;
         multithreaded(false);
     }

     TMTrace (1, ("Setting SeaTrans Singlethread mode to %d.\n", !iv_multithreaded));


    // Test pause state is used for testing only and requires the define 
    // debug_mode=1 to be set
#ifdef debug_mode
    int lv_test_pause_state = -1;
    ms_getenv_int ("DTM_TEST_PAUSE_STATE", &lv_test_pause_state);

    if (lv_test_pause_state != -1)
        TMTrace (1, ("Setting TEST_PAUSE_STATE to %d from env variable\n", lv_test_pause_state));

    lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                          (char *) CLUSTER_GROUP, (char *) DTM_TEST_PAUSE_STATE, la_value);

    if (lv_error == 0)
       lv_test_pause_state = atoi(la_value);

     switch (lv_test_pause_state)
     {
         case -2:
         {
             TMTrace (1, ("Setting TEST_PAUSE_STATE to RANDOM.\n"));
             gv_test_pause_type = TX_PAUSE_STATE_TYPE_RANDOM;
             gv_test_pause = -1;
             break;
         }
         case -1:
         {
             TMTrace (1, ("Setting TEST_PAUSE_STATE to off.\n"));
             gv_test_pause_type = TX_PAUSE_STATE_TYPE_OFF;
             gv_test_pause = -1;
             break;
         }
         default:
         {
             TMTrace (1, ("Setting TEST_PAUSE_STATE to FIXED %d.\n", lv_test_pause_state));
             gv_test_pause_type = TX_PAUSE_STATE_TYPE_FIXED;
             gv_test_pause = lv_test_pause_state;
             break;
         }
     }
#endif //deug_mode       

    unlock();
}

void  TM_Info::set_xa_trace (char *pp_string)
{
    char * lp_trace_string_end;
    unsigned long lv_trace_mask;
    lp_trace_string_end = pp_string + strlen (pp_string);
    lv_trace_mask = strtoul (pp_string, &lp_trace_string_end, 16);
    xaTM_setTrace(lv_trace_mask);
}

void  TM_Info::set_trace (int32 pv_detail)
{
     iv_trace_level = pv_detail;
     if (!iv_trace_initialized)
         init_tracing (true, (char *) "tm_trace", iv_trace_level); // use defaults
}

void TM_Info::init_tracing(bool pv_unique, const char *pp_trace_file, int32 pv_detail)
{
       iv_trace_initialized = true;
       if (pp_trace_file != NULL)
       {
          char *lp_trace_file = (char*)pp_trace_file;
          trace_init(lp_trace_file, pv_unique, NULL, false);
       }
       else
          trace_init ((char *) "tm_trace", pv_unique, NULL, false);

      iv_trace_level = pv_detail;
      gv_tm_trace_level = iv_trace_level;
      TMTrace (1, ("TM Tracing is on, trace level %d.\n", iv_trace_level));
}

void TM_Info::send_system_status(TM_STATUSSYS *pp_system_status)
{
    short                      la_results[6];
    Tm_Sys_Status_Req_Type     *lp_req = NULL;
    Tm_Sys_Status_Rsp_Type     *lp_rsp = NULL;
    int32                      lv_error = FEOK;
    int32                      lv_index = 0;
    int32                      lv_num_sent = 0;
    pid_msgid_struct           lv_pid_msgid[MAX_NODES];
    int32                      lv_reqLen = 0;
    long                       lv_ret;
    long                       lv_ret2;
    int32                      lv_rspLen = 0;
    int                        lv_rsp_rcvd = 0;
    BMS_SRE_LDONE              lv_sre;

    int32 lv_up = 0;
    int32 lv_down = 0;
    int32 lv_recovering = 0;
    int32 lv_totaltms = 0;
    int32 lv_activetxns = 0;
    int32 lv_leadtm = 0;

    TMTrace (2, ("TM_Info::send_system_status: ENTRY\n"));

    //initialize lv_pid_msgid
    for (int32 i = 0; i <= tms_highest_index_used(); i++)
    {
        lv_pid_msgid[i].iv_tag = 0;
        lv_pid_msgid[i].iv_msgid = 0;
        lv_pid_msgid[i].iv_nid = 0;
    }

    TMTrace (3, ("TM_Info::send_system_status Sending Stats request to other TMs.\n"));

     lp_req = new Tm_Sys_Status_Req_Type [tms_highest_index_used() + 1];  
     lp_rsp = new Tm_Sys_Status_Rsp_Type [tms_highest_index_used() + 1];  

     for (int lv_idx = 0; lv_idx <= tms_highest_index_used(); lv_idx++)
     {
         
         //gather even if not in_use
         if (lv_idx == iv_nid)
         {
            lv_pid_msgid[lv_idx].iv_tag = -1;
         }
         else 
         {
             lv_pid_msgid[lv_idx].iv_tag = lv_idx + 1; // non zero
             lp_req[lv_idx].iv_msg_hdr.rr_type.request_type = TM_MSG_TYPE_STATUSSYSTEM;
             lp_req[lv_idx].iv_msg_hdr.version.request_version = TM_SQ_MSG_VERSION_CURRENT;
             lv_pid_msgid[lv_idx].iv_nid = lv_idx;
         
             lv_reqLen = sizeof (Tm_Sys_Status_Req_Type);
             lv_rspLen = sizeof (Tm_Sys_Status_Rsp_Type);

             lv_error = link(&(iv_open_tms[lv_idx].iv_phandle),     // phandle,
                             &lv_pid_msgid[lv_idx].iv_msgid,        // msgid
                             (char *) &lp_req[lv_idx],    // reqdata
                             lv_reqLen,                   // reqdatasize
                             (char *) &lp_rsp[lv_idx],    // replydata
                             lv_rspLen,                   // replydatamax
                             lv_pid_msgid[lv_idx].iv_tag, // linkertag
                             TM_TM_LINK_PRIORITY,         // pri
                             BMSG_LINK_LDONEQ,            // linkopts
                             TM_LINKRETRY_RETRIES);       // retry count

             if (lv_error != 0)
             {
                 TMTrace (1, ("TM_Info::send_system_status BMSG_LINK_ failed with error %d. failure ignored.\n",lv_error));
             }
             else
                 lv_num_sent++;
         }
      } // for each tm

      // LDONE LOOP
      while (lv_rsp_rcvd < lv_num_sent)
      {

        // wait for an LDONE wakeup 
        XWAIT(LDONE, -1);
 
        do {
             lv_error = 0;

             // we've reached our message reply count, break
             if (lv_rsp_rcvd >= lv_num_sent)
                 break;

             lv_ret = BMSG_LISTEN_((short *)&lv_sre, 
                                    BLISTEN_ALLOW_LDONEM, 0);

             if (lv_ret == BSRETYPE_LDONE)
             {
                lv_index = -1;
                for (int32 lv_idx2 = 0; lv_idx2 <=tms_highest_index_used(); lv_idx2++)
                {
                    if (lv_pid_msgid[lv_idx2].iv_tag == lv_sre.sre_linkTag)
                    {
                       lv_index = lv_idx2;
                       break;
                     }
                }
                if (lv_index == -1)
                {
                    TMTrace (1, ("TM_Info::send_system_status - Link Tag %d not found\n", (int)lv_sre.sre_linkTag));
                    lv_error = FEDEVDOWN;
                }

                if (!lv_error)
                {
                    lv_ret2 = BMSG_BREAK_(lv_pid_msgid[lv_index].iv_msgid, 
                                         la_results,
                                         &(iv_open_tms[lv_pid_msgid[lv_index].iv_nid].iv_phandle)); 
                    if (lv_ret2 != 0)
                    {
                        TMTrace (1, ("TM_Info::send_system_status ERROR BMSG_BREAK_ returned %ld, index %d, msgid %d.\n",
                                lv_ret2, lv_index, lv_pid_msgid[lv_index].iv_msgid));
                        lv_error = FEDEVDOWN;
                    }
                }
          
                if (lv_error == FEDEVDOWN)
                {
                   lv_down += 1;
                   lv_totaltms +=1;
                   TMTrace (1, ("TM_Info::send_system_status - TM respond error\n"));
                }
                else
                {
                    lv_up += lp_rsp[lv_index].iv_status_system.iv_up;
                    lv_down += lp_rsp[lv_index].iv_status_system.iv_down;
                    lv_recovering += lp_rsp[lv_index].iv_status_system.iv_recovering;
                    lv_totaltms += lp_rsp[lv_index].iv_status_system.iv_totaltms;
                    lv_activetxns += lp_rsp[lv_index].iv_status_system.iv_activetxns;
                    
                    if (lp_rsp[lv_index].iv_status_system.iv_leadtm == 1) {
                        lv_leadtm = lv_index;
                    }
                }
               lv_rsp_rcvd++;
               }
          } while (lv_ret == BSRETYPE_LDONE); 
       }// while (lv_rsp_rcvd < lv_num_sent)

    delete []lp_rsp;
    delete []lp_req;

    if(state() == TM_STATE_UP) {
        lv_up += 1;
    }
    else if (state() == TM_STATE_DOWN) {
        lv_down += 1;
    }
    if(sys_recov_state() != TM_SYS_RECOV_STATE_END) {
        lv_recovering += 1;
    }
    lv_totaltms += 1;
    lv_activetxns += num_active_txs();

    // If we're still in recovery we need to add any transactions 
    // still queued to recover.
    if (ip_ClusterRecov)
       lv_activetxns += ip_ClusterRecov->txnStateList()->size();

    if(lead_tm()) {
        lv_leadtm = nid();
    }

    pp_system_status->iv_up = lv_up;
    pp_system_status->iv_down = lv_down;
    pp_system_status->iv_recovering = lv_recovering;
    pp_system_status->iv_totaltms = lv_totaltms;
    pp_system_status->iv_activetxns = lv_activetxns;
    pp_system_status->iv_leadtm = lv_leadtm;

    TMTrace (2, ("TM_Info::send_system_status: EXIT\n"));
}

// ------------------------------------------------------------
// lock
// Purpose : locks access to the structure to enforce 
//           serialization.
// Now using recursive semaphores
// ------------------------------------------------------------
void TM_Info::lock()
{
   TMTrace(4, ("TM_Info::lock, count %d, owner %ld\n", 
           ip_mutex->lock_count(), ip_mutex->lock_owner()));
   int lv_error = ip_mutex->lock();
   if (lv_error)
   {
      TMTrace(1, ("TM_Info::lock returned error %d.\n", lv_error));
      abort();
   }
}

// ------------------------------------------------------------
// unlock
// Purpose : unlocks access to the structure to enforce 
//           serialization.
// ------------------------------------------------------------
void TM_Info::unlock()
{
   iv_lock_count--;
   if (iv_lock_count == 0)
      iv_lock_owner= -1;

   TMTrace(4, ("TM_Info::unlock, count %d, owner %ld\n", 
           ip_mutex->lock_count(), ip_mutex->lock_owner()));
   int lv_error = ip_mutex->unlock();
   if (lv_error)
   {
         TMTrace(1, ("TM_Info::unlock returned error %d.\n", lv_error));
         abort();
   }
}


// ---------------------------------------------------------
// cleanup tx
// Purpose : Clean up transaction related info when a
// transaction exits.
// ---------------------------------------------------------
void TM_Info::cleanup(void *pp_txn)
{
   CTmTxBase *lp_txn = (CTmTxBase *) pp_txn;
   TMTrace (2, ("TM_Info::cleanup : ENTRY, Txn ID (%d,%d).\n", lp_txn->node(), lp_txn->seqnum()));

   //No longer need to check for additional queued requests here as 
   // nothing else could have been queued because we don't queue begintxn  
   // any more, just end and aborts.
   // Reply to any queued application requests
   //pp_txn->reply_to_queuedRequests(FEINVTRANSID);

   lock();
   if (lp_txn->in_use())
      remove_tx(lp_txn);
   unlock();

   TMTrace (2, ("TM_Info::cleanup : EXIT, Active Txns=%d.\n", num_active_txs()));
}


// --------------------------------------------------------------
// TM_Info::init_slot
// Purpose : Initialize a slot in the array with RM indicated
//           by pv_rmid
// --------------------------------------------------------------
int32 TM_Info::init_slot (MS_Mon_Process_Info_Type *pp_info, int32 pv_rmid, bool pv_is_ax_reg)
{
     int32 lv_err =  gv_RMs.TSE()->init(pp_info->pid, pp_info->nid, pp_info->process_name, 
                                     pv_rmid, pv_is_ax_reg, TSEBranch_UP);
     return lv_err;
} //TM_Info::init_slot


// --------------------------------------------------------------------
// TM_Info::schedule_init_and_recover_rms
// Purpose : this is called at startup to schedule the opening of RMs.
// It does this by creating a timerEvent and queuing it to the timer
// thread for immediate processing.
// ---------------------------------------------------------------------
void TM_Info::schedule_init_and_recover_rms()
{
   const int32 lc_delay = 0;
   TMTrace (2, ("TM_Info::schedule_init_and_recover_rms : ENTRY, delay time: %dusec\n", lc_delay));

   CTmTimerEvent *lp_timerEvent = 
       new CTmTimerEvent(TM_MSG_TXINTERNAL_INITIALIZE_RMS, lc_delay);

   tmTimer()->eventQ_push((CTmEvent *) lp_timerEvent);
   // We don't wait here - the starup will be driven forward by
   // confirmation that the TM_SYS_RECOV_START_SYNC sync has 
   // been sent to all TMs (tm_originating_sync_commit()).
} //TM_Info::schedule_init_and_recover_rms


//----------------------------------------------------------------------------
// TM_Info::schedule_recover_system
// Purpose : Schedules system recovery to execute under the timer thread.
// This frees up the main thread to handle any requests which arrive during
// the recovery window.
//----------------------------------------------------------------------------
void TM_Info::schedule_recover_system()
{
   const int32 lc_delay = 0;
   TMTrace (2, ("TM_Info::schedule_recover_system : ENTRY, delay time: %dusec\n", lc_delay));

   CTmTimerEvent *lp_timerEvent = 
       new CTmTimerEvent(TM_MSG_TXINTERNAL_SYSTEM_RECOVERY, lc_delay);

   tmTimer()->eventQ_push((CTmEvent *) lp_timerEvent);
} //TM_Info::schedule_recover_system


// --------------------------------------------------------------------
// TM_Info::init_and_recover_rms
// Purpose : this is called upon startup.  We call into the monitor
//           to get all the DP2s in the system.  Then we try to open
//           them.  This is done to drive recovery (if need be) as well
//           get a handle to them for future communication.
//
// ---------------------------------------------------------------------
void TM_Info::init_and_recover_rms()
{
#define MAX_RM_PROCS MAX_OPEN_RMS*2 //Allow for backups
    TM_RM_Responses          la_resp[MAX_RM_PROCS];
    //char                     la_value[9];
    int32                    lv_count;
    int32                    lv_error;
    int32                    lv_index = 0;
    MS_Mon_Process_Info_Type *lp_info;
    int32                    lv_msg_count = 0;
    RM_Open_struct           lv_open;
    RMID                     lv_rmid;
   
    int32                    la_rmid[MAX_RM_PROCS];
    int32                    lv_rmindex = 0; 

    TMTrace (2, ("TM_Info::init_and_recover_rms : ENTRY.\n"));

    if (!all_rms_closed()) {
       // RMs open, no need to re-open
       TMTrace (2, ("TM_Info::init_and_recover_rms - all RMs open: EXIT.\n"));
       return;
    }

    lp_info = new MS_Mon_Process_Info_Type[MAX_RM_PROCS];
    if (!lp_info)
    {
       TMTrace(1, ("TM_Info::init_and_recover_rms - Unable to allocate process array, aborting.\n"));
       tm_log_event(DTM_OUT_OF_MEMORY, SQ_LOG_CRIT, "DTM_OUT_OF_MEMORY",
           -1, -1, nid(), -1, -1, -1, MAX_RM_PROCS, -1, -1, -1, -1, -1, -1, -1, -1,
           -1, "TM_Info::init_and_recover_rms");
       abort();
    }

    // get all DP2s in the system and open them all
    lv_error = msg_mon_get_process_info_type(MS_ProcessType_TSE,
                                             &lv_count,
                                             MAX_RM_PROCS,
                                             lp_info);
    switch (lv_error)
    {
    case FEOK:
       break;
    case FEBOUNDSERR:
         TMTrace(1, ("TM_Info::init_and_recover_rms - Error 22 (FEBOUNDSERR) received from "
                 "msg_mon_get_process_info_type. DTM currently only supports %d TSEs.\n",
                 MAX_RM_PROCS));
         // Intentional drop-through.
    default:
         TMTrace(1, ("TM_Info::init_and_recover_rms - Error %d, aborting.\n", lv_error));
         tm_log_event(DTM_RM_OPEN_FAILED, SQ_LOG_CRIT, "DTM_RM_OPEN_FAILED",
                     lv_error /*error_code*/ );
          abort();
     }

    TMTrace( 3, ("TM_Info::init_and_recover_rms : received type for %d rms.\n", lv_count));
    for (lv_index = 0; ((lv_index < MAX_RM_PROCS) && (lv_index < lv_count)); lv_index++)
    {
      // we don't care about it if its a backup
      if (lp_info[lv_index].backup == 1)
      {
          la_rmid[lv_index] = -1;
          continue;
      }

      // Removed RMID registry value in M7.
      // M8_TODO: Remove these lines and implement internal rmid algorithm similar
      // to seq num with node and locally unique number.
      /*lv_error = tm_reg_get(MS_Mon_ConfigType_Process, 
                        lp_info[lv_index].process_name, (char *) "RMID", la_value);
      
      lv_rmid=(lv_error) ? 0 : (atoi(la_value));
       */   
      lv_rmid.iv_rmid = 0;
      lv_rmid.s.iv_nid = nid();
      lv_rmid.s.iv_num = lv_index;

      la_rmid[lv_index] = lv_rmid.iv_rmid; // coordinate lv_info index with rmid
      strcpy(lv_open.process_name, lp_info[lv_index].process_name);
      lv_open.incarnation_num = incarnation_num();
      lv_open.seq_num_block_start = SeqNumBlockStart();

      lv_error = (*tm_switch).xa_open_entry ((char *)&lv_open, lv_rmid.iv_rmid, TMNOFLAGS);
      if (lv_error == XA_OK)
      {
         lv_msg_count++;
      }
      else
      {
         // Handle RM error. 
         tm_log_event(DTM_RM_OPEN_FAILED, SQ_LOG_CRIT, "DTM_RM_OPEN_FAILED",
                    -1, /*error_code*/ 
                    lv_rmid.s.iv_num, /*rmid*/
                    -1, /*dtmid*/ 
                    -1, /*seq_num*/
                    -1, /*msgid*/
                    -1, /*xa_error*/
                    -1, /*pool_size*/
                    -1, /*pool_elems*/
                    -1, /*msg_retries*/
                    -1, /*pool_high*/
                    -1, /*pool_low*/
                    -1, /*pool_max*/
                    -1, /*tx_state*/
                    -1, /*data */
                    -1, /*data1*/
                    -1,/*data2 */
                     lp_info[lv_index].process_name);
         TMTrace(1, ("TM_Info::init_and_recover_rms - Failed to open RM %s, rmid %d\n",
                 lp_info[lv_index].process_name, lv_rmid.s.iv_num));
      }
    }
     
    //Since we ignore backup TSEs, there could be gaps in the la_rmid array.
    //So, we cannot just check lv_msg_count entries in the la_rmid array.
    //Instead, we should go through 'lv_index-1' entries.
    lv_rmindex = lv_index-1;
    
    int32 lv_repliesOutstanding = complete_all(lv_msg_count, la_resp, MAX_TMTIMER_WAIT_TIME);
    if (lv_repliesOutstanding > 0)
    {
         tm_log_event(DTM_RM_REPLY_FAILED, SQ_LOG_CRIT , "DTM_RM_REPLY_FAILED",
                    -1, /*error_code*/ 
                    -1, /*rmid*/
                    -1, /*dtmid*/ 
                    -1, /*seq_num*/
                    -1, /*msgid*/
                    -1, /*xa_error*/
                    -1, /*pool_size*/
                    -1, /*pool_elems*/
                    -1, /*msg_retries*/
                    -1, /*pool_high*/
                    -1, /*pool_low*/
                    -1, /*pool_max*/
                    -1, /*tx_state*/
                    lv_repliesOutstanding); /*data */

         TMTrace(1, ("TM_Info::init_and_recover_rms - %d RMs failed to reply to xa_open request.\n",
                 lv_repliesOutstanding));
         abort();
    }

    // if there are not errors, then initialize the rm slot
    for (int32 lv_idx = 0; lv_idx < lv_msg_count; lv_idx++)
    {
        if (!la_resp[lv_idx].iv_error)
        {
            // find right slot
            int lv_i = 0;
            for (; lv_i <= lv_rmindex; lv_i++)
                if ((la_rmid[lv_i] != -1) && 
                    (la_rmid[lv_i] == la_resp[lv_idx].iv_rmid))
                    break;

            if (lv_i > lv_rmindex) 
            {
                tm_log_event(DTM_RM_NO_MATCH, SQ_LOG_CRIT, "DTM_RM_NO_MATCH",-1,la_resp[lv_idx].iv_rmid);
                TMTrace(1, ("TM_Info::init_and_recover_rms - RM id %d did not match any of the RM slots.\n", 
                    la_resp[lv_idx].iv_rmid));
                abort ();
            }

            init_slot(&lp_info[lv_i], la_resp[lv_idx].iv_rmid, la_resp[lv_idx].iv_ax_reg);
        }
        else 
        {
            tm_log_event(DTM_RM_OPEN_FAILED2, SQ_LOG_CRIT,"DTM_RM_OPEN_FAILED2",
                          la_resp[lv_idx].iv_error, la_resp[lv_idx].iv_rmid);
            TMTrace(1, ("TM_Info::init_and_recover_rms - Failed to open RM rmid %d, error %d\n",
                    la_resp[lv_idx].iv_rmid, la_resp[lv_idx].iv_error));
        }
    }
    all_rms_closed(false);  // Finished openning RMs.  This flag is used in Shutdown. 

    // once recovery is done, write 2 control points for a clean slate
    if (lead_tm())
    {
        TMTrace(2, ("TM_Info::init_and_recover_rms : lead_dtm.\n"));
        // If this is the Lead TM, proceeds with system crash recovery.
        // Once recovery is done, the recover_system function will 
        // write 2 control points for a clean slate.  

        if (sys_recov_state() != TM_SYS_RECOV_STATE_END)
        {
           // send out recovery start sync.  The rest of system recover will be driven
           // from the completion
           ClusterRecov(new TM_Recov(gv_tm_info.rm_wait_time()));
#if 0
           lv_error = ClusterRecov()->initiate_start_sync();
#endif
           gv_tm_info.set_sys_recov_status(TM_SYS_RECOV_STATE_END,
                                           gv_tm_info.lead_tm_nid()); //my node sb the lead tm
           gv_tm_info.schedule_recover_system();      
        }
	tm_up();
    }
    else
    {
       // Mark the TM up now if it's pending.
       TMTrace(2, ("TM_Info::init_and_recover_rms : not the lead_dtm.\n"));
       gv_tm_info.set_sys_recov_status(TM_SYS_RECOV_STATE_END,lead_tm_nid());
       tm_up();
       if (restarting_tm() == nid())
       {
          TMTrace(2, ("TM_Info::init_and_recover_rms : Calling msg_mon_tm_ready.\n"));
          msg_mon_tm_ready();
          restarting_tm(-1);
       }
    }

    delete[] lp_info;
   
    TMTrace(2, ("TM_Info::init_and_recover_rms : EXIT.\n"));
} //TM_Info::init_and_recover_rms


// -------------------------------------------------------------------
// restart_tm _process
// Purpose : This method will be called by the lead TM when a node Up
//           message is received by the monitor. It will restart the
//           TM process immediately and send a sync to update the 
//           restarted process.
//
// Update: This method will NOT be called by the lead DTM as
//         the 'pstartd' (process startup daemon)will start up 
//         the TM on it's node.
//
//         Keeping this method for the time being.
// -------------------------------------------------------------------
int32 TM_Info::restart_tm_process(int32 pv_nid) 
{
    char la_buf_tm_name[20];
    char la_prog[MS_MON_MAX_PROCESS_PATH];
    char la_out_file[MS_MON_MAX_PROCESS_PATH];
    int lv_server_nid = pv_nid;
    int lv_server_pid;
    int32 lv_oid = 0;
    int lv_error = 0;

    TMTrace(2, ("TM_Info::restart_tm_process : ENTRY.\n"));

    // Make sure we're the Lead TM
    if (!lead_tm())
    {
        TMTrace(1, ("TM_Info::restart_tm_process: Only the Lead TM can restart $TM%d. "
            "This TM is %d and the current Lead TM is %d, aborting self!\n",
            pv_nid, nid(), lead_tm_nid()));
        tm_log_event(DTM_RECOV_NOT_LEAD_TM, SQ_LOG_CRIT, "DTM_RECOV_LOST_LEAD_TM", 
            -1,-1,pv_nid,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,nid(),-1,-1,NULL,lead_tm_nid());
        //msg_mon_shutdown(MS_Mon_ShutdownLevel_Abrupt);
        abort();
    }

    strcpy(la_prog, "tm");
    const char *la_logpath = ms_getenv_str("TRAF_LOG");
    sprintf(la_out_file, "%s/stdout_dtm_%d", la_logpath, pv_nid);

    sprintf (la_buf_tm_name, "$tm%d", pv_nid);

    lock();
    lv_error = msg_mon_start_process2(
                           la_prog,                 /* prog */
                           la_buf_tm_name,          /* name */
                           NULL,                    /* ret name */
                           0,                       /* argc */
                           NULL,                    /* argv */
                           &(iv_open_tms[pv_nid].iv_phandle),
                           1,                       /* open */
                           &lv_oid,                 /* oid */
                           MS_ProcessType_DTM,      /* type */
                           0,                       /* priority */
                           0,                       /* debug */
                           0,                       /* backup */
                           &lv_server_nid,          /* nid */
                           &lv_server_pid,          /* pid */
                           NULL,                    /* infile */
                           (char *) &la_out_file,   /* outfile */
                           true);                   /* unhooked */
    if (lv_error == FEOK)
    {
        iv_open_tms[pv_nid].iv_in_use = 1;
        if (pv_nid > iv_tms_highest_index_used)
            iv_tms_highest_index_used = pv_nid;
        restart_tm_process_helper(pv_nid);
    }
    
    if (lv_error || !can_takeover())
    {
        TMTrace(2, ("TM_Info::restart_tm_process: msg_mon_start_process2 for $TM%d failed with "
                "error %d, canTakeover %d, scheduling retry to timer thread.\n", 
                pv_nid, lv_error, can_takeover()));
        tm_log_event(DTM_TMRESTART_ERROR, SQ_LOG_ERR, "DTM_TMRESTART_ERROR", lv_error, 
            -1, pv_nid, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1, can_takeover());
    }
    else
    {
        iv_allTMsOpen = all_tms_recovered();
        tm_fail_recov_state(pv_nid, TM_FAIL_RECOV_STATE_INITIAL);

		gv_HbaseTM.nodeUp(pv_nid);

        tm_log_event(DTM_TM_RESTARTED, SQ_LOG_INFO, "DTM_TM_RESTARTED", 
            -1,-1, pv_nid, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,NULL, gv_tm_info.nid());
    }

    unlock();
    
    dummy_link_to_refresh_phandle(pv_nid);
//    SB_Thread::Sthr::sleep(100); // in msec
    dummy_link_to_refresh_phandle(pv_nid); // The second one actually updates the phandle
    
     TMTrace(2, ("TM_Info::restart_tm_process : EXIT.\n"));
    return lv_error;
} // TM_Info::restart_tm_process


// -------------------------------------------------------------------
// recover_tm 
// Purpose : This method will be called by the lead TM when a node down
//           message is received by the monitor, or a DTM process death
//           notice is received in the case of a logical node death.
//           It attempts recovery if recovery has not yet completed 
//           successfully.  The gv_tm_info.iv_tm_fail_recov_state will
//           be INITIAL the first time we're called and will change to
//           RUNNING once started and COMPLETE once recovery completes.   
//           This causes the TM to reject disableTrans(shutdown) requests 
//           during recovery.
// -------------------------------------------------------------------
int32 TM_Info::recover_tm(int32 pv_nid)
{
    int    lv_error = 0;
    bool   lv_recov_success = false;
   
    TMTrace (2, ("TM_Info::recover_tm: ENTRY - starting TM on node %d\n",pv_nid));

    // Make sure we're the Lead TM
    if (!lead_tm())
    {
        TMTrace(1, ("TM_Info::recover_tm: Only the Lead TM can restart $TM%d. "
            "This TM is %d and the current Lead TM is %d, aborting self!\n",
            pv_nid, nid(), lead_tm_nid()));
        tm_log_event(DTM_RECOV_NOT_LEAD_TM, SQ_LOG_CRIT, "DTM_RECOV_LOST_LEAD_TM", 
            -1,-1,pv_nid,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,nid(),-1,-1,NULL,lead_tm_nid());
        //msg_mon_shutdown(MS_Mon_ShutdownLevel_Abrupt);
        abort();
    }

    // At least one TM is down
    iv_allTMsOpen = false;

    // No other TMs will be doing work during system recovery, so we can
    // safely restart those TMs without any recovery.
    if (iv_sys_recov_state != TM_SYS_RECOV_STATE_END)
    {
        TMTrace(1, ("TM_Info::recover_tm: System recovery still active when $TM%d failed!\n", pv_nid));
        tm_log_event(DTM_RECOV_LOST_TM_DURING_SYSRECOV, SQ_LOG_INFO, "DTM_RECOV_LOST_TM_DURING_SYSRECOV",
            -1,-1,pv_nid);
        tm_fail_recov_state(pv_nid, TM_FAIL_RECOV_STATE_COMPLETE);
    }

    // We only want to run the recovery the first time we enter recover_tm
    if (tm_fail_recov_state(pv_nid) == TM_FAIL_RECOV_STATE_INITIAL)
    {
       tm_fail_recov_state(pv_nid, TM_FAIL_RECOV_STATE_RUNNING);

       if (can_takeover())
          set_recovery_start(pv_nid);

       lv_recov_success = recover_failed_tm(pv_nid, MAX_TMTIMER_WAIT_TIME);
       if (lv_recov_success)
       {
          TMTrace(2, ("TM_Info::recover_tm: Recovery completed for failed TM %d. "
                   "Enabling shutdown and restarting TM.\n",pv_nid));
          tm_fail_recov_state(pv_nid, TM_FAIL_RECOV_STATE_COMPLETE);
       }
       else
       {
          TMTrace (2, ("TM_Info::recover_tm: Recovery not completed for failed TM %d "
                   " Exiting early.\n", pv_nid));
          addTMRestartRetry(pv_nid, -1);
          return FERETRY;
       }
    }

    // If we are the new lead TM, need to see if any node recoveries are active and
    // need to be redriven
    if (gv_tm_info.lead_tm_takeover()) 
       {
       gv_tm_info.lead_tm_takeover(false);  
       for (int lv_idx = 0; lv_idx < MAX_NODES; lv_idx++)
          {
          if (gv_tm_info.node_being_recovered(lv_idx) != -1)
             {
             TMTrace(2, ("tm_process_node_down_msg  - restarting recovery for node %d.\n", lv_idx));
             lv_error = gv_tm_info.recover_tm(lv_idx);
             }
          }
       }

    iv_allTMsOpen = all_tms_recovered();
    gv_tm_info.set_recovery_end(pv_nid);

    tm_log_event(DTM_TM_RECOVERED, SQ_LOG_INFO, "DTM_TM_RECOVERED", 
        -1,-1, pv_nid, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,NULL, gv_tm_info.nid());

    if (gv_tm_info.mode() == TM_SYNC_MODE)
        broadcast_sync_data (pv_nid);

    TMTrace (2, ("TM_Info::recover_tm: EXIT, returning %d\n", lv_error));
    return lv_error;
} //TM_Info::recover_tm


// ---------------------------------------------------------------------------
// TM_Info::recover_failed_tm
// Purpose : Attempt to recover a failed TM.
// ---------------------------------------------------------------------------
bool TM_Info::recover_failed_tm(int32 pv_nid, int32 pv_rm_wait_time)
{
    bool lv_success = false;
    int32 lv_error = FEOK;

    TMTrace(1, ("TM_Info::recover_failed_tm ENTRY, Failed TM nid %d.\n", pv_nid));

    // NON SYNC MODE
    if (mode() == TM_NONSYNC_MODE)
    {

       // create the NodeRecov object if not yet created
       if (!gv_tm_info.NodeRecov(pv_nid))
          gv_tm_info.NodeRecov(new TM_Recov(pv_rm_wait_time), pv_nid);

       // send recov messages to TSE
       lv_error = gv_tm_info.NodeRecov(pv_nid)->recover_dtm_death(pv_nid);
       if (lv_error != FEOK) // issue an EMS
       {
          tm_log_event(DTM_RECOVERY_FAILED1, SQ_LOG_CRIT, "DTM_RECOVERY_FAILED1",
                       lv_error, /*error_code*/ 
                       -1, /*rmid*/
                       pv_nid); /*dtmid*/ ;
          TMTrace(1, ("TM_Info::recover_failed_tm : Failed to recover from the death of $TM%d.  "
                  "Shutting down!\n", pv_nid));
          // Recovery for dead TM failed, so can't continue
          error_shutdown_abrupt(lv_error);
       }
       else
           lv_success = true;
     //  delete lp_recov;  never get rid of the node recovery object once created in lead TM
    }
    else
    {
       TM_MAP *lp_dataList = get_node_syncDataList(pv_nid);
       do_take_over(lp_dataList);
       lv_success = true;
    }

    TMTrace(1, ("TM_Info::recover_failed_tm EXIT, returning %d.\n", lv_success));
    return lv_success;
} //TM_Info::recover_failed_tm


// ---------------------------------------------------------------------------
// TM_Info::error_shutdown_abrupt
// Purpose : Handle recover error by checking env variable to decide
//           whether to shutdown, loop-wait, or abort
// Parameters:
//      pv_error is the file error which caused the shutdown
// Registry value DTM_ERROR_SHUTDOWN_MODE is used as follows:
//    1  Sleep in 1 minute intervals forever (used for debugging).
//    2  Dump core and don't drive shutdown (debugging).
//    other Shutdown abrupt (normal use).
// ---------------------------------------------------------------------------
void TM_Info::error_shutdown_abrupt(int32 pv_error) {
    int lv_counter = 0;
    int32 lv_error = 0;
    char la_value[9];
    int lv_error_shutdown_mode = 0;

    TMTrace(1, ("TM_Info::error_shutdown_abrupt ENTRY\n"));

    ms_getenv_int ("DTM_ERROR_SHUTDOWN_MODE", &lv_error_shutdown_mode);
    if(!lv_error_shutdown_mode) {
        lv_error = tm_reg_get(MS_Mon_ConfigType_Cluster, 
                       (char *) CLUSTER_GROUP, (char *)DTM_ERROR_SHUTDOWN_MODE, la_value);
        if (lv_error == 0) 
        {
            lv_error_shutdown_mode = atoi(la_value);
        }
    }

    if(lv_error_shutdown_mode == 1) {
        while (1) {
            if(lv_counter == 0) {
                tm_log_event(DTM_ERROR_SHUTDOWN_DEBUG, SQ_LOG_CRIT, "DTM_ERROR_SHUTDOWN_DEBUG",
                     pv_error, /*error_code*/
                     -1, /*rmid*/
                     -1); /*dtmid*/ ;
            }
            TMTrace(1, ("TM_Info::recover_failed_tm Looping where we would normally issue a shutdown Abrupt error %d:  %d iterations", pv_error, lv_counter));
            SB_Thread::Sthr::sleep(60000); // 60 seconds
            lv_counter++;
        }
    }
    else if(lv_error_shutdown_mode == 2) {
        TMTrace(1, ("TM_Info::error_shutdown_abrupt Proceeding with abort. Error: %d, Shutdown mode: %d\n", pv_error, lv_error_shutdown_mode));
        tm_log_event(DTM_ERROR_SHUTDOWN_DEBUG, SQ_LOG_CRIT, "DTM_ERROR_SHUTDOWN_DEBUG",
             pv_error, /*error_code*/
             -1, /*rmid*/
             -1); /*dtmid*/ ;
        abort();
    }
    else {
        TMTrace(1, ("TM_Info::error_shutdown_abrupt Proceeding with shutdown. Error: %d,  Shutdown mode: %d.\n", pv_error, lv_error_shutdown_mode));
        tm_log_event(DTM_ERROR_SHUTDOWN_ABRUPT, SQ_LOG_CRIT, "DTM_ERROR_SHUTDOWN_ABRUPT",
             pv_error, /*error_code*/
             -1, /*rmid*/
             -1); /*dtmid*/ ;
        msg_mon_shutdown(MS_Mon_ShutdownLevel_Abrupt);
    }

    TMTrace(1, ("TM_Info::error_shutdown_abrupt EXIT\n"));
} //TM_Info::error_shutdown_abrupt

// ----------------------------------------------------
// TM_Info::take_over_abort
// Purpose - this is called during a takeover to abort
//           transactions from the down TM.  This will
//           get called for transactions in the ACTIVE
//           and ABORTED state
// ----------------------------------------------------
int32 TM_Info::take_over_abort(TM_Txid_Internal *pv_transid, bool pv_do_phase1, 
                          TM_TX_STATE pv_state)
{
    // import this foreign tx into this TM
    TMTrace(2, ("TM_Info::take_over_abort ENTRY\n"));
    CTmTxBase *lp_tx = (CTmTxBase *) import_tx(pv_transid, pv_state);
    
    if (lp_tx == NULL)
    {
         // we just imported, this shouldn't happen
         // generate EMS message , DTM_TAKEOVER_TX_FAILED
         // DTM_DEATH here when it is available
         tm_log_event(DTM_TAKEOVER_TX_FAILED, SQ_LOG_CRIT, "DTM_TAKEOVER_TX_FAILED",
                    -1, /*error_code*/ 
                    -1, /*rmid*/
                    nid()); /*dtmid*/ 

         TMTrace(1, ("TM_Info::take_over_abort : Invalid transaction to take over in DTM%d\n", 
                 nid()));

         abort();
    }

    // phase 1 and phase 2 (when the tx is in ACTIVE state)
    lp_tx->remove_app_partic(-1, pv_transid->iv_node); // stored as -1 above
    if (pv_do_phase1)
    {
       lp_tx->internal_abortTrans(true /*takeover*/);
    } 
    // phase 2 only (when the TX is in ABORTED state)
    else
    {
       lp_tx->redrive_rollback();
    }
    TMTrace(2, ("TM_Info::take_over_abort EXIT\n"));
    return 0;
} //TM_Info::take_over_abort


// -------------------------------------------------------
// TM_Info::take_over_commit
// Purpose : called during a takeover for transaction in 
//           the COMMITTED state
// -------------------------------------------------------
int32 TM_Info::take_over_commit(TM_Txid_Internal *pv_transid, TM_TX_STATE pv_state)
{
  TMTrace(2, ("TM_Info::take_over_commit ENTRY\n"));

   CTmTxBase *lp_tx = (CTmTxBase *) import_tx(pv_transid, pv_state);
   
   if (lp_tx == NULL)
   {
       // we just imported, this shouldn't be NULL
       // EMS message here, DTM_TAKEOVER_TX_FAILED
       tm_log_event(DTM_TAKEOVER_TX_FAILED, SQ_LOG_CRIT, "DTM_TAKEOVER_TX_FAILED",
                    -1, /*error_code*/ 
                    -1, /*rmid*/
                    nid()); /*dtmid*/ 
       TMTrace(1, ("TM_Info::take_over_commit : Invalid transaction to take over in DTM%d\n", 
                   nid()));

       abort();
   }

   lp_tx->redrive_commit();
   
   TMTrace(2, ("TM_Info::take_over_commit EXIT\n"));

   return 0;
} //TM_Info::take_over_commit


// --------------------------------------------------------
// TM_Info::do_take_over
// Purpose - the lead TM will call this when a NodeDown
//           message is received.
// --------------------------------------------------------
bool TM_Info::do_take_over(TM_MAP *pp_dataList)
{
    TMTrace(2, ("TM_Info::do_take_over ENTRY\n"));
    // for each entry for the foreign TM, process depending on state
    Tm_Tx_Sync_Data *lp_data = (Tm_Tx_Sync_Data *) pp_dataList->get_first();
    bool lv_return = true;

    while (lp_data != NULL && lp_data->iv_is_valid == true && lv_return == true)
    {
        TM_Txid_Internal *lv_transid = (TM_Txid_Internal *)
                                        &lp_data->iv_transid;

        switch (lp_data->iv_state)
        {
        case TM_TX_STATE_ABORTED:
        {
            // redrive abort
            take_over_abort(lv_transid, false, lp_data->iv_state);
            break;
        }
        case TM_TX_STATE_ACTIVE:
        case TM_TX_STATE_BEGINNING:
        case TM_TX_STATE_ABORTING:
        case TM_TX_STATE_ABORTING_PART2:
        {
            // drive abort
            take_over_abort(lv_transid, true, lp_data->iv_state);
            break;
        }
        case TM_TX_STATE_COMMITTED:
        {
            // redrive commit
            take_over_commit(lv_transid, lp_data->iv_state);
            break;
        }
        case TM_TX_STATE_FORGOTTEN:
        {
            break;
        }
        default:
        {
             // EMS message here, DTM_TAKEOVER_TX_FAILED
            tm_log_event(DTM_TAKEOVER_TX_FAILED, SQ_LOG_CRIT, "DTM_TAKEOVER_TX_FAILED",
                    -1, /*error_code*/ 
                    -1, /*rmid*/
                    nid()); /*dtmid*/ 
            TMTrace(1, ("TM_Info::do_take_over : Transaction in invalid state during takeover\n"));
            lv_return = false;
            break;            
        }
        }; //switch on state

        // Get next sync data element
        lp_data = (Tm_Tx_Sync_Data *) pp_dataList->get_next();
    } // while there is still sync data in the list

    // Unlock the list
    pp_dataList->get_end();

    // Clear the list
    pp_dataList->clear();
    TMTrace(2, ("TM_Info::do_take_over EXIT.\n"));
    return lv_return;
} //TM_Info::do_take_over

// -----------------------------------------------------------------------
// TM_Info::set_recovery_start
// Purpose :  send out syncs for each TM we need to set RECOVERY_START for
// ----------------------------------------------------------------------- 
void TM_Info::set_recovery_start(int32 pv_nid)
{
    int32 lv_error = FEOK;
    int32 lv_node_count = 0;
    int32 lv_lnode_count = 0;
    MS_Mon_Node_Info_Entry_Type *lv_info;

    TMTrace(2, ("TM_Info::set_recovery_start ENTRY, Node %d\n",pv_nid));

    gv_tm_info.node_being_recovered (pv_nid, nid());

    //send_takeover_tm_sync(TM_RECOVERY_START, nid(), pv_nid);
               
    lv_error = msg_mon_get_node_info2(&lv_node_count, MAX_NODES, 
                                      NULL, &lv_lnode_count, NULL, NULL, NULL);
    if (lv_error != FEOK)
    {
        TMTrace(1, ("TM_Info::set_recovery_start Fatal Error: msg_mon_get_node_info "
                "returned error %d.\n", lv_error));
        tm_log_event(DTM_TM_GET_NODEINFO_FAILED, SQ_LOG_CRIT, "DTM_TM_GET_NODEINFO_FAILED", 
                     lv_error, -1, pv_nid);
        abort();
    }

    lv_info = new MS_Mon_Node_Info_Entry_Type[lv_node_count];
    if (!lv_info)
    {
        TMTrace(1, ("TM_Info::set_recovery_start Fatal Error: out of memory, %d nodes.\n",
                lv_node_count));
        tm_log_event(DTM_TM_TAKEOVER_NOMEM, SQ_LOG_CRIT, "DTM_TM_TAKEOVER_NOMEM", 
                     FENOBUFSPACE, -1, pv_nid, -1, -1, -1, -1, -1, -1, -1, -1, 
                     -1, -1, lv_node_count);
        abort();
    }
    msg_mon_get_node_info2(&lv_node_count, lv_node_count, lv_info, 
                           NULL, NULL, NULL, NULL);

    // now go through any pending recoveries
 //   for (int32 lv_inx = 0; lv_inx < lv_lnode_count; lv_inx++)
//    {
        // Should not see spare nodes with new API 
        //if (lv_info[lv_inx].spare_node)
        //    continue;
        // Skip down node
 //       if (lv_info[lv_inx].state != MS_Mon_State_Up)
 //           continue;
        //lv_lnode_count++;
  //      if (gv_tm_info.node_being_recovered(lv_info[lv_inx].nid) == -1)
  //         continue;

        // send sync here for start of sync
  //      send_takeover_tm_sync(TM_RECOVERY_START, nid(), lv_info[lv_inx].nid);
  //  }

    delete [] lv_info;
    TMTrace(2, ("TM_Info::set_recovery_start EXIT, %d nodes detected.\n", lv_lnode_count));
} //TM_Info::set_recovery_start


// ---------------------------------------------------------------------------
// TM_Info::set_recovery_end
// Purpose :  sets recovery flag to RECOVERY_END for given TM
// If we are running in Non-sync mode, there's nothing to do here as we
// recovered the indoubt transactions associated with the down node at the 
// beginning of restart_tm which is driven by nodeDown or DTM death notice.
// ---------------------------------------------------------------------------
void TM_Info::set_recovery_end(int32 pv_nid)
{
     bool  lv_success = false;

     TMTrace(2, ("TM_Info::set_recovery_end ENTRY, Node %d\n",pv_nid));

    gv_tm_info.node_being_recovered (pv_nid, -1);

    TMTrace(3, ("TM_Info::set_recovery_end setting recovery_list_built to FALSE for Node %d\n",pv_nid));
    gv_tm_info.recovery_list_built (pv_nid, false);  

    // NON SYNC MODE
    if (mode() == TM_NONSYNC_MODE)
    {
        TMTrace(2, ("TM_Info::set_recovery_end (NONSYNC MODE)\n"));
	lv_success = true;
        // Nothing to do!
    }
    else
    {
         TM_MAP *lp_dataList = get_node_syncDataList(pv_nid);
         lv_success = do_take_over(lp_dataList);
    }

    if (! lv_success)
    {
        tm_log_event(DTM_TM_TAKEOVER_FAILED, SQ_LOG_CRIT, "DTM_TM_TAKEOVER_FAILED",
                     -1, -1, pv_nid);
        TMTrace(1, ("TM_Info::set_recovery_end error occcurred during recovery, Node %d\n",pv_nid));
    }

    TMTrace(2, ("TM_Info::set_recovery_end EXIT with success = %d\n", lv_success));
} //TM_Info::set_recovery_end


// -----------------------------------------------------------------------
// TM_Info::restart_tm_process_helper
// Purpose :  send out syncs for each TM we need to set RECOVERY_START for
//            send out sync for TM up
// ----------------------------------------------------------------------- 
void TM_Info::restart_tm_process_helper(int32 pv_nid)
{
    int32 lv_error = FEOK;
    int32 lv_node_count = 0;
    int32 lv_lnode_count = 0;
    MS_Mon_Node_Info_Entry_Type *lv_info;

    TMTrace(2, ("TM_Info::restart_tm_process_helper ENTRY, Node %d\n",pv_nid));

    lv_error = msg_mon_get_node_info2(&lv_node_count, MAX_NODES, 
                                      NULL, &lv_lnode_count, NULL, NULL, NULL);
    if (lv_error != FEOK)
    {
        TMTrace(1, ("TM_Info::restart_tm_process_helper Fatal Error: msg_mon_get_node_info "
                "returned error %d.\n", lv_error));
        tm_log_event(DTM_TM_GET_NODEINFO_FAILED, SQ_LOG_CRIT, "DTM_TM_GET_NODEINFO_FAILED", 
                     lv_error, -1, pv_nid);
        abort();
    }

    lv_info = new MS_Mon_Node_Info_Entry_Type[lv_node_count];
    if (!lv_info)
    {
        TMTrace(1, ("TM_Info::restart_tm_process_helper Fatal Error: out of memory, %d nodes.\n",
                lv_node_count));
        tm_log_event(DTM_TM_TAKEOVER_NOMEM, SQ_LOG_CRIT, "DTM_TM_TAKEOVER_NOMEM", 
                     FENOBUFSPACE, -1, pv_nid, -1, -1, -1, -1, -1, -1, -1, -1, 
                     -1, -1, lv_node_count);
        abort();
    }
    msg_mon_get_node_info2(&lv_node_count, lv_node_count, lv_info, 
                           NULL, NULL, NULL, NULL);

    // now go through any pending recoveries
    for (int32 lv_inx = 0; lv_inx < lv_lnode_count; lv_inx++)
    {
        // Should not see spare nodes with new API 
        if (lv_info[lv_inx].state != MS_Mon_State_Up)
            continue;
        if (gv_tm_info.node_being_recovered(lv_info[lv_inx].nid) == -1)
           continue;

        // send sync here for start of sync
        //send_takeover_tm_sync(TM_RECOVERY_START, nid(), lv_info[lv_inx].nid);
    }

    delete [] lv_info;

    // process has been restarted, set to up and recover rms
    //send_tm_process_restart_sync(nid(), pv_nid);

    send_tm_state_information();

    TMTrace(2, ("TM_Info::restart_tm_process_helper EXIT, %d nodes detected.\n", lv_lnode_count));
} //TM_Info::restart_tm_process_helper

// --------------------------------------------------------------------------------------
//
// send_tm_state_information
// Purpose : determine what state information we need to send for a node reintegration
//
// --------------------------------------------------------------------------------------
void TM_Info::send_tm_state_information()
{
    TMTrace(2, ("TM_Info::send_tm_state_information ENTRY.\n"));

    int lv_index = 0;   

    for (lv_index = 0; lv_index < MAX_NODES; lv_index++)
    {
        if ((iv_recovery[lv_index].iv_down_without_sync == true) ||
            (iv_recovery[lv_index].iv_node_being_recovered != -1) ||
            (iv_recovery[lv_index].iv_list_built == true))

        {
#ifdef SUPPORT_TM_SYNC
            send_state_resync (iv_nid, iv_recovery[lv_index].iv_down_without_sync, 
                               iv_recovery[lv_index].iv_node_being_recovered,
                               iv_recovery[lv_index].iv_list_built, lv_index);
#endif
            TMTrace(2, ("TM_Info::send_tm_state_information sent state for index %d.\n", lv_index));
        }
    }

    TMTrace(2, ("TM_Info::send_tm_state_information EXIT.\n"));
}

// ----------------------------------------------------------------
// pack_sync_buffer
// Purpose : pack the internal contents of sync data into
//           supplied Request buffer starting at index pv_startAt
//           for MAX_TRANS_PER_SYNC.  
// ----------------------------------------------------------------
void TM_Info::pack_sync_buffer (Tm_Broadcast_Req_Type *pp_data, 
                                int32 pv_node, int32 pv_startAt)
{
   // Walk through the sync data for node pv_node moving it into the broadcast
   // buffer for sending.
   int32 lv_inx = pv_startAt;
   Tm_Tx_Sync_Data *lp_syncData = (Tm_Tx_Sync_Data *) iv_syncDataList[pv_node].get_first();
   while (lp_syncData != NULL && lv_inx < transactionPool()->get_maxPoolSize() && lv_inx < MAX_TRANS_PER_SYNC)
   {
      memcpy (&pp_data->iv_data[lv_inx], lp_syncData, sizeof (Tm_Tx_Sync_Data));
      lv_inx++;
      lp_syncData = (Tm_Tx_Sync_Data *) iv_syncDataList[pv_node].get_next();
   }
   iv_syncDataList[pv_node].get_end();

   // Blank out those entries not in use
   while (lv_inx < MAX_TRANS_PER_SYNC)
   {
      pp_data->iv_data[lv_inx++].iv_is_valid = false;
   }

   pp_data->iv_sys_recov_data.iv_sys_recov_state = iv_sys_recov_state;
   pp_data->iv_sys_recov_data.iv_sys_recov_lead_tm_node = iv_sys_recov_lead_tm_nid;
}

// -------------------------------------------------------------
// unpack_sync_buffer
// Purpose : unpack entire sync data
// ------------------------------------------------------------
void TM_Info::unpack_sync_buffer (Tm_Broadcast_Req_Type *pp_data, int32 pv_node)
{
   Tm_Tx_Sync_Data  *lp_syncData;
   TM_Txid_Internal *lp_transid;
   int32             lv_inx = 0;

   lock();
   // If this is a new sequence number then clear out the sync data
   // slot for ALL nodes before we start to unpack.
   if (pp_data->iv_BroadcastSeqNum > iv_receivingBroadcastSeqNum)
   {
      iv_receivingBroadcastSeqNum = pp_data->iv_BroadcastSeqNum;
      for (int32 lv_node = 0; lv_node < MAX_NODES; lv_node++)
         iv_syncDataList[lv_node].clear();
   }

   // Move all the sync data for the node into the appropriate slot
   while (lv_inx < MAX_TRANS_PER_SYNC && pp_data->iv_data[lv_inx].iv_is_valid == true)
   {
      lp_syncData = new Tm_Tx_Sync_Data;
      memcpy(lp_syncData, &pp_data->iv_data[lv_inx], sizeof(Tm_Tx_Sync_Data));
      lp_transid = (TM_Txid_Internal *) &(lp_syncData->iv_transid);
      iv_syncDataList[pv_node].put(lp_transid->iv_seq_num, lp_syncData);
      lv_inx++;
   }

   iv_sys_recov_state = pp_data->iv_sys_recov_data.iv_sys_recov_state;
   iv_sys_recov_lead_tm_nid = pp_data->iv_sys_recov_data.iv_sys_recov_lead_tm_node;
   unlock();
}

// -------------------------------------------------------------
//  write_trans_state
//  Purpose : write a transaction state record
// -------------------------------------------------------------
void TM_Info::write_trans_state(TM_Txid_Internal *pp_transid, TM_TX_STATE pv_state, 
                                int32 pv_abort_flags, bool pv_hurry)
{
   int32 lv_write_cp = 0;
    
   if (pp_transid == NULL)
   {
      tm_log_event(DTM_TM_INFO_INVALID_TRANSID, SQ_LOG_CRIT,"DTM_TM_INFO_INVALID_TRANSID");
      TMTrace (1, ("TM_Info::write_trans_state - Attempt to write a transaction "
                   " state record with a NULL transid\n"));
      abort (); 
   }

   TMTrace (2, ("TM_Info::write_trans_state: ENTRY txn (%d,%d), state=%d, abortFlags=%d\n", 
            pp_transid->iv_node, pp_transid->iv_seq_num, pv_state, pv_abort_flags));

   // TT - we do not want to write a transaction state record
   //      if this transaction is read only
   if ((TTflagstoint64(pp_transid->iv_tt_flags) & TM_TT_READ_ONLY) == TM_TT_READ_ONLY)
      return; 

   if (pv_state != TM_TX_STATE_NOTX)
   {
        if (iv_audit_mode == TM_NORMAL_AUDIT_MODE)
        {
            lv_write_cp = ip_tmAuditObj->write_trans_state ((TM_Transid_Type *) pp_transid, 
                            pp_transid->iv_node, pv_state, pv_abort_flags);
        }
        else
        {
              char la_buf[REC_SIZE];
              int64 lv_vsn = ip_tmAuditObj->prepare_trans_state_rec(la_buf, REC_SIZE, (TM_Transid_Type *) pp_transid, 
                                                     pp_transid->iv_node, pv_state, pv_abort_flags);
              ip_tmAuditObj->push_audit_rec(REC_SIZE, la_buf, lv_vsn, pv_hurry);
              lv_write_cp = iv_initiate_cp; // set by audit thread
        }
        check_for_rollover(lv_write_cp);
   }
   TMTrace (2, ("TM_Info::write_trans_state: EXIT\n"));
} //TM_Info::write_trans_state


// ---------------------------------------------------------------
// TM_Info::check_for_rollover
// Purpose : This function handles an audit rollover or threshold.
// We note the rollover but there's not much to do here as CPs are
// issued only by the Lead TM and on 2 minute intervals.
// ---------------------------------------------------------------
void TM_Info::check_for_rollover(int32 pv_notification)
{
   // if we need to write a cp - we don't want to overwrite what is there with false
   switch (pv_notification)
   {
       // rolled over
       case 1:
       {
          TMTrace (1,  ("TM_Info::check_for_rollover: Received rollover request\n"));
          iv_cps_in_curr_file = 0;
          iv_write_cp = true;
          iv_lastAuditRolloverTime = Ctimeval::now();
          break; 
       }
       // threshold
       case 2:
       {
          if (iv_cps_in_curr_file < 2)
          {
             TMTrace (1,  ("TM_Info::check_for_rollover: Wrote cp due to threshold\n"));
             iv_write_cp = true;
             iv_lastAuditThresholdTime = Ctimeval::now();
           }
           break;
       }
       // nothing to do
       case 0:
       default:
       {
          break;
       }
    }
             
    if ((!iv_write_cp) && (pv_notification) && (iv_cps_in_curr_file < 2))
    {
       iv_cps_in_curr_file = 0;  // rolled over
       iv_write_cp = pv_notification;
    }
} //TM_Info::check_for_rollover


// ---------------------------------------------------------------
// write_all_trans_state
// Purpose : write all transaction state records that haven't been
//           written in this control point
// Locks the CTmTxBase object to ensure no one changes the state
// while we're writing the trans state record to audit!
// ----------------------------------------------------------------
void TM_Info::write_all_trans_state()
{
   TMTrace (2, ("TM_Info::write_all_trans_state: ENTRY, " PFLL " active txns.\n",
       transactionPool()->get_inUseList()->size()));
   CTmTxBase *lp_tx = transactionPool()->getFirst_inUseList();

   while (lp_tx != NULL)
   {
      if (lp_tx->wrote_trans_state())
         lp_tx->wrote_trans_state(false);
      else
      {
         write_trans_state(lp_tx->transid(), lp_tx->tx_state(),
                           lp_tx->abort_flags(), false /*no hurry*/);
         lp_tx->wrote_trans_state(false);
      }
      lp_tx = transactionPool()->getNext_inUseList();
   }
   transactionPool()->getEnd_inUseList();
   TMTrace (2, ("TM_Info::write_all_trans_state: EXIT\n"));
}

// --------------------------------------------------------------
// TM_Info::write_control_point
// Purpose :  [lead tm only] Send message out to all TMs to 
//            write their state records.  Once they all respond,
//            write the control point.
// --------------------------------------------------------------
void TM_Info::write_control_point(bool pv_cp_only, bool pv_startup)
{
    short                      la_results[6];
    Tm_Control_Point_Req_Type *lp_req = NULL;
    Tm_Control_Point_Rsp_Type *lp_rsp = NULL;
    int32                      lv_error = 0;
    int32                      lv_index = 0;
    int32                      lv_num_sent = 0;
    pid_msgid_struct           lv_pid_msgid[MAX_NODES];
    int32                      lv_reqLen = 0;
    long                       lv_ret;
    long                       lv_ret2;
    int32                      lv_rspLen = 0;
    int                        lv_rsp_rcvd = 0;
    BMS_SRE_LDONE              lv_sre;

    TMTrace (2, ("TM_Info::write_control_point: ENTRY (Lead TM).\n"));
 
    iv_write_cp = false;
    iv_cps_in_curr_file++; // gets reset on rollover

    //initialize lv_pid_msgid
    for (int32 i = 0; i <= tms_highest_index_used(); i++)
    {
        lv_pid_msgid[i].iv_tag = 0;
        lv_pid_msgid[i].iv_msgid = 0;
        lv_pid_msgid[i].iv_nid = 0;
    }

    if ((!pv_cp_only) || (pv_startup))
    {
      TMTrace (3, ("TM_Info::write_control_point Sending CP request to other TMs.\n"));

     lp_req = new Tm_Control_Point_Req_Type [tms_highest_index_used() + 1];  
     lp_rsp = new Tm_Control_Point_Rsp_Type [tms_highest_index_used() + 1];  

     for (int lv_idx = 0; lv_idx <= tms_highest_index_used(); lv_idx++)
     {
         if ((lv_idx == iv_nid) ||
             (iv_open_tms[lv_idx].iv_in_use == false))
         {
            lv_pid_msgid[lv_idx].iv_tag = -1;
         }
         else 
         {
             lv_pid_msgid[lv_idx].iv_tag = lv_idx + 1; // non zero
             lp_req[lv_idx].iv_msg_hdr.rr_type.request_type = TM_MSG_TYPE_CP;
             lp_req[lv_idx].iv_msg_hdr.version.request_version = TM_SQ_MSG_VERSION_CURRENT;
             lv_pid_msgid[lv_idx].iv_nid = lv_idx;
             lp_req[lv_idx].iv_sending_tm_nid = lead_tm_nid();
             lp_req[lv_idx].iv_startup = pv_startup;
         
             lv_reqLen = sizeof (Tm_Control_Point_Req_Type);
             lv_rspLen = sizeof (Tm_Control_Point_Rsp_Type);
             lv_error = link(&(iv_open_tms[lv_idx].iv_phandle),       // phandle,
                          &lv_pid_msgid[lv_idx].iv_msgid,        // msgid
                          (char *) &lp_req[lv_idx],    // reqdata
                          lv_reqLen,                   // reqdatasize
                          (char *) &lp_rsp[lv_idx],    // replydata
                          lv_rspLen,                   // replydatamax
                          lv_pid_msgid[lv_idx].iv_tag, // linkertag
                          TM_TM_LINK_PRIORITY,         // pri
                          BMSG_LINK_LDONEQ,            // linkopts
                          TM_LINKRETRY_RETRIES);       // retry

             if (lv_error != 0)
             {
                 tm_log_event (DTM_TM_INFO_LINK_MSG_FAIL, SQ_LOG_CRIT,"DTM_TM_INFO_INVALID_TRANSID", lv_error);
                 TMTrace (1, ("TM_Info::write_control_point-BMSG_LINK_ failed with error %d\n",lv_error));
                 abort ();
             }
             else
                 lv_num_sent++;
         }
      }

      // LDONE LOOP
      while (lv_rsp_rcvd < lv_num_sent)
      {
        // wait for an LDONE wakeup 
        XWAIT(LDONE, -1);
 
        do {
             // we've reached our message reply count, break
             if (lv_rsp_rcvd >= lv_num_sent)
             {
                 TMTrace (1, ("TM_Info::write_control_point-reached our message reply count, break 2.\n"));
                 break;
             }

             lv_ret = BMSG_LISTEN_((short *)&lv_sre, 
                                    BLISTEN_ALLOW_LDONEM, 0);

             if (lv_ret == BSRETYPE_LDONE)
             {
                lv_index = -1;
                for (int32 lv_idx2 = 0; lv_idx2 <=tms_highest_index_used(); lv_idx2++)
                {
                    if (lv_pid_msgid[lv_idx2].iv_tag == lv_sre.sre_linkTag)
                    {
                       lv_index = lv_idx2;
                       TMTrace (1, ("TM_Info::write_control_point-found a match %d.\n", lv_idx2));
                       break;
                    }
                }
                if (lv_index == -1)
                {
                    tm_log_event (DTM_TM_INFO_NO_LTAG, SQ_LOG_CRIT,"DTM_TM_INFO_NO_LTAG");
                    TMTrace (1, ("TM_Info::write_control_point - Link Tag %d not found in lv_pid_msgid.\n", 
                              (int)lv_sre.sre_linkTag));
                    abort ();
                }

                lv_ret2 = BMSG_BREAK_(lv_pid_msgid[lv_index].iv_msgid, 
                                     la_results,
                                     &(iv_open_tms[lv_pid_msgid[lv_index].iv_nid].iv_phandle)); 

                // We just ignore errors and keep going
                if (lv_ret2 != 0)
                {
                    
                    tm_log_event (DTM_CP_TO_TM_FAILED, SQ_LOG_WARNING, "DTM_CP_TO_TM_FAILED", 
                                  lv_ret2,-1,nid(),-1,lv_pid_msgid[lv_index].iv_msgid,
                                 -1,-1,-1,-1,-1,-1,-1,-1,iv_open_tms[lv_index].iv_in_use,
                                 -1,-1,NULL,lv_index);
                    TMTrace (1, ("TM_Info::write_control_point ERROR BMSG_BREAK_ returned %ld, node %d, msgid %d.\n",
                            lv_ret2, lv_index, lv_pid_msgid[lv_index].iv_msgid));
                }
          
                if (!lv_ret2 && lp_rsp[lv_index].iv_error != 0)
                {
                    tm_log_event(DTM_TM_INFO_CNTRL_PTRSP_FAIL, SQ_LOG_CRIT, 
                                 "DTM_TM_INFO_CNTRL_PTRSP_FAIL",
                                 lp_rsp[lv_index].iv_error,-1,nid(),
                                 -1,lv_pid_msgid[lv_index].iv_msgid,
                                 -1,-1,-1,-1,-1,-1,-1,-1,lv_index);
                    // for now, lets make sure everything succeeded
                    TMTrace (1, ("TM_Info::write_control_point - TM control point respond error %d from node %d "
                             ", msgid %d, index %d.\n",
                             lp_rsp[lv_index].iv_error, nid(), lv_pid_msgid[lv_index].iv_msgid, lv_index));
                }
               lv_rsp_rcvd++;
            }
          } while (lv_ret == BSRETYPE_LDONE); 
       } // while (lv_rsp_rcvd < lv_num_sent)

    /* Removed this check as it causes problems in 2 node configurations when a TM fails.
    // Check we received a response from other TMs.
    // If we didn't receive any then take the Lead TM down as it has become
    // isolated - the others will appoint a new leader.  We wait for one full
    // control point cycle to ensure it's not just a momentary failure which
    // we're in the process of recovering from.
    // If we received some but not all the responses then we can (hopefully)
    // assume we're still the lead TM.  We expect to see node down messages,
    // so don't do anything here.
    if (tms_highest_index_used() > 0 && lv_rsp_rcvd == 0)
    {
        tm_log_event(DTM_TM_INFO_CNTRL_PT_ISOLATED, SQ_LOG_CRIT, 
                     "DTM_TM_INFO_CNTRL_PT_ISOLATED");

        // Abort if we're still isolated after 2 control points.
        if (iv_leadTM_isolated)
        {
           TMTrace (1, ("TM_Info::write_control_point - TM control point detected "
                    "Lead TM isolation for second consecutive control point - assuming "
                    "Lead TM is really isolated and aborting.\n"));
           abort();
        }
        else
        {
           TMTrace (1, ("TM_Info::write_control_point - TM control point detected "
                    "Lead TM isolation. Waiting for next control point to see "
                    "if I'm still isolated.\n"));
           iv_leadTM_isolated = true;
        }
    }
    else
        iv_leadTM_isolated = false;
        */
    iv_leadTM_isolated = false;

    delete []lp_rsp;
    delete []lp_req;
    }
    // everything is ok, lets write the control point
    int32 lv_notification = ip_tmAuditObj->write_control_point(iv_nid);
    check_for_rollover(lv_notification);
    iv_write_cp = false;

    TMTrace (2, ("TM_Info::write_control_point: EXIT\n"));
}


// -----------------------------------------------------------------
// addControlPointEvent
// Purpose: Adds Control Point event to timer thread to be 
//          executed immediately
// -----------------------------------------------------------------
void TM_Info::addControlPointEvent() 
{
   CTmTxMessage    *lp_msg;

   TMTrace (3, ("TM_Info::addControlPointEvent : ENTRY\n"));

   lp_msg = new CTmTxMessage(TM_MSG_TXINTERNAL_CONTROLPOINT);
   addTimerEvent(lp_msg, 0);
   delete lp_msg;

   TMTrace (3, ("TM_Info::addControlPointEvent : EXIT\n"));
}

// -----------------------------------------------------------------
// write_rollover_control_point
// Purpose: Writes a control point due to audit file rollover
//          If called by lead TM then write CP directly, otherwise
//          Send message to Lead to write control point
// -----------------------------------------------------------------
int32 TM_Info::write_rollover_control_point()
{
   Tm_RolloverCP_Req_Type *lp_req = NULL;
   Tm_RolloverCP_Rsp_Type *lp_rsp = NULL;
   int16 la_results[6];
   int32 lv_error = 0;
   int32 lv_msgid = 1; 
   int32     lv_oid=0;
   TPT_DECL (lv_phandle);
   char      la_buffer[8];
   int64 lv_audit_pos = ip_tmAuditObj->audit_position();

   TMTrace (2, ("TM_Info::write_rollover_control_point: ENTRY\n"));
   
   if(lead_tm()) {
      if((lv_audit_pos > iv_audit_seqno) || ((lv_audit_pos == 1) && (iv_audit_seqno != 1))) {
         iv_audit_seqno = lv_audit_pos;
         addControlPointEvent();
      }
   }
   else {
      sprintf(la_buffer, "$tm%d", lead_tm_nid());

      lv_error = msg_mon_open_process(la_buffer,
                                      &lv_phandle,
                                      &lv_oid);
      if (lv_error)
      {
         tm_log_event (DTM_ROLLOVER_OPENPROC_FAILED, SQ_LOG_CRIT,"DTM_ROLLOVER_OPENPROC_FAILED",
                    lv_error, /*error_code*/ 
                    -1, /*rmid*/
                    -1, /*dtmid*/ 
                    -1, /*seq_num*/
                    -1, /*msgid*/
                    -1, /*xa_error*/
                    -1, /*pool_size*/
                    -1, /*pool_elems*/
                    -1, /*msg_retries*/
                    -1, /*pool_high*/
                    -1, /*pool_low*/
                    -1, /*pool_max*/
                    -1, /*tx_state*/
                    -1, /*data */
                    -1, /*data1*/
                    -1, /*data2*/
                    NULL, /*string1*/
                    lead_tm_nid());
         TMTrace (1, ("TM_Info::write_rollover_control_point: Error sending message err: %d \n", lv_error));
         return lv_error;
      }
      
      lp_req = new Tm_RolloverCP_Req_Type();
      lp_rsp = new Tm_RolloverCP_Rsp_Type();
      
      lp_req->iv_msg_hdr.rr_type.request_type = TM_MSG_TYPE_ROLLOVER_CP;
      lp_req->iv_msg_hdr.version.request_version = TM_SQ_MSG_VERSION_CURRENT;
      lp_req->iv_nid = iv_nid;
      lp_req->iv_sequence_no = ip_tmAuditObj->audit_position();

      lv_error = link(&lv_phandle,                                   //phandle
                      &lv_msgid,                                     //msgId
                      (char *)lp_req,                               //reqdata
                      sizeof(Tm_Req_Msg_Type),                       //reqdatsize
                      (char *)lp_rsp,                               //replydata
                      sizeof(Tm_Rsp_Msg_Type),                       //replydatamax
                      0,                                             //linkertag
                      TM_TM_LINK_PRIORITY,                           //pri
                      0,                                             //linkopts
                      TM_LINKRETRY_RETRIES);
      if(!lv_error) {
         lv_error = BMSG_BREAK_(lv_msgid, la_results, 
                                &lv_phandle);
      }
      else {
         tm_log_event (DTM_ROLLOVER_LINK_TO_DTM_FAILED, SQ_LOG_CRIT,"DTM_ROLLOVER_LINK_TO_DTM_FAILED",
                    lv_error, /*error_code*/ 
                    -1, /*rmid*/
                    -1, /*dtmid*/ 
                    -1, /*seq_num*/
                    -1, /*msgid*/
                    -1, /*xa_error*/
                    -1, /*pool_size*/
                    -1, /*pool_elems*/
                    -1, /*msg_retries*/
                    -1, /*pool_high*/
                    -1, /*pool_low*/
                    -1, /*pool_max*/
                    -1, /*tx_state*/
                    -1, /*data */
                    -1, /*data1*/
                    -1, /*data2*/
                    NULL, /*string1*/
                    iv_nid);
         TMTrace (1, ("TM_Info::write_rollover_control_point: Error sending message err: %d \n", lv_error));
      }

      delete lp_req;
      delete lp_rsp;
   }

   TMTrace (2, ("TM_Info::write_rollover_control_point, return code %d : EXIT\n", lv_error));

   return lv_error;
}

// -----------------------------------------------------------------
// write_shutdown
// Purpose: Writes a shutdown record to indicate clean shutdown.
//          The write is always forced.
// -----------------------------------------------------------------
void TM_Info::write_shutdown()
{
    ip_tmAuditObj->write_shutdown(iv_nid, 0 /* for state */);
}

// -----------------------------------------------------------------
// start_backwards_scan
// Purpose : starts a backward scan of TLOG to build outstanding tx
//           list for crash recovery
// -----------------------------------------------------------------
void TM_Info::start_backwards_scan()
{
    ip_tmAuditObj->start_backwards_scan();
}

// -----------------------------------------------------------------
// end_backwards_scan
// Purpose : ends a backward scan of TLOG after outstanding tx
//           list for crash recovery
// -----------------------------------------------------------------
void TM_Info::end_backwards_scan()
{
    ip_tmAuditObj->end_backwards_scan();
}

// -----------------------------------------------------------------
// read_audit_rec
// Purpose : reads one audit record at the cursor from TLog.
// -----------------------------------------------------------------
Addr TM_Info::read_audit_rec()
{
    return ip_tmAuditObj->read_audit_rec();
}

// -----------------------------------------------------------------
// release_audit_rec
// Purpose : releases the previously read record
//           list for crash recovery
// -----------------------------------------------------------------
void TM_Info::release_audit_rec()
{
    ip_tmAuditObj->release_audit_rec();
}

// -----------------------------------------------------------------
// add_sync_data
// Purpose : insert a sync structure into array, the tx was begun on
//           another node by another TM
// -----------------------------------------------------------------
void TM_Info::add_sync_data (int32 pv_nid, Tm_Tx_Sync_Data *pp_data)
{
   if (!pp_data)
      return;

   TM_Txid_Internal *lp_transid = (TM_Txid_Internal*)&(pp_data->iv_transid);
   Tm_Tx_Sync_Data *lp_syncData = new Tm_Tx_Sync_Data;

   memcpy(lp_syncData, pp_data, sizeof(Tm_Tx_Sync_Data));
   lock();
   iv_syncDataList[pv_nid].put(lp_transid->iv_seq_num, lp_syncData);
   unlock();

   lp_syncData->iv_is_valid = true;
}

// ------------------------------------------------------------------
// get_node_syncDataList
// Purpose : Return the sync data list for a given node.  This is
//            used at takeover time to walk through and deal with any
//            outstanding txs
// ------------------------------------------------------------------
TM_MAP * TM_Info::get_node_syncDataList (int32 pv_nid)
{
    return &iv_syncDataList[pv_nid];
}

// -------------------------------------------------------------
// get_sync_data
// Purpose : return the given sync data associated with this
//           transaction.
// -------------------------------------------------------------
Tm_Tx_Sync_Data *TM_Info::get_sync_data (Tm_Tx_Sync_Data *pp_data)
{
    if (!pp_data)
        return NULL;

    TM_Txid_Internal *lp_tx = (TM_Txid_Internal*) &pp_data->iv_transid;
    return (Tm_Tx_Sync_Data *) iv_syncDataList[lp_tx->iv_node].get(lp_tx->iv_seq_num);
}

// ------------------------------------------------------------
// remove_sync_data
// Purpose : remove sync data from the sync data list.
// ------------------------------------------------------------
void TM_Info::remove_sync_data (Tm_Tx_Sync_Data *pp_data)
{
    if (!pp_data)
       return;

    TM_Txid_Internal *lp_tx = (TM_Txid_Internal *) &pp_data->iv_transid;

    lock();
    iv_syncDataList[lp_tx->iv_node].remove(lp_tx->iv_seq_num);
    unlock();
}

// ------------------------------------------------------------
// broadcast_sync_packet
// Send a single broadcast packet to a TM.  This is used to 
// break the Broadcast Sync down into multiple packets when 
// the number of transactions exceeds the maximum that will
// fit in a broadcast buffer (Tm_Broadcast_Request_Type).
// ------------------------------------------------------------
int32 TM_Info::broadcast_sync_packet(TPT_PTR(pp_TMphandle),
                                     int32 pv_node,
                                     Tm_Broadcast_Req_Type *pp_req,
                                     Tm_Broadcast_Rsp_Type *pp_rsp,
                                     int32 pv_start)
{
   char                   la_buf[DTM_STRING_BUF_SIZE];
   int16                  la_results[6];
   int32                  lv_error = FEOK;
   int32                  lv_msgid;
   int32                  lv_size = 0;

   pack_sync_buffer (pp_req, pv_node, pv_start);

   if (pv_node == (MAX_NODES-1))  // last one
       pp_req->iv_state_up = true;
   else
       pp_req->iv_state_up = false;

   pp_req->iv_BroadcastSeqNum = iv_sendingBroadcastSeqNum;
   pp_req->iv_DataStartAddr = pv_start;

   if (iv_trace_level >= 2)
   {
      trace_printf("TM_Info::broadcast_sync_packet ENTRY: Broadcast SeqNum %d, node %d, start at %d\n",
                   iv_sendingBroadcastSeqNum, pv_node, pv_start);
      lv_size = sizeof (Tm_Broadcast_Req_Type);
      trace_printf("DATA : %d\n", lv_size);
   }
    
   lv_error = link(pp_TMphandle,                     // phandle
                   &lv_msgid,                        // msgid
                   (char *) pp_req,                  // reqdata
                   sizeof (Tm_Broadcast_Req_Type),   // reqdatasize
                   (char *) pp_rsp,                  // replydata
                   sizeof (Tm_Broadcast_Rsp_Type),   // replydatamax
                   0,                                // linkertag
                   TM_BROADCAST_LINK_PRIORITY,       // pri
                   0,                                // linkopts
                   TM_LINKRETRY_RETRIES);  

   if (!lv_error)
         lv_error = BMSG_BREAK_(lv_msgid, la_results, pp_TMphandle);

   if (lv_error)
   {   
      // EMS message DTM_BCAST_MSG_TO_DTM_FAILED
      sprintf(la_buf, "Broadcast to DTM%d failed with error %d, broadcast seqnum %d, Starting offset %d.\n", 
               pv_node, lv_error, iv_sendingBroadcastSeqNum, pv_start);
      tm_log_event (DTM_BCAST_MSG_TO_DTM_FAILED, SQ_LOG_CRIT,"DTM_BCAST_MSG_TO_DTM_FAILED",
                    lv_error,-1,-1,iv_sendingBroadcastSeqNum,
                    lv_msgid, /*msgid*/
                    -1, /*xa_error*/
                    -1, /*pool_size*/
                    -1, /*pool_elems*/
                    -1, /*msg_retries*/
                    -1, /*pool_high*/
                    -1, /*pool_low*/
                    -1, /*pool_max*/
                    -1, /*tx_state*/
                    -1, /*data */
                    -1, /*data1*/
                    -1, /*data2*/
                    NULL, /*string1*/
                    pv_node, /*node*/
                    -1, /*msgid2*/
                    pv_start /*offset*/);
      TMTrace (1, ("TM_Info::broadcast_sync_packet - %s", la_buf));

      abort ();
      // TODO: We need to handle this error here and not abort.
      return TM_ERR;
   }

   TMTrace (2, ("TM_Info::broadcast_sync_packet: EXIT\n"));
   return TM_NO_ERR;
}
    
// ------------------------------------------------------------
// broadcast_sync_data
// Purpose : Send a copy of the entire sync data table for
// each node to the respective TM in that node.
// ------------------------------------------------------------
int32 TM_Info::broadcast_sync_data (int32 pv_nid)
{
    char                   la_buffer[8];
    Tm_Broadcast_Req_Type *lp_req = new Tm_Broadcast_Req_Type();
    Tm_Broadcast_Rsp_Type *lp_rsp = new Tm_Broadcast_Rsp_Type();
    int32                  lv_error = 0;
    int32                  lv_TMerror = TM_NO_ERR;
    int32                  lv_oid = 0;
    TPT_DECL               (lv_phandle);

    lp_req->iv_msg_hdr.dialect_type = DIALECT_TM_SQ;
    lp_req->iv_msg_hdr.version.request_version = 
                               TM_SQ_MSG_VERSION_CURRENT;
    lp_req->iv_msg_hdr.miv_err.minimum_interpretation_version = 
                               TM_SQ_MSG_VERSION_CURRENT;
    lp_req->iv_msg_hdr.rr_type.request_type = TM_MSG_TYPE_BROADCAST;
    lp_rsp->iv_msg_hdr.rr_type.reply_type = TM_MSG_TYPE_BROADCAST_REPLY;

    sprintf(la_buffer, "$tm%d", pv_nid);
    lv_error = msg_mon_open_process(la_buffer,
                                    &lv_phandle,
                                    &lv_oid);

    if (lv_error)
    {
         tm_log_event (DTM_CANNOT_OPEN_DTM, SQ_LOG_CRIT,"DTM_CANNOT_OPEN_DTM",
                    lv_error, /*error_code*/ 
                    -1, /*rmid*/
                    -1, /*dtmid*/ 
                    -1, /*seq_num*/
                    -1, /*msgid*/
                    -1, /*xa_error*/
                    -1, /*pool_size*/
                    -1, /*pool_elems*/
                    -1, /*msg_retries*/
                    -1, /*pool_high*/
                    -1, /*pool_low*/
                    -1, /*pool_max*/
                    -1, /*tx_state*/
                    -1, /*data */
                    -1, /*data1*/
                    -1, /*data2*/
                    NULL, /*string1*/
                    pv_nid);
         TMTrace (1, ("broadcast_sync_data : cannot open $TM%d\n", pv_nid));
    
        abort ();
        // TODO: We need to handle this error here and not abort.
        return TM_ERR;
    }

    inc_broadcastSeqNum();
    
    for (int lv_idx = 0; lv_idx < MAX_NODES; lv_idx++)
    {
      for (int lv_startAt = 0; lv_startAt < transactionPool()->get_maxPoolSize(); lv_startAt += MAX_TRANS_PER_SYNC)
      {
         lv_TMerror = broadcast_sync_packet(
                                            &lv_phandle,
                                            lv_idx,
                                            lp_req, lp_rsp, lv_startAt);
         if (lv_TMerror)
            break;
      }
    } // for each node
    
    delete lp_req;
    delete lp_rsp;
    return lv_TMerror;
}


// ------------------------------------------------------------
// new_tx
// Purpose : Instantiated a new transaction object
// pv_creator_nid INPUT: nid of transaction beginner.  This need not
//    match the TM nid.
// pv_creator_pid INPUT: pid of transaction beginner.  For imported 
//    (recovered) transactions this should be the pid of the tm.
// pv_node INPUT, optional, default=-1:  Node id for this transaciton
//    This should always be the node of the beginning TM.
// pv_seq_num INPUT, optional, default=-1: Sequence number of this
//    transaction.  Must be -1 for new transactions (begintxn).
// Returns NULL if we reach the maximum transactions configured
//         Otherwise it returns the txn object pointer.
// ------------------------------------------------------------
void *TM_Info::new_tx(int32 pv_creator_nid, int32 pv_creator_pid, int32 pv_node, int32 pv_seqnum, 
                      void * (*constructPoolElement)(int64))
{
   CTmTxKey lv_txKey(0,0);
   CTmTxBase *lp_tx = NULL;
   bool lv_reused = false;
   
   TMTrace (2, ("TM_Info::new_tx : ENTRY. Txn ID (%d,%d).\n",
            pv_node, pv_seqnum));

   lock();
   if (pv_node != -1 && pv_seqnum != -1)
      lv_txKey.set(pv_node, pv_seqnum);
   else
      // Allocate a new sequence number
      lv_txKey.set(gv_tm_info.nid(), tm_new_seqNum());

   TMTrace (3, ("TM_Info::new_tx : Calling CTmPool<CTmTxBase>::newElement "
         "transid (%d,%d).\n", lv_txKey.node(), lv_txKey.seqnum()));
   lp_tx = ip_transactionPool->newElement(lv_txKey.id(), &lv_reused, 
                                          false, /*force lv_txKey.id() as index*/
                                          constructPoolElement);
   TMTrace (3, ("TM_Info::new_tx : CTmPool<CTmTxBase>::newElement returned "
         "reused %d, CTmTxBase object %p.\n", lv_reused, (void *) lp_tx));

   if (lp_tx)
   {
      lp_tx->initialize(lv_txKey.node(), 0, iv_trace_level, lv_txKey.seqnum(), 
                        pv_creator_nid, pv_creator_pid, iv_rm_wait_time);
      // Add the tx object to the tx lists.
      add_tx(lp_tx);
   }
   unlock();

   TMTrace (2, ("TM_Info::new_tx : EXIT, ID (%d,%d) creator (%d,%d), CTmTxBase object %p.\n",
            lv_txKey.node(), lv_txKey.seqnum(), pv_creator_nid, pv_creator_pid, (void *) lp_tx));

   return (void *) lp_tx;
} //new_tx


// ------------------------------------------------------------------
// import_tx
// Purpose - import a transaction into this TM - used during takeover
// and recovery.
// Parameters:
// pv_state: Input, optional.  Default TM_TX_STATE_NOTX
// pv_txnType: Input, optional.  Default TM_TX_TYPE_DTM
// -------------------------------------------------------------------
void * TM_Info::import_tx (TM_Txid_Internal *pv_transid, TM_TX_STATE pv_state, TM_TX_TYPE pv_txnType)
{
    CTmTxBase *lp_tx;   
    TMTrace(2, ("TM_Info::import_tx : ID (%d,%d) ENTRY\n", pv_transid->iv_node, pv_transid->iv_seq_num));

    // we need to import this transaction into our system in order
    // to properly drive commitment.
    // Right now recovery for XARM is not fully implemented, so this will always be a DTM transaction (the  default).

    switch (pv_txnType)
      {
         case TM_TX_TYPE_DTM:
            lp_tx = (TM_TX_Info *) new_tx(nid(), pid(), pv_transid->iv_node, pv_transid->iv_seq_num,
                                          (void* (*)(long int)) &TM_TX_Info::constructPoolElement);
           break;
         case TM_TX_TYPE_XARM:
            lp_tx = (CTmXaTxn *) new_tx(nid(), pid(), pv_transid->iv_node, pv_transid->iv_seq_num,
                                        (void* (*)(long int)) &CTmXaTxn::constructPoolElement);
            break;
         default:
            TMTrace (2, ("TM_Info::import_txt : ERROR Instantiating new Txn ID (%d,%d) of bad type %d.\n",
                     pv_transid->iv_node, pv_transid->iv_seq_num, pv_txnType));
            tm_log_event(DTM_RECOV_FAIL_BAD_TXN_TYPE, SQ_LOG_CRIT, "DTM_RECOV_FAIL_BAD_TXN_TYPE",
                         -1,-1,pv_transid->iv_node,pv_transid->iv_seq_num,-1,-1,-1,-1,-1,-1,-1,-1,-1,pv_txnType);
            abort();
      }
    
    // An error indicates when we are handling our maximum number of concurrent
    // transactions.
    if (lp_tx == NULL)
    {
       // Removing this event for now as we keep hitting it and it's just announcing that 
       //   we've reached the maximum transactions allowed per node.
       //tm_log_event(DTM_TX_MAX_EXCEEDED, SQ_LOG_WARNING, "DTM_TX_MAX_EXCEEDED", 
       //             FETOOMANYTRANSBEGINS, /*error_code*/ 
       //            -1, /*rmid*/
       //             nid(), /*dtmid*/ 
       //             pv_transid->iv_seqnum, /*seq_num*/
       //             -1, /*msgid*/
       //             -1, /*xa_error*/
       //             transactionPool()->get_maxPoolSize(), /*pool_size*/
       //             transactionPool()->totalElements() /*pool_elems*/);
       TMTrace(1, ("TM_Info::import_tx, FETOOMANYTRANSBEGINS\n"));
    }
    else
    {
       lp_tx->tx_state(pv_state);

       // Start statistics counters TODO
       //lp_tx->stats()->txnTotal()->start();
       //lp_tx->stats()->txnBegin()->start();

       // Don't start thread or queue any work against the transaction yet.
    }

    TMTrace(2, ("TM_Info::import_tx EXIT ID (%d,%d).\n", pv_transid->iv_node, pv_transid->iv_seq_num));
    return (void *) lp_tx;
} //import_tx


// ------------------------------------------------------------
// add_tx
// Purpose : add a transaction to the TX list
// Semaphore locked by caller.
// ------------------------------------------------------------
int32 TM_Info::add_tx(CTmTxBase *pp_tx)
{
   TMTrace (2, ("TM_Info::add_tx : ENTRY.\n"));

   if (!pp_tx)
   {
      TMTrace (1, ("TM_Info::add_tx : NULL pp_tx object pointer passed in.\n"));
      return TM_ERR;
   }

   pp_tx->in_use(true);
   num_active_txs_inc(); 
   gv_system_tx_count++;   

   pidKey lv_pidKey;
   lv_pidKey.k.iv_pid = pp_tx->ender_pid();
   lv_pidKey.k.iv_seqnum = pp_tx->seqnum();
   CTmTxBase * lv_tx = (CTmTxBase *) iv_txPidList.get(lv_pidKey.intKey);
   if (lv_tx)
   {
       TMTrace(1, ("TM_Info::add_tx : Warning: ID %d, pid %d found in iv_txPidList replaced.\n",
               pp_tx->seqnum(), pp_tx->ender_pid()));
       iv_txPidList.remove(lv_pidKey.intKey);
   }
   iv_txPidList.put(lv_pidKey.intKey, pp_tx);

   iv_tx_start_list.push(pp_tx->transid());
   TMTrace (3, ("TM_Info::add_tx : iv_tx_start_list size : " PFLL ".\n", iv_tx_start_list.size()));
   TMTrace (2, ("TM_Info::add_tx : EXIT.\n"));
   return TM_OK;
}

// ------------------------------------------------------------
// get_tx
// Purpose : find a tx in the TX list and return it
// ------------------------------------------------------------
void * TM_Info::get_tx(int32 pv_node, int32 pv_seq)
{
   CTmTxKey k(pv_node, pv_seq);
   return transactionPool()->get(k.id());
}

// ------------------------------------------------------------
// get_tx
// Purpose : find a tx in the TX list and return it
// ------------------------------------------------------------
void * TM_Info::get_tx(TM_Txid_Internal *pv_transid)
{
    CTmTxKey k(pv_transid->iv_node, pv_transid->iv_seq_num);

    return transactionPool()->get(k.id());
}

// ------------------------------------------------------------
// get_tx
// Purpose : find a tx in the TX list and return it
// ------------------------------------------------------------
void * TM_Info::get_tx(int64 pv_txnId) 
{
   CTmTxBase *lp_txBase = transactionPool()->get(pv_txnId);
   void *lp_txn = (void *) lp_txBase;
   return lp_txn;
}

void ** TM_Info::get_all_txs(int64 *pv_size)
{
    return transactionPool()->get_inUseList()->return_all(pv_size);
}
// ------------------------------------------------------------
// getFirst_tx
// Purpose : find the first tx in the TX list and return it
// ------------------------------------------------------------
void * TM_Info::getFirst_tx()
{
    return transactionPool()->getFirst_inUseList();
}

// ------------------------------------------------------------
// getNext_tx
// Purpose : find the next tx in the TX list and return it
// ------------------------------------------------------------
void * TM_Info::getNext_tx()
{
    return transactionPool()->getNext_inUseList();
}

// ------------------------------------------------------------
// getEnd_tx
// Purpose : Unlock the map when we've finished.  Must be called
// by any code which calls getFirst_tx.
// ------------------------------------------------------------
void TM_Info::getEnd_tx()
{
    transactionPool()->getEnd_inUseList();
}

// ------------------------------------------------------------
// getFirst_tx_byPid
// Purpose : get the first transaction object in the tx list with
// with this pid.
//NOTE: This is currently not used and has issues because pid
//      and seqNum are not sufficient to uniquely identify a
//      transaction/process association!!!!!!!!!!!!!!!!
// ------------------------------------------------------------
void *TM_Info::getFirst_tx_byPid(int32 pv_pid)
{
   pidKey lv_pidKey;
   lv_pidKey.k.iv_pid = pv_pid;
   lv_pidKey.k.iv_seqnum = 0;
   return (CTmTxBase *) iv_txPidList.get(lv_pidKey.intKey);
}

// ------------------------------------------------------------
// getNext_tx_byPid
// Purpose : get the next transaction object in the tx list
// with this pid.
// ------------------------------------------------------------
void *TM_Info::getNext_tx_byPid(int32 pv_pid)
{
   CTmTxBase *lp_tx = (CTmTxBase *) iv_txPidList.get_next();
   if (lp_tx != NULL && lp_tx->ender_pid() == pv_pid)
      return lp_tx;
   else
      return NULL;
}

// ------------------------------------------------------------
// getEnd_tx_byPid
// Purpose : Unlock the iv_txPidList when finished.  This method
// must be called after calling getFirst_tx_byPid.
// ------------------------------------------------------------
void TM_Info::getEnd_tx_byPid()
{
   iv_txPidList.get_end();
}

// ------------------------------------------------------------
// remove_tx
// Purpose : return the transaction object to the transactionPool.
// Note that the caller must lock the TM_Info object prior to calling.
// ------------------------------------------------------------
void TM_Info::remove_tx (CTmTxBase * pp_tx)
{

   CTmTxKey k(pp_tx->node(), pp_tx->seqnum());

   TMTrace (2, ("TM_Info::remove_tx : ENTRY, tx object %p ID (%d,%d).\n", 
            (void *) pp_tx, pp_tx->node(), pp_tx->seqnum()));

   pp_tx->cleanup();

   //lock(); Must be locked by caller
   TMTrace (2, ("TM_Info::remove_tx : Calling CTmPool<CTmTxBase>::deleteElement "
      "index (%d, %d).\n", k.node(), k.seqnum()));

   //If it was a recovery transaction from a node failure, we can finally decrement the counter now
   if ((!gv_tm_info.ClusterRecov()) && pp_tx->recovering())
    {
      TMTrace (3, ("TM_Info::remove_tx : Finished recovery for transaction on node %d.\n", k.node()));
      gv_tm_info.NodeRecov(k.node())->dec_txs_to_recover();
      TMTrace (3, ("TM_Info::remove_tx : %d transactions left for recovery on node %d.\n", gv_tm_info.NodeRecov(k.node())->total_txs_to_recover(),
               k.node()));
      if (gv_tm_info.NodeRecov(k.node())->total_txs_to_recover() <= 0)
         {
           TMTrace (3, ("TM_Info::remove_tx : Finished recovery for node %d.\n", k.node()));
           gv_tm_info.NodeRecov(k.node())->listBuilt (false);  // reset since recovery is done
           gv_tm_info.set_recovery_end(k.node());
           tm_log_event(DTM_RECOVERY_COMPLETED, SQ_LOG_NOTICE, "DTM_RECOVERY_COMPLETED",-1,-1,k.node());
           TMTrace (1, ("TM_Info::remove_tx : DTM Recovery completed for node %d.\n", k.node()));
         }
    }

   pidKey lv_pidKey;
   lv_pidKey.k.iv_pid = pp_tx->ender_pid();
   lv_pidKey.k.iv_seqnum = pp_tx->seqnum();
   iv_txPidList.remove(lv_pidKey.intKey);

   ip_transactionPool->deleteElement(k.id());

   num_active_txs_dec();

   TMTrace (3, ("TM_Info::remove_tx : iv_tx_start_list size : " PFLL ".\n", iv_tx_start_list.size()));
   //unlock();

   TMTrace (2, ("TM_Info::remove_tx : EXIT.\n"));
}

// Methods to access sync tags.  Sync Tags replace sync handles.
// ------------------------------------------------------------------
// add_sync_otag
// Purpose : Add a new sync tag to the list.  This also allocates a
// new tag and returns it.  Unlike sync handles which are returned by 
// a call to msg_mon_issue_tmsync(), sync tags are allocated by the
// TM and passed to msg_mon_issue_tmsync().  This avoids a problem
// in multi-threaded TMs where a completion can arrive for a sync
// handle before the msg_mon_issue_tmsync() call completes and the
// handle is known to the TM.
// Note: This function will call lock(true) which allows reentrant
// calls for a thread.
// ------------------------------------------------------------------
int32 TM_Info::add_sync_otag(Tm_Sync_Type_Transid *pp_data)
{
   int32 lv_sync_otag;
   TMTrace (2, ("TM_Info::add_sync_otag ENTRY ID %d\n", 
                   pp_data->u.iv_seqnum));

   // Allocate a new tag
   // Note that we check to make sure the tag is not currently in use.
   lock();
   
   do
   {
      iv_sync_otag++;
      if (iv_sync_otag > 1000000)
         iv_sync_otag = 0;
      lv_sync_otag = iv_sync_otag;
   }
   while (iv_synctags.get(lv_sync_otag) != NULL);

   unlock();

   pp_data->iv_sync_otag = lv_sync_otag;

   iv_synctags.put(lv_sync_otag, pp_data);

   TMTrace (2, ("TM_Info::add_sync_otag, tag %d, EXIT.\n", lv_sync_otag));

   return lv_sync_otag;
}

// ------------------------------------------------------------------
// get_sync_otag
// Purpose : Retrieve the transaction from the iv_synctags map based 
// on pv_tag.
// ------------------------------------------------------------------
Tm_Sync_Type_Transid *TM_Info::get_sync_otag(int32 pv_tag)
{
   Tm_Sync_Type_Transid *lp_data = NULL;

   TMTrace (2, ("TM_Info::get_sync_otag ENTRY with tag %d, size " PFLL ".\n", 
                    pv_tag, iv_synctags.size()));

   lp_data = (Tm_Sync_Type_Transid *) iv_synctags.get(pv_tag);

   if (iv_trace_level >= 2)
   {
      if (lp_data != NULL)
         trace_printf("TM_Info::get_sync_otag EXIT TxnId %d.\n",
                      lp_data->u.iv_seqnum);
      else
         trace_printf("TM_Info::get_sync_otag EXIT lp_data null.\n");
   }

   return lp_data;
}

// ------------------------------------------------------------------
// remove_sync_otag
// Purpose : Remove the tag from iv_synctags map.
// ------------------------------------------------------------------
void TM_Info::remove_sync_otag (int32 pv_tag)
{
   TMTrace (2, ("TM_Info::remove_sync_otag with tag %d , ENTER.\n", pv_tag));
   
   Tm_Sync_Type_Transid *lp_data = (Tm_Sync_Type_Transid *) iv_synctags.remove(pv_tag);

   if (lp_data)
      delete lp_data;
   else
      TMTrace (1, ("TM_Info::remove_sync_otag : WARNING tag not found in iv_synctags!\n"));


   TMTrace (2, ("TM_Info::remove_sync_otag : EXIT.\n"));
}

// ------------------------------------------------------------------
// tm_test_verify
// for testing only.  This returns true if the syncDataList was 
// cleaned up, that, is it is empty.
// ------------------------------------------------------------------
bool TM_Info::tm_test_verify (int32 pv_nid)
{
   if (iv_syncDataList[pv_nid].size() == 0)
      return true;
   else
      return false;
}

// ------------------------------------------------------------------
// Transaction thread related helper functions:
// ------------------------------------------------------------------
// new_thread
// Purpose : Allocate a new thread object.
// ------------------------------------------------------------------
CTxThread * TM_Info::new_thread(CTmTxBase *pp_Txn)
{
   CTxThread *lp_Thread = NULL;
   bool lv_startThread = false;
   lv_startThread = lv_startThread; // compiler error

   TMTrace (2, ("TM_Info::new_thread : ENTRY.\n"));

   // Reject the call if no transaction object was supplied.
   if (!pp_Txn)
   {
      TMTrace (1, ("TM_Info::new_thread : No transaction object specified on call.\n"));
      return NULL;
   }

   TMTrace (2, ("TM_Info::new_thread : Calling CTmPool<CTxThread>::newElement "
            "next index is " PFLL ".\n", iv_txThreadNum));
   bool lv_reused = false;

   lp_Thread = ip_threadPool->newElement(iv_txThreadNum, &lv_reused);
   TMTrace (3, ("TM_Info::new_thread : CTmPool<CTxThread>::newElement returned "
            "reused %d, thread %p.\n", lv_reused, (void *) lp_Thread));
   if (lp_Thread == NULL)
   {
      TMTrace(1, ("TM_Info::new_thread : Maximum threads of %d in use.  "
              "Transaction suspended, waiting for thread to become available.\n", 
              ip_threadPool->get_maxPoolSize()));
      /* Removing event as it's a pain
      tm_log_event(DTM_MAX_THREADS, SQ_LOG_INFO, "DTM_MAX_THREADS",
                  -1, //error_code
                  -1, //rmid
                  nid(), //dtmid 
                  pp_Txn->seqnum(), //seq_num
                  -1, //msgid
                  -1, //xa_error
                  ip_threadPool->get_maxPoolSize(), //pool_size
                  ip_threadPool->totalElements());    //pool_elems */
   }
   else
   {
      if (!lv_reused)
         iv_txThreadNum++;

      // Queue an Initialize event to the thread
      // This is always placed at the top of the queue.
      CTmEvent *lp_event = new CTmEvent(TM_MSG_TXTHREAD_INITIALIZE, lp_Thread);
      lp_event->request()->u.iv_init_txthread.ip_txObject = pp_Txn;
      lp_Thread->eventQ_push_top(lp_event);
   }

   TMTrace (2, ("TM_Info::new_thread : EXIT, thread object %p(%s).\n",
                (void *) lp_Thread, 
                (lp_Thread?lp_Thread->get_name():"undefined")));

   return lp_Thread;
} //new_thread


// ------------------------------------------------------------------
// release_thread
// Purpose : Release a thread object back to the pool.
// The return value indicates whether the calling thread should exit.
// ------------------------------------------------------------------
bool TM_Info::release_thread(CTxThread * pp_thread)
{
   CTmTxMessage    *lp_msg;
   bool             lv_exit = false;

  TMTrace (2, ("TM_Info::release_thread : ENTRY thread object %p(%s).\n",
               (void *) pp_thread, pp_thread->get_name()));

   // Signal thread to terminate
   // This shouldn't happen as we shouldn't have an associated transaction
   CTmTxBase * lp_tx = (CTmTxBase *) pp_thread->transaction();
   if (lp_tx != NULL)
   {
      lp_msg = new CTmTxMessage(TM_MSG_TXTHREAD_RELEASE);
      lp_tx->eventQ_push(lp_msg);
      return false; // Drop out early to process the release.
   }

   if (!check_for_queued_requests(pp_thread))
   {
      TMTrace (2, ("TM_Info::release_thread : Calling CTmPool<CTxThread>::deleteElement "
            "index " PFLL ".\n", pp_thread->threadNum()));
      lv_exit = ip_threadPool->deleteElement(pp_thread->threadNum());
   }

   TMTrace (2, ("TM_Info::release_thread : EXIT, returning %d.\n", lv_exit));
   return lv_exit;
} //TM_Info::release_thread

// ------------------------------------------------------------------
// get_thread
// Purpose : Get the thread with name pp_name.
// Walks through the ip_threadPool's inUseList looking for pp_name.
// ------------------------------------------------------------------
CTxThread * TM_Info::get_thread(char * pp_name)
{
   TMTrace (2, ("TM_Info::get_thread : ENTRY, looking for thread %s.\n", pp_name));


   CTxThread * lp_thread = ip_threadPool->getFirst_inUseList();

   int lv_inx = 0;
   while (lp_thread != NULL && strcmp(pp_name, lp_thread->get_name()) != 0)
   {
      TMTrace (4, ("TM_Info::get_thread : thread %d: name=%s.\n", lv_inx, lp_thread->get_name()));
      lp_thread = ip_threadPool->getNext_inUseList();
      lv_inx++;
   }

   // Must unlock the thread list once we've finished.
   ip_threadPool->getEnd_inUseList();

   TMTrace(2, ("TM_Info::get_thread : EXIT, thread %s found.\n",
           ((lp_thread == NULL)?"not":pp_name)));
   return lp_thread;
} //TM_Info::get_thread


// -------------------------------------------------------------------
// tm_init_other_tms
// Purpose : lead tm will open other tms
// ------------------------------------------------------------------- 
void TM_Info::open_other_tms()
{
    int32  lv_count;
    int32  lv_error;
    int    lv_oid;
    MS_Mon_Process_Info_Type lv_info[MAX_NODES];

    if (!iv_lead_tm)
        return;

    TMTrace (2, ("TM_Info::open_other_tms : ENTRY.\n"));

    // get all TMs in the system and open them all
    lv_error = msg_mon_get_process_info_type(MS_ProcessType_DTM,
                                  &lv_count,
                                  MAX_NODES,
                                  lv_info);

    if (lv_error != 0 && iv_trace_level)
        trace_printf("TM_Info::open_other_tms : Error opening other TMs\n");

    if (lv_error)
    {   
         tm_log_event (DTM_CANNOT_OPEN_DTM, SQ_LOG_CRIT, "DTM_CANNOT_OPEN_DTM", lv_error);
         abort();
    }

    lock();
    for (int lv_idx = 0; lv_idx < lv_count; lv_idx++)
    {
         // we don't want to open ourselves
         if ((lv_info[lv_idx].nid != iv_nid) &&
              (lv_info[lv_idx].state != MS_Mon_State_Stopped))
         {
             TMTrace(3, ("TM_Info::open_other_tms : opening TM %s.\n",
                             lv_info[lv_idx].process_name));

             iv_open_tms[lv_info[lv_idx].nid].iv_in_use = 1;

             if (lv_info[lv_idx].nid > iv_tms_highest_index_used)
                 iv_tms_highest_index_used = lv_info[lv_idx].nid;

             lv_error = msg_mon_open_process(lv_info[lv_idx].process_name,
                                             &(iv_open_tms[lv_info[lv_idx].nid].iv_phandle),
                                             &lv_oid);
             if (lv_error)
             {   
                TMTrace(1, ("TM_Info::open_other_tms : msg_mon_open_process error %d, trying to open %s.\n",
                             lv_error, lv_info[lv_idx].process_name));
                 tm_log_event(DTM_CANNOT_OPEN_DTM, SQ_LOG_WARNING, "DTM_CANNOT_OPEN_DTM", lv_error, 
                              -1, lv_info[lv_idx].nid);
                 // This is generally caused by a window where the TM/node has failed since the 
                 // msg_mon_get_process_info_type() call was made.
                 // Lead TM will retry open when a node_up notification arrives from the monitor.
                 iv_open_tms[lv_info[lv_idx].nid].iv_in_use = 0;
             }
          }
          else
             iv_open_tms[lv_info[lv_idx].nid].iv_in_use = 0;
    }
    iv_allTMsOpen = all_tms_recovered();
    unlock();

    TMTrace (2, ("TM_Info::open_other_tms : EXIT allTMsOpen=%d.\n", iv_allTMsOpen));
}

// -------------------------------------------------------------------
// open_restarted_tm
// Purpose : open the restarted TM by the lead TM
// This routine is ONLY called in response to a monitor TMRestarted message!
// ------------------------------------------------------------------- 
 int32 TM_Info::open_restarted_tm(int32  pv_nid)
 {
   char la_buffer[20];
   int  lv_oid;
   int  lv_error;

   if (!iv_lead_tm)
      return (FEOK);
   
   TMTrace (2, ("TM_Info::open_restarted_tm, nid:%d : ENTRY.\n", pv_nid));
  
   sprintf(la_buffer, "$tm%d", pv_nid);
   lock();
   lv_error = msg_mon_open_process(la_buffer,
                                   &(iv_open_tms[pv_nid].iv_phandle),
                                   &lv_oid);
   if (lv_error)
   {
      TMTrace(1, ("TM_Info::open_restarted_tm : msg_mon_open_process error %d, trying to open %s.\n",
              lv_error, la_buffer));
      tm_log_event(DTM_CANNOT_OPEN_DTM, SQ_LOG_WARNING, "DTM_CANNOT_OPEN_DTM", lv_error,
                   -1, pv_nid);
   }

   if (lv_error == FEOK)
   {
      iv_open_tms[pv_nid].iv_in_use = 1;
      if (pv_nid > iv_tms_highest_index_used)
	  iv_tms_highest_index_used = pv_nid;
      restart_tm_process_helper(pv_nid);
   }

   if (lv_error != 0 && iv_trace_level)
      trace_printf("TM_Info::open_restarted_tm : Error opening TM %d\n",pv_nid);
   unlock();

    dummy_link_to_refresh_phandle(pv_nid);
//    SB_Thread::Sthr::sleep(100); // in msec
    dummy_link_to_refresh_phandle(pv_nid); // The second one actually updates the phandle
    
   TMTrace (2, ("TM_Info::open_restarted_tm, nid:%d : EXIT.\n", pv_nid));
   return lv_error;
}

// -------------------------------------------------------------------
// get_opened_tm_phandle
// Purpose : return the phandle of another TM opened by the lead TM
// ------------------------------------------------------------------- 
SB_Phandle_Type *
    TM_Info::get_opened_tm_phandle(int32 pv_index)
{
   if (iv_open_tms[pv_index].iv_in_use)
      return &(iv_open_tms[pv_index].iv_phandle);
   else
      return NULL;
}

//----------------------------------------------------------------------------
// TM_Info::tm_new_seqNum
// Purpose : This method allocates a new unique sequence number.
// If iv_globalUniqueSeqNum is true then a global unique sequence number is
// retrieved from the Monitor.
// If not, then TM local sequence numbering is used.
// Because sequence number is monotonically increasing, we can only get a 
// collision when we wraparound. 
// The caller must lock the TM_Info object prior to calling tm_new_seqNum.
//----------------------------------------------------------------------------
unsigned int TM_Info::tm_new_seqNum()
{
   bool         lv_noMoreSeqNums = false;
   unsigned int lv_seqNum;
   unsigned int lv_start = 0;

   TMTrace (2, ("TM_Info::tm_new_seqNum: ENTRY\n"));

   if (iv_nextSeqNum >= iv_nextSeqNumBlockStart)
      lv_seqNum = setNextSeqNumBlock();
   else
      lv_seqNum = iv_nextSeqNum;
   iv_nextSeqNum = lv_seqNum + 1;
  
   // If we hit a sequence number that's in use, then we need to roll 
   // over it and look for the next unused one. 
   lv_start = lv_seqNum;

   while (tm_active_seqNum(lv_seqNum))
   {
      // If we're here we got a collision!
      // EMS DTM_SEQNUM_COLLISION
      tm_log_event (DTM_SEQNUM_COLLISION, SQ_LOG_WARNING,"DTM_SEQNUM_COLLISION",
                    -1, /*error_code*/ 
                    -1, /*rmid*/
                    -1, /*dtmid*/ 
                    -1, /*seq_num*/
                    -1, /*msgid*/
                    -1, /*xa_error*/
                    -1, /*pool_size*/
                    -1, /*pool_elems*/
                    -1, /*msg_retries*/
                    -1, /*pool_high*/
                    -1, /*pool_low*/
                    -1, /*pool_max*/
                    -1, /*tx_state*/
                    -1, /*data */
                    -1, /*data1*/
                    -1, /*data2*/
                    NULL, /*string1*/
                    -1, /*node*/
                    -1, /*msgid2*/
                    -1, /*offset*/
                    -1, /*tm_event_msg*/
                    lv_seqNum);

      TMTrace (1, ("TM_Info::tm_new_seqNum: Sequence number %u already in use", lv_seqNum));

      if (lv_seqNum >= iv_nextSeqNumBlockStart)
         lv_seqNum = setNextSeqNumBlock();
      else
         lv_seqNum = iv_nextSeqNum;
      iv_nextSeqNum++;

      if (iv_nextSeqNum == lv_start)
      {
         lv_noMoreSeqNums = true;
         break;
      }
   }

   // If we wrap around and come back to the starting sequence number, there
   // are no free slots - all sequence numbers are allocated!
   if (lv_noMoreSeqNums)
   {
      // EMS DTM_NO_MORE_SEQNUMS
      tm_log_event (DTM_NO_MORE_SEQNUMS, SQ_LOG_CRIT, "DTM_NO_MORE_SEQNUMS");
      TMTrace (1, ("TM_Info::tm_new_seqNum: No more sequence numbers available!"));
      abort ();
   }

   TMTrace (2, ("TM_Info::tm_new_seqNum EXIT: Allocating sequence number %u\n", 
                   lv_seqNum));
   return lv_seqNum;
} //tm_new_seqNum

//----------------------------------------------------------------------------
// TM_Info::tm_new_seqNumBlock
// Purpose: This methods returns a block of sequence number.s
// Because sequence number is monotonically increasing, we can only get a 
// collision when we wraparound. 
// The caller must lock the TM_Info object prior to calling tm_new_seqNumBlock.
//----------------------------------------------------------------------------
void TM_Info::tm_new_seqNumBlock(int pv_blockSize, unsigned int *pp_start, int *pp_count)
{
   TMTrace (2, ("TM_Info::tm_new_seqNumBlock: ENTRY, block Size %d.\n", pv_blockSize));
   *pp_start = getSeqNumBlock(pv_blockSize);
   *pp_count = pv_blockSize;

   TMTrace (2, ("TM_Info::tm_new_seqNumBlock: EXIT. Returned block starting at %d, for %d seqNums.\n",
            *pp_start, *pp_count));
} //tm_new_seqNumBlock


//----------------------------------------------------------------------------
// TM_Info::tm_up
// Purpose : Set the tm state to TM_UP and inform any waiters
//----------------------------------------------------------------------------
void TM_Info::tm_up()
{
   TMTrace (2, ("TM_Info::tm_up : ENTRY TM state %d, sys_recov_state %d, all_rms_closed %d.\n",
            iv_state, iv_sys_recov_state, all_rms_closed()));

   // We need to block new transactions while the RMs are still being opened.
   if (all_rms_closed()) {
      TMTrace(2, ("TM_Info::tm_up. Still waiting for RMS\n"));
      state(TM_STATE_WAITING_RM_OPEN);
   }
   else {
      TMTrace(2, ("TM_Info::tm_up. Setting state to UP\n"));
      state(TM_STATE_UP);
   }

   if (iv_sys_recov_state == TM_SYS_RECOV_STATE_END)
      wake_TMUP_waiters(FEOK);

   TMTrace (2, ("TM_Info::tm_up : EXIT\n"));
}

//----------------------------------------------------------------------------
// TM_Info::wake_TMUP_waiters
// Purpose : Wake up any applications which called TMWAIT.
//----------------------------------------------------------------------------
void TM_Info::wake_TMUP_waiters(short pv_error)
{
   CTmTxMessage * lp_msg = (CTmTxMessage *) iv_TMUP_wait_list.pop();

   while (lp_msg)
   {
      TMTrace (3, ("TM_Info::wake_TMUP_waiters : replying to wait_TMUP request msgid(%d), error(%d).\n",
                      lp_msg->msgid(), pv_error));
      lp_msg->reply(pv_error);
      delete lp_msg;
      lp_msg = (CTmTxMessage *) iv_TMUP_wait_list.pop();
   }
}

//----------------------------------------------------------------------------
// TM_Info::terminate_all_threads
// Purpose : queue a terminate request to all transaction threads.
// This should only be done when the TM is shutting down!
//----------------------------------------------------------------------------
void TM_Info::terminate_all_threads()
{
   CTmEvent        *lp_event;
   CTxThread       *lp_thread;
   int32            lv_signalledThreads = 0;


  TMTrace (2, ("TM_Info::terminate_all_threads : ENTRY.\n"));
   
   // Terminate any threads still in the in-use list
   lp_thread = ip_threadPool->getFirst_inUseList();
   while (lp_thread != NULL)
   {
      lp_event = new CTmEvent(TM_MSG_TXTHREAD_TERMINATE, lp_thread);
      lp_thread->eventQ_push(lp_event);
      lv_signalledThreads++;

     TMTrace (3, ("TM_Info::terminate_all_threads : Warning: signalling in-use "
                      "thread %s to terminate.\n",
                      lp_thread->get_name()));

      lp_thread = ip_threadPool->getNext_inUseList();
   }
   ip_threadPool->getEnd_inUseList();

   // Terminate all threads in the free list
   lp_thread = ip_threadPool->getFirst_freeList();
   while (lp_thread != NULL)
   {
      lp_event = new CTmEvent(TM_MSG_TXTHREAD_TERMINATE, lp_thread);
      lp_thread->eventQ_push(lp_event);
      lv_signalledThreads++;

      TMTrace (3, ("TM_Info::terminate_all_threads : Signalling free thread %s "
                      "to terminate.\n",  lp_thread->get_name()));

      lp_thread = ip_threadPool->getNext_freeList();
   }
   ip_threadPool->getEnd_freeList();

   // Now wait for all transaction threads to terminate
   // The maximum time we wait for is 1 second, then just give up and exit.
   int lv_waitCount = 0;
   while (ip_threadPool->totalElements() > 0 && (lv_waitCount/100) < 1)
   {
      SB_Thread::Sthr::sleep(10); //1/100th of a second
      lv_waitCount++;
   }

   stopTimerEvent();
   stopAuditThread();

   if (iv_trace_level >= 2)
   {
       if (ip_threadPool->totalElements() > 0)
           trace_printf("TM_Info::terminate_all_threads : WARNING %d threads did "
                        "not terminate within 1 second, exiting anyway.\n", 
                        ip_threadPool->totalElements());
       else
           trace_printf("TM_Info::terminate_all_threads : EXIT, all transaction "
                        "threads terminated.\n");
   }
} //terminate_all_threads


//----------------------------------------------------------------------------
// TM_Info::setNextSeqNumBlock
// Purpose : Gets the iv_nextSeqNum based on the DTM_NEXT_SEQNUM_BLOCK registry
// value the first time it is called by the TM. 
// On subsequent calls it increments the sequence number block.
// Finally it sets the next sequence number  block in the registry by adding 
// iv_SeqNumInterval to the next sequence number.
// This function handles sequence number and block wraparound.
// Returns the sequence number which begins the next block.
// Trafodion: Changed to use the next sequence number 
// as base for next block rather than Registry value.
// The caller is expected to lock the TM_Info object.
//----------------------------------------------------------------------------
unsigned int TM_Info::setNextSeqNumBlock()
{
   char          la_seq_num[20];
   char          la_tm_name[8];
   char         *lp_stop;
   int32         lv_error = 0;
   static bool   lv_firstTime = true;
   unsigned int  lv_startSeqNum = 1;

   sprintf(la_tm_name, "$tm%d", iv_nid);

   TMTrace (2, ("TM_Info::setNextSeqNumBlock : ENTRY.\n"));

   if (lv_firstTime)
   {
      lv_firstTime = false;
      iv_SeqNumInterval = TM_DEFAULT_SEQ_NUM_INTERVAL;
      ms_getenv_int("DTM_SEQ_NUM_INTERVAL", &iv_SeqNumInterval);

      lv_error = tm_reg_get(MS_Mon_ConfigType_Process,
                            (char *) la_tm_name, (char *) DTM_NEXT_SEQNUM_BLOCK, 
                            la_seq_num);

      TMTrace (2, ("TM_Info::setNextSeqNumBlock : proc:%s, seqnum block:%s.\n",
                   la_tm_name, 
                   la_seq_num));
      if (lv_error == 0)
         lv_startSeqNum = (unsigned int) strtoul((char *) &la_seq_num, &lp_stop, 10);

      TMTrace (2, ("TM_Info::setNextSeqNumBlock : proc:%s, seqnum block:%s, startseqnum: %d.\n",
                   la_tm_name, 
                   la_seq_num, 
                   lv_startSeqNum));
   }
   else
      lv_startSeqNum = iv_nextSeqNum;
  
   // Check for sequence number wraparound
   if (lv_startSeqNum >= MAX_SEQNUM)
   {
      // EMS DTM_SEQNUM_WRAPAROUND
      tm_log_event (DTM_SEQNUM_WRAPAROUND, SQ_LOG_WARNING, "DTM_SEQNUM_WRAPAROUND");
      TMTrace (1, ("TM_Info::setNextSeqNumBlock: Sequence number wraparound\n"));

      lv_startSeqNum = iv_SeqNumBlockStart = 1;
   }
   else
      // set the sequence number block 
      iv_SeqNumBlockStart = lv_startSeqNum;

   iv_nextSeqNumBlockStart = lv_startSeqNum + iv_SeqNumInterval;

   // Check for short final block
   // In this case we create a short block
   if (iv_nextSeqNumBlockStart < lv_startSeqNum)
   {
      TMTrace (1, ("TM_Info::setNextSeqNumBlock : Sequence number block "
                      "reached maximum sequence number and will wrap on next allocation!\n"));
      iv_nextSeqNumBlockStart = MAX_SEQNUM;
   }

   sprintf(la_seq_num, "%u", iv_nextSeqNumBlockStart);
   lv_error = tm_reg_set(MS_Mon_ConfigType_Process,
                         la_tm_name, (char *) DTM_NEXT_SEQNUM_BLOCK, la_seq_num);

   if (lv_error)
   {
        tm_log_event(DTM_TM_REGISTRY_SET_ERROR, SQ_LOG_CRIT, "DTM_TM_REGISTRY_SET_ERROR", lv_error);
        TMTrace (1, ("Failed to write the DTM next seqnum value into the registry.  Error %d\n", lv_error));
        abort (); 
    }

   TMTrace (2, ("TM_Info::setNextSeqNumBlock : EXIT, current seqNum block: "
            "%u - %u, returning seqNum %u.\n",
            iv_SeqNumBlockStart, (iv_nextSeqNumBlockStart-1), lv_startSeqNum));

   return lv_startSeqNum;
} //setNextSeqNumBlock


//----------------------------------------------------------------------------
// TM_Info::getSeqNumBlock
// Purpose : (Trafodion only)
// Returns the next sequence number.
// Allocates the next sequence number block to the caller.
// This is used by local transactions to allocate a block of
// sequence numbers to a client process.
// Added a TM Library specified block size which must be less
// than the iv_SeqNumInterval.  This allows the Library to 
// grab a block without always updating the registry value.
// The caller is expected to lock the TM_Info object.
//----------------------------------------------------------------------------
unsigned int TM_Info::getSeqNumBlock(int32 pv_blockSize)
{
   char          la_seq_num[20];
   char          la_tm_name[8];
   int32         lv_error = 0;
   unsigned int  lv_startSeqNum = iv_nextSeqNum;
   bool          lv_updateRegistry = false;

   sprintf(la_tm_name, "$tm%d", iv_nid);

   TMTrace (2, ("TM_Info::getSeqNumBlock : ENTRY blockSize %d, nextSeqNum %u, startNextSeqNumBlock %u "
            "seqNumBlockSize %u.\n", pv_blockSize, iv_nextSeqNum, iv_nextSeqNumBlockStart, iv_SeqNumInterval));

   // Check for sequence number wraparound 
   //  We need to check that there is more than a full block left as we're allocating the block to the client.
   if (lv_startSeqNum >= MAX_SEQNUM - pv_blockSize)
   {
      // EMS DTM_SEQNUM_WRAPAROUND
      tm_log_event (DTM_SEQNUM_WRAPAROUND, SQ_LOG_WARNING, "DTM_SEQNUM_WRAPAROUND");
      TMTrace (1, ("TM_Info::getSeqNumBlock: Sequence number wraparound\n"));

      lv_startSeqNum = iv_SeqNumBlockStart = 1;
      lv_updateRegistry = true;
   }
   else
      // set the sequence number block for the TM
      lv_startSeqNum = iv_nextSeqNum;

   iv_nextSeqNum = lv_startSeqNum + pv_blockSize;

   // if we exceeded the TM's allocation, get the next block
   if (iv_nextSeqNum >= iv_nextSeqNumBlockStart) {
      iv_nextSeqNumBlockStart = iv_nextSeqNum + iv_SeqNumInterval;

      // Check for sequence number wraparound 
      //  We need to check that there is more than a full block left as we're allocating the block to the client.
      if (iv_nextSeqNum >= MAX_SEQNUM - iv_SeqNumInterval)
      {
         // EMS DTM_SEQNUM_WRAPAROUND
         tm_log_event (DTM_SEQNUM_WRAPAROUND, SQ_LOG_WARNING, "DTM_SEQNUM_WRAPAROUND");
         TMTrace (1, ("TM_Info::getSeqNumBlock: Sequence number wraparound\n"));

         iv_nextSeqNum = iv_SeqNumBlockStart = 1;
         iv_nextSeqNumBlockStart = 1 + iv_SeqNumInterval;
         lv_updateRegistry = true;
      }
   }

   if (lv_updateRegistry) {
      // Copy new sequence number block back to the registry
      sprintf(la_seq_num, "%u", iv_nextSeqNumBlockStart);
      lv_error = tm_reg_set(MS_Mon_ConfigType_Process,
                            la_tm_name, (char *) DTM_NEXT_SEQNUM_BLOCK, la_seq_num);

      if (lv_error)
      {
           tm_log_event(DTM_TM_REGISTRY_SET_ERROR, SQ_LOG_CRIT, "DTM_TM_REGISTRY_SET_ERROR", lv_error);
           TMTrace (1, ("Failed to write the DTM next seqnum value %u into the registry.  Error %d\n", 
                    iv_nextSeqNumBlockStart, lv_error));
           abort (); 
       }
   }

   TMTrace (1, ("TM_Info::getSeqNumBlock : EXIT returning seqNum block %u - %u, "
            "current TM seqNum block: %u - %u.\n", 
            lv_startSeqNum, (lv_startSeqNum + pv_blockSize-1),
            iv_SeqNumBlockStart, (iv_nextSeqNumBlockStart-1)));

   return lv_startSeqNum;
} //getSeqNumBlock


//----------------------------------------------------------------------------
// TM_Info::check_for_queued_requests
// Purpose : Check the iv_txdisassociatedQ for disassociated transactions and
// reuse the thread if one is found with a request queued.
// Returns true if one was found and the thread was reused.
//         false if no disassociated txns with events queued were found.
// This method is called when releasing a thread back to the free list. 
// There will no longer be an associated transaction, so we don't need to 
// check it here!
//----------------------------------------------------------------------------
bool TM_Info::check_for_queued_requests(CTxThread *pp_thread)
{
   CTmTxBase * lp_txn;
   TM_DEQUE * lp_txn_eventQ;
   TM_DEQUE * lp_txn_PendingRequestQ;

   TMTrace (2, ("TM_Info::check_for_queued_requests : ENTRY.\n"));

   iv_txdisassociatedQ.lock();
   lp_txn = (CTmTxBase *) iv_txdisassociatedQ.get_firstFIFO();
   if (lp_txn)
   {
      lp_txn_eventQ = lp_txn->eventQ();
      lp_txn_PendingRequestQ = lp_txn->PendingRequestQ();
   }
   while (lp_txn && lp_txn_eventQ->empty() && lp_txn_PendingRequestQ->empty())
   {
      TMTrace (3, ("TM_Info::check_for_queued_requests : WARNING thread %s(%ld) found "
                     "transaction ID %d on disassociatedTxn "
                     "queue with no work to do!\n",
                     pp_thread->get_name(), pp_thread->get_id(), lp_txn->seqnum()));
      // Get next
      lp_txn = (CTmTxBase *) iv_txdisassociatedQ.get_nextFIFO();
      if (lp_txn != NULL)
      {
         lp_txn_eventQ = lp_txn->eventQ();
         lp_txn_PendingRequestQ = lp_txn->PendingRequestQ();
      }
   }
   // If we found a disassociated transaction with outstanding events
   // remove the element from the iv_txdisassociatedQ and associate
   // with the thread we have just released.
   // otherwise add the thread to the free list.
   if (lp_txn)
   {
      TMTrace (3, ("CTmTxBase::check_for_queued_requests : Reusing the thread %s(%ld) for "
                        "disassociated transaction, ID %d.\n", 
                        pp_thread->get_name(), pp_thread->get_id(), lp_txn->seqnum()));

      iv_txdisassociatedQ.erase(); //delete the current entry
   }
   iv_txdisassociatedQ.unlock();

   
   // If we have a transaction object pointer then there are outstanding events 
   // for this transaction, reuse this thread to process them.
   if (lp_txn)
   {
      // For worker threads we queue an Initialize event against the thread so that it
      // will process the outstanding events.
      //if (iv_threadModel == worker)
      //{
         TMTrace (3, ("TM_Info::check_for_queued_requests : ID %d:"
            " Thread %s(%ld), TxnObjThread %s(%ld) Worker thread queuing initialize "
            " request to thread to process outstanding events.\n", 
            lp_txn->seqnum(), 
            pp_thread->get_name(), pp_thread->get_id(),
            ((lp_txn->thread()==NULL)?"none":lp_txn->thread()->get_name()), 
            ((lp_txn->thread()==NULL)?-1:lp_txn->thread()->get_id())));

         CTmEvent *lp_event = new CTmEvent(TM_MSG_TXTHREAD_INITIALIZE, pp_thread);
         lp_event->request()->u.iv_init_txthread.ip_txObject = lp_txn;
         TMTrace (3, ("TM_Info::check_for_queued_requests : ID %d:"
            " Thread Initialize event queued to thread %s (%ld).\n", 
            lp_txn->seqnum(), pp_thread->get_name(), 
            pp_thread->get_id()));
         pp_thread->eventQ_push_top(lp_event);
      //}


      // If we have a pending request but nothing in the eventQ and the transaction
      // isn't busy then re-queue it to the transaction.
      if (lp_txn_eventQ->empty() && !lp_txn->transactionBusy())
      {
         // Check the PendingRequest queue
         CTmTxMessage *lp_msg = (CTmTxMessage *) lp_txn_PendingRequestQ->pop_end();
         if (lp_msg)
         {
            TMTrace (3, ("TM_Info::check_for_queued_requests : ID %d"
                           " request popped off PendingRequestQ for msgid(%d).\n", 
                           lp_txn->seqnum(), lp_msg->msgid()));

            lp_txn->queueToTransaction(lp_txn->transid(), lp_msg);
         }
         else
         {
            TMTrace (3, ("TM_Info::check_for_queued_requests : LOGIC ERROR! ID %d "
                              "Didn't find anything in eventQ or PendingRequestQ "
                              "for txn.\n", lp_txn->seqnum()));
         }
      }

      TMTrace(2, ("TM_Info::check_for_queued_requests : EXIT returning true - thread reused.\n"));
      return true;
   } 
   else 
   {
      TMTrace (2, ("TM_Info::check_for_queued_requests : EXIT returning false - nothing outstanding to process, thread discarded.\n"));
      return false;
   }
} // TM_Info::check_for_queued_requests


// ----------------------------------------------------------------------------
// TM_Info::stopTimerEvent
// Purpose : Sends a stop event to the timer thread.  This should be called
// only when the TM is exiting.
// ----------------------------------------------------------------------------
void TM_Info::stopTimerEvent()
{
   TMTrace (2, ("TM_Info::stopTimerEvent : ENTRY.\n"));

   CTmTimerEvent *lp_timerEvent = new CTmTimerEvent(TmTimerCmd_Stop, tmTimer());

   if (tmTimer())
      tmTimer()->eventQ_push((CTmEvent *) lp_timerEvent);

   // Wait for Timer thread to exit.
   int lv_waitCount = 0;
   while (tmTimer()->state() != TmTimer_Down)
   {
      SB_Thread::Sthr::sleep(10); //1/100th of a second
      lv_waitCount++;
      // Write an event every second we have to wait for the thread to exit
      if (lv_waitCount % 100 == 0)
      {
         TMTrace(1, ("TM_Info::stopTimerEvent : Have waited %d sec for Timer "
             "thread to exit.\n", (lv_waitCount/100)));
         //tm_log_event(DTM_TIMER_TH_WAITING_FOR_EXIT, SQ_LOG_WARNING, "DTM_TIMER_TH_WAITING_FOR_EXIT");
      }
   }
   //tmTimer()->stop(); //Stop the thread now
   //SB_Thread::Sthr::sleep(10); //1/100th of a second to allow thread to stop.

   delete tmTimer();
   TMTrace (2, ("TM_Info::stopTimerEvent : EXIT.\n"));
} // TM_Info::stopTimerEvent


// ----------------------------------------------------------------------------
// TM_Info::stopAuditThread
// Purpose : Stops the audit thread.  This should be called
// only when the TM is exiting.  It must be called AFTER stopping the timer
// thread to ensure that we can still process any audit writes which were
// queued by the timer thread before it stopped.
//12/8/2010 Added this to cleanup the audit thread during TM shutdown.
// ----------------------------------------------------------------------------
void TM_Info::stopAuditThread()
{
   TMTrace (2, ("TM_Info::stopAuditThread : ENTRY.\n"));

   tmAuditObj()->exitNow();
   SB_Thread::Sthr::sleep(10); //1/100th of a second to allow thread to stop.

   delete ip_tmAuditObj;

} // TM_Info::stopAuditThread


// ----------------------------------------------------------------------------
// TM_Info::CheckFailed_RMs
// Purpose : Tries to reopen any RMs in failed state.  This is performed
// periodically from a RMRetry timer event, and when a node up notification is
// received.
// It is also invoked to reintegrate a TSE back into the system
// Note that TSEs currently don't support lock reinstatement, so we must mark
// them as TSEBranch_RECOVERING so that they can't process new transactions while we
// have indoubt transactions!
// This routine sets the RM state to 
// TSEBranch_RECOVERING if the TSE crashed or 
// TSEBranch_FAILOVER if a failover was detected.  
// TSEBranch_RECOVERING will drive recovery of any indoubt 
// (hung) transactions.  For failover we don't want to send the 
// TSE an xa_recover as it already has all the details.  We do,
// however want to redrive all hung transasctions.
//
// Paremters:
//           pp_rmname - name of TSE to integrate or NULL for general case
// ----------------------------------------------------------------------------
void TM_Info::CheckFailed_RMs(char *pp_rmname)
{
#define MAX_RM_PROCS MAX_OPEN_RMS*2 //Allow for backups
    int32                    lv_count;
    MS_Mon_Process_Info_Type lv_info[MAX_RM_PROCS];
    int32                    lv_index = 0;
    char                     la_value[9];
    TM_RM_Responses          la_resp[MAX_RM_PROCS];
    int32                    lv_error = FEOK;
    int32                    lv_msg_count = 0;
    RM_Open_struct           lv_open;
    int32                    lv_rmid = 0;
    int32                    la_rmid[MAX_RM_PROCS];
    int32                    lv_rmindex = 0; 
    RM_Info_TSEBranch       *lp_TSEBranchInfo;
    TM_Recov                *lp_recov;  
    TSEBranch_state          lv_RM_state = TSEBranch_RECOVERING; // Assume recovering.
    TSEBranch_state          lv_RM_failover_or_recovery = TSEBranch_RECOVERING;
                             // Set to TSEBranch_FAILOVER if at least one TSE has failed over.

    TMTrace (2, ("TM_Info::CheckFailed_RMs : ENTRY.\n"));

    // get all DP2s in the system and open them all
    msg_mon_get_process_info_type(MS_ProcessType_TSE,
                                  &lv_count,
                                  MAX_RM_PROCS,
                                  lv_info);

    TMTrace(3, ("TM_Info::CheckFailed_RMs : received type for %d rms.\n", lv_count));

    // Walk through the list of current TSEs and work out which ones are new or 
    // marked as TSEBranch_FAILED||TSEBranch_DOWN.  New and failed/down RMs are re-opened.
    for (lv_index = 0; ((lv_index < MAX_RM_PROCS) && (lv_index < lv_count)); lv_index++)
    {
      // we don't care about it if its a backup
      if (lv_info[lv_index].backup == 1)
      {
          la_rmid[lv_index] = -1;
          continue;
      }

      // if a particular TSE name was passed in, 
      // then only worry about that particular TSE, none others
      if ((pp_rmname != NULL) && (strcasecmp(pp_rmname, lv_info[lv_index].process_name) != 0))
      {
         TMTrace(4, ("TM_Info::CheckFailed_RMs : skipping %s, looking for %s.\n",
                      lv_info[lv_index].process_name, pp_rmname));
         continue;
      }
      else
         TMTrace(4, ("TM_Info::CheckFailed_RMs : found %s.\n", pp_rmname));

      lv_error = tm_reg_get(MS_Mon_ConfigType_Process, 
                        lv_info[lv_index].process_name, (char *) "RMID", la_value);
      lv_rmid = (lv_error) ? 0 : (atoi(la_value));

      // Lookup is more efficient by rmid, so use this if possible
      if (lv_rmid == 0)
      {
         lp_TSEBranchInfo = gv_RMs.TSE()->return_slot((char *) lv_info[lv_index].process_name);
         if (lp_TSEBranchInfo != NULL)
            lv_rmid = lp_TSEBranchInfo->rmid();
      }
      else
         lp_TSEBranchInfo = gv_RMs.TSE()->return_slot(lv_rmid);

      // If return_slot returned 0 then this is a new TSE
      if (lp_TSEBranchInfo == NULL)
      {
         // If no RMID was configured, allocate one now
         if (lv_rmid == 0)
         {
             RMID llv_rmid;
             llv_rmid.s.iv_nid = nid(); //TMs nid
             llv_rmid.s.iv_num = lv_index;
             lv_rmid = llv_rmid.iv_rmid;
         } 
         tm_log_event(DTM_TM_INTEGRATING_TSE, SQ_LOG_WARNING, "DTM_TM_INTEGRATING_TSE", -1,
                      lv_rmid, iv_nid, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
                      lv_info[lv_index].pid, -1, -1, (char *) &lv_info[lv_index].process_name, 
                      lv_info[lv_index].nid);
         TMTrace(1, ("TM_Info::CheckFailed_RMs : Integrating new TSE %s (%d,%d), rmid %d into TM.\n", 
                 lv_info[lv_index].process_name, lv_info[lv_index].nid, lv_info[lv_index].pid, lv_rmid));
         // is_ax_reg is set to false here. We need the TSE response to set
         // it correctly, so we do this later.
         // The RM is left in TSEBranch_DOWN state until the open completes.
         lv_error = gv_RMs.TSE()->init(lv_info[lv_index].pid, lv_info[lv_index].nid, 
                                        lv_info[lv_index].process_name, 
                                        lv_rmid, false);
      }
      else
         // Ignore any RM which is already up.
         if (lp_TSEBranchInfo->state() == TSEBranch_UP)
         {
             la_rmid[lv_index] = -1;
             // Update nid & pid every time, it could have changed (failover).
             lp_TSEBranchInfo->nid(lv_info[lv_index].nid);
             lp_TSEBranchInfo->pid(lv_info[lv_index].pid);
             continue;
         }
         else
         {
            tm_log_event(DTM_TM_INTEGRATING_TSE2, SQ_LOG_INFO, "DTM_TM_INTEGRATING_TSE2", -1,
                         lv_rmid, iv_nid, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
                         lv_info[lv_index].pid, -1, -1, (char *) &lv_info[lv_index].process_name, 
                         lv_info[lv_index].nid);
            TMTrace(1, ("TM_Info::CheckFailed_RMs : Reintegrating TSE %s (%d,%d), rmid %d into TM.\n", 
                    lv_info[lv_index].process_name, lv_info[lv_index].nid, lv_info[lv_index].pid, lv_rmid));

            lp_TSEBranchInfo->state(TSEBranch_RECOVERING);
            lp_TSEBranchInfo->nid(lv_info[lv_index].nid);
            lp_TSEBranchInfo->pid(lv_info[lv_index].pid);
         }

      // Now send open to the RM.  Note the wait for completion is later.
      la_rmid[lv_index] = lv_rmid; // coordinate lv_info index with rmid
      strcpy(lv_open.process_name, lv_info[lv_index].process_name);
      lv_open.incarnation_num = gv_tm_info.incarnation_num();
      lv_open.seq_num_block_start = gv_tm_info.SeqNumBlockStart();

      lv_error = (*tm_switch).xa_open_entry ((char *)&lv_open, lv_rmid, TMNOFLAGS);
      switch (lv_error)
      {
      case XA_OK:
       {
         lv_msg_count++;
         break;
       }
      default:
       {
         // Handle RM error. 
         tm_log_event(DTM_RM_OPEN_FAILED, SQ_LOG_CRIT, "DTM_RM_OPEN_FAILED", -1,
                         lv_rmid, iv_nid, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
                         lv_info[lv_index].pid, -1, -1, (char *) &lv_info[lv_index].process_name, 
                         lv_info[lv_index].nid);
         TMTrace(1, ("TM_Info::CheckFailed_RMs - Failed to open RM %s, rmid %d\n",
                 lv_info[lv_index].process_name, lv_rmid));
         // Remark the RM as down since OPEN failed and we don't want to try and sent a recovery to it
         if (lp_TSEBranchInfo != NULL)
            lp_TSEBranchInfo->state(TSEBranch_DOWN);

       } 
      } // switch xa_open error
    } // for each new, down or failed RM
     
    //Since we ignore backup TSEs, there could be gaps in the la_rmid array.
    //So, we cannot just check lv_msg_count entries in the la_rmid array.
    //Instead, we should go through 'lv_index-1' entries.
    lv_rmindex = lv_index-1;

    int32 lv_repliesOutstanding = complete_all(lv_msg_count, la_resp, MAX_TMTIMER_WAIT_TIME);
    if (lv_repliesOutstanding > 0)
    {
         tm_log_event(DTM_RM_REPLY_FAILED, SQ_LOG_WARNING , "DTM_RM_REPLY_FAILED",
                    -1, /*error_code*/ 
                    -1, /*rmid*/
                    -1, /*dtmid*/ 
                    -1, /*seq_num*/
                    -1, /*msgid*/
                    -1, /*xa_error*/
                    -1, /*pool_size*/
                    -1, /*pool_elems*/
                    -1, /*msg_retries*/
                    -1, /*pool_high*/
                    -1, /*pool_low*/
                    -1, /*pool_max*/
                    -1, /*tx_state*/
                    lv_repliesOutstanding); /*data */

         TMTrace(1, ("TM_Info::CheckFailed_RMs - %d RMs failed to reply to xa_open request.\n",
                 lv_repliesOutstanding));
      }

      // if there are not errors, then initialize the rm slot
      // Note that we process any responses even though we may have timed out on the XWAIT.
      for (int32 lv_idx = 0; lv_idx < (lv_msg_count-lv_repliesOutstanding); lv_idx++)
      {
         switch (la_resp[lv_idx].iv_error)
         {
         case XA_RETRY:
          { // This indicates a failover! The new primary TSE replies XA_RETRY 
            // when it receives an xa_open request.
            if(lv_index < MAX_RM_PROCS) {
                tm_log_event(DTM_TM_TSE_FAILOVER, SQ_LOG_WARNING, "DTM_TM_TSE_FAILOVER", -1,
                             lv_rmid, iv_nid, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
                             lv_info[lv_index].pid, -1, -1, (char *) &lv_info[lv_index].process_name, 
                             lv_info[lv_index].nid);
                 TMTrace(1, ("TM_Info::CheckFailed_RMs : TSE %s (%d,%d) failover, rmid %d.\n", 
                        lv_info[lv_index].process_name, lv_info[lv_index].nid, lv_info[lv_index].pid, lv_rmid));
            }
            lv_RM_failover_or_recovery = lv_RM_state = TSEBranch_FAILOVER;
          }
          // Intentional drop through
        case XA_OK:
         {
            // find right slot
            int lv_i = 0;
            for (; lv_i <= lv_rmindex; lv_i++)
                if ((la_rmid[lv_i] != -1) && 
                    (la_rmid[lv_i] == la_resp[lv_idx].iv_rmid))
                    break;

            // if we didn't find the rmid in la_rmid then something is very wrong - give up!
            if (lv_i > lv_rmindex) 
            {
                tm_log_event(DTM_RM_NO_MATCH, SQ_LOG_CRIT, "DTM_RM_NO_MATCH",-1,la_resp[lv_idx].iv_rmid);
                TMTrace(1, ("TM_Info::CheckFailed_RMs - RM id %d did not match any of the RM slots.\n", 
                    la_resp[lv_idx].iv_rmid));
                abort ();
            }

            lv_error = gv_RMs.TSE()->reinit(lv_info[lv_i].pid, lv_info[lv_i].nid, 
                                             lv_info[lv_i].process_name, 
                                             la_resp[lv_idx].iv_rmid, 
                                             la_resp[lv_idx].iv_ax_reg,
                                             false,
                                             lv_RM_state);
            break;
         }
        default: 
         {
            tm_log_event(DTM_RM_OPEN_FAILED2, SQ_LOG_CRIT,"DTM_RM_OPEN_FAILED2",
                          la_resp[lv_idx].iv_error, la_resp[lv_idx].iv_rmid);
            TMTrace(1, ("TM_Info::CheckFailed_RMs - Failed to open RM rmid %d, error %d\n",
                    la_resp[lv_idx].iv_rmid, la_resp[lv_idx].iv_error));
         }
        } // switch response error
    } //for each down/failed/new RM send an xa_open

// Oliver - CR 5283
    // create the NodeRecov object if not yet created
    if (!gv_tm_info.NodeRecov(iv_nid))
        gv_tm_info.NodeRecov(new TM_Recov(MAX_TMTIMER_WAIT_TIME), iv_nid);
    lp_recov = gv_tm_info.NodeRecov(iv_nid);

    // We only want to recover those TSEs which we marked for recovery.
    for (lv_index = 0; 
         ((lv_index <= gv_RMs.TSE()->return_highest_index_used()) && 
          (lv_index < lv_count));
         lv_index++)
    {
       lp_TSEBranchInfo = gv_RMs.TSE()->get_slot(lv_index);
       if (lp_TSEBranchInfo->state() == TSEBranch_RECOVERING)
       {
          lp_recov->recover_tse_restart(lp_TSEBranchInfo->rmid());
          // Only allow the RM to come up if all txns were recovered.
          if (lp_recov->total_txs_to_recover() == 0)
          {
             lp_TSEBranchInfo->up();
             TMTrace(1, ("TM_Info::CheckFailed_RMs : All txns recovered and RM %d is now up.\n", 
                     lp_TSEBranchInfo->rmid()));
          }
          else
          {
             lp_TSEBranchInfo->totalBranchesLeftToRecover(lp_recov->total_txs_to_recover());
             TMTrace(1, ("TM_Info::CheckFailed_RMs : Not all txns recovered. RM %d "
                     "remains in recovering state.\n", lp_TSEBranchInfo->rmid()));
          }
       }
       else
          // Reset failover flag on RM
          if (lp_TSEBranchInfo->state() == TSEBranch_FAILOVER)
             lp_TSEBranchInfo->up();

       if (lp_TSEBranchInfo->state() != TSEBranch_UP)
       {
          // Abort any active transactions which have this RM enlisted.
          abort_active_txns(lp_TSEBranchInfo->rmid());
       }
    } //for each rm to be recovered
    // Oliver - 5283 delete lp_recov;

    /*M8 Commented out. We shouldn't be disabling transactions for the node when an RM fails!
    // Set the TM state now that we've been through all the RMs.  This toggles
    // between UP and TX_DISABLED states.
    if ((state() == TM_STATE_UP && lv_TMState == TM_STATE_TX_DISABLED) ||
        (state() == TM_STATE_TX_DISABLED && lv_TMState == TM_STATE_UP))
        state(lv_TMState);M8*/

    // If we had a failover, force all hung transactions to retry commit/rollback now
    if (lv_RM_failover_or_recovery == TSEBranch_FAILOVER)
    {
       CTmTxBase * lp_txn = (CTmTxBase *) getFirst_tx();
       while (lp_txn)
       {
         lp_txn->hung_redrive();
         lp_txn = (CTmTxBase *) getNext_tx();
       }
       getEnd_tx();
    }
   
    TMTrace(2, ("TM_Info::CheckFailed_RMs : EXIT.\n"));
} // TM_Info::CheckFailed_RMs


void TM_Info::convert_tx_to_str(std::string &pp_str, TM_Txid_Internal &pp_tx, bool pv_empty)
{
    char la_buf[1024];
    TM_Transid_Type *pp_tx_ex =  (TM_Transid_Type *)&pp_tx;

    if (!pv_empty)
        sprintf(la_buf, "%d.%d.%d." PFLL "." PFLL "." PFLL,
                     pp_tx.iv_seq_num, pp_tx.iv_node, pp_tx.iv_incarnation_num,
                     pp_tx_ex->id[1], pp_tx_ex->id[2], pp_tx_ex->id[3]);

/*        sprintf(la_buf, "%d.%d.%d.%d.%d.%d.%d.%d.%d.%d." PFLL,
                     pp_tx.iv_seqnum, pp_tx.iv_node, pp_tx.iv_incarnation_num,
                     pp_tx.iv_tx_flags, pp_tx.iv_tt_flags.Application, pp_tx.iv_tt_flags.Reserved[0],
                     pp_tx.iv_tt_flags.Reserved[1], pp_tx.iv_tt_flags.Predefined,
                     pp_tx.iv_version, pp_tx.iv_check_sum, pp_tx.iv_timestamp);
 */

     
   else
       sprintf (la_buf, "0.0.0.0.0.0");

   pp_str = la_buf;
                  
}


// ----------------------------------------------------------------------------
// TM_Info::abort_active_txns
// This function aborts any transactions in active or beginning states for the
// specified RM.
// ----------------------------------------------------------------------------
void TM_Info::abort_active_txns(int32 pv_rmid)
{
    RM_Info_TSEBranch * lp_TSEBranch;

    TMTrace(2, ("TM_Info::abort_active_txns : ENTRY rmid %d.\n", pv_rmid));

    // Look up the RM slot first and get the index as it's more efficient than
    // using rmid lookup for every transaction.
    int32 lv_rmidx = gv_RMs.TSE()->return_slot_index(pv_rmid);

    if (lv_rmidx == -1)
    {
        tm_log_event(DTM_RM_NOT_FOUND, SQ_LOG_CRIT,"DTM_RM_NOT_FOUND", -1, pv_rmid);
        TMTrace(1, ("TM_Info::abort_active_txns : Programming Bug!! Rmid %d "
            "not found in global RM list.\n", pv_rmid));
        abort();
    }

    int32 lv_count = 0;
    CTmTxBase *lp_tx = (CTmTxBase*) getFirst_tx();

    while (lp_tx != NULL)
    {
        if (lp_tx->tx_state() == TM_TX_STATE_NOTX ||
            lp_tx->tx_state() == TM_TX_STATE_ACTIVE ||
            lp_tx->tx_state() == TM_TX_STATE_BEGINNING)
        {
            lp_TSEBranch = lp_tx->TSEBranch(lv_rmidx);
            if (lp_TSEBranch == NULL)
            {
                tm_log_event(DTM_RM_NOT_FOUND2, SQ_LOG_CRIT,"DTM_RM_NOT_FOUND2", -1, pv_rmid);
                TMTrace(1, ("TM_Info::abort_active_txns : Programming Bug!! Rmid %d "
                    "not found in global TSE RM list(2).\n", pv_rmid));
                abort();
            }
            if (lp_TSEBranch->partic() && lp_TSEBranch->in_use() && lp_TSEBranch->state() != TSEBranch_UP)
            {
                lp_tx->internal_abortTrans(true);  // Abort the tx for shutdown.
                lv_count++;
            }
        }
        lp_tx = (CTmTxBase*) getNext_tx();
    }
   getEnd_tx();
    TMTrace(2, ("TM_Info::abort_active_txns : EXIT %d Txns aborted.\n", lv_count));
} //TM_Info::abort_active_txns


// ----------------------------------------------------------------------------
// TM_Info::abort_all_active_txns
// This function aborts all transactions in active or beginning states.
// ----------------------------------------------------------------------------
void TM_Info::abort_all_active_txns()
{
    TMTrace(2, ("TM_Info::abort_all_active_txns : ENTRY.\n"));

    int32 lv_count = 0;
    CTmTxBase *lp_tx = (CTmTxBase *) getFirst_tx();

    while (lp_tx != NULL)
    {
        if (lp_tx->tx_state() == TM_TX_STATE_NOTX ||
            lp_tx->tx_state() == TM_TX_STATE_ACTIVE ||
            lp_tx->tx_state() == TM_TX_STATE_BEGINNING)
        {
            lp_tx->internal_abortTrans(true);  // Abort the tx for shutdown.
            lv_count++;
        }
        lp_tx = (CTmTxBase*) getNext_tx();
    }
   getEnd_tx();
   tm_log_event(DTM_TM_SHUTDOWN_ABORT_TXNS, SQ_LOG_INFO,"DTM_TM_SHUTDOWN_ABORT_TXNS",
                -1,-1,gv_tm_info.nid(),-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,lv_count);
   TMTrace(2, ("TM_Info::abort_all_active_txns : ENTRY %d Txns aborted.\n", lv_count));
} //TM_Info::abort_all_active_txns


// ----------------------------------------------------------------------------
// TM_Info::state
// Sets the TM state to a new value.
// ----------------------------------------------------------------------------
void  TM_Info::state (int32 pv_state)
{ 
   char la_buf[DTM_STRING_BUF_SIZE];
   int32 lv_old_state = iv_state;
   char * lp_tmState = tmStatetoa(pv_state);

   lock();
   iv_state = pv_state; 
   unlock();

   sprintf(la_buf, "Old state %d, new state %d", lv_old_state, iv_state);
   tm_log_event(DTM_TM_STATE_CHANGE, SQ_LOG_INFO,"DTM_TM_STATE_CHANGE",
                       -1, /*error_code*/ 
                       -1, /*rmid*/
                       iv_nid, /*dtmid*/ 
                       -1, /*seq_num*/
                       -1, /*msgid*/
                       -1, /*xa_error*/
                       -1, /*pool_size*/
                       -1, /*pool_elems*/
                       -1, /*msg_retries*/
                       -1, /*pool_high*/
                       -1, /*pool_low*/
                       -1, /*pool_max*/
                       iv_state,  /*tx_state  - TM state in this case!*/
                       lv_old_state, /*data*/
                       -1, /*data1*/
                       -1, /*data2*/
                       lp_tmState /*string1*/);

   TMTrace(1, ("TM_Info::state, TM state changed from %d(%s) to %d(%s).\n", lv_old_state, tmStatetoa(lv_old_state), iv_state, tmStatetoa(iv_state)));
} //TM_Info::state


// ----------------------------------------------------------------------------
// TM_Info::addTimerEvent
// Purpose : Wrapper to simplify the addition of timer events to the timer 
// threads event queue.  Because there is no transaction associated with these
// events, they will be executed by the timer thread.  There must be a 
// corresponding implementation, something like TM_Info::enableTrans.
// pv_type is a message type
// pv_delayInterval is the interval the timer thread will wait before
// executing the request in msec.
// ----------------------------------------------------------------------------
CTmTimerEvent * TM_Info::addTimerEvent(CTmTxMessage *pp_msg, int pv_delayInterval)
{
   TMTrace (2, ("TM_Info::addTimerEvent : ENTRY, msg type %d, delay %d\n", 
      pp_msg->requestType(), pv_delayInterval));

   CTmTimerEvent *lp_timerEvent = new CTmTimerEvent(pp_msg, pv_delayInterval);

   tmTimer()->eventQ_push((CTmEvent *) lp_timerEvent);

   return lp_timerEvent;
} // TM_Info::addTimerEvent


// ----------------------------------------------------------------------------
// TM_Info::addTMRestartRetry
// Purpose : Add a TM Restart retry event.  This event will schedule/retry the call
// to TM_Info::restart_tm.  If it fails then the code also calls addTMRestartRetry
// to put the event back on the queue.  
// There will be one of these on the timer list for each failed non-lead TM
// in the Lead TM only.
// pv_nid input: Node id of the TM to be restarted.
// pv_waitTime: Time interval the Timer thread waits for before driving the event.
//              -1 = default (TM_Info::iv_TMRestartRetry_interval)
//              0  = no wait - execute immediately.
//              > 0 = wait time in msec
// ----------------------------------------------------------------------------
CTmTimerEvent * TM_Info::addTMRestartRetry(int32 pv_nid, int32 pv_waitTime=-1)
{
   int32 lv_wait = (pv_waitTime < 0)? iv_TMRestartRetry_interval: pv_waitTime;
   TMTrace (2, ("TM_Info::addTMRestartRetry : ENTRY, $TM%d, wait %d.\n", pv_nid, lv_wait));

   if (pv_nid < 0)
   {
       tm_log_event(DTM_TM_RESTART_RETRY_PROGERROR, SQ_LOG_CRIT, "DTM_TM_RESTART_RETRY_PROGERROR",
                    -1,-1,nid(),-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,NULL,pv_nid);
       TMTrace(1, ("TM_Info::addTMRestartRetry : Programming ERROR : Bad node id %d\n",
               pv_nid));
       abort();
   }

   //Must never cancel a on off event!
   // We no longer want to cancel the event.  Duplicates will get taken
   // care of appropriately when popped off the queue
  // if (iv_open_tms[pv_nid].ip_restartTimerEvent)
  //    cancelTMRestartEvent(pv_nid);

   CTmTimerEvent *lp_timerEvent = 
       new CTmTimerEvent(TM_MSG_TXINTERNAL_TMRESTART_RETRY, lv_wait);
   lp_timerEvent->request()->u.iv_tmrestart_internal.iv_nid = pv_nid;

   tmTimer()->eventQ_push((CTmEvent *) lp_timerEvent);

   // Save the event pointer for node restarts
   iv_open_tms[pv_nid].ip_restartTimerEvent = lp_timerEvent;

   return lp_timerEvent;
} //TM_Info::addTMRestartRetry


// --------------------------------------------------------------
// TM_Info::cancelTMRestartEvent
// Purpose : Cancel the TM Restart event.  
// --------------------------------------------------------------
void TM_Info::cancelTMRestartEvent(int32 pv_nid)
{
   TMTrace (2, ("TM_Info::cancelTMRestartEvent : ENTRY. Node %d, Event %p.\n",
            pv_nid, (void *) iv_open_tms[pv_nid].ip_restartTimerEvent));

   if (iv_open_tms[pv_nid].ip_restartTimerEvent != NULL)
      tmTimer()->cancelEvent(iv_open_tms[pv_nid].ip_restartTimerEvent);

   iv_open_tms[pv_nid].ip_restartTimerEvent = NULL;
} //TM_Info::cancelTMRestartEvent


// ----------------------------------------------------------------------------
// TM_Info::addTMRecoveryWait
// Purpose : Add a TM Recovery wait event.  This event will wait for transactions
// to be 0 to continue with recovery.  If greater than 0 then it will
// put the event back on the queue.  
// pv_nid input: -1 is cluster recovery else node recovery
// pv_delay input: amount of time to delay 
// ----------------------------------------------------------------------------
CTmTimerEvent * TM_Info::addTMRecoveryWait(int32 pv_nid, int32 pv_delay)
{
   TMTrace (2, ("TM_Info::addTMRecoveryWait : ENTRY, node: %d delay time: %d\n", pv_nid, pv_delay));

   CTmTimerEvent *lp_timerEvent = 
       new CTmTimerEvent(TM_MSG_TXINTERNAL_RECOVERY_WAIT, pv_delay);

   lp_timerEvent->request()->u.iv_tmrecovery_internal.iv_nid = pv_nid;

   tmTimer()->eventQ_push((CTmEvent *) lp_timerEvent);

   // Do not need to store event info since multiple events for a given node are fine

   return lp_timerEvent;
} //TM_Info::addTMRecoveryWait


// ----------------------------------------------------------------------------
// TM_Info::sendAllTMs
// Purpose : Sends the request out to all open TMs and waits for their replies.
// The caller must be the Lead TM.
// NOTE: This function adds 10000 (TM_TM_MSG_OFFSET) to the request type to  
// make sure it can be distingushed by the receiving TM from an incoming client 
// request.
// ----------------------------------------------------------------------------
int32 TM_Info::sendAllTMs(CTmTxMessage *pp_msg)
{
    short                     la_results[6];
    Tm_Req_Msg_Type             *lp_req = NULL;
    Tm_Rsp_Msg_Type             *lp_rsp = NULL;
    int32                     lv_error = 0;
    int32                     lv_index = 0;
    int32                     lv_num_sent = 0;
    pid_msgid_struct          lv_pid_msgid[MAX_NODES];
    int32                     lv_reqLen = 0;
    long                      lv_ret;
    long                      lv_ret2;
    int32                     lv_rspLen = 0;
    int                       lv_rsp_rcvd = 0;
    BMS_SRE_LDONE             lv_sre;

    //initialize lv_pid_msgid
    for (int32 i = 0; i <= tms_highest_index_used(); i++)
    {
        lv_pid_msgid[i].iv_tag = 0;
        lv_pid_msgid[i].iv_msgid = 0;
        lv_pid_msgid[i].iv_nid = 0;
    }

    TMTrace (2, ("TM_Info::sendAllTMs ENTRY. Sending request %d request to other TMs.\n", pp_msg->requestType()));

     lp_req = new Tm_Req_Msg_Type [tms_highest_index_used() + 1];  
     lp_rsp = new Tm_Rsp_Msg_Type [tms_highest_index_used() + 1];

     for (int lv_idx = 0; lv_idx <= tms_highest_index_used(); lv_idx++)
     {
         if ((lv_idx == iv_nid) ||
             (iv_open_tms[lv_idx].iv_in_use == false))
         {
            lv_pid_msgid[lv_idx].iv_tag = -1;
         }
         else 
         {
             lv_pid_msgid[lv_idx].iv_tag = lv_idx + 1; // non zero
             memcpy (&lp_req[lv_idx], pp_msg->request(), sizeof(Tm_Req_Msg_Type)); 
             lp_req[lv_idx].iv_msg_hdr.rr_type.request_type = pp_msg->requestType() + TM_TM_MSG_OFFSET;
             lp_req[lv_idx].iv_msg_hdr.version.request_version = TM_SQ_MSG_VERSION_CURRENT;
             lv_pid_msgid[lv_idx].iv_nid = lv_idx;
         
             lv_reqLen = sizeof (Tm_Req_Msg_Type);
             lv_rspLen = sizeof (Tm_Rsp_Msg_Type);
             lv_error = link(&(iv_open_tms[lv_idx].iv_phandle),     // phandle,
                             &lv_pid_msgid[lv_idx].iv_msgid,        // msgid
                             (char *) &lp_req[lv_idx],    // reqdata
                             lv_reqLen,                   // reqdatasize
                             (char *) &lp_rsp[lv_idx],    // replydata
                             lv_rspLen,                   // replydatamax
                             lv_pid_msgid[lv_idx].iv_tag, // linkertag
                             TM_TM_LINK_PRIORITY,         // pri
                             BMSG_LINK_LDONEQ,            // linkopts
                             TM_LINKRETRY_WAITFOREVER);

             if (lv_error != 0)
             {
               //  sprintf(la_buf, "BMSG_LINK_ failed with error %d\n", lv_error);
               //  tm_log_write(DTM_TM_INFO_LINK_MSG_FAIL, SQ_LOG_CRIT, la_buf);
                 TMTrace (1, ("TM_Info::sendAllTMs-BMSG_LINK_ failed with error %d. failure ignored.\n",lv_error));
                 // Ignore errors here. We assume that the TM is down.
                 //abort ();
                 return lv_error;
             }
             else
                 lv_num_sent++;
         }
      }

      // LDONE LOOP
      while (lv_rsp_rcvd < lv_num_sent)
      {
        // wait for an LDONE wakeup 
        XWAIT(LDONE, -1);
 
        do {
             // we've reached our message reply count, break
             if (lv_rsp_rcvd >= lv_num_sent)
                 break;

             lv_ret = BMSG_LISTEN_((short *)&lv_sre, 
                                    BLISTEN_ALLOW_LDONEM, 0);

             if (lv_ret == BSRETYPE_LDONE)
             {
                lv_index = -1;
                for (int32 lv_idx2 = 0; lv_idx2 <=tms_highest_index_used(); lv_idx2++)
                {
                    if (lv_pid_msgid[lv_idx2].iv_tag == lv_sre.sre_linkTag)
                    {
                       lv_index = lv_idx2;
                       break;
                     }
                }
                if (lv_index == -1)
                {
                    tm_log_event(DTM_TM_INFO_NO_LTAG, SQ_LOG_WARNING, "DTM_TM_INFO_NO_LTAG");
                    TMTrace (1, ("TM_Info::sendAllTMs - Link Tag %d not found\n", (int)lv_sre.sre_linkTag));
                    lv_error = FEDEVDOWN;
                }

                if (!lv_error)
                {
                    lv_ret2 = BMSG_BREAK_(lv_pid_msgid[lv_index].iv_msgid, 
                                         la_results,
                                         &(iv_open_tms[lv_pid_msgid[lv_index].iv_nid].iv_phandle)); 
                    if (lv_ret2 != 0)
                    {
                        tm_log_event(DTM_TM_INFO_MSGBRK_FAIL2, SQ_LOG_WARNING, "DTM_TM_INFO_MSGBRK_FAIL2",
                                     lv_ret2,-1,nid(),-1,lv_pid_msgid[lv_index].iv_msgid);
                        TMTrace (1, ("TM_Info::sendAllTMs ERROR BMSG_BREAK_ returned %ld, index %d, msgid %d.\n",
                                lv_ret2, lv_index, lv_pid_msgid[lv_index].iv_msgid));
                        lv_error = FEDEVDOWN;
                    }
                }
          
                if (lv_error || lp_rsp[lv_index].iv_msg_hdr.miv_err.error != 0)
                {
                    if (lv_error)
                        tm_log_event(DTM_TM_INFO_SENDALL_FAIL, SQ_LOG_WARNING, "DTM_TM_INFO_SENDALL_FAIL", lv_error);
                    else
                    {
                        tm_log_event(DTM_TM_INFO_SENDALL_FAIL, SQ_LOG_WARNING, "DTM_TM_INFO_SENDALL_FAIL", lp_rsp[lv_index].iv_msg_hdr.miv_err.error);
                        TMTrace (1, ("TM_Info::sendAllTMs - Request %d responded with error %d, %d. Ignoring.\n", 
                             pp_msg->requestType(), lv_error, lp_rsp[lv_index].iv_msg_hdr.miv_err.error));
                        lv_error = lp_rsp[lv_index].iv_msg_hdr.miv_err.error;
                    }
                }
               lv_rsp_rcvd++;
            }
          } while (lv_ret == BSRETYPE_LDONE); 
       } // while (lv_rsp_rcvd < lv_num_sent)
    delete []lp_rsp;
    delete []lp_req;
    TMTrace (2, ("TM_Info::sendAllTMs : EXIT, %d.\n", lv_error));
    return lv_error;
} //TM_Info::sendAllTMs


// ----------------------------------------------------------------------------
// TM_Info::set_txnsvc_ready
// Purpose : Set the SQ_TXNSVC_READY registry value.
// Note that this routine will NOT return if there is an error, but will
// instead abort().
// When SQ_TXNSVC_READY is "1" the transaction service is available.
//                           "0" the transaction service is unavailable.
//                         "2" transaction service is unavailable, shutdown phase 2.
// The value of 2 is used to tell the sqstop shell script that it can now
// commence phase 2 of shutdown.
// ----------------------------------------------------------------------------
void TM_Info::set_txnsvc_ready(int32 pv_flag)
{
    int32 lv_error = FEOK;
    TMTrace (2, ("TM_Info::set_txnsvc_ready : ENTRY, ready? %d.\n", pv_flag));

    // set registry entry to indicate whether transaction service is ready.
    switch (pv_flag)
    {
    case 0:
       lv_error = tm_reg_set(MS_Mon_ConfigType_Cluster, 
                             (char *) "CLUSTER", (char *) "SQ_TXNSVC_READY",
                             (char *) "0");
       break;
    case 1:
       lv_error = tm_reg_set(MS_Mon_ConfigType_Cluster, 
                             (char *) "CLUSTER", (char *) "SQ_TXNSVC_READY",
                             (char *) "1");
       break;
    case 2:
       lv_error = tm_reg_set(MS_Mon_ConfigType_Cluster, 
                             (char *) "CLUSTER", (char *) "SQ_TXNSVC_READY",
                             (char *) "2");
       break;
    default:
        lv_error = FEINVALOP;
    }

    if (lv_error != FEOK)
    {
        tm_log_event(DTM_TM_REGISTRY_SET_ERROR, SQ_LOG_CRIT, "DTM_TM_REGISTRY_SET_ERROR", lv_error);
        TMTrace(1, ("TM_Info::set_txnsvc_ready - Registry entry error %d.  TM is not ready!\n", lv_error));
        abort ();
    }
    TMTrace (2, ("TM_Info::set_txnsvc_ready : EXIT.\n"));
} //TM_Info::set_txnsvc_ready

// ----------------------------------------------------------------------------
// TM_Info::attachRm
// Purpose : attach an RM back into the system
// ----------------------------------------------------------------------------
int32 TM_Info::attachRm(CTmTxMessage * pp_msg)
{
    int32 lv_error = FEOK;
    char *lp_TSEBranchname = pp_msg->request()->u.iv_attachrm.ia_rmname;
    TMTrace (2, ("TM_Info::attachRm for %s : ENTRY.\n", lp_TSEBranchname));
    CheckFailed_RMs (lp_TSEBranchname);

    TMTrace (2, ("TM_Info::attachRm : EXIT.\n"));
    return lv_error;
}

// ----------------------------------------------------------------------------
// TM_Info::enableTrans
// Purpose : Executes an enabletransaction command within the Lead TM.
// Lead TM: Sends the enableTrans out to all open TMs and waits for their replies.
// non-Lead TMs: enable transactions if in the right state.
// Returns a FEDEVDOWN error if the TM is not in tx disabled state, or
//         an error if sendAllTMs fails.
// enableTrans is called by both lead and non-lead TMs.
// ----------------------------------------------------------------------------
int32 TM_Info::enableTrans(CTmTxMessage * pp_msg)
{
    int32 lv_error = FEOK;

    TMTrace (2, ("TM_Info::enableTrans : ENTRY.\n"));

    // You can only enable transactions when they have been disabled.
    // If we're shutting down it's already too late to enable transactions
    if (state() != TM_STATE_TX_DISABLED)
    {
        TMTrace (1, ("TM_Info::enableTrans: EXIT - TM not in TxDisabled state.\n"));
        return FEDEVDOWN;
    }

    if (iv_lead_tm)
    {
       lv_error = sendAllTMs(pp_msg);
       if (lv_error == FEOK)
       {
          tm_up();
          set_txnsvc_ready(TXNSVC_UP);
       }
       else
       {  // Currently we don't handle an error, just issue a warning and continue
          tm_log_event(DTM_TM_ENABLETRANS_FAIL, SQ_LOG_WARNING, "DTM_TM_ENABLETRANS_FAIL", lv_error);
          TMTrace(1, ("TM_Info::enableTrans - Error %d returned by sendAllTMs.\n", lv_error));
       }
    }
    else // Non-lead TMs must reply to the lead TM now
    {
       pp_msg->reply(lv_error);
       tm_up();
    }

    TMTrace (2, ("TM_Info::enableTrans : EXIT, error %d.\n", lv_error));
    return lv_error;
} //TM_Info::enableTrans


// ----------------------------------------------------------------------------
// TM_Info::disableTrans
// Purpose : Executes an disabletransaction command within both the Lead TM
// and all other TMs (via sendAllTMs).
// Lead TM: Sends the disableTrans out to all open TMs and waits for their replies.
// non-Lead TMs: disable transactions if in the right state.
// Returns the error returned by sendAllTMs.
// disableTrans is called by both lead and non-lead TMs.
// ----------------------------------------------------------------------------
int32 TM_Info::disableTrans(CTmTxMessage * pp_msg)
{
    int32 lv_error = FEOK;
    TMTrace (2, ("TM_Info::disableTrans : ENTRY, reqType %d, shutdown level %d.\n", 
             pp_msg->requestType(), pp_msg->request()->u.iv_disabletrans.iv_shutdown_level));

    // We should never get an abrupt!!
    /*if (pp_msg->request()->u.iv_disabletrans.iv_shutdown_level == TM_DISABLE_SHUTDOWN_ABRUPT)
    {
        TMTrace (1, ("TM_Info::disableTrans : disableTrans ABRUPT, shutting down SQ!\n"));
        msg_mon_shutdown(MS_Mon_ShutdownLevel_Abrupt);
        return FEBADERR;
    } */

    if ((state() == TM_STATE_DOWN || state() == TM_STATE_WAITING_RM_OPEN ||
         state() == TM_STATE_SHUTTING_DOWN || state() == TM_STATE_SHUTDOWN_FAILED || 
         state() == TM_STATE_SHUTDOWN_COMPLETED || state() == TM_STATE_QUIESCE))
    {
        lv_error = FEBADSTATE;
        tm_log_event(DTM_TM_DISABLETRANS_TOOLATE, SQ_LOG_WARNING, "DTM_TM_DISABLETRANS_TOOLATE", 
            lv_error, -1, -1, nid(), -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, state());
        TMTrace (1, ("TM_Info::disableTrans: Error %d - Too late for disableTrans - TM state = %d.\n", 
            lv_error, state()));
    }
    else
    {
        if (pp_msg->request()->u.iv_disabletrans.iv_shutdown_level == TM_DISABLE_SHUTDOWN_NORMAL || 
            pp_msg->request()->u.iv_disabletrans.iv_shutdown_level == TM_DISABLE_SHUTDOWN_IMMEDIATE)
        {
            state(TM_STATE_TX_DISABLED_SHUTDOWN_PHASE1);
            TMTrace (3, ("TM_Info::disableTrans: Shutdown Phase 1 detected, level %d.\n", 
                     pp_msg->request()->u.iv_disabletrans.iv_shutdown_level));
            addShutdownPhase1WaitEvent(pp_msg);
            // Only delete the message object here if we are not the lead TM.  The lead TM will reply and delete
            // it in tm_process_req_disabletrans when it's finished scheduling the request. 
            if (!iv_lead_tm)
               delete pp_msg;
        }
        else 
            if (state() == TM_STATE_TX_DISABLED_SHUTDOWN_PHASE1)
            {
                lv_error = FEINVALIDSTATE;
                TMTrace (1, ("TM_Info::disableTrans: Error %d - Already shutting down.\n", lv_error));
            }
            else
            {

                state(TM_STATE_TX_DISABLED);
                // Lead TM waits here for all other TMs to complete disableTrans here
                if (iv_lead_tm)
                {
                   lv_error = sendAllTMs(pp_msg);
                   if (lv_error == FEOK)
                   {
                     TMTrace (3, ("TM_Info::disableTrans: Disable, no shutdown.\n"));
                     set_txnsvc_ready(TXNSVC_DOWN);
                   }
                   else
                   {  // Currently we don't handle an error, just issue a warning and continue
                      tm_log_event(DTM_TM_DISABLETRANS_FAIL, SQ_LOG_WARNING, "DTM_TM_DISABLETRANS_FAIL", lv_error);
                      TMTrace(1, ("TM_Info::disableTrans - Error %d returned by sendAllTMs.\n", lv_error));
                   }
                }
            }
    }

    TMTrace(2, ("TM_Info::disableTrans : EXIT, error %d.\n", lv_error));
    return lv_error;
} //TM_Info::disableTrans


// --------------------------------------------------------------
// TM_Info::addShutdownPhase1WaitEvent
// Purpose : Add a Shutdown Phase 1 Wait Event to wait for 
// transactions to complete.
// All TMs call this.
// pp_msg is the original disableTrans message if this is the 
// lead tm, or message from the lead TM if non-lead. We copy the 
// msgid and request into our event so that the timer thread can 
// reply if this is a non-lead TM.
// --------------------------------------------------------------
void TM_Info::addShutdownPhase1WaitEvent(CTmTxMessage * pp_msg)
{
   TMTrace (2, ("TM_Info::addShutdownPhase1WaitEvent : ENTRY, msgid %d.\n", pp_msg->msgid()));

   CTmTxMessage *lp_msg = new CTmTxMessage(pp_msg->request(), pp_msg->msgid());
   cancelShutdownPhase1WaitEvent();

   // Add Shutdown Phase 1 Wait Timer event.
   // This is always processed by the timer thread, so no need to specify thread or tranasction.
   // Repeat it forever, with an interval of TM_SHUTDOWNP1WAIT_DEFAULT msec.
   lp_msg->requestType(TM_MSG_TXINTERNAL_SHUTDOWNP1_WAIT);
   tmTimer()->ShutdownP1_event(new CTmTimerEvent(lp_msg, TM_SHUTDOWNP1WAIT_DEFAULT, -1));

   tmTimer()->eventQ_push((CTmEvent *) tmTimer()->ShutdownP1_event());

   TMTrace (2, ("TM_Info::addShutdownPhase1WaitEvent : EXIT, new TmTxMessage 0x%p, msgid %d\n", (void *) lp_msg, lp_msg->msgid()));
} //TM_Info::addShutdownPhase1WaitEvent


// --------------------------------------------------------------
// TM_Info::cancelShutdownPhase1WaitEvent
// Purpose : Cancel the Shutdown Phase 1 Wait event.  
// --------------------------------------------------------------
void TM_Info::cancelShutdownPhase1WaitEvent()
{
   TMTrace (2, ("TM_Info::cancelShutdownPhase1WaitEvent : ENTRY. Event %p\n",
            (void *) tmTimer()->ShutdownP1_event()));

   // If we already have a shutdown event then we must have created a corresponding
   // TmTxMessage object.
   if (tmTimer()->ShutdownP1_event() != NULL)
      tmTimer()->cancelEvent(tmTimer()->ShutdownP1_event());

   tmTimer()->ShutdownP1_event(NULL);
} //TM_Info::cancelShutdownPhase1WaitEvent


// ---------------------------------------------------------------------------
// TM_Info::ShutdownPhase1Wait
// This is called as a recurring timer event to wait for transactions to
// complete or abort during the first part of shutdown.  This is driven by
// a disable transactions, shutdown normal|immediate and executes within
// the Timer thread.
// ---------------------------------------------------------------------------
void TM_Info::ShutdownPhase1Wait(CTmTxMessage *pp_msg)
{
   static bool lv_first = true;
   static int32 lv_calls = 1;
   int32 lv_error = FEOK;
   int32 lv_activeTxns = transactionPool()->get_inUseList()->size();

   TMTrace (2, ("TM_Info::ShutdownPhase1Wait : ENTRY msgid %d, msg 0x%p, first %d, call %d.\n",
            pp_msg->msgid(), (void *) pp_msg, lv_first, lv_calls++));
   if (lv_first)
   {
      lv_first = false;
      // If this is a shutdown immediate then abort all transactions now.
      if (pp_msg->request()->u.iv_disabletrans.iv_shutdown_level == TM_DISABLE_SHUTDOWN_IMMEDIATE)
      {
         abort_all_active_txns();
         tm_log_event(DTM_DISABLE_TRANSACTIONS_IMMEDIATE, SQ_LOG_WARNING, 
             "DTM_DISABLE_TRANSACTIONS_IMMEDIATE",-1,-1,nid(),-1,-1,-1,-1,
             -1,-1,-1,-1,-1,-1,num_active_txs());
      }
      else
         tm_log_event(DTM_DISABLE_TRANSACTIONS_NORMAL, SQ_LOG_WARNING, 
             "DTM_DISABLE_TRANSACTIONS_NORMAL",-1,-1,nid(),-1,-1,-1,-1,
             -1,-1,-1,-1,-1,-1,num_active_txs());

      // Lead TM waits here for all other TMs to complete transaction processing here
      if (iv_lead_tm)
         lv_error = sendAllTMs(pp_msg);
   }
   else
   {
      tm_log_event(DTM_TM_SHUTDOWNP1WAIT_RUNNING, SQ_LOG_WARNING, 
            "DTM_TM_SHUTDOWNP1WAIT_RUNNING",-1,-1,nid(),-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,num_active_txs(), lv_calls);
   }

   if (lv_error)
   {
      if (iv_lead_tm)
      {
         tm_log_event(DTM_TM_SHUTDOWNP1WAIT_ERR, SQ_LOG_CRIT, "DTM_TM_SHUTDOWNP1WAIT_ERR", 
                      lv_error, -1, nid(), -1, -1, -1, -1, -1, -1, -1, -1, -1, state(), lv_activeTxns);
         TMTrace (1, ("TM_Info::ShutdownPhase1Wait Lead TM error %d, there are still %d active txns waiting.\n",
                  lv_error, lv_activeTxns));
         msg_mon_shutdown(MS_Mon_ShutdownLevel_Abrupt);
      }
      else
      {
         tm_log_event(DTM_TM_SHUTDOWNP1WAIT_NONLEAD_ERR, SQ_LOG_ERR, "DTM_TM_SHUTDOWNP1WAIT_NONLEAD_ERR", 
                      lv_error, -1, nid(), -1, -1, -1, -1, -1, -1, -1, -1, -1, state(), lv_activeTxns);
         TMTrace (1, ("TM_Info::ShutdownPhase1Wait non-lead TM error %d, there are still %d active txns waiting.\n",
                  lv_error, lv_activeTxns));
         pp_msg->reply(lv_error);
      }
   }
   else
   {
      lv_activeTxns = transactionPool()->get_inUseList()->size();
        
      if (lv_activeTxns)
      {
         TMTrace (3, ("TM_Info::ShutdownPhase1Wait There are still %d active txns waiting.\n",
                  lv_activeTxns));
         if (lv_calls == 1 || lv_calls % 10)
            tm_log_event(DTM_TM_SHUTDOWNP1WAIT_TXNS, SQ_LOG_INFO, "DTM_TM_SHUTDOWNP1WAIT_TXNS", 
                         -1, -1, nid(), -1, -1, -1, -1, -1, -1, -1, -1, -1, state(), lv_activeTxns);
      }
      else
      {
         if (iv_lead_tm)
            set_txnsvc_ready(TXNSVC_SHUTDOWN);
         else
            pp_msg->reply(lv_error);
         cancelShutdownPhase1WaitEvent();
      }
   }

   TMTrace (2, ("TM_Info::ShutdownPhase1Wait : EXIT.\n"));
} //TM_Info::ShutdownPhase1Wait


// ---------------------------------------------------------------------------
// TM_Info::link
// Purpose : Centralized call to BMSG_LINK_.
// All linkers should use this function.
// This function will retry any retriable errors such as FENOLCB (30).
// Parameters are as for BMSG_LINK_ without control, xmitclass and including
// transid.
//  pp_phandle      input
//  pp_msgid        output
//  pp_reqdata      input
//  pv_reqdatasize  input
//  pp_replydata    input (where to write reply data to)
//  pp_replydatamax input (max size of reply buffer)
//  pv_linkertag    input
//  pv_pri          input priority
//  pv_linkopts     input Link options
//  pv_transid      input Default 0
//  pv_maxretries    input Maximum retries. Default is -1 = forever.
//  Returns error from BMSG_LINK_ call.
// ---------------------------------------------------------------------------
short TM_Info::link(SB_Phandle_Type    *pp_phandle,
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
                    int32               pv_maxretries,
                    TM_Transid          *pp_transid)

{
    short lv_ret = 0;
    int32 lv_retries = 0;
    bool lv_exit = false;
    TM_Transid lv_transid = *pp_transid;

    TMTrace(2, ("TM_Info::link ENTRY : ID (%d,%d), linker tag %ld, linkopts %d.\n", 
            lv_transid.get_node(), lv_transid.get_seq_num(), pv_linkertag, pv_linkopts));

    do {
       lv_ret = BMSG_LINK_     (pp_phandle,            
                                pp_msgid,            
                                NULL,                        // reqctrl
                                0,                           // reqctrlsize
                                NULL,                        // replyctrl
                                0,                           // replyctrlmax
                                pp_reqdata,        
                                pv_reqdatasize,  
                                pp_replydata,   
                                pv_replydatamax,  
                                pv_linkertag,  
                                pv_pri,    
                                0,                           // xmitclass
                                pv_linkopts);  

       lv_retries++;
       if (lv_ret == FENOLCB && 
          //((pv_maxretries == -1 && (lv_retries % TM_LINKRETRY_RETRIES == 0)) ||
          (pv_maxretries == -1 ||
           (pv_maxretries > 0 && (lv_retries <= pv_maxretries))))
       {  // Message Descriptor depletion.  This means we ran out of MDs.
          // This is retriable, and we want to slow down the TM to allow
          // some of the outstanding requests to complete.
          TMTrace(1, ("TM_Info::link BMSG_LINK_ error %d, "
                       "linker tag %ld, retires %d/%d - Pausing thread for %dms before retrying.\n",
                       lv_ret, pv_linkertag, 
                       lv_retries, pv_maxretries,
                       TM_LINKRETRY_PAUSE));
          tm_log_event(DTM_TM_LINK_PAUSED, SQ_LOG_WARNING, "DTM_TM_LINK_PAUSED", 
                   lv_ret, -1, lv_transid.get_node(), lv_transid.get_seq_num(), -1, -1, -1, -1, lv_retries, 
                   -1, -1, -1, -1, TM_LINKRETRY_PAUSE /*pause in ms*/, pv_linkertag);
          SB_Thread::Sthr::sleep(TM_LINKRETRY_PAUSE); // in msec
       }
       if (lv_ret != FENOLCB)
          lv_exit = true;
       else
          if (pv_maxretries > 0 && lv_retries >= pv_maxretries)
             lv_exit = true;

    } while (!lv_exit);

    if (lv_ret)
    {
       TMTrace(2, ("TM_Info::link EXIT : returning error %d.\n", lv_ret));
    }
    else
    {
       TMTrace(2, ("TM_Info::link EXIT : returning msgid %d.\n", *pp_msgid));
    }
    return lv_ret;
} //TM_Info::link


// ---------------------------------------------------------------------------
// TM_Info::all_tms_recovered
// Purpose : Determine whether all of the TMs have been recovered.  They don't
// need to be up, but we must have completed recovery.
// ---------------------------------------------------------------------------
bool TM_Info::all_tms_recovered()
{
    bool lv_ret = true;
    lock();
    // Check to see if any TMs are still recovering or down and haven't started 
    // recovery yet.  We exclude ourselves.
    for (int i=0;i<iv_tms_highest_index_used; i++)
        if (i != iv_nid && 
            ((iv_open_tms[i].iv_recov_state == TM_FAIL_RECOV_STATE_RUNNING) ||
             (iv_open_tms[i].iv_in_use == false && 
              iv_open_tms[i].iv_recov_state == TM_FAIL_RECOV_STATE_INITIAL &&
              iv_recovery[i].iv_node_being_recovered != -1)))
        {
            lv_ret = false;
            break;
        }
    unlock();
    return lv_ret;
} //TM_Info::all_tms_recovered


// ----------------------------------------------------------------------------
// TM_Info::drainTrans
// Purpose : Executes an draintransaction command within any TM.
// Returns the error ??
// drainTrans is called by both lead and non-lead TMs.
// ----------------------------------------------------------------------------
int32 TM_Info::drainTrans(CTmTxMessage * pp_msg)
{
    int32 lv_error = FEOK;
    TMTrace (2, ("TM_Info::drainTrans : ENTRY, immediate %d.\n", 
             pp_msg->request()->u.iv_draintrans.iv_immediate));

    if (state() == TM_STATE_DOWN|| state() == TM_STATE_WAITING_RM_OPEN || 
        state() == TM_STATE_SHUTTING_DOWN || state() == TM_STATE_SHUTDOWN_FAILED || 
        state() == TM_STATE_SHUTDOWN_COMPLETED || state() == TM_STATE_QUIESCE)
    {
        lv_error = FEBADSTATE;
        tm_log_event(DTM_TM_DRAINTRANS_TOOLATE, SQ_LOG_WARNING, "DTM_TM_DRAINTRANS_TOOLATE", 
            lv_error, -1, -1, nid(), -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, state());
        TMTrace (1, ("TM_Info::drainTrans: Error %d - Too late for drainTrans - TM state = %d.\n", 
            lv_error, state()));
    }
    else
    {
       state(TM_STATE_DRAIN);
       if (pp_msg->request()->u.iv_draintrans.iv_immediate)
          abort_all_active_txns();
    }

    pp_msg->reply(lv_error);

    TMTrace(2, ("TM_Info::drainTrans : EXIT, error %d.\n", lv_error));
    return lv_error;
} //TM_Info::drainTrans


// ----------------------------------------------------------------------------
// TM_Info::set_sys_recov_status
// Purpose : Set the sys_recov_status.  This is used to define the period of
// System Recovery in the Lead TM at startup.
// ----------------------------------------------------------------------------
void TM_Info::set_sys_recov_status(int32 pv_sys_recov_state, int32 pv_sys_recov_lead_tm_nid)
{
   if (iv_trace_level >= 2)
      trace_printf("TM_Info::set_sys_recov_status: ENTRY, recovery state %d, lead tm %d,  TM state is %d\n", 
                   pv_sys_recov_state, pv_sys_recov_lead_tm_nid, iv_state);

   lock();
   iv_sys_recov_state = pv_sys_recov_state;
   iv_sys_recov_lead_tm_nid = pv_sys_recov_lead_tm_nid;
   unlock();
   
   if (iv_sys_recov_state == TM_SYS_RECOV_STATE_END && iv_state == TM_UP)
      wake_TMUP_waiters(FEOK);
   if (pv_sys_recov_state == TM_SYS_RECOV_STATE_END)
      tm_log_event(DTM_RECOV_SYSRECOV_COMPLETE, SQ_LOG_NOTICE, "DTM_RECOV_SYSRECOV_COMPLETE", 
                   -1,-1,gv_tm_info.nid(),-1,-1,-1,-1,-1,-1,-1,-1,-1,iv_state); //Uses Tx state for TM State!

   if (iv_trace_level >= 2)
       trace_printf("TM_Info::set_sys_recov_status: EXIT\n");
} //TM_Info::set_sys_recov_status


// ---------------------------------------------------------------------------
// TM_Info::txStatetoa
// Purpose : Returns a string describing the txn state
// ---------------------------------------------------------------------------
char * txStatetoa(int32 pv_state)
{
   const int lc_maxStateLength = 48;
   static char lv_txState[lc_maxStateLength];
   char * lp_txState = (char *) &lv_txState;
   memset(lp_txState, 0, lc_maxStateLength);

   switch (pv_state)
   {
   case TM_TX_STATE_ACTIVE:
      strcpy(lp_txState,"ACTIVE");
      break;
   case TM_TX_STATE_FORGOTTEN:
      strcpy(lp_txState,"FORGOTTEN");
      break;
   case TM_TX_STATE_COMMITTED:
      strcpy(lp_txState,"COMMITTED");
      break;
   case TM_TX_STATE_ABORTING:
      strcpy(lp_txState,"ABORTING");
      break;
   case TM_TX_STATE_ABORTING_PART2:
      strcpy(lp_txState,"ABORTING PT2");
      break;
   case TM_TX_STATE_ABORTED:
      strcpy(lp_txState,"ABORTED");
      break;
   case TM_TX_STATE_HUNGABORTED:
      strcpy(lp_txState,"HUNGABORTED");
      break;
   case TM_TX_STATE_HUNGCOMMITTED:
      strcpy(lp_txState,"HUNGCOMMITTED");
      break;
   case TM_TX_STATE_COMMITTING:
      strcpy(lp_txState,"COMMITTING");
      break;
   case TM_TX_STATE_PREPARING:
      strcpy(lp_txState,"PREPARING");
      break;
   case TM_TX_STATE_FORGETTING:
      strcpy(lp_txState,"FORGETTING");
      break;
   case TM_TX_STATE_FORGOTTEN_HEUR:
      strcpy(lp_txState,"FORGOTTEN_HEUR");
      break;
   case TM_TX_STATE_FORGETTING_HEUR:
      strcpy(lp_txState,"FORGETTING_HEUR");
      break;
   case TM_TX_STATE_BEGINNING:
      strcpy(lp_txState,"BEGINNING");
      break;
   case TM_TX_STATE_NOTX:
      strcpy(lp_txState,"INITIALIZE");
      break;
   default:
      sprintf(lp_txState,"(Unknown Txn state %d)", pv_state);
      break;
   } //switch

   return lp_txState;
   // TM_Info::txStatetoa
}

// ---------------------------------------------------------------------------
// TM_Info::tmStatetoa
// Purpose : Returns a string describing the TM state
// ---------------------------------------------------------------------------
char * TM_Info::tmStatetoa(int32 pv_state)
{
   const int lc_maxStateLength = 48;
   static char lv_tmState[lc_maxStateLength];
   char * lp_tmState = (char *) &lv_tmState;
   memset(lp_tmState, 0, lc_maxStateLength);

   switch (pv_state)
   {
   case TM_STATE_INITIAL:
      strcpy(lp_tmState,"INITIAL");
      break;
   case TM_STATE_UP:
      strcpy(lp_tmState,"UP");
      break;
   case TM_STATE_DOWN:
      strcpy(lp_tmState,"DOWN");
      break;
   case TM_STATE_SHUTTING_DOWN:
      strcpy(lp_tmState,"SHUTTING DOWN");
      break;
   case TM_STATE_SHUTDOWN_FAILED:
      strcpy(lp_tmState,"SHUTDOWN FAILED");
      break;
   case TM_STATE_SHUTDOWN_COMPLETED:
      strcpy(lp_tmState,"SHUTDOWN COMPLETE");
      break;
   case TM_STATE_TX_DISABLED:
      strcpy(lp_tmState,"TXNS DISABLED");
      break;
   case TM_STATE_TX_DISABLED_SHUTDOWN_PHASE1:
      strcpy(lp_tmState,"TXNS DISABLED, SHUTDOWN PHASE 1");
      break;
   case TM_STATE_QUIESCE:
      strcpy(lp_tmState,"QUIESCING");
      break;
   case TM_STATE_DRAIN:
      strcpy(lp_tmState,"DRAINING");
      break;
   case TM_STATE_WAITING_RM_OPEN:
      strcpy(lp_tmState,"WAITING FOR RM OPENS TO COMPLETE");
      break;
   default:
      sprintf(lp_tmState,"(Unknown TM state %d)", pv_state);
      break;
   } //switch

   return lp_tmState;
} // TM_Info::tmStatetoa

// ---------------------------------------------------------------------------
// TM_Info::dummy_link_to_refresh_phandle
// Purpose : On NodeUp or TmRestarted, the new TM phandle will be stale
// This procedure will force a refresh in seabed and is ONLY called by the Lead DTM!
// ---------------------------------------------------------------------------
void TM_Info::dummy_link_to_refresh_phandle(int32 pv_nid)
{
    short                      la_results[6];
    Tm_Req_Msg_Type     *lp_req = NULL;
    Tm_Rsp_Msg_Type     *lp_rsp = NULL;
    int32                      lv_error = FEOK;
    int32                      lv_index = 0;
    int32                      lv_num_sent = 0;
    pid_msgid_struct           lv_pid_msgid;
    int32                      lv_reqLen = 0;
    long                       lv_ret;
    long                       lv_ret2;
    int32                      lv_rspLen = 0;
    int                        lv_rsp_rcvd = 0;
    BMS_SRE_LDONE              lv_sre;

    TMTrace (2, ("TM_Info::dummy_link_to_refresh_phandle: ENTRY\n"));

    //initialize lv_pid_msgid
    lv_pid_msgid.iv_tag = 0;
    lv_pid_msgid.iv_msgid = 0;
    lv_pid_msgid.iv_nid = 0;

    TMTrace (3, ("TM_Info::dummy_link_to_refresh_phandle sending Leadtm request to TM%d.\n",pv_nid));

     lp_req = new Tm_Req_Msg_Type;
     lp_rsp = new Tm_Rsp_Msg_Type;  

//Send messaget to tm
    lv_pid_msgid.iv_tag = 1; // non zero
    lp_req->iv_msg_hdr.dialect_type = DIALECT_TM_SQ;
    lp_req->iv_msg_hdr.rr_type.request_type = TM_MSG_TYPE_LEADTM;
    lp_req->iv_msg_hdr.version.request_version = TM_SQ_MSG_VERSION_CURRENT;
    lv_pid_msgid.iv_nid = pv_nid;
    
    lv_reqLen = sizeof (Tm_Req_Msg_Type);
    lv_rspLen = sizeof (Tm_Rsp_Msg_Type);
    
    lv_error = link(&(iv_open_tms[pv_nid].iv_phandle),     // phandle,
                   &lv_pid_msgid.iv_msgid,        // msgid
                   (char *) lp_req,    // reqdata
                   lv_reqLen,                   // reqdatasize
                   (char *) lp_rsp,    // replydata
                   lv_rspLen,                   // replydatamax
                   lv_pid_msgid.iv_tag, // linkertag
                   TM_TM_LINK_PRIORITY,         // pri
                   BMSG_LINK_LDONEQ,            // linkopts
                   TM_LINKRETRY_RETRIES);       // retry count
    
    if (lv_error != 0)
    {
       TMTrace (1, ("TM_Info::dummy_link_to_refresh_phandle BMSG_LINK_ failed with error %d. failure ignored.\n",lv_error));
    }
    else
       lv_num_sent++;
// for one tm

      // LDONE LOOP
    while (lv_rsp_rcvd < lv_num_sent)
    {

      // wait for an LDONE wakeup 
      XWAIT(LDONE, -1);

      do {
           lv_error = 0;

           // we've reached our message reply count, break
           if (lv_rsp_rcvd >= lv_num_sent)
               break;

           lv_ret = BMSG_LISTEN_((short *)&lv_sre, 
                                  BLISTEN_ALLOW_LDONEM, 0);

           if (lv_ret == BSRETYPE_LDONE)
           {
              lv_index = -1;
              if (lv_pid_msgid.iv_tag == lv_sre.sre_linkTag)
              {
                 lv_index = pv_nid;
              }
            
              if (lv_index == -1)
              {
                  TMTrace (1, ("TM_Info::dummy_link_to_refresh_phandle - Link Tag %d not found\n", (int)lv_sre.sre_linkTag));
                  lv_error = FEDEVDOWN;
              }

              if (!lv_error)
              {
                  lv_ret2 = BMSG_BREAK_(lv_pid_msgid.iv_msgid, 
                                       la_results,
                                       &(iv_open_tms[pv_nid].iv_phandle)); 
                  if (lv_ret2 != 0)
                  {
                      TMTrace (1, ("TM_Info::dummy_link_to_refresh_phandle ERROR BMSG_BREAK_ returned %ld, index %d, msgid %d.\n",
                              lv_ret2, lv_index, lv_pid_msgid.iv_msgid));
                      lv_error = FEDEVDOWN;
                  }
              }
        
              if (lv_error == FEDEVDOWN)
              {
                 TMTrace (1, ("TM_Info::dummy_link_to_refresh_phandle - TM respond error\n"));
              }
             lv_rsp_rcvd++;
             }
        } while (lv_ret == BSRETYPE_LDONE); 
     }// while (lv_rsp_rcvd < lv_num_sent)

    TMTrace (2, ("TM_Info::dummy_link_to_refresh_phandle: EXIT\n"));
}

