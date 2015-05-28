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
#include "ttrans.h"
#include "tutil.h"
#include "tutilp.h"
#include "ufsri.h"

enum {              MAX_OUT = 2 };
bool                client       = false;
MS_Mon_Transid_Type curr_transid;
int                 exp_fun      = -1;
int                 exp_fun_next = -1;
MS_Mon_Transid_Type exp_trans;
bool                print        = false;


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

void trans_check(MS_Mon_Transid_Type trans) {

    if (!TRANSID_EQUALS(trans, exp_trans)) {
        char exp_transbuf[100];
        char transbuf[100];
        util_format_transid(exp_transbuf, exp_trans);
        util_format_transid(transbuf, trans);
        printf("client=%d, expected trans=%s, actual trans=%s\n",
               client, exp_transbuf, transbuf);
    }
    assert(TRANSID_EQUALS(trans, exp_trans));
}

void trans_set(MS_Mon_Transid_Type trans) {
    exp_trans = trans;
}

void trans_set_curr(MS_Mon_Transid_Type trans) {
    curr_transid = trans;
}

int tmlib(MS_Mon_Tmlib_Fun_Type  fun,
          MS_Mon_Transid_Type    transid,
          MS_Mon_Transid_Type   *transid_out) {
    const char          *funp;
    MS_Mon_Transid_Type  ltransid;

    if (exp_fun < 0) {
        printf("client=%d, cb not expected\n", client);
        assert(false); // callback not expected
    } else
        fun_check(fun);
    switch (fun) {
    case TMLIB_FUN_REG_TX:
        funp = "REG";
        trans_set_curr(transid);
        break;
    case TMLIB_FUN_CLEAR_TX:
        funp = "CLEAR";
        TRANSID_SET_NULL(ltransid);
        trans_set_curr(ltransid);
        break;
    case TMLIB_FUN_REINSTATE_TX:
        funp = "REINSTATE";
        if (client)
            trans_check(transid);
        trans_set_curr(transid);
        break;
    case TMLIB_FUN_GET_TX:
        funp = "GET";
        break;
    default:
        funp = "<unknown>";
        TRANSID_SET_NULL(ltransid);
        trans_set_curr(ltransid);
        break;
    }
    if (print) {
        char transidbuf[100];
        char curr_transidbuf[100];
        const char *clientp = client ? "client" : "server";
        util_format_transid(transidbuf, transid);
        util_format_transid(curr_transidbuf, curr_transid);
        printf("%s tmlib callback, fun=%s(%d), transid=%s, curr-transid=%s\n",
               clientp, funp, fun, transidbuf, curr_transidbuf);
    }
    fun_set(exp_fun_next, -1);
    if (fun == TMLIB_FUN_GET_TX)
        *transid_out = curr_transid;
    return 0;
}

_xcc_status         cc[MAX_OUT];
char                my_name[BUFSIZ];
char                recv_buffer[MAX_OUT][BUFSIZ];
RI_Type             ri[MAX_OUT];
char                send_buffer[MAX_OUT][BUFSIZ];
int                 tm_seq[MAX_OUT];
MS_Mon_Transid_Type transid[MAX_OUT];

