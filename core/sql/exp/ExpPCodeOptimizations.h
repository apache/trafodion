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

#ifndef EXP_PCODE_OPTIMIZATIONS_H
#define EXP_PCODE_OPTIMIZATIONS_H

//
// Definitions for Native Expression debug messages
//
#define VV_XD 14  /* Extremely Verbose Debugging       */
#define VV_VD 12  /* Verbose   Debugging               */
#define VV_BD 10  /* Basic     Debugging               */
#define VV_I3  8  /* Extended  Instrumentation Level 3 */
#define VV_I2  6  /* Extended  Instrumentation Level 2 */
#define VV_I1  4  /* Extended  Instrumentation Level 1 */
#define VV_I0  2  /* Basic     Instrumentation Level 0 */
#define VV_NO  0  /* No        Instrumentation         */

//
// For common subroutines (typically in the PCodeOperand class), we
// may not have access to the NExprDbgLvl_ member variable in the
// PCodeCfg class to determine the user-specified debug level.
// However, the debugging messages in these common subroutines are
// all needed only in the Extremely Verbose debugging mode anyway.
// So, we need only a #define to be able to easily turn them on/off.

#define NExprDbgLvl VV_XD  // See #defines above.

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ExpPCodeOptimizations.h
 * Description:  Infrastructure to perform low-level optimizations on PCODE
 *               
 *               
 * Created:      7/11/2007
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ExpError.h"
#include "exp_attrs.h"

#include "Collections.h"
#include "ComSpace.h"
#include "Int64.h"
#include "OperTypeEnum.h"
#include "ExpAtp.h"
#include "exp_expr.h"
#include "exp_function.h"
#include "exp_clause.h"
#include "exp_clause_derived.h"
#include "NABitVector.h"
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>

#define NA_LINUX_LLVMJIT
#undef  NA_LINUX_LIBJIT

//
// NOTE: The old LIBJIT code is being kept around, but #ifdef'd out,
//       because it is a useful guide to how to implement PCODE-to-Native Code
//       translations using LLVM.

#ifdef NA_LINUX_LIBJIT
  #include "jit/jit.h"
  #include "jit/jit-dump.h"
#endif

#ifdef NA_LINUX_LLVMJIT

//
// NOTE: /usr/include/sys/types.h and an LLVM header file pulled in via
//       the following " #include DerivedTypes.h " both try to use
//       typedef to define ssize_t...resulting in a C++ compilation
//       error.  This override code prevents the compilation error.
//
#ifdef __ssize_t_defined
#define ssize_t  my_llvm_ssize_t
#define ssize_t_overridden_here
#endif
#include "DerivedTypes.h"
#ifdef ssize_t_overridden_here
#undef ssize_t_overridden_here
#undef ssize_t
#endif

#ifdef COMING_FROM_NATIVE_EXPR_SOURCE_FILE
#ifndef _byteswap_ushort

inline uint16_t _byteswap_ushort(uint16_t value) {
  uint16_t Hi = value << 8;
  uint16_t Lo = value >> 8;
  return Hi | Lo;
}

#endif  /* _byteswap_ushort */

#include "ExecutionEngine/ExecutionEngine.h"

#include "ExecutionEngine/JIT.h"
#include "LLVMContext.h"
#include "llvm/Module.h"  /* Must put llvm/ here to avoid including .../cli/Module.h */
#include "PassManager.h"
#include "Analysis/Verifier.h"
#include "Analysis/Passes.h"
#include "DataLayout.h"
#include "Transforms/Scalar.h"

#include "IRBuilder.h"
#include "Config/config.h"
#include "Support/TargetSelect.h"
#include "InstrTypes.h" // for ICMP_SLT, ICMP_EQ, etc
#include "Instructions.h"
#include "CodeGen/MachineCodeInfo.h"
#include "Type.h"

typedef llvm::IRBuilder<> IRBldr_t ;

// NOTE:  IR stands for Intermediate Representation.  It is the input that
//        is acceptable to libjit's JIT compiler or the input that
//        is acceptable to LLVM's JIT compiler.
//        Using LLMV, a IR Block *is* an llvm::BasicBlock.
//        Using LIBJIT, a IR Block would hold LIBJIT's intermediate rep.

typedef llvm::BasicBlock * p_IR_block_t ;

#else    // NOT COMING FROM NATIVE EXPR SOURCE FILE

typedef char             * p_IR_block_t ; // Dummy def'n to prevent C++ complaints
#endif // COMING_FROM_NATIVE_EXPR_SOURCE_FILE

typedef llvm::Value      * jit_value_t  ; // To simplify converting from old LIBJIT
typedef llvm::Type       * jit_type_t   ; // To simplify converting from old LIBJIT

#define IR_block_undefined  ((p_IR_block_t) NULL)


  enum cmpKind { IntCompare_EQ
               , IntCompare_NE
               , IntCompare_SGT
               , IntCompare_UGT
               , IntCompare_SGE
               , IntCompare_UGE
               , IntCompare_SLT
               , IntCompare_ULT
               , IntCompare_SLE
               , IntCompare_ULE
               , ByteCompare_EQ
               , ByteCompare_NE
               , ByteCompare_LT
               , ByteCompare_LE
               , ByteCompare_GT
               , ByteCompare_GE
               };

#endif  /* NA_LINUX_LLVMJIT */

// Forward external declaractions
//
class PCodeOperand;
class PCodeInst;
class PCodeBlock;
class PCodeCfg;
class PCodePredicateGroup;
class NExTempListEntry ;

typedef NAList<PCodeOperand*> OPLIST;
typedef NAList<PCodeBlock*> BLOCKLIST;
typedef NAList<PCodeInst*> INSTLIST;
typedef NAList<PCodePredicateGroup*> GROUPLIST;
typedef NAList<NExTempListEntry*> NExTEMPSLIST ;

typedef NAArray<PCodeBlock*> BLOCKARRAY;

ULng32 collIndexHashFunc2(const CollIndex & o);

// -----------------------------------------------------------------------
// Macros
// -----------------------------------------------------------------------
#define FOREACH_BLOCK_ALL(PCBlk, firstPCInst, lastPCInst, indx) \
  { PCodeBlock* PCBlk;                                          \
    PCodeInst* firstPCInst;                                     \
    PCodeInst* lastPCInst;                                      \
    CollIndex indx;                                             \
    for (indx=0; indx < allBlocks_->entries(); indx++) {        \
      PCBlk = allBlocks_->at(indx);                             \
      firstPCInst = PCBlk->getFirstInst();                      \
      lastPCInst = PCBlk->getLastInst();                        \
      if (firstPCInst == NULL) {                                \
        assert (lastPCInst == NULL);                            \
        /*continue;*/                                           \
      }

#define FOREACH_BLOCK(PCBlk, firstPCInst, lastPCInst, indx)     \
        FOREACH_BLOCK_ALL(PCBlk, firstPCInst, lastPCInst, indx)

#define ENDFE_BLOCK                                                \
  }}

#define FOREACH_BLOCK_FAST(PCBlk, firstPCInst, lastPCInst, indx) \
  { PCodeBlock* PCBlk;                                           \
    PCodeInst* firstPCInst;                                      \
    PCodeInst* lastPCInst;                                       \
    CollIndex indx;                                              \
    for (indx=0; indx < allBlocks_->entries(); indx++) {         \
      PCBlk = allBlocks_->at(indx);                              \
      firstPCInst = PCBlk->getFirstInst();                       \
      lastPCInst = PCBlk->getLastInst();                         \
      if (PCBlk->getPreds().isEmpty() && (PCBlk != entryBlock_)) \
        continue;                                                \
      if (firstPCInst == NULL) {                                 \
        assert (lastPCInst == NULL);                             \
        /*continue;*/                                            \
      }

#define ENDFE_BLOCK_FAST                                           \
  }}


#define FOREACH_BLOCK_DFO_AT(e, PCBlk, firstPCInst, lastPCInst, indx) \
  { PCodeBlock* PCBlk;                                                \
    PCodeInst* firstPCInst;                                           \
    PCodeInst* lastPCInst;                                            \
    CollIndex indx;                                                   \
    BLOCKARRAY _list(heap_);                                          \
    PCodeBlock* _entry = entryBlock_;                                 \
    entryBlock_ = e;                                                  \
    getBlocksInDFO(_list);                                            \
    entryBlock_ = _entry;                                             \
    for (indx=0; indx < _list.entries(); indx++) {                    \
      PCBlk = _list[indx];                                            \
      firstPCInst = PCBlk->getFirstInst();                            \
      lastPCInst = PCBlk->getLastInst();                              \
      if (firstPCInst == NULL) {                                      \
        assert (lastPCInst == NULL);                                  \
        /*continue;*/                                                 \
      }

#define ENDFE_BLOCK_DFO_AT                                         \
  }}


#define FOREACH_BLOCK_DFO(PCBlk, firstPCInst, lastPCInst, indx) \
  { PCodeBlock* PCBlk;                                          \
    PCodeInst* firstPCInst;                                     \
    PCodeInst* lastPCInst;                                      \
    CollIndex indx;                                             \
    BLOCKARRAY _list(heap_);                                    \
    getBlocksInDFO(_list);                                      \
    for (indx=0; indx < _list.entries(); indx++) {              \
      PCBlk = _list[indx];                                      \
      firstPCInst = PCBlk->getFirstInst();                      \
      lastPCInst = PCBlk->getLastInst();                        \
      if (firstPCInst == NULL) {                                \
        assert (lastPCInst == NULL);                            \
        /*continue;*/                                           \
      }

#define ENDFE_BLOCK_DFO                                            \
  }}


