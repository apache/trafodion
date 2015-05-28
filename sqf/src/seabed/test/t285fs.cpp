//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2015 Hewlett-Packard Development Company, L.P.
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
#include "ttrans.h"
#include "tutil.h"
#include "tutilp.h"

bool                 client       = false;
MS_Mon_Transseq_Type curr_startid;
MS_Mon_Transid_Type  curr_transid;
int                  exp_fun      = -1;
int                  exp_fun_next = -1;
MS_Mon_Transseq_Type exp_startid;
MS_Mon_Transid_Type  exp_trans;
bool                 verbose      = false;


void fun_check(int fun) {
    if (fun != exp_fun)
        printf("client=%d, expected fun=%d, actual fun=%d\n",
               client, exp_fun, fun);
    assert(fun == exp_fun);
}

void fun_set(int fun, int fun_next) {
    exp_fun = fun;
    exp_fun_next = fun_next;
}

void trans_check(MS_Mon_Transid_Type  trans,
                 MS_Mon_Transseq_Type startid) {

    if (!TRANSID_EQUALS(trans, exp_trans) || !TRANSSEQ_EQUALS(startid, exp_startid)) {
        char exp_startidbuf[100];
        char exp_transbuf[100];
        char transbuf[100];
        char startidbuf[100];
        util_format_transid(exp_transbuf, exp_trans);
        util_format_startid(exp_startidbuf, exp_startid);
        util_format_transid(transbuf, trans);
        util_format_startid(startidbuf, startid);
        printf("client=%d, expected trans=%s, actual trans=%s, expected startid=%s, startid=%s\n",
               client, exp_transbuf, transbuf, exp_startidbuf, startidbuf);
    }
    assert(TRANSID_EQUALS(trans, exp_trans));
    assert(TRANSSEQ_EQUALS(startid, exp_startid));
}

void trans_set(MS_Mon_Transid_Type  trans,
               MS_Mon_Transseq_Type startid) {
    exp_trans = trans;
    exp_startid = startid;
}

void trans_set_curr(MS_Mon_Transid_Type  trans,
                    MS_Mon_Transseq_Type startid) {
    TRANSID_COPY(curr_transid, trans);
    TRANSSEQ_COPY(curr_startid, startid);
}

