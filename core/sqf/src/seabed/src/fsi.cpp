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

#include "seabed/int/opts.h"

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <sys/time.h>

#include "seabed/fs.h"
#include "seabed/ms.h"
#include "seabed/pevents.h"
#include "seabed/trace.h"

#include "buf.h"
#include "cap.h"
#include "chk.h"
#include "env.h"
#include "fsi.h"
#include "fsx.h"
#include "fstrace.h"
#include "fsutil.h"
#include "labelmapsx.h"
#include "logaggr.h"
#include "msi.h"
#include "msix.h"
#include "msmon.h"
#include "msutil.h"
#include "msx.h"
#include "phan.h"
#include "qid.h"
#include "slotmgr.h"
#include "smap.h"
#include "tablemgr.h"
#include "threadtlsx.h"
#include "timerx.h"
#include "trans.h"
#include "util.h"
#include "utilalloc.h"
#include "utracex.h"

typedef struct Map_Fileno_Entry_Type {
    SB_ML_Type iv_link;
} Map_Fileno_Entry_Type;

//
// open thread.
//
class Fs_Open_Thread : public SB_Thread::Thread {
public:
    static void create(const char *pp_where,
                       MS_Md_Type *pp_md,
                       FS_Fd_Type *pp_fd,
                       const char *pp_filename,
                       bool        pv_self);
    static void destroy(Fs_Open_Thread *pp_thr);
    void        run();
    static void shutdown();

private:
    enum { MAX_THREADS = FS_MAX_CONCUR_NOWAIT_OPENS };

    typedef struct Map_Open_Entry_Type {
        SB_ML_Type      iv_link;
        Fs_Open_Thread *ip_thr;
    } Map_Open_Entry_Type;

    Fs_Open_Thread(const char *pp_name);
    virtual ~Fs_Open_Thread();

    void                     md_list_add(MS_Md_Type *pp_md);
    bool                     md_list_empty();
    MS_Md_Type              *md_list_remove();
    static Fs_Open_Thread   *thread_get(const char *pp_filename,
                                        MS_Md_Type *pp_md);
    bool                     thread_put(const char *pp_filename);

    static SB_Ms_Event_Mgr  *ca_event_mgr[MAX_THREADS];
    static SB_Smap           cv_map;
    static SB_Thread::ECM    cv_map_mutex;
    static SB_Thread::CV     cv_thread_cv;
    static SB_Thread::ECM    cv_thread_get_mutex;
    static SB_Slot_Mgr       cv_thread_inx_mgr;
    static bool              cv_thread_inited;
    const char              *ip_thr_name;
    int                      iv_inx;
    SB_QL_Type               iv_link;
    SB_Sig_D_Queue           iv_md_list;
};

//
// msngr thread.
//
class Fs_Msngr_Thread : public SB_Thread::Thread {
public:
    typedef enum {
        FD_OP_CLOSE   = 1,
        FD_OP_DONE    = 2,
        FD_OP_UNKNOWN = 3
    } Fd_Op_Type;
    Fs_Msngr_Thread();
    ~Fs_Msngr_Thread();

    void         op_add(Fd_Op_Type pv_op, FS_Fd_Type *pp_fd);
    void         op_remove(Fd_Op_Type *pp_op, FS_Fd_Type **ppp_fd);
    void         run();
    void         shutdown();

private:
    typedef struct {
        SB_DQL_Type iv_link;
        Fd_Op_Type  iv_op;
        FS_Fd_Type *ip_fd;
    } Op_Type;
    SB_Sig_D_Queue iv_op_list;
    bool           iv_shutdown;
    bool           iv_started;
};

class Fs_Fd_Table_Entry_Mgr : public SB_Table_Entry_Mgr<FS_Fd_Type> {
public:
    FS_Fd_Type *entry_alloc(size_t pv_inx) {
        FS_Fd_Type *lp_fd;
        pv_inx = pv_inx; // touch
        lp_fd = new FS_Fd_Type;
        lp_fd->iv_closed = false;
        lp_fd->iv_nowait_open = false;
        lp_fd->iv_nowait_open_cancel = false;
        lp_fd->iv_ref_count.set_val(1);
        lp_fd->iv_inuse = true;
        lp_fd->iv_phandle_initialized = false;
        return lp_fd;
    }

    void entry_alloc_block(FS_Fd_Type **ppp_table,
                           size_t       pv_inx,
                           size_t       pv_count) {
        ppp_table = ppp_table; // touch
        pv_inx = pv_inx; // touch
        pv_count = pv_count; // touch
        SB_util_abort("NOT implemented"); // sw fault
    }

    void entry_cap_change(size_t pv_inx, size_t pv_cap) {
        const char *WHERE = "Fs_Fd_Table_Entry_Mgr::entry_cap_change";

        gv_ms_cap.iv_fd_table = pv_cap;
        if (pv_inx) {
            sb_log_aggr_cap_log(WHERE,
                                SB_LOG_AGGR_CAP_FD_TABLE_CAP_INC,
                                static_cast<int>(pv_cap));
            if (gv_fs_trace)
                trace_where_printf(WHERE,
                                   "FD-table capacity increased new-cap=%d\n",
                                   static_cast<int>(pv_cap));
        }
    }

    bool entry_free(FS_Fd_Type *pp_fd, size_t pv_inx, bool pv_urgent) {
        pv_inx = pv_inx; // touch
        if (pv_urgent) {
            delete pp_fd;
            return true;
        } else {
            if (pp_fd->iv_ref_count.read_val() == 0) {
                delete pp_fd;
                return true;
            }
            return false;
        }
    }

    void entry_free_block(FS_Fd_Type **ppp_table,
                          size_t       pv_inx,
                          size_t       pv_count) {
        ppp_table = ppp_table; // touch
        pv_inx = pv_inx; // touch
        pv_count = pv_count; // touch
        SB_util_abort("NOT implemented"); // sw fault
    }

    void entry_inuse(FS_Fd_Type *pp_fd, size_t pv_inx, bool pv_inuse) {
        pp_fd = pp_fd; // touch
        pv_inx = pv_inx; // touch
        pv_inuse = pv_inuse; // touch
        SB_util_abort("NOT implemented"); // sw fault
    }

    void entry_print_str(FS_Fd_Type *pp_fd, size_t pv_inx, char *pp_info) {
        pp_fd = pp_fd; // touch
        pv_inx = pv_inx; // touch
        pp_info = pp_info; // touch
    }
};

// forwards
static void fs_int_lasterr_key_dtor(void *pp_key);
static void fs_int_fd_cleanup_key_dtor(void *pp_map);

// globals
static SB_Phandle_Type        gv_fs_ri_phandle;
const char                   *gp_fs_receive_fname      = "$RECEIVE";
bool                          gv_fs_assert_error       = false;
static int                    gv_fs_lasterr_tls_inx  =
                                SB_create_tls_key(fs_int_lasterr_key_dtor,
                                                  "fs-lasterr");
static bool                   gv_fs_fd_cleanup_enabled;
static int                    gv_fs_fd_tls_inx     =
                                SB_create_tls_key(NULL,
                                                  "fs-fd-cleanup");
static short                  gv_fs_ri_file_number;
static short                  gv_fs_ri_io_type;
#ifdef USE_SB_NEW_RI
static int                    gv_fs_ri_max_reply_count;
#else
static short                  gv_fs_ri_max_reply_count;
#endif
static short                  gv_fs_ri_msg_tag;
static short                  gv_fs_ri_sync_id;
static SB_Uid_Type            gv_fs_ri_userid;
short                         gv_fs_receive_fn         = -1;
static SB_Smap                gv_fs_sender_map("map-fs-sender");
Fs_Fd_Table_Entry_Mgr         gv_fs_filenum_table_entry_mgr;
SB_Table_Mgr<FS_Fd_Type>      gv_fs_filenum_table("tablemgr-FD",
                                                  SB_Table_Mgr_Alloc::ALLOC_FAST,
                                                  SB_Table_Mgr_Alloc::ALLOC_ENTRY_DYN,
                                                  &gv_fs_filenum_table_entry_mgr,
                                                  1024, 128); // cap-init, cap-inc
SB_Thread::ECM                gv_fs_filenum_table_mutex;
const int                     gv_fs_receive_fname_len  = 8;
bool                          gv_fs_shutdown           = false;

static void        fs_int_fd_cleanup_add(int pv_filenum);
static void        fs_int_fd_cleanup_remove(int pv_filenum);
static short       fs_int_fs_file_awaitiox_fs(FS_Fd_Type      *pp_fd,
                                              MS_Md_Type      *pp_md,
                                              short           *pp_filenum,
                                              void           **ppp_buf,
                                              int             *pp_xfercount,
                                              SB_Tag_Type     *pp_tag,
                                              int              pv_timeout,
                                              short           *pp_segid,
                                              SB_Comp_Queue   *pp_fs_comp_q);
static short       fs_int_fs_file_awaitiox_fs_head(FS_Fd_Type     **ppp_fd,
                                                   bool            *pp_complete,
                                                   short           *pp_filenum,
                                                   void           **ppp_buf,
                                                   int             *pp_xfercount,
                                                   SB_Tag_Type     *pp_tag,
                                                   int              pv_timeout,
                                                   short           *pp_segid,
                                                   SB_Comp_Queue   *pp_fs_comp_q);
static short       fs_int_fs_file_awaitiox_fs_list(FS_Fd_Type      *pp_fd,
                                                   bool            *pp_complete,
                                                   short           *pp_filenum,
                                                   void           **ppp_buf,
                                                   int             *pp_xfercount,
                                                   SB_Tag_Type     *pp_tag,
                                                   int              pv_timeout,
                                                   short           *pp_segid,
                                                   SB_Comp_Queue   *pp_fs_comp_q);
static short       fs_int_fs_file_awaitiox_ms(FS_Fd_Type      *pp_fd,
                                              bool            *pp_complete,
                                              short           *pp_filenum,
                                              void           **ppp_buf,
                                              int             *pp_xfercount,
                                              SB_Tag_Type     *pp_tag,
                                              int              pv_timeout,
                                              short           *pp_segid);
static short       fs_int_fs_file_open_ph2(FS_Fd_Type *pp_fd,
                                           const char *pp_filename,
                                           bool        pv_self);

static void        fs_int_fs_format_startid(char  *pp_formatted,
                                            int64  pv_startid);
static void        fs_int_fs_format_tcbref(char     *pp_formatted,
                                           Tcbref_t *pp_tcbref);
static const char *fs_int_fs_get_dialect_type(uint16 pv_dialect_type);
static const char *fs_int_fs_get_req_type(uint16 pv_request_type);

SB_Ms_Event_Mgr   *Fs_Open_Thread::ca_event_mgr[MAX_THREADS];
SB_Smap            Fs_Open_Thread::cv_map("map-fs-open-thread");
SB_Thread::ECM     Fs_Open_Thread::cv_map_mutex;
SB_Thread::CV      Fs_Open_Thread::cv_thread_cv("cv-Fs_Open_Thread::cv_thread_cv");
SB_Thread::ECM     Fs_Open_Thread::cv_thread_get_mutex;
bool               Fs_Open_Thread::cv_thread_inited = false;
SB_Slot_Mgr        Fs_Open_Thread::cv_thread_inx_mgr("slotmgr-fs-open-thread", SB_Slot_Mgr::ALLOC_FAST, MAX_THREADS);

static void *fs_open_thread_fun(void *pp_arg) {
    Fs_Open_Thread *lp_thread = static_cast<Fs_Open_Thread *>(pp_arg);
    lp_thread->run();
    Fs_Open_Thread::destroy(lp_thread);
    return NULL;
}

Fs_Open_Thread::Fs_Open_Thread(const char *pp_name)
: SB_Thread::Thread(fs_open_thread_fun, pp_name),
  iv_md_list(QID_OPEN_THREAD, "q-open-thread", false) {
    int lv_inx;

    ip_thr_name = get_name();
    delete_exit(false);
    set_daemon(true);
    if (!cv_thread_inited) {
        cv_thread_inited = true;
        for (lv_inx = 0; lv_inx < MAX_THREADS; lv_inx++)
            ca_event_mgr[lv_inx] = NULL;
    }
}

Fs_Open_Thread::~Fs_Open_Thread() {
}

void Fs_Open_Thread::create(const char *pp_where,
                            MS_Md_Type *pp_md,
                            FS_Fd_Type *pp_fd,
                            const char *pp_filename,
                            bool        pv_self) {
    Fs_Open_Thread *lp_thr;

    pp_md->ip_where = pp_where;
    pp_md->iv_md_state = pv_self;
    pp_md->ip_stream = pp_fd;
    // inc ref count by two so that when open-thread goes away
    // there will still be a reference to the fd
    fs_int_fd_ref_count_inc(pp_fd, 2);
    lp_thr = thread_get(pp_filename, pp_md);
    lp_thr = lp_thr; // touch
}

void Fs_Open_Thread::destroy(Fs_Open_Thread *pp_thr) {
    const char *WHERE = "Fs_Open_Thread::destroy";

    if (gv_fs_trace)
        trace_where_printf(WHERE, "destroying thread, name=%s\n",
                           pp_thr->get_name());
    delete pp_thr;
}

void Fs_Open_Thread::md_list_add(MS_Md_Type *pp_md) {
    const char *WHERE = "Fs_Open_Thread::md_list_add";

    if (gv_fs_trace)
        trace_where_printf(WHERE, "name=%s, adding msgid=%d\n",
                           ip_thr_name, pp_md->iv_link.iv_id.i);
    iv_md_list.add(&pp_md->iv_link);
}

bool Fs_Open_Thread::md_list_empty() {
    return iv_md_list.empty();
}

MS_Md_Type *Fs_Open_Thread::md_list_remove() {
    const char *WHERE = "Fs_Open_Thread::md_list_remove";
    MS_Md_Type *lp_md;

    if (gv_fs_trace)
        trace_where_printf(WHERE, "name=%s, attempting to get msg\n",
                           ip_thr_name);
    lp_md = static_cast<MS_Md_Type *>(iv_md_list.remove());
    if (gv_fs_trace)
        trace_where_printf(WHERE, "name=%s, got msgid=%d\n",
                           ip_thr_name, lp_md->iv_link.iv_id.i);
    return lp_md;
}

void Fs_Open_Thread::run() {
    const char *WHERE = "Fs_Open_Thread::run";
    FS_Fd_Type *lp_fd;
    MS_Md_Type *lp_md;
    void       *lp_stream;
    bool        lv_empty;
    bool        lv_self;

    // remove from all-list - so that set_event_all doesn't wake us
    gv_ms_event_mgr.remove_from_event_all();

    ca_event_mgr[iv_inx] = gv_ms_event_mgr.get_mgr(NULL);

    for (;;) {
        lp_md = md_list_remove();
        lp_fd = reinterpret_cast<FS_Fd_Type *>(lp_md->ip_stream);
        lv_self = lp_md->iv_md_state;
        if (gv_fs_trace)
            trace_where_printf(WHERE, "name=%s, working on open=%s, msgid=%d\n",
                               ip_thr_name,
                               (lp_fd == NULL) ? NULL : lp_fd->ia_fname,
                               lp_md->iv_link.iv_id.i);
        if (lp_fd == NULL)
            break; // done
        SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_FS_OPEN_THREAD_START,
                           lp_fd->iv_file_num,
                           lp_md->iv_link.iv_id.i);
        lp_md->out.iv_fserr =
          fs_int_fs_file_open_ph2(lp_fd, lp_fd->ia_fname, lv_self);

        // do completion
        lp_stream = ms_od_map_oid_to_stream(lp_fd->iv_oid);
        SB_Trans::Msg_Mgr::set_stream(lp_md, lp_stream, NULL);
        lp_md->iv_break_done = true;
        lp_md->ip_fs_comp_q->add(&lp_md->iv_link);
        // if break returned before signal,
        // this will force the signal to complete
        ca_event_mgr[iv_inx]->set_event(0, NULL);
        SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_FS_OPEN_THREAD_STOP,
                           lp_fd->iv_file_num,
                           lp_md->iv_link.iv_id.i);
        lv_empty = thread_put(lp_fd->ia_fname);
        if (lv_empty)
            fs_int_fd_free(WHERE, lp_fd);
        lp_md->ip_mgr->set_event(LDONE, &lp_md->iv_reply_done);
        if (lv_empty)
            break;
        fs_int_fd_free(WHERE, lp_fd);
    }
    if (gv_fs_trace)
        trace_where_printf(WHERE, "EXITING open thread, name=%s\n",
                           ip_thr_name);
}

void Fs_Open_Thread::shutdown() {
    const char *WHERE = "Fs_Open_Thread::shutdown";
    int         lv_inx;
    int         lv_size;
    int         lv_status;

    gv_fs_shutdown = true;
    lv_status = cv_thread_cv.lock();
    SB_util_assert_ieq(lv_status, 0);
    for (;;) {
        lv_size = cv_thread_inx_mgr.size();
        if (lv_size == 0) {
            if (gv_fs_trace)
                trace_where_printf(WHERE, "open-threads finished\n");
            break;
        }
        if (gv_fs_trace)
            trace_where_printf(WHERE, "waiting for open-thread death size=%d\n",
                               lv_size);
        for (lv_inx = 0; lv_inx < MAX_THREADS; lv_inx++) {
            if (ca_event_mgr[lv_inx] != NULL) {
                if (gv_fs_trace)
                    trace_where_printf(WHERE, "sending LDONE to thr inx=%d\n",
                                       lv_inx);
                ca_event_mgr[lv_inx]->set_event(LDONE, NULL);
            }
        }
        lv_status = cv_thread_cv.wait(false); // already locked
        SB_util_assert_ieq(lv_status, 0);
    }
    lv_status = cv_thread_cv.unlock();
    SB_util_assert_ieq(lv_status, 0);
}

Fs_Open_Thread *Fs_Open_Thread::thread_get(const char *pp_filename,
                                           MS_Md_Type *pp_md) {
    const char          *WHERE = "Fs_Open_Thread::thread_get";
    char                 la_name[20];
    Map_Open_Entry_Type *lp_entry;
    Fs_Open_Thread      *lp_thr;
    long long            lv_delta;
    int                  lv_inx;
    bool                 lv_reuse;
    int                  lv_size;
    int                  lv_status;
    struct timeval       lv_tod_start;
    struct timeval       lv_tod_stop;

    lv_tod_start.tv_sec = 0;
    if (gv_fs_trace)
        trace_where_printf(WHERE, "attempt to get thread for %s\n",
                           pp_filename);

    // serialize, so that multiple threads with same filename get same entry
    lv_status = cv_thread_get_mutex.lock();
    SB_util_assert_ieq(lv_status, 0);

    // reuse thread if filename already being used
    lv_status = cv_map_mutex.lock();
    SB_util_assert_ieq(lv_status, 0);
    lp_entry = static_cast<Map_Open_Entry_Type *>(cv_map.getv(pp_filename));
    if (lp_entry != NULL) {
        lp_thr = lp_entry->ip_thr;
        lp_thr->md_list_add(pp_md);
        lv_reuse = true;
    } else {
        lp_thr = NULL;
        lv_reuse = false;
    }
    lv_status = cv_map_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0);

    if (lp_thr == NULL) {
        lv_status = cv_thread_cv.lock();
        SB_util_assert_ieq(lv_status, 0);
        // create thread - may need to wait
        do {
            lv_size = cv_thread_inx_mgr.size();
            if (lv_size < MAX_THREADS) {
                lv_inx = cv_thread_inx_mgr.alloc();
                sprintf(la_name, "fsopen-%d", lv_inx);
                lp_thr = new Fs_Open_Thread(la_name);
                lp_thr->iv_inx = lv_inx;
                if (gv_fs_trace) {
                    if (lv_tod_start.tv_sec) {
                        gettimeofday(&lv_tod_stop, NULL);
                        lv_delta = (lv_tod_stop.tv_sec * SB_US_PER_SEC +
                                    lv_tod_stop.tv_usec) -
                                   (lv_tod_start.tv_sec * SB_US_PER_SEC +
                                    lv_tod_start.tv_usec);
                        trace_where_printf(WHERE,
                                           "wait time time=%lld us\n",
                                           lv_delta);
                    }
                    trace_where_printf(WHERE, "starting thread, name=%s, size=%d\n",
                                       la_name, lv_size+1);
                }
                lp_thr->start();
            } else {
                // wait until freed-up thread
                if (gv_fs_trace)
                    trace_where_printf(WHERE, "waiting for thread count to drop, size=%d\n",
                                       lv_size);
                if (gv_fs_trace && (lv_tod_start.tv_sec == 0))
                    gettimeofday(&lv_tod_start, NULL);
                lv_status = cv_thread_cv.wait(false); // already locked
                SB_util_assert_ieq(lv_status, 0);
                if (gv_fs_trace)
                    trace_where_printf(WHERE, "signal rcvd check for thread count to drop, size=%d\n",
                                       lv_size);
                lp_thr = NULL;
            }
        } while (lp_thr == NULL);
        lv_status = cv_thread_cv.unlock();
        SB_util_assert_ieq(lv_status, 0);

        // add to thread to map
        lv_status = cv_map_mutex.lock();
        SB_util_assert_ieq(lv_status, 0);
        lp_entry = new Map_Open_Entry_Type;
        lp_entry->ip_thr = lp_thr;
        cv_map.putv(pp_filename, lp_entry);
        // this only want to fix error checked by tools
        if (NULL != lp_thr)
            lp_thr->md_list_add(pp_md);
        lv_status = cv_map_mutex.unlock();
        SB_util_assert_ieq(lv_status, 0);
    }

    if (gv_fs_trace) {
        if (lv_reuse)
            trace_where_printf(WHERE, "name=%s, reuse thread, filename=%s\n",
                    (lp_thr == NULL) ? "NULL" : lp_thr->get_name(), pp_filename);
        else
            trace_where_printf(WHERE, "name=%s, got thread, filename=%s\n",
                    (lp_thr == NULL) ? "NULL" : lp_thr->get_name(), pp_filename);
    }

    lv_status = cv_thread_get_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0);

    return lp_thr;
}

