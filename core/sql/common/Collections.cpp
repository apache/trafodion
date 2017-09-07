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
**************************************************************************
*
* File:         Collections.C
* Description:  Implementation for Collection type templates
* Created:      4/27/94
* Language:     C++
*
*
*
**************************************************************************
*/
 
// -----------------------------------------------------------------------
// functions for the NACollection<T> template
// -----------------------------------------------------------------------
#include "Platform.h"

  #include "Collections.h"

#pragma nowarn(1506)   // warning elimination 

template <class T> NACollection<T>::~NACollection()
{
  deallocate();
}

// For HSC, define these methods as inline methods in the Collections.h header file.

template <class T>
void NACollection<T>::copy(const NACollection<T> &other) 
{
  // allocate the arrays
  // heap_       = other.heap_; //!!!do not change the value of a collection's heap_!!!
  allocate(other.maxLength_);
  usedLength_ = other.usedLength_;
  entries_    = other.entries_;
  
  // copy the entries
  for (CollIndex i = FIRST_COLL_INDEX; i < usedLength_; i++)
    {
      arr_[i] = other.arr_[i];
      usages_[i]  = other.usages_[i];
    }
}


template <class T>
void NACollection<T>::insert(CollIndex posToInsert,
			     const T   &newElement,
			     CollIndex newUsage)
{
  // is a valid position and usage given?
  assert((posToInsert < MAX_COLL_INDEX) AND
	 (newUsage    != UNUSED_COLL_ENTRY));
  
  // do we need to increase the size?
  if (posToInsert >= maxLength_)
    resize(posToInsert + 1);
  
  // is the new position in the unused portion?
  if (posToInsert >= usedLength_)
    {
      // extend the used portion by filling in the usages_
      // entries with UNUSED_COLL_ENTRY values
      for (CollIndex i = usedLength_; i <= posToInsert; i++)
	{
	  usages_[i] = UNUSED_COLL_ENTRY;
	}
      usedLength_ = posToInsert + 1;
    }
  
  // overwrite or insert?
  if (usages_[posToInsert] == UNUSED_COLL_ENTRY)
    entries_++;
  
  // ok, now we're ready to insert the new element
  usages_[posToInsert]  = newUsage;
  arr_[posToInsert] = newElement;
  
  return;
}

template <class T>
CollIndex NACollection<T>::resize(CollIndex newSize)
{
  if (newSize == maxLength_ OR
      newSize < usedLength_)
    // no need to resize or impossible to resize
    return maxLength_;
  else
    {
      
      // increment the size in larger chunks to avoid
      // many expensive resize calls
      if (newSize > maxLength_ AND
	  newSize/3 < (maxLength_+2)/2)
	{
	  newSize = 3 * (maxLength_+2)/2;
	}
  
      // shouldn't even come close to this
      assert (newSize < MAX_COLL_INDEX);

      // use a temp collection with the new size
      NACollection<T> newOne(heap_,newSize);
    
      assert(newSize >= usedLength_);

      for (CollIndex i = FIRST_COLL_INDEX; i < usedLength_; i++)
	{
	  newOne.usages_[i] = usages_[i];
	  if (usages_[i] != UNUSED_COLL_ENTRY)
	    {
	      newOne.arr_[i] = arr_[i];
	    }
	}
      
      // now just deallocate the old arrays and take the new
      // arrays and maxLength_
      deallocate();
      usages_        = newOne.usages_;
      arr_           = newOne.arr_;
      maxLength_     = newOne.maxLength_;
      
      // make sure the destructor of newOne won't mess things up
      newOne.usages_     = NULL;
      newOne.arr_        = NULL;
      newOne.usedLength_ = 0;
      newOne.maxLength_  = 0;
      
      return maxLength_;
    }
}

#ifdef __GNUC__
#  if __GNUC__ * 100 + __GNUC_MINOR__ >= 404
     // push_options added in 4.4
#    pragma GCC push_options
     // GCC prior to 4.4 did not complain about this
     // need to ignore uninitialized for this statement:
     //   T temp;
#    pragma GCC diagnostic ignored "-Wuninitialized"
#  endif
#endif
template <class T> void NACollection<T>::allocate(CollIndex initLen)
{
  // NOTE: this assumes that the heap_ data member has been set.
  // No other data members need to be set before calling this.
  assert(initLen < MAX_COLL_INDEX);
 
  maxLength_  = initLen;
  usedLength_ = 0;
  entries_    = 0;
  if (maxLength_ > 0)
    {
      // Since new[] and new are different operators, the following
      // can not be combined, even that in new(size_t, NAMemory*)
      // used the ::operator new if NAMemory* is 0.
      if ( heap_ == NABasicObject::systemHeapPtr() )
	{
	  arr_ = new T[maxLength_];
	  usages_ = new CollIndex[maxLength_];
	}
      else
	{	
	  // The method for dynamic allocation should match the one in 
	  // deallocate. When the compiler supports the feature to overload
	  // new[] and delete[], the following two lines should be used
	  // instead.
	  // arr_ = new(heap_) T[maxLength_];
	  // usages_ = new(heap_) CollIndex[maxLength_];
	  arr_ = (T *)heap_->allocateMemory(sizeof(T) * ((size_t) maxLength_));
	  usages_ = (CollIndex *) heap_->allocateMemory(
               sizeof(CollIndex) * ((size_t) maxLength_));

          // To finish up, we copy uninitialized objects of type T into
          // the newly-alloc'd space, so vtbl-pointers get set properly.
          // (This is not always necessary, but it's a good idea in
          // general!)
          T temp; 
          for ( CollIndex i = 0 ; i < maxLength_ ; i++ )
            memcpy ((void*)&arr_[i], (void*)&temp, sizeof(T)) ;
        }
    }
  else
    {
      arr_    = NULL;
      usages_ = NULL;
    }
}
#ifdef __GNUC__
#  if __GNUC__ * 100 + __GNUC_MINOR__ >= 404
     // pop_options added in 4.4
