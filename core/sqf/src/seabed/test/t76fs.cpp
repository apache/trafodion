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
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"
#include "ufsri.h"


int main(int argc, char *argv[]) {
    void           *buf;
    _xcc_status     cc;
    bool            client = false;
    unsigned short  count_read;
    unsigned short  count_written;
    unsigned short  count_xferred;
    int             ferr;
    short           filenum;
    short           filenumc;
    short           filenumd;
    short           filenumdd;
    int             inx = 0;
    short           lasterr;
    int             loop = 10;
    char            my_name[BUFSIZ];
    char            recv_buffer[BUFSIZ];
    RI_Type         ri;
    char            send_buffer[BUFSIZ];
    SB_Tag_Type     tag;
    short           tfilenum;
    int             timeout = 1;
    bool            verbose = false;
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
    ferr = file_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    util_gethostname(my_name, (int) sizeof(my_name));

    if (client) {
        if (verbose)
            printf("client open C1\n");
        ferr = XFILE_OPEN_((char *) "$Z000009", 8, &filenumc,
                           0, 0, 1, // nowait
                           0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        if (verbose)
            printf("client open C1, fnum=%d\n", filenumc);
        strcpy(send_buffer, "control\n");
        if (verbose)
            printf("client WR C1\n");
        cc = XWRITEREADX(filenumc,
                         send_buffer,
                         (short) (strlen(send_buffer) + 1),
                         BUFSIZ,
                         &count_read,
                         0);
        TEST_CHK_CCEQ(cc);

        if (verbose)
            printf("client open D1\n");
        ferr = XFILE_OPEN_((char *) "$Z000009", 8, &filenumd,
                           0, 0, 0, // wait
                           0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        if (verbose)
            printf("client open D1, fnum=%d\n", filenumd);
        sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                my_name, inx);
        if (verbose)
            printf("client WR D1\n");
        cc = XWRITEREADX(filenumd,
                         send_buffer,
                         (short) (strlen(send_buffer) + 1),
                         BUFSIZ,
                         &count_read,
                         0);
        TEST_CHK_CCEQ(cc);

        if (verbose)
            printf("client open DD1\n");
        ferr = XFILE_OPEN_((char *) "$Z000009", 8, &filenumdd,
                           0, 0, 0, // wait
                           0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        if (verbose)
            printf("client open DD1, fnum=%d\n", filenumdd);
        sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                my_name, inx);
        if (verbose)
            printf("client WR DD1\n");
        cc = XWRITEREADX(filenumdd,
                         send_buffer,
                         (short) (strlen(send_buffer) + 1),
                         BUFSIZ,
                         &count_read,
                         0);
        TEST_CHK_CCEQ(cc);
        if (verbose)
            printf("client close DD1\n");
        ferr = XFILE_CLOSE_(filenumdd, 0);
        TEST_CHK_FEOK(ferr);

        sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                my_name, inx);
        if (verbose)
            printf("client WR D1\n");
        cc = XWRITEREADX(filenumd,
                         send_buffer,
                         (short) (strlen(send_buffer) + 1),
                         BUFSIZ,
                         &count_read,
                         0);
        TEST_CHK_CCEQ(cc);
        if (verbose)
            printf("client close D1\n");
        ferr = XFILE_CLOSE_(filenumd, 0);
        TEST_CHK_FEOK(ferr);

        if (verbose)
            printf("client open D2\n");
        ferr = XFILE_OPEN_((char *) "$Z000009", 8, &filenumd,
                           0, 0, 1, // nowait
                           0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        if (verbose)
            printf("client open D2, fnum=%d\n", filenumd);
        tfilenum = filenumc;
        if (verbose)
            printf("client open-wait D2\n");
        cc = XAWAITIOX(&tfilenum,
                       &buf,
                       &count_xferred,
                       &tag,
                       timeout,
                       NULL);
        TEST_CHK_CCNE(cc);
        ferr = XFILE_GETINFO_(filenumc,
                              &lasterr,
                              NULL,
                              0,
                              NULL,
                              NULL,
                              NULL);
        assert(lasterr == XZFIL_ERR_TIMEDOUT);
        if (verbose)
            printf("client close D2\n");
        ferr = XFILE_CLOSE_(filenumd, 0);
        TEST_CHK_FEOK(ferr);
        if (verbose)
            printf("client close C1\n");
        ferr = XFILE_CLOSE_(filenumc, 0);
        TEST_CHK_FEOK(ferr);
    } else {
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                           0, 0, 0,
                           2, 0, // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        if (verbose)
            printf("server read C1-open\n");
        cc = XREADUPDATEX(filenum, // control-open
                          recv_buffer,
                          BUFSIZ,
                          &count_read,
                          1);
        TEST_CHK_CCNE(cc);
        getri(&ri);
        assert(recv_buffer[0] == XZSYS_VAL_SMSG_OPEN);
        assert(ri.io_type == 0);
        filenumc = ri.file_number;
        cc = XREPLYX(recv_buffer,
                     0,
                     &count_written,
                     ri.message_tag,
                     XZFIL_ERR_OK);
        TEST_CHK_CCEQ(cc);
        if (verbose)
            printf("server read C1-WR\n");
        cc = XREADUPDATEX(filenum, // control-msg
                          recv_buffer,
                          BUFSIZ,
                          &count_read,
                          2);
        TEST_CHK_CCEQ(cc);
        getri(&ri);
        // do not reply to CONTROL!

        if (verbose)
            printf("server read D1-open\n");
        cc = XREADUPDATEX(filenum, // data-open
                          recv_buffer,
                          BUFSIZ,
                          &count_read,
                          3);
        TEST_CHK_CCNE(cc);
        getri(&ri);
        assert(recv_buffer[0] == XZSYS_VAL_SMSG_OPEN);
        assert(ri.io_type == 0);
        filenumd = ri.file_number;
        cc = XREPLYX(recv_buffer,
                     0,
                     &count_written,
                     ri.message_tag,
                     XZFIL_ERR_OK);
        TEST_CHK_CCEQ(cc);
        if (verbose)
            printf("server read D1-WR\n");
        cc = XREADUPDATEX(filenum, // data
                          recv_buffer,
                          BUFSIZ,
                          &count_read,
                          4);
        TEST_CHK_CCEQ(cc);
        getri(&ri);
        assert(ri.io_type == 3); // WR
        assert(ri.file_number == filenumd);
        strcat(recv_buffer, "- reply from ");
        strcat(recv_buffer, my_name);
        count_read = (short) (strlen(recv_buffer) + 1);
        cc = XREPLYX(recv_buffer,
                     count_read,
                     &count_written,
                     ri.message_tag,
                     XZFIL_ERR_OK);
        TEST_CHK_CCEQ(cc);

        if (verbose)
            printf("server read DD1-open\n");
        cc = XREADUPDATEX(filenum, // data-open
                          recv_buffer,
                          BUFSIZ,
                          &count_read,
                          5);
        TEST_CHK_CCNE(cc);
        getri(&ri);
        assert(recv_buffer[0] == XZSYS_VAL_SMSG_OPEN);
        assert(ri.io_type == 0);
        filenumdd = ri.file_number;
        cc = XREPLYX(recv_buffer,
                     0,
                     &count_written,
                     ri.message_tag,
                     XZFIL_ERR_OK);
        TEST_CHK_CCEQ(cc);
        if (verbose)
            printf("server read DD1-WR\n");
        cc = XREADUPDATEX(filenum, // data
                          recv_buffer,
                          BUFSIZ,
                          &count_read,
                          6);
        TEST_CHK_CCEQ(cc);
        getri(&ri);
        assert(ri.io_type == 3); // WR
        assert(ri.file_number == filenumdd);
        strcat(recv_buffer, "- reply from ");
        strcat(recv_buffer, my_name);
        count_read = (short) (strlen(recv_buffer) + 1);
        cc = XREPLYX(recv_buffer,
                     count_read,
                     &count_written,
                     ri.message_tag,
                     XZFIL_ERR_OK);
        TEST_CHK_CCEQ(cc);

        if (verbose)
            printf("server read DD1-close\n");
        cc = XREADUPDATEX(filenum, // data-close
                          recv_buffer,
                          BUFSIZ,
                          &count_read,
                          7);
        TEST_CHK_CCNE(cc);
        getri(&ri);
        assert(recv_buffer[0] == XZSYS_VAL_SMSG_CLOSE);
        assert(ri.io_type == 0);
        assert(ri.file_number == filenumdd);
        cc = XREPLYX(recv_buffer,
                     0,
                     &count_written,
                     ri.message_tag,
                     XZFIL_ERR_OK);
        TEST_CHK_CCEQ(cc);

        if (verbose)
            printf("server read D1-WR\n");
        cc = XREADUPDATEX(filenum, // data
                          recv_buffer,
                          BUFSIZ,
                          &count_read,
                          8);
        TEST_CHK_CCEQ(cc);
        getri(&ri);
        assert(ri.io_type == 3); // WR
        assert(ri.file_number == filenumd);
        strcat(recv_buffer, "- reply from ");
        strcat(recv_buffer, my_name);
        count_read = (short) (strlen(recv_buffer) + 1);
        cc = XREPLYX(recv_buffer,
                     count_read,
                     &count_written,
                     ri.message_tag,
                     XZFIL_ERR_OK);
        TEST_CHK_CCEQ(cc);

        if (verbose)
            printf("server read D1-close\n");
        cc = XREADUPDATEX(filenum, // data-close
                          recv_buffer,
                          BUFSIZ,
                          &count_read,
                          9);
        TEST_CHK_CCNE(cc);
        getri(&ri);
        assert(recv_buffer[0] == XZSYS_VAL_SMSG_CLOSE);
        assert(ri.io_type == 0);
        assert(ri.file_number == filenumd);
        cc = XREPLYX(recv_buffer,
                     0,
                     &count_written,
                     ri.message_tag,
                     XZFIL_ERR_OK);
        TEST_CHK_CCEQ(cc);

        if (verbose)
            printf("server read D2-open\n");
        cc = XREADUPDATEX(filenum, // data-open
                          recv_buffer,
                          BUFSIZ,
                          &count_read,
                          10);
        TEST_CHK_CCNE(cc);
        getri(&ri);
        assert(recv_buffer[0] == XZSYS_VAL_SMSG_OPEN);
        assert(ri.io_type == 0);
        filenumd = ri.file_number;
        cc = XREPLYX(recv_buffer,
                     0,
                     &count_written,
                     ri.message_tag,
                     XZFIL_ERR_OK);
        TEST_CHK_CCEQ(cc);
        if (verbose)
            printf("server read D2-close\n");
        cc = XREADUPDATEX(filenum, // data-close
                          recv_buffer,
                          BUFSIZ,
                          &count_read,
                          12);
        TEST_CHK_CCNE(cc);
        getri(&ri);
        assert(recv_buffer[0] == XZSYS_VAL_SMSG_CLOSE);
        assert(ri.io_type == 0);
        assert((ri.file_number == filenumc) || (ri.file_number == filenumd));
        cc = XREPLYX(recv_buffer,
                     0,
                     &count_written,
                     ri.message_tag,
                     XZFIL_ERR_OK);
        TEST_CHK_CCEQ(cc);

        if (verbose)
            printf("server read C1-close\n");
        cc = XREADUPDATEX(filenum, // control-close
                          recv_buffer,
                          BUFSIZ,
                          &count_read,
                          13);
        TEST_CHK_CCNE(cc);
        getri(&ri);
        assert(recv_buffer[0] == XZSYS_VAL_SMSG_CLOSE);
        assert(ri.io_type == 0);
        assert((ri.file_number == filenumc) || (ri.file_number == filenumd));
        cc = XREPLYX(recv_buffer,
                     0,
                     &count_written,
                     ri.message_tag,
                     XZFIL_ERR_OK);
        TEST_CHK_CCEQ(cc);
        ferr = XFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
