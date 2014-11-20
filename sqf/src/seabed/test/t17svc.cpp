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

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

int main(int argc, char *argv[]) {
    int       ferr;
    int       oid;
    TPT_DECL (phandle);
    bool      verbose = false;
    TAD       zargs[] = {
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    ferr = msg_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    if (getenv("DEBUGSVC") != NULL)
        msg_debug_hook("svc", "svc");
    arg_proc_args(zargs, false, argc, argv);
    ferr = msg_mon_process_startup(false); // system messages
    TEST_CHK_FEOK(ferr);
    // process-wait for client/server/service/shell
    ferr = msfs_util_wait_process_count(MS_ProcessType_Generic, 4, NULL, false);
    TEST_CHK_FEOK(ferr);
    sleep(2); // so that cli/srv detect
    if (verbose)
        printf("svc opening cli\n");
    ferr = msg_mon_open_process((char *) "$cli",      // name
                                TPT_REF(phandle),
                                &oid);
    TEST_CHK_FEOK(ferr);
    if (verbose)
        printf("svc closing cli\n");
    ferr = msg_mon_close_process(TPT_REF(phandle));
    TEST_CHK_FEOK(ferr);
    if (verbose)
        printf("svc doing shutdown\n");
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    return 0;
}
