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

#ifndef __MSI_INL_
#define __MSI_INL_

#include <stdlib.h>

SB_MSG_INLINE MS_SM_Seg_Mgr::MS_SM_Seg_Mgr()
: ip_segs(NULL),
  iv_seg_count_curr(0),
  iv_seg_count_max(0) {
}

SB_MSG_INLINE MS_SM_Seg_Mgr::~MS_SM_Seg_Mgr() {
    seg_mgr_del_segs();
}

SB_MSG_INLINE void MS_SM_Seg_Mgr::seg_mgr_add_seg(char *pp_seg,
                                                  int   pv_len) {
    Seg_Desc *lp_seg;

    if (iv_seg_count_curr >= iv_seg_count_max) {
        printf("buf-map-mgr, seg overflow\n");
        abort();
    }
    lp_seg = &ip_segs[iv_seg_count_curr];
    lp_seg->ip_seg = pp_seg;
    lp_seg->iv_seg_len = pv_len;
    iv_seg_count_curr++;
}

SB_MSG_INLINE void MS_SM_Seg_Mgr::seg_mgr_copy_mgr(MS_SM_Seg_Mgr *pp_mgr) {
    seg_mgr_del_segs(); // delete previous
    ip_segs = pp_mgr->ip_segs;
    iv_seg_count_curr = pp_mgr->iv_seg_count_curr;
    iv_seg_count_max = pp_mgr->iv_seg_count_max;
    pp_mgr->seg_mgr_reset(true, true, true);
}

SB_MSG_INLINE int MS_SM_Seg_Mgr::seg_mgr_count_segs() {
    return iv_seg_count_curr;
}

SB_MSG_INLINE void MS_SM_Seg_Mgr::seg_mgr_create(int pv_len,
                                                 int pv_segsize) {
    seg_mgr_reset(true, true, false);
    if (pv_len > 0) {
        iv_seg_count_max = (pv_len + pv_segsize - 1) / pv_segsize;
        seg_mgr_del_segs();
        ip_segs = new Seg_Desc[iv_seg_count_max];
    } else
        ip_segs = NULL;
}

SB_MSG_INLINE void MS_SM_Seg_Mgr::seg_mgr_del_segs() {
    if (ip_segs != NULL) {
        delete [] ip_segs;
        ip_segs = NULL;
    }
}

SB_MSG_INLINE int MS_SM_Seg_Mgr::seg_mgr_get_len() {
    Seg_Desc *lp_seg;
    int       lv_inx;
    int       lv_len;

    lv_len = 0;
    for (lv_inx = 0; lv_inx < iv_seg_count_curr; lv_inx++) {
        lp_seg = &ip_segs[lv_inx];
        lv_len += lp_seg->iv_seg_len;
    }

    return lv_len;
}

SB_MSG_INLINE void MS_SM_Seg_Mgr::seg_mgr_get_seg(int    pv_inx,
                                                  char **ppp_seg,
                                                  int   *pp_len) {
    Seg_Desc *lp_seg;

    if ((pv_inx < 0) || (pv_inx >= iv_seg_count_curr)) {
        printf("invalid buf-inx=%d\n", pv_inx);
        abort();
    }
    lp_seg = &ip_segs[pv_inx];
    *ppp_seg = lp_seg->ip_seg;
    *pp_len = lp_seg->iv_seg_len;
}

SB_MSG_INLINE void MS_SM_Seg_Mgr::seg_mgr_reset(bool pv_curr,
                                                bool pv_max,
                                                bool pv_map) {
    if (pv_curr)
        iv_seg_count_curr = 0;
    if (pv_max)
        iv_seg_count_max = 0;
    if (pv_map)
        ip_segs = NULL;
}

SB_MSG_INLINE MS_SM_Seg_Mgr::Seg_Desc::Seg_Desc()
: ip_seg(NULL), iv_seg_len(0) {
}

SB_MSG_INLINE MS_SM_Seg_Mgr::Seg_Desc::~Seg_Desc() {
}

#endif // !__MSI_INL_
