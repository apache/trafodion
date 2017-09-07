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

//
// Calculate the selectivity of this predicate group by taking both hit-ratio
// and cost into account.  The higher the hit-ratio (and lower the cost) results
// in a higher selectivity (i.e. a better chance for this predicate group to
// be scheduled earlier
//
float PCodePredicateGroup::getSelectivity(NABoolean isStatic)
{
  // If static, taken is 1 so that only cost is recognized.
  float taken = (isStatic) ? ((float)1) : ((float)takenCount_);
  float hitRatio = (((float)taken) / ((float)seenCount_));
  float efficiency = hitRatio / ((float)cost_);
  
  return efficiency;
}

//
// The major selectivity function is based on hit-ratio.  A predicate group
// that is seen just once, and taken, would result in a 100% hit-ratio, which
// would make it a better candidate than say a predicate group that is taken
// 99 times out of the 100 which it is seen.  With cost being equal, it seems
// unfair to penalize a predicate group just because it's hit-ratio isn't the
// best.
//
// This minor selectivity function is used to give credit to those predicate
// groups with high taken counts - with all else being equal, a predicate 
// group with a higher taken count can be considered a "reliable" or
// "guaranteed" choice.
//
float PCodePredicateGroup::getMinorSelectivity(NABoolean isStatic)
{
  // If static, taken is 1 so that only cost is recognized.
  float taken = (isStatic) ? ((float)1) : ((float)takenCount_);
  float efficiency = taken / ((float)cost_);
  
  return efficiency;
}

void PCodePredicateGroup::destroy(GROUPLIST& allGroups)
{
#if 0
  for (CollIndex i=0; i < allGroups.entries(); i++)
    NADELETE(allGroups[i], PCodePredicateGroup, heap_);
#endif
}

//
// Constructor for PCodePredicateGroup.  The "isStatic" parameter is passed in
// as TRUE when static reordering is performed, and FALSE when dynamic
// reordering is done during runtime optimizations.
//
PCodePredicateGroup::PCodePredicateGroup(CollHeap* heap,
                                         PCodeBlock* member,
                                         NABoolean isStatic,
                                         PCodeCfg* cfg)
  : heap_(heap), predicates_(heap), reads_(heap), writes_(heap)
{
  PCodeInst* lastInst = member->getLastInst();

  // Add member to predicate group list
  predicates_.insert(member);

  // Convert logical branches to logical counted branches
  if (lastInst->isLogicalBranch()) {
    // A predicate group that is a logical branch must be compile-time only
    assert(isStatic);

    // Get appropriate opcode for logical counted branch
    PCIT::Instruction opc = ((lastInst->getOpcode() == PCIT::BRANCH_AND)
                              ? PCIT::BRANCH_AND_CNT : PCIT::BRANCH_OR_CNT);

    // Add logical counted branch at end and delete old logical branch
    PCodeInst* branch = member->insertNewInstAfter(lastInst, opc);

    // Rewrite pcode operands for this inst
    branch->code[2] = 0; // clear trigger count
    branch->code[3] = lastInst->code[3];
    branch->code[4] = lastInst->code[4];
    branch->code[5] = lastInst->code[5];
    branch->code[6] = lastInst->code[6];
    branch->code[7] = 0; // clear seen count
    branch->code[8] = 0; // clear taken count

    // Reload operands
    branch->reloadOperands(cfg);

    // Delete old logical branch
    member->deleteInst(lastInst);
  }

  // Set the counts for this basic predicate group
  seenCount_ = (isStatic) ? 1 : member->getLastInst()->code[7];
  takenCount_ = (isStatic) ? 0 : member->getLastInst()->code[8];

  // Set the cost_ for this basic predicate group.
  cost_ = 0;
  FOREACH_INST_IN_BLOCK(member, inst) {
    cost_ += inst->getCost();
  } ENDFE_INST_IN_BLOCK

  // Always make cost_ a multiple of the second costliest PCODE instruction.
  // At minimum its cost should be 1.
  //cost_ = (cost_ < 100) ? 1 : (cost_/100 * 100);
}

#if defined(_DEBUG)

