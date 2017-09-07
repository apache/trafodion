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

#include "ExpPCodeOptimizations.h"

// Forward declarations

static void setNeg1ConstantsForNullBranch(PCodeInst* inst,
                                                     NABitVector& zeroes,
                                                     NABitVector& ones,
                                                     NABitVector& neg1);

//
// Short circuit optimization for conditionals
//
void PCodeCfg::reorderPredicatesHelper()
{
  CollIndex i;

  FOREACH_BLOCK(srcBlock, firstInst, lastInst, index)
  {
    if (lastInst && (lastInst->getOpcode() == PCIT::RETURN) &&
        lastInst->prev && (lastInst->prev->getOpcode() == PCIT::MOVE_MBIN32S))
    {
      NABoolean expand = FALSE;

      // First check to see if a logical branch comes into this RETURN
      for (i=0; !expand && (i < srcBlock->getPreds().entries()); i++) {
        PCodeInst* l = srcBlock->getPreds()[i]->getLastInst();
        if (l && l->isLogicalBranch())
          expand = TRUE;
      }

      // Don't continue if there is no point to expand
      if (expand == FALSE)
        continue;

      PCodeBlock* retBlk;
      PCodeInst *retInst, *branch;

      branch = srcBlock->insertNewInstAfter(NULL, PCIT::BRANCH_OR);
      // BRANCH_OR code[0] is opcode, code[1] and code[2] are target
      // clause pointer; code[3] and code[4] are 64-bit 0; code[5] and code[6]
      // for operand #1; code[7] and code[8] for operand #2
      branch->code[3] = 0; // not necessary to clear
      branch->code[4] = 0; // not necessary to clear
      branch->code[5] = lastInst->prev->code[1];
      branch->code[6] = lastInst->prev->code[2];
      branch->code[7] = branch->code[5];
      branch->code[8] = branch->code[6];

      branch->reloadOperands(this);

      // Get rid of MOVE_MBIN32S and RETURN instructions
      srcBlock->deleteInst(lastInst->prev);
      srcBlock->deleteInst(lastInst);

      // Create fall-through block
      retBlk = createBlock();
      retInst = retBlk->insertNewInstBefore(NULL, PCIT::RETURN_IBIN32S);
      retInst->code[1] = 0;

      // Add fall-through edge (first edge)
      srcBlock->addEdge(retBlk);

      // Now create target block
      retBlk = createBlock();
      retInst = retBlk->insertNewInstBefore(NULL, PCIT::RETURN_IBIN32S);
      retInst->code[1] = 1;

      // Add target edge (second edge)
      srcBlock->addEdge(retBlk);

      // We can break out now - no more RETURNs with MOVE_MBIN32S
      break;
    }
  } ENDFE_BLOCK

#if 0

  FOREACH_BLOCK(srcBlock, firstInst, lastInst, index)
  {
    // Ignore empty blocks
    if (!firstInst)
      continue;

    if (lastInst->isLogicalBranch())
    {
      NABitVector zeroes(heap_);
      NABitVector ones(heap_);
      NABitVector* bv;

      Int32 opcode = lastInst->getOpcode();
      PCodeOperand* read = lastInst->getROps()[0];
      PCodeOperand* write = lastInst->getWOps()[0];

      PCodeInst* indicatorInst = NULL;
      PCodeBlock* block = srcBlock;

      // The src and tgt operands are either 0 or 1 - set them so.
      bv = (opcode == PCIT::BRANCH_AND) ? &zeroes : &ones;
      bv->addElementFast(read->getBvIndex());
      bv->addElementFast(write->getBvIndex());

      block = block->getTargetBlock();

      // Starting with the block's target block, visit it and propogate 
      // constants.  If the direction of that target block can be determined,
      // restart the analysis with that block's target. 

      while (TRUE)
      {
        NABoolean branchTaken = FALSE;
        indicatorInst =
          shortCircuitConstantProp(block, &zeroes, &ones, &branchTaken);

        // Bail out if not all the instructions in the block could be evaluated.
        if (indicatorInst != NULL)
          break;

        block = (branchTaken ? block->getTargetBlock() :
                               block->getFallThroughBlock());
      }

      // Remove the constants associated with the original logical branch.
      // This is because the constants for the operands used in this branch
      // are implicitly set when the branch is executed.
      *bv -= read->getBvIndex();
      *bv -= write->getBvIndex();

      // If we shouldn't create a new block at all because there could be no
      // value in it, don't do it.
      if ((block == srcBlock->getTargetBlock()) && !block->getSuccs().isEmpty())
        continue;

      // Only write the stores which are actually live
      // TODO: Probably should make sure only TEMP vars are being removed
      //zeroes.intersectSet(block->liveVector);
      //ones.intersectSet(block->liveVector);

      // First remove edge from source block to it's previous target
      srcBlock->removeEdge(srcBlock->getTargetBlock());

      PCodeBlock* newBlock = createBlock();

      // Source block now goes to new block
      srcBlock->addEdge(newBlock);

      // Generate move instructions of the constants discovered during the 
      // short circuit evaluation. 
      shortCircuitOptHelper(newBlock, &zeroes, 0);
      shortCircuitOptHelper(newBlock, &ones, 1);

      // As a side optimization, if the block is an exit block, then go ahead
      // and copy the instructions starting from "indicatorInst" into the new
      // block. Otherwise, we need to add a branch in the new block to go to
      // the final target block.

      if (block->getSuccs().isEmpty())
      {
        FOREACH_INST_IN_BLOCK_AT(block, inst, indicatorInst) {
          PCodeInst* copy = copyInst(inst);
          newBlock->insertInstAfter(NULL, copy);
        } ENDFE_INST_IN_BLOCK_AT
      }
      else
      {
        lastInst = newBlock->insertNewInstAfter(NULL, PCIT::BRANCH);
        newBlock->addEdge(block);
      }
    }
  } ENDFE_BLOCK

#endif

}

