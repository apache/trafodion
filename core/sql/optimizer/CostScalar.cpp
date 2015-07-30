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
 * File:         CostScalar.cpp
 * Description:  A numeric class, to be protected from under/overflow
 * Created:      07/21/1998
 * Language:     C++
 *
 *
 *
 **************************************************************************
 */

//
// This file contains the code necessary to initialize a pair of
// NABooleans, both of which determine whether the CostScalar class either :
//  
//   (1) fires off an assertion in the case of a division by zero; or 
//   (2) tries to do something reasonable whenever this happens (best
//       I've come up with is setting the value to COSTSCALAR_MAX)       
// 

//
// These two variables (stored in CostScalar.cpp) determine whether
// or not we give assertion failures in case of div-by-zero stupidities.
//
//   USER_WANTS_DIVZERO_FAILURES : if the user has set the DIVZERO_ENV_VAR,
//                                 then s/he wants to get these assertions
//
//   ASSERTIONS_ALWAYS_ON        : at certain times we always want these assertions to fail
//

#include "CostScalar.h"
#include "NABoolean.h"
#include "CmpCommon.h"


#include "math.h"
#include "float.h"
#include "../exp/exp_ovfl_ptal.h"
// The union type IEEE_double is used to access raw exponent bits
// of a real IEEE value. Since the layout is different we need 2 
// definitions of this union type, one - for Windows and one - 
// for NSK. By casting address of CostScalar double dpv_ attribute
// to the pointer to this type we will be able to access exponent
// bits to prevent not only overflow/underflow but also to prevent
// getting dpv_ value out of the range [-COSTSCALAR_MAX, COSTSCALAR_MAX]
typedef union { double    real;
                Lng32  int32[2];
                struct field { 
                               UInt32 LSB_mant:32;
                               UInt32 MSB_mant:20;
                               UInt32 exp     :11;
                               UInt32 sign    : 1;
                             } field;
              } IEEE_double;

THREAD_P NABoolean USER_WANTS_DIVZERO_FAILURES ;
// USER_WANTS_DIVZERO_FAILURES = ( getenv( DIVZERO_ENV_VAR ) == NULL ) ? FALSE : TRUE ;
THREAD_P NABoolean ASSERTIONS_ALWAYS_ON = TRUE ; // until we stabilize FCS


// Commonly used CostScalar
const CostScalar csZero = 0.0;
const CostScalar csOne  = 1.0;
const CostScalar csTwo  = 2.0;
//to be used instead of DBL_MIN when we don't want to use csZero
const CostScalar csEpsilon = COSTSCALAR_MICRO_EPSILON; 
// to be used instead of DBL_MAX, see COSTSCALAR_MAX in .h file
const CostScalar csMax  =  COSTSCALAR_MAX;
const CostScalar csMin  = -COSTSCALAR_MAX;
const CostScalar csMinusOne = -1.0;
const CostScalar csOneKiloBytes = 1024.0;
// Because IEEE exponent is biased (increased by 1023)
// If we want to check the result of IEEE multiplication we need to
// compare the sum of two exponents with 2046 + CS_MAX_BIN_EXP
const Int32 maxBiasedExpForMult = CS_MAX_BIN_EXP + 2046;
const Int32 minBiasedExpForMult = CS_MIN_BIN_EXP + 2046;
 
CostScalar CostScalar::operator * (const CostScalar &other) const
{ 
  {
      IEEE_double * X = (IEEE_double *)&dpv_;
      IEEE_double * Y = (IEEE_double *)&(other.dpv_);

      register Int32 prodExp = X->field.exp + Y->field.exp;
      if ( prodExp < minBiasedExpForMult )
      {
          udflwCount_++;
          return csZero;
      }
      else if ( prodExp > maxBiasedExpForMult )
      {
           ovflwCount_++;
           if ( X->field.sign == Y->field.sign )
               return csMax;
           else
               return csMin;
      }
      return CostScalar( dpv_ * other.dpv_ );
  }
}

CostScalar CostScalar::operator / (const CostScalar &other) const
{ 
  {
      IEEE_double * X = (IEEE_double *)&dpv_;
      IEEE_double * Y = (IEEE_double *)&(other.dpv_);

      if ( X->field.exp == 0 )
          return csZero;
      if ( Y->field.exp == 0 )
      {
           ovflwCount_++;
           if ( X->field.sign == Y->field.sign )
               return csMax;
           else
               return csMin;
      }
      
      register Int32 divExp = X->field.exp;
      divExp -= Y->field.exp;
      if ( divExp < CS_MIN_BIN_EXP )
      {
          udflwCount_++;
          return csZero;
      }
      else if ( divExp > CS_MAX_BIN_EXP )
      {
           ovflwCount_++;
           if ( X->field.sign == Y->field.sign )
               return csMax;
           else
               return csMin;
      }
      return CostScalar( dpv_ / other.dpv_ );
  }
}
