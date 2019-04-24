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
 * File:         BigNumHelper.cpp
 * Description:  Implementation of class BigNumHelper
 * Created:      03/29/99
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
#include "Platform.h"
#include <math.h>
#define MathCeil(op,err) ceil(op)

#include "str.h"
#include "BigNumHelper.h"

static const unsigned short powersOfTen[] =                {10,                             // 10^1
                                                      100,                            // 10^2
                                                      1000,                           // 10^3
                                                      10000};                         // 10^4

static const unsigned short powersOfTenInBigNumForm[] =    {0x0001, 0x0000, 0x0000, 0x0000, // 10^0
                                                      0x000A, 0x0000, 0x0000, 0x0000, // 10^1
                                                      0x0064, 0x0000, 0x0000, 0x0000, // 10^2
                                                      0x03E8, 0x0000, 0x0000, 0x0000, // 10^3
                                                      0x2710, 0x0000, 0x0000, 0x0000, // 10^4
                                                      0x86A0, 0x0001, 0x0000, 0x0000, // 10^5
                                                      0x4240, 0x000F, 0x0000, 0x0000, // 10^6
                                                      0x9680, 0x0098, 0x0000, 0x0000, // 10^7
                                                      0xE100, 0x05F5, 0x0000, 0x0000, // 10^8
                                                      0xCA00, 0x3B9A, 0x0000, 0x0000, // 10^9
                                                      0xE400, 0x540B, 0x0002, 0x0000, // 10^10
                                                      0xE800, 0x4876, 0x0017, 0x0000, // 10^11
                                                      0x1000, 0xD4A5, 0x00E8, 0x0000, // 10^12
                                                      0xA000, 0x4E72, 0x0918, 0x0000, // 10^13
                                                      0x4000, 0x107A, 0x5AF3, 0x0000, // 10^14
                                                      0x8000, 0xA4C6, 0x8D7E, 0x0003, // 10^15
                                                      0x0000, 0x6FC1, 0x86F2, 0x0023, // 10^16
                                                      0x0000, 0x5D8A, 0x4578, 0x0163, // 10^17
                                                      0x0000, 0xA764, 0xB6B3, 0x0DE0, // 10^18
                                                      0x0000, 0x89E8, 0x2304, 0x8AC7};// 10^19


// The following method adds two Big Nums (without signs).

short BigNumHelper::AddHelper(Lng32 dataLength,
		              char * leftData,
		              char * rightData,
		              char * resultData)
{

  // Recast from bytes to unsigned shorts.
  Lng32 dataLengthInShorts = dataLength/2;
  unsigned short * leftDataInShorts = (unsigned short *) leftData;
  unsigned short * rightDataInShorts = (unsigned short *) rightData;
  unsigned short * resultDataInShorts = (unsigned short *) resultData;

#ifdef NA_LITTLE_ENDIAN
  union {
    ULng32 temp;
    struct {
      unsigned short remainder;
      unsigned short carry;
      } tempParts;
    };
#else
  union {
    ULng32 temp;
    struct {
      unsigned short carry;
      unsigned short remainder;
      } tempParts;
    };
#endif

  tempParts.carry = 0;
  for (Lng32 j = 0; j < dataLengthInShorts; j++)
    {
      temp = ((ULng32) leftDataInShorts[j]) + rightDataInShorts[j] + tempParts.carry;
      resultDataInShorts[j] = tempParts.remainder;
    }      
     
  return 0;

}


// The following method subtracts one Big Num from another (without signs).

short BigNumHelper::SubHelper(Lng32 dataLength,
		              char * leftData,
		              char * rightData,
		              char * resultData)
{

  // Recast from bytes to unsigned shorts.
  Lng32 dataLengthInShorts = dataLength/2;
  unsigned short * leftDataInShorts = (unsigned short *) leftData;
  unsigned short * rightDataInShorts = (unsigned short *) rightData;
  unsigned short * resultDataInShorts = (unsigned short *) resultData;

  // Check if left is smaller than right, and
  // if so, switch left with right.
  unsigned short * left = leftDataInShorts;
  unsigned short * right = rightDataInShorts;

  Int32 neg = 0;
  Lng32 j = 0;
  for (j = 0; j < dataLengthInShorts; j++) {
    if (rightDataInShorts[j] > leftDataInShorts[j]) 
		neg = -1;
    else if (rightDataInShorts[j] < leftDataInShorts[j])
		neg = 0;
  }

  if (neg == -1)
    {
      left = rightDataInShorts;
      right = leftDataInShorts;
    }
  
  short carry = 0;
  Lng32 temp;

#ifdef NA_LITTLE_ENDIAN
  union {
    ULng32 temp1;
    struct {
      unsigned short remainder;
      unsigned short filler;
      } tempParts;
    };
#else
  union {
    ULng32 temp1;
    struct {
      unsigned short filler;
      unsigned short remainder;
      } tempParts;
    };
#endif
  
  for (j = 0; j < dataLengthInShorts; j++)
    {
      temp = ((Lng32) left[j]) - right[j] + carry;  
      temp1 = temp + (Lng32) USHRT_MAX + 1; // Note that USHRT_MAX + 1 = 2^16.
      resultDataInShorts[j] = tempParts.remainder;
      carry = ( temp < 0 ? -1 : 0);
    }      
     
  return neg;

}

// The following method multiplies two Big Nums (without signs).
// The assumption is that the result is big enough to hold the
// product.

