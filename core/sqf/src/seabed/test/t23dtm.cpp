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
#include "tutil.h"

char  my_name[BUFSIZ];
char  recv_buffer[BUFSIZ];
short recv_buffer2[BUFSIZ];
char  recv_buffer3[BUFSIZ];

int main(int argc, char *argv[]) {
    int       exit;
    int       ferr;
    int       len;
    int       lerr;
    int       msgid;
    int       oid;
    TPT_DECL (phandle_tse);
    RT        results;
    MS_SRE    sre;

    ferr = msg_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_process_startup(true); // system messages
    TEST_CHK_FEOK(ferr);
    util_gethostname(my_name, sizeof(my_name));
    msg_mon_enable_mon_messages(true);

    ferr = msg_mon_open_process((char *) "$tse",      // name
                                TPT_REF(phandle_tse),
                                &oid);
    TEST_CHK_FEOK(ferr);
    exit = 0;
    while (!exit) {
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
            MS_Mon_Msg *msg = (MS_Mon_Msg *) recv_buffer;
            printf("dtm received mon message, type=%d\n",
                   msg->type);
            if (msg->type == MS_MsgType_Close)
                exit = 1;
            len = 0;
        } else {
            ferr = XMSG_LINK_(TPT_REF(phandle_tse),        // phandle
                              &msgid,                      // msgid
                              NULL,                        // reqctrl
                              0,                           // reqctrlsize
                              NULL,                        // replyctrl
                              0,                           // replyctrlmax
                              recv_buffer,                 // reqdata
                              sre.sre_reqDataSize,         // reqdatasize
                              recv_buffer3,                // replydata
                              BUFSIZ,                      // replydatamax
                              0,                           // linkertag
                              0,                           // pri
                              0,                           // xmitclass
                              0);                          // linkopts
            util_check("XMSG_LINK_", ferr);
            ferr = XMSG_BREAK_(msgid, results.u.s, TPT_REF(phandle_tse));
            util_check("XMSG_BREAK_", ferr);
            assert(results.u.t.ctrl_size == 0);
            if (results.u.t.data_size <= (uint) (sre.sre_reqDataSize + 13))
                printf("results.data_size=%d, exp=%d\n",
                       (int) results.u.t.data_size, sre.sre_reqDataSize + 7);
            assert(results.u.t.data_size > (uint) (sre.sre_reqDataSize + 13));
            assert(results.u.t.errm == RT_DATA_RCVD); // data
            printf("%s\n", recv_buffer);
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
    ferr = msg_mon_close_process(TPT_REF(phandle_tse));
    TEST_CHK_FEOK(ferr);
    printf("dtm shutting down\n");
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    return 0;
}
