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

#include "seabed/fs.h"
#include "seabed/ms.h"

#include "tchkfe.h"
#include "tutil.h"
#include "tutilp.h"


//#define DEBUG_ENV
#ifdef DEBUG_ENV
char        **env;
#endif

int main(int argc, char *argv[]) {
    int              arg;
    bool             attach = false;
    _xcc_status      cc;
    bool             chook = false;
    bool             client = false;
    unsigned short   count_read;
    unsigned short   count_written;
    int              ferr;
    short            filenum;
    int              inx;
    int              loop = 10;
    char             my_name[BUFSIZ];
    char             prog[MS_MON_MAX_PROCESS_PATH];
    char             recv_buffer[BUFSIZ];
    char             ret_name[MS_MON_MAX_PROCESS_NAME];
    char             send_buffer[BUFSIZ];
    const char      *server_name = "$SRV";
    int              server_nid = -1;
    TPT_DECL        (server_phandle);
    int              server_pid;
    bool             shook = false;
    TAD              zargs[] = {
      { "-attach",    TA_Bool, TA_NOMAX,    &attach    },
      { "-chook",     TA_Bool, TA_NOMAX,    &chook     },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-shook",     TA_Bool, TA_NOMAX,    &shook     },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

#ifdef DEBUG_ENV
    env = environ;
    while (*env != NULL) {
        printf("env=%s\n", *env);
        env++;
    }
#endif

    arg_proc_args(zargs, false, argc, argv);
    if (chook && client)
        test_debug_hook("c", "c");
    if (shook && !client)
        test_debug_hook("s", "s");
    if (client && attach)
        ferr = file_init_attach(&argc, &argv, true, (char *) "$CLI");
    else
        ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    util_test_start(client);
    ferr = file_mon_process_startup(!client); // system messages
    TEST_CHK_FEOK(ferr);
    util_gethostname(my_name, sizeof(my_name));

    if (client) {
        sprintf(prog, "%s/%s", getenv("PWD"), argv[0]);
        for (arg = 0; arg < argc; arg++)
            if (strcmp(argv[arg], "-client") == 0) // start_process
                argv[arg] = (char *) "-server";
        ferr = msg_mon_start_process(prog,                   // prog
                                     (char *) server_name,   // name
                                     ret_name,               // ret name
                                     argc,
                                     argv,
                                     TPT_REF(server_phandle),
                                     0,                      // open
                                     NULL,                   // oid
                                     MS_ProcessType_Generic, // type
                                     0,                      // priority
                                     0,                      // debug
                                     0,                      // backup
                                     &server_nid,            // nid
                                     &server_pid,            // pid
                                     NULL,                   // infile
                                     NULL);                  // outfile
        TEST_CHK_FEOK(ferr);

        assert(strcmp(ret_name, server_name) == 0);

        ferr = XFILE_OPEN_((char *) server_name, (short) strlen(server_name), &filenum,
                           0, 0, 0, 0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            cc = XWRITEREADX(filenum,
                             send_buffer,
                             (short) (strlen(send_buffer) + 1),
                             BUFSIZ,
                             &count_read,
                             0);
            TEST_CHK_CCEQ(cc);
            printf("%s\n", send_buffer);
        }
        ferr = XFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    } else {
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                           0, 0, 0,
                           1, 1, // no sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            cc = XREADUPDATEX(filenum,
                              recv_buffer,
                              BUFSIZ,
                              &count_read,
                              0);
            TEST_CHK_CCEQ(cc);
            strcat(recv_buffer, "- reply from ");
            strcat(recv_buffer, my_name);
            count_read = (short) (strlen(recv_buffer) + 1);
            cc = XREPLYX(recv_buffer,
                         count_read,
                         &count_written,
                         0,
                         XZFIL_ERR_OK);
            TEST_CHK_CCEQ(cc);
        }
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
