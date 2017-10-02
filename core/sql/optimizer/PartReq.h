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
#ifndef PARTREQ_H
#define PARTREQ_H
/* -*-C++-*-
*************************************************************************
*
* File:         PartReq.h
* Description:  Partitioning Requirements
* Created:      02/27/98
* Language:     C++
*
*
*
*
*************************************************************************
*/

#include "PartFunc.h"
#include "CmpStatement.h"

// ----------------------------------------------------------------------
// contents of this file
// ----------------------------------------------------------------------
class PartitioningRequirement;

class FuzzyPartitioningRequirement;
class FullySpecifiedPartitioningRequirement;
class RequireApproximatelyNPartitions;
class RequireExactlyOnePartition;  
class RequireReplicateViaBroadcast;
class RequireReplicateNoBroadcast;
class RequireHash;
class RequireHash2;
class RequireHive;
class RequireHashDist;
class RequireSkewed;
class RequireRange;
class RequireRoundRobin;

class LogicalPartitioningRequirement;

// ----------------------------------------------------------------------
// forward references
// ----------------------------------------------------------------------

// -----------------------------------------------------------------------
// PARTITIONING REQUIREMENTS.
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// A partitioning requirement can be an incomplete specification of a
// Partitioning Function. Essentially, it behaves like a wild card for a
// partititioning scheme. A partitioning requirement is specified in the
// Required Physical Properties that are used by Cascades. It allows the
// optimizer to state the desired criterion for partitioning namely,
// either the number of partitions, or the partitioning keys, or both.
// -----------------------------------------------------------------------
class PartitioningRequirement : public NABasicObject
{
protected:
  // --------------------------------------------------------------------
  // Partitioning requirement category identifier.
  // --------------------------------------------------------------------
  enum PartitioningRequirementCategoryEnum
    {
      FULLY_SPECIFIED_PART_REQ,
      FUZZY_PART_REQ
    };

public:

  // --------------------------------------------------------------------
  // Destructor functions
  // --------------------------------------------------------------------
  virtual ~PartitioningRequirement();

  // ---------------------------------------------------------------------
  // Perform type-safe pointer casts.
  // ---------------------------------------------------------------------
  virtual const
  FuzzyPartitioningRequirement*
                   castToFuzzyPartitioningRequirement() const;

  virtual const
  FullySpecifiedPartitioningRequirement*
                   castToFullySpecifiedPartitioningRequirement() const;

  virtual const
  RequireApproximatelyNPartitions*
                   castToRequireApproximatelyNPartitions() const;

  virtual const
  RequireExactlyOnePartition*
                   castToRequireExactlyOnePartition() const;

  virtual const
  RequireReplicateViaBroadcast*
                   castToRequireReplicateViaBroadcast() const;

  virtual const
  RequireReplicateNoBroadcast*
                   castToRequireReplicateNoBroadcast() const;

  virtual const
  RequireHash*
                   castToRequireHash() const;

  virtual const
  RequireHashDist*
                   castToRequireHashDist() const;

  virtual const
  RequireHash2*
                   castToRequireHash2() const;

  virtual const
  RequireSkewed*
                   castToRequireSkewed() const;


  virtual const
  RequireRange*
                   castToRequireRange() const;

  virtual const
  RequireRoundRobin*
                   castToRequireRoundRobin() const;

  virtual const
  RequireHive*
                   castToRequireHive() const;

  // ---------------------------------------------------------------------
  // The number of partitions that will be formed using this scheme.
  // ---------------------------------------------------------------------
  virtual Lng32 getCountOfPartitions() const;

  // Accessor method for the partitioning key
  virtual const ValueIdSet& getPartitioningKey() const;
  virtual const ValueIdSet& getPartialPartitioningKey() const 
  { return getPartitioningKey(); };

  virtual NABoolean partitioningKeyIsSpecified() const;

  // ---------------------------------------------------------------------
  // Partitioning Requirement Category Tests
  // ---------------------------------------------------------------------
  NABoolean isRequirementFullySpecified() const
    { return (requirementCategory_ == FULLY_SPECIFIED_PART_REQ); }

  NABoolean isRequirementFuzzy() const
    { return (requirementCategory_ == FUZZY_PART_REQ); }

  virtual NABoolean partReqAndFuncCompatible
                           (const PartitioningFunction* other) const;

  virtual COMPARE_RESULT comparePartReqToReq
                           (const PartitioningRequirement* other) const;

  virtual NABoolean isRequirementApproximatelyN() const
    { return FALSE; }

  virtual NABoolean isRequirementExactlyOne() const
    { return FALSE; }

