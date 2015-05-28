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

#include "labelmapsx.h"
#include "utracex.h"

#ifdef USE_SB_UTRACE_API
enum {
    SB_UTRACE_API_MAX_BUF = 0x1000    // must be a multiple of 2
};
#endif // USE_SB_UTRACE_API

#ifdef USE_SB_UTRACE_API
SB_Utrace<SB_Utrace_API_Type> sb_utrace_api(SB_UTRACE_API_MAX_BUF);
#endif // USE_SB_UTRACE_API

#ifdef USE_SB_UTRACE_API
const char *sb_get_utrace_api_op(int pv_op) {
    const char *lp_label = SB_get_label(&gv_sb_utrace_api_op_label_map, pv_op);
    return lp_label;
}

const char *sb_print_utrace_api_op(int pv_op) {
    const char *lp_label = sb_get_utrace_api_op(pv_op);
    printf("%s", lp_label);
    return lp_label;
}

void sb_print_utrace_api_entry(FILE               *pp_f,
                               SB_Utrace_API_Type *pp_rec,
                               int                 pv_inx) {
    const char *lp_msgtype_label;
    const char *lp_op_label;
    const char *lp_reqtype_label;

    lp_op_label = SB_get_label(&gv_sb_utrace_api_op_label_map, pp_rec->iv_op);
    switch (pp_rec->iv_op) {
    case SB_UTRACE_API_OP_MS_MON_MSG:
        lp_msgtype_label =
          SB_get_label(&gv_sb_utrace_api_mon_msgtype_label_map,
                       pp_rec->iv_id);
        lp_reqtype_label =
          SB_get_label(&gv_sb_utrace_api_mon_reqtype_label_map,
                       static_cast<int>(pp_rec->iv_info1));
        fprintf(pp_f,
                "rec[%05d]: tid=%d, op=%d(%s), id=%d(%s), info1=0x" SB_UTRACE_PINFOX " (%s)\n",
                pv_inx,
                pp_rec->iv_tid,
                pp_rec->iv_op,
                lp_op_label,
                pp_rec->iv_id,
                lp_msgtype_label,
                pp_rec->iv_info1,
                lp_reqtype_label);
        break;

    default:
        fprintf(pp_f,
                "rec[%05d]: tid=%d, op=%d(%s), id=%d, info1=0x" SB_UTRACE_PINFOX "\n",
                pv_inx,
                pp_rec->iv_tid,
                pp_rec->iv_op,
                lp_op_label,
                pp_rec->iv_id,
                pp_rec->iv_info1);
    }
}

void sb_print_utrace_api_table(int pv_cnt) {
    sb_utrace_api.print_entries_last("sb_utrace_api",
                                      stdout,
                                      sb_print_utrace_api_entry,
                                      pv_cnt);
}
#endif // USE_SB_UTRACE_API

