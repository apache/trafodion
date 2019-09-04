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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

#include "seabed/fserr.h"
#include "seabed/ms.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"


int main(int argc, char *argv[]) {
    int          arg;
    bool         client = false;
    char         core_file[MS_MON_MAX_PROCESS_PATH];
    int          err;
    int          ferr;
    bool         id = false;
    int          inx;
    int          loop = 1;
    char         my_name[BUFSIZ];
    char         prog[MS_MON_MAX_PROCESS_PATH];
    char         ret_name[MS_MON_MAX_PROCESS_NAME];
    bool         save = false;
    const char  *server_name = "$SRV";
    int          server_nid = -1;
    int          server_pid;
    TPT_DECL    (server_phandle);
    bool         shell = false;
    bool         sleepv = false;
    struct stat  statbuf;
    char        *vn;
    TAD          zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-id",        TA_Bool, TA_NOMAX,    &id        },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-save",      TA_Bool, TA_NOMAX,    &save      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-shell",     TA_Bool, TA_NOMAX,    &shell     },
      { "-sleep",     TA_Bool, TA_NOMAX,    &sleepv    },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    if (client && shell)
        ferr = msg_init_attach(&argc, &argv, true, (char *) "$CLI");
    else
        ferr = msg_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    util_test_start(client);
    ferr = msg_mon_process_startup(false); // system messages
    TEST_CHK_FEOK(ferr);
    util_gethostname(my_name, sizeof(my_name));

    if (client && !shell) {
        vn = getenv("SQ_VIRTUAL_NODES");
        if (vn == NULL)
            server_nid = 0; // real cluster needs same node
        sprintf(prog, "%s/%s", getenv("PWD"), argv[0]);
        for (arg = 0; arg < argc; arg++)
            if (strcmp(argv[arg], "-client") == 0) // start_process
                argv[arg] = (char *) "-server";
        ferr = msg_mon_start_process(prog,                   // prog
                                     (char *) server_name,   // name
                                     ret_name,               // ret name
                                     argc,
                                     argv,
                                     TPT_REF(server_phandle),
                                     0,                      // open
                                     NULL,                   // oid
                                     MS_ProcessType_Generic, // type
                                     0,                      // priority
                                     0,                      // debug
                                     0,                      // backup
                                     &server_nid,            // nid
                                     &server_pid,            // pid
                                     NULL,                   // infile
                                     NULL);                  // outfile
        TEST_CHK_FEOK(ferr);
        // process-wait for client/server/shell
        ferr = msfs_util_wait_process_count(MS_ProcessType_Generic, 3, NULL, false);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            if (id)
                ferr = msg_mon_dump_process_id(NULL,
                                               server_nid,
                                               server_pid,
                                               core_file);
            else
                ferr = msg_mon_dump_process_name(NULL,
                                                 server_name,
                                                 core_file);
            TEST_CHK_FEOK(ferr);
            printf("core-file=%s\n", core_file);
            char *pch;
            pch= strtok (core_file,":");
            pch = strtok (NULL,":");
            printf("pch=%s\n", pch);
            err = stat(pch, &statbuf);
            int error = errno;
            printf("stat() failed! - err=%d errno=%d (%s)\n"
                  , err
                  , error
                  , strerror(error) );
            assert(err == 0);
            if (!save)
            {
                printf("Removing core-file=%s\n", core_file);
                unlink(pch);
            }
            if ((loop > 1) && sleepv)
                sleep(1);
        }
    }
    if (client && shell)
        sleep(4);
    else if (!client) {
        for (;;) {
            sleep(1);
        }
    }
    if (client && !shell) {
        ferr = msg_mon_stop_process((char *) server_name, -1, -1);
        TEST_CHK_FEIGNORE(ferr);
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
