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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <linux/unistd.h>

#include <sys/utsname.h>

#include "seabed/debug.h"
#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"
#include "seabed/thread.h"
#include "seabed/trace.h"

#include "imap.h"
#include "lmap.h"
#include "llmap.h"
#include "mseventmgr.h"
#include "props.h"
#include "queue.h"
#include "queuemd.h"
#include "recvq.h"
#include "slotmgr.h"
#include "smap.h"
#include "tchkfe.h"
#include "tablemgr.h"
#include "timermap.h"
#include "tmap.h"
#include "tutil.h"
#include "tutilp.h"

#define gettid() (pid_t) syscall(__NR_gettid)

extern void msg_init_env(int pv_argc, char **ppp_argv);
extern void sb_timer_init();
extern void sb_timer_shutdown();

typedef struct {
    SB_ML_Type   iv_link;                // MUST be first
} Item;
Item *new_Item(int pv_id) {
    Item *lp_item = new Item();
    lp_item->iv_link.iv_id.i = pv_id;
    return lp_item;
}

typedef struct {
    SB_DQL_Type  iv_link;                // MUST be first
} DItem;
DItem *new_DItem(int pv_id) {
    DItem *lp_item = new DItem();
    lp_item->iv_link.iv_id.i = pv_id;
    return lp_item;
}

typedef struct {
    SB_LML_Type  iv_link;                // MUST be first
} Litem;
Litem *new_Litem(int pv_id) {
    Litem *lp_item = new Litem();
    lp_item->iv_link.iv_id.l = pv_id;
    return lp_item;
}

typedef struct {
    SB_LLML_Type  iv_link;                // MUST be first
} LLitem;
LLitem *new_LLitem(SB_Int64_Type pv_id) {
    LLitem *lp_item = new LLitem();
    lp_item->iv_link.iv_id.ll = pv_id;
    return lp_item;
}

typedef struct TimerItem {
    struct timeval iv_tod;
    bool           iv_called;
    int            iv_pad;
    int            iv_tics;
    SB_TML_Type    iv_link;
} TimerItem;
TimerItem *new_TimerItem(int pv_tics) {
    TimerItem *lp_item = new TimerItem();
    memset(lp_item, -1, sizeof(TimerItem));
    lp_item->iv_called = false;
    lp_item->iv_tics = pv_tics;
    gettimeofday(&lp_item->iv_tod, NULL);
    return lp_item;
}

typedef struct Table_Entry {
    bool    iv_inuse;
    size_t  iv_inx;
    size_t  iv_state;
} Table_Entry;

class Table_Entry_Mgr : public SB_Table_Entry_Mgr<Table_Entry> {
public:
    Table_Entry_Mgr() {
        iv_entry_size = sizeof(Table_Entry) + sizeof(long long) - 1;
        iv_entry_size = (iv_entry_size / sizeof(long long)) * sizeof(long long);
    }

    Table_Entry *entry_alloc(size_t pv_inx) {
        Table_Entry *lp_entry = new Table_Entry;
        lp_entry->iv_inuse = true;
        lp_entry->iv_inx = pv_inx;
        lp_entry->iv_state = lp_entry->iv_inx + 1;
        return lp_entry;
    }

    void entry_alloc_block(Table_Entry **ppp_table,
                           size_t        pv_inx,
                           size_t        pv_count) {
        Table_Entry *lp_entry;
        char        *lp_entries;
        size_t       lv_inx;

        lp_entries = new char[iv_entry_size * pv_count];
        for (lv_inx = 0; lv_inx < pv_count; lv_inx++) {
            lp_entry = (Table_Entry *) lp_entries;
            lp_entry->iv_inuse = false;
            lp_entry->iv_inx = pv_inx + lv_inx;
            lp_entry->iv_state = lp_entry->iv_inx + 1;
            ppp_table[pv_inx + lv_inx] = lp_entry;
            lp_entries += iv_entry_size;
        }
    }

    void entry_cap_change(size_t pv_inx, size_t pv_cap) {
        pv_inx = pv_inx; // touch
        pv_cap = pv_cap; // touch
    }

    bool entry_free(Table_Entry *pp_entry, size_t pv_inx, bool pv_urgent) {
        pv_urgent = pv_urgent; // touch
        assert(pp_entry->iv_inx == pv_inx);
        delete pp_entry;
        return true;
    }

    void entry_free_block(Table_Entry **ppp_table,
                          size_t        pv_inx,
                          size_t        pv_count) {
        char   *lp_entries;

        pv_count = pv_count; // touch
        lp_entries = (char *) ppp_table[pv_inx];
        delete [] lp_entries;
    }

    void entry_inuse(Table_Entry *pp_entry, size_t pv_inx, bool pv_inuse) {
        assert(pp_entry->iv_inx == pv_inx);
        pp_entry->iv_inuse = pv_inuse;
    }

    void entry_print_str(Table_Entry *pp_entry, size_t pv_inx, char *pp_info) {
        sprintf(pp_info, "entry=%p, inx=" PFSIZEOF ", inuse=%d, state=" PFSIZEOF,
                (void *) pp_entry,
                pv_inx,
                pp_entry->iv_inuse,
                pp_entry->iv_state);
    }

private:
    size_t iv_entry_size;
};
typedef SB_Table_Mgr<Table_Entry> Table_Mgr;
typedef SB_Ts_Table_Mgr<Table_Entry> Ts_Table_Mgr;

bool               gv_inited = false;
SB_Ms_Tl_Event_Mgr gv_mgr;
SB_Sig_Queue       gv_q1("global-sig-q1", false);
SB_Sig_Queue       gv_q2("global-sig-q2", false);
SB_Sig_Queue       gv_q3("global-sig-q3", false);
SB_Sig_Queue       gv_q4("global-sig-q4", false);
bool               gv_rh5 = false;
int                gv_rid;
int                gv_rid1;
int                gv_rid2;
unsigned int       gv_seed;
bool               gv_vg = false;
int                gv_wid;


class Rthr : public SB_Thread::Thread {
public:
    Rthr(Function pv_fun, const char *pp_name, SB_Sig_Queue *pp_q)
    : SB_Thread::Thread(pv_fun, pp_name), ip_q(pp_q) {
    }
    SB_Sig_Queue *ip_q;
};

class Wthr : public SB_Thread::Thread {
public:
    Wthr(Function pv_fun, const char *pp_name, SB_Sig_Queue *pp_q)
    : SB_Thread::Thread(pv_fun, pp_name), ip_q(pp_q) {
    }
    SB_Sig_Queue *ip_q;
};


void *t_r1(void *pp_arg) {
    SB_Sig_Queue *lp_q = (SB_Sig_Queue *) pp_arg; // cast

    lp_q->printself(true);
    printf("r about to remove\n");
    Item *lp_i1 = (Item *) lp_q->remove();
    printf("r remove done, n=%p\n\n", (void *) lp_i1);
    assert(lp_i1->iv_link.iv_id.i == 1);
    lp_q->printself(true);

    lp_q->printself(true);
    printf("r about to remove\n");
    Item *lp_i2 = (Item *) lp_q->remove();
    printf("r remove done, n=%p\n\n", (void *) lp_i2);
    assert(lp_i2->iv_link.iv_id.i == 2);
    lp_q->printself(true);

    lp_q->printself(true);
    printf("r about to remove\n");
    Item *lp_i3 = (Item *) lp_q->remove();
    printf("r remove done, n=%p\n\n", (void *) lp_i3);
    assert(lp_i3->iv_link.iv_id.i == 3);
    lp_q->printself(true);

     // cleanup
     delete lp_i1;
     delete lp_i2;
     delete lp_i3;

    return NULL;
}

void *t_w1(void *pp_arg) {
    SB_Sig_Queue *lp_q = (SB_Sig_Queue *) pp_arg; // cast
    Item *lp_i1 = new_Item(1);
    Item *lp_i2 = new_Item(2);
    Item *lp_i3 = new_Item(3);

    lp_q->printself(true);
    printf("w about to add, n=%p\n", (void *) lp_i1);
    lp_q->add(&lp_i1->iv_link);
    printf("w add done\n");
    lp_q->printself(true);

    printf("w about to add, n=%p\n", (void *) lp_i2);
    lp_q->add(&lp_i2->iv_link);
    printf("w add done\n");
    lp_q->printself(true);

    printf("w about to add, n=%p\n", (void *) lp_i3);
    lp_q->add(&lp_i3->iv_link);
    printf("w add done\n");
    lp_q->printself(true);

    return NULL;
}

void *t_r2(void *pp_arg) {
    Wthr *lp_thr = (Wthr *) pp_arg; // cast
    return t_r1(lp_thr->ip_q);
}

void *t_w2(void *pp_arg) {
    Rthr *lp_thr = (Rthr *) pp_arg; // cast
    return t_w1(lp_thr->ip_q);
}

void *t_r3(void *pp_arg) {
    gv_rid = (int) (long) pp_arg;
    proc_register_group_pin(-1, gv_rid);
    SB_Ms_Event_Mgr *lp_mgr = gv_mgr.get_mgr(NULL);
    lp_mgr->set_event(-1, NULL);
    assert(lp_mgr->get_event(PWU|LSEM) == PWU);
    lp_mgr->clear_event(-1);
    lp_mgr->set_event(PWU|LSEM, NULL);
    assert(lp_mgr->get_event(PWU|LSEM) == PWU);
    lp_mgr->clear_event(PWU);
    assert(lp_mgr->get_event(LSEM) == LSEM);
    lp_mgr->clear_event(LSEM);
    assert(lp_mgr->get_event(LSEM) == 0);

    Item *lp_wi1 = (Item *) gv_q1.remove(); // sync - id should be set

    Item *lp_i1 = new_Item(1);
    gv_q2.add(&lp_i1->iv_link);

    Item *lp_wi2 = (Item *) gv_q1.remove(); // sync - we should get LSEM
    assert(lp_mgr->get_event(LSEM) == LSEM); // get signal from w thread
    lp_mgr->clear_event(LSEM);
    assert(lp_mgr->get_event(LSEM) == 0);

    lp_mgr->set_event_pin(gv_wid, LDONE, 0);
    Item *lp_i2 = new_Item(1);
    gv_q2.add(&lp_i2->iv_link);

    // cleanup
    delete lp_wi1;
    delete lp_wi2;

    return NULL;
}

void *t_w3(void *pp_arg) {
    gv_wid = (int) (long) pp_arg;
    proc_register_group_pin(-1, gv_wid);
    SB_Ms_Event_Mgr *lp_mgr = gv_mgr.get_mgr(NULL);
    assert(lp_mgr->get_event(LREQ) == 0);
    lp_mgr->set_event(LDONE|LREQ, NULL);
    assert(lp_mgr->get_event(LDONE|LREQ) == LDONE);
    lp_mgr->clear_event(LDONE);
    assert(lp_mgr->get_event(LREQ) == LREQ);
    lp_mgr->clear_event(LREQ);
    assert(lp_mgr->get_event(LREQ) == 0);

    Item *lp_i1 = new_Item(1);
    gv_q1.add(&lp_i1->iv_link);
    Item *lp_ri1 = (Item *) gv_q2.remove(); // sync - id should be set

    lp_mgr->set_event_pin(gv_rid, LSEM, 0);
    Item *lp_i2 = new_Item(1);
    gv_q1.add(&lp_i2->iv_link);

    Item *lp_ri2 = (Item *) gv_q2.remove(); // sync - we should get LDONE
    assert(lp_mgr->get_event(LDONE) == LDONE); // get signal from w thread
    lp_mgr->clear_event(LDONE);
    assert(lp_mgr->get_event(LDONE) == 0);

    // cleanup
    delete lp_ri1;
    delete lp_ri2;

    return NULL;
}

void *t_r4(void *pp_arg) {
    gv_rid = (int) (long) pp_arg;
    proc_register_group_pin(-1, gv_rid);
    Item *lp_wi1 = (Item *) gv_q1.remove(); // sync - id should be set

    Item *lp_i1 = new_Item(1);
    gv_q2.add(&lp_i1->iv_link);

#ifdef USE_EVENT_REG
    proc_event_register(LREQ);
    proc_event_register(LDONE);
#endif
    short lv_lerr = XWAIT(-1, -1);
    assert(lv_lerr == LDONE);
    lv_lerr = XWAIT(-1, -1);
    assert(lv_lerr == LSEM);
    SB_Thread::Sthr::sleep(1000); // so that wakeup is ok

    // cleanup
    delete lp_wi1;

    return NULL;
}

void *t_w4(void *pp_arg) {
    gv_wid = (int) (long) pp_arg;
    proc_register_group_pin(-1, gv_wid);
    Item *lp_i1 = new_Item(1);
    gv_q1.add(&lp_i1->iv_link);
    Item *lp_ri1 = (Item *) gv_q2.remove(); // sync - id should be set

    XAWAKE(gv_rid, LDONE|LSEM);

    // cleanup
    delete lp_ri1;

    return NULL;
}

void *t_r51(void *pp_arg) {
    gv_rid1 = (int) (long) pp_arg;
    proc_register_group_pin(0, gv_rid1);
    Item *lp_wi1 = (Item *) gv_q1.remove(); // sync - id should be set

    Item *lp_i1 = new_Item(1);
    gv_q3.add(&lp_i1->iv_link);

#ifdef USE_EVENT_REG
    proc_event_register(LREQ);
    proc_event_register(LDONE);
#endif
    short lv_lerr = XWAIT(LDONE | LSEM, -1);
    assert(lv_lerr == LDONE);
    lv_lerr = XWAIT(LDONE | LSEM, -1);
    assert(lv_lerr == LSEM);
    SB_Thread::Sthr::sleep(1000); // so that wakeup is ok

    // cleanup
    delete lp_wi1;

    return NULL;
}

void *t_r52(void *pp_arg) {
    gv_rid2 = (int) (long) pp_arg;
    proc_register_group_pin(0, gv_rid2);
    Item *lp_wi1 = (Item *) gv_q2.remove(); // sync - id should be set

    Item *lp_i1 = new_Item(1);
    gv_q4.add(&lp_i1->iv_link);

#ifdef USE_EVENT_REG
    proc_event_register(LREQ);
    proc_event_register(LDONE);
#endif
    short lv_lerr = XWAIT(LDONE | LSEM, -1);
    assert(lv_lerr == LDONE);
    lv_lerr = XWAIT(LDONE | LSEM, -1);
    assert(lv_lerr == LSEM);
    SB_Thread::Sthr::sleep(1000); // so that wakeup is ok

    // cleanup
    delete lp_wi1;

    return NULL;
}

void *t_w5(void *pp_arg) {
    gv_wid = (int) (long) pp_arg;
    proc_register_group_pin(-1, gv_wid);
    Item *lp_i1 = new_Item(1);
    gv_q1.add(&lp_i1->iv_link);
    Item *lp_i2 = new_Item(1);
    gv_q2.add(&lp_i2->iv_link);
    Item *lp_r1i1 = (Item *) gv_q3.remove(); // sync - id should be set
    Item *lp_r2i1 = (Item *) gv_q4.remove(); // sync - id should be set

    XAWAKE_A06(gv_rid1, LDONE|LSEM, 1);

    // cleanup
     delete lp_r1i1;
     delete lp_r2i1;

    return NULL;
}

void test_assert() {
    enum { ZERO = 0, NEGONE = -1, ONE = 1 };
    // uncomment each run test
//  SB_util_assert(false);
//  SB_util_assert_if(1);
//  SB_util_assert_it(0);
//  SB_util_assert_ieq(ZERO, ONE);
//  SB_util_assert_ige(ZERO, ONE);
//  SB_util_assert_igt(ZERO, ONE);
//  SB_util_assert_ile(ZERO, NEGONE);
//  SB_util_assert_ilt(ZERO, NEGONE);
//  SB_util_assert_ine(ZERO, ZERO);
}

