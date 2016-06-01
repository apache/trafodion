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

#include <stdlib.h>

#include "utilalloc.h"

//
// Purpose: alloc space
//
void *SB_util_alloc(SB_HDR_TYPES pv_hdr_type, int pv_size) {
    SB_Hdr_Type *lp_hdr;

    lp_hdr = static_cast<SB_Hdr_Type *>(malloc(pv_size));
#ifdef DEBUG
    memset(lp_hdr, -1, pv_size);
#endif
    lp_hdr->iv_type = pv_hdr_type;
    lp_hdr->iv_size = pv_size;
    return lp_hdr;
}

//
// Purpose: free space
//
void SB_util_free(SB_Hdr_Type *pp_ds) {
#ifdef DEBUG
    memset(pp_ds, -2, pp_ds->iv_size);
#endif
    free(pp_ds);
}

