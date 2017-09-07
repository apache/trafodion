/* -*-C++-*-
******************************************************************************
*
* File:         ExBitMuxFunction.h
* RCS:          $Id
* Description:  ExBitMuxFunction class Declaration
* Created:      6/15/97
* Modified:     $Author
* Language:     C++
* Status:       $State
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
******************************************************************************
*/

#ifndef EXP_BITMUX_FUNCTION_H
#define EXP_BITMUX_FUNCTION_H
//
// Only include this file once

// Includes
//
#include "exp_clause.h"
#include "exp_clause_derived.h"

// Internal forward declarations
//
class ExpBitMuxFunction;

// ExpBitMuxFunction
//
// The ExBitMuxFunction class is a leaf of the derivation chain
// ex_function_clause->ex_clause->NABasicObject. The NABasicObject class provides a basic
// interface for memory management and packing of executor classes. The
// ex_clause class provides the basic unit of operation for the standard 
// executor expression evaluation engine. The ex_function_clause class is
// a specialization of the ex_clause class for "functions". Other examples
// of classes derived from ex_function_clause include the string functions
// trim, substring, etc. and the numeric functions variance and standard
// deviation.
// 
// The ExpBitMuxFunction class provides support for the "BitMuxing" function.
// The BitMuxing function is a variable arity function which computes a 
// unique byte stream for each unique set of inputs. The outputs of separate 
// applications of the BitMuxing function can be byte compared to determine
// the equality of sets of inputs. This is very similar to the encode
// function, except BitMuxing only works for equality comparisons and not
// less/greater than operations. This difference means that there are
// fewer restrictions for the BitMuxing operation and more oppurtunities
// for time and space optimizations. For example, the order of attributes
// and NULL/VARCHAR indicators in the BitMuxing output is not important.
// For encode, on the other hand all items relating to an attribute must
// be contiguous in the output.
//
// The BitMuxing function may be used in hash grouping and hash join
// operations in order to convert the input rows into byte comparable 
// form. This allows equality comparisions to be made without using the
// expression evaluation engine -- a large performance gain.
//

#pragma warning ( disable : 4251 )

// ExpBitMuxFunction declaration
//
class ExpBitMuxFunction : public ex_function_clause {
public:
  // Construction - this is the "real" constructor
  //
  ExpBitMuxFunction(OperatorTypeEnum oper_type,
			       Int32 arity,
			       Attributes ** attr,
			       Space * space);
  // This constructor is used only to get at the virtual function table.
  //
  ExpBitMuxFunction();


  // isNullInNullOut - Must redefine this virtual function to return 0
  // since a NULL input does not simply produce a NULL output.
  //
  Int32 isNullInNullOut() const { return 0; };

  // isNullRelevant - Must redefine this virtual function to return 0
  // since all the work is done in eval and none in processNulls.
  //
  Int32 isNullRelevant() const { return 0; };

  // eval - Must redefine eval to compute the BitMuxing function.
  //
  ex_expr::exp_return_type eval(char *op_data[], CollHeap*, 
					   ComDiagsArea** =0);

  // pack - Must redefine pack.
  Long pack(void *);

  // pCodeGenerate - Must redefine pCodeGenerate the produce specific
  // PCODE instructions for BitMuxing.
  //
  ex_expr::exp_return_type pCodeGenerate(Space *space,
						    UInt32 f);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(2,getClassVersionID());
    ex_function_clause::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------

  // There are no private members.
  //
private:

};

#pragma warning ( default : 4251 )

#endif

