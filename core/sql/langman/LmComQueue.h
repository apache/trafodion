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
****************************************************************************
*
* File:         ComQueue.h
* Description:  
*
* Created:      5/6/98
* Language:     C++
*
****************************************************************************
*/

#ifndef COMQUEUE_H
#define COMQUEUE_H

#include "NABasicObject.h"
#include "NAVersionedObject.h"


// ---------------------------------------------------------------------
// The PackQueue macros provide a way to really pack the "whole" queue.
// This includes the backbone of the queue as well as objects referenced
// by the (void *) in the Q_Entry's. The real class identity of those
// objects is passed as a parameter to the macros. Depending on whether
// those objects belong to a derived class of NAVersionedObject or not,
// either drivePack() or pack() will be called. Note that queuePtr has
// to be of type "QueuePtr".
// ---------------------------------------------------------------------

#define PackQueueOfNAVersionedObjects(queuePtr, space, Type) \
  if (queuePtr) \
  { \
    Type * entry; \
    queuePtr->position(); \
    while ((entry = (Type *)(queuePtr->getNext())) != NULL) \
      entry->drivePack(space); \
    queuePtr.pack(space); \
  }

#define PackQueueOfNonNAVersionedObjects(queuePtr, space, Type) \
  if (queuePtr) \
  { \
    Type * entry; \
    queuePtr->position(); \
    while ((entry = (Type *)(queuePtr->getNext())) != NULL) \
      entry->pack(space); \
    queuePtr.pack(space); \
  }

#define UnpackQueueOfNAVersionedObjects(queuePtr, base, Type) \
  if (queuePtr) \
  { \
    if(queuePtr.unpack(base)) return -1; \
    Type * entry; \
    queuePtr->position(); \
    while ((entry = (Type *)(queuePtr->getNext())) != NULL) \
    { \
      Type obj; \
      entry->driveUnpack(base,&obj); \
    } \
  }

#define UnpackQueueOfNonNAVersionedObjects(queuePtr, base, Type) \
  if (queuePtr) \
  { \
    if(queuePtr.unpack(base)) return -1; \
    Type * entry; \
    queuePtr->position(); \
    while ((entry = (Type *)(queuePtr->getNext())) != NULL) \
    { \
      entry->fixupVTblPtr(); \
      entry->unpack(base); \
    } \
  }

namespace LangManPack {
// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for Q_Entry
// ---------------------------------------------------------------------
class Q_Entry;
typedef NAOpenObjectPtrTempl<Q_Entry> Q_EntryPtr;

class Q_Entry
{
  friend class Queue;

  NABasicPtr  entry;                                             // 00-07
  Q_EntryPtr  prev;                                              // 08-15
  Q_EntryPtr  next;                                              // 16-31
  UInt32      packedLength_;                                     // 32-35
  char        fillersQ_Entry_[4];                                // 36-39

public:
  Q_Entry(void * entry_, Q_Entry * prev_, Q_Entry * next_);

  ~Q_Entry();

  Long pack(void *space);
  Lng32 unpack(void *base);

  ULng32 packedLength() { return packedLength_; }
};


// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for Queue
// ---------------------------------------------------------------------
class Queue;
typedef NAOpenObjectPtrTempl<Queue> QueuePtr;

class Queue
{
  Q_EntryPtr   head;                                             // 00-07
  Q_EntryPtr   tail;                                             // 08-15
  Q_EntryPtr   curr;                                             // 16-31

  CollHeap    *heap_;

  Int32        numEntries_;                                      // 40-43
  
  // length of queue in 'packed' (contiguous) form.
  // This field is updated whenever a new queue entry is added (removed).
  UInt32       packedLength_;                                    // 44-47
  char         fillersQueue_[8];                                 // 48-55

public:
  Queue();

  void operator delete (void *p)
  {
	CollHeap *h = ((Queue *)p)->heap_;
    if (h)
      h->deallocateMemory(p);
    else
      ::operator delete(p);
  }

