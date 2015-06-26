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

int main(int argc, char *argv[]) {
    char      bufi[BUFSIZ];
    char      bufo[BUFSIZ];
    int       ferr;
    FILE     *file;
    char      infile[MS_MON_MAX_PROCESS_PATH];
    char      outfile[MS_MON_MAX_PROCESS_PATH];
    char     *p;
    char      prog[MS_MON_MAX_PROCESS_PATH];
    int       server_nid;
    TPT_DECL (server_phandle);
    int       server_pid;

    // create infile/outfile
    p = getenv("PWD");
    assert(p != NULL);
    sprintf(infile, "%s/z70.infile", p);
    sprintf(outfile, "%s/z70.outfile", p);
    file = fopen(infile, "w");
    assert(file != NULL);
    sprintf(bufo, "infile=%s\n", infile);
    fputs(bufo, file);
    fclose(file);

    ferr = msfs_util_init_role(true, &argc, &argv, msg_debug_hook);
    TEST_CHK_FEOK(ferr);
    util_test_start(true);
    ferr = msg_mon_process_startup(true); // system messages
    TEST_CHK_FEOK(ferr);

    sprintf(prog, "%s/t70srv", getenv("PWD"));
    server_nid = -1;
    ferr = msg_mon_start_process(prog,                   // prog
                                 (char *) "$srv",        // name
                                 NULL,                   // ret name
                                 argc,
                                 argv,
                                 TPT_REF(server_phandle),
                                 1,                      // open
                                 NULL,                   // oid
                                 MS_ProcessType_TSE,     // type
                                 0,                      // priority
                                 0,                      // debug
                                 0,                      // backup
                                 &server_nid,            // nid
                                 &server_pid,            // pid
                                 infile,                 // infile
                                 outfile);               // outfile
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_close_process(TPT_REF(server_phandle));
    TEST_CHK_FEOK(ferr);
    printf("client shutting down\n");

    // open outfile and compare it to infile
    file = fopen(outfile, "r");
    assert(file != NULL);
    p = fgets(bufi, BUFSIZ, file);
    assert(p != NULL);
    if (memcmp(bufi, "HP-MPI", 6) == 0) {
        // ignore banner
        p = fgets(bufi, BUFSIZ, file);
        assert(p != NULL);
    }
    assert(strcmp(bufi, bufo) == 0);
    fclose(file);
    unlink(infile);
    unlink(outfile);

    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(true);
    return 0;
}
