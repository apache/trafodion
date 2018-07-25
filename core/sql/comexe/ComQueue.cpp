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
* File:         ComQueue.cpp
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

#include "ComQueue.h"
#include "ComPackDefs.h"

extern NABoolean checkIfRTSSemaphoreLocked();

template<> Long Q_EntryPtr::pack(void *space, short isSpacePtr)
{
  // Q_EntryPtr doesn't pack or unpack the Q_Entry referenced by the pointer.
  // It is handled by Queue::pack() and unpack() to avoid double (un)packing.
  // Since Queue and Q_Entry are not derived from NAVersionedObject, they
  // don't have a mechanism to prevent packing an already packed object again
  // when there are more than one reference to it.
  //
  return packShallow(space,isSpacePtr);
}

template<> Lng32 Q_EntryPtr::unpack(void * base)
{
  // See note about in Q_EntryPtr::unpack().
  //
  return unpackShallow(base);
}

Q_Entry::Q_Entry(void * entry_, Q_Entry * prev_, Q_Entry * next_)
{
  entry = entry_;
  prev = prev_;
  next = next_;
  packedLength_ = sizeof(*this);
}

// This destructor simply defined for the purpose of being
// declared resident in the queue.h file -- then in dp2 there
// will be no page faults.
Q_Entry::~Q_Entry()
{
}

Long Q_Entry::pack(void *space)
{
  // Note that no packing is done on the actual objects (those referenced by
  // the (void *) in a Q_Entry) stored in the queue. This should be handled
  // externally. Also, Q_EntryPtr::pack() just converts the pointer object
  // into an offset without calling pack() on the Q_Entry referenced.
  //
  entry.pack(space);
  prev.pack(space);
  next.pack(space);
  return ((Space *)space)->convertToOffset((char *)this);
}

Lng32 Q_Entry::unpack(void * base)
{
  // See note in Q_Entry::pack().
  //
  if(entry.unpack(base)) return -1;
  if(prev.unpack(base)) return -1;
  if(next.unpack(base)) return -1;
  return 0;
}

Queue::Queue(CollHeap * ch)
  : heap_(ch)
{
  curr = 
  head = 
  tail = new(ch) Q_Entry(0, 0, 0);
  numEntries_ = 0;
  packedLength_ = sizeof(*this) + tail->packedLength();
}

Queue::Queue()
  : heap_(0)
{
  curr = 
  head = 
  tail = 0;
  numEntries_ = 0;
  packedLength_ = sizeof(*this);
}

Queue::~Queue()
{
  while (! isEmpty())
    remove(); // the head entry

  if (tail)
  {
      // REVISIT
      if (heap_)
        heap_->deallocateMemory((void *) tail.getPointer());
      else
        delete tail;
  }

  curr = 0;
  head = 0;
  tail = 0;
}

void Queue::insert(const void * entry_, ULng32 packedLength)
{
  tail->entry = entry_;
  tail->packedLength_ += packedLength;
  packedLength_ += tail->packedLength();

  if (heap_)
    tail->next = new(heap_) Q_Entry(0, 0, 0);
  else
    tail->next = new Q_Entry(0, 0, 0);
  tail->next->prev = tail;
  tail = tail->next;
  numEntries_++;
}

void Queue::insert(const void * entry_, ULng32 packedLength,
		   void ** queueEntry)
{
  *queueEntry = tail;
  tail->entry = entry_;
  tail->packedLength_ += packedLength;
  packedLength_ += tail->packedLength();

  if (heap_)
    tail->next = new(heap_) Q_Entry(0, 0, 0);
  else
    tail->next = new Q_Entry(0, 0, 0);
  tail->next->prev = tail;
  tail = tail->next;
  numEntries_++;
}

void * Queue::get()
{
  return getHead();
}

void * Queue::getHead()
{
  return head->entry;
}

