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
****************************************************************************
*
* File:         ExpPCodeClauseGen.cpp
* Description:  
*
* Created:      8/25/97
* Language:     C++
*
*
*
****************************************************************************
*/

// ExpPCodeClauseGen.cpp
//
// The files contains the implementations of the virtual function
// pCodeGenerate for the ex_clause and derived classes.
//
//

#include "Platform.h"


// Includes
//
#include "Platform.h"
#include "exp_stdh.h"
#include "str.h"
#include "exp_datetime.h"
#include "exp_expr.h"
#include "exp_function.h"
#include "exp_math_func.h"
#include "ExpPCode.h"
#include "ExpLOB.h"

// #include "DatetimeType.h"

// ex_clause::pCodeGenerate
//
// PCode generation for the default case (ie. no fundamental pcode 
// instructions to do the operation) so the original clause->eval is 
// executed via the PCode instruction CLAUSE_EVAL.
//
// IN     : space - memory allocator
// OUT    : 
// RETURN : ex_expr::EXPR_OK is no errors
// EFFECTS: stores pointer to PCodeObject in clause
//
ex_expr::exp_return_type ex_clause::pCodeGenerate(Space *space, UInt32 f) {
  PCIList code(space);
  AttributesPtr *attrs = getOperand();

  Int32 nNullableInputs = 0;
  Int32 i = 0;
  for(i=0; i<getNumOperands(); i++) {
    if (! attrs[i])
      continue;

    if((i>0) && attrs[i]->getNullFlag())
      nNullableInputs++;
  }

  // Generate necessary pre clause PCI's 
  //
  PCode::preClausePCI(this, code);

  // stackDepth is the total Depth of the stack (ie. the size of the 
  // op_data vector that is being created) for the eval call.
  //
  // stackPushesWaiting is the current number of stack pushes needed for
  // the eval call. Pushes are not done immediately so that consective 
  // pushes can be coalesced.
  //
  //int stackDepth = 0;
  //int stackPushesWaiting = 0;
  Int32 useProcessNulls = 0;

  // Branch target IDs
  //
  PCIID branchEndA = 0;

  // Are any of the attributes varchar?
  //
  Int32 varchar = 0;
  for(i=0; i<getNumOperands(); i++) 
  {
    if (! attrs[i])
      continue;

    if(attrs[i]->getVCIndicatorLength() > 0) 
      varchar = 1;
  }

  // 
  // Set the first fixed field for the 2 disk formats.
  if(getNumOperands() > 0)
    code.append(PCode::storeVoa(attrs[0], space));

  // If any of the operands are nullable, NULL is relevant for
  // this clause, and any NULL input produces a NULL output, insert the
  // appropriate PCODE sequence to compute the nullness of the result.
  // (If branching is not desireable, then define the virtual methods
  //  below for that specific clause type.)
  if(isAnyOperandNullable() && isNullRelevant() && isNullInNullOut()
     && (nNullableInputs < 3)) { 
    branchEndA = PCode::nullBranch(this, code, attrs);
  }
  // If any of the inputs are nullable, insert the appropriate PCODE 
  // sequence to load the null indicators onto the stack.
  //
  else if(isAnyOperandNullable())
  {
    // Load the address of the NULL indicator for the result into the
    // OpData vector.
    //
    if(attrs[0]->getNullFlag())
    {
      if ( attrs[0]->isSQLMXAlignedFormat() )
        code.append(PCode::loadOpDataNullBitmapAddress(attrs[0], 0, space));
      else
        code.append(PCode::loadOpDataNullAddress(attrs[0], 0, space));
    }

    // Load and test each nullable input for NULL, and leave the results
    // on the stack for eval. If an operand is NULL leave a zero on the
    // stack, otherwise a non-zero.
    //
    for(i=1; i<getNumOperands(); i++) {
      if (! attrs[i])
        continue;

      if(attrs[i]->getNullFlag())
      {
        if ( attrs[i]->isSQLMXAlignedFormat() )
          code.append(PCode::loadOpDataNullBitmap(attrs[i], i, space));
        else
	  code.append(PCode::loadOpDataNull(attrs[i], i, space));
      }
    }

    // If special NULL processing is needed, set flag so the process nulls
    // flag in the CLAUSE_EVAL instruction will be set.
    if(isNullRelevant())
      useProcessNulls = 1;
  }

  // Setup the op_data vector for the call to eval. (The null indicator part
  // of the vector has already been constructed if necessary).
  //
  // Compute pointers to the varchar indicators (if necessary) on the stack
  //
  if(varchar) 
    {
      for(i=0; i<getNumOperands(); i++)
        // For indirect varchars, this also loads the data address.
	code.append(PCode::loadOpDataVCAddress(attrs[i], i, space));
    } 
  // If there are no varchar, but there are NULL considerations, push empty
  // space on the stack because eval may access the NULL information
  //
  else if(isAnyOperandNullable() && (1 || !isNullRelevant())) 
    {
      ;
    }

  // Load the data addresses.
  //
  for(i=0; i<getNumOperands(); i++) 
    {
      // For indirect varchars, the data address is loaded at the same time
      // as the VCLen Address.
      if( attrs[i] && (! attrs[i]->isIndirectVC()))
        code.append(PCode::loadOpDataDataAddress(attrs[i], i, space));
    }


  // Execute CLAUSE_EVAL which finishes pushing the stack, executes
  // clause->eval(), and then pops the stack.
  //
  AML aml(PCIT::IPTR, PCIT::IBIN32S);
  OL ol((Int64)this, useProcessNulls);
  PCI pci(PCIT::Op_CLAUSE_EVAL, aml, ol);
 
  setNoPCodeAvailable(TRUE);

  code.append(pci); 

  // update the varOffset if this CLAUSE_EVAL writes to a varchar in aligned format.
  // (unless this varchar is being treated as a fixed value in the aligned row).
  // Note that the updateRowlen instruction also updates the varOffset cursor of evalPCode().
  if ( getNumOperands() > 0 &&
       attrs[0]->isSQLMXDiskFormat() &&
       (attrs[0]->getVCIndicatorLength() > 0) &&
       !attrs[0]->isForceFixed() )
  {
    code.append(PCode::updateRowLen(attrs[0], space, f));
  }

  // Branch targets
  //
  if(branchEndA)
  {
    AML aml1;
    OL ol1((Int64)branchEndA);
    PCI pci1(PCIT::Op_TARGET, aml1, ol1);
    code.append(pci1);
  }

  // Handle clauses that branch. Insert a CLAUSE_BRANCH instruction into 
  // the code. A TARGET PCI instruction will be/has been added when PCI's are 
  // generated for the target clause. This no longer handles marking the
  // target clauses because it is too late for backwards branching at this
  // point. This is handled in ex_expr::pCodeGenerate().
  //
  if(isBranchingClause()) 
    {
      ex_branch_clause *branchClause = (ex_branch_clause*)this;
      ex_clause *targetClause = branchClause->get_branch_clause();
      AML aml(PCIT::IPTR, PCIT::IPTR);
      OL ol((Int64)targetClause, (Int64)this);
      PCI pci(PCIT::Op_CLAUSE_BRANCH, aml, ol);
      code.append(pci);
    }

  // Generate the necessary post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

// ex_inout_clause::pCodeGenerate
//
// For now simply calls the default ex_clause::pCodeGenerate
//
ex_expr::exp_return_type ex_inout_clause::pCodeGenerate(Space *space, UInt32 f) {
  return ex_clause::pCodeGenerate(space, f);
}

// ex_aggr_min_max_clause::pCodeGenerate
//
ex_expr::exp_return_type ex_aggr_min_max_clause::pCodeGenerate(Space *space, UInt32 f) {

#ifdef _DEBUG
  if (getenv("NO_PCODE_MIN_MAX"))
    return ex_clause::pCodeGenerate(space, f);
#endif

  if ((getOperand(1)->getNullFlag()) ||
      (getOperand(0)->getVCIndicatorLength() > 0))
    return ex_clause::pCodeGenerate(space, f);

  // Get a handle on the operands
  //
  AttributesPtr *attrs = getOperand();
  // Allocate the code list
  //
  PCIList code(space);

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  // First operand is the memory location of the result (min/max)
  // Second operand is the memory location of the 1st argument
  // Third operand is the memory location of the result of comparison
  // of new child value and current min/max.
  // If third operand is 1(TRUE), then move the first operand to result.
  // Fourth operand is the length of first operand. Data for this length
  // will be moved to result as the nee min/max value.
  AML aml(PCIT::MBIN8,PCIT::MBIN8,PCIT::MBIN32S,PCIT::IBIN32S);
  OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(),
        attrs[1]->getAtp(), attrs[1]->getAtpIndex(), attrs[1]->getOffset(),
        attrs[2]->getAtp(), attrs[2]->getAtpIndex(), attrs[2]->getOffset(),
        attrs[1]->getLength());

  PCI pci(PCIT::Op_MINMAX, aml, ol);
  code.append(pci);
  
  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK; 

  /* //  if ((getOperand(0)->getNullFlag()) ||
  if ((getOperand(1)->getNullFlag()) ||
      (getOperand(0)->getVCIndicatorLength() > 0))
    return ex_clause::pCodeGenerate(space, f);

  // Get a handle on the operands
  //
  AttributesPtr *attrs = getOperand();
  // Allocate the code list
  //
  PCIList code(space);

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  // Load the second operand
  //
  code.append(PCode::loadValue(attrs[2], space));

  // if second operand is 1(TRUE), then move the first operand to result.
  // else branch out.

  // The runtime BRANCH pcode instruction branches out if operand
  // is TRUE. We want to branch out if operand is FALSE. So, complement
  // the operand.
  AML aml0(PCIT::IBIN32S, PCIT::IBIN32S);
  PCI pci0(PCIT::Op_ZERO, aml0);
  code.append(pci0);
  
  // and branch out if it is true
  AML aml(PCIT::IBIN32S);
  OL ol(0); 
  PCI pci(PCIT::Op_BRANCH, aml, ol);
  code.append(pci);
  PCIID branchTgt = code.getTailId();
  
  code.append(PCode::moveValue(attrs[0], attrs[1], space));

  // End branch target.
  //
  AML aml6;
  OL ol6((Int64)branchTgt);
  PCI pci6(PCIType::Op_TARGET, aml6, ol6);
  code.append(pci6);
    
  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK; */
}

// ex_function_clause::pCodeGenerate
//
// For now simply calls the default ex_clause::pCodeGenerate
//
ex_expr::exp_return_type ex_function_clause::pCodeGenerate(Space *space, UInt32 f) {
  return ex_clause::pCodeGenerate(space, f);
}

// ex_function_encode::pCodeGenerate
//
// For now PCode in only generated for very simple encodings. This is 
// intended to improve scan/update node performance since the key encode 
// expression is called frequently in these nodes.
//
// IN     : space - memory allocator
// OUT    : 
// RETURN : ex_expr::EXPR_OK is no errors
// EFFECTS: stores pointer to PCodeObject in clause
//
ex_expr::exp_return_type ex_function_encode::pCodeGenerate(Space *space, UInt32 f) {
#ifdef _DEBUG
  // For debugging...
  if(getenv("PCODE_NO_ENCODE")) return ex_clause::pCodeGenerate(space, f);
#endif

  // Generate the standard clause->eval PCode for cases that
  // are not handled with more fundamental PCode operations
  //
  // For now, no nulls unless regularNullability has been explicitly asked.
  //

  if ((NOT regularNullability()) &&
      (isAnyOperandNullable()))
    return ex_clause::pCodeGenerate(space, f);

  AttributesPtr *attrs = getOperand();
  Attributes    *src = attrs[1];
  Lng32 fsDataType = src->getDatatype();
  Lng32 length = src->getLength();

  if (isDecode())
    {
      if (! getenv("PCODE_DECODE"))
	return ex_clause::pCodeGenerate(space, f);
    }
    
  switch(fsDataType) {
  case REC_BIN8_SIGNED:
  case REC_BIN8_UNSIGNED:
    break;

  case REC_BIN16_SIGNED:
  case REC_BIN16_UNSIGNED:
  case REC_BIN32_SIGNED:
  case REC_BIN32_UNSIGNED:
  case REC_BIN64_SIGNED:
    break;

  case REC_DECIMAL_LSE:
  case REC_DECIMAL_UNSIGNED:
    {
      // not enabled for NEO CA
      return ex_clause::pCodeGenerate(space, f);
    }
  break;

  case REC_BYTE_F_ASCII:
    // case REC_NCHAR_F_UNICODE:

    // for now, do non-pcode encoding for caseInsensitive datatypes.
    // Later, add this to PCODE.
    if (caseInsensitive())
      return ex_clause::pCodeGenerate(space, f);
    
    //no pcode for now for Czech collated encode 
    // this is for non nullable  op(1) 
    if (CollationInfo::isSystemCollation(getCollation()))
    {
      return ex_clause::pCodeGenerate(space, f);
    }
  break;

  case REC_DATETIME:
    // Do not generate PCODE to encode the non-standard SQL/MP
    // datetime types (for now).
    // Do not generate PCODE on windows(Little Endian) platform.
    // With little endian, we need to reversebytes the DATE and
    // FRACTION part. Pcode cannot do that.
#ifdef NA_LITTLE_ENDIAN
      return ex_clause::pCodeGenerate(space, f);
#else    
    if(src->getPrecision() > REC_DTCODE_TIMESTAMP)
      return ex_clause::pCodeGenerate(space, f);
#endif
  break;
  
  default:
    return ex_clause::pCodeGenerate(space, f);
  }

  // If we get to this point, we have decided to generate PCode for this
  // particular encode operation.
  //
  PCIList code(space);
  Attributes *tgt = attrs[0];

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  // Add the null logic if necessary.
  //
  PCIID branchToEnd = PCode::nullBranch(this, code, attrs);

  if ((fsDataType == REC_BYTE_F_ASCII) ||
      (fsDataType == REC_DATETIME) ||
      (fsDataType == REC_DECIMAL_LSE) ||
      (fsDataType == REC_DECIMAL_UNSIGNED))
    {
      // The first operand is the memory location for the result.
      // The second operand is the memory location for the source.
      // The third operand is the length of data to encode.
      // The fourth operand is a boolean for ascending/descending.
      //
      short attrs1Datatype = src->getDatatype();
      if ((fsDataType == REC_DATETIME) ||
	  (fsDataType == REC_DECIMAL_UNSIGNED))
	attrs1Datatype = REC_BYTE_F_ASCII;

      AML aml(PCIT::getMemoryAddressingMode(tgt->getDatatype()),
	      PCIT::getMemoryAddressingMode(attrs1Datatype),
	      PCIT::IBIN32S,
	      PCIT::IBIN32S);
      OL ol(tgt->getAtp(), tgt->getAtpIndex(),tgt->getOffset(),
	    src->getAtp(), src->getAtpIndex(),src->getOffset(),
	    tgt->getLength(),
	    (isDesc()));

      // Add the encode instruction.
      //
      if (isDecode())
	{
	  PCI pci(PCIT::Op_DECODE, aml, ol);
	  code.append(pci);
	}
      else
	{
	  PCI pci(PCIT::Op_ENCODE, aml, ol);
	  code.append(pci);
	}

    }
  else
    {
      // The first operand is the memory location for the result.
      // The second operand is the memory location for the source.
      // The third operand is a boolean for ascending/descending.
      //
      AML aml(PCIT::getMemoryAddressingMode(tgt->getDatatype()),
	      PCIT::getMemoryAddressingMode(src->getDatatype()),
	      PCIT::IBIN32S);
      OL ol(tgt->getAtp(), tgt->getAtpIndex(),tgt->getOffset(),
	    src->getAtp(), src->getAtpIndex(),src->getOffset(),
	    (isDesc()));

      // Add the encode instruction.
      //
      if (isDecode())
	{
	  PCI pci(PCIT::Op_DECODE, aml, ol);
	  code.append(pci);
	}
      else
	{
	  PCI pci(PCIT::Op_ENCODE, aml, ol);
	  code.append(pci);
	}
    }

  // Add the branch target if necessary.
  //
  if(branchToEnd)
  {
    AML aml;
    OL ol((Int64)branchToEnd);
    PCI pci(PCIT::Op_TARGET, aml, ol);
    code.append(pci);
  }

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

// ex_aggregate_clause::pCodeGenerate
//
// For now simply calls the default ex_clause::pCodeGenerate
//
ex_expr::exp_return_type ex_aggregate_clause::pCodeGenerate(Space *space, UInt32 f) { 
  return ex_clause::pCodeGenerate(space, f);
};

// ex_aggr_one_row_clause::pCodeGenerate
//
// For now simply calls the default ex_clause::pCodeGenerate
//
ex_expr::exp_return_type ex_aggr_one_row_clause::pCodeGenerate(Space *space, UInt32 f) { 
  return ex_clause::pCodeGenerate(space, f); 
};

// ex_aggr_any_true_max_clause::pCodeGenerate
//
// For now simply calls the default ex_clause::pCodeGenerate
//
ex_expr::exp_return_type ex_aggr_any_true_max_clause::pCodeGenerate(Space *space, UInt32 f) { 
  return ex_clause::pCodeGenerate(space, f);
};

// ex_bool_clause::pCodeGenerate
//
// Generates PCI's for ITM_AND and ITM_OR bool operations. The PCI's use
// tristate logic to compute either the logical AND or OR of the two
// input operands.
//
// IN     : space - memory allocator
// OUT    : 
// RETURN : ex_expr::EXPR_OK is no errors
// EFFECTS: stores pointer to PCodeObject in clause
//
ex_expr::exp_return_type ex_bool_clause::pCodeGenerate(Space *space, UInt32 f) { 
#ifdef _DEBUG
  // For debugging...
  if(getenv("PCODE_NO_BOOL")) return ex_clause::pCodeGenerate(space, f);
#endif

  // Generate the standard clause->eval PCode for cases that
  // are not handled with more fundamental PCode operations.
  //
  switch(getOperType()) {
  case ITM_AND:
  case ITM_OR:
    break;

  default:
    return ex_clause::pCodeGenerate(space, f);
  }

  // If we get to this point, we have decided to generate PCI's for this
  // particular bool operation. Get a handle on the attributes and allocate
  // the code list.
  //
  AttributesPtr *attrs = getOperand();
  PCIList code(space);

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  // The first operand is the memory location of the boolean result.
  // The second operand is the memory location of the 1st argument.
  // The third operand is the memory location of the 2nd argument.
  //
  AML aml(PCIT::MBIN32S, PCIT::MBIN32S, PCIT::MBIN32S);
  OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(),
	attrs[1]->getAtp(), attrs[1]->getAtpIndex(), attrs[1]->getOffset(),
	attrs[2]->getAtp(), attrs[2]->getAtpIndex(), attrs[2]->getOffset());

  // Generate the instruction
  //
  PCIT::Operation op = PCIT::Op_OPDATA; // prevent init warning
  switch(getOperType()) {
  case ITM_AND: op = PCIT::Op_AND; break;
  case ITM_OR: op = PCIT::Op_OR; break;
  };

  PCI pci(op, aml, ol); 
  code.append(pci); 

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK; 
};

// ex_bool_result_clause::pCodeGenerate
//
// Generates PCI's for bool result operation. The PCI's should return 
// ex_expr::EXPR_TRUE for the result of the expression if the 1st operand 
// is TRUE, otherwise return ex_expr::EXPR_FALSE.
//
// IN     : space - memory allocator
// OUT    : 
// RETURN : ex_expr::EXPR_OK is no errors
// EFFECTS: stores pointer to PCodeObject in clause
//
ex_expr::exp_return_type bool_result_clause::pCodeGenerate(Space *space, UInt32 f) { 
#ifdef _DEBUG
  // For debugging...
  if(getenv("PCODE_NO_BOOL_RESULT")) return ex_clause::pCodeGenerate(space, f);
#endif

  // Allocate the code list and get a handle on the operands
  //
  PCIList code(space);
  AttributesPtr *attrs = getOperand();

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  // The first operand is the memory location of the boolean return value.
  //
  AML aml(PCIT::MBIN32S);
  OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset());

  // Add the return instruction.
  //
  PCI pci(PCIT::Op_MOVE, aml, ol);
  code.append(pci);

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK; 
};

