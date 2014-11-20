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
#include <unistd.h>

#include "seabed/debug.h"
#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/thread.h"
#include "seabed/trace.h"

#include "tmsfsutil.h"
#include "tchkfe.h"
#include "tchkos.h"
#include "tutil.h"
#include "tutilp.h"

enum { MAX_THREADS = 20 };

bool sleepv = false;
bool verbose = false;

class My_Thread : public SB_Thread::Thread {
public:
    My_Thread(const char *name);
    void fin();
    void run();

private:
    bool ifin;
};

My_Thread *mythreads[MAX_THREADS];

void *my_thread_fun(void *pp_arg) {
    My_Thread *lp_thread = (My_Thread *) pp_arg;
    lp_thread->run();
    return NULL;
}

My_Thread::My_Thread(const char *name)
: SB_Thread::Thread(my_thread_fun, name), ifin(false) {
}

void My_Thread::fin() {
    ifin = true;
}

void My_Thread::run() {
    while (__sync_fetch_and_add_1(&ifin, 0) == 0) { // memory barrier
        if (sleepv)
            usleep(1000);
    }
    if (verbose)
        printf("exiting thread=%s\n", get_name());
}

int main(int argc, char *argv[]) {
    bool  debug = false;
    int   disable;
    FILE *f;
    int   ferr;
    int   inx;
    char  line[100];
    int   loop = 10;
    bool  ps = false;
    void *result;
    bool  solo = false;
    int   status;
    int   threads = 0;
    TAD              zargs[] = {
      { "-client",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-cluster",   TA_Ign,  TA_NOMAX,    NULL       },
      { "-debug",     TA_Bool, TA_NOMAX,    &debug     },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-ps",        TA_Bool, TA_NOMAX,    &ps        },
      { "-sleep",     TA_Bool, TA_NOMAX,    &sleepv    },
      { "-solo",      TA_Bool, TA_NOMAX,    &solo      },
      { "-threads",   TA_Int,  MAX_THREADS, &threads   },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "-verbose",   TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    util_test_start(true);
    if (solo)
        msg_init_trace();
    else {
        msfs_util_init(&argc, &argv, msg_debug_hook);
        ferr = msg_mon_process_startup(true);    // system messages?
        TEST_CHK_FEOK(ferr);
    }

    if (debug) {
        f = fopen("c", "w");
        fclose(f);
    }

    if (threads > 0) {
        for (inx = 0; inx < threads; inx++) {
            sprintf(line, "thr-%d", inx);
            mythreads[inx] = new My_Thread(line);
        }
        for (inx = 0; inx < threads; inx++)
            mythreads[inx]->start();
    }

    if (ps) {
        sprintf(line, "ps -p %d -T -o lwp,stat", getpid());
        if (verbose)
            printf("before suspend - %s\n", line);
        system(line);
    }

    disable = msg_test_assert_disable();
    ferr = thread_resume_suspended();
    assert(ferr == XZFIL_ERR_INVALIDSTATE);
    msg_test_assert_enable(disable);

    if (verbose)
        printf("calling thread_suspend_all\n");
    ferr = thread_suspend_all();
    TEST_CHK_FEOK(ferr);

    if (ps) {
        if (verbose)
            printf("after suspend - %s\n", line);
        system(line);
    }

    disable = msg_test_assert_disable();
    ferr = thread_suspend_all();
    assert(ferr == XZFIL_ERR_INVALIDSTATE);
    msg_test_assert_enable(disable);

    if (debug) {
        unlink("c");
        if (verbose)
            printf("calling msg_debug_hook\n");
        msg_debug_hook("c", "c");
    }

    if (verbose)
        printf("calling thread_resume_suspended\n");
    ferr = thread_resume_suspended();
    TEST_CHK_FEOK(ferr);

    if (ps) {
        if (verbose)
            printf("after resume - %s\n", line);
        system(line);
    }

    disable = msg_test_assert_disable();
    ferr = thread_resume_suspended();
    assert(ferr == XZFIL_ERR_INVALIDSTATE);
    msg_test_assert_enable(disable);

    for (inx = 0; inx < loop; inx++) {
        if (verbose)
            printf("calling thread_suspend_all\n");
        ferr = thread_suspend_all();
        TEST_CHK_FEOK(ferr);
        if (verbose)
            printf("calling thread_resume_suspended\n");
        ferr = thread_resume_suspended();
        TEST_CHK_FEOK(ferr);
    }

    if (threads > 0) {
        if (verbose)
            printf("setting fin on threads\n");
        for (inx = 0; inx < threads; inx++)
            mythreads[inx]->fin();
        if (verbose)
            printf("joining threads\n");
        for (inx = 0; inx < threads; inx++) {
            status = mythreads[inx]->join(&result);
            TEST_CHK_STATUSOK(status);
        }
        if (verbose)
            printf("threads joined\n");
    }

    if (debug) {
        f = fopen("c", "w");
        fclose(f);
    }

    if (!solo) {
        ferr = msg_mon_process_shutdown();
        TEST_CHK_FEOK(ferr);
    }

    util_test_finish(true);
    return 0;
}
