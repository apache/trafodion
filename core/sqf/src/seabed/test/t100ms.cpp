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

#include "tchkfe.h"
#include "tutil.h"
#include "tutilp.h"


char  recv_buffer[BUFSIZ];
short recv_buffer2[BUFSIZ];
short recv_buffer3[BUFSIZ];

int main(int argc, char *argv[]) {
    bool             chook = false;
    bool             client = false;
    FILE            *f;
    bool             fail = false;
    int              ferr;
    int              lerr;
    int              msgid;
    int              oid;
    TPT_DECL        (phandle);
    MS_Result_Type   results;
    MS_SRE           sre;
    TAD              zargs[] = {
      { "-chook",     TA_Bool, TA_NOMAX,    &chook     },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-cluster",   TA_Ign,  TA_NOMAX,    NULL       },
      { "-fail",      TA_Bool, TA_NOMAX,    &fail      },
      { "-verbose",   TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    if (chook)
        test_debug_hook("c", "c");
    util_test_start(true);
    if (client) {
        f = NULL;
        while (f == NULL) {
            f = fopen("z100", "r");
            if (f == NULL)
                sleep(1);
        }
        fclose(f);
    }
    if (client)
        ferr = msg_init_attach(&argc, &argv, true, (char *) "$CLI");
    else
        ferr = msg_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_process_startup(!client); // system messages
    TEST_CHK_FEOK(ferr);
    if (client) {
        ferr = msg_mon_open_process((char *) "$SRV", TPT_REF(phandle), &oid);
        TEST_CHK_FEOK(ferr);
        if (fail)
            util_abort_core_free();
        ferr = XMSG_LINK_(TPT_REF(phandle),            // phandle
                          &msgid,                      // msgid
                          NULL,                        // reqctrl
                          0,                           // reqctrlsize
                          recv_buffer3,                // replyctrl
                          0,                           // replyctrlmax
                          NULL,                        // reqdata
                          0,                           // reqdatasize
                          recv_buffer,                 // replydata
                          0,                           // replydatamax
                          0,                           // linkertag
                          0,                           // pri
                          0,                           // xmitclass
                          0);                          // linkopts
        TEST_CHK_FEOK(ferr);
        ferr = XMSG_BREAK_(msgid, (short *) &results, TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
    } else {
        f = fopen("z100", "w");
        assert(f != NULL);
        fclose(f);
        for (;;) {
            do {
                lerr = XWAIT(LREQ, 20);
                TEST_CHK_WAITIGNORE(lerr);
                if (!lerr)
                    continue;
                lerr = XMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
            } while (lerr == XSRETYPE_NOWORK);
            if (sre.sre_flags & XSRE_MON)
                printf("server received mon message\n");
            else
                printf("server received regular message\n");
            XMSG_REPLY_(sre.sre_msgId,       // msgid
                        recv_buffer2,        // replyctrl
                        0,                   // replyctrlsize
                        recv_buffer,         // replydata
                        0,                   // replydatasize
                        0,                   // errorclass
                        NULL);               // newphandle
            if ((sre.sre_flags & XSRE_MON) == 0)
                break;
        }
    }

    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);

    util_test_finish(true);
    return 0;
}
