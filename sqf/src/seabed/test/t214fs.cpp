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
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

enum { MAX_CLI = 100 };
enum { MAX_SRV = 100 };
char   my_name[BUFSIZ];
char   recv_buffer[BUFSIZ];
char   send_buffer[BUFSIZ];


int main(int argc, char *argv[]) {
    _xcc_status         cc;
    bool                client = false;
    int                 closes;
    int                 count_read;
    int                 count_written;
    int                 ferr;
    short               filenumr;
    short               filenums[MAX_SRV];
    int                 inxl;
    int                 inxs;
    int                 loop = 10;
    int                 maxcp = 1;
    int                 maxsp = 1;
    bool                mq = false;
    char                serv[20];
    xzsys_ddl_smsg_def *sys_msg = (xzsys_ddl_smsg_def *) recv_buffer;
    bool                verbose = false;
    TAD                 zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-maxcp",     TA_Int,  MAX_CLI,     &maxcp     },
      { "-maxsp",     TA_Int,  MAX_SRV,     &maxsp     },
      { "-mq",        TA_Bool, TA_NOMAX,    &mq        },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = file_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(my_name, BUFSIZ);
    util_check("msg_mon_get_my_process_name", ferr);

    if (client) {
        for (inxl = 0; inxl < loop; inxl++) {
            for (inxs = 0; inxs < maxsp; inxs++) {
                sprintf(serv, "$srv%d", inxs);
                ferr = BFILE_OPEN_(serv, (short) strlen(serv), &filenums[inxs],
                                   0, 0, 0, 0, 0, 0, 0, NULL);
                TEST_CHK_FEOK(ferr);
            }
            for (inxs = 0; inxs < maxsp; inxs++) {
                sprintf(send_buffer, "hello, greetings %d from %s, inx=%d",
                        inxs, my_name, inxl);
                cc = BWRITEREADX(filenums[inxs],
                                 send_buffer,
                                 (int) (strlen(send_buffer) + 1), // cast
                                 BUFSIZ,
                                 &count_read,
                                 0);
                TEST_CHK_CCEQ(cc);
                if (mq) {
                    if ((inxl % 100) == 0)
                        printf("count=%d\n", inxl);
                } else
                    printf("%s\n", send_buffer);
            }
            for (inxs = 0; inxs < maxsp; inxs++) {
                ferr = BFILE_CLOSE_(filenums[inxs], 0);
                TEST_CHK_FEOK(ferr);
            }
        }
    } else {
        closes = 0;
        ferr = BFILE_OPEN_((char *) "$RECEIVE", 8, &filenumr,
                           0, 0, 0,
                           1, 0, // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);

        // process requests
        for (inxl = 0;; inxl++) {
            cc = BREADUPDATEX(filenumr,
                              recv_buffer,
                              BUFSIZ,
                              &count_read,
                              0);
            if (_xstatus_ne(cc)) {
                int mt = sys_msg->u_z_msg.z_msgnumber[0];
                if (verbose) {
                    if (mt == XZSYS_VAL_SMSG_CLOSE)
                        printf("inx=%d, type=%d, cc=%d\n", inxl, mt, closes);
                    else
                        printf("inx=%d, type=%d\n", inxl, mt);
                }
                if (mt == XZSYS_VAL_SMSG_CLOSE)
                    closes++;
                count_read = 0;
            } else {
                strcat(recv_buffer, "- reply from ");
                strcat(recv_buffer, my_name);
                count_read = (int) (strlen(recv_buffer) + 1); // cast
            }
            cc = BREPLYX(recv_buffer,
                         count_read,
                         &count_written,
                         0,
                         XZFIL_ERR_OK);
            TEST_CHK_CCEQ(cc);
            if (closes >= maxcp * loop)
                break;
        }
        ferr = BFILE_CLOSE_(filenumr, 0);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
