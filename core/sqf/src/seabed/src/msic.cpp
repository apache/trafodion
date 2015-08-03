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

#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/trace.h"

#include "msi.h"
#include "msx.h"
#include "mstrace.h"
#include "smap.h"
#include "transport.h"

//
// message-system interceptor
//

typedef void (*Proc_IC)(BMS_SRE *pp_sre);
typedef union {
    Proc_IC  iic;
    void    *ipic;
} Proc_IC_Type;

class Ms_IC_Thread : public SB_Thread::Thread {
public:
    static Ms_IC_Thread *create(Proc_IC_Type  pv_msic,
                                MS_Md_Type   *pp_md);
    static void              destroy(Ms_IC_Thread *pp_thr);
    void                     run();

private:
    Ms_IC_Thread(const char   *pp_name,
                 Proc_IC_Type  pv_msic,
                 MS_Md_Type   *pp_md);
    virtual ~Ms_IC_Thread();

    static int    cv_inx;
    MS_Md_Type   *ip_md;
    Proc_IC_Type  iv_msic;
};

static SB_Smap gv_ms_name_dl_map("map-msic-name-dl");
static SB_Smap gv_ms_name_sym_map("map-msic-name-sym");

int             Ms_IC_Thread::cv_inx = 0;

// forwards
static void ms_ic_call(const char *pp_where,
                       MS_Md_Type *pp_md,
                       const char *pp_dll,
                       const char *pp_fun,
                       short      *pp_fserr,
                       char       *pp_err);
static void ms_ic_set_sre(const char *pp_where,
                          BMS_SRE    *pp_sre,
                          MS_Md_Type *pp_md);
static void ms_ic_unload(const char *pp_where,
                         const char *pp_dll,
                         const char *pp_fun,
                         short      *pp_fserr,
                         char       *pp_err);
static void ms_ic_unload_one(const char *pp_where,
                             const char *pp_dll);

static void *ms_ic_thread_fun(void *pp_arg) {
    Ms_IC_Thread *lp_thread = static_cast<Ms_IC_Thread *>(pp_arg);
    lp_thread->run();
    Ms_IC_Thread::destroy(lp_thread);
    return NULL;
}

Ms_IC_Thread::Ms_IC_Thread(const char   *pp_name,
                           Proc_IC_Type  pv_msic,
                           MS_Md_Type   *pp_md)
: SB_Thread::Thread(ms_ic_thread_fun, pp_name),
  ip_md(pp_md),
  iv_msic(pv_msic) {
    set_daemon(true);
}

Ms_IC_Thread::~Ms_IC_Thread() {
}

Ms_IC_Thread *Ms_IC_Thread::create(Proc_IC_Type  pv_msic,
                                   MS_Md_Type   *pp_md) {
    char              la_name[20];
    Ms_IC_Thread *lp_thr;

    sprintf(la_name, "msic-%d", cv_inx++);
    lp_thr = new Ms_IC_Thread(la_name, pv_msic, pp_md);
    return lp_thr;
}

void Ms_IC_Thread::destroy(Ms_IC_Thread *pp_thr) {
    const char *WHERE = "Ms_IC_Thread::destroy";

    if (gv_ms_trace_ic)
        trace_where_printf(WHERE, "destroying thread, name=%s\n",
                           pp_thr->get_name());
    delete pp_thr;
}

void Ms_IC_Thread::run() {
    const char *WHERE = "Ms_IC_Thread::run";
    BMS_SRE     lv_sre;

    // remove from all-list - so that set_event_all doesn't wake us
    gv_ms_event_mgr.remove_from_event_all();

    if (gv_ms_trace_ic)
        trace_where_printf(WHERE, "STARTING msic thread\n");
    ms_ic_set_sre(WHERE, &lv_sre, ip_md);
    iv_msic.iic(&lv_sre);
    if (gv_ms_trace_ic)
        trace_where_printf(WHERE, "EXITING msic thread\n");
}

