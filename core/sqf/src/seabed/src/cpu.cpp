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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "seabed/cpu.h"
#include "seabed/fserr.h"
#include "seabed/thread.h"
#include "seabed/trace.h"

#include "apictr.h"
#include "mstrace.h"
#include "util.h"

//
// It's unclear why this is needed by GNU
//
#ifndef offsetof
#define offsetof(type,member) (reinterpret_cast<size_t>(&((static_cast<type *>(0))->member)))
#endif

static int         fd_proc_meminfo   = -1;
static int         fd_proc_stat      = -1;
static int         fd_proc_vmstat    = -1;
static const char *file_proc_meminfo = "/proc/meminfo";
static const char *file_proc_stat    = "/proc/stat";
static const char *file_proc_vmstat  = "/proc/vmstat";

enum { BUF_MAX = 16384 };

typedef unsigned long long ulonglong;

//
// look-up type
//
typedef struct LU_Type {
    const char *ip_name;
    size_t      iv_offset;
} LU_Type;

//
// meminfo type
//
typedef struct MID_Type {
    ulonglong iv_Active;
    ulonglong iv_AnonPages;
    ulonglong iv_Bounce;
    ulonglong iv_Buffers;
    ulonglong iv_Cached;
    ulonglong iv_CommitLimit;
    ulonglong iv_Committed_AS;
    ulonglong iv_Dirty;
    ulonglong iv_HighFree;
    ulonglong iv_HighTotal;
    ulonglong iv_HugePages_Free;
    ulonglong iv_HugePages_Rsvd;
    ulonglong iv_HugePages_Total;
    ulonglong iv_Hugepagesize;
    ulonglong iv_Inactive;
    ulonglong iv_LowFree;
    ulonglong iv_LowTotal;
    ulonglong iv_Mapped;
    ulonglong iv_MemFree;
    ulonglong iv_MemTotal;
    ulonglong iv_NFS_Unstable;
    ulonglong iv_PageTables;
    ulonglong iv_Slab;
    ulonglong iv_SwapCached;
    ulonglong iv_SwapFree;
    ulonglong iv_SwapTotal;
    ulonglong iv_VmallocChunk;
    ulonglong iv_VmallocTotal;
    ulonglong iv_VmallocUsed;
    ulonglong iv_Writeback;
} MID_Type;

//
// meminfo data
//
static const LU_Type meminfo_data[] = {
    { "Active",          offsetof(MID_Type, iv_Active)             },
    { "AnonPages",       offsetof(MID_Type, iv_AnonPages)          },
    { "Bounce",          offsetof(MID_Type, iv_Bounce)             },
    { "Buffers",         offsetof(MID_Type, iv_Buffers)            },
    { "Cached",          offsetof(MID_Type, iv_Cached)             },
    { "CommitLimit",     offsetof(MID_Type, iv_CommitLimit)        },
    { "Committed_AS",    offsetof(MID_Type, iv_Committed_AS)       },
    { "Dirty",           offsetof(MID_Type, iv_Dirty)              },
    { "HighFree",        offsetof(MID_Type, iv_HighFree)           },
    { "HighTotal",       offsetof(MID_Type, iv_HighTotal)          },
    { "HugePages_Free",  offsetof(MID_Type, iv_HugePages_Free)     },
    { "HugePages_Rsvd",  offsetof(MID_Type, iv_HugePages_Rsvd)     },
    { "HugePages_Total", offsetof(MID_Type, iv_HugePages_Total)    },
    { "Hugepagesize",    offsetof(MID_Type, iv_Hugepagesize)       },
    { "Inactive",        offsetof(MID_Type, iv_Inactive)           },
    { "LowFree",         offsetof(MID_Type, iv_LowFree)            },
    { "LowTotal",        offsetof(MID_Type, iv_LowTotal)           },
    { "Mapped",          offsetof(MID_Type, iv_Mapped)             },
    { "MemFree",         offsetof(MID_Type, iv_MemFree)            },
    { "MemTotal",        offsetof(MID_Type, iv_MemTotal)           },
    { "NFS_Unstable",    offsetof(MID_Type, iv_NFS_Unstable)       },
    { "PageTables",      offsetof(MID_Type, iv_PageTables)         },
    { "Slab",            offsetof(MID_Type, iv_Slab)               },
    { "SwapCached",      offsetof(MID_Type, iv_SwapCached)         },
    { "SwapFree",        offsetof(MID_Type, iv_SwapFree)           },
    { "SwapTotal",       offsetof(MID_Type, iv_SwapTotal)          },
    { "VmallocChunk",    offsetof(MID_Type, iv_VmallocChunk)       },
    { "VmallocTotal",    offsetof(MID_Type, iv_VmallocTotal)       },
    { "VmallocUsed",     offsetof(MID_Type, iv_VmallocUsed)        },
    { "Writeback",       offsetof(MID_Type, iv_Writeback)          }
};
enum { MEMINFO_DATA_COUNT = sizeof(meminfo_data) / sizeof(LU_Type) };

