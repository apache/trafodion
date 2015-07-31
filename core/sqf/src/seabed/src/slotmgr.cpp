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

//
// Implement slot manager
//

#include <stdio.h>
#include <string.h>

#include "buf.h"
#include "slotmgr.h"
#include "threadtlsx.h"
#include "util.h"


#ifndef USE_SB_INLINE
#include "slotmgr.inl"
#endif

int SB_Ts_Slot_Mgr::cv_tls_inx =
      SB_create_tls_key(dtor, "slotmgr-lock");
static void *gp_sb_slotmgr_lock = reinterpret_cast<void *>(1);
static void *gp_sb_slotmgr_unlock = NULL;

static void print_str(FILE *pp_f, char *pp_str) {
    fprintf(pp_f, pp_str);
}

SB_Slot_Mgr::SB_Slot_Mgr(const char *pp_name, Alloc_Type pv_alloc, int pv_cap)
: iv_aecid_slot_mgr(SB_ECID_SLOTMGR),
  iv_alloc(pv_alloc), iv_cap(pv_cap), iv_change(false),
  iv_free(pv_cap), iv_head(0), iv_max(-1), iv_size(0), iv_tail(-1) {
    int lv_slot;

    ia_slotmgr_name[sizeof(ia_slotmgr_name) - 1] = '\0';
    SB_util_assert_cpne(pp_name, NULL);
    strncpy(ia_slotmgr_name, pp_name, sizeof(ia_slotmgr_name) - 1);

    SB_util_assert_ige(pv_cap, 1); // sw fault
    ip_slots = new int[pv_cap+1];
    switch (pv_alloc) {
    case ALLOC_FAST:
    case ALLOC_MIN:
        for (lv_slot = 0; lv_slot < pv_cap+1; lv_slot++)
            ip_slots[lv_slot] = lv_slot + 1;
        break;
    case ALLOC_FIFO:
        for (lv_slot = 0; lv_slot < pv_cap+1; lv_slot++)
            ip_slots[lv_slot] = lv_slot + 1;
        iv_tail = pv_cap - 1;
        ip_slots[iv_tail] = SLOT_TAIL;
        break;
    case ALLOC_SCAN:
        for (lv_slot = 0; lv_slot < pv_cap+1; lv_slot++)
            ip_slots[lv_slot] = SLOT_FREE;
        break;
    default:
        SB_util_abort("invalid pv_alloc"); // sw fault
        break;
    }
    ip_slots[pv_cap] = SLOT_TAIL;
}

SB_Slot_Mgr::~SB_Slot_Mgr() {
    delete [] ip_slots;
}

void SB_Slot_Mgr::print(int pv_print_type) {
    print(stdout, pv_print_type);
}

void SB_Slot_Mgr::print(FILE *pp_f, int pv_print_type) {
    print(pp_f, print_str, pv_print_type);
}

