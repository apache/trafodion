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
#include "ufsri.h"


int main(int argc, char *argv[]) {
    _xcc_status         cc;
    bool                client = false;
    unsigned short      count_read;
    unsigned short      count_written;
    int                 ferr;
    short               filenum;
    char                my_name[BUFSIZ];
    char                recv_buffer[BUFSIZ];
    RI_Type             ri;
    char                sender[100];
    short               sender_len;
    xzsys_ddl_smsg_def *sys_msg = (xzsys_ddl_smsg_def *) recv_buffer;
    TAD                 zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = file_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    util_gethostname(my_name, (int) sizeof(my_name));

    if (client) {
        ferr = XFILE_OPEN_((char *) "$srv", 4, &filenum,
                           0, 0, 0, 0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        util_abort_core_free();
    } else {
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                           0, 0, 0,
                           1, 0, // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        // pick up open
        cc = XREADUPDATEX(filenum,
                          recv_buffer,
                          BUFSIZ,
                          &count_read,
                          0);
        TEST_CHK_CCNE(cc);
        getri(&ri);
        assert(recv_buffer[0] == XZSYS_VAL_SMSG_OPEN);
        assert(ri.io_type == 0);
        assert(sys_msg->u_z_msg.z_msgnumber[0] == XZSYS_VAL_SMSG_OPEN);
        assert(sys_msg->u_z_msg.z_open.z_accessmode == 0);
        ferr = XPROCESSHANDLE_DECOMPOSE_(&ri.sender,
                                         NULL, // cpu
                                         NULL, // pin
                                         NULL, // nodenumber
                                         NULL, // nodename
                                         0,    // nodename
                                         NULL, // nodename_length
                                         sender,
                                         sizeof(sender),
                                         &sender_len,
                                         NULL); // sequence_number
        TEST_CHK_FEOK(ferr);
        sender[sender_len] = 0;
        assert(strcmp(sender, "$CLI") == 0);
        cc = XREPLYX(recv_buffer,
                     0,
                     &count_written,
                     0,
                     XZFIL_ERR_OK);
        TEST_CHK_CCEQ(cc);
        // pick up close
        cc = XREADUPDATEX(filenum,
                          recv_buffer,
                          BUFSIZ,
                          &count_read,
                          0);
        TEST_CHK_CCNE(cc);
        getri(&ri);
        assert(ri.io_type == 0);
        assert(recv_buffer[0] == XZSYS_VAL_SMSG_CLOSE);
        assert(ri.io_type == 0);
        assert(sys_msg->u_z_msg.z_msgnumber[0] == XZSYS_VAL_SMSG_CLOSE);
        ferr = XPROCESSHANDLE_DECOMPOSE_(&ri.sender,
                                         NULL, // cpu
                                         NULL, // pin
                                         NULL, // nodenumber
                                         NULL, // nodename
                                         0,    // nodename
                                         NULL, // nodename_length
                                         sender,
                                         sizeof(sender),
                                         &sender_len,
                                         NULL); // sequence_number
        TEST_CHK_FEOK(ferr);
        sender[sender_len] = 0;
        assert(strcmp(sender, "$CLI") == 0);
        cc = XREPLYX(recv_buffer,
                     0,
                     &count_written,
                     0,
                     XZFIL_ERR_OK);
        TEST_CHK_CCEQ(cc);
        ferr = XFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(!client);
    return 0;
}
