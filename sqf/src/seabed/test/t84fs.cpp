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
#include "ufsri.h"


int main(int argc, char *argv[]) {
    _bcc_status         bcc;
    void               *buf;
    bool                client = false;
    int                 count_read;
    int                 count_written;
    int                 count_xferred;
    int                 disable;
    int                 ferr;
    short               filenum;
    int                 inx;
    int                 loop = 10;
    char                my_name[BUFSIZ];
    int                 nid;
    bool                nowait = false;
    int                 pid;
    RI_Type             ri;
    char                recv_buffer[BUFSIZ];
    char                send_buffer[BUFSIZ];
    int                 srv1;
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
    ferr = file_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(my_name, BUFSIZ);
    util_check("msg_mon_get_my_process_name", ferr);
    ferr = msg_mon_get_process_info(my_name, &nid, &pid);
    TEST_CHK_FEOK(ferr);

    if (client) {
        ferr = BFILE_OPEN_((char *) "$SRV", 4, &filenum,
                           0, 0, 0, 0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            bcc = BWRITEREADX(filenum,
                              send_buffer,
                              (short) strlen(send_buffer) + 1,
                              BUFSIZ,
                              &count_read,
                              0);
            TEST_CHK_BCCEQ(bcc);
            printf("%s\n", send_buffer);
        }
        ferr = BFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    } else {
        srv1 = (strcmp(my_name, "$SRV") == 0);
        ferr = BFILE_OPEN_((char *) "$RECEIVE",
                           8,
                           &filenum,
                           BOMITSHORT,
                           BOMITSHORT,
                           nowait ? (short) 1 : (short) 0,
                           10); // rcv depth
        TEST_CHK_FEOK(ferr);
        // read open/node-down message
        bcc = BREADUPDATEX(filenum,
                           recv_buffer,
                           BUFSIZ,
                           &count_read,
                           0);
        if (nowait) {
            TEST_CHK_BCCEQ(bcc);
            tfilenum = filenum;
            bcc = BAWAITIOX(&tfilenum,
                            &buf,
                            &count_xferred,
                            &tag,
                            timeout,
                            NULL);
        }
        TEST_CHK_BCCNE(bcc);
        if (srv1)
            assert(sys_msg->u_z_msg.z_msgnumber[0] == XZSYS_VAL_SMSG_OPEN);
        else {
            assert(sys_msg->u_z_msg.z_msgnumber[0] == XZSYS_VAL_SMSG_CPUDOWN);
            assert(sys_msg->u_z_msg.z_cpudown.z_cpunumber == 1);
            assert(sys_msg->u_z_msg.z_cpudown.z_takeover == 0);
        }
        getri(&ri);
        bcc = BREPLYX(recv_buffer,
                      0,
                      &count_written,
                      ri.message_tag,
                      XZFIL_ERR_OK);
        TEST_CHK_BCCEQ(bcc);
        for (inx = 0; inx < loop; inx++) {
            if (!srv1)
                break;
            bcc = BREADUPDATEX(filenum,
                               recv_buffer,
                               BUFSIZ,
                               &count_read,
                               0);
            TEST_CHK_BCCEQ(bcc);
            if (nowait) {
                tfilenum = filenum;
                bcc = BAWAITIOX(&tfilenum,
                                &buf,
                                &count_xferred,
                                &tag,
                                timeout,
                                NULL);
                TEST_CHK_BCCEQ(bcc);
            }
            getri(&ri);
            strcat(recv_buffer, "- reply from ");
            strcat(recv_buffer, my_name);
            count_read = (short) strlen(recv_buffer) + 1;
            bcc = BREPLYX(recv_buffer,
                          count_read,
                          &count_written,
                          ri.message_tag,
                          XZFIL_ERR_OK);
            TEST_CHK_BCCEQ(bcc);
        }
        ferr = BFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
        if (srv1) {
            ferr = file_mon_process_close();
            TEST_CHK_FEOK(ferr);
        }
    }
    if (client) {
        disable = msg_test_assert_disable();
        ferr = file_mon_process_shutdown();
        TEST_CHK_FEIGNORE(ferr);
        msg_test_assert_enable(disable);
    } else {
        ferr = file_mon_process_shutdown();
        TEST_CHK_FEOK(ferr);
    }
    util_test_finish(client);
    return 0;
}
