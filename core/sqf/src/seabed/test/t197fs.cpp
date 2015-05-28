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

enum { BUF_SIZE    = 100 };
enum { MAXNOWAIT   = 10 };   // nowait-depth
enum { MAXRCVDEPTH = 4096 };
enum { RDOVER      = 2 * MAXNOWAIT };   // over-write by this amount
enum { MAXFILES1   = MAXRCVDEPTH / MAXNOWAIT };
enum { MAXFILES2   = RDOVER / MAXNOWAIT };

short   filenumc1[MAXFILES1];
short   filenumc2[MAXFILES2];
bool    msg_ok[MAXFILES2];
char    recv_buffer[MAXRCVDEPTH][BUF_SIZE];
char    send_buffer1[MAXFILES1][MAXRCVDEPTH][BUF_SIZE];
char    send_buffer2[MAXFILES2][RDOVER][BUF_SIZE];


int main(int argc, char *argv[]) {
    void           *buf;
    _xcc_status     cc;
    bool            client = false;
    unsigned short  count_read;
    unsigned short  count_written;
    unsigned short  count_xferred;
    int             disable;
    int             ferr;
    short           filenum;
    int             linx;
    int             loop = 1;
    int             maxopen1;
    int             maxopen2;
    char            my_name[BUFSIZ];
    int             ninx;
    int             oinx;
    int             rd;
    int             recvdepth = 1;
    int             rinx;
    SB_Tag_Type     tag;
    short           tfilenum;
    int             timeout = -1;
    bool            verbose = false;
    TAD             zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-recvdepth", TA_Int,  MAXRCVDEPTH, &recvdepth },
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
        maxopen1 = (recvdepth + MAXNOWAIT - 1 ) / MAXNOWAIT;
        maxopen2 = RDOVER / MAXNOWAIT;
        if (verbose)
            printf("client maxopen=%d, recvdepth=%d\n", maxopen1, recvdepth);
        for (oinx = 0; oinx < maxopen1; oinx++) {
            ferr = XFILE_OPEN_((char *) "$SRV", 8, &filenumc1[oinx],
                               0, 0, MAXNOWAIT,  // nowait
                               0, 0, 0, 0, NULL);
            TEST_CHK_FEOK(ferr);
        }
        for (oinx = 0; oinx < maxopen2; oinx++) {
            ferr = XFILE_OPEN_((char *) "$SRV", 8, &filenumc2[oinx],
                               0, 0, MAXNOWAIT,  // nowait
                               0, 0, 0, 0, NULL);
            TEST_CHK_FEOK(ferr);
        }
        for (linx = 0; linx < loop; linx++) {
            rd = 0;
            for (oinx = 0; oinx < maxopen1; oinx++) {
                for (ninx = 0; ninx < MAXNOWAIT; ninx++) {
                    if (verbose)
                        printf("client wr1, fnum=%d, linx=%d, oinx=%d, ninx=%d, rd=%d\n",
                               filenumc1[oinx], linx, oinx, ninx, rd);
                    sprintf(send_buffer1[oinx][ninx], "hello1, greetings from %s, oinx=%d, ninx=%d, rd=%d",
                            my_name, oinx, ninx, rd);
                    cc = XWRITEREADX(filenumc1[oinx],
                                     send_buffer1[oinx][ninx],
                                     (short) (strlen(send_buffer1[oinx][ninx]) + 1),
                                     BUFSIZ,
                                     &count_read,
                                     rd);
                    TEST_CHK_CCEQ(cc);
                    rd++;
                    if (rd >= recvdepth)
                        break;
                }
            }
            for (oinx = 0; oinx < maxopen2; oinx++) {
                for (ninx = 0; ninx < MAXNOWAIT; ninx++) {
                    if (verbose)
                        printf("client wr2, fnum=%d, linx=%d, oinx=%d, ninx=%d, rd=%d\n",
                               filenumc2[oinx], linx, oinx, ninx, rd);
                    sprintf(send_buffer2[oinx][ninx], "hello2, greetings from %s, oinx=%d, ninx=%d, rd=%d",
                            my_name, oinx, ninx, rd);
                    cc = XWRITEREADX(filenumc2[oinx],
                                     send_buffer2[oinx][ninx],
                                     (short) (strlen(send_buffer2[oinx][ninx]) + 1),
                                     BUFSIZ,
                                     &count_read,
                                     rd);
                    TEST_CHK_CCEQ(cc);
                    rd++;
                    if (rd >= (recvdepth + RDOVER))
                        break;
                }
            }

            rd = 0;
            for (oinx = 0; oinx < maxopen1; oinx++) {
                for (ninx = 0; ninx < MAXNOWAIT; ninx++) {
                    tfilenum = filenumc1[oinx];
                    if (verbose)
                        printf("client awaitio1, fnum=%d, linx=%d, oinx=%d, ninx=%d, rd=%d\n",
                               tfilenum, linx, oinx, ninx, rd);
                    cc = XAWAITIOX(&tfilenum,
                                   &buf,
                                   &count_xferred,
                                   &tag,
                                   timeout,
                                   NULL);
                    TEST_CHK_CCEQ(cc);
                    send_buffer1[oinx][ninx][count_xferred] = 0;
                    printf("%s\n", send_buffer1[oinx][ninx]);
                    rd++;
                    if (rd >= recvdepth)
                        break;
                }
            }
            rinx = 0;
            for (oinx = 0; oinx < maxopen2; oinx++) {
                for (ninx = 0; ninx < MAXNOWAIT; ninx++) {
                    tfilenum = filenumc2[oinx];
                    if (verbose)
                        printf("client awaitio2, fnum=%d, linx=%d, oinx=%d, ninx=%d, rd=%d\n",
                               tfilenum, linx, oinx, ninx, rd);
                    disable = msg_test_assert_disable();
                    cc = XAWAITIOX(&tfilenum,
                                   &buf,
                                   &count_xferred,
                                   &tag,
                                   timeout,
                                   NULL);
                    msg_test_assert_enable(disable);
                    msg_ok[rinx] = _xstatus_eq(cc);
                    if (msg_ok[rinx]) {
                        send_buffer2[oinx][ninx][count_xferred] = 0;
                        printf("%s\n", send_buffer2[oinx][ninx]);
                    } else if (verbose)
                        printf("client awaitio2-err\n");
                    rinx++;
                    rd++;
                    if (rd >= (recvdepth + RDOVER))
                        break;
                }
            }

            //
            // re-link any failures
            //
            rd = recvdepth;
            rinx = 0;
            for (oinx = 0; oinx < maxopen2; oinx++) {
                for (ninx = 0; ninx < MAXNOWAIT; ninx++) {
                    if (msg_ok[rinx++])
                        continue;
                    tfilenum = filenumc2[oinx];
                    if (verbose)
                        printf("client wr3, fnum=%d, linx=%d, oinx=%d, ninx=%d, rd=%d\n",
                               filenumc2[oinx], linx, oinx, ninx, rd);
                    sprintf(send_buffer2[oinx][ninx], "hello3, greetings from %s, oinx=%d, ninx=%d, rd=%d",
                            my_name, oinx, ninx, rd);
                    cc = XWRITEREADX(tfilenum,
                                     send_buffer2[oinx][ninx],
                                     (short) (strlen(send_buffer2[oinx][ninx]) + 1),
                                     BUFSIZ,
                                     &count_read,
                                     rd);
                    TEST_CHK_CCEQ(cc);
                    if (verbose)
                        printf("client awaitio3, fnum=%d, linx=%d, oinx=%d, ninx=%d, rd=%d\n",
                               tfilenum, linx, oinx, ninx, rd);
                    cc = XAWAITIOX(&tfilenum,
                                   &buf,
                                   &count_xferred,
                                   &tag,
                                   timeout,
                                   NULL);
                    TEST_CHK_CCEQ(cc);
                    send_buffer2[oinx][ninx][count_xferred] = 0;
                    printf("%s\n", send_buffer2[oinx][ninx]);
                    rd++;
                    if (rd >= (recvdepth + RDOVER))
                        break;
                }
            }
        }

    } else {
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                           0, 0, 0,
                           (short) recvdepth, 1, // no sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (linx = 0; linx < loop; linx++) {
            for (rinx = 0; rinx < recvdepth; rinx++) {
                if (verbose)
                    printf("server ru rinx=%d, rd=%d\n", rinx, rinx);
                cc = XREADUPDATEX(filenum,
                                  recv_buffer[rinx],
                                  BUFSIZ,
                                  &count_read,
                                  0);
                TEST_CHK_CCEQ(cc);
            }
            sleep(1); // force NOLCB
            for (rinx = 0; rinx < recvdepth; rinx++) {
                if (verbose)
                    printf("server reply rinx=%d, rd=%d\n", rinx, rinx);
                strcat(recv_buffer[rinx], "- reply from ");
                strcat(recv_buffer[rinx], my_name);
                count_read = (unsigned short) (strlen(recv_buffer[rinx]) + 1); // cast
                cc = XREPLYX(recv_buffer[rinx],
                             count_read,
                             &count_written,
                             (short) rinx,
                             XZFIL_ERR_OK);
                TEST_CHK_CCEQ(cc);
            }
            sleep(1); // delay
            rd = recvdepth;
            for (rinx = 0; rinx < RDOVER; rinx++) {
                if (verbose)
                    printf("server ru (over) rinx=%d, rd=%d\n", rinx, rd);
                cc = XREADUPDATEX(filenum,
                                  recv_buffer[rinx],
                                  BUFSIZ,
                                  &count_read,
                                  0);
                TEST_CHK_CCEQ(cc);
                if (verbose)
                    printf("server reply (over) rinx=%d, rd=%d\n", rinx, rd);
                strcat(recv_buffer[rinx], "- reply from ");
                strcat(recv_buffer[rinx], my_name);
                count_read = (unsigned short) (strlen(recv_buffer[rinx]) + 1); // cast
                cc = XREPLYX(recv_buffer[rinx],
                             count_read,
                             &count_written,
                             (short) 0,
                             XZFIL_ERR_OK);
                TEST_CHK_CCEQ(cc);
                rd++;
            }
        }

        ferr = XFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEIGNORE(ferr);
    util_test_finish(client);
    return 0;
}
