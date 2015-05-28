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
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

char my_name[BUFSIZ];
char recv_buffer[40000];
char send_buffer[40000];


int main(int argc, char *argv[]) {
    bool                client = false;
    bool                client1 = false;
    bool                client2 = false;
    bool                client3 = false;
    int                 ferr;
    int                 inx;
    int                 lerr;
    int                 loop = 10;
    int                 msgid;
    int                 oid;
    TPT_DECL           (phandle);
    MS_Result_Type      results;
    MS_SRE              sre;
    MS_Mon_Transid_Type transid;
    TAD                 zargs[] = {
      { "-client1",   TA_Bool, TA_NOMAX,    &client1   },
      { "-client2",   TA_Bool, TA_NOMAX,    &client2   },
      { "-client3",   TA_Bool, TA_NOMAX,    &client3   },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    if (client1)
        client = true;
    if (client2)
        client = true;
    if (client3)
        client = true;
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
        if (client1)
            sleep(5);
        else if (client2) {
            util_abort_core_free();
        } else {
            ferr = XMSG_LINK_(TPT_REF(phandle),            // phandle
                              &msgid,                      // msgid
                              NULL,                        // reqctrl
                              0,                           // reqctrlsize
                              NULL,                        // replyctrl
                              0,                           // replyctrlmax
                              send_buffer,                 // reqdata
                              0,                           // reqdatasize
                              recv_buffer,                 // replydata
                              0,                           // replydatamax
                              0,                           // linkertag
                              0,                           // pri
                              0,                           // xmitclass
                              0);                          // linkopts
            util_check("XMSG_LINK_", ferr);
            ferr = XMSG_BREAK_(msgid, (short *) &results, TPT_REF(phandle));
            util_check("XMSG_BREAK_", ferr);
        }
    } else {
        msg_mon_enable_mon_messages(true);
        for (inx = 0; inx < 7; inx++) {
            do {
                lerr = XWAIT(LREQ, -1);
                TEST_CHK_WAITIGNORE(lerr);
                lerr = XMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
            } while (lerr == XSRETYPE_NOWORK);
            ferr = XMSG_READDATA_(sre.sre_msgId,  // msgid
                                  recv_buffer,    // reqdata
                                  40000);         // bytecount
            util_check("XMSG_READDATA_", ferr);
            if (sre.sre_flags & XSRE_MON) {
                MS_Mon_Msg *msg = (MS_Mon_Msg *) recv_buffer;
                if (msg->type == MS_MsgType_Open) {
                    if (strcasecmp(msg->u.open.target_process_name, "$cli2") == 0) {
                        transid.id[0] = -1;
                        transid.id[1] = -1;
                        transid.id[2] = -1;
                        transid.id[3] = -1;
                        ferr = msg_mon_deregister_death_notification(-1, // msg->u.open.nid,
                                                                     -1, // msg->u.open.pid,
                                                                     transid);
                        TEST_CHK_FEOK(ferr);
                    }
                }
                printf("server reply type=%d, inx=%d\n", msg->type, inx);
            } else
                printf("server reply inx=%d\n", inx);
            XMSG_REPLY_(sre.sre_msgId,  // msgid
                        NULL,           // replyctrl
                        0,              // replyctrlsize
                        recv_buffer,    // replydata
                        0,              // replydatasize
                        0,              // errorclass
                        NULL);          // newphandle
        }
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
