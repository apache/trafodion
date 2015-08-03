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

#ifndef __SB_TRACEX_H_
#define __SB_TRACEX_H_

#include <pthread.h>

//
// A Trace Mutex
//
class SB_Trace_Mutex {
public:
    SB_Trace_Mutex();
    ~SB_Trace_Mutex();

    void lock();
    void unlock();

private:
    pthread_mutex_t iv_mutex;
};

//
// A Trace Signal-block
//
class SB_Trace_Sig_Block {
public:
    SB_Trace_Sig_Block();
    ~SB_Trace_Sig_Block();

    void lock(sigset_t *pp_set_old);
    void unlock(sigset_t *pp_set_old);

private:
    sigset_t iv_set_all;
};

extern SB_Trace_Mutex     gv_trace_mutex;
extern SB_Trace_Sig_Block gv_trace_sig;

#endif // !__SB_TRACEX_H_
