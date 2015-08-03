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
 *
 *
 *****************************************************************************
 */

#include "Platform.h"


#include <stddef.h>
#include "exp_stdh.h"
#include "exp_clause_derived.h"

ex_expr::exp_return_type ex_unlogic_clause::eval(char *op_data[],
						 CollHeap *heap,
						 ComDiagsArea** diagsArea)
{
  ex_expr::exp_return_type retcode = ex_expr::EXPR_OK;
  Attributes *tgtAttr = getOperand(0);
  Attributes *srcAttr = getOperand(1);

  switch (getOperType())
    {
    case ITM_IS_NULL:
      {
	// null value for operand 1 is pointed to by
	// op_data[- (2 * MAX_OPERANDS) + 1].
	if (srcAttr->getNullFlag() &&              // nullable and
	    (! op_data[- (2 * MAX_OPERANDS) + 1])) // missing (null)
        {
          if (tgtAttr->isSQLMXAlignedFormat())
              ExpAlignedFormat::setNullBit(op_data[0],
                                           tgtAttr->getNullBitIndex());
          else
	    *(Lng32 *)op_data[0] = 1; // value is null
        }
	else
        {
          if (tgtAttr->isSQLMXAlignedFormat())
            ExpAlignedFormat::clearNullBit(op_data[0], 
                                           tgtAttr->getNullBitIndex());
          else
	    *(Lng32 *)op_data[0] = 0;
        }		
      }
      break;
      
    case ITM_IS_NOT_NULL:
      {
	if (getOperand(1)->getNullFlag() &&
	    (! op_data[- (2 * MAX_OPERANDS) + 1]))
        {
          if (tgtAttr->isSQLMXAlignedFormat())
            ExpAlignedFormat::clearNullBit(op_data[0],
                                           tgtAttr->getNullBitIndex());
          else
	    *(Lng32 *)op_data[0] = 0; 
        }
	else
        {
          if (tgtAttr->isSQLMXAlignedFormat())
            ExpAlignedFormat::setNullBit(op_data[0], 
                                         tgtAttr->getNullBitIndex());
          else
            *(Lng32 *)op_data[0] = 1; // value is not null
        }		
      }
      break;

    case ITM_IS_UNKNOWN:
      {
	if (*(Lng32 *)op_data[1] == -1)   // null
	  *(Lng32 *)op_data[0] = 1; // result is true
	else
	  *(Lng32 *)op_data[0] = 0; // result is false
      }
      break;

     case ITM_IS_NOT_UNKNOWN:
      {
	if  (*(Lng32 *)op_data[1] != -1)   // not null
	  *(Lng32 *)op_data[0] = 1; // result is true
	else
	  *(Lng32 *)op_data[0] = 0; // result is false
      }
      break;
      
    case ITM_IS_TRUE:
      {
	if (*(Lng32 *)op_data[1] == 1)   // true 
	  *(Lng32 *)op_data[0] = 1; // result is true
	else
	  *(Lng32 *)op_data[0] = 0; // result is false
      }
      break;
      
    case ITM_IS_FALSE:
      {
	if (*(Lng32 *)op_data[1] == 0)   // false 
	  *(Lng32 *)op_data[0] = 1; // result is true
	else
	  *(Lng32 *)op_data[0] = 0; // result is false
      }
      break;
      
    case ITM_NOT:
      {
	if (*(Lng32 *)op_data[1] == 1) // TRUE
	  *(Lng32 *)op_data[0] = 0;    // make it FALSE
	else
	  if (*(Lng32 *)op_data[1] == 0) // FALSE
	    *(Lng32 *)op_data[0] = 1;    // make it TRUE
	  else
	    *(Lng32 *)op_data[0] = -1;   // NULL
      }
      break;

    default:
      ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
      retcode = ex_expr::EXPR_ERROR;
      break;
    }
  return retcode;
}      