bool Fs_Open_Thread::thread_put(const char *pp_filename) {
    const char          *WHERE = "Fs_Open_Thread::thread_put";
    Map_Open_Entry_Type *lp_entry;
    bool                 lv_empty;
    int                  lv_size;
    int                  lv_status;

    lv_status = cv_map_mutex.lock();
    SB_util_assert_ieq(lv_status, 0);
    lv_empty = md_list_empty();
    if (lv_empty) {
        lp_entry =
          static_cast<Map_Open_Entry_Type *>(cv_map.removev(pp_filename));
        SB_util_assert_pne(lp_entry, NULL);
        delete lp_entry;
    }
    lv_status = cv_map_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0);

    if (gv_fs_trace)
        trace_where_printf(WHERE, "name=%s, put, filename=%s, empty=%d\n",
                           ip_thr_name, pp_filename, lv_empty);
    if (lv_empty) {
        lv_status = cv_thread_cv.lock();
        SB_util_assert_ieq(lv_status, 0);
        if (gv_fs_trace) {
            lv_size = cv_thread_inx_mgr.size();
            trace_where_printf(WHERE, "thread stopping, signal, name=%s, size=%d\n",
                               ip_thr_name, lv_size-1);
        }
        delete ca_event_mgr[iv_inx];
        ca_event_mgr[iv_inx] = NULL;
        cv_thread_inx_mgr.free_slot(iv_inx);
        cv_thread_cv.signal(false);
        lv_status = cv_thread_cv.unlock();
        SB_util_assert_ieq(lv_status, 0);
    }
    return lv_empty;
}

static void *fs_msngr_thread_fun(void *pp_arg) {
    Fs_Msngr_Thread *lp_thread = static_cast<Fs_Msngr_Thread *>(pp_arg);
    lp_thread->run();
    return NULL;
}

Fs_Msngr_Thread::Fs_Msngr_Thread()
: SB_Thread::Thread(fs_msngr_thread_fun, "msngr-thread"),
  iv_op_list(QID_MESSENGER_THREAD, "q-msngr-thread", false),
  iv_shutdown(false), iv_started(false) {
}

Fs_Msngr_Thread::~Fs_Msngr_Thread() {
}

void Fs_Msngr_Thread::op_add(Fd_Op_Type pv_op, FS_Fd_Type *pp_fd) {
    const char *WHERE = "Fs_Msngr_Thread::op_add";
    Op_Type    *lp_op;

    if (!iv_started) {
        iv_op_list.lock();
        if (!iv_started) {
            start();
            iv_started = true;
            SB_Thread::Sthr::yield();
        }
        iv_op_list.unlock();
    }

    lp_op = new Op_Type;
    lp_op->iv_op = pv_op;
    lp_op->ip_fd = pp_fd;
    if (gv_fs_trace)
        trace_where_printf(WHERE, "adding op=%d, fd=%p, fnum=%d\n",
                           pv_op, pfp(pp_fd), pp_fd->iv_file_num);
    iv_op_list.add(&lp_op->iv_link);
}

void Fs_Msngr_Thread::op_remove(Fd_Op_Type *pp_op, FS_Fd_Type **ppp_fd) {
    const char *WHERE = "Fs_Msngr_Thread::op_remove";
    FS_Fd_Type *lp_fd;
    Op_Type    *lp_op;
    Fd_Op_Type  lv_op;

    if (gv_fs_trace)
        trace_where_printf(WHERE, "attempting to get msg\n");
    lp_op = static_cast<Op_Type *>(iv_op_list.remove());
    if (lp_op == NULL) {
        lv_op = FD_OP_UNKNOWN;
        lp_fd = NULL;
    } else {
        lv_op = lp_op->iv_op;
        lp_fd = lp_op->ip_fd;
        delete lp_op;
    }
    if (gv_fs_trace)
        trace_where_printf(WHERE, "got op=%d, fd=%p, fnum=%d\n",
                           lv_op,
                           pfp(lp_fd),
                           (lp_fd == NULL) ? -1 : lp_fd->iv_file_num);
    *pp_op = lv_op;
    *ppp_fd = lp_fd;
}

void Fs_Msngr_Thread::run() {
    const char            *WHERE = "Fs_Msngr_Thread::run";
    FS_Fd_Type            *lp_fd;
    Fd_Op_Type             lv_op;

    // remove from all-list - so that set_event_all doesn't wake us
    gv_ms_event_mgr.remove_from_event_all();

    for (;;) {
        op_remove(&lv_op, &lp_fd);
        if (gv_fs_trace)
            trace_where_printf(WHERE, "working on op=%d, fd=%p, fnum=%d\n",
                               lv_op, pfp(lp_fd), lp_fd->iv_file_num);
        if (lv_op == FD_OP_DONE)
            break;
        if (iv_shutdown) {
            if (gv_fs_trace)
                trace_where_printf(WHERE, "shutdown putting fd\n");
        } else {
            switch (lv_op) {
            case FD_OP_CLOSE:
                SB_util_abort("invalid lv_op(FD_OP_CLOSE)"); // sw fault
                break;

            default:
                SB_util_abort("invalid lv_op"); // sw fault
            }
        }
        fs_int_fd_free(WHERE, lp_fd);
    }
    if (gv_fs_trace)
        trace_where_printf(WHERE, "EXITING msngr thread\n");
}

void Fs_Msngr_Thread::shutdown() {
    const char *WHERE = "Fs_Msngr_Thread::shutdown";
    void       *lp_result;
    int         lv_status;

    if (gv_fs_trace)
        trace_where_printf(WHERE, "shutdown\n");
    if (iv_started) {
        iv_shutdown = true;
        op_add(FD_OP_DONE, NULL);
        lv_status = join(&lp_result);
        SB_util_assert_ieq(lv_status, 0);
    }
}

FS_Fd_Scoped_Mutex::FS_Fd_Scoped_Mutex(const char *pp_where,
                                       FS_Fd_Type *pp_fd)
: ip_fd(pp_fd), ip_where(pp_where), iv_locked(true) {
}

FS_Fd_Scoped_Mutex::~FS_Fd_Scoped_Mutex() {
    unlock();
}

void FS_Fd_Scoped_Mutex::forget() {
    ip_fd = NULL;
}

void FS_Fd_Scoped_Mutex::lock() {
    const char *WHERE = "FS_Fd_Scoped_Mutex::lock";

    if ((ip_fd != NULL) && !iv_locked) {
        fs_int_fd_mutex_lock(WHERE, ip_fd);
        iv_locked = true;
    }
}

void FS_Fd_Scoped_Mutex::unlock() {
    const char *WHERE = "FS_Fd_Scoped_Mutex::unlock";

    if ((ip_fd != NULL) && iv_locked) {
        fs_int_fd_mutex_unlock(WHERE, ip_fd);
        iv_locked = false;
    }
}

#if 0
//
// Purpose: display sender-map
//
static void fs_int_display_sender_map(const char *pp_msg) {
    SB_Smap_Enum *lp_pname_enum = gv_fs_sender_map.keys();
    trace_printf("display sender-map (%s)\n", pp_msg);
    while (lp_pname_enum->more()) {
        char *lp_pname = lp_pname_enum->next();
        SB_Imap *lp_file_map = static_cast<SB_Imap *>(gv_fs_sender_map.getv(lp_pname));
        trace_printf("map-size=%d\n", lp_file_map->size());
        SB_Imap_Enum *lp_filenum_enum = lp_file_map->keys();
        while (lp_filenum_enum->more()) {
            int lv_filenum = lp_filenum_enum->next()->iv_id.i;
            trace_printf("pname=%s, fnum=%d\n", lp_pname, lv_filenum);
        }
        delete lp_filenum_enum;
    }
    delete lp_pname_enum;
}
#endif

//
// Purpose: set lasterr
//
short fs_int_err_lasterr(short pv_fserr) {
    short *lp_lasterr;

    lp_lasterr = fs_int_lasterr_get();
    if ((pv_fserr != XZFIL_ERR_OK) && gv_fs_trace_errors)
        trace_printf("setting lasterr=%d (%p)\n", pv_fserr, pfp(lp_lasterr));
    *lp_lasterr = pv_fserr;
    return pv_fserr;
}

//
// Purpose: record file error and return pv_fserr
//
short fs_int_err_fd_rtn(const char *pp_where,
                        FS_Fd_Type *pp_fd,
                        short       pv_fserr) {
    fs_int_err_lasterr(pv_fserr);
    if (pp_fd != NULL)
        pp_fd->iv_lasterr = pv_fserr;
    return fs_int_err_rtn(pp_where, pv_fserr);
}

//
// Purpose: record file error and return pv_fserr
//
short fs_int_err_fd_rtn_assert(const char *pp_where,
                               FS_Fd_Type *pp_fd,
                               short       pv_fserr,
                               short       pv_fserr_ok) {
    fs_int_err_lasterr(pv_fserr);
    if (pp_fd != NULL)
        pp_fd->iv_lasterr = pv_fserr;
    return fs_int_err_rtn_assert(pp_where, pv_fserr, pv_fserr_ok);
}

//
// Purpose: trace pv_fserr and return pv_fserr
//
short fs_int_err_rtn(const char *pp_where, short pv_fserr) {
    if (pv_fserr != XZFIL_ERR_OK) {
        SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_FS_EXIT, pv_fserr);
        if (gv_fs_trace_errors)
            trace_where_printf(pp_where,
                               "EXIT [failed, ret=%d]\n",
                               pv_fserr);
    }
    return pv_fserr;
}

//
// Purpose: trace pv_fserr and return pv_fserr
//
short fs_int_err_rtn_assert(const char *pp_where,
                            short       pv_fserr,
                            short       pv_fserr_ok) {
    if ((pv_fserr != XZFIL_ERR_OK) && gv_fs_trace_errors)
        trace_where_printf(pp_where,
                           "EXIT [failed, ret=%d]\n",
                           pv_fserr);
    if (gv_fs_assert_error &&
        (pv_fserr != XZFIL_ERR_OK) &&
        (pv_fserr != pv_fserr_ok))
        SB_util_assert_ine(pv_fserr, pv_fserr_ok); // sw fault
    return pv_fserr;
}

//
// Purpose: trace badname and return error
//
short fs_int_err_rtn_badname(const char *pp_where,
                             const char *pp_filename,
                             int         pv_length) {
    if (gv_fs_trace_errors)
        trace_where_printf(pp_where,
                           "EXIT [failed, filename=%s, length=%d (bad name)]\n",
                           pp_filename, pv_length);
    return fs_int_err_rtn(pp_where, XZFIL_ERR_BADNAME);
}

//
// Purpose: trace msg and return error
//
short fs_int_err_rtn_msg(const char *pp_where,
                         const char *pp_msg,
                         short       pv_fserr) {
    if (gv_fs_trace_errors)
        trace_where_printf(pp_where, "%s\n", pp_msg);
    return fs_int_err_rtn(pp_where, pv_fserr);
}

//
// Purpose: trace notopen and return error
//
short fs_int_err_rtn_notopen(const char *pp_where, int pv_filenum) {
    if (gv_fs_trace_errors)
        trace_where_printf(pp_where,
                           "EXIT [invalid fnum=%d]\n", pv_filenum);
    return fs_int_err_rtn(pp_where, XZFIL_ERR_NOTOPEN);
}

//
// Purpose: trace and return error
//
short fs_int_err_rtn_toomany(const char *pp_where) {
    return fs_int_err_rtn(pp_where, XZFIL_ERR_TOOMANY);
}

//
// Purpose: alloc fd
// Locking: fd is locked on return
//
FS_Fd_Type *fs_int_fd_alloc(int pv_recv_depth, int pv_nowait_depth) {
    const char *WHERE = "fs_int_fd_alloc";
    FS_Fd_Type *lp_fd;

    fs_int_fd_table_mutex_lock(WHERE);
    if (gv_fs_trace)
        trace_where_printf(WHERE, "FD-inuse-count=%d\n",
                           static_cast<int>(gv_fs_filenum_table.get_inuse()));
    short lv_filenum = static_cast<short>(gv_fs_filenum_table.alloc_entry());
    lp_fd = gv_fs_filenum_table.get_entry(lv_filenum);

    lp_fd->iv_file_num = lv_filenum;
    if (lp_fd != NULL)
        fs_int_fd_mutex_lock(WHERE, lp_fd);
    fs_int_fd_table_mutex_unlock(WHERE);
    lp_fd->iv_recv_depth = pv_recv_depth;
    lp_fd->iv_nowait_depth = pv_nowait_depth;
    // READX uses ip_ru[0]
    lp_fd->ip_ru = new FS_Ru_Type[pv_recv_depth + 1];
    lp_fd->ip_ru_tag_mgr =
      new SB_Slot_Mgr("slotmgr-fs-ru-tag",
                      SB_Slot_Mgr::ALLOC_MIN,
                      pv_recv_depth + 1);
    // WRITEREADX uses ip_io[0]
    lp_fd->ip_io = new FS_Io_Type[pv_nowait_depth + 1];
    lp_fd->ip_io_old = NULL;
    lp_fd->ip_io_new = NULL;
    lp_fd->ip_io_tag_mgr =
      new SB_Slot_Mgr("slotmgr-fs-io-tag",
                      SB_Slot_Mgr::ALLOC_FIFO,
                      pv_nowait_depth + 1);
    lp_fd->ip_ru_old = NULL;
    lp_fd->ip_ru_new = NULL;
    for (int lv_tag = 0; lv_tag <= pv_recv_depth; lv_tag++) {
        lp_fd->ip_ru[lv_tag].iv_ru_tag = lv_tag;
        lp_fd->ip_ru[lv_tag].iv_inuse = false;
    }
    for (int lv_tag = 0; lv_tag <= pv_nowait_depth; lv_tag++) {
        lp_fd->ip_io[lv_tag].iv_inuse = false;
        lp_fd->ip_io[lv_tag].iv_tag_io = lv_tag;
    }
    lp_fd->iv_lasterr = XZFIL_ERR_OK;
    lp_fd->iv_op_depth = 0;
    lp_fd->iv_transid_supp = false;
    fs_int_fd_cleanup_add(lv_filenum);
    return lp_fd;
}

//
// Purpose: free fd
// Locking: Expects fd to be locked (if ref_count is 0)
//
void fs_int_fd_free(const char *pp_where, FS_Fd_Type *pp_fd) {
    const char *WHERE = "fs_int_fd_free";
    int         lv_filenum;
    int         lv_ref_count;

    pp_fd->iv_closed = true;
    lv_filenum = pp_fd->iv_file_num;
    lv_ref_count = fs_int_fd_ref_count_dec(pp_fd);
    SB_util_assert_ige(lv_ref_count, 0);
    if (gv_fs_trace)
        trace_where_printf(WHERE, "%s, fnum=%d, ref-count=%d\n",
                           pp_where, lv_filenum, lv_ref_count);

    if (lv_ref_count == 0) {
        delete [] pp_fd->ip_io;
        delete [] pp_fd->ip_ru;
        delete pp_fd->ip_io_tag_mgr;
        delete pp_fd->ip_ru_tag_mgr;
        fs_int_fd_cleanup_remove(lv_filenum);
        fs_int_fd_table_mutex_lock(WHERE);
        fs_int_fd_mutex_unlock(WHERE, pp_fd);
        gv_fs_filenum_table.free_entry(lv_filenum);
    }
    if (gv_fs_trace)
        trace_where_printf(WHERE, "FD-inuse-count=%d\n",
                           static_cast<int>(gv_fs_filenum_table.get_inuse()));
    if (lv_ref_count == 0)
        fs_int_fd_table_mutex_unlock(WHERE);
}

//
// Purpose: add fd-cleanup
//
void fs_int_fd_cleanup_add(int pv_filenum) {
    const char *WHERE = "fs_int_fd_cleanup_add";
    SB_ML_Type *lp_entry;
    SB_Ts_Imap *lp_map;

    lp_map =
      static_cast<SB_Ts_Imap *>(SB_Thread::Sthr::specific_get(gv_fs_fd_tls_inx));
    if (lp_map != NULL) {
        if (gv_fs_trace)
            trace_where_printf(WHERE, "add fnum=%d\n", pv_filenum);
        lp_entry = new SB_ML_Type;
        lp_entry->iv_id.i = pv_filenum;
        lp_map->put(lp_entry);
    }
}

//
// Purpose: enable fd-cleanup
//
int fs_int_fd_cleanup_enable() {
    const char *WHERE = "fs_int_fd_cleanup_enable";
    SB_Ts_Imap *lp_map;
    int         lv_status;

    if (!gv_fs_fd_cleanup_enabled) {
        gv_fs_fd_cleanup_enabled = true;
        lp_map = new SB_Ts_Imap();
        if (gv_fs_trace)
            trace_where_printf(WHERE, "key=%d\n", gv_fs_fd_tls_inx);
        lv_status = SB_Thread::Sthr::specific_set(gv_fs_fd_tls_inx, lp_map);
        SB_util_assert_ieq(lv_status, 0);
        gv_ms_event_mgr.get_mgr(NULL)->register_tls_dtor(0,
                                                         fs_int_fd_cleanup_key_dtor,
                                                         lp_map);
    }

    return XZFIL_ERR_OK;
}

//
// Purpose: delete fd-cleanup key
//
void fs_int_fd_cleanup_key_dtor(void *pp_map) {
    const char   *WHERE = "fs_int_fd_cleanup_key_dtor";
    SB_Imap_Enum *lp_enum;
    SB_ML_Type   *lp_entry;
    SB_Ts_Imap   *lp_map;
    short         lv_filenum;
    short         lv_fserr;
    bool          lv_more;

    lp_map = static_cast<SB_Ts_Imap *>(pp_map);
    if (gv_fs_trace)
        trace_where_printf(WHERE, "ENTER map=%p\n", pp_map);
    if (lp_map != NULL) {
        do {
            lp_enum = lp_map->keys();
            lv_more = lp_enum->more();
            if (lv_more) {
                lv_filenum = static_cast<short>(lp_enum->next()->iv_id.i);
                if (gv_fs_trace)
                    trace_where_printf(WHERE, "filenum=%d\n", lv_filenum);
                lp_entry =
                  static_cast<SB_ML_Type *>
                    (lp_map->remove(lv_filenum));
                lv_fserr = BFILE_CLOSE_(lv_filenum, 0);
                CHK_FEIGNORE(lv_fserr);
                delete lp_entry;
            }
            delete lp_enum;
        } while (lv_more);
        delete lp_map;
    }
    if (gv_fs_trace)
        trace_where_printf(WHERE, "EXIT\n");
}

