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
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

enum { MAX_ARGS = 100 };
enum { MAX_SRV  = 1000 };
enum {
    T_CLOSE,
    T_NEWPROC,
    T_OPEN,
    T_PROCINFO,
    T_TOTAL,
    T_MAX
};

int             count_opens;
SB_Thread::CV   cv;
int             maxsp = 10;
bool            nowait = false;
TPT_DECL2      (nphandle,MAX_SRV);
double          t_elapsed[T_MAX];
double          t_start[T_MAX];
double          t_stop[T_MAX];
bool            verbose = false;


inline void time_elapsed(int t) {
    double elapsed = t_stop[t] - t_start[t];
    t_elapsed[t] += elapsed;
}

inline double time_sec(int t) {
    double elapsed = t_elapsed[t];
    return elapsed;
}

inline double time_get() {
    static bool            first = true;
    static struct  timeval ftv;
    struct timeval         tv;
    long                   us;

    gettimeofday(&tv, NULL);
    if (first) {
        first = false;
        memcpy(&ftv, &tv, sizeof(tv));
    }
    us = (tv.tv_sec * USPS + tv.tv_usec) -
         (ftv.tv_sec * USPS + ftv.tv_usec);

    return (double) us / USPS;
}

inline void time_start(int t) {
    t_start[t] = time_get();
}

inline void time_stop(int t) {
    t_stop[t] = time_get();
}

void do_notice(int t) {
    int        ferr;
    int        lerr;
    MS_Mon_Msg msg;
    BMS_SRE    sre;

    do {
        lerr = XWAIT(LREQ, -1);
        TEST_CHK_WAITIGNORE(lerr);
        lerr = BMSG_LISTEN_((short *) &sre, // sre
                            0,              // listenopts
                            0);             // listenertag
    } while (lerr == BSRETYPE_NOWORK);
    if (sre.sre_flags & XSRE_MON) {
        ferr = BMSG_READDATA_(sre.sre_msgId,  // msgid
                              (char *) &msg,  // reqdata
                              sizeof(msg));   // bytecount
        TEST_CHK_FEOK(ferr);
        if (verbose)
            printf("cli: rcvd mon msg=%d, exp=%d\n", msg.type, t);
        assert(t == msg.type);
    } else {
        if (verbose)
            printf("cli: rcvd msg, exp=%d\n", t);
        assert(t == -1);
    }
    if (verbose)
        printf("cli: reply\n");
    BMSG_REPLY_(sre.sre_msgId,       // msgid
                NULL,                // replyctrl
                0,                   // replyctrlsize
                NULL,                // replydata
                0,                   // replydatasize
                0,                   // errorclass
                NULL);               // newphandle
}

void cb_open(MS_Mon_Open_Comp_Type *comp) {
    int inx;
    int status;

    if (verbose)
        printf("client received open complete message, ferr=%d, name=%s, oid=%d, tag=0x%llx\n",
               comp->ferr,
               comp->name,
               comp->oid,
               comp->tag);
    assert(comp->ferr == XZFIL_ERR_OK);
    assert(memcmp(comp->name, "$SRV", 4) == 0);
    inx = atoi(&comp->name[4]);
    assert(comp->tag == inx);
    memcpy(TPT_REF2(nphandle, inx), &comp->phandle, sizeof(comp->phandle));
    count_opens++;
    if (count_opens >= maxsp) {
        status = cv.signal(true);
        TEST_CHK_STATUSOK(status);
    }
}

int  noid[MAX_SRV];
char nname[MAX_SRV][BUFSIZ];
char nprog[MS_MON_MAX_PROCESS_PATH];
char recv_buffer[BUFSIZ];

