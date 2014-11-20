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

#ifndef __SB_MSOD_H_
#define __SB_MSOD_H_

#include <mpi.h> // for MPI_Request

#include "seabed/ms.h" // MS_MON_MAX_PROCESS_NAME

#include "cap.h"
#include "sbconst.h"
#include "transport.h"

// Open descriptor
typedef struct Ms_Od_Type {
    bool                    iv_inuse;
    char                    ia_port[MPI_MAX_PORT_NAME+1];
    char                    ia_process_name[MS_MON_MAX_PROCESS_NAME+1];
    char                    ia_prog[SB_MAX_PROG+1];
    SB_Trans::Trans_Stream *ip_stream;
    bool                    iv_death_notif;
    bool                    iv_fs_closed;
    bool                    iv_fs_open;
    SB_Thread::ECM          iv_mutex;
    bool                    iv_need_open;
    int                     iv_nid;
    int                     iv_oid;
    int                     iv_pid;
    int                     iv_ref_count;
    bool                    iv_self;
#ifdef SQ_PHANDLE_VERIFIER
    SB_Verif_Type           iv_verif;
#endif
} Ms_Od_Type;

class Ms_Od_Table_Entry_Mgr : public SB_Table_Entry_Mgr<Ms_Od_Type> {
public:
    Ms_Od_Type *entry_alloc(size_t pv_inx) {
        Ms_Od_Type *lp_od;
        lp_od = new Ms_Od_Type;
        lp_od->iv_inuse = false;
        lp_od->iv_oid = static_cast<int>(pv_inx);
        return lp_od;
    }

    void entry_alloc_block(Ms_Od_Type **ppp_table,
                            size_t      pv_inx,
                            size_t      pv_count) {
        ppp_table = ppp_table; // touch
        pv_inx = pv_inx; // touch
        pv_count = pv_count; // touch
        SB_util_abort("NOT implemented"); // sw fault
    }

    void entry_cap_change(size_t pv_inx, size_t pv_cap) {
        const char *WHERE = "Ms_Od_Table_Entry_Mgr::entry_cap_change";
        gv_ms_cap.iv_od_table = pv_cap;
        if (pv_inx && ((pv_cap % LOG_HIGH) == 0)) {
            sb_log_aggr_cap_log(WHERE,
                                SB_LOG_AGGR_CAP_OD_TABLE_CAP_INC,
                                static_cast<int>(pv_cap));
            if (gv_ms_trace)
                trace_where_printf(WHERE,
                                   "OD-table capacity increased new-cap=%d\n",
                                   static_cast<int>(pv_cap));
        }
    }

    bool entry_free(Ms_Od_Type *pp_od, size_t pv_inx, bool pv_urgent) {
        pv_inx = pv_inx; // touch
        pv_urgent = pv_urgent; // touch
        delete pp_od;
        return true;
    }

    void entry_free_block(Ms_Od_Type **ppp_table,
                          size_t      pv_inx,
                          size_t      pv_count) {
        ppp_table = ppp_table; // touch
        pv_inx = pv_inx; // touch
        pv_count = pv_count; // touch
        SB_util_abort("NOT implemented"); // sw fault
    }

    void entry_inuse(Ms_Od_Type *pp_od, size_t pv_inx, bool pv_inuse) {
        pp_od = pp_od; // touch
        pv_inx = pv_inx; // touch
        pv_inuse = pv_inuse; // touch
        SB_util_abort("NOT implemented"); // sw fault
    }

    void entry_print_str(Ms_Od_Type *pp_od, size_t pv_inx, char *pp_info) {
        pp_od = pp_od; // touch
        pv_inx = pv_inx; // touch
        pp_info = pp_info; // touch
    }

private:
    enum { LOG_HIGH = 100 }; // log every time high moves by this much
};

typedef SB_Ts_Table_Mgr<Ms_Od_Type> Ms_Od_Table_Mgr;

#endif // !__SB_MSOD_H_
