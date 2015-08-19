//------------------------------------------------------------------
//
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

#ifndef __SB_MSEVENTMGR_INL_
#define __SB_MSEVENTMGR_INL_

//
// Purpose: event manager - change replies done
//
SB_INLINE int SB_Ms_Event_Mgr::change_replies_done(int pv_change, int pv_mask) {
    const char *WHERE = "EVENT::change_replies_done";
    int         lv_awake;
    int         lv_replies;

    SB_util_assert((pv_change == -1) || (pv_change == 1)); // sw fault
    if (pv_change < 0) // check before changing
        SB_util_assert_igt(iv_replies, 0); // sw fault
    else // check before changing
        SB_util_assert_ige(iv_replies, 0); // sw fault
    lv_replies = __sync_add_and_fetch_4(&iv_replies, pv_change);
    if (cv_trace_events)
        trace_where_printf(WHERE, "id=%ld, mgr=%p, replies=%d\n",
                           iv_id, pfp(this), iv_replies);
    SB_util_assert_ige(lv_replies, 0); // sw fault
    if (pv_mask && (lv_replies == 0)) {
        // inline clear_event
        lv_awake = __sync_and_and_fetch_4(&iv_awake, ~pv_mask);
    } else
        lv_awake = 0; // for compiler
    if (cv_trace_events && pv_mask && (lv_replies == 0)) {
        char la_event[100];
        get_event_str(la_event, lv_awake);
        trace_where_printf(WHERE, "id=%ld, mgr=%p, pin=%d, setting awake=%s\n",
                           iv_id, pfp(this), iv_pin, la_event);
    }
    return lv_replies;
}

//
// Purpose: event manager - clear event
//
SB_INLINE void SB_Ms_Event_Mgr::clear_event(int pv_mask) {
    const char *WHERE = "EVENT::clear_event";
    int         lv_awake;

    lv_awake = __sync_and_and_fetch_4(&iv_awake, ~pv_mask);
    if (cv_trace_events) {
        char la_event[100];
        get_event_str(la_event, lv_awake);
        trace_where_printf(WHERE, "id=%ld, mgr=%p, pin=%d, setting awake=%s\n",
                           iv_id, pfp(this), iv_pin, la_event);
    }
}

//
// Purpose: event manager - get event
//
SB_INLINE int SB_Ms_Event_Mgr::get_event(int pv_event) {
    const char *WHERE = "EVENT::get_event";
    int         lv_status;

    lv_status = iv_event_mutex.lock();
    SB_util_assert_ieq(lv_status, 0);
    int lv_match = pv_event & iv_awake;
    int lv_hi;
    if (lv_match) {
        lv_hi = lv_match >> 8;
        if (lv_hi)
            lv_hi = ca_hi[lv_hi] << 8;
        else
            lv_hi = ca_hi[lv_match];
        iv_awake = (~lv_hi) & iv_awake;
        iv_awake_event = lv_hi;
        if (cv_trace_events) {
            char la_awake[100];
            char la_hi[100];
            get_event_str(la_hi, lv_hi);
            get_event_str(la_awake, iv_awake);
            trace_where_printf(WHERE, "id=%ld, mgr=%p, pin=%d, returning event=%s, setting awake=%s\n",
                               iv_id, pfp(this), iv_pin, la_hi, la_awake);
        }
    } else {
        lv_hi = 0;
        if (cv_trace_events) {
            char la_awake[100];
            char la_event[100];
            get_event_str(la_event, pv_event);
            get_event_str(la_awake, iv_awake);
            trace_where_printf(WHERE, "id=%ld, mgr=%p, pin=%d, no match, event=%s, awake=%s\n",
                               iv_id, pfp(this), iv_pin, la_event, la_awake);
        }
    }
    lv_status = iv_event_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0);
    return lv_hi;
}

