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
#ifndef EXP_MATH_FUNC_H
#define EXP_MATH_FUNC_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         <file>
 * Description:  
 *               
 *               
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "exp_clause.h"
#include "exp_clause_derived.h"


class ex_function_abs : public ex_function_clause {
public:
  ex_function_abs(OperatorTypeEnum oper_type,
			 Attributes ** attr, Space * space)
	: ex_function_clause(oper_type, 2, attr, space)
    {};
  ex_function_abs()
    {
    };

 
  ex_expr::exp_return_type eval(char *op_data[],
				CollHeap*,
				ComDiagsArea** diagsArea = 0);
  // Display
  //
  virtual void displayContents(Space * space, const char * displayStr, 
					  Int32 clauseNum, char * constsArea);

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
};

class ExFunctionBitOper : public ex_function_clause {
public:
  ExFunctionBitOper(OperatorTypeEnum oper_type,  short numOperands,
		    Attributes ** attr, Space * space)
       : ex_function_clause(oper_type, numOperands, attr, space)
  {
    setType(ex_clause::MATH_FUNCTION_TYPE);
  };

  ExFunctionBitOper()
    {
    };

 
  ex_expr::exp_return_type pCodeGenerate(Space *space, UInt32 f);

  ex_expr::exp_return_type eval(char *op_data[],
				CollHeap*,
				ComDiagsArea** diagsArea = 0);
  // Display
  //
  virtual void displayContents(Space * space, const char * displayStr, 
					  Int32 clauseNum, char * constsArea);

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
};

class ExFunctionMath : public ex_function_clause {
public:
  ExFunctionMath(OperatorTypeEnum oper_type, short numOperands, 
			 Attributes ** attr, Space * space)
	: ex_function_clause(oper_type, numOperands, attr, space)
  {
    setType(ex_clause::MATH_FUNCTION_TYPE);
  };

  ExFunctionMath()
       : ex_function_clause()
    {
    };

 
  ex_expr::exp_return_type eval(char *op_data[],
				CollHeap*,
				ComDiagsArea** diagsArea = 0);

  ex_expr::exp_return_type evalUnsupportedOperations(char *op_data[],
						     CollHeap *heap,
						     ComDiagsArea** diagsArea);

  // Display
  //
  virtual void displayContents(Space * space, const char * displayStr, 
					  Int32 clauseNum, char * constsArea);

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
};


#endif
