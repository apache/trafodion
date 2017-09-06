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
#ifndef COLLECTIONS_H
#define COLLECTIONS_H
/* -*-C++-*-
******************************************************************************
*
* File:         Collections.h
* Description:  Collection type templates
* Created:      4/26/94
* Language:     C++
*
*
******************************************************************************
*/
 
// -----------------------------------------------------------------------

#include <limits.h>
#include "BaseTypes.h"

#include <stdint.h>



/**
*** _m64_popcnt is an instruction-level intrinsic routine supported in the
*** IA64 compiler.  It quickly counts the number of bits set in a 64-bit 
*** register.  Although we define the intrinsics prototype here, we should
*** replace this with an include to "builtin.h" once the intrinsic is
*** officially supported. 
**/


// -----------------------------------------------------------------------
// File contents
// -----------------------------------------------------------------------

#pragma nowarn(1506)   // warning elimination

template <class T> class NACollection;
template <class T> class NASubCollection;
template <class T> class NASet;
template <class T> class NAList;
template <class T> class NAArray;
template <class T> class NASimpleArray;
template <class T> class NASubArray;
template <class K,class V> class NAHashDictionary;
template <class K,class V> class NAHashDictionaryIterator;
template <class K,class V> class NAKeyLookup;

// -----------------------------------------------------------------------
// Collection types implement collections of arbitrary class objects.
// The Noah code uses collection types to implement lists, sets, and
// other collections of classes. We intend the Noah code to be as
// independent as possible from the actual implementation of the
// collection type, although a complete independence is not always
// possible.
//
// Here are some points about how collection types are used:
//
// - Different collections, like SET, LIST, ARRAY, are defined in this
//   file. Only if necessary, the methods for those collections are defined
//   differently. For example, the entries() method will
//   work for all the mentioned collection type templates(although for
//   a set the name card() instead of entries() would be more appropriate).
//   This makes switching from one collection template to another more
//   easy. For example, one might detect that instead of a set one
//   wants a list, if it becomes necessary to identify elements by
//   position.
//
// - Collection type names are not given using the template syntax
//   when they are defined in code outside of this file. Instead, a
//   #define is used. This allows more changes in the implementation of
//   collection types without changing all the places where collection
//   types are defined(for example using the style of "generic.h").
//
// - In order to create a collection of an object of type T, the type
//   T must provide the following:
//   + a default constructor(sometimes it is necessary to create
//     empty entries in a collection)
//   + well-defined assignment semantics(copy constructor and
//     assignment operator), since objects are copied into the collection
//    (see below)
//   + a comparison operator
//           operator  ==(const T &other) const
//     for searching an object in a collection
//
// - Policy of ownership: the collection types defined here take ownership
//   of all objects that are part of the collection. So, if a collection of
//   objects of type X is defined and an object o of type X is inserted
//   into that collection, then a copy of o is made and retained in the
//   collection. If o is removed from the collection, the implementation
//   of the collection is free to delete that copy of o immediately, at
//   a later point in time, or never.
//
//   In many cases this is not what a user of a collection wants, since
//   it wastes space and time. Also, if an object o is part of several
//   collections, having multiple copies of it in those collections
//   would mean that one would have
//   to change all of the copies when o needs to be modified. As a
//   solution to this one can define a collection of pointers to X and
//   add a pointer to object o to the collection. There is no special
//   pointer-based collection, since this can be achieved by defining
//   collections on the pointer data type.
//
//   When creating the copy of the object to be inserted, the assignment
//   operator is used, so a valid operator = and a valid copy constructor
//   must be defined for the base type of the collection.
//
// - The SET, LIST, and ARRAY collections are based on an implementation
//   that uses a variable-length array. When the array is created, the
//   default constructor for all objects is called. However, users of a
//   collection must always overwrite array entries explicitly before
//   being able to use them. This is to avoid having uninitialized
//   entries for datatypes with no explicit constructor.
//
// - The simplest collection type template is that of a set. A set collection
//   assures that no two elements of the collection compare equal(as a
//   consequence, the operator == must be defined on the base type of the
//   set).
//
// - More complicated collections are lists. Neither sets, nor lists
//   have a pre-set limit on the number of elements that they can
//   contain. While sets avoid duplicate entries by using the == operator
//   of the underlying base type, lists allow duplicate entries,
//   since they use an index to identify an element.
//
// - To identify a list entry, an integral data type
//   CollIndex is defined in this file. The convention is that a value
//   of 0 identifies the first position in a collection.
//
// - In a list, entries are always numbered consecutively from 0 to n-1
//   where n is the number of entries. If entries are inserted between
//   existing elements or entries are deleted that are not at the end of
//   the list, then the indexes of other list entries are modified such
//   that the list remains contiguous with the first entry having index 0.
//
// - The array collection is intended mainly for performance-critical
//   cases where a fast direct and random access through the index operator
//   is needed(mostly for lookup tables with an integer key). Also,
//   unlike a list, an entry in an array never changes its index. It is
//   possible for an array to have "holes" meaning unused entries in
//   between used ones.
//
// - The following methods are defined for all collections:
//
//   + default constructor(create an empty collection with no limit for
//     the number of elements)
//
//   + copy constructor(make a new collection from an existing one)
//
//   + a method "entries" to determine the number of entries in the
//     set, array, or list
//
//   + a method "insert" to insert an entry without specifying its position
//
//   + a method "remove" to remove an element given by its value
//    (all entries with that value ?are removed? if the collection contains
//     duplicate values?
//     For lists, "remove" removes the first matching element;
//     "removeAll" removes all matching elements.)
//     Note that collections of pointers to objects will compare as equal
//     only if the pointers point to the same object, not if the pointers
//     point to different objects that have the "same" value!
//
//   + a method "clear" to remove all entries from the collection.
//
//   + a(generally non-commutative) method "insert" that allows to insert
//     the contents of one collection into another one(it is allowed to insert
//     sets into lists, lists into arrays, etc.) without defined positioning
//     of the inserted elements
//
//   + a method "contains" that checks whether a certain entry is in the
//     collection(this uses the == operator of the base type)
//     Note that collections of pointers to objects will compare as equal
//     only if the pointers point to the same object, not if the pointers
//     point to different objects that have the "same" value!
//
//   + a method "find" that finds and returns a given entry(using
//     the == operator of the base type)
//     Note that collections of pointers to objects will compare as equal
//     only if the pointers point to the same object, not if the pointers
//     point to different objects that have the "same" value!
//
// - The following methods are defined for the list type template:
//
//   + index access operators(both as const and non-const versions,
//     returning a value or an updatable reference of the object)
//
//   + an "insertAt" method to insert an entry at a specified position
//
//   + a "removeAt" method that removes an entry with a given index
//
//   + a "removeAll" method that removes all matching elements
//    (see additional notes above)
//
//   + an "index" method to find the index of an element
//
//   + "getFirst" and "getLast" methods to implement queues and
//     stacks(FIFO and LIFO lists)
//
// - The array template has the following method in addition to the other
//   collection templates:
//
//   + a method "used" that indicates whether a particular index of the
//     array is used or not.
//
// - Sometimes it is important to have very dense representations of sets
//   and to be able to perform set operations(union, intersect, add/
//   remove elements) very efficiently. Often, many subsets of a given
//   set are created and it is important to deal with the subsets very
//   efficiently.
//
//   This file defines a collection type template designed for this
//   special situation: a subset collection is defined on a particular
//   instance of a collection(right now this is only used for array
//   collections). A subset collection represents each element of the
//   super-collection as a bit. The subset collection has a built-in array
//   of enough bits to handle small to medium size super-collections.
//
//   Set operations between subsets from the same super-collection can
//   be done very fast by using bitwise AND and OR operators. Two subsets
//   that don't reference the same super-collection are not comparable.
//
//   Note that inconsistencies may arise when the super-collection changes
//   while subsets are defined for it. With the array collection these
//   conflicts are manageable by the user, since a subset merely
//   identifies a subset of the index positions of an array, without
//   making a statement about the contents of the array. Subsets on list,
//   however, may require updates to the list, except replacement of list
//   elements with another value and insertions at the end of the list.
//
//   Subset collection types don't have union and intersect operators
//   defined in the traditional way. This is because such operators
//   typically require the creation of a temporary object. The same
//   functionality can be achieved with union and intersect operators
//   that work in-place, meaning that they modify a subset. This puts
//   allocation of temporary objects into the hands of the user and
//   makes problems with temporaries more visible.
//
// - Finally, a fast implementation of a hash lookup table is defined,
//   using the Tools.h++ library implementation. The lookup table is
//   a pointer-based collection. In order to create a hash lookup table
//   for an object, a hash function must be defined for it that maps
//   the object into a random character string.
//
// - For users who want to control space allocation themselves, special
//   constructors for collection templates are provided. The collection
//   templates allocate arrays of the collected objects from the heap.
//   If the collection is constructed by passing in a pointer to a
//   CollHeap object, then the virtual "allocateMemory" and "deallocateMemory"
//   methods of the CollectionHeap object is used to allocate the arrays.
//   Since that method is virtual, users can define their own classes,
//   derived from CollHeap, that override the generic implementation. Note
//   that collection classes do NOT have an overloaded "operator new".
//   Users can do that for their own classes that are derived from
//   collection classes(they will still have to use the constructor form
//   that passes in a CollHeap object). A final note: users who can use
//   standard allocation from the C runtime heap can ignore all this.
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// define a data type for list indexes(the value 0 can be
// used to access the first element in an array or list)
// -----------------------------------------------------------------------
// this definition moved to BaseTypes.h:
//		typedef UInt32 CollIndex;

// -----------------------------------------------------------------------
// some constants for internal use of values of CollIndex
// -----------------------------------------------------------------------
const CollIndex FIRST_COLL_INDEX      =         0; // first element
const CollIndex NULL_COLL_INDEX       = 111111111; // pointer to nothing
const CollIndex UNUSED_COLL_ENTRY     = 111111112; // unused array entry

// -----------------------------------------------------------------------
// Limit the growth of a Collections class to MAX_COLL_INDEX elements.
// The bound provided by LONG_MAX ensures that an element can be
// indexed using a vraible of type long. This limit is imposed to
// prevent the proliferation of variables of type unsigned in the
// application code that uses these Collections classes. The C run
// time environments typically interpret a negative value that is
// assigned to an unsigned variable as an unsigned value.
// -----------------------------------------------------------------------
const NAUnsigned MAX_COLL_INDEX        = MINOF(100000000,LONG_MAX);

// -----------------------------------------------------------------------
// defines for collection types(avoids hard-coding the template
// syntax in other modules)
// -----------------------------------------------------------------------
#define SET(Type)      NASet<Type>
#pragma warning (disable : 4005)  //warning elimination
#pragma nowarn(140)   // warning elimination
#define LIST(Type)     NAList<Type>
#pragma warn(140)  // warning elimination
#pragma warning (default : 4005)  //warning elimination
#define ARRAY(Type)    NAArray<Type>
#define SUBARRAY(Type) NASubArray<Type>
#define HASHDICTIONARY(Key,Value) NAHashDictionary<Key,Value>

#define CollIndexList  LIST(CollIndex)

// -----------------------------------------------------------------------
// The generic collection template, base class of other collection types
// -----------------------------------------------------------------------
template <class T> class NACollection : public NABasicObject

