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

enum { MAX_DBUF = 1000  };
enum { MAX_OUT  = 2 };

enum { MAX_CLIENTS = 500 };
enum { MAX_SERVERS = 500 };

bool         client = false;
char         my_name[BUFSIZ];
char         recv_buffer[MAX_DBUF];
RI_Type      ri[MAX_OUT];
char         send_buffer[MAX_DBUF];
char         serv[BUFSIZ];


int main(int argc, char *argv[]) {
    bool            abortx = false;
    _bcc_status     bcc;
    void           *buf;
    int             count_read;
    int             count_written;
    int             count_xferred;
    int             disable = 0;
    int             ferr;
    short           filenum[MAX_SERVERS];
    short           filenumr;
    int             inx;
    short           lasterr;
    int             loop = 10;
    int             maxcp = 1;
    int             maxsp = 1;
    int             out;
    int             pinx;
    File_AS_Type    state;
    SB_Tag_Type     tag;
    int             timeout = -1;
    short           tfilenum;
    bool            tol = false;
    bool            verbose = false;
    TAD             zargs[] = {
      { "-abort",     TA_Bool, TA_NOMAX,    &abortx    },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-maxcp",     TA_Int,  TA_NOMAX,    &maxcp     },
      { "-maxsp",     TA_Int,  MAX_SERVERS, &maxsp     },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-tol",       TA_Bool, TA_NOMAX,    &tol       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    if (abortx)
        tol = true;
    maxcp = 1;
    if (maxsp < 0)
        maxsp = 1;
    ferr = file_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(my_name, BUFSIZ);
    TEST_CHK_FEOK(ferr);

    // process-wait for clients/servers/shell
    ferr = msfs_util_wait_process_count(MS_ProcessType_Generic,
                                        maxcp + maxsp + 1,
                                        NULL,
                                        verbose);
    TEST_CHK_FEOK(ferr);
    if (client) {
        inx = atoi(&my_name[4]);
        printf("loop=%d\n", loop);
        for (pinx = 0; pinx < maxsp; pinx++) {
            sprintf(serv, "$srv%d", pinx);
            ferr = BFILE_OPEN_(serv, (short) strlen(serv), &filenum[pinx],
                               0, 0, (short) 2,
                               0, 0, 0, 0, NULL);
            TEST_CHK_FEOK(ferr);
            if (verbose)
                printf("%s-open-count=%d\n", my_name, pinx + 1);
        }
        if (tol) {
            file_test_assert_enable(&state);
            disable = msg_test_assert_disable();
        }
        for (inx = 0; inx < loop; inx++) {
            for (pinx = 0; pinx < maxsp; pinx++) {
                for (out = 0; out < MAX_OUT; out++) {
                    if (filenum[pinx] < 0)
                        continue;
                    if (verbose)
                        printf("%s-WR[%d]=%d\n", my_name, pinx, filenum[pinx]);
                    bcc = BWRITEREADX(filenum[pinx],
                                      send_buffer,
                                      pinx + 1,
                                      BUFSIZ,
                                      &count_read,
                                      out);
                    if (!tol) {
                        TEST_CHK_BCCEQ(bcc);
                    }
                }
            }
            for (pinx = 0; pinx < maxsp; pinx++) {
                for (out = 0; out < MAX_OUT; out++) {
                    if (filenum[pinx] < 0)
                        continue;
                    if (verbose)
                        printf("%s-AIO[%d]=%d\n", my_name, pinx, filenum[pinx]);
                    tfilenum = -2;
                    if ((abortx) && (out == 1))
                        sleep(1);
                    bcc = BAWAITIOX(&tfilenum,
                                    &buf,
                                    &count_xferred,
                                    &tag,
                                    timeout,
                                    NULL);
                    if (tol) {
                        if (_bstatus_ne(bcc)) {
                            ferr = XFILE_GETINFO_(tfilenum, &lasterr, NULL, 0, NULL, NULL, NULL);
                            assert(lasterr == XZFIL_ERR_PATHDOWN);
                            ferr = BFILE_CLOSE_(filenum[pinx], 0);
                            TEST_CHK_FEOK(ferr);
                            filenum[pinx] = -1;
                        }
                    } else {
                        TEST_CHK_BCCEQ(bcc);
                    }
                }
            }
            for (;;) {
                tfilenum = -2;
                timeout = 1;
                bcc = BAWAITIOX(&tfilenum,
                                &buf,
                                &count_xferred,
                                &tag,
                                timeout,
                                NULL);
                if (tfilenum == -2)
                    tfilenum = -1;
                ferr = XFILE_GETINFO_(tfilenum, &lasterr, NULL, 0, NULL, NULL, NULL);
                assert(ferr == XZFIL_ERR_OK);
                if (lasterr == XZFIL_ERR_TIMEDOUT)
                    break;
                assert(lasterr == XZFIL_ERR_PATHDOWN);
            }
        }
        if (tol) {
            file_test_assert_enable(&state);
            msg_test_assert_enable(disable);
        }
        for (pinx = 0; pinx < maxsp; pinx++) {
            if (filenum[pinx] >= 0) {
                ferr = BFILE_CLOSE_(filenum[pinx], 0);
                TEST_CHK_FEOK(ferr);
            }
            if (verbose)
                printf("%s-close-count=%d\n", my_name, inx);
        }
    } else {
        ferr = BFILE_OPEN_((char *) "$RECEIVE", 8, &filenumr,
                           0, 0, (short) 0, // wait
                           MAX_OUT, 0, // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        bcc = BREADUPDATEX(filenumr,
                           recv_buffer,
                           (int) 10,    // cast
                           &count_read,
                           0);
        TEST_CHK_BCCNE(bcc);
        bcc = BREPLYX(recv_buffer,
                      0,
                      &count_written,
                      0,
                      XZFIL_ERR_OK);
        TEST_CHK_BCCEQ(bcc);
        for (inx = 0; inx < loop; inx++) {
            for (out = 0; out < MAX_OUT; out++) {
                bcc = BREADUPDATEX(filenumr,
                                   recv_buffer,
                                   (int) 10,    // cast
                                   &count_read,
                                   0);
                TEST_CHK_BCCEQ(bcc);
                getri(&ri[out]);
            }
            if (abortx)
                util_abort_core_free();
            for (out = 0; out < MAX_OUT; out++) {
                bcc = BREPLYX(recv_buffer,
                              0,
                              &count_written,
                              ri[out].message_tag,
                              XZFIL_ERR_OK);
                TEST_CHK_BCCEQ(bcc);
                if (verbose)
                    printf("%s-count=%d/%d\n", my_name, inx, out);
            }
        }
        bcc = BREADUPDATEX(filenumr,
                           recv_buffer,
                           (int) 10,    // cast
                           &count_read,
                           0);
        TEST_CHK_BCCNE(bcc);
        bcc = BREPLYX(recv_buffer,
                      0,
                      &count_written,
                      0,
                      XZFIL_ERR_OK);
        TEST_CHK_BCCEQ(bcc);
    }

    if (!client) {
        ferr = BFILE_CLOSE_(filenumr, 0);
        TEST_CHK_FEOK(ferr);
    }

    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
