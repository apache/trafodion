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
****************************************************************************
*/

#include "LmComQueue.h"
#include "ComPackDefs.h"

// Exclude the following functions for coverage as they are not used in LM.
template<>
Long Q_EntryPtr::pack(void *space, short isSpacePtr)
{
  // Q_EntryPtr doesn't pack or unpack the Q_Entry referenced by the pointer.
  // It is handled by Queue::pack() and unpack() to avoid double (un)packing.
  // Since Queue and Q_Entry are not derived from NAVersionedObject, they
  // don't have a mechanism to prevent packing an already packed object again
  // when there are more than one reference to it.
  //
  return packShallow(space,isSpacePtr);
}

template<>
Lng32 Q_EntryPtr::unpack(void *base)
{
  // See note about in Q_EntryPtr::unpack().
  //
  return unpackShallow(base);
}

template<>
Long QueuePtr::pack(void *space, short isSpacePtr)
{
  if(getPointer()) getPointer()->pack(space);
  return packShallow(space,isSpacePtr);
}

template<>
Lng32 QueuePtr::unpack(void *base)
{
  unpackShallow(base);
  if(getPointer())
    return (getPointer()->unpack(base));
  else
    return 0;
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

Lng32 Q_Entry::unpack(void *base)
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
  tail = new Q_Entry(0, 0, 0);
  numEntries_ = 0;
  packedLength_ = sizeof(*this) + tail->packedLength();
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

  curr =
  head =
  tail = (Q_EntryPtr)NULL;
}

void Queue::insert(void * entry_, ULng32 packedLength)
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

void * Queue::getCurr()
{
  void * temp = curr->entry;
  
  return temp;
}

void Queue::advance()
{
  void * temp = curr->entry;
  
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

void Queue::remove(void * entry)
{
  // BertBert VV
  // original code follows.  I believe it is never used (until now) and it never worked.
  /*
  curr = head;
  
  while ((curr->entry) && (curr->entry != entry))
    curr = curr->next;
  
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
    return;

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
}

Long Queue::pack(void * space)
{
  // Note that no packing is done on the actual objects (those referenced by
  // the (void *) in a Q_Entry) stored in the queue. This should be handled
  // externally.
  //

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
  return ((Space *)space)->convertToOffset((char *)this);
}

  
Lng32 Queue::unpack(void * base)
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
  return 0;
}

NABoolean Queue::packIntoBuffer(char * buffer, ULng32 &currPos)
{
  Queue * q = (Queue *)&buffer[currPos];

  str_cpy_all((char *)q, (char *)this, sizeof(*this));
  currPos += sizeof(*this);

  //  str_cpy_all((char *)&buffer[currPos], (char *)tail,
  //          tail->packedLength());
  //  q->tail = (Q_Entry *)(-currPos);
  //  currPos += tail->packedLength();

  q->head = (Q_Entry *)(buffer + currPos);
  q->head.packShallow(buffer,0);

  return TRUE;
}

void Queue::packCurrEntryIntoBuffer(char * buffer,
                                    ULng32 &currPos)
{
  Q_Entry * currBufEntry = (Q_Entry *)&buffer[currPos];

  str_cpy_all((char *)currBufEntry, (char *)(curr.pointer()), sizeof(Q_Entry));

  currBufEntry->next = (Q_Entry *)
                       (buffer + currPos + currBufEntry->packedLength());
  currBufEntry->next.packShallow(buffer,0);

  if (curr->prev)
  {
    currBufEntry->prev = (Q_Entry *)
                         (buffer + currPos - curr->prev->packedLength());
    currBufEntry->prev.packShallow(buffer,0);
  }

  currPos += sizeof(Q_Entry);

  currBufEntry->entry = (Q_Entry *)(buffer + currPos);
  currBufEntry->entry.pack(buffer,0);
}

void Queue::packTailIntoBuffer(char * buffer,
                               ULng32 &currPos,
                               Queue * packedQueue)
{
  Q_Entry * currBufEntry = (Q_Entry *)&buffer[currPos];

  str_cpy_all((char *)currBufEntry, (char *)(tail.pointer()), sizeof(Q_Entry));

  if (tail->prev)
  {
    currBufEntry->prev = (Q_Entry *)
                         (buffer + currPos - tail->prev->packedLength());
    currBufEntry->prev.packShallow(buffer,0);
  }

  packedQueue->tail = (Q_Entry *)(buffer + currPos);
  packedQueue->tail.packShallow(buffer,0);

  currPos += tail->packedLength();
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
      delete p;
    };
  };

  // delete the hash table itself
  if (heap_)
    heap_->deallocateMemory((void *) hashTable_);
  else
    delete [] hashTable_;
};

void HashQueue::insert(char * data,
		       ULng32 dataLength,
		       void * entry) {
  // set the hashValue_, currentChain_, and current_
  getHashValue(data, dataLength);

  // current_ points to the current head of the chain now!

  // insert the new entry as the new head
  hashTable_[currentChain_] = new(heap_) HashQueueEntry(entry,
							NULL,
							current_,
							hashValue_);
  // we got a new entry
  entries_++;

  // adjust the prev pointer of the former head of the chain
  if (current_)
    current_->prev_ = hashTable_[currentChain_];

  // reset all positioning info
  currentChain_ = hashTableSize_;
  current_ = 0;
  hashValue_ = 0;
};

void HashQueue::position(char * data, ULng32 dataLength) {
  // if we do not have entries in this hash queue, do not even
  // bother to calculate the hash value.
  if (!entries_) {
    current_ = NULL;
    return;
  };

  // if we are in a global scan, reset it.
  globalScan_ = FALSE;

  // set the hashValue_, currentChain_, and current_
  getHashValue(data, dataLength);

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

// Exclude the following functions for coverage as they are not used in LM.
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
  if (lastReturned_ && (lastReturned_->entry_ != entry)) {
    // go thru all the chains and exit when we find the entry
    for (currentChain_ = 0, current_ = NULL;
	 (currentChain_ < hashTableSize_) && !current_;
	 currentChain_++) {
      current_ = hashTable_[currentChain_];
      while (current_ && (current_->entry_ != entry))
	current_ = current_->next_;
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
    delete current_;
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
void HashQueue::getHashValue(char * data,
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



