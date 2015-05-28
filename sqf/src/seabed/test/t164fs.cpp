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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "seabed/fs.h"
#include "seabed/ms.h"
#include "seabed/thread.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

enum { MAXSP = 600 };
char   my_name[BUFSIZ];
char   recv_buffer[BUFSIZ];
char   send_buffer[BUFSIZ];

SB_Thread::CV cv[MAXSP];

void cb(SB_Phandle_Type *, struct MS_Mon_NewProcess_Notice_def *msg) {
    int status;

    status = cv[msg->tag].signal();
    assert(status == 0);
}

int main(int argc, char *argv[]) {
    bool            aborto = false;
    int             arg;
    void           *buf;
    _xcc_status     cc;
    bool            client = false;
    int             count;
    unsigned short  count_read;
    unsigned short  count_written;
    unsigned short  count_xferred;
    int             disable = 0;
    int             ferr;
    short           filenumc;
    short           filenumr;
    short           filenums[MAXSP];
    short           filenums_open[MAXSP];
    int             inxl;
    int             inxs;
    short           len;
    int             loop = 10;
    int             maxsp = 1;
    int             nid;
    int             opens;
    int             pid;
    char            prog[MS_MON_MAX_PROCESS_PATH];
    char            retname[BUFSIZ];
    bool            shutdown = false;
    char            sname[MAXSP][10];
    bool            specific = false;
    int             status;
    bool            sysmsg = false;
    SB_Tag_Type     tag;
    short           tfilenum;
    int             timeout;
    bool            verbose = false;
    TAD             zargs[] = {
      { "-aborto",    TA_Bool, TA_NOMAX,    &aborto    }, // abort open
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-maxsp",     TA_Int,  MAXSP,       &maxsp     },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-shutdown",  TA_Bool, TA_NOMAX,    &shutdown  },
      { "-specific",  TA_Bool, TA_NOMAX,    &specific  },
      { "-sysmsg",    TA_Bool, TA_NOMAX,    &sysmsg    },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    if (shutdown)
        sysmsg = true;
    for (arg = 1; arg < argc; arg++) {
        if (strcmp(argv[arg], "-client") == 0) // start_process
            argv[arg] = (char *) "-server";
    }
    util_test_start(client);
    ferr = file_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(my_name, BUFSIZ);
    util_check("msg_mon_get_my_process_name", ferr);

    if (client) {
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenumr,
                           0, 0, 0,
                           1, 1, // no sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inxs = 0; inxs < maxsp; inxs++) {
            sprintf(sname[inxs], "$srv%d", inxs);
            if (verbose)
                printf("client starting server %s\n", sname[inxs]);
            sprintf(prog, "%s/%s", getenv("PWD"), argv[0]);
            nid = -1;
            ferr =
              msg_mon_start_process_nowait_cb(cb,                    // callback
                                              prog,                  // prog
                                              (char *) sname[inxs],  // name
                                              retname,               // ret-name
                                              argc,                  // argc
                                              argv,                  // argv
                                              MS_ProcessType_Generic,// ptype
                                              0,                     // priority
                                              false,                 // debug
                                              false,                 // backup
                                              inxs,                  // tag
                                              &nid,                  // nid
                                              &pid,                  // pid
                                              NULL,                  // infile
                                              NULL);                 // outfile
            TEST_CHK_FEOK(ferr);
        }
        for (inxs = 0; inxs < maxsp; inxs++) {
            if (verbose)
                printf("client waiting for server=%s start\n", sname[inxs]);
            status = cv[inxs].wait(true);
            assert(status == 0);
        }

        if (aborto || shutdown)
            disable = msg_test_assert_disable();
        opens = 0;
        ferr = XCONTROLMESSAGESYSTEM(XCTLMSGSYS_SETSENDLIMIT,
                                     XMAX_SETTABLE_SENDLIMIT);
        assert(ferr == XZFIL_ERR_OK);
        ferr = XCONTROLMESSAGESYSTEM(XCTLMSGSYS_SETRECVLIMIT,
                                     XMAX_SETTABLE_RECVLIMIT);
        assert(ferr == XZFIL_ERR_OK);
        for (inxs = 0; inxs < maxsp; inxs++) {
            len = (short) strlen(sname[inxs]);
            if (verbose)
                printf("client nowait-opening server %s\n", sname[inxs]);
            ferr = XFILE_OPEN_(sname[inxs], len, &filenums[inxs],
                               0, 0, 0, 0,
                               0x4000, // nowait-open
                               0, 0, NULL);
            TEST_CHK_FEOK(ferr);
            if (verbose)
                printf("open filenums[%d]=%d\n", inxs, filenums[inxs]);
            filenums_open[inxs] = -1;
            opens++;
            if (shutdown) {
                if (opens >= FS_MAX_CONCUR_NOWAIT_OPENS) {
                    if (specific)
                        tfilenum = filenums[inxs];
                    else
                        tfilenum = -1;
                    timeout = -1;
                    if (verbose)
                        printf("client reached max nowait-opens - doing awaitiox\n");
                    cc = XAWAITIOX(&tfilenum,
                                   &buf,
                                   &count_xferred,
                                   &tag,
                                   timeout,
                                   NULL);
                    TEST_CHK_CCNE(cc);
                    opens--;
                }
            }
        }
        for (inxs = 0; inxs < maxsp; inxs++) {
            if (shutdown)
                break;
            if (specific)
                tfilenum = filenums[inxs];
            else
                tfilenum = -1;
            timeout = -1;
            cc = XAWAITIOX(&tfilenum,
                           &buf,
                           &count_xferred,
                           &tag,
                           timeout,
                           NULL);
            if (aborto) {
                TEST_CHK_CCNE(cc);
            } else {
                TEST_CHK_CCEQ(cc);
                assert(tag == -30);
                if (specific)
                    assert(tfilenum == filenums[inxs]);
                else {
                    if (verbose)
                        printf("open compl fnum=%d\n", tfilenum);
                    for (inxl = 0; inxl < maxsp; inxl++) {
                        if (tfilenum == filenums[inxl]) {
                            assert(filenums_open[inxl] < 0);
                            filenums_open[inxl] = tfilenum;
                            break;
                        }
                    }
                }
            }
        }
        if (!shutdown) {
            for (inxs = 0; inxs < maxsp; inxs++) {
                ferr = XFILE_CLOSE_(filenums[inxs], 0);
                TEST_CHK_FEOK(ferr);
                len = (short) strlen(sname[inxs]);
                if (verbose)
                    printf("client nowait-opening server %s\n", sname[inxs]);
                for (inxl = 0; inxl < len; inxl++)
                    sname[inxs][inxl] = (char) toupper(sname[inxs][inxl]);
                ferr = XFILE_OPEN_(sname[inxs], len, &filenums[inxs],
                                   0, 0, 0, 0,
                                   0x4000, // nowait-open
                                   0, 0, NULL);
                TEST_CHK_FEOK(ferr);
                tfilenum = -1;
                timeout = -1;
                cc = XAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag,
                               timeout,
                               NULL);
            }
        }

        if (!specific && !aborto && !shutdown) {
            for (inxl = 0; inxl < maxsp; inxl++)
                assert(filenums_open[inxl] == filenums[inxl]);
        }
        for (inxs = 0; inxs < maxsp; inxs++) {
            if (shutdown)
                break;
            if (aborto)
                break;
            for (inxl = 0; inxl < loop; inxl++) {
                sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                        my_name, inxl);
                cc = XWRITEREADX(filenums[inxs],
                                 send_buffer,
                                 (unsigned short) (strlen(send_buffer) + 1), // cast
                                 BUFSIZ,
                                 &count_read,
                                 0);
                TEST_CHK_CCEQ(cc);
                printf("%s\n", send_buffer);
                if (inxl == 0) {
                    cc = XREADUPDATEX(filenumr,
                                      recv_buffer,
                                      0,
                                      &count_read,
                                      0);
                    TEST_CHK_CCEQ(cc);
                    cc = XREPLYX(recv_buffer,
                                 0,
                                 &count_written,
                                 0,
                                 XZFIL_ERR_OK);
                    TEST_CHK_CCEQ(cc);
                }
            }
        }
        if (aborto || shutdown)
            disable = msg_test_assert_disable();
        for (inxs = 0; inxs < maxsp; inxs++) {
            if (shutdown)
                break;
            ferr = XFILE_CLOSE_(filenums[inxs], 0);
            TEST_CHK_FEOK(ferr);
        }

        if (shutdown)
            sleep(1);
        if (verbose)
            printf("client calling shutdown\n");
    } else {
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenumr,
                           0, 0, 0,
                           1, sysmsg ? 0 : 1, // sys msg (if sysmsg)
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        if (sysmsg) { // open-msg
            cc = XREADUPDATEX(filenumr,
                              recv_buffer,
                              BUFSIZ,
                              &count_read,
                              0);
            TEST_CHK_CCNE(cc);
            if (aborto)
                util_abort_core_free();
            if (shutdown) {
                for (;;) {
                    ferr = msg_mon_get_process_info_type(MS_ProcessType_Generic,
                                                         &count,
                                                         0, // max
                                                         NULL); // info
                    assert(ferr == XZFIL_ERR_OK);
                    sleep(1); // sleep at least 1 sec
                    if (count < 3)
                        break;
                    if (maxsp > FS_MAX_CONCUR_NOWAIT_OPENS)
                        break;
                }
            } else {
                count_read = 0;
                cc = XREPLYX(recv_buffer,
                             count_read,
                             &count_written,
                             0,
                             XZFIL_ERR_OK);
                TEST_CHK_CCEQ(cc);
            }
        }
        for (inxl = 0; inxl < loop; inxl++) {
            if (shutdown)
                break;
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
            if (inxl == 0) {
                ferr = XFILE_OPEN_((char *) "$cli", 4, &filenumc,
                                   0, 0, 0, 0,
                                   0,      // waited-open
                                   0, 0, NULL);
                TEST_CHK_FEOK(ferr);
                cc = XWRITEREADX(filenumc,
                                 send_buffer,
                                 0,
                                 0,
                                 &count_read,
                                 0);
                TEST_CHK_CCEQ(cc);
            }
        }
        if (sysmsg && !shutdown) { // close-msg
            cc = XREADUPDATEX(filenumr,
                              recv_buffer,
                              BUFSIZ,
                              &count_read,
                              0);
            TEST_CHK_CCNE(cc);
            count_read = 0;
            cc = XREPLYX(recv_buffer,
                         count_read,
                         &count_written,
                         0,
                         XZFIL_ERR_OK);
            TEST_CHK_CCEQ(cc);
        }
        ferr = XFILE_CLOSE_(filenumr, 0);
        TEST_CHK_FEOK(ferr);
        if (!shutdown) {
            ferr = XFILE_CLOSE_(filenumc, 0);
            TEST_CHK_FEOK(ferr);
        }
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    msg_test_assert_enable(disable);
    util_test_finish(client);
    return 0;
}
