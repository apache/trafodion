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
* File:         PartReq.cpp
* Description:  Partitioning Requirements
* Created:      11/16/1994
* Language:     C++
*
*
*
*
**************************************************************************
*/

#include "PartReq.h"
#include "ItemColRef.h"
#include "ItemLog.h"
#include "ItemFunc.h"
#include "ReqGen.h"
#include "GroupAttr.h"
#include "RelScan.h"
#include "opt.h"
#include "str.h"
#include "NumericType.h"
#include "MiscType.h"
#include "NAFileSet.h"


// ***********************************************************************
// PartitioningRequirement
// ***********************************************************************

PartitioningRequirement::~PartitioningRequirement() {}

// ---------------------------------------------------------------------
// Methods for performing type-safe pointer casts.
// ---------------------------------------------------------------------
const FuzzyPartitioningRequirement*
PartitioningRequirement::castToFuzzyPartitioningRequirement() const
  { return NULL; }

const FullySpecifiedPartitioningRequirement*
PartitioningRequirement::castToFullySpecifiedPartitioningRequirement() const
  { return NULL; }

const RequireApproximatelyNPartitions*
PartitioningRequirement::castToRequireApproximatelyNPartitions() const
  { return NULL; }

const RequireExactlyOnePartition*
PartitioningRequirement::castToRequireExactlyOnePartition() const
  { return NULL; }

const RequireReplicateViaBroadcast*
PartitioningRequirement::castToRequireReplicateViaBroadcast() const
  { return NULL; }

const RequireReplicateNoBroadcast*
PartitioningRequirement::castToRequireReplicateNoBroadcast() const
  { return NULL; }

const RequireHash*
PartitioningRequirement::castToRequireHash() const
  { return NULL; }

const RequireHashDist*
PartitioningRequirement::castToRequireHashDist() const
  { return NULL; }

const RequireHash2*
PartitioningRequirement::castToRequireHash2() const
  { return NULL; }

const RequireSkewed*
PartitioningRequirement::castToRequireSkewed() const
  { return NULL; }

const RequireRange*
PartitioningRequirement::castToRequireRange() const
  { return NULL; }

const RequireRoundRobin*
PartitioningRequirement::castToRequireRoundRobin() const
  { return NULL; }

const RequireHive*
PartitioningRequirement::castToRequireHive() const
  { return NULL; }


NABoolean PartitioningRequirement::partitioningKeyIsSpecified() const
{
  ABORT("PartitioningRequirement::partitioningKeyIsSpecified() needs to be redefined");
  return FALSE;
}

PartitioningFunction * PartitioningRequirement::realize(
       const Context *myContext,
       NABoolean useContextPartitioningRequirements,
       const PartitioningRequirement * softRequirements) 
{
  ABORT("Redefine PartitioningRequirement::realize()");
  return NULL;
}

const NAString PartitioningRequirement::getText() const
{
  if (castToFullySpecifiedPartitioningRequirement())
    {
      return castToFullySpecifiedPartitioningRequirement()->
	getPartitioningFunction()->getText();
    }
  else
    {
      NAString result(CmpCommon::statementHeap());

      if (getCountOfPartitions() != ANY_NUMBER_OF_PARTITIONS)
	{
	  char numString[30];
	  if (castToRequireApproximatelyNPartitions())
	    {
	      Lng32 lo, hi;
	      hi = getCountOfPartitions();
	      lo = hi - (Lng32) (hi * castToRequireApproximatelyNPartitions()-> 
		getAllowedDeviation());
	      // if rounded down we may have to add one
	      if (NOT castToRequireApproximatelyNPartitions()->
		  isPartitionCountWithinRange(lo) OR
		  lo == 0)
		lo++;
	      sprintf(numString,"%d...%d partns. ",lo,hi);
	      result += numString;
	    }
	  else
	    {
	      sprintf(numString,"%d partns. ", getCountOfPartitions());
	      result += numString;
	    }
	}
      
      if (NOT getPartitioningKey().isEmpty())
	{
	  if (result.length())
	    result += " on (";
	  else
	    result += "part. on (";
	  ((ValueIdSet &)(getPartitioningKey())).unparse(result,
							 DEFAULT_PHASE,
							 EXPLAIN_FORMAT);
	  result += ")";
	}
      return result;
    }
}

// -----------------------------------------------------------------------
// PartitioningRequirement::copyAndRemap()
// -----------------------------------------------------------------------
PartitioningRequirement* 
PartitioningRequirement::copyAndRemap
                         (ValueIdMap& map, NABoolean mapItUp) const
{
  if (getPartitioningKey().entries() == 0)
    return (PartitioningRequirement*)this;
  PartitioningRequirement* newPartReq = copy(); // virtual copy constructor
  newPartReq->remapIt(this, map, mapItUp);
  return newPartReq;
} // PartitioningRequirement::copyAndRemap() 

// -----------------------------------------------------------------------
// PartitioningRequirement::remapIt()
// -----------------------------------------------------------------------
void PartitioningRequirement::remapIt
                             (const PartitioningRequirement* opr,
			      ValueIdMap& map, NABoolean mapItUp)
{
  ABORT("Redefine PartitioningRequirement::remapIt()");

} // PartitioningRequirement::remapIt()

PartitioningRequirement* 
PartitioningRequirement::copy() const  
{ 
  ABORT("Redefine PartitioningRequirement::copy()");
  return NULL;
}

void PartitioningRequirement::print(FILE* ofd, const char* indent, 
				    const char* title) const
{
  print(ofd, indent, title);
} 


void PartitioningRequirement::display() const  { print(); }

Lng32 PartitioningRequirement::getCountOfPartitions() const   
{ 
  ABORT("Redefine PartitioningRequirement::getCountOfPartitions()");
  return 1; 
}

const ValueIdSet &PartitioningRequirement::getPartitioningKey() const
{ 
  static ValueIdSet nothing;
  ABORT("Redefine PartitioningRequirement::getPartitioningKey()");
  return nothing; 
}

NABoolean PartitioningRequirement::partReqAndFuncCompatible
                          (const PartitioningFunction* other) const
{
  ABORT("Redefine PartitioningRequirement::partReqAndFuncCompatible()");
  return FALSE;
}
                                         
COMPARE_RESULT PartitioningRequirement::comparePartReqToReq
                          (const PartitioningRequirement* other) const
{
  ABORT("Redefine PartitioningRequirement::comparePartReqToReq()");
  return INCOMPATIBLE;
}
                                         
// ***********************************************************************
// FuzzyPartitioningRequirement
// ***********************************************************************

// -----------------------------------------------------------------------
// Method for performing a pointer type cast
// -----------------------------------------------------------------------
const FuzzyPartitioningRequirement*
FuzzyPartitioningRequirement::castToFuzzyPartitioningRequirement() const
  { return this; }

NABoolean
FuzzyPartitioningRequirement::partReqAndFuncCompatible
                          (const PartitioningFunction* other) const
{
  ABORT("Redefine FuzzyPartitioningRequirement::partReqAndFuncCompatible()");
  return FALSE;
  
} // FuzzyPartitioningRequirement::partReqAndFuncCompatible()

COMPARE_RESULT FuzzyPartitioningRequirement::comparePartReqToReq
                          (const PartitioningRequirement* other) const
{
  ABORT("Redefine FuzzyPartitioningRequirement::comparePartReqToReq()");
  return INCOMPATIBLE;
}
                                         
