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

enum  { MAX_DBUF = 1024 * 1024 };        // 1 MB

void *buf_alloc(size_t len) {
    return malloc(len);
}

void  buf_free(void *buf) {
    free(buf);
}

char recv_buffer[MAX_DBUF];
char send_buffer[MAX_DBUF];


int main(int argc, char *argv[]) {
    bool            bidir = false;
    bool            bm = false;
    void           *buf;
    double          busy;
    _xcc_status     cc;
    bool            client = false;
    int             count_read;
    int             count_written;
    int             count_xferred;
    int             dsize = 1024;
    int             ferr;
    short           filenum;
    int             inx;
    int             loop = 10;
    bool            nocopy = false;
    bool            nowaitc = false;
    bool            nowaits = false;
    struct rusage   r_start;
    struct rusage   r_stop;
    char           *recv_buffer_ptr;
    struct timeval  t_elapsed_data;
    struct timeval  t_elapsed_total;
    struct timeval  t_start_data;
    struct timeval  t_start_total;
    struct timeval  t_stop;
    SB_Tag_Type     tag;
    short           tfilenum;
    int             timeout = -1;
    bool            verbose = false;
    TAD             zargs[] = {
      { "-bidir",     TA_Bool, TA_NOMAX,    &bidir     },
      { "-bm",        TA_Bool, TA_NOMAX,    &bm        },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-dsize",     TA_Int,  MAX_DBUF,    &dsize     },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-nocopy",    TA_Bool, TA_NOMAX,    &nocopy    },
      { "-nowaitc",   TA_Bool, TA_NOMAX,    &nowaitc   },
      { "-nowaits",   TA_Bool, TA_NOMAX,    &nowaits   },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    if (nocopy) {
        ferr = file_buf_register(buf_alloc, buf_free);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    util_test_start(client);
    ferr = file_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    util_time_timer_start(&t_start_total);

    if (client) {
        printf("dsize=%d, loop=%d\n", dsize, loop);
        ferr = BFILE_OPEN_((char *) "$srv", 4, &filenum,
                           0, 0, nowaitc ? (short) 1 : (short) 0,
                           0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        util_time_timer_start(&t_start_data);
        util_cpu_timer_start(&r_start);
        for (inx = 0; inx < loop; inx++) {
            if (verbose)
                printf("count=%d\n", inx);
            cc = BWRITEREADX(filenum,
                             send_buffer,
                             (int) dsize, // cast
                             bidir ? dsize : 0,
                             &count_read,
                             0);
            if (nowaitc) {
                TEST_CHK_CCEQ(cc);
                tfilenum = filenum;
                cc = BAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag,
                               timeout,
                               NULL);
                TEST_CHK_CCEQ(cc);
            }
        }
    } else {
        ferr = BFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                           0, 0, nowaits ? (short) 1 : (short) 0, // nowait
                           1, 1, // no sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        util_time_timer_start(&t_start_data);
        util_cpu_timer_start(&r_start);
        for (inx = 0; inx < loop; inx++) {
            if (nocopy) {
                cc = file_buf_readupdatex(filenum,
                                          &recv_buffer_ptr,
                                          &count_read,
                                          0);
                buf_free(recv_buffer_ptr);
            } else
                cc = BREADUPDATEX(filenum,
                                  recv_buffer,
                                  (int) dsize, // cast
                                  &count_read,
                                  0);
            TEST_CHK_CCEQ(cc);
            if (nowaits) {
                tfilenum = -1;
                cc = BAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag,
                               timeout,
                               NULL);
            }
            TEST_CHK_CCEQ(cc);
            cc = BREPLYX(recv_buffer,
                         bidir ? dsize : 0,
                         &count_written,
                         0,
                         XZFIL_ERR_OK);
            TEST_CHK_CCEQ(cc);
        }
    }

    util_cpu_timer_stop(&r_stop);
    util_time_timer_stop(&t_stop);
    util_time_elapsed(&t_start_total, &t_stop, &t_elapsed_total);
    util_time_elapsed(&t_start_data, &t_stop, &t_elapsed_data);
    util_cpu_timer_busy(&r_start, &r_stop, &t_elapsed_data, &busy);

    if (client) {
        if (!bm) {
            print_elapsed("", &t_elapsed_total);
            print_elapsed(" (data)", &t_elapsed_data);
        }
        print_rate(bm, "", bidir ? loop * 2 : loop, dsize, &t_elapsed_data, busy);
    } else
        print_server_busy(bm, "", busy);

    if (client) {
        ferr = BFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    } else {
        ferr = BFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    }

    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
