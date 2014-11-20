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

#ifndef __SB_QUEUE_INL_
#define __SB_QUEUE_INL_

inline void DEC_COUNT(int &pr_count) { pr_count--; }
inline void INC_COUNT(int &pr_count) { pr_count++; }


SB_INLINE void sb_queue_ql_init(SB_QL_Type *pp_ql) {
    pp_ql->ip_next = NULL;
    pp_ql->ip_prev = NULL;
    pp_ql->iv_qid = QID_NONE;
    pp_ql->iv_qid_last = QID_NONE;
    pp_ql->iv_id.ll = 0;
}

SB_INLINE void sb_queue_dql_init(SB_DQL_Type *pp_dql) {
    pp_dql->ip_next = NULL;
    pp_dql->ip_prev = NULL;
    pp_dql->iv_qid = QID_NONE;
    pp_dql->iv_qid_last = QID_NONE;
    pp_dql->iv_id.ll = 0;
}

SB_INLINE SB_Queue::~SB_Queue() {
}

SB_INLINE void SB_Queue::add(SB_QL_Type *pp_item) {
    pp_item->ip_next = NULL;
    if (ip_tail != NULL)
        ip_tail->ip_next = pp_item;
    else
        ip_head = pp_item;
    ip_tail = pp_item;
    INC_COUNT(iv_count);
    if (iv_count > iv_hi)
        iv_hi = iv_count;
}

SB_INLINE void SB_Queue::add_at_front(SB_QL_Type *pp_item) {
    pp_item->ip_next = ip_head;
    if (ip_tail == NULL)
        ip_tail = pp_item;
    ip_head = pp_item;
    INC_COUNT(iv_count);
    if (iv_count > iv_hi)
        iv_hi = iv_count;
}

SB_INLINE bool SB_Queue::empty() {
    return (iv_count == 0);
}

SB_INLINE int SB_Queue::hi() {
    return iv_hi;
}

SB_INLINE void *SB_Queue::remove() {
    SB_QL_Type *lp_item = ip_head;
    if (ip_head != NULL) {
        ip_head = lp_item->ip_next;
        lp_item->ip_next = NULL;
        if (ip_head == NULL)
            ip_tail = NULL;
        DEC_COUNT(iv_count);
        SB_util_assert_ige(iv_count, 0);
    }
    return lp_item;
}

SB_INLINE int SB_Queue::size() {
    return iv_count;
}

// SB_D_Queue destructor
SB_INLINE SB_D_Queue::~SB_D_Queue() {
}

//
// Add item
//
SB_INLINE void SB_D_Queue::add(SB_DQL_Type *pp_item) {
    pp_item->ip_prev = ip_tail;
    pp_item->ip_next = NULL;
    pp_item->iv_qid = iv_qid;
    if (ip_tail == NULL)
        ip_head = pp_item;
    else
        ip_tail->ip_next = pp_item;
    ip_tail = pp_item;
    INC_COUNT(iv_count);
    if (iv_count > iv_hi)
        iv_hi = iv_count;
}

SB_INLINE void SB_D_Queue::add_at_front(SB_DQL_Type *pp_item) {
    if (ip_head != NULL)
        ip_head->ip_prev = pp_item;
    if (ip_tail == NULL)
        ip_tail = pp_item;
    pp_item->ip_prev = NULL;
    pp_item->ip_next = ip_head;
    pp_item->iv_qid = iv_qid;
    ip_head = pp_item;
    INC_COUNT(iv_count);
    if (iv_count > iv_hi)
        iv_hi = iv_count;
}

SB_INLINE void SB_D_Queue::add_list(SB_DQL_Type *pp_prev, SB_DQL_Type *pp_item) {
    pp_item->ip_prev = pp_prev;
    pp_item->ip_next = pp_prev->ip_next;
    pp_item->iv_qid = iv_qid;
    if (pp_prev->ip_next != NULL)
        pp_prev->ip_next->ip_prev = pp_item;
    else
        ip_tail = pp_item;
    pp_prev->ip_next = pp_item;
    INC_COUNT(iv_count);
    if (iv_count > iv_hi)
        iv_hi = iv_count;
}

SB_INLINE bool SB_D_Queue::empty() {
    return (iv_count == 0);
}

SB_INLINE SB_DQL_Type *SB_D_Queue::head() {
    return ip_head;
}

