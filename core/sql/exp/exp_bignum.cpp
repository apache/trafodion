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
 * File:         exp_bignum.cpp
 * Description:  Implementation of class BigNum
 *               
 *               
 * Created:      3/31/99
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"  


#include "BigNumHelper.h"
#include "exp_bignum.h"
#include "Int64.h"
#include "str.h"
#include "SQLTypeDefs.h"
#include "exp_clause_derived.h"

#include <iostream>
#include <stdlib.h>

BigNum::BigNum(Lng32 length, Lng32 precision, short scale, short unSigned)
  : length_(length),
    precision_(precision),
    scale_(scale),
    unSigned_(unSigned),
    tempSpacePtr_(0)
{
  setClassID(BigNumID);  
}

BigNum::BigNum()
{
  setClassID(BigNumID);  
}

BigNum::~BigNum()
{
}

Attributes * BigNum::newCopy()
{
  BigNum * new_copy = new BigNum(); 
  *new_copy = *this;
  return new_copy;
};

Attributes * BigNum::newCopy(NAMemory * heap)
{
  BigNum * new_copy = new(heap) BigNum(); 
  *new_copy = *this;
  return new_copy;
};

void BigNum::copyAttrs(Attributes *source) 
{
  *this = *((BigNum *) source);
  return;
};

Lng32 BigNum::setTempSpaceInfo(OperatorTypeEnum operType,
				ULong offset, Lng32 length)
{
  if (length > 0)
    {
      // if length is passed in and offset is temp space address.
      tempSpaceLength_ = length;
      tempSpacePtr_ = offset;
    }
  else
    if (operType == ITM_DIVIDE || 
        operType == ITM_CAST || 
        operType == ITM_NARROW) // Narrow is another form of CAST
      {
	// compute the worst case temp space needed.
	// See the castFrom(), div(), and BigNumHelper::DivHelper() methods for details.
	tempSpaceLength_ = 3 * getLength() + 3*sizeof(short) ; // worst case
	tempSpaceOffset_ =  (UInt32) offset;
      }
    else
      {
        tempSpaceOffset_ = 0;
        tempSpaceLength_ = 0;
      }
  
  return tempSpaceLength_;
}

void BigNum::fixup(Space * space,
                   char * constantsArea,
		   char * tempsArea,
		   char * persistentArea,
		   short fixupConstsAndTemps,
                   NABoolean spaceCompOnly)
{
  
  if ((tempSpaceLength_ > 0) && (NOT spaceCompOnly))
    tempSpacePtr_ = (Long)(tempsArea + tempSpaceOffset_);

  Attributes::fixup(space, constantsArea, tempsArea, persistentArea,
		    fixupConstsAndTemps, spaceCompOnly);

}


// Convert (i.e. copy) the Big Num from op_data[1] ("source") to op_data[0] ("this").

short BigNum::conv(Attributes * source, char * op_data[])
{
  return BigNumHelper::ConvBigNumWithSignToBigNumWithSignHelper(((BigNum *) source)->getLength(),
                                                                getLength(),
                                                                op_data[1],
                                                                op_data[0]);
}


// Compare the Big Num in op_data[1] ("this") with op_data[2] ("other").
// Return  1, if "this <compOp> other" is true,0, otherwise.