//
// cpu-stats type
//
typedef struct Cpu_Stats_Type {
    ulong iv_user;
    ulong iv_nice;
    ulong iv_system;
    ulong iv_idle;
    ulong iv_iowait;
    ulong iv_irq;
    ulong iv_softirq;
    ulong iv_steal;
} Cpu_Stats_Type;

//
// stat type
//
typedef struct SD_Type {
    Cpu_Stats_Type iv_cpu_aggr; // aggregate of cpus
    ulong          iv_cores;
} SD_Type;

//
// vmstat type
//
typedef struct VSD_Type {
    ulong iv_allocstall;
    ulong iv_kswapd_inodesteal;
    ulong iv_kswapd_steal;
    ulong iv_nr_anon_pages;
    ulong iv_nr_bounce;
    ulong iv_nr_dirty;
    ulong iv_nr_file_pages;
    ulong iv_nr_mapped;
    ulong iv_nr_page_table_pages;
    ulong iv_nr_slab;
    ulong iv_nr_unstable;
    ulong iv_nr_writeback;
    ulong iv_numa_foreign;
    ulong iv_numa_hit;
    ulong iv_numa_interleave;
    ulong iv_numa_local;
    ulong iv_numa_miss;
    ulong iv_numa_other;
    ulong iv_pageoutrun;
    ulong iv_pgactivate;
    ulong iv_pgalloc_dma;
    ulong iv_pgalloc_dma32;
    ulong iv_pgalloc_high;
    ulong iv_pgalloc_normal;
    ulong iv_pgdeactivate;
    ulong iv_pgfault;
    ulong iv_pgfree;
    ulong iv_pginodesteal;
    ulong iv_pgmajfault;
    ulong iv_pgpgin;
    ulong iv_pgpgout;
    ulong iv_pgrefill_dma;
    ulong iv_pgrefill_dma32;
    ulong iv_pgrefill_high;
    ulong iv_pgrefill_normal;
    ulong iv_pgrotated;
    ulong iv_pgscan_direct_dma;
    ulong iv_pgscan_direct_dma32;
    ulong iv_pgscan_direct_high;
    ulong iv_pgscan_direct_normal;
    ulong iv_pgscan_kswapd_dma;
    ulong iv_pgscan_kswapd_dma32;
    ulong iv_pgscan_kswapd_high;
    ulong iv_pgscan_kswapd_normal;
    ulong iv_pgsteal_dma;
    ulong iv_pgsteal_dma32;
    ulong iv_pgsteal_high;
    ulong iv_pgsteal_normal;
    ulong iv_pswpin;
    ulong iv_pswpout;
    ulong iv_slabs_scanned;
} VSD_Type;

