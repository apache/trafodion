/**********************************************************************
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
**********************************************************************/
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         timeout_data.cpp
 * Description:  Methods for TimeoutData, an encapsulation of all the
 *               dynamically set timeout values 
 *               
 * Created:      1/12/2000
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

#include "NAStdlib.h" // Redefines strcpy(), strncpy(), strcmp() for SRLs
#include "timeout_data.h"
#include "ex_ex.h"  // for ex_assert()
#include "ComTdbRoot.h"

//**********************************************************************
// methods for TimeoutHashTable
//**********************************************************************

// Set the table name (in place for a short one, allocate space for long)
void TimeoutTableEntry::setTableName (char * tableName, CollHeap * heap)
{
  ex_assert( heap , "Bad heap given to setTableName" );
  heap_ = heap;  // keep the heap (for de-allocation)
  if ( str_len(tableName) >= MAX_INTERNAL_TN ) {
    tableNamePtr_ = (char *) heap_->allocateMemory( str_len(tableName) + 1 );
    strcpy( tableNamePtr_ , tableName );
  }
  else {
    strcpy( tableName_, tableName );
    tableNamePtr_ = NULL ;  // mark it as -- in place
  }
}

TimeoutHashTable::TimeoutHashTable(CollHeap * heap,
				   ULng32 hashTableSize)
  : entries_(0),
    hashTableSize_(hashTableSize),
    hashArray_(NULL),
    hashTableOriginalSize_(hashTableSize),
    heap_(heap)
{
  ex_assert( heap , "Bad heap given to ctor" );
}

TimeoutTableEntry * TimeoutHashTable::allocateHashTable(ULng32 size)
{
  // allocate the array
  return (TimeoutTableEntry *)
    heap_->allocateMemory(size * sizeof(TimeoutTableEntry));
}

// reset back to the empty state
void TimeoutHashTable::clearAll()
{
  if ( hashArray_ == NULL ) return; else entries_ = 0 ;

  // traverse array, deallocating long names if needed
  for ( ULng32 u = 0; u < hashTableSize_ ; u++ ) hashArray_[u].reset();
  heap_->deallocateMemory(hashArray_);

  hashArray_     = NULL;
  hashTableSize_ = hashTableOriginalSize_;
}

// Double hash table size if became %50 full, or resize down if the
// number of entries was significantly reduced
void TimeoutHashTable::resizeHashTableIfNeeded()
{
  ULng32 newSize ;
  if ( entries_ * 2 >= hashTableSize_ ) {   // at least 50 percent full
    newSize = 2 * hashTableSize_ ;
  } else if ( 0 == entries_ ) {             // last entry removed
    clearAll();
    return;
  } else if ( hashTableSize_ > hashTableOriginalSize_  &&
	      entries_ * 5 < hashTableSize_ ) {
    // rare case of significat reduction in the number of entries
    // after the table was enlarged from its original size
    newSize = hashTableOriginalSize_ ; // start at original size
    while ( entries_ * 3 > newSize ) newSize *= 2 ; // aim at %33 max full
  }
  else return; // no need to resize the hash table !!!

  // allocate a new table and insert into it all the values from the old one
  TimeoutTableEntry * newHashArray = NULL ;
  for ( ULng32 ul = 0; ul < hashTableSize_ ; ul++ )
    if ( hashArray_[ul].used )
      // this call would also allocate the new array
      internalInsert( & newHashArray, newSize,  hashArray_[ul].tableName(), 
		      hashArray_[ul].timeoutValue );
  
  // replace old hash array with the new one
  ULng32 keepEntries = entries_ ;
  clearAll();  // removes the old hash array, alse resets entries_
  entries_ = keepEntries;
  hashArray_     = newHashArray;
  hashTableSize_ = newSize;
}

// internal routine: insert entry into H.T., return TRUE iff entry is new
// The table parameter is a pointer to a pointer to allow allocation
NABoolean TimeoutHashTable::internalInsert( TimeoutTableEntry **hashTable, 
					    ULng32 hashTableSize,
					    char * tableName,
					    Lng32 timeoutValue )
{
  NABoolean itIsNew ;
  // get the hashValue
  Lng32 hashValue = getTimeoutHashValue(tableName);
  Lng32 originalHashValue = hashValue;
  
  // We may have to initialize the hash table
  if ( *hashTable == NULL ) {
    // allocate the array
    *hashTable = allocateHashTable(hashTableSize);
    // initialize each entry to "unused"
    for (ULng32 ul=0; ul<hashTableSize; ul++) (*hashTable)[ul].init(); 
  }

  // find an available location for this tablename (if new), or
  // its current location (if already there)
  while ( (*hashTable)[hashValue].used &&
	  strcmp( (*hashTable)[hashValue].tableName(), tableName ) )
    hashValue = ++hashValue % hashTableSize ;

  itIsNew =  ! (*hashTable)[hashValue].used ; // tableName is new in this H.T

  // insert the entry
  (*hashTable)[hashValue].used = TRUE;
  (*hashTable)[hashValue].hashValue = originalHashValue;
  (*hashTable)[hashValue].setTableName( tableName , heap_ ) ;
  (*hashTable)[hashValue].timeoutValue = timeoutValue;
 
  return itIsNew;
}

