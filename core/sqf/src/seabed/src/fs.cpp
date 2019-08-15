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

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include "seabed/fs.h"
#include "seabed/ms.h"
#include "seabed/trace.h"

#include "apictr.h"
#include "buf.h"
#include "chk.h"
#include "env.h"
#include "fsi.h"
#include "fsx.h"
#include "fstrace.h"
#include "fsutil.h"
#include "msmon.h"
#include "mstrace.h"
#include "msx.h"
#include "trans.h"
#include "util.h"
#include "utracex.h"


enum { FS_TAG                               = 10 }; // same as MS


static bool          gv_fs_inited           = false;
static bool          gv_fs_init_test        = false;



extern "C" const char *libsbfs_vers2_str();

// forwards
static int file_init_com(int    *pp_argc,
                         char ***pppp_argv,
                         bool    pv_mpi_init,
                         bool    pv_attach,
                         bool    pv_forkexec,
                         char   *pp_name) SB_THROWS_FATAL;
static short BFILE_OPEN_com(char            *pp_filename,
                            short            pv_length,
                            short           *pp_filenum,
                            short            pv_access,
                            short            pv_exclusion,
                            short            pv_nowait_depth,
                            short            pv_sync_or_receive_depth,
                            xfat_16          pv_options,
                            short            pv_seq_block_buffer_id,
                            short            pv_seq_block_buffer_length,
                            SB_Phandle_Type *pp_primary_processhandle,
                            bool             pv_self);
static _bcc_status BWRITEREADX_com(short        pv_filenum,
                                   char        *pp_wbuffer,
                                   int          pv_write_count,
                                   char        *pp_rbuffer,
                                   int          pv_read_count,
                                   int         *pp_count_read,
                                   SB_Tag_Type  pv_tag,
                                   SB_Uid_Type  pv_userid);


#define RETURNFSCC(fserr) \
return (fserr >= XZFIL_ERR_BADERR) ? -1 : (fserr == XZFIL_ERR_OK) ? 0 : fserr;

//
// Purpose: buf options
//
SB_Export short file_buf_options(int pv_options) {
    return msg_buf_options(pv_options);
}

//
// Purpose: buf readupdatex
//          (a near clone of BREADUPDATEX)
//
SB_Export _bcc_status file_buf_readupdatex(short         pv_filenum,
                                           char        **ppp_buffer,
                                           int          *pp_count_read,
                                           SB_Tag_Type   pv_tag) {
    const char *WHERE = "file_buf_readupdatex";
    FS_Fd_Type *lp_fd;
    FS_Ru_Type *lp_ru;
    int         lv_len;
    short       lv_fserr;
    int         lv_msgid;
    int         lv_ru_tag;
    SB_API_CTR (lv_zctr, FILE_BUF_READUPDATEX);

    if (pv_tag == XOMITTAG)
        pv_tag = 0;
    SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_FS_BUF_READUPDATEX, pv_filenum, pv_tag);
    if (gv_fs_trace_params)
        trace_where_printf(WHERE,
                           "ENTER fnum=%d, cr=%p, tag=" PFTAG "\n",
                           pv_filenum, pfp(pp_count_read), pv_tag);
    if (!gv_fs_inited)
        RETURNFSCC(fs_int_err_rtn_msg(WHERE, "file_init() not called",
                                      XZFIL_ERR_INVALIDSTATE));
    lp_fd = fs_int_fd_map_filenum_to_fd(pv_filenum, true); // file_buf_readupdatex
    FS_FD_SCOPED_MUTEX(WHERE, lv_smutex, lp_fd);
    if (lp_fd == NULL) {
        lv_fserr = fs_int_err_rtn_notopen(WHERE, pv_filenum);
        RETURNFSCC(lv_fserr);
    }
    if (pv_filenum != gv_fs_receive_fn) {
        lv_fserr = fs_int_err_rtn_msg(WHERE,
                                      "filenum not $RECEIVE file",
                                      XZFIL_ERR_INVALOP);
        RETURNFSCC(lv_fserr);
    }
    if (lp_fd->iv_recv_depth <= 0) {
        lv_fserr = fs_int_err_rtn_msg(WHERE,
                                      "receive-depth is 0",
                                      XZFIL_ERR_INVALOP);
        RETURNFSCC(lv_fserr);
    }
    lv_ru_tag = FS_util_ru_tag_alloc(lp_fd, false);
    if (lv_ru_tag < 0) {
        lv_fserr = fs_int_err_rtn_msg(WHERE,
                                      "too many readupdate's already",
                                      XZFIL_ERR_TOOMANY);
        RETURNFSCC(lv_fserr);
    }
    lp_fd->iv_ru_curr_tag = lv_ru_tag;
    lp_ru = &lp_fd->ip_ru[lv_ru_tag];
    lp_ru->ip_buffer = NULL;
    lp_ru->iv_read_count = 0;
    lp_ru->iv_count_written = 0;
    lp_ru->iv_io_type = -1;
    lp_ru->iv_read = false;
    lp_ru->iv_tag = pv_tag;
    lp_ru->iv_msgid = -1;
    lv_fserr = fs_int_fs_file_readupdatex(lp_fd, lv_ru_tag, &lv_msgid, &lv_len,
                                          ppp_buffer, -1);
    if (pp_count_read != NULL)
        *pp_count_read = lv_len;
    if (gv_fs_trace_data && (lp_fd->iv_nowait_depth <= 0)) {
        trace_where_printf(WHERE,
                           "buf=%p, cr=%d\n",
                           *ppp_buffer, lv_len);
        trace_print_data(*ppp_buffer, lv_len, gv_fs_trace_data_max);
    }
    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "EXIT cr=%d, ret=%d\n", lv_len, lv_fserr);
    fs_int_err_fd_rtn_assert(WHERE, lp_fd, lv_fserr, XZFIL_ERR_SYSMESS);
    RETURNFSCC(lv_fserr);
}

//
// Purpose: buf register
//
SB_Export short file_buf_register(FS_Buf_Alloc_Cb_Type pv_callback_alloc,
                                  FS_Buf_Free_Cb_Type  pv_callback_free) {
    return msg_buf_register(pv_callback_alloc, pv_callback_free);
}

//
// Purpose: debug hook
//
SB_Export void file_debug_hook(const char *pp_who, const char *pp_fname) {
    msg_debug_hook(pp_who, pp_fname);
}

//
// Purpose: set open-cleanup
//
SB_Export int file_enable_open_cleanup() {
    const char *WHERE = "file_enable_open_cleanup";
    int         lv_fserr;
    SB_API_CTR (lv_zctr, FILE_ENABLE_OPEN_CLEANUP);

    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "ENTER\n");
    lv_fserr = fs_int_fd_cleanup_enable();
    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return lv_fserr;
}

#ifdef USE_EVENT_REG
SB_Export int file_event_deregister(short pv_event) {
    return proc_event_deregister(pv_event);
}

SB_Export void file_event_disable_abort() {
    proc_event_disable_abort();
}

SB_Export int file_event_register(short pv_event) {
    return proc_event_register(pv_event);
}
#endif

//
// Purpose: initialize file module
//
SB_Export int file_init(int *pp_argc, char ***pppp_argv)
SB_THROWS_FATAL {
    SB_API_CTR (lv_zctr, FILE_INIT);
    return file_init_com(pp_argc,
                         pppp_argv,
                         true,
                         false,
                         false,
                         NULL);
}

//
// Purpose: initialize file module (attach)
//
SB_Export int file_init_attach(int    *pp_argc,
                               char ***pppp_argv,
                               int     pv_forkexec,
                               char   *pp_name)
SB_THROWS_FATAL {
    SB_API_CTR (lv_zctr, FILE_INIT_ATTACH);
     return file_init_com(pp_argc,
                          pppp_argv,
                          true,
                          true,
                          pv_forkexec,
                          pp_name);
}

//
// Purpose: initialize file module
//
int file_init_com(int    *pp_argc,
                  char ***pppp_argv,
                  bool    pv_mpi_init,
                  bool    pv_attach,
                  bool    pv_forkexec,
                  char   *pp_name)
