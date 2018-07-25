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
* File:   MJVIndexBuilder.cpp
* Description:  MJVIndexBuilder algorithm Data structures definition
* Created:      7/1/01
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "MJVIndexBuilder.h"
#include "ColIndList.h"

#define OPEN_SET "{"
#define CLOSE_SET "}"
#define OPEN_LIST "<"
#define CLOSE_LIST ">"
#define NEW_LINE "\n"


#ifdef MJVIB_VERBOSE
  static void	  PrintColIndList (const IndexList& toPrint, char* name	);
#endif

//-----------------------------------------------------------------------------
// class: ColIndSet methods
//-----------------------------------------------------------------------------

//
//-------	ColIndSet::ColIndSet
//---------------------------------------------------
// constructs ColIndSet of the elements of the given CollHeap
//---------------------------------------------------------------------------//
ColIndSet::ColIndSet(ColIndList indexList, CollHeap *heap) : 
SET(Lng32)(heap)
{
  size_t length = indexList.entries();
  for (size_t i = 0; i < length; i++)	{
    insert(indexList.at(i));	
  }
}


//
//-------	ColIndSet::out
//---------------------------------------------------
// print method
//---------------------------------------------------------------------------//
//virtual 
#ifdef MJVIB_VERBOSE
void
ColIndSet::out()	const
{
  size_t size = entries();
  printf(OPEN_SET);
  for (size_t i = 0; i < size; i++) {
    printf("%d", at(i));
    if (i < size-1) {
      printf(", ");
    }
  }
  printf(CLOSE_SET);
}
#endif


//
//-------	ColIndSet::operator ==
//---------------------------------------------------
// equality operator
//---------------------------------------------------------------------------//
NABoolean 
ColIndSet::operator == (const ColIndSet& other) const
{
  size_t size = entries();
  
  if (other.entries() != size) {
    return FALSE;
  }
  return isSubsetOf(other);
}

//
//-------	ColIndSet::isSubsetOf
//---------------------------------------------------
// return	TRUE if self is a subset of the given ColIndSet
//			FALSE - otherwise
//---------------------------------------------------------------------------//
NABoolean 
ColIndSet::isSubsetOf(const ColIndSet& other) const
{
  size_t size = entries();
  
  if (size > other.entries()) {
    return FALSE;
  }
  
  for (size_t i = 0; i < size; i++) {
    if (! other.contains(at(i))) {
      return  FALSE;
    }
  }
  return TRUE;
}

//
//-------	ColIndSet::intersect
//---------------------------------------------------
// returns intersection with the given <another> object
//---------------------------------------------------------------------------//
ColIndSet  
ColIndSet::intersect(const ColIndSet& another) const
{
  size_t minSize = entries();
  ColIndSet result;
  Lng32 currColInd;

  if (minSize < another.entries()){
    for (size_t i = 0; i < minSize; i++) {
      if (another.contains(currColInd = at(i))) {
	result.insert(currColInd);
      }
    }
  } else {
    minSize = another.entries();
    for (size_t i = 0; i < minSize; i++) {
      if (contains(currColInd = another.at(i))) {
	result.insert(currColInd);
      }
    }
  } 
  return result;
}

//-----------------------------------------------------------------------------
// class: ColIndSetBucketVector methods
//-----------------------------------------------------------------------------

//
//-------	ColIndSetBucketVector::out
//---------------------------------------------------
// print method
//---------------------------------------------------------------------------//
//virtual 
#ifdef MJVIB_VERBOSE
void
ColIndSetBucketVector::out()		const
{
  size_t size = getMaxNonEmptyEntrySize();
  CMPASSERT(size <= entries());

  printf("\n\tColIndSetBucketVector:\n");
  printf("\t----------------------\n");
  
  for (size_t i = 1; i <= size; i++) 
  {
    if (at(i)->entries() > 0) 
    {
      printf("\n%d --> ", i);

      size_t bucketSize = at(i)->entries();
      printf(OPEN_SET);
      for (size_t j = 0; j < bucketSize; j++) 
      {
	at(i)->at(j).out();
	if (j < bucketSize-1) 
	{
	  printf(", ");
	}
      }
      printf(CLOSE_SET);
      printf(NEW_LINE);

    } 
    else 
    {
      printf("\n%d\n", i );
    }
  }
};
#endif
//
//-------	ColIndSetBucketVector::getNumberOfEntities
//---------------------------------------------------
// returns the number of ColIndSets inserted
//---------------------------------------------------------------------------//
//virtual 
size_t
ColIndSetBucketVector::getNumberOfEntities()	const
{
  size_t size = 0;
  for (size_t i = getMaxNonEmptyEntrySize(); i > 0; i--) {
    size += at(i)->entries();
  }
  return size;
};

