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

#include "BaseTypes.h"
#include "ExpPCodeOptimizations.h"
#include "CmpCommon.h"
#include "PCodeExprCache.h"

#if defined(_DEBUG)
  #define DUMP_PHASE(str,flag1,flag2) \
    if (flag1) printBlocks(str, flag2);
#else
  #define DUMP_PHASE(str,flag1,flag2)
#endif

/*****************************************************************************
 * Debug code
 *****************************************************************************/
void PCodeCfg::printConstants()
{
  char NExBuf[200];
  CollIndex i;
  Int32 j;

  char * stk = expr_->getConstantsArea();
  Int32 len = expr_->getConstsLength();

  NExLog("Constants Table:\n----------------\n");

  for (j=0; j < len; j++) {
    if (j%4 == 0)
      NExLog(" ");
    sprintf( NExBuf, "%02x", *((char*)(stk+j)) & 0xff);
    NExLog(  NExBuf );
  }

  NExLog("\n\nConstants Hash Table:\n-------------------------\n");

  if (constToOffsetMap_) {
    PCodeConstants* key;
    CollIndex* value;
    NAHashDictionaryIterator<PCodeConstants,CollIndex> iter(*constToOffsetMap_);

    for (i=0; i < iter.entries(); i++) {
      iter.getNext(key, value);
      sprintf( NExBuf, "data: %p, len: %d, off: %d   ", key->getData(),
        key->getLen(), *value);
      NExLog(  NExBuf );
      for (j=0; j < key->getLen(); j++) {
        if (j%4 == 0)
          NExLog(" ");
        sprintf( NExBuf, "%02x", ((char*)(key->getData()))[j] & 0xff);
        NExLog(  NExBuf );
      }
      NExLog("\n");
    }
  }
}

void PCodeCfg::printBlocks(const char* header, NABoolean fullDebug)
{
  char NExBuf[2048];
  sprintf( NExBuf, "\n*-*-*-*\n%s\n*-*-*-*\n", header);
  NExLog(  NExBuf );
  for (CollIndex i=0; i < allBlocks_->entries(); i++)
  {
    if (allBlocks_->at(i)->print(fullDebug))
      NExLog("\n");
  }
  NExLog("\n");
}

void PCodeCfg::printInsts(NABoolean fullDebug) {
  NExLog("--------\n");
  for (CollIndex i=0; i < allInsts_->entries(); i++) {
    allInsts_->at(i)->print(NULL, fullDebug);
  }
}

NABoolean PCodeBlock::print(NABoolean fullDebug)
{
  char NExBuf[2048];
  CollIndex i;
  PCodeInst* tempInst;

  // Don't print out blocks which are clearly dead
#if 1
  if ((first_ == NULL) && (last_ == NULL) &&
      (predsList_.entries() == 0) && (succsList_.entries() == 0))
    return FALSE;
#endif

  sprintf( NExBuf, "BLOCK: %p (%d)\n", this, blockNum_);
  cfg_->NExLog(  NExBuf );

  if (fullDebug) {
    sprintf( NExBuf, "Top Offset: %d\n", blockTopOffset_);
    cfg_->NExLog(  NExBuf );
    sprintf( NExBuf, "Bottom Offset: %d\n", blockBottomOffset_);
    cfg_->NExLog(  NExBuf );
  }

  cfg_->NExLog("Preds: ");
  for (i=0; i < predsList_.entries(); i++) {
    sprintf( NExBuf, "%d ", predsList_[i]->getBlockNum());
    cfg_->NExLog(  NExBuf );
  }
  cfg_->NExLog("\n");

  cfg_->NExLog("Succs: ");
  for (i=0; i < succsList_.entries(); i++) {
    sprintf( NExBuf, "%d ", succsList_[i]->getBlockNum());
    cfg_->NExLog(  NExBuf );
  }
  cfg_->NExLog("\n");

  if (fullDebug) {
    cfg_->NExLog("Liveness: ");
    i = liveVector.getLastStaleBit();
    for (; (i = liveVector.prevUsed(i)) != NULL_COLL_INDEX; i--)
    {
      PCodeOperand* operand = cfg_->getMap()->getFirstValue(&i);
      operand->print(cfg_);
      cfg_->NExLog(" ");
    }
    cfg_->NExLog("\n");

    cfg_->NExLog("Reaching: ");
    for (i=0; reachingDefsTab_ && (i < reachingDefsTab_->getSize()); i++)
    {
      Int32 hashVal = i % reachingDefsTab_->getSize();
      ReachDefsTable::ReachDefsElem* node =
        reachingDefsTab_->getRDefIn()[hashVal];

      while (node) {
        PCodeOperand* op = cfg_->getMap()->getFirstValue(&(node->bvIndex_));
        op->print(cfg_);
        cfg_->NExLog(" ");

        node = node->next_;
      }
    }
    cfg_->NExLog("\n");
  }

  for (tempInst = first_;
       tempInst && (tempInst != last_);
       tempInst = tempInst->next)
  {
    tempInst->print(cfg_, fullDebug);
  }

  if (last_)
    last_->print(cfg_, fullDebug);

  return TRUE;
}

void PCodeInst::print(PCodeCfg* cfg, NABoolean fullDebug)
{
  char NExBuf[2048];
  PCodeBinary* pcode = code;
  CollIndex i;

  sprintf( NExBuf, "%s ", PCIT::instructionString(PCIT::Instruction(pcode[0])));
  cfg->NExLog(  NExBuf );

  OPLIST oplist = getWOps();
  for (i=0; i < oplist.entries(); i++)
  {
    PCodeOperand* operand = oplist[i];
    PCIT::AddressingMode am = operand->getType();

    operand->print(cfg);
    sprintf( NExBuf, "(%s)", PCIT::addressingModeString(am));
    cfg->NExLog(  NExBuf );
  }
  cfg->NExLog(" = ");
  oplist = getROps();
  for (i=0; i < oplist.entries(); i++)
  {
    PCodeOperand* operand = oplist[i];
    PCIT::AddressingMode am = operand->getType();

    operand->print(cfg);
    sprintf( NExBuf, "(%s)", PCIT::addressingModeString(am));
    cfg->NExLog(  NExBuf );
    cfg->NExLog(",");
  }
  cfg->NExLog(" ");

  for (Int32 j=0; j < PCode::getInstructionLength(pcode); j++)
  {
    sprintf( NExBuf, "%d ", code[j]);
    cfg->NExLog(  NExBuf );
  }

  if (fullDebug) {
    cfg->NExLog("LIVE: ");
    i = liveVector_.getLastStaleBit();
    for (; cfg && ((i = liveVector_.prevUsed(i)) != NULL_COLL_INDEX); i--)
    {
      PCodeOperand* operand = cfg->getMap()->getFirstValue(&i);
      operand->print(cfg);
      cfg->NExLog(" ");
    }
  }

  cfg->NExLog("\n");
}


void PCodeOperand::print( PCodeCfg *cfg )
{
  char NExBuf[2048];
  if (nullBitIndex_ != -1)
  {
    sprintf( NExBuf, "[%d,%d,%d]", stackIndex_, offset_, nullBitIndex_);
    cfg->NExLog(  NExBuf );
  }
  else
  {
    sprintf( NExBuf, "[%d,%d]", stackIndex_, offset_);
    cfg->NExLog(  NExBuf );
  }
}



/*****************************************************************************
 * Hash functions for hash table
 *****************************************************************************/
ULng32 constHashFunc(const PCodeConstants& c) {
  char* data = (char*)c.data_;
  ULng32 val = 0;
  Int32 i;

  for (i=0; i < c.len_; i++)
    val += data[i];

  return val + c.align_;
}

ULng32 nullTripleHashFunc(const NullTriple& o) {
  return (o.atp_ * 1000) + o.idx_ + o.off_;
}

ULng32 operandHashFunc(const PCodeOperand& o) {
  return (o.stackIndex_ * 1000) + o.offset_ + o.nullBitIndex_;
}

ULng32 collIndexHashFunc(const CollIndex & o) {
  return (ULng32)o;
}

ULng32 collIndexHashFunc2(const CollIndex & o) {
  return (ULng32)o;
}

// This function is used as built-in hash function for PCodeCfg::targets_
// which is instance of NAHashDictionary class template. Its input is the 
// key of the hash dictionary (a pointer to a sequence of PCodeBinaries)
// and it must return a 32-bit hash value required by the template, see
// common/Collections.h. Though the casting would truncate the upper 32-bit
// from the input on 64-bit platform, it wouldn't affect the quality of
// the hash value in most cases because the pcode binaries are normally
// stored in a confined area in the optimization phase.
ULng32 targetHashFunc(const ULong & o) {
  ULng32 v = (ULong)o;
  return v;
}

/*****************************************************************************
 * PCodeOperand
 *****************************************************************************/
Int32 PCodeOperand::getAlign()
{
  // Alignment of varchar depends on vc indicator length.
  if (isVarchar())
    return getVcIndicatorLen();

  switch (getType()) {
    case PCIT::MBIN8:
    case PCIT::MBIN8S:
    case PCIT::MBIN8U:
    case PCIT::MASCII:
      return 1;

    case PCIT::MBIN16U:
    case PCIT::MBIN16S:
    case PCIT::MUNI:
      return 2;

    case PCIT::MUNIV:
    case PCIT::MATTR5:
      // Fixed char or nchar only, since varchar handled earlier.  Because PCODE
      // instructions can sometimes support both nchar and char with same
      // instruction, better to assume worst-case alignment of 2.
      return 2;

    case PCIT::MFLT32:
    case PCIT::MBIN32U:
    case PCIT::MBIN32S:
      return 4;

    case PCIT::MFLT64:
    case PCIT::MBIN64S:
    case PCIT::MBIGS:
    case PCIT::MBIGU:
      return 8;

    case PCIT::MPTR32:
      // Assume worst case here.  TODO: Change MPTR32 declarations to a more
      // meaningful type, if possible, in PCodeCfg::loadOperandsOfInst(), so
      // that more accurate information could be tracked for each operand.
      return 8;
  }

  return -1;
}

NABoolean PCodeOperand::canOverlap(PCodeOperand* op2)
{
  CollIndex nbi, r1x, r2x, r1y, r2y;

  if (getBvIndex() == op2->getBvIndex())
    return TRUE;

  if (getStackIndex() != op2->getStackIndex())
    return FALSE;

  nbi = getNullBitIndex();

  r1x = (8*getOffset()) + ((nbi == -1) ? 0 : nbi);
  r1y = (getLen() == -1) ? r1x : r1x + (8*getLen());

  nbi = op2->getNullBitIndex();
  r2x = (8*op2->getOffset()) + ((nbi == -1) ? 0 : nbi);
  r2y = (op2->getLen() == -1) ? r2x : r2x + (8*op2->getLen());

  if (((r1x >= r2x) && (r1x < r2y)) ||
      ((r2x >= r1x) && (r2x < r1y)))
    return TRUE;

  return FALSE;
}


/*****************************************************************************
 * PCodeConstants
 *****************************************************************************/

NABoolean PCodeConstants::isQualifiedConstant(Int64 value)
{
  return ((value == 0) || (value == 1) || (value == -1));
}

void PCodeConstants::clearConstantVectors(CollIndex operandIndex,
                                          NABitVector& zeroes,
                                          NABitVector& ones,
                                          NABitVector& neg1)
{
  zeroes -= operandIndex;
  ones -= operandIndex;
  neg1 -= operandIndex;
}

NABoolean PCodeConstants::isAnyKnownConstant(CollIndex bvIndex,
                                             NABitVector& zeroes,
                                             NABitVector& ones,
                                             NABitVector& neg1)
{
  if ( zeroes.testBit(bvIndex) ) return TRUE;
  if ( ones.testBit(bvIndex) )   return TRUE;
  if ( neg1.testBit(bvIndex) )   return TRUE;

  return FALSE;
}

NABoolean PCodeConstants::canBeOne(CollIndex bvIndex, NABitVector& ones)
{
  return (ones.testBit(bvIndex));
}

NABoolean PCodeConstants::canBeZero(CollIndex bvIndex, NABitVector& zeroes)
{
  return (zeroes.testBit(bvIndex));
}

NABoolean PCodeConstants::canBeNeg1(CollIndex bvIndex, NABitVector& neg1)
{
  return (neg1.testBit(bvIndex));
}

//
// Get the constant associated with the specified index.  If no known constant
// is found, return UNKNOWN value.
//
Int32 PCodeConstants::getConstantValue(CollIndex bvIndex,
                                       NABitVector& zeroes,
                                       NABitVector& ones,
                                       NABitVector& neg1)
{
  NABoolean isZero = zeroes.testBit(bvIndex);
  NABoolean isOne = ones.testBit(bvIndex);
  NABoolean isNeg1 = neg1.testBit(bvIndex);

  if ( isZero )
  {
     if ( !isOne && !isNeg1 )
          return 0;
     else return UNKNOWN_CONSTANT ;
  }
  if ( isOne ) 
  {
     if ( !isNeg1 )
          return 1;
     else return UNKNOWN_CONSTANT ;
  }
  if ( isNeg1 )
     return -1;

  return UNKNOWN_CONSTANT;
}

//
// Copy known constants of one index into another.  Return the known constant
// if one is found. 
// 
Int32 PCodeConstants::copyConstantVectors(CollIndex bvIndexFrom,
                                          CollIndex bvIndexTo,
                                          NABitVector& zeroes,
                                          NABitVector& ones,
                                          NABitVector& neg1)
{
  NABoolean isZero, isOne, isNeg1;

  if ((isZero = zeroes.testBit(bvIndexFrom)))
    zeroes += bvIndexTo;

  if ((isOne = ones.testBit(bvIndexFrom)))
    ones += bvIndexTo;

  if ((isNeg1 = neg1.testBit(bvIndexFrom)))
    neg1 += bvIndexTo;

  if ( isZero )
  {
     if ( !isOne && !isNeg1 )
          return 0;
     else return UNKNOWN_CONSTANT ;
  }
  if ( isOne ) 
  {
     if ( !isNeg1 )
          return 1;
     else return UNKNOWN_CONSTANT ;
  }
  if ( isNeg1 )
     return -1;

  return PCodeConstants::UNKNOWN_CONSTANT;
}

//
// Merge known constants from the specified bitvectors. 
//
void PCodeConstants::mergeConstantVectors(NABitVector& newZeroes,
                                          NABitVector& newOnes,
                                          NABitVector& newNeg1,
                                          NABitVector& zeroes,
                                          NABitVector& ones,
                                          NABitVector& neg1)
{
  CollIndex i;

  NABitVector temp, temp2;

  // First copy target vector into temp vector
  temp = newZeroes;
  temp += newOnes;
  temp += newNeg1;

  // Next copy source vector into temp 2 vector.
  temp2 = zeroes;
  temp2 += ones;
  temp2 += neg1;

  // We need to determine what the maximum bit element is between the two
  // vectors - this is needed to ensure that all elements are checked if they
  // are set or not.
  CollIndex maxIndex = ((temp.getLastStaleBit() > temp2.getLastStaleBit()) ?
                       temp.getLastStaleBit() :
                       temp2.getLastStaleBit());

  // Loop through all elements that are included in the sets.
  for (i=0; i <= maxIndex; i++)
  {
    if (!temp.testBit(i)) {
      if (temp2.testBit(i))
        clearConstantVectors(i, zeroes, ones, neg1);
    }
    else {
      if (!temp2.testBit(i))
        clearConstantVectors(i, newZeroes, newOnes, newNeg1);
    }
  }

  // Include the source vector elements into the target vector
  newZeroes += zeroes;
  newOnes += ones;
  newNeg1 += neg1;
}

//
// Set the constant in the given vectors for the specified index
//
void PCodeConstants::setConstantInVectors(Int32 constant,
                                          CollIndex bvIndex,
                                          NABitVector& zeroes,
                                          NABitVector& ones,
                                          NABitVector& neg1)
{
  switch(constant) {
    case 0:
      zeroes += bvIndex;
      ones -= bvIndex;
      neg1 -= bvIndex;
      break;

    case 1:
      zeroes -= bvIndex;
      ones += bvIndex;
      neg1 -= bvIndex;
      break;

    case -1:
      zeroes -= bvIndex;
      ones -= bvIndex;
      neg1 += bvIndex;
      break;

    default:
      break;
      // Unknown constant
  }
}

/*****************************************************************************
 * ReachDefsTable
 *****************************************************************************/

//
// Copy all reaching defs from incoming table into hash table.  Hash table is
// assumed to be empty!
//
Int32
ReachDefsTable::copy(ReachDefsElem** from, NABoolean isInTab)
{
  CollIndex i, count = 0;
  ReachDefsElem** to = (isInTab) ? rDefIn_ : rDefOut_;

  for (i=0; i < size_; i++)
  {
    if (from[i] != NULL) {
      ReachDefsElem* node = new(heap_) ReachDefsElem(from[i]);
      ReachDefsElem* tNode = node;

      count++;

      // Walk through next pointers and replace them with a newer copy.
      while (tNode->next_) {
        tNode->next_ = new(heap_) ReachDefsElem(tNode->next_);
        tNode = tNode->next_;

        count++;
      }

      // Assign the head chain to the first node copied
      to[i] = node;
    }
  }

  return count;
}

//
// Merge all reaching defs from incoming table into hash table
//
Int32
ReachDefsTable::merge(ReachDefsElem** from, NABoolean isInTab)
{
  CollIndex i, count = 0;

  for (i=0; i < size_; i++)
  {
    if (from[i] != NULL) {
      ReachDefsElem* node = from[i];

      // Walk through chain and manually insert each reaching def
      while (node) {
        count += insert(node->bvIndex_, node->inst_, isInTab);
        node = node->next_;
      }
    }
  }

  return count;
}

//
// Find a reaching def in the hash table
//
PCodeInst*
ReachDefsTable::find(CollIndex bv, NABoolean isInTab)
{
  Int32 hashVal = bv % size_;
  ReachDefsElem** tab = (isInTab) ? rDefIn_ : rDefOut_;

  ReachDefsElem* node = tab[hashVal];

  if (node == NULL)
    return NULL;

  // Walk through chain in search for reaching def
  while (node) {
    if (node->bvIndex_ == bv)
      return node->inst_;

    node = node->next_;
  }

  return NULL;
}

//
// Find a reaching def in the hash tables.  Search Out table first, then In.
//
PCodeInst*
ReachDefsTable::find(CollIndex bv)
{
  PCodeInst* ret = find(bv, FALSE /* Out */);
  if (!ret)
    ret = find(bv, TRUE /* In */);

  return ret;
}

//
// Insert a new reaching def into the hash table.  Return false if entry
// already exists, and true otherwise.
//
NABoolean
ReachDefsTable::insert(CollIndex bv, PCodeInst* inst, NABoolean isInTab)
{
  Int32 hashVal = bv % size_;
  ReachDefsElem** tab = (isInTab) ? rDefIn_ : rDefOut_;

  ReachDefsElem* node = tab[hashVal];

  // Walk through chain to see if reaching def to insert already exists.
  while (node) {
    if (node->bvIndex_ == bv) {
      node->inst_ = inst;
      return FALSE;
    }
    node = node->next_;
  }

  // Insert new element at the beginning of the chain.
  node = new(heap_) ReachDefsElem(bv, inst);
  node->next_ = tab[hashVal];

  tab[hashVal] = node;

  return TRUE;
}

//
// Remove reaching def from table
//
void
ReachDefsTable::remove(CollIndex bv, NABoolean isInTab)
{
  Int32 hashVal = bv % size_;
  ReachDefsElem** tab = (isInTab) ? rDefIn_ : rDefOut_;

  ReachDefsElem* prev = NULL;
  ReachDefsElem* node = tab[hashVal];

  // Walk through chain in search for reaching def.  If found, first fix up next
  // pointers (paying special attention to first node), and then delete the node
  // to reclaim space.

  while (node) {
    if (node->bvIndex_ == bv) {
      if (prev == NULL)
        tab[hashVal] = node->next_;
      else
        prev->next_ = node->next_;

      NADELETEBASIC(node, heap_);
      return;
    }

    prev = node;
    node = node->next_;
  }
}

//
// Delete reaching def hash tables
//
void
ReachDefsTable::destroy()
{
  // Nothing to do if tables were already deallocated
  if (size_ == 0)
    return;

  for (CollIndex i=0; i < size_; i++)
  {
    ReachDefsElem* node;

    // Remove In table first
    node = rDefIn_[i];
    while (node) {
      ReachDefsElem* tNode = node->next_;
      NADELETEBASIC(node, heap_);
      node = tNode;
    }

    // Remove Out table next
    node = rDefOut_[i];
    while (node) {
      ReachDefsElem* tNode = node->next_;
      NADELETEBASIC(node, heap_);
      node = tNode;
    }
  }

  NADELETEBASIC(rDefIn_, heap_);
  NADELETEBASIC(rDefOut_, heap_);
  NADELETE(rDefVec_, NABitVector, heap_);

  size_ = 0;
}


/*****************************************************************************
 * PCodeBlock
 *****************************************************************************/

NABoolean PCodeBlock::isEntryBlock() {
  return (cfg_->getEntryBlock() == this);
}

void PCodeBlock::setToEntryBlock() {
  cfg_->setEntryBlock(this);
}

PCodeInst*
PCodeBlock::insertNewInstAfter(PCodeInst* inst, PCIT::Instruction instr)
{
  PCodeInst* newInst = cfg_->createInst(instr);
  return insertInstAfter(inst, newInst);
}

PCodeInst* PCodeBlock::insertInstAfter(PCodeInst* inst, PCodeInst* newInst)
{
  PCodeInst* temp;

  // If inst is NULL, that means you should add it after the tail inst;
  if (inst == NULL)
  {
    if (last_ != NULL)
      inst = last_;
    else
    {
      first_ = last_ = newInst;
      newInst->block = this;
      newInst->next = newInst->prev = NULL;
      return newInst;;
    }
  }

  // Make sure we're inserting the instruction in the right block
  assert (inst->block == this);

  // Also, never insert instruction between OPDATA/CLAUSE-EVAL sequence.  This
  // will break expression evaluation (which assumes this sequence pattern).
  // However, proceed if at the end of the block
  if (inst->isOpdata())
  {
    while ((inst != last_) && (inst->getOpcode() != PCIT::CLAUSE_EVAL))
      inst = inst->next;
  }

  // Fix pointers
  temp = inst->next;
  inst->next = newInst;
  newInst->next = temp;
  newInst->prev = inst;

  if (inst == last_)
    last_ = newInst;
  else
    temp->prev = newInst;

  // Set fields other than those defaulted by constructor
  newInst->block = this;

  return newInst;
}

PCodeInst*
PCodeBlock::insertNewInstBefore(PCodeInst* inst, PCIT::Instruction instr)
{
  PCodeInst* newInst = cfg_->createInst(instr);
  return insertInstBefore(inst, newInst);
}

PCodeInst*
PCodeBlock::insertInstBefore(PCodeInst* inst, PCodeInst* newInst)
{
  PCodeInst* temp;

  // If inst is NULL, that means you should add it before the head inst;
  if (inst == NULL)
  {
    if (first_ != NULL)
      inst = first_;
    else
      return insertInstAfter(inst, newInst);
  }

  // Make sure we're inserting the instruction in the right block
  assert (inst->block == this);

  // Also, never insert instruction between OPDATA/CLAUSE-EVAL sequence.  This
  // will break expression evaluation (which assumes this sequence pattern).
  // Proceed if we reach beginning of block
  if (inst->isOpdata() || inst->isClauseEval()) {
    while (inst->prev && inst->prev->isOpdata())
      inst = inst->prev;
  }

  // Fix pointers
  temp = inst->prev;
  inst->prev = newInst;
  newInst->prev = temp;
  newInst->next = inst;

  if (inst == first_)
    first_ = newInst;
  else
    temp->next = newInst;

  // Set fields other than those defaulted by constructor
  newInst->block = this;

  return newInst;
}

void PCodeBlock::deleteInst(PCodeInst* inst)
{
  // Make sure we're inserting the instruction in the right block
  assert (inst->block == this);

  // If we're deleting a clause eval, delete the opdata instrs too.
  if (inst->isClauseEval()) {
    while (inst->prev && inst->prev->isOpdata())
      deleteInst(inst->prev);
  }

  // Fix first_ and last_ pointers
  if (inst == first_)
  {
    if (first_ == last_)
      first_ = last_ = NULL;
    else {
      first_ = inst->next;
      first_->prev = NULL;
    }
  }
  else if (inst == last_) {
    last_ = inst->prev;
    last_->next = NULL;
  }
  else {
    inst->prev->next = inst->next;
    inst->next->prev = inst->prev;
  }
}

//
// Determine what constants are propagated when branching to the specified
// block.
//
void PCodeBlock::fixupConstantVectors(PCodeBlock* targetBlock,
                                      NABitVector& newZeroes,
                                      NABitVector& newOnes,
                                      NABitVector& newNeg1)
{
  // Don't process this block if it doesn't meet the requirements.
  if(isEmpty() || !last_->isBranch() || last_->isUncBranch())
    return;

  // Branches which both target and fall-through to the same block should not
  // be considered.
  if ((getSuccs().entries() >= 2) && (getSuccs()[0] == getSuccs()[1]))
    return;

  Int32 opc = last_->getOpcode();

  switch (opc) {
    case PCIT::BRANCH_AND:
    {
      CollIndex srcIdx = last_->getROps()[0]->getBvIndex();
      CollIndex writeIdx = last_->getWOps()[0]->getBvIndex();

      // If we branch to the target, both source and target must be zero.  Else,
      // they can't be zero.
      if (getTargetBlock() == targetBlock) {
        newOnes -= srcIdx;
        newNeg1 -= srcIdx;
        newOnes -= writeIdx;
        newNeg1 -= writeIdx;
        newZeroes += writeIdx;
        newZeroes += srcIdx;
      }
      else
      {
        assert (getFallThroughBlock() == targetBlock);
        newZeroes -= srcIdx;
        newZeroes -= writeIdx;
      }

      break;
    }

    case PCIT::BRANCH_OR:
    {
      CollIndex srcIdx = last_->getROps()[0]->getBvIndex();
      CollIndex writeIdx = last_->getWOps()[0]->getBvIndex();

      // If we branch to the target, both source and target must be one.  Else,
      // they can't be one.
      if (getTargetBlock() == targetBlock) {
        newZeroes -= srcIdx;
        newNeg1 -= srcIdx;
        newZeroes -= writeIdx;
        newNeg1 -= writeIdx;
        newOnes += writeIdx;
        newOnes += srcIdx;
      }
      else
      {
        assert (getFallThroughBlock() == targetBlock);
        newOnes -= srcIdx;
        newOnes -= writeIdx;
      }
      break;
    }

    case PCIT::NULL_BITMAP_BULK:
    case PCIT::NOT_NULL_BRANCH_BULK:
    {
      // If we branch to the target, all operands must be zero.
      if (getTargetBlock() == targetBlock) {
        for (CollIndex i=0; i < last_->getROps().entries(); i++) {
          CollIndex srcIdx = last_->getROps()[i]->getBvIndex();
          newOnes -= srcIdx;
          newNeg1 -= srcIdx;
          newZeroes += srcIdx;
        }
      }
      else
      {
        // If only 1 nullable column is tested, it's NULL in the fall through.
        // If this is NULL_BITMAP_BULK and all columns are aligned format,
        // then read operand is really the 2nd operand (since first op is the
        // null bit mask

        Int32 index = -1;

        if ((opc == PCIT::NOT_NULL_BRANCH_BULK) ||
            (opc == PCIT::NULL_BITMAP_BULK) && (last_->code[2] != 0)) {
          if (last_->getROps().entries() == 1)
            index = 0;
        }
        else if (last_->getROps().entries() == 2)
          index = 1;

        if (index != -1) {
          CollIndex srcIdx = last_->getROps()[index]->getBvIndex();
          PCodeConstants::setConstantInVectors(-1, srcIdx, newZeroes,
                                               newOnes, newNeg1);
        }
      }
      break;
    }

    case PCIT::NNB_MBIN32S_MATTR3_IBIN32S_IBIN32S:
    case PCIT::NNB_MATTR3_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN16S_IBIN32S:
    {
      CollIndex srcIdx = last_->getROps()[0]->getBvIndex();

      // If we branch to the target block, all read operands must be zero.
      if (getTargetBlock() == targetBlock) {
        newOnes -= srcIdx;
        newNeg1 -= srcIdx;
        newZeroes += srcIdx;
      }
      else
      {
        assert (getFallThroughBlock() == targetBlock);
        newZeroes -= srcIdx;
      }
      break;
    }

    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_IATTR3_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S:

    case PCIT::NNB_SPECIAL_NULLS_MBIN32S_MATTR3_MATTR3_IBIN32S_IBIN32S:

    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_MBIN16S_IBIN32S:
    {
      CollIndex writeIdx = last_->getWOps()[0]->getBvIndex();
      if (getTargetBlock() == targetBlock) {

        // If we branch to the target block, all read operands must be zero.
        for (CollIndex i=0; i < last_->getROps().entries(); i++) {
          CollIndex srcIdx = last_->getROps()[i]->getBvIndex();
          newOnes -= srcIdx;
          newNeg1 -= srcIdx;
          newZeroes += srcIdx;
        }

        // Also, target must be zero.     
        newOnes -= writeIdx;
        newNeg1 -= writeIdx;
        newZeroes += writeIdx;
      }
      else
      {
        assert (getFallThroughBlock() == targetBlock);

        // We can't determine the value of the result of NNB Special instr
        if (
            (PCIT::NNB_SPECIAL_NULLS_MBIN32S_MATTR3_MATTR3_IBIN32S_IBIN32S ==
             opc))
          break;

        // For only thouse NOT_NULL branches which have one read operand can
        // we definitively say that the operand can not be a zero, because in
        // the other cases the two operands are or'd, and we don't know which
        // one, if any, could be zero.
        if ((opc != PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_MBIN16S_IBIN32S) &&
            (opc != PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_MBIN16S_IBIN32S) &&
            (opc !=
             PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S) &&
            (opc !=
             PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S))
          newZeroes -= last_->getROps()[0]->getBvIndex();

        newZeroes -= writeIdx;
      }
      break;
    }

    default:
      return;
  }

  // For branches with only 1 read operand, see if we can derive some implicit
  // constants by searching backwards, based on the assumption we made for the
  // read operand of the branch
  if (last_->getROps().entries() == 1) {
    NABitVector tempBv(heap_);
    CollIndex i, propIdx = last_->getROps()[0]->getBvIndex();
  
    FOREACH_INST_IN_BLOCK_BACKWARDS_AT(this, inst, last_->prev) {
      if (inst->isMove()) {
        CollIndex moveSrcIdx = inst->getROps()[0]->getBvIndex();
        CollIndex moveWriteIdx = inst->getWOps()[0]->getBvIndex();

        if ((moveSrcIdx == propIdx) && (!tempBv.contains(moveWriteIdx))) {
          PCodeConstants::clearConstantVectors(moveWriteIdx,
                                               newZeroes, newOnes, newNeg1);
          PCodeConstants::copyConstantVectors(propIdx, moveWriteIdx,
                                              newZeroes, newOnes, newNeg1);
        }
        else if ((moveWriteIdx == propIdx) && (!tempBv.contains(moveSrcIdx))) {
          PCodeConstants::clearConstantVectors(moveSrcIdx,
                                               newZeroes, newOnes, newNeg1);
          PCodeConstants::copyConstantVectors(propIdx, moveSrcIdx,
                                              newZeroes, newOnes, newNeg1);

          // We have a new propagation index to follow
          propIdx = moveSrcIdx;
        }
      }
      else if ((inst->getOpcode() ==
               PCIT::NULL_TEST_MBIN32S_MATTR5_IBIN32S_IBIN32S) &&
               (propIdx == inst->getWOps()[0]->getBvIndex()))
      {
        CollIndex readIdx = inst->getROps()[0]->getBvIndex();
        CollIndex writeIdx = inst->getWOps()[0]->getBvIndex();
        Int32 constant = PCodeConstants::getConstantValue(propIdx, newZeroes,
                                                          newOnes, newNeg1);
        // Access bit which indicates test direction
        if ((constant == 1) && (inst->code[9] != 0))
          PCodeConstants::setConstantInVectors(-1, readIdx,
                                               newZeroes, newOnes, newNeg1);
        else if ((constant == 1) && (inst->code[9] == 0))
          PCodeConstants::setConstantInVectors(0, readIdx,
                                               newZeroes, newOnes, newNeg1);
        else if ((constant == 0) && (inst->code[9] != 0))
          PCodeConstants::setConstantInVectors(0, readIdx,
                                               newZeroes, newOnes, newNeg1);
        else if ((constant == 0) && (inst->code[9] == 0))
          PCodeConstants::setConstantInVectors(-1, readIdx,
                                               newZeroes, newOnes, newNeg1);
      }
          
      // Add all defines into temp bitvector
      for (i=0; i < inst->getWOps().entries(); i++)
        tempBv += inst->getWOps()[i]->getBvIndex();

      // If we've already now seen the def of the prop idx, break out
      if (tempBv.contains(propIdx))
        break;

    } ENDFE_INST_IN_BLOCK_BACKWARDS_AT
  }
}


/*****************************************************************************
 * PCodeInst
 *****************************************************************************/

NABoolean PCodeInst::isStringEqComp()
{
  Int32 opc = getOpcode();

  switch (opc) {
    case PCIT::EQ_MBIN32S_MASCII_MASCII:
      return TRUE;

    case PCIT::COMP_MBIN32S_MASCII_MASCII_IBIN32S_IBIN32S_IBIN32S:
      return (code[9] == ITM_EQUAL);

    case PCIT::COMP_MBIN32S_MATTR5_MATTR5_IBIN32S:
      return (code[13] == ITM_EQUAL);
  }

  return FALSE;
}

NABoolean PCodeInst::isIntEqComp()
{
  Int32 opc = getOpcode();

  switch (opc) {
    case PCIT::EQ_MBIN32S_MBIN8S_MBIN8S:
    case PCIT::EQ_MBIN32S_MBIN8U_MBIN8U:
    case PCIT::EQ_MBIN32S_MBIN16S_MBIN16S:
    case PCIT::EQ_MBIN32S_MBIN16S_MBIN32S:
    case PCIT::EQ_MBIN32S_MBIN32S_MBIN32S:
    case PCIT::EQ_MBIN32S_MBIN16U_MBIN16U:
    case PCIT::EQ_MBIN32S_MBIN16U_MBIN32U:
    case PCIT::EQ_MBIN32S_MBIN32U_MBIN32U:
    case PCIT::EQ_MBIN32S_MBIN16S_MBIN32U:
    case PCIT::EQ_MBIN32S_MBIN16U_MBIN16S:
    case PCIT::EQ_MBIN32S_MBIN64S_MBIN64S:
      return TRUE;
  }

  return FALSE;
}

NABoolean PCodeInst::isFloatEqComp()
{
  Int32 opc = getOpcode();

  switch (opc) {
    case PCIT::EQ_MBIN32S_MFLT64_MFLT64:
    case PCIT::EQ_MBIN32S_MFLT32_MFLT32:
      return TRUE;
  }

  return FALSE;
}

NABoolean PCodeInst::isEqComp()
{
  return (isStringEqComp() ||
          isIntEqComp() ||
          isFloatEqComp());
}

NABoolean PCodeInst::isRange()
{
  Int32 opc = getOpcode();

  switch (opc) {
    case PCIT::RANGE_LOW_S32S64:
    case PCIT::RANGE_HIGH_S32S64:
    case PCIT::RANGE_LOW_U32S64:
    case PCIT::RANGE_HIGH_U32S64:
    case PCIT::RANGE_LOW_S64S64:
    case PCIT::RANGE_HIGH_S64S64:
    case PCIT::RANGE_LOW_S16S64:
    case PCIT::RANGE_HIGH_S16S64:
    case PCIT::RANGE_LOW_U16S64:
    case PCIT::RANGE_HIGH_U16S64:
    case PCIT::RANGE_LOW_S8S64:
    case PCIT::RANGE_HIGH_S8S64:
    case PCIT::RANGE_LOW_U8S64:
    case PCIT::RANGE_HIGH_U8S64:
    case PCIT::RANGE_MFLT64:
      return TRUE;
  }

  return FALSE;
}

NABoolean PCodeInst::isEncode()
{
  Int32 opc = getOpcode();

  switch (opc) {
    case PCIT::ENCODE_MASCII_MBIN8S_IBIN32S:
    case PCIT::ENCODE_MASCII_MBIN8U_IBIN32S:
    case PCIT::ENCODE_MASCII_MBIN16S_IBIN32S:
    case PCIT::ENCODE_MASCII_MBIN16U_IBIN32S:
    case PCIT::ENCODE_MASCII_MBIN32S_IBIN32S:
    case PCIT::ENCODE_MASCII_MBIN32U_IBIN32S:
    case PCIT::ENCODE_MASCII_MBIN64S_IBIN32S:
    case PCIT::ENCODE_NXX:
      return TRUE;
  }

  return FALSE;
}

NABoolean PCodeInst::isDecode()
{
  Int32 opc = getOpcode();

  switch (opc) {
    case PCIT::DECODE_MASCII_MBIN8S_IBIN32S:
    case PCIT::DECODE_MASCII_MBIN8U_IBIN32S:
    case PCIT::DECODE_MASCII_MBIN16S_IBIN32S:
    case PCIT::DECODE_MASCII_MBIN16U_IBIN32S:
    case PCIT::DECODE_MASCII_MBIN32S_IBIN32S:
    case PCIT::DECODE_MASCII_MBIN32U_IBIN32S:
    case PCIT::DECODE_MASCII_MBIN64S_IBIN32S:
    case PCIT::DECODE_NXX:
      return TRUE;
  }

  return FALSE;
}

NABoolean PCodeInst::isOpdata()
{
  Int32 opc = getOpcode();

  switch (opc) {
    case PCIT::OPDATA_MPTR32_IBIN32S:
    case PCIT::OPDATA_MPTR32_IBIN32S_IBIN32S:
    case PCIT::OPDATA_MBIN16U_IBIN32S:
    case PCIT::OPDATA_MATTR5_IBIN32S:
      return TRUE;

    default:
      return FALSE;
  }

  return FALSE;
}

NABoolean PCodeInst::isNullBitmap()
{
  Int32 opc = getOpcode();

  switch (opc) {
    case PCIT::NULL_BITMAP:
    case PCIT::NULL_BITMAP_SET:
    case PCIT::NULL_BITMAP_CLEAR:
      return TRUE;

    default:
      return FALSE;
  }

  return FALSE;
}

NABoolean PCodeInst::isCopyMove()
{
  Int32 opc = getOpcode();

  switch (opc) {
    case PCIT::MOVE_MBIN8_MBIN8_IBIN32S:
    case PCIT::MOVE_MBIN16U_MBIN16U:
    case PCIT::MOVE_MBIN8_MBIN8:
    case PCIT::MOVE_MBIN32U_MBIN32U:
    case PCIT::MOVE_MBIN64S_MBIN64S:
      return TRUE;

    default:
      return FALSE;
  }

  return FALSE;
}

NABoolean PCodeInst::isMove()
{
  Int32 opc = getOpcode();

  switch (opc) {
    case PCIT::MOVE_MBIN8_MBIN8_IBIN32S:
    case PCIT::MOVE_MBIN16U_MBIN8:
    case PCIT::MOVE_MBIN32U_MBIN16U:
    case PCIT::MOVE_MBIN32S_MBIN16U:
    case PCIT::MOVE_MBIN64S_MBIN16U:
    case PCIT::MOVE_MBIN32U_MBIN16S:
    case PCIT::MOVE_MBIN32S_MBIN16S:
    case PCIT::MOVE_MBIN64S_MBIN16S:
    case PCIT::MOVE_MBIN64S_MBIN32U:
    case PCIT::MOVE_MBIN64S_MBIN32S:
    case PCIT::MOVE_MBIN16U_MBIN16U:
    case PCIT::MOVE_MBIN8_MBIN8:
    case PCIT::MOVE_MBIN32U_MBIN32U:
    case PCIT::MOVE_MBIN64S_MBIN64S:
    case PCIT::MOVE_MBIGS_MBIGS_IBIN32S_IBIN32S:
    case PCIT::MOVE_MBIGS_MBIN64S_IBIN32S:
    case PCIT::MOVE_MBIGS_MBIN32S_IBIN32S:
    case PCIT::MOVE_MBIGS_MBIN16S_IBIN32S:
    case PCIT::MOVE_MATTR5_MATTR5:
    case PCIT::MOVE_MATTR5_MASCII_IBIN32S:
    case PCIT::MOVE_MASCII_MATTR5_IBIN32S:
      return TRUE;

    default:
      return FALSE;
  }

  return FALSE;
}

NABoolean PCodeInst::isMoveToBignum()
{
  Int32 opc = getOpcode();

  switch (opc) {
    case PCIT::MOVE_MBIGS_MBIN64S_IBIN32S:
    case PCIT::MOVE_MBIGS_MBIN32S_IBIN32S:
    case PCIT::MOVE_MBIGS_MBIN16S_IBIN32S:
      return TRUE;

    default:
      return FALSE;
  }

  return FALSE;
}

NABoolean PCodeInst::isConstMove()
{
  Int32 opc = getOpcode();

  switch (opc) {
    case PCIT::MOVE_MBIN16U_IBIN16U:
    case PCIT::MOVE_MBIN32S_IBIN32S:
      return TRUE;

    default:
      return FALSE;
  }

  return FALSE;
}

NABoolean PCodeInst::isHash()
{
  Int32 opc = getOpcode();

  switch (opc) {
    case PCIT::HASH_MBIN32U_MBIN64_IBIN32S_IBIN32S:
    case PCIT::HASH_MBIN32U_MBIN32_IBIN32S_IBIN32S:
    case PCIT::HASH_MBIN32U_MBIN16_IBIN32S_IBIN32S:
    case PCIT::HASH_MBIN32U_MPTR32_IBIN32S_IBIN32S:
    // case PCIT::HASH_MBIN32U_MPTR32_IBIN32S_IBIN32S:
      return TRUE;

    default:
      return FALSE;
  }

  return FALSE;
}

NABoolean PCodeInst::isSwitch()
{
  Int32 opc = code[0];

  switch (opc) {
    case PCIT::SWITCH_MBIN32S_MBIN64S_MPTR32_IBIN32S_IBIN32S:
    case PCIT::SWITCH_MBIN32S_MATTR5_MPTR32_IBIN32S_IBIN32S:
      return TRUE;
  }

  return FALSE;
}

NABoolean PCodeInst::isInListSwitch()
{
  Int32 opc = code[0];

  switch (opc) {
    case PCIT::SWITCH_MBIN32S_MBIN64S_MPTR32_IBIN32S_IBIN32S:
      return (code[8] & 0x1);
    case PCIT::SWITCH_MBIN32S_MATTR5_MPTR32_IBIN32S_IBIN32S:
      return (code[11] & 0x1);
  }

  return FALSE;
}

NABoolean PCodeInst::isAnyLogicalBranch()
{   
  return (isLogicalBranch() || isLogicalCountedBranch());
}   

NABoolean PCodeInst::isLogicalBranch()
{
  Int32 opc = code[0];
  return ((opc == PCIT::BRANCH_AND) ||
          (opc == PCIT::BRANCH_OR));
}

NABoolean PCodeInst::isLogicalCountedBranch()
{
  Int32 opc = code[0];
  return ((opc == PCIT::BRANCH_AND_CNT) ||
          (opc == PCIT::BRANCH_OR_CNT));
}

NABoolean PCodeInst::isBranch()
{
  Int32 opc = code[0];

  switch (opc) {
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_IATTR3_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S:
    case PCIT::CLAUSE_BRANCH:
    case PCIT::NULL_BITMAP_BULK:
    case PCIT::NOT_NULL_BRANCH_BULK:
    case PCIT::NOT_NULL_BRANCH_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_MBIN16S_IBIN32S:
    case PCIT::BRANCH:
    case PCIT::BRANCH_AND:
    case PCIT::BRANCH_OR:
    case PCIT::BRANCH_AND_CNT:
    case PCIT::BRANCH_OR_CNT:
    case PCIT::NNB_SPECIAL_NULLS_MBIN32S_MATTR3_MATTR3_IBIN32S_IBIN32S:
    case PCIT::NNB_MBIN32S_MATTR3_IBIN32S_IBIN32S:
    case PCIT::NNB_MATTR3_IBIN32S:
      return TRUE;

    default:
      return FALSE;
  }

  return FALSE;
}

NABoolean PCodeInst::isVarcharNullBranch()
{
  Int32 opc = code[0];

  switch (opc) {
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_IATTR3_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S:
      return TRUE;
  }

  return FALSE;
}

NABoolean PCodeInst::isVarcharInstThatSupportsFixedCharOps()
{
  Int32 opc = code[0];

  switch (opc) {
    case PCIT::SUBSTR_MATTR5_MATTR5_MBIN32S_MBIN32S:
    case PCIT::GENFUNC_MATTR5_MATTR5_IBIN32S:
    case PCIT::GENFUNC_MATTR5_MATTR5_MBIN32S_IBIN32S:
    case PCIT::POS_MBIN32S_MATTR5_MATTR5:
    case PCIT::LIKE_MBIN32S_MATTR5_MATTR5_IBIN32S_IBIN32S:
    case PCIT::SWITCH_MBIN32S_MATTR5_MPTR32_IBIN32S_IBIN32S:
    case PCIT::CONCAT_MATTR5_MATTR5_MATTR5:
      return TRUE;
  }

  return FALSE;
}

NABoolean PCodeInst::isBulkNullBranch()
{
  Int32 opc = code[0];

  switch (opc) {
    case PCIT::NULL_BITMAP_BULK:
    case PCIT::NOT_NULL_BRANCH_BULK:
      return TRUE;
  }
  return FALSE;
}

NABoolean PCodeInst::isNullBranch()
{
  Int32 opc = code[0];

  switch (opc) {
    case PCIT::NULL_BITMAP_BULK:
    case PCIT::NOT_NULL_BRANCH_BULK:
    case PCIT::NOT_NULL_BRANCH_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_MBIN16S_IBIN32S:
    case PCIT::NNB_SPECIAL_NULLS_MBIN32S_MATTR3_MATTR3_IBIN32S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_IATTR3_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S:
    case PCIT::NNB_MBIN32S_MATTR3_IBIN32S_IBIN32S:
    case PCIT::NNB_MATTR3_IBIN32S:
      return TRUE;
  }

  return FALSE;
}

NABoolean PCodeInst::containsIndirectVarcharOperand()
{
  CollIndex i;

  for (i=0; i < readOps.entries(); i++)
    if (readOps[i]->getOffset() < 0)
      return TRUE;

  for (i=0; i < writeOps.entries(); i++)
    if (writeOps[i]->getOffset() < 0)
      return TRUE;

  return FALSE;
}

NABoolean PCodeInst::mayFailIfNull()
{
  Int32 opc = code[0];

  if (isRange())
    return TRUE;

  switch (opc) {
    case PCIT::NULL_VIOLATION_MPTR32_MPTR32_IPTR_IPTR:
      return TRUE;
  }

  return FALSE;
}

// 
// Return the addresses of branch target addresses stored in the pcode binaries
// for this instruction.
//
void PCodeInst::getBranchTargetOffsetPointers(NAList<PCodeBinary*>* targetOffPtrs,
                                              PCodeCfg* cfg)
{
  if (isIndirectBranch())
    assert (FALSE);

  PCodeBinary* pcode = code;
  Int32 opc = pcode[0];

  pcode++;

  switch (opc) {
    case PCIT::CLAUSE_BRANCH:
    case PCIT::BRANCH:
    case PCIT::BRANCH_AND:
    case PCIT::BRANCH_OR:
    case PCIT::BRANCH_AND_CNT:
    case PCIT::BRANCH_OR_CNT:
      targetOffPtrs->insert(&(pcode[0]));
      return;

    case PCIT::NOT_NULL_BRANCH_MBIN16S_IBIN32S:
      targetOffPtrs->insert(&(pcode[2]));
      return;

    case PCIT::NNB_MATTR3_IBIN32S:
      targetOffPtrs->insert(&(pcode[3]));
      return;

    case PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S:
      targetOffPtrs->insert(&(pcode[4]));
      return;

    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S_IBIN32S:
      targetOffPtrs->insert(&(pcode[5]));
      return;

    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_MBIN16S_IBIN32S:
    case PCIT::NNB_MBIN32S_MATTR3_IBIN32S_IBIN32S:
      targetOffPtrs->insert(&(pcode[6]));
      return;

    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_IATTR3_IBIN32S:
      targetOffPtrs->insert(&(pcode[7]));
      return;

    case PCIT::NNB_SPECIAL_NULLS_MBIN32S_MATTR3_MATTR3_IBIN32S_IBIN32S:
      targetOffPtrs->insert(&(pcode[9]));
      return;

    case PCIT::NULL_BITMAP_BULK:
    case PCIT::NOT_NULL_BRANCH_BULK:
      // Using the length, pcode[0], the branch target address is one off of 
      // that.  We subtract two because the array is relative to zero.
      targetOffPtrs->insert(&(pcode[pcode[0] - 2]));
      return;

    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
      targetOffPtrs->insert(&(pcode[10]));
      return;

    default:
      assert(FALSE);
  }

  return;
}

// 
// Return the addresses of branch target addresses stored in the jump table
// for this (switch) instruction.
//
void PCodeInst::getBranchTargetPointers(NAList<Long*>* targetOffPtrs,
                                              PCodeCfg* cfg)
{

  // Indirect branch must *always* be preceded by a SWITCH instruction 
  // in the same block. The SWITCH instruction contains the target
  // offsets that are used by the indirect branch.

  assert (prev &&
          prev->isSwitch() &&
          !prev->isInListSwitch());

  PCodeInst* switchInst = prev;

  Int32 size = 0;
  Int32* tab = NULL;
  Long* jumpTab = NULL;

  switch (switchInst->getOpcode()) {
    case PCIT::SWITCH_MBIN32S_MBIN64S_MPTR32_IBIN32S_IBIN32S:
    {
      size = switchInst->code[7];
      tab = (Int32*)cfg->getPtrConstValue(switchInst->getROps()[1]);
      break;
    }

    case PCIT::SWITCH_MBIN32S_MATTR5_MPTR32_IBIN32S_IBIN32S:
    {
      size = switchInst->code[10];
      tab = (Int32*)cfg->getPtrConstValue(switchInst->getROps()[1]);
      break;
    }
  }

  // Move table forward to start of jump table
  jumpTab = (Long*)&(tab[size << 1]);

  // Go through all elements in table, including default case (+ 1)
  for (Int32 i=0; i < size + 1; i++) {
    if (jumpTab[i] == PCodeCfg::INVALID_LONG)
      continue;

    targetOffPtrs->insert(&jumpTab[i]);
  }
}

// 
// Reset the operands for this instruction.
//
void PCodeInst::reloadOperands(PCodeCfg* cfg) {
  readOps.clear();
  writeOps.clear();
  cfg->loadOperandsOfInst(this);
}

//
// Generate move instruction to cast integer into 64-bit value
//
PCIT::Instruction PCodeInst::generateFloat64MoveOpc(PCIT::AddressingMode am)
{
  switch (am) {
    case PCIT::MFLT64:
      return PCIT::MOVE_MBIN64S_MBIN64S;
    case PCIT::MFLT32:
      return PCIT::MOVE_MFLT64_MFLT32;
    default:
      assert(FALSE);
  }

  return (PCIT::Instruction)-1;
}

//
// Generate move instruction to cast integer into 64-bit value
//
PCIT::Instruction PCodeInst::generateInt64MoveOpc(PCIT::AddressingMode am)
{
  switch (am) {
    case PCIT::MBIN16S:
      return PCIT::MOVE_MBIN64S_MBIN16S;
    case PCIT::MBIN16U:
      return PCIT::MOVE_MBIN64S_MBIN16U;
    case PCIT::MBIN32S:
      return PCIT::MOVE_MBIN64S_MBIN32S;
    case PCIT::MBIN32U:
      return PCIT::MOVE_MBIN64S_MBIN32U;
    case PCIT::MBIN64S:
      return PCIT::MOVE_MBIN64S_MBIN64S;
    default:
      assert(FALSE);
  }

  return (PCIT::Instruction)-1;
}

Int32 PCodeInst::generateCopyMoveOpc(PCIT::AddressingMode am)
{
  switch (am) {
    case PCIT::MBIN64S:
      return PCIT::MOVE_MBIN64S_MBIN64S;
    case PCIT::MBIN32U:
    case PCIT::MBIN32S:
      return PCIT::MOVE_MBIN32U_MBIN32U;
    case PCIT::MBIN16U:
    case PCIT::MBIN16S:
      return PCIT::MOVE_MBIN16U_MBIN16U;

    case PCIT::MFLT64:
      return PCIT::MOVE_MBIN64S_MBIN64S;
  }

  assert(FALSE);
  return -1;
}

//
// Fill in the pcode bytecode for a varchar instruction, given the operands
// passed in
//
void PCodeInst::setupPcodeForVarcharMove(PCodeOperand* tgt, PCodeOperand* src)
{
  UInt32 comboLen1 = 0, comboLen2 = 0;
  char* comboPtr1 = (char*)&comboLen1;
  char* comboPtr2 = (char*)&comboLen2;

  comboPtr1[0] = tgt->getVcNullIndicatorLen();
  comboPtr1[1] = tgt->getVcIndicatorLen();
  comboPtr2[0] = src->getVcNullIndicatorLen();
  comboPtr2[1] = src->getVcIndicatorLen();

  code[1] = tgt->getStackIndex();
  code[2] = tgt->getOffset();
  code[3] = tgt->getVoaOffset();
  code[4] = tgt->getVcMaxLen();
  code[5] = comboLen1;

  code[6] = src->getStackIndex();
  code[7] = src->getOffset();
  code[8] = src->getVoaOffset();
  code[9] = src->getVcMaxLen();
  code[10] = comboLen2;
}

//
// Replace a use operand in this instruction with another, by updating the pcode
// for this instruction.
//
void PCodeInst::replaceOperand(PCodeOperand* oldSrc, PCodeOperand* newSrc)
{
  NABoolean found = FALSE;

  // Replace the use source with move source in the pcode
  code[1+oldSrc->getStackIndexPos()] = newSrc->getStackIndex();
  code[1+oldSrc->getOffsetPos()] = newSrc->getOffset();

  if (oldSrc->isVarchar()) {
    if (newSrc->isVarchar()) {
      // Copy in voa first
      code[2+oldSrc->getOffsetPos()] = newSrc->getVoaOffset();

      // Copy in vc max length
      code[3+oldSrc->getOffsetPos()] = newSrc->getVcMaxLen();

      // Copy in vc/null indicator lens, paying careful attention to preserve
      // flags already in this instruction.
      char* comboPtr = (char*)(&code[4+oldSrc->getOffsetPos()]);
      comboPtr[0] = newSrc->getVcNullIndicatorLen();
      comboPtr[1] = newSrc->getVcIndicatorLen();
    }
    else {
      // Must be a fixed char if we fall through here.

      // Clear out various varchar fields
      code[2+oldSrc->getOffsetPos()] = -1;
      code[3+oldSrc->getOffsetPos()] = newSrc->getLen();

      // Copy in 0 for vc/null indicator lens, paying careful attention to
      // preserve flags already in this instruction.
      char* comboPtr = (char*)(&code[4+oldSrc->getOffsetPos()]);
      comboPtr[0] = 0;
      comboPtr[1] = 0;
    }
  }
}

#if 0
NABoolean PCodeInst::clauseOkayForConstFold(char *op_data[],
                                            CollHeap* heap)
{
  Int32 numOfReads, numOfWrites;
  assert (isClauseEval());

  // Clauses which must process nulls are not welcome.
  if (inst->mustProcessNulls())
    return FALSE;

  FOREACH_INST_IN_BLOCK_BACKWARDS_AT(block_, inst, this->prev) {
    if (!inst->isOpdata())
      break;

    if (inst->getROps().entries()) {
      if (!inst->getROps()[0]->isConstant())
        return FALSE;

      numOfReads++;
    }

  } ENDFE_INST_IN_BLOCK_BACKWARDS_AT
}
#endif

PCodeInst::~PCodeInst() {
  destroy();
}

void PCodeInst::destroy() {
  liveVector_.clear();
}


/*****************************************************************************
 * Main entry/exit points
 *****************************************************************************/

void PCodeCfg::runtimeOptimize()
{
  //
  // We can only enter runtime optimizations if the following criteria were met:
  //
  // 1. This pcode was previously processed for optimizations at compile-time
  // 2. Trigger count reached a limit
  // 3. This is a scan expression (restriction can be removed with new opts);
  //

  NABoolean enableOpt = TRUE;
  PCodeBinary * pCode = expr_->getPCodeBinary();
  PCodeBinary * lastOrigPCodeInst;
  Int32 debug = 0;

  assert (expr_->getType() == ex_expr::exp_SCAN_PRED);

#if defined(_DEBUG)
  if (getenv("PCODE_LLO_DEBUG"))
    debug = 1;

#endif // defined(_DEBUG)

  if (enableOpt) {
    // First translate pcode bytecode into PCodeInst objects
    createInsts(pCode);

    lastOrigPCodeInst = allInsts_->at(allInsts_->entries() - 1)->code;

    // Create the control flow graph out of PCodeInst objects
    createCfg();

    DUMP_PHASE("CFG [0]", debug, debug);

    // Unfortunately we need to compute reaching defs since predicate reordering
    // makes use of this info
    computeReachingDefs(0);

    // Attempt to reorder predicates.  If no change was done, enableOpt is 0
    enableOpt = reorderPredicates(FALSE);

    removeReachingDefs();

    DUMP_PHASE("REORDER [1]", debug, FALSE);

    // If for any reason we had to revert back to the original pcode graph,
    // we need to make sure and reset the last pcode instruction to END, since
    // creating the CFG requires us to rename that inst to a RETURN.  Also, if
    // no change was made, original pcode graph is used.
    if (!enableOpt || !layoutCode()) {
      lastOrigPCodeInst[0] = PCIT::END;
    }
  }
}

void PCodeCfg::optimize()
{
  PCodeBinary* pCode = expr_->getPCodeBinary();

  Int32 i;

  Int32 enableOpt = 1;
  Int32 optFlag = 1;
  Int32 debugSome = 0;
  Int32 debugAll = 0;
  NABoolean pcodeExprIsCacheable = TRUE ;
  NABoolean usingCachedPCodeExpr = FALSE ;
  NABoolean foundCachedPCodeExpr = FALSE ;

  NABoolean overlapFound = FALSE;
  NABoolean bulkNullGenerated = FALSE;

  // Initialize rewiring flags
  Int32 rewiringFlags = REMOVE_UNREACHABLE_BLOCKS |
                      REMOVE_TRAMPOLINE_CODE |
                      MERGE_BLOCKS |
                      REMOVE_EMPTY_BLOCKS;

#if defined(_DEBUG)
  //static unsigned int count = 0;
  //static unsigned int limit = 5000000; // count=69 is problem

  if (getenv("NO_PCODE_LLO"))
    enableOpt = 0;

  if (getenv("FULL_PCODE_LLO_DEBUG")) {
    debugSome = 1;
    debugAll = 1;
  }

  if (getenv("PCODE_LLO_DEBUG")) {
    debugSome = 1;
    debugAll = 0;
  }

  //if (++count > limit)
  //  return;
#endif // defined(_DEBUG)

  // If this pcode sequence can't be optimized, or if decided to disable opts,
  // return.

  UInt32 savedUnOptPCodeLen = 0 ;

  if ( !canPCodeBeOptimized(pCode , pcodeExprIsCacheable , savedUnOptPCodeLen ) ||
       !enableOpt )
  {
#if defined(_DEBUG)
   if ( debugSome )
     NExLog( "FOUND: PCODE EXPRESSION NOT EVEN OPTIMIZABLE\n" );
#endif // defined(_DEBUG)
    return;
  }

  // Set up optimization flags to enable/disable opts
  optFlags_ = expr_->getPCodeOptFlags();

  if ( optFlags_ & OPT_PCODE_CACHE_DISABLED )
     pcodeExprIsCacheable = FALSE ;

#if defined(_DEBUG)
  if ( debugSome )
  {
    if ( pcodeExprIsCacheable )
       NExLog( "FOUND: PCODE EXPRESSION IS CACHEABLE\n" );
    else
       NExLog( "FOUND: PCODE EXPRESSION IS NOT cacheable\n" );
  }
#endif // defined(_DEBUG)

  // Initialize counters
  initInstructionCounters();

  // First translate pcode bytecode into PCodeInst objects
  createInsts(pCode);

  // We currently can't deal with loops
  if (cfgHasLoop())
    return;

  // Create the control flow graph out of PCodeInst objects
  createCfg();

  // Initialize constant vectors and hash tables
  initConstants();

  // Initialize null mapping table
  initNullMapTable();

  CollIndex oldConstsAreaLen = expr_->getConstsLength(); // Save the old Consts Length

  // Now that we have the Constants needed for the unOptimized PCode ...

  PCodeBinary * savedUnOptPCodePtr = pCode ;
  PCodeBinary * optimizedPCode     = NULL  ;

  char        * cachedConstsArea   = NULL ;
  UInt32        cachedPCodeLen     = 0 ;
  UInt32        cachedNewConstsLen = 0 ;
  UInt32        cachedNEConstsLen  = 0 ;
  UInt32        cachedTempsLen     = 0 ;
  UInt32        origConstantsLen   = expr_->getConstsLength();
  UInt32        origTempsLen       = expr_->getTempsLength() ;

  if ( pcodeExprIsCacheable )
  {

#if OPT_PCC_DEBUG==1

     struct rusage begSrch;

     if ( CURROPTPCODECACHE->getPCECLoggingEnabled() )
     {
        if ( debugAll )
        {
           char NExBuf[100];
           sprintf( NExBuf, "PCODE EXPR cacheable - searching cache: ThisPtr=%p: oldLen=%d\n",
                                 CURROPTPCODECACHE->getThisPtr(), savedUnOptPCodeLen);
           NExLog(  NExBuf );
        }
        (void) getrusage( RUSAGE_THREAD, &begSrch );
     }
#endif // OPT_PCC_DEBUG==1

     PCECacheEntry * cachedPCE = NULL ;
     if ( cachedPCE = CURROPTPCODECACHE->findPCodeExprInCache(
                                           pCode
                                         , expr_->getConstantsArea()
                                         , optFlags_ & NATIVE_EXPR
                                         , savedUnOptPCodeLen
                                         , origConstantsLen
#if OPT_PCC_DEBUG==1
                                         , NExDbgInfoPtr_->getNExStmtSrc()
#endif // OPT_PCC_DEBUG==1
                                         ) )
     {
        foundCachedPCodeExpr = TRUE ;

        optimizedPCode       = cachedPCE->getOptPCptr();
        cachedPCodeLen       = cachedPCE->getOptPClen();
        cachedNewConstsLen   = cachedPCE->getOptConstsLen();
        cachedTempsLen       = cachedPCE->getTempsAreaLen();
        cachedConstsArea     = cachedPCE->getConstsArea();
        cachedNEConstsLen    = (optFlags_ & NATIVE_EXPR) ?
                                cachedPCE->getNEConstsLen()  :
                                cachedNewConstsLen           ;

        if ( (optFlags_ & EXPR_CACHE_CMP_ONLY) == 0 ) // If NOT in Compare-Only mode
           usingCachedPCodeExpr = TRUE ;

#if OPT_PCC_DEBUG==1

        if ( CURROPTPCODECACHE->getPCECLoggingEnabled() )
        {
           Int64 totalSearchTime = computeTimeUsed( begSrch.ru_utime ) ;
           CURROPTPCODECACHE->addToTotalSearchTime( totalSearchTime );
        }
#endif // OPT_PCC_DEBUG==1

     }

     if ( usingCachedPCodeExpr )
     {
#if OPT_PCC_DEBUG==1
        if ( debugSome )
        {
          char NExBuf1[80];
          sprintf( NExBuf1, "GETTING PCODE EXPRESSION FROM CACHE: UniqCtr=%ld\n", cachedPCE->getUniqCtr() );
          NExLog( NExBuf1 );
        }
        if ( debugAll )
        {
           char NExBuf[200];
           sprintf( NExBuf, "PCODE EXPR FOUND in cache: ThisPtr=%p: Loc=%p, oldLen=%d, newLen=%d, #Lookups=%ld, #Hits=%ld\n", CURROPTPCODECACHE->getThisPtr(), cachedPCE, savedUnOptPCodeLen, cachedPCodeLen, CURROPTPCODECACHE->getNumLookups(), CURROPTPCODECACHE->getNumHits() );
           NExLog(  NExBuf );
        }
#endif // OPT_PCC_DEBUG==1
        if ( ! space_ )
        {
          if ( cachedPCodeLen <= savedUnOptPCodeLen ) // If new can go over the old PCode
          {
            memcpy( pCode, optimizedPCode, sizeof(PCodeBinary) * cachedPCodeLen );
            optimizedPCode = pCode;
          }
          else { /* run with unoptimized PCODE and Constants */ }
        }
        else // Otherwise, we allocate space to hold the optimized code & constants
        {
           PCodeBinary * newPcode = new(space_) PCodeBinary[ cachedPCodeLen ];
           memcpy( newPcode, optimizedPCode, sizeof(PCodeBinary) * cachedPCodeLen );

           optimizedPCode = newPcode ;

           char * newConstsArea = expr_->getConstantsArea() ;
           if ( cachedNEConstsLen > 0 )
           {
              newConstsArea         = space_->allocateAlignedSpace( cachedNEConstsLen );
              char * constantsToUse = NULL ;

              if ( cachedNEConstsLen > oldConstsAreaLen )
              {
                 // We need ALL the constants in the PCEC entry.
                 // Note: If a match was found despite the oldConstantsLen NOT being
                 // the same as the cachedConstantsLen, it means the old constants
                 // were EXACTLY the same as the first oldConstantsLen bytes of the
                 // saved new constants.  Therefore, it doesn't matter which version
                 // of those constants we use.

                 constantsToUse = cachedConstsArea ;
              }
              else
              {
                 // If a match was found and the cachedConstantsLen == oldConstantsLen
                 // then we want to run with *this* expr_'s constants, not some
                 // cached version of the constants.

                 constantsToUse = expr_->getConstantsArea() ;
              }
              memcpy( newConstsArea, constantsToUse, cachedNEConstsLen );
           }

           expr_->setConstantsArea( newConstsArea     );
           expr_->setConstsLength(  cachedNEConstsLen );

           // Since Native Expressions are always put on 8-byte boundary:
           cachedNewConstsLen = ROUND8( cachedNewConstsLen ) ;

           if ( ( optFlags_ & NATIVE_EXPR ) &&
                ( cachedNewConstsLen < cachedNEConstsLen ) )
           {
              // Store offset into evalPtr_
              expr_->setEvalPtr((ex_expr::evalPtrType)((long)cachedNewConstsLen));

              // Mark this expression appropriately so that the native function gets called
              expr_->setPCodeMoveFastpath(TRUE);
              expr_->setPCodeNative(TRUE);
           }
        }

#if OPT_PCC_DEBUG==1
        if ( debugAll ) NExLog("DECIDED TO USE cached PCode Expr\n");
#endif // OPT_PCC_DEBUG==1

        // Update the expression's pcode object with the new pcode segment
        PCodeSegment* pcodeSegment = expr_->getPCodeSegment();
        pcodeSegment->setPCodeSegmentSize(sizeof(PCodeBinary) * cachedPCodeLen );
        pcodeSegment->setPCodeBinary( optimizedPCode );

        // TempsLength may have started out bigger on the current expr
        // than it did on the one we cached. So add it in just to be safe.
        //
        expr_->setTempsLength( cachedTempsLen + origTempsLen );
     }
     else if ( ! foundCachedPCodeExpr )
     {
#if OPT_PCC_DEBUG==1
        if ( debugSome )
           NExLog( "PCODE EXPRESSION NOT FOUND IN CACHE\n" );
#endif // OPT_PCC_DEBUG==1

        if (!space_) {
           // Must save a copy of the unoptimized pcode because orig might
           // be overwritten during layoutCode() ... but we still need the
           // unoptimized pcode for when we add to the PCode Expr Cache.
           //
           savedUnOptPCodePtr = new(heap_) PCodeBinary[ savedUnOptPCodeLen ];
           memcpy( savedUnOptPCodePtr, pCode, sizeof(PCodeBinary) * savedUnOptPCodeLen );
        }
     }
  }

  if ( usingCachedPCodeExpr )
     goto considerNativeCodeGen ;

#if defined(_DEBUG)
  if (debugSome) NExLog("-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n");
#endif // defined(_DEBUG)

#if OPT_PCC_DEBUG==1

  struct rusage begOpt;

  if ( pcodeExprIsCacheable && CURROPTPCODECACHE->getPCECLoggingEnabled() )
     (void) getrusage( RUSAGE_THREAD, &begOpt );

#endif // OPT_PCC_DEBUG==1

  DUMP_PHASE("CFG [0]", debugSome, debugAll);

  // Perform global constant propagation to get rid of stupid things
  // which may help later optimizations.
  if (optFlag) constantPropagation(FALSE /* Peeling */);

  // Flatten null branches if all they do is copy the null bit/value and
  // fill in the column in question with zero.  The assumption being that the
  // column value itself is not important if it is null.
  if (optFlag) flattenNullBranches();

  // Short-circuit optimization (not really short-circuiting here)
  if (optFlag) reorderPredicatesHelper();

  if (optFlag) cfgRewiring(rewiringFlags);

  DUMP_PHASE("Null Flattening [1]", debugSome, debugAll);

  // Global common-sub-expression elimination
  if (optFlags_ & CSE) {
    copyPropagation(2, FALSE);  // Forward prop only to get sub-exprs right
    globalCSE();
  }

  DUMP_PHASE("CSE [2]", debugSome, debugAll);

  // If operand overlap exists, properly remeber this for later marking
  if (optFlag && addOverlappingOperands(TRUE))
    overlapFound = TRUE;

  // Perform short circuiting
  if (optFlag && shortCircuitOpt(overlapFound))
    cfgRewiring(rewiringFlags);

  DUMP_PHASE("Short Circuiting [3]", debugSome, debugAll);

  // Don't perform bulk null branches if NATIVE_EXPR is enabled
  if (optFlag && (!(optFlags_ & NATIVE_EXPR))) {
    if (bulkNullBranch()) {
      bulkNullGenerated = TRUE;
    }
  }

  // Perform dead-code-elimination
  if (optFlag && !overlapFound) {
    deadCodeElimination();
  }

  if (optFlag) cfgRewiring(rewiringFlags);

  // Global constant propagation 
  if (optFlag) constantPropagation(TRUE);
  if (optFlag) cfgRewiring(rewiringFlags);

  DUMP_PHASE("Bulk / Short Circuit / Const Prop [5]", debugSome, debugAll);

  // Perform dead-code-elimination
  if (optFlag && !overlapFound) {
    deadCodeElimination();
  }

  if (optFlag) cfgRewiring(rewiringFlags);

  DUMP_PHASE("DCE [5]", debugAll, debugAll);

  // Backwards copy propagation
  if (optFlag) copyPropagation(1, FALSE);

  DUMP_PHASE("Copy Propagation [6]", debugAll, debugAll);

  // Forwards copy propagation
  if (optFlag) copyPropagation(2, FALSE);

  DUMP_PHASE("Copy Propagation [7]", debugAll, debugAll);

  // Peephole optimizations
  if (optFlag) peepholeConstants();

  DUMP_PHASE("After Peephole [8]", debugSome, debugAll);

  // Perform the in-list opt to try and reduce the graph further before doing
  // anything more aggressive.
  if (optFlag && (optFlags_ & INDIRECT_BRANCH) && counters_.logicalBranchCnt_) {
    // Remove duplicate blocks, as expected by in-list opt
    NABoolean rewire;
    if (bulkNullGenerated) {
      rewire = removeDuplicateBlocks(entryBlock_->getSuccs()[0]);
      rewire = removeDuplicateBlocks(entryBlock_->getSuccs()[1]) || rewire;
    }
    else
      rewire = removeDuplicateBlocks(entryBlock_);

    if (rewire)
      cfgRewiring(rewiringFlags);

    // Now perform DCE (if no overlap seen) and liveness analysis together.
    computeLiveness(!overlapFound /* DCE if overlap not found */);

    // NOW do the indirect branch opt (just the in-list opt)
    indirectBranchOpt(TRUE);
  }

  DUMP_PHASE("Indirect Branch 1 [9]", debugSome, debugAll);

  // Aggressively optimize while change seen - add sanity limit, however.
  for (i=0; optFlag && (i < 3); i++)
  {
    NABoolean restart = FALSE;
    if (optFlag)
      restart = constantPropagation(TRUE /* Peeling */);

    if (optFlag) cfgRewiring(rewiringFlags);

    // Global common-sub-expression elimination + efficient copy prop
    if (optFlags_ & CSE)
    {
      // Perform efficient copy prop using reaching defs.  Only do it once,
      // however, since it is expensive to compute reaching defs analysis.  Use
      // analysis for CSE algorithm as well for the first time through.

      if (i == 0) {
        computeReachingDefs(0);
        copyPropagation(2, TRUE);  // Forward prop only to get sub-exprs right
      }

      restart = globalCSE() || restart;

      if (i == 0)
        removeReachingDefs();
    }

    if (restart)
      DUMP_PHASE("Aggressive [9]", debugSome, debugAll);

    if (!restart)
      break;
  }

  DUMP_PHASE("Aggressive All [10]", debugAll, debugAll);

  // Disable Bulk Hash Optimizations after changing hash function to use Neo byte order
  // Revisit at some point.
  //
  // Bulk operations
  // if (optFlag && counters_.hashCombCnt_) {
  //   computeReachingDefs(0);
  //   bulkHashComb();
  //   removeReachingDefs();
  // }

  if (optFlag)
    constantFolding();

  DUMP_PHASE("CF [12]", debugAll, debugAll);

  if (optFlag) {
    NABoolean rewire = FALSE;
    if (bulkNullGenerated) {
      rewire = removeDuplicateBlocks(entryBlock_->getSuccs()[0]);
      rewire = removeDuplicateBlocks(entryBlock_->getSuccs()[1]) || rewire;
    }
    else 
      rewire = removeDuplicateBlocks(entryBlock_);
      
    if (optFlag && rewire)
      cfgRewiring(rewiringFlags);
  }

  DUMP_PHASE("Remove Duplicates [13]", debugAll, debugAll);

  // Find and insert overlapping operands so that we make sure they aren't 
  // deleted accidently.  Note, we do this later than sooner because operands
  // can be reloaded when an instruction is copied or modified, and since it
  // would be too expensive to find overlapping operands each time (so as to
  // ensure the modified operands are up-to-date) we only do it where it's
  // needed.
  if (optFlag && overlapFound) addOverlappingOperands(FALSE);

  // Perform dead-code-elimination
  if (optFlag) {
    deadCodeElimination();
  }

  if (optFlag) cfgRewiring(rewiringFlags);

  DUMP_PHASE("DCE [11]", debugSome, debugAll);

  if (optFlag && (!(optFlags_ & NATIVE_EXPR))) {
    codeMotion();
    bulkMove(TRUE /* same size moves */);
  }

  if (optFlag) cfgRewiring(rewiringFlags | LIVE_ANALYSIS_AVAILABLE);

  DUMP_PHASE("Bulk [14]", debugAll, debugAll);

  if (optFlag && (optFlags_ & INDIRECT_BRANCH) && counters_.logicalBranchCnt_) {
    computeLiveness(FALSE /* no DCE */);

    // Perform case statement opt first since it won't perturb liveness
    indirectBranchOpt(FALSE); // case stmts
    indirectBranchOpt(TRUE);  // in-lists
  }

  // Reorder predicates based on static estimation.  If the flag for this is
  // disabled, dynamic predicate reordering will not be done either
  if (optFlags_ & REORDER_PREDICATES) {
    if ((counters_.logicalBranchCnt_) &&
        (expr_->getType() == ex_expr::exp_SCAN_PRED))
    {
      computeReachingDefs(0);
      if (reorderPredicates(TRUE)) {
        DUMP_PHASE("Reordering Done [15]", debugSome, debugAll);
      }
      removeReachingDefs();
    }
  }

  // Mark exprs for inlining
  if (optFlags_ & INLINING)
    inlining();

  // Dump instructions back into pcode bytecode and store in expression
  layoutCode();

considerNativeCodeGen:

#if OPT_PCC_DEBUG==1

  UInt64 totalOptTime = 0 ;

  if ( pcodeExprIsCacheable && CURROPTPCODECACHE->getPCECLoggingEnabled() )
     totalOptTime = computeTimeUsed( begOpt.ru_utime ) ;

#endif // OPT_PCC_DEBUG==1

  Int32 constsLenAfterOpt = newConstsAreaLen_ ;

#if OPT_PCC_DEBUG==1
  UInt64 totalNEgenTime = 0;
#endif // OPT_PCC_DEBUG==1

  if ( ! usingCachedPCodeExpr )
  {
    Int32 debugNE = ( NExprDbgLvl_ >= VV_BD ) ; // Debugging Native Expr ?

    DUMP_PHASE("Before NE [15.5]", debugSome || debugNE , debugAll || debugNE );

    // Native code generation
    if ( optFlags_ & NATIVE_EXPR ) {

#if OPT_PCC_DEBUG==1
      struct rusage begNEtime;
      if ( CURROPTPCODECACHE->getPCECLoggingEnabled() )
         (void) getrusage( RUSAGE_THREAD, &begNEtime );
#endif // OPT_PCC_DEBUG==1

      cfgRewiring(rewiringFlags);
      computeLiveness(FALSE /* no DCE */);
      layoutNativeCode();

#if OPT_PCC_DEBUG==1
      if ( pcodeExprIsCacheable && CURROPTPCODECACHE->getPCECLoggingEnabled() )
         totalNEgenTime = computeTimeUsed( begNEtime.ru_utime ) ;

#endif // OPT_PCC_DEBUG==1
    }
    else
      expr_->setEvalPtr( (ex_expr::evalPtrType)( (CollIndex) 0 ) );//Ensure NULL!

  }

  if ( ! usingCachedPCodeExpr )
  {
    // Lay out any new constants back into the space object.
    layoutConstants();

    DUMP_PHASE("Layout [16]", debugSome, debugAll);

    if ( pcodeExprIsCacheable )
    {

#if OPT_PCC_DEBUG==1

      struct rusage begAdd;

      if ( CURROPTPCODECACHE->getPCECLoggingEnabled() )
         (void) getrusage( RUSAGE_THREAD, &begAdd );

#endif // OPT_PCC_DEBUG==1

      PCodeSegment* pcodeSegment = expr_->getPCodeSegment();
      PCodeBinary * newPCode = pcodeSegment->getPCodeBinary();
      UInt32     newPCodeLen = pcodeSegment->getPCodeSegmentSize() / sizeof(PCodeBinary);

      if ( foundCachedPCodeExpr ) // TRUE if we are in Compare-Only mode
      {
         // We have the newly generated optimized PCode and/or N.E.,
         // but we also found it was previously put in the PCode Expr Cache.
         Int32 fatals = 0;

         if ( cachedNewConstsLen !=  constsLenAfterOpt )               fatals |= 0x1;
         if ( cachedPCodeLen     !=  newPCodeLen )                     fatals |= 0x2;
         else if ( memcmp( optimizedPCode, newPCode,
                           sizeof(PCodeBinary) * newPCodeLen) !=0 )    fatals |= 0x4;

         if ( cachedNEConstsLen  !=  expr_->getConstsLength() )        fatals |= 0x8;
         else if (memcmp( cachedConstsArea, expr_->getConstantsArea()
                   , cachedNEConstsLen) != 0)                          fatals |= 0x10;
         assert ( fatals == 0 );

         //
         // Note: Assuming we didn't assert on the above, we will return and
         // use the optimized PCode and/or Native Expression we just generated. 
         //
      }
      else // It wasn't already in cache
      {
        CMPASSERT( expr_->getConstsLength()   >= constsLenAfterOpt );
        CMPASSERT( constsLenAfterOpt          >= oldConstsAreaLen );
        CMPASSERT( savedUnOptPCodePtr && newPCode );
        CMPASSERT( ( savedUnOptPCodeLen > 0 ) && ( newPCodeLen > 0 ) );

        NAHeap * cHeap = CURROPTPCODECACHE->getCacheHeapPtr() ;

        PCECacheEntry * newPCE = new(cHeap) PCECacheEntry( cHeap
                                            , savedUnOptPCodePtr
                                            , newPCode
                                            , expr_->getConstantsArea()
                                            , savedUnOptPCodeLen 
                                            , newPCodeLen
                                            , oldConstsAreaLen         // Before PC optimization
                                            , constsLenAfterOpt        // After PC optimization
                                            , expr_->getConstsLength() // Length with any Native Expr
                                            , expr_->getTempsLength()  // Length of TempsArea needed
#if OPT_PCC_DEBUG==1
                                            , totalOptTime
                                            , totalNEgenTime
                                            , CURROPTPCODECACHE->genNewUniqCtrVal()
#endif // OPT_PCC_DEBUG==1
                                            );

        CURROPTPCODECACHE->addPCodeExpr( newPCE
#if OPT_PCC_DEBUG==1
                                       , totalNEgenTime
                                       , begAdd.ru_utime
                                       , NExDbgInfoPtr_->getNExStmtSrc()
#endif // OPT_PCC_DEBUG==1
                                       );

#if OPT_PCC_DEBUG==1
        if ( debugSome )
        {
          char NExBuf7[80];
          sprintf( NExBuf7, "ADDING PCODE EXPRESSION TO CACHE: UniqCtr=%ld\n", newPCE->getUniqCtr() );
          NExLog( NExBuf7 );
        }
        if ( debugAll )
        {
           char NExBuf[200];
           sprintf( NExBuf, "CACHED PCODE EXPR: ThisPtr=%p: Loc=%p oldLen=%d, newLen=%d, numExprCached=%d, curSiz=%d, maxSiz=%d\n",
                    CURROPTPCODECACHE->getThisPtr(), newPCE,
                    savedUnOptPCodeLen, newPCodeLen, CURROPTPCODECACHE->getNumEntries(),
                    CURROPTPCODECACHE->getCurrSize(), CURROPTPCODECACHE->getMaxSize() );
           NExLog(  NExBuf );
        }
#endif // OPT_PCC_DEBUG==1
      }
    }
#if OPT_PCC_DEBUG==1
      else if ( debugSome )
           NExLog( "FINISHED OPTIMIZATION (and NE) ON PCODE EXPRESSION\n" );
#endif // OPT_PCC_DEBUG==1
  }
}

PCodeCfg::~PCodeCfg() {
  destroy();
}

void PCodeCfg::destroy()
{
  // If we're not dealing with runtime optimizations happening in EID, then
  // all memory can be quickly deallocated by deleting the main heap_, followed
  // by individual deletes of the member variables.
  //
  // All memory can be quickly deallocated by individual deletes of the
  // member variables, followed by deleting the main heap_.
  // Note that the order of the deletion, in particular the heap_ last, is
  // important.

  // All memory can be quickly deallocated by individual deletes of the
  // member variables, followed by deleting the main heap_.
  // Note that the order of the deletion, in particular the heap_ last, is
  // important.


  NADELETEBASIC(allBlocks_, origHeap_);
  NADELETEBASIC(allInsts_, origHeap_);
  NADELETEBASIC(zeroes_, origHeap_);
  NADELETEBASIC(ones_, origHeap_);
  NADELETEBASIC(neg1_, origHeap_);

  delete(heap_);
}

void PCodeCfg::generateShowPlan(PCodeBinary* pCode, Space* space)
{
  if (pCode == NULL)
    return;

  // TODO: Remove this after a clean build - constructor should be doing this
  zeroOffset_ = -1;

  createInsts(pCode);
  createCfg();

#if defined(_DEBUG)
  NExLog("-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n");
#endif // defined(_DEBUG)

  DUMP_PHASE("CFG [0]", TRUE, FALSE);

  // Need to reset last instruction to PCIT::END
  allInsts_->at(allInsts_->entries() - 1)->code[0] = PCIT::END;

  FOREACH_BLOCK(block, firstInst, lastInst, index)
  {
    CollIndex i;
    char buf[2048];
    NABoolean found = FALSE;

    // For showplans, we unfortuntely end up generating a block for just the
    // PCIT::END instruction.  This could be fixed, but the easiest solution
    // right now seems to be to exclude the block from being printed.
    if (block->getFirstInst() &&
        block->getFirstInst()->getOpcode() == PCIT::END)
      continue;

    str_sprintf(buf, "    [%d]  ", block->getBlockNum());

    // Write out preds for this block.
    for (i=0; i < block->getPreds().entries(); i++) {
      char tbuf[32];
      PCodeBlock* pred = block->getPreds()[i];
      PCodeInst* lastInst = pred->getLastInst();

      if (pred->branchesTo(block)) {
        if (!found)
          str_cat(buf, "(Preds: ", buf);

        found = TRUE;

        str_sprintf(tbuf, "%d ", pred->getBlockNum());

        // Make sure we don't overflow the buffer.  Really hate having to call
        // strlen all the time, but alternative is too hairy to code or equally
        // inefficient (e.g. calling allocate for space object each time).
        if ((strlen(buf) + strlen(tbuf)) >= 2048) {
          space->allocateAndCopyToAlignedSpace(buf, str_len(buf),sizeof(short));
          buf[0] = 0;
        }

        str_cat(buf, tbuf, buf);
      }
    }

    if (found)
      str_cat(buf, ")", buf);

    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

    // Write out each instruction for this block.
    FOREACH_INST_IN_BLOCK(block, inst) {
      char tbuf[32];
      PCodeBinary * code = inst->code;

      if (inst->getOpcode() == PCIT::END)
        continue;

      str_sprintf(buf, "    %s ",
        PCIT::instructionString(PCIT::Instruction(code[0])));

      for (Int32 j=0; j < PCode::getInstructionLength(code); j++)
      {
        if (j==0) str_sprintf(tbuf, "(%d) ", code[j]);
        else str_sprintf(tbuf, "%d ", code[j]);

        // Make sure we don't overflow the buffer.  Really hate having to call
        // strlen all the time, but alternative is too hairy to code or equally
        // inefficient (e.g. calling allocate for space object each time).
        if ((strlen(buf) + strlen(tbuf)) >= 2048) {
          space->allocateAndCopyToAlignedSpace(buf, str_len(buf),sizeof(short));
          buf[0] = 0;
        }

        str_cat(buf, tbuf, buf);
      }

      if (inst->isBranch() || inst->isIndirectBranch()) {
        BLOCKLIST succs(heap_);

        if (inst->isIndirectBranch())
          succs.insert(block->getSuccs());
        else
          succs.insert(block->getTargetBlock());

        for (i=0; i < succs.entries(); i++) {
          PCodeBlock* succ = succs[i];

          if (i == 0)
            str_sprintf(tbuf, " (Tgt: %d", succ->getBlockNum());
          else
            str_sprintf(tbuf, ", %d", succ->getBlockNum());

          // Make sure we don't overflow the buffer.  Really hate having to call
          // strlen all the time, but alternative is hairy to code or equally
          // inefficient (e.g. calling allocate for space object each time).
          if ((strlen(buf) + strlen(tbuf)) >= 2048) {
           space->allocateAndCopyToAlignedSpace(buf,str_len(buf),sizeof(short));
           buf[0] = 0;
          }
          str_cat(buf, tbuf, buf);
        }
        str_cat(buf, ")", buf);
      }

      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

    } ENDFE_INST_IN_BLOCK

    space->allocateAndCopyToAlignedSpace("\n", 1, sizeof(short));

  } ENDFE_BLOCK


  // Dump out the native expr assembly code
  if (expr_->getNEInShowplan())
    layoutNativeCode(space);
  else if (expr_->getPCodeNative()) {
    space->allocateAndCopyToAlignedSpace("Native Expression exists but is not being displayed.\n",
                                  sizeof("Native Expression exists but is not being displayed.\n"),
                                  sizeof(short) );
    }
}

/*****************************************************************************
 * PCodeCfg
 *****************************************************************************/

/*****************************************************************************
 * (Constants)
 *****************************************************************************/

//
// Update the CFG's constant vectors using the provided operand.  Return TRUE if
// the value of the operand qualifies as a constant worth tracking for constant
// propagation.  Otherwise return FALSE
//
NABoolean PCodeCfg::updateConstVectors(PCodeOperand* operand)
{
  Int64 value;

  // Make sure that we're passed in a constant operand
  assert (operand->isConst());

  // Special-case when dealing with empty strings.  Return TRUE so that the
  // constant values of accidentally overlapping operands are not perturbed.
  if (operand->isEmptyVarchar())
    return TRUE;

  CollIndex bvIndex = operand->getBvIndex();


  switch (operand->getType()) {
    case PCIT::MBIN16S:
    case PCIT::MBIN16U:
      // -O0 will cause overflow error if UInt16 operand has -1 value
      value = ((Int16)getIntConstValue(operand));
      break;
    case PCIT::MBIN32S:
    case PCIT::MBIN32U:
      // -O0 will cause overflow error if UInt32 operand has -1 value
      value = ((Int32)getIntConstValue(operand));
      break;
    case PCIT::MBIN64S:
      value = getIntConstValue(operand);
      break;

    default:
      return FALSE;
  }


  if (PCodeConstants::isQualifiedConstant(value)) {
    PCodeConstants::setConstantInVectors((Int32)value, bvIndex,
                                          *zeroes_, *ones_, *neg1_);
    return TRUE;
  }

  return FALSE;
}

//
// This routine initializes the CFG's constant vectors with those constant
// operands having values 0, 1, or -1.  It also tracks general constants using
// the constants hash tables.
//
void PCodeCfg::initConstants()
{
  CollIndex i, j;
  Int64 oldValue;

  // First initialize constant hash tables
  newConstsAreaLen_ = expr_->getConstsLength();

  constToOffsetMap_ = new(heap_) NAHashDictionary<PCodeConstants, CollIndex>
                            (&constHashFunc, 10, TRUE, heap_);

  offsetToConstMap_ = new(heap_) NAHashDictionary<CollIndex, PCodeConstants>
                            (&collIndexHashFunc, 10, TRUE, heap_);

  // Next add the zero constant
  zeroOffset_ = addNewIntConstant(0, 8);

  // Initialize the stack pointer to the constants area for this expression.
  char *stk = expr_->getConstantsArea();

  // Loop through all instructions and find constant operands. If that constant
  // operand is a 0, 1, or -1, initialize it in the init constant vector.
  for (i=0; i < allInsts_->entries(); i++)
  {
    PCodeInst* inst = allInsts_->at(i);

    for (j=0; j < inst->getROps().entries(); j++)
    {
      PCodeOperand* op = inst->getROps()[j];
      CollIndex bvIndex = op->getBvIndex();

      if (!op->isConst())
        continue;

      // Attempt to add (and track) constant
      CollIndex* offsetPtr = addConstant(op);

      // If offset pointer is NULL, the constant operand was added successfully.
      // Otherwise it wasn't added, and instead, a duplicate was found and
      // returned.  Replace the constant operand with the duplicate found.

      if (offsetPtr != NULL) {
        // Replace the offset of the constant operand in the pcode bytestream
        Int32 off = *offsetPtr;

        // If varchar, stored offset is null + vc ind lens.  Need to store
        // offset to start of actual string.
        if (op->isVarchar())
          off = off + op->getVcIndicatorLen() + op->getVcNullIndicatorLen();

        inst->code[1 + op->getOffsetPos()] = off;
        inst->reloadOperands(this);

        // Reset op variable to point to duplicated operand
        op = inst->getROps()[j];
      }

      // If a constant worth tracking was not found, mark vectors so that we
      // can later identify the operand as neither a 0, 1, or -1, and then
      // clear out appropriately.  This is done because constant operands are
      // sometimes referenced as different sizes - done for bulk moves, I
      // think.  In either event, if that's the case, it can represent more
      // than one value.  So we need to prevent propagating it's constant in
      // that case.

      if (!updateConstVectors(op)) {
        *zeroes_ += bvIndex;
        *ones_ += bvIndex;
        *neg1_ += bvIndex;
      }
    }
  }

  // Go through each operand in the init vectors.  If a known constant can't
  // be identified, clear out the operand in all the init vectors (i.e., we
  // can't say for certain what value this operand has).

  for (i=0; i <= maxBVIndex_; i++) {
    oldValue = PCodeConstants::getConstantValue(i, *zeroes_, *ones_, *neg1_);
    if (oldValue == PCodeConstants::UNKNOWN_CONSTANT)
      PCodeConstants::clearConstantVectors(i, *zeroes_, *ones_ ,*neg1_);
  }
}

//
// Add a new temporary to the temps area of the expression.
//
Int32 PCodeCfg::addTemp(Int32 size, Int32 alignment)
{
  Int32 tempAreaLen = expr_->getTempsLength();

  switch (alignment) {
    case 1:
      break;
    case 2:
      tempAreaLen = ((tempAreaLen + 1)/2)*2;
      break;
    case 4:
      tempAreaLen = ((tempAreaLen + 3)/4)*4;
      break;
    case 8:
      tempAreaLen = ((tempAreaLen + 7)/8)*8;
      break;
    default:
      assert(FALSE);
  }

  expr_->setTempsLength(tempAreaLen + size);

  return tempAreaLen;
}

//
// This routine is used to add the initial constants of an expression into a
// lookup hash table.  Not all types of constants will be tracked and
// maintained.  No constant-related optimizations can operate on constants that
// aren't tracked by the hash tables (except constant propagation with constant
// bit vectors).
//
// Return: off if constant was found in a different operand
//       : -1  otherwise
//
CollIndex* PCodeCfg::addConstant(PCodeOperand* op)
{
  PCodeConstants* constPtr;

  assert(op->isConst());

  // No operands with unknown lengths/alignment (unless they're varchars).
  if ((op->getOffset() == -1) ||
      (op->getAlign() == -1) ||
      (op->getLen() == -1) && !(op->isVarchar()))
    return NULL;

  char * stk = expr_->getConstantsArea();
  Int32 len = op->getLen();
  Int32 off = op->getOffset();

  void* data = (void*)(stk + off);

  // If the operand is a varchar, make sure to start the data pointer before
  // the null and vc indicator lengths.

  if (op->isVarchar()) {
    Int32 vcLen = op->getVcIndicatorLen();
    Int32 vcNullLen = op->getVcNullIndicatorLen();

    // Clear out bytes from end of string to max length.  This is done so that
    // duplicate string constants can be found with strcmp across max length of
    // the varchar.
    Int32 strLen = ((vcLen == 2) ? (Int32)(*((Int16*)(stk + off - vcLen)))
                                 : (Int32)(*((Int32*)(stk + off - vcLen))));

    // If space left over, zero it out
    if ( op->getVcMaxLen() > strLen ) //Ensure arg3 to memset is positive
       memset((char*)(stk + off + strLen), 0, op->getVcMaxLen() - strLen);

    data = (void*)((char *)data - vcLen - vcNullLen);
    off = op->getOffset() - vcLen - vcNullLen;
    len = op->getVcMaxLen() + vcLen + vcNullLen;
  }

  // Check if constant already recorded in hash table.
  constPtr = offsetToConstMap_->getFirstValue((CollIndex*)&off);
  if (constPtr) {
    // If constant record found overlaps with this operand, use the larger of
    // the two constant records.  Otherwise we're good and return NULL.
    if (constPtr->getLen() >= len)
      return NULL;

    offsetToConstMap_->remove((CollIndex*)&off);
    constToOffsetMap_->remove(constPtr);
  }

  // Check if another operand matches this constant
  PCodeConstants c(data, len, op->getAlign());
  CollIndex* offPtr = constToOffsetMap_->getFirstValue(&c);

  if (offPtr)
    return offPtr;

  // Add new constant to constants hash table

  constPtr = new(heap_) PCodeConstants(data, len, op->getAlign());
  offPtr = (CollIndex*) new(heap_) CollIndex;
  *offPtr = (CollIndex)off;
  constToOffsetMap_->insert(constPtr, offPtr);
  offsetToConstMap_->insert(offPtr, constPtr);

  return NULL;
}

//
// This routine is used to add any constant into the CFG's constants hash table.
// A pointer to the data, the length of the constant, and alignment is needed.
// If the data is a varchar, it has to 1) be a direct varchar and 2) the data
// pointer must point to the beginning of the null indicator length, so that the
// entire "varchar" is inserted.
//
// Note, the client is not responsible for providing a persistent pointer to
// the data, since the data is reallocated in this routine.  If the constant
// already exists, the offset for that constant will be returned.  Otherwise
// the constant is allocated in the hash table and a new offset is returned.
//
CollIndex* PCodeCfg::addConstant(void* data, Int32 len, Int32 alignment)
{
  PCodeConstants c(data, len, alignment);
  void* newData;

  CollIndex* off = constToOffsetMap_->getFirstValue(&c);
  if (off)
    return off;

  newData = new(heap_) char[len];
  memcpy(newData, data, len);

  switch (alignment) {
    case 1:
      break;
    case 2:
      newConstsAreaLen_ = ((newConstsAreaLen_ + 1)/2)*2;
      break;
    case 4:
      newConstsAreaLen_ = ((newConstsAreaLen_ + 3)/4)*4;
      break;
    case 8:
      newConstsAreaLen_ = ((newConstsAreaLen_ + 7)/8)*8;
      break;
  }

  // Add new constant to constants hash table
  PCodeConstants* constPtr = new(heap_) PCodeConstants(newData, len, alignment);
  CollIndex* offPtr = (CollIndex*) new(heap_) CollIndex;
  *offPtr = (CollIndex)newConstsAreaLen_;
  constToOffsetMap_->insert(constPtr, offPtr);
  offsetToConstMap_->insert(offPtr, constPtr);

  // Increment the length for the constants area
  newConstsAreaLen_ += len;

  return offPtr;
}

//
// The purpose of this routine is to layout a new constants area which will
// contain any new constants that were allocated during optimizations.  This
// routine does compact the area so as to only layout those constants which are
// used - that would require more analysis and more work.
//
void PCodeCfg::layoutConstants()
{
  CollIndex i;
  char * stk = expr_->getConstantsArea();
  CollIndex oldConstsAreaLen = expr_->getConstsLength();

  // First copy old constants area into new constants area. 
  char* newConstsArea = space_->allocateAlignedSpace(newConstsAreaLen_);
  str_cpy_all(newConstsArea, stk, oldConstsAreaLen);

  // If any constants were added during optimization ...
  if ( newConstsAreaLen_ > oldConstsAreaLen ) {
     PCodeConstants* key;
     CollIndex* value;
     NAHashDictionaryIterator<PCodeConstants, CollIndex> iter(*constToOffsetMap_);

     for (i=0; i < iter.entries(); i++) {
       iter.getNext(key, value);

       // If constant added goes beyond that in the constants area, add it
       if (*value >= oldConstsAreaLen) {
         str_cpy_all(newConstsArea + *value, (char*)key->getData(), key->getLen());
       }
     }
  }

  expr_->setConstantsArea(newConstsArea);
  expr_->setConstsLength(newConstsAreaLen_);
}


/*****************************************************************************
 * (Analysis)
 *****************************************************************************/

//
// Arrange blocks in post-order for backwards analysis
//
void PCodeCfg::getBlocksInDFO(BLOCKARRAY& list) {
  clearVisitedFlags();
  getBlocksInDFORecursive(list, entryBlock_);
  clearVisitedFlags();
}

void PCodeCfg::getBlocksInDFORecursive(BLOCKARRAY& list, PCodeBlock* block)
{
  CollIndex i;

  for (i=0; i < block->getSuccs().entries(); i++) {
    PCodeBlock* succ = block->getSuccs()[i];
    if (!succ->getVisitedFlag())
      getBlocksInDFORecursive(list, succ);
  }

  block->setVisitedFlag(TRUE);
  list.insertAt(list.entries(), block);
}

//
// Determine which operands are live at each instruction.  That means, live
// operands at point X indicates that there exists some path from X where the
// live operands is used by some instruction.  This analysis phase will also
// perform dead-code-elimination, if it is asked to do so.
//
void PCodeCfg::computeLiveness(NABoolean performDCE)
{
  CollIndex i, j;
  NABoolean changed;

  // Clear out liveVector

  FOREACH_BLOCK_FAST(block, firstInst, lastInst, inst) {
    block->getLiveVector().clear();
  } ENDFE_BLOCK_FAST

  // Algorithm:
  //
  // Loop over all blocks while at least one block experienced a change.  For
  // each block, we merge the live operands coming from the blocks successors.
  // Then, we visit each instruction backwards up the block and kill those live
  // operands which are defined in the block.  If the result live vector of 
  // operands at the top of the block does not equal what it was before, a 
  // change was made, implying that we need to revist all blocks again.
  //
  do
  {
    changed = FALSE;
    FOREACH_BLOCK_DFO(block, firstInst, lastInst, index)
    {
      NABitVector tempBv(heap_);

      // Set up OUT vector to begin with
      for (j=0; j < block->getSuccs().entries(); j++)
        tempBv += block->getSuccs()[j]->getLiveVector();

      FOREACH_INST_IN_BLOCK_BACKWARDS (block, inst)
      {
        // Attempt dead-code elimination, if that's being asked
        if (performDCE)
        {
          if (inst->isClauseEval())
          {
            NABoolean isLive = FALSE, writeFound = FALSE;
            PCodeInst* prev = inst->prev;

            // Search OPDATAs for write operand that is live.
            for (; prev && prev->isOpdata(); prev = prev->prev) {
              for (i=0; i < prev->getWOps().entries(); i++) {
                PCodeOperand* op = prev->getWOps()[i];
                if (!(op->isTemp() || op->isGarbage()) ||
                    tempBv.testBit(op->getBvIndex())) {
                  isLive = TRUE;
                  break;
                }
                writeFound = TRUE;
              }
            }

            // If no live write operand was found, attempt to delete instruction.
            if (writeFound && !isLive) {
              // If inst is removed, continue with inst before 1st OPDATA inst.
              if (removeOrRewriteDeadCodeInstruction(inst)) {
                inst = prev;
                RESTART_INST_IN_BLOCK_BACKWARDS;
                continue;
              }
            }
          }
          else
          {
            // Assume the inst is not live, unless there are no write operands.
            NABoolean isLive = (inst->getWOps().entries() > 0) ? FALSE : TRUE;
  
            for (i=0; i < inst->getWOps().entries(); i++) {
              PCodeOperand* op = inst->getWOps()[i];
              if (!(op->isTemp() || op->isGarbage()) ||
                  tempBv.testBit(op->getBvIndex()) ||
                  op->isEmptyVarchar()) {
                isLive = TRUE;
                break;
              }
            }

            if (!isLive)
              if (removeOrRewriteDeadCodeInstruction(inst))
                continue;
          }
        }

        //
        // Gather and propagate liveness information
        //

        const OPLIST& writes = inst->getWOps();
        const OPLIST& reads = inst->getROps();

        for (j=0; j < writes.entries(); j++) {
          CollIndex operandElem = writes[j]->getBvIndex();
          tempBv -= operandElem;
        }

        for (j=0; j < reads.entries(); j++) {
          CollIndex operandElem = reads[j]->getBvIndex();
          tempBv += operandElem;
        }

        if (tempBv != inst->liveVector_) {
          inst->liveVector_ = tempBv;
          changed = TRUE;
        }
      } ENDFE_BLOCK_BACKWARDS

      block->getLiveVector() = tempBv;
    } ENDFE_BLOCK_DFO
  } while (changed);
}

//
// Remove space used in computing reaching definitions analysis
//
void PCodeCfg::removeReachingDefs()
{
  FOREACH_BLOCK_FAST(block, firstInst, lastInst, inst) {
    if (block->reachingDefsTab_) {
      block->reachingDefsTab_ = NULL;
    }
  } ENDFE_BLOCK_FAST
  delete(rDefsHeap_);

  rDefsHeap_ = NULL;
}

//
// Update the reaching defs information to reflect a change resulting in the
// deletion of a write (and addition of a new write instruction).  The "index"
// passed in is the bit vector index of the write operand in question.  The
// argument "oldInst" represents the old (or soon to be deleted) instruction,
// and the "newInst" represents its replacement.  The "block" argument is the
// block whose reaching defs tables are being updated.
//
void PCodeCfg::updateReachingDefs(PCodeBlock* block,
                                  CollIndex bvIndex,
                                  PCodeInst* oldInst,
                                  PCodeInst* newInst)
{
  CollIndex i = 0;

  // Reaching defs must be available when this routine is called.
  if (rDefsHeap_ == NULL)
    return;

  // Get the reaching def table for this block.
  ReachDefsTable* rDefTab = block->reachingDefsTab_;

/* ??? what is this???
  if (i == 0) {
    // assert(FALSE);
    return;
  }
*/

  // If oldInst for bvIndex is found in OUT table of block, update it.
  if (rDefTab->find(bvIndex, FALSE /* Out */) == oldInst) {
    // Insert index into OUT table (will replace old def).
    rDefTab->insert(bvIndex, newInst, FALSE /* Out */); 
  }

  // Check if oldInst for bvIndex is found in each IN table of successors.
  for (i=0; i < block->getSuccs().entries(); i++) {
    ReachDefsTable* succRDefTab = block->getSuccs()[i]->reachingDefsTab_;

    // For any found, insert index into their IN tables, and then recursively
    // call updateReachingDefs() for each successor found.
    if (succRDefTab->find(bvIndex, TRUE /* In */) == oldInst) {
      succRDefTab->insert(bvIndex, newInst, TRUE /* In */); 
      updateReachingDefs(block->getSuccs()[i], bvIndex, oldInst, newInst);
    }
  }
}

//
// Compute a reaching definitions analysis
//
void PCodeCfg::computeReachingDefs(Int32 flags)
{
  char NExBuf[2048];
  CollIndex i, j;
  CollIndex numOfBlocks = 0;
  NABoolean flag = FALSE;

  NABoolean noTemps = flags & 0x1;

  // Allocate reach defs memory in separate heap for faster destruction.
  rDefsHeap_ = new(heap_) NAHeap("Pcode Rdef", (NAHeap*)heap_, (Lng32)32768);

  // Initialize reaching defs table for each live block
  FOREACH_BLOCK_REV_DFO(block, firstInst, lastInst, index) {
    block->reachingDefsTab_ = new(rDefsHeap_) ReachDefsTable(rDefsHeap_, 1024);
  } ENDFE_BLOCK_REV_DFO

  //
  // Walk through each block (top to bottom) and identify and merge in all
  // definitions that reach.  Each block maintains a reaching defs table.  That
  // table consists of two "hash" tables (one for the In set, and the other for
  // the Out set).  The In set maintains all unique reaching defs flowing into
  // the block.  The Out set consists of new defs discovered in the block
  // itself.  The Out set represents a list of differential definitions - to
  // accurately get the Out set contents, a merge must be done from the In set
  // and the Out set.  The Out set was designed this way so as to save in space.
  //
  FOREACH_BLOCK_REV_DFO(block, firstInst, lastInst, index)
  {
    Int32 inEntries = 0;
    Int32 outEntries = 0;

    // Get the reaching def objects for this block.
    ReachDefsTable* rDefTab     = block->reachingDefsTab_;
    NABitVector* reachBits      = rDefTab->getRDefVec();

    // If this block has a pred, incoming reaching defs need to be merged in.
    if (block->getPreds().entries() > 0)
    {
      // Get the reaching def objects for the pred block.
      PCodeBlock* pred            = block->getPreds()[0];
      ReachDefsTable* predRDefTab = pred->reachingDefsTab_;

      // Initialize reaching def vector with that from the first predecessor
      *(rDefTab->getRDefVec()) = *(pred->reachingDefsTab_->getRDefVec());

      // Bring in reaching defs from the first predecessor.  This is done by
      // copying in the In set, and then merging in the Out set (since Out set
      // contains differential list of reaching defs, and must be accounted for)

      inEntries += rDefTab->copy(predRDefTab->getRDefIn(), TRUE /* In */);
      inEntries += rDefTab->merge(predRDefTab->getRDefOut(), TRUE /* In */);

      // Now bring in the reaching defs from the other predecessors
      for (i=1; i < block->getPreds().entries(); i++)
      {
        // Get the reaching def objects for the pred block.
        pred = block->getPreds()[i];
        predRDefTab = pred->reachingDefsTab_;

        // Use the reaching defs bit vector to quickly search for defs that
        // reach in this block.  Search for each of these operands in the pred's
        // reaching defs tables to find a match.  If there isn't a match, then
        // we must remove it from this block's reaching defs table.

        for (j = reachBits->getLastStaleBit();
             (j = reachBits->prevUsed(j)) != NULL_COLL_INDEX; j--)
        {
          PCodeInst* prevInst;

          // Look in Out set first.  If not found, search in In set.  Again,
          // this is done because the Out set is a differentials list, that
          // requires the In set to complete it.

          prevInst = predRDefTab->find(j, FALSE /* Out */);
          if (!prevInst)
            prevInst = predRDefTab->find(j, TRUE /* In */);

          // If reach def was found in the pred, and it has the same definition
          // as that already available in this blocks In set, then continue.
          if ((prevInst != NULL) && (prevInst == rDefTab->find(j, TRUE)))
            continue;

          // Remove reaching def from this block
          *reachBits -= j;
          rDefTab->remove(j, TRUE /* In */);

          inEntries--;
        }
      }
    }
    else if (entryBlock_ == block)
    {
      PCodeOperand* key;
      CollIndex* value;
      NAHashDictionaryIterator<PCodeOperand, CollIndex>
        iter(*operandToIndexMap_);

      // For an entry block, all variables reach it.
      for (i=0; i < iter.entries(); i++) {
        iter.getNext(key, value);
        if (key->isVar()) {
          *reachBits += *value;  

          // All defs live upon entry will have a special def instruction of -1.
          rDefTab->insert(*value, (PCodeInst*)-1, TRUE /* In */);
          inEntries++;
        }
      }

      numOfBlocks = index;
    }

    // Each definition in the block kills the definition in the reaching
    // defs set and creates a new one at the given instruction.
    FOREACH_INST_IN_BLOCK(block, inst) {
      for (i=0; i < inst->getWOps().entries(); i++) {
        PCodeOperand* value = inst->getWOps()[i];
        CollIndex bv = value->getBvIndex();

        if (noTemps && value->isTemp())
          continue;

        outEntries += rDefTab->insert(bv, inst, FALSE /* Out */); 
        *reachBits += bv;
      }
    } ENDFE_INST_IN_BLOCK

    if (flag)
    {
      sprintf( NExBuf, "Blk: %d, Index: %d, In: %d, Out: %d\n",
        block->getBlockNum(), index, inEntries, outEntries);
      NExLog(  NExBuf );
    }

  } ENDFE_BLOCK_REV_DFO

}
/*****************************************************************************
 * Phase 3 (Layout Code)
 *****************************************************************************/

//
// Recursive helper routine that inserts blocks into a list based on their
// physical order in the code layout.
//
void PCodeCfg::createPhysListRecurse (PCodeBlock* block, BLOCKLIST& list)
{
  CollIndex i;
  PCodeInst* lastInst;

  // Conditions
  //
  // 1) A block can have at most 1 fall-through predecessor
  // 2) A block can have at most 2 successors
  // 3) No loops


  // If this block has been visited already, move on.
  if (block->getVisitedFlag() || block->isEmpty())
    return;

  // If the block we're trying to insert has a predecessor who falls-through to
  // it, *but* that predecessor has not been laid out yet, we can't insert
  // this block just yet
  for (i=0; i < block->getPreds().entries(); i++) {
    PCodeBlock* pred = block->getPreds()[i];
    if (pred->getVisitedFlag() ||
        (pred->getFallThroughBlock() != block))
      continue;

    // We don't care about unconditional branches which "fall-through".  They
    // can be scheduled whenever, however, with no restraints.
    lastInst = pred->getLastInst();
    if (lastInst && lastInst->isUncBranch())
      continue;

    // Otherwise we need to wait until this pred gets laid out.
    return;
  }

  // We can now safely insert this block into the phys order list.
  block->setVisitedFlag(TRUE);

  // Only insert block if it's not dead code
  if ((block->getPreds().entries() != 0) || (block == entryBlock_))
    list.insert(block);

  for (CollIndex i=0; i < block->getSuccs().entries(); i++)
  {
    lastInst = block->getLastInst();

    // Only recurse if we're dealing with the fall-through or if the block
    // we're branching to has no other predecessor.
    if (((i == 0) && !lastInst->isUncBranch()) ||
        (block->getSuccs()[i]->getPreds().entries() == 1)) {
      createPhysListRecurse(block->getSuccs()[i], list);
    }
  }
}

//
// Create physical order of blocks used for laying out code
//
void PCodeCfg::createPhysList (BLOCKLIST& list)
{
  // Need visited flags for this algorithm
  clearVisitedFlags();

  // Create physical order by starting with the entry block
  createPhysListRecurse(entryBlock_, list);

  // Recurse through all blocks to make sure they're all physically ordered
  FOREACH_BLOCK(block, firstInst, lastInst, index) {
    createPhysListRecurse(block, list);
  } ENDFE_BLOCK

  // As a sanity check, make sure all blocks have been visited and inserted
  FOREACH_BLOCK(block, firstInst, lastInst, index) {
    if (!block->isEmpty())
      assert(block->getVisitedFlag());
  } ENDFE_BLOCK

  // If we branch to the fall through block, we should get rid of the branch.
  for (CollIndex i=1; i < list.entries(); i++) {
    PCodeBlock* prevBlock = list[i-1];
    PCodeInst* prevLastInst = prevBlock->getLastInst();
    if (prevLastInst->isUncBranch() && prevBlock->getTargetBlock() == list[i])
      prevBlock->deleteInst(prevLastInst);
  }
}

//
// Given a list of blocks in physical layout order, assign block offsets to
// each block and use them to correctly point branches to their targets
//
void PCodeCfg::assignBlockOffsetsAndFixup(BLOCKLIST& blockList)
{
  PCodeInst* inst;
  PCodeBlock* block;
  CollIndex i, j;
  Int32 offset = 0;

  // Assign block offsets first
  for (i=0; i < blockList.entries(); i++)
  {
    block = blockList[i];

    // Set the pcode byte stream offset of the beginning of this block to
    // offset. 
    block->setBlockTopOffset(offset);

    // Walk through each instruction and increment offset with the length of
    // each instruction.
    for (inst = block->getFirstInst(); inst; inst = inst->next)
    {
      Int32 instrOffset = offset;

      offset += PCode::getInstructionLength(inst->code);
      if (inst == block->getLastInst())
      {
        // Set the pcode byte stream offset of the end of this block to offset.
        block->setBlockBottomOffset(instrOffset);
        break;
      }
    }
  }

  // Use block offsets to correctly specify branch targets

  NAList<PCodeBinary*> targetOffsetPtrs(heap_);
  NAList<Long*> targetPtrs(heap_);

  for (i=0; i < blockList.entries(); i++)
  {
    block = blockList[i];
    inst = block->getLastInst();

    if (inst && inst->isBranch())
    {
      targetOffsetPtrs.clear();
      inst->getBranchTargetOffsetPointers(&targetOffsetPtrs, this);

      for (j=0; j < targetOffsetPtrs.entries(); j++) {
        Int32 targetOffset;

        PCodeBlock* succ = block->getTargetBlock();

        // If the target block's beginning offset is greater than the block's
        // offset, then we're dealing with a backwards branch.  Else it's a 
        // forward's branch.
        if (succ->getBlockTopOffset() < block->getBlockTopOffset())
        {
          // Subtract the offsets (including one) to get the location of the
          // target block.
          targetOffset = succ->getBlockTopOffset() -
                           block->getBlockBottomOffset() - 1;
        }
        else
        {
          // Branching forwards

          // Get the starting offset of the branch instruction by subtracting
          // the size of the branch inst from the block offset of the next
          // physical block.  Distance to the tgt block then is the offset of
          // that block minus the offset of the source inst minus 1 -> the
          // minus 1 is done since the pcode evaluation of the inst happens
          // after first advancing the pointer passed the opcode.

          PCodeBlock* nextBlock = blockList[i+1];
          targetOffset = succ->getBlockTopOffset() -
                           (nextBlock->getBlockTopOffset() -
                             PCode::getInstructionLength(inst->code)) - 1;
        }

        // Null branch PCode Binaries do not contail address operands
        if (inst->isAnyLogicalBranch())
          *((Long*)(targetOffsetPtrs[j])) = targetOffset;
        else
          *(targetOffsetPtrs[j]) = (Int32)targetOffset;
      }
    }
    else if (inst && inst->isIndirectBranch())
    {
      targetPtrs.clear();
      inst->getBranchTargetPointers(&targetPtrs, this);

      for (j=0; j < targetPtrs.entries(); j++) {
        Long targetOffset;

        PCodeBlock* succ = (inst->isBranch() ? block->getTargetBlock()
                             : block->getSuccs()[j]);

        // If the target block's beginning offset is greater than the block's
        // offset, then we're dealing with a backwards branch.  Else it's a 
        // forward's branch.
        if (succ->getBlockTopOffset() < block->getBlockTopOffset())
        {
          // Subtract the offsets (including one) to get the location of the
          // target block.
          targetOffset = succ->getBlockTopOffset() -
                           block->getBlockBottomOffset() - 1;
        }
        else
        {
          // Branching forwards

          // Get the starting offset of the branch instruction by subtracting
          // the size of the branch inst from the block offset of the next
          // physical block.  Distance to the tgt block then is the offset of
          // that block minus the offset of the source inst minus 1 -> the
          // minus 1 is done since the pcode evaluation of the inst happens
          // after first advancing the pointer passed the opcode.

          PCodeBlock* nextBlock = blockList[i+1];
          targetOffset = succ->getBlockTopOffset() -
                           (nextBlock->getBlockTopOffset() -
                             PCode::getInstructionLength(inst->code)) - 1;
        }

        *(targetPtrs[j]) = targetOffset;
      }
    }
  }
}

//
// Write the new optimized pcode instructions back into the space object for
// the expression
//
NABoolean PCodeCfg::layoutCode()
{
  Int32 totalLength;
  PCodeBinary *currPointer, *pcode, *newPcode;
  CollIndex i;

  // Create the list of basic blocks in physical layout order
  BLOCKLIST physBlockList(heap_);

  createPhysList(physBlockList);

  // Get the original pcode bytestream
  pcode = expr_->getPCodeBinary();

  // Initialize totalLength to be the number of ATPs passed in.
  totalLength = (2 * *(pcode)) + 1;

  // Determine the total length of the new pcode graph.  This is needed first
  // so proper allocation can be done for generating the new pcode segment
  for (i=0; i < physBlockList.entries(); i++) {
    PCodeBlock* block = physBlockList[i];
    FOREACH_INST_IN_BLOCK(block, inst) {
      totalLength += PCode::getInstructionLength(inst->code);
    } ENDFE_INST_IN_BLOCK
  }

  // Add one more word for the PCIT::END opcode at the end.
  totalLength += 1;

  // If a space pointer exists, allocate space for the new pcode bytestream in
  // it.  Otherwise attempt to copy the new bytestream into the existing space
  // allocated for (and consumed by) the old bytestream.
  if (space_) {
    newPcode = new(space_) PCodeBinary[totalLength + 1];
  }
  else {
    // The size of the existing pcode bytestream space must be big enough to
    // hold the new pcode bytestream.
    if ((Int32)(totalLength*sizeof(PCodeBinary)) >
        (Int32)expr_->getPCodeSegment()->getPCodeSegmentSize())
      return FALSE;

    newPcode = new(heap_) PCodeBinary[totalLength];
  }

  // Finalize fixup of blocks
  assignBlockOffsetsAndFixup(physBlockList);

  // Initialize current pointer
  currPointer = newPcode;

  // First copy over header information and then adjust current pointer
  memcpy(currPointer, pcode, sizeof(PCodeBinary) * ((2 * *(pcode)) + 1));
  currPointer += ((2 * *(pcode)) + 1);

  // Walk through each block in physical order and copy the pcode segment
  // sequence into the newly allocated pcode segment.
  for (i=0; i < physBlockList.entries(); i++)
  {
    PCodeBlock* block = physBlockList[i];

    FOREACH_INST_IN_BLOCK(block, inst) {
      Int32 length = PCode::getInstructionLength(inst->code);
      memcpy(currPointer, inst->code, sizeof(PCodeBinary) * length);
      currPointer += length;
    } ENDFE_INST_IN_BLOCK
  }

  // Terminate the pcode byte stream with an END instruction.
  currPointer[0] = PCIT::END;

  // If space is null, new pcode was first created in heap memory.  Copy new
  // pcode from heap back into original pcode memory, and then delete memory
  // allocated in heap.
  if (!space_) {
    memcpy(pcode, newPcode, sizeof(PCodeBinary) * totalLength);
    NADELETEBASIC(newPcode, heap_);
    newPcode = pcode;
  }

  // Update the expression's pcode object with the new pcode segment
  PCodeSegment* pcodeSegment = expr_->getPCodeSegment();
  pcodeSegment->setPCodeSegmentSize(sizeof(PCodeBinary) * totalLength);
  pcodeSegment->setPCodeBinary(newPcode);

  return TRUE;
}

/*****************************************************************************
 * Phase 2 (Optimizations)
 *****************************************************************************/

//
// Inlining
//
void PCodeCfg::inlining()
{
  PCodeInst* inst;
  PCodeBinary* pcode;
  CollIndex i;

  // Inlining candidates are at most 1 block
  if (entryBlock_->getSuccs().entries() != 0)
    return;

  // There must be at least 1 tuple, but no more than 2.
  pcode = expr_->getPCodeBinary();
  if ((pcode[0] == 0) || (pcode[0] > 2))
    return;

  inst = entryBlock_->getFirstInst();

  // Better have at least one instruction
  if (!inst)
    return;

  // Overall check to make sure read operands are vars or consts
  for (i=0; i < inst->getROps().entries(); i++)
    if ((inst->getROps()[i]->getStackIndex() < 4) &&
        (inst->getROps()[i]->getStackIndex() != 1))
      return;

  switch (inst->getOpcode()) {
    case PCIT::EQ_MBIN32S_MBIN8S_MBIN8S:
    case PCIT::EQ_MBIN32S_MBIN8U_MBIN8U:

    case PCIT::EQ_MBIN32S_MBIN16S_MBIN16S:
    case PCIT::EQ_MBIN32S_MBIN32S_MBIN32S:
    case PCIT::EQ_MBIN32S_MBIN16U_MBIN16U:
    case PCIT::EQ_MBIN32S_MBIN32U_MBIN32U:
    case PCIT::EQ_MBIN32S_MBIN64S_MBIN64S:

    case PCIT::EQ_MBIN32S_MASCII_MASCII:
    case PCIT::LT_MBIN32S_MASCII_MASCII:
    case PCIT::LE_MBIN32S_MASCII_MASCII:
    case PCIT::GT_MBIN32S_MASCII_MASCII:
    case PCIT::GE_MBIN32S_MASCII_MASCII:

    case PCIT::COMP_MBIN32S_MASCII_MASCII_IBIN32S_IBIN32S_IBIN32S:
    case PCIT::NULL_TEST_MBIN32S_MATTR5_IBIN32S_IBIN32S:
    {
      CollIndex bvIndex = inst->getWOps()[0]->getBvIndex();

      inst = inst->next;
      if (inst && inst->getOpcode() == PCIT::MOVE_MBIN32S) {
        // Assert to make sure read is correct.
        if ((bvIndex != inst->getROps()[0]->getBvIndex()) ||
            (!inst->getROps()[0]->isTemp()))
        {
          assert (FALSE);
          break;
        }

        inst = inst->next;
        if (inst && inst->getOpcode() == PCIT::RETURN) {
          expr_->setPCodeMoveFastpath(TRUE);
        }
      }

      break;
    }

    case PCIT::HASH2_DISTRIB:
    case PCIT::HASH_MBIN32U_MBIN64_IBIN32S_IBIN32S:
    case PCIT::HASH_MBIN32U_MBIN32_IBIN32S_IBIN32S:
    case PCIT::HASH_MBIN32U_MBIN16_IBIN32S_IBIN32S:
    case PCIT::HASH_MBIN32U_MPTR32_IBIN32S_IBIN32S:

    case PCIT::MOVE_MBIN8_MBIN8_IBIN32S:
    case PCIT::MOVE_MBIN32U_MBIN32U:
    case PCIT::MOVE_MBIN64S_MBIN64S:
    {
      // Quick check to make sure write operand is var
      if (inst->getWOps()[0]->getStackIndex() < 4)
        return;

      inst = inst->next;
      if (inst && inst->getOpcode() == PCIT::RETURN) {
        expr_->setPCodeMoveFastpath(TRUE);
      }

      break;
    }
  }
}

//
// Overwite pad for null indicator moves
//
PCodeInst* PCodeCfg::extendPaddedNullIndMoves(PCodeInst* moveInst)
{
  // We are only interested in null indicator moves for exploded format
  if (moveInst->getOpcode() == PCIT::MOVE_MBIN16U_MBIN16U)
  {
    //
    // Expand this move to include whatever padding they may be known to have
    //

    if (moveInst->getROps()[0]->isVar() && moveInst->getWOps()[0]->isVar()) {
      CollIndex srcIndex = moveInst->getROps()[0]->getStackIndex() - 4;
      CollIndex dstIndex = moveInst->getWOps()[0]->getStackIndex() - 4;

      NullTriple src(atpMap_[srcIndex], atpIndexMap_[srcIndex],
                     moveInst->getROps()[0]->getOffset());

      NullTriple dst(atpMap_[dstIndex], atpIndexMap_[dstIndex],
                     moveInst->getWOps()[0]->getOffset());

      if (nullToPadMap_->contains(&src) && nullToPadMap_->contains(&dst)) {
        Int32 *srcPad, *dstPad;

        NullTriple* srcPtr = &src;
        NullTriple* dstPtr = &dst;

        srcPad = nullToPadMap_->getFirstValue(srcPtr);
        dstPad = nullToPadMap_->getFirstValue(dstPtr);

        // If both the source and the dest have the same pad-length, then the
        // the move could be converted into a larger move that includes the
        // pad.  Note, the "pad" stored in the hashtable is really the length
        // of the null indicator which *includes* the padding.

        if (*srcPad == *dstPad) {
          switch (*srcPad) {
            case 2:
              // Null indicator is already adjacent to store column.  Return.
              return moveInst;

            case 4:
              moveInst->code[0] = PCIT::MOVE_MBIN32U_MBIN32U;
              break;

            case 8:
              moveInst->code[0] = PCIT::MOVE_MBIN64S_MBIN64S;
              break;

            default:
            {
              PCodeBlock* block = moveInst->block;

              PCodeInst* newInst = block->insertNewInstAfter(moveInst,
                                     PCIT::MOVE_MBIN8_MBIN8_IBIN32S);
              newInst->code[1] = moveInst->code[1];
              newInst->code[2] = moveInst->code[2];
              newInst->code[3] = moveInst->code[3];
              newInst->code[4] = moveInst->code[4];
              newInst->code[5] = *srcPad;

              block->deleteInst(moveInst);
              moveInst = newInst;

              break;
            }
          }

          // Reload the move instruction
          moveInst->reloadOperands(this);
        }
      }
    }
  }

  // No change made
  return moveInst;
}

//
// Flatten Null Branches
//
void PCodeCfg::flattenNullBranches()
{
  PCodeInst *moveInst, *fillInst;
  PCodeBlock *ftBlock, *tgtBlock;

  FOREACH_BLOCK(nullBlock, firstInst, branchInst, index) {
    // Continue if we don't have a null block
    if (!branchInst || !branchInst->isNullBranch() ||
        (branchInst->getOpcode() !=
         PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_IATTR3_IBIN32S) ||
        branchInst->getWOps()[0]->forAlignedFormat() ||
        branchInst->getROps()[0]->forAlignedFormat())
      continue;

    ftBlock = nullBlock->getFallThroughBlock();
    tgtBlock = nullBlock->getTargetBlock();

    // Verify pattern
    if ((ftBlock->getSuccs().entries() != 1) ||
        (tgtBlock->getSuccs().entries() != 1) ||
        (ftBlock->getSuccs()[0] != tgtBlock->getSuccs()[0]))
      continue;

    // Check fill mem instruction
    fillInst = ftBlock->getFirstInst();
    if (!fillInst || (fillInst->next != ftBlock->getLastInst()) ||
        (fillInst->getOpcode() != PCIT::FILL_MEM_BYTES) ||
        !fillInst->next->isUncBranch())
      continue;

    // Check move instruction
    moveInst = tgtBlock->getFirstInst();
    if (!moveInst->isMove())
      moveInst = tgtBlock->getLastInst();

    if (!moveInst || !moveInst->isMove())
      continue;

    // No other instruction in the MOVE block should fail if column is actually
    // null - note, we only need to check the 1st instruction.
    if (tgtBlock->getFirstInst()->mayFailIfNull())
      continue;

    // Make sure the two match
    if (moveInst->getWOps()[0]->getBvIndex() !=
        fillInst->getWOps()[0]->getBvIndex())
      continue;

    // Lastly, bignum moves in particular can't always be executed if nullable
    // because they clear/set the sign bit.  If the sign bit (and storage data)
    // resides in the static const null data array, then we'll get a write
    // error.  Fortunately though we can avoid this problem if the BIGNUM move
    // has src/tgt lengths equal to each other (in which case peephole will
    // simply convert this into a fast move.  Similarly, other moves to bignum
    // can result in setting the sign bit.  But if the result type is a temp,
    // then it's impossible for that temp to reside in the const null data
    // array, so we're okay in that case.

    if (moveInst->getOpcode() == PCIT::MOVE_MBIGS_MBIGS_IBIN32S_IBIN32S) {
      if (moveInst->code[5] != moveInst->code[6])
        continue;
    }
    else if (moveInst->getWOps()[0]->getType() == PCIT::MBIGS) {
      if (!moveInst->getWOps()[0]->isTemp())
        continue;
    }

    moveInst = branchInst;
    moveInst->code[0] = PCIT::MOVE_MBIN16U_MBIN16U;

    deleteBlock(ftBlock);

    // Time to reload the operands
    moveInst->reloadOperands(this);

  } ENDFE_BLOCK
}


PCodeInst* PCodeBlock::findDef(PCodeInst* inst,
                               PCodeOperand* operand,
                               NABoolean globalSearch)
{
  CollIndex i;
  assert (inst != NULL);

  // Constant operands are always (and only) defined upon entry to the routine
  if (operand->isConst())
    return (PCodeInst*)-1;

  CollIndex bvIndex = operand->getBvIndex();

  // First search locally
  FOREACH_INST_IN_BLOCK_BACKWARDS_AT(inst->block, def, inst->prev) {
    for (i=0; i < def->getWOps().entries(); i++)
      if (def->getWOps()[i]->getBvIndex() == bvIndex)
        return def;
  } ENDFE_INST_IN_BLOCK_BACKWARDS_AT

  // If we're not searching globally with reaching defs, we can still continue
  // searching easily if the block has only one predecessor.
  if (!globalSearch)
  {
    PCodeBlock* block = inst->block;
    while (block->getPreds().entries() == 1) {
      block = block->getPreds()[0];

      // Search locally in block, as was done before, starting at the end.
      FOREACH_INST_IN_BLOCK_BACKWARDS(block, def) {
        for (i=0; i < def->getWOps().entries(); i++)
          if (def->getWOps()[i]->getBvIndex() == bvIndex)
            return def;
      } ENDFE_INST_IN_BLOCK_BACKWARDS
    }
    return NULL;
  }

  // Use reaching defs to find the definition.
  return inst->block->reachingDefsTab_->find(bvIndex, TRUE /* In */);
}

// Inputs: Both insts have the same number of read operands and they are the
// same bit vector index.
NABoolean PCodeCfg::doOperandsHaveSameDef(PCodeInst* head, PCodeInst* tail)
{
  CollIndex i;

  assert(head->getROps().entries() == tail->getROps().entries());

  // Clause evals are treated specially
  if (head->isClauseEval()) {
    PCodeInst* prevHead = head->prev;
    PCodeInst* prevTail = head->prev;

    while (prevHead && prevHead->isOpdata()) {
      if (prevHead->getROps().entries() != 0) {

        PCodeOperand* readOp = prevHead->getROps()[0];

        PCodeInst* defH = head->block->findDef(prevHead, readOp, TRUE);
        PCodeInst* defT = tail->block->findDef(prevTail, readOp, TRUE);

        if ((defH == NULL) || (defT == NULL) || (defH != defT))
          return FALSE;
      }

      prevHead = prevHead->prev;
      prevTail = prevTail->prev;
    }

    return TRUE;
  }

  // Check each read operand to verify that both head and tail have the same def
  for (i=0; i < head->getROps().entries(); i++) {
    PCodeOperand* readOp = head->getROps()[i];

    PCodeInst* defH = head->block->findDef(head, readOp, TRUE);
    PCodeInst* defT = tail->block->findDef(tail, readOp, TRUE);

    if ((defH == NULL) || (defT == NULL) || (defH != defT))
      return FALSE;
  }

  return TRUE;
}

NABoolean PCodeCfg::doesInstQualifyForCSE(PCodeInst* inst)
{
  CollIndex i;
  Int32 opc = inst->getOpcode();

  // Exlcude certain cases.
  if (inst->isBranch() ||
      inst->isIndirectBranch() ||
      inst->isCopyMove() ||
      inst->isConstMove() ||
      inst->isEncode() ||
      inst->isDecode() ||
      inst->isOpdata())
    return FALSE;

  // One write operand, and at least 1 read op.  TODO: We may want to make an
  // exception for range instructions, since the duplicated one can be deleted.
  // CLAUSE_EVAL instructions are excluded from this check.
  if (!inst->isClauseEval() &&
      ((inst->getWOps().entries() != 1) || (inst->getROps().entries() == 0)))
    return FALSE;

  // Moves with constant read operands are better off being handled by copy
  // propagation and constant folding.
  if (inst->isMove() && inst->getROps()[0]->isConst())
    return FALSE;

  switch (opc) {
    case PCIT::MOVE_MBIN32S:
    case PCIT::MOVE_MBIN64S_MDECU_IBIN32S:
    case PCIT::MOVE_MBIN64S_MDECS_IBIN32S:
    case PCIT::NULL_MBIN16U_IBIN16U:
    case PCIT::AND_MBIN32S_MBIN32S_MBIN32S:
    case PCIT::OR_MBIN32S_MBIN32S_MBIN32S:
    case PCIT::CLAUSE_BRANCH:
    case PCIT::PROFILE:
    case PCIT::NOP:
    case PCIT::END:
    case PCIT::FILL_MEM_BYTES:
    case PCIT::GENFUNC_PCODE_1:
    case PCIT::OFFSET_IPTR_IPTR_MBIN32S_MBIN64S_MBIN64S:
    case PCIT::OFFSET_IPTR_IPTR_IBIN32S_MBIN64S_MBIN64S:
    case PCIT::RETURN:
    case PCIT::RETURN_IBIN32S:
    case PCIT::HASHCOMB_BULK_MBIN32U:
    case PCIT::MOVE_BULK:
    case PCIT::MOVE_MATTR5_MATTR5:
    case PCIT::SWITCH_MBIN32S_MATTR5_MPTR32_IBIN32S_IBIN32S:
    case PCIT::SWITCH_MBIN32S_MBIN64S_MPTR32_IBIN32S_IBIN32S:
    case PCIT::HDR_MPTR32_IBIN32S_IBIN32S_IBIN32S_IBIN32S_IBIN32S:
    case PCIT::UPDATE_ROWLEN3_MATTR5_IBIN32S:
    case PCIT::FILL_MEM_BYTES_VARIABLE:
      return FALSE;
  }

  // No PCODE instructions that return 32-bit floats (not yet)
  for (i=0; i < inst->getWOps().entries(); i++) {
    switch (inst->getWOps()[i]->getType()) {
      case PCIT::MFLT32:
        return FALSE;
    }
  }

  // Only certain clauses qualify
  if (inst->isClauseEval()) {
    ex_clause* clause = (ex_clause*)*(Long*)&(inst->code[1]);
    switch (clause->getClassID()) {
      case ex_clause::CONV_TYPE:
      case ex_clause::ARITH_TYPE:
        break;

      default:
        return FALSE;
    }

    // Also, one, and only one, write operand allowed.
    PCodeInst* opData = inst->prev;
    Int32 numOfWrites = 0;
    for (; opData && opData->isOpdata(); opData=opData->prev) {
      if (opData->getWOps().entries())
        numOfWrites++;
    }

    if (numOfWrites != 1)
      return FALSE;
  }

  return TRUE;
}

void PCodeCfg::setupForCSE(INSTLIST** cseList)
{
  CollIndex i;

  // Set up cseList to keep track of like PCODE instructions
  for (i=0; i < MAX_NUM_PCODE_INSTS; i++)
    cseList[i] = new(heap_) INSTLIST(heap_);

  FOREACH_BLOCK_REV_DFO(block, firstInst, lastInst, index) {
    FOREACH_INST_IN_BLOCK(block, inst) {
      if (!doesInstQualifyForCSE(inst))
        continue;
      cseList[inst->getOpcode()]->insert(inst);
    } ENDFE_INST_IN_BLOCK
  } ENDFE_BLOCK_REV_DFO
}

void PCodeCfg::destroyForCSE(INSTLIST** cseList)
{
  CollIndex i;
  // Clean up memory allocated dynamically
  for (i=0; i < MAX_NUM_PCODE_INSTS; i++) {
    delete cseList[i];
  }
}

NABoolean PCodeCfg::localCSE(INSTLIST** parent, PCodeBlock* tailBlock, 
                             NABoolean* rDefsComputed)
{
  CollIndex i, j;
  NABoolean csePerformed = FALSE;

  assert (parent != NULL);

  // Walk through all instructions and identify potential sub-exprs
  FOREACH_INST_IN_BLOCK(tailBlock, tail) {
    Int32 opcode = tail->getOpcode();
    if (parent[opcode]->entries() == 0)
      continue;

    if (!doesInstQualifyForCSE(tail))
      continue;

    for (i=0; i < parent[tail->getOpcode()]->entries(); i++)
    {
      PCodeOperand *writeOperand, *readOperand;
      PCodeInst* head = (*(parent[opcode]))[i];

      // This can only happen for global CSE where all insts are recorded
      if (head == tail)
        continue;

      // Boolean used to say if inconsistency is found preventing a match
      NABoolean match = TRUE;

      // Checks done for clause evals
      if (head->isClauseEval())
      {
        NABoolean readSeen = FALSE;
        ex_clause* clauseHead = (ex_clause*)*(Long*)&(head->code[1]);
        ex_clause* clauseTail = (ex_clause*)*(Long*)&(tail->code[1]);

        PCodeInst *readDef = NULL, *writeDef = NULL;

        // And if head is a clause eval, tail better be the same type.
        if ((clauseHead->getClassID() != clauseTail->getClassID()) ||
            (clauseHead->getOperType() != clauseTail->getOperType()))
          continue;

        switch (clauseHead->getClassID()) {
          case ex_clause::CONV_TYPE:
            match =
              ((((ex_conv_clause*)clauseHead)->getInstruction() ==
               ((ex_conv_clause*)clauseTail)->getInstruction()) &&
              (((ex_conv_clause*)clauseHead)->treatAllSpacesAsZero() ==
               ((ex_conv_clause*)clauseTail)->treatAllSpacesAsZero()));
            if ( match ) {
               ConvInstruction instr = ((ex_conv_clause*)clauseHead)->getInstruction();
               if ((instr == CONV_DATETIME_DATETIME) ||
                   ((instr >= CONV_INTERVALY_INTERVALMO) && (instr <= CONV_INTERVAL_ASCII)))
               {
                  Attributes *tgtH = ((ex_conv_clause*)clauseHead)->getOperand(0);
                  Attributes *tgtT = ((ex_conv_clause*)clauseTail)->getOperand(0);
                  Attributes *srcH = ((ex_conv_clause*)clauseHead)->getOperand(1);
                  Attributes *srcT = ((ex_conv_clause*)clauseTail)->getOperand(1);
                  match = (
                          ( srcH->getDatatype()  == srcT->getDatatype()  )
                       && ( srcH->getPrecision() == srcT->getPrecision() )
                       && ( srcH->getScale()     == srcT->getScale()     )
                       && ( tgtH->getDatatype()  == tgtT->getDatatype()  )
                       && ( tgtH->getPrecision() == tgtT->getPrecision() )
                       && ( tgtH->getScale()     == tgtT->getScale()     )
                       );
               }
            }
            break;

          case ex_clause::ARITH_TYPE:
            match =
              (((ex_arith_clause*)clauseHead)->getInstruction() ==
               ((ex_arith_clause*)clauseTail)->getInstruction());
            break;
        }

        if (!match)
          continue;

        // ## First check if clause evals have same # of operands

        PCodeInst* prevHead = head->prev;
        PCodeInst* prevTail = tail->prev;

        Int32 headCount = 0;
        Int32 tailCount = 0;

        while (prevHead && prevHead->isOpdata()) {
          headCount++;
          prevHead = prevHead->prev;
        }

        while (prevTail && prevTail->isOpdata()) {
          tailCount++;
          prevTail = prevTail->prev;
        }

        // Verify the same # of operands are there - NOTE, the clause ID is
        // not enough since NULLS can represent additional OPDATAs
        if (headCount != tailCount)
          continue;

        // ## Now check if operands are same in clause evals

        prevHead = head->prev;
        prevTail = tail->prev;

        while (match && prevHead && prevTail && prevHead->isOpdata()) {
          assert (prevHead->block == head->block);
          assert (prevTail->block == tail->block);
          assert (prevTail->isOpdata());

          // Record the pcode opdata insts for the write operands
          if (prevHead->getWOps().entries() && prevTail->getWOps().entries())
          {
            if (prevHead->getWOps()[0]->getLen() !=
                prevTail->getWOps()[0]->getLen())
              match = FALSE;

            // Only one write operand allowed.
            if (writeDef)
              match = FALSE;
            else {
              readDef = prevHead;
              writeDef = prevTail;
            }
          }
          else if (prevHead->getROps().entries() &&
                   prevTail->getROps().entries())
          {
            readSeen = TRUE;
            if (prevHead->getROps()[0]->getBvIndex() !=
                prevTail->getROps()[0]->getBvIndex())
              match = FALSE;

            if (prevHead->getROps()[0]->getLen() !=
                prevTail->getROps()[0]->getLen())
              match = FALSE;
          }
          else {
            match = FALSE;
          }

          prevHead = prevHead->prev;
          prevTail = prevTail->prev;
        }

        // If match not found, or if no write or read operand found, continue.
        // Note, the readDef represents the head write instruction.
        if (!match || (readDef == NULL) || (readSeen == FALSE))
          continue;

        // If reaching defs information not available, compute it and indicate
        // that it was computed here by setting the appropriate flag.
        if (rDefsHeap_ == NULL) {
          computeReachingDefs(0);
          *rDefsComputed = TRUE;
        }

        readOperand = readDef->getWOps()[0];
        writeOperand = writeDef->getWOps()[0];

        // First verify that the head def reaches the tail inst
        if (tailBlock->findDef(tail, readOperand, TRUE) != readDef)
          continue;

        // Do the read operands of the head change along the path to this member
        if (!doOperandsHaveSameDef(head, tail))
          continue;

        if (!readOperand->hasKnownLen() && !readOperand->isVarchar())
          continue;
      }
      else
      {

      // Quick check that both instructions have the same number of operands.
      // This is done since some instructions (e.g. SUBSTR) can conditonally
      // have a different number of operands (noteably "length").
      if ((head->getROps().entries() != tail->getROps().entries()) ||
          (head->getWOps().entries() != tail->getWOps().entries()))
        continue;

      // First validate that this member has the same read operands
      for (j=0; match && (j < head->getROps().entries()); j++) {
        if (head->getROps()[j]->getBvIndex() !=
            tail->getROps()[j]->getBvIndex())
          match = FALSE;

        if (head->getROps()[j]->getLen() !=
            tail->getROps()[j]->getLen())
          match = FALSE;
      }

      // Verify that the write operand is similar.  Note, because MATTR5 is
      // used for varchar and char, and because we won't move a char into a
      // varchar (or vice-versa) just yet, guard against this for now.

      if ((head->getWOps()[0]->getLen() != tail->getWOps()[0]->getLen()) ||
          (head->getWOps()[0]->isVarchar() ^ tail->getWOps()[0]->isVarchar()))
        match = FALSE;

      // Verify that hidden (important) operands match
      if (match) {
        switch (head->getOpcode()) {
          case PCIT::COMP_MBIN32S_MBIGS_MBIGS_IBIN32S_IBIN32S:
            match = (head->code[8] == tail->code[8]);
            break;

          case PCIT::SUBSTR_MATTR5_MATTR5_MBIN32S_MBIN32S:
            // start matches
            match = (head->code[10] == tail->code[10]);
            break;

          // Repeat
          case PCIT::GENFUNC_MATTR5_MATTR5_MBIN32S_IBIN32S:
            // oper-type matches
            match = (head->code[13] == tail->code[13]);
            break;

          // Lower, Upper, Trim
          case PCIT::GENFUNC_MATTR5_MATTR5_IBIN32S:
            // oper-type matches and trim char matches
            match = (head->code[11] == tail->code[11]);
            break;

          case PCIT::COMP_MBIN32S_MUNI_MUNI_IBIN32S_IBIN32S_IBIN32S:
          case PCIT::COMP_MBIN32S_MASCII_MASCII_IBIN32S_IBIN32S_IBIN32S:
            match = (head->code[9] == tail->code[9]);
            break;

          case PCIT::NULL_TEST_MBIN32S_MBIN16U_IBIN32S:
            match = (head->code[5] == tail->code[5]);
            break;

          case PCIT::GENFUNC_MBIN8_MBIN8_MBIN8_IBIN32S_IBIN32S:
            match = (head->code[1] == tail->code[1]);
            break;

          case PCIT::ENCODE_MASCII_MBIN8S_IBIN32S:
          case PCIT::ENCODE_MASCII_MBIN8U_IBIN32S:
          case PCIT::ENCODE_MASCII_MBIN32S_IBIN32S:
          case PCIT::ENCODE_MASCII_MBIN32U_IBIN32S:
          case PCIT::ENCODE_MASCII_MBIN16S_IBIN32S:
          case PCIT::ENCODE_MASCII_MBIN16U_IBIN32S:
          case PCIT::ENCODE_MASCII_MBIN64S_IBIN32S:
          case PCIT::ENCODE_NXX:
          case PCIT::DECODE_MASCII_MBIN8S_IBIN32S:
          case PCIT::DECODE_MASCII_MBIN8U_IBIN32S:
          case PCIT::DECODE_MASCII_MBIN32S_IBIN32S:
          case PCIT::DECODE_MASCII_MBIN32U_IBIN32S:
          case PCIT::DECODE_MASCII_MBIN16S_IBIN32S:
          case PCIT::DECODE_MASCII_MBIN16U_IBIN32S:
          case PCIT::DECODE_MASCII_MBIN64S_IBIN32S:
          case PCIT::DECODE_NXX:
            match = (head->code[5] == tail->code[5]);
            break;

          case PCIT::MOVE_MASCII_MASCII_IBIN32S_IBIN32S:
          case PCIT::MOVE_MUNI_MUNI_IBIN32S_IBIN32S:
          case PCIT::HASH_MBIN32U_MATTR5:
          case PCIT::HASH_MBIN32U_MUNIV:
          case PCIT::STRLEN_MBIN32U_MATTR5:
          case PCIT::STRLEN_MBIN32U_MUNIV:
          case PCIT::NULL_TEST_MBIN32S_MATTR5_IBIN32S_IBIN32S:
          case PCIT::COMP_MBIN32S_MUNIV_MUNIV_IBIN32S:
          case PCIT::COMP_MBIN32S_MATTR5_MATTR5_IBIN32S:
          case PCIT::LIKE_MBIN32S_MATTR5_MATTR5_IBIN32S_IBIN32S:
          {
            for (j=3;
                 j < (CollIndex)PCode::getInstructionLength(head->code); j++)
              match = match && (head->code[j] == tail->code[j]);
          }

          default:
            break;
        }
      }

      if (!match) continue;

      // If reaching defs information not available, compute it and indicate
      // that it was computed here by setting the appropriate flag.
      if (rDefsHeap_ == NULL) {
        computeReachingDefs(0);
        *rDefsComputed = TRUE;
      }

      // First verify that the head def reaches the tail inst
      if (tailBlock->findDef(tail, head->getWOps()[0], TRUE) != head)
        continue;

      // Do the read operands of the head change along the path to this member
      if (!doOperandsHaveSameDef(head, tail))
        continue;

      if (!head->getWOps()[0]->hasKnownLen() &&
          !head->getWOps()[0]->isVarchar())
        continue;

      writeOperand = tail->getWOps()[0];
      readOperand = head->getWOps()[0];

      }

      // Match found!
      PCodeInst* newInst;

      // First create the new move instruction with the appropriate opcode.
      if (writeOperand->isVarchar()) {
        newInst = tailBlock->insertNewInstAfter(tail,
                             PCIT::MOVE_MATTR5_MATTR5);
      }
      else
        newInst = tailBlock->insertNewInstAfter(tail,
                                     PCIT::MOVE_MBIN8_MBIN8_IBIN32S);

      // Fill in the pcode bytecode for the appropriate opcode
      if (writeOperand->isVarchar()) {
        assert (readOperand->isVarchar());
        newInst->setupPcodeForVarcharMove(writeOperand, readOperand);
      }
      else
      {
        newInst->code[1] = writeOperand->getStackIndex();
        newInst->code[2] = writeOperand->getOffset();
        newInst->code[3] = readOperand->getStackIndex();
        newInst->code[4] = readOperand->getOffset();
        newInst->code[5] = readOperand->getLen();
      }

      newInst->reloadOperands(this);

      // The common sub-expression (tail) is no longer needed
      tailBlock->deleteInst(tail);

      // Remove inst from parent list also
      for (j=0; j < parent[opcode]->entries(); j++) {
        if (tail == (*(parent[opcode]))[j]) {
          parent[opcode]->remove(tail);
          break;
        }
      }

      //TODO: We need to propagate result here and restart;
      csePerformed = TRUE;

      // No need to keep searching through head (parent) list
      break;
    }
  } ENDFE_INST_IN_BLOCK

  return csePerformed;
}

NABoolean PCodeCfg::globalCSE()
{
  NABoolean restart = TRUE;
  NABoolean globalRestart = FALSE;
  NABoolean rDefsComputed = FALSE;
  INSTLIST* cseList[MAX_NUM_PCODE_INSTS];

  setupForCSE(cseList);

  while (restart) {
    restart = FALSE;
    FOREACH_BLOCK_REV_DFO(block, firstInst, lastInst, index) {
      restart = localCSE(cseList, block, &rDefsComputed) || restart;
    } ENDFE_BLOCK_REV_DFO

    globalRestart = globalRestart || restart;

    if (restart)
      copyPropagation(2, FALSE);
  }

  destroyForCSE(cseList); 

  if (rDefsComputed)
    removeReachingDefs();

  return globalRestart;
}

//
// Copy propagation helper routine.  Given a downstream use instruction, a use
// source operand is replaced with a source operand from an  upstream move
// instruction, if possible.
//
NABoolean PCodeCfg::copyPropForwardsHelper2(PCodeInst* use,
                                            PCodeOperand* useSrc,
                                            PCodeOperand* moveSrc)
{
  NABoolean restart = FALSE;

  if (moveSrc->hasSameLength(useSrc)) {
    // Replace the use source with move source in the pcode
    use->replaceOperand(useSrc, moveSrc);

    // Reload operands for def instruction
    use->reloadOperands(this);

    restart = TRUE;
  }
  else
  {
    // Some copy prop can be done for mixed-size moves if pcode
    // instructions exist for these mixed sizes
    PCIT::Instruction newOpc;

    switch (use->getOpcode()) {
      case PCIT::SUM_MBIN64S_MBIN64S:
      case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN64S:
        if ((useSrc->getType() != PCIT::MBIN64S) ||
            (moveSrc->getType() != PCIT::MBIN32S))
          return restart;

        newOpc = (use->getOpcode() == PCIT::SUM_MBIN64S_MBIN64S)
                   ? PCIT::SUM_MBIN64S_MBIN32S
                   : PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN32S;
        break;

      default:
        // No support yet for other instructions
        return restart;
    }

    // Replace the use source with move source in the pcode
    use->code[1+useSrc->getStackIndexPos()] = moveSrc->getStackIndex();
    use->code[1+useSrc->getOffsetPos()] = moveSrc->getOffset();

    // Change the opcode to reflect the new pcode instruction
    use->code[0] = newOpc;

    // Reload operands for def instruction
    use->reloadOperands(this);

    restart = TRUE;
  }

  return restart;
}

//
// Copy propagation helper for global optimizations.  This version is called
// on a block-by-block basis when reaching defs analysis is not available.  
// Local copy propagation is achieved if the passed in level is one less than
// the max allowed.
//
void PCodeCfg::copyPropForwardsHelper(PCodeBlock* block,
                                      PCodeInst* start,
                                      PCodeOperand* moveTgt,
                                      PCodeOperand* moveSrc,
                                      Int32 level)
{
  CollIndex i;

  // Only recurse down so many levels
  if (level >= COPY_PROP_MAX_RECURSION)
    return;

  // NABoolean isSrcAConst = moveSrc->isConst();

  FOREACH_INST_IN_BLOCK_AT(block, use, start) {
    for (i=0; i < use->getROps().entries(); i++) {
      NABoolean res = FALSE;
      PCodeOperand* useSrc = use->getROps()[i];

      // Proceed only if the use operand is the same as the target
      if (useSrc->getBvIndex() == moveTgt->getBvIndex())
        res = copyPropForwardsHelper2(use, useSrc, moveSrc);

#if 0
      //
      // If the move tgt and use src don't exactly match, they may still
      // overlap.  If the move src is a const, we can do something about this.
      // The following sequence is common when dealing with constants in an
      // in-list being compared to a numeric type with scale:
      //
      // MUL_MBIN64S_MBIN16S_MBIN32S (183) 2 64 1 10 1 4
      // RANGE_LOW_S64S64 (101) 2 64 -1 -999999999
      // RANGE_HIGH_S64S64 (102) 2 64 0 999999999
      // MOVE_MBIN32U_MBIN32U (202) 2 60 2 68
      //

      if (!res && isSrcAConst && 
          moveTgt->canOverlap(useSrc) && (moveTgt->getLen() > useSrc->getLen()))
      {
        Int32 diff = useSrc->getOffset() - moveTgt->getOffset();
        PCodeOperand op(1, moveSrc->getOffset() + diff, -1, -1);
        use->replaceOperand(useSrc, &op);
        use->reloadOperands(this);
      }
#endif
    }

    // If the instruction has a write operand that's the same as either the
    // move target or move source operand, then we need to stop searching.
    for (i=0; i < use->getWOps().entries(); i++) {
      if ((use->getWOps()[i]->getBvIndex() == moveTgt->getBvIndex()) ||
          (use->getWOps()[i]->getBvIndex() == moveSrc->getBvIndex()))
        return;
    }
  } ENDFE_INST_IN_BLOCK

  // 1. All succs processed must fall into block with no other pred
  for (i=0; i < block->getSuccs().entries(); i++) {
    PCodeBlock* succ = block->getSuccs()[i];
    if (succ->getPreds().entries() > 1)
      continue;

    copyPropForwardsHelper(succ,succ->getFirstInst(),moveTgt,moveSrc,level+1);
  }
}

//
// Copy propagation helper for global optimizations
//
NABoolean PCodeCfg::copyPropBackwardsHelper(INSTLIST& list,
                                            PCodeBlock* block,
                                            PCodeInst* start,
                                            PCodeOperand* moveTgt,
                                            PCodeOperand* moveSrc,
                                            Int32 level)
{
  CollIndex i;

  // Only recurse up so many levels
  if (level >= COPY_PROP_MAX_RECURSION)
    return FALSE;

  FOREACH_INST_IN_BLOCK_BACKWARDS_AT(block, def, start) {
    PCodeOperand* defTgt = NULL;

    // Only work with insts with a single write operand.  As a special case,
    // allow FILL_MEM_BYTES to proceed - it technically has one operand, but
    // implicit operands were added to prevent false overlap detection.
    if ((def->code[0] == PCIT::FILL_MEM_BYTES) || def->getWOps().entries() == 1)
      defTgt = def->getWOps()[0];

    // Assignment has to involve source operand.
    if (defTgt && (defTgt->getBvIndex() == moveSrc->getBvIndex()) &&
        moveTgt->hasSameLength(defTgt))
    {
      list.insert(def);
      return TRUE;
    }
    else
    {
      for (i=0; i < def->getWOps().entries(); i++) {
        if ((def->getWOps()[i]->getBvIndex() == moveTgt->getBvIndex()) ||
            (def->getWOps()[i]->getBvIndex() == moveSrc->getBvIndex()))
          return FALSE;
      }

      for (i=0; i < def->getROps().entries(); i++) {
        if (def->getROps()[i]->getBvIndex() == moveTgt->getBvIndex())
          return FALSE;
      }
    }
  } ENDFE_INST_IN_BLOCK_BACKWARDS_AT

  //
  // Search for defs in pred blocks, returning TRUE only if defs can be found
  // along all preds in order to copy propagate.  For now, however, only allow
  // preds which naturally fall into this block - this makes a whole bunch of
  // issues go away.  But we still want to eventually solve something like:
  //
  //                  [a = ]
  //                 /      \
  //              [..]       [..]
  //                \         /
  //                  [  = a]
  //
  // This will require using reaching defs and/or liveness info
  //

  // 1. All preds must fall into block
  for (i=0; i < block->getPreds().entries(); i++) {
    if (block->getPreds()[i]->getSuccs().entries() > 1)
      return FALSE;
  }

  // 2. All preds should have def
  for (i=0; i < block->getPreds().entries(); i++)
  {
    PCodeBlock* pred = block->getPreds()[i];
    PCodeInst* predLastInst = pred->getLastInst();
    if (!copyPropBackwardsHelper(list, pred, predLastInst,
                                 moveTgt, moveSrc, level + 1))
    {
      return FALSE;
    }
  }

  // Return TRUE, indicating that if the list has any defs, we can safely copy
  // propagate them, since no instructions were found to invalidate them
  return TRUE;
}

//
// Global copy propataion.
//
void PCodeCfg::copyPropagation(Int32 phase, NABoolean reachDefsAvailable)
{
  /* Algorithm
   *
   * 1. Find a move/copy instruction.
   * 2. If WRITE is NON-TEMP, backward search only
   * 3. If READ is NON-TEMP, forward search only
   * 4. Else backwards/forwards for all
   */

  CollIndex i, j;

  NABoolean doConstantFolding = TRUE;

  FOREACH_BLOCK(block, firstInst, lastInst, index) {
    FOREACH_INST_IN_BLOCK(block, inst)
    {
      PCIT::Instruction opc;

      // First perform some constant folding.  We do this beforehand to help
      // further propagate these constants downwards/upwards
      if (doConstantFolding &&
          ((inst = constantFold(inst, reachDefsAvailable)) == NULL))
        continue;

      opc = (PCIT::Instruction)(inst->getOpcode());

      // Forwards Propagation
      if (phase == 2) {
        if (reachDefsAvailable) {
          for (i=0; i < inst->getROps().entries(); i++) {
            PCodeOperand* op = inst->getROps()[i];
            CollIndex bvIndex = op->getBvIndex();

            // Find def of source.
            PCodeInst* def = block->findDef(inst, op, TRUE);

            // Skip if def is defined on entry, is null, or not a temp move.
            if ((def == (PCodeInst*)-1) || !def || !def->isMove() ||
                (!def->getWOps()[0]->isTemp()))
              continue;

            PCodeOperand* moveTgt = def->getWOps()[0];
            PCodeOperand* moveSrc = def->getROps()[0];

            // No support for mixed sized moves during forward prop just yet
            if (!moveTgt->hasSameLength(moveSrc)) {
              // Okay, support *some* mixed sized moves
              if (def->getOpcode() == PCIT::MOVE_MATTR5_MASCII_IBIN32S) {
                // Verify that def of source of move reaches inst
                def = def->block->findDef(def, moveSrc, TRUE);
                if (!def || (def != block->findDef(inst, moveSrc, TRUE)))
                  continue;

                if (inst->isVarcharInstThatSupportsFixedCharOps()) {
                  inst->replaceOperand(op, moveSrc);
                  inst->reloadOperands(this);
                  constantFold(inst, reachDefsAvailable);
                }
              }
              continue;
            }

            // Verify that def of source of move reaches inst
            def = def->block->findDef(def, moveSrc, TRUE);
            if (!def || (def != block->findDef(inst, moveSrc, TRUE)))
              continue;

            if (copyPropForwardsHelper2(inst, op, moveSrc))
              constantFold(inst, reachDefsAvailable);
          }
        }
        else if (inst->isMove() && inst->getWOps()[0]->isTemp())
        {
          PCodeOperand* moveTgt = inst->getWOps()[0];
          PCodeOperand* moveSrc = inst->getROps()[0];

          // No support for mixed sized moves during forward prop just yet
          if (!moveTgt->hasSameLength(moveSrc))
            continue;

          copyPropForwardsHelper(block, inst->next, moveTgt, moveSrc, 0);
        }
      }
      // Backwards Propagation
      else if (phase == 1 && inst->isMove() && inst->getROps()[0]->isTemp())
      {
        PCodeOperand* moveTgt = inst->getWOps()[0];
        PCodeOperand* moveSrc = inst->getROps()[0];

        // No support for mixed sized moves during forward prop just yet
        if (!moveTgt->hasSameLength(moveSrc))
          continue;

        // Varchar writes to aligned rows must remain in their original order
        // So do not apply backward copyPropagation to varchars in aligned rows.
        if (moveTgt->getType() == PCIT::MATTR5 &&
            moveTgt->isVarchar() && 
            moveTgt->getVoaOffset() != -1) {
          continue;
        }

        INSTLIST list(heap_);

        if (copyPropBackwardsHelper(list, block, inst->prev,moveTgt,moveSrc,0)
            && (list.entries() > 0))
        {
          for (i=0; i < list.entries(); i++)
          {
            PCodeInst* def = list[i];
            PCodeBlock* defBlock = def->block;
            PCodeOperand* defTgt = def->getWOps()[0];

            // If the def is a branch, then the new instruction must be
            // inserted at the beginning of each of it's successors.
            PCodeInst* newInst = (def->isBranch())
              ? defBlock->getSuccs()[0]->insertNewInstBefore(NULL, opc)
              : defBlock->insertNewInstAfter(def, opc);

            // Set up new inst that copies tgt into src.  This is done to
            // ensure correctness once move is deleted.
            if (opc == PCIT::MOVE_MATTR5_MATTR5)
              newInst->setupPcodeForVarcharMove(moveSrc, moveTgt);
            else
            {
              newInst->code[1] = moveSrc->getStackIndex();
              newInst->code[2] = moveSrc->getOffset();
              newInst->code[3] = moveTgt->getStackIndex();
              newInst->code[4] = moveTgt->getOffset();

              // Length fields are needed for some moves
              if (inst->getOpcode()==PCIT::MOVE_MBIGS_MBIGS_IBIN32S_IBIN32S) {
                newInst->code[5] = moveSrc->getLen();
                newInst->code[6] = moveTgt->getLen();
              }
              else if (inst->getOpcode() == PCIT::MOVE_MBIN8_MBIN8_IBIN32S) {
                newInst->code[5] = moveSrc->getLen();
              }
            }
                
            // Reload operands for new instruction
            loadOperandsOfInst(newInst);

            // Again, if def is a branch, we need to create the inst in each
            // of it's succs.  Since we already did it for the fall-through,
            // do so again for any other succ
            if (def->isBranch()) {
              for (j=1; j < defBlock->getSuccs().entries(); j++) {
                newInst = copyInst(newInst);
                defBlock->getSuccs()[j]->insertInstBefore(NULL, newInst);
              }
            }

            // Reset def instruction by changing the target of the def to
            // be the target of the move
            def->replaceOperand(defTgt, moveTgt);

            // Reload operands for def instruction
            def->reloadOperands(this);
          }
          // Move no longer needed
          block->deleteInst(inst);
        }
      }
    } ENDFE_INST_IN_BLOCK
  } ENDFE_BLOCK
}

//
// Local dead code elimination
//
void PCodeCfg::localDeadCodeElimination()
{
  CollIndex i;
  NABitVector writeWithNoReadList(heap_);

  FOREACH_BLOCK_FAST(block, firstInst, lastInst, index) {
    writeWithNoReadList.clear();

    FOREACH_INST_IN_BLOCK_BACKWARDS(block, inst) {
      // Assume we should remove inst (but only if it has a write operand)
      NABoolean remove = (inst->getWOps().entries() > 0);

      // If every write operand was not seen before subsequent write, remove
      for (i=0; remove && (i < inst->getWOps().entries()); i++)
        if (!writeWithNoReadList.contains(inst->getWOps()[i]->getBvIndex()))
          remove = FALSE;

      if (remove && !inst->isOpdata()) {
        removeOrRewriteDeadCodeInstruction(inst);
        continue;
      }

      // Add writes to list
      for (i=0; i < inst->getWOps().entries(); i++)
        writeWithNoReadList += inst->getWOps()[i]->getBvIndex();

      // Remove reads from list
      for (i=0; i < inst->getROps().entries(); i++)
        writeWithNoReadList -= inst->getROps()[i]->getBvIndex();

    } ENDFE_INST_IN_BLOCK_BACKWARDS
  } ENDFE_BLOCK_FAST
}

//
// Dead code elimination pass that removes those instructions which write to
// variables which aren't live.
//
NABoolean PCodeCfg::deadCodeElimination() {
  // The analysis routine which computes liveness also performs dead-code-elim
  // in passing, if asked to.
  computeLiveness(TRUE /* Perform DCE */);

  // No need to tell caller to perform DCE again, since DCE in computeLiveness()
  // is a one-pass phase.
  return FALSE;
}

#if 0
//
// Dead code elimination pass that removes those instructions which write to
// variables which aren't live.  May need to factor in side-effects.
//
NABoolean PCodeCfg::deadCodeElimination()
{
  CollIndex i;
  NABoolean restart = FALSE;

  FOREACH_BLOCK(block, firstInst, lastInst, index) {

    FOREACH_INST_IN_BLOCK(block, inst) {
      NABitVector succLiveVectors(heap_);
      NABitVector* liveVectorPtr;

      // Set liveVectorPtr to the set of live operands at this instruction
      if (inst == lastInst) {
        // If this is an exit block, there are no live operands
        if (block->getSuccs().entries() == 0) {
          liveVectorPtr = NULL;
        }
        else {
          // Union in the live vectors for all successor blocks.
          for (i=0; i < block->getSuccs().entries(); i++)
            succLiveVectors += block->getSuccs()[i]->getLiveVector();

          liveVectorPtr = &succLiveVectors;

        }
      }
      else
        liveVectorPtr = &inst->next->liveVector_;

      CollIndex numOps = inst->getWOps().entries();

      // Nothing to check if instruction has no write operands
      NABoolean isLive = (numOps > 0) ? FALSE : TRUE;

      if (inst->isClauseEval()) {
        // Assume the clause is not live
        isLive = FALSE;

        assert(inst->prev && inst->prev->isOpdata());

        PCodeInst* prev = inst->prev;
        NABoolean writeFound = FALSE;

        for (; prev && prev->isOpdata(); prev = prev->prev) {
          for (i=0; i < prev->getWOps().entries(); i++) {
            PCodeOperand* operand = prev->getWOps()[i];
            CollIndex wBvIndex = operand->getBvIndex();

            writeFound = TRUE;

            if (!(operand->isTemp() || operand->isGarbage()) ||
                liveVectorPtr->testBit(wBvIndex))
            {
              isLive = TRUE;
              break;
            }
          }
        }

        if (!writeFound)
          isLive = TRUE;
      }
      else
      {
        // Check each write operand to see if it is live at this instruction.
        // Also, since non-temps are always live, don't delete them.
        for (i=0; i < numOps; i++) {
          PCodeOperand* operand = inst->getWOps()[i];
          CollIndex wBvIndex = operand->getBvIndex();
          if (!(operand->isTemp() || operand->isGarbage()) ||
              liveVectorPtr->testBit(wBvIndex) ||
              operand->isEmptyVarchar())
          {
            isLive = TRUE;
            break;
          }
        }
      }

      // If all write operands are dead at this point, attempt to delete the
      // instruction.
      if (!isLive) {
        const OPLIST& reads = inst->getROps();

        NABoolean deleted = removeOrRewriteDeadCodeInstruction(inst);

        // Restart DCE algorithm upon completion if there are uses in this
        // instruction that can be deleted.

        if (deleted) {
          for (CollIndex i=0; i < reads.entries(); i++) {
            PCodeOperand* src = reads[i];
            if (src->isTemp() && !liveVectorPtr->testBit(src->getBvIndex())) {
              restart = TRUE;
              break;
            }
          }
        }
      }
    } ENDFE_INST_IN_BLOCK
  } ENDFE_BLOCK

  return restart;
}
#endif

//
// Check to see if it is safe to remove/replace the instruction with the
// assumption being that it's write operand is not live
//
NABoolean PCodeCfg::removeOrRewriteDeadCodeInstruction(PCodeInst* inst)
{
  PCodeBlock* block = inst->block;
  switch (inst->getOpcode()) {
    case PCIT::BRANCH_AND:
    case PCIT::BRANCH_OR:
    {
      // Assume branch can't be removed, unless proven otherwise.
      NABoolean remove = FALSE;

      PCodeBlock* tgtBlock = block->getTargetBlock();
      PCodeBlock* fallThroughBlock = block->getFallThroughBlock();

      // See if branch falls-through and targets the same exit block.  The
      // exit blocks may not physically be the same, but they should have the
      // same RETURN instruction.  If not, fall-through to the case below.

      while (TRUE) {
        if (tgtBlock->isEmpty() && tgtBlock->getFallThroughBlock())
          tgtBlock = tgtBlock->getFallThroughBlock();
        else if (tgtBlock->getFirstInst()->getOpcode() == PCIT::BRANCH)
          tgtBlock = tgtBlock->getTargetBlock();
        else
          break;
      }

      while (TRUE) {
        if (fallThroughBlock->isEmpty() &&
            fallThroughBlock->getFallThroughBlock())
          fallThroughBlock = fallThroughBlock->getFallThroughBlock();
        else if (fallThroughBlock->getFirstInst()->getOpcode() == PCIT::BRANCH)
          fallThroughBlock = fallThroughBlock->getTargetBlock();
        else
          break;
      }

      // First, if both the target and the fall-through arrive to the same
      // block, then this branch can be safely deleted.
      if (tgtBlock == fallThroughBlock)
        remove = TRUE;
      // Second, if both blocks are "technically" the same exit block, then
      // we can delete the block in this case too.
      else if (tgtBlock->isExitBlock() && fallThroughBlock->isExitBlock() &&
               (tgtBlock->getFirstInst() == tgtBlock->getLastInst()) &&
               (fallThroughBlock->getFirstInst() ==
                fallThroughBlock->getLastInst()) &&
               (tgtBlock->getFirstInst()->getOpcode() ==
                fallThroughBlock->getFirstInst()->getOpcode()))
      {
        // If PCIT::RETURN, no problem.  Else need to check value of 
        // PCIT::RETURN_IBIN32S
        if ((tgtBlock->getFirstInst()->getOpcode() == PCIT::RETURN) ||
            (tgtBlock->getFirstInst()->code[1] ==
             fallThroughBlock->getFirstInst()->code[1]))
        {
          remove = TRUE;
        }
      }

      if (remove) {
        // Remove target succ edge and delete branch inst.
        block->removeEdge(block->getTargetBlock());
        block->deleteInst(inst);
        return TRUE;
      }

      // Fall-through to next case if remove not done (i.e. don't break)
    }

    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S:
    {
      //
      // Since this write is useless, try and move a different write in it.
      // Note, this can be considerably simplified if done as a backwards
      // copy propagation after having first rewriting the MOVE in the target
      // succ to use the write operand in the BRANCH.  Then all that's needed
      // is up-to-date liveness analysis to ensure the substituion is okay.
      // Maintaining the LIVENESS analysis is questionable though.
      //

      PCodeInst* targetFirstInst;
      Int32 constant;
      NABoolean reuseReadOperand = TRUE;

      PCodeBlock* fallThroughSucc = inst->block->getFallThroughBlock();
      PCodeBlock* targetSucc = inst->block->getTargetBlock();

      if (inst->getOpcode() == PCIT::BRANCH_OR) {
        constant = 1;
        fallThroughSucc = inst->block->getFallThroughBlock();
        targetSucc = inst->block->getTargetBlock();
      }
      else if (inst->getOpcode() == PCIT::BRANCH_AND) {
        constant = 0;
        fallThroughSucc = inst->block->getFallThroughBlock();
        targetSucc = inst->block->getTargetBlock();
      }
      else
      {
        // For NOT-NULL branches, we really want to look at the fall-through.
        // To use the same algorithm for all, assign the variables to their
        // opposite meanings.
        targetSucc = inst->block->getFallThroughBlock();
        fallThroughSucc = inst->block->getTargetBlock();
        constant = -1;
      }

      targetFirstInst = targetSucc->getFirstInst();

      if (targetFirstInst && 
          (((targetFirstInst->getOpcode() == PCIT::MOVE_MBIN32S_IBIN32S) &&
            targetFirstInst->code[3] == constant) ||
           ((targetFirstInst->getOpcode() == PCIT::MOVE_MBIN32U_MBIN32U) &&
            targetFirstInst->getROps()[0]->isConst() &&
            (getIntConstValue(targetFirstInst->getROps()[0]) == constant))) &&
          targetFirstInst->getWOps()[0]->isTemp())
      {
        PCodeOperand* newWriteOp = targetFirstInst->getWOps()[0];

        if (!targetSucc->getLiveVector().testBit(newWriteOp->getBvIndex()) &&
            !fallThroughSucc->getLiveVector().testBit(newWriteOp->getBvIndex()))
        {
          // All signs are go - replace the write operand with the new one
          inst->code[1+inst->getWOps()[0]->getStackIndexPos()] =
            newWriteOp->getStackIndex();

          inst->code[1+inst->getWOps()[0]->getOffsetPos()] =
            newWriteOp->getOffset();

          inst->reloadOperands(this);

          // Now delete move instruction as useless.  But only if there are no
          // other preds which may need it.
          //
          // TODO: Really fix this up so that optimizations are better
          if (targetSucc->getPreds().entries() == 1)
            targetSucc->deleteInst(targetFirstInst);

          reuseReadOperand = FALSE;
        }
      }

      // If no other operand was found to replace this branch's write, then
      // replace the write operand with the read operand.  This can be
      // interpretted at runtime to be a nop.  Can only be done for branches
      //
      // TODO: disable this for now since it may prevent CSE from happening -
      // for example, read op of branch could be the write op from a previous
      // instruction that is a common instruction.

#if 0
      if (reuseReadOperand && inst->isLogicalBranch()) {
        inst->code[3] = inst->code[5];
        inst->code[4] = inst->code[6];
        inst->reloadOperands(this);
      }
#endif

      break;
    }

    case PCIT::OPDATA_MPTR32_IBIN32S:
    case PCIT::OPDATA_MPTR32_IBIN32S_IBIN32S:
    case PCIT::OPDATA_MBIN16U_IBIN32S:
    case PCIT::OPDATA_MATTR5_IBIN32S:
      // TODO: Maybe we don't need the clause eval either then?
      return FALSE;

    case PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S_IBIN32S:
    {
      // Overwrite inst with smaller NOT_NULL_BRANCH_MBIN16S_IBIN32S.  Clear
      // out branch target, although that's not really needed.
      inst->code[0] = PCIT::NOT_NULL_BRANCH_MBIN16S_IBIN32S;
      inst->code[1] = inst->getROps()[0]->getStackIndex();
      inst->code[2] = inst->getROps()[0]->getOffset();
      inst->code[3] = 0;

      // No write operands with this instruction
      inst->getWOps().clear();

      return FALSE;
    }

    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_IATTR3_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_MBIN16S_IBIN32S:
    case PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_MBIN16S_IBIN32S:
    case PCIT::NNB_MBIN32S_MATTR3_IBIN32S_IBIN32S:
      // TODO: Replace with NOT_NULL_BRANCH form which doesn't write the
      // result back.
      return FALSE;

    case PCIT::NNB_SPECIAL_NULLS_MBIN32S_MATTR3_MATTR3_IBIN32S_IBIN32S:
      // For those instructions which write the null bitmap, it may be
      // possible to change them so that we just check the bitmap.  Right now
      // though there are no NNBB instructions which don't write to a result.
      return FALSE;

    // Overflow checking code?

    case PCIT::CLAUSE_EVAL:
    {
      // Can't delete clauses with side-effects
      ex_clause* clause = (ex_clause*)*(Long*)&(inst->code[1]);
      switch(clause->getClassID()) {

        case ex_clause::FUNC_ROWSETARRAY_INTO_ID:
        case ex_clause::FUNC_ROWSETARRAY_ROW_ID:
        case ex_clause::FUNC_ROWSETARRAY_SCAN_ID:
        case ex_clause::AGGR_ONE_ROW_ID:
        case ex_clause::FUNC_RAISE_ERROR_ID:
          return FALSE;

        default:
          block->deleteInst(inst);
          return TRUE;
      }
    }

    default:
      block->deleteInst(inst);
      return TRUE;
  }

  return FALSE;
}

PCodeOperand*
PCodeBlock::isIndirectBranchCandidate(PCodeBlock* headBlock,
                                      PCodeOperand* column,
                                      NABoolean forInList)
{
  PCodeInst* branch = last_;
  PCodeInst* eqComp = (branch) ? branch->prev : NULL;
  PCodeOperand *var, *constant;

  // Block must contain logical branch with EQ comp preceeding it.
  if (!branch || !branch->isLogicalBranch() || !eqComp || !eqComp->isEqComp())
    return NULL;

  // If we're dealing with a case statement, this block must branch to an else
  // statement, which means the branch must be a BRANCH_AND.  For in-lists, only
  // the head block has a restriction of being a BRANCH_OR, since it must always
  // branch to a "TRUE" block.
  if (!forInList) {
    if (branch->getOpcode() != PCIT::BRANCH_AND)
      return NULL;
  }
  else {
    if ((headBlock == NULL) && (branch->getOpcode() != PCIT::BRANCH_OR))
      return NULL;
  }

  // Unless this block is being treated as the potential head of the IN-list or 
  // of case statement, it must contain only the compare and the branch (and
  // the compare must be the same type as that in the head block).
  if (headBlock) {
    if (getFirstInst() != eqComp)
      return NULL;

    PCodeInst* headEqComp = headBlock->getLastInst()->prev;

    // All equality comparisons must involve same type.
    if ((headEqComp->isIntEqComp() && !eqComp->isIntEqComp()) ||
        (headEqComp->isStringEqComp() && !eqComp->isStringEqComp()) ||
        (headEqComp->isFloatEqComp() && !eqComp->isFloatEqComp()))
      return NULL;
  }

  // Comp must involve one constant operand.
  if (eqComp->getROps()[0]->isConst()) {
    constant = eqComp->getROps()[0];
    var = eqComp->getROps()[1];
  }
  else if (eqComp->getROps()[1]->isConst()) {
    constant = eqComp->getROps()[1];
    var = eqComp->getROps()[0];
  }
  else
    return NULL;

  // And just to be paranoid, ensure that the write operand of the compare is
  // that being used in the logical branch and is a temporary.
  if (!eqComp->getWOps()[0]->isTemp() ||
      (eqComp->getWOps()[0]->getBvIndex()!=branch->getROps()[0]->getBvIndex()))
    return NULL;

  // More paranoia.  The var operand compared must not be redefined within the
  // block (i.e. no "T1 = (T1 == Constant1)").  Note, this restriction doesn't
  // pertain to instructions prior to the EQ instruction.
  if ((eqComp->getWOps()[0]->getBvIndex() == var->getBvIndex()) ||
      (branch->getWOps()[0]->getBvIndex() == var->getBvIndex()))
    return NULL;

  // If dealing with a constant compare, the constant must not be INVALID_INT64.
  // INVALID_INT64 is used in the constants table to indicate an entry that is
  // missing.  Because an item in the IN-list could indeed be INVALID_INT64, we
  // have to prevent that from being a candidate for the opt.
  if (eqComp->isIntEqComp() || eqComp->isFloatEqComp())
  {
    Int64 iVal;

    if (eqComp->isFloatEqComp()) {
      double dVal = cfg_->getFloatConstValue(constant);
      iVal = *((Int64*)(&dVal));
    }
    else
      iVal = cfg_->getIntConstValue(constant);

    if (iVal == PCodeCfg::INVALID_INT64)
      return NULL;
  }

  // At this point, we're good to go if no head block was passed in.  In other
  // words, this block is a candidate to be the "head" of an IN-list or switch
  // statement.
  if (!column)
    return constant;

  // If head already exists, then we need to verify that the column involved in
  // the IN-list or case statement is the same for all blocks.  Check type also
  // for further assurance (although may be overkill) - note, do not do this
  // for string compares, since MASCII and MATTR5 are identical for this opt.
  if ((var->getBvIndex() != column->getBvIndex()) ||
      (!eqComp->isStringEqComp() && (var->getType() != column->getType())))
    return NULL;

  return constant;
}

CollIndex* PCodeCfg::createLookupTableForSwitch(OPLIST& constants,
                                                BLOCKLIST& blocks,
                                                UInt32& tableSize,
                                                NABoolean forInList)
{
  CollIndex i, j;

  // Determine what the max chain length should be for this table.
  CollIndex maxChainLength = 4;

  // Make sure we have more than one constant.
  if (constants.entries() <= 1)
    return NULL;

  NAList<UInt32> hashVals(heap_);
  NAList<char*>  strVals(heap_);
  NAList<Int32>  strLens(heap_);
  NAList<Int64>  intVals(heap_);

  // Save last block in list of blocks - it's needed when processing default
  // cases for a switch statement.  We record it now, since it may get removed
  // later if it's found to be a duplicate of a previous case statement.
  PCodeBlock* lastBlock = blocks[blocks.entries()-1];

  // First remove duplicate constants in list.

  for (i=0; i < constants.entries(); i++) {
    PCodeOperand* constant = constants[i];
    PCIT::AddressingMode type = constant->getType();
    switch(type) {

      // Strings
      case PCIT::MASCII:
      case PCIT::MATTR5:
      {
        Int32 len, res;
        UInt32 hashVal;

        char* str = getStringConstValue(constant, &len);

        // To treat all strings equally, remove any spaces.
        while ((len > 0) && (str[len-1] == ' '))
          len--;

        hashVal = ExHDPHash::hash(str, ExHDPHash::NO_FLAGS, len);

        // Look for duplicate using hash values.  If hash values are the same,
        // compare strings.  If strings are the same, remove the duplicate.  If
        // not, we're just unlucky in that two strings hashed to the same value.

        for (j=0; j < hashVals.entries(); j++) {
          if (hashVals[j] == hashVal) {
            res = charStringCompareWithPad(str, len, strVals[j],strLens[j],' ');
            if (res == 0) {
              constants.removeAt(i);
              blocks.removeAt(i);
              i--;
              break;
            }
          }
        }

        // If duplicate wasn't found, insert known values.
        if (j == hashVals.entries()) {
          hashVals.insert(hashVal);
          strVals.insert(str);
          strLens.insert(len);
        }

        break;
      }

      // Integers and Floats
      default:
      {
        Int64 val;
        
        if ((type == PCIT::MFLT64) || (type == PCIT::MFLT32)) {
          double fVal = getFloatConstValue(constant);
          val = *((Int64*)(&fVal));
        }
        else
          val = getIntConstValue(constant);

        // Look for duplicates and remove if found.
        for (j=0; j < intVals.entries(); j++) {
          if (intVals[j] == val) {
            constants.removeAt(i);
            blocks.removeAt(i);
            i--;
            break;
          }
        }

        // If duplicate wasn't found, insert known values.
        if (j == intVals.entries()) {
          intVals.insert(val);
        }

        break;
      }
    }
  }

  // Just make sure of a few things after the duplicate removal phase.
  assert ((constants.entries() >= 1) &&
          ((strVals.entries() == constants.entries()) ||
           (intVals.entries() == constants.entries())));

  //
  // Attempt to hash all constants into a hash table big enough such that no
  // two constants collide in the table.  If a collision, happens, however,
  // consider moving the constant into the next entry (if we don't result in a
  // wraparound).  We don't want to encourage this strategy, however, so use a
  // heuristic to determine when "enough is enough" and that we should try a
  // a bigger sized table.
  //
  // For switching on strings, the layout of the hash table is:
  //
  //   [len1, off1, len2, off2, ..., lenN, offN]
  //
  // where len<i> is the length of the <i>th string constant and off<i> is
  // the offset of that string in the constants area.
  //
  // For switching on integers, the layout of the hash table is:
  //
  //   [int64Val_1, int64Val_2, ..., int64Val_N]
  //
  // If we're creating a jump table also (i.e. we're doing this for a case
  // statement), then adjacent to the table above is a jump table that will
  // look like this:
  //
  //   [locOff1, locOff2, ..., locOffN, locOffDefault]
  //
  // Each "locOff" represents the pcode-relative offset from an indirect branch
  // instruction to the target associated for a particular case statement.
  // This jump table will get properly initialized when the pcode is layed out
  // after optimizations.
  //
  // Note that the entries in the jump table sometimes store the absolute
  // address of the brach target, so it should be big enough, e.g. 64-bit
  // 
  // The length of the table will be a prime number, so as to reduce the number
  // of collisions.
  //

  NABoolean restart = TRUE; // Flag to keep increasing table size.

  // Set up prime table for use in establishing hash table length.
  Int32 primeIdx  = 0;
  const UInt32 MAX_NUM_OF_PRIMES = 10;
  UInt32 primes[MAX_NUM_OF_PRIMES] =
    {23, 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289};

  // Make sure we start off with a "big" enough table to cover constants.
  while ((primeIdx < MAX_NUM_OF_PRIMES) &&
         (primes[primeIdx] < 2*constants.entries()))
    primeIdx++;

  Int32 *table = NULL;
  Long *jumpTable = NULL;
  Int32 numTableEntries;
  // jump table size depends on the pointer size in 4-byte (Int32) unit
  Int32 numJumpTableEntries = 0;
  Int32 totalTableSizeInBytes = 0;

  // Outer-loop when trying to create constant table with primeIdx-related size.
  while (restart && (primeIdx < MAX_NUM_OF_PRIMES))
  {
    restart = FALSE; // Assume no further increase will be needed.

    tableSize = primes[primeIdx];
    numTableEntries = tableSize * 2;

    // Allocate the constant lookup table / jump table.  We need to clear or
    // initialize table with 0xFE (INVALID_CHAR).
    if (forInList) {
      totalTableSizeInBytes = sizeof(Int32) * numTableEntries;
    }
    else {
      // jump table size depends on the pointer size in 4-byte (Int32) unit
      numJumpTableEntries = (tableSize + 1) * (sizeof(Long)/4);
      totalTableSizeInBytes = sizeof(Int32) * (numTableEntries + numJumpTableEntries);
    }

    table = new(heap_) Int32[numTableEntries + numJumpTableEntries];
    memset((char*)(table), 0xFE, totalTableSizeInBytes);

    if (!forInList)
      // the jump table starts right after the hash table
      jumpTable = (Long*)&table[numTableEntries]; 

    NABoolean collisionOccurred = FALSE;

    // Walk through all constants and attempt to insert into lookup table.
    for (i=0; !restart && (i < constants.entries()); i++)
    {
      PCodeOperand* constant = constants[i];
      switch(constant->getType()) {
        // Strings
        case PCIT::MASCII:
        case PCIT::MATTR5:
        {
          UInt32 index = (hashVals[i] % tableSize) << 1;

          if (table[index] == INVALID_INT32) {
            table[index] = strLens[i];
            table[index + 1] = constant->getOffset();

            // Record target block for this case in jump table
            if (!forInList) {
              jumpTable[(index >> 1)] =
                (Long)(blocks[i]->getFallThroughBlock());
            }
          }
          else {
            // Collision.
            collisionOccurred = TRUE;

            // Try and store in next N-1 entries, if not at the end.
            for (j=1; j < maxChainLength; j++) {
              index += 2;
              if ((index < (tableSize << 1)) && (table[index] == INVALID_INT32))
              {
                table[index] = strLens[i];
                table[index + 1] = constant->getOffset();

                // Record target block for this case in jump table
                if (!forInList) {
                  jumpTable[(index >> 1)] =
                    (Long)(blocks[i]->getFallThroughBlock());
                }

                break;
              }
            }

            // If max chain length reached, we need to restart
            if (j == maxChainLength)
              restart = TRUE;
          }

          break;
        }

        // Integers
        default:
        {
          Int64* table64 = (Int64*)table;

          Int64 val = intVals[i];
          UInt32 hashVal = ExHDPHash::hash8((char*)&val, ExHDPHash::NO_FLAGS);
          UInt32 index = (hashVal % tableSize);

          if (table64[index] == INVALID_INT64) {
            table64[index] = val;

            // Record target block for this case in jump table
            if (!forInList) {
              jumpTable[index] = (Long)(blocks[i]->getFallThroughBlock());
            }
          }
          else {
            // Collision.
            collisionOccurred = TRUE;

            // Try and store in next N entries, if not at the end.
            for (j=1; j < maxChainLength; j++) {
              index++;
              if ((index < tableSize) && (table64[index] == INVALID_INT64)) {
                table64[index] = val;

                // Record target block for this case in jump table
                if (!forInList) {
                  jumpTable[index] = (Long)(blocks[i]->getFallThroughBlock());
                }

                break;
              }
            }

            // If max chain length reached, we need to restart
            if (j == maxChainLength)
              restart = TRUE;
          }

          break;
        }
      }
    }

    // Determine if there were too many collisions which could disrupt perf.
    // The heuristic used measures how many entries are taken in the table, and
    // how close in proximity are they to each other.

    if (!restart && collisionOccurred)
    {
      Int64* table64 = (Int64*)table;

      Int32 totalWorstCaseChecks = 0;
      Int32 totalCount = 0;

      // Walk through the table in search for valid entries.  Calculate the max
      // number of searches needed for such entries, and total everything up.
      for (i=0; i < tableSize; i++) {
        if (table64[i] != INVALID_INT64) {
          // Found a valid entry.
          totalCount++;

          totalWorstCaseChecks++;
          for (j=1; j < maxChainLength; j++) {
            if (((i+j) < tableSize) && (table64[i+j] != INVALID_INT64))
              totalWorstCaseChecks++;
            else
              break;
          }
        }
      }

      // If the average check is greater than 2, we should restart
      if (((float)totalWorstCaseChecks / (float)(totalCount)) > 2.0)
        restart = TRUE;
    }

    // If restart is TRUE, that implies that the hash table created was not big
    // engouh to ensure minimal collision at runtime.
    if (restart) {
      NADELETEBASIC(table, heap_);
      table = NULL;
      primeIdx++;
    }
  }

  // If table is NULL, that implies that not even the biggest hash table size
  // available was found to meet the requirements.
  if (table == NULL) {
#if defined(_DEBUG)
    assert(FALSE); // I DON'T BELIEVE IT! :)
#endif
    return NULL;
  }

  // If we're dealing with a case statement, we must record the default case
  // in the jump table.  It will be the successor block of the last block in
  // the switch table.
  if (!forInList)
    jumpTable[tableSize] = (Long)(lastBlock->getTargetBlock());

  // Add table to constants area.
  CollIndex* offsetPtr;
  offsetPtr = addConstant(table, totalTableSizeInBytes, 8);

  return offsetPtr;
}

//
// Helper function to determine if enough entries in the in-list or case stmt
// exist to justify the indirect branch optimization
//
NABoolean PCodeCfg::indirectBranchReqMet(PCodeInst* eqComp,
                                         PCodeOperand* var,
                                         CollIndex entries)
{
  //
  // Requirements based on column type:
  //
  // Fixed-length Strings : 6 items
  // The rest             : 3 items
  //
  // On platforms other than NSK, go to town everytime.
  //

  if (entries < 2)
    return FALSE;

  return TRUE;
}

//
// Indirect branch optimization that converts case statements and IN-lists with
// a large number of equality comparisons into a single indirect branch against
// a jump table.  Liveness information must be available.
//
void PCodeCfg::indirectBranchOpt(NABoolean forInList)
{
  CollIndex i, j, *offsetPtr;
  PCodeInst *eqComp, *newInst;
  PCIT::Instruction opc;

  CollIndex maxItemsForInListOpt = 1000;

  FOREACH_BLOCK_REV_DFO(headBlock, firstInst, lastInst, index)
  {
    PCodeOperand* constant = NULL;
    PCodeOperand* var = NULL;
    UInt32 tableSize = 0;

    // Block was previously visited.  It must've resulted in an "abort", so skip
    if (headBlock->getVisitedFlag())
      continue;

    // Check if headBlock is a good candidate.
    if ((constant = headBlock->isIndirectBranchCandidate(NULL, var, forInList)))
    {
      NAList<PCodeOperand*> constants(heap_);
      NAList<PCodeBlock*> blocks(heap_);

      // Get the column variable from the equality comparison instruction.
      if (lastInst->prev->getROps()[0]->isConst())
        var = lastInst->prev->getROps()[1];
      else
        var = lastInst->prev->getROps()[0];

      // Insert constant and block found into lists;
      constants.insert(constant);
      blocks.insert(headBlock);

      // Mark block as having been processed within a potential IN-list
      headBlock->setVisitedFlag(TRUE);

      // Find all potential candidates of IN-list.
      PCodeBlock* parent = headBlock;
      while (TRUE)
      {
        PCodeBlock* child = (forInList) ? parent->getFallThroughBlock()
                                        : parent->getTargetBlock();

        // Parent must dominate child block
        if (child->getPreds().entries() != 1)
          break;

        // If child block is NOT a candidate, break out.
        if (!(constant = child->isIndirectBranchCandidate(parent, var,
                                                            forInList))) {
          break;
        }

        // Target/Fall-Through of child block must be the same as the target
        // of the parent block if dealing with IN-lists.  This restriction is
        // not needed for case statements.
        if (forInList) {
          NABoolean isAndBranch =
            (child->getLastInst()->getOpcode() == PCIT::BRANCH_AND);

          // False branch needs to fall-through to TRUE block
          if (isAndBranch && 
              (child->getFallThroughBlock() != parent->getTargetBlock()))
            break;

          // True branch needs to target to TRUE block
          if (!isAndBranch && 
              (child->getTargetBlock() != parent->getTargetBlock()))
            break;
        }

        // Insert constant found into list;
        constants.insert(constant);

        // Keep track of child block
        blocks.insert(child);

        // Mark block as having been processed within a potential IN-list
        child->setVisitedFlag(TRUE);

        // Lastly, let's be conservative and note that for child blocks that
        // end in a false-branch, if we're processing for in-lists, that child
        // block will most likely terminate the list.  Maybe we won't optimize
        // for complicated scan exprs, but it's probably safer this way.
        if (forInList && (child->getLastInst()->getOpcode()==PCIT::BRANCH_AND))
          break;

        // One more condition.  If the list grows too big, the chances of
        // finding a suitable hash table grows slim.  It's best to divide a
        // large IN-list into several small ones.
        if (constants.entries() >= maxItemsForInListOpt)
          break;

        parent = child;
      }

      // If a minimum limit isn't met in terms of how many items in the
      // IN-list are needed for this optimization, don't perform the opt.
      eqComp = lastInst->prev;
      if (!indirectBranchReqMet(eqComp, var, constants.entries()))
        continue;

      // Identify a final write operand, if needed, using liveness analysis.
      // If only one logical branch target operand from the candidates is
      // live, use it for the indirect branch.  If more than one found, then
      // they all have to be the same operand.  Otherwise we have a strange
      // situation with multiple live target operands, and that won't work
      // with the indirect branch.

      NABoolean abort = FALSE;
      PCodeOperand* liveOp = NULL;

      // Walk through each block in IN-list
      for (i=0; !abort && (i < blocks.entries()); i++)
      {
        // Get write and read operands in BRANCH instruction.
        PCodeOperand* wOp = blocks[i]->getLastInst()->getWOps()[0];
        PCodeOperand* rOp = blocks[i]->getLastInst()->getROps()[0];

        // Check if operands are live, starting at each successor block.
        for (j=0; !abort && (j < blocks[i]->getSuccs().entries()); j++) {
          PCodeBlock* succ = blocks[i]->getSuccs()[j];
          if (succ->getLiveVector().testBit(wOp->getBvIndex()))
          {
            if (!liveOp)
              liveOp = wOp;
            else if (liveOp->getBvIndex() != wOp->getBvIndex())
              abort = TRUE;
          }
          if (succ->getLiveVector().testBit(rOp->getBvIndex()))
          {
            if (!liveOp)
              liveOp = rOp;
            else if (liveOp->getBvIndex() != rOp->getBvIndex())
              abort = TRUE;
          }
        }
      }

      // Although we asked to abort, maybe we can do something still.  Take out
      // the last case/constant in list (the one which resulted in the extra
      // live op, and see if we still meet the minimum allowed for the opt to
      // go through.  This is considered a partial opt.
      if (abort && indirectBranchReqMet(eqComp, var, i-1)) {
        CollIndex abortedIndex = i - 1;
        while (constants.removeAt(abortedIndex)) {
          // Allow other blocks to potentially group together.
          blocks[abortedIndex]->setVisitedFlag(FALSE);

          blocks.removeAt(abortedIndex);
        }
        abort = FALSE;
      }

      // Move onwards if we had to abort
      if (abort)
        continue;

      // It may be incorrect to reuse a live operand for the purposes of the
      // SWITCH instruction.  For example, suppose we have "a in (1,2,3)".  The
      // comparison of "a=1" in the in-list could be CSE'd for use in a later
      // comparison - i.e. it is live.  So to use it as a representation of
      // whether a is in (1,2,3) would be incorrect, since now the predicate "a"
      // has changed meanings.
      //
      // Having said this, only the first comparison in an in-list/case-stmt
      // could possibly be CSE'd, since otherwise we have speculative paths that
      // *may* show the dependency.  As such, check the first block to see if
      // the comparison result is the live operand discovered.  If so, keep it
      // around, and we can still perform the SWITCH.

      NABoolean keepCompare = FALSE;

      if (liveOp) {
        if (headBlock->isOperandLiveInSuccs(eqComp->getWOps()[0])) {
          // The result of the comparison is CSE'd with something.  Keep the
          // compare around and have the liveOp point to a new temp.
          keepCompare = TRUE;

          PCodeOperand op(2, addTemp(4,4), -1, -1);
          liveOp = &op;
        }
        else if (!forInList) {
          // Case statements should not have any live operands, other than
          // within the 1st block where the comparison was CSE'd.
          continue;
        }
      }
      else {
        // Use result of EQ instruction in headBlock for result of switch.
        liveOp = eqComp->getWOps()[0];
      }

      // First save original list of child blocks
      BLOCKLIST childBlocks(heap_);
      childBlocks.insert(blocks);

      // Create a constants hash table.  If it fails, return, since that would
      // imply that not even a LARGE hash table satisfied the requirements.
      offsetPtr = createLookupTableForSwitch(constants, childBlocks,
                                             tableSize, forInList);
      if (offsetPtr == NULL)
        return;

      if (eqComp->isIntEqComp() || eqComp->isFloatEqComp()) {
        // Create MOVE instruction from column type to MBIN64S
        Int32 int64TempOff = addTemp(8, 8);

        if (eqComp->isFloatEqComp())
          opc = eqComp->generateFloat64MoveOpc(var->getType());
        else
          opc = eqComp->generateInt64MoveOpc(var->getType());

        newInst = headBlock->insertNewInstBefore(lastInst, opc);

        newInst->code[1] = 2;
        newInst->code[2] = int64TempOff;
        newInst->code[3] = var->getStackIndex();
        newInst->code[4] = var->getOffset();

        newInst->reloadOperands(this);

        // Create SWITCH Instruction
        opc = PCIT::SWITCH_MBIN32S_MBIN64S_MPTR32_IBIN32S_IBIN32S;
        newInst = headBlock->insertNewInstBefore(lastInst, opc);

        newInst->code[1] = liveOp->getStackIndex();
        newInst->code[2] = liveOp->getOffset();
        newInst->code[3] = 2;            // represents temp stack index
        newInst->code[4] = int64TempOff;
        newInst->code[5] = 1;            // represents constant stack index
        newInst->code[6] = *offsetPtr;
        newInst->code[7] = tableSize;
        newInst->code[8] = 0;            // initialize flags

        // Modify flag to indicate identify of switch.
        newInst->code[8] |= (forInList ? 0x1 : 0x0);

        newInst->reloadOperands(this);
      }
      else
      {
        // Create SWITCH instruction.

        opc = PCIT::SWITCH_MBIN32S_MATTR5_MPTR32_IBIN32S_IBIN32S;
        newInst = headBlock->insertNewInstBefore(lastInst, opc);

        newInst->code[1] = liveOp->getStackIndex();
        newInst->code[2] = liveOp->getOffset();
        newInst->code[3] = var->getStackIndex();
        newInst->code[4] = var->getOffset();
        newInst->code[5] = var->getVoaOffset();
        newInst->code[6] = (var->isVarchar()) ? var->getVcMaxLen()
                                              : var->getLen();

        if (var->isVarchar()) {
          UInt32 comboLen1 = 0;
          char* comboPtr1 = (char*)&comboLen1;
          comboPtr1[0] = var->getVcNullIndicatorLen();
          comboPtr1[1] = var->getVcIndicatorLen();
          newInst->code[7] = comboLen1;
        }
        else
          newInst->code[7] = 0;

        newInst->code[8] = 1;          // represents constant stack index
        newInst->code[9] = *offsetPtr;
        newInst->code[10] = tableSize;
        newInst->code[11] = 0;         // initialize flags

        // Modify flag to indicate identity of switch.
        newInst->code[11] |= (forInList ? 0x1 : 0x0);

        newInst->reloadOperands(this);
      }

      if (forInList)
      {
        // Create new fall-through block
        PCodeBlock* newBlock = createBlock();
        newBlock->insertNewInstAfter(NULL, PCIT::BRANCH);

        // Delete compare instruction in headBlock if not needed
        if (!keepCompare)
          headBlock->deleteInst(eqComp);

        // Fix up the rest of headBlock.
        // note that the last instruction is either BRANCH_OR or BRANCH_AND
        // and we set the source operand in PCode binary
        assert(lastInst->isAnyLogicalBranch());  // just in case
        lastInst->code[3 + 2 * PCODEBINARIES_PER_PTR] = liveOp->getStackIndex();
        lastInst->code[4 + 2 * PCODEBINARIES_PER_PTR] = liveOp->getOffset();
        lastInst->reloadOperands(this);
        headBlock->removeEdge(headBlock->getFallThroughBlock());
        headBlock->addFallThroughEdge(newBlock);

        PCodeBlock *falseBlock =
          (blocks[blocks.entries()-1]->getTargetBlock() ==
           headBlock->getTargetBlock())
            ? blocks[blocks.entries()-1]->getFallThroughBlock()
            : blocks[blocks.entries()-1]->getTargetBlock();

        newBlock->addEdge(falseBlock);
      }
      else
      {
        // Delete compare instruction in headBlock if not needed
        if (!keepCompare)
          headBlock->deleteInst(eqComp);

        // Fix up the rest of headBlock.
        headBlock->removeEdge(headBlock->getTargetBlock());
        headBlock->removeEdge(headBlock->getFallThroughBlock());

        lastInst->code[0] = PCIT::BRANCH_INDIRECT_MBIN32S;
        lastInst->code[1] = liveOp->getStackIndex();
        lastInst->code[2] = liveOp->getOffset();
        lastInst->reloadOperands(this);

        // Get target location pointers in jump table and add all succ edges.
        NAList<Long*> targetOffPtrs(heap_);
        lastInst->getBranchTargetPointers(&targetOffPtrs, this);

        for (i=0; i < targetOffPtrs.entries(); i++)
          headBlock->addEdge((PCodeBlock*)(*(targetOffPtrs[i])));
      }

      // Delete all other blocks now, or let other phase do it.
      for (i=1; i < blocks.entries(); i++) {
        deleteBlock(blocks[i]);
      }
    }
  } ENDFE_BLOCK_REV_DFO
}


void PCodeCfg::cfgRewiring(Int32 flags)
{
  CollIndex i;

  // Remove unreachable blocks
  if (flags & REMOVE_UNREACHABLE_BLOCKS) {
    clearVisitedFlags();
    markReachableBlocks(entryBlock_);

    FOREACH_BLOCK_ALL(block, firstInst, lastInst, index) {
      if (!block->getVisitedFlag()) {
        deleteBlock(block);
      }
    } ENDFE_BLOCK
  }

  // Remove Trampoline Code
  if (flags & REMOVE_TRAMPOLINE_CODE) {
    FOREACH_BLOCK(block, firstInst, lastInst, index) {
      if (!block->isEmpty() && lastInst->isBranch())
      {
        PCodeBlock* tgtBlock = block->getTargetBlock();
        PCodeBlock* fallThroughBlock = block->getFallThroughBlock();

        PCodeInst* tgtInst = tgtBlock->getFirstInst();
        PCodeInst* fallInst = fallThroughBlock->getFirstInst();

        //
        // Sometimes we have branches that fall-through and branch to the same
        // location.  Just lookat logical branches for now.
        //
        // T0: BRANCH_AND T1
        // T1:
        //
        if (lastInst->isLogicalBranch() && (tgtBlock == fallThroughBlock)) {
          PCodeInst* newInst = block->reduceBranch(tgtBlock,
                                                   *zeroes_, *ones_, *neg1_);

          block->removeEdge(tgtBlock);
          block->deleteInst(lastInst);
          block->insertInstAfter(NULL, newInst);

          continue;
        }

        //
        // T0: BRANCH T1
        // T2: UNC_BRANCH T3
        // ..
        // T3 (no blocks fall through to it)
        //
        if (!lastInst->isUncBranch() && fallInst && fallInst->isUncBranch())
        {
          PCodeBlock* trampBlock = fallThroughBlock->getTargetBlock();
          NABoolean remove = TRUE;

          // If all edges into trampBlock are target edges, then we can remove
          // the target block.
          for (i=0; i < trampBlock->getPreds().entries(); i++) {
            PCodeBlock* pred = trampBlock->getPreds()[i];
            PCodeBlock* predFallThru = pred->getFallThroughBlock();
            if (predFallThru && predFallThru == trampBlock) {
              if (!pred->getLastInst() || !pred->getLastInst()->isUncBranch())
                remove = FALSE;
            }
          }

          if (remove == TRUE) {
            block->removeEdge(fallThroughBlock);
            block->addFallThroughEdge(trampBlock);

            // Delete trampoline block if it's now empty
            if (fallThroughBlock->getPreds().entries() == 0)
              deleteBlock(fallThroughBlock);
          }
        }

        //
        // T0: UNC_BRANCH T1
        // ..
        //
        // T1: BRANCH
        //
        if (lastInst->isUncBranch() && tgtInst && tgtInst->isLogicalBranch() &&
            !tgtInst->isUncBranch() && FALSE)
        {
          // Fix up source block
          block->removeEdge(tgtBlock);
          block->deleteInst(lastInst);
          block->insertInstAfter(NULL, copyInst(tgtInst));
          
          // Create new fall-through block
          PCodeBlock* newBlock = createBlock();
          newBlock->insertInstAfter(NULL, lastInst);

          // Fix up all edges
          block->addFallThroughEdge(newBlock);
          block->addEdge(tgtBlock->getTargetBlock());
          newBlock->addEdge(tgtBlock->getFallThroughBlock());

          // Delete trampoline block if it's now empty
          if (tgtBlock->getPreds().entries() == 0)
            deleteBlock(tgtBlock);
        }
        else if (tgtInst && (tgtInst->isUncBranch() ||
            ((flags & LIVE_ANALYSIS_AVAILABLE) && tgtInst->isLogicalBranch() &&
             lastInst->isLogicalBranch())))
        {
          PCodeBlock* trampBlock = tgtBlock->getTargetBlock();

          if (tgtInst->isLogicalBranch()) {
            NABitVector liveVector(heap_);
            CollIndex wBvIndex = tgtInst->getWOps()[0]->getBvIndex();

            liveVector = tgtBlock->getSuccs()[0]->getLiveVector();
            liveVector += tgtBlock->getSuccs()[1]->getLiveVector();

            if (liveVector.testBit(wBvIndex))
              continue;

            if ((tgtInst->getROps()[0]->getBvIndex() !=
                 lastInst->getWOps()[0]->getBvIndex()) &&
                (tgtInst->getROps()[0]->getBvIndex() !=
                 lastInst->getROps()[0]->getBvIndex()))
              continue;

            if (tgtInst->getOpcode() != lastInst->getOpcode())
              trampBlock = tgtBlock->getFallThroughBlock();
          }

          block->removeEdge(tgtBlock);
          block->addEdge(trampBlock);

          // Delete trampoline block if it's now empty
          if (tgtBlock->getPreds().entries() == 0)
            deleteBlock(tgtBlock);
        }
      }
    } ENDFE_BLOCK
  }

  // Merge contiguous blocks for better downstream local optimizations
  // Consider doing down-wards code motion here
  if (flags & MERGE_BLOCKS) {
    FOREACH_BLOCK(block, firstInst, lastInst, index) {
      // Skip empty blocks and blocks which don't meet the requirements
      if (block->isEmpty() ||
          (block->getSuccs().entries() != 1) ||
          (block->getFallThroughBlock()->getPreds().entries() != 1))
        continue;

      // Also skip block if an indirect branch targets it.  This is because it's
      // hard to fixup edges to/from an indirect branch.
      NABoolean found = FALSE;
      for (i=0; i < block->getPreds().entries(); i++) {
        PCodeBlock* pred = block->getPreds()[i];
        if (pred->getLastInst() && pred->getLastInst()->isIndirectBranch())
          found = TRUE;
      }

      // If block is targeted by an indirect branch, continue.
      if (found)
        continue;

      PCodeBlock* ftBlock = block->getFallThroughBlock();

      while (TRUE)
      {
        // Search for non-empty block down longest chain of contiguous blocks
        if (ftBlock->isEmpty() && (ftBlock->getSuccs().entries() == 1) &&
            ftBlock->getFallThroughBlock()->getPreds().entries() == 1)
        {
          ftBlock = ftBlock->getFallThroughBlock();
          continue;
        }
        break;
      }

      // If block we settle on to copy instructions into is empty, return
      if (ftBlock->isEmpty())
        continue;
  
      // TODO: Instead of going through each instruction one by one, just 
      // readjust pointers directly between the two blocks.  However, this is
      // currently testing insert of instructions
      FOREACH_INST_IN_BLOCK_BACKWARDS(block, inst) {
        // Skip over a branch
        if (inst->isBranch()) {
          assert (inst->isUncBranch());
          continue;
        }

        // Insert instruction into beginning of fall-through block.
        ftBlock->insertInstBefore(NULL, inst);

      } ENDFE_INST_IN_BLOCK_BACKWARDS

      // Make the block dead
      block->setFirstInst(NULL);
      block->setLastInst(NULL);

      // If we're moving from the entryBlock_, we'll now have a new entryBlock.
      if (block == entryBlock_)
        entryBlock_ = ftBlock;

    } ENDFE_BLOCK
  }

  // Remove empty blocks
  if (flags & REMOVE_EMPTY_BLOCKS)
  FOREACH_BLOCK(block, firstInst, lastInst, index) {
    if (block->isEmpty()) {
      deleteBlock(block);
    }
  } ENDFE_BLOCK
}

// 
// Find patterns and replace with shorter/faster sequences.
// 
void PCodeCfg::peepholeConstants()
{
  FOREACH_BLOCK(block, firstInst, lastInst, index)
  {
    FOREACH_INST_IN_BLOCK_BACKWARDS(block, inst)
    {
      //
      // 1. Attempt to expand null indicator moves.  We do this after
      //    flattening null branches since sometimes later phases expose more
      //    opportunities.  Note, extendPaddedNullIndMoves() may modify the
      //    instruction.  This should not change the previous/next insts, so
      //    the iterator above should not need to be modified.

      inst = extendPaddedNullIndMoves(inst);

      //
      // 2. Convert zero-fill FILL_MEM_BYTES to normal MOVES if possible
      //
      if ((inst->getOpcode() == PCIT::FILL_MEM_BYTES) &&
          (inst->code[4] == 0))
      {
        // Switch on length of zero-fill
        switch (inst->code[3]) {
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
          default:
            break;
        }

        // If the opcode was not changed, don't continue further
        if (inst->code[0] != PCIT::FILL_MEM_BYTES) {
          // Move constant 0 into target (located at [1,0])
          inst->code[3] = 1;
          inst->code[4] = zeroOffset_;

          // Reload operands so that MOVE instruction is now recognized
          inst->reloadOperands(this);
        }
      }
      //
      // 3. Delete useless moves
      //

      if (inst->isMove()) {
        if (inst->getWOps()[0]->getBvIndex() ==
            inst->getROps()[0]->getBvIndex()) {
          block->deleteInst(inst);
          continue;
        }
      }

      //
      // 4. Convert BIGNUM moves to memcpy if lengths of src and dest are same
      //
      if (inst->getOpcode() == PCIT::MOVE_MBIGS_MBIGS_IBIN32S_IBIN32S) {
        if (inst->code[5] == inst->code[6]) {
          inst->code[0] = PCIT::MOVE_MBIN8_MBIN8_IBIN32S;
          // No need to reload the operands, as they will be the same
        }
      }
#if 0
      if (((inst->getOpcode() == PCIT::MOVE_MBIGS_MBIGS_IBIN32S_IBIN32S) &&
           (inst->code[5] == inst->code[6])) ||
          ((inst->getOpcode() == PCIT::ENCODE_NXX) &&
           (inst->code[6] == 0))) {
        inst->code[0] = PCIT::MOVE_MBIN8_MBIN8_IBIN32S;
      }
#endif

      //
      // 5. Simplify MOVE/SUM into single SUM
      //
      if (inst->getOpcode() == PCIT::SUM_MBIN64S_MBIN64S)
      {
        CollIndex numOfOps = inst->getROps().entries();
        PCodeInst* def = block->findDef(inst,inst->getROps()[numOfOps-1],FALSE);
        if ((def == (PCodeInst*)-1) || !def ||
            (def->getOpcode() != PCIT::MOVE_MBIN64S_MBIN32S))
          continue;

        inst->code[0] = PCIT::SUM_MBIN64S_MBIN32S;
        inst->code[3] = def->code[3];
        inst->code[4] = def->code[4];

        inst->reloadOperands(this);
        continue;
      }

      if (inst->getOpcode() == PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN64S)
      {
        CollIndex numOfOps = inst->getROps().entries();
        PCodeInst* def = block->findDef(inst,inst->getROps()[numOfOps-1],FALSE);
        if ((def == (PCodeInst*)-1) || !def ||
            (def->getOpcode() != PCIT::MOVE_MBIN64S_MBIN32S))
          continue;

        inst->code[0] = PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN32S;
        inst->code[10] = def->code[3];
        inst->code[11] = def->code[4];

        inst->reloadOperands(this);
        continue;
      }
    } ENDFE_INST_IN_BLOCK_BACKWARDS
  } ENDFE_BLOCK
}

// Record atp/atp-index/offset triples for null indicators and map them to
// a pad length used to buffer it with the actual column value.
void PCodeCfg::initNullMapTable()
{
  short i;

  nullToPadMap_ = new(heap_) NAHashDictionary<NullTriple, Int32>
                             (&nullTripleHashFunc, 100, TRUE, heap_);

  ex_clause* clause = expr_->getClauses();
  while (clause) {
    short numOfOps = clause->getNumOperands();
    NABoolean isBranchingClause = clause->isBranchingClause();

    for (i=0; i < numOfOps; i++) {
      Attributes* op = clause->getOperand(i);

      if (op->getNullFlag() &&
          (op->getTupleFormat() == ExpTupleDesc::SQLARK_EXPLODED_FORMAT))
      {
        NullTriple* nt = new(heap_) NullTriple(op->getAtp(),
                                               op->getAtpIndex(),
                                               op->getNullIndOffset());

        Int32* pad = new(heap_) Int32(op->getOffset() - op->getNullIndOffset());

        nullToPadMap_->insert(nt, pad);
      }
    }

    clause = clause->getNextClause();
  }
}

/*****************************************************************************
 * Phase 0 (Utility Functions)
 *****************************************************************************/
//
// Create a new PCodeInst with the passed in opcode.  Note, this form can not
// be used for Bulk instructions
//
PCodeInst* PCodeCfg::createInst (PCIT::Instruction instr)
{
  PCodeBinary code = instr;
  return createInst(instr, PCode::getInstructionLength(&code));
}

//
// Create a new PCodeInst with the passed in opcode and length
//
PCodeInst* PCodeCfg::createInst (PCIT::Instruction instr, Int32 length)
{
  // Create new instruction and copy bytecode
  PCodeInst * newInst = new(heap_) PCodeInst(heap_);

  newInst->prev = NULL;
  newInst->next = NULL;

  newInst->code = (PCodeBinary*) new(heap_) PCodeBinary[length];
  newInst->code[0] = instr;

  allInsts_->insert(newInst);

  return newInst;
}

//
// Copy the given instruction and return the copy.
//
PCodeInst* PCodeCfg::copyInst (PCodeInst* inst)
{
  Int32 length = PCode::getInstructionLength(inst->code);

  // Create new instruction, copy bytecode, and load operands
  PCodeInst * newInst = createInst((PCIT::Instruction)inst->getOpcode());
  memcpy(newInst->code, inst->code, length * sizeof(PCodeBinary));

  // Copy any key member variables associated with opdata instructions
  newInst->clause_ = inst->clause_;
  newInst->opdataLen_ = inst->opdataLen_;

  loadOperandsOfInst(newInst);

  return newInst;
}

PCodeBlock* PCodeCfg::createBlock()
{
  PCodeBlock* block = new(heap_) PCodeBlock(++maxBlockNum_, NULL, heap_, this);
  allBlocks_->insert(block);

  // TODO: remove this once init of visited flags is in constructor
  block->setVisitedFlag(FALSE);
  return block;
}

//
// Remove the given block from the cfg, taking care of removing edges correctly.
//
void PCodeCfg::deleteBlock(PCodeBlock* block)
{
  // If we're deleting the entryBlock_, find the first non-empty block and
  // call it the new entry block
  if (block == entryBlock_) {
    PCodeBlock* succ = block;
    do {
      succ = succ->getFallThroughBlock();

      // There better be a successfor fallthrough block.
      assert(succ);

      if (!succ->isEmpty()) {
        entryBlock_ = succ;
        break;
      }
    } while (TRUE);
  }

  // First attempt to rewire edges if we're dealing with an empty block.  We
  // also don't need to process it if the block is an exit block.
  if (block->isEmpty() && (block->getSuccs().entries() == 1))
  {
    PCodeBlock* succ = block->getSuccs()[0];
    while (block->getPreds().entries() > 0) {
      PCodeBlock* pred = block->getPreds()[0];

      if (pred->getFallThroughBlock() == block)
        pred->addFallThroughEdge(succ);
      else
        pred->addEdge(succ);

      pred->removeEdge(block);
    }
  }

  // Get rid of succs
  while (block->getSuccs().entries())
    block->removeEdge(block->getSuccs()[0]);

  // Get rid of preds 
  while (block->getPreds().entries())
    block->getPreds()[0]->removeEdge(block);

  // Null out first and last instructions
  block->setFirstInst(NULL);
  block->setLastInst(NULL);
}

void PCodeCfg::clearVisitedFlags()
{
  FOREACH_BLOCK(block, firstInst, lastInst, index) {
    block->setVisitedFlag(FALSE);
  } ENDFE_BLOCK
}

//
// Recursive algorithm that visits all successors of the provided block and 
// marks them as visited.
//
void PCodeCfg::markReachableBlocks(PCodeBlock* block)
{
  block->setVisitedFlag(TRUE);
  for (CollIndex i=0; i < block->getSuccs().entries(); i++)
    if (!block->getSuccs()[i]->getVisitedFlag())
      markReachableBlocks(block->getSuccs()[i]);
}


//
// Create a copy of the cfg starting at the "start" block and ending at the 
// "end" block.  The map array is used to map original blocks to the copied 
// blocks.
//
PCodeBlock* PCodeCfg::copyCfg(PCodeBlock* start,
                              PCodeBlock* end,
                              PCodeBlock** map)
{
  CollIndex i;
  PCodeBlock* block = createBlock();

  if (start != end) {
    start->setVisitedFlag(TRUE);

    // Assign new copy block to map table if start and end are different - i.e
    // No map needed if we're just copying a block.
    map[start->getBlockNum()] = block;
  }

  // Copy each instruction into the new block.
  FOREACH_INST_IN_BLOCK(start, inst) {
    PCodeInst* copy = copyInst(inst);
    block->insertInstAfter(NULL, copy);
  } ENDFE_INST_IN_BLOCK

  // If we haven't reached the end block yet, recursively process each of the
  // block's successors.  If the successor was already visited, retrieve its
  // copy block from the map table.

  if (start != end) {
    for (i=0; i < start->getSuccs().entries(); i++) {
      PCodeBlock* succ = start->getSuccs()[i];
      block->addEdge((succ->getVisitedFlag()) ? map[succ->getBlockNum()]
                                              : copyCfg(succ, end, map));
    }
  }

  return block;
}


/*****************************************************************************
 * Phase 1 (Setup CFG)
 *****************************************************************************/

//
// Create a control flow graph (CFG) with blocks and edges.
//

void PCodeCfg::createCfg()
{
  PCodeInst* tempInst;
  PCodeBlock* tempBlock=NULL;
  PCodeBlock* prevBlock=NULL;

  // Reset the total number of blocks
  maxBlockNum_ = 0;

  // If no instructions, return
  if (allInsts_->entries() == 0)
    return;

  assert (allInsts_->at(allInsts_->entries() - 1)->getOpcode() == PCIT::END);

  // Replace PCIT::END at end with PCIT::RETURN.  This is temporarily done
  // since optimized code may have multiple exit points.  Once graph is ready
  // to be dumped, add PCIT::END at end of pcode so that downstream
  // functionality continues to work.

  allInsts_->at(allInsts_->entries() - 1)->code[0] = PCIT::RETURN;

  // Go through each instruction and attach them to a new or existing block.
  // Every iteration back to the beginning of the loop implies that the
  // instruction should go into a new block.
  for (tempInst = allInsts_->at(0); tempInst; tempInst = tempInst->next)
  {
    tempBlock =
      new(heap_) PCodeBlock(++maxBlockNum_, tempInst, heap_, this);

    tempInst->block = tempBlock;

    // Record entry block
    if (allBlocks_->isEmpty()) {
      entryBlock_ = tempBlock;
    }

    allBlocks_->insert(tempBlock);

    // Loop passed all instructions which are neither branches nor targets of
    // branches - instructions which would determine the end of a block

    ULong targAddr = (tempInst->next) ? (ULong)(tempInst->next->code) : 0;

    while (!tempInst->isBranch() &&
           !tempInst->isIndirectBranch() &&
           (tempInst->getOpcode() != PCIT::RETURN) &&
           (tempInst->getOpcode() != PCIT::RETURN_IBIN32S) &&
           (tempInst->next != NULL) &&
           !(targets_->contains(&targAddr)))
    {
      tempInst = tempInst->next;
      tempInst->block = tempBlock;
      targAddr = (tempInst->next) ? (ULong)tempInst->next->code : 0;
    }

    tempBlock->setLastInst(tempInst);

    tempInst->block = tempBlock;

    if (prevBlock) {
      prevBlock->addSucc(tempBlock);
      tempBlock->addPred(prevBlock);
    }

    // Make sure we initialize the new previous block *if* the current block
    // has a fall-through
    if (tempInst->isUncBranch() || tempInst->isIndirectBranch() ||
        (tempInst->getOpcode() == PCIT::RETURN) ||
        (tempInst->getOpcode() == PCIT::RETURN_IBIN32S))
    {
      prevBlock = NULL;
    }
    else
      prevBlock = tempBlock;
  }

  // Add target edges, since the previous code only created fall-through edges
  // Also nullify the prev and next pointers of the first and last instructions
  // of each block, respectively

  NAList<PCodeBinary*> targetOffPtrs(heap_);
  NAList<Long*> targetPtrs(heap_);

  FOREACH_BLOCK_ALL(branchBlock, bFirstInst, bLastInst, bIndex)
  {
    bFirstInst->prev = NULL;
    bLastInst->next = NULL;

    if (bLastInst->isBranch())
    {
      targetOffPtrs.clear();
      bLastInst->getBranchTargetOffsetPointers(&targetOffPtrs, this);

      for (CollIndex i=0; i < targetOffPtrs.entries(); i++) {
        PCodeBinary* target = bLastInst->code + 1 + *(targetOffPtrs[i]);

        FOREACH_BLOCK_ALL(targBlock, tFirstInst, tLastInst, tIndex)
        {
          if (tFirstInst) {
            if (tFirstInst->code == target) {
              branchBlock->addSucc(targBlock);
              targBlock->addPred(branchBlock);
              break;
            }
          }
        } ENDFE_BLOCK
      }
    }
    else if (bLastInst->isIndirectBranch())
    {
      targetPtrs.clear();
      bLastInst->getBranchTargetPointers(&targetPtrs, this);

      for (CollIndex i=0; i < targetPtrs.entries(); i++) {
        PCodeBinary* target = bLastInst->code + 1 + *(targetPtrs[i]);

        FOREACH_BLOCK_ALL(targBlock, tFirstInst, tLastInst, tIndex)
        {
          if (tFirstInst) {
            if (tFirstInst->code == target) {
              branchBlock->addSucc(targBlock);
              targBlock->addPred(branchBlock);
              break;
            }
          }
        } ENDFE_BLOCK
      }
    }

  } ENDFE_BLOCK
}

//
// Determine if the pcode graph can be optimized.  As of right now, the main
// limiting factor is if the pcode graph contains a clause that will violate
// the assumptions made by this infrastructure.
//
NABoolean PCodeCfg::canPCodeBeOptimized( PCodeBinary * pCode
                                       , NABoolean   & pExprCacheable /* OUT */
                                       , UInt32      & totalPCodeLen  /* OUT */ )
{
  // Caller may have already set these, but initialize the "out" params just to be safe.
  pExprCacheable = TRUE ;
  totalPCodeLen  = 0 ;

  // Obviously if we have no pCode then we should return FALSE :)
  if (pCode == NULL)
    return FALSE;

  for (ex_clause* clause = expr_->getClauses();
       clause;
       clause = clause->getNextClause())
  {
    // We can't distinguish read/write operands for INOUT clauses 
    if (clause->getClassID() == ex_clause::INOUT_TYPE)
      return FALSE;

    // Clauses evaluated before HbaseColumnCreate directly update the result
    // buffer of this clause. Pcode optimization will remove those pre-clauses
    // as their output is not directly referenced in another clause.
    // Do not do pcode opt if this clause is used.
    else if (clause->getClassID() == ex_clause::FUNC_HBASE_COLUMN_CREATE)
      return FALSE;

    else if (clause->getClassID() == ex_clause::FUNC_HBASE_COLUMNS_DISPLAY)
      return FALSE;
  }

  // Don't optimize if graph contains specific PCODE instructions.  This is
  // to help developers stage the introduction of new PCODE instructions - i.e.,
  // you can create a new PCODE instruction without supporting it in the PCODE
  // opts framework.  This solution is, however, draconian, in that the entire
  // expression is un-optimized.

  PCodeBinary * pCodeStart = pCode ;
  Int32 length = *(pCode++);
  pCode += (2*length);

  while (pCode[0] != PCIT::END)
  {
    switch (pCode[0]) {

      case PCIT::NULL_BYTES: 
        return FALSE;  

      case PCIT::CLAUSE_EVAL :
        pExprCacheable = FALSE ;
	break;

      default:
        break;
    }
    pCode += PCode::getInstructionLength(pCode);
  }
  totalPCodeLen = pCode - pCodeStart ;
  return TRUE;
}

// 
// We determine if the cfg has a loop if the branch target offset is negative.
// This isn't necessarily always correct, but for pcode coming directly from
// the generator, it is.
//
NABoolean PCodeCfg::cfgHasLoop()
{
  CollIndex i;
  UInt32 inst_cnt = 0;
  UInt32 brnch_cnt = 0;

  NAList<PCodeBinary*> targetOffPtrs(heap_);

  for (i=0; i < allInsts_->entries(); i++)
  {
    inst_cnt++;
    PCodeInst* inst = allInsts_->at(i);

    // Assume worst-case that indirect branches can loop back.
    if (inst->isIndirectBranch())
      return TRUE;

    if (inst->isBranch())
    {
      brnch_cnt++;
      targetOffPtrs.clear();
      inst->getBranchTargetOffsetPointers(&targetOffPtrs, this);

      // Only 1 entry possible in targetOffs since it's not an indirect branch.
      if (*(targetOffPtrs[0]) < 0)
        return TRUE;
    }
  }
  // If this expr involves too much PCODE, we will run out of virtual memory
  // in trying to optimize the pcode.  So, if it looks too big, return TRUE
  // to prevent doing the optimization.
  //
  if ( brnch_cnt > expr_->getPCodeMaxOptBrCnt() )
     return TRUE;
  if ( inst_cnt > expr_->getPCodeMaxOptInCnt() )
     return TRUE;

  return FALSE;
}


//
// Create list of PCodeInsts
//
NABoolean PCodeCfg::createInsts (PCodeBinary * pcode)
{
  Int32 length = *(pcode++);
  pcode += (2 * length);

  operandToIndexMap_ = new(heap_) NAHashDictionary<PCodeOperand, CollIndex>
                            (&operandHashFunc, 100, TRUE, heap_);

  indexToOperandMap_ = new(heap_) NAHashDictionary<CollIndex, PCodeOperand>
                            (&collIndexHashFunc, 100, TRUE, heap_);

  targets_ = new(heap_) NAHashDictionary<ULong, PCodeBinary>
                        (&targetHashFunc, 100, TRUE, heap_);

  while (TRUE)
  {
    PCodeInst* newInst = new(heap_) PCodeInst(heap_);

    newInst->code = pcode;

    // Fix pointers
    if (!allInsts_->isEmpty()) {
      PCodeInst* tailInst = allInsts_->at(allInsts_->entries() - 1);
      tailInst->next = newInst;
      newInst->prev = tailInst;
    }
  
    allInsts_->insert(newInst);

    loadOperandsOfInst(newInst);

    if (pcode[0] == PCIT::END)
      break;

    pcode += PCode::getInstructionLength(pcode);
  }

  return TRUE;
}


//
// Used for Aligned Format to get null bit index of column
//
static Int32 getBitIndex(PCodeBinary *code, const Int32 idx)
{
  Attributes* attr = (Attributes*)GetPCodeBinaryAsPtr(code, idx);
  if (attr->getTupleFormat() == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
    return attr->getNullBitIndex();
  return -1;
}

//
// Create new overlapping operands, should they exist, for this particular
// operand
//
NABoolean PCodeCfg::addOverlappingOperands(NABoolean detectOnly)
{
  CollIndex i, j, k;
  NAList<PCodeOperand*> reads(heap_);
  NAList<PCodeOperand*> writes(heap_);
  NAList<PCodeInst*> writesList(heap_);
  NABitVector tempBv(heap_);
  NABitVector tempWriteBv(heap_);
  NABoolean found = FALSE;
  NABoolean exclude = FALSE;

  // First get all read operands into the reads list.

  FOREACH_BLOCK_DFO(block, firstInst, lastInst, index) {
    FOREACH_INST_IN_BLOCK(block, inst) {
      CollIndex numOfReads = inst->getROps().entries();
      for (i=0; i < numOfReads; i++)  {
        PCodeOperand* op = inst->getROps()[i];

        // Exclude constant operands and operands already inserted
        if (op->isConst() || tempBv.testBit(op->getBvIndex()))
          continue;

        tempBv += op->getBvIndex();
        reads.insert(op);
      }

      CollIndex numOfWrites = inst->getWOps().entries();
      for (i=0; i < numOfWrites; i++)  {
        PCodeOperand* op = inst->getWOps()[i];

        // Exclude operands already inserted
        if (exclude && tempWriteBv.testBit(op->getBvIndex()))
          continue;

        tempWriteBv += op->getBvIndex();
        writes.insert(op);
        writesList.insert(inst);
      }
    } ENDFE_INST_IN_BLOCK
  } ENDFE_BLOCK_DFO

  // For each write operand identified, determine if an overlap is possible
  // with any read operand.

  for (i=0; i < writes.entries(); i++) {
    PCodeOperand* op = writes[i];
    CollIndex writeBv = op->getBvIndex();

    for (j=0; j < reads.entries(); j++) {
      PCodeOperand* readOp = reads[j];

      // If bvIndex are same, then dependency exists already.
      if (readOp->getBvIndex() == writeBv)
        continue;

      if (op->canOverlap(readOp)) {

        NABoolean allOpsOverlap = TRUE;

        // Check if instruction containing op has other operands which may
        // match the read operand.
        PCodeInst* inst = writesList[i];
        for (k=0; k < inst->getWOps().entries(); k++) {
          if (inst->getWOps()[k]->getBvIndex() == readOp->getBvIndex())
            allOpsOverlap = FALSE;
        }

        if (allOpsOverlap == FALSE)
          continue;

        // Overlap found
        found = TRUE;

        // If we're just detecting, return found
        if (detectOnly)
          return found;

        // Insert new write operand for inst
        PCodeOperand* operandPtr = readOp->copy(heap_);
        inst->getWOps().insert(operandPtr);
      }
    }
  }

  return found;
}

//
// Create a new operand for the instruction pointed to by "pcode" and add it
// to the specified operands list.  Additionally, update the hash tables
// "operandToIndexMap" and "indexToOperandMap" for faster index
//
void PCodeCfg::addOperand(PCodeBinary* pcode,
                          OPLIST& opListPtr,
                          Int32 stackIdxPos,
                          Int32 offsetPos,
                          Int32 nullBitIdx,
                          PCIT::AddressingMode am,
                          Int32 len,
                          Int32 voaOffset = -1)
{
  Int32 stackIdx = pcode[stackIdxPos];
  Int32 offset;

  // Negative offsetPos implies that an actual offset was passed in.
  if (offsetPos < 0)
    offset = -offsetPos;
  else
    offset = pcode[offsetPos];

  PCodeOperand operand(stackIdx, offset, nullBitIdx, voaOffset);
  PCodeOperand* operandPtr = &operand;

  NABoolean found = operandToIndexMap_->contains(operandPtr);

  // If we already created an index for this operand (i.e., found is TRUE),
  // retrieve the index and create a new operand.
  if (found) {
    CollIndex* val = operandToIndexMap_->getFirstValue(operandPtr);

    operandPtr = new(heap_) PCodeOperand(stackIdx,
                                         offset,
                                         nullBitIdx,
                                         stackIdxPos,
                                         offsetPos,
                                         *val,
                                         am,
                                         len,
                                         voaOffset);
  }
  else
  {
    // We're creating a new operand here. First allocate a new bit vector index
    // and then create an operand with it.  Then update the map tables.
 
    CollIndex* bvIndexPtr = (CollIndex*) new(heap_) CollIndex;

    *bvIndexPtr = maxBVIndex_++;

    operandPtr = new(heap_)
      PCodeOperand(stackIdx, offset, nullBitIdx,
                   stackIdxPos, offsetPos, *bvIndexPtr, am, len, voaOffset);

    operandToIndexMap_->insert(operandPtr, bvIndexPtr);
    indexToOperandMap_->insert(bvIndexPtr, operandPtr);

    // Update const vectors with this new operand, but only after we initialize
    // the constant vectors (determined by whether or not zeroOffset is set.
    if ((zeroOffset_ != -1) && (operandPtr->isConst()))
      updateConstVectors(operandPtr);
  }

  opListPtr.insert(operandPtr);
}

ex_clause* PCodeCfg::findClausePtr(PCodeInst* inst)
{
  PCodeBinary* pcode;

  // If inst->next is NULL, then that means we haven't created PCodeInst*
  // yet for the clause eval pointer, so peruse through the pcode.  Otherwise
  // go through the PCodeInst pointers.
  if (inst->next == NULL) {
    pcode = inst->code;
    while (pcode[0] != PCIT::CLAUSE_EVAL)
      pcode += PCode::getInstructionLength(pcode);
  }
  else {
    while (inst->code[0] != PCIT::CLAUSE_EVAL)
      inst = inst->next;
    pcode = inst->code;
  }

  return (ex_clause*)*(Long*)&(pcode[1]);
}

//
// Set up varchar fields for operands in inst
//
void PCodeInst::modifyOperandsForVarchar(PCodeCfg* cfg)
{
  Int32 comboLen;
  char* comboPtr = (char*)&comboLen;
  Int16* comboPtr16 = (Int16*)&comboLen;
  signed char vcLen, nullLen;

  PCodeBinary* pCode = code + 1;

  switch (getOpcode())
  {
    case PCIT::MOVE_MATTR5_MATTR5:
      comboLen = pCode[4];
      nullLen = comboPtr[0];
      vcLen = comboPtr[1];
      getWOps()[0]->setVarcharFields(2, -1, pCode[2], -1, pCode[3],
                                     nullLen, vcLen);

      comboLen = pCode[9];
      nullLen = comboPtr[0];
      vcLen = comboPtr[1];
      getROps()[0]->setVarcharFields(7, -1, pCode[7], -1, pCode[8],
                                     nullLen, vcLen);
      break;

    case PCIT::LIKE_MBIN32S_MATTR5_MATTR5_IBIN32S_IBIN32S:
    case PCIT::POS_MBIN32S_MATTR5_MATTR5:
    case PCIT::COMP_MBIN32S_MATTR5_MATTR5_IBIN32S:
    case PCIT::COMP_MBIN32S_MUNIV_MUNIV_IBIN32S:
      comboLen = pCode[6];
      nullLen = comboPtr[0];
      vcLen = comboPtr[1];
      getROps()[0]->setVarcharFields(4, -1, pCode[4], -1, pCode[5],
                                     nullLen, vcLen);

      comboLen = pCode[11];
      nullLen = comboPtr[0];
      vcLen = comboPtr[1];
      getROps()[1]->setVarcharFields(9, -1, pCode[9], -1, pCode[10],
                                     nullLen, vcLen);

      break;

    case PCIT::SWITCH_MBIN32S_MATTR5_MPTR32_IBIN32S_IBIN32S:
      comboLen = pCode[6];
      nullLen = comboPtr[0];
      vcLen = comboPtr[1];
      getROps()[0]->setVarcharFields(4, -1, pCode[4], -1, pCode[5],
                                     nullLen, vcLen);
      break;

    case PCIT::SUBSTR_MATTR5_MATTR5_MBIN32S_MBIN32S:
      comboLen = pCode[4];
      nullLen = comboPtr[0];
      vcLen = comboPtr[1];
      getWOps()[0]->setVarcharFields(2, -1, pCode[2], -1,
                                     pCode[3], nullLen, vcLen);


      comboLen = pCode[9];
      nullLen = comboPtr[0];
      vcLen = comboPtr[1];
      getROps()[0]->setVarcharFields(7, -1, pCode[7], -1,
                                     pCode[8], nullLen, vcLen);

      break;

    case PCIT::CONCAT_MATTR5_MATTR5_MATTR5:
      comboLen = pCode[4];
      nullLen = comboPtr[0];
      vcLen = comboPtr[1];
      getWOps()[0]->setVarcharFields(2, -1, pCode[2], -1,
                                     pCode[3], nullLen, vcLen);

      comboLen = pCode[9];
      nullLen = comboPtr[0];
      vcLen = comboPtr[1];
      getROps()[0]->setVarcharFields(7, -1, pCode[7], -1,
                                     pCode[8], nullLen, vcLen);

      comboLen = pCode[14];
      nullLen = comboPtr[0];
      vcLen = comboPtr[1];
      getROps()[1]->setVarcharFields(12, -1, pCode[12], -1,
                                     pCode[13], nullLen, vcLen);

      break;


    case PCIT::GENFUNC_MATTR5_MATTR5_MBIN32S_IBIN32S:
    case PCIT::GENFUNC_MATTR5_MATTR5_IBIN32S:
    {
      comboLen = pCode[4];
      nullLen = comboPtr[0];
      vcLen = comboPtr[1];
      getWOps()[0]->setVarcharFields(2, -1, pCode[2], -1,
                                     pCode[3], nullLen, vcLen);


      comboLen = pCode[9];
      nullLen = comboPtr[0];
      vcLen = comboPtr[1];
      getROps()[0]->setVarcharFields(7, -1, pCode[7], -1,
                                     pCode[8], nullLen, vcLen);

      break;
    }

    case PCIT::HASH_MBIN32U_MUNIV:
    case PCIT::HASH_MBIN32U_MATTR5:
    case PCIT::STRLEN_MBIN32U_MATTR5:
    case PCIT::STRLEN_MBIN32U_MUNIV:
    case PCIT::MOVE_MASCII_MATTR5_IBIN32S:
      comboLen = pCode[6];
      nullLen = (char)comboPtr[0];
      vcLen = (char)comboPtr[1];
      getROps()[0]->setVarcharFields(4, -1, pCode[4], -1, pCode[5],
                                     nullLen, vcLen);
      break;

    case PCIT::FILL_MEM_BYTES_VARIABLE:
    case PCIT::MOVE_MATTR5_MASCII_IBIN32S:
      comboLen = pCode[4];
      nullLen = (char)comboPtr[0];
      vcLen = (char)comboPtr[1];
      getWOps()[0]->setVarcharFields(2, -1, pCode[2], -1, pCode[3],
                                     nullLen, vcLen);
      break;


    case PCIT::UPDATE_ROWLEN3_MATTR5_IBIN32S:
      comboLen = pCode[4];
      nullLen = (char)comboPtr[0];
      vcLen = (char)comboPtr[1];
      getROps()[0]->setVarcharFields(2, -1, pCode[2], -1, pCode[3],
                                     nullLen, vcLen);
      break;

  }
}

//
// Load all operands for the specified PCodeInst
//
void PCodeCfg::loadOperandsOfInst (PCodeInst* newInst)
{
  PCodeBinary  nbi, voa;
  PCodeBinary *pcode = newInst->code;
  PCodeBinary  opc = pcode[0];

  PCIT::AddressingMode type = PCIT::AM_NONE;
  Int32 len;

  ex_clause* clause = newInst->clause_;
  Int32 sz = newInst->opdataLen_;

  pcode++;

  //
  // Format for adding new pcode instructions in framework
  //
  // For every new pcode instruction created, a call to addOperand must be
  // made for each of that instructions operands.  The format for the call
  // is as follows:
  //
  // addOperand(pcode, list, idx-poff, off-poff, nbx, type, size)
  //
  // where
  //
  // pcode    - pointer to pcode object starting after opcode
  // list     - pointer to read/write operand list
  // idx-poff - offset in pcode stream where stack index is located
  // off-poff - offset in pcode stream where column offset is located
  // nbx      - null bit index (or -1 if one doesn't exist)
  // type     - PCIT::TYPE of operand
  // size     - size of operand (or -1 is size is unknown)
  //

  switch(opc) {

    case PCIT::OPDATA_MPTR32_IBIN32S_IBIN32S:
      if ((pcode[3] % ex_clause::MAX_OPERANDS) == 0)
        addOperand(pcode, newInst->getWOps(), 0, 1, pcode[2], PCIT::MPTR32, sz);
      else
        addOperand(pcode, newInst->getROps(), 0, 1, pcode[2], PCIT::MPTR32, sz);
      break;

    case PCIT::OPDATA_MPTR32_IBIN32S:
      if ((pcode[2] % ex_clause::MAX_OPERANDS) == 0)
        addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MPTR32, sz);
      else
        addOperand(pcode, newInst->getROps(), 0, 1, -1, PCIT::MPTR32, sz);
      break;

    case PCIT::OPDATA_MBIN16U_IBIN32S:
      if ((pcode[2] % ex_clause::MAX_OPERANDS) == 0)
        addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN16U, sz);
      else
        addOperand(pcode, newInst->getROps(), 0, 1, -1, PCIT::MBIN16U, sz);
      break;

    case PCIT::OPDATA_MATTR5_IBIN32S:
      if ((pcode[5] % ex_clause::MAX_OPERANDS) == 0)
        addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MATTR5, sz);
      else
        addOperand(pcode, newInst->getROps(), 0, 1, -1, PCIT::MATTR5, sz);
      break;



    case PCIT::MOVE_MBIN32S:
      addOperand(pcode, newInst->getROps(), 0, 1, -1, PCIT::MBIN32S, 4);
      break;

    case PCIT::MOVE_MBIN32S_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      break;

    case PCIT::MOVE_MBIN8_MBIN8_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MPTR32, pcode[4]);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MPTR32, pcode[4]);
      break;

    case PCIT::MOVE_MBIN16S_MBIN8S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN16S, 2);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN8S, 1);
      break;

    case PCIT::MOVE_MBIN16U_MBIN8U:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN16U, 2);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN8U, 1);
      break;

     case PCIT::MOVE_MBIN16U_MBIN8:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN16U, 2);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN8, 1);
      break;

    case PCIT::MOVE_MBIN8S_MBIN16S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN8S, 1);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16S, 2);
      break;

    case PCIT::MOVE_MBIN8U_MBIN16U:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN8U, 1);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16U, 2);
      break;

    case PCIT::MOVE_MBIN8U_MBIN16S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN8U, 1);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16S, 2);
      break;

    case PCIT::MOVE_MBIN8S_MBIN16U:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN8S, 1);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16U, 2);
      break;

    case PCIT::MOVE_MBIN32S_MBIN8S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN8S, 1);
      break;

    case PCIT::MOVE_MBIN32U_MBIN8U:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32U, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN8U, 1);
      break;

    case PCIT::MOVE_MBIN64S_MBIN8S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN64S, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN8S, 1);
      break;

    case PCIT::MOVE_MBIN64U_MBIN8U:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN64U, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN8U, 1);
      break;

    case PCIT::MOVE_MBIN32U_MBIN16U:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32U, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16U, 2);
      break;

    case PCIT::MOVE_MBIN32S_MBIN16U:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16U, 2);
      break;

    case PCIT::MOVE_MBIN64S_MBIN16U:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN64S, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16U, 2);
      break;

    case PCIT::MOVE_MBIN32U_MBIN16S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32U, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16S, 2);
      break;

    case PCIT::MOVE_MBIN32S_MBIN16S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16S, 2);
      break;

    case PCIT::MOVE_MBIN64S_MBIN16S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN64S, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16S, 2);
      break;

    case PCIT::MOVE_MBIN64S_MBIN32U:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN64S, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN32U, 4);
      break;

    case PCIT::MOVE_MBIN64S_MBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN64S, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN32S, 4);
      break;

    case PCIT::MOVE_MBIN64S_MBIN64U:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN64S, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN64U, 8);
      break;

    case PCIT::MOVE_MBIN64U_MBIN64S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN64U, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN64S, 8);
      break;

    case PCIT::MOVE_MBIN64U_MBIN64U:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN64U, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN64U, 8);
      break;

     case PCIT::MOVE_MBIN64S_MDECS_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN64S, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MDECS, pcode[4]);
      break;

    case PCIT::MOVE_MBIN64S_MDECU_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN64S, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MDECU, pcode[4]);
      break;

    case PCIT::MOVE_MBIN8_MBIN8:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN8, 1);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN8, 1);
      break;

    case PCIT::MOVE_MBIN16U_MBIN16U:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN16U, 2);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16U, 2);
      break;

    case PCIT::MOVE_MBIN32U_MBIN32U:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32U, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN32U, 4);
      break;

    case PCIT::MOVE_MBIN64S_MBIN64S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN64S, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN64S, 8);
      break;

    case PCIT::MOVE_MBIN16U_IBIN16U:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN16U, 2);
      break;

    case PCIT::NULL_MBIN16U:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN16U, 2);
      break;

    case PCIT::NULL_MBIN16U_IBIN16U:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN16U, 2);
      break;

    case PCIT::NULL_TEST_MBIN32S_MBIN16U_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16U, 2);
      break;

    case PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN16S, 2);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16S, 2);
      targets_->insert(new(heap_) ULong((ULong)(pcode+pcode[4])), &pcode[4]);
      break;

    case PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_MBIN16S_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN16S, 2);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16S, 2);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN16S, 2);
      targets_->insert(new(heap_) ULong((ULong)(pcode+pcode[6])), &pcode[6]);
      break;




    case PCIT::NULL_VIOLATION_MBIN16U:
      addOperand(pcode, newInst->getROps(), 0, 1, -1, PCIT::MBIN16U, 2);
      break;

    case PCIT::NULL_VIOLATION_MBIN16U_MBIN16U:
      addOperand(pcode, newInst->getROps(), 0, 1, -1, PCIT::MBIN16U, 2);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16U, 2);
      break;

    case PCIT::UPDATE_ROWLEN3_MATTR5_IBIN32S:
    {
      voa = pcode[2];
      len = ((((char*)&pcode[4])[1]) > 0) ? -1 : pcode[3];
      addOperand(pcode, newInst->getROps(), 0, 1, -1, PCIT::MATTR5, len, voa);
      break;
    }


    case PCIT::ZERO_MBIN32S_MBIN32U:
    case PCIT::NOTZERO_MBIN32S_MBIN32U:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN32U, 4);
      break;


    case PCIT::EQ_MBIN32S_MBIN8S_MBIN8S:
    case PCIT::NE_MBIN32S_MBIN8S_MBIN8S:
    case PCIT::LT_MBIN32S_MBIN8S_MBIN8S:
    case PCIT::GT_MBIN32S_MBIN8S_MBIN8S:
    case PCIT::LE_MBIN32S_MBIN8S_MBIN8S:
    case PCIT::GE_MBIN32S_MBIN8S_MBIN8S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN8S, 1);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN8S, 1);
      break;

    case PCIT::EQ_MBIN32S_MBIN8U_MBIN8U:
    case PCIT::NE_MBIN32S_MBIN8U_MBIN8U:
    case PCIT::LT_MBIN32S_MBIN8U_MBIN8U:
    case PCIT::GT_MBIN32S_MBIN8U_MBIN8U:
    case PCIT::LE_MBIN32S_MBIN8U_MBIN8U:
    case PCIT::GE_MBIN32S_MBIN8U_MBIN8U:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN8U, 1);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN8U, 1);
      break;


    case PCIT::EQ_MBIN32S_MBIN16S_MBIN16S:
    case PCIT::NE_MBIN32S_MBIN16S_MBIN16S:
    case PCIT::LT_MBIN32S_MBIN16S_MBIN16S:
    case PCIT::GT_MBIN32S_MBIN16S_MBIN16S:
    case PCIT::LE_MBIN32S_MBIN16S_MBIN16S:
    case PCIT::GE_MBIN32S_MBIN16S_MBIN16S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16S, 2);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN16S, 2);
      break;

    case PCIT::EQ_MBIN32S_MBIN16S_MBIN32S:
    case PCIT::LT_MBIN32S_MBIN16S_MBIN32S:
    case PCIT::GT_MBIN32S_MBIN16S_MBIN32S:
    case PCIT::LE_MBIN32S_MBIN16S_MBIN32S:
    case PCIT::GE_MBIN32S_MBIN16S_MBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16S, 2);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN32S, 4);
      break;

    case PCIT::EQ_MBIN32S_MBIN32S_MBIN32S:
    case PCIT::LT_MBIN32S_MBIN32S_MBIN32S:
    case PCIT::GT_MBIN32S_MBIN32S_MBIN32S:
    case PCIT::LE_MBIN32S_MBIN32S_MBIN32S:
    case PCIT::GE_MBIN32S_MBIN32S_MBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN32S, 4);
      break;

    case PCIT::EQ_MBIN32S_MBIN16U_MBIN16U:
    case PCIT::LT_MBIN32S_MBIN16U_MBIN16U:
    case PCIT::GT_MBIN32S_MBIN16U_MBIN16U:
    case PCIT::LE_MBIN32S_MBIN16U_MBIN16U:
    case PCIT::GE_MBIN32S_MBIN16U_MBIN16U:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16U, 2);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN16U, 2);
      break;

    case PCIT::EQ_MBIN32S_MBIN16U_MBIN32U:
    case PCIT::LT_MBIN32S_MBIN16U_MBIN32U:
    case PCIT::GT_MBIN32S_MBIN16U_MBIN32U:
    case PCIT::LE_MBIN32S_MBIN16U_MBIN32U:
    case PCIT::GE_MBIN32S_MBIN16U_MBIN32U:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16U, 2);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN32U, 4);
      break;

    case PCIT::EQ_MBIN32S_MBIN32U_MBIN32U:
    case PCIT::LT_MBIN32S_MBIN32U_MBIN32U:
    case PCIT::GT_MBIN32S_MBIN32U_MBIN32U:
    case PCIT::LE_MBIN32S_MBIN32U_MBIN32U:
    case PCIT::GE_MBIN32S_MBIN32U_MBIN32U:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN32U, 4);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN32U, 4);
      break;

    case PCIT::EQ_MBIN32S_MBIN16S_MBIN32U:
    case PCIT::LT_MBIN32S_MBIN16S_MBIN32U:
    case PCIT::GT_MBIN32S_MBIN16S_MBIN32U:
    case PCIT::LE_MBIN32S_MBIN16S_MBIN32U:
    case PCIT::GE_MBIN32S_MBIN16S_MBIN32U:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16S, 2);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN32U, 4);
      break;

    case PCIT::EQ_MBIN32S_MBIN16U_MBIN16S:
    case PCIT::LT_MBIN32S_MBIN16U_MBIN16S:
    case PCIT::GT_MBIN32S_MBIN16U_MBIN16S:
    case PCIT::LE_MBIN32S_MBIN16U_MBIN16S:
    case PCIT::GE_MBIN32S_MBIN16U_MBIN16S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16U, 2);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN16S, 2);
      break;

    case PCIT::NE_MBIN32S_MBIN64S_MBIN64S:
    case PCIT::EQ_MBIN32S_MBIN64S_MBIN64S:
    case PCIT::LT_MBIN32S_MBIN64S_MBIN64S:
    case PCIT::GT_MBIN32S_MBIN64S_MBIN64S:
    case PCIT::LE_MBIN32S_MBIN64S_MBIN64S:
    case PCIT::GE_MBIN32S_MBIN64S_MBIN64S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN64S, 8);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN64S, 8);
      break;

    case PCIT::EQ_MBIN32S_MASCII_MASCII:
    case PCIT::LT_MBIN32S_MASCII_MASCII:
    case PCIT::GT_MBIN32S_MASCII_MASCII:
    case PCIT::LE_MBIN32S_MASCII_MASCII:
    case PCIT::GE_MBIN32S_MASCII_MASCII:
    case PCIT::NE_MBIN32S_MASCII_MASCII:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MASCII, pcode[6]);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MASCII, pcode[6]);
      break;

    case PCIT::COMP_MBIN32S_MUNI_MUNI_IBIN32S_IBIN32S_IBIN32S:
    case PCIT::COMP_MBIN32S_MASCII_MASCII_IBIN32S_IBIN32S_IBIN32S:
    {
      Int32 len1, len2;

      newInst->setCost(5);

      if (opc == PCIT::COMP_MBIN32S_MUNI_MUNI_IBIN32S_IBIN32S_IBIN32S) {
        type = PCIT::MUNI;

        // Length stored in number of chars, so double to indicate in bytes.
        len1 = pcode[6] << 1;
        len2 = pcode[7] << 1;
      }
      else {
        type = PCIT::MASCII;
        len1 = pcode[6];
        len2 = pcode[7];
      }

      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, type, len1);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, type, len2);

      break;
    }

    case PCIT::NE_MBIN32S_MFLT64_MFLT64:
    case PCIT::EQ_MBIN32S_MFLT64_MFLT64:
    case PCIT::LT_MBIN32S_MFLT64_MFLT64:
    case PCIT::GT_MBIN32S_MFLT64_MFLT64:
    case PCIT::LE_MBIN32S_MFLT64_MFLT64:
    case PCIT::GE_MBIN32S_MFLT64_MFLT64:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MFLT64, 8);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MFLT64, 8);
      break;

    case PCIT::NE_MBIN32S_MFLT32_MFLT32:
    case PCIT::EQ_MBIN32S_MFLT32_MFLT32:
    case PCIT::LT_MBIN32S_MFLT32_MFLT32:
    case PCIT::GT_MBIN32S_MFLT32_MFLT32:
    case PCIT::LE_MBIN32S_MFLT32_MFLT32:
    case PCIT::GE_MBIN32S_MFLT32_MFLT32:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MFLT32, 4);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MFLT32, 4);
      break;



    case PCIT::AND_MBIN32S_MBIN32S_MBIN32S:
    case PCIT::OR_MBIN32S_MBIN32S_MBIN32S :
    case PCIT::MOD_MBIN32S_MBIN32S_MBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN32S, 4);
      break;

    case PCIT::MOD_MBIN32U_MBIN32U_MBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32U, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN32U, 4);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN32S, 4);
      break;



    case PCIT::ADD_MBIN16S_MBIN16S_MBIN16S:
    case PCIT::SUB_MBIN16S_MBIN16S_MBIN16S:
    case PCIT::MUL_MBIN16S_MBIN16S_MBIN16S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN16S, 2);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16S, 2);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN16S, 2);
      break;


    case PCIT::ADD_MBIN32S_MBIN32S_MBIN32S:
    case PCIT::SUB_MBIN32S_MBIN32S_MBIN32S:
    case PCIT::MUL_MBIN32S_MBIN32S_MBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN32S, 4);
      break;

    case PCIT::ADD_MBIN64S_MBIN64S_MBIN64S:
    case PCIT::SUB_MBIN64S_MBIN64S_MBIN64S:
    case PCIT::MUL_MBIN64S_MBIN64S_MBIN64S:
    case PCIT::DIV_MBIN64S_MBIN64S_MBIN64S:
    case PCIT::DIV_MBIN64S_MBIN64S_MBIN64S_ROUND:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN64S, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN64S, 8);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN64S, 8);
      break;

    case PCIT::ADD_MBIN32S_MBIN16S_MBIN16S:
    case PCIT::SUB_MBIN32S_MBIN16S_MBIN16S:
    case PCIT::MUL_MBIN32S_MBIN16S_MBIN16S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16S, 2);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN16S, 2);
      break;

    case PCIT::ADD_MBIN32S_MBIN16S_MBIN32S:
    case PCIT::SUB_MBIN32S_MBIN16S_MBIN32S:
    case PCIT::MUL_MBIN32S_MBIN16S_MBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16S, 2);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN32S, 4);
      break;

    case PCIT::SUB_MBIN32S_MBIN32S_MBIN16S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN16S, 2);
      break;



    case PCIT::ADD_MBIN64S_MBIN32S_MBIN64S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN64S, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN64S, 8);
      break;

    case PCIT::MUL_MBIN64S_MBIN16S_MBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN64S, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16S, 2);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN32S, 4);
      break;

    case PCIT::MUL_MBIN64S_MBIN32S_MBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN64S, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN32S, 4);
      break;


    case PCIT::NEGATE_MASCII_MASCII:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MASCII, 2);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MASCII, 2);
      break;


    case PCIT::SUM_MBIN32S_MBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN32S, 4);
      break;

    case PCIT::SUM_MBIN64S_MBIN64S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN64S, 8);
      addOperand(pcode, newInst->getROps(), 0, 1, -1, PCIT::MBIN64S, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN64S, 8);
      break;

    case PCIT::SUM_MBIN64S_MBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN64S, 8);
      addOperand(pcode, newInst->getROps(), 0, 1, -1, PCIT::MBIN64S, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN32S, 4);
      break;


    case PCIT::MINMAX_MBIN8_MBIN8_MBIN32S_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MPTR32, pcode[6]);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MPTR32, pcode[6]);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN32S, 4);
      break;



    case PCIT::ENCODE_MASCII_MBIN8S_IBIN32S:
    case PCIT::ENCODE_MASCII_MBIN8U_IBIN32S:
    case PCIT::DECODE_MASCII_MBIN8S_IBIN32S:
    case PCIT::DECODE_MASCII_MBIN8U_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN8S, 1);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN8S, 1);
      break;

    case PCIT::ENCODE_MASCII_MBIN16S_IBIN32S:
    case PCIT::ENCODE_MASCII_MBIN16U_IBIN32S:
    case PCIT::DECODE_MASCII_MBIN16S_IBIN32S:
    case PCIT::DECODE_MASCII_MBIN16U_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN16S, 2);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16S, 2);
      break;

    case PCIT::ENCODE_MASCII_MBIN32S_IBIN32S:
    case PCIT::ENCODE_MASCII_MBIN32U_IBIN32S:
    case PCIT::DECODE_MASCII_MBIN32S_IBIN32S:
    case PCIT::DECODE_MASCII_MBIN32U_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN32S, 4);
      break;

    case PCIT::ENCODE_MASCII_MBIN64S_IBIN32S:
    case PCIT::DECODE_MASCII_MBIN64S_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN64S, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN64S, 8);
      break;


    case PCIT::ENCODE_NXX:
    case PCIT::ENCODE_DECS:
    case PCIT::DECODE_NXX:
    case PCIT::DECODE_DECS:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MPTR32, pcode[4]);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MPTR32, pcode[4]);
      break;

    case PCIT::BRANCH:
      targets_->insert(new(heap_) ULong((ULong)(pcode+pcode[0])), &pcode[0]);
      break;



    case PCIT::RANGE_LOW_S32S64 :
    case PCIT::RANGE_HIGH_S32S64:
      addOperand(pcode, newInst->getROps(), 0, 1, -1, PCIT::MBIN32S, 4);
      break;

    case PCIT::RANGE_LOW_U32S64:
    case PCIT::RANGE_HIGH_U32S64:
      addOperand(pcode, newInst->getROps(), 0, 1, -1, PCIT::MBIN32U, 4);
      break;

    case PCIT::RANGE_LOW_S64S64:
    case PCIT::RANGE_HIGH_S64S64:
      addOperand(pcode, newInst->getROps(), 0, 1, -1, PCIT::MBIN64S, 8);
      break;

    case PCIT::RANGE_LOW_S16S64:
    case PCIT::RANGE_HIGH_S16S64:
      addOperand(pcode, newInst->getROps(), 0, 1, -1, PCIT::MBIN16S, 2);
      break;

    case PCIT::RANGE_LOW_U16S64:
    case PCIT::RANGE_HIGH_U16S64:
      addOperand(pcode, newInst->getROps(), 0, 1, -1, PCIT::MBIN16U, 2);
      break;

    case PCIT::RANGE_LOW_S8S64:
    case PCIT::RANGE_HIGH_S8S64:
      addOperand(pcode, newInst->getROps(), 0, 1, -1, PCIT::MBIN8S, 2);
      break;

    case PCIT::RANGE_LOW_U8S64:
    case PCIT::RANGE_HIGH_U8S64:
      addOperand(pcode, newInst->getROps(), 0, 1, -1, PCIT::MBIN8U, 2);
      break;

    case PCIT::CLAUSE_EVAL:
    {
      NABoolean analyzeClause = TRUE;
      ex_clause* clause = (ex_clause*)*(Long*)&(pcode[0]);
      PCodeInst* opdata = newInst;
      short i;

      newInst->setCost(100);

      while (opdata->prev && opdata->prev->isOpdata())
        opdata = opdata->prev;

      // If opdatas for this clause have already been processed, then no need
      // to reprocess.
      if (!opdata->isOpdata() || (opdata->clause_ != NULL))
        break;

      // Some clauses have operands which are too difficult to analyze in terms
      // of what there true size is.  For example, some clauses use the routine
      // getStorageLength() to determine the length of the operand, and others
      // just use getLength().  Note, we should still be able to deal with
      // nulls, however.

      if ((clause->getClassID() == ex_clause::FUNC_ROWSETARRAY_SCAN_ID) ||
          (clause->getClassID() == ex_clause::FUNC_ROWSETARRAY_ROW_ID) ||
          (clause->getClassID() == ex_clause::FUNC_ROWSETARRAY_INTO_ID))
        analyzeClause = FALSE;

      while (opdata->isOpdata())
      {
        Int32 sz = -1;
        PCodeOperand* operand = opdata->getWOps().entries() ?
                                  opdata->getWOps()[0] : opdata->getROps()[0];

        for (i=0; (i < clause->getNumOperands()) && (sz == -1); i++) {
          Attributes* op = clause->getOperand(i);
          Int32 off = op->getOffset();
          Int32 si = -1;

          // Is it a constant?
          if ((op->getAtp() == 0) && (op->getAtpIndex() == 0))
            si = 1;
          // Is it a temporary?
          else if ((op->getAtp() == 0) && (op->getAtpIndex() == 1))
            si = 2;
          // Is it persistent?
          else if ((op->getAtp() == 1) && (op->getAtpIndex() == 1))
            si = 3;
          // Must be a var
          else {
            if (operand->isVar() && atpMap_ && atpIndexMap_ &&
                (atpMap_[operand->getStackIndex()-4] == op->getAtp()) &&
                (atpIndexMap_[operand->getStackIndex()-4] == op->getAtpIndex()))
              si = operand->getStackIndex();
          }
            
          if (operand->getStackIndex() == si) {
            // If the operands match, *and* we're allowing this comparison to
            // be done, continue.  If not, check if the operand is a null ind.
            // Analyzing this should always be safe.
            if (analyzeClause && (operand->getOffset() == off))
              sz = op->getLength();
            else if (op->getNullFlag() && 
                     operand->getOffset() == op->getNullIndOffset())
              sz = op->getNullIndicatorLength();
          }
        }

        operand->setLen(sz);

        opdata->clause_ = clause;
        opdata->opdataLen_ = sz;
        //opdata->opdataNum_ = (sz == -1) ? -1 : (i-1);  // i-1 cuz of loop i++

        opdata = opdata->next;
      }

      break;
    }

    case PCIT::CLAUSE_BRANCH:
      targets_->insert(new(heap_) ULong((ULong)(pcode+pcode[0])), &pcode[0]);
      break;



    case PCIT::PROFILE:
    case PCIT::NOP:
    case PCIT::END:
    case PCIT::RETURN:
    case PCIT::RETURN_IBIN32S:
    case PCIT::NULL_BYTES:
      break;



    case PCIT::FILL_MEM_BYTES:
    {
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MPTR32, pcode[2]);

      // Also add implicit write operands to start of string (assuming this is
      // a zero-fill for a string operand.  No harm done if it isn't.  This will
      // help reduce false-positives in identifying overlapping operands.
      Int32 off = pcode[1] + 2;
      Int32 len = pcode[2] - 2;
      if (pcode[2] > 2)
        addOperand(pcode, newInst->getWOps(), 0, -off, -1, PCIT::MPTR32, len);

      off = pcode[1] + 4;
      len = pcode[2] - 4;
      if (pcode[2] > 4)
        addOperand(pcode, newInst->getWOps(), 0, -off, -1, PCIT::MPTR32, len);

      break;
    }


    case PCIT::HASH2_DISTRIB:
    case PCIT::HASHCOMB_MBIN32U_MBIN32U_MBIN32U:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32U, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN32U, 4);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN32U, 4);

      if (opc == PCIT::HASHCOMB_MBIN32U_MBIN32U_MBIN32U)
        counters_.hashCombCnt_++;

      break;



    case PCIT::HASHCOMB_BULK_MBIN32U:
    {
      Int32 i, length = pcode[0];

      addOperand(pcode, newInst->getWOps(), 1, 2, -1, PCIT::MBIN32U, 4);

      for (i=3; i < length-1; i+=4) {
        Int32 size = pcode[i+2];

        // Order in which to combine hash value - not recorded here
        Int32 order = pcode[i+3];

        switch(size) {
          case 2:
            type = PCIT::MBIN16S;
            break;
          case 4:
            type = PCIT::MBIN32S;
            break;
          case 8:
            type = PCIT::MBIN64S;
            break;
          default:
            type = PCIT::MPTR32;
            break;
        }
        addOperand(pcode, newInst->getROps(), i, i+1, -1, type, size);
      }
        
      break;
    }

    case PCIT::NULL_BITMAP_BULK:
    {
      Int32 i, size, length = pcode[0];

      // Sub-opc determines whether all operands are from the same row or not
      if (pcode[1] == 0) {
        addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MPTR32, -1);
        for (i=0; i < 8*pcode[4]; i++) {
          UInt32 mask = ((UInt32)0x1 << (7 - (i & 7)));
          if ((((char*)&pcode[5])[i >> 3] & mask) != 0)
            addOperand(pcode, newInst->getROps(), 2, 3, i, PCIT::MPTR32, -1);
        }
      }
      else {
        // Operands from different rows
        for (i=2; i < length-2; i+=3) {
          if (pcode[i+2] != -1) {
            nbi = pcode[i+2];
            type = PCIT::MPTR32;
            size = -1;
          }
          else {
            nbi = -1;
            type = PCIT::MBIN16S;
            size = 2;
          }
          addOperand(pcode, newInst->getROps(), i, i+1, nbi, type, size);
        }
      }

      break;
    }

    case PCIT::NOT_NULL_BRANCH_BULK:
    {
      Int32 i, length = pcode[0];

      // Sub-opc determines whether all operands are from the same row or not
      if (pcode[1] == 0) {
        for (i=3; i < length-2; i+=1)
          addOperand(pcode, newInst->getROps(), 2, i, -1, PCIT::MBIN16S, 2);
      }
      else {
        for (i=2; i < length-2; i+=2)
          addOperand(pcode, newInst->getROps(), i, i+1, -1, PCIT::MBIN16S, 2);
      }
      break;
    }

    case PCIT::MOVE_BULK:
    {
      Int32 i, size = 0, length = pcode[0];

      for(i=3; i != (length - 1); i+=3) {
        Int32 opc = pcode[i];

        switch(opc) {
          case PCIT::MOVE_MBIN8_MBIN8_IBIN32S:
            size = pcode[i+3];
            type = PCIT::MPTR32;
            break;

          case PCIT::MOVE_MBIN8_MBIN8:
            size = 1;
            type = PCIT::MBIN8;
            break;

          case PCIT::MOVE_MBIN16U_MBIN16U:
            size = 2;
            type = PCIT::MBIN16U;
            break;

          case PCIT::MOVE_MBIN32U_MBIN32U:
            size = 4;
            type = PCIT::MBIN32U;
            break;

          case PCIT::MOVE_MBIN64S_MBIN64S:
            size = 8;
            type = PCIT::MBIN64S;
            break;

          case PCIT::MOVE_MBIN16U_IBIN16U:
            size = 2;
            type = PCIT::MBIN16U;
            break;

          case PCIT::MOVE_MBIN32S_IBIN32S:
            size = 4;
            type = PCIT::MBIN32S;
            break;

          default:
            assert(FALSE);
        }

        addOperand(pcode, newInst->getWOps(), 1, i+1, -1, type, size);

        if ((opc != PCIT::MOVE_MBIN16U_IBIN16U) &&
            (opc != PCIT::MOVE_MBIN32S_IBIN32S))
          addOperand(pcode, newInst->getROps(), 2, i+2, -1, type, size);

        // Fast moves have one extra entry in pcode for bulk move - size
        if (opc == PCIT::MOVE_MBIN8_MBIN8_IBIN32S)
          i++;
      }

      break;
    }

    case PCIT::BRANCH_AND:
    case PCIT::BRANCH_OR:
    case PCIT::BRANCH_AND_CNT:
    case PCIT::BRANCH_OR_CNT:
      {
        // 1st operand index located after 2 pointers for BRANCH PCode binaries
        Int32 opdIdx = PCODEBINARIES_PER_PTR + PCODEBINARIES_PER_PTR;
        addOperand(pcode, newInst->getWOps(), opdIdx, opdIdx+1, -1, PCIT::MBIN32S, 4);
        addOperand(pcode, newInst->getROps(), opdIdx+2, opdIdx+3, -1, PCIT::MBIN32S, 4);
        targets_->insert(new(heap_) ULong((ULong)(pcode+pcode[0])), &pcode[0]);
        counters_.logicalBranchCnt_++;
        break;
      }

    case PCIT::HASH_MBIN32U_MPTR32_IBIN32S_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32U, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MPTR32, pcode[5]);
      break;

    case PCIT::HASH_MBIN32U_MBIN64_IBIN32S_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32U, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN64S, 8);
      break;

    case PCIT::HASH_MBIN32U_MBIN32_IBIN32S_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32U, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN32S, 4);
      break;

    case PCIT::HASH_MBIN32U_MBIN16_IBIN32S_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32U, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16S, 2);
      break;

    // case PCIT::HASH_MBIN32U_MPTR32_IBIN32S_IBIN32S:
    //   addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32U, 4);
    //   addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MPTR32, -1);

    //   if (pcode[4] == sizeof(Int16))
    //     addOperand(pcode, newInst->getROps(), 2, 5, -1, PCIT::MBIN16S, 2);
    //   else
    //     addOperand(pcode, newInst->getROps(), 2, 5, -1, PCIT::MBIN32S, 4);

    //   break;



    case PCIT::GENFUNC_PCODE_1:
    {
      // NULLIFZERO
      if (pcode[0] == ITM_NULLIFZERO) {
        addOperand(pcode, newInst->getWOps(), 1, 2, -1, PCIT::MPTR32, pcode[8]);
        addOperand(pcode, newInst->getWOps(), 3, 4, -1, PCIT::MPTR32, pcode[5]);
        addOperand(pcode, newInst->getROps(), 6, 7, -1, PCIT::MPTR32, pcode[8]);
      }
      // RANDOMNUM
      else
      {
        addOperand(pcode, newInst->getWOps(), 1, 2, -1, PCIT::MBIN32U, 4);
      }
      break;
    }

    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_MBIN16S_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16S, 2);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIN16S, 2);
      targets_->insert(new(heap_) ULong((ULong)(pcode+pcode[6])), &pcode[6]);
      break;

    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16S, 2);
      targets_->insert(new(heap_) ULong((ULong)(pcode+pcode[4])), &pcode[4]);
      break;

    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16S, 2);
      targets_->insert(new(heap_) ULong((ULong)(pcode+pcode[5])), &pcode[5]);
      break;

    case PCIT::NOT_NULL_BRANCH_MBIN16S_IBIN32S:
      addOperand(pcode, newInst->getROps(), 0, 1, -1, PCIT::MBIN16S, 2);
      targets_->insert(new(heap_) ULong((ULong)(pcode+pcode[2])), &pcode[2]);
      break;



    case PCIT::GENFUNC_MBIN8_MBIN8_MBIN8_IBIN32S_IBIN32S:
    {
      switch(pcode[0]) {
        case ITM_CONCAT:
          addOperand(pcode, newInst->getWOps(), 1, 2, -1, PCIT::MPTR32, pcode[7]+pcode[8]);
          addOperand(pcode, newInst->getROps(), 3, 4, -1, PCIT::MPTR32, pcode[7]);
          addOperand(pcode, newInst->getROps(), 5, 6, -1, PCIT::MPTR32, pcode[8]);
          break;

        case ITM_BITOR:
        case ITM_BITAND:
        case ITM_BITXOR:
          if (pcode[7] == REC_BIN32_UNSIGNED) {
            addOperand(pcode, newInst->getWOps(), 1, 2, -1, PCIT::MBIN32U, 4);
            addOperand(pcode, newInst->getROps(), 3, 4, -1, PCIT::MBIN32U, 4);
            addOperand(pcode, newInst->getROps(), 5, 6, -1, PCIT::MBIN32U, 4);
          }
          else if (pcode[7] == REC_BIN32_SIGNED) {
            addOperand(pcode, newInst->getWOps(), 1, 2, -1, PCIT::MBIN32S, 4);
            addOperand(pcode, newInst->getROps(), 3, 4, -1, PCIT::MBIN32S, 4);
            addOperand(pcode, newInst->getROps(), 5, 6, -1, PCIT::MBIN32S, 4);
          }
          else {
            addOperand(pcode, newInst->getWOps(), 1, 2, -1, PCIT::MBIN64S, 8);
            addOperand(pcode, newInst->getROps(), 3, 4, -1, PCIT::MBIN64S, 8);
            addOperand(pcode, newInst->getROps(), 5, 6, -1, PCIT::MBIN64S, 8);
          }
          break;

        case ITM_BITNOT:
          if (pcode[7] == REC_BIN32_UNSIGNED) {
            addOperand(pcode, newInst->getWOps(), 1, 2, -1, PCIT::MBIN32U, 4);
            addOperand(pcode, newInst->getROps(), 3, 4, -1, PCIT::MBIN32U, 4);
          }
          else if (pcode[7] == REC_BIN32_SIGNED) {
            addOperand(pcode, newInst->getWOps(), 1, 2, -1, PCIT::MBIN32S, 4);
            addOperand(pcode, newInst->getROps(), 3, 4, -1, PCIT::MBIN32S, 4);
          }
          else if (pcode[7] == REC_BIN64_SIGNED) {
            addOperand(pcode, newInst->getWOps(), 1, 2, -1, PCIT::MBIN64S, 8);
            addOperand(pcode, newInst->getROps(), 3, 4, -1, PCIT::MBIN64S, 8);
          }
          else {
            addOperand(pcode, newInst->getWOps(), 1, 2, -1, PCIT::MPTR32, pcode[8]);
            addOperand(pcode, newInst->getROps(), 3, 4, -1, PCIT::MPTR32, pcode[8]);
          }
          break;
      }
      break;
    }

    case PCIT::MOVE_MFLT64_MBIN16S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MFLT64, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16S, 2);
      break;

    case PCIT::MOVE_MFLT64_MBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MFLT64, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN32S, 4);
      break;

    case PCIT::MOVE_MFLT64_MBIN64S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MFLT64, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN64S, 8);
      break;

    case PCIT::MOVE_MFLT64_MFLT32:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MFLT64, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MFLT32, 4);
      break;



    case PCIT::ADD_MFLT64_MFLT64_MFLT64:
    case PCIT::SUB_MFLT64_MFLT64_MFLT64:
    case PCIT::MUL_MFLT64_MFLT64_MFLT64:
    case PCIT::DIV_MFLT64_MFLT64_MFLT64:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MFLT64, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MFLT64, 8);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MFLT64, 8);
      break;

    case PCIT::SUM_MFLT64_MFLT64:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MFLT64, 8);
      addOperand(pcode, newInst->getROps(), 0, 1, -1, PCIT::MFLT64, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MFLT64, 8);
      break;



    case PCIT::RANGE_MFLT64:
      addOperand(pcode, newInst->getROps(), 0, 1, -1, PCIT::MFLT64, 8);
      break;



    case PCIT::NULL_BITMAP:
    case PCIT::NULL_BITMAP_SET:
    case PCIT::NULL_BITMAP_CLEAR:
      nbi = pcode[3];
      addOperand(pcode, newInst->getWOps(), 0, 1, nbi, PCIT::MPTR32, -1);
      break;

    case PCIT::NULL_BITMAP_TEST:
      nbi = pcode[3];
      addOperand(pcode, newInst->getROps(), 0, 1, nbi, PCIT::MPTR32, -1);
      break;

    case PCIT::NULL_VIOLATION_MPTR32_MPTR32_IPTR_IPTR:
      nbi = getBitIndex(pcode, 4);
      addOperand(pcode, newInst->getROps(), 0, 1, nbi, PCIT::MPTR32, -1);

      if (GetPCodeBinaryAsPtr(pcode, 4 + PCODEBINARIES_PER_PTR)) {
        nbi = getBitIndex(pcode, 4 + PCODEBINARIES_PER_PTR);
        addOperand(pcode, newInst->getROps(), 2, 3, nbi, PCIT::MPTR32, -1);
      }

      break;


    case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN32S_MBIN32S:
    case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN64S:
    case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN32S:
    case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MFLT64_MFLT64:
    case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIGS_MBIGS_IBIN32S:
    {
      Int32 size;

      // Bignum sums are more expensive.
      if (opc == PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIGS_MBIGS_IBIN32S)
        newInst->setCost(5);

      if (pcode[6]) // is nullable?
      {
        nbi = pcode[2];
        type = (nbi == -1) ? PCIT::MBIN16S : PCIT::MPTR32;
        size = (nbi == -1) ? 2 : -1;

        addOperand(pcode, newInst->getWOps(), 0, 1, nbi, type, size);
        addOperand(pcode, newInst->getROps(), 0, 1, nbi, type, size);

        nbi = pcode[5];
        type = (nbi == -1) ? PCIT::MBIN16S : PCIT::MPTR32;
        size = (nbi == -1) ? 2 : -1;

        addOperand(pcode, newInst->getROps(), 3, 4, nbi, type, size);
      }

      PCIT::AddressingMode type1 = PCIT::AM_NONE;
      PCIT::AddressingMode type2 = PCIT::AM_NONE;
      Int32 len1 = 0, len2 = 0;

      switch (opc) {
        case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN32S_MBIN32S:
          type1 = PCIT::MBIN32S; len1 = 4;
          type2 = type1; len2 = len1;
          break;
        case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN64S:
          type1 = PCIT::MBIN64S; len1 = 8;
          type2 = type1; len2 = len1;
          break;
        case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN32S:
          type1 = PCIT::MBIN64S; len1 = 8;
          type2 = PCIT::MBIN32S; len2 = 4;
          break;
        case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MFLT64_MFLT64:
          type1 = PCIT::MFLT64; len1 = 8;
          type2 = type1; len2 = len1;
          break;
        case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIGS_MBIGS_IBIN32S:
          type1 = PCIT::MBIGS; len1 = pcode[11];
          type2 = type1; len2 = len1;
          break;
      }

      addOperand(pcode, newInst->getWOps(), 7, 8, -1, type1, len1);
      addOperand(pcode, newInst->getROps(), 7, 8, -1, type1, len1);
      addOperand(pcode, newInst->getROps(), 9, 10, -1, type2, len2);

      break;
    }

    case PCIT::ADD_MBIGS_MBIGS_MBIGS_IBIN32S:
    case PCIT::SUB_MBIGS_MBIGS_MBIGS_IBIN32S:
      newInst->setCost(5);

      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIGS, pcode[6]);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIGS, pcode[6]);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIGS, pcode[6]);
      break;

    case PCIT::MUL_MBIGS_MBIGS_MBIGS_IBIN32S:
      newInst->setCost(5);

      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIGS, pcode[6]);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIGS, pcode[7]);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIGS, pcode[8]);
      break;

    case PCIT::MOVE_MBIGS_MBIGS_IBIN32S_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIGS, pcode[4]);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIGS, pcode[5]);
      break;

    case PCIT::MOVE_MBIGS_MBIN64S_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIGS, pcode[4]);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN64S, 8);
      break;

    case PCIT::MOVE_MBIGS_MBIN32S_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIGS, pcode[4]);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN32S, 4);
      break;

    case PCIT::MOVE_MBIGS_MBIN16S_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIGS, pcode[4]);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN16S, 2);
      break;

    case PCIT::MOVE_MBIN64S_MBIGS_IBIN32S:
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN64S, 8);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIGS, pcode[4]);
      break;

    case PCIT::COMP_MBIN32S_MBIGS_MBIGS_IBIN32S_IBIN32S:
      newInst->setCost(5);

      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIGS, pcode[6]);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MBIGS, pcode[6]);
      break;

    case PCIT::OFFSET_IPTR_IPTR_MBIN32S_MBIN64S_MBIN64S:
    {
      Int32 idx = 2 * PCODEBINARIES_PER_PTR;
      addOperand(pcode, newInst->getWOps(), idx + 2, idx + 3, -1, PCIT::MBIN64S, 8);
      addOperand(pcode, newInst->getROps(), idx, idx + 1, -1, PCIT::MBIN32S, 4);
      break;
    }

    case PCIT::OFFSET_IPTR_IPTR_IBIN32S_MBIN64S_MBIN64S:
    {
      Int32 idx = 1 + 2 * PCODEBINARIES_PER_PTR;
      addOperand(pcode, newInst->getWOps(), idx, idx + 1, -1, PCIT::MBIN64S, 8);
      break;
    }

    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
    {
      // Assume that only the first varchar is being accessed here.  Indirect
      // varchars can be tracked as well, but very little can be done at
      // compile time.

      PCodeAttrNull *attrs = (PCodeAttrNull *)&pcode[6];

      addOperand(pcode, newInst->getWOps(), 0, 1, attrs->op1NullBitIndex_,
                 ((attrs->fmt_.op1Fmt_ == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
                  ? PCIT::MPTR32
                  : PCIT::MBIN16S),
                 ((attrs->fmt_.op1Fmt_ == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
                  ? -1
                  : 2));

      addOperand(pcode, newInst->getROps(), 2, 3, attrs->op2NullBitIndex_,
                 ((attrs->fmt_.op2Fmt_ == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
                  ? PCIT::MPTR32
                  : PCIT::MBIN16S),
                 ((attrs->fmt_.op2Fmt_ == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
                  ? -1
                  : 2));

      addOperand(pcode, newInst->getROps(), 4, 5, attrs->op3NullBitIndex_,
                 ((attrs->fmt_.op3Fmt_ == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
                  ? PCIT::MPTR32
                  : PCIT::MBIN16S),
                 ((attrs->fmt_.op3Fmt_ == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
                  ? -1
                  : 2));

      targets_->insert(new(heap_) ULong((ULong)(pcode+pcode[10])), &pcode[10]);
      break;
    }

    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_IATTR3_IBIN32S:
    {
      // Assume that only the first varchar is being accessed here.  Indirect
      // varchars can be tracked as well, but very little can be done at
      // compile time.

      PCodeAttrNull *attrs = (PCodeAttrNull *)&pcode[4];

      addOperand(pcode, newInst->getWOps(), 0, 1, attrs->op1NullBitIndex_,
                 ((attrs->fmt_.op1Fmt_ == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
                  ? PCIT::MPTR32
                  : PCIT::MBIN16S),
                 ((attrs->fmt_.op1Fmt_ == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
                  ? -1
                  : 2));

      addOperand(pcode, newInst->getROps(), 2, 3, attrs->op2NullBitIndex_,
                 ((attrs->fmt_.op2Fmt_ == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
                  ? PCIT::MPTR32
                  : PCIT::MBIN16S),
                 ((attrs->fmt_.op2Fmt_ == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
                  ? -1
                  : 2));

      targets_->insert(new(heap_) ULong((ULong)(pcode+pcode[7])), &pcode[7]);
      break;
    }

    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
    {
      // Assume that only the first varchar is being accessed here.  Indirect
      // varchars can be tracked as well, but very little can be done at
      // compile time.

      PCodeAttrNull *attrs = (PCodeAttrNull *)&pcode[6];

      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);

      addOperand(pcode, newInst->getROps(), 2, 3, attrs->op2NullBitIndex_,
                 ((attrs->fmt_.op2Fmt_ == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
                  ? PCIT::MPTR32
                  : PCIT::MBIN16S),
                 ((attrs->fmt_.op2Fmt_ == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
                  ? -1
                  : 2));

      addOperand(pcode, newInst->getROps(), 4, 5, attrs->op3NullBitIndex_,
                 ((attrs->fmt_.op3Fmt_ == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
                  ? PCIT::MPTR32
                  : PCIT::MBIN16S),
                 ((attrs->fmt_.op3Fmt_ == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
                  ? -1
                  : 2));

      targets_->insert(new(heap_) ULong((ULong)(pcode+pcode[10])), &pcode[10]);
      break;
    }

    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S:
    {
      // Assume that only the first varchar is being accessed here.  Indirect
      // varchars can be tracked as well, but very little can be done at
      // compile time.

      PCodeAttrNull *attrs = (PCodeAttrNull *)&pcode[4];

      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);

      addOperand(pcode, newInst->getROps(), 2, 3, attrs->op2NullBitIndex_,
                 ((attrs->fmt_.op2Fmt_ == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
                  ? PCIT::MPTR32
                  : PCIT::MBIN16S),
                 ((attrs->fmt_.op2Fmt_ == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
                  ? -1
                  : 2));

      targets_->insert(new(heap_) ULong((ULong)(pcode+pcode[7])), &pcode[7]);
      break;
    }

    case PCIT::MOVE_MATTR5_MASCII_IBIN32S:
    {
      voa = pcode[2];
      len = ((((char*)&pcode[4])[1]) > 0) ? -1 : pcode[3];
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MATTR5, len, voa);

      addOperand(pcode, newInst->getROps(), 5, 6, -1, PCIT::MASCII, pcode[7]);

      break;
    }

    case PCIT::MOVE_MASCII_MATTR5_IBIN32S:
    {
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MASCII, pcode[7]);

      voa = pcode[4];
      len = ((((char*)&pcode[6])[1]) > 0) ? -1 : pcode[5];
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MATTR5, len, voa);

      break;
    }

    case PCIT::MOVE_MATTR5_MATTR5:
    {
      Int32 len;

      voa = pcode[2];
      len = ((((char*)&pcode[4])[1]) > 0) ? -1 : pcode[3];
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MATTR5, len, voa);

      voa = pcode[7];
      len = ((((char*)&pcode[9])[1]) > 0) ? -1 : pcode[8];
      addOperand(pcode, newInst->getROps(), 5, 6, -1, PCIT::MATTR5, len, voa);

      break;
    }

    case PCIT::CONVVCPTR_MBIN32S_MATTR5_IBIN32S:
    {
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, pcode[7]);

      voa = pcode[4];
      len = ((((char*)&pcode[6])[1]) > 0) ? -1 : pcode[5];
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MATTR5, len, voa);

      break;
    }

    case PCIT::CONVVCPTR_MBIN64S_MATTR5_IBIN32S:
    {
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN64S, pcode[7]);

      voa = pcode[4];
      len = ((((char*)&pcode[6])[1]) > 0) ? -1 : pcode[5];
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MATTR5, len, voa);

      break;
    }

   case PCIT::CONVVCPTR_MFLT32_MATTR5_IBIN32S:
    {
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MFLT32, pcode[7]);

      voa = pcode[4];
      len = ((((char*)&pcode[6])[1]) > 0) ? -1 : pcode[5];
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MATTR5, len, voa);

      break;
    }

   case PCIT::CONVVCPTR_MATTR5_MATTR5:
    {
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MATTR5, pcode[7]);

      voa = pcode[4];
      len = ((((char*)&pcode[6])[1]) > 0) ? -1 : pcode[5];
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MATTR5, len, voa);

      break;
    }

    case PCIT::MOVE_MUNI_MUNI_IBIN32S_IBIN32S:
    case PCIT::MOVE_MASCII_MASCII_IBIN32S_IBIN32S:
      type = (opc == PCIT::MOVE_MUNI_MUNI_IBIN32S_IBIN32S)
               ? PCIT::MUNI : PCIT::MASCII;

      addOperand(pcode, newInst->getWOps(), 0, 1, -1, type, pcode[4]);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, type, pcode[5]);
      break;

    case PCIT::HASH_MBIN32U_MUNIV:
    case PCIT::HASH_MBIN32U_MATTR5:
    case PCIT::STRLEN_MBIN32U_MATTR5:
    case PCIT::STRLEN_MBIN32U_MUNIV:
    {
      Int32 len;
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);

      PCIT::AddressingMode type =
        (((opc == PCIT::HASH_MBIN32U_MUNIV) ||
          (opc == PCIT::STRLEN_MBIN32U_MUNIV)) ? PCIT::MUNIV : PCIT::MATTR5);

      voa = pcode[4];
      len = ((((char*)&pcode[6])[1]) > 0) ? -1 : pcode[5];
      addOperand(pcode, newInst->getROps(), 2, 3, -1, type, len, voa);
      break;
    }

    case PCIT::NULL_TEST_MBIN32S_MATTR5_IBIN32S_IBIN32S:
    {
      Int32 len;
      PCIT::AddressingMode type;

      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);

      nbi = pcode[6];
      len = (nbi != -1) ? -1 : 2;
      type = (nbi != -1) ? PCIT::MPTR32 : PCIT::MBIN16S;
      addOperand(pcode, newInst->getROps(), 2, 3, nbi, type, len);
      break;
    }

    case PCIT::COMP_MBIN32S_MATTR4_MATTR4_IBIN32S_IBIN32S_IBIN32S:
    {
      Int32 len, voa;

      newInst->setCost(10);

      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);

      voa = pcode[4];
      len = ((((char*)&pcode[5])[1]) > 0) ? -1 : pcode[10];
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MPTR32, len, voa);

      voa = pcode[8];
      len = ((((char*)&pcode[9])[1]) > 0) ? -1 : pcode[11];
      addOperand(pcode, newInst->getROps(), 6, 7, -1, PCIT::MPTR32, len, voa);

      break;
    }

    case PCIT::COMP_MBIN32S_MUNIV_MUNIV_IBIN32S:
    case PCIT::COMP_MBIN32S_MATTR5_MATTR5_IBIN32S:
    case PCIT::LIKE_MBIN32S_MATTR5_MATTR5_IBIN32S_IBIN32S:
    case PCIT::POS_MBIN32S_MATTR5_MATTR5:
    {
      Int32 len, voa;

      newInst->setCost(10);

      PCIT::AddressingMode type =
        ((opc == PCIT::COMP_MBIN32S_MUNIV_MUNIV_IBIN32S)
          ? PCIT::MUNIV : PCIT::MATTR5);

      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);

      voa = pcode[4];
      len = ((((char*)&pcode[6])[1]) > 0) ? -1 : pcode[5];
      addOperand(pcode, newInst->getROps(), 2, 3, -1, type, len, voa);

      voa = pcode[9];
      len = ((((char*)&pcode[11])[1]) > 0) ? -1 : pcode[10];
      addOperand(pcode, newInst->getROps(), 7, 8, -1, type, len, voa);

      break;
    }

    case PCIT::GENFUNC_MATTR5_MATTR5_MBIN32S_IBIN32S:
    {
      Int32 len, voa;

      newInst->setCost(15);

      voa = pcode[2];
      len = ((((char*)&pcode[4])[1]) > 0) ? -1 : pcode[3];
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MATTR5, len, voa);

      voa = pcode[7];
      len = ((((char*)&pcode[9])[1]) > 0) ? -1 : pcode[8];
      addOperand(pcode, newInst->getROps(), 5, 6, -1, PCIT::MATTR5, len, voa);

      addOperand(pcode, newInst->getROps(), 10, 11, -1, PCIT::MBIN32S, 4);

      break;
    }

    case PCIT::GENFUNC_MATTR5_MATTR5_IBIN32S:
    {
      Int32 len, voa;

      newInst->setCost(15);

      voa = pcode[2];
      len = ((((char*)&pcode[4])[1]) > 0) ? -1 : pcode[3];
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MATTR5, len, voa);

      voa = pcode[7];
      len = ((((char*)&pcode[9])[1]) > 0) ? -1 : pcode[8];
      addOperand(pcode, newInst->getROps(), 5, 6, -1, PCIT::MATTR5, len, voa);

      break;
    }

    case PCIT::CONCAT_MATTR5_MATTR5_MATTR5:
    {
      Int32 len, voa;

      newInst->setCost(15);

      voa = pcode[2];
      len = ((((char*)&pcode[4])[1]) > 0) ? -1 : pcode[3];
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MATTR5, len, voa);

      voa = pcode[7];
      len = ((((char*)&pcode[9])[1]) > 0) ? -1 : pcode[8];
      addOperand(pcode, newInst->getROps(), 5, 6, -1, PCIT::MATTR5, len, voa);

      voa = pcode[12];
      len = ((((char*)&pcode[14])[1]) > 0) ? -1 : pcode[13];
      addOperand(pcode, newInst->getROps(), 10, 11, -1, PCIT::MATTR5, len, voa);

      break;
    }

    case PCIT::SWITCH_MBIN32S_MBIN64S_MPTR32_IBIN32S_IBIN32S:
    {
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN64S, 8);
      addOperand(pcode, newInst->getROps(), 4, 5, -1, PCIT::MPTR32, -1);

      break;
    }

    case PCIT::SWITCH_MBIN32S_MATTR5_MPTR32_IBIN32S_IBIN32S:
    {
      Int32 len, voa;

      newInst->setCost(10);

      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);

      voa = pcode[4];
      len = ((((char*)&pcode[6])[1]) > 0) ? -1 : pcode[5];
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MATTR5, len, voa);

      addOperand(pcode, newInst->getROps(), 7, 8, -1, PCIT::MPTR32, -1);

      break;
    }

    case PCIT::BRANCH_INDIRECT_MBIN32S:
    {
      // Previous instruction must be a SWITCH instruction.  That instruction
      // contains the jump table from which the incoming operand came from.  It
      // needs to be accessed here, however, so that the targets_ array is
      // properly defined (since this is, after all, a multi-way branch).

      assert (newInst->prev->isSwitch());

      NAList<Long*> targetOffPtrs(heap_);
      newInst->getBranchTargetPointers(&targetOffPtrs, this);

      for (CollIndex i=0; i < targetOffPtrs.entries(); i++) {
        Long* loc = targetOffPtrs[i];
        targets_->insert(new(heap_) ULong((ULong)(pcode+(*loc))), (PCodeBinary*)loc);
      }

      break;
    }

    case PCIT::FILL_MEM_BYTES_VARIABLE:
    {
      voa = pcode[2];
      len = pcode[5];
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MATTR5, len, voa);

      break;
    }

    case PCIT::HDR_MPTR32_IBIN32S_IBIN32S_IBIN32S_IBIN32S_IBIN32S:
    {
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MPTR32, pcode[2]);

      if (pcode[5] > 0)
        addOperand(pcode, newInst->getWOps(), 0, 6, -1, PCIT::MBIN16S, 2);

      break;
    }

    case PCIT::NNB_SPECIAL_NULLS_MBIN32S_MATTR3_MATTR3_IBIN32S_IBIN32S:
    {
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);

      nbi = pcode[4];
      len = (nbi != -1) ? -1 : 2;
      type = (nbi != -1) ? PCIT::MPTR32 : PCIT::MBIN16S;
      addOperand(pcode, newInst->getROps(), 2, 3, nbi, type, len);

      nbi = pcode[7];
      len = (nbi != -1) ? -1 : 2;
      type = (nbi != -1) ? PCIT::MPTR32 : PCIT::MBIN16S;
      addOperand(pcode, newInst->getROps(), 5, 6, nbi, type, len);

      targets_->insert(new(heap_) ULong((ULong)(pcode+pcode[9])), &pcode[9]);

      break;
    }

    case PCIT::NULLIFZERO_MPTR32_MATTR3_MPTR32_IBIN32S:
    {
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MPTR32, pcode[7]);

      nbi = pcode[4];
      len = (nbi != -1) ? -1 : 2;
      type = (nbi != -1) ? PCIT::MPTR32 : PCIT::MBIN16S;
      addOperand(pcode, newInst->getWOps(), 2, 3, nbi, type, len);

      addOperand(pcode, newInst->getROps(), 5, 6, -1, PCIT::MPTR32, pcode[7]);
      break;
    }

    case PCIT::NNB_MATTR3_IBIN32S:
    {
      nbi = pcode[2];
      len = (nbi != -1) ? -1 : 2;
      type = (nbi != -1) ? PCIT::MPTR32 : PCIT::MBIN16S;
      addOperand(pcode,newInst->getROps(), 0, 1, nbi, type, len);

      targets_->insert(new(heap_) ULong((ULong)(pcode+pcode[3])), &pcode[3]);

      break;
    }

    case PCIT::NNB_MBIN32S_MATTR3_IBIN32S_IBIN32S:
    {
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN32S, 4);

      nbi = pcode[4];
      len = (nbi != -1) ? -1 : 2;
      type = (nbi != -1) ? PCIT::MPTR32 : PCIT::MBIN16S;
      addOperand(pcode,newInst->getROps(), 2, 3, nbi, type, len);

      targets_->insert(new(heap_) ULong((ULong)(pcode+pcode[6])), &pcode[6]);

      break;
    }

    case PCIT::REPLACE_NULL_MATTR3_MBIN32S:
    case PCIT::REPLACE_NULL_MATTR3_MBIN32U:
    case PCIT::REPLACE_NULL_MATTR3_MBIN16S:
    case PCIT::REPLACE_NULL_MATTR3_MBIN16U:
    {
      PCIT::AddressingMode opType;

      opType = (pcode[9] == 2) ? PCIT::MBIN16U : PCIT::MBIN32U;

      addOperand(pcode,newInst->getWOps(), 0, 1, -1, opType, pcode[9]);

      nbi = pcode[4];
      len = (nbi != -1) ? -1 : 2;
      type = (nbi != -1) ? PCIT::MPTR32 : PCIT::MBIN16S;
      addOperand(pcode,newInst->getROps(), 2, 3, nbi, type, len);

      addOperand(pcode,newInst->getROps(), 5, 6, -1, opType, pcode[9]);
      addOperand(pcode,newInst->getROps(), 7, 8, -1, opType, pcode[9]);

      break;
    }

    case PCIT::SUBSTR_MATTR5_MATTR5_MBIN32S_MBIN32S:
    {
      Int32 len, voa;

      newInst->setCost(15);

      voa = pcode[2];
      len = ((((char*)&pcode[4])[1]) > 0) ? -1 : pcode[3];
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MATTR5, len, voa);

      voa = pcode[7];
      len = ((((char*)&pcode[9])[1]) > 0) ? -1 : pcode[8];
      addOperand(pcode, newInst->getROps(), 5, 6, -1, PCIT::MATTR5, len, voa);

      addOperand(pcode, newInst->getROps(), 10, 11, -1, PCIT::MBIN32S, 4);

      // If a length was provided for the substring, add the operand
      if (((char*)&pcode[9])[2])
        addOperand(pcode, newInst->getROps(), 12, 13, -1, PCIT::MBIN32S, 4);

      break;
    }
    case PCIT::COPYVARROW_MBIN8_MBIN8_IBIN32S_IBIN32S_IBIN32S_IBIN32S:
    {
      addOperand(pcode, newInst->getWOps(), 0, 1, -1, PCIT::MBIN8, -1);
      addOperand(pcode, newInst->getROps(), 2, 3, -1, PCIT::MBIN8, -1);
    }
    break;

    default:
      assert(FALSE);
      break;
  }

  newInst->modifyOperandsForVarchar(this);
}

//
// computeTimeUsed() takes a beginning timeval argument and computes
// the cpu time used (in microseconds) from the beginning time
// until the present time.
//
Int64
computeTimeUsed( timeval begTime )
{
   struct rusage endTime;
   (void) getrusage( RUSAGE_THREAD, &endTime );

   Int64 timeUsed  = ( endTime.ru_utime.tv_sec  - begTime.tv_sec ) * 1000000 +
                     ( endTime.ru_utime.tv_usec - begTime.tv_usec ) ;
   return ( timeUsed ) ;
}

//
// addToCreateOrderList  -- add a new PCE Cache Entry to the Creation Order List
//
void
OptPCodeCache::addToCreateOrderList( PCECacheEntry * PCEtoAdd )
{
  // Always put the new PCE cache entry at the tail of the Creating Order list

  PCEtoAdd->setNextInCrOrder( NULL );
  PCEtoAdd->setPrevInCrOrder( createOrderTail_ );

  if ( createOrderTail_ )
       createOrderTail_->setNextInCrOrder( PCEtoAdd );

  createOrderTail_ = PCEtoAdd ;

  if ( createOrderHead_ == NULL ) 
       createOrderHead_ = PCEtoAdd ;
}

//
// addToMRUList -- Add a PCE Cache Entry to the MRU list
//
void
OptPCodeCache::addToMRUList( PCECacheEntry * PCEtoAdd )
{
  // Always put the new PCE cache entry at the head of the MRU list
  PCEtoAdd->setNextInMRUOrder( MRUHead_ );
  PCEtoAdd->setPrevInMRUOrder( NULL     );

  if ( MRUHead_ )
       MRUHead_->setPrevInMRUOrder( PCEtoAdd );

  MRUHead_ = PCEtoAdd ;

  if ( MRUTail_ == NULL ) 
       MRUTail_ = PCEtoAdd ;
}

//
// removeFromCreateOrderList -- remove a PCE Cache Entry from the Creation Order list
//
void
OptPCodeCache::removeFromCreateOrderList( PCECacheEntry * PCEtoDel )
{
   PCECacheEntry * nextInCrOrder = PCEtoDel->getNextInCrOrder();
   PCECacheEntry * prevInCrOrder = PCEtoDel->getPrevInCrOrder();

   PCEtoDel->setNextInCrOrder( NULL );  //Just to keep things clean
   PCEtoDel->setPrevInCrOrder( NULL );  //Just to keep things clean

   if ( nextInCrOrder != NULL )
      nextInCrOrder->setPrevInCrOrder( prevInCrOrder );

   if ( prevInCrOrder != NULL )
      prevInCrOrder->setNextInCrOrder( nextInCrOrder );

   if ( PCEtoDel == createOrderHead_ )
      createOrderHead_ = nextInCrOrder ;

   if ( PCEtoDel == createOrderTail_ )
      createOrderTail_ = prevInCrOrder ;
}

//
// removeFromMRUList -- Remove a PCE Cache Entry from the MRU list
//
void
OptPCodeCache::removeFromMRUList( PCECacheEntry * PCEtoRem )
{
   PCECacheEntry * nextInMRUOrder = PCEtoRem->getNextInMRUOrder();
   PCECacheEntry * prevInMRUOrder = PCEtoRem->getPrevInMRUOrder();

   PCEtoRem->setNextInMRUOrder( NULL );  //Just to keep things clean
   PCEtoRem->setPrevInMRUOrder( NULL );  //Just to keep things clean

   if ( nextInMRUOrder != NULL )
      nextInMRUOrder->setPrevInMRUOrder( prevInMRUOrder );
   if ( prevInMRUOrder != NULL )
      prevInMRUOrder->setNextInMRUOrder( nextInMRUOrder );

   if ( PCEtoRem == MRUHead_ )
      MRUHead_ = nextInMRUOrder ;
   if ( PCEtoRem == MRUTail_ )
      MRUTail_ = prevInMRUOrder ;
}

//
// This PCECacheEntry::matches(...) method checks the cached entry
// (pointed to by 'this') to see if it is an acceptable match for the 
// the unOptimized PCode and Associated Constants that are passed in.
//
// NOTE: This routine assumes the caller has already ensured that
//       the length of the PCode matches AND that the length of the
//       unOptimized ConstantsArea maches.
//
NABoolean
PCECacheEntry::matches( PCodeBinary * unOptPCodePtr, char * unOptConstantsArea,
                        UInt32 unOptPCodeLen, UInt32 unOptConstsLen, UInt32 NEflag )
{
  if ( ( unOptConstsLen != getOptConstsLen() ) ||
       ( NEflag && ( getOptConstsLen() < getNEConstsLen() ) ) )
  {
     // There were constants added during PCode optimization
     // OR we are using Native Exprs and this entry has an N.E.
     // so in order to declare a match, the orginal constants
     // must match exactly.

     char * entryConstsArea = getConstsArea() ;
     if ( memcmp( unOptConstantsArea, entryConstsArea, unOptConstsLen) !=0 )
        return FALSE ;
  }

  // We know the unOptimized PC length matches, now compare PCode
  PCodeBinary * entryUnOptPC = getUnOptPCptr() ;
  if ( memcmp( unOptPCodePtr, entryUnOptPC,
               sizeof(PCodeBinary) * unOptPCodeLen ) !=0 )
     return FALSE ;

  return ( TRUE ) ;
}


//
// addPCodeExpr( ... ) -- Add PCode Expr to the cache
//
// Arguments: newPCE     - ptr to the PCECacheEntry object which the
//                         caller should have just constructed.
#if OPT_PCC_DEBUG==1
//            NEgenTime  - a UInt64 giving time spent in Native Expr generation
//            begAdd     - a timeval giving the time when we started adding the new entry
//            sqlStmt    - ptr to original SQL query string being compiled
#endif // OPT_PCC_DEBUG==1
//
void
OptPCodeCache::addPCodeExpr( PCECacheEntry * newPCE

#if OPT_PCC_DEBUG==1
                           , UInt64          NEgenTime
                           , timeval         begAdd
                           , char     *      sqlStmt
#endif // OPT_PCC_DEBUG==1
                           )
{
  addToMRUList( newPCE );

  addToCreateOrderList( newPCE );

  numEntries_++ ;
  currSize_ += newPCE->getTotalMemorySize() ;

  if ( maxOptPCodeSize_ < newPCE->getOptPClen() )
       maxOptPCodeSize_ = newPCE->getOptPClen() ;

#if OPT_PCC_DEBUG==1
  if ( PCECLoggingEnabled_ )
  {
     Int64 totalAddTime = computeTimeUsed( begAdd ) ;

     //
     // We now add the cpu time spent creating the new PCEC Entry
     // to the PCode Optimization time for this PCEC entry.
     // This is a little misleading since this time is really not
     // optimization time, but it should be a very tiny amount of
     // time and it isn't worth creating yet another stats counter.
     //
     newPCE->addToOptTime( totalAddTime );

     totalOptTime_   += newPCE->getOptTime() ;
     totalNEgenTime_ += NEgenTime ;

     logPCCEvent( 2, newPCE, sqlStmt );
  }
#endif // OPT_PCC_DEBUG==1

  throwOutExcessCacheEntries();

  return ;
}

//
// findPCodeExprInCache( ... ) - search PCode Expr cache 
//
// Arguments: unOptPCodePtr      - ptr to the Unoptimized PCode byte stream
//            unOptConstantsArea - ptr to the ConstantsArea (before any optimization)
//            NEflag             - Native Expressions in use flag
//            unOptPCodeLen      - Length (in PCodeBinary units) of Unopt. PCode
//            unOptConstsLen     - Length (in bytes) of Unoptimized ConstantsArea
//
// NOTE: We search the cache in the order that the cache entries were
// created ... starting with the next entry after the last matched entry.
// The reason for this is that there is a high likelihood that that next
// entry is the one we want!  This is particularly true for the instance of
// the PCode Expression Cache used by the Compiler instance used to compile
// metadata queries.  There are 7 metadata queries used to interrogate each
// user table referenced in an SQL statement.  For any particular user table,
// those 7 queries vary from the previously interrogated user table only by
// the constants that identify this particular user table.  Other than the
// constants, the Expressions are the same AND they are encountered in the
// same order in the compilation process as they were encountered when we
// interrogated the previous table.   Even for user queries, there is a 
// fairly good chance that the Expressions in the query currently being 
// compiled are similar Expressions in previous user queries and in the
// same order.   Of course, this is not a perfect scheme.  In the future
// we will want to improve the search time by using hashing or something.
//
PCECacheEntry *
OptPCodeCache::findPCodeExprInCache( PCodeBinary * unOptPCodePtr
                                   , char    * unOptConstantsArea
                                   , UInt32    NEflag        // Native Expr in use?
                                   , UInt32    unOptPCodeLen
                                   , UInt32    unOptConstsLen
#if OPT_PCC_DEBUG==1
                                   , char    * sqlStmt
#endif // OPT_PCC_DEBUG==1
                                   )
{
  CMPASSERT( unOptPCodePtr ) ;
  CMPASSERT( unOptPCodeLen > 0 ) ;

  PCECacheEntry * nextEntry   = lastMatchedEntry_ ;
  if ( nextEntry != NULL )
     nextEntry = nextEntry->getNextInCrOrder() ;

  if ( nextEntry == NULL )
       nextEntry = createOrderHead_ ;

  PCECacheEntry * currEntry  = nextEntry ;
  NABoolean     match_found  = FALSE ;

  numLookups_++ ;

  //
  // Run through the Creation Order list ... starting at the next entry
  // after the last matched entry and going in circular fashion until
  // we get back to where we started.
  //
  Int32 nSrchd = 1;
  for ( ; nSrchd <= numEntries_ ; nSrchd++, currEntry = nextEntry )
  {
     nextEntry = currEntry->getNextInCrOrder() ;
     if ( nextEntry == NULL )
          nextEntry = createOrderHead_ ;

     // Make quick comparisons here (to avoid calling matches() if possible)
     // 
     if ( unOptPCodeLen  != currEntry->getUnOptPClen() )
        continue ;          // cannot be a match

     if ( unOptConstsLen != currEntry->getUnOptConstsLen() )
        continue ;          // cannot be a match

     match_found = currEntry->matches( unOptPCodePtr, unOptConstantsArea,
                                       unOptPCodeLen, unOptConstsLen, NEflag) ;
     if ( match_found )
        break;
  }

  totSrchd_ += nSrchd;
  if ( match_found )
  {
     lastMatchedEntry_ = currEntry ;

     // Update statistical counters
     //
     numSrchd_ += nSrchd;
     totByCfC_ += currEntry->getOptPClen() * sizeof( PCodeBinary ) + currEntry->getNEConstsLen();

     numHits_++ ;
     if ( NEflag && ( currEntry->getOptConstsLen() < currEntry->getNEConstsLen() ) )
        numNEHits_++ ;
     UInt64 nHits = currEntry->incrPCEHits() ;
     if ( nHits > maxHits_ )
        maxHits_ = nHits ;

     // Put this cache entry at the head of the MRU list
     //
     if ( currEntry != MRUHead_ )
     {
        removeFromMRUList( currEntry ); // Remove from current position in MRU list
        addToMRUList( currEntry );      // Add it back in as MRU head
     }

#if OPT_PCC_DEBUG==1
     if ( PCECLoggingEnabled_ == 1 )
     {
        addToTotalSavedTime( currEntry->getOptTime() + currEntry->getNEgenTime() ) ;
        logPCCEvent( 1, currEntry, sqlStmt );
     }
#endif // OPT_PCC_DEBUG==1

     return currEntry ;
  }
  return NULL ;
}

//
// setPCDlogDirPath() - Saves the specified log directory pathname
// in memory allocated as part of this PCode Expr Cache
//
void
OptPCodeCache::setPCDlogDirPath( NAString * logDirPth )
{
   // These should not happen, but just in case ...
   if ( ( logDirPth == NULL ) || ( logDirPath_ != NULL ) )
      return ;

   Int32 len   = logDirPth->length() ;

   if ( len == 0 ) 
      return;

   logDirPath_ = new( heap_ ) char[ len +1 ];

   strncpy( logDirPath_, logDirPth->data(), len );
   logDirPath_[len] = '\0';
   PCECHeaderWritten_ = 0 ; // Need header line at start of file
}

void
OptPCodeCache::clearStats()
{
    numLookups_      = 0 ;
    numSrchd_        = 0 ;
    totSrchd_        = 0 ;
    numHits_         = 0 ;
    numNEHits_       = 0 ;
    maxHits_         = 0 ;
    maxHitsDel_      = 0 ;
    totByCfC_        = 0 ;
    maxOptPCodeSize_ = 0 ;

#if OPT_PCC_DEBUG==1
    totalSavedTime_  = 0 ;
    totalSearchTime_ = 0 ;
    totalOptTime_    = 0 ;
    totalNEgenTime_  = 0 ;
#endif // OPT_PCC_DEBUG==1

}

void
OptPCodeCache::resizeCache( Int32 newsiz )
{
   maxSize_ = newsiz ;
   throwOutExcessCacheEntries() ;
}

void
OptPCodeCache::throwOutExcessCacheEntries()
{
  while ( currSize_ > maxSize_ && numEntries_ > 0 )
  {
     PCECacheEntry * PCEtoDel    = MRUTail_ ;
     if ( PCEtoDel == NULL )                  // Shouldn't happen, but just in case ...
        break;

     removeFromMRUList( PCEtoDel );

     removeFromCreateOrderList( PCEtoDel );

     numEntries_-- ;

     if ( PCEtoDel->getPCEHits() > maxHitsDel_ )
        maxHitsDel_ = PCEtoDel->getPCEHits() ;

     if ( PCEtoDel == lastMatchedEntry_ )
        lastMatchedEntry_ = NULL ;

#if OPT_PCC_DEBUG==1
     if ( maxSize_ > 0 ) // Don't log when we are deleting the entire cache.
        logPCCEvent( 3, PCEtoDel, (char *)"Threw entry from cache" );
#endif // OPT_PCC_DEBUG==1

     currSize_ -= PCEtoDel->getTotalMemorySize() ;

     NADELETEBASIC( PCEtoDel , heap_ );
  }
}

#if OPT_PCC_DEBUG==1
void
OptPCodeCache::genUniqFileNamePart()
{
   if ( fileNameTime_ == -1 )
   {
     Int32 myPid = getpid();

     timeval curTime;
     GETTIMEOFDAY(&curTime, 0);
     Int64 timeInMics = ((Int64)curTime.tv_sec) * 1000000 + curTime.tv_usec ;

     fileNamePid_  = myPid ;
     fileNameTime_ = timeInMics ;
   }
}

void
OptPCodeCache::logPCCEvent( Int32           eventType
                          , PCECacheEntry * PCEptr
                          , char          * sqlStmt
                          )
{
#define LNGBUFLEN 1900

   char longBuf[LNGBUFLEN];

   if ( ( PCECLoggingEnabled_ == 0 ) ||
        ( logDirPath_ == NULL) || (*logDirPath_ == '\0') )
      return;

   Int32 logDirPathLen = strlen( logDirPath_ ) ; 

   //Form complete log file pathname, with unique part at end
   longBuf[0]='\0';
   sprintf(longBuf,"%s/PCEC.%x.%lx", logDirPath_, fileNamePid_, fileNameTime_);
   
   ofstream fileout( longBuf, ios::app); // Ensure file is created & open

#define MAX_UNIQ_PART (7+4+16)
   if ( ( PCECHeaderWritten_ == 0 ) &&
        ( logDirPathLen <= (LNGBUFLEN - MAX_UNIQ_PART - 1 )) )
   {
     PCECHeaderWritten_ = 1 ; // Remember so we put file hdr out only once
     sprintf(longBuf, "Ev Typ\tUniqCtr\tPCE Hits\t"
                      "PC Len\t"
                      "Opt PC Len\t"
                      "NE Con Len\tOpt Time\t"
                      "NEgen Tm\t"
                      "# Lookups\tTot Hits\tTot NE Hits\tTot Srchd\t"
                      "Num Srchd\tCurr SZ\tNum Entr\tCurr UniqCtr\t"
                      "Saved Tm\tSearch Tm\tTot Opt Tm\t"
                      "Tot NEgen Tm\tTotByCfC\tMX PCE Hits\tMX Hits Del\t"
                      "Max PC Len\t"
                      "SQL STATEMENT\n" );
     fileout << longBuf ;
   }
   if ( PCECHeaderWritten_ == 0 )
      return;

   sprintf(longBuf, "%d\t %ld\t %ld\t"         // eventType ...
                    "%ld\t "                   // PCEptr->getUnOptPClen()
                    "%ld\t "                   // PCEptr->getOptPClen()
                    "%d\t %ld\t "              // PCEptr->getNEConstsLen() ...
                    "%ld\t "                   // PCEptr->getNEgenTime()
                    "%ld\t %ld\t %ld\t %ld\t " // numLookups_     ...
                    "%ld\t %d\t %d\t %ld\t "   // numSrchd_       ...
                    "%ld\t %ld\t %ld\t "       // totalSavedTime_ ...
                    "%ld\t %ld\t %ld\t %ld\t " // totalNEgenTime_ ...
                    "%ld\t "                   // maxOptPCodeSize_ 
                   , eventType , PCEptr->getUniqCtr() , PCEptr->getPCEHits()
                   , PCEptr->getUnOptPClen()*sizeof(PCodeBinary)
                   , PCEptr->getOptPClen()*sizeof(PCodeBinary)
                   , PCEptr->getNEConstsLen() , PCEptr->getOptTime()
                   , PCEptr->getNEgenTime()
                   , numLookups_ , numHits_  , numNEHits_  , totSrchd_
                   , numSrchd_   , currSize_ , numEntries_ , uniqueCtr_
                   , totalSavedTime_ , totalSearchTime_     , totalOptTime_
                   , totalNEgenTime_ , totByCfC_ , maxHits_ , maxHitsDel_
                   , maxOptPCodeSize_ * sizeof(PCodeBinary)
                   );
   fileout << longBuf ;
   if ( sqlStmt != NULL )
   {
      strncpy( longBuf, sqlStmt, (LNGBUFLEN - 4) );
      longBuf[LNGBUFLEN - 4] = '\0'; // Ensure a null byte
      fileout << longBuf ;
   }
   fileout << "\n" ;
}
#endif // OPT_PCC_DEBUG==1

//
// printPCodeExprCacheStats() -- intended to be called by gdb to give the
// developer a formatted view of the cache's statistics.
//
void
OptPCodeCache::printPCodeExprCacheStats()
{
   printf("\nPCode Expression Cache Statistics - for cache anchored at %p\n", this);
   printf("MaxSize = %d (KB), CurrSize = %d, NumEntries = %d, NumLookups = %ld, NumHits = %ld\n",
           maxSize_ ,         currSize_ ,    numEntries_ ,    numLookups_ ,     numHits_ );
   printf("MaxOptPCodeSize = %d, MaxHitsForAnyOneEntry = %ld, MaxHitsForAnyDeletedEntry = %ld\n",
           maxOptPCodeSize_ ,    maxHits_ ,                   maxHitsDel_ );
}
