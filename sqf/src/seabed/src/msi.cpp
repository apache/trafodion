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

#include "msi.h"
#include "mstrace.h"

void MS_SM_Seg_Mgr::seg_mgr_copy_segs_to_block(char *pp_block, int pv_len) {
    char *lp_seg;
    int   lv_clen;
    int   lv_inx;
    int   lv_len;
    int   lv_seg_len;

    lv_clen = 0;
    for (lv_inx = 0; lv_inx < iv_seg_count_curr; lv_inx++) {
        seg_mgr_get_seg(lv_inx, &lp_seg, &lv_seg_len);
        lv_len = pv_len - lv_clen;
        if (lv_len > lv_seg_len)
            lv_len = lv_seg_len;
        memcpy(&pp_block[lv_clen], lp_seg, lv_len);
        lv_clen += lv_seg_len;
        if (lv_clen >= pv_len)
            break;
    }
}

void MS_SM_Seg_Mgr::seg_mgr_trace_segs_to_block(const char *pp_where,
                                                char       *pp_block,
                                                int         pv_len) {
    char *lp_seg;
    int   lv_clen;
    int   lv_inx;
    int   lv_len;
    int   lv_seg_len;

    lv_clen = 0;
    for (lv_inx = 0; lv_inx < iv_seg_count_curr; lv_inx++) {
        seg_mgr_get_seg(lv_inx, &lp_seg, &lv_seg_len);
        lv_len = pv_len - lv_clen;
        if (lv_len > lv_seg_len)
            lv_len = lv_seg_len;
        if (gv_ms_trace)
            trace_where_printf(pp_where,
                               "(copy) src=%p, scnt=%d, dest=%p, dcnt=%d, len=%d\n",
                               lp_seg, lv_seg_len, &pp_block[lv_clen], lv_len, lv_len);
        lv_clen += lv_seg_len;
        if (lv_clen >= pv_len)
            break;
    }
}

#ifndef SB_MSG_UTIL_INLINE_ENABLE
#include "msi.inl"
#endif