short BigNumHelper::MulHelper(Lng32 resultLength,
                              Lng32 leftLength,
		              Lng32 rightLength,
		              char * leftData,
		              char * rightData,
		              char * resultData)
{

  // Recast from bytes to unsigned shorts.
  unsigned short * leftDataInShorts = (unsigned short *) leftData;
  unsigned short * rightDataInShorts = (unsigned short *) rightData;
  unsigned short * resultDataInShorts = (unsigned short *) resultData;

  // Set result to zero.
  for (Int32 k = 0; k < resultLength/2; k++)
    resultDataInShorts[k] = 0;

  // Skip trailing zeros in the left and the right argument
  // to shorten the nested loop that appears later.
  Lng32 rightEndInShorts = rightLength/2 - 1;
  while (!rightDataInShorts[rightEndInShorts] && rightEndInShorts >= 0) 
    rightEndInShorts--;
  if (rightEndInShorts < 0) 
    return 0;

  Lng32 leftEndInShorts = leftLength/2 - 1;
  while (!leftDataInShorts[leftEndInShorts] && leftEndInShorts >= 0) 
    leftEndInShorts--;
  if (leftEndInShorts < 0) 
    return 0;

  // Int64 temp;
  // unsigned long * remainder = (unsigned long *) &temp;
  // unsigned long * carry = remainder + 1;

#ifdef NA_LITTLE_ENDIAN
  union {
    ULng32 temp;
    struct {
      unsigned short remainder;
      unsigned short carry;
      } tempParts;
    };
#else
  union {
    ULng32 temp;
    struct {
      unsigned short carry;
      unsigned short remainder;
      } tempParts;
    };
#endif

  Lng32 pos;
  for (Lng32 j = 0; j <= rightEndInShorts; j++)
    {
      tempParts.carry = 0;
      pos = j;
      for (Lng32 i = 0; i <= leftEndInShorts; i++, pos++)
	{
	  temp = ((ULng32) leftDataInShorts[i]) * rightDataInShorts[j] + resultDataInShorts[pos] + tempParts.carry;
	  resultDataInShorts[pos] = tempParts.remainder;
	}

      if (tempParts.carry) 
        resultDataInShorts[pos] = tempParts.carry;
    }

  return 0;

}

// The following method divides one Big Num by another (both without signs),
// only if the divisor fits in an unsigned short. It returns 1 if there is
// a remainder, 0 if there is no remainder, and -1 if there is an error.

short BigNumHelper::SimpleDivHelper(Lng32 dividendLength,
			            Lng32 divisorLength,
			            char * dividendData,
			            char * divisorData,
			            char * quotientData)

{
  // Recast from bytes to unsigned shorts.
  Lng32 dividendLengthInShorts = dividendLength/2;
  Lng32 divisorLengthInShorts = divisorLength/2;
  unsigned short * dividendDataInShorts = (unsigned short *) dividendData;
  unsigned short * divisorDataInShorts = (unsigned short *) divisorData;
  unsigned short * quotientDataInShorts = (unsigned short *) quotientData;

  if (divisorLengthInShorts > 1)
    return -1; // Error.

  unsigned short remainder = 0;
  union {
    ULng32 temp;
    unsigned short temp1[2];
    };

  for (Int32 j = dividendLengthInShorts - 1; j >= 0; j--)
    {
      
#ifdef NA_LITTLE_ENDIAN
      temp1[0] = dividendDataInShorts[j];
      temp1[1] = remainder;
#else
      temp1[1] = dividendDataInShorts[j];
      temp1[0] = remainder;
#endif

      quotientDataInShorts[j] = (unsigned short) (temp / divisorDataInShorts[0]);
      remainder = (unsigned short ) (temp % divisorDataInShorts[0]);
    }
  
  if (remainder != 0)
    return 1;
  else
    return 0;
}

// The following division algorithm is based on the one
// in Knuth's book on Seminumerical Algorithms, 2nd edition. 
// Some of the steps in our implementation are more optimized 
// to take advantage of base 2^16.
//
// The method divides one Big Num by another (both without signs).
// It returns 1 if there is a remainder, 0 if there is no remainder. 
// (Note: there does not seem to be a need yet to actually calculate 
// the remainder. If such a need arises in the future, the method 
// should be changed accordingly.)
//
// The caller should pass tempData, which must point to memory
// (aligned on 2-byte boundary) of size (in bytes) :
//       3 * dividendLength + 6 
// to be used for temporary calculations.

