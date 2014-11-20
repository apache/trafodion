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

#include "seabed/fs.h"
#include "seabed/ms.h"

#include "tchkfe.h"
#include "tutil.h"
#include "tutilp.h"


int main(int argc, char *argv[]) {
    int         arg;
    bool        attach = false;
    bool        chook = false;
    bool        client = false;
    bool        disable = false;
    int         ferr;
    char        my_name[BUFSIZ];
    char        prog[MS_MON_MAX_PROCESS_PATH];
    char        ret_name[MS_MON_MAX_PROCESS_NAME];
    const char *server_name = "$SRV";
    int         server_nid = 1;
    TPT_DECL   (server_phandle);
    int         server_pid;
    bool        shook = false;
    TAD         zargs[] = {
      { "-attach",    TA_Bool, TA_NOMAX,    &attach    },
      { "-chook",     TA_Bool, TA_NOMAX,    &chook     },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-shook",     TA_Bool, TA_NOMAX,    &shook     },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    if (chook && client)
        test_debug_hook("c", "c");
    if (shook && !client)
        test_debug_hook("s", "s");
    if (client && attach)
        ferr = msg_init_attach(&argc, &argv, true, (char *) "$CLI");
    else
        ferr = msg_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client); // system messages
    TEST_CHK_FEOK(ferr);
    util_gethostname(my_name, sizeof(my_name));

    if (client) {
        sprintf(prog, "%s/%s", getenv("PWD"), argv[0]);
        for (arg = 0; arg < argc; arg++)
            if (strcmp(argv[arg], "-client") == 0) // start_process
                argv[arg] = (char *) "-server";
        printf("starting process\n");
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
        printf("process started, err=%d\n", ferr);
        TEST_CHK_FEOK(ferr);
        assert(strcmp(ret_name, server_name) == 0);

        printf("i'm the client\n");
    } else {
        FILE *f = fopen("zos", "w");
        assert(f != NULL);
        fprintf(f, "i'm the server sleeping 10\n");
        fflush(f);
        sleep(5);
        fprintf(f, "i'm the server done\n");
        fclose(f);
    }
    if (!client)
        disable = msg_test_assert_disable();
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    if (!client)
        msg_test_assert_enable(disable);
    util_test_finish(client);
    return 0;
}
