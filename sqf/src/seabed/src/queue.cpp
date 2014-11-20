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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mstrace.h"
#include "queue.h"

#include "seabed/trace.h"

#ifndef USE_SB_INLINE
#include "queue.inl"
#endif


SB_Queue::SB_Queue(const char *pp_name)
: iv_aecid_queue(SB_ECID_QUEUE),
  ip_head(NULL), ip_tail(NULL), iv_count(0), iv_hi(0), iv_multi_reader(false) {
    ia_q_name[sizeof(ia_q_name) - 1] = '\0';
    SB_util_assert_cpne(pp_name, NULL);
    strncpy(ia_q_name, pp_name, sizeof(ia_q_name) - 1);
}

void SB_Queue::printself(bool pv_traverse) {
    int lv_inx;

    printf("this=%p(%s), size=%d %s\n", pfp(this),
           ia_q_name, iv_count, (ip_head == NULL) ? "empty" : "");
    if (pv_traverse) {
        SB_QL_Type *lp_item = ip_head;
        for (lv_inx = 0; lp_item != NULL; lv_inx++, lp_item = lp_item->ip_next)
            printf("  inx=%d, item=%p\n", lv_inx, pfp(lp_item));
    }
}

// SB_D_Queue constructor
SB_D_Queue::SB_D_Queue(int pv_qid, const char *pp_name)
: iv_aecid_d_queue(SB_ECID_QUEUE_D),
  ip_head(NULL), ip_tail(NULL), iv_count(0), iv_hi(0), iv_qid(pv_qid),
  iv_multi_reader(false) {
    ia_d_q_name[sizeof(ia_d_q_name) - 1] = '\0';
    SB_util_assert_cpne(pp_name, NULL);
    strncpy(ia_d_q_name, pp_name, sizeof(ia_d_q_name) - 1);
}

void SB_D_Queue::printself(bool pv_traverse) {
    int lv_inx;

    printf("this=%p(%s), q=%d, size=%d %s\n", pfp(this),
           ia_d_q_name, iv_qid, iv_count, (ip_head == NULL) ? "empty" : "");
    if (pv_traverse) {
        SB_DQL_Type *lp_item = ip_head;
        for (lv_inx = 0; lp_item != NULL; lv_inx++, lp_item = lp_item->ip_next)
            printf("  inx=%d, q=%d, p=%p, n=%p, Item=%p\n",
                   lv_inx,
                   lp_item->iv_qid,
                   pfp(lp_item->ip_prev),
                   pfp(lp_item->ip_next),
                   pfp(lp_item));
    }
}

// SB_Ts_Queue constructor
SB_Ts_Queue::SB_Ts_Queue(const char *pp_name) : SB_Queue(pp_name) {
    iv_cv.setname(pp_name);
}

// SB_Ts_D_Queue constructor
SB_Ts_D_Queue::SB_Ts_D_Queue(int pv_qid, const char *pp_name)
: SB_D_Queue(pv_qid, pp_name) {
    iv_cv.setname(pp_name);
}

// SB_Sig_Queue constructor
SB_Sig_Queue::SB_Sig_Queue(const char *pp_name, bool pv_multi_reader)
: SB_Ts_Queue(pp_name), iv_waiters(0) {
    SB_Queue::iv_multi_reader = pv_multi_reader;
}

// SB_Sig_D_Queue constructor
SB_Sig_D_Queue::SB_Sig_D_Queue(int         pv_qid,
                               const char *pp_name,
                               bool        pv_multi_reader)
: SB_Ts_D_Queue(pv_qid, pp_name), iv_waiters(0) {
    SB_D_Queue::iv_multi_reader = pv_multi_reader;
}

