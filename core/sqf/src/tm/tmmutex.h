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

#ifndef TMMUTEX_H_
#define TMMUTEX_H_

#include "seabed/thread.h"


// --------------------------------------------------------
//
// class TM_Mutex
//
// Purpose : Wrap SB_Thread::Mutex usage for uniformity
//
// --------------------------------------------------------
class TM_Mutex : SB_Thread::Mutex
{
    public:
        TM_Mutex();
        TM_Mutex(bool pv_recursive, bool pv_error_check);
        ~TM_Mutex(){}

        int lock();
        int  lock_count() {return iv_mutex.__data.__count;}
        long lock_owner() {return iv_mutex.__data.__owner;}
        int unlock();
};

#endif



