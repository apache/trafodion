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
#include "errno.h"


// on NT platform, call system math functions.
// On NSK, call our own version of math functions named as Math*
#include <math.h>

// define Math* functions to be the same as system functions.
#define MathCeil(op, err) ceil(op)
#define MathExp(op, err) exp(op)
#define MathFloor(op, err) floor(op)
#define MathLog(op, err) log(op)
#define MathLog10(op, err) log10(op)
#define MathModf(op, err) modf(op)
#define MathPow(op1, op2, err) pow(op1, op2)
//extern double MathPow(double Base, double Power, short &err);
#define MathSqrt(op, err) sqrt(op)
#define MathFmod(op, err) fmod(op)
#define MathSin(op, err) sin(op)
#define MathCos(op, err) cos(op)
#define MathTan(op, err) tan(op)
#define MathAsin(op, err) asin(op)
#define MathAcos(op, err) acos(op)
#define MathAtan(op, err) atan(op)
#define MathAtan2(op1, op2, err) atan2(op1, op2)
#define MathSinh(op, err) sinh(op)
#define MathCosh(op, err) cosh(op)
#define MathTanh(op, err) tanh(op)

#include "exp_stdh.h"
#include "exp_math_func.h"
#include "ComDiags.h"
#include "ComSysUtils.h"

#define HUGE_VAL_REAL64         1.15792089237316192E+77
#define HUGE_VAL_INT64          0777777777777777777777

ex_expr::exp_return_type ex_function_abs::eval(char *op_data[],
					       CollHeap *heap,
					       ComDiagsArea** diagsArea)
{
  ex_expr::exp_return_type retcode = ex_expr::EXPR_OK;

  switch (getOperand(1)->getDatatype())
    {
    case REC_BIN16_SIGNED:
#pragma nowarn(1506)   // warning elimination 
      *(short *)op_data[0] = (*(short *)op_data[1] < 0 ? -*(short *)op_data[1]
			      : *(short *)op_data[1]);
#pragma warn(1506)   // warning elimination 
      break;
      
    case REC_BIN32_SIGNED:
      *(Lng32 *)op_data[0] = labs(*(Lng32 *)op_data[1]);
      break;
 
    case REC_BIN64_SIGNED:
      *(Int64 *)op_data[0] = (*(Int64 *)op_data[1] < 0 ? -*(Int64 *)op_data[1]
			      : *(Int64 *)op_data[1]);
      break;
      
    case REC_FLOAT64:
      *(double *)op_data[0] = fabs(*(double *)op_data[1]);
      break;

    default:
      ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
      retcode = ex_expr::EXPR_ERROR;
      break;
      // LCOV_EXCL_STOP
    }
  
  return retcode;
}
;

