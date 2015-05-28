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


int main(int argc, char *argv[]) {
    bool            client = false;
    int             ferr;
    int             inx;
    int             inx2;
    int             lerr;
    int             loop = 10;
    int             oid;
    TPT_DECL       (phandle);
    MS_SRE          sre;
    long            t_elapsed;
    struct timeval  t_start;
    struct timeval  t_stop;
    bool            verbose = false;
    TAD             zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);

    util_time_timer_start(&t_start);
    if (client) {
        for (inx = 0; inx < loop; inx++) {
            if (verbose)
                printf("opening inx=%d\n", inx);
            ferr = msg_mon_open_process((char *) "$srv",      // name
                                        TPT_REF(phandle),
                                        &oid);
            TEST_CHK_FEOK(ferr);
            ferr = msg_mon_close_process(TPT_REF(phandle));
            TEST_CHK_FEOK(ferr);
        }
    } else {
        msg_mon_enable_mon_messages(true);
        for (inx = 0; inx < loop; inx++) {
            for (inx2 = 0; inx2 < 2; inx2++) {
                do {
                    lerr = XWAIT(LREQ, -1);
                    TEST_CHK_WAITIGNORE(lerr);
                    lerr = XMSG_LISTEN_((short *) &sre, // sre
                                        0,              // listenopts
                                        0);             // listenertag
                } while (lerr == XSRETYPE_NOWORK);
                if (verbose)
                    printf("replying inx=%d\n", inx);
                XMSG_REPLY_(sre.sre_msgId,  // msgid
                            NULL,           // replyctrl
                            0,              // replyctrlsize
                            NULL,           // replydata
                            0,              // replydatasize
                            0,              // errorclass
                            NULL);          // newphandle
            }
        }
    }
    util_time_timer_stop(&t_stop);
    t_elapsed = (t_stop.tv_sec * 1000000 + t_stop.tv_usec) -
                (t_start.tv_sec * 1000000 + t_start.tv_usec);
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    if (client)
        printf("elapsed time (gettimeofday us)=%ld\n", t_elapsed);
    return 0;
}
