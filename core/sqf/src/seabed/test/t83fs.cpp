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

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"
#include "ufsri.h"


int main(int argc, char *argv[]) {
    void           *buf;
    _xcc_status     cc;
    bool            client = false;
    unsigned short  count_read;
    unsigned short  count_written;
    unsigned short  count_xferred;
    int             ferr;
    short           filenum_rcv;
    short           filenum_wr;
    int             inx;
    int             inxc;
    short           lasterr;
    int             loop = 10;
    bool            mid = false;
    char            my_name[BUFSIZ];
    RI_Type         ri;
    char            recv_buffer[BUFSIZ];
    char            send_buffer[BUFSIZ];
    bool            server = false;
    SB_Tag_Type     tag;
    short           tfilenum;
    int             timeout = -1;
    bool            verbose = false;
    TAD             zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-mid",       TA_Bool, TA_NOMAX,    &mid       },
      { "-server",    TA_Bool, TA_NOMAX,    &server    },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = file_mon_process_startup(true); // system messages
    TEST_CHK_FEOK(ferr);
    util_gethostname(my_name, sizeof(my_name));

    if (client) {
        ferr = XFILE_OPEN_((char *) "$mid", 4, &filenum_wr,
                           0, 0, 1, // nowait
                           0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            if (verbose)
                printf("client sending inx=%d, wc=%d\n", inx, (int) strlen(send_buffer) + 1);
            cc = XWRITEREADX(filenum_wr,
                             send_buffer,
                             (short) (strlen(send_buffer) + 1),
                             BUFSIZ,
                             &count_read,
                             0);
            TEST_CHK_CCEQ(cc);
            tfilenum = -1;
            cc = XAWAITIOX(&tfilenum,
                           &buf,
                           &count_xferred,
                           &tag,
                           timeout,
                           NULL);
            TEST_CHK_CCEQ(cc);
            if (verbose)
                printf("client receiving inx=%d, cc=%d\n", inx, count_xferred);
            printf("%s\n", send_buffer);
        }
        ferr = XFILE_CLOSE_(filenum_wr, 0);
        TEST_CHK_FEOK(ferr);
    } else if (mid) {
        ferr = XFILE_OPEN_((char *) "$srv", 4, &filenum_wr,
                           0, 0, 1, // nowait
                           0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenum_rcv,
                           0, 0, 1, // nowait
                           1, 0, // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        // read open message
        cc = XREADUPDATEX(filenum_rcv,
                          recv_buffer,
                          BUFSIZ,
                          &count_read,
                          0);
        TEST_CHK_CCEQ(cc);
        tfilenum = -1;
        cc = XAWAITIOX(&tfilenum,
                       &buf,
                       &count_xferred,
                       &tag,
                       timeout,
                       NULL);
        if (_xstatus_ne(cc)) {
            ferr = XFILE_GETINFO_(-1, &lasterr, NULL, 0, NULL, NULL, NULL);
            TEST_CHK_FEOK(ferr);
            assert(lasterr == XZFIL_ERR_SYSMESS);
        }
        cc = XREPLYX(recv_buffer,
                     0,
                     &count_written,
                     0,
                     XZFIL_ERR_OK);
        TEST_CHK_CCEQ(cc);
        for (inx = 0; inx < loop; inx++) {
            cc = XREADUPDATEX(filenum_rcv,
                              recv_buffer,
                              BUFSIZ,
                              &count_read,
                              0);
            TEST_CHK_CCEQ(cc);
            for (inxc = 0; inxc < 2; inxc++) {
                tfilenum = -1;
                cc = XAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag,
                               timeout,
                               NULL);
                TEST_CHK_CCEQ(cc);
                if (tfilenum == filenum_rcv) {
                    getri(&ri);
                    if (verbose)
                        printf(" mid received inx=%d, rc=%d\n", inx, count_xferred);
                    cc = XWRITEREADX(filenum_wr,
                                     recv_buffer,
                                     count_xferred,
                                     (short) ri.max_reply_count,
                                     &count_read,
                                     0);
                    TEST_CHK_CCEQ(cc);
                } else {
                    if (verbose)
                        printf(" mid sending inx=%d, wc=%d\n", inx, count_xferred);
                    cc = XREPLYX(recv_buffer,
                                 count_xferred,
                                 &count_written,
                                 0,
                                 XZFIL_ERR_OK);
                    TEST_CHK_CCEQ(cc);
                }
            }
        }
        ferr = XFILE_CLOSE_(filenum_rcv, 0);
        TEST_CHK_FEOK(ferr);
        ferr = file_mon_process_close();
        TEST_CHK_FEOK(ferr);
    } else {
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenum_rcv,
                           0, 0, 1, // nowait
                           1, 0,    // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        // read open message
        cc = XREADUPDATEX(filenum_rcv,
                          recv_buffer,
                          BUFSIZ,
                          &count_read,
                          0);
        TEST_CHK_CCEQ(cc);
        tfilenum = -1;
        cc = XAWAITIOX(&tfilenum,
                       &buf,
                       &count_xferred,
                       &tag,
                       timeout,
                       NULL);
        if (_xstatus_ne(cc)) {
            ferr = XFILE_GETINFO_(-1, &lasterr, NULL, 0, NULL, NULL, NULL);
            TEST_CHK_FEOK(ferr);
            assert(lasterr == XZFIL_ERR_SYSMESS);
        }
        cc = XREPLYX(recv_buffer,
                     count_xferred,
                     &count_written,
                     0,
                     XZFIL_ERR_OK);
        TEST_CHK_CCEQ(cc);
        for (inx = 0; inx < loop; inx++) {
            sprintf(send_buffer, "i'm the server inx=%d", inx);
            cc = XREADUPDATEX(filenum_rcv,
                              recv_buffer,
                              BUFSIZ,
                              &count_read,
                              0);
            TEST_CHK_CCEQ(cc);
            tfilenum = -1;
            cc = XAWAITIOX(&tfilenum,
                           &buf,
                           &count_xferred,
                           &tag,
                           timeout,
                           NULL);
            TEST_CHK_CCEQ(cc);
            strcat(recv_buffer, "- reply from ");
            strcat(recv_buffer, my_name);
            if (verbose)
                printf("  server received inx=%d, rc=%d\n", inx, count_xferred);
            count_read = (short) (strlen(recv_buffer) + 1);
            if (verbose)
                printf("  server sending inx=%d, wc=%d\n", inx, count_read);
            cc = XREPLYX(recv_buffer,
                         count_read,
                         &count_written,
                         0,
                         XZFIL_ERR_OK);
            TEST_CHK_CCEQ(cc);
        }
        ferr = XFILE_CLOSE_(filenum_rcv, 0);
        TEST_CHK_FEOK(ferr);
        ferr = file_mon_process_close();
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
