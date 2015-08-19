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

#include "exp_ovfl_ptal.h"
#include "exp_ieee.h"
#include "BigNumHelper.h"
#include "ExpPCodeOptimizations.h"

//
// Perform constant propogation across all blocks.
//
NABoolean PCodeCfg::constantPropagation(NABoolean doPeeling)
{
  NABoolean notAllVisited, restart=FALSE;

  CollIndex i;

  // Mark all blocks as unvisited
  clearVisitedFlags();

  // Algorithm:
  //
  // Visit each block after all predecesors of that block have been visited.
  // For each block, merge constants from all pred blocks coming into the block.
  // 
  do
  {
    notAllVisited = FALSE;

    FOREACH_BLOCK_REV_DFO(block, firstInst, lastInst, index)
    {
      // If the block has already been processed, continue to the next block.
      if (block->getVisitedFlag())
        continue;

      const BLOCKLIST& preds = block->getPreds();

      // The constant propagation algorithm could result in dead blocks.  If
      // we process those dead blocks, it could result in incorrect propagation
      // information.
      if ((preds.entries() == 0) && (block != entryBlock_)) {
        deleteBlock(block);
        continue;
      }

      // Declare constant vectors used for this block.
      NABitVector zeroes(heap_), ones(heap_), neg1(heap_);

      // Visit each predecessor and merge in their constants.
      for (i=0; i < preds.entries(); i++)
      {
        if (preds[i]->getVisitedFlag() == FALSE) {
          notAllVisited = TRUE;
          break;
        }

        // Declare temp constant vectors used for merging purposes.
        NABitVector tempZeroes(heap_), tempOnes(heap_), tempNeg1(heap_);

        // Initialize the temp vectors with the predecessor's constant vectors.
        tempZeroes = preds[i]->zeroesVector;
        tempOnes = preds[i]->onesVector;
        tempNeg1 = preds[i]->neg1Vector;

        // Set implicit constants derived from knowing which target a branch 
        // took. 
        preds[i]->fixupConstantVectors(block, tempZeroes, tempOnes, tempNeg1);

        // If this is the first pred, initialize the block's constant vectors
        // to that of the preds.

        if (i == 0) {
          zeroes = tempZeroes;
          ones = tempOnes;
          neg1 = tempNeg1;
          continue;
        }

        // Merge the preds constants into that of this blocks.
        PCodeConstants::mergeConstantVectors(zeroes, ones, neg1,
                                             tempZeroes, tempOnes, tempNeg1);
      }

      // Bail out if not all preds are ready..
      if (i != preds.entries())
        continue;

      // If this is the entry block, initialize this block's constant vectors
      // with the known constant's vectors of the cfg.
      if (block == entryBlock_) {
        zeroes += *zeroes_;
        ones += *ones_;
        neg1 += *neg1_;
      }

      restart = localConstantPropagation(block, zeroes, ones, neg1) || restart;

      block->setVisitedFlag(TRUE);

      // Set the block's constant vectors to the local ones used here.
      block->zeroesVector = zeroes;
      block->onesVector = ones;
      block->neg1Vector = neg1;

      if (doPeeling && block->doesBlockQualifyForShortCircuit()) {
        PCodeBlock *t1, *t2;

        CollIndex entries = zeroes.entries();

#if 0
        CollIndex j;

        printf("Block %d:\n", block->getBlockNum());

        printf("  Zeroes: ");
        j = zeroes.getLastStaleBit();
        for (; (j = zeroes.prevUsed(j)) != NULL_COLL_INDEX; j--)
        {
          PCodeOperand* operand = getMap()->getFirstValue(&j);
          operand->print();
          printf ("(%d) ", j);
        }
        printf("\n");

        printf("  Ones: ");
        j = ones.getLastStaleBit();
        for (; (j = ones.prevUsed(j)) != NULL_COLL_INDEX; j--)
        {
          PCodeOperand* operand = getMap()->getFirstValue(&j);
          operand->print();
          printf ("(%d) ", j);
        }
        printf("\n");

        printf("  Neg1: ");
        j = neg1.getLastStaleBit();
        for (; (j = neg1.prevUsed(j)) != NULL_COLL_INDEX; j--)
        {
          PCodeOperand* operand = getMap()->getFirstValue(&j);
          operand->print();
          printf ("(%d) ", j);
        }
        printf("\n");
#endif

        t1 = shortCircuitOptForBlock(block, TRUE, TRUE, zeroes, ones, neg1);

        assert (zeroes.entries() == entries);

        // If peeled block was generated, mark visited flag since known
        // constants were updated (and set) in target block.
        if (t1)
          t1->setVisitedFlag(TRUE);

        t2 = shortCircuitOptForBlock(block, FALSE, TRUE, zeroes, ones, neg1);

        // If peeled block was generated, mark visited flag since known
        // constants were updated (and set) in target block.
        if (t2)
          t2->setVisitedFlag(TRUE);

        assert (zeroes.entries() == entries);

        restart = restart || (t1 != NULL) || (t2 != NULL);
      }

    } ENDFE_BLOCK_REV_DFO
  } while (notAllVisited);

  return restart;
}



