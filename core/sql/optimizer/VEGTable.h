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
#ifndef VEGTABLE_H
#define VEGTABLE_H
/* -*-C++-*-
******************************************************************************
*
* File:         ValueId Equality Group
* Description:  A set of ValueIds that have an equality relationship
*               between its elements.
* Created:      11/14/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

// -----------------------------------------------------------------------

#include "Collections.h"
#include "RelSet.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class VEGMember;
class VEGRegion;
class VEGTable;

// -----------------------------------------------------------------------
// Forward references
// -----------------------------------------------------------------------
class ItemExpr;
class VEG;
class VEGReference;

// -----------------------------------------------------------------------
// Declarations.
// -----------------------------------------------------------------------
typedef Lng32  RegionId; 

const RegionId NULL_VEG_REGION = -1;
const RegionId FIRST_VEG_REGION = 0;

/////////////////////////////////////////////////////////////////

template <class T> class RAList
{

public:

  // default constructor
 // RAList(CollIndex initLen = 0) : 
  //  theList_(initLen), 
  //  theArray_(new initLen),
  //  arrayValid_(FALSE)
 // {}


  // constructor with user-defined heap
  RAList(CollHeap * heap,
          CollIndex initLen = 0) :
    theList_(heap,initLen),
    theArray_(new(heap) ARRAY(CollIndex) (heap,initLen)),
    arrayValidUpTo_(new(heap) CollIndex(NULL_COLL_INDEX))
  {}


  // copy ctor
  RAList(const RAList<T> &other, CollHeap * heap=0) : 
    theList_(other.theList_, heap),
    theArray_(new(heap) ARRAY(CollIndex) (*(other.theArray_),heap)),
    arrayValidUpTo_(new(heap) CollIndex(*(other.arrayValidUpTo_)))
  {}

  // virtual destructor
  // oops I need to delete theArray_ and arrayValid_, but since I dont 
  // have time to run regression again and RAList is only used within
  // CmpStatementHeap so far, then I will delay this to next checkin
  virtual ~RAList() {};

  // assignment
  RAList<T> & operator =(const RAList<T> &other);

  // comparison
  NABoolean operator ==(const RAList<T> &other) const;

  inline CollIndex entries() const
  {  return theList_.entries();}


  // insert a new entry
  inline CollIndex insertAt(const CollIndex ix, const T &elem) 
  { 
    CollIndex newIndex = theList_.insertAt(ix,elem);

    if (*arrayValidUpTo_ == NULL_COLL_INDEX)
    {
      // do nothing for now// may be wana consider ix = 0 refinement
    }
    else if (ix <= (*arrayValidUpTo_) + 1)
    {
      theArray_->insertAt(ix,newIndex);
      *arrayValidUpTo_ = ix;
    }  

   return newIndex;
  }


  // append a new entry
  inline void insert(const T &elem) 
  { 
    insertAt(entries(), elem);
  }


  // remove an element(the first that matches) that is given by its value
  //(returns whether the element was found and removed)
  inline NABoolean remove(const T &elem)
  { 
    if (theList_.remove(elem))
    {
      // It does not worth it to use the find function to refine this 
      // because its more expensive. Would've been nice to have an NAList
      // function that returns the removed element index (-1 for FALSE).
      *arrayValidUpTo_ = NULL_COLL_INDEX;                              
      return TRUE;
    }
    else
      return FALSE;
  }
    
  // index access(both reference and value), zero based
  T & operator [](CollIndex ix)
  {
    if ((*arrayValidUpTo_==NULL_COLL_INDEX) || (ix > (*arrayValidUpTo_)))
    {
      // rebuild and validate the array
      theArray_->clear();     

      // This is not an n^2 algo its n because of the cache in
      // the NAList theList_
      CollIndex count = entries();
      for (CollIndex i = 0; i < count; i++)
      {
        theArray_->insert(i,theList_.findArraysIndex(i));
      }
      *arrayValidUpTo_ = count;
    }
    return theList_.usedEntry((*theArray_)[ix]);
  }


  const T & operator [](CollIndex ix) const
  {
    if ((*arrayValidUpTo_==NULL_COLL_INDEX) || (ix > (*arrayValidUpTo_)))
    {
      // rebuild and validate the array
      theArray_->clear();     

      // This is not an n^2 algo its n because of the cache in
      // the NAList theList_
      CollIndex count = entries();
      for (CollIndex i = 0; i < count; i++)
      {
        theArray_->insert(i,theList_.findArraysIndex(i));
      }
      *arrayValidUpTo_ = count;
    }
    return theList_.constEntry((*theArray_)[ix]);
  }

  inline T & at(CollIndex i) { return operator [](i); }
  const T & at(CollIndex i) const { return operator [](i); }

  private:

  LIST(T) theList_;
  ARRAY(CollIndex)* theArray_;
  CollIndex* arrayValidUpTo_;
  
}; // NAArray


/////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------
// A VEGRegion can either be be IMPORT_ONLY or IMPORT_AND_EXPORT.
// IMPORT_ONLY => 
//   A value that is produced in such a VEGRegion cannot be referenced 
//   in another VEGRegion. However, such a VEGRegion can reference a
//   value that is a member of another VEGRegion, i.e., it is an "outer
//   reference".
//   For example, a VEGRegion that is allocated for a scalar aggregate
//   can only import a VEG from another VEGRegion:
//   select ... 
//     from t1
//    where t1.x = 10
//      and t1.y = any (select t3.x from t3 where t3.x = t1.x )
//   It is legal to deduce that t3.x = 10 within the subquery but not
//   t1.y = t3.x = 10 = t1.x in the main query.
//   Similarly, a VEGRegion that is allocated for an "=" predicate that 
//   occurs underneath an OR operator is also IMPORT_ONLY. In all other
//   cases, the VEGRegion is of the type IMPORT_AND_EXPORT.
// IMPORT_AND_EXPORT =>
//   A value that is produced in such a VEGRegion can also be referenced
//   in another VEGRegion. 
// -----------------------------------------------------------------------
enum VEGRegionTypeEnum  { IMPORT_ONLY, EXPORT_ONLY, IMPORT_AND_EXPORT };
  
// ***********************************************************************
// VEGMember : A Member of a ValueId Equality Group
//
// A VEGMember is allocated for each expression that is a child of an
// "=" operator in a predicate.
// ***********************************************************************
class VEGMember : public NABasicObject
{
public:
                                                        
  // ---------------------------------------------------------------------
  // Constructor functions.
  // ---------------------------------------------------------------------
  VEGMember(const ValueId & memberValue, const ValueId & ofVEG,
            NABoolean deleteMeFlag = FALSE, NABoolean forwardingEntry = FALSE) 
         : memberM_(memberValue), groupG_(ofVEG), 
           deleteMeFlag_(deleteMeFlag), forwardingEntry_(forwardingEntry) 
  {} 

  // ---------------------------------------------------------------------
  // Destructor function.
  // ---------------------------------------------------------------------
  ~VEGMember() {}

  // ---------------------------------------------------------------------
  // Accessor functions.
  // ---------------------------------------------------------------------
  void setMemberValueId(const ValueId newId)         { memberM_ = newId; } 
  ValueId  getMemberValueId() const                   { return memberM_; }
  ItemExpr * getItemExpr() const        { return memberM_.getItemExpr(); }

  ValueId getVEGValueId() const                        { return groupG_; }
  VEG * getVEG() const          { return (VEG *)(groupG_.getItemExpr()); }
  
  // ---------------------------------------------------------------------
  // Standard operators.
  // ---------------------------------------------------------------------
  NABoolean operator==(const VEGMember & other) const;

  // ---------------------------------------------------------------------
  // Mutator functions.
  // ---------------------------------------------------------------------
  void setVEGValueId(const ValueId & newVEG)         { groupG_ = newVEG; }
				     
  // ---------------------------------------------------------------------
  // Methods for testing and setting the state of the deleteMeFLag_.
  // ---------------------------------------------------------------------
  void markAsToBeDeleted()                       { deleteMeFlag_ = TRUE; }
  NABoolean isToBeDeleted() const                { return deleteMeFlag_; }
  
  // ---------------------------------------------------------------------
  // Methods for testing and setting the state of the deleteMeFLag_.
  // ---------------------------------------------------------------------
  void markAsAForwardingEntry()               { forwardingEntry_ = TRUE; }
  NABoolean isAForwardingEntry() const        { return forwardingEntry_; }
  
  // ---------------------------------------------------------------------
  // Print 
  // ---------------------------------------------------------------------
  void display();

  void print( FILE* ofd = stdout,
	      const char* indent = DEFAULT_INDENT,
              const char* title = "VEGMember");

private:
  
  // *********************************************************************
  // VEGMember - private methods
  // *********************************************************************

  // *********************************************************************
  // VEGMember - private data
  // *********************************************************************

  // ---------------------------------------------------------------------
  // The VEG is a set that contains ValueIds that are equal to each 
  // other. It is shared by all the VEGMembers that are allocated for 
  // values that belong to the VEG.
  // The VEG is allocated in the top-down tree walking phase of 
  // transformNode(). At the end of its bottom-up tree walking phase,
  // transformNode() may merge two or more VEG sets into a single
  // one. This happens when they are found to share the same member.
  // Thus, when the transformNode() phase is completed, all the  
  // ValueIds that are transitively related because of an "="
  // predicate between them belong to the same VEG.
  // ---------------------------------------------------------------------
  
  // ---------------------------------------------------------------------
  // This is a VEGMember for a member M in Region R.
  // The given value can be "=" to different values in different Regions.
  // ---------------------------------------------------------------------
  ValueId   memberM_;        // the L or R child of an "="
  
  // ---------------------------------------------------------------------
  // The ValueId Equality Group G, that is, the VEG.
  // ---------------------------------------------------------------------
  ValueId  groupG_;         // ValueId of a VEG Item Expression

  // ---------------------------------------------------------------------
  // A flag that indicates whether this member is to be deleted.
  // ---------------------------------------------------------------------
  NABoolean  deleteMeFlag_;  
  
  // ---------------------------------------------------------------------
  // A flag that indicates wether this member exists simply to redirect
  // the lookup to a certain VEG. However, memberM_ does NOT belong to 
  // groupG_. Such a member is created when an InstantiateNull is 
  // replaced with its operand when the rewrite of a Left Join to an
  // Inner Join is performed. 
  // ---------------------------------------------------------------------
  NABoolean  forwardingEntry_;  
  
}; // class VEGMember

// ***********************************************************************
// VEGRegion :
// ***********************************************************************

// -----------------------------------------------------------------------
// An entry in a table that maintains ValueId Equality Groups (VEGs)
// -----------------------------------------------------------------------
class VEGRegion  : public NABasicObject
{
public:

  VEGRegion(VEGTable * vegTable, 
	    VEGRegion * parentVEGRegionPtr, 
	    const RegionId newRegionId,
            enum VEGRegionTypeEnum tev, 
	    const ExprNode * const ownerExpr, 
	    Lng32  subtreeId = 0
	    )
    : stateEnum_(ACTIVE_STATE), typeEnum_(tev), processedFlag_(FALSE),
      parentRegion_(parentVEGRegionPtr),  myRegionId_(newRegionId),
      ownerExpr_(ownerExpr), subtreeId_(subtreeId), vegTable_(vegTable),
      zones_(CmpCommon::statementHeap()), 
      members_(CmpCommon::statementHeap())
  {}

  ~VEGRegion() { }
  
  // ---------------------------------------------------------------------
  // Methods for querying the type of the Region
  // ---------------------------------------------------------------------
  VEGRegionTypeEnum getVEGRegionTypeEnum() { return typeEnum_; }
  
  // ---------------------------------------------------------------------
  // Methods for querying the state which this VEGRegion is in.
  // ---------------------------------------------------------------------
  NABoolean isActive() const 
                                  { return (stateEnum_ == ACTIVE_STATE); }
  NABoolean isToBeMerged() const 
                            { return (stateEnum_ == TO_BE_MERGED_STATE); }
  NABoolean isMerged() const 
                                  { return (stateEnum_ == MERGED_STATE); }

  NABoolean processingDone() const              { return processedFlag_; }


  // ---------------------------------------------------------------------
  // Mark a VEGRegion, "To be merged".
  // ---------------------------------------------------------------------
  void markAsToBeMerged() 
  { 
    CMPASSERT(stateEnum_ != MERGED_STATE); 
    stateEnum_ = TO_BE_MERGED_STATE; 
  }

  // ---------------------------------------------------------------------
  // Add an entry for each expression of the ValueId Equality Group (VEG)
  // ---------------------------------------------------------------------
  void addVEG(const ValueId & expr1Id, const ValueId & expr2Id);
  
  // ---------------------------------------------------------------------
  // VEGRegion::addVEG()
  // ---------------------------------------------------------------------
  void addVEG(const ValueIdSet & setOfValues);

  // ---------------------------------------------------------------------
  // Add a new entry for memberId if one doesn't already exist. If the
  // VEG of memberId exists, then merge the two VEGs.
  // If the memberId already exists, then merge the memberId's VEG to
  // the exisiting memberId's VEG.
  // ---------------------------------------------------------------------
  void addVEGMember(const ValueId & memberId);

  // ---------------------------------------------------------------------
  // VEGRegion::addZone()
  // ---------------------------------------------------------------------
  void addZone(VEGRegion * parentRegion, VEGRegion * newZone) 
                                 { parentRegion->zones_.insert(newZone); }

  // ---------------------------------------------------------------------
  // Search for a VEGMember that contains the given ValueId.
  // If not found, return NULL.
  // ---------------------------------------------------------------------
  VEGMember * findVEGMember(const ValueId & vid) const;


  VEGMember * findVEGMember(VEGReference *vegReference);
  
  // ---------------------------------------------------------------------
  // VEGRegion::getVEGReferenceFromCurrentVEGRegion()
  // The following method returns the VEGReference for the VEG to 
  // which the given value belongs to in the current VEGRegion. If
  // the given value is not a member of the current VEGRegion, the
  // method returns NULL.
  // ---------------------------------------------------------------------
  VEGReference * getVEGReferenceFromCurrentVEGRegion
                    (const ValueId & exprId, 
		     const VEGRegion * ignoreThisChild = NULL,
                     const VEGRegion * searchThisVEGRegionFirst = NULL) const;

  
  // ---------------------------------------------------------------------
  // VEGRegion::getVEGReferenceFromParentVEGRegion()
  // The following method searches for a VEGRegion amongst parent 
  // Regions, that contains the given exprId as a member. If
  // the given value is not a member of a parent VEGRegion, the
  // method returns NULL.
  // ---------------------------------------------------------------------
  VEGReference * getVEGReferenceFromParentVEGRegion
                    (const ValueId & exprId) const;
  
  // -----------------------------------------------------------------------
  // VEGRegion::importVEGsForUnionChildVEGRegion()
  // -----------------------------------------------------------------------
  void importVEGsForUnionChildVEGRegion();
  
  // ---------------------------------------------------------------------
  // VEGRegion::mergeZonesAndMakeReferencesConsistent()
  // ---------------------------------------------------------------------
  void mergeZonesAndMakeReferencesConsistent();
  
  // ---------------------------------------------------------------------
  // A method for computing the transitive closure of "=" predicates.
  // ---------------------------------------------------------------------
  ItemExpr * performTC(const ValueId & memberOfVEG);
  
  // ---------------------------------------------------------------------
  // Accessor function
  // ---------------------------------------------------------------------
  const ExprNode * getOwnerExpr() const             { return ownerExpr_; }   

  Lng32 getSubtreeId() const                         { return subtreeId_; }

  RegionId getRegionId() const                      { return myRegionId_; }   

  VEG * getVEG(const ValueId & valId) const;
  
  // ---------------------------------------------------------------------
  // Mutator functions.
  // ---------------------------------------------------------------------
  void setOwnerExpr(const ExprNode * const ownerExpr) 
		{ ownerExpr_ = ownerExpr; }   

  void setSubtreeId(const Lng32 subtreeId)         { subtreeId_ = subtreeId; }

  // ---------------------------------------------------------------------
  // Parent Region
  // ---------------------------------------------------------------------
  void setParentVEGRegion(VEGRegion * newParent) { parentRegion_ = newParent;}

  VEGRegion * getParentVEGRegion() const { return parentRegion_; }
  
  RegionId getParentVEGRegionId() const { return 
          parentRegion_ ? parentRegion_->getRegionId() : NULL_VEG_REGION ;}

  // ---------------------------------------------------------------------
  // A method for gathering all the ValueId's of null-instantiated members
  // present in the region.
  // ---------------------------------------------------------------------
  void gatherInstantiateNullMembers(ValueIdSet & vidset);

  // ---------------------------------------------------------------------
  // VEGRegion::gatherValueIdsOfVEGs()
  // Accumulate the ValueIds of all VEGs that are defined in this Region.
  // ---------------------------------------------------------------------
  void gatherValueIdsOfVEGs(ValueIdSet & vidSet) const;

  // ---------------------------------------------------------------------
  // VEGRegion::gatherValueIdsOfVEGs()
  // Accumulate the ValueIds of all VEGs that are defined in this Region
  // with ValueId vid.
  // ---------------------------------------------------------------------
  void gatherValueIdsOfMembersWithVEGVid(ValueIdSet &vidSet, ValueId vid) const;
  
  void replaceAllVEGs(ValueIdSet & vidSet, ValueId vid); 

  // For Full Outer Join,  we need to findout which of the children 
  // produce exprId as output and mark the region assosiated with 
  // that child as "to be merged".
  void locateDescendantVEGRegionAndMarkToBeMerged(const RelExpr *owner, 
						  const ValueId & exprId);
  
  // VEGRegion::mergeVEGRegion()

  void mergeVEGRegion(VEGRegion *fromRegion);

  void fixupZonesAfterFullToLeftConversion();

  void fixupZonesAndParentPointers();

  // ---------------------------------------------------------------------
  // What state is this VEGRegion in ?
  // ---------------------------------------------------------------------
  void markAsMerged()  
    { CMPASSERT(stateEnum_ != MERGED_STATE); stateEnum_ = MERGED_STATE; }

  // ---------------------------------------------------------------------
  // Delete the VEGMember that contains the given ValueId
  // ---------------------------------------------------------------------
  void deleteVEGMember(const ValueId & memberId)
                                           { replaceVEGMember(memberId); }
  
  // ---------------------------------------------------------------------
  // Print 
  // ---------------------------------------------------------------------
  void display();

  void print( FILE* ofd = stdout,
	      const char* indent = DEFAULT_INDENT,
              const char* title = "VEGRegion");

private:

  // *********************************************************************
  // VEGRegion - private declarations
  // *********************************************************************

  // ---------------------------------------------------------------------
  // A VEGRegion can be in one of three states:
  // ACTIVE_STATE
  //   A member of an active VEGRegion can be substituted with 
  //   another member of the VEG that it belongs to.      
  // TO_BE_MERGED_STATE
  //   The discovery of an "=" predicate that compares the member of
  //   one publisher VEGRegion with another publisher VEGRegion causes
  //   the more recently allocated (younger) VEGRegion to be merged
  //   with the other one. 
  //   The merging of VEGRegions occurs when a value that can suffer
  //   null-instantiation by a Left Join is compared with another
  //   value.
  // MERGED_STATE
  //   The MERGED_STATE and the ACTIVE_STATE are mutually exclusive.
  //   A VEGRegion must have been in the TO_BE_MERGED_STATE to be able to 
  //   reach the MERGED_STATE. A VEGRegion reaches this state after all
  //   the VEGs that belong to it have been combined with those that 
  //   belong to its parent VEGRegion.
  // ---------------------------------------------------------------------
  enum VEGRegionStateEnum { ACTIVE_STATE, 
			    TO_BE_MERGED_STATE, 
			    MERGED_STATE
			  };

  // *********************************************************************
  // VEGRegion - private methods
  // *********************************************************************

  // ---------------------------------------------------------------------
  // Allocate a new Zone
  // ---------------------------------------------------------------------
  VEGRegion * allocateNewZone(const VEGRegionTypeEnum tev, 
			      const ExprNode * const ownerExpr);
  
  // ---------------------------------------------------------------------
  // Does this VEGRegion export VEGs?
  // ---------------------------------------------------------------------
  NABoolean exportsVEG() const { return ( typeEnum_ == IMPORT_AND_EXPORT );}
  
  // ---------------------------------------------------------------------
  // VEGRegion::mergeForwardingEntries()
  // This is a help method used to merge the forwarding entries of the given
  // region into this region. It is called when we are merging regions. The
  // forwarding entries in the to-be-merged region need to identify its
  // corresponding new VEG in the new region before being merged in.
  // ---------------------------------------------------------------------
  void mergeForwardingEntries(const VEGRegion *fromRegion);

  // ---------------------------------------------------------------------
  // VEGRegion::fixupActiveZones()
  // This method is invoked after all the Zones that are marked "to be
  // merged" are merged. It updates the parentRegion for each VEGRegion 
  // and replaces those members that are also a member of a parent Region
  // with a VEGReference for the VEG to which they belong.
  // ---------------------------------------------------------------------
  void fixupActiveZones(VEGRegion * activeParentRegion,
		        const ValueIdSet & outerReferences);
  
  // ---------------------------------------------------------------------
  // VEGRegion::gatherValueIdsOfMembers()
  // Accumulate the ValueIds of the members of VEGs that are defined in 
  // this Region.  
  // ---------------------------------------------------------------------
  void gatherValueIdsOfMembers(ValueIdSet & vidSet) const;

  // ---------------------------------------------------------------------
  // VEGRegion::gatherAllForwardingEntries()
  // ---------------------------------------------------------------------
  void gatherForwardingEntries(LIST(VEGMember *) & vegMembers) const;
  
  // ---------------------------------------------------------------------
  // Get the VEGMember at the given position in the VEGRegion.
  // ---------------------------------------------------------------------
  VEGMember * getVEGMember(Lng32 index) { return members_[index]; } 
  
  // ---------------------------------------------------------------------
  // Get the ValueId for the VEG to which the given two expressions
  // should belong.
  // ---------------------------------------------------------------------
  ValueId getVEGValueId(VEGMember * vegDesc1, VEGMember * vegDesc2);
  

  void markAsProcessed()
    { CMPASSERT(NOT processingDone()); processedFlag_ = TRUE; }

  // ---------------------------------------------------------------------
  // Merge oldVEG into newVEG.
  // Replace all references of the VEG oldVEG in this VEGRegion
  // with a reference to the VEG newVEG.
  // ---------------------------------------------------------------------
  void mergeVEG(const ValueId & oldVEG, const ValueId & newVEG);
  
  // ---------------------------------------------------------------------
  // VEGRegion::mergeZonesFromSameVEGRegion()
  // This method is invoked on the Region into which VEGs from the 
  // fromRegion are to be merged. This Region is therefore the "toRegion".
  // The merge happens in the fashion: "toRegion" <- "fromRegion". 
  // ---------------------------------------------------------------------
  void mergeZonesFromSameVEGRegion(VEGRegion * fromRegion,
                                   VEGRegion * parentRegion = NULL);

  // ---------------------------------------------------------------------
  // VEGRegion::processZones()
  // ---------------------------------------------------------------------
  void processZones();


  // ---------------------------------------------------------------------
  // The following calls replace some members of the VEGRegion that are
  // an InstantiateNull with a new member that is its replacementExpr().
  // This replacement is done in order to eliminate those InstantiateNull 
  // operators that are no longer necessary because a left join is
  // being transformed to an inner join.
  // ---------------------------------------------------------------------
  void replaceInstantiateNullMembers();
  
  // ---------------------------------------------------------------------
  // VEGRegion::replaceOuterReferences()
  // ---------------------------------------------------------------------
  void replaceOuterReferences(const ValueIdSet & outerReferences);
  
  // ---------------------------------------------------------------------
  // Delete the VEGMember for the member whose ValueId is existingMemberId.
  // If newMemberId is not the NULL_VALUE_ID, then add a new VEGMember 
  // for newMemberId for replacing the deleted one.
  // If newMemberId is equal to the NULL_VALUE_ID, then replaceVEGMember()
  // is synonymous with deleteVEGMember().
  // ---------------------------------------------------------------------
  void replaceVEGMember(const ValueId & existingMemberId,
		        const ValueId & newMemberId = NULL_VALUE_ID);
  
  // *********************************************************************
  // VEGRegion - private data
  // *********************************************************************

  const enum VEGRegionTypeEnum typeEnum_;      // IMPORT_ONLY, IMPORT_AND_EXPORT
  
  enum VEGRegionStateEnum      stateEnum_;     // ACTIVE, TO BE MERGED or MERGED
  
  NABoolean                    processedFlag_; // to assure idempotence

  const ExprNode * 		ownerExpr_;     // the owner of this VEGRegion

  Lng32                         subtreeId_;     // VEGRegion is for which child 

  const RegionId               myRegionId_;    // should be equal to my array index

  VEGRegion *                  parentRegion_;  // pointer to my parent

  VEGTable  *                  vegTable_;      // in order to be able to allocate 
                                               // a new Zone on demand
  
  LIST(VEGRegion *)            zones_;         // descendants of this VEGRegion

  RAList<VEGMember *>          members_;       // members of this VEGRegion
  
}; // class VEGRegion
  
// ***********************************************************************
// VEGTable : Main memory table of ValueId Equality Groups.
// ***********************************************************************
class VEGTable : public NABasicObject
{
public:

  // ---------------------------------------------------------------------
  // Constructor
  // ---------------------------------------------------------------------
  VEGTable();

  // ---------------------------------------------------------------------
  // VEGTable::allocateAndSetVEGRegion()
  // Allocate a new VEGRegion and set currency pointer.
  // ---------------------------------------------------------------------
  void allocateAndSetVEGRegion(const VEGRegionTypeEnum tev,
			       const ExprNode * const ownerExpr,
			       Lng32 subtreeId = 0)
        { setCurrentVEGRegion(allocateVEGRegion(getCurrentVEGRegion(), tev,
					        ownerExpr, subtreeId)); } 

  // ---------------------------------------------------------------------
  // VEGTable::allocateVEGRegion()
  // This is a helper function that is used internally.
  // ---------------------------------------------------------------------
  VEGRegion * allocateVEGRegion(VEGRegion * parentRegion, 
                                const VEGRegionTypeEnum tev,
                                const ExprNode * const ownerExpr,
                                Lng32 subtreeId = 0);
  
  // ---------------------------------------------------------------------
  // Locates the Region to which a given ExprNode belongs and sets
  // it to become the current Region. If no Region contains the 
  // given ExprNode, it makes the FIRST_VEG_REGION the current Region.
  // ---------------------------------------------------------------------
  void locateAndSetVEGRegion(const ExprNode * const exprPtr, 
                             Lng32 subtreeId = 0); 

  //-----------------------------------------------------------------------
  //A method to locate the VEG region given an ExprNode
  //-----------------------------------------------------------------------
  VEGRegion* locateVEGRegion(ExprNode *ownerENptr)
                            { const ExprNode* const op= (ExprNode*)ownerENptr;
                               return getVEGRegion(op);}

  //-----------------------------------------------------------------------
  //A method to locate the VEG region given an ExprNode
  //-----------------------------------------------------------------------
  VEGRegion* locateVEGRegion(ExprNode *ownerENptr, Lng32 subtreeId)
                     { const ExprNode* const op= (ExprNode*)ownerENptr;
                              return getVEGRegion(op, subtreeId);}


  //-----------------------------------------------------------------------
  //A method to reassign a VEG region to an ExprNode
  // Currently only used for reasigning the regioin of a scalar-agg to 
  // a leftjoin during unnesting.
  //-----------------------------------------------------------------------
  void reassignVEGRegion(ExprNode *ownerENptr, Lng32 oldSubtreeId, ExprNode *newOwnerENptr, Lng32 newSubtreeId)
                     {  VEGRegion *vr = getVEGRegion(ownerENptr, oldSubtreeId);
                        CMPASSERT(vr != NULL); 
                        vr->setOwnerExpr(newOwnerENptr);
                        vr->setSubtreeId(newSubtreeId);}



  void restoreOriginalRegion();

  // ---------------------------------------------------------------------
  // Get the region Id of the current region
  // ---------------------------------------------------------------------
  RegionId getCurrentVEGRegionId() const {return getCurrentVEGRegion() 
          ? getCurrentVEGRegion()->getRegionId() : NULL_VEG_REGION ;}

  // ---------------------------------------------------------------------
  // Combine the current and the next VEGRegion in sequence.
  // ---------------------------------------------------------------------
  VEGRegion * locateVEGRegionAndMarkToBeMerged(const ValueId & exprId);
  
  // ---------------------------------------------------------------------
  // The following method locates the Region of which the given ExprNode
  // is the owner and returns TRUE if it is merged; FALSE otherwise.
  // ---------------------------------------------------------------------
  NABoolean locateVEGRegionAndCheckIfMerged
              (const ExprNode * const ownerExpr)
                      { return getVEGRegion(ownerExpr)->isMerged(); }

  NABoolean locateVEGRegionAndCheckIfMerged 
  (const ExprNode * const ownerExpr, Lng32 subtreeId)
  { return getVEGRegion(ownerExpr, subtreeId)->isMerged(); }

  
  // ---------------------------------------------------------------------
  // Add a new VEG to the VEGTable
  // ---------------------------------------------------------------------
  void addVEG(const ValueId & expr1Id, const ValueId & expr2Id);
  void addVEG(const ValueIdSet & setOfValues);
  void addVEGInOuterRegion(const ValueId & expr1Id, const ValueId & expr2Id);
  
  void deleteVEGMember(const ValueId &vId);

  // ---------------------------------------------------------------------
  // VEGTable::getVEGReference()
  // Rules:
  // 1) If the given expression belongs to a VEG in the current Region,
  //        + if the VEG contains a ITM_CONSTANT return a pointer to it
  //        + else return a pointer to its VEGReference.
  // 2) Otherwise, return a NULL pointer.
  // ---------------------------------------------------------------------
  ItemExpr * getVEGReference(const ValueId & exprId,
                      const VEGRegion *searchThisVegRegionFirst = NULL) const;


  // ---------------------------------------------------------------------
  // Method for performing transitive closure for "=" predicates,
  // given one of the values that participates in the relationship.
  // ---------------------------------------------------------------------
  ItemExpr * performTC(const ValueId & memberOfVEG)
                { return getVEGRegion(getCurrentVEGRegionId())
                                            ->performTC(memberOfVEG); }

  // ---------------------------------------------------------------------
  // Process the active Regions.
  // ---------------------------------------------------------------------
  void processVEGRegions(); 

  // ---------------------------------------------------------------------
  // Print 
  // ---------------------------------------------------------------------
  void display();

  void print( FILE* ofd = stdout,
	      const char* indent = DEFAULT_INDENT,
              const char* title = "VEGTable");

  VEGRegion * getCurrentVEGRegion() const { return currentRegion_; }

  Lng32 numberOfRegions() { return arrayEntry_.entries(); }

  // --------------------------------------------------------------------
  // Get addressability to the entry at the given entryIndex.
  // --------------------------------------------------------------------
  VEGRegion * getVEGRegion(const RegionId candidateRegion) const;

private:
  
  // *********************************************************************
  // VEGTable - private methods
  // *********************************************************************

  void setCurrentVEGRegion(VEGRegion * regPtr) { currentRegion_ = regPtr; }

  // --------------------------------------------------------------------
  // Locate the Region and VEGRegion to which a given ExprNode belongs.
  // --------------------------------------------------------------------
  VEGRegion * getVEGRegion(const ExprNode * const exprPtr,
                           Lng32 subtreeId = 0) const;

  // --------------------------------------------------------------------
  // Get the first VEGRegion that contains a vegmember for a given ValueId
  // --------------------------------------------------------------------
  VEGRegion * getVEGRegion(const ValueId exprId) const;

  // ********************************************************************
  // VEGTable - private data
  // ********************************************************************
  RegionId    nextInSequence_;       // The next RegionId to be assigned

  ARRAY(VEGRegion *)  arrayEntry_;   // 

  VEGRegion * currentRegion_;        // the Region that is currently being processed 

};  // class VEGTable

#endif /* VEGTABLE_H */