void test_excep() {
    const char *lp_excep_text1 = "excep1";
    const char *lp_excep_text2 = "excep2";
    SB_Excep *lp_excep1;
    SB_Excep *lp_excep2;
    SB_Excep *lp_excep3;
    lp_excep1 = new SB_Excep(lp_excep_text1);
    lp_excep2 = new SB_Excep(lp_excep_text2);
    lp_excep3 = new SB_Excep(*lp_excep2);
    assert(strcmp(lp_excep1->what(), lp_excep_text1) == 0);
    assert(strcmp(lp_excep2->what(), lp_excep_text2) == 0);
    assert(strcmp(lp_excep3->what(), lp_excep_text2) == 0);
    *lp_excep1 = *lp_excep2;
    assert(strcmp(lp_excep1->what(), lp_excep_text2) == 0);
    delete lp_excep1;
    delete lp_excep2;
    delete lp_excep3;
}

void test_lmap() {
    SB_Ts_Lmap *lp_map = new SB_Ts_Lmap;

    // empty
    int lv_size = lp_map->size();
    assert(lv_size == 0);
    printf("expecting empty map, this=%p\n", (void *) lp_map);
    lp_map->printself(true);
    printf("\n");

    // get a item
    Litem *lp_i = (Litem *) lp_map->get(0);
    assert(lp_i == NULL);

    // put 1 item
    Litem *lp_i1 = new_Litem(1);
    lp_map->put(&lp_i1->iv_link);
    lv_size = lp_map->size();
    assert(lv_size == 1);
    printf("expecting 1 item, Item=%p\n", (void *) lp_i1);
    lp_map->printself(true);
    printf("\n");

    // get the item
    lp_i = (Litem *) lp_map->get(1);
    assert(lp_i == lp_i1);

    // remove 1 item
    lp_i1 = (Litem *) lp_map->remove(1);
    assert(lp_i1->iv_link.iv_id.i == 1);
    lv_size = lp_map->size();
    assert(lv_size == 0);

    // put 3 items
    Litem *lp_i2 = new_Litem(2);
    Litem *lp_i3 = new_Litem(3);
    Litem *lp_i4 = new_Litem(2+61); // hash collision
    lp_map->put(&lp_i2->iv_link);
    lp_map->put(&lp_i3->iv_link);
    lp_map->put(&lp_i4->iv_link);
    lv_size = lp_map->size();
    assert(lv_size == 3);
    printf("expecting 3 items, Item=%p, Item=%p, Item=%p\n",
           (void *) lp_i2, (void *) lp_i3, (void *) lp_i4);
    lp_map->printself(true);
    printf("\n");

    // get the item
    lp_i = (Litem *) lp_map->get(2);
    assert(lp_i == lp_i2);
    lp_i = (Litem *) lp_map->get(3);
    assert(lp_i == lp_i3);

    // remove 3 items
    lp_i2 = (Litem *) lp_map->remove(2);
    lp_i3 = (Litem *) lp_map->remove(3);
    lp_i4 = (Litem *) lp_map->remove(2+61);
    assert(lp_i2->iv_link.iv_id.i == 2);
    assert(lp_i3->iv_link.iv_id.i == 3);
    assert(lp_i4->iv_link.iv_id.i == (2+61));
    lv_size = lp_map->size();
    assert(lv_size == 0);

    // cleanup
    delete lp_map;
    delete lp_i1;
    delete lp_i2;
    delete lp_i3;
    delete lp_i4;
}

void test_lmap_enum() {
    SB_Lmap lv_map;

    // put 3 items
    Litem *lp_i1 = new_Litem(2);
    Litem *lp_i2 = new_Litem(3);
    Litem *lp_i3 = new_Litem(2+61); // hash collision
    lv_map.put(&lp_i1->iv_link);
    lv_map.put(&lp_i2->iv_link);
    lv_map.put(&lp_i3->iv_link);
    SB_Lmap_Enum *lp_enum = lv_map.keys();
    int lv_inx = 0;
    while (lp_enum->more()) {
        lv_inx++;
        int lv_id = lp_enum->next()->iv_id.i;
        Litem *lp_item = (Litem *) lv_map.get(lv_id);
        switch (lv_id) {
        case 2:
            assert(lp_item == lp_i1);
            break;
        case 3:
            assert(lp_item == lp_i2);
            break;
        case (2+61):
            assert(lp_item == lp_i3);
            break;
        default:
            assert(false);
        }
    }
    delete lp_enum;
    assert(lv_inx == 3);

    // cleanup
    delete lp_i1;
    delete lp_i2;
    delete lp_i3;
}

void test_lmap_resize() {
    char la_name[20];
    int  lv_p2;
    int  lv_size;

    // check bucket sizes
    for (lv_size = 1; lv_size < 100000; lv_size *= 2) {
        sprintf(la_name, "resize-%d", lv_size);
        SB_Lmap *lp_map = new SB_Lmap(la_name, lv_size);
        printf("expecting %d buckets\n", lv_size);
        lp_map->printself(false);
        delete lp_map;
    }

    // check resize
    lv_p2 = 1;
    SB_Lmap *lp_map = new SB_Lmap("resize-check");
    for (lv_size = 1; lv_size < 100000; lv_size++) {
        Litem *lp_item = new_Litem(lv_size);
        lp_map->put(&lp_item->iv_link);
        if (lv_size == lv_p2) {
            printf("expecting %d items\n", lv_size);
            lp_map->printself(false);
            lv_p2 *= 2;
        }
    }

    // cleanup
    for (lv_size = 1; lv_size < 100000; lv_size++) {
        Litem *lp_item = (Litem *) lp_map->remove(lv_size);
        delete lp_item;
    }
    delete lp_map;
}

void test_llmap() {
    SB_Ts_LLmap *lp_map = new SB_Ts_LLmap;

    // empty
    int lv_size = lp_map->size();
    assert(lv_size == 0);
    printf("expecting empty map, this=%p\n", (void *) lp_map);
    lp_map->printself(true);
    printf("\n");

    // get a item
    LLitem *lp_i = (LLitem *) lp_map->get(0);
    assert(lp_i == NULL);

    // put 1 item
    LLitem *lp_i1 = new_LLitem(1);
    lp_map->put(&lp_i1->iv_link);
    lv_size = lp_map->size();
    assert(lv_size == 1);
    printf("expecting 1 item, Item=%p\n", (void *) lp_i1);
    lp_map->printself(true);
    printf("\n");

    // get the item
    lp_i = (LLitem *) lp_map->get(1);
    assert(lp_i == lp_i1);

    // remove 1 item
    lp_i1 = (LLitem *) lp_map->remove(1);
    assert(lp_i1->iv_link.iv_id.i == 1);
    lv_size = lp_map->size();
    assert(lv_size == 0);

    // put 3 items
    LLitem *lp_i2 = new_LLitem(2);
    LLitem *lp_i3 = new_LLitem(3);
    LLitem *lp_i4 = new_LLitem(2+61); // hash collision
    lp_map->put(&lp_i2->iv_link);
    lp_map->put(&lp_i3->iv_link);
    lp_map->put(&lp_i4->iv_link);
    lv_size = lp_map->size();
    assert(lv_size == 3);
    printf("expecting 3 items, Item=%p, Item=%p, Item=%p\n",
           (void *) lp_i2, (void *) lp_i3, (void *) lp_i4);
    lp_map->printself(true);
    printf("\n");

    // get the item
    lp_i = (LLitem *) lp_map->get(2);
    assert(lp_i == lp_i2);
    lp_i = (LLitem *) lp_map->get(3);
    assert(lp_i == lp_i3);

    // remove 3 items
    lp_i2 = (LLitem *) lp_map->remove(2);
    lp_i3 = (LLitem *) lp_map->remove(3);
    lp_i4 = (LLitem *) lp_map->remove(2+61);
    assert(lp_i2->iv_link.iv_id.i == 2);
    assert(lp_i3->iv_link.iv_id.i == 3);
    assert(lp_i4->iv_link.iv_id.i == (2+61));
    lv_size = lp_map->size();
    assert(lv_size == 0);

    // cleanup
    delete lp_map;
    delete lp_i1;
    delete lp_i2;
    delete lp_i3;
    delete lp_i4;
}

void test_llmap_enum() {
    SB_LLmap lv_map;

    // put 3 items
    LLitem *lp_i1 = new_LLitem(2);
    LLitem *lp_i2 = new_LLitem(3);
    LLitem *lp_i3 = new_LLitem(2+61); // hash collision
    lv_map.put(&lp_i1->iv_link);
    lv_map.put(&lp_i2->iv_link);
    lv_map.put(&lp_i3->iv_link);
    SB_LLmap_Enum *lp_enum = lv_map.keys();
    int lv_inx = 0;
    while (lp_enum->more()) {
        lv_inx++;
        int lv_id = lp_enum->next()->iv_id.i;
        LLitem *lp_item = (LLitem *) lv_map.get(lv_id);
        switch (lv_id) {
        case 2:
            assert(lp_item == lp_i1);
            break;
        case 3:
            assert(lp_item == lp_i2);
            break;
        case (2+61):
            assert(lp_item == lp_i3);
            break;
        default:
            assert(false);
        }
    }
    delete lp_enum;
    assert(lv_inx == 3);

    // cleanup
    delete lp_i1;
    delete lp_i2;
    delete lp_i3;
}

void test_llmap_resize() {
    char la_name[20];
    int  lv_p2;
    int  lv_size;

    // check bucket sizes
    for (lv_size = 1; lv_size < 100000; lv_size *= 2) {
        sprintf(la_name, "resize-%d", lv_size);
        SB_LLmap *lp_map = new SB_LLmap(la_name, lv_size);
        printf("expecting %d buckets\n", lv_size);
        lp_map->printself(false);
        delete lp_map;
    }

    // check resize
    lv_p2 = 1;
    SB_LLmap *lp_map = new SB_LLmap("resize-check");
    for (lv_size = 1; lv_size < 100000; lv_size++) {
        LLitem *lp_item = new_LLitem(lv_size);
        lp_map->put(&lp_item->iv_link);
        if (lv_size == lv_p2) {
            printf("expecting %d items\n", lv_size);
            lp_map->printself(false);
            lv_p2 *= 2;
        }
    }

    // cleanup
    for (lv_size = 1; lv_size < 100000; lv_size++) {
        LLitem *lp_item = (LLitem *) lp_map->remove(lv_size);
        delete lp_item;
    }
    delete lp_map;
}

void test_map() {
    SB_Ts_Imap *lp_map = new SB_Ts_Imap;

    // empty
    int lv_size = lp_map->size();
    assert(lv_size == 0);
    printf("expecting empty map, this=%p\n", (void *) lp_map);
    lp_map->printself(true);
    printf("\n");

    // get a item
    Item *lp_i = (Item *) lp_map->get(0);
    assert(lp_i == NULL);

    // put 1 item
    Item *lp_i1 = new_Item(1);
    lp_map->put(&lp_i1->iv_link);
    lv_size = lp_map->size();
    assert(lv_size == 1);
    printf("expecting 1 item, Item=%p\n", (void *) lp_i1);
    lp_map->printself(true);
    printf("\n");

    // get the item
    lp_i = (Item *) lp_map->get(1);
    assert(lp_i == lp_i1);

    // remove 1 item
    lp_i1 = (Item *) lp_map->remove(1);
    assert(lp_i1->iv_link.iv_id.i == 1);
    lv_size = lp_map->size();
    assert(lv_size == 0);

    // put 3 items
    Item *lp_i2 = new_Item(2);
    Item *lp_i3 = new_Item(3);
    Item *lp_i4 = new_Item(2+61); // hash collision
    lp_map->put(&lp_i2->iv_link);
    lp_map->put(&lp_i3->iv_link);
    lp_map->put(&lp_i4->iv_link);
    lv_size = lp_map->size();
    assert(lv_size == 3);
    printf("expecting 3 items, Item=%p, Item=%p, Item=%p\n",
           (void *) lp_i2, (void *) lp_i3, (void *) lp_i4);
    lp_map->printself(true);
    printf("\n");

    // get the item
    lp_i = (Item *) lp_map->get(2);
    assert(lp_i == lp_i2);
    lp_i = (Item *) lp_map->get(3);
    assert(lp_i == lp_i3);

    // remove 3 items
    lp_i2 = (Item *) lp_map->remove(2);
    lp_i3 = (Item *) lp_map->remove(3);
    lp_i4 = (Item *) lp_map->remove(2+61);
    assert(lp_i2->iv_link.iv_id.i == 2);
    assert(lp_i3->iv_link.iv_id.i == 3);
    assert(lp_i4->iv_link.iv_id.i == (2+61));
    lv_size = lp_map->size();
    assert(lv_size == 0);

    // lock/unlock
    lp_map->lock();
    lp_map->unlock();

    // cleanup
    delete lp_map;
    delete lp_i1;
    delete lp_i2;
    delete lp_i3;
    delete lp_i4;
}

void test_map_enum() {
    SB_Imap lv_map;

    // put 3 items
    Item *lp_i1 = new_Item(2);
    Item *lp_i2 = new_Item(3);
    Item *lp_i3 = new_Item(2+61); // hash collision
    lv_map.put(&lp_i1->iv_link);
    lv_map.put(&lp_i2->iv_link);
    lv_map.put(&lp_i3->iv_link);
    SB_Imap_Enum *lp_enum = lv_map.keys();
    delete lp_enum;
    SB_Imap_Enum lv_enum(&lv_map);
    int lv_inx = 0;
    while (lv_enum.more()) {
        lv_inx++;
        int lv_id = lv_enum.next()->iv_id.i;
        Item *lp_item = (Item *) lv_map.get(lv_id);
        switch (lv_id) {
        case 2:
            assert(lp_item == lp_i1);
            break;
        case 3:
            assert(lp_item == lp_i2);
            break;
        case (2+61):
            assert(lp_item == lp_i3);
            break;
        default:
            assert(false);
        }
    }
    assert(lv_inx == 3);

    // cleanup
    delete lp_i1;
    delete lp_i2;
    delete lp_i3;
}

void test_map_enum2() {
    SB_Imap lv_map;

    // put 2 items
    Item *lp_i1 = new_Item(0);
    Item *lp_i2 = new_Item(0+61); // hash collision
    lv_map.put(&lp_i1->iv_link);
    lv_map.put(&lp_i2->iv_link);
    SB_Imap_Enum *lp_enum = lv_map.keys();
    delete lp_enum;
    SB_Imap_Enum lv_enum(&lv_map);
    int lv_inx = 0;
    while (lv_enum.more()) {
        lv_inx++;
        int lv_id = lv_enum.next()->iv_id.i;
        Item *lp_item = (Item *) lv_map.get(lv_id);
        switch (lv_id) {
        case 0:
            assert(lp_item == lp_i1);
            break;
        case (0+61):
            assert(lp_item == lp_i2);
            break;
        default:
            assert(false);
        }
    }
    printf("inx=%d\n", lv_inx);
    printf("\n");
    assert(lv_inx == 2);

    // cleanup
    delete lp_i1;
    delete lp_i2;
}

