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

enum { MAX_BUF = 0x40000 };

bool  debug        = false;
int   dsize        = MAX_BUF;
bool  event        = false;
int   inst         = -1;
int   inxc         = 1;
int   inxs         = 1;
int   loop         = 1000;
int   maxc         = 1;
int   maxcp        = 1;
int   maxs         = 1;
int   maxsp        = 1;
bool  mq           = false;
char *name         = NULL;
bool  quiet        = false;
int   sre_count    = 0;
bool  verbose      = false;

enum {
    MAX_THR_C = 100,
    MAX_THR_S = 100
};

SB_Ts_Queue            free_q((char *) "freeQ");
SB_Thread::Mutex       mutex;
char                   my_name[BUFSIZ];
TPT_DECL2             (phandle,MAX_THR_S);
SB_Sig_Queue           work_q((char *) "workQ", false);

typedef struct  {
    SB_ML_Type     link;
    bool           fin;
    BMS_SRE        sre;
} Test_SRE;
enum { MAX_SRES = MAX_THR_S * 3 };
Test_SRE sres[MAX_SRES];


void *client_thr(void *arg) {
    Util_AA<char>   event_data(MS_MON_MAX_SYNC_DATA);
    int             event_len;
    int             ferr;
    int             inx;
    int             msgid;
    Util_AA<char>   recv_buffer(MAX_BUF);
    Util_AA<short>  recv_buffer3(MAX_BUF);
    RT              results;
    Util_AA<char>   send_buffer(MAX_BUF);
    Util_AA<short>  send_buffer2(MAX_BUF);
    int             srv;
    long            t_elapsed[MAX_THR_S];
    struct timeval  t_elapsedt;
    double          t_msgs;
    double          t_mb;
    double          t_sec;
    struct timeval  t_start;
    struct timeval  t_stop;
    int             whoami = inxc++;

    arg = arg; // touch
    if (event) {
        ferr = msg_mon_event_wait(1, &event_len, &event_data);
        TEST_CHK_FEOK(ferr);
    }
    for (srv = 0; srv < maxsp; srv++)
        t_elapsed[srv] = 0;
    for (inx = 0; inx < loop; inx++) {
        for (srv = 0; srv < maxsp; srv++) {
            if (mq) {
                if ((inx % 1000) == 0) {
                    if (t_elapsed[srv] == 0)
                        printf("%s: c-%d, inx=%d\n", name, whoami, inx);
                    else {
                        t_msgs = (double) 1000;
                        t_mb = (double) (t_msgs * dsize) / 1000000;
                        t_sec = ((double) t_elapsed[srv] / 1000000.0);
                        printf("%s: c-%d, inx=%d, mb/sec[%d]=%f, msg/sec[%d]=%f\n",
                               name, whoami, inx,
                               srv, t_mb/t_sec,
                               srv, t_msgs/t_sec);
                    }
                    t_elapsed[srv] = 0;
                }
            } else if (!quiet)
                printf("c-%d, inx=%d\n", whoami, inx);
            if (verbose)
                printf("c-%d: sending\n", whoami);
            util_time_timer_start(&t_start);
            send_buffer.ip_v[0] = 0;
            ferr = BMSG_LINK_(TPT_REF2(phandle,srv),       // phandle
                              &msgid,                      // msgid
                              &send_buffer2,               // reqctrl
                              (ushort) (inx & 1),          // reqctrlsize
                              &recv_buffer3,               // replyctrl
                              (ushort) 1,                  // replyctrlmax
                              &send_buffer,                // reqdata
                              dsize,                       // reqdatasize
                              &recv_buffer,                // replydata
                              MAX_BUF,                     // replydatamax
                              0,                           // linkertag
                              0,                           // pri
                              0,                           // xmitclass
                              0);                          // linkopts
            util_check("BMSG_LINK_", ferr);
            ferr = BMSG_BREAK_(msgid, results.u.s, TPT_REF2(phandle,srv));
            util_check("BMSG_BREAK_", ferr);
            util_time_timer_stop(&t_stop);
            util_time_elapsed(&t_start, &t_stop, &t_elapsedt);
            t_elapsed[srv] += (t_elapsedt.tv_sec *
                              1000000 + t_elapsedt.tv_usec);
            if (verbose)
                printf("c-%d: received\n", whoami);
            assert(results.u.t.ctrl_size == 0);
            assert(results.u.t.data_size == 0);
            assert(results.u.t.errm == 0);
            if (inx == (loop - 1)) {
                if (maxcp > 1)
                    printf("%s: received\n", name);
                else
                    printf("received\n");
            }
        }
    }
    return NULL;
}

void server(int whoami, Test_SRE *sre) {
    int             ferr;
    Util_AA<char>   recv_buffer(MAX_BUF);
    Util_AA<short>  recv_buffer2(MAX_BUF);

    ferr = BMSG_READCTRL_(sre->sre.sre_msgId,     // msgid
                          &recv_buffer2,          // reqctrl
                          1);                     // bytecount
    util_check("BMSG_READCTRL_", ferr);
    ferr = BMSG_READDATA_(sre->sre.sre_msgId,     // msgid
                          &recv_buffer,           // reqdata
                          MAX_BUF);               // bytecount
    util_check("BMSG_READDATA_", ferr);
    if (verbose)
        printf("s-%d: received %s\n", whoami, &recv_buffer);
    strcat(&recv_buffer, "- reply from ");
    strcat(&recv_buffer, my_name);
    if (verbose)
        printf("s-%d: sending %s\n", whoami, &recv_buffer);
    BMSG_REPLY_(sre->sre.sre_msgId,          // msgid
                NULL,                        // replyctrl
                0,                           // replyctrlsize
                NULL,                        // replydata
                0,                           // replydatasize
                0,                           // errorclass
                NULL);                       // newphandle
}

