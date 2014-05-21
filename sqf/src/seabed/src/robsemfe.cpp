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
// Implement SB_Thread::RUsem
//

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "seabed/int/opts.h"
#include "seabed/log.h"
#include "seabed/ms.h"
#include "seabed/thread.h"

#define USE_MON_ROB_SEM
#ifdef USE_MON_ROB_SEM
// add logging
enum { MON_ROBSEM_1        = 1 };
enum { MON_ROBSEM_2        = 2 };
enum { MON_ROBSEM_3        = 3 };
enum { MON_STRING_BUF_SIZE = 256 };

#include "common/evl_sqlog_eventnum.h"
#include "robsem.cxx"

typedef RobSem Mon_Robust_Sem;
#endif

#include "chk.h"

#define sb_map_robust_sem(pp_sem) \
    reinterpret_cast<Mon_Robust_Sem *>(pp_sem);

// logging
#ifdef USE_MON_ROB_SEM
void Mon_Robust_Sem::log_error(int, int pv_severity, char *pp_buf) {
    int lv_err;

    lv_err = SB_log_write_str(SQEVL_SEABED,
                              SB_EVENT_ID,
                              SQ_LOG_SEAQUEST,
                              pv_severity,
                              pp_buf);
    CHK_ERRIGNORE(lv_err);
}
#endif

SB_Thread::RUsem::RUsem() : ip_sem(NULL) {
}

SB_Thread::RUsem::~RUsem() {
#ifdef USE_MON_ROB_SEM
    int lv_status;

    lv_status = destroy();
    SB_util_assert(lv_status == 0); // sw fault
#endif
}

int SB_Thread::RUsem::check() {
    int lv_status;

    if (ip_sem == NULL)
        lv_status = ENOENT;
    else
        lv_status = 0;
    return lv_status;
}

int SB_Thread::RUsem::destroy() {
#ifdef USE_MON_ROB_SEM
    Mon_Robust_Sem *lp_sem;
    int             lv_status;

    if (ip_sem == NULL)
        lv_status = 0;
    else {
        lp_sem = sb_map_robust_sem(ip_sem);
        lv_status = Mon_Robust_Sem::destroy_sem(lp_sem);
        if (lv_status == 0)
            ip_sem = NULL;
    }
    return lv_status;
#else
    int lv_status = 0;
    return lv_status;
#endif
}

int SB_Thread::RUsem::init(unsigned int pv_appid, unsigned int pv_value) {
#ifdef USE_MON_ROB_SEM
    Mon_Robust_Sem *lp_sem;
    int             lv_err;
    unsigned int    lv_key;
    int             lv_nid;
    int             lv_segid;
    int             lv_status;

    if (ip_sem != NULL) {
        lv_status = EEXIST;
    } else {
        lv_err = msg_mon_get_my_segid(&lv_segid);
        SB_util_assert(lv_err == 0); // sw fault
        lv_err = msg_mon_get_my_info(&lv_nid, // mon node-id
                                     NULL,    // mon process-id
                                     NULL,    // mon name
                                     0,       // mon name-len
                                     NULL,    // mon process-type
                                     NULL,    // mon zone-id
                                     NULL,    // os process-id
                                     NULL);   // os thread-id
        SB_util_assert(lv_err == 0); // sw fault
        lv_key =
          Mon_Robust_Sem::getSegKey(pv_appid,   // applid
                                    lv_segid,   // segid
                                    lv_nid);    // nid
        lv_status =
          Mon_Robust_Sem::create_sem(lv_key,    // shared seg key
                                     0,         // flags
                                     lp_sem,    // sem
                                     pv_value); // value
        if (lv_status == 0)
            ip_sem = lp_sem;
    }

    return lv_status;
#else
    pv_value = pv_value; // touch
    int lv_status = 0;
    return lv_status;
#endif
}

int SB_Thread::RUsem::post() {
#ifdef USE_MON_ROB_SEM
    Mon_Robust_Sem *lp_sem;
    int             lv_status;

    lv_status = check();
    if (lv_status == 0) {
        lp_sem = sb_map_robust_sem(ip_sem);
        lv_status = lp_sem->post();
    }

    return lv_status;
#else
    int lv_status = 0;
    return lv_status;
#endif
}

int SB_Thread::RUsem::trywait() {
#ifdef USE_MON_ROB_SEM
    Mon_Robust_Sem *lp_sem;
    int             lv_status;

    lv_status = check();
    if (lv_status == 0) {
        lp_sem = sb_map_robust_sem(ip_sem);
        lv_status = lp_sem->trywait();
    }

    return lv_status;
#else
    int lv_status = 0;
    return lv_status;
#endif
}

int SB_Thread::RUsem::value(int *pp_val) {
#ifdef USE_MON_ROB_SEM
    Mon_Robust_Sem *lp_sem;
    int             lv_status;

    lv_status = check();
    if (lv_status == 0) {
        lp_sem = sb_map_robust_sem(ip_sem);
        lv_status = lp_sem->value(pp_val);
    }

    return lv_status;
#else
    int lv_status = 0;
    *pp_val = 0;
    return lv_status;
#endif
}

int SB_Thread::RUsem::wait() {
#ifdef USE_MON_ROB_SEM
    Mon_Robust_Sem *lp_sem;
    int             lv_status;

    lv_status = check();
    if (lv_status == 0) {
        lp_sem = sb_map_robust_sem(ip_sem);
        lv_status = lp_sem->wait();
    }

    return lv_status;
#else
    int lv_status = 0;
    return lv_status;
#endif
}

int SB_Thread::RUsem::wait_timed(int pv_sec, int pv_us) {
#ifdef USE_MON_ROB_SEM
    Mon_Robust_Sem *lp_sem;
    int             lv_status;

    lv_status = check();
    if (lv_status == 0) {
        lp_sem = sb_map_robust_sem(ip_sem);
        lv_status = lp_sem->wait_timed(pv_sec, pv_us);
    }

    return lv_status;
#else
    int lv_status = 0;

    pv_sec = pv_sec; // touch
    pv_us = pv_us; // touch
    return lv_status;
#endif
}

