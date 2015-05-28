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
//#include "seabed/thread.h"
//#include "seabed/time.h"

#include "tchkfe.h"
//#include "tchkos.h"
//#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"


TPT_DECL (cphandle);
TPT_DECL (sphandle);

char cname[BUFSIZ];
char cprog[MS_MON_MAX_PROCESS_PATH];
char recv_buffer[BUFSIZ];

int main(int argc, char *argv[]) {
    bool      client = false;
    bool      client2 = false;
    int       close_count;
    int       cnid;
    int       cpid;
    int       ferr;
    bool      fin = false;
    int       inx;
    int       lerr;
    int       loop = 10;
    int       oid;
    char     *sname = (char *) "$srv";
    BMS_SRE   sre;
    bool      verbose = false;
    TAD       zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-client2",   TA_Bool, TA_NOMAX,    &client2   },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "-verbose",   TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    ferr = msg_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    util_test_start(client);
    ferr = msg_mon_process_startup(true); // system messages
    TEST_CHK_FEOK(ferr);
    if (client) {
        msg_debug_hook("c", "c");
        sprintf(cprog, "%s/%s", getenv("PWD"), argv[0]);
        for (inx = 0; inx < argc; inx++) {
            if (strcmp(argv[inx], "-client") == 0)
                argv[inx] = (char *) "-client2";
        }
        for (inx = 0; inx < loop; inx++) {
            if (verbose)
                printf("cli: newproc, inx=%d\n", inx);
            sprintf(cname, "$cli%d", inx);
            cnid = -1;
            ferr = msg_mon_start_process(cprog,                  // prog
                                         cname,                  // name
                                         NULL,                   // ret_name
                                         argc,                   // argc
                                         argv,                   // argv
                                         TPT_REF(cphandle),      // phandle
                                         false,                  // open
                                         NULL,                   // oid
                                         MS_ProcessType_Generic, // type
                                         0,                      // priority
                                         false,                  // debug
                                         false,                  // backup
                                         &cnid,                  // nid
                                         &cpid,                  // pid
                                         NULL,                   // infile
                                         NULL);                  // outfile
            TEST_CHK_FEOK(ferr);
        }
    } else if (client2) {
        if (verbose)
            printf("cli: open\n");
        ferr = msg_mon_open_process(sname,  // name
                                    TPT_REF(sphandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
    } else {
        msg_mon_enable_mon_messages(true);
        close_count = 0;
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
                    if (++close_count >= loop)
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

    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