void SB_Slot_Mgr::print(FILE *pp_f, Cb_Type pv_cb, int pv_print_type) {
    SB_util_assert_ige(pv_print_type, PRINT_ALL); // sw fault
    SB_util_assert_ile(pv_print_type, PRINT_FREE); // sw fault
    SB_Buf_Line la_line;
    const char *lp_slot_type;
    switch (pv_print_type) {
    case PRINT_ALL:
        lp_slot_type = "ALL";
        break;
    case PRINT_FREE:
        lp_slot_type = "FREE";
        break;
    case PRINT_USED:
        lp_slot_type = "USED";
        break;
    default:
        lp_slot_type = "<unknown>";
        break;
    }
    sprintf(la_line, "name=%s, type=%s, cap=%d, free=%d, used=%d, free-head=%d, free-tail=%d\n",
            ia_slotmgr_name, lp_slot_type, iv_cap, iv_free,
            iv_cap - iv_free, iv_head, iv_tail);
    pv_cb(pp_f, la_line);
    int *lp_slot;
    int lv_slot;
    switch (pv_print_type) {
    case PRINT_ALL:
        for (lv_slot = 0, lp_slot = ip_slots;
             lv_slot < iv_cap;
             lv_slot++, lp_slot++)
            print_slot(pp_f, pv_cb, lv_slot, lp_slot);
        break;
    case PRINT_FREE:
        switch (iv_alloc) {
        case ALLOC_FAST:
        case ALLOC_MIN:
            for (lv_slot = iv_head;;) {
                SB_util_assert_ige(lv_slot, 0); // sw fault
                SB_util_assert_ile(lv_slot, iv_cap); // sw fault
                lp_slot = &ip_slots[lv_slot];
                if (*lp_slot == SLOT_TAIL)
                    break;
                print_slot(pp_f, pv_cb, lv_slot, lp_slot);
                lv_slot = *lp_slot;
            }
            break;
        case ALLOC_FIFO:
            if (iv_head != SLOT_TAIL) {
                for (lv_slot = iv_head;;) {
                    SB_util_assert_ige(lv_slot, 0); // sw fault
                    SB_util_assert_ile(lv_slot, iv_cap); // sw fault
                    lp_slot = &ip_slots[lv_slot];
                    print_slot(pp_f, pv_cb, lv_slot, lp_slot);
                    lv_slot = *lp_slot;
                    if (lv_slot == SLOT_TAIL)
                        break;
                }
            }
            break;
        case ALLOC_SCAN:
            for (lv_slot = 0, lp_slot = ip_slots;
                 lv_slot < iv_cap;
                 lv_slot++, lp_slot++)
                if (*lp_slot == SLOT_FREE)
                    print_slot(pp_f, pv_cb, lv_slot, lp_slot);
            break;
        default:
            SB_util_abort("invalid iv_alloc"); // sw fault
            break;
        }
        break;
    case PRINT_USED:
        for (lv_slot = 0, lp_slot = ip_slots;
             lv_slot < iv_cap;
             lv_slot++, lp_slot++)
            if (*lp_slot == SLOT_USED)
                print_slot(pp_f, pv_cb, lv_slot, lp_slot);
        break;
    }
}

void SB_Slot_Mgr::print_slot(FILE    *pp_f,
                             Cb_Type  pv_cb,
                             int      pv_slot,
                             int     *pp_slot) {
    SB_Buf_Line la_line;

    sprintf(la_line, "slot[%d].free=%d\n", pv_slot, *pp_slot);
    pv_cb(pp_f, la_line);
}

SB_Ts_Slot_Mgr::SB_Ts_Slot_Mgr(const char *pp_name,
                               Alloc_Type  pv_alloc,
                               int         pv_cap)
:  SB_Slot_Mgr(pp_name, pv_alloc, pv_cap) {
    int lv_status;

    iv_lock.setname(pp_name);
    lv_status = SB_Thread::Sthr::specific_set(cv_tls_inx, gp_sb_slotmgr_lock);
    SB_util_assert_ieq(lv_status, 0);
}

void SB_Ts_Slot_Mgr::dtor(void *) {
}

void SB_Ts_Slot_Mgr::print(int pv_print_type) {
    int lv_status;

    lock();
    lv_status =
      SB_Thread::Sthr::specific_set(cv_tls_inx, gp_sb_slotmgr_unlock);
    SB_util_assert_ieq(lv_status, 0);
    SB_Slot_Mgr::print(pv_print_type);
    lv_status =
      SB_Thread::Sthr::specific_set(cv_tls_inx, gp_sb_slotmgr_lock);
    SB_util_assert_ieq(lv_status, 0);
    unlock();
}

void SB_Ts_Slot_Mgr::print(FILE *pp_f, int pv_print_type) {
    void *lp_lock;

    lp_lock = SB_Thread::Sthr::specific_get(cv_tls_inx);
    if (lp_lock == gp_sb_slotmgr_lock)
        lock();
    SB_Slot_Mgr::print(pp_f, pv_print_type);
    if (lp_lock == gp_sb_slotmgr_lock)
        unlock();
}

void SB_Ts_Slot_Mgr::print(FILE *pp_f, Cb_Type pv_cb, int pv_print_type) {
    void *lp_lock;

    lp_lock = SB_Thread::Sthr::specific_get(cv_tls_inx);
    if (lp_lock == gp_sb_slotmgr_lock)
        lock();
    SB_Slot_Mgr::print(pp_f, pv_cb, pv_print_type);
    if (lp_lock == gp_sb_slotmgr_lock)
        unlock();
}