NABoolean PCodeCfg::localConstantPropagation(PCodeBlock* block,
                                             NABitVector& zeroes,
                                             NABitVector& ones,
                                             NABitVector& neg1)
{
  CollIndex i;
  PCodeOperand *operand1, *operand2;
  CollIndex bvIndex1, bvIndex2;
  PCodeInst* returnInst = NULL;
  Int32 returnValue = PCodeConstants::UNKNOWN_CONSTANT;
  Int32 constant, constant2;

  NABoolean isZero, isOne, isNeg1;
  NABoolean graphChanged = FALSE;

  // Use zeroes, ones, and neg1 to start constant propagating and optimizing
  // this basic block
  FOREACH_INST_IN_BLOCK(block, inst)
  {
    PCIT::Instruction opc = (PCIT::Instruction) inst->getOpcode();
    PCodeBinary* code = inst->code;
    NABoolean restart = TRUE;

    switch (inst->getOpcode()) {
      case PCIT::MOVE_MBIN16U_IBIN16U:
      case PCIT::MOVE_MBIN32S_IBIN32S:
      case PCIT::MOVE_MBIN8_MBIN8:
      case PCIT::MOVE_MBIN16U_MBIN16U:
      case PCIT::MOVE_MBIN32U_MBIN32U:
      case PCIT::MOVE_MBIN64S_MBIN64S:
        // These cases actually use const info regarding the write operands
        // in order to optimize.  Let them clear out the write operands
        // themselves.
        break;

      default:
      {
        // Clear out constant assumptions for all writes
        for (i=0; i < inst->getWOps().entries(); i++) {
          CollIndex bvIndex = inst->getWOps()[i]->getBvIndex();
#if 0
          // TODO: May need to fix this so that source operands not cleared
          // as well
          for (j=0; j < inst->getROps().entries(); j++) {
            if ((bvIndex == inst->getROps()[j]->getBvIndex()) &&
               (PCodeConstants::getConstantValue(bvIndex, zeroes, ones, neg1) !=
                PCodeConstants::UNKNOWN_CONSTANT))
              assert(FALSE);
          }
#endif
          // Special logical branches are created for the purposes of predicate
          // reordering.  Ignore these cases.
          if (inst->isLogicalBranch())
            if (bvIndex == inst->getROps()[0]->getBvIndex())
              continue;

          PCodeConstants::clearConstantVectors(bvIndex, zeroes, ones, neg1);
        }
      }
    }

    switch (opc)
    {
      case PCIT::MOVE_MBIN16U_IBIN16U:
      case PCIT::MOVE_MBIN32S_IBIN32S:
      {
        operand1 = inst->getWOps()[0];
        bvIndex1 = operand1->getBvIndex();
        constant = inst->code[3];

        constant2 = PCodeConstants::getConstantValue(bvIndex1, zeroes,
                                                     ones, neg1);

        // Delete moves whose constants are already known.
        if ((constant2 != PCodeConstants::UNKNOWN_CONSTANT) &&
            (constant == constant2))
        {
          block->deleteInst(inst);
          break;
        }

        // Clear out vector first since it's not done initially before switch
        PCodeConstants::clearConstantVectors(bvIndex1, zeroes, ones, neg1);

        PCodeConstants::setConstantInVectors(constant, bvIndex1,
                                             zeroes, ones, neg1);

        // Use constant operand instead for potential copy prop
        if (constant == 0) {
          PCodeInst* mv;
          if (opc == PCIT::MOVE_MBIN16U_IBIN16U)
            mv = block->insertNewInstAfter(inst, PCIT::MOVE_MBIN16U_MBIN16U);
          else
            mv = block->insertNewInstAfter(inst, PCIT::MOVE_MBIN32U_MBIN32U);

          mv->code[1] = inst->code[1];
          mv->code[2] = inst->code[2];
          mv->code[3] = 1;
          mv->code[4] = zeroOffset_;

          mv->reloadOperands(this);

          // Don't need the old NULL instruction any more.
          block->deleteInst(inst);
        }

        break;
      }

      case PCIT::NULL_VIOLATION_MBIN16U:
        operand1 = inst->getROps()[0];
        zeroes += operand1->getBvIndex();
        break;

      case PCIT::NULL_VIOLATION_MPTR32_MPTR32_IPTR_IPTR:
      case PCIT::NULL_VIOLATION_MBIN16U_MBIN16U:
      {
        operand1 = inst->getROps()[0];
        zeroes += operand1->getBvIndex();

        if ((opc != PCIT::NULL_VIOLATION_MPTR32_MPTR32_IPTR_IPTR) ||
            (GetPCodeBinaryAsPtr(inst->code, 5 + PCODEBINARIES_PER_PTR) != 0))
        {
          operand2 = inst->getROps()[1];
          zeroes += operand2->getBvIndex();
        }
        break;
      }

      case PCIT::REPLACE_NULL_MATTR3_MBIN32S:
      case PCIT::REPLACE_NULL_MATTR3_MBIN32U:
      case PCIT::REPLACE_NULL_MATTR3_MBIN16S:
      case PCIT::REPLACE_NULL_MATTR3_MBIN16U:
      {
        bvIndex2 = inst->getROps()[0]->getBvIndex();
        constant = PCodeConstants::getConstantValue(bvIndex2,zeroes,ones,neg1);
        if (constant != PCodeConstants::UNKNOWN_CONSTANT) {
          inst->code[0] = (inst->getWOps()[0]->getLen() == 2)
                             ? PCIT::MOVE_MBIN16U_MBIN16U
                             : PCIT::MOVE_MBIN32U_MBIN32U;

          if (constant == 0) {
            inst->code[3] = inst->code[6];
            inst->code[4] = inst->code[7];
          }
          else {
            inst->code[3] = inst->code[8];
            inst->code[4] = inst->code[9];
          }

          inst->reloadOperands(this);

          // For some reason the src types are MASCII - change it.
          if (inst->code[0] == PCIT::MOVE_MBIN16U_MBIN16U) {
            inst->getWOps()[0]->setType(PCIT::MBIN16U);
            inst->getROps()[0]->setType(PCIT::MBIN16U);
          }
          else {
            inst->getWOps()[0]->setType(PCIT::MBIN32U);
            inst->getROps()[0]->setType(PCIT::MBIN32U);
          }

          RESTART_INST_IN_BLOCK;
        }
        break;
      }

      case PCIT::NULL_MBIN16U:
      {
        PCodeInst* mv;

        // Propagate the known zero constant
        bvIndex1 = inst->getWOps()[0]->getBvIndex();
        PCodeConstants::setConstantInVectors(0, bvIndex1, zeroes, ones, neg1);

        // Convert NULL into MOVE of the zero constant - good for copy prop
        mv = block->insertNewInstBefore(inst, PCIT::MOVE_MBIN16U_MBIN16U);
        mv->code[1] = inst->code[1];
        mv->code[2] = inst->code[2];
        mv->code[3] = 1;
        mv->code[4] = zeroOffset_;

        mv->reloadOperands(this);

        // Don't need the old NULL instruction any more.
        block->deleteInst(inst);

        break;
      }

      case PCIT::NULL_BITMAP:
        bvIndex1 = inst->getWOps()[0]->getBvIndex();
        if (inst->code[3] == PCIT::NULL_BITMAP_SET) {
          constant = (inst->code[5]) ? -1 : 0;
          PCodeConstants::setConstantInVectors(constant, bvIndex1,
                                               zeroes, ones, neg1);
        }
        break;


      case PCIT::MOVE_MBIN32S:
        bvIndex1 = inst->getROps()[0]->getBvIndex();
        constant = PCodeConstants::getConstantValue(bvIndex1,zeroes,ones,neg1);
        if (constant != PCodeConstants::UNKNOWN_CONSTANT) {
          returnInst = inst;
          returnValue = constant;
        }
        break;

      case PCIT::RETURN:
        if (returnInst) {
          PCodeInst* ret;
          ret = block->insertNewInstBefore(inst, PCIT::RETURN_IBIN32S);
          ret->code[1] = returnValue;

          block->deleteInst(returnInst);
          block->deleteInst(inst);
        }
        break;

      case PCIT::MOVE_MBIN8_MBIN8_IBIN32S:
      {
        operand1 = inst->getROps()[0];

        switch (operand1->getLen()) {
          case 1:
            inst->code[0] = PCIT::MOVE_MBIN8_MBIN8;
            break;
          case 2:
            inst->code[0] = PCIT::MOVE_MBIN16U_MBIN16U;
            break;
          case 4:
            inst->code[0] = PCIT::MOVE_MBIN32U_MBIN32U;
            break;
          case 8:
            inst->code[0] = PCIT::MOVE_MBIN64S_MBIN64S;
            break;
        }

        if (inst->code[0] != PCIT::MOVE_MBIN8_MBIN8_IBIN32S) {
          inst->reloadOperands(this);
          RESTART_INST_IN_BLOCK;
        }
        else {
          // Just propagate the constants over
          operand2 = inst->getWOps()[0];
          PCodeConstants::copyConstantVectors(operand1->getBvIndex(),
                                              operand2->getBvIndex(),
                                              zeroes, ones, neg1);
        }

        break;
      }

      case PCIT::MOVE_MBIN8_MBIN8:
      case PCIT::MOVE_MBIN16U_MBIN16U:
      case PCIT::MOVE_MBIN32U_MBIN32U:
      case PCIT::MOVE_MBIN64S_MBIN64S:
      {
        operand1 = inst->getROps()[0];
        operand2 = inst->getWOps()[0];

        bvIndex1 = operand1->getBvIndex();
        bvIndex2 = operand2->getBvIndex();

        constant = PCodeConstants::getConstantValue(bvIndex2, zeroes,
                                                    ones, neg1);

        // Delete moves whose constants are already known.
        if ((constant != PCodeConstants::UNKNOWN_CONSTANT) &&
            (constant ==
             PCodeConstants::getConstantValue(bvIndex1, zeroes, ones, neg1)))
        {
          block->deleteInst(inst);
          break;
        }

        // Clear out vector first since it's not done initially before switch
        PCodeConstants::clearConstantVectors(bvIndex2, zeroes, ones, neg1);

        constant = PCodeConstants::copyConstantVectors(bvIndex1, bvIndex2,
                                                       zeroes, ones, neg1);

        // Let peephole take care of the rest of this.  This will allow copy
        // propagation to proceed, which may be better.
        if (operand1->isConst())
          break;

        if (constant != PCodeConstants::UNKNOWN_CONSTANT) {
          // If constant is zero, use the constant operand in order to try and
          // promote copy propagation later on
          if (constant == 0) {
            inst->code[3] = 1;
            inst->code[4] = zeroOffset_;
            inst->reloadOperands(this);
          }
          else {
            switch (opc) {
              case PCIT::MOVE_MBIN16U_MBIN16U:
                inst->code[0] = PCIT::MOVE_MBIN16U_IBIN16U;
                break;

              case PCIT::MOVE_MBIN32U_MBIN32U:
                inst->code[0] = PCIT::MOVE_MBIN32S_IBIN32S;
                break;

              case PCIT::MOVE_MBIN8_MBIN8:
              case PCIT::MOVE_MBIN64S_MBIN64S:
                // TODO: No IBIN equivalent yet
                break;
            }

            // If the opcode changed, update operands
            if (opc != inst->code[0]) {
              inst->code[3] = constant;
              inst->getROps().clear();
            }
          }
        }
           
        break;
      }


      case PCIT::OR_MBIN32S_MBIN32S_MBIN32S:

#if 0
        // First take advantage of the fact that all source operands into
        // logical boolean operators can *AND SHOULD* only have values 0, 1,
        // or -1. Therefore set this up first.  Note, we don't do this if we
        // know more about the operand's values than this fact alone.

        for (i=0; i < 2; i++) {
          operand1 = inst->getROps()[i];
          bvIndex1 = operand1->getBvIndex();

          // If this value truly is unknown, set it to what we do know.
          if (!PCodeConstants::isAnyKnownConstants(bvIndex1,zeroes,ones,neg1)) {
            zeroes += bvIndex1;
            ones += bvIndex1;
            neg1 += bvIndex1;
          }
        }
#endif

        for (i=0; i < 2; i++)
        {
          operand1 = inst->getROps()[i];
          operand2 = inst->getROps()[(i+1)%2];

          bvIndex1 = operand1->getBvIndex();
          bvIndex2 = operand2->getBvIndex();

          constant = PCodeConstants::getConstantValue(bvIndex1, zeroes,
                                                      ones, neg1);

          if (constant == 1) {
            inst->code[0] = PCIT::MOVE_MBIN32S_IBIN32S;
            inst->code[3] = constant;
            inst->getROps().clear();

            RESTART_INST_IN_BLOCK;
            break;
          }

          if (constant == 0)
          {
            inst->code[0] = PCIT::MOVE_MBIN32U_MBIN32U;
            inst->code[3] = operand2->getStackIndex();
            inst->code[4] = operand2->getOffset();

            inst->reloadOperands(this);

            RESTART_INST_IN_BLOCK;
            break;
          }

          if (constant == -1) {
            if (PCodeConstants::isAnyKnownConstant(bvIndex2, zeroes,
                                                   ones, neg1) &&
                !PCodeConstants::canBeOne(bvIndex2, ones))
            {
              inst->code[0] = PCIT::MOVE_MBIN32S_IBIN32S;
              inst->code[3] = -1;
              inst->getROps().clear();

              RESTART_INST_IN_BLOCK;
              break;
            }
          }
        }

        // If no change was made above, see if we can fine tune what the known
        // values of the target operand should be.
        if (inst->code[0] == PCIT::OR_MBIN32S_MBIN32S_MBIN32S) {
          bvIndex2 = inst->getWOps()[0]->getBvIndex();
          for (i=0; i < 2; i++) {
            bvIndex1 = inst->getROps()[i]->getBvIndex();

            // If src is 1 or -1, result is also 1 or -1
            if (PCodeConstants::isAnyKnownConstant(bvIndex1,zeroes,ones,neg1)&&
                !PCodeConstants::canBeZero(bvIndex1, zeroes))
            {
              ones += bvIndex2;
              neg1 += bvIndex2;
              break;
            }
          }
        }

        break;


      case PCIT::AND_MBIN32S_MBIN32S_MBIN32S:

#if 0
        // First take advantage of the fact that all source operands into
        // logical boolean operators can *AND SHOULD* only have values 0, 1,
        // or -1. Therefore set this up first.  Note, we don't do this if we
        // know more about the operand's values than this fact alone.

        for (i=0; i < 2; i++) {
          operand1 = inst->getROps()[i];
          bvIndex1 = operand1->getBvIndex();

          // If this value truly is unknown, set it to what we do know.
          if (!PCodeConstants::isAnyKnownConstants(bvIndex1,zeroes,ones,neg1)) {
            zeroes += bvIndex1;
            ones += bvIndex1;
            neg1 += bvIndex1;
          }
        }
#endif

        for (i=0; i < 2; i++)
        {
          operand1 = inst->getROps()[i];
          operand2 = inst->getROps()[(i+1)%2];

          bvIndex1 = operand1->getBvIndex();
          bvIndex2 = operand2->getBvIndex();

          isZero = zeroes.testBit(bvIndex1);
          isOne = ones.testBit(bvIndex1);
          isNeg1 = neg1.testBit(bvIndex1);

          if (isZero && !isOne && !isNeg1) {
            inst->code[0] = PCIT::MOVE_MBIN32S_IBIN32S;
            inst->code[3] = 0;

            inst->reloadOperands(this);

            RESTART_INST_IN_BLOCK;
            break;
          }

          // If this operand has a value of one, and the second operand is
          // known to have a value of 0, 1, or -1, result is the second 
          // operand.  Similarly, if this operand is -1 and the second
          // operand has value 0 or -1, then follow suit.

          if (((isOne && !isZero && !isNeg1) &&
              (zeroes.testBit(bvIndex2) || ones.testBit(bvIndex2) ||
               neg1.testBit(bvIndex2)))
              ||
              ((isNeg1 && !isZero && !isOne) &&
               (zeroes.testBit(bvIndex2) && neg1.testBit(bvIndex2) &&
                !ones.testBit(bvIndex2))))
          {
            inst->code[0] = PCIT::MOVE_MBIN32U_MBIN32U;
            inst->code[3] = operand2->getStackIndex();
            inst->code[4] = operand2->getOffset();

            inst->reloadOperands(this);

            RESTART_INST_IN_BLOCK;
            break;
          }

          // If this operand is -1, and the second operand is either -1
          // or one, the result is -1

          if ((isNeg1 && !isZero && !isOne) &&
               ((neg1.testBit(bvIndex2) || (ones.testBit(bvIndex2))) &&
                !zeroes.testBit(bvIndex2)))
          {
            inst->code[0] = PCIT::MOVE_MBIN32S_IBIN32S;
            inst->code[3] = -1;

            inst->reloadOperands(this);

            RESTART_INST_IN_BLOCK;
            break;
          }
        }

        // If no change was made above, see if we can fine tune what the known
        // values of the target operand should be.
        if (inst->code[0] == PCIT::AND_MBIN32S_MBIN32S_MBIN32S) {
          bvIndex2 = inst->getWOps()[0]->getBvIndex();
          for (i=0; i < 2; i++) {
            bvIndex1 = inst->getROps()[i]->getBvIndex();

            // If src is 0 or -1, result is also 0 or -1
            if (PCodeConstants::isAnyKnownConstant(bvIndex1,zeroes,ones,neg1)&&
                !PCodeConstants::canBeOne(bvIndex1, ones))
            {
              zeroes += bvIndex2;
              neg1 += bvIndex2;
              break;
            }
          }
        }

        break;

          case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN32S_MBIN32S:
          case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN64S:
          case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN32S:
          case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MFLT64_MFLT64:
          case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIGS_MBIGS_IBIN32S:
          {
            // Look for the null indicator used in this PCODE instruction and
            // see if we know it to be NULL.  If so, then this instruction
            // doesn't get invoked, and therefore we can remove it.
            for (i=0; i < inst->getROps().entries(); i++) {
              if (((inst->getROps()[i]->getStackIndexPos() == 2) &&
                   (inst->getROps()[i]->getOffsetPos() == 3)) ||
                  ((inst->getROps()[i]->getStackIndexPos() == 3) &&
                   (inst->getROps()[i]->getOffsetPos() == 4)))
              {
                bvIndex1 = inst->getROps()[i]->getBvIndex();
                constant = PCodeConstants::getConstantValue(bvIndex1,
                                                            zeroes,ones,neg1);

                if (constant == -1)
                  block->deleteInst(inst);

                break;
              }
            }
            break;
          }

          case PCIT::CLAUSE_EVAL:
          {
            ex_clause* clause = (ex_clause*)*(Long*)&(inst->code[1]);
            if ((clause->getType() == ex_clause::FUNCTION_TYPE) &&
                (clause->getClassID() == ex_clause::FUNC_RAISE_ERROR_ID) &&
                (((ExpRaiseErrorFunction*)clause)->raiseError()))
            {
              // Create a RETURN instruction after the clause, and then fix
              // up the block so as to remove trailing instrs in the block,
              // *IF* a RETURN isn't already there.

              if (!inst->next || !(inst->next->getOpcode() == PCIT::RETURN)) {
                inst = block->insertNewInstAfter(inst, PCIT::RETURN);
                block->setLastInst(inst);
                inst->next = NULL;
                while (block->getSuccs().entries())
                  block->removeEdge(block->getSuccs()[0]);


                RESTART_INST_IN_BLOCK;
              }
            }

            if (clause->getClassID() == ex_clause::AGGR_MIN_MAX_ID) {
              // Get the previous opdata.  It represents the condition for
              // which the min/max will be performed or not.  If not, then the
              // entire CLAUSE_EVAL sequence can be deleted.
              PCodeInst* opComp = inst->prev;
              bvIndex2 = opComp->getROps()[0]->getBvIndex();

              constant = PCodeConstants::getConstantValue(bvIndex2, zeroes,
                                                          ones, neg1);
              if (constant == 0) {
                // Delete clause-eval instruction
                block->deleteInst(inst);
              }
            }
            break;
          }

          case PCIT::OPDATA_MPTR32_IBIN32S_IBIN32S:
          case PCIT::OPDATA_MPTR32_IBIN32S:
          case PCIT::OPDATA_MBIN16U_IBIN32S:
          case PCIT::OPDATA_MATTR5_IBIN32S:
            if (inst->getWOps().entries()) {
              PCodeInst* tInst = NULL;
              ex_clause* clause = NULL;

              // Use for identifying read operands
              operand1 = NULL;
              operand2 = NULL;

              // Find the clause eval associated with this opdata and set
              // read operands along the way.
              for (tInst = inst->next;
                   tInst->getOpcode() != PCIT::CLAUSE_EVAL;
                   tInst = tInst->next)
              {
                if (tInst->getROps().entries()) {
                  if (operand1 == NULL)
                    operand1 = tInst->getROps()[0];
                  else
                    operand2 = tInst->getROps()[0];
                }
              }

              assert (tInst->block == block);

              // We can determine the constants of some clauses
              clause = (ex_clause*)*(Long*)&(tInst->code[1]);
              switch (clause->getType())
              {
                case ex_clause::UN_LOGIC_TYPE:
                  // Get read and write operands
                  bvIndex1 = operand1->getBvIndex();
                  bvIndex2 = inst->getWOps()[0]->getBvIndex();

                  constant = PCodeConstants::getConstantValue(bvIndex1, zeroes,
                                                              ones, neg1);

                  if (constant != PCodeConstants::UNKNOWN_CONSTANT) {

                    constant2 = PCodeConstants::UNKNOWN_CONSTANT;

                    switch (clause->getOperType()) {
                      case ITM_IS_UNKNOWN:
                        constant2 = (constant == -1) ? 1 : 0;
                        break;

                      case ITM_IS_NOT_UNKNOWN:
                        constant2 = (constant != -1) ? 1 : 0;
                        break;

                      case ITM_IS_TRUE:
                        constant2 = (constant == 1) ? 1 : 0;
                        break;

                      case ITM_IS_FALSE:
                        constant2 = (constant == 0) ? 1 : 0;
                        break;

                      case ITM_NOT:
                        if (constant == 1)
                          constant2 = 0;
                        else if (constant == 0)
                          constant2 = 1;
                        else
                          constant2 = -1;
                        break;

                      default:
                        break;
                    }

                    if (constant2 != PCodeConstants::UNKNOWN_CONSTANT) {
                      PCodeInst* move =
                        block->insertNewInstAfter(tInst,
                                                  PCIT::MOVE_MBIN32S_IBIN32S);
                      move->code[1] = inst->getWOps()[0]->getStackIndex();
                      move->code[2] = inst->getWOps()[0]->getOffset();
                      move->code[3] = constant2;

                      move->reloadOperands(this);

                      // Delete clause-eval instruction
                      block->deleteInst(tInst);

                      //PCodeConstants::setConstantInVectors(constant2,bvIndex2,
                                                         //zeroes, ones, neg1);

                      // In order to restart inst, inst needs to be assigned
                      // new move instruction, since inst was deleted
                      inst = move;
                      RESTART_INST_IN_BLOCK;

                      break;
                    }
                  }
                  else if (clause->getOperType() == ITM_NOT) {
                    if (PCodeConstants::canBeZero(bvIndex1, zeroes) &&
                        PCodeConstants::canBeOne(bvIndex1, ones) &&
                        !PCodeConstants::canBeNeg1(bvIndex1, neg1))
                    {
                      // Clause can be replaced with simple compare inst
                      PCodeInst* cmp =
                        block->insertNewInstAfter(tInst,
                                   PCIT::EQ_MBIN32S_MBIN32S_MBIN32S);

                      cmp->code[1] = inst->getWOps()[0]->getStackIndex();
                      cmp->code[2] = inst->getWOps()[0]->getOffset();
                      cmp->code[3] = operand1->getStackIndex();
                      cmp->code[4] = operand1->getOffset();
                      cmp->code[5] = 1;
                      cmp->code[6] = zeroOffset_;

                      cmp->reloadOperands(this);

                      // Delete clause-eval instruction
                      block->deleteInst(tInst);

                      inst = cmp;
                      RESTART_INST_IN_BLOCK;

                      break;
                    }
                    else
                      // Result can be -1
                      neg1 += bvIndex2;
                  }

                  // All UN_LOGIC clauses will return 0 or 1
                  zeroes += bvIndex2;
                  ones += bvIndex2;

                  break;

                case ex_clause::LIKE_TYPE:
                case ex_clause::LIKE_CLAUSE_CHAR_ID:
                case ex_clause::COMP_TYPE:
                  bvIndex2 = inst->getWOps()[0]->getBvIndex();
                  zeroes += bvIndex2;
                  ones += bvIndex2;

                  // If this clause processes nulls, we have to assume -1 can
                  // be returned - uggh.
                  if (tInst->code[1 + PCODEBINARIES_PER_PTR] & 1)
                    neg1 += bvIndex2;
                  break;

                case ex_clause::BOOL_TYPE:
                  assert(FALSE);
                  break;

                case ex_clause::FUNCTION_TYPE:
                  switch (clause->getClassID()) {
                    case ex_clause::FUNC_GET_BIT_VALUE_AT_ID:
                      bvIndex2 = inst->getWOps()[0]->getBvIndex();
                      zeroes += bvIndex2;
                      ones += bvIndex2;
                      break;
                  }
                  break;
              }
            }

            break;

     
          case PCIT::NULL_TEST_MBIN32S_MATTR5_IBIN32S_IBIN32S:
            // TODO: Indirect varchars are not supported right now
            if (inst->code[8] == 1) {
              bvIndex1 = inst->getWOps()[0]->getBvIndex();
              zeroes += bvIndex1;
              ones += bvIndex1;
              break;
            }
            // Otherwise non-indirect varchar, so fall-through

          case PCIT::NULL_TEST_MBIN32S_MBIN16U_IBIN32S:
          {
            bvIndex2= inst->getROps()[0]->getBvIndex();
            constant = PCodeConstants::getConstantValue(bvIndex2, zeroes,
                                                        ones, neg1);

            if (constant != PCodeConstants::UNKNOWN_CONSTANT) {
              Int32 val = 0;

              switch (opc) {
                case PCIT::NULL_TEST_MBIN32S_MBIN16U_IBIN32S:
                  val = inst->code[5];
                  break;

                case PCIT::NULL_TEST_MBIN32S_MATTR5_IBIN32S_IBIN32S:
                  val = inst->code[9];
                  break;
              }

              assert((val == 0) || (val == -1));

              inst->code[0] = PCIT::MOVE_MBIN32S_IBIN32S;
              inst->code[3] = (constant == val) ? 1 : 0;

              inst->getROps().clear();

              RESTART_INST_IN_BLOCK;
              break;
            }

            bvIndex1 = inst->getWOps()[0]->getBvIndex();
            zeroes += bvIndex1;
            ones += bvIndex1;

            break;
          }



          case PCIT::ZERO_MBIN32S_MBIN32U:
          case PCIT::NOTZERO_MBIN32S_MBIN32U:

          case PCIT::EQ_MBIN32S_MBIN16S_MBIN16S:
          case PCIT::NE_MBIN32S_MBIN16S_MBIN16S:
          case PCIT::LT_MBIN32S_MBIN16S_MBIN16S:
          case PCIT::GT_MBIN32S_MBIN16S_MBIN16S:
          case PCIT::LE_MBIN32S_MBIN16S_MBIN16S:
          case PCIT::GE_MBIN32S_MBIN16S_MBIN16S:

          case PCIT::EQ_MBIN32S_MBIN16S_MBIN32S:
          case PCIT::LT_MBIN32S_MBIN16S_MBIN32S:
          case PCIT::GT_MBIN32S_MBIN16S_MBIN32S:
          case PCIT::LE_MBIN32S_MBIN16S_MBIN32S:
          case PCIT::GE_MBIN32S_MBIN16S_MBIN32S:

          case PCIT::EQ_MBIN32S_MBIN32S_MBIN32S:
          case PCIT::LT_MBIN32S_MBIN32S_MBIN32S:
          case PCIT::GT_MBIN32S_MBIN32S_MBIN32S:
          case PCIT::LE_MBIN32S_MBIN32S_MBIN32S:
          case PCIT::GE_MBIN32S_MBIN32S_MBIN32S:

          case PCIT::EQ_MBIN32S_MBIN16U_MBIN16U:
          case PCIT::LT_MBIN32S_MBIN16U_MBIN16U:
          case PCIT::GT_MBIN32S_MBIN16U_MBIN16U:
          case PCIT::LE_MBIN32S_MBIN16U_MBIN16U:
          case PCIT::GE_MBIN32S_MBIN16U_MBIN16U:

          case PCIT::EQ_MBIN32S_MBIN16U_MBIN32U:
          case PCIT::LT_MBIN32S_MBIN16U_MBIN32U:
          case PCIT::GT_MBIN32S_MBIN16U_MBIN32U:
          case PCIT::LE_MBIN32S_MBIN16U_MBIN32U:
          case PCIT::GE_MBIN32S_MBIN16U_MBIN32U:

          case PCIT::EQ_MBIN32S_MBIN32U_MBIN32U:
          case PCIT::LT_MBIN32S_MBIN32U_MBIN32U:
          case PCIT::GT_MBIN32S_MBIN32U_MBIN32U:
          case PCIT::LE_MBIN32S_MBIN32U_MBIN32U:
          case PCIT::GE_MBIN32S_MBIN32U_MBIN32U:

          case PCIT::EQ_MBIN32S_MBIN16S_MBIN32U:
          case PCIT::LT_MBIN32S_MBIN16S_MBIN32U:
          case PCIT::GT_MBIN32S_MBIN16S_MBIN32U:
          case PCIT::LE_MBIN32S_MBIN16S_MBIN32U:
          case PCIT::GE_MBIN32S_MBIN16S_MBIN32U:

          case PCIT::EQ_MBIN32S_MBIN16U_MBIN16S:
          case PCIT::LT_MBIN32S_MBIN16U_MBIN16S:
          case PCIT::GT_MBIN32S_MBIN16U_MBIN16S:
          case PCIT::LE_MBIN32S_MBIN16U_MBIN16S:
          case PCIT::GE_MBIN32S_MBIN16U_MBIN16S:

          case PCIT::NE_MBIN32S_MBIN64S_MBIN64S:
          case PCIT::EQ_MBIN32S_MBIN64S_MBIN64S:
          case PCIT::LT_MBIN32S_MBIN64S_MBIN64S:
          case PCIT::GT_MBIN32S_MBIN64S_MBIN64S:
          case PCIT::LE_MBIN32S_MBIN64S_MBIN64S:
          case PCIT::GE_MBIN32S_MBIN64S_MBIN64S:

          case PCIT::NE_MBIN32S_MASCII_MASCII:
          case PCIT::EQ_MBIN32S_MASCII_MASCII:
          case PCIT::LT_MBIN32S_MASCII_MASCII:
          case PCIT::GT_MBIN32S_MASCII_MASCII:
          case PCIT::LE_MBIN32S_MASCII_MASCII:
          case PCIT::GE_MBIN32S_MASCII_MASCII:

          case PCIT::NE_MBIN32S_MFLT64_MFLT64:
          case PCIT::EQ_MBIN32S_MFLT64_MFLT64:
          case PCIT::LT_MBIN32S_MFLT64_MFLT64:
          case PCIT::GT_MBIN32S_MFLT64_MFLT64:
          case PCIT::LE_MBIN32S_MFLT64_MFLT64:
          case PCIT::GE_MBIN32S_MFLT64_MFLT64:

          case PCIT::NE_MBIN32S_MFLT32_MFLT32:
          case PCIT::EQ_MBIN32S_MFLT32_MFLT32:
          case PCIT::LT_MBIN32S_MFLT32_MFLT32:
          case PCIT::GT_MBIN32S_MFLT32_MFLT32: 
          case PCIT::LE_MBIN32S_MFLT32_MFLT32:
          case PCIT::GE_MBIN32S_MFLT32_MFLT32:

          case PCIT::COMP_MBIN32S_MUNIV_MUNIV_IBIN32S:
          case PCIT::COMP_MBIN32S_MATTR5_MATTR5_IBIN32S:
          case PCIT::COMP_MBIN32S_MUNI_MUNI_IBIN32S_IBIN32S_IBIN32S:
          case PCIT::COMP_MBIN32S_MBIGS_MBIGS_IBIN32S_IBIN32S:
          case PCIT::COMP_MBIN32S_MASCII_MASCII_IBIN32S_IBIN32S_IBIN32S:

          case PCIT::LIKE_MBIN32S_MATTR5_MATTR5_IBIN32S_IBIN32S:
          {
            bvIndex2 = inst->getWOps()[0]->getBvIndex();
            zeroes += bvIndex2;
            ones += bvIndex2;

            break;
          }

          case PCIT::SWITCH_MBIN32S_MBIN64S_MPTR32_IBIN32S_IBIN32S:
          case PCIT::SWITCH_MBIN32S_MATTR5_MPTR32_IBIN32S_IBIN32S:
          {
            // If for IN-lists, result can only be 0 or 1.
            if (inst->isInListSwitch()) {
              bvIndex2 = inst->getWOps()[0]->getBvIndex();
              zeroes += bvIndex2;
              ones += bvIndex2;
            }
            break;
          }

      case PCIT::NNB_MBIN32S_MATTR3_IBIN32S_IBIN32S:
      {
        PCodeInst* mv;
        PCodeBlock* fallThrough = block->getSuccs()[0];

        // Add move to fall-through
        mv = fallThrough->insertNewInstBefore(NULL, PCIT::MOVE_MBIN32S_IBIN32S);
        mv->code[1] = inst->getWOps()[0]->getStackIndex();
        mv->code[2] = inst->getWOps()[0]->getOffset();
        mv->code[3] = inst->code[6];
        mv->reloadOperands(this);

        // Rewrite branch to not have write operand
        inst->code[0] = PCIT::NNB_MATTR3_IBIN32S;
        inst->code[1] = inst->getROps()[0]->getStackIndex();
        inst->code[2] = inst->getROps()[0]->getOffset();
        inst->code[3] = inst->getROps()[0]->getNullBitIndex();
        inst->reloadOperands(this);

        RESTART_INST_IN_BLOCK;
        break;
      }

      case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S_IBIN32S:
      {
        PCodeInst* mv;
        PCodeBlock* fallThrough = block->getSuccs()[0];

        // Add move to fall-through
        mv = fallThrough->insertNewInstBefore(NULL, PCIT::MOVE_MBIN32S_IBIN32S);
        mv->code[1] = inst->getWOps()[0]->getStackIndex();
        mv->code[2] = inst->getWOps()[0]->getOffset();
        mv->code[3] = inst->code[5];
        mv->reloadOperands(this);

        // Rewrite branch to not have write operand
        inst->code[0] = PCIT::NOT_NULL_BRANCH_MBIN16S_IBIN32S;
        inst->code[1] = inst->getROps()[0]->getStackIndex();
        inst->code[2] = inst->getROps()[0]->getOffset();
        inst->reloadOperands(this);

        RESTART_INST_IN_BLOCK;
        break;
      }

      case PCIT::NNB_MATTR3_IBIN32S:
      case PCIT::NOT_NULL_BRANCH_MBIN16S_IBIN32S:
      {
        bvIndex1 = inst->getROps()[0]->getBvIndex();
        constant = PCodeConstants::getConstantValue(bvIndex1, zeroes,
                                                    ones, neg1);

        if (constant != PCodeConstants::UNKNOWN_CONSTANT) {
          if (constant == 0) {
            inst->code[0] = PCIT::BRANCH;
            inst->getROps().clear();
            block->removeEdge(block->getFallThroughBlock());
            break;
          }
          else {
            block->removeEdge(block->getTargetBlock());
            block->deleteInst(inst);
          }

          graphChanged = TRUE; // CFG was modifed
        }

        // Otherwise we can at least propagate what we do know
        zeroes += bvIndex1;
        neg1 += bvIndex1;
        break;
      }

      case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_IATTR3_IBIN32S:
      {
        operand1 = inst->getWOps()[0];
        bvIndex1 = operand1->getBvIndex();
        bvIndex2 = inst->getROps()[0]->getBvIndex();
        constant = PCodeConstants::getConstantValue(bvIndex2, zeroes,
                                                    ones, neg1);

        // Constants must be known, *and* target can't be indirect varchar
        if (((constant == -1) || (constant == 0)) &&
            (operand1->getOffset() >= 0))

        {
          if (!operand1->forAlignedFormat()) {
            inst->code[0] = PCIT::MOVE_MBIN16U_IBIN16U;
            inst->code[3] = (constant == 0) ? 0 : -1;
          }
          else {
            inst->code[0] = PCIT::NULL_BITMAP;
            inst->code[3] = PCIT::NULL_BITMAP_SET;
            inst->code[4] = operand1->getNullBitIndex();
            inst->code[5] = (constant == 0) ? 0 : 1;
          }

          if (constant == 0) {
            block->insertNewInstAfter(inst, PCIT::BRANCH);
            block->removeEdge(block->getFallThroughBlock());
          }
          else {
            block->removeEdge(block->getTargetBlock());
          }

          graphChanged = TRUE; // CFG was modifed

          inst->reloadOperands(this);

          RESTART_INST_IN_BLOCK;
          break;
        }

        zeroes += bvIndex1;
        neg1 += bvIndex1;
        break;
      }

      case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
      {
        operand1 = inst->getWOps()[0];

        constant = PCodeConstants::getConstantValue(
              inst->getROps()[0]->getBvIndex(), zeroes, ones, neg1);

        constant2 = PCodeConstants::getConstantValue(
              inst->getROps()[1]->getBvIndex(), zeroes, ones, neg1);

        NABoolean isNull = ((constant == -1) || (constant2 == -1));
        NABoolean isNotNull = ((constant == 0) && (constant2 == 0));

        //TODO: add case where if either constant or constant2 is 0, then
        //reduce NNBB branch to shorter form.

        // Constants must be known, *and* target can't be indirect varchar
        if ((isNull || isNotNull) && (operand1->getOffset() >= 0))
        {
          if (!operand1->forAlignedFormat()) {
            inst->code[0] = PCIT::MOVE_MBIN16U_IBIN16U;
            inst->code[3] = (isNull) ? -1 : 0;
          }
          else {
            inst->code[0] = PCIT::NULL_BITMAP;
            inst->code[3] = PCIT::NULL_BITMAP_SET;
            inst->code[4] = operand1->getNullBitIndex();
            inst->code[5] = (isNull) ? 1 : 0;
          }

          if (isNotNull) {
            block->insertNewInstAfter(inst, PCIT::BRANCH);
            block->removeEdge(block->getFallThroughBlock());
          }
          else {
            block->removeEdge(block->getTargetBlock());
          }

          graphChanged = TRUE; // CFG was modifed

          inst->reloadOperands(this);

          RESTART_INST_IN_BLOCK;
          break;
        }
        else if ((constant == 0) || (constant2 == 0))
        {
        }

        bvIndex1 = operand1->getBvIndex();
        zeroes += bvIndex1;
        neg1 += bvIndex1;
        break;
      }

      case PCIT::NNB_SPECIAL_NULLS_MBIN32S_MATTR3_MATTR3_IBIN32S_IBIN32S:
      {
        // Go through each read operand.
        for (i=0; i < 2; i++) {
          operand2 = inst->getROps()[i];
          bvIndex2 = inst->getROps()[i]->getBvIndex();
          constant = PCodeConstants::getConstantValue(bvIndex2, zeroes,
                                                      ones, neg1);

          if (constant != PCodeConstants::UNKNOWN_CONSTANT) {
            if (constant == 0) {
              // Get constant of second operand
              constant = PCodeConstants::getConstantValue(
                           inst->getROps()[(i+1)%2]->getBvIndex(), 
                           zeroes, ones, neg1);

              if (constant != 0)
                continue;

              // Both operands are zero
              inst->code[0] = PCIT::MOVE_MBIN32S_IBIN32S;
              inst->code[3] = 0;

              inst->reloadOperands(this);

              block->insertNewInstAfter(inst, PCIT::BRANCH);
              block->removeEdge(block->getFallThroughBlock());
            }
            else {
              assert (constant == -1);

              PCodeInst* newInst;
              PCodeOperand* op = (i == 0) ? inst->getROps()[1]
                                          : inst->getROps()[0];
 
              newInst = block->insertNewInstBefore(inst,
                          PCIT::NULL_TEST_MBIN32S_MATTR5_IBIN32S_IBIN32S);
              newInst->code[1] = inst->code[1];
              newInst->code[2] = inst->code[2];
              newInst->code[3] = op->getStackIndex();
              newInst->code[4] = op->getOffset();
              newInst->code[5] = -1; // voa that's unnecessary.
              newInst->code[6] = (op->forAlignedFormat())
                                   ? ExpTupleDesc::SQLMX_ALIGNED_FORMAT
                                   : ExpTupleDesc::SQLARK_EXPLODED_FORMAT;
              newInst->code[7] = op->getNullBitIndex();
              newInst->code[8] = 0;  // No voa lookup needed.
              newInst->code[9] = -1; // Test for NULL

              newInst->reloadOperands(this);

              block->deleteInst(inst);
              inst = newInst;

//              inst->code[0] = PCIT::EQ_MBIN32S_MBIN16S_MBIN16S;

              block->removeEdge(block->getTargetBlock());
            }

            graphChanged = TRUE; // CFG was modifed

            RESTART_INST_IN_BLOCK;
            break;
          }
        }

        bvIndex1 = inst->getWOps()[0]->getBvIndex();
        zeroes += bvIndex1;
        ones += bvIndex1;
        break;
      }


      case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S:
      case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S:
      case PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_IBIN32S:
      {
        operand1 = inst->getROps()[0];
        operand2 = inst->getWOps()[0];

        bvIndex1 = operand1->getBvIndex();
        bvIndex2 = operand2->getBvIndex();

        constant = PCodeConstants::getConstantValue(bvIndex1, zeroes,
                                                    ones, neg1);

        if (constant != PCodeConstants::UNKNOWN_CONSTANT) {
          switch(opc) {
            case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S:
            case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S:
              inst->code[0] = PCIT::MOVE_MBIN32S_IBIN32S;
              break;

            case PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_IBIN32S:
              inst->code[0] = PCIT::MOVE_MBIN16U_IBIN16U;
              break;
          }

          inst->code[1] = operand2->getStackIndex();
          inst->code[2] = operand2->getOffset();
          inst->code[3] = constant;

          inst->reloadOperands(this);

          if (constant == 0) {
            block->insertNewInstAfter(inst, PCIT::BRANCH);
            block->removeEdge(block->getFallThroughBlock());
          }
          else {
            block->removeEdge(block->getTargetBlock());
          }

          graphChanged = TRUE; // CFG was modifed

          RESTART_INST_IN_BLOCK;
          continue;
        }

        zeroes += bvIndex1;
        neg1 += bvIndex1;

        zeroes += bvIndex2;
        neg1 += bvIndex2;
        break;
      }

      case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
        operand1 = inst->getWOps()[0];
        bvIndex1 = operand1->getBvIndex();

        for (i=0; i < 2; i++) {
          operand2 = inst->getROps()[i];
          bvIndex2 = operand2->getBvIndex();
          constant = PCodeConstants::getConstantValue(bvIndex2, zeroes,
                                                      ones, neg1);

          if (constant != PCodeConstants::UNKNOWN_CONSTANT) {
              inst->code[0] =
                PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S;

              if (constant == -1) {
                inst->code[3] = operand2->getStackIndex();
                inst->code[4] = operand2->getOffset();
                inst->code[5] = inst->code[7];

                PCodeAttrNull * newAttr = (PCodeAttrNull *)&(inst->code[5]);
                PCodeAttrNull * oldAttr = (PCodeAttrNull *)&(inst->code[7]);

                // New size is 3 less since we're getting rid of 1 source op,
                // which is made up of index and offset, and 1 null bit index,
                // which corresponds to that source.
                newAttr->fmt_.size_ = oldAttr->fmt_.size_ - 3;
 
                // If src is second operand, need to set format correctly.
                // Otherwise it, along with target, are already set right.
                if (i == 1)
                  newAttr->fmt_.op2Fmt_ = oldAttr->fmt_.op3Fmt_;

                inst->code[6] = operand1->getNullBitIndex();
                inst->code[7] = operand2->getNullBitIndex();
                
              }
              else {
                inst->code[3] = inst->getROps()[(i+1)%2]->getStackIndex();
                inst->code[4] = inst->getROps()[(i+1)%2]->getOffset();
                inst->code[5] = inst->code[7];

                PCodeAttrNull * newAttr = (PCodeAttrNull *)&(inst->code[5]);
                PCodeAttrNull * oldAttr = (PCodeAttrNull *)&(inst->code[7]);

                // New size is 2 less since we're getting rid of 1 source op
                // which is made up of index and offset.
                newAttr->fmt_.size_ = oldAttr->fmt_.size_ - 3;
 
                // If src is first operand, need to get format of 2nd op
                // Otherwise it, along with target, are already set right.
                if (i == 0)
                  newAttr->fmt_.op2Fmt_ = oldAttr->fmt_.op3Fmt_;

                inst->code[6] = operand1->getNullBitIndex();
                inst->code[7] = inst->getROps()[(i+1)%2]->getNullBitIndex();
              }

            inst->reloadOperands(this);

            RESTART_INST_IN_BLOCK;
            break;
          }
        }

        zeroes += bvIndex1;
        neg1 += bvIndex1;

        break;


          case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_MBIN16S_IBIN32S:
          case PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_MBIN16S_IBIN32S:
            operand1 = inst->getWOps()[0];
            bvIndex1 = operand1->getBvIndex();

            for (i=0; i < 2; i++) {
              operand2 = inst->getROps()[i];
              bvIndex2 = operand2->getBvIndex();
              constant = PCodeConstants::getConstantValue(bvIndex2, zeroes,
                                                          ones, neg1);

              if ((constant == -1) || (constant == 0)) {
                if (opc==PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_MBIN16S_IBIN32S)
                  inst->code[0] = PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S;
                else
                  inst->code[0] = PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_IBIN32S;

                if (constant == 0) {
                  inst->code[3] = inst->getROps()[(i+1)%2]->getStackIndex();
                  inst->code[4] = inst->getROps()[(i+1)%2]->getOffset();
                }
                else {
                  inst->code[3] = operand2->getStackIndex();
                  inst->code[4] = operand2->getOffset();
                }
                inst->code[5] = inst->code[7];

                inst->reloadOperands(this);

                // No need to change anything else since retrying the new
                // instruction will end up removing this instruction.

                RESTART_INST_IN_BLOCK;
                break;
              }
            }

            zeroes += bvIndex1;
            neg1 += bvIndex1;

            break;

          case PCIT::NULL_BITMAP_BULK:
          case PCIT::NOT_NULL_BRANCH_BULK:
            for (i=0; i < inst->getROps().entries(); i++) {
              bvIndex2 = inst->getROps()[i]->getBvIndex();
              zeroes += bvIndex2;
              neg1 += bvIndex2;
            }
            break;

          case PCIT::BRANCH_OR:
          case PCIT::BRANCH_AND:
          {
            operand1 = inst->getROps()[0];
            operand2 = inst->getWOps()[0];

            bvIndex1 = operand1->getBvIndex();
            bvIndex2 = operand2->getBvIndex();

            constant = PCodeConstants::getConstantValue(bvIndex1, zeroes,
                                                        ones, neg1);

            if (constant != PCodeConstants::UNKNOWN_CONSTANT) {
              inst->code[0] = PCIT::MOVE_MBIN32S_IBIN32S;
              inst->code[1] = operand2->getStackIndex();
              inst->code[2] = operand2->getOffset();
              inst->code[3] = constant;

              inst->reloadOperands(this);

              if (((opc == PCIT::BRANCH_OR) && (constant == 1)) ||
                  ((opc == PCIT::BRANCH_AND) && (constant == 0)))
              {
                block->insertNewInstAfter(inst, PCIT::BRANCH);
                block->removeEdge(block->getFallThroughBlock());
              }
              else {
                block->removeEdge(block->getTargetBlock());
              }

              graphChanged = TRUE; // CFG was modifed

              RESTART_INST_IN_BLOCK;
              break;
            }
            else if (block->getFallThroughBlock() == block->getTargetBlock())
            {
              // Don't ask me why, but sometimes we see this: executor/TEST038
              // Change branch to move and fall-through
              inst->code[0] = PCIT::MOVE_MBIN32U_MBIN32U;
              inst->code[1] = operand2->getStackIndex();
              inst->code[2] = operand2->getOffset();
              inst->code[3] = operand1->getStackIndex();
              inst->code[4] = operand1->getOffset();

              inst->reloadOperands(this);
              block->removeEdge(block->getTargetBlock());

              graphChanged = TRUE; // CFG was modifed

              // TODO: Restart once we add constant prop support for MOVEs
              // other than constant moves.
              //RESTART_INST_IN_BLOCK;
              //break;
            }

            // The operand of a logical branch should have a boolean value.  In
            // our case, the values can be 0, 1, or -1.  If the read operand
            // is unknown, or if certain values are not possible (e.g. -1)
            // then set that up first.

            if (!PCodeConstants::isAnyKnownConstant(bvIndex1,zeroes,ones,neg1))
            {
              zeroes += bvIndex1;
              ones += bvIndex1;
              neg1 += bvIndex1;
            }

            // If a change was a made, restart without setting write constants
            if (inst->getOpcode() != opc) {
              RESTART_INST_IN_BLOCK;
              break;
            }

            // Set up write constants
            PCodeConstants::copyConstantVectors(bvIndex1, bvIndex2,
                                                zeroes, ones, neg1);

            //
            // Sometimes a logical branch targets a logic instruction
            // unnecessarily.  This is a branch optimization which could be
            // done in cfgRewiring(), but we opt to do it here instead.  An
            // example of this is:
            //
            // [1]
            // BRANCH_AND (168) 13 0 2 8 2 12  (Tgt: 4)
            // ...
            // [4]  (Preds: 2 )
            // AND_MBIN32S_MBIN32S_MBIN32S (63) 2 8 2 12 2 16
            //
            // [5]  (Preds: 1 )
            // OR_MBIN32S_MBIN32S_MBIN32S (64) 2 0 2 4 2 8
            //
            // We should just be able to branch from 1 directly to 5.
            //

            // Target block should have a single logical instruction.
            PCodeBlock* tgtBlock = block->getTargetBlock();
            if (tgtBlock->getLastInst() &&
                (tgtBlock->getLastInst() == tgtBlock->getFirstInst()))
            {
              PCodeInst* logicInst = tgtBlock->getLastInst();
              Int32 subOpc = logicInst->getOpcode();

              // ((BRANCH_OR -> OR) or (BRANCH_AND -> AND)) and both insts have
              // the same write operand.
              if ((((opc == PCIT::BRANCH_OR) &&
                   (subOpc == PCIT::OR_MBIN32S_MBIN32S_MBIN32S)) ||
                   ((opc == PCIT::BRANCH_AND) &&
                    (subOpc == PCIT::AND_MBIN32S_MBIN32S_MBIN32S))) &&
                  (logicInst->getWOps()[0]->getBvIndex() ==
                   inst->getWOps()[0]->getBvIndex()))
              {
                // The read operand of the branch must be a read operand of the
                // logic instruction.
                CollIndex branchOpIdx = inst->getROps()[0]->getBvIndex();
                if ((logicInst->getROps()[0]->getBvIndex() == branchOpIdx) ||
                    (logicInst->getROps()[1]->getBvIndex() == branchOpIdx))
                {
                  // Re-target branch to fall-through of original target block.
                  block->removeEdge(block->getTargetBlock());
                  block->addEdge(tgtBlock->getFallThroughBlock());

                  graphChanged = TRUE; // CFG was modifed
                }
              }
            }

            break;
          }
    }
  } ENDFE_INST_IN_BLOCK

  return graphChanged;
}

