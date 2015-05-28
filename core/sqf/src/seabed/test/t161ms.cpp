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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/unistd.h>

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"
#include "seabed/timer.h"
#include "seabed/thread.h"

#include "tchkfe.h"
#include "tchkos.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

#define gettid() (pid_t) syscall(__NR_gettid)

enum {
    MAX_THR  = 50
};

class Thread_Test : public SB_Thread::Thread {
public:
    Thread_Test(Function fun, const char *name, int inx)
    : SB_Thread::Thread(fun, name), whoami(inx) {
    }
    void work();

private:
    int whoami;
};

class Thread_Test2 : public SB_Thread::Thread {
public:
    Thread_Test2(Function fun, const char *name, int inx)
    : SB_Thread::Thread(fun, name), whoami(inx) {
    }
    void work();

private:
    int whoami;
};

class Thread_Test3 : public SB_Thread::Thread {
public:
    Thread_Test3(Function fun, const char *name, int inx)
    : SB_Thread::Thread(fun, name), whoami(inx) {
    }
    void work();

private:
    int whoami;
};


SB_Thread::CV  cv1;
SB_Thread::CV  cv2;
int            maxth = 1;
char           my_name[BUFSIZ];
MS_SRE_TPOP    t1;
MS_SRE_TPOP    t2;
Thread_Test   *thr[MAX_THR];
Thread_Test2  *thr2[2];
Thread_Test3  *thr3[MAX_THR];
short          tleid_test2;
short          tleid_test3;
int            tles = 0;
SB_Thread::SL  tles_sl;
int            to_tol = -1;
bool           verbose = false;


void cbx(MS_SRE_TPOP   *t,
         SB_Thread::CV *cv,
         int            tleid,
         int            toval,
         short          parm1,
         long           parm2) {
    t->sre_tleId = tleid;
    t->sre_tleTOVal = toval;
    t->sre_tleParm1 = parm1;
    t->sre_tleParm2 = parm2;
    cv->signal(true);
}

void cb1(int tleid, int toval, short parm1, long parm2) {
    if (verbose)
        printf("cv1 signal\n");
    cbx(&t1, &cv1, tleid, toval, parm1, parm2);
}

void cb2(int tleid, int toval, short parm1, long parm2) {
    if (verbose)
        printf("cv2 signal\n");
    cbx(&t2, &cv2, tleid, toval, parm1, parm2);
}

void cbcan(int tleid, int toval, short parm1, long parm2) {
    if (verbose)
        printf("cbcan called tleid=%d, toval=%d, p1=%d, p2=%ld\n",
               tleid, toval, parm1, parm2);
    assert(0); // should not be called
}