void PCodePredicateGroup::print()
{
  CollIndex i;

  // Print all predicate blocks in this group
  printf("(");
  for (i=0; i < predicates_.entries(); i++)
    printf(" %d ", predicates_[i]->getBlockNum());

  // Print counts and costs for this predicate group
  printf(") Cost=%d, Seen=%d, Taken=%d ",
          (Int32)cost_, (Int32)seenCount_, (Int32)takenCount_);

  // Print reads
  printf(", Reads=");
  for (i=0; i < reads_.entries(); i++)
    printf("%d ", reads_[i]->getBvIndex());

  // Print writes
  printf(", Writes=");
  for (i=0; i < writes_.entries(); i++)
    printf("%d ", writes_[i]->getBvIndex());

  printf("\n");
}

#endif // defined(_DEBUG)

//
// Gets invoked BEFORE predicate groups within unit have been shuffled.  This
// should not be called for the static solution
//
void PCodePredicateGroup::adjustCost(GROUPLIST& unit)
{
  CollIndex i;
  PCodePredicateGroup* first = unit[0];
  PCodePredicateGroup* last = unit[unit.entries() - 1];

  // Taken counts can be determined for any given predicate group by comparing
  // its seen count with that of it's successor - the difference indicates how
  // many times control exited the unit (early-exit).  This method can't be
  // done for the last predicate group in the unit, however.

  for (i=0; i < unit.entries()-1; i++)
    unit[i]->setTakenCount(unit[i]->getSeenCount() - unit[i+1]->getSeenCount());

  // Counts need to be flipped when the last member falls through to the target
  // of the other members in the unit.

  // The taken count for the last predicate group represents the number of times
  // control exited-early *within* that group.  But now that this group is
  // part of a unit, the taken count may need to be flipped since early-exit
  // means something else in this case.  And since both a BRANCH_AND_CNT or a
  // BRANCH_OR_CNT may be used in the last predicate block to target the same
  // exit block, special-care must be taken.
  //
  // If the last predicate group in the unit has a taken count associated with
  // the same early-exit as that of the first predicate group, then no need to
  // adjust the counts since they accurately reflect the early-exit.  If not,
  // however, then the counts needs to be flipped.
  //
  if (first->getLastPredBlock()->getTargetBlock() !=
      last->getFirstPredBlock()->getTargetBlock())
  {
    // The fallthrough of this last predicate group is in fact the early exit
    // block, so flip counts.
    last->setTakenCount(last->getSeenCount() - last->getTakenCount());
  }
}

//
// Gets invoked AFTER predicate groups within unit have been shuffled based
// on cost
//
void PCodePredicateGroup::setCost(GROUPLIST& unit, NABoolean isStatic)
{
  CollIndex i;
  Int64 totalSeen = 0;
  Int64 totalTaken = 0;
  Int64 totalCost = 0;

  // First determine how many times this unit was executed and set seenCount_
  // for the unit accordingly.  Because any predicate group can be the head of
  // this unit after swapping is done, the group with the highest seenCount_
  // must have the correct seenCount_ for the unit.

  for (i=1; i < unit.entries(); i++) {
    if (seenCount_ < unit[i]->getSeenCount())
      seenCount_ = unit[i]->getSeenCount();
  }

  // Just to avoid potential divide-by-zero
  if (seenCount_ == 0)
    return;

  // Recalculate seen and taken counts based on order of predicate groups in
  // unit. 
  for (totalSeen = seenCount_, i=0; i < unit.entries(); i++) {
    // Old counts for this unit
    Int64 seen = unit[i]->getSeenCount();
    Int64 taken = unit[i]->getTakenCount();

    // Increment taken count for this unit
    totalTaken += taken;

    // Calculate hit ratio
    double hitRatio = (seen == 0) ? 0 : ((float)taken / (float)seen);

    // New predicated taken counts for this unit
    taken = (Int64)(hitRatio * (float)totalSeen);

    // Increment cost for this unit
    totalCost += (unit[i]->getCost() * totalSeen);

    // Next child will see however many this child saw minus early exits
    totalSeen -= taken;
  }
  
  takenCount_ = totalTaken;

  // If static, assume on average that half the blocks are visited in this
  // unit before an early exit.  As such, divide the total cost seen by 2.  If
  // dynamic, the weighted average is taken by refactoring in the seen count.
  if (isStatic)
    cost_ = (Int64)((float)totalCost / (float)2);
  else
    cost_ = (Int64)((float)totalCost / (float)seenCount_);

  // Always make cost_ a multiple of the second costliest PCODE instruction.
  // At minimum its cost should be 1.
  //cost_ = (cost_ < 100) ? 1 : (cost_/100 * 100);
}

