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
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

char my_name[BUFSIZ];
char recv_buffer[40000];
char send_buffer[40000];


int main(int argc, char *argv[]) {
    bool        client = false;
    int         closes;
    bool        done;
    int         ferr;
    int         inx;
    int         lerr;
    int         loop = 10;
    int         oid;
    TPT_DECL   (phandle);
    MS_SRE      sre;
    bool        sys_mon_msg;
    MS_Mon_Msg *sys_msg = (MS_Mon_Msg *) recv_buffer;
    bool        verbose = false;
    TAD         zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    setenv("MS_CONN_IDLE_TIMEOUT", "1", 1);
    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    util_gethostname(my_name, sizeof(my_name));

    if (client) {
        for (inx = 0; inx < loop; inx++) {
            if (verbose)
                printf("client: open $srv, inx=%d\n", inx);
            ferr = msg_mon_open_process((char *) "$srv",      // name
                                        TPT_REF(phandle),
                                        &oid);
            TEST_CHK_FEOK(ferr);
            if (verbose)
                printf("client: close $srv, inx=%d\n", inx);
            ferr = msg_mon_close_process(TPT_REF(phandle));
            TEST_CHK_FEOK(ferr);
            if (inx & 1)
                sleep(1);
        }
    } else {
        msg_mon_enable_mon_messages(true);
        closes = 0;
        done = false;
        for (inx = 0; !done; inx++) {
            do {
                lerr = XWAIT(LREQ, -1);
                TEST_CHK_WAITIGNORE(lerr);
                lerr = XMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
            } while (lerr == XSRETYPE_NOWORK);
            sys_mon_msg = (sre.sre_flags & BSRE_MON);
            if (sys_mon_msg) {
                ferr = BMSG_READDATA_(sre.sre_msgId,  // msgid
                                      recv_buffer,    // reqdata
                                      40000);         // bytecount
                util_check("BMSG_READDATA_", ferr);
                if (sys_msg->type == MS_MsgType_Close) {
                    closes++;
                    if (closes >= loop)
                        done = true;
                }
            }
            if (verbose)
                printf("server: reply, inx=%d, closes=%d\n", inx, closes);
            XMSG_REPLY_(sre.sre_msgId,  // msgid
                        NULL,           // replyctrl
                        0,              // replyctrlsize
                        recv_buffer,    // replydata
                        0,              // replydatasize
                        0,              // errorclass
                        NULL);          // newphandle
        }
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
