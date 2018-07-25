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

#include "exp_stdh.h"
#include "exp_clause_derived.h"
#include "exp_datetime.h"
#include "ComSysUtils.h"
#include "exp_bignum.h"

#include "exp_ovfl_ptal.h"
#include "exp_ieee.h"

Int64 EXP_FIXED_ARITH_OV_OPER(short operation,
                              Int64 op1, Int64 op2, short * ov)
{
  if (NOT ((operation == ITM_PLUS) || (operation == ITM_MINUS) ||
           (operation == ITM_TIMES) || (operation == ITM_DIVIDE)))
    return -1; // invalid operation

  short rc = 0;
  *ov = 0;

  BigNum op1BN(BigNum::BIGNUM_TEMP_LEN, BigNum::BIGNUM_TEMP_PRECISION, 0, 0);
  BigNum op2BN(BigNum::BIGNUM_TEMP_LEN, BigNum::BIGNUM_TEMP_PRECISION, 0, 0);

  char op1BNdata[100];
  char op2BNdata[100];

  SimpleType op1ST(REC_BIN64_SIGNED, sizeof(Int64), 0, 0,
		   ExpTupleDesc::SQLMX_FORMAT,
		   8, 0, 0, 0, Attributes::NO_DEFAULT, 0);
  SimpleType op2ST(REC_BIN64_SIGNED, sizeof(Int64), 0, 0,
		   ExpTupleDesc::SQLMX_FORMAT,
		   8, 0, 0, 0, Attributes::NO_DEFAULT, 0);

  char * op1_data[2];
  char * op2_data[2];

  op1_data[0] = op1BNdata;
  op1_data[1] = (char*)&op1;

  op2_data[0] = op2BNdata;
  op2_data[1] = (char*)&op2;

  op1BN.castFrom(&op1ST, op1_data, NULL, NULL);
  op2BN.castFrom(&op2ST, op2_data, NULL, NULL);

  char * oper_data[3];
  char operBNdata[100];
  oper_data[0] = operBNdata;
  oper_data[1] = op1BNdata;
  oper_data[2] = op2BNdata;

  BigNum operBN(BigNum::BIGNUM_TEMP_LEN, BigNum::BIGNUM_TEMP_PRECISION, 0, 0);

  switch (operation)
    {
    case ITM_PLUS:
      {
        rc = operBN.add(&op1BN, &op2BN, oper_data);
      }
      break;

    case ITM_MINUS:
      {
        rc = operBN.sub(&op1BN, &op2BN, oper_data);
      }
      break;

    case ITM_TIMES:
      {
        rc = operBN.mul(&op1BN, &op2BN, oper_data);
      }
      break;

    case ITM_DIVIDE:
      {
        char tempSpace[200];
        operBN.setTempSpaceInfo(ITM_DIVIDE, (ULong)tempSpace, 200);
        rc = operBN.div(&op1BN, &op2BN, oper_data, NULL, NULL);
      }
      break;

    default:
      return -1;
    } // switch

  if (rc)
    {
      *ov = 1;
      return -1;
    }

  SimpleType resultST(REC_BIN64_SIGNED, sizeof(Int64), 0, 0,
		      ExpTupleDesc::SQLMX_FORMAT,
		      sizeof(Int64), 0, 0, 0, Attributes::NO_DEFAULT, 0);
  
  Int64 result;
  op1_data[0] = (char*)&result; 
  op1_data[1] = oper_data[0];

  rc = convDoIt(op1_data[1], BigNum::BIGNUM_TEMP_LEN, REC_NUM_BIG_SIGNED, 
                BigNum::BIGNUM_TEMP_PRECISION, 0,
		op1_data[0],  sizeof(Int64), REC_BIN64_SIGNED, 0, 0,
		NULL, 0, NULL, NULL);
  if (rc)
    {
      *ov = 1;
      return -1;
    }

  return result;
}

Int64 EXP_FIXED_OV_ADD(Int64 op1,Int64 op2, short * ov)
{
  return EXP_FIXED_ARITH_OV_OPER(ITM_PLUS, op1, op2, ov);
}

Int64 EXP_FIXED_OV_SUB(Int64 op1,Int64 op2, short * ov)
{
  return EXP_FIXED_ARITH_OV_OPER(ITM_MINUS, op1, op2, ov);
}

Int64 EXP_FIXED_OV_MUL(Int64 op1,Int64 op2, short * ov)
{
  return EXP_FIXED_ARITH_OV_OPER(ITM_TIMES, op1, op2, ov);
}

Int64 EXP_FIXED_OV_DIV(Int64 op1,Int64 op2, short * ov)
{
  return EXP_FIXED_ARITH_OV_OPER(ITM_DIVIDE, op1, op2, ov);
}


