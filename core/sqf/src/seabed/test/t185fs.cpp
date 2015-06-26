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

enum { MAXSRV = 10 };
char   my_name[BUFSIZ];
char   recv_buffer[BUFSIZ];
char   send_buffer[BUFSIZ];


int main(int argc, char *argv[]) {
    _xcc_status         cc;
    bool                client = false;
    unsigned short      count_read;
    unsigned short      count_written;
    int                 ferr;
    short               filenumr;
    short               filenums[MAXSRV];
    int                 inxl;
    int                 inxs;
    int                 loop = 10;
    int                 maxsp = 2;
    bool                verbose = false;
    xzsys_ddl_smsg_def *sys_msg = (xzsys_ddl_smsg_def *) recv_buffer;
    struct timeval      t_elapsed_close;
    struct timeval      t_elapsed_closem1;
    struct timeval      t_elapsed_open;
    struct timeval      t_elapsed_openm1;
    struct timeval      t_start_close;
    struct timeval      t_start_closem1;
    struct timeval      t_start_open;
    struct timeval      t_start_openm1;
    struct timeval      t_stop_close;
    struct timeval      t_stop_closem1;
    struct timeval      t_stop_open;
    struct timeval      t_stop_openm1;
    TAD                 zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-maxsp",     TA_Int,  MAXSRV,      &maxsp     },
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

    if (client) {
        util_time_timer_start(&t_start_open);
        for (inxs = 0; inxs < maxsp; inxs++) {
            ferr = XFILE_OPEN_((char *) "$srv", 4, &filenums[inxs],
                               0, 0, 0, 0, 0, 0, 0, NULL);
            TEST_CHK_FEOK(ferr);
            if (inxs == 0)
                util_time_timer_start(&t_start_openm1);
        }
        util_time_timer_stop(&t_stop_open);
        util_time_timer_stop(&t_stop_openm1);
        for (inxs = 0; inxs < maxsp; inxs++) {
            for (inxl = 0; inxl < loop; inxl++) {
                sprintf(send_buffer, "hello, greetings %d from %s, inx=%d",
                        inxs, my_name, inxl);
                cc = XWRITEREADX(filenums[inxs],
                                 send_buffer,
                                 (unsigned short) (strlen(send_buffer) + 1), // cast
                                 BUFSIZ,
                                 &count_read,
                                 0);
                TEST_CHK_CCEQ(cc);
                printf("%s\n", send_buffer);
            }
        }
        util_time_timer_start(&t_start_close);
        util_time_timer_start(&t_start_closem1);
        for (inxs = 0; inxs < maxsp; inxs++) {
            if (inxs == (maxsp - 1))
                util_time_timer_stop(&t_stop_closem1);
            ferr = XFILE_CLOSE_(filenums[inxs], 0);
            TEST_CHK_FEOK(ferr);
        }
        util_time_timer_stop(&t_stop_close);
        util_time_elapsed(&t_start_open, &t_stop_open, &t_elapsed_open);
        util_time_elapsed(&t_start_openm1, &t_stop_openm1, &t_elapsed_openm1);
        util_time_elapsed(&t_start_close, &t_stop_close, &t_elapsed_close);
        util_time_elapsed(&t_start_closem1, &t_stop_closem1, &t_elapsed_closem1);
        print_elapsed(" (open)", &t_elapsed_open);
        print_elapsed(" (open-1st)", &t_elapsed_openm1);
        print_elapsed(" (close)", &t_elapsed_close);
        print_elapsed(" (close-lst)", &t_elapsed_closem1);
    } else {
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenumr,
                           0, 0, 0,
                           1, 0, // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);

        // process opens
        for (inxs = 0; inxs < maxsp; inxs++) {
            cc = XREADUPDATEX(filenumr,
                              recv_buffer,
                              BUFSIZ,
                              &count_read,
                              0);
            TEST_CHK_CCNE(cc);
            if (verbose) {
                int mt = sys_msg->u_z_msg.z_msgnumber[0];
                printf("inx=%d, type=%d\n", inxs, mt);
            }
            cc = XREPLYX(recv_buffer,
                         0,
                         &count_written,
                         0,
                         XZFIL_ERR_OK);
            TEST_CHK_CCEQ(cc);
        }
        for (inxs = 0; inxs < maxsp; inxs++) {
            for (inxl = 0; inxl < loop; inxl++) {
                cc = XREADUPDATEX(filenumr,
                                  recv_buffer,
                                  BUFSIZ,
                                  &count_read,
                                  0);
                TEST_CHK_CCEQ(cc);
                strcat(recv_buffer, "- reply from ");
                strcat(recv_buffer, my_name);
                count_read = (unsigned short) (strlen(recv_buffer) + 1); // cast
                cc = XREPLYX(recv_buffer,
                             count_read,
                             &count_written,
                             0,
                             XZFIL_ERR_OK);
                TEST_CHK_CCEQ(cc);
            }
        }

        // process closes
        for (inxs = 0; inxs < maxsp; inxs++) {
            cc = XREADUPDATEX(filenumr,
                              recv_buffer,
                              BUFSIZ,
                              &count_read,
                              0);
            TEST_CHK_CCNE(cc);
            if (verbose) {
                int mt = sys_msg->u_z_msg.z_msgnumber[0];
                printf("inx=%d, type=%d\n", inxs, mt);
            }
            cc = XREPLYX(recv_buffer,
                         0,
                         &count_written,
                         0,
                         XZFIL_ERR_OK);
            TEST_CHK_CCEQ(cc);
        }
        ferr = XFILE_CLOSE_(filenumr, 0);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