void test_map_enum3() {
    SB_Imap lv_map;

    // put 4 items
    Item *lp_i1 = new_Item(0);
    Item *lp_i2 = new_Item(2);
    Item *lp_i3 = new_Item(3);
    Item *lp_i4 = new_Item(2+61); // hash collision
    lv_map.put(&lp_i1->iv_link);
    lv_map.put(&lp_i2->iv_link);
    lv_map.put(&lp_i3->iv_link);
    lv_map.put(&lp_i4->iv_link);
    int lv_max = 0;
    int la_ids[4];
    while (lv_map.size()) {
        SB_Imap_Enum *lp_enum = lv_map.keys();
        while (lp_enum->more()) {
            la_ids[lv_max] = lp_enum->next()->iv_id.i;
            lv_max++;
            assert(lv_max <= 4);
        }
        for (int lv_inx = 0; lv_inx < lv_max; lv_inx++) {
            int lv_id = la_ids[lv_inx];
            Item *lp_item = (Item *) lv_map.remove(lv_id);
            switch (lv_id) {
            case 0:
                assert(lp_item == lp_i1);
                break;
            case 2:
                assert(lp_item == lp_i2);
                break;
            case 3:
                assert(lp_item == lp_i3);
                break;
            case (2+61):
                assert(lp_item == lp_i4);
                break;
            default:
                assert(false);
            }
        }
        delete lp_enum;
    }
    assert(lv_max == 4);

    // cleanup
    delete lp_i1;
    delete lp_i2;
    delete lp_i3;
    delete lp_i4;
}

void test_map_enum4() {
    SB_Imap lv_map;

    // put items
    enum { MAX_ITEMS = 30 };
    enum { MAX_IDS = 2 };
    Item *la_i[MAX_ITEMS];
    int lv_id = 1;
    int lv_inx;
    for (lv_inx = 0; lv_inx < MAX_ITEMS; lv_id += 2) {
        switch (lv_id) {
        case 25:
        case 31:
        case 37:
        case 43:
        case 49:
        case 55:
        case 61:
            break;
        default:
            if (lv_id > 64)
                lv_map.remove(lv_id % 64);
            la_i[lv_inx] = new_Item(lv_id);
            lv_map.put(&la_i[lv_inx]->iv_link);
            lv_inx++;
            break;
        }
    }
    int lv_items = 0;
    int la_ids[MAX_IDS];
    while (lv_map.size()) {
        SB_Imap_Enum *lp_enum = lv_map.keys();
        int lv_max = 0;
        while (lp_enum->more() && (lv_max < MAX_IDS)) {
            la_ids[lv_max] = lp_enum->next()->iv_id.i;
            lv_max++;
            lv_items++;
        }
        for (lv_inx = 0; lv_inx < lv_max; lv_inx++) {
            lv_id = la_ids[lv_inx];
            lv_map.remove(lv_id);
        }
        delete lp_enum;
    }
    assert(lv_items == 25);
    for (lv_inx = 0; lv_inx < MAX_ITEMS; lv_inx++)
        delete la_i[lv_inx];
    lv_map.printself(true);
}

void test_map_enum5() {
    SB_Imap lv_map;
    bool    lv_more = false;
    bool    lv_next = false;

    Item *lp_i1 = new_Item(2);
    Item *lp_i2 = new_Item(3);
    lv_map.put(&lp_i1->iv_link);
    SB_Imap_Enum lv_enum(&lv_map);
    lv_map.put(&lp_i2->iv_link);
    if (lv_more)
        lv_enum.more(); // should assert
    if (lv_next)
        lv_enum.next(); // should assert
    delete lp_i1;
    delete lp_i2;
}

void test_map_enum6() {
    SB_Imap lv_map;

    Item *lp_i1 = new_Item(2);
    lv_map.put(&lp_i1->iv_link);
    SB_Imap_Enum lv_enum(&lv_map);
    assert(lv_enum.more());
    int lv_id = lv_enum.next()->iv_id.i;
    assert(lv_id == 2);
    assert(!lv_enum.more());
    assert(lv_enum.next() == NULL);
    delete lp_i1;
}

void test_map_resize() {
    char la_name[20];
    int  lv_p2;
    int  lv_size;

    // check bucket sizes
    for (lv_size = 1; lv_size < 100000; lv_size *= 2) {
        sprintf(la_name, "resize-%d", lv_size);
        SB_Imap *lp_map = new SB_Imap(la_name, lv_size);
        printf("expecting %d buckets\n", lv_size);
        lp_map->printself(false);
        delete lp_map;
    }

    // check resize
    lv_p2 = 1;
    SB_Imap *lp_map = new SB_Imap("resize-check");
    for (lv_size = 1; lv_size < 100000; lv_size++) {
        Item *lp_item = new_Item(lv_size);
        lp_map->put(&lp_item->iv_link);
        if (lv_size == lv_p2) {
            printf("expecting %d items\n", lv_size);
            lp_map->printself(false);
            lv_p2 *= 2;
        }
    }

    // cleanup
    for (lv_size = 1; lv_size < 100000; lv_size++) {
        Item *lp_item = (Item *) lp_map->remove(lv_size);
        delete lp_item;
    }
    delete lp_map;
}

void test_timer_map_cb_ign(long, void *pp_item) {
    assert(pp_item == NULL);
}

void test_timer_map_cb_to(long, void *pp_item) {
    TimerItem *lp_item;

    assert(pp_item != NULL);
    lp_item = (TimerItem *) pp_item;
    assert(!lp_item->iv_called);
    lp_item->iv_called = true;
}

int test_timer_map_walk_count;
void test_timer_map_cb_walk(long, void *) {
    test_timer_map_walk_count++;
}

void test_timer_map() {
    SB_TimerMap *lp_map = new SB_TimerMap("timermap", 50);

    sb_timer_init(); // block signals

    // empty
    int lv_size = lp_map->size();
    assert(lv_size == 0);
    printf("expecting empty map, this=%p\n", (void *) lp_map);
    lp_map->printself(true);
    printf("\n");

    // put 1 item
    TimerItem *lp_i1 = new_TimerItem(1);
    lp_map->put(&lp_i1->iv_link,
                lp_i1,
                1,
                test_timer_map_cb_ign,
                lp_i1->iv_tics);
    lv_size = lp_map->size();
    assert(lv_size == 1);
    printf("expecting 1 item, Item=%p\n", (void *) lp_i1);
    lp_map->printself(true);
    printf("\n");

    // get the item
    TimerItem *lp_i = (TimerItem *) lp_map->get(&lp_i1->iv_link);
    assert(lp_i == lp_i1);
    lp_i = (TimerItem *) lp_map->get_lock(&lp_i1->iv_link, true);
    assert(lp_i == lp_i1);

    // remove 1 item
    lp_i = (TimerItem *) lp_map->remove(&lp_i1->iv_link);
    assert(lp_i == lp_i1);
    lv_size = lp_map->size();
    assert(lv_size == 0);

    lp_i = (TimerItem *) lp_map->remove(&lp_i1->iv_link);
    assert(lp_i == NULL);
    lp_i = (TimerItem *) lp_map->get(&lp_i1->iv_link);
    assert(lp_i == NULL);

    // put 3 items
    TimerItem *lp_i2 = new_TimerItem(2);
    TimerItem *lp_i3 = new_TimerItem(2+256);
    TimerItem *lp_i4 = new_TimerItem(2+256*8192); // hash collision
    lp_map->put(&lp_i2->iv_link,
                lp_i2,
                2,
                test_timer_map_cb_ign,
                lp_i2->iv_tics);
    lp_map->put_lock(&lp_i3->iv_link,
                     lp_i3,
                     3,
                     test_timer_map_cb_ign,
                     lp_i3->iv_tics,
                     true);
    lp_map->put(&lp_i4->iv_link,
                lp_i4,
                4,
                test_timer_map_cb_ign,
                lp_i4->iv_tics);
    lv_size = lp_map->size();
    assert(lv_size == 3);
    printf("expecting 3 items, Item=%p, Item=%p, Item=%p\n",
           (void *) lp_i2, (void *) lp_i3, (void *) lp_i4);
    lp_map->printself(true);
    printf("\n");
    test_timer_map_walk_count = 0;
    lp_map->walk(test_timer_map_cb_walk);
    assert(test_timer_map_walk_count == 3);
    test_timer_map_walk_count = 0;
    lp_map->walk_lock(test_timer_map_cb_walk, true);
    assert(test_timer_map_walk_count == 3);

    // get the item
    lp_i = (TimerItem *) lp_map->get(&lp_i2->iv_link);
    assert(lp_i == lp_i2);
    lp_i = (TimerItem *) lp_map->get(&lp_i3->iv_link);
    assert(lp_i == lp_i3);

    // remove 3 items
    lp_i = (TimerItem *) lp_map->remove(&lp_i2->iv_link);
    assert(lp_i == lp_i2);
    lp_i = (TimerItem *) lp_map->remove_lock(&lp_i3->iv_link, true);
    assert(lp_i == lp_i3);
    lp_i = (TimerItem *) lp_map->remove(&lp_i4->iv_link);
    assert(lp_i == lp_i4);
    lv_size = lp_map->size();
    assert(lv_size == 0);

    TimerItem *lp_i5 = new_TimerItem(5);
    TimerItem *lp_i6 = new_TimerItem(6);
    lp_map->put(&lp_i5->iv_link,
                lp_i5,
                5,
                test_timer_map_cb_to,
                lp_i5->iv_tics);
    lp_map->put(&lp_i6->iv_link,
                lp_i6,
                6,
                test_timer_map_cb_to,
                lp_i6->iv_tics);
    sleep(1);
    assert(lp_i5->iv_called);
    assert(lp_i6->iv_called);
    lp_map->removeall_lock(true);

    // cleanup
    delete lp_map;
    delete lp_i1;
    delete lp_i2;
    delete lp_i3;
    delete lp_i4;
    delete lp_i5;
    delete lp_i6;

}

void test_timer_map_del_cb(long pv_user_param, void *pp_item) {
    pv_user_param = pv_user_param; // touch
    TimerItem *lp_item = (TimerItem *) pp_item;
    delete lp_item;
}

void test_timer_map_enum() {
    SB_TimerMap *lp_map = new SB_TimerMap("timermap-enum", 50);

    // put 3 items
    TimerItem *lp_i1 = new_TimerItem(2);
    TimerItem *lp_i2 = new_TimerItem(3);
    TimerItem *lp_i3 = new_TimerItem(2+256*8192); // hash collision
    lp_map->put(&lp_i1->iv_link,
                lp_i1,
                1,
                test_timer_map_cb_ign,
                lp_i1->iv_tics);
    lp_map->put(&lp_i2->iv_link,
                lp_i2,
                2,
                test_timer_map_cb_ign,
                lp_i2->iv_tics);
    lp_map->put(&lp_i3->iv_link,
                lp_i3,
                3,
                test_timer_map_cb_ign,
                lp_i3->iv_tics);
    SB_TimerMap_Enum *lp_enum = lp_map->keys();
    int lv_inx = 0;
    while (lp_enum->more()) {
        lv_inx++;
        SB_TML_Type *lp_link = lp_enum->next();
        TimerItem *lp_item = (TimerItem *) lp_map->get(lp_link);
        switch (lp_item->iv_tics) {
        case 2:
            assert(lp_item == lp_i1);
            break;
        case 3:
            assert(lp_item == lp_i2);
            break;
        case (2+256*8192):
            assert(lp_item == lp_i3);
            break;
        default:
            assert(false);
        }
    }
    delete lp_enum;
    assert(lv_inx == 3);

    lp_map->removeall_del(test_timer_map_del_cb);
    lp_map->removeall_del_lock(test_timer_map_del_cb, true);
    delete lp_map;

    sb_timer_shutdown();
}

void test_tmap() {
    SB_Tmap<int,Litem> *lp_map = new SB_Tmap<int,Litem>;

    // empty
    int lv_size = lp_map->size();
    assert(lv_size == 0);
    printf("expecting empty map, this=%p\n", (void *) lp_map);
    lp_map->printself(true);
    printf("\n");

    // get a item
    Litem *lp_i = lp_map->get(0);
    assert(lp_i == NULL);

    // put 1 item
    Litem *lp_i1 = new_Litem(1);
    lp_map->put(lp_i1->iv_link.iv_id.i, lp_i1);
    lv_size = lp_map->size();
    assert(lv_size == 1);
    printf("expecting 1 item, Item=%p\n", (void *) lp_i1);
    lp_map->printself(true);
    printf("\n");

    // get the item
    lp_i = lp_map->get(1);
    assert(lp_i == lp_i1);

    // remove 1 item
    lp_i1 = lp_map->remove(1);
    assert(lp_i1->iv_link.iv_id.i == 1);
    lv_size = lp_map->size();
    assert(lv_size == 0);

    // put 3 items
    Litem *lp_i2 = new_Litem(2);
    Litem *lp_i3 = new_Litem(3);
    Litem *lp_i4 = new_Litem(2+61); // hash collision
    lp_map->put(lp_i2->iv_link.iv_id.i, lp_i2);
    lp_map->put(lp_i3->iv_link.iv_id.i, lp_i3);
    lp_map->put(lp_i4->iv_link.iv_id.i, lp_i4);
    lv_size = lp_map->size();
    assert(lv_size == 3);
    printf("expecting 3 items, Item=%p, Item=%p, Item=%p\n",
           (void *) lp_i2, (void *) lp_i3, (void *) lp_i4);
    lp_map->printself(true);
    printf("\n");

    // get the item
    lp_i = lp_map->get(2);
    assert(lp_i == lp_i2);
    lp_i = lp_map->get(3);
    assert(lp_i == lp_i3);

    // remove 3 items
    lp_i2 = lp_map->remove(2);
    lp_i3 = lp_map->remove(3);
    lp_i4 = lp_map->remove(2+61);
    assert(lp_i2->iv_link.iv_id.i == 2);
    assert(lp_i3->iv_link.iv_id.i == 3);
    assert(lp_i4->iv_link.iv_id.i == (2+61));
    lv_size = lp_map->size();
    assert(lv_size == 0);

    // cleanup
    delete lp_map;
    delete lp_i1;
    delete lp_i2;
    delete lp_i3;
    delete lp_i4;
}

void test_tmap_enum() {
    SB_Tmap<int,Litem> lv_map;

    // put 3 items
    Litem *lp_i1 = new_Litem(2);
    Litem *lp_i2 = new_Litem(3);
    Litem *lp_i3 = new_Litem(2+61); // hash collision
    lv_map.put(lp_i1->iv_link.iv_id.i, lp_i1);
    lv_map.put(lp_i2->iv_link.iv_id.i, lp_i2);
    lv_map.put(lp_i3->iv_link.iv_id.i, lp_i3);
    SB_Tmap_Enum<int,Litem> *lp_enum = lv_map.keys();
    int lv_inx = 0;
    while (lp_enum->more()) {
        lv_inx++;
        int lv_id = lp_enum->next()->iv_link.iv_id.i;
        Litem *lp_item = lv_map.get(lv_id);
        switch (lv_id) {
        case 2:
            assert(lp_item == lp_i1);
            break;
        case 3:
            assert(lp_item == lp_i2);
            break;
        case (2+61):
            assert(lp_item == lp_i3);
            break;
        default:
            assert(false);
        }
    }
    delete lp_enum;
    assert(lv_inx == 3);

    lv_map.removeall();

    // cleanup
    delete lp_i1;
    delete lp_i2;
    delete lp_i3;
}

void test_tmap_resize() {
    char la_name[20];
    int  lv_p2;
    int  lv_size;

    // check bucket sizes
    for (lv_size = 1; lv_size < 100000; lv_size *= 2) {
        sprintf(la_name, "resize-%d", lv_size);
        SB_Tmap<int,Litem> *lp_map = new SB_Tmap<int,Litem>(la_name, lv_size);
        printf("expecting %d buckets\n", lv_size);
        lp_map->printself(false);
        delete lp_map;
    }

    // check resize
    lv_p2 = 1;
    SB_Tmap<int,Litem> *lp_map = new SB_Tmap<int,Litem>("resize-check");
    for (lv_size = 1; lv_size < 100000; lv_size++) {
        Litem *lp_item = new_Litem(lv_size);
        lp_map->put(lp_item->iv_link.iv_id.i, lp_item);
        if (lv_size == lv_p2) {
            printf("expecting %d items\n", lv_size);
            lp_map->printself(false);
            lv_p2 *= 2;
        }
    }

    // cleanup
    for (lv_size = 1; lv_size < 100000; lv_size++) {
        Litem *lp_item = lp_map->remove(lv_size);
        delete lp_item;
    }
    delete lp_map;
}

