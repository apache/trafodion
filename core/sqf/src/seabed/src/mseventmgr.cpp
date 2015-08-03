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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "seabed/pevents.h"

#include "mseventmgr.h"
#include "mstrace.h"
#include "qid.h"
#include "threadtlsx.h"
#include "util.h"


static const char *ga_event_map[16] = {
    "PON",
    "IOPON",
    "INTR",
    "LINSP",
    "LCAN",
    "LDONE",
    "LTMF",
    "LREQ",
    "LSIG",
    "LPIPE",
    "LCHLD",
    "0x0010",
    "LRABBIT",
    "0x0004",
    "PWU",
    "LSEM"
};

int     SB_Ms_Event_Mgr::ca_hi[256] =
//   0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f
{ 0x00,0x01,0x02,0x02,0x04,0x04,0x04,0x04,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08, // 0
  0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10, // 1
  0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20, // 2
  0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20, // 3
  0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40, // 4
  0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40, // 5
  0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40, // 6
  0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40, // 7
  0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80, // 8
  0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80, // 9
  0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80, // a
  0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80, // b
  0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80, // c
  0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80, // d
  0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80, // e
  0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80  // f
};
SB_Lmap          SB_Ms_Event_Mgr::ca_reg_map[SB_Ms_Event_Mgr::EVENT_MAX];
SB_D_Queue       SB_Ms_Event_Mgr::cv_all_list(QID_EVENT_MGR, "q-event-mgr-all-list-event-mgr");
SB_Lmap          SB_Ms_Event_Mgr::cv_all_map("map-event-mgr-all");
SB_Ts_Imap       SB_Ms_Event_Mgr::cv_group_map("map-event-mgr-group");
SB_Imap          SB_Ms_Event_Mgr::cv_pin_map("map-event-mgr-pin");
SB_Thread::MSL   SB_Ms_Event_Mgr::cv_sl_map("sl-SB_Ms_Event_Mgr::cv_sl_map");
bool             SB_Ms_Event_Mgr::cv_trace_events = false;

SB_Ms_Event_Mgr *SB_Ms_Tl_Event_Mgr::cp_gmgr = NULL;
SB_Thread::MSL   SB_Ms_Tl_Event_Mgr::cv_sl;
bool             SB_Ms_Tl_Event_Mgr::cv_ret_gmgr = false;
int              SB_Ms_Tl_Event_Mgr::cv_tls_inx =
                   SB_create_tls_key(dtor, "eventmgr-mgr");
bool             SB_Ms_Tl_Event_Mgr::cv_trace_events = false;

#ifndef USE_SB_INLINE
#include "mseventmgr.inl"
#endif


//
// Purpose: constructor event-manager
//
SB_Ms_Event_Mgr::SB_Ms_Event_Mgr(long pv_id, int pv_tls_inx)
: ip_waiter_next(NULL), ip_waiter_prev(NULL),
  iv_awake(0), iv_awake_event(0), iv_mutex_locked(false), iv_replies(0),
  iv_tls_inx(pv_tls_inx),
  iv_wait_start_time(0), iv_wait_us(0) {
    const char *WHERE = "SB_Ms_Event_Mgr::SB_Ms_Event_Mgr";
    int         lv_inx;
    int         lv_status;

    iv_cv.setname("cv-SB_Ms_Event_Mgr::iv_cv");
    iv_event_mutex.setname("mutex-SB_Ms_Event_Mgr::iv_event_mutex");
    for (lv_inx = 0; lv_inx < TLS_DTOR_MAX; lv_inx++) {
        ia_tls_dtor[lv_inx].ip_dtor = NULL;
        ia_tls_dtor_data[lv_inx] = NULL;
    }

    iv_group = -1;
    iv_pin = -1;
    if (pv_id < 0)
        iv_id = SB_Thread::Sthr::self_id();
    else
        iv_id = pv_id;
    iv_all_list_entry.iv_link.iv_id.l = iv_id;
    iv_all_list_entry.ip_mgr = this;
    iv_all_map_entry.iv_link.iv_id.l = iv_id;
    iv_all_map_entry.ip_mgr = this;
    for (int lv_event = 0; lv_event < EVENT_MAX; lv_event++)
        memset(&ia_reg_entry[lv_event], 0, sizeof(ia_reg_entry[lv_event]));
    lv_status = cv_sl_map.lock();
    SB_util_assert_ieq(lv_status, 0);
    cv_all_list.add(&iv_all_list_entry.iv_link);
    SB_util_assert_peq(cv_all_map.get(iv_id), NULL);
    cv_all_map.put(reinterpret_cast<SB_LML_Type *>(&iv_all_map_entry.iv_link));
    lv_status = cv_sl_map.unlock();
    SB_util_assert_ieq(lv_status, 0);
    iv_group_entry.ip_map = NULL;
    if (gv_ms_trace_events)
        trace_where_printf(WHERE, "id=%ld, mgr=%p\n", iv_id, pfp(this));
}