//
// Purpose: event manager - timed-out?
//
SB_INLINE bool SB_Ms_Event_Mgr::get_timedout(SB_Int64_Type *pp_curr_time) {
    const char *WHERE = "EVENT::get_timedout";
    long        lv_elapsed;

    switch (iv_wait_us) {
    case -2:
        if (cv_trace_events)
            trace_where_printf(WHERE, "id=%ld, mgr=%p, pin=%d, tic=-2, return true\n",
                               iv_id, pfp(this), iv_pin);
        return true; // don't wait
    case -1:
        return false; // wait forever
    default:
        if (*pp_curr_time == 0)
            *pp_curr_time = SB_Thread::Sthr::time_us();
        lv_elapsed = static_cast<long>(*pp_curr_time - iv_wait_start_time);
        bool lv_toret = (lv_elapsed >= iv_wait_us);
        if (cv_trace_events)
            trace_where_printf(WHERE, "id=%ld, mgr=%p, pin=%d, us=%ld, elapsed=%ld, ret=%d\n",
                               iv_id, pfp(this), iv_pin, iv_wait_us,
                               lv_elapsed, lv_toret);
        return lv_toret;
    }
}

//
// Purpose: event manager - return remaining time
//
SB_INLINE int SB_Ms_Event_Mgr::get_wait_time_rem(SB_Int64_Type pv_curr_time) {
    const char    *WHERE = "EVENT::get_wait_time_rem";
    int            lv_to;

    switch (iv_wait_us) {
    case -2:
        lv_to = -2;
        break;
    case -1:
        lv_to = -1;
        break;
    default:
        if (pv_curr_time == 0)
            pv_curr_time = SB_Thread::Sthr::time_us();
        lv_to =
          static_cast<int>(iv_wait_start_time + iv_wait_us - pv_curr_time);
        break;
    }
    if (cv_trace_events)
        trace_where_printf(WHERE, "id=%ld, mgr=%p, pin=%d, ret=%d\n",
                           iv_id, pfp(this), iv_pin, lv_to);
    return lv_to;
}

// pv_tics:
//   -2 check for event (don't wait)
//   -1 wait forever
//    0 use residual time value
//   >0 max time to wait in 10 ms units
//
// return us
SB_INLINE long SB_Ms_Event_Mgr::get_wait_time_start(int pv_tics) {
    const char *WHERE = "EVENT::get_wait_time_start";
    long        lv_to;

    switch (pv_tics) {
    case -2:
        lv_to = -2;
        break;
    case -1:
        lv_to = -1;
        break;
    case 0:
        lv_to = iv_wait_us;
        break;
    default:
        lv_to = pv_tics * 10 * SB_US_PER_MS; // convert to us
        break;
    }
    iv_wait_us = lv_to;
    if (cv_trace_events)
        trace_where_printf(WHERE, "id=%ld, mgr=%p, pin=%d, tics=%d, ret=%ld\n",
                           iv_id, pfp(this), iv_pin, pv_tics, lv_to);
    return lv_to;
}

//
// Purpose: event manager - set event
//
SB_INLINE void SB_Ms_Event_Mgr::set_event(int pv_event, bool *pp_done) {
    const char *WHERE = "EVENT::set_event";
    int         lv_awake;

    lv_awake = __sync_or_and_fetch_4(&iv_awake, pv_event);
    if (pp_done != NULL)
        *pp_done = true;
    iv_cv.signal(true); // need lock
    if (cv_trace_events) {
        char la_awake[100];
        char la_event[100];
        get_event_str(la_event, pv_event);
        get_event_str(la_awake, lv_awake);
        trace_where_printf(WHERE, "id=%ld, mgr=%p, group=%d, pin=%d, event=%s, new-awake=%s\n",
                           iv_id, pfp(this), iv_group, iv_pin,
                           la_event, la_awake);
    }
}

//
// Purpose: event manager - set event (atomic)
// TODO: make set_event atomic and remove set_event_atomic
//
SB_INLINE void SB_Ms_Event_Mgr::set_event_atomic(int pv_event, bool *pp_done) {
    const char *WHERE = "EVENT::set_event_atomic";
    int         lv_awake;
    int         lv_status;

    lv_awake = __sync_or_and_fetch_4(&iv_awake, pv_event);
    lv_status = iv_cv.lock();
    SB_util_assert_ieq(lv_status, 0);
    if (pp_done != NULL)
        *pp_done = true;
    iv_cv.signal(false); // no lock
    lv_status = iv_cv.unlock();
    SB_util_assert_ieq(lv_status, 0);
    if (cv_trace_events) {
        char la_awake[100];
        char la_event[100];
        get_event_str(la_event, pv_event);
        get_event_str(la_awake, lv_awake);
        trace_where_printf(WHERE, "id=%ld, mgr=%p, group=%d, pin=%d, event=%s, new-awake=%s\n",
                           iv_id, pfp(this), iv_group, iv_pin,
                           la_event, la_awake);
    }
}

