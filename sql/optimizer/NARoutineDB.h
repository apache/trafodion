/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2002-2015 Hewlett-Packard Development Company, L.P.
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
**********************************************************************/
#ifndef NAROUTINEDB_H
#define NAROUTINEDB_H

#include "Collections.h"
#include "ObjectNames.h"
#include "NAMemory.h"

class NARoutineDBKey;
class NARoutine;
class NARoutineCacheStats;

//
// External class definitions
//
//class NARoutine;

// Used as inital list size to construct a NARoutineDB object
#define NARoutineDB_INIT_SIZE  23

//
// NARoutineDB
// Lookup class for NARoutine
//
class NARoutineDB : public NAKeyLookup<NARoutineDBKey,NARoutine>
{
  friend class NARoutineCacheStatStoredProcedure;

public:
  NARoutineDB(NAMemory *h = 0);

  void       resetAfterStatement();
  NARoutine* get(BindWA *bindWA, const NARoutineDBKey *key);
  void       put(NARoutine *routine);
  NABoolean  cachingMetaData();

  // Defined member functions.
  void setCachingON() { resizeCache(defaultCacheSize_); cacheMetaData_ = TRUE; }
  void setCachingOFF(){ flushCache();                   cacheMetaData_= FALSE; }

  ULng32    getHighWatermarkCache()         { return highWatermarkCache_; }    
  ULng32    getTotalLookupsCount()          { return totalLookupsCount_; }
  ULng32    getTotalCacheHits()             { return totalCacheHits_; }
  NAString        &getMetadata()                   { return metadata; }
  void             setMetadata(const char *meta)   { metadata = meta; }

  inline void      resizeCache(Lng32 sizeInBytes)   { maxCacheSize_ = sizeInBytes;};
  inline void      refreshCacheInThisStatement()   { refreshCacheInThisStatement_=TRUE;}

  void getCacheStats(NARoutineCacheStats & stats);

private:
  // Remove everything from cache.
  void flushCache();

  // Reduce size of cache if it exceeds limit, by using LRU algorithm.
  NABoolean enforceMemorySpaceConstraints();
  Int64     getModifyTime(NARoutine &routine, Int32 &error);
  Int64     getRedefTime(BindWA *bindWA, NARoutine &routine, Int32 &error);
  Int64     getSchemaRedefTimeFromLabel(NARoutine &routine, Int32 &error);


  NAMemory *heap_;

  NABoolean cacheMetaData_;
  NAString  metadata;

  // Maximum, default, and current size of the cache in bytes.
  ULng32 maxCacheSize_;
  ULng32 defaultCacheSize_;
  ULng32 currentCacheSize_;
  // Number of entries in cache.
  ULng32 entries_;

  // List of routines to be deleted.
  // This list is used to collect routines that will be deleted after the statement 
  // completes to avoid affecting compile time.  The delete of the elements in this 
  // list is done in NARoutineDB::resetAfterStatement, which is called indirectly by
  // the CmpStatement destructor.
  LIST(NARoutine *) routinesToDeleteAfterStatement_;

  // This flag indicates that the entries (i.e. NARoutine objects) used during the 
  // current statement should be re-read from disk instead of using the entries
  // already in the cache. This helps to refresh the entries used in the current 
  // statement.
  NABoolean refreshCacheInThisStatement_;

  // Pointer to current location in the cachedRoutineList_. 
  // Used for cache entry replacement purposes
  Int32 replacementCursor_;

  // Statistics counters.
  ULng32 highWatermarkCache_;        // High watermark of currentCacheSize_    
  ULng32 totalLookupsCount_;         // NARoutine entries lookup counter 
  ULng32 totalCacheHits_;            // cache hit counter

}; // class NARoutineDB

#endif
