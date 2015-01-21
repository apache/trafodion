/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1998-2014 Hewlett-Packard Development Company, L.P.
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
/* -*-C++-*-
****************************************************************************
*
* File:         ComQueue.h
* Description:  
*
* Created:      5/6/98
* Language:     C++
*
*
*
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

#define UnpackQueueOfNAVersionedObjects(queuePtr, base, Type, Reallocator) \
  if (queuePtr) \
  { \
    if(queuePtr.unpack(base,reallocator)) return -1; \
    Type * entry; \
    queuePtr->position(); \
    while ((entry = (Type *)(queuePtr->getNext())) != NULL) \
    { \
      Type obj; \
      entry->driveUnpack(base,&obj,reallocator); \
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
  Q_EntryPtr  next;                                              // 16-23
  UInt32      packedLength_;                                     // 24-27
  char        fillersQ_Entry_[20];                               // 28-47

public:
NA_EIDPROC
  Q_Entry(void * entry_, Q_Entry * prev_, Q_Entry * next_);

NA_EIDPROC
  ~Q_Entry();

NA_EIDPROC
  Long pack(void *space);
NA_EIDPROC
  Lng32 unpack(void *base);

NA_EIDPROC
  ULng32 packedLength() { return packedLength_; }
};

class Queue : public NAVersionedObject
{
  Q_EntryPtr   head;                                             // 00-07
  Q_EntryPtr   tail;                                             // 08-15
  Q_EntryPtr   curr;                                             // 16-23

  CollHeap    *heap_;
#ifndef NA_64BIT
  char         fillersQueue1_[4];
#endif                                                           // 24-31

  Int32        numEntries_;                                      // 32-35
  
  // length of queue in 'packed' (contiguous) form.
  // This field is updated whenever a new queue entry is added (removed).
  UInt32       packedLength_;                                    // 36-39

  UInt16       flags_;                                           // 40-41
  char         fillersQueue_[14];                                // 42-55

  enum {
    // if set, then remove doesn't deletes Q_Entry, only makes the
    // entry pointer NULL.
    // Insert is done at the first empty Q_Entry.
    // remove removes the head entry and moves it to the end
    // of the list.
    DO_SPACE_OPT = 0x0001
  };

public:
NA_EIDPROC
  Queue();


  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
NA_EIDPROC
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }
  
NA_EIDPROC
  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }
  
NA_EIDPROC
  virtual short getClassSize() { return (short)sizeof(Queue); }

NA_EIDPROC
  Queue(CollHeap * heap);

NA_EIDPROC
  ~Queue();
  
  // inserts at tail
NA_EIDPROC
  void insert(const void * entry_, ULng32 entryPackedLength = 0);

  // inserts at tail, returns addr of Queue entry inserted
NA_EIDPROC
  void insert(const void * entry_, ULng32 entryPackedLength,
	      void ** queueEntry);

  // returns the head entry
NA_EIDPROC
  void * get();

  // returns the i'th entry
NA_EIDPROC
  void * get(ULng32 i);

  // returns the head entry
NA_EIDPROC
  void * getHead();

  // returns the tail(last) entry
NA_EIDPROC
  void * getTail();

  // positions on the head entry
NA_EIDPROC
  void position();
  
  // positions on a specified Queue entry 
NA_EIDPROC
  void position(void * queueEntry);

  // returns the current entry
NA_EIDPROC
  void * getCurr();

  // advances the current entry
NA_EIDPROC
  void advance();
  
  // returns the current entry and advances curr
NA_EIDPROC
  void * getNext();
  
  // returns -1, if all entries have been returned. 
NA_EIDPROC
  short atEnd();
  
  // removes the head entry
NA_EIDPROC
  void remove();

  // removes the head entry
NA_EIDPROC
  void removeHead() { remove(); };

  // removes the tail entry
NA_EIDPROC
  void removeTail();

NA_EIDPROC
  void removeWithSpaceOpt();

  // removes the 'entry' entry
NA_EIDPROC
  NABoolean remove(void * entry);


NA_EIDPROC
  virtual Long pack(void * space);
  
NA_EIDPROC
  virtual Lng32 unpack(void * base, void * reallocator);
  
  // returns -1, if queue is empty. Otherwise, returns 0.
NA_EIDPROC
  Int32 isEmpty()
    {
      return ((numEntries() == 0) ? -1 : 0);
    }
   
NA_EIDPROC
  Lng32 numEntries() { return numEntries_;}

   Lng32 entries() { return numEntries();}

NA_EIDPROC
  ULng32 packedLength() { return packedLength_; }

  NA_EIDPROC 
  void setDoSpaceOpt(short v) {(v ? flags_ |= DO_SPACE_OPT : flags_ &= ~DO_SPACE_OPT); };
  NABoolean doSpaceOpt() { return (flags_ & DO_SPACE_OPT) != 0; };

};

// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for Queue
// ---------------------------------------------------------------------
typedef NAVersionedObjectPtrTempl<Queue> QueuePtr;

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
  
class HashQueueEntry {
  friend class HashQueue;

public:
NA_EIDPROC
  HashQueueEntry(void * entry,
			HashQueueEntry * prev,
			HashQueueEntry * next,
			ULng32 hashValue)
  : entry_(entry),
  prev_(prev),
  next_(next),
  hashValue_(hashValue) {
  };

NA_EIDPROC
  ~HashQueueEntry() {
  };

private:
  HashQueueEntry * prev_;
  HashQueueEntry * next_;
  void * entry_;
  ULng32 hashValue_;
};

 
class HashQueue : public NABasicObject {
public:
NA_EIDPROC
  HashQueue(CollHeap * heap, ULng32 hashTableSize = 513);

NA_EIDPROC
  HashQueue(const NABoolean shadowCopy,
	    const HashQueue &other); 

NA_EIDPROC
  ~HashQueue();

NA_EIDPROC
  ULng32 entries() { return entries_; }

NA_EIDPROC
  ULng32 size() { return hashTableSize_; }

NA_EIDPROC
  void insert(const char * data, ULng32 dataLength, void * entry);

// position by hashing
NA_EIDPROC
  void position(const char * data, ULng32 dataLength);

// position globally
NA_EIDPROC
  void position();

NA_EIDPROC
  void * getNext();

  // returns the current entry
NA_EIDPROC
  void * getCurr();

  // advances the current entry
NA_EIDPROC
  void advance();

NA_EIDPROC
  void remove(void * entry);

// To remove the last rrturned entry via getNext() in case of global scan
// of hash queue
NA_EIDPROC
  void remove();

// To remove the entry by passing in the hashing fields and the corresponding entry
NA_EIDPROC
  void remove(const char *data, ULng32 dataLength, void *entry);

NA_EIDPROC
  Lng32 numEntries() { return (Lng32)entries_;}

NA_EIDPROC
  NABoolean sequentialAdd()      { return (flags_ & SEQUENTIAL_ADD)    != 0; }

NA_EIDPROC
  void setSequentialAdd(NABoolean v)      
           { (v ? flags_ |= SEQUENTIAL_ADD : flags_ &= ~SEQUENTIAL_ADD); }
NA_EIDPROC
  Int32 isEmpty() { return ((entries_ == 0) ? -1 : 0); }

private:
  enum Flags
  {
    // add hash entries in a sequential order (the same order in which
    // they are inserted).
    SEQUENTIAL_ADD = 0x0001
  };

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

  ULng32 flags_;

NA_EIDPROC
  void getHashValue(const char * data, ULng32 dataLength);

NA_EIDPROC
  void removeLastReturned();
};

class SyncHashQueue : public HashQueue {
public:
  SyncHashQueue(CollHeap * heap, ULng32 hashTableSize = 513);
  void position(const char * data, ULng32 dataLength);
  void position();
  void remove(void * entry);
  void remove();
  void remove(const char *data, ULng32 dataLength, void *entry);
  void insert(const char * data, ULng32 dataLength, void * entry);
};


#endif