// SB_Lf_Queue constructor
SB_Lf_Queue::SB_Lf_Queue(const char *pp_name)
: SB_Queue(pp_name) {
#ifdef SB_Q_ALLOW_QALLOC
    const char *WHERE = "SB_Lf_Queue::SB_Lf_Queue";
#endif
    LFNT       *lp_dummy;

    lp_dummy = new LFNT;
#ifdef SB_Q_ALLOW_QALLOC
    if (gv_ms_trace_qalloc)
        trace_where_printf(WHERE, "this=%p(%s), new=%p\n",
                           pfp(this), ia_q_name, pfp(lp_dummy));
#endif
    lp_dummy->ip_data = NULL;
    LFQ_PT_AS3(&lp_dummy->iv_next, NULL, 0);
    LFQ_PT_AS3(&iv_head, &lp_dummy->iv_next, 0);
    LFQ_PT_AS3(&iv_tail, &lp_dummy->iv_next, 0);
}

SB_Lf_Queue::~SB_Lf_Queue() {
#ifdef SB_Q_ALLOW_QALLOC
    const char *WHERE = "SB_Lf_Queue::~SB_Lf_Queue";
#endif
    void       *lp_node;

    do {
        lp_node = remove();
    } while (lp_node != NULL);
    if (iv_tail.ip_ptr != NULL) {
#ifdef SB_Q_ALLOW_QALLOC
    if (gv_ms_trace_qalloc)
        trace_where_printf(WHERE, "this=%p(%s), delete=%p\n",
                           pfp(this), ia_q_name, pfp(iv_tail.ip_ptr));
#endif
        delete iv_tail.ip_ptr;
    }
}

void SB_Lf_Queue::add(SB_QL_Type *pp_item) {
#ifdef SB_Q_ALLOW_QALLOC
    const char *WHERE = "SB_Lf_Queue::add";
#endif
    LFNT       *lp_node;
    PT          lv_next;
    PT          lv_tail;
    PT          lv_temp;

    lp_node = new LFNT;
#ifdef SB_Q_ALLOW_QALLOC
    if (gv_ms_trace_qalloc)
        trace_where_printf(WHERE, "this=%p(%s), new=%p\n",
                           pfp(this), ia_q_name, pfp(lp_node));
#endif
    LFQ_PT_AS3(&lp_node->iv_next, NULL, 0);
    lp_node->ip_data = pp_item;
    for (;;) {                                                        // Keep trying until Enqueue is done
        LFQ_PT_AS2(&lv_tail, &iv_tail);                               // Read Tail.ptr and Tail.count together
        LFQ_PT_AS2(&lv_next, lv_tail.ip_ptr);                         // Read next ptr and count fields together
        if (lv_tail == iv_tail) {                                     // Are tail and next consistent?
            if (lv_next.ip_ptr == NULL) {                             // Was Tail pointing to the last node?
                LFQ_PT_AS3(&lv_temp, &lp_node->iv_next, lv_next.iv_count + 1);
                if (LFQ_CAS(lv_tail.ip_ptr, lv_next, lv_temp)) {      // Try to link node at the end of the linked list
                    break;                                            // Enqueue is done. Exit loop
                }
            } else {                                                  // Tail was not pointing to the last node
                LFQ_PT_AS3(&lv_temp, lv_next.ip_ptr, lv_tail.iv_count + 1);
                LFQ_CAS(&iv_tail, lv_tail, lv_temp);                  // Try to swing Tail to the next node
            }
        }
    }
    LFQ_PT_AS3(&lv_temp, &lp_node->iv_next, lv_tail.iv_count + 1);
    LFQ_CAS(&iv_tail, lv_tail, lv_temp);                              // Enqueue is done. Try to swing Tail to the inserted node
}

void SB_Lf_Queue::add_at_front(SB_QL_Type *) {
    SB_util_abort("NOT implemented"); // not implemented
}

bool SB_Lf_Queue::empty() {
    return (iv_head.ip_ptr == iv_tail.ip_ptr);
}

