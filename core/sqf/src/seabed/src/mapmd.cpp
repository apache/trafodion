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

#include <string.h>

#include "mapmd.h"

#ifndef USE_SB_INLINE
#include "mapmd.inl"
#endif

// SB_Md_Md constructor
SB_Md_Map::SB_Md_Map(int         pv_qid,
                     const char *pp_name)
: SB_Imap(pp_name) {
    iv_qid = pv_qid;
    ia_md_map_name[sizeof(ia_md_map_name) - 1] = '\0';
    SB_util_assert_cpne(pp_name, NULL);
    strncpy(ia_md_map_name, pp_name, sizeof(ia_md_map_name) - 1);
}

// SB_Ts_Md_Md constructor
SB_Ts_Md_Map::SB_Ts_Md_Map(int         pv_qid,
                           const char *pp_name)
: SB_Ts_Imap(pp_name), iv_aecid_md_map(SB_ECID_MAP_MD) {
    iv_qid = pv_qid;
    ia_md_map_name[sizeof(ia_md_map_name) - 1] = '\0';
    SB_util_assert_cpne(pp_name, NULL);
    strncpy(ia_md_map_name, pp_name, sizeof(ia_md_map_name) - 1);
}