#    pragma GCC pop_options
#  endif
#endif



template <class T> void NACollection<T>::clearFrom( CollIndex entry )
{
  assert( entry >= FIRST_COLL_INDEX );

  for (CollIndex i = entry; i < usedLength_; i++)
    usages_[i] = UNUSED_COLL_ENTRY;

  entries_    = entry;
  usedLength_ = entry;
}

template <class T> T & NACollection<T>::rawEntry(CollIndex ix)
{
  // resize, if index lies out of the allocated portion
  if (ix >= maxLength_)
    resize(ix + 1);
  
  // adjust used part to contain ix
  if (ix >= usedLength_)
    {
      for (CollIndex i = usedLength_; i <= ix; i++)
	usages_[i] = UNUSED_COLL_ENTRY;
      usedLength_ = ix + 1;
    }
  
  // if ix refers to a new entry, initialize its usage
  if (usages_[ix] == UNUSED_COLL_ENTRY)
    {
      entries_++;
      usages_[ix] = NULL_COLL_INDEX;
    }
  
  return arr_[ix];
}


template <class T>
CollIndex NACollection<T>::findUsage(const CollIndex &toFind)
{
  for (CollIndex i = FIRST_COLL_INDEX; i < usedLength_; i++)
    {
      if (usages_[i] == toFind)
	return i;
    }
  
  // return a "not found" indicator
  return NULL_COLL_INDEX;
}

// For HSC, define these methods as inline methods in the Collections.h header file.

// -----------------------------------------------------------------------
// functions for the NASubCollection<T> template
// -----------------------------------------------------------------------

template <class T>
NASubCollection<T>::NASubCollection (NACollection<T> *superset,
                                     NAMemory *heap
                                    )
     : builtin_  ( TRUE ),
       entries_  ( 0 ),
       heap_     ( (heap==NULL && superset!=NULL) ? superset->heap_ : heap ),
       maxLength_( BuiltinSubsetWords ),
       superset_ ( superset ),
       wordSize_ ( BuiltinSubsetWords ),
       lastStaleBit_(0)  
   {
   CollIndex    i     = BuiltinSubsetWords;
   WordAsBits * pBits = (WordAsBits*)(&sbits_[ 0 ]);

   pBits_ = pBits;

   do
      {
      *pBits++ = 0x0;
      }
   while (--i);
   }

template <class T>
NASubCollection<T>::NASubCollection( const NASubCollection<T> &other,
                                     NAMemory * heap
                                   ) 
     : builtin_  ( other.builtin_ ),
       entries_  ( other.entries_ ),
       heap_     ( (heap==NULL) ? other.heap_ : heap ),
       maxLength_( other.maxLength_ ),
       superset_ ( other.superset_ ),
       wordSize_ ( other.wordSize_ ),
       lastStaleBit_( other.lastStaleBit_ )
   {
   CollIndex    i          = maxLength_;
   WordAsBits * pBits;
   WordAsBits * pOtherBits = other.pBits_;

   if (builtin_)
      pBits_ = (WordAsBits*)(&sbits_[ 0 ]);
   else
      pBits_ = allocateBits( maxLength_ );

   pBits = pBits_;

   do
      {
      *pBits++ = *pOtherBits++;
      }
   while (--i);
   }

template <class T>
NASubCollection<T>::~NASubCollection()
{
  if (!builtin_)
    deallocateBits();
}

template <class T>
void NASubCollection<T>::setHeap(CollHeap *heap)
   {
   assert( builtin_ );
   heap_ = heap;
   }

template <class T>
NABoolean NASubCollection<T>::contains( const NASubCollection<T> & other ) const
   {
   assert( superset_ == other.superset_ );

   if (other.entries_)
      {
      CollIndex    otherWords  = other.wordSize_;
      CollIndex    commonWords = MINOF( wordSize_, otherWords );
      CollIndex    i           = commonWords;
      WordAsBits * pBits       = pBits_;
      WordAsBits * pOtherBits  = other.pBits_;
      WordAsBits   w;

      if (i)
         {
         do
            {
            w = *pBits++;

            // for a word present in both, "other" mustn't have bits on that
            // are off in word(i) (in other words, when ORing the two words,
            // other must not cause any changes by having additional bits set)

            if (w != (w LOR *pOtherBits++))
	              return( FALSE );
            }
         while (--i);
         }

      i = otherWords - commonWords;

      if ((Lng32) i > 0)
         {
         do
            {
            // if "other" has any bits set that "this" doesn't have, return FALSE
            if (*pOtherBits++)
               return( FALSE );
            }
         while (--i);
         }
      }

   return( TRUE );
   }


template <class T>
NABoolean NASubCollection<T>::lastUsed(CollIndex &lastBit) const
  {
  CollIndex lastNonZeroWord = getWordSize() - 1;

  // find last non-zero word -- the > 0 test is there to avoid wrapping
  // in case CollIndex is an unsigned integer
  while ((lastNonZeroWord > 0) && (word(lastNonZeroWord) == 0))
    {
      lastNonZeroWord--;
    }

  // at this point, we either found the last non-zero word or there
  // are none

  if (word(lastNonZeroWord) == 0)
    return FALSE;  // return; there are none

  // we know we have the last non-zero word; find last set bit

  lastBit = 
    (lastNonZeroWord << LogBitsPerWord) + BitsPerWord - 1;

  while (!testBit(lastBit))
    {
      lastBit--;
    }
  
  return TRUE;  // found the last set bit
  }