short BigNumHelper::DivHelper(Lng32 dividendLength,
			      Lng32 divisorLength,
			      char * dividendData,
			      char * divisorData,
			      char * quotientData,
                              char * tempData)
{
  // Recast from bytes to unsigned shorts.
  Lng32 dividendLengthInShorts = dividendLength/2;
  Lng32 divisorLengthInShorts = divisorLength/2;
  unsigned short * divisorDataInShorts = (unsigned short *) divisorData;
  unsigned short * quotientDataInShorts = (unsigned short *) quotientData;

  char * tempDividendData = tempData; 
  unsigned short * tempDividendDataInShorts = (unsigned short *) tempDividendData;
  unsigned short * tempDivisorDataInShorts = tempDividendDataInShorts + dividendLengthInShorts + 1;
  unsigned short * tempDivisorData1InShorts = tempDivisorDataInShorts + divisorLengthInShorts + 1;
  char * tempDivisorData = (char *) tempDivisorDataInShorts;
  char * tempDivisorData1 = (char *) tempDivisorData1InShorts;

  // Begin normalizing step.

#ifdef NA_LITTLE_ENDIAN
  
  if (divisorDataInShorts[divisorLengthInShorts - 1] <= UCHAR_MAX) { // Note that UCHAR_MAX = 0xFF

    // Multiply divisorData by 0x100 (= UCHAR_MAX + 1) and store in tempDivisorData. 
    // Note that the size of tempDivisorData is 2 bytes more than divisorData.
    tempDivisorData[0] = 0;
    Int32 i;
    for (i = 0; i < divisorLength; i++)
      tempDivisorData[i+1] = divisorData[i];
    tempDivisorData[divisorLength + 1] = 0;

    // Multiply dividendData by 0x100 and store in tempDividendData.
    // Note that the size of tempDividendData is 2 bytes more than dividendData.
    tempDividendData[0] = 0;
    for (i = 0; i < dividendLength; i++)
      tempDividendData[i+1] = dividendData[i];
    tempDividendData[dividendLength + 1] = 0;
    
  }
  else {
    Int32 i;
    // Multiply divisorData by 1 and store in tempDivisorData.
    for (i = 0; i < divisorLength; i++)
      tempDivisorData[i] = divisorData[i];
    tempDivisorDataInShorts[divisorLengthInShorts] = 0;

    // Multiply dividendData by 1 and store in tempDividendData.
    for (i = 0; i < dividendLength; i++)
      tempDividendData[i] = dividendData[i];
    tempDividendDataInShorts[dividendLengthInShorts] = 0;

  }

#else

  if (divisorDataInShorts[divisorLengthInShorts - 1] <= UCHAR_MAX) { 
    
    Int32 i = 0;

    // Multiply divisorData by 0x100 and store in tempDivisorData.
    tempDivisorData[1] = 0;
    for (i = 0; i < divisorLengthInShorts; i++) {
      tempDivisorData[2*i+3] = divisorData[2*i];
      tempDivisorData[2*i] = divisorData[2*i+1];
    }
    tempDivisorData[divisorLength] = 0;

    // Multiply dividendData by 0x100 and store in tempDividendData.
    tempDividendData[1] = 0;
    for (i = 0; i < dividendLengthInShorts; i++) {
      tempDividendData[2*i+3] = dividendData[2*i];
      tempDividendData[2*i] = dividendData[2*i+1];
    }
    tempDividendData[dividendLength] = 0;
       
  }

  else {
    
    Int32 i = 0;

    // Multiply divisorData by 1 and store in tempDivisorData.
    for (i = 0; i < divisorLength; i++)
      tempDivisorData[i] = divisorData[i];
    tempDivisorDataInShorts[divisorLengthInShorts] = 0;

    // Multiply dividendData by 1 and store in tempDividendData.
    for (i = 0; i < dividendLength; i++)
      tempDividendData[i] = dividendData[i];
    tempDividendDataInShorts[dividendLengthInShorts] = 0;

  }

#endif
  // End normalizing step.
  
  // Set the quotient to zero.
  Int32 j = 0;
  for (j = 0; j < dividendLengthInShorts ; j++)
    quotientDataInShorts[j] = 0;
  
  unsigned short q;

  union {
    ULng32 temp1;
    unsigned short temp2[2];
    };

  union {
    Int64 temp3;
    unsigned short temp4[4];
    };

  Int32 m = dividendLengthInShorts - divisorLengthInShorts;
  Lng32 dividendPosInShorts = dividendLengthInShorts;

  // The main loop for division.
  for (j = 0; j <= m; j++) {

#ifdef NA_LITTLE_ENDIAN 
    temp4[3] = 0;
    temp4[2] = tempDividendDataInShorts[dividendPosInShorts];
    temp4[1] = tempDividendDataInShorts[dividendPosInShorts - 1];
    temp4[0] = tempDividendDataInShorts[dividendPosInShorts - 2];
    temp2[1] = tempDivisorDataInShorts[divisorLengthInShorts - 1];
    temp2[0] = tempDivisorDataInShorts[divisorLengthInShorts - 2];
#else
    temp4[0] = 0;
    temp4[1] = tempDividendDataInShorts[dividendPosInShorts];
    temp4[2] = tempDividendDataInShorts[dividendPosInShorts - 1];
    temp4[3] = tempDividendDataInShorts[dividendPosInShorts - 2];
    temp2[0] = tempDivisorDataInShorts[divisorLengthInShorts - 1];
    temp2[1] = tempDivisorDataInShorts[divisorLengthInShorts - 2];
#endif

    q = (unsigned short) (temp3 / temp1);

    if (q == 0)
    {
      if (tempDividendDataInShorts[dividendLengthInShorts - j] == 
		      tempDivisorDataInShorts[divisorLengthInShorts - 1])

      q = USHRT_MAX; // Note that USHRT_MAX is the largest digit in base 2^16.
    }
          
    BigNumHelper::MulHelper(divisorLength + 2, 
                            divisorLength, 
                            2, 
                            tempDivisorData, 
                            (char *) &q, 
                            tempDivisorData1);

    Int32 neg = BigNumHelper::SubHelper(divisorLength + 2, 
	                              (char *) &tempDividendDataInShorts[dividendPosInShorts - divisorLengthInShorts], 
				      tempDivisorData1, 
				      (char *) &tempDividendDataInShorts[dividendPosInShorts - divisorLengthInShorts]);
      
    if (neg) {
	  
      // q is too large.
      q--;

      // Add back by subtracting the result of the last subtraction (which is
      // actually negative) from tempDividendData. We ignore the return code of 
      // SubHelper(), because we know the result is always positive.
      BigNumHelper::SubHelper(divisorLength + 2, 
                              tempDivisorData,
		              (char *) &tempDividendDataInShorts[dividendPosInShorts - divisorLengthInShorts], 
			      (char *) &tempDividendDataInShorts[dividendPosInShorts - divisorLengthInShorts]);
    }
    
    quotientDataInShorts[dividendPosInShorts - divisorLengthInShorts] = q;

    dividendPosInShorts--;

  } // for j

  // Check whether there was any remainder.
  for (j = 0; j < divisorLengthInShorts; j++) {
    if (tempDividendDataInShorts[j] != 0)
      return 1;
    }
  return 0;

}