//
// Swap this predicate group with the "child" or "down" predicate group passed
// in.  Edges to and from each predicate block in each of the predicate groups
// are rewired so that one replaces the other in the control flow graph
//
void PCodePredicateGroup::swap(PCodePredicateGroup* down, NABoolean adjacent)
{
  CollIndex i;
  Int32 tempOpc;

  BLOCKLIST origPredsDown(heap_);
  BLOCKLIST origPredsUp(heap_);
  BLOCKLIST origSuccsDown(heap_);
  BLOCKLIST origSuccsUp(heap_);

  //
  // Copying one NALIST into another has proven to be difficult.  The "="
  // operator defined in the base class NACollection is for some reason not
  // found at compile-time.  And the copy constructor itself doesn't end up
  // copying the entries in the list.  So the solution used here is to first
  // create the list and then to directly each entry into the new list.
  //

  // Copy down's preds into origPredsDown
  for (i=0; i < down->getFirstPredBlock()->getPreds().entries(); i++)
    origPredsDown.insert(down->getFirstPredBlock()->getPreds()[i]);

  // Copy up's preds into origPredsUp
  for (i=0; i < getFirstPredBlock()->getPreds().entries(); i++)
    origPredsUp.insert(getFirstPredBlock()->getPreds()[i]);

  // Copy down's succs into origSuccsDown
  for (i=0; i < down->getLastPredBlock()->getSuccs().entries(); i++)
    origSuccsDown.insert(down->getLastPredBlock()->getSuccs()[i]);

  // Copy up's succs into origSuccsUp
  for (i=0; i < getLastPredBlock()->getSuccs().entries(); i++)
    origSuccsUp.insert(getLastPredBlock()->getSuccs()[i]);


  // Record the first and last block of the up and down groups

  PCodeBlock* origFirstBlockDown = down->getFirstPredBlock();
  PCodeBlock* origFirstBlockUp = getFirstPredBlock();
  PCodeBlock* origLastBlockDown = down->getLastPredBlock();
  PCodeBlock* origLastBlockUp = getLastPredBlock();

  //
  // Fix up pred edges to new top group
  //

  for (i=0; i < origPredsUp.entries(); i++) {

    // Add edge from preds to down, properly adding fall-through edges
    if (origPredsUp[i]->getFallThroughBlock() == origFirstBlockUp)
      origPredsUp[i]->addFallThroughEdge(origFirstBlockDown);
    else
      origPredsUp[i]->addEdge(origFirstBlockDown);

    // Remove the old pred edges to up
    origPredsUp[i]->removeEdge(origFirstBlockUp);
  }

  //
  // Fix up pred edges to new down group
  //

  for (i=0; !adjacent && (i < origPredsDown.entries()); i++) {
    // Add edge from preds to down, properly adding fall-through edges
    if (origPredsDown[i]->getFallThroughBlock() == origFirstBlockDown)
      origPredsDown[i]->addFallThroughEdge(origFirstBlockUp);
    else
      origPredsDown[i]->addEdge(origFirstBlockUp);

    // Remove the old pred edges to down
    origPredsDown[i]->removeEdge(origFirstBlockDown);
  }

  //
  // Fix interior nodes of new top group (i.e not last node)
  //

  for (i=0; i < down->getPredicateBlocks().entries()-1; i++) {
    PCodeBlock* block = down->getPredicateBlocks()[i];

    // If this block points to some other interior node, then keep it - it
    // doesn't need to be fixed up.
    if ((block->getTargetBlock() != origLastBlockDown->getTargetBlock()) &&
        (block->getTargetBlock() != origLastBlockDown->getFallThroughBlock())) 
      continue;

    // First remove edge to target block
    block->removeEdge(block->getTargetBlock());

    // Replace with edge to new target block, which will either be the beginning
    // of the old up group or the target of the old up group.
    if (block->getLastInst()->getOpcode() ==
        origLastBlockUp->getLastInst()->getOpcode())
      block->addEdge(origLastBlockUp->getTargetBlock());
    else {
      if (adjacent)
        block->addEdge(origFirstBlockUp);
      else
        block->addEdge(origLastBlockUp->getFallThroughBlock());
    }
  }

  //
  // Fix interior nodes of new down group (i.e. not last node)
  //

  for (i=0; i < getPredicateBlocks().entries()-1; i++) {
    PCodeBlock* block = getPredicateBlocks()[i];

    // If this block points to some other interior node, then keep it - it
    // doesn't need to be fixed up.
    if ((block->getTargetBlock() != origLastBlockUp->getTargetBlock()) &&
        (block->getTargetBlock() != origLastBlockUp->getFallThroughBlock())) 
      continue;

    // First remove edge to target block
    block->removeEdge(block->getTargetBlock());

    // Replace with edge to new target block, which will either be the beginning
    // of the old up group or the target of the old up group.
    if (block->getLastInst()->getOpcode() ==
        origLastBlockDown->getLastInst()->getOpcode())
      block->addEdge(origLastBlockDown->getTargetBlock());
    else
      block->addEdge(origLastBlockDown->getFallThroughBlock());
  }

  // Swap opcodes
  // TODO: we may need to swap result operands as well (qat/qatdml13)
#if 0
      select count(*)
      from btsel01
      where
            (binary_signed > 1000
            AND pic_decimal_1 + pic_decimal_2 * 2 <= 8)
            OR  pic_x_7 NOT LIKE ?p4
            OR  pic_decimal_3 <> 8
            AND pic_decimal_3 >= 6;
#endif

  tempOpc = origLastBlockUp->getLastInst()->getOpcode();
  origLastBlockUp->getLastInst()->code[0] =
    origLastBlockDown->getLastInst()->getOpcode();
  origLastBlockDown->getLastInst()->code[0] = tempOpc;


  //
  // Remove down's succs and up's succs and then fix them up
  //

  while (origLastBlockDown->getSuccs().entries())
    origLastBlockDown->removeEdge(origLastBlockDown->getSuccs()[0]);

  while (origLastBlockUp->getSuccs().entries())
    origLastBlockUp->removeEdge(origLastBlockUp->getSuccs()[0]);

  if (origSuccsUp[0] == origFirstBlockDown)
    origLastBlockDown->addEdge(origFirstBlockUp);
  else
    origLastBlockDown->addEdge(origSuccsUp[0]);

  origLastBlockDown->addEdge(origSuccsUp[1]);
  origLastBlockUp->addEdge(origSuccsDown[0]);
  origLastBlockUp->addEdge(origSuccsDown[1]);

  //
  // Fix up entryBlock
  //

  if (origFirstBlockUp->isEntryBlock())
    origFirstBlockDown->setToEntryBlock();
}

