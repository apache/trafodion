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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "seabed/fs.h"
#include "seabed/ms.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"
#include "ufsri.h"

enum { MAX_DBUF = 1024 * 1024 };        // 1 MB

enum { MAX_CLIENTS = 500 };
enum { MAX_SERVERS = 500 };

bool     client = false;
int      maxcp = 1;
char     my_name[BUFSIZ];
char     recv_buffer[MAX_DBUF];
char     send_buffer[MAX_DBUF];
char     serv[BUFSIZ];


int main(int argc, char *argv[]) {
    _xcc_status     cc;
    int             count_read;
    int             count_written;
    int             ferr;
    short           filenum[MAX_SERVERS];
    short           filenumr;
    int             inx;
    int             loop = 10;
    int             max;
    int             maxsp = 1;
    int             pinx;
    bool            shutdown = false;
    File_AS_Type    state;
    int             sys_msg;
    bool            verbose = false;
    TAD             zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-maxcp",     TA_Int,  MAX_CLIENTS, &maxcp     },
      { "-maxsp",     TA_Int,  MAX_SERVERS, &maxsp     },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-shutdown",  TA_Bool, TA_NOMAX,    &shutdown  },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    if (maxcp < 0)
        maxcp = 1;
    if (maxsp < 0)
        maxsp = 1;
    ferr = file_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(my_name, BUFSIZ);
    TEST_CHK_FEOK(ferr);

    // process-wait for clients/servers/shell
    ferr = msfs_util_wait_process_count(MS_ProcessType_Generic,
                                        maxcp + maxsp + 1,
                                        NULL,
                                        verbose);
    TEST_CHK_FEOK(ferr);
    sleep(2); // don't exit for a bit - in case other processes are behind
    if (client) {
        inx = atoi(&my_name[4]);
        printf("loop=%d\n", loop);
        for (inx = 0; inx < loop; inx++) {
            for (pinx = 0; pinx < maxsp; pinx++) {
                sprintf(serv, "$srv%d", pinx);
                if (shutdown)
                    file_test_assert_disable(&state);
                ferr = BFILE_OPEN_(serv, (short) strlen(serv), &filenum[pinx],
                                   0, 0, (short) 0,
                                   0, 0, 0, 0, NULL);
                TEST_CHK_FEOK(ferr);
                if (shutdown) {
                    file_test_assert_enable(&state);
                    if (ferr != XZFIL_ERR_OK)
                        filenum[pinx] = -1;
                } else
                    TEST_CHK_FEOK(ferr);
                if (verbose)
                    printf("%s-open-count=%d\n", my_name, inx);
            }
            for (pinx = 0; pinx < maxsp; pinx++) {
                if (filenum[pinx] >= 0) {
                    ferr = BFILE_CLOSE_(filenum[pinx], 0);
                    TEST_CHK_FEOK(ferr);
                }
                if (verbose)
                    printf("%s-close-count=%d\n", my_name, inx);
            }
        }
    } else {
        ferr = BFILE_OPEN_((char *) "$RECEIVE", 8, &filenumr,
                           0, 0, (short) 0, // wait
                           1, 0, // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        max = 2 * maxcp * loop;
        for (inx = 0; inx < max; inx++) {
            cc = BREADUPDATEX(filenumr,
                              recv_buffer,
                              (int) 10,    // cast
                              &count_read,
                              0);
            sys_msg = _xstatus_ne(cc);
            assert(sys_msg);
            cc = BREPLYX(recv_buffer,
                         0,
                         &count_written,
                         0,
                         XZFIL_ERR_OK);
            TEST_CHK_CCEQ(cc);
            if (verbose)
                printf("%s-count=%d\n", my_name, inx);
            if (shutdown) {
                ferr = file_mon_process_shutdown();
                TEST_CHK_FEOK(ferr);
                return 0;
            }
        }
    }

    if (!client) {
        ferr = BFILE_CLOSE_(filenumr, 0);
        TEST_CHK_FEOK(ferr);
    }

    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
