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
#include <sys/time.h>
#include <sys/resource.h>

#include <stdlib.h>

#include "tmaudit.h"
#include "tminfo.h"
#include "tmtx.h"
#include "tmlogging.h"
#include "tmglob.h"

/*****************************************************************
//
// This file is the Auditing interface file
//
*****************************************************************/

// -------------------------------------------------------------
// TM_Audit
// Purpose - Constructor
// -------------------------------------------------------------
TM_Audit::TM_Audit()
{
    iv_vsn = 1;
    ip_cursor = ip_audit_rec = ip_audit_file = NULL;
    iv_initialized = iv_notified_threshold = false;
    iv_position = -1;
    memset(ia_vol_name, 0, 4);
    memset(ia_vol_name2, 0, 9);
    memset(&iv_adp_phandle, 0, sizeof(SB_Phandle_Type));

#ifdef USE_FILE_AUDIT
     ip_audit_file = fopen ("tm_audit", "a");
#else
     ip_audit_file = NULL;
#endif
}

// -----------------------------------------------------------
// ~TM_AUDIT
// Purpose - Destructor
// -----------------------------------------------------------
TM_Audit::~TM_Audit()
{
#ifdef USE_FILE_AUDIT
    if (ip_audit_file)
       fclose(ip_audit_file);
    ip_audit_file = NULL;
#endif
    // Don't make this call since this is invoked from the global destructor,
    // after we've called msg_mon_process_shutdown. But this ends up calling
    // msg_mon_reg_get to talk to the AMP, and that is not valid.

    //adp_module_terminate();
}

// ----------------------------------------------------------
// start_adp
// Purpose - initialize the module and vol name for the ADP
// ----------------------------------------------------------
int TM_Audit::initialize_adp()
{
    TMTrace(2, ("TM_Audit::initialize_adp: ENTRY\n"));
    char  la_name[8];
    int32 lv_nid;
    int32 lv_pid;

    if (!iv_initialized)
    {
        msg_mon_get_process_info (NULL, &lv_nid, &lv_pid);
        sprintf(la_name, "0DTM%d", lv_nid);
        memcpy (ia_vol_name, la_name, strlen(la_name));
        sprintf (ia_vol_name2, la_name, strlen(la_name));

        iv_mutex.lock();
        iv_mutex.unlock();
    }

    TMTrace(2, ("TM_Audit::initialize_adp: EXIT, name %s\n", ia_vol_name2));
    return TM_OK;
}

// ----------------------------------------------------------
// initialize_hdr
// Purpose - initialize the header for an audit record
// -----------------------------------------------------------
void TM_Audit::initialize_hdr(Audit_Header *pp_hdr,int16 pv_rec_length,
                              int32 pv_type, TM_Transid_Type *pv_transid)
{
    pv_rec_length = pv_rec_length; //810
    pp_hdr->iv_length = REC_SIZE;
    pp_hdr->iv_type = (short)pv_type;
    if (pv_transid)
        memcpy (&(pp_hdr->iv_transid), pv_transid, TM_TRANSID_BYTE_SIZE);

    for (int lv_idx = 0; lv_idx < 4; lv_idx++)
        pp_hdr->iv_nameI[lv_idx] = ia_vol_name[lv_idx];

    memset(&(pp_hdr->iv_filler),0,2*sizeof(int16));
}

// ----------------------------------------------------------
// send_audit
// Purpose - send audit to the home grown TLOG - DELETE soon
// ----------------------------------------------------------

int TM_Audit::send_audit(char *pp_data, int32 pv_length)
{
#ifdef USE_FILE_AUDIT
    if (!pp_data)
        return -1;

    fwrite (pp_data, 1, pv_length, ip_audit_file);
    fflush(ip_audit_file);
#endif
    return TM_OK;
}


// -------------------------------------------------------
// write_buffer
// Purpose - write a buffer of audit to ASE
// ---------------------------------------------------------
int32 TM_Audit::write_buffer(int32 pv_length, char *pp_buffer, int64 pv_highest_vsn)
{
  TMTrace(2, ("TM_Audit::write_buffer: ENTRY\n"));
  int32 lv_notify = 0;
  TMTrace(2, ("TM_Audit::write_buffer EXIT\n"));
  return lv_notify;
}

