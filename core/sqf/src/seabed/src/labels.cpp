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

#include <assert.h>
#include <stdio.h>

#include "seabed/labels.h"

// forwards
static const char *SB_get_label_int(SB_Label_Map *pp_map,
                                    int           pv_value,
                                    bool         *pp_ok);

const char *SB_get_label(SB_Label_Map *pp_map, int pv_value) {
    return SB_get_label_int(pp_map, pv_value, NULL);
}

const char *SB_get_label_maps(SB_Label_Map **ppp_map, int pv_value) {
    SB_Label_Map *lp_map;
    const char   *lp_ret;
    int           lv_inx;
    bool          lv_ok;

    lp_map = ppp_map[0];
    assert(lp_map != NULL);
    for (lv_inx = 1; lp_map != NULL; lv_inx++) {
        lp_ret = SB_get_label_int(lp_map, pv_value, &lv_ok);
        if (lv_ok)
            return lp_ret;
        lp_map = ppp_map[lv_inx];
    }
    return ppp_map[0]->ip_unknown;
}

static const char *SB_get_label_int(SB_Label_Map *pp_map,
                                    int           pv_value,
                                    bool         *pp_ok) {
    int lv_inx = pv_value - pp_map->iv_low_inx;
    int lv_max = pp_map->iv_max - pp_map->iv_low_inx;
    assert(pp_map->ipp_labels[lv_max + 1] == SB_LABEL_END); // regular assert
    if ((lv_inx >= 0) && (lv_inx <= lv_max)) {
        if (pp_ok != NULL)
            *pp_ok = true;
        return pp_map->ipp_labels[lv_inx];
    } else {
        if (pp_ok != NULL)
            *pp_ok = false;
        return pp_map->ip_unknown;
    }
}

#if 0 // standalone test
enum { T1_L1, T1_L2, T1_L3, T1_L4, T1_ZLAST, T1_LMAX = T1_ZLAST-1 };
const char         *ga_test1_labels[] = {
    "t1L1", "t1L2", "t1L3", "t1L4", SB_LABEL_END
};
SB_Label_Map  gv_test1_label_map = {
    T1_L1, T1_LMAX, "<unknown>", ga_test1_labels
};

enum { T2_L1 = 3, T2_L2, T2_L3, T2_L4, T2_ZLAST, T2_LMAX = T2_ZLAST-1 };
const char         *ga_test2_labels[] = {
    "t2L1", "t2L2", "t2L3", "t2L4", SB_LABEL_END
};
SB_Label_Map  gv_test2_label_map = {
    T2_L1, T2_LMAX, "<unknown>", ga_test2_labels
};

enum { T3A_L1, T3A_L2, T3A_L3, T3A_ZLAST, T3A_LMAX = T3A_ZLAST-1 };
const char         *ga_test3a_labels[] = {
    "t3aL1", "t3aL2", "t3aL3", SB_LABEL_END
};
SB_Label_Map  gv_test3a_label_map = {
    T3A_L1, T3A_LMAX, "<unknown>", ga_test3a_labels
};

enum { T3B_L1 = 10, T3B_L2, T3B_ZLAST, T3B_LMAX = T3B_ZLAST-1 };
const char         *ga_test3b_labels[] = {
    "t3bL1", "t3bL2", SB_LABEL_END
};
SB_Label_Map  gv_test3b_label_map = {
    T3B_L1, T3B_LMAX, "<unknown>", ga_test3b_labels
};


SB_Label_Map *ga_test3_label_map[] = {
    &gv_test3a_label_map,
    &gv_test3b_label_map,
    NULL
};

void test_print(SB_Label_Map *pp_map, int pv_value) {
    printf("%d=%s\n", pv_value, SB_get_label(pp_map, pv_value));
}

void test_print2(SB_Label_Map **ppp_map, int pv_value) {
    printf("%d=%s\n", pv_value, SB_get_label_maps(ppp_map, pv_value));
}

int main() {
    int lv_inx;
    for (lv_inx = T1_L1 - 1; lv_inx <= T1_LMAX + 1; lv_inx++)
        test_print(&gv_test1_label_map, lv_inx);
    printf("\n");
    for (lv_inx = T2_L1 - 1; lv_inx <= T2_LMAX + 1; lv_inx++)
        test_print(&gv_test2_label_map, lv_inx);
    printf("\n");
    for (lv_inx = T3A_L1 - 1; lv_inx <= T3B_LMAX + 1; lv_inx++)
        test_print2(ga_test3_label_map, lv_inx);
    return 0;
}
#endif
