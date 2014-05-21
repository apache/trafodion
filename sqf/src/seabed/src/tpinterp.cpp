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

//
// Implement tp-interpreter (test point interpreter) module
//

#include <stdarg.h>

#include "seabed/sautil.h"
#include "seabed/tpinterp.h"

#include "verslib.h"

VERS_LIB(libsbtpinterp)



enum {
    // 1234567890123456
    // HH:MM:SS.MMM.UUU
    SB_INTERP_MAX_TOD = 20
};

//
// statics
//
SB_TP_Interp_Op::Factory_Map       SB_TP_Interp_Op::cv_op_factories;
SB_TP_Interp_Seq_Op::Seq_Map       SB_TP_Interp_Seq_Op::cv_seqs;

static SB_TP_Interp_Op_Loop        gv_sb_tp_interp_loop_reg;
static SB_TP_Interp_Op_Seq         gv_sb_tp_interp_seq_reg;
static SB_TP_Interp_Op_Sleep       gv_sb_tp_interp_sleep_reg;
static SB_TP_Interp_Op_Thread_Wait gv_sb_tp_interp_thread_wait_reg;
static SB_TP_Interp_Op_Usleep      gv_sb_tp_interp_usleep_reg;

// ----------------------------------------------------------------------------

//
// debug-printf
//
void sb_tp_interp_debug_printf(const char *pp_where,
                               const char *pp_format,
                               ...) {
    va_list lv_ap;

    va_start(lv_ap, pp_format);
    SB_SA_Util_Debug::debug_vprintf(pp_where, pp_format, lv_ap);
    va_end(lv_ap);
}

// ----------------------------------------------------------------------------

//
// op constructor
//
SB_TP_Interp_Op::SB_TP_Interp_Op(const char *pp_op_name,
                                 int         pv_exp_arg_cnt,
                                 Op_Factory  pv_op_factory) {
    const char    *WHERE = "SB_TP_Interp_Op::SB_TP_Interp_Op";
    Factory_Entry *lp_op_factory;

    if (pp_op_name != NULL) {
        // no duplicate factory names
        lp_op_factory = cv_op_factories.get(pp_op_name);
        if (lp_op_factory != NULL) {
            SB_SA_Util_Error::error_printf(WHERE,
                                           "duplicate op-name=%s\n",
                                           pp_op_name);
            SB_SA_util_assert_ine(strcmp(pp_op_name, lp_op_factory->ip_op_name), 0);
        }

        lp_op_factory = new Factory_Entry;
        lp_op_factory->ip_op_name = pp_op_name;
        lp_op_factory->iv_exp_arg_cnt = pv_exp_arg_cnt;
        lp_op_factory->iv_factory = pv_op_factory;
        cv_op_factories.put(pp_op_name, lp_op_factory);
    }
}

//
// op constructor
//
SB_TP_Interp_Op::SB_TP_Interp_Op(const char *pp_op_name, long pv_p1)
: ip_op_name(pp_op_name), iv_p1(pv_p1) {
}

//
// op destructor
//
SB_TP_Interp_Op::~SB_TP_Interp_Op() {
}

//
// create op
//
SB_TP_Interp_Op *SB_TP_Interp_Op::op_create(int   pv_argc,
                                            char *pa_argv[],
                                            bool  pv_debug) {
    const char      *WHERE = "SB_TP_Interp_Op::op_create";
    Factory_Entry   *lp_factory;
    SB_TP_Interp_Op *lp_op;
    char            *lp_op_str;
    int              lv_inx;

    if (pv_debug) {
        SB_SA_Util_Debug::debug_printf(WHERE, "argc=%d\n", pv_argc);
        for (lv_inx = 0; lv_inx < pv_argc; lv_inx++)
            SB_SA_Util_Debug::debug_printf(WHERE, "argv[%d]=%s\n",
                                           lv_inx, pa_argv[lv_inx]);
    }
    lp_op_str = pa_argv[0];
    lp_factory = cv_op_factories.get(lp_op_str);
    if (lp_factory != NULL) {
        if (pv_argc < lp_factory->iv_exp_arg_cnt) {
            SB_SA_Util_Error::error_printf(WHERE,
                                           "Not enough arguments supplied, expecting at least %d, but only %d specified\n",
                                           lp_factory->iv_exp_arg_cnt,
                                           pv_argc);
            SB_SA_util_assert_ige(pv_argc, lp_factory->iv_exp_arg_cnt);
        }
        return lp_factory->iv_factory(pv_argc, pa_argv, pv_debug);
    }
    lp_op = NULL;
    SB_SA_Util_Error::error_printf(WHERE, "op=%s has no factory\n", lp_op_str);
    SB_SA_util_assert_cpne(lp_op, NULL);
    return lp_op;
}