SB_THROWS_FATAL {
    const char *WHERE = "file_init";
    char        la_host[200];
    char        la_unique[200];
    const char *lp_p;

    if (gv_fs_inited && !gv_fs_init_test)
        return fs_int_err_rtn_msg(WHERE, "EXIT already initialized",
                                  XZFIL_ERR_INVALIDSTATE);

    // do this here, so we can trace
    msg_init_env(*pp_argc, *pppp_argv);

    // initialize trace subsystem
    ms_getenv_bool(gp_fs_env_assert_error, &gv_fs_assert_error);

    ms_getenv_bool(gp_fs_env_trace_enable, &gv_fs_trace_enable);
    if (gv_fs_trace_enable) {
        ms_getenv_bool(gp_fs_env_trace, &gv_fs_trace);
        ms_getenv_bool(gp_fs_env_trace_data, &gv_fs_trace_data);
        ms_getenv_int(gp_fs_env_trace_data_max, &gv_fs_trace_data_max);
        ms_getenv_bool(gp_fs_env_trace_detail, &gv_fs_trace_detail);
        ms_getenv_bool(gp_fs_env_trace_errors, &gv_fs_trace_errors);
        lp_p = ms_getenv_str(gp_fs_env_trace_file_dir);
        if (lp_p != NULL) {
            delete [] gp_fs_trace_file_dir;
            gp_fs_trace_file_dir = new char[strlen(lp_p) + 1];
            strcpy(gp_fs_trace_file_dir, lp_p);
        }
        ms_getenv_longlong(gp_fs_env_trace_file_maxsize,
                           &gv_fs_trace_file_maxsize);
        ms_getenv_bool(gp_fs_env_trace_file_nolock, &gv_fs_trace_file_nolock);
        ms_getenv_int(gp_fs_env_trace_file_unique, &gv_fs_trace_file_unique);
        lp_p = ms_getenv_str(gp_fs_env_trace_file);
        if (lp_p != NULL) {
            delete [] gp_fs_trace_file;
            if (gv_fs_trace_file_unique < 0) {
                gethostname(la_host, sizeof(la_host));
                sprintf(la_unique, "%s.%s.", lp_p, la_host);
                lp_p = la_unique;
            }
            if (gp_fs_trace_file_dir == NULL) {
                gp_fs_trace_file = new char[strlen(lp_p) + 1];
                strcpy(gp_fs_trace_file, lp_p);
            } else {
                gp_fs_trace_file =
                  new char[strlen(gp_fs_trace_file_dir) + strlen(lp_p) + 2];
                sprintf(gp_fs_trace_file, "%s/%s", gp_fs_trace_file_dir, lp_p);
            }
        }
        ms_getenv_bool(gp_fs_env_trace_file_delta, &gv_fs_trace_file_delta);
        ms_getenv_int(gp_fs_env_trace_file_fb, &gv_fs_trace_file_fb);
        if (gv_fs_trace_file_fb > 0)
            gv_fs_trace_file_maxsize = 0; // turn off maxsize!
        // note both ms and fs below
        ms_getenv_bool(gp_fs_env_trace_mon, &gv_fs_trace_mon);
        ms_getenv_bool(gp_fs_env_trace_mon, &gv_ms_trace_mon);
        ms_getenv_bool(gp_fs_env_trace_mt, &gv_fs_trace_mt);
        ms_getenv_bool(gp_fs_env_trace_params, &gv_fs_trace_params);
        ms_getenv_bool(gp_fs_env_trace_params0, &gv_fs_trace_params0);
        lp_p = ms_getenv_str(gp_fs_env_trace_prefix);
        if (lp_p != NULL) {
            delete [] gp_fs_trace_prefix;
            gp_fs_trace_prefix = new char[strlen(lp_p) + 1];
            strcpy(gp_fs_trace_prefix, lp_p);
        }
        if ((gv_fs_trace || gv_fs_trace_params) && !gv_fs_trace_params0)
            gv_fs_trace_errors = true;
        ms_getenv_bool(gp_fs_env_trace_verbose, &gv_fs_trace_verbose);
    }

    trace_set_assert_no_trace(gv_fs_assert_error);
    if (gv_fs_trace_enable) {
        trace_set_delta(gv_fs_trace_file_delta);
        if (gv_fs_trace_file_nolock)
            trace_set_lock(!gv_fs_trace_file_nolock);
        trace_init2(gp_fs_trace_file,
                    gv_fs_trace_file_unique,
                    gp_fs_trace_prefix,
                    false,
                    gv_fs_trace_file_maxsize);
    }
    if (gv_fs_trace_file_fb > 0)
        trace_set_mem(gv_fs_trace_file_fb);
    if (gv_fs_trace_enable) {
        SB_Buf_Line la_line;
        trace_printf("%s\n", libsbfs_vers2_str());
        trace_where_printf(WHERE,
                           "SEABED module version %s\n",
                            ms_seabed_vers());
        SB_util_get_cmdline(0,
                            true, // args
                            la_line,
                            sizeof(la_line));
        trace_where_printf(WHERE, "cmdline=%s\n", la_line);
        sprintf(la_line, "argv=");
        msg_trace_args(la_line, *pppp_argv, *pp_argc, sizeof(la_line));
        trace_where_printf(WHERE, "%s\n", la_line);
    }
    fs_int_fd_init();

    short lv_fserr;
    if (gv_ms_inited && !gv_ms_init_test)
        lv_fserr = XZFIL_ERR_OK; // already inited - all is well
    else {
        lv_fserr = static_cast<short>(
          msg_init_com(pp_argc,
                       pppp_argv,
                       pv_mpi_init,
                       pv_attach,
                       pv_forkexec,
                       pp_name,
                       true));
    }
    if (!pv_mpi_init)
        gv_fs_init_test = true;
    if (lv_fserr == XZFIL_ERR_OK)
        gv_fs_inited = true;
    fs_int_err_rtn_msg(WHERE, "file_init()", lv_fserr);
    return lv_fserr;
}

//
// Purpose: handle process close
//
SB_Export int file_mon_process_close() {
    return msg_mon_process_close();
}

//
// Purpose: handle process shutdown
//
SB_Export int file_mon_process_shutdown() {
    return msg_mon_process_shutdown();
}

//
// Purpose: handle process shutdown (now)
//
SB_Export void file_mon_process_shutdown_now() {
    msg_mon_process_shutdown_now();
}

//
// Purpose: handle process startup
//
SB_Export int file_mon_process_startup(int pv_sysmsgs)
SB_THROWS_FATAL {
    return msg_mon_process_startup(pv_sysmsgs);
}

//
// Purpose: handle process startup
//
SB_Export int file_mon_process_startup2(int pv_sysmsgs, int pv_pipeio, bool pv_remap_std_err)
SB_THROWS_FATAL {
  return msg_mon_process_startup3(pv_sysmsgs, pv_pipeio, pv_remap_std_err);
}

//
// Purpose: enable assert
//
SB_Export void file_test_assert_enable(File_AS_Type *pp_state) {
    gv_fs_assert_error = pp_state->assert1;
    msg_test_assert_enable(pp_state->assert2);
}

//
// Purpose: disable assert
//
SB_Export void file_test_assert_disable(File_AS_Type *pp_state) {
    pp_state->assert1 = gv_fs_assert_error;
    gv_fs_assert_error = 0;
    pp_state->assert2 = msg_test_assert_disable();
}

//
// Purpose: init (w/ or w/o mpi_init)
//
SB_Export int file_test_init(int    *pp_argc,
                             char ***pppp_argv,
                             int     pv_mpi_init) {
    return file_init_com(pp_argc,
                         pppp_argv,
                         pv_mpi_init,
                         false,
                         false,
                         NULL);
}


//
// Purpose: emulate ACTIVATERECEIVETRANSID
//
SB_Export _bcc_status BACTIVATERECEIVETRANSID(short pv_msg_num) {
    const char  *WHERE = "BACTIVATERECEIVETRANSID";
    FS_Fd_Type  *lp_fd;
    short        lv_fserr;
    SB_API_CTR  (lv_zctr, BACTIVATERECEIVETRANSID);

    if (!gv_fs_inited)
        RETURNFSCC(fs_int_err_rtn_msg(WHERE, "file_init() not called",
                                      XZFIL_ERR_INVALIDSTATE));
    lv_fserr = XZFIL_ERR_OK;
    lp_fd = fs_int_fd_map_filenum_to_fd(gv_fs_receive_fn, true); // BACTIVATERECEIVETRANSID
    FS_FD_SCOPED_MUTEX(WHERE, lv_smutex, lp_fd);
    if (lp_fd == NULL)
        lv_fserr = XZFIL_ERR_NOTOPEN;
    if (lv_fserr == XZFIL_ERR_OK) {
        if ((pv_msg_num < 0) ||
            (pv_msg_num > lp_fd->iv_recv_depth) ||
            (!lp_fd->ip_ru[pv_msg_num].iv_inuse))
        lv_fserr = XZFIL_ERR_BADREPLY;
    }
    if (lv_fserr == XZFIL_ERR_OK) {
        if (gv_fs_trace_params)
            trace_where_printf(WHERE, "ENTER msg_num=%d\n", pv_msg_num);
        SB_Transseq_Type lv_startid;
        SB_Transid_Type  lv_transid;
        TRANSID_COPY(lv_transid, lp_fd->ip_ru[pv_msg_num].iv_transid);
        TRANSSEQ_COPY(lv_startid, lp_fd->ip_ru[pv_msg_num].iv_startid);
        if (TRANSID_IS_VALID(lv_transid)) {
            lv_fserr = static_cast<short>(ms_transid_reg(lv_transid, lv_startid));
        }
    } else
        fs_int_err_lasterr(lv_fserr);
    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    RETURNFSCC(lv_fserr);
}