{

friend class NASubCollection<T>;

public:

  // return the number of entries in the collection
  inline CollIndex entries() const { return entries_; }

  inline NABoolean isEmpty() const { return(entries_ == 0); }

  // return the allocated size
  inline CollIndex getSize() const { return maxLength_; }

  inline CollIndex getUsedLength() const {return usedLength_; }

// The c89 compiler doesn't recognize the friend declaration above.
// For now, make all methods public when using c89.

#if !defined( NA_MSVC ) && !defined(NA_NO_FRIENDS_WITH_TEMPLATE_REFS)

protected:                                                // NT_PORT SK 04/08/97
#endif // ! NA_MSVC && ! NA_NO_FRIENDS_WITH_TEMPLATE_REFS

  // default constructor(empty collection)
  NACollection(CollIndex initLen = 0) : heap_(NULL)
  { allocate(initLen); }

  // constructor for a collection with user-defined heap management
  NACollection(CollHeap *heap, CollIndex initLen = 0) : heap_(heap)
  { allocate(initLen); }

  // copy ctor
  NACollection(const NACollection<T> &other, CollHeap *heap=0)
       : heap_( (heap==NULL) ? other.heap_ : heap )
  { copy(other); }

  // virtual destructor
  virtual ~NACollection();

  // copy another collection into this one
  // NOTE: this method is called by a constructor and it
  // assumes that the collection is in the deallocated state!!!
  void copy(const NACollection<T> &other) ;

  // assignment operator (deep copy instead of shallow one)
  inline NACollection<T> & operator =(const NACollection<T> &other)
  {
    if ( this != &other) { deallocate(); copy(other); } // avoid copy-self!
    return *this; 
  }

   // delete entries from <entry> to usedLength_ in the collection.
  void clearFrom( CollIndex entry );

  // return entry ix(create entry, if it doesn't exist already)
  T & rawEntry(CollIndex ix);

  // overloaded operator [] to access elements of the collection
 T & usedEntry(CollIndex ix)
  {
#if defined(_DEBUG)
    if((ix >= usedLength_) OR
       (usages_[ix] == UNUSED_COLL_ENTRY))
      ABORT("referencing an unused element of a collection");
#endif
    return arr_[ix];
  }

  // the const version of usedEntry
 const T & constEntry(CollIndex ix) const 
  {
#if defined(_DEBUG)
    if((ix >= usedLength_) OR
       (usages_[ix] == UNUSED_COLL_ENTRY)) 
      {
	ABORT("referencing an unused element of a collection");
      }
#endif
    return arr_[ix];
  }
  
  // get the usage info for an entry
  inline CollIndex getUsage(CollIndex pos) const
  {
    if(pos < usedLength_)
      return usages_[pos];
    else
      return UNUSED_COLL_ENTRY;
  }

  // find a particular usage in the array and return its position or
  // NULL_COLL_INDEX
  CollIndex findUsage(const CollIndex &toFind);

  // return byte size of this collection
  Lng32 getByteSize() const
    { return sizeof(*this) + (maxLength_ * (sizeof(T)+sizeof(CollIndex))); }

  // Resize the arrays to a new size(return new size)
  CollIndex resize(CollIndex newSize);  

  inline void setUsage(CollIndex pos, CollIndex newUsage)
  {
    if ((pos < usedLength_) AND 
       (usages_[pos] != UNUSED_COLL_ENTRY) AND
       (newUsage != UNUSED_COLL_ENTRY))
     usages_[pos] = newUsage;
  }


  // allocate the arrays(used in constructor, don't assume
  // that the data members except "heap_" are initialized yet)
  void allocate(CollIndex initLen);

  //inline void deallocate()
  virtual void deallocate()
  {
    // NOTE: dirty deallocate, no cleanup!!!
    if ( heap_ == NABasicObject::systemHeapPtr() )
    {
      // default: use global ::operator delete()
      if (arr_ != NULL) delete [] arr_;
      if (usages_ != NULL) delete [] usages_;
    }
    else
    {
      // user-specified delete operator
      // Note : this won't call the destructor of each arr_ element. 
      // after the compiler supports delete[] operator to be supprted,
      // it should be changed to
      // if (!arr_) delete [] arr_;
      // if (!usages_) delete [] usages_;

      if (arr_ != NULL) heap_->deallocateMemory(arr_);
      if (usages_ != NULL) heap_->deallocateMemory(usages_);
    }
    arr_ = NULL;
    usages_ = NULL;
  }

  inline void clear()
  {
    for (CollIndex i = FIRST_COLL_INDEX; i < usedLength_; i++)
      usages_[i] = UNUSED_COLL_ENTRY;
  
    entries_ = 0;
    usedLength_ = 0;
  }

  inline CollIndex freePos() const
  {
    // is there space in the unused but allocated portion?
    if (usedLength_ < maxLength_)
      return usedLength_;
  
    // now search the allocated portion for free entries
    for (CollIndex i = FIRST_COLL_INDEX; i < usedLength_; i++)
    {
      if (usages_[i] == UNUSED_COLL_ENTRY)
	    return i;
    }
  
    // no free positions in the allocated part
    return maxLength_;
  }

  inline void remove(CollIndex posToDelete)
  {
    if ((posToDelete < usedLength_) &&
      (usages_[posToDelete] != UNUSED_COLL_ENTRY))
    {
      // set the usage field to indicate an unused entry
      usages_[posToDelete] = UNUSED_COLL_ENTRY;
      entries_--;
      
      // change the usedLength_, if we deleted the last entry
      if (posToDelete == usedLength_ - 1)
	  {
	    while (usedLength_ > 0 AND
		   usages_[usedLength_ - 1] == UNUSED_COLL_ENTRY)
	    usedLength_--;
	  
	    // resize, if we gained back a substantial part of the array
	    if (maxLength_ > 10 AND
	       usedLength_ < maxLength_/4)
	    resize(usedLength_);
	  }
    }
    return;
  }

  void insert(CollIndex posToInsert,
	      const T   &newElement,
	      CollIndex newUsage = NULL_COLL_INDEX);

  inline CollIndex find(const T &toFind) const
  {
    for (CollIndex i = FIRST_COLL_INDEX; i < usedLength_; i++)
    {
      if ((usages_[i] != UNUSED_COLL_ENTRY) &&
	    (arr_[i] == toFind))
	  return i;
    }
  
    // return a "not found" indicator
    return NULL_COLL_INDEX;
  }

  inline void setHeap(CollHeap *heap)
   {
     heap_ = heap;
   }

private:

  CollIndex maxLength_;  // how many array entries are allocated?
  CollIndex usedLength_; // how many entries of adm_ are initialized?
  CollIndex entries_;    // caches number of entries
  T         *arr_;       // array for the members of the collection
  CollIndex *usages_;    // array for administrative information
  CollHeap  *heap_;      // user-defined heap object or NULL

}; // NACollection

// ***********************************************************************
// NASubCollection: A subset of a NACollection
//
// A NASubCollection is an array of bits. The array contains 512 bits
// initially but it grows from and shrinks to this inital allocation 
// on demand. Each bit is addressed using its bit position as  
// illustrated in the figure below:
//
// |...............|...............|...............|...............|......
// 0               32              64              96              128
//
// The array is implemented in units of 32 bit "words". Bits 0 through
// 31 are in word 0, bits 32 through 63 are in word 1, and so on.
//
// The ON/OFF state of each bit indicates the presence/absence 
// respectively of corresponding elements in the NACollection.
//
// An NASubCollection object with no superset can also be used as a
// bit vector.
// 
// ***********************************************************************
const Int32 BuiltinSubsetWords = 8; // size of the built-in array

typedef uint64_t DblWordAsBits;
typedef ULng32 WordAsBits;
// typedef change will affect clearFast, nextUsedFast and addElementFast. 

const UInt32 BitsPerWord = 32;
const UInt32 LogBitsPerWord = 5; // 0...31 can be expressed in 5 bits
const UInt32 LogBytesPerWord = 2;
const WordAsBits BitPortion = 0x1F;    // the 5 least significant bits
const WordAsBits AllBitsSet = 0xFFFFFFFF;
const WordAsBits SingleBitArray[BitsPerWord] = {
  0x80000000, 0x40000000, 0x20000000, 0x10000000,
  0x08000000, 0x04000000, 0x02000000, 0x01000000,
  0x00800000, 0x00400000, 0x00200000, 0x00100000,
  0x00080000, 0x00040000, 0x00020000, 0x00010000,
  0x00008000, 0x00004000, 0x00002000, 0x00001000,
  0x00000800, 0x00000400, 0x00000200, 0x00000100,
  0x00000080, 0x00000040, 0x00000020, 0x00000010,
  0x00000008, 0x00000004, 0x00000002, 0x00000001
};

const Lng32 bitsSet[] = {
  0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
  4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8 
};

// ***********************************************************************
//
// firstOne
//
// Returns the ordinal position of the first bit set in the unsigned long
// passed as an argument.
//
// NOTE: It is assumed (for performance reasons) that there is at least
//       one bit set in the argument. 
// 
// ***********************************************************************

inline Lng32 firstOne( ULng32 x )
{
  #define FIRSTHALFWORDBITS            0xFFFF0000
  #define FIRSTQUARTERWORDBITS         0xFF000000
  #define FIRSTEIGHTHWORDBITS          0xF0000000
  #define FIRSTSIXTEENTHWORDBITS       0xC0000000
  #define THIRDSIXTEENTHWORDBITS       0x0C000000
  #define THIRDEIGHTHWORDBITS          0x00F00000
  #define FIFTHSIXTEENTHWORDBITS       0x00C00000
  #define SEVENTHSIXTEENTHWORDBITS     0x000C0000
  #define THIRDQUARTERWORDBITS         0x0000FF00
  #define FIFTHEIGHTHWORDBITS          0x0000F000
  #define NINTHSIXTEENTHWORDBITS       0x0000C000
  #define ELEVENTHSIXTEENTHWORDBITS    0x00000C00
  #define SEVENTHEIGHTHWORDBITS        0x000000F0
  #define THIRTEENTHSIXTEENTHWORDBITS  0x000000C0
  #define FIFTEENTHSIXTEENTHWORDBITS   0x0000000C

  #define BIT31                        0x80000000
  #define BIT29                        0x20000000
  #define BIT27                        0x08000000
  #define BIT25                        0x02000000
  #define BIT23                        0x00800000
  #define BIT21                        0x00200000
  #define BIT19                        0x00080000
  #define BIT17                        0x00020000
  #define BIT15                        0x00008000
  #define BIT13                        0x00002000
  #define BIT11                        0x00000800
  #define BIT9                         0x00000200
  #define BIT7                         0x00000080
  #define BIT5                         0x00000020
  #define BIT3                         0x00000008
  #define BIT1                         0x00000002


  if (x & FIRSTHALFWORDBITS)
    {
      if (x & FIRSTQUARTERWORDBITS)
        {
          if (x & FIRSTEIGHTHWORDBITS)
            {
              if (x & FIRSTSIXTEENTHWORDBITS)
                {
                  if (x & BIT31)
                    return( 0 );
                  else        // Bit 30.
                    return( 1 );
                }
              else  //  Second sixteenth bits.
                {
                  if (x & BIT29)
                    return( 2 );
                  else        // Bit 28.
                    return( 3 );
                }
            }
          else  // Second eighth bits.
            {
              if (x & THIRDSIXTEENTHWORDBITS)
                {
                  if (x & BIT27)
                    return( 4 );
                  else        // Bit 26.
                    return( 5 );
                }
              else  //  Fourth sixteenth bits.
                {
                  if (x & BIT25)
                    return( 6 );
                  else        // Bit 24.
                    return( 7 );
                }
            }
        }
      else  // Second quarter.
        {
          if (x & THIRDEIGHTHWORDBITS)
            {
              if (x & FIFTHSIXTEENTHWORDBITS)
                {
                  if (x & BIT23)
                    return( 8 );
                  else        // Bit 22
                    return( 9 );
                }
              else  //  Second sixteenth bits.
                {
                  if (x & BIT21)
                    return( 10 );
                  else        // Bit 20.
                    return( 11 );
                }
            }
          else  // Fourth eighth bits.
            {
              if (x & SEVENTHSIXTEENTHWORDBITS)
                {
                  if (x & BIT19)
                    return( 12 );
                  else        // Bit 18.
                    return( 13 );
                }
              else  //  Eighth sixteenth bits.
                {
                  if (x & BIT17)
                    return( 14 );
                  else        // Bit 16.
                    return( 15 );
                }
            }
        }
    }
  else   // Second half
    {
      if (x & THIRDQUARTERWORDBITS)
        {
          if (x & FIFTHEIGHTHWORDBITS)
            {
              if (x & NINTHSIXTEENTHWORDBITS)
                {
                  if (x & BIT15)
                    return( 16 );
                  else        // Bit 14.
                    return( 17 );
                }
              else  //  Tenth sixteenth bits.
                {
                  if (x & BIT13)
                    return( 18 );
                  else        // Bit 12.
                    return( 19 );
                }
            }
          else  // Sixth eighth bits.
            {
              if (x & ELEVENTHSIXTEENTHWORDBITS)
                {
                  if (x & BIT11)
                    return( 20 );
                  else        // Bit 10.
                    return( 21 );
                }
              else  //  Twelveth sixteenth bits.
                {
                  if (x & BIT9)
                    return( 22 );
                  else        // Bit 8.
                    return( 23 );
                }
            }
        }
      else  // Fourth quarter.
        {
          if (x & SEVENTHEIGHTHWORDBITS)
            {
              if (x & THIRTEENTHSIXTEENTHWORDBITS)
                {
                  if (x & BIT7)
                    return( 24 );
                  else        // Bit 6.
                    return( 25 );
                }
              else  //  Fourteenth sixteenth bits.
                {
                  if (x & BIT5)
                    return( 26 );
                  else        // Bit 4.
                    return( 27 );
                }
            }
          else  // Eighth eighth bits.
            {
              if (x & FIFTEENTHSIXTEENTHWORDBITS)
                {
                  if (x & BIT3)
                    return( 28 );
                  else       // Bit 2.
                    return( 29 );
                }
              else  //  Sixteenth sixteenth bits.
                {
                  if (x & BIT1)
                    return( 30 );
                  else        // Bit 0.
                    return( 31 );
                }
            }
        }
    }                           
                     
}