//
// Determine if the predicate group in the unit at index "head" can be swapped
// with that at index "tail".  The two can't be swapped if, in between them,
// is a predicate group with a "no cross" stamp
//
NABoolean PCodePredicateGroup::containsXBlock(GROUPLIST& unit,
                                              CollIndex head,
                                              CollIndex tail)
{
  CollIndex i, j;
  PCodeOperand *read, *write;
  PCodePredicateGroup* top = unit[head];
  PCodePredicateGroup* bottom = unit[tail];

  // WAW violations - no violation if write are the same in all groups
  for (i=tail-1; (Int32)i >= (Int32)head; i--)
  {
    for (j=0; j < bottom->getWrites().entries(); j++) {
      write = bottom->getWrites()[j];
      if (!(unit[i]->getWrites().contains(write)))
        return TRUE;
    }

    for (j=0; j < unit[i]->getWrites().entries(); j++) {
      write = unit[i]->getWrites()[j];
      if (!(bottom->getWrites().contains(write)))
        return TRUE;
    }
  }

  // RAW violation
  for (i=0; i < bottom->getReads().entries(); i++) {
    read = bottom->getReads()[i];
    // Check if any of bottom's read operands conflict with write operands
    for (j=tail-1; (Int32)j >= (Int32)head; j--)
      if (unit[j]->getWrites().contains(read))
        return TRUE;
  }

  // WAR violations
  for (i=0; i < bottom->getWrites().entries(); i++) {
    write = bottom->getWrites()[i];
    // Check if any of bottom's write operands conflict with read operands
    for (j=tail-1; (Int32)j >= (Int32)head; j--)
      if (unit[j]->getReads().contains(write))
        return TRUE;
  }

  // RAW violation (starting with top now)
  for (i=0; i < top->getReads().entries(); i++) {
    read = top->getReads()[i];
    // Check if any of bottom's read operands conflict with write operands
    for (j=head+1; j <= tail; j++)
      if (unit[j]->getWrites().contains(read))
        return TRUE;
  }

  // WAR violations (starting with top now)
  for (i=0; i < top->getWrites().entries(); i++) {
    write = top->getWrites()[i];
    // Check if any of top's write operands conflict with read operands
    for (j=head+1; j <= tail; j++)
      if (unit[j]->getReads().contains(write))
        return TRUE;
  }

  return FALSE;
}

