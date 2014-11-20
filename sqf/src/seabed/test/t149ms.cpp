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

enum { MAX_THREADS = 100 };
int          *gargc;
char        **gargv;
int           loop = 10;
char          my_name[BUFSIZ];
TPT_DECL     (phandle);
char          recv_buffer[40000];
int           threads = 10;
bool          verbose = false;
char          who[BUFSIZ];


void restart_server() {
    int                       disable;
    int                       ferr;
    MS_Mon_Process_Info_Type  info;
    char                      prog[BUFSIZ];
    TPT_DECL                 (server_phandle);

    ferr = msg_mon_get_process_info_detail((char *) "$srv", &info);
    if (ferr == XZFIL_ERR_OK)
        return; // server is alive
    sprintf(prog, "%s/%s", getenv("PWD"), gargv[0]);
    if (verbose)
        printf("%s - starting server\n", who);
    // TODO: remove disable/loop
    disable = msg_test_assert_disable();
    do {
        ferr = msg_mon_start_process(prog,                   // prog
                                     (char *) "$srv",        // name
                                     NULL,                   // ret name
                                     *gargc,
                                     gargv,
                                     TPT_REF(server_phandle),
                                     0,                      // open
                                     NULL,                   // oid
                                     MS_ProcessType_Generic, // type
                                     0,                      // priority
                                     0,                      // debug
                                     0,                      // backup
                                     NULL,                   // nid
                                     NULL,                   // pid
                                     NULL,                   // infile
                                     NULL);                  // outfile
        if (ferr == XZFIL_ERR_BOUNDSERR) {
            printf("TODO: remove this sleep - $srv already in use\n");
            sleep(1);
        }
    } while (ferr == XZFIL_ERR_BOUNDSERR);
    TEST_CHK_FEOK(ferr);
    msg_test_assert_enable(disable);
}

void *thread_send(void *arg) {
    bool            end;
    int             ferr;
    int             inx;
    char            lwho[BUFSIZ];
    int             msgid;
    char           *name;
    RT              results;
    Util_AA<char>   send_buffer(40000);
    int             status;
    int             thrinx;

    end = (arg == NULL);
    if (end)
        thrinx = -1;
    else {
        name = SB_Thread::Sthr::self_name();
        thrinx = atoi(name);
    }
    sprintf(lwho, "client-%d", thrinx);
    for (inx = 0; inx < loop; inx++) {
        sprintf(&send_buffer, "hello, greetings from %s-%d, inx=%d",
                my_name, thrinx, inx);
        ferr = XMSG_LINK_(TPT_REF(phandle),            // phandle
                          &msgid,                      // msgid
                          NULL,                        // reqctrl
                          0,                           // reqctrlsize
                          NULL,                        // replyctrl
                          0,                           // replyctrlmax
                          &send_buffer,                // reqdata
                          end ?                        // reqdatasize
                         (ushort) 0 : (ushort) 39000,
                          recv_buffer,                 // replydata
                          40000,                       // replydatamax
                          0,                           // linkertag
                          0,                           // pri
                          0,                           // xmitclass
                          XMSG_LINK_LDONEQ);           // linkopts
        assert((ferr != XZFIL_ERR_NOBUFSPACE) &&
               (ferr != XZFIL_ERR_PATHDOWN));
        ferr = XMSG_BREAK_(msgid, results.u.s, TPT_REF(phandle));
        if (verbose) {
            printf("%s - BREAK returned err=%d\n", lwho, ferr);
            if (ferr == XZFIL_ERR_OK)
                printf("%s - rcvd=%s\n", lwho, recv_buffer);
        }
        if (ferr != XZFIL_ERR_OK) {
            static SB_Thread::Mutex mutex;
            if (mutex.trylock() == 0) { // got lock
                restart_server();
                status = mutex.unlock();
                TEST_CHK_STATUSOK(status);
            } else
                SB_Thread::Sthr::sleep(100); // 100 ms
            inx--;
            continue;
        } else if (end)
            break;
    }
    return NULL;
}

void *thread_send_fun(void *arg) {
    return thread_send(arg);
}

int main(int argc, char *argv[]) {
    int                arg;
    char               buf[BUFSIZ];
    bool               client = false;
    int                disable;
    int                ferr;
    int                inx;
    int                len;
    int                lerr;
    int                oid;
    void              *result;
    MS_SRE             sre;
    int                status;
    SB_Thread::Thread *thr_send[MAX_THREADS];
    TAD                zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-threads",   TA_Int,  MAX_THREADS, &threads   },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    gargc = &argc;
    gargv = argv;
    arg_proc_args(zargs, false, argc, argv);
    for (arg = 1; arg < argc; arg++) {
        if (strcmp(argv[arg], "-client") == 0) // start_process
            argv[arg] = (char *) "-server"; // set to server for start-process
    }
    if (threads < 1)
        threads = 1;
    if (client)
        strcpy(who, "client");
    else
        strcpy(who, "server");
    util_test_start(client);
    ferr = msg_mon_process_startup(true);  // system messages?
    TEST_CHK_FEOK(ferr);
    if (client) {
        if (verbose)
            printf("%s - opening server\n", who);
        ferr = msg_mon_open_process((char *) "$srv",      // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
    }
    util_gethostname(my_name, sizeof(my_name));

    if (client) {
        disable = msg_test_assert_disable();
        for (inx = 0; inx < threads; inx++) {
            sprintf(buf, "%d", inx);
            thr_send[inx] = new SB_Thread::Thread(thread_send_fun, buf);
            thr_send[inx]->start();
        }
        for (inx = 0; inx < threads; inx++) {
            status = thr_send[inx]->join(&result);
            TEST_CHK_STATUSOK(status);
            delete thr_send[inx];
        }
        thread_send_fun(NULL);
        msg_test_assert_enable(disable);
    } else {
        inx = 0;
        for (;;) {
            do {
                lerr = XWAIT(LREQ, -1);
                TEST_CHK_WAITIGNORE(lerr);
                lerr = XMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
            } while (lerr == XSRETYPE_NOWORK);
            inx++;
            if (inx > 4)
                util_abort_core_free(); // don't reply
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
            len = len; // touch
            XMSG_REPLY_(sre.sre_msgId,             // msgid
                        NULL,                      // replyctrl
                        0,                         // replyctrlsize
                        recv_buffer,               // replydata
                        39000,                     // replydatasize
                        0,                         // errorclass
                        NULL);                     // newphandle
            if (sre.sre_reqDataSize == 0)
                break;
        }
    }
    if (client) {
        ferr = msg_mon_close_process(TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