  virtual NABoolean isRequirementReplicateViaBroadcast() const
    { return FALSE; }

  virtual NABoolean isRequirementReplicateNoBroadcast() const
    { return FALSE; }

  virtual NABoolean isRequirementHash() const
    { return FALSE; }

  virtual NABoolean isRequirementSkewed() const
    { return FALSE; }

  virtual NABoolean isRequirementRange() const
    { return FALSE; }

  virtual NABoolean isRequirementRoundRobin() const
    { return FALSE; }

  virtual NABoolean isRequirementSkewBusterBroadcast() const
    { return FALSE; };


  virtual NABoolean isRequirementHive() const
    { return FALSE; };

  virtual NABoolean isReplicate() const { return FALSE; }

  // --------------------------------------------------------------------
  // Method used by the optimizer for replacing a partitioning
  // requirement with an actual partitioning function.
  // --------------------------------------------------------------------
  virtual PartitioningFunction * realize(
       const Context *myContext,
       NABoolean useContextPartitioningRequirements = FALSE,
       const PartitioningRequirement * softRequirements = NULL) ;

  // --------------------------------------------------------------------
  // Copy this partitioning requirement and rewrite the copy in terms of
  // the top or bottom values that are contained in the map.
  // If the parameter mapItUp is set to TRUE, then the partitioning key
  // or function is rewritten in terms of the values in the top map.
  // Otherwise, it is rewritten in terms of values in the bottom map.
  // The virtual function remapIt() implements the remapping.
  // --------------------------------------------------------------------
  PartitioningRequirement* copyAndRemap
                           (ValueIdMap& map, NABoolean mapItUp) const;

  virtual void remapIt(const PartitioningRequirement* opr,
                       ValueIdMap& map, NABoolean mapItUp);

  // --------------------------------------------------------------------
  // A method for copying the partitioning function.
  // --------------------------------------------------------------------
  virtual PartitioningRequirement* copy() const;

  // ---------------------------------------------------------------------
  // Print.
  // ---------------------------------------------------------------------
  virtual const NAString getText() const;
  virtual void print( FILE* ofd = stdout,
                      const char* indent = DEFAULT_INDENT,
                      const char* title = "PartitioningRequirement") const;

  virtual void display() const;

protected:

  // ---------------------------------------------------------------------
  // The following constructors are used by the derived classes and
  // are therefore hidden from public view.
  // ---------------------------------------------------------------------
  PartitioningRequirement(const PartitioningRequirementCategoryEnum rCat)
     : requirementCategory_ (rCat)
  {
  }

  PartitioningRequirement(const PartitioningRequirement& other)        
     : requirementCategory_ (other.requirementCategory_)
  {
  }

private :

  PartitioningRequirementCategoryEnum requirementCategory_;
}; // class PartitioningRequirement

// -----------------------------------------------------------------------
// Classes that are derived directly from PartitioningRequirement.
// -----------------------------------------------------------------------

class FuzzyPartitioningRequirement : public PartitioningRequirement
{
protected:
  // --------------------------------------------------------------------
  // Partitioning requirement type identifier.
  // --------------------------------------------------------------------
  enum FuzzyPartReqTypeEnum
    {
      APPROXIMATELY_N_PART_REQ
    };

public:

  // ---------------------------------------------------------------------
  // Perform a type-safe pointer cast.
  // ---------------------------------------------------------------------
  virtual const
  FuzzyPartitioningRequirement*
                   castToFuzzyPartitioningRequirement() const;

  // ---------------------------------------------------------------------
  // The number of partitions that will be formed using this scheme.
  // ---------------------------------------------------------------------
  virtual Lng32 getCountOfPartitions() const
    {  return numberOfPartitions_; }

  // Accessor method for the partitioning key
  virtual const ValueIdSet& getPartitioningKey() const
    {  return partitioningKeyColumns_; }

  virtual NABoolean partitioningKeyIsSpecified() const
                                   { return partitioningKeyIsSpecified_; }

  virtual NABoolean partReqAndFuncCompatible
                           (const PartitioningFunction* other) const;

  virtual COMPARE_RESULT comparePartReqToReq
                           (const PartitioningRequirement* other) const;

  FuzzyPartReqTypeEnum getPartitioningRequirementType() const
    { return requirementType_;}

  virtual NABoolean isRequirementApproximatelyN() const
    { return (requirementType_ == APPROXIMATELY_N_PART_REQ); }

  virtual void remapIt(const PartitioningRequirement* opr,
                       ValueIdMap& map, NABoolean mapItUp);

  // --------------------------------------------------------------------
  // A method for copying the partitioning function.
  // --------------------------------------------------------------------
  virtual PartitioningRequirement* copy() const;