short BigNum::comp (OperatorTypeEnum compOp,
		    Attributes * other, 
		    char * op_data[])
{
  // Recast char strings as arrays of unsigned shorts
  unsigned short * thisDataInShorts = (unsigned short *) op_data[1];
  unsigned short * otherDataInShorts = (unsigned short *) op_data[2];

  // Extract signs.
  char thisSign = BIGN_GET_SIGN(op_data[1], getLength());
  char otherSign = BIGN_GET_SIGN(op_data[2], getLength());

  // Clear sign bits
  BIGN_CLR_SIGN(op_data[1], getLength());
  BIGN_CLR_SIGN(op_data[2], getLength());

  short returnValue = 0;
  short compCode = 0;

  // if the signs are different but 'this' and 'other' values are zeroes,
  // then they are equal.
  NABoolean allZeroes = FALSE;
  if (thisSign != otherSign) {
    allZeroes = TRUE;
    // check if the values are zeroes.
    for (Lng32 i = 0; i < getLength()/2; i++) {
      // if a nonzero is found, break out.
      if ((thisDataInShorts[i] != 0) ||
	  (otherDataInShorts[i] != 0)) {
	allZeroes = FALSE;
	break;
      }
    } // for
  } // if
  
  if (allZeroes)
    compCode = 0;
  else {
    // We calculate compCode as follows: if this > other, then compCode = 1
    //                                   if this < other, then compCode = -1
    //                                   if this = other, then compCode = 0
    
    if ((thisSign == 0) && (otherSign))     // this +ve, other -ve
      compCode = 1;
    else
      if ((thisSign) && (otherSign == 0))  // this -ve, other +ve
	compCode = -1;
      else {                                        // both same sign, so check
	                                            // if magnitude of this is 
	                                            // larger than other

	for (Lng32 i = 0; i < getLength()/2; i++) {
	  if (otherDataInShorts[i] > thisDataInShorts[i]) 
	    compCode = -1;
	  else if (otherDataInShorts[i] < thisDataInShorts[i])
	    compCode = 1;
	}

	if (thisSign)                         // both -ve, so if magnitudes 
	                                      // are different, switch
	                                      // compCode
	  compCode = (compCode ? -compCode : 0); 
      } 
  }
  
  switch (compOp) {
    
  case ITM_EQUAL:
    returnValue = ((compCode == 0) ? 1 : 0);
    break;
    
  case ITM_NOT_EQUAL:
    returnValue = ((compCode != 0) ? 1 : 0);
    break;
    
  case ITM_LESS:
    returnValue = ((compCode < 0) ? 1 : 0);
    break;
    
  case ITM_LESS_EQ:
    returnValue = ((compCode <= 0) ? 1 : 0);
    break;
    
  case ITM_GREATER:
    returnValue = ((compCode > 0) ? 1 : 0);
    break;
    
  case ITM_GREATER_EQ:
    returnValue = ((compCode >= 0) ? 1 : 0);
    break;
    
  }

  // Reset sign bits
  if (thisSign)
    BIGN_SET_SIGN(op_data[1], getLength());

  if (otherSign)
    BIGN_SET_SIGN(op_data[2], getLength());
  
  return returnValue;
}


// Add the Big Nums in op_data[1] ("left") and op_data[2] ("right")
// and store in op_data[0] ("this").

short BigNum::add(Attributes * left,
		  Attributes * right,
		  char * op_data[])
{
  // Extract signs.
  char leftSign = BIGN_GET_SIGN(op_data[1], getLength());
  char rightSign = BIGN_GET_SIGN(op_data[2], getLength());

  // Clear sign bits
  BIGN_CLR_SIGN(op_data[1], getLength());
  BIGN_CLR_SIGN(op_data[2], getLength());
 
  Int32 code;
  
  if ((!isUnsigned()) && ((leftSign) || (rightSign))) {
    // Signed addition. Either left or right is signed.
    if ((leftSign == 0) && (rightSign)) {
      code = BigNumHelper::SubHelper(getLength(), op_data[1], op_data[2], op_data[0]); 
      }
    else
      if ((leftSign) && (rightSign == 0)) {
        code = BigNumHelper::SubHelper(getLength(), op_data[2], op_data[1], op_data[0]);
        }
      else {
        // Both left and right are negative numbers
        BigNumHelper::AddHelper(getLength(), op_data[1], op_data[2], op_data[0]);
        code = 1;
        }
    }
  else {
    // Both are positive numbers
    BigNumHelper::AddHelper(getLength(), op_data[1], op_data[2], op_data[0]);
    code = 0;
    }

  // Reset sign bits
  if (leftSign)
    BIGN_SET_SIGN(op_data[1], getLength());

  if (rightSign)
    BIGN_SET_SIGN(op_data[2], getLength());

  if (code) {
    BIGN_SET_SIGN(op_data[0], getLength());
  } else {
    BIGN_CLR_SIGN(op_data[0], getLength());
  }

  return 0;
};


