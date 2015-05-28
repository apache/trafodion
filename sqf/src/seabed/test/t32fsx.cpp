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

#include "tchkfe.h"
#include "tutil.h"
#include "tutilp.h"


int main(int argc, char *argv[]) {
    int             arg;
    _xcc_status     cc;
    bool            check = false;
    bool            client = false;
    unsigned short  count_read;
    short           done;
    int             ferr;
    short           filenum;
    int             inx;
    short           lasterr;
    int             loop = 10;
    char            my_name[BUFSIZ];
    char            recv_buffer[BUFSIZ];
    char            send_buffer[BUFSIZ];
    short           tfilenum;
    int             timeout;
    TAD             zargs[] = {
      { "-check",     TA_Bool, TA_NOMAX,    &check     },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    arg_proc_args(zargs, false, argc, argv);
    for (arg = 1; arg < argc; arg++) {
        if (strcmp(argv[arg], "-client") == 0) // start_process
            argv[arg] = (char *) "-server";
    }
    util_test_start(client);
    ferr = file_mon_process_startup(!client); // system messages
    TEST_CHK_FEOK(ferr);
    util_gethostname(my_name, sizeof(my_name));

    if (client) {
        file_debug_hook("c", "c");
        printf("loop=%d\n", loop);

        //
        // the connect fails if the first one is used
        //
#if 1
        ferr = XFILE_OPEN_((char *) "$srv", 4, &filenum, XOMITSHORT, XOMITSHORT, 1, 0);
#else
        ferr = XFILE_OPEN_((char *) "$srv", 4, &filenum, XOMITSHORT, XOMITSHORT, 1);
#endif
        TEST_CHK_FEOK(ferr);
        cc = XSETMODE(filenum, 1);
        TEST_CHK_CCEQ(cc);
        for (inx = 0; inx < loop; inx++) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            cc = XWRITEREADX(filenum,
                             send_buffer,
                             (short) (strlen(send_buffer) + 1),
                             BUFSIZ,
                             NULL,
                             1);
            TEST_CHK_CCEQ(cc);
            tfilenum = filenum;
            timeout = -1;
            done = true;
            if (check) {
                done = false;
                timeout = 0;
            }
            do {
                cc = XAWAITIOX(&tfilenum);
                if (check) {
                    if (_xstatus_eq(cc))
                        done = true;
                    else
                        util_time_sleep_ms(1);
                } else {
                    TEST_CHK_CCEQ(cc);
                }
            } while (!done);
            ferr = XFILE_GETINFO_(-1, &lasterr);
            TEST_CHK_FEOK(ferr);
            if (lasterr != XZFIL_ERR_OK)
                printf("lasterr=%d\n", lasterr);
            assert(lasterr == XZFIL_ERR_OK);
            printf("%s\n", send_buffer);
        }
        ferr = XFILE_CLOSE_(filenum);
        TEST_CHK_FEOK(ferr);
    } else {
        file_debug_hook("s", "s");
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                           XOMITSHORT, XOMITSHORT, 1,
                           1, 1);  // no sys msg
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            cc = XREADUPDATEX(filenum,
                              recv_buffer,
                              BUFSIZ,
                              &count_read);
            TEST_CHK_CCEQ(cc);
            tfilenum = filenum;
            timeout = -1;
            done = true;
            if (check) {
                done = false;
                timeout = 0;
            }
            timeout = timeout; // touch
            do {
                cc = XAWAITIOX(&tfilenum);
                if (check) {
                    if (_xstatus_eq(cc))
                        done = true;
                    else
                        util_time_sleep_ms(1);
                } else {
                    TEST_CHK_CCEQ(cc);
                }
            } while (!done);
            strcat(recv_buffer, "- reply from ");
            strcat(recv_buffer, my_name);
            count_read = (short) (strlen(recv_buffer) + 1);
            cc = XREPLYX(recv_buffer, count_read);
            TEST_CHK_CCEQ(cc);
        }
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
