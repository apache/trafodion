//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@

//
// Implement standalone-utilities module
//

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

#include <sys/time.h>

#include "seabed/sautil.h"

#include "verslib.h"

VERS_LIB(libsbsautil)



enum {
    // 1234567890123456
    // HH:MM:SS.MMM.UUU
    SB_SA_UTIL_MAX_TOD = 20
};

// ----------------------------------------------------------------------------

enum {
    SB_SA_UTIL_SAVE_ASSERT_BUF_SIZE = 1024
};
typedef struct SB_SA_Util_Save_Assert {
    char               ia_exp[SB_SA_UTIL_SAVE_ASSERT_BUF_SIZE];
    char               ia_file[SB_SA_UTIL_SAVE_ASSERT_BUF_SIZE];
    char               ia_fun[SB_SA_UTIL_SAVE_ASSERT_BUF_SIZE];
    char               ia_op[SB_SA_UTIL_SAVE_ASSERT_BUF_SIZE];
    int                iv_errno;
    unsigned           iv_line;
    int                iv_pid;
    int                iv_ppid;
    int                iv_tid;
    long               iv_lhs;
    long               iv_rhs;
} SB_SA_Util_Save_Assert;

static SB_SA_Util_Save_Assert gv_sb_sa_util_save_assert;

static void SB_SA_Util_assert_fun_com(const char *pp_exp,
                                      long        pv_lhs,
                                      const char *pp_op,
                                      long        pv_rhs,
                                      const char *pp_file,
                                      unsigned    pv_line,
                                      const char *pp_fun);

void SB_SA_Util_abort_fun(const char *pp_msg,
                          const char *pp_file,
                          unsigned    pv_line,
                          const char *pp_fun) {
    SB_SA_Util_assert_fun_com(pp_msg,
                              0,  // lhs
                              "", // op
                              0,  // rhs
                              pp_file,
                              pv_line,
                              pp_fun);
}

void SB_SA_Util_assert_fun_com(const char *pp_exp,
                               long        pv_lhs,
                               const char *pp_op,
                               long        pv_rhs,
                               const char *pp_file,
                               unsigned    pv_line,
                               const char *pp_fun) {
    pid_t lv_pid;
    pid_t lv_tid;

    lv_pid = getpid();
    lv_tid = sb_sa_util_gettid();
    gv_sb_sa_util_save_assert.iv_errno = errno;
    gv_sb_sa_util_save_assert.iv_line = pv_line;
    gv_sb_sa_util_save_assert.iv_lhs = pv_lhs;
    gv_sb_sa_util_save_assert.iv_rhs = pv_rhs;
    strncpy(gv_sb_sa_util_save_assert.ia_exp,
            pp_exp,
            sizeof(gv_sb_sa_util_save_assert.ia_exp));
    strncpy(gv_sb_sa_util_save_assert.ia_file,
            pp_file,
            sizeof(gv_sb_sa_util_save_assert.ia_file));
    strncpy(gv_sb_sa_util_save_assert.ia_fun,
            pp_fun,
            sizeof(gv_sb_sa_util_save_assert.ia_fun));
    strncpy(gv_sb_sa_util_save_assert.ia_op,
            pp_op,
            sizeof(gv_sb_sa_util_save_assert.ia_op));

    abort();
}

void SB_SA_Util_assert_fun_cpeq(const char *pp_exp,
                                const void *pp_lhs,
                                const void *pp_rhs,
                                const char *pp_file,
                                unsigned    pv_line,
                                const char *pp_fun) {
    SB_SA_Util_assert_fun_com(pp_exp,
                              reinterpret_cast<long>(pp_lhs),
                              "==",
                              reinterpret_cast<long>(pp_rhs),
                              pp_file,
                              pv_line,
                              pp_fun);
}

void SB_SA_Util_assert_fun_cpne(const char *pp_exp,
                                const void *pp_lhs,
                                const void *pp_rhs,
                                const char *pp_file,
                                unsigned    pv_line,
                                const char *pp_fun) {
    SB_SA_Util_assert_fun_com(pp_exp,
                              reinterpret_cast<long>(pp_lhs),
                              "!=",
                              reinterpret_cast<long>(pp_rhs),
                              pp_file,
                              pv_line,
                              pp_fun);
}

