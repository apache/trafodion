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
// Implement long lru map
//

#ifndef __SB_LLRUMAP_H_
#define __SB_LLRUMAP_H_

#include "seabed/int/opts.h"

#include "lmap.h"

//
// This map is similar to SB_Lmap,
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
class SB_LlruMap : public SB_Lmap {
public:
    typedef void (*Evicted_Cb_Type)(Key_Type pv_id, void *pp_item);
    SB_LlruMap(int              pv_cap,
               const char      *pp_name = NULL,
               Evicted_Cb_Type  pv_cb = NULL);
    virtual ~SB_LlruMap();

    friend class SB_LlruMap_Enum;

    bool                  empty();
    virtual void         *get(Key_Type pv_id);
    virtual void         *get_noage(Key_Type pv_id);
    virtual SB_Lmap_Enum *keys();
    virtual void          printself(bool pv_traverse);
    virtual void          put(Key_Type pv_id, void *pp_item);
    virtual void         *remove(Key_Type pv_id);
    virtual void          removeall();
    virtual void          resize(int pv_cap);
    int                   size();

protected:
    typedef struct Llru_Node {
        SB_LML_Type       iv_link;
        struct Llru_Node *ip_next;
        struct Llru_Node *ip_prev;
        void             *ip_item;
    } Llru_Node;
    Llru_Node       *ip_lru_head;
    Llru_Node       *ip_lru_tail;
    Evicted_Cb_Type  iv_evicted_cb;
    int              iv_lru_cap;
    int              iv_lru_count;
};

class SB_LlruMap_Enum : public SB_Lmap_Enum {
public:
    SB_LlruMap_Enum(SB_LlruMap *pp_map);
    virtual ~SB_LlruMap_Enum() {}

    virtual bool        more();
    virtual SB_ML_Type *next();

private:
    SB_LlruMap_Enum() {}
    SB_LlruMap            *ip_map;
    SB_LlruMap::Llru_Node *ip_node;
    int                    iv_count;
    int                    iv_inx;
    int                    iv_mod;
};

#ifdef USE_SB_INLINE
#include "llrumap.inl"
#endif

#endif // !__SB_LLRUMAP_H_
