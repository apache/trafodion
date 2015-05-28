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

enum {
    MAX_THR_S = 100
};

class Server_Thread : public SB_Thread::Thread {
public:
    Server_Thread(Function fun, const char *name, int inx)
    : SB_Thread::Thread(fun, name), whoami(inx) {
    }
    void work();

private:
    int whoami;
};


bool           delay = false;
int            maxs = 1;
char           my_name[BUFSIZ];
Server_Thread *thrs[MAX_THR_S];
bool           verbose = false;


void Server_Thread::work() {
    bool   done;
    int    ferr;
    int    lerr;
    char   recv_buffer[40000];
    MS_SRE sre;

    if (verbose)
        printf("server-%d thread starting\n", whoami);
    done = false;
    while (!done) {
        do {
            lerr = XWAIT(LREQ, -1);
            TEST_CHK_WAITIGNORE(lerr);
            lerr = XMSG_LISTEN_((short *) &sre, // sre
                                0,              // listenopts
                                0);             // listenertag
        } while (lerr == XSRETYPE_NOWORK);
        if (verbose)
            printf("server-%d thread processing msgid=%d\n",
                   whoami, sre.sre_msgId);
        ferr = XMSG_READCTRL_(sre.sre_msgId,  // msgid
                              NULL,           // reqctrl
                              0);             // bytecount
        util_check("XMSG_READCTRL_", ferr);
        ferr = XMSG_READDATA_(sre.sre_msgId,  // msgid
                              recv_buffer,    // reqdata
                              40000);         // bytecount
        util_check("XMSG_READDATA_", ferr);
        if (sre.sre_flags & XSRE_MON) {
        } else if (sre.sre_reqDataSize > 0) {
            strcat(recv_buffer, "- reply from ");
            strcat(recv_buffer, my_name);
        } else
            done = true;
        XMSG_REPLY_(sre.sre_msgId,  // msgid
                    NULL,           // replyctrl
                    0,              // replyctrlsize
                    recv_buffer,    // replydata
                    39000,          // replydatasize
                    0,              // errorclass
                    NULL);          // newphandle
        if (delay)
            sleep(1);
    }
    if (verbose)
        printf("server-%d thread stopping\n", whoami);
}

void *server_thr(void *arg) {
    Server_Thread *thr = (Server_Thread *) arg; // cast
    thr->work();
    return NULL;
}

int main(int argc, char *argv[]) {
    bool       client = false;
    int        ferr;
    int        inx;
    int        loop = 10;
    int        msgid;
    int        oid;
    TPT_DECL  (phandle);
    char       recv_buffer[40000];
    RT         results;
    char       send_buffer[40000];
    int        status;
    TAD        zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-delay",     TA_Bool, TA_NOMAX,    &delay     },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-maxs",      TA_Int,  TA_NOMAX,    &maxs      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    util_gethostname(my_name, sizeof(my_name));

    if (client) {
        ferr = msg_mon_open_process((char *) "$srv",      // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            ferr = XMSG_LINK_(TPT_REF(phandle),            // phandle
                              &msgid,                      // msgid
                              NULL,                        // reqctrl
                              0,                           // reqctrlsize
                              NULL,                        // replyctrl
                              0,                           // replyctrlmax
                              send_buffer,                 // reqdata
                              39000,                       // reqdatasize
                              recv_buffer,                 // replydata
                              40000,                       // replydatamax
                              0,                           // linkertag
                              0,                           // pri
                              0,                           // xmitclass
                              0);                          // linkopts
            util_check("XMSG_LINK_", ferr);
            ferr = XMSG_BREAK_(msgid, results.u.s, TPT_REF(phandle));
            util_check("XMSG_BREAK_", ferr);
            assert(results.u.t.ctrl_size == 0);
            assert(results.u.t.errm == RT_DATA_RCVD); // data
            printf("%s\n", recv_buffer);
        }
        for (inx = 0; inx < maxs; inx++) {
            ferr = XMSG_LINK_(TPT_REF(phandle),            // phandle
                              &msgid,                      // msgid
                              NULL,                        // reqctrl
                              0,                           // reqctrlsize
                              NULL,                        // replyctrl
                              0,                           // replyctrlmax
                              send_buffer,                 // reqdata
                              0,                           // reqdatasize
                              recv_buffer,                 // replydata
                              0,                           // replydatamax
                              0,                           // linkertag
                              0,                           // pri
                              0,                           // xmitclass
                              0);                          // linkopts
            util_check("XMSG_LINK_", ferr);
            ferr = XMSG_BREAK_(msgid, results.u.s, TPT_REF(phandle));
            util_check("XMSG_BREAK_", ferr);
            printf("fin %d\n", inx);
        }
    } else {
        msg_mon_enable_mon_messages(true);
        for (inx = 0; inx < maxs; inx++) {
            char name[10];
            sprintf(name, "s%d", inx);
            thrs[inx] = new Server_Thread(server_thr, name, inx);
        }
        for (inx = 0; inx < maxs; inx++) {
            thrs[inx]->start();
            SB_Thread::Sthr::yield();
        }
        for (inx = 0; inx < maxs; inx++) {
            void *res;
            status = thrs[inx]->join(&res);
            TEST_CHK_STATUSOK(status);
            printf("joined with server %d\n", inx);
        }
    }
    if (client) {
        ferr = msg_mon_close_process(TPT_REF(phandle));
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