//
//-------	ColIndSetBucketVector::insert
//---------------------------------------------------
// Inserts the given ColIndSet into self -- the element of size K is
// to be inserted into the bucket with size_ == K.
//---------------------------------------------------------------------------//
ColIndSetBucketVector& 
ColIndSetBucketVector::insert (ColIndSet& newSet)
{
  // get the size of new ColIndSet -
  // remember - the size defines the order number of ColIndSetBucket
  // the new ColIndSet will be inserted into.
  Int32 newSetSize = newSet.entries();
  
  if (newSetSize > (Int32)maxNonEmptyEntrySize_) 
  { // if the new ColIndSet is longer than the maximal already inserted
    maxNonEmptyEntrySize_ = newSetSize;
    // at the last resize(), getSize() entries were allocated 
    // (it can be more than asked) -- so the last entry is 
    // getSize()-1 
    Int32 oldArrSize = getSize();
    // if the size of the new ColIndSet is greater than the number of 
    // the last entry allocated - resize self
    if (newSetSize > (oldArrSize - 1)) 
    {
      // the last entry number must be <newSetSize> , 
      // i.e. the number of entries must be <newSetSize + 1>
      resize(newSetSize + 1);
      
      // insert empty ColIndSetBuckets into the recently added entries
      size_t newArrSize = getSize();
      ColIndSetBucket *empty;
      
      for (size_t j = oldArrSize; j < newArrSize; j++) 
      {
	empty = new (getHeapPtr()) ColIndSetBucket(getHeapPtr(),j);
	insertAt(j, empty);
      }
    }
  }
  
  at(newSetSize)->insert(newSet);
  return *this;
}

//-----------------------------------------------------------------------------
// class: ColIndSetMatrix methods
//-----------------------------------------------------------------------------

//
//-------	ColIndSetMatrix::out
//---------------------------------------------------
// print method
//---------------------------------------------------------------------------//
//virtual 
#ifdef MJVIB_VERBOSE
void
ColIndSetMatrix::out()		const
{
  // print setsList_ out
  size_t size = setsList_.entries();
  
  printf("\n\tSetsList:\n");
  printf("\t---------\n");
  size_t i;
  for (i = 1; i < size; i++) {
    printf("\n\t%2d <--> ", i);
    setsList_.at(i).out();
    printf("\n");
  }
  
  // print the matrix out
  size = entries();
  printf("\n\tColIndSetMatrix:\n");
  printf("\t----------------\n");
  
  printf("\t#super");
  
  for (i = 1; i < size; i++) {
    printf("\t%2d", i);
  }
  
  printf("\n\t#sub");
  for (i = 0; i < entries(); i++) 
  {
    printf("\n\t%2d ->  ", i);

    EnumArray *current = at(i);
    for (CollIndex j=0; j<current->entries(); j++)
    {
      printf("%2d ", current->at(j));
    }
  }
};
#endif


