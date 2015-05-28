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

#include <assert.h>

#include "seabed/map64.h"

#include "tutil.h"

int main(int /* argc */, char * /*argv*/ []) {

    SB_Ts_Map64 *lp_map = new SB_Ts_Map64();
    assert(lp_map->size() == 0);
    lp_map->lock();
    lp_map->unlock();

    char *lp_data1 = (char *) "data1";
    lp_map->put(1, lp_data1);
    assert(lp_map->size() == 1);
    assert(lp_map->get(1) == lp_data1);
    assert(lp_map->get(2) == NULL);

    char *lp_data2 = (char *) "data2";
    lp_map->put(2, lp_data2);
    assert(lp_map->size() == 2);
    assert(lp_map->get(1) == lp_data1);
    assert(lp_map->get(2) == lp_data2);
    assert(lp_map->get(3) == NULL);

    SB_Int64_Type lv_size;
    void **lp_data_all = (void **) lp_map->return_all(&lv_size);
    assert(lv_size == 2);
    assert(lp_data_all[0] != NULL);
    assert(lp_data_all[1] != NULL);
    if (lp_data_all[0] == lp_data1)
        assert((lp_data_all[0] == lp_data1) && (lp_data_all[1] == lp_data2));
    else
        assert((lp_data_all[1] == lp_data1) && (lp_data_all[0] == lp_data2));
    void *lp_data1x = lp_map->get_first();
    if (lp_data1x == lp_data1)
        assert(lp_data1x == lp_data1);
    else
        assert(lp_data1x == lp_data2);
    assert(!lp_map->end());
    void *lp_data2x = lp_map->get_next();
    if (lp_data1x == lp_data1)
        assert(lp_data2x == lp_data2);
    else
        assert(lp_data2x == lp_data1);
    void *lp_data3x = lp_map->get_next();
    assert(lp_data3x == NULL);
    assert(lp_map->end());
    lp_map->get_end();
    lp_data1x = lp_map->remove(3);
    assert(lp_data1x == NULL);
    lp_data1x = lp_map->remove(1);
    assert(lp_map->size() == 1);
    assert(lp_data1x == lp_data1);
    lp_map->clear();
    assert(lp_map->size() == 0);
    delete lp_map;

    return 0;
}
