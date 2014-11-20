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

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"


int main(int argc, char *argv[]) {
    void           *buf;
    _xcc_status     cc;
    bool            client = false;
    unsigned short  count_read;
    unsigned short  count_written;
    unsigned short  count_xferred;
    bool            dup = false;
    int             ferr;
    short           filenum[10];
    int             filenums = 1;
    int             finx;
    int             inx;
    int             loop = 10;
    bool            noclose = false;
    bool            nowait = false;
    int             num;
    char            recv_buffer1[BUFSIZ];
    char            recv_buffer2[BUFSIZ];
    char            send_buffer1[BUFSIZ];
    char            send_buffer2[BUFSIZ];
    SB_Tag_Type     tag1;
    SB_Tag_Type     tag2;
    short           tfilenum;
    int             timeout = -1;
    bool            verbose = false;
    bool            wait = false;
    int             xinx;
    SB_Tag_Type     xtag;
    TAD             zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-dup",       TA_Bool, TA_NOMAX,    &dup       },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-noclose",   TA_Bool, TA_NOMAX,    &noclose   },
      { "-nowait",    TA_Bool, TA_NOMAX,    &nowait    },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "-wait",      TA_Bool, TA_NOMAX,    &wait      },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    if (dup)
        filenums = 5;
    util_test_start(client);
    ferr = file_mon_process_startup(!client); // system messages
    TEST_CHK_FEOK(ferr);

    if (client) {
        if (verbose)
            printf("client opening server\n");
        if (nowait) {
            for (finx = 0; finx < filenums; finx++) {
                ferr = XFILE_OPEN_((char *) "$srv", 4, &filenum[finx],
                                   0, 0, 2,
                                   0,
                                   0x4000, // nowait open
                                   0, 0, NULL);
                TEST_CHK_FEOK(ferr);
            }
            for (finx = 0; finx < filenums; finx++) {
                tfilenum = filenum[finx];
                cc = XAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag1,
                               timeout,
                               NULL);
                TEST_CHK_CCEQ(cc);
                assert(tag1 == -30);
            }
        } else {
            for (finx = 0; finx < filenums; finx++) {
                ferr = XFILE_OPEN_((char *) "$srv", 4, &filenum[finx],
                                   0, 0, 2,
                                   0, 0, 0, 0, NULL);
                TEST_CHK_FEOK(ferr);
            }
        }
        for (inx = 0; inx < loop; inx++) {
            for (finx = 0; finx < filenums; finx++) {
                if (verbose)
                    printf("client wr-1 inx=%d, finx=%d\n", inx, finx);
                sprintf(send_buffer1, "inx=%d, tag=1", inx);
                cc = XWRITEREADX(filenum[finx],
                                 send_buffer1,
                                 (short) (strlen(send_buffer1) + 1),
                                 BUFSIZ,
                                 &count_read,
                                 1);
                TEST_CHK_CCEQ(cc);
                sprintf(send_buffer2, "inx=%d, tag=2", inx);
                if (verbose)
                    printf("client wr-2 inx=%d, finx=%d\n", inx, finx);
                cc = XWRITEREADX(filenum[finx],
                                 send_buffer2,
                                 (short) (strlen(send_buffer2) + 1),
                                 BUFSIZ,
                                 &count_read,
                                 2);
                TEST_CHK_CCEQ(cc);
                tfilenum = filenum[finx];
                if (verbose)
                    printf("client awaitio-1 inx=%d, finx=%d\n", inx, finx);
                cc = XAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag1,
                               timeout,
                               NULL);
                TEST_CHK_CCEQ(cc);
                assert(tag1 == 2);
                num = sscanf((char *) buf, "inx=%d, tag=" PFTAG, &xinx, &xtag);
                assert(num == 2);
                assert(xinx == inx);
                assert(xtag == 2);
                tfilenum = filenum[finx];
                if (verbose)
                    printf("client awaitio-2 inx=%d, finx=%d\n", inx, finx);
                cc = XAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag2,
                               timeout,
                               NULL);
                TEST_CHK_CCEQ(cc);
                assert(tag2 == 1);
                num = sscanf((char *) buf, "inx=%d, tag=" PFTAG, &xinx, &xtag);
                assert(num == 2);
                assert(xinx == inx);
                assert(xtag == 1);
            }
        }
        if (!noclose) {
            for (finx = 0; finx < filenums; finx++) {
                if (verbose)
                    printf("client closing server finx=%d\n", finx);
                ferr = XFILE_CLOSE_(filenum[finx], 0);
                TEST_CHK_FEOK(ferr);
            }
        }
        if (wait)
            sleep(5);
        printf("if there were no asserts, all is well\n");
    } else {
        if (verbose)
            printf("server opening $receive\n");
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenum[0],
                           0, 0, 1,
                           2, 0, // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (finx = 0; finx < filenums; finx++) {
            if (verbose)
                printf("server ru-open finx=%d\n", finx);
            cc = XREADUPDATEX(filenum[0],
                              recv_buffer1,
                              BUFSIZ,
                              &count_read,
                              1);
            TEST_CHK_CCEQ(cc);
            tfilenum = filenum[0];
            cc = XAWAITIOX(&tfilenum,
                           &buf,
                           &count_xferred,
                           &tag1,
                           timeout,
                           NULL);
            TEST_CHK_CCNE(cc);
            assert(recv_buffer1[0] == XZSYS_VAL_SMSG_OPEN);
            if (verbose)
                printf("server reply-open finx=%d\n", finx);
            cc = XREPLYX(recv_buffer1,
                         0,
                         &count_written,
                         0,
                         XZFIL_ERR_OK);
            TEST_CHK_CCEQ(cc);
        }
        for (inx = 0; inx < loop; inx++) {
            for (finx = 0; finx < filenums; finx++) {
                if (verbose)
                    printf("server ru-1 inx=%d, finx=%d\n", inx, finx);
                cc = XREADUPDATEX(filenum[0],
                                  recv_buffer1,
                                  BUFSIZ,
                                  &count_read,
                                  1);
                TEST_CHK_CCEQ(cc);
                tfilenum = filenum[0];
                cc = XAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag1,
                               timeout,
                               NULL);
                TEST_CHK_CCEQ(cc);
                if (verbose)
                    printf("server ru-2 inx=%d, finx=%d\n", inx, finx);
                cc = XREADUPDATEX(filenum[0],
                                  recv_buffer2,
                                  BUFSIZ,
                                  &count_read,
                                  2);
                TEST_CHK_CCEQ(cc);
                tfilenum = filenum[0];
                cc = XAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag2,
                               timeout,
                               NULL);
                TEST_CHK_CCEQ(cc);
                // reply in reverse order
                count_read = (short) (strlen(recv_buffer2) + 1);
                if (verbose)
                    printf("server reply-2 inx=%d, finx=%d\n", inx, finx);
                cc = XREPLYX(recv_buffer2,
                             count_read,
                             &count_written,
                             1,
                             XZFIL_ERR_OK);
                TEST_CHK_CCEQ(cc);
                count_read = (short) (strlen(recv_buffer1) + 1);
                if (verbose)
                    printf("server reply-1 inx=%d, finx=%d\n", inx, finx);
                cc = XREPLYX(recv_buffer1,
                             count_read,
                             &count_written,
                             0,
                             XZFIL_ERR_OK);
                TEST_CHK_CCEQ(cc);
            }
        }
        if (!noclose) {
            for (finx = 0; finx < filenums; finx++) {
                if (verbose)
                    printf("server ru-close finx=%d\n", finx);
                cc = XREADUPDATEX(filenum[0],
                                  recv_buffer1,
                                  BUFSIZ,
                                  &count_read,
                                  1);
                TEST_CHK_CCEQ(cc);
                tfilenum = filenum[0];
                cc = XAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag1,
                               timeout,
                               NULL);
                TEST_CHK_CCNE(cc);
                assert(recv_buffer1[0] == XZSYS_VAL_SMSG_CLOSE);
                if (verbose)
                    printf("server reply-close finx=%d\n", finx);
                cc = XREPLYX(recv_buffer1,
                             0,
                             &count_written,
                             0,
                             XZFIL_ERR_OK);
                TEST_CHK_CCEQ(cc);
            }
        }
        if (verbose)
            printf("server closing $receive\n");
        ferr = XFILE_CLOSE_(filenum[0], 0);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
