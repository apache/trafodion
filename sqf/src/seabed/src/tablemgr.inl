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

// constructor
template <class T>
SB_Table_Mgr<T>::SB_Table_Mgr(const char                     *pp_name,
                              SB_Table_Mgr_Alloc::Type        pv_alloc,
                              SB_Table_Mgr_Alloc::Entry_Type  pv_alloc_entry,
                              SB_Table_Entry_Mgr<T>          *pp_entry_mgr,
                              size_t                          pv_cap_init,
                              size_t                          pv_cap_inc)
: ip_table_blks(NULL),
  iv_alloc_entry(pv_alloc_entry),
  iv_cap(pv_cap_init),
  iv_cap_inc(pv_cap_inc),
  iv_cap_init(pv_cap_init),
  iv_cap_max(0),
  iv_inuse(0),
  iv_inuse_hi(0),
  iv_table_blks(0) {
    SB_Slot_Mgr::Alloc_Type lv_alloc;
    size_t                  lv_inx;

    SB_util_assert_stgt(pv_cap_init, 0);
    SB_util_assert_stgt(pv_cap_inc, 0);
    strcpy(ia_table_mgr_name, pp_name);
    ip_entry_mgr = pp_entry_mgr;
    lv_alloc = static_cast<SB_Slot_Mgr::Alloc_Type>(pv_alloc);
    ip_slot_mgr = new SB_Slot_Mgr(pp_name, lv_alloc, static_cast<int>(pv_cap_init));
    ipp_table = new T *[pv_cap_init];
    if (iv_alloc_entry == SB_Table_Mgr_Alloc::ALLOC_ENTRY_BLOCK) {
        pp_entry_mgr->entry_alloc_block(ipp_table, 0, pv_cap_init);
        iv_table_blks = 1;
        ip_table_blks = new size_t[iv_table_blks];
        ip_table_blks[0] = 0;
    } else {
        for (lv_inx = 0; lv_inx < pv_cap_init; lv_inx++)
            ipp_table[lv_inx] = NULL;
    }
}

// destructor
template <class T>
SB_Table_Mgr<T>::~SB_Table_Mgr() {
    T      *lp_entry;
    size_t  lv_count;
    size_t  lv_inx;
    size_t  lv_inx2;
    size_t  lv_off;

    if (iv_alloc_entry == SB_Table_Mgr_Alloc::ALLOC_ENTRY_BLOCK) {
        lv_count = iv_cap_init;
        for (lv_inx = 0; lv_inx < iv_table_blks; lv_inx++) {
            lv_off = ip_table_blks[lv_inx];
            ip_entry_mgr->entry_free_block(ipp_table, lv_off, lv_count);
            for (lv_inx2 = 0; lv_inx2 < lv_count; lv_inx2++)
                ipp_table[lv_off + lv_inx2] = NULL;
            lv_count = iv_cap_inc;
        }
        delete [] ip_table_blks;
    } else {
        for (lv_inx = 0; lv_inx < iv_cap; lv_inx++) {
            lp_entry = ipp_table[lv_inx];
            if (lp_entry != NULL) {
                SB_util_assert_bt(ip_entry_mgr->entry_free(lp_entry,
                                                           lv_inx,
                                                           true));
            }
        }
    }
    delete [] ipp_table;
    delete ip_slot_mgr;
}

// access table
template <class T>
T *SB_Table_Mgr<T>::get_entry(size_t pv_inx) {
    T *lp_entry;

    if (pv_inx <= iv_cap)
        lp_entry = ipp_table[pv_inx];
    else
        lp_entry = NULL;
    return lp_entry;
}

