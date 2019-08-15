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

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "tchkfe.h"
#include "tutil.h"
#include "tutilp.h"


enum { MAX_SRV = 20 };

int main(int argc, char *argv[]) {
    int         arg;
    bool        chook = false;
    bool        client = false;
    int         ferr;
    int         inx;
    int         lerr;
    int         maxs = 5;
    MS_Mon_Msg *msg;
    char        my_name[BUFSIZ];
    int         nid;
    int         pid;
    char        prog[MS_MON_MAX_PROCESS_PATH];
    char        recv_buffer[BUFSIZ];
    char        server_name[10];
    int         server_nid[MAX_SRV];
    TPT_DECL2  (server_phandle, MAX_SRV);
    int         server_pid[MAX_SRV];
    bool        shook = false;
    MS_SRE      sre;
    TAD         zargs[] = {
      { "-chook",     TA_Bool, TA_NOMAX,    &chook     },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-maxs",      TA_Int,  MAX_SRV,     &maxs      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-shook",     TA_Bool, TA_NOMAX,    &shook     },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    if (chook && client)
        test_debug_hook("c", "c");
    if (shook && !client)
        test_debug_hook("s", "s");
    if (client)
        ferr = msg_init_attach(&argc, &argv, true, (char *) "$CLI");
    else
        ferr = msg_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    if (!client)
        msg_mon_enable_mon_messages(true);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client); // system messages
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
        for (inx = 0; inx < maxs; inx++) {
            sprintf(server_name, "$SRV%d", inx);
            printf("starting process %s\n", server_name);
            server_nid[inx] = -1;
            ferr = msg_mon_start_process(prog,                   // prog
                                         server_name,            // name
                                         NULL,                   // ret name
                                         argc,
                                         argv,
                                         TPT_REF2(server_phandle, inx),
                                         0,                      // open
                                         NULL,                   // oid
                                         MS_ProcessType_Generic, // type
                                         0,                      // priority
                                         0,                      // debug
                                         0,                      // backup
                                         &server_nid[inx],       // nid
                                         &server_pid[inx],       // pid
                                         NULL,                   // infile
                                         NULL);                  // outfile
            printf("process started, err=%d\n", ferr);
            TEST_CHK_FEOK(ferr);
            ferr = msg_mon_register_death_notification2(server_nid[inx],
                                                        server_pid[inx],
                                                        nid,
                                                        pid);
            TEST_CHK_FEOK(ferr);
        }

        printf("i'm the client p-id=%d/%d, name=%s\n", nid, pid, my_name);
        sleep(3);
    } else {
        printf("i'm the server p-id=%d/%d, name=%s\n", nid, pid, my_name);
        do {
            lerr = XWAIT(LREQ, -1);
            TEST_CHK_WAITIGNORE(lerr);
            lerr = XMSG_LISTEN_((short *) &sre, // sre
                                0,              // listenopts
                                0);             // listenertag
        } while (lerr == XSRETYPE_NOWORK);
        assert(sre.sre_flags & XSRE_MON);
        ferr = XMSG_READDATA_(sre.sre_msgId,  // msgid
                              recv_buffer,    // reqdata
                              BUFSIZ);        // bytecount
        util_check("XMSG_READDATA_", ferr);
        msg = (MS_Mon_Msg *) recv_buffer;
#if 0
        assert(msg->type == MS_MsgType_ProcessDeath);
        printf("server name=%s received death message for %s\n",
               my_name, msg->u.death.process_name);
#else
        assert(msg->type == MS_MsgType_Shutdown);
        printf("server name=%s received shutdown message\n",
               my_name);
#endif
        XMSG_REPLY_(sre.sre_msgId,  // msgid
                    NULL,           // replyctrl
                    0,              // replyctrlsize
                    NULL,           // replydata
                    0,              // replydatasize
                    0,              // errorclass
                    NULL);          // newphandle
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