  // ---------------------------------------------------------------------
  // Print.
  // ---------------------------------------------------------------------
  virtual void print(FILE* ofd = stdout,
                     const char* indent = DEFAULT_INDENT,
                     const char* title = "FuzzyPartitioningRequirement") const;

  virtual void display() const;

  // helper function on skew property
  virtual const skewProperty& getSkewProperty() const { return skewProperty_; };
  virtual void setSkewProperty(const skewProperty& sk) { skewProperty_ = sk; };

protected:

  // ---------------------------------------------------------------------
  // The following constructors are used by the derived classes and
  // are therefore hidden from public view.
  // ---------------------------------------------------------------------
  FuzzyPartitioningRequirement(const FuzzyPartReqTypeEnum rType,
                               const ValueIdSet& partitioningKeyColumns,
                               Lng32 numberOfPartitions = 
                                      ANY_NUMBER_OF_PARTITIONS
                               ,const skewProperty& sk = ANY_SKEW_PROPERTY
                               )
     : PartitioningRequirement(FUZZY_PART_REQ),
       requirementType_ (rType),
       partitioningKeyColumns_(partitioningKeyColumns),
       partitioningKeyIsSpecified_(TRUE),
       numberOfPartitions_ (numberOfPartitions)
     ,skewProperty_(sk)
  {
  }

  FuzzyPartitioningRequirement(const FuzzyPartReqTypeEnum rType,
                               Lng32 numberOfPartitions = 
                                      ANY_NUMBER_OF_PARTITIONS
                               ,const skewProperty& sk = ANY_SKEW_PROPERTY
                               )
     : PartitioningRequirement(FUZZY_PART_REQ),
       requirementType_ (rType),
       partitioningKeyIsSpecified_(FALSE),
       numberOfPartitions_ (numberOfPartitions)
     ,skewProperty_(sk)
  {
  }

  FuzzyPartitioningRequirement(const FuzzyPartitioningRequirement& other)
     : PartitioningRequirement(other),
       requirementType_ (other.requirementType_),
       partitioningKeyColumns_(other.partitioningKeyColumns_),
       partitioningKeyIsSpecified_(other.partitioningKeyIsSpecified_),
       numberOfPartitions_ (other.numberOfPartitions_)
     ,skewProperty_(other.skewProperty_)
  {
  }

private :

  FuzzyPartReqTypeEnum requirementType_;
  ValueIdSet partitioningKeyColumns_;
  NABoolean partitioningKeyIsSpecified_;
  Lng32 numberOfPartitions_;
  skewProperty skewProperty_;
}; // class FuzzyPartitioningRequirement


class FullySpecifiedPartitioningRequirement : public PartitioningRequirement
{
public:

  // ---------------------------------------------------------------------
  // The number of partitions that will be formed using this scheme.
  // ---------------------------------------------------------------------
  virtual Lng32 getCountOfPartitions() const
    {  return partitioningFunction_->getCountOfPartitions(); }

  // Accessor method for the partitioning key
  virtual const ValueIdSet& getPartitioningKey() const
    {  return partitioningFunction_->getPartitioningKey(); }

  virtual NABoolean partitioningKeyIsSpecified() const { return TRUE; }

  virtual NABoolean partReqAndFuncCompatible
                           (const PartitioningFunction* other) const;

  virtual COMPARE_RESULT comparePartReqToReq
                           (const PartitioningRequirement* other) const;

  // ---------------------------------------------------------------------
  // Perform a type-safe pointer cast.
  // ---------------------------------------------------------------------
  virtual const
  FullySpecifiedPartitioningRequirement*
                   castToFullySpecifiedPartitioningRequirement() const;

  PartitioningFunction* getPartitioningFunction() const
    { return partitioningFunction_; }

  void setPartitioningFunction (PartitioningFunction * partFunc)
    { partitioningFunction_ = partFunc; }

  virtual NABoolean isRequirementExactlyOne() const
    { return
       getPartitioningFunction()->isASinglePartitionPartitioningFunction();
    }

  virtual NABoolean isRequirementReplicateViaBroadcast() const
    { return
       getPartitioningFunction()->
         isAReplicateViaBroadcastPartitioningFunction();
    }

  virtual NABoolean isRequirementReplicateNoBroadcast() const
    { return
       getPartitioningFunction()->
         isAReplicateNoBroadcastPartitioningFunction();
    }

  virtual NABoolean isRequirementHash() const
    { return
       getPartitioningFunction()->isAHashPartitioningFunction();
    }