// ex_branch_clause::pCodeGenerate
//
// For now simply calls default ex_clause::pCodeGenerate(space, f)
//
// IN     : space - memory allocator
// OUT    : 
// RETURN : ex_expr::EXPR_OK is no errors
// EFFECTS: stores pointer to PCodeObject in clause
//
ex_expr::exp_return_type ex_branch_clause::pCodeGenerate(Space *space, UInt32 f) {

  if ((getOperType() != ITM_AND) &&
      (getOperType() != ITM_OR))
    return ex_clause::pCodeGenerate(space, f);
    
  PCIList code(space);
  AttributesPtr *attrs = getOperand();

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);
  

  
  ex_clause *targetClause = get_branch_clause();
  AML aml(PCIT::IPTR, PCIT::IPTR, PCIT::MBIN32S, PCIT::MBIN32S);
  OL ol((Int64)targetClause, (Int64)0,
	attrs[0]->getAtp(), attrs[0]->getAtpIndex(), (Int32)attrs[0]->getOffset(),
	attrs[1]->getAtp(), attrs[1]->getAtpIndex(), (Int32)attrs[1]->getOffset());

  // Generate the branch instruction
  //
  if (getOperType() == ITM_AND)
    {
      PCI pci(PCIT::Op_BRANCH_AND, aml, ol);
      code.append(pci);
    }
  else
    {
      PCI pci(PCIT::Op_BRANCH_OR, aml, ol);
      code.append(pci);
    }
    
  // Finish up and return
  //
  PCode::postClausePCI(this, code);
  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
};

// ex_comp_clause::pCodeGenerate
//
// Generate PCI's for the comparison operation. The PCI's load the operands,
// do the comparison, and stores the tristate result.
//
// IN     : space - memory allocator
// OUT    : 
// RETURN : ex_expr::EXPR_OK is no errors
// EFFECTS: stores pointer to PCodeObject in clause
//
ex_expr::exp_return_type ex_comp_clause::pCodeGenerate(Space *space, UInt32 f) { 
#ifdef _DEBUG
  // For debugging...
  if(getenv("PCODE_NO_COMP")) return ex_clause::pCodeGenerate(space, f);
#endif

  NABoolean flipOperands = FALSE;
  AttributesPtr *attrs = getOperand();
  Attributes *op1 = attrs[1];
  Attributes *op2 = attrs[2];

  // if this comp clause need to return the column num which caused the
  // comparison to fail, then do not generate pcode. That functionality is
  // not yet supported in pcode.
  // This is used for rollup group computation which need to know the particular
  // grouping column that caused the comparison to fail.
  if (getRollupColumnNum() >= 0)
    return ex_clause::pCodeGenerate(space, f);

  // Generate the standard clause->eval PCode for cases that
  // are not handled with more fundamental PCode operations.
  //
  switch(getInstruction()) {
  case NE_ASCII_F_F:
  case EQ_ASCII_F_F:
  case LT_ASCII_F_F:
  case LE_ASCII_F_F:
  case GT_ASCII_F_F:
  case GE_ASCII_F_F:
    break;

  case NE_UNICODE_F_F:
  case EQ_UNICODE_F_F:
  case LT_UNICODE_F_F:
  case LE_UNICODE_F_F:
  case GT_UNICODE_F_F:
  case GE_UNICODE_F_F:
  case UNICODE_COMP:
    break ;

  case COMP_COMPLEX:
    if (attrs[1]->getClassID() != Attributes::BigNumID)
      return ex_clause::pCodeGenerate(space, f);
    break;

  case ASCII_COMP:
    // R2.4 has support for string compares of unequal lengths
    if ((attrs[1]->getDatatype() != REC_BYTE_F_ASCII) ||
        (attrs[2]->getDatatype() != REC_BYTE_F_ASCII))
      return ex_clause::pCodeGenerate(space, f);
    break;


  case EQ_ASCII_COMP:
  case NE_ASCII_COMP:
  case LT_ASCII_COMP:
  case LE_ASCII_COMP:
  case GT_ASCII_COMP:
  case GE_ASCII_COMP:
    // PCIT::getMemoryAddressingMode(), used below, cannot currently handle these.
    if ((attrs[1]->getDatatype() == REC_BYTE_V_ASCII_LONG ) ||
        (attrs[2]->getDatatype() == REC_BYTE_V_ASCII_LONG ))
      return ex_clause::pCodeGenerate(space, f);
    break;

  case EQ_BIN8S_BIN8S:
  case EQ_BIN8U_BIN8U:
  case EQ_BIN16S_BIN16S: 
  case EQ_BIN16S_BIN32S:
  case EQ_BIN32S_BIN16S: 
  case EQ_BIN32S_BIN32S:
  case EQ_BIN16U_BIN16U: 
  case EQ_BIN16U_BIN32U:
  case EQ_BIN32U_BIN16U: 
  case EQ_BIN32U_BIN32U: 
  case EQ_BIN16S_BIN32U:
  case EQ_BIN32U_BIN16S:

  case LT_BIN8S_BIN8S:
  case LT_BIN8U_BIN8U:
  case LT_BIN16S_BIN16S: 
  case LT_BIN16S_BIN32S:
  case LT_BIN32S_BIN16S: 
  case LT_BIN32S_BIN32S:
  case LT_BIN16U_BIN16U: 
  case LT_BIN16U_BIN32U:
  case LT_BIN32U_BIN16U: 
  case LT_BIN32U_BIN32U: 
  case LT_BIN16S_BIN32U:
  case LT_BIN32U_BIN16S:

  case GT_BIN8S_BIN8S:
  case GT_BIN8U_BIN8U:
  case GT_BIN16S_BIN16S: 
  case GT_BIN16S_BIN32S:
  case GT_BIN32S_BIN16S: 
  case GT_BIN32S_BIN32S:
  case GT_BIN16U_BIN16U: 
  case GT_BIN16U_BIN32U:
  case GT_BIN32U_BIN16U: 
  case GT_BIN32U_BIN32U: 
  case GT_BIN16S_BIN32U:
  case GT_BIN32U_BIN16S:
    break;

  case LE_BIN8S_BIN8S:
  case LE_BIN8U_BIN8U:
  case LE_BIN16S_BIN16S: 
  case LE_BIN16S_BIN32S:
  case LE_BIN32S_BIN16S: 
  case LE_BIN32S_BIN32S:
  case LE_BIN16U_BIN16U: 
  case LE_BIN16U_BIN32U:
  case LE_BIN32U_BIN16U: 
  case LE_BIN32U_BIN32U: 
  case LE_BIN16S_BIN32U:
  case LE_BIN32U_BIN16S:

  case GE_BIN8S_BIN8S:
  case GE_BIN8U_BIN8U:
  case GE_BIN16S_BIN16S: 
  case GE_BIN16S_BIN32S:
  case GE_BIN32S_BIN16S: 
  case GE_BIN32S_BIN32S:
  case GE_BIN16U_BIN16U: 
  case GE_BIN16U_BIN32U:
  case GE_BIN32U_BIN16U: 
  case GE_BIN32U_BIN32U: 
  case GE_BIN16S_BIN32U:
  case GE_BIN32U_BIN16S:
    break;

  case EQ_BIN16U_BIN16S: 
  case LE_BIN16U_BIN16S:
  case LT_BIN16U_BIN16S:
  case GE_BIN16U_BIN16S:
  case GT_BIN16U_BIN16S:
    break;

  case EQ_BIN64S_BIN64S:
  case LE_BIN64S_BIN64S:
  case LT_BIN64S_BIN64S:
  case GE_BIN64S_BIN64S:
  case GT_BIN64S_BIN64S:
  case NE_BIN64S_BIN64S:
    break;

  case NE_BIN8S_BIN8S:
  case NE_BIN8U_BIN8U:
  case NE_BIN16S_BIN16S:
    break;

  case NE_FLOAT32_FLOAT32:
  case EQ_FLOAT32_FLOAT32:
  case LT_FLOAT32_FLOAT32:
  case LE_FLOAT32_FLOAT32:
  case GT_FLOAT32_FLOAT32:
  case GE_FLOAT32_FLOAT32:

  case NE_FLOAT64_FLOAT64:
  case EQ_FLOAT64_FLOAT64:
  case LT_FLOAT64_FLOAT64:
  case LE_FLOAT64_FLOAT64:
  case GT_FLOAT64_FLOAT64:
  case GE_FLOAT64_FLOAT64:
    break;


  case EQ_DATETIME_DATETIME:
#ifndef NA_LITTLE_ENDIAN
  case LT_DATETIME_DATETIME:
  case GT_DATETIME_DATETIME:
  case LE_DATETIME_DATETIME:
  case GE_DATETIME_DATETIME:
#endif
    // Do not generate PCODE to encode the non-standard SQL/MP
    // datetime types (for now).
    if ((attrs[1]->getPrecision() > REC_DTCODE_TIMESTAMP) ||
	(attrs[1]->getPrecision() != attrs[2]->getPrecision()) ||
	(attrs[1]->getLength() != attrs[2]->getLength()))
      return ex_clause::pCodeGenerate(space, f);
    break;

  case EQ_BOOL_BOOL:
  case NE_BOOL_BOOL:
    break;

  default:
    return ex_clause::pCodeGenerate(space, f);
  }

  // Generate the comparison operator
  //
  PCIT::Operation op = PCIT::Op_OPDATA; // prevent init warning
  switch(getInstruction()) {

  case EQ_BIN32S_BIN16S:
  case EQ_BIN32U_BIN16U:
  case EQ_BIN32U_BIN16S:
    // Normalize instructions so smaller operand comes first
    flipOperands = TRUE;
    op = PCIT::Op_EQ;
    break;

  case EQ_BIN8S_BIN8S:
  case EQ_BIN8U_BIN8U:
  case EQ_BIN16U_BIN16S: 
  case EQ_BIN16S_BIN16S: 
  case EQ_BIN16S_BIN32S:
  case EQ_BIN32S_BIN32S:
  case EQ_BIN16U_BIN16U: 
  case EQ_BIN16U_BIN32U:
  case EQ_BIN32U_BIN32U: 
  case EQ_BIN16S_BIN32U:
  case EQ_FLOAT32_FLOAT32:
  case EQ_FLOAT64_FLOAT64:
  case EQ_BIN64S_BIN64S:
  case EQ_ASCII_F_F:
  case EQ_ASCII_COMP:
  case EQ_DATETIME_DATETIME:
     op = PCIT::Op_EQ;
    break;

  case EQ_BOOL_BOOL:
     op = PCIT::Op_EQ;
    break;

  case LT_BIN32S_BIN16S:
  case LT_BIN32U_BIN16U:
  case LT_BIN32U_BIN16S:
    // Normalize instructions so smaller operand comes first
    flipOperands = TRUE;
    op = PCIT::Op_GT;
    break;

  case LT_BIN8S_BIN8S:
  case LT_BIN8U_BIN8U:
  case LT_BIN16U_BIN16S:
  case LT_BIN16S_BIN16S:
  case LT_BIN16S_BIN32S:
  case LT_BIN32S_BIN32S:
  case LT_BIN16U_BIN16U:
  case LT_BIN16U_BIN32U:
  case LT_BIN32U_BIN32U:
  case LT_BIN16S_BIN32U:
  case LT_BIN64S_BIN64S:
  case LT_FLOAT32_FLOAT32:
  case LT_FLOAT64_FLOAT64:
  case LT_ASCII_F_F:
  case LT_ASCII_COMP:
  case LT_DATETIME_DATETIME:
    op = PCIT::Op_LT;
    break;

  case GT_BIN32S_BIN16S:
  case GT_BIN32U_BIN16U:
  case GT_BIN32U_BIN16S:
    // Normalize instructions so smaller operand comes first
    flipOperands = TRUE;
    op = PCIT::Op_LT;
    break;

  case GT_BIN8S_BIN8S:
  case GT_BIN8U_BIN8U:
  case GT_BIN16U_BIN16S:
  case GT_BIN16S_BIN16S:
  case GT_BIN16S_BIN32S:
  case GT_BIN32S_BIN32S:
  case GT_BIN16U_BIN16U:
  case GT_BIN16U_BIN32U:
  case GT_BIN32U_BIN32U:
  case GT_BIN16S_BIN32U:
  case GT_FLOAT32_FLOAT32:
  case GT_FLOAT64_FLOAT64:
  case GT_BIN64S_BIN64S:
  case GT_ASCII_F_F:
  case GT_ASCII_COMP:
  case GT_DATETIME_DATETIME:
    op = PCIT::Op_GT;
    break;

  case LE_BIN32S_BIN16S:
  case LE_BIN32U_BIN16U:
  case LE_BIN32U_BIN16S:
    // Normalize instructions so smaller operand comes first
    flipOperands = TRUE;
    op = PCIT::Op_GE;
    break;

  case LE_BIN8S_BIN8S:
  case LE_BIN8U_BIN8U:
  case LE_BIN16U_BIN16S:
  case LE_BIN16S_BIN16S:
  case LE_BIN16S_BIN32S:
  case LE_BIN32S_BIN32S:
  case LE_BIN16U_BIN16U:
  case LE_BIN16U_BIN32U:
  case LE_BIN32U_BIN32U:
  case LE_BIN16S_BIN32U:
  case LE_FLOAT32_FLOAT32:
  case LE_FLOAT64_FLOAT64:
  case LE_BIN64S_BIN64S:
  case LE_ASCII_F_F:
  case LE_ASCII_COMP:
  case LE_DATETIME_DATETIME:
    op = PCIT::Op_LE;
    break;

  case GE_BIN32S_BIN16S:
  case GE_BIN32U_BIN16U:
  case GE_BIN32U_BIN16S:
    // Normalize instructions so smaller operand comes first
    flipOperands = TRUE;
    op = PCIT::Op_LE;
    break;

  case GE_BIN8S_BIN8S:
  case GE_BIN8U_BIN8U:
  case GE_BIN16U_BIN16S:
  case GE_BIN16S_BIN16S:
  case GE_BIN16S_BIN32S:
  case GE_BIN32S_BIN32S:
  case GE_BIN16U_BIN16U:
  case GE_BIN16U_BIN32U:
  case GE_BIN32U_BIN32U:
  case GE_BIN16S_BIN32U:
  case GE_FLOAT32_FLOAT32:
  case GE_FLOAT64_FLOAT64:
  case GE_BIN64S_BIN64S:
  case GE_ASCII_F_F:
  case GE_ASCII_COMP:
#ifndef NA_LITTLE_ENDIAN
  case GE_DATETIME_DATETIME:
#endif
    op = PCIT::Op_GE;
    break;

  case NE_BIN8S_BIN8S:
  case NE_BIN8U_BIN8U:
  case NE_FLOAT32_FLOAT32:
  case NE_FLOAT64_FLOAT64:
  case NE_BIN64S_BIN64S:
  case NE_BIN16S_BIN16S:
  case NE_ASCII_COMP:
  case NE_ASCII_F_F:
  case NE_BOOL_BOOL:
    op = PCIT::Op_NE;
    break;

  case ASCII_COMP:
  case UNICODE_COMP:
  case COMP_COMPLEX:
  case EQ_UNICODE_F_F:
  case LT_UNICODE_F_F:
  case GT_UNICODE_F_F:
  case LE_UNICODE_F_F:
  case GE_UNICODE_F_F:
  case NE_UNICODE_F_F:
    op = PCIT::Op_COMP;
    break;

  }

  // No NULLS for now
  //
  if (isAnyOperandNullable())
    {
      // if special nulls, only handle equality predicate. 
      if ( isSpecialNulls() && (getOperType() != ITM_EQUAL) )
        return ex_clause::pCodeGenerate(space, f);
    }

  // Allocate the code list and get a handle on the attributes.
  //
  PCIList code(space);

  // Don't get mixed up in interval conversions. The precision for
  // interval conversions is not set up correctly.
  //
  Attributes *tgt = attrs[0];
  if((op1->getDatatype() >= REC_MIN_INTERVAL) &&
     (op1->getDatatype() <= REC_MAX_INTERVAL) &&
     (op2->getDatatype() >= REC_MIN_INTERVAL) &&
     (op2->getDatatype() <= REC_MAX_INTERVAL))
    return ex_clause::pCodeGenerate(space, f);

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  // Add the null logic if necessary.
  //
  PCIID branchToEnd = PCode::nullBranch(this, code, attrs);

  if ((op1->getDatatype() == REC_NUM_BIG_SIGNED) ||
      (op1->getDatatype() == REC_NUM_BIG_UNSIGNED))
  {
    // Operand 1: memory location of boolean result
    // Operand 2: memory location of 1st argument
    // Operand 3: memory location of 2nd argument
    // Operand 4: length of data to compare
    // Operand 5: comparison operand

    AML aml(PCIT::getMemoryAddressingMode(attrs[0]->getDatatype()),
	    PCIT::MBIGS,
	    PCIT::MBIGS,
	    PCIT::IBIN32S,
            PCIT::IBIN32S);

    OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(),
	  attrs[1]->getAtp(), attrs[1]->getAtpIndex(), attrs[1]->getOffset(),
	  attrs[2]->getAtp(), attrs[2]->getAtpIndex(), attrs[2]->getOffset(),
	  attrs[2]->getLength(), (Int32)getOperType());

    // Add the comparison instruction.
    //
    PCI pci(op, aml, ol);
    code.append(pci);
  }
  // both the operands are fixed chars/unicode or are of type DATETIME
  // or of type boolean.
  else if (((op1->getDatatype() == REC_BYTE_F_ASCII) &&
            (op2->getDatatype() == REC_BYTE_F_ASCII)) ||
           ((op1->getDatatype() == REC_NCHAR_F_UNICODE) &&
            (op2->getDatatype() == REC_NCHAR_F_UNICODE)) ||
           (op1->getDatatype() == REC_DATETIME) ||
           (op1->getDatatype() == REC_BOOLEAN))
  {
    // Operand 1: memory location of boolean result
    // Operand 2: memory location of 1st argument
    // Operand 3: memory location of 2nd argument
    // Operand 4: length of data to compare
    short attrs1Datatype = op1->getDatatype();
    short attrs2Datatype = op2->getDatatype();
    if (attrs1Datatype == REC_DATETIME)
      {
	attrs1Datatype = REC_BYTE_F_ASCII;
	attrs2Datatype = REC_BYTE_F_ASCII;
      }

    // Set "s" for attribute number with smaller string.
    Int32 s = (attrs[1]->getLength() > attrs[2]->getLength()) ? 2 : 1;
    Int32 l = (s == 1) ? 2 : 1;

    OperatorTypeEnum operType = getOperType();

    // The operator must be "flipped" as well if the strings were flipped
    if (s == 2) {
      switch (operType) {
        case ITM_LESS:
          operType = ITM_GREATER;
          break;

        case ITM_GREATER:
          operType = ITM_LESS;
          break;

        case ITM_GREATER_EQ:
          operType = ITM_LESS_EQ;
          break;

        case ITM_LESS_EQ:
          operType = ITM_GREATER_EQ;
          break;
      }
    }

    Int32 sLen = attrs[s]->getLength();
    Int32 lLen = attrs[l]->getLength();

    // Lens of unicode strings for this inst should be in num of double-bytes
    if (attrs1Datatype == REC_NCHAR_F_UNICODE) {
      sLen = sLen >> 1;
      lLen = lLen >> 1;
    }

    if (op == PCIT::Op_COMP) {
      // Only different lengths should use COMP pcode instruction.  Regression
      // seen otherwise if COMP instruction used for all cases - 3% in DWP.
      AML aml(PCIT::getMemoryAddressingMode(attrs[0]->getDatatype()),
              PCIT::getMemoryAddressingMode(attrs1Datatype),
              PCIT::getMemoryAddressingMode(attrs2Datatype),
              PCIT::IBIN32S, PCIT::IBIN32S, PCIT::IBIN32S);

      // Smaller string is always the first operand
      OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(),
            attrs[s]->getAtp(), attrs[s]->getAtpIndex(), attrs[s]->getOffset(),
            attrs[l]->getAtp(), attrs[l]->getAtpIndex(), attrs[l]->getOffset(),
            sLen, lLen, (Int32)operType);

      // Add the comparison instruction.
      //
      PCI pci(PCIT::Op_COMP, aml, ol);
      code.append(pci);
    }
    else
    {
      // Same length should use faster pcode

      AML aml(PCIT::getMemoryAddressingMode(attrs[0]->getDatatype()),
              PCIT::getMemoryAddressingMode(attrs1Datatype),
              PCIT::getMemoryAddressingMode(attrs2Datatype),
              PCIT::IBIN32S);

      // Smaller string is always the first operand
      OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(),
            attrs[s]->getAtp(), attrs[s]->getAtpIndex(), attrs[s]->getOffset(),
            attrs[l]->getAtp(), attrs[l]->getAtpIndex(), attrs[l]->getOffset(),
            sLen);

      // Add the comparison instruction.
      //
      PCI pci(op, aml, ol);
      code.append(pci);
    }
  }
  // One of the operand is a varchar
  else if (((op1->getDatatype() == REC_BYTE_V_ASCII) ||
            (op2->getDatatype() == REC_BYTE_V_ASCII)) ||
           ((op1->getDatatype() == REC_NCHAR_V_UNICODE) ||
            (op2->getDatatype() == REC_NCHAR_V_UNICODE)))
  {
    // get the actual length if operands are varchars.
    UInt32 len1 = attrs[1]->getLength();
    UInt32 len2 = attrs[2]->getLength();

    UInt32 comboLen1 = 0, comboLen2 = 0;
    char* comboPtr1 = (char*)&comboLen1;
    char* comboPtr2 = (char*)&comboLen2;

    PCIT::AddressingMode srcAm =
      (((op1->getDatatype() == REC_BYTE_V_ASCII) ||
        (op2->getDatatype() == REC_BYTE_V_ASCII))
         ? PCIT::MATTR5
         : PCIT::MUNIV);

    comboPtr1[0] = (char)attrs[1]->getNullIndicatorLength();
    comboPtr1[1] = (char)attrs[1]->getVCIndicatorLength();
    comboPtr2[0] = (char)attrs[2]->getNullIndicatorLength();
    comboPtr2[1] = (char)attrs[2]->getVCIndicatorLength();

    AML aml(PCIT::getMemoryAddressingMode(attrs[0]->getDatatype()),  // target
            srcAm, srcAm, PCIT::IBIN32S);

    OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(),
          attrs[1]->getAtp(), attrs[1]->getAtpIndex(), attrs[1]->getOffset(),
          attrs[1]->getVoaOffset(), len1, comboLen1,
          attrs[2]->getAtp(), attrs[2]->getAtpIndex(), attrs[2]->getOffset(),
          attrs[2]->getVoaOffset(), len2, comboLen2,
          (Int32)getOperType());

    // Add the comparison instruction.
    //
    PCI pci(PCIT::Op_COMP, aml, ol);
    code.append(pci);
  }
  else
  {
    // Operand 1: memory location of boolean result
    // Operand 2: memory location of 1st argument
    // Operand 3: memory location of 2nd argument
    //
    if (flipOperands) {
      Attributes* tempOp;
      tempOp = op1;
      op1 = op2;
      op2 = tempOp;
    }

    AML aml(PCIT::getMemoryAddressingMode(attrs[0]->getDatatype()),
            PCIT::getMemoryAddressingMode(op1->getDatatype()),
            PCIT::getMemoryAddressingMode(op2->getDatatype()));
    
    OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(),
          op1->getAtp(), op1->getAtpIndex(), op1->getOffset(),
          op2->getAtp(), op2->getAtpIndex(), op2->getOffset());
    
    // Add the comparison instruction.
    //
    PCI pci(op, aml, ol);
    code.append(pci);
  }

  // Add the branch target if necessary.
  //
  if(branchToEnd)
  {
    AML aml; 
    OL ol((Int64)branchToEnd);
    PCI pci(PCIT::Op_TARGET, aml, ol); 
    code.append(pci);
  }

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK; 
};