void * Queue::getTail()
{
  Q_Entry * temp = head;
  
  if (! temp->next)
    return temp->entry;
  
  while (temp->next->next)
    {
      temp = temp->next;
    }
  
  return temp->entry;
}

void Queue::position()
{
  curr = head;
}


void Queue::position(void * queueEntry)
{
  if (queueEntry != NULL)
    curr = queueEntry;
  else
    curr = head; 
}

void * Queue::getCurr()
{
  void * temp = curr->entry;
  
  return temp;
}

void Queue::advance()
{
  if (curr->next)
    curr = curr->next;
}

void * Queue::getNext()
{
  void * temp = curr->entry;
  
  if (curr->next)
    curr = curr->next;
  
  return temp;
}

void * Queue::get(ULng32 i)
{
  position();
  for (ULng32 j = 0; j < i; j++)
    getNext();

  return getCurr();
}

short Queue::atEnd()
{
  if (! curr->entry) // at End
    return -1;       // Yes.
  else
    return 0;
}

void Queue::remove()
{
  if (head->entry)
    {
      Q_Entry * temp = head;
      packedLength_ -= temp->packedLength();

      head = head->next;

      // REVISIT 
      if (heap_)
        heap_->deallocateMemory((void *) temp);
      else
        delete temp;

      head->prev = 0;
      numEntries_--;
    }
}

void Queue::removeTail()
{
  if (tail->prev)
    {
      Q_Entry * temp = tail->prev;
      packedLength_ -= temp->packedLength();

      if (temp->prev)
	{
	  temp->prev->next = tail;
	  tail->prev = temp->prev;
	}
      else
	{
	  tail->prev = (Q_EntryPtr)NULL;
	  head = tail;
	}

      // REVISIT 
      if (heap_)
        heap_->deallocateMemory((void *) temp);
      else
        delete temp;

      numEntries_--;
    }
}

NABoolean Queue::remove(void * entry)
{
  // BertBert VV
  // original code follows.  I believe it is never used (until now) and it never worked.
  /*
  // If 'entry' is passed in as NULL,
  // then back up one because caller got it via getNext() ...
  if (! entry)
    {
      curr = curr->prev;
    }
  else
    {
      curr = head;
  
      while (curr->entry && curr->entry != entry)
	curr = curr->next;
    }
  
  if (!curr->entry)
    return;

  packedLength_ -= curr->packedLength();
  
  if (curr == head)
    {
      head = curr->next;
      head->prev = 0;
    }
  else
    {
      curr->prev->next = curr->next;
      if (curr->next)
	curr->next->prev = curr->prev;
    }
  */
  // new code follows
  // new code leaves "curr" alone
  // new code "deletes" removed entry

  Q_Entry * temp = NULL;

  // If 'entry' is passed in as NULL,
  // then back up one because caller got it via getNext() ...
  if (! entry)
    {
      temp = curr->prev;
    }
  else
    {
      temp = head;
      
      while ((temp->entry) && (temp->entry != entry))
	temp = temp->next;
    }

  if (!temp->entry)
    return FALSE;	// Soln 10-040521-6302

  packedLength_ -= temp->packedLength();
  
  if (temp == head)
    {
      head = temp->next;
      head->prev = 0;
    }
  else
    {
      temp->prev->next = temp->next;
      if (temp->next)
	temp->next->prev = temp->prev;
    }

  // REVISIT 
  if (heap_)
    heap_->deallocateMemory((void *) temp);
  else
    delete temp;

  numEntries_--;
  // BertBert ^^

  return TRUE;
}