template <class T>
NABoolean NASubCollection<T>::operator == ( const NASubCollection<T> & other ) const
   {
   assert( superset_ == other.superset_ );

   if (other.entries_ != entries_)
      return( FALSE );
   else
      {
      if (other.entries_ == 0 && entries_ == 0)
         return( TRUE );
      }

   CollIndex    commonWords = MINOF( wordSize_, other.wordSize_ );
   CollIndex    i           = commonWords;
   WordAsBits * pBits       = pBits_;
   WordAsBits * pOtherBits  = other.pBits_;

   if (i)
      {
      //
      // compare the common words.
      //
      do
         {
         if (*pBits++ != *pOtherBits++)
            return( FALSE );
         }
      while (--i);
      }

   // if one of the bitmap arrays is longer, then it must contain zeroes.

   i = wordSize_ - commonWords;

   if ((Lng32) i > 0)
      {
      do
         {
	 if (*pBits++)
	    return( FALSE );
         }
      while (--i);
      }
   else
      {
      i = other.wordSize_ - commonWords;

      if ((Lng32) i > 0)
         {
         do
            {
            if (*pOtherBits++)
               return( FALSE );
            }
         while (--i);
         }
      }

   return( TRUE );
   }

template <class T>
NASubCollection<T> & NASubCollection<T>::addSet( const NASubCollection<T> & other )
   {
   CollIndex maxWords   = other.wordSize_;

   assert( superset_ == other.superset_ );

   if (other.entries_)
      {
      if (wordSize_ < maxWords)
         extendWordSize( maxWords );

      if (maxWords)
         {
         WordAsBits * pBits      = pBits_;
         WordAsBits * pOtherBits = other.pBits_;

         if (entries_ == 0)  // We can skip computing the entries.
            {
            do
               {
               *pBits++ |= *pOtherBits++;
               }
            while (--maxWords);

            entries_ = other.entries_;
            }
         else
            {
            Lng32 entryCount    = 0; 
            Lng32 trailingWords = (Lng32) (wordSize_ - maxWords);   // warning elimination

            do
               {
               *pBits |= *pOtherBits++;

               entryCount += ones( *pBits++ );
               }
            while (--maxWords);

            while (trailingWords-- > 0)
               {
               entryCount += ones( *pBits++ );
               }
 
            entries_ = entryCount;
            }
         }
      }
   if (other.lastStaleBit_ > lastStaleBit_)
     lastStaleBit_ = other.lastStaleBit_;

   return( *this );
   }

template <class T>
NASubCollection<T> & NASubCollection<T>::intersectSet( const NASubCollection<T> & other )
   {
   assert( superset_ == other.superset_ );

   if (entries_)
      {
      CollIndex commonWords = MINOF( wordSize_, other.wordSize_ );
      CollIndex i           = commonWords;
      WordAsBits * pBits    = pBits_;

      if (i)
         {
         WordAsBits * pOtherBits = other.pBits_;

        if (other.entries_)
            {
            Lng32 entryCount = 0;

            do
               {
               *pBits &= *pOtherBits++;

               entryCount += ones( *pBits++ );
               }
            while (--i);

            entries_ = entryCount;
            }
         else
            {
            do
               {
               *pBits++ = 0x0;
               }
            while (--i);

            entries_ = 0;
            }
         }

      i = wordSize_ - commonWords;

      if ((Lng32) i > 0)
         {
         do
            {
            *pBits++ = 0x0;
            }
         while (--i);
         }
      }

   return( *this );
   }


template <class T>
NASubCollection<T> & NASubCollection<T>::subtractSet( const NASubCollection<T> & other )
   {
   assert( superset_ == other.superset_ );

   if (other.entries_ && entries_)
      {
      CollIndex commonWords = MINOF( wordSize_, other.wordSize_ );

      if (commonWords)
         {
         Lng32         entryCount    = 0;
         CollIndex    trailingWords = wordSize_ - commonWords;
         WordAsBits * pBits         = pBits_;
         WordAsBits * pOtherBits    = other.pBits_;

         do
            {
            *pBits &= LNOT *pOtherBits++;

            entryCount += ones( *pBits++ );
            }
         while (--commonWords);

         while (trailingWords-- > 0)
            {
            entryCount += ones( *pBits++ );
            } 
         entries_ = entryCount;
         }
      }

   return( *this );
   }


template <class T>
WordAsBits NASubCollection<T>::hash() const
   {
   CollIndex    i      = wordSize_;
   WordAsBits   result = 0x0;
   WordAsBits * pBits  = pBits_;

   if (i && entries_)
      {
      do
         {
         result ^= *pBits++;
         }
      while (--i);
      }

   return( result );
   }

template <class T>
NASubCollection<T> & NASubCollection<T>::complement()
   {
  // can only take the complement of a subset
  assert(superset_ != NULL);

  CollIndex maxWords;
  CollIndex superSetSize = superset_->getSize();

  resize(superSetSize);
  maxWords = getWordSize();

  // for each used entry in the superset, toggle the corresponding subset bit
  for (Int32 i = 0; i < (Int32) superSetSize; i++)
    {
      if (superset_->getUsage(i) == UNUSED_COLL_ENTRY)
	{
	  // a subset shouldn't have an element that's not part of
	  // the superset
	  assert(NOT testBit(i));
	}
      else
	{
	  // is the element in the subset
	  if (testBit(i))
	    // yes, then delete it
	    subtractElement(i);
	  else
	    // no, then add it
	    addElement(i);
	}
    }
  
  return *this;
  }