//
// op list
//
void SB_TP_Interp_Op::op_list(int pv_level, int pv_inx) {
    int lv_inx;

    for (lv_inx = 0; lv_inx < pv_level; lv_inx++)
        printf(" ");
    printf("op[%d]=%s, p1=%ld\n", pv_inx, ip_op_name, iv_p1);
}

//
// op run enter debug
//
void SB_TP_Interp_Op::op_run_enter(int pv_inx, bool pv_debug) {
    const char *WHERE = "SB_TP_Interp_Op::op_run_enter";

    if (pv_debug)
        SB_SA_Util_Debug::debug_printf(WHERE, "ENTER op[%d]=%s, p1=%ld\n",
                                       pv_inx, ip_op_name, iv_p1);
}

//
// op run exit debug
//
int SB_TP_Interp_Op::op_run_exit(int pv_inx, bool pv_debug, int pv_ret) {
    const char *WHERE = "SB_TP_Interp_Op::op_run_exit";

    if (pv_debug)
        SB_SA_Util_Debug::debug_printf(WHERE, "EXIT op[%d]=%s, ret=%d\n",
                                       pv_inx, ip_op_name, pv_ret);

    return pv_ret;
}

// ----------------------------------------------------------------------------

//
// op-loop constructor
//
SB_TP_Interp_Op_Loop::SB_TP_Interp_Op_Loop()
: SB_TP_Interp_Op("loop",
                  4, // loop,<cnt>,<seq>,<seq-file>
                  op_create) {
}

//
// op-loop constructor
//
SB_TP_Interp_Op_Loop::SB_TP_Interp_Op_Loop(long pv_p1, SB_TP_Interp_Seq_Op *pp_seq)
: SB_TP_Interp_Op("loop", pv_p1), ip_seq(pp_seq) {
}

//
// op-loop destructor
//
SB_TP_Interp_Op_Loop::~SB_TP_Interp_Op_Loop() {
}

//
// create op
//
SB_TP_Interp_Op *SB_TP_Interp_Op_Loop::op_create(int   pv_argc,
                                                 char *pa_argv[],
                                                 bool  pv_debug) {
    const char          *WHERE = "SB_TP_Interp_Op_Loop::op_create";
    SB_TP_Interp_Op     *lp_op;
    char                *lp_p1;
    char                *lp_p2;
    char                *lp_p3;
    SB_TP_Interp_Seq_Op *lp_seq;
    int                  lv_p1;

    pv_argc = pv_argc; // touch
    lp_p1 = pa_argv[1];
    lv_p1 = atoi(lp_p1);
    lp_p2 = pa_argv[2];
    lp_p3 = pa_argv[3];
    if (pv_debug)
        SB_SA_Util_Debug::debug_printf(WHERE, "p1=%d, p2=%s, p3=%s\n",
                                       lv_p1, lp_p2, lp_p3);
    lp_seq = new SB_TP_Interp_Seq_Op(lp_p2, lp_p3, pv_debug);
    lp_op = new SB_TP_Interp_Op_Loop(lv_p1, lp_seq);
    return lp_op;
}

//
// op-loop list
//
void SB_TP_Interp_Op_Loop::op_list(int pv_level, int pv_inx) {
    SB_TP_Interp_Op::op_list(pv_level, pv_inx);
    ip_seq->seq_list(pv_level + 1);
}

//
// op-loop run
//
int SB_TP_Interp_Op_Loop::op_run(int pv_inx, bool pv_debug) {
    const char *WHERE = "SB_TP_Interp_Op_Loop::op_run";
    int         lv_inx;

    op_run_enter(pv_inx, pv_debug);
    for (lv_inx = 0; lv_inx < iv_p1; lv_inx++) {
        if (pv_debug)
            SB_SA_Util_Debug::debug_printf(WHERE, "inx=%d\n", lv_inx);
        ip_seq->seq_run(pv_debug);
    }
    return op_run_exit(pv_inx, pv_debug, 0);
}