//
// vmstat data
//
static const LU_Type vmstat_data[] = {
    { "allocstall",           offsetof(VSD_Type, iv_allocstall)           },
    { "kswapd_inodesteal",    offsetof(VSD_Type, iv_kswapd_inodesteal)    },
    { "kswapd_steal",         offsetof(VSD_Type, iv_kswapd_steal)         },
    { "nr_anon_pages",        offsetof(VSD_Type, iv_nr_anon_pages)        },
    { "nr_bounce",            offsetof(VSD_Type, iv_nr_bounce)            },
    { "nr_dirty",             offsetof(VSD_Type, iv_nr_dirty)             },
    { "nr_file_pages",        offsetof(VSD_Type, iv_nr_file_pages)        },
    { "nr_mapped",            offsetof(VSD_Type, iv_nr_mapped)            },
    { "nr_page_table_pages",  offsetof(VSD_Type, iv_nr_page_table_pages)  },
    { "nr_slab",              offsetof(VSD_Type, iv_nr_slab)              },
    { "nr_unstable",          offsetof(VSD_Type, iv_nr_unstable)          },
    { "nr_writeback",         offsetof(VSD_Type, iv_nr_writeback)         },
    { "numa_foreign",         offsetof(VSD_Type, iv_numa_foreign)         },
    { "numa_hit",             offsetof(VSD_Type, iv_numa_hit)             },
    { "numa_interleave",      offsetof(VSD_Type, iv_numa_interleave)      },
    { "numa_local",           offsetof(VSD_Type, iv_numa_local)           },
    { "numa_miss",            offsetof(VSD_Type, iv_numa_miss)            },
    { "numa_other",           offsetof(VSD_Type, iv_numa_other)           },
    { "pageoutrun",           offsetof(VSD_Type, iv_pageoutrun)           },
    { "pgactivate",           offsetof(VSD_Type, iv_pgactivate)           },
    { "pgalloc_dma",          offsetof(VSD_Type, iv_pgalloc_dma)          },
    { "pgalloc_dma32",        offsetof(VSD_Type, iv_pgalloc_dma32)        },
    { "pgalloc_high",         offsetof(VSD_Type, iv_pgalloc_high)         },
    { "pgalloc_normal",       offsetof(VSD_Type, iv_pgalloc_normal)       },
    { "pgdeactivate",         offsetof(VSD_Type, iv_pgdeactivate)         },
    { "pgfault",              offsetof(VSD_Type, iv_pgfault)              },
    { "pgfree",               offsetof(VSD_Type, iv_pgfree)               },
    { "pginodesteal",         offsetof(VSD_Type, iv_pginodesteal)         },
    { "pgmajfault",           offsetof(VSD_Type, iv_pgmajfault)           },
    { "pgpgin",               offsetof(VSD_Type, iv_pgpgin)               },
    { "pgpgout",              offsetof(VSD_Type, iv_pgpgout)              },
    { "pgrefill_dma",         offsetof(VSD_Type, iv_pgrefill_dma)         },
    { "pgrefill_dma32",       offsetof(VSD_Type, iv_pgrefill_dma32)       },
    { "pgrefill_high",        offsetof(VSD_Type, iv_pgrefill_high)        },
    { "pgrefill_normal",      offsetof(VSD_Type, iv_pgrefill_normal)      },
    { "pgrotated",            offsetof(VSD_Type, iv_pgrotated)            },
    { "pgscan_direct_dma",    offsetof(VSD_Type, iv_pgscan_direct_dma)    },
    { "pgscan_direct_dma32",  offsetof(VSD_Type, iv_pgscan_direct_dma32)  },
    { "pgscan_direct_high",   offsetof(VSD_Type, iv_pgscan_direct_high)   },
    { "pgscan_direct_normal", offsetof(VSD_Type, iv_pgscan_direct_normal) },
    { "pgscan_kswapd_dma",    offsetof(VSD_Type, iv_pgscan_kswapd_dma)    },
    { "pgscan_kswapd_dma32",  offsetof(VSD_Type, iv_pgscan_kswapd_dma32)  },
    { "pgscan_kswapd_high",   offsetof(VSD_Type, iv_pgscan_kswapd_high)   },
    { "pgscan_kswapd_normal", offsetof(VSD_Type, iv_pgscan_kswapd_normal) },
    { "pgsteal_dma",          offsetof(VSD_Type, iv_pgsteal_dma)          },
    { "pgsteal_dma32",        offsetof(VSD_Type, iv_pgsteal_dma32)        },
    { "pgsteal_high",         offsetof(VSD_Type, iv_pgsteal_high)         },
    { "pgsteal_normal",       offsetof(VSD_Type, iv_pgsteal_normal)       },
    { "pswpin",               offsetof(VSD_Type, iv_pswpin)               },
    { "pswpout",              offsetof(VSD_Type, iv_pswpout)              },
    { "slabs_scanned",        offsetof(VSD_Type, iv_slabs_scanned)        }
};

