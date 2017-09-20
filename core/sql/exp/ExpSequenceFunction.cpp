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
* File:         ExSequenceFunction.cpp
* RCS:          $Id
* Description:  ExSequenceFunction class Implementation
* Created:      9/11/98
* Modified:     $Author
* Language:     C++
* Status:       $State
*
*
*
*
******************************************************************************
*/

#include "Platform.h"


// Includes
//
#include "SQLTypeDefs.h"
#include "exp_clause.h"
#include "exp_attrs.h"
#include "ExpPCode.h"
#include "ExpSequenceFunction.h" // <--- See here for comments.

// ExpSequenceFunction::ExpSequenceFunction
//
// Constructs an ExpSequenceFunction object
//
// IN     : oper_type - The type of clause, must be ITM_OFFSET
// IN     : arity     - The number of input arguments
// IN     : space     - Memory allocator.
// EFFECTS: Calls the base class constructor
// PRECOND: The oper_type is equal to ITM_OFFSET.
//        : The arity doesn't exceed MAX_OPERANDS.
//        : The attributes pointer is valid.
//        : The space object is valid.
// PSTCOND: ExpSequenceFunction is constructed.
//
ExpSequenceFunction::ExpSequenceFunction
(OperatorTypeEnum oper_type, Int32 arity, Int32 index,
 Attributes ** attr, Space * space)
#pragma nowarn(1506)   // warning elimination 
  : ex_function_clause(oper_type, arity, attr, space), offsetIndex_(index), flags_(0) {
#pragma warn(1506)  // warning elimination 
};

// ExpSequenceFunction::ExpSequenceFunction
//
// Pseudo Constructor for getting VTable
//
// EFFECTS: None
//
ExpSequenceFunction::ExpSequenceFunction() {;};

// ExpSequenceFunction::pack
//
// Packs an ExpSequenceFunction object
//
// IN     : space - The memory allocator.
// RETURN : the offset withing space of the packed object.
// EFFECTS: Pointers are changed to offsets within space.
// PRECOND: Space is a valid space object.
Long ExpSequenceFunction::pack(void * space) {
  // Since ExpSequenceFunction has no private data, the pack_clause
  // helper of the ex_clause class can be used to pack this
  // class.
  //
  return packClause(space, sizeof(ExpSequenceFunction));
}  

