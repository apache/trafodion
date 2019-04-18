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
#ifndef EXP_BIGNUM_H
#define EXP_BIGNUM_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         exp_bignum.h
 * Description:  Definition of class BigNum
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

#include "exp_attrs.h"


class BigNum : public ComplexType {

  Int32               length_;               // 00-03
  Int32               precision_;            // 04-07
  Int16               scale_;                // 08-09
  Int16               unSigned_;             // 10-11

  // Temporary space used by this class
  Int32               tempSpaceLength_;      // 12-15
  UInt32              tempSpaceOffset_ ;     // 16-19


  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char                fillers_[4];           // 20-23      

  // Temporary space starting point at runtime
  ULong              tempSpacePtr_;         // 24-31 //Put on 8-byte boundary

public:
  // some internal computation use bignum as temp storage.
  // Use 38 digit precision and 16 bytes length for them.
  enum {BIGNUM_TEMP_LEN = 16 };
  enum {BIGNUM_TEMP_PRECISION = 38 };

  BigNum(Lng32 length, Lng32 precision, short scale, short unSigned);

  BigNum();

  ~BigNum();

  void init(char * op_data, char * str);
  
  short add  (Attributes * left,
	      Attributes * right,
	      char * op_data[]);

  short sub  (Attributes * left,
	      Attributes * right,
	      char * op_data[]);

  short mul  (Attributes * left,
	      Attributes * right,
	      char * op_data[]);

  short div  (Attributes * left,
	      Attributes * right,
	      char * op_data[],
	      NAMemory *heap,
	      ComDiagsArea** diagsArea);

  short conv (Attributes * source, 
	      char * op_data[]);
 
  short comp (OperatorTypeEnum compOp,
	      Attributes * other, 
	      char * op_data[]);

  short castFrom (Attributes * source /*source*/, 
		  char * op_data[],
		  NAMemory *heap,
		  ComDiagsArea** diagsArea);

  short round (Attributes * left,
               Attributes * right,
               char * op_data[],
               NAMemory *heap,
               ComDiagsArea** diagsArea);

  // if desc <> 0, then this is a descending key.
  void encode(const char * inBuf, char * outBuf, short desc = 0);

  void decode(const char * inBuf, char * outBuf, short desc = 0);
  
  Lng32 getDisplayLength()
    {
      return precision_ + (scale_ > 0 ? 2 : 1);
    };
  
  Lng32   getPrecision(){return precision_;};


  void setLength(Int32 length)
  {length_ = length;}

  Lng32   getLength()   {return length_;};

  short  getScale()    {return scale_;};

  short  isUnsigned()  {return unSigned_;};
  
  Lng32 getStorageLength()
    {
      return length_ + (getNullFlag() ? getNullIndicatorLength() : 0);
    };

  Lng32 getDefaultValueStorageLength()
    {
      return length_ + (getNullFlag() ? ExpTupleDesc::NULL_INDICATOR_LENGTH : 0);
    };

  Attributes * newCopy();

  Attributes * newCopy(NAMemory *);
  
  void copyAttrs(Attributes * source); 

  Lng32 setTempSpaceInfo(OperatorTypeEnum operType,
			 ULong offset, Lng32 length = 0);


  void fixup(Space * space,
             char * constantsArea,
             char * tempsArea,
             char * persistentArea,
             short fixupConstsAndTemps =0,
             NABoolean spaceCompOnly = FALSE);
  
// ---------------------------------------------------------------------
// Redefinition of methods inherited from NAVersionedObject.
// ---------------------------------------------------------------------
virtual unsigned char getClassVersionID()
{
  return 1;
};

virtual void populateImageVersionIDArray()
{
  setImageVersionID(2,getClassVersionID());
  ComplexType::populateImageVersionIDArray();
};

virtual short getClassSize() { return (short) sizeof(*this); };


// ---------------------------------------------------------------------
};

short EXP_FIXED_BIGN_OV_MUL(Attributes * op1,
                        Attributes * op2,
                        char * op_data[]);

short EXP_FIXED_BIGN_OV_DIV(Attributes * op1,
                        Attributes * op2,
                        char * op_data[]);

Int64 EXP_FIXED_BIGN_OV_MOD(Attributes * op1,
                            Attributes * op2,
                            char * op_data[],
                            short * ov,
                            Int64 * quotient = NULL);

short EXP_FIXED_BIGN_OV_ADD(Attributes * op1,
                        Attributes * op2,
                        char * op_data[]);

short EXP_FIXED_BIGN_OV_SUB(Attributes * op1,
                        Attributes * op2,
                        char * op_data[]);

 

#endif




