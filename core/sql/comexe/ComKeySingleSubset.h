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
****************************************************************************
*
* File:         ComKeySingleSubset.h
* Description:  
*
* Created:      5/6/98
* Language:     C++
*
*
*
*
****************************************************************************
*/

#ifndef COMKEYSINGLESUBSET_H
#define COMKEYSINGLESUBSET_H

#include "ComKeyRange.h"

class ex_expr;
class ex_cri_desc;

// -----------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////
//
// Class keySingleSubsetGen
//
// This class contains compiler-generated information used by scan
// operators that access a single key range.  It encapsulates methods
// needed to compute the begin and end key values and their exclusion
// flags.
//
//
/////////////////////////////////////////////////////////////////////////

class keySingleSubsetGen : public keyRangeGen
{
private:

  // expressions that compute begin and end keys respectively
  ExExprPtr bkPred_;                    // 00-07
  ExExprPtr ekPred_;                    // 08-15
  
  // boolean expression to compute whether low and high key should
  // be included in the range or subset
  ExExprPtr lowKeyExcludedExpr_;        // 16-23
  ExExprPtr highKeyExcludedExpr_;       // 24-31

  // hard-coded indicators whether to include low and/or high key
  // (used if the expressions above are NULL)
  Int16 lowKeyExcluded_;                // 32-33
  Int16 highKeyExcluded_;               // 34-35

  char fillersKeySingleSubsetGen_[20];  // 36-55

public:

  keySingleSubsetGen() {};  // default constructor used by UNPACK

  keySingleSubsetGen(ULng32 keyLen,
				ex_cri_desc * workCriDesc,
				unsigned short keyValuesAtpIndex,
				unsigned short excludeFlagAtpIndex,
				unsigned short dataConvErrorFlagAtpIndex,
				ex_expr * bk_pred,
				ex_expr * ek_pred,
				ex_expr * bkey_excluded_expr,
				ex_expr * ekey_excluded_expr,
				short bkey_excluded,
				short ekey_excluded);

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    keyRangeGen::populateImageVersionIDArray();
  }

  virtual short getClassSize()
                             { return (short)sizeof(keySingleSubsetGen); }

  virtual ~keySingleSubsetGen();
  
  virtual Long pack(void * space);
  virtual Lng32 unpack(void * base, void * reallocator);

  virtual ex_expr* getExpressionNode(Int32 pos);

  virtual keySingleSubsetGen * castToKeySingleSubsetGen() { return this; }

  // accessor functions
  inline ex_expr * bkPred() const
    { return bkPred_; };

  inline ex_expr * ekPred() const
    { return ekPred_; };

  inline ex_expr * bkExcludedExpr() const
    { return lowKeyExcludedExpr_; }

  inline ex_expr * ekExcludedExpr() const
    { return highKeyExcludedExpr_; }

  inline short isBkeyExcluded() const 
    { return lowKeyExcluded_;}

  inline short isEkeyExcluded() const 
    { return highKeyExcluded_;}


};

#endif
