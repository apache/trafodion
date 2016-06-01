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
#include "tmmmap.h"

#define MMAP std::multimap

// clear deletes all entries in the list.
void TM_MMAP::clear()
{
   lock();
   iv_multimap.clear();
   unlock();
}

// Return the key value for the current iterator value
int64 TM_MMAP::curr_key()
{
    return iv_iterator->first;
}

void *TM_MMAP::get (int64 pv_key)
{
    lock();
    MMAP<int64, void*>::const_iterator lv_iter;
    lv_iter = iv_multimap.find (pv_key);
    unlock();

    if (lv_iter == iv_multimap.end())
        return NULL;
    return lv_iter->second;
}

void *TM_MMAP::get_first()
{
    lock();
    iv_iterator = iv_multimap.begin();
    if (iv_iterator == iv_multimap.end())
    {
        return NULL;
    }
    return iv_iterator->second;
}

// Start reading the map at pv_key
void *TM_MMAP::get_first(int64 pv_key)
{
    lock();
    iv_iterator = iv_multimap.find (pv_key);
    if (iv_iterator == iv_multimap.end())
    {
        return NULL;
    }
    return iv_iterator->second;
}

// Caller must hold the lock
void *TM_MMAP::get_first_unprotected()
{
    //lock();
    iv_iterator = iv_multimap.begin();
    if (iv_iterator == iv_multimap.end())
    {
        return NULL;
    }
    return iv_iterator->second;
}

// Start reading the map at pv_key
// Caller must hold the lock
void *TM_MMAP::get_first_unprotected(int64 pv_key)
{
    //lock();
    iv_iterator = iv_multimap.find (pv_key);
    if (iv_iterator == iv_multimap.end())
    {
        return NULL;
    }
    return iv_iterator->second;
}

void *TM_MMAP::get_next()
{
   iv_iterator++;
   if (iv_iterator == iv_multimap.end())
        return NULL;
   return iv_iterator->second;
}

// Use this when you want to walk through all 
// entries for a particular key
void *TM_MMAP::get_next(int64 pv_key)
{
   iv_iterator++;
   if (iv_iterator == iv_multimap.end() || iv_iterator->first != pv_key)
        return NULL;
   return iv_iterator->second;
}

void  TM_MMAP::get_end()
{
   unlock();
}


void TM_MMAP::lock()
{
   iv_mutex.lock();
}

void TM_MMAP::put (int64 pv_key, void *pp_data)
{
    MMAP<int64, void*>::const_iterator lv_iter;

    lock();    
    lv_iter = iv_multimap.insert (std::make_pair(pv_key, pp_data));
    unlock();
}

void *TM_MMAP::remove (int64 pv_key)
{

    lock();
    MMAP<int64, void*>::const_iterator lv_iter;
    lv_iter = iv_multimap.find (pv_key);
    if (lv_iter == iv_multimap.end())
    {
       unlock();
       return NULL;
    }

    void * lp_rtn = lv_iter->second;
    iv_multimap.erase (pv_key);
    unlock(); 
    return lp_rtn;
}

void *TM_MMAP::remove_first (int64 pv_key)
{

    lock();
    MMAP<int64, void*>::iterator lv_iter;
    lv_iter = iv_multimap.find (pv_key);
    if (lv_iter == iv_multimap.end())
    {
       unlock();
       return NULL;
    }

    void * lp_rtn = lv_iter->second;
    iv_multimap.erase (lv_iter);
    unlock(); 
    return lp_rtn;
}

// Caller must hold the lock
void *TM_MMAP::remove_unprotected (int64 pv_key)
{

    //lock();
    MMAP<int64, void*>::const_iterator lv_iter;
    lv_iter = iv_multimap.find (pv_key);
    if (lv_iter == iv_multimap.end())
    {
       //unlock();
       return NULL;
    }

    void * lp_rtn = lv_iter->second;
    iv_multimap.erase (pv_key);
    //unlock(); 
    return lp_rtn;
}

// TM_MMAP::remove_all
// Returns the count of elements removed.
// Because this is a multi-map, erase removes all elements
// which match pv_key.
int TM_MMAP::remove_all (int64 pv_key)
{

    lock();
    int lv_rtn = iv_multimap.erase (pv_key);
    unlock(); 
    return lv_rtn;
}

void **TM_MMAP::return_all(int64 *pv_size)
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
   MMAP<int64, void*>::const_iterator lv_iterator;
   for (lv_iterator = iv_multimap.begin(); lv_iterator!=iv_multimap.end();lv_iterator++)
   {
       la_return[lv_index++] = lv_iterator->second;
   } 
   unlock();
   return la_return;
}

int64 TM_MMAP::size()
{
   return iv_multimap.size();
}

void TM_MMAP::unlock()
{
   iv_mutex.unlock();
}

void TM_MMAP::erase_this() 
{
    MMAP<int64, void*>::iterator lv_iter = iv_iterator;

   iv_multimap.erase(lv_iter);
}
