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

//#define MEM_LEAK

enum  { MAX_DBUF = 1024 * 1024 };        // 1 MB
enum  { CHK_CHAR = '4'         };


char recv_buffer[MAX_DBUF];
char send_buffer[MAX_DBUF];

int main(int argc, char *argv[]) {
    bool            bidir = false;
    char           *buf;
    _xcc_status     cc;
    bool            check = false;
    bool            client = false;
    bool            copy = false;
    int             count_read;
    int             count_written;
    int             count_xferred;
    int             dsize = 1024;
    int             ferr;
    short           filenum;
    int             inx;
    int             inx2;
#ifdef MEM_LEAK
    int             leak_loop;
#endif
    int             loop = 10;
    bool            nowait = false;
    long            t_elapsed;
    long            t_elapsed_sec;
    struct timeval  t_start;
    struct timeval  t_stop;
    SB_Tag_Type     tag;
    short           tfilenum;
    int             timeout = -1;
    int             rc;
    bool            reverse = false;
    bool            verbose = false;
    int             wc;
    TAD             zargs[] = {
      { "-bidir",     TA_Bool, TA_NOMAX,    &bidir     },
      { "-check",     TA_Bool, TA_NOMAX,    &check     },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-copy",      TA_Bool, TA_NOMAX,    &copy      },
      { "-dsize",     TA_Int,  MAX_DBUF,    &dsize     },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-nowait",    TA_Bool, TA_NOMAX,    &nowait    },
      { "-reverse",   TA_Bool, TA_NOMAX,    &reverse   },
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

    if (bidir)
        reverse = false;
    for (inx2 = 0; inx2 < dsize; inx2++)
        send_buffer[inx2] = CHK_CHAR;
    if (client) {
        printf("bidir=%d, check=%d, copy=%d, dsize=%d, loop=%d, nowait=%d, reverse=%d\n",
               bidir, check, copy, dsize, loop, nowait, reverse);
        ferr = BFILE_OPEN_((char *) "$srv", 4, &filenum,
                           0, 0, nowait ? (short) 1 : (short) 0,
                           0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        util_time_timer_start(&t_start);
#ifdef MEM_LEAK
        for (leak_loop = 0; ; leak_loop++) {
            if ((leak_loop % 100) == 0)
                printf("leak_loop=%d\n", leak_loop);
#endif
        for (inx = 0; inx < loop; inx++) {
            if (verbose)
                printf("count=%d\n", inx);
            wc = dsize;
            rc = dsize;
            if (reverse)
                wc = 0;
            else if (!bidir)
                rc = 0; // !bidir && !reverse
            cc = BWRITEREADX(filenum,
                             send_buffer,
                             wc,
                             rc,
                             &count_read,
                             0);
            TEST_CHK_CCEQ(cc);
            if (nowait) {
                tfilenum = filenum;
                cc = BAWAITIOX(&tfilenum,
                               (void **) &buf,
                               &count_xferred,
                               &tag,
                               timeout,
                               NULL);
                TEST_CHK_CCEQ(cc);
                if (check) {
                    assert(buf == send_buffer);
                    assert(count_xferred == 0);
                    if (rc) {
                        for (inx2 = 0; inx2 < dsize; inx2++) {
                            if (buf[inx2] != CHK_CHAR) {
                                printf("buf[%d]=%d\n", inx2, buf[inx2]);
                                assert(buf[inx2] == CHK_CHAR);
                            }
                        }
                    }
                }
            }
        }
#ifdef MEM_LEAK
        }
#endif
        util_time_timer_stop(&t_stop);
        t_elapsed = (t_stop.tv_sec * 1000000 + t_stop.tv_usec) -
                    (t_start.tv_sec * 1000000 + t_start.tv_usec);
        t_elapsed_sec = t_elapsed / 1000000;
        t_elapsed -= t_elapsed_sec * 1000000;
        printf("elapsed=%ld.%ld\n", t_elapsed_sec, t_elapsed);

        ferr = BFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    } else {
        ferr = BFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                           0, 0, nowait ? (short) 1 : (short) 0,
                           1, 1, // no sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
#ifdef MEM_LEAK
        for (leak_loop = 0; ; leak_loop++) {
#endif
        for (inx = 0; inx < loop; inx++) {
            if (copy)
                cc = BREADUPDATEX(filenum,
                                  recv_buffer,
                                  dsize,
                                  &count_read,
                                  inx);
            else
                cc = file_buf_readupdatex(filenum, &buf, &count_read, inx);
            TEST_CHK_CCEQ(cc);
            if (nowait) {
                tfilenum = filenum;
                cc = BAWAITIOX(&tfilenum,
                               (void **) &buf,
                               &count_xferred,
                               &tag,
                               timeout,
                               NULL);
                TEST_CHK_CCEQ(cc);
                if (check) {
                    if (reverse)
                        assert(count_xferred == 0);
                    else
                        assert(count_xferred == dsize);
                    assert(tag == inx);
                    if (copy)
                        assert(buf == recv_buffer);
                }
            } else {
                if (reverse)
                    assert(count_read == 0);
                else
                    assert(count_read == dsize);
                if (copy)
                    buf = recv_buffer;
                else if (reverse) {
                    buf = send_buffer;
                }
            }
            if (check && !reverse) {
                for (inx2 = 0; inx2 < dsize; inx2++) {
                    if (buf[inx2] != CHK_CHAR) {
                        printf("buf[%d]=%d\n", inx2, buf[inx2]);
                        assert(buf[inx2] == CHK_CHAR);
                    }
                }
            }
            if (bidir || reverse)
                cc = BREPLYX(buf,
                             dsize,  // wc
                             &count_written,
                             0,
                             XZFIL_ERR_OK);
            else
                cc = BREPLYX(buf,
                             0,      // wc
                             &count_written,
                             0,
                             XZFIL_ERR_OK);
            TEST_CHK_CCEQ(cc);
            if (!copy && !reverse)
                free(buf);
        }
#ifdef MEM_LEAK
        }
#endif
        ferr = BFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