void ms_interceptor(const char *pp_where,
                    MS_Md_Type *pp_md,
                    int         pv_reqid,
                    bool       *pp_reply,
                    short      *pp_fserr,
                    char       *pp_err) {
    enum {
        ACTION_CALL   = 1,
        ACTION_UNLOAD = 2,
        ACTION_ZLAST  = 3
    };
    char             *lp_action;
    char             *lp_dll;
    char             *lp_fun;
    char             *lp_p;
    char             *lp_recv_ctrl;
    int               lv_action;
    int               lv_inx;
    int               lv_len;
    bool              lv_term;

    if (gv_ms_trace_ic)
        trace_where_printf(pp_where,
                           "ENTER msic md (reqid=%d, msgid=%d, md=%p)\n",
                           pv_reqid,
                           pp_md->iv_link.iv_id.i, pfp(pp_md));

    *pp_reply = true;
    *pp_fserr = XZFIL_ERR_OK;

    // ctrl should contain <action>:<dll>:<fun>
    //   action ::= { call | unload }
    //   if action == unload
    //     fun ::= { all | dll }
    lv_term = false;
    lp_recv_ctrl = pp_md->out.ip_recv_ctrl;
    lv_len = pp_md->out.iv_recv_ctrl_size;
    for (lv_inx = 0; lv_inx < lv_len; lv_inx++) {
        if (lp_recv_ctrl[lv_inx] == 0) {
            lv_term = true;
            break;
        }
    }

    lp_action = lp_recv_ctrl;
    if (!lv_term) {
        strcpy(pp_err, "control-string not terminated");
        *pp_fserr = XZFIL_ERR_INVALOP;
    }

    if (*pp_fserr == XZFIL_ERR_OK) {
        if (gv_ms_trace_ic)
            trace_where_printf(pp_where, "req-ctrl=\"%s\"\n", lp_recv_ctrl);
        // get action
        lp_p = strchr(lp_action, ':');
        if (lp_p == NULL) {
            strcpy(pp_err,
                   "control-string did not have a action ':' terminator");
            *pp_fserr = XZFIL_ERR_INVALOP;
        } else {
            *lp_p = '\0';
            lp_p++;
            if (strcmp(lp_action, "call") == 0)
                lv_action = ACTION_CALL;
            else if (strcmp(lp_action, "unload") == 0)
                lv_action = ACTION_UNLOAD;
            else {
                strcpy(pp_err, "invalid action");
                *pp_fserr = XZFIL_ERR_INVALOP;
            }
            lp_dll = lp_p;
        }
    }
    if (*pp_fserr == XZFIL_ERR_OK) {
        // get dll
        lp_p = strchr(lp_dll, ':');
        if (lp_p == NULL) {
            strcpy(pp_err, "control-string did not have a dll ':' terminator");
            *pp_fserr = XZFIL_ERR_INVALOP;
        } else {
            *lp_p = '\0';
            lp_p++;
            lp_fun = lp_p;
        }
    }

    if (*pp_fserr == XZFIL_ERR_OK) {
        switch (lv_action) {
        case ACTION_CALL:
            ms_ic_call(pp_where, pp_md, lp_dll, lp_fun, pp_fserr, pp_err);
            if (*pp_fserr == XZFIL_ERR_OK)
                *pp_reply = false;
            break;

        case ACTION_UNLOAD:
            ms_ic_unload(pp_where, lp_dll, lp_fun, pp_fserr, pp_err);
            break;

        default:
            SB_util_abort("invalid lv_action"); // sw fault
            break;
        }
    }
}

void ms_interceptor_shutdown() {
    const char *WHERE = "ms_interceptor_shutdown";
    char        la_err[200];
    const char *lp_dll;
    short       lv_fserr;

    lv_fserr = XZFIL_ERR_OK;
    la_err[0] = '\0';
    lp_dll = "unload::all";
    ms_ic_unload(WHERE,
                 lp_dll,     // dll
                 "all",      // fun
                 &lv_fserr,
                 la_err);
    if (gv_ms_trace_ic)
        trace_where_printf(WHERE, "%s returned fserr=%d, err=%s\n",
                           lp_dll, lv_fserr, la_err);
}