// The following method rounds a Big Num (without signs).
short BigNumHelper::RoundHelper(Lng32 sourceLength,
                                Lng32 targetLength,
                                char * sourceData,
                                Int64 roundingValue,
                                char * targetData)
{
  short rc = 0;

  const int MaxPrecision = 128;  // the max of CQD MAX_NUMERIC_PRECISION_ALLOWED
  unsigned short tempBCDDataInShorts[MaxPrecision / 2];  // allow sufficient room for expansion
  rc = ConvBigNumToBcdHelper(sourceLength, sizeof(tempBCDDataInShorts), sourceData, (char *)tempBCDDataInShorts, NULL);  // TODO: need heap?
  
  if (rc == 0)
    {
      // The format for BCD is two digits per short, that is, one digit per byte.
      // The shorts are arranged big-endian, and within the shorts the digits
      // are arranged big-endian.

      // To round, we just zero out the last roundingValue digits. We also need 
      // to keep track of whether to round up or down. If the digits to be zeroed
      // are greater than or equal to 5000...000, we round up, otherwise we round
      // down.

      unsigned char * tempBCDDataInChars = (unsigned char *)tempBCDDataInShorts;
      NABoolean roundUp = FALSE;
      size_t limit = 0;
      if (roundingValue < 0)
        roundingValue = 0;  // don't go off the right end; roundingValue of 0 is a no-op

      if (MaxPrecision - 1 - roundingValue > 0) // if not off left end
        {
          limit = MaxPrecision - 1 - roundingValue; // lowest digit to retain
          if (roundingValue > 0)
            roundUp = (tempBCDDataInChars[limit+1] >= 0x05); // highest digit to zero out (if any) 
        }          

      // zero out all the rounded digits
      for (size_t i = MaxPrecision-1; i > limit; i--)
        {
          tempBCDDataInChars[i] = '\0';         
        }

      // if we need to round up, add one (with carries as needed)
      for (int j = limit; roundUp && (j >= 0); j--)
        {
          if (tempBCDDataInChars[j] >= 0x09)
            tempBCDDataInChars[j] = '\0';  // add + carry
          else
            {
              tempBCDDataInChars[j]++;
              roundUp = FALSE;
            }
        } 

      // if we overflowed when rounding up, raise an error
      if (roundUp)
        {
          return -1;  // numeric overflow
        } 

      // convert result back to BigNum
      rc = ConvBcdToBigNumHelper(sizeof(tempBCDDataInShorts),targetLength,(char *)tempBCDDataInShorts,targetData);
    }

  return rc;
}

// The following method converts a given Big Num (without sign) into
// its equivalent BCD string representation (with the more significant decimal
// digits in the lower addresses).

short BigNumHelper::ConvBigNumToBcdHelper(Lng32 sourceLength,
			                  Lng32 targetLength,
			                  char * sourceData,
			                  char * targetData,
                                          NAMemory * heap)

{
  // Recast from bytes to unsigned shorts.
  Lng32 sourceLengthInShorts = sourceLength/2;
  unsigned short * sourceDataInShorts = (unsigned short *) sourceData;

  unsigned short  tempSourceDataInShortsBuf[128 / 2];
  unsigned short* tempSourceDataInShorts = tempSourceDataInShortsBuf;

  if (heap)
    tempSourceDataInShorts = new (heap) unsigned short [sourceLength / 2];

  Int32 i = 0;
  for (i = 0; i < sourceLengthInShorts; i++)
    tempSourceDataInShorts[i] = sourceDataInShorts[i];
  
  // Initialize the BCD to zero.
  for (i = 0; i < targetLength; i++)
    targetData[i] = 0;
  char * finalTargetData = targetData + targetLength - 1;
  Lng32 finalTargetLength = 1;

  // Ignore trailing zeros in the Big Num. If all zeros, return.
  Lng32 actualSourceLengthInShorts = sourceLengthInShorts;
  while (!tempSourceDataInShorts[actualSourceLengthInShorts - 1] && actualSourceLengthInShorts > 0)
    actualSourceLengthInShorts--;
  if (!actualSourceLengthInShorts) {
    if (heap)
      NADELETEBASIC(tempSourceDataInShorts, (heap));
    return 0;
    }
 
  union {
    ULng32 temp;
    unsigned short temp1[2];
    };

  unsigned short remainder = 0;

  // Compute the final BCD as follows. Keep dividing the Big Num by 10^4
  // until the quotient becomes less than 10^4. At each iteration, compute 
  // the 4-digit BCD representation of the remainder, and append it to the 
  // left of the final BCD. For the final remainder, compute its BCD
  // representation (which may be less than 4-digits) and append it to the
  // left of the final BCD.

  finalTargetData++;
  finalTargetLength = 0;
  while ((actualSourceLengthInShorts != 1) || 
	 (tempSourceDataInShorts[actualSourceLengthInShorts - 1] >= 10000)) {
  
    // Divide the Big Num by 10^4. It is more efficient to insert
    // the division code than to call SimpleDivideHelper(); 	  
    Int32 j = 0;
    for (j = actualSourceLengthInShorts - 1; j >= 0; j--) {
      
#ifdef NA_LITTLE_ENDIAN
      temp1[0] = tempSourceDataInShorts[j];
      temp1[1] = remainder;
#else
      temp1[1] = tempSourceDataInShorts[j];
      temp1[0] = remainder;
#endif

      tempSourceDataInShorts[j] = (unsigned short) (temp / 10000);
      remainder = (unsigned short) (temp % 10000);
    }
    if (!tempSourceDataInShorts[actualSourceLengthInShorts - 1]) 
      actualSourceLengthInShorts--;
    
    // Compute the BCD representation of the remainder and append to the
    // left of the final BCD.
    for (j = 0; j < 4; j++) {
      if (finalTargetLength >= targetLength) {
        if (heap)
          NADELETEBASIC(tempSourceDataInShorts, (heap));
	return -1;
        }
      finalTargetData--;
      finalTargetLength++;
      finalTargetData[0] = remainder % 10;
      remainder = remainder / 10;
    }
  }

  // Compute the BCD representation of the final remainder and append to the
  // left of the final BCD.
  remainder = tempSourceDataInShorts[0];
  while (remainder) {
    if (finalTargetLength >= targetLength) {
      if (heap)
        NADELETEBASIC(tempSourceDataInShorts, (heap));
      return -1;
      }
    finalTargetData--;
    finalTargetLength++;
    finalTargetData[0] = remainder % 10;
    remainder = remainder / 10;
  }

  if (heap)
    NADELETEBASIC(tempSourceDataInShorts, (heap));

  return 0;
}

// The following method converts a given BCD string representation (without sign,
// and with the more significant decimal digits in the lower addresses) 
// into its equivalent Big Num representation.

short BigNumHelper::ConvBcdToBigNumHelper(Lng32 sourceLength,
			                  Lng32 targetLength,
			                  char * sourceData,
			                  char * targetData)