// ---------------------------------------------------------
// write_control_point
// Purpose - write a control point to audit trail
// ---------------------------------------------------------
int32 TM_Audit::write_control_point(int32 pv_nid)
{
    Audit_Control_Point lv_rec;
    char                lv_write_buffer[REC_SIZE];
    int32               lv_notify = 0;

    TMTrace(2, ("TM_Audit::write_control_point: ENTRY\n"));

    pv_nid = pv_nid; //810
    initialize_hdr (&lv_rec.iv_hdr, REC_SIZE, TM_Control_Point, NULL);

    lv_rec.iv_time_stamp = SB_Thread::Sthr::time();
    lv_rec.iv_length = lv_rec.iv_hdr.iv_length;
    iv_mutex.lock();
    memcpy (&lv_rec.iv_hdr.iv_vsn, &iv_vsn, sizeof (iv_vsn));
    iv_vsn++;
    iv_mutex.unlock();
    memcpy (lv_write_buffer, (char*)&lv_rec, REC_SIZE);

#ifdef USE_FILE_AUDIT
    TM_Audit::send_audit (lv_write_buffer, REC_SIZE);
#endif

    TMTrace(2, ("TM_Audit::write_control_point: EXIT, notify=%d\n", lv_notify));

    return lv_notify;
}

void TM_Audit::audit_send_position() {
    int32 lv_position;
    TMTrace(3, ("TM_Audit::audit_send_position : ENTRY\n"));

    lv_position = (iv_position % 999999)- 1;

    if(lv_position == 0) {
       lv_position = 999999;
    }
    else if(lv_position < 0) {
       lv_position = 999998;
    }
    TMTrace(3, ("TM_Audit::audit_send_position EXIT\n"));
}

bool TM_Audit::prepare_trans_state(Audit_Transaction_State *pp_state_rec,
                                   char *pp_write_buffer, TM_Transid_Type *pv_transid,
                                   int32 pv_nid, int32 pv_state, int32 pv_abort_flags, int64 *pp_vsn=0)
{
    int32   lv_rec_type = -1;
    bool    lv_force = true;

    if (gv_tm_info.iv_trace_level >= 2)
    {
       TM_Txid_Internal *lp_transid = (TM_Txid_Internal *) pv_transid;
       trace_printf("TM_Audit::prepare_trans_state : ENTRY, txn ID (%d,%d), state=%d.\n",
                    lp_transid->iv_node, lp_transid->iv_seq_num, pv_state);
    }

    pv_nid = pv_nid; //810

    switch (pv_state)
    {
    case TM_TX_STATE_BEGINNING:
    case TM_TX_STATE_ACTIVE:
    case TM_TX_STATE_IDLE: //XARM only
    case TM_TX_STATE_NOTX:
    case TM_TX_STATE_PREPARING:
        {
            lv_rec_type = Active_Trans_State;
            break;
        }
    case TM_TX_STATE_FORGOTTEN:
    case TM_TX_STATE_FORGOTTEN_HEUR:
    case TM_TX_STATE_FORGETTING:
    case TM_TX_STATE_TERMINATING: //XARM only
        {
            lv_rec_type = Forgotten_Trans_State;
            lv_force = false;
            break;
        }
    case TM_TX_STATE_COMMITTED:
    case TM_TX_STATE_COMMITTING:
        {
            lv_rec_type = Committed_Trans_State;
            break;
        }
    case TM_TX_STATE_ABORTING:
    case TM_TX_STATE_ABORTING_PART2:
        {
            lv_rec_type = Aborting_Trans_State;
            break;
        }
    case TM_TX_STATE_ABORTED:
        {
            lv_rec_type = Aborted_Trans_State;
            break;
        }
    case TM_TX_STATE_HUNGCOMMITTED:
        {
            lv_rec_type = HungCommitted_Trans_State;
            break;
        }
    case TM_TX_STATE_HUNGABORTED:
        {
            lv_rec_type = HungAborted_Trans_State;
            break;
        }
    default:
        {
            // bad transaction state record. very, very bad!
            tm_log_event(DTM_TM_AUD_INVALID_TRANS_STATE, SQ_LOG_CRIT, "DTM_TM_AUD_INVALID_TRANS_STATE",
               -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,pv_state);
            TMTrace(2, ("TM_Audit::prepare_trans_state : Invalid transaction state %d reached.\n",
                    pv_state));
            abort();
        }
    }

    initialize_hdr(&pp_state_rec->iv_hdr, REC_SIZE,
                  TM_Transaction_State, pv_transid);

    pp_state_rec->iv_time_stamp = SB_Thread::Sthr::time();
    pp_state_rec->iv_state = (short) lv_rec_type;
    pp_state_rec->iv_abort_flags = pv_abort_flags;
    pp_state_rec->iv_length = pp_state_rec->iv_hdr.iv_length;
    iv_mutex.lock();
    memcpy (&pp_state_rec->iv_hdr.iv_vsn, &iv_vsn, sizeof (iv_vsn));
    if (pp_vsn != NULL)
        *pp_vsn = iv_vsn;
    iv_vsn++;
    iv_mutex.unlock();
    memcpy (pp_write_buffer, pp_state_rec, REC_SIZE);
    TMTrace(2, ("TM_Audit::prepare_trans_state : EXIT\n"));
    return lv_force;
}
// ----------------------------------------------------------
// write_trans_state
// Purpose - write transaction state record to audit trail
// ----------------------------------------------------------
int32 TM_Audit::write_trans_state(TM_Transid_Type *pv_transid,
                                 int32 pv_nid, int32 pv_state, int32 pv_abort_flags)
{
    Audit_Transaction_State lv_state_rec;
    char      lv_write_buffer[REC_SIZE];
    bool      lv_force = true;
    int32     lv_notify = 0;

    TMTrace(2, ("TM_Audit::write_trans_state: ENTRY \n"));
    lv_force = prepare_trans_state(&lv_state_rec, lv_write_buffer, pv_transid,
                                   pv_nid, pv_state, pv_abort_flags);

#ifdef USE_FILE_AUDIT
    TM_Audit::send_audit (lv_write_buffer, REC_SIZE);
#endif
    TMTrace(2, ("TM_Audit::write_trans_state: EXIT, notify=%d\n", lv_notify));

    return lv_notify;
}

