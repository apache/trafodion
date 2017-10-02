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
#ifndef ITEMARITH_H
#define ITEMARITH_H
/* -*-C++-*-
******************************************************************************
*
* File:         ItemArith.h
* Description:  Arithmetic item expressions (+, -, *, /, **, unary -, ...)
*
*               
* Created:      11/04/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "QRExprElement.h"

// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class BiArith;

// forward references
class Generator;

// -----------------------------------------------------------------------
// binary arithmetic operators (+, -, *, /, **)
// -----------------------------------------------------------------------
class BiArith : public ItemExpr
{
  // ITM_PLUS, ITM_MINUS, ITM_TIMES, ITM_DIVIDE, ITM_EXPONENT
public:
  BiArith(OperatorTypeEnum otype,
	  ItemExpr *child0 = NULL,
	  ItemExpr *child1 = NULL)
       : ItemExpr(otype,child0,child1),
	 unaryNegate_(FALSE),
	 intervalQualifier_(NULL),
	 roundingMode_(0),
         ignoreSpecialRounding_(FALSE),
	 divToDownscale_(FALSE),
	  normalizeFlags_(0)
  {}

  // virtual destructor
  virtual ~BiArith() {}
 
  // accessor functions
  NABoolean isUnaryNegate() const 		{ return unaryNegate_; }
  void setIsUnaryNegate(NABoolean u = TRUE)	{ unaryNegate_ = u; }

  NABoolean isStandardNormalization() const 		{ return normalizeFlags_ == LAST_DAY_ON_ERR; }
  void setStandardNormalization()	                { normalizeFlags_ = LAST_DAY_ON_ERR; }

  NABoolean isKeepLastDay() const 		{ return normalizeFlags_ == LAST_DAY_OF_MONTH_FLAG; }
  void setKeepLastDay()	                { normalizeFlags_ = LAST_DAY_OF_MONTH_FLAG; }
  NABoolean isDateMathFunction() const  { return (isStandardNormalization() || isKeepLastDay()); }

  const NAType* getIntervalQualifier() const 	{ return intervalQualifier_; }
  void setIntervalQualifier(const NAType* intervalQualifier)
				   { intervalQualifier_ = intervalQualifier; }

  // get the degree of this node (it is a binary op).
  virtual Int32 getArity() const;

  // get a printable string that identifies the operator
  const NAString getText() const;

  virtual ConstValue *castToConstValue(NABoolean & negate);

  virtual NABoolean duplicateMatch(const ItemExpr &other) const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = NULL);

  virtual ItemExpr * simplifyOrderExpr(OrderComparison *newOrder = NULL);

  virtual NABoolean isOrderPreserving() const;

  // convert expressions of constant values into a single constant
  virtual ItemExpr *foldConstants(ComDiagsArea *diagsArea,
				  NABoolean newTypeSynthesis = FALSE);

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr *bindNode(BindWA *bindWA);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // determines whether this value can be produced by the same generated
  // expression as another (for common subexpression elimination)
  virtual NABoolean isEquivalentForCodeGeneration(const ItemExpr * other);

  // method to do code generation
  ItemExpr * preCodeGen(Generator*);
  short codeGen(Generator*);

  // change literals of a cacheable query into ConstantParameters 
  virtual ItemExpr* normalizeForCache(CacheWA& cachewa, BindWA& bindWA);

  // append an indication of rounding mode for datetime arithmetic functions
  virtual void generateCacheKey(CacheWA& cwa) const;

  virtual NABoolean calculateMinMaxUecs(ColStatDescList & histograms,
					       CostScalar & minUec,
					       CostScalar & maxUec);

  void setRoundingMode(short roundingMode)
  {
    roundingMode_ = roundingMode;
  }
  short getRoundingMode() const { return roundingMode_; }

  void setIgnoreSpecialRounding() 
  {
    ignoreSpecialRounding_ = TRUE;
    roundingMode_ = 0 ; // no rounding
  }

  NABoolean ignoreSpecialRounding() const { return ignoreSpecialRounding_; }

  void setDivToDownscale(NABoolean divToDownscale)
  {
    divToDownscale_ = divToDownscale;
  }
  NABoolean getDivToDownscale() const { return divToDownscale_; }

  virtual QR::ExprElement getQRExprElem() const;
  virtual NABoolean hasEquivalentProperties(ItemExpr * other);

private:

  // Parser implements "-x" (unary negate) as "0 - x" (const zero minus x).
  // Put this flag here so SynthType can differentiate.
  NABoolean unaryNegate_;

  UInt16           normalizeFlags_;
  enum normalize_flags_type 
    {
    LAST_DAY_ON_ERR        = 0x0001,  // If the ending day of resulting day is invalid,
                                      // day will be rounded down.
    LAST_DAY_OF_MONTH_FLAG = 0x0002   // used to set processing to the last day
                                      // of month when adding month/year intervals
    }; 

  // For "(datetime - datetime) interval_qualifier" (ANSI 6.15).
  const NAType *intervalQualifier_;

  // controls rounding of arith and cast results.
  // 0: truncate(no rounding)
  // 1: round half up (values .5 and up are rounded to nearest high digit).
  // 2: round half even
  short roundingMode_;

  // If set, ignore any special (1 or 2) rounding mode setting for this item 
  NABoolean ignoreSpecialRounding_;

  // if TRUE, indicates that this division is to downscale the
  // result.
  NABoolean divToDownscale_;
};

// Special case of BiArith -- sum
// 
// 
class BiArithSum : public BiArith
{
public:
  BiArithSum(OperatorTypeEnum otype,
		 ItemExpr *child0 = NULL,
		 ItemExpr *child1 = NULL)
    : BiArith(otype,child0,child1)
  {}
  
  // virtual destructor
  virtual ~BiArithSum() {}
 
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = NULL);

  virtual ItemExpr *preCodeGen(Generator*);
  virtual short codeGen(Generator*);

  virtual const NAString getText() const
  {
    return "BiArithSum";
  };
  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

private:
};

// Special case of BiArith -- count
// 
// 
class BiArithCount : public BiArith
{
public:
  inline BiArithCount(OperatorTypeEnum otype,
		      ItemExpr *child0 = NULL,
		      ItemExpr *child1 = NULL)
    : BiArith(otype,child0,child1)
    {}
  
  // virtual destructor
  inline virtual ~BiArithCount() {}
 
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = NULL);

  virtual ItemExpr *preCodeGen(Generator*);
  virtual short codeGen(Generator*);
  
  virtual const NAString getText() const
  {
    return "BiArithCount";
  };
  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

private:
};

class UnArith : public ItemExpr
{
  // ITM_NEGATE. Used with negation of boolean native datatype
public:
  UnArith(ItemExpr *child0 = NULL)
       : ItemExpr(ITM_NEGATE,child0,NULL)
  {
  }

  // get the degree of this node (it is a binary op).
  virtual Int32 getArity() const { return 1;}

  // get a printable string that identifies the operator
  const NAString getText() const { return '!'; }

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = NULL);

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr *bindNode(BindWA *bindWA);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // method to do code generation
  ItemExpr * preCodeGen(Generator*);
  short codeGen(Generator*);

private:
};


#endif /* ITEMARITH_H */