//
// Small version of constant propagation tailored for short circuit eval.
// 
PCodeInst* PCodeCfg::shortCircuitConstantProp(PCodeBlock* block,
                                              NABitVector* zeroes,
                                              NABitVector* ones,
                                              NABoolean* isBranchTaken)
{
  Int32 constant;
  NABitVector *bv1, *bv2;

  FOREACH_INST_IN_BLOCK(block, inst)
  {
    OPLIST reads = inst->getROps();
    OPLIST writes = inst->getWOps();

    Int32 opc = inst->getOpcode();
    switch (opc)
    {
      // For logical operators, propogate constants 0 and 1.
      case PCIT::AND_MBIN32S_MBIN32S_MBIN32S:
      case PCIT::OR_MBIN32S_MBIN32S_MBIN32S :
        bv1 = (opc == PCIT::OR_MBIN32S_MBIN32S_MBIN32S) ? ones : zeroes;
        bv2 = (opc == PCIT::OR_MBIN32S_MBIN32S_MBIN32S) ? zeroes : ones;

        if (bv1->testBit(reads[0]->getBvIndex()) ||
            bv1->testBit(reads[1]->getBvIndex())) {
          bv1->addElementFast(writes[0]->getBvIndex());
        }
        else if (bv2->testBit(reads[0]->getBvIndex()) &&
                 bv2->testBit(reads[1]->getBvIndex())) {
          bv2->addElementFast(writes[0]->getBvIndex());
        }
        else
          return inst;
        break;

      // For logical branches, propagate constants 0 and 1. Also, determine the
      // direction taken by the branch if possible.
      case PCIT::BRANCH_AND:
      case PCIT::BRANCH_OR:
        bv1 = (opc == PCIT::BRANCH_AND) ? zeroes : ones;
        bv2 = (opc == PCIT::BRANCH_AND) ? ones: zeroes;

        if (bv1->testBit(reads[0]->getBvIndex())) {
          bv1->addElementFast(writes[0]->getBvIndex());
          *isBranchTaken = TRUE;
        }
        else if (bv2->testBit(reads[0]->getBvIndex())) {
          bv2->addElementFast(writes[0]->getBvIndex());
          *isBranchTaken = FALSE;
        }
        else
          return inst;

        break;

      case PCIT::MOVE_MBIN32U_MBIN32U:
        if (zeroes->testBit(reads[0]->getBvIndex()))
          zeroes->addElementFast(writes[0]->getBvIndex());
        else if (ones->testBit(reads[0]->getBvIndex()))
          ones->addElementFast(writes[0]->getBvIndex());
        else
          return inst;
        break;

      case PCIT::BRANCH:
        *isBranchTaken = FALSE;
        break;

      case PCIT::MOVE_MBIN32S_IBIN32S:
        constant = inst->code[3];
        if (constant == 0)
          zeroes->addElementFast(writes[0]->getBvIndex());
        else if (constant == 1)
          ones->addElementFast(writes[0]->getBvIndex());
        else {
          return inst;
        }
        break;

      case PCIT::NULL_VIOLATION_MBIN16U_MBIN16U:
        if (!zeroes->testBit(reads[0]->getBvIndex()) ||
            !zeroes->testBit(reads[1]->getBvIndex()))
          return inst;
        break;


      // These instructions show up sometimes, but can't be handled
      case PCIT::NULL_VIOLATION_MBIN16U:
      case PCIT::MOVE_MBIN32S:
        return inst;

      default:
        // All instructions up to "inst" were evaluated with constants
        return inst;
    }
  } ENDFE_INST_IN_BLOCK

  // All instructions in block were evaluated with constants
  return NULL;
}

//
// Create immediate moves of operands identified in the provided bit vector.
// 
void PCodeCfg::shortCircuitOptHelper(PCodeBlock* block,
                                     NABitVector* bv,
                                     Int32 constant)
{
  CollIndex i = bv->getLastStaleBit();
  for (; (i = bv->prevUsed(i)) != NULL_COLL_INDEX; i--)
  {
    PCodeOperand* operand = indexToOperandMap_->getFirstValue(&i);
    PCodeInst* inst =
      block->insertNewInstAfter(NULL, PCIT::MOVE_MBIN32S_IBIN32S);

    inst->code[1] = operand->getStackIndex();
    inst->code[2] = operand->getOffset();
    inst->code[3] = constant;

    loadOperandsOfInst(inst);
  }
}