#define FOREACH_BLOCK_REV_DFO_AT(e,PCBlk,firstPCInst,lastPCInst,indx) \
  { PCodeBlock* PCBlk;                                                \
    PCodeInst* firstPCInst;                                           \
    PCodeInst* lastPCInst;                                            \
    CollIndex indx;                                                   \
    BLOCKARRAY _list(heap_);                                          \
    PCodeBlock* _entry = entryBlock_;                                 \
    entryBlock_ = e;                                                  \
    getBlocksInDFO(_list);                                            \
    entryBlock_ = _entry;                                             \
    for (indx=_list.entries(); indx > 0; indx--) {                    \
      PCBlk = _list[indx-1];                                          \
      firstPCInst = PCBlk->getFirstInst();                            \
      lastPCInst = PCBlk->getLastInst();                              \
      if (firstPCInst == NULL) {                                      \
        assert (lastPCInst == NULL);                                  \
        /*continue;*/                                                 \
      }

#define ENDFE_BLOCK_REV_DFO_AT                                     \
  }}


#define FOREACH_BLOCK_REV_DFO(PCBlk, firstPCInst, lastPCInst, indx) \
  { PCodeBlock* PCBlk;                                              \
    PCodeInst* firstPCInst;                                         \
    PCodeInst* lastPCInst;                                          \
    CollIndex indx;                                                 \
    BLOCKARRAY _list(heap_);                                        \
    getBlocksInDFO(_list);                                          \
    for (indx=_list.entries(); indx > 0; indx--) {                  \
      PCBlk = _list[indx-1];                                        \
      firstPCInst = PCBlk->getFirstInst();                          \
      lastPCInst = PCBlk->getLastInst();                            \
      if (firstPCInst == NULL) {                                    \
        assert (lastPCInst == NULL);                                \
        /*continue;*/                                               \
      }

#define ENDFE_BLOCK_REV_DFO                                        \
  }}


#define FOREACH_BLOCK_BACKWARDS(PCBlk, firstPCInst, lastPCInst, indx) \
  { PCodeBlock* PCBlk;                                                \
    PCodeInst* firstPCInst;                                           \
    PCodeInst* lastPCInst;                                            \
    CollIndex indx;                                                   \
    for (indx=allBlocks_->entries()-1;(Int32)indx>=0;indx--) {        \
      PCBlk = allBlocks_->at(indx);                                   \
      firstPCInst = PCBlk->getFirstInst();                            \
      lastPCInst = PCBlk->getLastInst();                              \
      if (PCBlk->getPreds().isEmpty() && (PCBlk != entryBlock_))      \
        continue;                                                     \
      if (firstPCInst == NULL) {                                      \
        assert (lastPCInst == NULL);                                  \
        /*continue;*/                                                 \
      }

#define ENDFE_BLOCK_BACKWARDS                                      \
  }}

#define FOREACH_INST_IN_BLOCK(PCBlk, inst)                         \
  { PCodeInst* inst;                                               \
    PCodeInst* _next;                                              \
    for (inst = PCBlk->getFirstInst(); inst; inst = _next) {       \
      _next = inst->next;

#define RESTART_INST_IN_BLOCK                                      \
      _next = inst;

#define ENDFE_INST_IN_BLOCK                                        \
  }}


#define FOREACH_INST_IN_BLOCK_AT(PCBlk, inst, start)               \
  { PCodeInst* inst;                                               \
    PCodeInst* _next;                                              \
    for (inst = start; inst; inst = _next) {                       \
      _next = inst->next;

#define RESTART_INST_IN_BLOCK_AT                                   \
      _next = inst;

#define ENDFE_INST_IN_BLOCK_AT                                     \
  }}



#define FOREACH_INST_IN_BLOCK_BACKWARDS(PCBlk, inst)               \
  { PCodeInst* inst;                                               \
    PCodeInst* _prev;                                              \
    for (inst = PCBlk->getLastInst(); inst; inst = _prev) {        \
      _prev = inst->prev;

#define RESTART_INST_IN_BLOCK_BACKWARDS                            \
      _prev = inst;

#define ENDFE_INST_IN_BLOCK_BACKWARDS                              \
  }}


#define FOREACH_INST_IN_BLOCK_BACKWARDS_AT(PCBlk, inst, start)     \
  { PCodeInst* inst;                                               \
    PCodeInst* _prev;                                              \
    for (inst = start; inst; inst = _prev) {                       \
      _prev = inst->prev;

#define RESTART_INST_IN_BLOCK_BACKWARDS_AT                         \
      _prev = inst;

#define ENDFE_INST_IN_BLOCK_BACKWARDS_AT                           \
  }}


// -----------------------------------------------------------------------
// Defines
// -----------------------------------------------------------------------

#define TRIGGER_COUNT		5000


// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------

//
// PCodePredicateGroup
//
// This class represents a predicate group used to encapsulate predicates seen
// in SCAN expressions.  It consists of 1 or more PCodeBlocks within a list.
// All operations in terms swapping and costing, for both static and dynamic
// predicate reordering, are defined here.
//
class PCodePredicateGroup
{
private:

  CollHeap* heap_;          // Heap used for all memory allocation
  BLOCKLIST predicates_;    // List of predicate blocks comprising this group
  Int64 cost_;              // Cost associated with this group
  Int64 takenCount_;        // Early-exit counts for this group
  Int64 seenCount_;         // Total seen counts for this group
  OPLIST reads_;            // Read operands not defined in group
  OPLIST writes_;           // Write operands defined in group and used by
                            // another group.

public:

  PCodePredicateGroup(CollHeap* heap,
                      PCodeBlock* member,
                      NABoolean isStatic,
                      PCodeCfg* cfg);

  ~PCodePredicateGroup() {}

  // Get list of predicate blocks
  BLOCKLIST& getPredicateBlocks() { return predicates_; }

  // Get last predicate block
  PCodeBlock* getLastPredBlock() { return predicates_[predicates_.entries()-1];}

  // Get first predicate block
  PCodeBlock* getFirstPredBlock() { return predicates_[0]; }

  // Get the major selectivity of this group
  float getSelectivity(NABoolean isStatic);

  // Get the minor selectivity of this group
  float getMinorSelectivity(NABoolean isStatic);

  // Get the cost of this group
  Int64 getCost() { return cost_; }

  // Set the costs and counters for the unit headed by this predicate group.
  void setCost(GROUPLIST& unit, NABoolean isStatic);

  // Reset counts of predicate groups based on their placement in the unit
  void adjustCost(GROUPLIST& unit);

  // Get the taken count of this group
  Int64 getTakenCount() { return takenCount_; }

  // Get the seen count of this group
  Int64 getSeenCount() { return seenCount_; }

  // Set the taken count of this group
  void setTakenCount(Int64 count) { takenCount_ = count; }

  // Set the seen count of this group
  void setSeenCount(Int64 count) { seenCount_ = count; }

  // Return the reads operand list
  OPLIST& getReads() { return reads_; }

  // Return the writes operand list
  OPLIST& getWrites() { return writes_; }

  // Add a read operand to the reads list
  void addRead(PCodeOperand* i) { reads_.insert(i); }

  // Add a write operand to the writes list
  void addWrite(PCodeOperand* i) { writes_.insert(i); }

  // Print group
  void print();

  // Determines if the block qualifies as being a predicate
  static NABoolean isPredicateBlock(PCodeBlock* block);

  // Mark those predicate groups in the allGroups list which are not PIC
  void identifyPICGroups(PCodeCfg* cfg, CollHeap* heap, GROUPLIST& allGroups);

  // Determines if any swapping can be done between predicate groups in the
  // passed in unit, starting from head to tail.
  NABoolean containsXBlock(GROUPLIST& unit, CollIndex head, CollIndex tail);

  // Swap this predicate group's position in the evaluation order with the
  // child predicate group passed in.  Also indicate if the child block passed
  // in is adjacent.
  void swap(PCodePredicateGroup* down, NABoolean adjacent);

  // Recursive function to find units, headed by this predicate group, and swap
  // them accordingly.
  NABoolean findUnits(GROUPLIST& unit, GROUPLIST& workList,
                      NABoolean& swapPerformed, NABoolean isStatic);

  // Determines if this group dominates the passed in group
  NABoolean dominates(PCodePredicateGroup* sibling);

  // Destroy all basic predicate groups in the passed in list of groups
  void destroy(GROUPLIST& allGroups);
};


//
// PCodeConstants
//
// This class serves to represent constants used in pcode.  Two flavors of
// constant tracking are maintained here.  The first is a bit-vector
// implementation where only 0, 1, and -1 constants are tracked.  These
// constants are heavily used in pcode (e.g. null indicators, TRUE, and FALSE).
// The bit-vector implmentation allows for fast data-flow analysis to be 
// performed.  The defined routines operate on bit vectors, where the element
// position represents the operand, and the bit set or unset represents whether
// or not that operand is a known constant.  A special constant (value 2)
// indicates that the operand is neither 0, 1, or -1.
//
// The second use of this class is to maintain and organize all general
// constants (strings, numerics, floats, etc).  The primary goal here is to
// prevent duplicate constant generation (or stated differently, to reuse
// already allocated constants).  This is useful during optimizations such as
// common sub-expression elimination, and is also important for good space
// allocation.
//
class PCodeConstants
{
public:

  /////////////////////////////////////////////////
  // General constant tracking routines
  /////////////////////////////////////////////////

  void* data_;  // Pointer to data
  Int32 len_;   // Length of data
  Int32 align_; // Alignment of data

  friend ULng32 constHashFunc(const PCodeConstants& c);

  // Two constants are equal if their lengths, alignment, and data are the same.
  NABoolean operator==(const PCodeConstants& other) const {
    if (other.len_ != len_)
      return FALSE;

    if (other.align_ != align_)
      return FALSE;

    if ((other.data_ == data_) || (memcmp(other.data_, data_, len_) == 0))
      return TRUE;

    return FALSE;
  }