//-------	ColIndSetMatrix::ColIndSetMatrix
//---------------------------------------------------
// ctor
//---------------------------------------------------------------------------//
ColIndSetMatrix::ColIndSetMatrix( const ColIndSetBucketVector& src, 
				  size_t size, 
				  CollHeap *heap) :
  ARRAY(EnumArray*)(heap, size + 1),
  currIndex_(0), 
  lastIndex_(size+1),
  setsList_(heap),
  settledList_(heap)
{
  size += 1;
  size_t i;
  EnumArray * currEnumArray;

  for (i = 0; i < size; i++) 
  {
    currEnumArray = new (heap) EnumArray(heap, size);
    CMPASSERT(NULL != currEnumArray);
    for (size_t j = 0; j < size; j++) {
      currEnumArray->insertAt(j, 0);
    }
    insertAt(i, currEnumArray);
  }
  
  setsList_.insertAt(0, NULL);
  settledList_.insertAt(0, FALSE);
  size_t k = 1;
  size_t currBucketSize;
  
  // fill the setsList_: enumeration list of ColIndSets
  for (i = src.getMaxNonEmptyEntrySize(); i > 0; i--) {
    currBucketSize = src.at(i)->entries();
    for (size_t j = 0; j < currBucketSize; j++)
    {
      // give order numbers to the sets:
      settledList_.insertAt(k, FALSE);
      setsList_.insertAt(k++, src.at(i)->at(j));
    }
  }
  
  ColIndSet currSet;
  
  // fill the matrix
  // The enumerations start from 1, since row #0 and column #0
  // are reserved for [super/sub]setsNumber.
  for (i = 1; i < size; i++) 
  {
    currSet = setsList_.at(i);
    for (size_t j = 1; j < i; j++) 
    {
      if (currSet.isSubsetOf(setsList_.at(j))) 
      {
		at(i)->insertAt(j, 1);
		at(0)->at(j)++;
		at(i)->at(0)++;
      }
    }
  }
  
  
  
};

//-------	ColIndSetMatrix::first
//---------------------------------------------------
// return the order number of the first unsettled ColIndSet
//---------------------------------------------------------------------------//
size_t 
ColIndSetMatrix::first()
{ 
  // starting from the absolutely first element
  // iterate until the first element, not marked as "settled"
  for (size_t i = 1; i < lastIndex_; i++) {
    if (!settledList_.at(i)) {
      currIndex_ = i;
      return i;
    }
  }
  // if above not found - return the lastIndex_ - kind of EOF
  currIndex_ = lastIndex_;
  return lastIndex_;
}

//-------	ColIndSetMatrix::next
//---------------------------------------------------
// iterator
//---------------------------------------------------------------------------//
size_t 
ColIndSetMatrix::next()
{ 
  // currIndex_ holds the last number referred;
  // starting from the next one (currIndex_+1)
  // iterate until the first element, not marked as "settled"
  for (size_t i = currIndex_+1; i < lastIndex_; i++) {
    if (!settledList_.at(i)) {
      currIndex_ = i;
      return i;
    }
  }

  // if above not found - return the lastIndex_ - kind of EOF
  currIndex_ = lastIndex_;
  return 0;
}


//-------	ColIndSetMatrix::setSettledByIndex
//---------------------------------------------------
// Logically -- mark ColIndSet #i as <settled> 
// (i.e. allready inserted into the matrix)
//---------------------------------------------------------------------------//
void 
ColIndSetMatrix::setSettledByIndex(size_t i)	
{ 
  settledList_.insertAt(i, TRUE);
  at(i)->at(0) = 0;
};


//-------	ColIndSetMatrix::pickTheBestUnsettled
//---------------------------------------------------
// Pick an unsettled ColIndSet with the minimum number of not <covered> yet
// supersets
//---------------------------------------------------------------------------//
size_t 
ColIndSetMatrix::pickTheBestUnsettled	()
{
  size_t  currMinSupersetsNum = entries(), 
	  currSupersetsNum, 
	  minSupersetsNumIndex = 0;
  
  for (currIndex_ = first(); currIndex_ > 0; currIndex_ = next()) 
  { // for each "unsettled" ColIndSet
    if ((currSupersetsNum = getSupersetsNum(currIndex_)) < currMinSupersetsNum) 
    { // update bestIndex, if the currColIndSet's supersets number 
      // is less than currMin
      currMinSupersetsNum = currSupersetsNum;
      minSupersetsNumIndex = currIndex_;
    }
  }
  return minSupersetsNumIndex;
};