void test_msg_init() {
    int ferr;

    if (!gv_inited) {
        int    lv_argc = 10;
        char **lp_argv;
        char   la_argv[100];
        lp_argv = (char **) &la_argv;
        lp_argv[0] = (char *) "t";
        lp_argv[1] = (char *) "SQMON1.0";
        lp_argv[2] = (char *) "1";
        lp_argv[3] = (char *) "1";
        lp_argv[4] = (char *) "2";
        lp_argv[5] = (char *) "$test";
        lp_argv[6] = (char *) "port";
        lp_argv[7] = (char *) "4";
        lp_argv[8] = (char *) "0";
        lp_argv[9] = (char *) "SPARE";
        ferr = msg_init(&lv_argc, &lp_argv);
        TEST_CHK_FEOK(ferr);
        gv_inited = true;
    }
}

void test_slotmgr() {
    enum        { MAX_ITERS = 1000 };
    enum        { MAX_SLOTS = 10 };
    int           la_slots[MAX_SLOTS];
    int           lv_count;
    int           lv_inx;
    int           lv_iter;
    SB_Slot_Mgr   lv_mgr_min("t1", SB_Slot_Mgr::ALLOC_MIN, MAX_SLOTS);
    int           lv_rnd;

    // fill slots
    lv_inx = 0;
    // lv_mgr_min.print(SB_Slot_Mgr::PRINT_ALL);
    lv_mgr_min.check_min();
    for (lv_count = 0; lv_count < MAX_SLOTS; lv_count++)
        la_slots[lv_inx++] = lv_mgr_min.alloc();
    // lv_mgr_min.print(SB_Slot_Mgr::PRINT_ALL);
    lv_mgr_min.check_min();

    // free slots forwards
    printf("all slots allocated - now free forwards\n");
    for (lv_count = 0; lv_count < MAX_SLOTS; lv_count++) {
        lv_mgr_min.free_slot(la_slots[lv_count]);
        // lv_mgr_min.print(SB_Slot_Mgr::PRINT_ALL);
        lv_mgr_min.check_min();
    }

    // fill slots
    lv_inx = 0;
    for (lv_count = 0; lv_count < MAX_SLOTS; lv_count++)
        la_slots[lv_inx++] = lv_mgr_min.alloc();
    // lv_mgr_min.print(SB_Slot_Mgr::PRINT_ALL);
    lv_mgr_min.check_min();

    // free slots backwards
    printf("all slots allocated - now free backwards\n");
    for (lv_count = MAX_SLOTS-1; lv_count >= 0; lv_count--) {
        lv_mgr_min.free_slot(la_slots[lv_count]);
        // lv_mgr_min.print(SB_Slot_Mgr::PRINT_ALL);
        lv_mgr_min.check_min();
    }

    printf("all slots allocated - now free random\n");
    srand(1);
    for (lv_iter = 0; lv_iter < MAX_ITERS; lv_iter++) {
        // printf("iter=%d\n", lv_iter);
        // fill slots
        lv_inx = 0;
        for (lv_count = 0; lv_count < MAX_SLOTS; lv_count++)
            la_slots[lv_inx++] = lv_mgr_min.alloc();
        lv_mgr_min.check_min();
        // lv_mgr_min.print(SB_Slot_Mgr::PRINT_ALL);

        for (lv_count = 0; lv_count < MAX_SLOTS; lv_count++) {
            // get a random slot
            for (;;) {
                lv_rnd = 1 + (int) ((float) (MAX_SLOTS-1) * (rand_r(&gv_seed) / (RAND_MAX + 1.0)));
                if (la_slots[lv_rnd] >= 0)
                    break;
                if ((lv_count == (MAX_SLOTS - 1)) && (la_slots[0] >= 0)) {
                    lv_rnd = 0;
                    break;
                }
            }
            la_slots[lv_rnd] = -1;
            // free it
            lv_mgr_min.free_slot(lv_rnd);
            lv_mgr_min.check_min();
            // lv_mgr_min.print(SB_Slot_Mgr::PRINT_ALL);
        }
    }
}

void test_slotmgr2() {
    enum        { MAX_ITERS = 1000 };
    enum        { MAX_SLOTS = 10 };
    int           la_slots[MAX_SLOTS];
    int           lv_count;
    int           lv_inx;
    int           lv_iter;
    SB_Slot_Mgr   lv_mgr_fifo("t2", SB_Slot_Mgr::ALLOC_FIFO, MAX_SLOTS);
    int           lv_rnd;

    // fill slots
    lv_inx = 0;
    // lv_mgr_fifo.print(SB_Slot_Mgr::PRINT_ALL);
    for (lv_count = 0; lv_count < MAX_SLOTS; lv_count++)
        la_slots[lv_inx++] = lv_mgr_fifo.alloc();
    // lv_mgr_fifo.print(SB_Slot_Mgr::PRINT_ALL);

    // free slots forwards
    // printf("all slots allocated - now free forwards\n");
    for (lv_count = 0; lv_count < MAX_SLOTS; lv_count++) {
        lv_mgr_fifo.free_slot(la_slots[lv_count]);
        // lv_mgr_fifo.print(SB_Slot_Mgr::PRINT_ALL);
    }

    // fill slots
    lv_inx = 0;
    for (lv_count = 0; lv_count < MAX_SLOTS; lv_count++)
        la_slots[lv_inx++] = lv_mgr_fifo.alloc();
    // lv_mgr_fifo.print(SB_Slot_Mgr::PRINT_ALL);

    // free slots backwards
    // printf("all slots allocated - now free backwards\n");
    for (lv_count = MAX_SLOTS-1; lv_count >= 0; lv_count--) {
        lv_mgr_fifo.free_slot(la_slots[lv_count]);
        // lv_mgr_fifo.print(SB_Slot_Mgr::PRINT_ALL);
    }

    // printf("all slots allocated - now free random\n");
    srand(1);
    for (lv_iter = 0; lv_iter < MAX_ITERS; lv_iter++) {
        // printf("iter=%d\n", lv_iter);
        // fill slots
        lv_inx = 0;
        for (lv_count = 0; lv_count < MAX_SLOTS; lv_count++)
            la_slots[lv_inx++] = lv_mgr_fifo.alloc();
        // lv_mgr_fifo.print(SB_Slot_Mgr::PRINT_ALL);

        for (lv_count = 0; lv_count < MAX_SLOTS; lv_count++) {
            // get a random slot
            for (;;) {
                lv_rnd = 1 + (int) ((float) (MAX_SLOTS-1) * (rand_r(&gv_seed) / (RAND_MAX + 1.0)));
                if (la_slots[lv_rnd] >= 0)
                    break;
                if ((lv_count == (MAX_SLOTS - 1)) && (la_slots[0] >= 0)) {
                    lv_rnd = 0;
                    break;
                }
            }
            la_slots[lv_rnd] = -1;
            // free it
            lv_mgr_fifo.free_slot(lv_rnd);
            // lv_mgr_fifo.print(SB_Slot_Mgr::PRINT_ALL);
        }
    }
}

void test_slotmgr3() {
    enum        { MAX_SLOTS = 10 };
    int           la_slots[MAX_SLOTS];
    int           lv_count;
    int           lv_inx;
    int           lv_inx2;
    SB_Slot_Mgr   lv_mgr_fifo("t3", SB_Slot_Mgr::ALLOC_FIFO, MAX_SLOTS);
    int           lv_slot;

    for (lv_inx2 = 0; lv_inx2 < MAX_SLOTS; lv_inx2++) {
        // fill slots
        lv_inx = 0;
        // lv_mgr_fifo.print(SB_Slot_Mgr::PRINT_ALL);
        for (lv_count = 0; lv_count < MAX_SLOTS; lv_count++)
            la_slots[lv_inx++] = lv_mgr_fifo.alloc();
        // lv_mgr_fifo.print(SB_Slot_Mgr::PRINT_ALL);

        // free slots forwards
        // printf("all slots allocated - now free forwards\n");
        for (lv_count = 0; lv_count < MAX_SLOTS; lv_count++) {
            lv_mgr_fifo.free_slot(la_slots[lv_count]);
            // lv_mgr_fifo.print(SB_Slot_Mgr::PRINT_ALL);
        }

        // alloc/free
        // printf("alloc/free - rotate\n");
        lv_slot = lv_mgr_fifo.alloc();
        // lv_mgr_fifo.print(SB_Slot_Mgr::PRINT_ALL);
        lv_mgr_fifo.free_slot(lv_slot);
        // lv_mgr_fifo.print(SB_Slot_Mgr::PRINT_ALL);
    }
}

void test_slotmgr4() {
    enum        { MAX_SLOTS = 10 };
    int           la_slots[MAX_SLOTS*2];
    int           lv_count;
    int           lv_inx;
    SB_Slot_Mgr   lv_mgr_fifo("t4", SB_Slot_Mgr::ALLOC_FIFO, MAX_SLOTS);

    // fill slots
    lv_inx = 0;
    // lv_mgr_fifo.print(SB_Slot_Mgr::PRINT_ALL);
    for (lv_count = 0; lv_count < MAX_SLOTS; lv_count++)
        la_slots[lv_inx++] = lv_mgr_fifo.alloc();
    lv_mgr_fifo.resize(2*MAX_SLOTS);
    for (lv_count = MAX_SLOTS; lv_count < MAX_SLOTS*2; lv_count++)
        la_slots[lv_inx++] = lv_mgr_fifo.alloc();
    // lv_mgr_fifo.print(SB_Slot_Mgr::PRINT_ALL);

    // printf("all slots allocated - now free forwards\n");
    for (lv_count = 0; lv_count < MAX_SLOTS*2; lv_count++) {
        lv_mgr_fifo.free_slot(la_slots[lv_count]);
        // lv_mgr_fifo.print(SB_Slot_Mgr::PRINT_ALL);
    }

    // fill slots
    lv_inx = 0;
    for (lv_count = 0; lv_count < MAX_SLOTS; lv_count++)
        la_slots[lv_inx++] = lv_mgr_fifo.alloc();
    lv_mgr_fifo.resize(2*MAX_SLOTS);
    for (lv_count = MAX_SLOTS; lv_count < MAX_SLOTS*2; lv_count++)
        la_slots[lv_inx++] = lv_mgr_fifo.alloc();
    // lv_mgr_fifo.print(SB_Slot_Mgr::PRINT_ALL);

    // free slots backwards
    // printf("all slots allocated - now free backwards\n");
    for (lv_count = MAX_SLOTS*2-1; lv_count >= 0; lv_count--) {
        lv_mgr_fifo.free_slot(la_slots[lv_count]);
        // lv_mgr_fifo.print(SB_Slot_Mgr::PRINT_ALL);
    }
}

void test_slotmgr5() {
    enum        { MAX_SLOTS = 10 };
    int           la_slots[MAX_SLOTS*2];
    int           lv_count;
    int           lv_inx;
    SB_Slot_Mgr   lv_mgr_fifo("t4", SB_Slot_Mgr::ALLOC_FIFO, MAX_SLOTS);

    // fill slots
    lv_inx = 0;
    // lv_mgr_fifo.print(SB_Slot_Mgr::PRINT_ALL);
    for (lv_count = 0; lv_count < MAX_SLOTS; lv_count++)
        la_slots[lv_inx++] = lv_mgr_fifo.alloc();
    lv_mgr_fifo.resize(2*MAX_SLOTS);
    // lv_mgr_fifo.print(SB_Slot_Mgr::PRINT_ALL);

    // printf("all slots allocated - now free forwards\n");
    for (lv_count = 0; lv_count < MAX_SLOTS; lv_count++) {
        lv_mgr_fifo.free_slot(la_slots[lv_count]);
        // lv_mgr_fifo.print(SB_Slot_Mgr::PRINT_ALL);
    }
}

void *t_sm_ts(void *pp_arg) {
    enum        { MAX_ITERS = 1000 };
    enum        { MAX_SLOTS = 10 };
    static int             la_slots[MAX_SLOTS];
    int                    la_slots_thr[MAX_SLOTS];
    static SB_Ts_Slot_Mgr  lv_mgr_min("t1", SB_Slot_Mgr::ALLOC_MIN, MAX_SLOTS);
    int                    lv_count;
    int                    lv_inx;
    static bool            lv_init = true;
    SB_Thread::Mutex       lv_lock;
    int                    lv_iter;
    int                    lv_max;
    int                    lv_rnd;
    int                    lv_size;
    int                    lv_slot;
    int                    lv_slots_used;
    int                    lv_status;

    pp_arg = pp_arg; // touch

    // init globals
    lv_status = lv_lock.lock();
    assert(lv_status == 0);
    if (lv_init) {
        for (lv_slot = 0; lv_slot < MAX_SLOTS; lv_slot++) {
            la_slots[lv_slot] = -1;
            la_slots[lv_slot] = la_slots[lv_slot]; // touch
        }
        lv_init = false;
    }
    lv_status = lv_lock.unlock();
    assert(lv_status == 0);

    // init thread
    lv_slots_used = 0;
    for (lv_slot = 0; lv_slot < MAX_SLOTS; lv_slot++)
        la_slots_thr[lv_slot] = -1;
    lv_max = MAX_ITERS;
    if (gv_vg) {
        //
        // there seems to be a problem in valgrind
        // at some point, the test deadlocks with one thread stuck in lock,
        // but the sl is -1.
        //
        lv_max /= 10;
    }
    for (lv_iter = 0; lv_iter < lv_max; lv_iter++) {
        lv_count = 1 + (int) ((float) (MAX_SLOTS-1) * (rand_r(&gv_seed) / (RAND_MAX + 1.0)));
        // printf("tid=%d, alloc-count=%d\n", gettid(), lv_count);
        for (lv_inx = 0; lv_inx < lv_count; lv_inx++) {
            lv_size = lv_mgr_min.size();
            lv_size = lv_size; // touch
            lv_slot = lv_mgr_min.alloc_if_cap();
            if (lv_slot < 0)
                break;
            lv_slots_used++;
            la_slots[lv_slot] = lv_slot;
            la_slots_thr[lv_slot] = lv_slot;
            // lv_mgr_min.print(SB_Slot_Mgr::PRINT_ALL);
        }
        lv_mgr_min.check_min();

        sched_yield();
        lv_count = 1 + (int) ((float) (MAX_SLOTS-1) * (rand_r(&gv_seed) / (RAND_MAX + 1.0)));
        // printf("tid=%d, free-count=%d\n", gettid(), lv_count);
        for (lv_inx = 0; lv_inx < lv_count; lv_inx++) {
            if (lv_slots_used <= 0)
                break;
            // get a random slot
            for (;;) {
                lv_rnd = 1 + (int) ((float) (MAX_SLOTS-1) * (rand_r(&gv_seed) / (RAND_MAX + 1.0)));
                if (la_slots_thr[lv_rnd] >= 0)
                    break;
                if ((lv_slots_used == 1) && (la_slots_thr[0] >= 0)) {
                    lv_rnd = 0;
                    break;
                }
            }
            la_slots_thr[lv_rnd] = -1;
            la_slots[lv_rnd] = -1;
            // free it
            lv_mgr_min.free_slot(lv_rnd);
            lv_slots_used--;
            lv_mgr_min.check_min();
            // lv_mgr_min.print(SB_Slot_Mgr::PRINT_ALL);
        }
    }

    return NULL;
}

