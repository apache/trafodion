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
#include "exp_function.h"
#include "exp_interval.h"
#include "SQLTypeDefs.h"

__declspec(dllimport) NABoolean ExExprComputeSpace(ex_tcb * tcb);

static void getCaseDatatypes(short attr_type1, Lng32 attr_len1, short &type_op1,
                             short attr_type2, Lng32 attr_len2, short &type_op2,
                             Lng32 scaleDifference)
{
  type_op1 = attr_type1;
  type_op2 = attr_type2;
  //
  // If an operand is an interval, get its end field.
  //
  rec_datetime_field endField1;
  rec_datetime_field endField2;
  if ((type_op1 >= REC_MIN_INTERVAL) && (type_op1 <= REC_MAX_INTERVAL) &&
      (ExpInterval::getIntervalEndField(type_op1, endField1) != 0))
    return;
  if ((type_op2 >= REC_MIN_INTERVAL) && (type_op2 <= REC_MAX_INTERVAL) &&
      (ExpInterval::getIntervalEndField(type_op2, endField2) != 0))
    return;
  //
  // If an operand is an interval and the other is an exact numeric or
  // datetime, treat the interval as numeric.  If both operands are intervals
  // and they have the same end fields and the same fraction precision, 
  // treat them both as numeric. If an interval operand remains after
  // these transformations, map it to a canonical type.
  //
  if ((type_op1 >= REC_MIN_INTERVAL) && (type_op1 <= REC_MAX_INTERVAL))
    {
    if ( ((type_op2 >= REC_MIN_BINARY_NUMERIC) && (type_op2 <= REC_MAX_BINARY_NUMERIC)) ||
         ((type_op2 >= REC_MIN_DECIMAL) && (type_op2 <= REC_MAX_DECIMAL)) ||
         (type_op2 == REC_DATETIME) ||
         ((type_op2 >= REC_MIN_INTERVAL) && (type_op2 <= REC_MAX_INTERVAL) &&
          (endField1 == endField2) && (scaleDifference == 0)) ) 
      {
      switch (attr_len1) 
        {
        case SQL_SMALL_SIZE:
          type_op1 = REC_BIN16_SIGNED;
          break;
        case SQL_INT_SIZE:
          type_op1 = REC_BIN32_SIGNED;
          break;
        case SQL_LARGE_SIZE:
          type_op1 = REC_BIN64_SIGNED;
          break;
        default:
          assert(0);
          break;
        }
      }

    // if type_op1 remained an interval datatype, convert it to canonical form
    // (since internal representation only depends on end field)

    switch(type_op1)
      {
      case REC_INT_YEAR_MONTH:
        type_op1 = REC_INT_MONTH;
        break;
      case REC_INT_DAY_HOUR:
        type_op1 = REC_INT_HOUR;
        break;
      case REC_INT_DAY_MINUTE:
      case REC_INT_HOUR_MINUTE:
        type_op1 = REC_INT_MINUTE;
        break;
      case REC_INT_DAY_SECOND:
      case REC_INT_HOUR_SECOND:
      case REC_INT_MINUTE_SECOND:
        type_op1 = REC_INT_SECOND;
        break;
      default:
        break;  // leave it unchanged
      }

    }  // end if type_op1 is an interval

  if ((type_op2 >= REC_MIN_INTERVAL) && (type_op2 <= REC_MAX_INTERVAL)) 
    {
    if ( ((type_op1 >= REC_MIN_BINARY_NUMERIC) && (type_op1 <= REC_MAX_BINARY_NUMERIC)) ||
         ((type_op1 >= REC_MIN_DECIMAL) && (type_op1 <= REC_MAX_DECIMAL)) ||
         (type_op1 == REC_DATETIME) )
      {
      switch (attr_len2) 
        {
        case SQL_SMALL_SIZE:
          type_op2 = REC_BIN16_SIGNED;
          break;
        case SQL_INT_SIZE:
          type_op2 = REC_BIN32_SIGNED;
          break;
        case SQL_LARGE_SIZE:
          type_op2 = REC_BIN64_SIGNED;
          break;
        }
      }

    switch(type_op2)
      {
      case REC_INT_YEAR_MONTH:
        type_op2 = REC_INT_MONTH;
        break;
      case REC_INT_DAY_HOUR:
        type_op2 = REC_INT_HOUR;
        break;
      case REC_INT_DAY_MINUTE:
      case REC_INT_HOUR_MINUTE:
        type_op2 = REC_INT_MINUTE;
        break;
      case REC_INT_DAY_SECOND:
      case REC_INT_HOUR_SECOND:
      case REC_INT_MINUTE_SECOND:
        type_op2 = REC_INT_SECOND;
        break;
      default:
        break;  // leave it unchanged
      }

    }  // end if type_op2 is an interval
}

static void getConvCaseDatatypes(short attr_type1, Lng32 attr_len1, short &type_op1,
				 short attr_type2, Lng32 attr_len2, short &type_op2,
				 Lng32 scaleDifference)
{
  // 
  // If an operand is ASCII and other is interval, just return
  // some interval type (so as to not add multiple entries in
  // the convert case array). At actual conversion time, the
  // correct conversion is done based on the start and end fields.
  // Lets make that 'fixed' datatype to be REC_INT_YEAR.
  // 
  if ((attr_type1 >= REC_MIN_INTERVAL) && (attr_type1 <= REC_MAX_INTERVAL) &&
      (attr_type2 >= REC_MIN_CHARACTER) && (attr_type2 <= REC_MAX_CHARACTER))
    {
      type_op1 = REC_INT_YEAR;
      type_op2 = attr_type2;
    }
  else
    if ((attr_type2 >= REC_MIN_INTERVAL) && (attr_type2 <= REC_MAX_INTERVAL) &&
	(attr_type1 >= REC_MIN_CHARACTER) && (attr_type1 <= REC_MAX_CHARACTER))
      {
	type_op2 = REC_INT_YEAR;
	type_op1 = attr_type1;
      }
    else
      getCaseDatatypes(attr_type1, attr_len1, type_op1, 
		       attr_type2, attr_len2, type_op2,
		       scaleDifference);
}

ex_expr::exp_return_type ex_expr::fixup(Lng32 /*base*/, unsigned short mode,
                                        const ex_tcb * tcb,
					Space * space,
					CollHeap * exHeap,
					NABoolean computeSpaceOnly,
					ex_globals * glob)
{
  exp_return_type retcode = EXPR_OK;

  setHeap(exHeap);
  myTcb_ = tcb;

  // The check for space is to prevent allocating off the heap in DP2.
  if (space && tempsAreaLength_ > 0)  // temps used
    {
      char * tempAllocArea = new(space) char[tempsAreaLength_];
      if (NOT computeSpaceOnly)
	tempsArea_ = tempAllocArea;
    }
  
  ex_clause *clause = clauses_;
  while (clause)
    {
      retcode = clause->fixup(space, exHeap,
			      constantsArea_, tempsArea_, persistentArea_,
			      (short)getFixupConstsAndTemps(),
			      computeSpaceOnly);
      clause->setProcessNulls();
      if (retcode == EXPR_ERROR)
	return retcode;

      clause->setExeGlobals(glob);
      clause->setTcb(tcb); 
      clause = clause->getNextClause();
    }

  // Set the fast evaluation pointer
  if (!computeSpaceOnly)
    setEvalPtr( TRUE /* isSamePrimary */ );

  if (mode & INJECT_ERROR)
    errorInjection_ = TRUE;
  else 
    if (mode & INJECT_WARNING)
      warningInjection_ = TRUE;

  return EXPR_OK;
};

ex_expr::exp_return_type ex_expr_lean::fixup(Lng32 /*base*/, 
					     unsigned short mode,
					     const ex_tcb * tcb,
					     Space * space,
					     CollHeap * exHeap,
					     NABoolean computeSpaceOnly)
{
  exp_return_type retcode = EXPR_OK;

  // The check for space is to prevent allocating off the heap in DP2.
  if (space && tempsAreaLength_ > 0)  // temps used
    {
      char * tempAllocArea = new(space) char[tempsAreaLength_];
      if (NOT computeSpaceOnly)
	tempsArea_ = tempAllocArea;
    }

  setHeap(exHeap);

  return retcode;
};

ex_expr::exp_return_type AggrExpr::fixup(Lng32 base, unsigned short mode,
                                         const ex_tcb * tcb,
					 Space * space,
					 CollHeap * exHeap,
					 NABoolean computeSpaceOnly,
					 ex_globals * glob)
{
  ex_expr::exp_return_type retcode;

  if (initExpr_)
    {
      if ((retcode = initExpr_->fixup(base, mode, tcb, space, 
				      exHeap, computeSpaceOnly, glob)) != EXPR_OK)
	return retcode;
    }
    
  if (perrecExpr_)
    {
      if ((retcode = perrecExpr_->fixup(base, mode, tcb, space, 
					exHeap, computeSpaceOnly, glob)) != EXPR_OK)
	return retcode;
    }

  if (finalNullExpr_)
    {
      if ((retcode = finalNullExpr_->fixup(base, mode, tcb, space, 
					   exHeap, computeSpaceOnly, glob))
	  != EXPR_OK)
	return retcode;
    }

  if (finalExpr_)
    {
      if ((retcode = finalExpr_->fixup(base, mode, tcb, space, 
				       exHeap, computeSpaceOnly, glob)) != EXPR_OK)
	return retcode;
    }

  if (groupingExpr_)
    {
      if ((retcode = groupingExpr_->fixup(base, mode, tcb, space, 
                                          exHeap, computeSpaceOnly, glob)) != EXPR_OK)
	return retcode;
    }

  // fixup the expression to do perrec aggregate evaluation.
  if ((retcode = ex_expr::fixup(base, mode, tcb, space, 
				exHeap, computeSpaceOnly, glob)) != EXPR_OK)
    return retcode;
    
  return EXPR_OK;
}  

/////////////////////////////////////////////////////////
// clause fixup
/////////////////////////////////////////////////////////
ex_expr::exp_return_type ex_clause::fixup(Space * space,
					  CollHeap * exHeap,
					  char * constantsArea,
					  char * tempsArea,
					  char * persistentArea,
					  short fixupFlag,
					  NABoolean spaceCompOnly)
{
  for (Lng32 i=0; i<numOperands_;i++)
    if (op_[i])
      op_[i]->fixup(space, 
		    constantsArea,
		    tempsArea,
		    persistentArea,
		    fixupFlag,
		    spaceCompOnly);
  
  return ex_expr::EXPR_OK;
};


ex_expr::exp_return_type ex_branch_clause::fixup(Space * space,
                                                 CollHeap * exHeap,
                                                 char * constantsArea,
                                                 char * tempsArea,
                                                 char * persistentArea,
                                                 short fixupFlag,
						 NABoolean spaceCompOnly) {
  if (getNextClause() != saved_next_clause)
    setNextClause(saved_next_clause);
  return ex_clause::fixup(space, exHeap, constantsArea,tempsArea, persistentArea, fixupFlag, spaceCompOnly);
}


const ex_arith_clause::ArithInstrStruct ex_arith_clause::arithInstrInfo[] =
  {
    // op       op1 datatype      op2 datatype      result datatype   case statement index
    {ITM_PLUS, REC_BIN16_SIGNED, REC_BIN16_SIGNED, REC_BIN16_SIGNED, instrAndText(ADD_BIN16S_BIN16S_BIN16S)},
    {ITM_PLUS, REC_BIN16_SIGNED, REC_BIN16_SIGNED, REC_BIN32_SIGNED, instrAndText(ADD_BIN16S_BIN16S_BIN32S)},
    {ITM_PLUS, REC_BIN16_SIGNED, REC_BIN32_SIGNED, REC_BIN32_SIGNED, instrAndText(ADD_BIN16S_BIN32S_BIN32S)},
    {ITM_PLUS, REC_BIN32_SIGNED, REC_BIN16_SIGNED, REC_BIN32_SIGNED, instrAndText(ADD_BIN32S_BIN16S_BIN32S)},
    {ITM_PLUS, REC_BIN32_SIGNED, REC_BIN32_SIGNED, REC_BIN32_SIGNED, instrAndText(ADD_BIN32S_BIN32S_BIN32S)},
    {ITM_PLUS, REC_BIN32_SIGNED, REC_BIN64_SIGNED, REC_BIN64_SIGNED, instrAndText(ADD_BIN32S_BIN64S_BIN64S)},
    {ITM_PLUS, REC_BIN64_SIGNED, REC_BIN32_SIGNED, REC_BIN64_SIGNED, instrAndText(ADD_BIN64S_BIN32S_BIN64S)},
    {ITM_PLUS, REC_BIN64_SIGNED, REC_BIN64_SIGNED, REC_BIN64_SIGNED, instrAndText(ADD_BIN64S_BIN64S_BIN64S)},

    {ITM_PLUS, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(ADD_BIN16U_BIN16U_BIN16U)},      
    {ITM_PLUS, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(ADD_BIN16U_BIN16U_BIN16U)},
    {ITM_PLUS, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(ADD_BIN16U_BIN16U_BIN16U)},
    {ITM_PLUS, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(ADD_BIN16U_BIN16U_BIN32U)},
    {ITM_PLUS, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(ADD_BIN16U_BIN16U_BIN32U)},
    {ITM_PLUS, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(ADD_BIN16U_BIN16U_BIN32U)},
    {ITM_PLUS, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(ADD_BIN16U_BIN32U_BIN32U)},
    {ITM_PLUS, REC_BIN32_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(ADD_BIN32U_BIN16U_BIN32U)},
    {ITM_PLUS, REC_BPINT_UNSIGNED, REC_BIN64_SIGNED,   REC_BIN64_SIGNED,   instrAndText(ADD_BPINTU_BIN64S_BIN64S)},
    {ITM_PLUS, REC_BIN64_SIGNED,   REC_BPINT_UNSIGNED, REC_BIN64_SIGNED,   instrAndText(ADD_BIN64S_BPINTU_BIN64S)},

    {ITM_PLUS, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(ADD_BIN16U_BIN16U_BIN16U)},
    {ITM_PLUS, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(ADD_BIN16U_BIN16U_BIN32U)},
    {ITM_PLUS, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(ADD_BIN16U_BIN32U_BIN32U)},
    {ITM_PLUS, REC_BIN32_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(ADD_BIN32U_BIN16U_BIN32U)},
    {ITM_PLUS, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(ADD_BIN32U_BIN32U_BIN32U)},
    {ITM_PLUS, REC_BIN32_UNSIGNED, REC_BIN64_SIGNED, REC_BIN64_SIGNED, instrAndText(ADD_BIN32U_BIN64S_BIN64S)},
    {ITM_PLUS, REC_BIN64_SIGNED, REC_BIN32_UNSIGNED, REC_BIN64_SIGNED, instrAndText(ADD_BIN64S_BIN32U_BIN64S)},

    {ITM_PLUS, REC_FLOAT32, REC_FLOAT32, REC_FLOAT32, instrAndText(ADD_FLOAT32_FLOAT32_FLOAT32)},
    {ITM_PLUS, REC_FLOAT64, REC_FLOAT64, REC_FLOAT64, instrAndText(ADD_FLOAT64_FLOAT64_FLOAT64)},
    {ITM_PLUS, REC_DATETIME, REC_BIN16_SIGNED, REC_DATETIME, instrAndText(ADD_DATETIME_INTERVAL_DATETIME)},
    {ITM_PLUS, REC_DATETIME, REC_BIN32_SIGNED, REC_DATETIME, instrAndText(ADD_DATETIME_INTERVAL_DATETIME)},
    {ITM_PLUS, REC_DATETIME, REC_BIN64_SIGNED, REC_DATETIME, instrAndText(ADD_DATETIME_INTERVAL_DATETIME)},
    {ITM_PLUS, REC_BIN16_SIGNED, REC_DATETIME, REC_DATETIME, instrAndText(ADD_INTERVAL_DATETIME_DATETIME)},
    {ITM_PLUS, REC_BIN32_SIGNED, REC_DATETIME, REC_DATETIME, instrAndText(ADD_INTERVAL_DATETIME_DATETIME)},
    {ITM_PLUS, REC_BIN64_SIGNED, REC_DATETIME, REC_DATETIME, instrAndText(ADD_INTERVAL_DATETIME_DATETIME)},

    {ITM_PLUS, REC_UNKNOWN, REC_UNKNOWN, REC_UNKNOWN,  instrAndText(ADD_COMPLEX)},

      
    {ITM_MINUS, REC_BIN16_SIGNED, REC_BIN16_SIGNED, REC_BIN16_SIGNED, instrAndText(SUB_BIN16S_BIN16S_BIN16S)},
    {ITM_MINUS, REC_BIN16_SIGNED, REC_BIN16_SIGNED, REC_BIN32_SIGNED, instrAndText(SUB_BIN16S_BIN16S_BIN32S)},
    {ITM_MINUS, REC_BIN16_SIGNED, REC_BIN32_SIGNED, REC_BIN32_SIGNED, instrAndText(SUB_BIN16S_BIN32S_BIN32S)},
    {ITM_MINUS, REC_BIN32_SIGNED, REC_BIN16_SIGNED, REC_BIN32_SIGNED, instrAndText(SUB_BIN32S_BIN16S_BIN32S)},
    {ITM_MINUS, REC_BIN32_SIGNED, REC_BIN32_SIGNED, REC_BIN32_SIGNED, instrAndText(SUB_BIN32S_BIN32S_BIN32S)},
    {ITM_MINUS, REC_BIN64_SIGNED, REC_BIN64_SIGNED, REC_BIN64_SIGNED, instrAndText(SUB_BIN64S_BIN64S_BIN64S)},
    {ITM_MINUS, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(SUB_BIN16U_BIN16U_BIN16U)},
    {ITM_MINUS, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(SUB_BIN16U_BIN16U_BIN16U)},
    {ITM_MINUS, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(SUB_BIN16U_BIN16U_BIN16U)},
    {ITM_MINUS, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(SUB_BIN16U_BIN16U_BIN32U)},
    {ITM_MINUS, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(SUB_BIN16U_BIN16U_BIN32U)},
    {ITM_MINUS, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(SUB_BIN16U_BIN16U_BIN32U)},
    {ITM_MINUS, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(SUB_BIN16U_BIN32U_BIN32U)},
    {ITM_MINUS, REC_BIN32_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(SUB_BIN32U_BIN16U_BIN32U)},
    {ITM_MINUS, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(SUB_BIN16U_BIN16U_BIN16U)},
    {ITM_MINUS, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(SUB_BIN16U_BIN16U_BIN32U)},
    {ITM_MINUS, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(SUB_BIN16U_BIN32U_BIN32U)},
    {ITM_MINUS, REC_BIN32_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(SUB_BIN32U_BIN16U_BIN32U)},
    {ITM_MINUS, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(SUB_BIN32U_BIN32U_BIN32U)},
    {ITM_MINUS, REC_FLOAT32, REC_FLOAT32, REC_FLOAT32, instrAndText(SUB_FLOAT32_FLOAT32_FLOAT32)},
    {ITM_MINUS, REC_FLOAT64, REC_FLOAT64, REC_FLOAT64, instrAndText(SUB_FLOAT64_FLOAT64_FLOAT64)},
    {ITM_MINUS, REC_DATETIME, REC_BIN16_SIGNED, REC_DATETIME, instrAndText(SUB_DATETIME_INTERVAL_DATETIME)},
    {ITM_MINUS, REC_DATETIME, REC_BIN32_SIGNED, REC_DATETIME, instrAndText(SUB_DATETIME_INTERVAL_DATETIME)},
    {ITM_MINUS, REC_DATETIME, REC_BIN64_SIGNED, REC_DATETIME, instrAndText(SUB_DATETIME_INTERVAL_DATETIME)},
    {ITM_MINUS, REC_DATETIME, REC_DATETIME, REC_BIN16_SIGNED, instrAndText(SUB_DATETIME_DATETIME_INTERVAL)},
    {ITM_MINUS, REC_DATETIME, REC_DATETIME, REC_BIN32_SIGNED, instrAndText(SUB_DATETIME_DATETIME_INTERVAL)},
    {ITM_MINUS, REC_DATETIME, REC_DATETIME, REC_BIN64_SIGNED, instrAndText(SUB_DATETIME_DATETIME_INTERVAL)},
    {ITM_MINUS, REC_UNKNOWN, REC_UNKNOWN, REC_UNKNOWN,  instrAndText(SUB_COMPLEX)},


    {ITM_TIMES, REC_BIN16_SIGNED, REC_BIN16_SIGNED, REC_BIN16_SIGNED, instrAndText(MUL_BIN16S_BIN16S_BIN16S)},
    {ITM_TIMES, REC_BIN16_SIGNED, REC_BIN16_SIGNED, REC_BIN32_SIGNED, instrAndText(MUL_BIN16S_BIN16S_BIN32S)},
    {ITM_TIMES, REC_BIN16_SIGNED, REC_BIN32_SIGNED, REC_BIN32_SIGNED, instrAndText(MUL_BIN16S_BIN32S_BIN32S)},
    {ITM_TIMES, REC_BIN32_SIGNED, REC_BIN16_SIGNED, REC_BIN32_SIGNED, instrAndText(MUL_BIN32S_BIN16S_BIN32S)},
    {ITM_TIMES, REC_BIN32_SIGNED, REC_BIN32_SIGNED, REC_BIN32_SIGNED, instrAndText(MUL_BIN32S_BIN32S_BIN32S)},
    {ITM_TIMES, REC_BIN64_SIGNED, REC_BIN64_SIGNED, REC_BIN64_SIGNED, instrAndText(MUL_BIN64S_BIN64S_BIN64S)},
    {ITM_TIMES, REC_BIN16_SIGNED, REC_BIN32_SIGNED, REC_BIN64_SIGNED, instrAndText(MUL_BIN16S_BIN32S_BIN64S)},
    {ITM_TIMES, REC_BIN32_SIGNED, REC_BIN16_SIGNED, REC_BIN64_SIGNED, instrAndText(MUL_BIN32S_BIN16S_BIN64S)},
    {ITM_TIMES, REC_BIN32_SIGNED, REC_BIN32_SIGNED, REC_BIN64_SIGNED, instrAndText(MUL_BIN32S_BIN32S_BIN64S)},
    {ITM_TIMES, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(MUL_BIN16U_BIN16U_BIN16U)},      
    {ITM_TIMES, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(MUL_BIN16U_BIN16U_BIN16U)},
    {ITM_TIMES, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(MUL_BIN16U_BIN16U_BIN16U)},
    {ITM_TIMES, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(MUL_BIN16U_BIN16U_BIN32U)},
    {ITM_TIMES, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(MUL_BIN16U_BIN16U_BIN32U)},
    {ITM_TIMES, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(MUL_BIN16U_BIN16U_BIN32U)},
    {ITM_TIMES, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(MUL_BIN16U_BIN32U_BIN32U)},
    {ITM_TIMES, REC_BIN32_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(MUL_BIN32U_BIN16U_BIN32U)},
    {ITM_TIMES, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(MUL_BIN16U_BIN16U_BIN16U)},
    {ITM_TIMES, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(MUL_BIN16U_BIN16U_BIN32U)},
    {ITM_TIMES, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(MUL_BIN16U_BIN32U_BIN32U)},
    {ITM_TIMES, REC_BIN32_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(MUL_BIN32U_BIN16U_BIN32U)},
    {ITM_TIMES, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(MUL_BIN32U_BIN32U_BIN32U)},
    {ITM_TIMES, REC_FLOAT32, REC_FLOAT32, REC_FLOAT32, instrAndText(MUL_FLOAT32_FLOAT32_FLOAT32)},
    {ITM_TIMES, REC_FLOAT64, REC_FLOAT64, REC_FLOAT64, instrAndText(MUL_FLOAT64_FLOAT64_FLOAT64)},
    {ITM_TIMES, REC_UNKNOWN, REC_UNKNOWN, REC_UNKNOWN,  instrAndText(MUL_COMPLEX)},


    {ITM_DIVIDE, REC_BIN16_SIGNED, REC_BIN16_SIGNED, REC_BIN16_SIGNED, instrAndText(DIV_BIN16S_BIN16S_BIN16S)},
    {ITM_DIVIDE, REC_BIN16_SIGNED, REC_BIN16_SIGNED, REC_BIN32_SIGNED, instrAndText(DIV_BIN16S_BIN16S_BIN32S)},
    {ITM_DIVIDE, REC_BIN16_SIGNED, REC_BIN32_SIGNED, REC_BIN32_SIGNED, instrAndText(DIV_BIN16S_BIN32S_BIN32S)},
    {ITM_DIVIDE, REC_BIN32_SIGNED, REC_BIN16_SIGNED, REC_BIN32_SIGNED, instrAndText(DIV_BIN32S_BIN16S_BIN32S)},
    {ITM_DIVIDE, REC_BIN32_SIGNED, REC_BIN32_SIGNED, REC_BIN32_SIGNED, instrAndText(DIV_BIN32S_BIN32S_BIN32S)},
    {ITM_DIVIDE, REC_BIN64_SIGNED, REC_BIN64_SIGNED, REC_BIN64_SIGNED, instrAndText(DIV_BIN64S_BIN64S_BIN64S)},
    {ITM_DIVIDE, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(DIV_BIN16U_BIN16U_BIN16U)},
    {ITM_DIVIDE, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(DIV_BIN16U_BIN16U_BIN16U)},
    {ITM_DIVIDE, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(DIV_BIN16U_BIN16U_BIN16U)},
    {ITM_DIVIDE, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(DIV_BIN16U_BIN16U_BIN32U)},
    {ITM_DIVIDE, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(DIV_BIN16U_BIN16U_BIN32U)},
    {ITM_DIVIDE, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(DIV_BIN16U_BIN16U_BIN32U)},
    {ITM_DIVIDE, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(DIV_BIN16U_BIN32U_BIN32U)},
    {ITM_DIVIDE, REC_BIN32_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(DIV_BIN32U_BIN16U_BIN32U)},
    {ITM_DIVIDE, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(DIV_BIN16U_BIN16U_BIN16U)},
    {ITM_DIVIDE, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(DIV_BIN16U_BIN16U_BIN32U)},
    {ITM_DIVIDE, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(DIV_BIN16U_BIN32U_BIN32U)},
    {ITM_DIVIDE, REC_BIN32_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(DIV_BIN32U_BIN16U_BIN32U)},
    {ITM_DIVIDE, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(DIV_BIN32U_BIN32U_BIN32U)},
    {ITM_DIVIDE, REC_FLOAT64, REC_FLOAT64, REC_FLOAT64,  instrAndText(DIV_FLOAT64_FLOAT64_FLOAT64)},
    {ITM_DIVIDE, REC_UNKNOWN, REC_UNKNOWN, REC_UNKNOWN,  instrAndText(DIV_COMPLEX)},

    {ITM_DIVIDE, REC_BIN64_SIGNED, REC_BIN64_SIGNED, REC_BIN64_SIGNED,  instrAndText(DIV_BIN64S_BIN64S_BIN64S_ROUND)},

    {ITM_NEGATE, REC_BOOLEAN, REC_BOOLEAN, REC_BOOLEAN, instrAndText(NEGATE_BOOLEAN)},

  };