//
// Purpose: emulate AWAITIOX
//
SB_Export _bcc_status BAWAITIOX(short        *pp_filenum,
                                void        **ppp_buf,
                                int          *pp_xfercount,
                                SB_Tag_Type  *pp_tag,
                                int           pv_timeout,
                                short        *pp_segid) {
    const char  *WHERE = "BAWAITIOX";
    SB_Buf_Line  la_trace;
    void        *lp_buf;
    short        lv_fserr;
    short        lv_segid;
    SB_Tag_Type  lv_tag;
    int          lv_xfercount;
    SB_API_CTR  (lv_zctr, BAWAITIOX);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_FS_AWAITIOX,
                       (pp_filenum == NULL) ? -1 : *pp_filenum);
    lv_fserr = XZFIL_ERR_OK;
    if (pv_timeout == XOMITINT)
        pv_timeout = -1;

    if (gv_fs_trace_params || gv_fs_trace_params0) {
        short lv_filenum;
        lp_buf = NULL;
        if (pp_filenum == NULL)
            lv_filenum = -1;
        else
            lv_filenum = *pp_filenum;
        if (ppp_buf == NULL)
            ppp_buf = &lp_buf;
        sprintf(la_trace, "ENTER fnum=%p/%d, buf=%p, xc=%p, tag=%p, to=%d\n",
                pfp(pp_filenum), lv_filenum, *ppp_buf,
                pfp(pp_xfercount), pfp(pp_tag), pv_timeout);
        if (!gv_fs_trace_params0)
            trace_where_printf(WHERE, la_trace);
    }
    if (!gv_fs_inited) {
        lv_fserr =
          fs_int_err_rtn_msg(WHERE, "file_init() not called",
                             XZFIL_ERR_INVALIDSTATE);
        if ((pp_filenum != NULL) && (*pp_filenum == -1))
            fs_int_err_lasterr(lv_fserr);
        RETURNFSCC(lv_fserr);
    }

    if (pp_filenum == NULL) {
        lv_fserr =
          fs_int_err_rtn_msg(WHERE, "filenum is NULL", XZFIL_ERR_BOUNDSERR);
        RETURNFSCC(lv_fserr);
    }
    if (*pp_filenum < -2) {
        lv_fserr =
          fs_int_err_rtn_msg(WHERE, "filenum < -2", XZFIL_ERR_NOTOPEN);
        RETURNFSCC(lv_fserr);
    }

    if (pp_tag != NULL)
        lv_tag = *pp_tag;
    lv_fserr = fs_int_fs_file_awaitiox(pp_filenum,
                                       &lp_buf,
                                       &lv_xfercount,
                                       &lv_tag,
                                       pv_timeout,
                                       &lv_segid,
                                       false,  // not internal
                                       false); // not ts
    // so we can trace!
    if (ppp_buf != NULL)
        *ppp_buf = lp_buf;
    if (pp_xfercount != NULL)
        *pp_xfercount = lv_xfercount;
    if (pp_tag != NULL)
        *pp_tag = lv_tag;
    if (pp_segid != NULL)
        *pp_segid = lv_segid;

    if (gv_fs_trace_params0 && (lv_fserr == XZFIL_ERR_OK))
        trace_where_printf(WHERE, la_trace);
    if ((gv_fs_trace_params && !gv_fs_trace_params0) ||
        (gv_fs_trace_params0 && (lv_fserr == XZFIL_ERR_OK)))
        trace_where_printf(WHERE, "EXIT ret=%d, fnum=%d, buf=%p, xc=%d, tag=" PFTAG ", segid=%d\n",
                           lv_fserr, *pp_filenum, lp_buf, lv_xfercount,
                           lv_tag, lv_segid);
    if (pp_filenum != NULL) {
        if (*pp_filenum == -1)
            fs_int_err_lasterr(lv_fserr);
    }
    RETURNFSCC(lv_fserr);
}

//
// Purpose: emulate AWAITIOX (thread-specific)
//
SB_Export _bcc_status BAWAITIOXTS(short        *pp_filenum,
                                  void        **ppp_buf,
                                  int          *pp_xfercount,
                                  SB_Tag_Type  *pp_tag,
                                  int           pv_timeout,
                                  short        *pp_segid) {
    const char  *WHERE = "BAWAITIOXTS";
    SB_Buf_Line  la_trace;
    void        *lp_buf;
    short        lv_fserr;
    short        lv_segid;
    SB_Tag_Type  lv_tag;
    int          lv_xfercount;
    SB_API_CTR  (lv_zctr, BAWAITIOXTS);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_FS_AWAITIOXTS,
                       (pp_filenum == NULL) ? -1 : *pp_filenum);
    lv_fserr = XZFIL_ERR_OK;
    if (pv_timeout == XOMITINT)
        pv_timeout = -1;

    if (gv_fs_trace_params || gv_fs_trace_params0) {
        short lv_filenum;
        lp_buf = NULL;
        if (pp_filenum == NULL)
            lv_filenum = -1;
        else
            lv_filenum = *pp_filenum;
        if (ppp_buf == NULL)
            ppp_buf = &lp_buf;
        sprintf(la_trace, "ENTER fnum=%p/%d, buf=%p, xc=%p, tag=%p, to=%d\n",
                pfp(pp_filenum), lv_filenum, *ppp_buf,
                pfp(pp_xfercount), pfp(pp_tag), pv_timeout);
        if (!gv_fs_trace_params0)
            trace_where_printf(WHERE, la_trace);
    }
    if (!gv_fs_inited) {
        lv_fserr =
          fs_int_err_rtn_msg(WHERE, "file_init() not called",
                             XZFIL_ERR_INVALIDSTATE);
        if ((pp_filenum != NULL) && (*pp_filenum == -1))
            fs_int_err_lasterr(lv_fserr);
        RETURNFSCC(lv_fserr);
    }

    if (pp_filenum == NULL) {
        lv_fserr =
          fs_int_err_rtn_msg(WHERE, "filenum is NULL", XZFIL_ERR_BOUNDSERR);
        RETURNFSCC(lv_fserr);
    }
    if (*pp_filenum < -2) {
        lv_fserr =
          fs_int_err_rtn_msg(WHERE, "filenum < -2", XZFIL_ERR_NOTOPEN);
        RETURNFSCC(lv_fserr);
    }

    if (pp_tag != NULL)
        lv_tag = *pp_tag;
    lv_fserr = fs_int_fs_file_awaitiox(pp_filenum,
                                       &lp_buf,
                                       &lv_xfercount,
                                       &lv_tag,
                                       pv_timeout,
                                       &lv_segid,
                                       false,  // not internal
                                       true);  // ts
    // so we can trace!
    if (ppp_buf != NULL)
        *ppp_buf = lp_buf;
    if (pp_xfercount != NULL)
        *pp_xfercount = lv_xfercount;
    if (pp_tag != NULL)
        *pp_tag = lv_tag;
    if (pp_segid != NULL)
        *pp_segid = lv_segid;

    if (gv_fs_trace_params0 && (lv_fserr == XZFIL_ERR_OK))
        trace_where_printf(WHERE, la_trace);
    if ((gv_fs_trace_params && !gv_fs_trace_params0) ||
        (gv_fs_trace_params0 && (lv_fserr == XZFIL_ERR_OK)))
        trace_where_printf(WHERE, "EXIT ret=%d, fnum=%d, buf=%p, xc=%d, tag=" PFTAG ", segid=%d\n",
                           lv_fserr, *pp_filenum, lp_buf, lv_xfercount,
                           lv_tag, lv_segid);
    if (pp_filenum != NULL) {
        if (*pp_filenum == -1)
            fs_int_err_lasterr(lv_fserr);
    }
    RETURNFSCC(lv_fserr);
}

//
// Purpose: emulate CANCEL
//
SB_Export _bcc_status BCANCEL(short pv_filenum) {
    const char *WHERE = "BCANCEL";
    FS_Fd_Type *lp_fd;
    short       lv_fserr;
    SB_API_CTR (lv_zctr, BCANCEL);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_FS_CANCEL, pv_filenum);
    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "ENTER fnum=%d\n", pv_filenum);
    if (!gv_fs_inited)
        return fs_int_err_rtn_msg(WHERE, "file_init() not called",
                                  XZFIL_ERR_INVALIDSTATE);
    lp_fd = fs_int_fd_map_filenum_to_fd(pv_filenum, true); // BCANCEL
    FS_FD_SCOPED_MUTEX(WHERE, lv_smutex, lp_fd);
    if (lp_fd == NULL)
        return fs_int_err_rtn_notopen(WHERE, pv_filenum);
    lv_fserr = fs_int_fs_file_cancelreq(lp_fd, BOMITTAG);
    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    RETURNFSCC(lv_fserr);
}

//
// Purpose: emulate CANCELREQ
//
SB_Export _bcc_status BCANCELREQ(short pv_filenum, SB_Tag_Type pv_tag) {
    const char *WHERE = "BCANCELREQ";
    FS_Fd_Type *lp_fd;
    short       lv_fserr;
    SB_API_CTR (lv_zctr, BCANCELREQ);

    SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_FS_CANCELREQ, pv_filenum, pv_tag);
    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "ENTER fnum=%d, tag=" PFTAG "\n",
                           pv_filenum, pv_tag);
    if (!gv_fs_inited)
        return fs_int_err_rtn_msg(WHERE, "file_init() not called",
                                  XZFIL_ERR_INVALIDSTATE);
    lp_fd = fs_int_fd_map_filenum_to_fd(pv_filenum, true); // BCANCELREQ
    FS_FD_SCOPED_MUTEX(WHERE, lv_smutex, lp_fd);
    if (lp_fd == NULL)
        return fs_int_err_rtn_notopen(WHERE, pv_filenum);
    lv_fserr = fs_int_fs_file_cancelreq(lp_fd, pv_tag);
    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    RETURNFSCC(lv_fserr);
}