SB_INLINE int SB_D_Queue::hi() {
    return iv_hi;
}

//
// Remove item
//
SB_INLINE void *SB_D_Queue::remove() {
    SB_DQL_Type *lp_item;

    lp_item = ip_head;
    if (ip_head != NULL) {
        ip_head = lp_item->ip_next;
        if (ip_head == NULL)
            ip_tail = NULL;
        else
            ip_head->ip_prev = NULL;
        lp_item->ip_next = NULL;
        lp_item->ip_prev = NULL;
        lp_item->iv_qid_last = lp_item->iv_qid;
        lp_item->iv_qid = QID_NONE;
        DEC_COUNT(iv_count);
        SB_util_assert_ige(iv_count, 0);
    }
    return lp_item;
}

//
// Remove item from list
//
SB_INLINE bool SB_D_Queue::remove_list(SB_DQL_Type *pp_item) {
    bool lv_on_list;

    if (pp_item->iv_qid == iv_qid) {
        if (ip_head == pp_item)
            ip_head = pp_item->ip_next;
        if (ip_tail == pp_item)
            ip_tail = pp_item->ip_prev;
        if (pp_item->ip_prev != NULL)
            pp_item->ip_prev->ip_next = pp_item->ip_next;
        if (pp_item->ip_next != NULL)
            pp_item->ip_next->ip_prev = pp_item->ip_prev;
        pp_item->ip_prev = NULL;
        pp_item->ip_next = NULL;
        pp_item->iv_qid_last = pp_item->iv_qid;
        pp_item->iv_qid = QID_NONE;
        DEC_COUNT(iv_count);
        SB_util_assert_ige(iv_count, 0);
        lv_on_list = true;
    } else
        lv_on_list = false;
    return lv_on_list;
}

SB_INLINE int SB_D_Queue::size() {
    return iv_count;
}

// SB_Ts_Queue destructor
SB_INLINE SB_Ts_Queue::~SB_Ts_Queue() {
}

//
// Add item
//
SB_INLINE void SB_Ts_Queue::add(SB_QL_Type *pp_item) {
    lock();
    SB_Queue::add(pp_item);
    unlock();
}

//
// Add item at head
//
SB_INLINE void SB_Ts_Queue::add_at_front(SB_QL_Type *pp_item) {
    lock();
    SB_Queue::add_at_front(pp_item);
    unlock();
}

//
// Lock queue (can be called by client)
//
SB_INLINE void SB_Ts_Queue::lock() {
    int lv_status;

    lv_status = iv_cv.lock();
    SB_util_assert_ieq(lv_status, 0);
}

//
// Remove item
// Suspend caller if queue is empty
//
SB_INLINE void *SB_Ts_Queue::remove() {
    lock();
    void *lp_item = SB_Queue::remove();
    unlock();
    return lp_item;
}

//
// Print queue
//
SB_INLINE void SB_Ts_Queue::printself(bool pv_traverse) {
    lock();
    SB_Queue::printself(pv_traverse);
    unlock();
}

//
// Unlock queue (can be called by client)
//
SB_INLINE void SB_Ts_Queue::unlock() {
    int lv_status;

    lv_status = iv_cv.unlock();
    SB_util_assert_ieq(lv_status, 0);
}

// SB_Ts_D_Queue destructor
SB_INLINE SB_Ts_D_Queue::~SB_Ts_D_Queue() {
}

//
// Add item
//
SB_INLINE void SB_Ts_D_Queue::add(SB_DQL_Type *pp_item) {
    lock();
    SB_D_Queue::add(pp_item);
    unlock();
}

//
// Add item at head
//
SB_INLINE void SB_Ts_D_Queue::add_at_front(SB_DQL_Type *pp_item) {
    lock();
    SB_D_Queue::add_at_front(pp_item);
    unlock();
}

//
// Add item to list
//
SB_INLINE void SB_Ts_D_Queue::add_list(SB_DQL_Type *pp_prev, SB_DQL_Type *pp_item) {
    lock();
    SB_D_Queue::add_list(pp_prev, pp_item);
    unlock();
}

//
// Lock queue (can be called by client)
//
SB_INLINE void SB_Ts_D_Queue::lock() {
    int lv_status;

    lv_status = iv_cv.lock();
    SB_util_assert_ieq(lv_status, 0);
}

