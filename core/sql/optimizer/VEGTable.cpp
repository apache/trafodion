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
* File:         VEGTable.C
* Description:  Group Attributes
*
*
* Created:      11/16/1994
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "Sqlcomp.h"
#include "GroupAttr.h"
#include "RelJoin.h"
#include "ItemLog.h"
#include "ItemOther.h"
#include "ItemColRef.h" 
#include "ItemFunc.h"
#include "VEGTable.h"


extern NABoolean GenEvalPredicate(ItemExpr * rootPtr);

// -----------------------------------------------------------------------
// static processMultipleConstValuesInVEG()
//
// This method searches for a ConstValue in the vegSet.
// If vegSet contains two different constant values, e.g. 
// {a, b, 10, 20 }, then it builds the boolean expression FALSE.
// If vegSet does not contain more than one ConstValue, or if
// all constants are equal to each other, it returns a NULL.
// -----------------------------------------------------------------------
static ItemExpr * processMultipleConstValuesInVEG(const ValueIdSet & vegSet) 
{
  ValueId exprId;
  ItemExpr * rootPtr = NULL;
  ValueIdSet constSet;
  // ---------------------------------------------------------------------
  // Search loop: Gather ConstValues from the VEG
  // ---------------------------------------------------------------------
  for (exprId = vegSet.init(); vegSet.next(exprId); vegSet.advance(exprId))
    {
      if (exprId.getItemExpr()->getOperatorType() == ITM_CONSTANT)
	{
	  constSet += exprId;
	}
    } // search loop: look for a ConstValue

  // ---------------------------------------------------------------------
  // Found more than one constant? 
  // Replace the = predicate with a return False function, if all constants
  // are not equal to each other.
  // ---------------------------------------------------------------------
  if (constSet.entries() > 1)
    {
      // Create a predicate,
      // where <const1> = <const2> and <const1> = <const3>..."
      // and evaluate it. If it returns FALSE, then the constants
      // are not equal to each other. Replace the predicate with
      // a "RETURN FALSE" function.
      NABoolean firstTime = TRUE;
      ItemExpr * leftEntry = NULL;
      for (exprId = constSet.init(); 
	   constSet.next(exprId); 
	   constSet.advance(exprId))
	{
	  if (firstTime)
	    {
	      leftEntry = exprId.getItemExpr();
	      firstTime = FALSE;
	    }
	  
	  else
	    {
	      ItemExpr * eqPred = 
		new(CmpCommon::statementHeap()) BiRelat(ITM_EQUAL,
							leftEntry,
							exprId.getItemExpr());
	      if (rootPtr == NULL)
		{
		  rootPtr = eqPred;
		}
	      else
		{
		  rootPtr =
		    new(CmpCommon::statementHeap()) BiLogic(ITM_AND,
							    rootPtr,
							    eqPred);
		}
	    } // not firstTime
	} // for

      if (GenEvalPredicate(rootPtr) == FALSE)
	{
	  rootPtr = new(CmpCommon::statementHeap()) BoolVal(ITM_RETURN_FALSE);
	  rootPtr->synthTypeAndValueId(); 
	  return rootPtr;
	}
    } // more than one constant values.
  
  return NULL;
} // static processMultipleConstValuesInVEG()

// ***********************************************************************
// Methods on VEGMember
// ***********************************************************************

// -----------------------------------------------------------------------
// == operator
// -----------------------------------------------------------------------
NABoolean VEGMember::operator==(const VEGMember & other) const
{
  return ( (memberM_ == ((const VEGMember&)other).memberM_) AND  
	   (groupG_ == ((const VEGMember&)other).groupG_) );
} // VEGMember::operator==()
  
// -----------------------------------------------------------------------
// display operator
// -----------------------------------------------------------------------
void VEGMember::print(FILE* ofd, const char* indent, const char* title)
{
  BUMP_INDENT(indent);
  fprintf(ofd,"%s %s %s",NEW_INDENT,title,NEW_INDENT);
  fprintf(ofd,"Member (0x%x) VEG (0x%x) VEGRef (0x%x) VEGPred (0x%x)  {%s}  {%s}\n", 
              CollIndex(memberM_), CollIndex(groupG_),
	      CollIndex(getVEG()->getVEGReference()->getVEG()->getValueId()),
	      CollIndex(getVEG()->getVEGPredicate()->getVEG()->getValueId()),
	      memberM_.getItemExpr()->getText().data(),
	      groupG_.getItemExpr()->getText().data() );

} // VEGMember::print()

// To be called from the debugger
void VEGMember::display()
{
 VEGMember::print();
} // VEGMember::display()
  
// ***********************************************************************
// Methods on VEGRegion.
// ***********************************************************************

// ***********************************************************************
// Methods on VEGRegion
// ***********************************************************************

VEGMember * VEGRegion::findVEGMember(VEGReference *vegReference)
{
  Lng32 ne = members_.entries();
  Lng32 i = 0;
  NABoolean found = FALSE;
  ValueId memberValueId;
  VEGReference *memberVEGReference;
  Lng32 index=0;
  for (index = 0; index < ne ; index++) 
    {
      memberVEGReference = members_[index]->getVEG()->getVEGReference();
      if (memberVEGReference == vegReference)
	{
	  found = TRUE;
	  break;
	}
    }
  if (found)
    return members_[index];
  return NULL;
}

// -----------------------------------------------------------------------
// VEGRegion::findVEGMember()
// -----------------------------------------------------------------------
VEGMember * VEGRegion::findVEGMember(const ValueId & vid) const
{
  Lng32 ne = members_.entries();
  Lng32 i = 0;
  NABoolean found = FALSE;
  ValueId memberValueId;

  while ( (i < ne) AND (NOT found) )
    {
      memberValueId = members_[i]->getMemberValueId();

      // does the VEGMember contain the given ValueId?
      if (memberValueId == vid)
	found = TRUE;
      else if (memberValueId.getItemExpr()->getOperatorType() == ITM_VEG_REFERENCE)
        {
          ValueIdSet expandedValues;
          ((VEGReference *) (memberValueId.getItemExpr())) -> getVEG() ->
           getAndExpandAllValues(expandedValues);

          if (expandedValues.contains(vid))
            found = TRUE;
          else
	    i++;
        }
      else
	i++;
    } // while

  if (found)
    return members_[i]; // return the ValueId for the VEG
  else 
    return NULL;
} // VEGRegion::findVEGMember()

// -----------------------------------------------------------------------
// VEGRegion::getVEGValueId()
// -----------------------------------------------------------------------
ValueId VEGRegion::getVEGValueId(VEGMember * vegDesc1, VEGMember * vegDesc2) 
{
  VEG * vegPtr;  
  ValueId  veg1Id, veg2Id, rtrnId;    // return value
  
  // ---------------------------------------------------------------------
  // Compute the ValueIds for the VEG's
  // ---------------------------------------------------------------------
  if (vegDesc1 != NULL)
    veg1Id = vegDesc1->getVEGValueId();
  else 
    veg1Id = NULL_VALUE_ID;

  if (vegDesc2 != NULL)
    veg2Id = vegDesc2->getVEGValueId();
  else 
    veg2Id = NULL_VALUE_ID;

  // ---------------------------------------------------------------------
  // Neither expression belongs to any of the existing VEGs
  // ---------------------------------------------------------------------
  if ( (veg1Id == NULL_VALUE_ID) AND (veg2Id == NULL_VALUE_ID) )
    {
      // Allocate a new VEG
      vegPtr = (VEG *)new(CmpCommon::statementHeap()) VEG();
      rtrnId = vegPtr->getValueId();
    }
  // ---------------------------------------------------------------------
  // Either one of the two expression belongs to an existing VEG
  // ---------------------------------------------------------------------
  else if ( (veg1Id != NULL_VALUE_ID) AND (veg2Id == NULL_VALUE_ID) )
    {
      rtrnId = veg1Id;
    }
  else if ( (veg1Id == NULL_VALUE_ID) AND (veg2Id != NULL_VALUE_ID) )
    {
      rtrnId = veg2Id;
    }
  // ---------------------------------------------------------------------
  // Both the expressions belong to an existing VEG
  // ---------------------------------------------------------------------
  else if ( veg1Id != veg2Id )    // but they belong to different VEGs 
    {
      rtrnId = veg1Id;
      // replace the ValueId for VEG2 with the ValueId for VEG1 
      // in the VEGRegion
      mergeVEG(veg1Id, veg2Id); 
    }
  else                            // and they both belong to the same VEG 
    rtrnId = veg1Id;
  
  return rtrnId;
    
} // VEGRegion::getVEGValueId()

