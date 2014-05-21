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

//
// Implement template lru map
//

#ifndef __SB_TLRUMAP_H_
#define __SB_TLRUMAP_H_

#include "seabed/int/opts.h"

#include "llmap.h"
#include "mapcom.h"

template <class I, class T>
class SB_TlruMap_Enum;

//
// This map is similar to a generic SB_Imap,
// but keeps track of least recently used entries.
// When this map reaches the specified capacity,
// the oldest entry is evicted from the map.
//
// If map is constructed with an eviction-callback,
// if an entry is evicted, the eviction-callback will be called.
// (i.e. on a put() when map is at capacity)
//
// get() or put() of an entry will make the entry the newest entry.
//
// get_noage() will not 'age' the entry.
//
// keys() will return an enumeration from newest to oldest.
//
template <class I, class T>
class SB_TlruMap : public SB_Map_Comm {
public:
    typedef void (*Evicted_Cb_Type)(I pv_id, void *pp_item);
    SB_TlruMap(int              pv_cap,
               const char      *pp_name = NULL,
               Evicted_Cb_Type  pv_cb = NULL);
    virtual ~SB_TlruMap();

    friend class SB_TlruMap_Enum<I,T>;

    bool                  empty();
    T                    *get(I pv_id);
    T                    *get_noage(I pv_id);
    SB_TlruMap_Enum<I,T> *keys();
    void                  printself(bool pv_traverse);
    void                  put(I pv_id, T *pp_item);
    T                    *remove(I pv_id);
    void                  removeall();
    void                  resize(int pv_cap);
    int                   size();

protected:
    typedef struct tmap_node {
        SB_LLML_Type      iv_link;
        struct tmap_node *ip_next;
        struct tmap_node *ip_prev;
        T                *ip_item;
    } Tmap_Node;

    char             ia_map_name[20];
    Tmap_Node       *ip_head;
    Tmap_Node       *ip_tail;
    int              iv_cap;
    int              iv_count;
    Evicted_Cb_Type  iv_evicted_cb;
    SB_LLmap         iv_map;
};

template <class I, class T>
class SB_TlruMap_Enum {
public:
    SB_TlruMap_Enum(SB_TlruMap<I,T> *pp_map);
    virtual ~SB_TlruMap_Enum() {}

    bool  more();
    T    *next();

private:
    typedef struct tmap_node {
        SB_LLML_Type      iv_link;
        struct tmap_node *ip_next;
        struct tmap_node *ip_prev;
        T                *ip_item;
    } Tmap_Node;

    SB_TlruMap_Enum() {}
    SB_TlruMap<I,T> *ip_map;
    Tmap_Node       *ip_node;
    int              iv_count;
    int              iv_inx;
    int              iv_mod;
};

#include "tlrumap.inl"

#endif // !__SB_TLRUMAP_H_
