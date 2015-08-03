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

#include "Collections.h"

// -------------------------------------------------------------------------------------
//
// Class ComPointerList methods
//
template <class Elem>
ComPointerList<Elem>::ComPointerList( CollHeap * heap
                                    , const CollIndex initSize
                                    , const CollIndex increment
                                    )
                                    : heap_ (heap)
                                    , increment_ (increment)
                                    , allocated_ (initSize)
                                    , inUse_ (0)
                                    , arr_ (NULL)
{
  if (initSize > 0)
    arr_ = new (heap_) Elem* [initSize];
}

template <class Elem>
ComPointerList<Elem>::~ComPointerList (void)
{
  // It is the responsibility of the instantiator to deallocate
  // what is pointed to.
  if (allocated_ > 0)
    NADELETEBASIC(arr_, heap_);
}

// Operators
template <class Elem>
Elem & ComPointerList<Elem>::operator[] (const CollIndex index)
{
  abortIfOutOfBounds(index);
  return *(arr_ [index]);
}

template <class Elem>
const Elem & ComPointerList<Elem>::operator[] (const CollIndex index) const
{
  abortIfOutOfBounds(index);
  return *(arr_ [index]);
}

// Insert pointer at specific position
template <class Elem>
void ComPointerList<Elem>::insertAt ( const Elem & element          // Insert the pointer
                                    , const CollIndex index)        // at the specified position
{
  
  // Increase inUse_ up front, to prevent an out of bounds condition 
  // on insert at the end
  inUse_++;

  abortIfOutOfBounds(index);
  // First, check if we need to allocate more room
  if (inUse_ > allocated_)
  {
    // all allocated entries (if any) are in use, get some more
    allocated_ += increment_;
    Elem ** newArr = new (heap_) Elem* [allocated_];

    // copy existing element pointers to new array
    if (inUse_ > 1)
      memcpy (newArr, arr_, ((inUse_ - 1) * sizeof(*newArr)));

    // remove old array and store pointer to new array
    if (allocated_ > increment_)
      NADELETEBASIC(arr_, heap_);
    arr_ = newArr;
  }

  // Move subsequent elements one up
  for (CollIndex i = inUse_-1;i > index;i--)
    arr_[i] = arr_[i-1];

  // Insert element
  arr_[index] = (Elem *)&element;
}


template <class Elem>
NABoolean ComPointerList<Elem>::insert (const Elem & element, CollIndex & position) 
{
  // Insert at the end
  position = inUse_;
  insertAt (element, inUse_);
  return TRUE;
}

template <class Elem>
NABoolean ComPointerList<Elem>::insert (const Elem & element)
{
  // Insert at the end
  CollIndex dummyPosition; 
  return insert (element, dummyPosition);
}

// Removal
template <class Elem>
void ComPointerList<Elem>::remove (const CollIndex index)           // Remove a pointer
{
  abortIfOutOfBounds(index);

  CollIndex i;

  // decrease used space
  inUse_--;

  // Move subsequent entries one down
  for (i = index;i < inUse_; i++)
    arr_[i] = arr_[i + 1];

  // See if tail chunk needs to be deallocated.
  // We will not deallocate the last chunk. However,
  // the logic does not preserve an initial allocation
  // greater than the increment. 
  if ( (allocated_ == inUse_ + increment_)   &&
       (allocated_ > increment_)
     )
  {
    allocated_ -= increment_;
    Elem ** newArr = new (heap_) Elem* [allocated_];

    // copy existing element pointers to new array
    for (i = 0; i < inUse_; i++)
      newArr[i] = arr_[i];

    // remove old array and store pointer to new array
    NADELETEBASIC(arr_, heap_);
    arr_ = newArr;
  }
}

template <class Elem>
void ComPointerList<Elem>::clear (void)                             // Remove all pointers
{
  // remove old array
  NADELETEBASIC(arr_, heap_);

  // allocate minimum array
  allocated_ = increment_;
  arr_ = new (heap_) Elem* [allocated_];

  // clear in-use
  inUse_ = 0;
}

template <class Elem>
void ComPointerList<Elem>::abortIfOutOfBounds (const CollIndex index) const
{
  if (index >= inUse_)
    // Programming error - eat dirt!
    ABORT ("Index exceeds # of entries");
}

// -------------------------------------------------------------------------------------
//
// Class ComOrderedPointerSet methods
//
// Find an element that may be in the list. 
template <class Elem, class Key>
CollIndex ComOrderedPointerSet<Elem, Key>::find (const Key & key) const
{
  CollIndex index;

  if (findPositionToInsert (index, key))
    // It's there, return its position
    return index;
  else
    // It's not there ...
    return NULL_COLL_INDEX;
}

