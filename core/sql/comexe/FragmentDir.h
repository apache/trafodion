/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2014 Hewlett-Packard Development Company, L.P.
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
#ifndef GEN_FRAGMENT_DIR_H
#define GEN_FRAGMENT_DIR_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         GenFragmentDir.h
 * Description:  Fragment directory for a generated statement. A fragment
 *               is a part of a plan executed in one process. Plans that use
 *               ESPs have multiple fragments. This file also defines
 *		 a directory describing the needed resources for each ESP.
 *               
 * Created:      12/4/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "Collections.h"

// -----------------------------------------------------------------------
// Contents of this file
// -----------------------------------------------------------------------
class FragmentDirEntry;
class FragmentDir;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class PartitioningFunction;

// -----------------------------------------------------------------------
// Entry of a fragment directory, containing info on the generated
// fragment (offsets, lengths, ...). Note that this class itself (not
// a pointer to it) is used as a collection member in class FragmentDir.
// -----------------------------------------------------------------------
class FragmentDirEntry : public NABasicObject
{
  // class FragmentDir is the only class that can use this object, since
  // it has no public accessor functions (could have made this a local
  // struct but some C++ compilers have a problem with that)
  friend class FragmentDir;

public:
  // public default constructor needed in order to create arrays of entries
  FragmentDirEntry() ;

  // comparison operator to make collection template happy
  inline NABoolean operator == (const FragmentDirEntry &other) const
                                        { return space_ == other.space_; }

private:
  Int32                        type_;
  Space                      *space_;
  CollIndex                  parentIndex_;
  char                       *topNode_;
  Lng32                       numESPs_;
  const PartitioningFunction *partFunc_;
  Lng32                       partInputDataLength_;
  NABoolean                  needsTransaction_;
  // a flag used only during codegen.
  NABoolean                  containsPushedDownOperators_;
  unsigned short             numBMOs_;
  double                     BMOsMemoryUsage_;

  // level of esp layer relative to root node. First esp layer is 1.
  Lng32                       espLevel_;
  NABoolean                  soloFragment_;
};

// -----------------------------------------------------------------------
// A global array containing the information for all fragments of a
// plan.
// -----------------------------------------------------------------------
class FragmentDir : public NABasicObject
{
public:

  enum FragmentTypeEnum { MASTER = 1,
			  DP2    = 2,
			  ESP    = 3,
			  EXPLAIN = 4};

  // default constructor
  FragmentDir(CollHeap * Heap);

  // destructor deletes all space objects that were used
  ~FragmentDir();

  // how many entries does the directory have
  inline CollIndex entries() const          { return entries_->entries(); }

  // create a new fragment as child of the current one and return its id
  CollIndex pushFragment(
       FragmentTypeEnum           type,
       Lng32                       numESPs = 0,
       const PartitioningFunction *partFunc = NULL,
       Lng32                       partInputDataLength = 0);

  // leave a created fragment and return to its parent fragment
  // (must call setTopObj() before doing this), return parent fragment id
  CollIndex popFragment();

  // removes the last fragment from the fragment dir.
  void removeFragment();

  // The top level index maintenance nodes belong in the master/esp 
  // even though they are generated in update (dp2 fragment). Thus, 
  // we must be able to switch back to the parent fragment temporarily.
  // See GenRelUpdate.C for usage example...
  //
  CollIndex setCurrentId(CollIndex currentId) {
    CollIndex oldId = currentFragmentId_;
    currentFragmentId_= currentId;
    return(oldId);
  }

  // get the "Space" object for the current fragment or for any fragment
  inline Space *getCurrentSpace() const
                           { return (*entries_)[currentFragmentId_]->space_; }
  inline Space *getSpace(CollIndex ix) const {return (*entries_)[ix]->space_;}

  // set the pointer to the generated top object for the current fragment
  inline void setTopObj(char *obj)
                          { (*entries_)[currentFragmentId_]->topNode_ = obj; }
  // Warning: know what you are doing when using this, it's not always valid!
  // This method is put here based on the code change on R2 path, it is 
  // needed for gui executor display.
  inline char * getTopObj(CollIndex ix)  { return (*entries_)[ix]->topNode_; }

