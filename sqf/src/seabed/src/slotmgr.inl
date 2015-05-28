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

#include "util.h"

SB_INLINE int SB_Slot_Mgr::alloc() {
    int lv_head;
    int lv_slot;

    switch (iv_alloc) {
    case ALLOC_FAST:
    case ALLOC_MIN:
        lv_slot = iv_head;
        lv_head = ip_slots[lv_slot];
        SB_util_assert_ige(lv_head, 0); // sw fault
        iv_head = lv_head;
        ip_slots[lv_slot] = SLOT_USED;
        break;
    case ALLOC_FIFO:
        SB_util_assert_ige(iv_head, 0); // sw fault
        lv_slot = iv_head;
        lv_head = ip_slots[lv_slot];
        if (lv_head == SLOT_TAIL)
            iv_tail = SLOT_TAIL;
        else
            SB_util_assert_ige(lv_head, 0); // sw fault
        iv_head = lv_head;
        ip_slots[lv_slot] = SLOT_USED;
        break;
    case ALLOC_SCAN:
        SB_util_assert_igt(iv_free, 0); // sw fault
        for (lv_slot = 0; lv_slot < iv_cap; lv_slot++) {
            if (ip_slots[lv_slot] == SLOT_FREE) {
                ip_slots[lv_slot] = SLOT_USED;
                break;
            }
        }
        break;
    default:
        lv_slot = 0; // fix compiler warning
        SB_util_abort("invalid iv_alloc"); // sw fault
        break;
    }
    iv_free--;
    iv_size++;
    if (lv_slot > iv_max)
        iv_max = lv_slot;
    if (iv_size > iv_hi)
        iv_hi = iv_size;
    return lv_slot;
}

SB_INLINE int SB_Slot_Mgr::alloc_if_cap() {
    if (iv_size < iv_cap)
        return SB_Slot_Mgr::alloc();
    else
        return -1;
}

SB_INLINE void SB_Slot_Mgr::check_min() {
    SB_util_assert_ieq(iv_alloc, ALLOC_MIN);
    int lv_next;
    int lv_slot;

    for (lv_slot = iv_head; lv_slot != SLOT_TAIL; lv_slot = lv_next) {
        SB_util_assert_ige(lv_slot, 0); // sw fault
        SB_util_assert_ile(lv_slot, iv_cap); // sw fault
        lv_next = ip_slots[lv_slot];
        if (lv_next != SLOT_TAIL)
            SB_util_assert_ilt(lv_slot, lv_next);
    }
}

SB_INLINE void SB_Slot_Mgr::free_slot(int pv_slot) {
    SB_util_assert_ige(pv_slot, 0); // sw fault
    SB_util_assert_ilt(pv_slot, iv_cap); // sw fault
    SB_util_assert_ieq(ip_slots[pv_slot], SLOT_USED); // sw fault

    int lv_next;
    int lv_prev;

    switch (iv_alloc) {
    case ALLOC_FAST:
        ip_slots[pv_slot] = iv_head;
        iv_head = pv_slot;
        break;
    case ALLOC_SCAN:
        ip_slots[pv_slot] = SLOT_FREE;
        break;
    case ALLOC_MIN:
        if (pv_slot < iv_head) {
            // slot is lower - make it the head
            ip_slots[pv_slot] = iv_head;
            iv_head = pv_slot;
        } else {
            // slot is higher - place in order
            lv_prev = -1;
            for (lv_next = iv_head;
                 lv_next != SLOT_TAIL;
                 lv_next = ip_slots[lv_next]) {
                SB_util_assert_ige(lv_next, 0); // sw fault
                SB_util_assert_ile(lv_next, iv_cap); // sw fault
                if (pv_slot < lv_next) {
                    ip_slots[pv_slot] = ip_slots[lv_prev];
                    ip_slots[lv_prev] = pv_slot;
                    break;
                }
                lv_prev = lv_next;
            }
            SB_util_assert_ine(lv_next, SLOT_TAIL); // always inserted
        }
        break;
    case ALLOC_FIFO:
        if (iv_tail == SLOT_TAIL) {
            iv_head = pv_slot;
            lv_next = SLOT_TAIL;
        } else {
            lv_next = ip_slots[iv_tail];
            ip_slots[iv_tail] = pv_slot;
        }
        ip_slots[pv_slot] = lv_next;
        iv_tail = pv_slot;
        break;
    default:
        SB_util_abort("invalid iv_alloc"); // sw fault
        break;
    }
    iv_free++;
    iv_size--;
    iv_change = true;
}

SB_INLINE int SB_Slot_Mgr::get_cap() {
    return iv_cap;
}

SB_INLINE int SB_Slot_Mgr::hi() {
    return iv_hi;
}

SB_INLINE bool SB_Slot_Mgr::inuse(int pv_slot) {
    SB_util_assert_ige(pv_slot, 0); // sw fault
    SB_util_assert_ilt(pv_slot, iv_cap); // sw fault
    return (ip_slots[pv_slot] == SLOT_USED);
}

