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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "seabed/cpu.h"
#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/trace.h"

#include "t137ms.h"
#include "tms.h"
#include "tutil.h"
#include "tutilp.h"


typedef unsigned long long ulonglong;

int         error_detail;
int         ret;
int         ret_attr_count;
int         ret_attr_list[30];
ulonglong   ret_values_list[40];
int         ret_values_len;
int         ret_values_maxlen;

void display_results() {
    int ainx;
    int vinx;

    for (ainx = 0, vinx = 0; ainx < ret_attr_count; ainx++) {
        switch (ret_attr_list[ainx]) {
        case PGIL_AC_S_ULLA_CPU_SUMMARY:
            printf("attr[%d]=%d, val=%llu-%llu.%llu.%llu.%llu.%llu.%llu.%llu.%llu\n",
                   ainx,
                   ret_attr_list[ainx],
                   ret_values_list[vinx],
                   ret_values_list[vinx+1],
                   ret_values_list[vinx+2],
                   ret_values_list[vinx+3],
                   ret_values_list[vinx+4],
                   ret_values_list[vinx+5],
                   ret_values_list[vinx+6],
                   ret_values_list[vinx+7],
                   ret_values_list[vinx+8]);
            vinx += 9;
            break;
        default:
            printf("attr[%d]=%d, val=%llu\n",
                   ainx, ret_attr_list[ainx], ret_values_list[vinx]);
            vinx++;
            break;
        }
    }
}