// 
// Change moves from constant's area to direct immediate moves.
//
void PCodeCfg::constantFolding()
{
  FOREACH_BLOCK(block, firstInst, lastInst, index) {
    FOREACH_INST_IN_BLOCK(block, inst) {
      if (inst->isMove()) {
        PCodeOperand* operand2 = inst->getROps()[0];
        if (!operand2->isConst())
          continue;

        switch(inst->getOpcode()) {
          case PCIT::MOVE_MBIN16U_MBIN8:
            inst->code[0] = PCIT::MOVE_MBIN16U_IBIN16U;
            inst->code[3] = (UInt8)getIntConstValue(operand2);
            break;

          case PCIT::MOVE_MBIN32U_MBIN16U:
          case PCIT::MOVE_MBIN32S_MBIN16U:
            inst->code[0] = PCIT::MOVE_MBIN32S_IBIN32S;
            inst->code[3] = (UInt16)getIntConstValue(operand2);
            break;

          case PCIT::MOVE_MBIN32S_MBIN16S:
          case PCIT::MOVE_MBIN32U_MBIN16S:
            inst->code[0] = PCIT::MOVE_MBIN32S_IBIN32S;
            inst->code[3] = (Int16)getIntConstValue(operand2);
            break;

          case PCIT::MOVE_MBIN16U_MBIN16U:
            inst->code[0] = PCIT::MOVE_MBIN16U_IBIN16U;
            inst->code[3] = (UInt16)getIntConstValue(operand2);
            break;

          case PCIT::MOVE_MBIN32U_MBIN32U:
            inst->code[0] = PCIT::MOVE_MBIN32S_IBIN32S;
            inst->code[3] = (UInt32)getIntConstValue(operand2);
            break;

          case PCIT::MOVE_MBIN8_MBIN8:
            continue;

          case PCIT::MOVE_MBIN64S_MBIN16U:
          case PCIT::MOVE_MBIN64S_MBIN16S:
          case PCIT::MOVE_MBIN64S_MBIN32U:
          case PCIT::MOVE_MBIN64S_MBIN32S:
            continue;

          case PCIT::MOVE_MBIN64S_MBIN64S:
            continue;

          default:
            continue;
        }

        inst->reloadOperands(this);
      }
    } ENDFE_INST_IN_BLOCK
  } ENDFE_BLOCK
}

