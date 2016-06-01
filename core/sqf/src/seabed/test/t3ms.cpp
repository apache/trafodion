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

#include <sys/wait.h>

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "tchkfe.h"
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

MS_Mon_Process_Info_Type info[1];
char                     my_name[BUFSIZ];
char                     recv_buffer[BUFSIZ];
short                    recv_buffer2[BUFSIZ];
short                    recv_buffer3[BUFSIZ];
char                     send_buffer[BUFSIZ];
short                    send_buffer2[BUFSIZ];

int main(int argc, char *argv[]) {
    bool            attach = false;
    bool            chook = false;
    bool            client = false;
    const char     *cname = "$cli";
    int             count;
    int             ferr;
    int             inx;
    int             len;
    int             lerr;
    int             loop = 10;
    int             msgid;
    int             oid;
    TPT_DECL       (phandle);
    char           *pname = NULL;
    RT              results;
    int             send_len;
    bool            shook = false;
    const char     *sname = "$srv";
    MS_SRE          sre;
    long            t_elapsed;
    struct timeval  t_start;
    struct timeval  t_stop;
    TAD             zargs[] = {
      { "-attach",    TA_Bool, TA_NOMAX,    &attach    },
      { "-chook",     TA_Bool, TA_NOMAX,    &chook     },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-cname",     TA_Str,  TA_NOMAX,    &cname     },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-name",      TA_Str,  TA_NOMAX,    &pname     },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-shook",     TA_Bool, TA_NOMAX,    &shook     },
      { "-sname",     TA_Str,  TA_NOMAX,    &sname     },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    if (client && chook)
        test_debug_hook("c", "c");
    if (!client && shook)
        test_debug_hook("s", "s");

    if (attach)
        msfs_util_init_attach(&argc, &argv, msg_debug_hook, false, pname);
    else
        msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_time_timer_start(&t_start);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);

    // process-wait for server/shell
    ferr = msfs_util_wait_process_count(MS_ProcessType_Generic, 2, NULL, false);
    TEST_CHK_FEOK(ferr);
    // process-wait for client
    ferr = msfs_util_wait_process_count(MS_ProcessType_TSE, 1, NULL, false);
    TEST_CHK_FEOK(ferr);
    if (client) {
        ferr = msg_mon_open_process((char *) sname,       // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
    }

    util_gethostname(my_name, sizeof(my_name));
    msg_mon_enable_mon_messages(true);
    if (client) {
        ferr = msg_mon_get_process_info_type(MS_ProcessType_TSE,
                                             &count,
                                             0,
                                             NULL);
        TEST_CHK_FEOK(ferr);
        if (!attach)
            assert(count == 1);
        ferr = msg_mon_get_process_info_type(MS_ProcessType_TSE,
                                             &count,
                                             1,
                                             info);
        TEST_CHK_FEOK(ferr);
        if (!attach)
            assert(count == 1);
    }

    for (inx = 0; inx < loop; inx++) {
        if (client) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            send_len = (int) strlen(send_buffer) + 1;
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
                              (short) (inx+1),             // pri
                              0,                           // xmitclass
                              0);                          // linkopts
            util_check("XMSG_LINK_", ferr);
            ferr = XMSG_BREAK_(msgid,
                               results.u.s,
                               TPT_REF(phandle));
            util_check("XMSG_BREAK_", ferr);
            assert(results.u.t.ctrl_size == (uint) (inx & 1));
            assert(results.u.t.data_size > (strlen(send_buffer) + 14));
            assert(results.u.t.errm == RT_DATA_RCVD); // data
            printf("%s\n", recv_buffer);
        } else {
            do {
                lerr = XWAIT(LREQ, -1);
                TEST_CHK_WAITIGNORE(lerr);
                lerr = XMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
            } while (lerr == XSRETYPE_NOWORK);
            ferr = XMSG_READCTRL_(sre.sre_msgId,  // msgid
                                  recv_buffer2,   // reqctrl
                                  1);             // bytecount
            util_check("XMSG_READCTRL_", ferr);
            ferr = XMSG_READDATA_(sre.sre_msgId,  // msgid
                                  recv_buffer,    // reqdata
                                  BUFSIZ);        // bytecount
            util_check("XMSG_READDATA_", ferr);
            if (sre.sre_flags & XSRE_MON) {
                printf("server received mon message\n");
                inx--;
                len = 0;
            } else {
                strcat(recv_buffer, "- reply from ");
                strcat(recv_buffer, my_name);
                len = (int) strlen(recv_buffer) + 1;
            }
            XMSG_REPLY_(sre.sre_msgId,       // msgid
                        recv_buffer2,        // replyctrl
                        sre.sre_reqCtrlSize, // replyctrlsize
                        recv_buffer,         // replydata
                        (ushort) len,        // replydatasize
                        0,                   // errorclass
                        NULL);               // newphandle
        }
    }

    if (client) {
        ferr = msg_mon_close_process(TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
    } else {
        ferr = msg_mon_process_close();
        TEST_CHK_FEOK(ferr);
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