//
// Purpose: remove fd cleanup
//
void fs_int_fd_cleanup_remove(int pv_filenum) {
    const char *WHERE = "fs_int_fd_cleanup_remove";
    SB_ML_Type *lp_entry;
    SB_Ts_Imap *lp_map;

    lp_map =
      static_cast<SB_Ts_Imap *>(SB_Thread::Sthr::specific_get(gv_fs_fd_tls_inx));
    if (lp_map != NULL) {
        lp_entry =
          static_cast<SB_ML_Type *>(lp_map->remove(pv_filenum));
        if (lp_entry != NULL) {
            if (gv_fs_trace)
                trace_where_printf(WHERE, "remove fnum=%d\n", pv_filenum);
            delete lp_entry;
        }
    }
}

int fs_int_fd_ref_count_dec(FS_Fd_Type *pp_fd) {
    int lv_ref_count;

    lv_ref_count = pp_fd->iv_ref_count.sub_and_fetch(1);
    SB_util_assert_ige(lv_ref_count, 0);
    return lv_ref_count;
}

int fs_int_fd_ref_count_inc(FS_Fd_Type *pp_fd, int pv_inc) {
    int lv_ref_count;

    lv_ref_count = pp_fd->iv_ref_count.add_and_fetch(pv_inc);
    return lv_ref_count;
}

//
// Purpose: init fds - allocate filenum=0
//
void fs_int_fd_init() {
    FS_Fd_Type *lp_fd;
    size_t      lv_filenum;
    int         lv_max_fds;

    lv_filenum = gv_fs_filenum_table.alloc_entry();
    lp_fd = gv_fs_filenum_table.get_entry(lv_filenum);
    lp_fd->iv_inuse = false;
    lp_fd->iv_file_num = static_cast<short>(lv_filenum);
    lp_fd->ip_io = NULL;
    lp_fd->ip_ru = NULL;
    lp_fd->ip_io_tag_mgr = NULL;
    lp_fd->ip_ru_tag_mgr = NULL;
    lv_max_fds = 0;
    ms_getenv_int(gp_fs_env_max_cap_fds, &lv_max_fds);
    if (lv_max_fds > 0)
        gv_fs_filenum_table.set_cap_max(lv_max_fds);
}

//
// Purpose: map filenum to fd
// Locking: fd is locked on return (if pv_lock)
//
FS_Fd_Type *fs_int_fd_map_filenum_to_fd(int pv_filenum, bool pv_lock) {
    const char *WHERE = "fs_int_fd_map_filenum_to_fd";
    FS_Fd_Type *lp_fd;

    fs_int_fd_table_mutex_lock(WHERE);
    lp_fd = gv_fs_filenum_table.get_entry(pv_filenum);
    if ((lp_fd != NULL) && (!lp_fd->iv_inuse))
        lp_fd = NULL;
    if (pv_lock && (lp_fd != NULL))
        fs_int_fd_mutex_lock(WHERE, lp_fd);
    fs_int_fd_table_mutex_unlock(WHERE);
    return lp_fd;
}

void fs_int_fd_mutex_lock(const char *pp_where, FS_Fd_Type *pp_fd) {
    int lv_status;

    if (gv_fs_trace_mt)
        trace_where_printf(pp_where, "fd lock start fd=%p, fnum=%d\n",
                           pfp(pp_fd), pp_fd->iv_file_num);
    lv_status = pp_fd->iv_mutex.lock();
    SB_util_assert_ieq(lv_status, 0);
    if (gv_fs_trace_mt)
        trace_where_printf(pp_where, "fd lock fin fd=%p, fnum=%d\n",
                           pfp(pp_fd), pp_fd->iv_file_num);
}

void fs_int_fd_mutex_unlock(const char *pp_where, FS_Fd_Type *pp_fd) {
    int lv_status;

    if (gv_fs_trace_mt)
        trace_where_printf(pp_where, "fd unlock fd=%p, fnum=%d\n",
                           pfp(pp_fd), pp_fd->iv_file_num);
    lv_status = pp_fd->iv_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0);
}

void fs_int_fd_nowait_open_mutex_lock(const char *pp_where, FS_Fd_Type *pp_fd) {
    int lv_status;

    if (gv_fs_trace_mt)
        trace_where_printf(pp_where, "fd-nowait-open lock start fd=%p, fnum=%d\n",
                           pfp(pp_fd), pp_fd->iv_file_num);
    lv_status = pp_fd->iv_nowait_open_mutex.lock();
    SB_util_assert_ieq(lv_status, 0);
    if (gv_fs_trace_mt)
        trace_where_printf(pp_where, "fd-nowait-open lock fin fd=%p, fnum=%d\n",
                           pfp(pp_fd), pp_fd->iv_file_num);
}

void fs_int_fd_nowait_open_mutex_unlock(const char *pp_where,
                                        FS_Fd_Type *pp_fd) {
    int lv_status;

    if (gv_fs_trace_mt)
        trace_where_printf(pp_where, "fd-nowait-open unlock fd=%p, fnum=%d\n",
                           pfp(pp_fd), pp_fd->iv_file_num);
    lv_status = pp_fd->iv_nowait_open_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0);
}

//
// Purpose: change op depth
//
static void fs_int_fd_op_change_depth(FS_Fd_Type *pp_fd, int pv_amount) {
    pp_fd->iv_op_depth += pv_amount;
}

//
// Purpose: check op depth
//
static short fs_int_fd_op_check(const char *pp_where, FS_Fd_Type *pp_fd) {
    if (pp_fd->iv_nowait_depth > 0) {
        if (pp_fd->iv_op_depth >= pp_fd->iv_nowait_depth)
            return fs_int_err_rtn_toomany(pp_where);
    }
    return XZFIL_ERR_OK;
}

//
// Purpose: setup fd
//
short fs_int_fd_setup(const char *pp_where,
                      const char *pp_filename,
                      int         pv_length,
                      int         pv_nowait_depth,
                      int         pv_options,
                      FS_Fd_Type *pp_fd) {
    int         lv_file_type;

    if (pp_filename == NULL) {
        lv_file_type = FS_FILE_TYPE_UNKNOWN;
        pp_filename = "<NULL>";
    } else if (pv_length < 2)
        lv_file_type = FS_FILE_TYPE_UNKNOWN;
    else if ((pv_length == gv_fs_receive_fname_len) &&
             (memcmp(pp_filename,
                     gp_fs_receive_fname,
                     gv_fs_receive_fname_len) == 0)) {
        lv_file_type = FS_FILE_TYPE_RECEIVE;
    } else if ((pv_length >= 1) && (pp_filename[0] == '$')) {
        // for now, assume process
        lv_file_type = FS_FILE_TYPE_PROCESS;
    } else
        lv_file_type = FS_FILE_TYPE_UNKNOWN;
    if (lv_file_type == FS_FILE_TYPE_UNKNOWN)
        return fs_int_err_rtn_badname(pp_where, pp_filename, pv_length);
    if (pv_length > (FS_MAX_FNAME - 1))
        return fs_int_err_rtn_msg(pp_where,
                                  "filename length too large",
                                  XZFIL_ERR_BOUNDSERR);
    switch (lv_file_type) {
    case FS_FILE_TYPE_RECEIVE:
        if (pv_nowait_depth > 1)
            return fs_int_err_rtn_msg(pp_where,
                                      "nowait-depth too large",
                                      XZFIL_ERR_BOUNDSERR);
        break;
    }
    memcpy(pp_fd->ia_fname, pp_filename, pv_length);
    pp_fd->ia_fname[pv_length] = '\0';
    pp_fd->iv_file_type = lv_file_type;
    pp_fd->iv_nowait_open = (pv_options & FS_OPEN_OPTIONS_NOWAIT);
    pp_fd->iv_options = pv_options;
    return XZFIL_ERR_OK;
}

void fs_int_fd_table_mutex_lock(const char *pp_where) {
    int lv_status;

    lv_status = gv_fs_filenum_table_mutex.lock();
    SB_util_assert_ieq(lv_status, 0);
    if (gv_fs_trace_mt)
        trace_where_printf(pp_where, "filenum-table locked\n");
}

void fs_int_fd_table_mutex_unlock(const char *pp_where) {
    int lv_status;

    if (gv_fs_trace_mt)
        trace_where_printf(pp_where, "filenum-table unlocked\n");
    lv_status = gv_fs_filenum_table_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0);
}

static void fs_int_fs_file_return_mem(MS_Md_Type *pp_md) {
    MS_BUF_MGR_FREE(pp_md->iv_ss.ip_req_ctrl);
    MS_BUF_MGR_FREE(pp_md->out.ip_reply_ctrl);
    SB_Trans::Msg_Mgr::put_md_link(pp_md, "fs done - return-mem");
}

//
// file-awaitiox over ms
//
short fs_int_fs_file_awaitiox(short           *pp_filenum,
                              void           **ppp_buf,
                              int             *pp_xfercount,
                              SB_Tag_Type     *pp_tag,
                              int              pv_timeout,
                              short           *pp_segid,
                              bool             pv_int,
                              bool             pv_ts) {
    const char    *WHERE = "XAWAITIOX(internal)";
    SB_Comp_Queue *lp_fs_comp_q;
    short          lv_fserr;
    SB_Int64_Type  lv_start_time;

    if (pv_timeout == 0)
        lv_start_time = 0;
    else
        lv_start_time = SB_Thread::Sthr::time();
    lp_fs_comp_q = ms_fsdone_get_comp_q(pv_ts);
    if (*pp_filenum < 0) {
        //
        // due to subtle differences, all filenum check is subtly different
        // from specific filenum check
        //
        FS_Fd_Type *lp_fd = NULL; // make compiler happy
        long lv_to = pv_timeout > 0 ? pv_timeout * 10 : pv_timeout; // ms
        bool lv_do_wait = false;
        static bool lv_check_sel = 0;
        // check queues before doing wait in case completion already
        for (;;) {
            if (lv_do_wait) {
                short lv_wait;
                if (*pp_filenum == -1)
                    lv_wait = LREQ | LDONE;
                else {
                    // like -1, but forget about $RECEIVE
                    lv_wait = LDONE;
                    lv_check_sel = 0;
                }
                int lv_wait_to;
                if (pv_timeout < 0)
                    lv_wait_to = -1;
                else if (pv_timeout == 0)
                    lv_wait_to = -2;
                else
                    lv_wait_to = static_cast<int>(lv_to) / 10; // in tics
                lv_fserr = XWAIT(lv_wait, lv_wait_to);
                CHK_WAITIGNORE(lv_fserr);
            }
            bool lv_complete;
            // alternate between lreq/ldone completions
            for (int lv_inx = 0; lv_inx < 2; lv_inx++) {
                switch (lv_check_sel) {
                case 0:
                    if (*pp_filenum == -1)
                        lv_check_sel = 1; // if -1, then check $RECEIVE
                    lv_fserr = fs_int_fs_file_awaitiox_fs_head(&lp_fd,
                                                               &lv_complete,
                                                               pp_filenum,
                                                               ppp_buf,
                                                               pp_xfercount,
                                                               pp_tag,
                                                               pv_timeout,
                                                               pp_segid,
                                                               lp_fs_comp_q);
                    if (lv_complete) {
                        *pp_filenum = lp_fd->iv_file_num;
                        return fs_int_err_fd_rtn_assert(WHERE,
                                                        lp_fd,
                                                        lv_fserr,
                                                        0);
                    }
                    break;

                case 1:
                    lv_check_sel = 0;
                    lp_fd = fs_int_fd_map_filenum_to_fd(gv_fs_receive_fn, false);
                    if ((lp_fd != NULL) &&
                        (!gv_ms_recv_q.empty() ||
                         !sb_timer_comp_q_empty())) {
                        lv_fserr = fs_int_fs_file_awaitiox_ms(lp_fd,
                                                              &lv_complete,
                                                              pp_filenum,
                                                              ppp_buf,
                                                              pp_xfercount,
                                                              pp_tag,
                                                              pv_timeout,
                                                              pp_segid);
                        if (lv_complete) {
                            *pp_filenum = lp_fd->iv_file_num;
                            return fs_int_err_fd_rtn(WHERE, lp_fd, lv_fserr);
                        }
                    }
                }
            }
            if (lv_do_wait || (pv_timeout == 0)) {
                if (pv_timeout) {
                    SB_Int64_Type lv_curr_time = SB_Thread::Sthr::time();
                    long lv_elapsed = static_cast<long>(lv_curr_time - lv_start_time);
                    lv_to -= lv_elapsed;
                }
                if ((pv_timeout == 0) ||
                    ((pv_timeout > 0) && (lv_to <= 0)))
                    return fs_int_err_fd_rtn_assert(WHERE,
                                                    lp_fd,
                                                    XZFIL_ERR_TIMEDOUT,
                                                    XZFIL_ERR_TIMEDOUT);
            } else
                lv_do_wait = true;
        }
    } else {
        //
        // due to subtle differences, all filenum check is subtly different
        // from specific filenum check
        //
        FS_Fd_Type *lp_fd = fs_int_fd_map_filenum_to_fd(*pp_filenum, false);
        if (lp_fd == NULL)
            return fs_int_err_rtn_notopen(WHERE, *pp_filenum);
        SB_util_assert_ieq(lp_fd->iv_file_num, *pp_filenum); // sw fault
        if (!lp_fd->iv_nowait_open) {
            if (!pv_int && (lp_fd->iv_nowait_depth <= 0))
                return fs_int_err_fd_rtn(WHERE, lp_fd, XZFIL_ERR_WAITFILE);
            if (lp_fd->iv_op_depth <= 0)
                return fs_int_err_fd_rtn(WHERE, lp_fd, XZFIL_ERR_NONEOUT);
        }
        long lv_to = pv_timeout > 0 ? pv_timeout * 10 : pv_timeout; // ms
        bool lv_do_wait = false;
        // check queues before doing wait in case completion already
        for (;;) {
            if (lv_do_wait) {
                short lv_wait = 0;
                if (lp_fd->iv_file_type == FS_FILE_TYPE_RECEIVE)
                    lv_wait |= LREQ;
                else
                    lv_wait |= LDONE;
                int lv_wait_to;
                if (pv_timeout < 0)
                    lv_wait_to = -1;
                else if (pv_timeout == 0)
                    lv_wait_to = -2;
                else
                    lv_wait_to = static_cast<int>(lv_to) / 10; // in tics
                lv_fserr = XWAIT(lv_wait, lv_wait_to);
                CHK_WAITIGNORE(lv_fserr);
            }
            bool lv_complete;
            if (lp_fd->iv_file_type != FS_FILE_TYPE_RECEIVE) {
                lv_fserr = fs_int_fs_file_awaitiox_fs_list(lp_fd,
                                                           &lv_complete,
                                                           pp_filenum,
                                                           ppp_buf,
                                                           pp_xfercount,
                                                           pp_tag,
                                                           pv_timeout,
                                                           pp_segid,
                                                           lp_fs_comp_q);
                if (lv_complete)
                    return fs_int_err_fd_rtn_assert(WHERE,
                                                    lp_fd,
                                                    lv_fserr,
                                                    0);
            } else {
                if (!gv_ms_recv_q.empty() || !sb_timer_comp_q_empty()) {
                    lv_fserr = fs_int_fs_file_awaitiox_ms(lp_fd,
                                                          &lv_complete,
                                                          pp_filenum,
                                                          ppp_buf,
                                                          pp_xfercount,
                                                          pp_tag,
                                                          pv_timeout,
                                                          pp_segid);
                    if (lv_complete)
                        return fs_int_err_fd_rtn(WHERE, lp_fd, lv_fserr);
                }
                // check for cancel
                if (lp_fd->iv_op_depth <= 0)
                    return fs_int_err_fd_rtn(WHERE, lp_fd, XZFIL_ERR_NONEOUT);
            }
            if (lv_do_wait || (pv_timeout == 0)) {
                if (pv_timeout) {
                    SB_Int64_Type lv_curr_time = SB_Thread::Sthr::time();
                    long lv_elapsed = static_cast<long>(lv_curr_time - lv_start_time);
                    if (pv_timeout > 0)
                        lv_to -= lv_elapsed;
                }
                if ((pv_timeout == 0) ||
                    ((pv_timeout > 0) && (lv_to <= 0)))
                    return fs_int_err_fd_rtn_assert(WHERE,
                                                    lp_fd,
                                                    XZFIL_ERR_TIMEDOUT,
                                                    XZFIL_ERR_TIMEDOUT);
            } else
                lv_do_wait = true;
        }
    }
}

//
// file-awaitiox over fs
//
short fs_int_fs_file_awaitiox_fs(FS_Fd_Type    *pp_fd,
                                 MS_Md_Type    *pp_md,
                                 short         *pp_filenum,
                                 void         **ppp_buf,
                                 int           *pp_xfercount,
                                 SB_Tag_Type   *pp_tag,
                                 int            pv_timeout,
                                 short         *pp_segid,
                                 SB_Comp_Queue *pp_fs_comp_q) {
    const char     *WHERE = "XAWAITIOX(internal-fs)";
    MS_Md_Type     *lp_md;
    short           lv_fserr;
    short           lv_fserr_temp;
    MS_Result_Type  lv_results;

    lp_md = pp_md; // in case break NULLs
    pp_filenum = pp_filenum; // no-warn
    pv_timeout = pv_timeout; // no-warn
    if (lp_md->iv_break_done) {
        // normally the break would do this
        pp_fs_comp_q->remove_list(&lp_md->iv_link);
        // nowait-open completion
        fs_int_fd_ref_count_dec(pp_fd); // need additional dec
        pp_fd->iv_nowait_open = false;
        lv_fserr = lp_md->out.iv_fserr;
        if (gv_fs_trace_params)
            trace_where_printf(WHERE, "from=%s, OPEN reply. fserr=%d\n",
                               pp_fd->ia_fname,
                               lv_fserr);
        SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_FS_AWAITIOX_COMP_OPEN,
                           lp_md->iv_link.iv_id.i,
                           lv_fserr);
        // record last error
        if (lv_fserr != XZFIL_ERR_OK)
            fs_int_err_fd_rtn_assert(WHERE,
                                     pp_fd,
                                     lv_fserr,
                                     XZFIL_ERR_TIMEDOUT);
        SB_Trans::Msg_Mgr::put_md(lp_md->iv_link.iv_id.i,
                                  "fs OPEN done - return-mem");
        if (pp_xfercount != NULL)
            *pp_xfercount = 0;
        if (pp_tag != NULL)
            *pp_tag = -30;
        if (pp_segid != NULL)
            *pp_segid = 0;
        return lv_fserr;
    }
    lv_fserr = xmsg_break_com(lp_md->iv_link.iv_id.i, // msgid
                              reinterpret_cast<short *>(&lv_results),  // results
                              &pp_fd->iv_phandle,     // phandle
                              &lp_md,
                              false);
    SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_FS_AWAITIOX_COMP,
                       lp_md->iv_link.iv_id.i,
                       pp_fd->iv_file_num);
    // this works for both WRITE and WRITEREAD
    fs_fs_writeread_reply *lp_wr_ctrl_rep =
      reinterpret_cast<fs_fs_writeread_reply *>(lp_md->out.ip_reply_ctrl);
    // record last error
    if (lv_fserr != XZFIL_ERR_OK) {
        lp_wr_ctrl_rep = NULL;
        fs_int_err_fd_rtn_assert(WHERE,
                                 pp_fd,
                                 lv_fserr,
                                 XZFIL_ERR_TIMEDOUT);
    } else if (lp_wr_ctrl_rep->error != XZFIL_ERR_OK)
        lv_fserr = fs_int_err_fd_rtn_assert(WHERE,
                                            pp_fd,
                                            lp_wr_ctrl_rep->error,
                                            XZFIL_ERR_TIMEDOUT);
    fs_int_fd_op_change_depth(pp_fd, -1);
    FS_Io_Type *lp_io = reinterpret_cast<FS_Io_Type *>(lp_md->iv_tag);
    SB_Transid_Type lv_transid;
    SB_Transseq_Type lv_startid;
    TRANSID_COPY(lv_transid, lp_io->iv_transid);
    TRANSSEQ_COPY(lv_startid, lp_io->iv_startid);
    if (TRANSID_IS_VALID(lv_transid)) {
        // check reinstate (switch error, if not already set)
        lv_fserr_temp = static_cast<short>(ms_transid_reinstate(lv_transid, lv_startid));
        if ((lv_fserr == XZFIL_ERR_OK) && (lv_fserr_temp != XZFIL_ERR_OK))
            lv_fserr = lv_fserr_temp;
    }
    if (ppp_buf != NULL)
        *ppp_buf = lp_io->ip_buffer;
    SB_Tag_Type lv_tag = lp_io->iv_tag_user;
    FS_util_io_tag_free(pp_fd, lp_io);
    if (gv_fs_trace) {
        if (lp_wr_ctrl_rep != NULL) {
            const char *lp_reply_type;
            if (lp_wr_ctrl_rep->reply_type == FS_FS_WRITE_REPLY)
                lp_reply_type = "W";
            else
                lp_reply_type = "WR";
            trace_where_printf(WHERE, "%s reply. dtype=%d(%s), rtype=%d(%s), rvers=%d, fserr=%d, cw=%d\n",
                               lp_reply_type,
                               lp_wr_ctrl_rep->dialect_type,
                               fs_int_fs_get_dialect_type(lp_wr_ctrl_rep->dialect_type),
                               lp_wr_ctrl_rep->reply_type,
                               fs_int_fs_get_req_type(lp_wr_ctrl_rep->reply_type),
                               lp_wr_ctrl_rep->reply_version,
                               lp_wr_ctrl_rep->error,
                               lp_wr_ctrl_rep->countwritten);
        } else {
            trace_where_printf(WHERE, "UKNOWN (no control) reply. fserr=%d\n",
                               lv_fserr);
        }
    }
    if (gv_fs_trace_params) {
        if (lp_wr_ctrl_rep != NULL) {
            const char *lp_reply_type;
            int lv_xfercount;
            if (lp_wr_ctrl_rep->reply_type == FS_FS_WRITE_REPLY) {
                lp_reply_type = "WRITE";
                lv_xfercount = lp_wr_ctrl_rep->countwritten;
            } else {
                lp_reply_type = "WRITEREAD";
                lv_xfercount = lv_results.rr_datasize;
            }
            trace_where_printf(WHERE, "from=%s, %s reply. fserr=%d, xc=%d\n",
                               pp_fd->ia_fname,
                               lp_reply_type,
                               lp_wr_ctrl_rep->error,
                               lv_xfercount);
        } else {
            trace_where_printf(WHERE, "from=%s, UNKNOWN (no control) reply. fserr=%d\n",
                               pp_fd->ia_fname,
                               lv_fserr);
        }
    }
    if (pp_xfercount != NULL) {
        if (lp_wr_ctrl_rep == NULL)
            *pp_xfercount = 0;
        else if (lp_wr_ctrl_rep->reply_type == FS_FS_WRITE_REPLY)
            *pp_xfercount = lp_wr_ctrl_rep->countwritten;
        else
            *pp_xfercount = lv_results.rr_datasize;
    }
    // now that lp_wr_ctrl_rep references are complete, return memory
    fs_int_fs_file_return_mem(lp_md);
    if (pp_tag != NULL)
        *pp_tag = lv_tag;
    if (pp_segid != NULL)
        *pp_segid = 0;
    return lv_fserr;
}