// Subtract the Big Num in op_data[2] ("right") from op_data[1] ("left")
// and store in op_data[0] ("this").

short BigNum::sub(Attributes * left,
		  Attributes * right,
		  char * op_data[])
{
  // Extract signs.
  char leftSign = BIGN_GET_SIGN(op_data[1], getLength());
  char rightSign = BIGN_GET_SIGN(op_data[2], getLength());

  // Clear sign bits
  BIGN_CLR_SIGN(op_data[1], getLength());
  BIGN_CLR_SIGN(op_data[2], getLength());
 
  Int32 code;

  if ((!isUnsigned()) && ((leftSign) || (rightSign))) {
    if (leftSign == 0) { // left - (-right) => left + right
      BigNumHelper::AddHelper(getLength(), op_data[1], op_data[2], op_data[0]);
      code = 0;
      }
    else
      if (rightSign == 0) { // -left - right => -(left + right)
        BigNumHelper::AddHelper(getLength(), op_data[1], op_data[2], op_data[0]);
        code = 1;
        }
      else
        { // -left - (-right) => right - left
        code = BigNumHelper::SubHelper(getLength(), op_data[2], op_data[1], op_data[0]);
        }
    }
  else {
    code = BigNumHelper::SubHelper(getLength(), op_data[1], op_data[2], op_data[0]);
    }

  // Reset sign bits
  if (leftSign)
    BIGN_SET_SIGN(op_data[1], getLength());

  if (rightSign)
    BIGN_SET_SIGN(op_data[2], getLength());

  if (code) {
    BIGN_SET_SIGN(op_data[0], getLength());
  } else {
    BIGN_CLR_SIGN(op_data[0], getLength());
  }
  
  return 0;
}


// Multiply the Big Nums in op_data[1] ("left") and op_data[2] ("right")
// and store in op_data[0] ("this").

short BigNum::mul(Attributes * left,
		  Attributes * right,
		  char * op_data[])
{
  // Extract signs.
  char leftSign = BIGN_GET_SIGN(op_data[1], ((BigNum*) left)->getLength());
  char rightSign = BIGN_GET_SIGN(op_data[2], ((BigNum*) right)->getLength());

  // Clear sign bits
  BIGN_CLR_SIGN(op_data[1], ((BigNum*) left)->getLength());
  BIGN_CLR_SIGN(op_data[2], ((BigNum*) right)->getLength());

  BigNumHelper::MulHelper(getLength(), 
                          ((BigNum *) left)->getLength(),
                          ((BigNum *) right)->getLength(),
                          op_data[1],
                          op_data[2],
                          op_data[0]);
  
  // Reset sign bits
  if (leftSign)
    BIGN_SET_SIGN(op_data[1], ((BigNum*) left)->getLength());

  if (rightSign)
    BIGN_SET_SIGN(op_data[2], ((BigNum*) right)->getLength());

  if (leftSign == rightSign) {
    BIGN_CLR_SIGN(op_data[0], getLength());
  } else {
    BIGN_SET_SIGN(op_data[0], getLength());
  }
    
  return 0;
}


// Divide the Big Num in op_data[1] ("left") by op_data[2] ("right")
// and store in op_data[0] ("this").