int main(int argc, char *argv[]) {
    int             inx;
    int             loop = 1000;
    struct timeval  t_elapsed;
    struct timeval  t_start;
    struct timeval  t_stop;
    bool            test_all = true;
    bool            test_bm = false;
    bool            test_error = false;
    bool            test_meminfo = false;
    bool            test_stat = false;
    bool            test_vmstat = false;
    bool            verbose = false;
    TAD             zargs[] = {
      { "-bm",        TA_Bool, TA_NOMAX,    &test_bm      },
      { "-error",     TA_Bool, TA_NOMAX,    &test_error   },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop         },
      { "-meminfo",   TA_Bool, TA_NOMAX,    &test_meminfo },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL          },
      { "-stats",     TA_Bool, TA_NOMAX,    &test_stat    },
      { "-trace",     TA_Bool, TA_NOMAX,    &gv_ms_trace  },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose      },
      { "-vmstat",    TA_Bool, TA_NOMAX,    &test_vmstat  },
      { "",           TA_End,  TA_NOMAX,    NULL          }
    };

    arg_proc_args(zargs, false, argc, argv);
    if (test_bm)
        test_all = false;
    if (test_error)
        test_all = false;
    if (test_meminfo)
        test_all = false;
    if (test_stat)
        test_all = false;
    if (gv_ms_trace) {
        gv_ms_trace_params = true;
        trace_init((char *) "STDOUT", false, argv[0], false);
    }
    if (test_vmstat)
        test_all = false;
    if (test_all) {
        test_error = true;
        test_meminfo = true;
        test_stat = true;
        test_vmstat = true;
    }
    util_test_start(true);

    if (test_meminfo) {
        //
        // /proc/meminfo
        //
        ret_attr_count = 0;
        ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_MEM_TOTAL;
        ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_MEM_FREE;
        ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_BUFFERS;
        ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_CACHED;
        ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_SWAP_CACHED;
        ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_ACTIVE;
        ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_INACTIVE;
        ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_HIGH_TOTAL;
        ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_HIGH_FREE;
        ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_LOW_TOTAL;
        ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_LOW_FREE;
        ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_SWAP_TOTAL;
        ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_SWAP_FREE;
        ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_DIRTY;
        ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_WRITEBACK;
        ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_MAPPED;
        ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_SLAB;
        ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_COMMIT_LIMIT;
        ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_COMMITTED_AS;
        ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_PAGE_TABLES;
        ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_VMALLOC_TOTAL;
        ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_VMALLOC_USED;
        ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_VMALLOC_CHUNK;
        ret_values_maxlen = sizeof(ret_values_list);
        ret = XPROCESSOR_GETINFOLIST_(ret_attr_list,
                                      ret_attr_count,
                                      ret_values_list,
                                      ret_values_maxlen,
                                      &ret_values_len,
                                      &error_detail);
        assert(ret == PGIL_RET_OK);
        if (verbose)
            display_results();
        system("cat /proc/meminfo");
    }

    if (test_stat) {
        //
        // /proc/stat
        //
        ret_attr_count = 2;
        ret_attr_list[0] = PGIL_AC_S_ULLA_CPU_SUMMARY;
        ret_attr_list[1] = PGIL_AC_S_ULL_CPU_CORES;
        ret_values_maxlen = sizeof(ret_values_list);
        ret = XPROCESSOR_GETINFOLIST_(ret_attr_list,
                                      ret_attr_count,
                                      ret_values_list,
                                      ret_values_maxlen,
                                      &ret_values_len,
                                      &error_detail);
        assert(ret == PGIL_RET_OK);
        if (verbose)
            display_results();
        system("cat /proc/stat|grep '^cpu'");
    }

    if (test_vmstat) {
        //
        // /proc/vmstat
        //
        ret_attr_count = 2;
        ret_attr_list[0] = PGIL_AC_VS_ULL_PAGE_IN;
        ret_attr_list[1] = PGIL_AC_VS_ULL_PAGE_OUT;
        ret_values_maxlen = sizeof(ret_values_list);
        ret = XPROCESSOR_GETINFOLIST_(ret_attr_list,
                                      ret_attr_count,
                                      ret_values_list,
                                      ret_values_maxlen,
                                      &ret_values_len,
                                      &error_detail);
        assert(ret == PGIL_RET_OK);
        if (verbose)
            display_results();
        system("cat /proc/vmstat|egrep '(pgpgin|pgpgout)'");
    }

    if (test_error) {
        //
        // Check bounds
        //
        ret = XPROCESSOR_GETINFOLIST_(NULL,
                                      ret_attr_count,
                                      ret_values_list,
                                      ret_values_maxlen,
                                      &ret_values_len,
                                      &error_detail);
        assert(ret == PGIL_RET_BOUNDS);
        assert(error_detail == 1);
        ret = XPROCESSOR_GETINFOLIST_(ret_attr_list,
                                      ret_attr_count,
                                      NULL,
                                      ret_values_maxlen,
                                      &ret_values_len,
                                      &error_detail);
        assert(ret == PGIL_RET_BOUNDS);
        assert(error_detail == 3);
        ret = XPROCESSOR_GETINFOLIST_(ret_attr_list,
                                      ret_attr_count,
                                      ret_values_list,
                                      ret_values_maxlen,
                                      NULL,
                                      &error_detail);
        assert(ret == PGIL_RET_BOUNDS);
        assert(error_detail == 5);
        ret = XPROCESSOR_GETINFOLIST_(ret_attr_list,
                                      ret_attr_count,
                                      ret_values_list,
                                      ret_values_maxlen,
                                      &ret_values_len,
                                      NULL);
        assert(ret == PGIL_RET_BOUNDS);
        // can't check to see if detail is 6

        //
        // Check invalid attribute code
        //
        ret_attr_count = 1;
        ret_attr_list[0] = -1;
        ret_values_maxlen = sizeof(ret_values_list);
        ret = XPROCESSOR_GETINFOLIST_(ret_attr_list,
                                      ret_attr_count,
                                      ret_values_list,
                                      ret_values_maxlen,
                                      &ret_values_len,
                                      &error_detail);
        assert(ret == PGIL_RET_INV_ATT_CODE);
        assert(error_detail == 0);

        //
        // Check short buf
        //
        ret_attr_count = 1;
        ret_attr_list[0] = PGIL_AC_VS_ULL_PAGE_IN;
        ret_values_maxlen = 0;
        ret = XPROCESSOR_GETINFOLIST_(ret_attr_list,
                                      ret_attr_count,
                                      ret_values_list,
                                      ret_values_maxlen,
                                      &ret_values_len,
                                      &error_detail);
        assert(ret == PGIL_RET_FILE_SYSTEM);
        assert(error_detail == XZFIL_ERR_BUFTOOSMALL);
    }

    if (test_bm) {
        //
        // bm
        //
        util_time_timer_start(&t_start);
        for (inx = 0; inx < loop; inx++) {
            ret_attr_count = 0;
            ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_SWAP_TOTAL;
            ret_attr_list[ret_attr_count++] = PGIL_AC_MI_ULL_SWAP_FREE;
            ret_attr_list[ret_attr_count++] = PGIL_AC_VS_ULL_PAGE_IN;
            ret_attr_list[ret_attr_count++] = PGIL_AC_VS_ULL_PAGE_OUT;
            ret_values_maxlen = sizeof(ret_values_list);
            ret = XPROCESSOR_GETINFOLIST_(ret_attr_list,
                                          ret_attr_count,
                                          ret_values_list,
                                          ret_values_maxlen,
                                          &ret_values_len,
                                          &error_detail);
            assert(ret == PGIL_RET_OK);
        }
        util_time_timer_stop(&t_stop);
        util_time_elapsed(&t_start, &t_stop, &t_elapsed);
        double sec = ((double) t_elapsed.tv_sec * 1000000.0 +
                      (double) t_elapsed.tv_usec) / 1000000.0;
        printf("XPROCESSOR_GETINFOLIST_/sec=%f\n", (double) loop/sec);
        printf("XPROCESSOR_GETINFOLIST_, us=%f\n", sec * 1000000.0 / (double) loop);
    }

    util_test_finish(true);
    return 0;
}