//
// file-awaitiox over fs
//
short fs_int_fs_file_awaitiox_fs_head(FS_Fd_Type   **ppp_fd,
                                      bool          *pp_complete,
                                      short         *pp_filenum,
                                      void         **ppp_buf,
                                      int           *pp_xfercount,
                                      SB_Tag_Type   *pp_tag,
                                      int            pv_timeout,
                                      short         *pp_segid,
                                      SB_Comp_Queue *pp_fs_comp_q) {
    *pp_complete = false;
    MS_Md_Type *lp_md = reinterpret_cast<MS_Md_Type *>(pp_fs_comp_q->remove());
    FS_Fd_Type *lp_fd = NULL;
    if (lp_md != NULL) {
        FS_Io_Type *lp_io = reinterpret_cast<FS_Io_Type *>(lp_md->iv_tag);
        lp_fd = static_cast<FS_Fd_Type *>(lp_io->ip_fd);
        *pp_complete = true;
        short lv_fserr = fs_int_fs_file_awaitiox_fs(lp_fd,
                                                    lp_md,
                                                    pp_filenum,
                                                    ppp_buf,
                                                    pp_xfercount,
                                                    pp_tag,
                                                    pv_timeout,
                                                    pp_segid,
                                                    pp_fs_comp_q);
        *ppp_fd = lp_fd;
        return lv_fserr;
    }
    return XZFIL_ERR_OK;
}

//
// file-awaitiox over fs
//
short fs_int_fs_file_awaitiox_fs_list(FS_Fd_Type      *pp_fd,
                                      bool            *pp_complete,
                                      short           *pp_filenum,
                                      void           **ppp_buf,
                                      int             *pp_xfercount,
                                      SB_Tag_Type     *pp_tag,
                                      int              pv_timeout,
                                      short           *pp_segid,
                                      SB_Comp_Queue   *pp_fs_comp_q) {
    const char *WHERE = "fs_int_fs_file_awaitiox_fs_list";

    *pp_complete = false;
    pp_fs_comp_q->lock();
    MS_Md_Type *lp_md = reinterpret_cast<MS_Md_Type *>(pp_fs_comp_q->head());
    FS_Fd_Type *lp_fd;
    while (lp_md != NULL) {
        FS_Io_Type *lp_io = reinterpret_cast<FS_Io_Type *>(lp_md->iv_tag);
        lp_fd = static_cast<FS_Fd_Type *>(lp_io->ip_fd);
        if (gv_fs_trace_verbose)
            trace_where_printf(WHERE, "msgid=%d, md=%p, fnum=%d\n",
                               lp_md->iv_link.iv_id.i, pfp(lp_md),
                               lp_fd->iv_file_num);
        if (lp_fd == pp_fd)
            break;
        lp_md = reinterpret_cast<MS_Md_Type *>(lp_md->iv_link.ip_next);
    }
    if ((lp_md != NULL) && (lp_fd == pp_fd))
        pp_fs_comp_q->remove_list_lock(&lp_md->iv_link, false);
    pp_fs_comp_q->unlock();
    if ((lp_md != NULL) && (lp_fd == pp_fd)) {
        *pp_complete = true;
        short lv_fserr = fs_int_fs_file_awaitiox_fs(pp_fd,
                                                    lp_md,
                                                    pp_filenum,
                                                    ppp_buf,
                                                    pp_xfercount,
                                                    pp_tag,
                                                    pv_timeout,
                                                    pp_segid,
                                                    pp_fs_comp_q);
        return lv_fserr;
    }
    return XZFIL_ERR_OK;
}

//
// file-awaitiox over ms
//
short fs_int_fs_file_awaitiox_ms(FS_Fd_Type      *pp_fd,
                                 bool            *pp_complete,
                                 short           *pp_filenum,
                                 void           **ppp_buf,
                                 int             *pp_xfercount,
                                 SB_Tag_Type     *pp_tag,
                                 int              pv_timeout,
                                 short           *pp_segid) {
    const char            *WHERE = "XAWAITIOX(internal-ms)";
    static char            la_open_ctrl_req_buf[sizeof(fs_fs_open)];
    static char            la_open_ctrl_rep_buf[sizeof(fs_fs_open_reply)];
    SB_Phandle_Type        lv_phandle;
    fs_fs_simple_reply    *lp_close_ctrl_rep;
    fs_fs_close           *lp_close_ctrl_req;
    SB_Imap_Enum          *lp_enum;
    Map_Fileno_Entry_Type *lp_entry;
    SB_Imap               *lp_file_map;
    fs_fs_open            *lp_open_ctrl_req;
    fs_fs_open_reply      *lp_open_ctrl_rep;
    SB_Phandle_Type       *lp_phandle;
    char                  *lp_pname;
    FS_Ru_Type            *lp_ru;
    xzsys_ddl_smsg_def    *lp_sys_msg;
    fs_fs_writeread       *lp_wr_ctrl_req;
    int                    lv_filenum;
    short                  lv_fserr;
    int                    lv_rc;
    BMS_SRE                lv_sre;
    BMS_SRE_TPOP           lv_sre_tpop;
    xzsys_ddl_smsg_def     lv_sys_msg;
    int                    lv_sys_msg_size;
    short                  lv_sre_type;

    pp_filenum = pp_filenum; // no-warn
    pv_timeout = pv_timeout; // no-warn
    // only listen to IREQ/TPOP
    lv_sre_type = BMSG_LISTEN_(reinterpret_cast<short *>(&lv_sre),
                               BLISTEN_ALLOW_IREQM | BLISTEN_ALLOW_TPOPM,
                               0);
    if (lv_sre_type == BSRETYPE_NOWORK) { // no work
        *pp_complete = false;
        return XZFIL_ERR_OK;
    }
    lp_ru = &pp_fd->ip_ru[pp_fd->iv_ru_curr_tag];
    if (ppp_buf != NULL)
        *ppp_buf = lp_ru->ip_buffer;
    lp_ru->iv_msgid = lv_sre.sre_msgId;
    lp_ru->iv_count_written = sbmin(lp_ru->iv_read_count, lv_sre.sre_reqDataSize);
    bool lv_is_sys_msg = false;
    lp_open_ctrl_req = reinterpret_cast<fs_fs_open *>(la_open_ctrl_req_buf);
    const char *lp_req_type = NULL;
    if (lv_sre_type == BSRETYPE_TPOP) {
        lv_is_sys_msg = true;
        lv_sys_msg_size = sizeof(lv_sys_msg.u_z_msg.z_timesignal);
        memcpy(&lv_sre_tpop, &lv_sre, sizeof(lv_sre_tpop));
        lv_sys_msg.u_z_msg.z_timesignal.z_msgnumber = XZSYS_VAL_SMSG_TIMESIGNAL;
        lv_sys_msg.u_z_msg.z_timesignal.z_parm1 = lv_sre_tpop.sre_tleParm1;
        lv_sys_msg.u_z_msg.z_timesignal.z_parm2 = lv_sre_tpop.sre_tleParm2;
        SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_FS_COMP_TPOP,
                           lv_sre.sre_msgId,
                           lv_sre_tpop.sre_tleParm1);
        if (gv_fs_trace || gv_fs_trace_params)
            trace_where_printf(WHERE,
                               "TIMER pop parm1=%d, parm2=%ld\n",
                               lv_sre_tpop.sre_tleParm1,
                               lv_sre_tpop.sre_tleParm2);

        gv_fs_ri_io_type = 0; // system messages get 0
        gv_fs_ri_file_number = static_cast<short>(-1);
        gv_fs_ri_sync_id = 0; // TODO?
        gv_fs_ri_userid = 0;
        memset(&gv_fs_ri_phandle, 0, sizeof(SB_Phandle));
        lv_rc = sbmin(lp_ru->iv_read_count, lv_sys_msg_size);
        if (lp_ru->ip_buffer != NULL)
            memcpy(lp_ru->ip_buffer, &lv_sys_msg, lv_rc);
        else {
            SB_util_assert_pne(ppp_buf, NULL); // MUST supply
            lp_sys_msg =
              static_cast<xzsys_ddl_smsg_def *>
                (MS_BUF_MGR_ALLOC(sizeof(xzsys_ddl_smsg_def)));
            *ppp_buf = lp_sys_msg;
            memcpy(lp_sys_msg, &lv_sys_msg, lv_sys_msg_size);
        }
        MS_Md_Type *lp_md;
        SB_Trans::Msg_Mgr::get_md(&lp_md, // fs_int_fs_file_awaitiox_ms
                                  NULL,
                                  NULL,
                                  false,  // recv
                                  NULL,   // fserr
                                  WHERE,
                                  MD_STATE_MSG_TIMER);
        SB_util_assert_pne(lp_md, NULL); // TODO?
        lp_md->out.iv_mon_msg = true;
        lp_md->out.iv_recv_req_id = 0;
        lp_ru->iv_msgid = lp_md->iv_msgid;
        lp_ru->iv_count_written = sbmin(lp_ru->iv_read_count, lv_sys_msg_size);
    } else if (lv_sre.sre_flags & XSRE_MON) {
        Mon_Msg_Type *lp_mon_msg;
        lv_rc = lv_sre.sre_reqDataSize;
        lv_fserr = msg_buf_read_data(lv_sre.sre_msgId,
                                     reinterpret_cast<char **>(&lp_mon_msg),
                                     &lv_rc,
                                     false);
        SB_util_assert_ieq(lv_fserr, XZFIL_ERR_OK); // sw fault
        const char *lp_msg_type = msg_util_get_msg_type(lp_mon_msg->type);
        if (gv_fs_trace || gv_fs_trace_mon) {
            trace_where_printf(WHERE,
                               "Received mon message, type=%s (%d)\n",
                               lp_msg_type,
                               lp_mon_msg->type);
        }
        // reply if no system message OR monitor open message
        if ((pp_fd->iv_options & FS_OPEN_OPTIONS_NO_SYS_MSGS) ||
            (lp_mon_msg->type == MsgType_Open)) {
            if (gv_fs_trace || gv_fs_trace_mon) {
                if (lp_mon_msg->type == MsgType_Open)
                    trace_where_printf(WHERE,
                                       "Received OPEN message - replying - mon-open\n");
                else
                    trace_where_printf(WHERE,
                                       "Received OPEN message - replying - no sys-msgs\n");
            }
            if (lp_mon_msg->type == MsgType_Open) {
                SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_FS_COMP_REPLY_OPEN,
                                   lv_sre.sre_msgId);
            } else {
                SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_FS_COMP_REPLY_NOSM,
                                   lv_sre.sre_msgId,
                                   lp_mon_msg->type);
            }
            BMSG_REPLY_(lv_sre.sre_msgId,            // msgid
                        NULL,                        // replyctrl
                        0,                           // replyctrlsize
                        NULL,                        // replydata
                        0,                           // replydatasize
                        0,                           // errorclass
                        NULL);                       // newphandle
            *pp_complete = false;
            return XZFIL_ERR_OK;
        }
        lv_is_sys_msg = true;
        lv_sys_msg_size = sizeof(lv_sys_msg);
        switch (lp_mon_msg->type) {
        case MsgType_Change:    // registry change notification
            lp_req_type = "change";
            lv_sys_msg_size = sizeof(lv_sys_msg.u_z_msg.z_change);
            lv_sys_msg.u_z_msg.z_change.z_msgnumber = XZSYS_VAL_SMSG_CHANGE;
            lv_sys_msg.u_z_msg.z_change.z_grouptype =
              static_cast<short>(lp_mon_msg->u.request.u.change.type);
            memcpy(lv_sys_msg.u_z_msg.z_change.z_groupname,
                   lp_mon_msg->u.request.u.change.group, MS_MON_MAX_KEY_NAME);
            memcpy(lv_sys_msg.u_z_msg.z_change.z_keyname,
                   lp_mon_msg->u.request.u.change.key, MS_MON_MAX_KEY_NAME);
            memcpy(lv_sys_msg.u_z_msg.z_change.z_value,
                   lp_mon_msg->u.request.u.change.value, MS_MON_MAX_VALUE_SIZE);
            lv_filenum = -1;
            lp_phandle = NULL;
            SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_FS_COMP_SM_CHANGE,
                               lv_sre.sre_msgId);
            break;

        case MsgType_Close:    // process close notification
            lp_req_type = "close";
#if 0
            fs_int_display_sender_map("monitor close msg rcvd");