// ***********************************************************************
//
// lastOne
//
// Returns the ordinal position of the last bit set in the unsigned long
// passed as an argument.
//
// NOTE: It is assumed (for performance reasons) that there is at least
//       one bit set in the argument. 
//
// NOTE2: If this routine ever gets extensively used, replace with call
//        to compiler intrinsic _m64_popcnt - much *MUCH* faster.
// 
// ***********************************************************************
inline Lng32 lastOne( ULng32 x )
{
  #define LASTHALFWORDBITS             0x0000FFFF
  #define LASTQUARTERWORDBITS          0x000000FF
  #define LASTEIGHTHWORDBITS           0x0000000F
  #define LASTSIXTEENTHWORDBITS        0x00000003
  #define FOURTEENTHSIXTEENTHWORDBITS  0x00000030
  #define SIXTHEIGHTHWORDBITS          0x00000F00
  #define TWELVETHSIXTEENTHWORDBITS    0x00000300
  #define TENTHSIXTEENTHWORDBITS       0x00003000
  #define SECONDQUARTERWORDBITS        0x00FF0000
  #define FOURTHEIGHTHWORDBITS         0x000F0000
  #define EIGHTHSIXTEENTHWORDBITS      0x00030000
  #define SIXTHSIXTEENTHWORDBITS       0x00300000
  #define SECONDEIGHTHWORDBITS         0x0F000000
  #define FOURTHSIXTEENTHWORDBITS      0x03000000
  #define SECONDSIXTEENTHWORDBITS      0x30000000

  #define BIT30                        0x40000000
  #define BIT28                        0x10000000
  #define BIT26                        0x04000000
  #define BIT24                        0x01000000
  #define BIT22                        0x00400000
  #define BIT20                        0x00100000
  #define BIT18                        0x00040000
  #define BIT16                        0x00010000
  #define BIT14                        0x00004000
  #define BIT12                        0x00001000
  #define BIT10                        0x00000400
  #define BIT8                         0x00000100
  #define BIT6                         0x00000040
  #define BIT4                         0x00000010
  #define BIT2                         0x00000004
  #define BIT0                         0x00000001

  if (x & LASTHALFWORDBITS)
    {
      if (x & LASTQUARTERWORDBITS)
        {
          if (x & LASTEIGHTHWORDBITS)
            {
              if (x & LASTSIXTEENTHWORDBITS)
                {
                  if (x & BIT0)
                    return( 0 );
                  else
                    return( 1 );  // Bit 1.
                }
              else  //  Fifteenth sixteenth bits.
                {
                  if (x & BIT2)
                    return( 2 );
                  else        // Bit 3.
                    return( 3 );
                }
            }
          else  // Seventh eighth bits.
            {
              if (x & FOURTEENTHSIXTEENTHWORDBITS)
                {
                  if (x & BIT4)
                    return( 4 );
                  else
                    return( 5 );  // Bit 5.
                }
              else  //  Thirteenth sixteenth bits.
                {
                  if (x & BIT6)
                    return( 6 );
                  else        // Bit 7.
                    return( 7 );
                }
            }
        }
      else  // Third quarter.
        {
          if (x & SIXTHEIGHTHWORDBITS)
            {
              if (x & TWELVETHSIXTEENTHWORDBITS)
                {
                  if (x & BIT8)
                    return( 8 );
                  else        // Bit 9
                    return( 9 );
                }
              else  //  Eleventh sixteenth bits.
                {
                  if (x & BIT10)
                    return( 10 );
                  else        // Bit 11.
                    return( 11 );
                }
            }
          else  // Fifth eighth bits.
            {
              if (x & TENTHSIXTEENTHWORDBITS)
                {
                  if (x & BIT12)
                    return( 12 );
                  else        // Bit 13.
                    return( 13 );
                }
              else  //  Eighth sixteenth bits.
                {
                  if (x & BIT14)
                    return( 14 );
                  else        // Bit 15.
                    return( 15 );
                }
            }
        }
    }
  else   // First half
    {
      if (x & SECONDQUARTERWORDBITS)
        {
          if (x & FOURTHEIGHTHWORDBITS)
            {
              if (x & EIGHTHSIXTEENTHWORDBITS)
                {
                  if (x & BIT16)
                    return( 16 );
                  else        // Bit 17.
                    return( 17 );
                }
              else  //  Seventh sixteenth bits.
                {
                  if (x & BIT18)
                    return( 18 );
                  else        // Bit 19.
                    return( 19 );
                }
            }
          else  // Third eighth bits.
            {
              if (x & SIXTHSIXTEENTHWORDBITS)
                {
                  if (x & BIT20)
                    return( 20 );
                  else        // Bit 10.
                    return( 21 );
                }
              else  //  Fifth sixteenth bits.
                {
                  if (x & BIT22)
                    return( 22 );
                  else        // Bit 23.
                    return( 23 );
                }
            }
        }
      else  // First quarter.
        {
          if (x & SECONDEIGHTHWORDBITS)
            {
              if (x & FOURTHSIXTEENTHWORDBITS)
                {
                  if (x & BIT24)
                    return( 24 );
                  else        // Bit 25.
                    return( 25 );
                }
              else  //  Third sixteenth bits.
                {
                  if (x & BIT26)
                    return( 26 );
                  else        // Bit 27.
                    return( 27 );
                }
            }
          else  // First eighth bits.
            {
              if (x & SECONDSIXTEENTHWORDBITS)
                {
                  if (x & BIT28)
                    return( 28 );
                  else        // Bit 29.
                    return( 29 );
                }
              else  //  First sixteenth bits.
                {
                  if (x & BIT30)
                    return( 30 );
                  else        // Bit 31.
                    return( 31 );
                }
            }
        }
    }                           
                     
}

// ***********************************************************************
//
// ones
//
// Returns the number of bits set in the unsigned long passed as an
// argument.
// 
// ***********************************************************************
inline Lng32 ones( ULng32 x )
{
  unsigned char * px = (unsigned char *) &x;

  return( bitsSet[ px[ 0 ] ]
              +
          bitsSet[ px[ 1 ] ]
              +
          bitsSet[ px[ 2 ] ]
              +
          bitsSet[ px[ 3 ] ]
        );
}