  void* getData()  { return data_; }
  Int32 getLen()   { return len_; }
  Int32 getAlign() { return align_; }

  PCodeConstants (void* data, Int32 len, Int32 align) {
    data_ = data;
    len_ = len;
    align_ = align;
  }

  /////////////////////////////////////////////////
  // Constant tracking routines for 0, 1, or -1
  /////////////////////////////////////////////////

  static const Int32 UNKNOWN_CONSTANT = 2;

  // Is the passed in constant a 0, 1, or -1
  static NABoolean isQualifiedConstant(Int64 constant);

  // Can the passed in operand be 0, 1, or -1
  static NABoolean isAnyKnownConstant(CollIndex operandIndex,
                                      NABitVector& zeroes,
                                      NABitVector& ones,
                                      NABitVector& neg1);

  // Reset the passed in operand for each of the constant vectors, making it an
  // unknown constant
  static void clearConstantVectors(CollIndex operandIndex,
                                   NABitVector& zeroes,
                                   NABitVector& ones,
                                   NABitVector& neg1);

  // Return 0, 1, or -1, if the passed in constant is known.  Return 2 otherwise
  static Int32 getConstantValue(CollIndex bvIndex,
                                NABitVector& zeroes,
                                NABitVector& ones,
                                NABitVector& neg1);

  // Copy the constant information known for bvIndexFrom into bvIndexTo.
  static Int32 copyConstantVectors(CollIndex bvIndexFrom,
                                   CollIndex bvIndexTo,
                                   NABitVector& zeroes,
                                   NABitVector& ones,
                                   NABitVector& neg1);

  // Merge the constant vectors together and fold results back into newZeroes,
  // newOnes, and newNeg1.
  static void mergeConstantVectors(NABitVector& newZeroes,
                                   NABitVector& newOnes,
                                   NABitVector& newNeg1,
                                   NABitVector& zeroes,
                                   NABitVector& ones,
                                   NABitVector& neg1);

  // Modify the constant vectors so that the passed in operand index is
  // associated with the passed in constant.
  static void setConstantInVectors(Int32 constant,
                                   CollIndex bvIndex,
                                   NABitVector& zeroes,
                                   NABitVector& ones,
                                   NABitVector& neg1);

  // Can this operand have the value 1?
  static NABoolean canBeOne(CollIndex bvIndex, NABitVector& ones);

  // Can this operand have the value 0?
  static NABoolean canBeZero(CollIndex bvIndex, NABitVector& zeroes);

  // Can this operand have the value -1?
  static NABoolean canBeNeg1(CollIndex bvIndex, NABitVector& neg1);
};


//
// NullTriple
//
// This class represents a null indicator operand, complete with the atp,
// index, and offset params.  Hash and equality functions are decalred for use
// in a hash table representing all known null indicators.  This is currently
// done only for the EXPLODED format
//
class NullTriple
{
public:
  Int32 atp_;  // Atp of null indicator
  Int32 idx_;  // Tupp index of null indicator
  Int32 off_;  // Column offset of null indicator

  friend ULng32 nullTripleHashFunc(const NullTriple& o);

  NABoolean operator==(const NullTriple& other) const {
    return ((other.off_ == off_) && (other.idx_ == idx_) &&
            (other.atp_ == atp_));
  }

  NullTriple(Int32 atp, Int32 idx, Int32 off) {
    atp_ = atp;
    idx_ = idx;
    off_ = off;
  }
};


//
// ReachDefs
//
class ReachDefsTable
{
public:

  class ReachDefsElem
  {
    public:
      CollIndex         bvIndex_;  // Index of operand that reaches
      PCodeInst*        inst_;     // Inst defining operand that reaches
      ReachDefsElem*    next_;     // Pointer to next hash table chain element

      ReachDefsElem (CollIndex b, PCodeInst* i) {
        bvIndex_ = b;
        inst_ = i;
        next_ = NULL;
      }

      ReachDefsElem (ReachDefsElem* copy) {
        bvIndex_ = copy->bvIndex_;
        inst_ = copy->inst_;
        next_ = copy->next_;
      }
  };

  // Constructor
  ReachDefsTable(CollHeap* heap, CollIndex size) {
    assert(size > 0);

    size_ = size;
    heap_ = heap;

    rDefIn_  = new(heap_) ReachDefsElem*[size_];
    rDefOut_ = new(heap_) ReachDefsElem*[size_];
    rDefVec_ = new(heap_) NABitVector(heap_);

    str_pad((char*)rDefIn_,  size_ * sizeof(ReachDefsElem*), 0);
    str_pad((char*)rDefOut_, size_ * sizeof(ReachDefsElem*), 0);
  }

  // Destructor
  ~ReachDefsTable() { destroy(); }

  // Copy incoming hash table into in/out table
  Int32 copy(ReachDefsElem** tabToCopy, NABoolean isInTab);

  // Merge incoming hash table into in/out table
  Int32 merge(ReachDefsElem** tabToMerge, NABoolean isInTab);

  // Find reaching operand in in/out table
  PCodeInst* find(CollIndex bv, NABoolean isInTab);

  // Find reaching operand in in/out table - search Out, then In
  PCodeInst* find(CollIndex bv);

  // Insert reaching operand into in/out table
  NABoolean insert(CollIndex bv, PCodeInst* inst, NABoolean isInTab);

  // Remove a reaching operand from in/out table
  void remove(CollIndex bv, NABoolean isInTab);

  // Deallocate space used by the hash tables
  void destroy();

  // Accessors
  ReachDefsElem** getRDefIn()  { return rDefIn_;  }
  ReachDefsElem** getRDefOut() { return rDefOut_; }
  NABitVector*    getRDefVec() { return rDefVec_; }
  CollIndex       getSize()    { return size_;    }

private:

  ReachDefsElem** rDefIn_;  // Hash table for ops that reach in
  ReachDefsElem** rDefOut_; // Differential hash table for ops that reach out
  NABitVector*    rDefVec_; // Bit vector used for faster search of reach defs

  CollHeap*       heap_;    // Heap needed for allocation

  CollIndex       size_;    // Fixed size of hash table
};


//
// PCodeOperand
//
// This class represents an operand of a pcode instruction.  At it's core is
// a set of member variables describing the pcode stack index and offset values
// for the operand.  There are, however, many other fields to accurately
// identify and distinguish this operand from others.  There are also hash
// and equality functions used for inserting this operand into a hash table.
//
class PCodeOperand
{
private:

  // Position of stack index in pcode
  Int32 stackIndexPos_;

  // Position of offset in pcode
  Int32 offsetPos_;

  // Stack index of operand
  Int32 stackIndex_;

  // Offset of operand
  Int32 offset_;

  // Potential null bit index of operand
  Int32 nullBitIndex_;

  // Operand type
  PCIT::AddressingMode operandType_;

  // Bit vector index for this operand
  Int32 bvIndex_;

  // Length of operand
  Int32 len_;

  // Position of varchar voaOffset
  signed char voaOffsetPos_;

  // Position of varchar vc indicator offset
  signed char vcLenIndicatorOffPos_;

  // Varchar indicator len;
  signed char vcIndicatorLen_;

  // Varchar null indicator len;
  signed char vcNullIndicatorLen_;

  // Varchar voaOffset
  Int32 voaOffset_;

  // Varchar vc indicator offset
  Int32 vcLenIndicatorOff_;

  // Varchar maxLen;
  Int32 vcMaxLen_;

  /**********************************************
   * Native Expr Related
   **********************************************/

  jit_value_t  jitValue_;      // Actual value of operand
  jit_value_t  jitValuePtr_;   // Pointer to operand
  jit_type_t   jitType_;       // Type of operand
  PCodeBlock * jitValueBlock_; // Block where jitValue_ of operand was last defined
  PCodeBlock * jitValuePtrBlock_; // Block where jitValuePtr_ of operand was last defined

public:

  // Hash function used to insert operand bv element into hash table
  friend ULng32 operandHashFunc(const PCodeOperand& o);

  friend ULng32 collIndexHashFunc(const CollIndex & o);

  // Operator equals function to compare two operand bv elements
  NABoolean operator==(const PCodeOperand& other) const {
    return ((other.stackIndex_ == stackIndex_) && (other.offset_ == offset_) &&
            (other.nullBitIndex_ == nullBitIndex_) &&
            (other.voaOffset_ == voaOffset_));
  }


  // Constructor I
  PCodeOperand(Int32 stackIndex,
               Int32 offset,
               Int32 nullBitIndex,
               Int32 voaOffset) :
    stackIndex_(stackIndex),
    offset_(offset),
    nullBitIndex_(nullBitIndex),
    bvIndex_(0),
    voaOffset_(voaOffset)
  {
    /**********************************************
     * Native Expr Related
     **********************************************/
    jitValue_ = NULL;
    jitValuePtr_ = NULL;
    jitValueBlock_ = NULL;
    jitValuePtrBlock_ = NULL;

  }


  // Constructor II
  PCodeOperand(Int32 stackIndex,
               Int32 offset,
               Int32 nullBitIndex,
               Int32 stackIndexPos,
               Int32 offsetPos,
               Int32 bvIndex,
               PCIT::AddressingMode opType,
               Int32 len,
               Int32 voaOffset) :
    stackIndex_(stackIndex),
    offset_(offset),
    nullBitIndex_(nullBitIndex),
    stackIndexPos_(stackIndexPos),
    offsetPos_(offsetPos),
    bvIndex_(bvIndex),
    len_(len),
    operandType_(opType),
    voaOffsetPos_(-1),
    vcLenIndicatorOffPos_(-1),
    voaOffset_(voaOffset),
    vcLenIndicatorOff_(-1),
    vcMaxLen_(-1),
    vcIndicatorLen_(0),
    vcNullIndicatorLen_(0)
  {
    /**********************************************
     * Native Expr Related
     **********************************************/
    jitValue_ = NULL;
    jitValuePtr_ = NULL;
    jitValueBlock_ = NULL;
    jitValuePtrBlock_ = NULL;

  }

