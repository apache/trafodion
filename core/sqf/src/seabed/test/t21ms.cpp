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

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"


void do_readmsg() {
    int     lerr;
    MS_SRE  sre;

    do {
        lerr = XWAIT(LREQ, -1);
        TEST_CHK_WAITIGNORE(lerr);
        lerr = XMSG_LISTEN_((short *) &sre, // sre
                            0,              // listenopts
                            0);             // listenertag
    } while (lerr == XSRETYPE_NOWORK);
    XMSG_REPLY_(sre.sre_msgId,       // msgid
                NULL,                // replyctrl
                0,                   // replyctrlsize
                NULL,                // replydata
                0,                   // replydatasize
                0,                   // errorclass
                NULL);               // newphandle
}

int main(int argc, char *argv[]) {
    bool      client = false;
    int       ferr;
    char      my_name[BUFSIZ];
    int       oid;
    TPT_DECL (phandle);
    TAD       zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msg_mon_enable_mon_messages(true);
    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(true);           // system messages
    TEST_CHK_FEOK(ferr);
    // process-wait for client/server/shell
    ferr = msfs_util_wait_process_count(MS_ProcessType_Generic, 3, NULL, false);
    TEST_CHK_FEOK(ferr);
    if (client) {
        ferr = msg_mon_open_process((char *) "$srv",      // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
        do_readmsg(); // open message
    } else {
        do_readmsg(); // open message
        ferr = msg_mon_open_process((char *) "$cli",      // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
    }
    util_gethostname(my_name, sizeof(my_name));

    if (client) {
        ferr = msg_mon_close_process(TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_process_close();
        TEST_CHK_FEOK(ferr);
        printf("if there were no asserts, all is well\n");
    } else {
        ferr = msg_mon_process_close();
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_close_process(TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
