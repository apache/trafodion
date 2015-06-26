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

#ifndef __SB_TABLEMGR_H_
#define __SB_TABLEMGR_H_

#include "seabed/int/opts.h"
#include "seabed/trace.h"

#include "slotmgr.h"
#include "util.h"

//
// Table entry manager
//   Manages table-entries
//
// Usage:
//   User subclasses this and implements the pure-virtual methods.
//   SB_Table_Mgr will call these methods as needed.
//
// Method calls:
//   entry_alloc:
//     action   - allocate new table-entry
//                only called if alloc_entry is ALLOC_ENTRY_DYN
//     input    - pv_inx - table-index
//     return   - new table-entry
//     rqmts    - allocate a table-entry
//   entry_alloc_block:
//     action   - allocate new table-entry block
//                only called if alloc_entry is ALLOC_ENTRY_BLOCK
//     output   - ppp_table - table
//     input    - pv_inx    - table-entry-inx
//              - pv_count  - table-entry-count
//     rqmts    - allocate a block sized to contain pv_count table-entries
//              - must populate ppp_table[pv_inx..pv_inx+pv_count-1]
//   entry_cap_change:
//     action   - capacity change
//                only called if alloc_entry is ALLOC_ENTRY_DYN
//     inputs   - pv_inx - table-entry-inx
//              - pv_cap - table-entry-count
//   entry_free:
//     action   - free table-entry
//                only called if alloc_entry is ALLOC_ENTRY_DYN
//                can deny if !pv_urgent (see below)
//     inputs   - pp_entry  - table-entry
//              - pv_inx    - table-index
//              - pv_urgent - urgent (cannot deny)
//     return   - true if freed; false otherwise
//                (cannot be false if pv_urgent)
//     rqmts    - deallocate a table-entry
//                if pv_urgent must deallocate
//                if !pv_urgent can choose to deallocate or not
//   entry_free_block:
//     action   - free table-entry block
//                only called if alloc_entry is ALLOC_ENTRY_BLOCK
//     inputs   - ppp_table - table-entry
//              - pv_inx    - table-entry-index
//              - pv_count  - table-entry-count
//     rqmts    - deallocate block from ppp_table[pv_inx]
//   entry_inuse:
//     action   - set table-entry inuse (either true or false)
//                only called if alloc_entry is ALLOC_ENTRY_BLOCK
//     inputs   - pp_entry - table-entry
//              - pv_inx   - table-index
//              - pv_inuse - inuse
//   entry_print_str:
//     action   - print table-entry
//     inputs   - pp_entry - table-entry
//              - pv_inx   - table-index
//     output   - pp_info  - string contain table-entry info
//
template <class T>
class SB_Table_Entry_Mgr {
private:
    SB_Ecid_Type iv_aecid_table_entry_mgr; // should be first instance

public:
    SB_Table_Entry_Mgr()
    : iv_aecid_table_entry_mgr(SB_ECID_TABLE_ENTRY_MGR) {
    }
    virtual ~SB_Table_Entry_Mgr() {}
    virtual T    *entry_alloc(size_t pv_inx)
                  = 0;
    virtual void  entry_alloc_block(T      **ppp_table,
                                    size_t   pv_inx,
                                    size_t   pv_count)
                  = 0;
    virtual void  entry_cap_change(size_t pv_inx,
                                   size_t pv_cap)
                  = 0;
    virtual bool  entry_free(T      *pp_entry,
                             size_t  pv_inx,
                             bool    pv_urgent)
                  = 0;
    virtual void  entry_free_block(T      **ppp_table,
                                   size_t   pv_inx,
                                   size_t   pv_count)
                  = 0;
    virtual void  entry_inuse(T      *pp_entry,
                              size_t  pv_inx,
                              bool    pv_inuse)
                  = 0;
    virtual void  entry_print_str(T      *pp_entry,
                                  size_t  pv_inx,
                                  char   *pp_info)
                  = 0;
};

//
// Usage:
//   User references these allocation-types for SB_Table_Mgr constructor
//
class SB_Table_Mgr_Alloc {
public:
    // use these allocation-types - which internally map to slot-mgr-types
    typedef enum {
        ALLOC_SCAN = SB_Slot_Mgr::ALLOC_SCAN,
        ALLOC_FAST = SB_Slot_Mgr::ALLOC_FAST,
        ALLOC_FIFO = SB_Slot_Mgr::ALLOC_FIFO,
        ALLOC_MIN  = SB_Slot_Mgr::ALLOC_MIN
    } Type;
    // use these allocation-entry-types
    typedef enum {
        ALLOC_ENTRY_DYN   = 1, // dynamic alloc/free
        ALLOC_ENTRY_BLOCK = 2  // alloc block
    } Entry_Type;
};