//
// Purpose: emulate FILE_CLOSE_
//
SB_Export short BFILE_CLOSE_(short pv_filenum, short pv_tapedisposition) {
    const char *WHERE = "BFILE_CLOSE_";
    FS_Fd_Type *lp_fd;
    short       lv_fserr;
    int         lv_status;
    int         lv_refcount;
    SB_API_CTR (lv_zctr, BFILE_CLOSE_);

    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_FS_CLOSE, pv_filenum);
    pv_tapedisposition = pv_tapedisposition; // no-warn
    lv_fserr = XZFIL_ERR_OK;
    // ignore tapedisposition XOMITSHORT

    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "ENTER fnum=%d\n", pv_filenum);
    if (!gv_fs_inited)
        return fs_int_err_rtn_msg(WHERE, "file_init() not called",
                                  XZFIL_ERR_INVALIDSTATE);
    lp_fd = fs_int_fd_map_filenum_to_fd(pv_filenum, true); // BFILE_CLOSE_
    FS_FD_SCOPED_MUTEX(WHERE, lv_smutex, lp_fd);
    if (lp_fd == NULL)
        return fs_int_err_rtn_notopen(WHERE, pv_filenum);
    if (pv_filenum == gv_fs_receive_fn)
        gv_fs_receive_fn = -1;
    else {
        if ((lv_refcount = msg_mon_get_ref_count(&lp_fd->iv_phandle)) > 1  || (lv_refcount == 0)) {
            // send close message to remote
            fs_int_fs_file_close(lp_fd);
        }
        if (lp_fd->iv_nowait_open) {
            fs_int_fs_file_cancel_nowait_open(lp_fd);
        } else {
            // if the phandle has been initialized,
            // decrement ref count, if last close, send monitor message
            if(lp_fd->iv_phandle_initialized) {
                lv_status = msg_mon_close_process(&lp_fd->iv_phandle);
                CHK_FEIGNORE(lv_status);
            }
            // cancel outstanding requests
            if ((lp_fd->iv_file_type == FS_FILE_TYPE_PROCESS) &&
                (lp_fd->iv_nowait_depth > 0))
                fs_int_fs_file_cancelreq_all(lp_fd);
        }
    }
    FS_FD_SCOPED_MUTEX_FORGET(lv_smutex);
    fs_int_fd_free(WHERE, lp_fd);
    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    return fs_int_err_rtn(WHERE, lv_fserr);
}


//
// Purpose: emulate FILE_GETINFO_
//
SB_Export short BFILE_GETINFO_(short  pv_filenum,      // IN
                               short *pp_lasterror,    // OUT OPTIONAL
                               char  *pp_filename,     // OUT OPTIONAL
                               short  pv_filename_len, // IN OPTIONAL
                               short *pp_filename_len, // OUT OPTIONAL
                               short *pp_typeinfo,     // OUT OPTIONAL
                                                       // [0:4]
                               short *pp_flags) {      // OUT OPTIONAL
    const char *WHERE = "BFILE_GETINFO_";
    FS_Fd_Type *lp_fd;
    short      *lp_lasterr;
    short       lv_lasterr;
    SB_API_CTR (lv_zctr, BFILE_GETINFO_);

    if (pv_filename_len == XOMITSHORT)
        pv_filename_len = 0;
    if (!gv_fs_trace_params0 && gv_fs_trace_params)
        trace_where_printf(WHERE, "ENTER fnum=%d, lasterr=%p, fname=%p, flen=%d, flen=%p, typeinfo=%p, flags=%p\n",
                           pv_filenum, pfp(pp_lasterror), pfp(pp_filename),
                           pv_filename_len, pfp(pp_filename_len),
                           pfp(pp_typeinfo), pfp(pp_flags));
    if (!gv_fs_inited)
        return fs_int_err_rtn_msg(WHERE, "file_init() not called",
                                  XZFIL_ERR_INVALIDSTATE);
    if (pv_filenum == -1) {
        lp_lasterr = fs_int_lasterr_get();
        lv_lasterr = *lp_lasterr;
        if (pp_lasterror != NULL)
            *pp_lasterror = lv_lasterr;
    } else {
        lp_fd = fs_int_fd_map_filenum_to_fd(pv_filenum, true); // BFILE_GETINFO_
        FS_FD_SCOPED_MUTEX(WHERE, lv_smutex, lp_fd);
        if (lp_fd == NULL) {
            if (!gv_fs_trace_params0 && gv_fs_trace_params)
                trace_where_printf(WHERE, "EXIT lasterr=%d\n", XZFIL_ERR_NOTOPEN);
            return fs_int_err_rtn_notopen(WHERE, pv_filenum);
        }
        lv_lasterr = lp_fd->iv_lasterr;
        if (pp_lasterror != NULL)
            *pp_lasterror = lv_lasterr;
    }
    if (!gv_fs_trace_params0 && gv_fs_trace_params)
        trace_where_printf(WHERE, "EXIT lasterr=%d\n", lv_lasterr);
    return XZFIL_ERR_OK; // don't use fs_int_err_rtn (messes up gv_fs_last_err)
}

//
// Purpose: emulate FILE_GETRECEIVEINFO_
//
SB_Export short BFILE_GETRECEIVEINFO_(FS_Receiveinfo_Type *pp_receiveinfo,
                                      short               *pp_reserved) {
    const char *WHERE = "BFILE_GETRECEIVEINFO_";
    SB_API_CTR (lv_zctr, BFILE_GETRECEIVEINFO_);

    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "ENTER receiveinfo=%p, reserved=%p\n",
                           pfp(pp_receiveinfo), pfp(pp_reserved));
    if (!gv_fs_inited)
        return fs_int_err_rtn_msg(WHERE, "file_init() not called",
                                  XZFIL_ERR_INVALIDSTATE);
    FS_Fd_Type *lp_fd = fs_int_fd_map_filenum_to_fd(gv_fs_receive_fn, true); // BFILE_GETRECEIVEINFO_
    FS_FD_SCOPED_MUTEX(WHERE, lv_smutex, lp_fd);
    if (lp_fd == NULL)
        return fs_int_err_rtn_notopen(WHERE, gv_fs_receive_fn);
    if (pp_receiveinfo == NULL)
        return fs_int_err_rtn(WHERE, XZFIL_ERR_BOUNDSERR);
    FS_RI_Type *lp_ri = reinterpret_cast<FS_RI_Type *>(pp_receiveinfo);
    short lv_fserr = fs_int_fs_file_getri(lp_ri);
    if (gv_fs_trace_params) {
        SB_Buf_Line la_sender;
        fs_int_fs_format_sender(la_sender, reinterpret_cast<short *>(&lp_ri->iv_sender));
        trace_where_printf(WHERE, "EXIT ret=%d, io=%d, max-reply=%d, msg-tag=%d, fnum=%d, sync-id=%d, sender=%s, open-label=%d, user-id=%d\n",
                           lv_fserr,
                           lp_ri->iv_io_type,
                           lp_ri->iv_max_reply_count,
                           lp_ri->iv_message_tag,
                           lp_ri->iv_file_number,
                           lp_ri->iv_sync_id,
                           la_sender,
                           lp_ri->iv_open_label,
                           lp_ri->iv_user_id);
    }
    return lv_fserr; // don't use fs_int_err_rtn (messes up gv_fs_last_err)
}

//
// Purpose: emulate FILE_OPEN_
//
SB_Export short BFILE_OPEN_(char            *pp_filename,
                            short            pv_length,
                            short           *pp_filenum,
                            short            pv_access,
                            short            pv_exclusion,
                            short            pv_nowait_depth,
                            short            pv_sync_or_receive_depth,
                            xfat_16          pv_options,
                            short            pv_seq_block_buffer_id,
                            short            pv_seq_block_buffer_length,
                            SB_Phandle_Type *pp_primary_processhandle) {
    SB_API_CTR (lv_zctr, BFILE_OPEN_);
    return BFILE_OPEN_com(pp_filename,
                          pv_length,
                          pp_filenum,
                          pv_access,
                          pv_exclusion,
                          pv_nowait_depth,
                          pv_sync_or_receive_depth,
                          pv_options,
                          pv_seq_block_buffer_id,
                          pv_seq_block_buffer_length,
                          pp_primary_processhandle,
                          false);
}

