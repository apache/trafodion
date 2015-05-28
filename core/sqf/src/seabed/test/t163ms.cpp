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

enum { MAX_MSGS = 100 };

char my_name[BUFSIZ];
char recv_buffer[MAX_MSGS][100];
char send_buffer[MAX_MSGS][100];


int main(int argc, char *argv[]) {
    bool            client = false;
    int             exprinx;
    int             ferr;
    int             inx;
    int             len;
    int             lerr;
    int             loop = 100;
    int             msgid[MAX_MSGS];
    int             oid;
    char           *p;
    TPT_DECL       (phandle);
    int             pri;
    RT              results;
    int             rinx;
    BMS_SRE         sre;
    BMS_SRE_LDONE   sre_ldone;
    TAD             zargs[] = {
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
        msg_enable_priority_queue();
    util_gethostname(my_name, sizeof(my_name));

    if (client) {
        for (inx = 0; inx < loop; inx++) {
            sprintf(send_buffer[inx], "hello, greetings from %s, inx=%d",
                    my_name, inx);
            pri = 2 * inx + 5;
            ferr = BMSG_LINK_(TPT_REF(phandle),            // phandle
                              &msgid[inx],                 // msgid
                              NULL,                        // reqctrl
                              0,                           // reqctrlsize
                              NULL,                        // replyctrl
                              0,                           // replyctrlmax
                              send_buffer[inx],            // reqdata
                              100,                         // reqdatasize
                              recv_buffer[inx],            // replydata
                              100,                         // replydatamax
                              inx,                         // linkertag
                              (short) pri,                 // pri
                              0,                           // xmitclass
                              BMSG_LINK_LDONEQ);           // linkopts
            util_check("BMSG_LINK_", ferr);
        }
        exprinx = loop - 1;
        for (inx = 0; inx < loop;) {
            lerr = XWAIT(LDONE, -1);
            TEST_CHK_WAITIGNORE(lerr);
            do {
                lerr = BMSG_LISTEN_((short *) &sre_ldone, // sre
                                    0,                    // listenopts
                                    0);                   // listenertag
                if (lerr == BSRETYPE_LDONE) {
                    ferr = BMSG_BREAK_(sre_ldone.sre_msgId, results.u.s, TPT_REF(phandle));
                    util_check("BMSG_BREAK_", ferr);
                    assert(results.u.t.ctrl_size == 0);
                    assert(results.u.t.errm == RT_DATA_RCVD); // data
                    printf("%s\n", recv_buffer[sre_ldone.sre_linkTag]);
                    p = strchr(recv_buffer[sre_ldone.sre_linkTag], '=');
                    assert(p != NULL);
                    rinx = atoi(&p[1]);
                    if (rinx != exprinx) {
                        printf("rinx=%d, exprinx=%d\n", rinx, exprinx);
                        assert(rinx == exprinx);
                    }
                    inx++;
                    exprinx--;
                }
            } while (lerr == BSRETYPE_LDONE);
        }
    } else {
        for (inx = 0; inx < loop; inx++) {
            do {
                lerr = XWAIT(LREQ, -1);
                if (inx == 0)
                    sleep(5);
                lerr = BMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
            } while (lerr == BSRETYPE_NOWORK);
            ferr = BMSG_READCTRL_(sre.sre_msgId,    // msgid
                                  NULL,             // reqctrl
                                  0);               // bytecount
            util_check("BMSG_READCTRL_", ferr);
            ferr = BMSG_READDATA_(sre.sre_msgId,    // msgid
                                  recv_buffer[inx], // reqdata
                                  100);             // bytecount
            util_check("BMSG_READDATA_", ferr);
            strcat(recv_buffer[inx], "- reply from ");
            strcat(recv_buffer[inx], my_name);
            len = (int) strlen(recv_buffer[inx]) + 1;
            len = len; // touch - not currently used
            BMSG_REPLY_(sre.sre_msgId,      // msgid
                        NULL,               // replyctrl
                        0,                  // replyctrlsize
                        recv_buffer[inx],   // replydata
                        100,                // replydatasize
                        0,                  // errorclass
                        NULL);              // newphandle
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