void SB_SA_Util_assert_fun_ieq(const char *pp_exp,
                               int         pv_lhs,
                               int         pv_rhs,
                               const char *pp_file,
                               unsigned    pv_line,
                               const char *pp_fun) {
    SB_SA_Util_assert_fun_com(pp_exp,
                              pv_lhs,
                              "==",
                              pv_rhs,
                              pp_file,
                              pv_line,
                              pp_fun);
}

void SB_SA_Util_assert_fun_if(const char *pp_exp,
                              int         pv_exp,
                              const char *pp_file,
                              unsigned    pv_line,
                              const char *pp_fun) {
    SB_SA_Util_assert_fun_com(pp_exp,
                              pv_exp,
                              "!",
                              0,
                              pp_file,
                              pv_line,
                              pp_fun);
}

void SB_SA_Util_assert_fun_ige(const char *pp_exp,
                               int         pv_lhs,
                               int         pv_rhs,
                               const char *pp_file,
                               unsigned    pv_line,
                               const char *pp_fun) {
    SB_SA_Util_assert_fun_com(pp_exp,
                              pv_lhs,
                              ">=",
                              pv_rhs,
                              pp_file,
                              pv_line,
                              pp_fun);
}

void SB_SA_Util_assert_fun_igt(const char *pp_exp,
                               int         pv_lhs,
                               int         pv_rhs,
                               const char *pp_file,
                               unsigned    pv_line,
                               const char *pp_fun) {
    SB_SA_Util_assert_fun_com(pp_exp,
                              pv_lhs,
                              ">",
                              pv_rhs,
                              pp_file,
                              pv_line,
                              pp_fun);
}

void SB_SA_Util_assert_fun_ile(const char *pp_exp,
                               int         pv_lhs,
                               int         pv_rhs,
                               const char *pp_file,
                               unsigned    pv_line,
                               const char *pp_fun) {
    SB_SA_Util_assert_fun_com(pp_exp,
                              pv_lhs,
                              "<=",
                              pv_rhs,
                              pp_file,
                              pv_line,
                              pp_fun);
}

void SB_SA_Util_assert_fun_ilt(const char *pp_exp,
                               int         pv_lhs,
                               int         pv_rhs,
                               const char *pp_file,
                               unsigned    pv_line,
                               const char *pp_fun) {
    SB_SA_Util_assert_fun_com(pp_exp,
                              pv_lhs,
                              "<",
                              pv_rhs,
                              pp_file,
                              pv_line,
                              pp_fun);
}

void SB_SA_Util_assert_fun_ine(const char *pp_exp,
                               int         pv_lhs,
                               int         pv_rhs,
                               const char *pp_file,
                               unsigned    pv_line,
                               const char *pp_fun) {
    SB_SA_Util_assert_fun_com(pp_exp,
                              pv_lhs,
                              "!=",
                              pv_rhs,
                              pp_file,
                              pv_line,
                              pp_fun);
}

void SB_SA_Util_assert_fun_it(const char *pp_exp,
                              int         pv_exp,
                              const char *pp_file,
                              unsigned    pv_line,
                              const char *pp_fun) {
    SB_SA_Util_assert_fun_com(pp_exp,
                              pv_exp,
                              "",
                              0,
                              pp_file,
                              pv_line,
                              pp_fun);
}

// ----------------------------------------------------------------------------

SB_SA_Util_CV::SB_SA_Util_CV() : iv_flag(false) {
    const char *WHERE = "SB_SA_Util_CV::SB_SA_Util_CV";
    int         lv_status;

    lv_status = pthread_cond_init(&iv_cv, NULL);
    if (lv_status != 0) {
        SB_SA_Util_Error::error_printf(WHERE, "pthread_cond_init status=%d\n",
                                       lv_status);
        SB_SA_util_assert_ieq(lv_status, 0);
    }
}

SB_SA_Util_CV::~SB_SA_Util_CV() {
    const char *WHERE = "SB_SA_Util_CV::~SB_SA_Util_CV";
    int         lv_status;

    lv_status = pthread_cond_destroy(&iv_cv);
    if (lv_status != 0) {
        SB_SA_Util_Error::error_printf(WHERE, "pthread_cond_destroy status=%d\n",
                                       lv_status);
        SB_SA_util_assert_ieq(lv_status, 0);
    }
}