//
// Given an operand known to be a constant, lookup it's value as a string and
// return it (and its length via an incoming parameter).
//
char* PCodeCfg::getStringConstValue(PCodeOperand* op, Int32* len)
{
  CollIndex off = op->getOffset();

  if (op->isVarchar()) {
    off = off - op->getVcIndicatorLen() - op->getVcNullIndicatorLen();
  }

  PCodeConstants* constPtr = offsetToConstMap_->getFirstValue(&off);

  assert(constPtr != NULL);

  switch (op->getType()) {
    case PCIT::MASCII:
      *len = op->getLen();
      return (char*)(constPtr->getData());

    case PCIT::MATTR5:
      if (op->isVarchar()) {
        *len = ((op->getVcIndicatorLen() == 2)
          ? (Int32)(*((Int16*)((char*)constPtr->getData() +
              op->getVcNullIndicatorLen())))
          : (Int32)(*((Int32*)((char*)constPtr->getData() +
              op->getVcNullIndicatorLen()))));

        return ((char*)constPtr->getData() + op->getVcIndicatorLen() +
                       op->getVcNullIndicatorLen());
      }

      // Otherwise we have a fixed-length string.
      *len = op->getLen();
      return (char*)(constPtr->getData());

    default:
      assert(FALSE);
      break;
  }

  return NULL;
}