// -----------------------------------------------------------------------
// FuzzyPartitioningRequirement::remapIt()
// -----------------------------------------------------------------------
void FuzzyPartitioningRequirement::remapIt
                             (const PartitioningRequirement* opr,
			      ValueIdMap& map, NABoolean mapItUp)
{
  // Clear because rewrite insists on it being so.
  partitioningKeyColumns_.clear();

  if (mapItUp)
      map.rewriteValueIdSetUp(partitioningKeyColumns_, 
                              opr->castToFuzzyPartitioningRequirement()->
			      partitioningKeyColumns_);
  else
      map.rewriteValueIdSetDown(opr->castToFuzzyPartitioningRequirement()->
                                partitioningKeyColumns_, 
				partitioningKeyColumns_);

} // FuzzyPartitioningRequirement::remapIt()

PartitioningRequirement* 
FuzzyPartitioningRequirement::copy() const  
{ 
  ABORT("Redefine FuzzyPartitioningRequirement::copy()");
  return NULL;
}

void FuzzyPartitioningRequirement::print(FILE* ofd, const char* indent, 
                                         const char* title) const
{
  PartitioningRequirement::print(ofd, indent, title);
} 

void FuzzyPartitioningRequirement::display() const  { print(); }

// ***********************************************************************
// FullySpecifiedPartitioningRequirement
// ***********************************************************************


// -----------------------------------------------------------------------
// Method for performing a pointer type cast
// -----------------------------------------------------------------------
const FullySpecifiedPartitioningRequirement*
FullySpecifiedPartitioningRequirement::
  castToFullySpecifiedPartitioningRequirement() const
  { return this; }

// -----------------------------------------------------------------------
// FullySpecifiedPartitioningRequirement::partReqAndFuncCompatible()
// Partitioning function comparison method for all fully specified
// partitioning requirements. 
// "Other" must be a synthesized partitioning function.
// -----------------------------------------------------------------------
NABoolean
FullySpecifiedPartitioningRequirement::partReqAndFuncCompatible
                          (const PartitioningFunction* other) const
{
  PartitioningFunction* myPartFunc = getPartitioningFunction();
  
  // If my requirement does not require a log phys part func,
  // then if the part func is a log phys dig underneath to
  // get the physical part func. We want to compare against
  // the physical part func because if the synthesized part func
  // is a log phys it means we are in dp2, and in dp2 all part
  // requirements must be physical requirements.
  if (other->isALogPhysPartitioningFunction() AND
      NOT myPartFunc->isALogPhysPartitioningFunction())
  {
    other = other->castToLogPhysPartitioningFunction()
      ->getPhysPartitioningFunction();
  }

  // Call the method that compares two part funcs to do all the
  // work in determining if the part req and func are compatible.
  // This method has various virtual implementations for all types of
  // part funcs so we avoid a lot of duplicated code by using it here.
  return (myPartFunc->comparePartFuncToFunc(*other) == SAME);
  
} // FullySpecifiedPartitioningRequirement::partReqAndFuncCompatible

// -----------------------------------------------------------------------
// Comparison method for comparing a fully specified partitioning 
// requirement against another partitioning requirement.
// -----------------------------------------------------------------------
COMPARE_RESULT
FullySpecifiedPartitioningRequirement::comparePartReqToReq
                          (const PartitioningRequirement* other) const
{
  // Is other a fuzzy requirement?
  if ( other->isRequirementFuzzy() ) 

  {
    // Yes.
    CMPASSERT(other->isRequirementApproximatelyN());

    ValueIdSet myPartKey = getPartitioningKey();
    ValueIdSet otherPartKey = other->getPartitioningKey();
    Lng32 myKeyCount = myPartKey.entries();
    Lng32 otherKeyCount = otherPartKey.entries();
    Lng32 myPartCount = getCountOfPartitions();
    Lng32 otherPartCount = other->getCountOfPartitions();
    float otherAllowedDeviation = 
            other->castToRequireApproximatelyNPartitions()->
                   getAllowedDeviation();
    COMPARE_RESULT result;

    // A fully specified requirement and a fuzzy requirement can never be
    // the SAME. At best, a fully specified requirement is MORE than a fuzzy
    // requirement.
    result = MORE;

    // Compare the required number of partitions.
    if ( (otherPartCount == ANY_NUMBER_OF_PARTITIONS) OR
         ((myPartCount >= (otherPartCount - 
                           (otherPartCount * otherAllowedDeviation))) AND
          (myPartCount <= otherPartCount)) )
      result = combine_compare_results(result,MORE);
    else
      result = combine_compare_results(result,INCOMPATIBLE);

    // Compare the required partitioning keys. 
    if (myKeyCount > 0) 
    {
      // The fuzzy req. specified required partitioning key columns, so 
      // the required key columns from the fully specified requirement must
      // be a subset of the fuzzy required key columns, in which case
      // the fully specified requirement requires MORE. Otherwise, they     
      // are INCOMPATIBLE. Note we come here even if the fully specified 
      // requirement specified no key columns. This is ok - the empty
      // set is a subset of anything, so the result will be MORE.
      if (otherPartKey.contains(myPartKey))
        result = combine_compare_results(result,MORE);
      else
        result = combine_compare_results(result,INCOMPATIBLE);
    }
    // If the fuzzy requirement specifies no partitioning key columns,
    // the fully specified requirement requires MORE than the fuzzy req.
    else 
      result = combine_compare_results(result,MORE);

    return result;
  } // end if other is a fuzzy requirement
  else // both are fully specified requirements
  {

    PartitioningFunction* myPartFunc = getPartitioningFunction();
    PartitioningFunction* otherPartFunc = 
      other->castToFullySpecifiedPartitioningRequirement()
           ->getPartitioningFunction();

    return myPartFunc->comparePartFuncToFunc(*otherPartFunc);
  } // end if both are fully specified requirements

} // FullySpecifiedPartitioningFunction::comparePartReqToReq

PartitioningFunction * FullySpecifiedPartitioningRequirement::realize(
     const Context *myContext,
     NABoolean useContextPartitioningRequirements,
     const PartitioningRequirement * softRequirements)
{
  // ---------------------------------------------------------------------
  // There is only one way to realize a fully specified partitioning
  // function: to return the actual function. However, we must make sure
  // that the actual function satisfies the requirements from the
  // context.
  // ---------------------------------------------------------------------
  if (NOT useContextPartitioningRequirements OR
      myContext->getReqdPhysicalProperty()->
      getPartitioningRequirement() == NULL OR
      myContext->getReqdPhysicalProperty()->getPartitioningRequirement()->
      castToFullySpecifiedPartitioningRequirement() == this OR
      myContext->getReqdPhysicalProperty()->
	  getPartitioningRequirement()->partReqAndFuncCompatible(
	       partitioningFunction_)
     ) 
    return partitioningFunction_;
  else
    return NULL; // context requirements conflict with partitioningFunction_
}

// -----------------------------------------------------------------------
// FullySpecifiedPartitioningRequirement::remapIt()
// -----------------------------------------------------------------------
void FullySpecifiedPartitioningRequirement::remapIt
                        (const PartitioningRequirement* opr,
                         ValueIdMap& map, NABoolean mapItUp)
{
  CMPASSERT(opr->castToFullySpecifiedPartitioningRequirement()); // I am pretty sure this will die every time
  partitioningFunction_ = opr->castToFullySpecifiedPartitioningRequirement()->
                          partitioningFunction_->copyAndRemap(map,mapItUp);

} // FullySpecifiedPartitioningRequirement::remapIt()

PartitioningRequirement* 
FullySpecifiedPartitioningRequirement::copy() const  
{ 
  ABORT("Redefine FullySpecifiedPartitioningRequirement::copy()");
  return NULL;
}

