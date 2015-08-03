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

#define HASH_ALL TRUE
#define REORDER_HASH TRUE

// 
// This is a recursive function that attempts to find a hash inst whose write
// operand is equivalent to the provided def operand.  Begin the search at the
// provided start inst. 
//
PCodeInst* PCodeCfg::findHashInst(PCodeOperand* def, PCodeInst* start)
{
  CollIndex defIndex = def->getBvIndex();
  PCodeBlock* block;

  if (start == NULL)
    return NULL;

  block = start->block;

  // Walk each inst backwards from start in search for an inst that defines
  // "def".  If that inst is not a hash or a bulk hash combined inst, then
  // return NULL.

  FOREACH_INST_IN_BLOCK_BACKWARDS_AT(block, inst, start) {
    for (CollIndex i=0; i < inst->getWOps().entries(); i++) {
      if (defIndex == inst->getWOps()[i]->getBvIndex()) {
        if (inst->isHash() ||
            inst->getOpcode() == PCIT::HASHCOMB_BULK_MBIN32U)
          return inst;

        return NULL;
      }
    }
  } ENDFE_INST_IN_BLOCK_BACKWARDS_AT

  // At this point, we could not find the instruction which defined def.  Now
  // if the definition reaches this block, then all paths to this block must
  // have the definition of this def operand.  So, only check the first pred
  // via a recursive call. 

  if (block->reachingDefsTab_->find(defIndex, TRUE /* In */))
    if (block->getPreds().entries() > 0)
      return findHashInst(def, block->getPreds()[0]->getLastInst());

  // Definition could not be found - return NULL.
  return NULL;
}