int tmlib2(MS_Mon_Tmlib_Fun_Type  fun,
           MS_Mon_Transid_Type    transid,
           MS_Mon_Transid_Type   *transid_out,
           MS_Mon_Transseq_Type   startid,
           MS_Mon_Transseq_Type  *startid_out) {
    const char           *funp;
    MS_Mon_Transseq_Type  lstartid;
    MS_Mon_Transid_Type   ltransid;

    if (exp_fun < 0) {
        printf("client=%d, cb not expected\n", client);
        assert(false); // callback not expected
    } else
        fun_check(fun);
    switch (fun) {
    case TMLIB_FUN_REG_TX:
        funp = "REG";
        trans_set_curr(transid, startid);
        break;
    case TMLIB_FUN_CLEAR_TX:
        funp = "CLEAR";
        TRANSID_SET_NULL(ltransid);
        TRANSSEQ_SET_NULL(lstartid);
        trans_set_curr(ltransid, lstartid);
        break;
    case TMLIB_FUN_REINSTATE_TX:
        funp = "REINSTATE";
        if (client)
            trans_check(transid, startid);
        trans_set_curr(transid, startid);
        break;
    case TMLIB_FUN_GET_TX:
        funp = "GET";
        break;
    default:
        funp = "<unknown>";
        TRANSID_SET_NULL(ltransid);
        TRANSSEQ_SET_NULL(lstartid);
        trans_set_curr(ltransid, lstartid);
        break;
    }
    if (verbose) {
        char startidbuf[100];
        char transidbuf[100];
        char curr_transidbuf[100];
        char curr_startidbuf[100];
        const char *clientp = client ? "client" : "server";
        util_format_transid(transidbuf, transid);
        util_format_startid(startidbuf, startid);
        util_format_transid(curr_transidbuf, curr_transid);
        util_format_startid(curr_startidbuf, curr_startid);
        printf("%s tmlib2 callback, fun=%s(%d), transid=%s, startid=%s, curr-transid=%s, curr-startid=%s\n",
               clientp,
               funp,
               fun,
               transidbuf,
               startidbuf,
               curr_transidbuf,
               curr_startidbuf);
    }
    fun_set(exp_fun_next, -1);
    if (fun == TMLIB_FUN_GET_TX) {
        *transid_out = curr_transid;
        *startid_out = curr_startid;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    void                 *buf;
    _xcc_status           cc;
    unsigned short        count_read;
    unsigned short        count_written;
    unsigned short        count_xferred;
    int                   ferr;
    short                 filenum;
    int                   inx;
    int                   loop = 10;
    char                  my_name[BUFSIZ];
    bool                  nowait = false;
    int                   nid;
    char                 *p;
    int                   pid;
    char                  recv_buffer[BUFSIZ];
    char                  send_buffer[BUFSIZ];
    MS_Mon_Transseq_Type  startid;
    SB_Tag_Type           tag;
    short                 tfilenum;
    int                   tm_seq;
    MS_Mon_Transseq_Type  tm_startid = 0;
    int                   timeout = -1;
    MS_Mon_Transid_Type   transid;
    TAD                   zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-nowait",    TA_Bool, TA_NOMAX,    &nowait    },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    TRANSID_SET_NULL(curr_transid);
    TRANSID_SET_NULL(exp_trans);
    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = file_mon_process_startup(!client); // system messages
    TEST_CHK_FEOK(ferr);
    util_gethostname(my_name, sizeof(my_name));

    ferr = msg_mon_trans_register_tmlib2(tmlib2);
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_process_info(NULL, &nid, &pid);
    TEST_CHK_FEOK(ferr);
    if (client) {
        ferr = XFILE_OPEN_((char *) "$srv", 4, &filenum,
                           0, 0, nowait ? (short) 1 : (short) 0,
                           0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            ferr = msg_mon_get_tm_seq(&tm_seq);
            TEST_CHK_FEOK(ferr);
            sprintf(send_buffer, "hello, greetings from %s, inx=%d, trans=%d, startid=%ld",
                    my_name, inx, tm_seq, tm_startid);
            if (verbose)
                printf("client enlist, transid=%d, startid=%ld\n",
                       tm_seq, tm_startid);
            TRANSID_SET_SEQ(transid, tm_seq);
            TRANSSEQ_SET_VALUE(startid, tm_startid);
            trans_set_curr(transid, startid);
            fun_set(TMLIB_FUN_GET_TX, TMLIB_FUN_REINSTATE_TX);
            trans_set(transid, startid);
            cc = XWRITEREADX(filenum,
                             send_buffer,
                             (short) (strlen(send_buffer) + 1),
                             BUFSIZ,
                             &count_read,
                             0);
            if (nowait) {
                TEST_CHK_CCEQ(cc);
                tfilenum = filenum;
                cc = XAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag,
                               timeout,
                               NULL);
            }
            TEST_CHK_CCEQ(cc);
            fun_check(-1);
            if (verbose)
                printf("client end, transid=%d\n", tm_seq);
            TRANSID_SET_SEQ(transid, tm_seq);
            printf("%s\n", send_buffer);
            tm_startid++;
        }
        ferr = XFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    } else {
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                           0, 0, nowait ? (short) 1 : (short) 0,
                           1, 1, // no sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            fun_set(TMLIB_FUN_REG_TX, -1);
            cc = XREADUPDATEX(filenum,
                              recv_buffer,
                              BUFSIZ,
                              &count_read,
                              0);
            TEST_CHK_CCEQ(cc);
            if (nowait) {
                tfilenum = filenum;
                cc = XAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag,
                               timeout,
                               NULL);
                TEST_CHK_CCEQ(cc);
            }
            p = &recv_buffer[strlen(recv_buffer)];
            while ((p > recv_buffer) && (*p != '='))
                p--;
            if (*p == '=') {
                char *xp = &p[-1];
                p++;
                sscanf(p, "%ld", &tm_startid);
                TRANSSEQ_SET_VALUE(startid, tm_startid);
                p = xp;
                while ((p > recv_buffer) && (*p != '='))
                    p--;
                if (*p == '=') {
                    p++;
                    sscanf(p, "%d", &tm_seq);
                    TRANSID_SET_SEQ(transid, tm_seq);
                } else
                    TRANSID_SET_NULL(transid);
            } else {
                TRANSID_SET_NULL(transid);
                TRANSSEQ_SET_NULL(startid);
            }
            trans_set(transid, startid);
            trans_check(curr_transid, curr_startid);
            fun_check(-1);
            fun_set(TMLIB_FUN_CLEAR_TX, -1);
            strcat(recv_buffer, "- reply from ");
            strcat(recv_buffer, my_name);
            count_read = (short) (strlen(recv_buffer) + 1);
            cc = XREPLYX(recv_buffer,
                         count_read,
                         &count_written,
                         0,
                         XZFIL_ERR_OK);
            TEST_CHK_FEOK(ferr);
            fun_check(-1);
        }
        ferr = XFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
