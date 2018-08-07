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

#include <algorithm>
#include <iostream>
#include <string>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <curses.h>
#include <term.h>
#include <time.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/time.h>
#include <vector>
#include <sstream>
#include <iomanip>

#include "dtm/tm.h"
#include "../tmlibmsg.h"
#include "seabed/ms.h"

// General Seaquest includes
#include "SCMVersHelp.h"


// Version
DEFINE_EXTERN_COMP_DOVERS(dtmci)
    
const char ga_timestamp[] = "v 3.1.0, Nov 26, 2014";

using namespace std;
extern const char *ms_getenv_str(const char *pp_key);

enum {
    INPUT_LENGTH = 1024,
    TRANSID_LEN  = 32
};

char * remove_white_space (char *pp_cmd)
{
    if(!pp_cmd) {
        return NULL;
    }
    char *lp_ptr = pp_cmd;

    while (lp_ptr && *lp_ptr && (*lp_ptr == ' ' || *lp_ptr == '\t'))
    {
        lp_ptr++;
    }

    int length = strlen(lp_ptr) - 1;
    char test = lp_ptr[length];
    while ((test == ' ' || test == '\t') && (length > 0))
    {
        length--;
        test = lp_ptr[length];
    }
    lp_ptr[length+1] = '\0';

    return lp_ptr;
}

void get_cmd(char*& pp_input, char pa_output[]) 
{
    // Empty string case
    if(strlen(pp_input) == 0) {
        pa_output[0] = '\0';
    }
    else {
        char * lp_remaining;
        pp_input = remove_white_space(pp_input);
    
        // Find where the space is, if no space then strcpy
        lp_remaining = strchr(pp_input, ' ');
        if(lp_remaining == NULL) {
            strncpy(pa_output, pp_input, INPUT_LENGTH);
            pa_output[INPUT_LENGTH-1] = '\0';
            strcpy(pp_input,"");
        }
        else {
            int lv_wordlen = lp_remaining-pp_input;
            strncpy(pa_output, pp_input, lv_wordlen);
            pa_output[lv_wordlen] = '\0';
            pp_input += lv_wordlen;
            pp_input = remove_white_space(pp_input);
        }
    }
}

char *printtimeformatted(struct timeval pp_time) {
    char *la_timereturn = new char[20];
    int lv_microsec = pp_time.tv_usec / 1000;
    unsigned int lv_hours, lv_minutes, lv_seconds, lv_timesec;
    lv_timesec = pp_time.tv_sec; 
 
    //hours
    lv_hours = (lv_timesec / 3600);
    //minutes
    lv_minutes = ((lv_timesec % 3600) / 60);
    //seconds
    lv_seconds = (( lv_timesec % 3600) % 60);
    
    
    sprintf(la_timereturn, "%d:%02d:%02d.%02d", lv_hours, lv_minutes, lv_seconds, lv_microsec);
    return la_timereturn;
}

void get_time_difference(struct timeval *pp_result, struct timeval *pp_firsttime, struct timeval *pp_secondtime) {
    long int lv_difference = (pp_secondtime->tv_usec + 1000000 * pp_secondtime->tv_sec) - 
                             (pp_firsttime->tv_usec + 1000000 * pp_firsttime->tv_sec); 
    pp_result->tv_sec  = lv_difference / 1000000;
    pp_result->tv_usec = lv_difference % 1000000;
}

void print_transid_str(int32 pv_node, int32 pv_seqnum) {
         char la_transid_str[TRANSID_LEN];
         sprintf(la_transid_str,"(%d,%d)", pv_node, pv_seqnum);
         la_transid_str[TRANSID_LEN-1] = '\0';
         printf("%-16s", la_transid_str);
}

long now()
{  
   timeval lv_now;

   int lv_success = gettimeofday(&lv_now, NULL);
   if (lv_success != 0)
   {
      printf("\n** gettimeofday returned error %d.", lv_success);
      abort();
   }
   return lv_now.tv_sec;
}

char *booltoa(bool pv_bool)
{
    static char la_text[10];
    char *lp_text = (char *) &la_text;
    memset(lp_text, 0, sizeof(la_text));
    if (pv_bool)
        strcpy(lp_text, "true");
    else
        strcpy(lp_text, "false");
    return lp_text;
} //booltoa

char *tmstatetoa(int32 pv_state)
{
    static char la_text[10];
    char *lp_text = (char *) &la_text;
    memset(lp_text, 0, sizeof(la_text));
    switch (pv_state)
    {
    case TM_STATE_INITIAL:     
        strcpy(lp_text, "INITIAL");
        break;
    case TM_STATE_UP:
        strcpy(lp_text, "UP");
        break;
    case TM_STATE_DOWN:
        strcpy(lp_text, "DOWN");
        break;
    case TM_STATE_SHUTTING_DOWN:
        strcpy(lp_text, "SHUTD_S");
        break;
    case TM_STATE_SHUTDOWN_FAILED:
        strcpy(lp_text, "SHUTD_F");
        break;
    case TM_STATE_SHUTDOWN_COMPLETED:
        strcpy(lp_text, "SHUTD_C");
        break;
    case TM_STATE_TX_DISABLED:
        strcpy(lp_text, "TX_DISA");
        break;
    case TM_STATE_TX_DISABLED_SHUTDOWN_PHASE1:
        strcpy(lp_text, "TX_DISS");
        break;
    case TM_STATE_QUIESCE:
        strcpy(lp_text, "QUIESCE");
        break;
    case TM_STATE_DRAIN:
        strcpy(lp_text, "DRAIN");
        break;
    case TM_STATE_WAITING_RM_OPEN:
        strcpy(lp_text, "WAIT_RM");
        break;
    case TM_STATE_NOTRUNNING:
        strcpy(lp_text, "NOT_RUNNING");
        break;
    default:
        sprintf(lp_text, "%d", pv_state);
        break;
    }
    return lp_text;
} //tmstatetoa


char *tmsysrecovstatetoa(int32 pv_state)
{
    static char la_text[10];
    char *lp_text = (char *) &la_text;
    memset(lp_text, 0, sizeof(la_text));
    switch (pv_state)
    {
    case TM_SYS_RECOV_STATE_INIT:
        strcpy(lp_text, "INIT");
        break;
    case TM_SYS_RECOV_STATE_START:
        strcpy(lp_text, "START");
        break;
    case TM_SYS_RECOV_STATE_END:
        strcpy(lp_text, "END");
        break;
    default:
        sprintf(lp_text, "%d", pv_state);
        break;
    }
    return lp_text;
} //tmsysrecovstatetoa


char *tmshutdownleveltoa(int32 pv_state)
{
    static char la_text[10];
    char *lp_text = (char *) &la_text;
    memset(lp_text, 0, sizeof(la_text));
    switch (pv_state)
    {
    case MS_Mon_ShutdownLevel_Undefined:
        strcpy(lp_text, "RUNNING");
        break;
    case MS_Mon_ShutdownLevel_Normal:
        strcpy(lp_text, "NORMAL");
        break;
    case MS_Mon_ShutdownLevel_Immediate:
        strcpy(lp_text, "IMMEDIATE");
        break;
    case MS_Mon_ShutdownLevel_Abrupt:
        strcpy(lp_text, "ABRUPT");
        break;
    default:
        sprintf(lp_text, "%d", pv_state);
        break;
    }
    return lp_text;
} //tmshutdownleveltoa


char *rmstatetoa(int32 pv_state)
{
    static char la_text[10];
    char *lp_text = (char *) &la_text;
    memset(lp_text, 0, sizeof(la_text));
    switch (pv_state)
    {
    case TSEBranch_UP:
        strcpy(lp_text, "UP");
        break;
    case TSEBranch_DOWN:
        strcpy(lp_text, "DOWN");
        break;
    case TSEBranch_FAILED:
        strcpy(lp_text, "FAILED");
        break;
    case TSEBranch_RECOVERING:
        strcpy(lp_text, "RECOVER");
        break;
    case TSEBranch_FAILOVER:
        strcpy(lp_text, "FAILOVER");
        break;
    default:
        sprintf(lp_text, "%d", pv_state);
        break;
    }
    return lp_text;
} //rmstatetoa