// ex_aggregate_clause::pCodeGenerate
//
// Does nothing.
//
ex_expr::exp_return_type ex_noop_clause::pCodeGenerate(Space *space, UInt32 f) { 
  //
  PCIList code(space);

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);
  
  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
};

// ex_unlogic_clause::pCodeGenerate
//
// Generate PCI's for the unilogic operations. The PCI's load the operand,
// perform the logical test, and store the tristate result.
//
// IN     : space - memory allocator
// OUT    : 
// RETURN : ex_expr::EXPR_OK is no errors
// EFFECTS: stores pointer to PCodeObject in clause
//
ex_expr::exp_return_type ex_unlogic_clause::pCodeGenerate(Space *space, UInt32 f) {
#ifdef _DEBUG
  // For debugging...
  if(getenv("PCODE_NO_UNLOGIC")) return ex_clause::pCodeGenerate(space, f);
#endif

  // Generate the standard clause->eval PCode for cases that
  // are not handled with more fundamental PCode operations.
  //
  switch(getOperType()) {
  case ITM_IS_NULL:
  case ITM_IS_NOT_NULL:
    break;

  default:
    return ex_clause::pCodeGenerate(space, f);
  }

  // Allocate the code list and get a handle on the attributes
  //
  PCIList code(space);
  AttributesPtr *attrs = getOperand();

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  // Do the test
  //
  switch(getOperType()) {
  case ITM_IS_NULL:
    // If the attribute is not nullable, then it is NOT NULL for sure
    if(!attrs[1]->getNullFlag()) 
      code.append(PCode::storeValue(0, attrs[0], space));
    // Otherwise, actually test it
    else
      code.append(PCode::isNull(attrs[0], attrs[1], space));
    break;

  case ITM_IS_NOT_NULL:
    // If the attribute is not nullable, then it is NOT NULL for sure
    if(!attrs[1]->getNullFlag()) 
      code.append(PCode::storeValue(1, attrs[0], space));
    // Otherwise, actually test it
    else
      code.append(PCode::isNotNull(attrs[0], attrs[1], space));
    break;
  }

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
};

// ex_function_bool::pCodeGenerate
//
// Generate PCI's for the bool operation. The PCI's load the constant
// boolean value.
//
// IN     : space - memory allocator
// OUT    : 
// RETURN : ex_expr::EXPR_OK is no errors
// EFFECTS: stores pointer to PCodeObject in clause
//
ex_expr::exp_return_type ex_function_bool::pCodeGenerate(Space *space, UInt32 f) {
#ifdef _DEBUG
  // For debugging...
  if(getenv("PCODE_NO_BOOL")) return ex_clause::pCodeGenerate(space, f);
#endif

  // Get a handle on the operands
  //
  AttributesPtr *attrs = getOperand();
  // Allocate the code list
  //
  PCIList code(space);

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  Int32 returnValue;
  switch(getOperType()) {
  case ITM_RETURN_TRUE: returnValue = 1; break;
  case ITM_RETURN_FALSE: returnValue = 0; break;
  case ITM_RETURN_NULL: returnValue = -1; break;
  default: return ex_clause::pCodeGenerate(space, f); break;
  }

  // First operand is the memory location of the result
  // Second operand is the immediate value to store in the result
  //
  AML aml(PCIT::MBIN32S, PCIT::IBIN32S);
  OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(),
	returnValue);

  // Add the move instruction.
  //
  PCI pci(PCIT::Op_MOVE, aml, ol);
  code.append(pci);

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

// ExHDPHash::pCodeGenerate
//
// Generate PCI's for the hash operation used by hash partitioning. The PCI's
// load the operands addresses and calls the hash function.
//
// IN     : space - memory allocator
// OUT    :
// RETURN : ex_expr::EXPR_OK is no errors
// EFFECTS: stores pointer to PCodeObject in clause
//
ex_expr::exp_return_type ExHDPHash::pCodeGenerate(Space *space, UInt32 f)
{
  // NOTE - This code is identical to exp_function_hash::pCodeGenerate().
  // Ideally we should be able to cast the this pointer to exp_function_hash
  // and call its pCodeGenerate(), but it doesn't work that way - the vtbl
  // entry isn't updated, and so we end up just calling this pCodeGenerate
  // again, resulting in an infinite loop.  So just duplicate the code below
  // and keep it up-to-date.

#ifdef _DEBUG
  // For debugging...
  if(getenv("PCODE_NO_HASH")) return ex_clause::pCodeGenerate(space, f);
#endif

  AttributesPtr *attrs = getOperand();

  // Don't handle indirect varchars if CQD says so
  if ( attrs[1]->isIndirectVC() && (NOT handleIndirectVC()) )
    return ex_clause::pCodeGenerate(space, f);

  // Allocate the code list and get a handle on the attributes
  //
  PCIList code(space);

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  // Add the null logic if necessary.
  //
  PCIID branchToEnd = PCode::nullBranch(this, code, attrs);

  // The first operand is the memory location of the result.
  // The second operand is the memory location of the data.
  // The third operand is the length of the data.
  //
  PCIType::AddressingMode oper;
  if (attrs[1]->getLength() == 8)
    oper = PCIT::MBIN64S;
  else if (attrs[1]->getLength() == 4)
    oper = PCIT::MBIN32S;
  else if (attrs[1]->getLength() == 2)
    oper = PCIT::MBIN16S;
  else
    oper = PCIT::MPTR32;

  UInt32 flags = NO_FLAGS;

  switch(attrs[1]->getDatatype()) {
  case REC_NUM_BIG_UNSIGNED:
  case REC_NUM_BIG_SIGNED:
  case REC_BIN16_SIGNED:
  case REC_BIN16_UNSIGNED:
  case REC_NCHAR_F_UNICODE:
  case REC_NCHAR_V_UNICODE:
  case REC_NCHAR_V_ANSI_UNICODE:
    flags = SWAP_TWO;
    break; 
  case REC_BIN32_SIGNED:
  case REC_BIN32_UNSIGNED:
  case REC_IEEE_FLOAT32:
    flags = SWAP_FOUR;
    break;
  case REC_BIN64_SIGNED:
  case REC_IEEE_FLOAT64:
    flags = SWAP_EIGHT;
    break;
  case REC_DATETIME:
    {
      oper = PCIT::MPTR32;

      rec_datetime_field start;
      rec_datetime_field end;
      ExpDatetime *datetime = (ExpDatetime*) getOperand(1);
      datetime->getDatetimeFields(attrs[1]->getPrecision(), start, end);
      if(start == REC_DATE_YEAR) {
        flags = SWAP_FIRSTTWO;
      }
      if(end == REC_DATE_SECOND && attrs[1]->getScale() > 0) {
        flags |= SWAP_LASTFOUR;
      }
      
    }
    break; 
  default:
    if(attrs[1]->getDatatype() >= REC_MIN_INTERVAL &&
       attrs[1]->getDatatype() <= REC_MAX_INTERVAL) {

      if (attrs[1]->getLength() == 8)
        flags = SWAP_EIGHT;
      else if (attrs[1]->getLength() == 4)
        flags = SWAP_FOUR;
      else if (attrs[1]->getLength() == 2)
        flags = SWAP_TWO;
      else
        assert(FALSE);
    }
  }

  // handle varchar
  if (attrs[1]->getVCIndicatorLength() > 0)
    {
      UInt32 comboLen = 0;
      char* comboPtr = (char*)&comboLen;

      // Use combo fields for specifying vc/null lengths of both operands
      comboPtr[0] = (char)attrs[1]->getNullIndicatorLength(),
      comboPtr[1] = (char)attrs[1]->getVCIndicatorLength();

      AML aml(PCIT::MBIN32U,
              PCIT::getMemoryAddressingMode(attrs[1]->getDatatype()));

      OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(),attrs[0]->getOffset(),
            attrs[1]->getAtp(), attrs[1]->getAtpIndex(),attrs[1]->getOffset(),
              attrs[1]->getVoaOffset(), attrs[1]->getLength(), comboLen);
      PCI pci(PCIT::Op_HASH, aml, ol);
      code.append( pci);
    }
  else
    {
      AML aml(PCIT::MBIN32U, oper, PCIT::IBIN32S, PCIT::IBIN32S);
      OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(),
            attrs[1]->getAtp(), attrs[1]->getAtpIndex(), attrs[1]->getOffset(),
            flags, attrs[1]->getLength());
      PCI pci(PCIT::Op_HASH, aml, ol);
      code.append( pci);
    }

  // Add the branch target if necessary.
  //
  if(branchToEnd)
    {
      AML aml;
      OL ol((Int64)branchToEnd);
      PCI pci(PCIT::Op_TARGET, aml, ol);
      code.append(pci);
    }

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

// ex_function_hash::pCodeGenerate
//
// Generate PCI's for the hash operation. The PCI's load the operands
// addresses and calls the hash function. 
//
// IN     : space - memory allocator
// OUT    : 
// RETURN : ex_expr::EXPR_OK is no errors
// EFFECTS: stores pointer to PCodeObject in clause
//
ex_expr::exp_return_type ex_function_hash::pCodeGenerate(Space *space, UInt32 f)
{

#ifdef _DEBUG
  // For debugging...
  if(getenv("PCODE_NO_HASH")) return ex_clause::pCodeGenerate(space, f);
#endif

  AttributesPtr *attrs = getOperand();

  // Don't handle indirect varchars if CQD says so
  if ( attrs[1]->isIndirectVC() && (NOT handleIndirectVC()) )
    return ex_clause::pCodeGenerate(space, f);

  // Allocate the code list and get a handle on the attributes
  //
  PCIList code(space);

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  // Add the null logic if necessary.
  //
  PCIID branchToEnd = PCode::nullBranch(this, code, attrs);

  // The first operand is the memory location of the result.
  // The second operand is the memory location of the data.
  // The third operand is the length of the data.
  //
  PCIType::AddressingMode oper;
  if (attrs[1]->getLength() == 8)
    oper = PCIT::MBIN64S;
  else if (attrs[1]->getLength() == 4)
    oper = PCIT::MBIN32S;
  else if (attrs[1]->getLength() == 2)
    oper = PCIT::MBIN16S;
  else
    oper = PCIT::MPTR32;

  // handle varchar
  if (attrs[1]->getVCIndicatorLength() > 0)
    {

      UInt32 comboLen = 0;
      char* comboPtr = (char*)&comboLen;

      // Use combo fields for specifying vc/null lengths of both operands
      comboPtr[0] = (char)attrs[1]->getNullIndicatorLength(),
      comboPtr[1] = (char)attrs[1]->getVCIndicatorLength();

      AML aml(PCIT::MBIN32U,
              PCIT::getMemoryAddressingMode(attrs[1]->getDatatype()));

      OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(),attrs[0]->getOffset(),
            attrs[1]->getAtp(), attrs[1]->getAtpIndex(),attrs[1]->getOffset(),
              attrs[1]->getVoaOffset(), attrs[1]->getLength(), comboLen);
      PCI pci(PCIT::Op_HASH, aml, ol);
      code.append( pci);
    }
  else
    {
      UInt32 flags = ExHDPHash::NO_FLAGS;

      AML aml(PCIT::MBIN32U, oper, PCIT::IBIN32S, PCIT::IBIN32S);
      OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(),
            attrs[1]->getAtp(), attrs[1]->getAtpIndex(), attrs[1]->getOffset(),
            flags, attrs[1]->getLength());
      PCI pci(PCIT::Op_HASH, aml, ol);
      code.append( pci);
    }

  // Add the branch target if necessary.
  //
  if(branchToEnd)
    {
      AML aml; 
      OL ol((Int64)branchToEnd);
      PCI pci(PCIT::Op_TARGET, aml, ol); 
      code.append(pci);
    }

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);
  
  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

// ExHDPHashComb::pCodeGenerate
//
// Generate PCI's for the hash value combination operation. The PCI's load
// the operands, perform the bit scrambling and bitwise xor operations,
// and store the result.
//
// IN     : space - memory allocator
// OUT    :
// RETURN : ex_expr::EXPR_OK is no errors
// EFFECTS: stores pointer to PCodeObject in clause
//
ex_expr::exp_return_type ExHDPHashComb::pCodeGenerate(Space *space, UInt32 f) {
#ifdef _DEBUG
  // For debugging...
  if(getenv("PCODE_NO_HASHCOMB")) return ex_clause::pCodeGenerate(space, f);
#endif

  // Get a handle on the operands
  //
  AttributesPtr *attrs = getOperand();

  // Use the default pCodeGenerate for cases not handles here
  //
  if((attrs[1]->getLength() != 4) || (attrs[2]->getLength() != 4))
    return ex_clause::pCodeGenerate(space, f);

  // Allocate the code list
  //
  PCIList code(space);

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  // Operand 1: memory location of HASHCOMB result
  // Operand 2: memory location of 1st argument
  // Operand 3: memory location of 2nd argument
  //
  AML aml(PCIT::getMemoryAddressingMode(attrs[0]->getDatatype()),
          PCIT::getMemoryAddressingMode(attrs[1]->getDatatype()),
          PCIT::getMemoryAddressingMode(attrs[2]->getDatatype()));
  OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(),
        attrs[1]->getAtp(), attrs[1]->getAtpIndex(), attrs[1]->getOffset(),
        attrs[2]->getAtp(), attrs[2]->getAtpIndex(), attrs[2]->getOffset());

  // Generate the operator
  //
  PCIT::Operation op = PCIT::Op_HASHCOMB;

  // Add the comparison instruction.
  //
  PCI pci(op, aml, ol);
  code.append(pci);

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

// ExHashComb::pCodeGenerate
//
// Generate PCI's for the hash value combination operation. The PCI's load
// the operands, perform the bit scrambling and bitwise xor operations,
// and store the result.
//
// IN     : space - memory allocator
// OUT    : 
// RETURN : ex_expr::EXPR_OK is no errors
// EFFECTS: stores pointer to PCodeObject in clause
//
ex_expr::exp_return_type ExHashComb::pCodeGenerate(Space *space, UInt32 f) {
#ifdef _DEBUG
  // For debugging...
  if(getenv("PCODE_NO_HASHCOMB")) return ex_clause::pCodeGenerate(space, f);
#endif

  // Get a handle on the operands
  //
  AttributesPtr *attrs = getOperand();

  // Use the default pCodeGenerate for cases not handles here
  //
  if((attrs[1]->getLength() != 4) || (attrs[2]->getLength() != 4))
    return ex_clause::pCodeGenerate(space, f);

  // Allocate the code list
  //
  PCIList code(space);

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  // Operand 1: memory location of HASHCOMB result
  // Operand 2: memory location of 1st argument
  // Operand 3: memory location of 2nd argument
  //
  AML aml(PCIT::getMemoryAddressingMode(attrs[0]->getDatatype()),
	  PCIT::getMemoryAddressingMode(attrs[1]->getDatatype()),
	  PCIT::getMemoryAddressingMode(attrs[2]->getDatatype()));
  OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(),
	attrs[1]->getAtp(), attrs[1]->getAtpIndex(), attrs[1]->getOffset(),
	attrs[2]->getAtp(), attrs[2]->getAtpIndex(), attrs[2]->getOffset());

  // Generate the operator
  //
  PCIT::Operation op = PCIT::Op_HASHCOMB;

  // Add the comparison instruction.
  //
  PCI pci(op, aml, ol);
  code.append(pci);

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

// ex_replace_null_clause::pCodeGenerate
//
//
ex_expr::exp_return_type ex_function_replace_null::pCodeGenerate(Space *space, UInt32 f){
#ifdef _DEBUG
  // For debugging...
  if(getenv("PCODE_NO_REPLACE_NULL")) return ex_clause::pCodeGenerate(space, f);
#endif

  // Get a handle on the attributes
  //
  AttributesPtr *attrs = getOperand();

  switch(attrs[0]->getDatatype()) {
  case REC_BIN32_SIGNED:
  case REC_BIN32_UNSIGNED:
  case REC_BIN16_SIGNED:
  case REC_BIN16_UNSIGNED:
    break;

  default:
    return ex_clause::pCodeGenerate(space, f);
  }

  // Allocate the code list and get a handle on the attributes
  //
  PCIList code(space);

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  Attributes *tgt = attrs[0];
  Attributes *op1 = attrs[1];
  Attributes *op2 = attrs[2];

  if (NOT op1->getNullFlag() )           // value is not nullable
  {
    AML aml(PCIT::MBIN8, PCIT::MBIN8,    // these two types must be the same
            PCIT::IBIN32S);

    OL ol(tgt->getAtp(), tgt->getAtpIndex(), tgt->getOffset(),  // result
          op2->getAtp(), op2->getAtpIndex(), op2->getOffset(),  // result to use for non-null 
          tgt->getLength());

    PCI pci(PCIT::Op_MOVE, aml, ol);
    code.append(pci);
  }
  else
  {
    Attributes *op3 = attrs[3];

    // First operand is the result--either second or third operand is null.
    // Second operand is the null indicator of the value to test for nullness
    // Third operand is the immediate value to return if first value is not null
    // Fourth operand is the immediate value to return if first value is null
    // Fifth operand is the length of operands three and four.

     AML aml(PCIT::getMemoryAddressingMode(op2->getDatatype()), 
             PCIT::MATTR3,PCIT::MBIN8,PCIT::MBIN8,PCIT::IBIN32S);

     OL ol(tgt->getAtp(), tgt->getAtpIndex(), tgt->getOffset(), // result
           op1->getAtp(), op1->getAtpIndex(), 
           op1->getNullIndOffset(), op1->getNullBitIndex(),     // value to check
           op2->getAtp(), op2->getAtpIndex(), op2->getOffset(), // result to use for non-null 
           op3->getAtp(), op3->getAtpIndex(), op3->getOffset(), // result to use for null
           op2->getLength());                   // length of replacement value

     // Add the REPLACE NULL instruction.
     //
     PCI pci(PCIT::Op_REPLACE_NULL, aml, ol);
     code.append(pci);
  }
  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);
  setPCIList(code.getList());
  return ex_expr::EXPR_OK; 
}