  // Get stack index
  Int32 getStackIndex() { return stackIndex_; }

  // Get offset
  Int32 getOffset() { return offset_; }

  // Get null bit index
  Int32 getNullBitIndex() { return nullBitIndex_; }

  NABoolean forAlignedFormat() { return (nullBitIndex_ != -1); }

  // Get pcode position of stack index
  Int32 getStackIndexPos() { return stackIndexPos_; }

  // Get pcode position of offset
  Int32 getOffsetPos() { return offsetPos_; }

  // Get the type of the operand 
  PCIT::AddressingMode getType() { return operandType_; }

  // Set the type of the operand
  void setType(PCIT::AddressingMode type) { operandType_ = type; }

  // Get the len of the operand
  Int32 getLen() { return len_; }

  // Set the len of the operand
  void setLen(Int32 len) { len_ = len; }

  // Get the alignment of the operand
  Int32 getAlign();

  // Varchar related functionality

  signed char getVoaOffsetPos() { return voaOffsetPos_; }
  signed char getVcLenIndicatorOffPos() { return vcLenIndicatorOffPos_; }
  Int32 getVoaOffset() { return voaOffset_; }
  Int32 getVcLenIndicatorOff() { return vcLenIndicatorOff_; }
  Int32 getVcMaxLen() { return vcMaxLen_; }
  signed char getVcIndicatorLen() { return vcIndicatorLen_; }
  signed char getVcNullIndicatorLen() { return vcNullIndicatorLen_; }

  void setVoaOffsetPos(signed char pos) { voaOffsetPos_ = pos; }
  void setVcLenIndicatorOffPos(signed char pos) { vcLenIndicatorOffPos_ = pos; }
  void setVoaOffset(Int32 off) { voaOffset_ = off; }
  void setVcLenIndicatorOff(Int32 off) { vcLenIndicatorOff_ = off; }
  void setVcMaxLen(Int32 len) { vcMaxLen_ = len; }
  void setVcIndicatorLen(signed char len) { vcIndicatorLen_ = len; }
  void setVcNullIndicatorLen(signed char len) { vcNullIndicatorLen_ = len; }

  void setVarcharFields(signed char  voaOffsetPos,
                        signed char  vcLenIndicatorOffPos,
                        Int32 voaOffset,
                        Int32 vcLenIndicatorOff,
                        Int32 vcMaxLen,
                        signed char  vcNullIndicatorLen,
                        signed char  vcIndicatorLen) {

    voaOffsetPos_ = voaOffsetPos;
    vcLenIndicatorOffPos_ = vcLenIndicatorOffPos;
    voaOffset_ = voaOffset;
    vcLenIndicatorOff_ = vcLenIndicatorOff;
    vcMaxLen_ = vcMaxLen;
    vcNullIndicatorLen_ = vcNullIndicatorLen;
    vcIndicatorLen_ = vcIndicatorLen;
  }

  NABoolean hasSameLength(PCodeOperand* op1) {
    if (getType() != op1->getType()) {
      // If types don't match, the operands may not be "identical".  As of
      // today, this can only happen between bignums and largeints.
      if (((getType() == PCIT::MBIGS) && (op1->getType() == PCIT::MBIN64S)) ||
          ((getType() == PCIT::MBIN64S) && (op1->getType() == PCIT::MBIGS)))
        return FALSE;
    }
    if (hasKnownLen() && op1->hasKnownLen())
      return (getLen() == op1->getLen());
    if (isVarchar() && op1->isVarchar())
      return (getVcMaxLen() == op1->getVcMaxLen());
    return FALSE;
  }

  NABoolean hasKnownLen() { return (len_ != -1); }
  NABoolean canOverlap(PCodeOperand* oper);  

  // Get the bit vector index for this operand
  Int32 getBvIndex() { return bvIndex_; }

  // Public accessor to return jitValue_ for this operand
  jit_value_t getCurrJitVal() { return jitValue_ ; }

  // Copy constructor
  PCodeOperand* copy(CollHeap* heap)
  {
    PCodeOperand* operand =
      new(heap) PCodeOperand(stackIndex_,
                             offset_,
                             nullBitIndex_,
                             stackIndexPos_,
                             offsetPos_,
                             bvIndex_,
                             operandType_,
                             len_,
                             voaOffset_);
#if 0
#if NExprDbgLvl >= VV_XD
printf("ZZZ In COPY Constructor(): allocated new PCodeOperand at %p, AM=%d\n", operand, operand->getType() );
#endif
#endif
    return operand;
  }

  // Is operand an empty varchar
  NABoolean isEmptyVarchar() { return (isVarchar() && (getVcMaxLen() == 0)); }

  // Is operand a garbage value
  NABoolean isGarbage() { return (stackIndex_ == 0); }

  // Is operand a constant
  NABoolean isConst() { return (stackIndex_ == 1); }

  // Is operand a temp
  NABoolean isTemp() { return (stackIndex_ == 2); }

  // Is operand a variable
  NABoolean isVar() { return (stackIndex_ > 2); }

  // Is operand a varchar
  NABoolean isVarchar() { return (vcIndicatorLen_ > 0); }

  // Is operand an indirect varchar
  NABoolean isIndirectVarchar() { return isVarchar() && (offset_ == -1); }

  // Is operand an Unsigned type
  NABoolean isUnsignedType() { return (operandType_ == PCIT::MBIN16U) ||
                                      (operandType_ == PCIT::MBIN32U) ||
                                      (operandType_ == PCIT::MDECU)   ||
                                      (operandType_ == PCIT::MBIGU) ; }
  void print( PCodeCfg * cfg );

  /**********************************************
   * Native Expr Related
   **********************************************/

  // Is operand a temp that needs to be allocated in memory?
  NABoolean isNonNativeTemp() {
    // Only bignums and strings are supported for now.
    return (isTemp() &&
             ((getType() == PCIT::MBIGS) ||
              (getType() == PCIT::MATTR5)));
  }

#ifdef NA_LINUX_LLVMJIT
#ifdef COMING_FROM_NATIVE_EXPR_SOURCE_FILE
  jit_value_t getJitValue( PCodeCfg       * cfg
                         , IRBldr_t       * Builder
                         , jit_type_t       type
                         , PCodeBlock     * b
                         , PCodeOperand   * orig
                         , NABoolean        noSaveJitVals
                         );

  void setJitValue(PCodeCfg* cfg, IRBldr_t *, jit_value_t val,
                   PCodeBlock* b, NABoolean noSaveJitVals);

  void clearJitValues(PCodeCfg* cfg);

  jit_type_t  getJitType( PCodeCfg  *  cfg,
                          PCIT::AddressingMode oprTyp = PCIT::AM_NONE );

  void storeJitValue( PCodeCfg    * cfg
                    , IRBldr_t    * Builder
                    , jit_type_t    type
                    , jit_value_t   val
                    , PCodeBlock  *
                    , jit_value_t * len
                    , NABoolean     noSaveJitVals
                    );

  void storeNullJitValueAndBranch( PCodeCfg   * cfg,
                                   IRBldr_t   * Builder,
                                   jit_value_t  value,
                                   p_IR_block_t targetLabel,
                                   NABoolean    forComp,
                                   PCodeBlock * b );

  void allocJitValue( PCodeCfg            * cfg
                    , IRBldr_t            * Bldr
                    , PCIT::AddressingMode  OprTyp
                    , Int32                 Len
                    , Int32                 Num
                    );


#endif /* COMING_FROM_NATIVE_EXPR_SOURCE_FILE */
#endif /* NA_LINUX_LLVMJIT */

#ifdef NA_LINUX_LIBJIT

  jit_value_t getJitValue(PCodeCfg* cfg,
                          jit_function_t func,
                          jit_type_t type,
                          PCodeBlock* b,
                          PCodeOperand* orig,
                          NABoolean noSaveJitVals);

  void setJitValue(PCodeCfg* cfg, jit_function_t, jit_value_t val,
                   PCodeBlock* b, NABoolean noSaveJitVals);

  void clearJitValues(PCodeCfg* cfg);

  jit_type_t  getJitType();

  void storeJitValue(PCodeCfg*, jit_function_t, jit_type_t, jit_value_t val,
                     PCodeBlock*, jit_value_t* len, NABoolean noSaveJitVals);

  void storeNullJitValueAndBranch(PCodeCfg*, jit_function_t, jit_value_t,
                                  jit_label_t*, NABoolean comp, PCodeBlock* b);

  PCodeBlock* getJitValueBlock() { return jitValueBlock_; }

#endif /* NA_LINUX_LIBJIT */
};


//
// PCodeInst
//
// This class represent a single pcode instruction.  Included in this
// representation is a classification of it's operands, as well as a pointer
// to the pcode bytstream of this pcode instruction.
//
class PCodeInst
{
private:

  CollHeap* heap_;         // Passed in from cfg (create operands with)
  Space* space_;           // Passed in from cfg (create pcode insts with)
  OPLIST readOps;          // List of read operands
  OPLIST writeOps;         // List of write operands
  Int32 cost_;             // Cost of instruction

public:

  PCodeBinary * code;      // Pointer to pcode inst data
  PCodeInst* prev;         // Pointer to previous PCodeInst
  PCodeInst* next;         // Pointer to next PCodeInst
  PCodeBlock* block;       // Pointer to block containing this instruction
  ex_clause* clause_;      // Clause pointer associated with this instruction
  Int32 opdataLen_;        // If inst is opdata, this is operand length

  NABitVector liveVector_;  // Live vector

