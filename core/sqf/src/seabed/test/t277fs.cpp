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
#include "seabed/thread.h"

#include "tchkfe.h"
#include "tchkos.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

bool        any1;
bool        any2;
short       filenum;
int         loop = 10;
char        my_name[BUFSIZ];
char        recv_buffer[BUFSIZ];
char        send_buffer[BUFSIZ];
const char *sname = "$srv";
bool        thread;


short get_fn(short filenum) {
    short tfilenum;

    if (any1)
        tfilenum = -1;
    else if (any2)
        tfilenum = -2;
    else
        tfilenum = filenum;
    return tfilenum;
}

void *thread_test_fun(void *arg) {
    void        *buf;
    _bcc_status  cc;
    int          count_read;
    int          count_xferred;
    int          ferr;
    int          inx;
    SB_Tag_Type  tag1;
    short        tfilenum;
    int          timeout = -1;

    arg = arg; // touch
    ferr = BFILE_OPEN_((char *) sname, 4, &filenum,
                       0, 0, 1,  // nowait
                       0,
                       0x4400,   // nowait-open/tsc
                       0, 0, NULL);
    TEST_CHK_FEOK(ferr);
    tfilenum = get_fn(filenum);
    cc = BAWAITIOXTS(&tfilenum,
                     &buf,
                     &count_xferred,
                     &tag1,
                     timeout,
                     NULL);
    TEST_CHK_CCEQ(cc);
    assert(tag1 == -30);

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
        tfilenum = get_fn(filenum);
        cc = BAWAITIOXTS(&tfilenum,
                         &buf,
                         &count_xferred,
                         &tag1,
                         timeout,
                         NULL);
        TEST_CHK_CCEQ(cc);
        printf("%s\n", send_buffer);
    }

    // let thread-exit close
    if (!thread) {
        ferr = BFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    _bcc_status        cc;
    bool               chook = false;
    bool               client = false;
    int                count_read;
    int                count_written;
    int                ferr;
    int                inx;
    void              *result;
    bool               shook = false;
    int                status;
    SB_Thread::Thread *thr_test;
    TAD                zargs[] = {
      { "-any1",      TA_Bool, TA_NOMAX,    &any1      },
      { "-any2",      TA_Bool, TA_NOMAX,    &any2      },
      { "-chook",     TA_Bool, TA_NOMAX,    &chook     },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-shook",     TA_Bool, TA_NOMAX,    &shook     },
      { "-thread",    TA_Bool, TA_NOMAX,    &thread    },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    if (client && chook)
        test_debug_hook("c", "c");
    if (!client && shook)
        test_debug_hook("s", "s");
    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    util_test_start(client);
    ferr = file_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);

    if (client) {
        if (thread) {
            thr_test = new SB_Thread::Thread(thread_test_fun, "thr-test");
            thr_test->start();
            status = thr_test->join(&result);
            TEST_CHK_STATUSOK(status);
            delete thr_test;
        } else {
            thread_test_fun(NULL);
        }
    } else {
        ferr = BFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                           0, 0, 0,
                           1, 1, // no sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            cc = BREADUPDATEX(filenum,
                              recv_buffer,
                              BUFSIZ,
                              &count_read,
                              0);
            TEST_CHK_CCEQ(cc);
            strcat(recv_buffer, "- reply from ");
            strcat(recv_buffer, my_name);
            count_read = (unsigned short) (strlen(recv_buffer) + 1); // cast
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
