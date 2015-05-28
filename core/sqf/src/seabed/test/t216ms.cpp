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

enum        { MAX_CLI = 100 };
enum        { MAX_OUT = 100 };
enum        { MAX_SRV = 100 };
char          my_name[BUFSIZ];
char          recv_buffer[MAX_OUT][BUFSIZ];
unsigned int  seed;
char          send_buffer[MAX_OUT][BUFSIZ];

int rnd(int max) {
    int rndret = (int) (1.0 * max*rand_r(&seed)/(RAND_MAX+1.0));
    return rndret;
}


int main(int argc, char *argv[]) {
    int                 abandon = 0;
    bool                client = false;
    int                 closes;
    int                 count;
    int                 count_abandon;
    int                 ferr;
    int                 inxl;
    int                 inxo;
    int                 inxs;
    int                 lerr;
    int                 loop = 10;
    int                 maxcp = 1;
    int                 maxout = 1;
    int                 maxsp = 1;
    int                 msgid[MAX_OUT];
    int                 msg_count;
    bool                mq = false;
    int                 oid;
    TPT_DECL2          (phandle, MAX_SRV);
    int                 recv_len;
    RT                  results;
    int                 send_len;
    char                serv[20];
    BMS_SRE             sre;
    bool                verbose = false;
    TAD                 zargs[] = {
      { "-abandon",   TA_Int,  TA_NOMAX,    &abandon   },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-maxcp",     TA_Int,  MAX_CLI,     &maxcp     },
      { "-maxout",    TA_Int,  MAX_OUT,     &maxout    },
      { "-maxsp",     TA_Int,  MAX_SRV,     &maxsp     },
      { "-mq",        TA_Bool, TA_NOMAX,    &mq        },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    ferr = msg_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(my_name, BUFSIZ);
    util_check("msg_mon_get_my_process_name", ferr);

    if (client) {
        ferr = XCONTROLMESSAGESYSTEM(XCTLMSGSYS_SETSENDLIMIT,
                                     XMAX_SETTABLE_SENDLIMIT);
        assert(ferr == XZFIL_ERR_OK);
        msg_count = 0;
        count_abandon = 0;
        for (inxl = 0; inxl < loop; inxl++) {
            for (inxs = 0; inxs < maxsp; inxs++) {
                sprintf(serv, "$srv%d", inxs);
                ferr = msg_mon_open_process((char *) serv,       // name
                                            TPT_REF2(phandle, inxs),
                                            &oid);
                TEST_CHK_FEOK(ferr);
            }
            count = 0;
            if (abandon)
                count_abandon = rnd(abandon);
            for (inxs = 0; inxs < maxsp; inxs++) {
                for (inxo = 0; inxo < maxout; inxo++) {
                    send_len = rnd(BUFSIZ);
                    ferr = BMSG_LINK_(TPT_REF2(phandle, inxs), // phandle
                                      &msgid[inxo],            // msgid
                                      NULL,                    // reqctrl
                                      0,                       // reqctrlsize
                                      NULL,                    // replyctrl
                                      0,                       // replyctrlmax
                                      send_buffer[inxo],       // reqdata
                                      send_len,                // reqdatasize
                                      recv_buffer[inxo],       // replydata
                                      BUFSIZ,                  // replydatamax
                                      0,                       // linkertag
                                      0,                       // pri
                                      0,                       // xmitclass
                                      0);                      // linkopts
                    util_check("XMSG_LINK_", ferr);
                }
            }
            for (inxs = 0; inxs < maxsp; inxs++) {
                for (inxo = 0; inxo < maxout; inxo++) {
                    msg_count++;
                    if (verbose)
                        printf("client(%s): msg-count=%d\n",
                               my_name, msg_count);
                    count++;
                    if (abandon && (count >= count_abandon)) {
                        if (verbose)
                            printf("client(%s): sending abandon, count=%d\n",
                                   my_name, count);
                        count_abandon = rnd(abandon);
                        count = 0;
                        ferr = XMSG_ABANDON_(msgid[inxo]);
                        util_check("XMSG_ABANDON_", ferr);
                    } else {
                        ferr = BMSG_BREAK_(msgid[inxo],
                                           results.u.s,
                                           TPT_REF2(phandle, inxs));
                        util_check("XMSG_BREAK_", ferr);
                    }
                }
            }
            if (mq) {
                if ((inxl % 100) == 0)
                    printf("client(%s): count=%d\n", my_name, inxl);
            }
        }
        for (inxs = 0; inxs < maxsp; inxs++) {
            ferr = msg_mon_close_process(TPT_REF2(phandle, inxs));
            TEST_CHK_FEOK(ferr);
        }
    } else {
        ferr = XCONTROLMESSAGESYSTEM(XCTLMSGSYS_SETRECVLIMIT,
                                     XMAX_SETTABLE_RECVLIMIT);
        assert(ferr == XZFIL_ERR_OK);
        closes = 0;
        msg_count = 0;
        msg_mon_enable_mon_messages(true);
        // process requests
        for (;;) {
            do {
                lerr = XWAIT(LREQ, -1);
                lerr = BMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
            } while (lerr == XSRETYPE_NOWORK);
            if (sre.sre_flags & XSRE_MON) {
                ferr = XMSG_READDATA_(sre.sre_msgId,  // msgid
                                      recv_buffer[0], // reqdata
                                      BUFSIZ);        // bytecount
                util_check("XMSG_READDATA_", ferr);
                MS_Mon_Msg *msg = (MS_Mon_Msg *) recv_buffer[0];
                if (verbose)
                    printf("server(%s): mon-msg type=%d\n",
                           my_name, msg->type);
                if (msg->type == MS_MsgType_Close) {
                    closes++;
                    if (verbose)
                        printf("server(%s): closes=%d\n",
                               my_name, closes);
                }
            } else {
                msg_count++;
                if (verbose)
                    printf("server(%s): msg-count=%d\n",
                           my_name, msg_count);
            }
            recv_len = rnd(BUFSIZ);
            BMSG_REPLY_(sre.sre_msgId,       // msgid
                        NULL,                // replyctrl
                        0,                   // replyctrlsize
                        recv_buffer[0],      // replydata
                        recv_len,            // replydatasize
                        0,                   // errorclass
                        NULL);               // newphandle
            if (closes >= maxcp)
                break;
        }
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