// -----------------------------------------------------------------------
// functions for the NASet<T> template
// -----------------------------------------------------------------------

template <class T>
NASet<T>::~NASet()
{}

template <class T>
T & NASet<T>::operator [] (CollIndex i)
{
  CollIndex userIndex;
  CollIndex arrayIndex;

  if (i >= this->entries())
    ABORT("Set index exceeds # of entries");

  if (userIndexCache_ <= i)
    {
      // start with the cached position
      userIndex = userIndexCache_;
      arrayIndex = arrayIndexCache_;
    }
  else
    {
      // start with the first occupied entry
      userIndex = 0;
      arrayIndex = 0;

      // skip over unused entries
      while (this->getUsage(arrayIndex) == UNUSED_COLL_ENTRY AND
	     arrayIndex < this->getSize())
	arrayIndex ++;
    }

  // advance to the desired entry
  while (userIndex < i)
    {
      userIndex++;
      arrayIndex++;
      // skip over unused entries
      while (this->getUsage(arrayIndex) == UNUSED_COLL_ENTRY AND
	     arrayIndex < this->getSize())
	arrayIndex ++;
    }

  // cache the results
  userIndexCache_ = userIndex;
  arrayIndexCache_ = arrayIndex;

  return this->usedEntry(arrayIndex);
}

template <class T>
const T & NASet<T>::operator [] (CollIndex i) const
{
  CollIndex userIndex = 0;
  CollIndex arrayIndex = 0;

  if (i >= this->entries())
    ABORT("Set index exceeds # of entries");

  // skip over unused entries
  while (this->getUsage(arrayIndex) == UNUSED_COLL_ENTRY AND
	 arrayIndex < this->getSize())
    arrayIndex ++;
  
  // advance to the desired entry
  while (userIndex < i)
    {
      userIndex++;
      arrayIndex++;
      // skip over unused entries
      while (this->getUsage(arrayIndex) == UNUSED_COLL_ENTRY AND
	     arrayIndex < this->getSize())
	arrayIndex ++;
    }

  return this->constEntry(arrayIndex);
}

template <class T>
NABoolean  NASet<T>::operator==(const NASet<T> &other) const
{
  CollIndex count = this->entries();

  if (count != other.entries())
    return FALSE;

  for (CollIndex i = 0; i < count; i++)
    {
      if (NOT this->contains(other[i]))
	  return FALSE;
    }

  return TRUE;
}

// -----------------------------------------------------------------------
// functions for the NAList<T> template
// -----------------------------------------------------------------------

template <class T>
NAList<T>::~NAList()
{}


template <class T>
NAList<T> & NAList<T>::operator=(const NAList<T> &other)
{
  if ( this != &other ) // avoid copy-self!
    {
      this->deallocate();
      NACollection<T>::copy( (const NACollection<T>&) other);
      first_ = other.first_;
      last_ = other.last_;
      userIndexCache_ = other.userIndexCache_;
      arrayIndexCache_ = other.arrayIndexCache_;
    }
  return *this;
}

template <class T>
NABoolean NAList<T>::operator== (const NAList<T> &other) const
{
  if (this->entries() != other.entries())
    return FALSE;

  for (CollIndex i = 0; i < this->entries(); i++)
    {
      if (NOT (at(i) == other.at(i)))
	return FALSE;
    }

  return TRUE;
}

  
template <class T>
NABoolean NAList<T>::find(const T &elem, T &returnedElem) const
{
  CollIndex foundIndex;
  
  if ((foundIndex = NACollection<T>::find(elem)) == NULL_COLL_INDEX)
    return FALSE;
  else
    {
      returnedElem = this->constEntry(foundIndex);
      return TRUE;
    }
}

template <class T>
T & NAList<T>::operator [] (CollIndex i)
{
  CollIndex userIndex;
  CollIndex arrayIndex;

  if (userIndexCache_ <= i)
    {
      // start with the cached position
      userIndex = userIndexCache_;
      arrayIndex = arrayIndexCache_;
    }
  else
    {
      // start with the first entry
      userIndex = 0;
      arrayIndex = first_;
    }

  while (userIndex < i)
    {
      userIndex++;
      arrayIndex = this->getUsage(arrayIndex);
      if (arrayIndex == NULL_COLL_INDEX)
	ABORT("List index exceeds # of entries");
    }

  userIndexCache_ = userIndex;
  arrayIndexCache_ = arrayIndex;

  return this->usedEntry(arrayIndex);
}

template <class T>
const T & NAList<T>::operator [] (CollIndex i) const
{
  CollIndex userIndex;
  CollIndex arrayIndex;

  if (userIndexCache_ <= i)
    {
      // start with the cached position
      userIndex = userIndexCache_;
      arrayIndex = arrayIndexCache_;
    }
  else
    {
      // start with the first entry
      userIndex = 0;
      arrayIndex = first_;
    }

  while (userIndex < i)
    {
      userIndex++;
      arrayIndex = this->getUsage(arrayIndex);
      if (arrayIndex == NULL_COLL_INDEX)
	{
	  ABORT("List index exceeds # of entries");
	}
    }

  // We cast away const-ness from _this_ so that we can update the 2
  // IndexCache data members.  Semantically, this is alright, since
  // changing those 2 data members does not truly "modify" the NAList
  // object.  But we get a big performance benefit from this.
  //
  // Yes, if MSVC++ supported the mutable keyword reliably, this unusual
  // cast would not be necessary.
  NAList<T>* thisPtr = (NAList<T>*) this ;

  thisPtr->userIndexCache_  = userIndex;
  thisPtr->arrayIndexCache_ = arrayIndex;

  return this->constEntry(arrayIndex);
}


