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

#include "ilrumap.h"

#ifndef USE_SB_INLINE
#include "ilrumap.inl"
#endif

SB_IlruMap::SB_IlruMap(int              pv_cap,
                       const char      *pp_name,
                       Evicted_Cb_Type  pv_cb)
: SB_Imap(pp_name, pv_cap, 1.0),
  ip_lru_head(NULL), ip_lru_tail(NULL),
  iv_evicted_cb(pv_cb), iv_lru_cap(pv_cap), iv_lru_count(0) {
}

SB_IlruMap::~SB_IlruMap() {
}

