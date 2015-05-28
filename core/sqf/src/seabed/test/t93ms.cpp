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

enum { MAX_SERVERS = 1024 };

char  my_name[BUFSIZ];
int   oid[MAX_SERVERS];
char  pname[BUFSIZ];
char  recv_buffer[BUFSIZ];
short recv_buffer2[BUFSIZ];
short recv_buffer3[BUFSIZ];
char  send_buffer[BUFSIZ];
short send_buffer2[BUFSIZ];


int main(int argc, char *argv[]) {
    bool            client = false;
    bool            done = false;
    int             ferr;
    int             inx;
    int             len;
    int             lerr;
    int             loop = 10;
    int             maxsp = 1;
    int             msgid;
    bool            open_rcvd = false;
    TPT_DECL2      (phandle,MAX_SERVERS);
    int             pinx;
    RT              results;
    int             send_len;
    MS_SRE          sre;
    long            t_elapsed;
    long            t_elapsed_sec;
    struct timeval  t_start;
    struct timeval  t_stop;
    bool            verbose = false;
    TAD             zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-maxsp",     TA_Int,  MAX_SERVERS, &maxsp      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(pname, sizeof(pname));
    TEST_CHK_FEOK(ferr);
    if (client) {
        char serv[BUFSIZ];
        for (pinx = 0; pinx < maxsp; pinx++) {
            sprintf(serv, "$srv%d", pinx + 1);
            ferr = msg_mon_open_process(serv,         // name
                                        TPT_REF2(phandle,pinx),
                                        &oid[pinx]);
            TEST_CHK_FEOK(ferr);
        }
    }

    util_gethostname(my_name, sizeof(my_name));
    msg_mon_enable_mon_messages(true);

    util_time_timer_start(&t_start);
    if (client) {
        for (inx = 0; inx < loop; inx++) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            send_len = (int) strlen(send_buffer) + 1;
            for (pinx = 0; pinx < maxsp; pinx++) {
                if (verbose)
                    printf("%s: sending message, pinx=%d, inx=%d\n",
                           pname, pinx, inx);
                ferr = XMSG_LINK_(TPT_REF2(phandle,pinx),      // phandle
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
                ferr = XMSG_BREAK_(msgid, results.u.s, TPT_REF2(phandle,pinx));
                util_check("XMSG_BREAK_", ferr);
                assert(results.u.t.ctrl_size == (uint) (inx & 1));
                assert(results.u.t.data_size > (strlen(send_buffer) + 14));
                assert(results.u.t.errm == RT_DATA_RCVD); // data
                printf("%s\n", recv_buffer);
            }
        }
        for (pinx = 0; pinx < maxsp; pinx++) {
            ferr = msg_mon_close_process(TPT_REF2(phandle,pinx));
            TEST_CHK_FEOK(ferr);
        }
    } else {
        inx = 0;
        while (!done) {
            lerr = XWAIT(LREQ, -1);
            TEST_CHK_WAITIGNORE(lerr);
            for (;;) {
                lerr = XMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
                if (lerr == XSRETYPE_NOWORK)
                    break;
                ferr = XMSG_READCTRL_(sre.sre_msgId,  // msgid
                                      recv_buffer2,   // reqctrl
                                      1);             // bytecount
                util_check("XMSG_READCTRL_", ferr);
                ferr = XMSG_READDATA_(sre.sre_msgId,  // msgid
                                      recv_buffer,    // reqdata
                                      BUFSIZ);        // bytecount
                util_check("XMSG_READDATA_", ferr);
                if (sre.sre_flags & XSRE_MON) {
                    MS_Mon_Msg *msg = (MS_Mon_Msg *) recv_buffer;
                    const char *type;
                    switch (msg->type) {
                    case MS_MsgType_Close:
                        type = "close";
                        done = true;
                        break;
                    case MS_MsgType_Open:
                        type = "open";
                        open_rcvd = true;
                        break;
                    default:
                        type = "<unknown>";
                    }
                    printf("server (%s) received mon message, type=%s(%d)\n",
                           pname, type, msg->type);
                    len = 0;
                } else {
                    inx++;
                    if (verbose)
                        printf("%s: received non-mon message, inx=%d\n",
                               pname, inx);
                    assert(open_rcvd);
                    strcat(recv_buffer, "- reply from ");
                    strcat(recv_buffer, my_name);
                    strcat(recv_buffer, " - ");
                    strcat(recv_buffer, pname);
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
        assert(inx == loop);
    }
    util_time_timer_stop(&t_stop);
    t_elapsed = (t_stop.tv_sec * 1000000 + t_stop.tv_usec) -
                (t_start.tv_sec * 1000000 + t_start.tv_usec);
    t_elapsed_sec = t_elapsed / 1000000;
    t_elapsed -= t_elapsed_sec * 1000000;

    if (client)
        printf("elapsed=%ld.%ld\n", t_elapsed_sec, t_elapsed);
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