void TimeoutHashTable::insert(char * tableName, Lng32 timeoutValue)
{
  if ( internalInsert( &hashArray_, hashTableSize_, tableName, timeoutValue ) )
    entries_ ++ ;  // if inserted a new table name, increment #entries
  
  resizeHashTableIfNeeded(); // as the name says ...
};

void TimeoutHashTable::remove(char * tableName)
{
  Lng32 dummy;
  if ( ! getTimeout( tableName, dummy ) ) return;  // entry not found
  
  // tempIndex_ was set internally by getTimeout; use it to remove this entry
  hashArray_[tempIndex_].reset();   // mark as unused
  entries_-- ;

  // "fill the hole": In case subsequent array locations are used, and the
  // entries there got there by being "pushed ahead" due to collisions, then 
  // we can not leave the removed location empty, because getTimeout() would
  // not be able to find them. Thus, we iteratively check ahead and move back
  // those "pushed" entries to fill the holes. 
  //  This is a little tricky, because several "pushed ahead" different chains
  // may overlap and mix, and the the whole sequence may wrap the end of the 
  // array back to the beginning of the array.
  //  Our solution: We go back in the used-sequence and find a base index,
  // thus any index smaller than that base means: this index was wrapped !!
  //  We fix the wrapping (normalize indices) and compare the entry's hash
  // value to the hole index to eliminate the case of a chain that "started
  // after the hole" and thus should not be moved back (skip that entry).


  ULng32 holeIndex = tempIndex_ ; // index to where the "hole" is
  // base index shows where the whole used-sequence begins
  ULng32 baseIndex = holeIndex;
  while ( 1 ) {
    ULng32 prev = ( baseIndex + hashTableSize_ - 1 ) % hashTableSize_ ;
    if ( ! hashArray_[prev].used ) break; // previous not used ==> base found
    baseIndex = prev;
  }
  // iterate ahead and move entries back; finish when next is not used
  for ( ULng32 nextIndex = ( tempIndex_ + 1 ) % hashTableSize_;
	hashArray_[nextIndex].used ;
	nextIndex = ( nextIndex + 1 ) % hashTableSize_ ) { 
    // Normalize the hole and hashValue indices (to simplify comparison)
    ULng32 normHashValue = hashArray_[nextIndex].hashValue;
    if ( normHashValue < baseIndex ) normHashValue += hashTableSize_ ; 
    ULng32 normHoleIndex = holeIndex;
    if ( normHoleIndex < baseIndex ) normHoleIndex += hashTableSize_ ; 

    // If this entry was hashed to "no after" than the hole: OK to move
    if ( normHashValue <= normHoleIndex ) {
      hashArray_[holeIndex] = hashArray_[nextIndex] ; // copy entry
      // Now mark next as the "hole" and continue iterating
      hashArray_[nextIndex].used = FALSE;
      holeIndex = nextIndex;
    }  // else it was "hashed after" the hole -- just skip it
  } // end of for(nextIndex)
 
  resizeHashTableIfNeeded(); // as the name says ...
};

// find and set the timeout value for this table; return FALSE iff not found
NABoolean TimeoutHashTable::getTimeout(char * tableName, 
				       Lng32 & timeoutValue)
{
  if (!entries_) return FALSE;

  Lng32 hashValue = getTimeoutHashValue(tableName);

  while ( hashArray_[hashValue].used )
    if ( strcmp( hashArray_[hashValue].tableName(), tableName ) )
      hashValue = ++hashValue % hashTableSize_ ; // no match, check next
    else { // match !
      timeoutValue = hashArray_[hashValue].timeoutValue; // copy the value
      tempIndex_ = hashValue;  // in case this was called by remove()
      return TRUE;  //  found the entry !
    }

  return FALSE; // entry not found
}