NABoolean PCodeCfg::removeDuplicateBlocks(PCodeBlock* start)
{
  CollIndex i, j, k;

  BLOCKLIST dupBlocks(heap_);
  BLOCKLIST requiredBlocks(heap_);

  NABoolean dupFound = FALSE;

  // First accumulate potential duplicate blocks
  FOREACH_BLOCK_DFO_AT(start, block, firstInst, lastInst, index) {
    if (block->isExitBlock() || (block->getSuccs().entries() == 1)) {
      NABoolean isFallThroughBlock = FALSE;
      NABoolean hasPriority = FALSE;

      // Prepare block for algorithm
      block->setVisitedFlag(FALSE);
#if 0
      // Init # of insts in this block
      block->setNumOfInsts();
#endif

      // If this block has a pred which falls through to it, then it is required
      for (i=0; i < block->getPreds().entries(); i++) {
        PCodeBlock* pred = block->getPreds()[i];
        PCodeInst* predLastInst = pred->getLastInst();

        if (predLastInst && predLastInst->isLogicalBranch())
          hasPriority = TRUE;

        // Identify pred if it truly falls through to the block
        if ((pred->getFallThroughBlock() == block) &&
            !(predLastInst && predLastInst->isUncBranch()))
        {
          if (hasPriority)
            requiredBlocks.insertAt(0, block);
          else
            requiredBlocks.insert(block);

          isFallThroughBlock = TRUE;
          break;
        }
      }

      if (!isFallThroughBlock) {
        if (hasPriority)
          dupBlocks.insertAt(0, block);
        else
          dupBlocks.insert(block);
      }
    }
  } ENDFE_BLOCK_DFO_AT

  // Use required blocks to first get rid of duplicate blocks
  for (i=0; i < requiredBlocks.entries(); i++) {
    PCodeBlock* reqBlock = requiredBlocks[i];

    // If this block was already processed, continue
    if (reqBlock->getVisitedFlag())
      continue;

    // Mark the required block as having been seen
    reqBlock->setVisitedFlag(TRUE);

    // Go through each exit block with no fall-through preds
    for (j=0; j < dupBlocks.entries(); j++) {
      NABoolean match = TRUE;
      PCodeBlock* dupBlock = dupBlocks[j];

      if (dupBlock->getVisitedFlag())
        continue;

      // Make sure succs are identical
      if (reqBlock->getSuccs().entries() == 1)
        if ((dupBlock->getSuccs().entries() != 1) ||
            (reqBlock->getSuccs()[0] != dupBlock->getSuccs()[0]))
          continue;


#if 0
      if (reqBlock->getNumOfInsts() != dupBlock->getNumOfInsts())
        continue;
#endif

      PCodeInst* inst1 = reqBlock->getFirstInst();
      PCodeInst* inst2 = dupBlock->getFirstInst();

      while (match && (inst1 || inst2)) {

        // Unconditional branches should be skipped when comparing blocks
        if (inst1 && inst1->isUncBranch()) {
          inst1 = inst1->next;
          continue;
        }

        if (inst2 && inst2->isUncBranch()) {
          inst2 = inst2->next;
          continue;
        }

        // Compare opcodes of the instructions
        if ((inst1 == NULL) || (inst2 == NULL) || 
            (inst1->getOpcode() != inst2->getOpcode())) {
          match = FALSE;
          break;
        }

        for (k=0;
             match && (k < (CollIndex)PCode::getInstructionLength(inst1->code));
             k++)
          match = match && (inst1->code[k] == inst2->code[k]);

        inst1 = inst1->next;
        inst2 = inst2->next;
      }

      if (!match)
        continue;

      dupFound = TRUE;

      // Redirect all preds of block to reqBlock.  Original links will get
      // deleted when the block gets deleted below.
      for (k=0; k < dupBlock->getPreds().entries(); k++) {
        dupBlock->getPreds()[k]->addEdge(reqBlock);
      }

      // Delete and remove exit block from list
      deleteBlock(dupBlock);
      dupBlock->setVisitedFlag(TRUE);
    }

    // If we just processed the last required exit block, start algorithm
    // again using the no-pred-fallthrough exit blocks.
    if (i == requiredBlocks.entries() - 1) {
      // TODO: Would like to one day just use the copy constructor for NALIST,
      // But we get unresolved references during release builds - more comments
      // in ExpPCodeOptsRuntime.cpp.  For now just copy in each item.

      requiredBlocks.clear();
      for (j=0; j < dupBlocks.entries(); j++)
        requiredBlocks.insert(dupBlocks[j]);

      i = -1;
    }
  }

  return dupFound;
}


#if 0
//
// Remove duplicate RETURN_IBIN32S blocks that get generated from short circuit
//
void PCodeCfg::removeDuplicateBlocks()
{
  CollIndex i;

  // Visit each block
  FOREACH_BLOCK(block, firstInst, lastInst, index) {

    // Find RETURN_IBIN32S block to serve as the single copy
    if (firstInst && (firstInst->getOpcode() == PCIT::RETURN_IBIN32S)) {
      NABoolean found = FALSE;
      for (i=0; !found && (i < block->getPreds().entries()); i++) {
        if (block->getPreds()[i]->getFallThroughBlock() == block)
          found = TRUE;
      }

      // Now search for a duplicate block of "block"
      FOREACH_BLOCK(block2, firstInst2, lastInst2, index2) {
        if (block == block2)
          continue;

        // A duplicate block will have the same argument for the RETURN
        if ((firstInst2 && (firstInst2->getOpcode() == PCIT::RETURN_IBIN32S)) &&

            (firstInst->code[1] == firstInst2->code[1]))
        {

          if (!found) {
            PCodeBlock* temp = block;
            block = block2;
            block2 = temp;
          }

          // With the duplicate found, attempt to re-route all predecessors of
          // block2 to point to "block" instead

          for (i=0; i < block2->getPreds().entries(); i++) {
            // You can't remove this block if another block falls through to it
            PCodeBlock* pred = block2->getPreds()[i];
            if (pred->getFallThroughBlock() == block2)
              continue;

            pred->removeEdge(block2);
            pred->addEdge(block);

            // Reduce i by 1 so that the index correctly points to next pred
            i--;
          }

          // Delete block if it's now empty
          if (block2->getPreds().entries() == 0) {
            deleteBlock(block2);

            // If the original block to serve as the target/mold was only
            // targetted too, then it just got deleted, so don't continue using
            // it as the mold
            if (!found)
              break;
          }
        }
      } ENDFE_BLOCK
    }

  } ENDFE_BLOCK
}
#endif

NABoolean PCodeBlock::doesBlockQualifyForShortCircuit() {
  if (last_ &&
      (last_->isNullBranch() || last_->isVarcharNullBranch() ||
      last_->isLogicalBranch()) &&
      (last_->getOpcode() != PCIT::NULL_BITMAP_BULK) &&
      (last_->getOpcode() != PCIT::NOT_NULL_BRANCH_BULK) &&
      (getSuccs()[0] != getSuccs()[1]))
    return TRUE;

  return FALSE;
}