//
// Table manager
//   Manages a table that can have entries allocated/deallocated from it
//   Table access is through SB_Table_Manager.get_entry(<index>)
//
//   If table runs out of space, then a new (enlarged) table
//   is created from old table.
//
//   Table entries are managed through SB_Table_Entry_Mgr
//
// Usage:
//   User creates an SB_Table_Mgr for table-of-<T>.
//   Table management depends on pv_alloc_entry:
//     if ALLOC_ENTRY_DYN:
//       When a new table-entry is needed, the table-manager calls entry_alloc.
//       When a table-entry is not needed, the table-manager calls entry_free.
//     if ALLOC_ENTRY_BLOCK:
//       When a new table-entry-block is needed,
//         the table-manager calls entry_alloc_block.
//       When a table-entry-block is not needed,
//         the table-manager calls entry_free_block.
//   User allocates table entries through either
//     entry_alloc/entry_free (ALLOC_ENTRY_DYN)
//     entry_alloc_block/entry_free_block (ALLOC_ENTRY_BLOCK)
//
template <class T>
class SB_Table_Mgr {
public:
    // constructor
    SB_Table_Mgr(const char                     *pp_name,
                 SB_Table_Mgr_Alloc::Type        pv_alloc,
                 SB_Table_Mgr_Alloc::Entry_Type  pv_alloc_entry,
                 SB_Table_Entry_Mgr<T>          *pp_entry_mgr,
                 size_t                          pv_cap_init,
                 size_t                          pv_cap_inc);
    // destructor
    virtual ~SB_Table_Mgr();

    // alloc entry - return index
    virtual size_t  alloc_entry();
    // free entry
    virtual void    free_entry(size_t pv_inx);
    // get capacity
    virtual size_t  get_cap();
    // table access through here
    virtual T      *get_entry(size_t pv_inx);
    // get inuse
    virtual size_t  get_inuse();
    // get hi
    virtual size_t  get_hi_inuse();
    // print table (info is for entry printing)
    virtual void    print(char *pp_info);
    // set max-cap
    virtual void    set_cap_max(size_t pv_max_cap);
    // trace table (info is for entry tracing)
    virtual void    trace(char *pp_info);

protected:
    typedef SB_Table_Mgr_Alloc::Entry_Type Alloc_Entry_Type;
    char                    ia_table_mgr_name[40]; // table name
    SB_Table_Entry_Mgr<T>  *ip_entry_mgr;          // table entry mgr
    SB_Slot_Mgr            *ip_slot_mgr;           // slot mgr
    size_t                 *ip_table_blks;         // table entry-blocks
    T                     **ipp_table;             // the table
    Alloc_Entry_Type        iv_alloc_entry;        // alloc-entry type
    size_t                  iv_cap;                // capacity of table
    size_t                  iv_cap_inc;            // capacity increment
    size_t                  iv_cap_init;           // initial capacity of table
    size_t                  iv_cap_max;            // max capacity of table
    size_t                  iv_inuse;              // count of inuse entries
    size_t                  iv_inuse_hi;           // hi-count of inuse entries
    size_t                  iv_table_blks;         // table entry-block cnts
};

//
// thread-safe version of SB_Table_Mgr.
//
template <class T>
class SB_Ts_Table_Mgr : public SB_Table_Mgr<T> {
public:
    SB_Ts_Table_Mgr(const char                     *pp_name,
                    SB_Table_Mgr_Alloc::Type        pv_alloc,
                    SB_Table_Mgr_Alloc::Entry_Type  pv_alloc_entry,
                    SB_Table_Entry_Mgr<T>          *pp_entry_mgr,
                    size_t                          pv_cap_init,
                    size_t                          pv_cap_inc);
    virtual ~SB_Ts_Table_Mgr();

    virtual size_t  alloc_entry();
    virtual size_t  alloc_entry_lock(bool pv_lock);
    virtual void    free_entry(size_t pv_inx);
    virtual size_t  get_cap();
    virtual T      *get_entry(size_t pv_inx);
    virtual T      *get_entry_lock(size_t pv_inx, bool pv_lock);
    virtual size_t  get_inuse();
    virtual size_t  get_hi_inuse();
    virtual void    lock();
    virtual void    print(char *pp_info);
    virtual void    set_cap_max(size_t pv_max_cap);
    virtual void    trace(char *pp_info);
    virtual void    unlock();

private:
    SB_Thread::MSL   iv_lock;         // for protection
};

#include "tablemgr.inl"

#endif // !__SB_TABLEMGR_H_