Long Queue::pack(void * space)
{
  // Pack all Q_Entry objects in the Queue backbone.
  //
  Q_Entry * temp = head;
  while (temp->entry)
  {
    // Store pointer to next object before packing it.
    Q_Entry * next = temp->next;

    // pack the Q_Entry object.
    temp->pack(space);
    temp = next;
  }

  // Pack my data members to offsets as well.
  head.pack(space);
  tail.pack(space);

  return NAVersionedObject::pack(space);
  // return ((Space *)space)->convertToOffset((char *)this);
}

  
Lng32 Queue::unpack(void * base, void * reallocator)
{
  // Unpack my data members.
  if(head.unpack(base)) return -1;
  if(tail.unpack(base)) return -1;

  // unpack data members in each Q_Entry.
  Q_Entry * temp = head;
  while (temp->entry)
  {
    if(temp->unpack(base)) return -1;
    temp = temp->next;
  }

  // REVISIT
  heap_ = 0;

  return NAVersionedObject::unpack(base, reallocator);
}


////////////////////////////////////////////////////////////////////////
// methods for HashQueue
////////////////////////////////////////////////////////////////////////

HashQueue::HashQueue(CollHeap * heap,
		     ULng32 hashTableSize)
  : entries_(0),
    hashTableSize_(hashTableSize),
    lastReturned_(NULL),
    current_(NULL),
    currentChain_(hashTableSize), // initialize with illegal value
    hashValue_(0),
    globalScan_(FALSE),
    heap_(heap),
    flags_(0),
    shadowCopy_(FALSE)  {
      // allocate the chains
      if (heap_)
	hashTable_ = (HashQueueEntry **)
	  heap_->allocateMemory(hashTableSize_ *
			       sizeof(HashQueueEntry*));
      else
	hashTable_ = new HashQueueEntry*[hashTableSize_];

      for (currentChain_ = 0; currentChain_ < hashTableSize_; currentChain_++)
	hashTable_[currentChain_] = NULL;
};

HashQueue::HashQueue(const NABoolean shadowCopy, 
		     const HashQueue &other)
{
  if (this == &other)
    return;
  *this = other;
  shadowCopy_ = shadowCopy;
  if (shadowCopy)
    heap_ = NULL;
}

HashQueue::~HashQueue() {
  
  if (shadowCopy_)
    return;
  // go thru all the chains and delete all entries in
  // each chain
  for (currentChain_ = 0;
       currentChain_ < hashTableSize_;
       currentChain_++) {
    HashQueueEntry * q = hashTable_[currentChain_];
    while (q) {
      HashQueueEntry * p = q;
      q = q->next_;
      NADELETE(p, HashQueueEntry, heap_);
    };
  };

  // delete the hash table itself
  if (heap_)
    heap_->deallocateMemory((void *) hashTable_);
  else
    delete [] hashTable_;
};

void HashQueue::insert(const char * data,
		       ULng32 dataLength,
		       void * entry) {
  if (! sequentialAdd())
    {
      // set the hashValue_, currentChain_, and current_
      getHashValue(data, dataLength);
    }
  else
    {
      // This will return the same hash value for all calls.
      getHashValue(NULL, 0);
    }

  // current_ points to the current head of the chain now!

  if (! sequentialAdd())
    {
      // insert the new entry as the new head
      hashTable_[currentChain_] = new(heap_) HashQueueEntry(entry,
							    NULL,
							    current_,
							    hashValue_);
      // adjust the prev pointer of the former head of the chain
      if (current_)
	current_->prev_ = hashTable_[currentChain_];
    }
  else
    {
      // find the last entry in this chain.
      while (current_ && current_->next_)
	current_ = current_->next_;

      HashQueueEntry * hqe = new(heap_) HashQueueEntry(entry,
						       current_,
						       NULL,
						       hashValue_);

      if (current_)
	current_->next_ = hqe;
      else
	hashTable_[currentChain_] = hqe;
    }

  // we got a new entry
  entries_++;

  // reset all positioning info
  currentChain_ = hashTableSize_;
  current_ = 0;
  hashValue_ = 0;
};