void FullySpecifiedPartitioningRequirement::print
       (FILE* ofd, const char* indent, const char* title) const
{
  PartitioningRequirement::print(ofd, indent, title);
} 

void FullySpecifiedPartitioningRequirement::display() const  { print(); }

// -----------------------------------------------------------------------
// RequireApproximatelyNPartitions
// -----------------------------------------------------------------------
RequireApproximatelyNPartitions::RequireApproximatelyNPartitions(
     const ValueIdSet& partitioningKeyColumns,
     float numOfPartsAllowedDeviation,
     Lng32 numberOfPartitions,
     NABoolean requireHash2Only
     ,const skewProperty& sk
                                                                 )
     : FuzzyPartitioningRequirement(APPROXIMATELY_N_PART_REQ,
                                    partitioningKeyColumns,
                                    numberOfPartitions
                                    , sk
                                    ),
       numOfPartsAllowedDeviation_ (numOfPartsAllowedDeviation),
       requireHash2Only_ (requireHash2Only)
{

  CMPASSERT(numOfPartsAllowedDeviation_ >= 0.0 AND
            numOfPartsAllowedDeviation_ <= 1.0);

  CMPASSERT(numberOfPartitions > 0 OR
	    numberOfPartitions == ANY_NUMBER_OF_PARTITIONS);
}

RequireApproximatelyNPartitions::RequireApproximatelyNPartitions(
     float numOfPartsAllowedDeviation,
     Lng32 numberOfPartitions,
     NABoolean requireHash2Only
     ,const skewProperty& sk
                                                                 )
     : FuzzyPartitioningRequirement(APPROXIMATELY_N_PART_REQ,
                                    numberOfPartitions, sk
                                    ),
       numOfPartsAllowedDeviation_ (numOfPartsAllowedDeviation),
       requireHash2Only_ (requireHash2Only)
{

  CMPASSERT(numOfPartsAllowedDeviation_ >= 0.0 AND
            numOfPartsAllowedDeviation_ <= 1.0);

  CMPASSERT(numberOfPartitions > 0 OR
	    numberOfPartitions == ANY_NUMBER_OF_PARTITIONS);
}

// -----------------------------------------------------------------------
// Method for performing a pointer type cast
// -----------------------------------------------------------------------
const RequireApproximatelyNPartitions*
RequireApproximatelyNPartitions::
  castToRequireApproximatelyNPartitions() const
  { return this; }

// -----------------------------------------------------------------------
// RequireApproximatelyNPartitions::partReqAndFuncCompatible()
// Partitioning function comparison method for comparing a partitioning
// requirement that specifies an allowable deviation for the number of
// partitions against a synthesized partitioning function.
// The number of partitions that the synthesized partitioning function
// exhibits must be equal to the required number of partitions, or
// a fewer number of partitions that is within the allowed deviation range.
// -----------------------------------------------------------------------
NABoolean
RequireApproximatelyNPartitions::partReqAndFuncCompatible
                          (const PartitioningFunction* other) const
{
  CMPASSERT(other != NULL);
  
  Lng32 reqKeyCount = getPartitioningKey().entries();
  Lng32 reqPartCount = getCountOfPartitions();
  Lng32 actualPartCount = other->getCountOfPartitions();
  float reqAllowedDeviation = getAllowedDeviation();

  if (isRequireHash2Only() && !other->isAHash2PartitioningFunction())
    return FALSE;

  // Make sure actual number of partitions satisfies this requirement.
  if ( NOT isPartitionCountWithinRange(actualPartCount) )  
  {
    return FALSE; 
  }

  // Check if the actual partitioning key columns are compatible
  // with the required partitioning key columns.
  if (reqKeyCount == 0) 
    // no required part key columns
    return TRUE;

  if ( getSkewProperty().isAnySkew() ) {
     // If no specific skew requirement is present, requiring the partkey has
     // the complete containment relationship with the partition func.
     return (getPartitioningKey().contains(other->getPartitioningKey()));
  } else {

     // If the skew requirement is present, requiring that the skew property
     // matches
     if ((NOT other->isASkewedDataPartitioningFunction()) OR
         (NOT (getSkewProperty() ==
            other->castToSkewedDataPartitioningFunction()->getSkewProperty()))
        )
       return FALSE;

    return (getPartialPartitioningKey() == other->getPartialPartitioningKey());
  }

  return FALSE;
  
} // RequireApproximatelyNPartitions::partReqAndFuncCompatible()

