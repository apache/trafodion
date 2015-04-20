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

#ifndef __SB_FSI_H_
#define __SB_FSI_H_

#include "seabed/int/opts.h"

#include "seabed/int/conv.h"
#include "seabed/int/types.h"

#include "seabed/atomic.h"

#include "slotmgr.h"
#include "util.h"
#include "utilalloc.h"

// internal header file



enum {
    FS_FILE_TYPE_UNKNOWN    = 0,
    FS_FILE_TYPE_RECEIVE    = 1,
    FS_FILE_TYPE_PROCESS    = 2
};
enum  { FS_MAX_FNAME        =  100 };
enum  { FS_MAX_NOWAIT_DEPTH =   16 };
enum  { FS_MAX_RECV_DEPTH   = 4047 };

enum  { FS_OPEN_OPTIONS_NOWAIT      = 0x4000 };
enum  { FS_OPEN_OPTIONS_TS          = 0x0400 };
enum  { FS_OPEN_OPTIONS_NO_SYS_MSGS = 0x0001 };

// file i/o
typedef struct FS_Io_Type {
    bool               iv_inuse;
    struct FS_Io_Type *ip_io_next;
    struct FS_Io_Type *ip_io_prev;
    char              *ip_buffer;
    void              *ip_fd;
    int                iv_io_type;
    int                iv_msgid;
    int                iv_msgid_open;
    SB_Transseq_Type   iv_startid;
    int                iv_tag_io;
    SB_Tag_Type        iv_tag_user;
    SB_Transid_Type    iv_transid;
} FS_Io_Type;

// receive info
typedef struct FS_RI_Type {
    short           iv_io_type;
#ifdef USE_SB_NEW_RI
    int             iv_max_reply_count;
#else
    short           iv_max_reply_count;
#endif
    short           iv_message_tag;
    short           iv_file_number;
    int             iv_sync_id;
    SB_Phandle_Type iv_sender;     // phandle
    short           iv_open_label;
    int             iv_user_id;
} FS_RI_Type;

typedef struct FS_Ru_Type {
    bool               iv_inuse;
    struct FS_Ru_Type *ip_ru_next;
    struct FS_Ru_Type *ip_ru_prev;
    char              *ip_buffer;
    int                iv_msgid;
    int                iv_read_count;
    int                iv_count_written;
    int                iv_io_type;
    bool               iv_read;
    int                iv_ru_tag;
    SB_Transseq_Type   iv_startid;
    SB_Tag_Type        iv_tag;
    SB_Transid_Type    iv_transid;
} FS_Ru_Type;

typedef struct FS_Fd_Type {
    char             ia_fname[FS_MAX_FNAME];
    FS_Io_Type      *ip_io;
    FS_Io_Type      *ip_io_old;              // track oldest/newest i/o
    FS_Io_Type      *ip_io_new;
    SB_Slot_Mgr     *ip_io_tag_mgr;
    FS_Ru_Type      *ip_ru;
    FS_Ru_Type      *ip_ru_old;              // track oldest/newest readupdate
    FS_Ru_Type      *ip_ru_new;
    SB_Slot_Mgr     *ip_ru_tag_mgr;
    bool             iv_inuse;
    bool             iv_closed;
    short            iv_file_num;            // file number
    int              iv_file_type;
    short            iv_lasterr;             // lasterr
    SB_Thread::ECM   iv_mutex;
    int              iv_nowait_depth;        // nowait-depth from open
    bool             iv_nowait_open;         // nowait open?
    bool             iv_nowait_open_cancel;  // nowait open cancel?
    FS_Io_Type       iv_nowait_open_io;
    SB_Thread::ECM   iv_nowait_open_mutex;
    int              iv_oid;
    int              iv_op_depth;            // how deep are we?
    int              iv_options;             // nowait-depth from open
    SB_Phandle_Type  iv_phandle;
    bool             iv_phandle_initialized; // phandle was initialized
    int              iv_recv_depth;          // recv-depth from open
    SB_Atomic_Int    iv_ref_count;
    int              iv_ru_curr_tag;
    bool             iv_transid_supp;
} FS_Fd_Type;

// use defines below to use this object

//
// Use the destructor property to cleanup an fd-lock
// when a this object goes out of scope.
//
class FS_Fd_Scoped_Mutex {
public:
    FS_Fd_Scoped_Mutex(const char *pp_where, FS_Fd_Type *pp_fd);
    ~FS_Fd_Scoped_Mutex();

    void forget();
    void lock();
    void unlock();

private:
    FS_Fd_Type *ip_fd;
    const char *ip_where;
    bool        iv_locked;
};
#define FS_FD_SCOPED_MUTEX(where,var,fd) \
FS_Fd_Scoped_Mutex var(where, fd)
#define FS_FD_SCOPED_MUTEX_FORGET(var) \
var.forget()



extern const char   *gp_fs_receive_fname;
extern char         *gp_fs_trace_file;
extern char         *gp_fs_trace_file_dir;
extern char         *gp_fs_trace_prefix;
extern bool          gv_fs_assert_error;
extern short         gv_fs_receive_fn;

extern short         fs_int_err_fd_rtn(const char *pp_where,
                                       FS_Fd_Type *pp_fd,
                                       short       pv_fserr);