//
// Purpose: destructor event-manager
//
SB_Ms_Event_Mgr::~SB_Ms_Event_Mgr() {
    const char *WHERE = "SB_Ms_Event_Mgr::~SB_Ms_Event_Mgr";
    int         lv_status;

    if (gv_ms_trace_events)
        trace_where_printf(WHERE, "ENTER id=%ld, mgr=%p\n",
                           iv_id, pfp(this));
    if (!iv_mutex_locked) {
        lv_status = cv_sl_map.lock();
        SB_util_assert_ieq(lv_status, 0);
    }
    cv_all_list.remove_list(&iv_all_list_entry.iv_link);
    cv_all_map.remove(iv_all_map_entry.iv_link.iv_id.l);
    if (!iv_mutex_locked) {
        lv_status = cv_sl_map.unlock();
        SB_util_assert_ieq(lv_status, 0);
    }
    for (int lv_event = 0; lv_event < EVENT_MAX; lv_event++)
        ca_reg_map[lv_event].remove(ia_reg_entry[lv_event].iv_link.iv_id.l);
    if (iv_pin >= 0) {
        Map_Pin_Entry_Type *lp_entry =
          static_cast<Map_Pin_Entry_Type *>(cv_pin_map.remove(iv_pin));
        SB_util_assert_peq(lp_entry, &iv_pin_entry); // sw fault
        lp_entry = lp_entry; // touch (in case assert disabled)
        if (gv_ms_trace_events)
            trace_where_printf(WHERE, "id=%ld, mgr=%p, deleting pin=%d\n",
                               iv_id, pfp(this), iv_pin);
    }
    if (iv_group_entry.ip_map != NULL)
        delete iv_group_entry.ip_map;
    // Clear TLS
    if (iv_tls_inx >= 0) {
        lv_status = SB_Thread::Sthr::specific_set(iv_tls_inx, NULL);
        SB_util_assert_ieq(lv_status, 0);
    }

    // destroy CV
    do {
        lv_status = iv_cv.destroy();
        if (lv_status == EBUSY)
            usleep(100);
    } while (lv_status);

    if (gv_ms_trace_events)
        trace_where_printf(WHERE, "EXIT id=%ld, mgr=%p\n",
                           iv_id, pfp(this));
}

//
// Purpose: event manager - call TLS dtors
//
void SB_Ms_Event_Mgr::call_tls_dtors() {
    const char *WHERE = "SB_Ms_Event_Mgr::call_tls_dtors";
    int         lv_inx;
    void       *lp_data;

    for (lv_inx = 0; lv_inx < TLS_DTOR_MAX; lv_inx++) {
        if (ia_tls_dtor[lv_inx].ip_dtor != NULL) {
            lp_data = ia_tls_dtor_data[lv_inx];
            if (gv_ms_trace_events)
                trace_where_printf(WHERE, "inx=%d, data=%p\n", lv_inx, lp_data);
            ia_tls_dtor[lv_inx].iv_dtor(lp_data);
        }
    }
}

//
// Purpose: event manager - de-register event
//
void SB_Ms_Event_Mgr::deregister_event(int pv_event) {
    const char *WHERE = "SB_Ms_Event_Mgr::deregister_event";

    if (gv_ms_trace_events)
        trace_where_printf(WHERE, "event=0x%x\n", pv_event);
    int lv_event;
    switch (pv_event) {
    case LREQ:
        lv_event = EVENT_LREQ;
        break;
    case LDONE:
        lv_event = EVENT_LDONE;
        break;
    default:
        lv_event = -1; // touch
        SB_util_abort("invalid pv_event"); // sw fault
    }
    // don't need to use cv_sl_map
    long lv_id = SB_Thread::Sthr::self_id();
    ca_reg_map[lv_event].remove(lv_id);
}