//-------	ColIndSetMatrix::pickTheBestSettlement
//---------------------------------------------------
// Pick a not <covered> ColIndSet 
// (which is a superset for ColIndSet #toPushIndex) 
// with the minimum number of not <settled> yet subsets
//---------------------------------------------------------------------------//
size_t 
ColIndSetMatrix::pickTheBestSettlement	(size_t toPushIndex)
{
  size_t  currMinSubsetsNum = entries(), 
	  currSubsetsNum, 
	  minSubsetsNumIndex = 0;
  
  for (currIndex_ = 1; currIndex_ < lastIndex_; currIndex_++) 
  { // for each ColIndSet:
    if (  at(toPushIndex)->at(currIndex_) == 1 &&
	  (currSubsetsNum = getSubsetsNum(currIndex_)) < currMinSubsetsNum) 
    { // if it is the superset of the  ColIndSet #toPushIndex
      // and it's subsets number is less than currMin, update minIndex
      currMinSubsetsNum = currSubsetsNum;
      minSubsetsNumIndex = currIndex_;
    }
  }
  return minSubsetsNumIndex;
};
//-------	ColIndSetMatrix::update
//---------------------------------------------------
// Update the matrix, i.e all its entries, that changed after inserting 
// a ColIndSet, covering the ColIndSet #index
//---------------------------------------------------------------------------//
void  
ColIndSetMatrix::update (size_t index)
{
  // for each "unsettled" ColIndSet in the matrix do:
  for (currIndex_ = 1; currIndex_ > 0 ;currIndex_ = next()) 
  {
    if (at(currIndex_)->at(index) == 1) 
    {// if ColIndSet #currIndex_ is-subset-of ColIndSet #index
      // remove the reation "is-subset-of" off future consideration
      at(currIndex_)->insertAt(index, 0); 
      // decrease the number of subsets of the ColIndSet #index
      at(0)->at(index)--;
      // decrease the number of supersets of the ColIndSet #currIndex_
      at(currIndex_)->at(0)--;
    }
  }
  // mark the ColIndSet #index as "covered"
  at(0)->at(index) = 0;
}

//-----------------------------------------------------------------------------
// class: NestingStack methods
//-----------------------------------------------------------------------------


//
//-------	NestingStack::out
//---------------------------------------------------
// print method
//---------------------------------------------------------------------------//
//virtual 
#ifdef MJVIB_VERBOSE
void
NestingStack::out()		const
{
  size_t size = entries();
  for (size_t i = 0; i < size; i++) {
    at(i)->out();
    if (i < size-1) {
      printf(" --> ");
    }
  }
};
#endif

//-------	NestingStack::top
//---------------------------------------------------
// return top
//---------------------------------------------------------------------------//
ColIndSet& 
NestingStack::top()
{
  return  *at(entries() - 1);
}

//
//-------	NestingStack::isSubsetOfTop
//---------------------------------------------------
// returns TRUE, if newSet is a subset of top; FALSE otherwise
//---------------------------------------------------------------------------//
NABoolean 
NestingStack::isSubsetOfTop (const ColIndSet& newSet)
{
	if (0 == entries()) {
		return TRUE;
	}
	return newSet.isSubsetOf(top());
}

//-------	NestingStack::push
//---------------------------------------------------
// push newSet
//---------------------------------------------------------------------------//
NestingStack& 
NestingStack::push(const ColIndSet& newSet)
{
  CMPASSERT(isSubsetOfTop(newSet));
  insertAt(entries(), new (getHeapPtr()) ColIndSet(newSet, getHeapPtr()));
  return *this;
}

//
//-------	NestingStack::buildIndex
//---------------------------------------------------
// builds index, containing each of the ColIndSet 
// (or their permutation) as a prefix
// The method is called at the end of the algorithm -- from MJVCook - to
// produce the output.
//---------------------------------------------------------------------------//
void 
NestingStack::buildIndex(ColIndList& result)
{
  ColIndSet	*last;
  ColIndSet	currSet;
  size_t	currSetSize;
  Lng32		currColumnNumber;
  
  while (!isEmpty()) // while there is a ColIndSet in the NestingStack
  { 
    getLast(last); // removes the last (the smallest) ColIndSet
    
    if (entries() > 0) 
    { // if there are more ColIndSets, 
      // currSet contains the columns, which are common for the 
      // recently removed set and the bottom (at(0) - the largest) one
      // Example:
      // for the NestingStack: (2,4)->(7,2,5,6,4)->(1,2,3,4,5,6,7)
      // that during the SECOND iteration will look like:
      //			      (7,2,5,6,4)->(1,3,5,6,7)
      // the <currSet> will be:
      //			      <7,5,6>
      // It is reasonable, because on this iteration these are the columns 
      // we want to insert (the columns <2,4> were already inserted into 
      // index and removed from the bottom ColIndSet at the end of 
      // the FIRST iteration)
      currSet = last->intersect(*at(0));
    } else {
      currSet = *last;
    }
    currSetSize = currSet.entries();
    // insert the column numbers from the <currSet> into the index (<result>)
    for (size_t i = 0; i < currSetSize; i++) 
    {
      if (entries() > 0) { 
	currColumnNumber = currSet.at(i);
	result.insert(currColumnNumber);
	// remove already inserted element from the bottom ColIndSet
	at(0)->remove(currColumnNumber);
      } else { 
	result.insert(last->at(i));
      }
    }
  }
}