{
  // Recast from bytes to unsigned shorts.
  Lng32 targetLengthInShorts = targetLength/2;
  unsigned short * targetDataInShorts = (unsigned short *) targetData;
  
  // Initialize the Big Num to zero.
  Int32 i = 0;
  for (i = 0; i < targetLengthInShorts; i++)
    targetDataInShorts[i] = 0;
  Lng32 finalTargetLengthInShorts = 1;

  // Ignore leading zeros in BCD. If all zeros, return.
  Lng32 zeros = 0;
  while ((zeros < sourceLength) && !sourceData[zeros])
    zeros++;
  if (zeros == sourceLength)
    return 1; // indicate that it is all zeros

  Int32 actualSourceLength = sourceLength - zeros;
  char * actualSourceData = sourceData + zeros;

  
#ifdef NA_LITTLE_ENDIAN
  union {
    ULng32 temp;
    struct {
      unsigned short remainder;
      unsigned short carry;
      } tempParts;
    };
#else
  union {
    ULng32 temp;
    struct {
      unsigned short carry;
      unsigned short remainder;
      } tempParts;
    };
#endif

  // Compute the Big Num as follows. First, chunk up the BCD 
  // string into chunks of 4-digit unsigned short numbers. Iterate
  // over these chunks, and at each iteration, multiply the current
  // Big Num by 10^4 and then add the numeric value of this chunk
  // to it.
  for (i = 0; i < actualSourceLength; i += 4) {
    
    // Compute the numeric value of the next 4-digit chunk.
    unsigned short temp1 = 0;
    Int32 j = 0;
    while ((j < 4) && (i+j < actualSourceLength)) {
      temp1 = temp1*10 + actualSourceData[i+j];
      j++;
    }
    unsigned short power = powersOfTen[j - 1];

    // Multiply the previous Big Num by 10^4 and add the value
    // of the current chunk to it. It is more efficient to insert the
    // multiplication and addition code here than to call the Helper() 
    // methods. 
    temp = ((ULng32) targetDataInShorts[0]) * power + temp1;
    targetDataInShorts[0] = tempParts.remainder;
    for (j = 1; j < finalTargetLengthInShorts; j++) {
      temp = ((ULng32) targetDataInShorts[j]) * power + tempParts.carry;
      targetDataInShorts[j] = tempParts.remainder;
    }	
    if (tempParts.carry) {
      if (finalTargetLengthInShorts >= targetLengthInShorts)
	return -1;
      finalTargetLengthInShorts++; 
      targetDataInShorts[finalTargetLengthInShorts - 1] = tempParts.carry;
    }
  }

  return 0;

}

// The following method converts a given Big Num (with sign) into
// its equivalent BCD string representation (with the more significant
// decimal digits in the lower addresses).

short BigNumHelper::ConvBigNumWithSignToBcdHelper(Lng32 sourceLength,
			                          Lng32 targetLength,
			                          char * sourceData,
			                          char * targetData,
                                                  NAMemory * heap)
{
  char sign = BIGN_GET_SIGN(sourceData, sourceLength);

  // Set up sign.
  targetData[0] = (sign) ? '-' : '+';

  // Temporarily clear sign
  BIGN_CLR_SIGN(sourceData, sourceLength);

  short returnValue = BigNumHelper::ConvBigNumToBcdHelper(sourceLength,
                                                          targetLength - 1,
                                                          sourceData,
                                                          targetData + 1,
                                                          heap);

  // Restore sign
  if (sign)
    BIGN_SET_SIGN(sourceData, sourceLength);

  return returnValue;
}

// The following method converts a given BCD string representation 
// (with sign, and with the more significant decimal digits in the lower
// addresses) into its equivalent Big Num representation.

short BigNumHelper::ConvBcdToBigNumWithSignHelper(Lng32 sourceLength,
			                          Lng32 targetLength,
			                          char * sourceData,
			                          char * targetData)
{
  short returnValue = BigNumHelper::ConvBcdToBigNumHelper(sourceLength - 1,
                                                          targetLength,
                                                          sourceData + 1,
                                                          targetData);
  if (returnValue == 1)  // don't care if all zeros
    returnValue = 0;

  // Set up sign after magnitude set.  TargetData already cleared.
  if (sourceData[0] == '-')
    BIGN_SET_SIGN(targetData, targetLength);

  return returnValue;
}


// The following method converts a given Big Num (without sign) into
// its equivalent ASCII string representation (with the more significant
// decimal digits in the lower addresses).

short BigNumHelper::ConvBigNumToAsciiHelper(Lng32 sourceLength,
			                    Lng32 targetLength,
			                    char * sourceData,
			                    char * targetData,
                                            NAMemory * heap)
{
  
  // Convert to BCD representation (without sign).
  short returnValue = BigNumHelper::ConvBigNumToBcdHelper(sourceLength,
                                                          targetLength,
                                                          sourceData,
                                                          targetData,
                                                          heap);
  // Convert from BCD to ASCII.
  for (Int32 i = 0; i < targetLength; i++)
    targetData[i] += '0';

  return returnValue;

}

// The following method converts a given ASCII string representation 
// (without sign, and with the more significant decimal digits in the lower
// addresses) into its equivalent Big Num representation.

short BigNumHelper::ConvAsciiToBigNumHelper(Lng32 sourceLength,
			                    Lng32 targetLength,
			                    char * sourceData,
			                    char * targetData)
{
  // Temporarily convert source from ASCII to BCD.
  Int32 i = 0;
  for (i = 0; i < sourceLength; i++)
    sourceData[i] -= '0';

  short returnValue =  BigNumHelper::ConvBcdToBigNumHelper(sourceLength,
                                                           targetLength,
                                                           sourceData,
                                                           targetData);
  if (returnValue == 1)  // don't care if all zeros
    returnValue = 0;

  // Restore source to ASCII.
  for (i = 0; i < sourceLength; i++)
    sourceData[i] += '0';

  return returnValue;

}

// The following method converts a given Big Num (with sign) into
// its equivalent ASCII string representation (with the more significant
// decimal digits in the lower addresses).

