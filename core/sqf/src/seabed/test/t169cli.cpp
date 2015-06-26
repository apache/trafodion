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
#include "seabed/thread.h"

#include "tchkfe.h"
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

int            maxcp = 1;
int            maxs = 1;
char           event_data[MS_MON_MAX_SYNC_DATA];
char           my_name[BUFSIZ];
bool           verbose = false;


int main(int argc, char *argv[]) {
    char       cli[10];
    bool       closer;
    int        disable;
    int        event_len;
    int        ferr;
    int        inst = 0;
    int        inx;
    int        lerr;
    int        loop = 10;
    int        msgid;
    int        nid;
    int        oid;
    TPT_DECL  (phandle);
    int        pid;
    char       recv_buffer[30000];
    RT         results;
    char       send_buffer[10];
    MS_SRE     sre;
    TAD        zargs[] = {
      { "-client",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-inst",      TA_Int,  TA_NOMAX,    &inst      },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-maxcp",     TA_Int,  TA_NOMAX,    &maxcp     },
      { "-maxs",      TA_Int,  TA_NOMAX,    &maxs      },
      { "-maxsp",     TA_Next, TA_NOMAX,    NULL       },
      { "-name",      TA_Next, TA_NOMAX,    NULL       },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(true);
    ferr = msg_mon_process_startup(true);  // system messages?
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(my_name, sizeof(my_name));
    TEST_CHK_FEOK(ferr);
    msg_mon_enable_mon_messages(true);

    // process-wait for clients/shell
    // (needed so msg_mon_register_death_notification below works correctly)
    ferr = msfs_util_wait_process_count(MS_ProcessType_TSE, maxcp, NULL, false);
    TEST_CHK_FEOK(ferr);
    sleep(1);

    if (verbose)
        printf("%s: inst=%d, maxcp=%d\n", my_name, inst, maxcp);
    closer = (inst == (maxcp -1));
    if (closer && (maxcp > 1)) {
        if (verbose)
           printf("%s: register-death-notifications\n", my_name);
        for (inx = 0; inx < (maxcp - 1); inx++) {
            sprintf(cli, "$c%d", inx);
            ferr = msg_mon_get_process_info(cli, &nid, &pid);
            TEST_CHK_FEOK(ferr);
            ferr = msg_mon_register_death_notification(nid, pid);
            TEST_CHK_FEOK(ferr);
        }
    }

    //
    // wait here until all clients sync-up
    // (needed so msg_mon_register_death_notification above works correctly)
    //
    ferr = msg_mon_event_send(-1,                         // nid
                              -1,                         // pid
                              MS_ProcessType_TSE,         // process-type
                              3,                          // event-id
                              0,                          // event-len
                              NULL);                      // event-data
    TEST_CHK_FEOK(ferr);
    for (inx = 0; inx < maxcp; inx++) {
        ferr = msg_mon_event_wait(3, &event_len, event_data);
        TEST_CHK_FEOK(ferr);
    }
    disable = msg_test_assert_disable();
    // check process-type range
    ferr = msg_mon_event_send(-1,                         // nid
                              -1,                         // pid
                              -1,                         // process-type
                              4,                          // event-id
                              0,                          // event-len
                              NULL);                      // event-data
    assert(ferr == XZFIL_ERR_BOUNDSERR);
    ferr = msg_mon_event_send(-1,                         // nid
                              -1,                         // pid
                              MS_ProcessType_SMS + 1,    // process-type
                              5,                          // event-id
                              0,                          // event-len
                              NULL);                      // event-data
    assert(ferr == XZFIL_ERR_BOUNDSERR);
    msg_test_assert_enable(disable);

    ferr = msg_mon_open_process((char *) "$srv",      // name
                                TPT_REF(phandle),
                                &oid);
    TEST_CHK_FEOK(ferr);
    for (inx = 0; inx < loop; inx++) {
        ferr = XMSG_LINK_(TPT_REF(phandle),            // phandle
                          &msgid,                      // msgid
                          NULL,                        // reqctrl
                          0,                           // reqctrlsize
                          NULL,                        // replyctrl
                          0,                           // replyctrlmax
                          send_buffer,                 // reqdata
                          1,                           // reqdatasize
                          recv_buffer,                 // replydata
                          0,                           // replydatamax
                          0,                           // linkertag
                          0,                           // pri
                          0,                           // xmitclass
                          0);                          // linkopts
        util_check("XMSG_LINK_", ferr);
        ferr = XMSG_BREAK_(msgid, results.u.s, TPT_REF(phandle));
        util_check("XMSG_BREAK_", ferr);
    }
    if (closer) {
        inx = 0;
        while (inx < (maxcp - 1)) {
            do {
                lerr = XWAIT(LREQ, -1);
                TEST_CHK_WAITIGNORE(lerr);
                lerr = XMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
            } while (lerr == XSRETYPE_NOWORK);
            if (sre.sre_flags & XSRE_MON) {
                ferr = BMSG_READDATA_(sre.sre_msgId,  // msgid
                                      recv_buffer,    // reqdata
                                      BUFSIZ);        // bytecount
                TEST_CHK_FEOK(ferr);
                MS_Mon_Msg *msg = (MS_Mon_Msg *) recv_buffer;
                if (msg->type == MS_MsgType_ProcessDeath) {
                    if (verbose)
                       printf("%s: process death process=%s\n",
                              my_name, msg->u.death.process_name);
                    inx++;
                }
            }
            XMSG_REPLY_(sre.sre_msgId,  // msgid
                        NULL,           // replyctrl
                        0,              // replyctrlsize
                        recv_buffer,    // replydata
                        0,              // replydatasize
                        0,              // errorclass
                        NULL);          // newphandle
        }
        if (verbose)
           printf("%s: sending stop\n", my_name);
        for (inx = 0; inx < maxs; inx++) {
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
            ferr = XMSG_BREAK_(msgid, results.u.s, TPT_REF(phandle));
            util_check("XMSG_BREAK_", ferr);
        }
    }
    ferr = msg_mon_close_process(TPT_REF(phandle));
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(true);
    return 0;
}