//
// Create bulk hash combined instructions from multiple hash and hashcomb
// instructions.
//
void PCodeCfg::bulkHashComb()
{
  CollIndex i, j, length, idx;

  FOREACH_BLOCK(block, firstInst, lastInst, index) {
    FOREACH_INST_IN_BLOCK(block, inst) {
      if (inst->getOpcode() == PCIT::HASHCOMB_MBIN32U_MBIN32U_MBIN32U)
      {
        PCodeInst* newInst;

        // Find the hash instruction which defines the source operands of this
        // hashcomb instruction.
        PCodeInst* src1 = findHashInst(inst->getROps()[0], inst);
        PCodeInst* src2 = findHashInst(inst->getROps()[1], inst);

        // Unless HASH_ALL is set, continue if either src1 or src2 is NULL.
        // HASH_ALL is used when the definition found does not need to be
        // hashed again - this is often the case when no hash instruction
        // exists (e.g., OPDATA/CLAUSE_EVAL is used).

        if (!HASH_ALL && (!src1 || !src2))
          continue;

        NABoolean topOpOnlySrc1 = FALSE, topOpOnlySrc2 = FALSE;
        CollIndex max = MAX_HASHCOMB_BULK_OPERANDS;

        // Set length taken by operands (index,offset,size,pos = 4 ints / oper)
        // If src1 is null (i.e. no proper HASH found) or the number of ops
        // is greater than the max allowed, just use the final result for the
        // new HASHCOMB_BULK instruction.  Otherwise read in all operands.

        if (src1 && (src1->getROps().entries() < max))
          length = src1->getROps().entries();
        else {
          topOpOnlySrc1 = TRUE;
          length = 1;
        }

        if (src2 && ((length+src2->getROps().entries()) <= max))
          length += src2->getROps().entries();
        else {
          topOpOnlySrc2 = TRUE;
          length += 1;
        }

        // Add length of write operand, length field, and opcode itself
        length = 4*(length+1);

        // Set up pcode byte stream with what we know
        newInst = createInst(PCIT::HASHCOMB_BULK_MBIN32U, length);
        newInst->code[1] = length;
        newInst->code[2] = inst->code[1];
        newInst->code[3] = inst->code[2];

        // Create one list with all the source operands
        OPLIST list;

        // If the source inst found is not null (i.e. HASH or HASHCOMB_BULK)
        // then include its operands.  Otherwise, we can't hash the source, but
        // we can still combine it, while hashing everything else.  This
        // requires a small hack of resetting the size of the operand to 0.
        // A zero length tells the hashing algoritm to return the value as is,
        // unhashed.

        if (!topOpOnlySrc1)
          list.insert(src1->getROps());
        else {
          inst->getROps()[0]->setLen(0);
          list.insert(inst->getROps()[0]);
        }

        if (!topOpOnlySrc2)
          list.insert(src2->getROps());
        else {
          inst->getROps()[1]->setLen(0);
          list.insert(inst->getROps()[1]);
        }

        for (i=0; i < list.entries(); i++) {
          newInst->code[(4*i) + 4] = list[i]->getStackIndex();
          newInst->code[(4*i) + 5] = list[i]->getOffset();
          newInst->code[(4*i) + 6] = list[i]->getLen();
          newInst->code[(4*i) + 7] = i;
        }

        loadOperandsOfInst(newInst);

        block->insertInstAfter(inst, newInst);
        block->deleteInst(inst);
      }
    } ENDFE_INST_IN_BLOCK
  } ENDFE_BLOCK


  // Now reorder operands in hash for better parallel hashing.
  if (!REORDER_HASH)
    return;

  FOREACH_BLOCK(block, firstInst, lastInst, index) {
    FOREACH_INST_IN_BLOCK(block, inst) {
      if (inst->getOpcode() == PCIT::HASHCOMB_BULK_MBIN32U) {

        // Create one list with all the source operands
        OPLIST list;

        list.insert(inst->getROps());

        // Go through the list of operands and find pairs with minimal
        // differences in lengths.  These pairs are more suitable for parallel
        // hashing.  Unfortunately, however, because the need to be combined in
        // the same order as was generated, we need to also keep track of each
        // operands position (a.k.a combined order index).

        for (idx = 0; idx != list.entries();)
        {
          CollIndex op1=0, op2=0; // Operand positions of ideal hash pairs
          UInt32 len1, len2;      // Lengths of operands 1 and 2
          UInt32 min = UINT_MAX;  // Init min diff between operand lengths

          // Go through each operand in list
          for (i=0; i < list.entries(); i++) {

            // If operand was already inserted, continue to next operand
            if (list[i] == NULL) continue;

            // Initialize op1 and op2 if this is the first legit operand seen
            if (min == UINT_MAX) {
              op1 = op2 = i;
            }

            // Length of operand 1
            len1 = (list[i]->getLen() == 0) ? -1 : list[i]->getLen();

            // Now go through the operand list again, but this time starting
            // with the operand after op1.  The difference in lengths will be
            // calculated here, and then compared to the least difference found
            // so far.

            for (j=i+1; j < list.entries(); j++) {

              // If operand was already inserted, continue to next operand
              if (list[j] == NULL) continue; 

              // Length of operand 2
              len2 = (list[j]->getLen() == 0) ? -1 : list[j]->getLen();

              // Difference calculated must be absolute for comparison purposes
              UInt32 diff = (len1 > len2) ? len1 - len2 : len2 - len1;

              // If diff is lowest found so far, we found a new hash candidate
              if (diff < min) {
                min = diff;

                // Always make op1 the smaller of the two, since parallel hash
                // requires this order
                op1 = (len1 > len2) ? j : i;
                op2 = (len1 > len2) ? i : j;
              }
            }
          }

          // At this point, it is only possible that we either found a pair or
          // we have but one operand left.  In either case, we're inserting at
          // least one operand.  Note that we move the index up in the last
          // store

          inst->code[(4*idx) + 4] = list[op1]->getStackIndex();
          inst->code[(4*idx) + 5] = list[op1]->getOffset();
          inst->code[(4*idx) + 6] = list[op1]->getLen();
          inst->code[(4*idx++) + 7] = op1;

          // Since removing entry out of list will mess up the position info of
          // each operand, replace with NULL, so that entry is side-stepped in
          // algorithm

          list.removeAt(op1);
          list.insertAt(op1, NULL);

          // If not a pair, then break out
          if (op1 == op2)
            break;

          // Add op2 and then effectively remove it from list

          inst->code[(4*idx) + 4] = list[op2]->getStackIndex();
          inst->code[(4*idx) + 5] = list[op2]->getOffset();
          inst->code[(4*idx) + 6] = list[op2]->getLen();
          inst->code[(4*idx++) + 7] = op2;

          list.removeAt(op2);
          list.insertAt(op2, NULL);
        }

        inst->reloadOperands(this);
      }
    } ENDFE_INST_IN_BLOCK
  } ENDFE_BLOCK
}

//
// Performs code motion and instruction grouping across all blocks
//
void PCodeCfg::codeMotion()
{
  FOREACH_BLOCK(block, firstInst, lastInst, index) {
    localCodeMotion(block);
  } ENDFE_BLOCK
}