//
// Given any pointer operand, return the pointer to it.
//
void* PCodeCfg::getPtrConstValue(PCodeOperand* op)
{
  CollIndex off = op->getOffset();

  // This routine can be called before PCodeConstant objects are initialized.
  // As such, the offset must come directly from the constants area of the expr.
  if (offsetToConstMap_ == NULL) {
    char * stk = expr_->getConstantsArea();
    Int32 len = expr_->getConstsLength();

    assert (off < (CollIndex)len);

    return (void*)(stk + off);
  }

  // Otherwise constant should be recorded in constants map.
  PCodeConstants* constPtr = offsetToConstMap_->getFirstValue(&off);
  if (constPtr == NULL)
    return NULL;

  return (void*)(constPtr->getData());
}


//
// Given an operand known to be an integer, find its value and return it.
//
Int64 PCodeCfg::getIntConstValue(PCodeOperand* op)
{
  CollIndex off = op->getOffset();

  // TODO: move this calculation to getStringValue()
  if (op->isVarchar()) {
    off = off - op->getVcIndicatorLen() - op->getVcNullIndicatorLen();
  }

  PCodeConstants* constPtr = offsetToConstMap_->getFirstValue(&off);

  //
  // All integer constant operands should be found in constants hash table.  If
  // not, then they must be constants defined prior to optimizations.  An
  // example on how this can happen is a MOVE_MBIN8_MBIN8_IBIN32S which has a
  // constant read operand with type MPTR32.  Operands with type MPTR32 are not
  // currently tracked.  If this instruction is converted into a normal (known
  // sized) move, then constant folding may be called and it may be further
  // converted into an immediate move.
  //
  // TODO: Keep the assert for now (change made in ::getAlign() to recognize
  // PCIT::MPTR32 operand (giving it worst-case 8-byte alignment).
  //

  assert(constPtr != NULL);

  Int64 value = 0;

  switch (op->getType()) {
    case PCIT::MBIN8:
      value = (Int64)*((UInt8*)(constPtr->getData()));
      break;
    case PCIT::MBIN16S:
      value = (Int64)*((Int16*)(constPtr->getData()));
      break;
    case PCIT::MBIN16U:
      value = (Int64)*((UInt16*)(constPtr->getData()));
      break;
    case PCIT::MBIN32S:
      value = (Int64)*((Int32*)(constPtr->getData()));
      break;
    case PCIT::MBIN32U:
      value = (Int64)*((UInt32*)(constPtr->getData()));
      break;
    case PCIT::MBIN64S:
      value = (Int64)*((Int64*)(constPtr->getData()));
      break;
    case PCIT::MPTR32:
      switch (constPtr->getLen()) {
        case 1:
          value = (Int64)*((UInt8*)(constPtr->getData()));
          break;
        case 2:
          value = (Int64)*((Int16*)(constPtr->getData()));
          break;
        case 4:
          value = (Int64)*((Int32*)(constPtr->getData()));
          break;
        case 8:
          value = (Int64)*((Int64*)(constPtr->getData()));
          break;
        default:
          assert(FALSE);
      }
      break;

    default:
      assert(FALSE);
      break;
  }

  return value;
}

