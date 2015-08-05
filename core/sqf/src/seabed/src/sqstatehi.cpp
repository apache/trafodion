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
// sqstate hi
//

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "seabed/fserr.h"
#include "seabed/sqstatehi.h"

#include "chk.h"
#include "props.h"
#include "sqstatei.h"

extern SB_Props         gv_module_m2l_env;

// since only one interceptor can run at a time, args can be static
static char            *gp_sqstateic_args = NULL;
static int              gv_sqstateic_args_len = 0;
static char            *gp_sqstatepi_pname = const_cast<char *>("");
static bool             gv_sqstatepi_opened = false;
static SB_Phandle_Type  gv_sqstatepi_phandle;

//
// ic - check if cmd is in data
//
// return true if valid
//
SB_Export bool sqstateic_get_ic_args(BMS_SRE *pp_sre,
                                     int     *pp_ic_argc,
                                     char    *pa_ic_argv[],
                                     int      pv_ic_argc_max,
                                     char    *pp_rsp,
                                     int      pv_rsp_len,
                                     int     *pp_rsp_len) {
    char *lp_end;
    char *lp_p;
    int   lv_dlen;
    int   lv_fserr;
    int   lv_inx;
    int   lv_len;
    bool  lv_valid;

    pv_rsp_len = pv_rsp_len; // touch
    lv_dlen = pp_sre->sre_reqDataSize;
    if ((gp_sqstateic_args != NULL) && (lv_dlen > gv_sqstateic_args_len)) {
        delete [] gp_sqstateic_args;
        gp_sqstateic_args = NULL;
    }
    if (gp_sqstateic_args == NULL) {
        gv_sqstateic_args_len = lv_dlen;
        gp_sqstateic_args = new char[lv_dlen];
    }
    lv_fserr = BMSG_READDATA_(pp_sre->sre_msgId,      // msgid
                              gp_sqstateic_args,      // reqdata
                              lv_dlen);               // bytecount

    *pp_rsp_len = 0;
    *pp_ic_argc = 0;
    if (lv_fserr == XZFIL_ERR_OK) {
        lv_valid = true;
        lp_p = gp_sqstateic_args;
        lp_end = &lp_p[lv_dlen];
        for (lv_inx = 0; lp_p < lp_end; lv_inx++) {
            lv_len = static_cast<int>(strlen(lp_p));
            if (lv_inx < pv_ic_argc_max)
                pa_ic_argv[lv_inx] = lp_p;
            lp_p += (lv_len + 1);
        }
        if (lv_inx >= pv_ic_argc_max)
            lv_inx = pv_ic_argc_max;
        *pp_ic_argc = lv_inx;
    } else {
        lv_valid = false;
        *pp_rsp_len = sprintf(pp_rsp, "unable to read request");
    }
    return lv_valid;
}

//
// ic - do reply
//
SB_Export void sqstateic_reply(BMS_SRE *pp_sre,
                               char    *pp_rsp,
                               int      pv_rsp_len) {
    if (gp_sqstateic_args != NULL) {
        delete [] gp_sqstateic_args;
        gp_sqstateic_args = NULL;
    }
    BMSG_REPLY_(pp_sre->sre_msgId,       // msgid
                NULL,                    // replyctrl
                0,                       // replyctrlsize
                pp_rsp,                  // replydata
                pv_rsp_len,              // replydatasize
                0,                       // errorclass
                NULL);                   // newphandle
}

