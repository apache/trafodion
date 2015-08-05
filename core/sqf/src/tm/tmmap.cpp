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

#include <stdlib.h>
#include "tmmap.h"

#ifdef DTM_HASHMAP
#define MAP __gnu_cxx::hash_map
#else
#define MAP std::map
#endif

// clear deletes all entries in the list.
void TM_MAP::clear()
{
   lock();
   iv_map.clear();
   unlock();
}

void *TM_MAP::get (int64 pv_key)
{
    lock();
    MAP<int64, void*>::const_iterator lv_iter;
    lv_iter = iv_map.find (pv_key);
    unlock();

    if (lv_iter == iv_map.end())
        return NULL;
    return lv_iter->second;
}

void *TM_MAP::get_first()
{
    lock();
    iv_iterator = iv_map.begin();
    if (iv_iterator == iv_map.end())
    {
        return NULL;
    }
    return iv_iterator->second;
}

// Start reading the map at pv_key
void *TM_MAP::get_first(int64 pv_key)
{
    lock();
    MAP<int64, void*>::const_iterator lv_iter;
    lv_iter = iv_map.find (pv_key);

    if (lv_iter == iv_map.end())
    {
        return NULL;
    }
    return lv_iter->second;
}

// get the current key value
int64 TM_MAP::curr_key()
{
    return iv_iterator->first;
}

void *TM_MAP::get_next()
{
   iv_iterator++;
   if (iv_iterator == iv_map.end())
        return NULL;
   return iv_iterator->second;
}

void  TM_MAP::get_end()
{
   unlock();
}


void TM_MAP::lock()
{
   iv_mutex.lock();
}

void TM_MAP::put (int64 pv_key, void *pp_data)
{
    std::pair<MAP<int64, void*>::const_iterator, bool> lv_result;
    lock();    
    lv_result = iv_map.insert (std::make_pair(pv_key, pp_data));
    if (lv_result.second != true)
        abort();
    unlock();
}

// erase provides the ability to remove an element from the map
// within a mutex lock.  This is used when walking through the 
// map using get_first, get_next, get_end.
void TM_MAP::erase(int64 pv_key)
{
    iv_map.erase(pv_key);
}

void *TM_MAP::remove (int64 pv_key)
{

    lock();
    MAP<int64, void*>::const_iterator lv_iter;
    lv_iter = iv_map.find (pv_key);
    if (lv_iter == iv_map.end())
    {
       unlock();
       return NULL;
    }

    void * lp_rtn = lv_iter->second;
    iv_map.erase (pv_key);
    unlock(); 
    return lp_rtn;
}

void **TM_MAP::return_all(int64 *pv_size)
{ 
   if (pv_size == NULL)
       return NULL;
 
   lock();
 
   int64 lv_index = 0;
   *pv_size = size(); 
   if (*pv_size <= 0)
   {
        unlock();
        return NULL;
   }

   void **la_return;
   la_return = new void*[*pv_size]; 
   MAP<int64, void*>::const_iterator lv_iterator;
   for (lv_iterator = iv_map.begin(); lv_iterator!=iv_map.end();lv_iterator++)
   {
       la_return[lv_index++] = lv_iterator->second;
   } 
   unlock();
   return la_return;
}
int64 TM_MAP::size()
{
   return iv_map.size();
}

void TM_MAP::unlock()
{
   iv_mutex.unlock();
}

