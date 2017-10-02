/* -*-C++-*-
 *****************************************************************************
 *
 * File:         timeout_data.h
 * Description:  Encapsulation of all the dynamicly set timeout values
 *               
 * Created:      1/12/2000
 * Language:     C++
 *
 *
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
 *
 *****************************************************************************
 */

#ifndef TIMEOUT_DATA_H
#define TIMEOUT_DATA_H

#include "NABasicObject.h"
#include "NAMemory.h"

#include "IpcMessageType.h" // for IpcMessageObjSize,  *IpcMessageBufferPtr,
                            //  and *IpcConstMessageBufferPtr

class ComTdbRoot;

// ********************************************************************** 
//   ====  TimeoutHashTable  ====
// Internal hash table to hold pairs of < tableName, timeoutValue >
// Uses "in place" hashing; i.e., collisions are handled during insert()
// by placing the new entry in the next empty slot in the hash array.
// ( Thus remove() has to do some work to maintain this invariant.)
// ********************************************************************** 


//////////////////////////////////////////////////////////////////////
//  The TimeoutTableEntry is just an entry in the hash table/array
//  The only complexity is the size of the table name. Short table
//  names are kept in place, but the long ones are allocated on the
//  heap instead.
/////////////////////////////////////////////////////////////////////
class TimeoutTableEntry : public NABasicObject
{
public:
  TimeoutTableEntry() : tableNamePtr_(NULL), heap_(NULL) {};
  ~TimeoutTableEntry() { reset(); } ;

  inline char * tableName() // return the table name
  { if ( tableNamePtr_ ) return tableNamePtr_; else return tableName_; };

  void setTableName (char * tableName, CollHeap * heap);

  // deallocate long table name, if needed; set to "unused"
  inline void reset() 
  {
    if ( tableNamePtr_ ) heap_->deallocateMemory(tableNamePtr_) ;
    init();
  };

  inline void init() { tableNamePtr_ = NULL;  used = FALSE; } ;

  // public data members: directly accessed entry fields
  // ( A lax design here, since this class is only used by TimeoutHashTable )
  Lng32 timeoutValue;
  ULng32 hashValue;
  NABoolean used;

private:
  enum { MAX_INTERNAL_TN = 40 }; // long enough for a full guardian file name
  char   tableName_[MAX_INTERNAL_TN]; // for the short names (most common)
  char * tableNamePtr_; // in case the table name is long ( > 40 ; this was
  // meant for those long ANSI table names; not currently used as we only
  // keep guardian file names now.)
  CollHeap * heap_;       // the heap a long tablename is allocated from
};

class TimeoutHashTable : public NABasicObject {
public:

  TimeoutHashTable(CollHeap * heap, ULng32 hashTableSize = 16 );

  ~TimeoutHashTable()
  {
    if ( hashArray_ == NULL ) return; 

    // traverse array, deallocating long names if needed
    for ( ULng32 u=0; u < hashTableSize_; u++ ) hashArray_[u].reset();
    heap_->deallocateMemory(hashArray_);
  };

  void insert(char * tableName,  Lng32 timeoutValue);

  void remove(char * tableName);

  // find and set the timeout value for this table; return FALSE iff not found
  NABoolean getTimeout(char * tableName, Lng32 & timeoutValue );

  // remove all the entries from the hash table
  void clearAll();

  ULng32 entries() { return entries_ ; } ;

  // Methods for packing/unpacking (used for passing information to ESPs)
  IpcMessageObjSize packedLength();    // return the length of the packed obj
  // pack or unpack, and move the pointer ("buffer") accordingly
  void packIntoBuffer(IpcMessageBufferPtr &buffer);
  void unpackObj(IpcConstMessageBufferPtr &buffer);

private:

  ULng32 entries_;         // number of entries in this HashQueue
  ULng32 hashTableSize_;   // current size of the hash table
  ULng32 hashTableOriginalSize_;   // original size of the hash table
  ULng32 tempIndex_; // temporary, set by getTimeout(), used by remove()
  TimeoutTableEntry * hashArray_;   // the hash table itself
  CollHeap * heap_;               // the heap a HashQueue allocates from

  void resizeHashTableIfNeeded();
  TimeoutTableEntry * allocateHashTable(ULng32 size);
  // internal routine: insert entry into H.T., return TRUE iff entry is new
  NABoolean internalInsert( TimeoutTableEntry ** hashTable, 
			    ULng32 hashTableSize,
			    char * tableName,
			    Lng32 timeoutValue );
  Lng32 getTimeoutHashValue(char * tableName);
};

// ********************************************************************** 


/////////////////////////////////////////////////////////
// 
//   TimeoutData  class
// 
/////////////////////////////////////////////////////////
class TimeoutData : public NABasicObject {
public:
  
  TimeoutData(CollHeap *heap, ULng32 estimatedMaxEntries = 16);  // ctor
  
  ~TimeoutData() {} ; // do nothing dtor
  
  // for lock timeout
  void setAllLockTimeout( Lng32 lockTimeoutValue );
  void resetAllLockTimeout();
  void setTableLockTimeout( char * tableName, Lng32 lockTimeoutValue );
  void resetTableLockTimeout( char * tableName );
  NABoolean noLockTimeoutsSet() { return noLockTimeoutsSet_ ; }
  NABoolean getLockTimeout(  char * tableName , Lng32 & timeoutValue );

  // for stream timeout
  void setStreamTimeout( Lng32 streamTimeoutValue );
  void resetStreamTimeout();
  NABoolean isStreamTimeoutSet()   { return streamTimeoutSet_;} ;
  Lng32 getStreamTimeout() { return streamTimeoutValue_;} ;

  // copy relevant data from this TimeoutData object into another object,
  // The "anotherTD" may need to be allocated (if there's relevant data)
  // lnil: gives the list of relevant tables (other timeouts are skipped)
  void copyData( TimeoutData ** anotherTD , 
		 CollHeap * heap,
		 ComTdbRoot * rootTdb ) ;

  // Check if another TimeoutData object is up to date with this one
  // ( lnil: gives the list of relevant tables (other timeouts are skipped)
  //   usingStreams: tells if streams are used in this statement )
  NABoolean isUpToDate( TimeoutData * anotherTD , ComTdbRoot * rootTdb ) ;

  // Methods for packing/unpacking (used for passing information to ESPs)
  IpcMessageObjSize packedLength();    // return the length of the packed obj
  // pack or unpack, and move the pointer ("buffer") accordingly
  void packIntoBuffer(IpcMessageBufferPtr &buffer);
  void unpackObj(IpcConstMessageBufferPtr &buffer);

#if _DEBUG   // for debugging only
  ULng32 entries() { return timeoutsHT_.entries() ; } ;
#endif  

private:

  // For lock timeouts
  NABoolean  noLockTimeoutsSet_; // flag to speed up checking a common case 
  // If noLockTimeoutsSet_ == TRUE, the following 3 fields are not used !!
  NABoolean  forAll_;    // Was the timeout set for all tables (i.e. *)
  Lng32       forAllTimeout_; // Lock timeout value for all tables (if set)
  TimeoutHashTable timeoutsHT_; // List of specified per table lock-timeouts

  // for stream timeout
  NABoolean streamTimeoutSet_ ; // Was the stream timeout set ?
  // If streamTimeoutSet_ == FALSE, the following field is ignored
  Lng32 streamTimeoutValue_;    // The process-global stream timeout

  // an internal utility method: Check if there's relevant timeout data
  NABoolean anyRelevantTimeoutData( ComTdbRoot * rootTdb );
};

#endif



