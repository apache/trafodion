//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
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
    int             loop = 10;
    char            my_name[BUFSIZ];
    bool            nowait = false;
    char            recv_buffer[BUFSIZ];
    RI_Type         ri;
    char            send_buffer[BUFSIZ];
    SB_Tag_Type     tag;
    short           tfilenum;
    int             timeout = -1;
    TAD             zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-nowait",    TA_Bool, TA_NOMAX,    &nowait    },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = file_mon_process_startup(!client); // system messages
    TEST_CHK_FEOK(ferr);
    util_gethostname(my_name, sizeof(my_name));

    if (client) {
        ferr = XFILE_OPEN_((char *) "$srv", 4, &filenum_wr,
                           0, 0, 0,
                           0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            cc = XWRITEREADX(filenum_wr,
                             send_buffer,
                             (short) (strlen(send_buffer) + 1),
                             BUFSIZ,
                             &count_read,
                             0);
            TEST_CHK_CCEQ(cc);
            printf("%s\n", send_buffer);
        }
        ferr = XFILE_CLOSE_(filenum_wr, 0);
        TEST_CHK_FEOK(ferr);
    } else {
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenum_rcv,
                           0, 0, nowait ? (short) 1 : (short) 0,
                           1, 0, // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        // read open message
        cc = XREADUPDATEX(filenum_rcv,
                          recv_buffer,
                          BUFSIZ,
                          &count_read,
                          0);
        if (nowait) {
            TEST_CHK_CCEQ(cc);
            tfilenum = filenum_rcv;
            cc = XAWAITIOX(&tfilenum,
                           &buf,
                           &count_xferred,
                           &tag,
                           timeout,
                           NULL);
        }
        TEST_CHK_CCNE(cc);
        getri(&ri);
        cc = XREPLYX(recv_buffer,
                     0,
                     &count_written,
                     ri.message_tag,
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
            if (nowait) {
                tfilenum = filenum_rcv;
                cc = XAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag,
                               timeout,
                               NULL);
                TEST_CHK_CCEQ(cc);
            }
            getri(&ri);
            strcat(recv_buffer, "- reply from ");
            strcat(recv_buffer, my_name);
            count_read = (short) (strlen(recv_buffer) + 1);
            cc = XREPLYX(recv_buffer,
                         count_read,
                         &count_written,
                         ri.message_tag,
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