void SB_SA_Util_CV::broadcast(bool pv_lock) {
    const char *WHERE = "SB_SA_Util_CV::broadcast";
    int         lv_status;

    if (pv_lock)
        lock();
    lv_status = pthread_cond_broadcast(&iv_cv);
    if (lv_status != 0) {
        SB_SA_Util_Error::error_printf(WHERE, "pthread_cond_broadcast status=%d\n",
                                       lv_status);
        SB_SA_util_assert_ieq(lv_status, 0);
    }
    if (pv_lock)
        unlock();
}

void SB_SA_Util_CV::reset_flag() {
    iv_flag = false;
}

void SB_SA_Util_CV::signal(bool pv_lock) {
    const char *WHERE = "SB_SA_Util_CV::signal";
    int         lv_status;

    if (pv_lock)
        lock();
    iv_flag = true;
    lv_status = pthread_cond_signal(&iv_cv);
    if (lv_status != 0) {
        SB_SA_Util_Error::error_printf(WHERE, "pthread_cond_signal status=%d\n",
                                       lv_status);
        SB_SA_util_assert_ieq(lv_status, 0);
    }
    if (pv_lock)
        unlock();
}

void SB_SA_Util_CV::wait(bool pv_lock) {
    const char *WHERE = "SB_SA_Util_CV::wait";
    int         lv_status;

    if (pv_lock)
        lock();
    if (!iv_flag) {
        lv_status = pthread_cond_wait(&iv_cv, &iv_mutex);
        if (lv_status != 0) {
            SB_SA_Util_Error::error_printf(WHERE, "pthread_cond_wait status=%d\n",
                                           lv_status);
            SB_SA_util_assert_ieq(lv_status, 0);
        }
    }
    iv_flag = false;

    if (pv_lock)
        unlock();
}

// ----------------------------------------------------------------------------

void SB_SA_Util_Debug::debug_printf(const char *pp_where,
                                    const char *pp_format,
                                    ...) {
    va_list lv_ap;

    va_start(lv_ap, pp_format);
    debug_vprintf(pp_where, pp_format, lv_ap);
    va_end(lv_ap);
}

void SB_SA_Util_Debug::debug_vprintf(const char *pp_where,
                                     const char *pp_format,
                                     va_list     pv_ap) {
    char    la_tod[SB_SA_UTIL_MAX_TOD];

    printf("%s %s: tid=%d, ",
           SB_SA_Util_Time::format_time(la_tod), pp_where, sb_sa_util_gettid());
    vprintf(pp_format, pv_ap);
}

// ----------------------------------------------------------------------------

void SB_SA_Util_Error::error_printf(const char *pp_where,
                                    const char *pp_format,
                                    ...) {
    va_list lv_ap;

    va_start(lv_ap, pp_format);
    printf("%s: ERROR tid=%d, ", pp_where, sb_sa_util_gettid());
    vprintf(pp_format, lv_ap);
    va_end(lv_ap);
}

// ----------------------------------------------------------------------------

SB_SA_Util_File::SB_SA_Util_File(const char *pp_file) {
    ip_file = fopen(pp_file, "r");
    iv_line_num = 0;
}

SB_SA_Util_File::~SB_SA_Util_File() {
    if (ip_file != NULL)
        fclose(ip_file);
}

char *SB_SA_Util_File::get_file_line(char *pp_line, int pv_line_len) {
    int lv_len;

    while (fgets(pp_line, pv_line_len, ip_file)) {
        iv_line_num++;
        lv_len = static_cast<int>(strlen(pp_line));
        if ((lv_len > 0) && (pp_line[lv_len - 1] == '\n'))
            pp_line[lv_len - 1] = '\0';
        return pp_line;
    }
    return NULL;
}

// ----------------------------------------------------------------------------

