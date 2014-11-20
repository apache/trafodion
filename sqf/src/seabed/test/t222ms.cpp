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
#include "tutil.h"
#include "tutilp.h"

SB_Thread::CV cv;
int           server_inx = 0;

void cb(SB_Phandle_Type *, struct MS_Mon_NewProcess_Notice_def *msg) {
    printf("client received new process message, ferr=%d, nid=%d, pid=%d, tag=0x%llx, pname=%s\n",
           msg->ferr,
           msg->nid,
           msg->pid,
           msg->tag,
           msg->process_name);
    assert(msg->ferr == XZFIL_ERR_OK);
    assert(msg->tag == server_inx);
    cv.signal(true);
    server_inx++;
}
void wait_death() {
    int         ferr;
    int         lerr;
    MS_Mon_Msg *msg;
    char        recv_buffer[BUFSIZ];
    BMS_SRE     sre;

    do {
        lerr = XWAIT(LREQ, -1);
        TEST_CHK_WAITIGNORE(lerr);
        lerr = BMSG_LISTEN_((short *) &sre, // sre
                            0,              // listenopts
                            0);             // listenertag
    } while (lerr == BSRETYPE_NOWORK);
    assert(sre.sre_flags & XSRE_MON);
    ferr = BMSG_READDATA_(sre.sre_msgId,  // msgid
                          recv_buffer,    // reqdata
                          BUFSIZ);        // bytecount
    util_check("XMSG_READDATA_", ferr);
    msg = (MS_Mon_Msg *) recv_buffer;
    assert(msg->type == MS_MsgType_ProcessDeath);
    printf("received death message for %s\n", msg->u.death.process_name);
    XMSG_REPLY_(sre.sre_msgId,  // msgid
                NULL,           // replyctrl
                0,              // replyctrlsize
                NULL,           // replydata
                0,              // replydatasize
                0,              // errorclass
                NULL);          // newphandle
}

int main(int argc, char *argv[]) {
    int         arg;
    bool        chook = false;
    bool        client = false;
    int         ferr;
    int         inx;
    int         loop = 10;
    char        my_name[BUFSIZ];
    bool        nowait = false;
    int         nid;
    int         pid;
    char        prog[MS_MON_MAX_PROCESS_PATH];
    char        retname[BUFSIZ];
    char        server_name[30];
    int         server_nid;
    TPT_DECL   (server_phandle);
    int         server_pid;
    bool        shook = false;
    int         status;
    TAD         zargs[] = {
      { "-chook",     TA_Bool, TA_NOMAX,    &chook     },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-nowait",    TA_Bool, TA_NOMAX,    &nowait    },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-shook",     TA_Bool, TA_NOMAX,    &shook     },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    if (chook && client)
        test_debug_hook("c", "c");
    if (shook && !client)
        test_debug_hook("s", "s");
    ferr = msg_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    if (client)
        msg_mon_enable_mon_messages(true);
    util_test_start(client);
    msg_mon_enable_mon_messages(true);
    ferr = msg_mon_process_startup(client); // system messages
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_process_info(NULL, &nid, &pid);
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(my_name, sizeof(my_name));
    TEST_CHK_FEOK(ferr);

    if (client) {
        sprintf(prog, "%s/%s", getenv("PWD"), argv[0]);
        for (arg = 0; arg < argc; arg++)
            if (strcmp(argv[arg], "-client") == 0) // start_process
                argv[arg] = (char *) "-server";
        for (inx = 0; inx < loop; inx++) {
            sprintf(server_name, "$SRV%d", inx);
            server_nid = -1;
            if (nowait)
                ferr = msg_mon_start_process_nowait_cb(cb,                       // callback
                                                       prog,                     // prog
                                                       server_name,              // name
                                                       retname,                  // ret-name
                                                       argc,                     // argc
                                                       argv,                     // argv
                                                       MS_ProcessType_Generic,   // type
                                                       0,                        // priority
                                                       false,                    // debug
                                                       false,                    // backup
                                                       inx,                      // tag
                                                       &server_nid,              // nid
                                                       &server_pid,              // pid
                                                       NULL,                     // infile
                                                       NULL);                    // outfile
            else
                ferr = msg_mon_start_process(prog,                   // prog
                                             server_name,            // name
                                             NULL,                   // ret name
                                             argc,
                                             argv,
                                             TPT_REF(server_phandle),
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
            printf("process started, err=%d\n", ferr);
            TEST_CHK_FEOK(ferr);
            if (nowait) {
                status = cv.wait(true);
                TEST_CHK_STATUSOK(status);
            }
            wait_death();
        }
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
