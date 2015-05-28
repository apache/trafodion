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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "seabed/fs.h"
#include "seabed/fserr.h"
#include "seabed/ms.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"

#define MAX_CYCLES  100

#include "t25.h"

#define USE_OPEN

char      gdisplay_name[MS_MON_MAX_PROCESS_NAME];
int       gnid = -1;
int       gpid = -1;
int       gserver_oid;
#ifdef USE_OPEN
TPT_DECL (gserver_phandle);
#else
TPT_PTR  (gserver_phandlep);
#endif

#include "t25.cpp.h"

int main(int argc, char *argv[]) {
    int            cycle;
    int            disable;
    int            ferr;
#ifndef USE_OPEN
    int            lnid;
    int            lpid;
#endif
    int            msgid;
    char           process_name[MS_MON_MAX_PROCESS_NAME];
    char           recvbuf[100];
    MS_Result_Type results;
    int            retries;
    int            sendbuf[3];

    ferr = msfs_util_init_role(true, &argc, &argv, msg_debug_hook);
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(gdisplay_name, sizeof(gdisplay_name));
    assert(ferr == XZFIL_ERR_OK);
    myprintf("processing startup.\n");
    util_test_start(true);
    ferr = msg_mon_process_startup(true); // system messages
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_process_info(gdisplay_name, &gnid, &gpid);
    TEST_CHK_FEOK(ferr);

    strcpy(process_name, "$srv");
    myprintf("opening server %s.\n", process_name);
#ifdef USE_OPEN
    ferr = msg_mon_open_process(process_name, TPT_REF(gserver_phandle), &gserver_oid);
    TEST_CHK_FEOK(ferr);
#else
    gserver_phandlep = msg_get_phandle(process_name);
    assert(gserver_phandlep != NULL);
#endif

    // do the work
    myprintf("Starting work with server\n");
    disable = msg_test_assert_disable();
    for (cycle = 0; cycle < MAX_CYCLES + 1; cycle++) {
        sendbuf[0] = gpid;
        sendbuf[1] = cycle;
        switch (cycle) {
        case 5:
            sendbuf[2] = CMD_ABORT;
            break;
        case 6:
            sendbuf[2] = CMD_CP_AB;
            break;
        case MAX_CYCLES:
            sendbuf[2] = CMD_END;
            break;
        default:
            sendbuf[2] = CMD_CONT;
        }
        retries = 5;
        do {
            myprintf("sending %d.%d.%d to server\n",
                     sendbuf[0], sendbuf[1], sendbuf[2]);
#ifdef USE_OPEN
            ferr = XMSG_LINK_(TPT_REF(gserver_phandle),             // phandle
#else
            ferr = XPROCESSHANDLE_DECOMPOSE_(gserver_phandlep, &lnid, &lpid);
            assert(ferr == XZFIL_ERR_OK);
            printf("phandle nid=%d, pid=%d\n", lnid, lpid);

            ferr = XMSG_LINK_(gserver_phandlep,            // phandle
#endif
                              &msgid,                      // msgid
                              NULL,                        // reqctrl
                              0,                           // reqctrlsize
                              NULL,                        // replyctrl
                              0,                           // replyctrlmax
                              (char *) sendbuf,            // reqdata
                              sizeof(sendbuf),             // reqdatasize
                              recvbuf,                     // replydata
                              sizeof(recvbuf),             // replydatamax
                              0,                           // linkertag
                              0,                           // pri
                              0,                           // xmitclass
                              0);                          // linkopts
            TEST_CHK_FEOK(ferr);
            ferr = XMSG_BREAK_(msgid,
                               (short *) &results,
#ifdef USE_OPEN
                               TPT_REF(gserver_phandle));
#else
                               gserver_phandlep);
#endif
            if (ferr == XZFIL_ERR_OK)
                myprintf("Cycle # %d - %s\n", cycle, recvbuf);
            else
                myprintf("XMSG_BREAK_ error ferr=%d\n", ferr);
            if (ferr != XZFIL_ERR_OK) {
                retries--;
                sleep(1);
                if (retries <= 0) {
                    myprintf("retries exhausted\n");
                    assert(retries > 0);
                }
                myprintf("retrying\n");
            } else
                retries = 0;
        } while (retries > 0);
    }
    msg_test_assert_enable(disable);

    // close the server processes
    myprintf("closing server %s.\n", process_name);
#ifdef USE_OPEN
    ferr = msg_mon_close_process(TPT_REF(gserver_phandle));
#else
    ferr = msg_mon_close_process(gserver_phandlep);
#endif
    TEST_CHK_FEOK(ferr);

    myprintf("sending exit process message.\n");
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(true);
    return 0;
}
