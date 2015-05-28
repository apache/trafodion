//------------------------------------------------------------------
//
// (C) Copyright 2006-2013 Hewlett-Packard Development Company, L.P.
//
//-@@@-END-COPYRIGHT-@@@--------------------------------------------

//
// Implement map and thread-safe map
//

#ifndef __SB_NPVMAP_H_
#define __SB_NPVMAP_H_

#include "seabed/int/opts.h"

#ifdef SQ_PHANDLE_VERIFIER

#include "seabed/thread.h"

#include "mapcom.h"
#include "ml.h"

class SB_NPVmap_Enum;

const float SB_NPVMAP_DEFAULT_LF = 0.75;

//
// This is an 'long long' (id is long long) map.
//
// Notes:
//   An item is an SB_NPVML_Type.
//   Typically this would be embedded within some data structure.
//   This type of construction makes get's and put's very fast.
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
class SB_NPVmap : public SB_Map_Comm {
public:
    SB_NPVmap(int   pv_buckets = DEFAULT_BUCKETS,
              float pv_lf = SB_NPVMAP_DEFAULT_LF);
    SB_NPVmap(const char *pp_name,
              int         pv_buckets = DEFAULT_BUCKETS,
              float       pv_lf = SB_NPVMAP_DEFAULT_LF);
    virtual ~SB_NPVmap();

    friend class SB_NPVmap_Enum;
    typedef SB_NPV_Type Key_Type;

    bool                    empty();
    virtual void           *get(Key_Type pv_id);
    virtual SB_NPVmap_Enum *keys();
    virtual void            printself(bool pv_traverse);
    virtual void            put(SB_LLML_Type *pp_item);
    virtual void           *remove(Key_Type pv_id);
    virtual void            removeall();
    virtual void            resize(int pv_buckets);
    int                     size();

protected:
#ifdef NPVMAP_CHECK
    void                   check_integrity();
#endif // NPVMAP_CHECK
    int                    hash(Key_Type pv_id, int pv_buckets);
    void                   init();

    enum         { DEFAULT_BUCKETS = 61 };
    SB_NPVML_Type **ipp_HT;
};

class SB_Ts_NPVmap : public SB_NPVmap {
public:
    SB_Ts_NPVmap(int   pv_buckets = DEFAULT_BUCKETS,
                 float pv_lf = SB_NPVMAP_DEFAULT_LF)
    : SB_NPVmap(pv_buckets, pv_lf) {}
    SB_Ts_NPVmap(const char *pp_name,
                 int         pv_buckets = DEFAULT_BUCKETS,
                 float       pv_lf = SB_NPVMAP_DEFAULT_LF)
    : SB_NPVmap(pp_name, pv_buckets, pv_lf) { iv_lock.setname(pp_name); }
    ~SB_Ts_NPVmap() {}

    virtual void *get(Key_Type pv_id);
    virtual void *get_lock(Key_Type pv_id, bool pv_lock);
    virtual void  lock();
    virtual void  printself(bool pv_traverse);
    virtual void  put(SB_NPVML_Type *pp_item);
    virtual void  put_lock(SB_NPVML_Type *pp_item, bool pv_lock);
    virtual void *remove(Key_Type pv_id);
    virtual void *remove_lock(Key_Type pv_id, bool pv_lock);
    virtual void  resize(int pv_buckets);
    void          setlockname(const char *pp_lockname);
    virtual int   trylock();
    virtual void  unlock();

private:
    SB_Thread::MSL   iv_lock;         // for protection
};

class SB_NPVmap_Enum {
public:
    SB_NPVmap_Enum(SB_NPVmap *pp_map);
    virtual ~SB_NPVmap_Enum() {}

    bool           more();
    SB_NPVML_Type *next();

protected:
    SB_NPVmap_Enum() {}

private:
    SB_NPVML_Type *ip_item;
    SB_NPVmap    *ip_map;
    int           iv_count;
    int           iv_hash;
    int           iv_inx;
    int           iv_mod;
};

#ifdef USE_SB_INLINE
#include "npvmap.inl"
#endif

#endif

#endif // !__SB_NPVMAP_H_
