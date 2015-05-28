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
#include <unistd.h>

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"
#include "seabed/thread.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"

int             pin;
long            t_elapsed;
long            t_elapsed2;
struct timeval  t_start;
struct timeval  t_start2;
struct timeval  t_start3;
struct timeval  t_stop;

enum { TOLERANCE = 50000 };

#define check_min(min) { \
  assert(t_elapsed >= min); \
  assert(t_elapsed <= (min + TOLERANCE)); \
}
#define check_max(max) { \
  assert(t_elapsed < max); \
}


void *thr(void *arg) {
    arg = arg; // touch
    usleep(20000);
    XAWAKE(pin, LSEM);
    return NULL;
}

void telapsed(struct timeval *start, struct timeval *stop) {
    t_elapsed = stop->tv_sec * 1000000 + stop->tv_usec -
                start->tv_sec * 1000000 - start->tv_usec;
    printf("elapsed=%ld\n", t_elapsed);
}

void tstart() {
    gettimeofday(&t_start, NULL);
//  printf("start=%ld.%ld\n", t_start.tv_sec, t_start.tv_usec);
}

void tstop() {
    gettimeofday(&t_stop, NULL);
//  printf("stop=%ld.%ld\n", t_stop.tv_sec, t_stop.tv_usec);
    telapsed(&t_start, &t_stop);
}

//
// Check XWAIT*(... -1)
//
void test_wait_neg1(bool zero) {
    short event;

    printf("XWAIT%s0(..., -1)\n", zero ? "" : "NO");
    XAWAKE(pin, LSEM);
    tstart();
    if (zero)
        event = XWAIT0(LSEM, -1);
    else
        event = XWAITNO0(LSEM, -1);
    assert(event == LSEM);
    tstop();
    check_max(TOLERANCE);
}

//
// Check XWAIT*(... -2)
//
void test_wait_neg2(bool zero) {
    short event;

    printf("XWAIT%s0(..., -2)\n", zero ? "" : "NO");
    tstart();
    if (zero)
        event = XWAIT0(0, -2);
    else
        event = XWAITNO0(0, -2);
    assert(event == 0);
    tstop();
    check_max(TOLERANCE);
}

//
// Check XWAITNO0(... 100)
// Check XWAITNO0(... 10)
// With first no timeout
//
void test_wait_pos() {
    short event;

    printf("XWAITNO0(..., 100); XWAITNO0(..., 10)\n");
    tstart();
    t_start2.tv_sec = t_start.tv_sec;
    t_start2.tv_usec = t_start.tv_usec;
    SB_Thread::Sthr::Id_Ptr lthr =
      SB_Thread::Sthr::create((char *) "th", thr, (void *) (long) pin);
    event = XWAITNO0(LSEM, 100);
    assert(event == LSEM);
    tstop();
    check_min(20000);
    tstart();
    event = XWAITNO0(LSEM, 10);
    assert(event == 0);
    tstop();
    check_min(100000);
    void *value;
    int status = SB_Thread::Sthr::join(lthr, &value);
    assert(status == 0);
    SB_Thread::Sthr::delete_id(lthr);
}

//
// Check XWAIT0(... 100)
// Check XWAIT0(... 0)
// With first no timeout
//
void test_wait_pos_no_timeout() {
    short event;

    printf("XWAIT0(..., 100); XWAIT0(..., 0)\n");
    tstart();
    t_start2.tv_sec = t_start.tv_sec;
    t_start2.tv_usec = t_start.tv_usec;
    SB_Thread::Sthr::Id_Ptr lthr =
      SB_Thread::Sthr::create((char *) "th", thr, (void *) (long) pin);
    gettimeofday(&t_start3, NULL);
    event = XWAIT0(LSEM, 100);
    assert(event == LSEM);
    tstop();
    check_min(20000);
    telapsed(&t_start3, &t_stop);
    t_elapsed2 = t_elapsed;
    tstart();
    event = XWAIT0(LSEM, 0);
    assert(event == 0);
    tstop();
    check_min(1000000 - t_elapsed2);
    telapsed(&t_start2, &t_stop);
    check_min(1000000);
    void *value;
    int status = SB_Thread::Sthr::join(lthr, &value);
    assert(status == 0);
    SB_Thread::Sthr::delete_id(lthr);
}

//
// Check XWAIT0(... 50)
// Check XWAIT0(... 0)
// With first timedout
//
void test_wait_pos_timeout() {
    short event;

    printf("XWAIT0(..., 50); XWAIT0(..., 0)\n");
    tstart();
    event = XWAIT0(LSEM, 50);
    assert(event == 0);
    tstop();
    check_min(500000);
    tstart();
    event = XWAIT0(LSEM, 0);
    assert(event == 0);
    tstop();
    check_max(TOLERANCE);
}

int main(int argc, char *argv[]) {
    SB_Thread::CV   cv;
    short           event;
    int             ferr;
    int             ret;

    msfs_util_init(&argc, &argv, msg_debug_hook);
    util_test_start(true);
    ferr = msg_mon_process_startup(false);  // system messages?
    TEST_CHK_FEOK(ferr);

    //
    // Check that cv_wait gets ETIMEDOUT
    //
    for (;;) {
        tstart();
        if ((t_start.tv_usec / 100000) == 8)
            break;
        usleep(10000);
    }
    ret = cv.wait(true, 0, 900000);
    assert(ret == ETIMEDOUT);
    tstop();
    printf("elapsed=%ld\n", t_elapsed);
    check_min(900000);

    pin = 1;
    proc_register_group_pin(-1, pin);

    //
    // Remove LREQ
    //
    event = XWAIT0(LREQ, -2);
    assert(event == LREQ);

    //
    // Test XWAIT0
    //
    test_wait_neg2(true);
    test_wait_neg1(true);
    test_wait_pos_timeout();
    test_wait_pos_no_timeout();

    //
    // Test XWAITNO0
    //
    test_wait_neg2(false);
    test_wait_neg1(false);
    test_wait_pos();

    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(true);
    printf("if there were no asserts, all is well\n");
    return 0;
}
