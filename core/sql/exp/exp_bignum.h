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

#pragma warning ( disable : 4251 )

class SQLEXP_LIB_FUNC  BigNum : public ComplexType {

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
#ifdef NA_64BIT
  ULong              tempSpacePtr_;         // 24-31 //Put on 8-byte boundary
#else
  Int32               tempSpacePtr_;         // 24-27
  char                fillers_b[4];           // 28-31      
#endif

public:
NA_EIDPROC
  BigNum(Lng32 length, Lng32 precision, short scale, short unSigned);

NA_EIDPROC
  BigNum();

NA_EIDPROC
  ~BigNum();

NA_EIDPROC
  void init(char * op_data, char * str);
  
NA_EIDPROC
  short add  (Attributes * left,
	      Attributes * right,
	      char * op_data[]);

NA_EIDPROC
  short sub  (Attributes * left,
	      Attributes * right,
	      char * op_data[]);

NA_EIDPROC
  short mul  (Attributes * left,
	      Attributes * right,
	      char * op_data[]);

NA_EIDPROC
  short div  (Attributes * left,
	      Attributes * right,
	      char * op_data[],
	      NAMemory *heap,
	      ComDiagsArea** diagsArea);

NA_EIDPROC
  short conv (Attributes * source, 
	      char * op_data[]);
 
NA_EIDPROC
  short comp (OperatorTypeEnum compOp,
	      Attributes * other, 
	      char * op_data[]);

NA_EIDPROC
  short castFrom (Attributes * source /*source*/, 
		  char * op_data[],
		  NAMemory *heap,
		  ComDiagsArea** diagsArea);

  // if desc <> 0, then this is a descending key.
NA_EIDPROC
  void encode(const char * inBuf, char * outBuf, short desc = 0);

NA_EIDPROC
  void decode(const char * inBuf, char * outBuf, short desc = 0);
  
NA_EIDPROC
  Lng32 getDisplayLength()
    {
      return precision_ + (scale_ > 0 ? 2 : 1);
    };
  
NA_EIDPROC
  Lng32   getPrecision(){return precision_;};

NA_EIDPROC 
  void setLength(Int32 length)
  {length_ = length;}

NA_EIDPROC
  Lng32   getLength()   {return length_;};

NA_EIDPROC
  short  getScale()    {return scale_;};

NA_EIDPROC
  short  isUnsigned()  {return unSigned_;};
  
NA_EIDPROC
  Lng32 getStorageLength()
    {
      return length_ + (getNullFlag() ? getNullIndicatorLength() : 0);
    };

NA_EIDPROC
  Lng32 getDefaultValueStorageLength()
    {
      return length_ + (getNullFlag() ? ExpTupleDesc::NULL_INDICATOR_LENGTH : 0);
    };

NA_EIDPROC
  Attributes * newCopy();

NA_EIDPROC
  Attributes * newCopy(NAMemory *);
  
NA_EIDPROC
  void copyAttrs(Attributes * source); 

NA_EIDPROC
  Lng32 setTempSpaceInfo(OperatorTypeEnum operType,
			 ULong offset, Lng32 length = 0);

NA_EIDPROC 
  void fixup(Space * space,
             char * constantsArea,
             char * tempsArea,
             char * persistentArea,
             short fixupConstsAndTemps =0,
             NABoolean spaceCompOnly = FALSE);
  
// ---------------------------------------------------------------------
// Redefinition of methods inherited from NAVersionedObject.
// ---------------------------------------------------------------------
NA_EIDPROC virtual unsigned char getClassVersionID()
{
  return 1;
};

NA_EIDPROC virtual void populateImageVersionIDArray()
{
  setImageVersionID(2,getClassVersionID());
  ComplexType::populateImageVersionIDArray();
};

NA_EIDPROC virtual short getClassSize() { return (short) sizeof(*this); };


// ---------------------------------------------------------------------
};

NA_EIDPROC
SQLEXP_LIB_FUNC
short EXP_FIXED_BIGN_OV_MUL(Attributes * op1,
                        Attributes * op2,
                        char * op_data[]);

NA_EIDPROC
SQLEXP_LIB_FUNC
short EXP_FIXED_BIGN_OV_DIV(Attributes * op1,
                        Attributes * op2,
                        char * op_data[]);

NA_EIDPROC
SQLEXP_LIB_FUNC
Int64 EXP_FIXED_BIGN_OV_MOD(Attributes * op1,
                        Attributes * op2,
                        char * op_data[],
                        short * ov);


 
#pragma warning ( default : 4251 )

#endif




