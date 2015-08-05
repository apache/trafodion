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

#ifndef __SB_ARRAY_H_
#define __SB_ARRAY_H_

#include <assert.h>

template <class T>
class SB_Array {
public:
    SB_Array(int pv_cap) : iv_cap(pv_cap), iv_size(0) {
        ip_array = new T[pv_cap];
    }

    ~SB_Array() {
        delete [] ip_array;
    }

    void add() {
        assert(iv_size < iv_cap);
        iv_size++;
    }

    int get_cap() {
        return iv_cap;
    }

    T get_val(int pv_inx) {
        return ip_array[pv_inx];
    }

    int get_size() {
        return iv_size;
    }

    void remove(int pv_inx) {
        assert((pv_inx >= 0) && (pv_inx < iv_size));
        iv_size--;
        for (int lv_inx = pv_inx; lv_inx < iv_size; lv_inx++)
            ip_array[lv_inx] = ip_array[lv_inx+1];
    }

    void set_val(int pv_inx, T pv_v) {
        ip_array[pv_inx] = pv_v;
    }

    T *ip_array;

private:
    int   iv_cap;
    int   iv_size;
};

#endif // !__SB_ARRAY_H_