// Find position to insert element. Return TRUE if element is there already,
// FALSE if not. Requires element classes to define comparison operators.
template <class Elem, class Key>
NABoolean ComOrderedPointerSet<Elem, Key>::findPositionToInsert (CollIndex & position, const Key & key) const
{
  position = 0;
  if (this->inUse_)
  {
    // Only go searching if set contains elements.
    // Optimisation: Start searching from last inserted position.
    // Saves searching if inserts are ordered in advance.
    if (key >= (*this)[lastInserted_].getKey())
      position = lastInserted_ + 1;

    while (position < this->inUse_)
    {
      if (key < (*this)[position].getKey())
        // found position to insert, quit
        break;

      // try next element
      position++;
    }
  }

  if (  (position)          &&
        (key == (*this)[position-1].getKey())
     )
  {
    // Element is in set already. Return the position of the element.
    position--;
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

// Mutators

// Insert the element pointer at the correct position
template <class Elem, class Key>
NABoolean ComOrderedPointerSet<Elem, Key>::insert (const Elem & element, CollIndex & position)       
{
  // Return if element is there already
  if (findPositionToInsert (position, element.getKey()))
    return FALSE;

  // OK, it isn't there - insert it at the found position
  insertAt (element, position);
  return TRUE;
}

template <class Elem, class Key>
NABoolean ComOrderedPointerSet<Elem, Key>::insert (const Elem & element)
{
  CollIndex dummyPosition; 
  return insert (element, dummyPosition);
}

template <class Elem, class Key>
void ComOrderedPointerSet<Elem, Key>::merge (const ComOrderedPointerSet<Elem, Key> & sourceSet)
{
  for (CollIndex i = 0;i<sourceSet.entries();i++) 
    insert (sourceSet[i]); 
}

template <class Elem, class Key>
void ComOrderedPointerSet<Elem, Key>::insertAt ( const Elem & element, const CollIndex index)
{
  ComPointerList<Elem>::insertAt (element, index); 
  lastInserted_ = index; 
}

// Removal
template <class Elem, class Key>
void ComOrderedPointerSet<Elem, Key>::remove (const CollIndex index)       // One element
{
  ComPointerList<Elem>::remove (index);

  // Invalidate lastInserted_ if needed
  if (this->inUse_ == 0)
    lastInserted_ = 0;
  else
    if (lastInserted_ >= this->inUse_)
      lastInserted_ = this->inUse_ - 1;
}

template <class Elem, class Key>
void ComOrderedPointerSet<Elem, Key>::clear (void)                        // All elements
{
  ComPointerList<Elem>::clear ();
  lastInserted_ = 0;
}

//--------------------------------------------------------------------
//
// Class ComOrderedSet methods

template <class Elem, class Key>
ComOrderedSet<Elem, Key>::~ComOrderedSet (void)
{
  // Deallocate all elements
  for (CollIndex index = 0; index < this->inUse_; index++)
    if (this->heap_)
      NADELETE (&((*this)[index]), Elem, this->heap_);
    else
      delete &((*this)[index]);
}

// Insert a pointer to a copy of an element. 
template <class Elem, class Key>
NABoolean ComOrderedSet<Elem, Key>::insert (const Elem & element, CollIndex & position)
{
  if (ComOrderedPointerSet<Elem, Key>::findPositionToInsert (position, element.getKey()))
    // Element is in set already
    return FALSE;

  const Elem * copyOfElement = new (this->heap_) Elem (element);
  ComPointerList<Elem>::insertAt (*copyOfElement, position);
  return TRUE;
}

template <class Elem, class Key>
NABoolean ComOrderedSet<Elem, Key>::insert (const Elem & element)
{
  CollIndex dummyPosition; 
  return insert (element, dummyPosition);
}

template <class Elem, class Key>
void ComOrderedSet<Elem, Key>::merge (const ComOrderedSet<Elem, Key> & sourceSet)
{
  for (CollIndex i = 0;i<sourceSet.entries();i++) 
    insert (sourceSet[i]); 
}

// Remove an entry, deallocate the element pointed to.
template <class Elem, class Key>
void ComOrderedSet<Elem, Key>::remove (const CollIndex index)
{
  Elem & element = (*this)[index];

  // remove and deallocate the element
  ComOrderedPointerSet<Elem, Key>::remove (index);
  if (this->heap_)
    NADELETE (&element, Elem, this->heap_);
  else
    delete &element;
}

  
// Remove all entries, deallocate all elements.
template <class Elem, class Key>
void ComOrderedSet<Elem, Key>::clear (void)
{

  // Deallocate all elements
  for (CollIndex index = 0; index < this->inUse_; index++)
    if (this->heap_)
      NADELETE (&((*this)[index]), Elem, this->heap_);
    else
      delete &((*this)[index]);

  // Clear pointer array
  ComOrderedPointerSet<Elem, Key>::clear ();
}