char * rmidtoa(int32 pv_rmid)
{
    RMID lv_rmid;
    static char la_text[10];
    char *lp_text = (char *) &la_text;
    memset(lp_text, 0, sizeof(la_text));

    lv_rmid.iv_rmid = pv_rmid;
    sprintf(lp_text, "(%d,%d)", lv_rmid.s.iv_nid, lv_rmid.s.iv_num);
    return lp_text;
} //rmidtoa

void print_timedStats(TIMEDSTATS pv_stats)
{
   double lv_mean = 0;
   double lv_stddevSq = 0;
   double lv_stddev = 0;

   if (pv_stats.iv_count > 0)
   {
      lv_mean = pv_stats.iv_total / pv_stats.iv_count;
      lv_stddevSq = (pv_stats.iv_totalSq - (pow(pv_stats.iv_total, 2) / pv_stats.iv_count)) / pv_stats.iv_count;
      if (lv_stddevSq < 0)
         lv_stddevSq = -lv_stddevSq;
      lv_stddev = sqrt(lv_stddevSq);
   }
   printf("%d(%0.3f+-%0.3fs)", pv_stats.iv_count, lv_mean, lv_stddev);
} //print_timedStats


void print_txnStats(TXNSTATS *pp_stats)
{
   printf("\n  Txns\tTotal ");
   print_timedStats(pp_stats->iv_txnTotal);
   printf("\tBegins ");
   print_timedStats(pp_stats->iv_txnBegin);
   printf("\n\tAborts ");
   print_timedStats(pp_stats->iv_txnAbort);
   printf("\tCommits ");
   print_timedStats(pp_stats->iv_txnCommit);

   printf("\n  XA\tstart ");
   print_timedStats(pp_stats->iv_xa_start);
   printf("\tend ");
   print_timedStats(pp_stats->iv_xa_end);

   printf("\n\tprepare ");
   print_timedStats(pp_stats->iv_xa_prepare);
   printf("\tax_reg ");
   print_timedStats(pp_stats->iv_ax_reg);

   printf("\n\tcommit ");
   print_timedStats(pp_stats->iv_xa_commit);
   printf("\trollback ");
   print_timedStats(pp_stats->iv_xa_rollback);

   printf("\n  RM\tSends ");
   print_timedStats(pp_stats->iv_RMSend);
   printf("\tPartic %d\t Non-partic %d",
      pp_stats->iv_RMParticCount,
      pp_stats->iv_RMNonParticCount);
} //print_txnStats


void print_counters(TMCOUNTS *pp_counts)
{
   printf("\n  Counts: Total = %ld, Current = %d, Begins = %ld, Aborts = %ld, Commits = %ld, "
      "TM Aborts = %ld, Hung total = %d, current = %d", pp_counts->iv_tx_count,
      pp_counts->iv_current_tx_count, pp_counts->iv_begin_count, pp_counts->iv_abort_count,
      pp_counts->iv_commit_count, pp_counts->iv_tm_initiated_aborts,
      pp_counts->iv_tx_hung_count, pp_counts->iv_current_tx_hung_count);
} //print_counters


void print_poolStats(TMPOOLSTATS pv_pool)
{
   int32 lv_secs;
   int32 lv_Allocs_new_persec;
   int32 lv_Allocs_free_persec;
   int32 lv_Deallocs_free_persec;
   int32 lv_Deallocs_delete_persec;
   long  lv_now = now();

   printf("Inuse %d\tFree %d\tSSLow %d\tSSHigh %d\tMax %d",
      pv_pool.iv_inUseListNow,
      pv_pool.iv_freeListNow,
      pv_pool.iv_steadyStateLow,
      pv_pool.iv_steadyStateHigh,
      pv_pool.iv_max
      );

   lv_secs = lv_now - (int32) pv_pool.iv_lastTimeInterval;
   if (lv_secs == 0)
      lv_secs = 1; //Crude, but avoids divide by 0 for now
   lv_Allocs_new_persec = pv_pool.iv_totalAllocs_new / lv_secs;
   lv_Allocs_free_persec = pv_pool.iv_totalAllocs_free / lv_secs;
   lv_Deallocs_free_persec = pv_pool.iv_totalDeallocs_free / lv_secs;
   lv_Deallocs_delete_persec = pv_pool.iv_totalDeallocs_delete  / lv_secs;
   printf("\n   Allocs:\tnew %d(%d/sec)\tfrees %d(%d/sec)\n   Deallocs:\tfrees %d(%d/sec)\tdeletes %d(%d/sec)",
      pv_pool.iv_totalAllocs_new,
      lv_Allocs_new_persec,
      pv_pool.iv_totalAllocs_free,
      lv_Allocs_free_persec,
      pv_pool.iv_totalDeallocs_free,
      lv_Deallocs_free_persec,
      pv_pool.iv_totalDeallocs_delete,
      lv_Deallocs_delete_persec
      );
} //print_poolStats

void print_txnstatus(int32 pv_status)
{
    switch (pv_status)
    {
        case TM_TX_STATE_ACTIVE:
            printf("ACTIVE");             
            break;
        case TM_TX_STATE_FORGOTTEN:
            printf("FORGOTTEN");             
            break;
        case TM_TX_STATE_COMMITTED:
            printf("COMMITTED");             
            break;
        case TM_TX_STATE_ABORTING:
            printf("ABORTING");             
            break;
        case TM_TX_STATE_ABORTING_PART2:
            printf("ABORTING PT2");             
            break;
        case TM_TX_STATE_ABORTED:
            printf("ABORTED");             
            break;
        case TM_TX_STATE_HUNGABORTED:
            printf("HUNGABORTED");             
            break;
        case TM_TX_STATE_HUNGCOMMITTED:
        printf("HUNGCOMMITTED");
        break;
        case TM_TX_STATE_COMMITTING:
            printf("COMMITTING");             
            break;
        case TM_TX_STATE_PREPARING:
            printf("PREPARING");             
            break;
        case TM_TX_STATE_FORGETTING:
            printf("FORGETTING");             
            break;
        case TM_TX_STATE_FORGOTTEN_HEUR:
            printf("FORGOTTEN_HEUR");             
            break;
        case TM_TX_STATE_FORGETTING_HEUR:
            printf("FORGETTING_HEUR");             
            break;
        case TM_TX_STATE_BEGINNING:
            printf("BEGINNING");             
            break;
        case TM_TX_STATE_NOTX:
            printf("INITIALIZE");             
            break;
        default:
        {
            printf("(%d)", pv_status);             
            break;
        }
    } //switch
} // print_txnstatus


void process_tmstats_node(bool pv_reset, int32 pv_nid, bool json)
{
    short lv_error = 0;
    TM_TMSTATS lv_stats;

    lv_error = TMSTATS(pv_nid, &lv_stats, pv_reset);

    if (lv_error) {
        printf("Node %d\t**Error %d.\n", pv_nid, lv_error);
    }

    else if(json==false) {
        printf("Node %d:", pv_nid);
        print_counters(&lv_stats.iv_counts);
        print_txnStats(&lv_stats.iv_txn);

        // Pool statistics
        printf("\n  Txn Pool:\t");
        print_poolStats(lv_stats.iv_transactionPool_stats);
        printf("\n  Thrd Pool:\t");
        print_poolStats(lv_stats.iv_threadPool_stats);
        printf("\n  RMMsg Pool:\t");
        print_poolStats(lv_stats.iv_RMMessagePool_stats);
        printf("\n");
    } 
    else {
        printf("{\"node\": %d", pv_nid);
        printf(",\"txnStats\":{\"txnBegins\": %ld", lv_stats.iv_counts.iv_begin_count);
        printf(",\"txnAborts\": %ld", lv_stats.iv_counts.iv_abort_count);
        printf(",\"txnCommits\": %ld}}", lv_stats.iv_counts.iv_commit_count);
        printf("\n");
    }
} //process_tmstats_node