short BigNum::div(Attributes * left,
		  Attributes * right,
		  char * op_data[],
		  NAMemory *heap,
		  ComDiagsArea** diagsArea)
{
  // Extract signs.
  // Extract signs.
  char leftSign = BIGN_GET_SIGN(op_data[1], ((BigNum*) left)->getLength());
  char rightSign = BIGN_GET_SIGN(op_data[2], ((BigNum*) right)->getLength());

  // Clear sign bits
  BIGN_CLR_SIGN(op_data[1], ((BigNum*) left)->getLength());
  BIGN_CLR_SIGN(op_data[2], ((BigNum*) right)->getLength());

  // Ignore trailing zeros in right. 
  unsigned short * rightDataInShorts = (unsigned short *) op_data[2];
  Lng32 rightLengthInShorts = (((BigNum *) right)->getLength())/2; 
  while ((rightDataInShorts[rightLengthInShorts - 1] == 0) && (rightLengthInShorts > 0))
    rightLengthInShorts--;

  if (rightLengthInShorts == 0) {
    if (heap)
      ExRaiseSqlError(heap, diagsArea, EXE_DIVISION_BY_ZERO);

    // Reset sign bits
    if (leftSign)
      BIGN_SET_SIGN(op_data[1], ((BigNum*) left)->getLength());

    if (rightSign)
      BIGN_SET_SIGN(op_data[2], ((BigNum*) right)->getLength());

    return -1;
  }

  if (rightLengthInShorts == 1) {
    BigNumHelper::SimpleDivHelper(((BigNum *) left)->getLength(),
                                  2, 
                                  op_data[1],
                                  op_data[2], 
                                  op_data[0]); 
    }
  else {
    BigNumHelper::DivHelper(((BigNum *) left)->getLength(), 
                            2*rightLengthInShorts, 
                            op_data[1],
                            op_data[2], 
                            op_data[0],
                            (char *) tempSpacePtr_);
    }
  
  if (leftSign)
    BIGN_SET_SIGN(op_data[1], ((BigNum*) left)->getLength());

  if (rightSign)
    BIGN_SET_SIGN(op_data[2], ((BigNum*) right)->getLength());

  if (leftSign == rightSign) {
    BIGN_CLR_SIGN(op_data[0], getLength());
  } else {
    BIGN_SET_SIGN(op_data[0], getLength());
  }

  return 0;
}


// This method converts from other simple types to Big Num

