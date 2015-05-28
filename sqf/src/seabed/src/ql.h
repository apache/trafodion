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
// Implement queue, thread-safe queue, and signaled queue
//

#ifndef __SB_QL_H_
#define __SB_QL_H_

#include "seabed/int/types.h" // SB_Int64_Type

#ifdef SQ_PHANDLE_VERIFIER
typedef struct sb_npv_type {
    int           iv_nid;
    int           iv_pid;
    SB_Verif_Type iv_verif;
} SB_NPV_Type;
#endif

// singly-linked (for covenience contains prev)
typedef struct sb_ql_type {
    struct sb_ql_type *ip_next;
    struct sb_ql_type *ip_prev;
    int                iv_qid;
    int                iv_qid_last;
    union {
        int                i;
        long               l;
        SB_Int64_Type      ll;
#ifdef SQ_PHANDLE_VERIFIER
        SB_NPV_Type        npv;
#endif
    } iv_id;
} SB_QL_Type;
// doubly-linked (match SB_QL_Type)
typedef struct sb_dql_type {
    struct sb_dql_type *ip_next;
    struct sb_dql_type *ip_prev;
    int                 iv_qid;
    int                 iv_qid_last;
    union {
        int                i;
        long               l;
        SB_Int64_Type      ll;
#ifdef SQ_PHANDLE_VERIFIER
        SB_NPV_Type        npv;
#endif
    } iv_id;
} SB_DQL_Type;

#endif // !__SB_QL_H_
