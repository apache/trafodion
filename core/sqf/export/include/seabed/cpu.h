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
// CPU module
//
#ifndef __SB_CPU_H_
#define __SB_CPU_H_

#include "int/diag.h"
#include "int/exp.h"

//
// Errors returned from XPROCESSOR_GETINFOLIST_
//
enum {
    PGIL_RET_OK           = 0,
    PGIL_RET_FILE_SYSTEM  = 1,
    PGIL_RET_PARAM        = 2,
    PGIL_RET_BOUNDS       = 3,
    PGIL_RET_RSVD         = 4, // not returned
    PGIL_RET_COMM_CPU     = 5, // not returned
    PGIL_RET_COMM_NODE    = 6, // not returned
    PGIL_RET_INV_ATT_CODE = 7
};

//
// Attribute codes for XPROCESSOR_GETINFOLIST_
//
// ULL=unsigned long long attribute
// ULLA=unsigned long long array attribute
//
// MI=/proc/meminfo
// S=/proc/stat
// VS=/proc/vmstat
//
// PGIL_AC_ULLA_S_CPU_SUMMARY:
//   each array element specifies number of 1/100 seconds since system booted.
//   unsigned long long[0]: number of elements: 8
//   unsigned long long[1]: user: normal processes executing in user mode
//   unsigned long long[2]: nice: niced processes executing in user mode
//   unsigned long long[3]: system: processes executing in kernel mode
//   unsigned long long[4]: idle: twiddling thumbs
//   unsigned long long[5]: iowait: waiting for I/O to complete
//   unsigned long long[6]: irq: servicing interrupts
//   unsigned long long[7]: softirq: servicing softirqs
//   unsigned long long[8]: steal: involuntary wait
//
enum {
    // /proc/meminfo items
    PGIL_AC_MI_ULL_MEM_TOTAL       = 100,
    PGIL_AC_MI_ULL_MEM_FREE        = 101,
    PGIL_AC_MI_ULL_BUFFERS         = 102,
    PGIL_AC_MI_ULL_CACHED          = 103,
    PGIL_AC_MI_ULL_SWAP_CACHED     = 104,
    PGIL_AC_MI_ULL_ACTIVE          = 105,
    PGIL_AC_MI_ULL_INACTIVE        = 106,
    PGIL_AC_MI_ULL_HIGH_TOTAL      = 107,
    PGIL_AC_MI_ULL_HIGH_FREE       = 108,
    PGIL_AC_MI_ULL_LOW_TOTAL       = 109,
    PGIL_AC_MI_ULL_LOW_FREE        = 110,
    PGIL_AC_MI_ULL_SWAP_TOTAL      = 111,
    PGIL_AC_MI_ULL_SWAP_FREE       = 112,
    PGIL_AC_MI_ULL_DIRTY           = 113,
    PGIL_AC_MI_ULL_WRITEBACK       = 114,
    PGIL_AC_MI_ULL_MAPPED          = 115,
    PGIL_AC_MI_ULL_SLAB            = 116,
    PGIL_AC_MI_ULL_COMMIT_LIMIT    = 117,
    PGIL_AC_MI_ULL_COMMITTED_AS    = 118,
    PGIL_AC_MI_ULL_PAGE_TABLES     = 119,
    PGIL_AC_MI_ULL_VMALLOC_TOTAL   = 120,
    PGIL_AC_MI_ULL_VMALLOC_USED    = 121,
    PGIL_AC_MI_ULL_VMALLOC_CHUNK   = 122,

    // /proc/stat items
    PGIL_AC_S_ULLA_CPU_SUMMARY     = 200, // see notes above
    PGIL_AC_S_ULL_CPU_CORES        = 201,

    // /proc/vmstat items
    PGIL_AC_VS_ULL_PAGE_IN         = 300,
    PGIL_AC_VS_ULL_PAGE_OUT        = 301
};

//
// Call this to get *this* processor info
//
SB_Export int XPROCESSOR_GETINFOLIST_(int                *ret_attr_list,
                                      int                 ret_attr_count,
                                      unsigned long long *ret_values_list,
                                      int                 ret_values_maxlen,
                                      int                *ret_values_len,
                                      int                *error_detail)
SB_DIAG_UNUSED;


#endif // !__SB_CPU_H_
