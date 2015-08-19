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

#ifndef __SB_ATOMIC_H_
#define __SB_ATOMIC_H_

//
// Atomic int
// (Encapsulate int and allow atomic operations)
//
class SB_Atomic_Int {
public:
    inline SB_Atomic_Int() : iv_val(0) {
    }

    inline ~SB_Atomic_Int() {
    }

    inline int add_and_fetch(int pv_add) {
        return __sync_add_and_fetch_4(&iv_val, pv_add);
    }

    inline void add_val(int pv_add) {
        __sync_add_and_fetch_4(&iv_val, pv_add);
    }

    inline int read_val() {
        return iv_val;
    }

    inline void set_val(int pv_val) {
        iv_val = pv_val;
    }

    inline int sub_and_fetch(int pv_sub) {
        return __sync_sub_and_fetch_4(&iv_val, pv_sub);
    }

    inline void sub_val(int pv_sub) {
        __sync_sub_and_fetch_4(&iv_val, pv_sub);
    }

private:
    int iv_val;
};

//
// Atomic long-long
// (Encapsulate int and allow atomic operations)
//
class SB_Atomic_Long_Long {
public:
    inline SB_Atomic_Long_Long() : iv_val(0) {
    }

    inline ~SB_Atomic_Long_Long() {
    }

    inline long long add_and_fetch(long long pv_add) {
        return __sync_add_and_fetch_8(&iv_val, pv_add);
    }

    inline void add_val(long long pv_add) {
        __sync_add_and_fetch_8(&iv_val, pv_add);
    }

    inline long long read_val() {
        return iv_val;
    }

    inline void set_val(long long pv_val) {
        iv_val = pv_val;
    }

    inline long long sub_and_fetch(long long pv_sub) {
        return __sync_sub_and_fetch_8(&iv_val, pv_sub);
    }

    inline void sub_val(long long pv_sub) {
        __sync_sub_and_fetch_8(&iv_val, pv_sub);
    }

private:
    long long iv_val;
};

#endif // !__SB_ATOMIC_H_
