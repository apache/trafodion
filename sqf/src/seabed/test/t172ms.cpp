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

#include <sys/wait.h>

#include "seabed/fs.h"
#include "seabed/ms.h"

#include "tchkfe.h"
#include "tutil.h"
#include "tutilp.h"


int main(int argc, char *argv[]) {
    int              arg;
    char            *argvn[100];
    char             argvna[100][200];
    bool             chook = false;
    bool             client = false;
    int              ferr;
    int              linx;
    int              loop = 1;
    char             name[20];
    int              pid;
    bool             server0 = false;
    bool             server1 = false;
    bool             server2 = false;
    char            *server_name = (char *) "$SRV";
    bool             shook = false;
    int              status;
    bool             strace = false;
    struct timeval   t_elapsed;
    struct timeval   t_start;
    struct timeval   t_stop;
    bool             test[] = { true, true, true };
    bool             test0 = false;
    bool             test1 = false;
    bool             test2 = false;
    int              tinx;
    int              uerr;
    bool             verbose = false;
    const char      *who;
    TAD              zargs[] = {
      { "-chook",     TA_Bool, TA_NOMAX,    &chook     },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-server0",   TA_Bool, TA_NOMAX,    &server0   },
      { "-server1",   TA_Bool, TA_NOMAX,    &server1   },
      { "-server2",   TA_Bool, TA_NOMAX,    &server2   },
      { "-shook",     TA_Bool, TA_NOMAX,    &shook     },
      { "-strace",    TA_Bool, TA_NOMAX,    &strace    },
      { "-test0",     TA_Bool, TA_NOMAX,    &test0     },
      { "-test1",     TA_Bool, TA_NOMAX,    &test1     },
      { "-test2",     TA_Bool, TA_NOMAX,    &test2     },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    if (test0) {
        test[0] = true;
        test[1] = false;
        test[2] = false;
    } else if (test1) {
        test[0] = false;
        test[1] = true;
        test[2] = false;
    } else if (test2) {
        test[0] = false;
        test[1] = false;
        test[2] = true;
    }
    if (client)
        who = "cli";
    else if (server0)
        who = "srv0";
    else if (server1)
        who = "srv1";
    else
        who = "srv2";
    if (verbose)
        for (arg = 0; arg < argc; arg++)
            printf("%s: argv[%d]=%s\n", who, arg, argv[arg]);
    if (chook && client)
        test_debug_hook("c", "c");
    if (shook && !client)
        test_debug_hook("s", "s");
    if (client) {
        if (verbose)
            printf("%s: about to msg_init\n", who);
        ferr = msg_init(&argc, &argv);
        TEST_CHK_FEOK(ferr);
    } else if (server0) {
        if (verbose)
            printf("%s: about to msg_init_attach\n", who);
        ferr = msg_init_attach(&argc, &argv, false, server_name);
    TEST_CHK_FEOK(ferr);
    } else if (server1) {
        if (verbose)
            printf("%s: about to msg_init_attach\n", who);
        ferr = msg_test_enable_client_only();
        TEST_CHK_FEOK(ferr);
        ferr = msg_init_attach(&argc, &argv, false, server_name);
        TEST_CHK_FEOK(ferr);
    }
    if (client)
        util_test_start(client);
    if (verbose)
        printf("%s: about to msg_mon_process_startup\n", who);
    if (client || server0 || server1) {
        ferr = msg_mon_process_startup(!client); // system messages
        TEST_CHK_FEOK(ferr);
    }

    if (client) {
        for (linx = 0; linx < loop; linx++) {
            for (tinx = 0; tinx < 3; tinx++) {
                if (!test[tinx])
                    continue;
                for (arg = 0; arg < argc; arg++) {
                    if (strcmp(argv[arg], "-client") == 0) { // start_process
                        sprintf(argvna[arg], "-server%d", tinx);
                        argvn[arg] = argvna[arg];
                    } else
                        argvn[arg] = argv[arg];
                }
                argvn[arg] = NULL;
                if (strace) {
                    sprintf(argvna[0], "xt172ms%d", tinx);
                    argvn[0] = argvna[0];
                }
                if (verbose)
                    printf("%s: about to fork-exec\n", who);
                util_time_timer_start(&t_start);
                pid = fork();
                switch (pid) {
                case 0:
                    // child
                    uerr = execvp(argvn[0], argvn);
                    if (uerr == -1)
                        perror("execvp");
                    exit(0);
                case -1:
                    // parent
                    perror("fork");
                    exit(1);
                default:
                    // parent
                    while (wait(&status) != pid)
                        ;
                    util_time_timer_stop(&t_stop);
                    util_time_elapsed(&t_start, &t_stop, &t_elapsed);
                    sprintf(name, " server%d", tinx);
                    print_elapsed(name, &t_elapsed);
                }
                TEST_CHK_FEOK(ferr);
            }
        }
    }
    if (client || server0 || server1) {
        if (verbose)
            printf("%s: about to msg_mon_process_shutdown\n", who);
        ferr = msg_mon_process_shutdown();
        TEST_CHK_FEOK(ferr);
    }
    if (client) {
        util_test_finish(client);
        util_time_timer_stop(&t_stop);
        util_time_elapsed(&t_start, &t_stop, &t_elapsed);
        print_elapsed(" client", &t_elapsed);
    }
    if (verbose)
        printf("%s: about to exit main\n", who);
    return 0;
}
