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
******************************************************************************
*
* File:         ExBitMuxFunction.cpp
* RCS:          $Id
* Description:  ExBitMuxFunction class Implementation
* Created:      6/15/97
* Modified:     $Author
* Language:     C++
* Status:       $State
*
*
*
******************************************************************************
*/

#include "Platform.h"


// Includes
//
#include "exp_clause.h"
#include "exp_attrs.h"
#include "ExpPCode.h"
#include "ExpBitMuxFunction.h" // <--- See here for comments.

// ExpBitMuxFunction::ExpBitMuxFunction
//
// Constructs an ExpBitMuxFunction object
//
// IN     : oper_type - The type of clause, must be ITM_BITMUX
// IN     : arity     - The number of input arguments
// IN     : space     - Memory allocator.
// EFFECTS: Calls the base class constructor
// PRECOND: The oper_type is equal to ITM_BITMUX.
//        : The arity doesn't exceed MAX_OPERANDS.
//        : The attributes pointer is valid.
//        : The space object is valid.
// PSTCOND: ExpBitMuxFunction is constructed.
//
ExpBitMuxFunction::ExpBitMuxFunction(OperatorTypeEnum oper_type, Int32 arity,
				     Attributes ** attr, Space * space)
  : ex_function_clause(oper_type, arity, attr, space) {
    // Test preconditions. It may already be too late ...
    //
#if 0
    ex_assert(oper_type == ITM_BITMUX,
	      "ExpBitMuxFunction - bad OperatorTypeEnum");
    ex_assert((arity > 0) && (arity <= ex_clause::MAX_OPERANDS),
	      "ExpBitMuxFunction - bad arity");
    ex_assert(attr,
	      "ExpBitMuxFunction - NULL attr pointer");
    ex_assert(space,
	      "ExpBitMuxFunction - NULL space pointer");
#endif
};

// ExpBitMuxFunction::~ExpBitMuxFunction
//
// Destructs an ExpBitMuxFunction object
//
// EFFECTS: None
//
ExpBitMuxFunction::ExpBitMuxFunction() {;};

// ExpBitMuxFunction::pack
//
// Packs an ExpBitMuxFunction object
//
// IN     : space - The memory allocator.
// RETURN : the offset withing space of the packed object.
// EFFECTS: Pointers are changed to offsets within space.
// PRECOND: Space is a valid space object.
Long ExpBitMuxFunction::pack(void * space) {
  // Test preconditions.
  //
#if 0
  ex_assert(space, "ExpBitMuxFunction::pack - Invalid space object");
#endif

  // Since ExpBitMuxFunction has no private data, the pack_clause
  // helper of the ex_clause class can be used to pack this
  // class.
  //
  return packClause(space, sizeof(ExpBitMuxFunction));
}  

// ExpBitMuxFunction::pCodeGenerate
//
// Generate PCI's for the BitMux operation. For now, only handle certain
// cases which give signifigant speedup. Otherwise, use CLAUSE_EVAL.
//
// IN     : space - memory allocator
// RETURN : ex_expr::EXPR_OK is no errors
// EFFECTS: stores pointer to PCodeObject in clause
//
ex_expr::exp_return_type ExpBitMuxFunction::pCodeGenerate(Space *space, UInt32 f) {
  // What is the arity?
  //
  Int32 numOperands = getNumOperands();

  // Get a handle on the operands.
  //
  AttributesPtr *attrs = getOperand();

  // Only support operations to/from ATPs with INT's or CHAR datatype.
  //
  // Dst must be ATP
  //
  if(attrs[0]->getAtpIndex() < 2)
    return ex_clause::pCodeGenerate(space, f);

  Int32 i=1;
  for(; i<numOperands; i++) 
    {
      // Src must be ATP
      //
      if(attrs[i]->getAtpIndex() < 2)
	return ex_clause::pCodeGenerate(space, f);

      // No varchars
      //
      if(attrs[i]->getVCIndicatorLength() > 0) 
	return ex_clause::pCodeGenerate(space, f);

      // No nulls. Fix for genesis case 10-980114-7618 : AS 01/22/98
      //
      if(attrs[i]->getNullIndicatorLength() > 0) 
	return ex_clause::pCodeGenerate(space, f);

      // Only ints or chars.
      //
      switch(attrs[i]->getDatatype())
	{
	case REC_BPINT_UNSIGNED:
	case REC_BIN16_SIGNED: case REC_BIN16_UNSIGNED:
	case REC_BIN32_SIGNED: case REC_BIN32_UNSIGNED:
	case REC_BIN64_SIGNED:
	case REC_BYTE_F_ASCII:
	  break;

      default:
	return ex_clause::pCodeGenerate(space, f);
	break;
	}
    }

  // Allocate the code list.
  //
  PCIList code(space);

  // Generate pre clause PCI's.
  // 
  PCode::preClausePCI(this, code);

  // Load and store each input value.
  //
  Int32 offset = attrs[0]->getOffset();
  AML aml(PCIT::MBIN8, PCIT::MBIN8, PCIT::IBIN32S);
  for(i=1; i<numOperands; i++) {
    if(attrs[i]->getNullFlag())
      {
	OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), offset,
	      attrs[i]->getAtp(), attrs[i]->getAtpIndex(), 
	      attrs[i]->getNullIndOffset(), 2);

        PCI pci(PCIT::Op_MOVE, aml, ol); 
	code.append(pci);
	offset += 2;
      }

    OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), offset,
	  attrs[i]->getAtp(), attrs[i]->getAtpIndex(), attrs[i]->getOffset(),
	  attrs[i]->getLength());
    PCI pci(PCIT::Op_MOVE, aml, ol);
    code.append(pci);
    offset += attrs[i]->getLength();
  }

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
};

