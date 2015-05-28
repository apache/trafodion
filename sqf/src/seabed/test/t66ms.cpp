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


//#define MEM_LEAK

bool verbose = false;
char who[BUFSIZ];

void *myalloc(size_t len) {
    void *p = malloc(len + 30); // make room for reply
    if (verbose)
        printf("%s: myalloc(" PFSZ ")=%p\n", who, len, p);
    return p;
}

void myfree(void *buf) {
    if (verbose)
        printf("%s: myfree(%p)\n", who, buf);
    free(buf);
}

void myfreeapp(void *buf) {
    bool save = verbose;
    verbose = false;
    myfree(buf);
    verbose = save;
}

char  my_name[BUFSIZ];
char  recv_buffer[BUFSIZ];
short recv_buffer2[BUFSIZ];
short recv_buffer3[BUFSIZ];
char  send_buffer[BUFSIZ];
short send_buffer2[BUFSIZ];

int main(int argc, char *argv[]) {
    bool       chook = false;
    bool       client = false;
    bool       copy = false;
    int        countc;
    int        countd;
    int        countx;
    int        county;
    int        ferr;
    bool       factory = false;
    int        inx;
#ifdef MEM_LEAK
    int        leak_loop;
#endif
    int        len;
    int        lerr;
    int        loop = 10;
    int        msgid;
    int        oid;
    TPT_DECL  (phandle);
    short     *recv_bufferc;
    char      *recv_bufferd;
    short     *recv_bufferx;
    char      *recv_buffery;
    RT         results;
    int        send_len;
    MS_SRE     sre;
    TAD        zargs[] = {
      { "-chook",     TA_Bool, TA_NOMAX,    &chook     },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-copy",      TA_Bool, TA_NOMAX,    &copy      },
      { "-factory",   TA_Bool, TA_NOMAX,    &factory   },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    if (client && chook)
        test_debug_hook("c", "c");

    // do this before init!
    if (copy) {
        ferr = msg_buf_options(MS_BUF_OPTION_COPY);
        TEST_CHK_FEOK(ferr);
    }
    if (factory) {
        ferr = msg_buf_register(myalloc, myfree);
        TEST_CHK_FEOK(ferr);
    }

    msfs_util_init(&argc, &argv, msg_debug_hook);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    if (client) {
        strcpy(who, "cli");
        ferr = msg_mon_open_process((char *) "$srv",      // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
    } else
        strcpy(who, "srv");

    util_gethostname(my_name, sizeof(my_name));

#ifdef MEM_LEAK
    for (leak_loop = 0; ; leak_loop++) {
        if (client && ((leak_loop % 100) == 0))
            printf("leak_loop=%d\n", leak_loop);
#endif
    for (inx = 0; inx < loop; inx++) {
        if (client) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            send_len = (int) strlen(send_buffer) + 1;
            if (verbose)
                printf("%s: do link\n", who);
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
            if (verbose)
                printf("%s: link finished - do break\n", who);
            ferr = XMSG_BREAK_(msgid, results.u.s, TPT_REF(phandle));
            util_check("XMSG_BREAK_", ferr);
            if (verbose)
                printf("%s: break finished\n", who);
            assert(results.u.t.ctrl_size == (uint) (inx & 1));
            assert(results.u.t.data_size > (strlen(send_buffer) + 14));
            assert(results.u.t.errm == RT_DATA_RCVD); // data
#ifndef MEM_LEAK
            printf("%s\n", recv_buffer);
#endif
        } else {
            if (verbose)
                printf("%s: do listen\n", who);
            do {
                lerr = XWAIT(LREQ, -1);
                TEST_CHK_WAITIGNORE(lerr);
                lerr = XMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
            } while (lerr == XSRETYPE_NOWORK);
            if (verbose)
                printf("%s: listen finished - read ctrl/data\n", who);
            if (copy) {
                ferr = XMSG_READCTRL_(sre.sre_msgId,  // msgid
                                      recv_buffer2,   // reqctrl
                                      1);             // bytecount
                util_check("XMSG_READCTRL_", ferr);
                ferr = XMSG_READDATA_(sre.sre_msgId,  // msgid
                                      recv_buffer,    // reqdata
                                      BUFSIZ);        // bytecount
                util_check("XMSG_READDATA_", ferr);
                recv_bufferc = recv_buffer2;
                recv_bufferd = recv_buffer;
            } else if (factory) {
                // we supplied buffer, let reply delete it
                ferr = msg_buf_read_ctrl(sre.sre_msgId,  // msgid
                                         &recv_bufferc,  // reqctrl
                                         &countc,        // bytecount
                                         false);         // clear
                util_check("msg_buf_read_ctrl", ferr);
                ferr = msg_buf_read_data(sre.sre_msgId,  // msgid
                                         &recv_bufferd,  // reqdata
                                         &countd,        // bytecount
                                         false);         // clear
                util_check("msg_buf_read_data", ferr);
                // need to copy it, because it's going to be expanded
                memcpy(recv_buffer, recv_bufferd, countd);
            } else {
                // ms already has data, just ask for it
                ferr = msg_buf_read_ctrl(sre.sre_msgId,  // msgid
                                         &recv_bufferc,  // reqctrl
                                         &countc,        // bytecount
                                         true);          // clear
                util_check("msg_buf_read_ctrl", ferr);
                // second call should come up empty
                ferr = msg_buf_read_ctrl(sre.sre_msgId,  // msgid
                                         &recv_bufferx,  // reqctrl
                                         &countx,        // bytecount
                                         true);          // clear
                util_check("msg_buf_read_ctrl", ferr);
                assert(recv_bufferx == NULL);
                assert(countx == 0);
                // ms already has data, just ask for it
                ferr = msg_buf_read_data(sre.sre_msgId,  // msgid
                                         &recv_bufferd,  // reqdata
                                         &countd,        // bytecount
                                         true);          // clear
                util_check("msg_buf_read_data", ferr);
                // second call should come up empty
                ferr = msg_buf_read_data(sre.sre_msgId,  // msgid
                                         &recv_buffery,  // reqdata
                                         &county,        // bytecount
                                         true);          // clear
                util_check("msg_buf_read_data", ferr);
                assert(recv_buffery == NULL);
                assert(county == 0);
                // we own buffer, copy and delete it
                // need to copy it, because it's going to be expanded
                memcpy(recv_buffer, recv_bufferd, countd);
                myfreeapp(recv_bufferd);
                recv_bufferd = recv_buffer;
            }
            strcat(recv_bufferd, "- reply from ");
            strcat(recv_bufferd, my_name);
            len = (int) strlen(recv_bufferd) + 1;
            if (verbose)
                printf("%s: do reply\n", who);
            XMSG_REPLY_(sre.sre_msgId,       // msgid
                        recv_bufferc,        // replyctrl
                        sre.sre_reqCtrlSize, // replyctrlsize
                        recv_bufferd,        // replydata
                        (ushort) len,        // replydatasize
                        0,                   // errorclass
                        NULL);               // newphandle
            if (verbose)
                printf("%s: reply finished\n", who);
            if (!copy && !factory) {
                // we own buffer, delete it
                myfreeapp(recv_bufferc);
            }
        }
    }

#ifdef MEM_LEAK
    }
#endif
    if (client) {
        ferr = msg_mon_close_process(TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