template <class T> class NASubCollection : public NABasicObject
{
  // protected:  doesn't work for NT nor C89
public:

  // constructor
  NASubCollection(NACollection<T> *superset, CollHeap * heap=0);

  // copy ctor and destructor
  NASubCollection(const NASubCollection<T> &other, CollHeap * heap=0);

  virtual ~NASubCollection();

  // set heap after constructor has been called but before object is used
  void setHeap(CollHeap *heap);

  inline CollIndex resize( CollIndex newSize )
   {
   if (newSize > maxLength_)
      {
      CollIndex    i        = wordSize_ - 1;
      WordAsBits * newBits;
      WordAsBits * pBits    = &pBits_[ i ];

      maxLength_ = newSize << 1;

      if (heap_)
         newBits = new (heap_) WordAsBits[ maxLength_ ];
      else
         newBits = new WordAsBits[ maxLength_ ];

      newBits    = &newBits[ i ];

      do
         {
         *newBits-- = *pBits--;
         }
      while (--i);

      *newBits = *pBits;
      
      for (i = wordSize_; i < maxLength_; i++)
         newBits[ i ] = 0x0;

      if (!builtin_)
         deallocateBits();
      else
        builtin_ = FALSE;

      pBits_     = newBits;
      wordSize_  = newSize;
      }
   else
      {
      CollIndex i = wordSize_ - newSize;

      if ((Lng32) i > 0)
         {
         WordAsBits * pBits = &pBits_[ newSize ];

         do
            {
            *pBits++ = 0x0;
            }
         while (--i);
         }

      wordSize_ = newSize;

      if (wordSize_ == 0)
         entries_ = 0;
      }
    
    return( wordSize_ * BitsPerWord );
	   }

  CollIndex getWordSize   () const { return( wordSize_ ); }
  CollIndex getLastStaleBit() const { return( lastStaleBit_ ); }

  inline void extendWordSize( CollIndex minWordSize )
   {
   // resize the array
   resize( minWordSize );
   }

  inline void clear()
   {
   CollIndex i = wordSize_;

   if (i && entries_)
      {
      WordAsBits * pBits = pBits_;

      do
         {
         *pBits++ = 0x0;
         }
      while (--i);
      }

   entries_ = 0;
   lastStaleBit_ = 0;
   }

  // Fast clear if bit 1 entries are probably in the first few words.
  inline void clearFast();

  // quick check whether the collection is empty
  inline NABoolean isEmpty() const { return( entries_ == 0 ); }

  // word() performs a bounds check.
  inline WordAsBits & word( CollIndex i )
  { 
      assert(i < wordSize_);

      return( pBits_[ i ] );
  }

  // word() const performs a bounds check.
  inline WordAsBits word( CollIndex i ) const
  { 
      assert(i < wordSize_);

      return( pBits_[ i ] );
  }

  // ---------------------------------------------------------------------
  // wordNo() : Given a bit position, it returns an index to the  
  //            word in which the bit is contained 
  // For bits  0 through 31 returns 0
  //          32         63         1
  //          64         95         2  ...  and so on
  // It is equivalent to(b / 32), where b is the bit position.
  // ---------------------------------------------------------------------
  inline CollIndex wordNo(CollIndex x) const { return x >> LogBitsPerWord; }

  // ---------------------------------------------------------------------
  // bitNo() : Given a bit position, it returns the position of the 
  //           bit within a 32 bit word
  // It is equivalent to(b mod 32), where b is the bit position
  // ---------------------------------------------------------------------
  inline CollIndex bitNo(CollIndex x) const { return x LAND BitPortion; }

  // ---------------------------------------------------------------------
  // entries() : Returns the number of entries in the subcollection.
  // ---------------------------------------------------------------------
  inline CollIndex entries() const { return( entries_ ); }

  // ---------------------------------------------------------------------
  // testBit() : Given a bit position, check whether the bit is set
  //             in the given position in the NASubCollection.
  // ---------------------------------------------------------------------

  inline NABoolean testBit( CollIndex b ) const
   {
   CollIndex wn = wordNo( b );

   if (wn >= wordSize_)
      {
      // this bit surely isn't part of the subset (although we can't
      // return an error, since the superset may have grown beyond the
      // allocated limit of the subset)

      return( FALSE );
      }
   else
      {
      // NOTE: for performance reasons we return arbitrary numbers for TRUE
      // cast to NABoolean added for warning elimination
      return( (NABoolean) (pBits_[ wn ] LAND SingleBitArray[ bitNo( b ) ]) );  
      }
   }

  // ---------------------------------------------------------------------
  // access a particular element of the superset, specified by the index
  // ---------------------------------------------------------------------
  const T & element(CollIndex e) const
                  { assert(testBit(e)); return superset_->constEntry(e); }

  // ---------------------------------------------------------------------
  // subset() : Check whether another set is a subset of this one
  // ---------------------------------------------------------------------
  NABoolean contains(const NASubCollection<T> & other) const;

  // ---------------------------------------------------------------------
  // prevUsed() : Given a starting bit position, find the previous bit that
  //              is set.
  // Returns: TRUE  if a bit that is set is found; decrements start
  //          FALSE if all bits < start are reset; start is undefined
  //
  // e.g. in a for-loop:  for(CollIndex i = lastStaleBit; prevUsed(i); i--) ...
  // ---------------------------------------------------------------------
  inline CollIndex prevUsed(CollIndex start) const;
  CollIndex prevUsedSlow(CollIndex start) const;
  // Fast path when prevUsed is probably in the first two words.
  inline CollIndex prevUsedFast(CollIndex start) const;

  // ---------------------------------------------------------------------
  // nextUsed() : Given a starting bit position, find the next bit that
  //              is set.
  // Returns: TRUE  if a bit that is set is found; advances start
  //          FALSE if all bits > start are reset; start is undefined
  //
  // e.g. in a for-loop:  for(CollIndex i = 0; nextUsed(i); i++) ...
  // ---------------------------------------------------------------------

  inline NABoolean nextUsed( CollIndex & start ) const
   {
   CollIndex limit = wordSize_ << LogBitsPerWord;

   if (start >= limit || entries_ == 0)
      return( FALSE );
   else
      {
      CollIndex startOffset = bitNo( start );
      CollIndex firstDWord  = pBits_[ wordNo( start ) ]
                                         &
                              ((~0u) >> startOffset);

      if (firstDWord)
         {
         start = start + firstOne( firstDWord ) - startOffset;

         return( TRUE );
         }
      else
         {
         start                     = start + (BitsPerWord - startOffset);

         CollIndex dWordsRemaining = (limit - start) >> LogBitsPerWord;

         if (dWordsRemaining == 0)    
            return( FALSE );

         WordAsBits * pBits        = &pBits_[ wordNo( start ) - 1 ];

         do
            {
            pBits++;
            }
         while (--dWordsRemaining && !*pBits);

         start = ((((char *) pBits) - ((char *) pBits_)
                  ) >> LogBytesPerWord
                 ) << LogBitsPerWord;

         if (*pBits)
            {
            start = start + firstOne( *pBits );

            return( TRUE );
            }
         else
            {
            start = limit;

            return( FALSE );
            }
         }
      }
   }

  // Fast path when nextUsed is probably in the first two words.
  inline NABoolean nextUsedFast(CollIndex &start) const;

  // ---------------------------------------------------------------------
  // lastUsed() : Find the last bit that is set.  
  // Returns: TRUE if a bit that is set is found; sets lastBit
  //          FALSE if no bits are set; no change to lastBit.
  //
  // e.g. in an if:
  //                    CollIndex lastBit = 0;
  //                    if (lastUsed(lastBit))
  //                      {
  //                        ... ok to make use of lastBit...
  //			  };
  // ---------------------------------------------------------------------
  NABoolean lastUsed(CollIndex &lastBit) const;

  NABoolean operator ==(const NASubCollection<T> & other) const;

  inline NASubCollection<T> & operator = (const NASubCollection<T> & other)
  {
  if (this != &other) // avoid copy-self!
    {
      CollIndex i;
      WordAsBits * pBits;
      WordAsBits * pOtherBits = other.pBits_;

      entries_  = other.entries_;
      superset_ = other.superset_;
      lastStaleBit_ = other.lastStaleBit_;

      if (other.wordSize_ > maxLength_)
         {
         maxLength_ = other.wordSize_;
         wordSize_  = maxLength_;

         if (!builtin_)
            deallocateBits();
        
         if (wordSize_ > BuiltinSubsetWords)
            {
            pBits_ = allocateBits( wordSize_ );
              
            builtin_ = FALSE;
            }
         else
            {
            pBits_   = (WordAsBits *)(&sbits_[ 0 ]);

            builtin_ = TRUE;
            }

         pBits = pBits_;

         i     = wordSize_;

         if (i)
            {
            do
               {
               *pBits++ = *pOtherBits++;
               }
            while (--i);
            }
         }
      else
         {
         i     = other.wordSize_;
         //If maxLength_ is greater than the other.wordSize_, it implies that
         //entries are on the builtin array. In this case we cannot assume that
         //pBits_ is pointing to its sbits_. Because structures like LIST(ValueIdSet)
         //cannot guarantee that each of its ValueIdSet is properly initialized.
         //In the future when we have the use of overloaded new[] we can change
         //NACollection::allocate to have each member properly initialized.
         //CR#10-011003-6224
         if(builtin_) //sometimes we come here for builtin_ false.
         pBits_ = (WordAsBits*)(&sbits_[ 0 ]);
         
         pBits = pBits_;

         if (i)
            {
            do
              {
              *pBits++ = *pOtherBits++;
              }
            while (--i);
            }
            
         i = wordSize_ - other.wordSize_;

         if ((Lng32) i > 0)
            {
            do
              {
              *pBits++ = 0x0;
              }
            while (--i);
            }

         wordSize_ = other.wordSize_;
         }
      }

   return( *this );
   }

  // ---------------------------------------------------------------------
  // Set-oriented operators on subcollections.
  // ---------------------------------------------------------------------
  NASubCollection<T> & addSet(const NASubCollection<T> & other);

  NASubCollection<T> & intersectSet(const NASubCollection<T> & other);
  NASubCollection<T> & subtractSet(const NASubCollection<T> & other);

  inline NASubCollection<T> & addElement( CollIndex elem )
  {
  CollIndex wordNumber = wordNo( elem );

  assert( superset_ == NULL
              OR
	  (superset_->getUsage( elem ) != UNUSED_COLL_ENTRY)
        );

  if ( wordNumber >= wordSize_ )
     {
     extendWordSize( wordNumber + 1 );

     pBits_[ wordNumber ] |= SingleBitArray[ bitNo( elem ) ];

     entries_++;
     }
  else
     {
     WordAsBits originalWord = pBits_[ wordNumber ];

     if (originalWord != (pBits_[ wordNumber ] |= SingleBitArray[ bitNo( elem ) ]))
        entries_++;
     }
   // Record the last bit ever set.
   if (elem > lastStaleBit_)
     lastStaleBit_ = elem;   
   return( *this );
   }

  // Fast path when elem is probably less than 64.  
  inline NASubCollection<T> & addElementFast(CollIndex elem);
  
  inline NASubCollection<T> & subtractElement( CollIndex elem )
   {
   CollIndex wordNumber = wordNo( elem );

   if (wordNumber < wordSize_)
      {
      WordAsBits originalWord = pBits_[ wordNumber ];

      if (originalWord != (pBits_[ wordNumber ] &= LNOT SingleBitArray[ bitNo( elem ) ]))
         entries_--;
      }

   return( *this );
   }

  NASubCollection<T> & complement();

  // ---------------------------------------------------------------------
  // Overloaded variants of some of the above.
  // ---------------------------------------------------------------------
  inline NASubCollection<T> & operator +=(const NASubCollection<T> & other)
  { return addSet(other); }
  inline NASubCollection<T> & operator -=(const NASubCollection<T> & other)
  { return subtractSet(other); }
  inline NASubCollection<T> & operator +=(CollIndex elem)
  { return addElement(elem); }
  inline NASubCollection<T> & operator -=(CollIndex elem)
  { return subtractElement(elem); }

  WordAsBits hash() const;

private:

  CollIndex  maxLength_;                     // Allocated size in dwords.
  CollIndex  wordSize_;                      // Dwords in use.
  Lng32       entries_;                       // Number of bits set (or -1 if don't know).
  NABoolean  builtin_;                       // TRUE if using builtin array.

  // sbits_ is defined as an array of double words instead of an array of
  // words so that we ensure it starts on an 8-byte boundary.  This is
  // important for routines like prevUsedFast() so that an unalignment trap
  // doesn't occur whenever we read an Int64 from the start of the array.
  // Also, there is no problem if we don't use the builtin words since any
  // dynamic allocation done will start the allocated buffer on a 16-byte
  // boundry

  DblWordAsBits sbits_[ BuiltinSubsetWords ];// Automatically allocated bits.

  WordAsBits *pBits_;                        // Points to the bit vector, 
                                             // wherever it may be.

  NACollection<WordAsBits> *longBits_;       // used if more bits are needed
  NACollection<T> *superset_;                // the superset
  CollHeap        *heap_;                    // heap used for longBits_

  CollIndex lastStaleBit_;                   // rightmost bit ever set to 1.
                                             // Any method setting bit 1 must
                                             // remember to maintain this var.
                                             // initialized to 0.

  // allocate and deallocate the pBits_ vector
  inline WordAsBits * allocateBits( CollIndex words )
  {
    return( heap_ ?
              (new( heap_ ) WordAsBits[ words ])
                 :
              (new WordAsBits[ words ])
          );
  }
  inline void deallocateBits()
  {
    if (heap_)
      heap_->deallocateMemory( pBits_ );
    else
      delete [] pBits_;
  }

}; // NASubCollection

// Fast path if bit 1 entries are probably in the first few words.
// If it is not the case, the regular clear will be called.
// Assume WordAsBits is 32 bit unsigned integer.
// Note it is not required to call clearFast, nextUsedFast, and 
// addElementFast together.
template <class T>
inline void NASubCollection<T>::clearFast()
{
  if (lastStaleBit_ >= 128)
    { // call the regular clear.
      clear();
      return;
    }

  *pBits_ = 0;  // clear 1st word

  entries_ = 0; 
  if (lastStaleBit_ < 32)
    {
      lastStaleBit_ = 0;
      return;
    }
  
  WordAsBits *pW = pBits_;
  *++pW = 0;    // clear 2nd word
  if (lastStaleBit_ < 64)
    {
      lastStaleBit_ = 0;
      return;
    }
  *++pW = 0;    // clear 3rd word
  if (lastStaleBit_ < 96)
    {
      lastStaleBit_ = 0;
      return;
    }
  *++pW = 0;    // clear 4th word
  lastStaleBit_ = 0;
  return;
}

// Fast path when prev used is probably in the first 2 words.
template <class T>
inline CollIndex NASubCollection<T>::prevUsed(CollIndex start) const
{
  if (start > lastStaleBit_)
    return NULL_COLL_INDEX;

  if (start >= 64)
    return prevUsedSlow(start);

  return prevUsedFast(start);
}