// -----------------------------------------------------------------------
// RequireApproximatelyNPartitions::comparePartReqToReq()
// Partitioning function comparison method for comparing a partitioning
// requirement that is not an actual partitioning function against
// another partitioning requirement. Used when comparing two different
// partitioning requirements from two different contexts.
// -----------------------------------------------------------------------
COMPARE_RESULT
RequireApproximatelyNPartitions::comparePartReqToReq
                          (const PartitioningRequirement* other) const
{
  CMPASSERT(other != NULL);
  
  ValueIdSet myPartKey = getPartitioningKey();
  ValueIdSet otherPartKey = other->getPartitioningKey();
  Lng32 myKeyCount = myPartKey.entries();
  Lng32 otherKeyCount = otherPartKey.entries();
  Lng32 myPartCount = getCountOfPartitions();
  Lng32 otherPartCount = other->getCountOfPartitions();
  float myAllowedDeviation = getAllowedDeviation();
  float myLowerBound = myPartCount - (myPartCount * myAllowedDeviation);

  COMPARE_RESULT result;
  
  // If other is a replication requirement, then the result must be            
  // INCOMPATIBLE, because replication requirements are only compatible 
  // with themselves.
  if (other->isRequirementReplicateViaBroadcast() OR
      other->isRequirementReplicateNoBroadcast())
    result = INCOMPATIBLE;
  else if (other->isRequirementFullySpecified())
  {
    // A fuzzy requirement and a fully specified requirement can never be
    // the SAME. At best, a fuzzy requirement is LESS than a fully specified
    // requirement.
    result = LESS;

    // Compare the required number of partitions.
    if ( (myPartCount == ANY_NUMBER_OF_PARTITIONS) OR
         ((otherPartCount >= myLowerBound) AND
          (otherPartCount <= myPartCount)) )
      result = combine_compare_results(result,LESS);
    else
      result = combine_compare_results(result,INCOMPATIBLE);

    // Compare the required partitioning keys. 
    if (myKeyCount > 0)
    {
      // The fuzzy requirement specified required partitioning key columns, 
      // so the required key columns from the fully specified requirement 
      // must be a subset of the fuzzy required key columns, in which case
      // the fuzzy requirement requires LESS. Otherwise, they are
      // INCOMPATIBLE. Note we come here even if the fully specified 
      // requirement specified no key columns. This is ok - the empty
      // set is a subset of anything, so the result will be LESS.
      if (myPartKey.contains(otherPartKey))
        result = combine_compare_results(result,LESS);
      else
        result = combine_compare_results(result,INCOMPATIBLE);
    }
    // If the fuzzy requirement specifies no partitioning key columns,
    // the fuzzy requirement requires LESS than the fully specified req.
    else 
      result = combine_compare_results(result,LESS);

    // specifically check when the other is a required Skew requirement
    if ( other->isRequirementSkewed() AND
         NOT (getSkewProperty() ==
              ((const RequireSkewed*)(other))->getSkewProperty()
             )
       )
       result = combine_compare_results(result, INCOMPATIBLE);


    // If the fuzzy requirement requires hash2 and the part func
    // of the fully specified requirement is not hash2, then they 
    // are INCOMPATIBLE.
    PartitioningFunction* opf = 
      other->castToFullySpecifiedPartitioningRequirement()
           ->getPartitioningFunction();
    if (isRequireHash2Only() AND !opf->isAHash2PartitioningFunction())
      result = combine_compare_results(result, INCOMPATIBLE);
  }
  else // other requirement is also a fuzzy requirement
  {
    CMPASSERT(other->isRequirementApproximatelyN());

    // Start off assuming the two are the SAME.
    result = SAME;

    // The two must be the same type of fuzzy requirement.
    // (Today there is only one kind).
    if (getPartitioningRequirementType() != 
        other->castToFuzzyPartitioningRequirement()
             ->getPartitioningRequirementType())
      result = INCOMPATIBLE;

    float otherAllowedDeviation =
            other->castToRequireApproximatelyNPartitions()->
                   getAllowedDeviation();
    float otherLowerBound =
            otherPartCount - (otherPartCount * otherAllowedDeviation);

    if (myPartCount == otherPartCount)
    {
      if ((myPartCount == ANY_NUMBER_OF_PARTITIONS) OR 
          (myAllowedDeviation == otherAllowedDeviation))
        result = combine_compare_results(result,SAME); 
      else if (myAllowedDeviation < otherAllowedDeviation)
        result = combine_compare_results(result, MORE); 
      else // (my AllowedDeviation > otherAllowedDeviation)
        result = combine_compare_results(result, LESS); 
    }
    else if (myPartCount == ANY_NUMBER_OF_PARTITIONS)
      result = combine_compare_results(result, LESS); 
    else if (otherPartCount == ANY_NUMBER_OF_PARTITIONS)
      result = combine_compare_results(result, MORE); 
    else // # of parts are different and neither said they didn't care
    {
      if (myPartCount > otherPartCount) // my # of parts is largest
      {
        if (otherPartCount >= myLowerBound) //overlap
        {
          // Only if other's lower bound is greater than my lower bound
          // can we definitely say LESS.
          // Otherwise, it might be possible for a plan for other to not
          // satisfy our requirements, if the # of parts for that plan
          // was not in the overlap region.
          if (otherLowerBound >= myLowerBound)
            result = combine_compare_results(result, LESS); 
          else
            // Other's plan may be in the overlap region, or may not.
            // We don't know - so the result is UNDEFINED.
            result = combine_compare_results(result, UNDEFINED); 
        }
        else
          // No overlap - so they are INCOMPATIBLE. 
          result = combine_compare_results(result, INCOMPATIBLE); 
      }
      else // other's # of parts is larger
      {
        if (myPartCount >= otherLowerBound) //overlap
        {
          // Only if my lower bound is greater than other's lower bound
          // can we definitely say MORE.
          // Otherwise, it might be possible for a plan for me to not
          // satisfy other's requirements, if the # of parts for my plan
          // was not in the overlap region.
          if (myLowerBound >= otherLowerBound)
            result = combine_compare_results(result, MORE); 
          else
            // My plan may be in the overlap region, or may not.
            // We don't know - so the result is UNDEFINED.
            result = combine_compare_results(result, UNDEFINED); 
        }
        else
          // No overlap - so they are INCOMPATIBLE. 
          result = combine_compare_results(result, INCOMPATIBLE); 
      }
    } // end if # of parts are different

    // Check if the required partitioning key columns are compatible.
    if ((myKeyCount > 0) AND (otherKeyCount > 0))
    // both partitioning requirements have non-empty required part keys
    {
      if (myPartKey.contains(otherPartKey) AND
          otherPartKey.contains(myPartKey))
        // Other is a subset of me, and I am a subset of other,   
        // so we contain the exact same columns (although not necessarily
        // in the same order). So we both require the same thing.
        result = combine_compare_results(result,SAME);
      else if (myPartKey.contains(otherPartKey))
        // Other is a subset of me, so I require less
        result = combine_compare_results(result,LESS);
      else if (otherPartKey.contains(myPartKey))
        // I am a subset of other, so I require more
        result = combine_compare_results(result,MORE);
      else
      {
        myPartKey.intersectSet(otherPartKey);
        if (NOT myPartKey.isEmpty() OR
            (((myPartCount == ANY_NUMBER_OF_PARTITIONS) OR 
              (myLowerBound <= 1)) AND
             ((otherPartCount == ANY_NUMBER_OF_PARTITIONS) OR
              (otherLowerBound <= 1))
            )
           )
          // Overlapping part keys, or the part keys are disjoint but
          // both requirements allowed a part. func. of 1 partition.
          // There is a chance that the synthesized partitioning
          // function for other will be 1 part, which will satisfy this 
          // requirement. We don't know for sure - so the results
          // are undefined.
          result = combine_compare_results(result,UNDEFINED);
        else
          // Disjoint part keys, and the # of parts required by at least
          // one of the requirements excluded a single partition part func.
          // So the solution for other will definitely not satisfy the  
          // current requirement.
          result = combine_compare_results(result,INCOMPATIBLE);
      } // end if part keys are overlapping or disjoint
    } // end if both specified part keys
    else if ((myKeyCount == 0) AND (otherKeyCount == 0))
      // Both partitioning requirements have empty required part keys -
      // so both require the same thing.
      result = combine_compare_results(result,SAME);
    else if (myKeyCount == 0)
      // I have no required part key columns, but the other does - 
      // so I require less.
      result = combine_compare_results(result,LESS);
    else if (otherKeyCount == 0)
      // I have required part key columns, but the other does not -
      // so I require more.
      result = combine_compare_results(result,MORE);

    if ( NOT (getSkewProperty() ==
              ((const FuzzyPartitioningRequirement*)(other))->getSkewProperty()
             )
       )
       result = combine_compare_results(result, INCOMPATIBLE);


    // The other's requireHash2Only_ flag.
    NABoolean isOtherRequireHash2Only = 
      other->castToRequireApproximatelyNPartitions()->isRequireHash2Only();

    // If requireHash2Only_ flag is same for both, then we require SAME.
    if (isRequireHash2Only() == isOtherRequireHash2Only)
      result = combine_compare_results(result, SAME);
    // I dont require hash2 only and the other does, so I require LESS.
    else if (!isRequireHash2Only() AND isOtherRequireHash2Only)
      result = combine_compare_results(result, LESS);
    // I require hash2 only and the other doesn't, so I require MORE.
    else // if (isRequireHash2Only() AND !isOtherRequireHash2Only)
      result = combine_compare_results(result, MORE);

  } // end if both are fuzzy requirements
  
  return result;
  
} // RequireApproximatelyNPartitions::comparePartReqToReq()

//<pb>
//==============================================================================
//  Determine if a specified number of partitions satisfies this partitioning
// requirement.
//
// INPUT:
//  numOfParts -- specified number of partitions.
//
// OUTPUT:
//  none
//
// RETURN:
//  TRUE if specified number of partitions satisfies this requirement;
//  FALSE otherwise.
//
//==============================================================================
NABoolean 
RequireApproximatelyNPartitions::isPartitionCountWithinRange(Lng32 numOfParts)
                                                                           const
{

  Lng32 reqPartCount = getCountOfPartitions();
  
  //--------------------------------------------------------------------
  //  Any number of partitions satisfy this requirement, so return TRUE.
  //--------------------------------------------------------------------
  if (reqPartCount == ANY_NUMBER_OF_PARTITIONS)
  {
    return TRUE;
  }

  //---------------------------------------------------------------------------
  //  See if specified number of partitions fails a bounds test.  The upper
  // bound is the required partition count.  The lower bound is the upper bound
   // less a given percentage deviation of that upper bound.  The specified
  // partition must fall between the lower and upper bound (inclusive).
  //---------------------------------------------------------------------------
  float lowerBound = reqPartCount - (reqPartCount * getAllowedDeviation());
  if (   numOfParts > reqPartCount
      OR numOfParts < lowerBound   )
  {
    return FALSE; 
  }

  //--------------------------------------
  //  Passed bounds check, so return TRUE.
  //--------------------------------------
  return TRUE;

} // RequireApproximatelyNPartitions::isPartitionCountWithinRange()


