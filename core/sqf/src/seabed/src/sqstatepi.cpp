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
// sqstate plug-in
//

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "seabed/fserr.h"
#include "seabed/labelmaps.h"
#include "seabed/sqstate.h"

#include "chk.h"
#include "sqstatei.h"
#include "sqstatesb.h"

// module name
#define mymodule sb

// library name
#define X_LIBNAME(module) SQSTATE_LIBNAME_PREFIX #module ".so"
#define XX_LIBNAME(module) X_LIBNAME(module)
#define LIBNAME XX_LIBNAME(mymodule)

enum { MAX_RSP  = 1024 * 1024 }; // 1MB


static char            *gp_pname = const_cast<char *>("");
static bool             gv_opened = false;
static SB_Phandle_Type  gv_phandle;
static bool             gv_verbose  = false;
static bool             gv_verbosev = false;

// forwards
static void sb_pi_ic_call_print_rsp(char *pp_title,
                                    bool  pv_title,
                                    char *pp_rsp,
                                    int   pv_rsp_len);
static void sb_pi_printf(const char *format, ...)
            __attribute__((format(printf, 1, 2)));
static void sb_pi_verb_ok(int pv_fserr, const char *pp_msg);
static void sb_pi_verb_print_info(MS_Mon_Node_Info_Entry_Type *pp_node,
                            MS_Mon_Process_Info_Type    *pp_proc,
                            const char                  *pp_test);
static void sb_pi_verb_printf(const char *format, ...)
            __attribute__((format(printf, 1, 2)));

