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
// Implement string map
//

#ifndef __SB_SMAP_H_
#define __SB_SMAP_H_

#include "seabed/int/opts.h"

#include "mapcom.h"
#include "ql.h"

class SB_Smap_Enum;

const float SB_SMAP_DEFAULT_LF = 0.75;

//
// This is an 'string' (key is string) map.
//
// constructors:
//   pp_name   : name of map
//   pv_buckets: initial number of buckets
//   pv_lf     : load factor
//
// methods:
//   empty     : returns true if map is empty
//   get       : returns value-string given a key
//   getv      : returns value-ptr given a key
//   keys      : returns enumeration of keys
//   printself : prints map
//   put       : add key/value-string to map
//               (note map contains copies of key/value)
//   putv      : add key/value-ptr to map
//               (note map contains a copy of key and specified value-ptr)
//   remove    : remove key/value-string from map
//   removev   : remove key/value-ptr from map
//   removeall : remove all key/values from map
//   resize    : set new number of buckets
//   size      : returns number of items in map
//
class SB_Smap : public SB_Map_Comm {
public:
    SB_Smap(int   pv_buckets = DEFAULT_BUCKETS,
            float pv_lf = SB_SMAP_DEFAULT_LF);
    SB_Smap(const char *pp_name,
            int         pv_buckets = DEFAULT_BUCKETS,
            float       pv_lf = SB_SMAP_DEFAULT_LF);
    virtual ~SB_Smap();

    friend class SB_Smap_Enum;
    typedef const char *Key_Type;

    virtual bool          empty();
    virtual const char   *get(Key_Type pp_key);
    virtual void         *getv(Key_Type pp_key);
    virtual SB_Smap_Enum *keys();
    virtual void          printself(bool pv_traverse);
    virtual void          put(Key_Type pp_key, const char *pp_value);
    virtual void          putv(Key_Type pp_key, void *pp_value);
    virtual void          remove(Key_Type pp_key, char *pp_value);
    virtual void         *removev(Key_Type pp_key);
    virtual void          resize(int pv_buckets);
    virtual int           size();

protected:
#ifdef SMAP_CHECK
    void                  check_integrity();
#endif // SMAP_CHECK
    int                   hash(Key_Type pp_key, int pv_buckets);
    void                  init();

    enum       { DEFAULT_BUCKETS = 61 };
    typedef struct SML_Type {
        SB_QL_Type  iv_link;
        char       *ip_key;
        char       *ip_value;
        void       *ip_vvalue;
        bool        iv_use_vvalue;
    } SML_Type;
    SML_Type   **ipp_HT;
};

class SB_Smap_Enum {
public:
    SB_Smap_Enum(SB_Smap *pp_map);
    virtual ~SB_Smap_Enum() {}

    bool  more();
    char *next();

private:
    SB_Smap_Enum() {}
    SB_Smap::SML_Type *ip_item;
    SB_Smap           *ip_map;
    int                iv_count;
    int                iv_hash;
    int                iv_inx;
    int                iv_mod;
};

#ifdef USE_SB_INLINE
#include "smap.inl"
#endif

#endif // !__SB_SMAP_H_
