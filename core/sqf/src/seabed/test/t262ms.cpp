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

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/time.h>

#include "seabed/ms.h"
#include "seabed/otimer.h"
#include "seabed/trace.h"

#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

enum { TOL = 1 };

bool verbose = false;

class TH1 : public SB_Timer::TH {
public:
    TH1() {}
    ~TH1() {}
    void handle_timeout(SB_Timer::Timer *pp_timer) {
        static long count = 0;
        long        user_param;

        count++;
        user_param = pp_timer->get_param();
        if (verbose)
            printf("TH1::to timer=%p, param=%ld, count=%ld\n",
                   (void *) pp_timer, user_param, count);
        assert(user_param == count);
    }

};

SB_Timer::Tics get_curr_tics() {
    SB_Timer::Tics tics;
    timeval         tod;

    gettimeofday(&tod, NULL);
    tics = ((SB_Timer::Tics) tod.tv_sec * SB_Timer::Time_Stamp::TICS_PER_SEC) +
           (tod.tv_usec / SB_Timer::Time_Stamp::US_PER_TIC);
    return tics;
}

void print_timer(SB_Timer::Timer &timer) {
    char       buf[100];
    char       buf_pop[100];
    static int which = 0;

    which = 1 - which;
    if (which) {
        timer.format_pop_time(buf_pop);
        sprintf(buf, "timer=%p, user-param=%ld, interval=" PF64 ", pop-time=%s",
                (void *) &timer, timer.get_param(), timer.get_interval(), buf_pop);
    } else
        timer.format_timer(buf);
    printf("%s\n", buf);
}

int main(int argc, char *argv[]) {
    char                  buf[100];
    bool                  client = false;
    SB_Timer::Tics        exp_tics;
    SB_Timer::Tics        tics;
    bool                  trace = false;
    SB_Timer::Time_Stamp  ts1;
    SB_Timer::Tics        ts1_tics;
    SB_Timer::Tics        ts2_tics;
    SB_Timer::Time_Stamp  ts2;
    SB_Timer::Tics        ts3_tics;
    TAD                   zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-cluster",   TA_Ign,  TA_NOMAX,    NULL       },
      { "-trace",     TA_Bool, TA_NOMAX,    &trace     },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "-verbose",   TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    exp_tics = get_curr_tics();
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    if (trace) {
        trace_init(NULL, false, NULL, true);
        SB_Timer::Timer::cv_trace_enabled = true;
    }

    // test timestamp
    printf("if there were no asserts, all is well\n");
    if (verbose)
        printf("exp-tics=" PF64 "\n", exp_tics);
    printf("ts1 %s\n", ts1.format_ts(buf));
    ts1_tics = ts1.tic_get();
    if (verbose)
        printf("ts1-tics=" PF64 "\n", ts1_tics);
    assert(ts1_tics >= exp_tics);
    assert(ts1_tics <= exp_tics + TOL);
    assert(ts1.tic_add(1) == (ts1_tics + 1));
    exp_tics = get_curr_tics() + 1;
    ts2.tic_set_now_add(1);
    ts2_tics = ts2.tic_get();
    if (verbose)
        printf("ts2-tics=" PF64 "\n", ts2_tics);
    assert(ts2_tics >= exp_tics);
    assert(ts2_tics <= exp_tics + TOL);
    SB_Timer::Time_Stamp ts3(ts2);
    exp_tics = get_curr_tics();
    ts3_tics = ts3.tic_get();
    if (verbose)
        printf("ts3-tics=" PF64 "\n", ts3_tics);
    assert(ts3_tics >= exp_tics);
    assert(ts3_tics <= exp_tics + TOL);
    assert(ts3.ts_eq(ts2));
    assert(ts3.ts_ge(ts2));
    assert(ts2.ts_gt(ts1));
    assert(ts1.ts_lt(ts2));
    assert(ts1.ts_le(ts2));
    assert(ts2.ts_ne(ts1));

    // test timer
    SB_Timer::Timer::init();
    TH1 th1;
    SB_Timer::Timer t1(&th1,
                       1,      // param
                       2,      // interval
                       false); // start
    SB_Timer::Timer t2(&th1,
                       2,      // param
                       2,      // interval
                       false); // start
    SB_Timer::Timer t3(&th1,
                       -3,     // param
                       258,    // interval (into different slot)
                       false); // start
    t1.start();
    t1.cancel();
    t1.set_interval(1,      // interval
                    false); // start
    t1.set_interval(1,      // interval
                    true);  // start
    t2.start();
    t2.start(); // check re-start
    t3.set_param(3);
    t3.start();
    SB_Timer::Timer::print_timers(print_timer);
    SB_Timer::Timer::check_timers();
    tics = SB_Timer::Timer::get_wait_time();
    if (verbose)
        printf("get_wait_time=" PF64 "\n", tics);
    tics = SB_Timer::Timer::get_wait_time();
    if (verbose)
        printf("get_wait_time=" PF64 "\n", tics);
    usleep(50000);
    SB_Timer::Timer::check_timers();

    util_test_finish(client);
    return 0;
}