//
// Remove item
// Suspend caller if queue is empty
//
SB_INLINE void *SB_Ts_D_Queue::remove() {
    lock();
    void *lp_item = SB_D_Queue::remove();
    unlock();
    return lp_item;
}

//
// Remove item
//
SB_INLINE bool SB_Ts_D_Queue::remove_list(SB_DQL_Type *pp_item) {
    lock();
    bool lv_ret = SB_D_Queue::remove_list(pp_item);
    unlock();
    return lv_ret;
}

//
// Print queue
//
SB_INLINE void SB_Ts_D_Queue::printself(bool pv_traverse, bool pv_lock) {
    if (pv_lock)
        lock();
    SB_D_Queue::printself(pv_traverse);
    if (pv_lock)
        unlock();
}

//
// Unlock queue (can be called by client)
//
SB_INLINE void SB_Ts_D_Queue::unlock() {
    int lv_status;

    lv_status = iv_cv.unlock();
    SB_util_assert_ieq(lv_status, 0);
}

// SB_Sig_Queue destructor
SB_INLINE SB_Sig_Queue::~SB_Sig_Queue() {
}

//
// Add item
// If there are waiters, then signal/broadcast
//
SB_INLINE void SB_Sig_Queue::add(SB_QL_Type *pp_item) {
    int lv_status;

    lock();
    SB_Queue::add(pp_item);
    if (iv_waiters > 0) {
        if (iv_multi_reader && (iv_count > 1)) {
            lv_status = iv_cv.broadcast();      // wake them all
            SB_util_assert_ieq(lv_status, 0);
        } else {
            lv_status = iv_cv.signal(false);      // already locked
            SB_util_assert_ieq(lv_status, 0);
        }
    }
    unlock();
}

//
// Add item at head
// If there are waiters, then signal/broadcast
//
SB_INLINE void SB_Sig_Queue::add_at_front(SB_QL_Type *pp_item) {
    int lv_status;

    lock();
    SB_Queue::add_at_front(pp_item);
    if (iv_waiters > 0) {
        if (iv_multi_reader && (iv_count > 1)) {
            lv_status = iv_cv.broadcast();      // wake them all
            SB_util_assert_ieq(lv_status, 0);
        } else {
            lv_status = iv_cv.signal(false);      // already locked
            SB_util_assert_ieq(lv_status, 0);
        }
    }
    unlock();
}

//
// Add item
// If there are waiters, then signal/broadcast
//
SB_INLINE void SB_Sig_Queue::add_lock(SB_QL_Type *pp_item, bool pv_lock) {
    int lv_status;

    if (pv_lock)
        lock();
    SB_Queue::add(pp_item);
    if (iv_waiters > 0) {
        if (iv_multi_reader && (iv_count > 1)) {
            lv_status = iv_cv.broadcast();      // wake them all
            SB_util_assert_ieq(lv_status, 0);
        } else {
            lv_status = iv_cv.signal(false);      // already locked
            SB_util_assert_ieq(lv_status, 0);
        }
    }
    if (pv_lock)
        unlock();
}

//
// Add item at head
// If there are waiters, then signal/broadcast
//
SB_INLINE void SB_Sig_Queue::add_lock_at_front(SB_QL_Type *pp_item,
                                               bool        pv_lock) {
    int lv_status;

    if (pv_lock)
        lock();
    SB_Queue::add_at_front(pp_item);
    if (iv_waiters > 0) {
        if (iv_multi_reader && (iv_count > 1)) {
            lv_status = iv_cv.broadcast();      // wake them all
            SB_util_assert_ieq(lv_status, 0);
        } else {
            lv_status = iv_cv.signal(false);      // already locked
            SB_util_assert_ieq(lv_status, 0);
        }
    }
    if (pv_lock)
        unlock();
}

//
// Lock queue (can be called by client)
//
SB_INLINE void SB_Sig_Queue::lock() {
    SB_Ts_Queue::lock();
}

//
// Print queue
//
SB_INLINE void SB_Sig_Queue::printself(bool pv_traverse) {
    lock();
    SB_Queue::printself(pv_traverse);
    unlock();
}

//
// Remove item
// Suspend caller if queue is empty
//
SB_INLINE void *SB_Sig_Queue::remove() {
    int lv_status;

    lock();
    iv_waiters++;
    while (iv_count == 0) {
        lv_status = iv_cv.wait(false);      // already locked
        SB_util_assert_ieq(lv_status, 0);
    }
    iv_waiters--;
    void *lp_item = SB_Queue::remove();
    unlock();
    return lp_item;
}

