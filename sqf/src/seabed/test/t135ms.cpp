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

enum { MAX_CLIENTS = 100 };
enum { MAX_SERVERS = 100 };

char  my_name[BUFSIZ];
char  recv_buffer[40000];
short recv_buffer2[20000];
char  send_buffer[40000];
short send_buffer2[20000];


int rnd(int max) {
    static uint seed = 1;
    int rndret = (int) ((double) max * rand_r(&seed) / (RAND_MAX+1.0));
    return rndret;
}

int main(int argc, char *argv[]) {
    bool       client = false;
    int        close_count;
    int        disable;
    int        ferr;
    int        inx;
    int        lerr;
    int        loop = 10;
    int        maxcp = 1;
    int        maxsp = 1;
    bool       mq = false;
    int        msgid;
    int        oid;
    TPT_DECL2  (phandle, MAX_SERVERS);
    int        pinx;
    RT         results;
    bool       rndab = false;
    int        s;
    char       serv[20];
    MS_SRE     sre;
    TAD        zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client       },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop         },
      { "-maxcp",     TA_Int,  MAX_CLIENTS, &maxcp        },
      { "-maxsp",     TA_Int,  MAX_SERVERS, &maxsp        },
      { "-mq",        TA_Bool, TA_NOMAX,    &mq           },
      { "-rndab",     TA_Bool, TA_NOMAX,    &rndab        },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL          },
      { "",           TA_End,  TA_NOMAX,    NULL          }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    if (client) {
        for (pinx = 0; pinx < maxsp; pinx++) {
            sprintf(serv, "$srv%d", pinx);
            ferr = msg_mon_open_process(serv,                 // name
                                        TPT_REF2(phandle, pinx),
                                        &oid);
            TEST_CHK_FEOK(ferr);
        }
    }
    util_gethostname(my_name, sizeof(my_name));
    msg_mon_enable_mon_messages(true);

    if (client) {
        for (inx = 0; inx < loop; inx++) {
            if (mq) {
                if ((inx % 100) == 0)
                    printf("inx=%d\n", inx);
            }
            for (pinx = 0; pinx < maxsp; pinx++) {
                ferr = XMSG_LINK_(TPT_REF2(phandle, pinx),     // phandle
                                  &msgid,                      // msgid
                                  send_buffer2,                // reqctrl
                                  19000,                       // reqctrlsize
                                  recv_buffer2,                // replyctrl
                                  20000,                       // replyctrlmax
                                  send_buffer,                 // reqdata
                                  39000,                       // reqdatasize
                                  recv_buffer,                 // replydata
                                  40000,                       // replydatamax
                                  0,                           // linkertag
                                  0,                           // pri
                                  0,                           // xmitclass
                                  0);                          // linkopts
                util_check("XMSG_LINK_", ferr);
                if (rndab)
                    s = rnd(2);
                else
                    s = 1;
                if (s) {
                    s = rnd(2);
                    util_time_sleep_ms(s);
                    ferr = XMSG_ABANDON_(msgid);
                    util_check("XMSG_ABANDON_", ferr);
                } else {
                    ferr = XMSG_BREAK_(msgid,
                                       results.u.s,
                                       TPT_REF2(phandle, pinx));
                    util_check("XMSG_BREAK_", ferr);
                }
            }
        }
    } else {
        close_count = 0;
        for (inx = 0; inx < loop * maxcp; inx++) {
            do {
                lerr = XWAIT(LREQ, -1);
                TEST_CHK_WAITIGNORE(lerr);
                lerr = XMSG_LISTEN_((short *) &sre,     // sre
                                    0,                  // listenopts
                                    0);                 // listenertag
            } while (lerr == XSRETYPE_NOWORK);
            // it's possible to get an abandon
            disable = msg_test_assert_disable();
            ferr = XMSG_READCTRL_(sre.sre_msgId,        // msgid
                                  recv_buffer2,         // reqctrl
                                  sre.sre_reqCtrlSize); // bytecount
            ferr = XMSG_READDATA_(sre.sre_msgId,        // msgid
                                  recv_buffer,          // reqdata
                                  sre.sre_reqDataSize); // bytecount
            msg_test_assert_enable(disable);
            XMSG_REPLY_(sre.sre_msgId,  // msgid
                        recv_buffer2,   // replyctrl
                        19000,          // replyctrlsize
                        recv_buffer,    // replydata
                        39000,          // replydatasize
                        0,              // errorclass
                        NULL);          // newphandle
            if (sre.sre_flags & XSRE_MON) {
                MS_Mon_Msg *msg = (MS_Mon_Msg *) recv_buffer;
                if (msg->type == MS_MsgType_Close) {
                    close_count++;
                    if (close_count >= maxcp)
                        break;
                }
            }
            if (lerr == XSRETYPE_IREQ)
                inx--; // stop on close, not count
        }
    }
    if (client) {
        for (pinx = 0; pinx < maxsp; pinx++) {
            ferr = msg_mon_close_process(TPT_REF2(phandle, pinx));
            TEST_CHK_FEOK(ferr);
        }
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