short BFILE_OPEN_com(char            *pp_filename,
                     short            pv_length,
                     short           *pp_filenum,
                     short            pv_access,
                     short            pv_exclusion,
                     short            pv_nowait_depth,
                     short            pv_sync_or_receive_depth,
                     xfat_16          pv_options,
                     short            pv_seq_block_buffer_id,
                     short            pv_seq_block_buffer_length,
                     SB_Phandle_Type *pp_primary_processhandle,
                     bool             pv_self) {
    const char *WHERE = "BFILE_OPEN_";
    char        la_filename[MS_MON_MAX_PROCESS_NAME+1];
    FS_Fd_Type *lp_fd;
    short       lv_fserr;
    short       lv_lfserr;
    bool        lv_receive;
    short       lv_recvdep;
    short       lv_recvlim;

    if (!gv_fs_inited)
        return fs_int_err_rtn_msg(WHERE, "file_init() not called",
                                  XZFIL_ERR_INVALIDSTATE);
    if (pv_access == XOMITSHORT)
        pv_access = 0;
    if (pv_exclusion == XOMITSHORT)
        pv_exclusion = 0;
    if (pv_nowait_depth == XOMITSHORT)
        pv_nowait_depth = 0;
    if (pv_nowait_depth < 0)
        pv_nowait_depth = 0;
    if (pv_sync_or_receive_depth == XOMITSHORT)
        pv_sync_or_receive_depth = 0;
    if (pv_options == XOMITFAT_16)
        pv_options = 0;
    if (pv_seq_block_buffer_id == XOMITSHORT)
        pv_seq_block_buffer_id = 0;
    if (pv_seq_block_buffer_length == XOMITSHORT)
        pv_seq_block_buffer_length = 0;
    if (gv_fs_trace_params) {
        char la_tfilename[PATH_MAX];
        if (pp_filename == NULL)
            strcpy(la_tfilename, "(nil)");
        else
            strncpy(la_tfilename, pp_filename, PATH_MAX - 1);
        trace_where_printf(WHERE,
                           "ENTER fname=%p(%s), length=%d, fnum=%p, access=%d, exclusion=%d\n",
                           pp_filename,
                           la_tfilename,
                           pv_length,
                           pfp(pp_filenum),
                           pv_access,
                           pv_exclusion);
        trace_where_printf(WHERE,
                           "ENTER nowait_depth=%d, sync_or_receive_depth=%d, options=0x%x, seq_block_buffer_id=%d\n",
                           pv_nowait_depth,
                           pv_sync_or_receive_depth,
                           pv_options,
                           pv_seq_block_buffer_id);
        trace_where_printf(WHERE,
                           "ENTER seq_block_buffer_length=%d, primary_processhandle=%p\n",
                           pv_seq_block_buffer_length,
                           pfp(pp_primary_processhandle));
    }
    if (pp_filename == NULL)
        return fs_int_err_rtn_msg(WHERE,
                                  "filename is NULL",
                                  XZFIL_ERR_BADNAME);

    SB_util_get_case_insensitive_name(pp_filename, la_filename);

    lv_receive = (strcmp(la_filename, gp_fs_receive_fname) == 0);
    if (lv_receive) {
        if (pv_sync_or_receive_depth > FS_MAX_RECV_DEPTH)
            return fs_int_err_rtn_toomany(WHERE);
    } else {
        if (pv_nowait_depth > FS_MAX_NOWAIT_DEPTH)
            return fs_int_err_rtn_msg(WHERE,
                                      "nowait-depth too large",
                                      XZFIL_ERR_BOUNDSERR);
    }
    if (pv_options & FS_OPEN_OPTIONS_TS)
        fs_int_fd_cleanup_enable();
    lp_fd = fs_int_fd_alloc(lv_receive ? pv_sync_or_receive_depth : 0, // BFILE_OPEN_*
                            pv_nowait_depth);
    if (lp_fd == NULL)
        return fs_int_err_rtn_msg(WHERE,
                                  "no more file descriptors available",
                                  XZFIL_ERR_NOBUFSPACE);
    FS_FD_SCOPED_MUTEX(WHERE, lv_smutex, lp_fd);

    lv_fserr = fs_int_fd_setup(WHERE, la_filename, pv_length,
                               pv_nowait_depth, pv_options, lp_fd);
    SB_UTRACE_API_ADD2(SB_UTRACE_API_OP_FS_OPEN, lp_fd->iv_file_num);
    if (lv_fserr != XZFIL_ERR_OK) {
        FS_FD_SCOPED_MUTEX_FORGET(lv_smutex);
        fs_int_fd_free(WHERE, lp_fd);
        if (gv_fs_trace_params)
            trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
        return lv_fserr;
    }
    if (lv_receive) {
        gv_fs_receive_fn = lp_fd->iv_file_num;
        if ((pv_options & FS_OPEN_OPTIONS_NO_SYS_MSGS) == 0) {
            msg_mon_enable_mon_messages(1);
            if (!gv_ms_su_sysmsgs) {
                if (gv_fs_trace_params)
                    trace_where_printf(WHERE, "*** WARNING *** Opening $RECEIVE with 'system messages', but file_mon_process_startup(false) was called - no system messages will ever be received\n");
            }
        }
        lv_recvdep = static_cast<short>(pv_sync_or_receive_depth + 10);
        if (lv_recvdep > 255) {
            lv_lfserr = XMESSAGESYSTEMINFO(XMSGSYSINFO_RECVLIMIT, &lv_recvlim);
            if ((lv_lfserr == XZFIL_ERR_OK) && (lv_recvdep > lv_recvlim)) {
                lv_fserr = XCONTROLMESSAGESYSTEM(XCTLMSGSYS_SETRECVLIMIT, lv_recvdep);
                CHK_FEIGNORE(lv_fserr);
            }

        }
    } else {
        lv_fserr = fs_int_fs_file_open(lp_fd, la_filename, pv_self);
        if (lv_fserr != XZFIL_ERR_OK) {
            FS_FD_SCOPED_MUTEX_FORGET(lv_smutex);
            fs_int_fd_free(WHERE, lp_fd);
            if (gv_fs_trace_params)
                trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
            return fs_int_err_fd_rtn(WHERE, NULL, lv_fserr);
        }
    }
    *pp_filenum = lp_fd->iv_file_num;
    lv_fserr = XZFIL_ERR_OK;
    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d, fnum=%d\n",
                           lv_fserr,
                           *pp_filenum);
    return fs_int_err_fd_rtn(WHERE, lp_fd, lv_fserr);
}

SB_Export short BFILE_OPEN_SELF_(short           *pp_filenum,
                                 short            pv_access,
                                 short            pv_exclusion,
                                 short            pv_nowait_depth,
                                 short            pv_sync_or_receive_depth,
                                 xfat_16          pv_options,
                                 short            pv_seq_block_buffer_id,
                                 short            pv_seq_block_buffer_length,
                                 SB_Phandle_Type *pp_primary_processhandle) {
    SB_API_CTR (lv_zctr, BFILE_OPEN_SELF_);
    return BFILE_OPEN_com(ga_ms_su_pname,
                          static_cast<short>(strlen(ga_ms_su_pname)),
                          pp_filenum,
                          pv_access,
                          pv_exclusion,
                          pv_nowait_depth,
                          pv_sync_or_receive_depth,
                          pv_options,
                          pv_seq_block_buffer_id,
                          pv_seq_block_buffer_length,
                          pp_primary_processhandle,
                          true); // self
}

//
// Purpose: emulate READX
//
SB_Export _bcc_status BREADX(short        pv_filenum,
                             char        *pp_buffer,
                             int          pv_read_count,
                             int         *pp_count_read,
                             SB_Tag_Type  pv_tag) {
    const char *WHERE = "BREADX";
    FS_Fd_Type *lp_fd;
    FS_Ru_Type *lp_ru;
    char       *lpp_buffer = NULL;
    int         lv_len;
    short       lv_fserr;
    int         lv_msgid;
    int         lv_ru_tag;
    SB_API_CTR (lv_zctr, BREADX);

    if (pv_tag == XOMITTAG)
        pv_tag = 0;
    SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_FS_READX, pv_filenum, pv_tag);
    if (gv_fs_trace_params)
        trace_where_printf(WHERE,
                           "ENTER fnum=%d, buf=%p, Rc=%d, cr=%p, tag=" PFTAG "\n",
                           pv_filenum, pp_buffer, pv_read_count,
                           pfp(pp_count_read), pv_tag);
    if (!gv_fs_inited)
        RETURNFSCC(fs_int_err_rtn_msg(WHERE, "file_init() not called",
                                      XZFIL_ERR_INVALIDSTATE));
    lp_fd = fs_int_fd_map_filenum_to_fd(pv_filenum, true); // BREADX
    FS_FD_SCOPED_MUTEX(WHERE, lv_smutex, lp_fd);
    if (lp_fd == NULL) {
        lv_fserr = fs_int_err_rtn_notopen(WHERE, pv_filenum);
        RETURNFSCC(lv_fserr);
    }
    if (pv_filenum != gv_fs_receive_fn) {
        lv_fserr = fs_int_err_rtn_msg(WHERE,
                                      "filenum not $RECEIVE file",
                                      XZFIL_ERR_INVALOP);
        RETURNFSCC(lv_fserr);
    }
    if (pp_buffer == NULL) {
        lv_fserr = fs_int_err_rtn_msg(WHERE,
                                      "buffer is NULL",
                                      XZFIL_ERR_BOUNDSERR);
        RETURNFSCC(lv_fserr);
    }
    lv_ru_tag = FS_util_ru_tag_alloc(lp_fd, true);
    if (lv_ru_tag < 0) {
        lv_fserr = fs_int_err_rtn_msg(WHERE,
                                      "too many read's already",
                                      XZFIL_ERR_TOOMANY);
        RETURNFSCC(lv_fserr);
    }
    lp_fd->iv_ru_curr_tag = lv_ru_tag;
    lp_ru = &lp_fd->ip_ru[lv_ru_tag];
    lp_ru->ip_buffer = pp_buffer;
    lp_ru->iv_read_count = pv_read_count;
    lp_ru->iv_count_written = 0;
    lp_ru->iv_io_type = -1;
    lp_ru->iv_read = true;
    lp_ru->iv_tag = pv_tag;
    lp_ru->iv_msgid = -1;
    lv_fserr = fs_int_fs_file_readx(lp_fd, lv_ru_tag, &lv_msgid, &lv_len,
                                    &lpp_buffer, pv_read_count);
    if (pp_count_read != NULL)
        *pp_count_read = lv_len;
    if (gv_fs_trace_data && (lp_fd->iv_nowait_depth <= 0)) {
        trace_where_printf(WHERE,
                           "buf=%p, cr=%d\n",
                           pp_buffer, lv_len);
        trace_print_data(pp_buffer, lv_len, gv_fs_trace_data_max);
    }
    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "EXIT cr=%d, ret=%d\n", lv_len, lv_fserr);
    fs_int_err_fd_rtn_assert(WHERE, lp_fd, lv_fserr, XZFIL_ERR_SYSMESS);
    RETURNFSCC(lv_fserr);
}