//
// Given an operand known to be a float, find its value and return it.
//
double PCodeCfg::getFloatConstValue(PCodeOperand* op)
{
  CollIndex off = op->getOffset();

  // TODO: move this calculation to getStringValue()
  if (op->isVarchar()) {
    off = off - op->getVcIndicatorLen() - op->getVcNullIndicatorLen();
  }

  PCodeConstants* constPtr = offsetToConstMap_->getFirstValue(&off);

  // All integer constant operands should be found in constants hash table.
  assert(constPtr != NULL);

  double value = 0;

  switch (op->getType()) {
    case PCIT::MFLT64:
      value = *((double*)(constPtr->getData()));
      break;

    case PCIT::MFLT32:
      value = *((float*)(constPtr->getData()));
      break;

    default:
      assert(FALSE);
      break;
  }

  return value;
}

//
// Add a new integer constant.  The alignment param is used to indicate the
// size of the datum as well as it's alignment.
//
Int32 PCodeCfg::addNewIntConstant(Int64 value, Int32 alignment)
{
  // Quickly return the zero offset if it exists and we're looking to add 0.
  if ((value == 0) && (zeroOffset_ != -1))
    return zeroOffset_;

  switch (alignment) {
    case 1:
    {
      UInt8 val = (UInt8)value;
      return *(addConstant(&val, alignment, alignment));
    }

    case 2:
    {
      UInt16 val = (UInt16)value;
      return *(addConstant(&val, alignment, alignment));
    }

    case 4:
    {
      UInt32 val = (UInt32)value;
      return *(addConstant(&val, alignment, alignment));
    }

    case 8:
    {
      return *(addConstant(&value, alignment, alignment));
    }

    default:
      assert(FALSE);
  }

  return -1;
}

//
// Add a new float constant.  The alignment param is used to indicate the
// size of the datum as well as it's alignment.
//
Int32 PCodeCfg::addNewFloatConstant(double value, Int32 alignment)
{
  switch (alignment) {
    case 4:
    {
      float val = (float)value;
      return *(addConstant(&val, alignment, alignment));
    }

    case 8:
    {
      return *(addConstant(&value, alignment, alignment));
    }

    default:
      assert(FALSE);
  }

  return -1;
}