template <class T>
CollIndex NAList<T>::removeCounted(const T &elem, const CollIndex desiredCount)
{
  CollIndex actualCount = 0;
  CollIndex curr = first_;             // current entry
  CollIndex pred = NULL_COLL_INDEX;    // predecessor of current entry

  while (curr != NULL_COLL_INDEX) {

    if (this->usedEntry(curr) == elem) {
      // ok, found (first) matching entry, now delete it
      if (pred == NULL_COLL_INDEX)
	// we have a new first element in the list
	first_ = this->getUsage(curr);
      else
	// unlink this element from the list
	this->setUsage(pred,this->getUsage(curr));

      // delete the actual entry
      NACollection<T>::remove(curr);

      // take care of the last_ pointer
      if (last_ == curr) last_ = pred;

      if (++actualCount == desiredCount) break;
      curr = pred;
    }

    // go to the next element
    pred = curr;
    curr = this->getUsage(curr);
  }

  // invalidate the cache if necessary
  if (actualCount) invalidateCache();

  return actualCount;
}


template <class T>
CollIndex NAList<T>::findArraysIndex(CollIndex i)
{
  CollIndex userIndex;
  CollIndex arrayIndex;

  if (userIndexCache_ <= i)
    {
      // start with the cached position
      userIndex = userIndexCache_;
      arrayIndex = arrayIndexCache_;
    }
  else
    {
      // start with the first entry
      userIndex = 0;
      arrayIndex = first_;
    }

  while (userIndex < i)
    {
      userIndex++;
      arrayIndex = this->getUsage(arrayIndex);
      if (arrayIndex == NULL_COLL_INDEX)
	ABORT("List index exceeds # of entries");
    }

  userIndexCache_ = userIndex;
  arrayIndexCache_ = arrayIndex;

  return arrayIndex;
}

template <class T>
CollIndex NAList<T>::findArraysIndex(CollIndex i) const
{
  CollIndex userIndex;
  CollIndex arrayIndex;

  if (userIndexCache_ <= i)
    {
      // start with the cached position
      userIndex = userIndexCache_;
      arrayIndex = arrayIndexCache_;
    }
  else
    {
      // start with the first entry
      userIndex = 0;
      arrayIndex = first_;
    }

  while (userIndex < i)
    {
      userIndex++;
      arrayIndex = this->getUsage(arrayIndex);
      if (arrayIndex == NULL_COLL_INDEX)
	ABORT("List index exceeds # of entries");
    }

  // We cast away const-ness from _this_ so that we can update the 2
  // IndexCache data members.  Semantically, this is alright, since
  // changing those 2 data members does not truly "modify" the NAList
  // object.  But we get a big performance benefit from this.
  //
  // Yes, if MSVC++ supported the mutable keyword reliably, this unusual
  // cast would not be necessary.
  NAList<T>* thisPtr = (NAList<T>*) this ;

  thisPtr->userIndexCache_  = userIndex;
  thisPtr->arrayIndexCache_ = arrayIndex;

  return arrayIndex;
}

// For HSC, define this method as an inline method in the Collections.h header file.

// A dumb but easy implementation for insert one LIST into another
template <class T>
void NAList<T>::insert(const NAList<T> &other)
{
  CollIndex count = other.entries();
  for (CollIndex i = 0; i < count; i++)
    {
      insert(other[i]);
    } // for loop for iterating over members of the set
} // insert(LIST(T))

// For HSC, define this method as an inline method in the Collections.h header file.

template <class T>
NABoolean NAList<T>::getLast(T &elem)
{
  if (this->entries() > 0)
    {
      // copy last element
      elem = this->usedEntry(last_);

      // remove the last element from the list
      NACollection<T>::remove(last_);

      // fix up the list pointers
      if (first_ == last_)
	first_ = last_ = NULL_COLL_INDEX;
      else
	{
	  last_ = this->findUsage(last_);
	  this->setUsage(last_, NULL_COLL_INDEX);
	}

      // invalidate the cache, if necessary
      if (userIndexCache_ >= this->entries())
	invalidateCache();

      return TRUE;
    }
  else
    return FALSE;
}

// -----------------------------------------------------------------------
// functions for the NAArray<T> template
// -----------------------------------------------------------------------

template <class T>
NABoolean NAArray<T>::operator ==(const NAArray<T> &other) const
{
  if(this->entries() != other.entries())
    return FALSE;

  for(CollIndex i = 0; i < this->entries(); i++)
    {
      if(NOT(at(i) == other.at(i)))
	return FALSE;
    }

  return TRUE;
}

// -----------------------------------------------------------------------
// functions for the NASubArray<T> template
// -----------------------------------------------------------------------

template <class T>
NASubArray<T>::~NASubArray()
{}

// ***********************************************************************
// functions for the Hash Dictionary
// ***********************************************************************

template <class K, class V>
NAHashBucketEntry<K,V>::~NAHashBucketEntry()
{
} //  NAHashBucketEntry<K,V>::~NAHashBucketEntry()
				    
