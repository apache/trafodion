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
#include "seabed/ms.h"

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
    int             xinx;
    TAD                 zargs[] = {
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
        ferr = XFILE_OPEN_((char *) "$srv", 4, &filenum,
                           0, 0, 1,
                           0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            sprintf(send_buffer, "inx=%d, tag=%d", inx, inx);
            cc = XWRITEREADX(filenum,
                             send_buffer,
                             (short) (strlen(send_buffer) + 1),
                             BUFSIZ,
                             &count_read,
                             inx);
            TEST_CHK_CCEQ(cc);
            if (inx < (loop - 1)) {
                cc = XCANCELREQ(filenum, inx);
                TEST_CHK_CCEQ(cc);
                if (verbose)
                    printf("client cancelled inx=%d\n", inx);
            } else {
                printf("client waiting response inx=%d\n", inx);
                tfilenum = filenum;
                cc = XAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag,
                               timeout,
                               NULL);
                TEST_CHK_CCEQ(cc);
                assert(tag == inx);
                printf("%s\n", send_buffer);
            }
        }
        ferr = XFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
        printf("if there were no asserts, all is well\n");
    } else {
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                           0, 0, 1,
                           1, 1, // no sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            cc = XREADUPDATEX(filenum,
                              recv_buffer,
                              BUFSIZ,
                              &count_read,
                              1);
            TEST_CHK_CCEQ(cc);
            tfilenum = filenum;
            cc = XAWAITIOX(&tfilenum,
                           &buf,
                           &count_xferred,
                           &tag,
                           timeout,
                           NULL);
            TEST_CHK_CCEQ(cc);
            count_read = (short) (strlen(recv_buffer) + 1);
            sscanf(recv_buffer, "inx=%d", &xinx);
            if (verbose)
                printf("server replying, server inx=%d, client inx=%d, %s\n",
                       inx, xinx, recv_buffer);
            inx = xinx;
            cc = XREPLYX(recv_buffer,
                         count_read,
                         &count_written,
                         0,
                         XZFIL_ERR_OK);
            TEST_CHK_CCEQ(cc);
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