//
// Identify groups which are (or aren't) PIC (position-independent-code) blocks.
// Blocks are not PIC if either 1) they defined a write operand which gets read
// in a different predicate block or 2) they read an operand which has no def in
// it's block.  In either case, the reads and writes list of the block are
// updated.
//
void PCodePredicateGroup::identifyPICGroups(PCodeCfg* cfg,
                                            CollHeap* heap,
                                            GROUPLIST& allGroups)
{
  CollIndex i, j, k;
  PCodeBlock* block = getPredicateBlocks()[0];

  NABitVector operands(heap);

  PCodeBlock* entryBlock = cfg->getEntryBlock();

  FOREACH_INST_IN_BLOCK_BACKWARDS(block, inst) {
    // Kill defs first
    for (i=0; i < inst->getWOps().entries(); i++)
      operands -= inst->getWOps()[i]->getBvIndex();

    // Now add uses - note, consts don't count since they are constant defs
    for (i=0; i < inst->getROps().entries(); i++)
      if (!inst->getROps()[i]->isConst())
        operands += inst->getROps()[i]->getBvIndex();
  } ENDFE_INST_IN_BLOCK_BACKWARDS

  i = operands.getLastStaleBit();
  for (; (i = operands.prevUsed(i)) != NULL_COLL_INDEX; i--)
  {
    PCodeOperand* read = cfg->getMap()->getFirstValue(&i);

    PCodeInst* defE = entryBlock->reachingDefsTab_->find(i, TRUE /* In */);
    PCodeInst* defB = block->reachingDefsTab_->find(i, TRUE /* In */);

    // If the def comes from entry, continue
    if ((defE != NULL) && (defE == defB))
      continue;

    // Add read to reads list
    addRead(read);

    // If the def comes from a predicate group, then continue to consider this
    // as a predicate block, but mark the predicate block containing the def
    // as one which swapping can't cross over - i.e. no cross group.
    for (j=0; j < allGroups.entries(); j++) {
      PCodeBlock* basicPB = allGroups[j]->getPredicateBlocks()[0];

      if (basicPB->reachingDefsTab_->find(i) == NULL)
        continue;

      FOREACH_INST_IN_BLOCK_BACKWARDS(basicPB, def) {
        for (k=0; k < def->getWOps().entries(); k++)
          if (def->getWOps()[k]->getBvIndex() == i) {
            allGroups[j]->addWrite(read);
          }
      } ENDFE_INST_IN_BLOCK_BACKWARDS
    }
  }
}

//
// Determine if this basic block can be treated as a basic predicate group (or
// a predicate block).  Basic predicate groups must be self-containing such
// that they are position-independent and can be moved freely.
//
NABoolean PCodePredicateGroup::isPredicateBlock(PCodeBlock* block)
{
  FOREACH_INST_IN_BLOCK_BACKWARDS(block, inst) {
    // Some clauses should not be allowed to be swapped because they have
    // side-effects.
    if (inst->clause_ &&
        (inst->clause_->getClassID() == ex_clause::FUNC_RAISE_ERROR_ID))
      return FALSE;
  } ENDFE_INST_IN_BLOCK_BACKWARDS

  return TRUE;
}

//
// Returns TRUE if this predicate group "dominates" the sibling predicate group
// passed in.  It does so if and only iff all edges flowing into the sibling
// predicate group come from this predicate group.
//
NABoolean PCodePredicateGroup::dominates(PCodePredicateGroup* sibling)
{
  CollIndex i;
  PCodeBlock* siblingFirstBlock = sibling->getFirstPredBlock();

  // Since the first predicate block of the sibling predicate group dominates
  // all the predicate block members in its group, we only need to check the
  // edges flowing into the sibling's first pred block to see if this predicate
  // group dominates sibling.
  for (i=0; i < siblingFirstBlock->getPreds().entries(); i++)
  {
    PCodeBlock* pred = siblingFirstBlock->getPreds()[i];

    // If this group does not contain the predicate block, then it can't
    // possibly dominate sibling
    if (predicates_.index(pred) == NULL_COLL_INDEX)
      return FALSE;
  }

  // If sibling has no preds, it can't be dominated
  if (i == 0)
    return FALSE;

  return TRUE;
}

