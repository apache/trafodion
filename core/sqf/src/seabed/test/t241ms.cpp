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

enum     { MAX_SERVERS = 2 };
int        loop = 10;
char       my_name[BUFSIZ];
TPT_DECL2 (phandle,MAX_SERVERS);
char       recv_buffer[40000];
char       send_buffer[40000];


void *thread_cli(void *arg) {
    int        ferr;
    int        inx;
    int        msgid;
    int        oid;
    RT         results;
    char       server_name[10];
    int        sinx;

    arg = arg; // touch

    ferr = msg_enable_open_cleanup();
    for (sinx = 0; sinx < MAX_SERVERS; sinx++) {
        sprintf(server_name, "$srv%d", sinx);
        ferr = msg_mon_open_process(server_name,           // name
                                    TPT_REF2(phandle,sinx),
                                    &oid);
        TEST_CHK_FEOK(ferr);
    }

    for (sinx = 0; sinx < MAX_SERVERS; sinx++) {
        for (inx = 0; inx < loop; inx++) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            ferr = XMSG_LINK_(TPT_REF2(phandle,sinx),      // phandle
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
            ferr = XMSG_BREAK_(msgid, results.u.s, TPT_REF2(phandle,sinx));
            util_check("XMSG_BREAK_", ferr);
            assert(results.u.t.ctrl_size == 0);
            assert(results.u.t.errm == RT_DATA_RCVD); // data
            printf("%s\n", recv_buffer);
        }
    }
    for (sinx = 0; sinx < MAX_SERVERS; sinx++) {
        if (sinx == 0)
            continue;
        ferr = msg_mon_close_process(TPT_REF2(phandle,sinx));
        TEST_CHK_FEOK(ferr);
    }
    return NULL;
}

void *thread_cli_fun(void *arg) {
    return thread_cli(arg);
}


int main(int argc, char *argv[]) {
    int                arg;
    bool               client = false;
    int                ferr;
    int                inx;
    int                len;
    int                lerr;
    char               prog[MS_MON_MAX_PROCESS_PATH];
    void              *result;
    char               server_name[10];
    int                server_nid;
    int                server_pid;
    MS_SRE             sre;
    int                status;
    SB_Thread::Thread *thr_cli;
    TAD                zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    util_gethostname(my_name, sizeof(my_name));
    if (client) {
        sprintf(prog, "%s/%s", getenv("PWD"), argv[0]);
        for (arg = 0; arg < argc; arg++)
            if (strcmp(argv[arg], "-client") == 0) // start_process
                argv[arg] = (char *) "-server";
        for (inx = 0; inx < MAX_SERVERS; inx++) {
            server_nid = -1;
            sprintf(server_name, "$srv%d", inx);
            ferr = msg_mon_start_process(prog,                   // prog
                                         server_name,            // name
                                         NULL,                   // ret name
                                         argc,
                                         argv,
                                         TPT_REF2(phandle,inx),
                                         0,                      // open
                                         NULL,                   // oid
                                         MS_ProcessType_Generic, // type
                                         0,                      // priority
                                         0,                      // debug
                                         0,                      // backup
                                         &server_nid,            // nid
                                         &server_pid,            // pid
                                         NULL,                   // infile
                                         NULL);                  // outfile
            TEST_CHK_FEOK(ferr);
        }
        thr_cli = new SB_Thread::Thread(thread_cli_fun, "cli");
        thr_cli->start();
        status = thr_cli->join(&result);
        TEST_CHK_STATUSOK(status);
        delete thr_cli;
    } else {
        for (inx = 0; inx < loop; inx++) {
            do {
                lerr = XWAIT(LREQ, -1);
                TEST_CHK_WAITIGNORE(lerr);
                lerr = XMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
            } while (lerr == XSRETYPE_NOWORK);
            ferr = XMSG_READCTRL_(sre.sre_msgId,  // msgid
                                  NULL,           // reqctrl
                                  0);             // bytecount
            util_check("XMSG_READCTRL_", ferr);
            ferr = XMSG_READDATA_(sre.sre_msgId,  // msgid
                                  recv_buffer,    // reqdata
                                  40000);         // bytecount
            util_check("XMSG_READDATA_", ferr);
            strcat(recv_buffer, "- reply from ");
            strcat(recv_buffer, my_name);
            len = (int) strlen(recv_buffer) + 1;
            len = len; // touch - not currently used
            XMSG_REPLY_(sre.sre_msgId,  // msgid
                        NULL,           // replyctrl
                        0,              // replyctrlsize
                        recv_buffer,    // replydata
                        39000,          // replydatasize
                        0,              // errorclass
                        NULL);          // newphandle
        }
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