// The hash-function
Lng32 TimeoutHashTable::getTimeoutHashValue(char * tableName)
{
  Lng32 tempVal = 0;
  Lng32 primes[4] = {1,3,5,7} ; // a mask to scatter similar names
  for ( Lng32 ul = 0; tableName[ul] != '\0'; ul++ )
    tempVal += primes[ ul % 4 ] * (char) tableName[ul] ;
  
  return ( tempVal % hashTableSize_ );
}

// Methods for packing/unpacking (used for passing information to ESPs)
IpcMessageObjSize TimeoutHashTable::packedLength()
{
  if ( 0 == entries_ ) return 0;
  IpcMessageObjSize result = sizeof ( ULng32 ); // #entries slot
  ULng32 count = entries_ ;  // used to speed-up hash array scanning
  
  for ( ULng32 ul = 0; count > 0 && ul < hashTableSize_ ; ul++ ) {
    if ( ! hashArray_[ul].used ) continue;  // skip unused array entry
    ULng32 nameLen = str_len( hashArray_[ul].tableName() );

    // add an alignment (note: When already aligned, add four null bytes).
    // This requirement is from the packIntoBuffer method.
    nameLen += 4 - nameLen % 4 ;  // add alignment size

    result += nameLen + sizeof(Lng32); // aligned name length + timeout_value

    count-- ;  // one less entry to go
  }
  
  return result;
}

// pack or unpack, and move the pointer ("buffer") accordingly
void TimeoutHashTable::packIntoBuffer(IpcMessageBufferPtr &buffer)
{
  if ( 0 == entries_ ) return;
  ULng32 count = 
    *(ULng32 *) buffer = entries_ ; // #entries is first in buffer
  buffer = (char *) buffer + 
    sizeof(ULng32);  // move pointer to next location

  for ( ULng32 ul = 0; count > 0 && ul < hashTableSize_ ; ul++ ) {
    if ( ! hashArray_[ul].used ) continue;  // skip unused array entry
    // copy the name of the table
    char * tblName = hashArray_[ul].tableName();
    ULng32 nameLen = str_len(tblName);
    strncpy( buffer, tblName, nameLen );
    buffer = (char *) buffer + nameLen ; // advance pointer
    // add an alignment (note: When already aligned, add four null bytes)
    ULng32 alignedNameLen = 0 ;  // align on a 4 byte boundaries
    alignedNameLen += 4 - nameLen % 4 ;  // Align to one of: 1,2,3,4 chars
    strncpy( buffer , "\0\0\0\0", alignedNameLen );
    buffer = (char *) buffer + alignedNameLen ; // advance pointer
    // now get the timeout value
    *(Lng32 *) buffer = hashArray_[ul].timeoutValue ;
    buffer = (char *) buffer + sizeof(Lng32);

    count-- ;  // one less entry to go
  }
}
// The following unpack method should ONLY be called when the previously
// packed (into the buffer) hash table had one or more entries (because
// an empty hash table does not pack anything into the buffer.)
void TimeoutHashTable::unpackObj(IpcConstMessageBufferPtr &buffer)
{
  ULng32 entries = * (ULng32 *) buffer;
  buffer += sizeof(ULng32);  // got # entries
  
  for ( ; entries > 0 ; entries-- ) {
    char * tblName = (char *) buffer ;
    ULng32 nameLen = str_len(tblName);
    // add an alignment (note: When already aligned, add four null bytes).
    // This requirement is from the packIntoBuffer method.
    nameLen += 4 - nameLen % 4 ; // add alignment size
    buffer += nameLen ;
    Lng32 timeoutVal = *(Lng32 *) buffer;
    buffer += sizeof(Lng32);
    // insert into the hash array
    this->insert( tblName, timeoutVal);
  }
}

//**********************************************************************

////////////////////////////////////////////////////////////////////////
//
//  Methods for  TimeoutData
//
////////////////////////////////////////////////////////////////////////

TimeoutData::TimeoutData(CollHeap *heap, ULng32 estimatedMaxEntries)
  : noLockTimeoutsSet_(TRUE),
    forAll_(FALSE),
    streamTimeoutSet_(FALSE),
    timeoutsHT_( heap, estimatedMaxEntries * 2 )
{
  ex_assert( heap && estimatedMaxEntries > 0 , "Bad parameters" );
};

void TimeoutData::setAllLockTimeout( Lng32 lockTimeoutValue )
{
  noLockTimeoutsSet_ = FALSE;
  forAll_ = TRUE;
  forAllTimeout_ = lockTimeoutValue ;
  // clean up the timeout table
  timeoutsHT_.clearAll();
}

