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

#include "Platform.h"

#include "exp_stdh.h"
#include "exp_clause_derived.h"
#include "exp_datetime.h"
#include "unicode_char_set.h"
#include "wstr.h"
#include "ex_globals.h"

ex_expr::exp_return_type ex_comp_clause::processNulls(char *op_data[],
						      CollHeap *heap,
						      ComDiagsArea **diagsArea)
{
   if (isSpecialNulls())
     {
       // special nulls. Nulls are values.
       // Null = Null, non-null-value < NULL, etc.
       short left_is_null = 0;
       short right_is_null = 0;
       
       if (getOperand(1)->getNullFlag() && (!op_data[1]))
   	left_is_null = -1;
   
       if (getOperand(2)->getNullFlag() && (!op_data[2]))
   	right_is_null = -1;
   
       Lng32 result = 0;
       if ((left_is_null) || (right_is_null))
   	{
   	  switch (getOperType())
   	    {
   	    case ITM_EQUAL:
   	      result = (left_is_null && right_is_null ? -1 : 0);
   	      break;
   	    case ITM_NOT_EQUAL:
   	      result = (left_is_null && right_is_null ? 0 : -1);
   	      break;
   	      
   	    case ITM_GREATER:
   	      result = (right_is_null ? 0 : -1);
   	      break;
   
   	    case ITM_LESS:
   	      result = (left_is_null ? 0 : -1);
   	      break;
   
   	    case ITM_GREATER_EQ:
   	      result = (left_is_null ? -1 : 0);
   	      break;
   	    case ITM_LESS_EQ:
   	      result = (right_is_null ? -1 : 0);
   	      break;
   	    }
   	  
   	  if (result)
   	    {
   	      // the actual result of this operation is pointed to
   	      // by op_data[2 * MAX_OPERANDS].
   	      *(Lng32 *)op_data[2 * MAX_OPERANDS] = 1; // result is TRUE
   	    }
   	  else
   	    {
   	      *(Lng32 *)op_data[2 * MAX_OPERANDS] = 0; // result is FALSE

              if ((getRollupColumnNum() >= 0) &&
                  (getExeGlobals()))
                {
                  getExeGlobals()->setRollupColumnNum(getRollupColumnNum());
                }
   	    }
   	  return ex_expr::EXPR_NULL;
   	} // one of the operands is a null value.
     } // nulls are to be treated as values
   
   for (short i = 1; i < getNumOperands(); i++)
     {
       // if value is missing, 
       // then move boolean unknown value to result and return.
       if (getOperand(i)->getNullFlag() && (!op_data[i])) // missing value 
   	{
   	  // move null value to result.
   	  *(Lng32 *)op_data[2 * MAX_OPERANDS] = -1;
   	  return ex_expr::EXPR_NULL;
   	}
     }
   
   return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type 
ex_comp_clause::processResult(Int32 compare_code, Lng32* result,
                              CollHeap *heap,
                              ComDiagsArea** diagsArea)
{
  *result = 0;
  
  switch (getOperType())
    {
    case ITM_EQUAL:
      if (compare_code == 0)
        *result = 1;
      break;
    case ITM_NOT_EQUAL: 
      if (compare_code != 0)
        *result = 1;
      break;
      
    case ITM_LESS:
      if (compare_code < 0)
        *result = 1;
      break;
      
    case ITM_LESS_EQ:
      if (compare_code <= 0)
        *result = 1;
      break;
      
    case ITM_GREATER:
      if (compare_code > 0)
        *result = 1;
      break;
      
    case ITM_GREATER_EQ:
      if (compare_code >= 0)
        *result = 1;
      break;
      
    default:
      ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
      return ex_expr::EXPR_ERROR;
      break;
    }
  return ex_expr::EXPR_OK;
}

/////////////////////////////////////////////////////////////////
// Compares operand 1 and operand 2. Moves boolean result to
// operand 0. Result is a boolean datatype.
// Result values: 1, TRUE. 0, FALSE.
//               -1, NULL (but this shouldn't happen here.
//                         Nulls have already been processed
//                         before coming here).
////////////////////////////////////////////////////////////////
ex_expr::exp_return_type ex_comp_clause::eval(char *op_data[],
					      CollHeap *heap,
					      ComDiagsArea** diagsArea)
{
  ex_expr::exp_return_type retcode = ex_expr::EXPR_OK;

  switch (getInstruction())
    {
    // EQUAL opcode
    case EQ_BIN8S_BIN8S:
      *(Lng32 *)op_data[0] = (*(Int8 *)op_data[1] == *(Int8 *)op_data[2]);
      break;
  
    case EQ_BIN8U_BIN8U:
      *(Lng32 *)op_data[0] = (*(UInt8 *)op_data[1] == *(UInt8 *)op_data[2]);
      break;
  
    case EQ_BIN16S_BIN16S:
      *(Lng32 *)op_data[0] = (*(short *)op_data[1] == *(short *)op_data[2]);
      break;
  
    case EQ_BIN16S_BIN32S:
      *(Lng32 *)op_data[0] = (*(short *)op_data[1] == *(Lng32 *)op_data[2]);
      break;
      
    case EQ_BIN16S_BIN16U:
      *(Lng32 *)op_data[0] = (*(short *)op_data[1] == *(unsigned short *)op_data[2]);
      break;
    case EQ_BIN16S_BIN32U:
      *(Lng32 *)op_data[0] = ((ULng32)*(short *)op_data[1] == *(ULng32 *)op_data[2]);
      break;
  
  
   
    case EQ_BIN16U_BIN16S:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] == *(short *)op_data[2]);
      break;
    case EQ_BIN16U_BIN32S:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] == *(Lng32 *)op_data[2]);
      break;
      
    case EQ_BIN16U_BIN16U:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] == *(unsigned short *)op_data[2]);
      break;
      
    case EQ_BIN16U_BIN32U:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] == *(ULng32 *)op_data[2]);
      break;
  
   
    case EQ_BIN32S_BIN16S:
      *(Lng32 *)op_data[0] = (*(Lng32 *)op_data[1] == *(short *)op_data[2]);
      break;
  
    case EQ_BIN32S_BIN32S:
      *(Lng32 *)op_data[0] = (*(Lng32 *)op_data[1] == *(Lng32 *)op_data[2]);
      break;
  
    case EQ_BIN32S_BIN16U:
      *(Lng32 *)op_data[0] = (*(Lng32 *)op_data[1] == *(unsigned short *)op_data[2]);
      break;
  
    case EQ_BIN32S_BIN32U:
       *(Lng32 *)op_data[0] = ((ULng32)*(Lng32 *)op_data[1] == *(ULng32 *)op_data[2]);
      break;
  
  
  
    case EQ_BIN32U_BIN16S:
      *(Lng32 *)op_data[0] = (*(ULng32 *)op_data[1] == (ULng32)*(short *)op_data[2]);
      break;
      
    case EQ_BIN32U_BIN32S:
      *(Lng32 *)op_data[0] = (*(ULng32 *)op_data[1] == (ULng32)*(Lng32 *)op_data[2]);
      break;
      
    case EQ_BIN32U_BIN16U:
      *(Lng32 *)op_data[0] = (*(ULng32 *)op_data[1] == *(unsigned short *)op_data[2]);
      break;
      
    case EQ_BIN32U_BIN32U:
      *(Lng32 *)op_data[0] = (*(ULng32 *)op_data[1] == *(ULng32 *)op_data[2]);
      break;
      
  
    case EQ_BIN64S_BIN64S:
      *(Lng32 *)op_data[0] = (*(Int64 *)op_data[1] == *(Int64 *)op_data[2]);
      break;
  
    case EQ_BIN64U_BIN64U:
      *(Lng32 *)op_data[0] = (*(UInt64 *)op_data[1] == *(UInt64 *)op_data[2]);
      break;
  
    case EQ_BIN64U_BIN64S:
      *(Lng32 *)op_data[0] = (*(UInt64 *)op_data[1] == *(Int64 *)op_data[2]);
      break;
  
    case EQ_BIN64S_BIN64U:
      *(Lng32 *)op_data[0] = (*(Int64 *)op_data[1] == *(UInt64 *)op_data[2]);
      break;
  
  
    case EQ_DECU_DECU:
    case EQ_DECS_DECS:
    case EQ_ASCII_F_F:
    case EQ_UNICODE_F_F: // 11/3/97 added for Unicode support
       
      if (str_cmp(op_data[1], op_data[2], (Int32)getOperand(1)->getLength()) == 0)
  	*(Lng32 *)op_data[0] = 1;
      else
  	*(Lng32 *)op_data[0] = 0;
      break;
  
  
    case EQ_FLOAT32_FLOAT32:
      *(Lng32 *)op_data[0] = (*(float *)op_data[1] == *(float *)op_data[2]);
      break;
  
    case EQ_FLOAT64_FLOAT64:
      *(Lng32 *)op_data[0] = (*(double *)op_data[1] == *(double *)op_data[2]);
      break;
  
  
    case EQ_DATETIME_DATETIME:
      if (((ExpDatetime *) getOperand(1))->compDatetimes(op_data[1],
                                                          op_data[2]) == 0)
        *(Lng32 *)op_data[0] = 1;
      else
        *(Lng32 *)op_data[0] = 0;
      break;
  
  
  
    // NOT EQUAL operator
    case NE_BIN8S_BIN8S:
      *(Lng32 *)op_data[0] = (*(Int8 *)op_data[1] != *(Int8 *)op_data[2]);
      break;

    case NE_BIN8U_BIN8U:
      *(Lng32 *)op_data[0] = (*(UInt8 *)op_data[1] != *(UInt8 *)op_data[2]);
      break;

    case NE_BIN16S_BIN16S:
      *(Lng32 *)op_data[0] = (*(short *)op_data[1] != *(short *)op_data[2]);
      break;
  
    case NE_BIN16S_BIN32S:
      *(Lng32 *)op_data[0] = (*(short *)op_data[1] != *(Lng32 *)op_data[2]);
      break;
  
    case NE_BIN16S_BIN16U:
      *(Lng32 *)op_data[0] = (*(short *)op_data[1] != *(unsigned short *)op_data[2]);
      break;
  
    case NE_BIN16S_BIN32U:
      *(Lng32 *)op_data[0] = ((ULng32)*(short *)op_data[1] != *(ULng32 *)op_data[2]);
      break;
  
  
  
    case NE_BIN16U_BIN16S:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] != *(short *)op_data[2]);
      break;
  
    case NE_BIN16U_BIN32S:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] != *(Lng32 *)op_data[2]);
      break;
    case NE_BIN16U_BIN16U:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] != *(unsigned short *)op_data[2]);
      break;
  
    case NE_BIN16U_BIN32U:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] != *(ULng32 *)op_data[2]);
      break;
  
  
    case NE_BIN32S_BIN16S:
      *(Lng32 *)op_data[0] = (*(Lng32 *)op_data[1] != *(short *)op_data[2]);
      break;
  
    case NE_BIN32S_BIN32S:
      *(Lng32 *)op_data[0] = (*(Lng32 *)op_data[1] != *(Lng32 *)op_data[2]);
      break;
  
    case NE_BIN32S_BIN16U:
      *(Lng32 *)op_data[0] = (*(Lng32 *)op_data[1] != *(unsigned short *)op_data[2]);
      break;
  
    case NE_BIN32S_BIN32U:
      *(Lng32 *)op_data[0] = ((ULng32)*(Lng32 *)op_data[1] != *(ULng32 *)op_data[2]);
      break;
  
  
  
    case NE_BIN32U_BIN16S:
      *(Lng32 *)op_data[0] = (*(ULng32 *)op_data[1] != (ULng32)*(short *)op_data[2]);
      break;
  
    case NE_BIN32U_BIN32S:
      *(Lng32 *)op_data[0] = (*(ULng32 *)op_data[1] != (ULng32)*(Lng32 *)op_data[2]);
      break;
  
    case NE_BIN32U_BIN16U:
      *(Lng32 *)op_data[0] = (*(ULng32 *)op_data[1] != *(unsigned short *)op_data[2]);
      break;
    case NE_BIN32U_BIN32U:
      *(Lng32 *)op_data[0] = (*(ULng32 *)op_data[1] != *(ULng32 *)op_data[2]);
      break;
  
    case NE_BIN64S_BIN64S:
      *(Lng32 *)op_data[0] = (*(Int64 *)op_data[1] != *(Int64 *)op_data[2]);
      break;

    case NE_BIN64U_BIN64U:
      *(Lng32 *)op_data[0] = (*(UInt64 *)op_data[1] != *(UInt64 *)op_data[2]);
      break;
  
    case NE_BIN64U_BIN64S:
      *(Lng32 *)op_data[0] = (*(UInt64 *)op_data[1] != *(Int64 *)op_data[2]);
      break;
  
    case NE_BIN64S_BIN64U:
      *(Lng32 *)op_data[0] = (*(Int64 *)op_data[1] != *(UInt64 *)op_data[2]);
      break;
  
    case NE_DECU_DECU:
    case NE_DECS_DECS:
    case NE_ASCII_F_F:
      if (str_cmp(op_data[1], op_data[2], (Int32)getOperand(1)->getLength()) != 0)
  	*(Lng32 *)op_data[0] = 1;
      else
  	*(Lng32 *)op_data[0] = 0;
      break;
      
    case NE_UNICODE_F_F: // 11/3/97: Added for Unicode support
    if (wc_str_cmp((NAWchar*)op_data[1], (NAWchar*)op_data[2], 
                (Int32)getOperand(1)->getLength() >> 1) != 0)
  	*(Lng32 *)op_data[0] = 1;
    else
  	*(Lng32 *)op_data[0] = 0;
    break;
    case NE_FLOAT32_FLOAT32:
      *(Lng32 *)op_data[0] = (*(float *)op_data[1] != *(float *)op_data[2]);
      break;
  
    case NE_FLOAT64_FLOAT64:
      *(Lng32 *)op_data[0] = (*(double *)op_data[1] != *(double *)op_data[2]);
      break;
  
  
    case NE_DATETIME_DATETIME:
      if (((ExpDatetime *) getOperand(1))->compDatetimes(op_data[1],
                                                          op_data[2]) != 0)
        *(Lng32 *)op_data[0] = 1;
      else
        *(Lng32 *)op_data[0] = 0;
      break;
  
  
  
  
    // LESS THAN opcode
    case LT_BIN8S_BIN8S:
      *(Lng32 *)op_data[0] = (*(Int8 *)op_data[1] < *(Int8 *)op_data[2]);
      break;

    case LT_BIN8U_BIN8U:
      *(Lng32 *)op_data[0] = (*(UInt8 *)op_data[1] < *(UInt8 *)op_data[2]);
      break;

    case LT_BIN16S_BIN16S:
      *(Lng32 *)op_data[0] = (*(short *)op_data[1] < *(short *)op_data[2]);
      break;
  
    case LT_BIN16S_BIN32S:
      *(Lng32 *)op_data[0] = (*(short *)op_data[1] < *(Lng32 *)op_data[2]);
      break;
      
    case LT_BIN16S_BIN16U:
      *(Lng32 *)op_data[0] = (*(short *)op_data[1] < *(unsigned short *)op_data[2]);
      break;
  
    case LT_BIN16S_BIN32U:
      *(Lng32 *)op_data[0] = ((Int64)*(short *)op_data[1] < (Int64)*(ULng32 *)op_data[2]);
      break;
  
  
   
    case LT_BIN16U_BIN16S:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] < *(short *)op_data[2]);
      break;
    case LT_BIN16U_BIN32S:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] < *(Lng32 *)op_data[2]);
      break;
    case LT_BIN16U_BIN16U:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] < *(unsigned short *)op_data[2]);
      break;
      
    case LT_BIN16U_BIN32U:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] < *(ULng32 *)op_data[2]);
      break;
  
   
    case LT_BIN32S_BIN16S:
      *(Lng32 *)op_data[0] = (*(Lng32 *)op_data[1] < *(short *)op_data[2]);
      break;
  
    case LT_BIN32S_BIN32S:
      *(Lng32 *)op_data[0] = (*(Lng32 *)op_data[1] < *(Lng32 *)op_data[2]);
      break;
  
    case LT_BIN32S_BIN16U:
      *(Lng32 *)op_data[0] = (*(Lng32 *)op_data[1] < *(unsigned short *)op_data[2]);
      break;
  
    case LT_BIN32S_BIN32U:
      *(Lng32 *)op_data[0] = ((Int64)*(Lng32 *)op_data[1] < (Int64)*(ULng32 *)op_data[2]);
      break;
  
  
    case LT_BIN32U_BIN16S:
      *(Lng32 *)op_data[0] = ((Int64)*(ULng32 *)op_data[1] < *(short *)op_data[2]);
      break;
      
    case LT_BIN32U_BIN32S:
      *(Lng32 *)op_data[0] = ((Int64)*(ULng32 *)op_data[1] < (Int64)*(Lng32 *)op_data[2]);
      break;
      
    case LT_BIN32U_BIN16U:
      *(Lng32 *)op_data[0] = (*(ULng32 *)op_data[1] < *(unsigned short *)op_data[2]);
      break;
      
    case LT_BIN32U_BIN32U:
      *(Lng32 *)op_data[0] = (*(ULng32 *)op_data[1] < *(ULng32 *)op_data[2]);
      break;
      
    case LT_BIN64S_BIN64S:
      *(Lng32 *)op_data[0] = (*(Int64 *)op_data[1] < *(Int64 *)op_data[2]);
      break;
  
    case LT_BIN64U_BIN64U:
      *(Lng32 *)op_data[0] = (*(UInt64 *)op_data[1] < *(UInt64 *)op_data[2]);
      break;
  
    case LT_BIN64U_BIN64S:
      *(Lng32 *)op_data[0] = 
        ((*(Int64*)op_data[2] < 0) ? 0 :
         (*(UInt64 *)op_data[1] < *(Int64 *)op_data[2]));
      break;
  
    case LT_BIN64S_BIN64U:
      *(Lng32 *)op_data[0] = 
        ((*(Int64*)op_data[1] < 0) ? 1 :
         (*(Int64 *)op_data[1] < *(UInt64 *)op_data[2]));
      break;
  
    case LT_DECS_DECS:
      {
  	if ((op_data[1][0] & 0200) == 0)
  	  {
  	    // first operand is positive
  	    if ((op_data[2][0] & 0200) == 0)
  	      {
  		// second operand is positive
  		if (str_cmp(op_data[1], op_data[2], 
  			    (Int32)getOperand(1)->getLength()) < 0) // l < r
  		  *(Lng32 *)op_data[0] = 1;
  		else
  		  *(Lng32 *)op_data[0] = 0;	
  	      }
  	    else
  	      {
  		// second operand is negative
  		*(Lng32 *)op_data[0] = 0; // +ve not < -ve
  	      }
  	  }
  	else
  	  {
  	    // first operand is negative
  	    if ((op_data[2][0] & 0200) == 0)
  	      {
  		// second operand is positive
  		*(Lng32 *)op_data[0] = 1; // -ve negative always < +ve
  	      }
  	    else
  	      {
  		// second operand is negative
  		if (str_cmp(op_data[1], op_data[2], 
  			    (Int32)getOperand(1)->getLength()) <= 0) // l <= r
  		  *(Lng32 *)op_data[0] = 0;
  		else
  		  *(Lng32 *)op_data[0] = 1;	
  	      }    
  	  } // first operand is negative
      }
      break;
  
    case LT_DECU_DECU:
    case LT_ASCII_F_F:
      if (str_cmp(op_data[1], op_data[2], (Int32)getOperand(1)->getLength()) < 0)
  	*(Lng32 *)op_data[0] = 1;
      else
  	*(Lng32 *)op_data[0] = 0;
      break;
  
    case LT_UNICODE_F_F: // 11/5/97: added for Unicode support
      if (wc_str_cmp((NAWchar*)op_data[1], (NAWchar*)op_data[2], 
              (Int32)getOperand(1)->getLength() >> 1) < 0)
        *(Lng32 *)op_data[0] = 1;
      else
        *(Lng32 *)op_data[0] = 0;
  
       break;
  
    case LT_FLOAT32_FLOAT32:
      *(Lng32 *)op_data[0] = (*(float *)op_data[1] < *(float *)op_data[2]);
      break;
  
    case LT_FLOAT64_FLOAT64:
      *(Lng32 *)op_data[0] = (*(double *)op_data[1] < *(double *)op_data[2]);
      break;
  
  
    case LT_DATETIME_DATETIME:
      if (((ExpDatetime *) getOperand(1))->compDatetimes(op_data[1],
                                                          op_data[2]) < 0)
        *(Lng32 *)op_data[0] = 1;
      else
        *(Lng32 *)op_data[0] = 0;
      break;
  
  
  
  
    // LESS THAN OR EQUAL TO opcode
    case LE_BIN8S_BIN8S:
      *(Lng32 *)op_data[0] = (*(Int8 *)op_data[1] <= *(Int8 *)op_data[2]);
      break;

    case LE_BIN8U_BIN8U:
      *(Lng32 *)op_data[0] = (*(UInt8 *)op_data[1] <= *(UInt8 *)op_data[2]);
      break;

    case LE_BIN16S_BIN16S:
      *(Lng32 *)op_data[0] = (*(short *)op_data[1] <= *(short *)op_data[2]);
      break;
  
    case LE_BIN16S_BIN32S:
      *(Lng32 *)op_data[0] = (*(short *)op_data[1] <= *(Lng32 *)op_data[2]);
      break;
      
    case LE_BIN16S_BIN16U:
      *(Lng32 *)op_data[0] = (*(short *)op_data[1] <= *(unsigned short *)op_data[2]);
      break;
  
    case LE_BIN16S_BIN32U:
      *(Lng32 *)op_data[0] = (*(short *)op_data[1] <= (Int64)*(ULng32 *)op_data[2]);
      break;
  
  
   
    case LE_BIN16U_BIN16S:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] <= *(short *)op_data[2]);
      break;
  
    case LE_BIN16U_BIN32S:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] <= *(Lng32 *)op_data[2]);
      break;
      
    case LE_BIN16U_BIN16U:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] <= *(unsigned short *)op_data[2]);
      break;
      
    case LE_BIN16U_BIN32U:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] <= *(ULng32 *)op_data[2]);
      break;
  
   
    case LE_BIN32S_BIN16S:
      *(Lng32 *)op_data[0] = (*(Lng32 *)op_data[1] <= *(short *)op_data[2]);
      break;
  
    case LE_BIN32S_BIN32S:
      *(Lng32 *)op_data[0] = (*(Lng32 *)op_data[1] <= *(Lng32 *)op_data[2]);
      break;
  
    case LE_BIN32S_BIN16U:
      *(Lng32 *)op_data[0] = (*(Lng32 *)op_data[1] <= *(unsigned short *)op_data[2]);
      break;
  
    case LE_BIN32S_BIN32U:
      *(Lng32 *)op_data[0] = ((Int64)*(Lng32 *)op_data[1] <= (Int64)*(ULng32 *)op_data[2]);
      break;
  
  
    case LE_BIN32U_BIN16S:
      *(Lng32 *)op_data[0] = ((Int64)*(ULng32 *)op_data[1] <= *(short *)op_data[2]);
      break;
      
    case LE_BIN32U_BIN32S:
      *(Lng32 *)op_data[0] = ((Int64)*(ULng32 *)op_data[1] <= (Int64)*(Lng32 *)op_data[2]);
      break;
      
    case LE_BIN32U_BIN16U:
      *(Lng32 *)op_data[0] = (*(ULng32 *)op_data[1] <= *(unsigned short *)op_data[2]);
      break;
      
    case LE_BIN32U_BIN32U:
      *(Lng32 *)op_data[0] = (*(ULng32 *)op_data[1] <= *(ULng32 *)op_data[2]);
      break;
      
  
    case LE_BIN64S_BIN64S:
      *(Lng32 *)op_data[0] = (*(Int64 *)op_data[1] <= *(Int64 *)op_data[2]);
      break;
  
    case LE_BIN64U_BIN64U:
      *(Lng32 *)op_data[0] = (*(UInt64 *)op_data[1] <= *(UInt64 *)op_data[2]);
      break;
  
    case LE_BIN64U_BIN64S:
      *(Lng32 *)op_data[0] = 
        ((*(Int64*)op_data[2] < 0) ? 0 :
         (*(UInt64 *)op_data[1] <= *(Int64 *)op_data[2]));
      break;
  
    case LE_BIN64S_BIN64U:
      *(Lng32 *)op_data[0] = 
        ((*(Int64*)op_data[1] < 0) ? 1 :
         (*(Int64 *)op_data[1] <= *(UInt64 *)op_data[2]));
      break;
   
   case LE_DECS_DECS:
      {
  	if ((op_data[1][0] & 0200) == 0)
  	  {
  	    // first operand is positive

  	    if ((op_data[2][0] & 0200) == 0)
  	      {
  		// second operand is positive
  		if (str_cmp(op_data[1], op_data[2], 
  			    (Int32)getOperand(1)->getLength()) <= 0) // l <= r
  		  *(Lng32 *)op_data[0] = 1;
  		else
  		  *(Lng32 *)op_data[0] = 0;	
  	      }
  	    else
  	      {
  		// second operand is negative
  		*(Lng32 *)op_data[0] = 0; // +ve not < -ve
  	      }
  	  }
  	else
  	  {
  	    // first operand is negative
  	    if ((op_data[2][0] & 0200) == 0)
  	      {
  		// second operand is positive
  		*(Lng32 *)op_data[0] = 1; // -ve negative always < +ve
  	      }
  	    else
  	      {
  		// second operand is negative
  		if (str_cmp(op_data[1], op_data[2], 
  			    (Int32)getOperand(1)->getLength()) < 0) // l < r
  		  *(Lng32 *)op_data[0] = 0;
  		else
  		  *(Lng32 *)op_data[0] = 1;	
  	      }    
  	  } // first operand is negative
      }
      break;
  
    case LE_DECU_DECU:
    case LE_ASCII_F_F:
      if (str_cmp(op_data[1], op_data[2], (Int32)getOperand(1)->getLength()) <= 0)
  	*(Lng32 *)op_data[0] = 1;
      else
  	*(Lng32 *)op_data[0] = 0;
      break;
  
    case LE_UNICODE_F_F: // 11/5/97: added for Unicode support
      if (wc_str_cmp((NAWchar*)op_data[1], (NAWchar*)op_data[2], 
              (Int32)getOperand(1)->getLength() >> 1) <= 0)
    	*(Lng32 *)op_data[0] = 1;
      else
    	*(Lng32 *)op_data[0] = 0;
  
      break;
  
    case LE_FLOAT32_FLOAT32:
      *(Lng32 *)op_data[0] = (*(float *)op_data[1] <= *(float *)op_data[2]);
      break;
  
    case LE_FLOAT64_FLOAT64:
      *(Lng32 *)op_data[0] = (*(double *)op_data[1] <= *(double *)op_data[2]);
      break;
  
  
    case LE_DATETIME_DATETIME:
      if (((ExpDatetime *) getOperand(1))->compDatetimes(op_data[1],
                                                          op_data[2]) <= 0)
        *(Lng32 *)op_data[0] = 1;
      else
        *(Lng32 *)op_data[0] = 0;
      break;
  
  
  
  
    // GREATER THAN opcode
    case GT_BIN8S_BIN8S:
      *(Lng32 *)op_data[0] = (*(Int8 *)op_data[1] > *(Int8 *)op_data[2]);
      break;

    case GT_BIN8U_BIN8U:
      *(Lng32 *)op_data[0] = (*(UInt8 *)op_data[1] > *(UInt8 *)op_data[2]);
      break;

    case GT_BIN16S_BIN16S:
      *(Lng32 *)op_data[0] = (*(short *)op_data[1] > *(short *)op_data[2]);
      break;
  
    case GT_BIN16S_BIN32S:
      *(Lng32 *)op_data[0] = (*(short *)op_data[1] > *(Lng32 *)op_data[2]);
      break;
      
    case GT_BIN16S_BIN16U:
      *(Lng32 *)op_data[0] = (*(short *)op_data[1] > *(unsigned short *)op_data[2]);
      break;
  
    case GT_BIN16S_BIN32U:
      *(Lng32 *)op_data[0] = (*(short *)op_data[1] > (Int64)*(ULng32 *)op_data[2]);
      break;
  
  
   
    case GT_BIN16U_BIN16S:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] > *(short *)op_data[2]);
      break;
  
    case GT_BIN16U_BIN32S:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] > *(Lng32 *)op_data[2]);
      break;
      
    case GT_BIN16U_BIN16U:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] > *(unsigned short *)op_data[2]);
      break;
      
    case GT_BIN16U_BIN32U:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] > *(ULng32 *)op_data[2]);
      break;
  
   
    case GT_BIN32S_BIN16S:
      *(Lng32 *)op_data[0] = (*(Lng32 *)op_data[1] > *(short *)op_data[2]);
      break;
  
    case GT_BIN32S_BIN32S:
      *(Lng32 *)op_data[0] = (*(Lng32 *)op_data[1] > *(Lng32 *)op_data[2]);
      break;
  
    case GT_BIN32S_BIN16U:
      *(Lng32 *)op_data[0] = (*(Lng32 *)op_data[1] > *(unsigned short *)op_data[2]);
      break;
  
    case GT_BIN32S_BIN32U:
      *(Lng32 *)op_data[0] = ((Int64)*(Lng32 *)op_data[1] > (Int64)*(ULng32 *)op_data[2]);
      break;
  
  
    case GT_BIN32U_BIN16S:
      *(Lng32 *)op_data[0] = ((Int64)*(ULng32 *)op_data[1] > *(short *)op_data[2]);
      break;
      
    case GT_BIN32U_BIN32S:
      *(Lng32 *)op_data[0] = ((Int64)*(ULng32 *)op_data[1] > (Int64)*(Lng32 *)op_data[2]);
      break;
      
    case GT_BIN32U_BIN16U:
      *(Lng32 *)op_data[0] = (*(ULng32 *)op_data[1] > *(unsigned short *)op_data[2]);
      break;
      
    case GT_BIN32U_BIN32U:
      *(Lng32 *)op_data[0] = (*(ULng32 *)op_data[1] > *(ULng32 *)op_data[2]);
      break;
      
    case GT_BIN64S_BIN64S:
      *(Lng32 *)op_data[0] = (*(Int64 *)op_data[1] > *(Int64 *)op_data[2]);
      break;
  
    case GT_BIN64U_BIN64U:
      *(Lng32 *)op_data[0] = (*(UInt64 *)op_data[1] > *(UInt64 *)op_data[2]);
      break;
  
    case GT_BIN64U_BIN64S:
      *(Lng32 *)op_data[0] = 
        ((*(Int64*)op_data[2] < 0) ? 1 :
         (*(UInt64 *)op_data[1] > *(Int64 *)op_data[2]));
      break;
  
    case GT_BIN64S_BIN64U:
      *(Lng32 *)op_data[0] = 
        ((*(Int64*)op_data[1] < 0) ? 0 :
         (*(Int64 *)op_data[1] > *(UInt64 *)op_data[2]));
      break;
  
    case GT_DECS_DECS:
      {
  	if ((op_data[1][0] & 0200) == 0)
  	  {
  	    // first operand is positive
  	    if ((op_data[2][0] & 0200) == 0)
  	      {
  		// second operand is positive
  		if (str_cmp(op_data[1], op_data[2], 
  			    (Int32)getOperand(1)->getLength()) > 0) // l > r
  		  *(Lng32 *)op_data[0] = 1;
  		else
  		  *(Lng32 *)op_data[0] = 0;	
  	      }
  	    else
  	      {
  		// second operand is negative
  		*(Lng32 *)op_data[0] = 1; // +ve always > -ve
  	      }
  	  }
  	else
  	  {
  	    // first operand is negative
  	    if ((op_data[2][0] & 0200) == 0)
  	      {
  		// second operand is positive
  		*(Lng32 *)op_data[0] = 0; // -ve always <= +ve
  	      }
  	    else
  	      {
  		// second operand is negative
  		if (str_cmp(op_data[1], op_data[2], 
  			    (Int32)getOperand(1)->getLength()) >= 0) // l >= r
  		  *(Lng32 *)op_data[0] = 0;
  		else
  		  *(Lng32 *)op_data[0] = 1;	
  	      }    
  	  } // first operand is negative
      }
      break;
  
    case GT_DECU_DECU:
    case GT_ASCII_F_F:
      if (str_cmp(op_data[1], op_data[2], (Int32)getOperand(1)->getLength()) > 0)
  	*(Lng32 *)op_data[0] = 1;
      else
  	*(Lng32 *)op_data[0] = 0;
      break;
  
  case GT_UNICODE_F_F:
  // 11/3/97: added for Unicode
    if (wc_str_cmp((NAWchar*)op_data[1], (NAWchar*)op_data[2], 
        (Int32)(getOperand(1)->getLength()) >>1) > 0)
  	*(Lng32 *)op_data[0] = 1;
    else
  	*(Lng32 *)op_data[0] = 0;
    break;
  
    case GT_FLOAT32_FLOAT32:
      *(Lng32 *)op_data[0] = (*(float *)op_data[1] > *(float *)op_data[2]);
      break;
  
    case GT_FLOAT64_FLOAT64:
      *(Lng32 *)op_data[0] = (*(double *)op_data[1] > *(double *)op_data[2]);
      break;
  
  
    case GT_DATETIME_DATETIME:
      if (((ExpDatetime *) getOperand(1))->compDatetimes(op_data[1],
                                                          op_data[2]) > 0)
        *(Lng32 *)op_data[0] = 1;
      else
        *(Lng32 *)op_data[0] = 0;
      break;
  
  
  
  
    // GREATER THAN OR EQUAL TO
    case GE_BIN8S_BIN8S:
      *(Lng32 *)op_data[0] = (*(Int8 *)op_data[1] >= *(Int8 *)op_data[2]);
      break;

    case GE_BIN8U_BIN8U:
      *(Lng32 *)op_data[0] = (*(UInt8 *)op_data[1] >= *(UInt8 *)op_data[2]);
      break;

    case GE_BIN16S_BIN16S:
      *(Lng32 *)op_data[0] = (*(short *)op_data[1] >= *(short *)op_data[2]);
      break;
  
    case GE_BIN16S_BIN32S:
      *(Lng32 *)op_data[0] = (*(short *)op_data[1] >= *(Lng32 *)op_data[2]);
      break;
      
    case GE_BIN16S_BIN16U:
      *(Lng32 *)op_data[0] = (*(short *)op_data[1] >= *(unsigned short *)op_data[2]);
      break;
  
    case GE_BIN16S_BIN32U:
      *(Lng32 *)op_data[0] = (*(short *)op_data[1] >= (Int64)*(ULng32 *)op_data[2]);
      break;
   
    case GE_BIN16U_BIN16S:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] >= *(short *)op_data[2]);
      break;
    case GE_BIN16U_BIN32S:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] >= *(Lng32 *)op_data[2]);
      break;
      
    case GE_BIN16U_BIN16U:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] >= *(unsigned short *)op_data[2]);
      break;
      
    case GE_BIN16U_BIN32U:
      *(Lng32 *)op_data[0] = (*(unsigned short *)op_data[1] >= *(ULng32 *)op_data[2]);
      break;
  
   
    case GE_BIN32S_BIN16S:
      *(Lng32 *)op_data[0] = (*(Lng32 *)op_data[1] >= *(short *)op_data[2]);
      break;
  
    case GE_BIN32S_BIN32S:
      *(Lng32 *)op_data[0] = (*(Lng32 *)op_data[1] >= *(Lng32 *)op_data[2]);
      break;
  
    case GE_BIN32S_BIN16U:
      *(Lng32 *)op_data[0] = (*(Lng32 *)op_data[1] >= *(unsigned short *)op_data[2]);
      break;
  
    case GE_BIN32S_BIN32U:
      *(Lng32 *)op_data[0] = ((Int64)*(Lng32 *)op_data[1] >= (Int64)*(ULng32 *)op_data[2]);
      break;
  
  
  
    case GE_BIN32U_BIN16S:
      *(Lng32 *)op_data[0] = ((Int64)*(ULng32 *)op_data[1] >= *(short *)op_data[2]);
      break;
      
    case GE_BIN32U_BIN32S:
      *(Lng32 *)op_data[0] = ((Int64)*(ULng32 *)op_data[1] >= (Int64)*(Lng32 *)op_data[2]);
      break;
      
    case GE_BIN32U_BIN16U:
      *(Lng32 *)op_data[0] = (*(ULng32 *)op_data[1] >= *(unsigned short *)op_data[2]);
      break;
      
    case GE_BIN32U_BIN32U:
      *(Lng32 *)op_data[0] = (*(ULng32 *)op_data[1] >= *(ULng32 *)op_data[2]);
      break;
      
    case GE_BIN64S_BIN64S:
      *(Lng32 *)op_data[0] = (*(Int64 *)op_data[1] >= *(Int64 *)op_data[2]);
      break;
  
    case GE_BIN64U_BIN64U:
      *(Lng32 *)op_data[0] = (*(UInt64 *)op_data[1] >= *(UInt64 *)op_data[2]);
      break;
  
    case GE_BIN64U_BIN64S:
      *(Lng32 *)op_data[0] = 
        ((*(Int64*)op_data[2] < 0) ? 1 :
         (*(UInt64 *)op_data[1] >= *(Int64 *)op_data[2]));
      break;
  
    case GE_BIN64S_BIN64U:
      *(Lng32 *)op_data[0] = 
        ((*(Int64*)op_data[1] < 0) ? 0 :
         (*(Int64 *)op_data[1] >= *(UInt64 *)op_data[2]));
      break;
  
    case GE_DECS_DECS:
      {
  	if ((op_data[1][0] & 0200) == 0)
  	  {
  	    // first operand is positive
  	    if ((op_data[2][0] & 0200) == 0)
  	      {
  		// second operand is positive
  		if (str_cmp(op_data[1], op_data[2], 
  			    (Int32)getOperand(1)->getLength()) >= 0) // l >= r
  		  *(Lng32 *)op_data[0] = 1;
  		else
  		  *(Lng32 *)op_data[0] = 0;	
  	      }
  	    else
  	      {
  		// second operand is negative
  		*(Lng32 *)op_data[0] = 1; // +ve always >= -ve
  	      }
  	  }
  	else
  	  {
  	    // first operand is negative
  	    if ((op_data[2][0] & 0200) == 0)
  	      {
  		// second operand is positive
  		*(Lng32 *)op_data[0] = 0; // -ve always < +ve
  	      }
  	    else
  	      {
  		// second operand is negative
  		if (str_cmp(op_data[1], op_data[2], 
  			    (Int32)getOperand(1)->getLength()) > 0) // l > r
  		  *(Lng32 *)op_data[0] = 0;
  		else
  		  *(Lng32 *)op_data[0] = 1;	
  	      }    
  	  } // first operand is negative
      }
      break;
      
    case GE_DECU_DECU:
    case GE_ASCII_F_F:
      if (str_cmp(op_data[1], op_data[2], (Int32)getOperand(1)->getLength()) >= 0)
  	*(Lng32 *)op_data[0] = 1;
      else
  	*(Lng32 *)op_data[0] = 0;
      break;
  
     case GE_UNICODE_F_F:
    // 11/3/97: added for Unicode
      if (wc_str_cmp((NAWchar*)op_data[1], (NAWchar*)op_data[2], 
                      (Int32)(getOperand(1)->getLength()) >> 1) >= 0)
    	*(Lng32 *)op_data[0] = 1;
      else
    	*(Lng32 *)op_data[0] = 0;
      break;
      
  
    case GE_FLOAT32_FLOAT32:
      *(Lng32 *)op_data[0] = (*(float *)op_data[1] >= *(float *)op_data[2]);
      break;
  
    case GE_FLOAT64_FLOAT64:
      *(Lng32 *)op_data[0] = (*(double *)op_data[1] >= *(double *)op_data[2]);
      break;
  
  
    case GE_DATETIME_DATETIME:
      if (((ExpDatetime *) getOperand(1))->compDatetimes(op_data[1],
                                                          op_data[2]) >= 0)
        *(Lng32 *)op_data[0] = 1;
      else
        *(Lng32 *)op_data[0] = 0;
      break;

     case ASCII_COMP:
     case EQ_ASCII_COMP:
     case GT_ASCII_COMP:
     case GE_ASCII_COMP:
     case LT_ASCII_COMP:
     case LE_ASCII_COMP:
     case NE_ASCII_COMP:
        {

       Lng32 length1 = getOperand(1)->getLength(op_data[-MAX_OPERANDS + 1]);
       Lng32 length2 = getOperand(2)->getLength(op_data[-MAX_OPERANDS + 2]) ;
       
       char padChar = ' ';
	
       if (getCollationEncodeComp())
       {
	  padChar = 0;
       }
       
       Int32 compare_code =
          charStringCompareWithPad( op_data[1], length1, op_data[2], length2, padChar);


       retcode = processResult(compare_code, (Lng32 *)op_data[0], 
			       heap, diagsArea);
       break;
     }

    case UNICODE_COMP: // 11/3/95: Unicode
     {
       Lng32 length1 = getOperand(1)->getLength(op_data[-MAX_OPERANDS + 1]); 
       Lng32 length2 = getOperand(2)->getLength(op_data[-MAX_OPERANDS + 2]);

       Int32 compare_code =
          wcharStringCompareWithPad((NAWchar*)op_data[1], length1>>1, 
                             (NAWchar*)op_data[2], length2>>1,
                              unicode_char_set::space_char()
                            );

       retcode = processResult(compare_code, (Lng32 *)op_data[0], 
			       heap, diagsArea);

       break;
        }

     // boolean comparison
    case EQ_BOOL_BOOL:
      {
        *(Lng32*)op_data[0] = (*(Int8 *)op_data[1] == *(Int8 *)op_data[2]);
      }
      break;

    case NE_BOOL_BOOL:
      {
        *(Lng32*)op_data[0] = (*(Int8 *)op_data[1] != *(Int8 *)op_data[2]);
      }
      break;

    case COMP_COMPLEX:
      *(Lng32 *)op_data[0] =
	((ComplexType *)getOperand(1))->comp(getOperType(), getOperand(2), op_data);
      break;

    case BINARY_COMP:
      {
        Lng32 length1 = getOperand(1)->getLength(op_data[-MAX_OPERANDS + 1]);
        Lng32 length2 = getOperand(2)->getLength(op_data[-MAX_OPERANDS + 2]) ;
        
        char padChar = 0;

        Int32 compare_code =
          charStringCompareWithPad( op_data[1], length1, op_data[2], length2, 
                                    padChar);

        retcode = processResult(compare_code, (Lng32 *)op_data[0], 
                                heap, diagsArea);
      }
      break;
    
    case COMP_NOT_SUPPORTED:
      {
	// this comparison operation not supported.
	// See if it could still be evaluated by doing some intermediate
	// operations.
	if (evalUnsupportedOperations(op_data, heap, diagsArea) !=
	    ex_expr::EXPR_OK)
	  return ex_expr::EXPR_ERROR;
      }
    break;

    default:
      ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
      retcode = ex_expr::EXPR_ERROR;
      break;
    }

  if ((getRollupColumnNum() >= 0) &&
      (*(Lng32*)op_data[0] == 0) &&
      (getExeGlobals()))
    {
      getExeGlobals()->setRollupColumnNum(getRollupColumnNum());
    }

  return retcode;

}
ex_expr::exp_return_type ex_comp_clause::evalUnsupportedOperations(
     char *op_data[],
     CollHeap *heap,
     ComDiagsArea** diagsArea)
{
  // if this operation could be done by converting to an
  // intermediate datatype, do it.
  short op1Type = getOperand(1)->getDatatype();
  short op2Type = getOperand(2)->getDatatype();
  
  ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
  return ex_expr::EXPR_ERROR;
}









