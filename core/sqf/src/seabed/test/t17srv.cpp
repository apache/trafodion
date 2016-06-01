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

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"


int main(int argc, char *argv[]) {
    int     ferr;
    int     inx;
    int     len;
    int     lerr;
    int     loop = 10;
    char    my_name[BUFSIZ];
    char    recv_buffer[BUFSIZ];
    short   recv_buffer2[BUFSIZ];
    MS_SRE  sre;
    bool    verbose = false;
    TAD     zargs[] = {
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init_role(false, &argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    ferr = msg_mon_process_startup(true); // system mesages
    TEST_CHK_FEOK(ferr);
    util_gethostname(my_name, sizeof(my_name));
    // process-wait for client/server/shell
    ferr = msfs_util_wait_process_count(MS_ProcessType_Generic, 4, NULL, false);
    TEST_CHK_FEOK(ferr);

    for (inx = 0; inx < loop; inx++) {
        do {
            lerr = XWAIT(LREQ, -1);
            TEST_CHK_WAITIGNORE(lerr);
            lerr = XMSG_LISTEN_((short *) &sre, // sre
                                0,              // listenopts
                                0);             // listenertag
        } while (lerr == XSRETYPE_NOWORK);
        if (verbose)
            printf("srv received msg, inx=%d\n", inx);
        ferr = XMSG_READCTRL_(sre.sre_msgId,  // msgid
                              recv_buffer2,   // reqctrl
                              1);             // bytecount
        util_check("XMSG_READCTRL_", ferr);
        ferr = XMSG_READDATA_(sre.sre_msgId,  // msgid
                              recv_buffer,    // reqdata
                              BUFSIZ);        // bytecount
        util_check("XMSG_READDATA_", ferr);
        strcat(recv_buffer, "- reply from ");
        strcat(recv_buffer, my_name);
        len = (int) strlen(recv_buffer) + 1;
        XMSG_REPLY_(sre.sre_msgId,       // msgid
                    recv_buffer2,        // replyctrl
                    sre.sre_reqCtrlSize, // replyctrlsize
                    recv_buffer,         // replydata
                    (ushort) len,        // replydatasize
                    0,                   // errorclass
                    NULL);               // newphandle
    }
    if (verbose)
        printf("srv doing shutdown\n");
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    return 0;
}