void test_slotmgr_ts() {
    enum        { MAX_THR = 10 };
    SB_Thread::Sthr::Id_Ptr la_thr[MAX_THR];
    char                    la_thr_name[10];
    int                     lv_thr;

    for (lv_thr = 0; lv_thr < MAX_THR; lv_thr++) {
        sprintf(la_thr_name, "sm_ts_%d", lv_thr + 1);
        printf("launching %s\n", la_thr_name);
        la_thr[lv_thr] =
          SB_Thread::Sthr::create(la_thr_name, t_sm_ts, (void *) (long) (lv_thr + 1));

    }

    void *lp_value;
    for (lv_thr = 0; lv_thr < MAX_THR; lv_thr++) {
        sprintf(la_thr_name, "sm_ts_%d", lv_thr + 1);
        int lv_status = SB_Thread::Sthr::join(la_thr[lv_thr], &lp_value);
        assert(lv_status == 0);
        printf("joined with %s\n", la_thr_name);
    }

    // cleanup
    for (lv_thr = 0; lv_thr < MAX_THR; lv_thr++)
        SB_Thread::Sthr::delete_id(la_thr[lv_thr]);
}

void test_smap_enum() {
    SB_Smap lv_map;

    // put 3 items
    const char *lp_k1 = "2";
    const char *lp_v1 = lp_k1;
    const char *lp_k2 = "3";
    const char *lp_v2 = lp_k2;
    const char  lv_k3 = '2' + 64;
    const char  la_k3[2] = { lv_k3, '\0' };
    const char *lp_v3 = la_k3;
    lv_map.put(lp_k1, lp_v1);
    lv_map.put(lp_k2, lp_v2);
    lv_map.put(la_k3, lp_v3);

    SB_Smap_Enum *lp_enum = lv_map.keys();
    delete lp_enum;
    SB_Smap_Enum lv_enum(&lv_map);
    int lv_inx = 0;
    while (lv_enum.more()) {
        lv_inx++;
        char *lp_key = lv_enum.next();
        const char *lp_value = lv_map.get(lp_key);
        if (strcmp(lp_key, lp_k1) == 0)
            assert(strcmp(lp_value, lp_v1) == 0);
        else if (strcmp(lp_key, lp_k2) == 0)
            assert(strcmp(lp_value, lp_v2) == 0);
        else if (strcmp(lp_key, la_k3) == 0)
            assert(strcmp(lp_value, lp_v3) == 0);
        else
            assert(false);
    }
    assert(lv_inx == 3);
}

void test_smap_map(SB_Smap *pp_map) {

    // empty
    int lv_size = pp_map->size();
    assert(lv_size == 0);
    printf("expecting empty map, this=%p\n", (void *) pp_map);
    pp_map->printself(true);
    printf("\n");

    // get a value
    const char *lp_v = pp_map->get("");
    assert(lp_v == NULL);

    // put 1 key/value
    const char *lp_k1 = "1";
    const char *lp_v1 = "v1";
    pp_map->put(lp_k1, lp_v1);
    lv_size = pp_map->size();
    assert(lv_size == 1);
    printf("expecting 1 item, key=%s, value=%s\n", lp_k1, lp_v1);
    pp_map->printself(true);
    printf("\n");

    // get the item
    lp_v = pp_map->get("");
    assert(lp_v == NULL);
    lp_v = pp_map->get(lp_k1);
    assert(strcmp(lp_v, lp_v1) == 0);

    // remove 1 item
    char la_value[100];
    pp_map->remove(lp_k1, la_value);
    assert(strcmp(la_value, lp_v1) == 0);
    lv_size = pp_map->size();
    assert(lv_size == 0);

    // put 3 items
    const char *lp_k2 = "2";
    const char *lp_v2 = lp_k2;
    const char *lp_k3 = "3";
    const char *lp_v3 = lp_k3;
    const char  lv_k4 = '2' + 61;
    const char  la_k4[2] = { lv_k4, '\0' };
    const char *lp_v4 = la_k4;
    pp_map->put(lp_k2, lp_v2);
    pp_map->put(lp_k3, lp_v3);
    pp_map->put(la_k4, lp_v4);
    lv_size = pp_map->size();
    assert(lv_size == 3);
    printf("expecting 3 items, Item=%s/%s, Item=%s/%s, Item=%s/%s\n",
           lp_k2, lp_v2, lp_k3, lp_v3, la_k4, lp_v4);
    pp_map->printself(true);
    printf("\n");

    // get the item
    lp_v = pp_map->get(lp_k2);
    assert(strcmp(lp_v, lp_v2) == 0);
    lp_v = pp_map->get(lp_k3);
    assert(strcmp(lp_v, lp_v3) == 0);

    // remove 3 items
    char la_value2[100];
    char la_value3[100];
    char la_value4[100];
    pp_map->remove(lp_k2, la_value2);
    pp_map->remove(lp_k3, la_value3);
    pp_map->remove(la_k4, la_value4);
    assert(strcmp(la_value2, lp_v2) == 0);
    assert(strcmp(la_value3, lp_v3) == 0);
    assert(strcmp(la_value4, lp_v4) == 0);
    lv_size = pp_map->size();
    assert(lv_size == 0);

    // test value

    // put 1 key/value
    const char *lp_vk1 = "v1";
    const char *lp_vv1 = "vv1";
    pp_map->putv(lp_vk1, (void *) lp_vv1);
    lv_size = pp_map->size();
    assert(lv_size == 1);
    printf("expecting 1 item, key=%s, value=%s\n", lp_vk1, lp_vv1);
    pp_map->printself(true);
    printf("\n");

    // get the item
    void *lp_vv = pp_map->getv("");
    assert(lp_vv == NULL);
    lp_vv = pp_map->getv(lp_vk1);
    assert(lp_vv != NULL);
    assert(strcmp((char *) lp_vv, lp_vv1) == 0);

    // remove 1 item
    lp_vv = pp_map->removev(lp_vk1);
    assert(strcmp((char *) lp_vv, lp_vv1) == 0);
    lv_size = pp_map->size();
    assert(lv_size == 0);
}

void test_smap() {
    SB_Smap lv_map;
    test_smap_map(&lv_map);

    // test destructor
    SB_Smap *lp_map = new SB_Smap;
    const char *lp_k2 = "2";
    const char *lp_v2 = lp_k2;
    const char *lp_k3 = "3";
    const char *lp_v3 = lp_k3;
    const char  lv_k4 = '2' + 64;
    const char  la_k4[2] = { lv_k4, '\0' };
    const char *lp_v4 = la_k4;
    lp_map->put(lp_k2, lp_v2);
    lp_map->put(lp_k3, lp_v3);
    lp_map->put(la_k4, lp_v4);
    delete lp_map;
}

void test_smap_resize() {
    char la_k[10];
    char la_name[20];
    char la_v[10];
    int  lv_p2;
    int  lv_size;

    // check bucket sizes
    for (lv_size = 1; lv_size < 100000; lv_size *= 2) {
        sprintf(la_name, "resize-%d", lv_size);
        SB_Smap *lp_map = new SB_Smap(la_name, lv_size);
        printf("expecting %d buckets\n", lv_size);
        lp_map->printself(false);
        delete lp_map;
    }

    // check resize
    lv_p2 = 1;
    SB_Smap *lp_map = new SB_Smap("resize-check");
    for (lv_size = 1; lv_size < 100000; lv_size++) {
        sprintf(la_k, "k%d", lv_size);
        sprintf(la_v, "v%d", lv_size);
        lp_map->put(la_k, la_v);
        if (lv_size == lv_p2) {
            printf("expecting %d items\n", lv_size);
            lp_map->printself(false);
            lv_p2 *= 2;
        }
    }

    // cleanup
    for (lv_size = 1; lv_size < 100000; lv_size++) {
        sprintf(la_k, "k%d", lv_size);
        lp_map->remove(la_k, NULL);
    }
    delete lp_map;
}

void test_props() {
    SB_Props lv_Props;
    test_smap_map(&lv_Props);

    SB_Props lv_props_file;
    lv_props_file.load("t9.props");
    assert(lv_props_file.size() == 3);
    lv_props_file.printself(true);
    assert(lv_props_file.get("") == NULL);
    assert(strcmp(lv_props_file.get("k1"), "v1") == 0);
    assert(strcmp(lv_props_file.get("k2"), "v2") == 0);
    assert(strcmp(lv_props_file.get("k3"), "v3") == 0);
    lv_props_file.store("zjunk");

    // test destructor
    const char *lp_k2 = "2";
    const char *lp_v2 = lp_k2;
    const char *lp_k3 = "3";
    const char *lp_v3 = lp_k3;
    const char  lv_k4 = '2' + 64;
    const char  la_k4[2] = { lv_k4, '\0' };
    const char *lp_v4 = la_k4;
    SB_Props *lp_props = new SB_Props;
    lp_props->put(lp_k2, lp_v2);
    lp_props->put(lp_k3, lp_v3);
    lp_props->put(la_k4, lp_v4);
    delete lp_props;
    unlink("zjunk");
}

void test_mutexes() {
    SB_Thread::Mutex *lp_m = new SB_Thread::Mutex();
    assert(lp_m->lock() == 0);
    assert(lp_m->unlock() == 0);
    assert(lp_m->trylock() == 0);
    assert(lp_m->unlock() == 0);
    assert(lp_m->lock() == 0);
    assert(lp_m->trylock());
    assert(lp_m->unlock() == 0);
    delete lp_m;

    SB_Thread::Preemptive_Mutex *lp_pm = new SB_Thread::Preemptive_Mutex();
    assert(lp_pm->lock() == 0);
    assert(lp_pm->unlock() == 0);
    assert(lp_pm->trylock() == 0);
    assert(lp_pm->unlock() == 0);
    assert(lp_pm->lock() == 0);
    assert(lp_pm->trylock());
    assert(lp_pm->unlock() == 0);
    delete lp_pm;

    SB_Thread::Errorcheck_Mutex *lp_ecm = new SB_Thread::Errorcheck_Mutex();
    assert(!lp_ecm->locked());
    assert(lp_ecm->lock() == 0);
    assert(lp_ecm->locked());
    assert(lp_ecm->lock() == EDEADLK); // double lock
    assert(lp_ecm->unlock() == 0);
    assert(!lp_ecm->locked());
    assert(lp_ecm->unlock() == EPERM); // double unlock
    assert(lp_ecm->trylock() == 0);
    if (gv_rh5)
        assert(lp_ecm->trylock() == EDEADLK); // double lock
    else
        assert(lp_ecm->trylock() == EBUSY); // double lock
    assert(lp_ecm->unlock() == 0);
    assert(lp_ecm->lock() == 0);
    assert(lp_ecm->trylock());
    assert(lp_ecm->unlock() == 0);
    delete lp_ecm;

    SB_Thread::Errorcheck_Mutex *lp_ecm2 =
      new SB_Thread::Errorcheck_Mutex(true);
    assert(lp_ecm2->lock() == 0);
    delete lp_ecm2; // should unlock
}

void test_queue() {
    SB_Sig_Queue *lp_q = new SB_Sig_Queue("queue-sig", false);

    // empty
    size_t lv_size = lp_q->size();
    assert(lv_size == 0);
    printf("expecting empty Q, this=%p\n", (void *) lp_q);
    lp_q->printself(true);
    printf("\n");

    // add 1 item
    Item *lp_i1 = new_Item(1);
    lp_q->add(&lp_i1->iv_link);
    lv_size = lp_q->size();
    assert(lv_size == 1);
    printf("expecting 1 item, Item=%p\n", (void *) lp_i1);
    lp_q->printself(true);
    printf("\n");

    // remove 1 item
    lp_i1 = (Item *) lp_q->remove();
    assert(lp_i1->iv_link.iv_id.i == 1);
    lv_size = lp_q->size();
    assert(lv_size == 0);

    // add 2 items
    Item *lp_i2 = new_Item(2);
    Item *lp_i3 = new_Item(3);
    lp_q->add(&lp_i2->iv_link);
    lp_q->add(&lp_i3->iv_link);
    lv_size = lp_q->size();
    assert(lv_size == 2);
    printf("expecting 2 items, Item=%p, Item=%p\n",
           (void *) lp_i2, (void *) lp_i3);
    lp_q->printself(true);
    printf("\n");

    // remove 2 items
    lp_i2 = (Item *) lp_q->remove();
    lp_i3 = (Item *) lp_q->remove();
    assert(lp_i2->iv_link.iv_id.i == 2);
    assert(lp_i3->iv_link.iv_id.i == 3);
    lv_size = lp_q->size();
    assert(lv_size == 0);

    // check lock/unlock
    lp_q->lock();
    lp_q->unlock();

    // cleanup
    delete lp_q;
    delete lp_i1;
    delete lp_i2;
    delete lp_i3;
}

void test_queue2() {
    SB_Queue *lp_q = new SB_Queue("queue2-q");

    // empty
    size_t lv_size = lp_q->size();
    assert(lv_size == 0);
    printf("expecting empty Q, this=%p\n", (void *) lp_q);
    lp_q->printself(true);
    printf("\n");

    // add 1 item/remove
    Item *lp_i1 = new_Item(1);
    lp_q->add(&lp_i1->iv_link);
    lp_q->remove();
    lv_size = lp_q->size();
    assert(lv_size == 0);
    printf("expecting empty Q, this=%p\n", (void *) lp_q);
    lp_q->printself(true);
    printf("\n");
    delete lp_i1;

    // add 2 items/remove
    Item *lp_i2 = new_Item(2);
    Item *lp_i3 = new_Item(3);
    lp_q->add(&lp_i2->iv_link);
    lp_q->add(&lp_i3->iv_link);
    lv_size = lp_q->size();
    assert(lv_size == 2);
    printf("expecting 2 items, Item=%p, Item=%p\n",
           (void *) lp_i2, (void *) lp_i3);
    lp_q->printself(true);
    printf("\n");

    lp_q->remove();
    lv_size = lp_q->size();
    assert(lv_size == 1);
    printf("expecting 1 item, Item=%p\n", (void *) lp_i3);
    lp_q->printself(true);
    printf("\n");
    lp_q->remove();
    lv_size = lp_q->size();
    assert(lv_size == 0);
    printf("expecting empty Q, this=%p\n", (void *) lp_q);
    lp_q->printself(true);
    printf("\n");

    // cleanup
    delete lp_q;
    delete lp_i2;
    delete lp_i3;
}

void test_queue3() {
    SB_D_Queue *lp_q = new SB_D_Queue(1, "queue3-d");
    bool        lv_on_list;

    // empty
    size_t lv_size = lp_q->size();
    assert(lv_size == 0);
    printf("expecting empty Q, this=%p\n", (void *) lp_q);
    lp_q->printself(true);
    printf("\n");

    // add 1 item/remove
    DItem *lp_i1 = new_DItem(1);
    lp_q->add(&lp_i1->iv_link);
    lv_on_list = lp_q->remove_list(&lp_i1->iv_link);
    assert(lv_on_list);
    lv_size = lp_q->size();
    assert(lv_size == 0);
    printf("expecting empty Q, this=%p\n", (void *) lp_q);
    lp_q->printself(true);
    printf("\n");
    delete lp_i1;

    // add 2 items/remove
    DItem *lp_i2 = new_DItem(2);
    DItem *lp_i3 = new_DItem(3);
    lp_q->add(&lp_i2->iv_link);
    lp_q->add(&lp_i3->iv_link);
    lv_size = lp_q->size();
    assert(lv_size == 2);
    printf("expecting 2 items, Item=%p, Item=%p\n",
           (void *) lp_i2, (void *) lp_i3);
    lp_q->printself(true);
    printf("\n");

    lv_on_list = lp_q->remove_list(&lp_i3->iv_link);
    assert(lv_on_list);
    lv_size = lp_q->size();
    assert(lv_size == 1);
    printf("expecting 1 item, Item=%p\n", (void *) lp_i2);
    lp_q->printself(true);
    printf("\n");
    lv_on_list = lp_q->remove_list(&lp_i2->iv_link);
    assert(lv_on_list);
    lv_size = lp_q->size();
    assert(lv_size == 0);
    printf("expecting empty Q, this=%p\n", (void *) lp_q);
    lp_q->printself(true);
    printf("\n");

    // cleanup
    delete lp_q;
    delete lp_i2;
    delete lp_i3;
}