static int compare_lu(const void *pp_l, const void *pp_r) {
    const LU_Type *lp_l = reinterpret_cast<const LU_Type *>(pp_l);
    const LU_Type *lp_r = reinterpret_cast<const LU_Type *>(pp_r);
    return strcmp(lp_l->ip_name, lp_r->ip_name);
}

//
// read a /proc file (open if necessary)
//
static void proc_read(const char *pp_file, int *pp_fd, char *pp_buf) {
    ssize_t                 lv_cnt;
    SB_Thread::ECM          lv_mutex;
    SB_Thread::Scoped_Mutex lv_smutex(lv_mutex);

    if (*pp_fd == -1)
        *pp_fd = open(pp_file, O_RDONLY);
    if (*pp_fd == -1) {
        pp_buf[0] = '\0';
        return;
    }
    lseek(*pp_fd, 0, SEEK_SET);
    lv_cnt = read(*pp_fd, pp_buf, BUF_MAX-1);
    if (lv_cnt == -1)
        pp_buf[0] = '\0';
    else
        pp_buf[lv_cnt] = '\0';
}

//
// get /proc/meminfo data
//
// Format:
//   <label>: <data>
//
static int get_proc_meminfo(const char *pp_where, MID_Type *pp_meminfo) {
    char       la_buf[BUF_MAX];
    LU_Type   *lp_found;
    char      *lp_head;
    ulonglong *lp_meminfo;
    char      *lp_tail;
    LU_Type    lv_key;

    lp_meminfo = reinterpret_cast<ulonglong *>(pp_meminfo);
    proc_read(file_proc_meminfo, &fd_proc_meminfo, la_buf);
    if (fd_proc_meminfo == -1) {
        if (gv_ms_trace)
            trace_where_printf(pp_where, "problem reading %s errno=%d\n",
                               file_proc_meminfo, errno);
        return XZFIL_ERR_FSERR;
    }
    lp_head = la_buf;
    while (*lp_head) {
        lp_tail = strchr(lp_head, ':');
        if (lp_tail == NULL)
            break;
        *lp_tail = 0;
        lv_key.ip_name = lp_head;
        lp_found = static_cast<LU_Type *>(bsearch(&lv_key,             // key
                                                  meminfo_data,        // base
                                                  MEMINFO_DATA_COUNT,  // nmemb
                                                  sizeof(LU_Type),     // size
                                                  compare_lu));        // compare
        if (lp_found != NULL) {
            lp_meminfo[lp_found->iv_offset/sizeof(ulonglong)] =
              strtoull(&lp_tail[1], &lp_tail, 10);
            lp_head = lp_tail;
        } else
            lp_head = &lp_tail[1];
        lp_tail = strchr(lp_head, '\n');
        if (lp_tail == NULL)
            break;
        lp_head = &lp_tail[1];
    }
    return XZFIL_ERR_OK;
}

//
// get /proc/stat data
//
// Format:
//   <label> <data>...
//
// cpu[x] d0 d1 d2 d3 d4 d5 d6 d7
//
//
static int get_proc_stat(const char *pp_where, SD_Type *pp_stat) {
    char      la_buf[BUF_MAX];
    char     *lp_head;
    char     *lp_tail;

    proc_read(file_proc_stat, &fd_proc_stat, la_buf);
    if (fd_proc_stat == -1) {
        if (gv_ms_trace)
            trace_where_printf(pp_where, "problem reading %s errno=%d\n",
                               file_proc_stat, errno);
        return XZFIL_ERR_FSERR;
    }
    lp_head = la_buf;
    pp_stat->iv_cores = 0;
    while (*lp_head) {
        lp_tail = strchr(lp_head, ' ');
        if (lp_tail == NULL)
            break;
        *lp_tail = 0;
        if (strcmp(lp_head, "cpu") == 0) {
            pp_stat->iv_cpu_aggr.iv_user = strtoul(&lp_tail[1], &lp_tail, 10);
            pp_stat->iv_cpu_aggr.iv_nice = strtoul(&lp_tail[1], &lp_tail, 10);
            pp_stat->iv_cpu_aggr.iv_system = strtoul(&lp_tail[1], &lp_tail, 10);
            pp_stat->iv_cpu_aggr.iv_idle = strtoul(&lp_tail[1], &lp_tail, 10);
            pp_stat->iv_cpu_aggr.iv_iowait = strtoul(&lp_tail[1], &lp_tail, 10);
            pp_stat->iv_cpu_aggr.iv_irq = strtoul(&lp_tail[1], &lp_tail, 10);
            pp_stat->iv_cpu_aggr.iv_softirq = strtoul(&lp_tail[1], &lp_tail, 10);
            pp_stat->iv_cpu_aggr.iv_steal = strtoul(&lp_tail[1], &lp_tail, 10);
            lp_head = lp_tail;
        } else if (memcmp(lp_head, "cpu", 3) == 0) {
            pp_stat->iv_cores++;
            lp_head = &lp_tail[1];
        } else
            break;
        lp_tail = strchr(lp_head, '\n');
        if (lp_tail == NULL)
            break;
        lp_head = &lp_tail[1];
    }
    return XZFIL_ERR_OK;
}

