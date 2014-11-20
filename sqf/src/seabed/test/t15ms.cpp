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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"
#include "seabed/thread.h"
#include "seabed/trace.h"

#include "ml.h"
#include "tchkfe.h"
#include "tchkos.h"
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

bool    debug = false;
int     last = 999;
int     count = 0;

char                   my_name[BUFSIZ];
SB_Ts_Queue            free_q((char *) "freeQ");
SB_Sig_Queue           work_q((char *) "workQ", false);

typedef struct  {
    SB_ML_Type     link;
    bool           fin;
    MS_SRE         sre;
} Test_SRE;
enum { MAX_SRES = 6 };
Test_SRE sres[MAX_SRES];


void server(Test_SRE *sre) {
    int             ferr;
    int             len;
    Util_AA<char>   recv_buffer(BUFSIZ);
    Util_AA<short>  recv_buffer2(BUFSIZ);

    ferr = XMSG_READCTRL_(sre->sre.sre_msgId,     // msgid
                          &recv_buffer2,          // reqctrl
                          1);                     // bytecount
    util_check("XMSG_READCTRL_", ferr);
    ferr = XMSG_READDATA_(sre->sre.sre_msgId,     // msgid
                          &recv_buffer,           // reqdata
                          BUFSIZ);                // bytecount
    util_check("XMSG_READDATA_", ferr);
    strcat(&recv_buffer, "- reply from ");
    strcat(&recv_buffer, my_name);
    len = 100;
    XMSG_REPLY_(sre->sre.sre_msgId,          // msgid
                &recv_buffer2,               // replyctrl
                sre->sre.sre_reqCtrlSize,    // replyctrlsize
                &recv_buffer,                // replydata
                (ushort) len,                // replydatasize
                0,                           // errorclass
                NULL);                       // newphandle
}

void *server_thr(void *arg) {
    arg = arg; // no-warn
    for (;;) {
        Test_SRE *sre = (Test_SRE *) work_q.remove();
        if (sre->fin)
            break;
        server(sre);
        free_q.add(&sre->link);
    }
    return NULL;
}

char  recv_buffer[BUFSIZ];
short recv_buffer3[BUFSIZ];
char  send_buffer[BUFSIZ];
short send_buffer2[BUFSIZ];

int main(int argc, char *argv[]) {
    bool       client = false;
    int        ferr;
    int        inx;
    int        lerr;
    int        msgid1;
    int        msgid2;
    int        oid;
    RT         results;
    TPT_DECL  (phandle);
    int        send_len;
    int        status;
    TAD        zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-debug",     TA_Bool, TA_NOMAX,    &debug     },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    SB_Thread::Thread s1(server_thr, "s1");
    SB_Thread::Thread s2(server_thr, "s2");

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client); // system messages
    TEST_CHK_FEOK(ferr);
    if (client) {
        ferr = msg_mon_open_process((char *) "$srv",      // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
    }
    util_gethostname(my_name, sizeof(my_name));

    if (!client) {
        for (inx = 0; inx < MAX_SRES; inx++)
            free_q.add(&sres[inx].link);
        s1.start();
        s2.start();
    }
    for (inx = 0; inx <= last; inx += 2) {
        if (client) {
            printf("inx=%d\n", inx);
            fflush(stdout);
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            send_len = (int) strlen(send_buffer) + 1;
            ferr = XMSG_LINK_(TPT_REF(phandle),                     // phandle
                              &msgid1,                     // msgid
                              send_buffer2,                // reqctrl
                              (ushort) (inx & 1),          // reqctrlsize
                              recv_buffer3,                // replyctrl
                              (ushort) 1,                  // replyctrlmax
                              send_buffer,                 // reqdata
                              (ushort) send_len,           // reqdatasize
                              recv_buffer,                 // replydata
                              (ushort) BUFSIZ,             // replydatamax
                              0,                           // linkertag
                              0,                           // pri
                              0,                           // xmitclass
                              0);                          // linkopts
            util_check("XMSG_LINK_", ferr);
            ferr = XMSG_LINK_(TPT_REF(phandle),                     // phandle
                              &msgid2,                     // msgid
                              send_buffer2,                // reqctrl
                              (ushort) (inx & 1),          // reqctrlsize
                              recv_buffer3,                // replyctrl
                              (ushort) 1,                  // replyctrlmax
                              send_buffer,                 // reqdata
                              (ushort) send_len,           // reqdatasize
                              recv_buffer,                 // replydata
                              (ushort) BUFSIZ,             // replydatamax
                              0,                           // linkertag
                              0,                           // pri
                              0,                           // xmitclass
                              0);                          // linkopts
            util_check("XMSG_LINK_", ferr);
            ferr = XMSG_BREAK_(msgid1, results.u.s, TPT_REF(phandle));
            util_check("XMSG_BREAK_", ferr);
            assert(results.u.t.ctrl_size == (uint) (inx & 1));
            assert(results.u.t.data_size == 100);
            assert(results.u.t.errm == RT_DATA_RCVD); // data
            ferr = XMSG_BREAK_(msgid2, results.u.s, TPT_REF(phandle));
            util_check("XMSG_BREAK_", ferr);
            assert(results.u.t.ctrl_size == (uint) (inx & 1));
            assert(results.u.t.data_size == 100);
            assert(results.u.t.errm == RT_DATA_RCVD); // data
            if (inx == last)
                printf("%s\n", recv_buffer);
        }
    }
    if (!client) {
        Test_SRE *sre = NULL;
        for (inx = 0; inx <= last;) {
            lerr = XWAIT(LREQ, -1);
            TEST_CHK_WAITIGNORE(lerr);
            do {
                if (sre == NULL) {
                    sre = (Test_SRE *) free_q.remove();
                    assert(sre != NULL);
                }
                sre->fin = false;
                lerr = XMSG_LISTEN_((short *) &sre->sre,    // sre
                                    0,                      // listenopts
                                    0);                     // listenertag
                if (lerr == XSRETYPE_IREQ) {
                    assert(sre->sre.sre_msgId > 0);
                    work_q.add(&sre->link);
                    sre = NULL;
                    inx++;
                }
            } while (lerr == XSRETYPE_IREQ);
        }
        Test_SRE *quit_sre = (Test_SRE *) free_q.remove();
        quit_sre->fin = true;
        work_q.add(&quit_sre->link);
        util_time_sleep_ms(0);
        quit_sre = (Test_SRE *) free_q.remove();
        quit_sre->fin = true;
        work_q.add(&quit_sre->link);
        util_time_sleep_ms(0);

        void *res;
        status = s1.join(&res);
        TEST_CHK_STATUSOK(status);
        printf("joined with server 1\n");
        status = s2.join(&res);
        TEST_CHK_STATUSOK(status);
        printf("joined with server 2\n");
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