// -----------------------------------------------------------------------
// VEGRegion::mergeVEG()
// -----------------------------------------------------------------------
void VEGRegion::mergeVEG(const ValueId & newVEG, const ValueId & oldVEG)
{
  if (NOT (oldVEG == newVEG))
    {
      VEG * vegPtr = (VEG *)(newVEG.getItemExpr());

      vegPtr->merge( *( (VEG *)(oldVEG.getItemExpr()) ) );

      // Walk through the VEGRegion and replace a reference to oldVEG
      // with a reference to newVEG.
      for (Lng32 i = 0; i < (Lng32)members_.entries(); i++)
	{
	  if ( members_[i]->getVEGValueId() == oldVEG )
	    {
	      VEGMember * vegDesc = members_[i];
	      vegDesc->setVEGValueId(newVEG);
	    } // endif
      
	} // for
 
      // add the ItemExpr for the oldVEG to a free list

    } // endif (NOT (oldVEG == newVEG))

} // VEGRegion::mergeVEG()

// -----------------------------------------------------------------------
// VEGRegion::getVEG()
// -----------------------------------------------------------------------
VEG * VEGRegion::getVEG(const ValueId & valId) const
{ 
  VEGMember * ZzTop = findVEGMember(valId);
  if (ZzTop != NULL)
    return ZzTop->getVEG();
  else
    return NULL;
} // VEGRegion::getVEG()


/*
void VEGRegion::deleteVEGMember(const ValueId &vId)
{

  VEGMember  *memberPtr = findVEGMember(vId);
  VEG  *vegPtr = memberPtr->getVEG(); 
  members_.remove(memberPtr);  

  vegPtr->getAllValuesToUpdate() -= existingMemberId;

  if (vegPtr->getAllValues().entries() == 1)
  {
    ValueId exprId = vegPtr->getAllValues().init();
    if (exprId.getItemExpr()->getOperatorType() == ITM_CONSTANT)
      deleteVEGMember(exprId);
  }
}
*/



// ---------------------------------------------------------------------
// Add a new entry for memberId if one doesn't already exist. If the
// VEG of memberId exists, then merge the two VEGs.
// If the memberId already exists, then merge the memberId's VEG to
// the exisiting memberId's VEG.
// ---------------------------------------------------------------------
void VEGRegion::addVEGMember(const ValueId & memberId)
{
  VEGMember * memberPtr = findVEGMember(memberId);

  VEGMember *memberIdPtr = (VEGMember *) memberId.getItemExpr();

#ifndef NDEBUG

  memberIdPtr->print();

#endif

  ValueId vegId = NULL_VALUE_ID;
  if (((ItemExpr *)memberIdPtr)->getOperatorType() == ITM_VEG_REFERENCE) 
    vegId = ((VEGReference *)memberIdPtr)->getVEG()->getValueId();
  else
    vegId = memberIdPtr->getVEGValueId();

  if (memberPtr) // found one in this VEGRegion
    {            // no need to add a new VEGMember.
      
      // merge the VEGs
      mergeVEG(vegId, // newVEG
	       memberPtr->getVEGValueId() //oldVEG
	       );
    }
  else // no existing VEGMember with memberId
    {
      // Create a new VEGMember entry in this Region.
      members_.insert(new(CmpCommon::statementHeap()) VEGMember
		      (memberId, vegId)); 
      
    } // no existing VEGMember with memberId
  
}

// -----------------------------------------------------------------------
// VEGRegion::addVEG(const ValueId &, const ValueId &)
// -----------------------------------------------------------------------
void VEGRegion::addVEG(const ValueId & expr1Id, const ValueId & expr2Id)
{
  // Find the VEGMember's for the two expressions
  VEGMember *vegDesc1 = findVEGMember(expr1Id);
  VEGMember *vegDesc2 = findVEGMember(expr2Id);
  // Compute the ValueId of the VEG to which the two expressions belong.
  ValueId vegId = getVEGValueId(vegDesc1, vegDesc2);
  // Allocate a VEGMember for each new expression.
  if (vegDesc1 == NULL)
    members_.insert(new(CmpCommon::statementHeap()) VEGMember(expr1Id,vegId)); 
  if ( (NOT (expr1Id == expr2Id)) AND (vegDesc2 == NULL) )
    members_.insert(new(CmpCommon::statementHeap()) VEGMember(expr2Id,vegId)); 
  // Add the ValueId of each expression to the VEG.
  ( ((VEG *)(vegId.getItemExpr())) )->insert(expr1Id);
  ( ((VEG *)(vegId.getItemExpr())) )->insert(expr2Id);
} // VEGRegion::addVEG()

// -----------------------------------------------------------------------
// VEGRegion::addVEG(const ValueIdSet &)
// -----------------------------------------------------------------------
void VEGRegion::addVEG(const ValueIdSet & vegMembers)
{
  ValueId commonMemberId, otherMemberId;

  vegMembers.getFirst(commonMemberId);  // extract one member

  // ---------------------------------------------------------------------
  // Iterate over the the given set of values
  // ---------------------------------------------------------------------
  for (otherMemberId = vegMembers.init(); 
       vegMembers.next(otherMemberId);
       vegMembers.advance(otherMemberId) )
     addVEG(commonMemberId, otherMemberId);
  
} // VEGRegion::addVEG()

// -----------------------------------------------------------------------
// VEGRegion::mergeForwardingEntries()
// This is a help method used to merge the forwarding entries of the given
// region into this region. It is called when we are merging regions. The
// forwarding entries in the to-be-merged region need to identify its
// corresponding new VEG in the new region before being merged in.
// -----------------------------------------------------------------------
void VEGRegion::mergeForwardingEntries(const VEGRegion *fromRegion)
{
  LIST(VEGMember *) allForwardingEntries(STMTHEAP);
  fromRegion->gatherForwardingEntries(allForwardingEntries);

  for(CollIndex i = 0; i < allForwardingEntries.entries(); i++)
  {
    VEGMember *vegMember = allForwardingEntries[i];
    CMPASSERT(vegMember->isAForwardingEntry());

    // Get the first entry from the original VEG.
    VEG *oldVEG = vegMember->getVEG();
    ValueId reprMemberInOldVEG;
    oldVEG->getAllValues().getFirst(reprMemberInOldVEG);

    // Search for the new VEG in this region.
    ValueId newVEGId = findVEGMember(reprMemberInOldVEG)->getVEGValueId();

    // Merge the forwarding entry into this region.
    members_.insert(new(CmpCommon::statementHeap())
                    VEGMember(vegMember->getMemberValueId(),newVEGId,
                              FALSE,TRUE));
  }
}

// -----------------------------------------------------------------------
// VEGRegion::gatherForwardingEntries()
// -----------------------------------------------------------------------
void VEGRegion::gatherForwardingEntries(LIST(VEGMember *) & vegMembers) const
{
  vegMembers.clear();
  for(CollIndex i = 0; i < members_.entries(); i++)
    if(members_[i]->isAForwardingEntry()) vegMembers.insert(members_[i]);
}

// -----------------------------------------------------------------------
// VEGRegion::allocateNewZone()
// -----------------------------------------------------------------------
VEGRegion * VEGRegion::allocateNewZone(const VEGRegionTypeEnum tev, 
				       const ExprNode * const ownerExpr)
{ 
  return vegTable_->allocateVEGRegion(this,tev,ownerExpr);  
}
  
// -----------------------------------------------------------------------
// VEGRegion::replaceVEGMember()
// -----------------------------------------------------------------------
void VEGRegion::replaceVEGMember(const ValueId & existingMemberId,
				 const ValueId & newMemberId )
{
  ValueId exprId;
  VEGMember * memberPtr = findVEGMember(existingMemberId);

  if (!memberPtr)
    return;

  VEG *  vegPtr = memberPtr->getVEG();  // -> its VEG

  // If this method is not called for deleting the member only
  // and fix is enabled
  Lng32 compInt54 = (ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_54);
  if (newMemberId != NULL_VALUE_ID AND compInt54 >= 0)
  {
    // reject if replacing existingMemberId in the VEG with newMemberId
    // results in circular VegReference see QC_1348.
    if (newMemberId.getItemExpr()->getOperatorType() == ITM_VEG_REFERENCE)
    {
      // get vegReference from VEG of existingMemberId
      VEGReference * oldMemVegRefPtr = vegPtr->getVEGReference();
      if (oldMemVegRefPtr != NULL)
      {
        ValueId ofOldVegRef = vegPtr->getVEGReference()->getValueId();
        // catch circular vegref cases in debug build when COMP_INT_54 is >0
        // regression tests can set COMP_INT_54 >0 to verify the fix
        if ( ((VEGReference *) (newMemberId.getItemExpr()))->
                 referencesVegRefValue(ofOldVegRef) ) 
        {
          if (compInt54 == 0)
            return;
          else
            DCMPASSERT("circular VegRef case found" == 0);
        }
      }
    }
  }

  members_.remove(memberPtr);  // remove member from the VEGRegion
  // Delete existingMemberId from the VEG.
  vegPtr->getAllValuesToUpdate() -= existingMemberId;
  // If the VEG contained exactly one member, delete the VEG
  if (vegPtr->getAllValues().entries() == 0)
  {
    exprId = newMemberId; // create a new VEG that contains newMemberId
  }
//  else if (vegPtr->getAllValues().entries() == 1 &&
//           newMemberId == NULL_VALUE_ID)
//  {
//    vegPtr->getAllValues().getFirst(exprId);
//    CMPASSERT(exprId != NULL_VALUE_ID); 
//    deleteVEGMember(exprId);
//  }
  else
    vegPtr->getAllValues().getFirst(exprId);
  // If this method is not called for deleting the member only
  if (newMemberId != NULL_VALUE_ID)
    {
      // Add the expression whose ValueId is newMemberId to the VEG.
      addVEG(exprId,newMemberId);
    }

} // VEGRegion::replaceVEGMember()

