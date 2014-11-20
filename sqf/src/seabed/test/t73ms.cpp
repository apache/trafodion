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

bool  debug        = false;
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
bool  shutdown     = true;
int   sre_count    = 0;
bool  thrlisten    = false;
bool  txrate       = false;
bool  verbose      = false;

enum {
    MAX_THR_C = 100,
    MAX_THR_S = 100
};

bool                   abortserver = false;
SB_Ts_Queue            free_q((char *) "freeQ");
SB_Thread::Mutex       mutex;
char                   my_name[BUFSIZ];
TPT_DECL              (phandle);
SB_Sig_Queue           work_q((char *) "workQ", false);

typedef struct  {
    SB_ML_Type     link;
    bool           fin;
    MS_SRE         sre;
} Test_SRE;
enum { MAX_SRES = MAX_THR_S * 3 };
Test_SRE sres[MAX_SRES];


void *client_thr(void *arg) {
    Util_AA<char>   event_data(MS_MON_MAX_SYNC_DATA);
    int             event_len;
    int             ferr;
    int             inx;
    int             msgid;
    Util_AA<char>   recv_buffer(BUFSIZ);
    Util_AA<short>  recv_buffer3(BUFSIZ);
    RT              results;
    Util_AA<char>   send_buffer(BUFSIZ);
    Util_AA<short>  send_buffer2(BUFSIZ);
    int             send_len;
    long            t_elapsed;
    struct timeval  t_start;
    struct timeval  t_stop;
    int             whoami = inxc++;

    arg = arg; // touch
    if (event) {
        ferr = msg_mon_event_wait(1, &event_len, &event_data);
        TEST_CHK_FEOK(ferr);
    }
    util_time_timer_start(&t_start);
    for (inx = 0; inx < loop; inx++) {
        if (txrate) {
            if ((inx % 100) == 0) {
                util_time_timer_stop(&t_stop);
                t_elapsed = (t_stop.tv_sec * 1000000 + t_stop.tv_usec) -
                            (t_start.tv_sec * 1000000 + t_start.tv_usec);
                t_elapsed = t_elapsed / 1000;
                if (inx == 0)
                    printf("%s: c-%d\n", name, whoami);
                else
                    printf("%s: c-%d, inx=%d, msg/ms=%ld\n",
                           name, whoami, inx, t_elapsed / inx);
            }
        } else if (mq) {
            if ((inx % 100) == 0)
                printf("%s: c-%d, inx=%d\n", name, whoami, inx);
        } else if (!quiet)
            printf("c-%d, inx=%d\n", whoami, inx);
        fflush(stdout);
        sprintf(&send_buffer, "hello, greetings from %s-%d, inx=%d",
                my_name, whoami, inx);
        send_len = (int) strlen(&send_buffer) + 1;
        if (verbose)
            printf("c-%d: sending %s\n", whoami, &send_buffer);
        ferr = XMSG_LINK_(TPT_REF(phandle),                     // phandle
                          &msgid,                      // msgid
                          &send_buffer2,               // reqctrl
                          (ushort) (inx & 1),          // reqctrlsize
                          &recv_buffer3,               // replyctrl
                          (ushort) 1,                  // replyctrlmax
                          &send_buffer,                // reqdata
                          (ushort) send_len,           // reqdatasize
                          &recv_buffer,                // replydata
                          (ushort) BUFSIZ,             // replydatamax
                          0,                           // linkertag
                          0,                           // pri
                          0,                           // xmitclass
                          0);                          // linkopts
        if (!abortserver)
            util_check("XMSG_LINK_", ferr);
        ferr = XMSG_BREAK_(msgid, results.u.s, TPT_REF(phandle));
        if (!abortserver)
            util_check("XMSG_BREAK_", ferr);
        if (!abortserver) {
            if (verbose)
                printf("c-%d: received %s\n", whoami, &recv_buffer);
            assert(results.u.t.ctrl_size == (uint) (inx & 1));
            assert(results.u.t.data_size == 100);
            assert(results.u.t.errm == RT_DATA_RCVD); // data
            if (inx == (loop - 1)) {
                if (maxcp > 1)
                    printf("%s: %s\n", name, &recv_buffer);
                else
                    printf("%s\n", &recv_buffer);
            }
        }
    }
    return NULL;
}

void server(int whoami, Test_SRE *sre) {
    static int      cnt = 1;
    int             ferr;
    int             len;
    char           *p;
    Util_AA<char>   recv_buffer(BUFSIZ);
    Util_AA<short>  recv_buffer2(BUFSIZ);
    bool            reply = true;

    if (abortserver) {
        cnt++;
        if (cnt >= loop * maxc * maxcp)
            util_abort_core_free();
    }
    ferr = XMSG_READCTRL_(sre->sre.sre_msgId,     // msgid
                          &recv_buffer2,          // reqctrl
                          1);                     // bytecount
    util_check("XMSG_READCTRL_", ferr);
    ferr = XMSG_READDATA_(sre->sre.sre_msgId,     // msgid
                          &recv_buffer,           // reqdata
                          BUFSIZ);                // bytecount
    util_check("XMSG_READDATA_", ferr);
    if (verbose)
        printf("s-%d: received %s\n", whoami, &recv_buffer);
    strcat(&recv_buffer, "- reply from ");
    strcat(&recv_buffer, my_name);
    if (abortserver) {
        p = index(&recv_buffer, '=');
        if (atoi(&p[1]) == (loop - 1))
            reply = false;
    }
    if (reply) {
        len = 100;
        if (verbose)
            printf("s-%d: sending %s\n", whoami, &recv_buffer);
        XMSG_REPLY_(sre->sre.sre_msgId,          // msgid
                    &recv_buffer2,               // replyctrl
                    sre->sre.sre_reqCtrlSize,    // replyctrlsize
                    &recv_buffer,                // replydata
                    (ushort) len,                // replydatasize
                    0,                           // errorclass
                    NULL);                       // newphandle
    }
}