NABoolean PCodeCfg::shortCircuitOpt(Int32 flags)
{
  NABoolean restart = FALSE;
  PCodeBlock* tBlk = NULL;
  NABoolean overlapFound = flags; // Only 1 bit flag passed in for now

  FOREACH_BLOCK_DFO(block, firstInst, lastInst, index)
  {
    if (block->doesBlockQualifyForShortCircuit())
    {
      NABitVector zeroes(heap_), ones(heap_), neg1(heap_);

      // First set up vectors to use what constants we already know
      zeroes += *zeroes_;
      ones += *ones_;
      neg1 += *neg1_;

      // Next run local constant prop on the source block so as to pull in
      // whatever known constants we can about this block.
      PCodeBlock* copyTgtBlock = copyCfg(block, block, NULL);
      copyTgtBlock->deleteInst(copyTgtBlock->getLastInst());
      localConstantPropagation(copyTgtBlock, zeroes, ones, neg1);
      deleteBlock(copyTgtBlock);

      tBlk = shortCircuitOptForBlock(block, TRUE, overlapFound,
                                     zeroes, ones, neg1);

      restart = (tBlk != NULL) || restart;

      tBlk = shortCircuitOptForBlock(block, FALSE, overlapFound,
                                     zeroes, ones, neg1);

      restart = (tBlk != NULL) || restart;
    }
  } ENDFE_BLOCK_DFO

  return restart;
}

//
// Perform copy propagation in straight-lined code embodied in the passed-in
// list of blocks.
//
void PCodeCfg::localCopyPropForPeeling(BLOCKLIST& blockList)
{
  CollIndex i;

  // Only process one block (i.e. "block") for now.
  Int32 maxLevel = COPY_PROP_MAX_RECURSION-1;

  for (i=0; i < blockList.entries(); i++) {
    PCodeBlock* block = blockList[i];
    FOREACH_INST_IN_BLOCK(block, inst) {
      if ((inst = constantFold(inst, FALSE)) == NULL)
        continue;

      // If this is a MOVE instruction involving a constant source operand, we
      // should try and forward propagate it.
      if (inst && inst->isMove() &&
          inst->getWOps()[0]->hasSameLength(inst->getROps()[0]) &&
          inst->getROps()[0]->isConst())
      {
        PCodeOperand* moveTgt = inst->getWOps()[0];
        PCodeOperand* moveSrc = inst->getROps()[0];

        PCodeInst* next = inst->next;
        PCodeBlock* startBlock = block;

        // If the next instruction is null, then go into the next block.
        if (!next) {
          if ((i+1) < blockList.entries()) {
            startBlock = blockList[i+1];
            next = startBlock->getFirstInst();
          }
          else
            continue;
        }

        // Call the copy prop functionality.
        copyPropForwardsHelper(startBlock, next, moveTgt, moveSrc, maxLevel);
      }
    } ENDFE_INST_IN_BLOCK
  }
}