void TM_Audit::write_shutdown(int32 pv_nid, int32 pv_state)
{
    Audit_TM_Shutdown       lv_rec;
    int64                   lv_vsn;
    char                    lv_write_buffer[REC_SIZE];

    pv_nid = pv_nid; // 810

    TMTrace(2, ("TM_Audit::write_shutdown: ENTRY, state = %d.\n", pv_state));
    initialize_hdr(&lv_rec.iv_hdr, REC_SIZE, TM_Shutdown, NULL);

    lv_rec.iv_time_stamp = SB_Thread::Sthr::time();
    lv_rec.iv_state = (short) pv_state;
    lv_rec.iv_length = lv_rec.iv_hdr.iv_length;
    memcpy (lv_write_buffer, (char *)&lv_rec, REC_SIZE);

#ifdef USE_FILE_AUDIT
    TM_Audit::send_audit (lv_write_buffer, REC_SIZE);
#endif

    iv_mutex.lock();
    lv_vsn = iv_vsn++;
    iv_mutex.unlock();


    TMTrace(2, ("TM_Audit::write_shutdown: EXIT\n"));
}

// reading utilities

// ------------------------------------------------------------
// end_backwards_scan
// Purpose - deactivate cursor which lets the adp reader know
//           we are done.
// ------------------------------------------------------------
void TM_Audit::end_backwards_scan()
{
}

// --------------------------------------------------------------
// start_backwards_scan
// Purpose - activate cursor - done at beginning of scan
// --------------------------------------------------------------
void TM_Audit::start_backwards_scan()
{
}

// --------------------------------------------------------------
// read_audit_rec
// Purpose - read audit record where cursor lies
// --------------------------------------------------------------
Addr TM_Audit::read_audit_rec()
{
    int16                     lv_hit_eof = false;

    ip_audit_rec = NULL;

    if (lv_hit_eof)
       ip_audit_rec = NULL;

    return ip_audit_rec ;
}

// -------------------------------------------------------------
// release_audit_rec
// Purpose - release the previously read audit rec
// -------------------------------------------------------------
void TM_Audit::release_audit_rec()
{
}

// wrappers for ADP API