// ----------------------------------------------------------------------------

//
// op-seq constructor
//
SB_TP_Interp_Op_Seq::SB_TP_Interp_Op_Seq()
: SB_TP_Interp_Op("seq",
                  3, // seq,<seq-name>,<seq-file>
                  op_create) {
}

//
// op-seq constructor
//
SB_TP_Interp_Op_Seq::SB_TP_Interp_Op_Seq(SB_TP_Interp_Seq_Op *pp_seq)
: SB_TP_Interp_Op("seq", 0), ip_seq(pp_seq) {
}

//
// op-seq destructor
//
SB_TP_Interp_Op_Seq::~SB_TP_Interp_Op_Seq() {
}

//
// create op
//
SB_TP_Interp_Op *SB_TP_Interp_Op_Seq::op_create(int   pv_argc,
                                                char *pa_argv[],
                                                bool  pv_debug) {
    const char          *WHERE = "SB_TP_Interp_Op_Seq::op_create";
    SB_TP_Interp_Op     *lp_op;
    char                *lp_p1;
    char                *lp_p2;
    SB_TP_Interp_Seq_Op *lp_seq;

    pv_argc = pv_argc; // touch
    lp_p1 = pa_argv[1];
    lp_p2 = pa_argv[2];
    if (pv_debug)
        SB_SA_Util_Debug::debug_printf(WHERE, "p1=%s, p2=%s\n", lp_p1, lp_p2);
    lp_seq = new SB_TP_Interp_Seq_Op(lp_p1, lp_p2, pv_debug);
    lp_op = new SB_TP_Interp_Op_Seq(lp_seq);
    return lp_op;
}

//
// op-seq list
//
void SB_TP_Interp_Op_Seq::op_list(int pv_level, int pv_inx) {
    SB_TP_Interp_Op::op_list(pv_level, pv_inx);
    ip_seq->seq_list(pv_level + 1);
}

//
// op-seq run
//
int SB_TP_Interp_Op_Seq::op_run(int pv_inx, bool pv_debug) {
    op_run_enter(pv_inx, pv_debug);
    ip_seq->seq_run(pv_debug);
    return op_run_exit(pv_inx, pv_debug, 0);
}

// ----------------------------------------------------------------------------

//
// op-sleep constructor
//
SB_TP_Interp_Op_Sleep::SB_TP_Interp_Op_Sleep()
: SB_TP_Interp_Op("sleep",
                  2, // sleep,<p1>
                  op_create) {
}

//
// op-sleep constructor
//
SB_TP_Interp_Op_Sleep::SB_TP_Interp_Op_Sleep(long pv_p1)
: SB_TP_Interp_Op("sleep", pv_p1) {
}

//
// op-sleep destructor
//
SB_TP_Interp_Op_Sleep::~SB_TP_Interp_Op_Sleep() {
}

//
// create op
//
SB_TP_Interp_Op *SB_TP_Interp_Op_Sleep::op_create(int   pv_argc,
                                                  char *pa_argv[],
                                                  bool  pv_debug) {
    const char      *WHERE = "SB_TP_Interp_Op_Sleep::op_create";
    SB_TP_Interp_Op *lp_op;
    char            *lp_p1;
    int              lv_p1;

    pv_argc = pv_argc; // touch
    lp_p1 = pa_argv[1];
    lv_p1 = atoi(lp_p1);
    if (pv_debug)
        SB_SA_Util_Debug::debug_printf(WHERE, "p1=%d\n", lv_p1);
    lp_op = new SB_TP_Interp_Op_Sleep(lv_p1);
    return lp_op;
}

//
// op-sleep run
//
int SB_TP_Interp_Op_Sleep::op_run(int pv_inx, bool pv_debug) {
    op_run_enter(pv_inx, pv_debug);
    sleep(static_cast<unsigned int>(iv_p1));
    return op_run_exit(pv_inx, pv_debug, 0);
}

// ----------------------------------------------------------------------------

//
// op-thread-wait constructor
//
SB_TP_Interp_Op_Thread_Wait::SB_TP_Interp_Op_Thread_Wait()
: SB_TP_Interp_Op("thread-wait",
                  2, // thread-wait,<p1>
                  op_create) {
}

