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

#ifndef TMPOOL_H_
#define TMPOOL_H_

#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>

#include "seabed/ms.h"
#include "seabed/trace.h"
#include "tmlogging.h"
#include "tmmap.h"
#include "tmdeque.h"
#include "tmtime.h"
#include "tmlibmsg.h"

// EID state values
#define EID_STATE_DELETED 'X'
#define EID_STATE_INUSE 'U'
#define EID_STATE_FREE 'F'

// CTmPool class definition
// This class encapsulates the generalized TM pool 
// T is the class type for the elements contained in the pool.  For example, the 
// transaction object pool uses class TM_TX_Info.
// Notes:
// 1. Elements must be initialized once returned by a call to CTmPool::newElement.
// 2. Element classes must externalize a cleanup() method as this is called by deleteElement.
template<class T> class CTmPool
{
private:

   TM_MAP   *ip_inUseList;
   TM_DEQUE *ip_freeList;

// Configuration
   int32    iv_max;        // Maximum size for pool
   int32    iv_steadyLow;  // Below this new requests always create a new element.
   int32    iv_steadyHigh; // Above this deleted elements are not returned to 
                           // the free list.

// Statistics
   bool  iv_collectStats;        // True = collect stats.
                                 // Set by DTM_TM_STATS=1 registry value, default is TM_STATS.

   Ctimeval iv_startTime;        // Time since pool was instantiated
   Ctimeval iv_lastTimeInterval; // Time since last timeSinceLastCumStats() call.
   int32 iv_poolThresholdEventCounter;
   int32 iv_total;               // Total elements in in-use and free lists.

   // Total values.  This is the total number since startup/reset.
   int32 iv_totalAllocs_new;       // Allocations instantiating a new object.
   int32 iv_totalAllocs_free;      // Allocations reusing elements from the free list.
   int32 iv_totalDeallocs_free;    // Deallocations returned to free list.
   int32 iv_totalDeallocs_delete;  // Deallocations deleting object.

   int32 iv_perSecCum_allocs_new;      // Cumulative allocations per sec 
   int32 iv_perSecCum_allocs_free;     // Cumulative allocations per sec 
   int32 iv_perSecCum_deallocs_free;   // Cumulative deallocations per sec
   int32 iv_perSecCum_deallocs_delete; // Cumulative deallocations per sec
   int32 iv_perSecCumSq_allocs_new;     // Cumulative squares of allocations per sec
   int32 iv_perSecCumSq_allocs_free;    // Cumulative squares of allocations per sec
   int32 iv_perSecCumSq_deallocs_free;  // Cumulative squares of deallocations per sec
   int32 iv_perSecCumSq_deallocs_delete;// Cumulative squares of deallocations per sec

   TM_Mutex  iv_mutex;

public:
//----------------------------------------------------------------------------
// CTmPool methods
// All CTmPool methods must be placed in the header file because
// the compiler can't cope with them being in a .cpp!
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CTmPool Constructor
//----------------------------------------------------------------------------
   CTmPool(bool pv_collectStats, int32 pv_max, int32 pv_steadyLow, 
           int32 pv_steadyHigh)
                 : iv_max(pv_max), iv_steadyLow(pv_steadyLow), 
                   iv_steadyHigh(pv_steadyHigh),
                   iv_collectStats(pv_collectStats), 
                   iv_poolThresholdEventCounter(0), iv_total(0),
                   iv_perSecCum_allocs_new(0), iv_perSecCum_allocs_free(0), 
                   iv_perSecCum_deallocs_free(0), iv_perSecCum_deallocs_delete(0), 
                   iv_perSecCumSq_allocs_new(0), iv_perSecCumSq_allocs_free(0), 
                   iv_perSecCumSq_deallocs_free(0), iv_perSecCumSq_deallocs_delete(0)
{
   char la_buf[DTM_STRING_BUF_SIZE];
   ip_inUseList = new TM_MAP();
   ip_freeList = new TM_DEQUE();
   resetCounters();


   if (iv_max < iv_steadyHigh ||
       iv_steadyHigh < iv_steadyLow ||
       iv_steadyLow < 0)
   {
      // EMS DTM_BAD_PARAM
      sprintf(la_buf, "CTmPool Fatal error. Pool configuration "
         "parameter mismatch. steadyLow %d, steadyHigh %d, max %d\n", 
         iv_steadyLow, iv_steadyHigh, iv_max);
      //TMTrace(1, ("CTmPool<class T>::CTmPool : %s", la_buf));
      tm_log_write(DTM_BAD_PARAM, SQ_LOG_CRIT, la_buf);
      abort();
   }

   //TMTrace(2, ("CTmPool<class T>::CTmPool : EXIT.\n"));
} //CTmPool<class T>::CTmPool

// --------------------------------------------------------------
// CTmPool<class T> Destructor
// --------------------------------------------------------------
   ~CTmPool()
{
   //TMTrace(2, ("CTmPool<class T>::~CTmPool : ENTRY/EXIT for pool %p\n", (void *) this));
   delete ip_inUseList;
   delete ip_freeList;
}

   int elementSize() {return sizeof(T);}
   TM_MAP   *get_inUseList() {return ip_inUseList;}
   TM_DEQUE *get_freeList() {return ip_freeList;}
   int32 get_maxPoolSize() {return iv_max;}
   int32 get_ssHigh() {return iv_steadyHigh;}
   int32 get_ssLow() {return iv_steadyLow;}
   int32 totalElements() {return iv_total;}
   void dec_totalElements() {
       lock();
       iv_total--;
       unlock();
   }

   void set_maxPoolSize(int32 pv_newMax) 
   {
      lock();
      iv_max=pv_newMax;
      unlock();
   }

   void set_ssHigh(int32 pv_new) 
   {
      lock();
      iv_steadyHigh=pv_new;
      unlock();
   }

   void set_ssLow(int32 pv_new) 
   {
      lock();
      iv_steadyLow=pv_new;
      unlock();
   }


// ------------------------------------------------------------
// CTmPool<class T>::get
// Purpose : find an element in the inUseList and return it
// ------------------------------------------------------------
   T * get(int64 pv_index)
   {
      return (T *) ip_inUseList->get(pv_index);
   }


// ------------------------------------------------------------
// CTmPool<class T>::getFirst_inUseList
// Purpose : find the first element in the inUseList and return it
// ------------------------------------------------------------
   T * getFirst_inUseList()
   {
      return (T *) ip_inUseList->get_first();
   }


// ------------------------------------------------------------
// CTmPool<class T>::getNext_inUseList
// Purpose : find the next element in the inUseList and return it
// ------------------------------------------------------------
   T * getNext_inUseList()
   {
      return (T *) ip_inUseList->get_next();
   }


// ------------------------------------------------------------
// CTmPool<class T>::getEnd_inUseList
// Purpose : Unlock the inUseList when we've finished.  Must be called
// by any code which calls getFirst_inUseList.
// ------------------------------------------------------------
   void getEnd_inUseList()
   {
      ip_inUseList->get_end();
   }


// ------------------------------------------------------------
// CTmPool<class T>::getFirst_freeList
// Purpose : find the first element in the freeList and return it
// ------------------------------------------------------------
   T * getFirst_freeList()
   {
      ip_freeList->lock();
      return (T *) ip_freeList->get_firstFIFO();
   }


// ------------------------------------------------------------
// CTmPool<class T>::getNext_freeList
// Purpose : find the next element in the freeList and return it
// ------------------------------------------------------------
   T * getNext_freeList()
   {
      return (T *) ip_freeList->get_nextFIFO();
   }


// ------------------------------------------------------------
// CTmPool<class T>::getEnd_freeList
// Purpose : Unlock the freeList when we've finished.  Must be called
// by any code which calls getFirst_freeList.
// ------------------------------------------------------------
   void getEnd_freeList()
   {
      ip_freeList->unlock();
   }


//----------------------------------------------------------------------------
// CTmPool<class T>::newElement
// Purpose : returns an element either from the free list, or instantiates one
// depending on availability (free list) and the configuration.  This and in  
// deleteElement is where the pool allocation algorithm is defined.
// T will be the class type of the element to be instantiated.
// pv_index is the next available index number. 
//   if pv_reuseIndex is true, pv_index will be used only if allocating a 
//      new element.
//   if pv_reuseIndex is false, pv_index will always be used as the index to
//      the new element.
// pp_reused is returned true of an element was reused from the freeList.
// constructPoolElement is a pointer to the function to be called if supplied.  This is used
//              to call the static function constructPoolElement in a derived class when
//              2 or more classes derive from the base class that implements the CTmPoolElement.
//----------------------------------------------------------------------------
   T * newElement(int64 pv_index, bool * pp_reused, bool pv_reuseIndex=true, 
                  void * (*constructPoolElement)(int64) =NULL)
   {
      int64 lv_index = pv_index;
      char la_buf[DTM_STRING_BUF_SIZE];
      T *lp_element = NULL;
      
      //TMTrace (2, ("CTmPool<class T>::newElement : ENTRY index " PFLL ".\n", pv_index));

      lock();
      // If we have less elements than the steady state low, then 
      // instantiate a newElement one.  Otherwise try to get one from the free list.
      if (iv_total >= iv_steadyLow)
      {
         lp_element = (T *) ip_freeList->pop();
         // If we got a NULL pointer back from pop() then freeList is empty.
         // Retrieve the thread number and reuse it
         if (lp_element)
         {
            lv_index = lp_element->cleanPoolElement();
            //TMTrace (3, ("CTmPool<class T>::newElement : Element %p allocated from Free list,"
            //             " total elements %d, inUseList size " PFLL ", freeList size " PFLL "\n", 
            //             (void *) lp_element, iv_total, ip_inUseList->size(), ip_freeList->size()));
         }
      }

      // Don't allow more than iv_max elements to be instantiated
      if (lp_element == NULL && iv_total >= iv_max)
      {
         unlock();
         *pp_reused = false;
         //sprintf(la_buf, "CTmPool Exceeded the maximum elements of %d.\n", iv_max);
         //tm_log_write(DTM_TMPOOL_MAX_EXCEEDED, SQ_LOG_CRIT, la_buf);
         //TMTrace (1, ("CTmPool<class T>::newElement : Exceeded maximum elements of %d" 
         //                ", inUseList size " PFLL ", freeList size " PFLL "\n", 
         //                iv_max, ip_inUseList->size(), ip_freeList->size()));
         return NULL;
      }

      // Instantiate a newElement element if we didn't get one from the freeList
      // Note that elements must be initialized by the caller after calling CTmPool<class T>::newElement.
      if (lp_element == NULL)
      {
         // If the constructPoolElement function pointer was passed in use it
         if (constructPoolElement != NULL)
            lp_element = (T *) constructPoolElement(lv_index);
         else
            lp_element = T::constructPoolElement(lv_index);
         iv_totalAllocs_new++;
         *pp_reused = false;
         iv_total++;
         //TMTrace (3, ("CTmPool<class T>::newElement : newElement element %p instantiated, "
         //             "total elements %d, inUseList size " PFLL ", freeList size " PFLL ".\n",
         //             (void *) lp_element, iv_total, ip_inUseList->size(), ip_freeList->size()));

         // Log an event because we're over the steady state (> iv_steadyHigh elements)
         // We only write an event every 5 times to try to avoid flooding the log.
         if (iv_total > iv_steadyHigh &&
            (iv_poolThresholdEventCounter++ % 5) == 0)
         {
            sprintf(la_buf, "TmPool %p is approaching its maximum size of %d. Currently %d elements.\n",
                  (void *) this, iv_max, iv_total);
            tm_log_write(DTM_TMPOOL_THRESHOLD_EXCEEDED, SQ_LOG_WARNING, la_buf);
            //TMTrace (3, ("CTmPool<class T>::newElement - %s",la_buf));
         }
      }
      else
      {
         // Reuse the element from the free list.
         iv_totalAllocs_free++;
         *pp_reused = true;
         // If we're not reusing the index, then reset to the supplied index now.
         if (pv_reuseIndex == false)
            lv_index = pv_index;
         //TMTrace (3, ("CTmPool<class T>::newElement : reusing element %p "
         //             "total elements %d, inUseList size " PFLL ", freeList size " PFLL ".\n",
         //             (void *) lp_element, iv_total, ip_inUseList->size(), ip_freeList->size()));
      }

      ip_inUseList->put(lv_index, lp_element);
      lp_element->EIDState(EID_STATE_INUSE);
      unlock();

      //TMTrace (2, ("CTmPool<class T>::newElement : EXIT, index " PFLL ", element %p.\n", lv_index, (void *) lp_element));
      return lp_element;
} //CTmPool<class T>::newElement


//----------------------------------------------------------------------------
// CTmPool<class T>::deleteElement
// Purpose : returns an element on the inUseList to the freeList or deletes it
// depending on number of elements allocated and the configuration.
// deleteElement uses pv_index to find the element in the inUseList.
//----------------------------------------------------------------------------
   bool deleteElement(int64 pv_index)
   {
      bool lv_delete = false;
      T *lp_element = NULL;
      char la_buf[DTM_STRING_BUF_SIZE];
      //TMTrace (2, ("CTmPool<class T>::deleteElement : ENTRY index " PFLL ".\n", pv_index));

      lock();
      lp_element = (T *) ip_inUseList->get(pv_index);
      if (lp_element)
         ip_inUseList->remove(pv_index);
      else
      {
         unlock();
         // EMS DTM_BAD_PARAM
         sprintf(la_buf, "CTmPool::deleteElement : Logic Error. Trying to deleteElement "
            "" PFLL " not found in inUseList.\n", 
            pv_index);
         //TMTrace(1, ("CTmPool: %s", la_buf));
         tm_log_write(DTM_LOGIC_ERROR, SQ_LOG_CRIT, la_buf);
         abort();
      }


      // Delete the element if we're above the steady state high
      if (iv_total > iv_steadyHigh)
      {
         //TMTrace (3, ("CTmPool<class T>::deleteElement : Above steady state high, deleting element %p. "
         //             "Total elements %d, inUseList size " PFLL ", freeList size " PFLL ".\n",
         //             (void *) lp_element, iv_total, ip_inUseList->size(), ip_freeList->size()));

         iv_total--;
         lv_delete = true;
         iv_totalDeallocs_delete++;
         lp_element->EIDState(EID_STATE_DELETED);
      }
      else
      {
         // Below the steady state high, put it on the freeList
         //TMTrace (3, ("CTmPool<class T>::deleteElement : Below steady state high, returning "
         //              "element %p to freeList. "
         //             "Total elements %d, inUseList size " PFLL ", freeList size " PFLL ".\n",
         //             (void *) lp_element, iv_total, ip_inUseList->size(), ip_freeList->size()));
         ip_freeList->push(lp_element);
         iv_totalDeallocs_free++;
         lp_element->EIDState(EID_STATE_FREE);
      }
      unlock();

      // We delay the delete to here so that we can unlock the pool mutex prior to the delete.
      // For thread objects the delete will stop execution!
      if (lv_delete)
         delete lp_element;

      //TMTrace (2, ("CTmPool<class T>::deleteElement : EXIT index " PFLL ", element %p deleted.\n",
      //             pv_index, (void *) lp_element));
      return lv_delete;
   } //CTmPool<class T>::deleteElement


//----------------------------------------------------------------------------
// CTmPool<class T>::setConfig
// Purpose : Set the pool configuration.
// It is intended that this will (eventually) allow dynamic reconfiguration
// of TmPools.
// pv_collectStats must always be provided.
// Other parameters are set only if the value is >= 0.
// Returns true if successful.
//----------------------------------------------------------------------------
   bool setConfig(bool pv_collectStats, int32 pv_max=-1, int32 pv_steadyLow=-1, 
                  int32 pv_steadyHigh=-1)
   {
      bool lv_return = false;
      char la_buf[DTM_STRING_BUF_SIZE];
      int32 lv_max = (pv_max >= 0)?pv_max:iv_max;
      int32 lv_steadyLow = (pv_steadyLow >= 0)?pv_steadyLow:iv_steadyLow;
      int32 lv_steadyHigh = (pv_steadyHigh >= 0)?pv_steadyHigh:iv_steadyHigh;

      //TMTrace (2, ("CTmPool<class T>::setConfig : ENTRY setting collectStats %d, "
      //   "max %d, steadyLow %d, steadyHigh %d\n", 
      //   pv_collectStats, pv_max, pv_steadyLow, pv_steadyHigh));

      lock();
      if (lv_max < lv_steadyHigh ||
         lv_steadyHigh < lv_steadyLow ||
         lv_steadyLow < 0)
      {
         // EMS DTM_BAD_PARAM
         sprintf(la_buf, "CTmPool Configuration error. Parameter mismatch. "
            "Configuration change ignored. steadyLow %d, steadyHigh %d, max %d\n", 
            lv_steadyLow, lv_steadyHigh, lv_max);
         //TMTrace(1, ("CTmPool<class T>::setConfig : %s", la_buf));
         tm_log_write(DTM_BAD_PARAM, SQ_LOG_WARNING, la_buf);
      }
      else
      {
         iv_collectStats = pv_collectStats;
         iv_max = lv_max;
         iv_steadyLow = lv_steadyLow;
         iv_steadyHigh = lv_steadyHigh;
         lv_return = true;
      }
      unlock();

      //TMTrace (2, ("CTmPool<class T>::setConfig : EXIT returning %d.\n", lv_return));
      return lv_return;
   } //CTmPool<class T>::setConfig


//----------------------------------------------------------------------------
// CTmPool<class T>::timeSinceLastCumStats
// Purpose : Calculate the number of seconds since the last call.
//----------------------------------------------------------------------------
   int timeSinceLastCumStats()
   {
      int lv_return = 0;
      Ctimeval lv_now = Ctimeval::now();
      //TMTrace(2, ("CTmPool<class T>::timeSinceLastCumStats : ENTRY.\n"));

      lv_return = lv_now.tv_sec - iv_lastTimeInterval.tv_sec;
      iv_lastTimeInterval = lv_now;

      //TMTrace(2, ("CTmPool<class T>::timeSinceLastCumStats : EXIT returning %d secs "
      //   "since last call.\n", lv_return));
      return lv_return;
   } //CTmPool<class T>::timeSinceLastCumStats


//----------------------------------------------------------------------------
// CTmPool<class T>::resetCounters
//----------------------------------------------------------------------------
   void resetCounters()
   {
      Ctimeval lv_now = Ctimeval::now();

      lock();
      iv_startTime = lv_now;
      iv_lastTimeInterval = lv_now;

      iv_totalAllocs_new = 0;
      iv_totalAllocs_free = 0;
      iv_totalDeallocs_free = 0;
      iv_totalDeallocs_delete = 0;
      unlock();

   } //CTmPool<class T>::resetCounters

// --------------------------------------------------------------
// CTmPool<class T>::lock
// Lock the mutex to serialize access to the object
// --------------------------------------------------------------
   void lock()
   {
      //TMTrace (5, ("CTmPool<class T>::lock(%d)\n", iv_lock_count));
      iv_mutex.lock();
   }

// --------------------------------------------------------------
// CTmPool<class T>::unlock
// Unlock the mutex to serialize access to the object
// --------------------------------------------------------------
   void unlock()
   {
      //TMTrace (5, ("CTmPool<class T>::unlock(%d)\n", iv_lock_count));
      iv_mutex.unlock();
   }

// --------------------------------------------------------------
// CTmPool<class T>::getPoolStats
// Retrieves the pool stats for this pool
// --------------------------------------------------------------
   void getPoolStats(TMPOOLSTATS *pp_stats)
   {
      //TMTrace (2, ("CTmPool<class T>::getPoolStats ENTRY.\n"));
      lock();
      pp_stats->iv_lastTimeInterval = timeSinceLastCumStats();
      pp_stats->iv_poolSizeNow = iv_total;
      pp_stats->iv_inUseListNow = ip_inUseList->size();
      pp_stats->iv_freeListNow = ip_freeList->size();
      pp_stats->iv_steadyStateLow = iv_steadyLow;
      pp_stats->iv_steadyStateHigh = iv_steadyHigh;
      pp_stats->iv_max = iv_max;
      pp_stats->iv_poolThresholdEventCounter = iv_poolThresholdEventCounter;
      CdblTime lv_start(iv_startTime);
      pp_stats->iv_startTime = lv_start.get();
      pp_stats->iv_totalAllocs_free = iv_totalAllocs_free;
      pp_stats->iv_totalAllocs_new = iv_totalAllocs_new;
      pp_stats->iv_totalDeallocs_delete = iv_totalDeallocs_delete;
      pp_stats->iv_totalDeallocs_free = iv_totalDeallocs_free;
      unlock();

   } //CTmPool<class T>::getPoolStats


}; //class CTmPool
#endif //TMPOOL_H_
