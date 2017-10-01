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
 * File:         exp_function_substring_unicode.h
 * RCS:          $Id: 
 * Description:  The implementation of NCHAR version of SQL SUBSTRING() 
 *               function
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


ex_function_substring_doublebyte::ex_function_substring_doublebyte(
	OperatorTypeEnum oper_type,
	short num_operands,
        Attributes ** attr, Space * space)
: ex_function_clause(oper_type, num_operands, attr, space)
{
  
}

ex_function_substring_doublebyte::ex_function_substring_doublebyte()
{
}

ex_expr::exp_return_type ex_function_substring_doublebyte::eval(char *op_data[],
						 CollHeap* heap,
						 ComDiagsArea** diagsArea)
{
  Lng32 len1 = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);

  len1 /= sizeof(NAWchar); // len1 now counts in terms of number of NCHARs.
  
  // Get the starting position from operand 2.
  Int64 start = *(Lng32 *)op_data[2];  
  
  // If operand 3 exists, get the length of substring from operand 3.
  // Otherwise, length of the substring is (len1 - start + 1).
  Int64 len = 0;
  Int64 temp = 0;
  if (getNumOperands() == 4)
    {
      len = *(Lng32 *)op_data[3];
      temp = start + len;
    }
  else
    {
      if (start > (len1 + 1)) 
	temp = start;
      else
	temp = len1 + 1;
    }
  
  // Check for error conditions.
  if (temp < start)
    {
      ExRaiseSqlError(heap, diagsArea, EXE_SUBSTRING_ERROR);
      return ex_expr::EXPR_ERROR;
    }
  
  Lng32 len0 = 0;
  if ((start <= len1) && (temp > 0)) 
    {
      if (start < 1)
        start = 1;
      if (temp > (len1 + 1))
        temp = len1 + 1;
      len0 = int64ToInt32(temp - start);     
    }

  len0 *= sizeof(NAWchar);  // convert the length back into the number of octets.
  
  // Result is always a varchar, so store the length of substring 
  // in the varlen indicator.
  getOperand(0)->setVarLength(len0, op_data[-MAX_OPERANDS]);
  
  // Now, copy the substring of operand 1 from the starting position into
  // operand 0, if len0 is greater than 0.
  if (len0 > 0) 
    str_cpy_all(op_data[0], &op_data[1][sizeof(NAWchar)*(int64ToInt32(start) - 1)], len0); 
  
  return ex_expr::EXPR_OK;
}
