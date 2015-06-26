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

#include "seabed/fs.h"
#include "seabed/ms.h"
#include "seabed/thread.h"

#include "tchkfe.h"
#include "tchkos.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

int         loop = 10;
char        my_name[BUFSIZ];
char        recv_buffer[BUFSIZ];
char        send_buffer[BUFSIZ];
const char *sname = "$srv";

void *thread_cli(void *arg) {
    _xcc_status cc;
    int         count_read;
    int         ferr;
    short       filenum;
    short       filenum2;
    int         inx;

    arg = arg; // touch

    ferr = file_enable_open_cleanup();
    TEST_CHK_FEOK(ferr);
    ferr = BFILE_OPEN_((char *) sname, 4, &filenum,
                       0, 0, 0, 0, 0, 0, 0, NULL);
    TEST_CHK_FEOK(ferr);
    ferr = BFILE_OPEN_((char *) sname, 4, &filenum2,
                       0, 0, 0, 0, 0, 0, 0, NULL);
    TEST_CHK_FEOK(ferr);
    for (inx = 0; inx < loop; inx++) {
        sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                my_name, inx);
        cc = BWRITEREADX(filenum,
                         send_buffer,
                         (int) (strlen(send_buffer) + 1), // cast
                         BUFSIZ,
                         &count_read,
                         0);
        TEST_CHK_CCEQ(cc);
        printf("%s\n", send_buffer);
    }
    ferr = BFILE_CLOSE_(filenum2);
    TEST_CHK_FEOK(ferr);
    return NULL;
}

void *thread_cli_fun(void *arg) {
    return thread_cli(arg);
}


int main(int argc, char *argv[]) {
    _xcc_status         cc;
    bool                client = false;
    int                 count_read;
    int                 count_written;
    int                 ferr;
    short               filenum;
    int                 inx;
    void               *result;
    int                 status;
    SB_Thread::Thread  *thr_cli;
    TAD                 zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-sname",     TA_Str,  TA_NOMAX,    &sname     },
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
        thr_cli = new SB_Thread::Thread(thread_cli_fun, "cli");
        thr_cli->start();
        status = thr_cli->join(&result);
        TEST_CHK_STATUSOK(status);
        delete thr_cli;
    } else {
        ferr = BFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                           0, 0, 0,
                           1, 0, // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < (loop + 4); inx++) {
            cc = BREADUPDATEX(filenum,
                              recv_buffer,
                              BUFSIZ,
                              &count_read,
                              0);
            if (_xstatus_ne(cc)) {
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
        }
        ferr = BFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