  // Operator equals function to compare two operand bv elements
  NABoolean operator==(const PCodeInst& other) const {
    return (&other == this);
  }

  PCodeInst(CollHeap* heap) :
    liveVector_(heap),
    prev(NULL),
    next(NULL),
    readOps(heap),
    writeOps(heap),
    heap_(heap),
    space_(NULL),
    clause_(NULL),
    opdataLen_(-1),
    cost_(1) {}

  PCodeInst(CollHeap* heap, Space* space, PCIT::Instruction inst) :
    heap_(heap),
    space_(space),
    prev(NULL),
    next(NULL),
    block(NULL),
    readOps(heap),
    writeOps(heap),
    liveVector_(heap),
    clause_(NULL),
    opdataLen_(-1),
    cost_(1)
  {
    PCodeBinary c = inst;
    Int32 length = PCode::getInstructionLength(&c);
    code = (space_) ? new(space_) PCodeBinary[length] : new(heap_) PCodeBinary[length];
    code[0] = inst;
  }

  ~PCodeInst();

  void destroy();

  NABoolean isEqComp();
  NABoolean isIntEqComp();
  NABoolean isFloatEqComp();
  NABoolean isStringEqComp();
  NABoolean isVarcharNullBranch();
  NABoolean isNullBitmap();
  NABoolean isCopyMove();
  NABoolean isSwitch();
  NABoolean isInListSwitch();
  NABoolean isMove();
  NABoolean isMoveToBignum();
  NABoolean isConstMove();
  NABoolean isBranch();
  NABoolean isNullBranch();
  NABoolean isBulkNullBranch();
  NABoolean isLogicalBranch();
  NABoolean isAnyLogicalBranch();
  NABoolean isLogicalCountedBranch();
  NABoolean isOpdata();
  NABoolean isEncode();
  NABoolean isDecode();
  NABoolean isRange();
  NABoolean isHash();
  NABoolean isVarcharInstThatSupportsFixedCharOps();
  NABoolean isUncBranch() { return (getOpcode() == PCIT::BRANCH); }
  NABoolean isClauseEval() { return (getOpcode() == PCIT::CLAUSE_EVAL); }
  NABoolean isIndirectBranch() {
    return (getOpcode() == PCIT::BRANCH_INDIRECT_MBIN32S);
  }

  NABoolean mayFailIfNull();

  // Routines for setting up the pcode array for certain instructions
  void setupPcodeForVarcharMove(PCodeOperand* tgt, PCodeOperand* src);
  void replaceOperand(PCodeOperand* oldSrc, PCodeOperand* newSrc);

  // Accessor routines for counters used by logical branches
  void setTriggerCount(UInt32 count) { code[2] = count; }
  void setSeenCount(UInt32 count) { code[7] = count; }
  void setTakenCount(UInt32 count) { code[8] = count; }
  void clearCounters() { setSeenCount(0); setTakenCount(0); }

  UInt32 getTriggerCount() { return code[2]; }
  UInt32 getSeenCount() { return code[7]; }
  UInt32 getTakenCount() { return code[8]; }

  PCIT::Instruction generateInt64MoveOpc(PCIT::AddressingMode am);
  PCIT::Instruction generateFloat64MoveOpc(PCIT::AddressingMode am);
  Int32 generateCopyMoveOpc(PCIT::AddressingMode);

  NABoolean containsIndirectVarcharOperand();

  void getBranchTargetOffsetPointers(NAList<PCodeBinary*>*, PCodeCfg* cfg);
  void getBranchTargetPointers(NAList<Long*>*, PCodeCfg* cfg);
  void getBranchTargets(NAList<Int32>*, PCodeCfg* cfg);

  void modifyOperandsForVarchar(PCodeCfg* cfg);

  Int32 getCost() { return cost_; }
  void setCost(Int32 cost) { cost_ = cost; }

  void print(PCodeCfg*, NABoolean debugFlag);

  Int32 getOpcode() { return code[0]; }

  OPLIST& getROps() { return readOps; }

  OPLIST& getWOps() { return writeOps; }

  // TODO: Maybe remove operands here as well
  void reloadOperands(PCodeCfg* cfg);
};


//
// PCodeBlock
//
// This class represents a block of PCodeInst pointers.  A block is a sequence
// of instructions where control flow can only enter from the top and exit from
// the bottom.
//
class PCodeBlock
{
private:

  CollHeap* heap_;           // Heap used for memory allocation
  PCodeInst* first_;         // First instruction in block
  PCodeInst* last_;          // Last instruction in block
  NABoolean visitedFlag_;    // Flag used to indicate if block has been visited
  Int32 blockNum_;           // Block number for identification
  Int32 blockTopOffset_;     // Physical offset of block upon entry
  Int32 blockBottomOffset_;  // Physical offset of block upon exit
  BLOCKLIST succsList_;      // List of successor blocks
  BLOCKLIST predsList_;      // List of predecessor blocks
  PCodeCfg* cfg_;            // Pointer to containing cfg
  Int32 cost_;               // Cost of block

  NABitVector liveVector;    // Live vector

#ifdef NA_LINUX_LIBJIT
  jit_label_t jitLabel_;     // Jit label for block
#endif

#ifdef NA_LINUX_LLVMJIT
  p_IR_block_t  p_IR_Block_ ; // Ptr to starting IR block for this PCode block
#endif

public:

  NABitVector onesVector;    // Ones constant vector
  NABitVector zeroesVector;  // Zeroes constant vector
  NABitVector neg1Vector;    // Neg1 constant vector

  friend ULng32 collIndexHashFunc2(const CollIndex & o);

  ReachDefsTable* reachingDefsTab_;

  NAHashDictionary<CollIndex, PCodeInst>* reachingDefsTableIn_;
  NAHashDictionary<CollIndex, PCodeInst>* reachingDefsTableOut_;

  // Constructor
  PCodeBlock(Int32 blockNum,
             PCodeInst* first,
             CollHeap *heap,
             PCodeCfg* cfg)
    : blockNum_(blockNum),
      first_(first),
      last_(first),
      heap_(heap),
      cfg_(cfg),
      succsList_(heap),
      predsList_(heap),
      liveVector(heap),
      onesVector(heap),
      zeroesVector(heap),
      neg1Vector(heap),
      visitedFlag_(FALSE),
      reachingDefsTab_(NULL),
      cost_(1)
  {
    reachingDefsTableIn_ = new(heap_) NAHashDictionary<CollIndex, PCodeInst>
                            (&collIndexHashFunc2, 10, TRUE, heap_);

    reachingDefsTableOut_ = new(heap_) NAHashDictionary<CollIndex, PCodeInst>
                             (&collIndexHashFunc2, 10, TRUE, heap_);

#ifdef NA_LINUX_LIBJIT
    jitLabel_ = jit_label_undefined;
#endif
#ifdef NA_LINUX_LLVMJIT
    p_IR_Block_ = IR_block_undefined;
#endif

  }

  // Destructor
  ~PCodeBlock() {}

  NABoolean isEntryBlock();
  void setToEntryBlock();

  NABoolean isExitBlock()
  {
    return (last_ && ((last_->getOpcode() == PCIT::RETURN) ||
                      (last_->getOpcode() == PCIT::RETURN_IBIN32S)));
  }

  Int32 getCost() { return cost_; }
  void setCost(Int32 cost) { cost_ = cost; }

  void setFirstInst(PCodeInst* first) { first_ = first; }
  void setLastInst(PCodeInst* last) { last_ = last; }
  void setVisitedFlag(NABoolean flag) { visitedFlag_ = flag; }
  void setBlockTopOffset(Int32 offset) { blockTopOffset_ = offset; }
  void setBlockBottomOffset(Int32 offset) { blockBottomOffset_ = offset; }

  PCodeInst* getFirstInst() { return first_; }
  PCodeInst* getLastInst() { return last_; }
  NABoolean getVisitedFlag() { return visitedFlag_; }
  Int32 getBlockTopOffset() { return blockTopOffset_; }
  Int32 getBlockBottomOffset() { return blockBottomOffset_; }
  Int32 getBlockNum() { return blockNum_; }

  PCodeOperand* isIndirectBranchCandidate(PCodeBlock* headBlock,
                                          PCodeOperand* var,
                                          NABoolean forInList);

  NABitVector& getLiveVector() { return liveVector; }

  NABoolean isOperandLiveInSuccs(PCodeOperand* op) {
    CollIndex i, bvIndex = op->getBvIndex();
    for (i=0; i < getSuccs().entries(); i++) {
      PCodeBlock* succ = getSuccs()[i];
      if (succ->getLiveVector().testBit(bvIndex))
        return TRUE;
    }

    return FALSE;
  }

  void addSucc(PCodeBlock* succBlock) { succsList_.insert(succBlock); }
  void addPred(PCodeBlock* predBlock) { predsList_.insert(predBlock); }

  void removeSucc(PCodeBlock* succBlock) { succsList_.remove(succBlock); }
  void removePred(PCodeBlock* predBlock) { predsList_.remove(predBlock); }

  void addEdge(PCodeBlock* succBlock) {
    addSucc(succBlock);
    succBlock->addPred(this);
  }

  void removeEdge(PCodeBlock* succBlock) {
    removeSucc(succBlock);
    succBlock->removePred(this);
  }

  void addFallThroughEdge(PCodeBlock* succBlock)
  {
    succsList_.insertAt(0, succBlock);
    succBlock->addPred(this);
  }

  PCodeInst* insertNewInstBefore(PCodeInst* inst, PCIT::Instruction instr);
  PCodeInst* insertNewInstAfter(PCodeInst* inst, PCIT::Instruction instr);
  PCodeInst* insertInstBefore(PCodeInst* inst, PCodeInst* newInst);
  PCodeInst* insertInstAfter(PCodeInst* inst, PCodeInst* newInst);
  void deleteInst(PCodeInst* inst);

