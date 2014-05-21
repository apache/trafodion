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
// Implement int lru map
//

#ifndef __SB_ILRUMAP_H_
#define __SB_ILRUMAP_H_

#include "seabed/int/opts.h"

#include "imap.h"

//
// This map is similar to SB_Imap,
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
class SB_IlruMap : public SB_Imap {
public:
    typedef void (*Evicted_Cb_Type)(Key_Type pv_id, void *pp_item);
    SB_IlruMap(int              pv_cap,
               const char      *pp_name = NULL,
               Evicted_Cb_Type  pv_cb = NULL);
    virtual ~SB_IlruMap();

    friend class SB_IlruMap_Enum;

    bool                  empty();
    virtual void         *get(Key_Type pv_id);
    virtual void         *get_noage(Key_Type pv_id);
    virtual SB_Imap_Enum *keys();
    virtual void          printself(bool pv_traverse);
    virtual void          put(Key_Type pv_id, void *pp_item);
    virtual void         *remove(Key_Type pv_id);
    virtual void          removeall();
    virtual void          resize(int pv_cap);
    int                   size();

protected:
    typedef struct Ilru_Node {
        SB_LML_Type       iv_link;
        struct Ilru_Node *ip_next;
        struct Ilru_Node *ip_prev;
        void             *ip_item;
    } Ilru_Node;
    Ilru_Node       *ip_lru_head;
    Ilru_Node       *ip_lru_tail;
    Evicted_Cb_Type  iv_evicted_cb;
    int              iv_lru_cap;
    int              iv_lru_count;
};

class SB_IlruMap_Enum : public SB_Imap_Enum {
public:
    SB_IlruMap_Enum(SB_IlruMap *pp_map);
    virtual ~SB_IlruMap_Enum() {}

    virtual bool        more();
    virtual SB_ML_Type *next();

private:
    SB_IlruMap_Enum() {}
    SB_IlruMap            *ip_map;
    SB_IlruMap::Ilru_Node *ip_node;
    int                    iv_count;
    int                    iv_inx;
    int                    iv_mod;
};

#ifdef USE_SB_INLINE
#include "ilrumap.inl"
#endif

#endif // !__SB_ILRUMAP_H_