//
// This function has two purposes.  The first is to group like instructions
// together so that 1) they can be later combined into "bulk" forms and 2) to
// remove any unnecessary dependencies on execution order.  The second
// purpose of this function is to find and generate genuine bulk moves where
// moves of two adjacent columns can be done in one shot.  This comes as an
// artifact from the NOT_NULL_BRANCH_BULK instruction
//
void PCodeCfg::localCodeMotion(PCodeBlock* block)
{
  FOREACH_INST_IN_BLOCK(block, inst) {
    // Group certain instructions as close together as possible
    if ((inst->getOpcode() == PCIT::RANGE_MFLT64) ||
        (inst->isNullBitmap()) ||
        (inst->isCopyMove() || inst->isConstMove()))
    {
      PCodeInst* insertInst = NULL;

      // Given inst, walk backwards from inst and try to find a spot to move
      // inst into, taking consideration of waw/raw/war dependency violations
      FOREACH_INST_IN_BLOCK_BACKWARDS_AT(block, prev, inst->prev) {

        CollIndex i, j;
        NABoolean found = FALSE;
        PCodeInst* nInst;

        // Moves are grouped in a more intricate way.  If both operands are
        // same-size moves, then we attempt to order them relative to each
        // other in a way which would expose adjacent/bulk moves.
        if (inst->isCopyMove() && prev->isCopyMove()) {
          Int32 pWOff = prev->getWOps()[0]->getOffset();
          Int32 pROff = prev->getROps()[0]->getOffset();
          Int32 iWOff = inst->getWOps()[0]->getOffset();
          Int32 iROff = inst->getROps()[0]->getOffset();

          // First group by writes 
          if ((inst->getWOps()[0]->getStackIndex() ==
              prev->getWOps()[0]->getStackIndex()) &&
              (inst->getWOps()[0]->getOffset() >=
              prev->getWOps()[0]->getOffset())) {

            // Next group by reads
            if ((inst->getROps()[0]->getStackIndex() ==
                prev->getROps()[0]->getStackIndex()) &&
                (inst->getROps()[0]->getOffset() >=
                prev->getROps()[0]->getOffset())) {

              // Moves are now in order.  Check to see if any overlap exists.
              // Note, overlapping moves should not involve the same tuple for
              // both source and target, *unless* we're dealing with different
              // ranges.  Since that's unlikely, just check for same tuple.
              if ((prev->getWOps()[0]->getStackIndex() !=
                   prev->getROps()[0]->getStackIndex()) &&
                  ((pWOff + prev->getWOps()[0]->getLen()) == iWOff) &&
                  ((pROff + prev->getROps()[0]->getLen()) == iROff)) {

                Int32 len = prev->getROps()[0]->getLen() + 
                            inst->getROps()[0]->getLen();

                PCIType::Instruction opc;

                switch (len) {
                  case 2:
                    opc = PCIT::MOVE_MBIN16U_MBIN16U;
                    break;
                  case 4:
                    opc = PCIT::MOVE_MBIN32U_MBIN32U;
                    break;
                  case 8:
                    opc = PCIT::MOVE_MBIN64S_MBIN64S;
                    break;
                  default:
                    opc = PCIT::MOVE_MBIN8_MBIN8_IBIN32S;
                    break;
                }

                // Create a move by combining the two moves together
                nInst = block->insertNewInstBefore(prev, opc);

                nInst->code[1] = prev->getWOps()[0]->getStackIndex();
                nInst->code[2] = pWOff;
                nInst->code[3] = prev->getROps()[0]->getStackIndex();
                nInst->code[4] = pROff;

                if (opc == PCIT::MOVE_MBIN8_MBIN8_IBIN32S)
                  nInst->code[5] = len;

                loadOperandsOfInst(nInst);

                block->deleteInst(inst);
                block->deleteInst(prev);

                // We're no longer trying to insert the inst
                insertInst = NULL;
              }
              break;
            }
          }
        }
        else {
          // Check moves first
          if (inst->isConstMove() &&
              (prev->isConstMove() || prev->isCopyMove()) &&
              (inst->getWOps()[0]->getStackIndex() ==
               prev->getWOps()[0]->getStackIndex()))
            break;

          // Check all other instructions next
          if (prev->getOpcode() == inst->getOpcode())
            // If prev is the same type of instr as inst, then already grouped.
            break;
        }

        // First check for write-after-write hazards
        // I.e. inst writes operand that prev writes to.

        for (i=0; !found && (i < prev->getWOps().entries()); i++)
          for (j=0; !found && (j < inst->getWOps().entries()); j++)
            if (inst->getWOps()[j]->canOverlap(prev->getWOps()[i]))
              found = TRUE;

        // Now check for read-after-write hazards
        // I.e. inst reads operand that prev writes to.

        for (i=0; !found && (i < prev->getWOps().entries()); i++)
          for (j=0; !found && (j < inst->getROps().entries()); j++)
            if (inst->getROps()[j]->canOverlap(prev->getWOps()[i]))
              found = TRUE;

        // Lastly, check for write-after-read hazards
        // I.e. prev reads operand that inst writes to.

        for (i=0; !found && (i < prev->getROps().entries()); i++)
          for (j=0; !found && (j < inst->getWOps().entries()); j++)
            if (inst->getWOps()[j]->canOverlap(prev->getROps()[i]))
              found = TRUE;

        // If found is true, or then we can't perform the code motion.
        if (found)
          break;

        // It is safe to move the instruction before prev.
        insertInst = prev;

      } ENDFE_INST_IN_BLOCK_BACKWARDS_AT

      // Move the instruction before insertInst
      if (insertInst != NULL) {

        // Don't move inst up if no like instruction was found, and is not a
        // copy move
        if ((insertInst->prev == NULL) && (!inst->isCopyMove()))
          continue;

        block->deleteInst(inst);

        // Careful inserting since code motion must be done correctly when
        // thinking about inserting in between opdata/clause-eval pairs
        if (insertInst->prev)
          block->insertInstAfter(insertInst->prev, inst);
        else
          block->insertInstBefore(insertInst, inst);
      }
    }
  } ENDFE_INST_IN_BLOCK
}