// Returns the ordinal position of the last bit set in the uint64_t passed as
// an argument.
inline ULng32 FindLastOne(uint64_t x)
{
  // Set bits right of, and clear bits left of, last bit set.
  uint64_t y = x ^ (x - 1);

  // Return count of all bits set in the computed value.  Fast version
  // implemented for each targetted OS.
  ULng32 result;
  ULng32* ptr = ((ULng32*)&y);
  ULng32 z;

  // Quickly count the number of set bits in the first word using a well known
  // population count algorithm
  z = ptr[0];
  z -= ((z >> 1) & 0x55555555);
  z = (((z >> 2) & 0x33333333) + (z & 0x33333333));
  z = (((z >> 4) + z) & 0x0f0f0f0f);
  z += (z >> 8);
  z += (z >> 16);

  result = (z & 0x0000003f);

  // Quickly count the number of set bits in the second word using a well known
  // population count algorithm
  z = ptr[1];
  z -= ((z >> 1) & 0x55555555);
  z = (((z >> 2) & 0x33333333) + (z & 0x33333333));
  z = (((z >> 4) + z) & 0x0f0f0f0f);
  z += (z >> 8);
  z += (z >> 16);

  return (result + (z & 0x0000003f));
}

// Slower path for calculating previous set bit.  We can make this routine
// twice as fast if we access 64-bits at a time - no need to implement now.
template <class T>
CollIndex NASubCollection<T>::prevUsedSlow(CollIndex start) const
{
  Lng32 startWordBucket;
  ULng32 startWord;
  ULng32 shiftAmount;

  startWordBucket = (Lng32)(start >> 5);  // word bucket to start search in
  shiftAmount = 32 - (start & 0x1f) - 1; // used to calculate prev bit position
  startWord = pBits_[startWordBucket] >> shiftAmount; // portion of bit vector
                                                      // with start bit as the
                                                      // least significant bit.
  // If start bit is set, return it
  if (startWord & 1)
    return start;

  // Search all words from startWord down to the 0th word in the bit vector in
  // search for a set bit.
  do {
    if (startWord) {
      // First calculation the ordinal position of the 1st set bit in the
      // next word.  Then subtract off the shiftAmount and the position of the
      // last set bit in the word.  This will give the ordinal position of the
      // last bit set.
      return ((startWordBucket+1) << 5) - shiftAmount - FindLastOne(startWord);
    }

    // Move on down to the next word bucket.
    startWordBucket--;

    // Set shiftAmount to 0 since start bit will now already be in the least
    // significant bit position.
    shiftAmount = 0;

    // Get the next word to process *if* we're not passed the edge.
    if (startWordBucket >= 0)
      startWord = pBits_[startWordBucket];
  } while (startWordBucket >= 0);

  // If we haven't returned already, we must not have found a prev set bit.
  return NULL_COLL_INDEX;
}

// Fast path when prev used is probably in the first 2 words.
template <class T>
inline CollIndex NASubCollection<T>::prevUsedFast(CollIndex start) const
{
  uint64_t startDoubleWord;
  ULng32 shiftAmount;

  // Amount to shift start bit to least significant bit position.
  shiftAmount = 64 - start - 1;

  // Get portion of bit vector with start bit in least signifcant bit position.
  // On NT, we have to byte-swap 64-bit value to big-endian.

  startDoubleWord =
    (((uint64_t)(*pBits_) << 32) | (uint64_t)(*(pBits_+ 1))) >> shiftAmount;

  // If double word is 0, no bits are set, so return.
  if (!startDoubleWord)
    return NULL_COLL_INDEX;

  // Return start bit if it is set.  Otherwise calculate ordinal position of
  // last bit set and return it.
  return ((startDoubleWord & 1)
          ? start
          : 64 - shiftAmount - FindLastOne(startDoubleWord));
}

const WordAsBits FirstOneLookup[16] = {
  4, 3, 2, 2,    //  0,  1,  2,  3
  1, 1, 1, 1,    //  4,  5,  6,  7
  0, 0, 0, 0,    //  8,  9, 10, 11
  0, 0, 0, 0     // 12, 13, 14, 15
};

// Fast path when next used is probably in the first two words.
// If it is not the case, the regular nextUsed will be called.
//
// TRUE if found next used and update start to the nextUsed position.
// FALSE otherwise and start value is undefined.
// Assume WordAsBits is 32 bit unsigned integer.
// For each 4 bits, do a lookup.
// Use lastStaleBit_ to limit the search.
template <class T>
inline NABoolean NASubCollection<T>::nextUsedFast(CollIndex &start) const
{
  if (start > lastStaleBit_)  // no bit 1 entry after lastStaleBit_.
    return FALSE;
  if (start >= 64)
    return nextUsed(start);   // call the regular nextUsed.

  WordAsBits w;    
  if (start >= 32)
    // truncate bits before starting bit in word 1.
    w = (pBits_[1] << (start - 32)); 
  else
    { // truncate bits before starting bit in word 0.
      w = ((*pBits_) << start); 
      if (!w)
        { 
	  if (lastStaleBit_ < 32)
	    return FALSE;     // lastStaleBit_ is in word 0.
	  // check next word.
          w = pBits_[1];
          start = 32;
        }
    }
  
  if (w >> 31)
    return TRUE;    // starting bit is set
  
  ULng32 j, k;
  // Do a lookup for each 4 bits.
  // first 8 bits.
  if (j = (w >> 24))
    { 
      if (k = (j >> 4))
	start += FirstOneLookup[k];                      // bit 0..3
      else
	start += (4 + FirstOneLookup[j & 0xf]);          // bit 4..7 
      return TRUE;
    }
  // 2nd 8 bits.
  if (j = (w & 0xff0000))
    {
      if (k = (j >> 20))                                 // bit 8..11
	start += (8 + FirstOneLookup[k]);     
      else
	start += (12 + FirstOneLookup[(j >> 16) & 0xf]); // bit 12..15 
      return TRUE;
    }
  // 3rd 8 bits   
  if (j = (w & 0xff00))
    { 
      if (k = (j >> 12))                                 // bit 16..19
	start += (16 + FirstOneLookup[k]);     
      else
	start += (20 + FirstOneLookup[(j >> 8) & 0xf]);  // bit 20..23 
      return TRUE;
    }    
  // 4th 8 bits   
  if (j = (w & 0xff))
    { 
      if (k = (j >> 4))                                  // bit 24..27
	start += (24 + FirstOneLookup[k]);     
      else
	start += (28 + FirstOneLookup[j & 0xf]);         // bit 28..31 
      return TRUE;
    } 
  // Not found in first two words.
  if (lastStaleBit_ < 64) 
    return FALSE;       // lastStaleBit_ is in the first two words.
  // Start from bit 64 to look for next used.
  start = 64;
  return nextUsed(start);  
}

// Fast path when elem is probably less than 64.  
// If it is not the case, the regular addElement will be called.
template <class T>
inline NASubCollection<T> & NASubCollection<T>::addElementFast(CollIndex elem) 
{
  if (elem >= 64)
    return addElement(elem);
  WordAsBits *w = pBits_;
  CollIndex j = elem;
  if (j >= 32)
    { 
      w = &pBits_[1];
      j -= 32;
    }

  if (((*w) << j) >> 31)
    return (*this);   // already set.

  if (elem > lastStaleBit_)
    lastStaleBit_ = elem;
  (*w) |= SingleBitArray[j]; 

  entries_++;
  return (*this);  
}

// -----------------------------------------------------------------------
// A set type template
//(NOTE: this is implemented as an array where used entries have usage
// NULL_COLL_INDEX and free entries have UNUSED_COLL_ENTRY usage)
// -----------------------------------------------------------------------
template <class T> class NASet : public NACollection<T>
{

public:

  // constructor with user-defined heap
  NASet(CollHeap  *heap,
        CollIndex initSize = 0) : NACollection<T>(heap,initSize)
  { invalidateCache(); }

  // copy ctor
  NASet(const SET(T) &other, CollHeap * heap) : NACollection<T>(other, heap)
  { invalidateCache(); }

  // virtual destructor
  virtual ~NASet();

  // assignment operator (deep copy instead of shallow one)
  inline NASet<T> & operator =(const NASet<T> &other)
  {     
      if ( this != &other) { this->deallocate(); NACollection<T>::copy( (const NACollection<T>&) other); } // avoid copy-self!
    
    return *this; 
  }

  // remove all entries from the set
  inline void clear() { NACollection<T>::clear(); invalidateCache(); }

  // check whether an element is in the collection
  inline NABoolean contains(const T &elem) const
	{ return(NACollection<T>::find(elem) != NULL_COLL_INDEX); }

  // find a given element in the collection and return it
  inline NABoolean find(const T &elem, T &returnedElem) const
  {
    CollIndex ix = NACollection<T>::find(elem);

    if(ix != NULL_COLL_INDEX)
      {
	returnedElem = this->constEntry(ix);
	return TRUE;
      }
    else
      return FALSE;
  }

  // index access(both reference and value) to be used as an iterator
  // over the set elements, not for determining any order of the set!!!
  T & operator [](CollIndex i);
  const T & operator [](CollIndex i) const;
  NABoolean  operator==(const NASet<T> &other) const;

  inline T & at(CollIndex i) { return operator [](i); }
  inline const T & at(CollIndex i) const { return operator [](i); }

  // return byte size of this collection
  Lng32 getByteSize() const
    { return NACollection<T>::getByteSize() + sizeof(*this) - 
        sizeof(NACollection<T>); }

  inline NABoolean insert(const T &elem)
  {
    invalidateCache();
    if (NACollection<T>::find(elem) == NULL_COLL_INDEX)
    {
      NACollection<T>::insert(this->freePos(),elem);
      return TRUE;
    }
    else
    return FALSE;
  }

  // A dumb but easy implementation for insert one SET into another
  inline NABoolean insert(const NASet<T> &other)
  {
    CollIndex count = other.entries();
    for (CollIndex i = 0; i < count; i++)
    {
      insert(other[i]);
    } // for loop for iterating over members of the set
    return TRUE;
  } // insert(SET(T))

  inline NABoolean remove(const T &elem)
  {
    CollIndex ix = NACollection<T>::find(elem);

    invalidateCache();
    if (ix != NULL_COLL_INDEX)
    {
      NACollection<T>::remove(ix);
      return TRUE;
    }
    else
      return FALSE;
  }

  // A dumb but easy implementation for remove one SET from  another
  inline NABoolean remove(const NASet<T> &other)
  {
    CollIndex count = this->entries();
    for (CollIndex i = 0; i < count; i++)
    {
      if ( contains(other[i]) )
         remove(other[i]);
      // else
      //   raise an exception?	
    } // for loop for iterating over members of the set
    return TRUE;
  } // removeSET(T))

private:

  inline void invalidateCache()
  { userIndexCache_ = arrayIndexCache_ = NULL_COLL_INDEX; }

  CollIndex userIndexCache_;     // cache of last accessed element # with []
  CollIndex arrayIndexCache_;    // where to find the last accessed element
}; // NASet