//
// plugin - send msg to ms-ic
//
static void sb_pi_ic_send(MS_Mon_Node_Info_Entry_Type *pp_node,
                          MS_Mon_Process_Info_Type    *pp_proc,
                          Sqstate_Pi_Info_Type        *pp_info,
                          char                        *pp_title,
                          bool                         pv_title,
                          const char                  *pp_lib,
                          const char                  *pp_call,
                          char                        *pp_rsp,
                          int                          pv_rsplen,
                          int                         *pp_rsplen) {
    char             la_ctrl[200];
    char            *lp_data;
    char            *lp_p;
    char             la_rsp[MAX_RSP];
    short           *lp_ctrl;
    short           *lp_results;
    char            *lp_rsp;
    int              lv_clen;
    int              lv_disable;
    int              lv_dlen;
    int              lv_fserr;
    int              lv_inx;
    int              lv_len;
    int              lv_msgid;
    int              lv_oid;
    bool             lv_open;
    MS_Result_Type   lv_results;
    int              lv_rsplen;

    pp_node = pp_node; // touch
    if (pp_rsplen != NULL)
        *pp_rsplen = 0;

    lv_open = (strcmp(pp_proc->process_name, gp_pname) != 0);
    gp_pname = pp_proc->process_name;

    if (strcmp(pp_call, "ic_unload") == 0)
        sprintf(la_ctrl, "unload" ":" ":" "all");
    else
        sprintf(la_ctrl, "call" ":" "%s" ":" "%s", pp_lib, pp_call);
    lv_clen = static_cast<int>(strlen(la_ctrl)) + 1;
    lv_dlen = 0;
    for (lv_inx = 0; lv_inx < pp_info->ic_argc; lv_inx++)
        lv_dlen += static_cast<int>(strlen(pp_info->ic_argv[lv_inx])) + 1;
    if (lv_dlen > 0) {
        lp_data = new char[lv_dlen];
        lp_p = lp_data;
        for (lv_inx = 0; lv_inx < pp_info->ic_argc; lv_inx++) {
            lv_len = sprintf(lp_p, "%s", pp_info->ic_argv[lv_inx]);
            lp_p += lv_len + 1;
        }
    } else
        lp_data = NULL;
    if (lv_open) {
        if (gv_verbosev)
            sb_pi_verb_printf("opening %s...\n", gp_pname);
        if (gv_opened) {
            gv_opened = false;
            lv_fserr = msg_mon_close_process(&gv_phandle);
            CHK_FEIGNORE(lv_fserr);
        }
        lv_disable = msg_test_assert_disable();
        if (pp_info->self)
            lv_fserr = msg_mon_open_process_self_ic(&gv_phandle, &lv_oid);
        else
            lv_fserr = msg_mon_open_process_ic(gp_pname, &gv_phandle, &lv_oid);
        msg_test_assert_enable(lv_disable);
        sb_pi_verb_ok(lv_fserr, "open");
    } else
        lv_fserr = XZFIL_ERR_OK;
    if (lv_fserr != XZFIL_ERR_OK) {
        sb_pi_verb_printf("ERROR: could not open %s, fserr=%d\n",
                          gp_pname,
                          lv_fserr);
    } else {
        lp_ctrl = reinterpret_cast<short *>(la_ctrl);
        gv_opened = true;
        if (gv_verbose)
            sb_pi_verb_printf("linking %s, ctrl=%s...\n", gp_pname, la_ctrl);
        lv_disable = msg_test_assert_disable();
        if (pp_rsp == NULL) {
            lp_rsp = la_rsp;
            lv_rsplen = MAX_RSP;
        } else {
            lp_rsp = pp_rsp;
            lv_rsplen = pv_rsplen;
        }
        lv_fserr = BMSG_LINK_(&gv_phandle,                 // phandle
                              &lv_msgid,                   // msgid
                              lp_ctrl,                     // reqctrl
                              lv_clen,                     // reqctrlsize
                              NULL,                        // replyctrl
                              0,                           // replyctrlmax
                              lp_data,                     // reqdata
                              lv_dlen,                     // reqdatasize
                              lp_rsp,                      // replydata
                              lv_rsplen,                   // replydatamax
                              0,                           // linkertag
                              0,                           // pri
                              0,                           // xmitclass
                              BMSG_LINK_MSINTERCEPTOR);    // linkopts
        msg_test_assert_enable(lv_disable);
        sb_pi_verb_ok(lv_fserr, "link");
        if (lv_fserr != XZFIL_ERR_OK) {
            sb_pi_printf("ERROR: error in link to %s, fserr=%d\n",
                           gp_pname, lv_fserr);
        } else {
            if (gv_verbosev)
                sb_pi_verb_printf("breaking %s...\n", gp_pname);
            lp_results = reinterpret_cast<short *>(&lv_results);
            lv_disable = msg_test_assert_disable();
            lv_fserr = BMSG_BREAK_(lv_msgid, lp_results, &gv_phandle);
            msg_test_assert_enable(lv_disable);
            sb_pi_verb_ok(lv_fserr, "break");
            if (lv_fserr == XZFIL_ERR_OK) {
                if (pp_rsp == NULL)
                    sb_pi_ic_call_print_rsp(pp_title,
                                            pv_title,
                                            la_rsp,
                                            lv_results.rr_datasize);
                else if (pp_rsplen != NULL)
                    *pp_rsplen = lv_results.rr_datasize;
            } else {
                sb_pi_printf("ERROR: error in break to %s, fserr=%d\n",
                               gp_pname, lv_fserr);
            }
        }
    }
}

//
// plugin - print ms-interceptor results
//
static void sb_pi_ic_call_print_rsp(char *pp_title,
                                    bool  pv_title,
                                    char *pp_rsp,
                                    int   pv_rsp_len) {
    int   lv_len;
    int   lv_rinx;
    int   lv_rlen;
    char *lp_rbeg;
    char *lp_rend;

    if (pv_title || (!pv_title && pv_rsp_len))
        sb_pi_printf("%s\n", pp_title);
    lv_len = pv_rsp_len;
    pp_rsp[lv_len] = '\0'; // terminate
    lv_rinx = 0;
    while (lv_rinx < lv_len) {
        lp_rbeg = &pp_rsp[lv_rinx];
        lp_rend = strchr(lp_rbeg, '\n');
        if (lp_rend != NULL) {
            *lp_rend = '\0';
            lv_rlen = static_cast<int>(lp_rend - lp_rbeg) + 1;
        } else {
            lv_rlen = static_cast<int>(strlen(lp_rbeg)) + 1;
            if (lv_rlen <= 1)
                break;
        }
        sb_pi_printf("  %s\n", lp_rbeg);
        lv_rinx += lv_rlen;
    }
}