void TimeoutData::resetAllLockTimeout()
{
  noLockTimeoutsSet_ = TRUE;
  forAll_ = FALSE;
  forAllTimeout_ = 0 ; // reset this, to allow up-to date checks
  // clean up the timeout table
  timeoutsHT_.clearAll();
}

void TimeoutData::setTableLockTimeout(char *tableName,Lng32 lockTimeoutValue )
{
  noLockTimeoutsSet_ = FALSE;
  timeoutsHT_.insert( tableName, lockTimeoutValue ) ;
}

void TimeoutData::resetTableLockTimeout( char * tableName )
{
  timeoutsHT_.remove( tableName );
  if ( timeoutsHT_.entries() == 0 && ! forAll_ ) noLockTimeoutsSet_ = TRUE;
}

NABoolean TimeoutData::getLockTimeout(char * tableName ,Lng32 & timeoutValue )
{
  if ( noLockTimeoutsSet_ ) return FALSE;

  // check timeoutsHT_ table, which overrides "set for all" (if set)
  if ( timeoutsHT_.getTimeout( tableName, timeoutValue ) ) return TRUE;

  // last, check the "set for all" case
  if ( ! forAll_ ) return FALSE;  // no "for all" (and no "per table") setting
  timeoutValue = forAllTimeout_ ;  // else -- return "for all" value
  return TRUE;
}

void TimeoutData::setStreamTimeout( Lng32 streamTimeoutValue )
{
  streamTimeoutSet_ = TRUE;
  streamTimeoutValue_ = streamTimeoutValue ;
}

void TimeoutData::resetStreamTimeout()
{
  streamTimeoutSet_ = FALSE;
}

// check if this TimeoutData (likely the global one) has any data that
// is relevant to a given execution plan
NABoolean TimeoutData::anyRelevantTimeoutData( ComTdbRoot * rootTdb )
{
  // any relevant stream timeout data ?
  if ( rootTdb->isStreamScan() && streamTimeoutSet_ ) return TRUE;

  LateNameInfoList * lnil = rootTdb->getLateNameInfoList() ;
  if ( NULL == lnil ) return FALSE ;   // ( can this ever happen ? )

  // we assume that "for-all lock timeout" always applies
  if ( ! noLockTimeoutsSet_ && forAll_ ) return TRUE;
  
  // now check each individual table used by this statement
  for (UInt32 ui = 0; ui < lnil->getNumEntries(); ui++) {
    char * tableName = lnil->getLateNameInfo(ui).resolvedPhyName();
    Lng32 dummyTimeoutValue;
    if ( timeoutsHT_.getTimeout( tableName, dummyTimeoutValue ) ) return TRUE ;
  }
  return FALSE;  // nothing relevant was found
}

// Copy relevant timeout data from this (global) to anotherTD
// (also allocates memory for anotherTD, when there is relevant data to copy)
void TimeoutData::copyData( TimeoutData ** anotherTD, 
			    CollHeap * heap,
			    ComTdbRoot * rootTdb )
{
  // check if need to allocate; i.e., there is some relevant timeout data  
  if ( ! anyRelevantTimeoutData( rootTdb ) ) return; 
  
  *anotherTD = new ( heap ) TimeoutData( heap ); // allocate
  
  // copy all scalar fields
  (*anotherTD)->noLockTimeoutsSet_    = noLockTimeoutsSet_ ;
  (*anotherTD)->forAll_               = forAll_ ;
  (*anotherTD)->forAllTimeout_        = forAllTimeout_ ;
  (*anotherTD)->streamTimeoutSet_     = streamTimeoutSet_ ;
  (*anotherTD)->streamTimeoutValue_   = streamTimeoutValue_ ;

  if ( 0 == timeoutsHT_.entries() ) return ; // no individual settings
  
  LateNameInfoList * lnil = rootTdb->getLateNameInfoList() ;
  if ( NULL == lnil ) return ;   // ( can this ever happen ? )

  // check for each table (used by this stmt) in the global hash array
  // if found -- make such an entry in the other TD's array
  for (UInt32 ui = 0; ui < lnil->getNumEntries(); ui++) {
    char * tableName = lnil->getLateNameInfo(ui).resolvedPhyName();
    Lng32 timeoutValue;
    if ( timeoutsHT_.getTimeout( tableName, timeoutValue ) )
      (*anotherTD)->timeoutsHT_.insert( tableName, timeoutValue ) ;
  }
}