//
// Perform short circuit optimizations for not null branches, making the 
// assmption that the branch isn't taken, and thereby propagating the known
// constant -1.
//
PCodeBlock* PCodeCfg::shortCircuitOptForBlock(PCodeBlock* block,
                                              NABoolean assumeTaken,
                                              NABoolean overlapFound,
                                              const NABitVector& zeroesConst,
                                              const NABitVector& onesConst,
                                              const NABitVector& neg1Const)
{
  CollIndex i;
  PCodeBlock *ftBlock, *tgtBlock, *newTgtBlock;

  NABitVector zeroes(heap_), ones(heap_), neg1(heap_);
  NABitVector savedZeroes(heap_), savedOnes(heap_), savedNeg1(heap_);
  NABitVector stopSavedZeroes(heap_), stopSavedOnes(heap_),stopSavedNeg1(heap_);

  NABoolean useStopSavedConstants = FALSE;

  BLOCKLIST blockList(heap_);

  PCodeInst* lastInst = block->getLastInst();

  Int32 numOfBranchesResolved = 0;

  PCodeBlock* stopTgtBlock = NULL;
  CollIndex stopCopyBlockIndex = -1;

  // If this block doesn't even qualify for short circuiting, return NULL
  if (!block->doesBlockQualifyForShortCircuit())
    return NULL;

  // If assumeTaken is TRUE, we do this only for logical branches
  if (!lastInst->isLogicalBranch() && assumeTaken)
    return NULL;

  // First set up vectors to use what constants we already know
  zeroes += zeroesConst;
  ones += onesConst;
  neg1 += neg1Const;

  if (lastInst->isLogicalBranch()) {
    if (assumeTaken)
      ftBlock = block->getTargetBlock();
    else
      ftBlock = block->getFallThroughBlock();

    block->fixupConstantVectors(ftBlock, zeroes, ones, neg1);
  }
  else
  {
    assert (assumeTaken == FALSE);
    setNeg1ConstantsForNullBranch(lastInst, zeroes, ones, neg1);
    ftBlock = block->getFallThroughBlock();

    // If the fall-through is an exit block such that no other block targets
    // it, then it is a waste of time to process this guy - no transformation
    // would occur.
    if (ftBlock->isExitBlock() && (ftBlock->getPreds().entries() == 1))
      return NULL;
  }

  tgtBlock = ftBlock;

  // Start with the fall through block of the null branch. If that block
  // ends in a branch, we want to see if we can determine the target of
  // that branch, while still propagating constants through that block.
  // If it doesn't end in a branch, it may still be possible to dig
  // deeper down.  Use local constant prop engine to dig down.

  while (tgtBlock->getLastInst())
  {
    PCodeInst* deletedBranchInst = NULL;

    // Make duplicate of target block for use by local constant prop
    // engine - it can make changes to the block's instrs, which we don't
    // want since we're taking a hypothetical approach first.
    PCodeBlock* copyTgtBlock = copyCfg(tgtBlock, tgtBlock, NULL);
    for (i=0; i < tgtBlock->getSuccs().entries(); i++)
      copyTgtBlock->addSucc(tgtBlock->getSuccs()[i]);

    // Get rid of branch inst at end of copy block since we are calling
    // local constant prop with the assumption that this block doesn't
    // have any succs.
    if (copyTgtBlock->getLastInst()->isBranch()) {
      deletedBranchInst = copyTgtBlock->getLastInst();
      copyTgtBlock->deleteInst(deletedBranchInst);
    }

    // If short circuit proves successful, we'll want to update the constant
    // vectors for the newly minted target block.  However, we can't use vectors
    // passed into localConstantPropagation because they will contain facts
    // about the last block - facts that won't necessarily be true for the
    // new target block.

    savedOnes = ones;
    savedZeroes = zeroes;
    savedNeg1 = neg1;

    //
    // Prior to performing local const propagation, apply a quick and dirty
    // copy prop step for a very common case.  Often times in case statements
    // you have 2 edges flowing into a block, where on both edges some operand
    // X has two different known values.  Constant prop will take care of the
    // case when the values are {0, 1, -1}, but what if it has a different
    // value?  For the time being, only apply if the following conditions are
    // met:
    //
    // 1. There exists a previous tgt block in the chain.
    // 2. The last inst in previous block is a move instruction.
    // 3. Move must involve a constant read operand.
    // 4. A successful copy prop can occur with it and an inst in tgt block.
    // 5. If reduced to a move, repeat step 3 starting at new location.
    // 6. Copy prop does not go beyond the existing block.
    //

    blockList.insert(copyTgtBlock);
    localCopyPropForPeeling(blockList);
    blockList.removeAt(blockList.entries()-1);

    // Perform local constant propagation on the target block
    localConstantPropagation(copyTgtBlock, zeroes, ones, neg1);

    // Add the branch inst back in, if it ever was there
    if (deletedBranchInst)
      copyTgtBlock->insertInstAfter(NULL, deletedBranchInst);

    // The exit block should always be considered when making super-blocks.
    // It may not, however, be included, if the superblock isn't long enough
    if (copyTgtBlock->isExitBlock()) {
      // We can force the entire chain to be included by resetting the stop
      // variables, but for now keep the stop pointers.
      //
      // LET'S TRY IT!
      blockList.insert(copyTgtBlock);
      numOfBranchesResolved++;
      stopTgtBlock = NULL;
      stopCopyBlockIndex = -1;

      // We have an exit block that can gain from being peeled off because
      // others go to it.  In addition, we should make sure that constant
      // prop did something to warrant this.
      if (tgtBlock->getPreds().entries() > 1)
        numOfBranchesResolved++;

      break;
    }

    // If we can determine the direction taken by the target block, get 
    // the target of the target block and save the target block in the
    // blocklist for later duplication.
    newTgtBlock = copyTgtBlock->determineTargetBlock(zeroes, ones, neg1);

    if (newTgtBlock) {
      PCodeInst* last = copyTgtBlock->getLastInst();

      // Lay out new blocks with the exit block duplicated - small optimization
      // meant primarily for better null-processing when nulls are predominant -
      // it gets rid of an extra branch.  And of course, lay out new blocks if
      // any branch (not unconditional branch since that's basically a
      // fall-through) is reduced.
      if (last && last->isBranch())
      {
        if (last->isUncBranch()) {
          numOfBranchesResolved++;

          // Don't normally do this, but try it.
          stopTgtBlock = NULL;
          stopCopyBlockIndex = -1;
        }
        else
        {
          numOfBranchesResolved += 2;
          stopTgtBlock = NULL;
          stopCopyBlockIndex = -1;
        }
      }

      if (last && last->isBranch())
      {
        PCodeInst* copy = copyTgtBlock->reduceBranch(newTgtBlock,
                                                     zeroes, ones, neg1);
        if (copy != NULL)
          copyTgtBlock->insertInstAfter(NULL, copy);

        copyTgtBlock->deleteInst(last);
      }
      else
      {
        stopTgtBlock = tgtBlock;
        stopCopyBlockIndex = blockList.entries();

        // Maintain constant vectors for stop target block.
        stopSavedZeroes = savedZeroes;
        stopSavedOnes = savedOnes;
        stopSavedNeg1 = savedNeg1;
      }

      blockList.insert(copyTgtBlock);
      tgtBlock = newTgtBlock;
    }
    else {
      copyTgtBlock->getSuccs().clear();
      deleteBlock(copyTgtBlock);
      break;
    }
  }

  if (stopTgtBlock == NULL) {
    stopTgtBlock = tgtBlock;
    stopCopyBlockIndex = blockList.entries();
  }
  else
    useStopSavedConstants = TRUE;

  // Create new super-block if we found a path deeper than the block's
  // current fall-through block.  Note, it's <= 2 because we want to ignore
  // the block's fall-through block and the target block, which are both
  // added to the list.
  if (numOfBranchesResolved <= 1) {
    // Do some cleanup
    for (i=0; i < blockList.entries(); i++) {
      blockList[i]->getSuccs().clear();
      deleteBlock(blockList[i]);
    }

    return NULL;
  }

  // First try some local dead-code-elimination to clean up blocks to copy.
  // This can only be done if the tgt block is an exit point.  NOTE, this DCE
  // algorithm only has 1 pass, which means that after deleting some write
  // instructions, some instructions which may use the write will not get
  // removed.

  if (!overlapFound && stopTgtBlock->getSuccs().isEmpty())
  {
    CollIndex i, j;
    NABitVector liveList(heap_);

    // Go through each block in reverse order
    for (i=0; i < stopCopyBlockIndex; i++) {
      PCodeBlock* block = blockList[stopCopyBlockIndex - 1 - i];

      // Walk block backwards and remove dead instructions
      FOREACH_INST_IN_BLOCK_BACKWARDS(block, inst) {
        // Assume we should remove inst (but only if it has a write operand)
        NABoolean remove = (inst->getWOps().entries() > 0);

        // If every write operand was not seen before subsequent write, remove
        for (j=0; remove && (j < inst->getWOps().entries()); j++) {
          PCodeOperand* op = inst->getWOps()[j];

          if (liveList.contains(op->getBvIndex()))
            remove = FALSE;

          if ((!op->isTemp() && !op->isGarbage()) || op->isEmptyVarchar())
            remove = FALSE;
        }

        if (remove && !inst->isClauseEval()) {
          removeOrRewriteDeadCodeInstruction(inst);
          continue;
        }

        // Remove writes to list
        for (j=0; j < inst->getWOps().entries(); j++)
          liveList -= inst->getWOps()[j]->getBvIndex();

        // Add reads from list
        for (j=0; j < inst->getROps().entries(); j++)
          liveList += inst->getROps()[j]->getBvIndex();

      } ENDFE_INST_IN_BLOCK_BACKWARDS
    }
  }

  PCodeBlock* newBlock = createBlock();

  // Loop through copy blocks and move instructions into new block
  for (i=0; i < stopCopyBlockIndex; i++)
  {
    PCodeBlock* copyBlock = blockList[i];
    FOREACH_INST_IN_BLOCK(copyBlock, copy) {
      newBlock->insertInstAfter(NULL, copy);
    } ENDFE_INST_IN_BLOCK
  }

  // Do some cleanup
  // never the copy.
  for (i=0; i < blockList.entries(); i++) {
    blockList[i]->getSuccs().clear();
    deleteBlock(blockList[i]);
  }
      
  if (!stopTgtBlock->getSuccs().isEmpty()) {
    // Add a branch inst at the end of the new block - it will go to the
    // final target block discovered by the algorithm.  Only do this,
    // however, if the tgtBlock is not an EXIT block
    newBlock->insertNewInstAfter(NULL, PCIT::BRANCH);
    newBlock->addEdge(stopTgtBlock);
  }

  // Fix up edges.
  block->removeEdge(ftBlock);
  if (assumeTaken) {
    block->addEdge(newBlock);
  }
  else {
    block->addFallThroughEdge(newBlock);
  }

  // Set the constant vectors for this new block.  Doing so will speed up
  // global constant propagation, and all the algorithm to terminate after 1
  // loop.

  if (useStopSavedConstants) {
    newBlock->zeroesVector = stopSavedZeroes;
    newBlock->onesVector = stopSavedOnes;
    newBlock->neg1Vector = stopSavedNeg1;
  }
  else {
    newBlock->zeroesVector = savedZeroes;
    newBlock->onesVector = savedOnes;
    newBlock->neg1Vector = savedNeg1;
  }

  return newBlock;
}