// -----------------------------------------------------------------------
// VEGRegion::gatherValueIdsOfVEGs()
// Accumulate the ValueIds of all VEGs that are defined in this Region.
// -----------------------------------------------------------------------
void VEGRegion::gatherValueIdsOfVEGs(ValueIdSet & vidSet) const
{
  for (Lng32 index = 0; index < (Lng32)members_.entries(); index++)
    vidSet += members_[index]->getVEGValueId();
} // VEGRegion::gatherValueIdsOfVEGs()

void VEGRegion::gatherValueIdsOfMembersWithVEGVid(ValueIdSet &vidSet, ValueId vid) const
{
  ValueIdSet vegVid;
  for (Lng32 index = 0; index < (Lng32)members_.entries(); index++) {
      vegVid = members_[index]->getVEGValueId();
      if (vegVid == vid)
	vidSet += members_[index]->getMemberValueId();
  }
} // VEGRegion::gatherValueIdsOfMembersWithVEGVid()


// -----------------------------------------------------------------------
// VEGRegion::replaceAllVEGs()
// -----------------------------------------------------------------------
void VEGRegion::replaceAllVEGs(ValueIdSet & vidSet, ValueId vid) 
{
  VEG *vegPtr = NULL;
  ValueId vegId = NULL_VALUE_ID;

  for (ValueId memberId = vidSet.init(); 
       vidSet.next(memberId); 
       vidSet.advance(memberId))
    {
      vegPtr = ((VEGMember *) (memberId.getItemExpr()))->getVEG();
      vegId = vegPtr->getValueId();
      vegId = vid;
      vegPtr->getVEGReference()->replaceVEG(vid);
      vegPtr->getVEGPredicate()->replaceVEG(vid);
    }
  
} // VEGRegion::replaceAllVEGs


// -----------------------------------------------------------------------
// VEGRegion::gatherValueIdsOfMembers()
// Accumulate the ValueIds of all the members of VEGs of this Region.
// -----------------------------------------------------------------------
void VEGRegion::gatherValueIdsOfMembers(ValueIdSet & vidSet) const
{
  for (Lng32 index = 0; index < (Lng32)members_.entries(); index++)
    vidSet += members_[index]->getMemberValueId();
} // VEGRegion::gatherValueIdsOfMembers()

// -----------------------------------------------------------------------
// VEGRegion::getVEGReferenceFromCurrentVEGRegion
// -----------------------------------------------------------------------
VEGReference * VEGRegion::getVEGReferenceFromCurrentVEGRegion
                             (const ValueId & exprId,
			      const VEGRegion * ignoreThisChildVEGRegion,
                              const VEGRegion * searchThisVegRegionFirst) const
{
  VEGReference * vegRefPtr = NULL;
  VEGRegion * currRegion= NULL;
  
  VEGMember * memberPtr = findVEGMember(exprId);

  Lng32 count = zones_.entries();

  if (memberPtr == NULL &&
      searchThisVegRegionFirst &&
      searchThisVegRegionFirst->exportsVEG() &&
      searchThisVegRegionFirst != ignoreThisChildVEGRegion)
  {
    for (Lng32 i=0; i < count && currRegion == NULL; i++)
    {
      if (searchThisVegRegionFirst == zones_[i])
        currRegion = zones_[i];
    }
  
    if (currRegion)
      memberPtr = currRegion->findVEGMember(exprId);
  }

  while ((memberPtr == NULL) AND (count > 0))
    {
      currRegion = zones_[--count];
      // If the current VEGRegion is allowed to export a reference
      // to a member of a VEG, check whether the given expression
      // is a member.
      if ( (currRegion != ignoreThisChildVEGRegion) AND
           (currRegion != searchThisVegRegionFirst) AND
           (currRegion->exportsVEG()) )
        memberPtr = currRegion->findVEGMember(exprId);
    } // end while loop over descendants of the same parent

  if (memberPtr) // found a VEGMember
    vegRefPtr = memberPtr->getVEG()->getVEGReference();
  
  return(vegRefPtr); // can be NULL
  
} // VEGRegion::getVEGReferenceFromCurrentVEGRegion()

// -----------------------------------------------------------------------
// VEGRegion::getVEGReferenceFromParentVEGRegion()
// -----------------------------------------------------------------------
VEGReference * VEGRegion::getVEGReferenceFromParentVEGRegion
                             (const ValueId & exprId) const
{
  VEGReference * vegRefPtr = NULL;
  const VEGRegion * parentRegion = getParentVEGRegion();

  while ((parentRegion != NULL) AND (vegRefPtr == NULL))
    {
      // Search through all of my parent VEGRegion's children
      // excepting myself.
      vegRefPtr = parentRegion->getVEGReferenceFromCurrentVEGRegion
	                           (exprId, this);
      if (vegRefPtr == NULL)
	parentRegion = parentRegion->getParentVEGRegion();
    } // end while loop over parent Regions
  
  return(vegRefPtr); // can be NULL
  
} // VEGRegion::getVEGReferenceFromParentVEGRegion()

// -----------------------------------------------------------------------
// VEGRegion::importVEGsForUnionChildVEGRegion()
// If any output value produced by the Union parent of this VEGRegion
// is a member of a parent VEGRegion, then add all the members of
// its VEG that are available in this VEGRegion as new members.
// 
// Even though the implementation is structured, this is pretty messy 
// stuff! If you can think of a better solution, please do feel 
// free to revisit the whole issue of dealing with ValueIdUnions.
// -----------------------------------------------------------------------
void VEGRegion::importVEGsForUnionChildVEGRegion()
{
  const Union * ownerUnion = (Union *)getOwnerExpr()->castToRelExpr();

  if ( (ownerUnion == NULL) OR 
       (ownerUnion->getOperatorType() != REL_UNION) )
    return;

  // ---------------------------------------------------------------------
  // MUST have a Union operator that is a parent of the owner of this
  // VEGRegion.
  // ---------------------------------------------------------------------
  ValueIdSet vegMembers;
  ValueIdSet valuesForLeftChild, valuesForRightChild;     // allocate containers
  VEGMember * memberPtr;
  
  // ---------------------------------------------------------------------
  // Is the owner the left or the right child of the Union?
  // ---------------------------------------------------------------------
  NABoolean leftChild = (getSubtreeId() == 0);   
  
  // ---------------------------------------------------------------------
  // Compute the set of values that the Union produces as output.
  // ---------------------------------------------------------------------
  ValueIdSet outputValues;
  ownerUnion->getPotentialOutputValues(outputValues);

  // ---------------------------------------------------------------------
  // Visit each VEGRegion that is an ancestor, searching for ValueIdUnions.
  // ---------------------------------------------------------------------
  VEGRegion * parentRegion = getParentVEGRegion();
  VEGRegion * parentZone;
  VEGRegion * unionChildRegion = this;
  Lng32 parentZoneCount, parentZoneCountPlusOne;
  Int32 iterations;                                         

  // ---------------------------------------------------------------------
  // Loop over all parent VEGRegions
  // ---------------------------------------------------------------------
  while (parentRegion)
    {
      parentZone = parentRegion; // initialize per iteration
      parentZoneCount = getParentVEGRegion()->zones_.entries();
      parentZoneCountPlusOne = 1 + parentZoneCount;
      iterations  = 0;

      // -----------------------------------------------------------------
      // Loop over a parent VEGRegion as well as all of its Zones
      // (descendant VEGRegions).
      // -----------------------------------------------------------------
      while (iterations < parentZoneCountPlusOne) 
	{

	  // -------------------------------------------------------------
	  // Examine only those VEGRegions that are either the parent
	  // VEGRegion or a Zone whose owner expression is an "=".
	  // Such a Zone is allocated for an "=" predicate that occurs
	  // in a subtree of an OR or an IS NULL/IS UNKNOWN. The VEGs
	  // that are contained in such VEGRegions are the only ones 
	  // that can be pushed down to the child subtrees of the 
	  // Union parent.
	  // -------------------------------------------------------------
	  if ((parentZone == parentRegion) OR       
	      (parentZone->getOwnerExpr()->getOperatorType() == ITM_EQUAL) )
	    {

	      // ---------------------------------------------------------
	      // Loop over each value that is produced as output by the
	      // Union parent and look for a VEG that contains it in a 
	      // parent VEGRegion or one of its Zones.
	      // ---------------------------------------------------------
	      for (ValueId memberId = outputValues.init();
		   outputValues.next(memberId); 
		   outputValues.advance(memberId))
		{

		  // -----------------------------------------------------
		  // Check if the given ValueId is a member of the current 
		  // VEGRegion.
		  // -----------------------------------------------------
		  memberPtr = parentZone->findVEGMember(memberId);
		  
		  if (memberPtr)
		    {
		      // -------------------------------------------------
		      // Check whether all the members of its VEG are 
		      // available at the Union parent. If they are, then
		      // the equality predicate represented by the VEG 
		      // will be pushed down to each child of the Union. 
		      // Otherwise, it cannot be pushed down and is not 
		      // interesting.
		      // -------------------------------------------------
		      vegMembers = (((VEG *)memberPtr->getVEG()))->getAllValues(); 
		      vegMembers -= ownerUnion->getGroupAttr()->getCharacteristicInputs();
		      vegMembers -= outputValues;
		  
		      if (vegMembers.isEmpty())
			{
			  // ---------------------------------------------
			  // Allocate a new Zone in this VEGRegion for 
			  // each VEG that is found in a Zone of a
			  // parent VEGRegion.
			  // ---------------------------------------------
			  if (parentZone != parentRegion)
			    {
			      CMPASSERT(parentZone->getOwnerExpr()->getOperatorType() == ITM_EQUAL);
			      vegMembers = parentZone->getOwnerExpr()->castToItemExpr()->getValueId();
			      // -----------------------------------------
			      // Rewrite the owner expression.
			      // -----------------------------------------
			      valuesForLeftChild.clear();	
			      valuesForRightChild.clear();  
			      ownerUnion->rewriteUnionExpr
				            (vegMembers,
					     valuesForLeftChild, 
					     valuesForRightChild);
			      ValueId exprId;
			      if (leftChild)
				valuesForLeftChild.getFirst(exprId);
			      else
				valuesForRightChild.getFirst(exprId);
			      unionChildRegion = allocateNewZone
				                   (parentZone->getVEGRegionTypeEnum(),
						    exprId.getItemExpr());
			    }

			  // ---------------------------------------------
			  // Rewrite the members of the VEG in terms of 
			  // the expressions that each child can 
			  // evaluate.
			  // ---------------------------------------------
			  valuesForLeftChild.clear();	
			  valuesForRightChild.clear();  
			  ownerUnion->rewriteUnionExpr
		                         (((VEG *)memberPtr->getVEG())->getAllValues(),
				          valuesForLeftChild, valuesForRightChild);

			  // ---------------------------------------------
			  // Add a new VEG for the rewritten members in 
			  // this VEGRegion.
			  // ---------------------------------------------
			  if (leftChild)
			    unionChildRegion->addVEG(valuesForLeftChild);
			  else
			    unionChildRegion->addVEG(valuesForRightChild);

			  // ---------------------------------------------
			  // Mark the member "To Be Deleted".
			  // ---------------------------------------------
			  memberPtr->markAsToBeDeleted();

			} // endif (vegMembers.isEmpty())

		    } // endif (memberPtr)
		  
		} // end for

	    } // endif (NOT parentZone->getUnionParent())

	  // -----------------------------------------------------------------
	  // Process each Zone of the parentVEGRegion.
	  // -----------------------------------------------------------------
	  if (iterations <  parentZoneCount)
	    parentZone = getParentVEGRegion()->zones_[iterations];
	  iterations++;

	} // end while

      parentRegion = parentRegion->getParentVEGRegion(); // advance on the iterator
      
    } // end while (parentRegion)

  
} // VEGRegion::importVEGsForUnionChildVEGRegion()