//
// plugin - get node name
//
static char *plugin_get_node_name(MS_Mon_Node_Info_Entry_Type *pp_node) {
    char *lp_node_name;

    if (pp_node == NULL)
        lp_node_name = const_cast<char *>("<uknown>");
    else
        lp_node_name = pp_node->node_name;
    return lp_node_name;
}

//
// plugin - get test info
//
static void plugin_get_test_info(Sqstate_Pi_Info_Type *pp_info) {
    gv_verbose = pp_info->verbose;
    gv_verbosev = pp_info->verbosev;
}

//
// plugin - printf
//
static void sb_pi_printf(const char *pp_format, ...) {
    va_list  lv_ap;

    va_start(lv_ap, pp_format);
    vprintf(pp_format, lv_ap);
    va_end(lv_ap);
}

//
// sqstate entry point - get openers (common)
//
static void sqstate_pi_openers_com(MS_Mon_Node_Info_Entry_Type *pp_node,
                                   MS_Mon_Process_Info_Type    *pp_proc,
                                   Sqstate_Pi_Info_Type        *pp_info,
                                   const char                  *pp_lib,
                                   char                        *pp_rsp,
                                   int                          pv_rsplen,
                                   int                         *pp_rsplen,
                                   bool                         pv_str) {
    char        la_title[100];
    const char *lp_op;
    const char *lp_proc_t;
    char       *lp_prog;

    plugin_get_test_info(pp_info);
    sb_pi_verb_print_info(pp_node, pp_proc, "sb-openers");
    lp_proc_t = SB_get_label_ms_mon_processtype_short(pp_proc->type);
    lp_prog = basename(pp_proc->program);
#ifdef SQ_PHANDLE_VERIFIER
    sprintf(la_title, "sb-openers for process=%s, type=%d(%s), p-id=%d/%d" PFVY ", prog=%s",
            pp_proc->process_name,
            pp_proc->type,
            lp_proc_t,
            pp_proc->nid,
            pp_proc->pid,
            pp_proc->verifier,
            lp_prog);
#else
    sprintf(la_title, "sb-openers for process=%s, type=%d(%s), p-id=%d/%d, prog=%s",
            pp_proc->process_name,
            pp_proc->type,
            lp_proc_t,
            pp_proc->nid,
            pp_proc->pid,
            lp_prog);
#endif

    if (pv_str)
        lp_op = "sb_ic_get_openers";
    else
        lp_op = "sb_ic_get_openers_bin";
    sb_pi_ic_send(pp_node,
                  pp_proc,
                  pp_info,
                  la_title,
                  true,
                  pp_lib,
                  lp_op,
                  pp_rsp,
                  pv_rsplen,
                  pp_rsplen);
}

//
// sqstate entry point - get opens (common)
//
static void sqstate_pi_opens_com(MS_Mon_Node_Info_Entry_Type *pp_node,
                                 MS_Mon_Process_Info_Type    *pp_proc,
                                 Sqstate_Pi_Info_Type        *pp_info,
                                 const char                  *pp_lib,
                                 char                        *pp_rsp,
                                 int                          pv_rsplen,
                                 int                         *pp_rsplen,
                                 bool                         pv_str) {
    char        la_title[100];
    const char *lp_op;
    const char *lp_proc_t;
    char       *lp_prog;

    plugin_get_test_info(pp_info);
    sb_pi_verb_print_info(pp_node, pp_proc, "sb-opens");
    lp_proc_t = SB_get_label_ms_mon_processtype_short(pp_proc->type);
    lp_prog = basename(pp_proc->program);
#ifdef SQ_PHANDLE_VERIFIER
    sprintf(la_title, "sb-opens for process=%s, type=%d(%s), p-id=%d/%d" PFVY ", prog=%s",
            pp_proc->process_name,
            pp_proc->type,
            lp_proc_t,
            pp_proc->nid,
            pp_proc->pid,
            pp_proc->verifier,
            lp_prog);
#else
    sprintf(la_title, "sb-opens for process=%s, type=%d(%s), p-id=%d/%d, prog=%s",
            pp_proc->process_name,
            pp_proc->type,
            lp_proc_t,
            pp_proc->nid,
            pp_proc->pid,
            lp_prog);
#endif
    if (pv_str)
        lp_op = "sb_ic_get_opens";
    else
        lp_op = "sb_ic_get_opens_bin";
    sb_pi_ic_send(pp_node,
                  pp_proc,
                  pp_info,
                  la_title,
                  true,
                  pp_lib,
                  lp_op,
                  pp_rsp,
                  pv_rsplen,
                  pp_rsplen);
}

