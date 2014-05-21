/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
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

// -----------------------------------------------------------------------

#include "Platform.h"


#include <stddef.h>
#include "exp_stdh.h"
#include "exp_clause_derived.h"
#include "str.h"

///////////////////////////////////////////////////
// class ex_aggregate_clause
//////////////////////////////////////////////////
ex_expr::exp_return_type ex_aggregate_clause::init()
{
  return ex_expr::EXPR_OK;
}
// LCOV_EXCL_START
ex_expr::exp_return_type ex_aggregate_clause::eval(char * /*op_data*/[],
						   CollHeap *heap,
						   ComDiagsArea** diagsArea)
{
  ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
  return ex_expr::EXPR_ERROR;
}
// LCOV_EXCL_STOP
/////////////////////////////////////////////////
// class ex_aggr_one_row_clause
/////////////////////////////////////////////////
ex_expr::exp_return_type ex_aggr_one_row_clause::init()
{
  oneRowProcessed_ = 0;

  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ex_aggr_one_row_clause::eval(char * /*op_data*/ [],
						      CollHeap *heap,
						      ComDiagsArea** diagsArea)
{
  ex_expr::exp_return_type retcode = ex_expr::EXPR_OK;
   
  // if oneRowProcessed_ is > 0, return error.
  if (oneRowProcessed_) {
    ExRaiseSqlError(heap, diagsArea, EXE_CARDINALITY_VIOLATION);
    retcode = ex_expr::EXPR_ERROR;
  } else
    {
      oneRowProcessed_ = 1;
    }

  return retcode;
}

/////////////////////////////////////////////////
// class ex_aggr_any_true_max_clause
/////////////////////////////////////////////////
ex_expr::exp_return_type ex_aggr_any_true_max_clause::init()
{
  nullSeen_ = 0;

  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ex_aggr_any_true_max_clause::eval(char *op_data[],
							   CollHeap *heap,
							   ComDiagsArea** diagsArea)
{
  ex_expr::exp_return_type retcode = ex_expr::EXPR_OK;

  switch (*(Lng32 *)op_data[1]) 
    {
    case 1:  // operand is TRUE
      {
	// return TRUE as result
	*(Lng32 *)op_data[0] = 1;
	retcode = ex_expr::EXPR_TRUE;
      }
      break;
      
    case -1:  // operand is NULL
      {
	// remember that a null was seen.
   // LCOV_EXCL_START
	nullSeen_ = 1;

	// Genesis 10-040203-2921
	// Fix for nested query returning different number of rows with ESP CQD.
	// The case where all operands are NULL wasnt being handled. When all
	// the operands are NULL, the result too is NULL.
	*(Lng32 *) op_data[0] = -1;
      }
      break;
      
    case 0:  // operand is FALSE
      {
	if (nullSeen_)
	  *(Lng32 *)op_data[0] = -1;
	else
	  *(Lng32 *)op_data[0] = 0;
      
	retcode = ex_expr::EXPR_TRUE;
      }
      break;
      
    default:
      {
	ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
	retcode = ex_expr::EXPR_ERROR;
	// LCOV_EXCL_STOP
      }
      break;

    }

  // return code of EXPR_TRUE tells the caller to short circuit
  // and not process any more rows.
  return retcode;
}

/////////////////////////////////////////////////
// class ex_aggr_min_max_clause
/////////////////////////////////////////////////
ex_expr::exp_return_type ex_aggr_min_max_clause::eval(char * op_data[],
						      CollHeap*,
						      ComDiagsArea **)
{
  ex_expr::exp_return_type retcode = ex_expr::EXPR_OK;
  
  // if the second expression is true, make child to be current
  // aggregate.
  if (*(Lng32 *)op_data[2] == 1)
    {
      if (getOperand(0)->getNullFlag())
        {
          // A pointer to the null indicators of the operands.
          //
          char **null_data = &op_data[-2 * ex_clause::MAX_OPERANDS];
          
	  if ((getOperand(1)->getNullFlag()) &&
	      (! null_data[1])) // missing value, indicates
	                        // child is null. Keep the current result.
	    return ex_expr::EXPR_OK;

          ExpTupleDesc::clearNullValue(null_data[0],
                                       getOperand(0)->getNullBitIndex(),
                                       getOperand(0)->getTupleFormat() );
        }

      if (getOperand(0)->getVCIndicatorLength() > 0)
	{
	  // variable length operand. Note that first child (operand1)
	  // and result have the SAME attributes for min/max aggr.
#pragma nowarn(1506)   // warning elimination 
	  Lng32 src_length = getOperand(1)->getLength(op_data[-MAX_OPERANDS + 1]);
#pragma warn(1506)  // warning elimination 
	  Lng32 tgt_length = getOperand(0)->getLength(); // max varchar length
	
	  str_cpy_all(op_data[0], op_data[1], src_length);
	  
	  // copy source length bytes to target length bytes.
	  // Note that the array index -MAX_OPERANDS will get to
	  // the corresponding varlen entry for that operand.
	  getOperand(0)->setVarLength(src_length, op_data[- MAX_OPERANDS]);
	}
      else
	{
	  str_cpy_all(op_data[0], op_data[1], getOperand(0)->getLength());
	}
    }
  
  return retcode;
}