//
// Purpose: event manager - set event via pin
//
SB_INLINE void SB_Ms_Event_Mgr::set_event_pin(int pv_pin, int pv_event, int pv_fun) {
    Map_Pin_Entry_Type *lp_pin_entry;
    // TODO: remove this do loop - added to work around dp2 problem on RH6
    do {
        lp_pin_entry =
          static_cast<Map_Pin_Entry_Type *>(cv_pin_map.get(pv_pin));
        if (lp_pin_entry == NULL) {
            usleep(1000);
            printf("waiting for pin=%d to register\n", pv_pin);
        }
    } while (lp_pin_entry == NULL);
    SB_util_assert_pne(lp_pin_entry, NULL); // sw fault
    if (pv_fun == 0)
        lp_pin_entry->ip_mgr->set_event(pv_event, NULL);
    else {
        int lv_group = lp_pin_entry->iv_group;
        Map_Group_Entry_Type *lp_group_entry =
          static_cast<Map_Group_Entry_Type *>(cv_group_map.get(lv_group));
        SB_util_assert_pne(lp_group_entry, NULL); // sw fault
        // iterate over pins in group
        SB_Imap_Enum lv_enum(lp_group_entry->ip_map);
        while (lv_enum.more()) {
            lp_pin_entry = reinterpret_cast<Map_Pin_Entry_Type *>(lv_enum.next());
            lp_pin_entry->ip_mgr->set_event(pv_event, NULL);
        }
    }
}

//
// Purpose: event manager - set trace-events
//
SB_INLINE void SB_Ms_Event_Mgr::set_trace_events(bool pv_trace_events) {
    cv_trace_events = pv_trace_events;
}

//
// Purpose: event manager - set wait start-time
//
SB_INLINE void SB_Ms_Event_Mgr::set_wait_start_time(int            pv_tics,
                                                    SB_Int64_Type *pp_curr_time) {
    const char *WHERE = "EVENT::set_wait_start_time";

    if (pv_tics >= 0) {
        iv_wait_start_time = SB_Thread::Sthr::time_us();
        if (cv_trace_events)
            trace_where_printf(WHERE, "id=%ld, mgr=%p, pin=%d, t=" PF64 "\n",
                               iv_id, pfp(this), iv_pin, iv_wait_start_time);
    } else
        iv_wait_start_time = 0;
    *pp_curr_time = iv_wait_start_time;
}

//
// Purpose: event manager - set wait time
//
SB_INLINE void SB_Ms_Event_Mgr::set_wait_time(int pv_tics) {
    const char *WHERE = "EVENT::set_wait_time";

    if (pv_tics >= 0) {
        SB_Int64_Type lv_curr_time = SB_Thread::Sthr::time_us();
        long lv_elapsed = static_cast<long>(lv_curr_time - iv_wait_start_time);
        iv_wait_us -= lv_elapsed;
        if (iv_wait_us < 0)
            iv_wait_us = -3;
        if (cv_trace_events)
            trace_where_printf(WHERE, "id=%ld, mgr=%p, pin=%d, elapsed=%ld, wait_us=%ld\n",
                               iv_id, pfp(this), iv_pin, lv_elapsed, iv_wait_us);
    } else {
        iv_wait_us = -3;
        if (cv_trace_events)
            trace_where_printf(WHERE, "id=%ld, mgr=%p, pin=%d, wait_us=%ld\n",
                               iv_id, pfp(this), iv_pin, iv_wait_us);
    }
}