short BigNumHelper::ConvBigNumWithSignToAsciiHelper(Lng32 sourceLength,
			                            Lng32 targetLength,
			                            char * sourceData,
			                            char * targetData,
                                                    NAMemory * heap)
{
  char sign = BIGN_GET_SIGN(sourceData, sourceLength);

  // Set up sign.
  targetData[0] = (sign) ? '-' : '+';

  // Temporarily clear sign
  BIGN_CLR_SIGN(sourceData, sourceLength);

  short returnValue = BigNumHelper::ConvBigNumToAsciiHelper(sourceLength,
                                                            targetLength - 1,
                                                            sourceData,
                                                            targetData + 1,
                                                            heap);

  // Restore sign
  if (sign)
    BIGN_SET_SIGN(sourceData, sourceLength);

  return returnValue;
}

// The following method converts a given ASCII string representation 
// (with sign, and with the more significant decimal digits in the lower
// addresses) into its equivalent Big Num representation.

short BigNumHelper::ConvAsciiToBigNumWithSignHelper(Lng32 sourceLength,
			                            Lng32 targetLength,
			                            char * sourceData,
			                            char * targetData)
{
  short returnValue = BigNumHelper::ConvAsciiToBigNumHelper(sourceLength - 1,
                                                            targetLength,
                                                            sourceData + 1,
                                                            targetData);

  // Set up sign.
  if (sourceData[0] == '-')
    BIGN_SET_SIGN(targetData, targetLength);

  return returnValue;
}

// Given a desired precision of a Big Num, the following method calculates 
// the required storage length (including the sign). We assume that the
// precision is > 0.

Lng32 BigNumHelper::ConvPrecisionToStorageLengthHelper(Lng32 precision)
{
  // The storage length is always a multiple of 2 bytes, and at minimum, 8 bytes
  // long.  1 bit must be available for the sign. Thus we use the formula:
  //
  //     storage length = ceil((precision*log2(10) + 1)/16)
  //
  short err = 0;

  // Change precision to be a minimum of 18 (i.e. 8 bytes)
  if (precision < 18)
    precision = 18;

  Lng32 stLen = (2*(Lng32)(MathCeil((precision*0.208) + 0.062, err)));

  if (err)
    return -1; // is -1 error return code from here???

  return stLen;
}


// The following method converts an integer, 10^exponent, to a Big Num 
// representation (without sign). The given exponent should be >= 0.  The
// given targetData should have a length which is a multiple of 8.

short BigNumHelper::ConvPowersOfTenToBigNumHelper(Lng32 exponent,
                                                  Lng32 targetLength,
                                                  Lng32 * finalTargetLength,
                                                  char * targetData)

{
  // If exponent is small enough, copy Big Num from precomputed table.
  if (exponent < sizeof(powersOfTenInBigNumForm)/8) {
    for (Int32 k = 0; k < 8 ; k++)
      targetData[k] = ((char *) (powersOfTenInBigNumForm + exponent*4))[k];
    *finalTargetLength = 8;
    return 0;
    }

  // Otherwise, we have to actually compute 10^exponent iteratively.
  // Recast from bytes to unsigned shorts. 
  unsigned short * targetDataInShorts = (unsigned short *) targetData;
  *finalTargetLength = BigNumHelper::ConvPrecisionToStorageLengthHelper(exponent); 

  Lng32 diffExponent = exponent - sizeof(powersOfTenInBigNumForm)/8 + 1;

#ifdef NA_LITTLE_ENDIAN
  union {
    ULng32 temp;
    struct {
      unsigned short remainder;
      unsigned short carry;
      } tempParts;
    };
#else
  union {
    ULng32 temp;
    struct {
      unsigned short carry;
      unsigned short remainder;
      } tempParts;
    };
#endif
  
  // Initialize the Big Num to 10^(sizeof(powersOfTenInBigNumForm)/8 - 1)
  Int32 k = 0;
  for (k = 0; k < 8; k++)
    targetData[k] = ((char *) (powersOfTenInBigNumForm + sizeof(powersOfTenInBigNumForm)/2 - 4))[k];
  Lng32 currentTargetLengthInShorts = 4;

  // Multiply the Big Num repeatedly by 10^4 (excepting the last 
  // multiplication, which is by 10^(diffExponent%4). It is more
  // efficient to insert the multiplication code here than to
  // call the mulHelper() method.

  for (Int32 i = 0; i < diffExponent; i += 4) {
    
    unsigned short power;
    if (i + 4 <= diffExponent)
      power = 10000;
    else
      power = powersOfTen[(diffExponent % 4) - 1];

    temp = ((ULng32) targetDataInShorts[0]) * power;
    targetDataInShorts[0] = tempParts.remainder;
    Int32 j = 1;
    for (j = 1; j < currentTargetLengthInShorts; j++) {
      temp = ((ULng32) targetDataInShorts[j]) * power + tempParts.carry;
      targetDataInShorts[j] = tempParts.remainder;
      }	
    if (tempParts.carry) {
      currentTargetLengthInShorts++; 
      targetDataInShorts[currentTargetLengthInShorts - 1] = tempParts.carry;
      }
   
    }

  // Fill up the trailing part of the target with zeros.
  for (k = 2*currentTargetLengthInShorts; k < *finalTargetLength; k++)
    targetData[k] = 0;
  
  return 0;

}

// The following converts an Int64 to a Big Num (with sign).

short BigNumHelper::ConvInt64ToBigNumWithSignHelper(Int32 targetLength,
                                                    Int64 sourceData,
                                                    char * targetData,
                                                    NABoolean isUnsigned)

{
  Int32 tgtLength16 = targetLength >> 1; 
  UInt16* tgt = (UInt16*)targetData;
  Int64* tgt64 = (Int64*)targetData;
  NABoolean isNeg = FALSE;


  // Initialize magnitude of target to zero.
  for (Int32 k = 0; k < tgtLength16; k++)
    tgt[k] = 0;

  if ((NOT isUnsigned) && (sourceData < 0)) {
    sourceData = -sourceData;
    isNeg = TRUE;
  }

  *tgt64 = sourceData;

  // If little endian, do nothing, target is already in correct form.
#ifndef NA_LITTLE_ENDIAN
  UInt16 temp1 = tgt[0];
  UInt16 temp2 = tgt[1];
  tgt[0] = tgt[3];
  tgt[1] = tgt[2];
  tgt[2] = temp2;
  tgt[3] = temp1;
#endif

  if (isNeg)
    BIGN_SET_SIGN(tgt, targetLength);

  return 0;

}

