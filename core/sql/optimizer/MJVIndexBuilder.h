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
******************************************************************************
*
* File:         MJVIndexBuilder.h
* Description:  MJVIndexBuilder algorithm Data structures definition
* Created:      7/1/01
* Language:     C++
*
*
*
*
******************************************************************************
*/

#ifndef MJV_INDEX_BUILDER_H
#define MJV_INDEX_BUILDER_H

#include <memory.h> 

#include <Collections.h> 
#include <CmpCommon.h>

class ColIndList;
class ColIndSet;
class ColIndSetBucketVector;
class NestingStack;
class MJVIndCook;
class ColIndSetMatrix;
class MJVIndexBuilder;


typedef LIST(ColIndList)	IndexList;
typedef SET(ColIndSet)		ColIndSetBucket;

// Actually this structure objects are used as rows of ColIndSetMatrix.
// The structure is not mentioned in the document,
// because it has no logical value.
typedef ARRAY(Lng32)		EnumArray;

// Uncomment the following define for debug printing:
//#define MJVIB_VERBOSE

//-----------------------------------------------------------------------------
// General
//-----------------------------------------------------------------------------
// All the classes below are designed for Minimal MV Secondary Indexes 
// algorithm's implementation.
// The aim of the algorithm is to save resources on the unnecessary 
// indices maintenance.
// The input is set of RCIs (Referencing Clustering Indexes) of the 
// base tables and the MV Clustering index; the output is the reduced 
// subset of the input RCIs (or their permutations)
// The necessary condition is that every of the RCIs can be permuted 
// to make prefix for one of the output RCIs. 
// The additional option for reducing the number of secondary indices is 
// that we can ignore the RCI that can be permuted to make 
// prefix for the MJV clustering key.
//
// For more information, see [MJVIndexBuilder.doc] 
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// class: ColIndSet
//-----------------------------------------------------------------------------
// This class implements the ColIndSet data structure the basic brick of the 
// MJVIndexBuilder algorithm..
// ColIndSet is simply a set of longs (column positions from MJV view).
//-----------------------------------------------------------------------------

class ColIndSet : public SET(Lng32)
{
public:
  //
  // ctors
  //
  ColIndSet(CollHeap *heap=0) :
    SET(Lng32)(heap)		{};
  
  ColIndSet(const ColIndSet& other, CollHeap *heap) :
    SET(Lng32)(other, heap)		
  {
  };
  
  ColIndSet(ColIndList indexList, CollHeap *heap);
  
  //
  // dtor
  //
  virtual ~ColIndSet()	{};
  
  
  NABoolean operator == (const ColIndSet& other) const;
  
  NABoolean isSubsetOf(const ColIndSet& other) const;
  
  ColIndSet intersect(const ColIndSet& other) const;

#ifdef MJVIB_VERBOSE
  virtual void out()	const	;
#endif
};

//-----------------------------------------------------------------------------
// class: ColIndSetBucketVector
//-----------------------------------------------------------------------------
// This class implements the ColIndSetBucketVector data structure.
// ColIndSetBucketVector is array of ColIndSetBuckets; 
// pointer in entry K points to ColIndSetBucket number K. 
// The purpose of ColIndSetBucketVector is to store the ColIndSets, 
// divided into groups of the same size, each group containing no two 
// identical (in terms of set) ColIndSets. 
//-----------------------------------------------------------------------------

class ColIndSetBucketVector : public ARRAY(ColIndSetBucket*)
{
public:
  //
  // default ctors
  //
  ColIndSetBucketVector(CollHeap *heap) :
		    maxNonEmptyEntrySize_(0), 
		    heapPtr_(heap),
		    ARRAY(ColIndSetBucket*)(heap) 
  {};
  
  //
  // dtor
  //
  virtual ~ColIndSetBucketVector() 
  {
    for (size_t i=0; i < entries(); i++) {
      delete at(i);
    }
  };
  
  size_t getMaxNonEmptyEntrySize() const {	return maxNonEmptyEntrySize_;	};
  
  size_t getNumberOfEntities() const ;
  
  CollHeap * getHeapPtr() const {	return heapPtr_;	};
  