//-----------------------------------------------------------------------------
// class: MJVIndCook methods
//-----------------------------------------------------------------------------

//
//-------	MJVIndCook::out
//---------------------------------------------------
// print method
//---------------------------------------------------------------------------//
//virtual 
#ifdef MJVIB_VERBOSE
void
MJVIndCook::out()		const
{
  size_t size = entries();
  printf("\n\tMJVIndCook:\n");
  printf("\t-----------\n");
  for (size_t i = 0; i < size; i++) {
    at(i)->out();
    if (i < size-1) {
      printf("\n |\n +\n");
    }
  }
};
#endif


//
//-------	MJVIndCook::insert
//---------------------------------------------------
// inserts newSet either into any of stacks, accepting it, 
// or in a new created stack, if nobody accepted the newSet
//---------------------------------------------------------------------------//
MJVIndCook& 
MJVIndCook::insert(const ColIndSet& newSet)
{
  size_t numOfStacks = entries();
  for (size_t i = 0; i < numOfStacks; i++) {
    if (at(i)->isSubsetOfTop(newSet)) 
    {
      at(i)->push(newSet);
      return *this;
    }
  }
  return addNewNestingStack(newSet);
}

//
//-------	MJVIndCook::insertOnto
//---------------------------------------------------
// inserts newSet either into self; exactly -
// onto the specified ColIndSet setToBeCovered. 
//---------------------------------------------------------------------------//
MJVIndCook&  
MJVIndCook::insertOnto(	const ColIndSet& setToBeCovered, 
			const ColIndSet& newSet)
{
  
  size_t numOfStacks = entries();
  for (size_t i = 0; i < numOfStacks; i++) {
    if (at(i)->top() == setToBeCovered) 
    {
      CMPASSERT(at(i)->isSubsetOfTop(newSet));
      at(i)->push(newSet);
      return *this;
    }
  }
  return addNewNestingStack(newSet);
}


//
//-------	MJVIndCook::addNewNestingStack
//---------------------------------------------------
// add new NestingStack,containing only newSet, into MJVIndCook
//---------------------------------------------------------------------------//
MJVIndCook& 
MJVIndCook::addNewNestingStack(const ColIndSet& newSet)
{
  NestingStack *newStack = new (getHeapPtr()) NestingStack(getHeapPtr());
  newStack->push(newSet);
  insertAt(entries(), newStack);
  return *this;
}

//
//-------	MJVIndCook::buildIndex
//---------------------------------------------------
// builds all the indexes -- each NestingStack builds itself
//---------------------------------------------------------------------------//
IndexList*
MJVIndCook::buildIndex()
{
  IndexList *resultList = new (getHeapPtr()) IndexList(getHeapPtr());
  ColIndList currResult;
  
  size_t numOfStacks = entries();
  
  for (size_t i = 0; i < numOfStacks; i++) 
  {
    at(i)->buildIndex(currResult);
    resultList->insert(currResult);
    currResult.clear();
  }
  return resultList;
}