  virtual NABoolean isRequirementRange() const
    { return
       getPartitioningFunction()->isARangePartitioningFunction();
    }

  virtual NABoolean isRequirementRoundRobin() const
    { return
       getPartitioningFunction()->isARoundRobinPartitioningFunction();
    }

  virtual NABoolean isRequirementSkewBusterBroadcast() const;

  // --------------------------------------------------------------------
  // Method used by the optimizer for replacing a partitioning
  // requirement with an actual partitioning function.
  // --------------------------------------------------------------------
  virtual PartitioningFunction * realize(
       const Context *myContext,
       NABoolean useContextPartitioningRequirements = FALSE,
       const PartitioningRequirement * softRequirements = NULL) ;

  virtual void remapIt(const PartitioningRequirement* opr,
                       ValueIdMap& map, NABoolean mapItUp);

  // --------------------------------------------------------------------
  // A method for copying the partitioning function.
  // --------------------------------------------------------------------
  virtual PartitioningRequirement* copy() const;

  // ---------------------------------------------------------------------
  // Print.
  // ---------------------------------------------------------------------
  virtual void print( FILE* ofd = stdout,
                      const char* indent = DEFAULT_INDENT,
            const char* title = "FullySpecifiedPartitioningRequirement") const;

  virtual void display() const;

protected:

  // ---------------------------------------------------------------------
  // The following constructors are used by the derived classes and
  // are therefore hidden from public view.
  // ---------------------------------------------------------------------
  FullySpecifiedPartitioningRequirement(PartitioningFunction* partFunc)
     : PartitioningRequirement(FULLY_SPECIFIED_PART_REQ),
       partitioningFunction_(partFunc)
  {
  }

  FullySpecifiedPartitioningRequirement()
     : PartitioningRequirement(FULLY_SPECIFIED_PART_REQ),
       partitioningFunction_(NULL)
  {
  }

  FullySpecifiedPartitioningRequirement
          (const FullySpecifiedPartitioningRequirement& other)
     : PartitioningRequirement(other),
       partitioningFunction_(other.partitioningFunction_)
  {
  }

private :

  PartitioningFunction* partitioningFunction_;

}; // class FullySpecifiedPartitioningRequirement


// -----------------------------------------------------------------------
// Classes that are derived from FuzzyPartitioningRequirement.
// -----------------------------------------------------------------------

class RequireApproximatelyNPartitions : public FuzzyPartitioningRequirement
{
public:

  RequireApproximatelyNPartitions (const ValueIdSet& partitioningKeyColumns,
                                   float numOfPartsAllowedDeviation =
                                     CURRSTMT_OPTDEFAULTS->numberOfPartitionsDeviation(),
                                   Lng32 numberOfPartitions = 
                                          ANY_NUMBER_OF_PARTITIONS,
                                   NABoolean requireHash2Only = FALSE
                                   ,const skewProperty& sk = ANY_SKEW_PROPERTY
                                   );

  RequireApproximatelyNPartitions (float numOfPartsAllowedDeviation = 
                                     CURRSTMT_OPTDEFAULTS->numberOfPartitionsDeviation(),
                                   Lng32 numberOfPartitions = 
                                          ANY_NUMBER_OF_PARTITIONS,
                                   NABoolean requireHash2Only = FALSE
                                   ,const skewProperty& sk = ANY_SKEW_PROPERTY
                                   );

  RequireApproximatelyNPartitions
      (const RequireApproximatelyNPartitions& other)
     : FuzzyPartitioningRequirement(other),
       numOfPartsAllowedDeviation_ (other.numOfPartsAllowedDeviation_),
       requireHash2Only_(other.requireHash2Only_)
  {
  }

  virtual NABoolean partReqAndFuncCompatible
                           (const PartitioningFunction* other) const;

  virtual COMPARE_RESULT comparePartReqToReq
                           (const PartitioningRequirement* other) const;

  // ---------------------------------------------------------------------
  // Perform a type-safe pointer cast.
  // ---------------------------------------------------------------------
  virtual const
  RequireApproximatelyNPartitions*
                   castToRequireApproximatelyNPartitions() const;

  // accesor method for the allowable deviation
  float getAllowedDeviation() const
    { return numOfPartsAllowedDeviation_; }

  // check whether an actual number of partitions is within the
  // range for the allowed number of partitions
  NABoolean isPartitionCountWithinRange(Lng32 numOfParts) const;
  
  // accesor method for the lower bound of the range for the 
  // allowed number of partitions
  Lng32 getCountOfPartitionsLowBound() const;

  NABoolean isRequireHash2Only() const { return requireHash2Only_; }
  
