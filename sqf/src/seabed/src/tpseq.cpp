//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@

//
// Implement tp-sequencer (test point sequencer) module
//

#include "seabed/sautil.h"
#include "seabed/tpseq.h"

#include "verslib.h"

VERS_LIB(libsbtpseq)


bool SB_TP_Seq::cv_debug = false;

//
// TP constructor
//
SB_TP_Seq::SB_TP_Seq::TP::TP(const char *pp_tp_name)
: ip_tp_name(pp_tp_name) {
}

SB_TP_Seq::SB_TP_Seq::TP::~TP() {
}

//
// TPL constructor
//
SB_TP_Seq::SB_TP_Seq::TPL::TPL(TP *pp_tp)
: ip_tp(pp_tp), ip_tp_next(NULL) {
}

SB_TP_Seq::SB_TP_Seq::TPL::~TPL() {
}

//
// TP-sequencer constructor
//
SB_TP_Seq::SB_TP_Seq(const char *pp_seq_name)
: ip_seq_name(pp_seq_name),
  ip_tp_cur(NULL),
  ip_tp_head(NULL),
  ip_tp_tail(NULL),
  iv_tp_count(0),
  iv_tp_state(TP_STATE_INIT) {
    TP_CV *lp_cv;
    int    lv_slot;

    for (lv_slot = 0; lv_slot < TP_MAX_CV; lv_slot++) {
        lp_cv = &ia_cv_list[lv_slot];
        lp_cv->iv_inuse = false;
        lp_cv->iv_slot = lv_slot;
        lp_cv->iv_cv.reset_flag();
    }
    ip_cv_mgr =
      new SB_SA_Util_Slot_Mgr("cv",
                              SB_SA_Util_Slot_Mgr::ALLOC_FIFO,
                              TP_MAX_CV);
}

//
// TP-sequencer destructor
//
SB_TP_Seq::~SB_TP_Seq() {
    delete ip_cv_mgr;
}

//
// TP-sequencer cv allocator
//
SB_TP_Seq::TP_CV *SB_TP_Seq::cv_alloc() {
    TP_CV *lp_cv;
    int    lv_slot;

    lv_slot = ip_cv_mgr->alloc();
    lp_cv = &ia_cv_list[lv_slot];
    lp_cv->iv_inuse = true;
    lp_cv->iv_cv.reset_flag();
    return lp_cv;
}

//
// TP-sequencer cv deallocator
//
void SB_TP_Seq::cv_free(TP_CV *pp_cv) {
    SB_SA_util_assert_cpne(pp_cv, NULL);
    ip_cv_mgr->free_slot(pp_cv->iv_slot);
    pp_cv->iv_inuse = false;
}

//
// TP-sequencer set debug
//
void SB_TP_Seq::set_debug(bool pv_debug) {
    cv_debug = pv_debug;
}

//
// TP-sequencer add TP to sequencer
//
void SB_TP_Seq::tp_add(TP *pp_tp) {
    TPL *lp_tp;

    lp_tp = new TPL(pp_tp);
    if (ip_tp_head == NULL)
        ip_tp_head = lp_tp;
    else
        ip_tp_tail->ip_tp_next = lp_tp;
    ip_tp_tail = lp_tp;
    iv_tp_count++;
}

//
// TP-sequencer advance to next TP
//
void SB_TP_Seq::tp_advance(TP *pp_tp) {
    const char *WHERE = "SB_TP_Seq::tp_advance";
    const char *lp_tp_name;

    switch (iv_tp_state) {
    case TP_STATE_INIT:
        ip_tp_cur = ip_tp_head->ip_tp_next;
        if (cv_debug) {
            if (ip_tp_cur == NULL)
                lp_tp_name = "<none>";
            else
                lp_tp_name = ip_tp_cur->ip_tp->ip_tp_name;
            SB_SA_Util_Debug::debug_printf(WHERE,
                                           "Sequencer. name=%s, INIT. cur-tp=%s, setting next-tp=%s\n",
                                           ip_seq_name, pp_tp->ip_tp_name, lp_tp_name);
        }
        iv_tp_state = TP_STATE_RUN;
        if (ip_tp_cur != NULL)
            tp_unblock(ip_tp_cur->ip_tp);
        break;
    case TP_STATE_RUN:
        if (ip_tp_cur == NULL)
            ip_tp_cur = ip_tp_head;
        else
            ip_tp_cur = ip_tp_cur->ip_tp_next;
        if (ip_tp_cur == NULL)
            iv_tp_state = TP_STATE_DONE;
        if (cv_debug) {
            if (ip_tp_cur == NULL)
                lp_tp_name = "<none>";
            else
                lp_tp_name = ip_tp_cur->ip_tp->ip_tp_name;
            SB_SA_Util_Debug::debug_printf(WHERE,
                                           "Sequencer. name=%s, RUN. cur-tp=%s, setting next-tp=%s\n",
                                           ip_seq_name, pp_tp->ip_tp_name, lp_tp_name);
        }
        if (ip_tp_cur != NULL)
            tp_unblock(ip_tp_cur->ip_tp);
        break;
    case TP_STATE_DONE:
        if (cv_debug) {
            SB_SA_Util_Debug::debug_printf(WHERE,
                                           "Sequencer. name=%s, DONE. cur-tp=%s\n",
                                           ip_seq_name, pp_tp->ip_tp_name);
        }
        break;
    default:
        SB_SA_util_assert_ige(iv_tp_state, TP_STATE_INIT);
        SB_SA_util_assert_ile(iv_tp_state, TP_STATE_DONE);
        break;
    }
}

