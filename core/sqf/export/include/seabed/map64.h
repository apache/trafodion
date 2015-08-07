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

//
// 64-bit key map.
//
#ifndef __SB_MAP64_H_
#define __SB_MAP64_H_

#include <map>

#include "int/opts.h"

#include "thread.h"

class SB_Export SB_Ts_Map64 {
public:
    SB_Ts_Map64(){};
    ~SB_Ts_Map64(){};

    void           clear();
    bool           end();
    void          *get(SB_Int64_Type key) SB_DIAG_UNUSED;
    void           get_end();
    void          *get_first() SB_DIAG_UNUSED;
    void          *get_next() SB_DIAG_UNUSED;
    void           lock();
    void           put(SB_Int64_Type key, void *data);
    void          *remove(SB_Int64_Type key) SB_DIAG_UNUSED;
    void          *return_all(SB_Int64_Type *size) SB_DIAG_UNUSED;
    SB_Int64_Type  size() SB_DIAG_UNUSED;
    void           unlock();

private:
    typedef std::map<SB_Int64_Type, void*>::const_iterator Iter;
    typedef std::map<SB_Int64_Type, void*>                 Map;

    Iter             iv_iterator;
    Map              iv_map;
    SB_Thread::Mutex iv_mutex;
};

#ifdef USE_SB_INLINE
#include "int/map64.inl"
#endif

#endif //!__SB_MAP64_H_
