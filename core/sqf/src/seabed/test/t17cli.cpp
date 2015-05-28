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
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

char  my_name[BUFSIZ];
char  recv_buffer[BUFSIZ];
short recv_buffer3[BUFSIZ];
char  send_buffer[BUFSIZ];
short send_buffer2[BUFSIZ];


int main(int argc, char *argv[]) {
    int       ferr;
    int       inx;
    int       lerr;
    int       loop = 10;
    int       msgid;
    int       oid;
    RT        results;
    TPT_DECL (phandle);
    int       send_len;
    MS_SRE    sre;
    bool      verbose = false;
    TAD       zargs[] = {
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init_role(true, &argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(true);
    ferr = msg_mon_process_startup(true); // system messages
    TEST_CHK_FEOK(ferr);
    // process-wait for client/server/shell
    ferr = msfs_util_wait_process_count(MS_ProcessType_Generic, 4, NULL, false);
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_open_process((char *) "$srv",      // name
                                TPT_REF(phandle),
                                &oid);
    TEST_CHK_FEOK(ferr);
    util_gethostname(my_name, sizeof(my_name));

    for (inx = 0; inx < loop; inx++) {
        sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                my_name, inx);
        send_len = (int) strlen(send_buffer) + 1;
        if (verbose)
            printf("cli linking srv, inx=%d\n", inx);
        ferr = XMSG_LINK_(TPT_REF(phandle),            // phandle
                          &msgid,                      // msgid
                          send_buffer2,                // reqctrl
                          (ushort) (inx & 1),          // reqctrlsize
                          recv_buffer3,                // replyctrl
                          1,                           // replyctrlmax
                          send_buffer,                 // reqdata
                          (ushort) send_len,           // reqdatasize
                          recv_buffer,                 // replydata
                          BUFSIZ,                      // replydatamax
                          0,                           // linkertag
                          0,                           // pri
                          0,                           // xmitclass
                          0);                          // linkopts
        util_check("XMSG_LINK_", ferr);
        ferr = XMSG_BREAK_(msgid, results.u.s, TPT_REF(phandle));
        util_check("XMSG_BREAK_", ferr);
        assert(results.u.t.ctrl_size == (uint) (inx & 1));
        assert(results.u.t.data_size > (strlen(send_buffer) + 14));
        assert(results.u.t.errm == RT_DATA_RCVD); // data
        printf("%s\n", recv_buffer);
    }
        if (verbose)
            printf("cli closing srv\n");
    ferr = msg_mon_close_process(TPT_REF(phandle));
    TEST_CHK_FEOK(ferr);

    //
    // Pick up open/close from svc.
    //
    msg_mon_enable_mon_messages(true); // get mon messages
    for (inx = 0; inx < 2; inx++) {
        do {
            lerr = XWAIT(LREQ, -1);
            TEST_CHK_WAITIGNORE(lerr);
            lerr = XMSG_LISTEN_((short *) &sre, // sre
                                0,              // listenopts
                                0);             // listenertag
        } while (lerr == XSRETYPE_NOWORK);
        assert(sre.sre_flags & XSRE_MON);
        if (verbose)
            printf("cli received mon msg inx=%d\n", inx);
        XMSG_REPLY_(sre.sre_msgId,       // msgid
                    NULL,                // replyctrl
                    0,                   // replyctrlsize
                    NULL,                // replydata
                    0,                   // replydatasize
                    0,                   // errorclass
                    NULL);               // newphandle
    }
    if (verbose)
        printf("cli doing shutdown\n");
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(true);
    return 0;
}