//
// Purpose: event manager - wait
//
SB_INLINE void SB_Ms_Event_Mgr::wait(long pv_us) {
    const char *WHERE = "EVENT::wait";
    int         lv_status;

    if ((pv_us < -1) || (pv_us == 0))
        return;
    if (pv_us == -1) {
        if (cv_trace_events)
            trace_where_printf(WHERE, "id=%ld, mgr=%p, wait_us=%ld\n",
                               iv_id, pfp(this), pv_us);
        lv_status = iv_cv.wait(true); // need lock
        SB_util_assert_ieq(lv_status, 0);
    } else {
        int lv_us = static_cast<int>(pv_us % SB_US_PER_SEC);
        int lv_sec = static_cast<int>(pv_us - lv_us) / SB_US_PER_SEC;
        if (cv_trace_events)
            trace_where_printf(WHERE, "id=%ld, mgr=%p, sec=%d, us=%d\n",
                               iv_id, pfp(this), lv_sec, lv_us);
        lv_status = iv_cv.wait(true, lv_sec, lv_us); // need lock
        CHK_STATUSIGNORE(lv_status); // could be timeout
    }
    if (cv_trace_events)
        trace_where_printf(WHERE, "done id=%ld, mgr=%p\n",
                           iv_id, pfp(this));
}

//
// Purpose: thread-local event manager get gmgr
//
SB_INLINE SB_Ms_Event_Mgr *SB_Ms_Tl_Event_Mgr::get_gmgr() {
    const char *WHERE = "EVENT::get_gmgr";
    int         lv_status;

    if (cp_gmgr == NULL) {
        lv_status = cv_sl.lock();
        SB_util_assert_ieq(lv_status, 0);
        if (cp_gmgr == NULL) {
            cp_gmgr = new SB_Ms_Event_Mgr(0, -1);
            if (cv_trace_events)
                trace_where_printf(WHERE, "gmgr=%p, mgr=%p\n",
                                   pfp(cp_gmgr), pfp(this));
        }
        lv_status = cv_sl.unlock();
        SB_util_assert_ieq(lv_status, 0);
    }
    return cp_gmgr;
}

//
// Purpose: thread-local event manager get mgr
//
SB_INLINE SB_Ms_Event_Mgr *SB_Ms_Tl_Event_Mgr::get_mgr(bool *pp_first) {
    bool             lv_first;
    int              lv_status;
    SB_Ms_Event_Mgr *lp_mgr;

    if (cv_ret_gmgr) {
        if (cp_gmgr == NULL) {
            if (pp_first != NULL)
                *pp_first = true;
        } else {
            if (pp_first != NULL)
                *pp_first = false;
        }
        return get_gmgr();
    }
    lp_mgr = static_cast<SB_Ms_Event_Mgr *>(SB_Thread::Sthr::specific_get(cv_tls_inx));
    if (lp_mgr == NULL) {
        lv_first = true;
        lp_mgr = new SB_Ms_Event_Mgr(-1, cv_tls_inx);
        lv_status = SB_Thread::Sthr::specific_set(cv_tls_inx, lp_mgr);
        SB_util_assert_ieq(lv_status, 0);
    } else
        lv_first = false;
    if (pp_first != NULL)
        *pp_first = lv_first;
    return lp_mgr;
}

//
// Purpose: thread-local event manager get mgr via tid
//
SB_INLINE SB_Ms_Event_Mgr *SB_Ms_Tl_Event_Mgr::get_mgr_tid(int pv_tid) {
    SB_Ms_Event_Mgr::Map_All_Entry_Type *lp_entry;
    SB_Ms_Event_Mgr                     *lp_mgr;

    lp_entry =
      reinterpret_cast<SB_Ms_Event_Mgr::Map_All_Entry_Type *>
        (SB_Ms_Event_Mgr::cv_all_map.get(pv_tid));
    if (lp_entry == NULL)
        lp_mgr = NULL;
    else
        lp_mgr = lp_entry->ip_mgr;
    return lp_mgr;
}

//
// Purpose: thread-local event manager set gmgr
//
SB_INLINE void SB_Ms_Tl_Event_Mgr::set_ret_gmgr(bool pv_on) {
    cv_ret_gmgr = pv_on;
}

//
// Purpose: thread-local event manager - set trace-events
//
SB_INLINE void SB_Ms_Tl_Event_Mgr::set_trace_events(bool pv_trace_events) {
    cv_trace_events = pv_trace_events;
    SB_Ms_Event_Mgr::set_trace_events(pv_trace_events);
}

#endif // !__SB_MSEVENTMGR_INL_