const ex_arith_clause::ArithInstrStruct * ex_arith_clause::getMatchingRow(OperatorTypeEnum op,
                                                                         short datatype1,
                                                                         short datatype2,
                                                                         short resulttype)
{
  Int32 max_array_size = sizeof(arithInstrInfo) / sizeof(ArithInstrStruct);

  Int32 i = 0;
  Int32 found = 0;
  while ((i < max_array_size) && (!found))
    if ((arithInstrInfo[i].op == op) && 
	(arithInstrInfo[i].type_op1 == datatype1) &&
	(arithInstrInfo[i].type_op2 == datatype2) &&
	(arithInstrInfo[i].type_op0 == resulttype))
      found = -1;
    else
      i++;
  
  if (found)
    {
      setInstrArrayIndex(i);
      return &arithInstrInfo[i];  
    }
  else
    return 0;
}

Lng32 ex_arith_clause::findIndexIntoInstrArray(ArithInstruction ci)
{
  Int32 max_array_size = sizeof(arithInstrInfo) / sizeof(ArithInstrStruct);
  
  Int32 i = 0;
  while ((i < max_array_size) && 
         (arithInstrInfo[i].instruction != ci)) 
    {
      i++;
    }

  if (i < max_array_size)
    return i;

  return -1; // not found
}

const ArithInstruction ex_arith_clause::computeCaseIndex(OperatorTypeEnum op, 
                                                         Attributes * attr1,
                                                         Attributes * attr2,
                                                         Attributes * result)
{
  ArithInstruction instruction = ARITH_NOT_SUPPORTED;

  short type_op1 = -1;
  short type_op2 = -1;
  short type_result = -1;
  getCaseDatatypes(attr1->getDatatype(), attr1->getLength(), type_op1,
                   result->getDatatype(), result->getLength(), type_result,
                   0 /* don't need to take scale difference into account here */);

// attr2 eill be null for unary operands.
// Some methods below expect 2 operands. Change oper2 to be the same as oper1
// to avoid null pointer exception.
  if (!attr2)
    attr2 = attr1;
  getCaseDatatypes(attr2->getDatatype(), attr2->getLength(), type_op2,
                   result->getDatatype(), result->getLength(), type_result,
                   0 /* don't need to take scale difference into account here */);
  
  const ArithInstrStruct * as = getMatchingRow(op, 
                                               type_op1,
                                               type_op2,
                                               type_result);
  
  if (as)
    {
      instruction = as->instruction;

      // for binary numeric and interval datatypes, arith done only if scales
      // are the same and operation is addition or subtraction
      if (((op == ITM_PLUS) || (op == ITM_MINUS)) &&
	  (((result->getDatatype() >= REC_MIN_BINARY_NUMERIC) &&
	    (result->getDatatype() <= REC_MAX_BINARY_NUMERIC)) ||
	   ((result->getDatatype() >= REC_MIN_INTERVAL) &&
	    (result->getDatatype() <= REC_MAX_INTERVAL))))
	{
	  // numeric or interval types
	  if (attr1->getScale() != attr2->getScale())
            {
              instruction = ARITH_NOT_SUPPORTED; 
              setInstrArrayIndex(-1);
            }
	  
	} // numeric or interval types
      else if ((op == ITM_DIVIDE) &&
	       (arithRoundingMode_ != 0))
	{
	  if (instruction == DIV_BIN64S_BIN64S_BIN64S)
            {
              instruction = DIV_BIN64S_BIN64S_BIN64S_ROUND;
              setInstrArrayIndex(findIndexIntoInstrArray(instruction));
             }
	  else
            {
              instruction = ARITH_NOT_SUPPORTED;
              setInstrArrayIndex(-1);
            }
	}
    }

  if (instruction == ARITH_NOT_SUPPORTED)
    setInstrArrayIndex(-1);

  return instruction;
}

void ex_arith_clause::setInstruction()
{
  ArithInstruction instruction = ARITH_NOT_SUPPORTED;
  setInstrArrayIndex(-1);

  if (getOperand(0)->isComplexType())
    {
      switch (getOperType())
	{
	case ITM_PLUS:      
	  instruction = ADD_COMPLEX;
	  break;
	
	case ITM_MINUS:      
	  instruction = SUB_COMPLEX;
	  break;

  	case ITM_TIMES:      
	  instruction = MUL_COMPLEX;
	  break;

  	case ITM_DIVIDE:      
	  instruction = DIV_COMPLEX;
	  break;

	default:
	  break;
	}

      setInstrArrayIndex(findIndexIntoInstrArray(instruction));
    }      
  else
    {
      // Simple types are handled here.
      instruction = computeCaseIndex(getOperType(), 
                                    getOperand(1), 
                                    (getNumOperands() == 3 ? getOperand(2) : NULL),
				    getOperand(0));
    }

  if ((instruction == ARITH_NOT_SUPPORTED) &&
      (getInstrArrayIndex() >= 0))
    {
      // this is an error, reset instr array index
      setInstrArrayIndex(-1);
    }
}

void ex_arith_clause::setInstruction(OperatorTypeEnum op,
				     Attributes * attr1,
				     Attributes * attr2,
				     Attributes * result)
{
  // Only simple types are handled here.
  ArithInstruction instruction = computeCaseIndex(op, attr1, attr2, result);

  if ((instruction == ARITH_NOT_SUPPORTED) &&
      (getInstrArrayIndex() >= 0))
    {
      // this is an error, reset instr array index
      setInstrArrayIndex(-1);
    }
}

short ex_arith_clause::isArithSupported(OperatorTypeEnum op,
					Attributes * attr1,
					Attributes * attr2,
					Attributes * result)
{
  if (computeCaseIndex(op, attr1, attr2, result) 
      == ARITH_NOT_SUPPORTED)
    return 0;
  else
    return -1;
}

ex_expr::exp_return_type ex_arith_clause::fixup(Space * space,
						CollHeap * exHeap,
						char * constants_area,
						char * temps_area,
						char * persistentArea,
						short fixupConstsAndTemps,
						NABoolean spaceCompOnly)
{
  setInstruction();
  return ex_clause::fixup(space, exHeap, constants_area, temps_area,
			  persistentArea,
			  fixupConstsAndTemps, spaceCompOnly);
}