// -----------------------------------------------------------------------
// a list type template
// -----------------------------------------------------------------------
template <class T> class NAList : public NACollection<T> 
{

public:

  // constructor with user-defined heap
  NAList(CollHeap * heap,
         CollIndex initLen = 0) : NACollection<T>(heap,initLen) 
  { first_ = last_ = userIndexCache_ = arrayIndexCache_ = NULL_COLL_INDEX; }

  // copy ctor
  NAList(const NAList<T> &other, CollHeap * heap) : NACollection<T>(other, heap)
  {
    first_ = other.first_;
    last_ = other.last_;
    userIndexCache_ = other.userIndexCache_;
    arrayIndexCache_ = other.arrayIndexCache_;
  }

  // virtual destructor
  virtual ~NAList();

  // assignment
  
  NAList<T> & operator =(const NAList<T> &other);

  // comparison
  NABoolean operator ==(const NAList<T> &other) const;

  // insert a new entry
  inline void insert(const T &elem) { insertAt(this->entries(),elem); }

  // insert a set, array, or list
  // void insert(const SET(T) &other);

  void insert(const LIST(T) &other);

  // remove an element(the first that matches) that is given by its value
  //(returns whether the element was found and removed)
  inline NABoolean remove(const T &elem)
  { return removeCounted(elem, 1) > 0; }

  // remove elements(all that match) given by value
  //(returns the number of elements removed)
  inline CollIndex removeAll(const T &elem)
  { return removeCounted(elem, 0); }

  // remove at most "desiredCount" matching elements
  //(returns the number of elements removed)

  CollIndex removeCounted(const T &elem, const CollIndex desiredCount);

  // remove all entries from the list
  inline void clear()
  {
    NACollection<T>::clear();
    first_ = last_ = userIndexCache_ = arrayIndexCache_ = NULL_COLL_INDEX;
  }

  // check whether an element is in the collection
  inline NABoolean contains(const T &elem) const
  { return(NACollection<T>::find(elem) != NULL_COLL_INDEX); }

  // find a given element in the collection and return it
  NABoolean find(const T &elem, T &returnedElem) const;

  // find the index of a given element or return NULL_COLL_INDEX if not found
  inline CollIndex index (const T &elem) const
  {
    CollIndex currEntry = first_;
    CollIndex result    = FIRST_COLL_INDEX;

    while (currEntry != NULL_COLL_INDEX)
    {
      if (this->constEntry(currEntry) == elem)
	    return result;
      else
	  {
	    // advance to the next entry
	    result++;
	    currEntry = this->getUsage(currEntry);
	  }
    }

    // not found
    return NULL_COLL_INDEX;
  }

  // index access(both reference and value), zero based
  T & operator [](CollIndex i);
  const T & operator [](CollIndex i) const;

  inline T & at(CollIndex i) { return operator [](i); }
  const T & at(CollIndex i) const { return operator [](i); }

  // for the RAlist
  CollIndex findArraysIndex(CollIndex i);
  CollIndex findArraysIndex(CollIndex i) const;

  // remove the last entry from the list and store it in "elem"
  //(returns FALSE if the list is empty and no value is returned)
  NABoolean getLast(T &elem);

  // return byte size of this collection
  Lng32 getByteSize() const
    { return NACollection<T>::getByteSize() + sizeof(*this) - 
        sizeof(NACollection<T>); }

  // remove the index'th element from the list
  //(returns TRUE if list[index] was found, FALSE if index was out of bounds)
  inline NABoolean removeAt(const CollIndex index)
  {
    // check whether index is legal
    if (index >= this->entries())
      return FALSE;

    if (index == 0)
    {
      // special case of removing the first list element
      CollIndex elemToRemove = first_;
      first_ = this->getUsage(elemToRemove);
      NACollection<T>::remove(elemToRemove);
      if (elemToRemove == last_)
	    last_ = first_;
    }
    else
    {
      // second or later entry
      CollIndex pred = first_;
      CollIndex predIx = 0;
      CollIndex elemToRemove;

      // find the predecessor in the list
      while (++predIx < index)
	  {
	    pred = this->getUsage(pred);
	  }

      // knowing the predecessor, unlink the element to remove from the list
      elemToRemove = this->getUsage(pred);
      this->setUsage(pred,this->getUsage(elemToRemove));
      if (elemToRemove == last_)
	    last_ = pred;
      NACollection<T>::remove(elemToRemove);
    }

    invalidateCache();
    return TRUE;
  }

  // insert a new entry at a given position(new element becomes element # i,
  // the rest of the list moves 1 entry up)
  // use i = entries() to insert at the end, i = 0 to insert at the front
  inline CollIndex insertAt(CollIndex i, const T &elem)
  {
    CollIndex newIndex = this->freePos();
    CollIndex newUsage;

    // handle special cases i=0, i=entries_
    if (i == 0)
    {
      // add the new element to the front of the list
      newUsage = first_;
      first_ = newIndex;
      if (last_ == NULL_COLL_INDEX)
	    last_ = first_;
    }
    else if (i == this->entries())
    {
      // add the new element at the tail of the list
      if (last_ != NULL_COLL_INDEX)
	     this->setUsage(last_,newIndex);
      newUsage = NULL_COLL_INDEX;
      last_ = newIndex;
    }
    else
    {
      CollIndex pred = first_;
      CollIndex predIx = 0;

      // neither the new first nor the new last entry
      while (predIx < i-1)
	  {
	    pred = this->getUsage(pred);
	    predIx++;
	    if (pred == NULL_COLL_INDEX)
	      ABORT("Insert position in list is invalid");
	  }
    
      newUsage = this->getUsage(pred);
      this->setUsage(pred,newIndex);
    }

    // do the actual insert (common for all three cases)
    NACollection<T>::insert(newIndex,
			  elem,
			  newUsage);

    // Invalidate the cache upon an insert.
    if (userIndexCache_ >= i)
      invalidateCache();

    return newIndex;
  }

  // remove the first entry from the list and store it in "elem"
  //(returns FALSE if the list is empty and no value is returned)

  inline NABoolean getFirst(T &elem)
  {
    if (this->entries() > 0)
    {
      CollIndex oldFirst = first_;

      // copy first element from the list
      elem = this->usedEntry(oldFirst);

      // set the new first element
      first_ = this->getUsage(oldFirst);

      // adjust the pointer to the last, if necessary
      if (last_ == oldFirst)
	    last_ = NULL_COLL_INDEX;

      // actually remove the first element
      NACollection<T>::remove(oldFirst);

      // invalidate the cache
      invalidateCache();
      
      return TRUE;
    }
    else
      return FALSE;
  }
    
private:

  inline void invalidateCache()
  { userIndexCache_ = arrayIndexCache_ = NULL_COLL_INDEX; }

  CollIndex first_;              // index of fist element in list or NULL
  CollIndex last_;               // index of last element in list or NULL
  CollIndex userIndexCache_;     // cache of last accessed element # with []
  CollIndex arrayIndexCache_;    // where to find the last accessed element
}; // NAList

// -----------------------------------------------------------------------
// Array collection type template: this is similar to a list, except
// that it stores the list elements in order. Insertions in the middle
// are not allowed, but "holes" in the array(unused elements) are.
// Access to array elements by index is very fast. Beware of accessing
// uninitialized elements of the array(this will give you a core file).
// -----------------------------------------------------------------------
template <class T> class NAArray : public NACollection<T>
{

public :

  // constructor with user-defined heap
  NAArray(CollHeap *heap, CollIndex initialElements = 0) : 
       NACollection<T>(heap,initialElements) {}

  // copy ctor
  NAArray(const NAArray & other, CollHeap * heap) : 
       NACollection<T>(other, heap) {}
  
  // Resize the array to a new size(return new size)
  inline CollIndex resize(CollIndex newSize)
  { return NACollection<T>::resize(newSize); }

  // access a used entry(warning: accessing unused entries aborts!!)
  inline T & operator [](CollIndex index) { return this->usedEntry(index); }
  inline const T & operator [](CollIndex index) const
  { return this->constEntry(index); }
  inline T & at(CollIndex i) { return operator [](i); }
  inline const T & at(CollIndex i) const { return operator [](i); }

  // check whether a certain entry is used(prior to using operator [])
  inline NABoolean used(CollIndex index) const
  { return(this->getUsage(index) != UNUSED_COLL_ENTRY); }

  // find an unused position in the array(not necessarily the first one)
  inline CollIndex unusedIndex() const { return this->freePos(); }

  // delete all entries in the collection
  inline void clear() { NACollection<T>::clear(); }

  // comparison
  NABoolean operator ==(const NAArray<T> &other) const;
  inline NABoolean operator!= (const NAArray<T> &other) const
    { return NOT operator==(other); };

  // assignment operator (deep copy instead of shallow one)
  inline NAArray<T> & operator =(const NAArray<T> &other)
  { 
      if ( this != &other) { this->deallocate(); NACollection<T>::copy( (const NACollection<T>&) other); } // avoid copy-self!
			return *this;
  }

  // find a particular element in the array and return its position or
  // NULL_COLL_INDEX
  inline CollIndex find(const T &toFind) const
  { return NACollection<T>::find(toFind); }

  // insert or overwrite array entry at a specified position
  inline void insertAt(CollIndex index, const T &newEntry)
  { NACollection<T>::insert(index, newEntry); }

  // make an array entry unused(caution: no destructor is called)
  inline NABoolean remove(CollIndex index)
  { if(used(index)) { NACollection<T>::remove(index); return TRUE; }
    else return FALSE; }
  
}; // NAArray

// -----------------------------------------------------------------------
// A subarray is a subset of an array, stored as a bit vector
// -----------------------------------------------------------------------
template <class T> class NASubArray : public NASubCollection<T>
{

public:

  NASubArray(NAArray<T> *superset, CollHeap* heap) : 
       NASubCollection<T>(superset, heap) {}

  NASubArray(const NASubArray<T> &other, CollHeap * heap) : 
       NASubCollection<T>(other, heap) {}

  virtual ~NASubArray();

  inline CollIndex resize(CollIndex newSize) 
  { return NASubCollection<T>::resize(newSize); }

  inline void clear() { NASubCollection<T>::clear(); }

  inline NABoolean operator ==(const NASubArray<T> & other) const
  { return NASubCollection<T>::operator ==(other); }
  inline NABoolean operator !=(const NASubArray<T> & other) const
  { return(NOT operator ==(other)); }

  inline NASubArray<T> & operator =(const NASubArray<T> & other)
  { NASubCollection<T>::operator =(other); return *this; }

  // check whether a particular entry is part of the subarray
  inline NABoolean contains(CollIndex elem) const
  { return NASubCollection<T>::testBit(elem); }
  inline NABoolean contains(const NASubArray<T> & other) const
  { return NASubCollection<T>::contains(other); }

  // find the next used entry in the array or return false
  // e.g. in a for-loop: 
  // for(CollIndex i = 0; nextUsed(i); i++) ...element(i)...
  inline NABoolean nextUsed(CollIndex &start) const
                       { return NASubCollection<T>::nextUsed(start); }

  inline NASubArray<T> & insert(const NASubArray<T> & other)
  { NASubCollection<T>::addSet(other); return *this; }
  inline NASubArray<T> & intersectSet(const NASubArray<T> & other)
  { NASubCollection<T>::intersectSet(other); return *this; }
  inline NASubArray<T> & remove(const NASubArray<T> & other)
  { NASubCollection<T>::subtractSet(other); return *this; }
  inline NASubArray<T> & insert(CollIndex elem)
  { NASubCollection<T>::addElement(elem); return *this; }
  inline NASubArray<T> & remove(CollIndex elem)
  { NASubCollection<T>::subtractElement(elem); return *this; }
  inline NASubArray<T> & complement()
  { NASubCollection<T>::complement(); return *this; }

  inline NASubArray<T> & operator +=(const NASubArray<T> & other)
  { return insert(other); }
  inline NASubArray<T> & operator -=(const NASubArray<T> & other)
  { return remove(other); }
  inline NASubArray<T> & operator +=(CollIndex elem)
  { return insert(elem); }
  inline NASubArray<T> & operator -=(CollIndex elem)
  { return remove(elem); }

  inline WordAsBits hash() const { return NASubCollection<T>::hash(); }

  inline const T & element(CollIndex e) const
                             { return NASubCollection<T>::element(e); }

}; // NASubArray

// For bit vectors, NASubArray without a superset, source in
// file NABitVector.h

