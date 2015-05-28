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

#include "seabed/fserr.h"
#include "seabed/ms.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"

enum { MAX_SRV = 5 };

int main(int argc, char *argv[]) {
    int                      count;
    int                      ferr;
    MS_Mon_Process_Info_Type info[MAX_SRV];
    int                      inx;
    char                     prog[MS_MON_MAX_PROCESS_PATH];
    char                     ret_name[MAX_SRV][MS_MON_MAX_PROCESS_NAME];
    char                     server_name[MAX_SRV][20];
    int                      server_nid[MAX_SRV];
    TPT_DECL2               (server_phandle,MAX_SRV);
    int                      server_pid[MAX_SRV];

    ferr = msfs_util_init_role(true, &argc, &argv, msg_debug_hook);
    TEST_CHK_FEOK(ferr);
    util_test_start(true);
    ferr = msg_mon_process_startup(false); // NO system messages
    TEST_CHK_FEOK(ferr);

    sprintf(prog, "%s/t42srv", getenv("PWD"));
    for (inx = 0; inx < MAX_SRV; inx++) {
        sprintf(server_name[inx], "$SRV%d", inx);
        server_nid[inx] = -1;
        ferr = msg_mon_start_process(prog,                   // prog
                                     server_name[inx],       // name
                                     ret_name[inx],          // ret name
                                     argc,
                                     argv,
                                     TPT_REF2(server_phandle,inx),
                                     1,                      // open
                                     NULL,                   // oid
                                     MS_ProcessType_TSE,     // type
                                     0,                      // priority
                                     0,                      // debug
                                     0,                      // backup
                                     &server_nid[inx],       // nid
                                     &server_pid[inx],       // pid
                                     NULL,                   // infile
                                     NULL);                  // outfile
        TEST_CHK_FEOK(ferr);
    }
    ferr = msg_mon_get_process_info_type(MS_ProcessType_TSE,
                                         &count,
                                         MAX_SRV,
                                         info);
    TEST_CHK_FEOK(ferr);
    assert(count == MAX_SRV);
    for (inx = 0; inx < MAX_SRV; inx++) {
        ferr = msg_mon_close_process(TPT_REF2(server_phandle,inx));
        TEST_CHK_FEOK(ferr);
    }
    printf("client shutting down\n");
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(true);
    return 0;
}