NABoolean PCodeCfg::removeBulkNullBranch()
{
  Int32 numOfNB = 0;

  if (!entryBlock_->getLastInst()->isBulkNullBranch())
    return FALSE;

  PCodeBlock* start = entryBlock_->getFallThroughBlock();

  FOREACH_BLOCK_REV_DFO_AT(start, block, firstInst, lastInst, index) {
    if (lastInst && lastInst->isNullBranch()) {
      // Don't support bulk null checking for indirect varchars
      if (lastInst->isVarcharNullBranch() &&
          lastInst->containsIndirectVarcharOperand())
        continue;

      numOfNB++;
    }
  } ENDFE_BLOCK_REV_DFO

  if (numOfNB < 2) {
    deleteBlock(entryBlock_);
    entryBlock_ = start;

    return TRUE;
  }

  return FALSE;
}

NABoolean PCodeCfg::bulkNullBranch()
{
  CollIndex i;
  PCodeBlock* newBlock;
  PCodeBlock* copyBlocks;
  PCodeInst* newInst;
  Int32 length;
  OPLIST list(heap_);
  PCodeBlock** map;
  NABitVector defs(heap_);

  NABoolean alignedFormat = FALSE;
  NABoolean packedFormat = FALSE;
  NABoolean differentRowsFound = FALSE;
  Int32 bitmapIdx = 0;
  Int32 bitmapOff = 0;
  Int32 bitmapMaxNullBitIndex = -1;

  Int32 numOfNB = 0;                     // Number of null branch insts in cfg
  Int32 numOfProcessedNB = 0;            // Num of nb insts we can eval early
  PCodeBlock* startBlock = entryBlock_;

  FOREACH_BLOCK_REV_DFO(block, firstInst, lastInst, index) {

    // Add all defs - the purpose of which is described later.
    FOREACH_INST_IN_BLOCK(block, inst) {
      for (i=0; i < inst->getWOps().entries(); i++)
        defs += inst->getWOps()[i]->getBvIndex();
    } ENDFE_INST_IN_BLOCK

    if (lastInst && lastInst->isNullBranch()) {
      NABoolean checked = FALSE;

      // Don't support bulk null checking for indirect varchars
      if (lastInst->isVarcharNullBranch() &&
          lastInst->containsIndirectVarcharOperand())
        continue;

      // Found a null branch
      numOfNB++;

      for (i=0; i < lastInst->getROps().entries(); i++) {
        PCodeOperand* read = lastInst->getROps()[i];
        CollIndex rd = read->getBvIndex();

        // Only variable read operands should be considered, since those are the
        // only ones that can be defined upon entry.  If this optimization were
        // to ever change such that the bulk null branch instruction could be
        // placed somewhere other than the entry block, revisit this.
        if (!read->isVar())
          continue;

#if 0
        // Old way of doing this using reaching defs

        PCodeInst* defI = startBlock->reachingDefsTab_->find(rd, TRUE /* In */);
        PCodeInst* defO = block->reachingDefsTab_->find(rd);

        // If the read operand of the null branch has a known def that reaches
        // it from the start block, and that def is not overwritten in the
        // block containing the null branch, then it serves as a candidate for
        // code motion for early null detection.  Also, don't add the operand
        // if it already exists in the list

        if ((defI != NULL) && (defI == defO))
#else
        // Instead of using costly reaching defs analysis, use the "defs" bit
        // vector that has been computed up to this point.  The idea being that
        // if this read operand has been written to at any point in this expr,
        // than it doesn't qualify for this opt.  In almost every situation, a
        // use of var operand in a null-branch check is done without a previous
        // write to it in the same expr.

        if (!defs.contains(rd))
#endif
        {
          CollIndex j;

          // Set checked flag indicating that this is a NULL branch to process
          checked = TRUE;

          // Don't add operand if it already exists in list
          for (j=0; j < list.entries(); j++)
            if (rd == list[j]->getBvIndex())
              break;

          // If operand does not already exist in the list, add it.
          if (j == list.entries())
            list.insert(read);
        }
      }

      if (checked)
        numOfProcessedNB++;

      // If there are too many null branches, return.
      if (numOfNB > 1000)
        return FALSE;
    }
  } ENDFE_BLOCK_REV_DFO

  // Heuristic - don't do optimization if less than two null branches were found
  // or if more than 50 unique nulls must be examined.  Why the latter?  Because
  // not all 50 nullable columns will likely be NULL for a given row.
  if ((numOfProcessedNB < 2) || (list.entries() > 50))
    return FALSE;

  // Mark all blocks as unvisited - used in copyCfg()
  clearVisitedFlags();

  // Create a map table used to map copy blocks to original blocks.  Make a copy  // of the cfg starting from startBlock.

  map = (PCodeBlock**) new(heap_) PCodeBlock*[maxBlockNum_ + 1];
  copyBlocks = copyCfg(startBlock, NULL, map);
  NADELETEBASIC(map, heap_);

  // Create new entry block for not-null-branch-bulk
  newBlock = createBlock();

  // Analyze the not-nulls collected to see if and how we should process
  for (i=0; i < list.entries(); i++) {
    Int32 nullBitIndex = list[i]->getNullBitIndex();
    if (nullBitIndex == -1)
      packedFormat = TRUE;
    else
      alignedFormat = TRUE;

    bitmapMaxNullBitIndex = ((nullBitIndex > bitmapMaxNullBitIndex)
                              ? nullBitIndex : bitmapMaxNullBitIndex);

    if (i == 0) {
      bitmapIdx = list[i]->getStackIndex();
      bitmapOff = list[i]->getOffset();
      bitmapMaxNullBitIndex = nullBitIndex;
    }

    // A different row representing different null indicators for either
    // formats must show a different stack index - i.e. no need to compare
    // offsets as well for aligned format bitmaps.
    if (bitmapIdx != list[i]->getStackIndex())
      differentRowsFound = TRUE;
  }

  // As a potential pcode bytecode bloat problem, we want to make sure that if
  // we're dealing with the alignedFormat, then expect to see at least 1 bit
  // check per byte in the bitmask (i.e. equivalent to a non-parallel check)
  if (alignedFormat && !packedFormat) {
    float real = ((float)list.entries() / (float)bitmapMaxNullBitIndex);
    float limit = (float)1/(float)8;

    // If limit reached, generate longer NOT-NULL_BRANCH check
    if (real < limit)
      differentRowsFound = TRUE;
  }

  if (packedFormat && !alignedFormat) {
    if (differentRowsFound) {
      // Length of inst is (opc + len + sub-opc + 2*#operands + branch)
      length = 3 + 2*list.entries() + 1;
      newInst = createInst(PCIT::NOT_NULL_BRANCH_BULK, length);
      newInst->code[1] = length;

      newInst->code[2] = (differentRowsFound) ? 1 : 0;

      for (i=0; i < list.entries(); i++) {
        newInst->code[2*i + 3] = list[i]->getStackIndex();
        newInst->code[2*i + 4] = list[i]->getOffset();
      }
    }
    else {
      // Length of inst is (opc + len + sub-opc + idx + #operands + branch)
      length = 4 + list.entries() + 1;
      newInst = createInst(PCIT::NOT_NULL_BRANCH_BULK, length);
      newInst->code[1] = length;

      newInst->code[2] = (differentRowsFound) ? 1 : 0;

      newInst->code[3] = list[0]->getStackIndex();
      for (i=0; i < list.entries(); i++) {
        newInst->code[4+i] = list[i]->getOffset();
      }
    }
  }
  else if ((alignedFormat && !packedFormat) && (!differentRowsFound)) {
    // Length of inst is (opc + len + sub-opc + bitmap-idx + bitmap-off +
    //                    bitmap-len + bitmap-mask + branch)
    length = 6 + ((bitmapMaxNullBitIndex >> 5) + 1) + 1;
    newInst = createInst(PCIT::NULL_BITMAP_BULK, length);
    newInst->code[1] = length;

    newInst->code[2] = (differentRowsFound) ? 1 : 0;

    newInst->code[3] = bitmapIdx;
    newInst->code[4] = bitmapOff;
    newInst->code[5] = (bitmapMaxNullBitIndex >> 3) + 1;

    // Null out bitmap mask
    for (i=0; i < (CollIndex)((bitmapMaxNullBitIndex >> 5) + 1); i++)
      newInst->code[6+i] = 0;

    for (i=0; i < list.entries(); i++) {
      Int32 bitIdx = list[i]->getNullBitIndex();

      ((char*)(&newInst->code[6]))[bitIdx >> 3] |=
        ((UInt32)0x1 << (7 - (bitIdx & 7)));
    }
  }
  else {
    // Length of inst is (opc + len + sub-opc + (3 * #operands) + branch)
    length = 3 + 3*list.entries() + 1;
    newInst = createInst(PCIT::NULL_BITMAP_BULK, length);
    newInst->code[1] = length;

    newInst->code[2] = (differentRowsFound) ? 1 : 0;

    for (i=0; i < list.entries(); i++) {
      newInst->code[3*i + 3] = list[i]->getStackIndex();
      newInst->code[3*i + 4] = list[i]->getOffset();
      newInst->code[3*i + 5] = list[i]->getNullBitIndex();
    }
  }


  loadOperandsOfInst(newInst);

  // Add the bulk not null branch to the end of the new block. Add the 
  // fall-through edge to the startBlock, and add an target edge to the 
  // copy blocks.

  newBlock->insertInstBefore(NULL, newInst);
  newBlock->addEdge(startBlock);
  newBlock->addEdge(copyBlocks);

  if (startBlock == entryBlock_)
    entryBlock_ = newBlock;

  return TRUE;
}

