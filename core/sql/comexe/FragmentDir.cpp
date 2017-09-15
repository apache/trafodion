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
 *****************************************************************************
 *
 * File:         GenFragmentDir.C
 * Description:  Methods for fragment directory
 *               
 *               
 * Created:      12/6/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "FragmentDir.h"
//#include "Generator.h"
#include "ComSpace.h"

// -----------------------------------------------------------------------
// forward declarations
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// Methods for class FragmentDir
// -----------------------------------------------------------------------


FragmentDir::FragmentDir(CollHeap * heap)
     : heap_(heap)
{
  currentFragmentId_              = NULL_COLL_INDEX;
  allEspFragmentsNeedTransaction_ = FALSE;

  entries_ = new(heap_) NAArray<FragmentDirEntry*>(heap_);
}

FragmentDir::~FragmentDir()
{
  // before getting rid of the array, delete all the Space objects
  // that are owned by the directory entries. NOTE: don't delete the
  // space objects in FragmentDirEntry::~FragmentDirEntry, since the
  // ARRAY collection copies and moves entries around which causes the
  // destructor to be called when such a deletion would be wrong.
  for (CollIndex i = 0; i < entries(); i++)
    if (heap_ 
       ) {
      // first free blocks allocated by this space
      (*entries_)[i]->space_->freeBlocks();

      // now deallocate the Space object itself. We need this two step
      // approach, because Space is not derived from NABasicObject
      heap_->deallocateMemory((*entries_)[i]->space_);      
    }	
    else
      delete (*entries_)[i]->space_;
}

CollIndex FragmentDir::pushFragment(FragmentTypeEnum            type,
				    Lng32                        numESPs,
				    const PartitioningFunction  *partFunc,
				    Lng32                        partInputDataLength)
{
  CollIndex newIndex = entries(); // assume a dense array
  FragmentDirEntry * fde = new(heap_) FragmentDirEntry();
  //  (*entries_).insertAt(newIndex,FragmentDirEntry());
  entries_->insertAt(newIndex, fde);
  FragmentDirEntry *newEntry = (*entries_)[newIndex];

  // now fill the new entry with stuff
  newEntry->type_        = type;
  
  // create new space manager for fragment
  newEntry->space_       = new(heap_) Space(Space::GENERATOR_SPACE);
  newEntry->space_->setParent(heap_); // was NAHeap*-casted, but this has MI-problems

  newEntry->parentIndex_ = currentFragmentId_;
  newEntry->topNode_     = NULL;       // will be set later
  newEntry->numESPs_     = numESPs;
  newEntry->partFunc_    = partFunc;
  newEntry->partInputDataLength_ = partInputDataLength;
  newEntry->needsTransaction_ = FALSE;
  newEntry->containsPushedDownOperators_ = FALSE;
  newEntry->soloFragment_ = FALSE;

  // check whether we already know that we need a transaction
  if (type == ESP && allEspFragmentsNeedTransaction_)
    setNeedsTransaction(newIndex);

  // the newly created fragment becomes the current one
  currentFragmentId_ = newIndex;
  return currentFragmentId_;
}

CollIndex FragmentDir::popFragment()
{
  // although the method is called "pop", don't destroy the entry
  // in the fragment directory, just return the index of its parent fragment
  currentFragmentId_ = (*entries_)[currentFragmentId_]->parentIndex_;
  //  NAAssert(currentFragmentId_ < entries(),"NOT currentFragmentId_ < entries()");
  assert(currentFragmentId_ < entries());
  return currentFragmentId_;
}

void FragmentDir::removeFragment()
{
  CollIndex lastEntry = entries_->entries() - 1;

  if (heap_ 
     ) {
    // first call destructor to free blocks allocated by this space
    (*entries_)[lastEntry]->space_->~Space();
    // now deallocate the Space object itself. We need this two step
    // approach, because Space is not derived from NABasicObject
    heap_->deallocateMemory((*entries_)[lastEntry]->space_);      
  }	
  else
    delete (*entries_)[lastEntry]->space_;

  currentFragmentId_ = (*entries_)[lastEntry]->parentIndex_;

  entries_->remove(lastEntry);
}

Lng32 FragmentDir::getFragmentLength(CollIndex ix) const
{
  Lng32 result = (*entries_)[ix]->space_->getAllocatedSpaceSize();

  // We want to align each fragment on an 8 byte boundary, so round
  // up the length to the next multiple of 8
  result = ((result-1)/8 + 1) * 8;

  return result;
}

void FragmentDir::setAllEspFragmentsNeedTransaction()
{
  // remember this for future fragments...
  allEspFragmentsNeedTransaction_ = TRUE;
  // ...and update all existing fragments
  for (CollIndex i = 0; i < entries(); i++)
    if (getType(i) == ESP)
      setNeedsTransaction(i);
}

Lng32 FragmentDir::getTotalLength() const
{
  Lng32 result = 0;
  for (CollIndex i = 0; i < entries(); i++)
    result += getFragmentLength(i);
  return result;
}

NABoolean FragmentDir::containsESPLayer()
{
   for (CollIndex i = 0; i < entries(); i++) 
     if (getPartitioningFunction(i) != NULL && getType(i) == FragmentDir::ESP)
     {
        return TRUE;
     }

   return FALSE;
}


FragmentDirEntry::FragmentDirEntry() 
			      { type_                = 99; 
                              space_               = NULL;
                              parentIndex_         = 0;
                              topNode_             = NULL;
                              numESPs_             = 0;
			      espLevel_            = 0;
                              partFunc_            = NULL;
                              partInputDataLength_ = 0;
			      numBMOs_              = 0;
                              BMOsMemoryUsage_      = 0;
                              needsTransaction_    = FALSE;
                              soloFragment_        = FALSE;
 }

