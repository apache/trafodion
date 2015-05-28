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

#include "seabed/fserr.h"
#include "seabed/fs.h"
#include "seabed/ms.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"

char group[BUFSIZ];
char recv_buffer[BUFSIZ];

int main(int argc, char *argv[]) {
    _xcc_status          cc;
    int                  ferr;
    short                filenum;
    const char          *key;
    int                  nid;
    int                  pid;
    xzsys_ddl_smsg_def  *sys_msg = (xzsys_ddl_smsg_def *) recv_buffer;
    const char          *value;

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
    ferr = msg_mon_get_process_info(NULL, &nid, &pid);
    TEST_CHK_FEOK(ferr);
    sprintf(group, "NODE%d", nid);
    key = "akey";
    value = "avalue";
    ferr = (short) msg_mon_reg_set(MS_Mon_ConfigType_Node,       // type
                                   group,                        // group
                                   (char *) key,                 // key
                                   (char *) value);              // value
    TEST_CHK_FEOK(ferr);
    sleep(1);
    cc = XREADUPDATEX(filenum,
                      recv_buffer,  // buffer
                      BUFSIZ,       // read_count
                      0,            // count_read
                      0);           // tag
    assert(_xstatus_gt(cc));
    int mt = sys_msg->u_z_msg.z_msgnumber[0];
    printf("msg.type=%d\n",  mt);
    assert(mt == XZSYS_VAL_SMSG_CHANGE);
    int gt = sys_msg->u_z_msg.z_change.z_grouptype;
    char *gn = sys_msg->u_z_msg.z_change.z_groupname;
    char *kn = sys_msg->u_z_msg.z_change.z_keyname;
    char *v = sys_msg->u_z_msg.z_change.z_value;
    printf("msg.grouptype=%d\n",  gt);
    printf("msg.groupname=%s\n",  gn);
    printf("msg.keyname=%s\n",  kn);
    printf("msg.value=%s\n",  v);
    assert(gt == MS_Mon_ConfigType_Node);
    assert(strcmp(gn, group) == 0);
    assert(strcmp(kn, "AKEY") == 0);
    assert(strcmp(v, "avalue") == 0);
    cc = XREPLYX(NULL,  // buffer
                 0,     // write_count
                 NULL,  // count_written
                 0,     // reply_num
                 0);    // err_ret
    TEST_CHK_CCEQ(cc);

    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(true);
    printf("if there were no asserts, all is well\n");
    return 0;
}
