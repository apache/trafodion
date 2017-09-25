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

// -----------------------------------------------------------------------

#include "Platform.h"


#include <stddef.h>
#include "exp_stdh.h"
#include "exp_clause_derived.h"
#include "str.h"

///////////////////////////////////////////////////
// class AggrExpr
//////////////////////////////////////////////////
ex_expr::exp_return_type AggrExpr::initializeAggr(atp_struct * atp)
{
  if (initExpr_)
    {
      if (initExpr_->eval(atp, atp) == ex_expr::EXPR_ERROR)
	return ex_expr::EXPR_ERROR;
    }

  if (perrecExpr_)
    {
      ex_clause *clause = perrecExpr_->getClauses();
      while (clause)
        {
          if (clause->getType() == ex_clause::AGGREGATE_TYPE)
            {
              if (((ex_aggregate_clause *)clause)->init() == ex_expr::EXPR_ERROR)
                return ex_expr::EXPR_ERROR;
            }
          
          clause = clause->getNextClause();
        }
    }

  if (groupingExpr_)
    {
      ex_clause *clause = groupingExpr_->getClauses();
      while (clause)
        {
          if (clause->getType() == ex_clause::AGGREGATE_TYPE)
            {
              if (((ex_aggregate_clause *)clause)->init() == ex_expr::EXPR_ERROR)
                return ex_expr::EXPR_ERROR;
            }
          
          clause = clause->getNextClause();
        }
    }

  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type AggrExpr::finalizeAggr(atp_struct * /*atp*/)
{
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type AggrExpr::finalizeNullAggr(atp_struct * atp)
{
  if (finalNullExpr_)
    return finalNullExpr_->eval(atp, atp);
  else
    return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type AggrExpr::evalGroupingForNull(
     Int16 startEntry, Int16 endEntry)
{
  if (groupingExpr_)
    {
      ex_clause *clause = groupingExpr_->getClauses();
      while (clause)
        {
          if (clause->getOperType() == ITM_AGGR_GROUPING_FUNC)
            {
              ExFunctionGrouping * g = (ExFunctionGrouping*)clause;
              if ((g->getRollupGroupIndex() >= startEntry) &&
                  (g->getRollupGroupIndex() <= endEntry))
                g->setRollupNull(-1);
            }
          
          clause = clause->getNextClause();
        }
    }

  return ex_expr::EXPR_OK;
}

///////////////////////////////////////////////////
// class ex_aggregate_clause
//////////////////////////////////////////////////
ex_expr::exp_return_type ex_aggregate_clause::init()
{
  return ex_expr::EXPR_OK;
}
ex_expr::exp_return_type ex_aggregate_clause::eval(char * /*op_data*/[],
						   CollHeap *heap,
						   ComDiagsArea** diagsArea)
{
  ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
  return ex_expr::EXPR_ERROR;
}
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

/////////////////////////////////////////////////
// class ExFunctionGrouping
/////////////////////////////////////////////////
ex_expr::exp_return_type ExFunctionGrouping::init()
{
  ex_expr::exp_return_type retcode = ex_expr::EXPR_OK;

  rollupNull_ = 0;

  return retcode;  
}

ex_expr::exp_return_type ExFunctionGrouping::eval(char *op_data[],
                                                  CollHeap *heap,
                                                  ComDiagsArea** diagsArea)
{
  char * tgt = op_data[0];
  if (rollupNull_)
    *(UInt32*)tgt = 1;
  else
    *(UInt32*)tgt = 0;

  return ex_expr::EXPR_OK;
}

/////////////////////////////////////////////////
// class ex_pivot_group_clause
/////////////////////////////////////////////////
ex_expr::exp_return_type ex_pivot_group_clause::init()
{
  ex_expr::exp_return_type retcode = ex_expr::EXPR_OK;

  currPos_ = 0;
  currTgtLen_ = 0;
  setOvflWarn(FALSE);

  return retcode;
}

ex_expr::exp_return_type ex_pivot_group_clause::eval(char * op_data[],
                                                     CollHeap       *heap,
                                                     ComDiagsArea  **diagsArea)
{
  Attributes * tgtOp = getOperand(0);
  Attributes * srcOp = getOperand(1);

  Lng32 src_length = srcOp->getLength(op_data[-MAX_OPERANDS + 1]);

  char * tgt = op_data[0];
  char * src = op_data[1];

  Lng32 currSrcPos = currPos_;

  Lng32 delimSize = strlen(delim_);
  Lng32 tgtBufNeeded = (currPos_ > 0 ? delimSize : 0)  + src_length;

  if ((currTgtLen_ + tgtBufNeeded) > maxLen_)
    {
      // not enough space in tgt buffer to move source.
      // return a warning, if it has not already been returned.
      if (NOT ovflWarn())
        {
          ExRaiseSqlWarning(heap, diagsArea, (ExeErrorCode)(8402));

          setOvflWarn(TRUE);
        }

      return ex_expr::EXPR_OK;
    }

  if (currPos_ > 0)
    {
      str_cpy_all(&tgt[currPos_], delim_, strlen(delim_));
      currPos_ += strlen(delim_);
    }

  str_cpy_all(&tgt[currPos_], src, src_length);
  currPos_ += src_length;

  currTgtLen_ += (currPos_ - currSrcPos);
  
  tgtOp->setVarLength(currTgtLen_, op_data[- MAX_OPERANDS]);
  
  return ex_expr::EXPR_OK;
}