short BigNum::castFrom (Attributes * source, 
			char * op_data[],
			NAMemory *heap,
			ComDiagsArea** diagsArea)
{
  
  SimpleType * source1 = (SimpleType *) source;
  short sourceType = source1->getDatatype();
  unsigned short * thisDataInShorts = (unsigned short *) op_data[0];
  
  if ((sourceType >= REC_MIN_INTERVAL) && (sourceType <= REC_MAX_INTERVAL)) {
    switch (source1->getLength()) {
    case SQL_SMALL_SIZE:
      sourceType = REC_BIN16_SIGNED;
      break;
    case SQL_INT_SIZE:
      sourceType = REC_BIN32_SIGNED;
      break;
    case SQL_LARGE_SIZE:
      sourceType = REC_BIN64_SIGNED;
      break;
    }
  }

  // Initialize magnitude of Big Num to zeros.
  for (Int32 k = 0; k < getLength(); k++)
    op_data[0][k] = 0;
  
  switch (sourceType) {
    case REC_BIN16_SIGNED: {
      if ( *((short *) op_data[1]) < 0) {
        thisDataInShorts[0] = -*((short *) op_data[1]);
        BIGN_SET_SIGN(op_data[0], getLength());
        }
      else {
        thisDataInShorts[0] = *((short *) op_data[1]);
        }
      }
      break;
     
    case REC_BPINT_UNSIGNED:
    case REC_BIN16_UNSIGNED: {
      thisDataInShorts[0] = *((unsigned short *) op_data[1]);
      }
      break;
      
    case REC_BIN32_SIGNED: {
      if ( *((Lng32 *) op_data[1]) < 0) {
        *((ULng32 *) op_data[0])  = -*((Lng32 *) op_data[1]);
        BIGN_SET_SIGN(op_data[0], getLength());
        }
      else {
        *((ULng32 *) op_data[0])  = *((Lng32 *) op_data[1]);
        }

#ifdef NA_LITTLE_ENDIAN
      // Do nothing, target is already in right format.
#else
      // Reverse the shorts in the target.
      unsigned short temp = thisDataInShorts[0];
      thisDataInShorts[0] = thisDataInShorts[1];
      thisDataInShorts[1] = temp;
#endif
      }
      break;
      
    case REC_BIN32_UNSIGNED: {
      *((ULng32 *) op_data[0])  = *((ULng32 *) op_data[1]);

#ifdef NA_LITTLE_ENDIAN
      // Do nothing, target is already in right format.
#else
      // Reverse the shorts in the target.
      unsigned short temp = thisDataInShorts[0];
      thisDataInShorts[0] = thisDataInShorts[1];
      thisDataInShorts[1] = temp;
#endif
      }
      break;

    case REC_BIN64_SIGNED: {
      // Since this case is a little more complex, we call a helper method. 
      BigNumHelper::ConvInt64ToBigNumWithSignHelper(getLength(),
                                                    *((Int64 *) op_data[1]),
                                                    op_data[0],
                                                    FALSE);
      }
      break;
      
    case REC_BIN64_UNSIGNED: {
      // Since this case is a little more complex, we call a helper method. 
      BigNumHelper::ConvInt64ToBigNumWithSignHelper(getLength(),
                                                    *((Int64 *) op_data[1]),
                                                    op_data[0],
                                                    TRUE);
      }
      break;
      
    case REC_DECIMAL_LSE: {
      // Remember first char of source in temp
      char temp = op_data[1][0];

      // Temporarily suppress sign bit in source
      op_data[1][0] = op_data[1][0] & 0177;

      // Convert source from ASCII to Big Num without sign and store in this. 
      BigNumHelper::ConvAsciiToBigNumHelper(source1->getLength(),
                                            getLength(),
                                            op_data[1],
                                            op_data[0]);

      // Set sign in this (must be done after conversion to prevent overwrite)
      if (temp & 0200)
        BIGN_SET_SIGN(op_data[0], getLength());

      // Restore first char in source
      op_data[1][0] = temp;

      }
      break;

    case REC_DECIMAL_UNSIGNED: {
      // Convert source from ASCII to Big Num without sign and store in this. 
      BigNumHelper::ConvAsciiToBigNumHelper(source1->getLength(), 
                                            getLength(),
                                            op_data[1],
                                            op_data[0]);
      }
      break;

    case REC_FLOAT32: {
      // The code below is modeled on the corresponding code in large decimals.

      char tempTarget[SQL_REAL_DISPLAY_SIZE + 1];
      // Map of tempTarget:  -d.dddddddE+dddn  
      //                     0123456789012345
      //                               1
      // where '-' is '-' or ' '; '+' is '+' or '-'; n is null character;
      //   and 'd' is a digit in ascii.
      if (convDoIt(op_data[1], 4, REC_FLOAT32, 0, 0,
		   tempTarget, SQL_REAL_DISPLAY_SIZE, REC_BYTE_F_ASCII, 0, 0,
		   NULL, 0, heap, diagsArea, CONV_UNKNOWN_LEFTPAD) != ex_expr::EXPR_OK)
	return -1;

      Lng32 i;

      // Compute the 8-digit mantissa by skipping the decimal point.
      ULng32 mantissa = 0;
      for (i = 1; i <= 9; i++) {
        if (i != 2)  
          mantissa = mantissa * 10 + (tempTarget[i] - '0');
        }

      // Get the exponent - absolute value only
      Lng32 exponent = 0;
      Lng32 multiplier = 100;
      for (i = 12; i < 15; i++) {
          exponent += (tempTarget[i] - '0') * multiplier;
          multiplier /= 10;
        }

      // If we're dealing with a positive exponent, then we have at least
      // "exponent" number of digits before a decimal point followed by "scale"
      // digits.  Make sure that the number of digits to the left of the
      // decimal point (i.e. precision - scale) is >= "exponent" number of
      // digits.  Special care is taken when the float is less than 1 - in this
      // case, the 0 digit before the decimal point should not count towards
      // the precision, since it will be ignored.

      if (tempTarget[11] != '-') {
        Int32 includeFirstDigit = (tempTarget[1] == '0') ? 0 : 1;
        if ((exponent + includeFirstDigit) > (getPrecision() - getScale()))
          return -1;
      }

      if (tempTarget[11] == '-')
        exponent = -exponent;

      // Subtract 7 from the exponent.  This will set the exponent exactly
      // where the decimal point should be in the mantissa.
      exponent -= 7;

      // Shift down the mantissa so as to leave "scale" digits to the right of
      // the decimal point.
      Lng32 scaleBy = getScale() + exponent;
      if (scaleBy < 0)
	{
          Int8 roundMe= 0;
	  for (i = 0; i < -scaleBy; i++)
	  {
	    roundMe = ( mantissa %10 ) > 4? 1:0;
	    mantissa /= 10;
	  }

	  if(roundMe == 1) //round it
	    mantissa++;

	  scaleBy = 0;
	}

      // At this stage, our source is effectively = mantissa * 10^exponent.
      // We need to create the target Big Num by multiplying the mantissa 
      // with 10^(scale + exponent).

      // Create a Big Num (without sign) for 10^(scale + exponent);
      // This can be stored in the temporary area pointed to by tempSpacePtr_.
      char * tempPowersOfTen = (char *) tempSpacePtr_;
      Lng32 tempPowersOfTenLength;
      BigNumHelper::ConvPowersOfTenToBigNumHelper(scaleBy,
                                                  tempSpaceLength_, 
                                                  &tempPowersOfTenLength,
                                                  tempPowersOfTen);

      // Convert the mantissa into a Big Num representation (without sign).
      unsigned short * mantissaDataInShorts = (unsigned short *) &mantissa;
#ifdef NA_LITTLE_ENDIAN
      // Do nothing, mantissaData is already in the correct format.
#else
      // Swap the two shorts in mantissaData.
      unsigned short temp = mantissaDataInShorts[0];
      mantissaDataInShorts[0] = mantissaDataInShorts[1];
      mantissaDataInShorts[1] = temp;
#endif

      // Multiply mantissaData with tempPowersOfTen and store in this. 
      BigNumHelper::MulHelper(getLength(), 
                              4, 
                              tempPowersOfTenLength,
                              (char *) &mantissa,
                              tempPowersOfTen,
                              op_data[0]);

      // Set sign of this.
      if ( tempTarget[0] == '-')
        BIGN_SET_SIGN(op_data[0], getLength());

      }

      break;
    case REC_FLOAT64: {

      // The code below is modeled on the corresponding code in large decimals.

      char tempTarget[SQL_FLOAT_DISPLAY_SIZE + 1];
      // Map of tempTarget:  -d.dddddddddddddddddE+dddn  
      //                     01234567890123456789012345
      //                               1         2
      // where '-' is '-' or ' '; '+' is '+' or '-'; n is null character;
      //   and 'd' is a digit in ascii.
      if (convDoIt(op_data[1], 8, REC_FLOAT64, 0, 0,
		   tempTarget, SQL_FLOAT_DISPLAY_SIZE, REC_BYTE_F_ASCII, 0, 0,
		   NULL, 0, heap, diagsArea, CONV_UNKNOWN_LEFTPAD) != ex_expr::EXPR_OK)
	return -1;

      Lng32 i;

      // Compute the 18-digit mantissa by skipping the decimal point.
      Int64 mantissa = 0;
      for (i = 1; i <= 19; i++) {
        if (i != 2)  
          mantissa = mantissa * 10 + (tempTarget[i] - '0');
        }

      // Get the exponent - absolute value only
      Lng32 exponent = 0;
      Lng32 multiplier = 100;
      for (i = 22; i < 25; i++)
        {
          exponent += (tempTarget[i] - '0') * multiplier;
          multiplier /= 10;
        }

      // If we're dealing with a positive exponent, then we have at least
      // "exponent" number of digits before a decimal point followed by "scale"
      // digits.  Make sure that the number of digits to the left of the
      // decimal point (i.e. precision - scale) is >= "exponent" number of
      // digits.  Special care is taken when the float is less than 1 - in this
      // case, the 0 digit before the decimal point should not count towards
      // the precision, since it will be ignored.

      if (tempTarget[21] != '-') {
        Int32 includeFirstDigit = (tempTarget[1] == '0') ? 0 : 1;
        if ((exponent + includeFirstDigit) > (getPrecision() - getScale()))
          return -1;
      }

      if (tempTarget[21] == '-')
        exponent = -exponent;

      // Subtract 17 from the exponent.  This will set the exponent exactly
      // where the decimal point should be in the mantissa.
      exponent -= 17;

      // Shift down the mantissa so as to leave "scale" digits to the right of
      // the decimal point.
      Lng32 scaleBy = getScale() + exponent;
      if (scaleBy < 0)
	{
	  Int8 roundMe = 0;
	  for (i = 0; i < -scaleBy; i++)
	  {
	    roundMe = ( mantissa %10 ) > 4? 1:0;
	    mantissa /= 10;
	  }

	  if(roundMe == 1)
	    mantissa++;

	  scaleBy = 0;
	}

      // At this stage, our source is effectively = mantissa * 10^exponent.
      // We need to create the target Big Num by multiplying the mantissa 
      // with 10^(scale + exponent).

      // Create a Big Num (without sign) for 10^(scale + exponent);
      // This can be stored in the temporary area pointed to by tempSpacePtr_.
      char * tempPowersOfTen = (char *) tempSpacePtr_;
      if (tempSpacePtr_ == 0)
	{
	  tempPowersOfTen = new(heap) char[tempSpaceLength_];
	}
      Lng32 tempPowersOfTenLength;
      BigNumHelper::ConvPowersOfTenToBigNumHelper(scaleBy,
                                                  tempSpaceLength_, 
                                                  &tempPowersOfTenLength,
                                                  tempPowersOfTen);
      
      // Convert the mantissa into a Big Num representation (without sign).
      unsigned short * mantissaDataInShorts = (unsigned short *) &mantissa;
#ifdef NA_LITTLE_ENDIAN
      // Do nothing, mantissaData is already in the correct format.
#else
      // Reverse the four shorts in mantissaData.
      unsigned short temp = mantissaDataInShorts[0];
      mantissaDataInShorts[0] = mantissaDataInShorts[3];
      mantissaDataInShorts[3] = temp;
      temp = mantissaDataInShorts[1];
      mantissaDataInShorts[1] = mantissaDataInShorts[2];
      mantissaDataInShorts[2] = temp;
#endif

      // Multiply mantissaData with tempPowersOfTen and store in this. 
      BigNumHelper::MulHelper(getLength(), 
			      8, 
			      tempPowersOfTenLength,
			      (char *) &mantissa,
			      tempPowersOfTen,
			      op_data[0]);

      // Set sign of this.
      if ( tempTarget[0] == '-')
        // This originally set sign to 0!
        //*thisSign = 0;
        BIGN_SET_SIGN(op_data[0], getLength());

      if ((Long)tempPowersOfTen != tempSpacePtr_)
	NADELETEBASIC(tempPowersOfTen, heap);

      }
      break;

    default:
      {
        // Inform the caller that the conversion is not supported.
        return -1;
      }
      break;
    }
  
  return 0;
};