SB_SA_Util_Mutex::SB_SA_Util_Mutex() {
    const char *WHERE = "SB_SA_Util_Mutex::SB_SA_Util_Mutex";
    int         lv_status;

    lv_status = pthread_mutex_init(&iv_mutex, NULL);
    if (lv_status != 0) {
        SB_SA_Util_Error::error_printf(WHERE, "pthread_mutex_init status=%d\n",
                                       lv_status);
        SB_SA_util_assert_ieq(lv_status, 0);
    }
}

SB_SA_Util_Mutex::~SB_SA_Util_Mutex() {
    const char *WHERE = "SB_SA_Util_Mutex::~SB_SA_Util_Mutex";
    int         lv_status;

    lv_status = pthread_mutex_destroy(&iv_mutex);
    if (lv_status != 0) {
        SB_SA_Util_Error::error_printf(WHERE, "pthread_mutex_destroy status=%d\n",
                                       lv_status);
        SB_SA_util_assert_ieq(lv_status, 0);
    }
}

void SB_SA_Util_Mutex::lock() {
    const char *WHERE = "SB_SA_Util_Mutex::lock";
    int         lv_status;

    lv_status = pthread_mutex_lock(&iv_mutex);
    if (lv_status != 0) {
        SB_SA_Util_Error::error_printf(WHERE, "pthread_mutex_lock status=%d\n",
                                       lv_status);
        SB_SA_util_assert_ieq(lv_status, 0);
    }
}

void SB_SA_Util_Mutex::unlock() {
    const char *WHERE = "SB_SA_Util_Mutex::unlock";
    int         lv_status;

    lv_status = pthread_mutex_unlock(&iv_mutex);
    if (lv_status != 0) {
        SB_SA_Util_Error::error_printf(WHERE, "pthread_mutex_unlock status=%d\n",
                                       lv_status);
        SB_SA_util_assert_ieq(lv_status, 0);
    }
}

// ----------------------------------------------------------------------------

SB_SA_Util_Op_Line::SB_SA_Util_Op_Line(SB_SA_Util_File *pp_file)
: ip_file(pp_file) {
}

SB_SA_Util_Op_Line::~SB_SA_Util_Op_Line() {
}

char *SB_SA_Util_Op_Line::deblank(int pv_skip, char *pp_line) {
    pp_line += pv_skip;
    while (isspace(*pp_line))
        pp_line++;
    return pp_line;
}

int SB_SA_Util_Op_Line::get_op_line(char *pa_arg[], int pv_max) {
    const char *WHERE = "SB_SA_Util_Op_Line::get_op_line";
    char        la_line[BUFSIZ];
    char       *lp_line;
    char        lv_char;
    int         lv_inx;

    lp_line = ip_file->get_file_line(la_line, BUFSIZ);
    if (lp_line != NULL)
        lp_line = SB_SA_Util_Op_Line::deblank(0, lp_line);
    if (lp_line == NULL)
        lv_inx = 0;
    else if (*lp_line == '#')
        lv_inx = 0;
    else {
        for (lv_inx = 0; lv_inx < pv_max; lv_inx++) {
            pa_arg[lv_inx] = lp_line;
            while (!isspace(*lp_line) && (*lp_line != ',') && *lp_line)
                lp_line++;
            if (pa_arg[lv_inx] == lp_line) {
                if (*lp_line == ',')
                    pa_arg[lv_inx] = NULL;
                else
                    break;
            }
            lv_char = *lp_line;
            *lp_line = '\0';
            if (!lv_char) {
                lv_inx++;
                break;
            }
            lp_line = deblank(1, lp_line);
        }
        if (lv_inx == 0) {
            SB_SA_Util_Error::error_printf(WHERE,
                                           "input line parse failed to acquire any arguments\n");
            SB_SA_util_assert_igt(lv_inx, 0);
        }
    }
    return lv_inx;
}

// ----------------------------------------------------------------------------

SB_SA_Util_Sem::SB_SA_Util_Sem() : iv_inited(false) {
}

SB_SA_Util_Sem::~SB_SA_Util_Sem() {
    const char *WHERE = "SB_SA_Util_Sem::~SB_SA_Util_Sem";
    int         lv_status;

    if (iv_inited) {
        lv_status = sem_destroy(&iv_sem);
        if (lv_status != 0) {
            SB_SA_Util_Error::error_printf(WHERE, "sem_destroy status=%d\n",
                                           lv_status);
            SB_SA_util_assert_ieq(lv_status, 0);
        }
    }
}