void *SB_Lf_Queue::remove() {
#ifdef SB_Q_ALLOW_QALLOC
    const char *WHERE = "SB_Lf_Queue::remove";
#endif
    LFNT       *lp_node;
    void       *lp_ret;
    PT          lv_head;
    PT          lv_next;
    PT          lv_tail;
    PT          lv_temp;

    for (;;) {                                             // Keep trying until Dequeue is done
        LFQ_PT_AS2(&lv_head, &iv_head);                    // Read Head
        LFQ_PT_AS2(&lv_tail, &iv_tail);                    // Read Tail
        LFQ_PT_AS2(&lv_next, lv_head.ip_ptr);              // Read Head.ptr->next
        if (lv_head == iv_head) {                          // Are head, tail, and next consistent?
            if (lv_head.ip_ptr == lv_tail.ip_ptr) {        // Is queue empty or Tail falling behind?
                if (lv_next.ip_ptr == NULL) {              // Is queue empty?
                    return NULL;                           // Queue is empty, couldn't dequeue
                }
                LFQ_PT_AS3(&lv_temp, lv_next.ip_ptr, lv_tail.iv_count + 1);
                LFQ_CAS(&iv_tail, lv_tail, lv_temp);       // Tail is falling behind. Try to advance it
            } else {                                       // No need to deal with Tail
                // read value before CAS, otherwise another dequeue might free the next node
                lp_node = reinterpret_cast<LFNT *>(lv_next.ip_ptr);
                lp_ret = lp_node->ip_data;
                lp_node->ip_data = NULL;                   // mark removed
                LFQ_PT_AS3(&lv_temp, lv_next.ip_ptr, lv_head.iv_count + 1);
                if (LFQ_CAS(&iv_head, lv_head, lv_temp)) { // Try to swing Head to the next node
                    break;                                 // Dequeue is done. Exit loop
                }
            }
        }
    }
#ifdef SB_Q_ALLOW_QALLOC
    if (gv_ms_trace_qalloc)
        trace_where_printf(WHERE, "this=%p(%s), delete=%p\n",
                           pfp(this), ia_q_name, pfp(lv_head.ip_ptr));
#endif
    delete lv_head.ip_ptr;                                 // It is safe now to free the old dummy node
    return lp_ret;                                         // Queue was not empty, dequeue succeeded
}

void SB_Lf_Queue::printself(bool pv_traverse) {
    LFNT       *lp_next;
    LFNT       *lp_node;
    SB_QL_Type *lp_item;
    int         lv_count;
    int         lv_inx;

    lv_count = size();
#ifdef PRINTSELF_NORMAL
    printf("this=%p(%s), size=%d\n", pfp(this),
           ia_q_name, lv_count);
#else
    printf("this=%p(%s), h=%p-%x, t=%p-%x, size=%d %s\n",
           pfp(this), ia_q_name,
           pfp(iv_head.ip_ptr), iv_head.iv_count,
           pfp(iv_tail.ip_ptr), iv_tail.iv_count,
           lv_count,
           (iv_head.ip_ptr == iv_tail.ip_ptr) ? "empty" : "");
#endif
    if (pv_traverse) {
        lp_node = reinterpret_cast<LFNT *>(iv_head.ip_ptr);
        for (lv_inx = 0; lp_node != NULL; lp_node = lp_next) {
            lp_item = lp_node->ip_data;
            lp_next = reinterpret_cast<LFNT *>(lp_node->iv_next.ip_ptr);
#ifdef PRINTSELF_NORMAL
            printf("  inx=%d, Item=%p\n", lv_inx, pfp(lp_item));
#else
            int lv_id;
            if (lp_item == NULL)
                lv_id = 0;
            else
                lv_id = lp_item->iv_id.i;
            printf("  inx=%d, Node=%p, Next=%p, Count=%x, Item=%p, Id=%x %s\n",
                   lv_inx,
                   pfp(lp_node),
                   pfp(lp_next),
                   lp_node->iv_next.iv_count,
                   pfp(lp_item),
                   lv_id,
                   (lp_item == NULL) ? "dummy" : "");
#endif
            if (lp_item != NULL)
                lv_inx++;
        }
    }
}

int SB_Lf_Queue::size() {
    PT         *lp_item;
    SB_QL_Type *lp_item2;
    int         lv_count;

    lv_count = 0;
    lp_item = iv_head.ip_ptr;
    lp_item = lp_item->ip_ptr;
    while (lp_item != NULL) {
        lp_item2 = reinterpret_cast<SB_QL_Type *>(lp_item);
        if (lp_item2->iv_qid != -1)
            lv_count++;
        lp_item = lp_item->ip_ptr;
    }
    return lv_count;
}