//
// Purpose: emulate READUPDATEX
//
SB_Export _bcc_status BREADUPDATEX(short        pv_filenum,
                                   char        *pp_buffer,
                                   int          pv_read_count,
                                   int         *pp_count_read,
                                   SB_Tag_Type  pv_tag) {
    const char *WHERE = "BREADUPDATEX";
    FS_Fd_Type *lp_fd;
    FS_Ru_Type *lp_ru;
    char       *lpp_buffer = NULL;
    int         lv_len;
    short       lv_fserr;
    int         lv_msgid;
    int         lv_ru_tag;
    SB_API_CTR (lv_zctr, BREADUPDATEX);

    if (pv_tag == XOMITTAG)
        pv_tag = 0;
    SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_FS_READUPDATEX, pv_filenum, pv_tag);
    if (gv_fs_trace_params)
        trace_where_printf(WHERE,
                           "ENTER fnum=%d, buf=%p, Rc=%d, cr=%p, tag=" PFTAG "\n",
                           pv_filenum, pp_buffer, pv_read_count,
                           pfp(pp_count_read), pv_tag);
    if (!gv_fs_inited)
        RETURNFSCC(fs_int_err_rtn_msg(WHERE, "file_init() not called",
                                      XZFIL_ERR_INVALIDSTATE));
    lp_fd = fs_int_fd_map_filenum_to_fd(pv_filenum, true); // BREADUPDATEX
    FS_FD_SCOPED_MUTEX(WHERE, lv_smutex, lp_fd);
    if (lp_fd == NULL) {
        lv_fserr = fs_int_err_rtn_notopen(WHERE, pv_filenum);
        RETURNFSCC(lv_fserr);
    }
    if (pv_filenum != gv_fs_receive_fn) {
        lv_fserr = fs_int_err_rtn_msg(WHERE,
                                      "filenum not $RECEIVE file",
                                      XZFIL_ERR_INVALOP);
        RETURNFSCC(lv_fserr);
    }
    if (pp_buffer == NULL) {
        lv_fserr = fs_int_err_rtn_msg(WHERE,
                                      "buffer is NULL",
                                      XZFIL_ERR_BOUNDSERR);
        RETURNFSCC(lv_fserr);
    }
    if (lp_fd->iv_recv_depth <= 0) {
        lv_fserr = fs_int_err_rtn_msg(WHERE,
                                      "receive-depth is 0",
                                      XZFIL_ERR_INVALOP);
        RETURNFSCC(lv_fserr);
    }
    lv_ru_tag = FS_util_ru_tag_alloc(lp_fd, false);
    if (lv_ru_tag < 0) {
        lv_fserr = fs_int_err_rtn_msg(WHERE,
                                      "too many readupdate's already",
                                      XZFIL_ERR_TOOMANY);
        RETURNFSCC(lv_fserr);
    }
    lp_fd->iv_ru_curr_tag = lv_ru_tag;
    lp_ru = &lp_fd->ip_ru[lv_ru_tag];
    lp_ru->ip_buffer = pp_buffer;
    lp_ru->iv_read_count = pv_read_count;
    lp_ru->iv_count_written = pv_read_count;
    lp_ru->iv_io_type = -1;
    lp_ru->iv_read = false;
    lp_ru->iv_tag = pv_tag;
    lp_ru->iv_msgid = -1;
    lv_fserr = fs_int_fs_file_readupdatex(lp_fd, lv_ru_tag, &lv_msgid, &lv_len,
                                          &lpp_buffer, pv_read_count);
    if (pp_count_read != NULL)
        *pp_count_read = lv_len;
    if (gv_fs_trace_data && (lp_fd->iv_nowait_depth <= 0)) {
        trace_where_printf(WHERE,
                           "buf=%p, cr=%d\n",
                           pp_buffer, lv_len);
        trace_print_data(pp_buffer, lv_len, gv_fs_trace_data_max);
    }
    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "EXIT cr=%d, (msg-tag=%d), ret=%d\n", lv_len, lv_ru_tag, lv_fserr);
    fs_int_err_fd_rtn_assert(WHERE, lp_fd, lv_fserr, XZFIL_ERR_SYSMESS);
    RETURNFSCC(lv_fserr);
}

//
// Purpose: emulate REPLYX
//
SB_Export _bcc_status BREPLYX(char  *pp_buffer,
                              int    pv_write_count,
                              int   *pp_count_written,
                              short  pv_reply_num,
                              short  pv_err_ret) {
    const char *WHERE = "BREPLYX";
    FS_Fd_Type *lp_fd;
    FS_Ru_Type *lp_ru;
    int         lv_count_written;
    short       lv_fserr;
    int         lv_msgid;
    int         lv_io_type;
    SB_API_CTR (lv_zctr, BREPLYX);

    SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_FS_REPLYX,
                       gv_fs_receive_fn,
                       pv_reply_num);
    if (pv_write_count == XOMITINT)
        pv_write_count = 0;
    if (pv_reply_num == XOMITSHORT)
        pv_reply_num = 0;
    if (pv_err_ret == XOMITSHORT)
        pv_err_ret = XZFIL_ERR_OK;
    if (gv_fs_inited)
        lp_fd = fs_int_fd_map_filenum_to_fd(gv_fs_receive_fn, true); // BREPLYX
    else
        lp_fd = NULL;
    FS_FD_SCOPED_MUTEX(WHERE, lv_smutex, lp_fd);
    if (gv_fs_trace_params) {
        char *lp_to = NULL;
        int   lv_tmsgid;
        if ((lp_fd != NULL) &&
            (pv_reply_num >= 0) &&
            (pv_reply_num <= lp_fd->iv_recv_depth) &&
            (lp_fd->ip_ru[pv_reply_num].iv_inuse)) {
            lv_tmsgid = lp_fd->ip_ru[pv_reply_num].iv_msgid;
            MS_Md_Type *lp_md = SB_Trans::Msg_Mgr::map_to_md(lv_tmsgid, WHERE);
            if (lp_md != NULL) {
                SB_Trans::Stream_Base *lp_stream =
                  reinterpret_cast<SB_Trans::Stream_Base *>(lp_md->ip_stream);
                if (lp_stream != NULL)
                    lp_to = lp_stream->get_name();
            }
        }
        if (lp_to == NULL)
            trace_where_printf(WHERE,
                               "ENTER fnum=%d, buf=%p, Wc=%d\n",
                               gv_fs_receive_fn,
                               pp_buffer,
                               pv_write_count);
        else
            trace_where_printf(WHERE,
                               "ENTER to=%s, msgid=%d, fnum=%d, buf=%p, Wc=%d\n",
                               lp_to,
                               lv_tmsgid,
                               gv_fs_receive_fn,
                               pp_buffer,
                               pv_write_count);
        trace_where_printf(WHERE,
                           "ENTER cw=%p, reply_num=%d, erret=%d\n",
                           pfp(pp_count_written),
                           pv_reply_num,
                           pv_err_ret);
    }
    if (!gv_fs_inited)
        RETURNFSCC(fs_int_err_rtn_msg(WHERE, "file_init() not called",
                                      XZFIL_ERR_INVALIDSTATE));
    if (lp_fd == NULL) {
        lv_fserr = fs_int_err_rtn_notopen(WHERE, gv_fs_receive_fn);
        RETURNFSCC(lv_fserr);
    }
    if (lp_fd->iv_recv_depth <= 0) {
        lv_fserr = fs_int_err_rtn_msg(WHERE,
                                      "receive-depth is 0",
                                      XZFIL_ERR_INVALOP);
        RETURNFSCC(lv_fserr);
    }
    if ((pv_reply_num < 0) ||
        (pv_reply_num >= lp_fd->iv_recv_depth) ||
        (!lp_fd->ip_ru[pv_reply_num].iv_inuse)) {
        lv_fserr = fs_int_err_rtn(WHERE, XZFIL_ERR_BADREPLY);
        RETURNFSCC(lv_fserr);
    }
    if (gv_fs_trace_data) {
        trace_where_printf(WHERE,
                           "buf=%p, Wc=%d\n",
                           pp_buffer, pv_write_count);
        trace_print_data(pp_buffer, pv_write_count, gv_fs_trace_data_max);
    }
    lp_ru = &lp_fd->ip_ru[pv_reply_num];
    lv_msgid = lp_ru->iv_msgid;
    lv_count_written = lp_ru->iv_count_written;
    lv_io_type = lp_ru->iv_io_type;
    SB_Transid_Type lv_transid = lp_ru->iv_transid;
    SB_Transseq_Type lv_startid = lp_ru->iv_startid;
    if (gv_fs_trace)
        trace_where_printf(WHERE, "tag=%d, msgid=%d\n",
                           pv_reply_num, lv_msgid);
    FS_util_ru_tag_free(lp_fd, pv_reply_num);
    lv_fserr = fs_int_fs_file_replyx(lp_fd,
                                     lv_msgid,
                                     pp_buffer,
                                     pv_write_count,
                                     lv_count_written,
                                     lv_io_type,
                                     pv_err_ret,
                                     lv_transid,
                                     lv_startid);
    if (pp_count_written != NULL)
        *pp_count_written = pv_write_count;
    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    fs_int_err_fd_rtn(WHERE, lp_fd, lv_fserr);
    RETURNFSCC(lv_fserr);
}