//
// Knowing that this block branches to the provided "to" block, we may be able
// to reduce the branch into some other inst.
//
PCodeInst*
PCodeBlock::reduceBranch(PCodeBlock* to,
                         NABitVector& zeroes,
                         NABitVector& ones,
                         NABitVector& neg1)
{
  PCodeInst* newInst = NULL;
  PCodeOperand *write, *read;
  Int32 constant = 0;

  PCodeInst* inst = getLastInst();
  Int32 opc = inst->getOpcode();

  switch (opc)
  {
    case PCIT::NNB_MATTR3_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN16S_IBIN32S:
    case PCIT::BRANCH:
      return NULL;
      break;

    case PCIT::NNB_SPECIAL_NULLS_MBIN32S_MATTR3_MATTR3_IBIN32S_IBIN32S:
    {
      constant = (to == getTargetBlock()) ? 0 : 1;

      CollIndex bvIndex1 = inst->getROps()[0]->getBvIndex();
      CollIndex bvIndex2 = inst->getROps()[1]->getBvIndex();

      Int32 c1 = PCodeConstants::getConstantValue(bvIndex1, zeroes, ones, neg1);
      Int32 c2 = PCodeConstants::getConstantValue(bvIndex2, zeroes, ones, neg1);

      if (constant == 1) {
        write = inst->getWOps()[0];

        PCodeOperand* op = (c1 == -1) ? inst->getROps()[1] : inst->getROps()[0];

        // TODO: If we could just tell this routine which operand was known to
        // be negative at the time this decision was made, we could use a
        // NULL_TEST instead.  But we could change this in constant prop.

        // Nulls must not be from aligned format - determineTargetBlock should
        // guard against this, since there is no EQ for bits in bitmap.  TODO:
        // add an EQ_MATTR3_MATTR3 to check if two null bits are the same.

        newInst =
          cfg_->createInst(PCIT::NULL_TEST_MBIN32S_MATTR5_IBIN32S_IBIN32S);

        newInst->code[1] = write->getStackIndex();
        newInst->code[2] = write->getOffset();
        newInst->code[3] = op->getStackIndex();
        newInst->code[4] = op->getOffset();
        newInst->code[5] = -1; // voa that's unnecessary.
        newInst->code[6] = (op->forAlignedFormat())
                             ? ExpTupleDesc::SQLMX_ALIGNED_FORMAT
                             : ExpTupleDesc::SQLARK_EXPLODED_FORMAT;
        newInst->code[7] = op->getNullBitIndex();
        newInst->code[8] = 0;  // No voa lookup needed.
        newInst->code[9] = -1; // Test for NULL

#if 0
        newInst = cfg_->createInst(PCIT::EQ_MBIN32S_MBIN16S_MBIN16S);
        newInst->code[1] = write->getStackIndex();
        newInst->code[2] = write->getOffset();
        newInst->code[3] = inst->getROps()[0]->getStackIndex();
        newInst->code[4] = inst->getROps()[0]->getOffset();
        newInst->code[5] = inst->getROps()[1]->getStackIndex();
        newInst->code[6] = inst->getROps()[1]->getOffset();
#endif

        cfg_->loadOperandsOfInst(newInst);

        return newInst;
      }

      newInst = cfg_->createInst(PCIT::MOVE_MBIN32S_IBIN32S);

      break;
    }

    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_IATTR3_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
      // TODO: Can't handle indirect varchars just yet
      assert(inst->getWOps()[0]->getOffset() >= 0);

      // Fall-through to handler below

      constant = (getTargetBlock() == to) ? 0 : -1;
      write = inst->getWOps()[0];

      if (write->forAlignedFormat()) {
        newInst = cfg_->createInst(PCIT::NULL_BITMAP);
        newInst->code[1] = write->getStackIndex();
        newInst->code[2] = write->getOffset();
        newInst->code[3] = PCIT::NULL_BITMAP_SET;
        newInst->code[4] = write->getNullBitIndex();
        newInst->code[5] = constant;

        cfg_->loadOperandsOfInst(newInst);

        return newInst;
      }
      else
        newInst = cfg_->createInst(PCIT::MOVE_MBIN16U_IBIN16U);

      break;

    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S_IBIN32S:
    case PCIT::NNB_MBIN32S_MATTR3_IBIN32S_IBIN32S:
      if (getTargetBlock() == to)
        return NULL;

      constant = (opc == PCIT::NNB_MBIN32S_MATTR3_IBIN32S_IBIN32S)
                   ? inst->code[6] : inst->code[5];
      newInst = cfg_->createInst(PCIT::MOVE_MBIN32S_IBIN32S);
      break;

    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_MBIN16S_IBIN32S:
      constant = (getTargetBlock() == to) ? 0 : -1;
      newInst = cfg_->createInst(PCIT::MOVE_MBIN32S_IBIN32S);
      break;

    case PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_MBIN16S_IBIN32S:
      constant = (getTargetBlock() == to) ? 0 : -1;
      newInst = cfg_->createInst(PCIT::MOVE_MBIN16U_IBIN16U);
      break;

    case PCIT::BRANCH_OR:
    case PCIT::BRANCH_AND:
      read = inst->getROps()[0];
      write = inst->getWOps()[0];

      newInst = cfg_->createInst(PCIT::MOVE_MBIN32U_MBIN32U);

      newInst->code[1] = write->getStackIndex();
      newInst->code[2] = write->getOffset();
      newInst->code[3] = read->getStackIndex();
      newInst->code[4] = read->getOffset();

      cfg_->loadOperandsOfInst(newInst);

      return newInst;
      break;
  }

  write = inst->getWOps()[0];

  newInst->code[1] = write->getStackIndex();
  newInst->code[2] = write->getOffset();
  newInst->code[3] = constant;

  cfg_->loadOperandsOfInst(newInst);

  return newInst;
}