// this method implements the ROUND function on big nums
short BigNum::round (Attributes * left,
                     Attributes * right,
                     char * op_data[],
                     NAMemory *heap,
                     ComDiagsArea** diagsArea)
{
  // obtain the rounding value
  Int64 roundingValue = left->getScale();
  if (right)
    {
      Int64 rightOperandValue = 0;
      switch (right->getDatatype())
        {
          case REC_BIN8_SIGNED:
            rightOperandValue = *((char *)op_data[2]);
            break;
          case REC_BIN8_UNSIGNED:
            rightOperandValue = *((unsigned char *)op_data[2]);
            break;
          case REC_BIN16_SIGNED:
            rightOperandValue = *((Int16 *)op_data[2]);
            break;
          case REC_BPINT_UNSIGNED:
          case REC_BIN16_UNSIGNED:
            rightOperandValue = *((UInt16 *)op_data[2]);
            break;
          case REC_BIN32_SIGNED:
            rightOperandValue = *((Int32 *)op_data[2]);
            break;
          case REC_BIN32_UNSIGNED:
            rightOperandValue = *((UInt32 *)op_data[2]);
            break;
          case REC_BIN64_SIGNED:
            rightOperandValue = *((Int64 *)op_data[2]);
            break;
          case REC_BIN64_UNSIGNED:
            rightOperandValue = *((UInt64 *)op_data[2]);
            break;
          default:
            {
              // MathFunc::preCodeGen (generator/GenPreCode.cpp) should
              // have guaranteed that we have an integer type here
              if (heap)
                ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
              return -1;
            }
            break;
        }
      roundingValue -= rightOperandValue;
    }

  // Extract sign
  char leftSign = BIGN_GET_SIGN(op_data[1], left->getLength());

  // Clear sign bits
  BIGN_CLR_SIGN(op_data[1], getLength());
 
  short result = BigNumHelper::RoundHelper(left->getLength(), getLength(), op_data[1], roundingValue, op_data[0]);
  if (result < 0)
    {
      if (heap)
        ExRaiseSqlError(heap, diagsArea, EXE_NUMERIC_OVERFLOW,
                        NULL, NULL, NULL, NULL,
                        " The error occurred when rounding up a value.");
      return -1;
    }

  // Reset sign bits
  if (leftSign)
    BIGN_SET_SIGN(op_data[1], left->getLength());

  if (leftSign && (result == 0)) 
    {
      BIGN_SET_SIGN(op_data[0], getLength());
    }
  else 
    {
      BIGN_CLR_SIGN(op_data[0], getLength());
    }

  return 0;
}


