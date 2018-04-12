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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "seabed/ms.h"
#include "seabed/fserr.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "tchkfe.h"
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

enum { MAX_DBUF    =  128 };

enum { MAX_CLIENTS =  500 };
enum { MAX_SERVERS =  500 };

bool     client = false;
bool     master = false;
int      maxcp = 1;
int      maxnodes = 1;
char     my_name[BUFSIZ];
char     recv_buffer[BUFSIZ];
char     send_buffer[BUFSIZ];
char     serv[BUFSIZ];


void next_nid(int *nidin) {
    int nid = *nidin;
    nid++;
    if (nid >= maxnodes)
        nid = 0;
    *nidin = nid;
}

int main(int argc, char *argv[]) {
    int             arg;
    int             cinx;
    char            client_name[MS_MON_MAX_PROCESS_NAME];
    int             client_nid;
    TPT_DECL2      (client_phandle,MAX_CLIENTS);
    int             client_pid;
    bool            done;
    int             dsize = MAX_DBUF;
    int             ferr;
    int             inx;
    int             lerr;
    int             loop = 10;
    int             maxsp = 1;
    int             msgid;
    const char     *mtype;
    int             nid;
    int             oid;
    int             pinx;
    char            prog[MS_MON_MAX_PROCESS_PATH];
    char           *reqdata;
    char           *repdata;
    RT              results;
    char            ret_name[MS_MON_MAX_PROCESS_NAME];
    int             rate = 0;
    int             rate_cnt = 0;
    int             rate_us = 0;
    int             rate_usleep = 0;
    bool            ratev = false;
    bool            server = false;
    char            server_name[MS_MON_MAX_PROCESS_NAME];
    int             server_nid;
    TPT_DECL2      (server_phandle,MAX_SERVERS);
    int             server_pid;
    MS_SRE          sre;
    struct timeval  t_start;
    struct timeval  t_stop;
    struct timeval  t_elapsed;
    int             t_elapsed_us;
    bool            verbose = false;
    int             xinx;
    TAD             zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-cluster",   TA_Ign,  TA_NOMAX,    NULL       },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-master",    TA_Bool, TA_NOMAX,    &master    },
      { "-maxcp",     TA_Int,  MAX_CLIENTS, &maxcp     },
      { "-maxnodes",  TA_Int,  TA_NOMAX,    &maxnodes  },
      { "-maxsp",     TA_Int,  MAX_SERVERS, &maxsp     },
      { "-rate",      TA_Int,  TA_NOMAX,    &rate      },
      { "-ratev",     TA_Bool, TA_NOMAX,    &ratev     },
      { "-server",    TA_Bool, TA_NOMAX,    &server    },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "-verbose",   TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    if (master)
        ferr = msg_init_attach(&argc, &argv, false, (char *) "$MASTER");
    else
        ferr = msg_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, msg_debug_hook);
    if (maxcp < 0)
        maxcp = 1;
    if (maxsp < 0)
        maxsp = 1;
    if (maxnodes < 0)
        maxnodes = 1;
    ferr = msg_mon_process_startup(true);  // system messages?
    TEST_CHK_FEOK(ferr);
    msg_mon_enable_mon_messages(true);
    ferr = msg_mon_get_my_process_name(my_name, BUFSIZ);
    TEST_CHK_FEOK(ferr);

    sprintf(prog, "%s/%s", getenv("PWD"), argv[0]);
    if (master) {
        for (arg = 0; arg < argc; arg++)
            if (strcmp(argv[arg], "-master") == 0) // start_process
                argv[arg] = (char *) "-client";
        client_nid = 0;
        for (inx = 0; inx < maxcp; inx++) {
            sprintf(client_name, "$CLI%02x", inx);
            nid = client_nid;
            ferr = msg_mon_start_process(prog,                   // prog
                                         client_name,            // name
                                         ret_name,               // ret name
                                         argc,
                                         argv,
                                         TPT_REF2(client_phandle,inx),
                                         0,                      // open
                                         NULL,                   // oid
                                         MS_ProcessType_Generic, // type
                                         0,                      // priority
                                         0,                      // debug
                                         0,                      // backup
                                         &nid,                   // nid
                                         &client_pid,            // pid
                                         NULL,                   // infile
                                         NULL);                  // outfile
            printf("client process started, inx=%d, err=%d\n", inx, ferr);
            TEST_CHK_FEOK(ferr);
            next_nid(&client_nid);
        }
        for (inx = 0; inx < maxcp; inx++) {
            sprintf(client_name, "$CLI%02x", inx);
            ferr = msg_mon_open_process(client_name,
                                        TPT_REF2(client_phandle,inx),
                                        &oid);
            TEST_CHK_FEOK(ferr);
            if (verbose)
                printf("client process opened, inx=%d\n", inx);
        }
        inx = 0;
        while (inx < maxcp) {
            // pck up death msgs
            do {
                lerr = XWAIT(LREQ, -1);
                TEST_CHK_WAITIGNORE(lerr);
                lerr = XMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
            } while (lerr == XSRETYPE_NOWORK);
            ferr = XMSG_READDATA_(sre.sre_msgId,        // msgid
                                  recv_buffer,          // reqdata
                                  sre.sre_reqDataSize); // bytecount
            util_check("XMSG_READDATA_", ferr);
            if (sre.sre_flags & XSRE_MON) {
                MS_Mon_Msg *msg = (MS_Mon_Msg *) recv_buffer;
                if (msg->type == MS_MsgType_ProcessDeath) {
                    inx++;
                    if (verbose)
                        printf("client process death, inx=%d\n", inx);
                }
            }
            XMSG_REPLY_(sre.sre_msgId,         // msgid
                        NULL,                  // replyctrl
                        0,                     // replyctrlsize
                        NULL,                  // replydata
                        0,                     // replydatasize
                        0,                     // errorclass
                        NULL);                 // newphandle
        }
    } else if (client) {
        cinx = (int) strtol(&my_name[4], NULL, 16);
        for (arg = 0; arg < argc; arg++)
            if (strcmp(argv[arg], "-client") == 0) // start_process
                argv[arg] = (char *) "-server";
        reqdata = send_buffer;
        repdata = recv_buffer;
        if (rate) {
            gettimeofday(&t_start, NULL);
            rate_cnt = 0;
            rate_us = 1000000 / rate;
        }
        for (inx = 0; inx < loop; inx++) {
            server_nid = 0;
            for (pinx = 0; pinx < maxsp; pinx++) {
                if (server_nid == cinx)
                    next_nid(&server_nid);
                nid = server_nid;
                // SRV x <cinx> x <pinx> x <nid> x <inx>
                sprintf(server_name, "$SRVx%02xx%02xx%02xx%03x", cinx, pinx, nid, inx % 1000);
                ferr = msg_mon_start_process(prog,                   // prog
                                             server_name,            // name
                                             ret_name,               // ret name
                                             argc,
                                             argv,
                                             TPT_REF2(server_phandle,pinx),
                                             0,                      // open
                                             NULL,                   // oid
                                             MS_ProcessType_Generic, // type
                                             0,                      // priority
                                             0,                      // debug
                                             0,                      // backup
                                             &nid,                   // nid
                                             &server_pid,            // pid
                                             NULL,                   // infile
                                             NULL);                  // outfile
                if (verbose)
                    printf("server process started, oinx=%d, iinx=%d, err=%d\n", inx, pinx, ferr);
                TEST_CHK_FEOK(ferr);
                next_nid(&server_nid);
            }

            nid = 0;
            for (pinx = 0; pinx < maxsp; pinx++) {
                if (nid == cinx)
                    next_nid(&nid);
                sprintf(serv, "$SRVx%02xx%02xx%02xx%03x", cinx, pinx, nid, inx % 1000);
                ferr = msg_mon_open_process(serv,
                                            TPT_REF2(server_phandle,pinx),
                                            &oid);
                TEST_CHK_FEOK(ferr);
                if (verbose)
                    printf("server process opened, oinx=%d, iinx=%d\n", inx, pinx);
                next_nid(&nid);
            }
            for (pinx = 0; pinx < maxsp; pinx++) {
                xinx = pinx + 1;
                if (xinx >= maxsp)
                    xinx = 0;
                ferr = XMSG_LINK_(TPT_REF2(server_phandle,xinx), // phandle
                                  &msgid,                 // msgid
                                  NULL,                   // reqctrl
                                  0,                      // reqctrlsize
                                  NULL,                   // replyctrl
                                  0,                      // replyctrlmax
                                  reqdata,                // reqdata
                                  (short) dsize,          // reqdatasize
                                  repdata,                // replydata
                                  (short) dsize,          // replydatamax
                                  0,                      // linkertag
                                  0,                      // pri
                                  0,                      // xmitclass
                                  0);                     // linkopts
                util_check("XMSG_LINK_", ferr);
                ferr = XMSG_BREAK_(msgid,
                                   results.u.s,
                                   TPT_REF2(server_phandle,xinx));
                util_check("XMSG_BREAK_", ferr);
                if (verbose)
                    printf("server process link/break, oinx=%d, iinx=%d\n", inx, pinx);
            }
            for (pinx = 0; pinx < maxsp; pinx++) {
                ferr = msg_mon_close_process(TPT_REF2(server_phandle,pinx));
                TEST_CHK_FEOK(ferr);
                if (verbose)
                    printf("server process closed, oinx=%d, iinx=%d\n", inx, pinx);
            }
            pinx = 0;
            while (pinx < maxsp) {
                // pck up death msgs
                do {
                    lerr = XWAIT(LREQ, -1);
                    TEST_CHK_WAITIGNORE(lerr);
                    lerr = XMSG_LISTEN_((short *) &sre, // sre
                                        0,              // listenopts
                                        0);             // listenertag
                } while (lerr == XSRETYPE_NOWORK);
                ferr = XMSG_READDATA_(sre.sre_msgId,        // msgid
                                      recv_buffer,          // reqdata
                                      sre.sre_reqDataSize); // bytecount
                util_check("XMSG_READDATA_", ferr);
                if (sre.sre_flags & XSRE_MON) {
                    MS_Mon_Msg *msg = (MS_Mon_Msg *) recv_buffer;
                    mtype = msg_mon_text_get_mon_msg_type(msg->type);
                    if (msg->type == MS_MsgType_ProcessDeath)
                        pinx++;
                    if (verbose)
                        printf("client process rcvd mon msg type=%d(%s), inx=%d\n", msg->type, mtype, pinx);
                }
                XMSG_REPLY_(sre.sre_msgId,         // msgid
                            NULL,                  // replyctrl
                            0,                     // replyctrlsize
                            NULL,                  // replydata
                            0,                     // replydatasize
                            0,                     // errorclass
                            NULL);                 // newphandle
            }
            if (rate) {
                gettimeofday(&t_stop, NULL);
                util_time_elapsed(&t_start, &t_stop, &t_elapsed);
                t_elapsed_us = (int) (t_elapsed.tv_sec * 1000000 + t_elapsed.tv_usec);
                if (t_elapsed_us < rate_us * rate_cnt) {
                    rate_usleep = rate_us * rate_cnt - t_elapsed_us;
                    if (ratev)
                        printf("%s-sleeping=%d\n", my_name, rate_usleep);
                    usleep(rate_usleep);
                }
                rate_cnt++;
                if (rate_cnt >= rate) {
                    gettimeofday(&t_start, NULL);
                    rate_cnt = 0;
                    if (ratev)
                        printf("%s-loop=%d, elapsed=%d us\n", my_name, inx, t_elapsed_us);
                }
            }
        }
    } else {
        done = false;
        inx = 0;
        while (!done) {
            do {
                do {
                    lerr = XWAIT(LREQ, -1);
                    TEST_CHK_WAITIGNORE(lerr);
                    lerr = XMSG_LISTEN_((short *) &sre, // sre
                                        0,              // listenopts
                                        0);             // listenertag
                } while (lerr == XSRETYPE_NOWORK);
                ferr = XMSG_READDATA_(sre.sre_msgId,        // msgid
                                      recv_buffer,          // reqdata
                                      sre.sre_reqDataSize); // bytecount
                util_check("XMSG_READDATA_", ferr);
                if (sre.sre_flags & XSRE_MON) {
                    MS_Mon_Msg *msg = (MS_Mon_Msg *) recv_buffer;
                    mtype = msg_mon_text_get_mon_msg_type(msg->type);
                    if (verbose)
                        printf("%s rcvd mon msg type=%d(%s)\n", my_name, msg->type, mtype);
                    if (msg->type == MS_MsgType_Close)
                        done = true;
                    XMSG_REPLY_(sre.sre_msgId,         // msgid
                                NULL,                  // replyctrl
                                0,                     // replyctrlsize
                                NULL,                  // replydata
                                0,                     // replydatasize
                                0,                     // errorclass
                                NULL);                 // newphandle
                }
            } while ((!done) && (sre.sre_flags & XSRE_MON));
            if (done)
                break;
            XMSG_REPLY_(sre.sre_msgId,         // msgid
                        NULL,                  // replyctrl
                        0,                     // replyctrlsize
                        NULL,                  // replydata
                        0,                     // replydatasize
                        0,                     // errorclass
                        NULL);                 // newphandle
            if (verbose)
                printf("%s-count=%d\n", my_name, inx);
            inx++;
        }
    }

    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(master);
    return 0;
}