  BLOCKLIST& getSuccs() { return succsList_; }
  BLOCKLIST& getPreds() { return predsList_; }

  // Find def (if one exists) starting at the inst *before* start.  Perform
  // globally if selected to do so.
  PCodeInst* findDef(PCodeInst* start, PCodeOperand* operand, NABoolean global);

  // Setup cseList for insts in this block, creating the list if necessary
  void setupForCSE(INSTLIST** cseList, NABoolean createNewList);

  // Determine if this block qualifies for short-circuiting
  NABoolean doesBlockQualifyForShortCircuit();

  // The first edge of a succs list is always the fall-through block. If, 
  // however, were dealing with an unconditional branch, then its target block
  // is represented by the first edge. Otherwise, the target block is always 
  // the second element.
  PCodeBlock* getTargetBlock()
  {
    assert (!last_->isIndirectBranch());
    return (last_->isUncBranch() ? succsList_[0] : succsList_[1]);
  }

  NABoolean branchesTo(PCodeBlock* block) {
    if (last_ && last_->isBranch() && (getTargetBlock() == block))
      return TRUE;

    if (last_ && last_->isIndirectBranch()) {
      for (CollIndex i=0; i < getSuccs().entries(); i++)
        if (getSuccs()[i] == block)
          return TRUE;
    }

    return FALSE;
  }

  PCodeBlock* determineTargetBlock(NABitVector& zeroes,
                                   NABitVector& ones,
                                   NABitVector& neg1);

  PCodeInst* reduceBranch(PCodeBlock* to,
                          NABitVector& zeroes,
                          NABitVector& ones,
                          NABitVector& neg1);

  void fixupConstantVectors(PCodeBlock* targetBlock,
                            NABitVector& newZeroes,
                            NABitVector& newOnes,
                            NABitVector& newNeg1);

  void mergeConstantVectors(NABitVector& newZeroes,
                            NABitVector& newOnes,
                            NABitVector& newNeg1,
                            NABitVector& zeroes,
                            NABitVector& ones,
                            NABitVector& neg1);

  PCodeBlock* getFallThroughBlock() { return succsList_[0]; }

  NABoolean isEmpty() { return (first_ == NULL); }

  NABoolean print(NABoolean debugFlag);

  /**********************************************
   * Native Expr Related
   **********************************************/

#ifdef NA_LINUX_LIBJIT
  jit_label_t* getJitLabel() { return &jitLabel_; };
#endif

#ifdef NA_LINUX_LLVMJIT
//
// NOTE: LIBJIT has a concept of being able to "branch to a label", but
// with LLVM, you must branch to the beginning of an IR block.
// Hence the change in the name of this routine as well as the return value.
//
  p_IR_block_t * getJitBlkLbl() { return &p_IR_Block_ ; };
#endif

};


//
// PCodeCfg
//
// This class represents a Control Flow Graph (CFG) of a particular pcode
// implemented expression.  This class is responsible for optimizing the passed
// in sequence of pcode bytestream if and when possible.
//
class PCodeCfg
{

private:  

  // Expression to optimize
  ex_expr* expr_;

  // Heap and space pointers
  CollHeap* heap_;
  CollHeap* origHeap_;
  CollHeap* rDefsHeap_;
  Space* space_;

  // Atp and AtpIndex maps
  Int32* atpMap_;
  Int32* atpIndexMap_;

  // Entry block for pcode sequence
  PCodeBlock* entryBlock_;

  // Pointer to all blocks created
  BLOCKLIST* allBlocks_;

  // Pointer to all instructions created
  INSTLIST* allInsts_;

  // Flags for cfg object
  UInt32 flags_;

  // What is the offset for the zero constant operand
  Int32 zeroOffset_;

  CollIndex maxBVIndex_;
  Int32 maxBlockNum_;

  struct {
    Int32 hashCombCnt_;
    Int32 nullBranchCnt_;
    Int32 logicalBranchCnt_;
  } counters_;

  NAHashDictionary<NullTriple, Int32>* nullToPadMap_;
  NAHashDictionary<PCodeOperand, CollIndex>* operandToIndexMap_;
  NAHashDictionary<CollIndex, PCodeOperand>* indexToOperandMap_;
  NAHashDictionary<ULong, PCodeBinary>* targets_;

  NABitVector* ones_;    // Ones constant vector
  NABitVector* zeroes_;  // Zeroes constant vector
  NABitVector* neg1_;    // Neg1 constant vector

  NAHashDictionary<PCodeConstants, CollIndex>* constToOffsetMap_;
  NAHashDictionary<CollIndex, PCodeConstants>* offsetToConstMap_;
  Int32 newConstsAreaLen_;

  // Optimization flags
  UInt32 optFlags_;

  static const Int32 COPY_PROP_MAX_RECURSION = 25;

#ifdef NA_LINUX_LLVMJIT

  /**********************************************
   * Native Expr Related
   **********************************************/

  jit_value_t* jitParams_;  // Array of incoming parameters to native expr.

  //
  // Ptrs to useful LLVM Constant Values
  //
  jit_value_t IR_Const_0_     ; // Has 32-bit 0 value.
  jit_value_t IR_Const_1_     ; // Has 32-bit 1 value.
  jit_value_t IR_Const_neg1_  ; // Has 32-bit -1 value.
  jit_value_t IR_Const_16bit_0_ ; // Has 16-bit 0 value.
  jit_value_t IR_Const_64bit_0_ ; // Has 64-bit 0 value.

  jit_value_t IR_Const_OK_    ; // Has ex_expr::OK value.
  jit_value_t IR_Const_TRUE_  ; // Has ex_expr::TRUE value.
  jit_value_t IR_Const_FALSE_ ; // Has ex_expr::FALSE value.
  jit_value_t IR_Const_ERR_   ; // Has ex_expr::ERROR value.

  jit_value_t hashTableJitVal_ ;// Pointer to hash table at runtime

  NABoolean jitFailureSeen_   ; // Flag to determine if a jit failure was seen.

  jit_value_t  pGlobTab_Val_  ;
  jit_value_t  atp1_Val_      ;
  jit_value_t  atp2_Val_      ;
  jit_value_t  ex_expr_p_Val_ ;

  //
  // Ptrs to useful LLVM Type objects.  We put them in the PCodeCfg object
  // so they can be referenced from any of the Native Expression methods.
  //
  jit_type_t  int8Ty_     ; 
  jit_type_t  int16Ty_    ;
  jit_type_t  int32Ty_    ;
  jit_type_t  int64Ty_    ;
  jit_type_t  int8PtrTy_  ;
  jit_type_t  int16PtrTy_ ;
  jit_type_t  int32PtrTy_ ;
  jit_type_t  int64PtrTy_ ;
  jit_type_t  int1PtrTy_  ;  // Used like old JIT's void ptr
  jit_type_t  floatPtrTy_ ;

  NExTEMPSLIST * NExTempsList_ ;

  NExDbgInfo *NExDbgInfoPtr_ ; // Native Expr Dbg Info Pointer
  Int32       NExprDbgLvl_ ;   // Native Expr Debug Level

#endif  /* NA_LINUX_LLVMJIT */

#ifdef NA_LINUX_LIBJIT

  /**********************************************
   * Native Expr Related
   **********************************************/

  jit_value_t* jitParams_;  // Array of incoming parameters to native expr.

  jit_value_t zeroJitVal_;  // Has 0 value.
  jit_value_t oneJitVal_;   // Has 1 value.
  jit_value_t neg1JitVal_;  // Has -1 value.

  jit_value_t okJitVal_;    // Has ex_expr::OK value.
  jit_value_t tJitVal_;     // Has ex_expr::TRUE value.
  jit_value_t fJitVal_;     // Has ex_expr::FALSE value.

  jit_value_t hashTableJitVal_;    // Pointer to hash table at runtime

  NABoolean jitFailureSeen_; // Flag to determine if a jit failure was seen.

#endif /* NA_LINUX_LIBJIT */

public:

  enum {
    REMOVE_UNREACHABLE_BLOCKS = 0x1,
    REMOVE_TRAMPOLINE_CODE = 0x2,
    MERGE_BLOCKS = 0x4,
    REMOVE_EMPTY_BLOCKS = 0x8,
    LIVE_ANALYSIS_AVAILABLE = 0x10
  };

  enum {
    REORDER_PREDICATES         = 0x1,
    DYNAMIC_REORDER_PREDICATES = 0x2,
    INLINING                   = 0x4,
    CSE                        = 0x8,
    INDIRECT_BRANCH            = 0x10,
    NATIVE_EXPR                = 0x20,
    BKWD_COPY_PROP             = 0x40,
    FWD_COPY_PROP              = 0x80,
    BULK_MOVE                  = 0x100,
    SHORT_CIRCUIT              = 0x200,
    AGGRESSIVE                 = 0x400,
    BULK_HASH                  = 0x800,
    OPT_PCODE_CACHE_DISABLED   = 0x1000,   // If ON, PCode Expr Cache is disabled.
    EXPR_CACHE_CMP_ONLY        = 0x2000,   // Compare-Only mode [for testing purposes only]
    LAST_OPT_FLAG              = 0x1FFFFFF
  };

  // Static const variables used in indirect branch optimization to signify a
  // an invalid value for equality comparisons.  It's used to identify a missing
  // value in the constants lookup table for this optimization.
  static const Int32 INVALID_INT32  = 0xFEFEFEFE;
  static const Int64 INVALID_INT64  = 0xFEFEFEFEFEFEFEFELL;

  #define INVALID_LONG INVALID_INT64

  // Static const variables used.
  static const Int32 MAX_NUM_PCODE_INSTS = 500;
  static const Int32 MAX_HASHCOMB_BULK_OPERANDS = 100;