// -----------------------------------------------------------------------
// VEGRegion::gatherInstantiateNullMembers()
// -----------------------------------------------------------------------
void VEGRegion::gatherInstantiateNullMembers(ValueIdSet & vidset)
{
  Lng32 ne = members_.entries();
  Lng32 index;
  for (index = 0; index < ne; index ++)
  {
    VEGMember * memberPtr = members_[index];
    if (memberPtr->getItemExpr()->getOperatorType() == ITM_INSTANTIATE_NULL)
    {
      vidset.insert(memberPtr->getMemberValueId());
    }
  }
}

// -----------------------------------------------------------------------
// VEGRegion::replaceInstantiateNullMembers()
// -----------------------------------------------------------------------
void VEGRegion::replaceInstantiateNullMembers()
{
  Lng32 ne = members_.entries();      // number of entries in the VEGRegion
  Lng32 index;               // loop index
  InstantiateNull * instNullPtr; // -> an InstantiateNull 
  VEGMember * memberPtr;         // -> a VEGMember
  LIST(VEGMember *)deleteStack(STMTHEAP);  // VEGMember that are to be deleted
  ValueId exprId;                // ValueId of a member of the VEG
  ValueIdSet vegMembers;         // set of all members for a VEG 


  //*************************************************************
  // case 10-040128-2764: When VEG Regions get merged, we need to
  // replace the InstantiateNull members in the nested VEG Regions 
  // as well: For example, if we have a cascade of Unions, we need 
  // visit all the regions and replace marked Instantiate Nulls if 
  // they are VEG Members. 
  //**************************************************************
  for (index=0; (UInt32)index < zones_.entries();index++)
    zones_[index]->replaceInstantiateNullMembers();

  // ---------------------------------------------------------------------
  // Iterate over all VEGMembers in this VEGRegion.
  // ---------------------------------------------------------------------
  for (index = 0; index < ne; index ++)
    {

      memberPtr = members_[index];

      // -----------------------------------------------------------------
      // If the current descriptor contains an InstantiateNull as its 
      // member ...
      // -----------------------------------------------------------------
      if ((NOT memberPtr->isAForwardingEntry()) AND
	  (memberPtr->getItemExpr()->getOperatorType() == ITM_INSTANTIATE_NULL))
	{

	  instNullPtr = (InstantiateNull *)memberPtr->getItemExpr();

          if (instNullPtr->lojTransformInProgress())
             instNullPtr->setLOJTransformComplete();

	  // -------------------------------------------------------------
	  // If the normalizer has decided to replace the InstantiateNull
	  // with another expression, replace the VEGMember for the
	  // InstantiateNull  with a VEGMember that contains its replacement.
	  // CAUTION: The code below assumes that the normalizer has set 
	  // =======  the replacement expression for this InstantiateNull
	  //          to be different from itself ONLY when a merge of 
	  //          VEGs is possible.
	  // -------------------------------------------------------------
	  if (instNullPtr != instNullPtr->getReplacementExpr(TRUE))//soln:10-060105-3714
	    deleteStack.insert(memberPtr); // remember the member
	
	} // endif member is an InstantiateNull

    } // endfor

  // ---------------------------------------------------------------------
  // Iterate over entries in deleteStack.
  // Each entry of deleteStack is for a member that is an InstantiateNull.
  // ---------------------------------------------------------------------
  ne = deleteStack.entries();
  ValueId vegId;
  for (index = 0; index < ne; index++)
    {
      memberPtr = deleteStack[index];         // -> VEGMember
      vegId = memberPtr->getVEGValueId();
      instNullPtr = (InstantiateNull *)memberPtr->getItemExpr();
      // -----------------------------------------------------------------
      // Replace the InstantiateNull with its replacement expression.
      // -----------------------------------------------------------------
      replaceVEGMember(instNullPtr->getValueId(),
		       instNullPtr->getReplacementExpr(TRUE)->getValueId());//soln:10-060105-3714
      // -----------------------------------------------------------------
      // Allocate a dummy VEGMember to act as a forward reference.
      // The InstantiateNull is no longer a member of the VEG.
      // However, expressions that reference it should continue to be 
      // directed to the VEG to which it used to belong.
      // -----------------------------------------------------------------

#if 0
      // -----------------------------------------------------------------
      // Why can't we redirect the inst-null values to the new VEG we just
      // created? Pending further investigation. Commented out for now.
      // -----------------------------------------------------------------
      VEG *veg =
        findVEGMember(instNullPtr->getReplacementExpr()->getValueId())->
          getVEG();
      vegId = veg->getValueId();
#endif

      members_.insert(new(CmpCommon::statementHeap())
                VEGMember(instNullPtr->getValueId(), vegId, FALSE, TRUE));
   }
  
} // VEGRegion::replaceInstantiateNullMembers()

// -----------------------------------------------------------------------
// VEGRegion::replaceOuterReferences()
// -----------------------------------------------------------------------
void VEGRegion::replaceOuterReferences(const ValueIdSet & outerReferences)
{
  VEGReference * outerVEGReference = NULL;
  
  // ---------------------------------------------------------------------
  // Iterate over the the given set of outer references that are
  // members of VEGs in this Region.
  // ---------------------------------------------------------------------
  for (ValueId memberId = outerReferences.init(); 
       outerReferences.next(memberId); 
       outerReferences.advance(memberId))
    {
      outerVEGReference = getVEGReferenceFromParentVEGRegion(memberId);
      
      CMPASSERT(outerVEGReference ); // MUST NOT be NULL
      
      replaceVEGMember(memberId, outerVEGReference->getValueId());
      
    } // iterate over outerReferences
  
} // VEGRegion::replaceOuterReferences()

