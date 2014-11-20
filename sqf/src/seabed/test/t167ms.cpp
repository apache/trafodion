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

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"
#include "seabed/thread.h"

#include "tchkfe.h"
#include "tchkos.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

enum {
    TIME_TOLERANCE = 3000,   // us
    CV_COUNT       = 10,     // count
    CV_TO          = 100000  // us
};

int  tol = TIME_TOLERANCE;
bool toldef = true;


int main(int argc, char *argv[]) {
    SB_Thread::CV  cv;
    int            cv_overhead;
    int            ferr;
    int            inx;
    int            lerr;
    int            status;
    struct timeval t_elapsed;
    struct timeval t_start;
    struct timeval t_stop;
    long           tact;
    long           texp;
    int            tout;
    TAD            zargs[] = {
      { "-client",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-tol",       TA_Int,  TA_NOMAX,    &tol       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    if (tol != TIME_TOLERANCE) {
        toldef = false;
        printf("using tol=%d\n", tol);
    }
    msfs_util_init(&argc, &argv, msg_debug_hook);
    util_test_start(true);
    ferr = msg_mon_process_startup(true); // system messages
    TEST_CHK_FEOK(ferr);

    // get CV wait average
    util_time_timer_start(&t_start);
    for (inx = 0; inx < CV_COUNT; inx++) {
        status = cv.wait(true, 0, CV_TO);
        assert(status == ETIMEDOUT);
    }
    util_time_timer_stop(&t_stop);
    util_time_elapsed(&t_start, &t_stop, &t_elapsed);
    tact = (long) t_elapsed.tv_sec * USPS + t_elapsed.tv_usec;
    cv_overhead = (int) ((tact - (CV_COUNT * CV_TO)) / CV_COUNT);
    printf("CV to overhead=%d\n", cv_overhead);

    // bump tolerance by averaged overhead
    if (toldef) {
        tol += cv_overhead;
        printf("changing tol=%d\n", tol);
    }

    tout = 0;
    for (inx = 0; tout <= 256; inx++) {
        printf("tout=%d\n", tout);
        util_time_timer_start(&t_start);
        lerr = XWAIT(0, tout);
        TEST_CHK_WAITIGNORE(lerr);
        util_time_timer_stop(&t_stop);
        util_time_elapsed(&t_start, &t_stop, &t_elapsed);
        tact = (long) t_elapsed.tv_sec * USPS + t_elapsed.tv_usec;
        texp = (long) tout * 10 * 1000;
        printf("actual elapsed=%ld.%06ld\n",
               t_elapsed.tv_sec, t_elapsed.tv_usec);
        if ((tact > texp + tol) ||
            (tact < texp - tol)) {
            printf("expected elapsed=%ld.%06ld\n",
                   texp / USPS,
                   texp % USPS);
            assert(tact <= texp + tol);
            assert(tact >= texp - tol);
        }
        if (tout == 0)
            tout = 1;
        else
            tout = tout * 2;
    }

    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(true);
    printf("if there were no asserts, all is well\n");
    return 0;
}
