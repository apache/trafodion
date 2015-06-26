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

enum         { MAX_OUT  = 16 };
unsigned int   seed;

int rnd() {
    int rndret = (int) ((float) MAX_OUT* (float) rand_r(&seed)/(RAND_MAX+1.0));
    return rndret;
}

int main(int argc, char *argv[]) {
    _bcc_status     bcc;
    void           *buf;
    bool            cancel = false;
    bool            client = false;
    int             count_read;
    int             count_written;
    int             count_xferred;
    bool            done;
    int             ferr;
    short           filenum;
    int             inx;
    int             loop = 10;
    int             out;
    int             outinx;
    SB_Tag_Type     out_list[MAX_OUT];
    bool            rand = false;
    char            recv_buffer[BUFSIZ];
    char            send_buffer[MAX_OUT][BUFSIZ];
    SB_Tag_Type     tagi;
    SB_Tag_Type     tago;
    short           tfilenum;
    int             timeout = -1;
    bool            verbose = false;
    bool            verbose2 = false;
    TAD             zargs[] = {
      { "-cancel",    TA_Bool, TA_NOMAX,    &cancel    },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-rand",      TA_Bool, TA_NOMAX,    &rand      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "-vv",        TA_Bool, TA_NOMAX,    &verbose2  },
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
        ferr = XFILE_OPEN_((char *) "$srv", 4, &filenum,
                           0, 0, MAX_OUT,
                           0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        tagi = 1;
        for (inx = 0; inx < loop; inx++) {
            if (verbose)
                printf("client writereading inx=%d\n", inx);
            for (out = 0; out < MAX_OUT; out++) {
                bcc = BWRITEREADX(filenum,
                                  send_buffer[out],
                                  out,
                                  BUFSIZ,
                                  &count_read,
                                  tagi);
                TEST_CHK_BCCEQ(bcc);
                out_list[out] = tagi;
                tagi++;
            }
            usleep(100);
            if (verbose)
                printf("client cancelling inx=%d\n", inx);
            if (rand) {
                for (outinx = 0; outinx < MAX_OUT; outinx++) {
                    for (;;) {
                        out = rnd();
                        if (out_list[out])
                            break;
                    }
                    if (verbose)
                        printf("client cancelling inx=%d, out=%d\n", inx, out);
                    bcc = BCANCELREQ(filenum, out_list[out]);
                    TEST_CHK_BCCEQ(bcc);
                    out_list[out] = 0;
                }
            } else {
                for (out = 0; out < MAX_OUT; out++) {
                    if (verbose)
                        printf("client cancelling inx=%d, out=%d\n", inx, out);
                    if (cancel)
                        bcc = BCANCEL(filenum);
                    else
                        bcc = BCANCELREQ(filenum);
                    TEST_CHK_BCCEQ(bcc);
                }
            }
        }
        if (verbose)
            printf("client writereading stop\n");
        strcpy(send_buffer[0], "STOP");
        bcc = BWRITEREADX(filenum,
                          send_buffer[0],
                          (int) strlen(send_buffer[0]) + 1,
                          BUFSIZ,
                          &count_read,
                          tagi);
        TEST_CHK_BCCEQ(bcc);
        printf("client waiting response inx=%d\n", inx);
        tfilenum = filenum;
        bcc = BAWAITIOX(&tfilenum,
                        &buf,
                        &count_xferred,
                        &tago,
                        timeout,
                        NULL);
        TEST_CHK_BCCEQ(bcc);
        assert(tago == tagi);
        printf("%s\n", send_buffer[0]);
        ferr = XFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
        printf("if there were no asserts, all is well\n");
    } else {
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                           0, 0, 1,
                           1, 1, // no sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        done = false;
        for (inx = 0; !done; inx++) {
            bcc = BREADUPDATEX(filenum,
                               recv_buffer,
                               BUFSIZ,
                               &count_read,
                               1);
            TEST_CHK_BCCEQ(bcc);
            tfilenum = filenum;
            bcc = BAWAITIOX(&tfilenum,
                            &buf,
                            &count_xferred,
                            &tago,
                            timeout,
                            NULL);
            TEST_CHK_BCCEQ(bcc);
            count_read = (short) (strlen(recv_buffer) + 1);
            if (strcmp(recv_buffer, "STOP") == 0)
                done = true;
            if (verbose) {
                if (done)
                    printf("server stop\n");
                else
                    printf("server replying, inx=%d\n", inx);
            }
            bcc = BREPLYX(recv_buffer,
                          count_read,
                          &count_written,
                          0,
                          XZFIL_ERR_OK);
            TEST_CHK_BCCEQ(bcc);
        }
        if (verbose)
            printf("server closing file inx=%d\n", inx);
        ferr = XFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