///////////////////////////////////////////////////////////////
short EXP_BIGN_ARITH_OPER(short operation,
                          Attributes * op1,
                          Attributes * op2,
                          char * op_data[])
{
  if (NOT ((operation == ITM_PLUS) || (operation == ITM_MINUS) ||
           (operation == ITM_TIMES) || (operation == ITM_DIVIDE)))
    return -1; // invalid operation

  short rc = 0;
  BigNum op1BN(BigNum::BIGNUM_TEMP_LEN, BigNum::BIGNUM_TEMP_PRECISION, 0, 0);
  BigNum op2BN(BigNum::BIGNUM_TEMP_LEN, BigNum::BIGNUM_TEMP_PRECISION, 0, 0);

  char op1BNdata[100];
  char op2BNdata[100];

  char * oper_data[3];

  //initialize result place holder.
  oper_data[0] = op_data[0];

  //convert op1 & op2 to bignum if not already bignum
  if(op1->isSimpleType())
  {
    char * op1_data[2];
    op1_data[0] = op1BNdata;
    op1_data[1] = op_data[1];
    rc = op1BN.castFrom(op1, op1_data, NULL, NULL);
    if(rc)
      return -1;
    oper_data[1] = op1BNdata;
  }
  else
    oper_data[1] = op_data[1]; 

  if(op2->isSimpleType())
  {
    char * op2_data[2];
    op2_data[0] = op2BNdata;
    op2_data[1] = op_data[2];
    rc = op2BN.castFrom(op2, op2_data, NULL, NULL);
    if(rc)
      return -1;
    oper_data[2] = op2BNdata;
  }
  else
    oper_data[2] = op_data[2]; 

  BigNum operBN(BigNum::BIGNUM_TEMP_LEN, BigNum::BIGNUM_TEMP_PRECISION, 0, 0);

  switch (operation)
    {
    case ITM_PLUS:
      {
        rc = operBN.add(&op1BN, &op2BN, oper_data);
      }
      break;

    case ITM_MINUS:
      {
        rc = operBN.sub(&op1BN, &op2BN, oper_data);
      }
      break;

    case ITM_TIMES:
      {
        rc = operBN.mul(&op1BN, &op2BN, oper_data);
      }
      break;

    case ITM_DIVIDE:
      {
        char tempSpace[200];
        operBN.setTempSpaceInfo(ITM_DIVIDE, (ULong)tempSpace, 200);

        rc = operBN.div(&op1BN, &op2BN, oper_data, NULL, NULL);
      }
      break;

    default:
      return -1;
    }

  if (rc)
    return -1;

  return 0;
}

short EXP_FIXED_BIGN_OV_ADD(Attributes * op1,
                            Attributes * op2,
                            char * op_data[])
{
  return EXP_BIGN_ARITH_OPER(ITM_PLUS, op1, op2, op_data);
}

short EXP_FIXED_BIGN_OV_SUB(Attributes * op1,
                            Attributes * op2,
                            char * op_data[])
{
  return EXP_BIGN_ARITH_OPER(ITM_MINUS, op1, op2, op_data);
}

short EXP_FIXED_BIGN_OV_MUL(Attributes * op1,
                            Attributes * op2,
                            char * op_data[])
{
  return EXP_BIGN_ARITH_OPER(ITM_TIMES, op1, op2, op_data);
}

short EXP_FIXED_BIGN_OV_DIV(Attributes * op1,
                            Attributes * op2,
                            char * op_data[])
{
  return EXP_BIGN_ARITH_OPER(ITM_DIVIDE, op1, op2, op_data);
}

Int64 EXP_FIXED_BIGN_OV_MOD(Attributes * op1,
                            Attributes * op2,
                            char * op_data[],
                            short * ov,
                            Int64 * quotient)
{
  short rc = 0;
  *ov = 0;

  BigNum op1BN(BigNum::BIGNUM_TEMP_LEN, BigNum::BIGNUM_TEMP_PRECISION, 0, 0);
  BigNum op2BN(BigNum::BIGNUM_TEMP_LEN, BigNum::BIGNUM_TEMP_PRECISION, 0, 0);

  char op1BNdata[100];
  char op2BNdata[100];

  char * mod_data[3];
  
  //convert op1 & op2 to bignum if not already bignum
  if(op1->isSimpleType())
  {
    char * op1_data[2];
    op1_data[0] = op1BNdata;
    op1_data[1] = op_data[0];
    rc = op1BN.castFrom(op1, op1_data, NULL, NULL);
    if(rc)
    {
      *ov = 1;
      return -1;
    }
    mod_data[1] = op1BNdata;
  }
  else
    mod_data[1] = op_data[0]; 

  if(op2->isSimpleType())
  {
    char * op2_data[2];
    op2_data[0] = op2BNdata;
    op2_data[1] = op_data[1];
    rc = op2BN.castFrom(op2, op2_data, NULL, NULL);
    if(rc)
    {
      *ov = 1;
      return -1;
    }
    mod_data[2] = op2BNdata;
  }
  else
    mod_data[2] = op_data[1]; 

  //Now begin MOD processing. Calculated
  //using basic operators:
  //z=MOD(x,y) then z = x - ((x/y)*(y))

  BigNum modBN(BigNum::BIGNUM_TEMP_LEN, BigNum::BIGNUM_TEMP_PRECISION, 0, 0);
  char * temp_data[3];
  
  //calculate (x/y)
  char xByY[100];
  temp_data[0] = xByY;
  temp_data[1] = mod_data[1];
  temp_data[2] = mod_data[2];
  char tempSpace[200];
  modBN.setTempSpaceInfo(ITM_DIVIDE, (ULong)tempSpace, 200);
  rc = modBN.div(&op1BN, &op2BN, temp_data, NULL, NULL);
  if(rc)
  {
    *ov = 1;
    return -1;
  }

  SimpleType resultST(REC_BIN64_SIGNED, sizeof(Int64), 0, 0,
                      ExpTupleDesc::SQLMX_FORMAT,
                      8, 0, 0, 0, Attributes::NO_DEFAULT, 0);
  
  // if quotient(x/y) is to be returned, return it
  if (quotient)
    {
      temp_data[0] = (char*)quotient; 
      temp_data[1] = xByY; //mod_data[0];
      
      rc = convDoIt(temp_data[1], BigNum::BIGNUM_TEMP_LEN, REC_NUM_BIG_SIGNED, BigNum::BIGNUM_TEMP_PRECISION, 0,
                    temp_data[0],  8, REC_BIN64_SIGNED, 0, 0,
                    NULL, 0, NULL, NULL);
      if (rc)
        {
          *ov = 1;
          return -1;
        }
    }

  //calculate (x/y) * y
  char xByYTimesY[100];
  temp_data[0] = xByYTimesY;
  temp_data[1] = xByY;
  //temp_data[2] already contains y.
  rc = modBN.mul(&op1BN, &op2BN, temp_data);
  if(rc)
  {
    *ov = 1;
    return -1;
  }
  
  //Calculate final result z = x - xByYTimesY
  //initialize result place holder.
  char resultBNdata[100];
  mod_data[0] = resultBNdata;
  //mod_data[1] already contains x.
  mod_data[2] = xByYTimesY;
  rc = modBN.sub(&op1BN, &op2BN, mod_data);
  if(rc)
  {
    *ov = 1;
    return -1;
  }

  Int64 result;
  temp_data[0] = (char*)&result; 
  temp_data[1] = mod_data[0];

  //rc = divBN.castTo(&resultST, op1_data);
  rc = convDoIt(temp_data[1], BigNum::BIGNUM_TEMP_LEN, REC_NUM_BIG_SIGNED, BigNum::BIGNUM_TEMP_PRECISION, 0,
		temp_data[0],  8, REC_BIN64_SIGNED, 0, 0,
		NULL, 0, NULL, NULL);
  if (rc)
    {
      *ov = 1;
      return -1;
    }

  return result;
}