//
// get /proc/vmstat data
//
// Format:
//   <label> <data>
//
static int get_proc_vmstat(const char *pp_where, VSD_Type *pp_vmstat) {
    char  la_buf[BUF_MAX];
    char *lp_head;
    char *lp_tail;

    proc_read(file_proc_vmstat, &fd_proc_vmstat, la_buf);
    if (fd_proc_vmstat == -1) {
        if (gv_ms_trace)
            trace_where_printf(pp_where, "problem reading %s errno=%d\n",
                               file_proc_vmstat, errno);
        return XZFIL_ERR_FSERR;
    }
    lp_head = la_buf;
    while (*lp_head) {
        lp_tail = strchr(lp_head, ' ');
        if (lp_tail == NULL)
            break;
        *lp_tail = 0;
        if (strcmp(lp_head, "pgpgin") == 0) {
            pp_vmstat->iv_pgpgin = strtoul(&lp_tail[1], &lp_tail, 10);
            lp_head = lp_tail;
        } else if (strcmp(lp_head, "pgpgout") == 0) {
            pp_vmstat->iv_pgpgout = strtoul(&lp_tail[1], &lp_tail, 10);
            lp_head = lp_tail;
        } else
            lp_head = &lp_tail[1];
        lp_tail = strchr(lp_head, '\n');
        if (lp_tail == NULL)
            break;
        lp_head = &lp_tail[1];
    }
    return XZFIL_ERR_OK;
}