void test_queue4() {
    SB_DQL_Type lv_item1;
    SB_DQL_Type lv_item2;
    SB_DQL_Type lv_item3;
    bool        lv_on_list;
    SB_D_Queue  lv_q(1, "queue4-d");

    lv_item1.iv_id.ll = 1;;
    lv_item2.iv_id.ll = 2;;
    lv_item3.iv_id.ll = 3;;
    lv_q.add(&lv_item1);
    lv_q.add(&lv_item2);
    lv_q.add(&lv_item3);
    lv_on_list = lv_q.remove_list(&lv_item3);
    assert(lv_on_list);
    lv_on_list = lv_q.remove_list(&lv_item1);
    assert(lv_on_list);
    lv_on_list = lv_q.remove_list(&lv_item2);
    assert(lv_on_list);
    lv_on_list = lv_q.remove_list(&lv_item1);
    assert(!lv_on_list);
    lv_on_list = lv_q.remove_list(&lv_item2);
    assert(!lv_on_list);
    lv_on_list = lv_q.remove_list(&lv_item3);
    assert(!lv_on_list);
}

void test_queue5() {
    SB_QL_Type  *lp_item;
    SB_QL_Type   lv_item1;
    SB_QL_Type   lv_item2;
    SB_QL_Type   lv_item3;
    SB_Lf_Queue  lv_q("queue5-lf");

    lv_item1.iv_id.ll = 1;;
    lv_item2.iv_id.ll = 2;;
    lv_item3.iv_id.ll = 3;;
    assert(lv_q.empty());
    assert(lv_q.size() == 0);
    lv_q.add(&lv_item1);
    assert(lv_q.size() == 1);
    assert(!lv_q.empty());
    lv_q.add(&lv_item2);
    assert(lv_q.size() == 2);
    lv_q.add(&lv_item3);
    assert(lv_q.size() == 3);

    lp_item = (SB_QL_Type *) lv_q.remove();
    assert(lp_item == &lv_item1);
    assert(lp_item->iv_id.ll == lv_item1.iv_id.ll);
    assert(lv_q.size() == 2);
    lp_item = (SB_QL_Type *) lv_q.remove();
    assert(lp_item == &lv_item2);
    assert(lp_item->iv_id.ll == lv_item2.iv_id.ll);
    assert(lv_q.size() == 1);
    lp_item = (SB_QL_Type *) lv_q.remove();
    assert(lp_item == &lv_item3);
    assert(lp_item->iv_id.ll == lv_item3.iv_id.ll);
    assert(lv_q.size() == 0);
    assert(lv_q.empty());
}

void test_queue6() {
    SB_Ts_Queue *lp_q = new SB_Ts_Queue("queue7-ts");

    // empty
    size_t lv_size = lp_q->size();
    assert(lv_size == 0);
    printf("expecting empty Q, this=%p\n", (void *) lp_q);
    lp_q->printself(true);
    printf("\n");

    // add 1 item/remove
    Item *lp_i1 = new_Item(1);
    lp_q->add(&lp_i1->iv_link);
    lp_q->remove();
    lv_size = lp_q->size();
    assert(lv_size == 0);
    printf("expecting empty Q, this=%p\n", (void *) lp_q);
    lp_q->printself(true);
    printf("\n");
    delete lp_i1;

    // add 2 items/remove
    Item *lp_i2 = new_Item(2);
    Item *lp_i3 = new_Item(3);
    lp_q->add(&lp_i2->iv_link);
    lp_q->add(&lp_i3->iv_link);
    lv_size = lp_q->size();
    assert(lv_size == 2);
    printf("expecting 2 items, Item=%p, Item=%p\n",
           (void *) lp_i2, (void *) lp_i3);
    lp_q->printself(true);
    printf("\n");

    lp_q->remove();
    lv_size = lp_q->size();
    assert(lv_size == 1);
    printf("expecting 1 item, Item=%p\n", (void *) lp_i3);
    lp_q->printself(true);
    printf("\n");
    lp_q->remove();
    lv_size = lp_q->size();
    assert(lv_size == 0);
    printf("expecting empty Q, this=%p\n", (void *) lp_q);
    lp_q->printself(true);
    printf("\n");

    // cleanup
    delete lp_q;
    delete lp_i2;
    delete lp_i3;
}

void test_queue7() {
    SB_Ts_D_Queue *lp_q = new SB_Ts_D_Queue(1, "queue7-tsd");

    // empty
    size_t lv_size = lp_q->size();
    assert(lv_size == 0);
    printf("expecting empty Q, this=%p\n", (void *) lp_q);
    lp_q->printself(true, true);
    printf("\n");

    // add 1 item/remove
    DItem *lp_i1 = new_DItem(1);
    lp_q->add(&lp_i1->iv_link);
    lp_q->remove();
    lv_size = lp_q->size();
    assert(lv_size == 0);
    printf("expecting empty Q, this=%p\n", (void *) lp_q);
    lp_q->printself(true, true);
    printf("\n");
    delete lp_i1;

    // add 2 items/remove
    DItem *lp_i2 = new_DItem(2);
    DItem *lp_i3 = new_DItem(3);
    lp_q->add(&lp_i2->iv_link);
    lp_q->add(&lp_i3->iv_link);
    lv_size = lp_q->size();
    assert(lv_size == 2);
    printf("expecting 2 items, Item=%p, Item=%p\n",
           (void *) lp_i2, (void *) lp_i3);
    lp_q->printself(true, true);
    printf("\n");

    lp_q->remove();
    lv_size = lp_q->size();
    assert(lv_size == 1);
    printf("expecting 1 item, Item=%p\n", (void *) lp_i3);
    lp_q->printself(true, true);
    printf("\n");
    lp_q->remove();
    lv_size = lp_q->size();
    assert(lv_size == 0);
    printf("expecting empty Q, this=%p\n", (void *) lp_q);
    lp_q->printself(true, true);
    printf("\n");

    // cleanup
    delete lp_q;
    delete lp_i2;
    delete lp_i3;
}

void test_queue8() {
    SB_Sig_Queue *lp_q = new SB_Sig_Queue("queue8-sig", false);

    // empty
    size_t lv_size = lp_q->size();
    assert(lv_size == 0);
    printf("expecting empty Q, this=%p\n", (void *) lp_q);
    lp_q->printself(true);
    printf("\n");

    // add 1 item/remove
    Item *lp_i1 = new_Item(1);
    lp_q->add(&lp_i1->iv_link);
    lp_q->remove();
    lv_size = lp_q->size();
    assert(lv_size == 0);
    printf("expecting empty Q, this=%p\n", (void *) lp_q);
    lp_q->printself(true);
    printf("\n");
    delete lp_i1;

    // add 2 items/remove
    Item *lp_i2 = new_Item(2);
    Item *lp_i3 = new_Item(3);
    lp_q->add(&lp_i2->iv_link);
    lp_q->add(&lp_i3->iv_link);
    lv_size = lp_q->size();
    assert(lv_size == 2);
    printf("expecting 2 items, Item=%p, Item=%p\n",
           (void *) lp_i2, (void *) lp_i3);
    lp_q->printself(true);
    printf("\n");

    lp_q->remove();
    lv_size = lp_q->size();
    assert(lv_size == 1);
    printf("expecting 1 item, Item=%p\n", (void *) lp_i3);
    lp_q->printself(true);
    printf("\n");
    lp_q->remove();
    lv_size = lp_q->size();
    assert(lv_size == 0);
    printf("expecting empty Q, this=%p\n", (void *) lp_q);
    lp_q->printself(true);
    printf("\n");

    // cleanup
    delete lp_q;
    delete lp_i2;
    delete lp_i3;
}

void test_queue9() {
    SB_Sig_D_Queue *lp_q = new SB_Sig_D_Queue(1, "queue9-sigd", false);

    // empty
    size_t lv_size = lp_q->size();
    assert(lv_size == 0);
    printf("expecting empty Q, this=%p\n", (void *) lp_q);
    lp_q->printself(true, true);
    printf("\n");

    // add 1 item/remove
    DItem *lp_i1 = new_DItem(1);
    lp_q->add(&lp_i1->iv_link);
    lp_q->remove();
    lv_size = lp_q->size();
    assert(lv_size == 0);
    printf("expecting empty Q, this=%p\n", (void *) lp_q);
    lp_q->printself(true, true);
    printf("\n");
    delete lp_i1;

    // add 2 items/remove
    DItem *lp_i2 = new_DItem(2);
    DItem *lp_i3 = new_DItem(3);
    lp_q->add(&lp_i2->iv_link);
    lp_q->add(&lp_i3->iv_link);
    lv_size = lp_q->size();
    assert(lv_size == 2);
    printf("expecting 2 items, Item=%p, Item=%p\n",
           (void *) lp_i2, (void *) lp_i3);
    lp_q->printself(true, true);
    printf("\n");

    lp_q->remove();
    lv_size = lp_q->size();
    assert(lv_size == 1);
    printf("expecting 1 item, Item=%p\n", (void *) lp_i3);
    lp_q->printself(true, true);
    printf("\n");
    lp_q->remove();
    lv_size = lp_q->size();
    assert(lv_size == 0);
    printf("expecting empty Q, this=%p\n", (void *) lp_q);
    lp_q->printself(true, true);
    printf("\n");

    // cleanup
    delete lp_q;
    delete lp_i2;
    delete lp_i3;
}

void test_queue10() {
    SB_Queue       lv_q("queue10-q");
    SB_D_Queue     lv_qd(1, "queue10-d");
    SB_Ts_Queue    lv_qts("queue10-ts");
    SB_Ts_D_Queue  lv_qtsd(2, "queue10-tsd");
    SB_Sig_Queue   lv_qsig("queue10-sig", false);
    SB_Sig_D_Queue lv_qsigd(3, "queue10-sigd", false);
    SB_Lf_Queue    lv_qlf("queue10-qlf");

    int            lv_cnt;
    size_t         lv_size;
    Item           lv_q_i1;
    Item           lv_q_i2;
    DItem          lv_qd_i1;
    DItem          lv_qd_i2;
    Item           lv_qts_i1;
    Item           lv_qts_i2;
    DItem          lv_qtsd_i1;
    DItem          lv_qtsd_i2;
    Item           lv_qsig_i1;
    Item           lv_qsig_i2;
    DItem          lv_qsigd_i1;
    DItem          lv_qsigd_i2;
    Item           lv_qlf_i1;
    Item           lv_qlf_i2;

    lv_cnt = 1;
    lv_q_i1.iv_link.iv_id.i = lv_cnt++;
    lv_q_i2.iv_link.iv_id.i = lv_cnt++;
    lv_qd_i1.iv_link.iv_id.i = lv_cnt++;
    lv_qd_i2.iv_link.iv_id.i = lv_cnt++;
    lv_qts_i1.iv_link.iv_id.i = lv_cnt++;
    lv_qts_i2.iv_link.iv_id.i = lv_cnt++;
    lv_qtsd_i1.iv_link.iv_id.i = lv_cnt++;
    lv_qtsd_i2.iv_link.iv_id.i = lv_cnt++;
    lv_qsig_i1.iv_link.iv_id.i = lv_cnt++;
    lv_qsig_i2.iv_link.iv_id.i = lv_cnt++;
    lv_qsigd_i1.iv_link.iv_id.i = lv_cnt++;
    lv_qsigd_i2.iv_link.iv_id.i = lv_cnt++;
    lv_qlf_i1.iv_link.iv_id.i = lv_cnt++;
    lv_qlf_i2.iv_link.iv_id.i = lv_cnt++;

    printf("expecting empty Q (SB_Queue), this=%p\n", (void *) &lv_q);
    lv_q.printself(true);
    printf("\n");

    printf("expecting empty Q (SB_D_Queue), this=%p\n", (void *) &lv_qd);
    lv_qd.printself(true);
    printf("\n");

    printf("expecting empty Q (SB_Ts_Queue), this=%p\n", (void *) &lv_qts);
    lv_qts.printself(true);
    printf("\n");

    printf("expecting empty Q (SB_Ts_D_Queue), this=%p\n", (void *) &lv_qtsd);
    lv_qtsd.printself(true, true);
    printf("\n");

    printf("expecting empty Q (SB_Sig_Queue), this=%p\n", (void *) &lv_qsig);
    lv_qsig.printself(true);
    printf("\n");

    printf("expecting empty Q (SB_Sig_D_Queue), this=%p\n", (void *) &lv_qsigd);
    lv_qsigd.printself(true, true);
    printf("\n");

    printf("expecting empty Q (SB_Lf_Queue), this=%p\n", (void *) &lv_qlf);
    lv_qlf.printself(true);
    printf("\n");

    lv_q.add(&lv_q_i1.iv_link);
    lv_q.add(&lv_q_i2.iv_link);
    lv_size = lv_q.size();
    assert(lv_size == 2);
    printf("expecting (SB_Queue) 2 items, Item=%p, Item=%p\n",
           (void *) &lv_q_i1, (void *) &lv_q_i2);
    lv_q.printself(true);
    printf("\n");

    lv_qd.add(&lv_qd_i1.iv_link);
    lv_qd.add(&lv_qd_i2.iv_link);
    lv_size = lv_qd.size();
    assert(lv_size == 2);
    printf("expecting (SB_D_Queue) 2 items, Item=%p, Item=%p\n",
           (void *) &lv_qd_i1, (void *) &lv_qd_i2);
    lv_qd.printself(true);
    printf("\n");

    lv_qts.add(&lv_qts_i1.iv_link);
    lv_qts.add(&lv_qts_i2.iv_link);
    lv_size = lv_qts.size();
    assert(lv_size == 2);
    printf("expecting (SB_Ts_Queue) 2 items, Item=%p, Item=%p\n",
           (void *) &lv_qts_i1, (void *) &lv_qts_i2);
    lv_qts.printself(true);
    printf("\n");

    lv_qtsd.add(&lv_qtsd_i1.iv_link);
    lv_qtsd.add(&lv_qtsd_i2.iv_link);
    lv_size = lv_qtsd.size();
    assert(lv_size == 2);
    printf("expecting (SB_Ts_D_Queue) 2 items, Item=%p, Item=%p\n",
           (void *) &lv_qtsd_i1, (void *) &lv_qtsd_i2);
    lv_qtsd.printself(true, true);
    printf("\n");

    lv_qsig.add(&lv_qsig_i1.iv_link);
    lv_qsig.add(&lv_qsig_i2.iv_link);
    lv_size = lv_qsig.size();
    assert(lv_size == 2);
    printf("expecting (SB_Sig_Queue) 2 items, Item=%p, Item=%p\n",
           (void *) &lv_qsig_i1, (void *) &lv_qsig_i2);
    lv_qsig.printself(true);
    printf("\n");

    lv_qsigd.add(&lv_qsigd_i1.iv_link);
    lv_qsigd.add(&lv_qsigd_i2.iv_link);
    lv_size = lv_qsigd.size();
    assert(lv_size == 2);
    printf("expecting (SB_Sig_D_Queue) 2 items, Item=%p, Item=%p\n",
           (void *) &lv_qsigd_i1, (void *) &lv_qsigd_i2);
    lv_qsigd.printself(true, true);
    printf("\n");

    lv_qlf.add(&lv_qlf_i1.iv_link);
    lv_qlf.add(&lv_qlf_i2.iv_link);
    lv_size = lv_qlf.size();
    assert(lv_size == 2);
    printf("expecting (SB_Lf_Queue) 2 items, Item=%p, Item=%p\n",
           (void *) &lv_qlf_i1, (void *) &lv_qlf_i2);
    lv_qlf.printself(true);
    printf("\n");

    lv_qlf.remove();
    lv_size = lv_qlf.size();
    assert(lv_size == 1);
    printf("expecting 1 item, Item=%p\n", (void *) &lv_qlf_i2);
    lv_qlf.printself(true);
    printf("\n");
    lv_qlf.remove();
    lv_size = lv_qlf.size();
    assert(lv_size == 0);
    printf("expecting empty Q, this=%p\n", (void *) &lv_qlf);
    lv_qlf.printself(true);
    printf("\n");
}

