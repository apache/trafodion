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

char  my_name[BUFSIZ];
char  recv_buffer[200];
short recv_bufferc[200];
char  send_buffer[200];
short send_bufferc[200];


int main(int argc, char *argv[]) {
    bool      client = false;
    int       count;
    int       ferr;
    int       inx;
    int       lerr;
    int       loop = 10;
    int       msgid;
    int       oid;
    TPT_DECL (phandle);
    RT        results;
    MS_SRE    sre;
    BMS_SRE   sre_tb;
    MS_SRE    sre_tx;
    TAD       zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
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
    } else
        msg_mon_enable_mon_messages(true); // need this
    util_gethostname(my_name, sizeof(my_name));

    for (inx = 0; inx < loop; inx++) {
        count = inx % 100;
        if (client) {
            ferr = XMSG_LINK_(TPT_REF(phandle),            // phandle
                              &msgid,                      // msgid
                              send_bufferc,                // reqctrl
                              (short) count,               // reqctrlsize
                              recv_bufferc,                // replyctrl
                              (short) (count+1),           // replyctrlmax
                              send_buffer,                 // reqdata
                              (short) (count+2),           // reqdatasize
                              recv_buffer,                 // replydata
                              (short) (count+3),           // replydatamax
                              0,                           // linkertag
                              (short) count,               // pri
                              0,                           // xmitclass
                              0);                          // linkopts
            util_check("XMSG_LINK_", ferr);
            ferr = XMSG_BREAK_(msgid, results.u.s, TPT_REF(phandle));
            util_check("XMSG_BREAK_", ferr);
        } else {
            do {
                lerr = XWAIT(LREQ, -1);
                TEST_CHK_WAITIGNORE(lerr);
                lerr = XMSG_LISTEN_((short *) &sre_tx,     // sre
                                    XLISTEN_TEST_IREQM,    // listenopts
                                    0);                    // listenertag
                if (lerr == XSRETYPE_NOWORK)
                    continue;
                if (sre_tx.sre_flags & XSRE_MON) {
                    // toss out monitor messages
                    lerr = XMSG_LISTEN_((short *) &sre, // sre
                                        0,              // listenopts
                                        0);             // listenertag
                    assert(lerr == XSRETYPE_IREQ);
                    assert(sre_tx.sre_flags & XSRE_MON);
                    lerr = XSRETYPE_NOWORK;
                    XMSG_REPLY_(sre.sre_msgId,  // msgid
                                NULL,           // replyctrl
                                0,              // replyctrlsize
                                NULL,           // replydata
                                0,              // replydatasize
                                0,              // errorclass
                                NULL);          // newphandle
                    continue;
                }
                lerr = BMSG_LISTEN_((short *) &sre_tb,     // sre
                                    BLISTEN_TEST_IREQM,    // listenopts
                                    0);                    // listenertag
                assert(lerr == BSRETYPE_IREQ);
                lerr = XMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
                assert(lerr == XSRETYPE_IREQ);
            } while (lerr == XSRETYPE_NOWORK);

            assert(sre.sre_pri == count);
            assert(sre.sre_reqCtrlSize == count);
            assert(sre.sre_reqDataSize == (count + 2));
            assert(sre.sre_replyCtrlMax == (count + 1));
            assert(sre.sre_replyDataMax == (count + 3));

            assert(sre.sre_msgId == sre_tx.sre_msgId);
            assert(sre.sre_flags == sre_tx.sre_flags);
            assert(sre.sre_pri == sre_tx.sre_pri);
            assert(sre.sre_reqCtrlSize == sre_tx.sre_reqCtrlSize);
            assert(sre.sre_reqDataSize == sre_tx.sre_reqDataSize);
            assert(sre.sre_replyCtrlMax == sre_tx.sre_replyCtrlMax);
            assert(sre.sre_replyDataMax == sre_tx.sre_replyDataMax);

            assert(sre.sre_msgId == sre_tb.sre_msgId);
            assert(sre.sre_flags == sre_tb.sre_flags);
            assert(sre.sre_pri == sre_tb.sre_pri);
            assert(sre.sre_reqCtrlSize == sre_tb.sre_reqCtrlSize);
            assert(sre.sre_reqDataSize == sre_tb.sre_reqDataSize);
            assert(sre.sre_replyCtrlMax == sre_tb.sre_replyCtrlMax);
            assert(sre.sre_replyDataMax == sre_tb.sre_replyDataMax);

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
    } else {
        ferr = msg_mon_process_close();
        TEST_CHK_FEOK(ferr);
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