static void CopyAttributes(char *dst, char *dstNull, char *dstVC, 
			   Attributes *dstAttr,
			   char *src, char *srcNotNull,char *srcVC,
			   Attributes *srcAttr)
{
  if(dstAttr->getNullFlag() && !srcNotNull)
    ExpTupleDesc::setNullValue(dstNull,
                               dstAttr->getNullBitIndex(),
                               dstAttr->getTupleFormat());
  else if(dstAttr->getNullFlag())
    ExpTupleDesc::clearNullValue(dstNull,
                                 dstAttr->getNullBitIndex(),
                                 dstAttr->getTupleFormat());

  if(srcNotNull)
    str_cpy_all(dst, src, dstAttr->getLength());
}

//  Implements the following "truth" table for the aggregate and
//  running sequence function sum.
//  A  B  result
//  x  y  call eval to compute A=x+y
//  x  N  set y to nonnull and zero and call eval to compute A=x+0
//  N  y  set x to nonnull and zero and call eval to compute A=0+y
//  N  N  set result to null.
//
ex_expr::exp_return_type 
ex_arith_sum_clause::processNulls(char *null_data[],
				  CollHeap *heap,
				  ComDiagsArea **diagsArea)
{
  // If both x and y are NULL, the result is NULL. Make it so and do not
  // call eval.
  //
  if(!null_data[1] && !null_data[2])
    {
      ExpTupleDesc::setNullValue(null_data[0],
                                 getOperand(0)->getNullBitIndex(),
                                 getOperand(0)->getTupleFormat() );
      return ex_expr::EXPR_NULL;
    }

  // If only x is NULL, then the result is y.
  //
  if(!null_data[1])
  {
    CopyAttributes(null_data[2*MAX_OPERANDS+0], null_data[0], 
		   null_data[MAX_OPERANDS+0], getOperand(0),
		   null_data[2*MAX_OPERANDS+2], null_data[2], 
		   null_data[MAX_OPERANDS+2], getOperand(2));
    return ex_expr::EXPR_NULL;
  }

  // If only y is NULL, then the result is x.
  //
  if(!null_data[2])
  {
    CopyAttributes(null_data[2*MAX_OPERANDS+0], null_data[0], 
		   null_data[MAX_OPERANDS+0], getOperand(0),
		   null_data[2*MAX_OPERANDS+1], null_data[1], 
		   null_data[MAX_OPERANDS+1], getOperand(1));
    return ex_expr::EXPR_NULL;
  }

  // Otherwise, set the result to not null and call eval to compute x+y.
  //
  ExpTupleDesc::clearNullValue(null_data[0],
                               getOperand(0)->getNullBitIndex(),
                               getOperand(0)->getTupleFormat() );
  return ex_expr::EXPR_OK;
}