void SB_SA_Util_Sem::init(bool pv_pshared, unsigned int pv_value) {
    const char *WHERE = "SB_SA_Util_Sem::init";
    int         lv_status;

    lv_status = sem_init(&iv_sem, pv_pshared, pv_value);
    if (lv_status != 0) {
        SB_SA_Util_Error::error_printf(WHERE, "sem_init status=%d\n",
                                       lv_status);
        SB_SA_util_assert_ieq(lv_status, 0);
    }
    iv_inited = true;
}

void SB_SA_Util_Sem::post() {
    const char *WHERE = "SB_SA_Util_Sem::post";
    int         lv_status;

    lv_status = sem_post(&iv_sem);
    if (lv_status != 0) {
        SB_SA_Util_Error::error_printf(WHERE, "sem_post status=%d\n",
                                       lv_status);
        SB_SA_util_assert_ieq(lv_status, 0);
    }
}

void SB_SA_Util_Sem::wait() {
    const char *WHERE = "SB_SA_Util_Sem::wait";
    int         lv_status;

    do {
        lv_status = sem_wait(&iv_sem);
    } while (lv_status == EINTR);
    if (lv_status != 0) {
        SB_SA_Util_Error::error_printf(WHERE, "sem_wait status=%d\n",
                                       lv_status);
        SB_SA_util_assert_ieq(lv_status, 0);
    }
}

// ----------------------------------------------------------------------------

static void sb_sa_util_slot_mgr_print_str(FILE *pp_f, char *pp_str) {
    fprintf(pp_f, pp_str);
}

SB_SA_Util_Slot_Mgr::SB_SA_Util_Slot_Mgr(const char *pp_name, Alloc_Type pv_alloc, int pv_cap)
: iv_alloc(pv_alloc), iv_cap(pv_cap), iv_change(false),
  iv_free(pv_cap), iv_head(0), iv_max(-1), iv_size(0), iv_tail(-1) {
    int lv_slot;

    ia_slotmgr_name[sizeof(ia_slotmgr_name) - 1] = '\0';
    SB_SA_util_assert_cpne(pp_name, NULL);
    strncpy(ia_slotmgr_name, pp_name, sizeof(ia_slotmgr_name) - 1);

    SB_SA_util_assert_ige(pv_cap, 1); // sw fault
    ip_slots = new int[pv_cap+1];
    switch (pv_alloc) {
    case ALLOC_FAST:
    case ALLOC_MIN:
        for (lv_slot = 0; lv_slot < pv_cap+1; lv_slot++)
            ip_slots[lv_slot] = lv_slot + 1;
        break;
    case ALLOC_FIFO:
        for (lv_slot = 0; lv_slot < pv_cap+1; lv_slot++)
            ip_slots[lv_slot] = lv_slot + 1;
        iv_tail = pv_cap - 1;
        ip_slots[iv_tail] = SLOT_TAIL;
        break;
    case ALLOC_SCAN:
        for (lv_slot = 0; lv_slot < pv_cap+1; lv_slot++)
            ip_slots[lv_slot] = SLOT_FREE;
        break;
    default:
        SB_SA_util_abort("invalid pv_alloc"); // sw fault
        break;
    }
    ip_slots[pv_cap] = SLOT_TAIL;
}

SB_SA_Util_Slot_Mgr::~SB_SA_Util_Slot_Mgr() {
    delete [] ip_slots;
}