//
// Remove item
// Suspend caller if queue is empty
//
SB_INLINE void *SB_Sig_Queue::remove_lock(bool pv_lock) {
    int lv_status;

    if (pv_lock)
        lock();
    iv_waiters++;
    while (iv_count == 0) {
        lv_status = iv_cv.wait(false);      // already locked
        SB_util_assert_ieq(lv_status, 0);
    }
    iv_waiters--;
    void *lp_item = SB_Queue::remove();
    if (pv_lock)
        unlock();
    return lp_item;
}

//
// Unlock queue (can be called by client)
//
SB_INLINE void SB_Sig_Queue::unlock() {
    SB_Ts_Queue::unlock();
}

// SB_Sig_D_Queue destructor
SB_INLINE SB_Sig_D_Queue::~SB_Sig_D_Queue() {
}

//
// Add item
// If there are waiters, then signal/broadcast
//
SB_INLINE void SB_Sig_D_Queue::add(SB_DQL_Type *pp_item) {
    int lv_status;

    lock();
    SB_D_Queue::add(pp_item);
    if (iv_waiters > 0) {
        if (iv_multi_reader && (iv_count > 1)) {
            lv_status = iv_cv.broadcast();      // wake them all
            SB_util_assert_ieq(lv_status, 0);
        } else {
            lv_status = iv_cv.signal(false);      // already locked
            SB_util_assert_ieq(lv_status, 0);
        }
    }
    unlock();
}

//
// Add item
// If there are waiters, then signal/broadcast
//
SB_INLINE void SB_Sig_D_Queue::add_lock(SB_DQL_Type *pp_item, bool pv_lock) {
    int lv_status;

    if (pv_lock)
        lock();
    SB_D_Queue::add(pp_item);
    if (iv_waiters > 0) {
        if (iv_multi_reader && (iv_count > 1)) {
            lv_status = iv_cv.broadcast();      // wake them all
            SB_util_assert_ieq(lv_status, 0);
        } else {
            lv_status = iv_cv.signal(false);      // already locked
            SB_util_assert_ieq(lv_status, 0);
        }
    }
    if (pv_lock)
        unlock();
}

//
// Add item at head
// If there are waiters, then signal/broadcast
//
SB_INLINE void SB_Sig_D_Queue::add_at_front(SB_DQL_Type *pp_item) {
    int lv_status;

    lock();
    SB_D_Queue::add_at_front(pp_item);
    if (iv_waiters > 0) {
        if (iv_multi_reader && (iv_count > 1)) {
            lv_status = iv_cv.broadcast();      // wake them all
            SB_util_assert_ieq(lv_status, 0);
        } else {
            lv_status = iv_cv.signal(false);      // already locked
            SB_util_assert_ieq(lv_status, 0);
        }
    }
    unlock();
}

//
// Add item to list
// If there are waiters, then signal/broadcast
//
SB_INLINE void SB_Sig_D_Queue::add_list(SB_DQL_Type *pp_prev,
                                        SB_DQL_Type *pp_item) {
    int lv_status;

    lock();
    SB_D_Queue::add_list(pp_prev, pp_item);
    if (iv_waiters > 0) {
        if (iv_multi_reader && (iv_count > 1)) {
            lv_status = iv_cv.broadcast();      // wake them all
            SB_util_assert_ieq(lv_status, 0);
        } else {
            lv_status = iv_cv.signal(false);      // already locked
            SB_util_assert_ieq(lv_status, 0);
        }
    }
    unlock();
}

//
// Lock queue (can be called by client)
//
SB_INLINE void SB_Sig_D_Queue::lock() {
    SB_Ts_D_Queue::lock();
}

//
// Add item at head
// If there are waiters, then signal/broadcast
//
SB_INLINE void SB_Sig_D_Queue::add_lock_at_front(SB_DQL_Type *pp_item,
                                                 bool         pv_lock) {
    int lv_status;

    if (pv_lock)
        lock();
    SB_D_Queue::add_at_front(pp_item);
    if (iv_waiters > 0) {
        if (iv_multi_reader && (iv_count > 1)) {
            lv_status = iv_cv.broadcast();      // wake them all
            SB_util_assert_ieq(lv_status, 0);
        } else {
            lv_status = iv_cv.signal(false);      // already locked
            SB_util_assert_ieq(lv_status, 0);
        }
    }
    if (pv_lock)
        unlock();
}