void process_tmstats(bool pv_reset, int32 pv_node, bool json)
{
    int lv_dtm_count = 0;
    bool del = false;

    if(json==true)
        printf("[");

    if (pv_node != -1)
        process_tmstats_node(pv_reset, pv_node, json);
    else
    {
        msg_mon_get_node_info        ( &lv_dtm_count,
                                                  MAX_NODES,
                                                  NULL);
        
        for (int lv_inx = 0; lv_inx < lv_dtm_count; lv_inx++) {
            if(del==true)
                printf(",");
            process_tmstats_node(pv_reset, lv_inx, json);
            del=true;
        }
    }

    if(json==true)
        printf("]\n");
} //process_tmstats

bool sort_comparator(TM_STATUS_ALL_TRANS a, TM_STATUS_ALL_TRANS b)
{
   return (a.iv_timestamp < b.iv_timestamp);
} // sort_comparator

void process_statusalltransactions_node(int32 pv_node)
{
   short  lv_error = FEOK;
   short  lv_count = 0;
   time_t lv_trans_timestamp;
   time_t lv_elapsed_time;
   time_t lv_localtime;

   struct tm * lv_trans_timeinfo;

   TM_STATUS_ALL_TRANS lv_trans[TM_MAX_LIST_TRANS];

   lv_error = DTM_STATUSALLTRANS((TM_STATUS_ALL_TRANS *) &lv_trans, &lv_count, pv_node);

   if(lv_error)
     printf("**Error returned for node %d : %d\n", pv_node, lv_error);
   else
   {
     if (lv_count == 0)
       printf("Node %d : No Transactions were returned.\n", pv_node);
     else
       printf("Transid         Owner\tJoins\tPartic\tUnresol\tElapsed(sec)\tState\t   Timestamp\n");

     sort(lv_trans, lv_trans + lv_count, sort_comparator);
     for(int i = 0; i < lv_count; i++)
     {
       print_transid_str(lv_trans[i].iv_nid, lv_trans[i].iv_seqnum);

       printf("%d,%d\t%d\t%d\t%d",
       lv_trans[i].iv_owner_nid,
       lv_trans[i].iv_owner_pid,
       lv_trans[i].iv_num_active_partic-1,
       lv_trans[i].iv_num_partic_RMs,
       lv_trans[i].iv_num_unresolved_RMs);

       lv_localtime = time(NULL);
       lv_trans_timestamp = (time_t)(lv_trans[i].iv_timestamp/1000);
       lv_elapsed_time = lv_localtime - lv_trans_timestamp;
       lv_trans_timeinfo = localtime (&lv_trans_timestamp);

       printf("\t%ld\t\t", lv_elapsed_time);

       print_txnstatus(lv_trans[i].iv_status);

       if (lv_trans[i].iv_XARM_branch)
          printf(" XARM");
       if (lv_trans[i].iv_transactionBusy)
          printf(" txnBusy");
       if (lv_trans[i].iv_mark_for_rollback)
          printf(" willRollback");
       if (lv_trans[i].iv_tm_aborted)
          printf(" tmAborted");
       if (lv_trans[i].iv_read_only)
          printf(" readOnly");
       if (lv_trans[i].iv_recovering)
          printf(" recovering");

       printf("\t%s", asctime (lv_trans_timeinfo));
    }
  }
}//process_statusalltransactions_node

void process_statusalltransactions(int32 pv_node)
{
  int lv_dtm_count = 0;

  if(pv_node !=-1)
     cout << "Info specific node: " << pv_node << "\n";
  else
  {
     msg_mon_get_node_info(&lv_dtm_count,
                                      MAX_NODES,
                                      NULL);

     for(int lv_inx =0; lv_inx < lv_dtm_count; lv_inx++)
     {
        process_statusalltransactions_node(lv_inx);
     }
  }
}//process_statusalltransactions

void process_list_node(int32 pv_node)
{
    short lv_error = FEOK;
    short lv_count = 0;
    TM_LIST_TRANS lv_trans[TM_MAX_LIST_TRANS];
    
    lv_error = LISTTRANSACTION((TM_LIST_TRANS *) &lv_trans, &lv_count, pv_node);

    if (lv_error)
        printf("**Error returned for node %d : %d\n", pv_node, lv_error); 
    else
    {
        if (lv_count == 0)
        printf("Node %d : No Transactions were returned.\n", pv_node);
        else
        printf("Transid         Owner\teventQ\tpending\tJoiners\tTSEs\tState\n");
        for (int i = 0; i < lv_count; i++)
        {
        print_transid_str(lv_trans[i].iv_nid, lv_trans[i].iv_seqnum);

        printf("%d,%d\t%d\t%d\t%d\t%d\t",
                lv_trans[i].iv_owner_nid,
                lv_trans[i].iv_owner_pid,
                lv_trans[i].iv_event_count,
                lv_trans[i].iv_pendingRequest_count,
                lv_trans[i].iv_num_active_partic-1,
                lv_trans[i].iv_num_partic_RMs);
                  
        print_txnstatus(lv_trans[i].iv_status);

        if (lv_trans[i].iv_XARM_branch)
            printf(" XARM");
        if (lv_trans[i].iv_transactionBusy)
            printf(" txnBusy");
        if (lv_trans[i].iv_mark_for_rollback)
            printf(" willRollback");
        if (lv_trans[i].iv_tm_aborted)
            printf(" tmAborted");
        if (lv_trans[i].iv_read_only)
            printf(" readOnly");
        if (lv_trans[i].iv_recovering)
            printf(" recovering");
        printf("\n");
        } //else for
    } //else no error.
} //process_list_node


void process_list(int32 pv_node)
{
    int lv_dtm_count = 0;

    if (pv_node != -1)
        process_list_node(pv_node);
    else
    {
        msg_mon_get_node_info (&lv_dtm_count,
                                          MAX_NODES,
                                          NULL);

        for (int lv_inx = 0; lv_inx < lv_dtm_count; lv_inx++)
            process_list_node(lv_inx);
    }
} //process_list

bool process_attachrm_node(char* pp_rmname, int32 pv_node)
{
    int32 lv_error = DTM_ATTACHRM(pv_node, pp_rmname);
    if (lv_error != FEOK)
    {
       printf("%d\tTM process down. Returned error: %d\n", pv_node, lv_error);
       return false; // fail
    }
    else
    {
       printf("%d\tTM process successfully received attach rm %s request\n", pv_node, pp_rmname);
       return true; // success
    }
}

void process_attachrm(char* pp_rmname)
{
   short lv_error = 0;
   static int lv_max_dtm_count = 0;
   int lv_dtm_count = 0;

      lv_error = msg_mon_get_node_info        ( &lv_dtm_count,
                                                MAX_NODES,
                                                NULL);
      if (lv_error != FEOK)
          cout << "** msg_mon_get_process_info_type returned error " << lv_error << endl;
      else
      {
         // Remember the lagest number of TMs seen
         if (lv_max_dtm_count < lv_dtm_count)
             lv_max_dtm_count = lv_dtm_count;

         if (lv_dtm_count == 0)
             cout << "** No TMs running." << endl;
         else
         {
             for (int lv_inx = 0; lv_inx < lv_max_dtm_count; lv_inx++)
             {
                 printf("sending attach for %s to node %d\n", pp_rmname, lv_inx);
                 process_attachrm_node(pp_rmname, lv_inx);
             }
         }

      }

} //process_attachrm

