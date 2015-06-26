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

enum         { MAX_SERVERS = 2 };
unsigned int   seed;

int rnd() {
    int rndret = (int) (10.0*rand_r(&seed)/(RAND_MAX+1.0));
    return rndret;
}

char          my_name[BUFSIZ];
char          name[BUFSIZ];
int           oid[MAX_SERVERS];
char          recv_buffer[MAX_SERVERS][BUFSIZ];
short         recv_buffer2[BUFSIZ];
short         recv_buffer3[MAX_SERVERS][BUFSIZ];
char          recv_buffer4[BUFSIZ];
char          send_buffer[MAX_SERVERS][BUFSIZ];
short         send_buffer2[BUFSIZ];


int main(int argc, char *argv[]) {
    bool            client = false;
    int             ferr;
    int             inx;
    int             len;
    int             lerr;
    int             loop = 10;
    int             msgid;
    TPT_DECL2      (phandle,MAX_SERVERS);
    bool            rand = false;
    RT              results;
    int             send_len;
    MS_SRE          sre;
    MS_SRE_LDONE    sre_ldone;
    int             srv;
    int             srv2;
    TAD             zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-rand",      TA_Bool, TA_NOMAX,    &rand      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client); // system messages
    TEST_CHK_FEOK(ferr);
    if (client) {
        for (srv = 0; srv < MAX_SERVERS; srv++) {
            sprintf(name, "$srv%d", srv);
            ferr = msg_mon_open_process(name,
                                        TPT_REF2(phandle,srv),
                                        &oid[srv]);
            TEST_CHK_FEOK(ferr);
        }
    }
    util_gethostname(my_name, sizeof(my_name));

    if (client) {
        for (inx = 0; inx < loop; inx++) {
            for (srv = 0; srv < MAX_SERVERS; srv++) {
                sprintf(send_buffer[srv], "hello, greetings from %s, inx=%d, srv=%d",
                        my_name, inx, srv);
                send_len = (int) strlen(send_buffer[srv]) + 1;
                ferr = XMSG_LINK_(TPT_REF2(phandle,srv),          // phandle
                                  &msgid,                         // msgid
                                  send_buffer2,                   // reqctrl
                                  (ushort) (inx & 1),             // reqctrlsize
                                  recv_buffer3[srv],              // replyctrl
                                  1,                              // replyctrlmax
                                  send_buffer[srv],               // reqdata
                                  (ushort) send_len,              // reqdatasize
                                  recv_buffer[srv],               // replydata
                                  BUFSIZ,                         // replydatamax
                                  (SB_Tag_Type) send_buffer[srv], // linkertag
                                  0,                              // pri
                                  0,                              // xmitclass
                                  XMSG_LINK_LDONEQ);              // linkopts
                util_check("XMSG_LINK_", ferr);
            }
            for (srv = 0; srv < MAX_SERVERS;) {
                if (rand)
                    util_time_sleep_ms(rnd());
                lerr = XWAIT(LDONE, -1);
                TEST_CHK_WAITIGNORE(lerr);
                for (;;) {
                    lerr = XMSG_LISTEN_((short *) &sre_ldone, // sre
                                        0,                    // listenopts
                                        0);                   // listenertag
                    if (lerr == XSRETYPE_NOWORK)
                        break;
                    srv++;
                    assert(lerr == XSRETYPE_LDONE);
                    for (srv2 = 0; srv2 < MAX_SERVERS; srv2++) {
                        if (sre_ldone.sre_linkTag ==
                            (SB_Tag_Type) send_buffer[srv2])
                            break;
                    }
                    assert(srv2 < MAX_SERVERS);
                    ferr = XMSG_BREAK_(sre_ldone.sre_msgId,
                                       results.u.s,
                                       TPT_REF2(phandle,srv2));
                    util_check("XMSG_BREAK_", ferr);
                    assert(results.u.t.ctrl_size == (uint) (inx & 1));
                    assert(results.u.t.data_size > (strlen(send_buffer[srv2]) + 14));
                    assert(results.u.t.errm == RT_DATA_RCVD); // data
                    printf("%s\n", recv_buffer[srv2]);
                }
            }
        }
    } else {
        for (inx = 0; inx < loop;) {
            lerr = XWAIT(LREQ, -1);
            lerr = XMSG_LISTEN_((short *) &sre, // sre
                                0,              // listenopts
                                0);             // listenertag
            if (lerr == XSRETYPE_NOWORK)
                continue;
            inx++;
            ferr = XMSG_READCTRL_(sre.sre_msgId,  // msgid
                                  recv_buffer2,   // reqctrl
                                  1);             // bytecount
            util_check("XMSG_READCTRL_", ferr);
            ferr = XMSG_READDATA_(sre.sre_msgId,  // msgid
                                  recv_buffer4,   // reqdata
                                  BUFSIZ);        // bytecount
            util_check("XMSG_READDATA_", ferr);
            strcat(recv_buffer4, "- reply from ");
            strcat(recv_buffer4, my_name);
            len = (int) strlen(recv_buffer4) + 1;
            if (rand)
                util_time_sleep_ms(rnd());
            XMSG_REPLY_(sre.sre_msgId,       // msgid
                        recv_buffer2,        // replyctrl
                        sre.sre_reqCtrlSize, // replyctrlsize
                        recv_buffer4,        // replydata
                        (ushort) len,        // replydatasize
                        0,                   // errorclass
                        NULL);               // newphandle
        }
    }

    if (client) {
        for (srv = 0; srv < MAX_SERVERS; srv++) {
            ferr = msg_mon_close_process(TPT_REF2(phandle,srv));
            TEST_CHK_FEOK(ferr);
        }
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
