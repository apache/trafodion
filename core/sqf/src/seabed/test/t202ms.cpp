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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "tchkfe.h"
#include "tutil.h"
#include "tutilp.h"


int main(int argc, char *argv[]) {
    int         arg;
    bool        chook = false;
    bool        client = false;
    int         ferr;
    int         lerr;
    char        my_name[BUFSIZ];
    char        prog[MS_MON_MAX_PROCESS_PATH];
    char        recv_buffer[30000];
    char        ret_name[MS_MON_MAX_PROCESS_NAME];
    const char *server_name = "$SRV";
    int         server_nid = 1;
    TPT_DECL   (server_phandle);
    int         server_pid;
    BMS_SRE     sre;
    bool        shook = false;
    TAD         zargs[] = {
      { "-chook",     TA_Bool, TA_NOMAX,    &chook     },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-shook",     TA_Bool, TA_NOMAX,    &shook     },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    if (chook && client)
        test_debug_hook("c", "c");
    if (shook && !client)
        test_debug_hook("s", "s");
    util_gethostname(my_name, sizeof(my_name));
    if (client) {
        ferr = msg_init_attach(&argc, &argv, true, (char *) "$CLI");
        TEST_CHK_FEOK(ferr);
        util_test_start(true);
        ferr = msg_mon_process_startup(client); // system messages
        TEST_CHK_FEOK(ferr);
        msg_mon_enable_mon_messages(true);

        sprintf(prog, "%s/%s", getenv("PWD"), argv[0]);
        for (arg = 0; arg < argc; arg++)
            if (strcmp(argv[arg], "-client") == 0) // start_process
                argv[arg] = (char *) "-server";
        printf("starting process\n");
        ferr = msg_mon_start_process(prog,                   // prog
                                     (char *) server_name,   // name
                                     ret_name,               // ret name
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
        assert(strcmp(ret_name, server_name) == 0);
        ferr = msg_mon_register_death_notification(server_nid, server_pid);
        TEST_CHK_FEOK(ferr);

        printf("i'm the client - wait for server death\n");

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
        TEST_CHK_FEOK(ferr);
        MS_Mon_Msg *msg = (MS_Mon_Msg *) recv_buffer;
        assert(msg->type == MS_MsgType_ProcessDeath);
        printf("process death process=%s\n", msg->u.death.process_name);
        BMSG_REPLY_(sre.sre_msgId,  // msgid
                    NULL,           // replyctrl
                    0,              // replyctrlsize
                    recv_buffer,    // replydata
                    0,              // replydatasize
                    0,              // errorclass
                    NULL);          // newphandle

        ferr = msg_mon_process_shutdown();
        TEST_CHK_FEOK(ferr);
        util_test_finish(true);
    } else {
        ferr = msg_init(&argc, &argv);
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_process_startup2(false, false); // no sys/event msgs
        TEST_CHK_FEOK(ferr);
        char *largs[] = { (char *) "t202s", (char *) "myarg", NULL };
        char path[PATH_MAX];
        sprintf(path, "%s/t202s", getenv("PWD"));
        execv(path, largs);
    }
    return 0;
}