  // Friend classes
  // friend ULng32 targetHashFunc(const Int32 & o);

  // Constructor Routines
  PCodeCfg(ex_expr* expr,
           Int32* atpMap,
           Int32* atpIndexMap,
           CollHeap *heap,
           Space * space) 
    : expr_(expr), space_(space), origHeap_(heap), rDefsHeap_(NULL),
      operandToIndexMap_(NULL), targets_(NULL), entryBlock_(NULL), 
      maxBVIndex_(0), zeroOffset_(-1), 
      flags_(0), indexToOperandMap_(NULL),
      atpMap_(atpMap),
      NExTempsList_(0),
      atpIndexMap_(atpIndexMap), nullToPadMap_(NULL), newConstsAreaLen_(0),
      constToOffsetMap_(NULL), offsetToConstMap_(NULL)
  {
    // Initialize heap_ to be a heap within the heap passed in, so that
    // deallocation can be done quickly.  If we're called from within EID,
    // however, heap_ should just be the heap passed in.

    heap_ = new(heap) NAHeap("Pcode", (NAHeap*)heap, (Lng32)32768);

    allBlocks_ = new(heap) BLOCKLIST(heap_, 10);
    allInsts_ = new(heap) INSTLIST(heap_, 20);
    zeroes_ = new(heap) NABitVector(heap_);
    ones_ = new(heap) NABitVector(heap_);
    neg1_ = new(heap) NABitVector(heap_);

    /**********************************************
     * Native Expr Related
     **********************************************/
    jitParams_ = NULL;
    jitFailureSeen_ = FALSE;

    // Copy Native Expression Debug Level from the expr into the PCodeCfg

    NExDbgInfoPtr_ = expr->getNExDbgInfoPtr() ;
    if ( NExDbgInfoPtr_ &&
         ( NExDbgInfoPtr_ > (NExDbgInfo *)(expr->getConstsLength()) )
       )
       NExprDbgLvl_    = NExDbgInfoPtr_->getNExDbgLvl() ;
    else
       NExprDbgLvl_    = 0 ;
  }

  // Constructor used only for showplan generation
  PCodeCfg(ex_expr* expr,
           CollHeap *heap)
    : expr_(expr), space_(NULL), origHeap_(heap), rDefsHeap_(NULL),
      atpMap_(NULL), atpIndexMap_(NULL), zeroOffset_(-1), maxBVIndex_(0),
      NExTempsList_(0),
      constToOffsetMap_(NULL), offsetToConstMap_(NULL)
  {
    heap_ = (heap == NULL) ? heap : new(heap) NAHeap("Pcode");

    allBlocks_ = new(heap) BLOCKLIST(heap_, 10);
    allInsts_ = new(heap) INSTLIST(heap_, 20);
    zeroes_ = new(heap) NABitVector(heap_);
    ones_ = new(heap) NABitVector(heap_);
    neg1_ = new(heap) NABitVector(heap_);

    /**********************************************
     * Native Expr Related
     **********************************************/
    jitParams_ = NULL;
    jitFailureSeen_ = FALSE;

    // Null out ptrs into Generator object as there is none for showplan
    // expr->setNExDbgInfo(NULL);
    NExDbgInfoPtr_ = NULL ;
    NExprDbgLvl_    = 0 ;
  }

  // Destructor Routines
  ~PCodeCfg();
  void destroy();

  // Accessor Routines
  PCodeBlock* getEntryBlock() { return entryBlock_; }
  void setEntryBlock(PCodeBlock* block) { entryBlock_ = block; }

  Int32  getNExprDbgLvl() { return NExprDbgLvl_ ; }

  // Utility Routines
  PCodeInst* createInst (PCIT::Instruction);
  PCodeInst* createInst (PCIT::Instruction, Int32 length);
  PCodeInst* copyInst (PCodeInst* inst);
  PCodeBlock* createBlock ();
  void deleteBlock(PCodeBlock* block);
  void clearVisitedFlags();

  // Setup Routines
  NABoolean canPCodeBeOptimized( PCodeBinary * pCode
                               , NABoolean   & pExprCacheable /* OUT */
                               , UInt32      & totalPCodeLen  /* OUT */ );
  void createCfg();
  NABoolean createInsts (PCodeBinary* pcode);
  NABoolean addOverlappingOperands(NABoolean detectOnly);
  void addOperand (PCodeBinary*, OPLIST&, Int32, Int32, Int32,
                   PCIT::AddressingMode, Int32, Int32);
  void loadOperandsOfInst (PCodeInst* newInst);
  ex_clause* findClausePtr(PCodeInst* inst);

  // Layout Routines
  NABoolean layoutCode();
  void createPhysList(BLOCKLIST& l);
  void createPhysListRecurse(PCodeBlock* b, BLOCKLIST& l);
  void assignBlockOffsetsAndFixup(BLOCKLIST& blockList);

  // Optimization Analysis Routines
  void computeDomTree();
  void computeLiveness(NABoolean performDCE);
  void computeReachingDefs(Int32 flags);
  void removeReachingDefs();
  void updateReachingDefs(PCodeBlock*, CollIndex, PCodeInst*, PCodeInst*);

  // Depth-first order algorithms (post-order)
  void getBlocksInDFO(BLOCKARRAY& list);
  void getBlocksInDFORecursive(BLOCKARRAY& list, PCodeBlock* block);

  // Dumping Routines
  void printInsts(NABoolean debugFlag);
  void printBlocks(const char* header, NABoolean debugFlag);
  void printConstants();

  // Showplan Routines
  void generateShowPlan(PCodeBinary* pCode, Space* space);

  // Optimization Routines
  void runtimeOptimize();
  void optimize();
  void peepholeConstants();
  void copyPropagation(Int32 phase, NABoolean reachDefsAvailable);
  NABoolean constantPropagation(NABoolean doPeeling);
  void constantFolding();
  PCodeInst* constantFold(PCodeInst* inst, NABoolean rDefsAvailable);
  void cfgRewiring(Int32 flags);
  NABoolean deadCodeElimination();
  NABoolean shortCircuitOpt(Int32 flags);
  void codeMotion();
  void flattenNullBranches();
  void inlining();
  NABoolean reorderPredicates(NABoolean isStatic);
  NABoolean globalCSE();
  void indirectBranchOpt(NABoolean forINList);

  // Bulk optimizations
  void bulkHashComb();
  void bulkNullBitmap();
  NABoolean bulkNullBranch();
  NABoolean removeBulkNullBranch();
  void bulkMove(NABoolean sameSizeMoves);

  // Counters related
  void initInstructionCounters() {
    counters_.hashCombCnt_ = 0;
    counters_.nullBranchCnt_ = 0;
    counters_.logicalBranchCnt_ = 0;
  }
  
  // Optimization Helper Routines
  CollIndex* createLookupTableForSwitch(OPLIST& constants, 
                                        BLOCKLIST& blocks,
                                        UInt32& size,
                                        NABoolean forInList);
  NABoolean indirectBranchReqMet(PCodeInst*, PCodeOperand*, CollIndex);
  NABoolean removeDuplicateBlocks(PCodeBlock* start);
  PCodeInst* extendPaddedNullIndMoves(PCodeInst* moveInst);
  PCodeBlock* copyCfg(PCodeBlock* start, PCodeBlock* end, PCodeBlock** map);
  void initNullMapTable();
  void initConstants();
  NABoolean updateConstVectors(PCodeOperand* operand);
  NABoolean removeOrRewriteDeadCodeInstruction(PCodeInst* inst);
  void markReachableBlocks(PCodeBlock* block);
  NABoolean cfgHasLoop();
  PCodeInst* findHashInst(PCodeOperand* def, PCodeInst* start);
  NABoolean copyPropBackwardsHelper(INSTLIST& list,
                                    PCodeBlock* block,
                                    PCodeInst* start,
                                    PCodeOperand* moveTgt,
                                    PCodeOperand* moveSrc,
                                    Int32 level);
  void copyPropForwardsHelper(PCodeBlock* block,
                              PCodeInst* start,
                              PCodeOperand* moveTgt,
                              PCodeOperand* moveSrc,
                              Int32 level);
  NABoolean copyPropForwardsHelper2(PCodeInst* start,
                                    PCodeOperand* useSrc,
                                    PCodeOperand* moveSrc);
  void shortCircuitOptHelper(PCodeBlock* block, NABitVector* bv,Int32 constant);
  PCodeInst* shortCircuitConstantProp(PCodeBlock* block,
                                      NABitVector* zeroes,
                                      NABitVector* ones,
                                      NABoolean* isBranchTaken);
  NABoolean localConstantPropagation(PCodeBlock* block,
                                     NABitVector& zeroes,
                                     NABitVector& ones,
                                     NABitVector& neg1);
  void localCopyPropForPeeling(BLOCKLIST& blockList);
  void localCodeMotion(PCodeBlock*);
  void localDeadCodeElimination();

  void reorderPredicatesHelper();

  NABoolean doOperandsHaveSameDef(PCodeInst* head, PCodeInst* tail);
  NABoolean doesInstQualifyForCSE(PCodeInst* inst);
  void setupForCSE(INSTLIST** cseList);
  NABoolean localCSE(INSTLIST** parent, PCodeBlock* tailBlock, NABoolean* rDef);
  void destroyForCSE(INSTLIST** cseList);

  // Temps Related
  Int32 addTemp(Int32 size, Int32 alignment);

  // Constants related
  CollIndex* addConstant(void* data, Int32 len, Int32 alignment);
  CollIndex* addConstant(PCodeOperand* op);
  Int32 addNewIntConstant(Int64 value, Int32 alignment);
  Int32 addNewFloatConstant(double value, Int32 alignment);
  void* getPtrConstValue(PCodeOperand*);
  Int64 getIntConstValue(PCodeOperand*);
  char* getStringConstValue(PCodeOperand*, Int32* len);
  double getFloatConstValue(PCodeOperand*);
  void layoutConstants();