/* For these arrays, we want to be able to append, access a particular      */
/* element, and be able to iterate over and tell the size.  Destroying      */
/* would be a welcome method as well.                                       */
/*                                                                          */
template <class T> class NASimpleArray : private NAArray<T>
{

public:
  NASimpleArray (CollHeap * h) : NAArray<T>(h) {}

  // copy ctor
  NASimpleArray (const NASimpleArray & orig, CollHeap * h) 
       : NAArray<T>(orig, h) {} 

  void clearAndDestroy()
  {
    CollIndex i;
    CollIndex last = entries();
    for(i=0;i!=last;i++) {
      delete(*this)[i];
    }
    this->clear();
  }
  
  T& operator [](CollIndex index)
  { return NAArray<T>::operator [](index); };
  
  void append( T newItem)
  {
    insertAt( entries(), newItem);
  }
  
  CollIndex entries() const
  {
    return NAArray<T>::entries();
  }
  
  // operator s new and delete get inherited from private base class,
  // make them publicly available by the following inline functions
  static inline void * operator new(size_t size, CollHeap* h)
                                { return NAArray<T>::operator new(size, h); }
  static inline void operator delete(void *buffer)
                                  { NAArray<T>::operator delete(buffer); }

  inline void setHeap(CollHeap *heap)
   {
     NAArray<T>::setHeap(heap);
   }
}; // NASimpleArray

// ----------------------------------------------------------------------
// A NAHashDictionary is a pointer-based collection of keys and values.
// It is implemented by a hash table. The keys are represented by the
// class K and the values are represented by the class V. 
//
// The hash table is an array of hash buckets. Each hash bucket is 
// a list of hash bucket entries. Each hash bucket entry contains a
// key value pair.
//
// The application that uses this hash dictionary must provide a 
// hash function that accepts a key instance as an argument and
// returns an integer that is the hash value, i.e., an index for
// a hash bucket in the hash table.
// 
// The initial number of hash buckets is specified in the
// initialHashSize. If data skew causes many values to be clustered
// within a few hash buckets, then the size of the hash table can be
// changed by means of the resize() function. Note that resize()
// is usually an expensive operation since it involves the creation
// of a new hash table and the rehashing of all the keys that are
// contained in the dictionary.
//
// If the application wants to enforce that exactly one instance of
// a certain key can be inserted in this dictionary, it can specify 
// such a requirement when the hash dictionary is created. The 
// insertion of a duplicate key will fail when enforceUniqueness is 
// set to TRUE. The first key value pair that is inserted in the hash
// table will persist after the failure in inserting a duplicate key.
// $$$$ In the future, the insertion of a duplicate key should
// $$$$ cause an exception.
// 
// In general, the hash table can contain duplicate keys or even
// duplicate key value pairs. The application must initialize an iterator 
// for retrieving mutiple instances of a given key or a given key
// value pair that are stored in the hash dictionary.
// In general, the hash table can contain duplicate keys or even duplicate
// key value pairs. The application must create an iterator object for
// retrieving mutiple instances of a given key or a given key value pair
// that are stored in the hash dictionary.  See NAHashDictionaryIterator<K,V> 
// below for how to create iterator objects.
// 
// Requirement:
// ***********
// The classes K and V must each support well-defined equality semantics.
// It must be implemented by overloading the operator "==". This means,
// an implementation must exist for K::operator ==() as well as for 
// V::operator ==(). The implementation of the hash dictionary invokes 
// these operator s in order to compare stored keys and values with a
// a given key and value.
// ----------------------------------------------------------------------

// -----------------------------------------------------------------------
// class NAHashBucketEntry - A hash bucket entry.
// -----------------------------------------------------------------------
template <class K, class V>
class NAHashBucketEntry  : public NABasicObject
{
public:
  // --------------------------------------------------------------------
  // Constructor function
  // --------------------------------------------------------------------
  NAHashBucketEntry(K* key, V* value) : key_(key), value_(value)
  { }

  // copy ctor
  NAHashBucketEntry(const NAHashBucketEntry<K,V>& other) 
   : key_(other.key_), value_(other.value_)
  { }

  // --------------------------------------------------------------------
  // Destructor function
  // --------------------------------------------------------------------
  virtual ~NAHashBucketEntry();
  
  // --------------------------------------------------------------------
  // Accessor functions for a NAHashBucket
  // --------------------------------------------------------------------
  inline K* getKey() const                               { return key_; }

  inline V* getValue() const                           { return value_; }

  // If deleteContents is set, then the key and the value that are
  // contained in this hash bucket entry are also deleted. 
  void clear(NABoolean deleteContents = FALSE);
  
  void display() const;

  Int32 printStatistics(char *buf);
  
  // return byte size of this object
  Lng32 getByteSize() const { return sizeof(*this); }

private:
  
  K* key_;

  V* value_;
  
}; // class NAHashBucketEntry

// -----------------------------------------------------------------------
// class NAHashBucket - A hash bucket.
// -----------------------------------------------------------------------

template <class K, class V>
class NAHashBucket : public NABasicObject
{
public:
  // --------------------------------------------------------------------
  // Constructor function
  // --------------------------------------------------------------------
  NAHashBucket(CollHeap * heap): bucket_(heap), heap_(heap) {}
 
  // copy ctor
  NAHashBucket(const NAHashBucket<K,V>& other, CollHeap * heap);
  
  // --------------------------------------------------------------------
  // Destructor function
  // --------------------------------------------------------------------
  virtual ~NAHashBucket();

  // --------------------------------------------------------------------
  // Accessor functions for a NAHashBucket
  // --------------------------------------------------------------------
  
  // Check whether this hash bucket contains a given key, or a given 
  // key value pair. 
  NABoolean contains(const K* key) const;
  NABoolean contains(const K* key, const V* value) const;

  // The number of entries contained in this hash bucket.
  inline CollIndex entries() const              { return bucket_.entries(); }
  
  // Get the first value that is encountered that corresponds to the 
  // given key. If no such key exists, getFirstValue() returns a NULL.
  V* getFirstValue(const K* key) const;

  // Get all key value pairs that correspond to the given key or
  // given key value pair. If both key and value are NULL, then
  // return all the key value pairs contained in this bucket.
  // The qualifying key value pairs are returned in the container.
  void getKeyValuePair(const K* key, const V* value,
		       NAHashBucket<K,V>& container) const;

  // Overload the array index operator to access a particular entry. 
  inline const NAHashBucketEntry<K,V>* operator [](Lng32 index) const
                                               { return bucket_[index]; }
  
  // --------------------------------------------------------------------
  // Mutator functions for a NAHashBucket
  // --------------------------------------------------------------------

  // Delete all hash bucket entries. If deleteContents is set, then
  // the contents of each hash bucket entry are also deleted. 
  void clear(NABoolean deleteContents = FALSE);

  // Insert a given key value pair into this hash bucket.
  inline void insert(K* key, V* value)       
    { bucket_.insertAt(0,new(heap_) NAHashBucketEntry<K,V>(key, value)); }

  // Remove the first hash bucket entry that contains the given key.
  K* remove(K* key);
 
  void display() const;
  
  Int32 printStatistics(char *buf);
  
  // return byte size of this object
  Lng32 getByteSize() const 
    { return sizeof(*this) + (bucket_ ? bucket_->getByteSize() : 0); }

private:
  
  // -------------------------------------------------------------------- 
  // A hash bucket contains a number of entries.
  // --------------------------------------------------------------------
  NAList<NAHashBucketEntry<K,V> *>  bucket_;

  // -----------------------------------------------------------------------
  // A CollHeap* for memory allocation within NAHashBucket. 
  // -----------------------------------------------------------------------
  
  CollHeap * heap_;

}; // class NAHashBucket

// -----------------------------------------------------------------------
// class NAHashDictionary - The hash dictionary.
// -----------------------------------------------------------------------
template <class K, class V>
class NAHashDictionary : public NABasicObject
{
public:

friend class NAKeyLookup<K,V>;
friend class NAHashDictionaryIterator<K,V>;

#define NAHashDictionary_Default_Size 11

  // --------------------------------------------------------------------
  // Constructor functions.
  // --------------------------------------------------------------------
  // Previously the code was ugly -- it used a hash function that was defined
  // in the scope of the file using the NAHashDictionary template. New 
  // compilers produce an error ("undeclared variable" ) in case the template
  // gets defined (i.e., Collections.h is included) but not instantiated!
  // Solution: The hash function should be a method of the "key" class K !!
  // Thus: we fixed the NAHashDictionary code.
  // Problem: The code using NAHashDictionary has to be fixed as well (turn
  // those hash functions into hash() methods). This code is mainly compiler
  // code, and we don't have the time to do and test those changes; thus we
  // left the old ugly way under the #else clause, until someone finishes
  // this work.
//  NAHashDictionary(
//#else // the old way
  NAHashDictionary(ULng32(*hashFunction)(const K&), 
//#endif
		   ULng32 initialHashSize = NAHashDictionary_Default_Size,
		   NABoolean enforceUniqueness = FALSE,
		   CollHeap * heap=0 /* where to allocate memory */ );

  // copy ctor
  NAHashDictionary(const NAHashDictionary<K,V>& other, CollHeap * heap);
  
  // --------------------------------------------------------------------
  // Destructor function
  // --------------------------------------------------------------------
  virtual ~NAHashDictionary();

  // --------------------------------------------------------------------
  // Accessor functions for a NAHashDictionary.
  // --------------------------------------------------------------------
  
  // Check whether this dictionary contains a certain key, or a 
  // certain key value pair. 
  inline NABoolean contains(const K* key) const
       { return(*hashTable_)[getHashCode(*key)]->contains(key); }
  inline NABoolean contains(const K* key, const V* value) const
       { return(*hashTable_)[getHashCode(*key)]->contains(key,value); }

  // The number of key value pairs that are contained in this dictionary.
  inline Lng32 entries() const                        { return entries_; }

  // Find the first value corresponding to the given key.
  // This method is especially useful(and effcient) when the hash
  // dictionary enforces uniqueness.
  V* getFirstValue(const K* key) const
        { return(*hashTable_)[getHashCode(*key)]->getFirstValue(key); } 

  // Returns TRUE if the dictionary contains no key value pairs.
  // FALSE otherwise.
  inline NABoolean isEmpty() const            { return(entries_ == 0); }

  // --------------------------------------------------------------------
  // NB: iterator functions for this class (which were previously
  // described here) have been taken over by the friend class
  // NAHashDictionaryIterator<K,V> (see below).
  // --------------------------------------------------------------------

  // --------------------------------------------------------------------
  // Mutator functions for a NAHashDictionary
  // --------------------------------------------------------------------

  // Remove all key value pairs.
  void clear(NABoolean deleteContents = FALSE);
  
  // Delete all key and value pairs. Does not check whether they are unique.
  // Delete all entries in the hash buckets.
  // Delete all hash buckets.
  // Delete the hash table and the dictionary.
  void clearAndDestroy()                                 { clear(TRUE); }
  
  // Insert a key value pair.
  // Returns the given value if the insertion is successful.
  // Returns a NULL if the insertion fails, such as when a 
  // duplicate key is inserted into a dictionary that enforces
  // uniqueness on keys.
  K* insert(K* key, V* value);
  
  // Remove one key value pair corresponding to the given key.
  // Returns the given key value if the removal is successful.
  // Returns NULL if the given key value is not found in the dictionary.
  K* remove(K* key);
	
  // Change the number of hash buckets in an attempt to distribute the 
  // keys and values uniformly over all the hash buckets.
  void resize(ULng32 newHashSize);
  
  virtual void display() const;
  
  Int32 printStatistics(char *buf);
  
  // return byte size of this object
  Lng32 getByteSize() const 
    { return sizeof(*this) + (hashTable_ ? hashTable_->getByteSize() : 0); }

  // return byte size of one NAHashBucketEntry
  static Lng32 getBucketEntrySize() { return sizeof(NAHashBucketEntry<K,V>); }