void VEGRegion::mergeVEGRegion(VEGRegion *fromRegion)
{
  
  //  merge fromRegion to toRegion (this Ptr)
  VEGRegion *toRegion = this;

  ValueIdSet setOfFromRegionVEGs;
  ValueId vegId;
  // ---------------------------------------------------------------------
  // Construct a set of all the VEGs that belong to the 
  // "fromRegion"  
  // ---------------------------------------------------------------------
  fromRegion->gatherValueIdsOfVEGs(setOfFromRegionVEGs);

  // ---------------------------------------------------------------------
  // Iterate over the fromRegion  VEGs.
  // ---------------------------------------------------------------------
  for (vegId = setOfFromRegionVEGs.init(); 
       setOfFromRegionVEGs.next(vegId); 
       setOfFromRegionVEGs.advance(vegId))
    {
      // find all the values for each VEG.
      ValueIdSet allValuesInVEG = ((VEG *)(vegId.getItemExpr()))->getAllValues();
      ValueId memberId, exprId;
      for (memberId = allValuesInVEG.init(); 
	   allValuesInVEG.next(memberId);
	   allValuesInVEG.advance(memberId) )
	{
	  if (memberId.getItemExpr()->getOperatorType() == ITM_VEG_REFERENCE)
	    {
	      // see if the reference was orginally from the toRegion.
	      // If so replace the reference with the original memberId.

	      // See if this reference came from the toRegion
	      VEGMember *memberPtr = toRegion->findVEGMember((VEGReference *)(memberId.getItemExpr()));

	      if (memberPtr) // came from the toRegion
              {
		fromRegion->replaceVEGMember(memberId, memberPtr->getMemberValueId());

		//ValueId vegVid = fromRegion->findVEGMember
		//  (memberPtr->getMemberValueId())->getVEGValueId();

		VEG *fromVEG = fromRegion->findVEGMember(memberPtr->getMemberValueId())->getVEG();
		
		toRegion->mergeVEG(fromVEG->getValueId(), // newVEG
                                   memberPtr->getVEGValueId() //oldVEG
				   );

		//		allValuesInVEG -= memberId;

		// fromVEG->merge(*memberPtr->getVEG());
		
		//members_.insert(new(CmpCommon::statementHeap()) VEGMember
		//			(memberPtr->getMemberValueId(), vegVid)); 
		//
		// ValueIdSet vidSet;
		// TBD - improve gatherValueIdsOfVEGsWithVid
		// toRegion->gatherValueIdsOfMembersWithVEGVid(vidSet, memberPtr->getVEGValueId());
		
		//toRegion->replaceAllVEGs(vidSet, fromVEG->getValueId());

		// now that we have a new entry for the memberPtr's vid, 
		// the existing entry with memberPtr's vid now becomes a 
		// forwarding entry.
		//memberPtr->markAsAForwardingEntry();
               }
	      else // it's a new entry to the toRegion.
		{
		  toRegion->addVEGMember(memberId);
		}
	    }
	  else // it's not an ITM_VEG_REFERENCE, hence a new entry to the toRegion
	    toRegion->addVEGMember(memberId);
	    //	    allValuesInVEG += memberId;
	}
     }

  fromRegion->markAsMerged();
  
}


// -----------------------------------------------------------------------
// VEGRegion::mergeZonesFromSameVEGRegion()
// This method is invoked on the Region into which VEGs from the 
// fromRegion are to be merged. This Region is therefore the "toRegion".
// The merge happens in the fashion: "toRegion" <- "fromRegion". 
// -----------------------------------------------------------------------
void VEGRegion::mergeZonesFromSameVEGRegion(VEGRegion * fromRegion,
                                            VEGRegion * parentRegion)

{
  ValueId vegId, commonMemberId, otherMemberId;
  ValueIdSet setOfVEGs;
  ValueIdSet vegMembers;
  fromRegion->markAsMerged();

  // ---------------------------------------------------------------------
  // Check if the owner expression for this VEGRegion is a LeftJoin.
  // If so, walk through its null instantiated output list and set
  // the replacement expression for each InstantiateNull that is its
  // member to be its child.
  // ---------------------------------------------------------------------
  if ( fromRegion->getOwnerExpr()->castToRelExpr() )
    {
      const Join * const joinPtr = ((const Join * const)(fromRegion->getOwnerExpr()->castToRelExpr()));
      // null instantiated columns for left join.
      
      if (joinPtr->getOperatorType() == REL_LEFT_JOIN)
	{
	  Lng32 index = 0;
	  ValueIdList & nullExprList =  
	    (ValueIdList &)joinPtr->nullInstantiatedOutput();
	  for (index = 0; index < (Lng32)nullExprList.entries(); index++)
	    {
	      ItemExpr * instNullPtr = nullExprList[index].getItemExpr();
	      instNullPtr->setReplacementExpr(instNullPtr->child(0).getPtr());
	    }
	}
      // null instantiated columns for full outer join.
      else if (joinPtr->getOperatorType() == REL_FULL_JOIN ) 
	{
	  Lng32 index = 0;
	  ValueIdList & nullExprList =  
	    (ValueIdList &)joinPtr->nullInstantiatedOutput();
	  for (index = 0; index < (Lng32)nullExprList.entries(); index++)
	    {
	      ItemExpr * instNullPtr = nullExprList[index].getItemExpr();
	      if (fromRegion->findVEGMember(instNullPtr->child(0).getValueId()))
		instNullPtr->setReplacementExpr(instNullPtr->child(0).getPtr());
	    }
	  
	  ValueIdList & nullExprListForRightJoinOutout =  
	    (ValueIdList &)joinPtr->nullInstantiatedForRightJoinOutput();
	  for (index = 0; index < (Lng32)nullExprListForRightJoinOutout.entries(); index++)
	    {
	      ItemExpr * instNullPtr = nullExprListForRightJoinOutout[index].getItemExpr();
	      if (fromRegion->findVEGMember(instNullPtr->child(0).getValueId()))
		instNullPtr->setReplacementExpr(instNullPtr->child(0).getPtr());
	    }
	} //if (joinPtr->getOperatorType() == REL_FULL_JOIN ) 
      
    }
  
// ---------------------------------------------------------------------
  // The following calls replace some members of the VEGRegion that are
  // an InstantiateNull with a new member that is its replacementExpr().
  // This replacement is done in order to eliminate those InstantiateNull 
  // operators that are no longer necessary because a left join is
  // being transformed to an inner join.
  // ---------------------------------------------------------------------
  if (parentRegion)
    parentRegion->replaceInstantiateNullMembers();
  else
    replaceInstantiateNullMembers();

  // Process all the descendant VEGRegions
  Lng32 ne = zones_.entries();

  // this call is not needed as the above call to
  // replaceInstantiateNullMembers() already does this
  for (Lng32 index = 0; index < ne; index++)
    zones_[index]->replaceInstantiateNullMembers();
  
  // ---------------------------------------------------------------------
  // Construct a set of all the VEGs that belong to the "fromRegion" and
  // add them to this Region.
  // ---------------------------------------------------------------------
  fromRegion->gatherValueIdsOfVEGs(setOfVEGs);

  // ---------------------------------------------------------------------
  // Iterate over the the given set of VEGs.
  // ---------------------------------------------------------------------
  for (vegId = setOfVEGs.init(); setOfVEGs.next(vegId); setOfVEGs.advance(vegId))
    addVEG( ((VEG *)(vegId.getItemExpr()))->getAllValues() );

  // ---------------------------------------------------------------------
  // Finally, merge the forwarding members from fromRegion to this.
  // ---------------------------------------------------------------------
  mergeForwardingEntries(fromRegion);
  
} // VEGRegion::mergeZonesFromSameVEGRegion()

// -----------------------------------------------------------------------
// VEGRegion::processZones()
// -----------------------------------------------------------------------
void VEGRegion::processZones()
{
  Lng32 index;
  Lng32 ne = members_.entries();
  LIST(VEGMember *) deleteStack(STMTHEAP);

  // Walk through the members of this VEGRegion to check if any member
  // is to be deleted.
  for (index = 0; index < ne; index ++)
    if (members_[index]->isToBeDeleted())
      deleteStack.insert(members_[index]);
  
  // Delete all the unwanted members.
  if (deleteStack.entries() > 0)
    {
      ne = deleteStack.entries();
      for (index = 0; index < ne; index ++)
	deleteVEGMember(deleteStack[index]->getMemberValueId());
    }
  
  // Process all the descendant VEGRegions
  ne = zones_.entries();
  for (index = 0; index < ne; index++)
    zones_[index]->processZones();

  // Merge the members of this Zone with its parent, if required.
  if (isToBeMerged())
    {
      CMPASSERT(getParentVEGRegion());  // MUST have a parent
      VEGRegion* parentVEGRegion = getParentVEGRegion();
      VEGRegion *grandParentVEGRegion = NULL;

      const ExprNode *fullJoinExpr =  parentVEGRegion->getOwnerExpr() ;
      if (fullJoinExpr && fullJoinExpr->getOperatorType() == REL_FULL_JOIN)
      {
        grandParentVEGRegion = parentVEGRegion->getParentVEGRegion();
        CMPASSERT(grandParentVEGRegion);  // MUST have a grandparent
      }
      parentVEGRegion->mergeZonesFromSameVEGRegion(this, grandParentVEGRegion);
    }
  
} // VEGRegion::processZones()


