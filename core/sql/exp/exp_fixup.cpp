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

#include "Platform.h"                                     // NT_PORT SK 02/10/97


#include "exp_stdh.h"
#include "exp_clause_derived.h"
#include "exp_function.h"
#include "exp_interval.h"
#include "SQLTypeDefs.h"

__declspec(dllimport) NABoolean ExExprComputeSpace(ex_tcb * tcb);

NA_EIDPROC
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
    if ( ((type_op2 >= REC_MIN_BINARY) && (type_op2 <= REC_MAX_BINARY)) ||
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
    if ( ((type_op1 >= REC_MIN_BINARY) && (type_op1 <= REC_MAX_BINARY)) ||
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

NA_EIDPROC
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


const ex_arith_struct * ex_arith_clause::getMatchingRow(OperatorTypeEnum op,
							short datatype1,
							short datatype2,
							short resulttype)
{
  static const ex_arith_struct as[] =
    {
      // op       op1 datatype      op2 datatype      result datatype   case statement index
      {ITM_PLUS, REC_BIN16_SIGNED, REC_BIN16_SIGNED, REC_BIN16_SIGNED, ADD_BIN16S_BIN16S_BIN16S},
      {ITM_PLUS, REC_BIN16_SIGNED, REC_BIN16_SIGNED, REC_BIN32_SIGNED, ADD_BIN16S_BIN16S_BIN32S},
      {ITM_PLUS, REC_BIN16_SIGNED, REC_BIN32_SIGNED, REC_BIN32_SIGNED, ADD_BIN16S_BIN32S_BIN32S},
      {ITM_PLUS, REC_BIN32_SIGNED, REC_BIN16_SIGNED, REC_BIN32_SIGNED, ADD_BIN32S_BIN16S_BIN32S},
      {ITM_PLUS, REC_BIN32_SIGNED, REC_BIN32_SIGNED, REC_BIN32_SIGNED, ADD_BIN32S_BIN32S_BIN32S},
      {ITM_PLUS, REC_BIN32_SIGNED, REC_BIN64_SIGNED, REC_BIN64_SIGNED, ADD_BIN32S_BIN64S_BIN64S},
      {ITM_PLUS, REC_BIN64_SIGNED, REC_BIN32_SIGNED, REC_BIN64_SIGNED, ADD_BIN64S_BIN32S_BIN64S},
      {ITM_PLUS, REC_BIN64_SIGNED, REC_BIN64_SIGNED, REC_BIN64_SIGNED, ADD_BIN64S_BIN64S_BIN64S},

      {ITM_PLUS, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, ADD_BIN16U_BIN16U_BIN16U},      
      {ITM_PLUS, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, ADD_BIN16U_BIN16U_BIN16U},
      {ITM_PLUS, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, ADD_BIN16U_BIN16U_BIN16U},
      {ITM_PLUS, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, ADD_BIN16U_BIN16U_BIN32U},
      {ITM_PLUS, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, ADD_BIN16U_BIN16U_BIN32U},
      {ITM_PLUS, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, ADD_BIN16U_BIN16U_BIN32U},
      {ITM_PLUS, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, ADD_BIN16U_BIN32U_BIN32U},
      {ITM_PLUS, REC_BIN32_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, ADD_BIN32U_BIN16U_BIN32U},
      {ITM_PLUS, REC_BPINT_UNSIGNED, REC_BIN64_SIGNED,   REC_BIN64_SIGNED,   ADD_BPINTU_BIN64S_BIN64S},
      {ITM_PLUS, REC_BIN64_SIGNED,   REC_BPINT_UNSIGNED, REC_BIN64_SIGNED,   ADD_BIN64S_BPINTU_BIN64S},

      {ITM_PLUS, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, ADD_BIN16U_BIN16U_BIN16U},
      {ITM_PLUS, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, ADD_BIN16U_BIN16U_BIN32U},
      {ITM_PLUS, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, ADD_BIN16U_BIN32U_BIN32U},
      {ITM_PLUS, REC_BIN32_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, ADD_BIN32U_BIN16U_BIN32U},
      {ITM_PLUS, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, ADD_BIN32U_BIN32U_BIN32U},
      {ITM_PLUS, REC_BIN32_UNSIGNED, REC_BIN64_SIGNED, REC_BIN64_SIGNED, ADD_BIN32U_BIN64S_BIN64S},
      {ITM_PLUS, REC_BIN64_SIGNED, REC_BIN32_UNSIGNED, REC_BIN64_SIGNED, ADD_BIN64S_BIN32U_BIN64S},

      {ITM_PLUS, REC_FLOAT32, REC_FLOAT32, REC_FLOAT32, ADD_FLOAT32_FLOAT32_FLOAT32},
      {ITM_PLUS, REC_FLOAT64, REC_FLOAT64, REC_FLOAT64, ADD_FLOAT64_FLOAT64_FLOAT64},
      {ITM_PLUS, REC_DATETIME, REC_BIN16_SIGNED, REC_DATETIME, ADD_DATETIME_INTERVAL_DATETIME},
      {ITM_PLUS, REC_DATETIME, REC_BIN32_SIGNED, REC_DATETIME, ADD_DATETIME_INTERVAL_DATETIME},
      {ITM_PLUS, REC_DATETIME, REC_BIN64_SIGNED, REC_DATETIME, ADD_DATETIME_INTERVAL_DATETIME},
      {ITM_PLUS, REC_BIN16_SIGNED, REC_DATETIME, REC_DATETIME, ADD_INTERVAL_DATETIME_DATETIME},
      {ITM_PLUS, REC_BIN32_SIGNED, REC_DATETIME, REC_DATETIME, ADD_INTERVAL_DATETIME_DATETIME},
      {ITM_PLUS, REC_BIN64_SIGNED, REC_DATETIME, REC_DATETIME, ADD_INTERVAL_DATETIME_DATETIME},

      
      {ITM_MINUS, REC_BIN16_SIGNED, REC_BIN16_SIGNED, REC_BIN16_SIGNED, SUB_BIN16S_BIN16S_BIN16S},
      {ITM_MINUS, REC_BIN16_SIGNED, REC_BIN16_SIGNED, REC_BIN32_SIGNED, SUB_BIN16S_BIN16S_BIN32S},
      {ITM_MINUS, REC_BIN16_SIGNED, REC_BIN32_SIGNED, REC_BIN32_SIGNED, SUB_BIN16S_BIN32S_BIN32S},
      {ITM_MINUS, REC_BIN32_SIGNED, REC_BIN16_SIGNED, REC_BIN32_SIGNED, SUB_BIN32S_BIN16S_BIN32S},
      {ITM_MINUS, REC_BIN32_SIGNED, REC_BIN32_SIGNED, REC_BIN32_SIGNED, SUB_BIN32S_BIN32S_BIN32S},
      {ITM_MINUS, REC_BIN64_SIGNED, REC_BIN64_SIGNED, REC_BIN64_SIGNED, SUB_BIN64S_BIN64S_BIN64S},
      {ITM_MINUS, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, SUB_BIN16U_BIN16U_BIN16U},
      {ITM_MINUS, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, SUB_BIN16U_BIN16U_BIN16U},
      {ITM_MINUS, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, SUB_BIN16U_BIN16U_BIN16U},
      {ITM_MINUS, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, SUB_BIN16U_BIN16U_BIN32U},
      {ITM_MINUS, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, SUB_BIN16U_BIN16U_BIN32U},
      {ITM_MINUS, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, SUB_BIN16U_BIN16U_BIN32U},
      {ITM_MINUS, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, SUB_BIN16U_BIN32U_BIN32U},
      {ITM_MINUS, REC_BIN32_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, SUB_BIN32U_BIN16U_BIN32U},
      {ITM_MINUS, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, SUB_BIN16U_BIN16U_BIN16U},
      {ITM_MINUS, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, SUB_BIN16U_BIN16U_BIN32U},
      {ITM_MINUS, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, SUB_BIN16U_BIN32U_BIN32U},
      {ITM_MINUS, REC_BIN32_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, SUB_BIN32U_BIN16U_BIN32U},
      {ITM_MINUS, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, SUB_BIN32U_BIN32U_BIN32U},
      {ITM_MINUS, REC_FLOAT32, REC_FLOAT32, REC_FLOAT32, SUB_FLOAT32_FLOAT32_FLOAT32},
      {ITM_MINUS, REC_FLOAT64, REC_FLOAT64, REC_FLOAT64, SUB_FLOAT64_FLOAT64_FLOAT64},
      {ITM_MINUS, REC_DATETIME, REC_BIN16_SIGNED, REC_DATETIME, SUB_DATETIME_INTERVAL_DATETIME},
      {ITM_MINUS, REC_DATETIME, REC_BIN32_SIGNED, REC_DATETIME, SUB_DATETIME_INTERVAL_DATETIME},
      {ITM_MINUS, REC_DATETIME, REC_BIN64_SIGNED, REC_DATETIME, SUB_DATETIME_INTERVAL_DATETIME},
      {ITM_MINUS, REC_DATETIME, REC_DATETIME, REC_BIN16_SIGNED, SUB_DATETIME_DATETIME_INTERVAL},
      {ITM_MINUS, REC_DATETIME, REC_DATETIME, REC_BIN32_SIGNED, SUB_DATETIME_DATETIME_INTERVAL},
      {ITM_MINUS, REC_DATETIME, REC_DATETIME, REC_BIN64_SIGNED, SUB_DATETIME_DATETIME_INTERVAL},


      {ITM_TIMES, REC_BIN16_SIGNED, REC_BIN16_SIGNED, REC_BIN16_SIGNED, MUL_BIN16S_BIN16S_BIN16S},
      {ITM_TIMES, REC_BIN16_SIGNED, REC_BIN16_SIGNED, REC_BIN32_SIGNED, MUL_BIN16S_BIN16S_BIN32S},
      {ITM_TIMES, REC_BIN16_SIGNED, REC_BIN32_SIGNED, REC_BIN32_SIGNED, MUL_BIN16S_BIN32S_BIN32S},
      {ITM_TIMES, REC_BIN32_SIGNED, REC_BIN16_SIGNED, REC_BIN32_SIGNED, MUL_BIN32S_BIN16S_BIN32S},
      {ITM_TIMES, REC_BIN32_SIGNED, REC_BIN32_SIGNED, REC_BIN32_SIGNED, MUL_BIN32S_BIN32S_BIN32S},
      {ITM_TIMES, REC_BIN64_SIGNED, REC_BIN64_SIGNED, REC_BIN64_SIGNED, MUL_BIN64S_BIN64S_BIN64S},
      {ITM_TIMES, REC_BIN16_SIGNED, REC_BIN32_SIGNED, REC_BIN64_SIGNED, MUL_BIN16S_BIN32S_BIN64S},
      {ITM_TIMES, REC_BIN32_SIGNED, REC_BIN16_SIGNED, REC_BIN64_SIGNED, MUL_BIN32S_BIN16S_BIN64S},
      {ITM_TIMES, REC_BIN32_SIGNED, REC_BIN32_SIGNED, REC_BIN64_SIGNED, MUL_BIN32S_BIN32S_BIN64S},
      {ITM_TIMES, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, MUL_BIN16U_BIN16U_BIN16U},      
      {ITM_TIMES, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, MUL_BIN16U_BIN16U_BIN16U},
      {ITM_TIMES, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, MUL_BIN16U_BIN16U_BIN16U},
      {ITM_TIMES, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, MUL_BIN16U_BIN16U_BIN32U},
      {ITM_TIMES, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, MUL_BIN16U_BIN16U_BIN32U},
      {ITM_TIMES, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, MUL_BIN16U_BIN16U_BIN32U},
      {ITM_TIMES, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, MUL_BIN16U_BIN32U_BIN32U},
      {ITM_TIMES, REC_BIN32_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, MUL_BIN32U_BIN16U_BIN32U},
      {ITM_TIMES, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, MUL_BIN16U_BIN16U_BIN16U},
      {ITM_TIMES, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, MUL_BIN16U_BIN16U_BIN32U},
      {ITM_TIMES, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, MUL_BIN16U_BIN32U_BIN32U},
      {ITM_TIMES, REC_BIN32_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, MUL_BIN32U_BIN16U_BIN32U},
      {ITM_TIMES, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, MUL_BIN32U_BIN32U_BIN32U},
      {ITM_TIMES, REC_FLOAT32, REC_FLOAT32, REC_FLOAT32, MUL_FLOAT32_FLOAT32_FLOAT32},
      {ITM_TIMES, REC_FLOAT64, REC_FLOAT64, REC_FLOAT64, MUL_FLOAT64_FLOAT64_FLOAT64},


      {ITM_DIVIDE, REC_BIN16_SIGNED, REC_BIN16_SIGNED, REC_BIN16_SIGNED, DIV_BIN16S_BIN16S_BIN16S},
      {ITM_DIVIDE, REC_BIN16_SIGNED, REC_BIN16_SIGNED, REC_BIN32_SIGNED, DIV_BIN16S_BIN16S_BIN32S},
      {ITM_DIVIDE, REC_BIN16_SIGNED, REC_BIN32_SIGNED, REC_BIN32_SIGNED, DIV_BIN16S_BIN32S_BIN32S},
      {ITM_DIVIDE, REC_BIN32_SIGNED, REC_BIN16_SIGNED, REC_BIN32_SIGNED, DIV_BIN32S_BIN16S_BIN32S},
      {ITM_DIVIDE, REC_BIN32_SIGNED, REC_BIN32_SIGNED, REC_BIN32_SIGNED, DIV_BIN32S_BIN32S_BIN32S},
      {ITM_DIVIDE, REC_BIN64_SIGNED, REC_BIN64_SIGNED, REC_BIN64_SIGNED, DIV_BIN64S_BIN64S_BIN64S},
      {ITM_DIVIDE, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, DIV_BIN16U_BIN16U_BIN16U},
      {ITM_DIVIDE, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, DIV_BIN16U_BIN16U_BIN16U},
      {ITM_DIVIDE, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, DIV_BIN16U_BIN16U_BIN16U},
      {ITM_DIVIDE, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, DIV_BIN16U_BIN16U_BIN32U},
      {ITM_DIVIDE, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, DIV_BIN16U_BIN16U_BIN32U},
      {ITM_DIVIDE, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, DIV_BIN16U_BIN16U_BIN32U},
      {ITM_DIVIDE, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, DIV_BIN16U_BIN32U_BIN32U},
      {ITM_DIVIDE, REC_BIN32_UNSIGNED, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, DIV_BIN32U_BIN16U_BIN32U},
      {ITM_DIVIDE, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, DIV_BIN16U_BIN16U_BIN16U},
      {ITM_DIVIDE, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, DIV_BIN16U_BIN16U_BIN32U},
      {ITM_DIVIDE, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, DIV_BIN16U_BIN32U_BIN32U},
      {ITM_DIVIDE, REC_BIN32_UNSIGNED, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, DIV_BIN32U_BIN16U_BIN32U},
      {ITM_DIVIDE, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, DIV_BIN32U_BIN32U_BIN32U},
      {ITM_DIVIDE, REC_FLOAT64, REC_FLOAT64, REC_FLOAT64, DIV_FLOAT64_FLOAT64_FLOAT64}
    };

  Int32 max_array_size = sizeof(as) / sizeof(ex_arith_struct);

  Int32 i = 0;
  Int32 found = 0;
  while ((i < max_array_size) && (!found))
    if ((as[i].op == op) && 
	(as[i].type_op1 == datatype1) &&
	(as[i].type_op2 == datatype2) &&
	(as[i].type_op0 == resulttype))
      found = -1;
    else
      i++;
  
  if (found)
    return &as[i];  
  else
    return 0;
}

const ex_arith_clause::arith_case_index ex_arith_clause::computeCaseIndex(OperatorTypeEnum op, 
									  Attributes * attr1,
									  Attributes * attr2,
									  Attributes * result)
{
  arith_case_index c_index = ARITH_NOT_SUPPORTED;
  
  short type_op1;
  short type_op2;
  short type_result;
  getCaseDatatypes(attr1->getDatatype(), attr1->getLength(), type_op1,
                   result->getDatatype(), result->getLength(), type_result,
                   0 /* don't need to take scale difference into account here */);
  getCaseDatatypes(attr2->getDatatype(), attr2->getLength(), type_op2,
                   result->getDatatype(), result->getLength(), type_result,
                   0 /* don't need to take scale difference into account here */);

  const ex_arith_struct * as = getMatchingRow(op, 
					      type_op1,
					      type_op2,
					      type_result);
  
  if (as)
    {
      c_index = as->index;

      // for binary numeric and interval datatypes, arith done only if scales
      // are the same and operation is addition or subtraction
      if (((op == ITM_PLUS) || (op == ITM_MINUS)) &&
	  (((result->getDatatype() >= REC_MIN_BINARY) &&
	    (result->getDatatype() <= REC_MAX_BINARY)) ||
	   ((result->getDatatype() >= REC_MIN_INTERVAL) &&
	    (result->getDatatype() <= REC_MAX_INTERVAL))))
	{
	  // numeric or interval types
	  if (attr1->getScale() != attr2->getScale())
	    c_index = ARITH_NOT_SUPPORTED; 
	  
	} // numeric or interval types
      else if ((op == ITM_DIVIDE) &&
	       (arithRoundingMode_ != 0))
	{
	  if (c_index == DIV_BIN64S_BIN64S_BIN64S)
	    c_index = DIV_BIN64S_BIN64S_BIN64S_ROUND;
	  else
	    c_index = ARITH_NOT_SUPPORTED;
	}
    }

  return c_index;
}

void ex_arith_clause::set_case_index()
{
  case_index = ARITH_NOT_SUPPORTED;

  if (getOperand(0)->isComplexType())
    {
      switch (getOperType())
	{
	case ITM_PLUS:      
	  case_index = ADD_COMPLEX;
	  break;
	
	case ITM_MINUS:      
	  case_index = SUB_COMPLEX;
	  break;

  	case ITM_TIMES:      
	  case_index = MUL_COMPLEX;
	  break;

  	case ITM_DIVIDE:      
	  case_index = DIV_COMPLEX;
	  break;

	default:
	  break;
	}
    }      
  else
    {
      // Simple types are handled here.
      case_index = computeCaseIndex(getOperType(), getOperand(1), getOperand(2),
				    getOperand(0));
    }
}

void ex_arith_clause::set_case_index(OperatorTypeEnum op,
				     Attributes * attr1,
				     Attributes * attr2,
				     Attributes * result)
{
  // Only simple types are handled here.
  case_index = computeCaseIndex(op, attr1, attr2, result);
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
  set_case_index();
  return ex_clause::fixup(space, exHeap, constants_area, temps_area,
			  persistentArea,
			  fixupConstsAndTemps, spaceCompOnly);
}

const ex_comp_struct * ex_comp_clause::getMatchingRow(OperatorTypeEnum op,
						      short datatype1,
						      short datatype2)
{
  const static ex_comp_struct cs[] =
    {
      // op       op1 datatype        op2 datatype        case statement index
      {ITM_EQUAL, REC_BIN16_SIGNED,   REC_BIN16_SIGNED,   EQ_BIN16S_BIN16S},
      {ITM_EQUAL, REC_BIN16_SIGNED,   REC_BIN32_SIGNED,   EQ_BIN16S_BIN32S},
      {ITM_EQUAL, REC_BIN16_SIGNED,   REC_BPINT_UNSIGNED, EQ_BIN16S_BIN16U},
      {ITM_EQUAL, REC_BIN16_SIGNED,   REC_BIN16_UNSIGNED, EQ_BIN16S_BIN16U},
      {ITM_EQUAL, REC_BIN16_SIGNED,   REC_BIN32_UNSIGNED, EQ_BIN16S_BIN32U},   // Was BIN32S. Error? ANS

      {ITM_EQUAL, REC_BPINT_UNSIGNED, REC_BIN16_SIGNED,   EQ_BIN16U_BIN16S},
      {ITM_EQUAL, REC_BPINT_UNSIGNED, REC_BIN32_SIGNED,   EQ_BIN16U_BIN32S},
      {ITM_EQUAL, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, EQ_BIN16U_BIN16U},
      {ITM_EQUAL, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, EQ_BIN16U_BIN16U},
      {ITM_EQUAL, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, EQ_BIN16U_BIN32U},

      {ITM_EQUAL, REC_BIN16_UNSIGNED, REC_BIN16_SIGNED,   EQ_BIN16U_BIN16S},
      {ITM_EQUAL, REC_BIN16_UNSIGNED, REC_BIN32_SIGNED,   EQ_BIN16U_BIN32S},
      {ITM_EQUAL, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, EQ_BIN16U_BIN16U},
      {ITM_EQUAL, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, EQ_BIN16U_BIN16U},
      {ITM_EQUAL, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, EQ_BIN16U_BIN32U},    // Was BIN32S. Error? ANS

      {ITM_EQUAL, REC_BIN32_SIGNED,   REC_BIN16_SIGNED,   EQ_BIN32S_BIN16S},
      {ITM_EQUAL, REC_BIN32_SIGNED,   REC_BIN32_SIGNED,   EQ_BIN32S_BIN32S},
      {ITM_EQUAL, REC_BIN32_SIGNED,   REC_BPINT_UNSIGNED, EQ_BIN32S_BIN16U},
      {ITM_EQUAL, REC_BIN32_SIGNED,   REC_BIN16_UNSIGNED, EQ_BIN32S_BIN16U},
      {ITM_EQUAL, REC_BIN32_SIGNED,   REC_BIN32_UNSIGNED, EQ_BIN32S_BIN32U},    // Was BIN32S. Error? ANS

      {ITM_EQUAL, REC_BIN32_UNSIGNED, REC_BIN16_SIGNED,   EQ_BIN32U_BIN16S},
      {ITM_EQUAL, REC_BIN32_UNSIGNED, REC_BIN32_SIGNED,   EQ_BIN32U_BIN32S},
      {ITM_EQUAL, REC_BIN32_UNSIGNED, REC_BPINT_UNSIGNED, EQ_BIN32U_BIN16U},
      {ITM_EQUAL, REC_BIN32_UNSIGNED, REC_BIN16_UNSIGNED, EQ_BIN32U_BIN16U},
      {ITM_EQUAL, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, EQ_BIN32U_BIN32U},    // Was BIN32S. Error? ANS

      {ITM_EQUAL, REC_BIN64_SIGNED,   REC_BIN64_SIGNED,   EQ_BIN64S_BIN64S},

      {ITM_EQUAL, REC_DECIMAL_UNSIGNED, REC_DECIMAL_UNSIGNED, EQ_DECU_DECU},
      {ITM_EQUAL, REC_DECIMAL_LSE,      REC_DECIMAL_LSE,      EQ_DECS_DECS},

      {ITM_EQUAL, REC_FLOAT32,        REC_FLOAT32,        EQ_FLOAT32_FLOAT32},
      {ITM_EQUAL, REC_FLOAT64,        REC_FLOAT64,        EQ_FLOAT64_FLOAT64},

      {ITM_EQUAL, REC_DATETIME,       REC_DATETIME,       EQ_DATETIME_DATETIME},

      {ITM_EQUAL, REC_BYTE_F_ASCII,     REC_BYTE_F_ASCII,     EQ_ASCII_F_F},

      {ITM_EQUAL, REC_BYTE_F_ASCII,       REC_BYTE_V_ASCII,       EQ_ASCII_COMP},
      {ITM_EQUAL, REC_BYTE_V_ASCII,       REC_BYTE_F_ASCII,       EQ_ASCII_COMP},
      {ITM_EQUAL, REC_BYTE_V_ASCII,       REC_BYTE_V_ASCII,       EQ_ASCII_COMP},
      {ITM_EQUAL, REC_BYTE_F_ASCII,       REC_BYTE_V_ASCII_LONG,  EQ_ASCII_COMP},
      {ITM_EQUAL, REC_BYTE_V_ASCII_LONG,  REC_BYTE_F_ASCII,       EQ_ASCII_COMP},
      {ITM_EQUAL, REC_BYTE_V_ASCII,       REC_BYTE_V_ASCII_LONG,  EQ_ASCII_COMP},
      {ITM_EQUAL, REC_BYTE_V_ASCII_LONG,  REC_BYTE_V_ASCII,       EQ_ASCII_COMP},
      {ITM_EQUAL, REC_BYTE_V_ASCII_LONG,  REC_BYTE_V_ASCII_LONG,  EQ_ASCII_COMP},

       {ITM_EQUAL,REC_NCHAR_F_UNICODE,REC_NCHAR_F_UNICODE,EQ_UNICODE_F_F},
       {ITM_EQUAL, REC_NCHAR_F_UNICODE,REC_NCHAR_V_UNICODE,UNICODE_COMP},
       {ITM_EQUAL, REC_NCHAR_V_UNICODE,REC_NCHAR_F_UNICODE,UNICODE_COMP},
       {ITM_EQUAL, REC_NCHAR_V_UNICODE,REC_NCHAR_V_UNICODE,UNICODE_COMP},

       {ITM_EQUAL, REC_BLOB,REC_BLOB,EQ_BLOB},
	
      {ITM_NOT_EQUAL, REC_BIN16_SIGNED,   REC_BIN16_SIGNED,   NE_BIN16S_BIN16S},
      {ITM_NOT_EQUAL, REC_BIN16_SIGNED,   REC_BIN32_SIGNED,   NE_BIN16S_BIN32S},
      {ITM_NOT_EQUAL, REC_BIN16_SIGNED,   REC_BPINT_UNSIGNED, NE_BIN16S_BIN16U},
      {ITM_NOT_EQUAL, REC_BIN16_SIGNED,   REC_BIN16_UNSIGNED, NE_BIN16S_BIN16U},
      {ITM_NOT_EQUAL, REC_BIN16_SIGNED,   REC_BIN32_UNSIGNED, NE_BIN16S_BIN32U}, //Was 32S. Error? ANS

      {ITM_NOT_EQUAL, REC_BPINT_UNSIGNED, REC_BIN16_SIGNED,   NE_BIN16U_BIN16S},
      {ITM_NOT_EQUAL, REC_BPINT_UNSIGNED, REC_BIN32_SIGNED,   NE_BIN16U_BIN32S},
      {ITM_NOT_EQUAL, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, NE_BIN16U_BIN16U},
      {ITM_NOT_EQUAL, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, NE_BIN16U_BIN16U},
      {ITM_NOT_EQUAL, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, NE_BIN16U_BIN32U},  //Was 32S. Error? ANS

      {ITM_NOT_EQUAL, REC_BIN16_UNSIGNED, REC_BIN16_SIGNED,   NE_BIN16U_BIN16S},
      {ITM_NOT_EQUAL, REC_BIN16_UNSIGNED, REC_BIN32_SIGNED,   NE_BIN16U_BIN32S},
      {ITM_NOT_EQUAL, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, NE_BIN16U_BIN16U},
      {ITM_NOT_EQUAL, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, NE_BIN16U_BIN16U},
      {ITM_NOT_EQUAL, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, NE_BIN16U_BIN32U},  //Was 32S. Error? ANS

      {ITM_NOT_EQUAL, REC_BIN32_SIGNED,   REC_BIN16_SIGNED,   NE_BIN32S_BIN16S},
      {ITM_NOT_EQUAL, REC_BIN32_SIGNED,   REC_BIN32_SIGNED,   NE_BIN32S_BIN32S},
      {ITM_NOT_EQUAL, REC_BIN32_SIGNED,   REC_BPINT_UNSIGNED, NE_BIN32S_BIN16U},
      {ITM_NOT_EQUAL, REC_BIN32_SIGNED,   REC_BIN16_UNSIGNED, NE_BIN32S_BIN16U},
      {ITM_NOT_EQUAL, REC_BIN32_SIGNED,   REC_BIN32_UNSIGNED, NE_BIN32S_BIN32U},  //Was 32S. Error? ANS

      {ITM_NOT_EQUAL, REC_BIN32_UNSIGNED, REC_BIN16_SIGNED,   NE_BIN32U_BIN16S},
      {ITM_NOT_EQUAL, REC_BIN32_UNSIGNED, REC_BIN32_SIGNED,   NE_BIN32U_BIN32S},
      {ITM_NOT_EQUAL, REC_BIN32_UNSIGNED, REC_BPINT_UNSIGNED, NE_BIN32U_BIN16U},
      {ITM_NOT_EQUAL, REC_BIN32_UNSIGNED, REC_BIN16_UNSIGNED, NE_BIN32U_BIN16U},
      {ITM_NOT_EQUAL, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, NE_BIN32U_BIN32U},  //Was 32S. Error? ANS

      {ITM_NOT_EQUAL, REC_BIN64_SIGNED,   REC_BIN64_SIGNED,   NE_BIN64S_BIN64S},

      {ITM_NOT_EQUAL, REC_DECIMAL_UNSIGNED, REC_DECIMAL_UNSIGNED, NE_DECU_DECU},
      {ITM_NOT_EQUAL, REC_DECIMAL_LSE,      REC_DECIMAL_LSE,      NE_DECS_DECS},

      {ITM_NOT_EQUAL, REC_FLOAT32,        REC_FLOAT32,        NE_FLOAT32_FLOAT32},
      {ITM_NOT_EQUAL, REC_FLOAT64,        REC_FLOAT64,        NE_FLOAT64_FLOAT64},

      {ITM_NOT_EQUAL, REC_DATETIME,       REC_DATETIME,       NE_DATETIME_DATETIME},

      {ITM_NOT_EQUAL, REC_BYTE_F_ASCII,     REC_BYTE_F_ASCII,     NE_ASCII_F_F},

      {ITM_NOT_EQUAL, REC_BYTE_F_ASCII,      REC_BYTE_V_ASCII,      NE_ASCII_COMP},
      {ITM_NOT_EQUAL, REC_BYTE_V_ASCII,      REC_BYTE_F_ASCII,      NE_ASCII_COMP},
      {ITM_NOT_EQUAL, REC_BYTE_V_ASCII,      REC_BYTE_V_ASCII,      NE_ASCII_COMP},
      {ITM_NOT_EQUAL, REC_BYTE_F_ASCII,      REC_BYTE_V_ASCII_LONG, NE_ASCII_COMP},
      {ITM_NOT_EQUAL, REC_BYTE_V_ASCII_LONG, REC_BYTE_F_ASCII,      NE_ASCII_COMP},
      {ITM_NOT_EQUAL, REC_BYTE_V_ASCII,      REC_BYTE_V_ASCII_LONG, NE_ASCII_COMP},
      {ITM_NOT_EQUAL, REC_BYTE_V_ASCII_LONG, REC_BYTE_V_ASCII,      NE_ASCII_COMP},
      {ITM_NOT_EQUAL, REC_BYTE_V_ASCII_LONG, REC_BYTE_V_ASCII_LONG, NE_ASCII_COMP},

      {ITM_NOT_EQUAL,REC_NCHAR_F_UNICODE,REC_NCHAR_F_UNICODE,NE_UNICODE_F_F},
      {ITM_NOT_EQUAL, REC_NCHAR_F_UNICODE,      REC_NCHAR_V_UNICODE,      UNICODE_COMP},
      {ITM_NOT_EQUAL, REC_NCHAR_V_UNICODE,      REC_NCHAR_F_UNICODE,      UNICODE_COMP},
      {ITM_NOT_EQUAL, REC_NCHAR_V_UNICODE,      REC_NCHAR_V_UNICODE,      UNICODE_COMP},

      {ITM_LESS, REC_BIN16_SIGNED,   REC_BIN16_SIGNED,   LT_BIN16S_BIN16S},
      {ITM_LESS, REC_BIN16_SIGNED,   REC_BIN32_SIGNED,   LT_BIN16S_BIN32S},
      {ITM_LESS, REC_BIN16_SIGNED,   REC_BPINT_UNSIGNED, LT_BIN16S_BIN16U},
      {ITM_LESS, REC_BIN16_SIGNED,   REC_BIN16_UNSIGNED, LT_BIN16S_BIN16U},
      {ITM_LESS, REC_BIN16_SIGNED,   REC_BIN32_UNSIGNED, LT_BIN16S_BIN32U},   // Was 32S. Error? ANS

      {ITM_LESS, REC_BPINT_UNSIGNED, REC_BIN16_SIGNED,   LT_BIN16U_BIN16S},
      {ITM_LESS, REC_BPINT_UNSIGNED, REC_BIN32_SIGNED,   LT_BIN16U_BIN32S},
      {ITM_LESS, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, LT_BIN16U_BIN16U},
      {ITM_LESS, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, LT_BIN16U_BIN16U},
      {ITM_LESS, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, LT_BIN16U_BIN32U},   // Was 32S. Error? ANS

      {ITM_LESS, REC_BIN16_UNSIGNED, REC_BIN16_SIGNED,   LT_BIN16U_BIN16S},
      {ITM_LESS, REC_BIN16_UNSIGNED, REC_BIN32_SIGNED,   LT_BIN16U_BIN32S},
      {ITM_LESS, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, LT_BIN16U_BIN16U},
      {ITM_LESS, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, LT_BIN16U_BIN16U},
      {ITM_LESS, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, LT_BIN16U_BIN32U},   // Was 32S. Error? ANS

      {ITM_LESS, REC_BIN32_SIGNED,   REC_BIN16_SIGNED,   LT_BIN32S_BIN16S},
      {ITM_LESS, REC_BIN32_SIGNED,   REC_BIN32_SIGNED,   LT_BIN32S_BIN32S},
      {ITM_LESS, REC_BIN32_SIGNED,   REC_BPINT_UNSIGNED, LT_BIN32S_BIN16U},
      {ITM_LESS, REC_BIN32_SIGNED,   REC_BIN16_UNSIGNED, LT_BIN32S_BIN16U},
      {ITM_LESS, REC_BIN32_SIGNED,   REC_BIN32_UNSIGNED, LT_BIN32S_BIN32U},   // Was 32S. Error? ANS

      {ITM_LESS, REC_BIN32_UNSIGNED, REC_BIN16_SIGNED,   LT_BIN32U_BIN16S},
      {ITM_LESS, REC_BIN32_UNSIGNED, REC_BIN32_SIGNED,   LT_BIN32U_BIN32S},
      {ITM_LESS, REC_BIN32_UNSIGNED, REC_BPINT_UNSIGNED, LT_BIN32U_BIN16U},
      {ITM_LESS, REC_BIN32_UNSIGNED, REC_BIN16_UNSIGNED, LT_BIN32U_BIN16U},
      {ITM_LESS, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, LT_BIN32U_BIN32U},   // Was 32S. Error? ANS

      {ITM_LESS, REC_BIN64_SIGNED,   REC_BIN64_SIGNED,   LT_BIN64S_BIN64S},

      {ITM_LESS, REC_DECIMAL_UNSIGNED, REC_DECIMAL_UNSIGNED, LT_DECU_DECU},
      {ITM_LESS, REC_DECIMAL_LSE,      REC_DECIMAL_LSE,      LT_DECS_DECS},

      {ITM_LESS, REC_FLOAT32,        REC_FLOAT32,        LT_FLOAT32_FLOAT32},
      {ITM_LESS, REC_FLOAT64,        REC_FLOAT64,        LT_FLOAT64_FLOAT64},

      {ITM_LESS, REC_DATETIME,       REC_DATETIME,       LT_DATETIME_DATETIME},

      {ITM_LESS, REC_BYTE_F_ASCII,     REC_BYTE_F_ASCII,     LT_ASCII_F_F},

      {ITM_LESS, REC_BYTE_F_ASCII,     REC_BYTE_V_ASCII,     LT_ASCII_COMP},
      {ITM_LESS, REC_BYTE_V_ASCII,     REC_BYTE_F_ASCII,     LT_ASCII_COMP},
      {ITM_LESS, REC_BYTE_V_ASCII,     REC_BYTE_V_ASCII,     LT_ASCII_COMP},
      {ITM_LESS, REC_BYTE_F_ASCII,     REC_BYTE_V_ASCII_LONG,     LT_ASCII_COMP},
      {ITM_LESS, REC_BYTE_V_ASCII_LONG,     REC_BYTE_F_ASCII,     LT_ASCII_COMP},
      {ITM_LESS, REC_BYTE_V_ASCII,     REC_BYTE_V_ASCII_LONG,     LT_ASCII_COMP},
      {ITM_LESS, REC_BYTE_V_ASCII_LONG,     REC_BYTE_V_ASCII,     LT_ASCII_COMP},
      {ITM_LESS, REC_BYTE_V_ASCII_LONG,     REC_BYTE_V_ASCII_LONG,     LT_ASCII_COMP},
      {ITM_LESS,REC_NCHAR_F_UNICODE,REC_NCHAR_F_UNICODE,LT_UNICODE_F_F},
      {ITM_LESS, REC_NCHAR_F_UNICODE,     REC_NCHAR_V_UNICODE,     UNICODE_COMP},
      {ITM_LESS, REC_NCHAR_V_UNICODE,     REC_NCHAR_F_UNICODE,     UNICODE_COMP},
      {ITM_LESS, REC_NCHAR_V_UNICODE,     REC_NCHAR_V_UNICODE,     UNICODE_COMP},


      {ITM_LESS_EQ, REC_BIN16_SIGNED,   REC_BIN16_SIGNED,   LE_BIN16S_BIN16S},
      {ITM_LESS_EQ, REC_BIN16_SIGNED,   REC_BIN32_SIGNED,   LE_BIN16S_BIN32S},
      {ITM_LESS_EQ, REC_BIN16_SIGNED,   REC_BPINT_UNSIGNED, LE_BIN16S_BIN16U},
      {ITM_LESS_EQ, REC_BIN16_SIGNED,   REC_BIN16_UNSIGNED, LE_BIN16S_BIN16U},
      {ITM_LESS_EQ, REC_BIN16_SIGNED,   REC_BIN32_UNSIGNED, LE_BIN16S_BIN32U},    // Was 32S. Error? ANS

      {ITM_LESS_EQ, REC_BPINT_UNSIGNED, REC_BIN16_SIGNED,   LE_BIN16U_BIN16S},
      {ITM_LESS_EQ, REC_BPINT_UNSIGNED, REC_BIN32_SIGNED,   LE_BIN16U_BIN32S},
      {ITM_LESS_EQ, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, LE_BIN16U_BIN16U},
      {ITM_LESS_EQ, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, LE_BIN16U_BIN16U},
      {ITM_LESS_EQ, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, LE_BIN16U_BIN32U},    // Was 32S. Error? ANS

      {ITM_LESS_EQ, REC_BIN16_UNSIGNED, REC_BIN16_SIGNED,   LE_BIN16U_BIN16S},
      {ITM_LESS_EQ, REC_BIN16_UNSIGNED, REC_BIN32_SIGNED,   LE_BIN16U_BIN32S},
      {ITM_LESS_EQ, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, LE_BIN16U_BIN16U},
      {ITM_LESS_EQ, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, LE_BIN16U_BIN16U},
      {ITM_LESS_EQ, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, LE_BIN16U_BIN32U},    // Was 32S. Error? ANS

      {ITM_LESS_EQ, REC_BIN32_SIGNED,   REC_BIN16_SIGNED,   LE_BIN32S_BIN16S},
      {ITM_LESS_EQ, REC_BIN32_SIGNED,   REC_BIN32_SIGNED,   LE_BIN32S_BIN32S},
      {ITM_LESS_EQ, REC_BIN32_SIGNED,   REC_BPINT_UNSIGNED, LE_BIN32S_BIN16U},
      {ITM_LESS_EQ, REC_BIN32_SIGNED,   REC_BIN16_UNSIGNED, LE_BIN32S_BIN16U},
      {ITM_LESS_EQ, REC_BIN32_SIGNED,   REC_BIN32_UNSIGNED, LE_BIN32S_BIN32U},    // Was 32S. Error? ANS

      {ITM_LESS_EQ, REC_BIN32_UNSIGNED, REC_BIN16_SIGNED,   LE_BIN32U_BIN16S},
      {ITM_LESS_EQ, REC_BIN32_UNSIGNED, REC_BIN32_SIGNED,   LE_BIN32U_BIN32S},
      {ITM_LESS_EQ, REC_BIN32_UNSIGNED, REC_BPINT_UNSIGNED, LE_BIN32U_BIN16U},
      {ITM_LESS_EQ, REC_BIN32_UNSIGNED, REC_BIN16_UNSIGNED, LE_BIN32U_BIN16U},
      {ITM_LESS_EQ, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, LE_BIN32U_BIN32U},    // Was 32S. Error? ANS

      {ITM_LESS_EQ, REC_BIN64_SIGNED,   REC_BIN64_SIGNED,   LE_BIN64S_BIN64S},

      {ITM_LESS_EQ, REC_DECIMAL_UNSIGNED, REC_DECIMAL_UNSIGNED, LE_DECU_DECU},
      {ITM_LESS_EQ, REC_DECIMAL_LSE,      REC_DECIMAL_LSE,      LE_DECS_DECS},

      {ITM_LESS_EQ, REC_FLOAT32,        REC_FLOAT32,        LE_FLOAT32_FLOAT32},
      {ITM_LESS_EQ, REC_FLOAT64,        REC_FLOAT64,        LE_FLOAT64_FLOAT64},

      {ITM_LESS_EQ, REC_DATETIME,       REC_DATETIME,       LE_DATETIME_DATETIME},

      {ITM_LESS_EQ, REC_BYTE_F_ASCII,     REC_BYTE_F_ASCII,     LE_ASCII_F_F},

      {ITM_LESS_EQ, REC_BYTE_F_ASCII,     REC_BYTE_V_ASCII,     LE_ASCII_COMP},
      {ITM_LESS_EQ, REC_BYTE_V_ASCII,     REC_BYTE_F_ASCII,     LE_ASCII_COMP},
      {ITM_LESS_EQ, REC_BYTE_V_ASCII,     REC_BYTE_V_ASCII,     LE_ASCII_COMP},
      {ITM_LESS_EQ, REC_BYTE_F_ASCII,     REC_BYTE_V_ASCII_LONG,     LE_ASCII_COMP},
      {ITM_LESS_EQ, REC_BYTE_V_ASCII_LONG,     REC_BYTE_F_ASCII,     LE_ASCII_COMP},
      {ITM_LESS_EQ, REC_BYTE_V_ASCII,     REC_BYTE_V_ASCII_LONG,     LE_ASCII_COMP},
      {ITM_LESS_EQ, REC_BYTE_V_ASCII_LONG,     REC_BYTE_V_ASCII,     LE_ASCII_COMP},
      {ITM_LESS_EQ, REC_BYTE_V_ASCII_LONG,     REC_BYTE_V_ASCII_LONG,     LE_ASCII_COMP},

      {ITM_LESS_EQ,REC_NCHAR_F_UNICODE,REC_NCHAR_F_UNICODE,LE_UNICODE_F_F},
      {ITM_LESS_EQ, REC_NCHAR_F_UNICODE,     REC_NCHAR_V_UNICODE,     UNICODE_COMP},
      {ITM_LESS_EQ, REC_NCHAR_V_UNICODE,     REC_NCHAR_F_UNICODE,     UNICODE_COMP},
      {ITM_LESS_EQ, REC_NCHAR_V_UNICODE,     REC_NCHAR_V_UNICODE,     UNICODE_COMP},

      {ITM_GREATER, REC_BIN16_SIGNED,   REC_BIN16_SIGNED,   GT_BIN16S_BIN16S},
      {ITM_GREATER, REC_BIN16_SIGNED,   REC_BIN32_SIGNED,   GT_BIN16S_BIN32S},
      {ITM_GREATER, REC_BIN16_SIGNED,   REC_BPINT_UNSIGNED, GT_BIN16S_BIN16U},
      {ITM_GREATER, REC_BIN16_SIGNED,   REC_BIN16_UNSIGNED, GT_BIN16S_BIN16U},
      {ITM_GREATER, REC_BIN16_SIGNED,   REC_BIN32_UNSIGNED, GT_BIN16S_BIN32U},    // Was 32S. Error? ANS

      {ITM_GREATER, REC_BPINT_UNSIGNED, REC_BIN16_SIGNED,   GT_BIN16U_BIN16S},
      {ITM_GREATER, REC_BPINT_UNSIGNED, REC_BIN32_SIGNED,   GT_BIN16U_BIN32S},
      {ITM_GREATER, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, GT_BIN16U_BIN16U},
      {ITM_GREATER, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, GT_BIN16U_BIN16U},
      {ITM_GREATER, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, GT_BIN16U_BIN32U},    // Was 32S. Error? ANS

      {ITM_GREATER, REC_BIN16_UNSIGNED, REC_BIN16_SIGNED,   GT_BIN16U_BIN16S},
      {ITM_GREATER, REC_BIN16_UNSIGNED, REC_BIN32_SIGNED,   GT_BIN16U_BIN32S},
      {ITM_GREATER, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, GT_BIN16U_BIN16U},
      {ITM_GREATER, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, GT_BIN16U_BIN16U},
      {ITM_GREATER, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, GT_BIN16U_BIN32U},    // Was 32S. Error? ANS

      {ITM_GREATER, REC_BIN32_SIGNED,   REC_BIN16_SIGNED,   GT_BIN32S_BIN16S},
      {ITM_GREATER, REC_BIN32_SIGNED,   REC_BIN32_SIGNED,   GT_BIN32S_BIN32S},
      {ITM_GREATER, REC_BIN32_SIGNED,   REC_BPINT_UNSIGNED, GT_BIN32S_BIN16U},
      {ITM_GREATER, REC_BIN32_SIGNED,   REC_BIN16_UNSIGNED, GT_BIN32S_BIN16U},
      {ITM_GREATER, REC_BIN32_SIGNED,   REC_BIN32_UNSIGNED, GT_BIN32S_BIN32U},    // Was 32S. Error? ANS

      {ITM_GREATER, REC_BIN32_UNSIGNED, REC_BIN16_SIGNED,   GT_BIN32U_BIN16S},
      {ITM_GREATER, REC_BIN32_UNSIGNED, REC_BIN32_SIGNED,   GT_BIN32U_BIN32S},
      {ITM_GREATER, REC_BIN32_UNSIGNED, REC_BPINT_UNSIGNED, GT_BIN32U_BIN16U},
      {ITM_GREATER, REC_BIN32_UNSIGNED, REC_BIN16_UNSIGNED, GT_BIN32U_BIN16U},
      {ITM_GREATER, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, GT_BIN32U_BIN32U},    // Was 32S. Error? ANS

      {ITM_GREATER, REC_BIN64_SIGNED,   REC_BIN64_SIGNED,   GT_BIN64S_BIN64S},

      {ITM_GREATER, REC_DECIMAL_UNSIGNED, REC_DECIMAL_UNSIGNED, GT_DECU_DECU},
      {ITM_GREATER, REC_DECIMAL_LSE,      REC_DECIMAL_LSE,      GT_DECS_DECS},

      {ITM_GREATER, REC_FLOAT32,        REC_FLOAT32,        GT_FLOAT32_FLOAT32},
      {ITM_GREATER, REC_FLOAT64,        REC_FLOAT64,        GT_FLOAT64_FLOAT64},

      {ITM_GREATER, REC_DATETIME,       REC_DATETIME,       GT_DATETIME_DATETIME},

      {ITM_GREATER, REC_BYTE_F_ASCII,     REC_BYTE_F_ASCII,     GT_ASCII_F_F},

      {ITM_GREATER, REC_BYTE_F_ASCII,     REC_BYTE_V_ASCII,     GT_ASCII_COMP},
      {ITM_GREATER, REC_BYTE_V_ASCII,     REC_BYTE_F_ASCII,     GT_ASCII_COMP},
      {ITM_GREATER, REC_BYTE_V_ASCII,     REC_BYTE_V_ASCII,     GT_ASCII_COMP},
      {ITM_GREATER, REC_BYTE_F_ASCII,     REC_BYTE_V_ASCII_LONG,     GT_ASCII_COMP},
      {ITM_GREATER, REC_BYTE_V_ASCII_LONG,     REC_BYTE_F_ASCII,     GT_ASCII_COMP},
      {ITM_GREATER, REC_BYTE_V_ASCII,     REC_BYTE_V_ASCII_LONG,     GT_ASCII_COMP},
      {ITM_GREATER, REC_BYTE_V_ASCII_LONG,     REC_BYTE_V_ASCII,     GT_ASCII_COMP},
      {ITM_GREATER, REC_BYTE_V_ASCII_LONG,     REC_BYTE_V_ASCII_LONG,     GT_ASCII_COMP},

      {ITM_GREATER,REC_NCHAR_F_UNICODE,REC_NCHAR_F_UNICODE,GT_UNICODE_F_F},
      {ITM_GREATER, REC_NCHAR_F_UNICODE,     REC_NCHAR_V_UNICODE,     UNICODE_COMP},
      {ITM_GREATER, REC_NCHAR_V_UNICODE,     REC_NCHAR_F_UNICODE,     UNICODE_COMP},
      {ITM_GREATER, REC_NCHAR_V_UNICODE,     REC_NCHAR_V_UNICODE,     UNICODE_COMP},

      {ITM_GREATER_EQ, REC_BIN16_SIGNED,   REC_BIN16_SIGNED,   GE_BIN16S_BIN16S},
      {ITM_GREATER_EQ, REC_BIN16_SIGNED,   REC_BIN32_SIGNED,   GE_BIN16S_BIN32S},
      {ITM_GREATER_EQ, REC_BIN16_SIGNED,   REC_BPINT_UNSIGNED, GE_BIN16S_BIN16U},
      {ITM_GREATER_EQ, REC_BIN16_SIGNED,   REC_BIN16_UNSIGNED, GE_BIN16S_BIN16U},
      {ITM_GREATER_EQ, REC_BIN16_SIGNED,   REC_BIN32_UNSIGNED, GE_BIN16S_BIN32U},   // Was 32S. Error? ANS

      {ITM_GREATER_EQ, REC_BPINT_UNSIGNED, REC_BIN16_SIGNED,   GE_BIN16U_BIN16S},
      {ITM_GREATER_EQ, REC_BPINT_UNSIGNED, REC_BIN32_SIGNED,   GE_BIN16U_BIN32S},
      {ITM_GREATER_EQ, REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED, GE_BIN16U_BIN16U},
      {ITM_GREATER_EQ, REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED, GE_BIN16U_BIN16U},
      {ITM_GREATER_EQ, REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED, GE_BIN16U_BIN32U},   // Was 32S. Error? ANS

      {ITM_GREATER_EQ, REC_BIN16_UNSIGNED, REC_BIN16_SIGNED,   GE_BIN16U_BIN16S},
      {ITM_GREATER_EQ, REC_BIN16_UNSIGNED, REC_BIN32_SIGNED,   GE_BIN16U_BIN32S},
      {ITM_GREATER_EQ, REC_BIN16_UNSIGNED, REC_BPINT_UNSIGNED, GE_BIN16U_BIN16U},
      {ITM_GREATER_EQ, REC_BIN16_UNSIGNED, REC_BIN16_UNSIGNED, GE_BIN16U_BIN16U},
      {ITM_GREATER_EQ, REC_BIN16_UNSIGNED, REC_BIN32_UNSIGNED, GE_BIN16U_BIN32U},   // Was 32S. Error? ANS

      {ITM_GREATER_EQ, REC_BIN32_SIGNED,   REC_BIN16_SIGNED,   GE_BIN32S_BIN16S},
      {ITM_GREATER_EQ, REC_BIN32_SIGNED,   REC_BIN32_SIGNED,   GE_BIN32S_BIN32S},
      {ITM_GREATER_EQ, REC_BIN32_SIGNED,   REC_BPINT_UNSIGNED, GE_BIN32S_BIN16U},
      {ITM_GREATER_EQ, REC_BIN32_SIGNED,   REC_BIN16_UNSIGNED, GE_BIN32S_BIN16U},
      {ITM_GREATER_EQ, REC_BIN32_SIGNED,   REC_BIN32_UNSIGNED, GE_BIN32S_BIN32U},   // Was 32S. Error? ANS

      {ITM_GREATER_EQ, REC_BIN32_UNSIGNED, REC_BIN16_SIGNED,   GE_BIN32U_BIN16S},
      {ITM_GREATER_EQ, REC_BIN32_UNSIGNED, REC_BIN32_SIGNED,   GE_BIN32U_BIN32S},
      {ITM_GREATER_EQ, REC_BIN32_UNSIGNED, REC_BPINT_UNSIGNED, GE_BIN32U_BIN16U},
      {ITM_GREATER_EQ, REC_BIN32_UNSIGNED, REC_BIN16_UNSIGNED, GE_BIN32U_BIN16U},
      {ITM_GREATER_EQ, REC_BIN32_UNSIGNED, REC_BIN32_UNSIGNED, GE_BIN32U_BIN32U},   // Was 32S. Error? ANS

      {ITM_GREATER_EQ, REC_BIN64_SIGNED,   REC_BIN64_SIGNED,   GE_BIN64S_BIN64S},

      {ITM_GREATER_EQ, REC_DECIMAL_UNSIGNED, REC_DECIMAL_UNSIGNED, GE_DECU_DECU},
      {ITM_GREATER_EQ, REC_DECIMAL_LSE,      REC_DECIMAL_LSE,      GE_DECS_DECS},

      {ITM_GREATER_EQ, REC_FLOAT32,        REC_FLOAT32,        GE_FLOAT32_FLOAT32},
      {ITM_GREATER_EQ, REC_FLOAT64,        REC_FLOAT64,        GE_FLOAT64_FLOAT64},

      {ITM_GREATER_EQ, REC_DATETIME,       REC_DATETIME,       GE_DATETIME_DATETIME},

      {ITM_GREATER_EQ, REC_BYTE_F_ASCII,     REC_BYTE_F_ASCII,     GE_ASCII_F_F},

      {ITM_GREATER_EQ, REC_BYTE_F_ASCII,     REC_BYTE_V_ASCII,     GE_ASCII_COMP},
      {ITM_GREATER_EQ, REC_BYTE_V_ASCII,     REC_BYTE_F_ASCII,     GE_ASCII_COMP},
      {ITM_GREATER_EQ, REC_BYTE_V_ASCII,     REC_BYTE_V_ASCII,     GE_ASCII_COMP},
      {ITM_GREATER_EQ, REC_BYTE_F_ASCII,     REC_BYTE_V_ASCII_LONG,     GE_ASCII_COMP},
      {ITM_GREATER_EQ, REC_BYTE_V_ASCII_LONG,     REC_BYTE_F_ASCII,     GE_ASCII_COMP},
      {ITM_GREATER_EQ, REC_BYTE_V_ASCII,     REC_BYTE_V_ASCII_LONG,     GE_ASCII_COMP},
      {ITM_GREATER_EQ, REC_BYTE_V_ASCII_LONG,     REC_BYTE_V_ASCII,     GE_ASCII_COMP},
      {ITM_GREATER_EQ, REC_BYTE_V_ASCII_LONG,     REC_BYTE_V_ASCII_LONG,     GE_ASCII_COMP},

      {ITM_GREATER_EQ,REC_NCHAR_F_UNICODE,REC_NCHAR_F_UNICODE,GE_UNICODE_F_F},
      {ITM_GREATER_EQ, REC_NCHAR_F_UNICODE,     REC_NCHAR_V_UNICODE,     UNICODE_COMP},
      {ITM_GREATER_EQ, REC_NCHAR_V_UNICODE,     REC_NCHAR_F_UNICODE,     UNICODE_COMP},
      {ITM_GREATER_EQ, REC_NCHAR_V_UNICODE,     REC_NCHAR_V_UNICODE,     UNICODE_COMP},
    };
  
  Int32 max_array_size = sizeof(cs) / sizeof(ex_comp_struct);

  Int32 i = 0;
//  while ((i < max_array_size) && (cs[i].op != op)) i++;
//  while ((i < max_array_size) && (cs[i].type_op1 != datatype1)) i++;
//  while ((i < max_array_size) && (cs[i].type_op2 != datatype2)) i++;
  
  Int32 found = 0;
  while ((i < max_array_size) && (!found))
    if ((cs[i].op == op) && 
	(cs[i].type_op1 == datatype1) &&
	(cs[i].type_op2 == datatype2))
      found = -1;
    else
      i++;
  
//  if (i < max_array_size)
  if (found)
    return &cs[i];  
  else
    return 0;
}

const ex_comp_clause::comp_case_index ex_comp_clause::computeCaseIndex(OperatorTypeEnum op, 
								       Attributes * attr1,
								       Attributes * attr2)
{
  comp_case_index c_index = COMP_NOT_SUPPORTED;
  
  short type_op1;
  short type_op2;
  getCaseDatatypes(attr1->getDatatype(), attr1->getLength(), type_op1,
                   attr2->getDatatype(), attr2->getLength(), type_op2,
                   0 /* don't need to take into account scale difference here */);

  const ex_comp_struct * cs = getMatchingRow(op, 
					     type_op1,
					     type_op2);
  if (cs)
    {
      c_index = cs->index;
      
      // Further checks are needed. Do them. 
      if ((attr1->getDatatype() == REC_BYTE_F_ASCII) &&
	  (attr2->getDatatype() == REC_BYTE_F_ASCII))
	{
	  if (attr1->getLength() != attr2->getLength())
	    c_index = ASCII_COMP;
	}

      if ((attr1->getDatatype() == REC_NCHAR_F_UNICODE) &&
	  (attr2->getDatatype() == REC_NCHAR_F_UNICODE))
	{
	  if (attr1->getLength() != attr2->getLength())
	    c_index = UNICODE_COMP;
	}
      
      // for all numeric, datetime and interval datatypes, comparison done
      // only if scale is the same
      if (((attr1->getDatatype() >= REC_MIN_BINARY) &&
	   (attr1->getDatatype() <= REC_MAX_BINARY)) ||
	  ((attr1->getDatatype() >= REC_DECIMAL_UNSIGNED) &&
	   (attr1->getDatatype() <= REC_DECIMAL_LSE)) ||
	  (attr1->getDatatype() == REC_DATETIME) ||
	  ((attr1->getDatatype() >= REC_MIN_INTERVAL) &&
	   (attr1->getDatatype() <= REC_MAX_INTERVAL)))
	{
	  // numeric, datetime or interval types
	  if (attr1->getScale() != attr2->getScale())
	    c_index = COMP_NOT_SUPPORTED; 
	  
	  // for decimal datatypes, comparison done only if lengths are same.
	  if ((attr1->getDatatype() >= REC_DECIMAL_UNSIGNED) &&
	      (attr1->getDatatype() <= REC_DECIMAL_LSE))
	    {
	      if (attr1->getLength() != attr2->getLength())
		c_index = COMP_NOT_SUPPORTED;
	    }
	} // numeric, datetime or interval types
    } // comparison supported 

  return c_index;
}


void ex_comp_clause::set_case_index()
{ 

  case_index = COMP_NOT_SUPPORTED;

  if ((getOperand(1)->isComplexType()) || (getOperand(2)->isComplexType()))
    {
      // complex comparison supported only if both operands are complex with
      // same datatype
      if ((getOperand(1)->isComplexType()) && (getOperand(2)->isComplexType()) &&
	  (getOperand(1)->getDatatype() == getOperand(2)->getDatatype()))
        case_index = COMP_COMPLEX;
    }
  else
    {
      // Simple types are handled here.
      case_index = computeCaseIndex(getOperType(), getOperand(1), getOperand(2));
    } // simple type
  
}
  
void ex_comp_clause::set_case_index(OperatorTypeEnum op,
				    Attributes * attr1,
				    Attributes * attr2)
{
  // Only simple types are handled here.
  case_index = computeCaseIndex(op, attr1, attr2);
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
  set_case_index();
  return ex_clause::fixup(space, exHeap, constants_area, temps_area,
			  persistentArea,
			  fixupConstsAndTemps, spaceCompOnly);
}

conv_case_index ex_conv_clause::find_case_index(short sourceType, Lng32 sourceLen,
                                                short targetType, Lng32 targetLen,
                                                Lng32 scaleDifference)
{
 
  typedef struct {
    short type_op1; // left operand
    short type_op2; // right operand
    conv_case_index index;
  } conv_struct;
  
  static const conv_struct conv_s[] = {
    //-----------------------------------------------------------------------
    // src datatype          tgt datatype            case statement index	
    //-----------------------------------------------------------------------
    {REC_BPINT_UNSIGNED, REC_BPINT_UNSIGNED,         CONV_BPINTU_BPINTU},
    {REC_BPINT_UNSIGNED, REC_BIN16_SIGNED,	     CONV_BIN16U_BIN16S},
    {REC_BPINT_UNSIGNED, REC_BIN16_UNSIGNED,         CONV_BIN16U_BIN16U},
    {REC_BPINT_UNSIGNED, REC_BIN32_SIGNED,           CONV_BIN16U_BIN32S},
    {REC_BPINT_UNSIGNED, REC_BIN32_UNSIGNED,         CONV_BIN16U_BIN32U},
    {REC_BPINT_UNSIGNED, REC_BIN64_SIGNED,           CONV_BIN16U_BIN64S},
    {REC_BPINT_UNSIGNED, REC_DECIMAL_LSE,            CONV_BIN16U_DECS},
    {REC_BPINT_UNSIGNED, REC_DECIMAL_UNSIGNED,       CONV_BIN16U_DECS},
    {REC_BPINT_UNSIGNED, REC_FLOAT32,                CONV_BIN16U_FLOAT32},
    {REC_BPINT_UNSIGNED, REC_FLOAT64,                CONV_BIN16U_FLOAT64},
    {REC_BPINT_UNSIGNED, REC_NUM_BIG_UNSIGNED,       CONV_BIN16U_BIGNUM},
    {REC_BPINT_UNSIGNED, REC_NUM_BIG_SIGNED,         CONV_BIN16U_BIGNUM},
    {REC_BPINT_UNSIGNED, REC_BYTE_F_ASCII,           CONV_BIN16U_ASCII},
    {REC_BPINT_UNSIGNED, REC_BYTE_V_ASCII,           CONV_BIN16U_ASCII},
    {REC_BPINT_UNSIGNED, REC_BYTE_V_ASCII_LONG,      CONV_BIN16U_ASCII},
    {REC_BPINT_UNSIGNED, REC_NCHAR_F_UNICODE,        CONV_BIN16U_UNICODE},
    {REC_BPINT_UNSIGNED, REC_NCHAR_V_UNICODE,        CONV_BIN16U_UNICODE},

    {REC_BIN16_SIGNED, 	REC_BPINT_UNSIGNED,         CONV_BIN16S_BPINTU},
    {REC_BIN16_SIGNED,  REC_BIN16_SIGNED,           CONV_BIN16S_BIN16S},
    {REC_BIN16_SIGNED,  REC_BIN16_UNSIGNED,         CONV_BIN16S_BIN16U},
    {REC_BIN16_SIGNED,  REC_BIN32_SIGNED,           CONV_BIN16S_BIN32S},
    {REC_BIN16_SIGNED,  REC_BIN32_UNSIGNED,         CONV_BIN16S_BIN32U},
    {REC_BIN16_SIGNED,  REC_BIN64_SIGNED,           CONV_BIN16S_BIN64S},
    {REC_BIN16_SIGNED,  REC_DECIMAL_LSE,            CONV_BIN16S_DECS},
    {REC_BIN16_SIGNED,  REC_DECIMAL_UNSIGNED,       CONV_BIN16S_DECU},
    {REC_BIN16_SIGNED,  REC_FLOAT32,                CONV_BIN16S_FLOAT32},
    {REC_BIN16_SIGNED,  REC_FLOAT64,                CONV_BIN16S_FLOAT64},
    {REC_BIN16_SIGNED,  REC_NUM_BIG_UNSIGNED,       CONV_BIN16S_BIGNUMU},
    {REC_BIN16_SIGNED,  REC_NUM_BIG_SIGNED,         CONV_BIN16S_BIGNUM},
    {REC_BIN16_SIGNED,  REC_BYTE_F_ASCII,           CONV_BIN16S_ASCII},
    {REC_BIN16_SIGNED,  REC_BYTE_V_ASCII,           CONV_BIN16S_ASCII},
    {REC_BIN16_SIGNED,  REC_BYTE_V_ASCII_LONG,      CONV_BIN16S_ASCII},
    {REC_BIN16_SIGNED,  REC_NCHAR_F_UNICODE,        CONV_BIN16S_UNICODE},
    {REC_BIN16_SIGNED,  REC_NCHAR_V_UNICODE,        CONV_BIN16S_UNICODE},
    
    {REC_BIN16_UNSIGNED,  REC_BPINT_UNSIGNED,         CONV_BIN16U_BPINTU},
    {REC_BIN16_UNSIGNED,  REC_BIN16_SIGNED,           CONV_BIN16U_BIN16S},
    {REC_BIN16_UNSIGNED,  REC_BIN16_UNSIGNED,         CONV_BIN16U_BIN16U},
    {REC_BIN16_UNSIGNED,  REC_BIN32_SIGNED,           CONV_BIN16U_BIN32S},
    {REC_BIN16_UNSIGNED,  REC_BIN32_UNSIGNED,         CONV_BIN16U_BIN32U},
    {REC_BIN16_UNSIGNED,  REC_BIN64_SIGNED,           CONV_BIN16U_BIN64S},
    {REC_BIN16_UNSIGNED,  REC_DECIMAL_LSE,            CONV_BIN16U_DECS},
    {REC_BIN16_UNSIGNED,  REC_DECIMAL_UNSIGNED,       CONV_BIN16U_DECS},
    {REC_BIN16_UNSIGNED,  REC_FLOAT32,                CONV_BIN16U_FLOAT32},
    {REC_BIN16_UNSIGNED,  REC_FLOAT64,                CONV_BIN16U_FLOAT64},
    {REC_BIN16_UNSIGNED,  REC_NUM_BIG_UNSIGNED,       CONV_BIN16U_BIGNUM},
    {REC_BIN16_UNSIGNED,  REC_NUM_BIG_SIGNED,         CONV_BIN16U_BIGNUM},
    {REC_BIN16_UNSIGNED,  REC_BYTE_F_ASCII,           CONV_BIN16U_ASCII},
    {REC_BIN16_UNSIGNED,  REC_BYTE_V_ASCII,           CONV_BIN16U_ASCII},
    {REC_BIN16_UNSIGNED,  REC_BYTE_V_ASCII_LONG,      CONV_BIN16U_ASCII},
    {REC_BIN16_UNSIGNED,  REC_NCHAR_F_UNICODE,        CONV_BIN16U_UNICODE},
    {REC_BIN16_UNSIGNED,  REC_NCHAR_V_UNICODE,        CONV_BIN16U_UNICODE},
   
    {REC_BIN32_SIGNED,	 REC_BPINT_UNSIGNED,         CONV_BIN32S_BPINTU}, 
    {REC_BIN32_SIGNED,   REC_BIN16_SIGNED,           CONV_BIN32S_BIN16S},
    {REC_BIN32_SIGNED,   REC_BIN16_UNSIGNED,         CONV_BIN32S_BIN16U},
    {REC_BIN32_SIGNED,   REC_BIN32_SIGNED,           CONV_BIN32S_BIN32S},
    {REC_BIN32_SIGNED,   REC_BIN32_UNSIGNED,         CONV_BIN32S_BIN32U},
    {REC_BIN32_SIGNED,   REC_BIN64_SIGNED,           CONV_BIN32S_BIN64S},
    {REC_BIN32_SIGNED,   REC_DECIMAL_LSE,            CONV_BIN32S_DECS},
    {REC_BIN32_SIGNED,   REC_DECIMAL_UNSIGNED,       CONV_BIN32S_DECU},
    {REC_BIN32_SIGNED,   REC_FLOAT32,                CONV_BIN32S_FLOAT32},
    {REC_BIN32_SIGNED,   REC_FLOAT64,                CONV_BIN32S_FLOAT64},
    {REC_BIN32_SIGNED,   REC_NUM_BIG_UNSIGNED,       CONV_BIN32S_BIGNUMU},
    {REC_BIN32_SIGNED,   REC_NUM_BIG_SIGNED,         CONV_BIN32S_BIGNUM},
    {REC_BIN32_SIGNED,   REC_BYTE_F_ASCII,           CONV_BIN32S_ASCII},
    {REC_BIN32_SIGNED,   REC_BYTE_V_ASCII,           CONV_BIN32S_ASCII},
    {REC_BIN32_SIGNED,   REC_BYTE_V_ASCII_LONG,      CONV_BIN32S_ASCII},
    {REC_BIN32_SIGNED,   REC_NCHAR_F_UNICODE,        CONV_BIN32S_UNICODE},
    {REC_BIN32_SIGNED,   REC_NCHAR_V_UNICODE,        CONV_BIN32S_UNICODE},
    
    {REC_BIN32_UNSIGNED,  REC_BPINT_UNSIGNED,         CONV_BIN32U_BPINTU}, 
    {REC_BIN32_UNSIGNED,  REC_BIN16_SIGNED,           CONV_BIN32U_BIN16S},
    {REC_BIN32_UNSIGNED,  REC_BIN16_UNSIGNED,         CONV_BIN32U_BIN16U},
    {REC_BIN32_UNSIGNED,  REC_BIN32_SIGNED,           CONV_BIN32U_BIN32S},
    {REC_BIN32_UNSIGNED,  REC_BIN32_UNSIGNED,         CONV_BIN32U_BIN32U},
    {REC_BIN32_UNSIGNED,  REC_BIN64_SIGNED,           CONV_BIN32U_BIN64S},
    {REC_BIN32_UNSIGNED,  REC_DECIMAL_LSE,            CONV_BIN32U_DECS},
    {REC_BIN32_UNSIGNED,  REC_DECIMAL_UNSIGNED,       CONV_BIN32U_DECS},
    {REC_BIN32_UNSIGNED,  REC_FLOAT32,                CONV_BIN32U_FLOAT32},
    {REC_BIN32_UNSIGNED,  REC_FLOAT64,                CONV_BIN32U_FLOAT64},
    {REC_BIN32_UNSIGNED,  REC_NUM_BIG_UNSIGNED,       CONV_BIN32U_BIGNUM},
    {REC_BIN32_UNSIGNED,  REC_NUM_BIG_SIGNED,         CONV_BIN32U_BIGNUM},
    {REC_BIN32_UNSIGNED,  REC_BYTE_F_ASCII,           CONV_BIN32U_ASCII},
    {REC_BIN32_UNSIGNED,  REC_BYTE_V_ASCII,           CONV_BIN32U_ASCII},
    {REC_BIN32_UNSIGNED,  REC_BYTE_V_ASCII_LONG,      CONV_BIN32U_ASCII},
    {REC_BIN32_UNSIGNED,  REC_NCHAR_F_UNICODE,        CONV_BIN32U_UNICODE},
    {REC_BIN32_UNSIGNED,  REC_NCHAR_V_UNICODE,        CONV_BIN32U_UNICODE},

    {REC_BIN64_SIGNED,   REC_BPINT_UNSIGNED,         CONV_BIN64S_BPINTU},
    {REC_BIN64_SIGNED,   REC_BIN16_SIGNED,           CONV_BIN64S_BIN16S},
    {REC_BIN64_SIGNED,   REC_BIN16_UNSIGNED,         CONV_BIN64S_BIN16U},
    {REC_BIN64_SIGNED,   REC_BIN32_SIGNED,           CONV_BIN64S_BIN32S},
    {REC_BIN64_SIGNED,   REC_BIN32_UNSIGNED,         CONV_BIN64S_BIN32U},
    {REC_BIN64_SIGNED,   REC_BIN64_SIGNED,           CONV_BIN64S_BIN64S},
    {REC_BIN64_SIGNED,   REC_DECIMAL_LSE,            CONV_BIN64S_DECS},
    {REC_BIN64_SIGNED,   REC_DECIMAL_UNSIGNED,       CONV_BIN64S_DECU},
    {REC_BIN64_SIGNED,   REC_FLOAT32,                CONV_BIN64S_FLOAT32},
    {REC_BIN64_SIGNED,   REC_FLOAT64,                CONV_BIN64S_FLOAT64},
    {REC_BIN64_SIGNED,   REC_NUM_BIG_UNSIGNED,       CONV_BIN64S_BIGNUMU},
    {REC_BIN64_SIGNED,   REC_NUM_BIG_SIGNED,         CONV_BIN64S_BIGNUM},
    {REC_BIN64_SIGNED,   REC_BYTE_F_ASCII,           CONV_BIN64S_ASCII},
    {REC_BIN64_SIGNED,   REC_BYTE_V_ASCII,           CONV_BIN64S_ASCII},
    {REC_BIN64_SIGNED,   REC_BYTE_V_ASCII_LONG,      CONV_BIN64S_ASCII},
    {REC_BIN64_SIGNED,   REC_NCHAR_F_UNICODE,        CONV_BIN64S_UNICODE},
    {REC_BIN64_SIGNED,   REC_NCHAR_V_UNICODE,        CONV_BIN64S_UNICODE},
    {REC_BIN64_SIGNED,   REC_DATETIME,               CONV_BIN64S_DATETIME},
    
    {REC_DECIMAL_UNSIGNED,   REC_BPINT_UNSIGNED,         CONV_DECS_BIN32U},
    {REC_DECIMAL_UNSIGNED,   REC_BIN16_UNSIGNED,         CONV_DECS_BIN32U},
    {REC_DECIMAL_UNSIGNED,   REC_BIN16_SIGNED,           CONV_DECS_BIN32S},
    {REC_DECIMAL_UNSIGNED,   REC_BIN32_UNSIGNED,         CONV_DECS_BIN32U},
    {REC_DECIMAL_UNSIGNED,   REC_BIN32_SIGNED,           CONV_DECS_BIN32S},
    {REC_DECIMAL_UNSIGNED,   REC_BIN64_SIGNED,           CONV_DECS_BIN64S},
    {REC_DECIMAL_UNSIGNED,   REC_DECIMAL_UNSIGNED,       CONV_DECS_DECU},
    {REC_DECIMAL_UNSIGNED,   REC_DECIMAL_LSE,            CONV_DECS_DECS},
    {REC_DECIMAL_UNSIGNED,   REC_FLOAT32,                CONV_DECS_FLOAT32},
    {REC_DECIMAL_UNSIGNED,   REC_FLOAT64,                CONV_DECS_FLOAT64},
    {REC_DECIMAL_UNSIGNED,   REC_NUM_BIG_UNSIGNED,       CONV_DECS_BIN32U},
    {REC_DECIMAL_UNSIGNED,   REC_NUM_BIG_SIGNED,         CONV_DECS_BIN32S},
    {REC_DECIMAL_UNSIGNED,   REC_BYTE_F_ASCII,           CONV_DECS_ASCII},
    {REC_DECIMAL_UNSIGNED,   REC_BYTE_V_ASCII,           CONV_DECS_ASCII},
    {REC_DECIMAL_UNSIGNED,   REC_BYTE_V_ASCII_LONG,      CONV_DECS_ASCII},
    {REC_DECIMAL_UNSIGNED,   REC_NCHAR_F_UNICODE,        CONV_DECS_UNICODE},
    {REC_DECIMAL_UNSIGNED,   REC_NCHAR_V_UNICODE,        CONV_DECS_UNICODE},
    
    {REC_DECIMAL_LSE,        REC_BPINT_UNSIGNED,         CONV_DECS_BIN32U},
    {REC_DECIMAL_LSE,        REC_BIN16_SIGNED,           CONV_DECS_BIN32S},
    {REC_DECIMAL_LSE,        REC_BIN16_UNSIGNED,         CONV_DECS_BIN32U},
    {REC_DECIMAL_LSE,        REC_BIN32_SIGNED,           CONV_DECS_BIN32S},
    {REC_DECIMAL_LSE,        REC_BIN32_UNSIGNED,         CONV_DECS_BIN32U},
    {REC_DECIMAL_LSE,        REC_BIN64_SIGNED,           CONV_DECS_BIN64S},
    {REC_DECIMAL_LSE,        REC_DECIMAL_LSE,            CONV_DECS_DECS},
    {REC_DECIMAL_LSE,        REC_DECIMAL_UNSIGNED,       CONV_DECS_DECU},
    {REC_DECIMAL_LSE,        REC_FLOAT32,                CONV_DECS_FLOAT32},
    {REC_DECIMAL_LSE,        REC_FLOAT64,                CONV_DECS_FLOAT64},
    {REC_DECIMAL_LSE,        REC_NUM_BIG_UNSIGNED,       CONV_DECS_BIN32U},
    {REC_DECIMAL_LSE,        REC_NUM_BIG_SIGNED,         CONV_DECS_BIN32S},
    {REC_DECIMAL_LSE,        REC_DECIMAL_LS,             CONV_DECS_ASCII},
    {REC_DECIMAL_LSE,        REC_BYTE_F_ASCII,           CONV_DECS_ASCII},
    {REC_DECIMAL_LSE,        REC_BYTE_V_ASCII,           CONV_DECS_ASCII},
    {REC_DECIMAL_LSE,        REC_BYTE_V_ASCII_LONG,      CONV_DECS_ASCII},
    {REC_DECIMAL_LSE,        REC_NCHAR_F_UNICODE,        CONV_DECS_UNICODE},
    {REC_DECIMAL_LSE,        REC_NCHAR_V_UNICODE,        CONV_DECS_UNICODE},
    
    {REC_FLOAT32,            REC_BPINT_UNSIGNED,     CONV_FLOAT32_BPINTU},
    {REC_FLOAT32,            REC_BIN16_SIGNED,       CONV_FLOAT32_BIN16S},
    {REC_FLOAT32,            REC_BIN16_UNSIGNED,     CONV_FLOAT32_BIN16U},
    {REC_FLOAT32,            REC_BIN32_SIGNED,       CONV_FLOAT32_BIN32S},
    {REC_FLOAT32,            REC_BIN32_UNSIGNED,     CONV_FLOAT32_BIN32U},
    {REC_FLOAT32,            REC_BIN64_SIGNED,       CONV_FLOAT32_BIN64S},
    {REC_FLOAT32,            REC_DECIMAL_LSE,        CONV_FLOAT32_DECS},
    {REC_FLOAT32,            REC_DECIMAL_UNSIGNED,   CONV_FLOAT32_DECU},
    {REC_FLOAT32,            REC_FLOAT32,            CONV_FLOAT32_FLOAT32},
    {REC_FLOAT32,            REC_FLOAT64,            CONV_FLOAT32_FLOAT64},
    {REC_FLOAT32,            REC_BYTE_F_ASCII,       CONV_FLOAT32_ASCII},
    {REC_FLOAT32,            REC_BYTE_V_ASCII,       CONV_FLOAT32_ASCII},
    {REC_FLOAT32,            REC_BYTE_V_ASCII_LONG,  CONV_FLOAT32_ASCII},
    {REC_FLOAT32,            REC_NCHAR_F_UNICODE,    CONV_FLOAT32_UNICODE},
    {REC_FLOAT32,            REC_NCHAR_V_UNICODE,    CONV_FLOAT32_UNICODE},
    {REC_FLOAT32,            REC_TDM_FLOAT32,        CONV_FLOAT32IEEE_FLOAT32TDM},
    {REC_FLOAT32,            REC_TDM_FLOAT64,        CONV_FLOAT32IEEE_FLOAT64TDM},
    
    {REC_FLOAT64,            REC_BPINT_UNSIGNED,     CONV_FLOAT64_BPINTU},
    {REC_FLOAT64,            REC_BIN16_SIGNED,       CONV_FLOAT64_BIN16S},
    {REC_FLOAT64,            REC_BIN16_UNSIGNED,     CONV_FLOAT64_BIN16U},
    {REC_FLOAT64,            REC_BIN32_SIGNED,       CONV_FLOAT64_BIN32S},
    {REC_FLOAT64,            REC_BIN32_UNSIGNED,     CONV_FLOAT64_BIN32U},
    {REC_FLOAT64,            REC_BIN64_SIGNED,       CONV_FLOAT64_BIN64S},
    {REC_FLOAT64,            REC_DECIMAL_LSE,        CONV_FLOAT64_DECS},
    {REC_FLOAT64,            REC_DECIMAL_UNSIGNED,   CONV_FLOAT64_DECU},
    {REC_FLOAT64,            REC_FLOAT32,            CONV_FLOAT64_FLOAT32},
    {REC_FLOAT64,            REC_FLOAT64,            CONV_FLOAT64_FLOAT64},
    {REC_FLOAT64,            REC_BYTE_F_ASCII,       CONV_FLOAT64_ASCII},
    {REC_FLOAT64,            REC_BYTE_V_ASCII,       CONV_FLOAT64_ASCII},
    {REC_FLOAT64,            REC_BYTE_V_ASCII_LONG,  CONV_FLOAT64_ASCII},
    {REC_FLOAT64,            REC_NCHAR_F_UNICODE,    CONV_FLOAT64_UNICODE},
    {REC_FLOAT64,            REC_NCHAR_V_UNICODE,    CONV_FLOAT64_UNICODE},
    {REC_FLOAT64,            REC_TDM_FLOAT32,        CONV_FLOAT64IEEE_FLOAT32TDM},
    {REC_FLOAT64,            REC_TDM_FLOAT64,        CONV_FLOAT64IEEE_FLOAT64TDM},

    {REC_TDM_FLOAT32,        REC_IEEE_FLOAT32,       CONV_FLOAT32TDM_FLOAT32IEEE},
    {REC_TDM_FLOAT32,        REC_IEEE_FLOAT64,       CONV_FLOAT32TDM_FLOAT64IEEE},
    {REC_TDM_FLOAT32,        REC_TDM_FLOAT32,        CONV_FLOAT32TDM_FLOAT32TDM},
    {REC_TDM_FLOAT32,        REC_TDM_FLOAT64,        CONV_FLOAT32TDM_FLOAT64TDM},
    {REC_TDM_FLOAT32,        REC_BYTE_F_ASCII,       CONV_FLOAT32TDM_ASCII},


    {REC_TDM_FLOAT64,        REC_TDM_FLOAT32,        CONV_FLOAT64TDM_FLOAT32TDM},
    {REC_TDM_FLOAT64,        REC_TDM_FLOAT64,        CONV_FLOAT64TDM_FLOAT64TDM},
    {REC_TDM_FLOAT64,        REC_IEEE_FLOAT32,       CONV_FLOAT64TDM_FLOAT32IEEE},
    {REC_TDM_FLOAT64,        REC_IEEE_FLOAT64,       CONV_FLOAT64TDM_FLOAT64IEEE},
    {REC_TDM_FLOAT64,        REC_BYTE_F_ASCII,       CONV_FLOAT64TDM_ASCII},


    {REC_NUM_BIG_UNSIGNED, REC_BPINT_UNSIGNED,         CONV_BIGNUM_BIN16U},
    {REC_NUM_BIG_UNSIGNED, REC_BIN16_SIGNED,           CONV_BIGNUM_BIN16S},
    {REC_NUM_BIG_UNSIGNED, REC_BIN16_UNSIGNED,         CONV_BIGNUM_BIN16U},
    {REC_NUM_BIG_UNSIGNED, REC_BIN32_SIGNED,           CONV_BIGNUM_BIN32S},
    {REC_NUM_BIG_UNSIGNED, REC_BIN32_UNSIGNED,         CONV_BIGNUM_BIN32U},
    {REC_NUM_BIG_UNSIGNED, REC_BIN64_SIGNED,           CONV_BIGNUM_BIN64S},
    {REC_NUM_BIG_UNSIGNED, REC_DECIMAL_LSE,            CONV_BIGNUM_DECS},
    {REC_NUM_BIG_UNSIGNED, REC_DECIMAL_UNSIGNED,       CONV_BIGNUM_DECU},
    {REC_NUM_BIG_UNSIGNED, REC_FLOAT32,                CONV_BIGNUM_FLOAT32},
    {REC_NUM_BIG_UNSIGNED, REC_FLOAT64,                CONV_BIGNUM_FLOAT64},
    {REC_NUM_BIG_UNSIGNED, REC_NUM_BIG_UNSIGNED,       CONV_BIGNUM_BIGNUM},
    {REC_NUM_BIG_UNSIGNED, REC_NUM_BIG_SIGNED,         CONV_BIGNUM_BIGNUM},
    {REC_NUM_BIG_UNSIGNED, REC_BYTE_F_ASCII,           CONV_BIGNUM_ASCII},
    {REC_NUM_BIG_UNSIGNED, REC_BYTE_V_ASCII,           CONV_BIGNUM_ASCII},
    {REC_NUM_BIG_UNSIGNED, REC_BYTE_V_ASCII_LONG,      CONV_BIGNUM_ASCII},
    {REC_NUM_BIG_UNSIGNED, REC_NCHAR_F_UNICODE,        CONV_BIGNUM_UNICODE},
    {REC_NUM_BIG_UNSIGNED, REC_NCHAR_V_UNICODE,        CONV_BIGNUM_UNICODE},

    {REC_NUM_BIG_SIGNED, REC_BPINT_UNSIGNED,         CONV_BIGNUM_BIN16U},
    {REC_NUM_BIG_SIGNED, REC_BIN16_SIGNED,           CONV_BIGNUM_BIN16S},
    {REC_NUM_BIG_SIGNED, REC_BIN16_UNSIGNED,         CONV_BIGNUM_BIN16U},
    {REC_NUM_BIG_SIGNED, REC_BIN32_SIGNED,           CONV_BIGNUM_BIN32S},
    {REC_NUM_BIG_SIGNED, REC_BIN32_UNSIGNED,         CONV_BIGNUM_BIN32U},
    {REC_NUM_BIG_SIGNED, REC_BIN64_SIGNED,           CONV_BIGNUM_BIN64S},
    {REC_NUM_BIG_SIGNED, REC_DECIMAL_LSE,            CONV_BIGNUM_DECS},
    {REC_NUM_BIG_SIGNED, REC_DECIMAL_UNSIGNED,       CONV_BIGNUM_DECU},
    {REC_NUM_BIG_SIGNED, REC_FLOAT32,                CONV_BIGNUM_FLOAT32},
    {REC_NUM_BIG_SIGNED, REC_FLOAT64,                CONV_BIGNUM_FLOAT64},
    {REC_NUM_BIG_SIGNED, REC_NUM_BIG_UNSIGNED,       CONV_BIGNUM_BIGNUM},
    {REC_NUM_BIG_SIGNED, REC_NUM_BIG_SIGNED,         CONV_BIGNUM_BIGNUM},
    {REC_NUM_BIG_SIGNED, REC_BYTE_F_ASCII,           CONV_BIGNUM_ASCII},
    {REC_NUM_BIG_SIGNED, REC_BYTE_V_ASCII,           CONV_BIGNUM_ASCII},
    {REC_NUM_BIG_SIGNED, REC_BYTE_V_ASCII_LONG,      CONV_BIGNUM_ASCII},
    {REC_NUM_BIG_SIGNED, REC_NCHAR_F_UNICODE,        CONV_BIGNUM_UNICODE},
    {REC_NUM_BIG_SIGNED, REC_NCHAR_V_UNICODE,        CONV_BIGNUM_UNICODE},

    // leading sign seperate decimal is identical to ascii representation.
    // Thus, use CONV_ASCII_DEC to convert it to decimal
    {REC_DECIMAL_LS,         REC_DECIMAL_LSE,        CONV_ASCII_DEC},
    {REC_DECIMAL_LS,         REC_DECIMAL_LS,         CONV_DECLS_DECLS},
    {REC_DECIMAL_LS,         REC_BYTE_F_ASCII,       CONV_DECLS_ASCII},
    {REC_DECIMAL_LS,         REC_DECIMAL_UNSIGNED,   CONV_DECLS_DECU},
    
    {REC_DATETIME,           REC_DATETIME,           CONV_DATETIME_DATETIME},
    {REC_DATETIME,           REC_BYTE_F_ASCII,       CONV_DATETIME_ASCII},
    {REC_DATETIME,           REC_BYTE_V_ASCII,       CONV_DATETIME_ASCII},
    {REC_DATETIME,           REC_BYTE_V_ASCII_LONG,  CONV_DATETIME_ASCII},
    {REC_DATETIME,           REC_NCHAR_F_UNICODE,     CONV_DATETIME_UNICODE},
    {REC_DATETIME,           REC_NCHAR_V_UNICODE,     CONV_DATETIME_UNICODE},

    {REC_DATETIME,           REC_BIN64_SIGNED,       CONV_DATETIME_BIN64S},

    {REC_INT_YEAR,           REC_INT_MONTH,          CONV_INTERVALY_INTERVALMO},

    // see proc getConvCaseDatatypes.
    {REC_INT_YEAR,      REC_BYTE_F_ASCII,      CONV_INTERVAL_ASCII},
    {REC_INT_YEAR,      REC_BYTE_V_ASCII,      CONV_INTERVAL_ASCII},
    {REC_INT_YEAR,      REC_NCHAR_F_UNICODE,   CONV_INTERVAL_UNICODE},
    {REC_INT_YEAR,      REC_NCHAR_V_UNICODE,   CONV_INTERVAL_UNICODE},


    {REC_INT_MONTH,          REC_INT_YEAR,           CONV_INTERVALMO_INTERVALY},
     
    {REC_INT_DAY,            REC_INT_HOUR,           CONV_INTERVALD_INTERVALH},
    {REC_INT_DAY,            REC_INT_MINUTE,         CONV_INTERVALD_INTERVALM},
    {REC_INT_DAY,            REC_INT_SECOND,         CONV_INTERVALD_INTERVALS},

    {REC_INT_HOUR,           REC_INT_DAY,            CONV_INTERVALH_INTERVALD},
    {REC_INT_HOUR,           REC_INT_MINUTE,         CONV_INTERVALH_INTERVALM},
    {REC_INT_HOUR,           REC_INT_SECOND,         CONV_INTERVALH_INTERVALS},

    {REC_INT_MINUTE,         REC_INT_DAY,            CONV_INTERVALM_INTERVALD},
    {REC_INT_MINUTE,         REC_INT_HOUR,           CONV_INTERVALM_INTERVALH},
    {REC_INT_MINUTE,         REC_INT_SECOND,         CONV_INTERVALM_INTERVALS},

    {REC_INT_SECOND,         REC_INT_DAY,            CONV_INTERVALS_INTERVALD},
    {REC_INT_SECOND,         REC_INT_HOUR,           CONV_INTERVALS_INTERVALH},
    {REC_INT_SECOND,         REC_INT_MINUTE,         CONV_INTERVALS_INTERVALM},

    // on the next one, we have to consider scale difference also to distinguish
    // between CONV_INTERVALS_INTERVALS_DIV and CONV_INTERVALS_INTERVALS_MULT
    {REC_INT_SECOND,         REC_INT_SECOND,         CONV_INTERVALS_INTERVALS_DIV},
 
    {REC_BYTE_F_ASCII,       REC_BPINT_UNSIGNED,         CONV_ASCII_BIN16U},
    {REC_BYTE_F_ASCII,       REC_BIN16_SIGNED,           CONV_ASCII_BIN16S},
    {REC_BYTE_F_ASCII,       REC_BIN16_UNSIGNED,         CONV_ASCII_BIN16U},    
    {REC_BYTE_F_ASCII,       REC_BIN32_SIGNED,           CONV_ASCII_BIN32S},
    {REC_BYTE_F_ASCII,       REC_BIN32_UNSIGNED,         CONV_ASCII_BIN32U},
    {REC_BYTE_F_ASCII,       REC_BIN64_SIGNED,           CONV_ASCII_BIN64S},
    {REC_BYTE_F_ASCII,       REC_DECIMAL_LSE,            CONV_ASCII_DEC},
    {REC_BYTE_F_ASCII,       REC_DECIMAL_UNSIGNED,       CONV_ASCII_DEC},

    {REC_BYTE_F_ASCII,       REC_FLOAT32,                CONV_ASCII_FLOAT32},
    {REC_BYTE_F_ASCII,       REC_FLOAT64,                CONV_ASCII_FLOAT64},

    {REC_BYTE_F_ASCII,       REC_TDM_FLOAT32,             CONV_ASCII_FLOAT32TDM},
    {REC_BYTE_F_ASCII,       REC_TDM_FLOAT64,             CONV_ASCII_FLOAT64TDM},

    {REC_BYTE_F_ASCII,       REC_NUM_BIG_UNSIGNED,       CONV_ASCII_BIGNUM},
    {REC_BYTE_F_ASCII,       REC_NUM_BIG_SIGNED,         CONV_ASCII_BIGNUM},
    {REC_BYTE_F_ASCII,       REC_BYTE_F_ASCII,           CONV_ASCII_F_F},
    {REC_BYTE_F_ASCII,       REC_BYTE_V_ASCII,           CONV_ASCII_F_V},
    {REC_BYTE_F_ASCII,       REC_BYTE_V_ASCII_LONG,      CONV_ASCII_F_V},
    {REC_BYTE_F_ASCII,       REC_BYTE_V_ANSI,            CONV_ASCII_TO_ANSI_V}, 
    {REC_BYTE_F_ASCII,       REC_DATETIME,               CONV_ASCII_DATETIME},
    // see proc getConvCaseDatatypes.
    {REC_BYTE_F_ASCII,       REC_INT_YEAR,               CONV_ASCII_INTERVAL},
    {REC_BYTE_F_ASCII,       REC_NCHAR_F_UNICODE,        CONV_ASCII_UNICODE_F},
    {REC_BYTE_F_ASCII,       REC_NCHAR_V_UNICODE,        CONV_ASCII_UNICODE_V},
    {REC_BYTE_F_ASCII,       REC_NCHAR_V_ANSI_UNICODE,   CONV_ASCII_TO_ANSI_V_UNICODE},

// 12/8/97: added for Unicode. Note the importance of
// grouping tuples with similar source types together.
    {REC_NCHAR_F_UNICODE,    REC_NCHAR_F_UNICODE,        CONV_UNICODE_F_F},
    {REC_NCHAR_F_UNICODE,    REC_NCHAR_V_UNICODE,        CONV_UNICODE_F_V},
    {REC_NCHAR_F_UNICODE,    REC_BYTE_F_ASCII,        CONV_UNICODE_F_ASCII_F},
    {REC_NCHAR_F_UNICODE,    REC_MBYTE_F_SJIS,        CONV_UNICODE_F_SJIS_F},
    {REC_NCHAR_F_UNICODE,    REC_MBYTE_V_SJIS,        CONV_UNICODE_F_SJIS_V},
    {REC_NCHAR_F_UNICODE,    REC_MBYTE_LOCALE_F, CONV_UNICODE_F_MBYTE_LOCALE_F},
    {REC_NCHAR_F_UNICODE,    REC_SBYTE_LOCALE_F, CONV_UNICODE_F_SBYTE_LOCALE_F},
    {REC_NCHAR_F_UNICODE,    REC_DATETIME,         CONV_UNICODE_DATETIME},
    {REC_NCHAR_F_UNICODE,    REC_BIN16_SIGNED,     CONV_UNICODE_BIN16S},
    {REC_NCHAR_F_UNICODE,    REC_BIN16_UNSIGNED,   CONV_UNICODE_BIN16U},
    {REC_NCHAR_F_UNICODE,    REC_BPINT_UNSIGNED,   CONV_UNICODE_BIN16U},
    {REC_NCHAR_F_UNICODE,    REC_BIN32_UNSIGNED,   CONV_UNICODE_BIN32U},
    {REC_NCHAR_F_UNICODE,    REC_BIN32_SIGNED,     CONV_UNICODE_BIN32S},
    {REC_NCHAR_F_UNICODE,    REC_BIN64_SIGNED,     CONV_UNICODE_BIN64S},
    {REC_NCHAR_F_UNICODE,    REC_FLOAT32,          CONV_UNICODE_FLOAT32},
    {REC_NCHAR_F_UNICODE,    REC_FLOAT64,          CONV_UNICODE_FLOAT64},
    {REC_NCHAR_F_UNICODE,    REC_DECIMAL_LSE,      CONV_UNICODE_DEC},
    {REC_NCHAR_F_UNICODE,    REC_DECIMAL_UNSIGNED, CONV_UNICODE_DEC},
    {REC_NCHAR_F_UNICODE,    REC_NUM_BIG_UNSIGNED,       CONV_UNICODE_BIGNUM},
    {REC_NCHAR_F_UNICODE,    REC_NUM_BIG_SIGNED,         CONV_UNICODE_BIGNUM},
    {REC_NCHAR_F_UNICODE,    REC_INT_YEAR,               CONV_UNICODE_INTERVAL},
    {REC_NCHAR_F_UNICODE,    REC_BYTE_V_ASCII,     CONV_UNICODE_F_ASCII_V},
    {REC_NCHAR_F_UNICODE, REC_NCHAR_V_ANSI_UNICODE,CONV_UNICODE_TO_ANSI_V_UNICODE},
    {REC_NCHAR_F_UNICODE,    REC_BYTE_V_ANSI, CONV_UNICODE_F_ANSI_V},

    {REC_NCHAR_V_UNICODE,    REC_BYTE_F_ASCII,        CONV_UNICODE_V_ASCII_F},
    {REC_NCHAR_V_UNICODE,    REC_MBYTE_F_SJIS,        CONV_UNICODE_V_SJIS_F},
    {REC_NCHAR_V_UNICODE,    REC_MBYTE_V_SJIS,        CONV_UNICODE_V_SJIS_V},
    {REC_NCHAR_V_UNICODE,    REC_NCHAR_F_UNICODE,     CONV_UNICODE_V_F},
    {REC_NCHAR_V_UNICODE,    REC_NCHAR_V_UNICODE,     CONV_UNICODE_V_V},
    {REC_NCHAR_V_UNICODE,    REC_MBYTE_LOCALE_F, CONV_UNICODE_V_MBYTE_LOCALE_F},
    {REC_NCHAR_V_UNICODE,    REC_SBYTE_LOCALE_F, CONV_UNICODE_V_SBYTE_LOCALE_F},
    {REC_NCHAR_V_UNICODE,    REC_DATETIME,       CONV_UNICODE_DATETIME},
    {REC_NCHAR_V_UNICODE,    REC_BIN16_SIGNED,   CONV_UNICODE_BIN16S},
    {REC_NCHAR_V_UNICODE,    REC_BIN16_UNSIGNED,   CONV_UNICODE_BIN16U},
    {REC_NCHAR_V_UNICODE,    REC_BPINT_UNSIGNED,   CONV_UNICODE_BIN16U},
    {REC_NCHAR_V_UNICODE,    REC_BIN32_UNSIGNED,   CONV_UNICODE_BIN32U},
    {REC_NCHAR_V_UNICODE,    REC_BIN32_SIGNED,     CONV_UNICODE_BIN32S},
    {REC_NCHAR_V_UNICODE,    REC_BIN64_SIGNED,     CONV_UNICODE_BIN64S},
    {REC_NCHAR_V_UNICODE,    REC_FLOAT32,          CONV_UNICODE_FLOAT32},
    {REC_NCHAR_V_UNICODE,    REC_FLOAT64,          CONV_UNICODE_FLOAT64},
    {REC_NCHAR_V_UNICODE,    REC_DECIMAL_LSE,      CONV_UNICODE_DEC},
    {REC_NCHAR_V_UNICODE,    REC_DECIMAL_UNSIGNED, CONV_UNICODE_DEC},
    {REC_NCHAR_V_UNICODE,    REC_NUM_BIG_UNSIGNED,       CONV_UNICODE_BIGNUM},
    {REC_NCHAR_V_UNICODE,    REC_NUM_BIG_SIGNED,         CONV_UNICODE_BIGNUM},
    {REC_NCHAR_V_UNICODE,    REC_INT_YEAR,               CONV_UNICODE_INTERVAL},
    {REC_NCHAR_V_UNICODE,    REC_BYTE_V_ASCII,     CONV_UNICODE_V_ASCII_V},
    {REC_NCHAR_V_UNICODE, REC_NCHAR_V_ANSI_UNICODE,CONV_UNICODE_TO_ANSI_V_UNICODE},
    {REC_NCHAR_V_UNICODE,    REC_BYTE_V_ANSI,      CONV_UNICODE_V_ANSI_V},


    {REC_BYTE_V_ASCII,       REC_BPINT_UNSIGNED,         CONV_ASCII_BIN16U},    
    {REC_BYTE_V_ASCII,       REC_BIN16_SIGNED,           CONV_ASCII_BIN16S},
    {REC_BYTE_V_ASCII,       REC_BIN16_UNSIGNED,         CONV_ASCII_BIN16U},    
    {REC_BYTE_V_ASCII,       REC_BIN32_SIGNED,           CONV_ASCII_BIN32S},
    {REC_BYTE_V_ASCII,       REC_BIN32_UNSIGNED,         CONV_ASCII_BIN32U},
    {REC_BYTE_V_ASCII,       REC_BIN64_SIGNED,           CONV_ASCII_BIN64S},
    {REC_BYTE_V_ASCII,       REC_DECIMAL_LSE,            CONV_ASCII_DEC},
    {REC_BYTE_V_ASCII,       REC_DECIMAL_UNSIGNED,       CONV_ASCII_DEC},

    {REC_BYTE_V_ASCII,       REC_FLOAT32,                CONV_ASCII_FLOAT32},
    {REC_BYTE_V_ASCII,       REC_FLOAT64,                CONV_ASCII_FLOAT64},

    {REC_BYTE_V_ASCII,       REC_DATETIME,               CONV_ASCII_DATETIME},
    {REC_BYTE_V_ASCII,       REC_NUM_BIG_UNSIGNED,       CONV_ASCII_BIGNUM},
    {REC_BYTE_V_ASCII,       REC_NUM_BIG_SIGNED,         CONV_ASCII_BIGNUM},
    {REC_BYTE_V_ASCII,       REC_BYTE_F_ASCII,           CONV_ASCII_V_F},
    {REC_BYTE_V_ASCII,       REC_BYTE_V_ASCII,           CONV_ASCII_V_V},
    {REC_BYTE_V_ASCII,       REC_BYTE_V_ASCII_LONG,      CONV_ASCII_V_V},
    {REC_BYTE_V_ASCII,       REC_BYTE_V_ANSI,            CONV_ASCII_TO_ANSI_V},

    // see proc getConvCaseDatatypes.
    {REC_BYTE_V_ASCII,       REC_INT_YEAR,               CONV_ASCII_INTERVAL},

    {REC_BYTE_V_ASCII,       REC_NCHAR_F_UNICODE,        CONV_ASCII_UNICODE_F},
    {REC_BYTE_V_ASCII,       REC_NCHAR_V_UNICODE,        CONV_ASCII_UNICODE_V},
    {REC_BYTE_V_ASCII,       REC_NCHAR_V_ANSI_UNICODE,   CONV_ASCII_TO_ANSI_V_UNICODE},


    {REC_BYTE_V_ASCII_LONG,   REC_BPINT_UNSIGNED,         CONV_ASCII_BIN16U},  
    {REC_BYTE_V_ASCII_LONG,   REC_BIN16_SIGNED,           CONV_ASCII_BIN16S},
    {REC_BYTE_V_ASCII_LONG,   REC_BIN16_UNSIGNED,         CONV_ASCII_BIN16U},    
    {REC_BYTE_V_ASCII_LONG,   REC_BIN32_SIGNED,           CONV_ASCII_BIN32S},
    {REC_BYTE_V_ASCII_LONG,   REC_BIN32_UNSIGNED,         CONV_ASCII_BIN32U},
    {REC_BYTE_V_ASCII_LONG,   REC_BIN64_SIGNED,           CONV_ASCII_BIN64S},
    {REC_BYTE_V_ASCII_LONG,   REC_DECIMAL_LSE,            CONV_ASCII_DEC},
    {REC_BYTE_V_ASCII_LONG,   REC_DECIMAL_UNSIGNED,       CONV_ASCII_DEC},

    {REC_BYTE_V_ASCII_LONG,   REC_FLOAT32,                CONV_ASCII_FLOAT32},
    {REC_BYTE_V_ASCII_LONG,   REC_FLOAT64,                CONV_ASCII_FLOAT64},

    {REC_BYTE_V_ASCII_LONG,   REC_DATETIME,               CONV_ASCII_DATETIME},
    {REC_BYTE_V_ASCII_LONG,   REC_NUM_BIG_UNSIGNED,       CONV_ASCII_BIGNUM},
    {REC_BYTE_V_ASCII_LONG,   REC_NUM_BIG_SIGNED,         CONV_ASCII_BIGNUM},
    {REC_BYTE_V_ASCII_LONG,   REC_BYTE_F_ASCII,           CONV_ASCII_V_F},
    {REC_BYTE_V_ASCII_LONG,   REC_BYTE_V_ASCII,           CONV_ASCII_V_V},
    {REC_BYTE_V_ASCII_LONG,   REC_BYTE_V_ASCII_LONG,      CONV_ASCII_V_V},
    {REC_BYTE_V_ASCII_LONG,   REC_BYTE_V_ANSI,            CONV_ASCII_TO_ANSI_V},

    {REC_BYTE_V_ANSI,        REC_BYTE_F_ASCII,       CONV_ANSI_V_TO_ASCII_F},
    {REC_BYTE_V_ANSI,        REC_BYTE_V_ASCII,       CONV_ANSI_V_TO_ASCII_V},
    {REC_BYTE_V_ANSI,        REC_BYTE_V_ASCII_LONG,  CONV_ANSI_V_TO_ASCII_V},
    {REC_BYTE_V_ANSI,        REC_BYTE_V_ANSI,        CONV_ANSI_V_TO_ANSI_V},

    {REC_NCHAR_V_ANSI_UNICODE, REC_NCHAR_V_UNICODE, CONV_ANSI_V_UNICODE_TO_UNICODE_V},
    {REC_NCHAR_V_ANSI_UNICODE, REC_NCHAR_V_ANSI_UNICODE, CONV_ANSI_V_UNICODE_TO_ANSI_V_UNICODE},
    {REC_NCHAR_V_ANSI_UNICODE, REC_BYTE_V_ASCII, CONV_ANSI_V_UNICODE_TO_ASCII_V},
    {REC_NCHAR_V_ANSI_UNICODE, REC_BYTE_F_ASCII, CONV_ANSI_V_UNICODE_TO_ASCII_F},
    {REC_NCHAR_V_ANSI_UNICODE, REC_NCHAR_F_UNICODE, CONV_ANSI_V_UNICODE_TO_UNICODE_F},
    {REC_NCHAR_V_ANSI_UNICODE, REC_NCHAR_V_UNICODE, CONV_ANSI_V_UNICODE_TO_UNICODE_V},
    {REC_BLOB,          REC_BLOB,         CONV_BLOB_BLOB},
    {REC_BLOB,          REC_BYTE_F_ASCII,      CONV_BLOB_ASCII_F},
    {REC_CLOB,          REC_CLOB,         CONV_BLOB_BLOB},
    {REC_CLOB,          REC_BYTE_F_ASCII,      CONV_BLOB_ASCII_F}
  };
  
  getConvCaseDatatypes(sourceType, sourceLen, sourceType, targetType, targetLen, targetType, scaleDifference);

  case_index = CONV_NOT_SUPPORTED;

  Int32 max_array_size = sizeof(conv_s) / sizeof(conv_struct);

  Int32 i = 0;
  while ((i < max_array_size) && (conv_s[i].type_op1 != sourceType)) i++;

  NABoolean done = FALSE;
  while ((!done) && (i < max_array_size)) {
    if (conv_s[i].type_op1 != sourceType)
      done = TRUE;
    else
      if (conv_s[i].type_op2 == targetType) {
	done = TRUE;
	case_index = conv_s[i].index;
      }
    i++;
  };

  if (case_index == CONV_NOT_SUPPORTED) {
#if (!defined (__TANDEM) && !defined(__EID))
    //    cout << sourceType << " to " 
    //	 << targetType << " conversion not yet supported. \n";
#endif
  };


  if (case_index == CONV_INTERVALS_INTERVALS_DIV)
    {
    // have to look at scale difference to distinguish whether
    // to divide or multiply by the scale factor
    if (scaleDifference < 0)
      {
      case_index = CONV_INTERVALS_INTERVALS_MULT;
      }
    }

  return (enum conv_case_index)case_index;

};

void ex_conv_clause::set_case_index() {
  // Test added to allow conditional union to be rowset-aware
  if((getOperand(0)->getRowsetSize() > 0) && (getOperand(1)->getRowsetSize() > 0)) 
    { 
      case_index = CONV_ASCII_F_F; 
      SimpleType *op0 = (SimpleType *) getOperand(0);
      SimpleType *op1 = (SimpleType *) getOperand(1);
      if (!(op0->getUseTotalRowsetSize())) {
#pragma nowarn(1506)   // warning elimination 
	op0->setLength(sizeof(Lng32) + (op0->getLength() * op0->getRowsetSize()));
#pragma warn(1506)  // warning elimination 
	op0->setUseTotalRowsetSize();
	op1->setUseTotalRowsetSize();
      }
      op1->setLength(op0->getLength());
    }
  else
    case_index = find_case_index(getOperand(1)->getDatatype(),
                                 getOperand(1)->getLength(),
                                 getOperand(0)->getDatatype(),
                                 getOperand(0)->getLength(),
                                 getOperand(1)->getScale() - getOperand(0)->getScale());

  if (case_index == CONV_NOT_SUPPORTED)
    {
      if ((getOperand(0)->isComplexType()) && !(getOperand(1)->isComplexType())) 
	case_index = CONV_SIMPLE_TO_COMPLEX;
      else if (! (getOperand(0)->isComplexType()) && (getOperand(1)->isComplexType()))
	case_index = CONV_COMPLEX_TO_SIMPLE;
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
  set_case_index();
  return ex_clause::fixup(space, exHeap, constants_area, temps_area,
			  persistentArea,
			  fixupConstsAndTemps, spaceCompOnly);
}

ex_expr::exp_return_type ExAuditImage::fixup(Space * space,
					     CollHeap * exHeap,
					     char * constantsArea,
					     char * tempsArea,
					     char * persistentArea,
					     short fixupFlag,
					     NABoolean spaceCompOnly) 
{

  ex_expr::exp_return_type retcode;

  assert ( auditImageContainerExpr_ != (ExpDP2ExprPtr) NULL );
  ex_expr * auditRowImageExpr = (ex_expr *)auditImageContainerExpr_->getExpr();

  assert (auditRowImageExpr != NULL);

  retcode = auditRowImageExpr->fixup(0,       // base is really not use. so set it to zero.
				     ex_expr::PCODE_NONE, 
				     // The ExpressionMode enumeration specifies 
				     // the type of PCODE to generate at expression 
				     // fixup time.  Also, 
				     // config's for error injection at fixup time.
				     NULL,  // tcb. For error injection testing. 
				     // We porbably don't need it, so setting it to NULL
				     space, 
				     exHeap, 
				     spaceCompOnly, // computeSpaceOnly: if TRUE, then compute space 
				     // requirement only. Do not make any changes to the
				     // generated expressions,(like assigning tempsArea
				     // , assigning generated pcode, etc).
				     NULL);
  
  if (retcode != ex_expr::EXPR_OK) 
    return retcode;
  
  
  return ex_clause::fixup(space, exHeap, constantsArea,tempsArea, 
			  persistentArea, fixupFlag, spaceCompOnly);
}

