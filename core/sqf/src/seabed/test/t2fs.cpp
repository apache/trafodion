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

char my_name[BUFSIZ];
char recv_buffer[BUFSIZ];
char send_buffer[BUFSIZ];


int main(int argc, char *argv[]) {
    _xcc_status     cc;
    bool            chook = false;
    bool            client = false;
    unsigned short  count_read;
    unsigned short  count_written;
    TCPU_DECL      (cpu);
    int             ferr;
    short           filenum;
    int             inx;
    int             loop = 10;
    TPT_DECL       (mphandle);
    char           *mphandlec = (char *) TPT_REF(mphandle);
    TPT_DECL_INT   (mphandlei);
    int             nid;
    char            nodename[20];
    short           nodename_length;
    int             nodenumber;
    int             pid;
    TPIN_DECL      (pin);
    char            procname[40];
    short           procname_length;
    SB_Int64_Type   sequence_number;
    bool            shook = false;
    const char     *sname = "$srv";
    TAD             zargs[] = {
      { "-chook",     TA_Bool, TA_NOMAX,    &chook     },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-shook",     TA_Bool, TA_NOMAX,    &shook     },
      { "-sname",     TA_Str,  TA_NOMAX,    &sname     },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    if (client && chook)
        test_debug_hook("c", "c");
    if (!client && shook)
        test_debug_hook("s", "s");
    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    util_test_start(client);
    ferr = file_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(my_name, BUFSIZ);
    util_check("msg_mon_get_my_process_name", ferr);
    ferr = msg_mon_get_process_info(my_name, &nid, &pid);
    TEST_CHK_FEOK(ferr);

    ferr = XPROCESSHANDLE_GETMINE_(TPT_REF(mphandle));
    util_check("XPROCESSHANDLE_GETMINE_", ferr);
    TPT_COPY_INT(mphandlei, mphandle);
    assert((mphandlei[0] & 0xf0) == 0x20); // named
    assert(strncmp(my_name, &mphandlec[4], 32) == 0);
    printf("phandle=%x.%x.%x.%x.%x, pname=%s\n",
           mphandlei[0], mphandlei[1], mphandlei[2], mphandlei[3], mphandlei[4],
           my_name);
    ferr = XPROCESSHANDLE_DECOMPOSE_(TPT_REF(mphandle),
                                     &cpu,
                                     &pin,
                                     &nodenumber,
                                     nodename,
                                     sizeof(nodename),
                                     &nodename_length,
                                     procname,
                                     sizeof(procname),
                                     &procname_length,
                                     &sequence_number);
    assert(cpu == nid);
    assert(pin == pid);
    assert(nodenumber == 0);
    assert(nodename_length == 4);
    nodename[nodename_length] = 0;
    assert(strcmp(nodename, "\\NSK") == 0);
    procname[procname_length] = 0;
    assert(strcmp(procname, my_name) == 0);
#ifdef SQ_PHANDLE_VERIFIER
    assert(sequence_number != 0);
#else
    assert(sequence_number == 0);
#endif

    if (client) {
        ferr = XFILE_OPEN_((char *) sname, 4, &filenum,
                           0, 0, 0, 0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            cc = XWRITEREADX(filenum,
                             send_buffer,
                             (unsigned short) (strlen(send_buffer) + 1), // cast
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
            count_read = (unsigned short) (strlen(recv_buffer) + 1); // cast
            cc = XREPLYX(recv_buffer,
                         count_read,
                         &count_written,
                         0,
                         XZFIL_ERR_OK);
            TEST_CHK_CCEQ(cc);
        }
        ferr = XFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
