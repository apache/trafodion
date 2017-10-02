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
#ifndef EXP_DP2_EXPR_H
#define EXP_DP2_EXPR_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         exp_dp2_expr.h
 * Description:  expression that is not directly called by executor.
 *               Called by DP2 to encode keys, evaluate scan expressions.
 *               
 *               
 * Created:      7/28/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "exp_expr.h"
#include "ExpCriDesc.h"

// classes referenced here
class ExpDP2Expr;


typedef NAOpenObjectPtrTempl<atp_struct> AtpStructPtr;
typedef NAVersionedObjectPtrTempl<ExpDP2Expr> ExpDP2ExprPtr;


/////////////////////////////////////////////////////////////////////////////
// class ExpDP2Expr
/////////////////////////////////////////////////////////////////////////////
class ExpDP2Expr : public NAVersionedObject
{
private:

  ExExprBasePtr    expr_;                                          // 00-07
  ExCriDescPtr criDesc_;                                           // 08-15
  Int32        workAtpSpace_;                                      // 16-19
  Int16        pCodeMode_;                                         // 20-21
  char         fillersExpDP2Expr_[18];                             // 22-39

  // Pointer to space for creating the workAtp at run time. An ExpDp2Expr
  // must be allocate enough free space to create a workAtp as described
  // by its ex_cri_desc at run time.
  AtpStructPtr workAtp_;                                           // 40-47

public:

  ExpDP2Expr(ex_expr_base * expr,
             ex_cri_desc * work_cri_desc,
             Space * space,
             short allocateWorkAtpAtRunTime = 0);
  ExpDP2Expr() : NAVersionedObject(-1)                                   {}
  virtual ~ExpDP2Expr();
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }
  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }
  virtual short getClassSize()        { return (short)sizeof(ExpDP2Expr); }
  Long pack(void * space);
  Lng32 unpack(void * base, void * reallocator);
  Lng32 spaceNeededForWorkAtp(); 
  Int32 workAtpSpace(){return workAtpSpace_;};

  // if inbuf is passed in, create work atp in it.
  // if createTempTupp is TRUE, allocate tupp descriptor for
  // temp space.
  void createWorkAtp(char* &inbuf, NABoolean createTempTupp);
  inline atp_struct * getWorkAtp()                    { return workAtp_; }
  inline ex_expr_base    * getExpr()                  { return expr_; }
  ex_cri_desc * criDesc(){return criDesc_;}; 

  Int16 getPCodeMode()                     { return pCodeMode_ ; }
  inline void setPCodeMode(Int16 mode)     { pCodeMode_ = mode; }

};

class ExpDP2KeyEncodeExpr : public ExpDP2Expr
{
private:
  #define FILLERS_EXP_DP2_KE_EXPR_SIZE 24
  enum FlagsVal
  {
    // with this optimization, we move 'keyLen_' number of bytes
    // from the source row starting at offset 'firstKeyColOffset_'
    // to the key buffer.
    // This optimization is done if none of the key columns need
    // to be encoded, and the key columns are aligned next to each
    // other in the key order.
    KEY_ENCODE_OPT_1 = 0x0001
  };
  UInt32  flags_;                                             // 00-03
  UInt16 firstKeyColOffset_;                                        // 04-05
  UInt16 keyLen_;                                                   // 06-07
  char          fillersExpDP2KEExpr_[FILLERS_EXP_DP2_KE_EXPR_SIZE]; // 08-31

public:

  ExpDP2KeyEncodeExpr(ex_expr_base * expr,
		      ex_cri_desc * work_cri_desc,
		      Space * space,
		      short allocateWorkAtpAtRunTime = 0);
  ExpDP2KeyEncodeExpr() : ExpDP2Expr()                                   {}

  virtual short getClassSize()        { return (short)sizeof(ExpDP2KeyEncodeExpr); }

  UInt16 getFirstKeyColOffset() {return firstKeyColOffset_;}
  void setFirstKeyColOffset(UInt16 v) {firstKeyColOffset_ = v;}
  UInt16 getKeyLen() {return keyLen_;}
  void setKeyLen(UInt16 v) {keyLen_ = v;}

  NABoolean keyEncodeOpt1()      { return (flags_ & KEY_ENCODE_OPT_1)!= 0; }
  void setKeyEncodeOpt1(NABoolean v)
  { (v ? flags_ |= KEY_ENCODE_OPT_1 : flags_ &= ~KEY_ENCODE_OPT_1); }
    
};


#endif