int main(int argc, char *argv[]) {
    bool                      attach = false;
    bool                      client = false;
    int                       close_count;
    double                    dloop;
    double                    dms;
    int                       done;
    double                    dsec;
    int                       ferr;
    bool                      fin = false;
    MS_Mon_Process_Info_Type  info;
    int                       inx;
    int                       inx2;
    int                       lerr;
    int                       loop = 10;
    int                       max;
    int                       nargc;
    char                     *nargv[MAX_ARGS];
    int                       nnid;
    int                       npid;
    BMS_SRE                   sre;
    int                       status;
    TAD                       zargs[] = {
      { "-attach",    TA_Bool, TA_NOMAX,    &attach    },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-maxsp",     TA_Int,  MAX_SRV,     &maxsp     },
      { "-nowait",    TA_Bool, TA_NOMAX,    &nowait    },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "-verbose",   TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    if (attach)
        ferr = msg_init_attach(&argc, &argv, false, NULL);
    else
        ferr = msg_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    dloop = (double) loop;
    util_test_start(client);
    ferr = msg_mon_process_startup(true); // system messages
    TEST_CHK_FEOK(ferr);
    for (inx = 0; inx < T_MAX; inx++)
        t_elapsed[inx] = 0.0;
    if (client) {
        msg_mon_enable_mon_messages(true);
        printf("loop=%d, maxsp=%d, nowait=%d\n", loop, maxsp, nowait);
        sprintf(nprog, "%s/%s", getenv("PWD"), argv[0]);
        nargc = argc;
        assert(nargc < MAX_ARGS);
        for (inx = 0; inx < argc; inx++) {
            if (strcmp(argv[inx], "-client") == 0)
                nargv[inx] = (char *) "-server";
            else
                nargv[inx] = argv[inx];
            if (strcmp(argv[inx], "-attach") == 0)
                nargv[inx] = (char *) "-server";
        }
        nnid = -1;
        time_start(T_TOTAL);
        for (inx = 0; inx < loop; inx += maxsp) {
            max = loop - inx;
            if (max > maxsp)
                max = maxsp;
            if (verbose)
                printf("cli: newproc, inx=%d\n", inx);
            time_start(T_NEWPROC);
            for (inx2 = 0; inx2 < max; inx2++)
                sprintf(nname[inx2], "$srv%d", inx2);
            for (inx2 = 0; inx2 < max; inx2++) {
nnid = 0;
                ferr = msg_mon_start_process(nprog,                  // prog
                                             nname[inx2],            // name
                                             NULL,                   // ret_name
                                             nargc,                  // argc
                                             nargv,                  // argv
                                             TPT_REF2(nphandle,inx2),// phandle
                                             false,                  // open
                                             &noid[inx2],            // oid
                                             MS_ProcessType_Generic, // type
                                             0,                      // priority
                                             false,                  // debug
                                             false,                  // backup
                                             &nnid,                  // nid
                                             &npid,                  // pid
                                             NULL,                   // infile
                                             NULL);                  // outfile
                TEST_CHK_FEOK(ferr);
            }
            time_stop(T_NEWPROC);
            time_elapsed(T_NEWPROC);
            if (verbose)
                printf("cli: open, inx=%d\n", inx);
            time_start(T_OPEN);
            count_opens = 0;
            for (inx2 = 0; inx2 < max; inx2++) {
                if (nowait) {
                    ferr = msg_mon_open_process_nowait_cb(nname[inx2],  // name
                                                          TPT_REF2(nphandle,inx2),
                                                          cb_open,      // cb
                                                          inx2,         // tag
                                                          &done,        // done?
                                                          &noid[inx2]); // oid
                    TEST_CHK_FEOK(ferr);
                    assert(!done);
                } else {
                    ferr = msg_mon_open_process(nname[inx2],  // name
                                                TPT_REF2(nphandle,inx2),
                                                &noid[inx2]);
                    TEST_CHK_FEOK(ferr);
                }
            }
            if (nowait) {
                status = cv.wait(true);
                TEST_CHK_STATUSOK(status);
            }
            time_stop(T_OPEN);
            time_elapsed(T_OPEN);

            if (verbose)
                printf("cli: procinfo, inx=%d\n", inx);
            time_start(T_PROCINFO);
            for (inx2 = 0; inx2 < max; inx2++) {
                ferr = msg_mon_get_process_info_detail(nname[inx2], &info);
                TEST_CHK_FEOK(ferr);
            }
            time_stop(T_PROCINFO);
            time_elapsed(T_PROCINFO);

            if (verbose)
                printf("cli: close, inx=%d\n", inx);
            time_start(T_CLOSE);
            for (inx2 = 0; inx2 < max; inx2++) {
                ferr = msg_mon_close_process(TPT_REF2(nphandle,inx2));
                TEST_CHK_FEOK(ferr);
            }
            time_stop(T_CLOSE);
            time_elapsed(T_CLOSE);

            // re-open/close
            for (inx2 = 0; inx2 < max; inx2++) {
                ferr = msg_mon_open_process(nname[inx2],  // name
                                            TPT_REF2(nphandle,inx2),
                                            &noid[inx2]);
                TEST_CHK_FEOK(ferr);
            }
            for (inx2 = 0; inx2 < max; inx2++) {
                ferr = msg_mon_close_process(TPT_REF2(nphandle,inx2));
                TEST_CHK_FEOK(ferr);
            }

            for (inx2 = 0; inx2 < max; inx2++)
                do_notice(MS_MsgType_ProcessDeath);
        }
    } else {
        msg_mon_enable_mon_messages(true);
        close_count = 0;
        time_start(T_TOTAL);
        for (inx = 0; !fin; inx++) {
            do {
                lerr = XWAIT(LREQ, -1);
                lerr = BMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
            } while (lerr == BSRETYPE_NOWORK);
            if (sre.sre_flags & XSRE_MON) {
                ferr = BMSG_READDATA_(sre.sre_msgId,  // msgid
                                      recv_buffer,    // reqdata
                                      BUFSIZ);        // bytecount
                MS_Mon_Msg *msg = (MS_Mon_Msg *) recv_buffer;
                if (verbose)
                    printf("srv: rcvd mon msg=%d, inx=%d\n", msg->type, inx);
                if (msg->type == MS_MsgType_Close)
                    if (++close_count >= 2)
                        fin = true;
            }
            if (verbose)
                printf("srv: reply, inx=%d\n", inx);
            BMSG_REPLY_(sre.sre_msgId,       // msgid
                        NULL,                // replyctrl
                        0,                   // replyctrlsize
                        NULL,                // replydata
                        0,                   // replydatasize
                        0,                   // errorclass
                        NULL);               // newphandle
        }
    }
    time_stop(T_TOTAL);
    time_elapsed(T_TOTAL);

    if (client) {
        printf("elapsed=%f\n", t_elapsed[T_TOTAL]);
        printf("open/close/newprocess/processinfo=%d\n", loop);
        dsec = time_sec(T_OPEN);
        dms = dsec * 1000.0;
        printf("open:     total-time=%f ms, time/loop=%f ms, ops/sec=%f\n",
               dms, dms / dloop, dloop / dsec);
        dsec = time_sec(T_CLOSE);
        dms = dsec * 1000.0;
        printf("close:    total-time=%f ms, time/loop=%f ms, ops/sec=%f\n",
               dms, dms / dloop, dloop / dsec);
        dsec = time_sec(T_PROCINFO);
        dms = dsec * 1000.0;
        printf("procinfo: total-time=%f ms, time/loop=%f ms, ops/sec=%f\n",
               dms, dms / dloop, dloop / dsec);
        dsec = time_sec(T_NEWPROC);
        dms = dsec * 1000.0;
        printf("newproc:  total-time=%f ms, time/loop=%f ms, ops/sec=%f\n",
               dms, dms / dloop, dloop / dsec);
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