//
// op-thread-wait constructor
//
SB_TP_Interp_Op_Thread_Wait::SB_TP_Interp_Op_Thread_Wait(long pv_p1)
: SB_TP_Interp_Op("thread-wait", pv_p1), iv_count(0) {
    iv_sem.init(false, static_cast<unsigned int>(pv_p1));
}

//
// op-thread-wait destructor
//
SB_TP_Interp_Op_Thread_Wait::~SB_TP_Interp_Op_Thread_Wait() {
}

//
// create op
//
SB_TP_Interp_Op *SB_TP_Interp_Op_Thread_Wait::op_create(int   pv_argc,
                                                        char *pa_argv[],
                                                        bool  pv_debug) {
    const char      *WHERE = "SB_TP_Interp_Op_Thread_Wait::op_create";
    SB_TP_Interp_Op *lp_op;
    char            *lp_p1;
    int              lv_p1;

    pv_argc = pv_argc; // touch
    lp_p1 = pa_argv[1];
    lv_p1 = atoi(lp_p1);
    if (pv_debug)
        SB_SA_Util_Debug::debug_printf(WHERE, "p1=%d\n", lv_p1);
    lp_op = new SB_TP_Interp_Op_Thread_Wait(lv_p1);
    return lp_op;
}

//
// op-thread-wait run
//
int SB_TP_Interp_Op_Thread_Wait::op_run(int pv_inx, bool pv_debug) {
    const char *WHERE = "SB_TP_Interp_Op_Thread_Wait::op_run";
    const char *lp_msg;
    bool        lv_wait;

    op_run_enter(pv_inx, pv_debug);

    //
    // let p1 threads into the semaphore
    //
    iv_sem.wait();

    iv_cv.lock();
    iv_count++;

    lv_wait = (iv_count < iv_p1);

    if (pv_debug) {
        if (lv_wait)
            lp_msg = "WAITing";
        else
            lp_msg = "SIGNALing";
        SB_SA_Util_Debug::debug_printf(WHERE, "%s, count=%d\n",
                                       lp_msg, iv_count);
    }

    if (lv_wait)
        iv_cv.wait(false);
    else
        iv_cv.broadcast(false);

    iv_cv.unlock();

    iv_sem.post();

    return op_run_exit(pv_inx, pv_debug, 0);
}

// ----------------------------------------------------------------------------

//
// op-usleep constructor
//
SB_TP_Interp_Op_Usleep::SB_TP_Interp_Op_Usleep()
: SB_TP_Interp_Op("usleep",
                  2, // usleep,<p1>
                  op_create) {
}

//
// op-usleep constructor
//
SB_TP_Interp_Op_Usleep::SB_TP_Interp_Op_Usleep(long pv_p1)
: SB_TP_Interp_Op("usleep", pv_p1) {
}

//
// op-usleep destructor
//
SB_TP_Interp_Op_Usleep::~SB_TP_Interp_Op_Usleep() {
}

//
// create op
//
SB_TP_Interp_Op *SB_TP_Interp_Op_Usleep::op_create(int   pv_argc,
                                                   char *pa_argv[],
                                                   bool  pv_debug) {
    const char      *WHERE = "SB_TP_Interp_Op_Usleep::op_create";
    SB_TP_Interp_Op *lp_op;
    char            *lp_p1;
    long             lv_p1;

    pv_argc = pv_argc; // touch
    lp_p1 = pa_argv[1];
    lv_p1 = atol(lp_p1);
    if (pv_debug)
        SB_SA_Util_Debug::debug_printf(WHERE, "p1=%ld\n", lv_p1);
    lp_op = new SB_TP_Interp_Op_Usleep(lv_p1);
    return lp_op;
}

//
// op-usleep run
//
int SB_TP_Interp_Op_Usleep::op_run(int pv_inx, bool pv_debug) {
    op_run_enter(pv_inx, pv_debug);
    usleep(static_cast<useconds_t>(iv_p1));
    return op_run_exit(pv_inx, pv_debug, 0);
}

// ----------------------------------------------------------------------------

//
// op-seq constructor
//
SB_TP_Interp_Seq_Op::SB_TP_Interp_Seq_Op(const char *pp_name) {
    init(pp_name);
}