void HashQueue::position(const char * data, ULng32 dataLength) {
  // if we do not have entries in this hash queue, do not even
  // bother to calculate the hash value.
  if (!entries_) {
    current_ = NULL;
    return;
  };

  // if we are in a global scan, reset it.
  globalScan_ = FALSE;

  // set the hashValue_, currentChain_, and current_
  if (! sequentialAdd())
    {
      // set the hashValue_, currentChain_, and current_
      getHashValue(data, dataLength);
    }
  else
    {
      // This will return the same hash value for all calls.
      getHashValue(NULL, 0);
    }

  while (current_ && (current_->hashValue_ != hashValue_))
    current_ = current_->next_;
};

void HashQueue::position() {
  // if we do not have entries in this hash queue, do not even
  // bother to calculate the hash value.
  if (!entries_) {
    current_ = NULL;
    return;
  };

  // we want to do a global scan. Position on the first entry
  for (currentChain_ = 0; currentChain_ < hashTableSize_; currentChain_++) {
    current_ = hashTable_[currentChain_];
    if (current_)
      break;
  };

  globalScan_ = TRUE;
};

void * HashQueue::getNext() {

  lastReturned_ = current_;

  if (globalScan_) {
    if (current_) {
      // we are not done with the global scan. Look at next entry
      current_ = current_->next_;
      // if next entry is NULL. Look at next non-empty chain
      while (!current_ && (++currentChain_ < hashTableSize_))
	current_ = hashTable_[currentChain_];
    }
    else {
      // current_ is NULL. We are at the end of the global
      // scan.
      globalScan_ = FALSE;
    };
  }
  else {
    // look for the next matching entry in the current chain
    if (current_)
      current_ = current_->next_;
    while (current_ && (current_->hashValue_ != hashValue_))
      current_ = current_->next_;
  };

  if (lastReturned_)
    return(lastReturned_->entry_);
  else
    return(NULL);
};

void * HashQueue::getCurr() {

  if (current_)
    return current_->entry_;
  else
    return NULL;
}

void HashQueue::advance() {

  if (globalScan_) {
    if (current_) {
      // we are not done with the global scan. Look at next entry
      current_ = current_->next_;
      // if next entry is NULL. Look at next non-empty chain
      while (!current_ && (++currentChain_ < hashTableSize_))
	current_ = hashTable_[currentChain_];
    }
    else {
      // current_ is NULL. We are at the end of the global
      // scan.
      globalScan_ = FALSE;
    };
  }
  else {
    // look for the next matching entry in the current chain
    if (current_)
      current_ = current_->next_;
    while (current_ && (current_->hashValue_ != hashValue_))
      current_ = current_->next_;
  };
};

void HashQueue::remove(void * entry) {
  if (!entry)
    return;

  // remove is only allowed, if we are not in a global
  // scan. In a global scan currentChain_ might change
  // with getNext().
  // remove relies on the fact that currentChain_ does
  // not change with a getNext(). A regular hash lookup
  // guarantees this.
  // assert(!globalScan_);

  // first check if the entry we want to remove is the
  // last entry returned. At least in SQLCLI this is always true,
  // because before we do a remove, we do a lookup.
  // if the entry to delete is not the last one returned,
  // we have to globaly search the hash table
  if (lastReturned_ == NULL || lastReturned_->entry_ != entry) {
    // go thru all the chains and exit when we find the entry
    for (currentChain_ = 0, current_ = NULL;
	 (currentChain_ < hashTableSize_);
	 currentChain_++) {
      current_ = hashTable_[currentChain_];
      while (current_ && (current_->entry_ != entry))
	current_ = current_->next_;
      if (current_ && current_->entry_ == entry)
        break;
    };
  }
  else
    // the last one returned is the one we want to remove. Since
    // we are removing current_, set current_ accordingly
    current_ = lastReturned_;

  if (current_) {
    // now unchain the entry. First take care of the next_ pointer
    // of the previous entry
    if (!current_->prev_)
      // current_ is the first element in the chain
      hashTable_[currentChain_] = current_->next_;
    else
      // we are somewhere in the chain
      current_->prev_->next_ = current_->next_;

    // now take care of the prev_ pointer of the next entry
    if (current_->next_)
      current_->next_->prev_ = current_->prev_;

    // finally delete the current entry.
    if (heap_)
      heap_->deallocateMemory(current_);
    else
      delete current_;
    //
    // should we intrdouce deleteMe concept
    current_ = NULL;
    // adjust counter
    entries_--;
  };

  // we either deleted current_, or we couldn't find the entry.
  // in either case, we reset all positioning info
  currentChain_ = hashTableSize_;
  lastReturned_ = NULL;
  hashValue_ = 0;
};