void Thread_Test::work() {
    _xcc_status     cc;
    int             lerr;
    MS_SRE_TPOP     sre;
    int             status;
    struct timeval  t_elapsed;
    struct timeval  t_start;
    struct timeval  t_stop;
    int             tid;
    short           tleid1;
    short           tleid2;
    short           tleid3;
    short           tleid4;
    long            to_act;
    long            to_exp;

    tid = gettid();
    // drain events
    lerr = XWAIT(-1, -2);
    TEST_CHK_WAITIGNORE(lerr);
    lerr = XWAIT(-1, -2);
    TEST_CHK_WAITIGNORE(lerr);
    util_time_timer_start(&t_start);
    cc = XSIGNALTIMEOUT(40, 4, tid, &tleid1);
    TEST_CHK_CCEQ(cc);
    cc = XSIGNALTIMEOUT(80, 8, tid, &tleid2);
    TEST_CHK_CCEQ(cc);
    cc = XSIGNALTIMEOUT(20, 2, tid, &tleid3);
    TEST_CHK_CCEQ(cc);
    cc = XSIGNALTIMEOUT(160, 16, tid, &tleid4);
    TEST_CHK_CCEQ(cc);
    if (verbose)
        printf("w=%d\n", whoami);
    while (tles < 4 * maxth) {
        lerr = XWAIT(-1, 10);
        do {
            lerr = XMSG_LISTEN_((short *) &sre, // sre
                                0,              // listenopts
                                0);             // listenertag
            switch (lerr) {
            case XSRETYPE_TPOP:
                util_time_timer_stop(&t_stop);
                util_time_elapsed(&t_start, &t_stop, &t_elapsed);
                to_act = t_elapsed.tv_sec * 1000000 + t_elapsed.tv_usec;
                to_exp = sre.sre_tleTOVal * 10000;
                if (verbose || (to_act < to_exp))
                    printf("w=%d, tid=%d, tleid=%d, exp-to=%ld, act-to=%ld\n",
                           whoami, tid, sre.sre_tleId, to_exp, to_act);
                assert(to_act >= to_exp);
                if (to_tol > 0)
                    assert(to_act <= (to_exp + to_tol));
                status = tles_sl.lock();
                TEST_CHK_STATUSOK(status);
                tles++;
                status = tles_sl.unlock();
                TEST_CHK_STATUSOK(status);
                if (verbose)
                    printf("w=%d, TLEs=%d\n", whoami, tles);
                break;
            case XSRETYPE_NOWORK:
                break;
            default:
                assert((lerr == XSRETYPE_TPOP) || (lerr == XSRETYPE_NOWORK));
            }
        } while (lerr != XSRETYPE_NOWORK);
    }
}

void Thread_Test2::work() {
    _xcc_status     cc;
    int             lerr;
    MS_SRE_TPOP     sre;

    if (whoami == 0) {
        while (tleid_test2 < 0)
            SB_Thread::Sthr::yield();
        usleep(100000);
        cc = XCANCELTIMEOUT(tleid_test2);
        TEST_CHK_CCNE(cc);
    } else {
        lerr = XWAIT(LREQ, -1);
        assert(lerr == LREQ);
        cc = XSIGNALTIMEOUT(1, 1, 1, &tleid_test2);
        TEST_CHK_CCEQ(cc);
        lerr = XWAIT(LREQ, -1);
        assert(lerr == LREQ);
        lerr = XMSG_LISTEN_((short *) &sre, // sre
                            0,              // listenopts
                            0);             // listenertag
    }
}

void Thread_Test3::work() {
    _xcc_status  cc;
    int          inx;
    int          lerr;
    MS_SRE_TPOP  sre;
    short        tleid;

    for (inx = 0; inx < 1000; inx++) {
        cc = XSIGNALTIMEOUT(1, 1, 1, &tleid);
        TEST_CHK_CCEQ(cc);
        do {
            lerr = XWAIT(LREQ, -1);
            assert(lerr == LREQ);
            lerr = XMSG_LISTEN_((short *) &sre, // sre
                                0,              // listenopts
                                0);             // listenertag
        } while (lerr == XSRETYPE_NOWORK);
        assert(lerr == XSRETYPE_TPOP);
    }
}

void *thr_test(void *arg) {
    Thread_Test *lthr = (Thread_Test *) arg; // cast
    lthr->work();
    return NULL;
}

void *thr_test2(void *arg) {
    Thread_Test2 *lthr = (Thread_Test2 *) arg; // cast
    lthr->work();
    return NULL;
}

void *thr_test3(void *arg) {
    Thread_Test3 *lthr = (Thread_Test3 *) arg; // cast
    lthr->work();
    return NULL;
}

