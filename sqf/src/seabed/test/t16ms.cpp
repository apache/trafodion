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
#include "seabed/thread.h"
#include "seabed/trace.h"

#include "ml.h"
#include "tchkfe.h"
#include "tchkos.h"
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

int     last = 99;
int     count = 0;

SB_Ts_Queue            free_q("test-free-ts");
char                   my_name[BUFSIZ];
SB_Sig_Queue           work_q("test-work-sig", false);

typedef struct  {
    SB_ML_Type     link;
    bool           fin;
    MS_SRE         sre;
} Test_SRE;
Test_SRE sres[100];
enum { MAX_THR = 4 };


bool server() {
    int             ferr;
    int             len;
    int             lerr;
    Util_AA<char>   recv_buffer(BUFSIZ);
    Util_AA<short>  recv_buffer2(BUFSIZ);

    if (count > last)
        return false;

    Test_SRE *sre = (Test_SRE *) free_q.remove();
    if (sre == NULL) {
        printf("woops\n");
    }
    assert(sre != NULL);
    sre->fin = false;
    do {
        if (count > last) {
            free_q.add(&sre->link);
            return false;
        }
        lerr = XWAIT(LREQ, -2);
        TEST_CHK_WAITIGNORE(lerr);
        lerr = XMSG_LISTEN_((short *) &sre->sre,    // sre
                            0,                      // listenopts
                            0);                     // listenertag
        if (lerr == XSRETYPE_NOWORK)
            util_time_sleep_us(1000);
    } while (lerr == XSRETYPE_NOWORK);
    ferr = XMSG_READCTRL_(sre->sre.sre_msgId,     // msgid
                          &recv_buffer2,          // reqctrl
                          3);                     // bytecount
    util_check("XMSG_READCTRL_", ferr);
    ferr = XMSG_READDATA_(sre->sre.sre_msgId,     // msgid
                          &recv_buffer,           // reqdata
                          BUFSIZ);                // bytecount
    util_check("XMSG_READDATA_", ferr);
    strcat(&recv_buffer, "- reply from ");
    strcat(&recv_buffer, my_name);
    len = 10;
    XMSG_REPLY_(sre->sre.sre_msgId,          // msgid
                &recv_buffer2,               // replyctrl
                sre->sre.sre_reqCtrlSize,    // replyctrlsize
                &recv_buffer,                // replydata
                (ushort) len,                // replydatasize
                0,                           // errorclass
                NULL);                       // newphandle
    free_q.add(&sre->link);
    return true;
}

void *server_thr(void *arg) {
    arg = arg; // no-warn
    for (;;) {
        if (server()) {
            free_q.lock();
            count++;
            printf("server-count=%d\n", count);
            free_q.unlock();
        }
        if (count > last) {
            printf("thread exit\n");
            break;
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    bool               client = false;
    int                ferr;
    int                msgid1;
    int                msgid2;
    int                oid;
    char               recv_buffer[BUFSIZ];
    short              recv_buffer3[BUFSIZ];
    RT                 results;
    TPT_DECL          (phandle);
    char               send_buffer[BUFSIZ];
    short              send_buffer2[BUFSIZ];
    int                send_len;
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
        for (int tinx = 0; tinx < MAX_THR; tinx++) {
            char name[10];
            sprintf(name, "s%d", tinx);
            thr[tinx] = new SB_Thread::Thread(server_thr, name);
        }
        for (int inx = 0; inx < 100; inx++)
            free_q.add(&sres[inx].link);
        for (int tinx = 0; tinx < MAX_THR; tinx++)
            thr[tinx]->start();
    }
    for (int inx = 0; inx <= last; inx += 2) {
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
                              (ushort) ((inx + 1) & 3),    // reqctrlsize
                              recv_buffer3,                // replyctrl
                              (ushort) 3,                  // replyctrlmax
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
            assert(results.u.t.data_size == 10);
            assert(results.u.t.errm == RT_DATA_RCVD); // data
            ferr = XMSG_BREAK_(msgid2, results.u.s, TPT_REF(phandle));
            util_check("XMSG_BREAK_", ferr);
            assert(results.u.t.ctrl_size == (uint) ((inx + 1) & 3));
            assert(results.u.t.data_size == 10);
            assert(results.u.t.errm == RT_DATA_RCVD); // data
            if (inx == last)
                printf("%s\n", recv_buffer);
        }
    }
    if (!client) {
        for (int tinx = 0; tinx < MAX_THR; tinx++) {
            void *res;
            status = thr[tinx]->join(&res);
            TEST_CHK_STATUSOK(status);
            delete thr[tinx];
            printf("joined with server %d\n", tinx); fflush(stdout);
        }
        printf("going through shutdown\n"); fflush(stdout);
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