bool process_statustm_node(int32 pv_node, bool pv_detail, bool pv_sortrmid, bool json)
{
    RM_INFO temp_rm;
    char lv_buffer[TM_MsgSize(Statustm_Rsp_Type) + (sizeof(RM_INFO) * MAX_OPEN_RMS)];
    memset(lv_buffer,0,TM_MsgSize(Statustm_Rsp_Type) + (sizeof(RM_INFO) * MAX_OPEN_RMS));
    TMSTATUS *lp_tmstatus = (TMSTATUS *) &lv_buffer;
    char *lp_string;

    int32 lv_error = DTM_STATUSTM(pv_node, lp_tmstatus);
    if (json==true)
    {
       if (lv_error != FEOK)
       {
          printf("{\"node\":%d", pv_node);
          printf(", \"isLeadTM\":false");
          printf(", \"state\":\"%s\"",tmstatetoa(TM_STATE_NOTRUNNING));
          printf(", \"sys_recovery_state\":\"NOT AVAILABLE\"");
          printf(", \"tmshutdown_level\":\"NOT AVAILABLE\"");
          printf(", \"number_active_txns\":0}");
          
          return false; // fail
       }

       printf("{\"node\":%d", lp_tmstatus->iv_node);
       printf(", \"isLeadTM\":%s", booltoa(lp_tmstatus->iv_isLeadTM));
       printf(", \"state\":\"%s\"",tmstatetoa(lp_tmstatus->iv_state));
       printf(", \"sys_recovery_state\":\"%s\"",tmsysrecovstatetoa(lp_tmstatus->iv_sys_recovery_state));
       lp_string = tmshutdownleveltoa(lp_tmstatus->iv_shutdown_level);
       printf(", \"tmshutdown_level\":\"%s\"", lp_string);
       printf(", \"number_active_txns\":%d}", lp_tmstatus->iv_number_active_txns);

       return true;
    }
    else
    {
       if (lv_error != FEOK)
       {
          printf("%d\tTM process down. Returned error: %d\n", pv_node, lv_error);
          return false; // fail
       }
       printf("%d\t%s\t%s\t",
              lp_tmstatus->iv_node,
              booltoa(lp_tmstatus->iv_isLeadTM),
              tmstatetoa(lp_tmstatus->iv_state));
       printf("%s\t",
              tmsysrecovstatetoa(lp_tmstatus->iv_sys_recovery_state));
       lp_string = tmshutdownleveltoa(lp_tmstatus->iv_shutdown_level);
       printf("%s\t", lp_string);
       if (strlen(lp_string) < 9)
          printf("\t");
       printf("%d\t%d\t%s\t%d\n",
              lp_tmstatus->iv_incarnation_num,
              lp_tmstatus->iv_number_active_txns,
              booltoa(lp_tmstatus->iv_is_isolated),
              lp_tmstatus->iv_rm_count);
       if (pv_detail)
       {
          bool done;
          int j;
          if(pv_sortrmid) {
              for (int i=1; i<lp_tmstatus->iv_rm_count; i++)
              {
                  temp_rm = lp_tmstatus->ia_rminfo[i];
                  j = i-1;
                  done = false;
                  do {
                      if(lp_tmstatus->ia_rminfo[j].iv_rmid >  temp_rm.iv_rmid) {
                         lp_tmstatus->ia_rminfo[j+1] = lp_tmstatus->ia_rminfo[j];
                         j = j-1;
                         if(j < 0) {
                             done = true;
                         }
                      }
                      else {
                          done = true;
                      }
                  } while (done == false);
                  lp_tmstatus->ia_rminfo[j+1] = temp_rm;
                  
              }

          }
          else {
              //sorting by name - insertion sort
              for (int i=1; i<lp_tmstatus->iv_rm_count; i++)
              {
                  temp_rm = lp_tmstatus->ia_rminfo[i];
                  j = i-1;
                  done = false;
                  do {
                      if(strcmp(lp_tmstatus->ia_rminfo[j].ia_name, temp_rm.ia_name) > 0) {
                         lp_tmstatus->ia_rminfo[j+1] = lp_tmstatus->ia_rminfo[j];
                         j = j-1;
                         if(j < 0) {
                             done = true;
                         }
                      }
                      else {
                          done = true;
                      }
                  } while (done == false);
                  lp_tmstatus->ia_rminfo[j+1] = temp_rm;
                  
              }
          }
              
          if(lp_tmstatus->iv_rm_count > 0) {
             printf("\n  RM            State\tinuse\trmid\tpartic?\tpid\n");
             for (int i=0; i<lp_tmstatus->iv_rm_count; i++)
             {
                 printf("  %s\t", lp_tmstatus->ia_rminfo[i].ia_name);
                 if (strlen(lp_tmstatus->ia_rminfo[i].ia_name) < 6)
                    printf("\t");
                 printf("%s\t%s\t%s\t%s\t(%d,%d)\n",
                        rmstatetoa(lp_tmstatus->ia_rminfo[i].iv_state),
                        booltoa(lp_tmstatus->ia_rminfo[i].iv_in_use),
                        rmidtoa(lp_tmstatus->ia_rminfo[i].iv_rmid),
                        booltoa(lp_tmstatus->ia_rminfo[i].iv_partic),
                        lp_tmstatus->ia_rminfo[i].iv_nid,
                        lp_tmstatus->ia_rminfo[i].iv_pid);
             }
          }
       }
       return true; // success
    }
} //process_statustm_node


void process_statustm(int32 pv_node, bool pv_sortrmid, bool json)
{
   short lv_error = 0;
   static int lv_max_dtm_count = 0;
   int lv_dtm_count = 0;
   bool jdel = false;

   if(json==true)
      printf("[");
   else {
      printf("Node\tLeadTM\tState\tSysRec\tShutdownLevel\tIncarn"
              "\tTxns\tIsolTM\tRMs\n");
   }

   if (pv_node != -1)
   {
       process_statustm_node(pv_node, true, pv_sortrmid, json);
   }
   else
   {
      lv_error = msg_mon_get_node_info        ( &lv_dtm_count,
                                                MAX_NODES,
                                                NULL);
      if (lv_error != FEOK)
          cout << "** msg_mon_get_process_info_type returned error " << lv_error << endl;
      else
      {
         // Remember the lagest number of TMs seen
         if (lv_max_dtm_count < lv_dtm_count)
             lv_max_dtm_count = lv_dtm_count;

         if (lv_dtm_count == 0)
             cout << "** No TMs running." << endl;
         else
         {
            for (int lv_inx = 0; lv_inx < lv_max_dtm_count; lv_inx++) {
               if ((jdel==true) && (json==true))
                  printf(",");
               process_statustm_node(lv_inx, false, false, json);
               jdel = true;    
            }
         }

      }
   }
   if(json==true)
       printf("]\n");

} //process_statustm

void process_statussystem()
{
   TM_STATUSSYS lv_system_info;

          
   //DTM_STATUSSYSTEM(&lv_system_info);
   short lv_error = DTM_STATUSSYSTEM(&lv_system_info);

   if (lv_error != FEOK)
      printf("** DTM_STATUSSYSTEM returned error %d.\n", lv_error);
   else
   {
      printf("Lead\tUp\tDown\tRecov\tTotal\tActive Txs\n");
      printf("%d\t%d\t%d\t%d\t%d\t%d\n",
         lv_system_info.iv_leadtm,
         lv_system_info.iv_up,
         lv_system_info.iv_down,
         lv_system_info.iv_recovering,
         lv_system_info.iv_totaltms,
         lv_system_info.iv_activetxns
         );
   }
} //process_statustransaction

