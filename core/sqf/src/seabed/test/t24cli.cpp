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
#include "ttrans.h"
#include "tutil.h"

int main(int argc, char *argv[]) {
    int                 exit;
    int                 ferr;
    bool                ldone;
    int                 lerr;
    int                 msgid;
    int                 nid;
    int                 oid;
    int                 pid;
    TPT_DECL           (phandle);
    char                recv_buffer[BUFSIZ];
    short               recv_buffer2[BUFSIZ];
    RT                  results;
    MS_SRE              sre;
    MS_Mon_Transid_Type transid;

    ferr = msfs_util_init_role(true, &argc, &argv, msg_debug_hook);
    TEST_CHK_FEOK(ferr);
    util_test_start(true);
    ferr = msg_mon_process_startup(true); // system messages
    TEST_CHK_FEOK(ferr);
    msg_mon_enable_mon_messages(true);
    ferr = msg_mon_get_process_info((char *) "$srv", &nid, &pid);
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_open_process((char *) "$srv",      // name
                                TPT_REF(phandle),
                                &oid);
    TEST_CHK_FEOK(ferr);
    TRANSID_SET_NULL(transid);
    ferr = msg_mon_deregister_death_notification(nid, pid, transid);
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_register_death_notification(nid, pid);
    TEST_CHK_FEOK(ferr);
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
                      XMSG_LINK_LDONEQ);           // linkopts
    util_check("XMSG_LINK_", ferr);
    exit = 0;
    ldone = false;
    while (!exit) {
        do {
            lerr = XWAIT(LREQ|LDONE, -1);
            TEST_CHK_WAITIGNORE(lerr);
            if (lerr == LDONE) {
                ldone = true;
                break;
            }
            lerr = XMSG_LISTEN_((short *) &sre, // sre
                                0,              // listenopts
                                0);             // listenertag
        } while (lerr == XSRETYPE_NOWORK);
        if (ldone) {
            printf("LINK done\n");
            lerr = XMSG_LISTEN_((short *) &sre,       // sre
                                XLISTEN_ALLOW_LDONEM, // listenopts
                                0);                   // listenertag
            assert(lerr == XSRETYPE_LDONE);
            ferr = XMSG_BREAK_(sre.sre_msgId,
                               results.u.s,
                               TPT_REF(phandle));
            assert(ferr == XZFIL_ERR_OK);
            break;
        }
        ferr = XMSG_READDATA_(sre.sre_msgId,  // msgid
                              recv_buffer,    // reqdata
                              BUFSIZ);        // bytecount
        util_check("XMSG_READDATA_", ferr);
        if (sre.sre_flags & XSRE_MON) {
            MS_Mon_Msg *msg = (MS_Mon_Msg *) recv_buffer;
            printf("cli: mon message, type=%d\n", msg->type);
            if (msg->type == MS_MsgType_ProcessDeath) {
                assert(msg->u.death.nid == nid);
                assert(msg->u.death.pid == pid);
                exit = 1;
            } else if (msg->type == MS_MsgType_Shutdown) {
                printf("cli: received cluster shutdown, level=%d\n", msg->u.shutdown.level);
                assert(msg->u.shutdown.level==MS_Mon_ShutdownLevel_Immediate);
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
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(true);
    return 0;
}