void VEGRegion::fixupZonesAfterFullToLeftConversion()
{
  Lng32 index;
  Lng32 ne = zones_.entries();

  // And that this method is invoked right after conversion
  // of full outer Join to left outer join. 
  // That means this region must have 3 descendents.
  
  CMPASSERT(ne == 3);
  for (index = 0; index < ne; index++)
    {
      if (zones_[index]->isActive())
	zones_[index]->setSubtreeId(0); // only for index = 1 (right child region.);
      else
	zones_[index]->setSubtreeId(-1);
    }
 }


// -----------------------------------------------------------------------
// VEGRegion::fixupActiveZones()
// This method is invoked after all the Regions are marked "to be merged" 
// are merged and then the zones and the parent pointers of these merged 
// regions have been fixed. It updates the parentRegion for each VEGRegion 
// and replaces those members that are also a member of a parent Region
// with a VEGReference for the VEG to which they belong.
// -----------------------------------------------------------------------
void VEGRegion::fixupActiveZones(VEGRegion * activeParentRegion,
				 const ValueIdSet & outerReferences)
{
   //Since all the zones and parent pointers of the merged (inactive)
   // regions have been fixed up in method fixupZonesAndParentPointers()
   // we should never come here for a region that is already merged (inactive)
   CMPASSERT(isActive());

  Lng32 index;
  Lng32 ne = zones_.entries();

  ValueIdSet  newOuterReferences(outerReferences);
  ValueIdSet  setOfMembers;

  // -----------------------------------------------------------------
  // Construct the set of values that are members of VEGs in this
  // Region. They are outer references for my descendants.
  // -----------------------------------------------------------------
  gatherValueIdsOfMembers(setOfMembers);
  newOuterReferences += setOfMembers; 

  // -----------------------------------------------------------------
  // Recursively fixup all my descendants.
  // -----------------------------------------------------------------
  for (index = 0; index < ne; index++)
  {
    const RelExpr *fullJoinExpr =  
    zones_[index]->getOwnerExpr()->castToRelExpr();
	  
    CMPASSERT(fullJoinExpr); // The descendent's owner better be a RelExpr.
    if (fullJoinExpr->getOperatorType() == REL_FULL_JOIN)
    {
      // before fixing up the decendents, export as outerReferences any
      // values from the other decendents.
      Lng32 index1=0;
      ValueIdSet setOfMembersFromDecendents, noLongerOuterReferences;
      for (index1 = 0; index1 < ne; index1++) // for index1 - inner loop
      {
        const RelExpr *fullJoinExpr1 =  
              zones_[index1]->getOwnerExpr()->castToRelExpr();

        //The two condition below state which zones the FOJ should look 
        // for outer references. The first states that a zone should exclude
        // itself, as there cannot be any outer references to VEGRegion from within
        // that region. The second condition states that for an FOJ we look 
        // for outer references only among the other two VegRegions of that FOJ
        // (each FOJ has three VEGRegions) in addition to outerreferences computed by
        // the standard rule. The second rule was added to fix soln. 10-081027-6839 
                   
	if ((index1 == index) /*zoneToExclude */ || 
                      (fullJoinExpr != fullJoinExpr1)) /* look for outerrefeences only in this FOJ */
	  continue;
	  
	if (zones_[index1]->exportsVEG() && zones_[index1]->isActive())
	{
	  zones_[index1]->gatherValueIdsOfMembers(setOfMembersFromDecendents);

          // we want to remove consts from being candidates for
          // substitution to avoid ending up with expressions like 
          // t.a = 45 and 45 = 45 for the join predicate. With this fix
          // we get the correct t.a = 45 and t1.b = 45 expression.
          setOfMembersFromDecendents.removeConstExprReferences();

	  newOuterReferences += setOfMembersFromDecendents;
	      
	  // We would no longer need this as outerReferences for the
	  // next recursive call to fixupActiveZones.
	  noLongerOuterReferences += setOfMembersFromDecendents;
	  setOfMembersFromDecendents.clear();  
	}
	  
      } // For index1 - inner loop.
	      
      // It may be that we should build the newOuterReferences
      // valueIdSet for each of the VEGRegions (indexed by index)
      // before we call fixupActiveZoes() to prevent recursive 
      // substitutions. Task for another day..
      zones_[index]->fixupActiveZones(this,newOuterReferences);
	      
      newOuterReferences.remove(noLongerOuterReferences);
      noLongerOuterReferences.clear();
    } // fullJoinExpr->getOperatorType() == REL_FULL_JOIN)
  else
    zones_[index]->fixupActiveZones(this,newOuterReferences);
  } // for index - outer loop
      
      
  // -----------------------------------------------------------------
  // If a value that is a member of this Region also belongs to a 
  // parent Region, then replace it with a VEGReference for the VEG
  // to which it belongs from the nearest (innermost) parent Region. 
  // -----------------------------------------------------------------
      
  setOfMembers.intersectSet(outerReferences);
  if (NOT setOfMembers.isEmpty())
    replaceOuterReferences(setOfMembers);
      
  // -----------------------------------------------------------------
  // For each VEG in this VEGRegion, synthesize the type for its
  // VEGReference. It is possible perform type synthesis for the 
  // first time for a VEG because membership to the VEG is made 
  // final only at this stage.
  // -----------------------------------------------------------------
  setOfMembers.clear();
  gatherValueIdsOfVEGs(setOfMembers);
  for (ValueId exprId = setOfMembers.init(); 
       setOfMembers.next(exprId); 
       setOfMembers.advance(exprId))
  {
    ((VEG *)exprId.getItemExpr())->getVEGReference()->synthTypeAndValueId();
  }
      
  
  // ---------------------------------------------------------------------
  // Mark all my descendant regions as "Processed"
  // ---------------------------------------------------------------------
  for (index = 0; index < ne; index++)
    zones_[index]->markAsProcessed();

} // VEGRegion::fixupActiveZones()

// -----------------------------------------------------------------------
// VEGRegion::mergeZonesAndMakeReferencesConsistent()
// -----------------------------------------------------------------------
void VEGRegion::mergeZonesAndMakeReferencesConsistent()
{
  if (processingDone())
    return;
  
  CMPASSERT(isActive());

  processZones();
  
  ValueIdSet outerReferences; // supply an empty set
  
  fixupZonesAndParentPointers();

  fixupActiveZones(getParentVEGRegion(),outerReferences);
  
  markAsProcessed();
  
} // VEGRegion::mergeZonesAndMakeReferencesConsistent()

//This method is invoked after all the Regions that have been marked 
// "to be merged" are merged.  After the regions have been merged 
// this method fixes the zones_ of the merged regions by adding them 
// to the parent zones_ and also fixes the parent pointers of the 
// children(zones) of the merged region.
void VEGRegion::fixupZonesAndParentPointers()
{

  RegionId index = FIRST_VEG_REGION;

  for (index = FIRST_VEG_REGION;
       index < (RegionId)(vegTable_->numberOfRegions());
       index++)
  {
    
    VEGRegion *candidateRegion = vegTable_->getVEGRegion((RegionId)index);

    if (candidateRegion->isMerged() && !(candidateRegion->processingDone()))
    {
      VEGRegion *parentRegion = candidateRegion->getParentVEGRegion();
   
      CollIndex indexOfThisInParentZone = parentRegion->zones_.index(candidateRegion);
 
      CMPASSERT (indexOfThisInParentZone != NULL_COLL_INDEX);
  
      parentRegion->zones_.remove(candidateRegion);

      Lng32 ne = candidateRegion->zones_.entries();

      // loop throught the zones of this region and add them to the 
      // parent region zones
      for (Lng32 i = ne-1; i >= 0; i--)
      {
        parentRegion->zones_.insertAt(indexOfThisInParentZone,
                                      candidateRegion->zones_[i]);
        candidateRegion->zones_[i]->setParentVEGRegion(parentRegion);
      }

      candidateRegion->markAsProcessed();
    }
  }
}  //VEGRegion::fixupZonesAndParentPointers()