#define PIVALUE 3.1415926E0;
// LCOV_EXCL_START
ex_expr::exp_return_type ExFunctionMath::evalUnsupportedOperations(
     char *op_data[],
     CollHeap *heap,
     ComDiagsArea** diagsArea)
{
  ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
  return ex_expr::EXPR_ERROR;
}
// LCOV_EXCL_STOP
ex_expr::exp_return_type ExFunctionMath::eval(char *op_data[],
					      CollHeap *heap,
					      ComDiagsArea** diagsArea)
{
  ex_expr::exp_return_type retcode = ex_expr::EXPR_OK;

  short err = 0;
  errno = 0;

  if ((getOperand()) &&
      (getOperand(0)->getDatatype() != REC_FLOAT64))
    {
      return evalUnsupportedOperations(op_data, heap, diagsArea);
    }

  switch (getOperType())
    {
    case ITM_DEGREES:
      // radians to degrees
      *(double *)op_data[0] = (*(double *)op_data[1] * 180E0) / PIVALUE;
      break;

    case ITM_PI:
      *(double *)op_data[0] = PIVALUE;
      break;

    case ITM_RADIANS:
      // degrees to radians
      *(double *)op_data[0] = (*(double *)op_data[1] / 180E0) * PIVALUE;
      break;

    case ITM_ROUND:
    {
      double op, res;
      short err1 = 0, err2 = 0, err3 = 0;
      Int32 roundTo;

      roundTo = (getNumOperands() > 2) ? (Int32)(*((double*)op_data[2])) : 0;

      switch (getOperand(1)->getDatatype()) {
        case REC_IEEE_FLOAT32:
          op = *((float*)op_data[1]);
          break;
        case REC_IEEE_FLOAT64:
          op = *((double*)op_data[1]);
          break;
        default:
            // LCOV_EXCL_START
          ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
          **diagsArea << DgString0("ROUND");
          return ex_expr::EXPR_ERROR;
          // LCOV_EXCL_STOP
      }

      //
      // For commenting purposes, assume the following:
      //
      // op      = -12.58
      // roundTo = 0
      //
      //

      double pow1 = MathPow(10.0, roundTo, err1);             // = 1
      double pow2 = MathPow(10.0, -roundTo, err2);            // = 1
      double pow3 = MathPow(10.0, roundTo+1, err3);           // = 10


      double abs1 = (op < 0.0) ? -op : op;                    // = 12.58
      double sign = (op < 0.0) ? -1.0 : 1.0;                  // = -1.0

      double floor1 = MathFloor(abs1*pow3, err);              // = 125
      double floor2 = 10.0 * MathFloor(floor1 / 10.0, err);   // = 120
      double floor3 = floor1 - floor2;                        // = 5

      double floor4 = MathFloor(abs1*pow1, err);              // = 12
      double floor5 = pow2 * sign;                            // = -1.0

      // Is digit at rounding position less than 5?
      if (floor3 < 5.0) {
        res = floor4 * floor5;                                // = -12.0
      }
      // Is digit at rounding position greater than 5?
      else if (floor3 > 5.0) {
        res = (floor4 + 1.0) * floor5;                        // = -13.0
      }
      else {
        // Are digits after rounding position greater than 0?
        if (((abs1*pow3) - floor1) > 0.0)
          res = (floor4 + 1.0) * floor5;                      // = -13.0
        else {
          double floor6 = 2.0 * MathFloor(floor4 / 2.0, err); // = 12.0

          // Now round to even
          if ((floor4 - floor6) == 1.0)
            res = (floor4 + 1.0) * floor5;                    // = -13.0
          else
            res = floor4 * floor5;                            // = -12.0
        }
      }

      *(double *)op_data[0] = res;

      break;
    }

    case ITM_SCALE_TRUNC:
      {
          // LCOV_EXCL_START
	ExRaiseSqlError(heap, diagsArea, EXE_MATH_FUNC_NOT_SUPPORTED);
	if (getOperType() == ITM_ROUND)
	  **diagsArea << DgString0("ROUND");
	else
	  **diagsArea << DgString0("TRUNCATE");
	  
	retcode = ex_expr::EXPR_ERROR;
      }
     break;
     // LCOV_EXCL_STOP

    case ITM_ACOS:
      if ((*(double *)op_data[1] < -1) ||
	  (*(double *)op_data[1] > 1))
	{
	  ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
	  **diagsArea << DgString0("ACOS");
	  return ex_expr::EXPR_ERROR;
	}

      *(double *)op_data[0] = MathAcos(*(double *)op_data[1], err);
      break;

    case ITM_ASIN:
      if ((*(double *)op_data[1] < -1) ||
	  (*(double *)op_data[1] > 1))
	{
	  ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
	  **diagsArea << DgString0("ASIN");

	  return ex_expr::EXPR_ERROR;
	}

      *(double *)op_data[0] = MathAsin(*(double *)op_data[1], err);
      break;

    case ITM_ATAN:
      // No error checks, all numeric values allowed.

      *(double *)op_data[0] = MathAtan(*(double *)op_data[1], err);
      break;
      
    case ITM_ATAN2:
      // No error checks, all numeric values allowed.

      *(double *)op_data[0] = MathAtan2(*(double *)op_data[1],
				    *(double *)op_data[2], err);
      break;

    case ITM_CEIL:
      // No error checks, all numeric values allowed.

      *(double *)op_data[0] = MathCeil(*(double *)op_data[1], err);
      break;

    case ITM_COS:

       *(double *)op_data[0] = MathCos(*(double *)op_data[1], err);
      break;

    case ITM_COSH:
      *(double *)op_data[0] = MathCosh(*(double *)op_data[1], err);
      if (*(double *)op_data[0] == HUGE_VAL)
	{
	  ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
	  **diagsArea << DgString0("COSH");
	  return ex_expr::EXPR_ERROR;
	}
      break;

    case ITM_EXP:
      *(double *)op_data[0] = MathExp(*(double *)op_data[1], err);

      // Check for overflow. 
      if (err) 
	// if  (*(double *)op_data[0] == HUGE_VAL_REAL64)
	{
	  // Overflow

	  ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
	  **diagsArea << DgString0("EXP");
	  return ex_expr::EXPR_ERROR;
	}

      break;

    case ITM_FLOOR:
      // No error checks, all numeric values allowed.

      *(double *)op_data[0] = MathFloor(*(double *)op_data[1], err);
      break;

    case ITM_LOG:
      if (*(double *)op_data[1] <= 0)
	{
	  ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
	  **diagsArea << DgString0("LOG");
	  return ex_expr::EXPR_ERROR;
	}
      
      *(double *)op_data[0] = MathLog(*(double *)op_data[1], err);
      break;

    case ITM_LOG10:
      if (*(double *)op_data[1] <= 0)
	{
	  ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
	  **diagsArea << DgString0("LOG10");
	  return ex_expr::EXPR_ERROR;
	}
      
      *(double *)op_data[0] = MathLog10(*(double *)op_data[1], err);
      break;

    case ITM_SIN:

      *(double *)op_data[0] = MathSin(*(double *)op_data[1], err);
      break;

    case ITM_SINH:
      *(double *)op_data[0] = MathSinh(*(double *)op_data[1], err);
      if (*(double *)op_data[0] == HUGE_VAL)
	{
	  ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
	  **diagsArea << DgString0("SINH");
	  return ex_expr::EXPR_ERROR;
	}
	
      break;

    case ITM_SQRT:
      if (*(double *)op_data[1] < 0)
	{
	  ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
	  **diagsArea << DgString0("SQRT");
	  return ex_expr::EXPR_ERROR;
	}
      
      *(double *)op_data[0] = MathSqrt(*(double *)op_data[1], err);
      break;

    case ITM_TAN:

      *(double *)op_data[0] = MathTan(*(double *)op_data[1], err);
      break;

    case ITM_TANH:
      // No error checks, all numeric values allowed.

      *(double *)op_data[0] = MathTanh(*(double *)op_data[1], err);
      break;
     
    case ITM_EXPONENT:
    case ITM_POWER:
      if (((*(double *)op_data[1] == 0) &&
	   (*(double *)op_data[2] <= 0)) ||
	  ((*(double *)op_data[1] < 0) &&
	   (MathFloor(*(double *)op_data[2], err) != *(double *)op_data[2])))
	{
	  ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
	  **diagsArea << DgString0("POWER");
	  return ex_expr::EXPR_ERROR;
	}

      *(double *)op_data[0] = MathPow(*(double *)op_data[1],
				  *(double *)op_data[2], err);
        if (errno == ERANGE)	
	{
	  // Overflow

	  ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
	  **diagsArea << DgString0("POWER");
	  return ex_expr::EXPR_ERROR;
	}
	

      break;

    default:
      {
	ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
	retcode = ex_expr::EXPR_ERROR;
      }
      break;
     }
  
  return retcode;
}

