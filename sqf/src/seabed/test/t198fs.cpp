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

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

short  filenumr;
char   my_name[BUFSIZ];
char   recv_buffer[BUFSIZ];
char   send_buffer[BUFSIZ];
bool   verbose = false;

void readsysmsg(const char *who, const char *which) {
    _xcc_status     cc;
    unsigned short  count_read;
    unsigned short  count_written;

    cc = XREADUPDATEX(filenumr,
                      recv_buffer,
                      BUFSIZ,
                      &count_read,
                      0);
    TEST_CHK_CCNE(cc);
    if (verbose)
        printf("%s received %s\n", who, which);
    count_read = 0;
    cc = XREPLYX(recv_buffer,
                 count_read,
                 &count_written,
                 0,
                 XZFIL_ERR_OK);
    TEST_CHK_CCEQ(cc);
    if (verbose)
        printf("%s sent %s reply\n", who, which);
}

int main(int argc, char *argv[]) {
    _xcc_status     cc;
    bool            client = false;
    const char     *cname = "$cli";
    unsigned short  count_read;
    unsigned short  count_written;
    int             disable;
    int             ferr;
    short           filenumc1;
    short           filenumc2;
    short           filenums1;
    short           filenums2;
    int             inx;
    int             loop = 10;
    const char     *sname = "$srv";
    TAD             zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = file_mon_process_startup(true);  // system messages?
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(my_name, BUFSIZ);
    util_check("msg_mon_get_my_process_name", ferr);

    ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenumr,
                       0, 0, 0,
                       1, 0, // sys msg
                       0, 0, NULL);
    TEST_CHK_FEOK(ferr);
    // process-wait for client/server/shell
    ferr = msfs_util_wait_process_count(MS_ProcessType_Generic, 3, NULL, verbose);
    TEST_CHK_FEOK(ferr);
    if (client) {
        if (verbose)
            printf("client opening server 1\n");
        ferr = XFILE_OPEN_((char *) sname, 4, &filenums1,
                           0, 0, 0, 0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        if (verbose)
            printf("client opening server 2\n");
        ferr = XFILE_OPEN_((char *) sname, 4, &filenums2,
                           0, 0, 0, 0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        readsysmsg("client", "open");
        readsysmsg("client", "open");
        for (inx = 0; inx < loop; inx++) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            if (verbose)
                printf("client wr server inx=%d\n", inx);
            cc = XWRITEREADX(filenums1,
                             send_buffer,
                             (unsigned short) (strlen(send_buffer) + 1), // cast
                             BUFSIZ,
                             &count_read,
                             0);
            TEST_CHK_CCEQ(cc);
            printf("%s\n", send_buffer);
        }
        ferr = XFILE_CLOSE_(filenums1, 0);
        TEST_CHK_FEOK(ferr);
        ferr = XFILE_CLOSE_(filenums2, 0);
        TEST_CHK_FEOK(ferr);
        readsysmsg("client", "close");
        readsysmsg("client", "close");
    } else {
        readsysmsg("server", "open");
        readsysmsg("server", "open");
        if (verbose)
            printf("server opening client\n");
        ferr = XFILE_OPEN_((char *) cname, 4, &filenumc1,
                           0, 0, 0, 0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        ferr = XFILE_OPEN_((char *) cname, 4, &filenumc2,
                           0, 0, 0, 0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            cc = XREADUPDATEX(filenumr,
                              recv_buffer,
                              BUFSIZ,
                              &count_read,
                              0);
            TEST_CHK_CCEQ(cc);
            strcat(recv_buffer, "- reply from ");
            strcat(recv_buffer, my_name);
            count_read = (unsigned short) (strlen(recv_buffer) + 1); // cast
            cc = XREPLYX(recv_buffer,
                         count_read,
                         &count_written,
                         0,
                         XZFIL_ERR_OK);
            TEST_CHK_CCEQ(cc);
            if (verbose)
                printf("server sent reply inx=%d\n", inx);
        }
        disable = msg_test_assert_disable();
        ferr = XFILE_CLOSE_(filenumc1, 0);
        TEST_CHK_FEOK(ferr);
        ferr = XFILE_CLOSE_(filenumc2, 0);
        TEST_CHK_FEOK(ferr);
        msg_test_assert_enable(disable);
        readsysmsg("server", "close");
        readsysmsg("server", "close");
    }
    ferr = XFILE_CLOSE_(filenumr, 0);
    TEST_CHK_FEOK(ferr);
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