// accesor method for the lower bound of the range for the 
// allowed number of partitions
Lng32 RequireApproximatelyNPartitions::getCountOfPartitionsLowBound() const
{  
    Lng32 numberOfPartitions = getCountOfPartitions();
    if ( numberOfPartitions > 0)
    {
      // this formula should be consistent with the way allowed
      // deviation is calculated. In the first version I used
      // (numberOfPartitions * (1-getAllowedDeviation()) which
      // could be a little smaller but this low bond could be
      // out of range, for example, previous formula would give 8 
      // partitions which does not satisfy 9..N fuzzy requirement
      Lng32 numOfPartsLowBound = MINOF(numberOfPartitions,
        (Lng32)ceil(numberOfPartitions * (1.001-getAllowedDeviation())));
      // make numOfPartitions as available level of parallelism 
      // 16*N, 8*N, 4*N,..., N,1 where N is the number of segments
      Lng32 i = CURRSTMT_OPTDEFAULTS->getMaximumDegreeOfParallelism(); 
      Lng32 MinParallelism = 
        MAXOF(CURRSTMT_OPTDEFAULTS->getMinimumESPParallelism(),numOfPartsLowBound);
      while(i > MinParallelism) 
        i/=2;
      numOfPartsLowBound = (i<MinParallelism) ? i*2 : i;
      
      return numOfPartsLowBound;
    }
    else
      return numberOfPartitions; 
 
} //RequireApproximatelyNPartitions::getCountOfPartitionsLowBound()