void test_cancel() {
    _xcc_status     cc;
    short           tleid1;
    short           tleid2;

    if (verbose)
        printf("cancel-test\n");
    cc = XSIGNALTIMEOUT(4, 4, 4, &tleid1);
    TEST_CHK_CCEQ(cc);
    cc = XCANCELTIMEOUT(tleid1);
    TEST_CHK_CCEQ(cc);
    cc = XCANCELTIMEOUT(tleid1);
    assert(_xstatus_ge(cc));
    cc = XSIGNALTIMEOUT(1, 1, 1, &tleid1);
    TEST_CHK_CCEQ(cc);
    usleep(4000);
    cc = XCANCELTIMEOUT(tleid1);
    TEST_CHK_CCEQ(cc);
    cc = XSIGNALTIMEOUT(1, 1, 1, &tleid1);
    TEST_CHK_CCEQ(cc);
    cc = XSIGNALTIMEOUT(1, 1, 1, &tleid2);
    TEST_CHK_CCEQ(cc);
    usleep(4000);
    cc = XCANCELTIMEOUT(tleid2);
    TEST_CHK_CCEQ(cc);
    cc = XCANCELTIMEOUT(tleid1);
    TEST_CHK_CCEQ(cc);
    cc = XSIGNALTIMEOUT(200, 1, 1, &tleid1);
    TEST_CHK_CCEQ(cc);
    cc = XSIGNALTIMEOUT(100, 1, 1, &tleid2);
    TEST_CHK_CCEQ(cc);
    cc = XCANCELTIMEOUT(tleid1);
    TEST_CHK_CCEQ(cc);
    cc = XCANCELTIMEOUT(tleid2);
    TEST_CHK_CCEQ(cc);
}

void test_cancel_cb() {
    int   disable;
    int   ferr;
    short tleid1;
    short tleid2;

    if (verbose)
        printf("cancel-cb-test\n");
    ferr = timer_start_cb(4, 4, 4, &tleid1, &cbcan);
    assert(ferr == XZFIL_ERR_OK);
    ferr = timer_cancel(tleid1);
    assert(ferr == XZFIL_ERR_OK);
    disable = msg_test_assert_disable();
    ferr = timer_cancel(tleid1);
    assert(ferr == XZFIL_ERR_NOTFOUND);
    msg_test_assert_enable(disable);
    ferr = timer_start_cb(1, 1, 1, &tleid1, &cbcan);
    assert(ferr == XZFIL_ERR_OK);
    usleep(4000);
    ferr = timer_cancel(tleid1);
    assert(ferr == XZFIL_ERR_OK);
    ferr = timer_start_cb(1, 1, 1, &tleid1, &cbcan);
    assert(ferr == XZFIL_ERR_OK);
    ferr = timer_start_cb(1, 1, 1, &tleid2, &cbcan);
    assert(ferr == XZFIL_ERR_OK);
    usleep(4000);
    ferr = timer_cancel(tleid2);
    assert(ferr == XZFIL_ERR_OK);
    ferr = timer_cancel(tleid1);
    assert(ferr == XZFIL_ERR_OK);
    ferr = timer_start_cb(214800, 4, 4, &tleid1, &cbcan); // test CR 6539
    assert(ferr == XZFIL_ERR_OK);
    ferr = timer_cancel(tleid1);
    assert(ferr == XZFIL_ERR_OK);
}