ex_expr::exp_return_type ExFunctionBitOper::eval(char *op_data[],
						 CollHeap *heap,
						 ComDiagsArea** diagsArea)
{
  ex_expr::exp_return_type retcode = ex_expr::EXPR_OK;

  switch (getOperType())
    {
    case ITM_BITAND:
      {
          // LCOV_EXCL_START
	if (getOperand(0)->getDatatype() == REC_BIN32_UNSIGNED)
	  *(UInt32 *)op_data[0] =
	    *(UInt32 *)op_data[1] & *(UInt32 *)op_data[2];
	else if (getOperand(0)->getDatatype() == REC_BIN32_SIGNED)
	  *(Int32 *)op_data[0] =
	    *(Int32 *)op_data[1] & *(Int32 *)op_data[2];
	else
	  *(Int64 *)op_data[0] =
	    *(Int64 *)op_data[1] & *(Int64 *)op_data[2];
      }
    break;
    
    case ITM_BITOR:
      {
	if (getOperand(0)->getDatatype() == REC_BIN32_UNSIGNED)
	  *(UInt32 *)op_data[0] =
	    *(UInt32 *)op_data[1] | *(UInt32 *)op_data[2];
	else if (getOperand(0)->getDatatype() == REC_BIN32_SIGNED)
	  *(Int32 *)op_data[0] =
	    *(Int32 *)op_data[1] | *(Int32 *)op_data[2];
	else
	  *(Int64 *)op_data[0] =
	    *(Int64 *)op_data[1] | *(Int64 *)op_data[2];
      }
    break;
    
    case ITM_BITXOR:
      {
	if (getOperand(0)->getDatatype() == REC_BIN32_UNSIGNED)
	  *(UInt32 *)op_data[0] =
	    *(UInt32 *)op_data[1] ^ *(UInt32 *)op_data[2];
	else if (getOperand(0)->getDatatype() == REC_BIN32_SIGNED)
	  *(Int32 *)op_data[0] =
	    *(Int32 *)op_data[1] ^ *(Int32 *)op_data[2];
	else
	  *(Int64 *)op_data[0] =
	    *(Int64 *)op_data[1] ^ *(Int64 *)op_data[2];
      }
    break;
    
    case ITM_BITNOT:
      {
	if (getOperand(0)->getDatatype() == REC_BIN32_UNSIGNED)
	  *(UInt32 *)op_data[0] = ~*(UInt32 *)op_data[1];
	else if (getOperand(0)->getDatatype() == REC_BIN32_SIGNED)
	  *(Int32 *)op_data[0] = ~*(Int32 *)op_data[1];
	else if (getOperand(0)->getDatatype() == REC_BIN64_SIGNED)
	  *(Int64 *)op_data[0] = ~*(Int64 *)op_data[1];
	else
	  {
	    for (Int32 i = 0; i < getOperand(0)->getLength(); i++)
	      {
		((char*)(op_data[0]))[i] =
		  ~((char*)(op_data[1]))[i];
	      }
	  }
      }
    break;
    // LCOV_EXCL_STOP
    case ITM_BITEXTRACT:
      {
	UInt32 startBit   = *(UInt32 *)op_data[2];
	UInt32 numBits    = *(UInt32 *)op_data[3];
	UInt32 opLen      = getOperand(1)->getLength();
	UInt32 startByte = startBit / 8;
	UInt32 endByte   = (startBit + numBits - 1) / 8 ;

	if ((numBits == 0) ||
	    (endByte >= opLen))
	  {
	    ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
	    **diagsArea << DgString0("BITEXTRACT");
	    return ex_expr::EXPR_ERROR;
	  }
	
	UInt64 result = 0;

	//Int64 temp = 0;
	switch (getOperand(1)->getDatatype())
	  {
	  case REC_BIN16_SIGNED:
	  case REC_BIN16_UNSIGNED:
	    {
	      UInt16 temp;
	      temp = *(UInt16*)op_data[1] << startBit;
	      temp = temp >> (16 - numBits);
	      result = temp;
	    }
	  break;

	  case REC_BIN32_SIGNED:
	  case REC_BIN32_UNSIGNED:
	  case REC_IEEE_FLOAT32:
	    {
	      UInt32 temp;
	      // LCOV_EXCL_START
	      temp = *(UInt32*)op_data[1] << startBit;
	      temp = temp >> (32 - numBits);
	      result = temp;
	    }
	  break;

	  case REC_BIN64_SIGNED:
	  case REC_IEEE_FLOAT64:
	    {
	      result = *(Int64*)op_data[1] << startBit;
	      result = result >> (64 - numBits);
	    }
	  break;

	  default:
	    {
	      // not yet supported
	      //	      UInt32 middleBytes = 
	      //((endByte - startByte) > 1 ? (endByte - startByte) : 0);
	      ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
	      **diagsArea << DgString0("BITEXTRACT");
	      return ex_expr::EXPR_ERROR;
	    }
	      // LCOV_EXCL_STOP
	  break;
	  } // switch

	if (getOperand(0)->getDatatype() == REC_BIN32_SIGNED)
	  *(Int32*)op_data[0] = (Int32)result;
	else
	  *(Int64*)op_data[0] = result;
      }
    break;

    case ITM_CONVERTTOBITS:
      {
#pragma nowarn(1506)   // warning elimination 
	Lng32 len1 = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);
#pragma warn(1506)  // warning elimination 
	Int32 i;
	if ( DFS2REC::isDoubleCharacter(getOperand(1)->getDatatype()) ) 
	  {
	    ExRaiseSqlError(heap, diagsArea, EXE_BAD_ARG_TO_MATH_FUNC);
	    **diagsArea << DgString0("CONVERTTOBITS");
	    return ex_expr::EXPR_ERROR;
	  } 
	else 
	  {
	    for (i = 0; i < len1; i++)
	      {
		op_data[0][8*i+0] = (0x80 & op_data[1][i] ? '1' : '0');
		op_data[0][8*i+1] = (0x40 & op_data[1][i] ? '1' : '0');
		op_data[0][8*i+2] = (0x20 & op_data[1][i] ? '1' : '0');
		op_data[0][8*i+3] = (0x10 & op_data[1][i] ? '1' : '0');
		op_data[0][8*i+4] = (0x08 & op_data[1][i] ? '1' : '0');
		op_data[0][8*i+5] = (0x04 & op_data[1][i] ? '1' : '0');
		op_data[0][8*i+6] = (0x02 & op_data[1][i] ? '1' : '0');
		op_data[0][8*i+7] = (0x01 & op_data[1][i] ? '1' : '0');
	      }
	  }
	
	getOperand(0)->setVarLength(2 * len1, op_data[-MAX_OPERANDS]);
	
      }
    break;
    
    }
 
  return retcode;
}
;