//  Implements the following "truth" table for the count aggregate (A=B+A).
//  A  B  result
//  x  y  call eval to compute A=1+A
//  x  N  do nothing (A=x)
//  N  y  set a to nonnull and zero and call eval to compute A=1+0
//  N  N  do nothing (A=N)
//
ex_expr::exp_return_type 
ex_arith_count_clause::processNulls(char *null_data[],
				    CollHeap *heap,
				    ComDiagsArea **diagsArea)
{
  // If the first arg (B of A=B+A) is NULL, then do nothing
  //
  if(!null_data[1])
  {
    return ex_expr::EXPR_NULL;
  }

  // If the second arg (A of A=B+A) is NULL, zero it and call eval
  //
  if(!null_data[2])
  {
    ExpTupleDesc::clearNullValue(null_data[0],
                                 getOperand(0)->getNullBitIndex(),
                                 getOperand(0)->getTupleFormat() );
    str_pad(null_data[2*MAX_OPERANDS],getOperand(0)->getLength(), '\0');
    return ex_expr::EXPR_OK;
  }

  return ex_expr::EXPR_OK;
}
ex_expr::exp_return_type ex_arith_clause::eval(char *op_data[],
                                               CollHeap *heap,
                                               ComDiagsArea** diagsArea)
{

  switch (getInstruction())
    {
      /* ADD operation */
    case ADD_BIN16S_BIN16S_BIN16S:
      *(short *)op_data[0] = *(short *)op_data[1] + *(short *)op_data[2];
      break;
    case ADD_BIN16S_BIN16S_BIN32S:
      *(Lng32 *)op_data[0] = *(short *)op_data[1] + *(short *)op_data[2];
      break;
    case ADD_BIN16S_BIN32S_BIN32S:
      *(Lng32 *)op_data[0] = *(short *)op_data[1] + *(Lng32 *)op_data[2];
      break;
    case ADD_BIN32S_BIN16S_BIN32S:
      *(Lng32 *)op_data[0] = *(Lng32 *)op_data[1] + *(short *)op_data[2];
      break;
    case ADD_BIN32S_BIN32S_BIN32S:
      *(Lng32 *)op_data[0] = *(Lng32 *)op_data[1] + *(Lng32 *)op_data[2];
      break;
    case ADD_BIN32S_BIN64S_BIN64S:
      *(Int64 *)op_data[0] = *(Int64 *)op_data[2] + *(Lng32 *)op_data[1];
      break;
    case ADD_BIN64S_BIN32S_BIN64S:
      *(Int64 *)op_data[0] = *(Int64 *)op_data[1] + *(Lng32 *)op_data[2];
      break;
    
    case ADD_BIN64S_BIN64S_BIN64S:
      {
	short ov;
	*(Int64 *)op_data[0] = EXP_FIXED_OV_ADD(*(Int64 *)op_data[1],
					     *(Int64 *)op_data[2],
					     &ov);
	if (ov)
	  {
	    ExRaiseSqlError(heap, diagsArea, EXE_NUMERIC_OVERFLOW);
	    return ex_expr::EXPR_ERROR;
	  }

      }
      break;
     
    case ADD_BIN16U_BIN16U_BIN16U:
      *(unsigned short *)op_data[0] = *(unsigned short *)op_data[1] + *(unsigned short *)op_data[2];
      break;
    case ADD_BIN16U_BIN16U_BIN32U:
      *(ULng32 *)op_data[0] = *(unsigned short *)op_data[1] + *(unsigned short *)op_data[2];
      break;
    case ADD_BIN16U_BIN32U_BIN32U:
      *(ULng32 *)op_data[0] = *(unsigned short *)op_data[1] + *(ULng32 *)op_data[2];
      break;
    case ADD_BIN32U_BIN16U_BIN32U:
      *(ULng32 *)op_data[0] = *(ULng32 *)op_data[1] + *(unsigned short *)op_data[2];
      break;
    case ADD_BIN32U_BIN32U_BIN32U:
      *(ULng32 *)op_data[0] = *(ULng32 *)op_data[1] + *(ULng32 *)op_data[2];
      break;
    case ADD_BPINTU_BIN64S_BIN64S:
      *(Int64 *)op_data[0] = *(Int64 *)op_data[2] + *(unsigned short *)op_data[1];
      break;
    case ADD_BIN64S_BPINTU_BIN64S:
      *(Int64 *)op_data[0] = *(Int64 *)op_data[1] + *(unsigned short *)op_data[2];
      break;
    case ADD_BIN32U_BIN64S_BIN64S:
      *(Int64 *)op_data[0] = *(Int64 *)op_data[2] + *(ULng32 *)op_data[1];
      break;
    case ADD_BIN64S_BIN32U_BIN64S:
      *(Int64 *)op_data[0] = *(Int64 *)op_data[1] + *(ULng32 *)op_data[2];
      break;

    case ADD_FLOAT32_FLOAT32_FLOAT32:
      *(float *)op_data[0] = *(float *)op_data[1] + *(float *)op_data[2];
      break;

    case ADD_FLOAT64_FLOAT64_FLOAT64:
      {
	short ov;
	*(double *)op_data[0] = MathReal64Add(*(double *)op_data[1],
					      *(double *)op_data[2],
					      &ov);
	if (ov)
	  {
	    ExRaiseSqlError(heap, diagsArea, EXE_NUMERIC_OVERFLOW);
	    return ex_expr::EXPR_ERROR;
	  }
      }
      break;

    case ADD_DATETIME_INTERVAL_DATETIME:
      if (((ExpDatetime *) getOperand(0))->
          arithDatetimeInterval(ExpDatetime::DATETIME_ADD,
                                (ExpDatetime *)getOperand(1),
                                getOperand(2),
                                op_data[1],
                                op_data[2],
                                op_data[0],
                                heap,
                                diagsArea) != 0)
        return ex_expr::EXPR_ERROR;
      break;
    case ADD_INTERVAL_DATETIME_DATETIME:
      if (((ExpDatetime *) getOperand(0))->
          arithDatetimeInterval(ExpDatetime::DATETIME_ADD,
                                (ExpDatetime *)getOperand(2),
                                getOperand(1),
                                op_data[2],
                                op_data[1],
                                op_data[0],
                                heap,
                                diagsArea) != 0)
        return ex_expr::EXPR_ERROR;
      break;
      
      
      /* SUB operation */
    case SUB_BIN16S_BIN16S_BIN16S:
      *(short *)op_data[0] = *(short *)op_data[1] - *(short *)op_data[2];
      break;
    case SUB_BIN16S_BIN16S_BIN32S:
      *(Lng32 *)op_data[0] = *(short *)op_data[1] - *(short *)op_data[2];
      break;
    case SUB_BIN16S_BIN32S_BIN32S:
      *(Lng32 *)op_data[0] = *(short *)op_data[1] - *(Lng32 *)op_data[2];
      break;
    case SUB_BIN32S_BIN16S_BIN32S:
      *(Lng32 *)op_data[0] = *(Lng32 *)op_data[1] - *(short *)op_data[2];
      break;
    case SUB_BIN32S_BIN32S_BIN32S:
      *(Lng32 *)op_data[0] = *(Lng32 *)op_data[1] - *(Lng32 *)op_data[2];
      break;
    case SUB_BIN64S_BIN64S_BIN64S:
      {
	short ov;
	*(Int64 *)op_data[0] = EXP_FIXED_OV_SUB(*(Int64 *)op_data[1],
					     *(Int64 *)op_data[2],
					     &ov);
	if (ov)
	  {
	    ExRaiseSqlError(heap, diagsArea, EXE_NUMERIC_OVERFLOW);
	    return ex_expr::EXPR_ERROR;
	  }

	//*(Int64 *)op_data[0] = *(Int64 *)op_data[1] - *(Int64 *)op_data[2];
      }
      break;

    case SUB_BIN16U_BIN16U_BIN16U:
      *(unsigned short *)op_data[0] = *(unsigned short *)op_data[1] - *(unsigned short *)op_data[2];
      break;
    case SUB_BIN16U_BIN16U_BIN32U:
      *(ULng32 *)op_data[0] = *(unsigned short *)op_data[1] - *(unsigned short *)op_data[2];
      break;
    case SUB_BIN16U_BIN32U_BIN32U:
      *(ULng32 *)op_data[0] = *(unsigned short *)op_data[1] - *(ULng32 *)op_data[2];
      break;
    case SUB_BIN32U_BIN16U_BIN32U:
      *(ULng32 *)op_data[0] = *(ULng32 *)op_data[1] - *(unsigned short *)op_data[2];
      break;
    case SUB_BIN32U_BIN32U_BIN32U:
      *(ULng32 *)op_data[0] = *(ULng32 *)op_data[1] - *(ULng32 *)op_data[2];
      break;

    case SUB_FLOAT32_FLOAT32_FLOAT32:
      *(float *)op_data[0] = *(float *)op_data[1] - *(float *)op_data[2];
      break;
    case SUB_FLOAT64_FLOAT64_FLOAT64:
      {
	short ov;
	*(double *)op_data[0] = MathReal64Sub(*(double *)op_data[1],
					      *(double *)op_data[2],
					      &ov);
	if (ov)
	  {
	    ExRaiseSqlError(heap, diagsArea, EXE_NUMERIC_OVERFLOW);
	    return ex_expr::EXPR_ERROR;
	  }
      }
      break;

    case SUB_DATETIME_INTERVAL_DATETIME:
      if (((ExpDatetime *) getOperand(0))->
          arithDatetimeInterval(ExpDatetime::DATETIME_SUB,
                                (ExpDatetime *)getOperand(1),
                                getOperand(2),
                                op_data[1],
                                op_data[2],
                                op_data[0],
                                heap,
                                diagsArea) != 0)
        return ex_expr::EXPR_ERROR;
      break;
    case SUB_DATETIME_DATETIME_INTERVAL:
      if (((ExpDatetime *) getOperand(1))->subDatetimeDatetime(getOperand(1),
                                                                getOperand(0),
                                                                op_data[1],
                                                                op_data[2],
                                                                op_data[0],
                                                                heap,
                                                                diagsArea) != 0)
        return ex_expr::EXPR_ERROR;
      break;
        

      /* MUL operation */
    case MUL_BIN16S_BIN16S_BIN16S:
      *(short *)op_data[0] = *(short *)op_data[1] * *(short *)op_data[2];
      break;
    case MUL_BIN16S_BIN16S_BIN32S:
      *(Lng32 *)op_data[0] = *(short *)op_data[1] * *(short *)op_data[2];
      break;
    case MUL_BIN16S_BIN32S_BIN32S:
      *(Lng32 *)op_data[0] = *(short *)op_data[1] * *(Lng32 *)op_data[2];
      break;
    case MUL_BIN32S_BIN16S_BIN32S:
      *(Lng32 *)op_data[0] = *(Lng32 *)op_data[1] * *(short *)op_data[2];
      break;
    case MUL_BIN32S_BIN32S_BIN32S:
      *(Lng32 *)op_data[0] = *(Lng32 *)op_data[1] * *(Lng32 *)op_data[2];
      break;
    case MUL_BIN16S_BIN32S_BIN64S:
      *(Int64 *)op_data[0] = (Int64)*(short *)op_data[1] * (Int64)*(Lng32 *)op_data[2];
      break;
    case MUL_BIN32S_BIN16S_BIN64S:
      *(Int64 *)op_data[0] = (Int64)*(Lng32 *)op_data[1] * (Int64)*(short *)op_data[2];
      break;
    case MUL_BIN32S_BIN32S_BIN64S:
      *(Int64 *)op_data[0] = (Int64)*(Lng32 *)op_data[1] * (Int64)*(Lng32 *)op_data[2];
      break;
    case MUL_BIN64S_BIN64S_BIN64S:
      {
	short ov;
	*(Int64 *)op_data[0] = EXP_FIXED_OV_MUL(*(Int64 *)op_data[1],
					     *(Int64 *)op_data[2],
					     &ov);
	if (ov)
	  {
	    ExRaiseSqlError(heap, diagsArea, EXE_NUMERIC_OVERFLOW);
	    return ex_expr::EXPR_ERROR;
	  }
	//*(Int64 *)op_data[0] = *(Int64 *)op_data[1] * *(Int64 *)op_data[2];
      }
      break;
    
    case MUL_BIN16U_BIN16U_BIN16U:
      *(unsigned short *)op_data[0] = *(unsigned short *)op_data[1] * *(unsigned short *)op_data[2];
      break;
    case MUL_BIN16U_BIN16U_BIN32U:
      *(ULng32 *)op_data[0] = *(unsigned short *)op_data[1] * *(unsigned short *)op_data[2];
      break;
    case MUL_BIN16U_BIN32U_BIN32U:
      *(ULng32 *)op_data[0] = *(unsigned short *)op_data[1] * *(ULng32 *)op_data[2];
      break;
    case MUL_BIN32U_BIN16U_BIN32U:
      *(ULng32 *)op_data[0] = *(ULng32 *)op_data[1] * *(unsigned short *)op_data[2];
      break;
    case MUL_BIN32U_BIN32U_BIN32U:
      *(ULng32 *)op_data[0] = *(ULng32 *)op_data[1] * *(ULng32 *)op_data[2];
      break;

    case MUL_FLOAT32_FLOAT32_FLOAT32:
      *(float *)op_data[0] = *(float *)op_data[1] * *(float *)op_data[2];
      break;

    case MUL_FLOAT64_FLOAT64_FLOAT64:
      {
	short ov;
	*(double *)op_data[0] = MathReal64Mul(*(double *)op_data[1],
					      *(double *)op_data[2],
					      &ov);
	if (ov)
	  {
	    ExRaiseSqlError(heap, diagsArea, EXE_NUMERIC_OVERFLOW);
	    return ex_expr::EXPR_ERROR;
	  }
      }
      break;

      /* DIV operation */
    case DIV_BIN16S_BIN16S_BIN16S:
      if (*(short *)op_data[2] == 0) {
        ExRaiseSqlError(heap, diagsArea, EXE_DIVISION_BY_ZERO);
        return ex_expr::EXPR_ERROR;
      }
      *(short *)op_data[0] = *(short *)op_data[1] / *(short *)op_data[2];
      break;
    case DIV_BIN16S_BIN16S_BIN32S:
      if (*(short *)op_data[2] == 0) {
        ExRaiseSqlError(heap, diagsArea, EXE_DIVISION_BY_ZERO);
        return ex_expr::EXPR_ERROR;
      }
      *(Lng32 *)op_data[0] = *(short *)op_data[1] / *(short *)op_data[2];
      break;
    case DIV_BIN16S_BIN32S_BIN32S:
      if (*(Lng32 *)op_data[2] == 0) {
        ExRaiseSqlError(heap, diagsArea, EXE_DIVISION_BY_ZERO);
        return ex_expr::EXPR_ERROR;
      }
      *(Lng32 *)op_data[0] = *(short *)op_data[1] / *(Lng32 *)op_data[2];
      break;
    case DIV_BIN32S_BIN16S_BIN32S:
      if (*(short *)op_data[2] == 0) {
        ExRaiseSqlError(heap, diagsArea, EXE_DIVISION_BY_ZERO);
        return ex_expr::EXPR_ERROR;
      }
      *(Lng32 *)op_data[0] = *(Lng32 *)op_data[1] / *(short *)op_data[2];
      break;
    case DIV_BIN32S_BIN32S_BIN32S:
      if (*(Lng32 *)op_data[2] == 0) {
        ExRaiseSqlError(heap, diagsArea, EXE_DIVISION_BY_ZERO);
        return ex_expr::EXPR_ERROR;
      }
      *(Lng32 *)op_data[0] = *(Lng32 *)op_data[1] / *(Lng32 *)op_data[2];
      break;
    case DIV_BIN64S_BIN64S_BIN64S:
      {
	if (*(Int64 *)op_data[2] == 0) {
	  ExRaiseSqlError(heap, diagsArea, EXE_DIVISION_BY_ZERO);
	  return ex_expr::EXPR_ERROR;
	}

	short ov;
	*(Int64 *)op_data[0] = EXP_FIXED_OV_DIV(*(Int64 *)op_data[1],
					     *(Int64 *)op_data[2],
					     &ov);
	if (ov)
	  {
	    ExRaiseSqlError(heap, diagsArea, EXE_NUMERIC_OVERFLOW);
	    return ex_expr::EXPR_ERROR;
	  }

	//	*(Int64 *)op_data[0] = *(Int64 *)op_data[1] / *(Int64 *)op_data[2];
      }
      break;

    case DIV_BIN64S_BIN64S_BIN64S_ROUND:
      {
	if (*(Int64 *)op_data[2] == 0) {
	  ExRaiseSqlError(heap, diagsArea, EXE_DIVISION_BY_ZERO);
	  return ex_expr::EXPR_ERROR;
	}

	short ov;

	// upscale numerator by 1 
	NABoolean upscaled;
	Int64 temp;
	if (getDivToDownscale())
	  {
	    temp = *(Int64 *)op_data[2] / 10;
	    temp = *(Int64 *)op_data[1] / temp;
	    upscaled = TRUE;
	  }
	else
	  {
	    temp = EXP_FIXED_OV_MUL(*(Int64 *)op_data[1], 10, &ov);
	    if (ov)
	      {
		// couldn't upscale. Use the original value and don't do
		// any rounding.
		upscaled = FALSE;
		temp = *(Int64 *)op_data[1];
	      }
	    else
	      {
		upscaled = TRUE;
	      }
	    
	    temp = EXP_FIXED_OV_DIV(temp, *(Int64 *)op_data[2], &ov);
	    if (ov)
	      {
		ExRaiseSqlError(heap, diagsArea, EXE_NUMERIC_OVERFLOW);
		return ex_expr::EXPR_ERROR;
	      }
	  }

	if (upscaled)
	  {
	    short nsign = 0; // indicates negative sign when set to 1.
	    
	    // get the last digit
	    Lng32 v = (Lng32) (temp % (Int64)10);
            
            if( v < 0 ) v = -v;
            if(temp < 0) nsign = 1;
            
	    // downscale the result
	    temp = temp / (Int64)10;

	    if (arithRoundingMode_ == 1)
	      {
		// ROUND HALF UP MODE
		if (v >= 5)
		  {
		    // round up the result by 1
		    temp = nsign ? temp-1 : temp+1;
		  }
	      }
	    else if (arithRoundingMode_ == 2)
	      {
		// ROUND HALF EVEN MODE
		if (v > 5)
		  {
		    // round up the result by 1
		    temp = nsign ? temp-1 : temp+1;
		  }
		else if (v == 5)
		  {
		    // Roundup 'w' only if all the trailing digits following
		    // 'v' is zero and 'w' is even. If 'w' is odd, irrespective
		    // of trailing digits following 'v', 'w' is rounded up.
		    // w is second last digit.
		    Lng32 w = (Lng32) (temp % (Int64)10);
		    if ((w & 0x1) != 0)
		    {
		      // odd number, round up.
		      temp = nsign ? temp-1 : temp+1;
		    }
		    else
		    {
		      // Since 'w' is an even digit, we need to determine if
		      // all the trailing digits following 'v' is zero.
		      // If any digit following 'v' is nonzero, then it is
		      // similar to v > 5.
		      NABoolean vGT5 = FALSE;
		      if(! getDivToDownscale())
		      {
		        //Figure out if digits following 'v' is non zero.
		        Int64 multiplier = 100;//For digit following 'v'.
		        Int64 temp1;
		        NABoolean biggerPrecision = FALSE;
		        while(!vGT5)
		        {
		          temp1 = EXP_FIXED_OV_MUL(*(Int64 *)op_data[1], multiplier, &ov);
	                  if (ov)
	                  {
	                    //end of digits.
	                    //When we reach here, temp1 is overflowed over Int64. 
                            //In this rear but possible situation, we should try checking
                           //for additional digits using BigNum datatype.
                            biggerPrecision = TRUE;
	                    break;
	                  }
	                  temp1 = EXP_FIXED_OV_DIV(temp1, *(Int64 *)op_data[2], &ov);
	                  if (ov)
	                  {
		            //Something went wrong, lets consider
	                    //it as end of digits.
	                    break;
	                  }
	                  if(temp1 % (Int64)10)
	                  {
	                    vGT5 = TRUE;
	                    break;
	                  }
	                  multiplier = EXP_FIXED_OV_MUL(multiplier, 10, &ov);
	                  if (ov)
	                  {
	                    //end of digits.
	                    break;
	                  }
		        }
		        if(biggerPrecision)
                       { 
                          short rc = 0;
                          Int64 dividend = *(Int64 *)op_data[1];
                          Int64 divisor  = *(Int64 *)op_data[2];
                          char *op_data[3];
                          char result1[100];
                          char result2[100];
                          Int64 result3 = 0;
                          Int64 ten = 10;
                          SimpleType opST(REC_BIN64_SIGNED, sizeof(Int64), 0, 0,
		                         ExpTupleDesc::SQLMX_FORMAT,
		                         8, 0, 0, 0, Attributes::NO_DEFAULT, 0);
                          
                          BigNum opBN(BigNum::BIGNUM_TEMP_LEN, BigNum::BIGNUM_TEMP_PRECISION, 0, 0);                          
                          
                          while(!vGT5)
		           {
                            op_data[0] = result1;
                            op_data[1] = (char *) &dividend;
                            op_data[2] = (char *) &multiplier; 
                            rc = EXP_FIXED_BIGN_OV_MUL(&opST,
                                                       &opST,
                                                       op_data);
                           if (rc)
	                    {
	                      //end of digits.
	                      break;
	                    }
                           
                           op_data[0] = result2;
                           op_data[1] = result1;
                           op_data[2] = (char *) &divisor;
                           rc = EXP_FIXED_BIGN_OV_DIV(&opBN,
                                                      &opST,
                                                      op_data);
	                    if (rc)
	                    {
	                      //Something went wrong, lets consider
	                      //it as end of digits.
		              break;
	                    }
                           
                           op_data[0] = result2;
                           op_data[1] = (char *) &ten;
                           result3 = EXP_FIXED_BIGN_OV_MOD(&opBN,
                                                      &opST,
                                                      op_data,
                                                      &ov);
                           if (ov)
	                    {
	                      //end of digits.
	                      break;
	                    }
                            
	                    if(result3)
	                    {
	                      vGT5 = TRUE;
	                      break;
	                    }
		             
                           multiplier = EXP_FIXED_OV_MUL(multiplier, 10, &ov);
	                    if (ov)
	                    {
	                      //end of digits.
	                      break;
	                    }
		           }
                       }
		      }
		      else
		      {
		        Int64 divisor = 100;//devisor=10 corresponds to v digit.
		        Int64 temp1;
		        while(!vGT5)
		        {
		          temp1 = *(Int64 *)op_data[2] /divisor;
  		        
		          if(!temp1) //reached end of digits.
		          {
		            break;
		          }
  		        
		          temp1 = *(Int64 *)op_data[1]/ temp1;
		          if(temp1 % (Int64)10)
	                  {
	                    vGT5 = TRUE;
	                    break;
	                  }
	                  divisor = EXP_FIXED_OV_MUL(divisor, 10, &ov);
	                  if (ov)
	                  {
	                    //end of digits.
	                    break;
	                  }
		        }
		      }
  		    
		      if(vGT5)
		      {
		        //irrespective of w being an even number,
		        //round up the value;
		        temp = nsign ? temp-1 : temp+1;  
		      }
		    }
		  }
	      }
	    else
	      {
		ExRaiseSqlError(heap, diagsArea, EXE_NUMERIC_OVERFLOW);
		return ex_expr::EXPR_ERROR;
	      }
	  }

	*(Int64 *)op_data[0] = temp;

      }
      break;

    case DIV_BIN16U_BIN16U_BIN16U:
      if (*(unsigned short *)op_data[2] == 0) {
        ExRaiseSqlError(heap, diagsArea, EXE_DIVISION_BY_ZERO);
        return ex_expr::EXPR_ERROR;
      }
      *(unsigned short *)op_data[0] = *(unsigned short *)op_data[1] / *(unsigned short *)op_data[2];
      break;
    case DIV_BIN16U_BIN16U_BIN32U:
      if (*(unsigned short *)op_data[2] == 0) {
        ExRaiseSqlError(heap, diagsArea, EXE_DIVISION_BY_ZERO);
        return ex_expr::EXPR_ERROR;
      }
      *(ULng32 *)op_data[0] = *(unsigned short *)op_data[1] / *(unsigned short *)op_data[2];
      break;
    case DIV_BIN16U_BIN32U_BIN32U:
      if (*(ULng32 *)op_data[2] == 0) {
        ExRaiseSqlError(heap, diagsArea, EXE_DIVISION_BY_ZERO);
        return ex_expr::EXPR_ERROR;
      }
      *(ULng32 *)op_data[0] = *(unsigned short *)op_data[1] / *(ULng32 *)op_data[2];
      break;
    case DIV_BIN32U_BIN16U_BIN32U:
      if (*(unsigned short *)op_data[2] == 0) {
        ExRaiseSqlError(heap, diagsArea, EXE_DIVISION_BY_ZERO);
        return ex_expr::EXPR_ERROR;
      }
      *(ULng32 *)op_data[0] = *(ULng32 *)op_data[1] / *(unsigned short *)op_data[2];
      break;
    case DIV_BIN32U_BIN32U_BIN32U:
      if (*(ULng32 *)op_data[2] == 0) {
        ExRaiseSqlError(heap, diagsArea, EXE_DIVISION_BY_ZERO);
        return ex_expr::EXPR_ERROR;
      }
      *(ULng32 *)op_data[0] = *(ULng32 *)op_data[1] / *(ULng32 *)op_data[2];
      break;

    case DIV_FLOAT64_FLOAT64_FLOAT64:
      {
	if (*(double *)op_data[2] == 0) {
	  ExRaiseSqlError(heap, diagsArea, EXE_DIVISION_BY_ZERO);
	  return ex_expr::EXPR_ERROR;
	}

	short ov;
	*(double *)op_data[0] = MathReal64Div(*(double *)op_data[1],
					       *(double *)op_data[2],
					       &ov);
	if (ov)
	  {
	    ExRaiseSqlError(heap, diagsArea, EXE_NUMERIC_OVERFLOW);
	    return ex_expr::EXPR_ERROR;
	  }
      }
      break;

    case NEGATE_BOOLEAN:
      *(Int8 *)op_data[0] = (*(Int8*)op_data[1] == 1 ? 0 : 1);
      break;

      // COMPLEX datatype operations
    case ADD_COMPLEX:
      ((ComplexType *)getOperand(0))->add(getOperand(1), getOperand(2), op_data);
      break;

     case SUB_COMPLEX:
      ((ComplexType *)getOperand(0))->sub(getOperand(1), getOperand(2), op_data);
      break;
      
    case MUL_COMPLEX:
      ((ComplexType *)getOperand(0))->mul(getOperand(1), getOperand(2), op_data);
      break;

    case DIV_COMPLEX:
      if (((ComplexType *)getOperand(0))->div(getOperand(1), getOperand(2), op_data,
                                               heap, diagsArea))
        return ex_expr::EXPR_ERROR;
      break;
 
    case ARITH_NOT_SUPPORTED:
      {
	// this arith operation not supported.
	// See if it could still be evaluated by doing some intermediate
	// operations.
	if (evalUnsupportedOperations(op_data, heap, diagsArea) !=
	    ex_expr::EXPR_OK)
	  return ex_expr::EXPR_ERROR;
      }
    break;

    default:
      ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
      return ex_expr::EXPR_ERROR;

    }
  
  return ex_expr::EXPR_OK;
}
;

ex_expr::exp_return_type ex_arith_clause::evalUnsupportedOperations(
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