ex_expr::exp_return_type ExpSequenceFunction::pCodeGenerate(Space *space, UInt32 f) 
{

  if(isAnyOperandNullable()) 
    return ex_clause::pCodeGenerate(space, f);

  if(getNumOperands() != 2 && getNumOperands() != 3)
    return ex_clause::pCodeGenerate(space, f);

  if(!nullRowIsZero())
    return ex_clause::pCodeGenerate(space, f);

  if(!isLeading() || (winSize() != 0))
    return ex_clause::pCodeGenerate(space, f);

  AttributesPtr *attrs = getOperand();
  Lng32 fsDataType = attrs[1]->getDatatype();
  Lng32 length = attrs[1]->getLength();
  
  if(fsDataType != REC_BIN64_SIGNED || length != 8)
    return ex_clause::pCodeGenerate(space, f);

  if(getNumOperands() == 3) {
    fsDataType = attrs[2]->getDatatype();
    length = attrs[2]->getLength();

    if(fsDataType != REC_BIN32_SIGNED || length != 4)
      return ex_clause::pCodeGenerate(space, f);
  }

  // If we get to this point, we have decided to generate PCode for this
  // particular sequence function operation.
  //
  PCIList code(space);

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  if(getNumOperands() == 2) {
    Int32 index = offsetIndex_;
  
       
    // The 1st operand is a pointer to the function.
    // The 2nd operand is a pointer the Tcb to get to the history buffer.
    // The 3rd operand is the immediate index value.
    // The 4th operand is the memory location for the target.
    // The 5th operand is the memory location for the source.
    //
    AML aml(PCIT::IPTR, 
            PCIT::IPTR, 
            PCIT::IBIN32S,
            PCIT::getMemoryAddressingMode(attrs[0]->getDatatype()),
            PCIT::getMemoryAddressingMode(attrs[1]->getDatatype()));

    OL ol((Int64)isOLAP(), (Int64)0, index, 
          attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(),
          attrs[1]->getAtp(), attrs[1]->getAtpIndex(), attrs[1]->getOffset());

    // Add the OFFSET instruction.
    //
    PCI pci(PCIT::Op_OFFSET, aml, ol);
    code.append(pci);
  } else {
    // The 1st operand is a pointer to the function.
    // The 2nd operand is a pointer the Tcb to get to the history buffer.
    // The 3rd operand is the memory location for the index value.
    // The 4th operand is the memory location for the target.
    // The 5th operand is the memory location for the source.
    //
    AML aml(PCIT::IPTR,
            PCIT::IPTR,
            PCIT::getMemoryAddressingMode(attrs[2]->getDatatype()),
            PCIT::getMemoryAddressingMode(attrs[0]->getDatatype()),
            PCIT::getMemoryAddressingMode(attrs[1]->getDatatype()));

    OL ol((Int64)(isOLAP()), (Int64)0,
          attrs[2]->getAtp(), attrs[2]->getAtpIndex(), attrs[2]->getOffset(),
          attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(),
          attrs[1]->getAtp(), attrs[1]->getAtpIndex(), attrs[1]->getOffset());

    // Add the OFFSET instruction.
    //
    PCI pci(PCIT::Op_OFFSET, aml, ol);
    code.append(pci);
  }
  
  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

// ExpSequenceFunction::eval
//
// Implment sequence functions...
//
// IN     : op_data      - output/input vector
// IN     : heap         - not used
// IN     : comDiagsArea - not used
// RETURN : ex_expr::EXPR_OK is no errors
// PRECOND: The op_data vector points to the NULL/VARCHAR and data.
//
ex_expr::exp_return_type ExpSequenceFunction::eval(char *op_data[],
						   CollHeap *heap,
						   ComDiagsArea **diagsArea) {
  
  // Get a handle on the operands so they can be queried for
  // NULL and VARCHAR -ness.
  //
  AttributesPtr *attrs = getOperand();

  // Get the pointer to the index data. If this clause has three operands
  // then the index is given by the last operand. Otherwise, the index
  // is a constant set in the clause.
  //
  Int32 index;
  if(getNumOperands() >= 3) {
    if(attrs[2]->getNullFlag() && !op_data[-2 * MAX_OPERANDS + 2])
      index = -1;
    else
      index = *((Int32 *)op_data[2]);
  }
  else
    index = offsetIndex_;

  // Lookup the indexed row in the history buffer. Compute pointers
  // to the attribute data, null indicator, and varchar indicator.
  //
  char *srcData = NULL;
  UInt32 srcLen = 0;
  char *srcNull = NULL;
  char *srcVC   = NULL;
  Int32 rc=0;

  {
    char *row = (*GetRow_)(GetRowData_, index, isLeading(), winSize(), rc);

    if(rc == -1)
      {
        ExRaiseSqlError(heap, diagsArea, EXE_HISTORY_BUFFER_TOO_SMALL);
        return ex_expr::EXPR_ERROR;
      }
    if(row) 
      {
        srcData = row + attrs[1]->getOffset();
        if(attrs[1]->getVCIndicatorLength() > 0)
          srcVC = row + attrs[1]->getVCLenIndOffset();
        if(attrs[1]->getNullFlag())
          srcNull = row + attrs[1]->getNullIndOffset();

        srcLen = getOperand(1)->getLength(srcVC);
      } else {
          if(getNumOperands() == 4) {
  
             srcData = op_data[3];
             srcLen = getOperand(3)->getLength(op_data[-MAX_OPERANDS+3]);
             srcNull = NULL;
          }
      }
  }

  // Is the source null? There are two reaons the source data can be null:
  // 1. The row does not exist in the history buffer (srcData == NULL)
  // 2. The data exist but is itself NULL (srcNull == 0xFFFF)
  //
  Int32 srcIsNull = (srcData ? 0 : 1);
  if(srcData && srcNull)
    srcIsNull = *((unsigned short*)(srcNull)) == 0xFFFFU;

  // Get the pointer to the start of the result data.
  //
  char *dstData = op_data[0];
  char *dstNull = op_data[-2 * MAX_OPERANDS + 0];
  char *dstVC   = op_data[- MAX_OPERANDS];

  if (rc == -3 && !srcData )
  {
    *((unsigned short*)dstNull) = 0xFFFFU;
    return ex_expr::EXPR_OK;
  }
  // Copy the source data to the destination data.
  //
  if(srcIsNull)
    {
      if(nullRowIsZero() || (rc == -2)) {
        switch (getOperand(1)->getDatatype()) {
        case REC_BIN16_UNSIGNED:
        case REC_BIN16_SIGNED: 
        {
          // hiding code from code coverag tool-- 
          short value = 0;
          str_cpy_all(dstData, (char *) &value, sizeof(value));
          break;
        }
        case REC_BIN32_SIGNED:
        case REC_BIN32_UNSIGNED:
        {
          Lng32 value = 0;
          str_cpy_all(dstData, (char *) &value, sizeof(value));
          break;
        }
        case REC_BIN64_SIGNED:
        {
          Int64 value = 0;
          str_cpy_all(dstData, (char *) &value, sizeof(value));
          break;
        }
        case REC_IEEE_FLOAT32:
        {
          float value = 0;
          str_cpy_all(dstData, (char *) &value, sizeof(value));
          break;
        }
        case REC_IEEE_FLOAT64:
        {
          double value = 0;
          str_cpy_all(dstData, (char *) &value, sizeof(value));
          break;
        }
        default:
        {
          Lng32 value = 0;
          if (convDoIt((char *)&value,
                       sizeof(value),
                       REC_BIN32_SIGNED,
                       0,
                       0,
                       dstData,
                       getOperand(1)->getLength(),
                       getOperand(1)->getDatatype(),
                       getOperand(1)->getPrecision(),
                       getOperand(1)->getScale(),
                       NULL, 0, heap, diagsArea,
                       CONV_UNKNOWN) != ex_expr::EXPR_OK) {
            return ex_expr::EXPR_ERROR;
          }

          break;
        }
        }

        if(attrs[0]->getNullFlag())
          *((short*)dstNull) = 0x0000;

      } else {
        *((unsigned short*)dstNull) = 0xFFFFU;
      }
    }
  else
    {
      Int32 len = attrs[0]->getLength();
      str_cpy_all(dstData, srcData, attrs[0]->getLength());
      if (attrs[1]->getVCIndicatorLength() > 0)
           getOperand(0)->setVarLength(srcLen, dstVC);
           //getOperand(0)->setVarLength(getOperand(1)->getLength(srcVC), dstVC);
      if(attrs[0]->getNullFlag())
        *((short*)dstNull) = 0x0000;
    }

  // Return all OK...
  //
  return ex_expr::EXPR_OK;
};