void test_timers() {
    _xcc_status     cc;
    int             lerr;
    MS_SRE_TPOP     sre;
    struct timeval  t_elapsed;
    struct timeval  t_start1;
    struct timeval  t_start2;
    struct timeval  t_stop;
    short           tleid1;
    short           tleid2;
    long            to_act;
    long            to_exp;

    if (verbose)
        printf("timer-test\n");

    // drain pending LREQ
    lerr = XWAIT(LREQ, -1);
    assert(lerr == LREQ);

    // check timers
    util_time_timer_start(&t_start1);
    cc = XSIGNALTIMEOUT(40, 2, 2, &tleid1);
    TEST_CHK_CCEQ(cc);
    usleep(30000);
    util_time_timer_start(&t_start2);
    cc = XSIGNALTIMEOUT(40, 4, 4, &tleid2);
    TEST_CHK_CCEQ(cc);

    lerr = XWAIT(LREQ, -1);
    assert(lerr == LREQ);
    lerr = XMSG_LISTEN_((short *) &sre, // sre
                        0,              // listenopts
                        0);             // listenertag
    assert(lerr == XSRETYPE_TPOP);
    util_time_timer_stop(&t_stop);
    util_time_elapsed(&t_start1, &t_stop, &t_elapsed);
    to_act = t_elapsed.tv_sec * 1000000 + t_elapsed.tv_usec;
    to_exp = sre.sre_tleTOVal * 10000;
    if (verbose || (to_act < to_exp))
        printf("tleid=%d, exp-to=%ld, act-to=%ld\n",
               sre.sre_tleId, to_exp, to_act);
    assert(to_act >= to_exp);
    if (to_tol > 0)
        assert(to_act <= (to_exp + to_tol));

    lerr = XWAIT(LREQ, -1);
    assert(lerr == LREQ);
    lerr = XMSG_LISTEN_((short *) &sre, // sre
                        0,              // listenopts
                        0);             // listenertag
    assert(lerr == XSRETYPE_TPOP);
    util_time_timer_stop(&t_stop);
    util_time_elapsed(&t_start2, &t_stop, &t_elapsed);
    to_act = t_elapsed.tv_sec * 1000000 + t_elapsed.tv_usec;
    to_exp = sre.sre_tleTOVal * 10000;
    if (verbose || (to_act < to_exp))
        printf("tleid=%d, exp-to=%ld, act-to=%ld\n",
               sre.sre_tleId, to_exp, to_act);
    assert(to_act >= to_exp);
    if (to_tol > 0)
        assert(to_act <= (to_exp + to_tol));
}

void test_timers_alloc() {
    enum {          MAX_TLES = 98 };
    _xcc_status     cc;
    short           inx;
    int             inx2;
    short           tleid[MAX_TLES];

    if (verbose)
        printf("timer-test-alloc\n");

    for (inx2 = 0; inx2 < MAX_TLES; inx2++) {
        for (inx = 0; inx < MAX_TLES; inx++) {
            cc = XSIGNALTIMEOUT(100, inx, inx, &tleid[inx]);
            TEST_CHK_CCEQ(cc);
        }
        for (inx = 0; inx < MAX_TLES; inx++) {
            cc = XCANCELTIMEOUT(tleid[inx]);
            TEST_CHK_CCEQ(cc);
        }
        cc = XSIGNALTIMEOUT(100, inx, inx, &tleid[0]);
        TEST_CHK_CCEQ(cc);
        cc = XCANCELTIMEOUT(tleid[0]);
        TEST_CHK_CCEQ(cc);
    }
}

void test_timers_cb() {
    int             ferr;
    int             status;
    struct timeval  t_elapsed;
    struct timeval  t_start1;
    struct timeval  t_start2;
    struct timeval  t_stop;
    short           tleid1;
    short           tleid2;
    long            to_act;
    long            to_exp;

    if (verbose)
        printf("timer-cb-test\n");

    // check timers
    util_time_timer_start(&t_start1);
    ferr = timer_start_cb(40, 2, 2, &tleid1, &cb1);
    assert(ferr == XZFIL_ERR_OK);
    usleep(30000);
    util_time_timer_start(&t_start2);
    ferr = timer_start_cb(40, 4, 4, &tleid2, &cb2);
    assert(ferr == XZFIL_ERR_OK);

    status = cv1.wait(true);
    TEST_CHK_STATUSOK(status);
    if (verbose)
        printf("cv1 wait done\n");
    util_time_timer_stop(&t_stop);
    util_time_elapsed(&t_start1, &t_stop, &t_elapsed);
    to_act = t_elapsed.tv_sec * 1000000 + t_elapsed.tv_usec;
    to_exp = t1.sre_tleTOVal * 10000;
    if (verbose || (to_act < to_exp))
        printf("tleid=%d, exp-to=%ld, act-to=%ld\n",
               t1.sre_tleId, to_exp, to_act);
    assert(to_act >= to_exp);
    if (to_tol > 0)
        assert(to_act <= (to_exp + to_tol));

    status = cv2.wait(true);
    TEST_CHK_STATUSOK(status);
    if (verbose)
        printf("cv2 wait done\n");
    util_time_timer_stop(&t_stop);
    util_time_elapsed(&t_start2, &t_stop, &t_elapsed);
    to_act = t_elapsed.tv_sec * 1000000 + t_elapsed.tv_usec;
    to_exp = t2.sre_tleTOVal * 10000;
    if (verbose || (to_act < to_exp))
        printf("tleid=%d, exp-to=%ld, act-to=%ld\n",
               t2.sre_tleId, to_exp, to_act);
    assert(to_act >= to_exp);
    if (to_tol > 0)
        assert(to_act <= (to_exp + to_tol));
}