const ex_comp_clause::CompInstrStruct ex_comp_clause::compInstrInfo[] =
  {
    // op       op1 datatype        op2 datatype        instruction+str

    // ITM_ANY_COMP entries are generic entries needed to get the corresponding
    // instructions added to this array.
    {ITM_ANY_COMP, REC_UNKNOWN,     REC_UNKNOWN,        instrAndText(ASCII_COMP)},
    {ITM_ANY_COMP, REC_UNKNOWN,     REC_UNKNOWN,        instrAndText(COMP_COMPLEX)},

    {ITM_ANY_COMP, REC_UNKNOWN,     REC_UNKNOWN,        instrAndText(BINARY_COMP)},

    {ITM_EQUAL, REC_BIN8_SIGNED,    REC_BIN8_SIGNED,    instrAndText(EQ_BIN8S_BIN8S)},
    {ITM_EQUAL, REC_BIN8_UNSIGNED,  REC_BIN8_UNSIGNED,  instrAndText(EQ_BIN8U_BIN8U)},

    {ITM_EQUAL, REC_BIN16_SIGNED,   REC_BIN16_SIGNED,   instrAndText(EQ_BIN16S_BIN16S)},
    {ITM_EQUAL, REC_BIN16_SIGNED,   REC_BIN32_SIGNED,   instrAndText(EQ_BIN16S_BIN32S)},
    {ITM_EQUAL, REC_BIN16_SIGNED,   REC_BPINT_UNSIGNED, instrAndText(EQ_BIN16S_BIN16U)},
    {ITM_EQUAL, REC_BIN16_SIGNED,   REC_BIN16_UNSIGNED, instrAndText(EQ_BIN16S_BIN16U)},
    {ITM_EQUAL, REC_BIN16_SIGNED,   REC_BIN32_UNSIGNED, instrAndText(EQ_BIN16S_BIN32U)}, 

    {ITM_EQUAL, REC_BPINT_UNSIGNED, REC_BIN16_SIGNED,   instrAndText(EQ_BIN16U_BIN16S)},
    {ITM_EQUAL, REC_BPINT_UNSIGNED, REC_BIN32_SIGNED,   instrAndText(EQ_BIN16U_BIN32S)},
    {ITM_EQUAL, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, instrAndText(EQ_BIN16U_BIN16U)},
    {ITM_EQUAL, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(EQ_BIN16U_BIN16U)},
    {ITM_EQUAL, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(EQ_BIN16U_BIN32U)},

    {ITM_EQUAL, REC_BIN16_UNSIGNED, REC_BIN16_SIGNED,   instrAndText(EQ_BIN16U_BIN16S)},
    {ITM_EQUAL, REC_BIN16_UNSIGNED, REC_BIN32_SIGNED,   instrAndText(EQ_BIN16U_BIN32S)},
    {ITM_EQUAL, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, instrAndText(EQ_BIN16U_BIN16U)},
    {ITM_EQUAL, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(EQ_BIN16U_BIN16U)},
    {ITM_EQUAL, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(EQ_BIN16U_BIN32U)},    // Was BIN32S. Error? ANS

    {ITM_EQUAL, REC_BIN32_SIGNED,   REC_BIN16_SIGNED,   instrAndText(EQ_BIN32S_BIN16S)},
    {ITM_EQUAL, REC_BIN32_SIGNED,   REC_BIN32_SIGNED,   instrAndText(EQ_BIN32S_BIN32S)},
    {ITM_EQUAL, REC_BIN32_SIGNED,   REC_BPINT_UNSIGNED, instrAndText(EQ_BIN32S_BIN16U)},
    {ITM_EQUAL, REC_BIN32_SIGNED,   REC_BIN16_UNSIGNED, instrAndText(EQ_BIN32S_BIN16U)},
    {ITM_EQUAL, REC_BIN32_SIGNED,   REC_BIN32_UNSIGNED, instrAndText(EQ_BIN32S_BIN32U)},    // Was BIN32S. Error? ANS

    {ITM_EQUAL, REC_BIN32_UNSIGNED, REC_BIN16_SIGNED,   instrAndText(EQ_BIN32U_BIN16S)},
    {ITM_EQUAL, REC_BIN32_UNSIGNED, REC_BIN32_SIGNED,   instrAndText(EQ_BIN32U_BIN32S)},
    {ITM_EQUAL, REC_BIN32_UNSIGNED, REC_BPINT_UNSIGNED, instrAndText(EQ_BIN32U_BIN16U)},
    {ITM_EQUAL, REC_BIN32_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(EQ_BIN32U_BIN16U)},
    {ITM_EQUAL, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(EQ_BIN32U_BIN32U)},    // Was BIN32S. Error? ANS

    {ITM_EQUAL, REC_BIN64_SIGNED,   REC_BIN64_SIGNED,     instrAndText(EQ_BIN64S_BIN64S)},
    {ITM_EQUAL, REC_BIN64_UNSIGNED, REC_BIN64_UNSIGNED,   instrAndText(EQ_BIN64U_BIN64U)},
    {ITM_EQUAL, REC_BIN64_SIGNED,   REC_BIN64_UNSIGNED,   instrAndText(EQ_BIN64S_BIN64U)},
    {ITM_EQUAL, REC_BIN64_UNSIGNED, REC_BIN64_SIGNED,     instrAndText(EQ_BIN64U_BIN64S)},

    {ITM_EQUAL, REC_DECIMAL_UNSIGNED, REC_DECIMAL_UNSIGNED, instrAndText(EQ_DECU_DECU)},
    {ITM_EQUAL, REC_DECIMAL_LSE,      REC_DECIMAL_LSE,      instrAndText(EQ_DECS_DECS)},

    {ITM_EQUAL, REC_FLOAT32,        REC_FLOAT32,        instrAndText(EQ_FLOAT32_FLOAT32)},
    {ITM_EQUAL, REC_FLOAT64,        REC_FLOAT64,        instrAndText(EQ_FLOAT64_FLOAT64)},

    {ITM_EQUAL, REC_DATETIME,       REC_DATETIME,       instrAndText(EQ_DATETIME_DATETIME)},

    {ITM_EQUAL, REC_BYTE_F_ASCII,     REC_BYTE_F_ASCII,     instrAndText(EQ_ASCII_F_F)},

    {ITM_EQUAL, REC_BYTE_F_ASCII,       REC_BYTE_V_ASCII,       instrAndText(EQ_ASCII_COMP)},
    {ITM_EQUAL, REC_BYTE_V_ASCII,       REC_BYTE_F_ASCII,       instrAndText(EQ_ASCII_COMP)},
    {ITM_EQUAL, REC_BYTE_V_ASCII,       REC_BYTE_V_ASCII,       instrAndText(EQ_ASCII_COMP)},
    {ITM_EQUAL, REC_BYTE_F_ASCII,       REC_BYTE_V_ASCII_LONG,  instrAndText(EQ_ASCII_COMP)},
    {ITM_EQUAL, REC_BYTE_V_ASCII_LONG,  REC_BYTE_F_ASCII,       instrAndText(EQ_ASCII_COMP)},
    {ITM_EQUAL, REC_BYTE_V_ASCII,       REC_BYTE_V_ASCII_LONG,  instrAndText(EQ_ASCII_COMP)},
    {ITM_EQUAL, REC_BYTE_V_ASCII_LONG,  REC_BYTE_V_ASCII,       instrAndText(EQ_ASCII_COMP)},
    {ITM_EQUAL, REC_BYTE_V_ASCII_LONG,  REC_BYTE_V_ASCII_LONG,  instrAndText(EQ_ASCII_COMP)},

    {ITM_EQUAL,REC_NCHAR_F_UNICODE,REC_NCHAR_F_UNICODE,instrAndText(EQ_UNICODE_F_F)},
    {ITM_EQUAL, REC_NCHAR_F_UNICODE,REC_NCHAR_V_UNICODE,instrAndText(UNICODE_COMP)},
    {ITM_EQUAL, REC_NCHAR_V_UNICODE,REC_NCHAR_F_UNICODE,instrAndText(UNICODE_COMP)},
    {ITM_EQUAL, REC_NCHAR_V_UNICODE,REC_NCHAR_V_UNICODE,instrAndText(UNICODE_COMP)},
      
    {ITM_EQUAL, REC_BLOB,REC_BLOB,instrAndText(EQ_BLOB)},

    {ITM_EQUAL, REC_BOOLEAN, REC_BOOLEAN, instrAndText(EQ_BOOL_BOOL)},

    {ITM_NOT_EQUAL, REC_BIN8_SIGNED,    REC_BIN8_SIGNED,    instrAndText(NE_BIN8S_BIN8S)},
    {ITM_NOT_EQUAL, REC_BIN8_UNSIGNED,  REC_BIN8_UNSIGNED,  instrAndText(NE_BIN8U_BIN8U)},
      
    {ITM_NOT_EQUAL, REC_BIN16_SIGNED,   REC_BIN16_SIGNED,   instrAndText(NE_BIN16S_BIN16S)},
    {ITM_NOT_EQUAL, REC_BIN16_SIGNED,   REC_BIN32_SIGNED,   instrAndText(NE_BIN16S_BIN32S)},
    {ITM_NOT_EQUAL, REC_BIN16_SIGNED,   REC_BPINT_UNSIGNED, instrAndText(NE_BIN16S_BIN16U)},
    {ITM_NOT_EQUAL, REC_BIN16_SIGNED,   REC_BIN16_UNSIGNED, instrAndText(NE_BIN16S_BIN16U)},
    {ITM_NOT_EQUAL, REC_BIN16_SIGNED,   REC_BIN32_UNSIGNED, instrAndText(NE_BIN16S_BIN32U)}, //Was 32S. Error? ANS

    {ITM_NOT_EQUAL, REC_BPINT_UNSIGNED, REC_BIN16_SIGNED,   instrAndText(NE_BIN16U_BIN16S)},
    {ITM_NOT_EQUAL, REC_BPINT_UNSIGNED, REC_BIN32_SIGNED,   instrAndText(NE_BIN16U_BIN32S)},
    {ITM_NOT_EQUAL, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, instrAndText(NE_BIN16U_BIN16U)},
    {ITM_NOT_EQUAL, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(NE_BIN16U_BIN16U)},
    {ITM_NOT_EQUAL, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(NE_BIN16U_BIN32U)},  //Was 32S. Error? ANS

    {ITM_NOT_EQUAL, REC_BIN16_UNSIGNED, REC_BIN16_SIGNED,   instrAndText(NE_BIN16U_BIN16S)},
    {ITM_NOT_EQUAL, REC_BIN16_UNSIGNED, REC_BIN32_SIGNED,   instrAndText(NE_BIN16U_BIN32S)},
    {ITM_NOT_EQUAL, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, instrAndText(NE_BIN16U_BIN16U)},
    {ITM_NOT_EQUAL, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(NE_BIN16U_BIN16U)},
    {ITM_NOT_EQUAL, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(NE_BIN16U_BIN32U)},  //Was 32S. Error? ANS

    {ITM_NOT_EQUAL, REC_BIN32_SIGNED,   REC_BIN16_SIGNED,   instrAndText(NE_BIN32S_BIN16S)},
    {ITM_NOT_EQUAL, REC_BIN32_SIGNED,   REC_BIN32_SIGNED,   instrAndText(NE_BIN32S_BIN32S)},
    {ITM_NOT_EQUAL, REC_BIN32_SIGNED,   REC_BPINT_UNSIGNED, instrAndText(NE_BIN32S_BIN16U)},
    {ITM_NOT_EQUAL, REC_BIN32_SIGNED,   REC_BIN16_UNSIGNED, instrAndText(NE_BIN32S_BIN16U)},
    {ITM_NOT_EQUAL, REC_BIN32_SIGNED,   REC_BIN32_UNSIGNED, instrAndText(NE_BIN32S_BIN32U)},  //Was 32S. Error? ANS

    {ITM_NOT_EQUAL, REC_BIN32_UNSIGNED, REC_BIN16_SIGNED,   instrAndText(NE_BIN32U_BIN16S)},
    {ITM_NOT_EQUAL, REC_BIN32_UNSIGNED, REC_BIN32_SIGNED,   instrAndText(NE_BIN32U_BIN32S)},
    {ITM_NOT_EQUAL, REC_BIN32_UNSIGNED, REC_BPINT_UNSIGNED, instrAndText(NE_BIN32U_BIN16U)},
    {ITM_NOT_EQUAL, REC_BIN32_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(NE_BIN32U_BIN16U)},
    {ITM_NOT_EQUAL, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(NE_BIN32U_BIN32U)},  //Was 32S. Error? ANS

    {ITM_NOT_EQUAL, REC_BIN64_SIGNED,   REC_BIN64_SIGNED,   instrAndText(NE_BIN64S_BIN64S)},
    {ITM_NOT_EQUAL, REC_BIN64_UNSIGNED, REC_BIN64_UNSIGNED, instrAndText(NE_BIN64U_BIN64U)},
    {ITM_NOT_EQUAL, REC_BIN64_SIGNED,   REC_BIN64_UNSIGNED, instrAndText(NE_BIN64S_BIN64U)},
    {ITM_NOT_EQUAL, REC_BIN64_UNSIGNED, REC_BIN64_SIGNED,   instrAndText(NE_BIN64U_BIN64S)},

    {ITM_NOT_EQUAL, REC_DECIMAL_UNSIGNED, REC_DECIMAL_UNSIGNED, instrAndText(NE_DECU_DECU)},
    {ITM_NOT_EQUAL, REC_DECIMAL_LSE,      REC_DECIMAL_LSE,      instrAndText(NE_DECS_DECS)},

    {ITM_NOT_EQUAL, REC_FLOAT32,        REC_FLOAT32,        instrAndText(NE_FLOAT32_FLOAT32)},
    {ITM_NOT_EQUAL, REC_FLOAT64,        REC_FLOAT64,        instrAndText(NE_FLOAT64_FLOAT64)},

    {ITM_NOT_EQUAL, REC_DATETIME,       REC_DATETIME,       instrAndText(NE_DATETIME_DATETIME)},

    {ITM_NOT_EQUAL, REC_BYTE_F_ASCII,     REC_BYTE_F_ASCII,     instrAndText(NE_ASCII_F_F)},

    {ITM_NOT_EQUAL, REC_BYTE_F_ASCII,      REC_BYTE_V_ASCII,      instrAndText(NE_ASCII_COMP)},
    {ITM_NOT_EQUAL, REC_BYTE_V_ASCII,      REC_BYTE_F_ASCII,      instrAndText(NE_ASCII_COMP)},
    {ITM_NOT_EQUAL, REC_BYTE_V_ASCII,      REC_BYTE_V_ASCII,      instrAndText(NE_ASCII_COMP)},
    {ITM_NOT_EQUAL, REC_BYTE_F_ASCII,      REC_BYTE_V_ASCII_LONG, instrAndText(NE_ASCII_COMP)},
    {ITM_NOT_EQUAL, REC_BYTE_V_ASCII_LONG, REC_BYTE_F_ASCII,      instrAndText(NE_ASCII_COMP)},
    {ITM_NOT_EQUAL, REC_BYTE_V_ASCII,      REC_BYTE_V_ASCII_LONG, instrAndText(NE_ASCII_COMP)},
    {ITM_NOT_EQUAL, REC_BYTE_V_ASCII_LONG, REC_BYTE_V_ASCII,      instrAndText(NE_ASCII_COMP)},
    {ITM_NOT_EQUAL, REC_BYTE_V_ASCII_LONG, REC_BYTE_V_ASCII_LONG, instrAndText(NE_ASCII_COMP)},

    {ITM_NOT_EQUAL,REC_NCHAR_F_UNICODE,REC_NCHAR_F_UNICODE,instrAndText(NE_UNICODE_F_F)},
    {ITM_NOT_EQUAL, REC_NCHAR_F_UNICODE,      REC_NCHAR_V_UNICODE,      instrAndText(UNICODE_COMP)},
    {ITM_NOT_EQUAL, REC_NCHAR_V_UNICODE,      REC_NCHAR_F_UNICODE,      instrAndText(UNICODE_COMP)},
    {ITM_NOT_EQUAL, REC_NCHAR_V_UNICODE,      REC_NCHAR_V_UNICODE,      instrAndText(UNICODE_COMP)},

    {ITM_NOT_EQUAL, REC_BOOLEAN, REC_BOOLEAN, instrAndText(NE_BOOL_BOOL)},

    {ITM_LESS, REC_BIN8_SIGNED,    REC_BIN8_SIGNED,    instrAndText(LT_BIN8S_BIN8S)},
    {ITM_LESS, REC_BIN8_UNSIGNED,  REC_BIN8_UNSIGNED,  instrAndText(LT_BIN8U_BIN8U)},

    {ITM_LESS, REC_BIN16_SIGNED,   REC_BIN16_SIGNED,   instrAndText(LT_BIN16S_BIN16S)},
    {ITM_LESS, REC_BIN16_SIGNED,   REC_BIN32_SIGNED,   instrAndText(LT_BIN16S_BIN32S)},
    {ITM_LESS, REC_BIN16_SIGNED,   REC_BPINT_UNSIGNED, instrAndText(LT_BIN16S_BIN16U)},
    {ITM_LESS, REC_BIN16_SIGNED,   REC_BIN16_UNSIGNED, instrAndText(LT_BIN16S_BIN16U)},
    {ITM_LESS, REC_BIN16_SIGNED,   REC_BIN32_UNSIGNED, instrAndText(LT_BIN16S_BIN32U)},   // Was 32S. Error? ANS

    {ITM_LESS, REC_BPINT_UNSIGNED, REC_BIN16_SIGNED,   instrAndText(LT_BIN16U_BIN16S)},
    {ITM_LESS, REC_BPINT_UNSIGNED, REC_BIN32_SIGNED,   instrAndText(LT_BIN16U_BIN32S)},
    {ITM_LESS, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, instrAndText(LT_BIN16U_BIN16U)},
    {ITM_LESS, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(LT_BIN16U_BIN16U)},
    {ITM_LESS, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(LT_BIN16U_BIN32U)},   // Was 32S. Error? ANS

    {ITM_LESS, REC_BIN16_UNSIGNED, REC_BIN16_SIGNED,   instrAndText(LT_BIN16U_BIN16S)},
    {ITM_LESS, REC_BIN16_UNSIGNED, REC_BIN32_SIGNED,   instrAndText(LT_BIN16U_BIN32S)},
    {ITM_LESS, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, instrAndText(LT_BIN16U_BIN16U)},
    {ITM_LESS, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(LT_BIN16U_BIN16U)},
    {ITM_LESS, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(LT_BIN16U_BIN32U)},   // Was 32S. Error? ANS

    {ITM_LESS, REC_BIN32_SIGNED,   REC_BIN16_SIGNED,   instrAndText(LT_BIN32S_BIN16S)},
    {ITM_LESS, REC_BIN32_SIGNED,   REC_BIN32_SIGNED,   instrAndText(LT_BIN32S_BIN32S)},
    {ITM_LESS, REC_BIN32_SIGNED,   REC_BPINT_UNSIGNED, instrAndText(LT_BIN32S_BIN16U)},
    {ITM_LESS, REC_BIN32_SIGNED,   REC_BIN16_UNSIGNED, instrAndText(LT_BIN32S_BIN16U)},
    {ITM_LESS, REC_BIN32_SIGNED,   REC_BIN32_UNSIGNED, instrAndText(LT_BIN32S_BIN32U)},   // Was 32S. Error? ANS

    {ITM_LESS, REC_BIN32_UNSIGNED, REC_BIN16_SIGNED,   instrAndText(LT_BIN32U_BIN16S)},
    {ITM_LESS, REC_BIN32_UNSIGNED, REC_BIN32_SIGNED,   instrAndText(LT_BIN32U_BIN32S)},
    {ITM_LESS, REC_BIN32_UNSIGNED, REC_BPINT_UNSIGNED, instrAndText(LT_BIN32U_BIN16U)},
    {ITM_LESS, REC_BIN32_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(LT_BIN32U_BIN16U)},
    {ITM_LESS, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(LT_BIN32U_BIN32U)},   // Was 32S. Error? ANS

    {ITM_LESS, REC_BIN64_SIGNED,   REC_BIN64_SIGNED,   instrAndText(LT_BIN64S_BIN64S)},
    {ITM_LESS, REC_BIN64_UNSIGNED, REC_BIN64_UNSIGNED, instrAndText(LT_BIN64U_BIN64U)},
    {ITM_LESS, REC_BIN64_SIGNED,   REC_BIN64_UNSIGNED,   instrAndText(LT_BIN64S_BIN64U)},
    {ITM_LESS, REC_BIN64_UNSIGNED, REC_BIN64_SIGNED,     instrAndText(LT_BIN64U_BIN64S)},

    {ITM_LESS, REC_DECIMAL_UNSIGNED, REC_DECIMAL_UNSIGNED, instrAndText(LT_DECU_DECU)},
    {ITM_LESS, REC_DECIMAL_LSE,      REC_DECIMAL_LSE,      instrAndText(LT_DECS_DECS)},

    {ITM_LESS, REC_FLOAT32,        REC_FLOAT32,        instrAndText(LT_FLOAT32_FLOAT32)},
    {ITM_LESS, REC_FLOAT64,        REC_FLOAT64,        instrAndText(LT_FLOAT64_FLOAT64)},

    {ITM_LESS, REC_DATETIME,       REC_DATETIME,       instrAndText(LT_DATETIME_DATETIME)},

    {ITM_LESS, REC_BYTE_F_ASCII,     REC_BYTE_F_ASCII,     instrAndText(LT_ASCII_F_F)},

    {ITM_LESS, REC_BYTE_F_ASCII,     REC_BYTE_V_ASCII,     instrAndText(LT_ASCII_COMP)},
    {ITM_LESS, REC_BYTE_V_ASCII,     REC_BYTE_F_ASCII,     instrAndText(LT_ASCII_COMP)},
    {ITM_LESS, REC_BYTE_V_ASCII,     REC_BYTE_V_ASCII,     instrAndText(LT_ASCII_COMP)},
    {ITM_LESS, REC_BYTE_F_ASCII,     REC_BYTE_V_ASCII_LONG,     instrAndText(LT_ASCII_COMP)},
    {ITM_LESS, REC_BYTE_V_ASCII_LONG,     REC_BYTE_F_ASCII,     instrAndText(LT_ASCII_COMP)},
    {ITM_LESS, REC_BYTE_V_ASCII,     REC_BYTE_V_ASCII_LONG,     instrAndText(LT_ASCII_COMP)},
    {ITM_LESS, REC_BYTE_V_ASCII_LONG,     REC_BYTE_V_ASCII,     instrAndText(LT_ASCII_COMP)},
    {ITM_LESS, REC_BYTE_V_ASCII_LONG,     REC_BYTE_V_ASCII_LONG,     instrAndText(LT_ASCII_COMP)},
    {ITM_LESS,REC_NCHAR_F_UNICODE,REC_NCHAR_F_UNICODE,instrAndText(LT_UNICODE_F_F)},
    {ITM_LESS, REC_NCHAR_F_UNICODE,     REC_NCHAR_V_UNICODE,     instrAndText(UNICODE_COMP)},
    {ITM_LESS, REC_NCHAR_V_UNICODE,     REC_NCHAR_F_UNICODE,     instrAndText(UNICODE_COMP)},
    {ITM_LESS, REC_NCHAR_V_UNICODE,     REC_NCHAR_V_UNICODE,     instrAndText(UNICODE_COMP)},


    {ITM_LESS_EQ, REC_BIN8_SIGNED,    REC_BIN8_SIGNED,    instrAndText(LE_BIN8S_BIN8S)},
    {ITM_LESS_EQ, REC_BIN8_UNSIGNED,  REC_BIN8_UNSIGNED,  instrAndText(LE_BIN8U_BIN8U)},

    {ITM_LESS_EQ, REC_BIN16_SIGNED,   REC_BIN16_SIGNED,   instrAndText(LE_BIN16S_BIN16S)},
    {ITM_LESS_EQ, REC_BIN16_SIGNED,   REC_BIN32_SIGNED,   instrAndText(LE_BIN16S_BIN32S)},
    {ITM_LESS_EQ, REC_BIN16_SIGNED,   REC_BPINT_UNSIGNED, instrAndText(LE_BIN16S_BIN16U)},
    {ITM_LESS_EQ, REC_BIN16_SIGNED,   REC_BIN16_UNSIGNED, instrAndText(LE_BIN16S_BIN16U)},
    {ITM_LESS_EQ, REC_BIN16_SIGNED,   REC_BIN32_UNSIGNED, instrAndText(LE_BIN16S_BIN32U)},    // Was 32S. Error? ANS

    {ITM_LESS_EQ, REC_BPINT_UNSIGNED, REC_BIN16_SIGNED,   instrAndText(LE_BIN16U_BIN16S)},
    {ITM_LESS_EQ, REC_BPINT_UNSIGNED, REC_BIN32_SIGNED,   instrAndText(LE_BIN16U_BIN32S)},
    {ITM_LESS_EQ, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, instrAndText(LE_BIN16U_BIN16U)},
    {ITM_LESS_EQ, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(LE_BIN16U_BIN16U)},
    {ITM_LESS_EQ, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(LE_BIN16U_BIN32U)},    // Was 32S. Error? ANS

    {ITM_LESS_EQ, REC_BIN16_UNSIGNED, REC_BIN16_SIGNED,   instrAndText(LE_BIN16U_BIN16S)},
    {ITM_LESS_EQ, REC_BIN16_UNSIGNED, REC_BIN32_SIGNED,   instrAndText(LE_BIN16U_BIN32S)},
    {ITM_LESS_EQ, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, instrAndText(LE_BIN16U_BIN16U)},
    {ITM_LESS_EQ, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(LE_BIN16U_BIN16U)},
    {ITM_LESS_EQ, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(LE_BIN16U_BIN32U)},    // Was 32S. Error? ANS

    {ITM_LESS_EQ, REC_BIN32_SIGNED,   REC_BIN16_SIGNED,   instrAndText(LE_BIN32S_BIN16S)},
    {ITM_LESS_EQ, REC_BIN32_SIGNED,   REC_BIN32_SIGNED,   instrAndText(LE_BIN32S_BIN32S)},
    {ITM_LESS_EQ, REC_BIN32_SIGNED,   REC_BPINT_UNSIGNED, instrAndText(LE_BIN32S_BIN16U)},
    {ITM_LESS_EQ, REC_BIN32_SIGNED,   REC_BIN16_UNSIGNED, instrAndText(LE_BIN32S_BIN16U)},
    {ITM_LESS_EQ, REC_BIN32_SIGNED,   REC_BIN32_UNSIGNED, instrAndText(LE_BIN32S_BIN32U)},    // Was 32S. Error? ANS

    {ITM_LESS_EQ, REC_BIN32_UNSIGNED, REC_BIN16_SIGNED,   instrAndText(LE_BIN32U_BIN16S)},
    {ITM_LESS_EQ, REC_BIN32_UNSIGNED, REC_BIN32_SIGNED,   instrAndText(LE_BIN32U_BIN32S)},
    {ITM_LESS_EQ, REC_BIN32_UNSIGNED, REC_BPINT_UNSIGNED, instrAndText(LE_BIN32U_BIN16U)},
    {ITM_LESS_EQ, REC_BIN32_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(LE_BIN32U_BIN16U)},
    {ITM_LESS_EQ, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(LE_BIN32U_BIN32U)},    // Was 32S. Error? ANS

    {ITM_LESS_EQ, REC_BIN64_SIGNED,   REC_BIN64_SIGNED,   instrAndText(LE_BIN64S_BIN64S)},
    {ITM_LESS_EQ, REC_BIN64_UNSIGNED, REC_BIN64_UNSIGNED, instrAndText(LE_BIN64U_BIN64U)},
    {ITM_LESS_EQ, REC_BIN64_SIGNED,   REC_BIN64_UNSIGNED,   instrAndText(LE_BIN64S_BIN64U)},
    {ITM_LESS_EQ, REC_BIN64_UNSIGNED, REC_BIN64_SIGNED,     instrAndText(LE_BIN64U_BIN64S)},

    {ITM_LESS_EQ, REC_DECIMAL_UNSIGNED, REC_DECIMAL_UNSIGNED, instrAndText(LE_DECU_DECU)},
    {ITM_LESS_EQ, REC_DECIMAL_LSE,      REC_DECIMAL_LSE,      instrAndText(LE_DECS_DECS)},

    {ITM_LESS_EQ, REC_FLOAT32,        REC_FLOAT32,        instrAndText(LE_FLOAT32_FLOAT32)},
    {ITM_LESS_EQ, REC_FLOAT64,        REC_FLOAT64,        instrAndText(LE_FLOAT64_FLOAT64)},

    {ITM_LESS_EQ, REC_DATETIME,       REC_DATETIME,       instrAndText(LE_DATETIME_DATETIME)},

    {ITM_LESS_EQ, REC_BYTE_F_ASCII,     REC_BYTE_F_ASCII,     instrAndText(LE_ASCII_F_F)},

    {ITM_LESS_EQ, REC_BYTE_F_ASCII,     REC_BYTE_V_ASCII,     instrAndText(LE_ASCII_COMP)},
    {ITM_LESS_EQ, REC_BYTE_V_ASCII,     REC_BYTE_F_ASCII,     instrAndText(LE_ASCII_COMP)},
    {ITM_LESS_EQ, REC_BYTE_V_ASCII,     REC_BYTE_V_ASCII,     instrAndText(LE_ASCII_COMP)},
    {ITM_LESS_EQ, REC_BYTE_F_ASCII,     REC_BYTE_V_ASCII_LONG,     instrAndText(LE_ASCII_COMP)},
    {ITM_LESS_EQ, REC_BYTE_V_ASCII_LONG,     REC_BYTE_F_ASCII,     instrAndText(LE_ASCII_COMP)},
    {ITM_LESS_EQ, REC_BYTE_V_ASCII,     REC_BYTE_V_ASCII_LONG,     instrAndText(LE_ASCII_COMP)},
    {ITM_LESS_EQ, REC_BYTE_V_ASCII_LONG,     REC_BYTE_V_ASCII,     instrAndText(LE_ASCII_COMP)},
    {ITM_LESS_EQ, REC_BYTE_V_ASCII_LONG,     REC_BYTE_V_ASCII_LONG,     instrAndText(LE_ASCII_COMP)},

    {ITM_LESS_EQ,REC_NCHAR_F_UNICODE,REC_NCHAR_F_UNICODE,instrAndText(LE_UNICODE_F_F)},
    {ITM_LESS_EQ, REC_NCHAR_F_UNICODE,     REC_NCHAR_V_UNICODE,     instrAndText(UNICODE_COMP)},
    {ITM_LESS_EQ, REC_NCHAR_V_UNICODE,     REC_NCHAR_F_UNICODE,     instrAndText(UNICODE_COMP)},
    {ITM_LESS_EQ, REC_NCHAR_V_UNICODE,     REC_NCHAR_V_UNICODE,     instrAndText(UNICODE_COMP)},


    {ITM_GREATER, REC_BIN8_SIGNED,    REC_BIN8_SIGNED,    instrAndText(GT_BIN8S_BIN8S)},
    {ITM_GREATER, REC_BIN8_UNSIGNED,  REC_BIN8_UNSIGNED,  instrAndText(GT_BIN8U_BIN8U)},

    {ITM_GREATER, REC_BIN16_SIGNED,   REC_BIN16_SIGNED,   instrAndText(GT_BIN16S_BIN16S)},
    {ITM_GREATER, REC_BIN16_SIGNED,   REC_BIN32_SIGNED,   instrAndText(GT_BIN16S_BIN32S)},
    {ITM_GREATER, REC_BIN16_SIGNED,   REC_BPINT_UNSIGNED, instrAndText(GT_BIN16S_BIN16U)},
    {ITM_GREATER, REC_BIN16_SIGNED,   REC_BIN16_UNSIGNED, instrAndText(GT_BIN16S_BIN16U)},
    {ITM_GREATER, REC_BIN16_SIGNED,   REC_BIN32_UNSIGNED, instrAndText(GT_BIN16S_BIN32U)},    // Was 32S. Error? ANS

    {ITM_GREATER, REC_BPINT_UNSIGNED, REC_BIN16_SIGNED,   instrAndText(GT_BIN16U_BIN16S)},
    {ITM_GREATER, REC_BPINT_UNSIGNED, REC_BIN32_SIGNED,   instrAndText(GT_BIN16U_BIN32S)},
    {ITM_GREATER, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, instrAndText(GT_BIN16U_BIN16U)},
    {ITM_GREATER, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(GT_BIN16U_BIN16U)},
    {ITM_GREATER, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(GT_BIN16U_BIN32U)},    // Was 32S. Error? ANS

    {ITM_GREATER, REC_BIN16_UNSIGNED, REC_BIN16_SIGNED,   instrAndText(GT_BIN16U_BIN16S)},
    {ITM_GREATER, REC_BIN16_UNSIGNED, REC_BIN32_SIGNED,   instrAndText(GT_BIN16U_BIN32S)},
    {ITM_GREATER, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, instrAndText(GT_BIN16U_BIN16U)},
    {ITM_GREATER, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(GT_BIN16U_BIN16U)},
    {ITM_GREATER, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(GT_BIN16U_BIN32U)},    // Was 32S. Error? ANS

    {ITM_GREATER, REC_BIN32_SIGNED,   REC_BIN16_SIGNED,   instrAndText(GT_BIN32S_BIN16S)},
    {ITM_GREATER, REC_BIN32_SIGNED,   REC_BIN32_SIGNED,   instrAndText(GT_BIN32S_BIN32S)},
    {ITM_GREATER, REC_BIN32_SIGNED,   REC_BPINT_UNSIGNED, instrAndText(GT_BIN32S_BIN16U)},
    {ITM_GREATER, REC_BIN32_SIGNED,   REC_BIN16_UNSIGNED, instrAndText(GT_BIN32S_BIN16U)},
    {ITM_GREATER, REC_BIN32_SIGNED,   REC_BIN32_UNSIGNED, instrAndText(GT_BIN32S_BIN32U)},    // Was 32S. Error? ANS

    {ITM_GREATER, REC_BIN32_UNSIGNED, REC_BIN16_SIGNED,   instrAndText(GT_BIN32U_BIN16S)},
    {ITM_GREATER, REC_BIN32_UNSIGNED, REC_BIN32_SIGNED,   instrAndText(GT_BIN32U_BIN32S)},
    {ITM_GREATER, REC_BIN32_UNSIGNED, REC_BPINT_UNSIGNED, instrAndText(GT_BIN32U_BIN16U)},
    {ITM_GREATER, REC_BIN32_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(GT_BIN32U_BIN16U)},
    {ITM_GREATER, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(GT_BIN32U_BIN32U)},    // Was 32S. Error? ANS

    {ITM_GREATER, REC_BIN64_SIGNED,   REC_BIN64_SIGNED,   instrAndText(GT_BIN64S_BIN64S)},
    {ITM_GREATER, REC_BIN64_UNSIGNED, REC_BIN64_UNSIGNED, instrAndText(GT_BIN64U_BIN64U)},
    {ITM_GREATER, REC_BIN64_SIGNED,   REC_BIN64_UNSIGNED,   instrAndText(GT_BIN64S_BIN64U)},
    {ITM_GREATER, REC_BIN64_UNSIGNED, REC_BIN64_SIGNED,     instrAndText(GT_BIN64U_BIN64S)},

    {ITM_GREATER, REC_DECIMAL_UNSIGNED, REC_DECIMAL_UNSIGNED, instrAndText(GT_DECU_DECU)},
    {ITM_GREATER, REC_DECIMAL_LSE,      REC_DECIMAL_LSE,      instrAndText(GT_DECS_DECS)},

    {ITM_GREATER, REC_FLOAT32,        REC_FLOAT32,        instrAndText(GT_FLOAT32_FLOAT32)},
    {ITM_GREATER, REC_FLOAT64,        REC_FLOAT64,        instrAndText(GT_FLOAT64_FLOAT64)},

    {ITM_GREATER, REC_DATETIME,       REC_DATETIME,       instrAndText(GT_DATETIME_DATETIME)},

    {ITM_GREATER, REC_BYTE_F_ASCII,     REC_BYTE_F_ASCII,     instrAndText(GT_ASCII_F_F)},

    {ITM_GREATER, REC_BYTE_F_ASCII,     REC_BYTE_V_ASCII,     instrAndText(GT_ASCII_COMP)},
    {ITM_GREATER, REC_BYTE_V_ASCII,     REC_BYTE_F_ASCII,     instrAndText(GT_ASCII_COMP)},
    {ITM_GREATER, REC_BYTE_V_ASCII,     REC_BYTE_V_ASCII,     instrAndText(GT_ASCII_COMP)},
    {ITM_GREATER, REC_BYTE_F_ASCII,     REC_BYTE_V_ASCII_LONG,     instrAndText(GT_ASCII_COMP)},
    {ITM_GREATER, REC_BYTE_V_ASCII_LONG,     REC_BYTE_F_ASCII,     instrAndText(GT_ASCII_COMP)},
    {ITM_GREATER, REC_BYTE_V_ASCII,     REC_BYTE_V_ASCII_LONG,     instrAndText(GT_ASCII_COMP)},
    {ITM_GREATER, REC_BYTE_V_ASCII_LONG,     REC_BYTE_V_ASCII,     instrAndText(GT_ASCII_COMP)},
    {ITM_GREATER, REC_BYTE_V_ASCII_LONG,     REC_BYTE_V_ASCII_LONG,     instrAndText(GT_ASCII_COMP)},

    {ITM_GREATER,REC_NCHAR_F_UNICODE,REC_NCHAR_F_UNICODE,instrAndText(GT_UNICODE_F_F)},
    {ITM_GREATER, REC_NCHAR_F_UNICODE,     REC_NCHAR_V_UNICODE,     instrAndText(UNICODE_COMP)},
    {ITM_GREATER, REC_NCHAR_V_UNICODE,     REC_NCHAR_F_UNICODE,     instrAndText(UNICODE_COMP)},
    {ITM_GREATER, REC_NCHAR_V_UNICODE,     REC_NCHAR_V_UNICODE,     instrAndText(UNICODE_COMP)},


    {ITM_GREATER_EQ, REC_BIN8_SIGNED,    REC_BIN8_SIGNED,    instrAndText(GE_BIN8S_BIN8S)},
    {ITM_GREATER_EQ, REC_BIN8_UNSIGNED,  REC_BIN8_UNSIGNED,  instrAndText(GE_BIN8U_BIN8U)},

    {ITM_GREATER_EQ, REC_BIN16_SIGNED,   REC_BIN16_SIGNED,   instrAndText(GE_BIN16S_BIN16S)},
    {ITM_GREATER_EQ, REC_BIN16_SIGNED,   REC_BIN32_SIGNED,   instrAndText(GE_BIN16S_BIN32S)},
    {ITM_GREATER_EQ, REC_BIN16_SIGNED,   REC_BPINT_UNSIGNED, instrAndText(GE_BIN16S_BIN16U)},
    {ITM_GREATER_EQ, REC_BIN16_SIGNED,   REC_BIN16_UNSIGNED, instrAndText(GE_BIN16S_BIN16U)},
    {ITM_GREATER_EQ, REC_BIN16_SIGNED,   REC_BIN32_UNSIGNED, instrAndText(GE_BIN16S_BIN32U)},   // Was 32S. Error? ANS

    {ITM_GREATER_EQ, REC_BPINT_UNSIGNED, REC_BIN16_SIGNED,   instrAndText(GE_BIN16U_BIN16S)},
    {ITM_GREATER_EQ, REC_BPINT_UNSIGNED, REC_BIN32_SIGNED,   instrAndText(GE_BIN16U_BIN32S)},
    {ITM_GREATER_EQ, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, instrAndText(GE_BIN16U_BIN16U)},
    {ITM_GREATER_EQ, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(GE_BIN16U_BIN16U)},
    {ITM_GREATER_EQ, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(GE_BIN16U_BIN32U)},   // Was 32S. Error? ANS

    {ITM_GREATER_EQ, REC_BIN16_UNSIGNED, REC_BIN16_SIGNED,   instrAndText(GE_BIN16U_BIN16S)},
    {ITM_GREATER_EQ, REC_BIN16_UNSIGNED, REC_BIN32_SIGNED,   instrAndText(GE_BIN16U_BIN32S)},
    {ITM_GREATER_EQ, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, instrAndText(GE_BIN16U_BIN16U)},
    {ITM_GREATER_EQ, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(GE_BIN16U_BIN16U)},
    {ITM_GREATER_EQ, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(GE_BIN16U_BIN32U)},   // Was 32S. Error? ANS

    {ITM_GREATER_EQ, REC_BIN32_SIGNED,   REC_BIN16_SIGNED,   instrAndText(GE_BIN32S_BIN16S)},
    {ITM_GREATER_EQ, REC_BIN32_SIGNED,   REC_BIN32_SIGNED,   instrAndText(GE_BIN32S_BIN32S)},
    {ITM_GREATER_EQ, REC_BIN32_SIGNED,   REC_BPINT_UNSIGNED, instrAndText(GE_BIN32S_BIN16U)},
    {ITM_GREATER_EQ, REC_BIN32_SIGNED,   REC_BIN16_UNSIGNED, instrAndText(GE_BIN32S_BIN16U)},
    {ITM_GREATER_EQ, REC_BIN32_SIGNED,   REC_BIN32_UNSIGNED, instrAndText(GE_BIN32S_BIN32U)},   // Was 32S. Error? ANS

    {ITM_GREATER_EQ, REC_BIN32_UNSIGNED, REC_BIN16_SIGNED,   instrAndText(GE_BIN32U_BIN16S)},
    {ITM_GREATER_EQ, REC_BIN32_UNSIGNED, REC_BIN32_SIGNED,   instrAndText(GE_BIN32U_BIN32S)},
    {ITM_GREATER_EQ, REC_BIN32_UNSIGNED, REC_BPINT_UNSIGNED, instrAndText(GE_BIN32U_BIN16U)},
    {ITM_GREATER_EQ, REC_BIN32_UNSIGNED, REC_BIN16_UNSIGNED, instrAndText(GE_BIN32U_BIN16U)},
    {ITM_GREATER_EQ, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, instrAndText(GE_BIN32U_BIN32U)},   // Was 32S. Error? ANS

    {ITM_GREATER_EQ, REC_BIN64_SIGNED,   REC_BIN64_SIGNED,   instrAndText(GE_BIN64S_BIN64S)},
    {ITM_GREATER_EQ, REC_BIN64_UNSIGNED, REC_BIN64_UNSIGNED, instrAndText(GE_BIN64U_BIN64U)},
    {ITM_GREATER_EQ, REC_BIN64_SIGNED,   REC_BIN64_UNSIGNED,   instrAndText(GE_BIN64S_BIN64U)},
    {ITM_GREATER_EQ, REC_BIN64_UNSIGNED, REC_BIN64_SIGNED,     instrAndText(GE_BIN64U_BIN64S)},

    {ITM_GREATER_EQ, REC_DECIMAL_UNSIGNED, REC_DECIMAL_UNSIGNED, instrAndText(GE_DECU_DECU)},
    {ITM_GREATER_EQ, REC_DECIMAL_LSE,      REC_DECIMAL_LSE,      instrAndText(GE_DECS_DECS)},

    {ITM_GREATER_EQ, REC_FLOAT32,        REC_FLOAT32,        instrAndText(GE_FLOAT32_FLOAT32)},
    {ITM_GREATER_EQ, REC_FLOAT64,        REC_FLOAT64,        instrAndText(GE_FLOAT64_FLOAT64)},

    {ITM_GREATER_EQ, REC_DATETIME,       REC_DATETIME,       instrAndText(GE_DATETIME_DATETIME)},

    {ITM_GREATER_EQ, REC_BYTE_F_ASCII,     REC_BYTE_F_ASCII,     instrAndText(GE_ASCII_F_F)},

    {ITM_GREATER_EQ, REC_BYTE_F_ASCII,     REC_BYTE_V_ASCII,     instrAndText(GE_ASCII_COMP)},
    {ITM_GREATER_EQ, REC_BYTE_V_ASCII,     REC_BYTE_F_ASCII,     instrAndText(GE_ASCII_COMP)},
    {ITM_GREATER_EQ, REC_BYTE_V_ASCII,     REC_BYTE_V_ASCII,     instrAndText(GE_ASCII_COMP)},
    {ITM_GREATER_EQ, REC_BYTE_F_ASCII,     REC_BYTE_V_ASCII_LONG,     instrAndText(GE_ASCII_COMP)},
    {ITM_GREATER_EQ, REC_BYTE_V_ASCII_LONG,     REC_BYTE_F_ASCII,     instrAndText(GE_ASCII_COMP)},
    {ITM_GREATER_EQ, REC_BYTE_V_ASCII,     REC_BYTE_V_ASCII_LONG,     instrAndText(GE_ASCII_COMP)},
    {ITM_GREATER_EQ, REC_BYTE_V_ASCII_LONG,     REC_BYTE_V_ASCII,     instrAndText(GE_ASCII_COMP)},
    {ITM_GREATER_EQ, REC_BYTE_V_ASCII_LONG,     REC_BYTE_V_ASCII_LONG,     instrAndText(GE_ASCII_COMP)},

    {ITM_GREATER_EQ,REC_NCHAR_F_UNICODE,REC_NCHAR_F_UNICODE,instrAndText(GE_UNICODE_F_F)},
    {ITM_GREATER_EQ, REC_NCHAR_F_UNICODE,     REC_NCHAR_V_UNICODE,     instrAndText(UNICODE_COMP)},
    {ITM_GREATER_EQ, REC_NCHAR_V_UNICODE,     REC_NCHAR_F_UNICODE,     instrAndText(UNICODE_COMP)},
    {ITM_GREATER_EQ, REC_NCHAR_V_UNICODE,     REC_NCHAR_V_UNICODE,     instrAndText(UNICODE_COMP)},

  };
  