  //
  // Inserts the given ColIndSet into self -- the element of size K is
  // to be inserted into the bucket with size_ == K.
  //
  ColIndSetBucketVector& insert (ColIndSet& newSet);
  
#ifdef MJVIB_VERBOSE
  virtual void out()	const	;
#endif
						
  
private:
  // Prevent accidental use of default copy Ctor and = operator.
  ColIndSetBucketVector& operator=(const ColIndSetBucketVector& other);
  ColIndSetBucketVector();
  ColIndSetBucketVector(const ColIndSetBucketVector& other);

private:

  size_t maxNonEmptyEntrySize_;
  CollHeap *heapPtr_;
};

//-----------------------------------------------------------------------------
// class: ColIndSetMatrix
//-----------------------------------------------------------------------------
// This class implements the ColIndSetMatrix data structure.
// ColIndSetMatrix is kind of relations matrix, storing information 
// about ColIndSets relations by operator "is subset of": columns as well 
// as rows are ColIndSets' order numbers and 
// "1/0" means "is subset of/is not subset of" 
// for correspondent ColIndSets.
// The matrix is used for deciding which ColIndSets is the best to be 
// the result subset if which -- resultin minimum new index creation.
//-----------------------------------------------------------------------------

class ColIndSetMatrix : public ARRAY(EnumArray*)
{
public:
  //
  // ctors
  //
  ColIndSetMatrix	(const ColIndSetBucketVector& src, 
			 size_t size, CollHeap *heap);
  
  //
  // dtor
  //
  virtual ~ColIndSetMatrix() {};
  
  NABoolean	endOfMatrix()		{ return currIndex_ == lastIndex_;};
  
  size_t	getSupersetsNum (size_t index)	{ return at(index)->at(0); };
  size_t	getSubsetsNum	(size_t index)	{ return at(0)->at(index); };
  
  ColIndSet&	getSetByIndex(size_t index)	{ return setsList_.at(index);};
  
  void	setSettledByIndex(size_t i);
  
  size_t first() ; 
  size_t next() ; 
  
  size_t 	pickTheBestUnsettled	();
  size_t 	pickTheBestSettlement	(size_t toPushIndex);
  
  void update (size_t index); 
#ifdef MJVIB_VERBOSE
  virtual void out()	const	;
#endif

private:
  // Prevent accidental use of default copy Ctor and = operator.
  ColIndSetMatrix& operator=(const ColIndSetMatrix& other);
  //
  // ctors
  //
  ColIndSetMatrix();
  ColIndSetMatrix(const ColIndSetMatrix& other);
  
private:
  ARRAY(ColIndSet)  setsList_;
  ARRAY(NABoolean)  settledList_;
  size_t	    currIndex_;
  size_t	    lastIndex_;
  
};



//-----------------------------------------------------------------------------
// class: NestingStack
//-----------------------------------------------------------------------------
// This class implements the NestingStack data structure.
// NestingStack is stack (list implementation) of ColIndSets, 
// nested from top to bottom - each new top is a subset of the previous one.
// (For MJV, NestingStack is a basis for a single secondary index).
//	Example:
//		For sets: {5}, {2,5}, {4,5,6,2}
//		the NestingStack will be:
//							{5}
//							{2,5}
//							{4,5,6,2}
//
//-----------------------------------------------------------------------------

class NestingStack : public LIST(ColIndSet*)
{
public:
  //
  // ctors
  //
  NestingStack(CollHeap *heap) :
	heapPtr_(heap),
	LIST(ColIndSet*)(heap)		
  {};
      
  NestingStack(const NestingStack& other, CollHeap *heap):
	heapPtr_(heap),
	LIST(ColIndSet*)(heap)		
  {};
  
  //
  // dtor
  //
  virtual ~NestingStack()
  {
    for (size_t i=0; i < entries(); i++) {
      delete at(i);
    }
  };
  
  CollHeap * getHeapPtr() const {	return heapPtr_;	};
  
  //
  // This is the main function of the class. 
  // It buils index corresponding to self, 
  // i.e. for each ColIndSet in the structure 
  // there is a prefix of the result to which the ColIndSet is equal.
  //
  void buildIndex(ColIndList& result);
  