//
// Recursive algorithm to determine the unit head by this predicate group.  In
// the process of this algorithm, other sub-units may be discovered and thereby
// processed, in order to correctly discover this unit.
//
// A "unit" list is passed in to record all predicate groups for this unit.  The
// workList of predicate groups to search for inclusion is also passed in.  If
// a swap occurred at any point during this algorithm, this knowledge is passed
// back to the caller.  And lastly, the flag "isStatic" is passed in for use
// in the algorithm
//
// Return TRUE if any unit is found
//
NABoolean PCodePredicateGroup::findUnits(GROUPLIST& unit,
                                         GROUPLIST& workList,
                                         NABoolean& swapPerformed,
                                         NABoolean isStatic)
{
  CollIndex i, j, k;
  PCodePredicateGroup *sibling, *group = this;
  NABoolean unitFound = FALSE;

#if defined(_DEBUG)
  NABoolean debug = FALSE;
#endif // defined(_DEBUG)

  // The group is part of the unit, so add to unit and remove from workList
  unit.insert(group);
  workList.remove(group);

  // Find siblings
  for (i=0; i < workList.entries(); i++)
  {
    // Record potential sibling
    sibling = workList[i];

    // 1. This group must dominate the potential sibling group
    if (!group->dominates(sibling))
      continue;

    // 2. Target of this group must either be the fall-through/target block of
    //    the potential sibling group, and that even depends on the branch opc
    //    A change in opc implies that the target of the older sibling should
    //    be the fall-through of the younger sibling, if the two are to be in
    //    the same predicate group.

    PCodeBlock* target = group->getLastPredBlock()->getTargetBlock();
    PCodeBlock* siblingLastBlock = sibling->getLastPredBlock();
    Int32 opc = group->getLastPredBlock()->getLastInst()->getOpcode();

    // If older and younger sibling have different opcodes...
    if (opc != siblingLastBlock->getLastInst()->getOpcode())
    {
      // The two siblings are in the same predicate group if the target block
      // of the older sibling is the same as the fall-through block of the
      // younger sibling.  If they're not the same, this implies that the
      // younger sibling may in fact be part of a sub-unit, and that unit needs
      // to be materialized first before we can move forward.

      if (target != siblingLastBlock->getFallThroughBlock()) {

        // Attempt to find a sub-unit for this younger sibling
        GROUPLIST tempUnit(heap_);
        if (sibling->findUnits(tempUnit, workList, swapPerformed, isStatic))
          unitFound = TRUE;

        // Check if condition (2) is now met.  If not, restart search for
        // younger sibling.
        if (opc != sibling->getLastPredBlock()->getLastInst()->getOpcode()) {
          if (sibling->getLastPredBlock()->getFallThroughBlock() != target) {
            i = (CollIndex)-1;
            continue;
          }
        }
        else {
          if (sibling->getLastPredBlock()->getTargetBlock() != target) {
            i = (CollIndex)-1;
            continue;
          }
        }

        // All conditions met, so insert younger sibling into predicate group
        unit.insert(tempUnit[0]);
        workList.remove(tempUnit[0]);
      }
      else {
        // Insert younger sibling into predicate group
        unit.insert(sibling);
        workList.remove(sibling);
      }
    }
    else
    {
      // This algorithm is the same as the logic above in the then clause - it
      // just treats things differently because of the opcode check.
      if (target != siblingLastBlock->getTargetBlock()) {
        GROUPLIST tempUnit(heap_);
        if (sibling->findUnits(tempUnit, workList, swapPerformed, isStatic))
          unitFound = TRUE;

        if (opc != sibling->getLastPredBlock()->getLastInst()->getOpcode()) {
          if (sibling->getLastPredBlock()->getFallThroughBlock() != target) {
            i = (CollIndex)-1;
            continue;
          }
        }
        else {
          if (sibling->getLastPredBlock()->getTargetBlock() != target) {
            i = (CollIndex)-1;
            continue;
          }
        }

        unit.insert(tempUnit[0]);
        workList.remove(tempUnit[0]);
      }
      else {
        unit.insert(sibling);
        workList.remove(sibling);
      }
    }

    // Now group becomes what the sibling was
    group = sibling;

    // Restart search from the beginning now
    i=(CollIndex)-1;
  }

#if defined(_DEBUG)
  // Just some extra checking to make sure we've exhausted all possible
  // siblings.  TODO: remove this once testing shows it to be stable.
  sibling = unit[0];
  for (i=0; i < workList.entries(); i++) {
    group = workList[i];

    if (group->dominates(sibling))
      assert(FALSE);
  }
#endif // defined(_DEBUG)

  // If unit isn't big enough, continue
  if (unit.entries() < 2)
    return unitFound;

  // Mark flag that a unit was found and so this expression should be analyzed
  // at runtime to see if counts affect how predicates within this expression
  // should be arranged.
  unitFound = TRUE;

  // At this point, group represents the leader of the unit.  Fix up the
  // costs of the last block within this unit, should it need fixing.
  if (!isStatic)
    group->adjustCost(unit);

#if defined(_DEBUG)
  if (debug) {
    printf("Unit found:\n");
    for (i=0; i < unit.entries(); i++)
      unit[i]->print();
    printf("\n");
  }
#endif // defined(_DEBUG)

  // Record the fact on whether or not this unit contains a predicate group
  // with a write operand.
  NABoolean writeFound = FALSE;
  for (j=0; j < unit.entries(); j++) {
    if (unit[j]->getWrites().entries()) {
      writeFound = TRUE;
      break;
    }
  }

  // Swap elements in unit based on cost
  for (j=0; j < unit.entries(); j++) {
    // Make the assumption that the first predicate group member in this unit
    // has the best selectivity, and therefore is appropriately placed
    CollIndex minIndex = j;

    // Go through each predicate group member and check if that member has a
    // selectivity higher than the one identified so far.
    for (k=j+1; k < unit.entries(); k++) {

      float kSel = unit[k]->getSelectivity(isStatic);
      float minSel = unit[minIndex]->getSelectivity(isStatic);
      float diff = kSel - minSel;

      // Any difference in selectivity results is a reason for swapping
      if (diff > 0.0) {
        // Now for the secondary criteria where the hit-ratio isn't factored
        // in - the taken/cost ratio should be considerable so that we don't
        // let go of a good thing.

        kSel = unit[k]->getMinorSelectivity(isStatic);
        minSel = unit[minIndex]->getMinorSelectivity(isStatic);
        diff = (minSel == 0) ? kSel : (kSel/minSel);

        if (diff >= 0.25) {
          // One more check.  Do not allow the swap to occur if either groups
          // violate any read/write dependencies.
          if (!writeFound || !unit[j]->containsXBlock(unit, j, k))
            minIndex = k;
        }
      }
    }

    // Identify the minGroup - i.e. the group that should be scheduled earliest
    PCodePredicateGroup* minGroup = unit[minIndex];

    if (minIndex != j) {
      unit[j]->swap(minGroup, (j+1 == minIndex) /* are groups adjacent */ );

      // Move minimal cost group to position j, so as to not process it again
      unit[minIndex] = unit[j];
      unit[j] = minGroup;

      // Record the fact that a swap was made, changing the pcode graph
      swapPerformed = TRUE;
    }
  }

  // Have "group" point to the beginning of the unit, and then reset its cost
  // and counters based on the unit containing it.
  group = unit[0];
  group->setCost(unit, isStatic);

  // Propagate knowledge about reads and writes for this unit.
  for (i=1; i < unit.entries(); i++) {
    for (j=0; j < unit[i]->getReads().entries(); j++)
      group->getReads().insert(unit[i]->getReads()[j]);
    for (j=0; j < unit[i]->getWrites().entries(); j++)
      group->getWrites().insert(unit[i]->getWrites()[j]);
  }

#if defined(_DEBUG)
  if (debug) {
    printf("Unit after swap:\n");
    for (i=0; i < unit.entries(); i++)
      unit[i]->print();
    printf("\n");
  }
#endif // defined(_DEBUG)

  // Merge the rest of the unit into the leader predicate group
  while (unit.entries() > 1) {
    group->getPredicateBlocks().insert(unit[1]->getPredicateBlocks());
    unit.remove(unit[1]);
  }

  // Reinsert the merged group into the worklist
  workList.insertAt(0, group);

  return unitFound;
}

