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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "tminfo.h"
#include "tmlogging.h"
#include "tmregistry.h"
#include "tmtx.h"

#include "seabed/pctl.h"
#include "seabed/trace.h"

Tm_Sync_Type_Transid * new_sync_transid_type (TM_SYNC_TYPE pv_type)
{
     Tm_Sync_Type_Transid *pp_data = new Tm_Sync_Type_Transid;
     pp_data->iv_sync_type = pv_type;
     pp_data->iv_num_tries = 0;

     return pp_data;
}

// ---------------------------------------------------------------
// init_and_send_sync
// Purpose - init data and send the sync
// ---------------------------------------------------------------
void init_and_send_tx_sync_data( TM_SYNC_TYPE pv_type, TM_TX_STATE pv_state,
                     TM_Transid_Type *pp_transid, int32 pv_nid, 
                     int32 pv_pid)
{
    TM_Txid_Internal     *lp_i_transid = (TM_Txid_Internal*) pp_transid;
    Tm_Sync_Type_Transid *lp_data = NULL;
    Tm_Sync_Data          lv_data;
    
    lv_data.iv_hdr.iv_nid = pv_nid;
    lv_data.iv_hdr.iv_type = pv_type;

    lv_data.u.iv_tx_data.iv_state = pv_state;
    lv_data.u.iv_tx_data.iv_pid = pv_pid;
    memcpy(&lv_data.u.iv_tx_data.iv_transid , pp_transid, TM_TRANSID_BYTE_SIZE);

    lp_data = new_sync_transid_type (pv_type);
    lp_data->u.iv_seqnum = lp_i_transid->iv_seq_num;
    send_sync_data (&lv_data, sizeof (lv_data), pv_type, pv_nid, lp_data);
}

// --------------------------------------------------------------
// send_sync_data
// Purpose - send the sync data out via the monitor.  If there is
//           an error, we will retry 25 times with a node specific
//           delay time (to avoid continuous collisions if that is
//           why we failed
// --------------------------------------------------------------
void send_sync_data (void *pv_buffer, int32 pv_length,
                     TM_SYNC_TYPE pv_type, int32 pv_nid, 
                     Tm_Sync_Type_Transid* pp_data)
{
    const int64     lc_wait_interval = (2 + pv_nid);// 10ms units
    int32           lv_count = 0;
    bool            lv_done = false;
    int32           lv_error = 0;
    int32           lv_handle = -1;
    int64           lv_wait = lc_wait_interval;
    int32           lv_sync_otag = gv_tm_info.add_sync_otag(pp_data);

    pv_type = pv_type; // intel compiler warning 869
    TMTrace(2, ("send_sync_data ENTRY for ID %d, type %d\n",
                      pp_data->u.iv_seqnum, pv_type));

    // arbitrarily try for 25 times now

    while ((!lv_done) && (lv_count++ < 25))
    {
        // now send to monitor via seabed, and get the handle
        //gv_tm_info.msg_mon_lock();
        lv_error = msg_mon_tmsync_issue (pv_buffer, pv_length, &lv_handle, lv_sync_otag);
        //gv_tm_info.msg_mon_unlock();
        if (!lv_error)
        {
            lv_done = true;
            TMTrace(3, ("msg_mon_tmsync_issue() for ID %d, type %d, returned handle %d, tag %d.\n",
                            pp_data->u.iv_seqnum, pv_type, lv_handle, lv_sync_otag));
        }
        else
        {
            TMTrace(3, ("msg_mon_tmsync_issue() failed with error %d, retry attempt %d.\n",
                            lv_error, lv_count));
            XWAIT(0, (int)lv_wait); // the 0 means only wake up for timeout
        }
    }

    // if after 25 tries, find a better way to exit
    // DTM_DEATH here when it is available
    if (!lv_done)
    {
      // EMS DTM_XATM_COMPLETEALL_FAILED
      tm_log_event(DTM_SYNC_SEND_FAILED, SQ_LOG_CRIT, "DTM_SYNC_SEND_FAILED", lv_error);
      TMTrace(1, ("send_sync_data - msg_mon_tmsync_issue failed with error %d, retries exceeded. "
              "TM exiting.\n", lv_error));
      // Dispose of sync tag
      gv_tm_info.remove_sync_otag(lv_sync_otag);
      assert (lv_done == true);
    }
    TMTrace(2, ("send_sync_data EXIT\n"));
}

