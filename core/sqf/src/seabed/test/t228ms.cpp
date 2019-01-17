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

class MyThread : public SB_Thread::Thread {
public:
    MyThread(Function fun, const char *name, int pinx);
    virtual ~MyThread();

    int inx;
};

enum        { MAX_OUT = 100 };
enum        { MAX_THR_S = 100 };
int           closes;
int           count;
int           loop = 1000;
int           maxmsg;
int           maxout = 1;
SB_Atomic_Int msg_count;
char          my_name[100];
char          recv_buffer[MAX_OUT][100];
unsigned int  seed;
char          send_buffer[MAX_OUT][200];
short         send_buffer2[MAX_OUT][100];
MyThread     *thrs[MAX_THR_S];
bool          verbose = false;

MyThread::MyThread(Function fun, const char *name, int pinx) :
    SB_Thread::Thread(fun, name), inx(pinx) {
}

MyThread::~MyThread() {
}

int rnd(int max) {
    int rndret = (int) (1.0 * max*rand_r(&seed)/(RAND_MAX+1.0));
    return rndret;
}

void *server_thr(void *arg) {
    int                 ferr;
    int                 lerr;
    int                 recv_len;
    BMS_SRE             sre;
    MyThread           *thr;
    int                 thrinx;

    thr = (MyThread *) arg;
    thrinx = thr->inx;
    // process requests
    for (;;) {
        do {
            lerr = XWAIT(LREQ, 100);
            lerr = BMSG_LISTEN_((short *) &sre, // sre
                                0,              // listenopts
                                0);             // listenertag
            if ((lerr == XSRETYPE_NOWORK) &&
                (msg_count.read_val() >= maxmsg))
                return NULL;
        } while (lerr == XSRETYPE_NOWORK);
        if (lerr == XSRETYPE_ABANDON)
            continue;
        if (sre.sre_flags & XSRE_MON) {
            ferr = XMSG_READDATA_(sre.sre_msgId,       // msgid
                                  recv_buffer[thrinx], // reqdata
                                  BUFSIZ);             // bytecount
            util_check("XMSG_READDATA_", ferr);
            MS_Mon_Msg *msg = (MS_Mon_Msg *) recv_buffer[thrinx];
            if (verbose)
                printf("server(%s): mon-msg type=%d\n", my_name, msg->type);
            if (msg->type == MS_MsgType_Close) {
                closes++;
                if (verbose)
                    printf("server(%s): closes=%d\n", my_name, closes);
            }
        } else {
            msg_count.add_val(1);
            if (verbose)
                printf("server(%s): msg-count=%d\n",
                       my_name, msg_count.read_val());
        }
        recv_len = rnd(BUFSIZ);
        BMSG_REPLY_(sre.sre_msgId,       // msgid
                    NULL,                // replyctrl
                    0,                   // replyctrlsize
                    recv_buffer[thrinx], // replydata
                    recv_len,            // replydatasize
                    0,                   // errorclass
                    NULL);               // newphandle
        if (closes)
            break;
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    bool                client = false;
    int                 ferr;
    int                 inxl;
    int                 inxo;
    int                 inxs;
    int                 maxs = 2;
    int                 msgid[MAX_OUT];
    bool                mq = false;
    int                 oid;
    TPT_DECL           (phandle);
    void               *res;
    RT                  results;
    const char         *serv = "$srv";
    int                 status;
    TAD                 zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-maxout",    TA_Int,  MAX_OUT,     &maxout    },
      { "-maxs",      TA_Int,  MAX_THR_S,   &maxs      },
      { "-mq",        TA_Bool, TA_NOMAX,    &mq        },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    ferr = msg_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(my_name, BUFSIZ);
    util_check("msg_mon_get_my_process_name", ferr);

    if (client) {
        ferr = XCONTROLMESSAGESYSTEM(XCTLMSGSYS_SETSENDLIMIT,
                                     XMAX_SETTABLE_SENDLIMIT);
        assert(ferr == XZFIL_ERR_OK);
        msg_count.set_val(0);
        for (inxl = 0; inxl < loop; inxl++) {
            ferr = msg_mon_open_process((char *) serv,       // name
                                        TPT_REF(phandle),
                                        &oid);
            TEST_CHK_FEOK(ferr);
            count = 0;
            for (inxo = 0; inxo < 2; inxo++) {
                ferr = BMSG_LINK_(TPT_REF(phandle),        // phandle
                                  &msgid[inxo],            // msgid
                                  NULL,                    // reqctrl
                                  0,                       // reqctrlsize
                                  send_buffer2[inxo],      // replyctrl
                                  24,                      // replyctrlmax
                                  send_buffer[inxo],       // reqdata
                                  0,                       // reqdatasize
                                  recv_buffer[inxo],       // replydata
                                  BUFSIZ,                  // replydatamax
                                  0,                       // linkertag
                                  0,                       // pri
                                  0,                       // xmitclass
                                  0);                      // linkopts
                util_check("XMSG_LINK_", ferr);
                msg_count.add_val(1);
            }
            ferr = XMSG_ABANDON_(msgid[0]);
            util_check("XMSG_ABANDON_", ferr);
            ferr = BMSG_LINK_(TPT_REF(phandle),        // phandle
                              &msgid[inxo],            // msgid
                              NULL,                    // reqctrl
                              0,                       // reqctrlsize
                              send_buffer2[inxo],      // replyctrl
                              24,                      // replyctrlmax
                              send_buffer[inxo],       // reqdata
                              0,                       // reqdatasize
                              recv_buffer[inxo],       // replydata
                              BUFSIZ,                  // replydatamax
                              0,                       // linkertag
                              0,                       // pri
                              0,                       // xmitclass
                              0);                      // linkopts
            util_check("XMSG_LINK_", ferr);
            msg_count.add_val(1);
            ferr = BMSG_BREAK_(msgid[inxo],
                               results.u.s,
                               TPT_REF(phandle));
            util_check("XMSG_BREAK_", ferr);
            ferr = XMSG_ABANDON_(msgid[1]);
            util_check("XMSG_ABANDON_", ferr);
            if (maxout < 4)
                maxout = 4;
            for (inxo = 3; inxo < maxout; inxo++) {
                ferr = BMSG_LINK_(TPT_REF(phandle),        // phandle
                                  &msgid[inxo],            // msgid
                                  NULL,                    // reqctrl
                                  0,                       // reqctrlsize
                                  send_buffer2[inxo],      // replyctrl
                                  60,                      // replyctrlmax
                                  send_buffer[inxo],       // reqdata
                                  120,                     // reqdatasize
                                  recv_buffer[inxo],       // replydata
                                  BUFSIZ,                  // replydatamax
                                  0,                       // linkertag
                                  0,                       // pri
                                  0,                       // xmitclass
                                  0);                      // linkopts
                util_check("XMSG_LINK_", ferr);
                msg_count.add_val(1);
                if (verbose)
                    printf("client(%s): msg-count=%d\n",
                           my_name, msg_count.read_val());
                count++;
                ferr = BMSG_BREAK_(msgid[inxo],
                                   results.u.s,
                                   TPT_REF(phandle));
                util_check("XMSG_BREAK_", ferr);
            }
            if (mq) {
                if ((inxl % 100) == 0)
                    printf("client(%s): count=%d\n", my_name, inxl);
            }
        }
        ferr = msg_mon_close_process(TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
    } else {
        ferr = XCONTROLMESSAGESYSTEM(XCTLMSGSYS_SETRECVLIMIT,
                                     XMAX_SETTABLE_RECVLIMIT);
        assert(ferr == XZFIL_ERR_OK);
        closes = 0;
        maxmsg = loop * maxout;
        msg_count.set_val(0);
        msg_mon_enable_mon_messages(true);
        for (inxs = 0; inxs < maxs; inxs++) {
            char lname[10];
            sprintf(lname, "s%d", inxs);
            thrs[inxs] = new MyThread(server_thr, lname, inxs);
        }
        for (inxs = 0; inxs < maxs; inxs++)
            thrs[inxs]->start();
        for (inxs = 0; inxs < maxs; inxs++) {
            status = thrs[inxs]->join(&res);
            TEST_CHK_STATUSOK(status);
            printf("joined with server %d\n", inxs);
        }
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