// ExpBitMuxFunction::eval
//
// Evaluate the BitMux function.
//
// IN     : op_data      - output/input vector
// IN     : heap         - not used
// IN     : comDiagsArea - not used
// RETURN : ex_expr::EXPR_OK is no errors
// EFFECTS: Writes byte equality comparable output to
//          result attribute based on input attributes.
// PRECOND: The op_data vector points to the NULL/VARCHAR and data.
// PSTCONT: The BitMuxing result is stored in the result attribute.
//
ex_expr::exp_return_type ExpBitMuxFunction::eval(char *op_data[],
						 CollHeap*,
						 ComDiagsArea**) {
  
  // Get the pointer to the start of the result data.
  //
  char *dstData = op_data[0];

  // Get a handle on the operands so they can be queried for
  // NULL and VARCHAR ness.
  //
  AttributesPtr *attrs = getOperand();

  // Loop over each input attribute and convert the attribute data into
  // BitMux form.
  //
  for(Int32 i=1; i<getNumOperands(); i++) {

    // Query the attribute to the size of the data. This size does not
    // include NULL or VARCHAR information.
    //
    Int32 length = attrs[i]->getLength();

    // Start by assumming that the attribute is not NULL and uses the
    // entire allocate space. Change below if necessary.
    //
    Int32 isNull = 0;
    Int32 dataLength = length;

    // Remember the pointer to the attribute data.
    //
    const char *srcData = op_data[i];

    // If the attribute is nullable, then add two bytes indicating
    // the nullness. If the attribute is NULL, also set the isNULL flag.
    //
    if(attrs[i]->getNullFlag()) {
      if(!op_data[-2 * MAX_OPERANDS + i]) {
	*(dstData++) = (char)0xFF;
	*(dstData++) = (char)0xFF;
	isNull = 1;
      } else {
	*(dstData++) = (char)0x00;
	*(dstData++) = (char)0x00;
      }
    }

    // If the attribute is a VARCHAR, then add two bytes indicating
    // the VARCHAR length and set dataLength to be the actual length
    // of the VARCHAR.
    //
    if(attrs[i]->getVCIndicatorLength() > 0) {
      if (isNull) // a null value, make datalength 0.
	dataLength = 0;
      else
	{
	  dataLength = *(short*)op_data[- ex_clause::MAX_OPERANDS + i];
	  
	  // skip trailing blanks, they do not count as far as ANSI SQL 
	  // comparison rules go.
	  while ((dataLength > 0) && (srcData[dataLength-1] == ' '))
	    dataLength--;
	}

      *((short*)dstData) = dataLength;
      dstData += 2;
    } // source is a VARCHAR

    // If the attribute is NULL, then NULL out the data space.
    //
    if(isNull) {
      for(Int32 j=0; j<length; j++)
	dstData[j] = 0;
    } 
    // Otherwise, copy the data to the data space and pad with
    // zeros if necessary.
    //
    else {
      Int32 j=0;
      for(; j<dataLength; j++)
	dstData[j] = srcData[j]; 
      while(j<length)
	dstData[j++] = 0;
    }

    // Adjust the result pointer ahead length bytes to compensate for the
    // data added in the last loop above.
    //
    dstData += length;
  }

  // Return all OK...
  //
  return ex_expr::EXPR_OK;
};
