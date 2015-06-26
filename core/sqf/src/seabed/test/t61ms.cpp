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
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"


enum { MAX_BUF = 0x40000 };

void check_buf(void *send, void *recv, int count) {
    char *lrecv = (char *) recv;
    char *lsend = (char *) send;

    for (int inx = 0; inx < count; inx++) {
        if (lsend[inx] != lrecv[inx]) {
            printf("buffer mismatch at index=%d, expected=%x, actual=%x\n",
                   inx, lsend[inx], lrecv[inx]);
            assert(lsend[inx] == lrecv[inx]);
        }
    }
}


char  my_name[BUFSIZ];
char  recv_buffer[MAX_BUF];
short recv_buffer2[MAX_BUF];
short recv_buffer3[MAX_BUF];
char  send_buffer[MAX_BUF];
short send_buffer2[MAX_BUF];

int main(int argc, char *argv[]) {
    bool       check = false;
    int        csize = -1;
    bool       client = false;
    int        dsize = MAX_BUF;
    int        ferr;
    int        inx;
    int        len;
    int        lerr;
    int        loop = 10;
    int        min_csize;
    int        min_dsize;
    int        max_csize = -1;
    int        max_dsize = -1;
    int        msgid;
    int        oid;
    TPT_DECL  (phandle);
    uint       result;
    RT         results;
    BMS_SRE    sre;
    bool       verbose = false;
    TAD        zargs[] = {
      { "-check",     TA_Bool, TA_NOMAX,    &check     },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-csize",     TA_Int,  MAX_BUF,     &csize     },
      { "-dsize",     TA_Int,  MAX_BUF,     &dsize     },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-maxcsize",  TA_Int,  MAX_BUF,     &max_csize },
      { "-maxdsize",  TA_Int,  MAX_BUF,     &max_dsize },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    if (client) {
        ferr = msg_mon_open_process((char *) "$srv",      // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
    }

    util_gethostname(my_name, sizeof(my_name));
    msg_mon_enable_mon_messages(true);

    if (check) {
        if (csize < 0)
            csize = 0;
        if (dsize == MAX_BUF)
            dsize = 0;
        min_csize = csize;
        min_dsize = dsize;
        min_csize = min_csize; // touch
        min_dsize = min_dsize; // touch
        if (client) {
            for (inx = 0; inx < MAX_BUF; inx++) {
                send_buffer[inx] = (char) inx; // cast
                send_buffer2[inx] = (short) inx; // cast
            }
        }
    } else
        min_csize = 0;
    for (inx = 0; inx < loop; inx++) {
        if (verbose)
            printf("client=%d, csize=%d, dsize=%d\n", client, csize, dsize);
        else if (check && (csize == min_csize))
            printf("client=%d, csize=%d, dsize=%d\n", client, csize, dsize);
        if (client) {
            if (check) {
                send_buffer[0] = (char) dsize; // cast
                send_buffer2[0] = (short) csize; // cast
                len = 0;
            } else {
                sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                        my_name, inx);
                len = (int) strlen(send_buffer) + 1; // cast
            }
            ferr = BMSG_LINK_(TPT_REF(phandle),            // phandle
                              &msgid,                      // msgid
                              send_buffer2,                // reqctrl
                              csize < 0 ?                  // reqctrlsize
                                (inx & 1) : csize,
                              recv_buffer3,                // replyctrl
                              csize < 0 ? 1 : csize,       // replyctrlmax
                              send_buffer,                 // reqdata
                              check ? dsize : len,         // reqdatasize
                              recv_buffer,                 // replydata
                              MAX_BUF,                     // replydatamax
                              0,                           // linkertag
                              0,                           // pri
                              0,                           // xmitclass
                              0);                          // linkopts
            util_check("BMSG_LINK_", ferr);
            ferr = BMSG_BREAK_(msgid, results.u.s, TPT_REF(phandle));
            util_check("BMSG_BREAK_", ferr);
            if (check)
                assert(results.u.t.ctrl_size == (uint) csize);
            else
                assert(results.u.t.ctrl_size == (uint) (csize < 0 ? (inx & 1) : csize));
            if (check)
                assert(results.u.t.data_size == (uint) dsize);
            else
                assert(results.u.t.data_size > (strlen(send_buffer) + 14));
            if (check) {
                if (dsize)
                    result = RT_DATA_RCVD; // data
                else
                    result = 0x0; // no-data
                assert(results.u.t.errm == result);
                check_buf(send_buffer2, recv_buffer3, csize);
                check_buf(send_buffer, recv_buffer, dsize);
            } else {
                assert(results.u.t.errm == RT_DATA_RCVD); // data
                printf("%s\n", recv_buffer);
            }
        } else {
            do {
                lerr = XWAIT(LREQ, -1);
                TEST_CHK_WAITIGNORE(lerr);
                lerr = BMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
            } while (lerr == XSRETYPE_NOWORK);
            ferr = BMSG_READCTRL_(sre.sre_msgId,          // msgid
                                  recv_buffer2,           // reqctrl
                                  sre.sre_reqCtrlSize);   // bytecount
            util_check("BMSG_READCTRL_", ferr);
            ferr = BMSG_READDATA_(sre.sre_msgId,        // msgid
                                  recv_buffer,          // reqdata
                                  sre.sre_reqDataSize); // bytecount
            util_check("BMSG_READDATA_", ferr);
            if (sre.sre_flags & XSRE_MON) {
                printf("server received mon message\n");
                inx--;
                len = 0;
            } else {
                if (!check) {
                    strcat(recv_buffer, "- reply from ");
                    strcat(recv_buffer, my_name);
                    len = (int) strlen(recv_buffer) + 1; // cast
                } else
                    len = 0;
            }
            BMSG_REPLY_(sre.sre_msgId,                     // msgid
                        recv_buffer2,                      // replyctrl
                        sre.sre_reqCtrlSize,               // replyctrlsize
                        recv_buffer,                       // replydata
                        check ? sre.sre_reqDataSize : len, // replydatasize
                        0,                                 // errorclass
                        NULL);                             // newphandle
            if (sre.sre_flags & XSRE_MON)
                continue;
        }
        if (check) {
            if (max_csize >= 0) {
                csize++;
                if (csize >= max_csize)
                    csize = min_csize;
            }
            if ((max_dsize >= 0) && (csize == min_csize))
                dsize++;
            if (dsize >= max_dsize) {
                printf("break client=%d, csize=%d, dsize=%d\n", client, csize, dsize);
                break;
            }
            if ((max_csize >= 0) || (max_dsize >= 0)) {
                inx--;
                continue;
            }
        }
    }

    if (client) {
        ferr = msg_mon_close_process(TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
    } else {
        ferr = msg_mon_process_close();
        TEST_CHK_FEOK(ferr);
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