//
// plugin - send message to interceptor
//
SB_Export bool sqstatepi_send_ic_ok(const char                  *pp_module,
                                    const char                  *pp_call,
                                    MS_Mon_Node_Info_Entry_Type *pp_node,
                                    MS_Mon_Process_Info_Type    *pp_proc,
                                    Sqstate_Pi_Info_Type        *pp_info,
                                    const char                  *pp_lib,
                                    char                        *pp_rsp,
                                    int                          pv_rsp_len,
                                    int                         *pp_rsp_len) {
    char             la_ctrl[200];
    char            *lp_data;
    char            *lp_p;
    short           *lp_results;
    int              lv_clen;
    int              lv_dlen;
    int              lv_fserr;
    int              lv_inx;
    int              lv_len;
    int              lv_msgid;
    int              lv_oid;
    int              lv_ok;
    bool             lv_open;
    MS_Result_Type   lv_results;

    pp_node = pp_node; // touch
    lv_ok = false;
    *pp_rsp_len = 0;

    lv_open = (strcmp(pp_proc->process_name, gp_sqstatepi_pname) != 0);
    gp_sqstatepi_pname = pp_proc->process_name;

    sqstatepi_set_ctrl(la_ctrl, pp_info, pp_lib, pp_module, pp_call);
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
        if (pp_info->verbosev)
            sqstatepi_printf("opening %s...\n", gp_sqstatepi_pname);
        if (gv_sqstatepi_opened) {
            gv_sqstatepi_opened = false;
            lv_fserr = msg_mon_close_process(&gv_sqstatepi_phandle);
            CHK_FEIGNORE(lv_fserr);
        }
        if (pp_info->self)
            lv_fserr = msg_mon_open_process_self_ic(&gv_sqstatepi_phandle,
                                                    &lv_oid);
        else
            lv_fserr = msg_mon_open_process_ic(gp_sqstatepi_pname,
                                               &gv_sqstatepi_phandle,
                                               &lv_oid);
        sqstatepi_verbose_ok(pp_info, lv_fserr, "open");
    } else
        lv_fserr = XZFIL_ERR_OK;
    if (lv_fserr != XZFIL_ERR_OK) {
        sqstatepi_printf("ERROR: could not open %s, fserr=%d\n",
                         gp_sqstatepi_pname, lv_fserr);
    } else {
        gv_sqstatepi_opened = true;
        if (pp_info->verbose)
            sqstatepi_printf("linking %s, ctrl=%s...\n",
                             gp_sqstatepi_pname, la_ctrl);
        lv_fserr = BMSG_LINK_(&gv_sqstatepi_phandle,    // phandle
                              &lv_msgid,                // msgid
                              (short *) la_ctrl,        // reqctrl
                              lv_clen,                  // reqctrlsize
                              NULL,                     // replyctrl
                              0,                        // replyctrlmax
                              lp_data,                  // reqdata
                              lv_dlen,                  // reqdatasize
                              pp_rsp,                   // replydata
                              pv_rsp_len,               // replydatamax
                              0,                        // linkertag
                              0,                        // pri
                              0,                        // xmitclass
                              BMSG_LINK_MSINTERCEPTOR); // linkopts
        sqstatepi_verbose_ok(pp_info, lv_fserr, "link");
        if (lv_fserr != XZFIL_ERR_OK) {
            sqstatepi_printf("ERROR: error in link to %s, fserr=%d\n",
                             gp_sqstatepi_pname, lv_fserr);
        } else {
            if (pp_info->verbosev)
                sqstatepi_printf("breaking %s...\n", gp_sqstatepi_pname);
            lp_results = (short *) &lv_results;
            lv_fserr = BMSG_BREAK_(lv_msgid, lp_results, &gv_sqstatepi_phandle);
            sqstatepi_verbose_ok(pp_info, lv_fserr, "break");
            if (lv_fserr == XZFIL_ERR_OK) {
                *pp_rsp_len = lv_results.rr_datasize;
                lv_ok = true;
            } else {
                sqstatepi_printf("ERROR: error in break to %s, fserr=%d\n",
                                 gp_sqstatepi_pname, lv_fserr);
            }
        }
    }

    if (lp_data != NULL)
        delete [] lp_data;

    return lv_ok;
}

//
// plugin - set ctrl
//
SB_Export void sqstatepi_set_ctrl(char                 *pp_ctrl,
                                  Sqstate_Pi_Info_Type *pp_info,
                                  const char           *pp_lib,
                                  const char           *pp_module,
                                  const char           *pp_call) {
    sprintf(pp_ctrl, "call" ":" "%s" ":" "sqstate_ic_" "%s" "_%s",
            pp_lib, pp_module, pp_call);
    if (pp_info->verbosev)
        sqstatepi_printf("ctrl=%s\n", pp_ctrl);
}

//
// plugin - print interceptor reply
//
SB_Export void sqstatepi_print_ic_reply(char *pp_rsp, int pv_rsp_len) {
    char *lp_rbeg;
    char *lp_rend;
    int   lv_len;
    int   lv_rinx;
    int   lv_rlen;

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
        sqstatepi_printf("  %s\n", lp_rbeg);
        lv_rinx += lv_rlen;
    }
}

//
// plugin - printf
//
SB_Export void sqstatepi_printf(const char *pp_format, ...) {
    va_list  lv_ap;

    va_start(lv_ap, pp_format);
    vprintf(pp_format, lv_ap);
    va_end(lv_ap);
}

//
// plugin - if verbose & ok, print msg
//
SB_Export void sqstatepi_verbose_ok(Sqstate_Pi_Info_Type *pp_info,
                                    int                   pv_fserr,
                                    const char           *pp_msg) {
    if (pp_info->verbose && (pv_fserr == XZFIL_ERR_OK))
        sqstatepi_printf("%s OK...\n", pp_msg);
}
