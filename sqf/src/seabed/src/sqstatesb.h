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

//
// sqstate-sb
//

#ifndef __SB_SQSTATESB_H_
#define __SB_SQSTATESB_H_

#include "seabed/ms.h" // MS_MON_MAX_PROCESS_NAME

#include "sbconst.h"

typedef struct sb_ic_get_opener_od_type {
    char          ia_name[MS_MON_MAX_PROCESS_NAME+1];
    int           iv_nid;
    int           iv_pid;
#ifdef SQ_PHANDLE_VERIFIER
    SB_Verif_Type iv_verif;
#endif
} SB_IC_Get_Opener_Od_Type;
typedef struct sb_ic_get_opener_type {
    int                      iv_count;
    SB_IC_Get_Opener_Od_Type ia_od[1];
} SB_IC_Get_Opener_Type;

typedef struct sb_ic_get_opens_od_type {
    char          ia_process_name[MS_MON_MAX_PROCESS_NAME+1];
    char          ia_prog[SB_MAX_PROG];
    int           iv_nid;
    int           iv_pid;
#ifdef SQ_PHANDLE_VERIFIER
    SB_Verif_Type iv_verif;
#endif
} SB_IC_Get_Opens_Od_Type;
typedef struct sb_ic_get_opens_type {
    int                     iv_count;
    SB_IC_Get_Opens_Od_Type ia_od[1];
} SB_IC_Get_Opens_Type;

#endif // !__SB_SQSTATESB_H_