// -------------------------------------------------------------
// adp_module_init
// Purpose - initialize module for use
// -------------------------------------------------------------
void TM_Audit::adp_module_init()
{
    TMTrace(2, ("TM_Audit::adp_module_init: ENTER\n"));

    MS_Mon_Reg_Get_Type lv_info;
    char  lv_my_pname[8+1];
    char  lv_tlog_pname[8+1];
    int   lv_len = 8;
    int   lv_err = 0;
    int32 lv_oid = 0;
    TPT_DECL (lv_phandle);
    int   lv_TLOG_index = TLOG_AuditTrailIndex;

    if (iv_initialized)
        return;

    msg_mon_get_my_process_name(lv_my_pname,lv_len);

    lv_err = msg_mon_reg_get(MS_Mon_ConfigType_Process, false,
                             lv_my_pname, (char *)"TMASE", &lv_info);
    if ((lv_err == 0) && (lv_info.num_returned == 1))
    {
        strncpy (lv_tlog_pname, lv_info.list[0].value, 8);
        TMTrace(3, ("TM_Audit::adp_module_init: %s opening TLOG %s\n", lv_my_pname, lv_tlog_pname));

        lv_err = msg_mon_open_process(lv_tlog_pname, &lv_phandle, &lv_oid);

        if (!lv_err)
        {
            memcpy (&iv_adp_phandle, &lv_phandle, sizeof (SB_Phandle_Type));
            // If we using multiple TLOGs, we need to set the correct index (= nid)
            if (gv_tm_info.TLOGperTM())
               lv_TLOG_index = gv_tm_info.nid();
            iv_initialized = true;
        }
        else
        {
           TMTrace(1, ("TM_Audit::adp_module_init: error %d opening TLOG %s\n",
                    lv_err, lv_tlog_pname));
           tm_log_event(DTM_AUDIT_FAILED_ADPOPEN, SQ_LOG_CRIT, "DTM_AUDIT_FAILED_ADPOPEN",
                        lv_err, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                        -1, -1, -1, -1, lv_my_pname);

           // Fast fail here if $TLOG is gone as we can't continue in M8.
           if (lv_err == FENOSUCHDEV)
           {
              gv_tm_info.error_shutdown_abrupt(lv_err);
              // Abort without core - don't need the core in this case.
              struct rlimit limit;
              limit.rlim_cur = 0;
              limit.rlim_max = 0;
              setrlimit(RLIMIT_CORE, &limit);
              abort();
           }
           iv_initialized = false;
        }
    } // lv_err = 0
    else
       iv_initialized = false;

    if (gv_tm_info.TLOGperTM())
    {
       TMTrace(2, ("TM_Audit::adp_module_init: EXIT, initialized = %d, name %s, TLOG%d.\n",
               iv_initialized, ia_vol_name2, lv_TLOG_index));
    }
    else
    {
       TMTrace(2, ("TM_Audit::adp_module_init: EXIT, initialized = %d, name %s, single TLOG.\n",
               iv_initialized, ia_vol_name2));
    }
}

// ------------------------------------------------------------
// adp_module_terminate
// Purpose - end module use
// ------------------------------------------------------------
void TM_Audit::adp_module_terminate()
{
    if (!iv_initialized)
        return;

}


// -------------------------------------------------------------
// adp_activate_cursor
// Purpose - activate cursor or a scan
// ------------------------------------------------------------
void TM_Audit::adp_activate_cursor()
{
   // If we're using a TLOG/TM then the index is nid.
   AuditTrailPosition_Struct lv_low_pos, lv_high_pos;

    if (!iv_initialized)
       return; // for now

    lv_low_pos.Sequence = 0;
    lv_low_pos.rba = 0;
    lv_high_pos.Sequence = 0;
    lv_high_pos.rba = 0;

}

// ------------------------------------------------------------
// adp_deactivate_cursor
// Purpose - end of a scan, release cursor
// ------------------------------------------------------------
void TM_Audit::adp_deactivate_cursor()
{
    if (!iv_initialized)
        return;

}