const ex_comp_clause::CompInstrStruct * ex_comp_clause::getMatchingRow(OperatorTypeEnum op,
						      short datatype1,
						      short datatype2)
{
  Int32 max_array_size = sizeof(compInstrInfo) / sizeof(CompInstrStruct);

  Int32 i = 0;
  
  Int32 found = 0;
  while ((i < max_array_size) && (!found))
    if ((compInstrInfo[i].op == op) && 
	(compInstrInfo[i].type_op1 == datatype1) &&
	(compInstrInfo[i].type_op2 == datatype2))
      found = -1;
    else
      i++;
  
  if (found)
    {
      setInstrArrayIndex(i);
      return &compInstrInfo[i];  
    }
  else
    return 0;
}

Lng32 ex_comp_clause::findIndexIntoInstrArray(CompInstruction ci)
{
  Int32 max_array_size = sizeof(compInstrInfo) / sizeof(CompInstrStruct);
  
  Int32 i = 0;
  while ((i < max_array_size) && 
         (compInstrInfo[i].instruction != ci)) 
    {
      i++;
    }

  if (i < max_array_size)
    return i;

  return -1; // not found
}

const CompInstruction ex_comp_clause::computeCaseIndex(OperatorTypeEnum op, 
                                                       Attributes * attr1,
                                                       Attributes * attr2)
{
  CompInstruction instruction = COMP_NOT_SUPPORTED;
  
  short type_op1;
  short type_op2;
  getCaseDatatypes(attr1->getDatatype(), attr1->getLength(), type_op1,
                   attr2->getDatatype(), attr2->getLength(), type_op2,
                   0 /* don't need to take into account scale difference here */);

  const CompInstrStruct * cs = getMatchingRow(op, 
                                              type_op1,
                                              type_op2);
  if (cs)
    {
      instruction = cs->instruction;
      
      // Further checks are needed. Do them. 
      if ((attr1->getDatatype() == REC_BYTE_F_ASCII) &&
	  (attr2->getDatatype() == REC_BYTE_F_ASCII))
	{
	  if (attr1->getLength() != attr2->getLength())
            {
              instruction = ASCII_COMP;
              setInstrArrayIndex(findIndexIntoInstrArray(instruction));
            }
	}

      if ((attr1->getDatatype() == REC_NCHAR_F_UNICODE) &&
	  (attr2->getDatatype() == REC_NCHAR_F_UNICODE))
	{
	  if (attr1->getLength() != attr2->getLength())
            {
              instruction = UNICODE_COMP;
              setInstrArrayIndex(findIndexIntoInstrArray(instruction));
             }
	}
      
      // for all numeric, datetime and interval datatypes, comparison done
      // only if scale is the same
      if (((attr1->getDatatype() >= REC_MIN_BINARY_NUMERIC) &&
	   (attr1->getDatatype() <= REC_MAX_BINARY_NUMERIC)) ||
	  ((attr1->getDatatype() >= REC_DECIMAL_UNSIGNED) &&
	   (attr1->getDatatype() <= REC_DECIMAL_LSE)) ||
	  (attr1->getDatatype() == REC_DATETIME) ||
	  ((attr1->getDatatype() >= REC_MIN_INTERVAL) &&
	   (attr1->getDatatype() <= REC_MAX_INTERVAL)))
	{
	  // numeric, datetime or interval types
	  if (attr1->getScale() != attr2->getScale())
	    instruction = COMP_NOT_SUPPORTED; 
	  
	  // for decimal datatypes, comparison done only if lengths are same.
	  if ((attr1->getDatatype() >= REC_DECIMAL_UNSIGNED) &&
	      (attr1->getDatatype() <= REC_DECIMAL_LSE))
	    {
	      if (attr1->getLength() != attr2->getLength())
		instruction = COMP_NOT_SUPPORTED;
	    }
	} // numeric, datetime or interval types
    } // comparison supported 
  else
    {
      if (((attr1->getDatatype() == REC_BINARY_STRING) ||
           (attr1->getDatatype() == REC_VARBINARY_STRING)) &&
          ((attr2->getDatatype() == REC_BINARY_STRING) ||
           (attr2->getDatatype() == REC_VARBINARY_STRING)))
        {
          instruction = BINARY_COMP;
          setInstrArrayIndex(findIndexIntoInstrArray(instruction));
        }
    }

  if (instruction == COMP_NOT_SUPPORTED)
    setInstrArrayIndex(-1);

  return instruction;
}