void test_thread1() {
    int inx;
    int status;

    if (verbose)
        printf("thread1-test\n");

    for (inx = 0; inx < maxth; inx++) {
        char name[10];
        sprintf(name, "s%d", inx);
        thr[inx] = new Thread_Test(thr_test, name, inx);
    }
    for (inx = 0; inx < maxth; inx++) {
        thr[inx]->start();
        SB_Thread::Sthr::yield();
    }
    for (inx = 0; inx < maxth; inx++) {
        void *res;
        status = thr[inx]->join(&res);
        TEST_CHK_STATUSOK(status);
        printf("joined with thread %d\n", inx);
    }
}

void test_thread2() {
    int inx;
    int status;

    if (verbose)
        printf("thread2-test\n");

    tleid_test2 = -1;
    for (inx = 0; inx < 2; inx++) {
        char name[10];
        sprintf(name, "s%d", inx);
        thr2[inx] = new Thread_Test2(thr_test2, name, inx);
    }
    for (inx = 0; inx < 2; inx++) {
        thr2[inx]->start();
        SB_Thread::Sthr::yield();
    }
    for (inx = 0; inx < 2; inx++) {
        void *res;
        status = thr2[inx]->join(&res);
        TEST_CHK_STATUSOK(status);
        printf("joined with thread %d\n", inx);
    }
}

void test_thread3() {
    int inx;
    int status;

    if (verbose)
        printf("thread3-test\n");

    for (inx = 0; inx < maxth; inx++) {
        char name[10];
        sprintf(name, "s%d", inx);
        thr3[inx] = new Thread_Test3(thr_test3, name, inx);
    }
    for (inx = 0; inx < maxth; inx++) {
        thr3[inx]->start();
        SB_Thread::Sthr::yield();
    }
    for (inx = 0; inx < maxth; inx++) {
        void *res;
        status = thr3[inx]->join(&res);
        TEST_CHK_STATUSOK(status);
        printf("joined with thread %d\n", inx);
    }
}


int main(int argc, char *argv[]) {
    bool  chook = false;
    int   ferr;
    int   loop = 10;
    TAD   zargs[] = {
      { "-chook",     TA_Bool, TA_NOMAX,    &chook     },
      { "-client",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-maxth",     TA_Int,  MAX_THR,     &maxth     },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-tol",       TA_Int,  TA_NOMAX,    &to_tol    },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    if (chook)
        msg_debug_hook("c", "c");
    util_test_start(true);
    ferr = msg_mon_process_startup(true);  // system messages?
    TEST_CHK_FEOK(ferr);
    util_gethostname(my_name, sizeof(my_name));

#if 0 // reproduce bug 1570
    // - add sleep(1) in sb_timer_setitimer when pv_to != 0
#endif
    test_cancel();
    test_timers();
    test_timers_alloc();
    test_thread1();
    test_cancel_cb();
    test_timers_cb();
    test_thread3();
#if 0 // reproduce bug 728
// - must add sleep(1) after call-to-sb_timer_comp_q_remove() in MSG_LISTEN_
    test_thread2();
#endif

    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(true);
    return 0;
}