//
// sqstate entry point - mds
//
SQSTATE_PI_EP(mymodule,mds,pp_node,pp_proc,pp_info,pp_lib) {
    char        la_title[100];
    const char *lp_proc_t;
    char       *lp_prog;

    plugin_get_test_info(pp_info);
    sb_pi_verb_print_info(pp_node, pp_proc, "sb-mds");
    lp_proc_t = SB_get_label_ms_mon_processtype_short(pp_proc->type);
    lp_prog = basename(pp_proc->program);
#ifdef SQ_PHANDLE_VERIFIER
    sprintf(la_title, "sb-mds for process=%s, type=%d(%s), p-id=%d/%d/" PFVY ", prog=%s",
            pp_proc->process_name,
            pp_proc->type,
            lp_proc_t,
            pp_proc->nid,
            pp_proc->pid,
            pp_proc->verifier,
            lp_prog);
#else
    sprintf(la_title, "sb-mds for process=%s, type=%d(%s), p-id=%d/%d, prog=%s",
            pp_proc->process_name,
            pp_proc->type,
            lp_proc_t,
            pp_proc->nid,
            pp_proc->pid,
            lp_prog);
#endif
    sb_pi_ic_send(pp_node,
                  pp_proc,
                  pp_info,
                  la_title,
                  true,
                  pp_lib,
                  "sb_ic_get_mds",
                  NULL,
                  0,
                  NULL);
}

//
// sqstate entry point - metrics
//
SQSTATE_PI_EP(mymodule,metrics,pp_node,pp_proc,pp_info,pp_lib) {
    char        la_title[100];
    const char *lp_proc_t;
    char       *lp_prog;

    plugin_get_test_info(pp_info);
    sb_pi_verb_print_info(pp_node, pp_proc, "sb-metrics");
    lp_proc_t = SB_get_label_ms_mon_processtype_short(pp_proc->type);
    lp_prog = basename(pp_proc->program);
    sprintf(la_title, "sb-metrics for process=%s, type=%d(%s), p-id=%d/%d, prog=%s",
            pp_proc->process_name,
            pp_proc->type,
            lp_proc_t,
            pp_proc->nid,
            pp_proc->pid,
            lp_prog);
    sb_pi_ic_send(pp_node,
                  pp_proc,
                  pp_info,
                  la_title,
                  true,
                  pp_lib,
                  "sb_ic_get_metrics",
                  NULL,
                  0,
                  NULL);
}

//
// sqstate entry point - get openers
//
SQSTATE_PI_EP(mymodule,openers,pp_node,pp_proc,pp_info,pp_lib) {
    sqstate_pi_openers_com(pp_node, pp_proc, pp_info, pp_lib, NULL, 0, 0, true);
}

//
// sqstate entry point - get openers (binary)
//
SQSTATE_PI_SQSTATE_EP(mymodule,openers,pp_node,pp_proc,pp_info,pp_lib,pp_rsp,pv_rsplen,pp_rsplen) {
    sqstate_pi_openers_com(pp_node,
                           pp_proc,
                           pp_info,
                           pp_lib,
                           pp_rsp,
                           pv_rsplen,
                           pp_rsplen,
                           false);
}

//
// sqstate entry point - get opens
//
SQSTATE_PI_EP(mymodule,opens,pp_node,pp_proc,pp_info,pp_lib) {
    sqstate_pi_opens_com(pp_node, pp_proc, pp_info, pp_lib, NULL, 0, 0, true);
}