int main(int argc, char *argv[]) {
    void           *buf;
    unsigned short  count_read;
    unsigned short  count_written;
    unsigned short  count_xferred;
    int             ferr;
    short           filenum;
    int             inx;
    int             loop = 10;
    int             nid;
    bool            nowait = false;
    char           *p;
    int             pid;
    int             ru;
    SB_Tag_Type     tag;
    short           tfilenum;
    int             timeout = -1;
    TAD             zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-nowait",    TA_Bool, TA_NOMAX,    &nowait    },
      { "-print",     TA_Bool, TA_NOMAX,    &print     },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
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

    ferr = msg_mon_trans_register_tmlib(tmlib);
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_process_info(NULL, &nid, &pid);
    TEST_CHK_FEOK(ferr);
    if (client) {
        ferr = XFILE_OPEN_((char *) "$srv", 4, &filenum,
                           0, 0, MAX_OUT,
                           0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            ferr = msg_mon_get_tm_seq(&tm_seq[0]);
            TEST_CHK_FEOK(ferr);
            ferr = msg_mon_get_tm_seq(&tm_seq[1]);
            TEST_CHK_FEOK(ferr);
            TRANSID_SET_SEQ(transid[0], tm_seq[0]);
            TRANSID_SET_SEQ(transid[1], tm_seq[1]);
            sprintf(send_buffer[0], "hello, greetings from %s, inx=%d-1, trans=%d",
                    my_name, inx, tm_seq[0]);
            sprintf(send_buffer[1], "hello, greetings from %s, inx=%d-2, trans=%d",
                    my_name, inx, tm_seq[1]);
            if (print)
                printf("client enlist, transid=%d\n", tm_seq[0]);
            ferr = msg_mon_trans_enlist(nid, pid, transid[0]);
            TEST_CHK_FEOK(ferr);
            fun_set(TMLIB_FUN_GET_TX, TMLIB_FUN_REINSTATE_TX);
            trans_set_curr(transid[0]);
            cc[0] = XWRITEREADX(filenum,
                                send_buffer[0],
                                (short) (strlen(send_buffer[0]) + 1),
                                BUFSIZ,
                                &count_read,
                                0);
            ferr = msg_mon_trans_enlist(nid, pid, transid[1]);
            TEST_CHK_FEOK(ferr);
            fun_set(TMLIB_FUN_GET_TX, TMLIB_FUN_REINSTATE_TX);
            trans_set_curr(transid[1]);
            cc[1] = XWRITEREADX(filenum,
                              send_buffer[1],
                              (short) (strlen(send_buffer[1]) + 1),
                              BUFSIZ,
                              &count_read,
                              0);
            TEST_CHK_CCEQ(cc[0]);
            TEST_CHK_CCEQ(cc[1]);
            tfilenum = filenum;
            trans_set(transid[1]);
            cc[0] = XAWAITIOX(&tfilenum,
                              &buf,
                              &count_xferred,
                              &tag,
                              timeout,
                              NULL);
            fun_set(TMLIB_FUN_REINSTATE_TX, -1);
            trans_set(transid[0]);
            cc[1] = XAWAITIOX(&tfilenum,
                              &buf,
                              &count_xferred,
                              &tag,
                              timeout,
                              NULL);
            TEST_CHK_CCEQ(cc[0]);
            TEST_CHK_CCEQ(cc[1]);
            fun_check(-1);
            if (print)
                printf("client end, transid=%d\n", tm_seq[0]);
            ferr = msg_mon_trans_end(nid, pid, transid[0]);
            TEST_CHK_FEOK(ferr);
            if (print)
                printf("client end, transid=%d\n", tm_seq[1]);
            ferr = msg_mon_trans_end(nid, pid, transid[1]);
            TEST_CHK_FEOK(ferr);
            printf("%s\n", send_buffer[0]);
        }
        ferr = XFILE_CLOSE_(filenum, XZFIL_ERR_OK);
        TEST_CHK_FEOK(ferr);
    } else {
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                           0, 0, nowait ? (short) 1 : (short) 0,
                           MAX_OUT, 1, // no sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            for (ru = 0; ru < MAX_OUT; ru++) {
                fun_set(TMLIB_FUN_REG_TX, -1);
                cc[0] = XREADUPDATEX(filenum,
                                     recv_buffer[ru],
                                     BUFSIZ,
                                     &count_read,
                                     0);
                TEST_CHK_CCEQ(cc[0]);
                if (nowait) {
                    tfilenum = filenum;
                    cc[0] = XAWAITIOX(&tfilenum,
                                      &buf,
                                      &count_xferred,
                                      &tag,
                                      timeout,
                                      NULL);
                    TEST_CHK_CCEQ(cc[0]);
                }
                getri(&ri[ru]);
                p = &recv_buffer[ru][strlen(recv_buffer[ru])];
                while ((p > recv_buffer[ru]) && (*p != '='))
                    p--;
                if (*p == '=') {
                    p++;
                    sscanf(p, "%d", &tm_seq[ru]);
                    TRANSID_SET_SEQ(transid[ru], tm_seq[ru]);
                } else
                    TRANSID_SET_NULL(transid[ru]);
                trans_set(transid[ru]);
                trans_check(curr_transid);
            }
            for (ru = 1; ru >= 0; ru--) {
                fun_check(-1);
                fun_set(TMLIB_FUN_REG_TX, TMLIB_FUN_CLEAR_TX);
                cc[0] = XACTIVATERECEIVETRANSID((short) ru);
                TEST_CHK_CCEQ(cc[0]);
                trans_set(transid[ru]);
                trans_check(curr_transid);
                strcat(recv_buffer[ru], "- reply from ");
                strcat(recv_buffer[ru], my_name);
                count_read = (short) (strlen(recv_buffer[ru]) + 1);
                cc[0] = XREPLYX(recv_buffer[ru],
                                count_read,
                                &count_written,
                                ri[ru].message_tag,
                                XZFIL_ERR_OK);
                TEST_CHK_CCEQ(cc[0]);
                fun_check(-1);
            }
        }
        ferr = XFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