//
// Purpose: event manager - de-register TLS destructor
//
void SB_Ms_Event_Mgr::deregister_tls_dtor(int pv_inx) {
    const char *WHERE = "SB_Ms_Event_Mgr::deregister_tls_dtor";

    SB_util_assert_pne(ia_tls_dtor[pv_inx].ip_dtor, NULL);
    ia_tls_dtor[pv_inx].ip_dtor = NULL;
    if (gv_ms_trace_events)
        trace_printf(WHERE, "inx=%d\n", pv_inx);
}

//
// Purpose: event manager - get event string
//
void SB_Ms_Event_Mgr::get_event_str(char *pp_line, int pv_mask) {
    char         la_line[100];
    unsigned int lv_hi = 0x8000;

    la_line[0] = '\0';
    for (int lv_inx = 0; lv_inx < 16; lv_inx++) {
        if (lv_hi & pv_mask) {
            if (la_line[0])
                strcat(la_line, "|");
            strcat(la_line, ga_event_map[lv_inx]);
        }
        lv_hi = lv_hi >> 1;
    }
    sprintf(pp_line, "0x%x(%s)", pv_mask, la_line);
}

//
// Purpose: event manager - print (stdout)
//
void SB_Ms_Event_Mgr::print() {
    print(stdout);
}

//
// Purpose: event manager - print to file
//
void SB_Ms_Event_Mgr::print(FILE *pp_file) {
    char la_awake[100];

    get_event_str(la_awake, iv_awake);
    fprintf(pp_file, "group=%d, pin=%d, awake=%s\n", iv_group, iv_pin, la_awake);
    fprintf(pp_file, "wait_start_time=" PF64 ", wait_us=%ld\n",
            iv_wait_start_time, iv_wait_us);
}

//
// Purpose: event manager - register event
//
bool SB_Ms_Event_Mgr::register_event(int pv_event) {
    const char *WHERE = "SB_Ms_Event_Mgr::register_event";
    int         lv_status;

    if (gv_ms_trace_events)
        trace_where_printf(WHERE, "event=0x%x\n", pv_event);
    int lv_event;
    switch (pv_event) {
    case LREQ:
        lv_event = EVENT_LREQ;
        break;
    case LDONE:
        lv_event = EVENT_LDONE;
        break;
    default:
        lv_event = -1; // touch
        SB_util_abort("invalid pv_event"); // sw fault
    }
    long lv_id = SB_Thread::Sthr::self_id();
    Map_Reg_Entry_Type *lp_reg_entry =
      static_cast<Map_Reg_Entry_Type *>(ca_reg_map[lv_event].get(lv_id));
    if (lp_reg_entry == NULL) {
        ia_reg_entry[lv_event].iv_link.iv_id.l = lv_id;
        ia_reg_entry[lv_event].ip_mgr = this;
        lv_status = cv_sl_map.lock();
        SB_util_assert_ieq(lv_status, 0);
        ca_reg_map[lv_event].put(reinterpret_cast<SB_LML_Type *>(&ia_reg_entry[lv_event].iv_link));
        lv_status = cv_sl_map.unlock();
        SB_util_assert_ieq(lv_status, 0);
        return true;
    } else
        return false;
}

//
// Purpose: event manager - register event check
//
bool SB_Ms_Event_Mgr::register_event_check(int pv_event) {
    int lv_event;

    switch (pv_event) {
    case LREQ:
        lv_event = EVENT_LREQ;
        break;
    case LDONE:
        lv_event = EVENT_LDONE;
        break;
    default:
        lv_event = -1; // touch
        SB_util_abort("invalid pv_event"); // sw fault
    }
    Map_Reg_Entry_Type *lp_reg_entry =
      static_cast<Map_Reg_Entry_Type *>
        (ca_reg_map[lv_event].get(ia_reg_entry[lv_event].iv_link.iv_id.l));
    return (lp_reg_entry != NULL);
}