void ex_comp_clause::setInstruction()
{ 
  CompInstruction instruction = COMP_NOT_SUPPORTED;
  setInstrArrayIndex(-1);

  if ((getOperand(1)->isComplexType()) || (getOperand(2)->isComplexType()))
    {
      // complex comparison supported only if both operands are complex with
      // same datatype
      if ((getOperand(1)->isComplexType()) && (getOperand(2)->isComplexType()) &&
	  (getOperand(1)->getDatatype() == getOperand(2)->getDatatype()))
        {
          instruction = COMP_COMPLEX;
          setInstrArrayIndex(findIndexIntoInstrArray(instruction));
        }
    }
  else
    {
      // Simple types are handled here.
      instruction = computeCaseIndex(getOperType(), getOperand(1), getOperand(2));
    } // simple type
 
  if ((instruction == COMP_NOT_SUPPORTED) &&
      (getInstrArrayIndex() >= 0))
    {
      // this is an error, reset instr array index
      setInstrArrayIndex(-1);
    }
}
  
void ex_comp_clause::setInstruction(OperatorTypeEnum op,
				    Attributes * attr1,
				    Attributes * attr2)
{
  // Only simple types are handled here.
  CompInstruction instruction = computeCaseIndex(op, attr1, attr2);

  if ((instruction == COMP_NOT_SUPPORTED) &&
      (getInstrArrayIndex() >= 0))
    {
      // this is an error, reset instr array index
      setInstrArrayIndex(-1);
    }  
}

short ex_comp_clause::isComparisonSupported(OperatorTypeEnum op,
					    Attributes * attr1,
					    Attributes * attr2)
{
  if (computeCaseIndex(op, attr1, attr2) 
      == COMP_NOT_SUPPORTED)
    return 0;
  else
    return -1;
}

ex_expr::exp_return_type ex_comp_clause::fixup(Space * space,
					       CollHeap * exHeap,
					       char * constants_area,
					       char * temps_area,
					       char * persistentArea,
                                               short fixupConstsAndTemps,
					       NABoolean spaceCompOnly)
{
  setInstruction();
  return ex_clause::fixup(space, exHeap, constants_area, temps_area,
			  persistentArea,
			  fixupConstsAndTemps, spaceCompOnly);
}