  // --------------------------------------------------------------------
  // Method used by the optimizer for replacing a partitioning
  // requirement with an actual partitioning function.
  // --------------------------------------------------------------------
  virtual PartitioningFunction * realize(
       const Context *myContext,
       NABoolean useContextPartitioningRequirements = FALSE,
       const PartitioningRequirement * softRequirements = NULL) ;

  // --------------------------------------------------------------------
  // A method for copying the partitioning function.
  // --------------------------------------------------------------------
  virtual PartitioningRequirement* copy() const;

  // ---------------------------------------------------------------------
  // Print.
  // ---------------------------------------------------------------------
  virtual void print( FILE* ofd = stdout,
                      const char* indent = DEFAULT_INDENT,
                  const char* title = "RequireApproximatelyNPartitions") const;

  virtual void display() const;

private :

  float numOfPartsAllowedDeviation_;
  NABoolean requireHash2Only_;

}; // class RequireApproximatelyNPartitions


// -----------------------------------------------------------------------
// Classes that are derived from FullySpecifiedPartitioningRequirement.
// -----------------------------------------------------------------------

class RequireExactlyOnePartition : public FullySpecifiedPartitioningRequirement
{
public:

  RequireExactlyOnePartition (PartitioningFunction* partFunc)
     : FullySpecifiedPartitioningRequirement(partFunc)
  {
    CMPASSERT(partFunc->isASinglePartitionPartitioningFunction());
  }

  RequireExactlyOnePartition (NABoolean withNodeMap=FALSE);

  RequireExactlyOnePartition (const RequireExactlyOnePartition& other)
    :  FullySpecifiedPartitioningRequirement (other)
  {
  }
     
  // ---------------------------------------------------------------------
  // Perform a type-safe pointer cast.
  // ---------------------------------------------------------------------
  virtual const
  RequireExactlyOnePartition*
                   castToRequireExactlyOnePartition() const;

  // --------------------------------------------------------------------
  // A method for copying the partitioning function.
  // --------------------------------------------------------------------
  virtual PartitioningRequirement* copy() const;

  // ---------------------------------------------------------------------
  // Print.
  // ---------------------------------------------------------------------
  virtual void print( FILE* ofd = stdout,
                      const char* indent = DEFAULT_INDENT,
                      const char* title = "RequireExactlyOnePartition") const;

  virtual void display() const;

}; // class RequireExactlyOnePartition


class RequireReplicateViaBroadcast :
        public FullySpecifiedPartitioningRequirement
{
public:

  RequireReplicateViaBroadcast (PartitioningFunction* partFunc)
     : FullySpecifiedPartitioningRequirement(partFunc)
  {
    CMPASSERT(partFunc->isAReplicateViaBroadcastPartitioningFunction());
  }

  RequireReplicateViaBroadcast (Lng32 numOfReplicas);

  RequireReplicateViaBroadcast (PartitioningFunction *childPF,
                                NABoolean useChildsNodeMap);

  RequireReplicateViaBroadcast (const RequireReplicateViaBroadcast& other)
    :  FullySpecifiedPartitioningRequirement (other)
  {
  }
     
  virtual COMPARE_RESULT comparePartReqToReq
                           (const PartitioningRequirement* other) const;

  NABoolean isReplicate() const { return TRUE; }

  // ---------------------------------------------------------------------
  // Perform a type-safe pointer cast.
  // ---------------------------------------------------------------------
  virtual const
  RequireReplicateViaBroadcast*
                   castToRequireReplicateViaBroadcast() const;


  // --------------------------------------------------------------------
  // A method for copying the partitioning function.
  // --------------------------------------------------------------------
  virtual PartitioningRequirement* copy() const;

  // ---------------------------------------------------------------------
  // Print.
  // ---------------------------------------------------------------------
  virtual void print( FILE* ofd = stdout,
                      const char* indent = DEFAULT_INDENT,
                      const char* title =
                                    "RequireReplicateViaBroadcast") const;

  virtual void display() const;

private :


}; // class RequireReplicateViaBroadcast