//
// Given a set of constant vectors, see if we can determine what direction the
// branch for this block takes. Return the identified target block.
//
PCodeBlock*
PCodeBlock::determineTargetBlock(NABitVector& zeroes,
                                 NABitVector& ones,
                                 NABitVector& neg1)
{
  CollIndex bvIndex1, bvIndex2;


  PCodeInst* inst = getLastInst();
  Int32 const1, const2;

  // If the block doesn't have a branch, or the block is empty, then the
  // default target is the fall-through block
  if (!inst || (!inst->isBranch() && getSuccs().entries()))
    return getFallThroughBlock();

  Int32 opc = inst->getOpcode();

  switch(opc)
  {
    case PCIT::BRANCH:
      return getTargetBlock();
      break;

    case PCIT::NNB_SPECIAL_NULLS_MBIN32S_MATTR3_MATTR3_IBIN32S_IBIN32S:
      bvIndex1 = inst->getROps()[0]->getBvIndex();
      bvIndex2 = inst->getROps()[1]->getBvIndex();

      const1 = PCodeConstants::getConstantValue(bvIndex1, zeroes, ones, neg1);
      const2 = PCodeConstants::getConstantValue(bvIndex2, zeroes, ones, neg1);

      if ((const1 == 0) && (const2 == 0)) {
        PCodeConstants::setConstantInVectors(0,
          inst->getWOps()[0]->getBvIndex(), zeroes, ones, neg1);
        return getTargetBlock();
      }
      else if ((const1 == -1) || (const2 == -1)) {
        // If both are -1, then you know the result.  Otherwise, all you know
        // is that we fall through
        if (const1 == const2) {
          PCodeConstants::setConstantInVectors(1,
            inst->getWOps()[0]->getBvIndex(), zeroes, ones, neg1);
        }
        return getFallThroughBlock();
      }
      break;


    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_IATTR3_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S:
      // TODO: no support for indirect varchars.  Run sanity check before
      // we fall-through.
      if ((inst->getROps()[0]->getOffset() < 0) ||
          (inst->getWOps()[0]->getOffset() < 0))
        break;

    case PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S_IBIN32S:
    case PCIT::NNB_MBIN32S_MATTR3_IBIN32S_IBIN32S:
    case PCIT::BRANCH_AND:
      bvIndex1 = inst->getROps()[0]->getBvIndex();
      bvIndex2 = inst->getWOps()[0]->getBvIndex();

      if (PCodeConstants::isAnyKnownConstant(bvIndex1, zeroes, ones, neg1))
      {
        const1 = PCodeConstants::getConstantValue(bvIndex1, zeroes, ones, neg1);
        NABoolean canBeOneOrNeg1 = !PCodeConstants::canBeZero(bvIndex1, zeroes);

        if ((const1 == -1) || (const1 == 1) || (canBeOneOrNeg1))
        {
          if (opc == PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S_IBIN32S) {
            PCodeConstants::setConstantInVectors(inst->code[5], bvIndex2,
                                                 zeroes, ones, neg1);
          }
          else if (opc == PCIT::NNB_MBIN32S_MATTR3_IBIN32S_IBIN32S) {
            PCodeConstants::setConstantInVectors(inst->code[6], bvIndex2,
                                                 zeroes, ones, neg1);
          }
          else {
            PCodeConstants::clearConstantVectors(bvIndex2, zeroes, ones, neg1);
            PCodeConstants::copyConstantVectors(bvIndex1, bvIndex2,
                                                zeroes, ones, neg1);
          }

          return getFallThroughBlock();
        }
        else if (const1 == 0) {
          if ((opc != PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S_IBIN32S) &&
              (opc != PCIT::NNB_MBIN32S_MATTR3_IBIN32S_IBIN32S))
            PCodeConstants::setConstantInVectors(0, bvIndex2,
                                                 zeroes, ones, neg1);

          return getTargetBlock();
        }
      }

      break;

    case PCIT::BRANCH_OR:
      bvIndex1 = inst->getROps()[0]->getBvIndex();
      bvIndex2 = inst->getWOps()[0]->getBvIndex();

      if (PCodeConstants::isAnyKnownConstant(bvIndex1, zeroes, ones, neg1))
      {
        const1 = PCodeConstants::getConstantValue(bvIndex1, zeroes, ones, neg1);
        NABoolean canBeZeroOrNeg1 = !PCodeConstants::canBeOne(bvIndex1, ones);

        if ((const1 == -1) || (const1 == 0) || (canBeZeroOrNeg1))
        {
          PCodeConstants::clearConstantVectors(bvIndex2, zeroes, ones, neg1);
          PCodeConstants::copyConstantVectors(bvIndex1, bvIndex2,
                                              zeroes, ones, neg1);

          return getFallThroughBlock();
        }
        else if (const1 == 1) {
          PCodeConstants::setConstantInVectors(const1, bvIndex2,
                                               zeroes, ones, neg1);
          return getTargetBlock();
        }
      }

      break;

    case PCIT::NNB_MATTR3_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN16S_IBIN32S:
      bvIndex1 = inst->getROps()[0]->getBvIndex();
      const1 = PCodeConstants::getConstantValue(bvIndex1, zeroes, ones, neg1);

      if (const1 == 0)
        return getTargetBlock();
      else if (const1 == -1)
        return getFallThroughBlock();

      break;

    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
      // TODO: no support for indirect varchars.  Run sanity check before
      // we fall-through.
      if ((inst->getROps()[0]->getOffset() < 0) ||
          (inst->getROps()[1]->getOffset() < 0) ||
          (inst->getWOps()[0]->getOffset() < 0))
        break;

    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_MBIN16S_IBIN32S:
      bvIndex1 = inst->getROps()[0]->getBvIndex();
      bvIndex2 = inst->getROps()[1]->getBvIndex();

      const1 = PCodeConstants::getConstantValue(bvIndex1, zeroes, ones, neg1);
      const2 = PCodeConstants::getConstantValue(bvIndex2, zeroes, ones, neg1);

      if ((const1 == 0) && (const2 == 0)) {
        PCodeConstants::setConstantInVectors(0, 
          inst->getWOps()[0]->getBvIndex(), zeroes, ones, neg1);
        return getTargetBlock();
      }
      else if ((const1 == -1) || (const2 == -1)) {
        PCodeConstants::setConstantInVectors(-1, 
          inst->getWOps()[0]->getBvIndex(), zeroes, ones, neg1);

        return getFallThroughBlock();
      }

      break;
  }

  return NULL;
}

