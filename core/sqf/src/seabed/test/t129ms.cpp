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

// derived from t115ms
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

enum { MAX_BUF = 0x40000 };


int rnd(int max) {
    static uint seed = 1;
    int rndret = (int) ((double) max * rand_r(&seed) / (RAND_MAX+1.0));
    if (rndret < 4)
        rndret = 4; // save room for size
    return rndret;
}

char my_name[BUFSIZ];
char recv_buffer[MAX_BUF];
char recv_ctrl[MAX_BUF];
char send_buffer[MAX_BUF];
char send_ctrl[MAX_BUF];

int main(int argc, char *argv[]) {
    bool       client = false;
    int        ctrl_len;
    int        csize = 0;
    int        dsize = 1024;
    int        ferr;
    int        inx;
    int        inx2;
    int        inx3 = 0;
    int        lcsize;
    int        ldsize;
    int        lerr;
    int        loop = 10;
    int        msgid;
    int        oid;
    TPT_DECL  (phandle);
    int        reply_len;
    int        req_len;
    RT         results;
    BMS_SRE    sre;
    TAD        zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-csize",     TA_Int,  MAX_BUF,     &csize     },
      { "-dsize",     TA_Int,  MAX_BUF,     &dsize     },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);             // system messages?
    TEST_CHK_FEOK(ferr);
    if (client) {
        ferr = msg_mon_open_process((char *) "$srv",              // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
    }
    util_gethostname(my_name, sizeof(my_name));

    for (inx = 0; inx < loop; inx++) {
        ctrl_len = rnd(csize);
        req_len = rnd(dsize);
        reply_len = 10;
        for (inx2 = 0; inx2 < ctrl_len; inx2++)
            send_ctrl[inx2] = (char) (inx + inx2);
        for (inx2 = 0; inx2 < req_len; inx2++)
            send_buffer[inx2] = (char) (inx + inx2 + 1);
        memcpy(send_ctrl, &ctrl_len, sizeof(ctrl_len));
        memcpy(send_buffer, &req_len, sizeof(req_len));
        if (client) {
            if (++inx3 >= 1000) {
                printf("inx=%d\n", inx);
                inx3 = 0;
            }
            ferr = BMSG_LINK_(TPT_REF(phandle),            // phandle
                              &msgid,                      // msgid
                              (short *) send_ctrl,         // reqctrl
                              ctrl_len,                    // reqctrlsize
                              (short *) recv_ctrl,         // replyctrl
                              MAX_BUF,                     // replyctrlmax
                              send_buffer,                 // reqdata
                              req_len,                     // reqdatasize
                              recv_buffer,                 // replydata
                              MAX_BUF,                     // replydatamax
                              0,                           // linkertag
                              0,                           // pri
                              0,                           // xmitclass
                              0);                          // linkopts
            util_check("BMSG_LINK_", ferr);
            ferr = BMSG_BREAK_(msgid, results.u.s, TPT_REF(phandle));
            util_check("BMSG_BREAK_", ferr);
            assert(results.u.t.ctrl_size == 0);
        } else {
            do {
                lerr = XWAIT(LREQ, -1);
                TEST_CHK_WAITIGNORE(lerr);
                lerr = BMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
            } while (lerr == XSRETYPE_NOWORK);
            ferr = BMSG_READCTRL_(sre.sre_msgId,       // msgid
                                  (short *) recv_ctrl, // reqctrl
                                  MAX_BUF);            // bytecount
            util_check("BMSG_READDATA_", ferr);
            ferr = BMSG_READDATA_(sre.sre_msgId,  // msgid
                                  recv_buffer,    // reqdata
                                  MAX_BUF);       // bytecount
            util_check("BMSG_READDATA_", ferr);
            assert(sre.sre_reqCtrlSize == ctrl_len);
            assert(sre.sre_reqDataSize == req_len);
            memcpy(&lcsize, recv_ctrl, sizeof(lcsize));
            memcpy(&ldsize, recv_buffer, sizeof(ldsize));
            for (inx2 = 4; inx2 < lcsize; inx2++) {
                if (recv_ctrl[inx2] != send_ctrl[inx2]) {
                    printf("CTRL mismatch inx=%d, inx2=%d, exp=%x, obs=%x\n",
                           inx,
                           inx2,
                           send_ctrl[inx2],
                           recv_ctrl[inx2]);
                    assert(recv_ctrl[inx2] == send_ctrl[inx2]);
                }
            }
            for (inx2 = 4; inx2 < ldsize; inx2++) {
                if (recv_buffer[inx2] != send_buffer[inx2]) {
                    printf("DATA mismatch inx=%d, inx2=%d, exp=%x, obs=%x\n",
                           inx,
                           inx2,
                           send_buffer[inx2],
                           recv_buffer[inx2]);
                    assert(recv_buffer[inx2] == send_buffer[inx2]);
                }
            }
            for (inx2 = sre.sre_reqDataSize; inx2 < reply_len; inx2++)
                recv_buffer[inx2] = (char) (inx2 + 1);
            BMSG_REPLY_(sre.sre_msgId,  // msgid
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