//
// Purpose: emulate SETMODE
//
SB_Export _bcc_status BSETMODE(short  pv_filenum,
                               short  pv_modenum,
                               int    pv_parm1,
                               int    pv_parm2,
                               short *pp_oldval) {
    const char  *WHERE = "BSETMODE";
    FS_Fd_Type  *lp_fd;
    short        lv_fserr;
    short        lv_oldval;
    SB_API_CTR  (lv_zctr, BSETMODE);

    lv_fserr = XZFIL_ERR_OK;

    if (gv_fs_trace_params)
        trace_where_printf(WHERE,
                           "ENTER fnum=%d, num=%d, parm1=%d, parm2=%d, oldval=%p\n",
                           pv_filenum,
                           pv_modenum,
                           pv_parm1,
                           pv_parm2,
                           pfp(pp_oldval));
    if (!gv_fs_inited)
        RETURNFSCC(fs_int_err_rtn_msg(WHERE, "file_init() not called",
                                      XZFIL_ERR_INVALIDSTATE));
    lp_fd = fs_int_fd_map_filenum_to_fd(pv_filenum, true); // BSETMODE
    FS_FD_SCOPED_MUTEX(WHERE, lv_smutex, lp_fd);
    if (lp_fd == NULL) {
        lv_fserr = fs_int_err_rtn_notopen(WHERE, pv_filenum);
        RETURNFSCC(lv_fserr);
    }
    lv_fserr = fs_int_fs_file_setmode(lp_fd,
                                      pv_modenum,
                                      pv_parm1,
                                      pv_parm2,
                                      &lv_oldval);
    if (pp_oldval != NULL)
        *pp_oldval = lv_oldval;
    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d, oldval=%d\n",
                           lv_fserr, lv_oldval);
    fs_int_err_rtn(WHERE, lv_fserr);
    RETURNFSCC(lv_fserr);
}

//
// Purpose: emulate WRITEX
//
SB_Export _bcc_status BWRITEX(short        pv_filenum,
                              char        *pp_buffer,
                              int          pv_write_count,
                              int         *pp_count_written,
                              SB_Tag_Type  pv_tag,
                              SB_Uid_Type  pv_userid) {
    const char *WHERE = "BWRITEX";
    FS_Fd_Type *lp_fd;
    short       lv_fserr;
    int         lv_len;
    SB_API_CTR (lv_zctr, BWRITEX);

    if (pv_tag == BOMITTAG)
        pv_tag = 0;
    SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_FS_WRITEX, pv_filenum, pv_tag);
    if (gv_fs_inited)
        lp_fd = fs_int_fd_map_filenum_to_fd(pv_filenum, true); // BWRITEX
    else
        lp_fd = NULL;
    FS_FD_SCOPED_MUTEX(WHERE, lv_smutex, lp_fd);
    if (gv_fs_trace_params) {
        char *lp_to = NULL;
        if (lp_fd != NULL)
            lp_to = lp_fd->ia_fname;
        if (lp_to == NULL)
            trace_where_printf(WHERE,
                               "ENTER fnum=%d, buf=%p, Wc=%d, cw=%p, tag=" PFTAG ", uid=%d\n",
                               pv_filenum,
                               pp_buffer,
                               pv_write_count,
                               pfp(pp_count_written),
                               pv_tag,
                               pv_userid);
        else
            trace_where_printf(WHERE,
                               "ENTER to=%s, fnum=%d, buf=%p, Wc=%d, cw=%p, tag=" PFTAG ", uid=%d\n",
                               lp_to,
                               pv_filenum,
                               pp_buffer,
                               pv_write_count,
                               pfp(pp_count_written),
                               pv_tag,
                               pv_userid);
    }
    if (!gv_fs_inited)
        RETURNFSCC(fs_int_err_rtn_msg(WHERE, "file_init() not called",
                                      XZFIL_ERR_INVALIDSTATE));
    if (lp_fd == NULL) {
        lv_fserr = fs_int_err_rtn_notopen(WHERE, pv_filenum);
        fs_int_err_fd_rtn_assert(WHERE, NULL, lv_fserr, 0);
        RETURNFSCC(lv_fserr);
    }
    if (gv_fs_trace_data) {
        trace_where_printf(WHERE,
                           "(write) buf=%p, Wc=%d\n",
                           pp_buffer, pv_write_count);
        trace_print_data(pp_buffer, pv_write_count, gv_fs_trace_data_max);
    }
    lv_fserr = fs_int_fs_file_writex(lp_fd, pp_buffer, pv_write_count,
                                     &lv_len, pv_tag, pv_userid);
    if (pp_count_written != NULL)
        *pp_count_written = lv_len;
    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d, cw=%d\n", lv_fserr, lv_len);
    fs_int_err_fd_rtn_assert(WHERE, lp_fd, lv_fserr, XZFIL_ERR_TIMEDOUT);
    RETURNFSCC(lv_fserr);
}

//
// Purpose: emulate WRITEREADX
//
SB_Export _bcc_status BWRITEREADX(short        pv_filenum,
                                  char        *pp_buffer,
                                  int          pv_write_count,
                                  int          pv_read_count,
                                  int         *pp_count_read,
                                  SB_Tag_Type  pv_tag,
                                  SB_Uid_Type  pv_userid) {
    SB_API_CTR (lv_zctr, BWRITEREADX);
    return BWRITEREADX_com(pv_filenum,
                           pp_buffer,
                           pv_write_count,
                           pp_buffer,
                           pv_read_count,
                           pp_count_read,
                           pv_tag,
                           pv_userid);
}

SB_Export _bcc_status BWRITEREADX2(short        pv_filenum,
                                   char        *pp_wbuffer,
                                   int          pv_write_count,
                                   char        *pp_rbuffer,
                                   int          pv_read_count,
                                   int         *pp_count_read,
                                   SB_Tag_Type  pv_tag,
                                   SB_Uid_Type  pv_userid) {
    SB_API_CTR (lv_zctr, BWRITEREADX2);
    return BWRITEREADX_com(pv_filenum,
                           pp_wbuffer,
                           pv_write_count,
                           pp_rbuffer,
                           pv_read_count,
                           pp_count_read,
                           pv_tag,
                           pv_userid);
}