void process_statustransaction(const char *transid)
{
   char la_transid_node[TRANSID_LEN];
   char la_transid_seqn[TRANSID_LEN];
   char *lp_deleteval;

   short lv_error = 0;
   TM_STATUS_TRANS lv_trans_info;

   typedef struct txid{
       int32 iv_seqnum;
       int32 iv_node;
   } txid;

   union {
       txid  iv_txid;
       int64 iv_int_txid;
   } u;
   
   //specify default for union
   u.iv_int_txid = 0;

   if (!strchr(transid, ',')) {
       lv_error = DTM_STATUSTRANSACTION(atoi(transid), &lv_trans_info);
   }
   else {
       strncpy(la_transid_node, transid, TRANSID_LEN);
       la_transid_node[TRANSID_LEN -1] = '\0';
       la_transid_node[strchr(transid, ',') - transid] = '\0';
       strncpy(la_transid_seqn, strstr(transid, ","), TRANSID_LEN);
       la_transid_seqn[TRANSID_LEN -1] = '\0';

       lp_deleteval = la_transid_seqn;
       ++lp_deleteval;
       u.iv_txid.iv_seqnum = atoi(lp_deleteval);
       u.iv_txid.iv_node    = atoi(la_transid_node);

       lv_error = DTM_STATUSTRANSACTION(u.iv_int_txid, &lv_trans_info);
   }

   if (lv_error == FENOTRANSID) {
       cout << "** Trans ID not found\n";
   }
   else if(lv_error == FEINVTRANSID) {
       cout << "** Invalid Trans ID\n";
   }
   else if(lv_error != FEOK) {
       cout << "** Unable to obtain transaction status\n";
   }
   else {
       union txid {
           TM_Txid_Internal iv_txid_internal;
           TM_Transid_Type  iv_transid_type;
       } u;
       u.iv_transid_type = lv_trans_info.iv_transid;
       time_t lv_timestamp = (time_t)(u.iv_txid_internal.iv_timestamp/1000);
       struct tm * lv_timeinfo;
       lv_timeinfo = localtime (&lv_timestamp);

       printf("Transid         Owner\teventQ\tpending\tJoiners\tTSEs\tTX Fl\tTT Fl\tROnly\tRecov\tState\n");
       
       print_transid_str(lv_trans_info.iv_nid, lv_trans_info.iv_seqnum);
       
       printf("%d,%d\t%d\t%d\t%d\t%d\t0x%x\t0x" PFLLX "\t",
           lv_trans_info.iv_owner_nid,
           lv_trans_info.iv_owner_pid,
           lv_trans_info.iv_event_count,
           lv_trans_info.iv_pendingRequest_count,
           lv_trans_info.iv_num_active_partic-1,
           lv_trans_info.iv_num_partic_RMs,
           lv_trans_info.iv_tx_flags,
           lv_trans_info.iv_tt_flags);
           fputs(lv_trans_info.iv_read_only ? "true" : "false",stdout);
           fputs("\t",stdout);
           fputs(lv_trans_info.iv_recovering ? "true" : "false",stdout);
           fputs("\t",stdout);
    
       print_txnstatus(lv_trans_info.iv_status);
       printf ("\n");
       printf("Timestamp: %s", asctime (lv_timeinfo));
   }

} //process_statustransaction


void process_gettransinfo(const char *transid, bool pv_string_cmd)
{
   char la_transid_node[TRANSID_LEN];
   char la_transid_seqn[TRANSID_LEN];
   char *lp_deleteval;
   short lv_error = 0;

   int32 *lp_seq_num = new int32();
   int32 *lp_node = new int32();
   int16 *lp_incarnation_num = new int16();
   int16 *lp_tx_flags = new int16();
   TM_TT_Flags *lp_tt_flags = new TM_TT_Flags();
   int16 *lp_version = new int16();
   int16 *lp_checksum = new int16();
   int64 *lp_timestamp = new int64();

   typedef struct txid{
       int32 iv_seqnum;
       int32 iv_node;
   } txid;

   union {
       txid  iv_txid;
       int64 iv_int_txid;
   } u;

   union {
       TM_TT_Flags iv_tt_flags;
       int64       iv_int_tt_flags;
   } u_flag;
   
   //specify default for union
   u.iv_int_txid = 0;

   if (!strchr(transid, ',')) {
       lv_error = DTM_GETTRANSINFO(atoi(transid), 
                                   lp_seq_num, 
                                   lp_node, 
                                   lp_incarnation_num, 
                                   lp_tx_flags,
                                   lp_tt_flags,
                                   lp_version, 
                                   lp_checksum,
                                   lp_timestamp);
   }
   else {
       strncpy(la_transid_node, transid, TRANSID_LEN - 1);
       la_transid_node[strchr(transid, ',') - transid] = '\0';
       strncpy(la_transid_seqn, strstr(transid, ","), TRANSID_LEN - 1);

       lp_deleteval = la_transid_seqn;
       ++lp_deleteval;
       u.iv_txid.iv_seqnum = atoi(lp_deleteval);
       u.iv_txid.iv_node    = atoi(la_transid_node);

       lv_error = DTM_GETTRANSINFO(u.iv_int_txid, 
                                   lp_seq_num, 
                                   lp_node, 
                                   lp_incarnation_num, 
                                   lp_tx_flags,
                                   lp_tt_flags,
                                   lp_version, 
                                   lp_checksum,
                                   lp_timestamp);
   }

   if (lv_error == FENOTRANSID) {
       cout << "** Trans ID not found\n";
   }
   else if(lv_error == FEINVTRANSID) {
       cout << "** Invalid Trans ID\n";
   }
   else if(lv_error != FEOK) {
       cout << "** Unable to obtain transaction status\n";
   }
   
   if(lv_error == FEOK) {
      if(pv_string_cmd) {
          //process string command output
          cout << "(" << *lp_node << "," << *lp_seq_num 
            << "," << *lp_incarnation_num << ")" << endl;
      }
      else {
         u_flag.iv_tt_flags = *lp_tt_flags; 
         //output regular transid command output
         printf("Transid         Node    Seq #    Incarn    TX Flags    TT Flags\n");
         print_transid_str(*lp_node, *lp_seq_num);
#if __WORDSIZE == 64
         printf("%-4d    %-5d    %-6d    0x%-6x    0x%-6lx\t\n", *lp_node, *lp_seq_num,
                *lp_incarnation_num, *lp_tx_flags, u_flag.iv_int_tt_flags);
         printf("Version    Checksum          Timestamp\n");
         printf("%-7d    %-8d    %15ld\n", *lp_version, *lp_checksum, *lp_timestamp);
#else
         printf("%-4d    %-5d   %-6d    0x%-6x    0x%-6llx\t\n", *lp_node, *lp_seq_num,
                *lp_incarnation_num, *lp_tx_flags, u_flag.iv_int_tt_flags);
         printf("Version    Checksum          Timestamp\n");
         printf("%-7d    %-8d    %15lld\n", *lp_version, *lp_checksum, *lp_timestamp);
#endif
      }
   }

   delete lp_seq_num;
   delete lp_node;
   delete lp_incarnation_num;
   delete lp_tx_flags;
   delete lp_tt_flags;
   delete lp_version;
   delete lp_checksum;
   delete lp_timestamp;

} //process_gettransinfo

//-----------------------------------------------------------------------------------------
// process_request_regions_info
//
// Purpose: Display transaction region information from hbase and hbase-trx client.
//          client.
//-----------------------------------------------------------------------------------------
void process_request_regions_info()
{
   short lv_error = FEOK;
   short lv_count = 0;
   TM_HBASEREGIONINFO lv_trans_reg[TM_MAX_LIST_TRANS];

   string regioninfo, regioninfo_full, regioninfo_tmp, tname, tname_tmp, tablename, regid, hname, pname, startkey, endkey, tstamp;

   lv_error = HBASETM_REQUESTREGIONINFO((TM_HBASEREGIONINFO *) &lv_trans_reg, &lv_count);

   if(lv_error!=FEOK)
      printf("Error Returned for HBASETM_REQUESTREGIONINFO: %d\n", lv_error);
   else
   {
      if(lv_count == 0)
          printf("No Regions Returned from HBASETM_REQUESTREGIONINFO\n");
      else
      {
          printf("\nTransid\t\tStatus\n");
          for(int i=0; i<lv_count; i++)
          {
             bool first_entry = true;
             printf("-----------------------------------------------------------------------------------------------------\n");

             print_transid_str(lv_trans_reg[i].iv_nid, lv_trans_reg[i].iv_seqnum);
             print_txnstatus(lv_trans_reg[i].iv_status);
             cout << "\n-----------";

             tname   = lv_trans_reg[i].iv_tablename;

             while(tname.length() > 0) {
                std::size_t position = tname.find("|");
                if(! (position == string::npos )) {
                   regioninfo_tmp = tname.substr(0, position);
                   tname = tname.substr(position + 1);
                }
                else {
                   regioninfo_tmp = tname;
                   tname = "";
                }
                tablename = regioninfo_tmp.substr(0, regioninfo_tmp.find(";"));
                regioninfo_tmp = regioninfo_tmp.substr(regioninfo_tmp.find(";") + 1);

                regid = regioninfo_tmp.substr(0, regioninfo_tmp.find(";"));
                regioninfo_tmp = regioninfo_tmp.substr(regioninfo_tmp.find(";") + 1);

                regioninfo_full = regioninfo_tmp.substr(0, regioninfo_tmp.find(";"));
                regioninfo_tmp = regioninfo_tmp.substr(regioninfo_tmp.find(";") + 1);

                tstamp = regioninfo_tmp.substr(0, regioninfo_tmp.find(";"));
                regioninfo_tmp = regioninfo_tmp.substr(regioninfo_tmp.find(";") + 1);

                hname = regioninfo_tmp.substr(0, regioninfo_tmp.find(";"));
                regioninfo_tmp = regioninfo_tmp.substr(regioninfo_tmp.find(";") + 1);

                pname = regioninfo_tmp.substr(0, regioninfo_tmp.find(";"));
                regioninfo_tmp = regioninfo_tmp.substr(regioninfo_tmp.find(";") + 1);

                if(regioninfo_tmp.find(";") == string::npos)
                    continue;

                startkey = regioninfo_tmp.substr(0, regioninfo_tmp.find(";"));
                regioninfo_tmp = regioninfo_tmp.substr(regioninfo_tmp.find(";") + 1);
                
                endkey = regioninfo_tmp.substr(0, regioninfo_tmp.find(";"));

                if(!first_entry) 
                    cout << "-----------";
                first_entry = false;

                cout << endl;
                cout << "RegionInfo: \t" << regioninfo_full << endl;
                cout << "Hostname: \t" << hname << ":" <<pname << endl;
                cout << "Start/End Key: \t" << setw(40) << left << startkey <<  setw(40) << left << endkey << endl;
             }
             printf("\n");
          }
      }
   }
}// process_request_regions_info