  ULng32 getNumBuckets() { return hashSize_; }

private:
  // --------------------------------------------------------------------
  // Helper function for creating a hash table
  // --------------------------------------------------------------------
  void createHashTable(ULng32 hashSize);
  
  // --------------------------------------------------------------------
  // Private method for generating a hash code.
  // --------------------------------------------------------------------
  ULng32 getHashCode(const K& key) const;

private: 

  // Needed only for the old way of NAHashDictionary -- see comment above
  // --------------------------------------------------------------------
  // The hash function that is applied to the key for determining the 
  // bucket to which it belongs.				  
  // --------------------------------------------------------------------
  ULng32(*hash_)(const K&);
  
  // --------------------------------------------------------------------
  // The hash table is an array of hash buckets.
  // --------------------------------------------------------------------
  NAArray<NAHashBucket<K,V>*>* hashTable_;
 
  // --------------------------------------------------------------------
  // The hashSize is stored here for improving efficiency.
  // --------------------------------------------------------------------
  ULng32 hashSize_;

  // --------------------------------------------------------------------
  // The number of key value pairs existing in the hash dictionary are
  // cached here.
  // --------------------------------------------------------------------
  Lng32 entries_;

  // --------------------------------------------------------------------
  // The application is allowed to insert only one instance of a key
  // if uniqueness is to be enforced. An attempt to insert another
  // instance causes an exception to be raised. 
  //($$$ Presently this exception is not implemented!)
  // --------------------------------------------------------------------
  NABoolean enforceUniqueness_;
  
  // -----------------------------------------------------------------------
  // The CollHeap* for memory allocation within NAHashDictionary 
  // -----------------------------------------------------------------------
  CollHeap * heap_;

}; // class NAHashDictionary

// -----------------------------------------------------------------------
// class NAHashDictionaryIterator - iterator class for NAHashDictionary
// -----------------------------------------------------------------------
  // --------------------------------------------------------------------
  // An iterator for the NAHashDictionary.
  //
  // The iterator is a mechanism that allows an application to iterate
  // over a set of key value pairs that are stored in the hash dictionary.
  // The set is constructed according to one of the following three
  // specifications.
  // 1) The application does not provide either a key or a value, i.e.,
  //    both are NULL. Such an iterator is used for iterating over 
  //    all the key value pairs that are stored in the hash dictionary.
  // 2) The application provides a key but not a value, i.e., value is
  //    NULL. Such an iterator can be used for iterating over all 
  //    those key value pairs in the hash dictionary whose keys are 
  //    equal to(the same as) the given key.
  // 3) The application provides a key as well as a value. Such an 
  //    iterator can be used for iterating over all those key value
  //    pairs that are equal to(the same as) the given key value pair.
  // 
  // --------------------------------------------------------------------

// -----------------------------------------------------------------------
// Iterators are easy to use!
//
// ( given the existence of 'NAHashDictionary<int,double> bob' )
// 
// --> create an iterator for bob for key-value 3
// int * key ;
// double * value ; 
// NAHashDictionaryIterator<int,double> bobIter (bob, 3) ;
// 
// --> loop over and print out everything in the iterator
// for ( int i = 0 ; i < bobIter.entries() ; i++) 
// {
//   bobIter.getNext (key,value) ; 
//   cout << key << " : " << value << endl ;
// } 
//
// --> another way to loop over it (taste preference)
// bobIter.getNext (key,value) ; 
// while ( key && value )
// {
//   cout << key << " : " << value << endl ;
//   bobIter.getNext (key,value) ; 
// } 
//
// --> create a copy of this iterator
// bobIter2 (bobIter) ; 
//
// --> reset this copied iterator to loop from the beginning
// bobIter2.reset() ; 
//
// --> destroying the iterator : just let it go out of scope!
// -----------------------------------------------------------------------

template <class K, class V>
class NAHashDictionaryIterator : public NABasicObject
{
public:
  // basic ctor
  NAHashDictionaryIterator<K,V> (const NAHashDictionary<K,V> & dict, 
                                 const K* key = NULL,
                                 const V* value = NULL) ;

  // copy ctor
  NAHashDictionaryIterator<K,V> (const NAHashDictionaryIterator<K,V> & other,
                                 CollHeap * heap) ; 

  // dtor
  ~NAHashDictionaryIterator<K,V>() ; 

  // The number of key value pairs that can be accessed using this
  // iterator.
  inline CollIndex entries() const       { return iterator_.entries(); }

  // Our current position within the list
  inline Lng32 position() const      { return iteratorPosition_; }

  // Reset the counter so we can iterate again from the start
  inline void reset()               
  { 
    if ( entries() > 0 ) 
      iteratorPosition_ = FIRST_COLL_INDEX ; 
    else
      iteratorPosition_ = NULL_COLL_INDEX ; 
  }

  // Advances the iterator and returns a new key value pair.
  // Both key and value are set to NULL after all the key value pairs
  // have been iterated over.
  void getNext(K*& key, V*& value) ;

private:
  NAHashDictionaryIterator<K,V>() ; // never create an uninitialized iterator!

  // --------------------------------------------------------------------
  // The iterator for the hash dictionary is implemented by an
  // empty hash bucket which is initialized with pointers to hash
  // bucket entries when the iterator is created.
  // --------------------------------------------------------------------
  NAHashBucket<K,V> iterator_; 
  Lng32      iteratorPosition_; // where in iterator_ we currently are
};

// **********************************************************************
// gcc requires these definitions be seen before the hashKey is used in
// NAKeyLookup. This may be a bug in gcc that could be fixed in a
// newer release. The other solution to this problem involves reordering
// include files. 
// **********************************************************************
class QualifiedName;
class ExtendedQualName;
class CorrName;
class ColRefName;
class NAString;
class NARoutineDBKey;
ULng32 hashKey(const QualifiedName&);
ULng32 hashKey(const ExtendedQualName&);
ULng32 hashKey(const CorrName&);
ULng32 hashKey(const ColRefName&);
ULng32 hashKey(const NAString&);
ULng32 hashKey(const NARoutineDBKey&);

// **********************************************************************
// NAKeyLookup : A Descriptor Store
//            (a storage area for a given class of descriptors)
//
// The descriptor store is an associative storage for object descriptors.
// It is implemented by a hash table in which the name of the object
// is used as the hash key. The hash table provides a fast associative
// lookup through the association of a name(as a key) with each object
// descriptor. Each descriptor store imposes a uniqueness constraint
// on the names that are used as keys.
//
// Consumers need to support:
//   - a V.getKey() function that returns a value of type K*
//   - well-defined equality semantics, i.e. K::operator ==(const K&)
//   - an external unsigned long hashKey(const K&) function
//    (must be external because NAString, i.e. RWCString, does not have
//     a static function of this signature)
//   - a K::K(const K &, CollHeap * h), i.e., a copy ctor that takes a 
//     CollHeap* as a 2nd parameter
//     e.g., NAString::NAString (const NAString & other, CollHeap * h=0) ;
// **********************************************************************

class NAKeyLookupEnums		// not a template, just a namespace for enum
{
public:
  enum KeyProvenance { KEY_INSIDE_VALUE, KEY_OUTSIDE_VALUE };
};

template <class K, class V>
class NAKeyLookup : public NAHashDictionary<K,V>
{
public:

  // --------------------------------------------------------------------
  // Constructor functions
  // --------------------------------------------------------------------
  NAKeyLookup(short initSize, 
              NAKeyLookupEnums::KeyProvenance keyProvenance,
              CollHeap * heap) :
       NAHashDictionary<K,V>(&hashKey, initSize, TRUE,heap),
       keyProvenance_(keyProvenance)
  {}

  // copy ctor
  NAKeyLookup (const NAKeyLookup & nakl, CollHeap * heap) :
       NAHashDictionary<K,V>(nakl, heap),
       keyProvenance_(nakl.keyProvenance_)
  {}

  // --------------------------------------------------------------------
  // Destructor functions
  // --------------------------------------------------------------------
  ~NAKeyLookup() {}

  // --------------------------------------------------------------------
  // Methods for a NAKeyLookup.
  // 'Const' is used on method signatures where it makes sense
  //(thus allowing the methods to be called more easily, and safely).
  // --------------------------------------------------------------------
  void clearAndDestroy()
  {
    // Frees memory for BOTH K AND V for all such pairs,
    // plus internal RW data structures.
    NAHashDictionary<K,V>::clearAndDestroy();
  }

  inline V* get(const K* kk) const                      // retrieve a tuple
  {
    return getFirstValue(kk);
  }
       		
  inline void insert(const V* vv)			// insert a tuple
  {
    // K and V must point to distinct regions of memory
    // in order for clearAndDestroy() to work 
    //(i.e. not free same mem twice)!
    const K* kk;
    if(keyProvenance_ == NAKeyLookupEnums::KEY_INSIDE_VALUE)
      kk = new(this->heap_) K(*vv->getKey(), this->heap_);
    else
      kk =        vv->getKey();
    K* rv = NAHashDictionary<K,V>::insert((K*)kk,(V*)vv); //(cast away constness)

    if ( rv == NULL && keyProvenance_ == NAKeyLookupEnums::KEY_INSIDE_VALUE )
      delete kk ; // avoid memleak!
  }							

  void remove(const K* kk)
  {
    // insert creates a key object and here we undo this operation. We collect
    // keys and values that match the key in the iterator. Because of unique-
    // ness constraints, there should only be one element. The uniqueness is
    // asserted during insert operation

    V* vv=NULL;
    NAHashDictionaryIterator<K,V> iter (*this, kk, vv); 

    K* rv = NAHashDictionary<K,V>::remove((K*)kk);
    if (keyProvenance_ == NAKeyLookupEnums::KEY_INSIDE_VALUE && rv!= NULL)
    {
      K* key;
      V* value;
      iter.getNext(key, value);
      delete key;
    }
  }
  
  void dump(NAList<V *>& vv) const			// dump contents to list
  {
    NAHashDictionaryIterator<K,V> iter (*this) ; 

    K* key;
    V* value;
    iter.getNext(key,value);
    while(key)
      {
	vv.insert(value); 
	iter.getNext(key,value);
      }
  }

  void dumpKeys (NAList<K *>& kk) const                 // dump the keys to a list
  {
    NAHashDictionaryIterator<K,V> iter (*this) ; 

    K* key;
    V* value;
    iter.getNext(key,value);
    while(key)
      {
	kk.insert(key); 
	iter.getNext(key,value);
      }
  }

  // NAKeyLookup::insert() may have allocated memory to insert the key
  // part of the <key,value> pairs that were put into the NAKeyLookup.
  // However, since we didn't allocate the value parts, we might now 
  // want to deallocate them.
  //
  // This method can be called as an alternative to clearAndDestroy(),
  // when the user only wants to delete the memory that NAKeyLookup itself
  // allocated.
  void clearAndDestroyKeysOnly ()
  {
    // no-op if NAKeyLookup couldn't possibly allocate any memory 
    if ( keyProvenance_ != NAKeyLookupEnums::KEY_INSIDE_VALUE ) return ;

    NAList<K *> keys (this->heap_);
    dumpKeys (keys) ;
    CollIndex entries = keys.entries() ; 
    for ( CollIndex i = 0 ; i < entries ; i++ )
      delete keys[i] ; 

    this->clear() ;
  }


private:  

  const NAKeyLookupEnums::KeyProvenance keyProvenance_;

}; // NAKeyLookup


#pragma warn(1506)   // warning elimination


// -----------------------------------------------------------------------
// This is done similarly to Tools.h++: if we want to instantiate
// templates at compile time, the compiler needs to know the
// implementation of the template functions. Do this by setting the
// preprocessor define NA_COMPILE_INSTANTIATE.
// -----------------------------------------------------------------------
#if defined(NA_COMPILE_INSTANTIATE)
 #include "Collections.cpp"
#endif

#endif /* COLLECTIONS_H */

