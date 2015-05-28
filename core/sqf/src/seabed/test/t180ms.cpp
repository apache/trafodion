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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "seabed/ms.h"
#include "seabed/fserr.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "tchkfe.h"
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

enum { MAX_CBUF = 1024 * 32 };        // 32 KB
enum { MAX_DBUF = 1024 * 1024 };      // 1 MB

enum { MAX_CLIENTS = 500 };
enum { MAX_SERVERS = 500 };

bool     client = false;
int      maxcp = 1;
char     my_name[BUFSIZ];
char     recv_buffer[MAX_DBUF];
short    recv_buffer2[MAX_CBUF/2];
short    recv_buffer3[MAX_CBUF/2];
char     send_buffer[MAX_DBUF];
short    send_buffer2[MAX_CBUF/2];
char     serv[BUFSIZ];


int main(int argc, char *argv[]) {
    int        csize = 0;
    int        dinx;
    int        disable;
    int        dloop = 10;
    int        dsize = 1024;
    int        ferr;
    TPT_DECL2 (phandle,MAX_SERVERS);
    int        inx;
    int        lerr;
    int        loop = 10;
    int        max;
    int        maxsp = 1;
    int        msgid;
    int        msgids[MAX_SERVERS];
    bool       nowait = false;
    int        oid;
    int        pinx;
    RT         results;
    MS_SRE     sre;
    bool       toomany = false;
    bool       verbose = false;
    TAD        zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-csize",     TA_Int,  MAX_CBUF,    &csize     },
      { "-dloop",     TA_Int,  TA_NOMAX,    &dloop     },
      { "-dsize",     TA_Int,  MAX_DBUF,    &dsize     },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-maxcp",     TA_Int,  MAX_CLIENTS, &maxcp     },
      { "-maxsp",     TA_Int,  MAX_SERVERS, &maxsp     },
      { "-nowait",    TA_Bool, TA_NOMAX,    &nowait    },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-toomany",   TA_Bool, TA_NOMAX,    &toomany   },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    ferr = msg_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    if (maxcp < 0)
        maxcp = 1;
    if (maxsp < 0)
        maxsp = 1;
    ferr = msg_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    if (!client)
        msg_mon_enable_mon_messages(true);
    ferr = msg_mon_get_my_process_name(my_name, BUFSIZ);
    TEST_CHK_FEOK(ferr);

    // process-wait for clients/servers/shell
    ferr = msfs_util_wait_process_count(MS_ProcessType_Generic,
                                        maxcp + maxsp + 1,
                                        NULL,
                                        verbose);
    TEST_CHK_FEOK(ferr);
    sleep(3); // wait for all process_count's to complete
    if (client) {
        pinx = atoi(&my_name[4]);
        assert(pinx >= 0);
        printf("loop=%d\n", loop);
        for (inx = 0; inx < loop; inx++) {
            for (pinx = 0; pinx < maxsp; pinx++) {
                sprintf(serv, "$srv%d", pinx);
                if (toomany)
                    disable = msg_test_assert_disable();
                else
                    disable = 0;
                ferr = msg_mon_open_process(serv,
                                            TPT_REF2(phandle,pinx),
                                            &oid);
                if (toomany) {
                    msg_test_assert_enable(disable);
                    if (ferr == XZFIL_ERR_TOOMANY) {
                        sleep(1);
                        pinx--;
                        continue;
                    }
                }
                TEST_CHK_FEOK(ferr);
            }
            for (pinx = 0; pinx < maxsp; pinx++) {
                if (nowait) {
                    for (dinx = 0; dinx < dloop; dinx++) {
                        ferr = XMSG_LINK_(TPT_REF2(phandle,pinx), // phandle
                                          &msgids[dinx],          // msgid
                                          send_buffer2,           // reqctrl
                                          (short) csize,          // reqctrlsize
                                          recv_buffer3,           // replyctrl
                                          (short) csize,          // replyctrlmax
                                          send_buffer,            // reqdata
                                          (short) dsize,          // reqdatasize
                                          recv_buffer,            // replydata
                                          (short) dsize,          // replydatamax
                                          0,                      // linkertag
                                          0,                      // pri
                                          0,                      // xmitclass
                                          0);                     // linkopts
                        util_check("XMSG_LINK_", ferr);
                    }
                    for (dinx = 0; dinx < dloop; dinx++) {
                        ferr = XMSG_BREAK_(msgids[dinx],
                                           results.u.s,
                                           TPT_REF2(phandle,pinx));
                        util_check("XMSG_BREAK_", ferr);
                        assert((int) results.u.t.ctrl_size == csize);
                        assert((int) results.u.t.data_size == dsize);
                        assert(results.u.t.errm == RT_DATA_RCVD); // data
                    }
                } else {
                    for (dinx = 0; dinx < dloop; dinx++) {
                        ferr = XMSG_LINK_(TPT_REF2(phandle,pinx), // phandle
                                          &msgid,                 // msgid
                                          send_buffer2,           // reqctrl
                                          (short) csize,          // reqctrlsize
                                          recv_buffer3,           // replyctrl
                                          (short) csize,          // replyctrlmax
                                          send_buffer,            // reqdata
                                          (short) dsize,          // reqdatasize
                                          recv_buffer,            // replydata
                                          (short) dsize,          // replydatamax
                                          0,                      // linkertag
                                          0,                      // pri
                                          0,                      // xmitclass
                                          0);                     // linkopts
                        util_check("XMSG_LINK_", ferr);
                        ferr = XMSG_BREAK_(msgid,
                                           results.u.s,
                                           TPT_REF2(phandle,pinx));
                        util_check("XMSG_BREAK_", ferr);
                        assert((int) results.u.t.ctrl_size == csize);
                        assert((int) results.u.t.data_size == dsize);
                        assert(results.u.t.errm == RT_DATA_RCVD); // data
                    }
                }
                if (verbose)
                    printf("%s-open-count=%d\n", my_name, inx);
            }
            for (pinx = 0; pinx < maxsp; pinx++) {
                ferr = msg_mon_close_process(TPT_REF2(phandle,pinx));
                TEST_CHK_FEOK(ferr);
                if (verbose)
                    printf("%s-close-count=%d\n", my_name, inx);
            }
        }
    } else {
        max = 2 * maxcp * loop + maxcp * dloop * loop;
        for (inx = 0; inx < max; inx++) {
            do {
                lerr = XWAIT(LREQ, -1);
                TEST_CHK_WAITIGNORE(lerr);
                lerr = XMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
            } while (lerr == XSRETYPE_NOWORK);
            ferr = XMSG_READCTRL_(sre.sre_msgId,        // msgid
                                  recv_buffer2,         // reqctrl
                                  sre.sre_reqCtrlSize); // bytecount
            util_check("XMSG_READCTRL_", ferr);
            ferr = XMSG_READDATA_(sre.sre_msgId,        // msgid
                                  recv_buffer,          // reqdata
                                  sre.sre_reqDataSize); // bytecount
            util_check("XMSG_READDATA_", ferr);
            XMSG_REPLY_(sre.sre_msgId,         // msgid
                        recv_buffer2,          // replyctrl
                        sre.sre_reqCtrlSize,   // replyctrlsize
                        recv_buffer,           // replydata
                        sre.sre_reqDataSize,   // replydatasize
                        0,                     // errorclass
                        NULL);                 // newphandle
            if (verbose)
                printf("%s-count=%d\n", my_name, inx);
        }
    }

    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
