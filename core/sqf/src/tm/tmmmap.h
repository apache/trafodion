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

#ifndef TMMMAP_H_
#define TMMMAP_H_

#include <map>

#include <sys/time.h>
#include "tmmutex.h"
#include "dtm/tm_util.h"

//
// General TM_MMAP class to hold Timer specific multimap implementation
//

class TM_MMAP
{
    public:
        TM_MMAP(){};
        ~TM_MMAP(){};

        void clear();
        int64 curr_key();
        bool end();
        void *get (int64 pv_key);
        void *get_first();
        void *get_first(int64 pv_key);
        void *get_first_unprotected();
        void *get_first_unprotected(int64 pv_key);
        void *get_next();
        void *get_next(int64 pv_key);
        void  get_end();

        void lock();
        void put (int64 pv_key, void *pp_data);
        void *remove (int64 pv_key);
        void *remove_first (int64 pv_key);
        void *remove_unprotected (int64 pv_key);
        int remove_all (int64 pv_key);
        void **return_all (int64 *pv_size);
        int64 size();
        void unlock();
        void erase_this();

        int count(int64 pv_key) {return (int) iv_multimap.count(pv_key);}
    
    private:
        std::multimap<int64, void*> iv_multimap;
        std::multimap<int64, void*>::iterator iv_iterator;
        //std::multimap<int64, void*>::key_compare iv_compare;
        TM_Mutex                    iv_mutex;
};

#endif