// Helpers for working with attributes
//
Int32 isSameAttribute(Attributes *attrA, Attributes *attrB) {
  if(attrA->getAtpIndex() != attrB->getAtpIndex()) return 0;
  if(attrA->getOffset() != attrB->getOffset()) return 0;
  return 1;
};
Int32 isConstantAttribute(Attributes *attr) { return attr->getAtpIndex() == 0; };
Int32 isTemporaryAttribute(Attributes *attr) { return attr->getAtpIndex() == 1;};
Int32 isAtpAttribute(Attributes *attr) { return attr->getAtpIndex() > 1; };

ex_expr::exp_return_type ex_arith_clause::unaryArithPCodeGenerate
(Space *space, UInt32 f) 
{
  if (getNumOperands() != 2)
    return ex_expr::EXPR_ERROR;

  AttributesPtr *attrs = getOperand();

  switch(getInstruction()) {

  case NEGATE_BOOLEAN:
    break;

  default:
    return ex_clause::pCodeGenerate(space, f);
  }

  // Allocate the code list and get a handle on the attributes
  //
  PCIList code(space);

  // Don't get mixed up in interval conversions. The precision for
  // interval conversions is not set up correctly.
  //
  Attributes *dst = attrs[0];
  Attributes *op1 = attrs[1];
  
  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  // Construct the operands.
  //
  OL ol2(dst->getAtp(), dst->getAtpIndex(), dst->getOffset(),  
	 op1->getAtp(), op1->getAtpIndex(), op1->getOffset());

  AML aml2(PCIT::getMemoryAddressingMode(dst->getDatatype()),
	   PCIT::getMemoryAddressingMode(op1->getDatatype()));

  // Construct the operation.
  //
  AML *aml = NULL;
  OL *ol = NULL;
  PCIT::Operation inst = PCIT::Op_OPDATA;  // prevent uninitialized var warning
 
  switch(getInstruction()) 
    {
    case NEGATE_BOOLEAN:
      aml = &aml2;
      ol = &ol2;
      inst = PCIT::Op_NEG;
      break;
    }

  // Add the null logic if necessary.
  //
  PCIID branchToEnd = PCode::nullBranch(this, code, attrs);

  // Add the instruction.
  //
  PCI pci(inst, *aml, *ol);
  code.append(pci);

  // Add the branch target if necessary.
  //
  if(branchToEnd)
  {
    AML aml; 
    OL ol((Int64)branchToEnd);
    PCI pci(PCIT::Op_TARGET, aml, ol); 
    code.append(pci);
  }

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

// ex_arith_clause::pCodeGenerate
//
// Generate PCI's for the arithematic operations. The PCI's load the operands,
// do the arithematic, and then store the result.
//
// IN     : space - memory allocator
// OUT    : 
// RETURN : ex_expr::EXPR_OK is no errors
// EFFECTS: stores pointer to PCodeObject in clause
//
ex_expr::exp_return_type ex_arith_clause::pCodeGenerate(Space *space, UInt32 f) {
#ifdef _DEBUG
  // For debugging...
  if(getenv("PCODE_NO_ARITH")) return ex_clause::pCodeGenerate(space, f);
#endif

  // if unary arith operator, generator unary pcode.
  if (getNumOperands() == 2) // 1 result and 1 operand
    {
      return unaryArithPCodeGenerate(space, f);
    }

  // Generate the standard clause->eval PCode for cases that
  // are not handled with more fundamental PCode operations.
  //
  AttributesPtr *attrs = getOperand();
  switch(getInstruction()) {
  case ADD_BIN16S_BIN16S_BIN16S:
  case ADD_BIN32S_BIN32S_BIN32S:
  case ADD_BIN64S_BIN64S_BIN64S:
  case SUB_BIN16S_BIN16S_BIN16S:
  case SUB_BIN32S_BIN32S_BIN32S:
  case SUB_BIN64S_BIN64S_BIN64S:
  case MUL_BIN16S_BIN16S_BIN16S:
  case MUL_BIN32S_BIN32S_BIN32S:
  case MUL_BIN64S_BIN64S_BIN64S:
    break;

  case ADD_BIN16S_BIN16S_BIN32S:
  case ADD_BIN16S_BIN32S_BIN32S:
  case ADD_BIN32S_BIN16S_BIN32S:
  case ADD_BIN32S_BIN64S_BIN64S:
  case ADD_BIN64S_BIN32S_BIN64S:

  case SUB_BIN16S_BIN16S_BIN32S:
  case SUB_BIN16S_BIN32S_BIN32S:
  case SUB_BIN32S_BIN16S_BIN32S:

  case MUL_BIN16S_BIN16S_BIN32S:
  case MUL_BIN16S_BIN32S_BIN32S:
  case MUL_BIN32S_BIN16S_BIN32S:
    break;

  case MUL_BIN16S_BIN32S_BIN64S:
  case MUL_BIN32S_BIN16S_BIN64S:
  case MUL_BIN32S_BIN32S_BIN64S:
    break;

  case DIV_BIN64S_BIN64S_BIN64S:
    break;

  case DIV_BIN64S_BIN64S_BIN64S_ROUND:
    break;

  case ADD_FLOAT64_FLOAT64_FLOAT64:
  case SUB_FLOAT64_FLOAT64_FLOAT64:
  case MUL_FLOAT64_FLOAT64_FLOAT64:
  case DIV_FLOAT64_FLOAT64_FLOAT64:
    break;

  case SUB_COMPLEX:
  case ADD_COMPLEX:
  case MUL_COMPLEX:
    if (attrs[0]->getClassID() != Attributes::BigNumID)
      return ex_clause::pCodeGenerate(space, f);
    break;

  default:
    return ex_clause::pCodeGenerate(space, f);
  }

  // Allocate the code list and get a handle on the attributes
  //
  PCIList code(space);

  // Don't get mixed up in interval conversions. The precision for
  // interval conversions is not set up correctly.
  //
  Attributes *dst = attrs[0];
  Attributes *op1 = attrs[1];
  Attributes *op2 = attrs[2];

  if((dst->getDatatype() < REC_MIN_NUMERIC) ||
     (dst->getDatatype() > REC_MAX_NUMERIC) ||
     (op1->getDatatype() < REC_MIN_NUMERIC) ||
     (op1->getDatatype() > REC_MAX_NUMERIC))
    return ex_clause::pCodeGenerate(space, f);
  
  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  // Construct the operands.
  //

  OL ols(dst->getAtp(), dst->getAtpIndex(), dst->getOffset(),  
	 op1->getAtp(), op1->getAtpIndex(), op1->getOffset(), 
	 op2->getAtp(), op2->getAtpIndex(), op2->getOffset());

  OL olsFlipped(
         dst->getAtp(), dst->getAtpIndex(), dst->getOffset(),
         op2->getAtp(), op2->getAtpIndex(), op2->getOffset(),
         op1->getAtp(), op1->getAtpIndex(), op1->getOffset());

  OL olx(dst->getAtp(), dst->getAtpIndex(), dst->getOffset(), 
	 dst->getPrecision(),
	 op1->getAtp(), op1->getAtpIndex(), op1->getOffset(), 
	 op1->getPrecision(),
	 op2->getAtp(), op2->getAtpIndex(), op2->getOffset(),
	 op2->getPrecision());

  OL olb(dst->getAtp(), dst->getAtpIndex(), dst->getOffset(),
         op1->getAtp(), op1->getAtpIndex(), op1->getOffset(),
         op2->getAtp(), op2->getAtpIndex(), op2->getOffset(),
         dst->getLength());

  OL olb2(dst->getAtp(), dst->getAtpIndex(), dst->getOffset(),
          op1->getAtp(), op1->getAtpIndex(), op1->getOffset(),
          op2->getAtp(), op2->getAtpIndex(), op2->getOffset(),
          dst->getLength(), op1->getLength(), op2->getLength());

  AML amlb(PCIT::MBIGS, PCIT::MBIGS, PCIT::MBIGS, PCIT::IBIN32S);
  AML amlb2(PCIT::MBIGS, PCIT::MBIGS, PCIT::MBIGS,
            PCIT::IBIN32S, PCIT::IBIN32S, PCIT::IBIN32S);

  // OL for division using rounding. Need to pass in rounding mode and
  // if this rounding division is being done to downscale.
  // Lowest byte(byte 4) in roundingInfo is rounding mode.
  // Rightmost bit in byte 3 is divToDownscale.
  Lng32 roundingInfo;
  roundingInfo = (Lng32)arithRoundingMode_;
  if (getDivToDownscale())
    roundingInfo |= 0x100;

  OL ol_rd(dst->getAtp(), dst->getAtpIndex(), (Int32)dst->getOffset(),
	   op1->getAtp(), op1->getAtpIndex(), (Int32)op1->getOffset(), 
	   op2->getAtp(), op2->getAtpIndex(), (Int32)op2->getOffset(),
	   roundingInfo);

  // Construct the addressing modes.
  //
  // Construct the addressing modes.
  //
  AML amls(PCIT::getMemoryAddressingMode(dst->getDatatype()),
	   PCIT::getMemoryAddressingMode(op1->getDatatype()),
	   PCIT::getMemoryAddressingMode(op2->getDatatype()));

  AML amlsFlipped(
           PCIT::getMemoryAddressingMode(dst->getDatatype()),
           PCIT::getMemoryAddressingMode(op2->getDatatype()),
           PCIT::getMemoryAddressingMode(op1->getDatatype()));

  AML amlx(PCIT::getMemoryAddressingMode(dst->getDatatype()),
	   PCIT::IBIN32S,
	   PCIT::getMemoryAddressingMode(op1->getDatatype()),
	   PCIT::IBIN32S,
	   PCIT::getMemoryAddressingMode(op2->getDatatype()),
   	   PCIT::IBIN32S);

  // AML for division using rounding. Need to pass in rounding mode.
  AML aml_rd(PCIT::getMemoryAddressingMode(dst->getDatatype()),
	     PCIT::getMemoryAddressingMode(op1->getDatatype()),
	     PCIT::getMemoryAddressingMode(op2->getDatatype()),
	     PCIT::IBIN32S);

  // Construct the operation.
  //
  AML *aml = NULL;
  OL *ol = NULL;
  PCIT::Operation inst = PCIT::Op_OPDATA;  // prevent uninitialized var warning
  switch(getInstruction()) 
    {
    case ADD_BIN32S_BIN16S_BIN32S:
    case ADD_BIN64S_BIN32S_BIN64S:
      // Normalize instructions so smaller operand comes first
      aml = &amlsFlipped;
      ol = &olsFlipped;
      inst = PCIT::Op_ADD;
      break;

    case ADD_BIN16S_BIN16S_BIN16S:
    case ADD_BIN32S_BIN32S_BIN32S:
    case ADD_BIN64S_BIN64S_BIN64S:
    case ADD_BIN16S_BIN16S_BIN32S:
    case ADD_BIN16S_BIN32S_BIN32S:
    case ADD_BIN32S_BIN64S_BIN64S:
    case ADD_FLOAT64_FLOAT64_FLOAT64:
      aml = &amls;
      ol = &ols;
      inst = PCIT::Op_ADD;
      break;
      
    case SUB_BIN16S_BIN16S_BIN16S:
    case SUB_BIN32S_BIN32S_BIN32S:
    case SUB_BIN64S_BIN64S_BIN64S:
    case SUB_BIN16S_BIN16S_BIN32S:
    case SUB_BIN16S_BIN32S_BIN32S:
    case SUB_BIN32S_BIN16S_BIN32S:
    case SUB_FLOAT64_FLOAT64_FLOAT64:
      aml = &amls;
      ol = &ols;
      inst = PCIT::Op_SUB;
      break;
      
    case MUL_BIN32S_BIN16S_BIN32S:
    case MUL_BIN32S_BIN16S_BIN64S:
      // Normalize instructions so smaller operand comes first
      aml = &amlsFlipped;
      ol = &olsFlipped;
      inst = PCIT::Op_MUL;
      break;

    case MUL_BIN16S_BIN16S_BIN16S:
    case MUL_BIN32S_BIN32S_BIN32S:
    case MUL_BIN64S_BIN64S_BIN64S:
    case MUL_BIN16S_BIN16S_BIN32S:
    case MUL_BIN16S_BIN32S_BIN32S:
    case MUL_BIN16S_BIN32S_BIN64S:
    case MUL_BIN32S_BIN32S_BIN64S:
    case MUL_FLOAT64_FLOAT64_FLOAT64:
      aml = &amls;
      ol = &ols;
      inst = PCIT::Op_MUL;
      break;

    case DIV_BIN64S_BIN64S_BIN64S:
    case DIV_FLOAT64_FLOAT64_FLOAT64:
      aml = &amls;
      ol = &ols;
      inst = PCIT::Op_DIV;
      break;

    case DIV_BIN64S_BIN64S_BIN64S_ROUND:
      aml = &aml_rd;
      ol = &ol_rd;
      inst = PCIT::Op_DIV_ROUND;
      break;

    case SUB_COMPLEX:
      aml = &amlb;
      ol = &olb;
      inst = PCIT::Op_SUB;
      break;

    case ADD_COMPLEX:
      aml = &amlb;
      ol = &olb;
      inst = PCIT::Op_ADD;
      break;

    case MUL_COMPLEX:
      aml = &amlb2;
      ol = &olb2;
      inst = PCIT::Op_MUL;
      break;

#if 0
    case MUL_COMPLEX:
      aml = &amlx;
      ol = &olx;
      inst = PCIT::Op_MUL;
      break;      
#endif

    case DIV_COMPLEX:
      aml = &amlx;
      ol = &olx;
      inst = PCIT::Op_DIV;
      break;      

    }

  // Add the null logic if necessary.
  //
  PCIID branchToEnd = PCode::nullBranch(this, code, attrs);

  // Add the instruction.
  //
  PCI pci(inst, *aml, *ol);
  code.append(pci);

  // Add the branch target if necessary.
  //
  if(branchToEnd)
  {
    AML aml; 
    OL ol((Int64)branchToEnd);
    PCI pci(PCIT::Op_TARGET, aml, ol); 
    code.append(pci);
  }

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

// ex_arith_sum_clause::pCodeGenerate
//
// Generate PCI's for the sum operation.
//
// IN     : space - memory allocator
// OUT    : 
// RETURN : ex_expr::EXPR_OK is no errors
// EFFECTS: stores pointer to PCodeObject in clause
//
ex_expr::exp_return_type ex_arith_sum_clause::pCodeGenerate(Space *space, UInt32 f) {
#ifdef _DEBUG
  // For debugging...
  if(getenv("PCODE_NO_ARITH_SUM")) return ex_clause::pCodeGenerate(space, f);
#endif
  
  // Allocate the code list and get a handle on the attributes
  //
  AttributesPtr *attrs = getOperand();

  // Generate the standard clause->eval PCode for cases that
  // are not handled with more fundamental PCode operations.
  // Since this is arith_sum, only add operations should appear.
  //
  switch(getInstruction()) {
  case ADD_BIN32S_BIN32S_BIN32S:
  case ADD_BIN64S_BIN64S_BIN64S:
    break;

  case ADD_FLOAT64_FLOAT64_FLOAT64:
    break;

  case ADD_COMPLEX:
    if (attrs[0]->getClassID() != Attributes::BigNumID)
      return ex_clause::pCodeGenerate(space, f);
    break;

  default:
    return ex_clause::pCodeGenerate(space, f);
  }

// Don't get mixed up in interval conversions. The precision for
  // interval conversions is not set up correctly.
  //
  Attributes *dst = attrs[0];
  Attributes *op1 = attrs[1];
  Attributes *op2 = attrs[2];

  if((dst->getDatatype() < REC_MIN_NUMERIC) ||
     (dst->getDatatype() > REC_MAX_NUMERIC) ||
     (op1->getDatatype() < REC_MIN_NUMERIC) ||
     (op1->getDatatype() > REC_MAX_NUMERIC))
    return ex_clause::pCodeGenerate(space, f);

  if (! isAugmentedAssignOperation())
     return ex_clause::pCodeGenerate(space, f);

  // The result should be the same as one of the operands.
  //
  Int32 firstOperand = isSameAttribute(dst, op1);
  Int32 secondOperand = isSameAttribute(dst, op2);
  
  if(!firstOperand && !secondOperand)
    return ex_clause::pCodeGenerate(space, f);

  // The offset must be positive for PCODE to work.
  //
  if(dst->getOffset() == ExpOffsetMax)
    return ex_clause::pCodeGenerate(space, f);

  // The sum's are implemented in PCODE by the increment operation.
  // So, generate the appropriate increment operation.
  //
  Attributes *attrOp = (firstOperand ? op2 : op1);

  // If the sum expression is nullable, then the sum must also be
  // nullable for PCODE to work.
  //
  Int32 nullable = attrOp->getNullFlag();
  if(nullable && !dst->getNullFlag())
    return ex_clause::pCodeGenerate(space, f);

  // Both operands must be the same type as well.
  //
  PCIT::AddressingMode amA 
    = PCIT::getMemoryAddressingMode(dst->getDatatype());
  PCIT::AddressingMode amB 
    = PCIT::getMemoryAddressingMode(attrOp->getDatatype());
  if(amA != amB) 
    return ex_clause::pCodeGenerate(space, f);

  // Allocate the code list and get a handle on the attributes
  //
  PCIList code(space);

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  // Based on the type of addition and whether the operand is nullable or
  // not, choose the appropriate operand number and size for the 
  // increment operation.
  //
  if (getInstruction() == ADD_COMPLEX)
  {
    AML aml(PCIT::MATTR3, PCIT::MATTR3, PCIT::IBIN32S, PCIT::MBIGS,
            PCIT::MBIGS, PCIT::IBIN32S);
    OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(),
            attrs[0]->getNullIndOffset(), attrs[0]->getNullBitIndex(),
          attrOp->getAtp(), attrOp->getAtpIndex(),
            attrOp->getNullIndOffset(), attrOp->getNullBitIndex(),
          ((nullable) ? 1 : 0),
          attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(),
          attrOp->getAtp(), attrOp->getAtpIndex(), attrOp->getOffset(),
          attrs[0]->getLength());
    PCI pci(PCIT::Op_SUM, aml, ol);
    code.append(pci);
  }
  else if(!nullable)
    {
      AML aml(amA, amB);
      OL ol(dst->getAtp(), dst->getAtpIndex(), (Int32)dst->getOffset(), 
	    attrOp->getAtp(), attrOp->getAtpIndex(), (Int32)attrOp->getOffset());

      PCI pci(PCIT::Op_SUM, aml, ol);
      code.append(pci);
    }
  else 
    {
    AML aml(PCIT::MATTR3, PCIT::MATTR3, PCIT::IBIN32S, amA, amB);
    OL ol(dst->getAtp(), dst->getAtpIndex(), (Int32)dst->getNullIndOffset(),
          (Int32)dst->getNullBitIndex(),
          attrOp->getAtp(), attrOp->getAtpIndex(), attrOp->getNullIndOffset(),
          (Int32)attrOp->getNullBitIndex(),
          ((nullable) ? 1 : 0),
          dst->getAtp(), dst->getAtpIndex(), dst->getOffset(), 
          attrOp->getAtp(), attrOp->getAtpIndex(), attrOp->getOffset());
      PCI pci(PCIT::Op_SUM, aml, ol);
      code.append(pci);
    }

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

// ex_arith_count_clause::pCodeGenerate
//
// Generate PCI's for the sum operation.
//
// IN     : space - memory allocator
// OUT    : 
// RETURN : ex_expr::EXPR_OK is no errors
// EFFECTS: stores pointer to PCodeObject in clause
//
ex_expr::exp_return_type ex_arith_count_clause::pCodeGenerate(Space *space, UInt32 f) {
  return ex_clause::pCodeGenerate(space, f);
};

static void computeBounds(Attributes *attr, Int64 &lowBounds, 
			  UInt64 &highBounds, Int32 &bigBounds, Int32 &isSigned)
{
const  UInt64 decimalPrecision[] = {
    0,
    9, 
    99, 
    999, 
    9999, 
    99999, 
    999999, 
    9999999, 
    99999999, 
    999999999,
    9999999999LL, 
    99999999999LL, 
    999999999999LL, 
    9999999999999LL,
    99999999999999LL,
    999999999999999LL,
    9999999999999999LL,
    99999999999999999LL,
    999999999999999999LL,
    4999999999999999999LL,
    9999999999999999999ULL
  };

const  Int32 bpPrecision[] = { 0, 1, 3, 7, 15, 31, 63, 127, 255, 511,
			     1023, 2047, 4095, 8191, 16483, 32767,
			     65535, 65535 };
    

  // By default, unsigned ints.
  //
  isSigned = 0;
  bigBounds = 0;
  
  // Decimals have precision > 0 (Except for BPINTs).
  //
  if((attr->getPrecision() > 0) && (attr->getDatatype() != REC_BPINT_UNSIGNED))
    {
      isSigned = 1;
      switch(attr->getDatatype())
	{
	case REC_BIN8_UNSIGNED:
	  isSigned = 0;
	  break;

	case REC_BIN16_UNSIGNED:
	  isSigned = 0;
	  break;

	case REC_BIN32_UNSIGNED:
	  isSigned = 0;
	  break;

	case REC_BIN64_SIGNED:
	  bigBounds = 1;
          break;

	case REC_BIN64_UNSIGNED:
          isSigned = 0;
	  bigBounds = 1;
          break;
	}

      lowBounds = 0;
      if (isSigned) 
        lowBounds = - decimalPrecision[attr->getPrecision()];
      highBounds = decimalPrecision[attr->getPrecision()];
    }
  // Binarys have precision = 0
  //
  else
    {
      switch(attr->getDatatype())
	{
	case REC_BPINT_UNSIGNED:
	  lowBounds = 0;
	  highBounds = bpPrecision[attr->getPrecision()];
	  break;

	case REC_BIN8_SIGNED:
	  lowBounds = CHAR_MIN;
	  highBounds = CHAR_MAX;
	  isSigned = 1;
	  break;

	case REC_BIN8_UNSIGNED:
	  lowBounds = 0;
	  highBounds = UCHAR_MAX;
	  break;

	case REC_BIN16_SIGNED:
	  lowBounds = SHRT_MIN;
	  highBounds = SHRT_MAX;
	  isSigned = 1;
	  break;

	case REC_BIN16_UNSIGNED:
	  lowBounds = 0;
	  highBounds = USHRT_MAX;
	  break;

	case REC_BIN32_SIGNED:
	  lowBounds = INT_MIN;
	  highBounds = INT_MAX;
	  isSigned = 1;
	  break;

	case REC_BIN32_UNSIGNED:
	  lowBounds = 0;
	  highBounds = UINT_MAX;
	  break;

	case REC_BIN64_SIGNED:
	  bigBounds = 1;
	  // lowBounds = -(Int64)9223372036854775808;
	  lowBounds = (Int64)LLONG_MIN;
	  // highBounds = (Int64)9223372036854775807;
	  highBounds = (Int64)LLONG_MAX;
	  isSigned = 1;
	  break;

	case REC_BIN64_UNSIGNED:
	  bigBounds = 1;
	  lowBounds = 0;
	  highBounds = ULLONG_MAX;
	  break;

	}
    }
}


// ex_conv_clause::pCodeGenerate
//
// Generate PCI's for the conversion operations. The PCI's load the operand,
// do the conversion, and then store the result.
//
// IN     : space - memory allocator
// OUT    : 
// RETURN : ex_expr::EXPR_OK is no errors
// EFFECTS: stores pointer to PCodeObject in clause
//
ex_expr::exp_return_type ex_conv_clause::pCodeGenerate(Space *space, UInt32 f) {
#ifdef _DEBUG
  // For debugging...
  if(getenv("PCODE_NO_CONV")) return ex_clause::pCodeGenerate(space, f);
#endif
  if( ( flags_ & CONV_TO_NULL_WHEN_ERROR ) != 0 ) return ex_clause::pCodeGenerate(space, f);

  // If there is a third argument to the convert, it indicates that
  // a data conversion flag is present. This is used for converting keys
  // where instead of an error, the expression needs to set the
  // data conversion flag and the caller will insert either the
  // min or max value instead of issuing an error. PCODE currently
  // does not handle this case.
  // 
  // Also, if there's a potential for a string truncation error,
  // use the hybrid expression evaluator.
  //
  if ( (getNumOperands() > 2) || getCheckTruncationFlag() )
    return ex_clause::pCodeGenerate(space, f);

  // CIF bulk move -- no pcode yet --there will be later

  if ( lastVOAoffset_>0)
    {
      if (getenv("NO_CIF_BULK_MOVE_PCODE"))
        return ex_clause::pCodeGenerate(space, f);
    }

  // Get a handle on the operands.
  //
  AttributesPtr *attrs = getOperand();
  Attributes *dst = attrs[0];
  Attributes *src = attrs[1];

  if (srcIsVarcharPtr())
    {
      if (NOT ((dst->getDatatype() == REC_BIN32_SIGNED) ||
	       (dst->getDatatype() == REC_BIN64_SIGNED) ||
	       (dst->getDatatype() == REC_FLOAT32) ||
	       (dst->getDatatype() == REC_BYTE_V_ASCII)))
	return ex_clause::pCodeGenerate(space, f);

      if (dst->getDatatype() == REC_BYTE_V_ASCII)
	{
	  if(getenv("NO_PCODE_VC_PTR_CONV"))
	    return ex_clause::pCodeGenerate(space, f);
	}

      if (((dst->getDatatype() == REC_BIN32_SIGNED) ||
	   (dst->getDatatype() == REC_BIN64_SIGNED)) &&
	  (dst->getScale() > 0))
	{
	  return ex_clause::pCodeGenerate(space, f);
	}
    }
  else if ((dst->getDatatype() == REC_BOOLEAN) &&
           (src->getDatatype() == REC_BOOLEAN))
    {
      // boolean conversions are pcode supported.
    }
  else if (((dst->getDatatype() < REC_MIN_NUMERIC) ||
            (dst->getDatatype() > REC_MAX_NUMERIC) ||
            (src->getDatatype() < REC_MIN_NUMERIC) ||
            (src->getDatatype() > REC_MAX_NUMERIC)) &&
           (((dst->getDatatype() != REC_BYTE_F_ASCII) ||
             (src->getDatatype() != REC_BYTE_F_ASCII)) &&
            ((dst->getDatatype() != REC_BYTE_V_ASCII) ||
             (src->getDatatype() != REC_BYTE_V_ASCII)) &&
            ((dst->getDatatype() != REC_BYTE_F_ASCII) ||
             (src->getDatatype() != REC_BYTE_V_ASCII)) &&
            ((dst->getDatatype() != REC_BYTE_V_ASCII) ||
             (src->getDatatype() != REC_BYTE_F_ASCII)) &&
            ((dst->getDatatype() != REC_NCHAR_V_UNICODE) ||
             (src->getDatatype() != REC_NCHAR_V_UNICODE)) &&
            ((dst->getDatatype() != REC_NCHAR_F_UNICODE) ||
             (src->getDatatype() != REC_NCHAR_F_UNICODE)) &&
            ((dst->getDatatype() != REC_DATETIME) ||
             (src->getDatatype() != REC_DATETIME))))
    return ex_clause::pCodeGenerate(space, f);
  
  // Generate the standard clause->eval PCode for particular
  // conversions that are not handled with more fundamental PCode 
  // operations.
  //
  switch(getInstruction()) {
  case CONV_BPINTU_BPINTU:
    
  case CONV_BIN16S_BIN16S: case CONV_BIN16S_BIN16U: 
  case CONV_BIN16S_BIN32S: case CONV_BIN16S_BIN32U: 
  case CONV_BIN16S_BIN64S: 
    /*case CONV_BIN16S_FLOAT32: case CONV_BIN16S_FLOAT64:*/
    
  case CONV_BIN16U_BIN16S: case CONV_BIN16U_BIN16U: 
  case CONV_BIN16U_BIN32S: case CONV_BIN16U_BIN32U: 
  case CONV_BIN16U_BIN64S: 
    /*case CONV_BIN16U_FLOAT32: case CONV_BIN16U_FLOAT64:*/
    
  case CONV_BIN32S_BIN16S: case CONV_BIN32S_BIN16U:
  case CONV_BIN32S_BIN32S: case CONV_BIN32S_BIN32U:
  case CONV_BIN32S_BIN64S: 
    /*case CONV_BIN32S_FLOAT32: case CONV_BIN32S_FLOAT64:*/
    
  case CONV_BIN32U_BIN16S: case CONV_BIN32U_BIN16U:
  case CONV_BIN32U_BIN32S: case CONV_BIN32U_BIN32U:
  case CONV_BIN32U_BIN64S: 
    /*case CONV_BIN32U_FLOAT32: case CONV_BIN32U_FLOAT64:*/
    
  case CONV_BIN64S_BIN16S: /*case CONV_BIN64S_BIN16U:*/
  case CONV_BIN64S_BIN32S: case CONV_BIN64S_BIN32U:
  case CONV_BIN64S_BIN64S: 
  case CONV_BIN64U_BIN64U: 

  case CONV_BIN64S_BIN64U:
    //  case CONV_BIN64U_BIN64S: // not yet supported
    break;
    /*case CONV_BIN64S_FLOAT32: case CONV_BIN64S_FLOAT64:*/
    
    /*case CONV_FLOAT32_BIN16U: case CONV_FLOAT32_BIN16S:
      case CONV_FLOAT32_BIN32U: case CONV_FLOAT32_BIN32S:*/
    /*case CONV_FLOAT32_BIN64S:*/ 
    
    /*case CONV_FLOAT64_BIN16U: case CONV_FLOAT64_BIN16S:
      case CONV_FLOAT64_BIN32U: case CONV_FLOAT64_BIN32S:*/
    /*case CONV_FLOAT64_BIN64S:*/ 
    /*case CONV_FLOAT64_FLOAT32:*/
    
  case CONV_DECS_BIN64S:
    {
      // not enabled for NEO CA
      return ex_clause::pCodeGenerate(space, f);
      
      if (attrs[0]->getScale() != attrs[1]->getScale())
	return ex_clause::pCodeGenerate(space, f);
    }
  break;

  case CONV_BIN16S_BIGNUM:
  case CONV_BIN32S_BIGNUM:
  case CONV_BIN64S_BIGNUM:
    break;

  case CONV_BIGNUM_BIN64S:
    if ((src->getPrecision() > dst->getPrecision()) &&
        (dst->getPrecision() > 0))
      return ex_clause::pCodeGenerate(space, f);
    break;

  case CONV_BIGNUM_BIGNUM:
    if (src->getPrecision() > dst->getPrecision())
      return ex_clause::pCodeGenerate(space, f);
    break;

  case CONV_SIMPLE_TO_COMPLEX:
    if ((dst->getDatatype() != REC_NUM_BIG_SIGNED) ||
        (src->getDatatype() != REC_BIN64_SIGNED) ||
        (src->getPrecision() > dst->getPrecision()))
      return ex_clause::pCodeGenerate(space, f);
    break;

  case CONV_FLOAT32_FLOAT32:
  case CONV_FLOAT64_FLOAT64:
  break;
  
  case CONV_BIN64S_FLOAT64:
  case CONV_BIN32S_FLOAT64:
  case CONV_BIN16S_FLOAT64:
  case CONV_FLOAT32_FLOAT64:
  break;

  case CONV_ASCII_F_F:
  case CONV_ASCII_V_V:
  case CONV_ASCII_F_V:
  case CONV_ASCII_V_F:
    {
      if (NOT srcIsVarcharPtr())
	{
	  if (!requiresNoConvOrVal(src->getLength(),
				   src->getPrecision(),
				   src->getScale(),
				   dst->getLength(),
				   dst->getPrecision(),
				   dst->getScale(),
				   getInstruction()
				   ))
	    return ex_clause::pCodeGenerate(space, f);
	}
    }
    // fall through to next case

  case CONV_UNICODE_F_F:
  case CONV_UNICODE_V_V:
  case CONV_DATETIME_DATETIME:
  case CONV_DECS_DECS:
  case CONV_ASCII_BIN32S:
  case CONV_ASCII_BIN64S:
  case CONV_ASCII_FLOAT32:
    {
      if (getInstruction() == CONV_DECS_DECS)
	{
	  // not enabled for NEO CA
	  return ex_clause::pCodeGenerate(space, f);
	}
      
      // Only handle equal length strings.
      //
      if (NOT srcIsVarcharPtr())
	{
	  if (dst->getLength() < src->getLength())
	    return ex_clause::pCodeGenerate(space, f);
	}

      if ((getInstruction() == CONV_ASCII_V_V) ||
	  (getInstruction() == CONV_DATETIME_DATETIME) ||
	  (getInstruction() == CONV_DECS_DECS))
	{
	  if ((getInstruction() == CONV_DATETIME_DATETIME) ||
	      (getInstruction() == CONV_DECS_DECS))
	    {
	      if ((dst->getPrecision() != src->getPrecision()) ||
		  (dst->getScale() != src->getScale()))
		return ex_clause::pCodeGenerate(space, f);
	    }
	}

      PCIList code(space);
      
      // Generate pre clause PCI's
      //
      PCode::preClausePCI(this, code);
      
      // handle NULLs (may jump)
      PCIID nullJmp = PCode::nullBranch(this, code, attrs);
      
      // copy the value
      switch(getInstruction())
      {
        case CONV_ASCII_V_V:
        case CONV_UNICODE_V_V:
          {
	    if (NOT srcIsVarcharPtr())
	      {
		code.append(PCode::moveVarcharValue(dst, src, space));
	      }
	    else
	      {
		code.append(PCode::convertVarcharPtrToTarget(dst, src, space));
	      }
          }
        break;

        case CONV_ASCII_F_V:
          {
            code.append(PCode::moveFixedVarcharValue(dst, src, space));
          }
        break;

        case CONV_ASCII_V_F:
          {
            code.append(PCode::moveVarcharFixedValue(dst, src, space));
          }
        break;

      case CONV_ASCII_BIN32S:
      case CONV_ASCII_BIN64S:
      case CONV_ASCII_FLOAT32:
	{
	  code.append(PCode::convertVarcharPtrToTarget(dst, src, space));
	}
	break;

      default:
	{
            if(lastVOAoffset_>0 )
            {//cif bulk move
              code.append(PCode::copyVarRow( dst,
                                              src,
                                              lastVOAoffset_,
                                              lastVcIndicatorLength_,
                                              lastNullIndicatorLength_,
                                              alignment_,
                                              space));
            }
            else
           {
	    code.append(PCode::moveValue(dst, src, space));
           }
          }
      }
      
      // we'll jump here if the value is NULL
      if (nullJmp)
	{
	  AML aml;
	  OL ol((Int64) nullJmp);
	  PCI pci(PCIT::Op_TARGET, aml, ol);
	  code.append(pci);
	}
      
//      Update row length at the end of the loop. Move it to
//        ex_expr::pCodeGenerate.
//      if(getNumOperands() > 0) 
//	code.append(PCode::updateRowLen(attrs[0], space, f));
      
      // Finish up and return
      //
      PCode::postClausePCI(this, code);
      setPCIList(code.getList());
      return ex_expr::EXPR_OK;
    }
  break;
  
  case CONV_BIN8S_BIN8S:
  case CONV_BIN8U_BIN8U:
  case CONV_BIN8S_BIN16S:
  case CONV_BIN8U_BIN16U:
  case CONV_BIN8S_BIN32S:
  case CONV_BIN8U_BIN32U:
  case CONV_BIN8S_BIN64S:
  case CONV_BIN8U_BIN64U:
  case CONV_BIN16S_BIN8S:
  case CONV_BIN16U_BIN8U:
  case CONV_BIN16S_BIN8U:  
  case CONV_BIN16U_BIN8S:
    break;

  case CONV_BOOL_BOOL:
    break;

  default:
    return ex_clause::pCodeGenerate(space, f);
  }
  
#ifdef _DEBUG
  // Allow 64 -> 64 with differing scale
  //
  if(getInstruction() == CONV_BIN64S_BIN64S)
    {
      if(getenv("PCODE_NO_INT64_SCALE_CONV"))
        {
          return ex_clause::pCodeGenerate(space, f);
        }
    }
#endif
  
  // Generate the standard clause->eval PCODE if NULLS need to be handled
  // specially and any of the inputs are nullable.
  //
  if(isAnyInputNullable() && isNullRelevant() && !isNullInNullOut())
    return ex_clause::pCodeGenerate(space, f);

  // If we get here, we have decided that we should generate PCI's to handle
  // the particular conversion for this clause. Allocate the PCI list and
  // get a handle on the operands for this clause.
  //
  PCIList code(space);

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  // Handle the NULL processing
  //
  PCIID nullBranch = PCode::nullBranch(this, code, attrs);

  if((getInstruction() != CONV_BIGNUM_BIN64S) &&
     (getInstruction() != CONV_BIN64S_BIGNUM) &&
     (getInstruction() != CONV_BIN32S_BIGNUM) &&
     (getInstruction() != CONV_BIN16S_BIGNUM) &&
     (getInstruction() != CONV_BIGNUM_BIGNUM) &&
     (getInstruction() != CONV_SIMPLE_TO_COMPLEX) &&
     (getInstruction() != CONV_FLOAT64_FLOAT64) &&
     (getInstruction() != CONV_FLOAT32_FLOAT32) &&
     (getInstruction() != CONV_FLOAT32_FLOAT64) &&
     (getInstruction() != CONV_BIN16S_FLOAT64) &&
     (getInstruction() != CONV_BIN32S_FLOAT64) &&
     (getInstruction() != CONV_BIN64S_FLOAT64) &&
     (getInstruction() != CONV_BOOL_BOOL)) {


    // Compute the high and low bounds of the source and destination
    // operands.
    //
    Int32 srcBig, dstBig, srcSigned, dstSigned;
    UInt64 srcHighBounds = 0;
    Int64 srcLowBounds = 0;
    UInt64 dstHighBounds = 0;
    Int64 dstLowBounds = 0;

    computeBounds(src, srcLowBounds, srcHighBounds, srcBig, srcSigned);
    computeBounds(dst, dstLowBounds, dstHighBounds, dstBig, dstSigned);

    // Determine which bounds must be checked. If the source bound are
    // tighter than the destination bound, then no check is necessary.
    //
    AML srcAml(PCIT::getMemoryAddressingMode(src->getDatatype()), PCIT::IBIN64S);
    Int32 srcAtp = src->getAtp();
    Int32 srcAtpIndex = src->getAtpIndex();
    Int32 srcOffset = (Int32)src->getOffset();

    if(srcLowBounds < dstLowBounds)
      {
	if (getInstruction() == CONV_DECS_BIN64S)
	  return ex_clause::pCodeGenerate(space, f);

        OL ol(srcAtp, srcAtpIndex, srcOffset, (Int64)dstLowBounds); 
        PCI pci(PCIT::Op_RANGE_LOW, srcAml, ol); // RANGE_LOW_S32S64, RANGE_LOW_U32S64, RANGE_LOW_S64S64, 
        code.append(pci);
      }
    if(srcHighBounds > dstHighBounds) 
      {        
	if (getInstruction() == CONV_DECS_BIN64S)
	  return ex_clause::pCodeGenerate(space, f);

        OL ol(srcAtp, srcAtpIndex, srcOffset, (Int64)dstHighBounds);               
        PCI pci(PCIT::Op_RANGE_HIGH, srcAml, ol); // RANGE_HIGH_S32S64, RANGE_HIGH_U32S64, RANGE_HIGH_S64S64,
        code.append(pci);
      }
  }

#if (defined (_DEBUG) )
  if(!getenv("NO_PCODE_FLOAT_RANGE"))
#endif
    if( getInstruction() == CONV_FLOAT64_FLOAT64 &&
       !ex_expr::notValidateFloat64(f)) {
      AML aml(PCIT::MFLT64);
      OL ol(src->getAtp(), src->getAtpIndex(), (Int32)src->getOffset());               
      PCI pci(PCIT::Op_RANGE_LOW, aml, ol); // RANGE_MFLT64
      code.append(pci);
    }

  // Do the conversion
  //
  switch(getInstruction()) 
    {
      // For conversions that are simple moves, generate the byte move
      // instruction. This includes unmatched signedness conversions such
      // as BIN32U-->BIN32S since the bounds checking insures that
      // copying the bytes will effect the correct conversion.
      //
    case CONV_BIN16U_BIN16U: case CONV_BIN16U_BIN16S:
    case CONV_BIN16S_BIN16S: case CONV_BIN16S_BIN16U:
    case CONV_BIN32U_BIN32U: case CONV_BIN32U_BIN32S:
    case CONV_BIN32S_BIN32S: case CONV_BIN32S_BIN32U:
    case CONV_BIN64S_BIN64S:
    case CONV_BIN64U_BIN64U:
    case CONV_BPINTU_BPINTU:
    case CONV_FLOAT64_FLOAT64:
    case CONV_FLOAT32_FLOAT32:
    case CONV_BIN8S_BIN8S:
    case CONV_BIN8U_BIN8U:
    case CONV_BOOL_BOOL:
      {
	AML aml(PCIT::MBIN8, PCIT::MBIN8, PCIT::IBIN32S);
	OL ol(dst->getAtp(), dst->getAtpIndex(), dst->getOffset(),
	      src->getAtp(), src->getAtpIndex(), src->getOffset(),
	      dst->getLength());
	PCI pci(PCIT::Op_MOVE, aml, ol);
	code.append(pci);
      }
    break;

    // Conversions from bigger to smaller types can be accomplished by
    // moving the right subset of bytes from the source to the destination.
    //
    case CONV_BIN32S_BIN16U: case CONV_BIN32S_BIN16S:
    case CONV_BIN32U_BIN16U: case CONV_BIN32U_BIN16S:
    case CONV_BIN64S_BIN16U: case CONV_BIN64S_BIN16S:
    case CONV_BIN64S_BIN32U: case CONV_BIN64S_BIN32S:
    case CONV_BIN16S_BIN8S:  case CONV_BIN16U_BIN8U:
    case CONV_BIN16S_BIN8U:  case CONV_BIN16U_BIN8S:
      {
	AML aml(PCIT::MBIN8, PCIT::MBIN8, PCIT::IBIN32S);
#ifdef NA_LITTLE_ENDIAN
	Int32 srcOffset = src->getOffset();
#else
	Int32 srcOffset = src->getOffset() + src->getLength() - dst->getLength();
#endif
	OL ol(dst->getAtp(), dst->getAtpIndex(), dst->getOffset(),
	      src->getAtp(), src->getAtpIndex(), srcOffset,
	      dst->getLength());
	PCI pci(PCIT::Op_MOVE, aml, ol);
	code.append(pci);
      }
    break;

      // Other conversions require specific operations.
      //
#if 0
    case CONV_SIMPLE_TO_COMPLEX:
      {
	AML aml(PCIT::getMemoryAddressingMode(dst->getDatatype()),
		PCIT::getMemoryAddressingMode(src->getDatatype()),
		PCIT::IBIN32S);
	OL ol(dst->getAtp(), dst->getAtpIndex(), dst->getOffset(),
	      src->getAtp(), src->getAtpIndex(), src->getOffset(),
	      dst->getPrecision());
	PCI pci(PCIT::Op_MOVE, aml, ol);
	code.append(pci);
      }
      break;
#endif

    case CONV_COMPLEX_TO_COMPLEX:
      {
	if((dst->getPrecision() == src->getPrecision())
	   /*&&	(dst->getScale() == src->getScale())*/)
	  {
	    AML aml(PCIT::getMemoryAddressingMode(dst->getDatatype()),
		    PCIT::getMemoryAddressingMode(src->getDatatype()),
		    PCIT::IBIN32S);
	    OL ol(dst->getAtp(), dst->getAtpIndex(), dst->getOffset(),
		  src->getAtp(), src->getAtpIndex(), src->getOffset(),
		  dst->getPrecision());
	    PCI pci(PCIT::Op_MOVE, aml, ol);
	    code.append(pci);
	  }
	else
	  {
	    AML aml(PCIT::getMemoryAddressingMode(dst->getDatatype()),
		    PCIT::IBIN32S,
		    PCIT::getMemoryAddressingMode(src->getDatatype()),
		    PCIT::IBIN32S, PCIT::IBIN32S);
	    OL ol(dst->getAtp(), dst->getAtpIndex(), dst->getOffset(),
		  dst->getPrecision(),
		  src->getAtp(), src->getAtpIndex(), src->getOffset(),
		  src->getPrecision(), dst->getScale() - src->getScale());
	    PCI pci(PCIT::Op_MOVE, aml, ol);
	    code.append(pci);
	  }
      }
    break;
      
    case CONV_DECS_BIN64S:
      {
	AML aml(PCIT::getMemoryAddressingMode(dst->getDatatype()),
		PCIT::getMemoryAddressingMode(src->getDatatype()),
		PCIT::IBIN32S);
	OL ol(dst->getAtp(), dst->getAtpIndex(), dst->getOffset(),
	      src->getAtp(), src->getAtpIndex(), src->getOffset(),
	      src->getLength());
	PCI pci(PCIT::Op_MOVE, aml, ol);
	code.append(pci);
      }
    break;

    case CONV_BIGNUM_BIGNUM:
    {
      AML aml(PCIT::MBIGS, PCIT::MBIGS, PCIT::IBIN32S, PCIT::IBIN32S);
      OL ol(dst->getAtp(), dst->getAtpIndex(), dst->getOffset(),
            src->getAtp(), src->getAtpIndex(), src->getOffset(),
            dst->getLength(),
            src->getLength());
      PCI pci(PCIT::Op_MOVE, aml, ol);
      code.append(pci);
      break;
    }

    case CONV_BIGNUM_BIN64S:
    {
      AML aml(PCIT::MBIN64S, PCIT::MBIGS, PCIT::IBIN32S);
      OL ol(dst->getAtp(), dst->getAtpIndex(), dst->getOffset(),
            src->getAtp(), src->getAtpIndex(), src->getOffset(),
            src->getLength());
      PCI pci(PCIT::Op_MOVE, aml, ol);
      code.append(pci);
      break;
    }

    case CONV_SIMPLE_TO_COMPLEX:
    case CONV_BIN64S_BIGNUM:
    case CONV_BIN32S_BIGNUM:
    case CONV_BIN16S_BIGNUM:
    {
      AML aml(PCIT::MBIGS,
              PCIT::getMemoryAddressingMode(src->getDatatype()),
              PCIT::IBIN32S);
      OL ol(dst->getAtp(), dst->getAtpIndex(), dst->getOffset(),
            src->getAtp(), src->getAtpIndex(), src->getOffset(),
            dst->getLength());
      PCI pci(PCIT::Op_MOVE, aml, ol);
      code.append(pci);
      break;
    }

    case CONV_BIN8S_BIN16S:
    case CONV_BIN8U_BIN16U:
     {
	AML aml(PCIT::getMemoryAddressingMode(dst->getDatatype()),
		PCIT::getMemoryAddressingMode(src->getDatatype()));
	OL ol(dst->getAtp(), dst->getAtpIndex(), (Int32)dst->getOffset(),
	      src->getAtp(), src->getAtpIndex(), (Int32)src->getOffset());
	PCI pci(PCIT::Op_MOVE, aml, ol);

	code.append(pci);
      }
      break;

    default:
      {
	AML aml(PCIT::getMemoryAddressingMode(dst->getDatatype()),
		PCIT::getMemoryAddressingMode(src->getDatatype()));
	OL ol(dst->getAtp(), dst->getAtpIndex(), (Int32)dst->getOffset(),
	      src->getAtp(), src->getAtpIndex(), (Int32)src->getOffset());
	PCI pci(PCIT::Op_MOVE, aml, ol);

	code.append(pci);
      }
    };

  // Add the branch target if necessary.
  //
  if(nullBranch)
  {
    AML aml; 
    OL ol((Int64)nullBranch);
    PCI pci(PCIT::Op_TARGET, aml, ol); 
    code.append(pci);
  }

  // Administration...
  //

  // store the first fixed field offset
  code.append(PCode::storeVoa(attrs[0], space));

//  if(getNumOperands() > 0) 
//    code.append(PCode::updateRowLen(attrs[0], space, f));

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}


// ExUnPackCol::pCodeGenerate() ------------------------------------
// Generate PCode for the ExUnPackCol clause.  The ExUnPackCol
// clause extracts a set of bits from a CHAR value.  The set of
// bits to extract is described by a base offset, a width, and
// an index.  The offset and width are known at compile time, but
// the index is a run time variable.  ExUnPackCol clause also gets
// the null indicator of the result from a bitmap within the CHAR
// field. This clause is implemented using the following pCode instructions:
//
//        'loadValue'           - to load the index value
//        Op_MV_INDEX_ATP_NB    - to move the NULL indicator
//        Op_MV_INDEXED_ATP_BXX - to move the set of bits
//        Op_POP                - to remove the index from the stack
//
//  where XX is one of {32,8,4,2,1} and indicates the width of the value
//  in bits.
//
// A series of UnPackCol clauses will generate redundant loads and pops
// of the index value.  These will be removed in a pass over the generated
// instructions.
//
// ExUnPackCol supports other bit widths, but no pCode will be generated
// for these.
//
ex_expr::exp_return_type ExUnPackCol::pCodeGenerate(Space *space, UInt32 f) {
  return ex_clause::pCodeGenerate(space, f);

  /*

#ifdef _DEBUG
  // For debugging to disable the pCode generation
  if(getenv("PCODE_NO_UNPACK"))
    // Generate the default clause_eval instruction.
    //
    return ex_clause::pCodeGenerate(space, f);
#endif

  // Get a handle on the operands
  //
  AttributesPtr *attrs = getOperand();

  // Use the default pCodeGenerate for cases not handled here
  //
  if(((attrs[0]->getLength() == 4) && (width_ == 32)) ||
     ((attrs[0]->getLength() == 2) && (width_ == 8)) ||
     ((attrs[0]->getLength() == 2) && (width_ == 4)) ||
     ((attrs[0]->getLength() == 2) && (width_ == 2)) ||
     ((attrs[0]->getLength() == 2) && (width_ == 1))) {
    
    // Determine the instruction to use to extract (UnPack) the bits.
    //
    PCIType::Operation oper;

    switch(width_) {
    case 32:
      oper = PCIT::Op_MV_INDEXED_ATP_B32;
      break;
    case 8:
      oper = PCIT::Op_MV_INDEXED_ATP_B8;
      break;
    case 4:
      oper = PCIT::Op_MV_INDEXED_ATP_B4;
      break;
    case 2:
      oper = PCIT::Op_MV_INDEXED_ATP_B2;
      break;
    case 1:
      oper = PCIT::Op_MV_INDEXED_ATP_B1;
      break;
    }
    
    // Allocate the code list
    //
    PCIList code(space);

    // Generate pre clause PCI's
    //
    PCode::preClausePCI(this, code);

    // Load the index value.
    //
    code.append(PCode::loadValue(attrs[2], space));

    // Null Processing...
    // If packed value has a null bitmap, extract the bit into
    // the NULL indicator of the result.
    //
    if(nullsPresent_) {
      AML aml(PCIT::IBIN32S, PCIT::IBIN32S);
      OL ol(attrs[0]->getAtp(),
            attrs[0]->getAtpIndex(),
            attrs[0]->getNullIndOffset(),
            attrs[1]->getAtp(),
            attrs[1]->getAtpIndex(),
            attrs[1]->getOffset() + sizeof(int));
      PCI pci(PCIT::Op_MV_INDEXED_ATP_NB, aml, ol);

      code.append(pci);
    }

    // UnPack the value at the specified index.
    //
    AML aml(PCIT::IBIN32S, PCIT::IBIN32S);
    OL ol(attrs[0]->getAtp(),
          attrs[0]->getAtpIndex(),
          attrs[0]->getOffset(),
          attrs[1]->getAtp(),
          attrs[1]->getAtpIndex(),
          attrs[1]->getOffset() + base_);

    PCI pci(oper, aml, ol);
    code.append(pci);

    // Consume the index...
    //
    AML aml1;
    OL ol1(1);
    PCI pci1(PCIT::Op_POP, aml1, ol1);
    code.append(pci1);

    // Generate post clause PCI's
    //
    PCode::postClausePCI(this, code);

    setPCIList(code.getList());
    return ex_expr::EXPR_OK;

  } else if ((attrs[0]->getLength() * 8) == width_) {

    // Allocate the code list
    //
    PCIList code(space);

    // Generate pre clause PCI's
    //
    PCode::preClausePCI(this, code);

    // Load the index value.
    //
    code.append(PCode::loadValue(attrs[2], space));

    // Null Processing...
    // If packed value has a null bitmap, extract the bit into
    // the NULL indicator of the result.
    //
    if(nullsPresent_) {
      AML aml(PCIT::I32, PCIT::I32);
      OL ol(attrs[0]->getAtp(),
            attrs[0]->getAtpIndex(),
            attrs[0]->getNullIndOffset(),
            attrs[1]->getAtp(),
            attrs[1]->getAtpIndex(),
            attrs[1]->getOffset() + sizeof(int));
      
      PCI pci(PCIT::Op_MV_INDEXED_ATP_NB, aml, ol);
      code.append(pci);
    }

    // UnPack the value at the specified index.
    //
    AML aml(PCIT::I32, PCIT::I32);
    OL ol(attrs[0]->getAtp(),
          attrs[0]->getAtpIndex(),
          attrs[0]->getOffset(),
          attrs[1]->getAtp(),
          attrs[1]->getAtpIndex(),
          attrs[1]->getOffset() + base_,
          attrs[0]->getLength());
    PCI pci(PCIT::Op_MV_INDEXED_ATP_N, aml, ol);
    code.append(pci);


    // Consume the index...
    //
    AML aml1;
    OL ol1(1);
    PCI pci1(PCIT::Op_POP, aml1, ol1);
    code.append(pci1);
    
    // Generate post clause PCI's
    //
    PCode::postClausePCI(this, code);

    setPCIList(code.getList());
    return ex_expr::EXPR_OK;
  } else {
    return ex_clause::pCodeGenerate(space, f);
  } */
}

ex_expr::exp_return_type ex_function_mod::pCodeGenerate(Space *space, UInt32 f)
{

  PCIID branchEnd = 0;

  // Get a handle on the operands
  //
  AttributesPtr *attrs = getOperand();

  // Use the default pCodeGenerate for cases not handled here
  //
  if (NOT (((attrs[0]->getDatatype() == REC_BIN32_SIGNED) &&
	    (attrs[1]->getDatatype() == REC_BIN32_SIGNED) &&
	    (attrs[2]->getDatatype() == REC_BIN32_SIGNED)) ||
	   ((attrs[0]->getDatatype() == REC_BIN32_UNSIGNED) &&
	    (attrs[1]->getDatatype() == REC_BIN32_UNSIGNED) &&
	    (attrs[2]->getDatatype() == REC_BIN32_SIGNED)))) {
    return ex_clause::pCodeGenerate(space, f);
  }

  // If any of the inputs are nullable, and we need to 
  // call processNulls, use clauseEval.
  //
  if(isAnyOperandNullable() && (!isNullRelevant() || !isNullInNullOut())) {
    return ex_clause::pCodeGenerate(space, f);
  }

  // Allocate the code list
  //
  PCIList code(space);

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  // If any of the operands are nullable, NULL is relevant for this clause, 
  // and any NULL input produces a NULL output, insert the appropriate 
  // PCODE sequence to compute the nullness of the result.
  //
  if(isAnyOperandNullable() && isNullRelevant() && isNullInNullOut()) {
    branchEnd = PCode::nullBranch(this, code, attrs);
  }

  // First operand is the memory location of the result of the modulo
  // Second operand is the memory location of the 1st argument to modulo
  // Third operand is the memory location of the 2nd argument to modulo
  //
  AML aml(PCIT::getMemoryAddressingMode(attrs[0]->getDatatype()),
	  PCIT::getMemoryAddressingMode(attrs[1]->getDatatype()),
	  PCIT::getMemoryAddressingMode(attrs[2]->getDatatype()));
  OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(), 
	attrs[1]->getAtp(), attrs[1]->getAtpIndex(), attrs[1]->getOffset(), 
	attrs[2]->getAtp(), attrs[2]->getAtpIndex(), attrs[2]->getOffset());

  PCI pci(PCIT::Op_MOD, aml, ol);
  code.append(pci);

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);
  if(branchEnd)
  {
    AML aml1;
    OL ol1((Int64)branchEnd);
    PCI pci1(PCIT::Op_TARGET, aml1, ol1);
    code.append(pci1);
  }

  setPCIList(code.getList());

  return ex_expr::EXPR_OK;
}