int SB_SA_Util_Slot_Mgr::alloc() {
    int lv_head;
    int lv_slot;

    switch (iv_alloc) {
    case ALLOC_FAST:
    case ALLOC_MIN:
        lv_slot = iv_head;
        lv_head = ip_slots[lv_slot];
        SB_SA_util_assert_ige(lv_head, 0); // sw fault
        iv_head = lv_head;
        ip_slots[lv_slot] = SLOT_USED;
        break;
    case ALLOC_FIFO:
        SB_SA_util_assert_ige(iv_head, 0); // sw fault
        lv_slot = iv_head;
        lv_head = ip_slots[lv_slot];
        if (lv_head == SLOT_TAIL)
            iv_tail = SLOT_TAIL;
        else
            SB_SA_util_assert_ige(lv_head, 0); // sw fault
        iv_head = lv_head;
        ip_slots[lv_slot] = SLOT_USED;
        break;
    case ALLOC_SCAN:
        SB_SA_util_assert_igt(iv_free, 0); // sw fault
        for (lv_slot = 0; lv_slot < iv_cap; lv_slot++) {
            if (ip_slots[lv_slot] == SLOT_FREE) {
                ip_slots[lv_slot] = SLOT_USED;
                break;
            }
        }
        break;
    default:
        lv_slot = 0; // fix compiler warning
        SB_SA_util_abort("invalid iv_alloc"); // sw fault
        break;
    }
    iv_free--;
    iv_size++;
    if (lv_slot > iv_max)
        iv_max = lv_slot;
    return lv_slot;
}

int SB_SA_Util_Slot_Mgr::alloc_if_cap() {
    if (iv_size < iv_cap)
        return SB_SA_Util_Slot_Mgr::alloc();
    else
        return -1;
}

void SB_SA_Util_Slot_Mgr::check_min() {
    SB_SA_util_assert_ieq(iv_alloc, ALLOC_MIN);
    int lv_next;
    int lv_slot;

    for (lv_slot = iv_head; lv_slot != SLOT_TAIL; lv_slot = lv_next) {
        SB_SA_util_assert_ige(lv_slot, 0); // sw fault
        SB_SA_util_assert_ile(lv_slot, iv_cap); // sw fault
        lv_next = ip_slots[lv_slot];
        if (lv_next != SLOT_TAIL)
            SB_SA_util_assert_ilt(lv_slot, lv_next);
    }
}

void SB_SA_Util_Slot_Mgr::free_slot(int pv_slot) {
    SB_SA_util_assert_ige(pv_slot, 0); // sw fault
    SB_SA_util_assert_ilt(pv_slot, iv_cap); // sw fault
    SB_SA_util_assert_ieq(ip_slots[pv_slot], SLOT_USED); // sw fault

    int lv_next;
    int lv_prev;

    switch (iv_alloc) {
    case ALLOC_FAST:
        ip_slots[pv_slot] = iv_head;
        iv_head = pv_slot;
        break;
    case ALLOC_SCAN:
        ip_slots[pv_slot] = SLOT_FREE;
        break;
    case ALLOC_MIN:
        if (pv_slot < iv_head) {
            // slot is lower - make it the head
            ip_slots[pv_slot] = iv_head;
            iv_head = pv_slot;
        } else {
            // slot is higher - place in order
            lv_prev = -1;
            for (lv_next = iv_head;
                 lv_next != SLOT_TAIL;
                 lv_next = ip_slots[lv_next]) {
                SB_SA_util_assert_ige(lv_next, 0); // sw fault
                SB_SA_util_assert_ile(lv_next, iv_cap); // sw fault
                if (pv_slot < lv_next) {
                    ip_slots[pv_slot] = ip_slots[lv_prev];
                    ip_slots[lv_prev] = pv_slot;
                    break;
                }
                lv_prev = lv_next;
            }
            SB_SA_util_assert_ine(lv_next, SLOT_TAIL); // always inserted
        }
        break;
    case ALLOC_FIFO:
        if (iv_tail == SLOT_TAIL) {
            iv_head = pv_slot;
            lv_next = SLOT_TAIL;
        } else {
            lv_next = ip_slots[iv_tail];
            ip_slots[iv_tail] = pv_slot;
        }
        ip_slots[pv_slot] = lv_next;
        iv_tail = pv_slot;
        break;
    default:
        SB_SA_util_abort("invalid iv_alloc"); // sw fault
        break;
    }
    iv_free++;
    iv_size--;
    iv_change = true;
}

int SB_SA_Util_Slot_Mgr::get_cap() {
    return iv_cap;
}

bool SB_SA_Util_Slot_Mgr::inuse(int pv_slot) {
    SB_SA_util_assert_ige(pv_slot, 0); // sw fault
    SB_SA_util_assert_ilt(pv_slot, iv_cap); // sw fault
    return (ip_slots[pv_slot] == SLOT_USED);
}