PartitioningFunction * RequireApproximatelyNPartitions::realize(
     const Context *myContext,
     NABoolean useContextPartitioningRequirements,
     const PartitioningRequirement * softRequirements) 
{
  // ---------------------------------------------------------------------
  // combine "this" and the partitioning requirement from the
  // context, if this is required
  // ---------------------------------------------------------------------
  if (useContextPartitioningRequirements AND
      myContext->requiresPartitioning())
  {
    // We have multiple partitioning requirements, "this" and
    // the additional requirement from the context.

    // add partitioning requirements of context to "this"
    RequirementGenerator rg(ExprGroupId(myContext->getGroupId()));

    rg.addPartRequirement(this);
    rg.addPartRequirement(myContext->getReqdPhysicalProperty()->
                          getPartitioningRequirement());

    // did the two requirements have a conflict?
    if (rg.checkFeasibility())
    {
      // no, call realize() recursively for the combined requirement
      ReqdPhysicalProperty *rpp = rg.produceRequirement();
      return rpp->getPartitioningRequirement()->realize(myContext,
                                                        FALSE,
                                                        softRequirements);
    }
    else
    {
      // yes, conflicting requirements, return no actual part. func.
      return NULL;
    }
  }

  // ---------------------------------------------------------------------
  // Realize a fuzzy partitioning requirement by deciding on the
  // partitioning key, the number of partitions, and the means to
  // achieve that partitioning (hash, range, ...). Ok, to be honest,
  // we only consider hash for now...
  // ---------------------------------------------------------------------
  Lng32 numOfParts =
   ( (CmpCommon::getDefault(COMP_BOOL_127) == DF_ON) AND 
     (CURRSTMT_OPTDEFAULTS->attemptESPParallelism() == DF_SYSTEM)
   ) ? getCountOfPartitionsLowBound()
       : getCountOfPartitions();
  
  ValueIdSet partKey(getPartitioningKey());

  // ---------------------------------------------------------------------
  // Set numOfParts from "this" if specified, otherwise
  // take it from the soft requirement, if specified there,
  // otherwise return a SinglePartitionPartitioningFunction.
  // ---------------------------------------------------------------------
  if (numOfParts < 1 AND softRequirements)
    numOfParts = softRequirements->getCountOfPartitions();

  if (numOfParts <= 1)
    return new(CmpCommon::statementHeap())
      SinglePartitionPartitioningFunction();
  
  // ---------------------------------------------------------------------
  // Set partKey from "this" if specified, otherwise set
  // it from the soft requirement, if specified there,
  // otherwise return a hash partitioning scheme with
  // numOfParts partitions on arbitrary columns of the
  // characteristic outputs of the group (could also do
  // round robin but that would need executor work).
  // ---------------------------------------------------------------------
  
  // try to fulfill both our and the "soft" requirements
  if (NOT partitioningKeyIsSpecified())
  {
    // Take the partitioning key from the soft requirements if the
    // soft requirement partitioning key is not empty.
    if (softRequirements AND
        NOT softRequirements->getPartitioningKey().isEmpty())
    {
      partKey = softRequirements->getPartitioningKey();
    }
    else
    {
      // Nobody says anything about the partitioning key, now we
      // are free to choose anything we want. To get the best
      // possible distribution use a random number.
      //
      ItemExpr *randNum = 
	new(CmpCommon::statementHeap()) RandomNum(NULL, TRUE);
      randNum->synthTypeAndValueId();
      partKey.insert(randNum->getValueId());
    }
  }

  // We have decided how many partitions and what to partition on,
  // now decide whether we want to use hash or range partitioning.
  // Try to do something that is similar to the soft requirements,
  // even if it doesn't really satisfy the soft requirements. We
  // need to make sure that if we can produce a grouping of the soft
  // requirements that we actually do it.

// if (softRequirements AND
//     softRequirements->castToFullySpecifiedPartitioningRequirement() AND
//     partKey == softRequirements->getPartitioningKey())

  if (softRequirements AND
      softRequirements->castToFullySpecifiedPartitioningRequirement()
      AND
       (
        (getSkewProperty().isAnySkew() AND
         partKey.contains(softRequirements->getPartitioningKey()))
         ||
        (!getSkewProperty().isAnySkew() AND
         partKey == softRequirements->getPartitioningKey())
       )
     )
  {
    // try to use the (fully specified) partitioning function of the
    // soft requirements or a scaled version of it
    PartitioningFunction *spf = softRequirements->
      castToFullySpecifiedPartitioningRequirement()->
      getPartitioningFunction();

    Lng32 softPartCount = spf->getCountOfPartitions();

    // This flag is added for "better parallelism project". Otherwise
    // softPartCount will in most cases satisfy the required range and
    // we would not try to scale to numOfParts.
    NABoolean shouldTryScaling = (softPartCount != numOfParts) AND
      (CURRSTMT_OPTDEFAULTS->attemptESPParallelism() == DF_SYSTEM);

    // See if we will be grouping based on the number of active partitions
    // and, if so, get the number of active partitions
    Lng32 numActivePartitions = 1;
    NABoolean baseNumPartsOnAP = FALSE;
    if ((CmpCommon::getDefault(BASE_NUM_PAS_ON_ACTIVE_PARTS) == DF_ON) AND
        (spf->castToRangePartitioningFunction() != NULL))
    {
      baseNumPartsOnAP = TRUE;
      CostScalar activePartitions =
        ((NodeMap *)(spf->getNodeMap()))->getNumActivePartitions();
      numActivePartitions = (Lng32)activePartitions.getValue();
    }

    // Return if the number of partitions from the soft part reqs. 
    // satisfies the requirement and we are not trusting the number of
    // active partitions estimate, or we are but the number of 
    // active partitions is the same as that from the soft requirement.
    // The idea is that if we are using the number of active partitions,
    // then we want to group the number of logical partitions down 
    // so there are not more logical partitions then active physical
    // partitions, even if the number of physical partitions (from
    // the soft requirements) is within range. This is so we do not
    // end up with a lot of logical partitions (ESPs) that do nothing.
    if (isPartitionCountWithinRange(softPartCount) AND 
        (!isRequireHash2Only() || spf->isAHash2PartitioningFunction() ||
         spf->isASinglePartitionPartitioningFunction()) AND
        NOT shouldTryScaling AND
        ((NOT baseNumPartsOnAP) OR (numActivePartitions == softPartCount)))
    {
      // allright, the fully specified soft part reqs. will do the job

      // If skew requirement is present, make sure the spf can deal with it
      // or spf is a singlePartFunc. Otherwise do not use the spf.
      if (NOT getSkewProperty().isAnySkew()) {
         if (spf->canHandleSkew() AND !isRequireHash2Only()) {
            spf = new (STMTHEAP) SkewedDataPartitioningFunction(
                   spf->copy(),
                   getSkewProperty()
                               );
            return spf;
         }

         if (spf->isASinglePartitionPartitioningFunction()) 
            return spf;
      } else {
         spf = spf -> copy();
         return spf;
      }
    }

    // we can still try to scale the number of partitions
    // (make sure not to trash the passed-in object and our part count)
    Lng32 sNumOfParts = numOfParts;
    // If we are grouping based on the number of active partitions, and
    // there won't be enough active partitions to go around, then reduce
    // the number of groups to the number of active partitions. Then check
    // after the scaling to see if the resultant number of groups meets
    // the requirement. The resultant number of partitions should meet the
    // requirement unless the number of active partitions was too small,
    // or the scaling failed because we were grouping based on the
    // number of physical partitions and there were not enough physical
    // partitions. 
    if (baseNumPartsOnAP AND 
        (sNumOfParts > numActivePartitions))
      sNumOfParts = numActivePartitions;

    spf = spf->copy();
    spf = spf->scaleNumberOfPartitions(sNumOfParts);
    if (isPartitionCountWithinRange(sNumOfParts) AND
        (!isRequireHash2Only() || spf->isAHash2PartitioningFunction() ||
         spf->isASinglePartitionPartitioningFunction())) 
    {

      if (NOT getSkewProperty().isAnySkew()) {
         if (spf->canHandleSkew() AND !isRequireHash2Only()) {
            spf = new (STMTHEAP) SkewedDataPartitioningFunction(
                       spf,
                       getSkewProperty());
            return spf;
         }

         if (spf->isASinglePartitionPartitioningFunction()) 
             return spf;
      } else
         return spf;
    }
  }

  // If we come here we did not have luck with the approach to modify
  // the soft requirements.
  // Generate a hash partitioning function with the given key and
  // number of partitions (or a single partition for some cases).
  // Since a singlePartFunc can deal with skew, there is no need to check 
  // skew requirement here.
  if (partKey.isEmpty() OR
       numOfParts == 1 OR
       numOfParts == ANY_NUMBER_OF_PARTITIONS) 
    return new(CmpCommon::statementHeap())
                SinglePartitionPartitioningFunction();

  // At this point, we use the CQD "SOFT_REQ_HASH_TYPE" to
  // determine which hash partitioning scheme to use.  The historical
  // method is to use modulo hash, which is used by the 
  // HashPartitioningFunction.  If Hash2 seems to produce better
  // plans with Hash2 tables, then this section of code may be
  // changed in the future.  For now a CQD is available to make
  // testing of this possible.
  //
  // All these hash partfuncs can deal with skew.
  Lng32 hash_type = CmpCommon::getDefaultLong(SOFT_REQ_HASH_TYPE);
  PartitioningFunction* newPartFunc = NULL;

  if ( NOT (getSkewProperty().isAnySkew()) AND !isRequireHash2Only() ) {

     // Exclusively use hash2 for skew buster. This is necessary because
     // for CHAR typed join column, we compute an surrogate representation
     // for each skew character literal and stored it in the skew list. This
     // reprentation is computed by using the hash2 hashing function. Without
     // the exclusive use of hash2, we need to compute three reprentations.
     // Besides, hash2 is always better than hash0 and hash1.

     newPartFunc = new(CmpCommon::statementHeap())
        Hash2PartitioningFunction(partKey, partKey, numOfParts);

     newPartFunc = new (STMTHEAP)
         SkewedDataPartitioningFunction(newPartFunc, getSkewProperty());
  } else {



     // Check if requireHash2Only_ flag is set AND hash_type is 2 i.e. hash2. 
     // If not, return NULL;
     if (isRequireHash2Only() AND hash_type != 2)
       return NULL;
     switch (hash_type)
     {
       case 0:
         newPartFunc = new(CmpCommon::statementHeap())
           HashPartitioningFunction(partKey, partKey, numOfParts);
         break;
   
       case 1:
         newPartFunc = new(CmpCommon::statementHeap())
           HashDistPartitioningFunction(partKey, partKey, numOfParts);
         break;
   
       case 2:

/*
         if (CmpCommon::getDefault(HIVE_USE_HASH2_AS_PARTFUNCION) == DF_OFF )
            newPartFunc = new(CmpCommon::statementHeap())
              HivePartitioningFunction(partKey, partKey, numOfParts);
         else
*/
            newPartFunc = new(CmpCommon::statementHeap())
              Hash2PartitioningFunction(partKey, partKey, numOfParts);
    
         break;
        
       default:
         CMPASSERT(hash_type >= 0 && hash_type <= 3);
         return NULL; // Required to prevent a compiler warning.
     }
  }

  return newPartFunc;

} // RequireApproximatelyNPartitions::realize

PartitioningRequirement* 
RequireApproximatelyNPartitions::copy() const  
{ 
  return 
    new (CmpCommon::statementHeap()) RequireApproximatelyNPartitions(*this); 
}

void RequireApproximatelyNPartitions::print(FILE* ofd, const char* indent, 
				       const char* title) const
{
  FuzzyPartitioningRequirement::print(ofd, indent, title);
} 

void RequireApproximatelyNPartitions::display() const  { print(); }


// -----------------------------------------------------------------------
// RequireExactlyOnePartition
// -----------------------------------------------------------------------

RequireExactlyOnePartition::RequireExactlyOnePartition 
(NABoolean withNodeMap) : FullySpecifiedPartitioningRequirement()
{
  NodeMap *nodemap = !withNodeMap ? NULL : new(CmpCommon::statementHeap()) 
    NodeMap(CmpCommon::statementHeap(), 1, NodeMapEntry::ACTIVE);
  PartitioningFunction* partFunc = new(CmpCommon::statementHeap())
    SinglePartitionPartitioningFunction(nodemap); 
  setPartitioningFunction(partFunc);
}

// -----------------------------------------------------------------------
// Method for performing a pointer type cast
// -----------------------------------------------------------------------
const RequireExactlyOnePartition*
RequireExactlyOnePartition::castToRequireExactlyOnePartition() const
  { return this; }

PartitioningRequirement* 
RequireExactlyOnePartition::copy() const  
{ 
  return 
    new (CmpCommon::statementHeap()) RequireExactlyOnePartition(*this); 
}

