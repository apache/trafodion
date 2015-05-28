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


int main(int argc, char *argv[]) {
    _xcc_status     cc;
    bool            client = false;
    unsigned short  count_read;
    unsigned short  count_written;
    unsigned short  count_xferred;
    int             ferr;
    short           filenum;
    short           filenumr;
    short           lasterr;
    int             loop = 10;
    TPT_DECL       (mphandle);
    char           *mphandlec = (char *) &mphandle;
    TPT_DECL_INT   (mphandlei);
    char            my_name[BUFSIZ];
    int             nid;
    int             pid;
    char            recv_buffer[BUFSIZ];
    SB_Tag_Type     tag;
    int             timeout = 0;
    short           tfilenum;
    TAD             zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    util_test_start(client);
    ferr = file_mon_process_startup(true);  // system messages?
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(my_name, BUFSIZ);
    util_check("msg_mon_get_my_process_name", ferr);
    ferr = msg_mon_get_process_info(my_name, &nid, &pid);
    TEST_CHK_FEOK(ferr);

    ferr = XPROCESSHANDLE_GETMINE_(TPT_REF(mphandle));
    util_check("XPROCESSHANDLE_GETMINE_", ferr);
    TPT_COPY_INT(mphandlei, mphandle);
    assert((mphandlei[0] & 0xf0) == 0x20); // named
    assert(strcmp(my_name, (char *) &mphandlec[4]) == 0);
    printf("phandle=%x.%x.%x.%x.%x\n",
           mphandlei[0], mphandlei[1], mphandlei[2], mphandlei[3], mphandlei[4]);

    // process-wait for client/server/shell
    ferr = msfs_util_wait_process_count(MS_ProcessType_Generic, 3, NULL, false);
    TEST_CHK_FEOK(ferr);
    if (client) {
        msfs_util_event_send(1, true);
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenumr,
                           0, 0, 1, // nowait
                           1, 0,    // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        cc = XREADUPDATEX(filenumr,
                          recv_buffer,
                          BUFSIZ,
                          &count_read,
                          0);
        TEST_CHK_CCEQ(cc);
        do {
            tfilenum = -1;
            cc = XAWAITIOX(&tfilenum,
                           NULL,
                           &count_xferred,
                           &tag,
                           timeout,
                           NULL);
            TEST_CHK_CCNE(cc);
            ferr = XFILE_GETINFO_(-1, &lasterr, NULL, 0, NULL, NULL, NULL);
            if (lasterr == XZFIL_ERR_TIMEDOUT)
                sleep(1);
        } while (lasterr == XZFIL_ERR_TIMEDOUT);
        cc = XREPLYX(recv_buffer,
                     count_xferred,
                     &count_written,
                     0,
                     XZFIL_ERR_OK);
        TEST_CHK_CCEQ(cc);
        ferr = XFILE_OPEN_((char *) "$srv", 4, &filenum,
                           0, 0, 0, 0, 0, 0, 0, NULL);
        printf("client OPEN of $srv ferr=%d\n", ferr);
        TEST_CHK_FEOK(ferr);
        ferr = XFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);

        // close
        cc = XREADUPDATEX(filenumr,
                          recv_buffer,
                          BUFSIZ,
                          &count_read,
                          0);
        TEST_CHK_CCEQ(cc);
        do {
            tfilenum = -1;
            cc = XAWAITIOX(&tfilenum,
                           NULL,
                           &count_xferred,
                           &tag,
                           timeout,
                           NULL);
            TEST_CHK_CCNE(cc);
            ferr = XFILE_GETINFO_(-1, &lasterr, NULL, 0, NULL, NULL, NULL);
            if (lasterr == XZFIL_ERR_TIMEDOUT) {
                printf("awaitiox for close timeout - sleeping\n");
                sleep(1);
            }
        } while (lasterr == XZFIL_ERR_TIMEDOUT);
        cc = XREPLYX(recv_buffer,
                     count_xferred,
                     &count_written,
                     0,
                     XZFIL_ERR_OK);
        TEST_CHK_CCEQ(cc);
    } else {
        msfs_util_event_wait(1);
        ferr = XFILE_OPEN_((char *) "$cli", 4, &filenum,
                           0, 0, 0, 0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        ferr = XFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenumr,
                           0, 0, 1, // nowait
                           1, 0,    // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        cc = XREADUPDATEX(filenumr,
                          recv_buffer,
                          BUFSIZ,
                          &count_read,
                          0);
        TEST_CHK_CCEQ(cc);
        timeout = -1;
        tfilenum = -1;
        cc = XAWAITIOX(&tfilenum,
                       NULL,
                       &count_xferred,
                       &tag,
                       timeout,
                       NULL);
        TEST_CHK_CCNE(cc);
        ferr = XFILE_GETINFO_(-1, &lasterr, NULL, 0, NULL, NULL, NULL);
        cc = XREPLYX(recv_buffer,
                     count_xferred,
                     &count_written,
                     0,
                     XZFIL_ERR_OK);
        TEST_CHK_CCEQ(cc);
        ferr = file_mon_process_close();
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
