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

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "tchkfe.h"
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

short ctrl_buffer[100];
char  my_name[BUFSIZ];
char  recv_buffer[40000];
char  send_buffer[40000];


int main(int argc, char *argv[]) {
    bool       client = false;
    int        ferr;
    int        inx;
    int        lerr;
    int        loop = 10;
    int        msgid;
    int        oid;
    RT         results;
    TPT_DECL  (phandle);
    MS_SRE     sre;
    TAD        zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client); // system messages
    TEST_CHK_FEOK(ferr);
    if (client) {
        ferr = msg_mon_open_process((char *) "$srv",      // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
    }
    util_gethostname(my_name, sizeof(my_name));

    for (inx = 0; inx < loop; inx++) {
        if (client) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            if (inx == 0)
                ferr = XMSG_LINK_(TPT_REF(phandle),            // phandle
                                  &msgid,                      // msgid
                                  NULL,                        // reqctrl
                                  0,                           // reqctrlsize
                                  ctrl_buffer,                 // replyctrl
                                  0,                           // replyctrlmax
                                  send_buffer,                 // reqdata
                                  57,                          // reqdatasize
                                  recv_buffer,                 // replydata
                                  256,                         // replydatamax
                                  0,                           // linkertag
                                  0,                           // pri
                                  0,                           // xmitclass
                                  0);                          // linkopts
            else
                ferr = XMSG_LINK_(TPT_REF(phandle),            // phandle
                                  &msgid,                      // msgid
                                  NULL,                        // reqctrl
                                  0,                           // reqctrlsize
                                  ctrl_buffer,                 // replyctrl
                                  100,                         // replyctrlmax
                                  send_buffer,                 // reqdata
                                  39000,                       // reqdatasize
                                  recv_buffer,                 // replydata
                                  8096,                        // replydatamax
                                  0,                           // linkertag
                                  0,                           // pri
                                  0,                           // xmitclass
                                  0);                          // linkopts
            util_check("XMSG_LINK_", ferr);
            ferr = XMSG_BREAK_(msgid, results.u.s, TPT_REF(phandle));
            util_check("XMSG_BREAK_", ferr);
            assert(results.u.t.ctrl_size == 0);
            assert(results.u.t.errm == 0x0); // no-data
        } else {
            do {
                lerr = XWAIT(LREQ, -1);
                TEST_CHK_WAITIGNORE(lerr);
                lerr = XMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
            } while (lerr == XSRETYPE_NOWORK);
            if (inx == 0)
                XMSG_REPLY_(sre.sre_msgId,            // msgid
                            (short *) recv_buffer,    // replyctrl
                            8,                        // replyctrlsize
                            recv_buffer,              // replydata
                            0,                        // replydatasize
                            0,                        // errorclass
                            NULL);                    // newphandle
            else
                XMSG_REPLY_(sre.sre_msgId,            // msgid
                            (short *) recv_buffer,    // replyctrl
                            0,                        // replyctrlsize
                            recv_buffer,              // replydata
                            0,                        // replydatasize
                            0,                        // errorclass
                            NULL);                    // newphandle
        }
    }
    if (client) {
        ferr = msg_mon_close_process(TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
        printf("if there were no asserts, all is well\n");
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
