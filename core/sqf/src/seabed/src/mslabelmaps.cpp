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


// ---------------------------------------------------------------------------


#include <assert.h>
#include <stdio.h>

#include <string.h>


#include "seabed/int/assert.h"

#include "dtm/tmtransid.h"

#include "mslabelmapsx.h"


bool ms_dialect_format_transid(SB_Transid_Type *pp_transid_in,
                               char            *pp_transid_out,
                               int              pv_transid_out_len) {
    TM_Txid_Internal *lp_transid_int;
    int               lv_len;
    bool              lv_ret;

    lp_transid_int = reinterpret_cast<TM_Txid_Internal *>(pp_transid_in);
    if ((pp_transid_in == NULL) || (lp_transid_int->iv_seq_num == 0)) {
        strcpy(pp_transid_out, "<none>");
        lv_len = 6;
        lv_ret = false;
    } else {
        lv_len = sprintf(pp_transid_out, "(%d,%d)",
                         lp_transid_int->iv_node,
                         lp_transid_int->iv_seq_num);
        lv_ret = true;
    }
    SB_util_assert_ilt(lv_len, pv_transid_out_len);
    pv_transid_out_len = pv_transid_out_len; // touch (in case assert disabled)
    return lv_ret;
}