  Queue(CollHeap * heap);

  ~Queue();
  
  // inserts at tail
  void insert(void * entry_, ULng32 entryPackedLength = 0);

  // returns the head entry
  void * get();

  // returns the i'th entry
  void * get(ULng32 i);

  // returns the head entry
  void * getHead();

  // returns the tail(last) entry
  void * getTail();

  // positions on the head entry
  void position();
  
  // returns the current entry
  void * getCurr();

  // advances the current entry
  void advance();
  
  // returns the current entry and advances curr
  void * getNext();
  
  // returns -1, if all entries have been returned. 
  short atEnd();
  
  // removes the head entry
  void remove();

  // removes the 'entry' entry
  void remove(void * entry);

  void packCurrEntryIntoBuffer(char * buffer, ULng32 &currPos);

  Long pack(void * space);
  
  Lng32 unpack(void * base);
  
  // returns -1, if queue is empty. Otherwise, returns 0.
  Int32 isEmpty()
    {
      return ((numEntries() == 0) ? -1 : 0);
    }
   
  Lng32 numEntries() { return numEntries_;}

  ULng32 packedLength() { return packedLength_; }

  // packs 'this' into a contiguous buffer. Converts pointers
  // to offsets. The passed buflen must not be less than
  // packedLength_. Returns TRUE, if no error.
  NABoolean packIntoBuffer(char * buffer, ULng32 &buflen);

  void packTailIntoBuffer(char * buffer, 
			  ULng32 &currPos,
			  Queue * packedQueue);
};

// A HashQueue is basically a container which is accessed by
// hashing. It is introduced here to speed up the statement
// and descriptor lookup in the sqlcli. The statement- and
// descriptor lists were originally implemented as Queues. Lookup
// was always just a sequential search in this list. Note
// that the following is a quick and dirty implementation
// to eliviate this performance problem. If we would only
// have more time (sigh), a better design would be a template
// based implementation of containers (lists, arrays, queues,
// stacks, you name it) for the executor similar to the collections
// in the compiler.
  
class HashQueueEntry : public NABasicObject {
  friend class HashQueue;

public:
  HashQueueEntry(void * entry,
                 HashQueueEntry * prev,
                 HashQueueEntry * next,
                 ULng32 hashValue)
    : entry_(entry),
      prev_(prev),
      next_(next),
      hashValue_(hashValue)
  {}

  ~HashQueueEntry() {}

private:
  HashQueueEntry * prev_;
  HashQueueEntry * next_;
  void * entry_;
  ULng32 hashValue_;
};


class HashQueue : public NABasicObject {
public:
  HashQueue(CollHeap * heap, ULng32 hashTableSize = 513);

  HashQueue(const NABoolean shadowCopy,
	    const HashQueue &other); 

  ~HashQueue();

  ULng32 entries() { return entries_; }

  ULng32 size() { return hashTableSize_; }

  void insert(char * data, ULng32 dataLength, void * entry);

// position by hashing
  void position(char * data, ULng32 dataLength);

// posuition globaly
  void position();

  void * getNext();

  // returns the current entry
  void * getCurr();

  // advances the current entry
  void advance();

  void remove(void * entry);

  Lng32 numEntries() { return (Lng32)entries_;}

private:
  ULng32 entries_;         // number of entries in this HashQueue
  ULng32 hashTableSize_;   // size of the hash table
  HashQueueEntry ** hashTable_;   // the hash table itself
  HashQueueEntry * lastReturned_; // the last entry returned by getNext()
  HashQueueEntry * current_;      // points to the current entry
  ULng32 currentChain_;    // the chain were current_ is located
  ULng32 hashValue_;       // hash value of the last position
  NABoolean globalScan_;          // if true, getNext ignores hashValue
  CollHeap * heap_;               // the heap a HashQueue allocates from
  NABoolean shadowCopy_;          // TRUE if constructed by shadow copy

  void getHashValue(char * data, ULng32 dataLength);
};
}
using namespace LangManPack;

#endif