// allocate entry
template <class T>
size_t SB_Table_Mgr<T>::alloc_entry() {
    size_t  *lp_table_blks;
    T      **lpp_table;
    T      **lpp_table_temp;
    size_t   lv_cap;
    size_t   lv_inx;
    size_t   lv_slot;

    if ((iv_inuse + 1) >= iv_cap) {
        // need to expand array - create new array, copy, delete old, reset
        lv_cap = iv_cap + iv_cap_inc;
        lpp_table = new T *[lv_cap];
        for (lv_inx = 0; lv_inx < iv_cap; lv_inx++)
            lpp_table[lv_inx] = ipp_table[lv_inx];
        if (iv_alloc_entry == SB_Table_Mgr_Alloc::ALLOC_ENTRY_BLOCK) {
            ip_entry_mgr->entry_alloc_block(lpp_table, iv_cap, iv_cap_inc);
            // create/copy new block list
            lp_table_blks = new size_t[iv_table_blks + 1];
            for (lv_inx = 0; lv_inx < iv_table_blks; lv_inx++)
                lp_table_blks[lv_inx] = ip_table_blks[lv_inx];
            lp_table_blks[iv_table_blks] = iv_cap;
            iv_table_blks++;
            delete [] ip_table_blks;
            ip_table_blks = lp_table_blks;
        } else {
            ip_entry_mgr->entry_cap_change(iv_cap, lv_cap);
            for (lv_inx = iv_cap; lv_inx < lv_cap; lv_inx++)
                lpp_table[lv_inx] = NULL;
        }
        lpp_table_temp = ipp_table;
        ipp_table = lpp_table;
        delete [] lpp_table_temp;
        iv_cap = lv_cap;
        // need to resize slots
        ip_slot_mgr->resize(static_cast<int>(lv_cap));
    }
    iv_inuse++;
    if (iv_inuse > iv_inuse_hi)
        iv_inuse_hi = iv_inuse;
    if (iv_cap_max)
        SB_util_assert_stlt(iv_inuse, iv_cap_max);
    lv_slot = ip_slot_mgr->alloc();
    if (iv_alloc_entry == SB_Table_Mgr_Alloc::ALLOC_ENTRY_BLOCK)
        ip_entry_mgr->entry_inuse(ipp_table[lv_slot], lv_slot, true);
    else
        ipp_table[lv_slot] = ip_entry_mgr->entry_alloc(lv_slot);
    return lv_slot;
}

// deallocate entry
template <class T>
void SB_Table_Mgr<T>::free_entry(size_t pv_inx) {
    SB_util_assert_stle(pv_inx, iv_cap);
    if (iv_alloc_entry == SB_Table_Mgr_Alloc::ALLOC_ENTRY_BLOCK)
        ip_entry_mgr->entry_inuse(ipp_table[pv_inx], pv_inx, false);
    else {
        if (!ip_entry_mgr->entry_free(ipp_table[pv_inx], pv_inx, false))
            return; // not freed
        ipp_table[pv_inx] = NULL;
    }
    ip_slot_mgr->free_slot(static_cast<int>(pv_inx));
    iv_inuse--;
}

// get capacity
template <class T>
size_t SB_Table_Mgr<T>::get_cap() {
    return iv_cap;
}

// get inuse count
template <class T>
size_t SB_Table_Mgr<T>::get_inuse() {
    return iv_inuse;
}

// get max inuse count
template <class T>
size_t SB_Table_Mgr<T>::get_hi_inuse() {
    return iv_inuse_hi;
}

// print table
template <class T>
void SB_Table_Mgr<T>::print(char *pp_info) {
    size_t lv_inx;

    printf("this=%p, inuse=" PFSIZEOF ", cap=" PFSIZEOF ", cap-inc=" PFSIZEOF "\n",
           static_cast<void *>(this), iv_inuse, iv_cap, iv_cap_inc);
    for (lv_inx = 0; lv_inx < iv_cap; lv_inx++) {
        ip_entry_mgr->entry_print_str(ipp_table[lv_inx], lv_inx, pp_info);
        printf("array[" PFSIZEOF "]=%p, %s\n",
               lv_inx, static_cast<void *>(ipp_table[lv_inx]), pp_info);
    }
}

// set max-cap
template <class T>
void SB_Table_Mgr<T>::set_cap_max(size_t pv_max_cap) {
    iv_cap_max = pv_max_cap;
}

// print table
template <class T>
void SB_Table_Mgr<T>::trace(char *pp_info) {
    size_t lv_inx;

    trace_printf("this=%p, inuse=" PFSIZEOF ", cap=" PFSIZEOF ", cap-inc=" PFSIZEOF "\n",
                 static_cast<void *>(this), iv_inuse, iv_cap, iv_cap_inc);
    for (lv_inx = 0; lv_inx < iv_cap; lv_inx++) {
        ip_entry_mgr->entry_print_str(ipp_table[lv_inx], lv_inx, pp_info);
        trace_printf("array[" PFSIZEOF "]=%p, %s\n",
                     lv_inx, static_cast<void *>(ipp_table[lv_inx]), pp_info);
    }
}