//
// Purpose: event manager - register group/pin
//
void SB_Ms_Event_Mgr::register_group_pin(int pv_group, int pv_pin) {
    const char *WHERE = "SB_Ms_Event_Mgr::register_group_pin";

    if (gv_ms_trace_events)
        trace_where_printf(WHERE, "id=%ld, mgr=%p, group=%d, pin=%d\n",
                           iv_id, pfp(this), pv_group, pv_pin);
    iv_group = pv_group;
    iv_pin = pv_pin;

    if (pv_group >= 0) {
        cv_group_map.lock();
        Map_Group_Entry_Type *lp_group_entry =
          static_cast<Map_Group_Entry_Type *>(cv_group_map.get_lock(pv_group, false));
        if (lp_group_entry == NULL) {
            iv_group_entry.iv_link.iv_id.i = pv_group;
            iv_group_entry.ip_map = new SB_Imap();
            cv_group_map.put_lock(&iv_group_entry.iv_link, false);
            lp_group_entry = &iv_group_entry;
        }
        cv_group_map.unlock();
        iv_group_pin_entry.iv_link.iv_id.i = pv_pin;
        iv_group_pin_entry.ip_mgr = this;
        lp_group_entry->ip_map->put(&iv_group_pin_entry.iv_link);
    }

    iv_pin_entry.iv_link.iv_id.i = pv_pin;
    iv_pin_entry.iv_group = pv_group;
    iv_pin_entry.ip_mgr = this;
    cv_pin_map.put(&iv_pin_entry.iv_link);
}

//
// Purpose: event manager - remove_from_event_all
//
void SB_Ms_Event_Mgr::remove_from_event_all() {
    const char *WHERE = "SB_Ms_Event_Mgr::remove_from_event_all";
    int         lv_status;

    if (gv_ms_trace_events)
        trace_where_printf(WHERE, "id=%ld, mgr=%p\n", iv_id, pfp(this));

    lv_status = cv_sl_map.lock();
    SB_util_assert_ieq(lv_status, 0);

    cv_all_list.remove_list(&iv_all_list_entry.iv_link);

    lv_status = cv_sl_map.unlock();
    SB_util_assert_ieq(lv_status, 0);
}

//
// Purpose: event manager - register tls destructor
//
void SB_Ms_Event_Mgr::register_tls_dtor(int    pv_inx,
                                        void (*pp_dtor)(void *pp_data),
                                        void  *pp_data) {
    const char *WHERE = "SB_Ms_Event_Mgr::register_tls_dtor";

    SB_util_assert_peq(ia_tls_dtor[pv_inx].ip_dtor, NULL);
    ia_tls_dtor[pv_inx].iv_dtor = pp_dtor;
    ia_tls_dtor_data[pv_inx] = pp_data;
    if (gv_ms_trace_events)
        trace_where_printf(WHERE, "inx=%d, data=%p\n", pv_inx, pp_data);
}

//
// Purpose: event manager - shutdown
//
void SB_Ms_Event_Mgr::shutdown() {
    bool lv_done = false;
    int  lv_status;

    lv_status = cv_sl_map.lock();
    SB_util_assert_ieq(lv_status, 0);
    do {
        SB_Lmap_Enum *lp_enum = cv_all_map.keys();
        if (lp_enum->more()) {
            Map_All_Entry_Type *lp_all_map_entry =
              reinterpret_cast<Map_All_Entry_Type *>(lp_enum->next());
            // entry will be removed from table on delete
            lp_all_map_entry->ip_mgr->iv_mutex_locked = true;
            delete lp_all_map_entry->ip_mgr;
        } else
            lv_done = true;
        delete lp_enum;
    } while (!lv_done);
    lv_status = cv_sl_map.unlock();
    SB_util_assert_ieq(lv_status, 0);
}

// ---------------------------------------------------------------

//
// Purpose: constructor thread-local event manager
//
SB_Ms_Tl_Event_Mgr::SB_Ms_Tl_Event_Mgr() {
}

//
// Purpose: destructor thread-local event manager
//
SB_Ms_Tl_Event_Mgr::~SB_Ms_Tl_Event_Mgr() {
}

