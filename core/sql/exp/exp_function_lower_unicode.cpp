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
 * File:         exp_function_lower_unicode.h
 * RCS:          $Id: 
 * Description:  The implementation of NCHAR version of SQL LOWER() function
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
#include "unicode_char_set.h"

ex_function_lower_unicode::ex_function_lower_unicode(OperatorTypeEnum oper_type,
				     Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 2, attr, space)
{
  
}

ex_function_lower_unicode::ex_function_lower_unicode()
{
}

extern NAWchar unicodeToLower(NAWchar wc);


ex_expr::exp_return_type ex_function_lower_unicode::eval(char *op_data[],
						 CollHeap*,
						 ComDiagsArea**)
{
  Lng32 len1 = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);
  
  getOperand(0)->setVarLength(len1, op_data[-MAX_OPERANDS]);
  
  // Now, copy the contents of operand 1 after the case change into
  // operand 0.

  NAWchar* target = (NAWchar*)op_data[0];
  NAWchar* source = (NAWchar*)op_data[1];
  Int32 wc_len = len1/sizeof(NAWchar);

  for (Int32 i = 0; i < wc_len; i++)
    {
      //op_data[0][i] =  TOLOWER(op_data[1][i]); 
      //op_data[0][i] = '1';
      
     target[i] = unicode_char_set::to_lower(source[i]);
    }         
  
  return ex_expr::EXPR_OK;
};