void process_disableTrans(int32 pv_type)
{
    short lv_error = DTM_DISABLETRANSACTIONS(pv_type);
    if (lv_error != FEOK)
        cout << "** DTM_DISABLETRANSACITONS(" << pv_type << ") returned error " 
             << lv_error << endl;
} //process_disableTrans


void process_drainTrans(int32 pv_node, bool pv_immediate)
{
    short lv_error = DTM_DRAINTRANSACTIONS(pv_node, pv_immediate);
    if (lv_error != FEOK)
        cout << "** DTM_DRAINTRANSACITONS(" << pv_node << ", " << pv_immediate  
             << ") returned error " << lv_error << endl;
} //process_drainTrans


void process_enableTrans()
{
    short lv_error = DTM_ENABLETRANSACTIONS();
    if (lv_error != FEOK) {
        cout << "** DTM_ENABLETRANSACITONS returned error " << lv_error;
        if (lv_error == 2) {
            cout << ", FEINVALOP";
        }
        cout << endl;
    }
} //process_enableTrans


void process_quiesce(int32 pv_node)
{
    short lv_error = DTM_QUIESCE(pv_node);
    if (lv_error != FEOK)
        cout << "** DTM_QUIESCE(" << pv_node   
             << ") returned error " << lv_error << endl;
} //process_quiesce


void process_unquiesce(int32 pv_node)
{
    short lv_error = DTM_UNQUIESCE(pv_node);
    if (lv_error != FEOK)
        cout << "** DTM_UNQUIESCE(" << pv_node   
             << ") returned error " << lv_error << endl;
} //process_unquiesce

FILE * openInput(char * pp_inFile)
{
    FILE *lp_in = fopen(pp_inFile, "r");
    if (!lp_in)
    {
        cout << "**Error opening input file " << pp_inFile << endl;
        exit(-1);
    }    
    return lp_in;
}


void print_helptext()
{
   cout <<         "DTM Command line interface. " << ga_timestamp << " Help:";
   cout << endl << " a | abort ";
   cout << endl << "        : Will abort the current transaction.";
   cout << endl << " b | begin "; 
   cout << endl << "        : Will begin a transaction and display the tag";
   cout << endl << " disable trans[actions] [, shutdown normal|immediate] ";
   cout << endl << "        : Disable transaction processing in DTM.";
   cout << endl << "        : shutdown is only provided for testing and should";
   cout << endl << "        : not be used.  Use sqstop instead.";
   cout << endl << " drain <nid> [, immediate]  "
        << endl << "        : Drain transactions from a node."
        << endl << "        : This is similar to disable but affects"
        << endl << "        : only one node."
        << endl << "        : Immediately causes all active transactions"
        << endl << "        : in the node to be aborted.";
   cout << endl << " end | commit ";
   cout << endl << "        : Will end the current transaction";
   cout << endl << " enable trans[actions] ";
   cout << endl << "        : Enable transaction processing in DTM.";
   cout << endl << " l | list [<nid>] ";
   cout << endl << "        : Will list all transactions and their status for ";
   cout << endl << "        : node <nid> or all nodes if none specified.";
   cout << endl << " r | t | resume [<tag>]";
   cout << endl << "        : Will suspend the current transaction if no tag is supplied.";
   cout << endl << "        : if tag is specified it will resume work on the given tag";
   cout << endl << " s | stats [<nid> | [reset] -j]";
   cout << endl << "        : Will list the current TM statistics";
   cout << endl << "        : If reset is specified, reset statistics after ";
   cout << endl << "        : displaying them.";
   cout << endl << " status system";
   cout << endl << "        : Prints system TM information";
   cout << endl << " status tm [<nid>] [rmid] -j";
   cout << endl << "        : Status of the TM in node <nid>.  Returns TM information ";
   cout << endl << "        : for all nodes if none specified.  Specifying a node gives ";
   cout << endl << "        : RM details.";
   cout << endl << "        : <rmid> option sorts by rmid, default is sort by rm name.";
   cout << endl << " status trans[action] [transid]";
   cout << endl << "        : Status of the specified transaction.";
   cout << endl << "        : transid may be in numeric or <node>,<sequence> format.";
   cout << endl << " status regions";
   cout << endl << "        : Status of the transactions on the hbase regions - client side";
   cout << endl << " transid [string] <transid>";
   cout << endl << "        : Prints transaction ID information";
   cout << endl << "        : Entering string option outputs (node, sequence, incarn #)";
   cout << endl << " h | ? | help";
   cout << endl << "        : Display this help.";
   cout << endl << " q | quit | exit";
   cout << endl << "        : Exit dtmci" << endl;
} //print_helptext


void print_ciHelp()
{
   cout << "dtmci: DTM command line interface." << endl;
   cout << "Usage: dtmci [OPTION] [command]" << endl;
   cout << "  -h, -?, --help" << "\tdisplay help text." << endl;
   cout << "  --version\t\tversion information." << endl;
   cout << "  command\t\tdtmci command." << endl << endl;
} //print_ciHelp

void print_ciError(char *argv)
{
   cout << "dtmci: Invalid option -- " << argv << endl;
} //print_ciError


int processArgs(int argc, char *argv[]) 
{
   int i = 1; //Ignore arg[0] - prog name
   bool done = false;
   char *lp_arg = argv[i];

   while (!done)
   {
      if (!memcmp(&lp_arg[0], "-", 1))
      {
         if (!strcmp(&lp_arg[1], "h") || !strcmp(&lp_arg[1], "H") ||
             !strcmp(&lp_arg[1], "?"))
         {
            print_ciHelp();
            exit(0);
         }
         else if (!strcmp(&lp_arg[1], "-help"))
         {
            print_ciHelp();
            print_helptext();
            exit(0);
         }
         else 
         {
            print_ciError(argv[i]);
            exit(-1);
         }
         i++;
         if (i == argc)
            done = true;
         else
            lp_arg = argv[i];
      }
      else
         done = true;
   }
   return argc - i;
} //processArgs