#endif
            lp_pname = lp_mon_msg->u.request.u.close.process_name;
            lp_file_map = static_cast<SB_Imap *>(gv_fs_sender_map.getv(lp_pname));
            lv_filenum = -1;

            //
            // if there are no known opens, just reply (don't tell user)
            //
            if ((lp_file_map == NULL) || (lp_file_map->size() == 0)) {
                if (gv_fs_trace || gv_fs_trace_mon)
                    trace_where_printf(WHERE,
                                       "Received CLOSE message - replying - no known opens\n");
                SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_FS_COMP_REPLY_CLOSE_NOOPEN,
                                   lv_sre.sre_msgId,
                                   lp_mon_msg->type);
                BMSG_REPLY_(lv_sre.sre_msgId,            // msgid
                            NULL,                        // replyctrl
                            0,                           // replyctrlsize
                            NULL,                        // replydata
                            0,                           // replydatasize
                            0,                           // errorclass
                            NULL);                       // newphandle
                *pp_complete = false;
                return XZFIL_ERR_OK;
            } else if (lp_file_map->size() > 1)
                ms_msg_set_requeue(WHERE, lv_sre.sre_msgId, true);
            lp_enum = lp_file_map->keys();
            if (lp_enum->more()) {
                lv_filenum = lp_enum->next()->iv_id.i;
                lp_entry =
                  static_cast<Map_Fileno_Entry_Type *>
                    (lp_file_map->remove(lv_filenum));
                delete lp_entry;
            }
            delete lp_enum;
            lv_sys_msg_size = sizeof(lv_sys_msg.u_z_msg.z_close);
            lv_sys_msg.u_z_msg.z_close.z_msgnumber = XZSYS_VAL_SMSG_CLOSE;
            lv_sys_msg.u_z_msg.z_close.z_tapedisposition = 0;
            ms_util_fill_phandle_name(&lv_phandle,
                                      lp_mon_msg->u.request.u.close.process_name,
                                      lp_mon_msg->u.request.u.close.nid,
                                      lp_mon_msg->u.request.u.close.pid
#ifdef SQ_PHANDLE_VERIFIER
                                     ,lp_mon_msg->u.request.u.close.verifier
#endif
                                     );
            lp_phandle = &lv_phandle;
            SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_FS_COMP_SM_CLOSE,
                               lv_sre.sre_msgId,
                               lv_filenum);
            break;

        case MsgType_NodeDown: // node down notification
            lp_req_type = "node-down";
            lv_sys_msg_size = sizeof(lv_sys_msg.u_z_msg.z_cpudown);
            lv_sys_msg.u_z_msg.z_cpudown.z_msgnumber = XZSYS_VAL_SMSG_CPUDOWN;
            lv_sys_msg.u_z_msg.z_cpudown.z_cpunumber =
              static_cast<short>(lp_mon_msg->u.request.u.down.nid);
            lv_sys_msg.u_z_msg.z_cpudown.z_takeover =
              lp_mon_msg->u.request.u.down.takeover;
            lv_filenum = -1;
            lp_phandle = NULL;
            SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_FS_COMP_SM_CPUDOWN,
                               lv_sre.sre_msgId);
            break;

        case MsgType_NodeUp: // node up notification
            lp_req_type = "node-up";
            lv_sys_msg_size = sizeof(lv_sys_msg.u_z_msg.z_cpuup);
            lv_sys_msg.u_z_msg.z_cpuup.z_msgnumber = XZSYS_VAL_SMSG_CPUUP;
            lv_sys_msg.u_z_msg.z_cpuup.z_cpunumber =
              static_cast<short>(lp_mon_msg->u.request.u.up.nid);
            lv_sys_msg.u_z_msg.z_cpuup.z_takeover =
              lp_mon_msg->u.request.u.up.takeover;
            lv_filenum = -1;
            lp_phandle = NULL;
            SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_FS_COMP_SM_CPUUP,
                               lv_sre.sre_msgId);
            break;

        case MsgType_ProcessDeath: // process death notification
            lp_req_type = "process-death";
            lv_sys_msg_size = sizeof(lv_sys_msg.u_z_msg.z_procdeath);
            lv_sys_msg.u_z_msg.z_procdeath.u_z_msgnumber.z_msgnumber =
              XZSYS_VAL_SMSG_PROCDEATH;
            ms_util_fill_phandle_name(&lv_phandle,
                                      lp_mon_msg->u.request.u.death.process_name,
                                      lp_mon_msg->u.request.u.death.nid,
                                      lp_mon_msg->u.request.u.death.pid
#ifdef SQ_PHANDLE_VERIFIER
                                     ,lp_mon_msg->u.request.u.death.verifier
#endif
                                     );
            lp_phandle = &lv_phandle;
            memcpy(&lv_sys_msg.u_z_msg.z_procdeath.z_phandle,
                   lp_phandle,
                   sizeof(SB_Phandle)
                   );
            lv_sys_msg.u_z_msg.z_procdeath.z_cputime = 0;
            lv_sys_msg.u_z_msg.z_procdeath.z_jobid = 0;
            lv_sys_msg.u_z_msg.z_procdeath.z_completion_code = 0; // TODO
            lv_sys_msg.u_z_msg.z_procdeath.u_z_termination_code.z_termination_code = 0; // TODO
            memset(&lv_sys_msg.u_z_msg.z_procdeath.z_subsystem, 0,
                  sizeof(lv_sys_msg.u_z_msg.z_procdeath.z_subsystem));
            memset(&lv_sys_msg.u_z_msg.z_procdeath.z_killer, -1,
                  sizeof(lv_sys_msg.u_z_msg.z_procdeath.z_killer));
            lv_sys_msg.u_z_msg.z_procdeath.z_termtext_len = 0;
            lv_sys_msg.u_z_msg.z_procdeath.z_procname.zoffset = 0;
            lv_sys_msg.u_z_msg.z_procdeath.z_procname.zlen = 0;
            lv_sys_msg.u_z_msg.z_procdeath.z_flags = 0;
            lv_sys_msg.u_z_msg.z_procdeath.z_osspid = 0;
            lv_sys_msg.u_z_msg.z_procdeath.z_reserved = 0;
            memset(&lv_sys_msg.u_z_msg.z_procdeath.u_z_data.z_termtext, 0,
                  sizeof(lv_sys_msg.u_z_msg.z_procdeath.u_z_data.z_termtext));
            lv_filenum = -1;
            SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_FS_COMP_SM_PROCDEATH,
                               lv_sre.sre_msgId);
            break;

        case MsgType_Shutdown:  // cluster shutdown notification
            lp_req_type = "shut-down";
            lv_sys_msg_size = sizeof(lv_sys_msg.u_z_msg.z_shutdown);
            lv_sys_msg.u_z_msg.z_shutdown.z_msgnumber = XZSYS_VAL_SMSG_SHUTDOWN;
            lv_sys_msg.u_z_msg.z_shutdown.z_shutdownlevel
                = static_cast<short>(lp_mon_msg->u.request.u.shutdown.level);
            lv_filenum = -1;
            lp_phandle = NULL;
            SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_FS_COMP_SM_SHUTDOWN,
                               lv_sre.sre_msgId);
            break;

        default:
            lv_sys_msg.u_z_msg.z_msgnumber[0] = XZSYS_VAL_SMSG_UNKNOWN;
            lv_filenum = -1;
            lp_phandle = NULL;
            SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_FS_COMP_SM_UNKNOWN,
                               lv_sre.sre_msgId);
            break;
        }
        gv_fs_ri_io_type = 0; // system messages get 0
        gv_fs_ri_file_number = static_cast<short>(lv_filenum);
        gv_fs_ri_sync_id = 0; // TODO?
        gv_fs_ri_userid = 0;
        if (lp_phandle != NULL)
            memcpy(&gv_fs_ri_phandle, lp_phandle, sizeof(SB_Phandle));
        else
            memset(&gv_fs_ri_phandle, 0, sizeof(SB_Phandle));
        lv_rc = sbmin(lp_ru->iv_read_count, lv_sys_msg_size);
        if (lp_ru->ip_buffer != NULL)
            memcpy(lp_ru->ip_buffer, &lv_sys_msg, lv_rc);
        else {
            SB_util_assert_pne(ppp_buf, NULL); // MUST supply
            *ppp_buf = lp_mon_msg;
        }
    } else {
        lp_open_ctrl_rep = reinterpret_cast<fs_fs_open_reply *>(la_open_ctrl_rep_buf);

#if 0
        // dirty reply
        memset(lp_open_ctrl_rep, -1, sizeof(fs_fs_open_reply));
#endif
        lv_rc =
          sbmin(static_cast<short>(sizeof(la_open_ctrl_req_buf)), lv_sre.sre_reqCtrlSize);
        lv_fserr = msg_buf_read_ctrl(lv_sre.sre_msgId,
                                     reinterpret_cast<short **>(&lp_open_ctrl_req),
                                     &lv_rc,
                                     false);
        SB_util_assert_ieq(lv_fserr, XZFIL_ERR_OK); // sw fault
        if (gv_fs_trace)
            trace_where_printf(WHERE,
                               "Received message, dtype=%d(%s), rtype=%d(%s), rvers=%d, minvers=%d\n",
                               lp_open_ctrl_req->dialect_type,
                               fs_int_fs_get_dialect_type(lp_open_ctrl_req->dialect_type),
                               lp_open_ctrl_req->request_type,
                               fs_int_fs_get_req_type(lp_open_ctrl_req->request_type),
                               lp_open_ctrl_req->request_version,
                               lp_open_ctrl_req->minimum_interpretation_version);
        switch (lp_open_ctrl_req->request_type) {
        case FS_FS_OPEN:
            lp_req_type = "open";
            // update sender-map
            lp_pname =
               ms_od_map_phandle_to_name(reinterpret_cast<SB_Phandle_Type *>(&lp_open_ctrl_req->sender.phandle));
            lp_file_map = static_cast<SB_Imap *>(gv_fs_sender_map.getv(lp_pname));
            if (lp_file_map == NULL) {
                lp_file_map = new SB_Imap("map-fs-file");
                gv_fs_sender_map.putv(lp_pname, lp_file_map);
            }
            lp_entry = new Map_Fileno_Entry_Type;
            lp_entry->iv_link.iv_id.i =
              lp_open_ctrl_req->sender.first_word.filenum;
            lp_file_map->put(&lp_entry->iv_link);
#if 0
            fs_int_display_sender_map("open rcvd, filenum added");
#endif

            if (pp_fd->iv_options & FS_OPEN_OPTIONS_NO_SYS_MSGS) {
                if (gv_fs_trace || gv_fs_trace_params)
                    trace_where_printf(WHERE,
                                       "Received OPEN message - replying - no sys-msgs\n");
                lp_open_ctrl_rep->dialect_type = DIALECT_FS_FS;
                lp_open_ctrl_rep->reply_type = FS_FS_OPEN_REPLY;
                lp_open_ctrl_rep->reply_version = CURRENT_VERSION_FS_FS;
                lp_open_ctrl_rep->error = XZFIL_ERR_OK;
                if (gv_fs_trace)
                    trace_where_printf(WHERE, "OPEN reply, dtype=%d(%s), rtype=%d(%s), rvers=%d, fserr=%d\n",
                                       lp_open_ctrl_rep->dialect_type,
                                       fs_int_fs_get_dialect_type(lp_open_ctrl_rep->dialect_type),
                                       lp_open_ctrl_rep->reply_type,
                                       fs_int_fs_get_req_type(lp_open_ctrl_rep->reply_type),
                                       lp_open_ctrl_rep->reply_version,
                                       lp_open_ctrl_rep->error);
                SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_FS_COMP_REPLY_OPEN_NOSM,
                                   lv_sre.sre_msgId);
                BMSG_REPLY_(lv_sre.sre_msgId,            // msgid
                            reinterpret_cast<short *>(lp_open_ctrl_rep),  // replyctrl
                            sizeof(fs_fs_open_reply),    // replyctrlsize
                            NULL,                        // replydata
                            0,                           // replydatasize
                            0,                           // errorclass
                            NULL);                       // newphandle
                *pp_complete = false;
                return XZFIL_ERR_OK;
            }
            if (gv_fs_trace) {
                SB_Buf_Line la_sender;
                fs_int_fs_format_sender(la_sender,
                                        reinterpret_cast<short *>(&lp_open_ctrl_req->sender.phandle));
                trace_where_printf(WHERE, "Received OPEN message, sender=%s, file=%d\n",
                                   la_sender,
                                   lp_open_ctrl_req->sender.first_word.filenum);
            }
            lv_is_sys_msg = true;
            lv_rc = static_cast<int>(sbmin(static_cast<size_t>(lp_ru->iv_read_count),
                                         sizeof(xzsys_ddl_smsg_def)));
            if (lp_ru->ip_buffer != NULL)
                lp_sys_msg = &lv_sys_msg;
            else {
                SB_util_assert_pne(ppp_buf, NULL); // MUST supply
                lp_sys_msg = reinterpret_cast<xzsys_ddl_smsg_def *>(lp_open_ctrl_req);
            }
            lp_sys_msg->u_z_msg.z_open.u_z_msgnumber.z_msgnumber =
              XZSYS_VAL_SMSG_OPEN;
            lp_sys_msg->u_z_msg.z_open.z_accessmode =
              lp_open_ctrl_req->access;
            lp_sys_msg->u_z_msg.z_open.z_exclusionmode =
              lp_open_ctrl_req->exclusion;
            lp_sys_msg->u_z_msg.z_open.z_nowait =
              lp_open_ctrl_req->nowait;
            lp_sys_msg->u_z_msg.z_open.z_syncdepth =
              lp_open_ctrl_req->syncdepth;
            lp_sys_msg->u_z_msg.z_open.z_options =
              lp_open_ctrl_req->options.flags.initialize0;
            lp_sys_msg->u_z_msg.z_open.z_paid = 0; // TODO
            lp_sys_msg->u_z_msg.z_open.z_flags = 0; // TODO
            memset(&lp_sys_msg->u_z_msg.z_open.z_primary, -1, // TODO
                   sizeof(lp_sys_msg->u_z_msg.z_open.z_primary));
            lp_sys_msg->u_z_msg.z_open.z_qualifier_len = 0; // TODO
            lp_sys_msg->u_z_msg.z_open.z_opener_name.zoffset = 0; // TODO
            lp_sys_msg->u_z_msg.z_open.z_opener_name.zlen = 0; // TODO
            lp_sys_msg->u_z_msg.z_open.z_primary_fnum = 0; // TODO
            lp_sys_msg->u_z_msg.z_open.z_craid = 0; // TODO
            lp_sys_msg->u_z_msg.z_open.z_hometerm_name.zoffset = 0; // TODO
            lp_sys_msg->u_z_msg.z_open.z_hometerm_name.zlen = 0; // TODO
            memset(&lp_sys_msg->u_z_msg.z_open.z_reserved, 0, // TODO
                   sizeof(lp_sys_msg->u_z_msg.z_open.z_reserved));
            lp_sys_msg->u_z_msg.z_open.z_qualifier_len = 0; // TODO
            if (lp_ru->ip_buffer != NULL)
                memcpy(lp_ru->ip_buffer, &lv_sys_msg, lv_rc);
            else
                *ppp_buf = lp_sys_msg;
            gv_fs_ri_io_type = 0; // system messages get 0
            gv_fs_ri_file_number = lp_open_ctrl_req->sender.first_word.filenum;
            gv_fs_ri_sync_id = static_cast<short>(lp_open_ctrl_req->sender.syncid);
            gv_fs_ri_userid = 0;
            memcpy(&gv_fs_ri_phandle,
                   &lp_open_ctrl_req->sender.phandle,
                   sizeof(SB_Phandle));
            SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_FS_COMP_SM_OPEN,
                               lv_sre.sre_msgId,
                               lp_open_ctrl_req->sender.first_word.filenum);
            break;

        case FS_FS_CLOSE:
            lp_req_type = "close";
            // update sender-map
            lp_close_ctrl_req = reinterpret_cast<fs_fs_close *>(lp_open_ctrl_req);
            lp_pname =
               ms_od_map_phandle_to_name(reinterpret_cast<SB_Phandle_Type *>(&lp_close_ctrl_req->sender.phandle));
            lp_file_map = static_cast<SB_Imap *>(gv_fs_sender_map.getv(lp_pname));
            if (lp_file_map != NULL) {
                lp_entry =
                  static_cast<Map_Fileno_Entry_Type *>
                    (lp_file_map->remove(lp_close_ctrl_req->sender.first_word.filenum));
                delete lp_entry;
            }
#if 0
            fs_int_display_sender_map("close rcvd, filenum remove attempted");
#endif
            if (pp_fd->iv_options & FS_OPEN_OPTIONS_NO_SYS_MSGS) {
                if (gv_fs_trace || gv_fs_trace_params)
                    trace_where_printf(WHERE,
                                       "Received CLOSE message - replying - no sys-msgs\n");
                lp_close_ctrl_rep = reinterpret_cast<fs_fs_simple_reply *>(lp_open_ctrl_rep);
                lp_close_ctrl_rep->error = XZFIL_ERR_OK;
                if (gv_fs_trace)
                    trace_where_printf(WHERE, "CLOSE reply, dtype=%d(%s), rtype=%d(%s), rvers=%d, fserr=%d\n",
                                       lp_close_ctrl_rep->dialect_type,
                                       fs_int_fs_get_dialect_type(lp_close_ctrl_rep->dialect_type),
                                       lp_close_ctrl_rep->reply_type,
                                       fs_int_fs_get_req_type(lp_close_ctrl_rep->reply_type),
                                       lp_close_ctrl_rep->reply_version,
                                       lp_close_ctrl_rep->error);
                SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_FS_COMP_REPLY_CLOSE_NOSM,
                                   lv_sre.sre_msgId);
                BMSG_REPLY_(lv_sre.sre_msgId,            // msgid
                            reinterpret_cast<short *>(lp_close_ctrl_rep), // replyctrl
                            sizeof(fs_fs_open_reply),    // replyctrlsize
                            NULL,                        // replydata
                            0,                           // replydatasize
                            0,                           // errorclass
                            NULL);                       // newphandle
                *pp_complete = false;
                return XZFIL_ERR_OK;
            }
            lp_close_ctrl_req = reinterpret_cast<fs_fs_close *>(lp_open_ctrl_req);
            if (gv_fs_trace) {
                SB_Buf_Line la_sender;
                fs_int_fs_format_sender(la_sender,
                                        reinterpret_cast<short *>(&lp_close_ctrl_req->sender.phandle));
                trace_where_printf(WHERE, "Received CLOSE message, sender=%s, file=%d\n",
                                   la_sender,
                                   lp_close_ctrl_req->sender.first_word.filenum);
            }
            lv_is_sys_msg = true;
            lv_rc = static_cast<int>(sbmin(static_cast<size_t>(lp_ru->iv_read_count),
                                         sizeof(xzsys_ddl_smsg_close_def)));
            if (lp_ru->ip_buffer != NULL)
                lp_sys_msg = &lv_sys_msg;
            else {
                SB_util_assert_pne(ppp_buf, NULL); // MUST supply
                lp_sys_msg = reinterpret_cast<xzsys_ddl_smsg_def *>(lp_close_ctrl_req);
                *ppp_buf = lp_sys_msg;
            }
            lp_sys_msg->u_z_msg.z_close.z_msgnumber = XZSYS_VAL_SMSG_CLOSE;
            if (lp_ru->ip_buffer != NULL)
                memcpy(lp_ru->ip_buffer, &lv_sys_msg, lv_rc);
            else
                *ppp_buf = lp_sys_msg;
            gv_fs_ri_io_type = 0; // system messages get 0
            gv_fs_ri_file_number = lp_close_ctrl_req->sender.first_word.filenum;
            gv_fs_ri_sync_id = static_cast<short>(lp_close_ctrl_req->sender.syncid);
            gv_fs_ri_userid = 0;
            memcpy(&gv_fs_ri_phandle,
                   &lp_close_ctrl_req->sender.phandle,
                   sizeof(SB_Phandle));
            SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_FS_COMP_SM_CLOSE,
                               lv_sre.sre_msgId,
                               lp_close_ctrl_req->sender.first_word.filenum);
            break;
        }
    }
    *pp_complete = true;
    fs_int_fd_op_change_depth(pp_fd, -1);
    // save ri info
    if (!lv_is_sys_msg) {
        lp_wr_ctrl_req = reinterpret_cast<fs_fs_writeread *>(lp_open_ctrl_req);
        lp_ru->iv_io_type = lp_wr_ctrl_req->request_type;
        switch (lp_wr_ctrl_req->request_type) {
        case FS_FS_WRITE:
            lp_req_type = "write";
            if (gv_fs_trace || gv_fs_trace_params) {
                SB_Buf_Line la_sender;
                char        la_startid[100];
                char        la_transid[100];
                fs_int_fs_format_sender(la_sender,
                                        reinterpret_cast<short *>(&lp_wr_ctrl_req->sender.phandle));
                fs_int_fs_format_tcbref(la_transid, &lp_wr_ctrl_req->tcbref);
                fs_int_fs_format_startid(la_startid, lp_wr_ctrl_req->startid);
                trace_where_printf(WHERE, "Received WRITE message, sender=%s, file=%d, tcbref=%s, startid=%s, uid=%d\n",
                                   la_sender,
                                   lp_wr_ctrl_req->sender.first_word.filenum,
                                   la_transid,
                                   la_startid,
                                   lp_wr_ctrl_req->userid);
            }
            if (TRANSID_IS_VALID(lp_wr_ctrl_req->tcbref)) {
                TRANSID_COPY(lp_ru->iv_transid, lp_wr_ctrl_req->tcbref);
                lv_fserr = static_cast<short>(ms_transid_reg(lp_ru->iv_transid, lp_wr_ctrl_req->startid));
            } else
                lv_fserr = XZFIL_ERR_OK;
            if (lv_fserr == XZFIL_ERR_OK) {
                gv_fs_ri_io_type = XZSYS_VAL_RCV_IOTYPE_WRITE;
                SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_FS_COMP_IO_WRITE,
                                   lv_sre.sre_msgId);
            }
            gv_fs_ri_userid = lp_wr_ctrl_req->userid;
            break;

        case FS_FS_WRITEREAD:
            lp_req_type = "writeread";
            if (gv_fs_trace || gv_fs_trace_params) {
                SB_Buf_Line la_sender;
                char        la_startid[100];
                char        la_transid[100];
                fs_int_fs_format_sender(la_sender,
                                        reinterpret_cast<short *>(&lp_wr_ctrl_req->sender.phandle));
                fs_int_fs_format_tcbref(la_transid, &lp_wr_ctrl_req->tcbref);
                fs_int_fs_format_startid(la_startid, lp_wr_ctrl_req->startid);
                trace_where_printf(WHERE, "Received WRITEREAD message, sender=%s, file=%d, tcbref=%s, startid=%s, uid=%d\n",
                                   la_sender,
                                   lp_wr_ctrl_req->sender.first_word.filenum,
                                   la_transid,
                                   la_startid,
                                   lp_wr_ctrl_req->userid);
            }
            if (TRANSID_IS_VALID(lp_wr_ctrl_req->tcbref)) {
                TRANSID_COPY(lp_ru->iv_transid, lp_wr_ctrl_req->tcbref);
                lv_fserr = static_cast<short>(ms_transid_reg(lp_ru->iv_transid, lp_wr_ctrl_req->startid));
            } else
                lv_fserr = XZFIL_ERR_OK;
            if (lv_fserr == XZFIL_ERR_OK) {
                gv_fs_ri_io_type = XZSYS_VAL_RCV_IOTYPE_WRITEREAD;
                SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_FS_COMP_IO_WRITEREAD,
                                   lv_sre.sre_msgId);
            }
            gv_fs_ri_userid = lp_wr_ctrl_req->userid;
            break;

        default:
            SB_util_abort("invalid lp_wr_ctrl_req->request_type"); // sw fault
        }

        if (lv_fserr != XZFIL_ERR_OK) {
            // send back an error reply
            lp_open_ctrl_rep->dialect_type = DIALECT_FS_FS;
            lp_open_ctrl_rep->reply_type = FS_FS_OPEN_REPLY;
            lp_open_ctrl_rep->reply_version = CURRENT_VERSION_FS_FS;
            lp_open_ctrl_rep->error = lv_fserr;
            SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_FS_COMP_REPLY_ERR,
                               lv_sre.sre_msgId,
                               lv_fserr);
            BMSG_REPLY_(lv_sre.sre_msgId,            // msgid
                        reinterpret_cast<short *>(lp_open_ctrl_rep),  // replyctrl
                        sizeof(fs_fs_open_reply),    // replyctrlsize
                        NULL,                        // replydata
                        0,                           // replydatasize
                        0,                           // errorclass
                        NULL);                       // newphandle
            *pp_complete = false;
            fs_int_fd_op_change_depth(pp_fd, 1); // readupdate not done
            return XZFIL_ERR_OK;
        }
        gv_fs_ri_file_number = lp_wr_ctrl_req->sender.first_word.filenum;
        gv_fs_ri_sync_id = static_cast<short>(lp_wr_ctrl_req->sender.syncid);
        memcpy(&gv_fs_ri_phandle,
               &lp_wr_ctrl_req->sender.phandle,
               sizeof(SB_Phandle));
    }
    gv_fs_ri_msg_tag = static_cast<short>(pp_fd->iv_ru_curr_tag);
#ifdef USE_SB_NEW_RI
    gv_fs_ri_max_reply_count = lv_sre.sre_replyDataMax;
#else
    gv_fs_ri_max_reply_count = static_cast<short>(lv_sre.sre_replyDataMax);