//-----------------------------------------------------------------------------
// class: MJVIndexBuilder methods
//-----------------------------------------------------------------------------
//
//-------	MJVIndexBuilder::buildIndex
//---------------------------------------------------
//	 The algorithm flow itself.
//		1) Initializes ColIndSetBucketVector from <code>inputList_</code>
//		2) Initializes MJVIndMatrix from ColIndSetBucketVector
//		2) Builds MJVIndCook out from MJVIndMatrix
//		3) Calls buildIndex() for MJVIndCook built
//---------------------------------------------------------------------------//
IndexList *
MJVIndexBuilder::buildIndex(const IndexList inputList)
{
#ifdef MJVIB_VERBOSE
  PrintColIndList (inputList, "inputRCIList");
#endif

  size_t inputSize = inputList.entries() - 1 ; // for MVCI
  if (inputSize < 1) {
    return new (getHeapPtr()) IndexList(getHeapPtr());
  }
  
  ColIndSetBucketVector buckets(getHeapPtr());
  MJVIndCook indexCook(getHeapPtr());
  
  
  // insert each list into ColIndSetBucketVector
  ColIndList mvCI = inputList[0];
  
  for (size_t i = 1; i < inputSize+1; i++) {
    if (!inputList[i].isPrefixOf(mvCI)) {
      ColIndSet currRCI = ColIndSet(inputList[i], getHeapPtr());
      buckets.insert(currRCI);
    }
  }
  
#ifdef MJVIB_VERBOSE
  // debug print
  buckets.out();
#endif
  
  // Initialize the colIndSetsRelationsMatrix matrix
  size_t matrixSize = buckets.getNumberOfEntities();
  ColIndSetMatrix colIndSetsRelationsMatrix(buckets, matrixSize, getHeapPtr());
  
#ifdef MJVIB_VERBOSE
  // debug print
  colIndSetsRelationsMatrix.out();
#endif
  
  size_t numSettled = 0;
  size_t currSetInd, currSuperSetInd;
  
  // fill the colIndSetsRelationsMatrix matrix
  while (numSettled < matrixSize)
  {
    size_t currIndex;
    
    // choose unsettled with #super = 0
    // and insert it into a new NestingStack
    while (!colIndSetsRelationsMatrix.endOfMatrix())	
    {
      if (0 == (currIndex = colIndSetsRelationsMatrix.next())) {
		break;
      }
      
      if(colIndSetsRelationsMatrix.getSupersetsNum(currIndex) == 0) 
      {
		indexCook.insert(colIndSetsRelationsMatrix.getSetByIndex(currIndex));
		colIndSetsRelationsMatrix.setSettledByIndex(currIndex);
		numSettled++;
      };
    }	
    // if we've settled all the sets -- no need to proceed
    if (numSettled == matrixSize) {
      continue;
    }
    
    // choose the one of unsettled with the minimal #super
    // and insert it onto the one with minimal #sub 
    currSetInd	    = colIndSetsRelationsMatrix.pickTheBestUnsettled();
    currSuperSetInd = colIndSetsRelationsMatrix.pickTheBestSettlement(currSetInd);
    
    indexCook.insertOnto (colIndSetsRelationsMatrix.getSetByIndex(currSuperSetInd), 
			  colIndSetsRelationsMatrix.getSetByIndex(currSetInd));
    
    colIndSetsRelationsMatrix.setSettledByIndex(currSetInd);
    numSettled++;
    
    // update the colIndSetsRelationsMatrix matrix
    colIndSetsRelationsMatrix.update(currSuperSetInd);
    
  }

  IndexList *outputRCIList = indexCook.buildIndex();
  
#ifdef MJVIB_VERBOSE
  // debug print
  indexCook.out();
  printf("\n\n");
  PrintColIndList (*outputRCIList, "outputRCIList");
#endif
  
  return outputRCIList;
}


//*
//*-------	PrintColIndList
//*---------------------------------------------------
#ifdef MJVIB_VERBOSE
void PrintColIndList (const IndexList& toPrint, char* name)
{
  printf ("\n%s:\n",name);
  printf ("--------------------------\n");
  for (size_t i = 0; i < toPrint.entries(); i++) 
  {
    ColIndList currList = toPrint.at(i);	
    size_t size = currList.entries();
    printf(OPEN_LIST);
    for (size_t j = 0; j < size; j++) {
      printf("%d", currList.at(j));
      if (j < size-1) {
	printf(", ");
      }
    }
    printf(CLOSE_LIST);
    printf("\n");
  }
}
#endif // MJVIB_VERBOSE
