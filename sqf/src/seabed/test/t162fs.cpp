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
#include <string.h>
#include <unistd.h>

#include <linux/unistd.h>

#include "seabed/fserr.h"
#include "seabed/fs.h"
#include "seabed/thread.h"
#include "seabed/timer.h"

#include "tchkfe.h"
#include "tchkos.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

#define gettid() (pid_t) syscall(__NR_gettid)

enum { MAX_THREADS = 10 };
SB_Thread::CV      cv;
short              filenum;
bool               nowait = false;
bool               onercv = false;
pid_t              rtid = 0;
struct timeval     t_start;
SB_Thread::Thread *thr_rcv;
SB_Thread::Thread *thr_time[MAX_THREADS];
int                threads = 1;
bool               verbose = false;

void do_rcv(pid_t tid) {
    void                *buf;
    _xcc_status          cc;
    unsigned short       count_xferred;
    int                  msg;
    char                 recv_buffer[BUFSIZ];
    xzsys_ddl_smsg_def  *sys_msg = (xzsys_ddl_smsg_def *) recv_buffer;
    struct timeval       t_elapsed;
    struct timeval       t_stop;
    SB_Tag_Type          tag;
    short                tfilenum;
    int                  timeout = -1;
    long                 to_act;
    long                 to_exp;

    for (msg = 0; msg < 3; msg++) {
        cc = XREADUPDATEX(filenum,
                          recv_buffer,  // buffer
                          BUFSIZ,       // read_count
                          0,            // count_read
                          0);           // tag
        if (nowait) {
            TEST_CHK_CCEQ(cc);
            tfilenum = -1;
            cc = XAWAITIOX(&tfilenum,
                           &buf,
                           &count_xferred,
                           &tag,
                           timeout,
                           NULL);
        }
        assert(_xstatus_gt(cc));
        int mt = sys_msg->u_z_msg.z_msgnumber[0];
        printf("tid=%d, msg[%d].type=%d\n", tid, msg, mt);
        assert(sys_msg->u_z_msg.z_msgnumber[0] == XZSYS_VAL_SMSG_TIMESIGNAL);
        util_time_timer_stop(&t_stop);
        util_time_elapsed(&t_start, &t_stop, &t_elapsed);
        to_act = t_elapsed.tv_sec * 1000000 + t_elapsed.tv_usec;
        printf("tid=%d, msg[%d].p1=%d, p2=%ld\n",
               tid,
               msg,
               sys_msg->u_z_msg.z_timesignal.z_parm1,
               sys_msg->u_z_msg.z_timesignal.z_parm2);
        switch (msg) {
        case 0:
            assert(sys_msg->u_z_msg.z_timesignal.z_parm1 == 1);
            assert(sys_msg->u_z_msg.z_timesignal.z_parm2 == 10);
            break;
        case 1:
            assert(sys_msg->u_z_msg.z_timesignal.z_parm1 == 3);
            assert(sys_msg->u_z_msg.z_timesignal.z_parm2 == 20);
            break;
        case 2:
            assert(sys_msg->u_z_msg.z_timesignal.z_parm1 == 5);
            assert(sys_msg->u_z_msg.z_timesignal.z_parm2 == 30);
            break;
        }
        to_exp = sys_msg->u_z_msg.z_timesignal.z_parm2 * 10000;
        if (verbose || (to_act < to_exp))
            printf("exp-to=%ld, act-to=%ld\n",
                   to_exp, to_act);
        assert(to_act >= to_exp);
        cc = XREPLYX(NULL,  // buffer
                     0,     // write_count
                     NULL,  // count_written
                     0,     // reply_num
                     0);    // err_ret
        TEST_CHK_CCEQ(cc);
        if (mt == XZSYS_VAL_SMSG_SHUTDOWN)
            break;
    }
    if (onercv) {
        if (verbose)
            printf("waking timer thread\n");
        cv.signal(true);
    }
}

void do_timers() {
    _xcc_status cc;
    int         status;
    int         tid;
    short       tleid1;
    short       tleid2;
    short       tleid3;

    util_time_timer_start(&t_start);
    tid = gettid();
    if (threads <= 1) {
        cc = XSIGNALTIMEOUT(10, 1, 10, &tleid1);
        TEST_CHK_CCEQ(cc);
        cc = XSIGNALTIMEOUT(20, 3, 20, &tleid2);
        TEST_CHK_CCEQ(cc);
        cc = XSIGNALTIMEOUT(30, 5, 30, &tleid3);
        TEST_CHK_CCEQ(cc);
    } else {
        cc = XSIGNALTIMEOUT(10, 1, 10, &tleid1, rtid);
        TEST_CHK_CCEQ(cc);
        cc = XSIGNALTIMEOUT(20, 3, 20, &tleid2, rtid);
        TEST_CHK_CCEQ(cc);
        cc = XSIGNALTIMEOUT(30, 5, 30, &tleid3, rtid);
        TEST_CHK_CCEQ(cc);
    }
    if (threads <= 1)
        util_cpu_timer_wait(9);
    if (onercv) {
        if (verbose)
            printf("waiting for rcv\n");
        status = cv.wait(true);
        TEST_CHK_STATUSOK(status);
    } else
        do_rcv(tid);
}

void *thread_rcv(void *arg) {
    int   ferr;
    int   inx;
    pid_t tid;

    arg = arg; // touch
    ferr = timer_register();
    TEST_CHK_FEOK(ferr);
    tid = gettid();
    if (onercv)
        rtid = tid;
    else
        rtid = 0;
    cv.signal(true);
    for (inx = 0; inx < threads; inx++)
        do_rcv(tid);
    return NULL;
}

void *thread_rcv_fun(void *arg) {
    return thread_rcv(arg);
}

void *thread_time(void *arg) {
    arg = arg; // touch
    do_timers();
    return NULL;
}

void *thread_time_fun(void *arg) {
    return thread_time(arg);
}


int main(int argc, char *argv[]) {
    int   ferr;
    int   inx;
    void *result;
    int   status;
    char  thread_name[20];
    TAD   zargs[] = {
      { "-client",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-nowait",    TA_Bool, TA_NOMAX,    &nowait    },
      { "-onercv",    TA_Bool, TA_NOMAX,    &onercv    },
      { "-threads",   TA_Int,  MAX_THREADS, &threads   },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    util_test_start(true);
    ferr = file_mon_process_startup(true); // system messages
    TEST_CHK_FEOK(ferr);
    ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                       0, 0,
                       nowait ? (short) 1 : (short) 0,
                       1, 0, // sys msg
                       0, 0, NULL);
    TEST_CHK_FEOK(ferr);

    if (threads > 1) {
        if (onercv) {
            thr_rcv = new SB_Thread::Thread(thread_rcv_fun, "rcv");
            thr_rcv->start();
            status = cv.wait(true);
            TEST_CHK_STATUSOK(status);
        }
        for (inx = 0; inx < threads; inx++) {
            sprintf(thread_name, "time%d", inx);
            thr_time[inx] = new SB_Thread::Thread(thread_time_fun, thread_name);
        }
        for (inx = 0; inx < threads; inx++) {
            thr_time[inx]->start();
            status = thr_time[inx]->join(&result);
            TEST_CHK_STATUSOK(status);
        }
        if (onercv) {
            status = thr_rcv->join(&result);
            TEST_CHK_STATUSOK(status);
        }
    } else {
        do_timers();
    }

    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(true);
    printf("if there were no asserts, all is well\n");
    return 0;
}