ex_expr::exp_return_type
ex_function_nullifzero::pCodeGenerate(Space *space, UInt32 f)
{
  // Get a handle on the operands
  //
  AttributesPtr *attrs = getOperand();
  Attributes *dst = attrs[0];
  Attributes *src = attrs[1];

  //
  PCIList code(space);
  
  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);
  
  // handle NULLs (may jump)
  PCIID nullBranch = PCode::nullBranch(this, code, attrs);

  AML aml(PCIT::MPTR32,    // target ptr
          PCIT::MATTR3,    // source null ptr
          PCIT::MPTR32,    // source ptr
          PCIT::IBIN32S);  // source/target length

  OL ol(dst->getAtp(), dst->getAtpIndex(), (Int32)dst->getOffset(),
	dst->getAtp(), dst->getAtpIndex(), dst->getNullIndOffset(),
          dst->getNullBitIndex(),
	src->getAtp(), src->getAtpIndex(), (Int32)src->getOffset(),
	src->getLength());

  PCI pci(PCIT::Op_NULLIFZERO, aml, ol);
  code.append(pci);
  
  // Add the branch target if necessary.
  //
  if (nullBranch)
    {
      AML aml; 
      OL ol((Int64)nullBranch);
      PCI pci(PCIT::Op_TARGET, aml, ol); 
      code.append(pci);
    }
  
  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

