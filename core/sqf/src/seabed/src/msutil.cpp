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

//
// msg utilities
//
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "seabed/labelmaps.h"
#include "seabed/trace.h"

#include "labelmapsx.h"
#include "monmsgtype.h" // Mon_Msg_Type
#include "mstrace.h"
#include "msutil.h"
#include "msx.h"
#include "phan.h"

//
// Purpose: format phandle
//   figure out how much storage is needed for a formatted phandle
//   0x0123456789abcdef [                  - 20
//   0123456789012345.                     - 17 * 8 = 136
//   0123456789012345.
//   0123456789012345.
//   0123456789012345.
//   0123456789012345.
//   0123456789012345.
//   0123456789012345.
//   0123456789012345]
//   (12345678901234567890123456789012-    - 34
//   0123456789/                           - 11 * 3 = 33
//   0123456789/
//   0123456789)
//
// 20 + 136 + 34 + 33 = 222
//
void msg_util_format_phandle(char *pp_buf, SB_Phandle_Type *pp_phandle) {
    if (pp_phandle == NULL)
        strcpy(pp_buf, "<null>");
    else {
        SB_Phandle *lp_phandle = reinterpret_cast<SB_Phandle *>(pp_phandle);
        char la_name[SB_PHANDLE_NAME_SIZE+1];
        la_name[SB_PHANDLE_NAME_SIZE] = '\0';
        for (int lv_inx = 0;
             lv_inx < static_cast<int>(sizeof(lp_phandle->ia_name));
             lv_inx++) {
            char lv_char = lp_phandle->ia_name[lv_inx];
            if ((lv_char >= ' ') && (lv_char <= 0x7e))
                la_name[lv_inx] = lv_char;
            else if (lv_char == 0) {
                la_name[lv_inx] = '\0';
                break;
            } else
                la_name[lv_inx] = '.';
        }
#ifdef SQ_PHANDLE_VERIFIER
        sprintf(pp_buf, "%p [" PF64X "." PF64X "." PF64X "." PF64X "." PF64X "." PF64X "." PF64X "." PF64X "](%s-%d/%d/%d)",
                pfp(pp_phandle),
                pp_phandle->_data[0],
                pp_phandle->_data[1],
                pp_phandle->_data[2],
                pp_phandle->_data[3],
                pp_phandle->_data[4],
                pp_phandle->_data[5],
                pp_phandle->_data[6],
                pp_phandle->_data[7],
                la_name,
                lp_phandle->iv_nid,
                lp_phandle->iv_pid,
                lp_phandle->iv_verifier);
#else
        sprintf(pp_buf, "%p [" PF64X "." PF64X "." PF64X "." PF64X "." PF64X "." PF64X "." PF64X "." PF64X "](%s-%d/%d)",
                pfp(pp_phandle),
                pp_phandle->_data[0],
                pp_phandle->_data[1],
                pp_phandle->_data[2],
                pp_phandle->_data[3],
                pp_phandle->_data[4],
                pp_phandle->_data[5],
                pp_phandle->_data[6],
                pp_phandle->_data[7],
                la_name,
                lp_phandle->iv_nid,
                lp_phandle->iv_pid);
#endif
    }
}

//
// Purpose: format transid
//
void msg_util_format_transid(char            *pp_buf,
                             SB_Transid_Type  pv_transid) {
    sprintf(pp_buf, PF64 "." PF64 "." PF64 "." PF64,
            pv_transid.id[0],
            pv_transid.id[1],
            pv_transid.id[2],
            pv_transid.id[3]);
}

//
// Purpose: format trans-seq
//
void msg_util_format_transseq(char             *pp_buf,
                              SB_Transseq_Type  pv_transseq) {
    sprintf(pp_buf, "%ld", pv_transseq);
}

//
// Purpose: get msg type
//
const char *msg_util_get_msg_type(MSGTYPE pv_msg_type) {
    const char *lp_label;

    lp_label = SB_get_label_ms_mon_msgtype_short(pv_msg_type);
    return lp_label;
}

const char *sb_print_util_msmon_msg_type(int pv_msg_type) {
    const char *lp_label;

    lp_label = SB_get_label_ms_mon_msgtype_short(pv_msg_type);
    printf("%s", lp_label);
    return lp_label;
}

//
// Purpose: get req type
//
const char *msg_util_get_req_type(REQTYPE pv_req_type) {
    const char *lp_label;

    lp_label = SB_get_label_ms_mon_reqtype_short(pv_req_type);
    return lp_label;
}

const char *sb_print_util_msmon_req_type(int pv_req_type) {
    const char *lp_label;

    lp_label = SB_get_label_ms_mon_reqtype_short(pv_req_type);
    printf("%s", lp_label);
    return lp_label;
}

//
// Purpose: get reply type
//
const char *msg_util_get_reply_type(REPLYTYPE pv_reply_type) {
    const char *lp_label;

    lp_label = SB_get_label(&gv_sb_msmon_reply_type_label_map,
                            pv_reply_type);
    return lp_label;
}

const char *sb_print_util_msmon_reply_type(int pv_reply_type) {
    const char *lp_label;

    lp_label = SB_get_label(&gv_sb_msmon_reply_type_label_map, pv_reply_type);
    printf("%s", lp_label);
    return lp_label;
}