template <class K, class V>
void NAHashBucketEntry<K,V>::clear(NABoolean deleteContents)
{
  if (deleteContents)
    {
      delete key_;
      delete value_;
    }
} //  NAHashBucketEntry<K,V>::clear()
				    
template <class K, class V>
void NAHashBucketEntry<K,V>::display() const
{
  printf("(%p,%p) ",key_, value_);
} //  NAHashBucketEntry<K,V>::display()
				    
template <class K, class V>
Int32 NAHashBucketEntry<K,V>::printStatistics(char *buf)
{
  return sprintf(buf, "(0X%X,0X%X) ",key_, value_);
} //  NAHashBucketEntry<K,V>::printStatistics()
				    
// -----------------------------------------------------------------------
// NAHashBucket::NAHashBucket()
// -----------------------------------------------------------------------
template <class K, class V>
NAHashBucket<K,V>::NAHashBucket(const NAHashBucket<K,V> & other, NAMemory * heap)
     : heap_( (heap==NULL) ? other.heap_ : heap )
       ,bucket_(other.bucket_)
{
} // NAHashBucket<K,V>::NAHashBucket()

template <class K, class V>
NAHashBucket<K,V>::~NAHashBucket()
{
  clear(FALSE); // delete all hash bucket entries
} // NAHashBucket<K,V>::~NAHashBucket()

// -----------------------------------------------------------------------
// NAHashBucket::clear()
// -----------------------------------------------------------------------
template <class K, class V>
void NAHashBucket<K,V>::clear(NABoolean deleteContents)
{
  CollIndex ne = bucket_.entries();
  for (CollIndex index = 0; index < ne; index++)
    {
      bucket_[index]->clear(deleteContents);
      delete bucket_[index];
    }
  bucket_.clear(); // clear list head
} // NAHashBucket<K,V>::clear()

// -----------------------------------------------------------------------
// NAHashBucket::contains()
// Making two separate contains() methods allows a hash dictionary with
// a value type for which we don't have a comparison operator
// -----------------------------------------------------------------------
template <class K, class V>
NABoolean NAHashBucket<K,V>::contains(const K* key) const 
{
  CollIndex ne = bucket_.entries();
  
  for (CollIndex index = 0; index < ne; index++)
    {
      // ----------------------------------------------------------------
      // Index a hash bucket entry.
      // ----------------------------------------------------------------
      NAHashBucketEntry<K,V>* bep = bucket_[index];

      // ----------------------------------------------------------------
      // Compare the stored key value with the given key value.
      // ----------------------------------------------------------------
      if (*bep->getKey() == * key)  // NOTE: uses K::operator==()
	{
          return TRUE;
	} // endif

    } // end for

  return FALSE;

} // NAHashBucket<K,V>::contains()

template <class K, class V>
NABoolean NAHashBucket<K,V>::contains(const K* key, const V* value) const 
{
  CollIndex ne = bucket_.entries();
  
  for (CollIndex index = 0; index < ne; index++)
    {
      // ----------------------------------------------------------------
      // Index a hash bucket entry.
      // ----------------------------------------------------------------
      NAHashBucketEntry<K,V>* bep = bucket_[index];

      // ----------------------------------------------------------------
      // Compare the stored key value with the given key value.
      // ----------------------------------------------------------------
      if (*bep->getKey() == * key)  // NOTE: uses K::operator==()
	{

	  if (value) // a value is also supplied
	    {
	      // --------------------------------------------------------
	      // Compare the stored value with the given value.
	      // --------------------------------------------------------
	      if (*bep->getValue() == * value) // NOTE: uses V::operator==()
		return TRUE;
	    }
	  else           // keys are equal
	    return TRUE;

	} // endif

    } // end for

  return FALSE;

} // NAHashBucket<K,V>::contains()

// -----------------------------------------------------------------------
// NAHashBucket::display()
// -----------------------------------------------------------------------
template <class K, class V>
void NAHashBucket<K,V>::display() const
{
  CollIndex ne = bucket_.entries();
  if (ne > 0)
    {
      for (CollIndex index = 0; index < ne; index++)
	{
	  bucket_[index]->display();
	  if (index > 0 AND (index/4* 4 == index)) printf("\n");
	}
    }
  else
    printf("*** empty ***\n");
} //  NAHashBucket<K,V>::display()
				    
// -----------------------------------------------------------------------
// NAHashBucket::printStatistics()
// -----------------------------------------------------------------------
template <class K, class V>
Int32 NAHashBucket<K,V>::printStatistics(char *buf)
{
  CollIndex ne = bucket_.entries(); Int32 c = 0;
  if (ne > 0) {
    for (CollIndex index = 0; index < ne; index++) {
	  c += bucket_[index]->printStatistics(buf+c);
	  if (index > 0 AND (index/4* 4 == index)) 
        c += sprintf(buf+c,"\n");
	}
  }
  else {
    c = sprintf(buf,"*** empty ***\n");
  }
  return c;
} //  NAHashBucket<K,V>::printStatistics()
				    
// -----------------------------------------------------------------------
// NAHashBucket::getFirstValue()
// -----------------------------------------------------------------------
template <class K, class V>
V* NAHashBucket<K,V>::getFirstValue(const K* key) const
{
  CollIndex ne = bucket_.entries();
  
  for (CollIndex index = 0; index < ne; index++)
    {
      // ----------------------------------------------------------------
      // Index a hash bucket entry.
      // ----------------------------------------------------------------
      NAHashBucketEntry<K,V>* bep = bucket_[index];

      // ----------------------------------------------------------------
      // Compare the stored key value with the given key value.
      // ----------------------------------------------------------------
      if (*bep->getKey() == *key)  // NOTE: uses K::operator==()
	return bep->getValue();
      
    } // end for

  return NULL; // key not found in the hash dictionary
  
} // NAHashBucket<K,V>::getFirstValue()

