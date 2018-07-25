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
 * File:         exp_function_upper_unicode.h
 * RCS:          $Id: 
 * Description:  The implementation of NCHAR version of SQL UPPER() function
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

ex_function_upper_unicode::ex_function_upper_unicode(OperatorTypeEnum oper_type,
				     Attributes ** attr, Space * space)
: ex_function_clause(oper_type, 2, attr, space)
{
  
}

ex_function_upper_unicode::ex_function_upper_unicode()
{
}

extern NAWchar unicodeToUpper(NAWchar wc);


ex_expr::exp_return_type ex_function_upper_unicode::eval(char *op_data[],
				 CollHeap* heap,
				 ComDiagsArea** diagsArea)
{ 
  Lng32 len1 = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);
  // Now, copy the contents of operand 1 after the case change into
  // operand 0.

  NAWchar* target = (NAWchar*)op_data[0];
  NAWchar* source = (NAWchar*)op_data[1];
  Int32 wc_len = len1/sizeof(NAWchar);
  Int32 actual_len = 0;
  NAWchar* tmpWCP = NULL;
  Int32 maxWideChars = getOperand(0)->getLength() / sizeof(NAWchar);
  
  // JQ
  // A given character will be searched against two mapping tables
  for (Int32 i = 0; i < wc_len; i++) { 
    // search against unicode_lower2upper_mapping_table_full
    tmpWCP = unicode_char_set::to_upper_full(source[i]);
    if (tmpWCP) { 
      // there is an entry for the given character
      // for the sake of uniformity, the length for the returned NAWchar
      // array is always 3, but the third element can be 0, 
      // assign the third element to the target if it is not 0      

      if (actual_len + ((tmpWCP[2] == 0) ? 2 : 3) > maxWideChars ) {
        ExRaiseSqlError(heap, diagsArea, EXE_STRING_OVERFLOW);
        return ex_expr::EXPR_ERROR;
      }

      target[actual_len++] = tmpWCP[0];
      target[actual_len++] = tmpWCP[1];

      if (tmpWCP[2] != (NAWchar)0) {
	target[actual_len++] = tmpWCP[2];
      }
    } else { 

      if (actual_len >= maxWideChars ) {
         ExRaiseSqlError(heap, diagsArea, EXE_STRING_OVERFLOW);
         return ex_expr::EXPR_ERROR;
      }

      // a NULL return 
      // search against unicode_lower2upper_mapping_table then
      target[actual_len++] = unicode_char_set::to_upper(source[i]);
    }

  }

  getOperand(0)->setVarLength(actual_len*2, op_data[-MAX_OPERANDS]);
  
  return ex_expr::EXPR_OK;
};

