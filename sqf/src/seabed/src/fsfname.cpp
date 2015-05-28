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

#include "seabed/int/opts.h"

#include <stdio.h>

#include "seabed/fs.h"
#include "seabed/trace.h"

#include "apictr.h"
#include "fstrace.h"
#include "mserrmsg.h"
#include "msutil.h"
#include "msx.h"
#include "util.h"

SB_Export short XFILENAME_TO_PROCESSHANDLE_(const char      *pp_filename,
                                            short            pv_length,
                                            SB_Phandle_Type *pp_processhandle) {
    const char    *WHERE = "XFILENAME_TO_PROCESSHANDLE_";
    char           la_filename[XZSYS_VAL_LEN_FILENAME + 1];
    int            lv_fserr = XZFIL_ERR_OK;
    int            lv_nid;
    int            lv_pid;
#ifdef SQ_PHANDLE_VERIFIER
    SB_Verif_Type  lv_verif;
#endif
    SB_API_CTR    (lv_zctr, XFILENAME_TO_PROCESSHANDLE_);

    if (gv_fs_trace_params)
        trace_where_printf(WHERE,
                           "ENTER filename=%p(%s), length=%d, processhandle=%p\n",
                           pp_filename,
                           pp_filename,
                           pv_length,
                           pfp(pp_processhandle));
    if ((pp_filename == NULL) ||
        (pv_length > XZSYS_VAL_LEN_FILENAME) ||
        (pv_length < 2))
        lv_fserr = XZFIL_ERR_BADPARMVALUE;
    else {
        memcpy(la_filename, pp_filename, pv_length);
        la_filename[pv_length] = '\0';
#ifdef SQ_PHANDLE_VERIFIER
        lv_fserr = msg_mon_get_process_info2(la_filename, &lv_nid, &lv_pid, &lv_verif);
#else
        lv_fserr = msg_mon_get_process_info(la_filename, &lv_nid, &lv_pid);
#endif
        if (lv_fserr == XZFIL_ERR_OK) {
            ms_util_fill_phandle_name(pp_processhandle,
                                      la_filename,
                                      lv_nid,
                                      lv_pid
#ifdef SQ_PHANDLE_VERIFIER
                                     ,lv_verif
#endif
                                     );
        }
    }
    if (gv_fs_trace_params) {
        char la_phandle[MSG_UTIL_PHANDLE_LEN];
        if (lv_fserr == XZFIL_ERR_OK) {
            msg_util_format_phandle(la_phandle, pp_processhandle);
            trace_where_printf(WHERE, "EXIT processhandle=%s, ret=%d\n",
                               la_phandle, lv_fserr);
        } else
            trace_where_printf(WHERE, "EXIT ret=%d\n", lv_fserr);
    }
    return ms_err_rtn(static_cast<short>(lv_fserr));
}

