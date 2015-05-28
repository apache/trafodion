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

enum { MAX_TH = 2 };

int              cp = 0;
SB_Thread::Mutex cpm;
int              maxcp = 1;
bool             verbose = false;


void *thread_list(void *arg) {
    int            lerr;
    int            limit;
    int            ferr;
    bool           mon_msg;
    Util_AA<char>  recv_buffer(40000);
    MS_SRE         sre;
    int            status;

    arg = arg; // touch
    limit = maxcp * 3;
    while (cp < limit) {
        do {
            lerr = XWAIT(LREQ, 10);
            TEST_CHK_WAITIGNORE(lerr);
            if (cp >= limit)
                return NULL;
            lerr = XMSG_LISTEN_((short *) &sre, // sre
                                0,              // listenopts
                                0);             // listenertag
        } while (lerr == XSRETYPE_NOWORK);
        ferr = XMSG_READDATA_(sre.sre_msgId,  // msgid
                              &recv_buffer,   // reqdata
                              40000);         // bytecount
        util_check("XMSG_READDATA_", ferr);
        mon_msg = (sre.sre_flags & XSRE_MON);
        status = cpm.lock();
        TEST_CHK_STATUSOK(status);
        cp++;
        status = cpm.unlock();
        TEST_CHK_STATUSOK(status);
        if (verbose) {
            if (mon_msg)
                printf("%s: received mon-msg, cp=%d\n",
                       SB_Thread::Sthr::self_name(), cp);
            else
                printf("%s: received WR, msg=%s, cp=%d\n",
                       SB_Thread::Sthr::self_name(), &recv_buffer, cp);
        }
        XMSG_REPLY_(sre.sre_msgId,  // msgid
                    NULL,           // replyctrl
                    0,              // replyctrlsize
                    NULL,           // replydata
                    0,              // replydatasize
                    0,              // errorclass
                    NULL);          // newphandle
    }
    if (verbose)
        printf("%s: exiting cp=%d\n",
               SB_Thread::Sthr::self_name(), cp);
    return NULL;
}

void *thread_list_fun(void *arg) {
    return thread_list(arg);
}

int main(int argc, char *argv[]) {
    bool                      client = false;
    bool                      event = false;
    int                       ferr;
    MS_Mon_Process_Info_Type  info;
    int                       msgid;
    char                      my_name[BUFSIZ];
    char                      name[BUFSIZ];
    int                       oid;
    TPT_DECL                 (phandle);
    char                      recv_buffer[BUFSIZ];
    void                     *result;
    RT                        results;
    char                      send_buffer[BUFSIZ];
    int                       send_len;
    int                       status;
    int                       t;
    SB_Thread::Thread        *thr_list[MAX_TH];
    TAD                       zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-event",     TA_Bool, TA_NOMAX,    &event     },
      { "-maxcp",     TA_Int,  TA_NOMAX,    &maxcp     },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    msfs_util_init(&argc, &argv, msg_debug_hook);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    msg_mon_enable_mon_messages(true);
    util_gethostname(my_name, sizeof(my_name));

    ferr = msg_mon_get_process_info_detail(NULL, &info);
    TEST_CHK_FEOK(ferr);
    printf("client=%d, event=%d, maxcp=%d, v=%d\n",
           client, event, maxcp, verbose);
    if (client) {
        if (event) {
            ferr = msg_mon_event_wait(1, NULL, NULL);
            TEST_CHK_FEOK(ferr);
        }
        if (verbose)
            printf("%s: opening $srv\n", info.process_name);
        ferr = msg_mon_open_process((char *) "$srv",      // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
        sprintf(send_buffer, "hello, greetings from %s", info.process_name);
        send_len = (int) strlen(send_buffer) + 1;
        ferr = XMSG_LINK_(TPT_REF(phandle),            // phandle
                          &msgid,                      // msgid
                          NULL,                        // reqctrl
                          0,                           // reqctrlsize
                          NULL,                        // replyctrl
                          0,                           // replyctrlmax
                          send_buffer,                 // reqdata
                          (ushort) send_len,           // reqdatasize
                          recv_buffer,                 // replydata
                          BUFSIZ,                      // replydatamax
                          0,                           // linkertag
                          0,                           // pri
                          0,                           // xmitclass
                          0);                          // linkopts
        util_check("XMSG_LINK_", ferr);
        ferr = XMSG_BREAK_(msgid,
                           results.u.s,
                           TPT_REF(phandle));
        util_check("XMSG_BREAK_", ferr);
    } else {
        for (t = 0; t < MAX_TH; t++) {
            sprintf(name, "list%d", t);
            thr_list[t] = new SB_Thread::Thread(thread_list_fun, name);
            thr_list[t]->start();
        }
        for (t = 0; t < MAX_TH; t++) {
            status = thr_list[t]->join(&result);
            TEST_CHK_STATUSOK(status);
            delete thr_list[t];
        }
    }
    if (verbose)
        printf("%s: shutting down\n", info.process_name);
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
