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

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"


int main(int argc, char *argv[]) {
    void               *buf;
    _xcc_status         cc;
    bool                client = false;
    unsigned short      count_read;
    unsigned short      count_written;
    unsigned short      count_xferred;
    int                 ferr;
    short               filenum_rcv;
    short               filenum_wr;
    int                 inx;
    short               lasterr;
    int                 loop = 10;
    char                my_name[BUFSIZ];
    bool                nowait = false;
    char                recv_buffer[BUFSIZ];
    char                send_buffer[BUFSIZ];
    xzsys_ddl_smsg_def *sys_msg = (xzsys_ddl_smsg_def *) recv_buffer;
    SB_Tag_Type         tag;
    short               tfilenum;
    int                 timeout = -1;
    TAD                 zargs[] = {
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
    ferr = file_mon_process_startup(true); // system messages
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
            cc = XWRITEX(filenum_wr,
                         send_buffer,
                         (short) (strlen(send_buffer) + 1),
                         &count_written,
                         0);
            TEST_CHK_CCEQ(cc);
        }
        ferr = XFILE_CLOSE_(filenum_wr, 0);
        TEST_CHK_FEOK(ferr);
    } else {
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenum_rcv,
                           0, 0, nowait ? (short) 1 : (short) 0,
                           0, 0, // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        // read open message
        cc = XREADX(filenum_rcv,
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
            TEST_CHK_CCNE(cc);
            ferr = XFILE_GETINFO_(-1, &lasterr, NULL, 0, NULL, NULL, NULL);
            TEST_CHK_FEOK(ferr);
            assert(lasterr == XZFIL_ERR_SYSMESS);
        } else {
            TEST_CHK_CCNE(cc);
        }
        assert(sys_msg->u_z_msg.z_msgnumber[0] == XZSYS_VAL_SMSG_OPEN);
        for (inx = 0; inx < loop; inx++) {
            cc = XREADX(filenum_rcv,
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
            printf("%s\n", recv_buffer);
        }
        // read close message
        cc = XREADX(filenum_rcv,
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
            TEST_CHK_CCNE(cc);
            ferr = XFILE_GETINFO_(-1, &lasterr, NULL, 0, NULL, NULL, NULL);
            TEST_CHK_FEOK(ferr);
            assert(lasterr == XZFIL_ERR_SYSMESS);
        } else {
            TEST_CHK_CCNE(cc);
        }
        assert(sys_msg->u_z_msg.z_msgnumber[0] == XZSYS_VAL_SMSG_CLOSE);
        ferr = XFILE_CLOSE_(filenum_rcv, 0);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