//
// Purpose: emulate WRITEREADX2
//
_bcc_status BWRITEREADX_com(short        pv_filenum,
                            char        *pp_wbuffer,
                            int          pv_write_count,
                            char        *pp_rbuffer,
                            int          pv_read_count,
                            int         *pp_count_read,
                            SB_Tag_Type  pv_tag,
                            SB_Uid_Type  pv_userid) {
    const char *WHERE = "BWRITEREADX";
    FS_Fd_Type *lp_fd;
    short       lv_fserr;
    int         lv_len;

    if (pv_tag == BOMITTAG)
        pv_tag = 0;
    SB_UTRACE_API_ADD3(SB_UTRACE_API_OP_FS_WRITEREADX, pv_filenum, pv_tag);
    if (gv_fs_inited)
        lp_fd = fs_int_fd_map_filenum_to_fd(pv_filenum, true); // BWRITEREADX*
    else
        lp_fd = NULL;
    FS_FD_SCOPED_MUTEX(WHERE, lv_smutex, lp_fd);
    if (gv_fs_trace_params) {
        char *lp_to = NULL;
        if (lp_fd != NULL)
            lp_to = lp_fd->ia_fname;
        if (lp_to == NULL)
            trace_where_printf(WHERE,
                               "ENTER fnum=%d, Wbuf=%p, Wc=%d, Rbuf=%p, Rc=%d, cr=%p, tag=" PFTAG ", uid=%d\n",
                               pv_filenum,
                               pp_wbuffer,
                               pv_write_count,
                               pp_rbuffer,
                               pv_read_count,
                               pfp(pp_count_read),
                               pv_tag,
                               pv_userid);
        else
            trace_where_printf(WHERE,
                               "ENTER to=%s, fnum=%d, Wbuf=%p, Wc=%d, Rbuf=%p, Rc=%d, cr=%p, tag=" PFTAG ", uid=%d\n",
                               lp_to,
                               pv_filenum,
                               pp_wbuffer,
                               pv_write_count,
                               pp_rbuffer,
                               pv_read_count,
                               pfp(pp_count_read),
                               pv_tag,
                               pv_userid);
    }
    if (!gv_fs_inited)
        RETURNFSCC(fs_int_err_rtn_msg(WHERE, "file_init() not called",
                                      XZFIL_ERR_INVALIDSTATE));
    if (lp_fd == NULL) {
        lv_fserr = fs_int_err_rtn_notopen(WHERE, pv_filenum);
        fs_int_err_fd_rtn_assert(WHERE, NULL, lv_fserr, 0);
        RETURNFSCC(lv_fserr);
    }
    if (gv_fs_trace_data) {
        trace_where_printf(WHERE,
                           "Wbuf=%p, Wc=%d\n",
                           pp_wbuffer, pv_write_count);
        trace_print_data(pp_wbuffer, pv_write_count, gv_fs_trace_data_max);
    }
    lv_fserr = fs_int_fs_file_writereadx(lp_fd, pp_wbuffer, pv_write_count,
                                         pp_rbuffer, pv_read_count,
                                         &lv_len, pv_tag, pv_userid);
    if (pp_count_read != NULL)
        *pp_count_read = lv_len;
    if (gv_fs_trace_data) {
        trace_where_printf(WHERE,
                           "Rbuf=%p, cr=%d\n",
                           pp_rbuffer, lv_len);
        trace_print_data(pp_rbuffer, lv_len, gv_fs_trace_data_max);
    }
    if (gv_fs_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d, cr=%d\n", lv_fserr, lv_len);
    fs_int_err_fd_rtn_assert(WHERE, lp_fd, lv_fserr, XZFIL_ERR_TIMEDOUT);
    RETURNFSCC(lv_fserr);
}

SB_Export _xcc_status XACTIVATERECEIVETRANSID(short pv_msg_num) {
    return BACTIVATERECEIVETRANSID(pv_msg_num);
}

SB_Export _xcc_status XAWAITIOX(short           *pp_filenum,
                                void           **ppp_buf,
                                unsigned short  *pp_xfercount,
                                SB_Tag_Type     *pp_tag,
                                int              pv_timeout,
                                short           *pp_segid) {
    int lv_xfercount;

    _xcc_status lv_status =
      BAWAITIOX(pp_filenum,
                ppp_buf,
                pp_xfercount != NULL ? &lv_xfercount : NULL,
                pp_tag,
                pv_timeout,
                pp_segid);
    if (pp_xfercount != NULL)
        *pp_xfercount = static_cast<ushort>(lv_xfercount);
    return lv_status;
}

SB_Export _xcc_status XAWAITIOXTS(short           *pp_filenum,
                                  void           **ppp_buf,
                                  unsigned short  *pp_xfercount,
                                  SB_Tag_Type     *pp_tag,
                                  int              pv_timeout,
                                  short           *pp_segid) {
    int lv_xfercount;

    _xcc_status lv_status =
      BAWAITIOXTS(pp_filenum,
                  ppp_buf,
                  pp_xfercount != NULL ? &lv_xfercount : NULL,
                  pp_tag,
                  pv_timeout,
                  pp_segid);
    if (pp_xfercount != NULL)
        *pp_xfercount = static_cast<ushort>(lv_xfercount);
    return lv_status;
}

//
// Purpose: emulate CANCEL
//
SB_Export _xcc_status XCANCEL(short pv_filenum) {
    return BCANCEL(pv_filenum);
}

//
// Purpose: emulate CANCELREQ
//
SB_Export _xcc_status XCANCELREQ(short                 pv_filenum,
                                 SB_Tag_Type           pv_tag) {
    return BCANCELREQ(pv_filenum, pv_tag);
}

SB_Export short XFILE_CLOSE_(short pv_filenum, short pv_tapedisposition) {
    return BFILE_CLOSE_(pv_filenum, pv_tapedisposition);
}

SB_Export short XFILE_GETINFO_(short  pv_filenum,
                               short *pp_lasterror,
                               char  *pp_filename,
                               short  pv_filename_len,
                               short *pp_filename_len,
                               short *pp_typeinfo,
                               short *pp_flags) {
    return BFILE_GETINFO_(pv_filenum,
                         pp_lasterror,
                         pp_filename,
                         pv_filename_len,
                         pp_filename_len,
                         pp_typeinfo,
                         pp_flags);
}

SB_Export short XFILE_GETRECEIVEINFO_(FS_Receiveinfo_Type *pp_receiveinfo,
                                      short               *pp_reserved) {
    return BFILE_GETRECEIVEINFO_(pp_receiveinfo,
                                 pp_reserved);
}

SB_Export short XFILE_OPEN_(char            *pp_filename,
                            short            pv_length,
                            short           *pp_filenum,
                            short            pv_access,
                            short            pv_exclusion,
                            short            pv_nowait_depth,
                            short            pv_sync_or_receive_depth,
                            xfat_16          pv_options,
                            short            pv_seq_block_buffer_id,
                            short            pv_seq_block_buffer_length,
                            SB_Phandle_Type *pp_primary_processhandle) {
    SB_API_CTR (lv_zctr, BFILE_OPEN_);
    return BFILE_OPEN_com(pp_filename,
                          pv_length,
                          pp_filenum,
                          pv_access,
                          pv_exclusion,
                          pv_nowait_depth,
                          pv_sync_or_receive_depth,
                          pv_options,
                          pv_seq_block_buffer_id,
                          pv_seq_block_buffer_length,
                          pp_primary_processhandle,
                          false); // self
}

SB_Export short XFILE_OPEN_SELF_(short           *pp_filenum,
                                 short            pv_access,
                                 short            pv_exclusion,
                                 short            pv_nowait_depth,
                                 short            pv_sync_or_receive_depth,
                                 xfat_16          pv_options,
                                 short            pv_seq_block_buffer_id,
                                 short            pv_seq_block_buffer_length,
                                 SB_Phandle_Type *pp_primary_processhandle) {
    SB_API_CTR (lv_zctr, BFILE_OPEN_SELF_);
    return BFILE_OPEN_com(ga_ms_su_pname,
                          static_cast<short>(strlen(ga_ms_su_pname)),
                          pp_filenum,
                          pv_access,
                          pv_exclusion,
                          pv_nowait_depth,
                          pv_sync_or_receive_depth,
                          pv_options,
                          pv_seq_block_buffer_id,
                          pv_seq_block_buffer_length,
                          pp_primary_processhandle,
                          true); // self
}

SB_Export _xcc_status XREADX(short           pv_filenum,
                             char           *pp_buffer,
                             unsigned short  pv_read_count,
                             unsigned short *pp_count_read,
                             SB_Tag_Type     pv_tag) {
    int lv_count_read;
    _xcc_status lv_status =
      BREADX(pv_filenum,
             pp_buffer,
             pv_read_count,
             pp_count_read != NULL ? &lv_count_read : NULL,
             pv_tag);
    if (pp_count_read != NULL)
        *pp_count_read = static_cast<ushort>(lv_count_read);
    return lv_status;
}

SB_Export _xcc_status XREADUPDATEX(short           pv_filenum,
                                   char           *pp_buffer,
                                   unsigned short  pv_read_count,
                                   unsigned short *pp_count_read,
                                   SB_Tag_Type     pv_tag) {
    int lv_count_read;
    _xcc_status lv_status =
      BREADUPDATEX(pv_filenum,
                   pp_buffer,
                   pv_read_count,
                   pp_count_read != NULL ? &lv_count_read : NULL,
                   pv_tag);
    if (pp_count_read != NULL)
        *pp_count_read = static_cast<ushort>(lv_count_read);
    return lv_status;
}

SB_Export _xcc_status XREPLYX(char           *pp_buffer,
                              unsigned short  pv_write_count,
                              unsigned short *pp_count_written,
                              short           pv_reply_num,
                              short           pv_err_ret) {
    int lv_count_written;
    _xcc_status lv_status =
      BREPLYX(pp_buffer,
              pv_write_count,
              pp_count_written != NULL ? &lv_count_written : NULL,
              pv_reply_num,
              pv_err_ret);
    if (pp_count_written != NULL)
        *pp_count_written = static_cast<ushort>(lv_count_written);
    return lv_status;
}

SB_Export _xcc_status XSETMODE(short  pv_filenum,
                               short  pv_modenum,
                               int    pv_parm1,
                               int    pv_parm2,
                               short *pp_oldval) {
    return BSETMODE(pv_filenum,
                    pv_modenum,
                    pv_parm1,
                    pv_parm2,
                    pp_oldval);
}

SB_Export _xcc_status XWRITEX(short           pv_filenum,
                              char           *pp_buffer,
                              unsigned short  pv_write_count,
                              unsigned short *pp_count_written,
                              SB_Tag_Type     pv_tag,
                              SB_Uid_Type     pv_userid) {
    int lv_count_written;
    _xcc_status lv_status =
      BWRITEX(pv_filenum,
              pp_buffer,
              pv_write_count,
              pp_count_written != NULL ? &lv_count_written : NULL,
              pv_tag,
              pv_userid);
    if (pp_count_written != NULL)
        *pp_count_written = static_cast<ushort>(lv_count_written);
    return lv_status;
}

SB_Export _xcc_status XWRITEREADX(short           pv_filenum,
                                  char           *pp_buffer,
                                  unsigned short  pv_write_count,
                                  unsigned short  pv_read_count,
                                  unsigned short *pp_count_read,
                                  SB_Tag_Type     pv_tag,
                                  SB_Uid_Type     pv_userid) {
    int         lv_count_read;
    SB_API_CTR (lv_zctr, BWRITEREADX);

    _xcc_status lv_status =
      BWRITEREADX_com(pv_filenum,
                      pp_buffer,
                      pv_write_count,
                      pp_buffer,
                      pv_read_count,
                      pp_count_read != NULL ? &lv_count_read : NULL,
                      pv_tag,
                      pv_userid);
    if (pp_count_read != NULL)
        *pp_count_read = static_cast<ushort>(lv_count_read);
    return lv_status;
}

SB_Export _xcc_status XWRITEREADX2(short           pv_filenum,
                                   char           *pp_wbuffer,
                                   unsigned short  pv_write_count,
                                   char           *pp_rbuffer,
                                   unsigned short  pv_read_count,
                                   unsigned short *pp_count_read,
                                   SB_Tag_Type     pv_tag,
                                   SB_Uid_Type     pv_userid) {
    int         lv_count_read;
    SB_API_CTR (lv_zctr, BWRITEREADX2);

    _xcc_status lv_status =
      BWRITEREADX_com(pv_filenum,
                      pp_wbuffer,
                      pv_write_count,
                      pp_rbuffer,
                      pv_read_count,
                      pp_count_read != NULL ? &lv_count_read : NULL,
                      pv_tag,
                      pv_userid);
    if (pp_count_read != NULL)
        *pp_count_read = static_cast<ushort>(lv_count_read);
    return lv_status;
}