// 
// Create bulk move instructions for adjacent moves.
// 
void PCodeCfg::bulkMove(NABoolean sameSizeMoves)
{
  CollIndex i, j, srcStackIndex;
  PCodeInst* newInst;
  INSTLIST list(heap_);
  PCodeInst* lastMoveInst = NULL;

  FOREACH_BLOCK(block, firstInst, lastInst, index) {

    // Since we're finding groups of candidate instructions for the bulk move,
    // we need to restart at the end of each group.  Begin at first inst
    for (PCodeInst* start = firstInst; start; ) {

      Int32 length = 0;

      // Clear the bulk move group list;
      list.clear();

      srcStackIndex = 0;

      // Given a start inst, traverse down the block in search of adjacent
      // move inst with similar source and target operands.
      FOREACH_INST_IN_BLOCK_AT(block, inst, start) {

        // Reset start inst in prep for next group
        start = inst->next;

        // Since sameSizeMoves should always be TRUE for now, we only allow
        // copy moves.
        assert (sameSizeMoves == TRUE);
        if (inst->isCopyMove() || inst->isConstMove()) {

          if (list.isEmpty()) {
            // This inst will begin the bulk move group.  Record the stack
            // index used for the source, if one exists

            list.insert(inst);
            if (inst->getROps().entries() > 0)
              srcStackIndex = inst->getROps()[0]->getStackIndex();

            if (inst->getOpcode() == PCIT::MOVE_MBIN8_MBIN8_IBIN32S)
              length++;
          }
          else if (inst->getWOps()[0]->getStackIndex() ==
                   list[0]->getWOps()[0]->getStackIndex())
          {
            // Target operands match, now we just need to make sure that source
            // operands match, if one exists

            if (inst->getROps().entries() > 0) {
              if (srcStackIndex != 0) {
                if (srcStackIndex != inst->getROps()[0]->getStackIndex()) {
                  start = inst;
                  break;
                }
              }
              else
                srcStackIndex = inst->getROps()[0]->getStackIndex();
            }

            lastMoveInst = inst;

            // Insert this inst in the list by grouping them with like instrs.
            for (i=list.entries()-1; i >= 0; i--) {
              CollIndex instWIdx, prevWIdx;

              if (list[i]->getOpcode() == inst->getOpcode()) {
                list.insertAt(i+1, inst);
                break;
              }

              instWIdx = inst->getWOps()[0]->getBvIndex();
              prevWIdx = list[i]->getWOps()[0]->getBvIndex();

              if ((i == 0) ||
                  (inst->getROps().entries() &&
                    (inst->getROps()[0]->getBvIndex() == prevWIdx)) ||
                  (list[i]->getROps().entries() &&
                    (instWIdx == list[i]->getROps()[0]->getBvIndex())) ||
                  (instWIdx == prevWIdx))
              {
                // If no match was found or a dependency violation exists 
                // which prevents the rearrangement, just insert at the end
                list.insert(inst);
                break;
              }
            }

            if (inst->getOpcode() == PCIT::MOVE_MBIN8_MBIN8_IBIN32S)
              length++;
          }
          else {
            start = inst;
            break;
          }
        }
        else if (!list.isEmpty()) {
          // If we arrived at a non-move inst and the group has already been
          // formed, break out
          break;
        }
      } ENDFE_INST_IN_BLOCK

      // Here we generate the bulk move inst for the group found if a group of
      // more than 1 moves were found.
      if (list.entries() > 1) {

        // Length of inst is (3 * #operands) + opc + len + tgt-idx + src-idx
        // Each operand will be (opc, tgt-off, src-off), and may include
        // len of move as well if dealing with PCIT::MOVE_MBIN8_MBIN8_IBIN32S.
        // Also, we do a "+=" because length is previously incremented while
        // searching for groups for the car string moves

        length += 3*list.entries() + 4;

        // Format of bulk move instruction is this:
        // 
        // Opc, length, wrt stk idx, src stk idx, [opc1, tgt-off1, src-off1]
        //                                        [opc2, tgt-off2, src-off2]
        //                                        ....
        //
        // And if we're dealing with a fast move instruction, that will be:
        // 
        //                                        [opc3, tgt-off3, src-off3, sz]
        //
        newInst = createInst(PCIT::MOVE_BULK, length);
        newInst->code[1] = length;
        newInst->code[2] = list[0]->getWOps()[0]->getStackIndex();
        newInst->code[3] = srcStackIndex;

        // Record the last inst before we should insert the buld move inst.
        PCodeInst* indicatorInst = lastMoveInst->next;

        // Walk through each read operand in the list and insert the appropriate
        // fields in the pcode byte stream for the bulk move instruction.
        for (i=0, j=4; i < list.entries(); i++, j+=3) {
          // Source offset will either be a *real* source offset, or it is the
          // constant for a constant move
          Int32 srcOffset = ((list[i]->getROps().entries())
                               ? list[i]->getROps()[0]->getOffset()
                               : list[i]->code[3]);

          // Target offset
          Int32 tgtOffset = list[i]->getWOps()[0]->getOffset();

          newInst->code[j] = list[i]->getOpcode();
          newInst->code[j+1] = list[i]->getWOps()[0]->getOffset();
          newInst->code[j+2] = srcOffset;

          if (list[i]->getOpcode() == PCIT::MOVE_MBIN8_MBIN8_IBIN32S) {
            newInst->code[j+3] = list[i]->code[5];
            j++;
          }

          block->deleteInst(list[i]);
        }

        loadOperandsOfInst(newInst);

        // If indicatorInst is null, that means we want to insert the bulk move
        // inst at the end of the block.
        if (indicatorInst)
          block->insertInstBefore(indicatorInst, newInst);
        else
          block->insertInstAfter(NULL, newInst);
      }
    }
  } ENDFE_BLOCK
}