// Entries in this struct must group same src datatypes together.
const ex_conv_clause::ConvInstrStruct ex_conv_clause::convInstrInfo[] = {
  //-----------------------------------------------------------------------
  // src datatype          tgt datatype            instruction and indexStr
  //-----------------------------------------------------------------------
  {REC_UNKNOWN,        REC_UNKNOWN,                instrAndText(CONV_COMPLEX_TO_COMPLEX)},
  {REC_UNKNOWN,        REC_UNKNOWN,                instrAndText(CONV_SIMPLE_TO_COMPLEX)},
  {REC_UNKNOWN,        REC_UNKNOWN,                instrAndText(CONV_COMPLEX_TO_SIMPLE)},

  {REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED,         instrAndText(CONV_BPINTU_BPINTU)},
  {REC_BPINT_UNSIGNED, REC_BIN16_SIGNED,	   instrAndText(CONV_BIN16U_BIN16S)},
  {REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED,         instrAndText(CONV_BIN16U_BIN16U)},
  {REC_BPINT_UNSIGNED, REC_BIN32_SIGNED,           instrAndText(CONV_BIN16U_BIN32S)},
  {REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED,         instrAndText(CONV_BIN16U_BIN32U)},
  {REC_BPINT_UNSIGNED, REC_BIN64_SIGNED,           instrAndText(CONV_BIN16U_BIN64S)},
  {REC_BPINT_UNSIGNED, REC_DECIMAL_LSE,            instrAndText(CONV_BIN16U_DECS)},
  {REC_BPINT_UNSIGNED, REC_DECIMAL_UNSIGNED,       instrAndText(CONV_BIN16U_DECS)},
  {REC_BPINT_UNSIGNED, REC_FLOAT32,                instrAndText(CONV_BIN16U_FLOAT32)},
  {REC_BPINT_UNSIGNED, REC_FLOAT64,                instrAndText(CONV_BIN16U_FLOAT64)},
  {REC_BPINT_UNSIGNED, REC_NUM_BIG_UNSIGNED,       instrAndText(CONV_BIN16U_BIGNUM)},
  {REC_BPINT_UNSIGNED, REC_NUM_BIG_SIGNED,         instrAndText(CONV_BIN16U_BIGNUM)},
  {REC_BPINT_UNSIGNED, REC_BYTE_F_ASCII,           instrAndText(CONV_BIN16U_ASCII)},
  {REC_BPINT_UNSIGNED, REC_BYTE_V_ASCII,           instrAndText(CONV_BIN16U_ASCII)},
  {REC_BPINT_UNSIGNED, REC_BYTE_V_ASCII_LONG,      instrAndText(CONV_BIN16U_ASCII)},
  {REC_BPINT_UNSIGNED, REC_NCHAR_F_UNICODE,        instrAndText(CONV_BIN16U_UNICODE)},
  {REC_BPINT_UNSIGNED, REC_NCHAR_V_UNICODE,        instrAndText(CONV_BIN16U_UNICODE)},

  {REC_BIN8_SIGNED,    REC_BIN16_SIGNED,           instrAndText(CONV_BIN8S_BIN16S)},
  {REC_BIN8_SIGNED,    REC_BIN32_SIGNED,           instrAndText(CONV_BIN8S_BIN32S)},
  {REC_BIN8_SIGNED,    REC_BIN64_SIGNED,           instrAndText(CONV_BIN8S_BIN64S)},
  {REC_BIN8_SIGNED,    REC_BIN8_SIGNED,            instrAndText(CONV_BIN8S_BIN8S)},
  {REC_BIN8_SIGNED,    REC_BIN8_UNSIGNED,          instrAndText(CONV_BIN8S_BIN8U)},
  {REC_BIN8_SIGNED,    REC_BYTE_F_ASCII,           instrAndText(CONV_BIN8S_ASCII)},
  {REC_BIN8_SIGNED,    REC_BYTE_V_ASCII,           instrAndText(CONV_BIN8S_ASCII)},

  {REC_BIN8_UNSIGNED,  REC_BIN16_UNSIGNED,         instrAndText(CONV_BIN8U_BIN16U)},
  {REC_BIN8_UNSIGNED,  REC_BIN32_UNSIGNED,         instrAndText(CONV_BIN8U_BIN32U)},
  {REC_BIN8_UNSIGNED,  REC_BIN64_UNSIGNED,         instrAndText(CONV_BIN8U_BIN64U)},
  {REC_BIN8_UNSIGNED,  REC_BIN16_SIGNED,           instrAndText(CONV_BIN8U_BIN16S)},
  {REC_BIN8_UNSIGNED,  REC_BIN8_UNSIGNED,          instrAndText(CONV_BIN8U_BIN8U)},
  {REC_BIN8_UNSIGNED,  REC_BYTE_F_ASCII,           instrAndText(CONV_BIN8U_ASCII)},
  {REC_BIN8_UNSIGNED,  REC_BYTE_V_ASCII,           instrAndText(CONV_BIN8U_ASCII)},
  {REC_BIN8_UNSIGNED,  REC_BIN8_SIGNED,            instrAndText(CONV_BIN8U_BIN8S)},

  {REC_BIN16_SIGNED, 	REC_BPINT_UNSIGNED,         instrAndText(CONV_BIN16S_BPINTU)},
  {REC_BIN16_SIGNED,  REC_BIN16_SIGNED,           instrAndText(CONV_BIN16S_BIN16S)},
  {REC_BIN16_SIGNED,  REC_BIN16_UNSIGNED,         instrAndText(CONV_BIN16S_BIN16U)},
  {REC_BIN16_SIGNED,  REC_BIN32_SIGNED,           instrAndText(CONV_BIN16S_BIN32S)},
  {REC_BIN16_SIGNED,  REC_BIN32_UNSIGNED,         instrAndText(CONV_BIN16S_BIN32U)},
  {REC_BIN16_SIGNED,  REC_BIN64_SIGNED,           instrAndText(CONV_BIN16S_BIN64S)},
  {REC_BIN16_SIGNED,  REC_DECIMAL_LSE,            instrAndText(CONV_BIN16S_DECS)},
  {REC_BIN16_SIGNED,  REC_DECIMAL_UNSIGNED,       instrAndText(CONV_BIN16S_DECU)},
  {REC_BIN16_SIGNED,  REC_FLOAT32,                instrAndText(CONV_BIN16S_FLOAT32)},
  {REC_BIN16_SIGNED,  REC_FLOAT64,                instrAndText(CONV_BIN16S_FLOAT64)},
  {REC_BIN16_SIGNED,  REC_NUM_BIG_UNSIGNED,       instrAndText(CONV_BIN16S_BIGNUMU)},
  {REC_BIN16_SIGNED,  REC_NUM_BIG_SIGNED,         instrAndText(CONV_BIN16S_BIGNUM)},
  {REC_BIN16_SIGNED,  REC_BYTE_F_ASCII,           instrAndText(CONV_BIN16S_ASCII)},
  {REC_BIN16_SIGNED,  REC_BYTE_V_ASCII,           instrAndText(CONV_BIN16S_ASCII)},
  {REC_BIN16_SIGNED,  REC_BYTE_V_ASCII_LONG,      instrAndText(CONV_BIN16S_ASCII)},
  {REC_BIN16_SIGNED,  REC_NCHAR_F_UNICODE,        instrAndText(CONV_BIN16S_UNICODE)},
  {REC_BIN16_SIGNED,  REC_NCHAR_V_UNICODE,        instrAndText(CONV_BIN16S_UNICODE)},

  {REC_BIN16_SIGNED,   REC_BIN8_SIGNED,           instrAndText(CONV_BIN16S_BIN8S)},
  {REC_BIN16_SIGNED,   REC_BIN8_UNSIGNED,         instrAndText(CONV_BIN16S_BIN8U)},

  {REC_BIN16_UNSIGNED,  REC_BPINT_UNSIGNED,         instrAndText(CONV_BIN16U_BPINTU)},
  {REC_BIN16_UNSIGNED,  REC_BIN16_SIGNED,           instrAndText(CONV_BIN16U_BIN16S)},
  {REC_BIN16_UNSIGNED,  REC_BIN16_UNSIGNED,         instrAndText(CONV_BIN16U_BIN16U)},
  {REC_BIN16_UNSIGNED,  REC_BIN32_SIGNED,           instrAndText(CONV_BIN16U_BIN32S)},
  {REC_BIN16_UNSIGNED,  REC_BIN32_UNSIGNED,         instrAndText(CONV_BIN16U_BIN32U)},
  {REC_BIN16_UNSIGNED,  REC_BIN64_SIGNED,           instrAndText(CONV_BIN16U_BIN64S)},
  {REC_BIN16_UNSIGNED,  REC_DECIMAL_LSE,            instrAndText(CONV_BIN16U_DECS)},
  {REC_BIN16_UNSIGNED,  REC_DECIMAL_UNSIGNED,       instrAndText(CONV_BIN16U_DECS)},
  {REC_BIN16_UNSIGNED,  REC_FLOAT32,                instrAndText(CONV_BIN16U_FLOAT32)},
  {REC_BIN16_UNSIGNED,  REC_FLOAT64,                instrAndText(CONV_BIN16U_FLOAT64)},
  {REC_BIN16_UNSIGNED,  REC_NUM_BIG_UNSIGNED,       instrAndText(CONV_BIN16U_BIGNUM)},
  {REC_BIN16_UNSIGNED,  REC_NUM_BIG_SIGNED,         instrAndText(CONV_BIN16U_BIGNUM)},
  {REC_BIN16_UNSIGNED,  REC_BYTE_F_ASCII,           instrAndText(CONV_BIN16U_ASCII)},
  {REC_BIN16_UNSIGNED,  REC_BYTE_V_ASCII,           instrAndText(CONV_BIN16U_ASCII)},
  {REC_BIN16_UNSIGNED,  REC_BYTE_V_ASCII_LONG,      instrAndText(CONV_BIN16U_ASCII)},
  {REC_BIN16_UNSIGNED,  REC_NCHAR_F_UNICODE,        instrAndText(CONV_BIN16U_UNICODE)},
  {REC_BIN16_UNSIGNED,  REC_NCHAR_V_UNICODE,        instrAndText(CONV_BIN16U_UNICODE)},

  {REC_BIN16_UNSIGNED, REC_BIN8_SIGNED,            instrAndText(CONV_BIN16U_BIN8S)},
  {REC_BIN16_UNSIGNED, REC_BIN8_UNSIGNED,          instrAndText(CONV_BIN16U_BIN8U)},
   
  {REC_BIN32_SIGNED,	 REC_BPINT_UNSIGNED,         instrAndText(CONV_BIN32S_BPINTU)}, 
  {REC_BIN32_SIGNED,   REC_BIN16_SIGNED,           instrAndText(CONV_BIN32S_BIN16S)},
  {REC_BIN32_SIGNED,   REC_BIN16_UNSIGNED,         instrAndText(CONV_BIN32S_BIN16U)},
  {REC_BIN32_SIGNED,   REC_BIN32_SIGNED,           instrAndText(CONV_BIN32S_BIN32S)},
  {REC_BIN32_SIGNED,   REC_BIN32_UNSIGNED,         instrAndText(CONV_BIN32S_BIN32U)},
  {REC_BIN32_SIGNED,   REC_BIN64_SIGNED,           instrAndText(CONV_BIN32S_BIN64S)},
  {REC_BIN32_SIGNED,   REC_DECIMAL_LSE,            instrAndText(CONV_BIN32S_DECS)},
  {REC_BIN32_SIGNED,   REC_DECIMAL_UNSIGNED,       instrAndText(CONV_BIN32S_DECU)},
  {REC_BIN32_SIGNED,   REC_FLOAT32,                instrAndText(CONV_BIN32S_FLOAT32)},
  {REC_BIN32_SIGNED,   REC_FLOAT64,                instrAndText(CONV_BIN32S_FLOAT64)},
  {REC_BIN32_SIGNED,   REC_NUM_BIG_UNSIGNED,       instrAndText(CONV_BIN32S_BIGNUMU)},
  {REC_BIN32_SIGNED,   REC_NUM_BIG_SIGNED,         instrAndText(CONV_BIN32S_BIGNUM)},
  {REC_BIN32_SIGNED,   REC_BYTE_F_ASCII,           instrAndText(CONV_BIN32S_ASCII)},
  {REC_BIN32_SIGNED,   REC_BYTE_V_ASCII,           instrAndText(CONV_BIN32S_ASCII)},
  {REC_BIN32_SIGNED,   REC_BYTE_V_ASCII_LONG,      instrAndText(CONV_BIN32S_ASCII)},
  {REC_BIN32_SIGNED,   REC_NCHAR_F_UNICODE,        instrAndText(CONV_BIN32S_UNICODE)},
  {REC_BIN32_SIGNED,   REC_NCHAR_V_UNICODE,        instrAndText(CONV_BIN32S_UNICODE)},
    
  {REC_BIN32_UNSIGNED,  REC_BPINT_UNSIGNED,         instrAndText(CONV_BIN32U_BPINTU)}, 
  {REC_BIN32_UNSIGNED,  REC_BIN16_SIGNED,           instrAndText(CONV_BIN32U_BIN16S)},
  {REC_BIN32_UNSIGNED,  REC_BIN16_UNSIGNED,         instrAndText(CONV_BIN32U_BIN16U)},
  {REC_BIN32_UNSIGNED,  REC_BIN32_SIGNED,           instrAndText(CONV_BIN32U_BIN32S)},
  {REC_BIN32_UNSIGNED,  REC_BIN32_UNSIGNED,         instrAndText(CONV_BIN32U_BIN32U)},
  {REC_BIN32_UNSIGNED,  REC_BIN64_SIGNED,           instrAndText(CONV_BIN32U_BIN64S)},
  {REC_BIN32_UNSIGNED,  REC_DECIMAL_LSE,            instrAndText(CONV_BIN32U_DECS)},
  {REC_BIN32_UNSIGNED,  REC_DECIMAL_UNSIGNED,       instrAndText(CONV_BIN32U_DECS)},
  {REC_BIN32_UNSIGNED,  REC_FLOAT32,                instrAndText(CONV_BIN32U_FLOAT32)},
  {REC_BIN32_UNSIGNED,  REC_FLOAT64,                instrAndText(CONV_BIN32U_FLOAT64)},
  {REC_BIN32_UNSIGNED,  REC_NUM_BIG_UNSIGNED,       instrAndText(CONV_BIN32U_BIGNUM)},
  {REC_BIN32_UNSIGNED,  REC_NUM_BIG_SIGNED,         instrAndText(CONV_BIN32U_BIGNUM)},
  {REC_BIN32_UNSIGNED,  REC_BYTE_F_ASCII,           instrAndText(CONV_BIN32U_ASCII)},
  {REC_BIN32_UNSIGNED,  REC_BYTE_V_ASCII,           instrAndText(CONV_BIN32U_ASCII)},
  {REC_BIN32_UNSIGNED,  REC_BYTE_V_ASCII_LONG,      instrAndText(CONV_BIN32U_ASCII)},
  {REC_BIN32_UNSIGNED,  REC_NCHAR_F_UNICODE,        instrAndText(CONV_BIN32U_UNICODE)},
  {REC_BIN32_UNSIGNED,  REC_NCHAR_V_UNICODE,        instrAndText(CONV_BIN32U_UNICODE)},

  {REC_BIN32_UNSIGNED,   REC_BIN8_SIGNED,           instrAndText(CONV_NUMERIC_BIN8S)},
  {REC_BIN32_UNSIGNED,   REC_BIN8_UNSIGNED,         instrAndText(CONV_NUMERIC_BIN8U)},

  {REC_BIN64_SIGNED,   REC_BPINT_UNSIGNED,         instrAndText(CONV_BIN64S_BPINTU)},
  {REC_BIN64_SIGNED,   REC_BIN16_SIGNED,           instrAndText(CONV_BIN64S_BIN16S)},
  {REC_BIN64_SIGNED,   REC_BIN16_UNSIGNED,         instrAndText(CONV_BIN64S_BIN16U)},
  {REC_BIN64_SIGNED,   REC_BIN32_SIGNED,           instrAndText(CONV_BIN64S_BIN32S)},
  {REC_BIN64_SIGNED,   REC_BIN32_UNSIGNED,         instrAndText(CONV_BIN64S_BIN32U)},
  {REC_BIN64_SIGNED,   REC_BIN64_SIGNED,           instrAndText(CONV_BIN64S_BIN64S)},
  {REC_BIN64_SIGNED,   REC_BIN64_UNSIGNED,         instrAndText(CONV_BIN64S_BIN64U)},
  {REC_BIN64_SIGNED,   REC_DECIMAL_LSE,            instrAndText(CONV_BIN64S_DECS)},
  {REC_BIN64_SIGNED,   REC_DECIMAL_UNSIGNED,       instrAndText(CONV_BIN64S_DECU)},
  {REC_BIN64_SIGNED,   REC_FLOAT32,                instrAndText(CONV_BIN64S_FLOAT32)},
  {REC_BIN64_SIGNED,   REC_FLOAT64,                instrAndText(CONV_BIN64S_FLOAT64)},
  {REC_BIN64_SIGNED,   REC_NUM_BIG_UNSIGNED,       instrAndText(CONV_BIN64S_BIGNUMU)},
  {REC_BIN64_SIGNED,   REC_NUM_BIG_SIGNED,         instrAndText(CONV_BIN64S_BIGNUM)},
  {REC_BIN64_SIGNED,   REC_BYTE_F_ASCII,           instrAndText(CONV_BIN64S_ASCII)},
  {REC_BIN64_SIGNED,   REC_BYTE_V_ASCII,           instrAndText(CONV_BIN64S_ASCII)},
  {REC_BIN64_SIGNED,   REC_BYTE_V_ASCII_LONG,      instrAndText(CONV_BIN64S_ASCII)},
  {REC_BIN64_SIGNED,   REC_NCHAR_F_UNICODE,        instrAndText(CONV_BIN64S_UNICODE)},
  {REC_BIN64_SIGNED,   REC_NCHAR_V_UNICODE,        instrAndText(CONV_BIN64S_UNICODE)},
  {REC_BIN64_SIGNED,   REC_DATETIME,               instrAndText(CONV_BIN64S_DATETIME)},

  {REC_BIN64_UNSIGNED,   REC_BIN64_SIGNED,           instrAndText(CONV_BIN64U_BIN64S)},
  {REC_BIN64_UNSIGNED,   REC_BIN64_UNSIGNED,         instrAndText(CONV_BIN64U_BIN64U)},
  {REC_BIN64_UNSIGNED,   REC_NUM_BIG_SIGNED,         instrAndText(CONV_BIN64U_BIGNUM)},
  {REC_BIN64_UNSIGNED,   REC_NUM_BIG_UNSIGNED,       instrAndText(CONV_BIN64U_BIGNUM)},
  {REC_BIN64_UNSIGNED,   REC_FLOAT32,                instrAndText(CONV_BIN64U_FLOAT32)},
  {REC_BIN64_UNSIGNED,   REC_FLOAT64,                instrAndText(CONV_BIN64U_FLOAT64)},
  {REC_BIN64_UNSIGNED,   REC_BYTE_F_ASCII,           instrAndText(CONV_BIN64U_ASCII)},
  {REC_BIN64_UNSIGNED,   REC_BYTE_V_ASCII,           instrAndText(CONV_BIN64U_ASCII)},

  {REC_DECIMAL_UNSIGNED,   REC_BPINT_UNSIGNED,         instrAndText(CONV_DECS_BIN32U)},
  {REC_DECIMAL_UNSIGNED,   REC_BIN16_UNSIGNED,         instrAndText(CONV_DECS_BIN32U)},
  {REC_DECIMAL_UNSIGNED,   REC_BIN16_SIGNED,           instrAndText(CONV_DECS_BIN32S)},
  {REC_DECIMAL_UNSIGNED,   REC_BIN32_UNSIGNED,         instrAndText(CONV_DECS_BIN32U)},
  {REC_DECIMAL_UNSIGNED,   REC_BIN32_SIGNED,           instrAndText(CONV_DECS_BIN32S)},
  {REC_DECIMAL_UNSIGNED,   REC_BIN64_SIGNED,           instrAndText(CONV_DECS_BIN64S)},
  {REC_DECIMAL_UNSIGNED,   REC_DECIMAL_UNSIGNED,       instrAndText(CONV_DECS_DECU)},
  {REC_DECIMAL_UNSIGNED,   REC_DECIMAL_LSE,            instrAndText(CONV_DECS_DECS)},
  {REC_DECIMAL_UNSIGNED,   REC_FLOAT32,                instrAndText(CONV_DECS_FLOAT32)},
  {REC_DECIMAL_UNSIGNED,   REC_FLOAT64,                instrAndText(CONV_DECS_FLOAT64)},
  {REC_DECIMAL_UNSIGNED,   REC_NUM_BIG_UNSIGNED,       instrAndText(CONV_DECS_BIN32U)},
  {REC_DECIMAL_UNSIGNED,   REC_NUM_BIG_SIGNED,         instrAndText(CONV_DECS_BIN32S)},
  {REC_DECIMAL_UNSIGNED,   REC_BYTE_F_ASCII,           instrAndText(CONV_DECS_ASCII)},
  {REC_DECIMAL_UNSIGNED,   REC_BYTE_V_ASCII,           instrAndText(CONV_DECS_ASCII)},
  {REC_DECIMAL_UNSIGNED,   REC_BYTE_V_ASCII_LONG,      instrAndText(CONV_DECS_ASCII)},
  {REC_DECIMAL_UNSIGNED,   REC_NCHAR_F_UNICODE,        instrAndText(CONV_DECS_UNICODE)},
  {REC_DECIMAL_UNSIGNED,   REC_NCHAR_V_UNICODE,        instrAndText(CONV_DECS_UNICODE)},
    
  {REC_DECIMAL_LSE,        REC_BPINT_UNSIGNED,         instrAndText(CONV_DECS_BIN32U)},
  {REC_DECIMAL_LSE,        REC_BIN16_SIGNED,           instrAndText(CONV_DECS_BIN32S)},
  {REC_DECIMAL_LSE,        REC_BIN16_UNSIGNED,         instrAndText(CONV_DECS_BIN32U)},
  {REC_DECIMAL_LSE,        REC_BIN32_SIGNED,           instrAndText(CONV_DECS_BIN32S)},
  {REC_DECIMAL_LSE,        REC_BIN32_UNSIGNED,         instrAndText(CONV_DECS_BIN32U)},
  {REC_DECIMAL_LSE,        REC_BIN64_SIGNED,           instrAndText(CONV_DECS_BIN64S)},
  {REC_DECIMAL_LSE,        REC_DECIMAL_LSE,            instrAndText(CONV_DECS_DECS)},
  {REC_DECIMAL_LSE,        REC_DECIMAL_UNSIGNED,       instrAndText(CONV_DECS_DECU)},
  {REC_DECIMAL_LSE,        REC_FLOAT32,                instrAndText(CONV_DECS_FLOAT32)},
  {REC_DECIMAL_LSE,        REC_FLOAT64,                instrAndText(CONV_DECS_FLOAT64)},
  {REC_DECIMAL_LSE,        REC_NUM_BIG_UNSIGNED,       instrAndText(CONV_DECS_BIN32U)},
  {REC_DECIMAL_LSE,        REC_NUM_BIG_SIGNED,         instrAndText(CONV_DECS_BIN32S)},
  {REC_DECIMAL_LSE,        REC_DECIMAL_LS,             instrAndText(CONV_DECS_ASCII)},
  {REC_DECIMAL_LSE,        REC_BYTE_F_ASCII,           instrAndText(CONV_DECS_ASCII)},
  {REC_DECIMAL_LSE,        REC_BYTE_V_ASCII,           instrAndText(CONV_DECS_ASCII)},
  {REC_DECIMAL_LSE,        REC_BYTE_V_ASCII_LONG,      instrAndText(CONV_DECS_ASCII)},
  {REC_DECIMAL_LSE,        REC_NCHAR_F_UNICODE,        instrAndText(CONV_DECS_UNICODE)},
  {REC_DECIMAL_LSE,        REC_NCHAR_V_UNICODE,        instrAndText(CONV_DECS_UNICODE)},
    
  {REC_FLOAT32,            REC_BPINT_UNSIGNED,     instrAndText(CONV_FLOAT32_BPINTU)},
  {REC_FLOAT32,            REC_BIN16_SIGNED,       instrAndText(CONV_FLOAT32_BIN16S)},
  {REC_FLOAT32,            REC_BIN16_UNSIGNED,     instrAndText(CONV_FLOAT32_BIN16U)},
  {REC_FLOAT32,            REC_BIN32_SIGNED,       instrAndText(CONV_FLOAT32_BIN32S)},
  {REC_FLOAT32,            REC_BIN32_UNSIGNED,     instrAndText(CONV_FLOAT32_BIN32U)},
  {REC_FLOAT32,            REC_BIN64_SIGNED,       instrAndText(CONV_FLOAT32_BIN64S)},
  {REC_FLOAT32,            REC_BIN64_UNSIGNED,     instrAndText(CONV_FLOAT32_BIN64U)},
  {REC_FLOAT32,            REC_DECIMAL_LSE,        instrAndText(CONV_FLOAT32_DECS)},
  {REC_FLOAT32,            REC_DECIMAL_UNSIGNED,   instrAndText(CONV_FLOAT32_DECU)},
  {REC_FLOAT32,            REC_FLOAT32,            instrAndText(CONV_FLOAT32_FLOAT32)},
  {REC_FLOAT32,            REC_FLOAT64,            instrAndText(CONV_FLOAT32_FLOAT64)},
  {REC_FLOAT32,            REC_BYTE_F_ASCII,       instrAndText(CONV_FLOAT32_ASCII)},
  {REC_FLOAT32,            REC_BYTE_V_ASCII,       instrAndText(CONV_FLOAT32_ASCII)},
  {REC_FLOAT32,            REC_BYTE_V_ASCII_LONG,  instrAndText(CONV_FLOAT32_ASCII)},
  {REC_FLOAT32,            REC_NCHAR_F_UNICODE,    instrAndText(CONV_FLOAT32_UNICODE)},
  {REC_FLOAT32,            REC_NCHAR_V_UNICODE,    instrAndText(CONV_FLOAT32_UNICODE)},
    
  {REC_FLOAT64,            REC_BPINT_UNSIGNED,     instrAndText(CONV_FLOAT64_BPINTU)},
  {REC_FLOAT64,            REC_BIN16_SIGNED,       instrAndText(CONV_FLOAT64_BIN16S)},
  {REC_FLOAT64,            REC_BIN16_UNSIGNED,     instrAndText(CONV_FLOAT64_BIN16U)},
  {REC_FLOAT64,            REC_BIN32_SIGNED,       instrAndText(CONV_FLOAT64_BIN32S)},
  {REC_FLOAT64,            REC_BIN32_UNSIGNED,     instrAndText(CONV_FLOAT64_BIN32U)},
  {REC_FLOAT64,            REC_BIN64_SIGNED,       instrAndText(CONV_FLOAT64_BIN64S)},
  {REC_FLOAT64,            REC_BIN64_UNSIGNED,     instrAndText(CONV_FLOAT64_BIN64U)},
  {REC_FLOAT64,            REC_DECIMAL_LSE,        instrAndText(CONV_FLOAT64_DECS)},
  {REC_FLOAT64,            REC_DECIMAL_UNSIGNED,   instrAndText(CONV_FLOAT64_DECU)},
  {REC_FLOAT64,            REC_FLOAT32,            instrAndText(CONV_FLOAT64_FLOAT32)},
  {REC_FLOAT64,            REC_FLOAT64,            instrAndText(CONV_FLOAT64_FLOAT64)},
  {REC_FLOAT64,            REC_BYTE_F_ASCII,       instrAndText(CONV_FLOAT64_ASCII)},
  {REC_FLOAT64,            REC_BYTE_V_ASCII,       instrAndText(CONV_FLOAT64_ASCII)},
  {REC_FLOAT64,            REC_BYTE_V_ASCII_LONG,  instrAndText(CONV_FLOAT64_ASCII)},
  {REC_FLOAT64,            REC_NCHAR_F_UNICODE,    instrAndText(CONV_FLOAT64_UNICODE)},
  {REC_FLOAT64,            REC_NCHAR_V_UNICODE,    instrAndText(CONV_FLOAT64_UNICODE)},

  {REC_NUM_BIG_UNSIGNED, REC_BPINT_UNSIGNED,         instrAndText(CONV_BIGNUM_BIN16U)},
  {REC_NUM_BIG_UNSIGNED, REC_BIN16_SIGNED,           instrAndText(CONV_BIGNUM_BIN16S)},
  {REC_NUM_BIG_UNSIGNED, REC_BIN16_UNSIGNED,         instrAndText(CONV_BIGNUM_BIN16U)},
  {REC_NUM_BIG_UNSIGNED, REC_BIN32_SIGNED,           instrAndText(CONV_BIGNUM_BIN32S)},
  {REC_NUM_BIG_UNSIGNED, REC_BIN32_UNSIGNED,         instrAndText(CONV_BIGNUM_BIN32U)},
  {REC_NUM_BIG_UNSIGNED, REC_BIN64_SIGNED,           instrAndText(CONV_BIGNUM_BIN64S)},
  {REC_NUM_BIG_UNSIGNED, REC_BIN64_UNSIGNED,         instrAndText(CONV_BIGNUM_BIN64U)},
  {REC_NUM_BIG_UNSIGNED, REC_DECIMAL_LSE,            instrAndText(CONV_BIGNUM_DECS)},
  {REC_NUM_BIG_UNSIGNED, REC_DECIMAL_UNSIGNED,       instrAndText(CONV_BIGNUM_DECU)},
  {REC_NUM_BIG_UNSIGNED, REC_FLOAT32,                instrAndText(CONV_BIGNUM_FLOAT32)},
  {REC_NUM_BIG_UNSIGNED, REC_FLOAT64,                instrAndText(CONV_BIGNUM_FLOAT64)},
  {REC_NUM_BIG_UNSIGNED, REC_NUM_BIG_UNSIGNED,       instrAndText(CONV_BIGNUM_BIGNUM)},
  {REC_NUM_BIG_UNSIGNED, REC_NUM_BIG_SIGNED,         instrAndText(CONV_BIGNUM_BIGNUM)},
  {REC_NUM_BIG_UNSIGNED, REC_BYTE_F_ASCII,           instrAndText(CONV_BIGNUM_ASCII)},
  {REC_NUM_BIG_UNSIGNED, REC_BYTE_V_ASCII,           instrAndText(CONV_BIGNUM_ASCII)},
  {REC_NUM_BIG_UNSIGNED, REC_BYTE_V_ASCII_LONG,      instrAndText(CONV_BIGNUM_ASCII)},
  {REC_NUM_BIG_UNSIGNED, REC_NCHAR_F_UNICODE,        instrAndText(CONV_BIGNUM_UNICODE)},
  {REC_NUM_BIG_UNSIGNED, REC_NCHAR_V_UNICODE,        instrAndText(CONV_BIGNUM_UNICODE)},

  {REC_NUM_BIG_SIGNED, REC_BPINT_UNSIGNED,         instrAndText(CONV_BIGNUM_BIN16U)},
  {REC_NUM_BIG_SIGNED, REC_BIN16_SIGNED,           instrAndText(CONV_BIGNUM_BIN16S)},
  {REC_NUM_BIG_SIGNED, REC_BIN16_UNSIGNED,         instrAndText(CONV_BIGNUM_BIN16U)},
  {REC_NUM_BIG_SIGNED, REC_BIN32_SIGNED,           instrAndText(CONV_BIGNUM_BIN32S)},
  {REC_NUM_BIG_SIGNED, REC_BIN32_UNSIGNED,         instrAndText(CONV_BIGNUM_BIN32U)},
  {REC_NUM_BIG_SIGNED, REC_BIN64_SIGNED,           instrAndText(CONV_BIGNUM_BIN64S)},
  {REC_NUM_BIG_SIGNED, REC_BIN64_UNSIGNED,         instrAndText(CONV_BIGNUM_BIN64U)},
  {REC_NUM_BIG_SIGNED, REC_DECIMAL_LSE,            instrAndText(CONV_BIGNUM_DECS)},
  {REC_NUM_BIG_SIGNED, REC_DECIMAL_UNSIGNED,       instrAndText(CONV_BIGNUM_DECU)},
  {REC_NUM_BIG_SIGNED, REC_FLOAT32,                instrAndText(CONV_BIGNUM_FLOAT32)},
  {REC_NUM_BIG_SIGNED, REC_FLOAT64,                instrAndText(CONV_BIGNUM_FLOAT64)},
  {REC_NUM_BIG_SIGNED, REC_NUM_BIG_UNSIGNED,       instrAndText(CONV_BIGNUM_BIGNUM)},
  {REC_NUM_BIG_SIGNED, REC_NUM_BIG_SIGNED,         instrAndText(CONV_BIGNUM_BIGNUM)},
  {REC_NUM_BIG_SIGNED, REC_BYTE_F_ASCII,           instrAndText(CONV_BIGNUM_ASCII)},
  {REC_NUM_BIG_SIGNED, REC_BYTE_V_ASCII,           instrAndText(CONV_BIGNUM_ASCII)},
  {REC_NUM_BIG_SIGNED, REC_BYTE_V_ASCII_LONG,      instrAndText(CONV_BIGNUM_ASCII)},
  {REC_NUM_BIG_SIGNED, REC_NCHAR_F_UNICODE,        instrAndText(CONV_BIGNUM_UNICODE)},
  {REC_NUM_BIG_SIGNED, REC_NCHAR_V_UNICODE,        instrAndText(CONV_BIGNUM_UNICODE)},

  // leading sign seperate decimal is identical to ascii representation.
  // Thus, use instrAndText(CONV_ASCII_DEC) to convert it to decimal
  {REC_DECIMAL_LS,         REC_DECIMAL_LSE,        instrAndText(CONV_ASCII_DEC)},
  {REC_DECIMAL_LS,         REC_DECIMAL_LS,         instrAndText(CONV_DECLS_DECLS)},
  {REC_DECIMAL_LS,         REC_BYTE_F_ASCII,       instrAndText(CONV_DECLS_ASCII)},
  {REC_DECIMAL_LS,         REC_DECIMAL_UNSIGNED,   instrAndText(CONV_DECLS_DECU)},
    
  {REC_DATETIME,           REC_DATETIME,           instrAndText(CONV_DATETIME_DATETIME)},
  {REC_DATETIME,           REC_BYTE_F_ASCII,       instrAndText(CONV_DATETIME_ASCII)},
  {REC_DATETIME,           REC_BYTE_V_ASCII,       instrAndText(CONV_DATETIME_ASCII)},
  {REC_DATETIME,           REC_BYTE_V_ASCII_LONG,  instrAndText(CONV_DATETIME_ASCII)},
  {REC_DATETIME,           REC_NCHAR_F_UNICODE,     instrAndText(CONV_DATETIME_UNICODE)},
  {REC_DATETIME,           REC_NCHAR_V_UNICODE,     instrAndText(CONV_DATETIME_UNICODE)},

  {REC_DATETIME,           REC_BIN64_SIGNED,       instrAndText(CONV_DATETIME_BIN64S)},

  {REC_INT_YEAR,           REC_INT_MONTH,          instrAndText(CONV_INTERVALY_INTERVALMO)},

  {REC_INT_YEAR,      REC_BYTE_F_ASCII,      instrAndText(CONV_INTERVAL_ASCII)},
  {REC_INT_YEAR,      REC_BYTE_V_ASCII,      instrAndText(CONV_INTERVAL_ASCII)},
  {REC_INT_YEAR,      REC_NCHAR_F_UNICODE,   instrAndText(CONV_INTERVAL_UNICODE)},
  {REC_INT_YEAR,      REC_NCHAR_V_UNICODE,   instrAndText(CONV_INTERVAL_UNICODE)},

  {REC_INT_MONTH,          REC_INT_YEAR,           instrAndText(CONV_INTERVALMO_INTERVALY)},
     
  {REC_INT_DAY,            REC_INT_HOUR,           instrAndText(CONV_INTERVALD_INTERVALH)},
  {REC_INT_DAY,            REC_INT_MINUTE,         instrAndText(CONV_INTERVALD_INTERVALM)},
  {REC_INT_DAY,            REC_INT_SECOND,         instrAndText(CONV_INTERVALD_INTERVALS)},

  {REC_INT_HOUR,           REC_INT_DAY,            instrAndText(CONV_INTERVALH_INTERVALD)},
  {REC_INT_HOUR,           REC_INT_MINUTE,         instrAndText(CONV_INTERVALH_INTERVALM)},
  {REC_INT_HOUR,           REC_INT_SECOND,         instrAndText(CONV_INTERVALH_INTERVALS)},

  {REC_INT_MINUTE,         REC_INT_DAY,            instrAndText(CONV_INTERVALM_INTERVALD)},
  {REC_INT_MINUTE,         REC_INT_HOUR,           instrAndText(CONV_INTERVALM_INTERVALH)},
  {REC_INT_MINUTE,         REC_INT_SECOND,         instrAndText(CONV_INTERVALM_INTERVALS)},

  {REC_INT_SECOND,         REC_INT_DAY,            instrAndText(CONV_INTERVALS_INTERVALD)},
  {REC_INT_SECOND,         REC_INT_HOUR,           instrAndText(CONV_INTERVALS_INTERVALH)},
  {REC_INT_SECOND,         REC_INT_MINUTE,         instrAndText(CONV_INTERVALS_INTERVALM)},

  // on the next one, we have to consider scale difference also to distinguish
  // between CONV_INTERVALS_INTERVALS_DIV and CONV_INTERVALS_INTERVALS_MULT
  {REC_INT_SECOND,         REC_INT_SECOND,         instrAndText(CONV_INTERVALS_INTERVALS_DIV)},
  {REC_INT_SECOND,         REC_INT_SECOND,         instrAndText(CONV_INTERVALS_INTERVALS_MULT)},
 
  {REC_BYTE_F_ASCII,       REC_BPINT_UNSIGNED,         instrAndText(CONV_ASCII_BIN16U)},
  {REC_BYTE_F_ASCII,       REC_BIN8_SIGNED,            instrAndText(CONV_ASCII_BIN8S)},
  {REC_BYTE_F_ASCII,       REC_BIN8_UNSIGNED,          instrAndText(CONV_ASCII_BIN8U)},
  {REC_BYTE_F_ASCII,       REC_BIN16_SIGNED,           instrAndText(CONV_ASCII_BIN16S)},
  {REC_BYTE_F_ASCII,       REC_BIN16_UNSIGNED,         instrAndText(CONV_ASCII_BIN16U)},    
  {REC_BYTE_F_ASCII,       REC_BIN32_SIGNED,           instrAndText(CONV_ASCII_BIN32S)},
  {REC_BYTE_F_ASCII,       REC_BIN32_UNSIGNED,         instrAndText(CONV_ASCII_BIN32U)},
  {REC_BYTE_F_ASCII,       REC_BIN64_SIGNED,           instrAndText(CONV_ASCII_BIN64S)},
  {REC_BYTE_F_ASCII,       REC_BIN64_UNSIGNED,         instrAndText(CONV_ASCII_BIN64U)},
  {REC_BYTE_F_ASCII,       REC_DECIMAL_LSE,            instrAndText(CONV_ASCII_DEC)},
  {REC_BYTE_F_ASCII,       REC_DECIMAL_UNSIGNED,       instrAndText(CONV_ASCII_DEC)},

  {REC_BYTE_F_ASCII,       REC_FLOAT32,                instrAndText(CONV_ASCII_FLOAT32)},
  {REC_BYTE_F_ASCII,       REC_FLOAT64,                instrAndText(CONV_ASCII_FLOAT64)},

  {REC_BYTE_F_ASCII,       REC_NUM_BIG_UNSIGNED,       instrAndText(CONV_ASCII_BIGNUM)},
  {REC_BYTE_F_ASCII,       REC_NUM_BIG_SIGNED,         instrAndText(CONV_ASCII_BIGNUM)},
  {REC_BYTE_F_ASCII,       REC_BYTE_F_ASCII,           instrAndText(CONV_ASCII_F_F)},
  {REC_BYTE_F_ASCII,       REC_BYTE_V_ASCII,           instrAndText(CONV_ASCII_F_V)},
  {REC_BYTE_F_ASCII,       REC_BYTE_V_ASCII_LONG,      instrAndText(CONV_ASCII_F_V)},
  {REC_BYTE_F_ASCII,       REC_BYTE_V_ANSI,            instrAndText(CONV_ASCII_TO_ANSI_V)}, 
  {REC_BYTE_F_ASCII,       REC_DATETIME,               instrAndText(CONV_ASCII_DATETIME)},

  {REC_BYTE_F_ASCII,       REC_INT_YEAR,               instrAndText(CONV_ASCII_INTERVAL)},
  {REC_BYTE_F_ASCII,       REC_NCHAR_F_UNICODE,        instrAndText(CONV_ASCII_UNICODE_F)},
  {REC_BYTE_F_ASCII,       REC_NCHAR_V_UNICODE,        instrAndText(CONV_ASCII_UNICODE_V)},
  {REC_BYTE_F_ASCII,       REC_NCHAR_V_ANSI_UNICODE,   instrAndText(CONV_ASCII_TO_ANSI_V_UNICODE)},

  {REC_BYTE_F_ASCII,      REC_BOOLEAN,                 instrAndText(CONV_ASCII_BOOL)},
  {REC_NCHAR_F_UNICODE,    REC_NCHAR_F_UNICODE,        instrAndText(CONV_UNICODE_F_F)},
  {REC_NCHAR_F_UNICODE,    REC_NCHAR_V_UNICODE,        instrAndText(CONV_UNICODE_F_V)},
  {REC_NCHAR_F_UNICODE,    REC_BYTE_F_ASCII,        instrAndText(CONV_UNICODE_F_ASCII_F)},
  {REC_NCHAR_F_UNICODE,    REC_MBYTE_F_SJIS,        instrAndText(CONV_UNICODE_F_SJIS_F)},
  {REC_NCHAR_F_UNICODE,    REC_MBYTE_V_SJIS,        instrAndText(CONV_UNICODE_F_SJIS_V)},
  {REC_NCHAR_F_UNICODE,    REC_MBYTE_LOCALE_F, instrAndText(CONV_UNICODE_F_MBYTE_LOCALE_F)},
  {REC_NCHAR_F_UNICODE,    REC_SBYTE_LOCALE_F, instrAndText(CONV_UNICODE_F_SBYTE_LOCALE_F)},
  {REC_NCHAR_F_UNICODE,    REC_DATETIME,         instrAndText(CONV_UNICODE_DATETIME)},
  {REC_NCHAR_F_UNICODE,    REC_BIN16_SIGNED,     instrAndText(CONV_UNICODE_BIN16S)},
  {REC_NCHAR_F_UNICODE,    REC_BIN16_UNSIGNED,   instrAndText(CONV_UNICODE_BIN16U)},
  {REC_NCHAR_F_UNICODE,    REC_BPINT_UNSIGNED,   instrAndText(CONV_UNICODE_BIN16U)},
  {REC_NCHAR_F_UNICODE,    REC_BIN32_UNSIGNED,   instrAndText(CONV_UNICODE_BIN32U)},
  {REC_NCHAR_F_UNICODE,    REC_BIN32_SIGNED,     instrAndText(CONV_UNICODE_BIN32S)},
  {REC_NCHAR_F_UNICODE,    REC_BIN64_SIGNED,     instrAndText(CONV_UNICODE_BIN64S)},
  {REC_NCHAR_F_UNICODE,    REC_FLOAT32,          instrAndText(CONV_UNICODE_FLOAT32)},
  {REC_NCHAR_F_UNICODE,    REC_FLOAT64,          instrAndText(CONV_UNICODE_FLOAT64)},
  {REC_NCHAR_F_UNICODE,    REC_DECIMAL_LSE,      instrAndText(CONV_UNICODE_DEC)},
  {REC_NCHAR_F_UNICODE,    REC_DECIMAL_UNSIGNED, instrAndText(CONV_UNICODE_DEC)},
  {REC_NCHAR_F_UNICODE,    REC_NUM_BIG_UNSIGNED,       instrAndText(CONV_UNICODE_BIGNUM)},
  {REC_NCHAR_F_UNICODE,    REC_NUM_BIG_SIGNED,         instrAndText(CONV_UNICODE_BIGNUM)},
  {REC_NCHAR_F_UNICODE,    REC_INT_YEAR,               instrAndText(CONV_UNICODE_INTERVAL)},
  {REC_NCHAR_F_UNICODE,    REC_BYTE_V_ASCII,     instrAndText(CONV_UNICODE_F_ASCII_V)},
  {REC_NCHAR_F_UNICODE, REC_NCHAR_V_ANSI_UNICODE,instrAndText(CONV_UNICODE_TO_ANSI_V_UNICODE)},
  {REC_NCHAR_F_UNICODE,    REC_BYTE_V_ANSI, instrAndText(CONV_UNICODE_F_ANSI_V)},

  {REC_NCHAR_V_UNICODE,    REC_BYTE_F_ASCII,        instrAndText(CONV_UNICODE_V_ASCII_F)},
  {REC_NCHAR_V_UNICODE,    REC_MBYTE_F_SJIS,        instrAndText(CONV_UNICODE_V_SJIS_F)},
  {REC_NCHAR_V_UNICODE,    REC_MBYTE_V_SJIS,        instrAndText(CONV_UNICODE_V_SJIS_V)},
  {REC_NCHAR_V_UNICODE,    REC_NCHAR_F_UNICODE,     instrAndText(CONV_UNICODE_V_F)},
  {REC_NCHAR_V_UNICODE,    REC_NCHAR_V_UNICODE,     instrAndText(CONV_UNICODE_V_V)},
  {REC_NCHAR_V_UNICODE,    REC_MBYTE_LOCALE_F, instrAndText(CONV_UNICODE_V_MBYTE_LOCALE_F)},
  {REC_NCHAR_V_UNICODE,    REC_SBYTE_LOCALE_F, instrAndText(CONV_UNICODE_V_SBYTE_LOCALE_F)},
  {REC_NCHAR_V_UNICODE,    REC_DATETIME,       instrAndText(CONV_UNICODE_DATETIME)},
  {REC_NCHAR_V_UNICODE,    REC_BIN16_SIGNED,   instrAndText(CONV_UNICODE_BIN16S)},
  {REC_NCHAR_V_UNICODE,    REC_BIN16_UNSIGNED,   instrAndText(CONV_UNICODE_BIN16U)},
  {REC_NCHAR_V_UNICODE,    REC_BPINT_UNSIGNED,   instrAndText(CONV_UNICODE_BIN16U)},
  {REC_NCHAR_V_UNICODE,    REC_BIN32_UNSIGNED,   instrAndText(CONV_UNICODE_BIN32U)},
  {REC_NCHAR_V_UNICODE,    REC_BIN32_SIGNED,     instrAndText(CONV_UNICODE_BIN32S)},
  {REC_NCHAR_V_UNICODE,    REC_BIN64_SIGNED,     instrAndText(CONV_UNICODE_BIN64S)},
  {REC_NCHAR_V_UNICODE,    REC_FLOAT32,          instrAndText(CONV_UNICODE_FLOAT32)},
  {REC_NCHAR_V_UNICODE,    REC_FLOAT64,          instrAndText(CONV_UNICODE_FLOAT64)},
  {REC_NCHAR_V_UNICODE,    REC_DECIMAL_LSE,      instrAndText(CONV_UNICODE_DEC)},
  {REC_NCHAR_V_UNICODE,    REC_DECIMAL_UNSIGNED, instrAndText(CONV_UNICODE_DEC)},
  {REC_NCHAR_V_UNICODE,    REC_NUM_BIG_UNSIGNED, instrAndText(CONV_UNICODE_BIGNUM)},
  {REC_NCHAR_V_UNICODE,    REC_NUM_BIG_SIGNED,   instrAndText(CONV_UNICODE_BIGNUM)},
  {REC_NCHAR_V_UNICODE,    REC_INT_YEAR,         instrAndText(CONV_UNICODE_INTERVAL)},
  {REC_NCHAR_V_UNICODE,    REC_BYTE_V_ASCII,     instrAndText(CONV_UNICODE_V_ASCII_V)},
  {REC_NCHAR_V_UNICODE,    REC_NCHAR_V_ANSI_UNICODE,instrAndText(CONV_UNICODE_TO_ANSI_V_UNICODE)},
  {REC_NCHAR_V_UNICODE,    REC_BYTE_V_ANSI,      instrAndText(CONV_UNICODE_V_ANSI_V)},


  {REC_BYTE_V_ASCII,       REC_BPINT_UNSIGNED,         instrAndText(CONV_ASCII_BIN16U)},    
  {REC_BYTE_V_ASCII,       REC_BIN8_SIGNED,            instrAndText(CONV_ASCII_BIN8S)},
  {REC_BYTE_V_ASCII,       REC_BIN8_UNSIGNED,          instrAndText(CONV_ASCII_BIN8U)},
  {REC_BYTE_V_ASCII,       REC_BIN16_SIGNED,           instrAndText(CONV_ASCII_BIN16S)},
  {REC_BYTE_V_ASCII,       REC_BIN16_UNSIGNED,         instrAndText(CONV_ASCII_BIN16U)},    
  {REC_BYTE_V_ASCII,       REC_BIN32_SIGNED,           instrAndText(CONV_ASCII_BIN32S)},
  {REC_BYTE_V_ASCII,       REC_BIN32_UNSIGNED,         instrAndText(CONV_ASCII_BIN32U)},
  {REC_BYTE_V_ASCII,       REC_BIN64_SIGNED,           instrAndText(CONV_ASCII_BIN64S)},
  {REC_BYTE_V_ASCII,       REC_BIN64_UNSIGNED,         instrAndText(CONV_ASCII_BIN64U)},
  {REC_BYTE_V_ASCII,       REC_DECIMAL_LSE,            instrAndText(CONV_ASCII_DEC)},
  {REC_BYTE_V_ASCII,       REC_DECIMAL_UNSIGNED,       instrAndText(CONV_ASCII_DEC)},

  {REC_BYTE_V_ASCII,       REC_FLOAT32,                instrAndText(CONV_ASCII_FLOAT32)},
  {REC_BYTE_V_ASCII,       REC_FLOAT64,                instrAndText(CONV_ASCII_FLOAT64)},

  {REC_BYTE_V_ASCII,       REC_DATETIME,               instrAndText(CONV_ASCII_DATETIME)},
  {REC_BYTE_V_ASCII,       REC_NUM_BIG_UNSIGNED,       instrAndText(CONV_ASCII_BIGNUM)},
  {REC_BYTE_V_ASCII,       REC_NUM_BIG_SIGNED,         instrAndText(CONV_ASCII_BIGNUM)},
  {REC_BYTE_V_ASCII,       REC_BYTE_F_ASCII,           instrAndText(CONV_ASCII_V_F)},
  {REC_BYTE_V_ASCII,       REC_BYTE_V_ASCII,           instrAndText(CONV_ASCII_V_V)},
  {REC_BYTE_V_ASCII,       REC_BYTE_V_ASCII_LONG,      instrAndText(CONV_ASCII_V_V)},
  {REC_BYTE_V_ASCII,       REC_BYTE_V_ANSI,            instrAndText(CONV_ASCII_TO_ANSI_V)},

  {REC_BYTE_V_ASCII,       REC_INT_YEAR,               instrAndText(CONV_ASCII_INTERVAL)},

  {REC_BYTE_V_ASCII,       REC_NCHAR_F_UNICODE,        instrAndText(CONV_ASCII_UNICODE_F)},
  {REC_BYTE_V_ASCII,       REC_NCHAR_V_UNICODE,        instrAndText(CONV_ASCII_UNICODE_V)},
  {REC_BYTE_V_ASCII,       REC_NCHAR_V_ANSI_UNICODE,   instrAndText(CONV_ASCII_TO_ANSI_V_UNICODE)},

  {REC_BYTE_V_ASCII,      REC_BOOLEAN,                 instrAndText(CONV_ASCII_BOOL)},

  {REC_BYTE_V_ASCII_LONG,   REC_BPINT_UNSIGNED,         instrAndText(CONV_ASCII_BIN16U)},  
  {REC_BYTE_V_ASCII_LONG,   REC_BIN16_SIGNED,           instrAndText(CONV_ASCII_BIN16S)},
  {REC_BYTE_V_ASCII_LONG,   REC_BIN16_UNSIGNED,         instrAndText(CONV_ASCII_BIN16U)},    
  {REC_BYTE_V_ASCII_LONG,   REC_BIN32_SIGNED,           instrAndText(CONV_ASCII_BIN32S)},
  {REC_BYTE_V_ASCII_LONG,   REC_BIN32_UNSIGNED,         instrAndText(CONV_ASCII_BIN32U)},
  {REC_BYTE_V_ASCII_LONG,   REC_BIN64_SIGNED,           instrAndText(CONV_ASCII_BIN64S)},
  {REC_BYTE_V_ASCII_LONG,   REC_DECIMAL_LSE,            instrAndText(CONV_ASCII_DEC)},
  {REC_BYTE_V_ASCII_LONG,   REC_DECIMAL_UNSIGNED,       instrAndText(CONV_ASCII_DEC)},

  {REC_BYTE_V_ASCII_LONG,   REC_FLOAT32,                instrAndText(CONV_ASCII_FLOAT32)},
  {REC_BYTE_V_ASCII_LONG,   REC_FLOAT64,                instrAndText(CONV_ASCII_FLOAT64)},

  {REC_BYTE_V_ASCII_LONG,   REC_DATETIME,               instrAndText(CONV_ASCII_DATETIME)},
  {REC_BYTE_V_ASCII_LONG,   REC_NUM_BIG_UNSIGNED,       instrAndText(CONV_ASCII_BIGNUM)},
  {REC_BYTE_V_ASCII_LONG,   REC_NUM_BIG_SIGNED,         instrAndText(CONV_ASCII_BIGNUM)},
  {REC_BYTE_V_ASCII_LONG,   REC_BYTE_F_ASCII,           instrAndText(CONV_ASCII_V_F)},
  {REC_BYTE_V_ASCII_LONG,   REC_BYTE_V_ASCII,           instrAndText(CONV_ASCII_V_V)},
  {REC_BYTE_V_ASCII_LONG,   REC_BYTE_V_ASCII_LONG,      instrAndText(CONV_ASCII_V_V)},
  {REC_BYTE_V_ASCII_LONG,   REC_BYTE_V_ANSI,            instrAndText(CONV_ASCII_TO_ANSI_V)},

  {REC_BYTE_V_ANSI,        REC_BYTE_F_ASCII,       instrAndText(CONV_ANSI_V_TO_ASCII_F)},
  {REC_BYTE_V_ANSI,        REC_BYTE_V_ASCII,       instrAndText(CONV_ANSI_V_TO_ASCII_V)},
  {REC_BYTE_V_ANSI,        REC_BYTE_V_ASCII_LONG,  instrAndText(CONV_ANSI_V_TO_ASCII_V)},
  {REC_BYTE_V_ANSI,        REC_BYTE_V_ANSI,        instrAndText(CONV_ANSI_V_TO_ANSI_V)},

  {REC_NCHAR_V_ANSI_UNICODE, REC_NCHAR_V_UNICODE, instrAndText(CONV_ANSI_V_UNICODE_TO_UNICODE_V)},
  {REC_NCHAR_V_ANSI_UNICODE, REC_NCHAR_V_ANSI_UNICODE, instrAndText(CONV_ANSI_V_UNICODE_TO_ANSI_V_UNICODE)},
  {REC_NCHAR_V_ANSI_UNICODE, REC_BYTE_V_ASCII, instrAndText(CONV_ANSI_V_UNICODE_TO_ASCII_V)},
  {REC_NCHAR_V_ANSI_UNICODE, REC_BYTE_F_ASCII, instrAndText(CONV_ANSI_V_UNICODE_TO_ASCII_F)},
  {REC_NCHAR_V_ANSI_UNICODE, REC_NCHAR_F_UNICODE, instrAndText(CONV_ANSI_V_UNICODE_TO_UNICODE_F)},
  {REC_NCHAR_V_ANSI_UNICODE, REC_NCHAR_V_UNICODE, instrAndText(CONV_ANSI_V_UNICODE_TO_UNICODE_V)},

  {REC_BLOB,          REC_BLOB,              instrAndText(CONV_BLOB_BLOB)},     
  {REC_BLOB,          REC_BYTE_F_ASCII,      instrAndText(CONV_BLOB_ASCII_F)},

  {REC_CLOB,          REC_CLOB,              instrAndText(CONV_BLOB_BLOB)},
  {REC_CLOB,          REC_BYTE_F_ASCII,      instrAndText(CONV_BLOB_ASCII_F)},

  {REC_BOOLEAN,       REC_BOOLEAN,           instrAndText(CONV_BOOL_BOOL)},
  {REC_BOOLEAN,       REC_BYTE_F_ASCII,      instrAndText(CONV_BOOL_ASCII)},
  {REC_BOOLEAN,       REC_BYTE_V_ASCII,      instrAndText(CONV_BOOL_ASCII)},

  {REC_BINARY_STRING, REC_BINARY_STRING,     instrAndText(CONV_BINARY_TO_BINARY)},
  {REC_BINARY_STRING, REC_VARBINARY_STRING,  instrAndText(CONV_BINARY_TO_VARBINARY)},
  {REC_BINARY_STRING, REC_UNKNOWN,           instrAndText(CONV_BINARY_TO_OTHER)},

  {REC_VARBINARY_STRING,REC_BINARY_STRING,   instrAndText(CONV_VARBINARY_TO_BINARY)},
  {REC_VARBINARY_STRING,REC_VARBINARY_STRING,instrAndText(CONV_VARBINARY_TO_VARBINARY)},
  {REC_VARBINARY_STRING,REC_UNKNOWN,         instrAndText(CONV_VARBINARY_TO_OTHER)},

  {REC_UNKNOWN,       REC_BINARY_STRING,     instrAndText(CONV_OTHER_TO_BINARY)},
  {REC_UNKNOWN,       REC_VARBINARY_STRING,  instrAndText(CONV_OTHER_TO_VARBINARY)},


};

