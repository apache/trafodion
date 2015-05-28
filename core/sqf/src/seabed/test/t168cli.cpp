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

#include "seabed/fs.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

typedef struct ctx {
    short     filenum;
    char      name[20];
    int       nid;
    bool      out;
    TPT_DECL (phandle);
    int       pid;
    char      send_buffer[BUFSIZ];
} Ctx_Type;

enum { MAX_SPS = 1000 };

Ctx_Type ctx[MAX_SPS];


int main(int argc, char *argv[]) {
    void         *buf;
    _bcc_status   bcc;
    int           count_read;
    int           count_xferred;
    short         event;
    int           ferr;
    bool          first;
    int           linx;
    int           loop = 10;
    int           maxsp = 1;
    bool          mq = false;
    char          my_name[BUFSIZ];
    int           out;
    char         *p;
    char          prog[MS_MON_MAX_PROCESS_PATH];
    int           sinx;
    char          srv[20];
    SB_Tag_Type   tag;
    short         tfilenum;
    int           timeout;
    TAD           zargs[] = {
      { "-client",    TA_Ign,  TA_NOMAX,    NULL          },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop         },
      { "-maxsp",     TA_Int,  MAX_SPS,     &maxsp        },
      { "-mq",        TA_Bool, TA_NOMAX,    &mq           },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL          },
      { "",           TA_End,  TA_NOMAX,    NULL          }
    };

    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(true);
    ferr = file_mon_process_startup(false); // system messages
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(my_name, BUFSIZ);
    util_check("msg_mon_get_my_process_name", ferr);

    strcpy(srv, argv[0]);
    p = strstr(srv, "cli");
    strcpy(p, "srv");
    sprintf(prog, "%s/%s", getenv("PWD"), srv);
    for (sinx = 0; sinx < maxsp; sinx++) {
        sprintf(srv, "$srv%d", sinx);
        strcpy(ctx[sinx].name, srv);
        ferr = msg_mon_start_process(prog,                   // prog
                                     srv,                    // name
                                     NULL,                   // ret name
                                     argc,
                                     argv,
                                     TPT_REF(ctx[sinx].phandle),
                                     0,                      // open
                                     NULL,                   // oid
                                     MS_ProcessType_Generic, // type
                                     0,                      // priority
                                     0,                      // debug
                                     0,                      // backup
                                     &ctx[sinx].nid,         // nid
                                     &ctx[sinx].pid,         // pid
                                     NULL,                   // infile
                                     NULL);                  // outfile
        TEST_CHK_FEOK(ferr);
    }
    for (sinx = 0; sinx < maxsp; sinx++) {
        strcpy(srv, ctx[sinx].name);
        ferr = XFILE_OPEN_(srv, (short) strlen(srv), &ctx[sinx].filenum,
                           0, 0, 1,
                           0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
    }
    for (linx = 0; linx < loop; linx++) {
        for (sinx = 0; sinx < maxsp; sinx++) {
            sprintf(ctx[sinx].send_buffer,
                    "hello, greetings from %s, to %s, inx=%d",
                    my_name, ctx[sinx].name, linx);
            bcc = BWRITEREADX(ctx[sinx].filenum,
                              ctx[sinx].send_buffer,
                              (int) strlen(ctx[sinx].send_buffer) + 1,
                              BUFSIZ,
                              &count_read,
                              0);
            TEST_CHK_BCCEQ(bcc);
            ctx[sinx].out = true;
        }
        timeout = 0;
        out = maxsp;
        first = true;
        for (;;) {
            if (!first) {
                event = XWAIT(LDONE, 1000);
                assert(event);
            }
            for (sinx = 0; sinx < maxsp; sinx++) {
                if (ctx[sinx].out) {
                    tfilenum = ctx[sinx].filenum;
                    bcc = BAWAITIOX(&tfilenum,
                                    &buf,
                                    &count_xferred,
                                    &tag,
                                    timeout,
                                    NULL);
                    if (_bstatus_eq(bcc)) {
                        ctx[sinx].out = false;
                        out--;
                        if (out == 0)
                            break;
                    }
                }
            }
            first = false;
            if (out == 0)
                break;
        }
    }
    for (sinx = 0; sinx < maxsp; sinx++) {
        ferr = BFILE_CLOSE_(ctx[sinx].filenum, 0);
        TEST_CHK_FEOK(ferr);
    }
    printf("if there were no asserts, all is well\n");
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(true);
    return 0;
}
