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

#ifndef __SB_MSAGGR_H_
#define __SB_MSAGGR_H_

enum    { SB_AGGR_ALIGN_INT_VAL = sizeof(int) - 1 };
#define   SB_AGGR_ALIGN_INT(addr) ((reinterpret_cast<long>(addr) + SB_AGGR_ALIGN_INT_VAL) & ~SB_AGGR_ALIGN_INT_VAL)

typedef struct SB_Ms_Aggr_C2S_Type {
    int iv_reqid;
    int iv_pri;
    int iv_req_ctrl_len;
    int iv_req_data_len;
    int iv_rep_ctrl_max;
    int iv_rep_data_max;
} SB_Ms_Aggr_C2S_Type;

typedef struct SB_Ms_Aggr_S2C_Type {
    int iv_cmsgid;        // client msgid
    int iv_smsgid;        // server msgid
    int iv_rep_ctrl_len;
    int iv_rep_data_len;
} SB_Ms_Aggr_S2C_Type;

#endif // !__SB_MSAGGR_H_
