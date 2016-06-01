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
short recv_buffer2[BUFSIZ];
short recv_buffer3[BUFSIZ];
char  send_buffer[BUFSIZ];
short send_buffer2[BUFSIZ];

int main(int argc, char *argv[]) {
    bool        client = false;
    int         ferr;
    bool        forkexec = false;
    int         inx;
    int         len;
    int         lerr;
    int         loop = 10;
    int         msgid;
    const char *name;
    int         nid;
    int         pid;
    int         oid;
    TPT_DECL   (phandle);
    RT          results;
    int         send_len;
    MS_SRE      sre;
    TAD         zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-forkexec",  TA_Bool, TA_NOMAX,    &forkexec  },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    if (client)
        name = "$cli";
    else
        name = "$srv";
    msfs_util_init_attach(&argc, &argv, msg_debug_hook, forkexec, (char *) name);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    if (client) {
        do {
            ferr = msg_mon_get_process_info((char *) "$srv", &nid, &pid);
            if (ferr != XZFIL_ERR_OK)
                printf("msg_mon_get_process_info ret=%d\n", ferr);
        } while (ferr != XZFIL_ERR_OK);
        ferr = msg_mon_open_process((char *) "$srv",      // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
    }

    util_gethostname(my_name, sizeof(my_name));

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
    }

    if (client) {
        ferr = msg_mon_close_process(TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
