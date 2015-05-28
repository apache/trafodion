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

#include <stdio.h>

#include "seabed/sqstate.h"

#include "mstrace.h"


static uint8_t  gv_ms_ic_var8  = 0x01;
static uint16_t gv_ms_ic_var16 = 0x0123;
static uint32_t gv_ms_ic_var32 = 0x01234567;
static uint64_t gv_ms_ic_var64 = static_cast<uint64_t>(0x0123456789abcdefLL);

SQSTATE_IC_VAR_TABLE_BEGIN(ga_ms_ic_var_test_table)
    SQSTATE_IC_VAR_ENTRY(gv_ms_ic_var8),  // test
    SQSTATE_IC_VAR_ENTRY(gv_ms_ic_var16), // test
    SQSTATE_IC_VAR_ENTRY(gv_ms_ic_var32), // test
    SQSTATE_IC_VAR_ENTRY(gv_ms_ic_var64), // test
SQSTATE_IC_VAR_TABLE_END(ga_ms_ic_var_test_table)

SQSTATE_IC_VAR_TABLE_BEGIN(ga_ms_ic_var_trace_table)
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_abandon),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_alloc),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_data),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_data_max),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_detail),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_dialect),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_enable),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_environ),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_errors),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_events),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_evlog),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_file_delta),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_file_fb),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_file_inmem),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_file_maxsize),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_file_nolock),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_file_signal),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_file_unique),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_ic),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_locio),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_md),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_mon),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_name),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_params),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_qalloc),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_ref),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_sm),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_sock),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_stats),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_timer),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_timermap),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_trans),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_verbose),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_wait),
    SQSTATE_IC_VAR_ENTRY(gv_ms_trace_xx),
    SQSTATE_IC_VAR_ENTRY(gv_sb_trace_pthread),
    SQSTATE_IC_VAR_ENTRY(gv_sb_trace_thread),
SQSTATE_IC_VAR_TABLE_END(ga_ms_ic_var_trace_table)

typedef struct Sqstate_Ic_Test_Struct {
    int iv_f1;
    int iv_f2;
    int iv_f3;
} Sqstate_Ic_Test_Struct;

void sb_ic_get_var(char        *pp_var_str,
                   const char **ppp_struct_str,
                   void       **ppp_var_value,
                   long        *pp_var_size,
                   int         *pp_var_flags) {
    Sqstate_Ic_Var         *lp_entry;
    Sqstate_Ic_Test_Struct *lp_test_struct1;
    Sqstate_Ic_Test_Struct *lp_test_struct2;
    static int              lv_inited = false;

    if (!lv_inited) {
        lv_inited = true;
        sqstateic_var_add(ga_ms_ic_var_test_table,
                          sizeof(ga_ms_ic_var_test_table));
        sqstateic_var_add(ga_ms_ic_var_trace_table,
                          sizeof(ga_ms_ic_var_trace_table));
        lp_test_struct1 = new Sqstate_Ic_Test_Struct();
        lp_test_struct1->iv_f1 = 1;
        lp_test_struct1->iv_f2 = 2;
        lp_test_struct1->iv_f3 = 3;
        sqstateic_var_dyn_add("gp_ms_ic_test_dyn_1",
                              lp_test_struct1,
                              sizeof(*lp_test_struct1),
                              false);  // rw
        lp_test_struct2 = new Sqstate_Ic_Test_Struct();
        lp_test_struct2->iv_f1 = 3;
        lp_test_struct2->iv_f2 = 2;
        lp_test_struct2->iv_f3 = 1;
        sqstateic_var_dyn_add("gp_ms_ic_test_dyn_2",
                              lp_test_struct2,
                              sizeof(*lp_test_struct2),
                              true);  // ro
    }

    lp_entry = sqstateic_var_lookup(pp_var_str);
    if (lp_entry == NULL) {
        *ppp_struct_str = NULL;
        *ppp_var_value = NULL;
        *pp_var_size = 0;
        *pp_var_flags = 0;
    } else {
        *ppp_struct_str = lp_entry->var_struct;
        *ppp_var_value = lp_entry->var_addr;
        *pp_var_size = lp_entry->var_size;
        *pp_var_flags = lp_entry->var_flags;
    }
}