// ------------------------------------------------------------
// send_state_up_sync
// Purpose - the lead DTM will send this out to allow other
//           DTMs up for processing
// -----------------------------------------------------------
void send_state_up_sync(int32 pv_nid)
{
    TMTrace(2, ("send_state_up_sync ENTRY.\n"));
    Tm_Sync_Header lv_hdr;
    lv_hdr.iv_type = TM_UP;
    lv_hdr.iv_nid = pv_nid;
    send_sync_data(&lv_hdr, sizeof (lv_hdr), TM_UP, pv_nid, 
                    new_sync_transid_type (TM_UP));
}

// --------------------------------------------------------------------
// send_sys_recov_start_sync
// Purpose : Indicate begin of sys recovery but scantrail not completed
// --------------------------------------------------------------------
void send_sys_recov_start_sync(int32 pv_nid)
{
    TMTrace(2, ("send_sys_recov_start_sync ENTRY.\n"));
    Tm_Sync_Data lv_sync;
    lv_sync.iv_hdr.iv_type = TM_SYS_RECOV_START_SYNC;
    lv_sync.iv_hdr.iv_nid = pv_nid;
    lv_sync.u.iv_sys_recov_data.iv_sys_recov_state = TM_SYS_RECOV_STATE_START;
    lv_sync.u.iv_sys_recov_data.iv_sys_recov_lead_tm_node = pv_nid;

    send_sync_data (&lv_sync, sizeof (lv_sync), TM_SYS_RECOV_START_SYNC, pv_nid,
                    new_sync_transid_type (TM_SYS_RECOV_START_SYNC));
}

// ----------------------------------------------
// send_sys_recov_end_sync
// Purpose : Indicate completion of sys recovery.
// ----------------------------------------------
void send_sys_recov_end_sync(int32 pv_nid)
{
    TMTrace(2, ("send_sys_recov_end_sync ENTRY.\n"));
    Tm_Sync_Data lv_sync;
    lv_sync.iv_hdr.iv_type = TM_SYS_RECOV_END_SYNC;
    lv_sync.iv_hdr.iv_nid = pv_nid;
    lv_sync.u.iv_sys_recov_data.iv_sys_recov_state = TM_SYS_RECOV_STATE_END;
    lv_sync.u.iv_sys_recov_data.iv_sys_recov_lead_tm_node = pv_nid;

    send_sync_data (&lv_sync, sizeof (lv_sync), TM_SYS_RECOV_END_SYNC, pv_nid, 
                     new_sync_transid_type (TM_SYS_RECOV_END_SYNC));
}

void send_takeover_tm_sync(TM_SYNC_TYPE pv_type, int32 pv_nid, 
                           int32 pv_down_tm)
{
    TMTrace(2, ("send_takeover_tm_sync ENTRY type %d, my nid %d, down tm nid %d.\n",
            pv_type, pv_nid, pv_down_tm));
    Tm_Sync_Type_Transid *lp_data = NULL;
    Tm_Sync_Data          lv_sync;

    lv_sync.iv_hdr.iv_type = pv_type;
    lv_sync.iv_hdr.iv_nid = pv_nid;
    lv_sync.u.iv_to_data.iv_my_node = pv_nid;
    lv_sync.u.iv_to_data.iv_down_node = pv_down_tm;

    lp_data = new_sync_transid_type (pv_type);
    lp_data->u.iv_node_to_takeover = pv_down_tm;

   send_sync_data (&lv_sync, sizeof (lv_sync), pv_type, pv_nid, lp_data);
   TMTrace(2, ("send_takeover_tm_sync EXIT.\n"));
}