Lng32 ex_conv_clause::findIndexIntoInstrArray(ConvInstruction ci)
{
  Int32 max_array_size = sizeof(convInstrInfo) / sizeof(ConvInstrStruct);
  
  Int32 i = 0;
  while ((i < max_array_size) && 
         (convInstrInfo[i].instruction != ci)) 
    {
      i++;
    }

  if (i < max_array_size)
    return i;

  return -1; // not found
}

bool   ex_conv_clause::sv_instrOffsetIndexPopulated = false;
short  ex_conv_clause::sv_MaxOpTypeValue = -1;
int   *ex_conv_clause::sv_convIndexSparse = 0;

/* 
 * Scans convInstrInfo[] and populates a 
 * sparsely populated index: sv_convIndexSparse. 
 * 
 * For a given op type: op_type, 
 * if (sv_convIndexSparse[op_type>] != -1) then 
 * it contains the offset into the first row of 
 * convInstrInfo[]
 */
void ex_conv_clause::populateInstrOffsetIndex()
{
  if (sv_instrOffsetIndexPopulated) {
    return;
  }

  typedef struct convInstrIndexStruct {
    short type_op1;
    short offset;
  } convInstrIndexDef;
  
  short lv_MaxIndex = -1;

  // Currently, the number of distinct 'type_op1' values in the convInstrInf[] is 32
  const int lv_MaxIndexCapacity = 100;
  convInstrIndexDef lv_convIndex[lv_MaxIndexCapacity];

  Int32 i = 0;
  Int32 lv_source_type = -1;
  Int32 lv_max_array_size = sizeof(ex_conv_clause::convInstrInfo) / sizeof(ex_conv_clause::ConvInstrStruct);
  
  // collect distinct type_op1 values and their 1st offset in convInstrInfo[]
  while (i < lv_max_array_size) {
    if (ex_conv_clause::convInstrInfo[i].type_op1 != lv_source_type) {
      lv_source_type = ex_conv_clause::convInstrInfo[i].type_op1;
      if ((lv_source_type > -1) && 
	  (lv_MaxIndex < (lv_MaxIndexCapacity - 1))) {
	lv_convIndex[++lv_MaxIndex].type_op1 = lv_source_type;
	lv_convIndex[lv_MaxIndex].offset = i;
      }
    }
    i++;
  }

  // Find the max op type value in lv_convIndex
  int j = 0;
  while (j <= lv_MaxIndex) {
    if (sv_MaxOpTypeValue < lv_convIndex[j].type_op1) {
      sv_MaxOpTypeValue = lv_convIndex[j].type_op1;
    }
    j++;
  }

  // just in case some op type has been assigned a pretty large value
  // we dont want to allocate a very large sparse value
  if (sv_MaxOpTypeValue > 8000) {
    sv_MaxOpTypeValue = 8000;
  }
  
  // Allocate a sparsely populated array
  int *lv_convIndexSparse = (int *) calloc(sizeof(int), 
					   (sv_MaxOpTypeValue+1));
  // Initialize to -1
  for (j = 0; j <= sv_MaxOpTypeValue; j++) {
    lv_convIndexSparse[j] = -1;
  }
  
  // Setup the sparsely populated array
  j = 0;
  while (j <= lv_MaxIndex) {
    lv_convIndexSparse[lv_convIndex[j].type_op1] = lv_convIndex[j].offset;
    j++;
  }

  sv_convIndexSparse = lv_convIndexSparse;
  sv_instrOffsetIndexPopulated = true;
}