// -----------------------------------------------------------------------
// VEGRegion::performTC()
// -----------------------------------------------------------------------
ItemExpr * VEGRegion::performTC(const ValueId & vegMember)
{
  // ---------------------------------------------------------------------
  // Check if the given ValueId is a member of the current Region.
  // ---------------------------------------------------------------------
  VEGMember * memberPtr = findVEGMember(vegMember);
  
  if (memberPtr == NULL) // The given value is not a member of this Region
    {
      // -----------------------------------------------------------------
      // It MUST be a value that is an "outer reference", i.e., a
      // value that is a member of some parent Region.
      // -----------------------------------------------------------------
      VEGReference * vegRefPtr = getVEGReferenceFromParentVEGRegion(vegMember);

      // -----------------------------------------------------------------
      // If the value is not a member of the current Region and is also
      // not a member of some parent Region, then the "=" predicate 
      // has not been registered in the VEGTable. Issue an internal error.
      // -----------------------------------------------------------------
      CMPASSERT(vegRefPtr);
      
      // -----------------------------------------------------------------
      // Check if the VEGReference is a member of the current Region.
      // -----------------------------------------------------------------
      memberPtr = findVEGMember(vegRefPtr->getValueId());
      
      // -----------------------------------------------------------------
      // Case-10-040630-8369
      // This may be an expression that is not transformed yet.
      // If the VEGMember is not found, return null, instead of asserting. The 
      // caller can decide what to do based on the null result.
      // -----------------------------------------------------------------
      // CMPASSERT(memberPtr);
      if (memberPtr==0)
      {
        return 0;
      }
    }
    
  // ---------------------------------------------------------------------
  // Find the VEG for the given expression
  // ---------------------------------------------------------------------
  VEG * vegPtr = memberPtr->getVEG();
  
  if (NOT vegPtr->isNormalized())
    {
      // If there are multiple constants in this VEG, the 
      // following method returns a BoolVal(RETURN_FALSE)
      ItemExpr * iePtr = processMultipleConstValuesInVEG(vegPtr->getAllValues());
      if (iePtr != NULL)  // have multiple constants in the VEG ?
	vegPtr->setVEGPredicate((VEGPredicate *)iePtr); // replace the VEGPredicate
      vegPtr->setNormalized(); // is being normalized in this call
    }

  return vegPtr->getVEGPredicate();
  
} // VEGRegion::performTC()

// -----------------------------------------------------------------------
// display operator
// -----------------------------------------------------------------------
void VEGRegion::print(FILE* ofd, const char* indent, const char* title)
{
  Lng32 ne;
  
  BUMP_INDENT(indent);
  fprintf(ofd,"************\n");
  fprintf(ofd,"%s %s[%d] %s",NEW_INDENT,title,getRegionId(),NEW_INDENT);
  if (isActive())
    fprintf(ofd,"active ");
  else if (isMerged())
    fprintf(ofd,"merged ");
  else if (isToBeMerged())
    fprintf(ofd,"to be merged ");
  fprintf(ofd,"entries(%d) parent (%d) ",members_.entries(),getParentVEGRegionId());
  fprintf(ofd,"owner (%p) ",getOwnerExpr());
  if (getOwnerExpr())
    fprintf(ofd,"is a (%s) ",getOwnerExpr()->getText().data());
  fprintf(ofd,"child (%d) ",getSubtreeId());
  fprintf(ofd,"children ");
  if (zones_.entries() == 0)
    fprintf(ofd,"(none) ");
  else
    for (ne = 0; ne < (Lng32)zones_.entries(); ne++)
      fprintf(ofd,"(%d) ",zones_[ne]->getRegionId());

  if (processingDone())
    fprintf(ofd,"processing DONE \n");
  else
    fprintf(ofd,"\n");
  fprintf(ofd,"************\n");
  for (ne = 0; ne < (Lng32)members_.entries(); ne ++)
    members_[ne]->print(ofd, indent);
} // VEGRegion::print()

// To be called from the debugger
void VEGRegion::display()
{
 VEGRegion::print();
} // VEGRegion::display()

// ***********************************************************************
// ***********************************************************************
// Methods on VEGTable
// ***********************************************************************
// ***********************************************************************

// -----------------------------------------------------------------------
// VEGTable::VEGTable() 
// -----------------------------------------------------------------------
VEGTable::VEGTable()
         : nextInSequence_(FIRST_VEG_REGION), currentRegion_(NULL),
	   arrayEntry_(CmpCommon::statementHeap())
{
}

// -----------------------------------------------------------------------
// VEGTable::allocateRegion()
// -----------------------------------------------------------------------
VEGRegion * VEGTable::allocateVEGRegion(VEGRegion * parentRegion,
					const VEGRegionTypeEnum tev,
					const ExprNode * const ownerOfRegion,
					Lng32 subtreeId)
{
  // Create the new Region.
  VEGRegion * newRegion = new(CmpCommon::statementHeap())
    VEGRegion(this,            // -> the VEGTable  
	      parentRegion,    // parent
	      nextInSequence_, // me
	      tev,             // import or import-export
	      ownerOfRegion,   // owner
	      subtreeId);     // index for its child
  // Add the new Region to the VEGTable.
  arrayEntry_.insertAt(nextInSequence_++, newRegion);
  if (getCurrentVEGRegion())
    getCurrentVEGRegion()->addZone(getCurrentVEGRegion(), newRegion);
  return newRegion;
} // VEGTable::allocateRegion()

// -----------------------------------------------------------------------
// VEGTable::locateAndSetVEGRegion()
// -----------------------------------------------------------------------
void VEGTable::locateAndSetVEGRegion(const ExprNode * const ownerExpr,
				     Lng32 subtreeId)
{
  VEGRegion * regionPtr = getVEGRegion(ownerExpr, subtreeId);

  CMPASSERT(regionPtr);

  setCurrentVEGRegion(regionPtr);
} // VEGTable::locateAndSetVEGRegion()

// -----------------------------------------------------------------------
// VEGTable::restoreOriginalRegion()
// -----------------------------------------------------------------------
void VEGTable::restoreOriginalRegion()
{
  CMPASSERT(getCurrentVEGRegion());
  setCurrentVEGRegion(getCurrentVEGRegion()->getParentVEGRegion());
} // VEGTable::restoreOriginalRegion()

// -----------------------------------------------------------------------
// VEGTable::getVEGRegion(Regionid )
// -----------------------------------------------------------------------
VEGRegion * VEGTable::getVEGRegion(const RegionId candidateRegion) const  
{
  CMPASSERT(candidateRegion >= FIRST_VEG_REGION &&
         candidateRegion < (RegionId)arrayEntry_.entries() ); 

  CMPASSERT(arrayEntry_[candidateRegion]); // VEGRegion allocated

  return (arrayEntry_[candidateRegion]);

} // VEGTable::getVEGRegion()

// -----------------------------------------------------------------------
// VEGTable::getVEGRegion(ValueId )
//   Loops through the VEGRegions to find the first region that
//   contains a given valueid
// -----------------------------------------------------------------------
VEGRegion * VEGTable::getVEGRegion(const ValueId exprId) const 
{
  VEGRegion* candidateRegion = NULL;
  VEGMember * memberPtr;
  
  // Loop through the regions to find the first one for this ItemExpr
  for (RegionId i = FIRST_VEG_REGION;
       i < (RegionId)arrayEntry_.entries();
       i++)
    {
      candidateRegion = arrayEntry_[i];
      memberPtr = candidateRegion->findVEGMember(exprId);
      if (memberPtr)
	return candidateRegion;
    }

    // Didn't find any
  return NULL;
} // VEGTable::getVEGRegion()

// -----------------------------------------------------------------------
// VEGTable::getVEGRegion(ExprNode *)
// -----------------------------------------------------------------------
VEGRegion * VEGTable::getVEGRegion(const ExprNode * const ownerExpr,
				   Lng32 subtreeId) const
{
  VEGRegion* candidateRegion = NULL;
  const ExprNode * ownerOrOrig = ownerExpr;
  
  // Loop over the node and its original expressions
  while (ownerOrOrig)
    {
      // Loop through the regions to find the one for this ExprNode
      for (RegionId i = FIRST_VEG_REGION;
           i < (RegionId)arrayEntry_.entries();
           i++)
        {
          candidateRegion = arrayEntry_[i];
          if ( (candidateRegion->getOwnerExpr() == ownerOrOrig) AND 
               (candidateRegion->getSubtreeId() == subtreeId))
            return candidateRegion;
        }

      // VEGRegion not found, try with one of the original expressions
      const RelExpr *re = ownerOrOrig->castToRelExpr();

      if (re && re->getOriginalExpr(FALSE) != re)
        ownerOrOrig = re->getOriginalExpr(FALSE);
      else
        ownerOrOrig = NULL;
    }

    // Didn't find any
  return NULL;
}