  // 
  // returns	TRUE if newSet is a subset of the top;
  //		FALSE - otherwise
  //
  NABoolean	isSubsetOfTop (const ColIndSet& newSet);
  
  ColIndSet&	top();
  NestingStack& push(const ColIndSet& newSet);
  

#ifdef MJVIB_VERBOSE
  virtual void out()	const	;
#endif

private:
  // Prevent accidental use of default copy Ctor and = operator.
  NestingStack& operator=(const NestingStack& other);
  NestingStack();
  NestingStack(const NestingStack& other);
  
private:
  CollHeap *heapPtr_;
  
};

//-----------------------------------------------------------------------------
// class: MJVIndCook
//-----------------------------------------------------------------------------
// This class implements the MJVIndCook data structure.
// MJVIndCook is a list of NestingStacks. Each stack is, actually, the 
// representation of one of the result indices. 
// The size of list is the number of result indices.
//-----------------------------------------------------------------------------

class MJVIndCook : public LIST(NestingStack*)
{
public:
  //
  // ctors
  //
  MJVIndCook(CollHeap *heap):
	heapPtr_(heap),
	LIST(NestingStack*)(heap)		
  {};
      
  MJVIndCook(const MJVIndCook& other, CollHeap *heap):
	heapPtr_(heap),
	LIST(NestingStack*)(heap)		
  {};
  
  //
  // dtor
  //
  virtual ~MJVIndCook()
  {
    for (size_t i=0; i < entries(); i++) {
      delete at(i);
    }
  };
  
  CollHeap * getHeapPtr() const {	return heapPtr_;	};
  
  //
  // Calls the buildIndex() function in all NestingStack contained
  // and makes list of the indices
  //
  IndexList* buildIndex();
  
  //
  // Inserts the given ColIndSet into self. 
  //
  MJVIndCook& insert(const ColIndSet& newSet);
  
  //
  // Inserts the given ColIndSet into self 
  // onto the specified ColIndSet setToBeCovered. 
  //
  MJVIndCook& insertOnto  ( const ColIndSet& setToBeCovered, 
			    const ColIndSet& newSet	    );
  
  //
  // Inserts new NestingStack, containing the given 
  // ColIndSet set, into self. 
  //
  MJVIndCook& addNewNestingStack(const ColIndSet& set);
  
#ifdef MJVIB_VERBOSE
  virtual void out()	const	;
#endif

private:
  // Prevent accidental use of default copy Ctor and = operator.
  MJVIndCook& operator=(const MJVIndCook& other);
  MJVIndCook();
  MJVIndCook(const MJVIndCook& other);
  
private:
  
  CollHeap *heapPtr_;
};


//-----------------------------------------------------------------------------
// class: MJVIndexBuilder
//-----------------------------------------------------------------------------
// This class implements the MJVIndexBuilder algorithm.
// The algorithm flow itself.
//	1) Initializes ColIndSetBucketVector from <code>inputList_</code>
//	2) Initializes MJVIndMatrix from ColIndSetBucketVector
//	2) Builds MJVIndCook out from MJVIndMatrix
//	3) Calls buildIndex() for MJVIndCook built, which envokes 
//	    buildIndex() for each of its NestingStacks.
//-----------------------------------------------------------------------------

class MJVIndexBuilder
{
public:
  //
  // ctors
  //
  MJVIndexBuilder(CollHeap *heap) :
      heap_(heap)
  {};
  
  //
  // dtor
  //
  virtual ~MJVIndexBuilder() {};
  
  //
  // The algorithm flow itself.
  //	1) Initializes ColIndSetBucketVector from inputList_
  //	2) Initializes MJVIndCook from ColIndSetBucketVector
  //	3) Calls buildIndex() for MJVIndCook built
  //
  IndexList* buildIndex(const IndexList inputList);
  
  CollHeap * getHeapPtr()   { return heap_;	}
  
private:
  // Prevent accidental use of default copy Ctor and = operator.
  MJVIndexBuilder& operator=(const MJVIndexBuilder& other);
  MJVIndexBuilder();
  MJVIndexBuilder(const MJVIndexBuilder& other);
  
private:
  
  CollHeap *heap_;
};


#endif // MJV_INDEX_BUILDER_H
