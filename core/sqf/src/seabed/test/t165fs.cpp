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

enum  { MAX_DBUF = 1024 };

enum { MAX_SERVERS = 1024 };

bool     client = false;
char     my_name[BUFSIZ];


char recv_buffer[MAX_DBUF];
char send_buffer[MAX_DBUF];
char sender[BUFSIZ];
char serv[BUFSIZ];


int main(int argc, char *argv[]) {
    void           *buf;
    _xcc_status     cc;
    int             count_read;
    int             count_written;
    int             count_xferred;
    int             ferr;
    short           filenum[MAX_SERVERS];
    short           filenumr;
    int             inx;
    int             maxsp = 1;
    bool            nowait = false;
    bool            sys_msg;
    long            t_elapsed;
    long            t_elapsed_sec;
    struct timeval  t_start;
    struct timeval  t_stop;
    SB_Tag_Type     tag;
    short           tfilenum;
    int             timeout;
    bool            verbose = false;
    TAD             zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-maxcp",     TA_Next, TA_NOMAX,    NULL       },
      { "-maxsp",     TA_Int,  MAX_SERVERS, &maxsp     },
      { "-nowait",    TA_Bool, TA_NOMAX,    &nowait    },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    if (maxsp < 0)
        maxsp = 1;
    util_test_start(client);
    ferr = file_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(my_name, BUFSIZ);
    TEST_CHK_FEOK(ferr);

    if (client) {
        util_time_timer_start(&t_start);
        for (inx = 0; inx < maxsp; inx++) {
            sprintf(serv, "$srv%d", inx);
            ferr = BFILE_OPEN_(serv, (short) strlen(serv), &filenum[inx],
                               0, 0, (short) 0,
                               0,
                               nowait ? 0x4000 : 0,
                               0, 0, NULL);
            TEST_CHK_FEOK(ferr);
        }
        if (nowait) {
            for (inx = 0; inx < maxsp; inx++) {
                tfilenum = filenum[inx];
                timeout = -1;
                cc = BAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag,
                               timeout,
                               NULL);
                TEST_CHK_CCEQ(cc);
                assert(tag == -30);
            }
        }
        util_time_timer_stop(&t_stop);
        for (inx = 0; inx < maxsp; inx++) {
            if (inx == 0) {
                if (verbose)
                    printf("%s-count=%d\n", my_name, inx);
            }
            cc = BWRITEREADX(filenum[inx],
                             send_buffer,
                             (int) 0,     // cast
                             0,
                             &count_read,
                             0);
        }
    } else {
        ferr = BFILE_OPEN_((char *) "$RECEIVE", 8, &filenumr,
                           0, 0, (short) 0, // wait
                           1, 0, // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        // pick up open/msg/close/shutdown
        for (inx = 0; inx < 4; inx++) {
            cc = BREADUPDATEX(filenumr,
                              recv_buffer,
                              (int) MAX_DBUF,     // cast
                              &count_read,
                              0);
            sys_msg = _xstatus_ne(cc);
            cc = BREPLYX(recv_buffer,
                         0,
                         &count_written,
                         0,
                         XZFIL_ERR_OK);
            TEST_CHK_CCEQ(cc);
            if (sys_msg && (recv_buffer[0] == XZSYS_VAL_SMSG_CLOSE))
                break;
        }
    }

    if (client) {
        for (inx = 0; inx < maxsp; inx++) {
            ferr = BFILE_CLOSE_(filenum[inx], 0);
            TEST_CHK_FEOK(ferr);
        }
        t_elapsed = (t_stop.tv_sec * 1000000 + t_stop.tv_usec) -
                    (t_start.tv_sec * 1000000 + t_start.tv_usec);
        t_elapsed_sec = t_elapsed / 1000000;
        t_elapsed -= t_elapsed_sec * 1000000;
        printf("elapsed open=%ld.%ld\n", t_elapsed_sec, t_elapsed);
    } else {
        ferr = BFILE_CLOSE_(filenumr, 0);
        TEST_CHK_FEOK(ferr);
    }

    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