// The following converts a Big Num (with sign) into an Int64.

short BigNumHelper::ConvBigNumWithSignToInt64Helper(Lng32 sourceLength,
                                                    char * sourceData,
                                                    void * targetDataPtr,
                                                    NABoolean isUnsigned)

{
  UInt64 *uTargetData = (UInt64*)targetDataPtr;
  Int64  *targetData  = (Int64*)targetDataPtr;

  // Recast from bytes to unsigned shorts.
  unsigned short * sourceDataInShorts = (unsigned short *) sourceData;
  Lng32 sourceLengthInShorts = sourceLength/2;
  char srcSign = BIGN_GET_SIGN(sourceData, sourceLength);

  // Clear source sign temporarily
  BIGN_CLR_SIGN(sourceData, sourceLength);

  // Remove trailing zeros in source. Note that we want a length of 4 unsigned shorts.
  while (sourceDataInShorts[sourceLengthInShorts - 1] == 0 && sourceLengthInShorts > 4)
    sourceLengthInShorts--;

  // Check for overflow. Source cannot have more than 4 unsigned shorts. 
  if (sourceLengthInShorts > 4) {
    // Restore sign in source
    if (srcSign)
      BIGN_SET_SIGN(sourceData, sourceLength);
    return -1;
  }

  // Copy the magnitude of source to target.
  // TODO: unaligned store here
  *targetData = *((Int64 *) sourceData);

  // Restore sign in source
  if (srcSign)
    BIGN_SET_SIGN(sourceData, sourceLength);

#ifdef NA_LITTLE_ENDIAN
    // Do nothing, target already in correct format.
#else
    // Reverse the shorts in the target.
    unsigned short * targetDataInShorts = (unsigned short *) targetData;

    unsigned short temp = targetDataInShorts[0];
    targetDataInShorts[0] = targetDataInShorts[3];
    targetDataInShorts[3] = temp;
    temp = targetDataInShorts[1];
    targetDataInShorts[1] = targetDataInShorts[2];
    targetDataInShorts[2] = temp;
#endif 

  // We now check the sign of the source. Unfortunately there are complications. 
  // The target, an Int64, ranges between 2^63-1 and -2^63, which is asymmetric. 
  // We have to make sure that the source did not contain a Big Num outside this range.

  if (srcSign == 0) { // source is positive.
    if (isUnsigned)
      return 0; // all values are ok

    else if (*targetData >= 0) // target magnitude is between 0 and 2^63-1.
      return 0;            
    else  // target magnitude is beyond 2^63-1.
      return -1;
    }
  else {                    // source is negative.
    if (isUnsigned)
      return -1; // error
    else if (*targetData >= 0) { // target magnitude is between 0 and 2^63-1.
      *targetData = -*targetData;
      return 0;
    }
    else if (*targetData == LLONG_MIN) // target magnitude is 2^63. We do not even 
                                       // have to negate the target, because the bit
                                       // representations of 2^63 and -2^63 (the latter
                                       // in 2's-complement) are the same.
      return 0;
    else  // target magnitude is beyond 2^63
      return -1;
    }

}


// The following converts a BIGNUM (with sign) into Int64 and scale it, 
// returning information about the conversion to the caller (e.g. 
// truncations, overflows). 
//
// The return value can have 5 possible values:
//   0: the conversion was ok
//   1: the result was rounded up to LLONG_MIN
//   2: the result was rounded down to LLONG_MAX
//   3: the result was rounded up
//   4: the result was rounded down