void test_queue12() {
    enum { MAX_LIST = 10 };
    MS_Md_Type     la_mds[MAX_LIST];
    MS_Md_Type    *lp_md;
    int            lv_inx;
    SB_Ts_Md_Queue lv_q(1, "queue12-ts-md");
    int            lv_size;

    // empty
    lv_size = lv_q.size();
    assert(lv_size == 0);
    printf("expecting empty q, this=%p\n", (void *) &lv_q);
    lv_q.printself(true, true);
    printf("\n");

    for (lv_inx = 0; lv_inx < MAX_LIST; lv_inx++) {
        lp_md = &la_mds[lv_inx];
        lp_md->iv_link.iv_id.i = lv_inx + 1;
        lv_q.add(&lp_md->iv_link);
    }

    lv_size = lv_q.size();
    assert(lv_size == MAX_LIST);
    printf("expecting %d items in q, this=%p\n", MAX_LIST, (void *) &lv_q);
    lv_q.printself(true, true);
    printf("\n");

    for (lv_inx = 0; lv_inx < MAX_LIST; lv_inx++) {
        lp_md = (MS_Md_Type *) lv_q.remove();
        assert(lp_md == &la_mds[lv_inx]);
    }

    lv_size = lv_q.size();
    assert(lv_size == 0);
    printf("expecting empty q, this=%p\n", (void *) &lv_q);
    lv_q.printself(true, true);
    printf("\n");
}

void test_queue13() {
    enum { MAX_LIST = 10 };
    MS_Md_Type     la_mds[MAX_LIST];
    MS_Md_Type    *lp_md1;
    MS_Md_Type    *lp_md2;
    SB_Ts_Md_Queue lv_q(1, "queue13-ts-md");
    int            lv_size;

    lp_md1 = &la_mds[0];
    lp_md2 = &la_mds[1];
    lp_md1->iv_link.iv_id.i = 1;
    lp_md2->iv_link.iv_id.i = 2;
    lv_q.add(&lp_md1->iv_link);
    lv_q.add(&lp_md2->iv_link);
    printf("expecting 2 items\n");
    lv_q.printself(true, true);
    printf("\n");
    lv_size = lv_q.size();
    assert(lv_size == 2);
    assert(lp_md1 == (MS_Md_Type *) lv_q.head());

    assert(lv_q.remove_list(&lp_md1->iv_link));
    printf("expecting 1 items\n");
    lv_q.printself(true, true);
    printf("\n");

    assert(lp_md2 == (MS_Md_Type *) lv_q.head());
    assert(lv_q.remove_list(&lp_md2->iv_link));
    printf("expecting 0 items\n");
    lv_q.printself(true, true);
    printf("\n");

    lv_size = lv_q.size();
    assert(lv_size == 0);
    assert(lv_q.empty());

    lv_q.add(&lp_md2->iv_link);
    lv_q.add(&lp_md1->iv_link);
    printf("expecting 2 items\n");
    lv_q.printself(true, true);
    printf("\n");
    lv_size = lv_q.size();
    assert(lv_size == 2);

    assert(lv_q.remove_list(&lp_md1->iv_link));
    printf("expecting 1 items\n");
    lv_q.printself(true, true);
    printf("\n");

    assert(lv_q.remove_list(&lp_md2->iv_link));
    printf("expecting 0 items\n");
    lv_q.printself(true, true);
    printf("\n");
    lv_size = lv_q.size();
    assert(lv_size == 0);
    assert(lv_q.empty());
}

void test_queue14() {
    enum { MAX_LIST = 10 };
    MS_Md_Type     la_mds[MAX_LIST];
    MS_Md_Type    *lp_md;
    MS_Md_Type    *lp_md1;
    MS_Md_Type    *lp_md2;
    MS_Md_Type    *lp_md3;
    SB_Recv_Queue  lv_q("queue14-recv-md");
    int            lv_size;

    lp_md1 = &la_mds[0];
    lp_md2 = &la_mds[1];
    lp_md3 = &la_mds[3];
    lp_md1->iv_link.iv_id.i = 1;
    lp_md1->ip_stream = NULL;
    lp_md1->out.iv_mon_msg = false;
    lp_md2->iv_link.iv_id.i = 2;
    lp_md2->ip_stream = NULL;
    lp_md2->out.iv_mon_msg = false;
    lp_md3->iv_link.iv_id.i = 3;
    lp_md3->ip_stream = NULL;
    lp_md3->out.iv_mon_msg = false;
    lp_md1->out.iv_pri = 1;
    lp_md2->out.iv_pri = 3;
    lp_md3->out.iv_pri = 5;

    lv_q.set_priority_queue(true);
    lv_q.add(&lp_md1->iv_link);
    lv_q.add(&lp_md2->iv_link);
    lv_q.add(&lp_md3->iv_link);
    printf("expecting 3 items 3-2-1\n");
    lv_q.printself(true, true);
    printf("\n");
    lv_size = lv_q.size();
    assert(lv_size == 3);

    lp_md = (MS_Md_Type *) lv_q.remove();
    assert(lp_md == lp_md3);
    lp_md = (MS_Md_Type *) lv_q.remove();
    assert(lp_md == lp_md2);
    lp_md = (MS_Md_Type *) lv_q.remove();
    assert(lp_md == lp_md1);
    assert(lv_q.empty());
    lv_size = lv_q.size();
    assert(lv_size == 0);
    printf("expecting 0 items\n");
    lv_q.printself(true, true);
    printf("\n");

    lp_md1->out.iv_pri = 30;
    lp_md2->out.iv_pri = 20;
    lp_md3->out.iv_pri = 10;
    lv_q.add(&lp_md1->iv_link);
    printf("expecting 1 items 1\n");
    lv_q.printself(true, true);
    printf("\n");

    lv_q.add(&lp_md2->iv_link);
    printf("expecting 2 items 1-2\n");
    lv_q.printself(true, true);
    printf("\n");

    lv_q.add(&lp_md3->iv_link);
    printf("expecting 3 items 1-2-3\n");
    lv_q.printself(true, true);
    printf("\n");
    lv_size = lv_q.size();
    assert(lv_size == 3);

    lp_md = (MS_Md_Type *) lv_q.remove();
    assert(lp_md == lp_md1);
    lp_md = (MS_Md_Type *) lv_q.remove();
    assert(lp_md == lp_md2);
    lp_md = (MS_Md_Type *) lv_q.remove();
    assert(lp_md == lp_md3);
    assert(lv_q.empty());
    printf("expecting 0 items\n");
    lv_q.printself(true, true);
    printf("\n");
    lv_size = lv_q.size();
    assert(lv_size == 0);
}

void test_queue15() {
    typedef struct X_Type {
        int         iv_junk[5];
        SB_DQL_Type iv_link;
    } X_Type;
    X_Type     lv_item1;
    X_Type     lv_item2;
    X_Type     lv_item3;
    X_Type     lv_item4;
    bool       lv_on_list;
    SB_D_Queue lv_q(1, "queue15-d");

    memset(&lv_item1, -1, sizeof(lv_item1));
    memset(&lv_item2, -1, sizeof(lv_item2));
    memset(&lv_item3, -1, sizeof(lv_item3));
    memset(&lv_item4, -1, sizeof(lv_item4));
    lv_item1.iv_link.iv_id.ll = 1;;
    lv_item2.iv_link.iv_id.ll = 2;;
    lv_item3.iv_link.iv_id.ll = 3;;
    lv_item4.iv_link.iv_id.ll = 4;;
    lv_q.add(&lv_item1.iv_link);
    lv_q.add(&lv_item2.iv_link);
    lv_q.add(&lv_item3.iv_link);
    lv_on_list = lv_q.remove_list(&lv_item3.iv_link);
    assert(lv_on_list);
    lv_q.add_at_front(&lv_item3.iv_link);
    lv_on_list = lv_q.remove_list(&lv_item1.iv_link);
    assert(lv_on_list);
    lv_q.add_at_front(&lv_item1.iv_link);
    lv_on_list = lv_q.remove_list(&lv_item2.iv_link);
    assert(lv_on_list);
    lv_q.add_at_front(&lv_item2.iv_link);
    lv_on_list = lv_q.remove_list(&lv_item1.iv_link);
    assert(lv_on_list);
    lv_on_list = lv_q.remove_list(&lv_item2.iv_link);
    assert(lv_on_list);
    lv_on_list = lv_q.remove_list(&lv_item3.iv_link);
    assert(lv_on_list);
    lv_q.add_at_front(&lv_item4.iv_link);
    lv_on_list = lv_q.remove_list(&lv_item1.iv_link);
    assert(!lv_on_list);
    lv_on_list = lv_q.remove_list(&lv_item2.iv_link);
    assert(!lv_on_list);
    lv_on_list = lv_q.remove_list(&lv_item3.iv_link);
    assert(!lv_on_list);
    lv_on_list = lv_q.remove_list(&lv_item4.iv_link);
    assert(lv_on_list);
    lv_on_list = lv_q.remove_list(&lv_item1.iv_link);
    assert(!lv_on_list);
    lv_on_list = lv_q.remove_list(&lv_item2.iv_link);
    assert(!lv_on_list);
    lv_on_list = lv_q.remove_list(&lv_item3.iv_link);
    assert(!lv_on_list);
    lv_on_list = lv_q.remove_list(&lv_item4.iv_link);
    assert(!lv_on_list);
}

void test_rw_lock() {
    SB_Thread::RWL lv_rwl;

    assert(lv_rwl.readlock() == 0);
    assert(lv_rwl.unlock() == 0);

    assert(lv_rwl.writelock() == 0);
    assert(lv_rwl.unlock() == 0);

    assert(lv_rwl.tryreadlock() == 0);
    assert(lv_rwl.tryreadlock() == 0);
    assert(lv_rwl.unlock() == 0);
    assert(lv_rwl.unlock() == 0);

    assert(lv_rwl.trywritelock() == 0);
    assert(lv_rwl.trywritelock());
    assert(lv_rwl.unlock() == 0);

    assert(lv_rwl.writelock() == 0);
    assert(lv_rwl.tryreadlock());
    assert(lv_rwl.unlock() == 0);
}

void test_scoped_mutex1(SB_Thread::Mutex *pp_m) {
    SB_Thread::Scoped_Mutex lv_m(*pp_m);
}

void test_scoped_mutex2(SB_Thread::Mutex *pp_m) {
    SB_Thread::Scoped_Mutex lv_m(*pp_m);
    lv_m.lock();
}

void test_scoped_mutex3(SB_Thread::Mutex *pp_m) {
    SB_Thread::Scoped_Mutex lv_m(*pp_m);
    lv_m.lock();
    lv_m.unlock();
}

void test_scoped_mutexes() {
    SB_Thread::Mutex lv_m;
    test_scoped_mutex1(&lv_m);
    test_scoped_mutex2(&lv_m);
    test_scoped_mutex3(&lv_m);
}

void test_thread1() {
    printf("launching r1\n");
    SB_Sig_Queue lv_q("thread1-sig", false);
    SB_Thread::Sthr::Id_Ptr lp_thr_r = SB_Thread::Sthr::create((char *) "reader", t_r1, &lv_q);
    assert(lp_thr_r != NULL);

    SB_Thread::Sthr::sleep(10); // just to try this method

    printf("launching w1\n");
    SB_Thread::Sthr::Id_Ptr lp_thr_w = SB_Thread::Sthr::create((char *) "writer", t_w1, &lv_q);
    assert(lp_thr_w != NULL);

    void *lp_value;
    int lv_status = SB_Thread::Sthr::join(lp_thr_w, &lp_value);
    assert(lv_status == 0);
    printf("joined with w1\n");

    lv_status = SB_Thread::Sthr::join(lp_thr_r, &lp_value);
    assert(lv_status == 0);
    printf("joined with r1\n");

    // cleanup
    SB_Thread::Sthr::delete_id(lp_thr_r);
    SB_Thread::Sthr::delete_id(lp_thr_w);
}

void test_thread2() {
    printf("launching r2\n");
    SB_Sig_Queue lv_q("thread2-sig", false);
    Rthr *lp_thr_r = new Rthr(t_r2, "reader", &lv_q);
    lp_thr_r->start();

    printf("launching w2\n");
    Wthr *lp_thr_w = new Wthr(t_w2, "writer", &lv_q);
    lp_thr_w->start();

    void *lp_value;
    int lv_status = lp_thr_w->join(&lp_value);
    assert(lv_status == 0);
    printf("joined with w2\n");

    lv_status = lp_thr_r->join(&lp_value);
    assert(lv_status == 0);
    printf("joined with r2\n");

    // cleanup
    delete lp_thr_r;
    delete lp_thr_w;
}

void test_thread3() {
    printf("launching r3\n");
    SB_Thread::Sthr::Id_Ptr lp_thr_r = SB_Thread::Sthr::create((char *) "reader", t_r3, (void *) 1);

    printf("launching w3\n");
    SB_Thread::Sthr::Id_Ptr lp_thr_w = SB_Thread::Sthr::create((char *) "writer", t_w3, (void *) 2);

    void *lp_value;
    int lv_status = SB_Thread::Sthr::join(lp_thr_w, &lp_value);
    assert(lv_status == 0);
    printf("joined with w3\n");

    lv_status = SB_Thread::Sthr::join(lp_thr_r, &lp_value);
    assert(lv_status == 0);
    printf("joined with r3\n");

    // cleanup
    SB_Thread::Sthr::delete_id(lp_thr_r);
    SB_Thread::Sthr::delete_id(lp_thr_w);
}

void test_thread4() {
    msg_test_disable_wait(true);
    test_msg_init();
    printf("launching r4\n");
    SB_Thread::Sthr::Id_Ptr lp_thr_r = SB_Thread::Sthr::create((char *) "reader", t_r4, (void *) 1);

    printf("launching w4\n");
    SB_Thread::Sthr::Id_Ptr lp_thr_w = SB_Thread::Sthr::create((char *) "writer", t_w4, (void *) 2);

    void *lp_value;
    int lv_status = SB_Thread::Sthr::join(lp_thr_w, &lp_value);
    assert(lv_status == 0);
    printf("joined with w4\n");

    lv_status = SB_Thread::Sthr::join(lp_thr_r, &lp_value);
    assert(lv_status == 0);
    printf("joined with r4\n");
    msg_test_disable_wait(false);

    // cleanup
    SB_Thread::Sthr::delete_id(lp_thr_r);
    SB_Thread::Sthr::delete_id(lp_thr_w);
}