//
// sqstate entry point - get opens (binary)
//
SQSTATE_PI_SQSTATE_EP(mymodule,opens,pp_node,pp_proc,pp_info,pp_lib,pp_rsp,pv_rsplen,pp_rsplen) {
    sqstate_pi_opens_com(pp_node,
                         pp_proc,
                         pp_info,
                         pp_lib,
                         pp_rsp,
                         pv_rsplen,
                         pp_rsplen,
                         false);
}

//
// sqstate entry point - prog
//
SQSTATE_PI_EP(mymodule,prog,pp_node,pp_proc,pp_info,pp_lib) {
    char        la_title[100];
    const char *lp_proc_t;
    char       *lp_prog;

    plugin_get_test_info(pp_info);
    sb_pi_verb_print_info(pp_node, pp_proc, "sb-prog");
    lp_proc_t = SB_get_label_ms_mon_processtype_short(pp_proc->type);
    lp_prog = basename(pp_proc->program);
#ifdef SQ_PHANDLE_VERIFIER
    sprintf(la_title, "sb-prog for process=%s, type=%d(%s), p-id=%d/%d/" PFVY ", prog=%s",
            pp_proc->process_name,
            pp_proc->type,
            lp_proc_t,
            pp_proc->nid,
            pp_proc->pid,
            pp_proc->verifier,
            lp_prog);
#else
    sprintf(la_title, "sb-prog for process=%s, type=%d(%s), p-id=%d/%d, prog=%s",
            pp_proc->process_name,
            pp_proc->type,
            lp_proc_t,
            pp_proc->nid,
            pp_proc->pid,
            lp_prog);
#endif
    sb_pi_ic_send(pp_node,
                  pp_proc,
                  pp_info,
                  la_title,
                  true,
                  pp_lib,
                  "sb_ic_prog",
                  NULL,
                  0,
                  NULL);
}

//
// sqstate entry point - pstate
//
SQSTATE_PI_EP(mymodule,pstate,pp_node,pp_proc,pp_info,pp_lib) {
    char        la_title[100];
    const char *lp_proc_t;
    char       *lp_prog;

    plugin_get_test_info(pp_info);
    sb_pi_verb_print_info(pp_node, pp_proc, "sb-pstate");
    lp_proc_t = SB_get_label_ms_mon_processtype_short(pp_proc->type);
    lp_prog = basename(pp_proc->program);
#ifdef SQ_PHANDLE_VERIFIER
    sprintf(la_title, "sb-pstate for process=%s, type=%d(%s), p-id=%d/%d/" PFVY ", prog=%s",
            pp_proc->process_name,
            pp_proc->type,
            lp_proc_t,
            pp_proc->nid,
            pp_proc->pid,
            pp_proc->verifier,
            lp_prog);
#else
    sprintf(la_title, "sb-pstate for process=%s, type=%d(%s), p-id=%d/%d, prog=%s",
            pp_proc->process_name,
            pp_proc->type,
            lp_proc_t,
            pp_proc->nid,
            pp_proc->pid,
            lp_prog);
#endif
    sb_pi_ic_send(pp_node,
                  pp_proc,
                  pp_info,
                  la_title,
                  true,
                  pp_lib,
                  "sb_ic_get_pstate",
                  NULL,
                  0,
                  NULL);
}