//
// NVL(e1, e2) returns e2 if e1 is NULL otherwise e1. NVL(e1, e2) is
// equivalent to ANSI/ISO
//      COALESCE(e1, e2)
//        or,
//      CASE WHEN e1 IS NULL THEN e2 ELSE e1 END
// Both arguments can be nullable and actually null; they both can
// be constants as well.
// NVL() on CHAR type expressions is mapped to CASE. ISNULL(e1, e2) is
// mapped into NVL(e1, e2)
// Datatypes of e1 and e2 must be comparable/compatible.
//
ex_expr::exp_return_type ex_function_nvl::pCodeGenerate(Space *space, UInt32 f)
{
  // Get a handle on the operands
  //
  AttributesPtr *attrs = getOperand();
  Attributes *res = attrs[0];
  Attributes *arg1 = attrs[1];
  Attributes *arg2 = attrs[2];

  Int16 resAtp = res->getAtp();
  Int16 resAtpIdx = res->getAtpIndex();
  UInt32 resOffs = res->getOffset();
  Int32 resNullOffs = res->getNullIndOffset();

  // As of today, NVL() on CHAR types becomes CASE. So make sure we are
  // not dealing with any CHAR types
  assert(!DFS2REC::isAnyCharacter(arg1->getDatatype()) &&
        !DFS2REC::isAnyCharacter(arg2->getDatatype()));

  PCIList code(space);

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  PCIID nullBranch = 0;

  // If the argument is nullable, then proceed to perform a null test first.
  // Otherwise we skip the test and just copy the column to the result.

  if (arg1->getNullFlag()) {

  // handle NULLs (may jump)
  //  PCIID nullBranch = PCode::nullBranch(this, code, attrs);
  PCIID notNullBranch = 0;
  if (arg1->isSQLMXAlignedFormat())
  {
    PCodeTupleFormats tpf(res->getTupleFormat(), 
                          arg1->getTupleFormat());

    AML aml(PCIT::MBIN32S,PCIT::MBIN32S,PCIT::IATTR3,PCIT::IBIN32S);
    OL ol(resAtp, resAtpIdx, resNullOffs, 
          arg1->getAtp(), arg1->getAtpIndex(), arg1->getNullIndOffset(), 
          EXPAND_PCODEATTRNULL2(*(UInt32 *)&tpf, res, arg1),
          0);
    PCI pci(PCIT::Op_NOT_NULL_BRANCH, aml, ol);
    code.append(pci);
    notNullBranch = code.getTailId();
  }
  else
  {
    AML aml(PCIT::MBIN16S, PCIT::IBIN32S);
    OL ol(arg1->getAtp(), arg1->getAtpIndex(), arg1->getNullIndOffset(), 0);
    PCI pci(PCIT::Op_NOT_NULL_BRANCH, aml, ol);
    code.append(pci);
    notNullBranch = code.getTailId();
  }

  // it will come here if arg1 is a NULL value.
  // Move arg2 to dst. arg2 and dst have the same data attrs.
  {
    assert((UInt32)(res->getLength()) >= (UInt32)(arg2->getLength()));

    AML aml(PCIT::MBIN8,PCIT::MBIN8,PCIT::IBIN32S);
    OL ol(resAtp, resAtpIdx, (Int32)resOffs,
          arg2->getAtp(), arg2->getAtpIndex(), (Int32)arg2->getOffset(),
          (Int32)arg2->getLength());
    PCI pci(PCIT::Op_MOVE, aml, ol);
    code.append(pci);

    // Set or Clear NULL flag.

    // if the second operand is not nullable then just clear the null
    // indicator in the result
    if (!arg2->getNullFlag() && res->getNullFlag())
    {
      AML amlNotNull(PCIT::MBIN16U,PCIT::IBIN16U);
      OL olNotNull(resAtp, resAtpIdx, resNullOffs, 0);
      PCI pciNotNull(PCIT::Op_MOVE, amlNotNull, olNotNull);
      code.append(pciNotNull);
    }
    else
    {

      // second argument is NULLABLE, so copy the null status from the
      // second argument into the null status of the result using one
      // of the two possible formats

      code.append(PCode::isNull(res, arg2, space));
    }
  }

  nullBranch = PCode::generateJumpAndBranch(res, code, notNullBranch);

  }

  // it will come here if arg1 is not a NULL value.
  // Move arg1 to dst. arg1 and dst have the same data attrs.
  {
    assert((UInt32)(res->getLength()) >= (UInt32)(arg1->getLength()));

    AML aml(PCIT::MBIN8,PCIT::MBIN8,PCIT::IBIN32S);
    OL ol(resAtp, resAtpIdx, (Int32)resOffs,
          arg1->getAtp(), arg1->getAtpIndex(), (Int32)arg1->getOffset(),
          (Int32)arg1->getLength());
    PCI pci(PCIT::Op_MOVE, aml, ol);
    code.append(pci);

    // first operand is NOT NULL, simply clear the status in result
    if (res->getNullFlag())
    {
      AML amlNotNull(PCIT::MBIN16U,PCIT::IBIN16U);
      OL olNotNull(resAtp, resAtpIdx, resNullOffs, (Int16)0);
      PCI pciNotNull(PCIT::Op_MOVE, amlNotNull, olNotNull);
      code.append(pciNotNull);
    }
  }

  // Add the branch target if necessary.
  //
  if (nullBranch)
  {
    AML aml;
    OL ol((Int64)nullBranch);
    PCI pci(PCIT::Op_TARGET, aml, ol);
    code.append(pci);
  }

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ExFunctionRandomNum::pCodeGenerate(Space *space, UInt32 f)
{
  if (NOT simpleRandom()) 
    return ex_clause::pCodeGenerate(space, f);
    
  // Get a handle on the operands
  //
  AttributesPtr *attrs = getOperand();
  Attributes *dst = attrs[0];
  Attributes *src = attrs[1];

  if (getNumOperands() > 1)
    return ex_clause::pCodeGenerate(space, f);
    
  //
  PCIList code(space);
  
  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);
  
  AML aml(PCIT::IBIN32S,  // OperTypeEnum
	  PCIT::MBIN8,    // target ptr
	  PCIT::MBIN8,    // not needed
	  PCIT::IBIN32S,  // not needed
	  PCIT::MBIN8,    // not needed
	  PCIT::IBIN32S); // seed
  OL ol(getOperType(),
	dst->getAtp(), dst->getAtpIndex(), (Lng32)dst->getOffset(),
	-1, -1, -1,
	-1,
	-1, -1, -1,
	0); // init seed to zero
  PCI pci(PCIT::Op_GENFUNC, aml, ol);
  code.append(pci);
  
  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ExHash2Distrib::pCodeGenerate(Space *space, UInt32 f)
{
  // Get a handle on the operands
  //
  AttributesPtr *attrs = getOperand();

  PCIList code(space);
  PCode::preClausePCI(this, code);

  AML aml(PCIT::MBIN32U,  // Target (the partition number)
          PCIT::MBIN32U,  // The hash value
          PCIT::MBIN32U); // Number of partitions

  // attr[0] = result
  // attr[1] = keyValue
  // attr[2] = numParts
  OL ols(attrs[0]->getAtp(),attrs[0]->getAtpIndex(),(Int32)attrs[0]->getOffset(),
	 attrs[1]->getAtp(),attrs[1]->getAtpIndex(),(Int32)attrs[1]->getOffset(),
	 attrs[2]->getAtp(),attrs[2]->getAtpIndex(),(Int32)attrs[2]->getOffset());

  PCI pci(PCIT::Op_HASH2_DISTRIB, aml, ols);
  code.append(pci);

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type
ex_function_concat::pCodeGenerate(Space *space, UInt32 f)
{
  AttributesPtr *attrs = getOperand();

  Int32 len0 = attrs[0]->getLength(); // Length of output string space
  Int32 len1 = attrs[1]->getLength(); // Length of input string 1
  Int32 len2 = attrs[2]->getLength(); // Length of input string 2

  if ( len0 < (len1 + len2) )         // If output space is too small
    return ex_clause::pCodeGenerate(space, f);

  CharInfo::CharSet cs1 = ((SimpleType *)getOperand(1))->getCharSet();
  CharInfo::CharSet cs2 = ((SimpleType *)getOperand(2))->getCharSet();
  // If a character set is SJIS or Unicode
  if( (cs1 == CharInfo::UTF8) /* || (cs2 == CharInfo::SJIS) */ )
  {
    Int32 prec1 = ((SimpleType *)getOperand(1))->getPrecision();
    Int32 prec2 = ((SimpleType *)getOperand(2))->getPrecision();
    if (  ( prec1 && (prec1 != len1) )
       || ( prec2 && (prec2 != len2) )
       )
      // Strings may have filler spaces at the end which pcode concat function
      // cannot handle, so call ex_clause version instead.
      return ex_clause::pCodeGenerate(space, f);
  }
  //
  PCIList code(space);

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  // Add the null logic if necessary.
  //
  PCIID branchToEnd = PCode::nullBranch(this, code, attrs);

  AML aml(PCIT::MATTR5,  // target ptr
          PCIT::MATTR5,  // src1 ptr
          PCIT::MATTR5); // src2 ptr

  UInt32 comboLen1=0;
  UInt32 comboLen2=0;
  UInt32 comboLen3=0;
  char* comboPtr1 = (char*)&comboLen1;
  char* comboPtr2 = (char*)&comboLen2;
  char* comboPtr3 = (char*)&comboLen3;

  // Use combo fields for specifying null/vc lengths of both operands
  comboPtr1[0] = (char)attrs[0]->getNullIndicatorLength(),
  comboPtr1[1] = (char)attrs[0]->getVCIndicatorLength();
  comboPtr2[0] = (char)attrs[1]->getNullIndicatorLength(),
  comboPtr2[1] = (char)attrs[1]->getVCIndicatorLength();
  comboPtr3[0] = (char)attrs[2]->getNullIndicatorLength(),
  comboPtr3[1] = (char)attrs[2]->getVCIndicatorLength();

  OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(),
          attrs[0]->getVoaOffset(), len0, comboLen1,
        attrs[1]->getAtp(), attrs[1]->getAtpIndex(), attrs[1]->getOffset(),
          attrs[1]->getVoaOffset(), len1, comboLen2,
        attrs[2]->getAtp(), attrs[2]->getAtpIndex(), attrs[2]->getOffset(),
          attrs[2]->getVoaOffset(), len2, comboLen3);

  PCI pci(PCIT::Op_GENFUNC, aml, ol);
  code.append(pci);

  // Add the branch target if necessary.
  //
  if(branchToEnd)
  {
    AML aml;
    OL ol((Int64)branchToEnd);
    PCI pci(PCIT::Op_TARGET, aml, ol);
    code.append(pci);
  }

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ex_function_substring::pCodeGenerate(Space *space,
                                                              UInt32 f)
{
  // How many nullable attrs are there?
  //
  AttributesPtr *attrs = getOperand();
  Int32 numOfNullableFields = 0;
  for(Int32 i=0; i<getNumOperands(); i++) {
    // Count nullable attributes (but not output)
    if (attrs[i]->getNullFlag() && (i != 0))
      numOfNullableFields++;
  }

  // Temporary fix for substring function. Since pcode substring function
  // cannot handle SJIS and UTF-8 characters, call ex_clause version of
  // substring. The following 5 lines of code can be removed after pcode
  // substring function can handle SJIS and UTF-8 characters on SJIS or
  // Unicode configuration.

  CharInfo::CharSet cs = ((SimpleType *)getOperand(0))->getCharSet();

  if(cs != CharInfo::ISO88591)
    return ex_clause::pCodeGenerate(space, f);

  // If there are too many nullable fields
  if (numOfNullableFields > 2)
    return ex_clause::pCodeGenerate(space, f);

  //
  PCIList code(space);

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  // Get a handle on the operands
  //
  Attributes *dst = attrs[0];
  Attributes *src = attrs[1];

  // Add the null logic if necessary.
  //
  PCIID nullBranch = PCode::nullBranch(this, code, attrs);

  AML aml(PCIT::MATTR5,   // target ptr
          PCIT::MATTR5,   // operand1 (string)
          PCIT::MBIN32S,  // operand2 (start position)
          PCIT::MBIN32S); // operand3 (length)

  UInt32 comboLen1 = 0;
  UInt32 comboLen2 = 0;
  char* comboPtr1 = (char*)&comboLen1;
  char* comboPtr2 = (char*)&comboLen2;

  // Use combo fields for specifying vc/null lengths of both operands
  comboPtr1[0] = (char)dst->getNullIndicatorLength(),
  comboPtr1[1] = (char)dst->getVCIndicatorLength();
  comboPtr2[0] = (char)src->getNullIndicatorLength(),
  comboPtr2[1] = (char)src->getVCIndicatorLength();

  // Use part of combo field to specify additional info
  comboPtr2[2] = (getNumOperands() == 4);  // Was length passed in?
  comboPtr2[3] = getOperType();            // What is the operation type?

  // If no length field exists, just copy over "start" (attribute 2) instead
  Int32 a3 = 3;
  if (comboPtr2[2] == 0)
    a3 = 2;

  OL ol(dst->getAtp(), dst->getAtpIndex(), dst->getOffset(),
          dst->getVoaOffset(), dst->getLength(), comboLen1,
        src->getAtp(), src->getAtpIndex(), src->getOffset(),
          src->getVoaOffset(), src->getLength(), comboLen2,
        attrs[2]->getAtp(), attrs[2]->getAtpIndex(), attrs[2]->getOffset(),
        attrs[a3]->getAtp(), attrs[a3]->getAtpIndex(), attrs[a3]->getOffset());

  PCI pci(PCIT::Op_GENFUNC, aml, ol);

  code.append(pci);

  // Add the branch target if necessary.
  //
  if (nullBranch)
  {
    AML aml;
    OL ol((Int64)nullBranch);
    PCI pci(PCIT::Op_TARGET, aml, ol);
    code.append(pci);
  }

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());

  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ExFunctionBitOper::pCodeGenerate(Space *space, UInt32 f)
{
  if ((getOperType() == ITM_CONVERTTOBITS) ||
      (getOperType() == ITM_BITEXTRACT))
    return ex_clause::pCodeGenerate(space, f);

  AttributesPtr *attrs = getOperand();

  //
  PCIList code(space);

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  // Add the null logic if necessary.
  //
  PCIID branchToEnd = PCode::nullBranch(this, code, attrs);

  AML aml(PCIT::IBIN32S,  // OperTypeEnum
	  PCIT::MBIN8,    // target ptr
	  PCIT::MBIN8,    // operand1
	  PCIT::MBIN8,    // operand2
	  PCIT::IBIN32S,  // datatype operand0
	  PCIT::IBIN32S); // length operand0
  OL ol(getOperType(),
	attrs[0]->getAtp(), attrs[0]->getAtpIndex(), (Lng32)attrs[0]->getOffset(),
	attrs[1]->getAtp(), attrs[1]->getAtpIndex(), (Lng32)attrs[1]->getOffset(),
	((getOperType() == ITM_BITNOT) ? attrs[1]->getAtp() : attrs[2]->getAtp()),
	((getOperType() == ITM_BITNOT) ? attrs[1]->getAtpIndex() : attrs[2]->getAtpIndex()),
	((getOperType() == ITM_BITNOT) ? (Lng32)attrs[1]->getOffset() : (Lng32)attrs[2]->getOffset()),
	attrs[0]->getDatatype(), attrs[0]->getLength());
  PCI pci(PCIT::Op_GENFUNC, aml, ol);
  code.append(pci);

  // Add the branch target if necessary.
  //
  if(branchToEnd)
  {
    AML aml; 
    OL ol((Int64)branchToEnd);
    PCI pci(PCIT::Op_TARGET, aml, ol); 
    code.append(pci);
  }

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ExHeaderClause::pCodeGenerate( Space  *space,
                                                        UInt32  flags )
{
  Attributes *tgt = getOperand(0);

  // Initialize the pCode
  PCIList code(space);

  // Generate pre clause PCI's
  PCode::preClausePCI(this, code);

  AML aml(PCIT::MPTR32,
          PCIT::IBIN32S,PCIT::IBIN32S,PCIT::IBIN32S,PCIT::IBIN32S,PCIT::IBIN32S);

  OL ol(tgt->getAtp(), tgt->getAtpIndex(), (Int32)tgt->getOffset(),
        (Int32)adminSz_,     // bytes to clear
        (Int32)firstFixedOffset_,
        (Int32)( isSQLMXAlignedFormat()
                 ? ExpAlignedFormat::OFFSET_SIZE : ExpVoaSize),
        (Int32)bitmapOffset_, 
        (Int32)entryOffset_
        );

  PCI pci(PCIT::Op_HDR, aml, ol);
  code.append(pci);

  // Generate post clause PCI's
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type
ex_function_upper::pCodeGenerate(Space *space, UInt32 f)

{
  // Get a handle on the operands
  //
  AttributesPtr *attrs = getOperand();
  Attributes *dst = attrs[0];
  Attributes *src = attrs[1];

  // Temporary fix for upper function. Since pcode substring function
  // cannot handle SJIS and UTF-8 characters, call ex_clause version of
  // upper. The following 5 lines of code can be removed after pcode
  // upper function can handle SJIS and UTF-8 characters on SJIS or
  // Unicode configuration.

  CharInfo::CharSet cs = ((SimpleType *)getOperand(0))->getCharSet();

  if(cs != CharInfo::ISO88591)
    return ex_clause::pCodeGenerate(space, f);

  if (src->widechar())
    return ex_clause::pCodeGenerate(space, f);

  //
  PCIList code(space);

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  // handle NULLs (may jump)
  PCIID nullBranch = PCode::nullBranch(this, code, attrs);

  UInt32 comboLen1=0;
  UInt32 comboLen2=0;
  char* comboPtr1 = (char*)&comboLen1;
  char* comboPtr2 = (char*)&comboLen2;

  // Use combo fields for specifying null/vc lengths of both operands
  comboPtr1[0] = (char)dst->getNullIndicatorLength(),
  comboPtr1[1] = (char)dst->getVCIndicatorLength();
  comboPtr2[0] = (char)src->getNullIndicatorLength(),
  comboPtr2[1] = (char)src->getVCIndicatorLength();

  AML aml(PCIT::MATTR5,   // target ptr
	  PCIT::MATTR5,   // source ptr
	  PCIT::IBIN32S); // OperTypeEnum

  OL ol(dst->getAtp(), dst->getAtpIndex(), dst->getOffset(), 
        dst->getVoaOffset(), dst->getLength(), comboLen1,
        src->getAtp(), src->getAtpIndex(), src->getOffset(), 
        src->getVoaOffset(), src->getLength(), comboLen2,
        getOperType());

  PCI pci(PCIT::Op_GENFUNC, aml, ol);
  code.append(pci);

  // Add the branch target if necessary.
  //
  if (nullBranch)
    {
      AML aml;
      OL ol((Int64)nullBranch);
      PCI pci(PCIT::Op_TARGET, aml, ol);
      code.append(pci);
    }

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type
ex_function_lower::pCodeGenerate(Space *space, UInt32 f)
{
  // Get a handle on the operands
  //
  AttributesPtr *attrs = getOperand();
  Attributes *dst = attrs[0];
  Attributes *src = attrs[1];

  // Temporary fix for upper function. Since pcode substring function
  // cannot handle SJIS and UTF-8 characters, call ex_clause version of
  // upper. The following 5 lines of code can be removed after pcode
  // upper function can handle SJIS and UTF-8 characters on SJIS or
  // Unicode configuration.

  CharInfo::CharSet cs = ((SimpleType *)getOperand(0))->getCharSet();

  if(cs != CharInfo::ISO88591)
    return ex_clause::pCodeGenerate(space, f);

  if (src->widechar())
    return ex_clause::pCodeGenerate(space, f);

  //
  PCIList code(space);

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  // handle NULLs (may jump)
  PCIID nullBranch = PCode::nullBranch(this, code, attrs);

  UInt32 comboLen1=0;
  UInt32 comboLen2=0;
  char* comboPtr1 = (char*)&comboLen1;
  char* comboPtr2 = (char*)&comboLen2;

  // Use combo fields for specifying null/vc lengths of both operands
  comboPtr1[0] = (char)dst->getNullIndicatorLength(),
  comboPtr1[1] = (char)dst->getVCIndicatorLength();
  comboPtr2[0] = (char)src->getNullIndicatorLength(),
  comboPtr2[1] = (char)src->getVCIndicatorLength();

  AML aml(PCIT::MATTR5,   // target ptr
          PCIT::MATTR5,   // source ptr
          PCIT::IBIN32S); // OperTypeEnum

  OL ol(dst->getAtp(), dst->getAtpIndex(), dst->getOffset(),
          dst->getVoaOffset(), dst->getLength(), comboLen1,
        src->getAtp(), src->getAtpIndex(), src->getOffset(),
          src->getVoaOffset(), src->getLength(), comboLen2,
        getOperType());

  PCI pci(PCIT::Op_GENFUNC, aml, ol);
  code.append(pci);

  // Add the branch target if necessary.
  //
  if (nullBranch)
    {
      AML aml;
      OL ol((Int64)nullBranch);
      PCI pci(PCIT::Op_TARGET, aml, ol);
      code.append(pci);
    }

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type
ex_function_trim_char::pCodeGenerate(Space *space, UInt32 f)
{
  // Only trim where the trim character is a single space literal
  // is supported. The flag noPCodeAvailable is set by the caller if
  // the trim char is something other than a single space literal.
  if (noPCodeAvailable())
    return ex_clause::pCodeGenerate(space, f);

  // Get a handle on the operands
  //
  AttributesPtr *attrs = getOperand();
  Attributes *dst = attrs[0];
  Attributes *src = attrs[2];

  // Temporary fix for trim function. Since pcode trim function
  // cannot handle SJIS and UTF-8 characters, call ex_clause version of
  // trim. The following 5 lines of code can be removed after pcode
  // trim function can handle SJIS and UTF-8 characters on SJIS or
  // Unicode configuration.

  CharInfo::CharSet cs = ((SimpleType *)getOperand(0))->getCharSet();

  if(cs != CharInfo::ISO88591)
    return ex_clause::pCodeGenerate(space, f);

  if (src->widechar())
    return ex_clause::pCodeGenerate(space, f);

  //
  PCIList code(space);

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  // handle NULLs (may jump)
  PCIID nullBranch = PCode::nullBranch(this, code, attrs);

  UInt32 comboLen1=0;
  UInt32 comboLen2=0;
  char* comboPtr1 = (char*)&comboLen1;
  char* comboPtr2 = (char*)&comboLen2;

  // Use combo fields for specifying null/vc lengths of both operands
  comboPtr1[0] = (char)dst->getNullIndicatorLength(),
  comboPtr1[1] = (char)dst->getVCIndicatorLength();
  comboPtr2[0] = (char)src->getNullIndicatorLength(),
  comboPtr2[1] = (char)src->getVCIndicatorLength();

  AML aml(PCIT::MATTR5,   // target ptr
	  PCIT::MATTR5,   // source ptr
	  PCIT::IBIN32S); // OperTypeEnum

  OL ol(dst->getAtp(), dst->getAtpIndex(), dst->getOffset(), 
        dst->getVoaOffset(), dst->getLength(), comboLen1,
        src->getAtp(), src->getAtpIndex(), src->getOffset(), 
        src->getVoaOffset(), src->getLength(), comboLen2,
        (getTrimMode() == 0 ? ITM_RTRIM :
          (getTrimMode() == 1 ? ITM_LTRIM : ITM_TRIM)));

  PCI pci(PCIT::Op_GENFUNC, aml, ol);
  code.append(pci);

  // Add the branch target if necessary.
  //
  if (nullBranch)
    {
      AML aml;
      OL ol((Int64)nullBranch);
      PCI pci(PCIT::Op_TARGET, aml, ol);
      code.append(pci);
    }

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type
ex_function_char_length_doublebyte::pCodeGenerate(Space *space, UInt32 f)
{
  // Get a handle on the operands
  //
  AttributesPtr *attrs = getOperand();

  // Only proceed (for now) with unicode strings.  Other charsets, like Kanji,
  // have 2 bytes for chars, but let's not deal with that now.
  if ((attrs[1]->getDatatype() != REC_NCHAR_F_UNICODE) &&
      (attrs[1]->getDatatype() != REC_NCHAR_V_UNICODE))
    return ex_clause::pCodeGenerate(space, f);

  // Allocate the code list
  //
  PCIList code(space);

  // Generate pre clause PCI's
  PCode::preClausePCI(this, code);

  // Add the null logic if necessary.
  PCIID branchToEnd = PCode::nullBranch(this, code, attrs);

  // Just return length/2 if the string is fixed
  if (attrs[1]->getDatatype() == REC_NCHAR_F_UNICODE) {
    AML aml(PCIT::MBIN32S, PCIT::IBIN32S);
    OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(),
          (Int32)(attrs[1]->getLength() / 2));
    PCI pci(PCIT::Op_MOVE, aml, ol);
    code.append(pci);
  }
  else
  {
    UInt32 comboLen1=0;
    char* comboPtr1 = (char*)&comboLen1;

    // Use combo fields for specifying null/vc lengths of both operands
    comboPtr1[0] = (char)attrs[1]->getNullIndicatorLength(),
    comboPtr1[1] = (char)attrs[1]->getVCIndicatorLength();

    AML aml(PCIT::MBIN32U, PCIT::MUNIV);
    OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(),
          attrs[1]->getAtp(), attrs[1]->getAtpIndex(), attrs[1]->getOffset(),
            attrs[1]->getVoaOffset(), attrs[1]->getLength(), comboLen1);
    PCI pci(PCIT::Op_GENFUNC, aml, ol);
    code.append(pci);
  }

  // Add the branch target if necessary.
  //
  if(branchToEnd)
  {
    AML aml;
    OL ol((Int64)branchToEnd);
    PCI pci(PCIT::Op_TARGET, aml, ol);
    code.append(pci);
  }

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type
ex_function_char_length::pCodeGenerate(Space *space, UInt32 f)
{
  // Get a handle on the operands
  //
  AttributesPtr *attrs = getOperand();

  // Only support ascii strings for now
  CharInfo::CharSet cs = ((SimpleType *)getOperand(1))->getCharSet();

  if(cs != CharInfo::ISO88591)
    return ex_clause::pCodeGenerate(space, f);

  // Allocate the code list
  //
  PCIList code(space);

  // Generate pre clause PCI's
  PCode::preClausePCI(this, code);

  // Add the null logic if necessary.
  PCIID branchToEnd = PCode::nullBranch(this, code, attrs);

  // Just return length if the string is fixed
  if (DFS2REC::isSQLFixedChar(attrs[1]->getDatatype())) {
    AML aml(PCIT::MBIN32S, PCIT::IBIN32S);
    OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(),
          (Int32)attrs[1]->getLength());
    PCI pci(PCIT::Op_MOVE, aml, ol);
    code.append(pci);
  }
  else
  {
    UInt32 comboLen1=0;
    char* comboPtr1 = (char*)&comboLen1;

    // Use combo fields for specifying null/vc lengths of both operands
    comboPtr1[0] = (char)attrs[1]->getNullIndicatorLength(),
    comboPtr1[1] = (char)attrs[1]->getVCIndicatorLength();

    AML aml(PCIT::MBIN32U, PCIT::MATTR5);
    OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(),
          attrs[1]->getAtp(), attrs[1]->getAtpIndex(), attrs[1]->getOffset(),
            attrs[1]->getVoaOffset(), attrs[1]->getLength(), comboLen1);
    PCI pci(PCIT::Op_GENFUNC, aml, ol);
    code.append(pci);
  }

  // Add the branch target if necessary.
  //
  if(branchToEnd)
  {
    AML aml;
    OL ol((Int64)branchToEnd);
    PCI pci(PCIT::Op_TARGET, aml, ol);
    code.append(pci);
  }

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type
ExFunctionRepeat::pCodeGenerate(Space *space, UInt32 f)
{
  // Get a handle on the operands
  //
  AttributesPtr *attrs = getOperand();
  Attributes *dst = attrs[0];
  Attributes *src = attrs[1];

  if ( ( src->getScale() == SQLCHARSETCODE_UTF8 ) &&
       ( src->getPrecision() > 0 )                &&
       ( src->getPrecision() < src->getLength() ) )
     return( ex_expr::EXPR_NULL );    //pCode cannot handle this case

  //
  PCIList code(space);

  // Generate pre clause PCI's
  //
  PCode::preClausePCI(this, code);

  // handle NULLs (may jump)
  PCIID nullBranch = PCode::nullBranch(this, code, attrs);

  AML aml(PCIT::MATTR5,   // target ptr
          PCIT::MATTR5,   // source ptr
          PCIT::MBIN32S,  // len
          PCIT::IBIN32S); // OperTypeEnum

  UInt32 comboLen1=0;
  UInt32 comboLen2=0;
  char* comboPtr1 = (char*)&comboLen1;
  char* comboPtr2 = (char*)&comboLen2;

  // Use combo fields for specifying null/vc lengths of both operands
  comboPtr1[0] = (char)dst->getNullIndicatorLength(),
  comboPtr1[1] = (char)dst->getVCIndicatorLength();
  comboPtr2[0] = (char)src->getNullIndicatorLength(),
  comboPtr2[1] = (char)src->getVCIndicatorLength();

  OL ol(dst->getAtp(), dst->getAtpIndex(), dst->getOffset(),
          dst->getVoaOffset(), dst->getLength(), comboLen1,
        src->getAtp(), src->getAtpIndex(), src->getOffset(),
          src->getVoaOffset(), src->getLength(), comboLen2,
        attrs[2]->getAtp(), attrs[2]->getAtpIndex(), attrs[2]->getOffset(),
        ITM_REPEAT);

  PCI pci(PCIT::Op_GENFUNC, aml, ol);
  code.append(pci);

  // Add the branch target if necessary.
  //
  if (nullBranch)
    {
      AML aml;
      OL ol((Int64)nullBranch);
      PCI pci(PCIT::Op_TARGET, aml, ol);
      code.append(pci);
    }

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type
ex_function_position::pCodeGenerate(Space *space, UInt32 f)
{
  // Get a handle on the operands
  //
  AttributesPtr *attrs = getOperand();

  // Only support ascii strings for now
  CharInfo::CharSet cs = ((SimpleType *)getOperand(1))->getCharSet();

  if(cs != CharInfo::ISO88591)
    return ex_clause::pCodeGenerate(space, f);

  // pcode currently doesn't handle non-default start position or n'th occurrence
  if (getNumOperands() > 3)
    return ex_clause::pCodeGenerate(space, f);

  // We don't support system collations (e.g. czech).  Note, some clauses have
  // the collation defined in the clause.  Others don't - in which case the
  // info needs to be derived from the operand.

  CharInfo::Collation co = getCollation();
  if (CollationInfo::isSystemCollation(co))
    return ex_clause::pCodeGenerate(space, f);

  // Allocate the code list
  //
  PCIList code(space);

  // Generate pre clause PCI's
  PCode::preClausePCI(this, code);

  // Add the null logic if necessary.
  PCIID branchToEnd = PCode::nullBranch(this, code, attrs);

  UInt32 comboLen1=0;
  UInt32 comboLen2=0;
  char*  comboPtr1 = (char*)&comboLen1;
  char*  comboPtr2 = (char*)&comboLen2;

  // Use combo fields for specifying null/vc lengths of both operands
  comboPtr1[0] = (char)attrs[1]->getNullIndicatorLength(),
  comboPtr1[1] = (char)attrs[1]->getVCIndicatorLength();
  comboPtr2[0] = (char)attrs[2]->getNullIndicatorLength(),
  comboPtr2[1] = (char)attrs[2]->getVCIndicatorLength();

  AML aml(PCIT::MBIN32S, PCIT::MATTR5, PCIT::MATTR5);
  OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(),
        attrs[1]->getAtp(), attrs[1]->getAtpIndex(), attrs[1]->getOffset(),
          attrs[1]->getVoaOffset(), attrs[1]->getLength(), comboLen1,
        attrs[2]->getAtp(), attrs[2]->getAtpIndex(), attrs[2]->getOffset(),
          attrs[2]->getVoaOffset(), attrs[2]->getLength(), comboLen2);
  PCI pci(PCIT::Op_GENFUNC, aml, ol);
  code.append(pci);

  // Add the branch target if necessary.
  //
  if(branchToEnd)
  {
    AML aml;
    OL ol((Int64)branchToEnd);
    PCI pci(PCIT::Op_TARGET, aml, ol);
    code.append(pci);
  }

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type
ex_like_clause_base::pCodeGenerate(Space *space, UInt32 f)
{
  // Get a handle on the operands
  //
  AttributesPtr *attrs = getOperand();

  // Only support ascii strings for now
  CharInfo::CharSet cs = ((SimpleType *)getOperand(1))->getCharSet();


  //
  // The following criteria must be met for PCode to be generated for LIKE
  //
  // 1. Pattern string must be a constant
  // 2. Pattern can't contain any ESCAPE characters
  // 3. We can't use a collation for the comparison.
  // 4. Pattern can't contain any '_' characters
  // 5. Character set can't have a possibility of matching partial characters
  //    in a % wildcard (issue with SJIS, not an issue with UTF-8, due to the
  //    way UTF-8 is encoded where the first byte indicates the length and
  //    continuation bytes can never match the first byte of a UTF-8 character).
  // 6. Pattern string and UTF-8 char limits can't exceed 255 bytes in length.
  //    This is because both relative offsets and lengths of patterns are 
  //    stored in 32-bit containers divided into 4 unsigned chars.
  // 7. Pattern string must contain 1-4 non-zero-length sub-patterns which
  //    are separated by '%', with at least one '%'.  For example:
  //
  //      a) '%pat1%pat2%pat3'
  //      b) 'pat1%'
  //      c) 'pat1%pat2%%%pat3%'
  //
  //    Here are some examples of pattern strings that aren't supported:
  //
  //      a) 'sank'
  //      b) 'pat1%pat2%pat3%pat4%pat5%'
  //      c) '%'
  //
  // Criteria (1) and (2) were checked in the generator.  The rest must be done
  // here.  The member variable "pattern_" is used to retrieve the pattern char
  // string - this is registered, again, by the generator.
  //

  // (3) We don't support system collations (e.g. czech).  Note, some clauses have
  // the collation defined in the clause.  Others don't - in which case the
  // info needs to be derived from the operand.

  CharInfo::Collation co = ((SimpleType *)getOperand(1))->getCollation();
  if (CollationInfo::isSystemCollation(co))
    return ex_clause::pCodeGenerate(space, f);

  // (5) Allow only ISO and UTF8 charsets
  if(cs != CharInfo::ISO88591 && cs != CharInfo::UTF8)
    return ex_clause::pCodeGenerate(space, f);

  Int32 precision = 0;

  if (cs == CharInfo::UTF8)
    {
      precision = attrs[1]->getPrecision();
    }

  // Get pattern string and length
  char* pat = getPatternStr();
  Int32 patLen = attrs[2]->getLength();

  // (6) Generator restrictions met, as well as pattern length restriction
  if (noPCodeAvailable() || (patLen > UCHAR_MAX) || (precision > UCHAR_MAX))
    return ex_clause::pCodeGenerate(space, f);

  char   numOfPatterns = 0;
  Int32  patternOffsets = -1;
  Int32  patternLengths = -1;
  UInt8* pOffPtr = (UInt8*)&patternOffsets;
  UInt8* pLenPtr = (UInt8*)&patternLengths;

  Int32 i;
  NABoolean open = FALSE, percentFound = FALSE;

  for (i=0; i < patLen; i++)
  {
    // (4) No underscore allowed (i.e. no "any" character)
    if (pat[i] == '_')
      return ex_clause::pCodeGenerate(space, f);

    if (pat[i] == '%') {
      percentFound = TRUE;

      // If open pattern found, close it off by setting the pattern length
      if (open) {
        pLenPtr[numOfPatterns-1] = (UInt8)(i - pOffPtr[numOfPatterns-1]);
        open = FALSE;     // Indicate end of pattern
      }
    }
    else if (!open) {
      // (7a) Can only support 4 patterns - bail out otherwise
      if (numOfPatterns >= 4)
        return ex_clause::pCodeGenerate(space, f);

      // Otherwise start new pattern.  Assume max length of pattern
      pOffPtr[numOfPatterns] = (UInt8)i;
      pLenPtr[numOfPatterns] = (UInt8)(patLen - i);

      numOfPatterns++;  // Increase num of patterns
      open = TRUE;      // Indicate start of new pattern
    }
  }

  // (7b) Return if no pattern strings or no wildcard found
  if ((numOfPatterns == 0) || (percentFound == FALSE))
    return ex_clause::pCodeGenerate(space, f);

  // Allocate the code list
  //
  PCIList code(space);

  // Generate pre clause PCI's
  PCode::preClausePCI(this, code);

  // Add the null logic if necessary.
  PCIID branchToEnd = PCode::nullBranch(this, code, attrs);

  UInt32 comboLen1=0;
  UInt32 comboLen2=0;
  char*  comboPtr1 = (char*)&comboLen1;
  char*  comboPtr2 = (char*)&comboLen2;

  // Use combo fields for specifying null/vc lengths of both operands
  comboPtr1[0] = (char)attrs[1]->getNullIndicatorLength(),
  comboPtr1[1] = (char)attrs[1]->getVCIndicatorLength();
  comboPtr2[0] = (char)attrs[2]->getNullIndicatorLength(),
  comboPtr2[1] = (char)attrs[2]->getVCIndicatorLength();

  // Indicate if boundary checks are needed, and set other flags
  comboPtr1[2] |= (pat[0] != '%') ? ex_like_clause_base::LIKE_HEAD : 0;
  comboPtr1[2] |= (pat[patLen-1] != '%') ? ex_like_clause_base::LIKE_TAIL : 0;

  comboPtr1[3] = numOfPatterns;
  comboPtr2[2] = (UInt8) precision;

  AML aml(PCIT::MBIN32S, PCIT::MATTR5, PCIT::MATTR5,
          PCIT::IBIN32S, PCIT::IBIN32S);
  OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(),
        attrs[1]->getAtp(), attrs[1]->getAtpIndex(), attrs[1]->getOffset(),
          attrs[1]->getVoaOffset(), attrs[1]->getLength(), comboLen1,
        attrs[2]->getAtp(), attrs[2]->getAtpIndex(), attrs[2]->getOffset(),
          attrs[2]->getVoaOffset(), attrs[2]->getLength(), comboLen2,
        patternOffsets, patternLengths);
  PCI pci(PCIT::Op_GENFUNC, aml, ol);
  code.append(pci);

  // Add the branch target if necessary.
  //
  if(branchToEnd)
  {
    AML aml;
    OL ol((Int64)branchToEnd);
    PCI pci(PCIT::Op_TARGET, aml, ol);
    code.append(pci);
  }

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type
ex_function_extract::pCodeGenerate(Space *space, UInt32 f)
{
  // Get a handle on the operands
  //
  AttributesPtr *attrs = getOperand();

  // Only deal with datetime types, for now.  Also, no extracting seconds with
  // fractional precision - this requires slightly more work.
  if ((attrs[1]->getDatatype() != REC_DATETIME) ||
      (getExtractField() > REC_DATE_MAX_SINGLE_FIELD) ||
      (getExtractField()>=REC_DATE_CENTURY && getExtractField()<=REC_DATE_WOM) ||
      ((getExtractField() == REC_DATE_SECOND) && (attrs[1]->getScale() > 0)))
    return ex_clause::pCodeGenerate(space, f);

  // Determine what the offset is of the field to be extracted
  rec_datetime_field start;
  rec_datetime_field end;
  ExpDatetime *datetime = (ExpDatetime*) getOperand(1);
  if (datetime->getDatetimeFields(attrs[1]->getPrecision(), start, end) != 0)
    assert(FALSE);

  Int32 offset = 0, lastFieldSize = 0;

  for (Int32 field = start; field <= getExtractField(); field++) {
    offset += lastFieldSize;
    switch (field) {
      case REC_DATE_YEAR:
        lastFieldSize = 2;
        break;

      case REC_DATE_MONTH:
      case REC_DATE_DAY:
      case REC_DATE_HOUR:
      case REC_DATE_MINUTE:
      case REC_DATE_SECOND:
        lastFieldSize = 1;
        break;

      default:
        assert(FALSE);
    }
  }

  // Allocate the code list
  //
  PCIList code(space);

  // Generate pre clause PCI's
  PCode::preClausePCI(this, code);

  // Add the null logic if necessary.
  PCIID branchToEnd = PCode::nullBranch(this, code, attrs);

  PCIT::AddressingMode type = (getExtractField() == REC_DATE_YEAR)
                                ? PCIT::MBIN16U : PCIT::MBIN8;

  AML aml(PCIT::MBIN16U, type);
  OL ol(attrs[0]->getAtp(), attrs[0]->getAtpIndex(), attrs[0]->getOffset(),
        attrs[1]->getAtp(), attrs[1]->getAtpIndex(),
          attrs[1]->getOffset() + offset);
  PCI pci(PCIT::Op_MOVE, aml, ol);
  code.append(pci);

  // Add the branch target if necessary.
  //
  if(branchToEnd)
  {
    AML aml;
    OL ol((Int64)branchToEnd);
    PCI pci(PCIT::Op_TARGET, aml, ol);
    code.append(pci);
  }

  // Generate post clause PCI's
  //
  PCode::postClausePCI(this, code);

  setPCIList(code.getList());
  return ex_expr::EXPR_OK;
}

// ExpLOBoper::pCodeGenerate
//
// For now simply calls the default ex_clause::pCodeGenerate
//
ex_expr::exp_return_type ExpLOBoper::pCodeGenerate(Space *space, UInt32 f) 
{
  return ex_clause::pCodeGenerate(space, f);
}