int SB_SA_Util_Slot_Mgr::max_slot() {
    int lv_slot;

    if (iv_change) {
        iv_change = false;
        for (lv_slot = iv_max; lv_slot >= 0; lv_slot--) {
            iv_max = lv_slot;
            if (ip_slots[lv_slot] == SLOT_USED)
                break;
        }
        if ((iv_max == 0) && (ip_slots[0] == SLOT_FREE))
            iv_max = -1;
    }
    return iv_max;
}

void SB_SA_Util_Slot_Mgr::print(int pv_print_type) {
    print(stdout, pv_print_type);
}

void SB_SA_Util_Slot_Mgr::print(FILE *pp_f, int pv_print_type) {
    print(pp_f, sb_sa_util_slot_mgr_print_str, pv_print_type);
}

void SB_SA_Util_Slot_Mgr::print(FILE *pp_f, Cb_Type pv_cb, int pv_print_type) {
    SB_SA_util_assert_ige(pv_print_type, PRINT_ALL); // sw fault
    SB_SA_util_assert_ile(pv_print_type, PRINT_FREE); // sw fault
    char la_line[200];
    const char *lp_slot_type;
    switch (pv_print_type) {
    case PRINT_ALL:
        lp_slot_type = "ALL";
        break;
    case PRINT_FREE:
        lp_slot_type = "FREE";
        break;
    case PRINT_USED:
        lp_slot_type = "USED";
        break;
    default:
        lp_slot_type = "<unknown>";
        break;
    }
    sprintf(la_line, "name=%s, type=%s, cap=%d, free=%d, used=%d, free-head=%d, free-tail=%d\n",
            ia_slotmgr_name, lp_slot_type, iv_cap, iv_free,
            iv_cap - iv_free, iv_head, iv_tail);
    pv_cb(pp_f, la_line);
    int *lp_slot;
    int lv_slot;
    switch (pv_print_type) {
    case PRINT_ALL:
        for (lv_slot = 0, lp_slot = ip_slots;
             lv_slot < iv_cap;
             lv_slot++, lp_slot++)
            print_slot(pp_f, pv_cb, lv_slot, lp_slot);
        break;
    case PRINT_FREE:
        switch (iv_alloc) {
        case ALLOC_FAST:
        case ALLOC_MIN:
            for (lv_slot = iv_head;;) {
                SB_SA_util_assert_ige(lv_slot, 0); // sw fault
                SB_SA_util_assert_ile(lv_slot, iv_cap); // sw fault
                lp_slot = &ip_slots[lv_slot];
                if (*lp_slot == SLOT_TAIL)
                    break;
                print_slot(pp_f, pv_cb, lv_slot, lp_slot);
                lv_slot = *lp_slot;
            }
            break;
        case ALLOC_FIFO:
            if (iv_head != SLOT_TAIL) {
                for (lv_slot = iv_head;;) {
                    SB_SA_util_assert_ige(lv_slot, 0); // sw fault
                    SB_SA_util_assert_ile(lv_slot, iv_cap); // sw fault
                    lp_slot = &ip_slots[lv_slot];
                    print_slot(pp_f, pv_cb, lv_slot, lp_slot);
                    lv_slot = *lp_slot;
                    if (lv_slot == SLOT_TAIL)
                        break;
                }
            }
            break;
        case ALLOC_SCAN:
            for (lv_slot = 0, lp_slot = ip_slots;
                 lv_slot < iv_cap;
                 lv_slot++, lp_slot++)
                if (*lp_slot == SLOT_FREE)
                    print_slot(pp_f, pv_cb, lv_slot, lp_slot);
            break;
        default:
            SB_SA_util_abort("invalid iv_alloc"); // sw fault
            break;
        }
        break;
    case PRINT_USED:
        for (lv_slot = 0, lp_slot = ip_slots;
             lv_slot < iv_cap;
             lv_slot++, lp_slot++)
            if (*lp_slot == SLOT_USED)
                print_slot(pp_f, pv_cb, lv_slot, lp_slot);
        break;
    }
}