extern short         fs_int_err_fd_rtn_assert(const char *pp_where,
                                              FS_Fd_Type *pp_fd,
                                              short       pv_fserr,
                                              short       pv_fserr_ok);
extern short         fs_int_err_lasterr(short pv_fserr);
extern short         fs_int_err_rtn(const char *pp_where, short pv_fserr);
extern short         fs_int_err_rtn_assert(const char *pp_where,
                                           short pv_fserr, short pv_fserr_ok);
extern short         fs_int_err_rtn_badname(const char *pp_where,
                                            const char *pp_filename,
                                            int         pv_length);
extern short         fs_int_err_rtn_msg(const char *pp_where,
                                        const char *pp_msg,
                                        short       pv_fserr);
extern short         fs_int_err_rtn_notopen(const char *pp_where,
                                            int         pv_filenum);
extern short         fs_int_err_rtn_toomany(const char *pp_where);
extern FS_Fd_Type   *fs_int_fd_alloc(int pv_recv_depth, int pv_nowait_depth);
extern int           fs_int_fd_cleanup_enable();
extern void          fs_int_fd_free(const char *pp_where, FS_Fd_Type *pp_fd);
extern void          fs_int_fd_init();
extern FS_Fd_Type   *fs_int_fd_map_filenum_to_fd(int pv_filenum, bool pv_lock);
extern void          fs_int_fd_mutex_lock(const char *pp_where,
                                          FS_Fd_Type *pp_fd);
extern void          fs_int_fd_mutex_unlock(const char *pp_where,
                                            FS_Fd_Type *pp_fd);
extern int           fs_int_fd_ref_count_dec(FS_Fd_Type *pp_fd);
extern int           fs_int_fd_ref_count_inc(FS_Fd_Type *pp_fd, int pv_inc);
extern short         fs_int_fd_setup(const char *pp_where,
                                     const char *pp_filename,
                                     int         pv_length,
                                     int         pv_nowait_depth,
                                     int         pv_options,
                                     FS_Fd_Type *pp_fd);
extern void          fs_int_fd_table_mutex_lock(const char *pp_where);
extern void          fs_int_fd_table_mutex_unlock(const char *pp_where);
extern short         fs_int_fs_file_awaitiox(short        *pp_filenum,
                                             void        **ppp_buf,
                                             int          *pp_xfercount,
                                             SB_Tag_Type  *pp_tag,
                                             int           pv_timeout,
                                             short        *pp_segid,
                                             bool          pv_int,
                                             bool          pv_ts);
extern void          fs_int_fs_file_cancel_nowait_open(FS_Fd_Type *pp_fd);
extern short         fs_int_fs_file_cancelreq(FS_Fd_Type  *pp_fd,
                                              SB_Tag_Type  pv_tag);
extern void          fs_int_fs_file_cancelreq_all(FS_Fd_Type *pp_fd);
extern short         fs_int_fs_file_close(FS_Fd_Type *pp_fd);
extern short         fs_int_fs_file_getri(FS_RI_Type *pp_ri);
extern short         fs_int_fs_file_open(FS_Fd_Type  *pp_fd,
                                         const char  *pp_filename,
                                         bool         pv_self);
extern short         fs_int_fs_file_readx(FS_Fd_Type  *pp_fd,
                                          int          pv_ru_tag,
                                          int         *pp_msgid,
                                          int         *pp_length,
                                          char       **pp_buffer,
                                          int          pv_read_count);
extern short         fs_int_fs_file_readupdatex(FS_Fd_Type  *pp_fd,
                                                int          pv_ru_tag,
                                                int         *pp_msgid,
                                                int         *pp_length,
                                                char       **pp_buffer,
                                                int          pv_read_count);
extern short         fs_int_fs_file_replyx(FS_Fd_Type      *pp_fd,
                                           int              pv_msgid,
                                           char            *pp_buffer,
                                           int              pv_write_count,
                                           int              pv_count_written,
                                           int              pv_io_type,
                                           short            pv_err_ret,
                                           SB_Transid_Type  pv_transid,
                                           SB_Transseq_Type pv_startid);
extern void          fs_int_fs_file_replyx_auto(FS_Fd_Type *pp_fd);
extern short         fs_int_fs_file_setmode(FS_Fd_Type *pp_fd,
                                            short       pv_modenum,
                                            int         pv_parm1,
                                            int         pv_parm2,
                                            short      *pp_oldval);
extern short         fs_int_fs_file_writex(FS_Fd_Type  *pp_fd,
                                           char        *pp_buffer,
                                           int          pv_write_count,
                                           int         *pp_count_written,
                                           SB_Tag_Type  pv_tag,
                                           SB_Uid_Type  pv_userid);
extern short         fs_int_fs_file_writereadx(FS_Fd_Type  *pp_fd,
                                               char        *pp_wbuffer,
                                               int          pv_write_count,
                                               char        *pp_rbuffer,
                                               int          pv_read_count,
                                               int         *pp_count_read,
                                               SB_Tag_Type  pv_tag,
                                               SB_Uid_Type  pv_userid);
extern void          fs_int_fs_format_sender(char  *pp_formatted,
                                             short *pp_sender);
extern short        *fs_int_lasterr_get();

#endif // !__SB_FSI_H_
