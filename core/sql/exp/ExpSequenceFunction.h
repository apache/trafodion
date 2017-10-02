#ifndef EXP_SEQUENCE_FUNCTION_H
#define EXP_SEQUENCE_FUNCTION_H
//
// Only include this file once

/* -*-C++-*-
******************************************************************************
*
* File:         ExSequenceFunction.h
* RCS:          $Id
* Description:  ExSequenceFunction class Declaration
* Created:      9/11/98
* Modified:     $Author
* Language:     C++
* Status:       $State
*
*
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
*
*
******************************************************************************
*/

// Includes
//
#include "exp_clause.h"
#include "exp_clause_derived.h"

// Internal forward declarations
//
class ExpSequenceFunction;
class ExpSequenceExpression;

// ExpSequenceFunction
//

// ExpSequenceFunction declaration
//
class ExpSequenceFunction : public ex_function_clause {
public:
  enum { SF_NONE, SF_OFFSET };

  // Construction - this is the "real" constructor
  //
  ExpSequenceFunction(OperatorTypeEnum oper_type,
				 Int32 arity,
				 Int32 index,
				 Attributes ** attr,
				 Space * space);

  // This constructor is used only to get at the virtual function table.
  //
  ExpSequenceFunction();


  // isNullInNullOut - Must redefine this virtual function to return 0
  // since a NULL input does not simply produce a NULL output.
  //
  Int32 isNullInNullOut() const { return 0; };

  // isNullRelevant - Must redefine this virtual function to return 0
  // since all the work is done in eval and none in processNulls.
  //
  Int32 isNullRelevant() const { return 0; };

  ex_expr::exp_return_type pCodeGenerate(Space *space, UInt32 f);

  // eval - Must redefine eval to do the effective NOP sequence function.
  //
  ex_expr::exp_return_type eval(char *op_data[], CollHeap*, 
					   ComDiagsArea** =0);

  inline NABoolean nullRowIsZero(void)
  {
    return ((flags_ & SF_NULL_ROW_IS_ZERO) != 0);
  };

  inline void setNullRowIsZero(NABoolean v)
  {
    (v)? flags_ |= SF_NULL_ROW_IS_ZERO: flags_ &= ~SF_NULL_ROW_IS_ZERO;
  };

  inline NABoolean isOLAP(void)
  {
    return ((flags_ & SF_IS_OLAP) != 0);
  };

  inline void setIsOLAP(NABoolean v)
  {
    (v)? flags_ |= SF_IS_OLAP: flags_ &= ~SF_IS_OLAP;
  };

  inline NABoolean isLeading(void)
  {
    return ((flags_ & SF_IS_LEADING) != 0);
  };

  inline void setIsLeading(NABoolean v)
  {
    (v)? flags_ |= SF_IS_LEADING: flags_ &= ~SF_IS_LEADING;
  };

  inline Int32 winSize(void)
  {
    return winSize_;
  };

  inline void setWinSize(Int32 winSize)
  {
    winSize_ = winSize;
  };
  

  // pack - Must redefine pack.
  Long pack(void *);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(2,getClassVersionID());
    ex_function_clause::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------

  enum
  {
    SF_NULL_ROW_IS_ZERO = 0x0001,
    SF_IS_OLAP          = 0x0002,
    SF_IS_LEADING       = 0x0004
  };

  void *GetRowData_;
  char *(*GetRow_)(void*,Int32,NABoolean,Lng32,Int32&);

  char *(*PutRow_)(void*,Int32,NABoolean,Lng32,Int32&);

  Int32            offsetIndex_; // 00-03
  Int32            flags_;       // 04-07
  Int32            winSize_;     // 08-11
  char             filler_[4];   // 12-15
  
};

// ExpSequenceExpression
//
class ExpSequenceExpression : public ex_expr {
public:
  inline void seqFixup(void *data, 
                                  char *(*getRow)(void*,Int32,NABoolean,Lng32,Int32&),
                                  char *(*getRowOLAP)(void*,Int32,NABoolean,Lng32,Int32&))
  {
    ex_clause *clause = getClauses();
    while(clause)
      {
	if(clause->getOperType() == ITM_OFFSET)
	  {
	    ExpSequenceFunction *seqClause = (ExpSequenceFunction*)clause;
	    seqClause->GetRowData_ = data;
	    seqClause->GetRow_ = (seqClause->isOLAP() ? getRowOLAP : getRow);
	  }
	clause = clause->getNextClause();
      }

    PCodeBinary *pCode = getPCodeBinary();
    if (!pCode)
      return;

    Int32 length = *(pCode++);
    pCode += (2 * length);

    while (pCode[0] != PCIT::END) {
      if (pCode[0] == PCIT::OFFSET_IPTR_IPTR_IBIN32S_MBIN64S_MBIN64S ||
          pCode[0] == PCIT::OFFSET_IPTR_IPTR_MBIN32S_MBIN64S_MBIN64S) {
          Int32 idx = 1;
          if (GetPCodeBinaryAsPtr(pCode, idx)) {  //if (pCode[1])
            idx += setPtrAsPCodeBinary(pCode, idx, (Long) getRowOLAP);
            // pCode[1] = (PCodeBinary) getRowOLAP;
          } else {
            idx += setPtrAsPCodeBinary(pCode, idx, (Long) getRow);
            // pCode[1] = (PCodeBinary) getRow;
          }
        idx = setPtrAsPCodeBinary(pCode, idx, (Long) data);  
        // pCode[2] = (PCodeBinary) data;  
      }
      
      pCode += PCode::getInstructionLength(pCode);
    }
  }

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ex_expr::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------

};


#endif

