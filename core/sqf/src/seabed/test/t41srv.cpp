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
#include "tmsfsutil.h"
#include "tutil.h"

int main(int argc, char *argv[]) {
    int             ferr;
    int             exit;
    int             inx;
    int             lerr;
    int             msgid;
    int             nid;
    int             oid;
    int             open;
    TPT_DECL       (phandle);
    int             pid;
    char            recv_buffer[BUFSIZ];
    short           recv_buffer2[BUFSIZ];
    MS_Result_Type  results;
    MS_SRE          sre;

    msfs_util_init_role(false, &argc, &argv, msg_debug_hook);
    ferr = msg_mon_process_startup(true); // system messages
    TEST_CHK_FEOK(ferr);
    msg_mon_enable_mon_messages(true);
    exit = 0;
    while (!exit) {
        open = 0;
        lerr = XWAIT(LREQ, -1);
        TEST_CHK_WAITIGNORE(lerr);
        for (;;) {
            lerr = XMSG_LISTEN_((short *) &sre, // sre
                                0,              // listenopts
                                0);             // listenertag
            if (lerr == XSRETYPE_NOWORK)
                break;
            ferr = XMSG_READDATA_(sre.sre_msgId,  // msgid
                                  recv_buffer,    // reqdata
                                  BUFSIZ);        // bytecount
            util_check("XMSG_READDATA_", ferr);
            if (sre.sre_flags & XSRE_MON) {
                MS_Mon_Msg *msg = (MS_Mon_Msg *) recv_buffer;
                printf("srv: mon message, type=%d\n", msg->type);
                open = (msg->type == MS_MsgType_Open);
                if (msg->type == MS_MsgType_Open) {
                    // there's is a chance that $cli hasn't registered yet!
                    for (inx = 0; inx < 1000; inx++) {
                        ferr = msg_mon_get_process_info((char *) "$cli", &nid, &pid);
                        TEST_CHK_FEOK(ferr);
                        if (nid < 0)
                            util_time_sleep_ms(1);
                        else
                            break;
                    }
                    assert(msg->u.open.nid == nid);
                    assert(msg->u.open.pid == pid);
                    assert(strcasecmp(msg->u.open.target_process_name, "$cli") == 0);
                }
            } else {
                exit = 1;
                printf("srv: NON-mon message\n");
            }
            XMSG_REPLY_(sre.sre_msgId,       // msgid
                        NULL,                // replyctrl
                        0,                   // replyctrlsize
                        NULL,                // replydata
                        0,                   // replydatasize
                        0,                   // errorclass
                        NULL);               // newphandle
            if (open) {
                printf("srv: opening client\n");
                ferr = msg_mon_open_process((char *) "$cli",      // name
                                            TPT_REF(phandle),
                                            &oid);
                TEST_CHK_FEOK(ferr);
                printf("srv: client opened\n");
            }
        }
    }
    printf("srv: sending message to client\n");
    ferr = XMSG_LINK_(TPT_REF(phandle),            // phandle
                      &msgid,                      // msgid
                      NULL,                        // reqctrl
                      0,                           // reqctrlsize
                      recv_buffer2,                // replyctrl
                      1,                           // replyctrlmax
                      NULL,                        // reqdata
                      0,                           // reqdatasize
                      recv_buffer,                 // replydata
                      BUFSIZ,                      // replydatamax
                      0,                           // linkertag
                      0,                           // pri
                      0,                           // xmitclass
                      0);                          // linkopts
    util_check("XMSG_LINK_", ferr);
    ferr = XMSG_BREAK_(msgid, (short *) &results, TPT_REF(phandle));
    util_check("XMSG_BREAK_", ferr);
    printf("srv: message received, shutting down\n");
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    return 0;
}