void *server_thr(void *arg) {
    Test_SRE *srep;
    int       whoami = inxs++;

    arg = arg; // touch
    for (;;) {
        srep = (Test_SRE *) work_q.remove();
        if (verbose)
            printf("s-%d: have work, fin=%d\n", whoami, srep->fin);
        if (srep->fin)
            break;
        server(whoami, srep);
        free_q.add(&srep->link);
    }
    return NULL;
}

SB_Thread::Thread *thrc[MAX_THR_C];
SB_Thread::Thread *thrs[MAX_THR_S];

int main(int argc, char *argv[]) {
    bool  client = false;
    int   ferr;
    int   inx;
    int   lerr;
    int   max;
    int   oid;
    void *res;
    int   status;
    TAD   zargs[] = {
      { "-client",      TA_Bool, TA_NOMAX,    &client       },
      { "-debug",       TA_Bool, TA_NOMAX,    &debug        },
      { "-dsize",       TA_Int,  MAX_BUF,     &dsize        },
      { "-inst",        TA_Int,  TA_NOMAX,    &inst         },
      { "-loop",        TA_Int,  TA_NOMAX,    &loop         },
      { "-maxc",        TA_Int,  MAX_THR_C,   &maxc         },
      { "-maxcp",       TA_Int,  TA_NOMAX,    &maxcp        },
      { "-maxs",        TA_Int,  MAX_THR_S,   &maxs         },
      { "-maxsp",       TA_Int,  TA_NOMAX,    &maxsp        },
      { "-mq",          TA_Bool, TA_NOMAX,    &mq           },
      { "-name",        TA_Str,  TA_NOMAX,    &name         },
      { "-quiet",       TA_Bool, TA_NOMAX,    &quiet        },
      { "-server",      TA_Ign,  TA_NOMAX,    NULL          },
      { "-v",           TA_Bool, TA_NOMAX,    &verbose      },
      { "",             TA_End,  TA_NOMAX,    NULL          }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);

    // setup threads
    if (client) {
        for (inx = 0; inx < maxc; inx++) {
            char lname[10];
            sprintf(lname, "c%d", inx);
            thrc[inx] = new SB_Thread::Thread(client_thr, lname);
        }
    } else {
        for (inx = 0; inx < maxs; inx++) {
            char lname[10];
            sprintf(lname, "s%d", inx);
            thrs[inx] = new SB_Thread::Thread(server_thr, lname);
        }
    }

    util_test_start(client);
    ferr = msg_mon_process_startup(!client); // system messages
    TEST_CHK_FEOK(ferr);
    if (client) {
        printf("inst=%d, loop=%d, maxc=%d, maxcp=%d, maxs=%d, maxsp=%d\n", inst, loop, maxc, maxcp, maxs, maxsp);
        for (inx = 0; inx < maxsp; inx++) {
            char lname[10];
            if (inx == 0)
                strcpy(lname, "$srv");
            else
                sprintf(lname, "$s%d", inx);
            ferr = msg_mon_open_process(lname,
                                        TPT_REF2(phandle,inx),
                                        &oid);
            TEST_CHK_FEOK(ferr);
        }
    }
    util_gethostname(my_name, sizeof(my_name));

    if (client) {
        for (inx = 0; inx < maxc; inx++)
            thrc[inx]->start();
        for (inx = 0; inx < maxc; inx++) {
            status = thrc[inx]->join(&res);
            TEST_CHK_STATUSOK(status);
            printf("joined with client %d\n", inx);
        }
    } else {
        for (inx = 0; inx < MAX_SRES; inx++)
            free_q.add(&sres[inx].link);
        for (inx = 0; inx < maxs; inx++)
            thrs[inx]->start();
        Test_SRE *sre = NULL;
        max = maxcp * maxc * loop;
        inx = 0;
        while (inx < max) {
            lerr = XWAIT(LREQ, -1);
            TEST_CHK_WAITIGNORE(lerr);
            do {
                if (sre == NULL) {
                    sre = (Test_SRE *) free_q.remove();
                    assert(sre != NULL);
                }
                sre->fin = false;
                lerr = BMSG_LISTEN_((short *) &sre->sre, // sre
                                    0,                   // listenopts
                                    0);                  // listenertag
                if (lerr == BSRETYPE_IREQ) {
                    assert(sre->sre.sre_msgId > 0);
                    if (verbose)
                        printf("s-main: queue work inx=%d\n", inx);
                    work_q.add(&sre->link);
                    sre = NULL;
                    inx++;
                }
            } while (lerr == BSRETYPE_IREQ);
        }

        for (inx = 0; inx < maxs; inx++) {
            Test_SRE *quit_sre = (Test_SRE *) free_q.remove();
            quit_sre->fin = true;
            if (verbose)
                printf("s-main: fin inx=%d\n", inx);
            work_q.add(&quit_sre->link);
            util_time_sleep_ms(1);
        }

        for (inx = 0; inx < maxs; inx++) {
            status = thrs[inx]->join(&res);
            TEST_CHK_STATUSOK(status);
            printf("joined with server %d\n", inx);
        }
    }

    if (client) {
        for (inx = 0; inx < maxsp; inx++) {
            ferr = msg_mon_close_process(TPT_REF2(phandle,inx));
            TEST_CHK_FEOK(ferr);
        }
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