//
// sqstate entry point - trans (common)
//
static void sqstate_pi_trans_com(MS_Mon_Node_Info_Entry_Type *pp_node,
                                 MS_Mon_Process_Info_Type    *pp_proc,
                                 Sqstate_Pi_Info_Type        *pp_info,
                                 const char                  *pp_lib,
                                 char                        *pp_rsp,
                                 int                          pv_rsplen,
                                 int                         *pp_rsplen,
                                 bool                         pv_str) {
    char        la_title[100];
    const char *lp_op;
    const char *lp_proc_t;
    char       *lp_prog;

    plugin_get_test_info(pp_info);
    sb_pi_verb_print_info(pp_node, pp_proc, "sb-trans");
    lp_proc_t = SB_get_label_ms_mon_processtype_short(pp_proc->type);
    lp_prog = basename(pp_proc->program);
#ifdef SQ_PHANDLE_VERIFIER
    sprintf(la_title, "sb-trans for process=%s, type=%d(%s), p-id=%d/%d/" PFVY ", prog=%s",
            pp_proc->process_name,
            pp_proc->type,
            lp_proc_t,
            pp_proc->nid,
            pp_proc->pid,
            pp_proc->verifier,
            lp_prog);
#else
    sprintf(la_title, "sb-trans for process=%s, type=%d(%s), p-id=%d/%d, prog=%s",
            pp_proc->process_name,
            pp_proc->type,
            lp_proc_t,
            pp_proc->nid,
            pp_proc->pid,
            lp_prog);
#endif
    if (pv_str)
        lp_op = "sb_ic_get_trans";
    else
        lp_op = "sb_ic_get_trans_bin";
    sb_pi_ic_send(pp_node,
                  pp_proc,
                  pp_info,
                  la_title,
                  false,
                  pp_lib,
                  lp_op,
                  pp_rsp,
                  pv_rsplen,
                  pp_rsplen);
}

//
// sqstate entry point - trans
//
SQSTATE_PI_EP(mymodule,trans,pp_node,pp_proc,pp_info,pp_lib) {
    sqstate_pi_trans_com(pp_node, pp_proc, pp_info, pp_lib, NULL, 0, 0, true);
}

//
// sqstate entry point - trans (binary)
//
SQSTATE_PI_SQSTATE_EP(mymodule,trans,pp_node,pp_proc,pp_info,pp_lib,pp_rsp,pv_rsplen,pp_rsplen) {
    sqstate_pi_trans_com(pp_node,
                         pp_proc,
                         pp_info,
                         pp_lib,
                         pp_rsp,
                         pv_rsplen,
                         pp_rsplen,
                         false);
}

//
// sqstate entry point - unload ms-ic
//
SQSTATE_PI_EP(mymodule,unload,pp_node,pp_proc,pp_info,pp_lib) {
    char la_title[100];

    plugin_get_test_info(pp_info);
    sb_pi_verb_print_info(pp_node, pp_proc, "sb-unload");
    sprintf(la_title, "sb-unload for process %s", pp_proc->process_name);
    sb_pi_ic_send(pp_node,
                  pp_proc,
                  pp_info,
                  la_title,
                  true,
                  pp_lib,
                  "ic_unload",
                  NULL,
                  0,
                  NULL);
}

//
// verbose - ok
//
static void sb_pi_verb_ok(int pv_fserr, const char *pp_msg) {
    if (gv_verbosev && (pv_fserr == XZFIL_ERR_OK))
        sb_pi_verb_printf("%s OK...\n", pp_msg);
}

//
// verbose - ok
//
static void sb_pi_verb_print_info(MS_Mon_Node_Info_Entry_Type *pp_node,
                                  MS_Mon_Process_Info_Type    *pp_proc,
                                  const char                  *pp_test) {
    char *lp_node_name;

    if (gv_verbose) {
        lp_node_name = plugin_get_node_name(pp_node);
#ifdef SQ_PHANDLE_VERIFIER
        sb_pi_printf("in target test=%s for node=%p, proc=%p, p-id=%d/%d/" PFVY ", pname=%s, node-name=%s\n",
                      pp_test,
                      static_cast<void *>(pp_node),
                      static_cast<void *>(pp_proc),
                      pp_proc->nid,
                      pp_proc->pid,
                      pp_proc->verifier,
                      pp_proc->process_name,
                      lp_node_name);
#else
        sb_pi_printf("in target test=%s for node=%p, proc=%p, p-id=%d/%d, pname=%s, node-name=%s\n",
                      pp_test,
                      static_cast<void *>(pp_node),
                      static_cast<void *>(pp_proc),
                      pp_proc->nid,
                      pp_proc->pid,
                      pp_proc->process_name,
                      lp_node_name);
#endif
    }
}

//
// verbose - printf
//
static void sb_pi_verb_printf(const char *pp_format, ...) {
    va_list  lv_ap;

    va_start(lv_ap, pp_format);
    vprintf(pp_format, lv_ap);
    va_end(lv_ap);
}