//
// Constant fold instructions with input operands known to be constant.  It's
// important to update the reaching defs information if the new instruction
// being added is generated from scratch - see updateReachingDefs().
//
PCodeInst* PCodeCfg::constantFold(PCodeInst* inst, NABoolean rDefsAvailable)
{
  CollIndex i;
  PCodeOperand *op1, *op2;
  Int16 ov; // overflow indicator

  Int32 opc = inst->getOpcode();

  switch (opc) {
#if 0
    case PCIT::CLAUSE_EVAL:
    {
      ex_expr::exp_return_type retCode;
      char *opData[3 * ex_clause::MAX_OPERANDS];
      char **opDataData = &opData[2 * ex_clause::MAX_OPERANDS];

      clause = inst->code[1];

      if (inst->mustProcessNulls() ||
          !inst->clauseInitOperandsForConstFold(opData, heap_))
        return;

      retCode = clause->eval(opDataData, heap_, diagsArea)

      if (retCode == ex_expr::EXPR_ERROR)
        return;

      break;
    }
#endif

    case PCIT::MOVE_MASCII_MASCII_IBIN32S_IBIN32S:
    case PCIT::MOVE_MASCII_MATTR5_IBIN32S:
    {
      Int32 len, tgtLen;
      CollIndex* off;

      // Folding inst into same-sized moves is useful for copy propagation, and
      // that being only if the write operand is a temp.
      if (!inst->getWOps()[0]->isTemp())
        break;

      op2 = inst->getROps()[0];
      if (op2->isConst()) {
        assert(!op2->isIndirectVarchar());

        op1 = inst->getWOps()[0];
        tgtLen = op1->getLen();

        char* str = getStringConstValue(op2, &len);
        Int32 padLen = tgtLen - len;

        char* newStr = (char*)new(heap_) char[tgtLen];
        if (padLen > 0) {
          str_cpy_all(newStr, str, len);
          str_pad(newStr + len, padLen, ' ');
        }
        else {
          str_cpy_all(newStr, str, tgtLen);
        }

        off = addConstant((void*)newStr, tgtLen, 1);

        inst->code[0] = PCIT::MOVE_MBIN8_MBIN8_IBIN32S;
        inst->code[3] = 1;
        inst->code[4] = *off;
        inst->code[5] = tgtLen;

        NADELETEBASIC(newStr, heap_);
        inst->reloadOperands(this);
      }

      break;
    }

    case PCIT::MOVE_MATTR5_MASCII_IBIN32S:
    {
      Int32 len, tgtLen;
      CollIndex* off;

      op1 = inst->getWOps()[0];

      // Folding inst into same-sized moves is useful for copy propagation, and
      // that being only if the write operand is a temp.
      if (!op1->isTemp())
        break;

      op2 = inst->getROps()[0];
      if (op2->isConst()) {
        // Get source string
        char* str = getStringConstValue(op2, &len);

        // Since char is being stored in varchar, if the length is too big to
        // be represented by a varchar (in terms of a 2-byte vc length field),
        // then skip this transformation.
        if (len >= SHRT_MAX)
          break;

        // Target length that should be stored.
        tgtLen = (op1->getVcMaxLen() >= len) ? len : op1->getVcMaxLen();

        // Allocate new varchar string with 2 bytes for vc len
        char* newStr = (char*) new(heap_) char[2 + tgtLen];
        *((Int16*)(newStr)) = (Int16)tgtLen;  
        str_cpy_all(newStr + 2, str, tgtLen);

        off = addConstant((void*)newStr, tgtLen + 2, 2);

        Int32 comboLen = 0;
        char* comboPtr = (char*)(&comboLen);
        comboPtr[0] = 0; // no null
        comboPtr[1] = 2; // vc length is 2 bytes

        PCodeInst* newInst;
        PCodeBlock* block = inst->block;

        newInst = block->insertNewInstAfter(inst, PCIT::MOVE_MATTR5_MATTR5);

        newInst->code[1] = inst->code[1];
        newInst->code[2] = inst->code[2];
        newInst->code[3] = inst->code[3];
        newInst->code[4] = inst->code[4];
        newInst->code[5] = inst->code[5];

        newInst->code[6] = 1;
        newInst->code[7] = *off + 2;            // Up 2 to get to str itself
        newInst->code[8] = -1;                  // voa
        newInst->code[9] = op1->getVcMaxLen();  // tgt/src share max vc length
        newInst->code[10] = comboLen;

        newInst->reloadOperands(this);

        // Attempt to update reaching defs info.
        if (rDefsAvailable)
          updateReachingDefs(block, op1->getBvIndex(), inst, newInst);

        block->deleteInst(inst);
        inst = newInst;

        NADELETEBASIC(newStr, heap_);
      }

      break;
    }

    case PCIT::MOVE_MBIGS_MBIN16S_IBIN32S:
    case PCIT::MOVE_MBIGS_MBIN32S_IBIN32S:
    case PCIT::MOVE_MBIGS_MBIN64S_IBIN32S:
    {
      op1 = inst->getWOps()[0];

      // Folding inst into same-sized moves is useful for copy propagation, and
      // that being only if the write operand is a temp.
      if (!op1->isTemp())
        break;

      op2 = inst->getROps()[0];
      if (op2->isConst()) {
        CollIndex* off;
        Int64 value = getIntConstValue(op2);
        Int32 tgtLen = inst->getWOps()[0]->getLen();
        char* result = (char*) new(heap_) char[tgtLen];
        BigNumHelper::ConvInt64ToBigNumWithSignHelper(tgtLen, value, result);

        off = addConstant((void*)result, tgtLen,inst->getWOps()[0]->getAlign());

        PCodeInst* newInst;
        PCodeBlock* block = inst->block;

        newInst = block->insertNewInstAfter(inst,
                    PCIT::MOVE_MBIGS_MBIGS_IBIN32S_IBIN32S);
        newInst->code[1] = inst->code[1];
        newInst->code[2] = inst->code[2];
        newInst->code[3] = 1;
        newInst->code[4] = *off;
        newInst->code[5] = tgtLen;
        newInst->code[6] = tgtLen;

        newInst->reloadOperands(this);

        // Attempt to update reaching defs info.
        if (rDefsAvailable)
          updateReachingDefs(block, op1->getBvIndex(), inst, newInst);

        block->deleteInst(inst);
        inst = newInst;
      }

      break;
    }

    case PCIT::MOVE_MBIN32S_IBIN32S:
    {
      PCodeInst* newInst;
      PCodeBlock* block = inst->block;

      op1 = inst->getWOps()[0];

      // Folding inst into same-sized moves is useful for copy propagation, and
      // that being only if the write operand is a temp.
      if (!op1->isTemp())
        break;

      newInst = block->insertNewInstAfter(inst, PCIT::MOVE_MBIN32U_MBIN32U);
      newInst->code[1] = inst->code[1];
      newInst->code[2] = inst->code[2];
      newInst->code[3] = 1;
      newInst->code[4] = addNewIntConstant(inst->code[3], 4);

      newInst->reloadOperands(this);

      // Attempt to update reaching defs info.
      if (rDefsAvailable)
        updateReachingDefs(block, op1->getBvIndex(), inst, newInst);

      block->deleteInst(inst);
      inst = newInst;

      break;
    }

    case PCIT::MOVE_MBIN64S_MBIN16U:
    case PCIT::MOVE_MBIN64S_MBIN16S:
    case PCIT::MOVE_MBIN64S_MBIN32S:
    case PCIT::MOVE_MBIN64S_MBIN32U:

    case PCIT::MOVE_MBIN32U_MBIN16U:
    case PCIT::MOVE_MBIN32S_MBIN16U:
    case PCIT::MOVE_MBIN32U_MBIN16S:
    case PCIT::MOVE_MBIN32S_MBIN16S:

    case PCIT::MOVE_MBIN16U_MBIN8:
    {
      // Folding inst into same-sized moves is useful for copy propagation, and
      // that being only if the write operand is a temp.
      if (!inst->getWOps()[0]->isTemp())
        break;

      op1 = inst->getROps()[0];
      if (op1->isConst()) {
        Int64 value = getIntConstValue(op1);

        // Re-define the instruction and reload operands
        switch (inst->getWOps()[0]->getType()) {
          case PCIT::MBIN64S:
            inst->code[0] = PCIT::MOVE_MBIN64S_MBIN64S;
            inst->code[4] = addNewIntConstant(value, 8);
            break;

          case PCIT::MBIN32U:
          case PCIT::MBIN32S:
            inst->code[0] = PCIT::MOVE_MBIN32U_MBIN32U;
            inst->code[4] = addNewIntConstant(value, 4);
            break;

          case PCIT::MBIN16U:
            inst->code[0] = PCIT::MOVE_MBIN16U_MBIN16U;
            inst->code[4] = addNewIntConstant(value, 2);
            break;
        }

        inst->reloadOperands(this);
      }

      break;
    }

    case PCIT::MOVE_MFLT64_MBIN16S:
    case PCIT::MOVE_MFLT64_MBIN32S:
    case PCIT::MOVE_MFLT64_MBIN64S:
    case PCIT::MOVE_MFLT64_MFLT32:
    {
      // Folding inst into same-sized moves is useful for copy propagation, and
      // that being only if the write operand is a temp.
      if (!inst->getWOps()[0]->isTemp())
        break;

      op1 = inst->getROps()[0];
      if (op1->isConst()) {
        double fvalue =
          (inst->getROps()[0]->getType() == PCIT::MFLT32)
            ? (double)(getFloatConstValue(op1))
            : (double)(getIntConstValue(op1));

        // Re-define the instruction and reload operands
        inst->code[0] = PCIT::MOVE_MBIN64S_MBIN64S;
        inst->code[4] = addNewFloatConstant(fvalue, 8);

        inst->reloadOperands(this);
      }

      break;
    }

    case PCIT::MUL_MBIN16S_MBIN16S_MBIN16S:
    case PCIT::MUL_MBIN32S_MBIN32S_MBIN32S:
    case PCIT::MUL_MBIN32S_MBIN16S_MBIN16S:
    case PCIT::MUL_MBIN32S_MBIN16S_MBIN32S:
    case PCIT::MUL_MBIN64S_MBIN16S_MBIN32S:
    case PCIT::MUL_MBIN64S_MBIN32S_MBIN32S:
    case PCIT::MUL_MBIN64S_MBIN64S_MBIN64S:
      for (i=0; i < inst->getROps().entries(); i++) {
        op1 = inst->getROps()[i];
        if (op1->isConst() && (getIntConstValue(op1) == 1)) {
          switch (opc) {
            case PCIT::MUL_MBIN64S_MBIN16S_MBIN32S:
              inst->code[0] = (i == 0) ? PCIT::MOVE_MBIN64S_MBIN32S :
                                         PCIT::MOVE_MBIN64S_MBIN16S;
              break;

            case PCIT::MUL_MBIN32S_MBIN16S_MBIN32S:
              inst->code[0] = (i == 0) ? PCIT::MOVE_MBIN32U_MBIN32U :
                                         PCIT::MOVE_MBIN32S_MBIN16S;
              break;

            case PCIT::MUL_MBIN16S_MBIN16S_MBIN16S:
              inst->code[0] = PCIT::MOVE_MBIN16U_MBIN16U;
              break;

            case PCIT::MUL_MBIN32S_MBIN32S_MBIN32S:
              inst->code[0] = PCIT::MOVE_MBIN32U_MBIN32U;
              break;

            case PCIT::MUL_MBIN32S_MBIN16S_MBIN16S:
              inst->code[0] = PCIT::MOVE_MBIN32S_MBIN16S;
              break;

            case PCIT::MUL_MBIN64S_MBIN32S_MBIN32S:
              inst->code[0] = PCIT::MOVE_MBIN64S_MBIN32S;
              break;

            case PCIT::MUL_MBIN64S_MBIN64S_MBIN64S:
              inst->code[0] = PCIT::MOVE_MBIN64S_MBIN64S;
              break;
          }

          inst->code[3] = inst->getROps()[(i+1)%2]->getStackIndex();
          inst->code[4] = inst->getROps()[(i+1)%2]->getOffset();

          inst->reloadOperands(this);

          // Try to fold the move even further
          return constantFold(inst, rDefsAvailable);
        }
      }

      op1 = inst->getROps()[0];
      op2 = inst->getROps()[1];

      if (op1->isConst() && op2->isConst())
      {
        Int64 x = getIntConstValue(op1);
        Int64 y = getIntConstValue(op2);

        Int64 value = EXP_FIXED_OV_MUL(x, y, &ov);

        if (ov)
          return inst;

        Int32 offset = addNewIntConstant(value, inst->getWOps()[0]->getLen());
        inst->code[0] = inst->generateCopyMoveOpc(
                          inst->getWOps()[0]->getType());
        inst->code[3] = 1;
        inst->code[4] = offset;

        inst->reloadOperands(this);
      }

      break;

    case PCIT::EQ_MBIN32S_MASCII_MASCII:
    case PCIT::LT_MBIN32S_MASCII_MASCII:
    case PCIT::LE_MBIN32S_MASCII_MASCII:
    case PCIT::GT_MBIN32S_MASCII_MASCII:
    case PCIT::GE_MBIN32S_MASCII_MASCII:
    {
      op1 = inst->getROps()[0];
      op2 = inst->getROps()[1];

      if (op1->isConst() && op2->isConst())
      {
        Int32 z, len;

        char* x = getStringConstValue(op1, &len);
        char* y = getStringConstValue(op2, &len);

        z = memcmp(x, y, op1->getLen());

        switch (opc) {
          case PCIT::EQ_MBIN32S_MASCII_MASCII:
            z = (z == 0);
            break;

          case PCIT::LT_MBIN32S_MASCII_MASCII:
            z = (z < 0);
            break;

          case PCIT::LE_MBIN32S_MASCII_MASCII:
            z = (z <= 0);
            break;

          case PCIT::GT_MBIN32S_MASCII_MASCII:
            z = (z > 0);
            break;

          case PCIT::GE_MBIN32S_MASCII_MASCII:
            z = (z >= 0);
            break;
        }

        Int32 offset = addNewIntConstant(z, 4);
        inst->code[0] = inst->generateCopyMoveOpc(PCIT::MBIN32S);
        inst->code[3] = 1;
        inst->code[4] = offset;

        inst->reloadOperands(this);
      }

      break;
    }


    case PCIT::LE_MBIN32S_MBIN16S_MBIN16S:
    case PCIT::LE_MBIN32S_MBIN16S_MBIN32S:
    case PCIT::LE_MBIN32S_MBIN32S_MBIN32S:
    case PCIT::LE_MBIN32S_MBIN16U_MBIN16U:
    case PCIT::LE_MBIN32S_MBIN16U_MBIN32U:
    case PCIT::LE_MBIN32S_MBIN32U_MBIN32U:
    case PCIT::LE_MBIN32S_MBIN16S_MBIN32U:
    case PCIT::LE_MBIN32S_MBIN16U_MBIN16S:
    case PCIT::LE_MBIN32S_MBIN64S_MBIN64S:

    case PCIT::GE_MBIN32S_MBIN16S_MBIN16S:
    case PCIT::GE_MBIN32S_MBIN16S_MBIN32S:
    case PCIT::GE_MBIN32S_MBIN32S_MBIN32S:
    case PCIT::GE_MBIN32S_MBIN16U_MBIN16U:
    case PCIT::GE_MBIN32S_MBIN16U_MBIN32U:
    case PCIT::GE_MBIN32S_MBIN32U_MBIN32U:
    case PCIT::GE_MBIN32S_MBIN16S_MBIN32U:
    case PCIT::GE_MBIN32S_MBIN16U_MBIN16S:
    case PCIT::GE_MBIN32S_MBIN64S_MBIN64S:

    case PCIT::LT_MBIN32S_MBIN16S_MBIN16S:
    case PCIT::LT_MBIN32S_MBIN16S_MBIN32S:
    case PCIT::LT_MBIN32S_MBIN32S_MBIN32S:
    case PCIT::LT_MBIN32S_MBIN16U_MBIN16U:
    case PCIT::LT_MBIN32S_MBIN16U_MBIN32U:
    case PCIT::LT_MBIN32S_MBIN32U_MBIN32U:
    case PCIT::LT_MBIN32S_MBIN16S_MBIN32U:
    case PCIT::LT_MBIN32S_MBIN16U_MBIN16S:
    case PCIT::LT_MBIN32S_MBIN64S_MBIN64S:

    case PCIT::GT_MBIN32S_MBIN16S_MBIN16S:
    case PCIT::GT_MBIN32S_MBIN16S_MBIN32S:
    case PCIT::GT_MBIN32S_MBIN32S_MBIN32S:
    case PCIT::GT_MBIN32S_MBIN16U_MBIN16U:
    case PCIT::GT_MBIN32S_MBIN16U_MBIN32U:
    case PCIT::GT_MBIN32S_MBIN32U_MBIN32U:
    case PCIT::GT_MBIN32S_MBIN16S_MBIN32U:
    case PCIT::GT_MBIN32S_MBIN16U_MBIN16S:
    case PCIT::GT_MBIN32S_MBIN64S_MBIN64S:

    case PCIT::EQ_MBIN32S_MBIN16S_MBIN16S:
    case PCIT::EQ_MBIN32S_MBIN16S_MBIN32S:
    case PCIT::EQ_MBIN32S_MBIN32S_MBIN32S:
    case PCIT::EQ_MBIN32S_MBIN16U_MBIN16U:
    case PCIT::EQ_MBIN32S_MBIN16U_MBIN32U:
    case PCIT::EQ_MBIN32S_MBIN32U_MBIN32U:
    case PCIT::EQ_MBIN32S_MBIN16S_MBIN32U:
    case PCIT::EQ_MBIN32S_MBIN16U_MBIN16S:
    case PCIT::EQ_MBIN32S_MBIN64S_MBIN64S:

    case PCIT::NE_MBIN32S_MBIN64S_MBIN64S:
    case PCIT::NE_MBIN32S_MBIN16S_MBIN16S:
    {
      op1 = inst->getROps()[0];
      op2 = inst->getROps()[1];

      if (op1->isConst() && op2->isConst())
      {
        Int32 z = 0;

        Int64 x = getIntConstValue(op1);
        Int64 y = getIntConstValue(op2);

        switch (opc) {
          case PCIT::LE_MBIN32S_MBIN16S_MBIN16S:
          case PCIT::LE_MBIN32S_MBIN16S_MBIN32S:
          case PCIT::LE_MBIN32S_MBIN32S_MBIN32S:
          case PCIT::LE_MBIN32S_MBIN16U_MBIN16U:
          case PCIT::LE_MBIN32S_MBIN16U_MBIN32U:
          case PCIT::LE_MBIN32S_MBIN32U_MBIN32U:
          case PCIT::LE_MBIN32S_MBIN16S_MBIN32U:
          case PCIT::LE_MBIN32S_MBIN16U_MBIN16S:
          case PCIT::LE_MBIN32S_MBIN64S_MBIN64S:
            z = (x <= y) ? 1 : 0;
            break;

          case PCIT::GE_MBIN32S_MBIN16S_MBIN16S:
          case PCIT::GE_MBIN32S_MBIN16S_MBIN32S:
          case PCIT::GE_MBIN32S_MBIN32S_MBIN32S:
          case PCIT::GE_MBIN32S_MBIN16U_MBIN16U:
          case PCIT::GE_MBIN32S_MBIN16U_MBIN32U:
          case PCIT::GE_MBIN32S_MBIN32U_MBIN32U:
          case PCIT::GE_MBIN32S_MBIN16S_MBIN32U:
          case PCIT::GE_MBIN32S_MBIN16U_MBIN16S:
          case PCIT::GE_MBIN32S_MBIN64S_MBIN64S:
            z = (x >= y) ? 1 : 0;
            break;

          case PCIT::LT_MBIN32S_MBIN16S_MBIN16S:
          case PCIT::LT_MBIN32S_MBIN16S_MBIN32S:
          case PCIT::LT_MBIN32S_MBIN32S_MBIN32S:
          case PCIT::LT_MBIN32S_MBIN16U_MBIN16U:
          case PCIT::LT_MBIN32S_MBIN16U_MBIN32U:
          case PCIT::LT_MBIN32S_MBIN32U_MBIN32U:
          case PCIT::LT_MBIN32S_MBIN16S_MBIN32U:
          case PCIT::LT_MBIN32S_MBIN16U_MBIN16S:
          case PCIT::LT_MBIN32S_MBIN64S_MBIN64S:
            z = (x < y) ? 1 : 0;
            break;

          case PCIT::GT_MBIN32S_MBIN16S_MBIN16S:
          case PCIT::GT_MBIN32S_MBIN16S_MBIN32S:
          case PCIT::GT_MBIN32S_MBIN32S_MBIN32S:
          case PCIT::GT_MBIN32S_MBIN16U_MBIN16U:
          case PCIT::GT_MBIN32S_MBIN16U_MBIN32U:
          case PCIT::GT_MBIN32S_MBIN32U_MBIN32U:
          case PCIT::GT_MBIN32S_MBIN16S_MBIN32U:
          case PCIT::GT_MBIN32S_MBIN16U_MBIN16S:
          case PCIT::GT_MBIN32S_MBIN64S_MBIN64S:
            z = (x > y) ? 1 : 0;
            break;

          case PCIT::EQ_MBIN32S_MBIN16S_MBIN16S:
          case PCIT::EQ_MBIN32S_MBIN16S_MBIN32S:
          case PCIT::EQ_MBIN32S_MBIN32S_MBIN32S:
          case PCIT::EQ_MBIN32S_MBIN16U_MBIN16U:
          case PCIT::EQ_MBIN32S_MBIN16U_MBIN32U:
          case PCIT::EQ_MBIN32S_MBIN32U_MBIN32U:
          case PCIT::EQ_MBIN32S_MBIN16S_MBIN32U:
          case PCIT::EQ_MBIN32S_MBIN16U_MBIN16S:
          case PCIT::EQ_MBIN32S_MBIN64S_MBIN64S:
            z = (x == y) ? 1 : 0;
            break;

          case PCIT::NE_MBIN32S_MBIN64S_MBIN64S:
          case PCIT::NE_MBIN32S_MBIN16S_MBIN16S:
            z = (x != y) ? 1 : 0;
            break;
        }

        Int32 offset = addNewIntConstant(z, 4);
        inst->code[0] = inst->generateCopyMoveOpc(PCIT::MBIN32S);
        inst->code[3] = 1;
        inst->code[4] = offset;

        inst->reloadOperands(this);
      }

      break;
    }

    case PCIT::ADD_MBIN16S_MBIN16S_MBIN16S:
    case PCIT::ADD_MBIN32S_MBIN32S_MBIN32S:
    case PCIT::ADD_MBIN64S_MBIN64S_MBIN64S:
    case PCIT::ADD_MBIN32S_MBIN16S_MBIN16S:
    case PCIT::ADD_MBIN32S_MBIN16S_MBIN32S:
    case PCIT::ADD_MBIN64S_MBIN32S_MBIN64S:
    {
      op1 = inst->getROps()[0];
      op2 = inst->getROps()[1];

      if (op1->isConst() && op2->isConst())
      {
        Int64 x = getIntConstValue(op1);
        Int64 y = getIntConstValue(op2);

        Int64 value = EXP_FIXED_OV_ADD(x, y, &ov);

        if (ov)
          return inst;

        Int32 offset = addNewIntConstant(value, inst->getWOps()[0]->getLen());
        inst->code[0] = inst->generateCopyMoveOpc(
                          inst->getWOps()[0]->getType());
        inst->code[3] = 1;
        inst->code[4] = offset;

        inst->reloadOperands(this);
      }

      break;
    }

    case PCIT::SUB_MBIN16S_MBIN16S_MBIN16S:
    case PCIT::SUB_MBIN32S_MBIN32S_MBIN32S:
    case PCIT::SUB_MBIN64S_MBIN64S_MBIN64S:
    case PCIT::SUB_MBIN32S_MBIN16S_MBIN16S:
    case PCIT::SUB_MBIN32S_MBIN16S_MBIN32S:
    case PCIT::SUB_MBIN32S_MBIN32S_MBIN16S:
    {
      op1 = inst->getROps()[0];
      op2 = inst->getROps()[1];

      if (op1->isConst() && op2->isConst())
      {
        Int64 x = getIntConstValue(op1);
        Int64 y = getIntConstValue(op2);

        Int64 value = EXP_FIXED_OV_SUB(x, y, &ov);

        if (ov)
          return inst;

        Int32 offset = addNewIntConstant(value, inst->getWOps()[0]->getLen());
        inst->code[0] = inst->generateCopyMoveOpc(
                          inst->getWOps()[0]->getType());
        inst->code[3] = 1;
        inst->code[4] = offset;

        inst->reloadOperands(this);
      }

      break;
    }

    case PCIT::MUL_MFLT64_MFLT64_MFLT64:
    case PCIT::ADD_MFLT64_MFLT64_MFLT64:
    case PCIT::SUB_MFLT64_MFLT64_MFLT64:
    case PCIT::DIV_MFLT64_MFLT64_MFLT64:
    {
      op1 = inst->getROps()[0];
      op2 = inst->getROps()[1];

      if (op1->isConst() && op2->isConst()) {
        double x = getFloatConstValue(op1);
        double y = getFloatConstValue(op2);

        double res = 0.0;
        Int16 ov = 0;

        switch (opc) {
          case PCIT::MUL_MFLT64_MFLT64_MFLT64:
            res = MathReal64Mul(x, y, &ov);
            break;
          case PCIT::ADD_MFLT64_MFLT64_MFLT64:
            res = MathReal64Add(x, y, &ov);
            break;
          case PCIT::SUB_MFLT64_MFLT64_MFLT64:
            res = MathReal64Sub(x, y, &ov);
            break;
          case PCIT::DIV_MFLT64_MFLT64_MFLT64:
            res = MathReal64Div(x, y, &ov);
            break;
        }

        if (ov)
          return inst;

        // FIXME: I think we need to appropriately support a MOVE_MFLT64_MFLT64
        // instruction so that Native Expressions doesn't get confused.

        Int32 offset = addNewFloatConstant(res, 8);
        inst->code[0] = PCIT::MOVE_MBIN64S_MBIN64S;
        inst->code[3] = 1;
        inst->code[4] = offset;

        inst->reloadOperands(this);
      }

      break;
    }

    case PCIT::HASH_MBIN32U_MATTR5:
    case PCIT::HASH_MBIN32U_MPTR32_IBIN32S_IBIN32S:
    case PCIT::HASH_MBIN32U_MBIN16_IBIN32S_IBIN32S:
    case PCIT::HASH_MBIN32U_MBIN32_IBIN32S_IBIN32S:
    case PCIT::HASH_MBIN32U_MBIN64_IBIN32S_IBIN32S:
    {
      op1 = inst->getROps()[0];

      if (op1->isConst()) {
        Int32 len;
        char* x;
        UInt32 flags = ExHDPHash::NO_FLAGS;

        if (opc == PCIT::HASH_MBIN32U_MATTR5) {
          x = getStringConstValue(op1, &len);

          // Reduce length if padding exists.
          while ((len > 0) && (x[len-1] == ' '))
            len--;
        }
        else {
          len = op1->getLen();
          x = (char*)getPtrConstValue(op1);
          flags = inst->code[5];
        }

        UInt32 res = ExHDPHash::hash(x, flags, len);

        Int32 offset = addNewIntConstant(res, 4);
        inst->code[0] = inst->generateCopyMoveOpc(PCIT::MBIN32S);
        inst->code[3] = 1;
        inst->code[4] = offset;

        inst->reloadOperands(this);
      }

      break;
    }

    case PCIT::RANGE_LOW_S32S64:
    case PCIT::RANGE_HIGH_S32S64:
    case PCIT::RANGE_LOW_U32S64:
    case PCIT::RANGE_HIGH_U32S64:
    case PCIT::RANGE_LOW_S64S64:
    case PCIT::RANGE_HIGH_S64S64:
    case PCIT::RANGE_LOW_S16S64:
    case PCIT::RANGE_HIGH_S16S64:
    case PCIT::RANGE_MFLT64:
    {
      Int64 x, y;
      NABoolean error = TRUE; // Assume range instruction is needed

      PCodeOperand* op = inst->getROps()[0];

      // Only range instructions with const operands can be removed
      if (!op->isConst())
        break;

      if (opc != PCIT::RANGE_MFLT64) {
        x = getIntConstValue(op);
        y = (*(Int64*)&inst->code[3]);
      }

      switch (opc) {
        case PCIT::RANGE_LOW_S32S64:
        case PCIT::RANGE_LOW_U32S64:
        case PCIT::RANGE_LOW_S64S64:
        case PCIT::RANGE_LOW_S16S64:
          error = (x < y);
          break;

        case PCIT::RANGE_HIGH_S32S64:
        case PCIT::RANGE_HIGH_U32S64:
        case PCIT::RANGE_HIGH_S64S64:
        case PCIT::RANGE_HIGH_S16S64:
          error = (x > y);
          break;

        case PCIT::RANGE_MFLT64:
          error = FALSE;
          break;
      }

      if (!error) {
        PCodeBlock* block = inst->block;
        block->deleteInst(inst);
        inst = NULL;
      }
      break;
    }

    case PCIT::COMP_MBIN32S_MATTR5_MATTR5_IBIN32S:
    {
      op1 = inst->getROps()[0];
      op2 = inst->getROps()[1];
      if (!op1->isConst() || !op2->isConst())
        break;

      Int32 len1, len2;

      char* str1 = getStringConstValue(op1, &len1);
      char* str2 = getStringConstValue(op2, &len2);

      Int32 compTable[6][3] = {
        /* ITM_EQUAL */      {0, 1, 0},
        /* ITM_NOT_EQUAL */  {1, 0, 1},
        /* ITM_LESS */       {1, 0, 0},
        /* ITM_LESS_EQ */    {1, 1, 0},
        /* ITM_GREATER */    {0, 0, 1},
        /* ITM_GREATER_EQ */ {0, 1, 1}
      };

      // Set table pointer to appropriate position based on operation
      Int32* table = &(compTable[inst->code[13] - ITM_EQUAL][1]);

      Int32 compCode = charStringCompareWithPad(str1, len1, str2, len2, ' ');
      Int32 res = table[compCode];

      Int32 offset = addNewIntConstant(res, 4);
      inst->code[0] = inst->generateCopyMoveOpc(PCIT::MBIN32S);
      inst->code[3] = 1;
      inst->code[4] = offset;

      inst->reloadOperands(this);

      break;
    }

    case PCIT::GENFUNC_MATTR5_MATTR5_IBIN32S:
    {
      Int32 len;
      Int32 subOpc = inst->code[11];

      // Get source operand and check if constant.
      op2 = inst->getROps()[0];
      if (!op2->isConst())
        break;

      // Get tgt operand.
      op1 = inst->getWOps()[0];

      switch (subOpc) {
        case ITM_TRIM:
        case ITM_LTRIM:
        case ITM_RTRIM:
        case ITM_UPPER:
        case ITM_LOWER:
        {
          Int32 startIdx = 0;
          char* fromStr = NULL;
          NABoolean fromStrAllocated = FALSE;
          PCodeInst* newInst;

          PCodeBlock* block = inst->block;

          // Get source string and len (initial target length)
          char* str = getStringConstValue(op2, &len);
          CollIndex tgtLen = len;

          if ((subOpc == ITM_LOWER) || (subOpc == ITM_UPPER))
          {
            fromStr = (char*) new(heap_) char[tgtLen];
            for (i=0; i < tgtLen; i++)
              fromStr[i] = (subOpc == ITM_UPPER)
                ? TOUPPER(str[i]) : TOLOWER(str[i]);

            fromStrAllocated = TRUE;
          }
          else {
            // Set string up to first non-space
            if ((subOpc == ITM_TRIM) || (subOpc == ITM_LTRIM)) {
              for (i=0; i < (CollIndex)len; i++, startIdx++, tgtLen--)
                if (str[i] != ' ')
                  break;
            }

            // Set string up to last non-space
            if ((subOpc == ITM_TRIM) || (subOpc == ITM_RTRIM)) {

              if ( len > 0 ) // Protect against 0-length strings
              {
                for (i=len-1; i > (CollIndex)startIdx; i--, tgtLen--)
                  if (str[i] != ' ')
                    break;
              }
            }

            // Set "fromStr" to be "str" so that code below works for all cases.
            fromStr = str;
          }

          if (op1->isVarchar()) {
            // Allocate new varchar string with 2 bytes for vc len
            char* newStr = (char*) new(heap_) char[2 + tgtLen];
            *((Int16*)(newStr)) = (Int16)tgtLen;  

            if (tgtLen > 0)
              str_cpy_all(newStr + 2, &fromStr[startIdx], tgtLen);

            CollIndex* off = addConstant((void*)newStr, tgtLen + 2, 2);

            Int32 comboLen = 0;
            char* comboPtr = (char*)(&comboLen);
            comboPtr[0] = 0; // no null
            comboPtr[1] = 2; // vc length is 2 bytes

            newInst = block->insertNewInstAfter(inst, PCIT::MOVE_MATTR5_MATTR5);

            newInst->code[1] = inst->code[1];
            newInst->code[2] = inst->code[2];
            newInst->code[3] = inst->code[3];
            newInst->code[4] = inst->code[4];
            newInst->code[5] = inst->code[5];

            newInst->code[6] = 1;
            newInst->code[7] = *off + 2;            // Up 2 to get to str itself
            newInst->code[8] = -1;                  // voa
            newInst->code[9] = op1->getVcMaxLen();  // tgt/src share max vc len
            newInst->code[10] = comboLen;
 
            NADELETEBASIC(newStr, heap_);
          }
          else {
            CollIndex* off = addConstant((void*)fromStr, tgtLen, 1);

            newInst =
              block->insertNewInstAfter(inst, PCIT::MOVE_MBIN8_MBIN8_IBIN32S);

            newInst->code[1] = inst->code[1];
            newInst->code[2] = inst->code[2];
            newInst->code[3] = 1;
            newInst->code[4] = *off;
            newInst->code[5] = tgtLen;
          }

          newInst->reloadOperands(this);

          // Attempt to update reaching defs info.
          if (rDefsAvailable)
            updateReachingDefs(block, op1->getBvIndex(), inst, newInst);

          block->deleteInst(inst);
          inst = newInst;

          // Cleanup if needed.
          if (fromStrAllocated)
            NADELETEBASIC(fromStr, heap_);

          break;
        }

        default:
          break;
      }
    }

    default:
      break;
  }

  return inst;
}
