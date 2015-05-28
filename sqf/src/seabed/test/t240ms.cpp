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

#include "seabed/fserr.h"
#include "seabed/ms.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

const char *core_prefix = "zt240ms";

void do_core1(char *prog) {
    FILE  *f;
    char   file_err[100];
    char   file_in[100];
    char   file_out[100];
    char   file_pid[100];
    char   line[BUFSIZ];
    pid_t  pid;

    pid = getpid();

    // create pid file
    sprintf(file_pid, "%s.pid", core_prefix);
    f = fopen(file_pid, "w");
    fprintf(f, "%d", pid);
    fclose(f);

    // create gdb stdout/stderr file
    sprintf(file_out, "/tmp/%s.out.%d", core_prefix, pid);
    sprintf(file_err, "/tmp/%s.err.%d", core_prefix, pid);

    // create gdb input file
    sprintf(file_in, "/tmp/%s.in.%d", core_prefix, pid);
    f = fopen(file_in, "w");
    fprintf(f, "gcore %s.core.%d", core_prefix, pid);
    fclose(f);

    // tell gdb create core-file
    sprintf(line, "gdb %s %d < %s > %s 2>%s",
            prog,
            pid,
            file_in,
            file_out,
            file_err);
    system(line);
    util_test_finish(true);
    util_abort_core_free();
}

int main(int argc, char *argv[]) {
    bool core1 = false;
    int  ferr;
    TAD  zargs[] = {
      { "-client",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-core1",     TA_Bool, TA_NOMAX,    &core1     },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    if (!core1)
        core1 = true;
    util_test_start(true);
    ferr = msg_mon_process_startup(false);  // system messages?
    TEST_CHK_FEOK(ferr);
    if (core1)
        do_core1(argv[0]);
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    return 0;
}