short BigNumHelper::ConvBigNumWithSignToInt64AndScaleHelper(Lng32 sourceLength,
                                                            char * sourceData,
                                                            Int64 * targetData,
                                                            Lng32 exponent,
                                                            NAMemory * heap)
{

  Lng32 sourceLengthInShorts = sourceLength/2;
  unsigned short * targetDataInShorts = (unsigned short *) targetData;
  
  Lng32 absExponent = (exponent >= 0 ? exponent : -exponent);

  char srcSign = BIGN_GET_SIGN(sourceData, sourceLength);

  // Clear source sign temporarily
  BIGN_CLR_SIGN(sourceData, sourceLength);

  // Allocate temp space for various calculations:

  // Allocate space for the scale factor, which is a Big Num (without sign) with value 10^(|exponent|).
  Lng32 crudeScaleFactorLength = BigNumHelper::ConvPrecisionToStorageLengthHelper(absExponent + 1);
  unsigned short * scaleFactorInShorts = new (heap) unsigned short [crudeScaleFactorLength/2];
  char * scaleFactor = (char *) scaleFactorInShorts;

  // Allocate space for temporary Big Num (without sign) to store source after scaling.
  unsigned short * sourceDataAfterScalingInShorts = new (heap) unsigned short [(crudeScaleFactorLength + sourceLength)/2];
  char * sourceDataAfterScaling = (char *) sourceDataAfterScalingInShorts;
  Lng32 sourceLengthAfterScalingInShorts;

  // Allocate temp space for the division routine; see DivHelper for details.
  unsigned short * tempDataInShorts = new (heap) unsigned short [3 * (sourceLength) / 2 + 3];
  char * tempData = (char *) tempDataInShorts;

  Lng32 scaleFactorLength;
  BigNumHelper::ConvPowersOfTenToBigNumHelper(absExponent,
                                              crudeScaleFactorLength,
                                              &scaleFactorLength,
                                              scaleFactor);

  // Since the above function returns a Big Num with length a multiple of 8, there may be
  // trailing zeros. Adjust the length to ignore trailing zeros.
  Lng32 scaleFactorLengthInShorts = scaleFactorLength/2;
  while (scaleFactorInShorts[scaleFactorLengthInShorts - 1] == 0 && scaleFactorLengthInShorts > 1)
    scaleFactorLengthInShorts--;
  scaleFactorLength = 2*scaleFactorLengthInShorts;

  short anyTruncation = 0;
                                           
  if (exponent < 0) { 
    // If exponent is negative, we need to scale up. Thus divide source by scaleFactor. 
    // If there is a remainder, this signifies that some truncation occurred.

    if (scaleFactorLengthInShorts > sourceLengthInShorts) { 
      // I.e. the scaleFactor is longer than the magnitude of the source,
      // thus everything got truncated! Set source after scaling to zero.
      for (Int32 k = 0; k < (crudeScaleFactorLength + sourceLength); k++)
        sourceDataAfterScaling[k] = 0;
      sourceLengthAfterScalingInShorts = 4; 
      anyTruncation = 1;
      }
    else if (scaleFactorLengthInShorts == 1) {
      anyTruncation = BigNumHelper::SimpleDivHelper(sourceLength,
                                                    2,
                                                    sourceData,
                                                    scaleFactor,
                                                    sourceDataAfterScaling);
      sourceLengthAfterScalingInShorts = sourceLengthInShorts;
      }
    else {
      anyTruncation = BigNumHelper::DivHelper(sourceLength,
                                              scaleFactorLength,
                                              sourceData,
                                              scaleFactor,
                                              sourceDataAfterScaling,
                                              tempData);
      sourceLengthAfterScalingInShorts = sourceLengthInShorts;
      }
    } 

  else {
    // If exponent is positive, we need to scale down, so multiply source by scaleFactor. 
    BigNumHelper::MulHelper(sourceLength + scaleFactorLength, 
                            sourceLength,  // ignore the sign byte
                            scaleFactorLength,
                            sourceData,
                            scaleFactor,
                            sourceDataAfterScaling);
    sourceLengthAfterScalingInShorts = sourceLengthInShorts + scaleFactorLengthInShorts;

    }

  // Adjust the length of source after scaling to ignore trailing zeros (we do not have to
  // go below a length of 4).
  while (sourceDataAfterScalingInShorts[sourceLengthAfterScalingInShorts - 1] == 0 
         && sourceLengthAfterScalingInShorts > 4)
    sourceLengthAfterScalingInShorts--;

  
  short retValue = 0;

  // Check for overflow. After scaling, the magnitude of source cannot have more 
  // than 4 unsigned shorts. 
  if (sourceLengthAfterScalingInShorts > 4) {
    if (srcSign == 0) { // source is positive.
      *targetData = LLONG_MAX;
      retValue = 2; // the result was rounded down to LLONG_MAX.
      }
    else {
      *targetData = LLONG_MIN;
      retValue = 1; // the result was rounded up to LLONG_MIN.
      }
    }

  else {

    // Copy the magnitude of source to target.
    *targetData = *((Int64 *) sourceDataAfterScaling);

#ifdef NA_LITTLE_ENDIAN
    // Do nothing, target already in correct format.
#else
    // Reverse the shorts in the target.
    unsigned short temp = targetDataInShorts[0];
    targetDataInShorts[0] = targetDataInShorts[3];
    targetDataInShorts[3] = temp;
    temp = targetDataInShorts[1];
    targetDataInShorts[1] = targetDataInShorts[2];
    targetDataInShorts[2] = temp;
#endif 

    // The target, an Int64, should range between 2^63-1 and -2^63, which is asymmetric. 
    // We have to make sure that the source did not contain a Big Num outside this range.

    if (srcSign == 0) { // source is positive.

      if (*targetData < 0) { // target magnitude is beyond 2^63-1.
        *targetData = LLONG_MAX;
        retValue = 2; // the result was rounded down to LLONG_MAX.
        }

      }
    else {                    // source is negative.

      if (*targetData < 0 && *targetData != LLONG_MIN) { // target magnitude is beyond 2^63
        *targetData = LLONG_MIN;
        retValue = 1; // the result was rounded up to LLONG_MIN.
        }
      else if (*targetData != LLONG_MIN) { 
	// target magnitude is between 0 and 2^63.
	// The value LLONG_MIN is already in correct format and doesn't
	// need to be negated.
        *targetData = -*targetData;
      }
    }
    
    if (retValue == 0 && anyTruncation) {
      if (srcSign == 0) 
        retValue = 4; // the result was rounded down.
      else
        retValue = 3; // the result was rounded up;
      }

    }

  NADELETEBASIC(scaleFactorInShorts, (heap));
  NADELETEBASIC(sourceDataAfterScalingInShorts, (heap));
  NADELETEBASIC(tempDataInShorts, (heap));

  // Restore sign in source
  if (srcSign)
    BIGN_SET_SIGN(sourceData, sourceLength);

  return retValue;

}

// The following converts a Big Num to a Big Num.

short BigNumHelper::ConvBigNumWithSignToBigNumWithSignHelper(Lng32 sourceLength,
                                                             Lng32 targetLength,
                                                             char * sourceData,
                                                             char * targetData)
{
  // Recast char strings as arrays of unsigned shorts
  unsigned short * sourceDataInShorts = (unsigned short *) sourceData;
  unsigned short * targetDataInShorts = (unsigned short *) targetData;
  Lng32 sourceLengthInShorts = sourceLength/2; // length in number of shorts.

  char srcSign = BIGN_GET_SIGN(sourceData, sourceLength);

  // Clear sign bit
  BIGN_CLR_SIGN(sourceData, sourceLength);
 
  // Ignore trailing zeros in source. Minimum number of shorts in source is 2.
  while ((sourceDataInShorts[sourceLengthInShorts - 1] == 0) && (sourceLengthInShorts > 2))
    sourceLengthInShorts--;

  // Return error if overflow
  if (sourceLengthInShorts > targetLength/2) {
    // Reset sign bit of source
    if (srcSign)
      BIGN_SET_SIGN(sourceData, sourceLength);
    return -1;
  }

  // Initialize magnitude of target to zeros.
  Int32 k = 0;
  for (k = 0; k < targetLength; k++)
    targetData[k] = 0;

  // Copy source to target.
  for (k = 0; k < sourceLengthInShorts; k++)
    targetDataInShorts[k] = sourceDataInShorts[k];

  // Restore sign in source and set sign in target
  if (srcSign) {
    BIGN_SET_SIGN(sourceData, sourceLength);
    BIGN_SET_SIGN(targetData, targetLength);
  }

  return 0;
}