//
// Purpose: thread-local event manager destructor
//
void SB_Ms_Tl_Event_Mgr::dtor(void *pp_mgr) {
    SB_Ms_Event_Mgr *lp_mgr = static_cast<SB_Ms_Event_Mgr *>(pp_mgr);
    if (lp_mgr != NULL) {
        // only delete if it hasn't already been deleted
        long lv_id = SB_Thread::Sthr::self_id();
        if (SB_Ms_Event_Mgr::cv_all_map.get(lv_id) != NULL)
        {
            int lv_status = SB_Thread::Sthr::specific_set(lp_mgr->iv_tls_inx, pp_mgr); // put it back!
            SB_util_assert_ieq(lv_status, 0);
            lp_mgr->call_tls_dtors();
            delete lp_mgr;
        }
    }
}

//
// Purpose: thread-local event manager remove local thread mgr from all
//
void SB_Ms_Tl_Event_Mgr::remove_from_event_all() {
    SB_Ms_Event_Mgr *lp_mgr;

    lp_mgr = get_mgr(NULL);
    lp_mgr->remove_from_event_all();
}

//
// Purpose: thread-local event manager set event (all)
//
void SB_Ms_Tl_Event_Mgr::set_event_all(int pv_event) {
    const char *WHERE = "SB_Ms_Event_Mgr::set_event_all";
    int         lv_status;

    SB_Ms_Event_Mgr::Map_All_Entry_Type *lp_all_list_entry;

    lv_status = SB_Ms_Event_Mgr::cv_sl_map.lock();
    SB_util_assert_ieq(lv_status, 0);
    lp_all_list_entry =
      reinterpret_cast<SB_Ms_Event_Mgr::Map_All_Entry_Type *>
        (SB_Ms_Event_Mgr::cv_all_list.head());
    if (gv_ms_trace_events && (lp_all_list_entry == NULL))
        trace_where_printf(WHERE, "all=EMPTY\n");
    while (lp_all_list_entry != NULL) {
        if (gv_ms_trace_events)
            trace_where_printf(WHERE, "id=%ld, mgr=%p\n",
                               lp_all_list_entry->iv_link.iv_id.l,
                               pfp(lp_all_list_entry->ip_mgr));
        lp_all_list_entry->ip_mgr->set_event(pv_event, NULL);
        lp_all_list_entry =
          reinterpret_cast<SB_Ms_Event_Mgr::Map_All_Entry_Type *>
            (lp_all_list_entry->iv_link.ip_next);
    }
    lv_status = SB_Ms_Event_Mgr::cv_sl_map.unlock();
    SB_util_assert_ieq(lv_status, 0);
}

//
// Purpose: thread-local event manager set event (registered)
//
void SB_Ms_Tl_Event_Mgr::set_event_reg(int pv_event, bool *pp_done) {
    const char *WHERE = "SB_Ms_Event_Mgr::set_event_reg";
    int         lv_status;

    int lv_event;
    switch (pv_event) {
    case LREQ:
        lv_event = SB_Ms_Event_Mgr::EVENT_LREQ;
        break;
    case LDONE:
        lv_event = SB_Ms_Event_Mgr::EVENT_LDONE;
        break;
    default:
        lv_event = -1; // touch
        SB_util_abort("invalid pv_event"); // sw fault
    }
    lv_status = SB_Ms_Event_Mgr::cv_sl_map.lock();
    SB_util_assert_ieq(lv_status, 0);
    SB_Lmap_Enum lv_enum(&SB_Ms_Event_Mgr::ca_reg_map[lv_event]);
    while (lv_enum.more()) {
        SB_Ms_Event_Mgr::Map_Reg_Entry_Type *lp_reg_entry =
          reinterpret_cast<SB_Ms_Event_Mgr::Map_Reg_Entry_Type *>(lv_enum.next());
        if (gv_ms_trace_events)
            trace_where_printf(WHERE, "id=%ld, mgr=%p\n",
                               lp_reg_entry->iv_link.iv_id.l,
                               pfp(lp_reg_entry->ip_mgr));
        lp_reg_entry->ip_mgr->set_event(pv_event, pp_done);
    }
    lv_status = SB_Ms_Event_Mgr::cv_sl_map.unlock();
    SB_util_assert_ieq(lv_status, 0);
}