// -----------------------------------------------------------------------
// VEGTable::getVEGReference()
// Rules:
// 1) If the given expression belongs to a VEG in the current Region,
//    return a pointer to its VEGReference.
// 2) Otherwise, return a NULL pointer.
// -----------------------------------------------------------------------
ItemExpr * VEGTable::getVEGReference(const ValueId & exprId,
                       const VEGRegion *searchThisVegRegionFirst) const
{
  CMPASSERT(getCurrentVEGRegion()); // assert that we have a Region

#if 0
  // ---------------------------------------------------------------------
  //  find the vid for the replacement expression.
  // This is part of the changes necessary to rewrite null-inst values
  // into VEGRef. Unfortunately, there is some code in histogram/costing
  // which relies on the fact that null-inst values are not rewritten in
  // terms of VEGRef. Code commented out for now pending a more detailed
  // study.
  // ---------------------------------------------------------------------

  ItemExpr * replacedExpr = exprId.getItemExpr()->getReplacementExpr();
  if (replacedExpr)
  {
    if (replacedExpr->getValueId() != exprId)
      return getVEGReference(replacedExpr->getValueId());
  }
#endif

  // ---------------------------------------------------------------------
  // Check whether the given value is a member of the current VEGRegion.
  // ---------------------------------------------------------------------
  VEGReference * vegRefPtr = 
    getCurrentVEGRegion()->getVEGReferenceFromCurrentVEGRegion(exprId,
                                               NULL,
                                               searchThisVegRegionFirst);

  // ---------------------------------------------------------------------
  // If the given value is not a member of the current VEGRegion and
  // a parent VEGRegion exists, check whether it is a member of 
  // the parent VEGRegion.
  // ---------------------------------------------------------------------
  if ((vegRefPtr == NULL) AND 
      (getCurrentVEGRegion()->getParentVEGRegion())) // search in parent VEGRegions.
    {

      vegRefPtr = getCurrentVEGRegion()->getVEGReferenceFromParentVEGRegion
	                                     (exprId);

      // -----------------------------------------------------------------
      // If the given value is a member of a parent VEGRegion, then check
      // whether its VEGReference is a member of the current VEGRegion.
      // If so, return the VEGReference for the VEG that it belongs to 
      // in the current VEGRegion.
      // -----------------------------------------------------------------
      if (vegRefPtr)
	{
	  VEGReference * vegRef2Ptr = 
	    getCurrentVEGRegion()
	     ->getVEGReferenceFromCurrentVEGRegion(vegRefPtr->getValueId());
	  // -------------------------------------------------------------
	  // If the VEGReference from a parent VEGRegion is itself a
	  // member of the current VEGRegion, return the VEGReference 
	  // for the VEG that it belongs to in the current VEGRegion. 
	  // -------------------------------------------------------------
	  if (vegRef2Ptr) 
	    vegRefPtr = vegRef2Ptr;
	}
    } // endid

  // --------------------------------------------------------------------
  // If the VEG contains a constant we will return the constant
  // --------------------------------------------------------------------
//	  ItemExpr * itemConstantPtr;
//	  if (vegRefPtr->getVEG()->getAllValues().referencesAConstValue(
//	                                               &itemConstantPtr))
//	    return itemConstantPtr;

  return vegRefPtr;
  
} // VEGTable::getVEGReference()

void VEGTable::deleteVEGMember(const ValueId &vId)
{
  CMPASSERT(getCurrentVEGRegion()); // assert that we have a Region
  getCurrentVEGRegion()->deleteVEGMember(vId);
}

// -----------------------------------------------------------------------
// VEGTable::addVEG(const ValueId &, const ValueId &)
// -----------------------------------------------------------------------
void VEGTable::addVEG(const ValueId & expr1Id, const ValueId & expr2Id)
{
  CMPASSERT(getCurrentVEGRegion()); // assert that we have a Region
  getCurrentVEGRegion()->addVEG(expr1Id,expr2Id);
} // VEGTable::addVEG()

// -----------------------------------------------------------------------
// VEGTable::addVEG(const ValueIdSet &)
// -----------------------------------------------------------------------
void VEGTable::addVEG(const ValueIdSet & setOfValues)
{
  CMPASSERT(getCurrentVEGRegion()); // assert that we have a Region
  getCurrentVEGRegion()->addVEG(setOfValues);
} // VEGTable::addVEG()

// -----------------------------------------------------------------------
// VEGTable::addVEGInOuterRegion(const ValueId &, const ValueId &)
//    Find the first region than contains a VEGMember for exprr1Id
//     or use the current region if none was found.
//    On that region add the VEG.
//
// Used when we are aliasing or equivalencing a valueId to another that
// came from an outer region.
// -----------------------------------------------------------------------
void VEGTable::addVEGInOuterRegion(const ValueId & expr1Id, const ValueId & expr2Id)
{
  VEGRegion * regPtr = getVEGRegion(expr1Id);
  if (regPtr == NULL)
    regPtr = getCurrentVEGRegion();

  CMPASSERT(regPtr); // assert that we have a Region
  regPtr->addVEG(expr1Id,expr2Id);
} // VEGTable::addVEG()

// -----------------------------------------------------------------------
// VEGTable::locateVEGRegionAndMarkToBeMerged()
// -----------------------------------------------------------------------
VEGRegion * VEGTable::locateVEGRegionAndMarkToBeMerged(const ValueId & exprId)
{
  CMPASSERT(getCurrentVEGRegion()); // assert that we have a Region
  NABoolean found = FALSE;
  VEGRegion * regPtr;
  for (RegionId index = FIRST_VEG_REGION;
       index < (Lng32)arrayEntry_.entries();
       index++)
    {
      regPtr = arrayEntry_[index];

      ValueId childVid = exprId.getItemExpr()->child(0)->getValueId();

      CMPASSERT("childVid not found for the case of NullInst.");

      const RelExpr * owner = regPtr->getOwnerExpr()->castToRelExpr();

      if ( owner AND 
	   owner->getGroupAttr()->getCharacteristicOutputs().contains(exprId))
	{
	  // If the owner is FOJ - Full Outer Join, then 
	  // we need to further find out which of the children 
	  // produce exprId as output. Mark the region associated
	  // with that child as "to be merged".
	  if (owner->getOperatorType() == REL_FULL_JOIN)
	    {
	      //regPtr->getParentVEGRegion()
              // ->locateDescendantVEGRegionAndMarkToBeMerged(owner, childVid); 
              return NULL;
	    }
	  else
	    regPtr->markAsToBeMerged();
 	  return regPtr;
	}
    }

 DisplayVid(exprId);
 CMPASSERT(found);		// MUST have the ValueId in the VEGTable
 return NULL;
} // VEGTable::locateVEGRegionAndMarkToBeMerged()


void VEGRegion::locateDescendantVEGRegionAndMarkToBeMerged(const RelExpr *owner, 
							   const ValueId &exprId)
{
  CMPASSERT (owner->getOperatorType() == REL_FULL_JOIN);
  
  Lng32 arity = owner->getArity();

  for (Lng32 index = 0; index < arity; index++)
    {
      if (owner->child(index)->getGroupAttr()->getCharacteristicOutputs().contains(exprId))
	{
	zones_[index]->markAsToBeMerged(); // note, there is 1:1 correspondence
                                           // between zones and children. For example.
                                           // zones_[0] corresponds to child(0),
                                           // zones_[1] corresponds to child(1)
	break;
	}
    }
}

// -----------------------------------------------------------------------
// VEGTable::processVEGRegions()
// -----------------------------------------------------------------------
void VEGTable::processVEGRegions()
{
  // ---------------------------------------------------------------------
  // In the first pass over the VEGTable, process each VEGRegion that 
  // was allocated by a Union operator. We shall such a VEGRegion a
  // "Union child VEGRegion". It is distinguished by the fact that a 
  // Union operator is its owner expression. 
  // If any output value of the Union parent is a member of the parent
  // VEGRegion, then allocate a new VEG in the Union child VEGRegion. 
  // The new VEG will contain all those values as members that belong to
  // the same VEG as the output value in the parent VEGRegion, provided 
  // they are also available in the Union child VEGRegion. 
  // ---------------------------------------------------------------------
  RegionId index = FIRST_VEG_REGION;
  for (; index < (Lng32)arrayEntry_.entries(); index++)
    arrayEntry_[index]->importVEGsForUnionChildVEGRegion();

  // ---------------------------------------------------------------------
  // In the second pass over the VEGTable, perform the following steps:
  // 1) For each Union child VEGRegion, delete the output values of the
  //    Union parent that are members of its parent VEGRegion. This
  //    is done in order to ensure that VEGPredicates and VEGReferences
  //    are not generated when an expression contains such a value.
  // 2) Merge all Zones that are marked "To Be Merged".
  // 3) Replace each member of a VEGRegion that is referenced from its
  //    parent VEGRegion with a VEGReference.
  // ---------------------------------------------------------------------
  for (index = FIRST_VEG_REGION;
       index < (RegionId)arrayEntry_.entries();
       index++)
    arrayEntry_[index]->mergeZonesAndMakeReferencesConsistent();

} // VEGTable::processVEGRegions()
  
// -----------------------------------------------------------------------
// display operator
// -----------------------------------------------------------------------
void VEGTable::print(FILE* ofd, const char* indent, const char* title)
{
  BUMP_INDENT(indent);
  fprintf(ofd,"%s %s %s",NEW_INDENT,title,NEW_INDENT);
  if (getCurrentVEGRegion())
    {
      fprintf(ofd,"nextInSequence = %d\n>>>> Current Region is Region[%d] <<<<\n", 
	      nextInSequence_,getCurrentVEGRegion()->getRegionId());

      Lng32 ne = arrayEntry_.entries();
      // Print all the regions
      for (RegionId index = FIRST_VEG_REGION;
           index < (RegionId)arrayEntry_.entries();
           index++)
	arrayEntry_[index]->print(ofd, indent);
    }  
  else
    fprintf(ofd,"is empty\n");
} // VEGTable::print()

// To be called from the debugger
void VEGTable::display()
{
 VEGTable::print();
} // VEGTable::display()