// Make a detailed check whether another object is up to date with this/global
// (This method is called after a change-counter missmatch was detected)
NABoolean TimeoutData::isUpToDate( TimeoutData * anotherTD , 
				   ComTdbRoot * rootTdb )
{
  // if no local TD; up-to-date depends on relevancy of global timeout data
  if ( NULL == anotherTD ) return ! anyRelevantTimeoutData( rootTdb ) ;

  // so anotherTD exists; now do an item by item check ...

  if ( rootTdb->isStreamScan() && // streams are used: check change there
       ( anotherTD->streamTimeoutSet_   != streamTimeoutSet_   ||
	 ! streamTimeoutSet_  ||  // stream timeouts are not set in both !!
	 anotherTD->streamTimeoutValue_ != streamTimeoutValue_ ) )
    return FALSE;  // stream timeout was changed

  // The following check is a little too restrictive (e.g., setting for-all
  // to the same value as per-table, would unnecessarily fail this check )
  if ( anotherTD->forAll_        != forAll_  ||
       ! forAll_  ||  // for-all lock timeouts are not set in both !!
       anotherTD->forAllTimeout_ != forAllTimeout_ ) return FALSE;
       
  // Run individual check per each table used in this statement
  LateNameInfoList * lnil = rootTdb->getLateNameInfoList() ;
  if ( NULL == lnil ) return TRUE ;   // ( can this ever happen ? )
  
  // check timeout similarity for each table (used by this stmt)
  for (UInt32 ui = 0; ui < lnil->getNumEntries(); ui++) {
    char * tableName = lnil->getLateNameInfo(ui).resolvedPhyName();
    Lng32 globalTimeoutValue = 0 , stmtTimeoutValue = 0 ; 
    NABoolean globalFound = 
      timeoutsHT_.getTimeout( tableName, globalTimeoutValue );
    NABoolean stmtFound = 
      anotherTD->timeoutsHT_.getTimeout( tableName, stmtTimeoutValue );

    // again a little too restrictive (e.g., for-all may have the same value)
    if ( globalFound != stmtFound ) return FALSE;
    // compare actual values (both are zero if both not found)
    if ( globalTimeoutValue != stmtTimeoutValue ) return FALSE;
  }
  return TRUE; // if we've gotten so far; otherTD must be up to date
}

//
//  Method for doing the (un)packing -- used to transfer this obj to ESPs
//
IpcMessageObjSize TimeoutData::packedLength()
{
  IpcMessageObjSize result = sizeof(ULng32) ; // "flags" are always there

  if ( streamTimeoutSet_ ) result += sizeof(Lng32) ;
  if ( ! noLockTimeoutsSet_ & forAll_ ) result += sizeof(Lng32) ;
  result += timeoutsHT_.packedLength() ;
  return result;
}

void TimeoutData::packIntoBuffer(IpcMessageBufferPtr &buffer)
{
  // pack the flags (must be in sync with unpackObj() )
  ULng32 flags = streamTimeoutSet_ ? 0x01 : 0;
  if ( ! noLockTimeoutsSet_ ) {
    flags |= 0x02 ;
    if ( forAll_ ) flags |= 0x04 ;
    if ( timeoutsHT_.entries() > 0 ) flags |= 0x08 ; // is timeout HT empty ?
  }
  *(ULng32 *) buffer = flags ;           // store the flags
  buffer = (char *) buffer + sizeof(Lng32);    // incr buffer
  if ( streamTimeoutSet_ ) {
    *(Lng32 *) buffer = streamTimeoutValue_ ;   // store the stream t/o value
    buffer = (char *) buffer + sizeof(Lng32);    // incr buffer
  }  
  if ( ! noLockTimeoutsSet_ & forAll_ ) {
    *(Lng32 *) buffer = forAllTimeout_ ;           // store the for-all value
    buffer = (char *) buffer + sizeof(Lng32);    // incr buffer
  }

  timeoutsHT_.packIntoBuffer(buffer);
}

void TimeoutData::unpackObj(IpcConstMessageBufferPtr &buffer)
{
  ULng32 flags = *(ULng32 *) buffer;
  buffer += sizeof(ULng32); 

  streamTimeoutSet_ =  flags & 0x01 ;
  if ( streamTimeoutSet_ ) {
    streamTimeoutValue_ = *(Lng32 *) buffer;
    buffer += sizeof(Lng32); 
  }

  noLockTimeoutsSet_ = ! ( flags & 0x02 );
  forAll_ = flags & 0x04 ;
  if ( ! noLockTimeoutsSet_ && forAll_ ) { 
    forAllTimeout_ = *(Lng32 *) buffer;
    buffer += sizeof(Lng32); 
  }

  // only if hash table existed then unpack it!
  if ( flags & 0x08 ) // individual lock-timeout settings exist
    timeoutsHT_.unpackObj(buffer);
}