NABoolean PCodeCfg::reorderPredicates(NABoolean isStatic)
{
  CollIndex i;
  GROUPLIST allGroups(heap_);
  Int32 triggerCount;

  NABoolean swapPerformed = FALSE;
  NABoolean unitFound = FALSE;
  PCodeBlock* start = entryBlock_;

  // Currently only run this optimization if we're dealing with a SCAN
  if (expr_->getType() != ex_expr::exp_SCAN_PRED)
    return swapPerformed;

  // If we have a bulk null branch, we only want to optimize in the case when
  // we have no nulls to process.  This will make it less likely to find false
  // predicate blocks.
  if (start->getLastInst()->isBulkNullBranch())
    start = start->getTargetBlock();

  // Find all basic predicate groups.  It's very important that we process
  // blocks in the reverse depth-first-order so that calls to "findUnit" come
  // from predicate blocks which aren't dominated by any other predicate block.
  // This will ensure faster convergence for the algorithm.
  FOREACH_BLOCK_REV_DFO_AT(start, block, firstInst, lastInst, index)
  {
    // Only blocks terminated by a logical branch qualify (potentially) as 
    // a predicate block.  If terminated by a logical counted branch, no
    // additional check is needed since that implies that the block already
    // meets the requirements.

    if (lastInst &&
        (lastInst->isLogicalBranch() || lastInst->isLogicalCountedBranch()) &&
        PCodePredicateGroup::isPredicateBlock(block))
    {
      PCodePredicateGroup* group =
        (PCodePredicateGroup*) new(heap_)
          PCodePredicateGroup(heap_, block, isStatic, this);

      allGroups.insert(group);
    }
  } ENDFE_BLOCK_REV_DFO

  // Don't proceed if we don't have at least 2 predicate groups to swap.
  // Similarly, don't proceed if we have *too* many groups.
  if ((allGroups.entries() < 2) || (allGroups.entries() > 1000)) {
    if (allGroups.entries())
      allGroups[0]->destroy(allGroups);
    return FALSE;
  }

  // Now that we have the possibility of having a unit, identify read operands
  // that reach each predicate group, and write operands which are live at the
  // end of each predicate group.
  for (i=0; i < allGroups.entries(); i++)
    allGroups[i]->identifyPICGroups(this, heap_, allGroups);

  // Create a workList so as to maintain allGroups for later iteration
  GROUPLIST workList(heap_);
  for (i=0; i < allGroups.entries(); i++)
    workList.insert(allGroups[i]);

  // Find all units and reorder them individually (and together) based on cost
  // and frequency.
  while (workList.entries()) {
    GROUPLIST tempUnit(heap_);
    if (workList[0]->findUnits(tempUnit, workList, swapPerformed, isStatic))
      unitFound = TRUE;
  }

  // If no unit was found, then don't set trigger count for predicates in this
  // expr since no predicates were identified and thus none can be swapped
  // And since trigger count at compile-time is 0, we just need to return.
  // We also can leave trigger count as 0 if dynamic predicate reordering is
  // disabled
  if (!unitFound || (isStatic && !(optFlags_ & DYNAMIC_REORDER_PREDICATES))) {
    allGroups[0]->destroy(allGroups);
    return swapPerformed;
  }

  // Initialize trigger count for dynamic predicate reordering, and then reset
  // it based on whether or not a swap was performed
  triggerCount = allGroups[0]->getPredicateBlocks()[0]->getLastInst()->code[2];

  // Scale trigger count down if it reached a point that's too high, and force
  // counters to be cleansed by assuming a swap was done.
  if (triggerCount > 0x3FFFFFFF) {
    triggerCount = 0xFFFFF;
    swapPerformed = TRUE;
  }

  // Reset trigger count - init if static, double if dynamic
  triggerCount = (isStatic) ? TRIGGER_COUNT : (triggerCount + triggerCount);

  // Reset counters and trigger count here 
  for (i=0; i < allGroups.entries(); i++) {
    PCodePredicateGroup* group = allGroups[i];
    PCodeBlock* block = group->getPredicateBlocks()[0];
    PCodeInst* lastInst = block->getLastInst();

    // Set trigger count
    lastInst->code[2] = triggerCount;

    // Clear out counters if a swap was performed
    if (swapPerformed)
      lastInst->clearCounters();
  }

  allGroups[0]->destroy(allGroups);
  return swapPerformed;
}
