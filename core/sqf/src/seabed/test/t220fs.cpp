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
#include "seabed/thread.h"

#include "tchkfe.h"
#include "tchkos.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

enum { MAX_OUT = 16 };

SB_Thread::CV  cv;
short          filenumr;
char           recv_buffer[MAX_OUT][BUFSIZ];
char           send_buffer[MAX_OUT][BUFSIZ];
bool           verbose = false;


void *thread_wait(void *arg) {
    void           *buf;
    _xcc_status     cc;
    unsigned short  count_xferred;
    int             ferr;
    short           lasterr;
    SB_Tag_Type     tag;
    short           tfilenum;
    int             timeout = -1;

    arg = arg; // touch
    tfilenum = filenumr;
    if (verbose)
        printf("XAWAITIOX\n");
    cv.signal(true);
    tag = 39;
    cc = XAWAITIOX(&tfilenum,
                   &buf,
                   &count_xferred,
                   &tag,
                   timeout,
                   NULL);
    TEST_CHK_CCNE(cc);
    ferr = XFILE_GETINFO_(tfilenum, &lasterr, NULL, 0, NULL, NULL, NULL);
    TEST_CHK_FEIGNORE(ferr);
    assert(lasterr == XZFIL_ERR_NONEOUT);
    assert(tag == 39);
    return NULL;
}

void *thread_wait_fun(void *arg) {
    return thread_wait(arg);
}

void test_cancel() {
    _xcc_status        cc;
    unsigned short     count_read;
    int                ferr;
    short              lasterr;
    int                out;
    void              *result;
    int                status;
    SB_Thread::Thread *thr_wait;

    //
    // check cancel/cancelreq
    //
    if (verbose)
        printf("testing XCANCEL\n");
    cc = XCANCEL(filenumr);
    TEST_CHK_CCNE(cc);
    ferr = XFILE_GETINFO_(filenumr, &lasterr, NULL, 0, NULL, NULL, NULL);
    TEST_CHK_FEIGNORE(ferr);
    assert(lasterr == XZFIL_ERR_NONEOUT);
    for (out = 0; out < MAX_OUT; out++) {
        cc = XREADUPDATEX(filenumr,
                          recv_buffer[out],
                          BUFSIZ,
                          &count_read,
                          out + 1);
        TEST_CHK_CCEQ(cc);
        cc = XCANCEL(filenumr);
        TEST_CHK_CCEQ(cc);
    }
    cc = XCANCEL(filenumr);
    TEST_CHK_CCNE(cc);
    ferr = XFILE_GETINFO_(filenumr, &lasterr, NULL, 0, NULL, NULL, NULL);
    TEST_CHK_FEIGNORE(ferr);
    assert(lasterr == XZFIL_ERR_NONEOUT);
    if (verbose)
        printf("testing XCANCELREQ\n");
    for (out = 0; out < MAX_OUT; out++) {
        cc = XREADUPDATEX(filenumr,
                          recv_buffer[out],
                          BUFSIZ,
                          &count_read,
                          out + 1);
        TEST_CHK_CCEQ(cc);
        cc = XCANCELREQ(filenumr, out + 1);
        TEST_CHK_CCEQ(cc);
    }
    cc = XCANCEL(filenumr);
    TEST_CHK_CCNE(cc);
    ferr = XFILE_GETINFO_(filenumr, &lasterr, NULL, 0, NULL, NULL, NULL);
    TEST_CHK_FEIGNORE(ferr);
    assert(lasterr == XZFIL_ERR_NONEOUT);

    //
    // start a wait and then cancel
    //
    if (verbose)
        printf("testing CANCEL to XAWAITIOX\n");
    if (verbose)
        printf("XREADUPDATEX\n");
    cc = XREADUPDATEX(filenumr,
                      recv_buffer[0],
                      BUFSIZ,
                      &count_read,
                      1);
    TEST_CHK_CCEQ(cc);
    thr_wait = new SB_Thread::Thread(thread_wait_fun, "wait");
    thr_wait->start();
    status = cv.wait(true);
    TEST_CHK_STATUSOK(status);
    usleep(1000); // make sure XAWAITIO called
    if (verbose)
        printf("XCANCEL\n");
    cc = XCANCEL(filenumr);
    TEST_CHK_CCEQ(cc);
    status = thr_wait->join(&result);
    TEST_CHK_STATUSOK(status);
    delete thr_wait;
}

int main(int argc, char *argv[]) {
    void              *buf;
    _xcc_status        cc;
    bool               client = false;
    unsigned short     count_read;
    unsigned short     count_written;
    unsigned short     count_xferred;
    int                ferr;
    short              filenum;
    int                inx;
    int                loop = 10;
    int                num;
    int                out;
    SB_Tag_Type        tag;
    short              tfilenum;
    int                timeout = -1;
    int                xinx;
    SB_Tag_Type        xtag;
    TAD                zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = file_mon_process_startup(!client); // system messages
    TEST_CHK_FEOK(ferr);

    if (client) {
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenumr,
                           0, 0, 1,
                           MAX_OUT, 1, // no sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        ferr = XFILE_OPEN_((char *) "$srv", 4, &filenum,
                           0, 0, MAX_OUT,
                           0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            test_cancel();
            for (out = 0; out < MAX_OUT; out++) {
                sprintf(send_buffer[out], "inx=%d, tag=%d", inx, out + 1);
                cc = XWRITEREADX(filenum,
                                 send_buffer[out],
                                 0,
                                 0,
                                 &count_read,
                                 out + 1);
                TEST_CHK_CCEQ(cc);
            }
            for (out = 0; out < MAX_OUT; out++) {
                tfilenum = filenum;
                cc = XAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &xtag,
                               timeout,
                               NULL);
                TEST_CHK_CCEQ(cc);
                assert(xtag == (MAX_OUT - out));
                num = sscanf((char *) buf, "inx=%d, tag=" PFTAG, &xinx, &xtag);
                assert(num == 2);
                assert(xinx == inx);
                assert(xtag == (MAX_OUT - out));
            }
        }
        ferr = XFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
        printf("if there were no asserts, all is well\n");
    } else {
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenumr,
                           0, 0, 1,
                           MAX_OUT, 1, // no sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            for (out = 0; out < MAX_OUT; out++) {
                cc = XREADUPDATEX(filenumr,
                                  recv_buffer[out],
                                  BUFSIZ,
                                  &count_read,
                                  out + 1);
                TEST_CHK_CCEQ(cc);
                tfilenum = filenumr;
                cc = XAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag,
                               timeout,
                               NULL);
                TEST_CHK_CCEQ(cc);
            }
            for (out = MAX_OUT - 1; out >= 0; out--) {
                // reply in reverse order
                count_read = (short) (strlen(recv_buffer[out]) + 1);
                cc = XREPLYX(recv_buffer[out],
                             count_read,
                             &count_written,
                             (short) out,
                             XZFIL_ERR_OK);
                TEST_CHK_CCEQ(cc);
            }
        }
        ferr = XFILE_CLOSE_(filenumr, 0);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
