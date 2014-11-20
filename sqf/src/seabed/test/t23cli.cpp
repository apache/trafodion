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

#include "seabed/fserr.h"
#include "seabed/ms.h"

#include "tchkfe.h"
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"


int main(int argc, char *argv[]) {
    bool      client = false;
    int       ferr;
    int       msgid;
    char      my_name[BUFSIZ];
    int       oid;
    char      recv_buffer[BUFSIZ];
    RT        results;
    TPT_DECL (phandle_dtm);
    TPT_DECL (phandle_tse);
    char      send_buffer[BUFSIZ];
    int       send_len;

    msfs_util_init_role(true, &argc, &argv, msg_debug_hook);
    client = true;
    util_test_start(client);
    ferr = msg_mon_process_startup(!client); // system messages
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_open_process((char *) "$dtm",      // name
                                TPT_REF(phandle_dtm),
                                &oid);
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_open_process((char *) "$tse",     // name
                                TPT_REF(phandle_tse),
                                &oid);
    TEST_CHK_FEOK(ferr);
    util_gethostname(my_name, sizeof(my_name));

    strcpy(send_buffer, "to-dtm");
    send_len = (int) strlen(send_buffer) + 1; // cast
    ferr = XMSG_LINK_(TPT_REF(phandle_dtm),        // phandle
                      &msgid,                      // msgid
                      NULL,                        // reqctrl
                      0,                           // reqctrlsize
                      NULL,                        // replyctrl
                      0,                           // replyctrlmax
                      send_buffer,                 // reqdata
                      (ushort) send_len,           // reqdatasize
                      recv_buffer,                 // replydata
                      BUFSIZ,                      // replydatamax
                      0,                           // linkertag
                      0,                           // pri
                      0,                           // xmitclass
                      0);                          // linkopts
    util_check("XMSG_LINK_", ferr);
    ferr = XMSG_BREAK_(msgid, results.u.s, TPT_REF(phandle_dtm));
    util_check("XMSG_BREAK_", ferr);
    assert(results.u.t.ctrl_size == 0);
    assert(results.u.t.data_size > (uint) (send_len + 13));
    assert(results.u.t.errm == RT_DATA_RCVD); // data
    printf("%s\n", recv_buffer);

    strcpy(send_buffer, "to-tse");
    send_len = (int) strlen(send_buffer) + 1; // cast
    ferr = XMSG_LINK_(TPT_REF(phandle_tse),                 // phandle
                      &msgid,                      // msgid
                      NULL,                        // reqctrl
                      0,                           // reqctrlsize
                      NULL,                        // replyctrl
                      0,                           // replyctrlmax
                      send_buffer,                 // reqdata
                      (ushort) send_len,           // reqdatasize
                      recv_buffer,                 // replydata
                      BUFSIZ,                      // replydatamax
                      0,                           // linkertag
                      0,                           // pri
                      0,                           // xmitclass
                      0);                          // linkopts
    util_check("XMSG_LINK_", ferr);
    ferr = XMSG_BREAK_(msgid, results.u.s, TPT_REF(phandle_tse));
    util_check("XMSG_BREAK_", ferr);
    assert(results.u.t.ctrl_size == 0);
    assert(results.u.t.data_size > (uint) (send_len + 13));
    assert(results.u.t.errm == RT_DATA_RCVD); // data
    printf("%s\n", recv_buffer);

    ferr = msg_mon_close_process(TPT_REF(phandle_dtm));
    ferr = msg_mon_close_process(TPT_REF(phandle_tse));
    printf("close ferr=%d\n", ferr);
    printf("client shutting down\n");
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