#endif
    if (gv_fs_trace)
        trace_where_printf(WHERE, "tag=%d, msgid=%d\n",
                           pp_fd->iv_ru_curr_tag, lp_ru->iv_msgid);
    if (lv_is_sys_msg)
        lv_fserr = fs_int_err_fd_rtn_assert(WHERE,
                                            pp_fd,
                                            XZFIL_ERR_SYSMESS,
                                            XZFIL_ERR_SYSMESS);
    else {
        lv_rc = sbmin(lp_ru->iv_read_count, lv_sre.sre_reqDataSize);
        if (lp_ru->ip_buffer != NULL)
            lv_fserr = msg_buf_read_data_int(lv_sre.sre_msgId,
                                             lp_ru->ip_buffer,
                                             lv_rc);
        else {
            SB_util_assert_pne(ppp_buf, NULL); // MUST supply
            lv_fserr = msg_buf_read_data(lv_sre.sre_msgId,
                                         reinterpret_cast<char **>(ppp_buf),
                                         &lv_rc,
                                         true);
        }
    }
    if (pp_xfercount != NULL)
        *pp_xfercount = lv_rc;
    if (pp_tag != NULL)
        *pp_tag = lp_ru->iv_tag;
    if (pp_segid != NULL)
        *pp_segid = 0;
    if (gv_fs_trace_params) {
        if (lv_sre_type == BSRETYPE_IREQ) {
            MS_Md_Type *lp_md =
              SB_Trans::Msg_Mgr::map_to_md(lv_sre.sre_msgId, WHERE);
            if (lp_md != NULL) {
                SB_Trans::Stream_Base *lp_stream =
                  static_cast<SB_Trans::Stream_Base *>(lp_md->ip_stream);
                if (lp_stream != NULL) {
                    char *lp_from = lp_stream->get_name();
                    if (lv_sre.sre_flags & XSRE_MON)
                        trace_where_printf(WHERE, "Received message from=%s, msgid=%d, type=mon-%s, tag=" PFTAG ", msg-num=%d\n",
                                           lp_from, lv_sre.sre_msgId,
                                           lp_req_type, lp_ru->iv_tag,
                                           gv_fs_ri_msg_tag);
                    else
                        trace_where_printf(WHERE, "Received message from=%s, msgid=%d, type=fs-%s, tag=" PFTAG ", msg-num=%d\n",
                                           lp_from, lv_sre.sre_msgId,
                                           lp_req_type, lp_ru->iv_tag,
                                           gv_fs_ri_msg_tag);
                }
            }
        }
    }
    if (lp_ru->iv_read) {
        if ((lv_fserr == XZFIL_ERR_OK) || (lv_fserr != XZFIL_ERR_TIMEDOUT))
            fs_int_fs_file_replyx_auto(pp_fd);
    }
    return lv_fserr;
}

//
// Purpose: cancel nowait-open
//
void fs_int_fs_file_cancel_nowait_open(FS_Fd_Type *pp_fd) {
    const char    *WHERE = "XFILE_CLOSE_(internal)";
    SB_Comp_Queue *lp_fs_comp_q;
    FS_Io_Type    *lp_io;
    MS_Md_Type    *lp_md;
    bool           lv_ts;
    short          lv_wait;

    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "ENTER fnum=%d\n", pp_fd->iv_file_num);

    lp_io = &pp_fd->iv_nowait_open_io;

    fs_int_fd_nowait_open_mutex_lock(WHERE, pp_fd);
    pp_fd->iv_nowait_open_cancel = true;

    if (lp_io->iv_msgid) {
        // open-thread running
        if (gv_fs_trace)
            trace_where_printf(WHERE, "nowait-open, set LDONE, io=%p, inuse=true, msgid=%d\n",
                               pfp(lp_io), lp_io->iv_msgid);
        lp_md = SB_Trans::Msg_Mgr::map_to_md(lp_io->iv_msgid, WHERE);
        lp_md->ip_mgr->set_event(LDONE, NULL);
    }
    fs_int_fd_nowait_open_mutex_unlock(WHERE, pp_fd);
    lp_md = SB_Trans::Msg_Mgr::map_to_md(lp_io->iv_msgid_open, WHERE);
    // open-thread complete - cleanup any fs-completion
    while (!lp_md->iv_reply_done) {
        lv_wait = XWAIT(LDONE, -1);
        CHK_WAITIGNORE(lv_wait);
    }
    if (gv_fs_trace)
        trace_where_printf(WHERE, "nowait-open, remove fs-comp, io=%p, inuse=false, msgid-open=%d\n",
                           pfp(lp_io), lp_io->iv_msgid_open);
    lv_ts = pp_fd->iv_options & FS_OPEN_OPTIONS_TS;
    lp_fs_comp_q = ms_fsdone_get_comp_q(lv_ts);
    if (lp_fs_comp_q->remove_list(&lp_md->iv_link)) {
        fs_int_fd_ref_count_dec(pp_fd); // need additional dec
        SB_Trans::Msg_Mgr::put_md(lp_io->iv_msgid_open,
                                  "CLOSE on nowait-open");
    }

    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "EXIT\n");
}

//
// Purpose: cancelreq over ms
//
short fs_int_fs_file_cancelreq(FS_Fd_Type  *pp_fd,
                               SB_Tag_Type  pv_tag) {
    const char      *WHERE = "XCANCELREQ(internal)";
    FS_Io_Type      *lp_io;
    FS_Ru_Type      *lp_ru;
    short            lv_fserr;
    short            lv_fserr_temp;
    int              lv_msgid;
    SB_Transseq_Type lv_startid;
    SB_Transid_Type  lv_transid;

    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "ENTER fnum=%d, tag=" PFTAG "\n",
                           pp_fd->iv_file_num, pv_tag);
    switch (pp_fd->iv_file_type) {
    case FS_FILE_TYPE_PROCESS:
    case FS_FILE_TYPE_RECEIVE:
        break;
    default:
        return fs_int_err_fd_rtn(WHERE, pp_fd, XZFIL_ERR_INVALOP);
    }
    if (pp_fd->iv_nowait_depth <= 0)
        return fs_int_err_fd_rtn(WHERE, pp_fd, XZFIL_ERR_WAITFILE);

    switch (pp_fd->iv_file_type) {
    case FS_FILE_TYPE_PROCESS:
        lp_io = FS_util_io_tag_get_by_tag(pp_fd, pv_tag);
        if (lp_io == NULL)
            lv_fserr = fs_int_err_fd_rtn(WHERE, pp_fd, XZFIL_ERR_NONEOUT);
        else {
            if (gv_fs_trace_params && (pv_tag == BOMITTAG))
                trace_where_printf(WHERE, "omitted tag converted to tag=" PFTAG "\n",
                                   lp_io->iv_tag_user);
            lv_msgid = lp_io->iv_msgid;
            lv_fserr = BMSG_ABANDON_(lv_msgid);
            if ((lv_fserr == XZFIL_ERR_OK) ||
                (lv_fserr == XZFIL_ERR_PATHDOWN)) {
                TRANSID_COPY(lv_transid, lp_io->iv_transid);
                TRANSSEQ_COPY(lv_startid, lp_io->iv_startid);
                if (TRANSID_IS_VALID(lv_transid)) {
                    // check reinstate
                    lv_fserr_temp = static_cast<short>(ms_transid_reinstate(lv_transid, lv_startid));
                    if (lv_fserr_temp != XZFIL_ERR_OK)
                        lv_fserr = lv_fserr_temp;
                }
                FS_util_io_tag_free(pp_fd, lp_io);
                fs_int_fd_op_change_depth(pp_fd, -1);
            }
        }
        break;

    case FS_FILE_TYPE_RECEIVE:
        lp_ru = FS_util_ru_tag_get_by_tag(pp_fd, pv_tag);
        if (lp_ru == NULL)
            lv_fserr = fs_int_err_fd_rtn(WHERE, pp_fd, XZFIL_ERR_NONEOUT);
        else {
            if (gv_fs_trace_params && (pv_tag == BOMITTAG))
                trace_where_printf(WHERE, "omitted tag converted to tag=" PFTAG "\n",
                                   lp_ru->iv_tag);
            FS_util_ru_tag_free(pp_fd, lp_ru->iv_ru_tag);
            fs_int_fd_op_change_depth(pp_fd, -1);
            lv_fserr = XZFIL_ERR_OK;
            gv_ms_event_mgr.set_event_all(LREQ); // wakeup
        }
        break;

    default:
        SB_util_abort("invalid pp_fd->iv_file_type"); // sw fault
        lv_fserr = XZFIL_ERR_OK;
        break;
    }
    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return lv_fserr;
}

void fs_int_fs_file_cancelreq_all(FS_Fd_Type *pp_fd) {
    const char *WHERE = "XCANCELREQ(all-internal)";
    int         lv_tag;

    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "ENTER fnum=%d\n", pp_fd->iv_file_num);
    for (lv_tag = 0; lv_tag <= pp_fd->iv_nowait_depth; lv_tag++) {
        if (pp_fd->ip_io[lv_tag].iv_inuse)
            fs_int_fs_file_cancelreq(pp_fd,
                                     pp_fd->ip_io[lv_tag].iv_tag_user);
    }
    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "EXIT\n");
}

short fs_int_fs_file_close(FS_Fd_Type *pp_fd) {
    const char          *WHERE = "XFILE_CLOSE_(internal)";
    char                 la_close_ctrl_req_buf[sizeof(fs_fs_close)];
    char                 la_close_ctrl_rep_buf[sizeof(fs_fs_simple_reply)];
    fs_fs_close         *lp_close_ctrl_req;
    fs_fs_simple_reply  *lp_close_ctrl_rep;
    short                lv_fserr;
    int                  lv_msgid;
    MS_Result_Type       lv_results;
    bool                 lv_self;

    lv_self = msg_mon_is_self(&pp_fd->iv_phandle);
    if (lv_self)
        lp_close_ctrl_req =
          static_cast<fs_fs_close *>(MS_BUF_MGR_ALLOC(sizeof(fs_fs_close)));
    else
        lp_close_ctrl_req = reinterpret_cast<fs_fs_close *>(la_close_ctrl_req_buf);
#if 0
    // dirty request
    memset(lp_close_ctrl_req, -1, sizeof(fs_fs_close) + 10);
#endif
    lp_close_ctrl_rep = reinterpret_cast<fs_fs_simple_reply *>(la_close_ctrl_rep_buf);
    lp_close_ctrl_req->dialect_type = DIALECT_FS_FS;
    lp_close_ctrl_req->request_type = FS_FS_CLOSE;
    lp_close_ctrl_req->request_version = CURRENT_VERSION_FS_FS;
    lp_close_ctrl_req->minimum_interpretation_version = MINIMUM_VERSION_FS_FS;
    lp_close_ctrl_req->sender.first_word.filenum = pp_fd->iv_file_num;
    lp_close_ctrl_req->sender.syncid = 0;
    ms_od_get_my_phandle(reinterpret_cast<SB_Phandle_Type *>(&lp_close_ctrl_req->sender.phandle));
    lp_close_ctrl_req->sender.user_openid = 0;
    lp_close_ctrl_req->sender.id.openid = 0;
    if (gv_fs_trace)
        trace_where_printf(WHERE,
                           "CLOSE req=%p, dtype=%d(%s), rtype=%d(%s), rvers=%d, minvers=%d\n",
                           pfp(lp_close_ctrl_req),
                           lp_close_ctrl_req->dialect_type,
                           fs_int_fs_get_dialect_type(lp_close_ctrl_req->dialect_type),
                           lp_close_ctrl_req->request_type,
                           fs_int_fs_get_req_type(lp_close_ctrl_req->request_type),
                           lp_close_ctrl_req->request_version,
                           lp_close_ctrl_req->minimum_interpretation_version);
    if (lv_self) {
        if (gv_fs_trace)
            trace_where_printf(WHERE, "sending to self\n");
        msg_send_self(&pp_fd->iv_phandle,                // phandle
                      reinterpret_cast<short *>(lp_close_ctrl_req),       // reqctrl
                      sizeof(fs_fs_close),               // reqctrlsize
                      NULL,                              // reqdata
                      0);                                // reqdatasize
        return XZFIL_ERR_OK;
    }
    lv_fserr = BMSG_LINK_(&pp_fd->iv_phandle,                // phandle
                          &lv_msgid,                         // msgid
                          reinterpret_cast<short *>(lp_close_ctrl_req),       // reqctrl
                          sizeof(fs_fs_close),               // reqctrlsize
                          reinterpret_cast<short *>(lp_close_ctrl_rep),       // replyctrl
                          sizeof(la_close_ctrl_rep_buf),     // replyctrlmax
                          NULL,                              // reqdata
                          0,                                 // reqdatasize
                          NULL,                              // replydata
                          0,                                 // replydatamax
                          reinterpret_cast<long>(pp_fd),     // linkertag
                          0,                                 // pri
                          0,                                 // xmitclass
                          BMSG_LINK_FSREQ                    // linkopts
                         );
    if (lv_fserr != XZFIL_ERR_OK)
        return fs_int_err_fd_rtn(WHERE, pp_fd, lv_fserr);
    lv_fserr = BMSG_BREAK_(lv_msgid,              // msgid
                           reinterpret_cast<short *>(&lv_results), // results
                           &pp_fd->iv_phandle);   // phandle
    if (lv_fserr != XZFIL_ERR_OK)
        return fs_int_err_fd_rtn(WHERE, pp_fd, lv_fserr);
    return XZFIL_ERR_OK;
}


short fs_int_fs_file_getri(FS_RI_Type *pp_ri) {
    pp_ri->iv_io_type = gv_fs_ri_io_type;
    pp_ri->iv_max_reply_count = gv_fs_ri_max_reply_count;
    pp_ri->iv_message_tag = gv_fs_ri_msg_tag;
    pp_ri->iv_file_number = gv_fs_ri_file_number;
    pp_ri->iv_sync_id = gv_fs_ri_sync_id;
    memcpy(&pp_ri->iv_sender, &gv_fs_ri_phandle, sizeof(SB_Phandle));
    pp_ri->iv_open_label = -1; // TODO
    pp_ri->iv_user_id = gv_fs_ri_userid;
    return XZFIL_ERR_OK;
}


//
// Purpose: file-open over ms
//
short fs_int_fs_file_open(FS_Fd_Type *pp_fd,
                          const char *pp_filename,
                          bool        pv_self) {
    const char *WHERE = "XFILE_OPEN_(internal)";
    FS_Io_Type *lp_io;
    MS_Md_Type *lp_md;
    int         lv_msgid;
    short       lv_ret;
    bool        lv_ts;

    if (pp_fd->iv_nowait_open) {
        SB_Trans::Msg_Mgr::get_md(&lp_md, // fs_int_fs_file_open
                                  NULL,
                                  gv_ms_event_mgr.get_mgr(NULL),
                                  true,      // send
                                  &lv_ret,   // fserr
                                  WHERE,
                                  MD_STATE_MSG_FS_NOWAIT_OPEN);
        if (lp_md != NULL) {
            // setup for completion
            lv_msgid = lp_md->iv_link.iv_id.i;
            lp_io = FS_util_io_tag_alloc_nowait_open(pp_fd, lv_msgid);
            lp_md->iv_tag = reinterpret_cast<long>(lp_io);
            lv_ts = pp_fd->iv_options & FS_OPEN_OPTIONS_TS;
            lp_md->ip_fs_comp_q = ms_fsdone_get_comp_q(lv_ts);
            SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_FS_OPEN_MD,
                               pp_fd->iv_file_num,
                               lv_msgid);
            if (gv_fs_trace_params)
                trace_where_printf(WHERE, "nowait-open md msgid=%d, md-tag=0x%lx, filename=%s, fnum=%d\n",
                                   lv_msgid,
                                   lp_md->iv_tag,
                                   pp_fd->ia_fname,
                                   pp_fd->iv_file_num);
            // dispatch work
            Fs_Open_Thread::create(WHERE,
                                   lp_md,
                                   pp_fd,
                                   pp_filename,
                                   pv_self);
            lv_ret = XZFIL_ERR_OK;
        }
    } else
        lv_ret = fs_int_fs_file_open_ph2(pp_fd, pp_filename, pv_self);
    return lv_ret;
}

short fs_int_fs_file_open_ph2(FS_Fd_Type *pp_fd,
                              const char *pp_filename,
                              bool        pv_self) {
    const char        *WHERE = "XFILE_OPEN_(internal)";
    char               la_open_ctrl_req_buf[sizeof(fs_fs_open)];
    char               la_open_ctrl_rep_buf[sizeof(fs_fs_open)];
    FS_Io_Type        *lp_io;
    fs_fs_open        *lp_open_ctrl_req;
    fs_fs_open_reply  *lp_open_ctrl_rep;
    bool               lv_break;
    bool               lv_cancel;
    short              lv_fserr;
    MS_Result_Type     lv_results;

    if (pv_self)
        lv_fserr = static_cast<short>(
          msg_mon_open_process_self(&pp_fd->iv_phandle,
                                    &pp_fd->iv_oid));
    else
        lv_fserr = static_cast<short>(
          msg_mon_open_process_fs(const_cast<char *>(pp_filename),
                               &pp_fd->iv_phandle,
                               &pp_fd->iv_oid));
    if (lv_fserr != XZFIL_ERR_OK)
        return fs_int_err_fd_rtn(WHERE, pp_fd, lv_fserr);

    if (gv_fs_trace)
        trace_where_printf(WHERE, "open-process complete, phandle initialized\n");
    pp_fd->iv_phandle_initialized = true;

    if (pp_fd->iv_nowait_open_cancel) {
        if (gv_fs_trace)
            trace_where_printf(WHERE, "nowait-open, open-process complete, handle cancel\n");
        return fs_int_err_fd_rtn(WHERE, pp_fd, XZFIL_ERR_REQABANDONED);
    }

    if (pv_self)
        lp_open_ctrl_req =
          static_cast<fs_fs_open *>(MS_BUF_MGR_ALLOC(sizeof(fs_fs_open)));
    else
        lp_open_ctrl_req = reinterpret_cast<fs_fs_open *>(la_open_ctrl_req_buf);
#if 0
    // dirty request
    memset(lp_open_ctrl_req, -1, sizeof(fs_fs_open) + 10);
#endif
    lp_open_ctrl_rep = reinterpret_cast<fs_fs_open_reply *>(la_open_ctrl_rep_buf);
    lp_open_ctrl_req->dialect_type = DIALECT_FS_FS;
    lp_open_ctrl_req->request_type = FS_FS_OPEN;
    lp_open_ctrl_req->request_version = CURRENT_VERSION_FS_FS;
    lp_open_ctrl_req->minimum_interpretation_version = MINIMUM_VERSION_FS_FS;
    lp_open_ctrl_req->security.offset = 0;
    lp_open_ctrl_req->security.length = 0;
    lp_open_ctrl_req->access = 0;
    lp_open_ctrl_req->exclusion = 0;
    lp_open_ctrl_req->nowait = 0;
    lp_open_ctrl_req->syncdepth = 0;
    lp_open_ctrl_req->options.flags.initialize0 = 0;
    lp_open_ctrl_req->sender.first_word.filenum = pp_fd->iv_file_num;
    lp_open_ctrl_req->sender.syncid = 0;
    ms_od_get_my_phandle(reinterpret_cast<SB_Phandle_Type *>(&lp_open_ctrl_req->sender.phandle));
    lp_open_ctrl_req->sender.user_openid = 0;
    lp_open_ctrl_req->sender.id.openid = 0;
    lp_open_ctrl_req->sender_crtpid.crt.crt.name.name[0] = 0; // int16
    lp_open_ctrl_req->sender_crtpid.crt.crt.name.name[1] = 0; // int16
    lp_open_ctrl_req->sender_crtpid.crt.crt.name.name[2] = 0; // int16
    lp_open_ctrl_req->sender_crtpid.pid.pid = 0;
    lp_open_ctrl_req->flags.flags0d = 0;
    memset(&lp_open_ctrl_req->gmom.phandle, 0,
           sizeof(lp_open_ctrl_req->gmom.phandle));
    lp_open_ctrl_req->gmom.jobid = 0;
    memset(&lp_open_ctrl_req->primary.phandle, 0,
           sizeof(lp_open_ctrl_req->primary.phandle));
    lp_open_ctrl_req->primary.filenum = 0;
    lp_open_ctrl_req->primary.openid = 0;
    lp_open_ctrl_req->qualifier.offset = 0;
    lp_open_ctrl_req->qualifier.length = 0;
    lp_open_ctrl_req->opener_filename.offset = 0;
    lp_open_ctrl_req->opener_filename.length = 0;
    if (gv_fs_trace)
        trace_where_printf(WHERE,
                           "OPEN req=%p, dtype=%d(%s), rtype=%d(%s), rvers=%d, minvers=%d\n",
                           pfp(lp_open_ctrl_req),
                           lp_open_ctrl_req->dialect_type,
                           fs_int_fs_get_dialect_type(lp_open_ctrl_req->dialect_type),
                           lp_open_ctrl_req->request_type,
                           fs_int_fs_get_req_type(lp_open_ctrl_req->request_type),
                           lp_open_ctrl_req->request_version,
                           lp_open_ctrl_req->minimum_interpretation_version);

    if (pv_self) {
        if (gv_fs_trace)
            trace_where_printf(WHERE, "sending to self\n");
        msg_send_self(&pp_fd->iv_phandle,                // phandle
                      reinterpret_cast<short *>(lp_open_ctrl_req),        // reqctrl
                      sizeof(fs_fs_open),                // reqctrlsize
                      NULL,                              // reqdata
                      0);                                // reqdatasize
        return XZFIL_ERR_OK;
    }

    lp_io = &pp_fd->iv_nowait_open_io;
    fs_int_fd_nowait_open_mutex_lock(WHERE, pp_fd);
    lv_fserr = BMSG_LINK_(&pp_fd->iv_phandle,                // phandle
                          &lp_io->iv_msgid,                  // msgid
                          reinterpret_cast<short *>(lp_open_ctrl_req),        // reqctrl
                          sizeof(fs_fs_open),                // reqctrlsize
                          reinterpret_cast<short *>(lp_open_ctrl_rep),        // replyctrl
                          sizeof(la_open_ctrl_rep_buf),      // replyctrlmax
                          NULL,                              // reqdata
                          0,                                 // reqdatasize
                          NULL,                              // replydata
                          0,                                 // replydatamax
                          reinterpret_cast<long>(pp_fd),     // linkertag
                          0,                                 // pri
                          0,                                 // xmitclass
                          BMSG_LINK_LDONEQ);                 // linkopts
    lv_cancel = pp_fd->iv_nowait_open_cancel;
    fs_int_fd_nowait_open_mutex_unlock(WHERE, pp_fd);
    if (lv_fserr != XZFIL_ERR_OK)
        return fs_int_err_fd_rtn(WHERE, pp_fd, lv_fserr);
    if (!lv_cancel) {
        lv_fserr = XWAIT(LDONE, -1);
        CHK_WAITIGNORE(lv_fserr);
    }
    fs_int_fd_nowait_open_mutex_lock(WHERE, pp_fd);
    lv_cancel = pp_fd->iv_nowait_open_cancel;
    if (gv_fs_shutdown || lv_cancel) {
        if (BMSG_ISDONE_(lp_io->iv_msgid))
            lv_break = true;
        else {
            lv_break = false;
            lv_fserr = BMSG_ABANDON_(lp_io->iv_msgid);
            CHK_FEIGNORE(lv_fserr);
            lp_io->iv_msgid = 0;
        }
    } else
        lv_break = true;
    if (lv_break) {
        lv_fserr = BMSG_BREAK_(lp_io->iv_msgid,                        // msgid
                               reinterpret_cast<short *>(&lv_results), // results
                               &pp_fd->iv_phandle);                    // phandle
        lp_io->iv_msgid = 0;
        fs_int_fd_nowait_open_mutex_unlock(WHERE, pp_fd);
        if (lv_fserr != XZFIL_ERR_OK)
            return fs_int_err_fd_rtn(WHERE, pp_fd, lv_fserr);
        if (lp_open_ctrl_rep->error != XZFIL_ERR_OK)
            return lp_open_ctrl_rep->error;
    } else {
        fs_int_fd_nowait_open_mutex_unlock(WHERE, pp_fd);
    }
    return XZFIL_ERR_OK;
}

