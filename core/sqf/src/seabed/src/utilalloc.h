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

#ifndef __SB_UTILALLOC_H_
#define __SB_UTILALLOC_H_

typedef enum { SB_HDR_TYPE_UNKNOWN = 0,
               SB_HDR_TYPE_HDR     = 1,
               SB_HDR_TYPE_FD      = 2,
               SB_HDR_TYPE_OD      = 3,
               SB_HDR_TYPE_CTRL    = 4,
               SB_HDR_TYPE_DATA    = 5
} SB_HDR_TYPES;

typedef struct SB_Hdr_Type {
    SB_HDR_TYPES iv_type;
    int          iv_size;
} SB_Hdr_Type;

extern void *SB_util_alloc(SB_HDR_TYPES pv_hdr_type, int pv_size);
extern void  SB_util_free(SB_Hdr_Type *pp_ds);

#endif // !__SB_UTILALLOC_H_
