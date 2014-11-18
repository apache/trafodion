/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1995-2014 Hewlett-Packard Development Company, L.P.
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
#ifndef ELEMDDLSGOPTIONS_H
#define ELEMDDLSGOPTIONS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLSGOptions.h
 * Description:  classes for sequence generator options specified in DDL statements
 *               
 * Created:      4/22/08
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "BaseTypes.h"
#include "ElemDDLNode.h"
#include "SequenceGeneratorAttributes.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLSGOptions;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// This enum has a similar one, ComSequenceGeneratorType, in
// common/ComSmallDefs.h. Should keep them in sync.
  enum SG_IE_TYPE { SG_UNKNOWN = 0,
                    SG_INTERNAL,
                    SG_EXTERNAL,
                    SG_INTERNAL_COMPUTED };

  enum CD_TYPE { CD_UNKNOWN = 0,
                 CD_GENERATED_BY_DEFAULT,
                 CD_GENERATED_ALWAYS };

  enum { INDEX_SG_OPT_LIST = 0,
         MAX_ELEM_DDL_SG_OPTS_ARITY };

// -----------------------------------------------------------------------
// definition of base class ElemDDLSGOptions
// -----------------------------------------------------------------------
class ElemDDLSGOptions : public ElemDDLNode
{

public:

  // default constructor
  ElemDDLSGOptions();

  ElemDDLSGOptions(OperatorTypeEnum operType);

  ElemDDLSGOptions(Int32 operType,
                   ElemDDLNode * pSGOptList);

  // virtual destructor
  virtual ~ElemDDLSGOptions();

  // cast
  virtual ElemDDLSGOptions * castToElemDDLSGOptions();

  // method for tracing
  virtual const NAString getText() const;

  // method for building text
  virtual NAString getSyntax() const;

  // Accessors

  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);

  inline Int64 getStartValue()  const { return startValue_; }
  inline Int64 getIncrement()   const { return increment_; }
  inline Int64 getMinValue()    const { return minValue_; }
  inline Int64 getMaxValue()    const { return maxValue_; }

  inline NABoolean getCycle()   const { return cycle_; }
  inline Int64 getCache()   const { return cache_; }

  inline SG_IE_TYPE getIEType()      const { return ieType_; }

  inline CD_TYPE getCDType()      const { return cdType_; }

  inline CollIndex getNumberOfOptions() const { return numOptions_; };

  inline NABoolean isStartValueSpecified()  const { return isStartValueSpec_; }
  inline NABoolean isIncrementSpecified()   const { return isIncrementSpec_; }
  inline NABoolean isMinValueSpecified()    const { return isMinValueSpec_; }
  inline NABoolean isMaxValueSpecified()    const { return isMaxValueSpec_; }
  inline NABoolean isCycleSpecified()       const { return isCycleSpec_; }
  inline NABoolean isCacheSpecified()       const { return isCacheSpec_; }
  
  inline NABoolean isNoMinValue()    const { return isNoMinValue_; }
  inline NABoolean isNoMaxValue()    const { return isNoMaxValue_; }
  inline NABoolean isCycle()         const { return cycle_ == TRUE; }
  inline NABoolean isNoCycle()       const { return cycle_ == FALSE; }
  inline NABoolean isCache()         const { return cache_ > 0; }
  inline NABoolean isNoCache()       const { return isNoCache_; }

  inline NABoolean isInternalSG()      const { return ieType_ == SG_INTERNAL; }
  inline NABoolean isExternalSG()      const { return ieType_ == SG_EXTERNAL; }
  inline NABoolean isUnknownSG()       const { return ieType_ == SG_UNKNOWN; }

  inline NABoolean isGeneratedByDefault()   const { return cdType_ == CD_GENERATED_BY_DEFAULT; }
  inline NABoolean isGeneratedAlways()      const { return cdType_ == CD_GENERATED_ALWAYS; }
  inline NABoolean isUnknownCD()            const { return cdType_ == CD_UNKNOWN; }
  
  // Mutators
  virtual void setChild(Lng32 index, ExprNode * pChildNode);

  inline void setStartValue(Int64 startValue)   { startValue_ = startValue; }
  inline void setIncrement(Int64 increment)     { increment_ = increment; }
  inline void setMinValue(Int64 minValue)       { minValue_ = minValue; }
  inline void setMaxValue(Int64 maxValue)       { maxValue_ = maxValue; }
 
  inline void setIEType(SG_IE_TYPE ieType)      { ieType_ = ieType; }
  inline void setCDType(CD_TYPE cdType)         { cdType_ = cdType; }
  void setCDType(Int32 cdType);


  inline void setStartValueSpec(NABoolean startValue)   { isStartValueSpec_ = startValue; }
  inline void setIncrementSpec(NABoolean increment)     { isIncrementSpec_ = increment; }
  inline void setMinValueSpec(NABoolean minValue)       { isMinValueSpec_ = minValue; }
  inline void setMaxValueSpec(NABoolean maxValue)       { isMaxValueSpec_ = maxValue; }
  inline void setCycleSpec(NABoolean cycle)             { isCycleSpec_ = cycle; }
  inline void setCacheSpec(NABoolean cache)             { isCacheSpec_ = cache; }
 
  inline void setNoMinValue(NABoolean minValue)       { isNoMinValue_ = minValue; }
  inline void setNoMaxValue(NABoolean maxValue)       { isNoMaxValue_ = maxValue; }
  inline void setCycle(NABoolean cycle)               { cycle_ = cycle; }
  inline void setCache(Int64 cache)               { cache_ = cache; }
  
  ComFSDataType getFSDataType() { return fsDataType_; }
  void setFSDataType (ComFSDataType dt) { fsDataType_ = dt;}

  //
  // method for binding
  //

  virtual ExprNode * bindNode(BindWA * pBindWA);

  // queryType:  0, create sequence.  1, alter sequence.  2, IDENTITY col.
  short validate(short queryType);

  short genSGA(SequenceGeneratorAttributes &sga);

  short importSGA(const SequenceGeneratorAttributes *sga);
  short importSGO(const ElemDDLSGOptions *sgo);

  //
  // pointer to child parse nodes
  //

  ElemDDLNode * children_[MAX_ELEM_DDL_SG_OPTS_ARITY];

  //
  // Method for tracing
  //

  NATraceList getDetailInfo() const;

private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  // accessor
  inline ElemDDLNode * getSGOptList() const;

  // mutators
  void initializeDataMembers();
  void setSGOpt(ElemDDLNode * pOptNode);
  
  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------

  // StartValue
  NABoolean isStartValueSpec_;
  Int64 startValue_;

  // Increment
  NABoolean isIncrementSpec_;
  Int64 increment_;  

  // MinValue
  NABoolean isMinValueSpec_;
  NABoolean isNoMinValue_;
  Int64 minValue_; 

  // MaxValue
  NABoolean isMaxValueSpec_;
  NABoolean isNoMaxValue_;
  Int64 maxValue_;  

  // Cycle
  NABoolean isCycleSpec_;
  NABoolean cycle_;

  // Cache
  NABoolean isCacheSpec_;
  NABoolean isNoCache_;
  Int64 cache_;

  // Datatype
  NABoolean isDatatypeSpec_;
  ComFSDataType   fsDataType_;

  // Internal or External SG
  SG_IE_TYPE ieType_;

  // COLUMN Default Type
  CD_TYPE cdType_;

  // Number of options in list
  CollIndex numOptions_;

}; // class ElemDDLSGOptions
#endif 
