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
    bool            done;
    int             ferr;
    short           filenum;
    int             inx;
    int             loop = 10;
    char            recv_buffer[BUFSIZ];
    char            send_buffer[BUFSIZ];
    SB_Tag_Type     tag;
    short           tfilenum;
    int             timeout = -1;
    bool            verbose = false;
    TAD             zargs[] = {
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
        if (verbose)
            printf("client opening server\n");
        ferr = XFILE_OPEN_((char *) "$srv", 4, &filenum,
                           0, 0, 2,
                           0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            if (verbose)
                printf("client wr inx=%d\n", inx);
            sprintf(send_buffer, "inx=%d, tag=1", inx);
            cc = XWRITEREADX(filenum,
                             send_buffer,
                             (short) (strlen(send_buffer) + 1),
                             BUFSIZ,
                             &count_read,
                             1);
            TEST_CHK_CCEQ(cc);
            if (inx == 5) {
                if (verbose)
                    printf("client closing server\n");
                ferr = XFILE_CLOSE_(filenum, 0);
                if (verbose)
                    printf("client reopening server\n");
                ferr = XFILE_OPEN_((char *) "$srv", 4, &filenum,
                                   0, 0, 2,
                                   0, 0, 0, 0, NULL);
                TEST_CHK_FEOK(ferr);
            } else {
                tfilenum = filenum;
                if (verbose)
                    printf("client awaitio inx=%d\n", inx);
                cc = XAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag,
                               timeout,
                               NULL);
                TEST_CHK_CCEQ(cc);
            }
        }
        if (verbose)
            printf("client wr shutdown\n");
        cc = XWRITEREADX(filenum,
                         send_buffer,
                         0,
                         0,
                         &count_read,
                         1);
        TEST_CHK_CCEQ(cc);
        tfilenum = filenum;
        if (verbose)
            printf("client awaitio inx=%d\n", inx);
        cc = XAWAITIOX(&tfilenum,
                       &buf,
                       &count_xferred,
                       &tag,
                       timeout,
                       NULL);
        TEST_CHK_CCEQ(cc);
        if (verbose)
            printf("client closing server\n");
        ferr = XFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
        printf("if there were no asserts, all is well\n");
    } else {
        if (verbose)
            printf("server opening $receive\n");
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                           0, 0, 0,
                           1, 0, // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        done = false;
        for (inx = 0; !done; inx++) {
            if (verbose)
                printf("server ru inx=%d\n", inx);
            cc = XREADUPDATEX(filenum,
                              recv_buffer,
                              BUFSIZ,
                              &count_read,
                              1);
            if (_xstatus_eq(cc)) {
                if (count_read == 0)
                    done = true;
                count_read = (short) (strlen(recv_buffer) + 1);
            } else
                count_read = 0;
            if (verbose)
                printf("server reply inx=%d\n", inx);
            cc = XREPLYX(recv_buffer,
                         count_read,
                         &count_written,
                         0,
                         XZFIL_ERR_OK);
            TEST_CHK_CCEQ(cc);
        }
        if (verbose)
            printf("server closing $receive\n");
        ferr = XFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
