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

short ctrl_buffer[100];
char  my_name[BUFSIZ];
char  recv_buffer[40000];
char  send_buffer[40000];


int main(int argc, char *argv[]) {
    bool            client = false;
    int             disable;
    int             ferr;
    int             lerr;
    int             msgid;
    int             oid;
    MS_Result_Type  results;
    TPT_DECL       (phandle);
    MS_SRE          sre;
    MS_SRE_LDONE    sre_ldone;
    TAD             zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client); // system messages
    TEST_CHK_FEOK(ferr);
    if (client) {
        ferr = msg_mon_open_process((char *) "$srv",      // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
    }
    util_gethostname(my_name, sizeof(my_name));

    if (client) {
        sprintf(send_buffer, "hello, greetings from %s", my_name);
        ferr = XMSG_LINK_(TPT_REF(phandle),            // phandle
                          &msgid,                      // msgid
                          NULL,                        // reqctrl
                          0,                           // reqctrlsize
                          ctrl_buffer,                 // replyctrl
                          0,                           // replyctrlmax
                          send_buffer,                 // reqdata
                          57,                          // reqdatasize
                          recv_buffer,                 // replydata
                          256,                         // replydatamax
                          (SB_Tag_Type) send_buffer,   // linkertag
                          0,                           // pri
                          0,                           // xmitclass
                          XMSG_LINK_LDONEQ);           // linkopts
        util_check("XMSG_LINK_", ferr);
        lerr = XWAIT(LDONE, -1);
        TEST_CHK_WAITIGNORE(lerr);
        lerr = XMSG_LISTEN_((short *) &sre_ldone, // sre
                            0,                    // listenopts
                            0);                   // listenertag
        assert(lerr == XSRETYPE_LDONE);
        assert(sre_ldone.sre_linkTag == (SB_Tag_Type) send_buffer);
        disable = msg_test_assert_disable();
        ferr = XMSG_BREAK_(msgid, (short *) &results, TPT_REF(phandle));
        assert(ferr != XZFIL_ERR_OK);
        msg_test_assert_enable(disable);
    } else {
        do {
            lerr = XWAIT(LREQ, -1);
            lerr = XMSG_LISTEN_((short *) &sre, // sre
                                0,              // listenopts
                                0);             // listenertag
        } while (lerr == XSRETYPE_NOWORK);
        util_abort_core_free();
        return 0;
    }
    if (client) {
        ferr = msg_mon_close_process(TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
        printf("if there were no asserts, all is well - EXPECT server ABEND\n");
    } else {
        ferr = msg_mon_process_close();
        TEST_CHK_FEOK(ferr);
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