// -----------------------------------------------------------
// adp_send_audit
// Purpose - send audit buffer to adp
// return values : 0 - nothing to do
//                 1 - rollover
//                 2 - threshold hit
// ------------------------------------------------------------
int32 TM_Audit::adp_send_audit(char *pp_buffer, int32 pv_length, int64 pv_vsn, bool pv_force)
{
    int32 lv_notify = 0;
    int64 lv_old_seq = 0;
    int32 lv_error = FEEOF; // Set to non-zero for first pass to make sure we enter the while loop.
    int32 lv_error2 = 0;
    int32 lv_send_count = 0;
    const int lc_adp_send_retry_delay = 100; // .1 sec
    const int lc_adp_send_retry_maxretries = 30;

    if (!iv_initialized)
        return -1;

    TMTrace(2, ("TM_Audit::adp_send_audit: ENTER\n"));

    //If we all in state Quiesce, we can no longer talk to the ADP
    if (gv_tm_info.state() == TM_STATE_QUIESCE)
     {
        TMTrace(2, ("TM_Audit::adp_send_audit: Quiesced so not sending audit EXIT\n"));
        return 0;
     }

    iv_mutex.lock();
    TMTrace(4, ("TM_Audit::adp_send_audit: lock obtained\n"));
    AuditTrailPosition_Struct lv_buffer_pos;
   lv_buffer_pos.Sequence = lv_buffer_pos.rba = 0;

    while ((gv_tm_info.state() != TM_STATE_QUIESCE) &&
           (lv_error) && (++lv_send_count < lc_adp_send_retry_maxretries))
    {
      // We only force control points and commit/abort records.  Forgotten
      // ones we allow TSEs to buffer if they choose
        if (lv_error && (lv_send_count < lc_adp_send_retry_maxretries) &&
         (lv_send_count > 0))
           SB_Thread::Sthr::sleep(lc_adp_send_retry_delay);
    }
    if (gv_tm_info.state() == TM_STATE_QUIESCE)
     {
        TMTrace(2, ("TM_Audit::adp_send_audit: Quiesced so not sending audit EXIT(2)\n"));
        return 0;
     }

    if (lv_error)
    {
       // if after our retries and the ASE lib retries, we still can't write a record,
       // we have no choice but to die.
       tm_log_event(DTM_AUDIT_FAILED_WRITE, SQ_LOG_CRIT, "DTM_AUDIT_FAILED_WRITE", lv_error);
       TMTrace(1, ("TM_Audit::adp_send_audit: Failed to write audit to adp. Error "
                   "%d Shutting down Seaquest.\n", lv_error));
       msg_mon_shutdown(MS_Mon_ShutdownLevel_Abrupt);
       // Abort without core - don't need the core in this case.
       struct rlimit limit;
       limit.rlim_cur = 0;
       limit.rlim_max = 0;
       setrlimit(RLIMIT_CORE, &limit);
       abort();

    }
    // we rolled over, this will initiate a cp
    if (pv_force)  // we'll have a buffer position
   {
       if (iv_position != lv_buffer_pos.Sequence)
       {
             TMTrace(3, ("adp_send_audit: ROLLING OVER from " PFLL " to %d\n", iv_position, lv_buffer_pos.Sequence));
             lv_old_seq = iv_position;
             iv_position = lv_buffer_pos.Sequence;
             if (lv_old_seq != -1)
             {
                 iv_vsn = 1;
                 lv_notify = 1;
                 iv_notified_threshold = false;  // rolled over, under threshold
                 lv_error2 = gv_tm_info.write_rollover_control_point();
                 if(lv_error2) {
                     // Unable to inform lead TM to write control point
                     tm_log_event(DTM_ROLLOVER_CP_ERROR, SQ_LOG_CRIT, "DTM_ROLLOVER_CP_ERROR",
                                  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,lv_error2);
                     TMTrace(1, ("TM_Audit::adp_send_audit: Error occurred attempting to write rollover control point"
                                 " %d.\n", lv_error2));
                     abort();
                 }
             }
       }
       else if ((!iv_notified_threshold) &&
             (lv_buffer_pos.rba > CP_THRESHOLD))
       {
             TMTrace(3, ("adp_send_audit: THRESHOLD from " PFLL " to %d\n", iv_position, lv_buffer_pos.Sequence));
             lv_notify = 2;
             iv_notified_threshold = true;
       }
   }

    iv_mutex.unlock();
    TMTrace(2, ("TM_Audit::adp_send_audit: EXIT. notify=%d, vsn=" PFLL ", notified threshold=%d\n",
        lv_notify, iv_vsn, iv_notified_threshold));
    return lv_notify;
}

// -----------------------------------------------------------
// adp_release_record
// Purpose - release record that was just read
// -----------------------------------------------------------
void TM_Audit::adp_release_record()
{
    if (!iv_initialized)
        return;

}
