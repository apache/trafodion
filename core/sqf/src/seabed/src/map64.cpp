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

#include "seabed/map64.h"

#ifndef USE_SB_INLINE
#include "seabed/int/map64.inl"
#endif

void *SB_Ts_Map64::return_all(SB_Int64_Type *pp_size) {
    void          **lpp_return;
    SB_Int64_Type   lv_index;
    Iter            lv_iterator;

    lock();
    *pp_size = size();
    lpp_return = new void*[*pp_size];
    for (lv_iterator = iv_map.begin(), lv_index = 0;
        lv_iterator != iv_map.end();
        lv_iterator++) {
        lpp_return[lv_index++] = lv_iterator->second;
    }
    unlock();

    return lpp_return;
}

