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
#include <string.h>
#include <unistd.h>

#include "seabed/fserr.h"
#include "seabed/fs.h"
#include "seabed/ms.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"

static int exp_msg_types[] = {
    XZSYS_VAL_SMSG_CPUDOWN,
    XZSYS_VAL_SMSG_CPUUP,
    XZSYS_VAL_SMSG_SHUTDOWN
};
enum { MAX_MSGS = sizeof(exp_msg_types)/sizeof(int) };

int main(int argc, char *argv[]) {
    _xcc_status          cc;
    int                  ferr;
    short                filenum;
    char                 recv_buffer[BUFSIZ];
    xzsys_ddl_smsg_def  *sys_msg = (xzsys_ddl_smsg_def *) recv_buffer;

    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    util_test_start(true);
    ferr = file_mon_process_startup(true); // system messages
    TEST_CHK_FEOK(ferr);
    ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                       0, 0, 0,
                       1, 0, // sys msg
                       0, 0, NULL);
    TEST_CHK_FEOK(ferr);
    sleep(2);

    for (int msg = 0; msg < MAX_MSGS; msg++) {
        cc = XREADUPDATEX(filenum,
                          recv_buffer,  // buffer
                          BUFSIZ,       // read_count
                          0,            // count_read
                          0);           // tag
        assert(_xstatus_gt(cc));
        int mt = sys_msg->u_z_msg.z_msgnumber[0];
        printf("msg[%d].type=%d\n", msg, mt);
        assert(mt == exp_msg_types[msg]);
        if (mt == XZSYS_VAL_SMSG_SHUTDOWN) {
            int sl = sys_msg->u_z_msg.z_shutdown.z_shutdownlevel;
            assert(sl == MS_Mon_ShutdownLevel_Normal);
        }
        cc = XREPLYX(NULL,  // buffer
                     0,     // write_count
                     NULL,  // count_written
                     0,     // reply_num
                     0);    // err_ret
        TEST_CHK_CCEQ(cc);
    }

    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(true);
    printf("if there were no asserts, all is well\n");
    return 0;
}