void RequireExactlyOnePartition::print(FILE* ofd, const char* indent, 
                                       const char* title) const
{
  FullySpecifiedPartitioningRequirement::print(ofd, indent, title);
} 

void RequireExactlyOnePartition::display() const  { print(); }
// -----------------------------------------------------------------------
// RequireReplicateViaBroadcast
// -----------------------------------------------------------------------

RequireReplicateViaBroadcast::RequireReplicateViaBroadcast (
                         Lng32 numOfReplicas)
     : FullySpecifiedPartitioningRequirement()
  {
    PartitioningFunction* partFunc = 
        new(CmpCommon::statementHeap())
             ReplicateViaBroadcastPartitioningFunction(numOfReplicas); 

    setPartitioningFunction(partFunc);
  }

// RequireReplicateViaBroadcast constructor used by HashJoin to
// have right child use left child's partitioning function's nodemap
RequireReplicateViaBroadcast::RequireReplicateViaBroadcast 
(PartitioningFunction *childPF, NABoolean useChildsNodeMap) 
  : FullySpecifiedPartitioningRequirement()
{
  PartitioningFunction* partFunc = new(CmpCommon::statementHeap())
    ReplicateViaBroadcastPartitioningFunction
    (childPF->getCountOfPartitions(), childPF->getNodeMap()->copy());

  setPartitioningFunction(partFunc);
}

// -----------------------------------------------------------------------
// Comparison method for comparing a replicate via broadcast
// requirement against another partitioning requirement.
// -----------------------------------------------------------------------
COMPARE_RESULT
RequireReplicateViaBroadcast::comparePartReqToReq
                          (const PartitioningRequirement* other) const
{
  if ( NOT other->isRequirementFullySpecified() ) 
    return INCOMPATIBLE;

  PartitioningFunction* myPartFunc = getPartitioningFunction();
  PartitioningFunction* otherPartFunc = 
    other->castToFullySpecifiedPartitioningRequirement()
         ->getPartitioningFunction();
  
  return myPartFunc->comparePartFuncToFunc(*otherPartFunc);

} //RequireReplicateViaBroadcast::comparePartReqToReq

// -----------------------------------------------------------------------
// Method for performing a pointer type cast
// -----------------------------------------------------------------------
const RequireReplicateViaBroadcast*
RequireReplicateViaBroadcast::castToRequireReplicateViaBroadcast() const
  { return this; }

PartitioningRequirement* 
RequireReplicateViaBroadcast::copy() const  
{ 
  return 
    new (CmpCommon::statementHeap()) RequireReplicateViaBroadcast(*this); 
}

void RequireReplicateViaBroadcast::print(FILE* ofd, const char* indent, 
                                         const char* title) const
{
  FullySpecifiedPartitioningRequirement::print(ofd, indent, title);
} 

void RequireReplicateViaBroadcast::display() const  { print(); }

// -----------------------------------------------------------------------
// RequireReplicateNoBroadcast
// -----------------------------------------------------------------------

// RequireReplicateNoBroadcast constructor used by NestedJoin to
// have right child use parent's partitioning function's nodemap
RequireReplicateNoBroadcast::RequireReplicateNoBroadcast 
(PartitioningFunction *parentPF, NABoolean useParentsNodeMap)
  : FullySpecifiedPartitioningRequirement(),
    parentPartFunc_(parentPF)
{
  PartitioningFunction* partFunc;

  if(useParentsNodeMap) {
    partFunc = new(CmpCommon::statementHeap())
      ReplicateNoBroadcastPartitioningFunction(parentPF->getCountOfPartitions(),
                                               parentPF->getNodeMap()->copy()); 
  } else {
    partFunc = new(CmpCommon::statementHeap())
      ReplicateNoBroadcastPartitioningFunction(parentPF->getCountOfPartitions()); 
  }
  setPartitioningFunction(partFunc);
}

// -----------------------------------------------------------------------
// Comparison method for comparing a replicate with no broadcast
// requirement against another partitioning requirement.
// -----------------------------------------------------------------------
COMPARE_RESULT
RequireReplicateNoBroadcast::comparePartReqToReq
                          (const PartitioningRequirement* other) const
{
  if ( NOT other->isRequirementFullySpecified() ) 
    return INCOMPATIBLE;

  PartitioningFunction* myPartFunc = getPartitioningFunction();
  PartitioningFunction* otherPartFunc = 
    other->castToFullySpecifiedPartitioningRequirement()
         ->getPartitioningFunction();
  
  return myPartFunc->comparePartFuncToFunc(*otherPartFunc);

} //RequireReplicateNoBroadcast::comparePartReqToReq

// -----------------------------------------------------------------------
// Method for performing a pointer type cast
// -----------------------------------------------------------------------
const RequireReplicateNoBroadcast*
RequireReplicateNoBroadcast::castToRequireReplicateNoBroadcast() const
  { return this; }

PartitioningRequirement* 
RequireReplicateNoBroadcast::copy() const  
{ 
  return 
    new (CmpCommon::statementHeap()) RequireReplicateNoBroadcast(*this); 
}

void RequireReplicateNoBroadcast::print(FILE* ofd, const char* indent, 
                                         const char* title) const
{
  FullySpecifiedPartitioningRequirement::print(ofd, indent, title);
} 

void RequireReplicateNoBroadcast::display() const  { print(); }

// -----------------------------------------------------------------------
// RequireHash
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// Method for performing a pointer type cast
// -----------------------------------------------------------------------
const RequireHash*
RequireHash::castToRequireHash() const
  { return this; }

PartitioningRequirement* 
RequireHash::copy() const  
{ 
  return 
    new (CmpCommon::statementHeap()) RequireHash(*this); 
}

void RequireHash::print(FILE* ofd, const char* indent, 
                        const char* title) const
{
  FullySpecifiedPartitioningRequirement::print(ofd, indent, title);
} 

void RequireHash::display() const  { print(); }

// -----------------------------------------------------------------------
// RequireHashDist
// -----------------------------------------------------------------------

NABoolean
RequireHashDist::partReqAndFuncCompatible
                (const PartitioningFunction* other) const
{
  const HashDistPartitioningFunction *partFunc = 
    getPartitioningFunction()->castToHashDistPartitioningFunction();

  CMPASSERT(partFunc);

  if(partFunc->comparePartFuncToFunc(*other) == SAME)
    return TRUE;

  return FALSE;

} // RequireHashDist::partReqAndFuncCompatible()

// -----------------------------------------------------------------------
// Method for performing a pointer type cast
// -----------------------------------------------------------------------
const RequireHashDist*
RequireHashDist::castToRequireHashDist() const
  { return this; }

PartitioningRequirement* 
RequireHashDist::copy() const  
{ 
  return 
    new (CmpCommon::statementHeap()) RequireHashDist(*this); 
}

void RequireHashDist::print(FILE* ofd, const char* indent, 
                        const char* title) const
{
  FullySpecifiedPartitioningRequirement::print(ofd, indent, title);
} 

void RequireHashDist::display() const  { print(); }

// -----------------------------------------------------------------------
// RequireHash2
// -----------------------------------------------------------------------

NABoolean
RequireHash2::partReqAndFuncCompatible(const PartitioningFunction* other) const
{
  const Hash2PartitioningFunction *partFunc =
    getPartitioningFunction()->castToHash2PartitioningFunction();

  CMPASSERT(partFunc);

  // If my requirement does not require a log phys part func,
  // then if the part func is a log phys dig underneath to
  // get the physical part func. We want to compare against
  // the physical part func because if the synthesized part func
  // is a log phys it means we are in dp2, and in dp2 all part
  // requirements must be physical requirements.

  if (other->isALogPhysPartitioningFunction())
  {
    other = other->castToLogPhysPartitioningFunction()
      ->getPhysPartitioningFunction();
  }

  return (partFunc->comparePartFuncToFunc(*other) == SAME);

} // RequireHash2::partReqAndFuncCompatible()

