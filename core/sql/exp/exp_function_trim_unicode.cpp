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
 * File:         exp_function_trim_unicode.h
 * RCS:          $Id: 
 * Description:  The implementation of NCHAR/UCS2 version of SQL TRIM() function
 *               
 *               
 * Created:      7/8/98
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"


#include "exp_function.h"

ex_function_trim_doublebyte::ex_function_trim_doublebyte(){};

ex_function_trim_doublebyte::ex_function_trim_doublebyte(OperatorTypeEnum oper_type,
				   Attributes ** attr,
				   Space * space,
				   Int32 mode)
: ex_function_trim(oper_type, attr, space, mode)
{
};

ex_expr::exp_return_type ex_function_trim_doublebyte::eval(char *op_data[],
						CollHeap *heap,
						ComDiagsArea** diagsArea)
{
  // find out the length of trim character.
  Lng32 len1 = (getOperand(1)->getLength(op_data[-MAX_OPERANDS+1])) / sizeof(NAWchar);
  
  // len1 (length of trim character) must be 1. Raise an exception if greater
  // than 1.
  if (len1 != 1)
    {
      ExRaiseSqlError(heap, diagsArea, EXE_TRIM_ERROR);
      return ex_expr::EXPR_ERROR;
    }   
  
  Lng32 len2 = (getOperand(2)->getLength(op_data[-MAX_OPERANDS+2])) / sizeof(NAWchar);
  
  // Find how many leading characters in operand 2 correspond to the trim
  // character.
  Lng32 len0 = len2;
  Lng32 start = 0;   

  NAWchar trimNChar = *((NAWchar*)op_data[1]);
  NAWchar* trimSource = (NAWchar*)op_data[2];
  
  if ((getTrimMode() == 1) || (getTrimMode() == 2))
    while ((start < len2) && 
	   //(op_data[1][0] == op_data[2][start])
	   (trimNChar == trimSource[start])
	   ){
      start++;
      len0--;
    }
  
  // Find how many trailing characters in operand 2 correspond to the trim
  // character.
  Lng32 end = len2;
  if ((getTrimMode() == 0) || (getTrimMode() == 2))
    while ((end > (start)) && 
	   //(op_data[1][0] == op_data[2][end-1])
	   ( trimNChar == trimSource[end-1])
	   ){
      end--;
      len0--;
    }
  
  len0 *= sizeof(NAWchar);  // convert to the length in terms of number of bytes.
  start *= sizeof(NAWchar); // convert to the start index in terms of number of bytes.

  // Result is always a varchar.
  // store the length of trimmed string in the varlen indicator.
  getOperand(0)->setVarLength(len0, op_data[-MAX_OPERANDS]);

  
  // Now, copy operand 2 skipping the trim characters into
  // operand 0.
  
  if (len0 > 0) 
    str_cpy_all(op_data[0], &op_data[2][start], len0); 
  
  return ex_expr::EXPR_OK;
};