//
// op-seq constructor
//
SB_TP_Interp_Seq_Op::SB_TP_Interp_Seq_Op(const char *pp_name,
                                         const char *pp_file,
                                         bool        pv_debug) {
    init(pp_name);
    file_read(pp_file, pv_debug);
}

//
// op-seq destructor
//
SB_TP_Interp_Seq_Op::~SB_TP_Interp_Seq_Op() {
    int lv_inx;

    for (lv_inx = 0; lv_inx < iv_ops.iv_count; lv_inx++)
        delete iv_ops.ipp_list[lv_inx];
    delete [] iv_ops.ipp_list;


    if (ip_seq_name != NULL)
        cv_seqs.remove(ip_seq_name);
}

//
// op-seq read file
//
void SB_TP_Interp_Seq_Op::file_read(const char *pp_file, bool pv_debug) {
    const char          *WHERE = "SB_TP_Interp_Seq_Op::file_read";
    enum {               MAX_ARGS = 20 };
    char                *la_arg[MAX_ARGS];
    SB_TP_Interp_Op     *lp_op;
    int                  lv_count;
    int                  lv_err;
    SB_SA_Util_File      lv_file(pp_file);
    SB_SA_Util_Op_Line   lv_line(&lv_file);

    for (;;) {
        lv_count = lv_line.get_op_line(la_arg, MAX_ARGS);
        if (lv_count == 0)
            break;
        lp_op = SB_TP_Interp_Op::op_create(lv_count, la_arg, pv_debug);
        lv_err = op_add(lp_op);
        if (lv_err != 0) {
            SB_SA_Util_Error::error_printf(WHERE, "op_add returned err=%d\n", lv_err);
            SB_SA_util_assert_ieq(lv_err, 0);
        }
    }
}

//
// op-seq init
//
void SB_TP_Interp_Seq_Op::init(const char *pp_seq_name) {
    const char *WHERE = "SB_TP_Interp_Seq_Op::init";
    Seq_Entry  *lp_entry;

    iv_ops.init();

    ip_seq_name = pp_seq_name;
    if (pp_seq_name != NULL) {
        lp_entry = cv_seqs.get(pp_seq_name);
        if (lp_entry != NULL) {
            SB_SA_Util_Error::error_printf(WHERE,
                                           "seq-name=%s, duplicate seq-names not allowed\n", pp_seq_name);
            SB_SA_util_assert_cpeq(lp_entry, NULL);
        }
        lp_entry = new Seq_Entry;
        lp_entry->ip_seq_name = pp_seq_name;
        lp_entry->ip_seq = this;
        cv_seqs.put(pp_seq_name, lp_entry);
    }
}

//
// op-seq add op
//
int SB_TP_Interp_Seq_Op::op_add(SB_TP_Interp_Op *pp_op) {
    iv_ops.cap_inc(OPS_CAP_INC);
    iv_ops.add(pp_op);

    return 0;
}

//
// op-seq list sequence
//
void SB_TP_Interp_Seq_Op::seq_list(int pv_level) {
    SB_TP_Interp_Op *lp_op;
    int              lv_inx;

    for (lv_inx = 0; lv_inx < iv_ops.iv_count; lv_inx++) {
        lp_op = iv_ops.ipp_list[lv_inx];
        lp_op->op_list(pv_level, lv_inx);
    }
}

//
// op-seq lookup sequence
//
SB_TP_Interp_Seq_Op *SB_TP_Interp_Seq_Op::seq_lookup(const char *pp_seq_name) {
    Seq_Entry *lp_entry;

    lp_entry = cv_seqs.get(pp_seq_name);
    if (lp_entry != NULL)
        return lp_entry->ip_seq;
    return NULL;
}

//
// op-seq run sequence
//
int SB_TP_Interp_Seq_Op::seq_run(bool pv_debug) {
    const char *WHERE = "SB_TP_Interp_Seq_Op::seq_run";
    int         lv_inx;
    int         lv_ret;

    for (lv_inx = 0; lv_inx < iv_ops.iv_count; lv_inx++) {
        lv_ret = iv_ops.ipp_list[lv_inx]->op_run(lv_inx, pv_debug);
        if (lv_ret != 0) {
            SB_SA_Util_Error::error_printf(WHERE, "op_run returned ret=%d\n", lv_ret);
            SB_SA_util_assert_ieq(lv_ret, 0);
        }
    }
    return 0;
}