//
// TP-sequencer block caller
//
void SB_TP_Seq::tp_block(TP *pp_tp) {
    const char *WHERE = "SB_TP_Seq::tp_block";
    TP_CV      *lp_cv;
    int         lv_tid;

    lv_tid = sb_sa_util_gettid();
    if (cv_debug)
        SB_SA_Util_Debug::debug_printf(WHERE,
                                       "Sequencer. name=%s, thread tid=%d, waiting on tp=%s\n",
                                       ip_seq_name, lv_tid, pp_tp->ip_tp_name);
    lp_cv = cv_alloc();        // setup to block this thread
    lp_cv->ip_tp = pp_tp;      // save tp
    lp_cv->iv_tid = lv_tid,    // save tid
    iv_mutex.unlock();         // unblock other threads waiting
    lp_cv->iv_cv.wait(true);   // wait for the tp
    iv_mutex.lock();           // get master lock back
    if (cv_debug) {
        SB_SA_Util_Debug::debug_printf(WHERE,
                                       "Sequencer. name=%s, thread tid=%d, resuming from tp=%s\n",
                                       ip_seq_name, lv_tid, pp_tp->ip_tp_name);
    }
    cv_free(lp_cv);            // free cv
}

//
// TP-sequencer print
//
void SB_TP_Seq::tp_print() {
    tp_print_file(stdout);
}

//
// TP-sequencer print to a file
//
void SB_TP_Seq::tp_print_file(FILE *pp_file) {
    TPL *lp_tp;
    int  lv_inx;

    fprintf(pp_file, "Sequencer. name=%s, tp-count=%d\n",
            ip_seq_name, iv_tp_count);
    lp_tp = ip_tp_head;
    for (lv_inx = 0; lp_tp != NULL; lv_inx++) {
        fprintf(pp_file, "  tp[%d]. name=%s\n",
                lv_inx, lp_tp->ip_tp->ip_tp_name);
        lp_tp = lp_tp->ip_tp_next;
    }
}

//
// TP-sequencer unblock waiter
//
void SB_TP_Seq::tp_unblock(TP *pp_tp) {
    const char *WHERE = "SB_TP_Seq::tp_unblock";
    TP_CV      *lp_cv;
    int         lv_slot;

    for (lv_slot = 0; lv_slot < TP_MAX_CV; lv_slot++) {
        lp_cv = &ia_cv_list[lv_slot];
        if (lp_cv->iv_inuse && (lp_cv->ip_tp == pp_tp)) {
            if (cv_debug)
                SB_SA_Util_Debug::debug_printf(WHERE,
                                               "Sequencer. name=%s, waking thread tid=%d, tp=%s\n",
                                               ip_seq_name, lp_cv->iv_tid, pp_tp->ip_tp_name);
            lp_cv->iv_cv.signal(true);
            break;
        }
    }
}

//
// TP-sequencer wait for TP
//
void SB_TP_Seq::tp_wait(TP *pp_tp) {
    iv_mutex.lock();

    switch (iv_tp_state) {
    case TP_STATE_INIT:
        if (ip_tp_head == NULL) {
        } else if (pp_tp == ip_tp_head->ip_tp) {
            tp_advance(pp_tp);
        } else
            tp_block(pp_tp);
        break;
    case TP_STATE_RUN:
        if (ip_tp_cur == NULL) {
        } else if (pp_tp == ip_tp_cur->ip_tp) {
            tp_advance(pp_tp);
        } else
            tp_block(pp_tp);
        break;
    case TP_STATE_DONE:
        break;
    default:
        SB_SA_util_assert_ige(iv_tp_state, TP_STATE_INIT);
        SB_SA_util_assert_ile(iv_tp_state, TP_STATE_DONE);
        break;
    }

    iv_mutex.unlock();
}
