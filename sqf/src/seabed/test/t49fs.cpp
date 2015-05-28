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
    short           filenum1;
    short           filenum2;
    int             inx;
    int             loop = 10;
    char            my_name[BUFSIZ];
    char           *p;
    char            recv_buffer[BUFSIZ];
    char            send_buffer1[BUFSIZ];
    char            send_buffer2[BUFSIZ];
    int             sleep1;
    int             sleep2;
    int             sleep_time;
    SB_Tag_Type     tag;
    short           tfilenum;
    int             timeout;
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
        ferr = XFILE_OPEN_((char *) "$srv1", 4, &filenum1,
                           0, 0, 1,
                           0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        ferr = XFILE_OPEN_((char *) "$srv2", 4, &filenum2,
                           0, 0, 1,
                           0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            if (inx & 1) {
                sleep1 = 0;
                sleep2 = 1000;
            } else {
                sleep1 = 1000;
                sleep2 = 0;
            }
            sprintf(send_buffer1, "hello, greetings from %s, inx=%d, sleep=%d",
                    my_name, inx, sleep1);
            sprintf(send_buffer2, "hello, greetings from %s, inx=%d, sleep=%d",
                    my_name, inx, sleep2);
            cc = XWRITEREADX(filenum1,
                             send_buffer1,
                             (short) (strlen(send_buffer1) + 1),
                             BUFSIZ,
                             &count_read,
                             (SB_Tag_Type) send_buffer1);
            TEST_CHK_CCEQ(cc);
            cc = XWRITEREADX(filenum2,
                             send_buffer2,
                             (short) (strlen(send_buffer2) + 1),
                             BUFSIZ,
                             &count_read,
                             (SB_Tag_Type) send_buffer2);
            TEST_CHK_CCEQ(cc);
            tfilenum = filenum1;
            timeout = -1;
            cc = XAWAITIOX(&tfilenum,
                           &buf,
                           &count_xferred,
                           &tag,
                           timeout,
                           NULL);
            TEST_CHK_CCEQ(cc);
            assert(tfilenum == filenum1);
            assert(buf == send_buffer1);
            assert(tag == (SB_Tag_Type) send_buffer1);
            tfilenum = filenum2;
            timeout = -1;
            cc = XAWAITIOX(&tfilenum,
                           &buf,
                           &count_xferred,
                           &tag,
                           timeout,
                           NULL);
            TEST_CHK_CCEQ(cc);
            assert(tfilenum == filenum2);
            assert(buf == send_buffer2);
            assert(tag == (SB_Tag_Type) send_buffer2);
            printf("%s\n", send_buffer1);
            printf("%s\n", send_buffer2);
        }
        ferr = XFILE_CLOSE_(filenum1, 0);
        ferr = XFILE_CLOSE_(filenum2, 0);
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
            p = &recv_buffer[count_read];
            while ((p > recv_buffer) && (*p != '='))
                p--;
            assert(*p == '=');
            p++;
            sscanf(p, "%d", &sleep_time);
            SB_Thread::Sthr::sleep(sleep_time); // sleep
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
