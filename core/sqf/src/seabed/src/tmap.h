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
// Implement map and thread-safe map
//

#ifndef __SB_TMAP_H_
#define __SB_TMAP_H_

#include "seabed/int/opts.h"

#include "seabed/thread.h"

#include "mapcom.h"

template <class I, class T>
class SB_Tmap_Enum;

const float SB_TMAP_DEFAULT_LF = 0.75;

//
// This is a template map.
//
// constructors:
//   pp_name   : name of map
//   pv_buckets: initial number of buckets
//   pv_lf     : load factor
//
// methods:
//   empty     : returns true if map is empty
//   get       : returns item given a id
//   keys      : returns enumeration of items
//   printself : prints map
//   put       : add item to map
//   remove    : remove item from map
//   removeall : remove all items from map
//   resize    : set new number of buckets
//   size      : returns number of items in map
//
template <class I, class T>
class SB_Tmap : public SB_Map_Comm {
public:
    SB_Tmap(int   pv_buckets = DEFAULT_BUCKETS,
            float pv_lf = SB_TMAP_DEFAULT_LF);
    SB_Tmap(const char *pp_name,
            int         pv_buckets = DEFAULT_BUCKETS,
            float       pv_lf = SB_TMAP_DEFAULT_LF);
    virtual ~SB_Tmap();

    friend class SB_Tmap_Enum<I,T>;

    bool                       empty();
    virtual T                 *get(I pv_id);
    virtual SB_Tmap_Enum<I,T> *keys();
    virtual void               printself(bool pv_traverse);
    virtual void               put(I pv_id, T *pp_item);
    virtual T                 *remove(I pv_id);
    virtual void               removeall();
    virtual void               resize(int pv_buckets);
    int                        size();

protected:
    typedef struct tmap_node {
        I                 iv_id;
        struct tmap_node *ip_next;
        T                *ip_item;
    } Tmap_Node;
#ifdef TMAP_CHECK
    void                  check_integrity();
#endif // TMAP_CHECK
    int                   hash(I pv_id, int pv_buckets);
    void                  init();

    enum       { DEFAULT_BUCKETS = 61 };
    Tmap_Node  **ipp_HT;
};

template <class I, class T>
class SB_Tmap_Enum {
public:
    SB_Tmap_Enum(SB_Tmap<I,T> *pp_map);
    virtual ~SB_Tmap_Enum() {}

    bool  more();
    T    *next();

private:
    typedef struct tmap_node {
        I                 iv_id;
        struct tmap_node *ip_next;
        T                *ip_item;
    } Tmap_Node;

    SB_Tmap_Enum() {}
    SB_Tmap<I,T> *ip_map;
    Tmap_Node    *ip_node;
    int           iv_count;
    int           iv_hash;
    int           iv_inx;
    int           iv_mod;
};

#include "tmap.inl"

#endif // !__SB_TMAP_H_
