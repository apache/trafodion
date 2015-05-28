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

#include "tchkfe.h"
#include "tchkos.h"
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

int       loop = 10;
char      my_name[BUFSIZ];
TPT_DECL (phandle);


void *thread_send(void *arg) {
    int             ferr;
    int             inx;
    int             msgid;
    Util_AA<char>   recv_buffer(40000);
    RT              results;
    Util_AA<char>   send_buffer(40000);

#ifdef USE_EVENT_REG
    proc_event_register(LDONE);
#endif
    arg = arg; // touch
    for (inx = 0; inx < loop; inx++) {
        sprintf(&send_buffer, "hello, greetings from %s, inx=%d",
                my_name, inx);
        ferr = XMSG_LINK_(TPT_REF(phandle),            // phandle
                          &msgid,                      // msgid
                          NULL,                        // reqctrl
                          0,                           // reqctrlsize
                          NULL,                        // replyctrl
                          0,                           // replyctrlmax
                          &send_buffer,                // reqdata
                          39000,                       // reqdatasize
                          &recv_buffer,                // replydata
                          40000,                       // replydatamax
                          0,                           // linkertag
                          0,                           // pri
                          0,                           // xmitclass
                          XMSG_LINK_LDONEQ);           // linkopts
        util_check("XMSG_LINK_", ferr);
        ferr = XMSG_BREAK_(msgid, results.u.s, TPT_REF(phandle));
        util_check("XMSG_BREAK_", ferr);
        assert(results.u.t.ctrl_size == 0);
        assert(results.u.t.errm == RT_DATA_RCVD); // data
        printf("%s\n", &recv_buffer);
    }
    return NULL;
}

void *thread_send_fun(void *arg) {
    return thread_send(arg);
}

void *thread_wait(void *arg) {
    int lerr;

    arg = arg; // touch
#ifdef USE_EVENT_REG
    proc_event_register(LREQ);
    proc_event_register(LDONE);
#endif
    for (int inx = 0; inx < loop / 5; inx++) {
        lerr = XWAIT(LREQ | LDONE, -1);
        TEST_CHK_WAITIGNORE(lerr);
        printf("wait-thread woke up\n");
    }
    return NULL;
}

void *thread_wait_fun(void *arg) {
    return thread_wait(arg);
}

char recv_buffer[40000];

int main(int argc, char *argv[]) {
    bool               client = false;
    int                ferr;
    int                inx;
    int                len;
    int                lerr;
    int                oid;
    void              *result;
    MS_SRE             sre;
    int                status;
    SB_Thread::Thread *thr_send;
    SB_Thread::Thread *thr_wait;
    TAD                zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    ferr = proc_set_process_completion();
    TEST_CHK_FEOK(ferr);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    if (client) {
        ferr = msg_mon_open_process((char *) "$srv",      // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
    }
    util_gethostname(my_name, sizeof(my_name));

    if (client) {
        thr_wait = new SB_Thread::Thread(thread_wait_fun, "wait");
        thr_wait->start();
        thr_send = new SB_Thread::Thread(thread_send_fun, "send");
        thr_send->start();
        status = thr_send->join(&result);
        TEST_CHK_STATUSOK(status);
        status = thr_wait->join(&result);
        TEST_CHK_STATUSOK(status);
        delete thr_send;
        delete thr_wait;
    } else {
        for (inx = 0; inx < loop; inx++) {
            do {
                lerr = XWAIT(LREQ, -1);
                TEST_CHK_WAITIGNORE(lerr);
                lerr = XMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
            } while (lerr == XSRETYPE_NOWORK);
            ferr = XMSG_READCTRL_(sre.sre_msgId,  // msgid
                                  NULL,           // reqctrl
                                  0);             // bytecount
            util_check("XMSG_READCTRL_", ferr);
            ferr = XMSG_READDATA_(sre.sre_msgId,  // msgid
                                  recv_buffer,    // reqdata
                                  40000);         // bytecount
            util_check("XMSG_READDATA_", ferr);
            strcat(recv_buffer, "- reply from ");
            strcat(recv_buffer, my_name);
            len = (int) strlen(recv_buffer) + 1;
            len = len; // touch
            XMSG_REPLY_(sre.sre_msgId,  // msgid
                        NULL,           // replyctrl
                        0,              // replyctrlsize
                        recv_buffer,    // replydata
                        39000,          // replydatasize
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