//
// Purpose: readx over ms
//
short fs_int_fs_file_readx(FS_Fd_Type  *pp_fd,
                           int          pv_ru_tag,
                           int         *pp_msgid,
                           int         *pp_length,
                           char       **ppp_buffer,
                           int          pv_read_count) {
    const char *WHERE = "XREADX(internal)";
    short       lv_fserr;

    pv_ru_tag = pv_ru_tag;         // no-warn
    pp_msgid = pp_msgid;           // no-warn
    pv_read_count = pv_read_count; // no-warn

    switch (pp_fd->iv_file_type) {
    case FS_FILE_TYPE_RECEIVE:
        break;
    default:
        return fs_int_err_fd_rtn(WHERE, pp_fd, XZFIL_ERR_INVALOP);
    }
    lv_fserr = fs_int_fd_op_check(WHERE, pp_fd);
    if (lv_fserr != XZFIL_ERR_OK)
        return fs_int_err_fd_rtn(WHERE, pp_fd, lv_fserr);
    fs_int_fd_op_change_depth(pp_fd, 1);
    if (pp_fd->iv_nowait_depth <= 0) {
        short lv_tfilenum = pp_fd->iv_file_num;
        int   lv_xfer_count;
        lv_fserr = fs_int_fs_file_awaitiox(&lv_tfilenum,
                                           reinterpret_cast<void **>(ppp_buffer), // buf
                                           &lv_xfer_count,
                                           NULL,                 // tag
                                           -1,                   // timeout
                                           NULL,                 // segid
                                           true,                 // internal
                                           false);               // ts
        if (lv_fserr != XZFIL_ERR_OK)
            return fs_int_err_fd_rtn(WHERE, pp_fd, lv_fserr);
        if (pp_length != NULL)
            *pp_length = lv_xfer_count;
    } else {
        if (pp_length != NULL)
            *pp_length = -1;
        lv_fserr = XZFIL_ERR_OK;
    }
    return lv_fserr;
}

//
// Purpose: readupdatex over ms
//
short fs_int_fs_file_readupdatex(FS_Fd_Type  *pp_fd,
                                 int          pv_ru_tag,
                                 int         *pp_msgid,
                                 int         *pp_length,
                                 char       **ppp_buffer,
                                 int          pv_read_count) {
    const char *WHERE = "XREADUPDATEX(internal)";
    short       lv_fserr;

    pv_ru_tag = pv_ru_tag;         // no-warn
    pp_msgid = pp_msgid;           // no-warn
    pv_read_count = pv_read_count; // no-warn

    switch (pp_fd->iv_file_type) {
    case FS_FILE_TYPE_RECEIVE:
        break;
    default:
        return fs_int_err_fd_rtn(WHERE, pp_fd, XZFIL_ERR_INVALOP);
    }
    lv_fserr = fs_int_fd_op_check(WHERE, pp_fd);
    if (lv_fserr != XZFIL_ERR_OK)
        return fs_int_err_fd_rtn(WHERE, pp_fd, lv_fserr);
    fs_int_fd_op_change_depth(pp_fd, 1);
    if (pp_fd->iv_nowait_depth <= 0) {
        short lv_tfilenum = pp_fd->iv_file_num;
        int   lv_xfer_count;
        lv_fserr = fs_int_fs_file_awaitiox(&lv_tfilenum,
                                           reinterpret_cast<void **>(ppp_buffer), // buf
                                           &lv_xfer_count,
                                           NULL,                 // tag
                                           -1,                   // timeout
                                           NULL,                 // segid
                                           true,                 // internal
                                           false);               // ts
        if (pp_length != NULL)
            *pp_length = lv_xfer_count;
        if (lv_fserr != XZFIL_ERR_OK)
            return fs_int_err_fd_rtn(WHERE, pp_fd, lv_fserr);
    } else {
        if (pp_length != NULL)
            *pp_length = -1;
        lv_fserr = XZFIL_ERR_OK;
    }
    return lv_fserr;
}

//
// Purpose: replyx over ms
//
short fs_int_fs_file_replyx(FS_Fd_Type       *pp_fd,
                            int               pv_msgid,
                            char             *pp_buffer,
                            int               pv_write_count,
                            int               pv_count_written,
                            int               pv_io_type,
                            short             pv_err_ret,
                            SB_Transid_Type   pv_transid,
                            SB_Transseq_Type  pv_startid) {
    const char             *WHERE = "XREPLYX(internal)";
    char                    la_wr_ctrl_rep_buf[sizeof(fs_fs_writeread_reply)];
    fs_fs_writeread_reply  *lp_wr_ctrl_rep;

    switch (pp_fd->iv_file_type) {
    case FS_FILE_TYPE_RECEIVE:
        break;
    default:
        return fs_int_err_fd_rtn(WHERE, pp_fd, XZFIL_ERR_INVALOP);
    }
    lp_wr_ctrl_rep = reinterpret_cast<fs_fs_writeread_reply *>(la_wr_ctrl_rep_buf);
    lp_wr_ctrl_rep->dialect_type = DIALECT_FS_FS;
    if (pv_io_type == FS_FS_WRITE)
        lp_wr_ctrl_rep->reply_type = FS_FS_WRITE_REPLY;
    else
        lp_wr_ctrl_rep->reply_type = FS_FS_WRITEREAD_REPLY;
    lp_wr_ctrl_rep->reply_version = CURRENT_VERSION_FS_FS;
    lp_wr_ctrl_rep->error = pv_err_ret;
    lp_wr_ctrl_rep->countwritten = pv_count_written;
    if (gv_fs_trace || (gv_fs_trace_errors && (pv_err_ret != XZFIL_ERR_OK))) {
        const char *lp_reply_type;
        if (lp_wr_ctrl_rep->reply_type == FS_FS_WRITE_REPLY)
            lp_reply_type = "W";
        else
            lp_reply_type = "WR";
        trace_where_printf(WHERE, "%s reply. dtype=%d(%s), rtype=%d(%s), rvers=%d, fserr=%d, cw=%d [op-depth=%d]\n",
                           lp_reply_type,
                           lp_wr_ctrl_rep->dialect_type,
                           fs_int_fs_get_dialect_type(lp_wr_ctrl_rep->dialect_type),
                           lp_wr_ctrl_rep->reply_type,
                           fs_int_fs_get_req_type(lp_wr_ctrl_rep->reply_type),
                           lp_wr_ctrl_rep->reply_version,
                           lp_wr_ctrl_rep->error,
                           lp_wr_ctrl_rep->countwritten,
                           pp_fd->iv_op_depth);
    }
    if (TRANSID_IS_VALID(pv_transid))
        ms_transid_clear(pv_transid, pv_startid);
    BMSG_REPLY_(pv_msgid,                       // msgid
                reinterpret_cast<short *>(lp_wr_ctrl_rep),       // replyctrl
                sizeof(fs_fs_writeread_reply),  // replyctrlsize
                pp_buffer,                      // replydata
                pv_write_count,                 // replydatasize
                0,                              // errorclass
                NULL);                          // newphandle
    return XZFIL_ERR_OK;
}

void fs_int_fs_file_replyx_auto(FS_Fd_Type *pp_fd) {
    const char      *WHERE = "XREPLYX(auto-internal)";
    FS_Ru_Type      *lp_ru;
    int              lv_count_written;
    short            lv_fserr;
    int              lv_io_type;
    int              lv_msgid;
    int              lv_reply_num;
    SB_Transseq_Type lv_startid;
    SB_Transid_Type  lv_transid;

    lv_reply_num = gv_fs_ri_msg_tag;
    lp_ru = &pp_fd->ip_ru[lv_reply_num];
    lv_msgid = lp_ru->iv_msgid;
    lv_count_written = lp_ru->iv_count_written;
    lv_io_type = lp_ru->iv_io_type;
    lv_transid = lp_ru->iv_transid;
    lv_startid = lp_ru->iv_startid;
    if (gv_fs_trace)
        trace_where_printf(WHERE, "tag=%d, msgid=%d\n",
                           lv_reply_num, lv_msgid);
    FS_util_ru_tag_free(pp_fd, lv_reply_num);
    lv_fserr = fs_int_fs_file_replyx(pp_fd,
                                     lv_msgid,
                                     NULL,              // buffer
                                     0,                 // write count
                                     lv_count_written,
                                     lv_io_type,
                                     XZFIL_ERR_OK,      // err ret
                                     lv_transid,
                                     lv_startid);
    CHK_FEIGNORE(lv_fserr);
}

//
// Purpose: setmode over ms
//
short fs_int_fs_file_setmode(FS_Fd_Type *pp_fd,
                             short       pv_modenum,
                             int         pv_parm1,
                             int         pv_parm2,
                             short      *pp_oldval) {
    const char        *WHERE = "XSETMODE(internal)";
    short              lv_fserr;

    pv_parm2 = pv_parm2; // touch
    switch (pv_modenum) {
    case  117: // set TRANSID forwarding
        switch (pv_parm1) {
        case 0: // normal mode
            *pp_oldval = static_cast<short>(pp_fd->iv_transid_supp ? 1 : 0);
            pp_fd->iv_transid_supp = false;
            lv_fserr = XZFIL_ERR_OK;
            break;
        case 1: // suppress mode
            *pp_oldval = static_cast<short>(pp_fd->iv_transid_supp ? 1 : 0);
            pp_fd->iv_transid_supp = true;
            lv_fserr = XZFIL_ERR_OK;
            break;
        default:
            lv_fserr = XZFIL_ERR_BOUNDSERR;
            break;
        }
        break;
    default:
        lv_fserr = XZFIL_ERR_OK; // no-op
        break;
    }
    return fs_int_err_fd_rtn(WHERE, pp_fd, lv_fserr);
}

//
// Purpose: writex over ms
//
short fs_int_fs_file_writex(FS_Fd_Type  *pp_fd,
                            char        *pp_buffer,
                            int          pv_write_count,
                            int         *pp_count_written,
                            SB_Tag_Type  pv_tag,
                            SB_Uid_Type  pv_userid) {
    const char         *WHERE = "XWRITEX(internal)";
    char               *lp_w_ctrl_req_buf;
    fs_fs_write        *lp_w_ctrl_req;
    char               *lp_w_ctrl_rep_buf;
    fs_fs_write_reply  *lp_w_ctrl_rep;
    short               lv_fserr;
    short               lv_fserr_temp;
    int                 lv_msgid;
    short               lv_opts;
    MS_Result_Type      lv_results;
    SB_Transseq_Type    lv_startid;
    SB_Transid_Type     lv_transid;

    switch (pp_fd->iv_file_type) {
    case FS_FILE_TYPE_PROCESS:
        break;
    default:
        return fs_int_err_fd_rtn(WHERE, pp_fd, XZFIL_ERR_INVALOP);
    }
    lv_fserr = fs_int_fd_op_check(WHERE, pp_fd);
    if (lv_fserr != XZFIL_ERR_OK)
        return fs_int_err_fd_rtn(WHERE, pp_fd, lv_fserr);
    lv_fserr = static_cast<short>(ms_transid_get(pp_fd->iv_transid_supp,
                                                 false,
                                                 &lv_transid,
                                                 &lv_startid));
    if (lv_fserr != XZFIL_ERR_OK)
        return fs_int_err_fd_rtn(WHERE, pp_fd, lv_fserr);
    lp_w_ctrl_req_buf =
      static_cast<char *>(MS_BUF_MGR_ALLOC(sizeof(fs_fs_write)));
    lp_w_ctrl_rep_buf =
      static_cast<char *>(MS_BUF_MGR_ALLOC(sizeof(fs_fs_write_reply)));
    lp_w_ctrl_req = reinterpret_cast<fs_fs_write *>(lp_w_ctrl_req_buf);
#if 0
    // dirty request
    memset(lp_w_ctrl_req, -1, sizeof(fs_fs_write));
#endif
    lp_w_ctrl_rep = reinterpret_cast<fs_fs_write_reply *>(lp_w_ctrl_rep_buf);
    lp_w_ctrl_req->dialect_type = DIALECT_FS_FS;
    lp_w_ctrl_req->request_type = FS_FS_WRITE;
    lp_w_ctrl_req->request_version = CURRENT_VERSION_FS_FS;
    lp_w_ctrl_req->minimum_interpretation_version = MINIMUM_VERSION_FS_FS;
    lp_w_ctrl_req->sender.first_word.filenum = pp_fd->iv_file_num;
    lp_w_ctrl_req->sender.syncid = 1;
    ms_od_get_my_phandle(reinterpret_cast<SB_Phandle_Type *>(&lp_w_ctrl_req->sender.phandle));
    lp_w_ctrl_req->sender.user_openid = 1;
    lp_w_ctrl_req->sender.id.openid = 1;
    lp_w_ctrl_req->flags.flags0d = 1;
    TRANSID_COPY(lp_w_ctrl_req->tcbref, lv_transid);
    TRANSSEQ_COPY(lp_w_ctrl_req->startid, lv_startid);
    lp_w_ctrl_req->userid = pv_userid;
    lp_w_ctrl_req->lid.offset = 1;
    lp_w_ctrl_req->lid.length = 1;
    FS_Io_Type *lp_io =
      FS_util_io_tag_alloc(pp_fd,
                           FS_FS_WRITE,
                           pv_tag,
                           lv_transid,
                           lv_startid,
                           pp_buffer);
    if (gv_fs_trace)
        trace_where_printf(WHERE,
                           "W req=%p, dtype=%d(%s), rtype=%d(%s), rvers=%d, minvers=%d, io=%ld, fnum=%d, tag=" PFTAG ", uid=%d\n",
                           pfp(lp_w_ctrl_req),
                           lp_w_ctrl_req->dialect_type,
                           fs_int_fs_get_dialect_type(lp_w_ctrl_req->dialect_type),
                           lp_w_ctrl_req->request_type,
                           fs_int_fs_get_req_type(lp_w_ctrl_req->request_type),
                           lp_w_ctrl_req->request_version,
                           lp_w_ctrl_req->minimum_interpretation_version,
                           reinterpret_cast<long>(lp_io),
                           pp_fd->iv_file_num,
                           pv_tag,
                           pv_userid);
    lv_opts = XMSG_LINK_FSDONEQ;
    if (pp_fd->iv_options & FS_OPEN_OPTIONS_TS)
        lv_opts |= XMSG_LINK_FSDONETSQ;
    lv_fserr = BMSG_LINK_(&pp_fd->iv_phandle,                // phandle
                          &lv_msgid,                         // msgid
                          reinterpret_cast<short *>(lp_w_ctrl_req), // reqctrl
                          sizeof(fs_fs_write),               // reqctrlsize
                          reinterpret_cast<short *>(lp_w_ctrl_rep), // replyctrl
                          sizeof(fs_fs_write_reply),         // replyctrlmax
                          pp_buffer,                         // reqdata
                          pv_write_count,                    // reqdatasize
                          NULL,                              // replydata
                          0,                                 // replydatamax
                          reinterpret_cast<long>(lp_io),     // linkertag
                          0,                                 // pri
                          0,                                 // xmitclass
                          lv_opts);                          // linkopts
    if (lv_fserr != XZFIL_ERR_OK)
        return fs_int_err_fd_rtn(WHERE, pp_fd, lv_fserr);
    lp_io->iv_msgid = lv_msgid;
    fs_int_fd_op_change_depth(pp_fd, 1);
    if (pp_fd->iv_nowait_depth <= 0) {
        MS_Md_Type *lp_md;
        lv_fserr = xmsg_break_com(lv_msgid,               // msgid
                                  reinterpret_cast<short *>(&lv_results),  // results
                                  &pp_fd->iv_phandle,     // phandle
                                  &lp_md,
                                  false);

        fs_int_fd_op_change_depth(pp_fd, -1);

        if (lv_fserr != XZFIL_ERR_OK) {
            FS_util_io_tag_free(pp_fd, lp_io);
            if (lp_md != NULL)
                fs_int_fs_file_return_mem(lp_md);
            if (gv_fs_trace)
                trace_where_printf(WHERE, "W reply, fserr=%d\n", lv_fserr);
            return fs_int_err_fd_rtn(WHERE, pp_fd, lv_fserr);
        }

        TRANSID_COPY(lv_transid, lp_io->iv_transid);
        if (TRANSID_IS_VALID(lv_transid)) {
            // check reinstate
            lv_fserr_temp = static_cast<short>(ms_transid_reinstate(lv_transid, lv_startid));
            if (lv_fserr_temp != XZFIL_ERR_OK)
                lv_fserr = lv_fserr_temp;
        }
        FS_util_io_tag_free(pp_fd, lp_io);
        if (pp_count_written != NULL)
            *pp_count_written = lp_w_ctrl_rep->countwritten;
        if (gv_fs_trace)
            trace_where_printf(WHERE, "W reply, dtype=%d(%s), rtype=%d(%s), rvers=%d, fserr=%d, cw=%d\n",
                               lp_w_ctrl_rep->dialect_type,
                               fs_int_fs_get_dialect_type(lp_w_ctrl_rep->dialect_type),
                               lp_w_ctrl_rep->reply_type,
                               fs_int_fs_get_req_type(lp_w_ctrl_rep->reply_type),
                               lp_w_ctrl_rep->reply_version,
                               lp_w_ctrl_rep->error,
                               lp_w_ctrl_rep->countwritten);
        if (lp_w_ctrl_rep->error != XZFIL_ERR_OK)
            lv_fserr = fs_int_err_fd_rtn_assert(WHERE,
                                                pp_fd,
                                                lp_w_ctrl_rep->error,
                                                XZFIL_ERR_TIMEDOUT);
        // Free AFTER looking at reply
        if (lp_md != NULL)
            fs_int_fs_file_return_mem(lp_md);
    } else {
        if (pp_count_written != NULL)
            *pp_count_written = 0;
    }
    return lv_fserr;
}