SB_INLINE int SB_Slot_Mgr::max_slot() {
    int lv_slot;

    if (iv_change) {
        iv_change = false;
        for (lv_slot = iv_max; lv_slot >= 0; lv_slot--) {
            iv_max = lv_slot;
            if (ip_slots[lv_slot] == SLOT_USED)
                break;
        }
        if ((iv_max == 0) && (ip_slots[0] == SLOT_FREE))
            iv_max = -1;
    }
    return iv_max;
}

SB_INLINE void SB_Slot_Mgr::resize(int pv_cap) {
    int *lp_slots;
    int  lv_slot;

    if (pv_cap > iv_cap) {
        // alloc new, copy, delete, restore, fixup
        lp_slots = new int[pv_cap+1];
        for (lv_slot = 0; lv_slot < iv_cap; lv_slot++)
            lp_slots[lv_slot] = ip_slots[lv_slot];
        delete [] ip_slots;
        ip_slots = lp_slots;
        switch (iv_alloc) {
        case ALLOC_FAST:
        case ALLOC_MIN:
            //
            // cap=4
            // +---+---+---+---+---+
            // | 1 | 2 | 3 | 4 | T |
            // +---+---+---+---+---+
            //   0   1   2   3   4
            //   H               F
            //
            // cap=6
            // +---+---+---+---+---+---+---+
            // | 1 | 2 | 3 | 4 | 5 | 6 | T |
            // +---+---+---+---+---+---+---+
            //   0   1   2   3   4   5   6
            //   H                       F
            //
            for (lv_slot = iv_cap; lv_slot < pv_cap+1; lv_slot++)
                ip_slots[lv_slot] = lv_slot + 1;
            break;
        case ALLOC_FIFO:
            //
            // cap=4
            // +---+---+---+---+---+
            // | 1 | 2 | 3 | T | T |
            // +---+---+---+---+---+
            //   0   1   2   3   4
            //   H           T
            //
            // cap=6
            // +---+---+---+---+---+---+---+
            // | 1 | 2 | 3 | 4 | 5 | T | T |
            // +---+---+---+---+---+---+---+
            //   0   1   2   3   4   5   6
            //   H                   T
            //
            for (lv_slot = iv_cap; lv_slot < pv_cap+1; lv_slot++)
                ip_slots[lv_slot] = lv_slot + 1;
            if (iv_tail == SLOT_TAIL) // no tail
                iv_head = iv_cap;
            else
                ip_slots[iv_tail] = iv_cap; // fix old tail
            iv_tail = pv_cap - 1;
            ip_slots[iv_tail] = SLOT_TAIL;
            break;
        case ALLOC_SCAN:
            //
            // cap=4
            // +---+---+---+---+---+
            // | F | F | F | F | T |
            // +---+---+---+---+---+
            //   0   1   2   3   4
            //   H
            //
            // cap=6
            // +---+---+---+---+---+---+---+
            // | F | F | F | F | F | F | T |
            // +---+---+---+---+---+---+---+
            //   0   1   2   3   4   5   6
            //   H
            //
            for (lv_slot = iv_cap; lv_slot < pv_cap+1; lv_slot++)
                ip_slots[lv_slot] = SLOT_FREE;
            break;
        default:
            SB_util_abort("invalid iv_alloc"); // sw fault
            break;
        }
        ip_slots[pv_cap] = SLOT_TAIL;
        iv_free += (pv_cap - iv_cap);
        iv_cap = pv_cap;
    }
}

SB_INLINE int SB_Slot_Mgr::size() {
    return iv_size;
}

SB_INLINE int SB_Ts_Slot_Mgr::alloc() {
    lock();
    int lv_slot = SB_Slot_Mgr::alloc();
    unlock();
    return lv_slot;
}

SB_INLINE int SB_Ts_Slot_Mgr::alloc_if_cap() {
    lock();
    int lv_slot = SB_Slot_Mgr::alloc_if_cap();
    unlock();
    return lv_slot;
}

SB_INLINE void SB_Ts_Slot_Mgr::check_min() {
    lock();
    SB_Slot_Mgr::check_min();
    unlock();
}

SB_INLINE void SB_Ts_Slot_Mgr::free_slot(int pv_slot) {
    lock();
    SB_Slot_Mgr::free_slot(pv_slot);
    unlock();
}

SB_INLINE void SB_Ts_Slot_Mgr::lock() {
    int lv_status;

    lv_status = iv_lock.lock();
    SB_util_assert_ieq(lv_status, 0);
}

SB_INLINE int SB_Ts_Slot_Mgr::max_slot() {
    lock();
    int lv_slot = SB_Slot_Mgr::max_slot();
    unlock();
    return lv_slot;
}

SB_INLINE void SB_Ts_Slot_Mgr::resize(int pv_cap) {
    lock();
    SB_Slot_Mgr::resize(pv_cap);
    unlock();
}

SB_INLINE void SB_Ts_Slot_Mgr::unlock() {
    int lv_status;

    lv_status = iv_lock.unlock();
    SB_util_assert_ieq(lv_status, 0);
}

