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
* File:         ExpPCodeExpGen.cpp
* RCS:          $Id: exppcodeexpgen.cpp,v 1.2 2006/11/21 05:53:06  Exp $
* Description:  
*
* Created:      8/25/97
* Modified:     $ $Date: 2006/11/21 05:53:06 $ (GMT)
* Language:     C++
* Status:       $State: Exp $
*
*
*
****************************************************************************
*/

#include "Platform.h"


#include "exp_stdh.h"
#include "str.h"
#include "exp_expr.h"
#include "exp_clause_derived.h"
#include "ExpPCode.h"
#include "ExpPCodeOptimizations.h"

#define GenAssert(p, msg) if (!(p)) { NAAssert(msg, __FILE__ , __LINE__ ); };

ex_expr::exp_return_type ex_expr::pCodeGenerate(Space * space,
                                                CollHeap *heap,
						UInt32 f) {

#ifdef _DEBUG
  if (getenv("NO_PCODE"))
    return ex_expr::EXPR_OK;
#endif

  // If the expression mode does not indicate that PCODE is "ON", then
  // don't generate any code. It is important not to allocate any
  // memory either since this may be the resource fork expression which
  // currently has a NULL space pointer.
  //
  if(!(pCodeMode_ & PCODE_ON)) {
    setPCodeObject(0);
    pCode_ = 0;
    return ex_expr::EXPR_OK;
  }

  // cannot proceed if no space was passed in.
  if (! space)
    return ex_expr::EXPR_OK;

  PCodeBinary *pCode = NULL;
  NABoolean versionOK = TRUE;
#ifdef _DEBUG
  // for debug
  char  *buf;
  Int32 bufLen = 0;  // to store PCode in ascii for to buf, set it to 16384
  Int32 dumpPci = 0; // set this to 1 as well
  if (bufLen > 0) {
    buf = (char*) space->allocateSpaceMemory(bufLen);
    if (buf == NULL)  // failed to get memory
      bufLen = 0;
  }
#endif

  if (pCode_.getPointer()) {
    pCode = getPCodeBinary();
    versionOK = pCode_.getPointer()->versionOK();
  } 
  if (versionOK && pCode && getPCodeGenCompile()) {
    return ex_expr::EXPR_OK;
  }
  else
    // If pcode is supposedly generated at compile-time, but there's no pcode,
    // set PCodeGenCompile flag to 0
    if (getPCodeGenCompile()) { 
      setPCodeGenCompile(0);
    }

  // Allocate the PCODE object and instruction list.
  //
  PCode *codeObject = new(heap)PCode(heap, space); 
  PCIList code(heap);

  ex_clause *clause = clauses_;

  //
  // Clear count of branch targets from any previous processing.
  //
  NABoolean branchingClausesExist = FALSE;
  clause = clauses_;
  while(clause) 
    {
      // copy handleIndirectVC flag into the clause
      clause->setHandleIndirectVC( handleIndirectVC() );

      // copy forInsertUpdate flag into the clause
      clause->setForInsertUpdate( forInsertUpdate() );

      if(clause->isBranchingClause()) 
	{
	  ex_branch_clause *branchClause = (ex_branch_clause*)clause;
	  ex_clause *targetClause = branchClause->get_branch_clause();
          targetClause->setNumberBranchTargets(0);
          branchingClausesExist = TRUE;
	}
      clause = clause->getNextClause();
    }

  // Handle clauses that branch. Set the target clause's flags to
  // indicate that the clause is a target of a branch. This operation
  // was previously performed in the clause::pCodeGenerate but this
  // did not allow for backwards branching since the target clause
  // would have already been processed.  A TARGET PCI instruction
  // will be added when PCI's are generated for the target clause.
  // (See ExpPCode.cpp:preClausePCI()).
  //

  clause = clauses_;
  if (branchingClausesExist)
  {
    while(clause) 
    {
      if(clause->isBranchingClause()) 
	{
	  ex_branch_clause *branchClause = (ex_branch_clause*)clause;
	  ex_clause *targetClause = branchClause->get_branch_clause();
	  targetClause->addBranchTarget();
	}
      clause = clause->getNextClause();
    }
  }
  
  // If the expression mode indicates "EVAL", then generate PCIs for
  // calling the standard clause->eval() routine. IF pcode generate
  // returns EXPR_NULL, something fatal occurred so abort PCODE
  // generation altogether. Currently, this happens if generate encounters
  // an access to a column following a VARCHAR column (does not work because
  // of a bug with computeDataPtr).
  //
  clause = clauses_;
  if (pCodeMode_ & PCODE_EVAL) {
    while(clause) {
      // Check if any of the operands can not generate pCode.
      // See ExpTupleDesc::computeOffsets() for when offsets are set to
      // UINT_MAX.  It is dependent on the tuple data format.
      // If offset is UINT_MAX, pcode is normally generated unless
      // CQD says otherwise or if this operation is for insert/update.
      for(short i = 0; i < clause->getNumOperands(); i++) {
	if (((clause->getOperand(i)->getOffset() == UINT_MAX) && 
              !handleIndirectVC()) ||                             
            (clause->getOperand(i)->isAddedCol() && 
              !pCodeSpecialFields()))
        {
	  setPCodeObject(0);
	  pCode_ = 0;
	  NADELETE(codeObject, PCode, heap);
	  return ex_expr::EXPR_OK;
	}
      }

      // Allow for a clause to return EXPR_NULL - unlikely, but keep for
      // future possibilities.
      if(clause->ex_clause::pCodeGenerate(space, f) == ex_expr::EXPR_NULL) {
	setPCodeObject(0);
	pCode_ = 0;
	NADELETE(codeObject, PCode, heap);
	return ex_expr::EXPR_OK;
      }
      code.append(PCIList(clause->getPCIList(), space));
      clause = clause->getNextClause();
    }
  } 
  // Otherwise, generate PCI instructions for each clause and only use
  // clause->eval() for unimplemented operations.
  //
  else {
    Attributes *tmpAttr = NULL;
    while(clause) {
      // Check if any of the operands can not generate pCode.
      // See ExpTupleDesc::computeOffsets() for when offsets are set to
      // UINT_MAX.  It is dependent on the tuple data format.
      // If offset is UINT_MAX, pcode is normally generated unless
      // CQD says otherwise or if this operation is for insert/update.
      for(short i = 0; i < clause->getNumOperands(); i++) {
        if (! clause->getOperand(i))
          continue;

	if (((clause->getOperand(i)->getOffset() == UINT_MAX) &&
              !handleIndirectVC()) ||
            (clause->getOperand(i)->isAddedCol() && 
              !pCodeSpecialFields()))
        {
	  setPCodeObject(0);
	  pCode_ = 0;
	  NADELETE(codeObject, PCode, heap);
	  return ex_expr::EXPR_OK;
	}
      }

      // If we can't generate PCode for a specific clause, then no PCode
      // is generated at all and we throw away what we've generated already.
      if(clause->pCodeGenerate(space, f) == ex_expr::EXPR_NULL) {
	setPCodeObject(0);
	pCode_= 0;
	NADELETE(codeObject, PCode, heap);
	return ex_expr::EXPR_OK;
      }

      code.append(PCIList(clause->getPCIList(), space));

      // If the target type in the clause is varchar, save the attribute for
      // update row length.
      // (But not if the varchar is being treated as a fixed field (forceFixed)).
      if(clause->getNumOperands() > 0)
      {
        if((clause->getOperand()[0])->getVCIndicatorLength() > 0 &&
           (clause->getOperand()[0])->isSQLMXDiskFormat() &&
           !(clause->getOperand()[0])->isForceFixed())
        {
            // save the attribute for updateRowlen
            tmpAttr = clause->getOperand()[0];
        }
      }

      // update row len when the current clause is the last one in the list
      // and one of the target type in a clause is varchar.

      if(clause->getNextClause() == NULL && tmpAttr)
      {
        code.append(PCode::updateRowLen(tmpAttr, space, f));
      }

      clause = clause->getNextClause();
    }
  }

#ifdef _DEBUG
  if (dumpPci == 1)
    PCode::dumpContents(code, buf, bufLen);
#endif

  // Fixup PCI CLAUSE_BRANCH instructions. Modify the corresponding TARGET
  // PCIs so that they point to the actual PCIs for the branch sources.
  // The actual branch offset fixup is done at the end of this procedure.
  //
  PCIListIter iter(code);
  PCI * pci;
  NABoolean containsClauseEval = FALSE;
  for(pci = iter.first(); pci; pci = iter.next()) 
    {
      if ((NOT containsClauseEval) &&
	  (PCode::IsClauseEvalInstruction(pci)))
	containsClauseEval = TRUE;

      if(PCode::IsBranchInstruction(pci))
        {
          PCIListIter iter2(code);
          for(PCI *tpci = iter2.first(); tpci; tpci = iter2.next()) {
   
            // A TARGET PCI with the same pointer as the CLAUSE_BRANCH PCI
            // indicates a branch-target pair. The target PCI operand needs
            // to be changed to point to the branch PCI.
            //
            if(PCode::IsTargetInstruction(tpci) &&
               (pci->getLongOperand(0) == tpci->getLongOperand(0)))
              {
                pci->setLongOperand(0, (Long)0);
                tpci->setLongOperand(0, (Long)pci);
                break;
              }
          }
        }
    }


#ifdef _DEBUG
  if (dumpPci == 1)
    PCode::dumpContents(code, buf, bufLen);
#endif

  // Fixup addresses. This requires a pass over
  // the PCODE instruction list. Any instruction that access memory
  // may need to be fixed up with the correct pointers. These operations
  // include:  Op_SUM.
  //
  // Also, use this pass over the PCODE instructions to compute the 
  // minimum and maximum ATP and ATP indexes that are accessed in 
  // LD/ST_ATP operations. This information is used later to make the
  // ATP/ATP Index map.
  //
  // Also, use this pass over the PCODE instructions to compute the
  // maximum PCODE stack depth needed. If this exceeds the stack depth
  // allocated in eval (which is currently fixed), then abort PCODE
  // generation.
  //
  Int32 maximumStackDepth = 0;
  Int32 minAtp = INT_MAX, maxAtp = INT_MIN;
  Int32 minAtpIndex = INT_MAX, maxAtpIndex = INT_MIN;
  Int32 atpOperation = 0;
  for(pci = iter.first(); pci; pci = iter.next()) {

    // Fixup constant, temp, and persistent pointers, if necessary.
    //
    if(getFixupConstsAndTemps())
      {
	Long tempsArea  = (Long)(tempsArea_.getPointer());
	Long constsArea = (Long)(constantsArea_.getPointer());
	Long persistentArea = (Long)(persistentArea_.getPointer());
	
	PCIT::AddressingMode am;
	for(Int32 i=0,op=0; 
	    i<pci->getNumberAddressingModes(); 
	    i++,op+=PCIT::getNumberOperandsForAddressingMode(am))
	  {
	    am = pci->getAddressingMode(i);
	    if(!PCIT::isMemoryAddressingMode(am)) continue;
	    
	    if(pci->getOperand(op+1) == 0)
	      pci->setLongOperand(op+2, pci->getLongOperand(op+2) - constsArea);
	    else if(pci->getOperand(op+1) == 1)
	      {
		if(pci->getOperand(op) == 0)
		  pci->setLongOperand(op+2, pci->getLongOperand(op+2) - tempsArea);
		else
		  pci->setLongOperand(op+2, 
				  pci->getLongOperand(op+2) - persistentArea);
	      }
	    
	  }
      }

    // Record min and max ATP/ATP Index accesses.
    //
    atpOperation = 1;
    PCIT::AddressingMode am;
    for(Int32 i=0,op=0; 
	i<pci->getNumberAddressingModes(); 
	i++,op+=PCIT::getNumberOperandsForAddressingMode(am))
      {
	am = pci->getAddressingMode(i);
	if(!PCIT::isMemoryAddressingMode(am)) continue;
	
	minAtp = MINOF(minAtp, pci->getOperand(op));
	maxAtp = MAXOF(maxAtp, pci->getOperand(op));
	minAtpIndex = MINOF(minAtpIndex, pci->getOperand(op+1));
	maxAtpIndex = MAXOF(maxAtpIndex, pci->getOperand(op+1));
      }
  }

#ifdef _DEBUG
  if (dumpPci == 1)
    PCode::dumpContents(code, buf, bufLen);
#endif


  // Do some optimization if required and possible:
  // 1 Eliminate move to temporary space, e.g.
  //   MV(A->B) and ADD(C=D+B) becomes ADD(C=D+A) and MV(A->B) removed.
  // 2 Eliminate redundant MOVE_MBIN8_MBIN8_IBIN32S to TEMPORARY SPACE, e.g.
  //   MV(A->B, sizeof(A)) and MV(B->C, sizeof(B)) becomes
  //   MV(A->C, sizeof(A))
  //   ?? Improvement: could we combine 1 and 2 in one loop?
  // 3 Combine contiguous move instructions into one, e.g.
  //   MV(A->B, sizeof(A)) and MV((A+1)->(B+1), sizeof(A)) becomes
  //   MV(A->B, 2*sizeof(A))
  // 4 Use standard size move (assignment) instruction if possible, e.g.
  //   MV(A->B, sizeof(A)) becomes MV(A->B) if sizeof(A) is 1, 2, 4, or 8.
  //   Do not do this optimization if pCodeMoveFastpath optimization could
  //   be done. That opt depends on the instruction being MOVE_MBIN8_MBIN8_IBIN32S.

  if (pCodeMode_ & PCODE_OPTIMIZE) 
    {
      NABoolean doPCodeMoveFastpathOpt = FALSE;
      if ((iter.first()) &&
	  (iter.last() == iter.first()) &&
	  (PCode::getInstruction(iter.first()) ==
	   PCIT::MOVE_MBIN8_MBIN8_IBIN32S))
	doPCodeMoveFastpathOpt = TRUE;

      PCI *storePci = iter.first();

#if (defined (_DEBUG))
      if(!getenv("PCODE_NO_MOVE_REMOVE"))
#endif
        // Case 1:
        // Keep in mind that we do not eliminate the MOVE (storePci)
        // if the temporary space is the target space of PCI inside
        // or not inside a branch. For example, in the sequece of
        // MOVE (TEMP <- A) ... ADD (TEMP = TEMP + B), the MOVE PCI
        // can not be removed.
        // Any MOVE PCI inside a branch will not be removed either,
        // as the PCI might be conditionally executed.
        {
          Int32 inBranch = 0;
          Int32 branchCnt = 0;
          const Int32 maxBranches = 256;
          PCI *branches[maxBranches];
      
          while (storePci)
            {

              if (PCode::IsBranchInstruction(storePci)) {

                Int32 found = 0;
                for (Int32 i=0; i<branchCnt; i++) {
                  if (storePci == branches[i]) {
                    inBranch--;
                    GenAssert (inBranch >= 0,
			"Expression contains unmatched branch and target instruction")
                   // branches[i] = branches[inBranch];
                    found = 1;
                    break;
                  }
                }

                if (!found) {
                  if (branchCnt > maxBranches - 1) {
                    // Too many branches, give up now and assume what
                    // optmization has done is correct
                    inBranch = 0;
                    break;  // from while (storePci) loop
                  }
                  branches[branchCnt] = storePci;
                  inBranch++;
                  branchCnt++;
                }
              }
              else if (PCode::IsTargetInstruction(storePci)) {

                PCIID branchId = storePci->getLongOperand(0);
                PCI *branch = PCode::getPCI(branchId);
            
                Int32 found = 0;
                for (Int32 i=0; i<branchCnt; i++) {
                  if (branch == branches[i]) {
                    inBranch--;
                    GenAssert (inBranch >= 0,
			"Expression contains unmatched branch and target instruction")
                    found = 1;
                    break; // Should have no other branch instruction
                           // points to this target instruction
                  }
                }
            
                if (!found) {
                  // encounted the target before the branch instruction
                  if (branchCnt > maxBranches - 1) {
                    // Too many branches, give up now and assume what
                    // optmization has done is correct
                    inBranch = 0;
                    break;  // from while (storePci) loop
                  }
                  branches[branchCnt] = branch;
                  inBranch++;
                  branchCnt++;
                }
              }

              // If this instruction stores data to temporary space and
              // it is not in branch
              //
              else if (!inBranch && PCode::IsTemporaryStore(storePci))
                {
                  Int32 replaceUses = 1;
                  PCIListIter iterInner(iter);
                  PCI *usePci = iterInner.next();
                  for (; usePci; usePci = iterInner.next())
                    {
                      if (PCI::replaceUsesOfTarget(storePci, usePci, 1)) {
                        replaceUses = 0;
                        // since we can not eliminate the use of temp space
                        break;  // out from the for loop
                      }
                    }

                  if (replaceUses) {
                    // now we know we can at least replace the use of
                    // temp space. Replace it and see if we can remove it,
                    // i.e. no place the temp space is ever referenced
                    Int32 removeMove = 1;
                    PCIListIter iterInner(iter);
                    PCI *usePci = iterInner.next();
                    for (; usePci; usePci = iterInner.next())
                      {
                        if (PCI::replaceUsesOfTarget(storePci, usePci, 0)) {
                          // can not remove the store PCI
                          removeMove = 0;
                        }
                      }

                    if (removeMove) {
                      code.remove(iter.currItem());
                    }
                  }
                }  // If it's temporay store instruction and not in branch
                   // else check the next instruction

              storePci = iter.next();
            } // while (storePci)
#ifdef _DEBUG
          GenAssert(inBranch == 0, "Unmatched branch and target instruction")
#endif
        }  // !getenv("PCODE_NO_MOVE_REMOVE")

      // Case 2
      // Eliminate redundant MOVE_MBIN8_MBIN8_IBIN32S (to/from TEMPORARY SPACE) pairs
      // from ex_expr lists. All move operations between identical datatypes 
      // result in the generation of this move instruction. 
      // Once a matching MOVE pair is identified we scan the remaining
      // part of the list to ensure that no other instruction accesses the temporary 
      // location we are about to eliminate. 

      // And, combine contiguous move instructions.
      //

      storePci = iter.first();
      while(storePci)
	{
	  // If this instruction does not store data to temporary space,
	  // try the next one.
	  //
	  if(!PCode::IsTemporaryStore(storePci))
	    {
	      storePci = iter.next();
	      continue;
	    }

	  Int32 match = 0;
	  PCIListIter iterInner(iter);
          PCI *loadPci = iterInner.next();
	  for(; loadPci; 
	      loadPci = iterInner.next()) 
	    {
                if(PCode::IsBranchOrTarget(loadPci)) break;

                if(PCI::temporaryStoreLoadOverlap(storePci, loadPci))  
                {
                  if(PCode::getInstruction(loadPci) != PCIT::MOVE_MBIN8_MBIN8_IBIN32S) 
                    break ;  // an instruction other than a move access the same temporary
                             // space as the move in storepci

	          // Else if Move(store) and Move(load) are
	          // to/from the same location, see if the pair can be removed.
	          //
	          if(PCI::temporaryStoreLoadMatch(storePci, loadPci)) 
		  {
		    Int32 anotherLD = 0;
		    PCIListIter iterInnerInner(iterInner);
		    for(PCI *lastPci = iterInnerInner.next(); lastPci;
		        lastPci = iterInnerInner.next()) 
		      {
		        if(PCI::temporaryStoreLoadOverlap(storePci, lastPci)) 
			  {
			    anotherLD = 1;
			    break;
			  }
		      }
		    if(!anotherLD) match = 1;
		    break;
		  }
                  else   // not loadStoreMatch but is a MOVE_MBIN8_MBIN8_IBIN32S
                    break;
                }
               else  //not LoadStoreOverlap
                 continue;
	      }

	    // If a matching pair was not found, try the next instruction.
	    //
	    if(!match)
	      {
	        storePci = iter.next();
	        continue;
	      }

	  // We have found store and load operations to temporary space
	  // that are redundant.
	  //
	  // MV-MV becomes MV. 

	    else {
	      storePci->setOperand(0, loadPci->getOperand(0));
	      storePci->setOperand(1, loadPci->getOperand(1));
	      storePci->setOperand(2, loadPci->getOperand(2));
	      code.remove(iterInner.currItem());
	    }
          }


      // Case 3:
      // Now do two optimizations in this loop
      // a. Combine contiguous move operations into one.
      // b. Replace standard size moves with specific move instructions
      // c. Remove RANGE_MFLT64 instructions which companion MOVE MFLT64
      //    instructions were removed at previous steps
      //
      for(storePci = iter.first(); storePci; storePci = iter.next()) 
	{
	  // If this instruction is a MV, see if it can be combined with the
	  // next instruction if it is also a MV.
	  //
        if(PCode::getInstruction(storePci) == PCIT::MOVE_MBIN8_MBIN8_IBIN32S) 
	    {
	      PCIListIter iterNext(iter);

              // Don't do this optimization if LLO optimization is specified
	      for(PCI *nextPci = iterNext.next();
                  nextPci && !(pCodeMode_ & PCODE_LLO); 
		  nextPci=iterNext.next()) 
		{
                    if(PCode::getInstruction(nextPci) != PCIT::MOVE_MBIN8_MBIN8_IBIN32S) break;

		  // If the two MV's are contiguous, combine them.
		  // The DST ATP's must be the same, and the SRC
		  // ATP's must be the same, and the second DST
		  // start must be equal to the first DST end, and
		  // the second SRC start must be equal to the
		  // first SRC end.
		  //
                  if((nextPci->getOperand(0) == storePci->getOperand(0)) &&
		     (nextPci->getOperand(1) == storePci->getOperand(1)) &&
		     (nextPci->getOperand(3) == storePci->getOperand(3)) &&
		     (nextPci->getOperand(4) == storePci->getOperand(4)) &&
		     (nextPci->getOperand(2) == (storePci->getOperand(2) 
						 + storePci->getOperand(6))) &&
		     (nextPci->getOperand(5) == (storePci->getOperand(5) 
						 + storePci->getOperand(6)))) 
		    {
      
		      // The length of the combined MV is the sum
		      // of the two MV's.
		      //
		      storePci->setOperand(6, storePci->getOperand(6) 
					   + nextPci->getOperand(6));

		      // Remove the second MV.
		      //
		      code.remove(iterNext.currItem());
		    }
		  else 
		    {
		      break;
		    }
		}

                // Next try to replace standard size moves with specific
                // move instructions for these sizes.
                // Standard Sizes are: 1, 2, 4 and 8 byte moves.
                // This avoids doing a str_cpy_all for these standard sizes.
                //
#if (defined (_DEBUG))
                if (!getenv("PCODE_NO_STD_MOVE"))
#endif
                  {
                  if (NOT doPCodeMoveFastpathOpt)
                    {
                    Lng32 length = storePci->getOperand(6);

                    if (length == 1 ||
                        length == 2 ||
                        length == 4 ||
                        length == 8)
                      {
                        PCIType::AddressingMode am = PCIT::AM_NONE;

                        // Determine the proper addressing mode for this size.
                        //
                        switch(length)
                          {
                          case 1:
                            am = PCIT::MBIN8;
                            break;
                          case 2:
                            am = PCIT::MBIN16U;
                            break;
                          case 4:
                            am = PCIT::MBIN32U;
                            break;
                          case 8:
                            am = PCIT::MBIN64S;
                            break;
                          }

                        // Addressing modes for move instruction.
                        // These are always 'same-size' to 'same-size' moves.
                        //
                        AML aml(am, am);

                        // Location of data is the same as for original
                        // move instruction.  but we no longer need the
                        // length operand.
                        //
                        OL ol(storePci->getOperand(0),
                              storePci->getOperand(1),
                              storePci->getOperand(2),
                              storePci->getOperand(3),
                              storePci->getOperand(4),
                              storePci->getOperand(5));

                        // Change the generic move instuction to a standard
                        // move.
                        storePci->replaceAddressingModesAndOperands(aml, ol);
                      }
	            }   // if
	          }
	    }    // if PCIT::MOVE_MBIN8_MBIN8_IBIN32S
        else
#if (defined (_DEBUG))
             if(!getenv("PCODE_NO_FLOAT_RANGE_REMOVE"))
            {
#endif
              if(PCode::getInstruction(storePci) == PCIT::RANGE_MFLT64)
                {
                  PCIListIter iterRange(iter);
                  PCI *rangePci = iterRange.next();
                  for (; rangePci; rangePci = iterRange.next())
                    {
                      if(PCode::IsBranchOrTarget(rangePci)) break;

                      if(PCode::getInstruction(rangePci) == PCIT::RANGE_MFLT64)
                        {
                          if((rangePci->getOperand(0) == storePci->getOperand(0)) &&
                             (rangePci->getOperand(1) == storePci->getOperand(1)) &&
                             (rangePci->getOperand(2) == storePci->getOperand(2)))
                            {
                              code.remove(iterRange.currItem());
                            }
                        }
                    }
                }
#if (defined (_DEBUG))
            }  // !getenv("PCODE_NO_FLOAT_RANGE_REMOVE")
#endif

	}   // combine contiguous moves and use standard size move

    } // PCODE_OPTIMIZE

#ifdef _DEBUG
  if (dumpPci == 1)
    PCode::dumpContents(code, buf, bufLen);
#endif

  // Since the call to get the base data address for a tupp is relatively
  // expensive (triple dereference along with relocation checking), we want
  // to minimize the number of calls to getDataPointer. This is accomplished
  // be loading the base data address for each tupp that is possibly
  // accessed by the expression once before evaluating the expression. The
  // pointers are stored at the top of the expression stack and all
  // ATP references in the expression use the address pointers from the
  // stack.
  //
  // Make a map of the accessed tupps. The mapping function subtracts the
  // min ATP from the ATP and the min ATP index from the ATP index and then
  // maps the resuling shifted ATP and ATP index into a 2-D array. This
  // compacts the map somewhat.
  //
  Int32 opNum = 0;
  Int32 *atpMap = 0;
  Int32 *atpIndexMap = 0;
  if(atpOperation) {
    Int32 numAtps = (maxAtp - minAtp) + 1;
    Int32 numAtpIndexs = (maxAtpIndex - minAtpIndex) + 1;
    Int32 numOps = numAtps * numAtpIndexs;
    char *tuppMap = new(space) char[numOps];
    atpMap = new(space) Int32[numOps];
    atpIndexMap = new(space) Int32[numOps];
    Int32 i=0;
    for(; i<numOps; i++) {
      atpMap[i] = -1;
      atpIndexMap[i] = -1;
      tuppMap[i] = 0;
    }

    // Iterate over all of the LD/ST_ATP* operations.
    // Mark in the map the accessed tupps and change the LD/ST_ATP* operand
    // to be the computed index.
    //
    for(pci = iter.first(); pci; pci = iter.next()) {
      Int32 atp, atpIndex, index;
      PCIT::AddressingMode am;
      for(Int32 i=0,op=0; 
	  i<pci->getNumberAddressingModes(); 
	  i++,op+=PCIT::getNumberOperandsForAddressingMode(am))
	{
	  am = pci->getAddressingMode(i);
	  if(!PCIT::isMemoryAddressingMode(am)) continue;
	  
	  atp = pci->getOperand(op);
	  atpIndex = pci->getOperand(op+1);
	  index = numAtpIndexs * (atp - minAtp) + (atpIndex - minAtpIndex);
	  tuppMap[index] = 1;
	  atpMap[index] = atp;
	  atpIndexMap[index] = atpIndex;
	}
    }


    // Compress the array of ATPs and ATP indexes to those that are actually
    // accessed.
    //
    for(i=0; i<numOps; i++) {
      if(tuppMap[i] && (atpIndexMap[i] > 1)) {
        tuppMap[i] = opNum + 4;
        atpMap[opNum] = atpMap[i];
        atpIndexMap[opNum] = atpIndexMap[i];
        opNum++;
      } else if(tuppMap[i]) {
	if(atpMap[i] == 0)
	  tuppMap[i] = atpIndexMap[i] + 1;
	else
	  tuppMap[i] = 3;
      }
    }

    // Iterate over all of the LD/ST_ATP* operations.
    //
    for(pci = iter.first(); pci; pci = iter.next()) {
      Int32 atp, atpIndex, index, compressedIndex;
      PCIT::AddressingMode am;
      for(Int32 i=0,op=0; 
	  i<pci->getNumberAddressingModes(); 
	  i++,op+=PCIT::getNumberOperandsForAddressingMode(am))
	{
	  am = pci->getAddressingMode(i);
	  if(!PCIT::isMemoryAddressingMode(am)) continue;
	  
	  // Compute the index into the tupp map.
	  //
	  atp = pci->getOperand(op);
	  atpIndex = pci->getOperand(op+1);
	  index = numAtpIndexs * (atp - minAtp) + (atpIndex - minAtpIndex);
	  compressedIndex = tuppMap[index];
	  
	  // Store the new index as the ith operands and move the
	  // offset from the (i+2)th to the (i+1)th operand, etc.
	  //
	  pci->setOperand(op, compressedIndex);
	  for(Int32 j=op+1; j<pci->getNumberOperands()-1; j++)
	    pci->setOperand(j, pci->getOperand(j+1));
	  pci->setNumberOperands(pci->getNumberOperands()-1);
	  op--;
	}
    }
  }
    
  // There is only a fixed amount of PCODE stack space available for the
  // expression evaluation at runtime. This is currently fixed to
  // 256 integers. If the number of tupp pointers plus the maximum stack
  // depth exceeds 256, then abort PCODE generation because a PCODE stack
  // overflow is possible at runtime.
  //
  if(4 + opNum + maximumStackDepth >= 256) {
    setPCodeObject(0);
    pCode_ = 0;
    NADELETE(codeObject, PCode, heap);
    return ex_expr::EXPR_OK;
  }

  // Fixup the branch targets (no more code changes after this).
  //
  // Compute the offset in the generated code vector of the first element
  // for each PCI.
  //
  Int32 position = 0;
  for(pci = iter.first(); pci; pci = iter.next()) {
    pci->setCodePosition(position);
    position += pci->getGeneratedCodeSize();
  }
  // Based on the offset for the branch and target PCI's, compute and set 
  // the branch distance in the branch instruction.
  //
  for(pci = iter.first(); pci; pci = iter.next()) {
    if(PCode::IsTargetInstruction(pci)) {
      PCIID branchId = pci->getLongOperand(0);
      PCI *branchPci = PCode::getPCI(branchId);
      Int32 distance = pci->getCodePosition() - branchPci->getCodePosition() - 1;
      if((branchPci->getOperation() == PCIT::Op_CLAUSE_BRANCH) ||
	 (branchPci->getOperation() == PCIT::Op_BRANCH_AND) ||
	 (branchPci->getOperation() == PCIT::Op_BRANCH_OR))
	branchPci->setLongOperand(0, distance);
      else
	branchPci->setLongOperand(branchPci->getNumberOperands()-1, distance);
    }
  }

  //#ifndef NDEBUG
#ifdef _DEBUG
  if(getenv("PCODE_PRINT")) {
    fprintf(stderr, "PCode ...\n");
    PCode::print(code);
    fprintf(stderr, "\n");
  }
#endif

#ifdef _DEBUG
  if (dumpPci == 1)
    PCode::dumpContents(code, buf, bufLen);
#endif

  codeObject->setPCIList(code);
  setPCodeObject(codeObject);
  if (!pCode_.getPointer())
    pCode_ = new(space) PCodeSegment();

  Int32 codeSize;
  setPCodeBinary(codeObject->generateCodeSegment(opNum, atpMap, atpIndexMap, 
           &codeSize));
  pCode_.getPointer()->setPCodeSegmentSize(codeSize);
  pCode_.getPointer()->setContainsClauseEval(containsClauseEval);

#ifdef _DEBUG
  if (dumpPci == 1)
    PCode::dumpContents(pCode_.getPointer()->getPCodeBinary(), buf, bufLen);
#endif

  // get rid of the temporary object
  if (! ex_expr_base::forShowplan(f)) 
    setPCodeObject(0);

  // Perform PCODE optimizations
  if (pCodeMode_ & PCODE_LLO) {
    PCodeCfg* cfg = new(heap) PCodeCfg(this, atpMap, atpIndexMap, heap, space);
    cfg->optimize();
    NADELETE(cfg, PCodeCfg, heap);
  }

  // see if move fastpath could be done.
  // In this case, we move source to target directly without going
  // through expr eval at runtime.
  // Done if there is only one pcode instruction, MOVE_MBIN8_MBIN8_IBIN32S.
  // 
  PCodeBinary *pc = getPCodeBinary();
#ifdef _DEBUG
  if (getenv("NO_PCODE_MOVE_FASTPATH"))
    return ex_expr::EXPR_OK;
#endif
  if (pc)
    {
      if ((pc[0] == 2) &&
	  (pc[5] == PCIT::MOVE_MBIN8_MBIN8_IBIN32S) &&
          ((pc[11] == PCIT::END) || (pc[11] == PCIT::RETURN)))
	{
	  setPCodeMoveFastpath(TRUE);
	}
    }

  return ex_expr::EXPR_OK;
};

void ex_expr::pCodePrint() {
  PCode *pc = getPCodeObject();
  if(pc) {
    if(pc->isProfilingOn()) {
     // fprintf(stderr, "Expression ??? \n");
      pc->profilePrint();
    }
  }
};