  // set the pointer to the generated top object for the current fragment
  inline void setTopObj(CollIndex fragIndex, char *obj)
                          { (*entries_)[fragIndex]->topNode_ = obj; }

  // get the type of a fragment
  inline FragmentTypeEnum getType(CollIndex ix) const
                         { return (FragmentTypeEnum) (*entries_)[ix]->type_; }

  // get the generated length for a given fragment (use this proc only
  // for obtaining fragment length, since it rounds up the length indicated
  // by the Space object to the nearest multiple of 8)
  Lng32 getFragmentLength(CollIndex ix) const;

  // get the parent id of a fragment
  inline CollIndex getParentId() const 
                     { return (*entries_)[currentFragmentId_]->parentIndex_; }
  inline CollIndex getParentId(CollIndex ix) const
                                     { return (*entries_)[ix]->parentIndex_; }

  // get the id of the current fragment
  inline CollIndex getCurrentId() const { return currentFragmentId_; }

  // get a pointer to the top node of a fragment
  inline char * getTopNode(CollIndex ix) const
                                         { return (*entries_)[ix]->topNode_; }

  inline Lng32 getNumESPs(CollIndex ix) const
                                         { return (*entries_)[ix]->numESPs_; }

  inline Lng32 getEspLevel(CollIndex ix) const
                                         { return (*entries_)[ix]->espLevel_; }
  inline void setEspLevel(CollIndex ix, Lng32 level)
  { (*entries_)[ix]->espLevel_ = level; }

  inline const PartitioningFunction *
  getPartitioningFunction(CollIndex ix) const
                                        { return (*entries_)[ix]->partFunc_; }

  inline Lng32 getPartInputDataLength(CollIndex ix) const
                             { return (*entries_)[ix]->partInputDataLength_; }
  inline void setPartInputDataLength(CollIndex ix, Lng32 l)
                                { (*entries_)[ix]->partInputDataLength_ = l; }

  inline NABoolean getNeedsTransaction(CollIndex ix) const
                                { return (*entries_)[ix]->needsTransaction_; }
  inline void setNeedsTransaction(CollIndex ix, NABoolean n = TRUE)
                                   { (*entries_)[ix]->needsTransaction_ = n; }

  inline NABoolean getContainsPushedDownOperators(CollIndex ix) const
                                { return (*entries_)[ix]->containsPushedDownOperators_; }
  inline void setContainsPushedDownOperators(CollIndex ix, NABoolean n = TRUE)
                                   { (*entries_)[ix]->containsPushedDownOperators_= n; }

  inline NABoolean getSoloFragment(CollIndex ix) const
                                { return (*entries_)[ix]->soloFragment_; }
  inline void setSoloFragment(CollIndex ix, NABoolean n = TRUE)
                                   { (*entries_)[ix]->soloFragment_ = n; }

  void setAllEspFragmentsNeedTransaction();

  // get the generated length of all fragments together
  Lng32 getTotalLength() const;

  inline void setNumBMOs(CollIndex ix, unsigned short num)
  { (*entries_)[ix]->numBMOs_ = num; }
  inline unsigned short getNumBMOs()
  { return (*entries_)[currentFragmentId_]->numBMOs_ ; }
  inline unsigned short getNumBMOs(CollIndex ix)
  { return (*entries_)[ix]->numBMOs_ ; }

  inline void setBMOsMemoryUsage(CollIndex ix, double memInMB)
  { (*entries_)[ix]->BMOsMemoryUsage_ = memInMB; }
  inline double getBMOsMemoryUsage()
  { return (*entries_)[currentFragmentId_]->BMOsMemoryUsage_; }
  inline double getBMOsMemoryUsage(CollIndex ix)
  { return (*entries_)[ix]->BMOsMemoryUsage_; }

  NABoolean containsESPLayer();

private:
  CollHeap * heap_;
  NAArray<FragmentDirEntry*> * entries_;
  CollIndex currentFragmentId_;
  NABoolean allEspFragmentsNeedTransaction_;
};

#endif /* GEN_FRAGMENT_DIR_H */