void BigNum::encode(const char * inBuf, char * outBuf, short desc)
{
  char sign = BIGN_GET_SIGN(inBuf, getLength());
  NABoolean neg = (sign) ? TRUE : FALSE;

  // Clear sign bits
  BIGN_CLR_SIGN(inBuf, getLength());

  // If the platform is little endian
  //   copy in reverse the bytes of the magnitude
  // else
  //   copy in reverse the shorts of the magnitude.

#ifdef NA_LITTLE_ENDIAN
  Int32 i = 0;
  for (i = 0; i < getLength(); i++)
    outBuf[getLength() - i - 1] = (neg ? ~inBuf[i] : inBuf[i]);
#else
  Int32 i = 0;
  for (i = 0; i < getLength(); i += 2) {
    outBuf[getLength() - i - 2] = (neg ? ~inBuf[i] : inBuf[i]);
    outBuf[getLength() - i - 1] = (neg ? ~inBuf[i+1] : inBuf[i+1]);
    }
#endif

  // Reset sign
  if (neg)
    BIGN_SET_SIGN(inBuf, getLength());

  // if positive number, set the leftmost bit. Else zero it out.
  if (neg)
    outBuf[0] &= MSB_CLR_MSK;
  else
    outBuf[0] |= MSB_SET_MSK;
}

