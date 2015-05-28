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
#include "seabed/thread.h"

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
    short           lasterr;
    int             loop = 10;
    char            my_name[BUFSIZ];
    bool            nowait = true;
    char            recv_buffer[BUFSIZ];
    char            send_buffer[BUFSIZ];
    SB_Tag_Type     tag;
    short           tfilenum;
    int             timeout;
    int             timeout_count;
    TAD             zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
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
        timeout_count = 0;
        ferr = XFILE_OPEN_((char *) "$srv", 4, &filenum,
                           0, 0, nowait ? (short) 1 : (short) 0,
                           0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            cc = XWRITEREADX(filenum,
                             send_buffer,
                             (short) (strlen(send_buffer) + 1),
                             BUFSIZ,
                             &count_read,
                             0);
            if (nowait) {
                TEST_CHK_CCEQ(cc);
                tfilenum = filenum;
                timeout = 1;
                cc = XAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag,
                               timeout,
                               NULL);
                if (_xstatus_ne(cc)) {
                    timeout_count++;
                    TEST_CHK_CCNE(cc);
                    ferr = XFILE_GETINFO_(filenum,
                                          &lasterr,
                                          NULL,
                                          0,
                                          NULL,
                                          NULL,
                                          NULL);
                    assert(lasterr == XZFIL_ERR_TIMEDOUT);
                    tfilenum = filenum;
                    timeout = 200;
                    cc = XAWAITIOX(&tfilenum,
                                   &buf,
                                   &count_xferred,
                                   &tag,
                                   timeout,
                                   NULL);
                    TEST_CHK_CCEQ(cc);
                }
            } else {
                TEST_CHK_CCNE(cc);
                ferr = XFILE_GETINFO_(-1, &lasterr, NULL, 0, NULL, NULL, NULL);
                TEST_CHK_FEOK(ferr);
                assert(lasterr == XZFIL_ERR_TIMEDOUT);
            }
            printf("%s\n", send_buffer);
        }
        assert(timeout_count);
        ferr = XFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    } else {
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                           0, 0, 0,
                           1, 1, // no sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            cc = XREADUPDATEX(filenum,
                              recv_buffer,
                              BUFSIZ,
                              &count_read,
                              0);
            SB_Thread::Sthr::sleep(30); // cause timeout
            TEST_CHK_CCEQ(cc);
            strcat(recv_buffer, "- reply from ");
            strcat(recv_buffer, my_name);
            count_read = (short) (strlen(recv_buffer) + 1);
            cc = XREPLYX(recv_buffer,
                         count_read,
                         &count_written,
                         0,
                         XZFIL_ERR_OK);
            TEST_CHK_CCEQ(cc);
        }
        ferr = XFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