template <class T>
SB_Ts_Table_Mgr<T>::SB_Ts_Table_Mgr(const char                     *pp_name,
                                    SB_Table_Mgr_Alloc::Type        pv_alloc,
                                    SB_Table_Mgr_Alloc::Entry_Type  pv_alloc_entry,
                                    SB_Table_Entry_Mgr<T>          *pp_entry_mgr,
                                    size_t                          pv_cap_init,
                                    size_t                          pv_cap_inc)
: SB_Table_Mgr<T>::SB_Table_Mgr(pp_name,
                                pv_alloc,
                                pv_alloc_entry,
                                pp_entry_mgr,
                                pv_cap_init,
                                pv_cap_inc) {
    iv_lock.setname(pp_name);
}

template <class T>
SB_Ts_Table_Mgr<T>::~SB_Ts_Table_Mgr() {
}

template <class T>
size_t SB_Ts_Table_Mgr<T>::alloc_entry() {
    lock();
    size_t lv_ret = SB_Table_Mgr<T>::alloc_entry();
    unlock();
    return lv_ret;
}

template <class T>
size_t SB_Ts_Table_Mgr<T>::alloc_entry_lock(bool pv_lock) {
    if (pv_lock)
        lock();
    size_t lv_ret = SB_Table_Mgr<T>::alloc_entry();
    if (pv_lock)
        unlock();
    return lv_ret;
}

template <class T>
void SB_Ts_Table_Mgr<T>::free_entry(size_t pv_inx) {
    lock();
    SB_Table_Mgr<T>::free_entry(pv_inx);
    unlock();
}

template <class T>
size_t SB_Ts_Table_Mgr<T>::get_cap() {
    size_t lv_ret = SB_Table_Mgr<T>::get_cap();
    return lv_ret;
}

// since alloc_entry may resize, need to protect
template <class T>
T *SB_Ts_Table_Mgr<T>::get_entry(size_t pv_inx) {
    T *lp_entry;

    lock();
    lp_entry = SB_Table_Mgr<T>::get_entry(pv_inx);
    unlock();
    return lp_entry;
}

// alloc_entry may resize, test to determine protection
template <class T>
T *SB_Ts_Table_Mgr<T>::get_entry_lock(size_t pv_inx, bool pv_lock) {
    T *lp_entry;

    if (pv_lock)
        lock();
    lp_entry = SB_Table_Mgr<T>::get_entry(pv_inx);
    if (pv_lock)
        unlock();
    return lp_entry;
}

template <class T>
size_t SB_Ts_Table_Mgr<T>::get_inuse() {
    size_t lv_ret = SB_Table_Mgr<T>::get_inuse();
    return lv_ret;
}

template <class T>
size_t SB_Ts_Table_Mgr<T>::get_hi_inuse() {
    size_t lv_ret = SB_Table_Mgr<T>::get_hi_inuse();
    return lv_ret;
}

template <class T>
void SB_Ts_Table_Mgr<T>::lock() {
    int lv_status;

    lv_status = iv_lock.lock();
    SB_util_assert_ieq(lv_status, 0);
}

template <class T>
void SB_Ts_Table_Mgr<T>::print(char *pp_info) {
    lock();
    SB_Table_Mgr<T>::print(pp_info);
    unlock();
}

// set max-cap
template <class T>
void SB_Ts_Table_Mgr<T>::set_cap_max(size_t pv_max_cap) {
    SB_Table_Mgr<T>::set_cap_max(pv_max_cap);
}

template <class T>
void SB_Ts_Table_Mgr<T>::trace(char *pp_info) {
    lock();
    SB_Table_Mgr<T>::trace(pp_info);
    unlock();
}

template <class T>
void SB_Ts_Table_Mgr<T>::unlock() {
    int lv_status;

    lv_status = iv_lock.unlock();
    SB_util_assert_ieq(lv_status, 0);
}