// -----------------------------------------------------------------------
// Method for performing a pointer type cast
// -----------------------------------------------------------------------
const RequireHash2*
RequireHash2::castToRequireHash2() const
  { return this; }

PartitioningRequirement*
RequireHash2::copy() const
{
  return
    new (CmpCommon::statementHeap()) RequireHash2(*this);
}

void RequireHash2::print(FILE* ofd, const char* indent,
                        const char* title) const
{
  FullySpecifiedPartitioningRequirement::print(ofd, indent, title);
}

void RequireHash2::display() const  { print(); }

//===========
// -----------------------------------------------------------------------
// RequireSkewed
// -----------------------------------------------------------------------
const ValueIdSet& RequireSkewed::getPartialPartitioningKey() const
{  
   return getPartitioningFunction() ->
             castToSkewedDataPartitioningFunction()->
                getPartialPartitioningFunction()->getPartitioningKey(); 
}

COMPARE_RESULT
RequireSkewed::comparePartReqToReq(const PartitioningRequirement* other) const
{
   COMPARE_RESULT result = 
      FullySpecifiedPartitioningRequirement::comparePartReqToReq(other);

   if ( other -> isRequirementSkewed() ) {
     if ( NOT (getSkewProperty() ==
             other->castToRequireSkewed()->getSkewProperty()) )
      result = combine_compare_results(result,INCOMPATIBLE);
   } else {
     if ( NOT (other -> isRequirementFuzzy()) OR 
         (NOT (getSkewProperty() ==
               other->castToFuzzyPartitioningRequirement()->getSkewProperty()))
        )
      result = combine_compare_results(result,INCOMPATIBLE);
   }

   return result;
}

const skewProperty& RequireSkewed::getSkewProperty() const
{
  return getPartitioningFunction()-> 
             castToSkewedDataPartitioningFunction()->
                getSkewProperty();
}

NABoolean
RequireSkewed::partReqAndFuncCompatible(const PartitioningFunction* other) const
{
  const PartitioningFunction *thisPartFunc = getPartitioningFunction();
  CMPASSERT(thisPartFunc->castToSkewedDataPartitioningFunction());
  return (thisPartFunc->comparePartFuncToFunc(*other) == SAME );
} // RequireSkewed::partReqAndFuncCompatible()

// -----------------------------------------------------------------------
// Method for performing a pointer type cast
// -----------------------------------------------------------------------
const RequireSkewed*
RequireSkewed::castToRequireSkewed() const
  { return this; }

PartitioningRequirement*
RequireSkewed::copy() const
{
  return new (CmpCommon::statementHeap()) RequireSkewed(*this);
}

void RequireSkewed::print(FILE* ofd, const char* indent,
                        const char* title) const
{
  FullySpecifiedPartitioningRequirement::print(ofd, indent, title);
}

void RequireSkewed::display() const  { print(); }


// -----------------------------------------------------------------------
// RequireRange
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// Method for performing a pointer type cast
// -----------------------------------------------------------------------
const RequireRange*
RequireRange::castToRequireRange() const
  { return this; }

PartitioningRequirement* 
RequireRange::copy() const  
{ 
  return 
    new (CmpCommon::statementHeap()) RequireRange(*this); 
}

void RequireRange::print(FILE* ofd, const char* indent, 
                         const char* title) const
{
  FullySpecifiedPartitioningRequirement::print(ofd, indent, title);
} 

void RequireRange::display() const  { print(); }

// -----------------------------------------------------------------------
// RequireRoundRobin
// -----------------------------------------------------------------------

NABoolean
RequireRoundRobin::
partReqAndFuncCompatible(const PartitioningFunction* other) const
{
  const RoundRobinPartitioningFunction *partFunc = 
    getPartitioningFunction()->castToRoundRobinPartitioningFunction();

  CMPASSERT(partFunc);

  if(partFunc->comparePartFuncToFunc(*other) == SAME)
    return TRUE;

  return FALSE;

} // RequireHashDist::partReqAndFuncCompatible()

// -----------------------------------------------------------------------
// Method for performing a pointer type cast
// -----------------------------------------------------------------------
const RequireRoundRobin*
RequireRoundRobin::castToRequireRoundRobin() const
  { return this; }

PartitioningRequirement* 
RequireRoundRobin::copy() const  
{ 
  return 
    new (CmpCommon::statementHeap()) RequireRoundRobin(*this); 
}

void RequireRoundRobin::print(FILE* ofd, const char* indent, 
                              const char* title) const
{
  FullySpecifiedPartitioningRequirement::print(ofd, indent, title);
} 

void RequireRoundRobin::display() const  { print(); }


// -----------------------------------------------------------------------
// Methods for class LogicalPartitioningRequirement
// -----------------------------------------------------------------------

COMPARE_RESULT LogicalPartitioningRequirement::compareLogPartRequirements(
     const LogicalPartitioningRequirement &other) const
{
  if (logPartType_ != other.logPartType_)
    return INCOMPATIBLE;

  if (numClients_ != other.numClients_)
    return INCOMPATIBLE;

  if (logPartReq_ AND other.logPartReq_)
    return logPartReq_->comparePartReqToReq(other.logPartReq_);
  else if (logPartReq_)
    return MORE;
  else if (other.logPartReq_)
    return LESS;
  else
    return SAME;
}

NABoolean LogicalPartitioningRequirement::satisfied(
     const RelExpr * const physExpr,
     const PhysicalProperty * const physProp) const
{
  const PartitioningFunction *actualPartFunc =
    physProp->getPartitioningFunction();
  const LogPhysPartitioningFunction *lpf =
    actualPartFunc->castToLogPhysPartitioningFunction();

  if (lpf == NULL)
  {
    // if the RelExpr does not have a LogPhysPartitioningFunction
    // then make sure that we required only the simple properties
    // that another partitioning function can provide
    return ((logPartType_ == LogPhysPartitioningFunction::PA_PARTITION_GROUPING OR
             logPartType_ == LogPhysPartitioningFunction::ANY_LOGICAL_PARTITIONING)
            AND
            (numClients_ == ANY_NUMBER_OF_PARTITIONS OR numClients_ == 1)
            AND
            (logPartReq_ == NULL OR
             logPartReq_->partReqAndFuncCompatible(actualPartFunc)
            )
           );
  }

  // if we required a specific type of logical partitioning, then check
  // for it here
  if (logPartType_ !=
      LogPhysPartitioningFunction::ANY_LOGICAL_PARTITIONING AND
      lpf->getLogPartType() != logPartType_)
    return FALSE;

  // don't check # of clients, it is just a suggestion

  // check forced use of PAPA
  if (mustUsePapa_ AND
      NOT lpf->getUsePapa())
    return FALSE;

  // check the logical partitioning requirement
  if (logPartReq_ AND
      NOT logPartReq_->partReqAndFuncCompatible(
	   lpf->getLogPartitioningFunction()) 
     )
    return FALSE;

  return TRUE;
}

NABoolean
FullySpecifiedPartitioningRequirement::isRequirementSkewBusterBroadcast() const
{
  const SkewedDataPartitioningFunction *skpf =
    getPartitioningFunction()->castToSkewedDataPartitioningFunction();

   return (skpf AND skpf->getSkewProperty().isBroadcasted());
}