//
// Emulate PROCESSOR_GETINFOLIST_
//
SB_Export int XPROCESSOR_GETINFOLIST_(int       *pp_ret_attr_list,
                                      int        pv_ret_attr_count,
                                      ulonglong *pp_ret_values_list,
                                      int        pv_ret_values_maxlen,
                                      int       *pp_ret_values_len,
                                      int       *pp_error_detail) {
    const char *WHERE = "BPROCESSOR_GETINFOLIST_";
    int         lv_ac;
    int         lv_ainx;
    int         lv_err;
    bool        lv_get_proc_meminfo;
    bool        lv_get_proc_stat;
    bool        lv_get_proc_vmstat;
    MID_Type    lv_meminfo;
    int         lv_ret;
    int         lv_ret_size;
    SD_Type     lv_stat;
    int         lv_vinx;
    VSD_Type    lv_vmstat;
    SB_API_CTR (lv_zctr, XPROCESSOR_GETINFOLIST_);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER ret_attr_list=%p, ret_attr_count=%d, ret_values_list=%p, ret_values_maxlen=%d, ret_values_len=%p, error_detail=%p\n",
                           pfp(pp_ret_attr_list),
                           pv_ret_attr_count,
                           pfp(pp_ret_values_list),
                           pv_ret_values_maxlen,
                           pfp(pp_ret_values_len),
                           pfp(pp_error_detail));
    //
    // None of the pointers should be NULL (especially error-detail)
    //
    lv_err = 0;
    if (pp_ret_attr_list == NULL)
        lv_err = 1;
    else if (pp_ret_values_list == NULL)
        lv_err = 3;
    else if (pp_ret_values_len == NULL)
        lv_err = 5;
    else if (pp_error_detail == NULL)
        lv_err = 6;
    if (lv_err) {
        lv_ret = PGIL_RET_BOUNDS;
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "EXIT ret=%d, error_detail=%d\n",
                               lv_ret, lv_err);
        if (pp_error_detail != NULL)
            *pp_error_detail = lv_err;
        return lv_ret;
    }

    //
    // First scan checks:
    //   attribute-codes
    //   return buffer big enough
    // Also determines data sources (mi/s/vs)
    //
    lv_get_proc_meminfo = true;
    lv_get_proc_stat = false;
    lv_get_proc_vmstat = false;
    lv_ret_size = 0;
    lv_ret = PGIL_RET_OK;
    for (lv_ainx = 0; lv_ainx < pv_ret_attr_count; lv_ainx++) {
        lv_ac = pp_ret_attr_list[lv_ainx];
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "ret_attr_list[%d]=%d\n", lv_ainx, lv_ac);
        switch (lv_ac) {
        case PGIL_AC_MI_ULL_MEM_TOTAL:
        case PGIL_AC_MI_ULL_MEM_FREE:
        case PGIL_AC_MI_ULL_BUFFERS:
        case PGIL_AC_MI_ULL_CACHED:
        case PGIL_AC_MI_ULL_SWAP_CACHED:
        case PGIL_AC_MI_ULL_ACTIVE:
        case PGIL_AC_MI_ULL_INACTIVE:
        case PGIL_AC_MI_ULL_HIGH_TOTAL:
        case PGIL_AC_MI_ULL_HIGH_FREE:
        case PGIL_AC_MI_ULL_LOW_TOTAL:
        case PGIL_AC_MI_ULL_LOW_FREE:
        case PGIL_AC_MI_ULL_SWAP_TOTAL:
        case PGIL_AC_MI_ULL_SWAP_FREE:
        case PGIL_AC_MI_ULL_DIRTY:
        case PGIL_AC_MI_ULL_WRITEBACK:
        case PGIL_AC_MI_ULL_MAPPED:
        case PGIL_AC_MI_ULL_SLAB:
        case PGIL_AC_MI_ULL_COMMIT_LIMIT:
        case PGIL_AC_MI_ULL_COMMITTED_AS:
        case PGIL_AC_MI_ULL_PAGE_TABLES:
        case PGIL_AC_MI_ULL_VMALLOC_TOTAL:
        case PGIL_AC_MI_ULL_VMALLOC_USED:
        case PGIL_AC_MI_ULL_VMALLOC_CHUNK:
            lv_get_proc_meminfo = true;
            lv_ret_size += static_cast<int>(sizeof(ulonglong));
            break;

        case PGIL_AC_S_ULLA_CPU_SUMMARY:
            lv_get_proc_stat = true;
            lv_ret_size += static_cast<int>((9 * sizeof(ulonglong)));
            break;

        case PGIL_AC_S_ULL_CPU_CORES:
            lv_get_proc_stat = true;
            lv_ret_size += static_cast<int>(sizeof(ulonglong));
            break;

        case PGIL_AC_VS_ULL_PAGE_IN:
        case PGIL_AC_VS_ULL_PAGE_OUT:
            lv_get_proc_vmstat = true;
            lv_ret_size += static_cast<int>(sizeof(ulonglong));
            break;

        default:
            *pp_error_detail = lv_ainx;
            lv_ret = PGIL_RET_INV_ATT_CODE;
            break;
        }
        if (lv_ret != PGIL_RET_OK)
            break;
        if (pv_ret_values_maxlen < lv_ret_size) {
            *pp_error_detail = XZFIL_ERR_BUFTOOSMALL;
            lv_ret = PGIL_RET_FILE_SYSTEM;
        }
    }
    if (lv_ret != PGIL_RET_OK) {
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "EXIT ret=%d, error_detail=%d\n",
                               lv_ret, *pp_error_detail);
        return lv_ret;
    }

    //
    // Get data sources
    //
    if (lv_get_proc_meminfo) {
        lv_err = get_proc_meminfo(WHERE, &lv_meminfo);
        if (lv_err != XZFIL_ERR_OK) {
            *pp_error_detail = lv_err;
            lv_ret = PGIL_RET_FILE_SYSTEM;
        }
    }
    if ((lv_ret == PGIL_RET_OK) && lv_get_proc_stat) {
        lv_err = get_proc_stat(WHERE, &lv_stat);
        if (lv_err != XZFIL_ERR_OK) {
            *pp_error_detail = lv_err;
            lv_ret = PGIL_RET_FILE_SYSTEM;
        }
    }
    if ((lv_ret == PGIL_RET_OK) && lv_get_proc_vmstat) {
        lv_err = get_proc_vmstat(WHERE, &lv_vmstat);
        if (lv_err != XZFIL_ERR_OK) {
            *pp_error_detail = lv_err;
            lv_ret = PGIL_RET_FILE_SYSTEM;
        }
    }
    if (lv_ret != PGIL_RET_OK) {
        if (gv_ms_trace_params)
            trace_where_printf(WHERE, "EXIT ret=%d, error_detail=%d\n",
                               lv_ret, *pp_error_detail);
        return lv_ret;
    }

    //
    // Second scan fills in data
    //
    for (lv_ainx = 0, lv_vinx = 0; lv_ainx < pv_ret_attr_count; lv_ainx++) {
        lv_ac = pp_ret_attr_list[lv_ainx];
        switch (lv_ac) {
        case PGIL_AC_MI_ULL_MEM_TOTAL:
            pp_ret_values_list[lv_vinx] = lv_meminfo.iv_MemTotal;
            break;

        case PGIL_AC_MI_ULL_MEM_FREE:
            pp_ret_values_list[lv_vinx] = lv_meminfo.iv_MemFree;
            break;

        case PGIL_AC_MI_ULL_BUFFERS:
            pp_ret_values_list[lv_vinx] = lv_meminfo.iv_Buffers;
            break;

        case PGIL_AC_MI_ULL_CACHED:
            pp_ret_values_list[lv_vinx] = lv_meminfo.iv_Cached;
            break;

        case PGIL_AC_MI_ULL_SWAP_CACHED:
            pp_ret_values_list[lv_vinx] = lv_meminfo.iv_SwapCached;
            break;

        case PGIL_AC_MI_ULL_ACTIVE:
            pp_ret_values_list[lv_vinx] = lv_meminfo.iv_Active;
            break;

        case PGIL_AC_MI_ULL_INACTIVE:
            pp_ret_values_list[lv_vinx] = lv_meminfo.iv_Inactive;
            break;

        case PGIL_AC_MI_ULL_HIGH_TOTAL:
            pp_ret_values_list[lv_vinx] = lv_meminfo.iv_HighTotal;
            break;

        case PGIL_AC_MI_ULL_HIGH_FREE:
            pp_ret_values_list[lv_vinx] = lv_meminfo.iv_HighFree;
            break;

        case PGIL_AC_MI_ULL_LOW_TOTAL:
            pp_ret_values_list[lv_vinx] = lv_meminfo.iv_LowTotal;
            break;

        case PGIL_AC_MI_ULL_LOW_FREE:
            pp_ret_values_list[lv_vinx] = lv_meminfo.iv_LowFree;
            break;

        case PGIL_AC_MI_ULL_SWAP_TOTAL:
            pp_ret_values_list[lv_vinx] = lv_meminfo.iv_SwapTotal;
            break;

        case PGIL_AC_MI_ULL_SWAP_FREE:
            pp_ret_values_list[lv_vinx] = lv_meminfo.iv_SwapFree;
            break;

        case PGIL_AC_MI_ULL_DIRTY:
            pp_ret_values_list[lv_vinx] = lv_meminfo.iv_Dirty;
            break;

        case PGIL_AC_MI_ULL_WRITEBACK:
            pp_ret_values_list[lv_vinx] = lv_meminfo.iv_Writeback;
            break;

        case PGIL_AC_MI_ULL_MAPPED:
            pp_ret_values_list[lv_vinx] = lv_meminfo.iv_Mapped;
            break;

        case PGIL_AC_MI_ULL_SLAB:
            pp_ret_values_list[lv_vinx] = lv_meminfo.iv_Slab;
            break;

        case PGIL_AC_MI_ULL_COMMIT_LIMIT:
            pp_ret_values_list[lv_vinx] = lv_meminfo.iv_CommitLimit;
            break;

        case PGIL_AC_MI_ULL_COMMITTED_AS:
            pp_ret_values_list[lv_vinx] = lv_meminfo.iv_Committed_AS;
            break;

        case PGIL_AC_MI_ULL_PAGE_TABLES:
            pp_ret_values_list[lv_vinx] = lv_meminfo.iv_PageTables;
            break;

        case PGIL_AC_MI_ULL_VMALLOC_TOTAL:
            pp_ret_values_list[lv_vinx] = lv_meminfo.iv_VmallocTotal;
            break;

        case PGIL_AC_MI_ULL_VMALLOC_USED:
            pp_ret_values_list[lv_vinx] = lv_meminfo.iv_VmallocUsed;
            break;

        case PGIL_AC_MI_ULL_VMALLOC_CHUNK:
            pp_ret_values_list[lv_vinx] = lv_meminfo.iv_VmallocChunk;
            break;

        case PGIL_AC_S_ULLA_CPU_SUMMARY:
            pp_ret_values_list[lv_vinx + 0] = 8;
            pp_ret_values_list[lv_vinx + 1] = lv_stat.iv_cpu_aggr.iv_user;
            pp_ret_values_list[lv_vinx + 2] = lv_stat.iv_cpu_aggr.iv_nice;
            pp_ret_values_list[lv_vinx + 3] = lv_stat.iv_cpu_aggr.iv_system;
            pp_ret_values_list[lv_vinx + 4] = lv_stat.iv_cpu_aggr.iv_idle;
            pp_ret_values_list[lv_vinx + 5] = lv_stat.iv_cpu_aggr.iv_iowait;
            pp_ret_values_list[lv_vinx + 6] = lv_stat.iv_cpu_aggr.iv_irq;
            pp_ret_values_list[lv_vinx + 7] = lv_stat.iv_cpu_aggr.iv_softirq;
            pp_ret_values_list[lv_vinx + 8] = lv_stat.iv_cpu_aggr.iv_steal;
            break;

        case PGIL_AC_S_ULL_CPU_CORES:
            pp_ret_values_list[lv_vinx] = lv_stat.iv_cores;
            break;

        case PGIL_AC_VS_ULL_PAGE_IN:
            pp_ret_values_list[lv_vinx] = lv_vmstat.iv_pgpgin;
            break;

        case PGIL_AC_VS_ULL_PAGE_OUT:
            pp_ret_values_list[lv_vinx] = lv_vmstat.iv_pgpgout;
            break;

        default:
            break;
        }

        //
        // Update where next value goes
        //
        switch (lv_ac) {
        case PGIL_AC_S_ULLA_CPU_SUMMARY:
            if (gv_ms_trace_params)
                trace_where_printf(WHERE, "ret_values_list[%d]=%llu-%llu.%llu.%llu.%llu.%llu.%llu.%llu.%llu\n",
                                   lv_vinx,
                                   pp_ret_values_list[lv_vinx+0],
                                   pp_ret_values_list[lv_vinx+1],
                                   pp_ret_values_list[lv_vinx+2],
                                   pp_ret_values_list[lv_vinx+3],
                                   pp_ret_values_list[lv_vinx+4],
                                   pp_ret_values_list[lv_vinx+5],
                                   pp_ret_values_list[lv_vinx+6],
                                   pp_ret_values_list[lv_vinx+7],
                                   pp_ret_values_list[lv_vinx+8]);
            lv_vinx += 9;
            break;
        default:
            if (gv_ms_trace_params)
                trace_where_printf(WHERE, "ret_values_list[%d]=%llu\n",
                                   lv_vinx,
                                   pp_ret_values_list[lv_vinx]);
            lv_vinx++;
            break;
        }
    }
    *pp_ret_values_len = static_cast<int>(lv_vinx * sizeof(ulonglong));

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT ret=%d, ret_values_len=%d\n",
                           lv_ret,
                           *pp_ret_values_len);
    return lv_ret;
}

