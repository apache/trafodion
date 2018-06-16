//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "seabed/atomic.h"
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


SB_Atomic_Int  count;
bool           first = true;
int            loop = 10;
int            maxcp = 1;
int            maxs = 1;
char           my_name[BUFSIZ];
struct timeval t_elapsed;
struct timeval t_start;
struct timeval t_stop;
Server_Thread *thrs[MAX_THR_S];
bool           verbose = false;


void Server_Thread::work() {
    bool   done;
    int    lerr;
    int    msgs = 0;
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
        if (first) {
            first = false;
            util_time_timer_start(&t_start);
        }
        if (verbose)
            printf("server-%d thread processing msgid=%d\n",
                   whoami, sre.sre_msgId);
        if (sre.sre_flags & XSRE_MON) {
        } else if (sre.sre_reqDataSize == 0)
            done = true;
        else {
            msgs++;
            count.add_val(1);
            int lcount = count.read_val();
            if (lcount == (maxcp * loop)) {
                util_time_timer_start(&t_stop);
                util_time_elapsed(&t_start, &t_stop, &t_elapsed);
                print_elapsed("", &t_elapsed);
                print_rate(false, "", maxcp * loop, 1, &t_elapsed, 0.0);
            }
            usleep(1);
        }
        XMSG_REPLY_(sre.sre_msgId,  // msgid
                    NULL,           // replyctrl
                    0,              // replyctrlsize
                    recv_buffer,    // replydata
                    0,              // replydatasize
                    0,              // errorclass
                    NULL);          // newphandle
    }
    printf("server-%d thread stopping, msgs=%d\n", whoami, msgs);
}

void *server_thr(void *arg) {
    Server_Thread *thr = (Server_Thread *) arg; // cast
    thr->work();
    return NULL;
}

int main(int argc, char *argv[]) {
    int  ferr;
    int  inx;
    int  status;
    TAD  zargs[] = {
      { "-inst",      TA_Next, TA_NOMAX,    NULL       },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-maxcp",     TA_Int,  TA_NOMAX,    &maxcp     },
      { "-maxs",      TA_Int,  TA_NOMAX,    &maxs      },
      { "-maxsp",     TA_Next, TA_NOMAX,    NULL       },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(false);
    ferr = msg_mon_process_startup(true);  // system messages?
    TEST_CHK_FEOK(ferr);
    util_gethostname(my_name, sizeof(my_name));
    count.set_val(0);

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
    ferr = msg_mon_process_close();
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(false);
    return 0;
}
