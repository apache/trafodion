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
#include <unistd.h>

#include "seabed/fs.h"
#include "seabed/ms.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"
#include "ufsri.h"

enum  { MAX_DBUF = 1024 * 1024 };        // 1 MB


void *buf_alloc(size_t len) {
    return malloc(len);
}

void  buf_free(void *buf) {
    free(buf);
}

enum { MAX_CLIENTS = 100 };
enum { MAX_SERVERS = 100 };

int      account[MAX_CLIENTS];
bool     client = false;
int      maxcp = 1;
char     my_name[BUFSIZ];


void printaccount(int) {
    int inx;

    if (!client) {
        for (inx = 0; inx < maxcp; inx++)
            printf("%s: account[%d]=%d\n", my_name, inx, account[inx]);
    }
}

char recv_buffer[MAX_DBUF];
char send_buffer[MAX_DBUF];
char sender[BUFSIZ];
char serv[BUFSIZ];

int main(int argc, char *argv[]) {
    bool            bidir = false;
    bool            bm = false;
    void           *buf;
    double          busy;
    _xcc_status     cc;
    int             count_read;
    int             count_written;
    int             count_xferred;
    int             dsize = 1024;
    int             ferr;
    short           filenum[MAX_SERVERS];
    short           filenumr;
    int             inx;
    int             loop = 10;
    int             max;
    int             maxsp = 1;
    bool            mq = false;
    bool            nocopy = false;
    bool            nowaitc = false;
    bool            nowaits = false;
    int             pinx;
    struct rusage   r_start;
    struct rusage   r_stop;
    char           *recv_buffer_ptr;
    RI_Type         ri;
    short           sender_len;
    int             sys_msg;
    struct timeval  t_elapsed_data;
    struct timeval  t_elapsed_open;
    struct timeval  t_elapsed_total;
    struct timeval  t_start_data;
    struct timeval  t_start_total;
    struct timeval  t_stop;
    SB_Tag_Type     tag;
    short           tfilenum;
    int             timeout = -1;
    bool            verbose = false;
    TAD             zargs[] = {
      { "-bidir",     TA_Bool, TA_NOMAX,    &bidir        },
      { "-bm",        TA_Bool, TA_NOMAX,    &bm           },
      { "-client",    TA_Bool, TA_NOMAX,    &client       },
      { "-dsize",     TA_Int,  MAX_DBUF,    &dsize        },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop         },
      { "-maxcp",     TA_Int,  MAX_CLIENTS, &maxcp        },
      { "-maxsp",     TA_Int,  MAX_SERVERS, &maxsp        },
      { "-mq",        TA_Bool, TA_NOMAX,    &mq           },
      { "-nocopy",    TA_Bool, TA_NOMAX,    &nocopy       },
      { "-nowaitc",   TA_Bool, TA_NOMAX,    &nowaitc      },
      { "-nowaits",   TA_Bool, TA_NOMAX,    &nowaits      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL          },
      { "",           TA_End,  TA_NOMAX,    NULL          }
    };

    for (inx = 0; inx < MAX_CLIENTS; inx++)
        account[inx] = 0;
    signal(SIGUSR2, printaccount);
    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    if (maxcp < 0)
        maxcp = 1;
    if (maxsp < 0)
        maxsp = 1;
    util_test_start(client);
    ferr = file_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    if (nocopy) {
        ferr = file_buf_register(buf_alloc, buf_free);
        TEST_CHK_FEOK(ferr);
    }
    ferr = msg_mon_get_my_process_name(my_name, BUFSIZ);
    TEST_CHK_FEOK(ferr);

    // process-wait for clients/servers/shell
    ferr = msfs_util_wait_process_count(MS_ProcessType_Generic,
                                        maxcp + maxsp + 1,
                                        NULL,
                                        verbose);
    if (client)
        sleep(1);
    util_time_timer_start(&t_start_total);
    if (client) {
        inx = atoi(&my_name[4]);
        printf("dsize=%d, loop=%d\n", dsize, loop);
        for (pinx = 0; pinx < maxsp; pinx++) {
            sprintf(serv, "$srv%d", pinx);
            ferr = BFILE_OPEN_(serv, (short) strlen(serv), &filenum[pinx],
                               0, 0, nowaitc ? (short) 1 : (short) 0,
                               0, 0, 0, 0, NULL);
            TEST_CHK_FEOK(ferr);
        }
        util_time_timer_start(&t_start_data);
        util_cpu_timer_start(&r_start);
        util_time_elapsed(&t_start_total, &t_start_data, &t_elapsed_open);
        max = loop;
        for (inx = 0; inx < max; inx++) {
            for (pinx = 0; pinx < maxsp; pinx++) {
                if (pinx == 0) {
                    if (verbose)
                        printf("%s-count=%d\n", my_name, inx);
                    else if (mq && ((inx % 1000) == 0))
                        printf("%s-count=%d\n", my_name, inx);
                }
                cc = BWRITEREADX(filenum[pinx],
                                 send_buffer,
                                 (int) dsize, // cast
                                 bidir ? dsize : 0,
                                 &count_read,
                                 0);
            }
            for (pinx = 0; pinx < maxsp; pinx++) {
                if (nowaitc) {
                    TEST_CHK_CCEQ(cc);
                    tfilenum = filenum[pinx];
                    cc = BAWAITIOX(&tfilenum,
                                   &buf,
                                   &count_xferred,
                                   &tag,
                                   timeout,
                                   NULL);
                    TEST_CHK_CCEQ(cc);
                }
            }
        }
    } else {
        ferr = BFILE_OPEN_((char *) "$RECEIVE", 8, &filenumr,
                           0, 0, nowaits ? (short) 1 : (short) 0, // nowait
                           1, 0, // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        util_time_timer_start(&t_start_data);
        util_cpu_timer_start(&r_start);
        max = maxcp * loop;
        for (inx = 0; inx < max; inx++) {
            if (nocopy) {
                cc = file_buf_readupdatex(filenumr,
                                          &recv_buffer_ptr,
                                          &count_read,
                                          0);
                buf_free(recv_buffer_ptr);
            } else
                cc = BREADUPDATEX(filenumr,
                                  recv_buffer,
                                  (int) dsize, // cast
                                  &count_read,
                                  0);
            if (nowaits) {
                tfilenum = -1;
                cc = BAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag,
                               timeout,
                               NULL);
                // don't check cc - could be sys msg
                sys_msg = _xstatus_ne(cc);
            } else
                sys_msg = _xstatus_ne(cc);
            if (sys_msg)
                inx--;
            if (!sys_msg) {
                getri(&ri);
                ferr = XPROCESSHANDLE_DECOMPOSE_(TPT_REF(ri.sender),
                                                 NULL, // cpu
                                                 NULL, // pin
                                                 NULL, // nodenumber
                                                 NULL, // nodename
                                                 0,    // nodename
                                                 NULL, // nodename_length
                                                 sender,
                                                 sizeof(sender),
                                                 &sender_len,
                                                 NULL); // sequence_number
                TEST_CHK_FEOK(ferr);
                sender[sender_len] = 0;
                if (verbose)
                    printf("sender=%s\n", sender);
                char *p = &sender[4]; // past $cli
                int sender_inx = atoi(p);
                account[sender_inx]++;
            }
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
            print_elapsed(" (open)", &t_elapsed_open);
        }
        print_rate(bm, "", bidir ? 2 * loop : loop, dsize, &t_elapsed_data, busy);
    } else
        print_server_busy(bm, "", busy);

    if (client) {
        for (pinx = 0; pinx < maxsp; pinx++) {
            ferr = BFILE_CLOSE_(filenum[pinx], 0);
            TEST_CHK_FEOK(ferr);
        }
    } else {
        ferr = BFILE_CLOSE_(filenumr, 0);
        TEST_CHK_FEOK(ferr);
        ferr = file_mon_process_close();
        TEST_CHK_FEOK(ferr);
    }

    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    printaccount(0);
    return 0;
}