void ms_ic_call(const char *pp_where,
                MS_Md_Type *pp_md,
                const char *pp_dll,
                const char *pp_fun,
                short      *pp_fserr,
                char       *pp_err) {
    const char   *lp_err;
    void         *lp_handle;
    SB_Smap      *lp_map_fun;
    Ms_IC_Thread *lp_thr;
    Proc_IC_Type  lv_msic;
    BMS_SRE       lv_sre;
    bool          lv_thread;

    // see if dll already in map, if not add
    lv_thread = false;
    lp_handle = gv_ms_name_dl_map.getv(pp_dll);
    if (lp_handle == NULL) {
        lp_handle = dlopen(pp_dll, RTLD_NOW);
        if (lp_handle == NULL) {
            lp_err = dlerror();
            if (gv_ms_trace_ic)
                trace_where_printf(pp_where, "dlopen(%s) handle=%p, err=%s\n",
                                   pp_dll, lp_handle, lp_err);
            sprintf(pp_err, "could not open dll '%s' err=%s",
                    pp_dll, lp_err);
            *pp_fserr = XZFIL_ERR_NOTFOUND;
        } else {
            if (gv_ms_trace_ic)
                trace_where_printf(pp_where, "dlopen(%s) handle=%p\n",
                                   pp_dll, lp_handle);
            gv_ms_name_dl_map.putv(pp_dll, lp_handle);
        }
    } else {
        if (gv_ms_trace_ic)
            trace_where_printf(pp_where, "dlopen(%s) [CACHE] handle=%p\n",
                               pp_dll, lp_handle);
    }
    if (*pp_fserr == XZFIL_ERR_OK) {
        lp_map_fun =
          static_cast<SB_Smap *>(gv_ms_name_sym_map.getv(pp_dll));
        if (lp_map_fun == NULL) {
            lp_map_fun = new SB_Smap("map-msic-fun");
            gv_ms_name_sym_map.putv(pp_dll, lp_map_fun);
        }
        lv_msic.ipic = lp_map_fun->getv(pp_fun);
        if (lv_msic.ipic == NULL) {
            lv_msic.ipic = dlsym(lp_handle, pp_fun);
            if (lv_msic.ipic == NULL) {
                if (gv_ms_trace_ic)
                    trace_where_printf(pp_where, "dlsym(%p,%s) ret=%p, err=%s\n",
                                       lp_handle, pp_fun,
                                       lv_msic.ipic, dlerror());
            } else {
                if (gv_ms_trace_ic)
                    trace_where_printf(pp_where, "dlsym(%p,%s) ret=%p\n",
                                       lp_handle, pp_fun,
                                       lv_msic.ipic);
                lp_map_fun->putv(pp_fun, lv_msic.ipic);
            }
        } else {
            if (gv_ms_trace_ic)
                trace_where_printf(pp_where, "dlsym(%p,%s) [CACHE] ret=%p\n",
                                   lp_handle, pp_fun,
                                   lv_msic.ipic);
        }
        if (lv_msic.ipic == NULL) {
            lp_err = dlerror();
            sprintf(pp_err, "could not find function '%s' in dll '%s' err=%s",
                    pp_fun, pp_dll, lp_err);
            *pp_fserr = XZFIL_ERR_NOTFOUND;
        } else {
            lv_thread = strstr(pp_fun, "_mspthread");
            if (lv_thread) {
                if (gv_ms_trace_ic)
                    trace_where_printf(pp_where,
                                       "calling threaded-msic, dll=%s, fun=%s, msgid=%d\n",
                                       pp_dll, pp_fun,
                                       pp_md->iv_msgid);
                lp_thr = Ms_IC_Thread::create(lv_msic, pp_md);
                lp_thr->run();
            } else {
                ms_ic_set_sre(pp_where, &lv_sre, pp_md);
                if (gv_ms_trace_ic)
                    trace_where_printf(pp_where,
                                       "calling msic, dll=%s, fun=%s, msgid=%d\n",
                                       pp_dll, pp_fun,
                                       pp_md->iv_msgid);
                lv_msic.iic(&lv_sre);
            }
            if (gv_ms_trace_ic)
                trace_where_printf(pp_where, "msic returned\n");
        }
    }
}

void ms_ic_set_sre(const char *pp_where,
                   BMS_SRE    *pp_sre,
                   MS_Md_Type *pp_md) {
    pp_sre->sre_msgId = pp_md->iv_link.iv_id.i;
    pp_sre->sre_flags = BSRE_REMM;
    pp_sre->sre_pri = pp_md->out.iv_pri;
    pp_sre->sre_reqCtrlSize = pp_md->out.iv_recv_ctrl_size;
    pp_sre->sre_reqDataSize = pp_md->out.iv_recv_data_size;
    pp_sre->sre_replyCtrlMax = pp_md->out.iv_recv_ctrl_max;
    pp_sre->sre_replyDataMax = pp_md->out.iv_recv_data_max;
    if (gv_ms_trace_ic) {
        SB_Trans::Stream_Base *lp_stream =
          static_cast<SB_Trans::Stream_Base *>(pp_md->ip_stream);
        char *lp_from;
        if (lp_stream == NULL)
            lp_from = NULL;
        else
            lp_from = lp_stream->get_name();
        trace_where_printf(pp_where,
                           "from=%s, reqid=%d, sre.msgid=%d, .flags=0x%x, .pri=%d, .ctrl=%d, .data=%d\n",
                           lp_from,
                           pp_md->out.iv_recv_req_id,
                           pp_sre->sre_msgId,
                           pp_sre->sre_flags,
                           pp_sre->sre_pri,
                           pp_sre->sre_reqCtrlSize,
                           pp_sre->sre_reqDataSize);
    }
}