void SB_SA_Util_Slot_Mgr::print_slot(FILE    *pp_f,
                                     Cb_Type  pv_cb,
                                     int      pv_slot,
                                     int     *pp_slot) {
    char la_line[100];

    sprintf(la_line, "slot[%d].free=%d\n", pv_slot, *pp_slot);
    pv_cb(pp_f, la_line);
}

void SB_SA_Util_Slot_Mgr::resize(int pv_cap) {
    int *lp_slots;
    int  lv_slot;

    if (pv_cap > iv_cap) {
        // alloc new, copy, delete, restore, fixup
        lp_slots = new int[pv_cap+1];
        for (lv_slot = 0; lv_slot < iv_cap; lv_slot++)
            lp_slots[lv_slot] = ip_slots[lv_slot];
        delete [] ip_slots;
        ip_slots = lp_slots;
        switch (iv_alloc) {
        case ALLOC_FAST:
        case ALLOC_MIN:
            //
            // cap=4
            // +---+---+---+---+---+
            // | 1 | 2 | 3 | 4 | T |
            // +---+---+---+---+---+
            //   0   1   2   3   4
            //   H               F
            //
            // cap=6
            // +---+---+---+---+---+---+---+
            // | 1 | 2 | 3 | 4 | 5 | 6 | T |
            // +---+---+---+---+---+---+---+
            //   0   1   2   3   4   5   6
            //   H                       F
            //
            for (lv_slot = iv_cap; lv_slot < pv_cap+1; lv_slot++)
                ip_slots[lv_slot] = lv_slot + 1;
            break;
        case ALLOC_FIFO:
            //
            // cap=4
            // +---+---+---+---+---+
            // | 1 | 2 | 3 | T | T |
            // +---+---+---+---+---+
            //   0   1   2   3   4
            //   H           T
            //
            // cap=6
            // +---+---+---+---+---+---+---+
            // | 1 | 2 | 3 | 4 | 5 | T | T |
            // +---+---+---+---+---+---+---+
            //   0   1   2   3   4   5   6
            //   H                   T
            //
            for (lv_slot = iv_cap; lv_slot < pv_cap+1; lv_slot++)
                ip_slots[lv_slot] = lv_slot + 1;
            if (iv_tail == SLOT_TAIL) // no tail
                iv_head = iv_cap;
            else
                ip_slots[iv_tail] = iv_cap; // fix old tail
            iv_tail = pv_cap - 1;
            ip_slots[iv_tail] = SLOT_TAIL;
            break;
        case ALLOC_SCAN:
            //
            // cap=4
            // +---+---+---+---+---+
            // | F | F | F | F | T |
            // +---+---+---+---+---+
            //   0   1   2   3   4
            //   H
            //
            // cap=6
            // +---+---+---+---+---+---+---+
            // | F | F | F | F | F | F | T |
            // +---+---+---+---+---+---+---+
            //   0   1   2   3   4   5   6
            //   H
            //
            for (lv_slot = iv_cap; lv_slot < pv_cap+1; lv_slot++)
                ip_slots[lv_slot] = SLOT_FREE;
            break;
        default:
            SB_SA_util_abort("invalid iv_alloc"); // sw fault
            break;
        }
        ip_slots[pv_cap] = SLOT_TAIL;
        iv_free += (pv_cap - iv_cap);
        iv_cap = pv_cap;
    }
}

int SB_SA_Util_Slot_Mgr::size() {
    return iv_size;
}

// ----------------------------------------------------------------------------

char *SB_SA_Util_Time::format_time(char *pa_tl) {
    struct tm      *lp_tx;
    int             lv_ms;
    struct timeval  lv_t;
    struct tm       lv_tx;
    int             lv_us;

    gettimeofday(&lv_t, NULL);
    lp_tx = localtime_r(&lv_t.tv_sec, &lv_tx);
    lv_ms = static_cast<int>(lv_t.tv_usec) / 1000;
    lv_us = static_cast<int>(lv_t.tv_usec) - lv_ms * 1000;
    sprintf(pa_tl, "%02d:%02d:%02d.%03d.%03d",
            lp_tx->tm_hour, lp_tx->tm_min, lp_tx->tm_sec, lv_ms, lv_us);
    return pa_tl;
}