// -----------------------------------------------------------------------
// NAHashBucket::getKeyValuePair()
// -----------------------------------------------------------------------
template <class K, class V>
void NAHashBucket<K,V>::getKeyValuePair(const K* key, const V* value,
				        NAHashBucket<K,V>& container) const
{
  CollIndex ne = bucket_.entries();
  
  if (key)
    { // a key is given
      for (CollIndex index = 0; index < ne; index++)
	{
	  // -------------------------------------------------------------
	  // Index a hash bucket entry.
	  // -------------------------------------------------------------
	  NAHashBucketEntry<K,V>* bep = bucket_[index];

	  // -------------------------------------------------------------
	  // Compare the stored key value with the given key value.
	  // -------------------------------------------------------------
	  if (*bep->getKey() == * key)  // NOTE: uses K::operator==()
	    {

	      if (value) // a value is also supplied
		{
		  // -----------------------------------------------------
		  // Compare the stored value with the given value.
		  // -----------------------------------------------------
		  if (*bep->getValue() == * value) // NOTE: uses V::operator==()
		    container.insert(bep->getKey(), bep->getValue());
		}
	      else      // keys are equal
		container.insert(bep->getKey(), bep->getValue());

	    } // endif stored key == given key

	} // end for

    } // a key is given
  else
    {
      // Return all hash bucket entries
      for (CollIndex index = 0; index < ne; index++)
	container.insert(bucket_[index]->getKey(), bucket_[index]->getValue());
    }
  
} // NAHashBucket<K,V>::getKeyValuePair()

// -----------------------------------------------------------------------
// NAHashBucket::remove()
// -----------------------------------------------------------------------
template <class K, class V>
K* NAHashBucket<K,V>::remove(K* key)
{
  CollIndex ne = bucket_.entries();
  
  for (CollIndex index = 0; index < ne; index++)
    {
      // ----------------------------------------------------------------
      // Index a hash bucket entry.
      // ----------------------------------------------------------------
      NAHashBucketEntry<K,V>* bep = bucket_[index];

      // ----------------------------------------------------------------
      // Compare the stored key value with the given key value.
      // ----------------------------------------------------------------
      if (*bep->getKey() == *key)  // NOTE: uses K::operator==()
	{
	  bucket_.removeAt(index);
      delete bep;
	  return key;
	}
      
    } // end for

  return NULL; // key not found in the hash dictionary
  
} // NAHashBucket<K,V>::remove()

// ----------------------------------------------------------------------
// NAHashDictionary constructor functions
// ----------------------------------------------------------------------
template <class K, class V>
NAHashDictionary<K,V>::NAHashDictionary(
// see long detailed comment in Collections.h about the hash function param.
			ULng32 (*hashFunction)(const K &), 
			ULng32 hashSize,
			NABoolean enforceUniqueness,
			NAMemory* heap)
			: heap_(heap),
		       hash_(hashFunction),
		       entries_(0), 
		       enforceUniqueness_(enforceUniqueness)
{
   createHashTable(hashSize);
}

template <class K, class V>
NAHashDictionary<K,V>::NAHashDictionary (const NAHashDictionary<K,V> & other,
                                         NAMemory * heap) 
     : heap_( (heap==NULL) ? other.heap_ : heap ),
       hash_(other.hash_),
       entries_(other.entries_), 
       enforceUniqueness_(other.enforceUniqueness_)
{
  createHashTable(other.hashSize_);
  for (CollIndex index = 0; index < hashSize_; index++)
     (*hashTable_)[index] = (*other.hashTable_)[index];
}

// ----------------------------------------------------------------------
// NAHashDictionary destructor function.
// ----------------------------------------------------------------------
template <class K, class V>
NAHashDictionary<K,V>::~NAHashDictionary() 
{
  for (CollIndex index = 0; index < hashSize_; index++)
    delete (*hashTable_)[index];
  delete hashTable_; 
}

// ----------------------------------------------------------------------
// NAHashDictionary::clear()
// Deletes all entries in the hash buckets.
// Does not delete any key or value.
// Does not delete any hash bucket or the hash table.
// ----------------------------------------------------------------------
template <class K, class V>
void NAHashDictionary<K,V>::clear(NABoolean deleteContents)
{
  for (ULng32 index = 0; index < hashSize_; index++)
    (*hashTable_)[index]->clear(deleteContents);
  entries_ = 0;
} //  NAHashDictionary<K,V>::clear()

// ----------------------------------------------------------------------
// NAHashDictionary::createHashTable()
// Helper function for creating a hash table.
// ----------------------------------------------------------------------
template <class K, class V>
void NAHashDictionary<K,V>::createHashTable(ULng32 hashSize)
{
  assert (hashSize > 0);
  hashSize_ = hashSize;
  hashTable_ = new(heap_) NAArray<NAHashBucket<K,V>* >(heap_,hashSize);
  for (CollIndex index = 0; index < hashSize; index++)
     hashTable_->insertAt(index, new(heap_) NAHashBucket<K,V>(heap_));
} //  NAHashDictionary<K,V>::createHashTable()

// ----------------------------------------------------------------------
// NAHashDictionary::getHashCode()
// Function for generating a hash code
// ----------------------------------------------------------------------
template <class K, class V>
ULng32 NAHashDictionary<K,V>::getHashCode(const K& key) const
{
  // use the key's hash method to get the hash value
//  unsigned long hashValue = key.hash() % hashSize_;
//#else
  ULng32 hashValue = hash_(key) % hashSize_;
//#endif
  assert(hashValue < hashSize_);
  return hashValue;
} //  NAHashDictionary<K,V>::getHashCode()

