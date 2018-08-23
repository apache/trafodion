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

ex_expr::exp_return_type ex_bool_clause::eval(char *op_data[],
					      CollHeap *heap,
					      ComDiagsArea** diagsArea)
{
  ex_expr::exp_return_type retcode = ex_expr::EXPR_OK;

  // boolean values: 0 = false, 1 = true, -1 = null
  switch (getOperType())
    {
    case ITM_AND:
      if ((*(Lng32 *)op_data[1] == -1) &&
	  (*(Lng32 *)op_data[2] != 0))
	*(Lng32 *)op_data[0] = -1;
      else
	if (*(Lng32 *)op_data[1] == 0)
	  *(Lng32 *)op_data[0] = 0;
	else
	  *(Lng32 *)op_data[0] = *(Lng32 *)op_data[2];
      break;

    case ITM_OR:
       if ((*(Lng32 *)op_data[1] == -1) &&
	  (*(Lng32 *)op_data[2] != 1))
	*(Lng32 *)op_data[0] = -1;
      else
	if (*(Lng32 *)op_data[1] == 1)
	  *(Lng32 *)op_data[0] = 1;
	else
	  *(Lng32 *)op_data[0] = *(Lng32 *)op_data[2];
     
      break;

    default:
      ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
      retcode = ex_expr::EXPR_ERROR;
      break;
    }
  return retcode;
  
}

///////////////////////////////////////////////////////////////
// class ex_branch_clause
///////////////////////////////////////////////////////////////
ex_expr::exp_return_type ex_branch_clause::eval(char *op_data[],
						CollHeap *heap,
						ComDiagsArea** diagsArea)
{
  ex_expr::exp_return_type retcode = ex_expr::EXPR_OK;

  // boolean values: 0 = false, 1 = true, -1 = null

  switch (getOperType())
    {
    case ITM_AND:
      if (*(Lng32 *)op_data[1] == 0 || *(Lng32 *)op_data[1] == -1)  // null treated as false
	{
	  *(Lng32 *)op_data[0] = 0;
	  setNextClause(branch_clause);
	}
      else
	{
	  *(Lng32 *)op_data[0] = *(Lng32 *)op_data[1];
	  setNextClause(saved_next_clause);
	}
      
      break;

    case ITM_OR:
      if (*(Lng32 *)op_data[1] == 1)
	{
	  *(Lng32 *)op_data[0] = 1;
	  setNextClause(branch_clause);
	}
      else
	{
	  *(Lng32 *)op_data[0] = *(Lng32 *)op_data[1];
	  setNextClause(saved_next_clause);
	}
      
      break;

    case ITM_RETURN_TRUE:
      setNextClause(branch_clause);
      break;

    default:
      ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
      retcode = ex_expr::EXPR_ERROR;
      break;
    }
  return retcode;
  
}

/////////////////////////////////////////////////////////////
// class bool_result_clause
/////////////////////////////////////////////////////////////
ex_expr::exp_return_type bool_result_clause::eval(char *op_data[],
						  CollHeap*,
						  ComDiagsArea**)
{
  // boolean values: 0 = false, 1 = true, -1 = null
  if ((*(Lng32 *)op_data[0] == 0) || (*(Lng32 *)op_data[0] == -1))
    return ex_expr::EXPR_FALSE;
  else
    return ex_expr::EXPR_TRUE;
}