// simple method to calculate a hash value
void HashQueue::getHashValue(const char * data,
			     ULng32 dataLength) {
  hashValue_ = 0;
  for(ULng32 i = 0; i < dataLength; i++)
    hashValue_ += (unsigned char)data[i];

  // we never return 0 as hashValue_
  if (!hashValue_)
    hashValue_ = 1;

  // set the currentChain_
  currentChain_ = hashValue_ % hashTableSize_;
  current_ = hashTable_[currentChain_];
};


void HashQueue::remove() {
    
  assert(globalScan_);
  removeLastReturned();
}
 

void HashQueue::remove(const char *data, ULng32 dataLength, void *entry)
{
  assert(entry);
  if (lastReturned_ == NULL || lastReturned_->entry_ != entry)
  {
    position(data, dataLength);
 
    while (current_ && (current_->entry_ != entry))
	current_ = current_->next_;
    lastReturned_ = current_;
  }
  if (lastReturned_)
    removeLastReturned(); 
  currentChain_ = hashTableSize_;
  current_ = NULL;
  hashValue_ = 0;
}

void HashQueue::removeLastReturned()
{
  assert(lastReturned_);
  ULng32 currentChain;
  
  // now unchain the entry. First take care of the next_ pointer
  // of the previous entry
  if (!lastReturned_->prev_)
  {
     // lastReturned_ is the first element in the chain
    currentChain = lastReturned_->hashValue_ % hashTableSize_;
    hashTable_[currentChain] = lastReturned_->next_;
  }
  else
    // we are somewhere in the chain
    lastReturned_->prev_->next_ = lastReturned_->next_;

  // now take care of the prev_ pointer of the next entry
  if (lastReturned_->next_)
    lastReturned_->next_->prev_ = lastReturned_->prev_;
  // finally delete the lastReturned_ entry.
  if (heap_)
      heap_->deallocateMemory(lastReturned_);
    else
      delete lastReturned_;
  // adjust counter
  entries_--;  
  lastReturned_ = NULL;
}

SyncHashQueue::SyncHashQueue(CollHeap * heap,
		     ULng32 hashTableSize)
   : HashQueue(heap, hashTableSize)
{
}
void SyncHashQueue::position(const char * data, ULng32 dataLength) {
   if (! checkIfRTSSemaphoreLocked())
      abort();
   HashQueue::position(data, dataLength);
}

void SyncHashQueue::position() {
   if (! checkIfRTSSemaphoreLocked())
      abort();
   HashQueue::position();
}

void SyncHashQueue::remove() {
   if (! checkIfRTSSemaphoreLocked())
      abort();
   HashQueue::remove();
}

void SyncHashQueue::remove(void *entry) {
   if (! checkIfRTSSemaphoreLocked())
      abort();
   HashQueue::remove(entry);
}

void SyncHashQueue::remove(const char *data, ULng32 dataLength, void *entry) {
   if (! checkIfRTSSemaphoreLocked())
      abort();
   HashQueue::remove(data, dataLength, entry);
}

void SyncHashQueue::insert(const char *data, ULng32 dataLength, void *entry) {
   if (! checkIfRTSSemaphoreLocked())
      abort();
   HashQueue::insert(data, dataLength, entry);
}