void test_thread5() {
    test_msg_init();
    msg_test_disable_wait(true);
    printf("launching r51\n");
    SB_Thread::Sthr::Id_Ptr lp_thr_r1 = SB_Thread::Sthr::create((char *) "reader1", t_r51, (void *) 1);

    printf("launching r52\n");
    SB_Thread::Sthr::Id_Ptr lp_thr_r2 = SB_Thread::Sthr::create((char *) "reader2", t_r52, (void *) 2);

    printf("launching w5\n");
    SB_Thread::Sthr::Id_Ptr lp_thr_w = SB_Thread::Sthr::create((char *) "writer", t_w5, (void *) 3);

    void *lp_value;
    int lv_status = SB_Thread::Sthr::join(lp_thr_w, &lp_value);
    assert(lv_status == 0);
    printf("joined with w5\n");

    lv_status = SB_Thread::Sthr::join(lp_thr_r1, &lp_value);
    assert(lv_status == 0);
    printf("joined with r51\n");
    lv_status = SB_Thread::Sthr::join(lp_thr_r2, &lp_value);
    assert(lv_status == 0);
    printf("joined with r52\n");
    msg_test_disable_wait(false);

    // cleanup
    SB_Thread::Sthr::delete_id(lp_thr_r1);
    SB_Thread::Sthr::delete_id(lp_thr_r2);
    SB_Thread::Sthr::delete_id(lp_thr_w);
}

void test_tls_dtor(void *) {
}

void test_tls() {
    enum { MAX_KEYS = 1024 };
    int    la_keys[MAX_KEYS];
    void  *lp_data;
    void  *lp_data_exp;
    int    lv_inx;
    int    lv_max;
    int    lv_status;

    lv_max = MAX_KEYS;
    for (lv_inx = 0; lv_inx < MAX_KEYS; lv_inx++) {
        lv_status =
          SB_Thread::Sthr::specific_key_create(la_keys[lv_inx],
                                               test_tls_dtor);
        if (lv_status == EAGAIN) {
            lv_max = lv_inx - 1;
            break;
        }
        assert(lv_status == 0);
    }
    for (lv_inx = 0; lv_inx < lv_max; lv_inx++) {
        lp_data = (void *) (long) (lv_inx + 1);
        lv_status = SB_Thread::Sthr::specific_set(la_keys[lv_inx], lp_data);
        assert(lv_status == 0);
    }
    for (lv_inx = 0; lv_inx < lv_max; lv_inx++) {
        lp_data_exp = (void *) (long) (lv_inx + 1);
        lp_data = SB_Thread::Sthr::specific_get(la_keys[lv_inx]);
        assert(lp_data == lp_data_exp);
    }
}

void test_table_ex(Table_Mgr *pp_table) {
    enum {           MAX_ENTRIES = 20 };
    //char           la_info[100];
    Table_Entry     *lp_entry;
    size_t           lv_cap;
    size_t           lv_cinx;
    size_t           lv_expinuse;
    int              lv_inx;
    size_t           lv_inuse;

    // pp_table->print(la_info);
    for (lv_inx = 0; lv_inx < MAX_ENTRIES; lv_inx++) {
        pp_table->alloc_entry();
        // pp_table->print(la_info);
        lv_inuse = pp_table->get_inuse();
        lv_expinuse = lv_inx + 1;
        assert(lv_inuse == lv_expinuse);
        lv_inuse = 0;
        lv_cap = pp_table->get_cap();
        for (lv_cinx = 0; lv_cinx < lv_cap; lv_cinx++) {
            lp_entry = pp_table->get_entry(lv_cinx);
            if ((lp_entry != NULL) && lp_entry->iv_inuse)
                lv_inuse++;
        }
        assert(lv_inuse == lv_expinuse);
    }
    lv_expinuse = MAX_ENTRIES;
    for (lv_inx = 0; lv_inx < MAX_ENTRIES; lv_inx++) {
        pp_table->free_entry(lv_inx);
        lv_expinuse--;
        // pp_table->print(la_info);
        lv_inuse = pp_table->get_inuse();
        assert(lv_inuse == lv_expinuse);
        lv_inuse = 0;
        lv_cap = pp_table->get_cap();
        for (lv_cinx = 0; lv_cinx < lv_cap; lv_cinx++) {
            lp_entry = pp_table->get_entry(lv_cinx);
            if ((lp_entry != NULL) && lp_entry->iv_inuse)
                lv_inuse++;
        }
        assert(lv_inuse == lv_expinuse);
    }
}

void test_table() {
    Table_Entry_Mgr  lv_entry_mgr;
    Table_Mgr        lv_blk_table_fast("blk-fast",
                                       SB_Table_Mgr_Alloc::ALLOC_FAST,
                                       SB_Table_Mgr_Alloc::ALLOC_ENTRY_BLOCK,
                                       &lv_entry_mgr, 5, 5);
    Table_Mgr        lv_blk_table_fifo("blk-fifo",
                                       SB_Table_Mgr_Alloc::ALLOC_FIFO,
                                       SB_Table_Mgr_Alloc::ALLOC_ENTRY_BLOCK,
                                       &lv_entry_mgr, 5, 5);
    Table_Mgr        lv_blk_table_min("blk-min",
                                      SB_Table_Mgr_Alloc::ALLOC_FAST,
                                      SB_Table_Mgr_Alloc::ALLOC_ENTRY_BLOCK,
                                      &lv_entry_mgr, 5, 5);
    Table_Mgr        lv_blk_table_scan("blk-scan",
                                       SB_Table_Mgr_Alloc::ALLOC_SCAN,
                                       SB_Table_Mgr_Alloc::ALLOC_ENTRY_BLOCK,
                                       &lv_entry_mgr, 5, 5);
    Ts_Table_Mgr     lv_blk_ts_table_fast("ts-blk-fast",
                                          SB_Table_Mgr_Alloc::ALLOC_FAST,
                                          SB_Table_Mgr_Alloc::ALLOC_ENTRY_BLOCK,
                                          &lv_entry_mgr, 5, 5);
    Ts_Table_Mgr     lv_blk_ts_table_fifo("ts-blk-fifo",
                                          SB_Table_Mgr_Alloc::ALLOC_FIFO,
                                          SB_Table_Mgr_Alloc::ALLOC_ENTRY_BLOCK,
                                          &lv_entry_mgr, 5, 5);
    Ts_Table_Mgr     lv_blk_ts_table_min("ts-blk-min",
                                         SB_Table_Mgr_Alloc::ALLOC_FAST,
                                         SB_Table_Mgr_Alloc::ALLOC_ENTRY_BLOCK,
                                         &lv_entry_mgr, 5, 5);
    Ts_Table_Mgr     lv_blk_ts_table_scan("ts-blk-scan",
                                          SB_Table_Mgr_Alloc::ALLOC_SCAN,
                                          SB_Table_Mgr_Alloc::ALLOC_ENTRY_BLOCK,
                                          &lv_entry_mgr, 5, 5);
    Table_Mgr        lv_dyn_table_fast("dyn-fast",
                                       SB_Table_Mgr_Alloc::ALLOC_FAST,
                                       SB_Table_Mgr_Alloc::ALLOC_ENTRY_DYN,
                                       &lv_entry_mgr, 5, 5);
    Table_Mgr        lv_dyn_table_fifo("dyn-fifo",
                                       SB_Table_Mgr_Alloc::ALLOC_FIFO,
                                       SB_Table_Mgr_Alloc::ALLOC_ENTRY_DYN,
                                       &lv_entry_mgr, 5, 5);
    Table_Mgr        lv_dyn_table_min("dyn-min",
                                      SB_Table_Mgr_Alloc::ALLOC_FAST,
                                      SB_Table_Mgr_Alloc::ALLOC_ENTRY_DYN,
                                      &lv_entry_mgr, 5, 5);
    Table_Mgr        lv_dyn_table_scan("dyn-scan",
                                       SB_Table_Mgr_Alloc::ALLOC_SCAN,
                                       SB_Table_Mgr_Alloc::ALLOC_ENTRY_DYN,
                                       &lv_entry_mgr, 5, 5);

    test_table_ex(&lv_blk_table_fast);
    test_table_ex(&lv_blk_table_fifo);
    test_table_ex(&lv_blk_table_min);
    test_table_ex(&lv_blk_table_scan);
    test_table_ex(&lv_dyn_table_fast);
    test_table_ex(&lv_dyn_table_fifo);
    test_table_ex(&lv_dyn_table_min);
    test_table_ex(&lv_dyn_table_scan);
    test_table_ex(&lv_blk_ts_table_fast);
    test_table_ex(&lv_blk_ts_table_fifo);
    test_table_ex(&lv_blk_ts_table_min);
    test_table_ex(&lv_blk_ts_table_scan);
}

void test_table2_rnd(Table_Mgr *pp_table) {
    enum           { MAX_ITERS = 1000 };
    enum           { MAX_SLOTS = 100 };
    int              la_slots[MAX_SLOTS];
    size_t           lv_inuse;
    size_t           lv_inx;
    size_t           lv_iter;
    size_t           lv_max;
    int              lv_max_slot;
    size_t           lv_rnd;
    size_t           lv_slot;

    srand(1);
    for (lv_iter = 0; lv_iter < MAX_ITERS; lv_iter++) {
        for (lv_inx = 0; lv_inx < MAX_SLOTS; lv_inx++)
            la_slots[lv_inx] = -1;
        // allocate some random number of entries
        lv_max = 1 + (int) ((float) (MAX_SLOTS-1) * (rand_r(&gv_seed) / (RAND_MAX + 1.0)));
        for (lv_inx = 0; lv_inx < lv_max; lv_inx++) {
            lv_slot = pp_table->alloc_entry();
            la_slots[lv_slot] = (int) lv_slot;
        }
        assert(pp_table->get_inuse() == lv_max);
        lv_inuse = lv_max;
        lv_max_slot = (int) lv_max;
        // delete entries in random order
        for (lv_inx = 0; lv_inx < lv_max; lv_inx++) {
            // get a random slot
            for (;;) {
                // find max-slot
                for (lv_max_slot = MAX_SLOTS - 1;
                     lv_max_slot >= 0;
                    lv_max_slot--) {
                    if (la_slots[lv_max_slot] >= 0)
                        break;
                }
                lv_max_slot++;
                lv_rnd = (int) ((float) lv_max_slot * (rand_r(&gv_seed) / (RAND_MAX + 1.0)));
                if (la_slots[lv_rnd] >= 0)
                    break;
            }
            la_slots[lv_rnd] = -1;
            // free it
            pp_table->free_entry(lv_rnd);
            lv_inuse--;
            assert(pp_table->get_inuse() == lv_inuse);
        }
        assert(pp_table->get_inuse() == 0);
    }
}

void test_table2() {
    Table_Entry_Mgr  lv_entry_mgr;
    Table_Mgr        lv_blk_table_fast("blk-fast",
                                       SB_Table_Mgr_Alloc::ALLOC_FAST,
                                       SB_Table_Mgr_Alloc::ALLOC_ENTRY_BLOCK,
                                       &lv_entry_mgr, 5, 5);
    Table_Mgr        lv_blk_table_fifo("blk-fifo",
                                       SB_Table_Mgr_Alloc::ALLOC_FIFO,
                                       SB_Table_Mgr_Alloc::ALLOC_ENTRY_BLOCK,
                                       &lv_entry_mgr, 5, 5);
    Table_Mgr        lv_blk_table_min("blk-min",
                                       SB_Table_Mgr_Alloc::ALLOC_MIN,
                                       SB_Table_Mgr_Alloc::ALLOC_ENTRY_BLOCK,
                                       &lv_entry_mgr, 5, 5);
    Table_Mgr        lv_blk_table_scan("blk-scan",
                                       SB_Table_Mgr_Alloc::ALLOC_SCAN,
                                       SB_Table_Mgr_Alloc::ALLOC_ENTRY_BLOCK,
                                       &lv_entry_mgr, 5, 5);
    Table_Mgr        lv_dyn_table_fast("dyn-fast",
                                       SB_Table_Mgr_Alloc::ALLOC_FAST,
                                       SB_Table_Mgr_Alloc::ALLOC_ENTRY_DYN,
                                       &lv_entry_mgr, 5, 5);
    Table_Mgr        lv_dyn_table_fifo("dyn-fifo",
                                       SB_Table_Mgr_Alloc::ALLOC_FIFO,
                                       SB_Table_Mgr_Alloc::ALLOC_ENTRY_DYN,
                                       &lv_entry_mgr, 5, 5);
    Table_Mgr        lv_dyn_table_min("dyn-min",
                                       SB_Table_Mgr_Alloc::ALLOC_MIN,
                                       SB_Table_Mgr_Alloc::ALLOC_ENTRY_DYN,
                                       &lv_entry_mgr, 5, 5);
    Table_Mgr        lv_dyn_table_scan("dyn-scan",
                                       SB_Table_Mgr_Alloc::ALLOC_SCAN,
                                       SB_Table_Mgr_Alloc::ALLOC_ENTRY_DYN,
                                       &lv_entry_mgr, 5, 5);

    test_table2_rnd(&lv_blk_table_fast);
    test_table2_rnd(&lv_blk_table_fifo);
    test_table2_rnd(&lv_blk_table_min);
    test_table2_rnd(&lv_blk_table_scan);
    test_table2_rnd(&lv_dyn_table_fast);
    test_table2_rnd(&lv_dyn_table_fifo);
    test_table2_rnd(&lv_dyn_table_min);
    test_table2_rnd(&lv_dyn_table_scan);
}

int main(int pv_argc, char *pa_argv[]) {
    bool           lv_debug = false;
    int            lv_err;
    bool           lv_hook = false;
    struct utsname lv_uname;
    TAD            la_zargs[] = {
      { "-cluster",   TA_Ign,  TA_NOMAX,    NULL       },
      { "-debug",     TA_Bool, TA_NOMAX,    &lv_debug  },
      { "-hook",      TA_Bool, TA_NOMAX,    &lv_hook   },
      { "-verbose",   TA_Ign,  TA_NOMAX,    NULL       },
      { "-vg",        TA_Bool, TA_NOMAX,    &gv_vg     },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(la_zargs, false, pv_argc, pa_argv);
    if (lv_debug)
        XDEBUG();
    if (lv_hook)
        test_debug_hook("c", "c");
    lv_err = uname(&lv_uname);
    assert(lv_err == 0);
    if (memcmp(lv_uname.release, "2.6.18-", 7) == 0)
        gv_rh5 = true;
    else
        gv_rh5 = false;
    msg_init_env(pv_argc, pa_argv);
    msg_init_trace();
    test_assert();
    test_mutexes();
    test_scoped_mutexes();
    test_rw_lock();
    test_queue();
    test_queue2();
    test_queue3();
    test_queue4();
    test_queue5();
    test_queue6();
    test_queue7();
    test_queue8();
    test_queue9();
    test_queue10();
    test_queue12();
    test_queue13();
    test_queue14();
    test_queue15();
    test_map();
    test_map_enum();
    test_map_enum2();
    test_map_enum3();
    test_map_enum4();
    test_map_enum5();
    test_map_enum6();
    test_map_resize();
    test_lmap();
    test_lmap_enum();
    test_lmap_resize();
    test_llmap();
    test_llmap_enum();
    test_llmap_resize();
    test_smap();
    test_smap_enum();
    test_smap_resize();
    test_tmap();
    test_tmap_enum();
    test_tmap_resize();
    test_timer_map();
    test_timer_map_enum();
    test_props();
    test_slotmgr();
    test_slotmgr2();
    test_slotmgr3();
    test_slotmgr4();
    test_slotmgr5();
    test_slotmgr_ts();
    test_table();
    test_table2();
    test_excep();
    test_thread1();
    test_thread2();
    test_thread3();
    test_thread4();
    test_thread5();
#if 0
    test_tls();
#endif
    return 0;
}
