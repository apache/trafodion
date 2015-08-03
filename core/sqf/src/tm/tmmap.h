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

#ifndef TMMAP_H_
#define TMMAP_H_

#ifdef TMHASHMAP
#include <ext/hash_map>
#else
#include <map>
#endif

#include "tmmutex.h"
#include "dtm/tm_util.h"

//
// General TM MAP class to hold TM representation
// TM_MAP has now been changed to use the hash_map standard template library.
//
#ifdef TMHASHMAP
namespace __gnu_cxx 
{
    template<> struct hash< int64 > 
    {
        size_t operator()( const int64& x ) const 
        {
            return hash< int64 >()( x );
        }
    };
}
#endif

class TM_MAP
{
    public:
        TM_MAP(){};
        ~TM_MAP(){};

        void clear();
        bool end();
        void *get (int64 pv_key);
        void *get_first();
        void *get_first(int64 pv_key);
        int64 curr_key();
        void *get_next();
        void  get_end();

        void lock();
        void put (int64 pv_key, void *pp_data);
        void erase(int64 key);
        void *remove (int64 pv_key);
        void **return_all (int64 *pv_size);
        int64 size();
        void unlock();
    
    private:
#ifdef TMHASHMAP
        __gnu_cxx::hash_map<int64, void*, int64 &hash(int64 key)> iv_map;
        __gnu_cxx::hash_map<int64, void*>::const_iterator iv_iterator;
#else
        std::map<int64, void*> iv_map;
        std::map<int64, void*>::const_iterator iv_iterator;
#endif
        TM_Mutex               iv_mutex;
};

#endif