class RequireReplicateNoBroadcast :
        public FullySpecifiedPartitioningRequirement
{
public:

  RequireReplicateNoBroadcast (PartitioningFunction* partFunc)
    : FullySpecifiedPartitioningRequirement(partFunc),
      parentPartFunc_(NULL)
  {
    CMPASSERT(partFunc->isAReplicateNoBroadcastPartitioningFunction());
  }

  RequireReplicateNoBroadcast (PartitioningFunction *parentPF,
                               NABoolean useParentsNodeMap);

  RequireReplicateNoBroadcast (const RequireReplicateNoBroadcast& other)
    :  FullySpecifiedPartitioningRequirement (other),
       parentPartFunc_(other.getParentPartFunc())
  {
  }
     
  virtual COMPARE_RESULT comparePartReqToReq
                           (const PartitioningRequirement* other) const;

  NABoolean isReplicate() const { return TRUE; }

  // ---------------------------------------------------------------------
  // Perform a type-safe pointer cast.
  // ---------------------------------------------------------------------
  virtual const
  RequireReplicateNoBroadcast*
                   castToRequireReplicateNoBroadcast() const;

  // --------------------------------------------------------------------
  // A method for copying the partitioning function.
  // --------------------------------------------------------------------
  virtual PartitioningRequirement* copy() const;

  // ---------------------------------------------------------------------
  // Print.
  // ---------------------------------------------------------------------
  virtual void print( FILE* ofd = stdout,
                      const char* indent = DEFAULT_INDENT,
                      const char* title =
                                    "RequireReplicateNoBroadcast") const;

  virtual void display() const;

  const PartitioningFunction *getParentPartFunc() const {return parentPartFunc_;};

private :

  // The partitioning function of the operator which is placing this
  // Rep-N requirement on its child.  This is used to determine if
  // each instance of the operator will access all the children or
  // just a group of the children.  Ultimately used to determine the
  // total number of PAs used by the PAPA.
  const PartitioningFunction *parentPartFunc_;

}; // class RequireReplicateNoBroadcast


class RequireHash : public FullySpecifiedPartitioningRequirement
{
public:

  RequireHash (PartitioningFunction* partFunc)
     : FullySpecifiedPartitioningRequirement(partFunc)
  {
    CMPASSERT(partFunc->isAHashPartitioningFunction());
    CMPASSERT(NOT partFunc->isASkewedDataPartitioningFunction());
  }

  RequireHash (const RequireHash& other)
    :  FullySpecifiedPartitioningRequirement (other)
  {
  }
     
  // ---------------------------------------------------------------------
  // Perform a type-safe pointer cast.
  // ---------------------------------------------------------------------
  virtual const
  RequireHash*
                   castToRequireHash() const;

  // --------------------------------------------------------------------
  // A method for copying the partitioning function.
  // --------------------------------------------------------------------
  virtual PartitioningRequirement* copy() const;

  // ---------------------------------------------------------------------
  // Print.
  // ---------------------------------------------------------------------
  virtual void print( FILE* ofd = stdout,
                      const char* indent = DEFAULT_INDENT,
                      const char* title = "RequireHash") const;

  virtual void display() const;

}; // class RequireHash


class RequireHashDist : public FullySpecifiedPartitioningRequirement
{
public:

  RequireHashDist (PartitioningFunction* partFunc)
     : FullySpecifiedPartitioningRequirement(partFunc)
  {
    CMPASSERT(partFunc->isAHashDistPartitioningFunction());
    CMPASSERT(NOT partFunc->isASkewedDataPartitioningFunction());
  }

  RequireHashDist (const RequireHashDist& other)
    :  FullySpecifiedPartitioningRequirement (other)
  {
  }
     
  virtual NABoolean partReqAndFuncCompatible
                           (const PartitioningFunction* other) const;

  // ---------------------------------------------------------------------
  // Perform a type-safe pointer cast.
  // ---------------------------------------------------------------------
  virtual const
  RequireHashDist* castToRequireHashDist() const;

  // --------------------------------------------------------------------
  // A method for copying the partitioning function.
  // --------------------------------------------------------------------
  virtual PartitioningRequirement* copy() const;

  // ---------------------------------------------------------------------
  // Print.
  // ---------------------------------------------------------------------
  virtual void print( FILE* ofd = stdout,
                      const char* indent = DEFAULT_INDENT,
                      const char* title = "RequireHashDist") const;

  virtual void display() const;

}; // class RequireHashDist


class RequireHash2 : public FullySpecifiedPartitioningRequirement
{
public:

  RequireHash2 (PartitioningFunction* partFunc)
     : FullySpecifiedPartitioningRequirement(partFunc)
  {
    CMPASSERT(partFunc->isAHash2PartitioningFunction());
    CMPASSERT(NOT partFunc->isASkewedDataPartitioningFunction());
  }

  RequireHash2 (const RequireHash2& other)
    :  FullySpecifiedPartitioningRequirement (other)
  {
  }

  virtual NABoolean partReqAndFuncCompatible
                           (const PartitioningFunction* other) const;

  // ---------------------------------------------------------------------
  // Perform a type-safe pointer cast.
  // ---------------------------------------------------------------------
  virtual const
  RequireHash2* castToRequireHash2() const;