/* returns the offset into the convInstrInfo[] (the first row where the 
 * src data type equals the parameter pv_op1)
 */
int ex_conv_clause::getInstrOffset(short pv_op1)
{
  if ((pv_op1 < 0) || 
      (pv_op1 > sv_MaxOpTypeValue)) {
    return -1;
  }

  return sv_convIndexSparse[pv_op1];
}

ConvInstruction ex_conv_clause::findInstruction(short sourceType, Lng32 sourceLen,
                                                short targetType, Lng32 targetLen,
                                                Lng32 scaleDifference)
{
  ConvInstruction instruction = CONV_NOT_SUPPORTED;
  setInstrArrayIndex(-1);

  getConvCaseDatatypes(sourceType, sourceLen, sourceType, 
                       targetType, targetLen, targetType, scaleDifference);

  Int32 max_array_size = sizeof(convInstrInfo) / sizeof(ConvInstrStruct);

  Int32 i = 0;
  i = getInstrOffset(sourceType);
  if (i < 0) {
    i = 0;
    while ((i < max_array_size) && (convInstrInfo[i].type_op1 != sourceType)) {
      i++;
    }
  }

  NABoolean done = FALSE;
  while ((!done) && (i < max_array_size)) {
    if (convInstrInfo[i].type_op1 != sourceType)
      done = TRUE;
    else if (convInstrInfo[i].type_op2 == targetType) {
      done = TRUE;
      instruction = convInstrInfo[i].instruction;
      setInstrArrayIndex(i);
    }
    else
      i++;
  }; // while

  if (instruction == CONV_INTERVALS_INTERVALS_DIV)
    {

      // have to look at scale difference to distinguish whether
      // to divide or multiply by the scale factor
      if (scaleDifference < 0)
        {
          instruction = CONV_INTERVALS_INTERVALS_MULT;
          setInstrArrayIndex(findIndexIntoInstrArray(instruction));
        }
    }
  
  // if this is a conversion from numeric to tinyint and not handled
  // above, then add a generic conv to tinyint index.
  // At runtime, actual conversion will be done based on source datatype.
  // This is done to handle the case where target is a key column used
  // in a predicate and conversion is being done to its datatype.
  // In this case, we dont want to return a conversion error.
  // See handling of dataConversionErrorFlag in exp_conv.cpp.
  if (instruction == CONV_NOT_SUPPORTED)
    {
      if (targetType == REC_BINARY_STRING)
        {
          instruction = CONV_OTHER_TO_BINARY;
          
          setInstrArrayIndex(findIndexIntoInstrArray(instruction));
        }
      else if (targetType == REC_VARBINARY_STRING)
        {
          instruction = CONV_OTHER_TO_VARBINARY;
          
          setInstrArrayIndex(findIndexIntoInstrArray(instruction));
        }
      else if (sourceType == REC_BINARY_STRING)
        {
          instruction = CONV_BINARY_TO_OTHER;
          
          setInstrArrayIndex(findIndexIntoInstrArray(instruction));
        }
      else if (sourceType == REC_VARBINARY_STRING)
        {
          instruction = CONV_VARBINARY_TO_OTHER;
          
          setInstrArrayIndex(findIndexIntoInstrArray(instruction));
        }
      else if ((DFS2REC::isNumeric(sourceType)) &&
               (DFS2REC::isTinyint(targetType)))
        {
          if (targetType == REC_BIN8_SIGNED)
            {
              instruction = CONV_NUMERIC_BIN8S;
            }
          else
            {
              instruction = CONV_NUMERIC_BIN8U;
            }

          setInstrArrayIndex(findIndexIntoInstrArray(instruction));
        }
    }

  return (enum ConvInstruction)instruction;

};

NABoolean ex_conv_clause::isConversionSupported
(short sourceType, Lng32 sourceLen, short targetType, Lng32 targetLen)
{
  ConvInstruction ci = 
    findInstruction(sourceType, sourceLen, targetType, targetLen, 0);
  if (ci == CONV_NOT_SUPPORTED)
    return FALSE;
  else
    return TRUE;
}

void ex_conv_clause::setInstruction() {
  ConvInstruction instruction = CONV_NOT_SUPPORTED;

  // Test added to allow conditional union to be rowset-aware
  if((getOperand(0)->getRowsetSize() > 0) && (getOperand(1)->getRowsetSize() > 0)) 
    { 
      instruction = CONV_ASCII_F_F; 
      setInstrArrayIndex(findIndexIntoInstrArray(instruction));

      SimpleType *op0 = (SimpleType *) getOperand(0);
      SimpleType *op1 = (SimpleType *) getOperand(1);
      if (!(op0->getUseTotalRowsetSize())) {
	op0->setLength(sizeof(Lng32) + (op0->getLength() * op0->getRowsetSize()));
	op0->setUseTotalRowsetSize();
	op1->setUseTotalRowsetSize();
      }
      op1->setLength(op0->getLength());
    }
  else
    instruction = findInstruction(getOperand(1)->getDatatype(),
                                  getOperand(1)->getLength(),
                                  getOperand(0)->getDatatype(),
                                  getOperand(0)->getLength(),
                                  getOperand(1)->getScale() - getOperand(0)->getScale());
  
  if (instruction == CONV_NOT_SUPPORTED)
    {
      if ((getOperand(0)->isComplexType()) && !(getOperand(1)->isComplexType()))
        {
          instruction = CONV_SIMPLE_TO_COMPLEX;
          setInstrArrayIndex(findIndexIntoInstrArray(instruction));
        }
      else if (! (getOperand(0)->isComplexType()) && (getOperand(1)->isComplexType()))
        {
          instruction = CONV_COMPLEX_TO_SIMPLE;
          setInstrArrayIndex(findIndexIntoInstrArray(instruction));
        }
    }

  if ((instruction == CONV_NOT_SUPPORTED) &&
      (getInstrArrayIndex() >= 0))
    {
      // this is an error, reset instr array index
      setInstrArrayIndex(-1);
    }
};

ex_expr::exp_return_type ex_conv_clause::fixup(Space * space,
					       CollHeap * exHeap,
					       char * constants_area,
					       char * temps_area,
					       char * persistentArea,
					       short fixupConstsAndTemps,
					       NABoolean spaceCompOnly)
{
  setInstruction();
  return ex_clause::fixup(space, exHeap, constants_area, temps_area,
			  persistentArea,
			  fixupConstsAndTemps, spaceCompOnly);
}