//
// Add item to list
// If there are waiters, then signal/broadcast
//
SB_INLINE void SB_Sig_D_Queue::add_lock_list(SB_DQL_Type *pp_prev,
                                             SB_DQL_Type *pp_item,
                                             bool         pv_lock) {
    int lv_status;

    if (pv_lock)
        lock();
    SB_D_Queue::add_list(pp_prev, pp_item);
    if (iv_waiters > 0) {
        if (iv_multi_reader && (iv_count > 1)) {
            lv_status = iv_cv.broadcast();      // wake them all
            SB_util_assert_ieq(lv_status, 0);
        } else {
            lv_status = iv_cv.signal(false);      // already locked
            SB_util_assert_ieq(lv_status, 0);
        }
    }
    if (pv_lock)
        unlock();
}

//
// Print queue
//
SB_INLINE void SB_Sig_D_Queue::printself(bool pv_traverse, bool pv_lock) {
    SB_Ts_D_Queue::printself(pv_traverse, pv_lock);
}

//
// Remove item
// Suspend caller if queue is empty
//
SB_INLINE void *SB_Sig_D_Queue::remove() {
    int lv_status;

    lock();
    iv_waiters++;
    while (iv_count == 0) {
        lv_status = iv_cv.wait(false);      // already locked
        SB_util_assert_ieq(lv_status, 0);
    }
    iv_waiters--;
    void *lp_item = SB_D_Queue::remove();
    unlock();
    return lp_item;
}

//
// Remove item
// Suspend caller if queue is empty
//
SB_INLINE void *SB_Sig_D_Queue::remove_lock(bool pv_lock) {
    int lv_status;

    if (pv_lock)
        lock();
    iv_waiters++;
    while (iv_count == 0) {
        lv_status = iv_cv.wait(false);      // already locked
        SB_util_assert_ieq(lv_status, 0);
    }
    iv_waiters--;
    void *lp_item = SB_D_Queue::remove();
    if (pv_lock)
        unlock();
    return lp_item;
}

//
// Unlock queue (can be called by client)
//
SB_INLINE void SB_Sig_D_Queue::unlock() {
    SB_Ts_D_Queue::unlock();
}

//
// Lock-free queue utilities
//
SB_INLINE void LFQ_PT_AS2(SB_Lf_Queue::PT *pp_lhs,
                          SB_Lf_Queue::PT *pp_rhs) {
    pp_lhs->ip_ptr = pp_rhs->ip_ptr;
    pp_lhs->iv_count = pp_rhs->iv_count;
}

SB_INLINE void LFQ_PT_AS3(SB_Lf_Queue::PT *pp_lhs,
                          SB_Lf_Queue::PT *pp_rhs_ptr,
                          unsigned int     pv_rhs_count) {
    pp_lhs->ip_ptr = pp_rhs_ptr;
    pp_lhs->iv_count = pv_rhs_count;
}

SB_INLINE bool operator==(SB_Lf_Queue::PT pv_lhs, SB_Lf_Queue::PT pv_rhs) {
    return ((pv_lhs.ip_ptr == pv_rhs.ip_ptr) && (pv_lhs.iv_count == pv_rhs.iv_count));
}

SB_INLINE bool operator!=(SB_Lf_Queue::PT pv_lhs, SB_Lf_Queue::PT pv_rhs) {
    return (!((pv_lhs.ip_ptr == pv_rhs.ip_ptr) && (pv_lhs.iv_count == pv_rhs.iv_count)));
}

SB_INLINE bool LFQ_CAS(volatile SB_Lf_Queue::PT *pp_mem,
                       SB_Lf_Queue::PT           pv_oldv,
                       SB_Lf_Queue::PT           pv_newv) {
    long long *lp_oldv;
    long long *lp_newv;

    lp_oldv = reinterpret_cast<long long *>(&pv_oldv);
    lp_newv = reinterpret_cast<long long *>(&pv_newv);
    return __sync_bool_compare_and_swap_8(pp_mem, *lp_oldv, *lp_newv);
}

#endif // !__SB_QUEUE_INL_
