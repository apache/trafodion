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
#include "seabed/thread.h"
#include "seabed/trace.h"

#include "tchkfe.h"
#include "tchkos.h"
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

char      my_name[BUFSIZ];
TPT_DECL (phandle);

enum { MAX_MSGS_PER_THR =   5 };
enum { MAX_SENDS        =   2 };
enum { MAX_THR          =  10 };
enum { MAX_MSGS         =  MAX_MSGS_PER_THR * MAX_THR * MAX_SENDS };


void client(int clinx, char *name, long id, int inx) {
    int            ferr;
    int            lerr;
    bool           ok;
    int            match;
    Util_AA<int>   msgid(MAX_SENDS);
    Util_AA<char>  recv_buffer(MAX_SENDS * BUFSIZ);
    Util_AA<short> recv_buffer3(MAX_SENDS * BUFSIZ);
    RT             results;
    int            send;
    Util_AA<char>  send_buffer(MAX_SENDS * BUFSIZ);
    Util_AA<short> send_buffer2(MAX_SENDS * BUFSIZ);
    int            send_len;
    MS_SRE_LDONE   sre_ldone;

    for (send = 0; send < MAX_SENDS; send++) {
        sprintf(&send_buffer.ip_v[send * BUFSIZ],
                "hello, greetings from %s, name=%s, id=%ld, inx=%d, send=%d",
                my_name, name, id, inx, send);
        while (clinx >= 0) {
            strcat(&send_buffer.ip_v[send * BUFSIZ], "!");
            clinx--;
        }
        send_len = (int) strlen(&send_buffer.ip_v[send * BUFSIZ]) + 1;
        ferr =
          XMSG_LINK_(TPT_REF(phandle),                         // phandle
                            &msgid.ip_v[send],                        // msgid
                            &send_buffer2.ip_v[send * BUFSIZ],        // reqctrl
                            (ushort) (inx & 1),                       // reqctrlsize
                            &recv_buffer3.ip_v[send * BUFSIZ],        // replyctrl
                            1,                                        // replyctrlmax
                            &send_buffer.ip_v[send * BUFSIZ],         // reqdata
                            (ushort) send_len,                        // reqdatasize
                            &recv_buffer.ip_v[send * BUFSIZ],         // replydata
                            BUFSIZ,                                   // replydatamax
                            (SB_Tag_Type)                             // linkertag
                              &send_buffer.ip_v[send * BUFSIZ],
                            0,                                        // pri
                            0,                                        // xmitclass
                            XMSG_LINK_LDONEQ);                        // linkopts
        util_check("XMSG_LINK_", ferr);
    }
    for (send = 0; send < MAX_SENDS;) {
        lerr = XWAIT(LDONE, -1);
        TEST_CHK_WAITIGNORE(lerr);
        for (;;) {
            lerr = XMSG_LISTEN_((short *) &sre_ldone, // sre
                                0,                    // listenopts
                                0);                   // listenertag
            if (lerr == XSRETYPE_NOWORK)
                break;
            send++;
            assert(lerr == XSRETYPE_LDONE);
            ok = false;
            for (match = 0; match < MAX_SENDS; match++) {
                if (sre_ldone.sre_linkTag ==
                    (SB_Tag_Type) &send_buffer.ip_v[match * BUFSIZ]) {
                    ok = true;
                    break;
                }
            }
            assert(ok);
            ferr = XMSG_BREAK_(msgid.ip_v[match],
                               results.u.s,
                               TPT_REF(phandle));
            util_check("XMSG_BREAK_", ferr);
            assert(results.u.t.ctrl_size == (uint) (inx & 1));
            assert(results.u.t.data_size >
                   (strlen(&send_buffer.ip_v[match * BUFSIZ]) + 14));
            assert(results.u.t.errm == RT_DATA_RCVD); // data
            printf("%s\n", &recv_buffer.ip_v[match * BUFSIZ]);
        }
    }
}

void *client_thr(void *arg) {
    long id = SB_Thread::Sthr::self_id();
    SB_Thread::Thread *thr = (SB_Thread::Thread *) arg;
    char *name = thr->get_name();
    int clinx;
    sscanf(&name[1], "%d", &clinx);
#ifdef USE_EVENT_REG
    proc_event_register(LDONE);
#endif
    for (int inx = 0; inx < 5; inx++)
        client(clinx, name, id, inx);
    return NULL;
}

int main(int argc, char *argv[]) {
    bool               client = false;
    int                ferr;
    int                inx;
    int                len;
    int                lerr;
    int                oid;
    char               recv_buffer[BUFSIZ];
    short              recv_buffer2[BUFSIZ];
    MS_SRE             sre;
    int                status;
    SB_Thread::Thread *thr[MAX_THR];
    TAD                zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    if (client)
        printf("threads=%d, msgs-per-thread=%d, msgs=%d\n",
               MAX_THR, MAX_MSGS_PER_THR, MAX_MSGS);
    ferr = msg_mon_process_startup(!client); // system messages
    TEST_CHK_FEOK(ferr);
    if (client) {
        ferr = msg_mon_open_process((char *) "$srv",      //  name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
    }
    util_gethostname(my_name, sizeof(my_name));

    if (client) {
        for (inx = 0; inx < MAX_THR; inx++) {
            char name[10];
            sprintf(name, "c%d", inx);
            thr[inx] = new SB_Thread::Thread(client_thr, name);
        }
        for (inx = 0; inx < MAX_THR; inx++)
            thr[inx]->start();
        for (inx = MAX_THR - 1; inx >= 0; inx--) {
            void *res;
            status = thr[inx]->join(&res);
            TEST_CHK_STATUSOK(status);
            printf("joined with client %d\n", inx);
        }
        for (inx = 0; inx < MAX_THR; inx++)
            delete thr[inx];
    } else {
        for (inx = 0; inx < MAX_MSGS;) {
            lerr = XWAIT(LREQ, -1);
            TEST_CHK_WAITIGNORE(lerr);
            do {
                lerr = XMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
                if (lerr == XSRETYPE_IREQ) {
                    inx++;
                    ferr = XMSG_READCTRL_(sre.sre_msgId,    // msgid
                                          recv_buffer2,     // reqctrl
                                          1);               // bytecount
                    util_check("XMSG_READCTRL_", ferr);
                    ferr = XMSG_READDATA_(sre.sre_msgId,    // msgid
                                          recv_buffer,      // reqdata
                                          BUFSIZ);          // bytecount
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
                } else {
                    assert(lerr == XSRETYPE_NOWORK);
                }
            } while (lerr != XSRETYPE_NOWORK);
        }
    }

    if (client) {
        ferr = msg_mon_close_process(TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
    }
    printf("%s shutting down\n", client ? "client" : "server");
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
