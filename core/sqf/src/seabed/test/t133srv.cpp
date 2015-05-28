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
#include <unistd.h>

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "tchkfe.h"

int main(int argc, char *argv[]) {
    int     exit = false;
    int     err;
    int     ferr;
    int     len;
    char    recv_buffer[BUFSIZ];
    BMS_SRE sre;

    ferr = msg_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_process_startup(true);  // system messages?
    TEST_CHK_FEOK(ferr);
    msg_mon_enable_mon_messages(true);

    for (;;) {
        do {
            err = XWAIT(LREQ, -1);
            TEST_CHK_WAITIGNORE(err);
            err = BMSG_LISTEN_((short *) &sre, // sre
                               0,              // listenopts
                               0);             // listenertag
        } while (err == BSRETYPE_NOWORK);
        ferr = BMSG_READDATA_(sre.sre_msgId,  // msgid
                              recv_buffer,    // reqdata
                              BUFSIZ);        // bytecount
        TEST_CHK_FEOK(ferr);
        if ((sre.sre_flags & BSRE_MON)) {
            MS_Mon_Msg *msg = (MS_Mon_Msg *) recv_buffer;
            if (msg->type == MS_MsgType_Close)
                exit = true;
        }
        strcat(recv_buffer, "- reply from server");
        len = (int) strlen(recv_buffer) + 1;
        BMSG_REPLY_(sre.sre_msgId,       // msgid
                    NULL,                // replyctrl
                    0,                   // replyctrlsize
                    recv_buffer,         // replydata
                    len,                 // replydatasize
                    0,                   // errorclass
                    NULL);               // newphandle
        if (exit)
            break;
    }

    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    return 0;
}