// ----------------------------------------------------------------------
// NAHashDictionary::insert()
// ----------------------------------------------------------------------
template <class K, class V>
K* NAHashDictionary<K,V>::insert(K* key, V* value) 
{ 
  if (enforceUniqueness_ AND contains(key))
    {
      assert(enforceUniqueness_);
      return NULL; // don't insert a duplicate key
    }
  (*hashTable_)[getHashCode(*key)]->insert(key, value); 
  entries_++;
  return key;
} // NAHashDictionary<K,V>::insert()				

// ----------------------------------------------------------------------
// NAHashDictionaryIterator::ctor()
// ----------------------------------------------------------------------
template <class K, class V>
NAHashDictionaryIterator<K,V>::NAHashDictionaryIterator (const NAHashDictionary<K,V> & dict,
                                                         const K* key, 
                                                         const V* value)
     : iterator_(dict.heap_)
{
  if (key)
    (*(dict.hashTable_))[dict.getHashCode(*key)]->getKeyValuePair(key,value,iterator_);
  else 
    {      
      // iterate over all key value pairs in the hash table
      for (CollIndex index = 0; index < dict.hashSize_; index++)
	(*(dict.hashTable_))[index]->getKeyValuePair(key,value,iterator_) ; 
    } 
  // iterator_ now contains all the qualifying key value pairs  
  reset() ; // set the position so we're ready to iterate

} // NAHashDictionaryIterator<K,V>()

// ----------------------------------------------------------------------
// NAHashDictionaryIterator::copy ctor()
// ----------------------------------------------------------------------
template <class K, class V>
NAHashDictionaryIterator<K,V>::NAHashDictionaryIterator (const NAHashDictionaryIterator<K,V> & other, NAMemory * heap)
     : iterator_(heap) // NB: we always put copies on the stmt heap
{
  iterator_         = other.iterator_ ; 
  iteratorPosition_ = other.iteratorPosition_ ; 
} // NAHashDictionaryIterator<K,V>() copy ctor()

// ----------------------------------------------------------------------
// ~NAHashDictionaryIterator()
// ----------------------------------------------------------------------
template <class K, class V>
NAHashDictionaryIterator<K,V>::~NAHashDictionaryIterator() 
{
  iterator_.clear() ; 
  reset() ; 
} // ~NAHashDictionaryIterator<K,V>

// ----------------------------------------------------------------------
// NAHashDictionaryIterator::getNext()
// ----------------------------------------------------------------------
template <class K, class V>
void NAHashDictionaryIterator<K,V>::getNext(K*& key, V*& value)
{
#pragma warning (disable : 4018)  //warning elimination
  if (iteratorPosition_ < iterator_.entries())
    {
      key = iterator_[iteratorPosition_]->getKey();
      value = iterator_[iteratorPosition_]->getValue();
      iteratorPosition_++ ;
    }
#pragma warning (default : 4018)  //warning elimination
  else
    {
      // If the application has advanced the iterator beyond the number
      // of entries, signal that there are no more keys and values.
      key = NULL;
      value = NULL;
    }

} // NAHashDictionaryIterator<K,V>::getNext()				

// ----------------------------------------------------------------------
// NAHashDictionary::remove()
// ----------------------------------------------------------------------
template <class K, class V>
K* NAHashDictionary<K,V>::remove(K* key)
{
  K* removedKey = (*hashTable_)[getHashCode(*key)]->remove(key);
  if (removedKey)
    entries_--;
  return removedKey;
} // NAHashDictionary<K,V>::remove()				

// ----------------------------------------------------------------------
// NAHashDictionary::resize()
// ----------------------------------------------------------------------
template <class K, class V>
void NAHashDictionary<K,V>::resize(ULng32 newHashSize)
{
  assert(newHashSize > 0);

  // TODO -- if indeed this is dead code -- remove this method !

  // This method is not used anywhere, and makes calls to undefined functions
  // like iteratorCreate() and iteratorDestroy()
  assert(FALSE);
} // NAHashDictionary<K,V>::resize()				

// ----------------------------------------------------------------------
// NAHashDictionary::display()
// ----------------------------------------------------------------------
template <class K, class V>
void NAHashDictionary<K,V>::display() const
{
  if (hashSize_ > 0)
    {
      for (CollIndex index = 0; index < hashSize_; index++)
	{
	  printf("\nbucket[%d] : \n",index);
	  (*hashTable_)[index]->display();
	}
    }
  else
    printf("*** hash table is empty ***");
  printf("\n");
} //  NAHashDictionary<K,V>::display()
				    
// ----------------------------------------------------------------------
// NAHashDictionary::printStatistics()
// ----------------------------------------------------------------------
template <class K, class V>
Int32 NAHashDictionary<K,V>::printStatistics(char *buf)
{
  Int32 c = 0;
  if (hashSize_ > 0) {
    for (CollIndex index = 0; index < hashSize_; index++) {
	  c += sprintf(buf+c, "\nbucket[%d] : \n",index);
	  c += (*hashTable_)[index]->printStatistics(buf+c);
	}
  }
  else 
    c = sprintf(buf,"*** hash table is empty ***");
  c += sprintf(buf+c,"\n");
  return c;
} //  NAHashDictionary<K,V>::printStatistics()

#pragma warn(1506)   // warning elimination 