void BigNum::decode(const char * inBuf, char * outBuf, short desc)
{
  // If MSB set in key, then value is NOT negative.
  NABoolean neg = !(inBuf[0] & MSB_SET_MSK);

  // If the platform is little endian
  //   copy in reverse the bytes of the magnitude
  // else
  //   copy in reverse the shorts of the magnitude.

#ifdef NA_LITTLE_ENDIAN
  Int32 i = 0;
  for (i = 0; i < getLength(); i++)
    outBuf[getLength() - i - 1] = (neg ? ~inBuf[i] : inBuf[i]);
#else
  Int32 i = 0;
  for (i = 0; i < getLength(); i += 2) {
    outBuf[getLength() - i - 2] = (neg ? ~inBuf[i] : inBuf[i]);
    outBuf[getLength() - i - 1] = (neg ? ~inBuf[i+1] : inBuf[i+1]);
    }
#endif

  // If positive, clear out sign bit.  If it's negative, the bit would have
  // already been flipped on during the copy above.
  if (!neg)
    BIGN_CLR_SIGN(outBuf, getLength());
}

void BigNum::init(char * op_data, char * str)

{
  Lng32 strLength = str_len(str);

  // Skip ascii sign
  Int32 skip = ((str[0] == '-') || (str[0] == '+')) ? 1 : 0;

  // Convert magnitude from ASCII to Big Num without sign. 
  BigNumHelper::ConvAsciiToBigNumHelper(strLength - skip,
                                        getLength(),
                                        str + skip,
                                        op_data);

  // Set sign bit
  if (str[0] == '-')
    BIGN_SET_SIGN(op_data, getLength());
}