  PCodeBlock* shortCircuitOptForBlock(PCodeBlock* block,
                                      NABoolean assumeTaken,
                                      NABoolean overlapFound,
                                      const NABitVector& zeroesConst,
                                      const NABitVector& onesConst,
                                      const NABitVector& neg1Const);

  // Accessor Routines
  NAHashDictionary<CollIndex, PCodeOperand>* getMap() {
    return indexToOperandMap_;
  }
  NAHashDictionary<PCodeOperand, CollIndex>* getOpToIndexMap() {
    return operandToIndexMap_;
  }


#ifdef NA_LINUX_LIBJIT

  /**********************************************
   * Native Expr Related
   **********************************************/

  // Offsets of globals and functions available at runtime in globals table.
  enum {
    JIT_GLOB_NULL_TABLE               = 0,
    JIT_GLOB_RANDOM_HASH_VALS_TABLE   = 4,
    JIT_GLOB_COMPARE_WITH_PAD_FUNC    = 8,
    JIT_GLOB_STRCMP_FUNC              = 12,
    JIT_GLOB_STRCPY_FUNC              = 16,
    JIT_GLOB_DBL_LE_CMP_FUNC          = 20,
    JIT_GLOB_DBL_GE_CMP_FUNC          = 24,
    JIT_GLOB_DBL_LT_CMP_FUNC          = 28,
    JIT_GLOB_DBL_GT_CMP_FUNC          = 32,
    JIT_GLOB_DBL_EQ_CMP_FUNC          = 36,
    JIT_GLOB_DBL_NE_CMP_FUNC          = 40,
    JIT_GLOB_REPORT_ERR_FUNC          = 44,
    JIT_GLOB_BIG_SUB_FUNC             = 48
  };
#endif /* NA_LINUX_LIBJIT */

  void layoutNativeCode(void);

  void layoutNativeCode(Space* space);  // used by showplan

  NABoolean canGenerateNativeExpr();
  jit_value_t* getJitParams() { return jitParams_; }

#ifdef NA_LINUX_LIBJIT
  void jitProcessFloatExceptionCheck(jit_function_t f,
                                     PCodeOperand* fRes,
                                     jit_label_t* errorJitLabel,
                                     PCodeBlock* block);
  NABoolean jitProcessPredicate(PCodeInst* comp,
                                jit_function_t f,
                                jit_label_t* falseLabel,
                                jit_label_t* trueLabel,
                                NABoolean dumpTrueBlockFirst);

  void genUnalignedMemset(jit_function_t f,
                          jit_value_t tgt,
                          Int32 val,
                          Int32 length);
#endif // NA_LINUX_LIBJIT

#ifdef NA_LINUX_LLVMJIT
#ifdef COMING_FROM_NATIVE_EXPR_SOURCE_FILE
  void genUnalignedMemset( IRBldr_t * Builder,
                           jit_value_t tgt,
                           Int32 val,
                           Int32 length);

  void genUnalignedMemcpy( IRBldr_t * Builder,
                           jit_value_t tgt,
                           jit_value_t src,
                           jit_value_t len,
                           Int32 constantLen);
#endif // COMING_FROM_NATIVE_EXPR_SOURCE_FILE
#endif /* NA_LINUX_LLVMJIT */


#ifdef NA_LINUX_LIBJIT
  void genUnalignedMemcpy(jit_function_t f,
                          jit_value_t tgt,
                          jit_value_t src,
                          jit_value_t len,
                          Int32 constantLen);

  void genBignumSub(PCodeCfg* cfg,
                    jit_function_t f,
                    PCodeOperand* res,
                    jit_value_t src1,
                    jit_value_t src2,
                    jit_value_t sign1,
                    jit_value_t sign2,
                    jit_value_t tSrc1,
                    jit_value_t tSrc2,
                    PCodeBlock* block);


  jit_value_t genStringSetup(jit_function_t f,
                             jit_value_t* tStr,
                             PCodeOperand* src,
                             NABoolean padExists,
                             NABoolean lenNeeded);

  jit_value_t genHash(jit_function_t f,
                      jit_value_t tStr,
                      jit_value_t tStrLen,
                      jit_value_t loopIndex,
                      Int32 constantLen);
#endif /* NA_LINUX_LIBJIT */

  void setupClauseOperand(PCodeCfg* cfg,
                          OPLIST& opList,
                          PCodeOperand** opData,
                          Int32 index,
                          ex_clause* clause);

#ifdef NA_LINUX_LIBJIT
  jit_value_t getZeroJitVal() { return zeroJitVal_; }
  jit_value_t getOneJitVal() { return oneJitVal_; }
  jit_value_t getNeg1JitVal() { return neg1JitVal_; }

  jit_value_t getHashTableJitVal() { return hashTableJitVal_; }

#endif /* NA_LINUX_LIBJIT */

  void setJitFailureSeen() { jitFailureSeen_ = TRUE; }
  NABoolean getJitFailureSeen() { return jitFailureSeen_; }

  NABoolean isSupportedClauseOperandType(Attributes* attr);

#ifdef NA_LINUX_LLVMJIT

  jit_value_t getZeroJitVal()    { return IR_Const_0_     ; }
  jit_value_t getOneJitVal()     { return IR_Const_1_     ; }
  jit_value_t getNeg1JitVal()    { return IR_Const_neg1_  ; }
  jit_value_t getFalse_JitVal_() { return IR_Const_FALSE_ ; }
  jit_value_t getError_JitVal_() { return IR_Const_ERR_   ; }

  jit_type_t  getInt8Ty()     { return int8Ty_     ; }
  jit_type_t  getInt16Ty()    { return int16Ty_    ; }
  jit_type_t  getInt32Ty()    { return int32Ty_    ; }
  jit_type_t  getInt64Ty()    { return int64Ty_    ; }
  jit_type_t  getInt8PtrTy()  { return int8PtrTy_  ; }
  jit_type_t  getInt16PtrTy() { return int16PtrTy_ ; }
  jit_type_t  getInt32PtrTy() { return int32PtrTy_ ; }
  jit_type_t  getInt64PtrTy() { return int64PtrTy_ ; }
  jit_type_t  getInt1PtrTy()  { return int1PtrTy_  ; } // Used like old JIT's void ptr
  jit_type_t  getFloatPtrTy() { return floatPtrTy_ ; }

#ifdef COMING_FROM_NATIVE_EXPR_SOURCE_FILE

  jit_value_t jitGenCompare( IRBldr_t    * Bldr
                           , enum cmpKind  kindOfCompare
                           , jit_value_t   oper1
                           , jit_value_t   oper2
                           , jit_value_t   trueRslt
                           , jit_value_t   falseRslt
                           );

  jit_value_t IR_LoadRelativeWithType( IRBldr_t    * Bldr
                                     , jit_value_t   int8p
                                     , Int64         byteOffset
                                     , jit_type_t    typeToLoad
                                     );

  void  IR_StoreRelativeWithType( IRBldr_t    * Bldr
                                , jit_value_t   val
                                , jit_value_t   tgt8ByPtr
                                , Int64         byteOffset
                                , jit_type_t    typeToStore
                                );

  void  genMemCopyLoop( IRBldr_t    * Bldr
                      , jit_value_t   srcPtr
                      , jit_value_t   tgtPtr
                      , jit_value_t   maxLen
                      );

  jit_value_t genMemCmpLoop( IRBldr_t    * Bldr
                           , enum cmpKind  kindOfCompare
                           , jit_value_t   srcPtr
                           , jit_value_t   tgtPtr
                           , jit_value_t   MaxLen
                           );

  void  genMemSetLoop( IRBldr_t   * Bldr
                     , Int32        val
                     , jit_value_t  tgtPtr
                     , jit_value_t  maxLen
                     );

  void  IR_insn_branch_if_zero( IRBldr_t     * Bldr
                              , jit_value_t    value
                              , p_IR_block_t * tgt_IR_block
                              );

  void  IR_insn_branch_if_not_zero( IRBldr_t     * Bldr
                                  , jit_value_t    value
                                  , p_IR_block_t * tgt_IR_block
                                  );

#endif /* COMING_FROM_NATIVE_EXPR_SOURCE_FILE */

  NABoolean inTempsList( Int32 Offset, PCIT::AddressingMode OprTyp );

  void addToTempsList( PCodeOperand * PCOp
                     , PCIT::AddressingMode oprTyp
                     , Int32 offset
                     , Int32 num = 1
                     );

  void NExLog(const char *data) ;

#endif /* NA_LINUX_LLVMJIT */

};

class NExTempListEntry
{
private:  
  PCodeOperand        * PCOp_   ;  // Ptr to PCodeOperand entry for Temp variable
  Int32                 num_    ;  // Number to allocate
  PCIT::AddressingMode  oprTyp_ ;  // Operand Type to use in allocJitValue()
  Int32                 offset_ ;  // Offset (that goes with stackIndex == 2)

public:
  PCodeOperand * getPCOp()                      { return PCOp_ ; }
  void           setPCOp( PCodeOperand * PCOp ) { PCOp_ = PCOp ; }

  PCIT::AddressingMode getOprTyp()              { return oprTyp_ ; }
  void                 setOprTyp( PCIT::AddressingMode opTyp) { oprTyp_ = opTyp ; }

  Int32          getNum()                       { return num_ ; }
  void           setNum( Int32 num )            { num_  = num ; }

  Int32          getOffset()                    { return offset_ ; }
  void           setOffset( Int32 offset )      { offset_ = offset ; }

  // Default constuctor
  NExTempListEntry() { };
};


#endif // EXP_PCODE_OPTIMIZATIONS_H