//
// Purpose: writereadx over ms
//
short fs_int_fs_file_writereadx(FS_Fd_Type  *pp_fd,
                                char        *pp_wbuffer,
                                int          pv_write_count,
                                char        *pp_rbuffer,
                                int          pv_read_count,
                                int         *pp_count_read,
                                SB_Tag_Type  pv_tag,
                                SB_Uid_Type  pv_userid) {
    const char             *WHERE = "XWRITEREADX(internal)";
    char                   *lp_wr_ctrl_req_buf;
    fs_fs_writeread        *lp_wr_ctrl_req;
    char                   *lp_wr_ctrl_rep_buf;
    fs_fs_writeread_reply  *lp_wr_ctrl_rep;
    short                   lv_fserr;
    short                   lv_fserr_temp;
    int                     lv_msgid;
    short                   lv_opts;
    MS_Result_Type          lv_results;
    SB_Transseq_Type        lv_startid;
    SB_Transid_Type         lv_transid;

    switch (pp_fd->iv_file_type) {
    case FS_FILE_TYPE_PROCESS:
        break;
    default:
        return fs_int_err_fd_rtn(WHERE, pp_fd, XZFIL_ERR_INVALOP);
    }
    lv_fserr = fs_int_fd_op_check(WHERE, pp_fd);
    if (lv_fserr != XZFIL_ERR_OK)
        return fs_int_err_fd_rtn(WHERE, pp_fd, lv_fserr);
    lv_fserr = static_cast<short>(ms_transid_get(pp_fd->iv_transid_supp,
                                                 false,
                                                 &lv_transid,
                                                 &lv_startid));
    if (lv_fserr != XZFIL_ERR_OK)
        return fs_int_err_fd_rtn(WHERE, pp_fd, lv_fserr);
    lp_wr_ctrl_req_buf =
      static_cast<char *>(MS_BUF_MGR_ALLOC(sizeof(fs_fs_writeread)));
    lp_wr_ctrl_rep_buf =
      static_cast<char *>(MS_BUF_MGR_ALLOC(sizeof(fs_fs_writeread_reply)));
    lp_wr_ctrl_req = reinterpret_cast<fs_fs_writeread *>(lp_wr_ctrl_req_buf);
#if 0
    // dirty request
    memset(lp_wr_ctrl_req, -1, sizeof(fs_fs_writeread));
#endif
    lp_wr_ctrl_rep = reinterpret_cast<fs_fs_writeread_reply *>(lp_wr_ctrl_rep_buf);
    lp_wr_ctrl_req->dialect_type = DIALECT_FS_FS;
    lp_wr_ctrl_req->request_type = FS_FS_WRITEREAD;
    lp_wr_ctrl_req->request_version = CURRENT_VERSION_FS_FS;
    lp_wr_ctrl_req->minimum_interpretation_version = MINIMUM_VERSION_FS_FS;
    lp_wr_ctrl_req->sender.first_word.filenum = pp_fd->iv_file_num;
    lp_wr_ctrl_req->sender.syncid = 1;
    ms_od_get_my_phandle(reinterpret_cast<SB_Phandle_Type *>(&lp_wr_ctrl_req->sender.phandle));
    lp_wr_ctrl_req->sender.user_openid = 1;
    lp_wr_ctrl_req->sender.id.openid = 1;
    lp_wr_ctrl_req->flags.flags0d = 1;
    TRANSID_COPY(lp_wr_ctrl_req->tcbref, lv_transid);
    TRANSSEQ_COPY(lp_wr_ctrl_req->startid, lv_startid);
    lp_wr_ctrl_req->userid = pv_userid;
    lp_wr_ctrl_req->lid.offset = 1;
    lp_wr_ctrl_req->lid.length = 1;
    FS_Io_Type *lp_io =
      FS_util_io_tag_alloc(pp_fd,
                           FS_FS_WRITEREAD,
                           pv_tag,
                           lv_transid,
                           lv_startid,
                           pp_wbuffer);
    if (gv_fs_trace)
        trace_where_printf(WHERE,
                           "WR req=%p, dtype=%d(%s), rtype=%d(%s), rvers=%d, minvers=%d, io=%ld, fnum=%d, tag=" PFTAG ", uid=%d\n",
                           pfp(lp_wr_ctrl_req),
                           lp_wr_ctrl_req->dialect_type,
                           fs_int_fs_get_dialect_type(lp_wr_ctrl_req->dialect_type),
                           lp_wr_ctrl_req->request_type,
                           fs_int_fs_get_req_type(lp_wr_ctrl_req->request_type),
                           lp_wr_ctrl_req->request_version,
                           lp_wr_ctrl_req->minimum_interpretation_version,
                           reinterpret_cast<long>(lp_io),
                           pp_fd->iv_file_num,
                           pv_tag,
                           pv_userid);
    lv_opts = XMSG_LINK_FSDONEQ;
    if (pp_fd->iv_options & FS_OPEN_OPTIONS_TS)
        lv_opts |= XMSG_LINK_FSDONETSQ;
    lv_fserr = BMSG_LINK_(&pp_fd->iv_phandle,                // phandle
                          &lv_msgid,                         // msgid
                          reinterpret_cast<short *>(lp_wr_ctrl_req),  // reqctrl
                          sizeof(fs_fs_writeread),           // reqctrlsize
                          reinterpret_cast<short *>(lp_wr_ctrl_rep),  // replyctrl
                          sizeof(fs_fs_writeread_reply),     // replyctrlmax
                          pp_wbuffer,                        // reqdata
                          pv_write_count,                    // reqdatasize
                          pp_rbuffer,                        // replydata
                          pv_read_count,                     // replydatamax
                          reinterpret_cast<long>(lp_io),     // linkertag
                          0,                                 // pri
                          0,                                 // xmitclass
                          lv_opts);                          // linkopts
    if (lv_fserr != XZFIL_ERR_OK)
        return fs_int_err_fd_rtn(WHERE, pp_fd, lv_fserr);
    lp_io->iv_msgid = lv_msgid;
    fs_int_fd_op_change_depth(pp_fd, 1);
    if (pp_fd->iv_nowait_depth <= 0) {
        MS_Md_Type *lp_md;
        lv_fserr = xmsg_break_com(lv_msgid,               // msgid
                                  reinterpret_cast<short *>(&lv_results),  // results
                                  &pp_fd->iv_phandle,     // phandle
                                  &lp_md,
                                  false);

        fs_int_fd_op_change_depth(pp_fd, -1);

        if (lv_fserr != XZFIL_ERR_OK) {
            FS_util_io_tag_free(pp_fd, lp_io);
            if (lp_md != NULL)
                fs_int_fs_file_return_mem(lp_md);
            if (gv_fs_trace)
                trace_where_printf(WHERE, "WR reply, fserr=%d\n", lv_fserr);
            return fs_int_err_fd_rtn(WHERE, pp_fd, lv_fserr);
        }

        TRANSID_COPY(lv_transid, lp_io->iv_transid);
        TRANSSEQ_COPY(lv_startid, lp_io->iv_startid);
        if (TRANSID_IS_VALID(lv_transid)) {
            // check reinstate
            lv_fserr_temp = static_cast<short>(ms_transid_reinstate(lv_transid, lv_startid));
            if (lv_fserr_temp != XZFIL_ERR_OK)
                lv_fserr = lv_fserr_temp;
        }
        FS_util_io_tag_free(pp_fd, lp_io);
        if (pp_count_read != NULL)
            *pp_count_read = lv_results.rr_datasize;
        if (gv_fs_trace || (gv_fs_trace_errors && (lp_wr_ctrl_rep->error != XZFIL_ERR_OK)))
            trace_where_printf(WHERE, "WR reply, dtype=%d(%s), rtype=%d(%s), rvers=%d, fserr=%d, cw=%d\n",
                               lp_wr_ctrl_rep->dialect_type,
                               fs_int_fs_get_dialect_type(lp_wr_ctrl_rep->dialect_type),
                               lp_wr_ctrl_rep->reply_type,
                               fs_int_fs_get_req_type(lp_wr_ctrl_rep->reply_type),
                               lp_wr_ctrl_rep->reply_version,
                               lp_wr_ctrl_rep->error,
                               lp_wr_ctrl_rep->countwritten);
        if (lp_wr_ctrl_rep->error != XZFIL_ERR_OK)
            lv_fserr = fs_int_err_fd_rtn_assert(WHERE,
                                                pp_fd,
                                                lp_wr_ctrl_rep->error,
                                                XZFIL_ERR_TIMEDOUT);
        // Free AFTER looking at reply
        if (lp_md != NULL)
            fs_int_fs_file_return_mem(lp_md);
    } else {
        if (pp_count_read != NULL)
            *pp_count_read = 0;
    }
    return lv_fserr;
}

//
// Purpose: format sender
//
void fs_int_fs_format_sender(char *pp_formatted, short *pp_sender) {
    char       la_sender[SB_PHANDLE_NAME_SIZE+1];
    long long *lp_sender = reinterpret_cast<long long *>(pp_sender);
    char      *lp_sender_str = reinterpret_cast<char *>(&pp_sender[2]);
    int        lv_inx;

    for (lv_inx = 0; lv_inx < SB_PHANDLE_NAME_SIZE; lv_inx++) {
        if ((*lp_sender_str >= ' ') && (*lp_sender_str <= 0x7e))
            la_sender[lv_inx] = *lp_sender_str;
        else if (*lp_sender_str == 0)
            break;
        else
            la_sender[lv_inx] = '.';
        lp_sender_str++;
    }
    la_sender[lv_inx] = '\0';

    sprintf(pp_formatted, "%llx.%llx.%llx.%llx.%llx.%llx.%llx.%llx(%s)",
            lp_sender[0],
            lp_sender[1],
            lp_sender[2],
            lp_sender[3],
            lp_sender[4],
            lp_sender[5],
            lp_sender[6],
            lp_sender[7],
            la_sender);
}

//
// Purpose: format startid
//
void fs_int_fs_format_startid(char *pp_formatted, int64 pv_startid) {
    sprintf(pp_formatted, "%lld", pv_startid);
}

//
// Purpose: format tcbref
//
void fs_int_fs_format_tcbref(char *pp_formatted, Tcbref_t *pp_tcbref) {
    sprintf(pp_formatted, "%lld.%lld.%lld.%lld",
            pp_tcbref->id[0],
            pp_tcbref->id[1],
            pp_tcbref->id[2],
            pp_tcbref->id[3]);
}

//
// Purpose: return string that represents dialect
//
const char *fs_int_fs_get_dialect_type(uint16 pv_dialect_type) {
    const char *lp_dialect_type;

    lp_dialect_type =
      SB_get_label(&gv_fs_fsdialect_type_label_map, pv_dialect_type);
    return lp_dialect_type;
}

//
// Purpose: return string that represents request_type
//
const char *fs_int_fs_get_req_type(uint16 pv_request_type) {
    const char *lp_request_type;

    lp_request_type =
      SB_get_label(&gv_fs_fsreq_type_label_map, pv_request_type);
    return lp_request_type;
}

//
// Purpose: get lasterr (thread specific)
//
short *fs_int_lasterr_get() {
    short *lp_lasterr;
    int    lv_status;

    lp_lasterr =
      static_cast<short *>(SB_Thread::Sthr::specific_get(gv_fs_lasterr_tls_inx));
    if (lp_lasterr == NULL) {
        lp_lasterr = new short;
        lv_status = SB_Thread::Sthr::specific_set(gv_fs_lasterr_tls_inx,
                                                  lp_lasterr);
        SB_util_assert_ieq(lv_status, 0);
        *lp_lasterr = XZFIL_ERR_OK;
    }
    return lp_lasterr;
}

//
// Purpose: delete lasterr key
//
void fs_int_lasterr_key_dtor(void *pp_key) {
    short *lp_key;

    lp_key = static_cast<short *>(pp_key);
    delete lp_key;
}

//
// Purpose: fsreq (called from ms)
//
extern "C" void fs_int_process_fsreq(MS_Md_Type *);
void fs_int_process_fsreq(MS_Md_Type *pp_md) {
    const char            *WHERE = "fs_int_process_fsreq";
    fs_fs_close           *lp_close_ctrl_req;
    MS_Md_Type            *lp_md;
    fs_fs_open            *lp_open_ctrl_req;
    SB_Trans::Stream_Base *lp_stream;

    lp_stream = static_cast<SB_Trans::Stream_Base *>(pp_md->ip_stream);
    lp_open_ctrl_req = reinterpret_cast<fs_fs_open *>(pp_md->out.ip_recv_ctrl);
    if (gv_fs_trace)
        trace_where_printf(WHERE,
                           "Received fsreq message, dtype=%d(%s), rtype=%d(%s), rvers=%d, minvers=%d\n",
                           lp_open_ctrl_req->dialect_type,
                           fs_int_fs_get_dialect_type(lp_open_ctrl_req->dialect_type),
                           lp_open_ctrl_req->request_type,
                           fs_int_fs_get_req_type(lp_open_ctrl_req->request_type),
                           lp_open_ctrl_req->request_version,
                           lp_open_ctrl_req->minimum_interpretation_version);

    switch (lp_open_ctrl_req->request_type) {
    case FS_FS_CLOSE:
        SB_Trans::Msg_Mgr::get_md(&lp_md, // fs_int_process_fsreq
                                  lp_stream,
                                  NULL,
                                  true, // send
                                  NULL, // fserr
                                  WHERE,
                                  MD_STATE_MSG_FS_SMSG_CLOSE);
        SB_util_assert_pne(lp_md, NULL); // TODO: can't get md
        lp_close_ctrl_req = static_cast<fs_fs_close *>(MS_BUF_MGR_ALLOC(sizeof(fs_fs_close)));
        // TODO: if can't get buffer
        memcpy(lp_close_ctrl_req, lp_open_ctrl_req, sizeof(fs_fs_close));
        lp_md->iv_tag = -1;
        lp_md->out.iv_ldone = false;
        lp_md->out.iv_nid = gv_ms_su_nid;
        lp_md->out.iv_pid = gv_ms_su_pid;
#ifdef SQ_PHANDLE_VERIFIER
        lp_md->out.iv_verif = gv_ms_su_verif;
#endif
        lp_md->out.iv_recv_req_id = 0;
        lp_md->out.iv_pri = -1;
        lp_md->out.iv_recv_mpi_source_rank = -1;
        lp_md->out.iv_recv_mpi_tag = -1;
        lp_md->out.ip_recv_ctrl = reinterpret_cast<char *>(lp_close_ctrl_req);
        lp_md->out.iv_recv_ctrl_size = sizeof(fs_fs_close);
        lp_md->out.ip_recv_data = NULL;
        lp_md->out.iv_recv_data_size = 0;
        lp_md->out.iv_recv_ctrl_max = 0;
        lp_md->out.iv_recv_data_max = 0;
        lp_md->out.iv_reply = false;

        if (gv_fs_trace)
            trace_where_printf(WHERE, "manufacturing/queueing fsreq close msg\n");
        gv_ms_recv_q.add(&lp_md->iv_link);
        gv_ms_event_mgr.set_event_all(LREQ);

        lp_stream->exec_reply(pp_md,
                              0,                                  // src
                              pp_md->out.iv_recv_mpi_source_rank, // dest
                              pp_md->out.iv_recv_req_id,          // reqid
                              NULL,                               // req_ctrl
                              0,                                  // req_ctrl_size
                              NULL,                               // req_data
                              0,                                  // req_data_size
                              XZFIL_ERR_OK);                      // fserr
        lp_stream->wait_rep_done(pp_md);
        lp_stream->free_reply_md(pp_md);
        break;

    default:
        SB_util_abort("invalid lp_open_ctrl_req->request_type"); // sw fault
        break;
    }
}

//
// Purpose: shutdown (called from ms)
//
extern "C" void fs_int_shutdown_ph1();
void fs_int_shutdown_ph1() {
    const char *WHERE = "fs_int_shutdown_ph1";
    FS_Fd_Type *lp_fd;
    int         lv_filenum;
    int         lv_size;

    Fs_Open_Thread::shutdown();
    if (gv_fs_trace) {
        lv_size = static_cast<int>(gv_fs_filenum_table.get_inuse());
        if (lv_size > 1) // don't count reserved fd
            trace_where_printf(WHERE, "WARNING FDs still inuse=%d\n",
                               lv_size - 1);
        lv_size = static_cast<int>(gv_fs_filenum_table.get_cap());
        for (lv_filenum = 1; lv_filenum < lv_size; lv_filenum++) {
            lp_fd = gv_fs_filenum_table.get_entry(lv_filenum);
            if (lp_fd != NULL) {
                trace_where_printf(WHERE, "fnum=%d, closed=%d, ref-count=%d\n",
                                   lv_filenum,
                                   lp_fd->iv_closed,
                                   lp_fd->iv_ref_count.read_val());
            }
        }
    }
}

//
// Purpose: shutdown (called from ms)
//
extern "C" void fs_int_shutdown_ph2();
void fs_int_shutdown_ph2() {
    const char            *WHERE = "fs_int_shutdown_ph2";
    Map_Fileno_Entry_Type *lp_entry;
    FS_Fd_Type            *lp_fd;
    SB_Imap               *lp_file_map;
    SB_Imap_Enum          *lp_filenum_enum;
    char                  *lp_pname;
    SB_Smap_Enum          *lp_pname_enum;
    bool                   lv_done_filenum;
    bool                   lv_done_pname;
    int                    lv_filenum;
    int                    lv_size;

    lv_size = static_cast<int>(gv_fs_filenum_table.get_cap());
    for (lv_filenum = 0; lv_filenum < lv_size; lv_filenum++) {
        lp_fd = gv_fs_filenum_table.get_entry(lv_filenum);
        if (lp_fd != NULL) {
            if (lp_fd->iv_nowait_open && (lp_fd->iv_ref_count.read_val() == 1)) {
                // fd already locked
                fs_int_fd_free(WHERE, lp_fd);
            } else {
                fs_int_fd_mutex_lock(WHERE, lp_fd);
                lp_fd->iv_ref_count.set_val(1);
                fs_int_fd_free(WHERE, lp_fd);
            }
        }
    }

    // cleanup sender-map
    lv_done_pname = false;
    do {
        lp_pname_enum = gv_fs_sender_map.keys();
        if (lp_pname_enum->more()) {
            lp_pname = lp_pname_enum->next();
            lp_file_map =
              static_cast<SB_Imap *>(gv_fs_sender_map.getv(lp_pname));
            lv_done_filenum = false;
            do {
                lp_filenum_enum = lp_file_map->keys();
                if (lp_filenum_enum->more()) {
                    lv_filenum = lp_filenum_enum->next()->iv_id.i;
                    lp_entry =
                      static_cast<Map_Fileno_Entry_Type *>
                        (lp_file_map->remove(lv_filenum));
                    delete lp_entry;
                } else
                    lv_done_filenum = true;
                delete lp_filenum_enum;
            } while (!lv_done_filenum);
            gv_fs_sender_map.removev(lp_pname);
            delete lp_file_map;
        } else
            lv_done_pname = true;
        delete lp_pname_enum;
    } while (!lv_done_pname);
}
