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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "seabed/atomic.h"
#include "seabed/fs.h"
#include "seabed/ms.h"
#include "seabed/thread.h"

#include "tchkfe.h"
#include "tchkos.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

enum { MAX_THREADS = 100 };
SB_Atomic_Int      cli_count;
int                loop = 10;
bool               mq = false;
char               my_name[BUFSIZ];
bool               nowait = false;
bool               quiet = false;
char               recv_buffer[BUFSIZ];
const char        *sname = "$srv";
SB_Thread::Thread *thr_send[MAX_THREADS];
bool               verbose = false;


void *thread_send(void *arg) {
    _bcc_status  bcc;
    void        *buf;
    int          count_read;
    int          count_xferred;
    int          ferr;
    short        filenum;
    int          inx;
    int          lcount;
    char         send_buffer[BUFSIZ];
    SB_Tag_Type  tag;
    short        tfilenum;
    int          timeout = -1;
    char        *thr_name;

    arg = arg; // touch
    thr_name = SB_Thread::Sthr::self_name();
    if (nowait) {
        ferr = BFILE_OPEN_((char *) sname, 4, &filenum,
                           0, 0, 1,
                           0,
                           0x4000, // nowait open
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        tfilenum = filenum;
        bcc = BAWAITIOX(&tfilenum,
                        &buf,
                        &count_xferred,
                        &tag,
                        timeout,
                        NULL);
        TEST_CHK_BCCEQ(bcc);
        assert(tag == -30);
    } else {
        ferr = BFILE_OPEN_((char *) sname, 4, &filenum,
                           0, 0, 0, 0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
    }
    if (verbose)
        printf("client %s, OPENED fnum=%d\n", my_name, filenum);
    for (inx = 0; inx < loop; inx++) {
        cli_count.add_val(1);
        lcount = cli_count.read_val();
        if (verbose)
            printf("client %s, inx=%d, count=%d\n", my_name, inx, lcount);
        sprintf(send_buffer, "hello, greetings from %s, inx=%d, fnum=%d",
                my_name, inx, filenum);
        bcc = BWRITEREADX(filenum,
                          send_buffer,
                          (int) (strlen(send_buffer) + 1), // cast
                          BUFSIZ,
                          &count_read,
                          0);
        TEST_CHK_BCCEQ(bcc);
        if (nowait) {
            tfilenum = filenum;
            bcc = BAWAITIOX(&tfilenum,
                            &buf,
                            &count_xferred,
                            &tag,
                            timeout,
                            NULL);
            TEST_CHK_BCCEQ(bcc);
        }
        if (mq) {
            if ((inx % 100) == 0)
                printf("%s-count=%d\n", thr_name, inx);
        } else if (!quiet)
            printf("%s\n", send_buffer);
    }
    if (verbose)
        printf("client %s, CLOSING fnum=%d\n", my_name, filenum);
    ferr = BFILE_CLOSE_(filenum, 0);
    TEST_CHK_FEOK(ferr);
    return NULL;
}

void *thread_send_fun(void *arg) {
    return thread_send(arg);
}

int main(int argc, char *argv[]) {
    _bcc_status  bcc;
    bool         client = false;
    int          count_read;
    int          count_written;
    int          ferr;
    short        filenum;
    int          inx;
    void        *result;
    int          status;
    char         thread_name[20];
    int          threads = 10;
    TAD          zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-mq",        TA_Bool, TA_NOMAX,    &mq        },
      { "-nowait",    TA_Bool, TA_NOMAX,    &nowait    },
      { "-quiet",     TA_Bool, TA_NOMAX,    &quiet     },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-threads",   TA_Int,  MAX_THREADS, &threads   },
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
        cli_count.set_val(0);
        ferr = BFILE_OPEN_((char *) sname, 4, &filenum,
                           0, 0, 0, 0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < threads; inx++) {
            sprintf(thread_name, "send%d", inx);
            thr_send[inx] = new SB_Thread::Thread(thread_send_fun, thread_name);
        }
        for (inx = 0; inx < threads; inx++)
            thr_send[inx]->start();
        for (inx = 0; inx < threads; inx++) {
            status = thr_send[inx]->join(&result);
            TEST_CHK_STATUSOK(status);
        }
        bcc = BWRITEREADX(filenum,
                         NULL,
                         0,
                         0,
                         &count_read,
                         0);
        TEST_CHK_BCCEQ(bcc);
        ferr = BFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    } else {
        ferr = BFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                           0, 0, 0,
                           1, 1, // no sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop * threads + 1; inx++) {
            if (verbose)
                printf("server inx=%d\n", inx);
            bcc = BREADUPDATEX(filenum,
                               recv_buffer,
                               BUFSIZ,
                               &count_read,
                               0);
            TEST_CHK_BCCEQ(bcc);
            strcat(recv_buffer, "- reply from ");
            strcat(recv_buffer, my_name);
            count_read = (unsigned short) (strlen(recv_buffer) + 1); // cast
            bcc = BREPLYX(recv_buffer,
                          count_read,
                          &count_written,
                          0,
                          XZFIL_ERR_OK);
            TEST_CHK_BCCEQ(bcc);
        }
        ferr = BFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
