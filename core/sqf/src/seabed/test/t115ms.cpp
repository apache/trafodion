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

// derived from t4ms
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

char  my_name[BUFSIZ];
char  recv_buffer[MAX_BUF];
char  send_buffer[2][MAX_BUF];


int rnd() {
    static uint seed = 1;
    int rndret = (int) ((double) MAX_BUF * rand_r(&seed) / (RAND_MAX+1.0));
    return rndret;
}

int main(int argc, char *argv[]) {
    bool       client = false;
    int        ferr;
    int        inx;
    int        inx2;
    int        inx3 = 0;
    int        lerr;
    int        loop = 10;
    int        msgid;
    int        oid;
    TPT_DECL  (phandle);
    int        reply_len;
    int        req_len;
    RT         results;
    BMS_SRE    sre;
    int        which;
    TAD        zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);          // system messages?
    TEST_CHK_FEOK(ferr);
    if (client) {
        ferr = msg_mon_open_process((char *) "$srv",           // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
    }
    util_gethostname(my_name, sizeof(my_name));

    for (inx = 0; inx < MAX_BUF; inx++) {
        send_buffer[0][inx] = (char) inx;
        send_buffer[1][inx] = (char) (inx + 1);
    }
    for (inx = 0; inx < loop; inx++) {
        which = inx & 1;
        req_len = rnd();
        reply_len = rnd();
        if (client) {
            if (++inx3 >= 1000) {
                printf("inx=%d\n", inx);
                inx3 = 0;
            }
            ferr = BMSG_LINK_(TPT_REF(phandle),            // phandle
                              &msgid,                      // msgid
                              NULL,                        // reqctrl
                              0,                           // reqctrlsize
                              NULL,                        // replyctrl
                              0,                           // replyctrlmax
                              send_buffer[which],          // reqdata
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
            assert(results.u.t.data_size == (uint) reply_len);
            assert(results.u.t.errm == RT_DATA_RCVD); // data
            for (inx2 = 0; inx2 < reply_len; inx2++) {
                if (send_buffer[which][inx2] != recv_buffer[inx2]) {
                    printf("send_buffer[%d](%d) != recv_buffer(%d) @ %d\n",
                           which, send_buffer[which][inx2],
                           recv_buffer[inx2], inx2);
                    assert(send_buffer[which][inx2] == recv_buffer[inx2]);
                }
            }
        } else {
            do {
                lerr = XWAIT(LREQ, -1);
                TEST_CHK_WAITIGNORE(lerr);
                lerr = BMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
            } while (lerr == XSRETYPE_NOWORK);
            ferr = BMSG_READDATA_(sre.sre_msgId,  // msgid
                                  recv_buffer,    // reqdata
                                  MAX_BUF);       // bytecount
            util_check("BMSG_READDATA_", ferr);
            if (which)
                for (inx2 = sre.sre_reqDataSize; inx2 < reply_len; inx2++)
                    recv_buffer[inx2] = (char) (inx2 + 1);
            else
                for (inx2 = sre.sre_reqDataSize; inx2 < reply_len; inx2++)
                    recv_buffer[inx2] = (char) inx2;
            BMSG_REPLY_(sre.sre_msgId,  // msgid
                        NULL,           // replyctrl
                        0,              // replyctrlsize
                        recv_buffer,    // replydata
                        reply_len,      // replydatasize
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