  // --------------------------------------------------------------------
  // A method for copying the partitioning function.
  // --------------------------------------------------------------------
  virtual PartitioningRequirement* copy() const;

  // ---------------------------------------------------------------------
  // Print.
  // ---------------------------------------------------------------------
  virtual void print( FILE* ofd = stdout,
                      const char* indent = DEFAULT_INDENT,
                      const char* title = "RequireHash2") const;

  virtual void display() const;

}; // class RequireHash2

class RequireSkewed : public FullySpecifiedPartitioningRequirement
{
public:

  RequireSkewed (PartitioningFunction* partFunc)
     : FullySpecifiedPartitioningRequirement(partFunc)
  {
    CMPASSERT(partFunc->isASkewedDataPartitioningFunction());
  }

  RequireSkewed (const RequireSkewed& other)
    :  FullySpecifiedPartitioningRequirement (other)
  {
  }

  const ValueIdSet& getPartialPartitioningKey() const;

  NABoolean isRequirementSkewed() const { return TRUE; }

  COMPARE_RESULT 
  comparePartReqToReq(const PartitioningRequirement* other) const;

  virtual NABoolean partReqAndFuncCompatible
                           (const PartitioningFunction* other) const;

  NABoolean isReplicate() const { return getSkewProperty().isBroadcasted(); }

  // ---------------------------------------------------------------------
  // Perform a type-safe pointer cast.
  // ---------------------------------------------------------------------
  virtual const
  RequireSkewed* castToRequireSkewed() const;

  // --------------------------------------------------------------------
  // A method for copying the partitioning function.
  // --------------------------------------------------------------------
  virtual PartitioningRequirement* copy() const;

  // Compare skew data property contained in this object with those
  // contained in a partitioning function
  NABoolean compareSkewRequirement(const PartitioningFunction& other) const;

  virtual const skewProperty& getSkewProperty() const;

  // ---------------------------------------------------------------------
  // Print.
  // ---------------------------------------------------------------------
  virtual void print( FILE* ofd = stdout,
                      const char* indent = DEFAULT_INDENT,
                      const char* title = "RequireSkewed") const;

  virtual void display() const;

protected:
}; // class RequireSkewed

class RequireRange : public FullySpecifiedPartitioningRequirement
{
public:

  RequireRange (PartitioningFunction* partFunc)
     : FullySpecifiedPartitioningRequirement(partFunc)
  {
    CMPASSERT(partFunc->isARangePartitioningFunction());
  }

  RequireRange (const RequireRange& other)
    :  FullySpecifiedPartitioningRequirement (other)
  {
  }
     
  // ---------------------------------------------------------------------
  // Perform a type-safe pointer cast.
  // ---------------------------------------------------------------------
  virtual const
  RequireRange*
                   castToRequireRange() const;

  // --------------------------------------------------------------------
  // A method for copying the partitioning function.
  // --------------------------------------------------------------------
  virtual PartitioningRequirement* copy() const;

  // ---------------------------------------------------------------------
  // Print.
  // ---------------------------------------------------------------------
  virtual void print( FILE* ofd = stdout,
                      const char* indent = DEFAULT_INDENT,
                      const char* title = "RequireRange") const;

  virtual void display() const;

}; // class RequireRange


class RequireRoundRobin : public FullySpecifiedPartitioningRequirement
{
public:

  RequireRoundRobin (PartitioningFunction* partFunc)
     : FullySpecifiedPartitioningRequirement(partFunc)
  {
    CMPASSERT(partFunc->isARoundRobinPartitioningFunction());
  }

  RequireRoundRobin (const RequireRoundRobin& other)
    :  FullySpecifiedPartitioningRequirement (other)
  {
  }
     
  virtual NABoolean partReqAndFuncCompatible
                           (const PartitioningFunction* other) const;

  // ---------------------------------------------------------------------
  // Perform a type-safe pointer cast.
  // ---------------------------------------------------------------------
  virtual const
  RequireRoundRobin*
                   castToRequireRoundRobin() const;

  // --------------------------------------------------------------------
  // A method for copying the partitioning function.
  // --------------------------------------------------------------------
  virtual PartitioningRequirement* copy() const;

  // ---------------------------------------------------------------------
  // Print.
  // ---------------------------------------------------------------------
  virtual void print( FILE* ofd = stdout,
                      const char* indent = DEFAULT_INDENT,
                      const char* title = "RequireRoundRobin") const;

  virtual void display() const;

}; // class RequireRoundRobin

class RequireHive : public FullySpecifiedPartitioningRequirement
{
public:

