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
    bool            exit;
    int             ferr;
    int             lerr;
    int             msgid;
    int             nid;
    int             oid;
    int             pid;
    TPT_DECL       (phandle);
    char            recv_buffer[BUFSIZ];
    short           recv_buffer2[BUFSIZ];
    MS_Result_Type  results;
    MS_SRE          sre;

    ferr = msfs_util_init_role(true, &argc, &argv, msg_debug_hook);
    TEST_CHK_FEOK(ferr);
    util_test_start(true);
    ferr = msg_mon_process_startup(true); // system messages
    TEST_CHK_FEOK(ferr);
    msg_mon_enable_mon_messages(true);
    printf("cli: opening server\n");
    ferr = msg_mon_get_process_info((char *) "$srv", &nid, &pid);
    TEST_CHK_FEOK(ferr);
    printf("cli: server opened\n");
    ferr = msg_mon_open_process((char *) "$srv",      // name
                                TPT_REF(phandle),
                                &oid);
    TEST_CHK_FEOK(ferr);
    exit = false;
    while (!exit) {
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
                printf("cli: mon message, type=%d\n", msg->type);
                if (msg->type == MS_MsgType_Open) {
                    ferr = msg_mon_get_process_info((char *) "$srv", &nid, &pid);
                    TEST_CHK_FEOK(ferr);
                    assert(msg->u.open.nid == nid);
                    assert(msg->u.open.pid == pid);
                    assert(strcasecmp(msg->u.open.target_process_name, "$srv") == 0);
                    exit = true;
                }
            } else {
                printf("cli: NON-mon message\n");
            }
            XMSG_REPLY_(sre.sre_msgId,       // msgid
                        NULL,                // replyctrl
                        0,                   // replyctrlsize
                        NULL,                // replydata
                        0,                   // replydatasize
                        0,                   // errorclass
                        NULL);               // newphandle
        }
    }
    printf("cli: sending message to server\n");
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
    exit = false;
    while (!exit) {
        lerr = XWAIT(LREQ, -1);
        for (;;) {
            lerr = XMSG_LISTEN_((short *) &sre, // sre
                                0,              // listenopts
                                0);             // listenertag
            if (lerr == XSRETYPE_NOWORK)
                break;
            ferr = XMSG_READDATA_(sre.sre_msgId,  // msgid
                                  recv_buffer,    // reqctrl
                                  BUFSIZ);        // bytecount
            util_check("XMSG_READDATA_", ferr);
            if (sre.sre_flags & XSRE_MON) {
                MS_Mon_Msg *msg = (MS_Mon_Msg *) recv_buffer;
                printf("cli: mon message, type=%d\n", msg->type);
                if (msg->type == MS_MsgType_ProcessDeath) {
                    assert(msg->u.death.nid == nid);
                    assert(msg->u.death.pid == pid);
                    exit = true;
                }
            } else
                printf("cli: NON-mon message\n");
            XMSG_REPLY_(sre.sre_msgId,       // msgid
                        NULL,                // replyctrl
                        0,                   // replyctrlsize
                        NULL,                // replydata
                        0,                   // replydatasize
                        0,                   // errorclass
                        NULL);               // newphandle
        }
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(true);
    return 0;
}
