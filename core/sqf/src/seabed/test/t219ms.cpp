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

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "tchkfe.h"
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

char  my_name[BUFSIZ];
char  recv_buffer[40000];
short recv_ctrl[4];
char  send_buffer[40000];
short send_ctrl[4];


int main(int argc, char *argv[]) {
    bool       client = false;
    bool       disable = false;
    int        ferr;
    int        lerr;
    int        msgid;
    int        oid;
    TPT_DECL  (phandle);
    RT         results;
    MS_SRE     sre;
    TAD        zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    if (client) {
        ferr = msg_mon_open_process((char *) "$srv",      // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
    }
    util_gethostname(my_name, sizeof(my_name));

    if (client) {
        sprintf(send_buffer, "hello, greetings from %s", my_name);
        ferr = XMSG_LINK2_(TPT_REF(phandle),            // phandle
                           &msgid,                      // msgid
                           send_ctrl,                   // reqctrl
                           sizeof(send_ctrl),           // reqctrlsize
                           recv_ctrl,                   // replyctrl
                           sizeof(recv_ctrl),           // replyctrlmax
                           send_buffer,                 // reqdata
                           39000,                       // reqdatasize
                           recv_buffer,                 // replydata
                           40000,                       // replydatamax
                           0,                           // linkertag
                           0,                           // pri
                           0,                           // xmitclass
                           0);                          // linkopts
        util_check("XMSG_LINK_", ferr);
        disable = msg_test_assert_disable();
        XMSG_BREAK2_(msgid, results.u.s, TPT_REF(phandle));
        msg_test_assert_enable(disable);
        assert(results.u.t.ctrl_size == sizeof(recv_ctrl));
        assert(results.u.t.data_size == 0);
        assert(results.u.t.errm == (RT_MS_ERR | RT_ERROR)); // error
        assert(recv_ctrl[0] == 1); // dialect_type
        assert(recv_ctrl[1] == 3); // reply_type
        assert(recv_ctrl[2] == 1); // reply_version
        assert(recv_ctrl[3] == XZFIL_ERR_PATHDOWN); // error
    } else {
        do {
            lerr = XWAIT(LREQ, -1);
            TEST_CHK_WAITIGNORE(lerr);
            lerr = XMSG_LISTEN_((short *) &sre, // sre
                                0,              // listenopts
                                0);             // listenertag
        } while (lerr == XSRETYPE_NOWORK);
        util_abort_core_free();
    }
    if (client) {
        ferr = msg_mon_close_process(TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