int main(int argc, char *argv[]) 
{
    char la_in[INPUT_LENGTH];
    char la_cpy[INPUT_LENGTH];
    char *lp_inputstr;
    char lp_nextcmd[INPUT_LENGTH] = "";
    bool lv_done = false;
    int  lv_param1;
    bool lv_bool1;
    const char lc_name[10] = "";
    char * lp_name = (char *) &lc_name;
    int32 lv_error = FEOK;
    struct timeval lv_begintime, lv_endtime, lv_timedifference;
    bool lv_timeset = false;
    char *lp_cmd = NULL;
    bool lv_singleCommand = false;
    bool lv_shellExec = false;
    FILE *lp_in = NULL;
    

    CALL_COMP_DOVERS(dtmci, argc, argv);

    lp_cmd = new char[INPUT_LENGTH];
    memset(lp_cmd, 0, INPUT_LENGTH);

    if (argc > 1)
    {
       int lv_startArg = 1;
       if(!strcmp(argv[1], "SQMON1.0")) {
          lv_shellExec = true;
       }
       else
          // Started normally and has argument
          lv_singleCommand = true;

       if(lv_shellExec && argc > 11) {
          lv_startArg = 11;
          lv_singleCommand = true;
       }

       if(lv_singleCommand) {
          int lv_commands = processArgs(argc, argv);
          lp_cmd = new char[INPUT_LENGTH];
          memset(lp_cmd, 0, INPUT_LENGTH);
          for (int i=lv_startArg; i<=lv_commands; i++)
          {
             strcat(lp_cmd, argv[i]);
             strcat(lp_cmd, " ");
             argc--;
             memset(argv[i], 0, 1);
          }
       }
    }

    //sleep(60);
    if (!lv_singleCommand)
       cout << "DTM Command line interface. " << ga_timestamp << endl;

    try 
    {
        if(lv_shellExec) {
            lv_error = msg_init(&argc, &argv);
        } 
        else {
            lv_error = msg_init_attach(&argc, &argv, true, lp_name);
        }
        if (lv_error)
        {
            cout << "** Error " << lv_error 
                << " while attempting to attach to Monitor, exiting." << endl;
            exit(-2);
        }
        lv_error = msg_mon_process_startup3(false, false); 
        if (lv_error)
        {
            cout << "** Error " << lv_error
                << " from msg_mon_process_startup, exiting." << endl;
            exit(-3);
        }
    }
    catch (SB_Fatal_Excep lv_except)
    {
        cout << "** Error: unable to attach to Monitor or start Seaquest process, exiting." << endl;
        exit(-1);
    }
    if(lv_shellExec) 
          lp_in = openInput(argv[10]);

    putenv((char *) "TMLIB_ENABLE_CLEANUP=0");
    while (!lv_done)
    {
        char *lp_readline;
        if (lv_singleCommand)
           lp_readline = lp_cmd;
        else if (lv_shellExec) {
           char *la_buffer = new char[INPUT_LENGTH];
           cout << ">>";
           if(fgets(la_buffer, INPUT_LENGTH, lp_in) == NULL) {
                 cout << "**Error: input value is NULL, exiting." << endl;
                 msg_mon_process_shutdown();
                 exit(-1);
           }
           la_buffer[strlen(la_buffer) - 1] = '\0';
           lp_readline = la_buffer;
        }
        else
           lp_readline = readline("DTMCI > ");
        if(!lp_readline) {
            lv_done = true;
            cout << endl;
            break;
        }
        strncpy(la_in , remove_white_space(lp_readline), INPUT_LENGTH);
        free(lp_readline);
        lp_inputstr = la_in;
        strncpy(la_cpy, la_in, INPUT_LENGTH);
        if(strcmp(lp_inputstr, "") != 0) { 
            add_history(lp_inputstr);
        }
        get_cmd(lp_inputstr, lp_nextcmd);

        // Process commands:
        if (!strcmp(lp_nextcmd, "l") || !strcmp(lp_nextcmd, "list"))
        {
            get_cmd(lp_inputstr, lp_nextcmd);
            if (lp_nextcmd[0] == '\0')
                lv_param1 = -1;
            else
            {
                if (!strcmp(lp_nextcmd, "*") || lp_nextcmd[0] == 0)
                    lv_param1 = -1;
                else
                    lv_param1 = atoi(lp_nextcmd);
            }
            try {
                process_list(lv_param1);
            }
            catch(SB_Fatal_Excep &){
                cout << "An error occurred while trying to process list\n";
            }
        }
        else if ( !strcmp(lp_nextcmd, "attach"))
        {
            get_cmd(lp_inputstr, lp_nextcmd);
            if (!strcmp(lp_nextcmd, "rm"))
            {
                get_cmd(lp_inputstr, lp_nextcmd);
                if (lp_nextcmd[0] == '\0')
                    cout << "Missing qualifier [TSE name]"<<endl;
                else 
                    process_attachrm(lp_nextcmd);
            }
            else
               cout << "Missing qualifier [rm]"<<endl; 

        }

        else if (!strcmp(lp_nextcmd, "b") || !strcmp(lp_nextcmd, "begin"))
        {
            int lv_tag = 0;
            int lv_begin_error = 0;
            lv_begin_error = BEGINTRANSACTION( &lv_tag  );
            if (lv_begin_error == 86) {
                cout << "Unable to begin transaction, BEGINTRANSACTION is disabled" << endl;
            }
            cout << "BEGINTRANSACTION returned error " << lv_begin_error
                << ", tag " << lv_tag << endl;

            int32 lv_node, lv_seqnum;
            bool lv_local = DTM_LOCALTRANSACTION(&lv_node, &lv_seqnum);
            if (lv_local) 
               cout << "Beginning local transaction (" << lv_node << "," << lv_seqnum << ")." << endl;
            else
               cout << "Beginning global transaction (" << lv_node << "," << lv_seqnum << ")." << endl;

            //set gettimeofday beginning struct here
            gettimeofday(&lv_begintime, NULL);
            lv_timeset = true;
        }
   
        else if (!strcmp(lp_nextcmd, "disable"))
        {
            get_cmd(lp_inputstr, lp_nextcmd);
            // Remove any trailing comma.
            char *lp_position = strchr(lp_nextcmd, ',');
            if (lp_position)
                *lp_position = 0;
            
            if (!strcmp(lp_nextcmd, "transactions") || !strcmp(lp_nextcmd, "transaction") ||
                !strcmp(lp_nextcmd, "trans"))
            {
                get_cmd(lp_inputstr, lp_nextcmd);
                if (lp_nextcmd[0] == '\0')
                    // Not a shutdown
                    process_disableTrans(TM_DISABLE_NORMAL);
                else
                {
                    if (!strcmp(lp_nextcmd, ","))
                        get_cmd(lp_inputstr, lp_nextcmd);
   
                    if (!strcmp(lp_nextcmd, "shutdown"))
                    {
                        get_cmd(lp_inputstr, lp_nextcmd);
                        if (!strcmp(lp_nextcmd, "normal"))
                            process_disableTrans(TM_DISABLE_SHUTDOWN_NORMAL);
                        else if (!strcmp(lp_nextcmd, "immediate"))
                            process_disableTrans(TM_DISABLE_SHUTDOWN_IMMEDIATE);
                        else
                            cout << "** Disable trans shutdown missing qualifier." << endl;
                    }
                    else
                        cout << "** Disable trans invalid qualifier " << lp_nextcmd << "." << endl;
                }
            }
            else
                cout << "** Disable missing qualifier trans[actions]." << endl;
        }
   
        else if (!strcmp(lp_nextcmd, "drain"))
        {
            get_cmd(lp_inputstr, lp_nextcmd);
            if (lp_nextcmd[0] == '\0')
            {
                 // Expected node
                cout << "** Drain missing node id." << endl;
                lv_param1 = -1;
            }
            else
            {
                // Get nid
                lv_param1 = atoi(lp_nextcmd);
                lv_bool1 = false;
                get_cmd(lp_inputstr, lp_nextcmd);
                if (!strcmp(lp_nextcmd, "immediate"))
                        lv_bool1 = true;
                    else
                        lv_bool1 = false;
   
                process_drainTrans(lv_param1, lv_bool1);
            }
        } //drainTrans
   
        else if (!strcmp(lp_nextcmd, "quiesce"))
        {
            cout << "!!!! Quiesce is only available here for Development testing and must "
                 << "not be used for any other purpose." << endl;

            get_cmd(lp_inputstr, lp_nextcmd);
            if (lp_nextcmd[0] == '\0')
            {
                // Expected node
                cout << "** Quiesce missing node id." << endl;
               lv_param1 = -1;
            }
            else
            {
                // Get nid
                lv_param1 = atoi(lp_nextcmd);
                lv_bool1 = false;
                get_cmd(lp_inputstr, lp_nextcmd);
                if (!strcmp(lp_nextcmd, "stop"))
                        lv_bool1 = true;
                    else
                        lv_bool1 = false;
   
                if (lv_bool1)
                    process_unquiesce(lv_param1);
                else
                    process_quiesce(lv_param1);
            }
        } //quiesce
   
       else if (!strcmp(lp_nextcmd, "end") || !strcmp(lp_nextcmd, "commit"))
        {
            int lv_end_error = ENDTRANSACTION();
            if(lv_end_error != 0) {
                cout << "ENDTRANSACTION returned error " << lv_end_error << endl;
            }
            if(lv_timeset == true) {

                gettimeofday(&lv_endtime, NULL);
                get_time_difference(&lv_timedifference, &lv_begintime, &lv_endtime);

                //Print out the time difference
                cout << "Elapsed Time: " << printtimeformatted(lv_timedifference) << endl;
                lv_timeset = false;
            }
        }
        else if (!strcmp(lp_nextcmd, "a") || !strcmp(lp_nextcmd, "abort"))
        {
            int lv_abort_error = ABORTTRANSACTION();
                if(lv_abort_error != 0) {
                cout << "** ABORTTRANSACTION returned error " << lv_abort_error;
                if(lv_abort_error == 75) {
                    cout << ", FENOTRANSID";
                }
                cout << endl;
            }
        }
   
        else if (!strcmp(lp_nextcmd, "enable"))
        {
            get_cmd(lp_inputstr, lp_nextcmd);
            if (!strcmp(lp_nextcmd, "transactions") || !strcmp(lp_nextcmd, "transaction") ||
                !strcmp(lp_nextcmd, "trans"))
            {
                process_enableTrans();
            }
            else
                cout << "** Enable missing qualifier trans[actions]." << endl;
        }
   
        else if (!strcmp(lp_nextcmd, "r") || !strcmp(lp_nextcmd, "t") || !strcmp(lp_nextcmd, "resume"))
        {
            get_cmd(lp_inputstr, lp_nextcmd);
            int lv_resume_error = FEOK;
            if (lp_nextcmd[0] == '\0')
                lv_param1 = 0;
            else
            {
                lv_param1 = atoi(lp_nextcmd);
            }
            if (lv_param1 == 0)
                lv_resume_error= RESUMETRANSACTION(0);
            else
                lv_resume_error= RESUMETRANSACTION(lv_param1);
                cout << "RESUMETRANSACTION(" << lv_param1 << ") returned error " << lv_resume_error << endl;
        }
   
        else if (!strcmp(lp_nextcmd, "s") || !strcmp(lp_nextcmd, "stats"))
        {
            get_cmd(lp_inputstr, lp_nextcmd);
            if (lp_nextcmd[0] == '\0')
                process_tmstats(false, -1, false);
            else
            {
                if(!strcmp(lp_nextcmd, "-j")) {
                    process_tmstats(false, -1, true);
                } else {
                    if (!strcmp(lp_nextcmd, "*"))
                        lv_param1 = -1;
                    else
                        lv_param1 = atoi(lp_nextcmd);

                    get_cmd(lp_inputstr, lp_nextcmd);
                    if (lp_nextcmd[0] == '\0') 
                        process_tmstats(false, lv_param1, false);                    
                    else if (!strcmp(lp_nextcmd, "-j"))
                        process_tmstats(false, lv_param1, true);
                    else
                    {
                        if (!strcmp(lp_nextcmd, "reset"))
                            process_tmstats(true, lv_param1, false);
                        else
                            cout << "** Stats invalid qualifier " << lp_nextcmd << "." << endl;
                    }
                }
            }
        }
        else if (!strcmp(lp_nextcmd, "status"))
        {
            bool b_rmid, b_json = false;

            get_cmd(lp_inputstr, lp_nextcmd);
            if (!strcmp(lp_nextcmd, "tm"))
            {
                get_cmd(lp_inputstr, lp_nextcmd);
                if (!strcmp(lp_nextcmd, "*") || lp_nextcmd[0] == '\0')
                    lv_param1 = -1;
                else if (isdigit(lp_nextcmd[0])) {
                    lv_param1 = atoi(lp_nextcmd);
                }
                else {
                    lv_param1 = -1;
                }
                if(!strcmp(lp_nextcmd, "-j"))
                    b_json = true;
                get_cmd(lp_inputstr, lp_nextcmd);
                if(!strcmp(lp_nextcmd, "rmid")) {
                    //process_statustm(lv_param1, true);
                    b_rmid = true;
                }
                else if(!strcmp(lp_nextcmd, "-j")) {
                    b_json = true;
                    //process_statustm(lv_param1, false);
                }
                process_statustm(lv_param1, b_rmid, b_json);
            }
            else if (!strcmp(lp_nextcmd, "transaction") || !strcmp(lp_nextcmd, "trans")) 
            {
               get_cmd(lp_inputstr, lp_nextcmd);
               if (!strcmp(lp_nextcmd, "*") || lp_nextcmd[0] == '\0')
               {
                  get_cmd(lp_inputstr, lp_nextcmd);
                  if (lp_nextcmd[0]=='\0')
                  {
                     lv_param1=-1;
                     process_statusalltransactions(lv_param1);
                  }
                  else
                  {
                     cout << "Please specify a transaction ID or use *" << endl;
                  }
               }
               else
                  process_statustransaction(lp_nextcmd);
            }
            else if (!strcmp(lp_nextcmd, "system"))
            {
                process_statussystem();
            }
            else if(!strcmp(lp_nextcmd, "regions"))
            {
                process_request_regions_info();
            }
            else
            {
                cout << endl << "** Invalid qualifier '" <<lp_nextcmd 
                    << "' for status command." << endl;
            }
        }
        else if (!strcmp(lp_nextcmd, "showenv"))
        {
           get_cmd(lp_inputstr, lp_nextcmd);
           if (lp_nextcmd[0] == '\0')
             cout <<"TODO: show all env var\n" << endl;
           else
           {
              const char* v = ms_getenv_str((const char*)lp_nextcmd);
              if(v == NULL)
                cout <<"*** Environment Var not exist" << endl;
              else
                cout << v << endl;
           }
        }
        else if (!strcmp(lp_nextcmd, "transid"))
        {
           get_cmd(lp_inputstr, lp_nextcmd);
           if (lp_nextcmd[0] == '\0')
              cout << "** Please specify a transaction ID" << endl;
           else if (!strcmp(lp_nextcmd, "string")) {
              get_cmd(lp_inputstr, lp_nextcmd);
              if (lp_nextcmd[0] == '\0')
                 cout << "** Please specify a transaction ID" << endl;
              else
                 process_gettransinfo(lp_nextcmd, true);
           }
           else
           {
              process_gettransinfo(lp_nextcmd, false);
           }
        }
        else if (!strcmp(lp_nextcmd, "e") || !strcmp(lp_nextcmd, "q") ||
                !strcmp(lp_nextcmd, "quit") || !strcmp(lp_nextcmd, "exit"))
        {
            cout << "dtmci exiting, goodbye." << endl;
            lv_done = true;
        }
   
        else if (!strcmp(lp_nextcmd, "h") || !strcmp(lp_nextcmd, "help") || !strcmp(lp_nextcmd, "?"))
        {
           print_helptext();
        }
        else if (strcmp(lp_nextcmd, "")!=0)
        {
            cout << "** Unknown command " << la_cpy << "." << endl;
        }
        if (lv_singleCommand)
           lv_done = true;
    } // while
    if(lv_shellExec)
        fclose(lp_in);
    TMCLIENTEXIT();
    msg_mon_process_shutdown();

    return 0;
   
}