void ms_ic_unload(const char *pp_where,
                  const char *pp_dll,
                  const char *pp_fun,
                  short      *pp_fserr,
                  char       *pp_err) {
    enum {
        UNLOAD_ALL    = 1,
        UNLOAD_DLL    = 2,
        UNLOAD_ZLAST  = 3
    };
    char         *lp_dll;
    char         *lp_dll_dup;
    SB_Smap_Enum *lp_map_dl_keys;
    int           lv_unload;

    if (strcmp(pp_fun, "all") == 0)
        lv_unload = UNLOAD_ALL;
    else if (strcmp(pp_fun, "dll") == 0)
        lv_unload = UNLOAD_DLL;
    else {
        sprintf(pp_err, "invalid fun for unload");
        *pp_fserr = XZFIL_ERR_INVALOP;
    }

    if (*pp_fserr == XZFIL_ERR_OK) {
        switch (lv_unload) {
        case UNLOAD_ALL:
            sprintf(pp_err, "unload all dlls ok");
            for (;;) {
                lp_map_dl_keys = gv_ms_name_dl_map.keys();
                if (!lp_map_dl_keys->more()) {
                    delete lp_map_dl_keys;
                    break;
                }
                lp_dll = lp_map_dl_keys->next();
                // need to copy as entry will be deleted
                lp_dll_dup = new char[strlen(lp_dll) + 1];
                strcpy(lp_dll_dup, lp_dll);
                ms_ic_unload_one(pp_where, lp_dll_dup);
                gv_ms_name_dl_map.removev(lp_dll_dup);
                delete [] lp_dll_dup;
                delete lp_map_dl_keys;
            }
            break;

        case UNLOAD_DLL:
            sprintf(pp_err, "unload dll(%s) ok", pp_dll);
            ms_ic_unload_one(pp_where, pp_dll);
            break;

        default:
            SB_util_abort("invalid lv_unload"); // sw fault
            break;
        }
    }
}

void ms_ic_unload_one(const char *pp_where,
                      const char *pp_dll) {
    char          la_errno[100];
    char         *lp_fun;
    void         *lp_funp;
    void         *lp_handle;
    SB_Smap      *lp_map_fun;
    SB_Smap_Enum *lp_map_fun_keys;
    int           lv_err;

    lp_handle = gv_ms_name_dl_map.getv(pp_dll);
    if (lp_handle != NULL) {
        lv_err = dlclose(lp_handle);
        if (gv_ms_trace_ic)
            trace_where_printf(pp_where,
                               "dlclose(%p[%s]) [UN-CACHE] ret=%d(%s)\n",
                               lp_handle, pp_dll, lv_err,
                               strerror_r(errno, la_errno, sizeof(la_errno)));
        gv_ms_name_dl_map.removev(pp_dll);
    }
    lp_map_fun =
      static_cast<SB_Smap *>(gv_ms_name_sym_map.getv(pp_dll));
    if (lp_map_fun != NULL) {
        for (;;) {
            lp_map_fun_keys = lp_map_fun->keys();
            if (!lp_map_fun_keys->more()) {
                delete lp_map_fun_keys;
                break;
            }
            lp_fun = lp_map_fun_keys->next();
            lp_funp = lp_map_fun->getv(lp_fun);
            if (gv_ms_trace_ic)
                trace_where_printf(pp_where, "dlsym(%p,%s,%p) [UN-CACHE]\n",
                                   lp_handle, lp_fun, lp_funp);
            lp_map_fun->removev(lp_fun);
            delete lp_map_fun_keys;
        }
        delete lp_map_fun;
        gv_ms_name_sym_map.removev(pp_dll);
    }
}

