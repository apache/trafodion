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


void do_readmsg() {
    int     lerr;
    MS_SRE  sre;

    do {
        lerr = XWAIT(LREQ, -1);
        TEST_CHK_WAITIGNORE(lerr);
        lerr = XMSG_LISTEN_((short *) &sre, // sre
                            0,              // listenopts
                            0);             // listenertag
    } while (lerr == XSRETYPE_NOWORK);
    XMSG_REPLY_(sre.sre_msgId,       // msgid
                NULL,                // replyctrl
                0,                   // replyctrlsize
                NULL,                // replydata
                0,                   // replydatasize
                0,                   // errorclass
                NULL);               // newphandle
}

char  my_name[BUFSIZ];
char  recv_buffer[BUFSIZ];
short recv_buffer2[BUFSIZ];
short recv_buffer3[BUFSIZ];
char  send_buffer[BUFSIZ];
short send_buffer2[BUFSIZ];

int main(int argc, char *argv[]) {
    bool            client = false;
    int             ferr;
    int             inx;
    int             inx2;
    int             len;
    int             lerr;
    int             loop = 10;
    int             msgid;
    int             oid;
    TPT_DECL       (phandle);
    RT              results;
    int             send_len;
    MS_SRE          sre;
    MS_SRE_LDONE    sre_ldone;
    TAD             zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(true); // system messages
    TEST_CHK_FEOK(ferr);
    msg_mon_enable_mon_messages(true);
    // process-wait for client/server/shell
    ferr = msfs_util_wait_process_count(MS_ProcessType_Generic, 3, NULL, false);
    TEST_CHK_FEOK(ferr);
    if (client) {
        ferr = msg_mon_open_process((char *) "$srv",      // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
        do_readmsg(); // open message
    } else {
        do_readmsg(); // open message
        ferr = msg_mon_open_process((char *) "$cli",      // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
    }
    util_gethostname(my_name, sizeof(my_name));

    for (inx = 0; inx < loop; inx++) {
        if (client) {
            sprintf(send_buffer, "hello, greetings from client %s, inx=%d",
                    my_name, inx);
            send_len = (int) strlen(send_buffer) + 1;
            ferr = XMSG_LINK_(TPT_REF(phandle),            // phandle
                              &msgid,                      // msgid
                              send_buffer2,                // reqctrl
                              (ushort) (inx & 1),          // reqctrlsize
                              recv_buffer3,                // replyctrl
                              1,                           // replyctrlmax
                              send_buffer,                 // reqdata
                              (ushort) send_len,           // reqdatasize
                              recv_buffer,                 // replydata
                              BUFSIZ,                      // replydatamax
                              (SB_Tag_Type) send_buffer,   // linkertag
                              0,                           // pri
                              0,                           // xmitclass
                              XMSG_LINK_LDONEQ);           // linkopts
            util_check("XMSG_LINK_", ferr);
            util_time_sleep_ms(10);
            for (inx2 = 0; inx2 < 2; inx2++) {
                switch (inx2 - (inx & 1)) {
                case 0:
                    do {
                        lerr = XWAIT(LREQ, -1);
                        TEST_CHK_WAITIGNORE(lerr);
                        lerr = XMSG_LISTEN_((short *) &sre,      // sre
                                            BLISTEN_ALLOW_IREQM, // listenopts
                                            0);                  // listenertag
                    } while (lerr == XSRETYPE_NOWORK);
                    ferr = XMSG_READDATA_(sre.sre_msgId,  // msgid
                                          recv_buffer,    // reqdata
                                          BUFSIZ);        // bytecount
                    util_check("XMSG_READDATA_", ferr);
                    printf("%s\n", recv_buffer);
                    XMSG_REPLY_(sre.sre_msgId,       // msgid
                                NULL,                // replyctrl
                                0,                   // replyctrlsize
                                NULL,                // replydata
                                0,                   // replydatasize
                                0,                   // errorclass
                                NULL);               // newphandle
                    break;
                default:
                    do {
                        lerr = XWAIT(LDONE, -1);
                        TEST_CHK_WAITIGNORE(lerr);
                        lerr = XMSG_LISTEN_((short *) &sre_ldone,// sre
                                            BLISTEN_ALLOW_LDONEM,// listenopts
                                            0);                  // listenertag
                    } while (lerr == XSRETYPE_NOWORK);
                    assert(lerr == XSRETYPE_LDONE);
                    assert(sre_ldone.sre_linkTag == (SB_Tag_Type) send_buffer);
                    ferr = XMSG_BREAK_(msgid, results.u.s, TPT_REF(phandle));
                    util_check("XMSG_BREAK_", ferr);
                    assert(results.u.t.ctrl_size == (uint) (inx & 1));
                    assert(results.u.t.data_size > (strlen(send_buffer) + 14));
                    assert(results.u.t.errm == RT_DATA_RCVD); // data
                    printf("%s\n", recv_buffer);
                    break;
                }
            }
        } else {
            sprintf(send_buffer, "hello, greetings from server %s, inx=%d",
                    my_name, inx);
            send_len = (int) strlen(send_buffer) + 1;
            ferr = XMSG_LINK_(TPT_REF(phandle),                     // phandle
                              &msgid,                      // msgid
                              send_buffer2,                // reqctrl
                              (ushort) (inx & 1),          // reqctrlsize
                              recv_buffer3,                // replyctrl
                              1,                           // replyctrlmax
                              send_buffer,                 // reqdata
                              (ushort) send_len,           // reqdatasize
                              recv_buffer,                 // replydata
                              BUFSIZ,                      // replydatamax
                              0,                           // linkertag
                              0,                           // pri
                              0,                           // xmitclass
                              0);                          // linkopts
            util_check("XMSG_LINK_", ferr);
            do {
                lerr = XWAIT(LREQ, -1);
                lerr = XMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
            } while (lerr == XSRETYPE_NOWORK);
            ferr = XMSG_READCTRL_(sre.sre_msgId,  // msgid
                                  recv_buffer2,   // reqctrl
                                  1);             // bytecount
            util_check("XMSG_READCTRL_", ferr);
            ferr = XMSG_READDATA_(sre.sre_msgId,  // msgid
                                  recv_buffer,    // reqctrl
                                  BUFSIZ);        // bytecount
            util_check("XMSG_READDATA_", ferr);
            strcat(recv_buffer, "- reply from ");
            strcat(recv_buffer, my_name);
            len = (int) strlen(recv_buffer) + 1;
            XMSG_REPLY_(sre.sre_msgId,       // msgid
                        recv_buffer2,        // replyctrl
                        sre.sre_reqCtrlSize, // replyctrlsize
                        recv_buffer,         // replydata
                        (ushort) len,        // replydatasize
                        0,                   // errorclass
                        NULL);               // newphandle
            ferr = XMSG_BREAK_(msgid, results.u.s, TPT_REF(phandle));
            util_check("XMSG_BREAK_", ferr);
        }
    }

    if (client) {
        ferr = msg_mon_close_process(TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_process_close();
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