  RequireHive (PartitioningFunction* partFunc)
     : FullySpecifiedPartitioningRequirement(partFunc)
  {
    CMPASSERT(partFunc->isAHivePartitioningFunction());
  }

  RequireHive(const RequireHive& other)
    :  FullySpecifiedPartitioningRequirement (other)
  {
  }
     
  // ---------------------------------------------------------------------
  // Perform a type-safe pointer cast.
  // ---------------------------------------------------------------------
  virtual const
  RequireHive* castToRequireHive() const { return this; };

  virtual NABoolean isRequirementHive() const { return TRUE; };

  // --------------------------------------------------------------------
  // A method for copying the partitioning function.
  // --------------------------------------------------------------------
  virtual PartitioningRequirement* copy() const 
   { return new (CmpCommon::statementHeap()) RequireHive(*this);}

  // ---------------------------------------------------------------------
  // Print.
  // ---------------------------------------------------------------------
  virtual void print( FILE* ofd = stdout,
                      const char* indent = DEFAULT_INDENT,
                      const char* title = "RequireHive") const
  { FullySpecifiedPartitioningRequirement::print(ofd, indent, title); }

  virtual void display() const { print(); };

}; // class RequireHash

// -----------------------------------------------------------------------
// LOGICAL PARTITIONING REQUIREMENT
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// This is a small helper class for passing logical partitioning
// requirements from the DP2 exchange to the DP2 scan node. This class
// is NOT part of the PartitioningRequirement class hierarchy.
// A LogicalPartitioningRequirement can be added to a ReqdPhysicalProperty
// object. That should only be done by an exchange when it creates a
// context for a DP2 child. Each DP2 operator should pass its logical
// partitioning requirement on to its child.
// -----------------------------------------------------------------------
class LogicalPartitioningRequirement : public NABasicObject
{
public:

  // constructor
  LogicalPartitioningRequirement( 
       PartitioningRequirement *logPartReq,
       LogPhysPartitioningFunction::logPartType logPartType = 
                    LogPhysPartitioningFunction::ANY_LOGICAL_PARTITIONING,
       Lng32 numClients = ANY_NUMBER_OF_PARTITIONS,
       NABoolean mustUsePapa = FALSE,
       NABoolean numPAsForced = FALSE) :
     logPartReq_(logPartReq),
     logPartType_(logPartType),
     numClients_(numClients),
     mustUsePapa_(mustUsePapa),
     numPAsForced_(numPAsForced)
  {}

  // copy constructor
  inline LogicalPartitioningRequirement(
       LogicalPartitioningRequirement &other) :
     logPartReq_(other.logPartReq_),
     logPartType_(other.logPartType_),
     numClients_(other.numClients_),
     mustUsePapa_(other.mustUsePapa_),
     numPAsForced_(other.numPAsForced_)
  {}

  // accessors

  PartitioningRequirement * getLogReq() const
                                                  { return logPartReq_; }
  LogPhysPartitioningFunction::logPartType getLogPartTypeReq() const
                                                  { return logPartType_; }
  Lng32 getNumClientsReq() const                   { return numClients_; }

  void setNumClientsReq(Lng32 n) {numClients_ = n;}

  NABoolean isNumberOfPAsForced() const { return numPAsForced_; }

  NABoolean getMustUsePapa() const                { return mustUsePapa_; }

  // comparison
  COMPARE_RESULT compareLogPartRequirements(
       const LogicalPartitioningRequirement &other) const;

  NABoolean satisfied(const RelExpr * const physExpr,
		      const PhysicalProperty * const physProp) const;

private:

  // ---------------------------------------------------------------------
  // The logical partitioning requirement is the partitioning requirement
  // of the DP2 exchange
  // ---------------------------------------------------------------------
  PartitioningRequirement            *logPartReq_;

  // ---------------------------------------------------------------------
  // The type of logical partitioning to be done by the DP2 fragment.
  // This is usually only set when
  // ---------------------------------------------------------------------
  LogPhysPartitioningFunction::logPartType logPartType_;

  // ---------------------------------------------------------------------
  // How many PA nodes do we want to have working on the DP2 fragment.
  // This is the total number of PAs in all ESPs, not the number of PAs
  // per ESP.
  // ---------------------------------------------------------------------
  Lng32                                     numClients_;

  // ---------------------------------------------------------------------
  // Do we have to use a PAPA node? TRUE forces one, FALSE allows one.
  // ---------------------------------------------------------------------
  NABoolean                                mustUsePapa_;

  // Is number of PAs forced by CQS?
  NABoolean                                 numPAsForced_;
};

#endif /* PARTREQ_H */