//
// Make the assumption that the provided instruction (a null branch) is not
// taken, and set the known constants for the operands involved in the inst.
//
static void setNeg1ConstantsForNullBranch(PCodeInst* inst,
                                          NABitVector& zeroes,
                                          NABitVector& ones,
                                          NABitVector& neg1)
{
  CollIndex bvIndex1, bvIndex2;
  Int32 opc = inst->getOpcode();

  switch(opc) {
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_IATTR3_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S:
      // TODO: nullable indirect varchars not supported just yet as tgts.
      if (inst->getWOps()[0]->getOffset() < 0)
        break;

      // If read is a varchar also, then just mark the write operand.  Otherwise
      // fall through to the next case to mark both operands as -1.
      if (inst->getROps()[0]->getOffset() < 0) {
        bvIndex1 = inst->getWOps()[0]->getBvIndex();
        PCodeConstants::setConstantInVectors(-1, bvIndex1, zeroes, ones, neg1);
        break;
      }

    case PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S_IBIN32S:
    case PCIT::NNB_MBIN32S_MATTR3_IBIN32S_IBIN32S:
      bvIndex1 = inst->getROps()[0]->getBvIndex();
      bvIndex2 = inst->getWOps()[0]->getBvIndex();

      PCodeConstants::setConstantInVectors(-1, bvIndex1, zeroes, ones, neg1);

      if ((opc != PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S_IBIN32S) &&
          (opc != PCIT::NNB_MBIN32S_MATTR3_IBIN32S_IBIN32S))
        PCodeConstants::setConstantInVectors(-1, bvIndex2, zeroes, ones, neg1);

      break;

    case PCIT::NNB_MATTR3_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN16S_IBIN32S:
      bvIndex1 = inst->getROps()[0]->getBvIndex();
      PCodeConstants::setConstantInVectors(-1, bvIndex1, zeroes, ones, neg1);
      break;

    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
      // TODO: nullable indirect varchars not supported just yet as tgts.
      // Fall-through otherwise.
      if (inst->getWOps()[0]->getOffset() < 0)
        break;

    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_MBIN16S_IBIN32S:
      bvIndex1 = inst->getWOps()[0]->getBvIndex();
      PCodeConstants::setConstantInVectors(-1, bvIndex1, zeroes, ones, neg1);
      break;

    case PCIT::NNB_SPECIAL_NULLS_MBIN32S_MATTR3_MATTR3_IBIN32S_IBIN32S:
      break;

    default:
      assert(FALSE);
      break;
  }
}
