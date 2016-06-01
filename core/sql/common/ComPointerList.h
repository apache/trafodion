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
#ifndef COMPOINTERLIST_H
#define COMPOINTERLIST_H

#include "BaseTypes.h"
#include "NAMemory.h"

#define ARRAY_INITIAL_SIZE 0
#define ARRAY_INCREMENT_SIZE 16
template <class Elem> class ComPointerList;
template <class Elem, class Key> class ComOrderedSet;
template <class Elem, class Key> class ComOrderedPointerSet;

// -------------------------------------------------------------------------------------
//
// Class ComPointerList
//
template <class Elem> class ComPointerList
{
public:
  // Default constructor. 
  // Methods of this class uses the heap for pointer array allocations only.
  ComPointerList ( CollHeap * heap = NULL
                 , const CollIndex initSize = ARRAY_INITIAL_SIZE
                 , const CollIndex increment = ARRAY_INCREMENT_SIZE
                 );
  virtual ~ComPointerList ();

  // Accessors
  CollIndex entries (void) const
    {return inUse_;};

  // Operators
  Elem & operator[] (const CollIndex index);
  const Elem & operator[] (const CollIndex index) const;

  // Mutators 

  // Insert element at specified position. 
  virtual void insertAt ( const Elem & element, const CollIndex index);    // Insert the element pointer into the array

  // Insert element pointer at the end. Declared as NABoolean so that ComOrderedSet can override.
  // Will unconditionally return TRUE.
  virtual NABoolean insert (const Elem & element, CollIndex & position);   // Return position of inserted element
  virtual NABoolean insert (const Elem & element);

  // Element removal. 
  virtual void remove (const CollIndex index);              // Remove a single element pointer 
  virtual void clear (void);                                // Remove all element pointers

protected:
  CollHeap * heap_;         // User defined heap or NULL
  CollIndex  inUse_;        // Number of elements in use

private:

  // Copy constructor, don't implement
  ComPointerList (const ComPointerList<Elem> & rhs);

  Elem    ** arr_;          // Array of pointers to elements
  CollIndex  allocated_;    // Number of allocated elements.
  CollIndex  increment_;    // The size of the chunk for allocation

  void abortIfOutOfBounds (const CollIndex index) const;    // Aborts if index is out of bounds

};

// -------------------------------------------------------------------------------------
//
// Class ComOrderedPointerSet
//

template <class Elem, class Key> class ComOrderedPointerSet : public ComPointerList<Elem>
{
public:
  // Default constructor. 
  // Methods of this class don't use the heap.
  ComOrderedPointerSet ( CollHeap * heap = NULL
                       , const CollIndex initSize = ARRAY_INITIAL_SIZE
                       , const CollIndex increment = ARRAY_INCREMENT_SIZE
                       )
                       : ComPointerList<Elem> (heap, initSize, increment)
                       , lastInserted_ (0)
  {};

  virtual ~ComOrderedPointerSet () {};

  // Accessors
  CollIndex find (const Key & key) const;              // Find an element that may be in the list

  // Mutators

  // Insert element pointer in the order specified by the element class' comparison operators. 
  // Return TRUE if the element was inserted, FALSE if it exists in the set already.
  // Maintains lastInserted_. These two methods are intended for the situation where the element
  // already has been constructed
  virtual NABoolean insert (const Elem & element, CollIndex & position);   // Return position of inserted element
  virtual NABoolean insert (const Elem & element);

  // The next two methods are intended for the situation where the element will not be
  // constructed if it is in the set already. insertAt should never be called without a 
  // preceding call to findPositionToInsert.
  NABoolean findPositionToInsert (CollIndex & position, const Key & key) const; // Return TRUE if element is there already, FALSE if not.
  virtual void insertAt ( const Elem & element, const CollIndex index);         // Insert the element pointer into the array, maintain lastInserted_
                                                                                
  // Insert all elements from another pointer set. 
  void merge (const ComOrderedPointerSet<Elem, Key> & sourceList);

  // Element removal. 
  virtual void remove (const CollIndex index);              // Remove a single element pointer 
  virtual void clear (void);                                // Remove all element pointers


private:
  // Copy constructor, don't implement
  ComOrderedPointerSet (const ComOrderedPointerSet<Elem, Key> & rhs);

  CollIndex  lastInserted_; // The index of the last inserted element. Ordering optimisation.

};

//--------------------------------------------------------------------
//
// Class ComOrderedSet

template <class Elem, class Key> class ComOrderedSet : public ComOrderedPointerSet<Elem, Key>
{

public:

  // Default constructor
  ComOrderedSet ( CollHeap * heap = NULL
                , const CollIndex initSize = ARRAY_INITIAL_SIZE
                , const CollIndex increment = ARRAY_INCREMENT_SIZE
                ) 
                : ComOrderedPointerSet<Elem, Key> (heap, initSize, increment)
  {};

  virtual ~ComOrderedSet (void);

  // Mutators

  // Element removal. 
  virtual void remove (const CollIndex index);              // Remove and deallocate a single entry
  virtual void clear (void);                                // Remove and deallocate all entries

  // Element insertion. Insert a pointer to a copy of the element
  virtual NABoolean insert (const Elem & element, CollIndex & position);   // Return position of inserted element
  virtual NABoolean insert (const Elem & element);

  // Insert a copy of each element from another ordered set. 
  inline void merge (const ComOrderedSet<Elem, Key> & sourceList);

private:
  // Copy constructor, don't implement
  ComOrderedSet (const ComOrderedSet<Elem, Key> & rhs);
};

// -----------------------------------------------------------------------
// This is done similarly to Tools.h++: if we want to instantiate
// templates at compile time, the compiler needs to know the
// implementation of the template functions. Do this by setting the
// preprocessor define NA_COMPILE_INSTANTIATE.
// -----------------------------------------------------------------------
#ifdef NA_COMPILE_INSTANTIATE
#include "ComPointerList.cpp"
#endif

#endif // COMPOINTERLIST_H