void send_tm_process_restart_sync(int32 pv_lead_nid, int32 pv_restart_nid)
{
    TMTrace(2, ("send_tm_process_restart_sync ENTRY.\n"));
    Tm_Sync_Data lv_sync;
    lv_sync.iv_hdr.iv_type = TM_PROCESS_RESTART;
    lv_sync.iv_hdr.iv_nid = pv_lead_nid;
    lv_sync.u.iv_proc_restart_data.iv_proc_restart_lead_tm_node = pv_lead_nid;
    lv_sync.u.iv_proc_restart_data.iv_proc_restart_node = pv_restart_nid;

    send_sync_data (&lv_sync, sizeof (lv_sync), TM_PROCESS_RESTART, pv_lead_nid, 
                     new_sync_transid_type (TM_PROCESS_RESTART));
    TMTrace(2, ("send_tm_process_restart_sync EXIT.\n"));
}

// --------------------------------------------------------------------
// send_takeover_tm_sync
// Purpose : Indicate begin/end of building the tx list during recovery
// --------------------------------------------------------------------
void send_recov_listbuilt_sync(int32 pv_nid, int32 pv_down_tm)
{
    TMTrace(2, ("send_recov_listbuilt my nid %d, down tm nid %d.\n",
                pv_nid, pv_down_tm));
    Tm_Sync_Type_Transid *lp_data = NULL;
    Tm_Sync_Data          lv_sync;

    lv_sync.iv_hdr.iv_type = TM_LISTBUILT_SYNC;
    lv_sync.iv_hdr.iv_nid = pv_nid;

    lp_data = new_sync_transid_type (TM_LISTBUILT_SYNC);
    lp_data->u.iv_node_to_takeover = pv_down_tm;
    lv_sync.u.iv_list_built.iv_down_node = pv_down_tm;

   send_sync_data (&lv_sync, sizeof (lv_sync), TM_LISTBUILT_SYNC, pv_nid, lp_data);
   TMTrace(2, ("send_recov_listbuilt EXIT.\n"));
}

// --------------------------------------------------------------------
// send_state_resync
// Purpose : Resent TM state information when a node is reintegrated
// --------------------------------------------------------------------
void send_state_resync(int32 pv_nid, bool  pv_down_without_sync, int32 pv_node_being_recovered,
          bool  pv_list_built, int32 pv_index)
{
    TMTrace(2, ("send_state_resync (TM_STATE_RESYNC) ENTRY with index %d, down_without_sync(%d), " 
                " node_being_recovered(%d), list_built(%d)\n", pv_index, pv_down_without_sync, 
                pv_node_being_recovered, pv_list_built));
    Tm_Sync_Type_Transid *lp_data = NULL;
    Tm_Sync_Data          lv_sync;

    lv_sync.iv_hdr.iv_type = TM_STATE_RESYNC;
    lv_sync.iv_hdr.iv_nid = pv_nid;

    lp_data = new_sync_transid_type (TM_STATE_RESYNC);
    lp_data->u.iv_node_to_takeover = pv_index;

    lv_sync.u.iv_state_resync.iv_index = pv_index;
    lv_sync.u.iv_state_resync.iv_down_without_sync = pv_down_without_sync;
    lv_sync.u.iv_state_resync.iv_node_being_recovered = pv_node_being_recovered;
    lv_sync.u.iv_state_resync.iv_list_built = pv_list_built;

    send_sync_data (&lv_sync, sizeof (lv_sync), TM_STATE_RESYNC, pv_nid, lp_data);
    TMTrace(2, ("send_state_resync (TM_STATE_RESYNC) EXIT.\n"));
} 

