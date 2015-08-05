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

#include "tmdeque.h"

#define DEQUE std::deque

// clear deletes all entries in the queue.
void TM_DEQUE::clear()
{
   lock();
   iv_deque.clear();
   unlock();
}

bool TM_DEQUE::empty()
{
   return iv_deque.empty();
}

//------------------------------------------------------------------------
// TM_DEQUE::get_firstFIFO
// Purpose : Read the first element of the queue in FIFO order
// non-destructively.
// Note: The caller must first lock() the queue and unlock() once the final
// call to get_nextFIFO() has been made.
// If the queue is empty, NULL is returned.
//------------------------------------------------------------------------
void *TM_DEQUE::get_firstFIFO()
{
   if (iv_deque.empty())
      return NULL;

   iv_iterator = iv_deque.end();
   iv_iterator--;
   return *iv_iterator;
}

//------------------------------------------------------------------------
// TM_DEQUE::get_nextFIFO
// Purpose : Read the next element of the queue in FIFO order
// non-destructively.  This call must be preceeded by a call to
// get_firstFIFO().
// If the queue is empty or at the end, NULL is returned.
//------------------------------------------------------------------------
void *TM_DEQUE::get_nextFIFO()
{
   if (iv_deque.empty())
      return NULL;

   if (iv_iterator == iv_deque.begin())
      return NULL;

   iv_iterator--;

   return *iv_iterator;
}

//------------------------------------------------------------------------
// TM_DEQUE::push
// Purpose : pushes data to the front of the queue.  This is used
// to push data in FIFO order to the queue.
//------------------------------------------------------------------------
void TM_DEQUE::push(void *pp_data)
{
    lock();
    iv_deque.push_front(pp_data);
    unlock();
}

//------------------------------------------------------------------------
// TM_DEQUE::push_back
// Purpose : pushes data to the back of the queue.  This is used
// to push data ahead of existing elements on the queue.
//------------------------------------------------------------------------
void TM_DEQUE::push_back(void *pp_data)
{
    lock();
    iv_deque.push_back(pp_data);
    unlock();
}

//------------------------------------------------------------------------
// TM_DEQUE::pop
// Purpose : distructively reads the first entry in the queue.
// If the queue is empty, NULL is returned.
//------------------------------------------------------------------------
void *TM_DEQUE::pop()
{
    if (iv_deque.empty())
      return NULL;
    else
    {
      lock();
      void * lp_Top = iv_deque.front();
      iv_deque.pop_front();
      unlock();
      return lp_Top;
    }
}
//------------------------------------------------------------------------
// TM_DEQUE::pop_end
// Purpose : distructively reads the last entry in the queue. This is used
// for FIFO queues.
// If the queue is empty, NULL is returned.
//------------------------------------------------------------------------
void *TM_DEQUE::pop_end()
{
    if (iv_deque.empty())
      return NULL;
    else
    {
      lock();
      void * lp_Bottom = iv_deque.back();
      iv_deque.pop_back();
      unlock();
      return lp_Bottom;
    }
}

//------------------------------------------------------------------------
// TM_DEQUE::erase
// Purpose : Delete the entry at the current iterator.  This is used with
// the get_next method/s to find and then delete an entry in the middle
// of the queue.
// Note: the caller must lock and unlock the TM_DEQUE.
//------------------------------------------------------------------------
void TM_DEQUE::erase()
{
   iv_deque.erase(iv_iterator);
}
//------------------------------------------------------------------------
// TM_DEQUE::remove
// Purpose : Delete the entry specified in pp_data.
//------------------------------------------------------------------------
void TM_DEQUE::remove(void *pp_data)
{
   if (pp_data == NULL)
      return;

   lock();
   void * lp_data = get_firstFIFO();

   while (lp_data != NULL && lp_data != pp_data)
      lp_data = get_nextFIFO();

   if (lp_data == pp_data)
      erase();
   unlock();
}

int64 TM_DEQUE::size()
{
   return iv_deque.size();
}

void TM_DEQUE::lock()
{
   iv_mutex.lock();
}

void TM_DEQUE::unlock()
{
   iv_mutex.unlock();
}

