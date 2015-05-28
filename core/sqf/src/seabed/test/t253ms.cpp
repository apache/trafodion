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

#include "tchkfe.h"
#include "tchkos.h"
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

enum { MAX_C = 100 };

char my_name[BUFSIZ];
char recv_buffer[40000];
char send_buffer[40000];

void *thread_open_fun(void *arg) {
    int        ferr;
    int        oid;
    TPT_DECL  (phandle);

    arg = arg; // touch
    ferr = msg_mon_open_process((char *) "$srv",      // name
                                TPT_REF(phandle),
                                &oid);
    TEST_CHK_FEOK(ferr);

    return NULL;
}

void open_server(int maxc) {
    int                inx;
    char               name[20];
    void              *result;
    int                status;
    SB_Thread::Thread *thr_open[MAX_C];

    for (inx = 0; inx < maxc; inx++) {
        sprintf(name, "open-%d", inx);
        thr_open[inx] = new SB_Thread::Thread(thread_open_fun, name);
        thr_open[inx]->start();
    }
    for (inx = 0; inx < maxc; inx++) {
        status = thr_open[inx]->join(&result);
        TEST_CHK_STATUSOK(status);
    }
    for (inx = 0; inx < maxc; inx++)
        delete thr_open[inx];
}

int main(int argc, char *argv[]) {
    bool       client = false;
    int        ferr;
    int        inx;
    int        lerr;
    int        maxc = 1;
    MS_SRE     sre;
    TAD        zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-maxc",      TA_Int,  MAX_C,       &maxc      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    if (client) {
    }
    util_gethostname(my_name, sizeof(my_name));

    if (client) {
        open_server(maxc);
    } else {
        msg_mon_enable_mon_messages(true);
        for (inx = 0; inx < maxc; inx++) {
            do {
                lerr = XWAIT(LREQ, -1);
                TEST_CHK_WAITIGNORE(lerr);
                lerr = XMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
            } while (lerr == XSRETYPE_NOWORK);
            XMSG_REPLY_(sre.sre_msgId,  // msgid
                        NULL,           // replyctrl
                        0,              // replyctrlsize
                        NULL,           // replydata
                        0,              // replydatasize
                        0,              // errorclass
                        NULL);          // newphandle
        }
    }
    sleep(2);  // don't exit immediately
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