void *server_thr(void *arg) {
    int       lerr;
    int       max_msgs = maxcp * maxc * loop;
    Test_SRE  sre;
    Test_SRE *srep;
    int       status;
    int       whoami = inxs++;

    arg = arg; // touch
    for (;;) {
        if (thrlisten) {
            sre.fin = false;
            if (sre_count >= max_msgs)
                break;
            do {
                lerr = XWAIT(LREQ, -1);
                lerr = XMSG_LISTEN_((short *) &sre.sre,  // sre
                                    0,                   // listenopts
                                    0);                  // listenertag
            } while (lerr == XSRETYPE_NOWORK);
            status = mutex.lock();
            TEST_CHK_STATUSOK(status);
            sre_count++;
            status = mutex.unlock();
            TEST_CHK_STATUSOK(status);
            srep = &sre;
        } else
            srep = (Test_SRE *) work_q.remove();
        if (verbose)
            printf("s-%d: have work, fin=%d\n", whoami, srep->fin);
        if (srep->fin)
            break;
        server(whoami, srep);
        if (!thrlisten)
            free_q.add(&srep->link);
    }
    return NULL;
}


SB_Thread::Thread *thrc[MAX_THR_C];
SB_Thread::Thread *thrs[MAX_THR_S];

int main(int argc, char *argv[]) {
    bool  abortclose = false;
    bool  client = false;
    int   ferr;
    int   inx;
    int   lerr;
    int   oid;
    int   proc;
    void *res;
    int   status;
    int   thr;
    TAD   zargs[] = {
      { "-abortclose",  TA_Bool, TA_NOMAX,    &abortclose   },
      { "-abortserver", TA_Bool, TA_NOMAX,    &abortserver  },
      { "-client",      TA_Bool, TA_NOMAX,    &client       },
      { "-debug",       TA_Bool, TA_NOMAX,    &debug        },
      { "-event",       TA_Bool, TA_NOMAX,    &event        },
      { "-inst",        TA_Int,  TA_NOMAX,    &inst         },
      { "-loop",        TA_Int,  TA_NOMAX,    &loop         },
      { "-maxc",        TA_Int,  MAX_THR_C,   &maxc         },
      { "-maxcp",       TA_Int,  TA_NOMAX,    &maxcp        },
      { "-maxs",        TA_Int,  MAX_THR_S,   &maxs         },
      { "-maxsp",       TA_Int,  TA_NOMAX,    &maxsp        },
      { "-mq",          TA_Bool, TA_NOMAX,    &mq           },
      { "-name",        TA_Str,  TA_NOMAX,    &name         },
      { "-noshutdown",  TA_Bool, TA_NOMAX,    &shutdown     },
      { "-quiet",       TA_Bool, TA_NOMAX,    &quiet        },
      { "-server",      TA_Ign,  TA_NOMAX,    NULL          },
      { "-thrlisten",   TA_Bool, TA_NOMAX,    &thrlisten    },
      { "-txrate",      TA_Bool, TA_NOMAX,    &txrate       },
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
        printf("inst=%d, loop=%d, maxc=%d, maxcp=%d, maxs=%d, maxsp=%d, shutdown=%d, thrlisten=%d, txrate=%d\n",
               inst, loop, maxc, maxcp, maxs, maxsp, shutdown, thrlisten, txrate);
        ferr = msg_mon_open_process((char *) "$srv",      // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
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
        if (!thrlisten) {
            Test_SRE *sre = NULL;
            for (proc = 0; proc < maxcp; proc++) {
                for (thr = 0; thr < maxc; thr++) {
                    for (inx = 0; inx < loop;) {
                        lerr = XWAIT(LREQ, -1);
                        TEST_CHK_WAITIGNORE(lerr);
                        do {
                            if (sre == NULL) {
                                sre = (Test_SRE *) free_q.remove();
                                assert(sre != NULL);
                            }
                            sre->fin = false;
                            lerr = XMSG_LISTEN_((short *) &sre->sre, // sre
                                                0,                   // listenopts
                                                0);                  // listenertag
                            if (lerr == XSRETYPE_IREQ) {
                                assert(sre->sre.sre_msgId > 0);
                                if (verbose)
                                    printf("s-main: queue work inx=%d\n", inx);
                                work_q.add(&sre->link);
                                sre = NULL;
                                inx++;
                            }
                        } while (lerr == XSRETYPE_IREQ);
                    }
                }
            }

            for (inx = 0; inx < maxs; inx++) {
                Test_SRE *quit_sre = (Test_SRE *) free_q.remove();
                quit_sre->fin = true;
                if (verbose)
                    printf("s-main: fin inx=%d\n", inx);
                work_q.add(&quit_sre->link);
                util_time_sleep_ms(1);
            }
        }

        for (inx = 0; inx < maxs; inx++) {
            status = thrs[inx]->join(&res);
            TEST_CHK_STATUSOK(status);
            printf("joined with server %d\n", inx);
        }
    }

    if (abortclose)
        util_abort_core_free();
    if (client) {
        ferr = msg_mon_close_process(TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
    }
    if (shutdown) {
        ferr = msg_mon_process_shutdown();
        TEST_CHK_FEOK(ferr);
    }
    util_test_finish(client);
    return 0;
}
