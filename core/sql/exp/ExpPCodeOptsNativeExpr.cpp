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

// Native Expressions only officially supported on Linux for now.

///////////////////////////////////////////////////////////////////////
//
// The following are a few notes regarding native exprs
//
// 1. A few things are implemented differently for native expressions.
//    For example, NULL values will be represented internally as an unsigned
//    char.  It will have either a non-zero value (null) or zero value (not
//    null).
//
// 2. Code the the pcode graph is generated in program layout order for
//    libjit, as there doesn't seem to be any other way or doing this.  This
//    can prove difficult in maintaining correctness for determining the temps
//    that correspond to various operands.
//
// 3. Related to (2), both getJitValue() and storeJitValue() may need to be
//    re-examined (specifically for attempting to use a jitValue_ that was
//    declared along one particular path, but not on another.  Since jitValue_
//    for an operand is saved irrespective of control flow, we can have problems
//
// 4. If a value is created in along a conditional path, it may still be
//    referenced anywhere if it is marked addressable.
//
///////////////////////////////////////////////////////////////////////

#include "Platform.h"

#if 1   /* NA_LINUX_LLVMJIT */
#include "NAAssert.h"

// This #define must come *before* the #include "ExpPCodeOptimizations.h" line
#define COMING_FROM_NATIVE_EXPR_SOURCE_FILE

#endif  /* NA_LINUX_LLVMJIT */

#include "ExpPCodeOptimizations.h"
#include <signal.h>

#if 1   /* NA_LINUX_LLVMJIT */
#include "llvm/Support/raw_ostream.h"    /* FOR NEW MODULE DUMPING IDEA */
#include "llvm/Support/InstIterator.h"   /* FOR NEW MODULE DUMPING IDEA */
#include <fstream>
#endif  /* NA_LINUX_LLVMJIT */

//
// NExLog() - Routine used to log Native Expression debug info to the
// log file for this instance of the Compiler.
//
void PCodeCfg::NExLog(const char *data)
{
   char       * NExLogPth    = NULL ;

   if ( NExDbgInfoPtr_  &&  NExDbgInfoPtr_ > (NExDbgInfo *)4096 ) // May be offset
        NExLogPth = NExDbgInfoPtr_->getNExLogPath() ;

   if ( NExLogPth         && // May be null when called by showplan
       *NExLogPth != '\0' )  // OR may have null pathname
   {
     ofstream fileout( NExLogPth, ios::app);
     fileout << data ;
   }
}

#if NExprDbgLvl > VV_NO
//
// For routines in the PCodeCfg class, we have access to the
// NExprDbgLvl_ member variable and we use it to determine how
// verbose the user wants the output to be.
//
#define DPT0( str1, VBlvl, str2 )                                      \
 { if ( NExprDbgLvl_ >= VBlvl ) {                                      \
      sprintf( NExBuf, str1 str2 );                                    \
      NExLog( NExBuf ); } }
#define DPT1( str1, VBlvl, str2, Arg5 )                                \
 { if ( NExprDbgLvl_ >= VBlvl ) {                                      \
      sprintf( NExBuf, str1 str2, Arg5 );                              \
      NExLog( NExBuf ); } }
#define DPT2( str1, VBlvl, str2, Arg5, Arg6 )                          \
 { if ( NExprDbgLvl_ >= VBlvl ) {                                      \
      sprintf( NExBuf, str1 str2, Arg5, Arg6 );                        \
      NExLog( NExBuf ); } }
#define DPT3( str1, VBlvl, str2, Arg5, Arg6, Arg7 )                    \
 { if ( NExprDbgLvl_ >= VBlvl ) {                                      \
      sprintf( NExBuf, str1 str2, Arg5, Arg6, Arg7 );                  \
      NExLog( NExBuf ); } }
#define DPT4( str1, VBlvl, str2, Arg5, Arg6, Arg7, Arg8 )              \
 { if ( NExprDbgLvl_ >= VBlvl ) {                                      \
      sprintf( NExBuf, str1 str2, Arg5, Arg6, Arg7, Arg8 );            \
      NExLog( NExBuf ); } }
#define DPT5( str1, VBlvl, str2, Arg5, Arg6, Arg7, Arg8, Arg9 )        \
 { if ( NExprDbgLvl_ >= VBlvl ) {                                      \
      sprintf( NExBuf, str1 str2, Arg5, Arg6, Arg7, Arg8, Arg9 );      \
      NExLog( NExBuf ); } }
#define DPT6( str1, VBlvl, str2, Arg5, Arg6, Arg7, Arg8, Arg9, Arg10 )     \
 { if ( NExprDbgLvl_ >= VBlvl ) {                                          \
      sprintf( NExBuf, str1 str2, Arg5, Arg6, Arg7, Arg8, Arg9, Arg10 );   \
      NExLog( NExBuf ); } }
//
// NOTE: The following CDPTxx macros are useable ONLY within routines
//       that have 'cfg' defined as a pointer to the PCodeCfg.
//
#define CDPT0( str1, VBlvl, str2 )                                     \
 { if ( cfg->getNExprDbgLvl() >= VBlvl ) {                             \
      sprintf( NExBuf, str1 str2 );                                    \
      cfg->NExLog( NExBuf ); } }
#define CDPT1( str1, VBlvl, str2, Arg5 )                               \
 { if ( cfg->getNExprDbgLvl() >= VBlvl ) {                             \
      sprintf( NExBuf, str1 str2, Arg5 );                              \
      cfg->NExLog( NExBuf ); } }
#define CDPT2( str1, VBlvl, str2, Arg5, Arg6 )                         \
 { if ( cfg->getNExprDbgLvl() >= VBlvl ) {                             \
      sprintf( NExBuf, str1 str2, Arg5, Arg6 );                        \
      cfg->NExLog( NExBuf ); } }
#define CDPT3( str1, VBlvl, str2, Arg5, Arg6, Arg7 )                   \
 { if ( cfg->getNExprDbgLvl() >= VBlvl ) {                             \
      sprintf( NExBuf, str1 str2, Arg5, Arg6, Arg7 );                  \
      cfg->NExLog( NExBuf ); } }
#define CDPT4( str1, VBlvl, str2, Arg5, Arg6, Arg7, Arg8 )             \
 { if ( cfg->getNExprDbgLvl() >= VBlvl ) {                             \
      sprintf( NExBuf, str1 str2, Arg5, Arg6, Arg7, Arg8 );            \
      cfg->NExLog( NExBuf ); } }
#define CDPT5( str1, VBlvl, str2, Arg5, Arg6, Arg7, Arg8, Arg9 )       \
 { if ( cfg->getNExprDbgLvl() >= VBlvl ) {                             \
      sprintf( NExBuf, str1 str2, Arg5, Arg6, Arg7, Arg8, Arg9 );      \
      cfg->NExLog( NExBuf ); } }
#define CDPT6( str1, VBlvl, str2, Arg5, Arg6, Arg7, Arg8, Arg9, Arg10 )    \
 { if ( cfg->getNExprDbgLvl() >= VBlvl ) {                                 \
      sprintf( NExBuf, str1 str2, Arg5, Arg6, Arg7, Arg8, Arg9, Arg10 );   \
      cfg->NExLog( NExBuf ); } }


#define PRINT_DETAILS_OF_RDOp( OpNum, PCOp ) {                                \
    if ( NExprDbgLvl_ >= VV_XD ) {                                            \
      if ( PCOp->getStackIndex() == 2 )                                       \
         sprintf( NExBuf, "DETAILS of RD_Op[%d] at %p "                       \
                          "WITH stackIndex_ = 2 are: \n",                     \
                           OpNum, PCOp );                                     \
      else                                                                    \
         sprintf( NExBuf, "DETAILS of RD_Op[%d] at %p are: \n", OpNum, PCOp );\
      NExLog( NExBuf );                                                       \
      sprintf( NExBuf, "stackIndex_ = %d, offset_ = %d, len_=%d, "            \
             "operandType_=%d, nullBitIdx=%d, CJV=%p\n",                      \
             PCOp->getStackIndex(), PCOp->getOffset(), PCOp->getLen(),        \
             PCOp->getType(), PCOp->getNullBitIndex(), PCOp->getCurrJitVal());\
      NExLog( NExBuf );                                                       \
    }                                                                         \
  }

#define PRINT_DETAILS_OF_WROp( OpNum, PCOp ) {                                \
    if ( NExprDbgLvl_ >= VV_XD ) {                                            \
      if ( PCOp->getStackIndex() == 2 )                                       \
         sprintf( NExBuf, "DETAILS of WR_Op[%d] at %p "                       \
                          "WITH stackIndex_ = 2 are: \n",                     \
                                 OpNum, PCOp );                               \
      else                                                                    \
         sprintf( NExBuf, "DETAILS of WR_Op[%d] at %p are: \n", OpNum, PCOp );\
      NExLog( NExBuf );                                                       \
      sprintf( NExBuf, "stackIndex_ = %d, offset_ = %d, len_=%d, "            \
             "operandType_=%d, nullBitIdx=%d, CJV=%p\n",                      \
             PCOp->getStackIndex(), PCOp->getOffset(), PCOp->getLen(),        \
             PCOp->getType(), PCOp->getNullBitIndex(), PCOp->getCurrJitVal());\
      NExLog( NExBuf );                                                       \
   }                                                                          \
 }
#else   /* Define this macros to result in nothing. */
#define DPT0( str1, VBlvl, str2 )
#define DPT1( str1, VBlvl, str2, Arg5 )
#define DPT2( str1, VBlvl, str2, Arg5, Arg6 )
#define DPT3( str1, VBlvl, str2, Arg5, Arg6, Arg7 )
#define DPT4( str1, VBlvl, str2, Arg5, Arg6, Arg7, Arg8 )
#define DPT5( str1, VBlvl, str2, Arg5, Arg6, Arg7, Arg8, Arg9 )
#define DPT6( str1, VBlvl, str2, Arg5, Arg6, Arg7, Arg8, Arg9, Arg10 )
#define CDPT0( str1, VBlvl, str2 )
#define CDPT1( str1, VBlvl, str2, Arg5 )
#define CDPT2( str1, VBlvl, str2, Arg5, Arg6 )
#define CDPT3( str1, VBlvl, str2, Arg5, Arg6, Arg7 )
#define CDPT4( str1, VBlvl, str2, Arg5, Arg6, Arg7, Arg8 )
#define CDPT5( str1, VBlvl, str2, Arg5, Arg6, Arg7, Arg8, Arg9 )
#define CDPT6( str1, VBlvl, str2, Arg5, Arg6, Arg7, Arg8, Arg9, Arg10 )
#define PRINT_DETAILS_OF_RDOp( OpNum, PCOp )
#define PRINT_DETAILS_OF_WROp( OpNum, PCOp )

#endif /* NExprDbgLvl >= VV_I0 */

#include "exp_datetime.h"


#include "udis86.h"

#include <sstream>
#include <iostream>
using namespace llvm;



// Can we have null tuple pointers (no, if we're on R2.5)
#define NULL_TUPLE_PTRS 0

//
// Compute the dominator tree
//
void PCodeCfg::computeDomTree()
{
  CollIndex i;

  // First we clear bit vector that tracks available ops.
  //
  // NOTE: We are using the onesVector to hold the "Dominator Tree" info.
  // There is a onesVector for each PCBlock.  For any particular PCBlock X,
  // if the bit corresponding to PCBlock Y is ON (in block X's onesVector),
  // then it means that PCBlock Y "dominates" block X.  That is, *all* paths
  // in the flow graph (PCodeCfg) from the entry block to block X are
  // guaranteed to go through block Y.  
  //
  FOREACH_BLOCK_REV_DFO( PCBlk, firstPCInst, lastPCInst, indx ) {
    PCBlk->onesVector.clear();
  } ENDFE_BLOCK_REV_DFO

  FOREACH_BLOCK_REV_DFO( PCBlk, firstPCInst, lastPCInst, indx ) {

    // IN = INTERSECT(PREDS)
    for (i=0; i < PCBlk->getPreds().entries(); i++) {
      if (i == 0)
        PCBlk->onesVector = PCBlk->getPreds()[i]->onesVector;
      else
        PCBlk->onesVector.intersectSet(PCBlk->getPreds()[i]->onesVector);
    }

    // Every PCode block dominates itself (immediate dominator)
    PCBlk->onesVector += PCBlk->getBlockNum();

  } ENDFE_BLOCK_REV_DFO
}

#if 1 /* NA_LINUX_LLVMJIT */
//
// NatExprName:  routine used by debug and instrumentation code to
// return a PCode Instruction name given its opcode.
//
// NOTE: This routine handles only the opcodes for PCODE instructions
//       that are translatable to native code.
//
// NOTE: In the future, as more PCODE instructions are made translatable
//       to native code, the routine should have lines of code added
//       to know about the new PCODE instructions.
//
#if NExprDbgLvl > VV_NO
const char * NatExprName( Int32 opc )
{
   switch ( opc )
   {
    case PCIT::OPDATA_MPTR32_IBIN32S:  {return "OPDATA_MPTR32_IBIN32S" ; break;}
    case PCIT::OPDATA_MBIN16U_IBIN32S: {return "OPDATA_MBIN16U_IBIN32S"; break;}
    case PCIT::MOVE_MBIN32S:             {return "MOVE_MBIN32S"        ; break;}
    case PCIT::MOVE_MBIN32S_IBIN32S:     {return "MOVE_MBIN32S_IBIN32S"; break;}
    case PCIT::MOVE_MBIN8_MBIN8_IBIN32S: {return "MOVE_MBIN8_MBIN8_IBIN32S"; break;}
    case PCIT::MOVE_MBIN32U_MBIN16U:     {return "MOVE_MBIN32U_MBIN16U"; break;}
    case PCIT::MOVE_MBIN32S_MBIN16U:     {return "MOVE_MBIN32S_MBIN16U"; break;}
    case PCIT::MOVE_MBIN64S_MBIN16U:     {return "MOVE_MBIN64S_MBIN16U"; break;}
    case PCIT::MOVE_MBIN32U_MBIN16S:     {return "MOVE_MBIN32U_MBIN16S"; break;}
    case PCIT::MOVE_MBIN32S_MBIN16S:     {return "MOVE_MBIN32S_MBIN16S"; break;}
    case PCIT::MOVE_MBIN64S_MBIN16S:     {return "MOVE_MBIN64S_MBIN16S"; break;}
    case PCIT::MOVE_MBIN64S_MBIN32U:     {return "MOVE_MBIN64S_MBIN32U"; break;}
    case PCIT::MOVE_MBIN64S_MBIN32S:     {return "MOVE_MBIN64S_MBIN32S"; break;}
    case PCIT::EQ_MBIN32S_MBIN16S_MBIN16S: {return "EQ_MBIN32S_MBIN16S_MBIN16S"; break;}
    case PCIT::EQ_MBIN32S_MBIN16S_MBIN32S: {return "EQ_MBIN32S_MBIN16S_MBIN32S"; break;}
    case PCIT::EQ_MBIN32S_MBIN32S_MBIN32S: {return "EQ_MBIN32S_MBIN32S_MBIN32S"; break;}
    case PCIT::EQ_MBIN32S_MBIN16U_MBIN16U: {return "EQ_MBIN32S_MBIN16U_MBIN16U"; break;}
    case PCIT::EQ_MBIN32S_MBIN16U_MBIN32U: {return "EQ_MBIN32S_MBIN16U_MBIN32U"; break;}
    case PCIT::EQ_MBIN32S_MBIN32U_MBIN32U: {return "EQ_MBIN32S_MBIN32U_MBIN32U"; break;}
    case PCIT::EQ_MBIN32S_MBIN16S_MBIN32U: {return "EQ_MBIN32S_MBIN16S_MBIN32U"; break;}
    case PCIT::LT_MBIN32S_MBIN16S_MBIN16S: {return "LT_MBIN32S_MBIN16S_MBIN16S"; break;}
    case PCIT::LT_MBIN32S_MBIN16S_MBIN32S: {return "LT_MBIN32S_MBIN16S_MBIN32S"; break;}
    case PCIT::LT_MBIN32S_MBIN32S_MBIN32S: {return "LT_MBIN32S_MBIN32S_MBIN32S"; break;}
    case PCIT::LT_MBIN32S_MBIN16U_MBIN16U: {return "LT_MBIN32S_MBIN16U_MBIN16U"; break;}
    case PCIT::LT_MBIN32S_MBIN16U_MBIN32U: {return "LT_MBIN32S_MBIN16U_MBIN32U"; break;}
    case PCIT::LT_MBIN32S_MBIN32U_MBIN32U: {return "LT_MBIN32S_MBIN32U_MBIN32U"; break;}
    case PCIT::GT_MBIN32S_MBIN16S_MBIN16S: {return "GT_MBIN32S_MBIN16S_MBIN16S"; break;}
    case PCIT::GT_MBIN32S_MBIN16S_MBIN32S: {return "GT_MBIN32S_MBIN16S_MBIN32S"; break;}
    case PCIT::GT_MBIN32S_MBIN32S_MBIN32S: {return "GT_MBIN32S_MBIN32S_MBIN32S"; break;}
    case PCIT::GT_MBIN32S_MBIN16U_MBIN16U: {return "GT_MBIN32S_MBIN16U_MBIN16U"; break;}
    case PCIT::GT_MBIN32S_MBIN16U_MBIN32U: {return "GT_MBIN32S_MBIN16U_MBIN32U"; break;}
    case PCIT::GT_MBIN32S_MBIN32U_MBIN32U: {return "GT_MBIN32S_MBIN32U_MBIN32U"; break;}
    case PCIT::SUM_MBIN32S_MBIN32S:        {return "SUM_MBIN32S_MBIN32S"; break;}
    case PCIT::SUM_MBIN64S_MBIN64S:        {return "SUM_MBIN64S_MBIN64S"; break;}
    case PCIT::MINMAX_MBIN8_MBIN8_MBIN32S_IBIN32S:
      {return "MINMAX_MBIN8_MBIN8_MBIN32S_IBIN32S"; break;}
    case PCIT::BRANCH:                     {return "BRANCH"; break;}
    case PCIT::CLAUSE_EVAL:                {return "CLAUSE_EVAL"; break;}
    case PCIT::FILL_MEM_BYTES:             {return "FILL_MEM_BYTES"; break;}
    case PCIT::MOVE_MBIN16U_IBIN16U:       {return "MOVE_MBIN16U_IBIN16U"; break;}
    case PCIT::LE_MBIN32S_MBIN16S_MBIN16S: {return "LE_MBIN32S_MBIN16S_MBIN16S"; break;}
    case PCIT::LE_MBIN32S_MBIN16S_MBIN32S: {return "LE_MBIN32S_MBIN16S_MBIN32S"; break;}
    case PCIT::LE_MBIN32S_MBIN32S_MBIN32S: {return "LE_MBIN32S_MBIN32S_MBIN32S"; break;}
    case PCIT::LE_MBIN32S_MBIN16U_MBIN16U: {return "LE_MBIN32S_MBIN16U_MBIN16U"; break;}
    case PCIT::LE_MBIN32S_MBIN16U_MBIN32U: {return "LE_MBIN32S_MBIN16U_MBIN32U"; break;}
    case PCIT::LE_MBIN32S_MBIN32U_MBIN32U: {return "LE_MBIN32S_MBIN32U_MBIN32U"; break;}
    case PCIT::GE_MBIN32S_MBIN16S_MBIN16S: {return "GE_MBIN32S_MBIN16S_MBIN16S"; break;}
    case PCIT::GE_MBIN32S_MBIN16S_MBIN32S: {return "GE_MBIN32S_MBIN16S_MBIN32S"; break;}
    case PCIT::GE_MBIN32S_MBIN32S_MBIN32S: {return "GE_MBIN32S_MBIN32S_MBIN32S"; break;}
    case PCIT::GE_MBIN32S_MBIN16U_MBIN16U: {return "GE_MBIN32S_MBIN16U_MBIN16U"; break;}
    case PCIT::GE_MBIN32S_MBIN16U_MBIN32U: {return "GE_MBIN32S_MBIN16U_MBIN32U"; break;}
    case PCIT::GE_MBIN32S_MBIN32U_MBIN32U: {return "GE_MBIN32S_MBIN32U_MBIN32U"; break;}
    case PCIT::EQ_MBIN32S_MBIN16U_MBIN16S: {return "EQ_MBIN32S_MBIN16U_MBIN16S"; break;}
    case PCIT::EQ_MBIN32S_MBIN64S_MBIN64S: {return "EQ_MBIN32S_MBIN64S_MBIN64S"; break;}
    case PCIT::LT_MBIN32S_MBIN64S_MBIN64S: {return "LT_MBIN32S_MBIN64S_MBIN64S"; break;}
    case PCIT::LE_MBIN32S_MBIN64S_MBIN64S: {return "LE_MBIN32S_MBIN64S_MBIN64S"; break;}
    case PCIT::GT_MBIN32S_MBIN64S_MBIN64S: {return "GT_MBIN32S_MBIN64S_MBIN64S"; break;}
    case PCIT::GE_MBIN32S_MBIN64S_MBIN64S: {return "GE_MBIN32S_MBIN64S_MBIN64S"; break;}
    case PCIT::BRANCH_AND:                 {return "BRANCH_AND"; break;}
    case PCIT::BRANCH_OR:                  {return "BRANCH_OR"; break;}
    case PCIT::MOVE_MBIN8_MBIN8:           {return "MOVE_MBIN8_MBIN8"; break;}
    case PCIT::MOVE_MBIN16U_MBIN16U:       {return "MOVE_MBIN16U_MBIN16U"; break;}
    case PCIT::MOVE_MBIN32U_MBIN32U:       {return "MOVE_MBIN32U_MBIN32U"; break;}
    case PCIT::MOVE_MBIN64S_MBIN64S:       {return "MOVE_MBIN64S_MBIN64S"; break;}
    case PCIT::OPDATA_MPTR32_IBIN32S_IBIN32S: {return "OPDATA_MPTR32_IBIN32S_IBIN32S"; break;}
    case PCIT::OPDATA_MATTR5_IBIN32S:      {return "OPDATA_MATTR5_IBIN32S"; break;}
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_IATTR3_IBIN32S:
      {return "NOT_NULL_BRANCH_MBIN32S_MBIN32S_IATTR3_IBIN32S"; break;}
    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S:
      {return "NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S"; break;}
    case PCIT::NULL_TEST_MBIN32S_MATTR5_IBIN32S_IBIN32S:
      {return "NULL_TEST_MBIN32S_MATTR5_IBIN32S_IBIN32S"; break;}
    case PCIT::RETURN:             {return "RETURN"; break;}
    case PCIT::RETURN_IBIN32S:     {return "RETURN_IBIN32S"; break;}
    case PCIT::MOVE_MATTR5_MATTR5: {return "MOVE_MATTR5_MATTR5"; break;}
    case PCIT::MOVE_MBIN16U_MBIN8: {return "MOVE_MBIN16U_MBIN8"; break;}
    case PCIT::NNB_MATTR3_IBIN32S: {return "NNB_MATTR3_IBIN32S"; break;}
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
         {return "NOT_NULL_BRANCH_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S"; break;}
    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
         {return "NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S"; break;}
    case PCIT::NNB_SPECIAL_NULLS_MBIN32S_MATTR3_MATTR3_IBIN32S_IBIN32S:
         {return "NNB_SPECIAL_NULLS_MBIN32S_MATTR3_MATTR3_IBIN32S_IBIN32S"; break;}
    case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN32S_MBIN32S:
         {return "SUM_MATTR3_MATTR3_IBIN32S_MBIN32S_MBIN32S"; break;}
    case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN64S:
         {return "SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN64S"; break;}
    case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN32S:
         {return "SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN32S"; break;}
    case PCIT::ADD_MBIN32S_MBIN16S_MBIN16S: {return "ADD_MBIN32S_MBIN16S_MBIN16S"; break;}
    case PCIT::ADD_MBIN16S_MBIN16S_MBIN16S: {return "ADD_MBIN16S_MBIN16S_MBIN16S"; break;}
    case PCIT::ADD_MBIN32S_MBIN32S_MBIN32S: {return "ADD_MBIN32S_MBIN32S_MBIN32S"; break;}
    case PCIT::ADD_MBIN64S_MBIN64S_MBIN64S: {return "ADD_MBIN64S_MBIN64S_MBIN64S"; break;}
    case PCIT::ADD_MBIN32S_MBIN16S_MBIN32S: {return "ADD_MBIN32S_MBIN16S_MBIN32S"; break;}
    case PCIT::ADD_MBIN64S_MBIN32S_MBIN64S: {return "ADD_MBIN64S_MBIN32S_MBIN64S"; break;}
    case PCIT::SUB_MBIN16S_MBIN16S_MBIN16S: {return "SUB_MBIN16S_MBIN16S_MBIN16S"; break;}
    case PCIT::SUB_MBIN32S_MBIN32S_MBIN32S: {return "SUB_MBIN32S_MBIN32S_MBIN32S"; break;}
    case PCIT::SUB_MBIN64S_MBIN64S_MBIN64S: {return "SUB_MBIN64S_MBIN64S_MBIN64S"; break;}
    case PCIT::SUB_MBIN32S_MBIN16S_MBIN16S: {return "SUB_MBIN32S_MBIN16S_MBIN16S"; break;}
    case PCIT::SUB_MBIN32S_MBIN16S_MBIN32S: {return "SUB_MBIN32S_MBIN16S_MBIN32S"; break;}
    case PCIT::SUB_MBIN32S_MBIN32S_MBIN16S: {return "SUB_MBIN32S_MBIN32S_MBIN16S"; break;}
    case PCIT::MUL_MBIN32S_MBIN32S_MBIN32S: {return "MUL_MBIN32S_MBIN32S_MBIN32S"; break;}
    case PCIT::MUL_MBIN32S_MBIN16S_MBIN16S: {return "MUL_MBIN32S_MBIN16S_MBIN16S"; break;}
    case PCIT::MUL_MBIN32S_MBIN16S_MBIN32S: {return "MUL_MBIN32S_MBIN16S_MBIN32S"; break;}
    case PCIT::MUL_MBIN16S_MBIN16S_MBIN16S: {return "MUL_MBIN16S_MBIN16S_MBIN16S"; break;}
    case PCIT::MUL_MBIN64S_MBIN64S_MBIN64S: {return "MUL_MBIN64S_MBIN64S_MBIN64S"; break;}
    case PCIT::MUL_MBIN64S_MBIN16S_MBIN32S: {return "MUL_MBIN64S_MBIN16S_MBIN32S"; break;}
    case PCIT::MUL_MBIN64S_MBIN32S_MBIN32S: {return "MUL_MBIN64S_MBIN32S_MBIN32S"; break;}
    case PCIT::FILL_MEM_BYTES_VARIABLE:     {return "FILL_MEM_BYTES_VARIABLE"    ; break;}
    case PCIT::EQ_MBIN32S_MASCII_MASCII:    {return "EQ_MBIN32S_MASCII_MASCII"   ; break;}
    case PCIT::NE_MBIN32S_MASCII_MASCII:    {return "NE_MBIN32S_MASCII_MASCII"   ; break;}
    case PCIT::LT_MBIN32S_MASCII_MASCII:    {return "LT_MBIN32S_MASCII_MASCII"   ; break;}
    case PCIT::GT_MBIN32S_MASCII_MASCII:    {return "GT_MBIN32S_MASCII_MASCII"   ; break;}
    case PCIT::LE_MBIN32S_MASCII_MASCII:    {return "LE_MBIN32S_MASCII_MASCII"   ; break;}
    case PCIT::GE_MBIN32S_MASCII_MASCII:    {return "GE_MBIN32S_MASCII_MASCII"   ; break;}
    default: 
      return "Opcode UNSUPPORTED by NatExprName()" ;
   }
}
#endif /* #if NExprDbgLvl >= VV_I1 */

//
// inTempsList() - Routine used to determine if the specified Temp operand
// is already in the Temps List.
//
NABoolean PCodeCfg::inTempsList( Int32 Offset, PCIT::AddressingMode OprTyp )
{
   for (  Int32 tli = 0 ; tli < NExTempsList_->entries() ; tli++ )
   {
      NExTempListEntry * tliEntry = (*NExTempsList_)[tli] ;

      if ( ( Offset == tliEntry->getOffset() ) && ( OprTyp == tliEntry->getOprTyp() ) )
         return TRUE;
   }
   return FALSE;
}

//
// addToTempsList() - Routine used to add the specified Temp operand
// to the Temps List.
//
void PCodeCfg::addToTempsList( PCodeOperand       * PCOp
                             , PCIT::AddressingMode OprTyp
                             , Int32                Offset
                             , Int32                Num
                             )
{
#if NExprDbgLvl > VV_NO
   char  NExBuf[500];
#endif
   DPT4( "VVA2T0: ", VV_XD, "In addToTempsList( %p, OprTyp = %d, Offset=%d, Num=%d)\n",
                                               PCOp, (Int32) OprTyp, Offset, Num ) ;
   CollIndex i = PCOp->getBvIndex();
   PCodeOperand* operand = this->getMap()->getFirstValue(&i);

#if NExprDbgLvl >= VV_BD
   PRINT_DETAILS_OF_WROp( 0, PCOp );
   DPT2( "VVA2T3: ", VV_XD, "In addToTempsList(): operand=%p, PCOp=%p\n",
                                                  operand,    PCOp );

   if (operand != NULL && operand != PCOp )
      PRINT_DETAILS_OF_WROp( 0, operand );
#endif

   if (operand != NULL && operand != PCOp )
   {
      PCOp = operand;
      OprTyp = operand->getType();
      // We assume that operand's offset and num values are the same as PCOp's values.
   }

   if ( inTempsList( Offset, OprTyp ) )
      return; // We will resuse the existing allocated space
   
   DPT1( "VVA2T4: ", VV_XD, "At start, NExTempsList_->entries() = %d\n", NExTempsList_->entries() );

   NExTempListEntry * newEntry = new(heap_) NExTempListEntry();

   newEntry->setPCOp( PCOp )          ;
   newEntry->setOprTyp( OprTyp )      ;
   newEntry->setOffset( Offset )      ;
   newEntry->setNum( Num )            ;

   DPT0( "VVA2T9: ", VV_XD, "Inserting into TempsList\n");
   NExTempsList_->insert( newEntry );
}

//
// Return TRUE if native instructions can be generated for this pcode graph.
//
NABoolean PCodeCfg::canGenerateNativeExpr()
{
#if NExprDbgLvl >= VV_NO
  char  NExBuf[500];
#endif

#if NExprDbgLvl >= VV_I2
  NABoolean RTNV = TRUE ; // Return TRUE for now.
  Int32     NumBad = 0 ;
  Int32     BadOpc = 0 ;
#endif

  FOREACH_BLOCK_REV_DFO(PCBlk, firstPCInst, lastPCInst, indx) {
    FOREACH_INST_IN_BLOCK(PCBlk, PCInst) {

//DPT1( "VVYN00: ", VV_I3, "In canGEN loop: Found opcd = %d \n", (int) PCInst->getOpcode() );
      Int32 opc = PCInst->getOpcode() ;
      switch ( opc )
      {

        // PCode Instructions with 0 Read Operands AND 0 Write Operands

        case PCIT::BRANCH:
        case PCIT::RETURN:
        case PCIT::RETURN_IBIN32S:
        {
          DPT2( "VVY001: ", VV_I3, "CAN    GENERATE for PCODE opcode %3d  %s\n",
                                                      opc, NatExprName( opc ) );
          break;
        }


        // PCode Instructions with 1 Read Operand AND 0 Write Operands

        case PCIT::NNB_MATTR3_IBIN32S:
        case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_IATTR3_IBIN32S:
        case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S:
        {
           PCodeOperand* read = PCInst->getROps()[0];
           if ( read->isTemp() &&  ! inTempsList( read->getOffset(), read->getType() ) )
           {
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
              DPT2( "VVN030: ", VV_I3, "Cannot GENERATE for PCODE opcode "
                                       "%3d  %s - Reads undefined temp\n",
                                        opc, NatExprName( opc ) );
              RTNV = FALSE;
              NumBad ++ ; BadOpc = opc;
              break;
 }
#endif
              return FALSE;
           }
           //Otherwise, instruction is OK

           DPT2("VVY001: ", VV_I3, "CAN    GENERATE for PCODE opcode %3d  %s\n",
                                                      opc, NatExprName( opc ) );
           break;
        }
        case PCIT::OPDATA_MPTR32_IBIN32S:
        case PCIT::OPDATA_MBIN16U_IBIN32S:
        case PCIT::OPDATA_MPTR32_IBIN32S_IBIN32S:
        case PCIT::OPDATA_MATTR5_IBIN32S:
        {
           if ( PCInst->getWOps().entries() )
           {
              NABoolean canHandle = TRUE;
              OPLIST opList = PCInst->getWOps() ;
              for (Int32 iii = 0; iii < opList.entries(); iii++ )
              {
                  PCodeOperand *PCOp = opList[iii];
                  if ( PCOp->getStackIndex() == 2 ) // DON'T YET KNOW HOW TO HANDLE THIS with LLVM!
                  {
                     canHandle = FALSE ;

#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
                     DPT1( "VVN001: ", VV_I3, "Cannot GENERATE for PCODE "
                                       "opcode %3d - Writes To Temp Var\n",
                                                     PCInst->getOpcode() );
                     DPT6( "VVN001a: ", VV_I3, "Details of Bad Temp Var:\n"
                           "stackIndex_ = %d, offset_ = %d, len_=%d, "
                           "operandType_=%d, nullBitIdx=%d, CJV=%p\n",
                           PCOp->getStackIndex(), PCOp->getOffset(),
                           PCOp->getLen(), PCOp->getType(),
                           PCOp->getNullBitIndex(), PCOp->getCurrJitVal() );

                     RTNV = FALSE;
                     NumBad ++ ; BadOpc = opc;
                     break;
 }
#endif
                     return FALSE;
                  }
              }
              if ( canHandle == FALSE )
                 break;
           }
           else // May have Read Operands
           {
              OPLIST opList = PCInst->getROps() ;
              for (Int32 iii = 0; iii < opList.entries(); iii++ )
              {
                 PCodeOperand *PCOp = opList[iii];
                 if ( PCOp->isTemp() &&  ! inTempsList( PCOp->getOffset(), PCOp->getType() ) )
                 {
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
                    DPT2( "VVN030: ", VV_I3, "Cannot GENERATE for PCODE "
                                    "opcode %3d  %s - Reads undefined temp\n",
                                            opc, NatExprName( opc ) );
                    RTNV = FALSE;
                    NumBad ++ ; BadOpc = opc;
                    break;
 }
#endif
                    return FALSE;
                 }
              }
           }

          DPT2("VVY001: ", VV_I3, "CAN    GENERATE for PCODE opcode %3d  %s\n",
                                                     opc, NatExprName( opc ) );
          break;
        }


        // PCode Instructions with 2 Read Operands AND 1 Write Operand

        case PCIT::NNB_SPECIAL_NULLS_MBIN32S_MATTR3_MATTR3_IBIN32S_IBIN32S:
        case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
        case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
        case PCIT::EQ_MBIN32S_MASCII_MASCII:
        case PCIT::NE_MBIN32S_MASCII_MASCII:
        case PCIT::LT_MBIN32S_MASCII_MASCII:
        case PCIT::GT_MBIN32S_MASCII_MASCII:
        case PCIT::LE_MBIN32S_MASCII_MASCII:
        case PCIT::GE_MBIN32S_MASCII_MASCII:
        case PCIT::EQ_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::EQ_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::EQ_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::EQ_MBIN32S_MBIN16U_MBIN16U:
        case PCIT::EQ_MBIN32S_MBIN16U_MBIN32U:
        case PCIT::EQ_MBIN32S_MBIN32U_MBIN32U:
        case PCIT::EQ_MBIN32S_MBIN16U_MBIN16S:
        case PCIT::EQ_MBIN32S_MBIN64S_MBIN64S:
        case PCIT::LT_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::LT_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::LT_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::LT_MBIN32S_MBIN64S_MBIN64S:
        case PCIT::LT_MBIN32S_MBIN16U_MBIN16U:
        case PCIT::LT_MBIN32S_MBIN16U_MBIN32U:
        case PCIT::LT_MBIN32S_MBIN32U_MBIN32U:
        case PCIT::LE_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::LE_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::LE_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::LE_MBIN32S_MBIN16U_MBIN16U:
        case PCIT::LE_MBIN32S_MBIN16U_MBIN32U:
        case PCIT::LE_MBIN32S_MBIN32U_MBIN32U:
        case PCIT::LE_MBIN32S_MBIN64S_MBIN64S:
        case PCIT::GT_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::GT_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::GT_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::GT_MBIN32S_MBIN16U_MBIN16U:
        case PCIT::GT_MBIN32S_MBIN16U_MBIN32U:
        case PCIT::GT_MBIN32S_MBIN32U_MBIN32U:
        case PCIT::GT_MBIN32S_MBIN64S_MBIN64S:
        case PCIT::GE_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::GE_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::GE_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::GE_MBIN32S_MBIN16U_MBIN16U:
        case PCIT::GE_MBIN32S_MBIN16U_MBIN32U:
        case PCIT::GE_MBIN32S_MBIN32U_MBIN32U:
        case PCIT::GE_MBIN32S_MBIN64S_MBIN64S:
        case PCIT::MINMAX_MBIN8_MBIN8_MBIN32S_IBIN32S:
        case PCIT::SUM_MBIN32S_MBIN32S:           // (Note: not used by regression tests! ?)
        case PCIT::SUM_MBIN64S_MBIN64S:
        case PCIT::ADD_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::ADD_MBIN16S_MBIN16S_MBIN16S:
        case PCIT::ADD_MBIN32S_MBIN32S_MBIN32S:
/////// case PCIT::ADD_MBIN64S_MBIN64S_MBIN64S:   // Cannot support yet: no overflow support
        case PCIT::ADD_MBIN32S_MBIN16S_MBIN32S:
/////// case PCIT::ADD_MBIN64S_MBIN32S_MBIN64S:   // Cannot support yet: no overflow support
        case PCIT::SUB_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::SUB_MBIN16S_MBIN16S_MBIN16S:
        case PCIT::SUB_MBIN32S_MBIN32S_MBIN32S:
/////// case PCIT::SUB_MBIN64S_MBIN64S_MBIN64S:   // Cannot support yet: no overflow support
        case PCIT::SUB_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::SUB_MBIN32S_MBIN32S_MBIN16S:
/////// case PCIT::MUL_MBIN64S_MBIN64S_MBIN64S:   // Cannot support yet: no overflow support
/////// case PCIT::MUL_MBIN64S_MBIN32S_MBIN32S:   // Cannot support yet: no overflow support
/////// case PCIT::MUL_MBIN64S_MBIN16S_MBIN32S:   // Cannot support yet: no overflow support
        case PCIT::MUL_MBIN16S_MBIN16S_MBIN16S:
        case PCIT::MUL_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::MUL_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::MUL_MBIN32S_MBIN16S_MBIN32S:
        {
           PCodeOperand* read = PCInst->getROps()[1];
           if ( read->isTemp() &&  ! inTempsList( read->getOffset(), read->getType() ) )
           {
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
              DPT2( "VVN030: ", VV_I3, "Cannot GENERATE for PCODE opcode "
                                       "%3d  %s - Reads undefined temp\n",
                                        opc, NatExprName( opc ) );
              RTNV = FALSE;
              NumBad ++ ; BadOpc = opc;
              break;
 }
#endif
              return FALSE;
           }
        }
        /* FALLTHROUGH */


        // PCode Instructions with 1 Read Operand AND 1 Write Operand

        case PCIT::BRANCH_OR:
        case PCIT::BRANCH_AND:
        case PCIT::MOVE_MBIN16U_MBIN8:
        case PCIT::MOVE_MBIN32U_MBIN16U:
        case PCIT::MOVE_MBIN32S_MBIN16U:
        case PCIT::MOVE_MBIN64S_MBIN16U:
        case PCIT::MOVE_MBIN64S_MBIN32U:
        case PCIT::MOVE_MBIN32U_MBIN16S:
        case PCIT::MOVE_MBIN32S_MBIN16S:
        case PCIT::MOVE_MBIN64S_MBIN16S:
        case PCIT::MOVE_MBIN64S_MBIN32S:
        case PCIT::MOVE_MBIN16U_MBIN16U:
        case PCIT::MOVE_MBIN8_MBIN8_IBIN32S:
        case PCIT::MOVE_MBIN32U_MBIN32U:
        case PCIT::MOVE_MBIN64S_MBIN64S:
        {
           //
           // NOTE: PCIT::MOVE_MBIN16U_MBIN8, in particular, has been observed
           // being used to reference the last part of a string that was put
           // into Temp space (i.e. stackIndex_ == 2) by a preceding PCODE
           // instruction.  When this happens, PCodeOperand::getJitValue()
           // comes to the conclusion that the reference to the last part of
           // the string is a reference to a new (and hence *undefined*) 
           // temporary variable and does an assert(FALSE) since such a 
           // reference is not supposed to happen.  Perhaps someday we will
           // be able to figure out how to get getJitValue() to understand
           // that the reference is OK, but for now, we just ensure this does
           // not happen by preventing reads of Temp variables that appear to
           // be undefined.   Since we don't really know all the possible 
           // situations that could lead to apparently undefined temp variables,
           // we test *all* reads of Temp variables by *any* PCODE instructions
           // that take a Read Operand and, if it looks like it is an
           // undefined Temp variable, we prevent generating a Native Expr.
           //
           PCodeOperand* read = PCInst->getROps()[0];
           if ( read->isTemp() &&  ! inTempsList( read->getOffset(), read->getType() ) )
           {
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
              DPT2( "VVN030: ", VV_I3, "Cannot GENERATE for PCODE opcode "
                                       "%3d  %s - Reads undefined temp\n",
                                        opc, NatExprName( opc ) );
              RTNV = FALSE;
              NumBad ++ ; BadOpc = opc;
              break;
 }
#endif
              return FALSE;
           }
        }
        /* FALLTHROUGH */


        // PCode Instructions with 1 Write Operand

        case PCIT::MOVE_MBIN16U_IBIN16U:
        case PCIT::MOVE_MBIN32S_IBIN32S:
        {
           PCodeOperand* write = PCInst->getWOps()[0];
           if ( write->isTemp() )
           {
              // Some regression tests fail if these opcodes write to Temp variables!
              // Perhaps someday this restriction will be removed?
              if ( opc == PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S )
              {
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
                 DPT2( "VVN001: ", VV_I3, "Cannot GENERATE for PCODE opcode "
                                          "%3d  %s - Writes To Temp Var\n",
                                           opc, NatExprName( opc ) );
                 RTNV = FALSE;
                 NumBad ++ ; BadOpc = opc;
                 break;
 }
#endif
                 return FALSE;
              }

              addToTempsList( write , write->getType() , write->getOffset() );
           }
           DPT2("VVY001: ", VV_I3, "CAN    GENERATE for PCODE opcode %3d  %s\n",
                                                      opc, NatExprName( opc ) );
           break;
        }

        case PCIT::MOVE_MBIN8_MBIN8:
        {
           PCodeOperand * src1 = PCInst->getROps()[0];
           if ( src1->isTemp() &&  ! inTempsList( src1->getOffset(), src1->getType() ) )
           {
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
              DPT2( "VVN030: ", VV_I3, "Cannot GENERATE for PCODE opcode "
                                       "%3d  %s - Reads undefined temp\n",
                                        opc, NatExprName( opc ) );
              RTNV = FALSE;
              NumBad ++ ; BadOpc = opc;
              break;
 }
#endif
              return FALSE;
           }

           if ( src1->isConst() )
           {
              if ( src1->getType() == PCIT::MPTR32 )
              {
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
                 DPT0( "VVN002: ", VV_I3, "Cannot GENERATE for PCODE "
                                          "opcode 200 - Const MPTR32 type\n");
                 RTNV = FALSE;
                 NumBad ++ ; BadOpc = opc;
 }
#endif
                 return FALSE;
              }
              else
              {
                 DPT3("VVY003: ", VV_I3, "CAN    GENERATE for PCODE opcode "
                                         "%3d  %s  type=%d\n",
                                          opc, NatExprName( opc ),
                                          (int)src1->getType() );
                 PCodeOperand* write = PCInst->getWOps()[0];
                 if ( write->isTemp() ) addToTempsList( write
                                                      , write->getType()
                                                      , write->getOffset()
                                                      );
              }
           }
           break;
        }

        case PCIT::CLAUSE_EVAL:
        {
          // on 64-bit the clause pointer is in code[1] and code[2]
          ex_clause* clause = (ex_clause*)*(Long*)&(PCInst->code[1]);
          NABoolean processNulls = PCInst->code[1 + PCODEBINARIES_PER_PTR];

          // Only certain clause eval instructions will be supported for now.
          switch (clause->getClassID()) {
            case ex_clause::AGGR_MIN_MAX_ID:
            {
              /* ZZZZ  CAN WE LIFT THIS RESTRICTION ??? */
              // Because of the "assert(!opDataNulls[0]->forAlignedFormat());"
              // line in the CLAUSE_EVAL handling within layoutNativeCode(),
              // AND because that assertion is apparently no longer valid
              // (as of 07/01/2013), we disallow the ex_clause::AGGR_MIN_MAX_ID
              // clause for now -- until we can add code in the CLAUSE_EVAL
              // handling to deal with AlignedFormat in the null indicator
              // for the aggregate.

#if 1  /* ZZZZ  CAN WE LIFT THIS RESTRICTION ??? */
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
              DPT1( "VVN005: ", VV_I3, "Cannot GENERATE for PCODE opcode %d -"
                                       " AGGR_MIN_MAX_ID disallowed for now\n",
                                                         PCInst->getOpcode() );
              RTNV = FALSE;
              NumBad ++ ; BadOpc = opc;
              break;
 }
#endif
              return FALSE;

#else /* the old code: allow AGGR_MIN_MAX_ID when we know how to do so: */

              // This clause must not call process nulls when evaluated
              if (!processNulls) {
                // Can only tolerate certain types for now.
                if (isSupportedClauseOperandType(clause->getOperand(0)))
                {
                  DPT2( "VVY004: ", VV_I3,
                        "CAN    GENERATE for PCODE opcode %3d  %s\n",
                                                    opc, NatExprName( opc ) );
                  break;
                }

              }

#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
              DPT1( "VVN005: ", VV_I3, "Cannot GENERATE for PCODE opcode %d -"
                                       " AGGR processes nulls\n",
                                                         PCInst->getOpcode() );
              RTNV = FALSE;
              NumBad ++ ; BadOpc = opc;
              break;
 }
#endif
              return FALSE;
#endif /* 1 */
            }

            case ex_clause::COMP_TYPE:
            {
              // No special nulls allowed for now.
              if (clause->isSpecialNulls())
              {
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
                DPT1( "VVN006: ", VV_I3, "Cannot GENERATE for PCODE opcode %d -"
                                         " COMP_TYPE wants Special Nulls\n",
                                                         PCInst->getOpcode() );
                RTNV = FALSE;
                NumBad ++ ; BadOpc = opc;
                break;
 }
#endif
                return FALSE;
              }

              switch (((ex_comp_clause*)clause)->getInstruction()) {
                case NE_DATETIME_DATETIME:
                case EQ_DATETIME_DATETIME:
                case LT_DATETIME_DATETIME:
                case GT_DATETIME_DATETIME:
                case LE_DATETIME_DATETIME:
                case GE_DATETIME_DATETIME:
                {
                  // Only support standard types
                  long dtCode =
                    ((ExpDatetime*)(clause->getOperand(1)))->getPrecision();

                  // The lengths of the source operands must be the same.
                  if ((clause->getOperand(1)->getLength()) !=
                      (clause->getOperand(1)->getLength()))
                  {
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
                     DPT1( "VVN007: ", VV_I3, "Cannot GENERATE for PCODE "
                           "opcode %d - COMP_TYPE Opers diff lengths\n",
                                                  PCInst->getOpcode() );
                     RTNV = FALSE;
                     NumBad ++ ; BadOpc = opc;
                     break;
 }
#endif
                     return FALSE;
                  }

                  switch (dtCode)
                  {
                    case REC_DTCODE_DATE:
                    case REC_DTCODE_TIME:
                    case REC_DTCODE_TIMESTAMP:
                      DPT2( "VVY008: ", VV_I3,
                            "CAN    GENERATE for PCODE opcode %3d  %s\n",
                                                     opc, NatExprName( opc ) );
                      break;
                    default:
                    {
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
                      DPT1( "VVN009: ", VV_I3, "Cannot GENERATE for PCODE "
                            "opcode %d - COMP_TYPE unsupported dtCode\n",
                                                   PCInst->getOpcode() );
                      RTNV = FALSE;
                      NumBad ++ ; BadOpc = opc;
                      break;
 }
#endif
                      return FALSE;
                    }
                  }
                  break;
                }

                default:
                {
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
                  DPT1( "VVN010: ", VV_I3, "Cannot GENERATE for PCODE opcode "
                                     "%d - COMP_TYPE unsupported D/T clause\n",
                                                         PCInst->getOpcode() );
                  RTNV = FALSE;
                  NumBad ++ ; BadOpc = opc;
                  break;
 }
#endif
                  return FALSE;
                }
              }
              break;
            }

            default:
            {
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
              DPT1( "VVN011: ", VV_I3, "Cannot GENERATE for PCODE opcode "
                                       "%d - COMP_TYPE unsupported ClassID\n",
                                                        PCInst->getOpcode() );
              RTNV = FALSE;
              NumBad ++ ; BadOpc = opc;
              break;
 }
#endif
              return FALSE;
            }
          }
          break;
        }

        case PCIT::FILL_MEM_BYTES:
        {
           PCodeOperand * write = PCInst->getWOps()[0];
           if ( write->isTemp() ) addToTempsList( write
                                                , write->getType()
                                                , write->getOffset()
                                                , PCInst->code[3]
                                                );
          break;
        }

        case PCIT::FILL_MEM_BYTES_VARIABLE:
        {
           PCodeOperand* write = PCInst->getWOps()[0];

          // Only support non-aligned format case.  Otherwise we have to start
          // tracking local varOffset (which we can do, but just not there yet).
          if ( write->getVoaOffset() == -1 )
          {
            DPT2("VVY012: ", VV_I3,"CAN    GENERATE for PCODE opcode %3d  %s\n",
                                                      opc, NatExprName( opc ) );

            if ( write->isTemp() ) addToTempsList( write
                                                  , write->getType()
                                                  , write->getOffset()
                                                  , PCInst->code[6]
                                                  );
            break;
          }

#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
          DPT1( "VVN013: ", VV_I3, "Cannot GENERATE for PCODE opcode "
                                   "%d - getVoaOffset() != -1\n",
                                           PCInst->getOpcode() );
          RTNV = FALSE;
          NumBad ++ ; BadOpc = opc;
          break;
 }
#endif
          return FALSE;
        }

        case PCIT::MOVE_MATTR5_MATTR5:
        {
          PCodeOperand* read = PCInst->getROps()[0];
          PCodeOperand* write = PCInst->getWOps()[0];

          if ( read->isTemp() &&  ! inTempsList( read->getOffset(), read->getType() ) )
          {
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
             DPT2( "VVN030: ", VV_I3, "Cannot GENERATE for PCODE opcode "
                                      "%3d  %s - Reads undefined temp\n",
                                       opc, NatExprName( opc ) );
             RTNV = FALSE;
             NumBad ++ ; BadOpc = opc;
             break;
 }
#endif
             return FALSE;
          }

          // Since we plan on copying everyting (length + string) in one shot,
          // make sure the vc lengths of the operands are the same.
          if (read->getVcIndicatorLen() != write->getVcIndicatorLen())
          {
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
             DPT1( "VVN014: ", VV_I3, "Cannot GENERATE for PCODE opcode "
                                      "%d - diff VcIndLen's\n",
                                      PCInst->getOpcode() );
             RTNV = FALSE;
             NumBad ++ ; BadOpc = opc;
             break;
 }
#endif
             return FALSE;
          }

          // For now, no support for temp varchars (since temp varchars will
          // likely need to come from a separate temps area.
          if (write->isTemp() || read->isTemp())
          {
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
             DPT1( "VVN015: ", VV_I3, "Cannot GENERATE for PCODE opcode "
                               "%d - write->isTemp() || read->isTemp()\n",
                                                     PCInst->getOpcode() );
             RTNV = FALSE;
             NumBad ++ ; BadOpc = opc;
             break;
 }
#endif
             return FALSE;
          }

          // Also, target max length must be greater than that of source
          if (read->getVcMaxLen() > write->getVcMaxLen())
          {
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
             DPT1( "VVN016: ", VV_I3, "Cannot GENERATE for PCODE opcode "
                                      "%d - srcLen > tgtLen\n",
                                         PCInst->getOpcode() );
             RTNV = FALSE;
             NumBad ++ ; BadOpc = opc;
             break;
 }
#endif
             return FALSE;
          }

          DPT2( "VVY017: ", VV_I3, "CAN    GENERATE for PCODE opcode %3d  %s\n",
                                                      opc, NatExprName( opc ) );
          break;
        }

        case PCIT::NULL_TEST_MBIN32S_MATTR5_IBIN32S_IBIN32S:
        {
          PCodeOperand* write = PCInst->getWOps()[0];

          // FIX: For now, just disallow the expr if write operand is not a temp
          if ( ! write->isTemp() )
          {
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
             DPT2( "VVN022: ", VV_I3, "Cannot GENERATE for PCODE opcode "
                                      "%3d  %s - Writes to non temp\n",
                                       opc, NatExprName( opc ) );
             RTNV = FALSE;
             NumBad ++ ; BadOpc = opc;
             break;
 }
#endif
             return FALSE;
          }

          // Just don't support case with nullable varchar in packed format,
          // since that requires an extra level of indirection when accessing
          // the null indicator -- not expensive/complicated to implement, but
          // rare enough to want to avoid.

          PCodeOperand* src = PCInst->getROps()[0];

          if ( src->isTemp() &&  ! inTempsList( src->getOffset(), src->getType() ) )
          {
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
             DPT2( "VVN030: ", VV_I3, "Cannot GENERATE for PCODE opcode "
                                      "%3d  %s - Reads undefined temp\n",
                                       opc, NatExprName( opc ) );
             RTNV = FALSE;
             NumBad ++ ; BadOpc = opc;
             break;
 }
#endif
             return FALSE;
          }

          if ((src->getOffset() == -1) && !src->forAlignedFormat())
          {
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
             DPT1( "VVN017: ", VV_I3, "Cannot GENERATE for PCODE opcode "
                                      "%d - srcLen > tgtLen\n",
                                         PCInst->getOpcode() );
             RTNV = FALSE;
             NumBad ++ ; BadOpc = opc;
             break;
 }
#endif
             return FALSE;
          }

          // Note: Could skip the write->isTemp() check in the next line
          // since we assert'd that above, but that assert may be removed
          // some day, so leave it for now...even though it is redundant.
          if ( write->isTemp() ) addToTempsList( write
                                               , write->getType()
                                               , write->getOffset()
                                               );
          DPT2( "VVY018: ", VV_I3,
                "CAN    GENERATE for PCODE opcode %3d  %s\n",
                                                  opc, NatExprName( opc ) );
          break;
        }

        case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN32S_MBIN32S:
        case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN64S:
        case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN32S:
        {
          // Make sure that this is nullable before we proceed
          if (!PCInst->code[7])
          {
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
             DPT1( "VVN018: ", VV_I3, "Cannot GENERATE for PCODE opcode "
                                      "%d - !PCInst->code[7] \n",
                                           PCInst->getOpcode() );
             RTNV = FALSE;
             NumBad ++ ; BadOpc = opc;
             break;
 }
#endif
             return FALSE;
          }

          PCodeOperand * src1 = PCInst->getROps()[0];
          PCodeOperand * src2 = PCInst->getROps()[1];
          PCodeOperand * src3 = PCInst->getROps()[(PCInst->getROps()).entries()-1];

          if ( ( src1->isTemp() &&  ! inTempsList( src1->getOffset(), src1->getType() ) ) ||
               ( src2->isTemp() &&  ! inTempsList( src2->getOffset(), src2->getType() ) ) ||
               ( src3->isTemp() &&  ! inTempsList( src3->getOffset(), src3->getType() ) )
             )
          {
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
             DPT2( "VVN030: ", VV_I3, "Cannot GENERATE for PCODE opcode "
                                      "%3d  %s - Reads undefined temp\n",
                                       opc, NatExprName( opc ) );
             RTNV = FALSE;
             NumBad ++ ; BadOpc = opc;
             break;
 }
#endif
             return FALSE;
          }

          DPT2( "VVY019: ", VV_I3,
                "CAN    GENERATE for PCODE opcode %3d  %s\n",
                                                  opc, NatExprName( opc ) );

          PCodeOperand * write = PCInst->getWOps()[ (PCInst->getWOps()).entries()-1 ];
          if ( write->isTemp() ) addToTempsList( write
                                               , write->getType()
                                               , write->getOffset()
                                               );
          break;
        }

        case PCIT::MOVE_MBIN32S:
        {
          PCodeOperand* read = PCInst->getROps()[0];
          if ( read->isTemp() &&  ! inTempsList( read->getOffset(), read->getType() ) )
          {
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
             DPT2( "VVN030: ", VV_I3, "Cannot GENERATE for PCODE opcode "
                                      "%3d  %s - Reads undefined temp\n",
                                       opc, NatExprName( opc ) );
             RTNV = FALSE;
             NumBad ++ ; BadOpc = opc;
             break;
 }
#endif
             return FALSE;
          }
          // Only supported if RETURN follows it (as it always should)
          if (PCInst->next && (PCInst->next->code[0] == PCIT::RETURN))
          {
             DPT2( "VVY020: ", VV_I3, "CAN    GENERATE for PCODE opcode "
                                      "%3d  %s\n", opc, NatExprName( opc ) );
             break;
          }

#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
           DPT1("VVN021: ", VV_I3, "Cannot GENERATE for PCODE opcode "
                                   "%d - not followed by RETURN\n",
                                              PCInst->getOpcode());
           RTNV = FALSE;
           NumBad ++ ; BadOpc = opc;
           break;
 }
#endif
           return FALSE;
        }
        default:
        {
#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
           DPT2( "VVN090: ", VV_I3,"Cannot GENERATE for PCODE opcode %3d  %s\n",
                                            opc, NatExprName( opc ) );
           RTNV = FALSE;
           NumBad ++ ; BadOpc = opc;
           break;
 }
#endif
           return FALSE;
        }
      }
    } ENDFE_INST_IN_BLOCK
  } ENDFE_BLOCK_REV_DFO

#if NExprDbgLvl >= VV_I2
  if ( NumBad == 1 )
  {
     DPT2( "VVN099: ", VV_I2,
           "Cannot GENERATE for PCODE opcode %3d  %s ONLY\n\n",
                                          BadOpc, NatExprName( BadOpc ) );
  }
  return RTNV;
#else
  return TRUE;
#endif
}
#endif /* NA_LINUX_LLVMJIT */

#ifdef NA_LINUX_LIBJIT
//
// Return TRUE if native instructions can be generated for this pcode graph.
//
NABoolean PCodeCfg::canGenerateNativeExpr()
{
  // . No persistent data (for now)
  // . Only certain instructions  

  FOREACH_BLOCK_REV_DFO(PCBlk, firstPCInst, lastPCInst, indx) {
    FOREACH_INST_IN_BLOCK(PCBlk, inst) {
      switch (inst->getOpcode()) {
        case PCIT::BRANCH:
        case PCIT::BRANCH_OR:
        case PCIT::BRANCH_AND:
        case PCIT::BRANCH_OR_CNT:
        case PCIT::BRANCH_AND_CNT:
          break;

        case PCIT::OR_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::AND_MBIN32S_MBIN32S_MBIN32S:
          break;

        case PCIT::NNB_MATTR3_IBIN32S:
        case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_IATTR3_IBIN32S:
        case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
        case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S:
        case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
        case PCIT::NNB_SPECIAL_NULLS_MBIN32S_MATTR3_MATTR3_IBIN32S_IBIN32S:
          break;

        case PCIT::FILL_MEM_BYTES:
          break;

        case PCIT::FILL_MEM_BYTES_VARIABLE:
        {
          // Only support non-aligned format case.  Otherwise we have to start
          // tracking local varOffset (which we can do, but just not there yet).
          if (inst->getWOps()[0]->getVoaOffset() == -1)
            break;

          return FALSE;
        }

        /* 64-bit comps (except EQ/NE) not supported in libjit. */
        //case PCIT::LT_MBIN32S_MBIN64S_MBIN64S:
        //case PCIT::LE_MBIN32S_MBIN64S_MBIN64S:
        //case PCIT::GT_MBIN32S_MBIN64S_MBIN64S:
        //case PCIT::GE_MBIN32S_MBIN64S_MBIN64S:
        case PCIT::EQ_MBIN32S_MBIN64S_MBIN64S:
        case PCIT::NE_MBIN32S_MBIN64S_MBIN64S:
          break;

        case PCIT::EQ_MBIN32S_MBIN16S_MBIN32U:
        case PCIT::LT_MBIN32S_MBIN16S_MBIN32U:
        case PCIT::GT_MBIN32S_MBIN16S_MBIN32U:
        case PCIT::LE_MBIN32S_MBIN16S_MBIN32U:
        case PCIT::GE_MBIN32S_MBIN16S_MBIN32U:
        {
          // The MBIN16S operand is casted to a 64-bit value before the comp is
          // performed.  Until we have 64-bit comparison support, we can only
          // do a little here.
          if (inst->getROps()[0]->isConst()) {
            Int64 val = getIntConstValue(inst->getROps()[0]);
            if (val < 0)
              return FALSE;

            // Positive value found that can be used in a MBIN32U comparison.
          }
          else
            return FALSE;

          break;
        }

        case PCIT::EQ_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::EQ_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::EQ_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::EQ_MBIN32S_MBIN16U_MBIN16U:
        case PCIT::EQ_MBIN32S_MBIN16U_MBIN32U:
        case PCIT::EQ_MBIN32S_MBIN32U_MBIN32U:
        case PCIT::EQ_MBIN32S_MBIN16U_MBIN16S:
          break;

        case PCIT::LT_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::LT_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::LT_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::LT_MBIN32S_MBIN16U_MBIN16U:
        case PCIT::LT_MBIN32S_MBIN16U_MBIN32U:
        case PCIT::LT_MBIN32S_MBIN32U_MBIN32U:
        case PCIT::LT_MBIN32S_MBIN16U_MBIN16S:
          break;

        case PCIT::LE_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::LE_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::LE_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::LE_MBIN32S_MBIN16U_MBIN16U:
        case PCIT::LE_MBIN32S_MBIN16U_MBIN32U:
        case PCIT::LE_MBIN32S_MBIN32U_MBIN32U:
        case PCIT::LE_MBIN32S_MBIN16U_MBIN16S:
          break;

        case PCIT::GT_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::GT_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::GT_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::GT_MBIN32S_MBIN16U_MBIN16U:
        case PCIT::GT_MBIN32S_MBIN16U_MBIN32U:
        case PCIT::GT_MBIN32S_MBIN32U_MBIN32U:
        case PCIT::GT_MBIN32S_MBIN16U_MBIN16S:
          break;

        case PCIT::GE_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::GE_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::GE_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::GE_MBIN32S_MBIN16U_MBIN16U:
        case PCIT::GE_MBIN32S_MBIN16U_MBIN32U:
        case PCIT::GE_MBIN32S_MBIN32U_MBIN32U:
        case PCIT::GE_MBIN32S_MBIN16U_MBIN16S:
          break;

        case PCIT::EQ_MBIN32S_MASCII_MASCII:
        case PCIT::NE_MBIN32S_MASCII_MASCII:
        case PCIT::LT_MBIN32S_MASCII_MASCII:
        case PCIT::GT_MBIN32S_MASCII_MASCII:
        case PCIT::LE_MBIN32S_MASCII_MASCII:
        case PCIT::GE_MBIN32S_MASCII_MASCII:
          break;

        case PCIT::COMP_MBIN32S_MASCII_MASCII_IBIN32S_IBIN32S_IBIN32S:
        case PCIT::COMP_MBIN32S_MATTR5_MATTR5_IBIN32S:
          break;

        case PCIT::SWITCH_MBIN32S_MBIN64S_MPTR32_IBIN32S_IBIN32S:
        {
          // Only in-lists supported, for now.
          if (inst->code[8] & 0x1)
            break;

          return (FALSE);
        }

        case PCIT::SWITCH_MBIN32S_MATTR5_MPTR32_IBIN32S_IBIN32S:
        {
          // Only in-lists supported, for now.
          if (inst->code[11] & 0x1)
            break;

          return (FALSE);
        }

        case PCIT::COMP_MBIN32S_MBIGS_MBIGS_IBIN32S_IBIN32S:
        case PCIT::ADD_MBIGS_MBIGS_MBIGS_IBIN32S:
        case PCIT::SUB_MBIGS_MBIGS_MBIGS_IBIN32S:
          break;

        case PCIT::ADD_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::ADD_MBIN16S_MBIN16S_MBIN16S:
        case PCIT::ADD_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::ADD_MBIN64S_MBIN64S_MBIN64S:
        case PCIT::ADD_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::ADD_MBIN64S_MBIN32S_MBIN64S:
          break;

        // case PCIT::SUB_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::SUB_MBIN16S_MBIN16S_MBIN16S:
        case PCIT::SUB_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::SUB_MBIN64S_MBIN64S_MBIN64S:
        case PCIT::SUB_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::SUB_MBIN32S_MBIN32S_MBIN16S:
          break;

        /* 64-bit multiply not supported in libjit - need to call routine. */
        /* No support to downcast result to 16-bit type for now - not needed */
        //case PCIT::MUL_MBIN64S_MBIN64S_MBIN64S:
        //case PCIT::MUL_MBIN64S_MBIN32S_MBIN32S:
        //case PCIT::MUL_MBIN64S_MBIN16S_MBIN32S:
        //case PCIT::MUL_MBIN16S_MBIN16S_MBIN16S:
        case PCIT::MUL_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::MUL_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::MUL_MBIN32S_MBIN16S_MBIN32S:
          break;

        case PCIT::ADD_MFLT64_MFLT64_MFLT64:
        case PCIT::SUB_MFLT64_MFLT64_MFLT64:
        case PCIT::MUL_MFLT64_MFLT64_MFLT64:
        case PCIT::DIV_MFLT64_MFLT64_MFLT64:
          break;

        case PCIT::REPLACE_NULL_MATTR3_MBIN32S:
        case PCIT::REPLACE_NULL_MATTR3_MBIN32U:
        case PCIT::REPLACE_NULL_MATTR3_MBIN16S:
        case PCIT::REPLACE_NULL_MATTR3_MBIN16U:
          break;

        case PCIT::RETURN_IBIN32S:
        case PCIT::RETURN:
          break;

        case PCIT::MOVE_MBIN16U_MBIN8:
        case PCIT::MOVE_MBIN32U_MBIN16U:
        case PCIT::MOVE_MBIN32S_MBIN16U:
        case PCIT::MOVE_MBIN64S_MBIN16U:
        case PCIT::MOVE_MBIN32U_MBIN16S:
        case PCIT::MOVE_MBIN32S_MBIN16S:
        case PCIT::MOVE_MBIN64S_MBIN16S:
        case PCIT::MOVE_MBIN64S_MBIN32U:
        case PCIT::MOVE_MBIN64S_MBIN32S:
          break;

        case PCIT::MOVE_MFLT64_MBIN32S:
        case PCIT::MOVE_MFLT64_MBIN16S:
          break;

        case PCIT::MOVE_BULK:
          break;

        case PCIT::MOVE_MBIN16U_MBIN16U:
        case PCIT::MOVE_MBIN8_MBIN8:
        case PCIT::MOVE_MBIN32U_MBIN32U:
        case PCIT::MOVE_MBIN64S_MBIN64S:
        case PCIT::MOVE_MBIN16U_IBIN16U:
        case PCIT::MOVE_MBIN32S_IBIN32S:
          break;

        case PCIT::MOVE_MATTR5_MATTR5:
        {
          PCodeOperand* read = inst->getROps()[0];
          PCodeOperand* write = inst->getWOps()[0];

          // Since we plan on copying everyting (length + string) in one shot,
          // make sure the vc lengths of the operands are the same.
          if (read->getVcIndicatorLen() != write->getVcIndicatorLen())
            return FALSE;

          // For now, no support for temp varchars (since temp varchars will
          // likely need to come from a separate temps area.
          if (write->isTemp() || read->isTemp())
            return FALSE;

          // Also, target max length must be greater than that of source
          if (read->getVcMaxLen() > write->getVcMaxLen())
            return FALSE;

          break;
        }

        case PCIT::NULL_TEST_MBIN32S_MATTR5_IBIN32S_IBIN32S:
        {
          // FIX: assert for now that write operand should be a temp
          assert(inst->getWOps()[0]->isTemp());

          // Just don't support case with nullable varchar in packed format,
          // since that requires an extra level of indirection when accessing
          // the null indicator -- not expensive/complicated to implement, but
          // rare enough to want to avoid.

          PCodeOperand* src = inst->getROps()[0];
          if ((src->getOffset() == -1) && !src->forAlignedFormat())
            return FALSE;

          break;
        }

        case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MFLT64_MFLT64:
        case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN32S_MBIN32S:
        case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN64S:
        case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN32S:
        {
          // Make sure that this is nullable before we proceed
          if (!inst->code[7])
            return FALSE;

          break;
        }

        case PCIT::SUM_MFLT64_MFLT64:
        case PCIT::SUM_MBIN32S_MBIN32S:
        case PCIT::SUM_MBIN64S_MBIN64S:
        case PCIT::SUM_MBIN64S_MBIN32S:
          break;

        case PCIT::STRLEN_MBIN32U_MATTR5:
        {
          // Only if we're dealing with a varchar (since fixed-chars should be
          // optimized anyways to just return constant length)
          if (inst->getROps()[0]->isVarchar())
            break;

          return FALSE;
        }

#if 0
        //
        // SUBSTR should be fully-implemented, but testing may still be needed.
        //
        case PCIT::SUBSTR_MATTR5_MATTR5_MBIN32S_MBIN32S:
        {
          PCodeOperand *op1, *op2 = NULL;

          // Assert that result is a varchar
          assert(inst->getWOps()[0]->isVarchar());

          op1 = inst->getROps()[1];
          if (inst->getROps().entries() == 3)
            op2 = inst->getROps()[2];

          // To simplify code generation, both the start offset and the length,
          // if provided, should be constants.
          if (!op1->isConst() || (op2 && !op2->isConst()))
            return FALSE;

          // To simplify code generation, do not process if the start point
          // provided is negative or 0.
          if (getIntConstValue(op1) <= 0)
            return FALSE;

          // Also, some bozo may put 0 for the for length - just ignore that,
          // even though the answer will always be an empty string.
          if (op2 && (getIntConstValue(op2) <= 0))
            return FALSE;

          break;
        }
#endif

#if 0
        case PCIT::POS_MBIN32S_MATTR5_MATTR5:
        {
          // Only do this if pattern string is a constant
          if (inst->getROps()[0]->isConst())
            break;

          return FALSE;
        }
#endif


        case PCIT::MOVE_MBIN32S:
        {
          // Only supported if RETURN follows it (as it always should)
          if (inst->next && (inst->next->code[0] == PCIT::RETURN))
            break;

          return FALSE;
        }


        case PCIT::MOVE_MBIN8_MBIN8_IBIN32S:
        {
          // With genUnalignedMemcpy() available, the jit's version of memcpy
          // is not needed, and so no external calls will be made.
          break;
#if 0
          // Only support if it's guaranteed to be inlined
          if (inst->getWOps()[0]->isVar() && inst->getROps()[0]->isVar() &&
              inst->getROps()[0]->getLen() <= 32)
            break;

          return FALSE;
#endif
        }

        case PCIT::HASHCOMB_BULK_MBIN32U:
          break;

        case PCIT::HASH_MBIN32U_MATTR5:
        case PCIT::HASH_MBIN32U_MPTR32_IBIN32S_IBIN32S:
        case PCIT::HASH_MBIN32U_MBIN16_IBIN32S_IBIN32S:
        case PCIT::HASH_MBIN32U_MBIN32_IBIN32S_IBIN32S:
        case PCIT::HASH_MBIN32U_MBIN64_IBIN32S_IBIN32S:
        {
          // Is it possible that we're hashing constants??  Fold that sh!%@#t !
          assert (!inst->getROps()[0]->isConst());

          // Only if source is a var.  Dealing with temps can be messy since
          // we would have to have the address of it and walk through it one
          // byte at a time - doable, but we shouldn't expect to see temps.
          if (!inst->getROps()[0]->isVar())
            return FALSE;

          // If we're hashing a fixed string, only support it for reasonable
          // sizes (TODO: until code is written to make the hash a loop).
          if ((inst->getOpcode() == PCIT::HASH_MBIN32U_MPTR32_IBIN32S_IBIN32S) &&
              (inst->getROps()[0]->getLen() > 1000))
            return FALSE;

          break;
        }

        case PCIT::MINMAX_MBIN8_MBIN8_MBIN32S_IBIN32S:
          break;

        case PCIT::RANGE_MFLT64:
          break;

        case PCIT::LE_MBIN32S_MFLT64_MFLT64:
        case PCIT::GE_MBIN32S_MFLT64_MFLT64:
        case PCIT::LT_MBIN32S_MFLT64_MFLT64:
        case PCIT::GT_MBIN32S_MFLT64_MFLT64:
        case PCIT::EQ_MBIN32S_MFLT64_MFLT64:
        case PCIT::NE_MBIN32S_MFLT64_MFLT64:
          break;

        case PCIT::OPDATA_MPTR32_IBIN32S:
        case PCIT::OPDATA_MPTR32_IBIN32S_IBIN32S:
        case PCIT::OPDATA_MBIN16U_IBIN32S:
        case PCIT::OPDATA_MATTR5_IBIN32S:
          break;

        case PCIT::CLAUSE_EVAL:
        {
          // on 64-bit the clause pointer is in code[1] and code[2]
          ex_clause* clause = (ex_clause*)*(Long*)&(inst->code[1]);
          NABoolean processNulls = inst->code[1 + PCODEBINARIES_PER_PTR];

          // Only certain clause eval instructions will be supported for now.
          switch (clause->getClassID()) {
            case ex_clause::AGGR_MIN_MAX_ID:
            {
              // This clause must not call process nulls when evaluated
              if (!processNulls) {
                // Can only tolerate certain types for now.
                if (isSupportedClauseOperandType(clause->getOperand(0)))
                  break;

              }

              return FALSE;
            }

            case ex_clause::COMP_TYPE:
            {
              // No special nulls allowed for now.
              if (clause->isSpecialNulls())
                return FALSE;

              switch (((ex_comp_clause*)clause)->getInstruction()) {
                case NE_DATETIME_DATETIME:
                case EQ_DATETIME_DATETIME:
                case LT_DATETIME_DATETIME:
                case GT_DATETIME_DATETIME:
                case LE_DATETIME_DATETIME:
                case GE_DATETIME_DATETIME:
                {
                  // Only support standard types
                  long dtCode =
                    ((ExpDatetime*)(clause->getOperand(1)))->getPrecision();

                  // The lengths of the source operands must be the same.
                  if ((clause->getOperand(1)->getLength()) !=
                      (clause->getOperand(1)->getLength()))
                    return FALSE;

                  switch (dtCode)
                  {
                    case REC_DTCODE_DATE:
                    case REC_DTCODE_TIME:
                    case REC_DTCODE_TIMESTAMP:
                      break;
                    default:
                      return FALSE;
                  }
                  break;
                }

                default:
                  return FALSE;
              }

              break;
            }

            default:
              return FALSE;
          }

          break;
        }

        case PCIT::CONCAT_MATTR5_MATTR5_MATTR5:
        {
          Int32 src1Len = inst->getROps()[0]->getVcMaxLen();
          Int32 src2Len = inst->getROps()[1]->getVcMaxLen();
          Int32 resLen = inst->getWOps()[0]->getVcMaxLen();

          Int32 maxSize = src1Len + src2Len;

          // If the max length of the target operand is less than the sum of
          // the lengths of the source operands, do not support.  Note, this is
          // an easy thing to support, but in almost all the cases the tgt len
          // should be big enough.
          if (resLen < maxSize) {
            assert(FALSE);
            return FALSE;
          }

          break;
        }

        case PCIT::GENFUNC_MATTR5_MATTR5_IBIN32S:
        {
          if ((inst->code[11] == ITM_UPPER) || (inst->code[11] == ITM_LOWER))
            break;

          return FALSE;
        }

        default:
          return FALSE;
      }
    } ENDFE_INST_IN_BLOCK
  } ENDFE_BLOCK_REV_DFO

  return TRUE;
}

jit_type_t PCodeOperand::getJitType()
{
  switch (operandType_) {
    case PCIT::MBIN8:
      return jit_type_ubyte;
    case PCIT::MBIN16U:
      return jit_type_ushort;
    case PCIT::MBIN16S:
      return jit_type_short;
    case PCIT::MBIN32U:
      return jit_type_uint;
    case PCIT::MBIN32S:
      return jit_type_int;
    case PCIT::MBIN64S:
      return jit_type_long;
    case PCIT::MASCII:
    case PCIT::MATTR5:
    case PCIT::MBIGS:
      return jit_type_void_ptr;
    case PCIT::MPTR32:
    {
      if (nullBitIndex_ != -1)
        return jit_type_ubyte;
      else
        return jit_type_void_ptr;
    }
    case PCIT::MFLT64:
      return jit_type_float64;
  }

  // No other types supported
  assert(FALSE);

  return jit_type_void;
}
#endif /* NA_LINUX_LIBJIT */

#if 1 /* NA_LINUX_LLVMJIT */
//
// FIX: This routine was added because of a bug in libjit involving a register
// allocation bug and integer promotion.  If a 32-bit value X is promoted to a
// 64-bit value, so that it can be added to an existing 64-bit value Y, then
// the "sar" instruction is used to extend the value.  Libjit's allocator then
// mistakingly confuses the shifted value to replace the 32-bit value itself.
// If we end up reusing/referencing the 32-bit value elsewhere, the wrong jit
// value will get used.  See fullstack2/TEST023.  Also, included below is a
// simple example that shows the problem:
//
//        temp1 = jit_insn_load_relative(function, s, 0, jit_type_long);
//        temp2 = jit_insn_load_relative(function, s, 8, jit_type_int);
//        temp3 = jit_insn_add(function, temp1, temp2);
//        jit_insn_store_relative(function, s, 12, temp3);
//
//        temp4 = jit_insn_convert(function, temp2, jit_type_long, 0);
//        jit_insn_store_relative(function, t, 0, temp4);
//
//        jit_insn_return(function, tempZero);
//
// If a newer version of libjit comes our way, or if a patch is made to fix the
// problem, this routine is not needed.  For now, it should be called whenever
// a source operand needs to be promoted to a 64-bit value before use.  It
// really is only needed if the operand is live after the use, but there's no
// harm in removing/clearing the saved jit values at this point.
//

void PCodeOperand::clearJitValues(PCodeCfg* cfg)
{
#if 1 /* NA_LINUX_LLVMJIT */
  return;  // Routine NOT needed for LLVM....we hope!
#endif /* NA_LINUX_LLVMJIT */

#ifdef NA_LINUX_LIBJIT
  // Get operand in hash table
  CollIndex i = bvIndex_;
  PCodeOperand* operand = cfg->getMap()->getFirstValue(&i);
  if (operand != this)
    return operand->clearJitValues(cfg);

  jitValue_ = NULL;
  jitValuePtr_ = NULL;
  jitValueBlock_ = NULL;
#endif /* NA_LINUX_LIBJIT */
}

//
// setJitValue() - see code for what this does.
//
// FIX: Overlapped types can have problems
// NOTE: It is unknown if this is a problem when using LLVM
// as opposed to LIBJIT.
//
void PCodeOperand::setJitValue( PCodeCfg    * cfg
                              , IRBldr_t    * Bldr
                              , jit_value_t   val
                              , PCodeBlock  * b
                              , NABoolean     noSave = FALSE
                              )
{
#if NExprDbgLvl >= VV_NO
  char  NExBuf[500];
#endif
  // If this was called to store into a non-temp, storeJitValue should be
  // called instead.
  if (!isTemp())
////return storeJitValue(cfg, f, jit_value_get_type(val), val, b, NULL, noSave);
    return storeJitValue(cfg, Bldr, val->getType(), val, b, NULL, noSave );

  // Also call storeJitValue if it's a temp and the temp already has a different
  // jit value associated with it.
  if (isTemp() && (jitValue_ != val))
////return storeJitValue(cfg, f, jit_value_get_type(val), val, b, NULL, noSave);
    return storeJitValue(cfg, Bldr, val->getType(), val, b, NULL, noSave );

  // Get operand in hash table
  CollIndex i = bvIndex_;
  PCodeOperand* operand = cfg->getMap()->getFirstValue(&i);

    CDPT2( "VV0c00: ", VV_BD, "In setJitValue(): operand=%p, this=%p\n", operand, this );

  if (operand != this)
    return operand->setJitValue(cfg, Bldr, val, b, noSave);

  // If we are directly told not to save the jit values, don't!
  if (!noSave) {
    jitValue_ = val;

    CDPT2( "VV0c10: ", VV_BD, "In setJitValue(): setting jitValue_ in PCodeOp "
                              "at %p to %p\n", this, jitValue_ );
    CDPT1( "VV0c11: ", VV_BD, "jitValue_'s Type ID is %d\n",
                                     (int) val->getType()->getTypeID() );
    CDPT2( "VV0c12: ", VV_BD, "In setJitValue(): setting jitValueBlock_ = "
                              "%p for PCOp = %p\n", b , this);
    jitValueBlock_ = b;
  }
}
#endif /* NA_LINUX_LLVMJIT */

#ifdef NA_LINUX_LIBJIT
// FIX: Overlapped types can have problems
void PCodeOperand::setJitValue(PCodeCfg* cfg,
                               jit_function_t f,
                               jit_value_t val,
                               PCodeBlock* b,
                               NABoolean noSave = FALSE)
{
  // If this was called to store into a non-temp, storeJitValue should be
  // called instead.
  if (!isTemp())
    return storeJitValue(cfg, f, jit_value_get_type(val), val, b, NULL, noSave);

  // Also call storeJitValue if it's a temp and the temp already has a different
  // jit value associated with it.
  if (isTemp() && (jitValue_ != val))
    return storeJitValue(cfg, f, jit_value_get_type(val), val, b, NULL, noSave);

  // Get operand in hash table
  CollIndex i = bvIndex_;
  PCodeOperand* operand = cfg->getMap()->getFirstValue(&i);
  if (operand != this)
    return operand->setJitValue(cfg, f, val, b, noSave);

  // If we are directly told not to save the jit values, don't!
  if (!noSave) {
    jitValue_ = val;
    jitValueBlock_ = b;
  }
}
#endif /* NA_LINUX_LIBJIT */

//
// isSupportedClauseOperandType() - determine if the NE code 
// supports this particular PCIT::CLAUSE_EVAL's operand.
//
NABoolean PCodeCfg::isSupportedClauseOperandType(exp_Attributes* attr)
{
  switch(PCIT::getMemoryAddressingMode(attr->getDatatype()))
  {
    case PCIT::MBIN8:
    case PCIT::MBIN16U:
    case PCIT::MBIN16S:
    case PCIT::MBIN32U:
    case PCIT::MBIN32S:
    case PCIT::MBIN64S:
    case PCIT::MASCII:
    case PCIT::MATTR3:
    // case PCIT::MATTR5: // Need to fix MIN_MAX for varchars first

#ifdef  NA_LINUX_LIBJIT
    case PCIT::MFLT64:       // Not supported by LLVMJIT yet
    case PCIT::MBIGS:        // Not supported by LLVMJIT yet
#endif /* NA_LINUX_LIBJIT */
      return TRUE;
  }

  return FALSE;
}

void PCodeCfg::setupClauseOperand(PCodeCfg* cfg,
                                  OPLIST& opList,
                                  PCodeOperand** opData,
                                  Int32 index,
                                  ex_clause* clause)
{
#if NExprDbgLvl > VV_NO
  char  NExBuf[500];
#endif
  Int32 len;
  PCIT::AddressingMode am;
  DPT5( "VV0a00: ", VV_XD, 
        "In setupClauseOperand(), cfg=%p, opList at %p, opData at %p, "
        "index=%d, clause at %p\n", cfg, &opList[0], opData, index, clause );

  // Ignore calls to setup the operand for varchar lengths, since that is not
  // useful once we have the actual column attribute.
  if ((index >= 5) && (index < 10))
  {
    DPT1( "VV0a01: ", VV_XD, 
          "Returning from setupClauseOperand() because index = %d - "
          "indicating varchar\n", index);
    return;
  }

  // Get the attribute pointer corresponding to this clause operand
  Int32 opIndex = index % ex_clause::MAX_OPERANDS;
  exp_Attributes* attr = clause->getOperand(opIndex);

  // Get the old (pcode opts) operand from the opList and clear the list now.

#if NExprDbgLvl >= VV_XD
  for (Int32 iii = 0; iii < opList.entries(); iii++ )
     DPT2( "VV0a10: ", VV_XD, "At start of setupClauseOperand(), opList[%d] = "
                              "%p\n", iii, opList[iii]);
#endif

  PCodeOperand* operand = opList[0];
  opList.clear();
  DPT1( "VV0a20: ", VV_XD, "AFTER clearing opList, 'operand=OLD opList[0] == "
                           "%p\n", operand );

#if NExprDbgLvl >= VV_XD
  for (Int32 jjj = 0; jjj < opList.entries(); jjj++ )
     DPT2( "VV0a30: ", VV_XD, "AFTER clearing opList, opList[%d] = %p\n",
                                                             jjj,  opList[jjj]);
#endif

  // Set up a bogus pcode array needed to call ::addOperand later on.
  PCodeBinary fakePCode[2] = { operand->getStackIndex(), operand->getOffset() };

  // Process things separately if we're getting the null indicator.
  if(index < 5) {
    am = ((attr->getNullBitIndex() != -1) ? PCIT::MPTR32 : PCIT::MBIN16S);
    len = (am == PCIT::MPTR32) ? -1 : 2;
    DPT3( "VV0a40: ", VV_XD, " AM = PCIT::%d, len=%d, index=%d\n",
                                     (int)am, len,        index );

    cfg->addOperand(fakePCode, opList, 0, 1, attr->getNullBitIndex(),am,len,-1);
  }
  // Now we're processing any other type (e.g string, int, floats, etc).
  else if (attr->getVCIndicatorLength() > 0) {
    am = PCIT::MATTR5;
    len = attr->getLength();
    DPT2( "VV0a50: ", VV_XD, " AM = PCIT::%d, index=%d\n", (int)am, index );

    cfg->addOperand(fakePCode, opList, 0, 1, -1, am, len, attr->getVoaOffset());
  }
  else {
    switch (attr->getLength()) {
      case 1:
      case 2:
      case 4:
      case 8:
        am = PCIT::getMemoryAddressingMode(attr->getDatatype());
        break;
      default:
        am = PCIT::MASCII;
        break;
    }

    DPT3("VV0a60: ", VV_XD, " AM = PCIT::%d, index=%d, attrLen = %d\n",
                                   (int)am , index,    attr->getLength());

    DPT2("VV0a80: ", VV_XD, " attr->getDatatype() = %d, "
     "PCIT::getMemoryAddressingMode(attr->getDatatype()= %d\n",
      attr->getDatatype(), PCIT::getMemoryAddressingMode(attr->getDatatype()) );

    cfg->addOperand(fakePCode, opList, 0, 1, -1, am, attr->getLength(), -1);
  }

  // Update the opData array with the new operand.
  opData[index] = opList[0];
  DPT2("VV0a90: ", VV_XD, " JUST UPDATED opData[index = %d] to be %p\n",
                                                     index,    opData[index] );
  DPT2("VV0a98: ", VV_XD, " At END of setupClauseOperand, opData[%d] == %p\n",
                                                         index, opData[index]);
  PCodeOperand *PCOp = opData[index];

  DPT6("VV0a99: ", VV_XD, " DETAILS OF PCodeOperand are: \n"
     "stackIndex_ = %d, offset_ = %d, len_=%d, operandType_=%d, "
     "nullBitIdx=%d, CJV=%p\n",
      PCOp->getStackIndex(), PCOp->getOffset(), PCOp->getLen(),
      PCOp->getType(), PCOp->getNullBitIndex(), PCOp->getCurrJitVal() );
}

#if 1 /* NA_LINUX_LLVMJIT */

//
// IR_LoadRelativeWithType() - generate a "load" instruction to load a value
// at a specified byte offset from where the specified pointer points.  
// The "typeToLoad" argument specifies what kind of "load" to generate.
//
// NOTE: Although this routine generates a "load" instruction, it returns
// the pointer to that instruction as a jit_value_t (i.e. an llvm::Value).
// The caller can use this jit_value_t AS IF we had returned the value
// that would actually be fetched by the load!  Basically, what we are
// really doing is NOT generating a "load" instruction, but rather
// teaching the Intermediate Representation generator HOW TO materialize
// the value at runtime for the object (byte, short, int, or whatever)
// that would be fetched if a "load" instruction was actually done.
// It is for this reason that the caller can use the returned jit_value_t
// AS IF we had returned the value of the object.
//
jit_value_t  PCodeCfg::IR_LoadRelativeWithType( IRBldr_t    * Bldr
                                              , jit_value_t   int8p
                                              , Int64         byteOffset
                                              , jit_type_t    typeToLoad
                                              )
{
#if NExprDbgLvl > VV_NO
   char  NExBuf[500];
#endif
   DPT0(  "VV0100: ", VV_BD, "In IR_LoadRelativeWithType\n" );

   jit_value_t int8p2, byOff, finalPtr, rtnVal ;

   if ( byteOffset == 0 )
      int8p2 = int8p ;
   else
   {
      byOff   = ConstantInt::get( int64Ty_, byteOffset ) ;

      DPT1("VV0101: ", VV_VD,
     "int8p2 = Bldr->CreateGEP( int8p, byOff = %ld)\n", byteOffset );
      int8p2 = Bldr->CreateGEP( int8p, byOff );
   }

   DPT0("VV0102: ", VV_VD, 
  "finalPtr   = Bldr->CreatePointerCast(int8p2, typeToLoad->getPointerTo() )\n" );
   finalPtr   = Bldr->CreatePointerCast( int8p2, typeToLoad->getPointerTo() );

   DPT0("VV0103: ", VV_VD,
              "Load_inst = Bldr->CreateLoad( finalPtr, 'DataVal' )\n" );
   LoadInst *  Load_inst = Bldr->CreateLoad( finalPtr, "DataVal" );

   rtnVal    = Load_inst ;
   return( rtnVal );
}

//
// IR_StoreRelativeWithType() - generate a "store" instruction to store a
// value at a specified byte offset from where the specified pointer points.  
// The "typeToStore" argument specifies what kind of "store" to generate.
//
void PCodeCfg::IR_StoreRelativeWithType( IRBldr_t    * Bldr
                                       , jit_value_t   val
                                       , jit_value_t   tgt8ByPtr
                                       , Int64         byteOffset
                                       , jit_type_t    typeToStore
                                       )
{
#if NExprDbgLvl > VV_NO
   char  NExBuf[500];
#endif
   jit_value_t tgt8ByPtr2, byOff, finalTgt ;

   DPT0(  "VV0200: ", VV_BD, "In IR_StoreRelativeWithType\n" );

   if ( byteOffset == 0 )
      tgt8ByPtr2 = tgt8ByPtr ;
   else
   {
      byOff  = ConstantInt::get( int64Ty_, byteOffset ) ;

      DPT1( "VV0201: ", VV_VD, 
     "Tgt_8p2 = Bldr->CreateGEP( tgt8ByPtr, byOff = %ld ) \n", byteOffset );
      tgt8ByPtr2 = Bldr->CreateGEP( tgt8ByPtr, byOff );
   }

   DPT0( "VV0202: ", VV_VD,
  "finalTgt = Bldr->CreatePointerCast(tgt8ByPtr2, typeToStore->getPointerTo() )\n" );
   finalTgt = Bldr->CreatePointerCast(tgt8ByPtr2, typeToStore->getPointerTo() );

   DPT0( "VV0202: ", VV_VD,
  "Bldr->CreateStore( val, finalTgt )\n" );
   Bldr->CreateStore( val, finalTgt );
}

//
// getJitValue() - generate code to fetch the specified value.
//
// FIX: Overlapped types can have problems too
// NOTE: It is unknown if this is a problem when using LLVM
// as opposed to LIBJIT.
//
jit_value_t PCodeOperand::getJitValue( PCodeCfg        * cfg
                                     , IRBldr_t        * Bldr
                                     , jit_type_t        IRtype
                                     , PCodeBlock      * blk
                                     , PCodeOperand    * orig = NULL
                                     , NABoolean         noSave = FALSE
                                     )
{
#if NExprDbgLvl >= VV_NO
  char  NExBuf[500];
#endif
  CDPT1( "VV0300: ", VV_BD, "GOT into getJitValue() on PCode Operand at %p\n", this );

  // Get operand in hash table
  CollIndex i = bvIndex_;
  PCodeOperand* operand = cfg->getMap()->getFirstValue(&i);

  CDPT2( "VV0301: ", VV_BD, "In getJitValue(): operand=%p, this=%p\n", operand, this );

  if (operand != this)
  {
    return operand->getJitValue(cfg, Bldr, IRtype, blk, this, noSave);
  }

  // Make sure we properly indicate that this object *is* the original one.
  if (orig == NULL)
      orig = this;

  //
  // Get pointer to operand, if that's what is needed
  // 

  if ( IRtype == cfg->getInt1PtrTy() ) {

  CDPT0( "VV0310: ", VV_BD, "In getJitValue, IRtype == cfg->getInt1PtrTy()\n");
  CDPT1( "VV0311: ", VV_BD, "jitValuePtr_ = %p\n", jitValuePtr_ );

    // If already defined somewhere, perhaps it can be reused?
    if (jitValuePtr_) {
      assert(jitValuePtrBlock_ != NULL);

      // If the PCBlock given in jitValuePtrBlock_ dominates PCBlock blk,
      // then we are assured that jitValuePtr_ is valid.
      //
      if (blk->onesVector.testBit(jitValuePtrBlock_->getBlockNum()))
        return jitValuePtr_;
      else
        jitValuePtr_ = NULL;
    }
    CDPT1( "VV0315: ", VV_BD, "jitValuePtr_ is NOW = %p\n", jitValuePtr_);

    // Non-native temps will be accessed directly from the passed in temps
    // area.  If that temp was previously defined as a jit value (e.g. size of
    // temp fit native storage container), then we skip to the else clause so
    // the address is simply taken for the jit value.
    if (isVar() || isConst() || (!jitValue_ && orig->isNonNativeTemp())) {
      jit_value_t parm1 = cfg->getJitParams()[stackIndex_];

    CDPT2( "VV0317: ", VV_BD, "In getJitValue, stackIndex_ = %d, parm1 at %p \n", stackIndex_ , parm1 );
    CDPT0( "VV0320: ", VV_BD, "param = Bldr->CreatePointerCast(parm1, cfg->getInt8PtrTy() )\n" );

    jit_value_t param = Bldr->CreatePointerCast(parm1, cfg->getInt8PtrTy() );

    CDPT1( "VV0321: ", VV_BD, "In getJitValue, param [Int8Ptr] at %p \n", param );
    CDPT0( "VV0322: ", VV_BD, "In getJitValue, found isVar() || isConst() || ...\n" );

#if  NExprDbgLvl >= VV_BD
    if (isVar())
       CDPT0( "VV0323: ", VV_BD, "In getJitValue, found isVar() TRUE\n" );
    if (isConst())
       CDPT0( "VV0324: ", VV_BD, "In getJitValue, found isConst() TRUE\n" );
    if (isNonNativeTemp())
       CDPT0( "VV0325: ", VV_BD, "In getJitValue, found isNonNativeTemp() TRUE\n" );
#endif
    CDPT1( "VV0326: ", VV_BD, "In getJitValue, getType() returns %d\n", getType() );

      // For varchars we return a pointer to the start of length indicator.
      if (getType() == PCIT::MATTR5) {

        // If we're not dealing with a varchar, or if it is the first varchar,
        // we don't need to go through the VOA offset.
        if (!isVarchar() || (getVoaOffset() == -1)) {
//        jitValuePtr_ =
//          jit_insn_add_relative(param, (offset_ - vcIndicatorLen_));

           jit_value_t tmp1 = ConstantInt::get( cfg->getInt64Ty(),
                                                offset_ - vcIndicatorLen_ ) ;
           CDPT2( "VV0330: ", VV_BD,
                  "jitValuePtr_ = Bldr->CreateGEP( param, tmp1 = %d - %d )\n",
                                                    offset_, vcIndicatorLen_ );

           jitValuePtr_ = Bldr->CreateGEP( param, tmp1 );
        }
        else
        {
          jit_type_t MyTy = ((vcIndicatorLen_ == 2)
//                         ? jit_type_ushort : jit_type_uint);
                           ? cfg->getInt16Ty() : cfg->getInt32Ty());

////      jit_value_t v = jit_insn_load_relative(f, param, voaOffset_, MyTy);

          jit_value_t vcLen = cfg->IR_LoadRelativeWithType( Bldr, param,
                                                    voaOffset_,  MyTy );

          //
          // NOTE: The 16-bit or 32-bit offset that we just "loaded" should be
          // treated as an *unsigned* value so that we don't reference some 
          // negative offset from the beginning of the string/object/structure.
          // So, here we generate a Zero-Extend instruction to get the offset
          // extended out to 64 bits before it is added to the address of the object.
          //
          jit_value_t unsignedVcLen ;

          CDPT0( "VV0334: ", VV_BD,
         "unsignedVcLen  = Bldr->CreateZExt( vcLen, cfg->getInt64Ty() , 'ZExtVcLen' )\n" );
          unsignedVcLen  = Bldr->CreateZExt( vcLen, cfg->getInt64Ty() , "ZExtVcLen" );

////      jitValuePtr_ = jit_insn_add(f, param, v);

          CDPT0( "VV0335: ", VV_BD,
         "jitValuePtr_ = Bldr->CreateGEP( param, unsignedVcLen )\n" );
          jitValuePtr_ = Bldr->CreateGEP( param, unsignedVcLen );

          // Now set up pointer, moving past null indicator if required.
          if (vcNullIndicatorLen_)
          {
////        jitValuePtr_ = jit_insn_add_relative(f, jitValuePtr_,
////                                             vcNullIndicatorLen_);

            CDPT0( "VV0340: ", VV_BD,
           "jitValuePtr_ = Bldr->CreatePointerCast( jitValuePtr_, cfg->getInt8PtrTy() )\n" );
            jitValuePtr_ = Bldr->CreatePointerCast( jitValuePtr_, cfg->getInt8PtrTy() );

            jit_value_t NI_len = ConstantInt::get( cfg->getInt64Ty(), vcNullIndicatorLen_ ) ;

            // Now make jitValuePtr_ point to the first byte AFTER the Null Indicator

            CDPT0( "VV0341: ", VV_BD,
           "jitValuePtr_ = Bldr->CreateGEP( jitValuePtr_ , NI_len )\n" );
            jitValuePtr_ = Bldr->CreateGEP( jitValuePtr_ , NI_len );
          }
        }
         CDPT0( "VV0345: ", VV_BD,
        "jitValuePtr_ = Bldr->CreatePointerCast( jitValuePtr_, cfg->getInt8PtrTy() )\n" );
         jitValuePtr_ = Bldr->CreatePointerCast( jitValuePtr_, cfg->getInt8PtrTy() );

      } // End: if (getType() == PCIT::MATTR5)

      else {
////    jitValuePtr_ = jit_insn_add_relative(f, param, offset_);

        jit_value_t tmp1 = ConstantInt::get( cfg->getInt64Ty(), offset_ ) ;

         CDPT1( "VV0348: ", VV_BD,
        "jitValuePtr_ = Bldr->CreateGEP( param, tmp1 = %d )\n", offset_ );
         jitValuePtr_ = Bldr->CreateGEP( param, tmp1 );
      }

    } // End: if (isVar() || isConst() || (!jitValue_ && orig->isNonNativeTemp()))

    else {

      CDPT1( "VV0350: ", VV_BD, "PCodeOperand::getType() RETURNED %d\n", (int)getType() );

      // Normally we would force the jitValue_ to be defined if we're attempting
      // to get it.  However, in some cases (like FILL_MEM_BYTES) we make a 
      // a call to get it first so that things like the vc length can be written
      // to it first.
      // FIX: fix this so that we can continue to assert that jitValue_ should
      //      be defined first.  Fixing it involves having storeJitValue support
      //      all possible stores (vc lengths, actual values, etc.).

      // Must be called and it must already be defined

      if (!jitValue_) {
        // We reached a situation that we can't support.  For example, we could
        // having something like this:
        //
        // B1:  [2,0] = ...
        //      Branch B3
        // B2:  [2,0] = ...
        // B3:    = [2,0]
        //
        // If we allocated space for [2,0] in B1, and then do the same in B2,
        // which one do we use for B3?  Also, if we try to allocate space for
        // [2,0] in B3, then it would be completely wrong.

        cfg->setJitFailureSeen();
        CDPT0( "VV0352: ", VV_BD, "setJitFailureSeen() was just called!\n" );

        // And for the sake of nothing else failing along the way, use a
        // bogus value to return.
        return cfg->getFalse_JitVal_() ;
      }

      assert (jitValue_ && isTemp());

#if 0 /* NOT NEEDED WITH LLVM */
      // Temporary value needs to be addressable.
      jit_value_set_addressable(jitValue_);
#endif

      // Return the address of this temp variable.

      // jitValuePtr_ = jit_insn_address_of(f, jitValue_);

      CDPT0( "VV0355: ", VV_BD,
     "jitValuePtr_ = jitValue_ ; // USE jitValue_ AS ITS ADDRESS\n");
      jitValuePtr_ = jitValue_ ; // For LLVM, we can use the Value ptr *AS* its address!

      CDPT2( "VV0357: ", VV_BD, "jitValuePtr_ at %p is POINTING to Value rep by "
                                                       "jitValue_ at %p\n",
                                                     jitValuePtr_, jitValue_ );
      CDPT1( "VV0359: ", VV_BD, "CREATED Value's Type ID is %d\n",
                                     (int) jitValue_->getType()->getTypeID() );

      // If the jitValue_ was not defined in this current block, then the safe
      // thing to do is simply return the address of jitValue, but *do not* 
      // save the pointer in jitValuePtr_, since that will lead to confusion
      // on what was defined where.
      if (jitValuePtrBlock_ != blk) {
        jit_value_t temp = jitValuePtr_;
        jitValuePtr_ = NULL;

        CDPT1( "VV0360: ", VV_BD, "returning tempVal = %p\n", temp );
        return temp;
      }

    } //End: else [i.e. NOT ((isVar() || isConst() || ... ]

    CDPT2( "VV0361: ", VV_BD, "In getJitValue(): setting jitValuePtrBlock_ = %p "
                                        "for PCop = %p\n", blk, this );
    jitValuePtrBlock_ = blk;

    // If we are directly told not to save the jit values, don't!
    if (noSave)
    {
      jit_value_t tempVal = jitValuePtr_;
      jitValueBlock_ = NULL;
      jitValuePtrBlock_ = NULL;

      CDPT1( "VV0362: ", VV_BD, "In getJitValue(): NULLing jitValue_ in PCodeOp at %p\n", this );
      jitValue_ = jitValuePtr_ = NULL;

      CDPT1( "VV0364: ", VV_BD, "returning tempVal = %p\n", tempVal );

      return tempVal;
    }

    CDPT1( "VV0366: ", VV_BD, "returning jitValuePtr_ = %p\n", jitValuePtr_ );

    return jitValuePtr_;

  } //End: if ( IRtype == cfg->getInt1PtrTy() )

  CDPT2( "VV0368: ", VV_BD, "IRtype != cfg->getInt1PtrTy() for PCodeOp at %p "
                                   "with jitValue_ = %p\n", this, jitValue_ );

  //
  // Get value of operand
  //

  // If already defined somewhere, perhaps it can be reused?  If it's a temp,
  // however, then the assumption is that all uses have a def which reach it,
  // and that def is available in jitValue_.
  if (jitValue_) {
    jit_type_t valType = jitValue_->getType() ;

    CDPT1( "VV0370: ", VV_BD, "jitValue_ != NULL, nullBitIndex_ = %d\n", nullBitIndex_ );
    CDPT2( "VV0372: ", VV_BD, "FOUND jitValue_ in PCodeOp at %p set to %p\n",
                                                           this, jitValue_ );
    CDPT2( "VV0374: ", VV_BD,
           "In getJitValue(): valType->getTypeID() = %d, IRtype->getTypeID()=%d \n",
                        (int) valType->getTypeID(), (int) IRtype->getTypeID() );
#if NExprDbgLvl >= VV_BD
    if (valType->isPointerTy())
       CDPT1( "VV0376: ", VV_BD, "In getJitValue(): valType->getContainedType(0)->getTypeID() = %d\n",
                                              (int) valType->getContainedType(0)->getTypeID() );
#endif

    assert(jitValueBlock_ != NULL);

  CDPT1( "VV0376A: ", VV_BD, "In getJitValue(): jitValueBlock_ = %p\n", jitValueBlock_) ;
  CDPT1( "VV0376A: ", VV_BD, "In getJitValue(): isTemp() = %x\n", (Int32) isTemp() );
  CDPT2( "VV0376A: ", VV_BD, "In getJitValue(): blk = %p, jitValueBlock Dominates blk = %x\n",
               blk,  (Int32) blk->onesVector.testBit(jitValueBlock_->getBlockNum()) );

    // If the PCBlock given in jitValueBlock_ dominates PCBlock blk,
    // then we are assured that jitValue_ is valid.
    //
    if ( (isTemp() || blk->onesVector.testBit(jitValueBlock_->getBlockNum())) &&
         (( valType->getTypeID() == IRtype->getTypeID() ) ||
          ( valType->isPointerTy()  &&
            valType->getContainedType(0)->getTypeID() == IRtype->getTypeID()
          ) ||
          ( isTemp() && valType->isPointerTy()  &&
            valType->getContainedType(0)->isPointerTy()
          )
         )
       )
    {
      CDPT0( "VV0377: ", VV_BD, "Found jitValue_ != NULL and MAY be able to use it\n" );

      // It's possible that we have some overlapping operands.  The following
      // deals with it (or at least for the majority of the cases where we see
      // an overlap :()
//      jit_type_t jitValType = jit_value_get_type(jitValue_);
//      if (jit_type_get_size(jitValType) > jit_type_get_size(type))
//        return jit_insn_convert(f, jitValue_, type, 0);

      assert ( IRtype->getTypeID() == Type::IntegerTyID );

      Int32        valBitWdth ;
      jit_value_t  actualVal = jitValue_ ;

      if ( valType->isPointerTy() )
      {
         CDPT0( "VV0378: ", VV_BD,
        "actualVal = Bldr->CreateLoad( jitValue_, 'loadTemp' )\n" );
         actualVal = Bldr->CreateLoad( jitValue_, "loadTemp" ) ;

         valBitWdth = actualVal->getType()->getIntegerBitWidth() ;
      }
      else
      {
         valBitWdth = valType->getIntegerBitWidth() ;
      }

      if ( valBitWdth != IRtype->getIntegerBitWidth() )
      {
         CDPT0( "VV0380: ", VV_BD,
        "return( Bldr->CreateSExtOrTrunc( actualVal, (IntegerType *)IRtype, 'SExtOrTrunk' ) )\n" );
         return( Bldr->CreateSExtOrTrunc( actualVal,
                                       (IntegerType *)IRtype, "SExtOrTrunk" ) );
      }

      return actualVal ;

    } // End: if ( (isTemp() || blk->onesVector.testBit(... 

    CDPT1( "VV0382: ", VV_BD, "In getJitValue(): NULLing jitValue_ in PCodeOp at %p\n", this );

    jitValue_  = NULL;

  } // End: if (jitValue_)

  // NOTE: If we get here, then either jitValue_ was NULL or
  //       we decided it was not valid.

  if (isConst()) {

    CDPT1( "VV0384: ", VV_BD, "isConst() is TRUE.  Also - nullBitIndex_ = %d\n", nullBitIndex_ );

    //
    // FIX: the assumption right now is that only integers are considered for
    // native expressions.  That will change shortly.  In the meantime, however,
    // this implies that getIntConstValue() should be called on the operand to
    // retrieve the constant value.
    //
    // FIX: getIntConstValue() should probably be "relaxed" a little so as to
    // support MPTR32 and MFLT (both of which may be casted into integer types
    // during same-sized moves).  For now, work around by adding logic here.
    //

    if ( IRtype == cfg->getInt64Ty() ) {
      // MFLT64 can sometimes be the original type of the operand being sought
      // after (e.g. PCIT::MFLT64_MFLT64 --> PCIT::MBIN64S_MBIN64S).  Use the
      // original operand to get the constant value.
      // Likewise, we should use the original type in the case of MBIGS or MBIGU
      // because PCODE optimization may change MBIG[SU]-type PCODE instructions
      // to simpler instructions such as 64-bit move instructions.

      PCodeOperand* op = ((getType() == PCIT::MFLT64) ||
                          (getType() == PCIT::MFLT32) ||
                          (getType() == PCIT::MBIGS ) ||
                          (getType() == PCIT::MBIGU )
                         )
                            ? orig : this;

      Int64 val = cfg->getIntConstValue(op);

//    jitValue_ = jit_value_create_long_constant(f, type, val);
      jitValue_ = ConstantInt::get( IRtype, val );
      CDPT2( "VV0386: ", VV_BD,
             "In getJitValue(): setting jitValue_ in PCodeOp at %p to %p\n",
                                                       this, jitValue_ );
      CDPT1( "VV0388: ", VV_BD, "In getJitValue(): CREATED Value's Type ID is %d\n",
                                        (int) jitValue_->getType()->getTypeID() );
    }
    else if (getType() == PCIT::MPTR32) {
      // MPTR32 can sometimes be the original type of the operand being sought
      // after (e.g. PCIT::MBIN8_MBIN8_IBIN32S --> PCIT::MBIN8_MBIN8).  Use the
      // original operand to get the constant value.

      Int32 val = (Int32)cfg->getIntConstValue(((orig) ? orig : this));
//    jitValue_ = jit_value_create_nint_constant(f, type, val);

      CDPT1( "VV0390: ", VV_BD, "In getJitValue(): setting jitValue_ to be a constant Int of %d\n", val );
      CDPT1( "VV0391: ", VV_BD, "In getJitValue(): and jitValue_'s bitWidth is %d\n",
                                               IRtype->getIntegerBitWidth() );
      jitValue_ = ConstantInt::get( IRtype, val );

      CDPT2( "VV0392: ", VV_BD, "In getJitValue(): setting jitValue_ in PCodeOp at %p to %p\n",
                                            this, jitValue_ );
      CDPT1( "VV0394: ", VV_BD, "In getJitValue(): CREATED Value's Type ID is %d\n",
                                         (int) jitValue_->getType()->getTypeID() );
    }
    else if ( IRtype->isFloatTy() ) {
      assert(0==245); // NEED TO add real code here when 
                      // we want to use LLVM on Floating Pt
                      // related PCODE instructions

      // Unless the value is "1.0", libjit will store a double constant in
      // memory, referenced via %ip, and that will blow us up at runtime.  Get
      // it from memory instead (if not 1.0)
#if 0  // FIX THIS when want to use LLVM on Floating Pt related PCODE instructions
      double val = cfg->getFloatConstValue(((orig) ? orig : this));
      if (val == 1.0)
        jitValue_ = jit_value_create_float64_constant(f, type, val);
      else {
        jit_value_t parm1 = cfg->getJitParams()[stackIndex_];
        jit_value_t param  = Bldr->CreatePointerCast(parm1, Int8PtrTy );
        jitValue_ = jit_insn_load_relative(f, param, offset_, type);
      }
#endif // 0
    }
    else {
      Int32 val = (Int32)cfg->getIntConstValue(((orig) ? orig : this));
//    jitValue_ = jit_value_create_nint_constant(f, type, val);
      jitValue_ = ConstantInt::get( IRtype , val );

      CDPT2( "VV0396: ", VV_BD, "In getJitValue(): setting jitValue_ in PCodeOp at %p to %p\n",
                                        this, jitValue_ );
      CDPT1( "VV0397: ", VV_BD, "CREATED Value's Type ID is %d\n", (int) jitValue_->getType()->getTypeID() );
      CDPT1( "VV0398: ", VV_BD, "CREATED Value's actual value is %d\n", val );
    }

  } // End: if (isConst())

  else if (isVar()) {

    CDPT1( "VV0399: ", VV_BD, "nullBitIndex_ = %d\n", nullBitIndex_ );

    jit_value_t parm1 = cfg->getJitParams()[stackIndex_];

    CDPT0( "VV0400: ", VV_BD,
             "param = Bldr->CreatePointerCast(parm1, cfg->getInt8PtrTy() )\n" );
    jit_value_t param = Bldr->CreatePointerCast(parm1, cfg->getInt8PtrTy() );

    // Are we getting the null bit?
    if (nullBitIndex_ != -1) {
      // For paranoia, make sure offset_ is not -1 also (indicating packed
      // format against a varchar, which is not supported yet).
      assert(offset_ != -1);

////  jit_value_t mask = jit_value_create_nint_constant(f, jit_type_ubyte,
////                       (UInt8)0x1 << (7 - (nullBitIndex_ & 7)));

      jit_value_t mask = ConstantInt::get( cfg->getInt8Ty(),
                                 ((UInt8)0x1 << (7 - (nullBitIndex_ & 7)) ) );

////  jit_value_t nByte = jit_insn_load_relative(f, param,
////                        offset_ + (nullBitIndex_ >> 3), jit_type_ubyte);

      jit_value_t nByte = cfg->IR_LoadRelativeWithType( Bldr, param,
                                     offset_ + (nullBitIndex_ >> 3),
                                     cfg->getInt8Ty() );

////  jitValue_ = jit_insn_and(f, nByte, mask);

      CDPT1( "VV0410: ", VV_BD,
     "jitValue_ = Bldr->CreateAnd( nByte, mask = 0x%x )\n", (UInt8)0x1 << (7 - (nullBitIndex_ & 7)) );
      jitValue_ = Bldr->CreateAnd( nByte, mask );

      //
      // NOTE: There is an interesting point to make here.  The CreateAnd()
      // operation that we just did actually returned a pointer to an
      // "AND" instruction whose first operand (nByte) is a pointer to a "load"
      // instruction!   See the discussion at the beginning of
      // IR_LoadRelativeWithType about the fact that what we are really doing
      // is teaching the IR generator HOW TO materialize values at runtime.
      // It is for this reason that jitValue_ now points to an "AND" instruction
      // whose first operand is a "load" instruction.
      //

      CDPT2( "VV0412: ", VV_BD, "In getJitValue(): setting jitValue_ in PCodeOp at %p to %p\n",
                                          this, jitValue_ );
      CDPT1( "VV0413: ", VV_BD, "CREATED Value's Type ID is %d\n",
                                   (int) jitValue_->getType()->getTypeID() );
    }
    else
    {
////  jitValue_ = jit_insn_load_relative(f, param, offset_, type);
      jitValue_ = cfg->IR_LoadRelativeWithType( Bldr, param, offset_, IRtype );

      CDPT2( "VV0415: ", VV_BD, "In getJitValue(): setting jitValue_ in PCodeOp at "
                                "%p to %p\n", this, jitValue_ );
      CDPT1( "VV0417: ", VV_BD, "CREATED Value's Type ID is %d\n",
                                   (int) jitValue_->getType()->getTypeID() );
    }

  } // End: else if (isVar())


  else {
    // Accessing an undefined temp is not allowed.
    // NOTE: An "undefined temp" is one which does not currently have a value.
    // If it has not been given a value, then we should not be in this routine
    // trying to get its value!

    CDPT0( "VV0420: ", VV_BD,
           "ABOUT TO DO assert (FALSE) -- undefined temp is not allowed\n" );

    assert (FALSE);
  }

  CDPT2( "VV0421: ", VV_BD, "In getJitValue(): setting jitValueBlock_ = %p for PCop = %p\n", blk, this );

  jitValueBlock_ = blk;

  // If we are directly told not to save the jit values, don't!
  if (noSave)
  {
    jit_value_t tempVal = jitValue_;
    jitValueBlock_ = NULL;
    jitValuePtrBlock_ = NULL;

    CDPT1( "VV0VV0422: ", VV_BD, "In getJitValue(): NULLing jitValue_ in PCodeOp at "
                                 "%p\n", this );

    jitValue_ = jitValuePtr_ = NULL;
    return tempVal;
  }

  return jitValue_;
}

//
// jitGenCompare() - generate a series of code to do a comparison
// between 2 values.  Returns a "PHI Node".
//
jit_value_t PCodeCfg::jitGenCompare( IRBldr_t    * Bldr
                                   , enum cmpKind  kindOfCompare
                                   , jit_value_t   oper1
                                   , jit_value_t   oper2
                                   , jit_value_t   trueRslt
                                   , jit_value_t   falseRslt
                                   )
{
#if NExprDbgLvl > VV_NO
   char  NExBuf[500];
#endif
   DPT0( "VV0500: ", VV_BD, "GOT into jitGenCompare()\n" );

   llvm::LLVMContext &LLContxt = Bldr->getContext();

   jit_value_t  Zero   = ConstantInt::get( oper1->getType(), 0 );
   jit_value_t  IsZero = (Value *)NULL;
   jit_value_t  Rslt   = (Value *)NULL;

   DPT2( "VV0502: ", VV_XD, "In jitGenCompare(): oper1 has TypeID = "
                            "%d, oper2 has %d\n",
               oper1->getType()->getTypeID(), oper2->getType()->getTypeID() );

#if NExprDbgLvl >= VV_BD
   if ( ( NExprDbgLvl_ >= VV_XD ) &&
        ( oper1->getType()->getTypeID() != oper2->getType()->getTypeID() ) )
   {
      DPT2( "VV0504: ", VV_XD, "In jitGenCompare(): oper1 at %p, oper2 at %p\n",
                                                    oper1,       oper2 );
   }
   else if ( oper1->getType()->getTypeID() == Type::IntegerTyID )
   {
      if ( oper1->getType()->getIntegerBitWidth() != 
           oper2->getType()->getIntegerBitWidth() )
      {
        DPT2( "VV0506: ", VV_XD,
              "In jitGenCompare(): oper1 width = %d, oper2 width = %d\n",
                                   oper1->getType()->getIntegerBitWidth(),
                                   oper2->getType()->getIntegerBitWidth() );
        DPT2( "VV0508: ", VV_XD,
              "In jitGenCompare(): oper1 at %p, oper2 at %p\n",
                                   oper1,       oper2 );
      }
   }
#endif
   switch ( kindOfCompare )
   {
      case IntCompare_EQ :
      {
         // NOTE: We use an XOR instruction here INSTEAD OF a ICmpEQ because
         // LLVM's JIT Compiler for the x86 has a bug in it ... at least in
         // Version 3.2 of LLVM.   If we use a ICmpEQ and the operands are
         // 32-bit operands, LLVM's JIT Compiler sometimes ends up producing
         // a 64-bit comparison!
#if 0
         DPT0( "VV0510: ", VV_VD,
        "IsZero = Bldr->CreateICmpEQ( oper1, oper2, 'ICmpEQ' )\n" );
         IsZero = Bldr->CreateICmpEQ( oper1, oper2, "ICmpEQ" );
#else
         DPT0( "VV0510: ", VV_VD,
        "Rslt = Bldr->CreateXor( oper1, oper2, 'IXor' )\n" );
         Rslt = Bldr->CreateXor( oper1, oper2, "IXor" );

         // NOTE: We must follow the XOR operation with a test for zero.
         // You would think this unnecessary since the 'xor' machine
         // instruction sets the Z flag in the x86's EFLAGS register.
         // However, LLVM requires the CreateCondBr call (see 
         // the one below) to be done on a boolean variable and the
         // result of the XOR is not a boolean, but multiple bits wide.
         // NOTE: This does *not* result in two insructions in the
         // final native code since LLVM's JIT Compiler is smart enough
         // to optimize the unnecessary 'cmp' instruction away!
         // 
         DPT0( "VV0511: ", VV_VD,
        "IsZero  = Bldr->CreateICmpEQ( Rslt, Zero, 'check0' )\n" );
         IsZero  = Bldr->CreateICmpEQ( Rslt, Zero, "check0" );
#endif

         break ;
      }
      case IntCompare_NE :
      {
         DPT0( "VV0520: ", VV_VD,
        "IsZero = Bldr->CreateICmpNE( oper1, oper2, 'ICmpNE' )\n" );
         IsZero = Bldr->CreateICmpNE( oper1, oper2, "ICmpNE" );
         break ;
      }
      case IntCompare_SGT :
      {
         DPT0( "VV0523: ", VV_VD,
        "IsZero = Bldr->CreateICmpSGT( oper1, oper2, 'ICmpSGT' )\n" );
         IsZero = Bldr->CreateICmpSGT( oper1, oper2, "ICmpSGT" );
         break ;
      }
      case IntCompare_UGT :
      {
         DPT0( "VV0525: ", VV_VD,
        "IsZero = Bldr->CreateICmpUGT( oper1, oper2, 'ICmpUGT' )\n" );
         IsZero = Bldr->CreateICmpUGT( oper1, oper2, "ICmpUGT" );
         break ;
      }
      case IntCompare_UGE :
      {
         DPT0( "VV0526: ", VV_VD,
        "IsZero = Bldr->CreateICmpUGE( oper1, oper2, 'ICmpUGE' )\n" );
         IsZero = Bldr->CreateICmpUGE( oper1, oper2, "ICmpUGE" );
         break ;
      }
      case IntCompare_SGE :
      {
         DPT0( "VV0527: ", VV_VD,
        "IsZero = Bldr->CreateICmpSGE( oper1, oper2, 'ICmpSGE' )\n" );
         IsZero = Bldr->CreateICmpSGE( oper1, oper2, "ICmpSGE" );
         break ;
      }
      case IntCompare_SLT:
      {
         DPT0( "VV0528: ", VV_VD,
        "IsZero = Bldr->CreateICmpSLT( oper1, oper2, 'ICmpSLT' )\n" );
         IsZero = Bldr->CreateICmpSLT( oper1, oper2, "ICmpSLT" );
         break ;
      }
      case IntCompare_SLE:
      {
         DPT0( "VV0529: ", VV_VD,
        "IsZero = Bldr->CreateICmpSLE( oper1, oper2, 'ICmpSLE' )\n" );
         IsZero = Bldr->CreateICmpSLE( oper1, oper2, "ICmpSLE" );
         break ;
      }
      case IntCompare_ULT :
      {
         DPT0( "VV0530: ", VV_VD,
        "IsZero = Bldr->CreateICmpULT( oper1, oper2, 'ICmpULT' )\n" );
         IsZero = Bldr->CreateICmpULT( oper1, oper2, "ICmpULT" );
         break ;
      }
      case IntCompare_ULE :
      {
         DPT0( "VV0531: ", VV_VD,
        "IsZero = Bldr->CreateICmpULE( oper1, oper2, 'ICmpULE' )\n" );
         IsZero = Bldr->CreateICmpULE( oper1, oper2, "ICmpULE" );
         break ;
      }
      default :
         assert(0==250) ; // ZZZZ FIX IF WE EVER GET HERE.
         break ;
   }
   // Get ptr to current Function
   llvm::Function *ExpFn = Bldr->GetInsertBlock()->getParent();

   // Create 3 BasicBlock objects
   //
   // Note: We tell LLVM to give informative "names" to the blocks
   // so we more easily recognize what we are seeing when we do an
   // Intermediate Representation (IR) dump....used in debugging.
   //
   p_IR_block_t ThenBB  = BasicBlock::Create( LLContxt, "CmpThenBlk"  );
   p_IR_block_t ElseBB  = BasicBlock::Create( LLContxt, "CmpElseBlk"  );
   p_IR_block_t MergeBB = BasicBlock::Create( LLContxt, "CmpMergeBlk" );

   DPT3( "VV0550: ", VV_VD, "ThenBB = %p, ElseBB = %p, MergeBB = %p\n",
                             ThenBB,      ElseBB,      MergeBB );
   DPT2( "VV0551: ", VV_VD,
  "Bldr->CreateCondBr( IsZero, ThenBB = %p, ElseBB = %p )\n", ThenBB, ElseBB );
   Bldr->CreateCondBr( IsZero, ThenBB, ElseBB );

   DPT1( "VV0552: ", VV_VD,
  "ExpFn->getBasicBlockList().push_back( ThenBB = %p )\n", ThenBB );
   ExpFn->getBasicBlockList().push_back( ThenBB );

   DPT1( "VV0553: ", VV_VD,
  "Bldr->SetInsertPoint( ThenBB = %p )\n", ThenBB );
   Bldr->SetInsertPoint( ThenBB );

   jit_value_t ThenVal = trueRslt;
// NOTE: Since the "Then" Value was passed in, we don't need to generate
// any code here except for a Branch to the Merge Block.

   DPT1( "VV0555: ", VV_VD,
  "Bldr->CreateBr( MergeBB = %p )\n", MergeBB );
   Bldr->CreateBr( MergeBB );

   DPT1( "VV0556: ", VV_VD,
  "ExpFn->getBasicBlockList().push_back( ElseBB = %p )\n", ElseBB );
   ExpFn->getBasicBlockList().push_back( ElseBB );

   DPT1( "VV0557: ", VV_VD,
  "Bldr->SetInsertPoint( ElseBB = %p )\n", ElseBB );
   Bldr->SetInsertPoint( ElseBB );

   jit_value_t  ElseVal = falseRslt;
// NOTE: Since the "Else" Value was passed in, we don't need to generate
// any code here except for a Branch to the Merge Block.

   DPT1( "VV0560: ", VV_VD,
  "Bldr->CreateBr( MergeBB = %p )\n", MergeBB );
   Bldr->CreateBr( MergeBB );

   // Now set up to insert code into the "merge back" common place
   DPT1( "VV0561: ", VV_VD,
  "ExpFn->getBasicBlockList().push_back( MergeBB = %p )\n", MergeBB );
   ExpFn->getBasicBlockList().push_back( MergeBB );

   DPT1( "VV0562: ", VV_VD,
  "Bldr->SetInsertPoint( MergeBB = %p )\n", MergeBB );
   Bldr->SetInsertPoint( MergeBB );

   //
   // Create PHI node.  NOTE: Type of PHI node *must* match Type of Values passed to
   //                         addIncoming(...)
   //
   DPT0( "VV0570: ", VV_VD,
           "PN = Bldr->CreatePHI( trueRslt->getType(), 2, 'iftmp' )\n" );
   PHINode *PN = Bldr->CreatePHI( trueRslt->getType(), 
                                    2, "iftmp" ); // '2' is LLVM version # !!
   DPT2( "VV0571: ", VV_VD,
  "PN->addIncoming( ThenVal at %p, from IR blk at %p )\n", ThenVal, ThenBB );
   PN->addIncoming( ThenVal, ThenBB );

   DPT2( "VV0572: ", VV_VD,
  "PN->addIncoming( ElseVal at %p, from IR blk at %p )\n", ElseVal, ElseBB );
   PN->addIncoming( ElseVal, ElseBB );
   return (PN);
}

//
// getJitType() - return an LLVM type corresponding to the specified
// operand type.
//
jit_type_t  PCodeOperand::getJitType( PCodeCfg * cfg , PCIT::AddressingMode oprTyp )
{
  PCIT::AddressingMode OperTyp = operandType_ ;

  if ( oprTyp != PCIT::AM_NONE )
     OperTyp = oprTyp ;

  switch ( OperTyp ) {
    case PCIT::MBIN8:   return cfg->getInt8Ty()  ;
    case PCIT::MBIN16U: return cfg->getInt16Ty() ;
    case PCIT::MBIN16S: return cfg->getInt16Ty() ;
    case PCIT::MBIN32U: return cfg->getInt32Ty() ;
    case PCIT::MBIN32S: return cfg->getInt32Ty() ;
    case PCIT::MBIN64S: return cfg->getInt64Ty() ;
    case PCIT::MASCII:
    case PCIT::MATTR5:
    case PCIT::MBIGS:
       return cfg->getInt1PtrTy() ; // Use instead of LIBJIT's void ptr type
    case PCIT::MPTR32:
    {
       if ( forAlignedFormat() )
          return cfg->getInt8Ty() ;  // Return a single byte
       else
          return cfg->getInt1PtrTy() ; // Use instead of LIBJIT's void ptr type
    }
    case PCIT::MFLT64:
       return cfg->getFloatPtrTy() ;
  }

  // No other types supported
  assert(FALSE);

  return cfg->getInt1PtrTy() ; // Should not get here.
}

//
// genMemCopyLoop() - generate code to do a memory-to-memory copy.
//
void PCodeCfg::genMemCopyLoop( IRBldr_t    * Bldr
                             , jit_value_t   srcPtr
                             , jit_value_t   tgtPtr
                             , jit_value_t   maxLen
                             )
{
#if NExprDbgLvl > VV_NO
   char  NExBuf[500];
#endif
   jit_value_t  BytesPer, CmpRslt ;
   jit_value_t  RndMaxLen, ByteCtVal ;
   jit_value_t  MaxLen64 ;
   DPT0( "VV0600: ", VV_BD, "GOT into genMemCopyLoop()\n" );

   jit_value_t  Zero  = ConstantInt::get( int64Ty_, 0 );

   llvm::LLVMContext &LLContxt = Bldr->getContext();
   llvm::Function *ExpFn       = Bldr->GetInsertBlock()->getParent();

   p_IR_block_t fastLoopLabel = BasicBlock::Create( LLContxt, "MemCpyFastBlk" );
   p_IR_block_t endLabel      = BasicBlock::Create( LLContxt, "MemCpyEndBlk" );

   DPT2( "VV0602: ", VV_VD, "fastLoopLabel = %p, endLabel = %p\n",
                             fastLoopLabel,      endLabel ); 

   Int32 bytesAtOnce = 8;

   BytesPer          = ConstantInt::get( int64Ty_, bytesAtOnce );
   DPT0( "VV0605: ", VV_VD,
  "MaxLen64          = Bldr->CreateZExt( maxLen, int64Ty_, 'ExtMaxLen')\n" );
   MaxLen64          = Bldr->CreateZExt( maxLen, int64Ty_, "ExtMaxLen");

   DPT0( "VV0606: ", VV_VD,
  "RndMaxLen         = Bldr->CreateUDiv( MaxLen64, BytesPer, 'UDiv' )\n" );
   RndMaxLen         = Bldr->CreateUDiv( MaxLen64, BytesPer, "UDiv" );

   DPT0( "VV0607: ", VV_VD,
  "RndMaxLen         = Bldr->CreateMul( RndMaxLen, BytesPer, 'Mul' )\n" );
   RndMaxLen         = Bldr->CreateMul( RndMaxLen, BytesPer, "Mul" );

   DPT0( "VV0610: ", VV_VD,
  "AllocaInst *  ByteCt = Bldr->CreateAlloca( int64Ty_ )\n" );
   AllocaInst *  ByteCt = Bldr->CreateAlloca( int64Ty_ );

   DPT0( "VV0611: ", VV_VD,
  "Bldr->CreateStore( Zero, ByteCt )\n" );
   Bldr->CreateStore( Zero, ByteCt );

   DPT0( "VV0612: ", VV_VD,
  "CmpRslt = Bldr->CreateICmpULT( RndMaxLen, BytesPer, 'ICmp.RndMaxLen' )\n" );
   CmpRslt = Bldr->CreateICmpULT( RndMaxLen, BytesPer, "ICmp.RndMaxLen" );

   DPT0( "VV0613: ", VV_VD,
  "Bldr->CreateCondBr( CmpRslt, endLabel, fastLoopLabel )\n" );
   Bldr->CreateCondBr( CmpRslt, endLabel, fastLoopLabel );

   // Start fast loop here

   DPT1( "VV0615: ", VV_VD,
  "ExpFn->getBasicBlockList().push_back( fastLoopLabel = %p )\n" , fastLoopLabel);
   ExpFn->getBasicBlockList().push_back( fastLoopLabel );

   DPT0( "VV0616: ", VV_VD,
  "Bldr->SetInsertPoint( fastLoopLabel )\n" );
   Bldr->SetInsertPoint( fastLoopLabel );

   // Copy multiple bytes at a time.
   DPT0( "VV0617: ", VV_VD,
  "ByteCtVal = Bldr->CreateLoad( ByteCt, 'loadByteCt' )\n");
   ByteCtVal = Bldr->CreateLoad( ByteCt, "loadByteCt" ); // Re-load within loop

   DPT0( "VV0618: ", VV_VD,
               "ThisSrc = Bldr->CreateGEP( srcPtr, ByteCtVal, 'ThisSrc' )\n" );
   jit_value_t  ThisSrc = Bldr->CreateGEP( srcPtr, ByteCtVal, "ThisSrc" );

   DPT0( "VV0619: ", VV_VD,
               "ThisTgt = Bldr->CreateGEP( tgtPtr, ByteCtVal, 'ThisTgt' )\n" );
   jit_value_t  ThisTgt = Bldr->CreateGEP( tgtPtr, ByteCtVal, "ThisTgt" );

   jit_value_t MultiBytes = IR_LoadRelativeWithType( Bldr, ThisSrc,
                                                     0, int64Ty_ );
   IR_StoreRelativeWithType( Bldr, MultiBytes, ThisTgt,
                                                     0, int64Ty_ );

   // Increment count of bytes copyied
   DPT0( "VV0621: ", VV_VD,
  "ByteCtVal = Bldr->CreateAdd( ByteCtVal, BytesPer, 'nextIncr')\n");
   ByteCtVal = Bldr->CreateAdd( ByteCtVal, BytesPer, "nextIncr" );

   DPT0( "VV0622: ", VV_VD,
  "Bldr->CreateStore( ByteCtVal, ByteCt, 'storeByteCt' )\n" );
   Bldr->CreateStore( ByteCtVal, ByteCt, "storeByteCt" );


   // while (ByteCtVal < RndMaxLen) branch to fastLoop
   DPT0( "VV0623: ", VV_VD,
  "CmpRslt = Bldr->CreateICmpULT( ByteCtVal, RndMaxLen, 'ICmp.ByteCt' )\n" );
   CmpRslt = Bldr->CreateICmpULT( ByteCtVal, RndMaxLen, "ICmp.ByteCt" );

   DPT0( "VV0624: ", VV_VD,
  "Bldr->CreateCondBr( CmpRslt, fastLoopLabel, endLabel )\n" );
   Bldr->CreateCondBr( CmpRslt, fastLoopLabel, endLabel );

   DPT1( "VV0625: ", VV_VD,
  "ExpFn->getBasicBlockList().push_back( endLabel = %p )\n", endLabel );
   ExpFn->getBasicBlockList().push_back( endLabel );

   DPT0( "VV0626: ", VV_VD,
  "Bldr->SetInsertPoint( endLabel )\n" );
   Bldr->SetInsertPoint( endLabel );

   // Now copy any remaining bytes (from 0 to 7 bytes possible)

   p_IR_block_t slowLoopLabel = BasicBlock::Create( LLContxt, "MemCpySlowBlk" );
   p_IR_block_t end2Label     = BasicBlock::Create( LLContxt, "MemCpyEndBlk" );

   DPT2( "VV0628: ", VV_VD, "slowLoopLabel = %p, end2Label = %p\n",
                             slowLoopLabel,      end2Label ); 

   bytesAtOnce = 1;

   BytesPer          = ConstantInt::get( int64Ty_, bytesAtOnce );

   DPT0( "VV0630: ", VV_VD,
  "ByteCtVal = Bldr->CreateLoad( ByteCt, 'loadByteCt' )\n");
   ByteCtVal = Bldr->CreateLoad( ByteCt, "loadByteCt" );

   DPT0( "VV0632: ", VV_VD,
  "CmpRslt = Bldr->CreateICmpUGE( ByteCtVal, MaxLen64, 'ICmp.RndMaxLen' )\n" );
   CmpRslt = Bldr->CreateICmpUGE( ByteCtVal, MaxLen64, "ICmp.RndMaxLen" );

   DPT0( "VV0633: ", VV_VD,
  "Bldr->CreateCondBr( CmpRslt, end2Label, slowLoopLabel )\n" );
   Bldr->CreateCondBr( CmpRslt, end2Label, slowLoopLabel );

   // Start slow loop here

   DPT1( "VV0635: ", VV_VD,
  "ExpFn->getBasicBlockList().push_back( slowLoopLabel = %p )\n", slowLoopLabel );
   ExpFn->getBasicBlockList().push_back( slowLoopLabel );

   DPT0( "VV0636: ", VV_VD,
  "Bldr->SetInsertPoint( slowLoopLabel )\n" );
   Bldr->SetInsertPoint( slowLoopLabel );

   // Copy multiple bytes at a time.
   DPT0( "VV0637: ", VV_VD,
  "ByteCtVal = Bldr->CreateLoad( ByteCt, 'loadByteCt' )\n");
   ByteCtVal = Bldr->CreateLoad( ByteCt, "loadByteCt" ); // Re-load within the loop

   DPT0( "VV0638: ", VV_VD,
               "CurrSrc = Bldr->CreateGEP( srcPtr, ByteCtVal, 'CurrSrc' )\n" );
   jit_value_t  CurrSrc = Bldr->CreateGEP( srcPtr, ByteCtVal, "CurrSrc" );

   DPT0( "VV0639: ", VV_VD,
               "CurrTgt = Bldr->CreateGEP( tgtPtr, ByteCtVal, 'CurrTgt' )\n" );
   jit_value_t  CurrTgt = Bldr->CreateGEP( tgtPtr, ByteCtVal, "CurrTgt" );

   jit_value_t OneByte = IR_LoadRelativeWithType( Bldr, CurrSrc,
                                                  0, int8Ty_ );
   IR_StoreRelativeWithType( Bldr, OneByte, CurrTgt,
                                                  0, int8Ty_ );

   // Increment loop count
   DPT0( "VV0641: ", VV_VD,
  "ByteCtVal = Bldr->CreateAdd( ByteCtVal, BytesPer, 'nextIncr')\n");
   ByteCtVal = Bldr->CreateAdd( ByteCtVal, BytesPer, "nextIncr" );

   DPT0( "VV0642: ", VV_VD,
  "Bldr->CreateStore( ByteCtVal, ByteCt, 'storeByteCt' )\n" );
   Bldr->CreateStore( ByteCtVal, ByteCt, "storeByteCt" );


   // while (ByteCtVal < MaxLen64) branch to slowLoop
   DPT0( "VV0643: ", VV_VD,
  "CmpRslt = Bldr->CreateICmpULT( ByteCtVal, MaxLen64, 'ICmp.ByteCt' )\n" );
   CmpRslt = Bldr->CreateICmpULT( ByteCtVal, MaxLen64, "ICmp.ByteCt" );

   DPT0( "VV0644: ", VV_VD,
  "Bldr->CreateCondBr( CmpRslt, slowLoopLabel, end2Label )\n" );
   Bldr->CreateCondBr( CmpRslt, slowLoopLabel, end2Label );

   DPT1( "VV0645: ", VV_VD,
  "ExpFn->getBasicBlockList().push_back( end2Label = %p )\n", end2Label );
   ExpFn->getBasicBlockList().push_back( end2Label );

   DPT0( "VV0646: ", VV_VD,
  "Bldr->SetInsertPoint( end2Label )\n" );
   Bldr->SetInsertPoint( end2Label );
}

//
// genMemCmpLoop() - generate code to do a memory-to-memory compare.
//
jit_value_t PCodeCfg::genMemCmpLoop( IRBldr_t    * Bldr
                                   , enum cmpKind  KindOfCmp
                                   , jit_value_t   src1Ptr
                                   , jit_value_t   src2Ptr
                                   , jit_value_t   maxLen
                                   )
{
#if NExprDbgLvl > VV_NO
   char  NExBuf[500];
#endif
   jit_value_t  BytesPer, Is_LT ;
   jit_value_t  RndMaxLen, ByteCtVal ;
   jit_value_t  MaxLen64 ;
   jit_value_t  IsZero   ;
   jit_value_t  IsEqual  ;
   DPT0( "VVa600: ", VV_BD, "GOT into genMemCmpLoop()\n" );

   jit_value_t  Zero  = ConstantInt::get( int64Ty_, 0 );

   llvm::LLVMContext &LLContxt = Bldr->getContext();
   llvm::Function *ExpFn       = Bldr->GetInsertBlock()->getParent();

   p_IR_block_t fastCmpLpLabel  = BasicBlock::Create( LLContxt, "MemCmpFastBlk" );
   p_IR_block_t fastCmpNxtLabel = BasicBlock::Create( LLContxt, "MemCmpFastNextBlk" );
   p_IR_block_t slowCmpLpLabel  = BasicBlock::Create( LLContxt, "MemCmpSlwBlk" );
   p_IR_block_t slowContLabel   = BasicBlock::Create( LLContxt, "MemCmpSlwContBlk" );
   p_IR_block_t slowCmpEndLabel = BasicBlock::Create( LLContxt, "MemCmpSlwEndBlk" );

   DPT2( "VVa602: ", VV_VD, "fastCmpLpLabel = %p, fastCmpNxtLabel = %p\n",
                             fastCmpLpLabel,      fastCmpNxtLabel ); 
   DPT2( "VVa602: ", VV_VD, "slowCmpLpLabel = %p, slowContLabel  = %p\n" ,
                             slowCmpLpLabel,      slowContLabel ); 
   DPT1( "VVa602: ", VV_VD, "slowCmpEndLabel = %p\n",
                             slowCmpEndLabel ); 
   Int32 bytesAtOnce = 8;

   BytesPer          = ConstantInt::get( int64Ty_, bytesAtOnce );
   DPT0( "VVa605: ", VV_VD,
  "MaxLen64          = Bldr->CreateZExt( maxLen, int64Ty_, 'ExtMaxLen')\n" );
   MaxLen64          = Bldr->CreateZExt( maxLen, int64Ty_, "ExtMaxLen");

// NOTE: We subtract 1 so that we ensure we always go through the slow loop
// and do the caller-specified comparison for at least one byte.

   jit_value_t One = ConstantInt::get( int64Ty_, 1 );

   DPT0( "VVa606: ", VV_VD,
  "RndMaxLen         = Bldr->CreateSub( MaxLen64, One, 'Sub1' )\n" );
   RndMaxLen         = Bldr->CreateSub( MaxLen64, One, "Sub1" );

   DPT0( "VVa607: ", VV_VD,
  "RndMaxLen         = Bldr->CreateUDiv( RndMaxLen, BytesPer, 'UDiv' )\n" );
   RndMaxLen         = Bldr->CreateUDiv( RndMaxLen, BytesPer, "UDiv" );

   DPT0( "VVa608: ", VV_VD,
  "RndMaxLen         = Bldr->CreateMul( RndMaxLen, BytesPer, 'Mul' )\n" );
   RndMaxLen         = Bldr->CreateMul( RndMaxLen, BytesPer, "Mul" );

   DPT0( "VVa610: ", VV_VD,
  "AllocaInst *  ByteCt = Bldr->CreateAlloca( int64Ty_ )\n" );
   AllocaInst *  ByteCt = Bldr->CreateAlloca( int64Ty_ );

   DPT0( "VVa611: ", VV_VD,
  "Bldr->CreateStore( Zero, ByteCt )\n" );
   Bldr->CreateStore( Zero, ByteCt );

   //
   // NOTE: If we don't have at least BytesPer bytes to compare, then
   // we go directly to the slow loop.
   //
   DPT0( "VVa612: ", VV_VD,
  "Is_LT = Bldr->CreateICmpSLT( RndMaxLen, BytesPer, 'ICmp.RndMaxLen' )\n" );
   Is_LT = Bldr->CreateICmpSLT( RndMaxLen, BytesPer, "ICmp.RndMaxLen" );

   DPT0( "VVa613: ", VV_VD,
  "Bldr->CreateCondBr( Is_LT, slowCmpLpLabel, fastCmpLpLabel )\n" );
   Bldr->CreateCondBr( Is_LT, slowCmpLpLabel, fastCmpLpLabel );

   // Start fast loop here

   DPT1( "VVa615: ", VV_VD,
  "ExpFn->getBasicBlockList().push_back( fastCmpLpLabel = %p )\n" , fastCmpLpLabel);
   ExpFn->getBasicBlockList().push_back( fastCmpLpLabel );

   DPT1( "VVa616: ", VV_VD,
  "Bldr->SetInsertPoint( fastCmpLpLabel = %p )\n", fastCmpLpLabel );
   Bldr->SetInsertPoint( fastCmpLpLabel );

   // Copy multiple bytes at a time.
   DPT0( "VVa617: ", VV_VD,
  "ByteCtVal = Bldr->CreateLoad( ByteCt, 'loadByteCt' )\n");
   ByteCtVal = Bldr->CreateLoad( ByteCt, "loadByteCt" ); // Re-load within loop

   DPT0( "VVa618: ", VV_VD,
               "ThisSrc1 = Bldr->CreateGEP( src1Ptr, ByteCtVal, 'ThisSrc1' )\n" );
   jit_value_t  ThisSrc1 = Bldr->CreateGEP( src1Ptr, ByteCtVal, "ThisSrc1" );

   DPT0( "VVa619: ", VV_VD,
               "ThisSrc2 = Bldr->CreateGEP( src2Ptr, ByteCtVal, 'ThisSrc2' )\n" );
   jit_value_t  ThisSrc2 = Bldr->CreateGEP( src2Ptr, ByteCtVal, "ThisSrc2" );

   jit_value_t MultiBytes1 = IR_LoadRelativeWithType( Bldr, ThisSrc1,
                                                      0, int64Ty_ );
   jit_value_t MultiBytes2 = IR_LoadRelativeWithType( Bldr, ThisSrc2,
                                                      0, int64Ty_ );
   // NOTE: Always compare for EQ in the fast loop.
   // The slow loop will do the real comparison -- once we find a difference
   DPT0( "VVa619a: ", VV_VD,
  "IsZero = Bldr->CreateICmpEQ( MultiBytes1, MultiBytes2, 'cmp8Bytes' )\n" );
   IsZero = Bldr->CreateICmpEQ( MultiBytes1, MultiBytes2, "cmp8Bytes" );

   DPT2( "VVa620: ", VV_VD,
  "Bldr->CreateCondBr( IsZero, fastCmpNxtLabel = %p, slowCmpLpLabel = %p )\n",
                               fastCmpNxtLabel, slowCmpLpLabel );
   Bldr->CreateCondBr( IsZero, fastCmpNxtLabel, slowCmpLpLabel );
   
   DPT1( "VVa620a: ", VV_VD,
  "ExpFn->getBasicBlockList().push_back( fastCmpNxtLabel = %p )\n" , fastCmpNxtLabel );
   ExpFn->getBasicBlockList().push_back( fastCmpNxtLabel );

   DPT1( "VVa620b: ", VV_VD,
  "Bldr->SetInsertPoint( fastCmpNxtLabel = %p )\n", fastCmpNxtLabel );
   Bldr->SetInsertPoint( fastCmpNxtLabel );

   // Increment count of bytes copied
   DPT0( "VVa621: ", VV_VD,
  "ByteCtVal = Bldr->CreateAdd( ByteCtVal, BytesPer, 'nextIncr')\n");
   ByteCtVal = Bldr->CreateAdd( ByteCtVal, BytesPer, "nextIncr" );

   DPT0( "VVa622: ", VV_VD,
  "Bldr->CreateStore( ByteCtVal, ByteCt, 'storeByteCt' )\n" );
   Bldr->CreateStore( ByteCtVal, ByteCt, "storeByteCt" );


   // while (ByteCtVal < RndMaxLen) branch to fastLoop
   DPT0( "VVa623: ", VV_VD,
  "Is_LT = Bldr->CreateICmpULT( ByteCtVal, RndMaxLen, 'ICmp.ByteCt' )\n" );
   Is_LT = Bldr->CreateICmpULT( ByteCtVal, RndMaxLen, "ICmp.ByteCt" );

   DPT0( "VVa624: ", VV_VD,
  "Bldr->CreateCondBr( Is_LT, fastCmpLpLabel, slowCmpLpLabel )\n" );
   Bldr->CreateCondBr( Is_LT, fastCmpLpLabel, slowCmpLpLabel );

   // Now compare any remaining bytes (from 1 to 8 bytes possible)

   // NOTE: Because we subtracted 1 from RndMaxLen (above), we are
   // guaranteed that ByteCt < MaxLen64 so we are guaranteed that
   // we *must* go through the slow loop at least once.

   // Start slow loop here

   DPT1( "VVa635: ", VV_VD,
  "ExpFn->getBasicBlockList().push_back( slowCmpLpLabel = %p )\n", slowCmpLpLabel );
   ExpFn->getBasicBlockList().push_back( slowCmpLpLabel );

   DPT1( "VVa636: ", VV_VD,
  "Bldr->SetInsertPoint( slowCmpLpLabel = %p )\n", slowCmpLpLabel );
   Bldr->SetInsertPoint( slowCmpLpLabel );

   bytesAtOnce = 1;

   BytesPer    = ConstantInt::get( int64Ty_, bytesAtOnce );

   // Copy one byte at a time.
   DPT0( "VVa637: ", VV_VD,
  "ByteCtVal = Bldr->CreateLoad( ByteCt, 'loadByteCt' )\n");
   ByteCtVal = Bldr->CreateLoad( ByteCt, "loadByteCt" ); // Re-load within the loop

   DPT0( "VVa638: ", VV_VD,
               "CurrSrc1 = Bldr->CreateGEP( src1Ptr, ByteCtVal, 'CurrSrc1' )\n" );
   jit_value_t  CurrSrc1 = Bldr->CreateGEP( src1Ptr, ByteCtVal, "CurrSrc1" );

   DPT0( "VVa639: ", VV_VD,
               "CurrSrc2 = Bldr->CreateGEP( src2Ptr, ByteCtVal, 'CurrSrc2' )\n" );
   jit_value_t  CurrSrc2 = Bldr->CreateGEP( src2Ptr, ByteCtVal, "CurrSrc2" );

   jit_value_t OneByte1 = IR_LoadRelativeWithType( Bldr, CurrSrc1,
                                                   0, int8Ty_ );
   jit_value_t OneByte2 = IR_LoadRelativeWithType( Bldr, CurrSrc2,
                                                   0, int8Ty_ );
   switch ( KindOfCmp )
   {
      default:
      case ByteCompare_EQ:
         DPT0( "VVa640eq: ", VV_VD,
        "IsZero = Bldr->CreateICmpEQ( OneByte1, OneByte2, 'cmp1ByteEQ' )\n" );
         IsZero = Bldr->CreateICmpEQ( OneByte1, OneByte2, "cmp1ByteEQ" );
         break;
      case ByteCompare_NE:
         DPT0( "VVa640ne: ", VV_VD,
        "IsZero = Bldr->CreateICmpNE( OneByte1, OneByte2, 'cmp1ByteNE' )\n" );
         IsZero = Bldr->CreateICmpNE( OneByte1, OneByte2, "cmp1ByteNE" );
         break;
      case ByteCompare_LT:
         DPT0( "VVa640ult: ", VV_VD,
        "IsZero = Bldr->CreateICmpULT( OneByte1, OneByte2, 'cmp1ByteLT' )\n" );
         IsZero = Bldr->CreateICmpULT( OneByte1, OneByte2, "cmp1ByteLT" );
         break;
      case ByteCompare_LE:
         DPT0( "VVa640ule: ", VV_VD,
        "IsZero = Bldr->CreateICmpULE( OneByte1, OneByte2, 'cmp1ByteLE' )\n" );
         IsZero = Bldr->CreateICmpULE( OneByte1, OneByte2, "cmp1ByteLE" );
         break;
      case ByteCompare_GT:
         DPT0( "VVa640ugt: ", VV_VD,
        "IsZero = Bldr->CreateICmpUGT( OneByte1, OneByte2, 'cmp1ByteGT' )\n" );
         IsZero = Bldr->CreateICmpUGT( OneByte1, OneByte2, "cmp1ByteGT" );
         break;
      case ByteCompare_GE:
         DPT0( "VVa640uge: ", VV_VD,
        "IsZero = Bldr->CreateICmpUGE( OneByte1, OneByte2, 'cmp1ByteGE' )\n" );
         IsZero = Bldr->CreateICmpUGE( OneByte1, OneByte2, "cmp1ByteGE" );
         break;
   }

   DPT0( "VVa641: ", VV_VD,
  "IsEqual = Bldr->CreateICmpEQ( OneByte1, OneByte2, 'cmp1ByteEQ' )" );
   IsEqual = Bldr->CreateICmpEQ( OneByte1, OneByte2, "cmp1ByteEQ" );

   DPT2( "VVa642: ", VV_VD,
  "Bldr->CreateCondBr( IsEqual, slowContLabel = %p, slowCmpEndLabel = %p )\n",
                                slowContLabel, slowCmpEndLabel );
   Bldr->CreateCondBr( IsEqual, slowContLabel, slowCmpEndLabel );
   
   DPT1( "VVa642a: ", VV_VD,
  "ExpFn->getBasicBlockList().push_back( slowContLabel = %p )\n" , slowContLabel );
   ExpFn->getBasicBlockList().push_back( slowContLabel );

   DPT1( "VVa642b: ", VV_VD,
  "Bldr->SetInsertPoint( slowContLabel = %p )\n", slowContLabel );
   Bldr->SetInsertPoint( slowContLabel );

   // Increment loop count
   DPT0( "VVa643: ", VV_VD,
  "ByteCtVal = Bldr->CreateAdd( ByteCtVal, BytesPer, 'nextIncr')\n");
   ByteCtVal = Bldr->CreateAdd( ByteCtVal, BytesPer, "nextIncr" );

   DPT0( "VVa644: ", VV_VD,
  "Bldr->CreateStore( ByteCtVal, ByteCt, 'storeByteCt' )\n" );
   Bldr->CreateStore( ByteCtVal, ByteCt, "storeByteCt" );


   // while (ByteCtVal < MaxLen64) branch to slowLoop
   DPT0( "VVa645: ", VV_VD,
  "Is_LT = Bldr->CreateICmpULT( ByteCtVal, MaxLen64, 'ICmp.ByteCt' )\n" );
   Is_LT = Bldr->CreateICmpULT( ByteCtVal, MaxLen64, "ICmp.ByteCt" );

   DPT0( "VVa646: ", VV_VD,
  "Bldr->CreateCondBr( Is_LT, slowCmpLpLabel, slowCmpEndLabel )\n" );
   Bldr->CreateCondBr( Is_LT, slowCmpLpLabel, slowCmpEndLabel );

   DPT1( "VVa647: ", VV_VD,
  "ExpFn->getBasicBlockList().push_back( slowCmpEndLabel = %p )\n", slowCmpEndLabel );
   ExpFn->getBasicBlockList().push_back( slowCmpEndLabel );

   DPT1( "VVa648: ", VV_VD,
  "Bldr->SetInsertPoint( slowCmpEndLabel = %p )\n", slowCmpEndLabel );
   Bldr->SetInsertPoint( slowCmpEndLabel );

   return ( IsZero );
}

//
// allocJitValue() - tell LLVM to allocate a location (on the stack)
// to hold a temporary value.  NOTE: Most of these are allocated
// just *before* we start generating native code for PCODE instructions.
// By doing it early, we ensure that all paths through the PCODE
// instructions will use the same allocation location for the 
// specified operand.
//
void PCodeOperand::allocJitValue( PCodeCfg            * cfg
                                , IRBldr_t            * Bldr
                                , PCIT::AddressingMode  OprTyp
                                , Int32                 Len
                                , Int32                 Num
                                )
{
#if NExprDbgLvl >= VV_NO
      char  NExBuf[500];
#endif
      // Get operand in hash table
      CollIndex i = bvIndex_;
      PCodeOperand* operand = cfg->getMap()->getFirstValue(&i);

      CDPT2( "VV0647: ", VV_BD, "In allocJitValue(): operand=%p, this=%p\n",
                                                     operand,    this );
      if (operand != this)
         return operand->allocJitValue( cfg, Bldr, OprTyp, Len, Num );

      if ( jitValue_ ) return;  // Already allocated 

      jit_type_t  IRtype = getJitType( cfg, OprTyp );
      if ( IRtype == cfg->getInt1PtrTy() )
      {
         if ( Len = 2 ) IRtype = cfg->getInt16PtrTy() ;
         if ( Len = 4 ) IRtype = cfg->getInt32PtrTy() ;
         if ( Len = 8 ) IRtype = cfg->getInt64PtrTy() ;
      }
      CDPT2( "VV0647a: ", VV_BD, "In allocJitValue(): Len = %d, IRtype = %p\n",
                                                      Len,      IRtype );

      jit_value_t NumV   = ConstantInt::get( cfg->getInt32Ty() , Num );

      CDPT3( "VV0648: ", VV_BD,
     "jitValue_  = (llvm::Value *)Bldr->CreateAlloca( IRtype = %p, NumV = %d )"
                                  " for PCOp = %p\n", IRtype,      Num, this );
      jitValue_  = (llvm::Value *)Bldr->CreateAlloca( IRtype , NumV );

      CDPT1( "VV0649: ", VV_BD, "Just set jitValue_ = %p (ptr to AllocaInst)\n",
                                            jitValue_ );
}
#endif /* NA_LINUX_LLVMJIT */

#ifdef NA_LINUX_LIBJIT

// FIX: Overlapped types can have problems too
jit_value_t PCodeOperand::getJitValue(PCodeCfg* cfg,
                                      jit_function_t f,
                                      jit_type_t type,
                                      PCodeBlock* b,
                                      PCodeOperand* orig = NULL,
                                      NABoolean noSave = FALSE)
{
  // Get operand in hash table
  CollIndex i = bvIndex_;
  PCodeOperand* operand = cfg->getMap()->getFirstValue(&i);
  if (operand != this)
    return operand->getJitValue(cfg, f, type, b, this, noSave);

  // Make sure we properly indicate this this object *is* the original one.
  if (orig == NULL)
    orig = this;

  //
  // Get pointer to operand, if that's what is needed
  // 

  if (type == jit_type_void_ptr) {

    // If already defined somewhere, perhaps it can be reused?
    if (jitValuePtr_) {
      assert(jitValueBlock_ != NULL);
      if (b->onesVector.testBit(jitValueBlock_->getBlockNum()))
        return jitValuePtr_;
      else
        jitValuePtr_ = NULL;
    }

    // Non-native temps will be accessed directly from the passed in temps
    // area.  If that temp was previously defined as a jit value (e.g. size of
    // temp fit native storage container), then we skip to the else clause so
    // the address is simply taken for the jit value.
    if (isVar() || isConst() || (!jitValue_ && orig->isNonNativeTemp())) {
      jit_value_t parm1 = cfg->getJitParams()[stackIndex_];

      // For varchars we return a pointer to the start of length indicator.
      if (getType() == PCIT::MATTR5) {
        // If we're not dealing with a varchar, or if it is the first varchar,
        // we don't need to go through the VOA offset.
        if (!isVarchar() || (getVoaOffset() == -1)) {
          jitValuePtr_ =
            jit_insn_add_relative(f, param, (offset_ - vcIndicatorLen_));
        }
        else
        {
          jit_type_t t = ((vcIndicatorLen_ == 2)
                           ? jit_type_ushort : jit_type_uint);

          jit_value_t v = jit_insn_load_relative(f, param, voaOffset_, t);

          jitValuePtr_ = jit_insn_add(f, param, v);

          // Now set up pointer, moving past null indicator if required.
          if (vcNullIndicatorLen_)
            jitValuePtr_ = jit_insn_add_relative(f, jitValuePtr_,
                                                 vcNullIndicatorLen_);
        }
      }
      else
        jitValuePtr_ = jit_insn_add_relative(f, param, offset_);
    }
    else {
      // Normally we would force the jitValue_ to be defined if we're attempting
      // to get it.  However, in some cases (like FILL_MEM_BYTES) we make a 
      // a call to get it first so that things like the vc length can be written
      // to it first.
      // FIX: fix this so that we can continue to assert that jitValue_ should
      //      be defined first.  Fixing it involves having storeJitValue support
      //      all possible stores (vc lengths, actual values, etc.).

      // Must be called and it must already be defined

      if (!jitValue_) {
        // We reached a situation that we can't support.  For example, we could
        // having something like this:
        //
        // B1:  [2,0] = ...
        //      Branch B3
        // B2:  [2,0] = ...
        // B3:    = [2,0]
        //
        // If we allocated space for [2,0] in B1, and then do the same in B2,
        // which one do we use for B3?  Also, if we try to allocate space for
        // [2,0] in B3, then it would be completely wrong.

        cfg->setJitFailureSeen();

        // And for the sake of nothing else failing along the way, create a
        // bogus value to return.
        return jit_value_create(f, jit_type_int);
      }

      assert (jitValue_ && isTemp());

      // Temporary value needs to be addressable.
      jit_value_set_addressable(jitValue_);

      // Return the address of this temp variable.
      jitValuePtr_ = jit_insn_address_of(f, jitValue_);

      // If the jitValue_ was not defined in this current block, then the safe
      // thing to do is simply return the address of jitValue, but *do not* 
      // save the pointer in jitValuePtr_, since that will lead to confusion
      // on what was defined where.
      if (jitValueBlock_ != b) {
        jit_value_t temp = jitValuePtr_;
        jitValuePtr_ = NULL;
        return temp;
      }
    }

    jitValueBlock_ = b;

    // If we are directly told not to save the jit values, don't!
    if (noSave) {
      jit_value_t tempVal = jitValuePtr_;
      jitValueBlock_ = NULL;
      jitValue_ = jitValuePtr_ = NULL;
      return tempVal;
    }

    return jitValuePtr_;
  }

  //
  // Get value of operand
  //

  // If already defined somewhere, perhaps it can be reused?  If it's a temp,
  // however, then the assumption is that all uses have a def which reach it,
  // and that def is available in jitValue_.
  if (jitValue_) {
    assert(jitValueBlock_ != NULL);
    if (isTemp() || b->onesVector.testBit(jitValueBlock_->getBlockNum()))
    {
      // It's possible that we have some overlapping operands.  The following
      // deals with it (or at least for the majority of the cases where we see
      // an overlap :()
      jit_type_t jitValType = jit_value_get_type(jitValue_);
      if (jit_type_get_size(jitValType) > jit_type_get_size(type))
        return jit_insn_convert(f, jitValue_, type, 0);

      return jitValue_;
    }

    jitValue_ = NULL;
  }

  if (isConst()) {
    //
    // FIX: the assumption right now is that only integers are considered for
    // native expressions.  That will change shortly.  In the meantime, however,
    // this implies that getIntConstValue() should be called on the operand to
    // retrieve the constant value.
    //
    // FIX: getIntConstValue() should probably be "relaxed" a little so as to
    // support MPTR32 and MFLT (both of which may be casted into integer types
    // during same-sized moves).  For now, work around by adding logic here.
    //

    if (type == jit_type_long) {
      // MFLT64 can sometimes be the original type of the operand being sought
      // after (e.g. PCIT::MFLT64_MFLT64 --> PCIT::MBIN64S_MBIN64S).  Use the
      // original operand to get the constant value.

      PCodeOperand* op = ((getType() == PCIT::MFLT64) ||
                          (getType() == PCIT::MFLT32))
                            ? orig : this;

      Int64 val = cfg->getIntConstValue(op);

      jitValue_ = jit_value_create_long_constant(f, type, val);
    }
    else if (getType() == PCIT::MPTR32) {
      // MPTR32 can sometimes be the original type of the operand being sought
      // after (e.g. PCIT::MBIN8_MBIN8_IBIN32S --> PCIT::MBIN8_MBIN8).  Use the
      // original operand to get the constant value.

      Int32 val = (Int32)cfg->getIntConstValue(((orig) ? orig : this));
      jitValue_ = jit_value_create_nint_constant(f, type, val);
    }
    else if (type == jit_type_float64) {
      // Unless the value is "1.0", libjit will store a double constant in
      // memory, referenced via %ip, and that will blow us up at runtime.  Get
      // it from memory instead (if not 1.0)
      double val = cfg->getFloatConstValue(((orig) ? orig : this));
      if (val == 1.0)
        jitValue_ = jit_value_create_float64_constant(f, type, val);
      else {
        jit_value_t param = cfg->getJitParams()[stackIndex_];
        jitValue_ = jit_insn_load_relative(f, param, offset_, type);
      }
    }
    else {
      Int32 val = (Int32)cfg->getIntConstValue(((orig) ? orig : this));
      jitValue_ = jit_value_create_nint_constant(f, type, val);
    }
  }
  else if (isVar()) {
    jit_value_t param = cfg->getJitParams()[stackIndex_];

    // Are we getting the null bit?
    if (nullBitIndex_ != -1) {
      // For paranoia, make sure offset_ is not -1 also (indicating packed
      // format against a varchar, which is not supported yet).
      assert(offset_ != -1);

      jit_value_t mask = jit_value_create_nint_constant(f, jit_type_ubyte,
                           (UInt8)0x1 << (7 - (nullBitIndex_ & 7)));

      jit_value_t nByte = jit_insn_load_relative(f, param,
                            offset_ + (nullBitIndex_ >> 3), jit_type_ubyte);
      jitValue_ = jit_insn_and(f, nByte, mask);
    }
    else
      jitValue_ = jit_insn_load_relative(f, param, offset_, type);
  }
  else {
    // Accessing an undefined temp is not allowed.
    assert (FALSE);
  }

  jitValueBlock_ = b;

  // If we are directly told not to save the jit values, don't!
  if (noSave) {
    jit_value_t tempVal = jitValue_;
    jitValueBlock_ = NULL;
    jitValue_ = jitValuePtr_ = NULL;
    return tempVal;
  }

  return jitValue_;
}

//
// 1. Get string length of varchar and return to caller.
// 2. Move pointer past string length field.
// 3. If asked to, resize length by subtracting off padding
//
jit_value_t PCodeCfg::genStringSetup(jit_function_t f,
                                      jit_value_t* tStr,
                                      PCodeOperand* src,
                                      NABoolean padExists = FALSE,
                                      NABoolean lenNeeded = TRUE)
{
  jit_value_t lenJitVal = NULL;

  Int32 vcIndLen = src->getVcIndicatorLen();

  // If pad exists, then the length must surely be retrieved.
  if (padExists)
    lenNeeded = TRUE;

  if (vcIndLen <= 0) {
    // For fixed chars, just record the max length, if length is needed
    if (lenNeeded) {
      jit_value_t max;
      max = jit_value_create_nint_constant(f, jit_type_uint, src->getLen());

      lenJitVal = jit_value_create(f, jit_type_uint);
      jit_insn_store(f, lenJitVal, max);
    }
  }
  else
  {
    // Only get length if needed - which is most of the time.
    if (lenNeeded) {
      if (vcIndLen == 2)
        lenJitVal = jit_insn_load_relative(f, *tStr, 0, jit_type_ushort);
      else
        lenJitVal = jit_insn_load_relative(f, *tStr, 0, jit_type_uint);
    }

    *tStr = jit_insn_add_relative(f, *tStr, vcIndLen);
  }

  // If padding exists that needs to be factored into the length, computet it
  // now with a loop.

  if (padExists) {
    jit_value_t temp1, temp2, temp3;
    jit_value_t spaceJitVal =
      jit_value_create_nint_constant(f, jit_type_ubyte, ' ');

    jit_label_t padStartLabel = jit_label_undefined;
    jit_label_t padEndLabel = jit_label_undefined;

    jit_insn_label(f, &padStartLabel);
    jit_insn_branch_if_not(f, lenJitVal, &padEndLabel);

    temp1 = jit_insn_sub(f, lenJitVal, oneJitVal_);
    temp2 = jit_insn_load_elem(f, *tStr, temp1, jit_type_ubyte);
    temp3 = jit_insn_eq(f, temp2, spaceJitVal);
    jit_insn_branch_if_not(f, temp3, &padEndLabel);

    jit_insn_store(f, lenJitVal, temp1);
    jit_insn_branch(f, &padStartLabel);

    jit_insn_label(f, &padEndLabel);
  }

  return lenJitVal;
}

jit_value_t PCodeCfg::genHash(jit_function_t f,
                              jit_value_t tStr,
                              jit_value_t tStrLen,
                              jit_value_t loopIndex,
                              Int32 constantLen = 0)
{
  // If constantLen is provided (i.e. tStrLen is NULL) then that implies that we
  // are hashing a standard-sized column.  Otherwise we are hashing some kind've
  // string.

  Int32 j;

  jit_value_t t1, t2, t3, t4, t5, t6, resJitVal;
  jit_label_t startLoop = jit_label_undefined;
  jit_label_t endLoop = jit_label_undefined;

  jit_value_t shiftRVal = jit_value_create_nint_constant(f, jit_type_int, 31);

  // In an effort to reduce space, generate loop for hashing fixed-length
  // strings greater than a certain length.
  if ((tStrLen == NULL) && (constantLen > 32))
    tStrLen = jit_value_create_nint_constant(f, jit_type_int, constantLen);

  if (tStrLen == NULL) {
    t1 = jit_insn_load_relative(f, tStr, 0, jit_type_ubyte);
    resJitVal = jit_insn_load_elem(f, hashTableJitVal_, t1, jit_type_uint);

    // Unroll hash using known length
    for (j=1; j < constantLen; j++)
    {
      //
      // Compute: t1 = src[j]
      //          t2 = randomHashValues[t1]
      //          t3 = resJitVal << 1
      //          t4 = resJitVal >> 31
      //          t5 = (t3 | t4)
      //    resJitVal = t5 ^ resJitVal
      //
      t1 = jit_insn_load_relative(f, tStr, j, jit_type_ubyte);
      t2 = jit_insn_load_elem(f, hashTableJitVal_, t1, jit_type_uint);
      t3 = jit_insn_shl(f, resJitVal, oneJitVal_);
      t4 = jit_insn_ushr(f, resJitVal, shiftRVal);
      t5 = jit_insn_or(f, t3, t4);
      resJitVal = jit_insn_xor(f, t5, t2);
    }

    return resJitVal;
  }

  // Create temp var to hold result of hashing string.  Temp space needs to be
  // allocated since it can have, by default, the value 0 (if length is 0)
  resJitVal = jit_value_create(f, jit_type_uint);

  jit_insn_store(f, resJitVal, zeroJitVal_);
  jit_insn_store(f, loopIndex, tStrLen);

  // Loop start
  jit_insn_label(f, &startLoop);

  // Branch to end label if loopIndex <= 0
  t1 = jit_insn_le(f, loopIndex, zeroJitVal_);
  jit_insn_branch_if(f, t1, &endLoop);

  //
  // Compute: t1 = src[i]
  //          t2 = randomHashValues[t1]
  //          t3 = resJitVal << 1
  //          t4 = resJitVal >> 31
  //          t5 = (t3 | t4)
  //    resJitVal = t5 ^ resJitVal
  //
  t1 = jit_insn_load_relative(f, tStr, 0, jit_type_ubyte);
  t2 = jit_insn_load_elem(f, hashTableJitVal_, t1, jit_type_uint);
  t3 = jit_insn_shl(f, resJitVal, oneJitVal_);
  t4 = jit_insn_ushr(f, resJitVal, shiftRVal);
  t5 = jit_insn_or(f, t3, t4);
  t6 = jit_insn_xor(f, t5, t2);
  jit_insn_store(f, resJitVal, t6);

  // Decrement loop index and store
  t1 = jit_insn_sub(f, loopIndex, oneJitVal_);
  jit_insn_store(f, loopIndex, t1);

  // Increment pointer and store
  t2 = jit_insn_add(f, tStr, oneJitVal_);
  jit_insn_store(f, tStr, t2);

  // Branch back to head of loop
  jit_insn_branch(f, &startLoop);

  // End label
  jit_insn_label(f, &endLoop);

  return resJitVal;
}
#endif /* NA_LINUX_LIBJIT */

#if 1 /* NA_LINUX_LLVMJIT */

//
// genMemSetLoop() - generate code to set every element in an long array
// to the same value.
//
void PCodeCfg::genMemSetLoop( IRBldr_t   * Bldr
                            , Int32        val
                            , jit_value_t  tgtPtr
                            , jit_value_t  maxLen
                            )
{
#if NExprDbgLvl > VV_NO
   char  NExBuf[500];
#endif
   UInt8  val1 = (UInt8)val;
   UInt16 val2 = (val1 << 8) | val1;
   UInt32 val4 = (val2 << 16) | val2;
   UInt64 val8 = val4 ;               //Note: if we use (val4 << 32) we get C++ error
          val8 = (val8 << 32) | val8;

   jit_value_t MultiBytes = ConstantInt::get( int64Ty_ , val8 );
   jit_value_t OneByte    = ConstantInt::get( int8Ty_ , val1 );

   jit_value_t  BytesPer, CmpRslt ;
   jit_value_t  RndMaxLen, ByteCtVal ;
   jit_value_t  MaxLen64 ;

   DPT0( "VV0700: ", VV_BD, "GOT into genMemSetLoop()\n" );

   jit_value_t  Zero  = ConstantInt::get( int64Ty_ , 0 );

   llvm::LLVMContext &LLContxt = Bldr->getContext();
   llvm::Function *ExpFn       = Bldr->GetInsertBlock()->getParent();

   p_IR_block_t fastLoopLabel = BasicBlock::Create( LLContxt, "MemSetFastBlk" );
   p_IR_block_t endLabel      = BasicBlock::Create( LLContxt, "MemSetEndBlk" );

   DPT2( "VV0702: ", VV_VD, "fastLoopLabel = %p, endLabel = %p\n",
                             fastLoopLabel,      endLabel ); 

   Int32 bytesAtOnce = 8;

   BytesPer  = ConstantInt::get( int64Ty_ , bytesAtOnce );
   DPT0( "VV0705: ", VV_VD,
  "MaxLen64  = Bldr->CreateZExt( maxLen, int64Ty_ , 'ExtMaxLen' )\n" );
   MaxLen64  = Bldr->CreateZExt( maxLen, int64Ty_ , "ExtMaxLen" );

   DPT0( "VV0706: ", VV_VD,
  "RndMaxLen = Bldr->CreateUDiv( MaxLen64, BytesPer, 'UDiv' )\n" );
   RndMaxLen = Bldr->CreateUDiv( MaxLen64, BytesPer, "UDiv" );

   DPT0( "VV0707: ", VV_VD,
  "RndMaxLen = Bldr->CreateMul( RndMaxLen, BytesPer, 'Mul' )\n" );
   RndMaxLen = Bldr->CreateMul( RndMaxLen, BytesPer, "Mul" );

   DPT0( "VV0710: ", VV_VD,
  "AllocaInst * ByteCt = Bldr->CreateAlloca( int64Ty_ )\n" );
   AllocaInst * ByteCt = Bldr->CreateAlloca( int64Ty_ );

   DPT0( "VV0711: ", VV_VD,
  "Bldr->CreateStore( Zero, ByteCt )\n" );
   Bldr->CreateStore( Zero, ByteCt );

   DPT0( "VV0712: ", VV_VD,
  "CmpRslt = Bldr->CreateICmpULT( RndMaxLen, BytesPer, 'ICmp.RndMaxLen' )\n" );
   CmpRslt = Bldr->CreateICmpULT( RndMaxLen, BytesPer, "ICmp.RndMaxLen" );

   DPT0( "VV0713: ", VV_VD,
  "Bldr->CreateCondBr( CmpRslt, endLabel, fastLoopLabel )\n" );
   Bldr->CreateCondBr( CmpRslt, endLabel, fastLoopLabel );

   // Start fast loop here

   DPT1( "VV0715: ", VV_VD,
  "ExpFn->getBasicBlockList().push_back( fastLoopLabel = %p )\n" , fastLoopLabel );
   ExpFn->getBasicBlockList().push_back( fastLoopLabel );

   DPT0( "VV0716: ", VV_VD,
  "Bldr->SetInsertPoint( fastLoopLabel )\n" );
   Bldr->SetInsertPoint( fastLoopLabel );

   // Copy multiple bytes at a time.
   DPT0( "VV0718: ", VV_VD,
  "ByteCtVal = Bldr->CreateLoad( ByteCt, 'loadByteCt' )\n");
   ByteCtVal = Bldr->CreateLoad( ByteCt, "loadByteCt" ); // Re-load within the loop

   DPT0( "VV0719: ", VV_VD,
               "ThisTgt = Bldr->CreateGEP( tgtPtr, ByteCtVal, 'ThisTgt' )\n" );
   jit_value_t  ThisTgt = Bldr->CreateGEP( tgtPtr, ByteCtVal, "ThisTgt" );

   IR_StoreRelativeWithType( Bldr, MultiBytes, ThisTgt,
                                                     0, int64Ty_ );

   // Increment count of bytes handled
   DPT0( "VV0721: ", VV_VD,
  "ByteCtVal = Bldr->CreateAdd( ByteCtVal, BytesPer, 'nextIncr')\n");
   ByteCtVal = Bldr->CreateAdd( ByteCtVal, BytesPer, "nextIncr" );

   DPT0( "VV0722: ", VV_VD,
  "Bldr->CreateStore( ByteCtVal, ByteCt, 'storeByteCt' )\n" );
   Bldr->CreateStore( ByteCtVal, ByteCt, "storeByteCt" );

   // while (ByteCtVal < RndMaxLen) branch to fastLoop
   DPT0( "VV0723: ", VV_VD,
  "CmpRslt = Bldr->CreateICmpULT( ByteCtVal, RndMaxLen, 'ICmp.ByteCt' )\n" );
   CmpRslt = Bldr->CreateICmpULT( ByteCtVal, RndMaxLen, "ICmp.ByteCt" );

   DPT0( "VV0724: ", VV_VD,
  "Bldr->CreateCondBr( CmpRslt, fastLoopLabel, endLabel )\n" );
   Bldr->CreateCondBr( CmpRslt, fastLoopLabel, endLabel );

   DPT1( "VV0725: ", VV_VD,
  "ExpFn->getBasicBlockList().push_back( endLabel = %p )\n" , endLabel );
   ExpFn->getBasicBlockList().push_back( endLabel );

   DPT0( "VV0726: ", VV_VD,
  "Bldr->SetInsertPoint( endLabel )\n" );
   Bldr->SetInsertPoint( endLabel );

   // Now copy any remaining bytes (from 0 to 7 bytes are possible)

   p_IR_block_t slowLoopLabel = BasicBlock::Create( LLContxt, "MemSetSlowBlk" );
   p_IR_block_t end2Label     = BasicBlock::Create( LLContxt, "MemSetEndBlk" );

   DPT2( "VV0728: ", VV_VD, "slowLoopLabel = %p, end2Label = %p\n",
                             slowLoopLabel,      end2Label ); 

   bytesAtOnce = 1;

   BytesPer  = ConstantInt::get( int64Ty_ , bytesAtOnce );

   DPT0( "VV0730: ", VV_VD,
  "ByteCtVal = Bldr->CreateLoad( ByteCt, 'loadByteCt' )\n");
   ByteCtVal = Bldr->CreateLoad( ByteCt, "loadByteCt" );

   DPT0( "VV0732: ", VV_VD,
  "CmpRslt = Bldr->CreateICmpUGE( ByteCtVal, MaxLen64, 'ICmp.RndMaxLen' )\n" );
   CmpRslt = Bldr->CreateICmpUGE( ByteCtVal, MaxLen64, "ICmp.RndMaxLen" );

   DPT0( "VV0733: ", VV_VD,
  "Bldr->CreateCondBr( CmpRslt, end2Label, slowLoopLabel )\n" );
   Bldr->CreateCondBr( CmpRslt, end2Label, slowLoopLabel );

   // Start slow loop here

   DPT1( "VV0735: ", VV_VD,
  "ExpFn->getBasicBlockList().push_back( slowLoopLabel = %p )\n", slowLoopLabel  );
   ExpFn->getBasicBlockList().push_back( slowLoopLabel );

   DPT0( "VV0736: ", VV_VD,
  "Bldr->SetInsertPoint( slowLoopLabel )\n" );
   Bldr->SetInsertPoint( slowLoopLabel );

   // Copy multiple bytes at a time.
   DPT0( "VV0738: ", VV_VD,
  "ByteCtVal = Bldr->CreateLoad( ByteCt, 'loadByteCt' )\n");
   ByteCtVal = Bldr->CreateLoad( ByteCt, "loadByteCt" ); // Re-load within the loop

   DPT0( "VV0739: ", VV_VD,
               "CurrTgt = Bldr->CreateGEP( tgtPtr, ByteCtVal, 'CurrTgt' )\n" );
   jit_value_t  CurrTgt = Bldr->CreateGEP( tgtPtr, ByteCtVal, "CurrTgt" );

   IR_StoreRelativeWithType( Bldr, OneByte, CurrTgt,
                                                  0, int8Ty_ );

   // Increment count of bytes handled
   DPT0( "VV0740: ", VV_VD,
  "ByteCtVal = Bldr->CreateAdd( ByteCtVal, BytesPer, 'nextIncr')\n");
   ByteCtVal = Bldr->CreateAdd( ByteCtVal, BytesPer, "nextIncr" );

   DPT0( "VV0742: ", VV_VD,
  "Bldr->CreateStore( ByteCtVal, ByteCt, 'storeByteCt' )\n" );
   Bldr->CreateStore( ByteCtVal, ByteCt, "storeByteCt" );


   // while (ByteCtVal < MaxLen64) branch to slowLoop
   DPT0( "VV0743: ", VV_VD,
  "CmpRslt = Bldr->CreateICmpULT( ByteCtVal, MaxLen64, 'ICmp.ByteCt' )\n" );
   CmpRslt = Bldr->CreateICmpULT( ByteCtVal, MaxLen64, "ICmp.ByteCt" );

   DPT0( "VV0744: ", VV_VD,
  "Bldr->CreateCondBr( CmpRslt, slowLoopLabel, end2Label )\n" );
   Bldr->CreateCondBr( CmpRslt, slowLoopLabel, end2Label );

   DPT1( "VV0745: ", VV_VD,
  "ExpFn->getBasicBlockList().push_back( end2Label = %p )\n", end2Label );
   ExpFn->getBasicBlockList().push_back( end2Label );

   DPT0( "VV0746: ", VV_VD,
  "Bldr->SetInsertPoint( end2Label )\n" );
   Bldr->SetInsertPoint( end2Label );
}

void PCodeCfg::genUnalignedMemset( IRBldr_t   * Bldr
                                 , jit_value_t  tgtPtr
                                 , Int32        val
                                 , Int32        length
                                 )
{
#if NExprDbgLvl > VV_NO
  char  NExBuf[500];
#endif
  Int32 i;
  UInt8  val1 = (UInt8)val;
  UInt16 val2 = (val1 << 8)  | val1;
  UInt32 val4 = (val2 << 16) | val2;
  UInt64 val8 = val4 ;               //Note: if we use (val4 << 32) we get C++ error
         val8 = (val8 << 32) | val8;

  if ( length < 32 )
  {
     // First try multiples of 8 
//// jit_value_t jVal8 = jit_value_create_nint_constant(f, jit_type_uint, val8);
     jit_value_t jVal8 = ConstantInt::get( int64Ty_, val8 );

     for (i=0; length >= 8; i+=8, length-=8) {
////   jit_insn_store_relative(f, Tgt, i, jVal8);

        DPT0( "VV0750: ", VV_VD,
       "IR_StoreRelativeWithType( Bldr, jVal8, tgtPtr, i, int64Ty_ )\n" );
        IR_StoreRelativeWithType( Bldr, jVal8, tgtPtr, i, int64Ty_ );
     }

     // Then try multiples of 4 
//// jit_value_t jVal4 = jit_value_create_nint_constant(f, jit_type_uint, val4);
     jit_value_t jVal4 = ConstantInt::get( int32Ty_, val4 );

     for (i=0; length >= 4; i+=4, length-=4) {
////   jit_insn_store_relative(f, Tgt, i, jVal4);

        DPT0( "VV0760: ", VV_VD,
       "IR_StoreRelativeWithType( Bldr, jVal4, tgtPtr, i, int32Ty_ )\n" );
        IR_StoreRelativeWithType( Bldr, jVal4, tgtPtr, i, int32Ty_ );
     }

     // Then try multiples of 2
//// jit_value_t jVal2 = jit_value_create_nint_constant(f, jit_type_ushort, val2);
     jit_value_t jVal2 = ConstantInt::get( int16Ty_, val2 );

     for (; length >= 2; i+=2, length-=2) {
////   jit_insn_store_relative(f, Tgt, i, jVal2);

        DPT0( "VV0770: ", VV_VD,
       "IR_StoreRelativeWithType( Bldr, jVal2, tgtPtr, i, int16Ty_ )\n" );
        IR_StoreRelativeWithType( Bldr, jVal2, tgtPtr, i, int16Ty_ );
     }

     // Lastly copy 1 byte at a time
//// jit_value_t jVal1 = jit_value_create_nint_constant(f, jit_type_ubyte, val1);
     jit_value_t jVal1 = ConstantInt::get( int8Ty_, val1 );

     for (; length >= 1; i+=1, length-=1) {
////   jit_insn_store_relative(f, Tgt, i, jVal1);

        DPT0( "VV0780: ", VV_VD,
       "IR_StoreRelativeWithType( Bldr, jVal1, tgtPtr, i, int8Ty_ )\n" );
        IR_StoreRelativeWithType( Bldr, jVal1, tgtPtr, i, int8Ty_ );
     }

  } // End: if ( length < 32 )

  else // long, fixed-length string ... use a loop instead of voluminous instruction generation
  {
      jit_value_t ConstLen = ConstantInt::get( int64Ty_, length );

      genMemSetLoop( Bldr, val, tgtPtr, ConstLen );
  }

  return;
}
#endif /* NA_LINUX_LLVMJIT */

#ifdef NA_LINUX_LIBJIT

void PCodeCfg::genUnalignedMemset(jit_function_t f,
                                  jit_value_t tgt,
                                  Int32 val,
                                  Int32 length)
{
  Int32 i;

#if 0
  // For lengths <= 32, jit_insn_memset can be called to generate the code.
  // Anything greater and an intrinsic call is made.  UPDATE: This is true only
  // when jit_optimization_level is 0.  If the optimization level is higher,
  // then the cutoff is 16 (not 32).  The best thing is to generate the code
  // ourselves.
  if (length <= 32) {
    jit_value_t jVal = jit_value_create_nint_constant(f, jit_type_int, val);
    jit_value_t jLen = jit_value_create_nint_constant(f, jit_type_int, length);

    jit_insn_memset(f, tgt, jVal, jLen);

    return;
  }
#endif

  UInt8 val1 = (UInt8)val;
  UInt16 val2 = (val1 << 8) | val1;
  UInt32 val4 = (val2 << 16) | val2;

  // First try multiples of 4 
  jit_value_t jVal4 = jit_value_create_nint_constant(f, jit_type_uint, val4);
  for (i=0; length >= 4; i+=4, length-=4) {
    jit_insn_store_relative(f, tgt, i, jVal4);
  }

  // Then try multiples of 2
  jit_value_t jVal2 = jit_value_create_nint_constant(f, jit_type_ushort, val2);
  for (; length >= 2; i+=2, length-=2) {
    jit_insn_store_relative(f, tgt, i, jVal2);
  }

  // Lastly copy 1 byte at a time
  jit_value_t jVal1 = jit_value_create_nint_constant(f, jit_type_ubyte, val1);
  for (; length >= 1; i+=1, length-=1) {
    jit_insn_store_relative(f, tgt, i, jVal1);
  }

  return;
}
#endif /* NA_LINUX_LIBJIT */

#if 1 /* NA_LINUX_LLVMJIT */
void PCodeCfg::genUnalignedMemcpy( IRBldr_t  * Bldr
                                 , jit_value_t tgt
                                 , jit_value_t src
                                 , jit_value_t len
                                 , Int32       constantLen = 0
                                 )
{
#if NExprDbgLvl > VV_NO
   char  NExBuf[500];
#endif
   DPT0( "VV0800: ", VV_BD, "GOT into genUnalignedMemcpy()\n" );

   // If dealing with a *short* fixed-length string ...
   // Note: The 32 in the next line is arbitrary.  We just don't want 
   // to generate dozens or even hundreds or thousands of load/store instructions.
   if ( (len == NULL) &&  (constantLen < 32) )
   {
      Int32 i, length = constantLen;

      DPT0( "VV0810: ", VV_BD,
            " GOT into genUnalignedMemcpy() with short fixed-length string\n" );

      // First try multiples of 8
      for (i=0; length >= 8; i+=8, length-=8)
      {
        jit_value_t val = IR_LoadRelativeWithType( Bldr, src, i, int64Ty_ );

        DPT0( "VV0812: ", VV_VD,
       "IR_StoreRelativeWithType( Bldr, val, tgt, i, int64Ty_ )\n" );
        IR_StoreRelativeWithType( Bldr, val, tgt, i, int64Ty_ );
      }

      // Then try multiples of 4
      for (; length >= 4; i+=4, length-=4)
      {
////    jit_value_t val = jit_insn_load_relative(f, src, i, jit_type_uint);
////    jit_insn_store_relative(f, tgt, i, val);

        jit_value_t val = IR_LoadRelativeWithType( Bldr, src, i, int32Ty_ );

        DPT0( "VV0814: ", VV_VD,
       "IR_StoreRelativeWithType( Bldr, val, tgt, i, int32Ty_ )\n" );
        IR_StoreRelativeWithType( Bldr, val, tgt, i, int32Ty_ );
      }

      // Then try multiples of 2
      for (; length >= 2; i+=2, length-=2)
      {
////    jit_value_t val = jit_insn_load_relative(f, src, i, jit_type_ushort);
////    jit_insn_store_relative(f, tgt, i, val);

        jit_value_t val = IR_LoadRelativeWithType( Bldr, src, i, int16Ty_ );

        DPT0( "VV0816: ", VV_VD,
       "IR_StoreRelativeWithType( Bldr, val, tgt, i, int16Ty_ )\n" );
        IR_StoreRelativeWithType( Bldr, val, tgt, i, int16Ty_ );
      }

      // Lastly copy 1 byte at a time
      for (; length >= 1; i+=1, length-=1)
      {
////    jit_value_t val = jit_insn_load_relative(f, src, i, jit_type_ubyte);
////    jit_insn_store_relative(f, tgt, i, val);

        jit_value_t val = IR_LoadRelativeWithType( Bldr, src, i, int8Ty_ );

        DPT0( "VV0819: ", VV_VD,
       "IR_StoreRelativeWithType( Bldr, val, tgt, i, int8Ty_ )\n" );
        IR_StoreRelativeWithType( Bldr, val, tgt, i, int8Ty_ );
      }

      return;

   } // End: if ( (len == NULL) &&  (constantLen < 32) )

   else if ( len == NULL ) // *long* fixed-length string
   {
      DPT0( "VV0820: ", VV_BD,
            "GOT into genUnalignedMemcpy() with long fixed-length string\n" );

      jit_value_t ConstLen = ConstantInt::get( int64Ty_, constantLen );

      genMemCopyLoop( Bldr, src, tgt, ConstLen );
   }
   else  // len != NULL
   {
       DPT0( "VV0830: ", VV_BD,
             "GOT into genUnalignedMemcpy() with len != NULL ptr\n" );

       genMemCopyLoop( Bldr, src, tgt, len );
   }

   return;
}
#endif /* NA_LINUX_LLVMJIT */

#ifdef NA_LINUX_LIBJIT

void PCodeCfg::genUnalignedMemcpy(jit_function_t f,
                                  jit_value_t tgt,
                                  jit_value_t src,
                                  jit_value_t len,
                                  Int32 constantLen = 0)
{
  //
  // Upon entering this routine, tgt, src, and len should all be temp operands
  // that were already initialized by caller.  len may be NULL, however,
  // indicating that a constant length (stored in "constantLen") is provided.
  // In this case, we attempt to use wide loads and stores to implement the
  // copy.
  //
 
  jit_value_t comp, val, temp1, temp2, temp3;

  jit_label_t fastLoopLabel = jit_label_undefined;
  jit_label_t remainderLabel = jit_label_undefined;
  jit_label_t endLabel = jit_label_undefined;

  jit_value_t max = jit_value_create_nint_constant(f, jit_type_int, 4);

  if (len == NULL) {
    Int32 i, length = constantLen;

    // First try multiples of 4
    for (i=0; length >= 4; i+=4, length-=4) {
      jit_value_t val = jit_insn_load_relative(f, src, i, jit_type_uint);
      jit_insn_store_relative(f, tgt, i, val);
    }

    // Then try multiples of 2
    for (; length >= 2; i+=2, length-=2) {
      jit_value_t val = jit_insn_load_relative(f, src, i, jit_type_ushort);
      jit_insn_store_relative(f, tgt, i, val);
    }

    // Lastly copy 1 byte at a time
    for (; length >= 1; i+=1, length-=1) {
      jit_value_t val = jit_insn_load_relative(f, src, i, jit_type_ubyte);
      jit_insn_store_relative(f, tgt, i, val);
    }

    return;
  }

  // Initial check for entering fast loop (i.e. if len >= max)
  comp = jit_insn_lt(f, len, max);
  jit_insn_branch_if(f, comp, &remainderLabel);

  // Start fast loop here
  jit_insn_label(f, &fastLoopLabel);

  // Copy 4 bytes at a time.
  val = jit_insn_load_relative(f, src, 0, jit_type_uint);
  jit_insn_store_relative(f, tgt, 0, val);

  // Decrement loop index
  temp1 = jit_insn_sub(f, len, max);
  jit_insn_store(f, len, temp1);

  // Increment string pointers
  temp2 = jit_insn_add(f, tgt, max);
  temp3 = jit_insn_add(f, src, max);
  jit_insn_store(f, tgt, temp2);
  jit_insn_store(f, src, temp3);

  // while (len >= 4) branch to fastLoop
  comp = jit_insn_ge(f, len, max);
  jit_insn_branch_if(f, comp, &fastLoopLabel);

  //
  // Start of remainder loop
  //

  jit_insn_label(f, &remainderLabel);

  // Branch to end label if len <= 0
  comp = jit_insn_le(f, len, zeroJitVal_);
  jit_insn_branch_if(f, comp, &endLabel);

  // Copy 1 byte at a time.
  val = jit_insn_load_relative(f, src, 0, jit_type_ubyte);
  jit_insn_store_relative(f, tgt, 0, val);

  // Decrement loop index
  temp1 = jit_insn_sub(f, len, oneJitVal_);
  jit_insn_store(f, len, temp1);

  // Increment string pointers
  temp2 = jit_insn_add(f, tgt, oneJitVal_);
  temp3 = jit_insn_add(f, src, oneJitVal_);
  jit_insn_store(f, tgt, temp2);
  jit_insn_store(f, src, temp3);

  jit_insn_branch(f, &remainderLabel);


  // End label
  jit_insn_label(f, &endLabel);
}

#if 0
void PCodeCfg::genUnalignedPadCheck(jit_function_t f,
                                    jit_value_t src,
                                    jit_value_t len,
                                    Int32 constantLen = 0)
{
  // Upon entering this routine, src, and len should all be temp operands
  // that were already initialized by caller.  len may be NULL, however,
  // indicating that a constant length (stored in "constantLen") is provided.
  // In this case, we attempt to use wide loads and stores to implement the
  // pad comparison

  if (len == NULL) {
  }
}
#endif

#if 0
void PCodeCfg::genUnalignedMemcmp(jit_function_t f,
                                  jit_value_t tgt,
                                  jit_value_t src,
                                  jit_value_t len,
                                  Int32 constantLen = 0)
{
  //
  // Upon entering this routine, tgt, src, and len should all be temp operands
  // that were already initialized by caller.  len may be NULL, however,
  // indicating that a constant length (stored in "constantLen") is provided.
  // In this case, we attempt to use wide loads and stores to implement the
  // compare.
  //
 
  jit_value_t comp, val, temp1, temp2, temp3;

  jit_label_t fastLoopLabel = jit_label_undefined;
  jit_label_t remainderLabel = jit_label_undefined;
  jit_label_t endLabel = jit_label_undefined;

  jit_value_t max = jit_value_create_nint_constant(f, jit_type_int, 4);

  if (len == NULL) {
    Int32 i, length = constantLen;

    // First try multiples of 4
    for (i=0; length >= 4; i+=4, length-=4) {
      jit_value_t val = jit_insn_load_relative(f, src, i, jit_type_uint);
      jit_insn_store_relative(f, tgt, i, val);
    }

    // Then try multiples of 2
    for (; length >= 2; i+=2, length-=2) {
      jit_value_t val = jit_insn_load_relative(f, src, i, jit_type_ushort);
      jit_insn_store_relative(f, tgt, i, val);
    }

    // Lastly copy 1 byte at a time
    for (; length >= 1; i+=1, length-=1) {
      jit_value_t val = jit_insn_load_relative(f, src, i, jit_type_ubyte);
      jit_insn_store_relative(f, tgt, i, val);
    }

    return;
  }

  // Initial check for entering fast loop (i.e. if len >= max)
  comp = jit_insn_lt(f, len, max);
  jit_insn_branch_if(f, comp, &remainderLabel);

  // Start fast loop here
  jit_insn_label(f, &fastLoopLabel);

  // Copy 4 bytes at a time.
  val = jit_insn_load_relative(f, src, 0, jit_type_uint);
  jit_insn_store_relative(f, tgt, 0, val);

  // Decrement loop index
  temp1 = jit_insn_sub(f, len, max);
  jit_insn_store(f, len, temp1);

  // Increment string pointers
  temp2 = jit_insn_add(f, tgt, max);
  temp3 = jit_insn_add(f, src, max);
  jit_insn_store(f, tgt, temp2);
  jit_insn_store(f, src, temp3);

  // while (len >= 4) branch to fastLoop
  comp = jit_insn_ge(f, len, max);
  jit_insn_branch_if(f, comp, &fastLoopLabel);

  //
  // Start of remainder loop
  //

  jit_insn_label(f, &remainderLabel);

  // Branch to end label if len <= 0
  comp = jit_insn_le(f, len, zeroJitVal_);
  jit_insn_branch_if(f, comp, &endLabel);

  // Copy 1 byte at a time.
  val = jit_insn_load_relative(f, src, 0, jit_type_ubyte);
  jit_insn_store_relative(f, tgt, 0, val);

  // Decrement loop index
  temp1 = jit_insn_sub(f, len, oneJitVal_);
  jit_insn_store(f, len, temp1);

  // Increment string pointers
  temp2 = jit_insn_add(f, tgt, oneJitVal_);
  temp3 = jit_insn_add(f, src, oneJitVal_);
  jit_insn_store(f, tgt, temp2);
  jit_insn_store(f, src, temp3);

  jit_insn_branch(f, &remainderLabel);


  // End label
  jit_insn_label(f, &endLabel);
}
#endif

void PCodeCfg::genBignumSub(PCodeCfg* cfg,
                            jit_function_t f,
                            PCodeOperand* res,
                            jit_value_t src1,
                            jit_value_t src2,
                            jit_value_t sign1,
                            jit_value_t sign2,
                            jit_value_t tSrc1,
                            jit_value_t tSrc2,
                            PCodeBlock* block)
{
  Int32 i;

  jit_value_t t1, t2, t3, t4, t5, t6 = NULL, carry = NULL;
  jit_label_t keepStringsLabel = jit_label_undefined;
  jit_label_t switchStringsLabel = jit_label_undefined;

  jit_value_t jitValMask16 =
    jit_value_create_nint_constant(f, jit_type_ushort, 0x8000);
  jit_value_t jitValNotMask16 =
    jit_value_create_nint_constant(f, jit_type_ushort, 0x7fff);
  jit_value_t jitValNotMask32 =
    jit_value_create_nint_constant(f, jit_type_ushort, 0x7fffffff);

  jit_value_t jitValUpper32 =
    jit_value_create_nint_constant(f, jit_type_int, 0x10000);

  Int32 len = res->getLen();
  Int32 len16 = len >> 1;

  Int32 bignumSize = len16;
  jit_type_t bignumType = jit_type_ushort;
  jit_value_t bignumNotMask = jitValNotMask16;

  // If we can process the bignum faster in chunks of 32-bits, we will.
  if ((len % 4) == 0) {
    bignumSize = len >> 2;
    bignumType = jit_type_uint;
    bignumNotMask = jitValNotMask32;
  }

  // Assume by default that the source order is correct - i.e. src1 has a
  // bigger magnitude that src2.
  jit_insn_store(f, tSrc1, src1);
  jit_insn_store(f, tSrc2, src2);

  // Which string is bigger?
  for (i=bignumSize-1; i >= 0; i--) {
    if (i == (bignumSize-1)) {
      // Need to compare without sign bit.
      t1 = jit_insn_and(f, sign1, bignumNotMask);
      t2 = jit_insn_and(f, sign2, bignumNotMask);

      // Store back into the source strings w/o sign bit enabled
      jit_insn_store_relative(f, tSrc1, i, t1);
      jit_insn_store_relative(f, tSrc2, i, t2);
    }
    else {
      t1 = jit_insn_load_relative(f, tSrc1, i, bignumType);
      t2 = jit_insn_load_relative(f, tSrc2, i, bignumType);
    }

    t3 = jit_insn_gt(f, t1, t2);
    jit_insn_branch_if(f, t3, &keepStringsLabel);

    // No need to do switcharoo comparison if this is the last one.  src1 will
    // either be equal or less that src2.  Since the chances of the two being
    // equal are unlikely, we assume not and always do a switcharoo.
    //
    // BTW, if both sources are equal, this is an example where we can have -0
    // emitted.  This is because, depending on which source we say is "bigger",
    // if the bigger one is negative, that's what the result will be.
    if (i != 0) {
      t3 = jit_insn_lt(f, t1, t2);
      jit_insn_branch_if(f, t3, &switchStringsLabel);
    }
  } 

  // Do the switcharoo
  jit_insn_label(f, &switchStringsLabel);
  jit_insn_store(f, tSrc1, src2);
  jit_insn_store(f, tSrc2, src1);

  // Keep things the same.
  jit_insn_label(f, &keepStringsLabel);

  // Now let's load up the result jit value.
  jit_value_t resJitVal = res->getJitValue(cfg, f, res->getJitType(), block);

  // Now start performing the subs.  Must do it 2-bytes at a time because libjit
  // doesn't support various 64-bit ops yet.
  for (i=0; i < len16; i++)
  {
    //
    // Perform: t6 = ((Int32)src1[i] - (Int32)src2[i]) - (carry)
    //   No need to subtract carry if this is the first time through.
    //
    t1 = jit_insn_load_relative(f, tSrc1, i, jit_type_ushort);
    t2 = jit_insn_load_relative(f, tSrc2, i, jit_type_ushort);
    t3 = jit_insn_convert(f, t1, jit_type_int, 0);
    t4 = jit_insn_convert(f, t2, jit_type_int, 0);
    t5 = jit_insn_sub(f, t3, t4);
    t6 = (i != 0) ? jit_insn_sub(f, t5, carry) : t5;

    // Perform: t4 = ((t6 + 0x10000) & 0xffff)
    t3 = jit_insn_add(f, t6, jitValUpper32);
    t4 = jit_insn_convert(f, t3, jit_type_ushort, 0);

    // If this is not the last short to subtrace, store the result and identify
    // a potential carry.
    if (i != (len16-1)) {
      jit_insn_store_relative(f, resJitVal, i, t4);
      carry = jit_insn_lt(f, t6, zeroJitVal_);
    }
    else {
      // Use "carry" to hold the sign value of src1.
      carry = jit_insn_and(f, t1, jitValMask16);

      // Now insert the sign bit into the final result.
      t6 = jit_insn_or(f, t4, carry);

      // Final result short is stored after source strings are updated with
      // their sign bits (so that we avoid situations like "A = A - B".
    }
  }

  // Reset sign in source
  jit_insn_store_relative(f, src1, bignumSize-1, sign1);
  jit_insn_store_relative(f, src2, bignumSize-1, sign2);

  // Store final res value here.
  jit_insn_store_relative(f, resJitVal, len16-1, t6);
}
#endif /* NA_LINUX_LIBJIT */

#if 1 /* NA_LINUX_LLVMJIT */

// FIX: Overlapped types can have problems too
void PCodeOperand::storeJitValue( PCodeCfg    * cfg
                                , IRBldr_t    * Bldr
                                , jit_type_t    IRtype
                                , jit_value_t   value
                                , PCodeBlock  * b
                                , jit_value_t * len = NULL
                                , NABoolean     noSave = FALSE
                                )
{
#if NExprDbgLvl >= VV_NO
  char  NExBuf[500];
#endif
  CDPT3( "VV0900: ", VV_BD, "In storeJitValue() on PCode Operand at %p, IRtype=%p, "
                      "value=%p\n", this, IRtype, value );

  // Get operand in hash table
  CollIndex i = bvIndex_;
  PCodeOperand* operand = cfg->getMap()->getFirstValue(&i);

  CDPT2( "VV0902: ", VV_BD, "In storeJitValue(): operand=%p, this=%p\n", operand, this );

  if (operand != this)
    return operand->storeJitValue(cfg, Bldr, IRtype, value, b, len, noSave);

  jit_type_t valType = value->getType() ;

  //
  // Are we being asked to perform a memcpy?
  //
  if ( valType->isPointerTy() )
  {
    jit_value_t size =
      ((len) ? *len : ConstantInt::get( cfg->getInt64Ty(), getLen() ) );

    if (jitValuePtr_)
    {
      CDPT0( "VV0904: ", VV_BD, "In storeJitValue, found jitValuePtr_ non-null\n");

      assert(jitValuePtrBlock_ != NULL);

      // If the PCBlock given in jitValuePtrBlock_ dominates PCBlock b,
      // then we are assured that jitValuePtr_ is valid.
      //
      if (!b->onesVector.testBit(jitValuePtrBlock_->getBlockNum()))
        jitValuePtr_ = NULL;
    }

    if (!jitValuePtr_)
    {
      CDPT0( "VV0906: ", VV_BD, "In storeJitValue, found jitValuePtr_ to be NULL\n");

      if (isVar() || isNonNativeTemp())
      {
        CDPT1( "VV0908: ", VV_BD, "In storeJitValue, getting param using stackIndex_ = "
                          "%d\n", stackIndex_);

        jit_value_t parm1 = cfg->getJitParams()[stackIndex_];

        CDPT0( "VV0910: ", VV_BD, 
                   "param  = Bldr->CreatePointerCast(parm1, cfg->getInt8PtrTy() )\n" );
        jit_value_t param  = Bldr->CreatePointerCast(parm1, cfg->getInt8PtrTy() );

////    jitValuePtr_ = jit_insn_add_relative(f, param, offset_); */

        CDPT1( "VV0912: ", VV_BD, "In storeJitValue, getting param using offset_ = "
                                                            "%d\n", offset_);

        jit_value_t tmp1 = ConstantInt::get( cfg->getInt64Ty(), offset_ ) ;

        CDPT1( "VV0914: ", VV_BD,
       "jitValuePtr_ = Bldr->CreateGEP( param, tmp1 = %d )\n", offset_ );
        jitValuePtr_ = Bldr->CreateGEP( param, tmp1 );
      }
      else if ( jitValue_ ) // If space already allocated
      {
        CDPT0( "VV0918: ", VV_BD,
       "jitValuePtr_ = Bldr->CreateGEP( jitValue_, cfg->getZeroJitVal() )\n" );
        jitValuePtr_ = Bldr->CreateGEP( jitValue_, cfg->getZeroJitVal() );
      }
      else
      {
        assert(0==390);    // ZZZZ FIX THIS IF WE EVER GET HERE.
#if 0 /* FIX THIS IF WE EVER GET HERE. */
        // FIXME: This needs to change.  We need to allocate the temp globally
        // so that references are available everywhere.  Best solution is to
        // pass in temp array and use that.
        jitValuePtr_ = jit_insn_alloca(size);
#endif /* FIX THIS */
      }

      CDPT2( "VV0919: ", VV_BD, "In storeJitValue(): setting jitValuePtrBlock_ = %p "
                                           "for PCop = %p\n",    b,    this );
      jitValuePtrBlock_ = b;

    } // End: if (!jitValuePtr_)

    CDPT1( "VV0920: ", VV_BD, "In storeJitValue, calling genUnalignedMemcpy() "
                              "with length=%d\n", getLen() );

    // Generate unaligned memcpy instead
////// jit_insn_memcpy(f, jitValuePtr_, value, size);
////cfg->genUnalignedMemcpy(f, jitValuePtr_, value, NULL, getLen());

    cfg->genUnalignedMemcpy( Bldr, jitValuePtr_, value, NULL, getLen());

    // If we are directly told not to save the jit values, don't!
    if (noSave)
    {
      jitValueBlock_ = NULL;
      jitValuePtrBlock_ = NULL;

      if ( jitValue_ )
         CDPT2( "VV0925: ", VV_BD, "In storeJitValue(): NULLing jitValue_ at %p "
                                   "-- in PCodeOp at %p\n", &jitValue_ , this);

      jitValue_ = jitValuePtr_ = NULL;
    }

    return;

  } // End: if ( valType->isPointerTy() )

  //
  // Peforming standard sized moves
  //

  if (isVar())
  {
    jit_value_t parm1 = cfg->getJitParams()[stackIndex_];

       CDPT0( "VV0940: ", VV_BD,
               "param  = Bldr->CreatePointerCast(parm1, cfg->getInt8PtrTy() )\n" );
    jit_value_t param  = Bldr->CreatePointerCast(parm1, cfg->getInt8PtrTy() );

////jit_insn_store_relative(param, offset_, value);

    CDPT0( "VV0942: ", VV_BD,
   "cfg->IR_StoreRelativeWithType( Bldr, value, param, offset_, IRtype )\n");
    cfg->IR_StoreRelativeWithType( Bldr, value, param, offset_, IRtype );
  }
  else
  {
    assert (isTemp());

#if 0
    // Just set the value to be the new jitValue
    // FIX: this is ripe for error :(
    jitValue_ = value;
#endif

    CDPT2( "VV0943: ", VV_BD, "For PCodeOp at %p, Curr Value of jitValue_ = %p\n",
                                            this,               jitValue_ );
    if (!jitValue_)
    {
////  jitValue_ = value; // SEE 'ripe for error' comment above.

      CDPT0( "VV0944: ", VV_BD,
     "jitValue_ = (llvm::Value *)Bldr->CreateAlloca( valType )\n" );
      jitValue_ = (llvm::Value *)Bldr->CreateAlloca( valType );

      CDPT2( "VV0945: ", VV_BD, "Just set jitValue_ in PCodeOp at %p to %p "
                                "via CreateAlloca()\n", this, jitValue_);
    }
    else
    {
      // If this PC block is not dominated by the previous block containing the
      // first jitValue_, then things like a jitValuePtr_ need to be reset.
      // Note, the definition of jitValue_ does not need to dominate this block
      // since when the value is created (as it is above), it is done so upon
      // entry to the routine (and therefore available everywhere).  Addresses
      // are not, however, and therefore may need to be recalculated.
      if ( jitValuePtr_  &&
           ( ( ! b->onesVector.testBit(jitValuePtrBlock_->getBlockNum()) ) ||
             ( jitValueBlock_ && 
              ( !b->onesVector.testBit(jitValueBlock_->getBlockNum()) ) )
           )
         )
        jitValuePtr_ = NULL;

    }

    CDPT1( "VV0946: ", VV_BD, "Value's Type ID is %d\n", (int) valType->getTypeID() );
    CDPT1( "VV0947: ", VV_BD, "jitValue_'s Type ID is %d\n",
                 (int) jitValue_->getType()->getTypeID() );

#if NExprDbgLvl >= VV_BD
    if ( jitValue_->getType()->isPointerTy() )
       CDPT1( "VV0948: ", VV_BD, "jitValue_ is PTR to type ID %d\n",
                      jitValue_->getType()->getContainedType(0)->getTypeID() );
#endif // NExprDbgLvl >= VV_BD


////jit_insn_store(f, jitValue_, value);

    CDPT0( "VV0949: ", VV_BD,
   "jit_value_t storePtr = Bldr->CreatePointerCast( jitValue_, valType->getPointerTo() )\n" );
    jit_value_t storePtr = Bldr->CreatePointerCast( jitValue_, valType->getPointerTo() );

    CDPT0( "VV0950: ", VV_BD,
   "Bldr->CreateStore( value, storePtr )\n" );
    Bldr->CreateStore( value, storePtr );


    CDPT2( "VV0951: ", VV_BD, "In storeJitValue(): setting jitValueBlock_ = %p "
                                                 "for PCop = %p\n", b, this );
    jitValueBlock_ = b;
  }

  // If we are directly told not to save the jit values, don't!
  if (noSave)
  {
    jitValueBlock_ = NULL;
    jitValuePtrBlock_ = NULL;

    CDPT2( "VV0952: ", VV_BD, "NULLing jitValue_ at %p -- in PCodeOp at %p\n",
                                                 &jitValue_ ,       this );
    jitValue_ = jitValuePtr_ = NULL;
  }
}
#endif /* NA_LINUX_LLVMJIT */

#ifdef NA_LINUX_LIBJIT
// FIX: Overlapped types can have problems too
void PCodeOperand::storeJitValue(PCodeCfg* cfg,
                                 jit_function_t f,
                                 jit_type_t type,
                                 jit_value_t value,
                                 PCodeBlock* b,
                                 jit_value_t* len = NULL,
                                 NABoolean noSave = FALSE)
{
  // Get operand in hash table
  CollIndex i = bvIndex_;
  PCodeOperand* operand = cfg->getMap()->getFirstValue(&i);
  if (operand != this)
    return operand->storeJitValue(cfg, f, type, value, b, len, noSave);

  //
  // Are we being asked to perform a memcpy?
  //
  if (jit_value_get_type(value) == jit_type_void_ptr) {
    jit_value_t size =
      ((len) ? *len : jit_value_create_nint_constant(f, jit_type_int,getLen()));

    if (jitValuePtr_) {
      assert(jitValueBlock_ != NULL);
      if (!b->onesVector.testBit(jitValueBlock_->getBlockNum()))
        jitValuePtr_ = NULL;
    }

    if (!jitValuePtr_) {
      if (isVar() || isNonNativeTemp()) {
        jit_value_t param = cfg->getJitParams()[stackIndex_];
        jitValuePtr_ = jit_insn_add_relative(f, param, offset_);
      }
      else {
        // FIXME: This needs to change.  We need to allocate the temp globally
        // so that references are available everywhere.  Best solution is to
        // pass in temp array and use that.
        jitValuePtr_ = jit_insn_alloca(f, size);
      }

      jitValueBlock_ = b;
    }

    // Generate unaligned memcpy instead
    // jit_insn_memcpy(f, jitValuePtr_, value, size);
    cfg->genUnalignedMemcpy(f, jitValuePtr_, value, NULL, getLen());

    // If we are directly told not to save the jit values, don't!
    if (noSave) {
      jitValueBlock_ = NULL;
      jitValue_ = jitValuePtr_ = NULL;
    }

    return;
  }

  //
  // Peforming standard sized moves
  //

  if (isVar()) {
    jit_value_t param = cfg->getJitParams()[stackIndex_];
    jit_insn_store_relative(f, param, offset_, value);
  }
  else {
    assert (isTemp());

#if 0
    // Just set the value to be the new jitValue
    // FIX: this is ripe for error :(
    jitValue_ = value;
#endif

    if (!jitValue_) {
      jitValue_ = jit_value_create(f, type);
    }
    else {
      // If this block is not dominated by the previous block containing the
      // first jitValue_, then things like a jitValuePtr_ need to be reset.
      // Note, the definition of jitValue_ does not need to dominate this block
      // since when the value is created (as it is above), it is done so upon
      // entry to the routine (and therefore available everywhere).  Addresses
      // are not, however, and therefore may need to be recalculated.
      if (jitValuePtr_ &&
          (!b->onesVector.testBit(jitValueBlock_->getBlockNum())))
        jitValuePtr_ = NULL;
    }

    jit_insn_store(f, jitValue_, value);

    jitValueBlock_ = b;
  }

  // If we are directly told not to save the jit values, don't!
  if (noSave) {
    jitValueBlock_ = NULL;
    jitValue_ = jitValuePtr_ = NULL;
  }
}
#endif /* NA_LINUX_LIBJIT */

#if 1 /* NA_LINUX_LLVMJIT */

//
// IR_insn_branch_if_zero() - generate branch instruction to specified
// target IR block *if* the specified 'value' is equal to 0.
//
// NOTES:
// (1) Works regardless of whether 'value' is an 8-bit, 16-bit,
//     32-bit, or 64-bit value.
// (2) Specified target IR block must have already been created.
// (3) This routine only generates the conditional branch.  It does *not*
//     do the "push_back" on the tgt block to place it in the
//     output "stream".
// (4) This routine always creates an IR block for the "if not zero" condition
//     and does the "push_back" on it as well as setting the "InsertPoint"
//     to the "if not" IR block.
//
void  PCodeCfg::IR_insn_branch_if_zero( IRBldr_t     * Bldr
                                      , jit_value_t    value
                                      , p_IR_block_t * tgt_IR_block
                                      )
{
#if NExprDbgLvl > VV_NO
   char  NExBuf[500];
#endif
   DPT0( "VV1000: ", VV_BD, "GOT into IR_insn_branch_if_zero()\n" );

   llvm::LLVMContext &LLContxt = Bldr->getContext();

   llvm::Function *ExpFn  = Bldr->GetInsertBlock()->getParent();

   // Create an IR block for the "if not zero" case
   p_IR_block_t ifNZ_Blk = BasicBlock::Create( LLContxt, "BrIf_NZ_Blk" );

   DPT1( "VV1001: ", VV_VD, "ifNZ_Blk = %p\n", ifNZ_Blk ); 

   jit_value_t  Zero   = ConstantInt::get( value->getType(), 0 );

   DPT0( "VV1002: ", VV_VD,
               "IsZero = Bldr->CreateICmpEQ( value, Zero, 'check0' )\n" );
   jit_value_t  IsZero = Bldr->CreateICmpEQ( value, Zero, "check0" );

   DPT2( "VV1004: ", VV_VD,
  "Bldr->CreateCondBr( IsZero, *tgt_IR_block = %p, ifNZ_Blk = %p )\n", *tgt_IR_block, ifNZ_Blk );
   Bldr->CreateCondBr( IsZero, *tgt_IR_block, ifNZ_Blk );

   DPT1( "VV1006: ", VV_VD,
  "ExpFn->getBasicBlockList().push_back( ifNZ_Blk = %p )\n", ifNZ_Blk );
   ExpFn->getBasicBlockList().push_back( ifNZ_Blk );

   DPT1( "VV1008: ", VV_VD,
  "Bldr->SetInsertPoint( ifNZ_Blk = %p )\n", ifNZ_Blk );
   Bldr->SetInsertPoint( ifNZ_Blk );

}

//
// IR_insn_branch_if_not_zero() - generate branch instruction to specified
// target IR block *if* the specified 'value' is NOT equal to 0.
//
// NOTES:
// (1) Works regardless of whether 'value' is an 8-bit, 16-bit,
//     32-bit, or 64-bit value.
// (2) Specified target IR block must have already been created.
// (3) This routine only generates the conditional branch.  It does *not*
//     do the "push_back" on the tgt block to place it in the
//     output "stream".
// (4) This routine always creates an IR block for the "if Zero" condition
//     and does the "push_back" on it as well as setting the "InsertPoint"
//     to the "if not" IR block.
//
void  PCodeCfg::IR_insn_branch_if_not_zero( IRBldr_t     * Bldr
                                          , jit_value_t    value
                                          , p_IR_block_t * tgt_IR_block
                                          )
{
#if NExprDbgLvl > VV_NO
   char  NExBuf[500];
#endif
   DPT0( "VV1100: ", VV_BD, "GOT into IR_insn_branch_if_not_zero()\n" );

   llvm::LLVMContext &LLContxt = Bldr->getContext();

   llvm::Function *ExpFn  = Bldr->GetInsertBlock()->getParent();

   // Create an IR block for the "if zero" case
   p_IR_block_t if_0_Blk = BasicBlock::Create( LLContxt, "BrIfZeroBlk" );

   DPT1( "VV1101: ", VV_VD, "if_0_Blk = %p\n", if_0_Blk ); 

   jit_value_t  Zero   = ConstantInt::get( value->getType(), 0 );

   DPT0( "VV1102: ", VV_VD,
               "IsZero = Bldr->CreateICmpEQ( value, Zero, 'check0' )\n" );
   jit_value_t  IsZero = Bldr->CreateICmpEQ( value, Zero, "check0" );

   DPT2( "VV1104: ", VV_VD,
  "Bldr->CreateCondBr( IsZero, if_0_Blk = %p, *tgt_IR_block = %p )\n", if_0_Blk, *tgt_IR_block );
   Bldr->CreateCondBr( IsZero, if_0_Blk, *tgt_IR_block );

   DPT1( "VV1106: ", VV_VD,
  "ExpFn->getBasicBlockList().push_back( if_0_Blk = %p )\n", if_0_Blk );
   ExpFn->getBasicBlockList().push_back( if_0_Blk );

   DPT1( "VV1108: ", VV_VD,
  "Bldr->SetInsertPoint( if_0_Blk = %p )\n", if_0_Blk );
   Bldr->SetInsertPoint( if_0_Blk );

}

void PCodeOperand::storeNullJitValueAndBranch( PCodeCfg   * cfg
                                             , IRBldr_t   * Bldr
                                             , jit_value_t  value
                                             , p_IR_block_t tgtLabel
                                             , NABoolean    forComp
                                             , PCodeBlock * b
                                             )
{
#if NExprDbgLvl >= VV_NO
  char  NExBuf[500];
#endif

  jit_value_t res = NULL ;

  CDPT0( "VV1200: ", VV_BD, "GOT into storeNullJitValueAndBranch()\n" );

// Use pCode[2-3] to chk if op2 is NULL
// If it is NULL, set output = NULL
//   and go to next PCode instruction;
// Else set output = NOT NULL
//   and add pCode[7] to pCode ptr

  // If 'value' is non-zero, then   

  // Get operand in hash table
  CollIndex i = bvIndex_;
  PCodeOperand* operand = cfg->getMap()->getFirstValue(&i);

  CDPT2( "VV1205: ", VV_BD, "In storeNullJVaBR(): operand=%p, this=%p\n", operand, this );

  if (operand != this)
    return operand->storeNullJitValueAndBranch(cfg, Bldr, value, tgtLabel,
                                               forComp, b);

//jit_value_t zeroJitVal16=jit_value_create_nint_constant(f, jit_type_short, 0);
  jit_value_t zeroJitVal16 = ConstantInt::get( cfg->getInt16Ty(), 0 );

//jit_value_t neg1JitVal16=jit_value_create_nint_constant(f, jit_type_short,-1);
  jit_value_t neg1JitVal16 = ConstantInt::get( cfg->getInt16Ty(), -1 );

  llvm::Function *ExpFn  = Bldr->GetInsertBlock()->getParent();

  // Is src a null bit value or not (doesn't matter)
  if (TRUE)
  {
    llvm::LLVMContext &LLContxt = Bldr->getContext();
    p_IR_block_t nullLabel    = BasicBlock::Create( LLContxt, "StoreNJ_nullBlk" );

    CDPT1( "VV1210: ", VV_BD, "nullLabel = %p\n", nullLabel );

    //
    // NOTE: We don't need a "merge" block because we will either branch
    // to the tgtLabel or we will use the "nullLabel" block after
    // we are done with this routine.
    //
    CDPT1( "VV1212: ", VV_BD, "In storeNullJitV... Value's Type ID is %d\n",
                                        (int) value->getType()->getTypeID() );

    // Branch away if source is null.
//  jit_insn_branch_if(f, value, &nullLabel);
    cfg->IR_insn_branch_if_not_zero( Bldr, value, &nullLabel );

    // At this point, we know 'value' is zero -- indicating a NON-NULL

    // Is tgt null-indicator just a bit?  If so, we must zero that bit to indicate a non-null
    if (nullBitIndex_ != -1) {
      UInt8 m = ((UInt8)0x1 << (7 - (nullBitIndex_ & 7)));

      // Tgt must be a var for it to have a null bit index.
      jit_value_t parm1 = cfg->getJitParams()[stackIndex_];

      CDPT0( "VV1210: ", VV_BD,
                 "param  = Bldr->CreatePointerCast(parm1, cfg->getInt8PtrTy() )\n" );
      jit_value_t param  = Bldr->CreatePointerCast(parm1, cfg->getInt8PtrTy() );

//    jit_value_t mask = jit_value_create_nint_constant(f, jit_type_ubyte, ~m);
      jit_value_t mask = ConstantInt::get( cfg->getInt8Ty() , ~m );

//    jit_value_t nByte = jit_insn_load_relative(f, param,
//                          offset_ + (nullBitIndex_ >> 3), jit_type_ubyte);
      jit_value_t nByte = cfg->IR_LoadRelativeWithType( Bldr, param,
                            offset_ + (nullBitIndex_ >> 3), cfg->getInt8Ty() );

      // Clear the null bit and store the byte (containing the bit)
//    jit_value_t res = jit_insn_and(f, nByte, mask);

      CDPT1( "VV1214: ", VV_BD,
     "res = Bldr->CreateAnd( nByte, mask = %d )\n", ~m );
      res = Bldr->CreateAnd( nByte, mask );

//    jit_insn_store_relative(f, param, offset_ + (nullBitIndex_ >> 3), res);

      CDPT0( "VV1217: ", VV_BD,
     "cfg->IR_StoreRelativeWithType( Bldr, res, param, offset_ + ..., res->getType() )\n");
      cfg->IR_StoreRelativeWithType( Bldr, res,
                                param, offset_ + (nullBitIndex_ >> 3),
                                res->getType() );

      // Branch to target label
//    jit_insn_branch(f, targetLabel);

      CDPT1( "VV1219: ", VV_BD,
     "Bldr->CreateBr( tgtLabel = %p )\n", tgtLabel );
      Bldr->CreateBr( tgtLabel );

      CDPT1( "VV1220: ", VV_BD, "In storeNullJitVal: CREATING BRANCH TO tgtLabel at "
                         "( %p )\n", tgtLabel );

      // Now handle NULL case.

//    jit_insn_label(f, &nullLabel);

      CDPT1( "VV1220: ", VV_BD,
     "ExpFn->getBasicBlockList().push_back( nullLabel = %p )\n", nullLabel );
      ExpFn->getBasicBlockList().push_back( nullLabel );

      CDPT1( "VV1222: ", VV_BD,
     "Bldr->SetInsertPoint( nullLabel = %p )\n", nullLabel );
      Bldr->SetInsertPoint( nullLabel );

      CDPT1( "VV1223: ", VV_BD,
             "In storeNullJitVal: Setting Insert Point to nullLabel at %p\n",
                                                                 nullLabel );

//    mask = jit_value_create_nint_constant(f, jit_type_ubyte, m);
      mask = ConstantInt::get( cfg->getInt8Ty() , m );

//    nByte = jit_insn_load_relative(f, param,
//                          offset_ + (nullBitIndex_ >> 3), jit_type_ubyte);
      nByte = cfg->IR_LoadRelativeWithType( Bldr, param,
                            offset_ + (nullBitIndex_ >> 3), cfg->getInt8Ty() );

      // Set the null bit and store it back
//    res = jit_insn_or(f, nByte, mask);

      CDPT1( "VV1224: ", VV_BD,
     "res = Bldr->CreateOr( nByte, mask = %d )\n", m );
      res = Bldr->CreateOr( nByte, mask );

//    jit_insn_store_relative(f, param, offset_ + (nullBitIndex_ >> 3), res);

      CDPT0( "VV1224: ", VV_BD,
     "cfg->IR_StoreRelativeWithType( Bldr, res, param, offset_ + ..., res->getType() )\n");
      cfg->IR_StoreRelativeWithType( Bldr, res,
                                param, offset_ + (nullBitIndex_ >> 3),
                                res->getType() );

    } // End: if (nullBitIndex_ != -1)

    else {
      // Tgt is a standard type, so we store a 0 in the null-indicator (to indicate non-null)
      if (isVar()) {
        jit_value_t parm1 = cfg->getJitParams()[stackIndex_];

        CDPT0( "VV1230: ", VV_BD,
                   "param  = Bldr->CreatePointerCast(parm1, cfg->getInt8PtrTy() )\n" );
        jit_value_t param  = Bldr->CreatePointerCast(parm1, cfg->getInt8PtrTy() );

        // Store a zero, indicating non-null
//      jit_insn_store_relative(f, param, offset_,
//        (forComp ? cfg->getZeroJitVal() : zeroJitVal16));

        res = forComp ? cfg->getZeroJitVal() : zeroJitVal16 ;

        CDPT1( "VV1232: ", VV_BD, "In storeNJ-forComp = %d\n", forComp);
        CDPT1( "VV1234: ", VV_BD, "In storeNJ-res=%s\n",
                  res==zeroJitVal16 ? "zeroJitVal16" : "cfg->getZeroJitVal()" );
        CDPT1( "VV1236: ", VV_BD, "In storeNJ-type WIDTH = %d\n",
                  res->getType()->getIntegerBitWidth() );

        CDPT0( "VV1240: ", VV_BD,
       "cfg->IR_StoreRelativeWithType( Bldr, res, param, offset_, res->getType() )\n");
        cfg->IR_StoreRelativeWithType( Bldr, res, param, offset_, res->getType() );

//      jit_insn_branch(f, targetLabel);

        CDPT1( "VV1242: ", VV_BD,
       "Bldr->CreateBr( tgtLabel = %p )\n", tgtLabel );
        Bldr->CreateBr( tgtLabel );

        // Now handle NULL case.

//      jit_insn_label(f, &nullLabel);

        CDPT1( "VV1244: ", VV_BD,
       "ExpFn->getBasicBlockList().push_back( nullLabel = %p )\n", nullLabel );
        ExpFn->getBasicBlockList().push_back( nullLabel );

        CDPT1( "VV1246: ", VV_BD,
       "Bldr->SetInsertPoint( nullLabel = %p )\n", nullLabel );
        Bldr->SetInsertPoint( nullLabel );

//      jit_insn_store_relative(f, param, offset_,
//        (forComp ? cfg->getNeg1JitVal() : neg1JitVal16));

        res      = forComp ? cfg->getNeg1JitVal() : neg1JitVal16 ;

        CDPT1( "VV1247: ", VV_BD, "In storeNJ1-forComp = %d\n", forComp);
        CDPT1( "VV1248: ", VV_BD, "In storeNJ1-res=%s\n",
                   res==neg1JitVal16 ? "neg1JitVal16":"cfg->getNeg1JitVal()");
        CDPT1( "VV1249: ", VV_BD, "In storeNJ1-type WIDTH = %d\n",
                   res->getType()->getIntegerBitWidth() );
       
        CDPT0( "VV1250: ", VV_BD,
       "cfg->IR_StoreRelativeWithType( Bldr, res, param, offset_, res->getType() )\n");
        cfg->IR_StoreRelativeWithType( Bldr, res,
                                  param, offset_,
                                  res->getType() );

      } // End: if (isVar())

      else {
        if ( 1 /* forComp ? */) {
//        storeJitValue(cfg, f, jit_type_int, cfg->getZeroJitVal(), b, NULL);

          CDPT1( "VV1252: ", VV_BD, "In storeNJ2 ... forComp = %d\n", forComp);
          CDPT1( "VV1253: ", VV_BD, "In storeNJ2 ... storetype = %s\n",
                                                        "cfg->getInt32Ty()");
          storeJitValue(cfg, Bldr, cfg->getInt32Ty(), cfg->getZeroJitVal(), b, NULL );

//        jit_insn_branch(f, targetLabel);

          CDPT1( "VV1255: ", VV_BD,
         "Bldr->CreateBr( tgtLabel = %p )\n", tgtLabel );
          Bldr->CreateBr( tgtLabel );

          // Now handle NULL case.
 
//        jit_insn_label(f, &nullLabel);

          CDPT1( "VV1260: ", VV_BD,
         "ExpFn->getBasicBlockList().push_back( nullLabel = %p )\n", nullLabel );
          ExpFn->getBasicBlockList().push_back( nullLabel );

          CDPT1( "VV1261: ", VV_BD,
         "Bldr->SetInsertPoint( nullLabel = %p )\n", nullLabel );
          Bldr->SetInsertPoint( nullLabel );

//        storeJitValue(cfg, f, jit_type_int, cfg->getNeg1JitVal(), b, NULL);
          storeJitValue(cfg, Bldr, cfg->getInt32Ty(), cfg->getNeg1JitVal(), b, NULL );
        }
#if 0 /* The following OPTIMIZATION (over the above section of code) does NOT always work */
      // because sometimes we NEED the call to storeJitValue above even for Temps.
      // Case in point:  Launchpad defect 1328248
        else { // forComp == FALSE

          CDPT1( "VV1262: ", VV_BD, "In storeNJ3 ... forComp = %d\n", forComp);

          jitValue_ = value;

          CDPT2( "VV1263: ", VV_BD,
                 "In storeNullJV(): setting jitValue_ in PCodeOp at %p to %p\n",
                                                              this, jitValue_ );
          CDPT1( "VV1264: ", VV_BD,
                 "In storeNJ- jitValue_'s Type ID is %d\n",
                                       (int)jitValue_->getType()->getTypeID() );
          CDPT2( "VV1264A: ", VV_BD,
                 "In storeNullJV(): setting jitValueBlock_ = %p for PCop = %p\n",
                                                              b,         this );
          jitValueBlock_ = b;

//        jit_insn_branch(f, targetLabel);

          CDPT1( "VV1265: ", VV_BD,
         "Bldr->CreateBr( tgtLabel = %p )\n", tgtLabel );
          Bldr->CreateBr( tgtLabel );

//        jit_insn_label(f, &nullLabel);

          CDPT1( "VV1267: ", VV_BD,
         "ExpFn->getBasicBlockList().push_back( nullLabel = %p )\n", nullLabel );
          ExpFn->getBasicBlockList().push_back( nullLabel );

          CDPT1( "VV1268: ", VV_BD,
         "Bldr->SetInsertPoint( nullLabel = %p )\n", nullLabel );
          Bldr->SetInsertPoint( nullLabel );

        } // End: else
#endif
      } // End: else  [i.e. (isVar()) == FALSE ]
    } // End: else  [ i.e. (nullBitIndex_ == -1) ]
  } // End:  if (TRUE)
}

#endif /* NA_LINUX_LLVMJIT */

#ifdef NA_LINUX_LIBJIT
void PCodeOperand::storeNullJitValueAndBranch(PCodeCfg* cfg,
                                              jit_function_t f,
                                              jit_value_t value,
                                              jit_label_t* targetLabel,
                                              NABoolean forComp,
                                              PCodeBlock* b)
{
  // Get operand in hash table
  CollIndex i = bvIndex_;
  PCodeOperand* operand = cfg->getMap()->getFirstValue(&i);
  if (operand != this)
    return operand->storeNullJitValueAndBranch(cfg, f, value, targetLabel,
                                               forComp, b);

  jit_label_t nullLabel = jit_label_undefined;

  jit_value_t zeroJitVal16=jit_value_create_nint_constant(f, jit_type_short, 0);
  jit_value_t neg1JitVal16=jit_value_create_nint_constant(f, jit_type_short,-1);

  // Is src a null bit value or not (doesn't matter)
  if (TRUE)
  {
    // Branch away if source is null.
    jit_insn_branch_if(f, value, &nullLabel);

    // Is tgt a null bit?
    if (nullBitIndex_ != -1) {
      UInt8 m = ((UInt8)0x1 << (7 - (nullBitIndex_ & 7)));

      // Tgt must be a var for it to have a null bit index.
      jit_value_t param = cfg->getJitParams()[stackIndex_];
      jit_value_t mask = jit_value_create_nint_constant(f, jit_type_ubyte, ~m);
      jit_value_t nByte = jit_insn_load_relative(f, param,
                            offset_ + (nullBitIndex_ >> 3), jit_type_ubyte);

      // Clear value and restore.
      jit_value_t res = jit_insn_and(f, nByte, mask);
      jit_insn_store_relative(f, param, offset_ + (nullBitIndex_ >> 3), res);

      // Branch to target label
      jit_insn_branch(f, targetLabel);

      // Now handle NULL case.

      jit_insn_label(f, &nullLabel);

      mask = jit_value_create_nint_constant(f, jit_type_ubyte, m);
      nByte = jit_insn_load_relative(f, param,
                            offset_ + (nullBitIndex_ >> 3), jit_type_ubyte);

      // Set value and restore.
      res = jit_insn_or(f, nByte, mask);
      jit_insn_store_relative(f, param, offset_ + (nullBitIndex_ >> 3), res);
    }
    else {
      // Tgt is a standard type
      if (isVar()) {
        jit_value_t param = cfg->getJitParams()[stackIndex_];
        jit_insn_store_relative(f, param, offset_,
          (forComp ? cfg->getZeroJitVal() : zeroJitVal16));
        jit_insn_branch(f, targetLabel);

        // Now handle NULL case.

        jit_insn_label(f, &nullLabel);
        jit_insn_store_relative(f, param, offset_,
          (forComp ? cfg->getNeg1JitVal() : neg1JitVal16));
      }
      else {
        if (forComp) {
          storeJitValue(cfg, f, jit_type_int, cfg->getZeroJitVal(), b, NULL);
          jit_insn_branch(f, targetLabel);

          // Now handle NULL case.
 
          jit_insn_label(f, &nullLabel);
          storeJitValue(cfg, f, jit_type_int, cfg->getNeg1JitVal(), b, NULL);
        }
        else { 
          jitValue_ = value;
          jitValueBlock_ = b;
          jit_insn_branch(f, targetLabel);
          jit_insn_label(f, &nullLabel);
        }
      }
    } 
  }
}

void PCodeCfg::jitProcessFloatExceptionCheck(jit_function_t f,
                                             PCodeOperand* fRes,
                                             jit_label_t* errorJitLabel,
                                             PCodeBlock* block)
{
  jit_value_t temp1, temp2, temp3, temp4;

  jit_value_t expShift = jit_value_create_nint_constant(f, jit_type_int, 20);
  jit_value_t expMask = jit_value_create_nint_constant(f, jit_type_int, 0x7ff);

  jit_value_t fVal = fRes->getJitValue(this, f, jit_type_void_ptr, block);

  // Get the upper 32-bits of the double
  temp1 = jit_insn_load_relative(f, fVal, 4, jit_type_uint);

  // Shift the load over so that the last 12 bits are in the lower
  temp2 = jit_insn_ushr(f, temp1, expShift);

  // Mask out exponent and compare to see if 11-bit exponent is all set.
  temp3 = jit_insn_and(f, temp2, expMask);
  temp4 = jit_insn_eq(f, temp3, expMask);

  jit_insn_branch_if(f, temp4, errorJitLabel);
}

#if 0

NABoolean PCodeCfg::jitProcessPredicate(PCodeInst* comp,
                                        jit_value_t compJitVal,
                                        jit_function_t f)
{
  if (comp->next && comp->next->isAnyLogicalBranch())
  {
    PCodeInst* branch = comp->next;

    PCodeOperand* compTgt = comp->getWOps()[0];
    PCodeOperand* branchSrc = branch->getROps()[0];
    PCodeOperand* branchTgt = branch->getWOps()[0];

    PCodeBlock* block = comp->block;

    NABoolean srcLive = FALSE, tgtLive = FALSE;

    if (branchSrc->getBvIndex() == compTgt->getBvIndex()) {
      // Is source or target live after branch?
      srcLive = branchSrc->isVar() || block->isOperandLiveInSuccs(branchSrc);
      tgtLive = branchTgt->isVar() || block->isOperandLiveInSuccs(branchTgt);

      if (srcLive)
        compTgt->storeJitValue(this, f, jit_type_int, compJitVal, block);

      if (tgtLive)
        branchTgt->storeJitValue(this, f, jit_type_int, compJitVal, block);

      if ((branch->getOpcode() == PCIT::BRANCH_OR) ||
          (branch->getOpcode() == PCIT::BRANCH_OR_CNT)) {
        jit_insn_branch(f, block->getTargetBlock()->getJitLabel());
      else
        jit_insn_branch(f, block->getFallThroughBlock()->getJitLabel());
      }
    }
  }
  else if (comp->next && (comp->next->getOpcode() == PCIT::MOVE_MBIN32S))
  {
    PCodeInst* move = comp->next;

    PCodeOperand* compTgt = comp->getWOps()[0];
    PCodeOperand* moveSrc = move->getROps()[0];

    if (compTgt->getBvIndex() == moveSrc->getBvIndex()) {
      // A RETURN is guaranteed to following a MOVE_MBIN32S (verify w/ assert)
      assert (move->next && (move->next->getOpcode() == PCIT::RETURN));
    }
  }
}

#endif

NABoolean PCodeCfg::jitProcessPredicate(PCodeInst* comp,
                                        jit_function_t f,
                                        jit_label_t* falseLabel,
                                        jit_label_t* trueLabel,
                                        NABoolean isFirstBlockTrue)
{
  CollIndex i;

  //
  // Called when generating the TRUE and FALSE blocks first for certain
  // comparisons.  Rather than check the result of the comparison twice (one
  // for the COMP and another for a subsequent BRANCH), we attempt to do it all
  // at once.
  //
  // A boolean called isFirstBlockTrue passed in to tell us if the 1st block
  // to generate is the "true" block or not.
  //

  if (comp->next && comp->next->isAnyLogicalBranch())
  {
    PCodeInst* branch = comp->next;

    PCodeOperand* compTgt = comp->getWOps()[0];
    PCodeOperand* branchSrc = branch->getROps()[0];
    PCodeOperand* branchTgt = branch->getWOps()[0];

    PCodeBlock* block = comp->block;

    NABoolean srcLive = FALSE, tgtLive = FALSE;

    if (branchSrc->getBvIndex() == compTgt->getBvIndex()) {
      // Is source or target live after branch?
      srcLive = branchSrc->isVar() || block->isOperandLiveInSuccs(branchSrc);
      tgtLive = branchTgt->isVar() || block->isOperandLiveInSuccs(branchTgt);

      // Dump out blocks now (either TRUE followed by FALSE, or vice-versa)
      // Use for loop to simplify code (i.e. reduce duplication of code) in
      // laying out the blocks one way or the other.

      for (i=0; i < 2; i++)
      {
        if ((isFirstBlockTrue && (i==0)) ||
            (!isFirstBlockTrue && (i==1)))
        {
          jit_insn_label(f, trueLabel);

          // Dump true block first
          if (srcLive)
            compTgt->storeJitValue(this, f, jit_type_int, oneJitVal_, block);

          if (tgtLive)
            branchTgt->storeJitValue(this, f, jit_type_int, oneJitVal_, block);

          if ((branch->getOpcode() == PCIT::BRANCH_OR) ||
              (branch->getOpcode() == PCIT::BRANCH_OR_CNT))
            jit_insn_branch(f, block->getTargetBlock()->getJitLabel());
          else
            jit_insn_branch(f, block->getFallThroughBlock()->getJitLabel());
        }

        if ((!isFirstBlockTrue && (i==0)) ||
            (isFirstBlockTrue && (i==1)))
        {
          // Dump false block next - layout false block label first.
          jit_insn_label(f, falseLabel);

          if (srcLive)
            compTgt->storeJitValue(this, f, jit_type_int, zeroJitVal_, block);

          if (tgtLive)
            branchTgt->storeJitValue(this, f, jit_type_int, zeroJitVal_, block);

          if ((branch->getOpcode() == PCIT::BRANCH_AND) ||
              (branch->getOpcode() == PCIT::BRANCH_AND_CNT))
            jit_insn_branch(f, block->getTargetBlock()->getJitLabel());
          else
            jit_insn_branch(f, block->getFallThroughBlock()->getJitLabel());
        }
      }

      return TRUE;
    }
  }
  else if (comp->next && (comp->next->getOpcode() == PCIT::MOVE_MBIN32S))
  {
    PCodeInst* move = comp->next;

    PCodeOperand* compTgt = comp->getWOps()[0];
    PCodeOperand* moveSrc = move->getROps()[0];

    if (compTgt->getBvIndex() == moveSrc->getBvIndex()) {
      // A RETURN is guaranteed to following a MOVE_MBIN32S (verify w/ assert)
      assert (move->next && (move->next->getOpcode() == PCIT::RETURN));

      if (isFirstBlockTrue) {
        jit_insn_label(f, trueLabel);
        jit_insn_return(f, tJitVal_);

        // False block next - add label first.
        jit_insn_label(f, falseLabel);
        jit_insn_return(f, fJitVal_);
      }
      else {
        jit_insn_label(f, falseLabel);
        jit_insn_return(f, fJitVal_);

        // True block next - add label first.
        jit_insn_label(f, trueLabel);
        jit_insn_return(f, tJitVal_);
      }

      return TRUE;
    }
  }
  return FALSE;
}
#endif /* NA_LINUX_LIBJIT */

#if 1 /* NA_LINUX_LLVMJIT */

void PCodeCfg::layoutNativeCode()
{
  static pthread_mutex_t Our_LLVM_mutex = PTHREAD_MUTEX_INITIALIZER;

  // List of signal numbers for which LLVM establishes its own signal handlers
  static const int SaveSigs[] = {
    SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGBUS, SIGFPE,
    SIGSEGV, SIGUSR2, SIGPIPE, SIGTERM, SIGXFSZ
  };
  static const int NumSaveSigs = sizeof(SaveSigs) / sizeof(SaveSigs[0]);

  static THREAD_P struct {
    struct sigaction SigAct ;
  } SavedSigInfo[ NumSaveSigs ];

  static struct {
    struct sigaction SigAct ;
  } SavedLLVMhandlers[ NumSaveSigs ];

  static NABoolean LLVMhandlersSaved = FALSE ;


#if NExprDbgLvl > VV_NO
  char  NExBuf[500];
#endif
  Int32   PCI_count = 0  ; //Count of number of PCODE instructions translated.

  expr_->setPCodeNative(FALSE); // For now, assume no native code generated.

  static NABoolean debug = (getenv("NATIVE_EXPR_DEBUG") != NULL);

  PCodeBinary* pCode = expr_->getPCodeBinary();

  NExTempsList_ = new(heap_) NExTEMPSLIST(heap_);


  // First see if this graph can be compiled natively
  if ( ! canGenerateNativeExpr() )
  {
     delete NExTempsList_ ;
     NExTempsList_ = NULL ;
     expr_->setEvalPtr( (ex_expr::evalPtrType)( (CollIndex) 0 ) );//Ensure NULL!
     return;
  }

#if NExprDbgLvl >= VV_I0
  struct rusage begTime;

  if ( NExprDbgLvl_ >= VV_I0 )
  {
    std::cout << std::flush ; // Flush anything in the data buffer
    (void) getrusage( RUSAGE_THREAD, &begTime );

    char*  StTime = ctime((const time_t*)&begTime.ru_utime.tv_sec);
    Int32 StTimeLn = strlen(StTime);
    StTime[StTimeLn-1] = ' ';

    DPT2( "VV90000: ", VV_I3,
          "STARTING layoutNativeCode() at %s : Microseconds: %d\n",
                                       StTime, (int) begTime.ru_utime.tv_usec );

    if ( NExprDbgLvl_ >= VV_I1  &&
         NExDbgInfoPtr_->getNExStmtPrinted() == FALSE )
    {
       NExDbgInfoPtr_->setNExStmtPrinted( TRUE );

       if ( NExDbgInfoPtr_->getNExStmtSrc() )
       {
          NExLog( "==Orig. SQL Stmt:\n" );
          NExLog( NExDbgInfoPtr_->getNExStmtSrc() );
          NExLog( "\n==\n" );
       }
    }
    std::cout << std::flush ; // Flush anything in the data buffer
  }
#endif // NExprDbgLvl >= VV_I0

  Int32 opc = PCIT::Op_END;
  CollIndex i, j, PCBlkIndex;

  Int32 skipInst = 0;

  // First compute the dominator tree
  computeDomTree();

  //
  // Since the LLVM library is not thread-safe, we get a mutex at this point.
  // If the mutex is not immediately available, then another thread is using
  // LLVM right now, so we just fall back to using PCODE and not try to 
  // generate a Native Expression right now.
  //
  int tryVal = pthread_mutex_trylock( &Our_LLVM_mutex );
  if ( tryVal )
  {
     delete NExTempsList_ ;
     NExTempsList_ = NULL ;
     expr_->setEvalPtr( (ex_expr::evalPtrType)( (CollIndex) 0 ) );//Ensure NULL!

#if NExprDbgLvl >= VV_I2
 if ( NExprDbgLvl_ >= VV_I2 ) {
    DPT0( "VV90000A: ", VV_XD, "Not translating to native code - mutex locked.\n");
 }
#endif
     return;
  }

  //
  // Now that we have the mutex, save the current signal handlers for signals
  // for which LLVM has handlers.  Also, if we have previously saved LLVM's
  // sigaction handlers, restore them before we go back into LLVM code.
  //
  for (Int32 ii = 0; ii < NumSaveSigs ; ii++ )
  {
     Int32 ret = sigaction( SaveSigs[ii], 
            ( LLVMhandlersSaved ) ?
            &(SavedLLVMhandlers[ii].SigAct) : (const struct sigaction *)NULL,
            &(SavedSigInfo[ii].SigAct) ) ;
     assert( ret == 0 );
  }

  llvm::InitializeNativeTarget();
  llvm::LLVMContext &LLContxt = llvm::getGlobalContext();

  // Create context to hold the LLVM JIT's primary state

  // Make the module, which holds all the code.
  llvm::Module * TheModule = new llvm::Module("ExpressionJit", LLContxt);

  llvm::IRBuilder<> * Bldr = new llvm::IRBuilder<>( LLContxt );

  std::map<std::string, Value*> NamedValues;


  // Create the "ExecutionEngine"
  std::string ErrStr;
  llvm::TargetOptions target_opts;
  target_opts.PositionIndependentExecutable = 1;

  //
  // NOTE: By specifying "x86-64" in the call to setMArch() below and
  //       specifying only the generic 64-bit attributes and *not* specifying
  //       any chip-specific attributes in the call to setMAttrs() below,
  //       we are telling the JIT compiler to generate machine code for a
  //       generic x86-64 chip rather than letting it default to generating
  //       machine code for the specific chip that the SQL Compiler happens
  //       to be running on at the moment.
  //
  std::vector<std::string>  MAttrs; // Machine Architecture attributes
  MAttrs.push_back("64bit-mode");

  llvm::ExecutionEngine* TheExecutionEngine =
    EngineBuilder(TheModule).setErrorStr(&ErrStr).setMArch("x86-64").setMAttrs(MAttrs).setTargetOptions(target_opts).setRelocationModel(llvm::Reloc::PIC_).setOptLevel(CodeGenOpt::Default).create();
  if ( !TheExecutionEngine )
  {
    std::cout << ErrStr << std::flush;
    printf("EXITING FROM layoutNativeCode() -could not create function !!\n");
    std::cout << std::flush ; // Flush anything in the data buffer

    delete Bldr ;

    //
    // Restore signal handlers to what they were on entry.
    // NOTE: We do NOT try to save the LLVM handlers when going through this
    // code the first time because LLVM has *not* set them up yet.
    //
    for (Int32 ii = 0; ii < NumSaveSigs ; ii++ )
    {
       Int32 ret = sigaction( SaveSigs[ii], &(SavedSigInfo[ii].SigAct),
                                             (struct sigaction *)NULL);
       assert( ret == 0 );
    }
    pthread_mutex_unlock( &Our_LLVM_mutex ); // Other threads could use LLVM now

    delete NExTempsList_ ;
    NExTempsList_ = NULL ;
    expr_->setEvalPtr( (ex_expr::evalPtrType)( (CollIndex) 0 ) );//Ensure NULL!
    return ;
  }

  // Initialize various jit_type_t (llvm::Type) pointers in PCodeCfg object
  // for easy reference later.
  //
  int1PtrTy_  = llvm::Type::getInt1PtrTy(LLContxt); // Used like old JIT's void ptr
  int8Ty_     = llvm::Type::getInt8Ty(LLContxt);
  int16Ty_    = llvm::Type::getInt16Ty(LLContxt);
  int32Ty_    = llvm::Type::getInt32Ty(LLContxt);
  int64Ty_    = llvm::Type::getInt64Ty(LLContxt);
  int8PtrTy_  = llvm::Type::getInt8PtrTy(LLContxt);
  int16PtrTy_ = llvm::Type::getInt16PtrTy(LLContxt);
  int32PtrTy_ = llvm::Type::getInt32PtrTy(LLContxt);
  int64PtrTy_ = llvm::Type::getInt64PtrTy(LLContxt);
  floatPtrTy_ = llvm::Type::getFloatPtrTy(LLContxt);

  DPT3( "VV90000a: ", VV_VD,
 "int1PtrTy_ = %p, int8Ty_ = %p,     int16Ty_ = %p\n", int1PtrTy_ , int8Ty_ , int16Ty_ );

  DPT2( "VV90000b: ", VV_VD,
 "int32Ty_ = %p,   int64Ty_ = %p\n", int32Ty_ , int64Ty_ );

  DPT3( "VV90000c: ", VV_VD,
 "int8PtrTy_  = %p, int16PtrTy_ = %p, int32PtrTy_ = %p\n", int8PtrTy_ , int16PtrTy_ , int32PtrTy_ );

  DPT2( "VV90000d: ", VV_VD,
 "int64PtrTy_ = %p, floatPtrTy_ = %p\n", int64PtrTy_     , floatPtrTy_ );

/************ KEEP IN COMMENTS FOR NOW -- MAY NEED TO PORT THESE IN THE FUTURE
  // Create commonly referenced constants
  JitVal_1_  = jit_value_create_nint_constant(f, jit_type_int, 1);
  zeroJitVal_ = jit_value_create_nint_constant(f, jit_type_int, 0);
  neg1JitVal_ = jit_value_create_nint_constant(f, jit_type_int, -1);
  jit_value_t zeroJitVal16=jit_value_create_nint_constant(f, jit_type_short, 0);
  jit_value_t neg1JitVal16=jit_value_create_nint_constant(f, jit_type_short,-1);
  padJitVal = jit_value_create_nint_constant(f, jit_type_ubyte, ' ');
  jit_value_t shiftRVal = jit_value_create_nint_constant(f, jit_type_int, 31);
  okJitVal_ = jit_value_create_nint_constant(f, jit_type_int,ex_expr::EXPR_OK);
  tJitVal_ = jit_value_create_nint_constant(f, jit_type_int,ex_expr::EXPR_TRUE);
  fJitVal_ = jit_value_create_nint_constant(f,jit_type_int,ex_expr::EXPR_FALSE);
  eJitVal  = jit_value_create_nint_constant(f,jit_type_int,ex_expr::EXPR_ERROR);
************ KEEP IN COMMENTS FOR NOW */

  // Create commonly referenced constants
  IR_Const_1_    = ConstantInt::get( int32Ty_ , 1 );
  IR_Const_0_    = ConstantInt::get( int32Ty_ , 0 );
  IR_Const_neg1_ = ConstantInt::get( int32Ty_ , -1 );
  IR_Const_16bit_0_ = ConstantInt::get( int16Ty_ , 0 );
  IR_Const_64bit_0_ = ConstantInt::get( int64Ty_ , 0 );

  jit_value_t  IR_Const_pad = ConstantInt::get( int8Ty_, ' ' );
 
  IR_Const_OK_    =  ConstantInt::get( int32Ty_ , ex_expr::EXPR_OK    );
  IR_Const_TRUE_  =  ConstantInt::get( int32Ty_ , ex_expr::EXPR_TRUE  );
  IR_Const_FALSE_ =  ConstantInt::get( int32Ty_ , ex_expr::EXPR_FALSE );
  IR_Const_ERR_   =  ConstantInt::get( int32Ty_ , ex_expr::EXPR_ERROR );

  pGlobTab_Val_  =  NamedValues[ "pGlobTab"  ] ;
  atp1_Val_      =  NamedValues[ "atp1"      ] ;
  atp2_Val_      =  NamedValues[ "atp2"      ] ;
  ex_expr_p_Val_ =  NamedValues[ "ex_expr_p" ] ;

  // Build the function signature.
  // Create an "expression" function with 4 parameters
  // and which returns an Int32 [Actually it returns an enum exp_return_type ]
  // 
  // NOTE: The 4 parameters (eventually) passed to the function
  // at runtime will be:
  //
  // param[0] = nativeGlobTable* pGlobTab
  // param[1] = atp_struct*      atp1
  // param[2] = atp_struct*      atp2
  // param[3] = ex_expr *        ex_expr_ptr  [Used to get ptrs to
  //                                           Constants and Temps]

  // NOTE: We declare all 4 arguments as (char *) [i.e. int8PtrTy_ ]
  // because otherwise we would have to cast them to Int8Ptr before
  // we use them since the first thing we want to do with them is to
  // add some *byte* offset.  In other words, it is just plain easier
  // to declare them as Int8Ptr, add the byte offset, and then cast
  // them to be the kind of pointer we really want them to be.
  //
  std::vector<Type*> args(4, int1PtrTy_ );  // Lie!  Say they are "void" ptrs
  FunctionType* functype = FunctionType::get( int32Ty_,
                                             args, false);

  // Now that we have the function signature (i.e. functype), we
  // can tell LLVM to create the "function".
  //
  llvm::Function * ExpFn = llvm::Function::Create(functype,
                                            llvm::Function::ExternalLinkage,
                                            "testExpr", TheModule);

  // Give the parameters names!
  std::vector<std::string> Args;
  Args.push_back("pGlobTab");
  Args.push_back("atp1");
  Args.push_back("atp2");
  Args.push_back("ex_expr_p");

  //
  // Loop through the function arguments (the function type built above
  // says there are 4 args) and create entries in the NamedValues
  // array saying that "pGlobTab" has the value that was passed to the
  // first arg and "atp1" has the value passed to the second arg, etc.
  //
  unsigned Idx = 0;
  for(llvm::Function::arg_iterator AI = ExpFn->arg_begin();
      Idx != Args.size(); ++AI, ++Idx)
  {
      AI->setName(Args[Idx]);
      NamedValues[Args[Idx]] = AI;
  }

  // Create a single IR block to hold instructions
  p_IR_block_t  IRBlk = BasicBlock::Create(LLContxt, "entryBlk", ExpFn);

  DPT1( "VV90001: ", VV_VD,
 "Bldr->SetInsertPoint( IRBlk = %p )\n", IRBlk );
  Bldr->SetInsertPoint(IRBlk);

  jit_value_t globTableJitVal = NamedValues[Args[0]];

  // Allocate incoming parameters
  DPT1( "VV90002: ", VV_XD,
        "SIZING jitParams_[] to array of size 4+pCode[0] == %d\n",
                                              4+pCode[0] );

  jitParams_ = (jit_value_t *) new(heap_) jit_value_t[4 + pCode[0]];

  jitParams_[0] = globTableJitVal ;

  // Generate code for getting tupp pointers
  //
  // 1. Get atp pointer (p) passed in
  // 2. Add bytes to get to tuple we care about
  //    off = &(tempAtp->getTupp(atpIndex)) - (tempAtp)
  //    temp1 = param[atp] + off;
  // 3. Dereference pointer to get tupp_descriptor pointer
  //    temp2 = load(temp1);
  // 4. Add bytes to get to dataTuple
  //    off = &(tempTuppDesc.getTupleAddress()

  atp_struct tempAtp;
  tupp tempTupp;
  tupp_descriptor tempTuppDesc;
  ex_expr Ex_Expr;

  for (i=0, j=1; i < (CollIndex)pCode[0]; i++, j+=2)
  {
    Int64 off1 = 0, off2 = 0;

    DPT3( "VV90004: ", VV_XD,
          "INITIALIZING jitParams_[4+i == %d] where j=%d, pCode[0] = %d\n",
                                   4+i,             j,    pCode[0] );

    // First get the atp we care about
    DPT2( "VV90006: ", VV_XD,
          "SETTING t1 to NamedValues[ Args[ 1 + pCode[j] ] ] "
          "where j=%d and pCode[j]=%d\n",   j,  pCode[j] );

    jit_value_t t1 = NamedValues[ Args[ 1 + pCode[j] ] ] ;

    // Now, at runtime, t1 will point to the atp we care about.
    // Well, except that t1 is of type "Pointer to 64-bit Int".

    // So, now we get the tupp descriptor pointer we care about.
    // We do that by first getting a ptr to the tuppArray entry within the atp_struct

    DPT1( "VV90008: ", VV_XD,
          "Passing pCode[j+1] = %d to getTuppForNativeExpr()\n", pCode[j+1] );
    off1 =
      (char*)(&(tempAtp.getTuppForNativeExpr(pCode[j+1]))) - (char*)(&tempAtp);

    DPT0( "VV90010: ", VV_VD,
               "Int8p_t1 = Bldr->CreatePointerCast(t1, int8PtrTy_ )\n");
    jit_value_t Int8p_t1 = Bldr->CreatePointerCast(t1, int8PtrTy_ );

    DPT1( "VV90012: ", VV_VD,
               "V_tuppArr = IR_LoadRelativeWithType( Bldr, Int8p_t1, off1=%ld, int8PtrTy_ )\n", off1 );
    jit_value_t V_tuppArr = IR_LoadRelativeWithType( Bldr, Int8p_t1, off1, int8PtrTy_ );

    off2 = ((char *)&(tempTuppDesc.tupleAddress_)) - (char *)(&tempTuppDesc);
    jit_value_t t3 = IR_LoadRelativeWithType( Bldr, V_tuppArr, off2, int8PtrTy_ );

    DPT2( "VV90013: ", VV_XD,
      "SETTING jitParams_[%d] = rtnval from IR_LoadRelativeWithType(...,V_tuppArr, %ld, ...)\n",
                          4+i,  off2 );
    jitParams_[4+i] = t3;

  }

  //
  // Allocate incoming constant pointer (needed for strings and floats).
  //
  // Do the same for the temps array pointer.  This array is used to store to
  // those temps which usually don't fit within a native container - e.g.
  // strings and bignums.
  //
  Int64 ConstantsOff = ((char *)&(Ex_Expr.constantsArea_)) - (char *)(&Ex_Expr);

  DPT1( "VV90013: ", VV_VD, "SET ConstantsOff to %ld\n", ConstantsOff);

  jit_value_t exprJitVal   = NamedValues[ Args[3] ] ;

  DPT0( "VV90020: ", VV_VD,
 "jitParams_[1] = IR_LoadRelativeWithType( Bldr, exprJitVal, ConstantsOff, int8PtrTy_ )\n" );
  jitParams_[1] = IR_LoadRelativeWithType( Bldr, exprJitVal,
                                           ConstantsOff, int8PtrTy_ );

  Int64 TempsOff = ((char *)&(Ex_Expr.tempsArea_)) - (char *)(&Ex_Expr);

  DPT1( "VV90022: ", VV_VD,
 "jitParams_[2] = IR_LoadRelativeWithType( Bldr, exprJitVal, TempsOff = %ld, int8PtrTy_ )\n", TempsOff);
  jitParams_[2] = IR_LoadRelativeWithType( Bldr, exprJitVal,
                                           TempsOff, int8PtrTy_ );

  // Allocate and initialize opData array for use with CLAUSE_EVAL instructions
  PCodeOperand * opData[(3*ex_clause::MAX_OPERANDS)];
  PCodeOperand** opDataNulls = &(opData[0]);
  PCodeOperand** opDataVals = &(opData[2*ex_clause::MAX_OPERANDS]);
  str_pad((char*)opData, (3*ex_clause::MAX_OPERANDS)*sizeof(PCodeOperand*), 0);

  // Allocate pointers to global defs available at runtime.  Note, if they are
  // not referenced by the pcode graph, these loads will be deleted by the jit.

// ZZZZ FIX THIS when we add support for something that needs 'nullTable'
//         DBG_C("VV90030: Bldr->CreateLoad(pGlobTab_Val_)\n");
//         DBG_C("VV90031: Bldr->CreateExtractValue(Bldr->CreateLoad(pGlobTab_Val_),0)\n");
//  jit_value_t *nullTable = Bldr->CreateExtractValue(Bldr->CreateLoad(pGlobTab_Val_),0);

  // Allocate all Temps that will be needed!   
  // NOTE: We do this *before* we start creating the native instructions
  // because otherwise LLVM's JIT Compiler can end up complaining that
  // some predecessor paths to an IR block allocate the variable while
  // other predecessor paths to the block don't allocate it.

  DPT1( "VV90034: ", VV_XD, "After CanGEN, NExTempsList_->entries()=%d\n",
                                           NExTempsList_->entries() );
  for (  Int32 tli = 0 ; tli < NExTempsList_->entries() ; tli++ )
  {
      NExTempListEntry * tliEntry = (*NExTempsList_)[tli] ;

      PCIT::AddressingMode OprTyp = tliEntry->getOprTyp() ;
      PCodeOperand       * Oper   = tliEntry->getPCOp()   ;
      Int32                Num    = tliEntry->getNum()    ;

      DPT3( "VV90035: ", VV_XD,
     "Oper->allocJitValue( this, Bldr, OprTyp = %d, Len = %d, Num = %d )\n",
                                       (Int32)OprTyp, Oper->getLen(), Num );
      Oper->allocJitValue( this, Bldr, OprTyp, Oper->getLen(), Num ) ;
  }
  delete NExTempsList_ ; // We are done with this, so free it!
  NExTempsList_ = NULL ;

  // NOW CREATE THE NATIVE INSTRUCTIONS

  // Create the list of pcode basic blocks in physical layout order
  BLOCKLIST physBlockList(heap_);

  createPhysList(physBlockList);

  // First create LLVM BasicBlocks objects and keep track of them.
  // We want one LLVM BasicBlock to start each physBlock.
  // Depending on the instructions in the block, additonal BasicBlocks
  // may get created later, but that's all right.  We just need
  // some initial ones so we can generate branches to them when
  // we encounter a PCODE instruction that branches to a physBlock
  //

  NABoolean justGeneratedRET = FALSE;
  NABoolean justHandledUnconditionBranch = FALSE;

  for ( PCBlkIndex=0;
       (PCBlkIndex < physBlockList.entries()) && !getJitFailureSeen();
        PCBlkIndex++ )
  {
      PCodeBlock* PCBlk = physBlockList[PCBlkIndex];

      p_IR_block_t * IR_Blk_Label = PCBlk->getJitBlkLbl();

      * IR_Blk_Label = BasicBlock::Create( LLContxt, "physBlk"  );

      DPT2( "VV90040: ", VV_XD,
            "In First Loop: Created BB AT ( %p ) for PCBlk = %p\n",
                                            *IR_Blk_Label,   PCBlk );

      if ( * IR_Blk_Label == IR_block_undefined )
         setJitFailureSeen();
  }

  // Branch from the initial IR block to the first IR block with a label.
  // Note: The 'SetInsertPoint' will be done the first time through the
  // following loop ... after the IR block gets push_back() called on it.

  DPT1( "VV90050: ", VV_XD,
 "Bldr->CreateBr( *(physBlockList[0]->getJitBlkLbl()) = %p )\n", *(physBlockList[0]->getJitBlkLbl()) );
  Bldr->CreateBr( *(physBlockList[0]->getJitBlkLbl()) );

  // Loop through each pcode block, generating instructions on a per block basis.
  // We stop immediately if a failure was seen during code gen.

  DPT1( "VV90052: ", VV_XD,
      "Starting loop creating one BLOCK at a time, with BLK COUNT=%d\n",
                                                     physBlockList.entries() );
  DPT1( "VV90054: ", VV_VD, "getJitFailureSeen() == %d \n",
                                                    (int)getJitFailureSeen() );

  for (PCBlkIndex=0;
       (PCBlkIndex < physBlockList.entries()) && !getJitFailureSeen();
       PCBlkIndex++)
  {
    // Must reset these flags for each PCode Block
    justGeneratedRET = FALSE;
    justHandledUnconditionBranch = FALSE;

    DPT0( "VV90056: ", VV_VD, "Top of loop creating one BLOCK at a time\n");

    PCodeBlock* PCBlk = physBlockList[PCBlkIndex];

    if ( *(PCBlk->getJitBlkLbl()) != NULL )
    {
       // Insert the IR block into the function and set the
       // insertion point to start putting instructions
       // into this IR block.
       p_IR_block_t  IR_Blk_Ptr = *( PCBlk->getJitBlkLbl() );

       DPT1( "VV90060: ", VV_VD,
      "ExpFn->getBasicBlockList().push_back( IR_Blk_Ptr= %p )\n", IR_Blk_Ptr );
       ExpFn->getBasicBlockList().push_back( IR_Blk_Ptr );

       DPT1( "VV90062: ", VV_VD,
      "Bldr->SetInsertPoint( IR_Blk_Ptr = %p )\n", IR_Blk_Ptr );
       Bldr->SetInsertPoint( IR_Blk_Ptr );
    }

    FOREACH_INST_IN_BLOCK(PCBlk, PCInst)
    {
      PCI_count++ ;

      DPT0( "VV90100: ", VV_VD,
            "In Main Loop creating one INSTRUCTION at a time\n");
      DPT2( "VV90102: ", VV_XD,
            "In Main Loop: PCInst at %p, PCInst->next = %p \n",
                                 PCInst, PCInst->next );
#if NExprDbgLvl >= VV_VD
      if ( PCInst->next )
         DPT2( "VV90103: ", VV_XD, "In Main Loop: next at %p has opc == %d\n",
                               PCInst->next , (int) PCInst->next->getOpcode());
#endif

      // If a failure occurred, stop translation immediately.
      if (getJitFailureSeen())
      {
        DPT0( "VV90105: ", VV_XD, "getJitFailureSeen() returned FAILURE\n");
        break;
      }

      // Sometimes native code is generated for multiple pcode instructions in
      // one pass, and so "skipped" instructions don't need to be processed.
      if (skipInst > 0)
      {
        DPT0( "VV90107: ", VV_XD,
              "skipInst is > 0 so we are doing a 'continue'\n");
        skipInst--;
        continue;
      }
      //
      // NOTE: The code after the end of the FOREACH_INST_IN_BLOCK loop
      // depends on doing the skipInst check and 'continue' above
      // *BEFORE* changing the value of the variable 'opc'.
      //

      opc = PCInst->getOpcode();

      DPT2( "VV90200: ", VV_I1, "Found opc = %d %s\n", (int) opc, NatExprName(opc) );

      DPT2( "VV90201: ", VV_XD, "PCInst at %p, PCInst->next = %p\n",
                                       PCInst, PCInst->next );
      //
      // LLVM's JIT Compiler gets confused if we generate two RET
      // instructions back-to-back (within the same IR block.)
      // So, here we explicitly *NOT* generate any code for a PCIT::RETURN
      // if the last instruction we generated was a RET instruction.
      //
      if ( opc != PCIT::RETURN )
         justGeneratedRET = FALSE;

      if ( justGeneratedRET == FALSE )
      switch (opc)
      {
        case PCIT::MOVE_MATTR5_MATTR5:
        {
          PCodeOperand *src1,      *res                    ;
          jit_value_t   src1JitVal, resJitVal, vcLenJitVal ;
          jit_type_t    src1IRtype, resIRtype, vcType      ;

          src1 = PCInst->getROps()[0]; 
          res  = PCInst->getWOps()[0];
          PRINT_DETAILS_OF_RDOp( 0, src1 );
          PRINT_DETAILS_OF_WROp( 0, res );

          if (src1->getVcIndicatorLen() == 2) {
////        vcType = jit_type_ushort;
////        vcLenJitVal = jit_value_create_nint_constant(f, jit_type_int, 2);

            vcType      = int16Ty_ ;
            vcLenJitVal = ConstantInt::get( int16Ty_ , 2 );
            DPT0( "VV91002: ", VV_XD,
               "In case PCIT::MOVE_MATTR5_MATTR5: vcLenJitVal = 2 (16-bit)\n");
          }
          else {
////        vcType = jit_type_uint;
////        vcLenJitVal = jit_value_create_nint_constant(f, jit_type_int, 4);

            vcType      = int32Ty_ ;
            vcLenJitVal = ConstantInt::get( int32Ty_ , 4 );
            DPT0( "VV91003: ", VV_XD,
               "In case PCIT::MOVE_MATTR5_MATTR5: vcLenJitVal = 4 (32-bit)\n");
          }

////      src1JitVal = src1->getJitValue(this, f, src1->getJitType(), PCBlk);
          src1IRtype = src1->getJitType( this );
          src1JitVal = src1->getJitValue(this, Bldr, src1IRtype, PCBlk);

////      resJitVal = res->getJitValue(this, f, res->getJitType(), PCBlk);
          resIRtype = res->getJitType( this );
          resJitVal = res->getJitValue(this, Bldr, resIRtype, PCBlk);

////      jit_insn_store(f, tRes, resJitVal);
////      jit_insn_store(f, tSrc1, src1JitVal);

////      jit_value_t srcLen = jit_insn_load_relative(f, src1JitVal, 0, vcType);
          jit_value_t srcLen = IR_LoadRelativeWithType( Bldr, 
                                                        src1JitVal, 0, vcType );

          DPT2( "VV91010: ", VV_XD, "srcLen      type = %d, num bits = %d\n",
                                    srcLen->getType()->getTypeID(),
                                    srcLen->getType()->getIntegerBitWidth() );
          DPT2( "VV91011: ", VV_XD, "vcLenJitVal type = %d, num bits = %d\n",
                               vcLenJitVal->getType()->getTypeID(),
                               vcLenJitVal->getType()->getIntegerBitWidth() );

////      srcLen = jit_insn_add(f, srcLen, vcLenJitVal);

          DPT0( "VV91014: ", VV_VD,
         "srcLen = Bldr->CreateAdd( srcLen , vcLenJitVal , 'IntAdd' )\n" );
          srcLen = Bldr->CreateAdd( srcLen , vcLenJitVal , "IntAdd" );

////      genUnalignedMemcpy(f, tRes, tSrc1, srcLen);
          DPT0( "VV91016: ", VV_XD,
         "genUnalignedMemcpy( Bldr, resJitVal, src1JitVal, srcLen )\n");
          genUnalignedMemcpy( Bldr, resJitVal, src1JitVal, srcLen );

          break;
        }

        case PCIT::BRANCH:
        {
////      jit_insn_branch(f, PCBlk->getTargetBlock()->getJitLabel());

          p_IR_block_t TgtBlk = *(PCBlk->getTargetBlock()->getJitBlkLbl()) ;

          DPT1( "VV92010: ", VV_VD,
         "Bldr->CreateBr( TgtBlk = %p )\n", TgtBlk );
          Bldr->CreateBr( TgtBlk );
          justHandledUnconditionBranch = TRUE;
          break;
        }

        case PCIT::BRANCH_OR:
        case PCIT::BRANCH_AND:
        {
          PCodeOperand * src1,      * res    ;
          jit_value_t    src1JitVal,  tRes   ;
          p_IR_block_t * IR_Blk_Label = NULL ;

          src1 = PCInst->getROps()[0]; 
          res  = PCInst->getWOps()[0];
          PRINT_DETAILS_OF_RDOp( 0, src1 );
          PRINT_DETAILS_OF_WROp( 0, res );

          jit_value_t takenJitVal =
            ((opc == PCIT::BRANCH_OR) || (opc == PCIT::BRANCH_OR_CNT))
              ? IR_Const_1_ : IR_Const_0_;

          IR_Blk_Label = PCBlk->getTargetBlock()->getJitBlkLbl();

////      src1JitVal = src1->getJitValue(this, f, jit_type_int, PCBlk);
          src1JitVal = src1->getJitValue(this, Bldr, int32Ty_, PCBlk);

          if (res->isVar() || PCBlk->isOperandLiveInSuccs(res))
////        res->storeJitValue(this, f, jit_type_int, src1JitVal, PCBlk, NULL);
            res->storeJitValue(this, Bldr, int32Ty_, src1JitVal, PCBlk);

////      resJitVal = jit_insn_eq(f, src1JitVal, takenJitVal);
          tRes = jitGenCompare( Bldr, IntCompare_EQ,
                                src1JitVal,   takenJitVal ,
                                IR_Const_1_ , IR_Const_0_  );
////      jit_insn_branch_if(f, resJitVal, jitLabel);
          IR_insn_branch_if_not_zero( Bldr, tRes, IR_Blk_Label );

          break;
        }

        case PCIT::NNB_SPECIAL_NULLS_MBIN32S_MATTR3_MATTR3_IBIN32S_IBIN32S:
        {
          PCodeOperand * src1,      * src2,      * res ;
          jit_value_t    src1JitVal,  src2JitVal       ;
          jit_type_t     src1IRtype,  src2IRtype       ;
          p_IR_block_t * IR_Blk_Label = NULL           ;

////      jit_label_t src1LabelNull = jit_label_undefined;
////      jit_label_t ret1Label = jit_label_undefined;
////      jit_label_t endLabel = jit_label_undefined;
          p_IR_block_t src1LabelNull = BasicBlock::Create( LLContxt, "NNB_SPEC_src1NullBlk" );
          p_IR_block_t ret0Label     = BasicBlock::Create( LLContxt, "NNB_SPEC_ret0Blk" );
          p_IR_block_t endLabel      = BasicBlock::Create( LLContxt, "NNB_SPEC_endBlk" );

          DPT3( "VV93000: ", VV_XD,
                "src1LabelNull = %p, ret0Label = %p, endLabel = %p\n",
                 src1LabelNull,      ret0Label,      endLabel );

////      jitLabel = block->getTargetBlock()->getJitLabel();
          IR_Blk_Label = PCBlk->getTargetBlock()->getJitBlkLbl();

////      src1 = inst->getROps()[0];
          src1 = PCInst->getROps()[0];

////      src2 = inst->getROps()[1];
          src2 = PCInst->getROps()[1];

////      res  = inst->getWOps()[0];
          res  = PCInst->getWOps()[0];
          PRINT_DETAILS_OF_RDOp( 0, src1 );
          PRINT_DETAILS_OF_RDOp( 1, src2 );
          PRINT_DETAILS_OF_WROp( 0, res  );

////      src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src1IRtype = src1->getJitType( this );
          src1JitVal = src1->getJitValue(this, Bldr, src1IRtype, PCBlk);

////      src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);
          src2IRtype = src2->getJitType( this );
          src2JitVal = src2->getJitValue(this, Bldr, src2IRtype, PCBlk);

////      jit_insn_branch_if(f, src1JitVal, &src1LabelNull);
          IR_insn_branch_if_not_zero( Bldr, src1JitVal, &src1LabelNull );

////      jit_insn_branch_if(f, src2JitVal, &ret1Label);  <<< RETURNING 1 WAS A BUG
          IR_insn_branch_if_not_zero( Bldr, src2JitVal, &ret0Label );

          // Both are not null - store 0 and take target branch
////      res->storeJitValue(this, f, jit_type_int, zeroJitVal_, block);
          res->storeJitValue(this, Bldr, int32Ty_, IR_Const_0_, PCBlk);

////      jit_insn_branch(f, jitLabel);
          DPT1( "VV93005: ", VV_VD,
         "Bldr->CreateBr( *IR_Blk_Label = %p )\n", *IR_Blk_Label );
          Bldr->CreateBr( *IR_Blk_Label );

          // src1 is null - what about src2?
////      jit_insn_label(f, &src1LabelNull);

// NOTE: We don't need to "complete the current block" since we just generated
//       an unconditional branch.  In fact, LLVM's JIT Compiler aborts if we do
//       add an additional unconditional branch here.

          DPT1( "VV93020: ", VV_VD,
         "ExpFn->getBasicBlockList().push_back( src1LabelNull = %p )\n", src1LabelNull );
          ExpFn->getBasicBlockList().push_back( src1LabelNull );

          DPT1( "VV93030: ", VV_VD,
         "Bldr->SetInsertPoint( src1LabelNull = %p )\n", src1LabelNull );
          Bldr->SetInsertPoint( src1LabelNull );

////      jit_insn_branch_if(f, src2JitVal, &ret1Label); <<<< RETURNING 1 WAS A BUG
          IR_insn_branch_if_zero( Bldr, src2JitVal, &ret0Label );

          // src2 is also null, so return 1
////      res->storeJitValue(this, f, jit_type_int, zeroJitVal_, block);
          res->storeJitValue(this, Bldr, int32Ty_, IR_Const_1_, PCBlk);

////      jit_insn_branch(f, &endLabel);

          DPT1( "VV93040: ", VV_VD,
         "Bldr->CreateBr( endLabel = %p )\n", endLabel );
          Bldr->CreateBr( endLabel );

          // Return 1
////      jit_insn_label(f, &ret1Label);

// NOTE: We don't need to "complete the current block" since we just generated
//       an unconditional branch.  In fact, LLVM's JIT Compiler aborts if we do
//       add an additional unconditional branch here.

          DPT1( "VV93060: ", VV_VD,
         "ExpFn->getBasicBlockList().push_back( ret0Label = %p )\n", ret0Label );
          ExpFn->getBasicBlockList().push_back( ret0Label );

          DPT1( "VV93070: ", VV_VD,
         "Bldr->SetInsertPoint( ret0Label = %p )\n", ret0Label );
          Bldr->SetInsertPoint( ret0Label );

////      res->storeJitValue(this, f, jit_type_int, oneJitVal_, block); <<< THIS WAS A BUG
          res->storeJitValue(this, Bldr, int32Ty_, IR_Const_0_, PCBlk);

          // End label to jump to
////      jit_insn_label(f, &endLabel);

          DPT1( "VV93080: ", VV_VD,
         "Bldr->CreateBr( endLabel = %p )\n", endLabel );
          Bldr->CreateBr( endLabel );  // Must complete the current block

          DPT1( "VV93090: ", VV_VD,
         "ExpFn->getBasicBlockList().push_back( endLabel = %p )\n", endLabel );
          ExpFn->getBasicBlockList().push_back( endLabel );

          DPT1( "VV93098: ", VV_VD,
         "Bldr->SetInsertPoint( endLabel = %p )\n", endLabel );
          Bldr->SetInsertPoint( endLabel );

          break;
        }

        case PCIT::NNB_MATTR3_IBIN32S:
        {
          PCodeOperand * src1                ;
          jit_value_t    src1JitVal          ;
          jit_type_t     src1IRtype          ;
          p_IR_block_t * IR_Blk_Label = NULL ;

          src1 = PCInst->getROps()[0]; 
          PRINT_DETAILS_OF_RDOp( 0, src1 );

////      src1JitVal = src1->getJitValue(this, f, src1->getJitType(), PCBlk);

          src1IRtype = src1->getJitType( this );
          src1JitVal = src1->getJitValue(this, Bldr, src1IRtype, PCBlk);

////      jitLabel = PCBlk->getTargetBlock()->getJitLabel();
          IR_Blk_Label = PCBlk->getTargetBlock()->getJitBlkLbl() ;

////      jit_insn_branch_if_not(f, src1JitVal, jitLabel);
          IR_insn_branch_if_zero( Bldr, src1JitVal, IR_Blk_Label );

          break;
        }

        case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
        case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
        {
          PCodeOperand * src1,      * src2,     * res         ;
          jit_type_t     src1IRtype,  src2IRtype              ;
          jit_value_t    src1JitVal,  src2JitVal, orResJitVal ;
          p_IR_block_t * IR_Blk_Label = NULL                  ;

          src1 = PCInst->getROps()[0]; 
          src2 = PCInst->getROps()[1]; 
          res  = PCInst->getWOps()[0];
          PRINT_DETAILS_OF_RDOp( 0, src1 );
          PRINT_DETAILS_OF_RDOp( 1, src2 );
          PRINT_DETAILS_OF_WROp( 0, res );

          NABoolean forComp = (opc ==
            PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S);

////      src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src1IRtype = src1->getJitType( this );
          src1JitVal = src1->getJitValue(this, Bldr, src1IRtype, PCBlk);

////      src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);
          src2IRtype = src2->getJitType( this );
          src2JitVal = src2->getJitValue(this, Bldr, src2IRtype, PCBlk);

////      orRes = jit_insn_or(f, src1JitVal, src2JitVal);

          Int32 src1Width = src1IRtype->getIntegerBitWidth();
          Int32 src2Width = src2IRtype->getIntegerBitWidth();
          if ( src1Width < src2Width )
          {
             DPT2( "VV94102: ", VV_VD,
                   "src1Width=%d < src2Width=%d\n", src1Width, src2Width);

             DPT0( "VV94103: ", VV_VD,
            "src1JitVal = Bldr->CreateZExt( src1JitVal, (IntegerType *)src2IRtype, 'ZExtArg1' )\n" );
             src1JitVal = Bldr->CreateZExt( src1JitVal,
                               (IntegerType *)src2IRtype, "ZExtArg1" ) ;
          }
          if ( src1Width > src2Width )
          {
             DPT2( "VV94105: ", VV_VD, "src1Width=%d > src2Width=%d\n",
                                        src1Width,     src2Width);
             DPT0( "VV94106: ", VV_VD,
            "src2JitVal = Bldr->CreateZExt( src2JitVal, (IntegerType *)src1IRtype, 'ZExtArg2' )\n" );
             src2JitVal = Bldr->CreateZExt( src2JitVal,
                               (IntegerType *)src1IRtype, "ZExtArg2" ) ;
          }

          DPT0( "VV94110: ", VV_VD,
         "orResJitVal = Bldr->CreateOr( src1JitVal, src2JitVal, 'BitwiseOr' )\n" );
          orResJitVal = Bldr->CreateOr( src1JitVal, src2JitVal, "BitwiseOr" );

////      jitLabel = block->getTargetBlock()->getJitLabel();
          IR_Blk_Label = PCBlk->getTargetBlock()->getJitBlkLbl() ;

          if (res->isVar() || PCBlk->isOperandLiveInSuccs(res)) {
////        res->storeNullJitValueAndBranch(this, f, orRes, jitLabel,
////                                        forComp, block);
            res->storeNullJitValueAndBranch(this, Bldr, orResJitVal,
                                      *IR_Blk_Label, forComp, PCBlk);
          }
          else {
////        jit_insn_branch_if_not(f, orRes, jitLabel);
            IR_insn_branch_if_zero( Bldr, orResJitVal, IR_Blk_Label );
          }

          break;
        }

        case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S:
        case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_IATTR3_IBIN32S:
        {
          PCodeOperand * src1,      * res    ;
          jit_type_t     src1IRtype          ;
          p_IR_block_t * IR_Blk_Label = NULL ;

          src1 = PCInst->getROps()[0]; 
          res  = PCInst->getWOps()[0];

          PRINT_DETAILS_OF_RDOp( 0, src1 );
          PRINT_DETAILS_OF_WROp( 0, res );

          NABoolean forComp =
            (opc == PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S);

//        src1JitVal = src1->getJitValue(this, f, src1->getJitType(), PCBlk);

          src1IRtype =  src1->getJitType( this );

          DPT1( "VV95010: ", VV_XD,
                "In NOT_NULL_BRANCH_MBIN32S... src1's Type ID is %d\n",
                                       (int) src1IRtype->getTypeID() );
#if NExprDbgLvl >= VV_XD
          if ( src1IRtype->isPointerTy() )
             DPT0( "VV95011: ", VV_XD,
                   "In NOT_NULL_BRANCH_MBIN32S ... src1IRtype->isPointerTy()"
                   " returned TRUE.\n" ) ;

          if ( src1IRtype->isPointerTy() )
             DPT1( "VV95012: ", VV_XD,
               "In NOT_NULL_BRANCH_MBIN32S... src1's Contained Type ID is %d\n",
                           (int) src1IRtype->getContainedType(0)->getTypeID() );
#endif

          jit_value_t src1JitVal = src1->getJitValue(this, Bldr, src1IRtype, PCBlk);

          DPT1( "VV95014: ", VV_XD,
                       "In NOT_NULL_BRANCH_MBIN32S... Val32's Type ID is %d\n",
                                    (int) src1JitVal->getType()->getTypeID() );

////      jitLabel = PCBlk->getTargetBlock()->getJitLabel();
          IR_Blk_Label = PCBlk->getTargetBlock()->getJitBlkLbl() ;

          if (res->isVar() || PCBlk->isOperandLiveInSuccs(res)) {
//          res->storeNullJitValueAndBranch(this, f, src1JitVal, jitLabel,
//                                          forComp, PCBlk);
            res->storeNullJitValueAndBranch(this, Bldr, src1JitVal, *IR_Blk_Label,
                                            forComp, PCBlk);
          }
          else {
//          jit_insn_branch_if_not(f, src1JitVal, jitLabel);
            IR_insn_branch_if_zero( Bldr, src1JitVal, IR_Blk_Label );
          }

          break;
        }

        case PCIT::FILL_MEM_BYTES_VARIABLE:
        case PCIT::FILL_MEM_BYTES:
        {
          PCodeOperand *res       ;
          jit_value_t   resJitVal ;
          jit_type_t    resIRtype ;

#ifdef NO_FILL_MEM_BYTES
          break;
#endif
          Int32 fillVal, fillLen;

          res = PCInst->getWOps()[0];

#if NExprDbgLvl >= VV_BD
          PRINT_DETAILS_OF_WROp( 0, res );

          if ( res->getStackIndex() == 2 )
          {
             if ( opc == PCIT::FILL_MEM_BYTES )
             {
                DPT0( "VV95101: ", VV_VD,
                      "found PCIT::FILL_MEM_BYTES: WITH stackIndex_ = 2\n" );
             }
             else
             {
                DPT0( "VV95101: ", VV_VD,
                      "found PCIT::FILL_MEM_BYTES_VARIABLE: WITH stackIndex_ = 2\n" );
             }
           }
#endif

          // Also, no need to zero out temporary operands, since we just care
          // about their values being appropriately nulled out.
          if (res->isTemp())
            break;

////      resJitVal = res->getJitValue(this, f, res->getJitType(), PCBlk);
          resIRtype = res->getJitType( this );
          resJitVal = res->getJitValue(this, Bldr, resIRtype, PCBlk);

          if (opc == PCIT::FILL_MEM_BYTES_VARIABLE)
          {
            jit_value_t lenJitVal;

            Int32 vcLen = res->getVcIndicatorLen();
////        jit_type_t vcType = (vcLen == 2) ? jit_type_ushort : jit_type_uint;
            jit_type_t vcType = (vcLen == 2) ? int16Ty_ : int32Ty_ ;

////        lenJitVal = jit_value_create_nint_constant(f, vcType,inst->code[6]);
            lenJitVal = ConstantInt::get( vcType , PCInst->code[6] );
            
            // Write out vc length first
////        jit_insn_store_relative(f, resJitVal, 0, lenJitVal);

            DPT0( "VV95110: ", VV_XD,
           "IR_StoreRelativeWithType( Bldr, lenJitVal, resJitVal, 0, vcType )\n" );
            IR_StoreRelativeWithType( Bldr, lenJitVal, resJitVal, 0, vcType );

            // Move pointer up
////        resJitVal = jit_insn_add_relative(f, resJitVal, vcLen);

            jit_value_t vcLenVal = ConstantInt::get( int64Ty_ , vcLen ) ;

            DPT1( "VV95112: ", VV_VD,
           "resJitVal = Bldr->CreateGEP( resJitVal, %d )\n", vcLen );
            resJitVal = Bldr->CreateGEP( resJitVal, vcLenVal );

            // Reduce fill length to no longer include vc length indicator
            fillLen = PCInst->code[6] - vcLen;
            fillVal = PCInst->code[7];

            DPT2( "VV95130: ", VV_XD,
                  "FILL_MEM_BYTES_VARIABLE: Fill Value=0x%x, Fill Len = %d\n",
                                                    fillVal, fillLen );
          }
          else {
            fillLen = PCInst->code[3];
            fillVal = PCInst->code[4];

            DPT2( "VV95140: ", VV_XD,
                  "FILL_MEM_BYTES: Fill Value=0x%x, Fill Len = %d\n",
                                           fillVal, fillLen );
          }

          // FIXME: If varchar, we currently set the vc length to the max length
          // of the varchar, but does that make sense if the value is NULL?
          // This only gets called when the the varchar is NULL, and so it's
          // length will always be zero, so why write it out?.

////      genUnalignedMemset(f, resJitVal, fillVal, fillLen);
          genUnalignedMemset( Bldr, resJitVal, fillVal, fillLen );

          break;
        }

        case PCIT::EQ_MBIN32S_MASCII_MASCII:
        case PCIT::NE_MBIN32S_MASCII_MASCII:
        case PCIT::LT_MBIN32S_MASCII_MASCII:
        case PCIT::GT_MBIN32S_MASCII_MASCII:
        case PCIT::LE_MBIN32S_MASCII_MASCII:
        case PCIT::GE_MBIN32S_MASCII_MASCII:
        {
          PCodeOperand *src1,      *src2,      *res ;
          jit_value_t   src1JitVal, src2JitVal      ;
          jit_type_t    src1IRtype, src2IRtype      ;

          res  = PCInst->getWOps()[0];
          src1 = PCInst->getROps()[0];
          src2 = PCInst->getROps()[1];
          PRINT_DETAILS_OF_RDOp( 0, src1 );
          PRINT_DETAILS_OF_RDOp( 1, src2 );
          PRINT_DETAILS_OF_WROp( 0, res  );

          p_IR_block_t falseLabel = BasicBlock::Create( LLContxt, "XX_M32_MA_MA_falseBlk" );
          p_IR_block_t trueLabel  = BasicBlock::Create( LLContxt, "XX_M32_MA_MA_trueBlk" );
          p_IR_block_t endLabel   = BasicBlock::Create( LLContxt, "XX_M32_MA_MA_endBlk" );

          DPT0( "VV95150: ", VV_XD, "In Cmp??_MBIN32S_MASCII_MASCII \n" );

          Int32  MaxlenInt32 = src1->getLen() ;
          jit_value_t Maxlen = ConstantInt::get( int32Ty_, MaxlenInt32 );

          enum cmpKind KindOfCmp = ByteCompare_EQ ; 

          switch ( opc )
          {
             case PCIT::EQ_MBIN32S_MASCII_MASCII:
                        { break ; }
             case PCIT::NE_MBIN32S_MASCII_MASCII:
                        { KindOfCmp = ByteCompare_NE ; break ; }
             case PCIT::LT_MBIN32S_MASCII_MASCII:
                        { KindOfCmp = ByteCompare_LT ; break ; }
             case PCIT::LE_MBIN32S_MASCII_MASCII:
                        { KindOfCmp = ByteCompare_LE ; break ; }
             case PCIT::GT_MBIN32S_MASCII_MASCII:
                        { KindOfCmp = ByteCompare_GT ; break ; }
             case PCIT::GE_MBIN32S_MASCII_MASCII:
                        { KindOfCmp = ByteCompare_GE ; break ; }
          }
          // For fixed-length strings, do first few bytes "by hand"
          // so that we use src1JitVal's jitValue_ field!  This is
          // important so LLVM will believe the src fields have been
          // accessed.  Otherwise, subsequent PCODE instructions that
          // reference the same operand value may cause LLVM to abort -- thinking
          // that one path through the PCODE instructions fetched the
          // first few bytes while another path did not.
          // NOTE: We put the result of this comparison in IsZero1, but
          // then we ignore it!  What's important is to generate the fetch!
          // 
          // If src1 has non-null jitValue_
          if ( jit_value_t src1CurrJitVal = src1->getCurrJitVal() )
          {
             if ( ( src1CurrJitVal->getType() == int64Ty_ ) &&
                  ( MaxlenInt32 >= 8 ) )
             {
                jit_value_t Maxlen64 = ConstantInt::get( int64Ty_, MaxlenInt32 );

                DPT0( "VV95154: ", VV_VD,
               "src1JitVal = src1->getJitValue( this, Bldr, int64Ty_, PCBlk )\n");
                src1JitVal = src1->getJitValue( this, Bldr, int64Ty_, PCBlk );

                DPT0( "VV95155: ", VV_VD,
               "jit_value_t IsZero1  = Bldr->CreateICmpEQ( src1JitVal, Maxlen64, 'junkCk' )\n" );
                jit_value_t IsZero1  = Bldr->CreateICmpEQ( src1JitVal, Maxlen64, "junkCk" );
             }
             else if ( ( src1CurrJitVal->getType() == int32Ty_ ) &&
                  ( MaxlenInt32 >= 4 ) )
             {
                DPT0( "VV95156: ", VV_VD,
               "src1JitVal = src1->getJitValue( this, Bldr, int32Ty_, PCBlk )\n");
                src1JitVal = src1->getJitValue( this, Bldr, int32Ty_, PCBlk );

                DPT0( "VV95157: ", VV_VD,
               "jit_value_t IsZero1  = Bldr->CreateICmpEQ( src1JitVal, Maxlen, 'junkCk' )\n" );
                jit_value_t IsZero1  = Bldr->CreateICmpEQ( src1JitVal, Maxlen, "junkCk" );
             }
             else if ( ( src1CurrJitVal->getType() == int16Ty_ ) &&
                  ( MaxlenInt32 >= 2 ) )
             {
                jit_value_t Maxlen16 = ConstantInt::get( int16Ty_, MaxlenInt32 );

                DPT0( "VV95158: ", VV_VD,
               "src1JitVal = src1->getJitValue( this, Bldr, int16Ty_, PCBlk )\n");
                src1JitVal = src1->getJitValue( this, Bldr, int16Ty_, PCBlk );

                DPT0( "VV95159: ", VV_VD,
               "jit_value_t IsZero1  = Bldr->CreateICmpEQ( src1JitVal, Maxlen16, 'junkCk' )\n" );
                jit_value_t IsZero1  = Bldr->CreateICmpEQ( src1JitVal, Maxlen16, "junkCk" );
             }
          }

          src1IRtype = src1->getJitType( this );
          src1JitVal = src1->getJitValue(this, Bldr, src1IRtype, PCBlk);
          src2IRtype = src2->getJitType( this );
          src2JitVal = src2->getJitValue(this, Bldr, src2IRtype, PCBlk);

          jit_value_t IsZero = genMemCmpLoop( Bldr
                                            , KindOfCmp
                                            , src1JitVal
                                            , src2JitVal
                                            , Maxlen
                                            ) ;
          DPT2( "VV95160: ", VV_VD,
         "Bldr->CreateCondBr( IsZero, trueLabel = %p, falseLabel = %p )\n", trueLabel, falseLabel );
          Bldr->CreateCondBr( IsZero, trueLabel, falseLabel );

          DPT1( "VV95162: ", VV_VD,
         "ExpFn->getBasicBlockList().push_back( trueLabel = %p )\n", trueLabel );
          ExpFn->getBasicBlockList().push_back( trueLabel );

          DPT1( "VV95164: ", VV_VD,
         "Bldr->SetInsertPoint( trueLabel = %p )\n", trueLabel );
          Bldr->SetInsertPoint( trueLabel );

          res->storeJitValue(this, Bldr, int32Ty_, IR_Const_1_, PCBlk);

          DPT1( "VV95166: ", VV_VD,
         "Bldr->CreateBr( endLabel = %p )\n", endLabel );
          Bldr->CreateBr( endLabel );

          DPT1( "VV95168: ", VV_VD,
         "ExpFn->getBasicBlockList().push_back( falseLabel = %p )\n", falseLabel );
          ExpFn->getBasicBlockList().push_back( falseLabel );

          DPT1( "VV95170: ", VV_VD,
         "Bldr->SetInsertPoint( falseLabel = %p )\n", falseLabel );
          Bldr->SetInsertPoint( falseLabel );

          res->storeJitValue(this, Bldr, int32Ty_, IR_Const_0_, PCBlk);

          DPT1( "VV95172: ", VV_VD,
         "Bldr->CreateBr( endLabel = %p )\n", endLabel );
          Bldr->CreateBr( endLabel );  // Must complete the current block

          DPT1( "VV95174: ", VV_VD,
         "ExpFn->getBasicBlockList().push_back( endLabel = %p )\n", endLabel );
          ExpFn->getBasicBlockList().push_back( endLabel );

          DPT1( "VV95179: ", VV_VD,
         "Bldr->SetInsertPoint( endLabel = %p )\n", endLabel );
          Bldr->SetInsertPoint( endLabel );

          break;
        }

        case PCIT::EQ_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::EQ_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::EQ_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::EQ_MBIN32S_MBIN16U_MBIN16U:
        case PCIT::EQ_MBIN32S_MBIN16U_MBIN32U:
        case PCIT::EQ_MBIN32S_MBIN32U_MBIN32U:
        case PCIT::EQ_MBIN32S_MBIN16U_MBIN16S:
        case PCIT::EQ_MBIN32S_MBIN64S_MBIN64S:
        {
          PCodeOperand *src1,      *src2,      *res       ;
          jit_value_t   src1JitVal, src2JitVal, resJitVal ;
          jit_type_t    src1IRtype, src2IRtype            ;

          src1 = PCInst->getROps()[0];
          src2 = PCInst->getROps()[1];
          res  = PCInst->getWOps()[0];

          PRINT_DETAILS_OF_RDOp( 0, src1 );
          PRINT_DETAILS_OF_RDOp( 1, src2 );
          PRINT_DETAILS_OF_WROp( 0, res  );

#if NExprDbgLvl >= VV_VD
          if (src1 == src2)
             DPT0( "VV96002: ", VV_XD,
                   "In EQ_MBIN32S_MBINxxS_MBINxxS: src1 == src2 !!!\n");
#endif

////      src1JitVal = src1->getJitValue(this, f, src1->getJitType(), PCBlk);
          src1IRtype = src1->getJitType( this );

          DPT1( "VV96004: ", VV_XD,
                "In EQ_MBIN32S_MBINxxS_MBINxxS: src1IRtype Width = %d\n",
                                      src1IRtype->getIntegerBitWidth() );

          src1JitVal = src1->getJitValue(this, Bldr, src1IRtype, PCBlk);

////      src2JitVal = src2->getJitValue(this, f, src2->getJitType(), PCBlk);
          src2IRtype = src2->getJitType( this );

          DPT1( "VV96006: ", VV_XD,
                "In EQ_MBIN32S_MBINxxS_MBINxxS: src2IRtype Width = %d\n",
                                       src2IRtype->getIntegerBitWidth() );

          src2JitVal = src2->getJitValue(this, Bldr, src2IRtype, PCBlk);

////      resJitVal = jit_insn_eq(f, src1JitVal, src2JitVal);
          if ( opc == PCIT::EQ_MBIN32S_MBIN16S_MBIN32S )
          {
             DPT0( "VV96010: ", VV_VD,
            "src1JitVal = Bldr->CreateSExt(src1JitVal, (IntegerType *)src2IRtype, 'SExt' )\n" );
             src1JitVal = Bldr->CreateSExt( src1JitVal,
                                    (IntegerType *)src2IRtype, "SExt" ) ;
          }
          if ( opc == PCIT::EQ_MBIN32S_MBIN16U_MBIN32U )
          {
             DPT0( "VV96020: ", VV_VD,
            "src1JitVal = Bldr->CreateZExt( src1JitVal, (IntegerType *)src2IRtype, 'ZExt' )\n" );
             src1JitVal = Bldr->CreateZExt( src1JitVal,
                                    (IntegerType *)src2IRtype, "ZExt" ) ;
          }
          resJitVal = jitGenCompare( Bldr, IntCompare_EQ,
                                     src1JitVal,   src2JitVal,
                                     IR_Const_1_ , IR_Const_0_  );

          DPT0( "VV96024: ", VV_XD,
                "In EQ_MBIN32S_MBINxxS_MBINxxS: Returned from jitGenCompare\n");

////      inst->getWOps()[0]->setJitValue(this, f, resJitVal, PCBlk);
          res->setJitValue(this, Bldr, resJitVal, PCBlk);

          DPT0( "VV96026: ", VV_XD,
                "In EQ_MBIN32S_MBINxxS_MBINxxS: Returned from setJitValue\n");
          break;
        }

        case PCIT::LT_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::LT_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::LT_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::LT_MBIN32S_MBIN64S_MBIN64S:
        case PCIT::LT_MBIN32S_MBIN16U_MBIN16U:
        case PCIT::LT_MBIN32S_MBIN16U_MBIN32U:
        case PCIT::LT_MBIN32S_MBIN32U_MBIN32U:
        {
          PCodeOperand *src1,      *src2,      *res       ;
          jit_value_t   src1JitVal, src2JitVal, resJitVal ;
          jit_type_t    src1IRtype, src2IRtype            ;

          src1 = PCInst->getROps()[0];
          src2 = PCInst->getROps()[1];
          res  = PCInst->getWOps()[0];
          PRINT_DETAILS_OF_RDOp( 0, src1 );
          PRINT_DETAILS_OF_RDOp( 1, src2 );
          PRINT_DETAILS_OF_WROp( 0, res  );

////      src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src1IRtype = src1->getJitType( this );
          src1JitVal = src1->getJitValue(this, Bldr, src1IRtype, PCBlk);

////      src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);
          src2IRtype = src2->getJitType( this );
          src2JitVal = src2->getJitValue(this, Bldr, src2IRtype, PCBlk);

////      resJitVal = jit_insn_lt(f, src1JitVal, src2JitVal);
          if ( opc == PCIT::LT_MBIN32S_MBIN16S_MBIN32S )
          {
             DPT0( "VV96510: ", VV_VD,
            "src1JitVal = Bldr->CreateSExt( src1JitVal, (IntegerType *)src2IRtype, 'SExt' )\n" );
             src1JitVal = Bldr->CreateSExt( src1JitVal,
                                    (IntegerType *)src2IRtype, "SExt" ) ;
          }
          if ( opc == PCIT::LT_MBIN32S_MBIN16U_MBIN32U )
          {
             DPT0( "VV96520: ", VV_VD,
            "src1JitVal = Bldr->CreateZExt( src1JitVal, (IntegerType *)src2IRtype, 'ZExt' )\n" );
             src1JitVal = Bldr->CreateZExt( src1JitVal,
                                    (IntegerType *)src2IRtype, "ZExt" ) ;
          }

          enum cmpKind KindOfCmp = IntCompare_SLT ; 
          switch (opc)
          {
             case PCIT::LT_MBIN32S_MBIN16U_MBIN16U:
             case PCIT::LT_MBIN32S_MBIN16U_MBIN32U:
             case PCIT::LT_MBIN32S_MBIN32U_MBIN32U:
                  KindOfCmp = IntCompare_ULT ;
                  break;
             default:
                  break;
          }
          resJitVal = jitGenCompare( Bldr, KindOfCmp,
                                     src1JitVal,   src2JitVal,
                                     IR_Const_1_ , IR_Const_0_  );

////      inst->getWOps()[0]->setJitValue(this, f, resJitVal, block);
          res->setJitValue(this, Bldr, resJitVal, PCBlk);

          break;
        }

        case PCIT::LE_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::LE_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::LE_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::LE_MBIN32S_MBIN16U_MBIN16U:
        case PCIT::LE_MBIN32S_MBIN16U_MBIN32U:
        case PCIT::LE_MBIN32S_MBIN32U_MBIN32U:
        case PCIT::LE_MBIN32S_MBIN64S_MBIN64S:
        {
          PCodeOperand *src1,      *src2,      *res       ;
          jit_value_t   src1JitVal, src2JitVal, resJitVal ;
          jit_type_t    src1IRtype, src2IRtype            ;

          src1 = PCInst->getROps()[0];
          src2 = PCInst->getROps()[1];
          res  = PCInst->getWOps()[0];
          PRINT_DETAILS_OF_RDOp( 0, src1 );
          PRINT_DETAILS_OF_RDOp( 1, src2 );
          PRINT_DETAILS_OF_WROp( 0, res  );

////      src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src1IRtype = src1->getJitType( this );
          src1JitVal = src1->getJitValue(this, Bldr, src1IRtype, PCBlk);

////      src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);
          src2IRtype = src2->getJitType( this );
          src2JitVal = src2->getJitValue(this, Bldr, src2IRtype, PCBlk);

////      resJitVal = jit_insn_le(f, src1JitVal, src2JitVal);

          if ( opc == PCIT::LE_MBIN32S_MBIN16S_MBIN32S )
          {
             DPT0( "VV96610: ", VV_VD,
            "src1JitVal = Bldr->CreateSExt( src1JitVal, (IntegerType *)src2IRtype, 'SExt' )\n" );
             src1JitVal = Bldr->CreateSExt( src1JitVal,
                                    (IntegerType *)src2IRtype, "SExt" ) ;
          }
          if ( opc == PCIT::LE_MBIN32S_MBIN16U_MBIN32U )
          {
             DPT0( "VV96620: ", VV_VD,
            "src1JitVal = Bldr->CreateZExt( src1JitVal, (IntegerType *)src2IRtype, 'ZExt' )\n" );
             src1JitVal = Bldr->CreateZExt( src1JitVal,
                                    (IntegerType *)src2IRtype, "ZExt" ) ;
          }

          enum cmpKind KindOfCmp = IntCompare_SLE ; 
          switch (opc)
          {
             case PCIT::LE_MBIN32S_MBIN16U_MBIN16U:
             case PCIT::LE_MBIN32S_MBIN16U_MBIN32U:
             case PCIT::LE_MBIN32S_MBIN32U_MBIN32U:
                  KindOfCmp = IntCompare_ULE ;
                  break;
             default:
                  break;
          }
          resJitVal = jitGenCompare( Bldr, KindOfCmp,
                                     src1JitVal,   src2JitVal,
                                     IR_Const_1_ , IR_Const_0_  );

////      inst->getWOps()[0]->setJitValue(this, f, resJitVal, block);
          res->setJitValue(this, Bldr, resJitVal, PCBlk);

          break;
        }

        case PCIT::GT_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::GT_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::GT_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::GT_MBIN32S_MBIN16U_MBIN16U:
        case PCIT::GT_MBIN32S_MBIN16U_MBIN32U:
        case PCIT::GT_MBIN32S_MBIN32U_MBIN32U:
        case PCIT::GT_MBIN32S_MBIN64S_MBIN64S:
        {
          PCodeOperand *src1,      *src2,      *res       ;
          jit_value_t   src1JitVal, src2JitVal, resJitVal ;
          jit_type_t    src1IRtype, src2IRtype            ;

          src1 = PCInst->getROps()[0];
          src2 = PCInst->getROps()[1];
          res  = PCInst->getWOps()[0];
          PRINT_DETAILS_OF_RDOp( 0, src1 );
          PRINT_DETAILS_OF_RDOp( 1, src2 );
          PRINT_DETAILS_OF_WROp( 0, res  );

////      src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src1IRtype = src1->getJitType( this );
          src1JitVal = src1->getJitValue(this, Bldr, src1IRtype, PCBlk);

////      src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);
          src2IRtype = src2->getJitType( this );
          src2JitVal = src2->getJitValue(this, Bldr, src2IRtype, PCBlk);

////      resJitVal = jit_insn_gt(f, src1JitVal, src2JitVal);
          if ( opc == PCIT::GT_MBIN32S_MBIN16S_MBIN32S )
          {
             DPT0( "VV96710: ", VV_VD,
            "src1JitVal = Bldr->CreateSExt( src1JitVal, (IntegerType *)src2IRtype, 'SExt' )\n" );
             src1JitVal = Bldr->CreateSExt( src1JitVal,
                                    (IntegerType *)src2IRtype, "SExt" ) ;
          }
          if ( opc == PCIT::GT_MBIN32S_MBIN16U_MBIN32U )
          {
             DPT0( "VV96720: ", VV_VD,
            "src1JitVal = Bldr->CreateZExt( src1JitVal, (IntegerType *)src2IRtype, 'ZExt' )\n" );
             src1JitVal = Bldr->CreateZExt( src1JitVal,
                                    (IntegerType *)src2IRtype, "ZExt" ) ;
          }

          enum cmpKind KindOfCmp = IntCompare_SGT ; 
          switch (opc)
          {
             case PCIT::GT_MBIN32S_MBIN16U_MBIN16U:
             case PCIT::GT_MBIN32S_MBIN16U_MBIN32U:
             case PCIT::GT_MBIN32S_MBIN32U_MBIN32U:
                  KindOfCmp = IntCompare_UGT ;
                  break;
             default:
                  break;
          }
          resJitVal = jitGenCompare( Bldr, KindOfCmp,
                                     src1JitVal,   src2JitVal,
                                     IR_Const_1_ , IR_Const_0_  );

////      inst->getWOps()[0]->setJitValue(this, f, resJitVal, block);
          res->setJitValue(this, Bldr, resJitVal, PCBlk);

          break;
        }

        case PCIT::GE_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::GE_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::GE_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::GE_MBIN32S_MBIN16U_MBIN16U:
        case PCIT::GE_MBIN32S_MBIN16U_MBIN32U:
        case PCIT::GE_MBIN32S_MBIN32U_MBIN32U:
        case PCIT::GE_MBIN32S_MBIN64S_MBIN64S:
        {
          PCodeOperand *src1,      *src2,      *res       ;
          jit_value_t   src1JitVal, src2JitVal, resJitVal ;
          jit_type_t    src1IRtype, src2IRtype            ;

          src1 = PCInst->getROps()[0];
          src2 = PCInst->getROps()[1];
          res  = PCInst->getWOps()[0];
          PRINT_DETAILS_OF_RDOp( 0, src1 );
          PRINT_DETAILS_OF_RDOp( 1, src2 );
          PRINT_DETAILS_OF_WROp( 0, res  );

////      src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src1IRtype = src1->getJitType( this );
          src1JitVal = src1->getJitValue(this, Bldr, src1IRtype, PCBlk);

////      src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);
          src2IRtype = src2->getJitType( this );
          src2JitVal = src2->getJitValue(this, Bldr, src2IRtype, PCBlk);

////      resJitVal = jit_insn_ge(f, src1JitVal, src2JitVal);
          if ( opc == PCIT::GE_MBIN32S_MBIN16S_MBIN32S )
          {
             DPT0( "VV96810: ", VV_VD,
            "src1JitVal = Bldr->CreateSExt( src1JitVal, (IntegerType *)src2IRtype, 'SExt' )\n" );
             src1JitVal = Bldr->CreateSExt( src1JitVal,
                                    (IntegerType *)src2IRtype, "SExt" ) ;
          }
          if ( opc == PCIT::GE_MBIN32S_MBIN16U_MBIN32U )
          {
             DPT0( "VV96820: ", VV_VD,
            "src1JitVal = Bldr->CreateZExt( src1JitVal, (IntegerType *)src2IRtype, 'ZExt' )\n" );
             src1JitVal = Bldr->CreateZExt( src1JitVal,
                                    (IntegerType *)src2IRtype, "ZExt" ) ;
          }

          enum cmpKind KindOfCmp = IntCompare_SGE ; 
          switch (opc)
          {
             case PCIT::GE_MBIN32S_MBIN16U_MBIN16U:
             case PCIT::GE_MBIN32S_MBIN16U_MBIN32U:
             case PCIT::GE_MBIN32S_MBIN32U_MBIN32U:
                  KindOfCmp = IntCompare_UGE ;
                  break;
             default:
                  break;
          }
          resJitVal = jitGenCompare( Bldr, KindOfCmp,
                                     src1JitVal,   src2JitVal,
                                     IR_Const_1_ , IR_Const_0_  );

////      inst->getWOps()[0]->setJitValue(this, f, resJitVal, block);
          res->setJitValue(this, Bldr, resJitVal, PCBlk);

          break;
        }

        case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN32S_MBIN32S:
        case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN64S:
        case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN32S:
        {
          PCodeOperand *src1,      *src2,      *src3,      *res       ;
          jit_value_t   src1JitVal, src2JitVal, src3JitVal, resJitVal ;
          jit_type_t    src1IRtype, src2IRtype, src3IRtype, resIRtype ;

////      jit_label_t sumLabel = jit_label_undefined;
////      jit_label_t doneLabel = jit_label_undefined;

          p_IR_block_t sumLabel  = BasicBlock::Create( LLContxt, "SUM_MATTR3sumBlk" );
          p_IR_block_t doneLabel = BasicBlock::Create( LLContxt, "SUM_MATTR3doneBlk" );

          DPT2( "VV96902: ", VV_XD, "sumLabel  = %p, doneLabel = %p\n",
                                     sumLabel,       doneLabel );


          // Must do getJitValue(...) calls *before* we do calls to
          // IR_insn_branch_if_[not]_zero().  If we call getJitValue(...)
          // after such a conditional branch and if a subsequent
          // PCODE instruction in the same PC block references the same
          // PCodeOperand, the jitValue_ member variable will be filled in
          // and we would fail to tell LLVM to generate a "load" instruction
          // for the path when we take the conditional branch.
          //
          // Note: LLVM's JIT Compiler is smart enough to rearrange the code
          // as appropriate to avoid actually doing the 'load' instructions
          // when they are not needed.
          //
          res  = PCInst->getWOps()[(PCInst->getWOps()).entries()-1];
          PRINT_DETAILS_OF_WROp(   (PCInst->getWOps()).entries()-1, res );

          src3 = PCInst->getROps()[(PCInst->getROps()).entries()-1];
          PRINT_DETAILS_OF_RDOp(   (PCInst->getROps()).entries()-1, src3 );

////      resJitVal = res->getJitValue(this, f, res->getJitType(), block);
          resIRtype = res->getJitType( this );
          resJitVal = res->getJitValue(this, Bldr, resIRtype, PCBlk);

////      src3JitVal = src3->getJitValue(this, f, src3->getJitType(), block);
          src3IRtype = src3->getJitType( this );
          src3JitVal = src3->getJitValue(this, Bldr, src3IRtype, PCBlk);

          // NOW, First check if src is null - if so, we just branch out and return.
          src2 = PCInst->getROps()[1];
          PRINT_DETAILS_OF_RDOp( 1, src2 );

          src2IRtype = src2->getJitType( this );
          src2JitVal = src2->getJitValue(this, Bldr, src2IRtype, PCBlk);

          if (!src2->isConst()) {
////        src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);
//          For LLVM, do nothing here as we already did this above.

            DPT0( "VV96904: ", VV_XD,
                  "GOT INTO if (!src2->isConst()) section.\n");

////        jit_insn_branch_if(f, src2JitVal, &doneLabel);
            IR_insn_branch_if_not_zero( Bldr, src2JitVal, &doneLabel );
          }
          else {
            // Skip check, since constant will be 0 (assert here to verify).
            DPT0( "VV96906: ", VV_XD,
                  "GOT INTO if (!src2->isConst()) section.\n");
          }


          // Now check if target is null.  If so, we just move the src into tgt.
          src1 = PCInst->getROps()[0];
          PRINT_DETAILS_OF_RDOp( 0, src1 );

////      src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src1IRtype = src1->getJitType( this );
          src1JitVal = src1->getJitValue(this, Bldr, src1IRtype, PCBlk);

          //
          // NOTE: MUST sign extend (or truncate) src3JitVal to the size
          // of the number of bits wide that resIRtype specifies.
          // FURTHERMORE, this MUST be done *BEFORE* we test src3JitVal
          // for zero and do any branching.  If we don't, the LLVM JIT
          // Compiler will abort with an assert indicating
          // "Can't find reaching def for virtreg".
          // Hitting that assert indicates that there are two or more paths
          // to an LLVM BasicBlock where not all of the paths have the
          // defined a temp variable (or not defined it in the same way).
          // In this particular case, src3JitVal could have two different
          // definitions (different bit widths) when we got to "sumLabel".
          //
          Int32 src1Width = src1IRtype->getIntegerBitWidth();
          Int32 src3Width = src3IRtype->getIntegerBitWidth();
          Int32 resWidth  =  resIRtype->getIntegerBitWidth();

          DPT3( "VV96910: ", VV_XD, "resWidth=%d, src1Width=%d, src3Width=%d\n",
                                     resWidth,    src1Width,    src3Width );
          if ( src3Width != resWidth )
          {
             DPT0( "VV96915: ", VV_VD,
            "src3JitVal = Bldr->CreateSExtOrTrunc( src3JitVal, (IntegerType *)resIRtype, 'SExtOrTr' )\n" );
             src3JitVal = Bldr->CreateSExtOrTrunc( src3JitVal,
                                           (IntegerType *)resIRtype, "SExtOrTr" ) ;
          }

          // If src1 indicates tgt is not null, take branch to sumLabel
////      jit_insn_branch_if_not(f, src1JitVal, &sumLabel);
          IR_insn_branch_if_zero( Bldr, src1JitVal, &sumLabel );


          // At this point, we know the tgt is null.  So, we set tgt's value to 0
          // and ADD src3's value by simply setting the value of target to src3's
          // value and clearing the null indicator for the tgt.
          // 
////      res->storeJitValue(this, f, res->getJitType(), src3JitVal, block);
          res->storeJitValue(this, Bldr, resIRtype, src3JitVal, PCBlk);

          // Clear null indicator for target (by setting src1's value to 0) & branch to doneLabel
////      src1->storeNullJitValueAndBranch(this, f, zeroJitVal_, &doneLabel,
////                                       FALSE, block);

          jit_value_t            Const_0 = IR_Const_16bit_0_ ;
          if ( src1Width == 32 ) Const_0 = IR_Const_0_ ;

          src1->storeNullJitValueAndBranch(this, Bldr, Const_0,
                                              doneLabel, FALSE, PCBlk);

          // NOTE: At runtime, the only way to get to sumLabel is by taking
          // the branch (above.)

          // Perform the sum here
////      jit_insn_label(f, &sumLabel);

          DPT1( "VV96920: ", VV_VD,
         "Bldr->CreateBr( sumLabel = %p )\n", sumLabel );
          Bldr->CreateBr( sumLabel );  // Must complete the current block

          DPT1( "VV96922: ", VV_VD,
         "ExpFn->getBasicBlockList().push_back( sumLabel = %p )\n", sumLabel );
          ExpFn->getBasicBlockList().push_back( sumLabel );

          DPT1( "VV96924: ", VV_VD,
         "Bldr->SetInsertPoint( sumLabel = %p )\n", sumLabel );
          Bldr->SetInsertPoint( sumLabel );

////      resJitVal = jit_insn_add(f, resJitVal, src3JitVal);

          DPT0( "VV96927: ", VV_VD,
         "resJitVal = Bldr->CreateAdd( resJitVal, src3JitVal, 'IntAdd' )\n" );
          resJitVal = Bldr->CreateAdd( resJitVal, src3JitVal, "IntAdd" );

////      res->setJitValue(this, f, resJitVal, block);
          res->setJitValue(this, Bldr, resJitVal, PCBlk);

////      jit_insn_label(f, &doneLabel);

          DPT1( "VV96930: ", VV_VD,
         "Bldr->CreateBr( doneLabel = %p )\n", doneLabel );
          Bldr->CreateBr( doneLabel );  // Must complete the current block

          DPT1( "VV96932: ", VV_VD,
         "ExpFn->getBasicBlockList().push_back( doneLabel = %p )\n", doneLabel );
          ExpFn->getBasicBlockList().push_back( doneLabel );

          DPT1( "VV96934: ", VV_VD,
         "Bldr->SetInsertPoint( doneLabel = %p )\n", doneLabel );
          Bldr->SetInsertPoint( doneLabel );

          // Lastly, clear out jit values for 32-bit source operand if needed.
          if ( resWidth > src3Width )
            src3->clearJitValues(this);

          break;
        }

        case PCIT::MOVE_MBIN16U_MBIN8:
        case PCIT::MOVE_MBIN32U_MBIN16U:
        case PCIT::MOVE_MBIN32S_MBIN16U:
        case PCIT::MOVE_MBIN64S_MBIN16U:
        case PCIT::MOVE_MBIN64S_MBIN32U:
        {
          PCodeOperand *src1,      *res       ;
          jit_value_t   src1JitVal            ;
          jit_type_t    src1IRtype, resIRtype ;

          src1 = PCInst->getROps()[0];
          res  = PCInst->getWOps()[0];
          PRINT_DETAILS_OF_RDOp( 0, src1 );
          PRINT_DETAILS_OF_WROp( 0, res );

          src1IRtype = src1->getJitType( this );

//        src1JitVal = src1->getJitValue(this, f, src1->getJitType(), PCBlk);
          src1JitVal = src1->getJitValue(this, Bldr, src1IRtype, PCBlk);
//        src1JitVal = jit_insn_convert(f, src1JitVal, res->getJitType(), 0);

          resIRtype =  res->getJitType( this );

          // NOTE: We always Zero Extend because the src1 operand is UNSIGNED

          DPT0( "VV97010: ", VV_VD,
         "src1JitVal = Bldr->CreateZExt( src1JitVal, (IntegerType *)resIRtype, 'ZExt' )\n" );
          src1JitVal = Bldr->CreateZExt( src1JitVal,
                                        (IntegerType *)resIRtype, "ZExt" ) ;

//        res->storeJitValue(this, f, res->getJitType(), src1JitVal, PCBlk);
          res->storeJitValue(this, Bldr, resIRtype, src1JitVal, PCBlk);

          // Lastly, clear out jit values for 32-bit source operand if needed.
          if ( (opc == PCIT::MOVE_MBIN64S_MBIN16U) ||
               (opc == PCIT::MOVE_MBIN64S_MBIN32U) )
            src1->clearJitValues(this);

          break;
        }

        case PCIT::MOVE_MBIN32U_MBIN16S:
        case PCIT::MOVE_MBIN32S_MBIN16S:
        case PCIT::MOVE_MBIN64S_MBIN16S:
        case PCIT::MOVE_MBIN64S_MBIN32S:
        {
          PCodeOperand *src1,      *res       ;
          jit_value_t   src1JitVal, resJitVal ;
          jit_type_t    src1IRtype, resIRtype ;

          src1 = PCInst->getROps()[0];
          res  = PCInst->getWOps()[0];
          PRINT_DETAILS_OF_RDOp( 0, src1 );
          PRINT_DETAILS_OF_WROp( 0, res );

          src1IRtype = src1->getJitType( this );

          DPT1( "VV98010: ", VV_XD, "typeID = %d\n",
                                           (int) src1IRtype->getTypeID() );

#if NExprDbgLvl >= VV_XD
          if ( src1IRtype->getTypeID() == Type::IntegerTyID )
             DPT1( "VV98011: ", VV_XD, "Integer Type - Number of bits = %d\n",
                                        src1IRtype->getIntegerBitWidth() );
#endif

//        src1JitVal = src1->getJitValue(this, f, src1->getJitType(), PCBlk);
          src1JitVal = src1->getJitValue(this, Bldr, src1IRtype, PCBlk);

//        src1JitVal = jit_insn_convert(f, src1JitVal, res->getJitType(), 0);

          resIRtype =  res->getJitType( this );

          DPT1( "VV98012: ", VV_XD, "typeID = %d\n",
                                              (int) resIRtype->getTypeID() );

#if NExprDbgLvl >= VV_XD
          if ( resIRtype->getTypeID() == Type::IntegerTyID )
            DPT1( "VV98013: ", VV_XD,"res Integer Type - Number of bits = %d\n",
                                              resIRtype->getIntegerBitWidth() );
#endif

          llvm::IntegerType * IntTyp = IntegerType::get( LLContxt,
                                       resIRtype->getIntegerBitWidth() );

          DPT1( "VV98014: ", VV_XD, "IntTyp - Number of bits = %d\n",
                                             IntTyp->getIntegerBitWidth() );

          // NOTE: We always Sign Extend because the src1 operand is SIGNED

          DPT0( "VV98016: ", VV_VD,
         "resJitVal = Bldr->CreateSExtOrTrunc( src1JitVal, IntTyp, 'SExt' )\n" );
          resJitVal = Bldr->CreateSExtOrTrunc( src1JitVal, IntTyp, "SExt" ) ;

//        res->storeJitValue(this, f, res->getJitType(), resJitVal, PCBlk);
          res->storeJitValue(this, Bldr, resIRtype, resJitVal, PCBlk);

          // Lastly, clear out jit values for 32-bit source operand if needed.
          if ( (opc == PCIT::MOVE_MBIN64S_MBIN16S) ||
               (opc == PCIT::MOVE_MBIN64S_MBIN32S) )
            src1->clearJitValues(this);

          break;
        }

        case PCIT::MOVE_MBIN16U_IBIN16U:
        case PCIT::MOVE_MBIN32S_IBIN32S:
        {
          PCodeOperand *res        ;
          jit_value_t   src1JitVal ;
          jit_type_t    resIRtype  ;

          res  = PCInst->getWOps()[0];
          PRINT_DETAILS_OF_WROp( 0, res );

//        type = (opc == PCIT::MOVE_MBIN32S_IBIN32S) ? jit_type_int : jit_type_ushort;
          resIRtype = (opc == PCIT::MOVE_MBIN32S_IBIN32S) ? int32Ty_ : int16Ty_;

          Int32 val = PCInst->code[3];

//        src1JitVal = jit_value_create_nint_constant(f, type, val);
//        res->storeJitValue(this, f, type, src1JitVal, PCBlk);

          src1JitVal = ConstantInt::get( resIRtype, val );

          DPT1( "VV99010: ", VV_VD,
         "res->storeJitValue(this, Bldr, resIRtype, src1JitVal=Const of %d, PCBlk)\n", val);
          res->storeJitValue(this, Bldr, resIRtype, src1JitVal, PCBlk);

          break;  
        }

        case PCIT::MOVE_MBIN16U_MBIN16U:
        case PCIT::MOVE_MBIN8_MBIN8:
        case PCIT::MOVE_MBIN8_MBIN8_IBIN32S:
        case PCIT::MOVE_MBIN32U_MBIN32U:
        case PCIT::MOVE_MBIN64S_MBIN64S:
        {
          PCodeOperand *src1,      *res       ;
          jit_value_t   src1JitVal, resJitVal ;
          jit_type_t    src1IRtype            ;

          src1 = PCInst->getROps()[0];
          res  = PCInst->getWOps()[0];
          PRINT_DETAILS_OF_RDOp( 0, src1 );
          PRINT_DETAILS_OF_WROp( 0, res );

          src1IRtype =  src1->getJitType( this );

          DPT1( "VV9A011: ", VV_XD, "For src1, src1IRtype->getTypeID() == %d\n",
                                               src1IRtype->getTypeID() );

#if NExprDbgLvl >= VV_XD
          if ( src1IRtype->getTypeID()==Type::IntegerTyID )
             DPT1( "VV9A015: ", VV_XD, "BitWidth=%d\n",
                                            src1IRtype->getIntegerBitWidth());
#endif
          if ( src1IRtype == int8PtrTy_ )
              assert (opc == PCIT::MOVE_MBIN8_MBIN8_IBIN32S);

#if NExprDbgLvl >= VV_BD
          if (opc == PCIT::MOVE_MBIN8_MBIN8_IBIN32S)
          {
            DPT1( "VV9A020: ", VV_XD, "src1->stackIndex_ = %d\n",
                                       src1->getStackIndex() );
            DPT1( "VV9A021: ", VV_XD, "res->stackIndex_  = %d\n",
                                       res->getStackIndex() );
            if ( src1IRtype->isPointerTy() )
              DPT1( "VV9A022: ", VV_XD,
                    "src1IRtype->getContainedType(0)->getTypeID() = %d\n",
                           src1IRtype->getContainedType(0)->getTypeID() );
          }
#endif

          DPT0( "VV9A026: ", VV_VD,
         "src1JitVal = src1->getJitValue( this, Bldr, src1IRtype, PCBlk )\n");

          src1JitVal = src1->getJitValue( this, Bldr, src1IRtype, PCBlk );

          DPT1( "VV9A027: ", VV_XD,"src1JitVal->getType()->getTypeID() == %d\n",
                                           src1JitVal->getType()->getTypeID() );
#if NExprDbgLvl >= VV_BD
          if ( src1JitVal->getType()->isPointerTy() )
            DPT1( "VV9A027A: ", VV_XD, "src1JitVal IS PointerType -> %d\n",
                      src1JitVal->getType()->getContainedType(0)->getTypeID() );

          if ( src1JitVal->getType()->getTypeID() == Type::IntegerTyID )
            DPT1( "VV9A027B: ", VV_XD, "src1JitVal IS Int Type. BitWidth = %d\n",
                                  src1JitVal->getType()->getIntegerBitWidth() );
#endif

          DPT2( "VV9A027D: ", VV_XD,
                "src1IRtype is at %p, src1JitVal->getType() is at %p\n",
                          src1IRtype, src1JitVal->getType() );

          DPT1( "VV9A028: ", VV_XD, "src1JitVal is at %p\n", src1JitVal );

          DPT0( "VV9A030: ", VV_VD,
         "res->storeJitValue(this, Bldr, src1IRtype, src1JitVal, PCBlk)\n" );

          res->storeJitValue(this, Bldr, src1IRtype, src1JitVal, PCBlk);

          break;
        }

        case PCIT::NULL_TEST_MBIN32S_MATTR5_IBIN32S_IBIN32S:
        {
          PCodeOperand *src1,      *res       ;
          jit_value_t   src1JitVal, resJitVal ;
          jit_type_t    src1IRtype            ;

          src1 = PCInst->getROps()[0];
          res  = PCInst->getWOps()[0];
          PRINT_DETAILS_OF_RDOp( 0, src1 );
          PRINT_DETAILS_OF_WROp( 0, res );

////      src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src1IRtype = src1->getJitType( this );
          src1JitVal = src1->getJitValue(this, Bldr, src1IRtype, PCBlk);

          NABoolean testForNull = (PCInst->code[9] != 0);

          // Generate more efficient code if next instruction is logical branch
          if (PCInst->next && PCInst->next->isAnyLogicalBranch())
          {
            NABoolean srcLive, tgtLive;

            PCodeInst* branch = PCInst->next;

            PCodeOperand* bSrc = branch->getROps()[0];
            PCodeOperand* bTgt = branch->getWOps()[0];

            // Is source or target live after branch?
            srcLive = bSrc->isVar() || PCBlk->isOperandLiveInSuccs(bSrc);
            tgtLive = bTgt->isVar() || PCBlk->isOperandLiveInSuccs(bTgt);

            // Only generate efficient path if the null-test is associated with
            // the branch, and that both the src and tgt of the branch are not
            // live (which would otherwise require one or more stores).
            if ((bSrc->getBvIndex()==res->getBvIndex()) && !srcLive && !tgtLive)
            {
////          jitLabel = PCBlk->getTargetBlock()->getJitLabel();
              p_IR_block_t * IR_Blk_Label =
                                       PCBlk->getTargetBlock()->getJitBlkLbl();

              // Branch to destination according to the test performed.
              switch (branch->getOpcode()) {
                case PCIT::BRANCH_OR:
                case PCIT::BRANCH_OR_CNT:
                  if (testForNull)
////                jit_insn_branch_if(f, src1JitVal, jitLabel);
                    IR_insn_branch_if_not_zero( Bldr, src1JitVal, IR_Blk_Label );
                  else
////                jit_insn_branch_if_not(f, src1JitVal, jitLabel);
                    IR_insn_branch_if_zero(Bldr, src1JitVal, IR_Blk_Label);
                  break;

                case PCIT::BRANCH_AND:
                case PCIT::BRANCH_AND_CNT:
                  if (testForNull)
////                jit_insn_branch_if_not(f, src1JitVal, jitLabel);
                    IR_insn_branch_if_zero(Bldr, src1JitVal, IR_Blk_Label);
                  else
////                jit_insn_branch_if(f, src1JitVal, jitLabel);
                    IR_insn_branch_if_not_zero( Bldr, src1JitVal, IR_Blk_Label );
                  break;
              }

              // Skip the next instruction (branch) since it was handled here.
              skipInst = 1;
              break;
            }
          }
          else if (PCInst->next && PCInst->next->getOpcode() == PCIT::MOVE_MBIN32S)
          {
            PCodeInst* move = PCInst->next;
            if (res->getBvIndex() == move->getROps()[0]->getBvIndex())
            {
////          jit_label_t l1 = jit_label_undefined;

              // A RETURN is guaranteed to following a MOVE_MBIN32S
              assert (move->next && (move->next->getOpcode() == PCIT::RETURN));

              enum cmpKind       KindOfCmp = IntCompare_EQ ; 
              if ( testForNull ) KindOfCmp = IntCompare_NE ; 

              jit_value_t  Zero = ConstantInt::get( src1JitVal->getType(), 0 );
              resJitVal = jitGenCompare( Bldr, KindOfCmp,
                                         src1JitVal,   Zero ,
                                         IR_Const_TRUE_, IR_Const_FALSE_ );

              DPT0( "VV9A360: ", VV_VD, "returned from jitGenCompare()\n");

              DPT0( "VV9A361: ", VV_VD,
             "Bldr->CreateRet( resJitVal )\n" );
              Bldr->CreateRet( resJitVal );

              justGeneratedRET = TRUE;

////          if (testForNull)
////            jit_insn_branch_if(f, src1JitVal, &l1);
////          else
////            jit_insn_branch_if_not(f, src1JitVal, &l1);

////          jit_insn_return(f, fJitVal_);

////          jit_insn_label(f, &l1);
////          jit_insn_return(f, tJitVal_);

              // Skip the next 2 instructions since it was handled here.
              skipInst = 2;
              break;
            }
          }
          
          // Fall-through (default) case to implement NULL_TEST

////      if (testForNull)
////        resJitVal = jit_insn_ne(f, src1JitVal, zeroJitVal_);
////      else
////        resJitVal = jit_insn_eq(f, src1JitVal, zeroJitVal_);

            enum cmpKind       KindOfCmp = IntCompare_EQ ; 
            if ( testForNull ) KindOfCmp = IntCompare_NE ; 

            jit_value_t  Zero = ConstantInt::get( src1JitVal->getType(), 0 );
            resJitVal = jitGenCompare( Bldr , KindOfCmp ,
                                       src1JitVal  , Zero ,
                                       IR_Const_1_ , IR_Const_0_  );

          DPT0( "VV9A380: ", VV_VD, "returned from jitGenCompare()\n");

////      inst->getWOps()[0]->setJitValue(this, f, resJitVal, block);
          res->setJitValue( this, Bldr, resJitVal, PCBlk);

          break;
        }

        case PCIT::MINMAX_MBIN8_MBIN8_MBIN32S_IBIN32S:
        {
          PCodeOperand *src1,      *src2,      *res ;
          jit_value_t   src1JitVal, src2JitVal      ;
          jit_type_t    src1IRtype                  ;

////      jit_type_t type;
////      jit_label_t doneLabel = jit_label_undefined;

          p_IR_block_t doneLabel = BasicBlock::Create( LLContxt, "MinMaxDoneBlk" );

          DPT1( "VV9A501: ", VV_XD, "doneLabel = %p\n", doneLabel );

          src1 = PCInst->getROps()[0]; // Source pointer
          src2 = PCInst->getROps()[1]; // Result of comparison
          res  = PCInst->getWOps()[0];
          PRINT_DETAILS_OF_RDOp( 0, src1 );
          PRINT_DETAILS_OF_RDOp( 1, src2 );
          PRINT_DETAILS_OF_WROp( 0, res );

////      src2JitVal = src2->getJitValue(this, f, jit_type_int, block);
          src2JitVal = src2->getJitValue(this, Bldr, int32Ty_, PCBlk);

          // Get type associated with length, since generic MBINS is used to
          // represent source/tgt.  If not, getJitType() is called, and it will
          // assume we're dealing with a void ptr by default.

          switch(src1->getLen()) {
            case 1:
////          type = jit_type_ubyte;
              src1IRtype = int8Ty_;
              break;
            case 2:
////          type = jit_type_ushort;
              src1IRtype = int16Ty_;
              break;
            case 4:
////          type = jit_type_uint;
              src1IRtype = int32Ty_;
              break;
            case 8:
////          type = jit_type_long;
              src1IRtype = int64Ty_;
              break;
            default:
            {
////          type = jit_type_void_ptr;
              src1IRtype = int1PtrTy_;
              break;
            }
          }

////      src1JitVal = src1->getJitValue(this, f, type, block);
          src1JitVal = src1->getJitValue(this, Bldr, src1IRtype, PCBlk);

////      jit_insn_branch_if_not(f, src2JitVal, &doneLabel);
          IR_insn_branch_if_zero( Bldr, src2JitVal, &doneLabel );


////      res->setJitValue(this, f, src1JitVal, block);
          res->setJitValue( this, Bldr, src1JitVal, PCBlk );

////      jit_insn_label(f, &doneLabel);

          DPT1( "VV9A520: ", VV_VD,
         "Bldr->CreateBr( doneLabel = %p )\n", doneLabel );
          Bldr->CreateBr( doneLabel );  // Must complete the current block

          DPT1( "VV9A522: ", VV_VD,
         "ExpFn->getBasicBlockList().push_back( doneLabel = %p )\n", doneLabel );
          ExpFn->getBasicBlockList().push_back( doneLabel );

          DPT1( "VV9A524: ", VV_VD,
         "Bldr->SetInsertPoint( doneLabel = %p )\n", doneLabel );
          Bldr->SetInsertPoint( doneLabel );

          break;
        }

        case PCIT::OPDATA_MPTR32_IBIN32S:
        case PCIT::OPDATA_MPTR32_IBIN32S_IBIN32S:
        case PCIT::OPDATA_MBIN16U_IBIN32S:
        case PCIT::OPDATA_MATTR5_IBIN32S:
        {
          OPLIST opList = (PCInst->getWOps().entries() ?
                           PCInst->getWOps() : PCInst->getROps() );

          DPT1( "VV9B001: ", VV_XD, "In OPDATA_*, using %s operands!\n",
                               PCInst->getWOps().entries() ? "WRITE": "READ" );

#if NExprDbgLvl >= VV_BD
          for (Int32 iii = 0; iii < opList.entries(); iii++ )
          {
            DPT2( "VV9B002: ", VV_XD,
                  "In OPDATA_*, opList[%d] = %p\n", iii, opList[iii] );

            DPT1( "VV9B002a: ", VV_XD,
                  "Details of opList[%d] *BEFORE setupClauseOperand*  are:\n",
                                     iii );
            PCodeOperand *PCOp = opList[iii];
            if (!PCOp) continue;

            DPT6( "VV9B002b: ", VV_XD,
           "stackIndex_ = %d, offset_ = %d, len_=%d, operandType_=%d, nullBitIdx=%d, CJV=%p\n",
            PCOp->getStackIndex(), PCOp->getOffset(), PCOp->getLen(),
            PCOp->getType(), PCOp->getNullBitIndex(), PCOp->getCurrJitVal() );
 }
#endif

          Int32 index =
            (opc == PCIT::OPDATA_MPTR32_IBIN32S_IBIN32S) ?
                    PCInst->code[4] : ( (opc == PCIT::OPDATA_MATTR5_IBIN32S) ?
                    PCInst->code[6] : PCInst->code[3] );

          DPT1( "VV9B003: ", VV_XD, "OPDATA_* - index = %d\n", index );

#if NExprDbgLvl >= VV_BD
          for ( Int32 jjj = 0; (jjj <= index) && (jjj < opList.entries()); jjj++)
          {
            DPT2( "VV9B004: ", VV_XD, "OPDATA_* BEFORE opData[%d] = %p\n", jjj, opData[jjj]);
            if ( opData[jjj] )
            {
              DPT1( "VV9B005: ", VV_XD,
                    "Details of opData[%d] *BEFORE setupClauseOperand* are:\n",
                                       jjj );

              PCodeOperand *PCOp = opData[jjj];

              DPT6( "VV9B006: ", VV_XD,
             "stackIndex_ = %d, offset_ = %d, len_=%d, operandType_=%d, nullBitIdx=%d, CJV=%p\n",
              PCOp->getStackIndex(), PCOp->getOffset(), PCOp->getLen(),
              PCOp->getType(), PCOp->getNullBitIndex(), PCOp->getCurrJitVal() );
            }
          }
#endif
          // Add the "correct" operand for this opdata instruction, replacing
          // the old one in the operand list used during pcode opts.
          setupClauseOperand(this, opList, opData, index, PCInst->clause_);

          //
          // We must ensure Write Operands have space allocated for them
          // *before* we generated any conditional branches.
          // NOTE: It is possible that a case will be found in the future when
          //       we need to have allocJitValue() called *before* the main loop.
          if ( PCInst->getWOps().entries() )
          {
             for ( Int32 nnn = 0; (nnn <= index) && (nnn < opList.entries()); nnn++)
             {
               PCodeOperand * Oper = opData[nnn] ;
               if ( ! Oper ) continue;

               DPT0( "VV9B006a: ", VV_XD,
              "Oper->allocJitValue( this, Bldr, Oper->getType(), Oper->getLen(), 1 )\n" );
               Oper->allocJitValue( this, Bldr, Oper->getType(), Oper->getLen(), 1 ) ;
             }
          }

#if NExprDbgLvl >= VV_BD
          for (Int32 iii = 0; iii < opList.entries(); iii++ )
          {
            DPT2( "VV9B006b: ", VV_XD, "In OPDATA_*, opList[%d] = %p\n",
                                                            iii, opList[iii] );
            DPT1( "VV9B006c: ", VV_XD,
                  "Details of opList[%d] *BEFORE setupClauseOperand*  are:\n",
                                     iii );
            PCodeOperand *PCOp = opList[iii];
            if (!PCOp) continue;

            DPT6( "VV9B006d: ", VV_XD,
           "stackIndex_ = %d, offset_ = %d, len_=%d, operandType_=%d, nullBitIdx=%d, CJV=%p\n",
            PCOp->getStackIndex(), PCOp->getOffset(), PCOp->getLen(),
            PCOp->getType(), PCOp->getNullBitIndex(), PCOp->getCurrJitVal() );
          }

          for ( Int32 mmm = 0; (mmm <= index) && (mmm < opList.entries()); mmm++)
          {
            DPT2( "VV9B007: ", VV_XD, "OPDATA_* AFTER opData[%d] = %p\n",
                                                            mmm, opData[mmm] );
            if ( opData[mmm] )
            {
              DPT1( "VV9B008: ", VV_XD,
                    "Details of opData[%d] *AFTER setupClauseOperand() are:\n",
                                       mmm );

              PCodeOperand *PCOp = opData[mmm];
              if (!PCOp) continue;

              DPT6( "VV9B009: ", VV_XD,
             "stackIndex_ = %d, offset_ = %d, len_=%d, operandType_=%d, nullBitIdx=%d, CJV=%p\n",
              PCOp->getStackIndex(), PCOp->getOffset(), PCOp->getLen(),
              PCOp->getType(), PCOp->getNullBitIndex(), PCOp->getCurrJitVal() );
            }
          }
#endif /* NExprDbgLvl >= VV_BD */

          break;
        }

        case PCIT::CLAUSE_EVAL:
        {
          PCodeOperand *src1,      *src2,       *res       ;
          jit_value_t   src1JitVal, src2JitVal,  resJitVal ;
          jit_type_t    src1IRtype, resIRtype              ;

////      jit_label_t doneLabel = jit_label_undefined;
////      jit_label_t trueLabel = jit_label_undefined;
////      jit_label_t falseLabel = jit_label_undefined;
////      jit_label_t nullLabel = jit_label_undefined;

          p_IR_block_t doneLabel = BasicBlock::Create( LLContxt, "ClauseEvalDoneBlk" );

          DPT1( "VV9B101: ", VV_XD, "doneLabel = %p\n", doneLabel );

          // on 64-bit the clause pointer is in code[1] and code[2]
          ex_clause* clause = (ex_clause*)*(Long*)&(PCInst->code[1]);
          NABoolean processNulls = PCInst->code[1 + PCODEBINARIES_PER_PTR];

#if NExprDbgLvl >= VV_BD
          for (Int32 iii = 0; iii < 3; iii++ )
          {
            DPT2( "VV9B103: ", VV_XD, "In CLAUSE_EVAL opDataNulls[%d] = %p\n",
                                                      iii, opDataNulls[iii] );
            if ( opDataNulls[iii] )
            {
              DPT1( "VV9B104: ", VV_XD,
             "Details of opDataNulls[%d] *AT Start OF CLAUSE_EVAL*  are:\n", iii );

              PCodeOperand *PCOp = opDataNulls[iii];

              DPT6( "VV9B105: ", VV_XD,
             "stackIndex_ = %d, offset_ = %d, len_=%d, operandType_=%d, nullBitIdx=%d, CJV=%p\n",
              PCOp->getStackIndex(), PCOp->getOffset(), PCOp->getLen(),
              PCOp->getType(), PCOp->getNullBitIndex(), PCOp->getCurrJitVal() );
            }
          }

          for (Int32 jjj = 0; jjj < 3; jjj++ )
          {
            DPT2( "VV9B110: ", VV_XD, "In CLAUSE_EVAL opDataVals[%d] = %p\n",
                                                      jjj, opDataVals[jjj] );
            if ( opDataVals[jjj] )
            {
              DPT1( "VV9B114: ", VV_XD,
           "Details of opDataVals[%d] *AT Start OF CLAUSE_EVAL*  are:\n", jjj );

              PCodeOperand *PCOp = opDataVals[jjj];

              DPT6( "VV9B115: ", VV_XD,
             "stackIndex_ = %d, offset_ = %d, len_=%d, operandType_=%d, nullBitIdx=%d, CJV=%p\n",
              PCOp->getStackIndex(), PCOp->getOffset(), PCOp->getLen(),
              PCOp->getType(), PCOp->getNullBitIndex(), PCOp->getCurrJitVal() );
           }
         }
#endif /* NExprDbgLvl >= VV_BD */

          switch (clause->getClassID()) {
            case ex_clause::AGGR_MIN_MAX_ID:
            {
              DPT0( "VV9B120: ", VV_VD,
                    "CLAUSE_EVAL - found ClassID = AGGR_MIN_MAX_ID \n");
              //
              // Format:
              //
              // attr[0] - Aggregate
              // attr[1] - Column
              // attr[2] - Comparison result
              //

              // First branch away if comp result indicates no update needed. 
              // Note, since this check is always made, the jit value from the
              // null-check can be used outside of the CLAUSE_EVAL.
              src2 = opDataVals[2];
////          src2JitVal = src2->getJitValue(this, f, jit_type_int, block);
              src2JitVal = src2->getJitValue(this, Bldr, int32Ty_ , PCBlk);

////          jit_insn_branch_if_not(f, src2JitVal, &doneLabel);
              IR_insn_branch_if_zero( Bldr, src2JitVal, &doneLabel );

              // We need to copy over source into aggregate.  First take care of
              // the null indicator if the aggregate is nullable.
              if (clause->getOperand(0)->getNullFlag()) {
                // If column is nullable too, then we need to check it.  If it's
                // NULL, we don't do anything.

                if (clause->getOperand(1)->getNullFlag()) {
                  src1 = opDataNulls[1];

////              src1JitVal =
////                src1->getJitValue(this, f, src1->getJitType(), block,
////                                  NULL, TRUE /* no assign jit val */);

                  src1IRtype = src1->getJitType( this );
                  src1JitVal = src1->getJitValue(this, Bldr, src1IRtype, PCBlk,
                                                 NULL, TRUE /* no assign */);

////              jit_insn_branch_if(f, src1JitVal, &doneLabel);
                  IR_insn_branch_if_not_zero( Bldr, src1JitVal, &doneLabel );
                }

                // Null out the null indicator for the aggregate.  Based on the
                // current code, the null indicator for the aggregate is always
                // in exploded format, so assert that here and then simplify
                // code generation.
                assert(!opDataNulls[0]->forAlignedFormat());

////            opDataNulls[0]->setJitValue(this, f, zeroJitVal16, block, TRUE);
                opDataNulls[0]->setJitValue( this, Bldr, IR_Const_16bit_0_,
                                             PCBlk, TRUE);
              }

              // Now we have to handle moving the data over from column to aggr.
              res = opDataVals[0];
              src1 = opDataVals[1];

////          src1JitVal = src1->getJitValue(this, f, src1->getJitType(),
////                                         block, NULL, TRUE /* no assign */);
              src1IRtype = src1->getJitType( this );
              src1JitVal = src1->getJitValue( this, Bldr, src1IRtype, PCBlk,
                                              NULL, TRUE /* no assign */);

              if (res->isVarchar())
              {
                // The following code was taken from MOVE_MATTR5_MATTR5, but
                // re-formatted here.
                jit_type_t vcType;
                jit_value_t vcLenJitVal;

                if (src1->getVcIndicatorLen() == 2) {
////              vcType = jit_type_ushort;
                  vcType = int16Ty_ ;

////              vcLenJitVal=jit_value_create_nint_constant(f, jit_type_int,2);
                  vcLenJitVal = ConstantInt::get( int32Ty_, 2 );
                }
                else {
////              vcType = jit_type_uint;
                  vcType = int32Ty_ ;

////              vcLenJitVal=jit_value_create_nint_constant(f, jit_type_int,4);
                  vcLenJitVal = ConstantInt::get( int32Ty_, 4 );
                }

////            resJitVal = res->getJitValue(this, f, res->getJitType(), block,
////                                         NULL, TRUE /* no assign */);

                resIRtype = res->getJitType( this );
                resJitVal = res->getJitValue(this, Bldr, resIRtype, PCBlk,
                                             NULL, TRUE /* no assign */);

////            jit_insn_store(f, tRes, resJitVal);
////            jit_insn_store(f, tSrc1, src1JitVal);

////            jit_value_t srcLen =
////              jit_insn_load_relative(f, src1JitVal, 0, vcType);

                jit_value_t srcLen = IR_LoadRelativeWithType( Bldr, 
                                                        src1JitVal, 0, vcType );

////            srcLen = jit_insn_add(f, srcLen, vcLenJitVal);

                DPT0( "VV9B130: ", VV_VD,
               "srcLen = Bldr->CreateAdd( srcLen , vcLenJitVal , 'IntAdd' )\n");
                srcLen = Bldr->CreateAdd( srcLen , vcLenJitVal , "IntAdd" );

////            genUnalignedMemcpy(f, tRes, tSrc1, srcLen);
                genUnalignedMemcpy( Bldr, resJitVal, src1JitVal, srcLen );
              }
              else
              {
////            res->storeJitValue(this, f, res->getJitType(),src1JitVal,
////                               block, NULL, TRUE);
                jit_type_t resIRtype = res->getJitType( this ) ;
                res->storeJitValue(this, Bldr, resIRtype,
                                   src1JitVal, PCBlk, NULL, TRUE );
              }

              // Reached the end point
////          jit_insn_label(f, &doneLabel);

              DPT1( "VV9B134: ", VV_VD,
             "Bldr->CreateBr( doneLabel = %p )\n", doneLabel );
              Bldr->CreateBr( doneLabel );  // Must complete the current block

              DPT1( "VV9B136: ", VV_VD,
             "ExpFn->getBasicBlockList().push_back( doneLabel = %p )\n", doneLabel );
              ExpFn->getBasicBlockList().push_back( doneLabel );

              DPT1( "VV9B138: ", VV_VD,
             "Bldr->SetInsertPoint( doneLabel = %p )\n", doneLabel );
              Bldr->SetInsertPoint( doneLabel );

              break;
            }

            case ex_clause::COMP_TYPE:
            {
              DPT0( "VV9B200: ", VV_VD,
                    "CLAUSE_EVAL - found ClassID = COMP_TYPE \n");

              p_IR_block_t trueLabel  = BasicBlock::Create( LLContxt, "ClauseEvalTrueBlk"  );
              p_IR_block_t falseLabel = BasicBlock::Create( LLContxt, "ClauseEvalFalseBlk" );
              p_IR_block_t nullLabel  = BasicBlock::Create( LLContxt, "ClauseEvalNullBlk" );

              DPT3( "VV9B201: ", VV_VD,
                    "trueLabel = %p, falseLabel = %p, nullLabel = %p\n",
                     trueLabel,      falseLabel,      nullLabel );

              switch (((ex_comp_clause*)clause)->getInstruction()) {
                case NE_DATETIME_DATETIME:
                case EQ_DATETIME_DATETIME:
                case LT_DATETIME_DATETIME:
                case GT_DATETIME_DATETIME:
                case LE_DATETIME_DATETIME:
                case GE_DATETIME_DATETIME:
                {
                  DPT0( "VV9B210: ", VV_VD,
                      "CLAUSE_EVAL - COMP_TYPE - found DATETIME case index\n" );
                  jit_value_t year1, year2;

                  res = opDataVals[0];
                  src1 = opDataVals[1];
                  src2 = opDataVals[2];

                  DPT3( "VV9B211: ", VV_XD,
                        "COMP_TYPE - res = %p, src1 = %p, src2 = %p \n",
                                     res,      src1,      src2 );

////              src1JitVal = src1->getJitValue(this, f, jit_type_void_ptr,
////                                             block, NULL);
                  src1JitVal = src1->getJitValue(this, Bldr, int1PtrTy_,
                                                 PCBlk, NULL );

////              src2JitVal = src2->getJitValue(this, f, jit_type_void_ptr,
////                                             block, NULL);
                  src2JitVal = src2->getJitValue(this, Bldr, int1PtrTy_,
                                                 PCBlk, NULL );
                  DPT2( "VV9B212: ", VV_XD,
                        "COMP_TYPE - src1JitVal at %p, src2JitVal at %p \n",
                                     src1JitVal,       src2JitVal );
                  if (processNulls)
                  {
                    PCodeOperand* nullOp;
                    jit_value_t nullJitVal;

                    if (clause->getOperand(1)->getNullFlag())
                    {
                      DPT1( "VV9B213: ", VV_XD,
                            "COMP_TYPE - opDataNulls[1] == %p\n",
                                         opDataNulls[1] );

                      nullOp = opDataNulls[1];
////                  nullJitVal =
////                  nullOp->getJitValue(this, f, nullOp->getJitType(), block,
////                                      NULL, TRUE /* no assign jit val */);

                      nullJitVal = nullOp->getJitValue(this, Bldr,
                                             nullOp->getJitType( this ),
                                             PCBlk, NULL, TRUE /* no assign */);

                      DPT1( "VV9B214: ", VV_XD,
                            "COMP_TYPE - nullJitVal at %p\n", nullJitVal );

////                  jit_insn_branch_if(f, nullJitVal, &nullLabel);

                      DPT1( "VV9B215: ", VV_XD,
                     "IR_insn_branch_if_not_zero( Bldr, nullJitVal, &nullLabel = %p )\n", nullLabel );
                      IR_insn_branch_if_not_zero( Bldr, nullJitVal, &nullLabel );
                    }

                    if (clause->getOperand(2)->getNullFlag()) {
                      nullOp = opDataNulls[2];

                      DPT1( "VV9B216: ", VV_XD,
                            "COMP_TYPE-opDataNulls[2] == %p\n", opDataNulls[2]);

////                  nullJitVal =
////                  nullOp->getJitValue(this, f, nullOp->getJitType(), block,
////                                      NULL, TRUE /* no assign jit val */);
                      nullJitVal = nullOp->getJitValue(this, Bldr,
                                             nullOp->getJitType( this ),
                                             PCBlk, NULL, TRUE /* no assign */);

                      DPT1( "VV9B217: ", VV_XD,
                            "COMP_TYPE-nullJitVal at %p\n",  nullJitVal );

////                  jit_insn_branch_if(f, nullJitVal, &nullLabel);

                      DPT1( "VV9B218: ", VV_XD,
                     "IR_insn_branch_if_not_zero( Bldr, nullJitVal, &nullLabel = %p )\n", nullLabel );
                      IR_insn_branch_if_not_zero( Bldr, nullJitVal, &nullLabel );
                    }
                  }

                  long dtCode =
                    ((ExpDatetime*)(clause->getOperand(1)))->getPrecision();

                  switch (dtCode)
                  {
                    case REC_DTCODE_TIME:
                    case REC_DTCODE_DATE:
                    case REC_DTCODE_TIMESTAMP:
                    {
                      // Boolean used to indicate whether branch should go to
                      // true block or not.  Assume "not" by default.
                      NABoolean dumpTrueBlockFirst = TRUE;  // May change later

                      Int32 size = (clause->getOperand(1))->getLength();

                      // Cycle through each field in the date-related struct.
                      Int32 typeSize = 0;
                      for (i=0; i < size; i += typeSize)
                      {
                        jit_type_t type;

                        // Are we getting the fractional seconds?
                        if ((i == size-4) && (dtCode == REC_DTCODE_TIMESTAMP)) {
////                      type = jit_type_uint;
                          type = int32Ty_ ;
                          typeSize = 4;
                        }
                        // Are we getting the year?
                        else if ((i==0) && (dtCode != REC_DTCODE_TIME)) {
////                      type = jit_type_ushort;
                          type = int16Ty_ ;
                          typeSize = 2;
                        }
                        // We're just getting some particular byte field
                        else {
////                      type = jit_type_ubyte;
                          type = int8Ty_ ;
                          typeSize = 1;
                        }

                        // Get the fields from both sources.
////                    year1 = jit_insn_load_relative(f, src1JitVal, i, type);
                        year1 = IR_LoadRelativeWithType( Bldr, 
                                                         src1JitVal, i, type );

////                    year2 = jit_insn_load_relative(f, src2JitVal, i, type);
                        year2 = IR_LoadRelativeWithType( Bldr, 
                                                         src2JitVal, i, type );

                        DPT2( "VV9B219: ", VV_XD,
                              "COMP_TYPE-year1 at %p, year2 at %p\n",
                                               year1, year2 );

                        // Depending on the comparison being performed, generate
                        // 1 (or 2) compares and branches.  Also, based on the
                        // comparison, the fall-through may be the "true" block,
                        // or it may be the "false" block - default is TRUE.

                        switch (((ex_comp_clause*)clause)->getInstruction()) {
                          case NE_DATETIME_DATETIME:
////                        resJitVal = jit_insn_ne(f, year1, year2);
                            resJitVal = jitGenCompare( Bldr, IntCompare_NE,
                                                       year1,  year2,
                                                       IR_Const_1_ ,
                                                       IR_Const_0_  );
////                        jit_insn_branch_if(f, resJitVal, &trueLabel);

                            DPT1( "VV9B220: ", VV_XD,
                           "IR_insn_branch_if_not_zero( Bldr, resJitVal, &trueLabel = %p)\n", trueLabel);
                            IR_insn_branch_if_not_zero( Bldr, resJitVal, &trueLabel );

                            dumpTrueBlockFirst = FALSE;
                            break;
                          case EQ_DATETIME_DATETIME:
////                        resJitVal = jit_insn_ne(f, year1, year2);
                            resJitVal = jitGenCompare( Bldr, IntCompare_EQ,
                                                       year1,  year2,
                                                       IR_Const_1_ ,
                                                       IR_Const_0_  );

////                        jit_insn_branch_if(f, resJitVal, &falseLabel);

                            DPT1( "VV9B222: ", VV_XD,
                           "IR_insn_branch_if_not_zero( Bldr, resJitVal, &falseLabel = %p )\n", falseLabel);
                            IR_insn_branch_if_not_zero( Bldr, resJitVal, &falseLabel );

                            break;
                          case LT_DATETIME_DATETIME:
////                        resJitVal = jit_insn_lt(f, year1, year2);
                            resJitVal = jitGenCompare( Bldr, IntCompare_ULT,
                                                       year1,  year2,
                                                       IR_Const_1_ ,
                                                       IR_Const_0_  );

////                        jit_insn_branch_if(f, resJitVal, &trueLabel);

                            DPT1( "VV9B224: ", VV_XD,
                           "IR_insn_branch_if_not_zero( Bldr, resJitVal, &trueLabel = %p )\n", trueLabel);
                            IR_insn_branch_if_not_zero( Bldr, resJitVal, &trueLabel );

////                        resJitVal = jit_insn_gt(f, year1, year2);
                            resJitVal = jitGenCompare( Bldr, IntCompare_UGT,
                                                       year1,  year2,
                                                       IR_Const_1_ ,
                                                       IR_Const_0_  );

////                        jit_insn_branch_if(f, resJitVal, &falseLabel);

                            DPT1( "VV9B226: ", VV_XD,
                           "IR_insn_branch_if_not_zero( Bldr, resJitVal, &falseLabel = %p )\n", falseLabel);
                            IR_insn_branch_if_not_zero( Bldr, resJitVal, &falseLabel );

                            dumpTrueBlockFirst = FALSE;
                            break;
                          case GT_DATETIME_DATETIME:
////                        resJitVal = jit_insn_gt(f, year1, year2);
                            resJitVal = jitGenCompare( Bldr, IntCompare_UGT,
                                                       year1,  year2,
                                                       IR_Const_1_ ,
                                                       IR_Const_0_  );

////                        jit_insn_branch_if(f, resJitVal, &trueLabel);

                            DPT1( "VV9B230: ", VV_XD,
                           "IR_insn_branch_if_not_zero( Bldr, resJitVal, &trueLabel = %p )\n", trueLabel);
                            IR_insn_branch_if_not_zero( Bldr, resJitVal, &trueLabel );

////                        resJitVal = jit_insn_lt(f, year1, year2);
                            resJitVal = jitGenCompare( Bldr, IntCompare_ULT,
                                                       year1,  year2,
                                                       IR_Const_1_ ,
                                                       IR_Const_0_  );

////                        jit_insn_branch_if(f, resJitVal, &falseLabel);

                            DPT1( "VV9B232: ", VV_XD,
                           "IR_insn_branch_if_not_zero( Bldr, resJitVal, &falseLabel = %p )\n", falseLabel);
                            IR_insn_branch_if_not_zero( Bldr, resJitVal, &falseLabel );

                            dumpTrueBlockFirst = FALSE;
                            break;
                          case LE_DATETIME_DATETIME:
////                        resJitVal = jit_insn_lt(f, year1, year2);
                            resJitVal = jitGenCompare( Bldr, IntCompare_ULT,
                                                       year1,  year2,
                                                       IR_Const_1_ ,
                                                       IR_Const_0_  );

////                        jit_insn_branch_if(f, resJitVal, &trueLabel);

                            DPT1( "VV9B234: ", VV_XD,
                           "IR_insn_branch_if_not_zero( Bldr, resJitVal, &trueLabel = %p )\n", trueLabel);
                            IR_insn_branch_if_not_zero( Bldr, resJitVal, &trueLabel );

////                        resJitVal = jit_insn_gt(f, year1, year2);
                            resJitVal = jitGenCompare( Bldr, IntCompare_UGT,
                                                       year1,  year2,
                                                       IR_Const_1_ ,
                                                       IR_Const_0_  );

////                        jit_insn_branch_if(f, resJitVal, &falseLabel);

                            DPT1( "VV9B236: ", VV_XD,
                           "IR_insn_branch_if_not_zero( Bldr, resJitVal, &falseLabel = %p )\n", falseLabel);
                            IR_insn_branch_if_not_zero( Bldr, resJitVal, &falseLabel );

                            break;
                          case GE_DATETIME_DATETIME:
////                        resJitVal = jit_insn_gt(f, year1, year2);
                            resJitVal = jitGenCompare( Bldr, IntCompare_UGT,
                                                       year1,  year2,
                                                       IR_Const_1_ ,
                                                       IR_Const_0_  );

////                        jit_insn_branch_if(f, resJitVal, &trueLabel);

                            DPT1( "VV9B238: ", VV_XD,
                           "IR_insn_branch_if_not_zero( Bldr, resJitVal, &trueLabel = %p )\n", trueLabel);
                            IR_insn_branch_if_not_zero( Bldr, resJitVal, &trueLabel );

////                        resJitVal = jit_insn_lt(f, year1, year2);
                            resJitVal = jitGenCompare( Bldr, IntCompare_ULT,
                                                       year1,  year2,
                                                       IR_Const_1_ ,
                                                       IR_Const_0_  );

////                        jit_insn_branch_if(f, resJitVal, &falseLabel);

                            DPT1( "VV9B240: ", VV_XD,
                           "IR_insn_branch_if_not_zero( Bldr, resJitVal, &falseLabel = %p )\n", falseLabel);
                            IR_insn_branch_if_not_zero( Bldr, resJitVal, &falseLabel );

                            break;
                        }
                      }

                      if (!dumpTrueBlockFirst)
                      {
                        DPT1( "VV9B250: ", VV_VD,
                       "Bldr->CreateBr( falseLabel = %p )\n", falseLabel );
                        Bldr->CreateBr( falseLabel );  // Must complete the current block

                        // False label first.
////                    jit_insn_label(f, &falseLabel);

                        DPT1( "VV9B251: ", VV_VD,
                       "ExpFn->getBasicBlockList().push_back( falseLabel = %p )\n", falseLabel );
                        ExpFn->getBasicBlockList().push_back( falseLabel );

                        DPT1( "VV9B252: ", VV_VD,
                       "Bldr->SetInsertPoint( falseLabel = %p )\n", falseLabel );
                        Bldr->SetInsertPoint( falseLabel );

////                    res->storeJitValue(this, f, jit_type_int, zeroJitVal_,
////                                       block);
                        DPT0( "VV9B253: ", VV_XD,
                       "res->storeJitValue(this, Bldr, int32Ty_, IR_Const_0_, PCBlk) \n" );
                        res->storeJitValue(this, Bldr, int32Ty_, IR_Const_0_,
                                           PCBlk );
////                    jit_insn_branch(f, &doneLabel);

                        DPT1( "VV9B254: ", VV_VD,
                       "Bldr->CreateBr( doneLabel = %p )\n", doneLabel );
                        Bldr->CreateBr( doneLabel );

                        // True label next.
////                    jit_insn_label(f, &trueLabel);

                        DPT1( "VV9B256: ", VV_VD,
                       "ExpFn->getBasicBlockList().push_back( trueLabel = %p )\n", trueLabel );
                        ExpFn->getBasicBlockList().push_back( trueLabel );

                        DPT1( "VV9B258: ", VV_VD,
                       "Bldr->SetInsertPoint( trueLabel = %p )\n", trueLabel );
                        Bldr->SetInsertPoint( trueLabel );

////                    res->storeJitValue(this, f, jit_type_int, oneJitVal_,
////                                       block);
                        DPT0( "VV9B259: ", VV_XD,
                       "res->storeJitValue(this, Bldr, int32Ty_, IR_Const_1_, PCBlk )\n" );
                        res->storeJitValue(this, Bldr, int32Ty_, IR_Const_1_,
                                           PCBlk );
////                    jit_insn_branch(f, &doneLabel);

                        DPT1( "VV9B260: ", VV_VD,
                       "Bldr->CreateBr( doneLabel = %p )\n", doneLabel );
                        Bldr->CreateBr( doneLabel );
                      }
                      else {
                        DPT1( "VV9B261: ", VV_VD,
                       "Bldr->CreateBr( trueLabel = %p )\n", trueLabel );
                        Bldr->CreateBr( trueLabel );  // Must complete the current block

                        // True label first.
////                    jit_insn_label(f, &trueLabel);

                        DPT1( "VV9B262: ", VV_VD,
                       "ExpFn->getBasicBlockList().push_back( trueLabel = %p )\n", trueLabel );
                        ExpFn->getBasicBlockList().push_back( trueLabel );

                        DPT1( "VV9B264: ", VV_VD,
                       "Bldr->SetInsertPoint( trueLabel = %p )\n", trueLabel );
                        Bldr->SetInsertPoint( trueLabel );

////                    res->storeJitValue(this, f, jit_type_int, oneJitVal_,
////                                       block);
                        DPT0( "VV9B265: ", VV_XD,
                       "res->storeJitValue(this, Bldr, int32Ty_, IR_Const_1_, ...)\n" );
                        res->storeJitValue(this, Bldr, int32Ty_, IR_Const_1_,
                                           PCBlk );
////                    jit_insn_branch(f, &doneLabel);

                        DPT1( "VV9B266: ", VV_VD,
                       "Bldr->CreateBr( doneLabel = %p )\n", doneLabel );
                        Bldr->CreateBr( doneLabel );

                        // False label next.
////                    jit_insn_label(f, &falseLabel);

                        DPT1( "VV9B268: ", VV_VD,
                       "ExpFn->getBasicBlockList().push_back( falseLabel = %p )\n", falseLabel );
                        ExpFn->getBasicBlockList().push_back( falseLabel );

                        DPT1( "VV9B270: ", VV_VD,
                       "Bldr->SetInsertPoint( falseLabel = %p )\n", falseLabel );
                        Bldr->SetInsertPoint( falseLabel );

////                    res->storeJitValue(this, f, jit_type_int, zeroJitVal_,
////                                       block);

                        DPT0( "VV9B271: ", VV_XD,
                       "res->storeJitValue(this, Bldr, int32Ty_, IR_Const_0_, PCBlk )\n" );
                        res->storeJitValue(this, Bldr, int32Ty_, IR_Const_0_,
                                           PCBlk );
////                    jit_insn_branch(f, &doneLabel);

                        DPT1( "VV9B272: ", VV_VD,
                       "Bldr->CreateBr( doneLabel = %p )\n", doneLabel );
                        Bldr->CreateBr( doneLabel );
                      }

                      // Null label last.
////                  jit_insn_label(f, &nullLabel);

                      DPT1( "VV9B274: ", VV_VD,
                     "ExpFn->getBasicBlockList().push_back( nullLabel = %p )\n", nullLabel );
                      ExpFn->getBasicBlockList().push_back( nullLabel );

                      DPT1( "VV9B276: ", VV_VD,
                     "Bldr->SetInsertPoint( nullLabel = %p )\n", nullLabel );
                      Bldr->SetInsertPoint( nullLabel );

////                  res->storeJitValue(this,f,jit_type_int,neg1JitVal_,block);

                      DPT0( "VV9B277: ", VV_XD,
                     "res->storeJitValue(this, Bldr, int32Ty_, IR_Const_neg1_, PCBlk )\n" );
                      res->storeJitValue(this, Bldr, int32Ty_, IR_Const_neg1_,
                                         PCBlk );

////                  jit_insn_label(f, &doneLabel);

                      DPT1( "VV9B278: ", VV_VD,
                     "Bldr->CreateBr( doneLabel = %p )\n", doneLabel );
                      Bldr->CreateBr( doneLabel );  // Must complete the current block

                      DPT1( "VV9B279: ", VV_VD,
                     "ExpFn->getBasicBlockList().push_back( doneLabel = %p )\n", doneLabel );
                      ExpFn->getBasicBlockList().push_back( doneLabel );

                      DPT1( "VV9B280: ", VV_VD,
                     "Bldr->SetInsertPoint( doneLabel = %p )\n", doneLabel );
                      Bldr->SetInsertPoint( doneLabel );

                      break;
                    }
                      
                    default:
                      assert(FALSE);
                  }
                  break;
                }
              }

              break;
            }
          }

          // Clear out opData for subsequent use.
          str_pad((char*)opData,
                  (3*ex_clause::MAX_OPERANDS)*sizeof(PCodeOperand*), 0);

          break;
        }

        case PCIT::SUM_MBIN32S_MBIN32S:
        case PCIT::SUM_MBIN64S_MBIN64S:
        {
          PCodeOperand *src1,      *src2,      *res       ;
          jit_value_t   src1JitVal, src2JitVal, resJitVal ;

          res  = PCInst->getWOps()[0];
          src1 = res; /* same as write operand */
          src2 = PCInst->getROps()[1];
          PRINT_DETAILS_OF_WROp( 0, res );
          PRINT_DETAILS_OF_RDOp( 0, src1 );
          PRINT_DETAILS_OF_RDOp( 1, src2 );

////      src1JitVal = src1->getJitValue(this, f, src1->getJitType(), PCBlk);
          src1JitVal = src1->getJitValue(this, Bldr, src1->getJitType(this), PCBlk);

////      src2JitVal = src2->getJitValue(this, f, src2->getJitType(), PCBlk);
          src2JitVal = src2->getJitValue(this, Bldr, src2->getJitType(this), PCBlk);

#if NExprDbgLvl >= VV_VD
           if ( src1->isConst() )
              DPT0( "VV9C005: ", VV_VD,
                    "case PCIT::SUM_MB...  src1 is a CONSTANT !\n" );

           if ( src2->isConst() )
              DPT0( "VV9C006: ", VV_VD,
                    "case PCIT::SUM_MB...  src2 is a CONSTANT !\n" );
#endif

////      resJitVal = jit_insn_add(f, src1JitVal, src2JitVal);

          DPT0( "VV9C010: ", VV_VD,
         "resJitVal = Bldr->CreateAdd( src1JitVal , src2JitVal , 'IntAdd' )\n" );
          resJitVal = Bldr->CreateAdd( src1JitVal , src2JitVal , "IntAdd" );

////      res->setJitValue(this, f, resJitVal, PCBlk);
          res->setJitValue(this, Bldr, resJitVal, PCBlk);

          // Lastly, clear out jit values for 32-bit source operand if needed.
          if (opc == PCIT::SUM_MBIN64S_MBIN32S)
            src2->clearJitValues(this);

          break;
        }

        case PCIT::ADD_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::ADD_MBIN16S_MBIN16S_MBIN16S:
        case PCIT::ADD_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::ADD_MBIN64S_MBIN64S_MBIN64S:
        case PCIT::ADD_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::ADD_MBIN64S_MBIN32S_MBIN64S:
        {
          PCodeOperand *src1,      *src2,      *res       ;
          jit_value_t   src1JitVal, src2JitVal, resJitVal ;
          jit_type_t    src1IRtype, src2IRtype, resIRtype ;

          res  = PCInst->getWOps()[0];
          src1 = PCInst->getROps()[0];
          src2 = PCInst->getROps()[1];
          PRINT_DETAILS_OF_WROp( 0, res );
          PRINT_DETAILS_OF_RDOp( 0, src1 );
          PRINT_DETAILS_OF_RDOp( 1, src2 );

////      src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src1IRtype = src1->getJitType( this );
          src1JitVal = src1->getJitValue(this, Bldr, src1IRtype, PCBlk);

////      src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);
          src2IRtype = src2->getJitType( this );
          src2JitVal = src2->getJitValue(this, Bldr, src2IRtype, PCBlk);

          resIRtype  = res->getJitType( this );

          Int32 src1Width = src1IRtype->getIntegerBitWidth();
          Int32 src2Width = src2IRtype->getIntegerBitWidth();
          Int32 resWidth  =  resIRtype->getIntegerBitWidth();

          if ( src1Width < resWidth )
          {
             DPT0( "VV9C100: ", VV_VD,
            "src1JitVal = Bldr->CreateSExt( src1JitVal, (IntegerType *)resIRtype, 'SExt' )\n" );
             src1JitVal = Bldr->CreateSExt( src1JitVal,
                                           (IntegerType *)resIRtype, "SExt" ) ;
          }
          if ( src2Width < resWidth )
          {
             DPT0( "VV9C105: ", VV_VD,
            "src2JitVal = Bldr->CreateSExt( src2JitVal, (IntegerType *)resIRtype, 'SExt' )\n" );
             src2JitVal = Bldr->CreateSExt( src2JitVal,
                                           (IntegerType *)resIRtype, "SExt" ) ;
          }

////      resJitVal = jit_insn_add(f, src1JitVal, src2JitVal);

          DPT0( "VV9C110: ", VV_VD,
         "resJitVal = Bldr->CreateAdd( src1JitVal , src2JitVal , 'IntAdd' )\n" );
          resJitVal = Bldr->CreateAdd( src1JitVal , src2JitVal , "IntAdd" );

////      res->setJitValue(this, f, resJitVal, block);
          res->setJitValue(this, Bldr, resJitVal, PCBlk);

////      if (opc == PCIT::ADD_MFLT64_MFLT64_MFLT64)
////        jitProcessFloatExceptionCheck(f, res, &errorJitLabel, block);

          // Lastly, clear out jit values for 32-bit source operand if needed.
          if (opc == PCIT::ADD_MBIN64S_MBIN32S_MBIN64S)
            src1->clearJitValues(this);

          break;
        }

        case PCIT::SUB_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::SUB_MBIN16S_MBIN16S_MBIN16S:
        case PCIT::SUB_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::SUB_MBIN64S_MBIN64S_MBIN64S:
        case PCIT::SUB_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::SUB_MBIN32S_MBIN32S_MBIN16S:
        {
          PCodeOperand *src1,      *src2,      *res       ;
          jit_value_t   src1JitVal, src2JitVal, resJitVal ;
          jit_type_t    src1IRtype, src2IRtype, resIRtype ;

          res  = PCInst->getWOps()[0];
          src1 = PCInst->getROps()[0];
          src2 = PCInst->getROps()[1];
          PRINT_DETAILS_OF_WROp( 0, res );
          PRINT_DETAILS_OF_RDOp( 0, src1 );
          PRINT_DETAILS_OF_RDOp( 1, src2 );

////      src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src1IRtype = src1->getJitType( this );
          src1JitVal = src1->getJitValue(this, Bldr, src1IRtype, PCBlk);

////      src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);
          src2IRtype = src2->getJitType( this );
          src2JitVal = src2->getJitValue(this, Bldr, src2IRtype, PCBlk);

          resIRtype  = res->getJitType( this );

          Int32 src1Width = src1IRtype->getIntegerBitWidth();
          Int32 src2Width = src2IRtype->getIntegerBitWidth();
          Int32 resWidth  =  resIRtype->getIntegerBitWidth();

          if ( src1Width < resWidth )
          {
             DPT0( "VV9C200: ", VV_VD,
            "src1JitVal = Bldr->CreateSExt( src1JitVal, (IntegerType *)resIRtype, 'SExt' )\n" );
             src1JitVal = Bldr->CreateSExt( src1JitVal,
                                           (IntegerType *)resIRtype, "SExt" ) ;
          }
          if ( src2Width < resWidth )
          {
             DPT0( "VV9C205: ", VV_VD,
            "src2JitVal = Bldr->CreateSExt( src2JitVal, (IntegerType *)resIRtype, 'SExt' )\n" );
             src2JitVal = Bldr->CreateSExt( src2JitVal,
                                           (IntegerType *)resIRtype, "SExt" ) ;
          }

////      resJitVal = jit_insn_sub(f, src1JitVal, src2JitVal);

          DPT0( "VV9C210: ", VV_VD,
         "resJitVal = Bldr->CreateSub( src1JitVal , src2JitVal , 'IntSub' )\n" );
          resJitVal = Bldr->CreateSub( src1JitVal , src2JitVal , "IntSub" );

////      res->setJitValue(this, f, resJitVal, block);
          res->setJitValue(this, Bldr, resJitVal, PCBlk);

////      if (opc == PCIT::SUB_MFLT64_MFLT64_MFLT64)
////        jitProcessFloatExceptionCheck(f, res, &errorJitLabel, block);

////      // No opt support for this instruction
////      if (opc == PCIT::SUB_MBIN64S_MBIN64S_MBIN64S)
////        lowerOptLevel = TRUE;


          break;
        }

        case PCIT::MUL_MBIN64S_MBIN64S_MBIN64S:
        case PCIT::MUL_MBIN64S_MBIN32S_MBIN32S:
        case PCIT::MUL_MBIN64S_MBIN16S_MBIN32S:
        case PCIT::MUL_MBIN16S_MBIN16S_MBIN16S:
        case PCIT::MUL_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::MUL_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::MUL_MBIN32S_MBIN16S_MBIN32S:
        {
          PCodeOperand *src1,      *src2,      *res       ;
          jit_value_t   src1JitVal, src2JitVal, resJitVal ;
          jit_type_t    src1IRtype, src2IRtype, resIRtype ;

          res  = PCInst->getWOps()[0];
          src1 = PCInst->getROps()[0];
          src2 = PCInst->getROps()[1];
          PRINT_DETAILS_OF_WROp( 0, res );
          PRINT_DETAILS_OF_RDOp( 0, src1 );
          PRINT_DETAILS_OF_RDOp( 1, src2 );

////      src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src1IRtype = src1->getJitType( this );
          src1JitVal = src1->getJitValue(this, Bldr, src1IRtype, PCBlk);

////      src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);
          src2IRtype = src2->getJitType( this );
          src2JitVal = src2->getJitValue(this, Bldr, src2IRtype, PCBlk);

          resIRtype  = res->getJitType( this );

          Int32 src1Width = src1IRtype->getIntegerBitWidth();
          Int32 src2Width = src2IRtype->getIntegerBitWidth();
          Int32 resWidth  =  resIRtype->getIntegerBitWidth();

          if ( src1Width < resWidth )
          {
             DPT0( "VV9C300: ", VV_VD,
            "src1JitVal = Bldr->CreateSExt( src1JitVal, (IntegerType *)resIRtype, 'SExt' )\n" );
             src1JitVal = Bldr->CreateSExt( src1JitVal,
                                           (IntegerType *)resIRtype, "SExt" ) ;
          }
          if ( src2Width < resWidth )
          {
             DPT0( "VV9C305: ", VV_VD,
            "src2JitVal = Bldr->CreateSExt( src2JitVal, (IntegerType *)resIRtype, 'SExt' )\n" );
             src2JitVal = Bldr->CreateSExt( src2JitVal,
                                           (IntegerType *)resIRtype, "SExt" ) ;
          }

////      resJitVal = jit_insn_mul(f, src1JitVal, src2JitVal);

          DPT0( "VV9C310: ", VV_VD,
         "resJitVal = Bldr->CreateMul( src1JitVal , src2JitVal , 'IntMul' )\n" );
          resJitVal = Bldr->CreateMul( src1JitVal , src2JitVal , "IntMul" );

////      res->setJitValue(this, f, resJitVal, block);
          res->setJitValue(this, Bldr, resJitVal, PCBlk);


////      if (opc == PCIT::MUL_MFLT64_MFLT64_MFLT64)
////        jitProcessFloatExceptionCheck(f, res, &errorJitLabel, block);

          break;
        }

        case PCIT::MOVE_MBIN32S:
        {
          PCodeOperand *src1 ;
          jit_value_t   src1JitVal ;

          // RETURN instruction must follow
          assert (PCInst->next && (PCInst->next->code[0] == PCIT::RETURN));

////      jit_label_t l1 = jit_label_undefined;
////      jit_label_t l2 = jit_label_undefined;

          src1 = PCInst->getROps()[0];
          PRINT_DETAILS_OF_RDOp( 0, src1 );

          DPT1( "VV9D002: ", VV_XD,
                "Processing MOVE_MBIN32S - 1, src1 = PCodeOp at %p\n", src1 );

////      src1JitVal = src1->getJitValue(this, f, src1->getJitType(), PCBlk);
          src1JitVal = src1->getJitValue(this, Bldr, src1->getJitType(this), PCBlk);

          DPT1( "VV9D003: ", VV_XD,
                "Processing MOVE_MBIN32S - 1, src1JitVal at %p\n", src1JitVal );

////      jit_value_t t1 = jit_insn_eq(f, src1JitVal, oneJitVal_);
////      jit_insn_branch_if(f, t1, &l1);
////      jit_insn_label(f, &l2);
////      jit_insn_return(f, fJitVal_);
////      jit_insn_label(f, &l1);
////      jit_insn_return(f, tJitVal_);

          jit_value_t Rslt = jitGenCompare( Bldr, IntCompare_EQ,
                                            src1JitVal,   IR_Const_1_ ,
                                            IR_Const_TRUE_, IR_Const_FALSE_ );

          DPT0( "VV9D005: ", VV_VD, "returned from jitGenCompare()\n");

          DPT0( "VV9D010: ", VV_VD,
         "Bldr->CreateRet( Rslt )\n" );
          Bldr->CreateRet( Rslt );

          justGeneratedRET = TRUE;
          break;
        }

        case PCIT::RETURN_IBIN32S:
        {
          jit_value_t  resJitVal ;

//        if (inst->code[1] == 1) jit_insn_return(f, tJitVal_);
//        else                    jit_insn_return(f, fJitVal_);

          if (PCInst->code[1] == 1)
                resJitVal = IR_Const_TRUE_  ;
          else
                resJitVal = IR_Const_FALSE_ ;

          DPT0( "VV9E010: ", VV_VD,
         "Bldr->CreateRet( resJitVal )\n");
          Bldr->CreateRet( resJitVal );

          justGeneratedRET = TRUE;
          break;
        }

        case PCIT::RETURN:
        {
           DPT0( "VV9F001: ", VV_VD,
          "Bldr->CreateRet( IR_Const_OK_ )\n");
           Bldr->CreateRet( IR_Const_OK_ );

           justGeneratedRET = TRUE;
           break;
        }
      }  // end of switch (opc)

#if NExprDbgLvl >= VV_VD
      if ( PCInst )
         DPT2( "VVzz000: ", VV_VD, 
               "At end of Main Loop : PCInst at %p, PCInst->next = %p \n",
                                      PCInst,       PCInst->next );

      if ( PCInst && PCInst->next )
         DPT2( "VVzz001: ", VV_VD, 
               "At end of Main Loop: next at %p has opc == %d\n",
                          PCInst->next , (int) PCInst->next->getOpcode());
#endif

    } ENDFE_INST_IN_BLOCK

    if ( ( opc != PCIT::BRANCH         ) &&  // if last one was not a BRANCH
         ( opc != PCIT::RETURN         ) &&  // and was not a RETURN
         ( opc != PCIT::RETURN_IBIN32S ) &&  // of any sort, and if 
         ( (PCBlkIndex + 1) < physBlockList.entries() ) // there are more blocks
       )
    {
       PCodeBlock* nxtBlock = physBlockList[ PCBlkIndex + 1 ];
       assert( nxtBlock );

       //
       // IMPORTANT NOTE:
       // LLVM's verifyFunction() routine absolutely requires that *every*
       // llvm::BasicBlock be terminated with either a Branch or a Return.
       // If there is a BasicBlock that is not terminated, either 
       // verifyFunction() will complain or, worse yet, it may simply
       // never return (because it calls 'exit' instead.)
       // So here we "complete" the current block.
       //
       p_IR_block_t TgtBlk = *( nxtBlock->getJitBlkLbl() );

       DPT1( "VVzzz00: ", VV_VD, 
      "Bldr->CreateBr( TgtBlk = %p )\n", TgtBlk );
       Bldr->CreateBr( TgtBlk );
    }

    DPT0( "VVzzz09: ", VV_VD, "Finished the FOREACH_INST_IN_BLOCK loop\n");

  } // END: for (PCBlkIndex=0; ...

  DPT0( "VVzzz10: ", VV_VD, "FINISHED the MAIN loop\n");

  if ( justGeneratedRET == FALSE && justHandledUnconditionBranch == FALSE )
  {
     //
     // The last block should *always* end with a Return or Unconditional Br.
     //
     DPT0( "VVzzz12: ", VV_VD, 
    "Bldr->CreateRet( IR_Const_OK_ )\n");
     Bldr->CreateRet( IR_Const_OK_ );
  }

  DPT0( "VVzzz99: ", VV_BD, "GOT PAST the final CreateRet\n");

#if NExprDbgLvl >= VV_BD
  if ( NExprDbgLvl_ >= VV_BD )
     std::cout << std::flush ; // Flush anything in the data buffer
#endif

  std::stringstream myBuffer;
  std::streambuf * old = std::cerr.rdbuf( myBuffer.rdbuf() );

#if NExprDbgLvl >= VV_BD
  if ( NExprDbgLvl_ >= VV_BD )
  {
     std::cerr << std::flush ; // Flush anything in the data buffer
     std::cerr << "QRST\n"  ; // Put out marker so we can see that we got here

     DPT0( "VVzzz08: ", VV_BD, "CALLING verifyFunction() !!! \n");
     std::cerr << std::flush ; // Flush anything in the data buffer
  }
#endif // NExprDbgLvl >= VV_BD


// LLVM Requires this function be called!
//NABoolean VF_rtn = llvm::verifyFunction(*ExpFn) ;
  NABoolean VF_rtn = llvm::verifyFunction(*ExpFn, PrintMessageAction) ;


#if NExprDbgLvl >= VV_BD
  if ( NExprDbgLvl_ >= VV_BD )
  {
    DPT0( "VVzzz10: ", VV_VD, "RETURNED FROM verifyFunction() !!! \n");
    std::cerr << std::flush ; // Flush anything in the data buffer
    if ( VF_rtn )
    {
       DPT0( "VVzzz12: ", VV_VD, "llvm::verifyFunction returned TRUE -- "
                                 "meaning function is corrupt ???\n" );
       std::string myText = myBuffer.str();
       DPT2( "VVzzz14: ", VV_VD, "%ld bytes of text:\n %s\n",
                                  myText.length(),     myText.data() );
    }
    else
    {
       DPT0( "VVzzz16: ", VV_VD, "llvm::verifyFunction returned FALSE -- "
                                 "meaning function verifies OK !\n" );
    }
  }
#endif // NExprDbgLvl >= VV_BD

  std::cerr.rdbuf(old); // Restore stderr

  // *********************************************************************
  // Perform Optimizations
  // *********************************************************************

  //
  // Change the following "if (1)" to "if (0)" when debugging future
  // changes to Native Expressions and you would like the generated
  // native expression to be more easily understood code ... OR if you 
  // suspect that these LLVM optimizations might be resulting in 
  // incorrect generated code.
  //
  if ( 1 )
  {
    FunctionPassManager ExprFPM(TheModule);

    // Set up the optimizer pipeline.  Start with registering info about
    // how the target lays out data structures.
    ExprFPM.add(new DataLayout(*TheExecutionEngine->getDataLayout()));
    // Provide basic AliasAnalysis support for GVN.
    ExprFPM.add(createBasicAliasAnalysisPass());
    // Do simple "peephole" optimizations and bit-twiddling optzns.
    ExprFPM.add(createInstructionCombiningPass());
    // Reassociate expressions.
    ExprFPM.add(createReassociatePass());
    // Eliminate Common SubExpressions.
    ExprFPM.add(createGVNPass());
    // Simplify the control flow graph (deleting unreachable blocks, etc).
    ExprFPM.add(createCFGSimplificationPass());

    ExprFPM.doInitialization();

    // Set the global so the code gen can use this.
    // static FunctionPassManager * TheFPM = &ExprFPM;

    // Optimize the function.
    ExprFPM.run(*ExpFn);
  }

#if NExprDbgLvl >= VV_BD
  if ( NExprDbgLvl_ >= VV_BD )
  {
     // ********************************************************************
     //  Dump the Intermediate Representation (IR)
     // ********************************************************************

     NExLog("\nIntermediate Representation (IR) dump:");
     NExLog("\n======================================\n\n");

     // Dump the module/function

     // If CQD set to dump NE debug stuff to a file, then:
     if ( NExDbgInfoPtr_->getNExLogPath() &&
          NExDbgInfoPtr_->getNExLogPath()[0] != '\0' )
     {
        std::string myErrorInfoStr;
        raw_fd_ostream *myOuts = new raw_fd_ostream(
                       (const char *)NExDbgInfoPtr_->getNExLogPath(),
                       myErrorInfoStr,
                       (unsigned) llvm::raw_fd_ostream::F_Append
                       );

        TheModule->print( *myOuts, (llvm::AssemblyAnnotationWriter*)NULL );
     }
     else // Dump it to stdout
        TheModule->print( outs(), (llvm::AssemblyAnnotationWriter*)NULL );

  }
#endif // NExprDbgLvl >= VV_BD

  // *********************************************************************
  // jit -- and copy binary into buffer
  // *********************************************************************


  // JIT the function, returning a function pointer.

  MachineCodeInfo myMachineCodeInfo;

  TheExecutionEngine->runJITOnFunction( ExpFn, &myMachineCodeInfo );

  void *FPtr = myMachineCodeInfo.address();
  size_t funcBufLen = myMachineCodeInfo.size() ;

  //
  // DON'T NEED THIS IN PRODUCT, but something like this might be
  // useful in debugging.
  //
#if 0
#define MAX_EXEC_BUF_LEN 65536  /* Arbitrary maximum length - pick your own */
  if ( funcBufLen >= MAX_EXEC_BUF_LEN )
  {
     printf( "MACHINE CODE LENGTH BIGGER THAN 65536 -- consequently questionable\n");
     delete Bldr;
     //delete TheModule;
     delete TheExecutionEngine;

     //
     // Restore signal handlers to what they were on entry.  ALSO, if this is
     // the first time LLVM was called, save the LLVM handlers!
     //
     for (Int32 ii = 0; ii < NumSaveSigs ; ii++ )
     {
       Int32 ret = sigaction( SaveSigs[ii], &(SavedSigInfo[ii].SigAct),
                    (LLVMhandlersSaved == FALSE) ?
                   &(SavedLLVMhandlers[ii].SigAct) : (struct sigaction *)NULL);
       assert( ret == 0 );
     }
     LLVMhandlersSaved = TRUE ;
     pthread_mutex_unlock( &Our_LLVM_mutex ); // Other threads could use LLVM now


     printf("EXITING FROM layoutNativeCode() - funcBufLen >= 65536 \n");
     std::cout << std::flush ; // Flush anything in the data buffer
     expr_->setEvalPtr( (ex_expr::evalPtrType)( (CollIndex) 0 ) );//Ensure NULL!
     return;
  }
#endif

#if NExprDbgLvl >= VV_BD
  if ( ( NExprDbgLvl_ >= VV_BD ) &&
       ( 
         ( ( NExDbgInfoPtr_->getNExStmtSrc() != NULL ) &&
           ( NExDbgInfoPtr_->getNExStmtSrc()[0] != '\0' ) )
       )
     )
  {
     // ********************************************************************
     //  Dump the assembly code
     // ********************************************************************

     char insnLine[256];
     char insnLine2[256];

     char * NExLogPth    = NULL ;

     if ( NExDbgInfoPtr_  &&  NExDbgInfoPtr_ > (NExDbgInfo *)4096 ) // May be offset
          NExLogPth = NExDbgInfoPtr_->getNExLogPath() ;

     sprintf( insnLine, "\nNative Expr (Length: %ld bytes)", funcBufLen );
     NExLog(  insnLine );
     NExLog( "\n======================================\n" );

     if ( funcBufLen <= 0 )
     {
        NExLog( "MACHINE CODE LENGTH <= 0\n");
        NExLog("EXITING FROM layoutNativeCode() - funcBufLen <= 0 !!\n");
        std::cout << std::flush ; // Flush anything in the data buffer
        delete Bldr;
        //delete TheModule;
        delete TheExecutionEngine;

        //
        // Restore signal handlers to what they were on entry.  ALSO, if this is
        // the first time LLVM was called, save the LLVM handlers!
        //
        for (Int32 ii = 0; ii < NumSaveSigs ; ii++ )
        {
          Int32 ret = sigaction( SaveSigs[ii], &(SavedSigInfo[ii].SigAct),
                       (LLVMhandlersSaved == FALSE) ?
                      &(SavedLLVMhandlers[ii].SigAct) : (struct sigaction *)NULL);
          assert( ret == 0 );
        }
        LLVMhandlersSaved = TRUE ;
        pthread_mutex_unlock( &Our_LLVM_mutex ); // Other threads could use LLVM now

        expr_->setEvalPtr( (ex_expr::evalPtrType)( (CollIndex) 0 ) );//Ensure NULL!
        return;
     }

     ud_t ud_obj;
     ud_init(&ud_obj);
     ud_set_input_buffer(&ud_obj, (uint8_t*)FPtr, (size_t) funcBufLen);
     ud_set_mode(&ud_obj, 64);
     ud_set_syntax(&ud_obj, UD_SYN_ATT);
     ud_set_vendor(&ud_obj, UD_VENDOR_INTEL);

     while (ud_disassemble(&ud_obj))
     {
       if ( 0xcd == * ud_insn_ptr(&ud_obj) ) break; // end of data bug workaround

       // Find length of next assembler language instruction
       sprintf( insnLine, "%s", ud_insn_asm(&ud_obj) ) ;
       Int32 insnLen = str_len( insnLine );

       if ( insnLen >= 26 )
       {
          str_pad(&insnLine[insnLen], 1, ' ');
          insnLine[insnLen+1] ='\0' ;
       }
       else
       {
          str_pad(&insnLine[insnLen],(26-insnLen+1), ' ');
          insnLine[27] ='\0' ;
       }
       insnLen = str_len( insnLine ); //Increase insnLen for added spaces

       Int32 ooo = 0;
       for (Int32 jjj = 0; jjj < insnLen; jjj++ , ooo++)
       {
          // If output will be going to stdout (via printf)
          // then we must double any '%' chars in the Assembler Language
          //
          if ( ( ! NExLogPth || *NExLogPth == '\0' ) &&
               (insnLine[jjj] == '%') )
          {
             insnLine2[ooo] = '%';
             ooo++;
          }
          insnLine2[ooo] = insnLine[jjj] ;
       }
       insnLine2[ooo] = '\0'; // Ensure termination
       sprintf( NExBuf, "\t%s%s\n", insnLine2, ud_insn_hex(&ud_obj) );
       NExLog(  NExBuf );
     }
  }
#endif // NExprDbgLvl >= VV_BD
  // Get entry point into function and add code into constants array
  ex_expr::evalPtrType entry = (ex_expr::evalPtrType)FPtr;
  CollIndex* offPtr = addConstant((void*)entry, funcBufLen, 8);

#if NExprDbgLvl >= VV_I0
  if ( NExprDbgLvl_ >= VV_I0 )
  {
     struct rusage endTime;
     (void) getrusage( RUSAGE_THREAD, &endTime );

     Int64 totalTime = ( endTime.ru_utime.tv_sec - begTime.ru_utime.tv_sec   ) * 1000000 +
                       ( endTime.ru_utime.tv_usec - begTime.ru_utime.tv_usec ) ;

     DPT2( "VVzzz99: ", VV_I0, "NORMAL EXIT from layoutNativeCode(): "
           "%ld microseconds to translate %d PCODE instructions\n",
                                            totalTime, PCI_count );
  }
#endif // NExprDbgLvl >= VV_I0

#if 1 /* Use #if 0 to generate/compile, but NOT actually execute generated code */
  // Store offset into evalPtr_
  expr_->setEvalPtr((ex_expr::evalPtrType)((long)*offPtr));

  // Mark this expression appropriately so that the native function gets called
  expr_->setPCodeMoveFastpath(TRUE);
  expr_->setPCodeNative(TRUE);
#endif

  TheExecutionEngine->freeMachineCodeForFunction( ExpFn );
  delete Bldr;
  //delete TheModule;
  delete TheExecutionEngine;

  //
  // Restore signal handlers to what they were on entry.  ALSO, if this is
  // the first time LLVM was called, save the LLVM handlers!
  //
  for (Int32 ii = 0; ii < NumSaveSigs ; ii++ )
  {
     Int32 ret = sigaction( SaveSigs[ii], &(SavedSigInfo[ii].SigAct),
                  (LLVMhandlersSaved == FALSE) ?
                 &(SavedLLVMhandlers[ii].SigAct) : (struct sigaction *)NULL);
     assert( ret == 0 );
  }
  LLVMhandlersSaved = TRUE ;
  pthread_mutex_unlock( &Our_LLVM_mutex ); // Other threads could use LLVM now
  return ;
}

void PCodeCfg::layoutNativeCode(Space* showplanSpace)
{
  if (expr_->getPCodeNative())
  {
    NABoolean firstTimeSeen = TRUE;
    char line[256];
    char insnLine[256];
    Int32 i;
    void *FPtr =  expr_->getConstantsArea() + expr_->getEvalPtrOffset();
    Long nativeCodeLen = expr_->getConstsLength() - expr_->getEvalPtrOffset();

    // First print out a header for the native expr dump
    sprintf( line, "Native Expr (Length: %ld bytes)", nativeCodeLen );
    showplanSpace->allocateAndCopyToAlignedSpace(line, str_len(line),
                                                 sizeof(short));

    ud_t ud_obj;
    ud_init(&ud_obj);
    ud_set_input_buffer(&ud_obj, (uint8_t*)(FPtr), (size_t) nativeCodeLen);
    ud_set_mode(&ud_obj, 64);
    ud_set_syntax(&ud_obj, UD_SYN_ATT);
    ud_set_vendor(&ud_obj, UD_VENDOR_INTEL);

    while (ud_disassemble(&ud_obj))
    {
       if ( 0xcd == * ud_insn_ptr(&ud_obj) ) break; // end of data bug workaround

       // Find length of next assembler language instruction
       sprintf( insnLine, "%s", ud_insn_asm(&ud_obj) ) ;
       Int32 insnLen = str_len( insnLine );

       if ( insnLen >= 26 )
       {
          str_pad(&insnLine[insnLen], 1, ' ');
          insnLine[insnLen+1] ='\0' ;
       }
       else
       {
          str_pad(&insnLine[insnLen],(26-insnLen+1), ' ');
          insnLine[27] ='\0' ;
       }

       sprintf( line, "\t%s%s", insnLine, ud_insn_hex(&ud_obj) );
       showplanSpace->allocateAndCopyToAlignedSpace(line, str_len(line), sizeof(short));
    }
    system(line);

    // Add a newline to start the next expression off cleanly.
    showplanSpace->allocateAndCopyToAlignedSpace("\n", 1, sizeof(short));
  }

}

#endif /* NA_LINUX_LLVMJIT */

#ifdef NA_LINUX_LIBJIT

void PCodeCfg::layoutNativeCode(Space* showplanSpace = NULL)
{
  NABoolean debug = FALSE;
  NABoolean lowerOptLevel = FALSE; // Used to reduce jit opt level.

  if (getenv("NATIVE_EXPR_DEBUG"))
    debug = TRUE;

  PCodeOperand *src1, *src2, *res;

  jit_value_t resJitVal, src1JitVal, src2JitVal;
  jit_value_t tRes, tSrc1, tSrc2, loopIndex;
  jit_value_t eJitVal;
  jit_label_t* jitLabel = NULL;
  jit_label_t errorJitLabel = jit_label_undefined;
  jit_type_t type;

  jit_value_t padJitVal;

  Int32 opc;
  CollIndex i, j, blockIndex;

  Int32 skipInst = 0;

  PCodeBinary* pCode = expr_->getPCodeBinary();

  // First see if this graph can be compiled natively
  if (!canGenerateNativeExpr())
    return;

  // First compute the dominator tree
  computeDomTree();

  // Create context to hold the JIT's primary state
  jit_context_t context = jit_context_create();

  // Lock the context while we build and compile the function.  This is only
  // really necessary in a multithreaded environment.
  jit_context_build_start(context);

  // Generate code for getting tupp pointers
  //
  // 1. Get atp pointer (p) passed in
  // 2. Add bytes to get to tuple we care about
  //    off = &(tempAtp->getTupp(atpIndex)) - (tempAtp)
  //    temp1 = param[atp] + off;
  // 3. Dereference pointer to get tupp_descriptor pointer
  //    temp2 = load(temp1);
  // 4. Add bytes to get to dataTuple
  //    off = &(tempTuppDesc.getTupleAddress()

  atp_struct tempAtp;
  tupp tempTupp;
  tupp_descriptor tempTuppDesc;

/*
  ex_cri_desc criDesc;
  tempAtp.setCriDesc(&criDesc);
*/

  // Build the function signature.
  jit_type_t params[4];
  params[0] = jit_type_void_ptr;  // Int32*      pCode32
  params[1] = jit_type_void_ptr;  // atp_struct* atp2
  params[2] = jit_type_void_ptr;  // atp_struct* atp2
  params[3] = jit_type_void_ptr;  // Int32*      constants

  jit_type_t signature =
    jit_type_create_signature(jit_abi_cdecl, jit_type_int, params, 4, 1);

  // Create function
  jit_function_t f = jit_function_create(context, signature);

  // Create commonly referenced constants
  oneJitVal_  = jit_value_create_nint_constant(f, jit_type_int, 1);
  zeroJitVal_ = jit_value_create_nint_constant(f, jit_type_int, 0);
  neg1JitVal_ = jit_value_create_nint_constant(f, jit_type_int, -1);
  jit_value_t zeroJitVal16=jit_value_create_nint_constant(f, jit_type_short, 0);
  jit_value_t neg1JitVal16=jit_value_create_nint_constant(f, jit_type_short,-1);
  padJitVal = jit_value_create_nint_constant(f, jit_type_ubyte, ' ');
  jit_value_t shiftRVal = jit_value_create_nint_constant(f, jit_type_int, 31);
  okJitVal_ = jit_value_create_nint_constant(f, jit_type_int,ex_expr::EXPR_OK);
  tJitVal_ = jit_value_create_nint_constant(f, jit_type_int,ex_expr::EXPR_TRUE);
  fJitVal_ = jit_value_create_nint_constant(f,jit_type_int,ex_expr::EXPR_FALSE);
  eJitVal  = jit_value_create_nint_constant(f,jit_type_int,ex_expr::EXPR_ERROR);

  // Used for bignum related ops when manipulating sign bit.
  jit_value_t jitVal16 = jit_value_create_nint_constant(f, jit_type_uint, 16);
  jit_value_t jitValMask16 =
    jit_value_create_nint_constant(f, jit_type_ushort, 0x8000);
  jit_value_t jitValMask32 =
    jit_value_create_nint_constant(f, jit_type_ushort, 0x80000000);
  jit_value_t jitValNotMask16 =
    jit_value_create_nint_constant(f, jit_type_ushort, 0x7fff);
  jit_value_t jitValNotMask32 =
    jit_value_create_nint_constant(f, jit_type_ushort, 0x7fffffff);

  // Allocate pointers to global defs available at runtime.  Note, if they are
  // not referenced by the pcode graph, these loads will be deleted by the jit.
  jit_value_t globTableJitVal = jit_value_get_param(f, 0);
  jit_value_t nullTable = jit_insn_load_relative(f, globTableJitVal,
                                                 JIT_GLOB_NULL_TABLE,
                                                 jit_type_void_ptr);

  hashTableJitVal_ = jit_insn_load_relative(f, globTableJitVal,
                                            JIT_GLOB_RANDOM_HASH_VALS_TABLE,
                                            jit_type_void_ptr);

  jit_value_t compareWithPadJitVal = jit_insn_load_relative(f, globTableJitVal,
                                       JIT_GLOB_COMPARE_WITH_PAD_FUNC,
                                       jit_type_void_ptr);

  jit_value_t strcmpJitVal = jit_insn_load_relative(f, globTableJitVal,
                                       JIT_GLOB_STRCMP_FUNC, jit_type_void_ptr);

  jit_value_t strcpyJitVal = jit_insn_load_relative(f, globTableJitVal,
                                       JIT_GLOB_STRCPY_FUNC, jit_type_void_ptr);

  jit_value_t dblLeJitVal = jit_insn_load_relative(f, globTableJitVal,
                                 JIT_GLOB_DBL_LE_CMP_FUNC, jit_type_void_ptr);
  jit_value_t dblGeJitVal = jit_insn_load_relative(f, globTableJitVal,
                                 JIT_GLOB_DBL_GE_CMP_FUNC, jit_type_void_ptr);
  jit_value_t dblLtJitVal = jit_insn_load_relative(f, globTableJitVal,
                                 JIT_GLOB_DBL_LT_CMP_FUNC, jit_type_void_ptr);
  jit_value_t dblGtJitVal = jit_insn_load_relative(f, globTableJitVal,
                                 JIT_GLOB_DBL_GT_CMP_FUNC, jit_type_void_ptr);
  jit_value_t dblEqJitVal = jit_insn_load_relative(f, globTableJitVal,
                                 JIT_GLOB_DBL_EQ_CMP_FUNC, jit_type_void_ptr);
  jit_value_t dblNeJitVal = jit_insn_load_relative(f, globTableJitVal,
                                 JIT_GLOB_DBL_NE_CMP_FUNC, jit_type_void_ptr);
  jit_value_t reportErrJitVal = jit_insn_load_relative(f, globTableJitVal,
                                 JIT_GLOB_REPORT_ERR_FUNC, jit_type_void_ptr);
  jit_value_t bigSubJitVal = jit_insn_load_relative(f, globTableJitVal,
                               JIT_GLOB_BIG_SUB_FUNC, jit_type_void_ptr);


  jit_type_t compareParams[5] = {jit_type_void_ptr, jit_type_int,
                                 jit_type_void_ptr, jit_type_int,
                                 jit_type_ubyte};
  jit_type_t strcmpParams[5] = {jit_type_void_ptr, jit_type_void_ptr,
                                jit_type_uint};
  jit_type_t strcpyParams[5] = {jit_type_void_ptr, jit_type_void_ptr,
                                jit_type_uint};
  jit_type_t dblParams[2] = {jit_type_float64, jit_type_float64};
  jit_type_t reportErrParams[2] = {jit_type_void_ptr, jit_type_void_ptr};
  jit_type_t bigParams[4] = {jit_type_void_ptr, jit_type_void_ptr,
                             jit_type_void_ptr, jit_type_uint};


  jit_type_t compareSig = jit_type_create_signature (jit_abi_cdecl,
                            jit_type_int, compareParams, 5, 1);

  jit_type_t strcmpSig = jit_type_create_signature (jit_abi_cdecl,
                            jit_type_int, strcmpParams, 3, 1);
  jit_type_t strcpySig = jit_type_create_signature (jit_abi_cdecl,
                            jit_type_int, strcpyParams, 3, 1);
  jit_type_t dblSig = jit_type_create_signature (jit_abi_cdecl,
                          jit_type_int, dblParams, 2, 1);
  jit_type_t reportErrSig = jit_type_create_signature (jit_abi_cdecl,
                              jit_type_int, reportErrParams, 2, 1);
  jit_type_t bigSig = jit_type_create_signature (jit_abi_cdecl,
                        jit_type_int, bigParams, 4, 1);



  // Allocate incoming parameters
  jitParams_ = (jit_value_t*) new(heap_) jit_value_t[4 + pCode[0]];
  for (i=0, j=1; i < (CollIndex)pCode[0]; i++, j+=2)
  {
    Int32 off1, off2;

    // First get the atp we care about
    jit_value_t t1 = jit_value_get_param(f, 1 + pCode[j]);

    // Get the tupp descriptor pointer we care about
    off1 =
      (char*)(&(tempAtp.getTuppForNativeExpr(pCode[j+1]))) - (char*)(&tempAtp);
    jit_value_t t2 = jit_insn_load_relative(f, t1, off1, jit_type_void_ptr);

#if NULL_TUPLE_PTRS
    jit_label_t storeNullLabel = jit_label_undefined;
    jit_value_t res = jit_value_create(f, jit_type_void_ptr);

    jit_value_t t4 = jit_insn_eq(f, t2, zeroJitVal_);
    jit_insn_branch_if(f, t4, &storeNullLabel);
#endif

    // Lastly, get the tuple pointer
    off2 = 12; // (&(tempTuppDesc.getTupleAddress())) - (&tempTuppDesc);
    jit_value_t t3 = jit_insn_load_relative(f, t2, off2, jit_type_void_ptr);

#if NULL_TUPLE_PTRS
  // Set tuple pointers to nullData if they are NULL.  Only need to do this for
  // 2.4.2 and below.
    jit_insn_store(f, res, t3);

    jit_label_t storeLabel = jit_label_undefined;

    t4 = jit_insn_eq(f, t3, zeroJitVal_);
    jit_insn_branch_if_not(f, t4, &storeLabel);

    jit_insn_label(f, &storeNullLabel);
    jit_insn_store(f, res, nullTable);

    jit_insn_label(f, &storeLabel);

    // Make t3 the result so that we properly store (internally) what the tuple
    // pointer is.
    t3 = res;
#endif

    jitParams_[4+i] = t3;
  }

  //
  // Allocate incoming constant pointer (needed for strings and floats).  The
  // constantsArea_ pointer is 88 bytes from the object (note, we hardcode this
  // since access to the member variables are protected, and therefore not
  // accessible for use in calculating this automatically).  The 88 byte offset
  // comes from "20" being the offset to the "eyeCatcher_" field, plus "56" to
  // the constants area;
  //
  // Do the same for the temps array pointer.  This array is used to store to
  // those temps which usually don't fit within a native container - e.g.
  // strings and bignums.  The temps array is just above the constants area in
  // the expr - i.e. at "64" bytes from the eyeCatcher_.
  //
  jit_value_t exprJitVal = jit_value_get_param(f,3);
  jitParams_[1] = jit_insn_load_relative(f, exprJitVal, 88, jit_type_void_ptr);
  jitParams_[2] = jit_insn_load_relative(f, exprJitVal, 96, jit_type_void_ptr);

  // Create temp values used for strings (or data in general)
  tRes = jit_value_create(f, jit_type_void_ptr);  // ptr to walk string
  tSrc1 = jit_value_create(f, jit_type_void_ptr);  // ptr to walk string
  tSrc2 = jit_value_create(f, jit_type_void_ptr);  // ptr to walk string
  loopIndex = jit_value_create(f, jit_type_int);   // loop index for strcmp

  // Allocate and initialize opData array for use with CLAUSE_EVAL instructions
  PCodeOperand* opData[(3*ex_clause::MAX_OPERANDS)];
  PCodeOperand** opDataNulls = &(opData[0]);
  PCodeOperand** opDataVals = &(opData[2*ex_clause::MAX_OPERANDS]);
  str_pad((char*)opData, (3*ex_clause::MAX_OPERANDS)*sizeof(PCodeOperand*), 0);

  // Create the list of basic blocks in physical layout order
  BLOCKLIST physBlockList(heap_);

  createPhysList(physBlockList);

  // Loop through each block, generating instructions on a per block basis.  We
  // stop immediately if a failure was seen during code gen.

  for (blockIndex=0;
       (blockIndex < physBlockList.entries()) && !getJitFailureSeen();
       blockIndex++)
  {
    PCodeBlock* block = physBlockList[blockIndex];

    // Lay out the label for this block.
    jit_insn_label(f, block->getJitLabel());

    FOREACH_INST_IN_BLOCK(block, inst) {

      // If a failure occurred, stop translation immediately.
      if (getJitFailureSeen())
        break;

      // Sometimes native code is generated for multiple pcode instructions in
      // one pass, and so "skipped" instructions don't need to be processed.
      if (skipInst > 0) {
        skipInst--;
        continue;
      }

      opc = inst->getOpcode();
      switch (opc) {

        case PCIT::MOVE_MATTR5_MATTR5:
        {
          jit_type_t vcType;
          jit_value_t vcLenJitVal;

          src1 = inst->getROps()[0]; 
          res = inst->getWOps()[0];

          if (src1->getVcIndicatorLen() == 2) {
            vcType = jit_type_ushort;
            vcLenJitVal = jit_value_create_nint_constant(f, jit_type_int, 2);
          }
          else {
            vcType = jit_type_uint;
            vcLenJitVal = jit_value_create_nint_constant(f, jit_type_int, 4);
          }

          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          resJitVal = res->getJitValue(this, f, res->getJitType(), block);

          jit_insn_store(f, tRes, resJitVal);
          jit_insn_store(f, tSrc1, src1JitVal);

          jit_value_t srcLen = jit_insn_load_relative(f, src1JitVal, 0, vcType);
          srcLen = jit_insn_add(f, srcLen, vcLenJitVal);

          genUnalignedMemcpy(f, tRes, tSrc1, srcLen);

#if 0
          jit_value_t tgtLen = jit_insn_load_relative(f, resJitVal, 0, vcType);

          // Just store in length for now.
          jit_insn_store_relative(f, resJitVal, 0, srcLen);

          src1JitVal =
            jit_insn_add_relative(f, src1JitVal, src1->getVcIndicatorLen());
          resJitVal =
            jit_insn_add_relative(f, resJitVal, src1->getVcIndicatorLen());

          jit_insn_store(f, tSrc1, src1JitVal);
          jit_insn_store(f, tSrc2, src2JitVal);

          // Now copy string
          jit_insn_memcpy(f, resJitVal, src1JitVal, srcLen);
#endif

          break;
        }

        case PCIT::BRANCH:
        {
          jit_insn_branch(f, block->getTargetBlock()->getJitLabel());
          break;
        }


        case PCIT::BRANCH_OR:
        case PCIT::BRANCH_AND:
        case PCIT::BRANCH_OR_CNT:
        case PCIT::BRANCH_AND_CNT:
        {
          src1 = inst->getROps()[0]; 
          res = inst->getWOps()[0];

          jit_value_t takenJitVal =
            ((opc == PCIT::BRANCH_OR) || (opc == PCIT::BRANCH_OR_CNT))
              ? oneJitVal_ : zeroJitVal_;

          jitLabel = block->getTargetBlock()->getJitLabel();

          src1JitVal = src1->getJitValue(this, f, jit_type_int, block);

          if (res->isVar() || block->isOperandLiveInSuccs(res))
            res->storeJitValue(this, f, jit_type_int, src1JitVal, block, NULL);

          resJitVal = jit_insn_eq(f, src1JitVal, takenJitVal);
          jit_insn_branch_if(f, resJitVal, jitLabel);

          break;
        }

        case PCIT::NNB_SPECIAL_NULLS_MBIN32S_MATTR3_MATTR3_IBIN32S_IBIN32S:
        {
          jit_label_t src1LabelNull = jit_label_undefined;
          jit_label_t ret1Label = jit_label_undefined;
          jit_label_t endLabel = jit_label_undefined;

          jitLabel = block->getTargetBlock()->getJitLabel();

          src1 = inst->getROps()[0];
          src2 = inst->getROps()[1];

          res = inst->getWOps()[0];

          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);

          jit_insn_branch_if(f, src1JitVal, &src1LabelNull);
          jit_insn_branch_if(f, src2JitVal, &ret1Label);

          // Both are not null - store 0 and take target branch
          res->storeJitValue(this, f, jit_type_int, zeroJitVal_, block);
          jit_insn_branch(f, jitLabel);

          // Src1 is null - what about src2?
          jit_insn_label(f, &src1LabelNull);
          jit_insn_branch_if(f, src2JitVal, &ret1Label);

          // Src2 is not null, so return 0
          res->storeJitValue(this, f, jit_type_int, zeroJitVal_, block);
          jit_insn_branch(f, &endLabel);

          // Return 1
          jit_insn_label(f, &ret1Label);
          res->storeJitValue(this, f, jit_type_int, oneJitVal_, block);

          // End label to jump to
          jit_insn_label(f, &endLabel);

          break;
        }

        case PCIT::NNB_MATTR3_IBIN32S:
        {
          src1 = inst->getROps()[0]; 
          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);

          jitLabel = block->getTargetBlock()->getJitLabel();
          jit_insn_branch_if_not(f, src1JitVal, jitLabel);

          break;
        }

        case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
        case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
        {
          jit_value_t orRes;

          src1 = inst->getROps()[0]; 
          src2 = inst->getROps()[1]; 

          res = inst->getWOps()[0];

          NABoolean forComp = (opc ==
            PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S);

          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);

          orRes = jit_insn_or(f, src1JitVal, src2JitVal);

          jitLabel = block->getTargetBlock()->getJitLabel();

          if (res->isVar() || block->isOperandLiveInSuccs(res)) {
            res->storeNullJitValueAndBranch(this, f, orRes, jitLabel,
                                            forComp, block);
          }
          else {
            jit_insn_branch_if_not(f, orRes, jitLabel);
          }

          break;
        }

        case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S:
        case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_IATTR3_IBIN32S:
        {
          src1 = inst->getROps()[0]; 
          res = inst->getWOps()[0];

          NABoolean forComp =
            (opc == PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S);

          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          jitLabel = block->getTargetBlock()->getJitLabel();

          if (res->isVar() || block->isOperandLiveInSuccs(res)) {
            res->storeNullJitValueAndBranch(this, f, src1JitVal, jitLabel,
                                            forComp, block);
          }
          else {
            jit_insn_branch_if_not(f, src1JitVal, jitLabel);
          }

          break;
        }

        case PCIT::FILL_MEM_BYTES_VARIABLE:
        case PCIT::FILL_MEM_BYTES:
        {
#ifdef NO_FILL_MEM_BYTES
          break;
#endif
          Int32 fillVal, fillLen;

          res = inst->getWOps()[0];

          // Also, no need to zero out temporary operands, since we just care
          // about their values being appropriately nulled out.
          if (res->isTemp())
            break;

          resJitVal = res->getJitValue(this, f, res->getJitType(), block);

          if (opc == PCIT::FILL_MEM_BYTES_VARIABLE)
          {
            jit_value_t lenJitVal;

            Int32 vcLen = res->getVcIndicatorLen();
            jit_type_t vcType = (vcLen == 2) ? jit_type_ushort : jit_type_uint;

            lenJitVal = jit_value_create_nint_constant(f, vcType,inst->code[6]);
            
            // Write out vc length first
            jit_insn_store_relative(f, resJitVal, 0, lenJitVal);

            // Move pointer up
            resJitVal = jit_insn_add_relative(f, resJitVal, vcLen);

            // Reduce fill length to no longer include vc length indicator
            fillLen = inst->code[6] - vcLen;
            fillVal = inst->code[7];
          }
          else {
            fillLen = inst->code[3];
            fillVal = inst->code[4];
          }

          // FIXME: If varchar, we currently set the vc length to the max length
          // of the varchar, but does that make sense if the value is NULL?
          // This only gets called when the the varchar is NULL, and so it's
          // length will always be zero, so why write it out?.

          genUnalignedMemset(f, resJitVal, fillVal, fillLen);

          break;
        }

        case PCIT::COMP_MBIN32S_MASCII_MASCII_IBIN32S_IBIN32S_IBIN32S:
        case PCIT::COMP_MBIN32S_MATTR5_MATTR5_IBIN32S:
        {
          jit_value_t len1, len2;

          src1 = inst->getROps()[0]; 
          src2 = inst->getROps()[1];

          // Need to get pointer to operands
          src1JitVal = src1->getJitValue(this, f, jit_type_void_ptr, block);
          src2JitVal = src2->getJitValue(this, f, jit_type_void_ptr, block);

          // Setup for varchar and return length (if we're varchar)
          if (src1->isVarchar())
            len1 = genStringSetup(f, &src1JitVal, src1);
          else
            len1= jit_value_create_nint_constant(f,jit_type_int,src1->getLen());

          if (src2->isVarchar())
            len2 = genStringSetup(f, &src2JitVal, src2);
          else
            len2= jit_value_create_nint_constant(f,jit_type_int,src2->getLen());


          // Set up arguments and make indirect call to compare routine.
          jit_value_t args[5] = {src1JitVal, len1, src2JitVal, len2, padJitVal};
          resJitVal = jit_insn_call_indirect(f, compareWithPadJitVal,
                                             compareSig, args, 5, 0);

          Int32 subOpc = (opc == PCIT::COMP_MBIN32S_MATTR5_MATTR5_IBIN32S)
              ? inst->code[13] : inst->code[9];

          // Generate code for the appropriate compare operation.
          switch (subOpc) {
            case ITM_EQUAL:
              resJitVal = jit_insn_eq(f, resJitVal, zeroJitVal_);
              break;
            case ITM_NOT_EQUAL:
              resJitVal = jit_insn_ne(f, resJitVal, zeroJitVal_);
              break;
            case ITM_LESS:
              resJitVal = jit_insn_lt(f, resJitVal, zeroJitVal_);
              break;
            case ITM_LESS_EQ:
              resJitVal = jit_insn_le(f, resJitVal, zeroJitVal_);
              break;
            case ITM_GREATER:
              resJitVal = jit_insn_gt(f, resJitVal, zeroJitVal_);
              break;
            case ITM_GREATER_EQ:
              resJitVal = jit_insn_ge(f, resJitVal, zeroJitVal_);
              break;
          }

          inst->getWOps()[0]->setJitValue(this, f, resJitVal, block);

          break;
        }

        case PCIT::LE_MBIN32S_MFLT64_MFLT64:
        case PCIT::GE_MBIN32S_MFLT64_MFLT64:
        case PCIT::LT_MBIN32S_MFLT64_MFLT64:
        case PCIT::GT_MBIN32S_MFLT64_MFLT64:
        case PCIT::EQ_MBIN32S_MFLT64_MFLT64:
        case PCIT::NE_MBIN32S_MFLT64_MFLT64:
        {
          src1 = inst->getROps()[0]; 
          src2 = inst->getROps()[1];

          // Need to get pointer to operands
          src1JitVal = src1->getJitValue(this, f, jit_type_float64, block);
          src2JitVal = src2->getJitValue(this, f, jit_type_float64, block);

          // Set up arguments and make indirect call to compare routine.
          jit_value_t args[2] = {src1JitVal, src2JitVal};

          switch (opc) {
            case PCIT::LE_MBIN32S_MFLT64_MFLT64:
              resJitVal = jit_insn_call_indirect(f,dblLeJitVal,dblSig,args,2,0);
              break;
            case PCIT::GE_MBIN32S_MFLT64_MFLT64:
              resJitVal = jit_insn_call_indirect(f,dblGeJitVal,dblSig,args,2,0);
              break;
            case PCIT::LT_MBIN32S_MFLT64_MFLT64:
              resJitVal = jit_insn_call_indirect(f,dblLtJitVal,dblSig,args,2,0);
              break;
            case PCIT::GT_MBIN32S_MFLT64_MFLT64:
              resJitVal = jit_insn_call_indirect(f,dblGtJitVal,dblSig,args,2,0);
              break;
            case PCIT::EQ_MBIN32S_MFLT64_MFLT64:
              resJitVal = jit_insn_call_indirect(f,dblEqJitVal,dblSig,args,2,0);
              break;
            case PCIT::NE_MBIN32S_MFLT64_MFLT64:
              resJitVal = jit_insn_call_indirect(f,dblNeJitVal,dblSig,args,2,0);
              break;
          }

          inst->getWOps()[0]->setJitValue(this, f, resJitVal, block);

          break;
        }

        case PCIT::COMP_MBIN32S_MBIGS_MBIGS_IBIN32S_IBIN32S:
        {
          jit_label_t loopLabel = jit_label_undefined;
          jit_label_t falseLabel = jit_label_undefined;
          jit_label_t trueLabel = jit_label_undefined;
          jit_label_t endLabel = jit_label_undefined;
          jit_label_t negLabel = jit_label_undefined;
          jit_label_t resolveLabel = jit_label_undefined;
          jit_label_t diffSignsLabel = jit_label_undefined;
          jit_label_t loopDiffLabel = jit_label_undefined;
          jit_label_t emitFalseLabel = jit_label_undefined;
          jit_label_t emitTrueLabel = jit_label_undefined;

          jit_value_t t1, t2, size, cmpRes, cmpRes2, src1Sign, src2Sign;

          // Get operands
          res = inst->getWOps()[0];
          src1 = inst->getROps()[0];
          src2 = inst->getROps()[1];

          CollIndex len = src1->getLen();

          jit_value_t lenJitVal = 
            jit_value_create_nint_constant(f, jit_type_int, len);

          // Declare variables as if bignum will be processed in shorts
          jit_type_t bignumMaxType = jit_type_ushort;
          CollIndex startPos = len - 2;
          jit_value_t mask = jitValMask16;
          jit_value_t decrJitVal = 
            jit_value_create_nint_constant(f, jit_type_int, 2);

          // If bignum length is a multiple of 4, why not do 4 bytes at a time?
          // Alignment traps may occur, but they have been shown to have a
          // minimal impact on x86.

          if ((len % 4) == 0) {
            bignumMaxType = jit_type_uint;
            startPos = len - 4;
            mask = jitValMask32;
            decrJitVal = jit_value_create_nint_constant(f, jit_type_int, 4);
          }

          // Get pointers to the source operands.
          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);

          // Get the short/int containing the sign bits.
          t1 = jit_insn_load_relative(f, src1JitVal, startPos, bignumMaxType);
          t2 = jit_insn_load_relative(f, src2JitVal, startPos, bignumMaxType);

          // Get the sign bits
          src1Sign = jit_insn_and(f, t1, mask);
          src2Sign = jit_insn_and(f, t2, mask);

          // If signs don't match, branch to a fast path
          cmpRes = jit_insn_ne(f, src1Sign, src2Sign);
          jit_insn_branch_if(f, cmpRes, &diffSignsLabel);

          // Prepare for loop
          size = jit_value_create_nint_constant(f, jit_type_int, startPos);

          t1 = jit_insn_add(f, src1JitVal, size);
          t2 = jit_insn_add(f, src2JitVal, size);
          jit_insn_store(f, tSrc1, t1);
          jit_insn_store(f, tSrc2, t2);

          // Loop header: "loop index = length"
          jit_insn_store(f, loopIndex, lenJitVal);

          // Body: "while()"
          jit_insn_label(f, &loopLabel);

          // Get byte in string to compare 
          t1 = jit_insn_load_relative(f, tSrc1, 0, bignumMaxType);
          t2 = jit_insn_load_relative(f, tSrc2, 0, bignumMaxType);

          // Perform comparison(s) and branch.  For eq/ne we just do one
          // comparison.  For all others we do 2 comparisons.  The 2 compares
          // are done together, since libjit otherwise spills and restores the
          // arguments before each compare and branch!

          Int32 subOpc = inst->code[8];
          switch (subOpc) {
            case ITM_EQUAL:
              cmpRes = jit_insn_ne(f, t1, t2);
              // Jump directly to "return false" code
              jit_insn_branch_if(f, cmpRes, &resolveLabel);
              break;
            case ITM_NOT_EQUAL:
              cmpRes = jit_insn_ne(f, t1, t2);
              // Jump directly to "return true" code
              jit_insn_branch_if(f, cmpRes, &resolveLabel);
              break;
            case ITM_LESS:
              cmpRes = jit_insn_gt(f, t1, t2);
              jit_insn_branch_if(f, cmpRes, &falseLabel);
              cmpRes2 = jit_insn_lt(f, t1, t2);
              jit_insn_branch_if(f, cmpRes2, &trueLabel);
              break;
            case ITM_LESS_EQ:
              cmpRes = jit_insn_gt(f, t1, t2);
              jit_insn_branch_if(f, cmpRes, &falseLabel);
              cmpRes2 = jit_insn_lt(f, t1, t2);
              jit_insn_branch_if(f, cmpRes2, &trueLabel);
              break;
            case ITM_GREATER:
              cmpRes = jit_insn_lt(f, t1, t2);
              jit_insn_branch_if(f, cmpRes, &falseLabel);
              cmpRes2 = jit_insn_gt(f, t1, t2);
              jit_insn_branch_if(f, cmpRes2, &trueLabel);
              break;
            case ITM_GREATER_EQ:
              cmpRes = jit_insn_lt(f, t1, t2);
              jit_insn_branch_if(f, cmpRes, &falseLabel);
              cmpRes2 = jit_insn_gt(f, t1, t2);
              jit_insn_branch_if(f, cmpRes2, &trueLabel);
              break;
          }

          // Decrement loop index
          t1 = jit_insn_sub(f, loopIndex, decrJitVal);
          jit_insn_store(f, loopIndex, t1);

          // Increment string pointers
          t1 = jit_insn_sub(f, tSrc1, decrJitVal);
          t2 = jit_insn_sub(f, tSrc2, decrJitVal);
          jit_insn_store(f, tSrc1, t1);
          jit_insn_store(f, tSrc2, t2);

          // Branch back if loop index <> 0
          jit_insn_branch_if(f, loopIndex, &loopLabel);

          // If we reach this point, the two sources are equal to each other.
          if (subOpc == ITM_EQUAL)
          {
            res->storeJitValue(this, f, jit_type_int, oneJitVal_, block);
            jit_insn_branch(f, &endLabel);
          }
          else if (subOpc == ITM_NOT_EQUAL) {
            res->storeJitValue(this, f, jit_type_int, zeroJitVal_, block);
            jit_insn_branch(f, &endLabel);
          }
          else
          {
            // GE or LE will return TRUE right away.
            if ((subOpc == ITM_GREATER_EQ) || (subOpc == ITM_LESS_EQ)) {
              res->storeJitValue(this, f, jit_type_int, oneJitVal_, block);
              jit_insn_branch(f, &endLabel);
            }

            // False block emitted first, since we fall-through to it.
            jit_insn_label(f, &falseLabel);

            // If sources are negative, then return the opposite.
            jit_insn_branch_if(f, src1Sign, &emitTrueLabel);

            jit_insn_label(f, &emitFalseLabel);
            res->storeJitValue(this, f, jit_type_int, zeroJitVal_, block);
            jit_insn_branch(f, &endLabel);

            // True block
            jit_insn_label(f, &trueLabel);

            // If sources are negative, then return the opposite.
            jit_insn_branch_if(f, src1Sign, &emitFalseLabel);

            jit_insn_label(f, &emitTrueLabel);
            res->storeJitValue(this, f, jit_type_int, oneJitVal_, block);
            jit_insn_branch(f, &endLabel);
          }

          // Handle difference in signs here.  Normally a difference in signs
          // is enough to make the comparison, but because we can have both -0
          // and +0 (which are both equivalent), we have to make this code a
          // little lengthy.

          jit_insn_label(f, &diffSignsLabel);

          // Loop header: "loop index = 0"
          jit_insn_store(f, loopIndex, zeroJitVal_);

          // Body: "while()"
          jit_insn_label(f, &loopDiffLabel);

          // Get byte in string to compare 
          t1 = jit_insn_load_elem(f, src1JitVal, loopIndex, bignumMaxType);
          t2 = jit_insn_load_elem(f, src2JitVal, loopIndex, bignumMaxType);

          // Are the two different from each other?
          cmpRes = jit_insn_ne(f, t1, t2);
          jit_insn_branch_if(f, cmpRes, &resolveLabel);

          // Are the two equal to 0?
          cmpRes = jit_insn_ne(f, t1, zeroJitVal_);
          jit_insn_branch_if(f, cmpRes, &resolveLabel);

          // Increment loop index
          t1 = jit_insn_add(f, loopIndex, decrJitVal);
          jit_insn_store(f, loopIndex, t1);

          // Branch back if loop index == length 
          t1 = jit_insn_ne(f, loopIndex, lenJitVal);
          jit_insn_branch_if(f, loopIndex, &loopDiffLabel);

          if ((subOpc == ITM_EQUAL) || (subOpc == ITM_LESS_EQ) ||
              (subOpc == ITM_GREATER_EQ))
            res->storeJitValue(this, f, jit_type_int, oneJitVal_, block);
          else
            res->storeJitValue(this, f, jit_type_int, zeroJitVal_, block);

          jit_insn_branch(f, &endLabel);
          
          // We arrive here if 1) the signs are different, 2) we verified that
          // we don't have a +0/-0 situation.  In this case, EQ & NE can do a
          // fast return.  The other comparisons must check the sign directly
          // before making a decision on what to do.

          jit_insn_label(f, &resolveLabel);

          switch (subOpc) {
            case ITM_EQUAL:
              res->storeJitValue(this, f, jit_type_int, zeroJitVal_, block);
              break;
            case ITM_NOT_EQUAL:
              res->storeJitValue(this, f, jit_type_int, oneJitVal_, block);
              break;

            default:
            {
              jit_insn_branch_if(f, src1Sign, &negLabel);
              if ((subOpc == ITM_GREATER) || (subOpc == ITM_GREATER_EQ))
                res->storeJitValue(this, f, jit_type_int, oneJitVal_, block);
              else
                res->storeJitValue(this, f, jit_type_int, zeroJitVal_, block);

              jit_insn_branch(f, &endLabel);

              // Negative label
              jit_insn_label(f, &negLabel);
              if ((subOpc == ITM_GREATER) || (subOpc == ITM_GREATER_EQ))
                res->storeJitValue(this, f, jit_type_int, zeroJitVal_, block);
              else
                res->storeJitValue(this, f, jit_type_int, oneJitVal_, block);

              break;
            }
          }

          // End label
          jit_insn_label(f, &endLabel);

          break;
        }


        case PCIT::EQ_MBIN32S_MASCII_MASCII:
        case PCIT::NE_MBIN32S_MASCII_MASCII:
        case PCIT::LT_MBIN32S_MASCII_MASCII:
        case PCIT::GT_MBIN32S_MASCII_MASCII:
        case PCIT::LE_MBIN32S_MASCII_MASCII:
        case PCIT::GE_MBIN32S_MASCII_MASCII:
        {
          jit_value_t size, b1, b2, cmpRes, cmpRes2;
          jit_value_t temp1, temp2, temp3;
          jit_label_t loopLabel = jit_label_undefined;
          jit_label_t falseLabel = jit_label_undefined;
          jit_label_t trueLabel = jit_label_undefined;
          jit_label_t endLabel = jit_label_undefined;

          NABoolean isEqOrNe = (opc == PCIT::EQ_MBIN32S_MASCII_MASCII) ||
                                 (opc == PCIT::NE_MBIN32S_MASCII_MASCII);

          NABoolean dumpTrueBlockFirst = TRUE;  // May change later

          res = inst->getWOps()[0];
          src1 = inst->getROps()[0];
          src2 = inst->getROps()[1];

          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);

          jit_insn_store(f, tSrc1, src1JitVal);
          jit_insn_store(f, tSrc2, src2JitVal);

          // Loop header: "loop index = length"
          size = jit_value_create_nint_constant(f, jit_type_int,src1->getLen());
          jit_insn_store(f, loopIndex, size);

          // Body: "while()"
          jit_insn_label(f, &loopLabel);

          // Get byte in string to compare 
          b1 = jit_insn_load_relative(f, tSrc1, 0, jit_type_ubyte);
          b2 = jit_insn_load_relative(f, tSrc2, 0, jit_type_ubyte);

          // Perform comparison(s) and branch.  For eq/ne we just do one
          // comparison.  For all others we do 2 comparisons.  The 2 compares
          // are done together, since libjit otherwise spills and restores the
          // arguments before each compare and branch!

          if (opc == PCIT::EQ_MBIN32S_MASCII_MASCII) {
            cmpRes = jit_insn_ne(f, b1, b2);
            jit_insn_branch_if(f, cmpRes, &falseLabel);
          }
          else if (opc == PCIT::NE_MBIN32S_MASCII_MASCII) {
            cmpRes = jit_insn_ne(f, b1, b2);
            jit_insn_branch_if(f, cmpRes, &trueLabel);
            dumpTrueBlockFirst = FALSE;
          }
          else if (opc == PCIT::LT_MBIN32S_MASCII_MASCII) {
            cmpRes = jit_insn_gt(f, b1, b2);
            cmpRes2 = jit_insn_lt(f, b1, b2);
            jit_insn_branch_if(f, cmpRes, &falseLabel);
            jit_insn_branch_if(f, cmpRes2, &trueLabel);
            dumpTrueBlockFirst = FALSE;
          }
          else if (opc == PCIT::GT_MBIN32S_MASCII_MASCII) {
            cmpRes = jit_insn_lt(f, b1, b2);
            cmpRes2 = jit_insn_gt(f, b1, b2);
            jit_insn_branch_if(f, cmpRes, &falseLabel);
            jit_insn_branch_if(f, cmpRes2, &trueLabel);
            dumpTrueBlockFirst = FALSE;
          }
          else if (opc == PCIT::LE_MBIN32S_MASCII_MASCII) {
            cmpRes = jit_insn_gt(f, b1, b2);
            cmpRes2 = jit_insn_lt(f, b1, b2);
            jit_insn_branch_if(f, cmpRes, &falseLabel);
            jit_insn_branch_if(f, cmpRes2, &trueLabel);
          }
          else /* (opc == PCIT::GE_MBIN32S_MASCII_MASCII) */ {
            cmpRes = jit_insn_lt(f, b1, b2);
            cmpRes2 = jit_insn_gt(f, b1, b2);
            jit_insn_branch_if(f, cmpRes, &falseLabel);
            jit_insn_branch_if(f, cmpRes2, &trueLabel);
          }

          // Decrement loop index
          temp1 = jit_insn_sub(f, loopIndex, oneJitVal_);
          jit_insn_store(f, loopIndex, temp1);

          // Increment string pointers
          temp2 = jit_insn_add(f, tSrc1, oneJitVal_);
          temp3 = jit_insn_add(f, tSrc2, oneJitVal_);
          jit_insn_store(f, tSrc1, temp2);
          jit_insn_store(f, tSrc2, temp3);

          // Branch back if loop index <> 0
          jit_insn_branch_if(f, loopIndex, &loopLabel);

          // Usually comparison is followed by branch.  Attempt to combine
          // branches here, so as avoid overhead cost from logical branch.
          if (!jitProcessPredicate(inst, f, &falseLabel, &trueLabel,
                                   dumpTrueBlockFirst))
          {
            if (dumpTrueBlockFirst) {
              // True block
              jit_insn_label(f, &trueLabel);
              res->storeJitValue(this, f, jit_type_int, oneJitVal_, block);
              jit_insn_branch(f, &endLabel);

              // False block
              jit_insn_label(f, &falseLabel);
              res->storeJitValue(this, f, jit_type_int, zeroJitVal_, block);
            }
            else {
              // False block
              jit_insn_label(f, &falseLabel);
              res->storeJitValue(this, f, jit_type_int, zeroJitVal_, block);
              jit_insn_branch(f, &endLabel);

              // True block
              jit_insn_label(f, &trueLabel);
              res->storeJitValue(this, f, jit_type_int, oneJitVal_, block);
            }

            // End label
            jit_insn_label(f, &endLabel);
          }
          else
            skipInst = 1;

          break; }

        case PCIT::EQ_MBIN32S_MBIN64S_MBIN64S:
        case PCIT::NE_MBIN32S_MBIN64S_MBIN64S:
        {
          jit_value_t temp1, temp2, temp3;

          jit_label_t trueLabel = jit_label_undefined;
          jit_label_t falseLabel = jit_label_undefined;
          jit_label_t doneLabel = jit_label_undefined;

          src1 = inst->getROps()[0];
          src2 = inst->getROps()[1];

          // Need to get pointer to 64-bit vals, since we'll need to perform 2
          // 4-byte comparisons.  This is done because libjit does not support
          // comparison of 64-bit types.  We can work around this here for EQ/NE
          // since a memcmp is sufficient to give an answer.
          src1JitVal = src1->getJitValue(this, f, jit_type_void_ptr, block);
          src2JitVal = src2->getJitValue(this, f, jit_type_void_ptr, block);

          for (i=0; i < 8; i += 4) {
            temp1 = jit_insn_load_relative(f, src1JitVal, i, jit_type_uint);
            temp2 = jit_insn_load_relative(f, src2JitVal, i, jit_type_uint);
            temp3 = jit_insn_ne(f, temp1, temp2);

            if (opc == PCIT::EQ_MBIN32S_MBIN64S_MBIN64S)
              jit_insn_branch_if(f, temp3, &falseLabel);
            else
              jit_insn_branch_if(f, temp3, &trueLabel);
          }

          // Fall-through implies that the two operands are equal.  Generate
          // the appropriate code, given opc.
          if (opc == PCIT::EQ_MBIN32S_MBIN64S_MBIN64S)
          {
            inst->getWOps()[0]->setJitValue(this, f, oneJitVal_, block);
            jit_insn_branch(f, &doneLabel);
            jit_insn_label(f, &falseLabel);
            inst->getWOps()[0]->setJitValue(this, f, zeroJitVal_, block);
          }
          else
          {
            inst->getWOps()[0]->setJitValue(this, f, zeroJitVal_, block);
            jit_insn_branch(f, &doneLabel);
            jit_insn_label(f, &trueLabel);
            inst->getWOps()[0]->setJitValue(this, f, oneJitVal_, block);
          }

          jit_insn_label(f, &doneLabel);

          break;
        } 

        case PCIT::EQ_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::EQ_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::EQ_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::EQ_MBIN32S_MBIN16U_MBIN16U:
        case PCIT::EQ_MBIN32S_MBIN16U_MBIN32U:
        case PCIT::EQ_MBIN32S_MBIN32U_MBIN32U:
        case PCIT::EQ_MBIN32S_MBIN16U_MBIN16S:
        {
          src1 = inst->getROps()[0];
          src2 = inst->getROps()[1];

          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);

          resJitVal = jit_insn_eq(f, src1JitVal, src2JitVal);

          inst->getWOps()[0]->setJitValue(this, f, resJitVal, block);

          break;
        }

        case PCIT::LT_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::LT_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::LT_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::LT_MBIN32S_MBIN16U_MBIN16U:
        case PCIT::LT_MBIN32S_MBIN16U_MBIN32U:
        case PCIT::LT_MBIN32S_MBIN32U_MBIN32U:
        case PCIT::LT_MBIN32S_MBIN16U_MBIN16S:
        case PCIT::LT_MBIN32S_MBIN64S_MBIN64S:
        {
          src1 = inst->getROps()[0];
          src2 = inst->getROps()[1];

          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);

          resJitVal = jit_insn_lt(f, src1JitVal, src2JitVal);

          inst->getWOps()[0]->setJitValue(this, f, resJitVal, block);

          break;
        }

        case PCIT::LE_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::LE_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::LE_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::LE_MBIN32S_MBIN16U_MBIN16U:
        case PCIT::LE_MBIN32S_MBIN16U_MBIN32U:
        case PCIT::LE_MBIN32S_MBIN32U_MBIN32U:
        case PCIT::LE_MBIN32S_MBIN16U_MBIN16S:
        case PCIT::LE_MBIN32S_MBIN64S_MBIN64S:
        {
          src1 = inst->getROps()[0];
          src2 = inst->getROps()[1];

          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);

          resJitVal = jit_insn_le(f, src1JitVal, src2JitVal);

          inst->getWOps()[0]->setJitValue(this, f, resJitVal, block);

          break;
        }



        case PCIT::GT_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::GT_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::GT_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::GT_MBIN32S_MBIN16U_MBIN16U:
        case PCIT::GT_MBIN32S_MBIN16U_MBIN32U:
        case PCIT::GT_MBIN32S_MBIN32U_MBIN32U:
        case PCIT::GT_MBIN32S_MBIN16U_MBIN16S:
        case PCIT::GT_MBIN32S_MBIN64S_MBIN64S:
        {
          src1 = inst->getROps()[0];
          src2 = inst->getROps()[1];

          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);

          resJitVal = jit_insn_gt(f, src1JitVal, src2JitVal);

          inst->getWOps()[0]->setJitValue(this, f, resJitVal, block);

          break;
        }

        case PCIT::GE_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::GE_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::GE_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::GE_MBIN32S_MBIN16U_MBIN16U:
        case PCIT::GE_MBIN32S_MBIN16U_MBIN32U:
        case PCIT::GE_MBIN32S_MBIN32U_MBIN32U:
        case PCIT::GE_MBIN32S_MBIN16U_MBIN16S:
        case PCIT::GE_MBIN32S_MBIN64S_MBIN64S:
        {
          src1 = inst->getROps()[0];
          src2 = inst->getROps()[1];

          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);

          resJitVal = jit_insn_ge(f, src1JitVal, src2JitVal);

          inst->getWOps()[0]->setJitValue(this, f, resJitVal, block);

          break;
        }

        case PCIT::NE_MBIN32S_MBIN16S_MBIN16S:
        {
          src1 = inst->getROps()[0];
          src2 = inst->getROps()[1];

          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);

          resJitVal = jit_insn_ne(f, src1JitVal, src2JitVal);

          inst->getWOps()[0]->setJitValue(this, f, resJitVal, block);

          break;
        }

        case PCIT::EQ_MBIN32S_MBIN16S_MBIN32U:
        case PCIT::LT_MBIN32S_MBIN16S_MBIN32U:
        case PCIT::GT_MBIN32S_MBIN16S_MBIN32U:
        case PCIT::LE_MBIN32S_MBIN16S_MBIN32U:
        case PCIT::GE_MBIN32S_MBIN16S_MBIN32U:
        {
          src1 = inst->getROps()[0];
          src2 = inst->getROps()[1];

          assert (src1->isConst());

          // Use the jit type for src2 in both cases, since we checked
          // before-hand that the type for both are MBIN32U
          src1JitVal = src1->getJitValue(this, f, src2->getJitType(), block);
          src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);

          switch (opc) {
            case PCIT::EQ_MBIN32S_MBIN16S_MBIN32U:
              resJitVal = jit_insn_eq(f, src1JitVal, src2JitVal);
              break;
            case PCIT::LT_MBIN32S_MBIN16S_MBIN32U:
              resJitVal = jit_insn_lt(f, src1JitVal, src2JitVal);
              break;
            case PCIT::GT_MBIN32S_MBIN16S_MBIN32U:
              resJitVal = jit_insn_gt(f, src1JitVal, src2JitVal);
              break;
            case PCIT::LE_MBIN32S_MBIN16S_MBIN32U:
              resJitVal = jit_insn_le(f, src1JitVal, src2JitVal);
              break;
            case PCIT::GE_MBIN32S_MBIN16S_MBIN32U:
              resJitVal = jit_insn_ge(f, src1JitVal, src2JitVal);
              break;
          }

          inst->getWOps()[0]->setJitValue(this, f, resJitVal, block);

          break;
        }

        case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MFLT64_MFLT64:
        case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN32S_MBIN32S:
        case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN64S:
        case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN32S:
        {
          jit_label_t sumLabel = jit_label_undefined;
          jit_label_t doneLabel = jit_label_undefined;

          // First check if src is null - if so, we just branch out and return.
          src2 = inst->getROps()[1];
          if (!src2->isConst()) {
            src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);
            jit_insn_branch_if(f, src2JitVal, &doneLabel);
          }
          else {
            // Skip check, since constant will be 0 (assert here to verify).
          }


          // Let's get the operands now.
          res = inst->getWOps()[(inst->getWOps()).entries()-1];
          src2 = inst->getROps()[(inst->getROps()).entries()-1];

          resJitVal = res->getJitValue(this, f, res->getJitType(), block);
          src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);


#if 0
          // Now check if target is null.  If so, we just move the src into tgt.
          src1 = inst->getROps()[0];
          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          jit_insn_branch_if_not(f, src1JitVal, &sumLabel);


          // Clear null value of target and clear out it's value.
          res->storeJitValue(this, f, res->getJitType(), src2JitVal, block);
          src1->storeNullJitValueAndBranch(this, f, zeroJitVal_, &doneLabel,
                                           FALSE, block);


          // Perform the sum here
          jit_insn_label(f, &sumLabel);

          resJitVal = jit_insn_add(f, resJitVal, src2JitVal);
          res->setJitValue(this, f, resJitVal, block);
#else
          // In an effort to work around a libjit register allocation bug where
          // the register for the source is overwritten before being added to
          // the aggregate, we perform the add first (since that's the likely
          // scenario) and then do the check for null of the aggregate.

          jit_value_t addJitVal = jit_insn_add(f, resJitVal, src2JitVal);

          // Now check if target is null.  If so, we just move the src into tgt.
          src1 = inst->getROps()[0];
          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          jit_insn_branch_if_not(f, src1JitVal, &sumLabel);


          // Clear null value of target and clear out it's value.
          res->storeJitValue(this, f, res->getJitType(), src2JitVal, block);
          src1->storeNullJitValueAndBranch(this, f, zeroJitVal_, &doneLabel,
                                           FALSE, block);

          // Store the sum here.
          jit_insn_label(f, &sumLabel);
          res->setJitValue(this, f, addJitVal, block);
#endif

          jit_insn_label(f, &doneLabel);

          // Lastly, clear out jit values for 32-bit source operand if needed.
          if (opc == PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN32S)
            src2->clearJitValues(this);

          break;
        }


        case PCIT::SUM_MFLT64_MFLT64:
        case PCIT::SUM_MBIN32S_MBIN32S:
        case PCIT::SUM_MBIN64S_MBIN64S:
        case PCIT::SUM_MBIN64S_MBIN32S:
        {
          res = inst->getWOps()[0];
          src1 = res; /* same as write operand */
          src2 = inst->getROps()[1];

          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);

          resJitVal = jit_insn_add(f, src1JitVal, src2JitVal);
          res->setJitValue(this, f, resJitVal, block);

          // Lastly, clear out jit values for 32-bit source operand if needed.
          if (opc == PCIT::SUM_MBIN64S_MBIN32S)
            src2->clearJitValues(this);

          break;
        }

        case PCIT::ADD_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::ADD_MBIN16S_MBIN16S_MBIN16S:
        case PCIT::ADD_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::ADD_MBIN64S_MBIN64S_MBIN64S:
        case PCIT::ADD_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::ADD_MBIN64S_MBIN32S_MBIN64S:
        case PCIT::ADD_MFLT64_MFLT64_MFLT64:
        {
          res = inst->getWOps()[0];
          src1 = inst->getROps()[0];
          src2 = inst->getROps()[1];

          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);

          resJitVal = jit_insn_add(f, src1JitVal, src2JitVal);
          res->setJitValue(this, f, resJitVal, block);

          if (opc == PCIT::ADD_MFLT64_MFLT64_MFLT64)
            jitProcessFloatExceptionCheck(f, res, &errorJitLabel, block);

          // Lastly, clear out jit values for 32-bit source operand if needed.
          if (opc == PCIT::ADD_MBIN64S_MBIN32S_MBIN64S)
            src1->clearJitValues(this);

          break;
        }

        /*case PCIT::SUB_MBIN32S_MBIN16S_MBIN16S:*/
        case PCIT::SUB_MBIN16S_MBIN16S_MBIN16S:
        case PCIT::SUB_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::SUB_MBIN64S_MBIN64S_MBIN64S:
        case PCIT::SUB_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::SUB_MBIN32S_MBIN32S_MBIN16S:
        case PCIT::SUB_MFLT64_MFLT64_MFLT64:
        {
          res = inst->getWOps()[0];
          src1 = inst->getROps()[0];
          src2 = inst->getROps()[1];

          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);

          resJitVal = jit_insn_sub(f, src1JitVal, src2JitVal);
          res->setJitValue(this, f, resJitVal, block);

          if (opc == PCIT::SUB_MFLT64_MFLT64_MFLT64)
            jitProcessFloatExceptionCheck(f, res, &errorJitLabel, block);

          // No opt support for this instruction
          if (opc == PCIT::SUB_MBIN64S_MBIN64S_MBIN64S)
            lowerOptLevel = TRUE;


          break;
        }

        case PCIT::MUL_MBIN32S_MBIN32S_MBIN32S:
        case PCIT::MUL_MBIN32S_MBIN16S_MBIN16S:
        case PCIT::MUL_MBIN32S_MBIN16S_MBIN32S:
        case PCIT::MUL_MFLT64_MFLT64_MFLT64:
        {
          res = inst->getWOps()[0];
          src1 = inst->getROps()[0];
          src2 = inst->getROps()[1];

          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);

          resJitVal = jit_insn_mul(f, src1JitVal, src2JitVal);
          res->setJitValue(this, f, resJitVal, block);

          if (opc == PCIT::MUL_MFLT64_MFLT64_MFLT64)
            jitProcessFloatExceptionCheck(f, res, &errorJitLabel, block);

          break;
        }

        case PCIT::DIV_MFLT64_MFLT64_MFLT64:
        {
          res = inst->getWOps()[0];
          src1 = inst->getROps()[0];
          src2 = inst->getROps()[1];

          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);

          resJitVal = jit_insn_div(f, src1JitVal, src2JitVal);
          res->setJitValue(this, f, resJitVal, block);

          if (opc == PCIT::DIV_MFLT64_MFLT64_MFLT64)
            jitProcessFloatExceptionCheck(f, res, &errorJitLabel, block);

          break;
        }

        case PCIT::MOVE_MBIN32S:
        {
          // RETURN instruction must follow
          assert (inst->next && (inst->next->code[0] == PCIT::RETURN));

          jit_label_t l1 = jit_label_undefined;
          jit_label_t l2 = jit_label_undefined;

          src1 = inst->getROps()[0];
          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);

          jit_value_t t1 = jit_insn_eq(f, src1JitVal, oneJitVal_);
          jit_insn_branch_if(f, t1, &l1);

          jit_insn_label(f, &l2);
          jit_insn_return(f, fJitVal_);

          jit_insn_label(f, &l1);
          jit_insn_return(f, tJitVal_);

          break;
        }

        case PCIT::RETURN_IBIN32S:
        {
          if (inst->code[1] == 1)
            jit_insn_return(f, tJitVal_);
          else
            jit_insn_return(f, fJitVal_);
          break;
        }

        case PCIT::RETURN:
        {
          jit_insn_return(f, okJitVal_);
          break;
        }

        // FIX: Bulk instructions can sometimes redefine the operand (because
        // two moves can be combined into one).  So when storeJitValue or
        // getJitValue is called, the original operand is retrieved, and it's
        // length is used.  Perhaps that should be fixed from the get-go.
        case PCIT::MOVE_BULK:
        {
          CollIndex j, rOps=0, wOps=0;
          Int32 length = inst->code[1];

          // Reload operands to get rid of potential overlapping ones.
          inst->reloadOperands(this);

          for (j=3; j != (length-1); j+=3)
          {
            Int32 opc = inst->code[j+1];

            res = inst->getWOps()[wOps++];

            switch(opc) {
              case PCIT::MOVE_MBIN8_MBIN8_IBIN32S:
                // This instruction has one more entry (length) in MOVE_BULK.
                // Advance and then fall-through.
                j++;

              case PCIT::MOVE_MBIN64S_MBIN64S:
              case PCIT::MOVE_MBIN32U_MBIN32U:
              case PCIT::MOVE_MBIN16U_MBIN16U:
              case PCIT::MOVE_MBIN8_MBIN8:
              {
                src1 = inst->getROps()[rOps++];

                src1JitVal =
                  src1->getJitValue(this, f, src1->getJitType(), block);
                res->storeJitValue(this,f,res->getJitType(),src1JitVal,block);

                break;
              }

              case PCIT::MOVE_MBIN16U_IBIN16U:
              case PCIT::MOVE_MBIN32S_IBIN32S:
              {
                type = (opc == PCIT::MOVE_MBIN32S_IBIN32S)
                         ? jit_type_int : jit_type_ushort;

                Int32 val = inst->code[j+3];

                src1JitVal = jit_value_create_nint_constant(f, type, val);
                res->storeJitValue(this, f, type, src1JitVal, block);

                break;
              }

              default:
                assert(FALSE);
                break;
            }
          }

          break;
        }

        case PCIT::MOVE_MFLT64_MBIN16S:
        case PCIT::MOVE_MFLT64_MBIN32S:
        case PCIT::MOVE_MBIN16U_MBIN8:
        case PCIT::MOVE_MBIN32U_MBIN16U:
        case PCIT::MOVE_MBIN32S_MBIN16U:
        case PCIT::MOVE_MBIN64S_MBIN16U:
        case PCIT::MOVE_MBIN32U_MBIN16S:
        case PCIT::MOVE_MBIN32S_MBIN16S:
        case PCIT::MOVE_MBIN64S_MBIN16S:
        case PCIT::MOVE_MBIN64S_MBIN32U:
        case PCIT::MOVE_MBIN64S_MBIN32S:
        {
          res = inst->getWOps()[0];
          src1 = inst->getROps()[0];

          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src1JitVal = jit_insn_convert(f, src1JitVal, res->getJitType(), 0);
          res->storeJitValue(this, f, res->getJitType(), src1JitVal, block);

          // Lastly, clear out jit values for 32-bit source operand if needed.
          if ((opc == PCIT::MOVE_MBIN64S_MBIN16U) ||
              (opc == PCIT::MOVE_MBIN64S_MBIN16S) ||
              (opc == PCIT::MOVE_MBIN64S_MBIN32U) ||
              (opc == PCIT::MOVE_MBIN64S_MBIN32S))
            src1->clearJitValues(this);

          break;
        }

        case PCIT::MOVE_MBIN16U_IBIN16U:
        case PCIT::MOVE_MBIN32S_IBIN32S:
        {
          res = inst->getWOps()[0];
          type = (opc == PCIT::MOVE_MBIN32S_IBIN32S)
                   ? jit_type_int : jit_type_ushort;

          Int32 val = inst->code[3];

          src1JitVal = jit_value_create_nint_constant(f, type, val);
          res->storeJitValue(this, f, type, src1JitVal, block);

          break;  
        }

        case PCIT::MOVE_MBIN16U_MBIN16U:
        case PCIT::MOVE_MBIN8_MBIN8:
        case PCIT::MOVE_MBIN32U_MBIN32U:
        case PCIT::MOVE_MBIN64S_MBIN64S:
        case PCIT::MOVE_MBIN8_MBIN8_IBIN32S:
        {
          src1 = inst->getROps()[0];
          res = inst->getWOps()[0];

          switch(src1->getLen()) {
            case 1:
              type = jit_type_ubyte;
              break;
            case 2:
              type = jit_type_ushort;
              break;
            case 4:
              type = jit_type_uint;
              break;
            case 8:
              type = jit_type_long;
              break;
            default:
            {
              assert (opc == PCIT::MOVE_MBIN8_MBIN8_IBIN32S);
              type = jit_type_void_ptr;
              break;
            }
          }

          src1JitVal = src1->getJitValue(this, f, type, block);
          res->storeJitValue(this, f, type, src1JitVal, block);

          break;
        }

        case PCIT::REPLACE_NULL_MATTR3_MBIN32S:
        case PCIT::REPLACE_NULL_MATTR3_MBIN32U:
        case PCIT::REPLACE_NULL_MATTR3_MBIN16S:
        case PCIT::REPLACE_NULL_MATTR3_MBIN16U:
        {
          jit_label_t doneLabel = jit_label_undefined;
          jit_label_t nullLabel = jit_label_undefined;

          type = (inst->code[10] == 2) ? jit_type_short : jit_type_int;

          res = inst->getWOps()[0];
          src1 = inst->getROps()[0];

          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);

          // Branch if NULL
          jit_insn_branch_if(f, src1JitVal, &nullLabel);

          // not null case
          src2JitVal = inst->getROps()[1]->getJitValue(this, f, type, block);
          res->storeJitValue(this, f, type, src2JitVal, block);
          jit_insn_branch(f, &doneLabel);

          // not null case
          jit_insn_label(f, &nullLabel);
          src2JitVal = inst->getROps()[2]->getJitValue(this, f, type, block);
          res->storeJitValue(this, f, type, src2JitVal, block);

          jit_insn_label(f, &doneLabel);

          break;
        }

        case PCIT::NULL_TEST_MBIN32S_MATTR5_IBIN32S_IBIN32S:
        {
          src1 = inst->getROps()[0];
          res = inst->getWOps()[0];

          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);

          NABoolean testForNull = (inst->code[9] != 0);

          // Generate more efficient code if next instruction is logical branch
          if (inst->next && inst->next->isAnyLogicalBranch())
          {
            NABoolean srcLive, tgtLive;

            PCodeInst* branch = inst->next;

            PCodeOperand* bSrc = branch->getROps()[0];
            PCodeOperand* bTgt = branch->getWOps()[0];

            // Is source or target live after branch?
            srcLive = bSrc->isVar() || block->isOperandLiveInSuccs(bSrc);
            tgtLive = bTgt->isVar() || block->isOperandLiveInSuccs(bTgt);

            // Only generate efficient path if the null-test is associated with
            // the branch, and that both the src and tgt of the branch are not
            // live (which would otherwise require one or more stores).
            if ((bSrc->getBvIndex()==res->getBvIndex()) && !srcLive && !tgtLive)
            {
              jitLabel = block->getTargetBlock()->getJitLabel();

              // Branch to destination according to the test performed.
              switch (branch->getOpcode()) {
                case PCIT::BRANCH_OR:
                case PCIT::BRANCH_OR_CNT:
                  if (testForNull)
                    jit_insn_branch_if(f, src1JitVal, jitLabel);
                  else
                    jit_insn_branch_if_not(f, src1JitVal, jitLabel);
                  break;

                case PCIT::BRANCH_AND:
                case PCIT::BRANCH_AND_CNT:
                  if (testForNull)
                    jit_insn_branch_if_not(f, src1JitVal, jitLabel);
                  else
                    jit_insn_branch_if(f, src1JitVal, jitLabel);
                  break;
              }

              // Skip the next instruction (branch) since it was handled here.
              skipInst = 1;
              break;
            }
          }
          else if (inst->next && inst->next->getOpcode() == PCIT::MOVE_MBIN32S)
          {
            PCodeInst* move = inst->next;
            if (res->getBvIndex() == move->getROps()[0]->getBvIndex())
            {
              jit_label_t l1 = jit_label_undefined;

              // A RETURN is guaranteed to following a MOVE_MBIN32S
              assert (move->next && (move->next->getOpcode() == PCIT::RETURN));

              if (testForNull)
                jit_insn_branch_if(f, src1JitVal, &l1);
              else
                jit_insn_branch_if_not(f, src1JitVal, &l1);

              jit_insn_return(f, fJitVal_);

              jit_insn_label(f, &l1);
              jit_insn_return(f, tJitVal_);

              // Skip the next 2 instructions since it was handled here.
              skipInst = 2;
              break;
            }
          }
          
          // Fall-through (default) case to implement NULL_TEST

          if (testForNull)
            resJitVal = jit_insn_ne(f, src1JitVal, zeroJitVal_);
          else
            resJitVal = jit_insn_eq(f, src1JitVal, zeroJitVal_);

          inst->getWOps()[0]->setJitValue(this, f, resJitVal, block);

          break;
        }

        case PCIT::STRLEN_MBIN32U_MATTR5:
        {
          jit_type_t vcType;
          jit_value_t vcLenJitVal;

          src1 = inst->getROps()[0]; 
          res = inst->getWOps()[0];

          if (src1->getVcIndicatorLen() == 2) {
            vcType = jit_type_ushort;
            vcLenJitVal = jit_value_create_nint_constant(f, jit_type_int, 2);
          }
          else {
            vcType = jit_type_uint;
            vcLenJitVal = jit_value_create_nint_constant(f, jit_type_int, 4);
          }

          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          resJitVal = jit_insn_load_relative(f, src1JitVal, 0, vcType);

          // If length is 2 bytes, then convert it to 4 bytes before storage
          if (src1->getVcIndicatorLen() == 2)
            resJitVal = jit_insn_convert(f, resJitVal, jit_type_int, 0);

          res->storeJitValue(this, f, jit_type_int, resJitVal, block);

          break;
        }

        case PCIT::POS_MBIN32S_MATTR5_MATTR5:
        {
          break;
        }

        case PCIT::AND_MBIN32S_MBIN32S_MBIN32S:
        {
          jit_label_t ret0Label = jit_label_undefined;
          jit_label_t endLabel = jit_label_undefined;

          res = inst->getWOps()[0];

          src1 = inst->getROps()[0];
          src2 = inst->getROps()[1];

          src1JitVal = src1->getJitValue(this, f, jit_type_int, block);
          src2JitVal = src2->getJitValue(this, f, jit_type_int, block);

          resJitVal = jit_insn_and(f, src1JitVal, src2JitVal);
          jit_insn_branch_if_not(f, resJitVal, &ret0Label);

          resJitVal = jit_insn_or(f, src1JitVal, src2JitVal);
          res->storeJitValue(this, f, jit_type_int, resJitVal, block);
          jit_insn_branch(f, &endLabel);

          jit_insn_label(f, &ret0Label);
          res->storeJitValue(this, f, jit_type_int, zeroJitVal_, block);

          jit_insn_label(f, &endLabel);

          break;
        }

        case PCIT::OR_MBIN32S_MBIN32S_MBIN32S:
        {
          jit_value_t t1, t2;

          jit_label_t ret1Label = jit_label_undefined;
          jit_label_t endLabel = jit_label_undefined;

          res = inst->getWOps()[0];

          src1 = inst->getROps()[0];
          src2 = inst->getROps()[1];

          src1JitVal = src1->getJitValue(this, f, jit_type_int, block);
          t1 = jit_insn_eq(f, src1JitVal, oneJitVal_);

          src2JitVal = src2->getJitValue(this, f, jit_type_int, block);
          t2 = jit_insn_eq(f, src2JitVal, oneJitVal_);

          resJitVal = jit_insn_or(f, t1, t2);
          jit_insn_branch_if(f, resJitVal, &ret1Label);

          resJitVal = jit_insn_or(f, src1JitVal, src2JitVal);
          res->storeJitValue(this, f, jit_type_int, resJitVal, block);
          jit_insn_branch(f, &endLabel);

          jit_insn_label(f, &ret1Label);
          res->storeJitValue(this, f, jit_type_int, oneJitVal_, block);

          jit_insn_label(f, &endLabel);

          break;
        }

        case PCIT::MINMAX_MBIN8_MBIN8_MBIN32S_IBIN32S:
        {
          jit_type_t type;
          jit_label_t doneLabel = jit_label_undefined;

          src1 = inst->getROps()[0]; // Source pointer
          src2 = inst->getROps()[1]; // Result of comparison

          res = inst->getWOps()[0];

          src2JitVal = src2->getJitValue(this, f, jit_type_int, block);

          jit_insn_branch_if_not(f, src2JitVal, &doneLabel);

          // Get type associated with length, since generic MBINS is used to
          // represent source/tgt.  If not, getJitType() is called, and it will
          // assume we're dealing with a void ptr by default.

          switch(src1->getLen()) {
            case 1:
              type = jit_type_ubyte;
              break;
            case 2:
              type = jit_type_ushort;
              break;
            case 4:
              type = jit_type_uint;
              break;
            case 8:
              type = jit_type_long;
              break;
            default:
            {
              type = jit_type_void_ptr;
              break;
            }
          }

          src1JitVal = src1->getJitValue(this, f, type, block);
          res->setJitValue(this, f, src1JitVal, block);

          jit_insn_label(f, &doneLabel);

          break;
        }

        case PCIT::SWITCH_MBIN32S_MBIN64S_MPTR32_IBIN32S_IBIN32S:
        {
          jit_value_t t1, t2, t3, t4, t5, t6, t7, tableEnd;

          jit_label_t startLoop = jit_label_undefined;
          jit_label_t trueLabel = jit_label_undefined;
          jit_label_t falseLabel = jit_label_undefined;

          jit_value_t hashTableJitVal;

          jit_value_t invalidInt32JitVal =
            jit_value_create_nint_constant(f, jit_type_uint, INVALID_INT32);

          jit_value_t hashTableSizeJitVal =
            jit_value_create_nint_constant(f, jit_type_uint, inst->code[7]);

          // 1. Get the 64-bit value and hash it.
          // 2. Take result and mod it by size of hash table.
          // 3. Start loop
          // 4.  Get element and compare to INVALID_INT64 (maybe just INT32?)
          // 5.    If EQUAL - compare value with source
          // 6.      If EQUAL - store 1
          // 7.      ELSE: Increment index by 1 and compare with size
          // 8.        If beyond table size, store 0
          // 9.        ELSE: Jump back to loop header
          // 9.    ELSE: store 0 

          src1 = inst->getROps()[0];
          src2 = inst->getROps()[1];
          res = inst->getWOps()[0];

          // Step 1
          src1JitVal = src1->getJitValue(this, f, jit_type_void_ptr, block);
          resJitVal = genHash(f, src1JitVal, NULL, loopIndex, 8);

          // Step 2
          resJitVal = jit_insn_rem(f, resJitVal, hashTableSizeJitVal);

          // Step 3-4
          hashTableJitVal=src2->getJitValue(this, f, src2->getJitType(), block);
          resJitVal = jit_insn_load_elem_address(f, hashTableJitVal, resJitVal,
                                                 jit_type_ulong);

          // End of table is basically "size" * "sizeof(Int64)".
          tableEnd = jit_insn_add_relative(f, hashTableJitVal, 8*inst->code[7]);

          // Load the 8 bytes associated with the source before the loop
          t6 = jit_insn_load_relative(f, src1JitVal, 0, jit_type_uint);
          t7 = jit_insn_load_relative(f, src1JitVal, 4, jit_type_uint);

          jit_insn_label(f, &startLoop);

          t1 = jit_insn_load_relative(f, resJitVal, 0, jit_type_uint);
          t2 = jit_insn_load_relative(f, resJitVal, 4, jit_type_uint);
          t3 = jit_insn_eq(f, t1, invalidInt32JitVal);
          t4 = jit_insn_eq(f, t2, invalidInt32JitVal);
          t5 = jit_insn_and(f, t3, t4);

          jit_insn_branch_if(f, t5, &falseLabel);

          // Step 5-6

          t3 = jit_insn_eq(f, t1, t6);
          t4 = jit_insn_eq(f, t2, t7);
          t5 = jit_insn_and(f, t3, t4);
          jit_insn_branch_if(f, t5, &trueLabel);

          // Step 7
          t6 = jit_insn_add_relative(f, resJitVal, 8);
          jit_insn_store(f, resJitVal, t6);

          t4 = jit_insn_eq(f, t6, tableEnd);
          jit_insn_branch_if_not(f, t4, &startLoop);

          if (!jitProcessPredicate(inst, f, &falseLabel, &trueLabel, FALSE)) {
            assert(FALSE);
          } else
            skipInst = 1;

          // Libjit bad -O2 code gen when remainder generated with constants.
          lowerOptLevel = TRUE;

          break;
        }


        case PCIT::SWITCH_MBIN32S_MATTR5_MPTR32_IBIN32S_IBIN32S:
        {
          jit_value_t t1, t2, t3, t4, t5, tableEnd;
          jit_value_t len = NULL;

          jit_label_t startLoop = jit_label_undefined;
          jit_label_t trueLabel = jit_label_undefined;
          jit_label_t falseLabel = jit_label_undefined;
          jit_label_t tryAgainLabel = jit_label_undefined;

          jit_value_t hashTableJitVal;

          jit_value_t invalidInt32JitVal =
            jit_value_create_nint_constant(f, jit_type_uint, INVALID_INT32);

          jit_value_t hashTableSizeJitVal =
            jit_value_create_nint_constant(f, jit_type_uint, inst->code[10]);

          // 1. Get the string value (minus any padding) and hash it.
          // 2. Take result and mod it by size of hash table.
          // 3. Start loop
          // 4.  Get element and compare to INVALID_INT32
          // 5.    If EQUAL - compare string with source
          // 6.      If EQUAL - store 1
          // 7.      ELSE: Increment index by 1 and compare with size
          // 8.        If beyond table size, store 0
          // 9.        ELSE: Jump back to loop header
          // 9.    ELSE: store 0

          src1 = inst->getROps()[0];
          src2 = inst->getROps()[1];
          res = inst->getWOps()[0];

          // Step 1
          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          len = genStringSetup(f, &src1JitVal, src1, TRUE);

          jit_insn_store(f, tSrc1, src1JitVal);
          resJitVal = genHash(f, tSrc1, len, loopIndex, src1->getLen());

          // Step 2
          resJitVal = jit_insn_rem(f, resJitVal, hashTableSizeJitVal);

          // Step 3-4
          hashTableJitVal=src2->getJitValue(this, f, src2->getJitType(), block);
          resJitVal = jit_insn_load_elem_address(f, hashTableJitVal, resJitVal,
                                                 jit_type_ulong);

          // End of table is basically "size" * "sizeof(Int64)".
          tableEnd = jit_insn_add_relative(f, hashTableJitVal,8*inst->code[10]);

          jit_insn_label(f, &startLoop);

          t1 = jit_insn_load_relative(f, resJitVal, 0, jit_type_uint);
          t3 = jit_insn_eq(f, t1, invalidInt32JitVal);
          jit_insn_branch_if(f, t3, &falseLabel);

          // Step 5-6
          t4 = jit_insn_ne(f, t1, len);
          jit_insn_branch_if(f, t4, &tryAgainLabel);

          // Set up arguments and make indirect call to str_cmp routine.
          t2 = jit_insn_load_relative(f, resJitVal, 4, jit_type_uint);
          src2JitVal =
            jit_insn_load_elem_address(f, jitParams_[1], t2, jit_type_ubyte);

          jit_value_t args[3] = {src1JitVal, src2JitVal, len};
          t1 = jit_insn_call_indirect(f, strcmpJitVal,
                                      strcmpSig, args, 3, 0);

          jit_insn_branch_if_not(f, t1, &trueLabel);

          // Step 7
          jit_insn_label(f, &tryAgainLabel);

          t5 = jit_insn_add_relative(f, resJitVal, 8);
          jit_insn_store(f, resJitVal, t5);

          t4 = jit_insn_eq(f, t5, tableEnd);
          jit_insn_branch_if_not(f, t4, &startLoop);

          if (!jitProcessPredicate(inst, f, &falseLabel, &trueLabel, FALSE)) {
            assert(FALSE);
          } else {
            //FIXME: we may skip more than 1 inst if next inst is a MOVE_MBIN32S
            skipInst = 1;
          }

          // Libjit bad -O2 code gen when remainder generated with constants.
          lowerOptLevel = TRUE;

          break;
        }

        case PCIT::HASHCOMB_BULK_MBIN32U:
        {
          jit_value_t t3, t4, t5, oldResJitVal = NULL;

          res = inst->getWOps()[0];

          // The list of source operands come in potentially rearranged, so that
          // the parallel hash algorithm could run through this faster.  But
          // with native exprs, we do not call hashP, and so we need to hash
          // the source operands in order.  Re-re-arrange them :).

          OPLIST origList;                  // List of operands in order.

          Int32 min = 0;                    // first position to start off is 0.
          CollIndex length = inst->code[1]; // Total length of instruction.

          // Walk through instruction (skipping header stuff) and identify the
          // source operand having the "min" position.  Once found, insert it
          // into the origList, and then restart check with "min" set to the
          // next position.

          for (j=4; j < length;) {
            if (inst->code[j+3] == min) {
              // Add the appropriate source to the list
              origList.insert(inst->getROps()[(j >> 2) - 1]);

              // Get the next min position, and restart by setting j to 4;
              min++;
              j = 4;
            }
            else {
              // Move to the next source.
              j += 4;
            }
          }

          assert (inst->getROps().entries() == origList.entries());

          for (j=0; j < origList.entries(); j++) {
            src1 = origList[j];
            Int32 len = src1->getLen();

            // If the source length is 0, then the source represents the result
            // of a previously computed hash instruction.  Otherwise, get the
            // source as a pointer and call genHash to get the result.
            if (len > 0) {
              src1JitVal = src1->getJitValue(this, f, jit_type_void_ptr, block);
              resJitVal = genHash(f, src1JitVal, NULL, loopIndex, len);
            }
            else
              resJitVal = src1->getJitValue(this, f, jit_type_uint, block);
  
            // Combine previous hash results if this is not the 1st run through.
            if (j > 0) {
              //
              // Hashcomb is performed similarly to how Hash is performed - see
              // PCodeCfg::genHash()
              //
              // Compute: t3 = oldResJitVal << 1
              //          t4 = oldResJitVal >> 31
              //          t5 = (t3 | t4)
              //          resJitVal = t5 ^ resJitVal
              //

              t3 = jit_insn_shl(f, oldResJitVal, oneJitVal_);
              t4 = jit_insn_ushr(f, oldResJitVal, shiftRVal);
              t5 = jit_insn_or(f, t3, t4);

              resJitVal = jit_insn_xor(f, t5, resJitVal);
            }

            oldResJitVal = resJitVal;
          }

          res->storeJitValue(this, f, jit_type_uint, resJitVal, block);

          break;
        }

        case PCIT::HASH_MBIN32U_MATTR5:
        case PCIT::HASH_MBIN32U_MPTR32_IBIN32S_IBIN32S:
        case PCIT::HASH_MBIN32U_MBIN16_IBIN32S_IBIN32S:
        case PCIT::HASH_MBIN32U_MBIN32_IBIN32S_IBIN32S:
        case PCIT::HASH_MBIN32U_MBIN64_IBIN32S_IBIN32S:
        {
          jit_value_t len = NULL;

          src1 = inst->getROps()[0]; 
          res = inst->getWOps()[0];

          // Need to get pointer to operands
          src1JitVal = src1->getJitValue(this, f, jit_type_void_ptr, block);

          // Get length and setup string if we're hashing a varchar.
          if (opc == PCIT::HASH_MBIN32U_MATTR5)
            len = genStringSetup(f, &src1JitVal, src1, TRUE);

          resJitVal = genHash(f, src1JitVal, len, loopIndex, src1->getLen());

          res->storeJitValue(this, f, jit_type_uint, resJitVal, block);

          break;
        }

        case PCIT::RANGE_MFLT64:
        {
          break;
        }

#if 0
        case PCIT::HASH_MBIN32U_MPTR32_IBIN32S_IBIN32S:
        case PCIT::HASH_MBIN32U_MBIN16_IBIN32S_IBIN32S:
        case PCIT::HASH_MBIN32U_MBIN32_IBIN32S_IBIN32S:
        case PCIT::HASH_MBIN32U_MBIN64_IBIN32S_IBIN32S:
        {
          jit_value_t t1, t2, t3, t4, t5;

          src1 = inst->getROps()[0]; 
          res = inst->getWOps()[0];

          Int32 j, len = src1->getLen();

          // Need to get pointers to operands
          src1JitVal = src1->getJitValue(this, f, jit_type_void_ptr, block);

          // Handles fixed-length strings, but let's not do all of them now.
          if (len > 2000)
            assert(FALSE);

          t1 = jit_insn_load_relative(f, src1JitVal, 0, jit_type_ubyte);
          resJitVal = jit_insn_load_elem(f, hashTableJitVal_, t1,
                                         jit_type_uint);

          // Unroll hash using known length
          for (j=1; j < len; j++)
          {
            //
            // Compute: t1 = src[j]
            //          t2 = randomHashValues[t1]
            //          t3 = resJitVal << 1
            //          t4 = resJitVal >> 31
            //          t5 = (t3 | t4)
            //    resJitVal = t5 ^ resJitVal
            //
            t1 = jit_insn_load_relative(f, src1JitVal, j, jit_type_ubyte);
            t2 = jit_insn_load_elem(f, hashTableJitVal_, t1, jit_type_uint);
            t3 = jit_insn_shl(f, resJitVal, oneJitVal_);
            t4 = jit_insn_ushr(f, resJitVal, shiftRVal);
            t5 = jit_insn_or(f, t3, t4);
            resJitVal = jit_insn_xor(f, t5, t2);
          }

          res->storeJitValue(this, f, jit_type_uint, resJitVal, block);

          break;
        }
#endif

        case PCIT::SUB_MBIGS_MBIGS_MBIGS_IBIN32S:
        {
          res = inst->getWOps()[0];
          src1 = inst->getROps()[0]; 
          src2 = inst->getROps()[1]; 

          jit_value_t lenJitVal =
            jit_value_create_nint_constant(f, jit_type_int, src1->getLen());

          // Need to get pointers to operands
          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);
          resJitVal = res->getJitValue(this, f, res->getJitType(), block);

          // Set up arguments and make indirect call to compare routine.
          jit_value_t args[4] = {resJitVal, src1JitVal, src2JitVal, lenJitVal};
          resJitVal = jit_insn_call_indirect(f, bigSubJitVal,
                                             bigSig, args, 4, 0);

          jit_insn_branch_if(f, resJitVal, &errorJitLabel);

          break;
        }

        case PCIT::ADD_MBIGS_MBIGS_MBIGS_IBIN32S:
        {
          jit_label_t fastPathLabel = jit_label_undefined;
          jit_label_t endLabel = jit_label_undefined;
          jit_value_t t1, t2, t3, t4, t5, carry = NULL;

          res = inst->getWOps()[0];
          src1 = inst->getROps()[0]; 
          src2 = inst->getROps()[1]; 

          Int32 len = src1->getLen();

          // Need to get pointers to operands
          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);
          resJitVal = res->getJitValue(this, f, res->getJitType(), block);

          // Get sign bits.  No need to clear (I think)
          if ((len % 4) == 0) {
            t1=jit_insn_load_relative(f, src1JitVal, (len>>2)-1,jit_type_uint);
            t2=jit_insn_load_relative(f, src2JitVal, (len>>2)-1,jit_type_uint);
            t3=jit_insn_and(f, t1, jitValMask32);
            t4=jit_insn_and(f, t2, jitValMask32);
          }
          else {
            t1=jit_insn_load_relative(f, src1JitVal,(len>>1)-1,jit_type_ushort);
            t2=jit_insn_load_relative(f, src2JitVal,(len>>1)-1,jit_type_ushort);
            t3=jit_insn_and(f, t1, jitValMask16);
            t4=jit_insn_and(f, t2, jitValMask16);
          }
          t5 = jit_insn_eq(f, t3, t4);
          jit_insn_branch_if(f, t5, &fastPathLabel);

          // Call subtract function here.
          genBignumSub(this, f, res, src1JitVal, src2JitVal, t1, t2, tSrc1,
                       tSrc2, block);

          jit_insn_branch(f, &endLabel);


          //
          // Fast path.  Here's the algorithm:
          //
          // 1. Add each 16-bit value of the bignum and carry over remainder
          // 2. Ignore (and don't record) remainder for last addition
          // 3. Get sign of src1/src2 and use it to set sign of target.
          //

          jit_insn_label(f, &fastPathLabel);

          for (i=0; i < len; i+=2)
          {
            t1 = jit_insn_load_relative(f, src1JitVal, i, jit_type_ushort);
            t2 = jit_insn_load_relative(f, src2JitVal, i, jit_type_ushort);
            t3 = jit_insn_add(f, t1, t2);

            if (i != 0)
              t3 = jit_insn_add(f, t3, carry);

            t4 = jit_insn_convert(f, t3, jit_type_ushort, 0);

            // Process the last short with the sign bit differently.
            if (i == (len - 2)) {
              jit_value_t clear1 = jit_insn_and(f, t4, jitValNotMask16);
              jit_value_t get1  = jit_insn_and(f, t1, jitValMask16);
              t4 = jit_insn_or(f, clear1, get1);
              jit_insn_store_relative(f, resJitVal, i, t4);
            }
            else {
              jit_insn_store_relative(f, resJitVal, i, t4);
              carry = jit_insn_ushr(f, t3, jitVal16);
            } 
          }

          jit_insn_label(f, &endLabel);

          break;
        }

        case PCIT::OPDATA_MPTR32_IBIN32S:
        case PCIT::OPDATA_MPTR32_IBIN32S_IBIN32S:
        case PCIT::OPDATA_MBIN16U_IBIN32S:
        case PCIT::OPDATA_MATTR5_IBIN32S:
        {
          OPLIST opList =
            (inst->getWOps().entries() ? inst->getWOps() : inst->getROps());

          Int32 index =
            (opc == PCIT::OPDATA_MPTR32_IBIN32S_IBIN32S) ? inst->code[4] 
            : ((opc == PCIT::OPDATA_MATTR5_IBIN32S) ? inst->code[6]
               : inst->code[3]);

          // Add the "correct" operand for this opdata instruction, replacing
          // the old one in the operand list used during pcode opts.
          setupClauseOperand(this, opList, opData, index, inst->clause_);

          break;
        }

        case PCIT::CLAUSE_EVAL:
        {
          jit_label_t doneLabel = jit_label_undefined;
          jit_label_t trueLabel = jit_label_undefined;
          jit_label_t falseLabel = jit_label_undefined;
          jit_label_t nullLabel = jit_label_undefined;

          // on 64-bit the clause pointer is in code[1] and code[2]
          ex_clause* clause = (ex_clause*)*(Long*)&(inst->code[1]);
          NABoolean processNulls = inst->code[1 + PCODEBINARIES_PER_PTR];

          switch (clause->getClassID()) {
            case ex_clause::AGGR_MIN_MAX_ID:
            {
              //
              // Format:
              //
              // attr[0] - Aggregate
              // attr[1] - Column
              // attr[2] - Comparison result
              //

              // First branch away if comp result indicates no update needed. 
              // Note, since this check is always made, the jit value from the
              // null-check can be used outside of the CLAUSE_EVAL.
              src2 = opDataVals[2];
              src2JitVal = src2->getJitValue(this, f, jit_type_int, block);
              jit_insn_branch_if_not(f, src2JitVal, &doneLabel);

              // We need to copy over source into aggregate.  First take care of
              // the null indicator if the aggregate is nullable.
              if (clause->getOperand(0)->getNullFlag()) {
                // If column is nullable too, then we need to check it.  If it's
                // NULL, we don't do anything.

                if (clause->getOperand(1)->getNullFlag()) {
                  src1 = opDataNulls[1];
                  src1JitVal =
                    src1->getJitValue(this, f, src1->getJitType(), block,
                                      NULL, TRUE /* no assign jit val */);

                  jit_insn_branch_if(f, src1JitVal, &doneLabel);
                }

                // Null out the null indicator for the aggregate.  Based on the
                // current code, the null indicator for the aggregate is always
                // in exploded format, so assert that here and then simplify
                // code generation.
                assert(!opDataNulls[0]->forAlignedFormat());

                opDataNulls[0]->setJitValue(this, f, zeroJitVal16, block, TRUE);
              }

              // Now we have to handle moving the data over from column to aggr.
              res = opDataVals[0];
              src1 = opDataVals[1];

              src1JitVal = src1->getJitValue(this, f, src1->getJitType(),
                                             block, NULL, TRUE /* no assign */);

              if (res->isVarchar())
              {
                // The following code was taken from MOVE_MATTR5_MATTR5, but
                // re-formatted here.
                jit_type_t vcType;
                jit_value_t vcLenJitVal;

                if (src1->getVcIndicatorLen() == 2) {
                  vcType = jit_type_ushort;
                  vcLenJitVal=jit_value_create_nint_constant(f, jit_type_int,2);
                }
                else {
                  vcType = jit_type_uint;
                  vcLenJitVal=jit_value_create_nint_constant(f, jit_type_int,4);
                }

                resJitVal = res->getJitValue(this, f, res->getJitType(), block,
                                             NULL, TRUE /* no assign */);

                jit_insn_store(f, tRes, resJitVal);
                jit_insn_store(f, tSrc1, src1JitVal);

                jit_value_t srcLen =
                  jit_insn_load_relative(f, src1JitVal, 0, vcType);

                srcLen = jit_insn_add(f, srcLen, vcLenJitVal);

                genUnalignedMemcpy(f, tRes, tSrc1, srcLen);
              }
              else
                res->storeJitValue(this, f, res->getJitType(),src1JitVal,
                                   block, NULL, TRUE);

              // Reached the end point
              jit_insn_label(f, &doneLabel);

              break;
            }

            case ex_clause::COMP_TYPE:
            {
              switch (((ex_comp_clause*)clause)->getInstruction()) {
                case NE_DATETIME_DATETIME:
                case EQ_DATETIME_DATETIME:
                case LT_DATETIME_DATETIME:
                case GT_DATETIME_DATETIME:
                case LE_DATETIME_DATETIME:
                case GE_DATETIME_DATETIME:
                {
                  jit_value_t year1, year2;

                  res = opDataVals[0];
                  src1 = opDataVals[1];
                  src2 = opDataVals[2];

                  src1JitVal = src1->getJitValue(this, f, jit_type_void_ptr,
                                                 block, NULL);
                  src2JitVal = src2->getJitValue(this, f, jit_type_void_ptr,
                                                 block, NULL);
                  if (processNulls) {
                    PCodeOperand* nullOp;
                    jit_value_t nullJitVal;

                    if (clause->getOperand(1)->getNullFlag()) {
                      nullOp = opDataNulls[1];
                      nullJitVal =
                      nullOp->getJitValue(this, f, nullOp->getJitType(), block,
                                          NULL, TRUE /* no assign jit val */);

                      jit_insn_branch_if(f, nullJitVal, &nullLabel);
                    }

                    if (clause->getOperand(2)->getNullFlag()) {
                      nullOp = opDataNulls[2];
                      nullJitVal =
                      nullOp->getJitValue(this, f, nullOp->getJitType(), block,
                                          NULL, TRUE /* no assign jit val */);

                      jit_insn_branch_if(f, nullJitVal, &nullLabel);
                    }
                  }

                  long dtCode =
                    ((ExpDatetime*)(clause->getOperand(1)))->getPrecision();

                  switch (dtCode)
                  {
                    case REC_DTCODE_TIME:
                    case REC_DTCODE_DATE:
                    case REC_DTCODE_TIMESTAMP:
                    {
                      // Boolean used to indicate whether branch should go to
                      // true block or not.  Assume "not" by default.
                      NABoolean dumpTrueBlockFirst = TRUE;  // May change later

                      Int32 size = (clause->getOperand(1))->getLength();

                      // Cycle through each field in the date-related struct.
                      Int32 typeSize = 0;
                      for (i=0; i < size; i += typeSize)
                      {
                        jit_type_t type;

                        // Are we getting the fractional seconds?
                        if ((i == size-4) && (dtCode == REC_DTCODE_TIMESTAMP)) {
                          type = jit_type_uint;
                          typeSize = 4;
                        }
                        // Are we getting the year?
                        else if ((i==0) && (dtCode != REC_DTCODE_TIME)) {
                          type = jit_type_ushort;
                          typeSize = 2;
                        }
                        // We're just getting some particular byte field
                        else {
                          type = jit_type_ubyte;
                          typeSize = 1;
                        }

                        // Get the fields from both sources.
                        year1 = jit_insn_load_relative(f, src1JitVal, i, type);
                        year2 = jit_insn_load_relative(f, src2JitVal, i, type);

                        // Depending on the comparison being performed, generate
                        // 1 (or 2) compares and branches.  Also, based on the
                        // comparison, the fall-through may be the "true" block,
                        // or it may be the "false" block - default is TRUE.

                        switch (((ex_comp_clause*)clause)->getInstruction()) {
                          case NE_DATETIME_DATETIME:
                            resJitVal = jit_insn_ne(f, year1, year2);
                            jit_insn_branch_if(f, resJitVal, &trueLabel);
                            dumpTrueBlockFirst = FALSE;
                            break;
                          case EQ_DATETIME_DATETIME:
                            resJitVal = jit_insn_ne(f, year1, year2);
                            jit_insn_branch_if(f, resJitVal, &falseLabel);
                            break;
                          case LT_DATETIME_DATETIME:
                            resJitVal = jit_insn_lt(f, year1, year2);
                            jit_insn_branch_if(f, resJitVal, &trueLabel);
                            resJitVal = jit_insn_gt(f, year1, year2);
                            jit_insn_branch_if(f, resJitVal, &falseLabel);
                            dumpTrueBlockFirst = FALSE;
                            break;
                          case GT_DATETIME_DATETIME:
                            resJitVal = jit_insn_gt(f, year1, year2);
                            jit_insn_branch_if(f, resJitVal, &trueLabel);
                            resJitVal = jit_insn_lt(f, year1, year2);
                            jit_insn_branch_if(f, resJitVal, &falseLabel);
                            dumpTrueBlockFirst = FALSE;
                            break;
                          case LE_DATETIME_DATETIME:
                            resJitVal = jit_insn_lt(f, year1, year2);
                            jit_insn_branch_if(f, resJitVal, &trueLabel);
                            resJitVal = jit_insn_gt(f, year1, year2);
                            jit_insn_branch_if(f, resJitVal, &falseLabel);
                            break;
                          case GE_DATETIME_DATETIME:
                            resJitVal = jit_insn_gt(f, year1, year2);
                            jit_insn_branch_if(f, resJitVal, &trueLabel);
                            resJitVal = jit_insn_lt(f, year1, year2);
                            jit_insn_branch_if(f, resJitVal, &falseLabel);
                            break;
                        }
                      }

                      if (!dumpTrueBlockFirst)
                      {
                        // False label first.
                        jit_insn_label(f, &falseLabel);
                        res->storeJitValue(this, f, jit_type_int, zeroJitVal_,
                                           block);
                        jit_insn_branch(f, &doneLabel);

                        // True label next.
                        jit_insn_label(f, &trueLabel);
                        res->storeJitValue(this, f, jit_type_int, oneJitVal_,
                                           block);
                        jit_insn_branch(f, &doneLabel);
                      }
                      else {
                        // True label first.
                        jit_insn_label(f, &trueLabel);
                        res->storeJitValue(this, f, jit_type_int, oneJitVal_,
                                           block);
                        jit_insn_branch(f, &doneLabel);

                        // False label next.
                        jit_insn_label(f, &falseLabel);
                        res->storeJitValue(this, f, jit_type_int, zeroJitVal_,
                                           block);
                        jit_insn_branch(f, &doneLabel);
                      }

                      // Null label last.
                      jit_insn_label(f, &nullLabel);
                      res->storeJitValue(this,f,jit_type_int,neg1JitVal_,block);

                      jit_insn_label(f, &doneLabel);

                      break;
                    }
                      
                    default:
                      assert(FALSE);
                  }
                  break;
                }
              }

              break;
            }
          }

          // Clear out opData for subsequent use.
          str_pad((char*)opData,
                  (3*ex_clause::MAX_OPERANDS)*sizeof(PCodeOperand*), 0);

          break;
        }

#if 0
        //
        // SUBSTR should be completely implemented, but I don't recall if it
        // was properly tested.  Also, SUBSTR should be improved such that if
        // the result is to a temp operand, then modify the temp such that it
        // serves really as a pointer to a string, and not the string itself,
        // such that the computation needed would just be a) storage of the
        // variable length (if varchar), and b) the pointer to the start of the
        // substring.  This will save on unnecessary string copies.
        case PCIT::SUBSTR_MATTR5_MATTR5_MBIN32S_MBIN32S:
        {
          jit_value_t strLen, tgtLen, cmpVal, startPosVal, forLenVal;
          jit_label_t legitStartLabel = jit_label_undefined;
          jit_label_t startCopyLabel = jit_label_undefined;
          jit_label_t doneLabel = jit_label_undefined;
          Int32 startPos, totalLen=0, forLen=0;

          // Get operands
          src1 = inst->getROps()[0];
          res = inst->getWOps()[0];

          // Get start position and forLen (if provided)
          startPos = getIntConstValue(inst->getROps()[1]);
          if (inst->getROps().entries() == 3)
            forLen = getIntConstValue(inst->getROps()[2]);

          // Set up src1 string and length
          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          strLen = genStringSetup(f, &src1JitVal, src1);

          // Let's set up the src1JitVal pointer to the start position.
          startPosVal =
            jit_value_create_nint_constant(f, jit_type_int, startPos-1);
          src1JitVal = jit_insn_add(f, src1JitVal, startPosVal);

          // If we're dealing with a varchar, do checks at run-time, and then
          // generate copy to completion.
          if (src1->isVarchar())
          {
            // Set up value that is used to record length to copy
            jit_value_t subLenVal = jit_value_create(f, jit_type_int);

            // 1. If start > strLen, return empty string.
            cmpVal = jit_insn_lt(f, startPosVal, strLen);
            jit_insn_branch_if(f, cmpVal, &legitStartLabel);

            jit_insn_store(f, subLenVal, zeroJitVal_);
            jit_insn_branch(f, &startCopyLabel);

            jit_insn_label(f, &legitStartLabel);

            // 2. Calculate max # of chars to copy
            if (startPos == 1)
              tgtLen = strLen;
            else {
              jit_value_t tVal =
                jit_value_create_nint_constant(f, jit_type_int, 1-startPos);
              tgtLen = jit_insn_add(f, strLen, tVal);
            }

            jit_insn_store(f, subLenVal, tgtLen);

            // 3. If a forLen was provided, tgtLen = min (tgtLen, forLen)
            if (forLen) {
              forLenVal = jit_value_create_nint_constant(f,jit_type_int,forLen);

              cmpVal = jit_insn_ge(f, forLenVal, tgtLen);
              jit_insn_branch_if(f, cmpVal, &startCopyLabel);

              jit_insn_store(f, subLenVal, forLenVal);
            }

            jit_insn_label(f, &startCopyLabel);

            // Get result string.  Store length first before setting the string
            // up.  The result length does not need to be retrieved.
            resJitVal = res->getJitValue(this, f, res->getJitType(), block);
            if (res->getVcIndicatorLen() == 2)
              tgtLen = jit_insn_convert(f, subLenVal, jit_type_ushort, 0);
            else
              tgtLen = jit_insn_convert(f, subLenVal, jit_type_uint, 0);

            jit_insn_store_relative(f, resJitVal, 0, tgtLen);

            // If the length to copy is zero, bail out
            jit_insn_branch_if_not(f, subLenVal, &doneLabel);

            // Set up the target string, without asking for the length.
            genStringSetup(f, &resJitVal, res, FALSE /* pad */, FALSE /*len*/);

            jit_insn_store(f, tRes, resJitVal);
            genUnalignedMemcpy(f, tRes, src1JitVal, subLenVal);
          }
          else {
            // Do compile-time checks here, and then generate copy to completion
            Int32 maxLen = src1->getLen();
            if (startPos > maxLen)
              totalLen = 0;
            else {
              totalLen = (maxLen - startPos + 1);
              if (forLen) {
                if (forLen < totalLen)
                  totalLen = forLen;
              }
            }

            // Store the vc indicator length
            tgtLen = jit_value_create_nint_constant(f,jit_type_ushort,totalLen);
            if (res->getVcIndicatorLen() == 4) {
              tgtLen =
                jit_value_create_nint_constant(f, jit_type_uint, totalLen);
            }
            resJitVal = res->getJitValue(this, f, res->getJitType(), block);
            jit_insn_store_relative(f, resJitVal, 0, tgtLen);

            // Nothing to write out after length if length itself is 0.
            if (totalLen > 0)
            {
              // Set up the target string, without asking for the length.
              genStringSetup(f, &resJitVal, res, FALSE /*pad*/, FALSE /*len*/);

              tgtLen = jit_value_create_nint_constant(f,jit_type_uint,totalLen);

              jit_insn_store(f, loopIndex, tgtLen);
              jit_insn_store(f, tRes, resJitVal);
              genUnalignedMemcpy(f, tRes, src1JitVal, loopIndex);
            }
          }

          // Done!  resJitVal already attached to result, so just break.
          jit_insn_label(f, &doneLabel);

          break;
        }
#endif

        case PCIT::CONCAT_MATTR5_MATTR5_MATTR5:
        {
          jit_value_t len1, len2, totalLen;

          // Get operands
          src1 = inst->getROps()[0];
          src2 = inst->getROps()[1];
          res = inst->getWOps()[0];

          // Check previously made check that src1->len + src2->len <= tgt->len
          assert((src1->getVcMaxLen() + src2->getVcMaxLen() <=
                 res->getVcMaxLen()));

          // Set up src1 string and length
          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          len1 = genStringSetup(f, &src1JitVal, src1);

          // Set up src2 string and length
          src2JitVal = src2->getJitValue(this, f, src2->getJitType(), block);
          len2 = genStringSetup(f, &src2JitVal, src2);

          // Calculate what the final length should be for target.
          totalLen = jit_insn_add(f, len1, len2);

          // Get result string.  Store length (if varchar) first before setting
          // the string up.  The result length does not need to be retrieved.
          resJitVal = res->getJitValue(this, f, res->getJitType(), block);
          if (res->isVarchar()) {
            if (res->getVcIndicatorLen() == 2)
              totalLen = jit_insn_convert(f, totalLen, jit_type_ushort, 0);
            else
              totalLen = jit_insn_convert(f, totalLen, jit_type_uint, 0);

            jit_insn_store_relative(f, resJitVal, 0, totalLen);
          }
          genStringSetup(f, &resJitVal, res, FALSE /* pad */, FALSE /* len */);

          // If all operands are fixed strings, then call genUnalignedMemcpy
          // such that the moves are inlined.
          if (!res->isVarchar() && !src1->isVarchar() && !src2->isVarchar()) {
            genUnalignedMemcpy(f, resJitVal, src1JitVal, NULL, src1->getLen());
            resJitVal = jit_insn_add(f, resJitVal, len1);
            genUnalignedMemcpy(f, resJitVal, src2JitVal, NULL, src2->getLen());
          }
          else {
            // Call genUnalignedMemcpy such that a loop is generated to perform
            // each of the moves.
            jit_insn_store(f, tRes, resJitVal);
            jit_insn_store(f, tSrc1, src1JitVal);

            // If src1 is not a varchar, length must be stored in variable first
            if (src1->isVarchar())
              genUnalignedMemcpy(f, tRes, tSrc1, len1);
            else {
              jit_insn_store(f, loopIndex, len1);
              genUnalignedMemcpy(f, tRes, tSrc1, loopIndex);
            }

            // Set up next source string.  Note, tRes will already be in pos
            // for storage.
            jit_insn_store(f, tSrc1, src2JitVal);

            // If src2 is not a varchar, length must be stored in variable first
            if (src2->isVarchar())
              genUnalignedMemcpy(f, tRes, tSrc1, len2);
            else {
              jit_insn_store(f, loopIndex, len2);
              genUnalignedMemcpy(f, tRes, tSrc1, loopIndex);
            }
          }

          // Nothing more to be done - resJitVal already attached to result.

          break;
        }

        case PCIT::GENFUNC_MATTR5_MATTR5_IBIN32S:
        {
          jit_value_t len, cmp, addVal, minVal, maxVal, changeVal, b1;
          jit_value_t temp1, temp2, temp3;
          jit_label_t loopLabel = jit_label_undefined;
          jit_label_t endLabel = jit_label_undefined;
          jit_label_t noChangeLabel = jit_label_undefined;
          Int32 subOpc = inst->code[11];

          src1 = inst->getROps()[0];
          res = inst->getWOps()[0];

          src1JitVal = src1->getJitValue(this, f, src1->getJitType(), block);
          len = genStringSetup(f, &src1JitVal, src1);

          // Get result string.  Store length (if varchar) first before setting
          // the string up.
          resJitVal = res->getJitValue(this, f, res->getJitType(), block);
          if (res->isVarchar()) {
            jit_value_t newLen;
            if (res->getVcIndicatorLen() == 2)
              newLen = jit_insn_convert(f, len, jit_type_ushort, 0);
            else
              newLen = jit_insn_convert(f, len, jit_type_uint, 0);

            jit_insn_store_relative(f, resJitVal, 0, newLen);
          }
          genStringSetup(f, &resJitVal, res, FALSE /* pad */, FALSE /* len */);

          if (subOpc == ITM_UPPER) {
            minVal = jit_value_create_nint_constant(f, jit_type_sbyte, 'a');
            maxVal = jit_value_create_nint_constant(f, jit_type_sbyte, 'z');
            changeVal = jit_value_create_nint_constant(f, jit_type_sbyte, -32);
          } else {
            minVal = jit_value_create_nint_constant(f, jit_type_sbyte, 'A');
            maxVal = jit_value_create_nint_constant(f, jit_type_sbyte, 'Z');
            changeVal = jit_value_create_nint_constant(f, jit_type_sbyte, 32);
          }

          // Set up pointers to be used and modified inside of loop.
          jit_insn_store(f, tRes, resJitVal);
          jit_insn_store(f, tSrc1, src1JitVal);
          jit_insn_store(f, loopIndex, len);

          // Branch away if we're at end of string
          jit_insn_branch_if_not(f, loopIndex, &endLabel);

          // Loop start
          jit_insn_label(f, &loopLabel);

          // Get byte in source string to compare and inc/dec (if needed)
          b1 = jit_insn_load_relative(f, tSrc1, 0, jit_type_sbyte);

          // Perform comparison(s) and branch.  For eq/ne we just do one
          cmp = jit_insn_lt(f, b1, minVal);
          jit_insn_branch_if(f, cmp, &noChangeLabel);
          cmp = jit_insn_gt(f, b1, maxVal);
          jit_insn_branch_if(f, cmp, &noChangeLabel);

          // Transform byte to upper/lower case and re-save.
          addVal = jit_insn_add(f, b1, changeVal);
          jit_insn_store(f, b1, addVal);

          // Store byte into result.
          jit_insn_label(f, &noChangeLabel);
          jit_insn_store_relative(f, tRes, 0, b1);

          // Decrement loop index
          temp1 = jit_insn_sub(f, loopIndex, oneJitVal_);
          jit_insn_store(f, loopIndex, temp1);

          // Increment string pointers
          temp2 = jit_insn_add(f, tSrc1, oneJitVal_);
          temp3 = jit_insn_add(f, tRes, oneJitVal_);
          jit_insn_store(f, tSrc1, temp2);
          jit_insn_store(f, tRes, temp3);

          // Branch back if loop index <> 0
          jit_insn_branch_if(f, loopIndex, &loopLabel);

          // Nothing more to do since result written inside loop.
          jit_insn_label(f, &endLabel);

          break;
        }
      }
    } ENDFE_INST_IN_BLOCK
  }

  // Lastly, emit an error label where we return ex_expr::ERROR, if such a
  // label was generated.
  if (errorJitLabel != jit_label_undefined) {
    jit_insn_label(f, &errorJitLabel);

    // Set up arguments and make indirect call to compare routine.
    jit_value_t atp1JitVal = jit_value_get_param(f,1);
    jit_value_t args[2] = {atp1JitVal, exprJitVal};
    resJitVal = jit_insn_call_indirect(f, reportErrJitVal,
                                       reportErrSig, args, 2, 0);
    jit_insn_return(f, resJitVal);
  }

  if (getJitFailureSeen()) {
    jit_context_destroy(context);
    return;
  }

  if (debug)
    printBlocks("NativeExpr", FALSE);

  // Set optimization level for jit compilation.
  if (!lowerOptLevel)
    jit_function_set_optimization_level(f, 2);

  // Dump IL for function
  if (debug)
    jit_dump_function(stdout, f, "IL:");

  // Compile the function
  Int32 size = jit_function_compile(f);

  // Get entry point into function and add code into constants array
  ex_expr::evalPtrType entry = (ex_expr::evalPtrType)jit_function_to_closure(f);
  CollIndex* offPtr = addConstant((void*)entry, size, 8);

  // Dump function 
  if (debug)
    jit_dump_function(stdout, f, "X86:");

  // Store offset into evalPtr_
  expr_->setEvalPtr((ex_expr::evalPtrType)(*offPtr));

  // Mark this expression appropriately so that the native function gets called
  expr_->setPCodeMoveFastpath(TRUE);
  expr_->setPCodeNative(TRUE);

  if (showplanSpace)
  {
    NABoolean firstTimeSeen = TRUE;
    char line[256];
    char* linePtr;
    char fileName[50];
    Int32 i;

    // Indicates how much to indent each line.
    const int INDENT_START = 4;

    // Filename for use in dumping text should have process pid in it.
    sprintf(fileName, "/tmp/%d%s", getpid(), "dump1.txt");

    // Use the jit's dump interface to generate the x86 instructions.
    FILE* out = fopen(fileName, "w");
    jit_dump_function(out, f, "eval:");
    fclose(out);
    
    // First print out a header for the native expr dump
    sprintf(line, "Native Expr (Length: %d, Opt: %d)", *offPtr, optLevel);
    showplanSpace->allocateAndCopyToAlignedSpace(line, str_len(line),
                                                 sizeof(short));

    // Start each line with some indentation.
    for (i=0; i < INDENT_START; i++)
      line[i] = ' ';

    // Now read from the jit dump file and feed the lines into our space
    // object.  Is there a better way of doing all of this?
    out = fopen(fileName, "r");
    while (fgets(&(line[INDENT_START]), sizeof line, out) != NULL) {
      // All lines we're interested in begin with "f", as in the address
      // starting as "fffff..".
      if (line[INDENT_START] != 'f')
        continue;

      if (firstTimeSeen) {
        // The first string is just a header that starts the dump off - we can
        // ignore this.
        firstTimeSeen = FALSE;
        continue;
      }

      // Remove newline char (if exists) since call to insert into space object
      // automatically puts in a newline character.
      Int32 len = str_len(line);
      if ((len >= 1) && (line[len-1] = '\n')) {
        len--;
        line[len] = 0;
      }

      showplanSpace->allocateAndCopyToAlignedSpace(line, len, sizeof(short));
    }

    // Close the file out and remove temporary file created.
    fclose(out);
    sprintf(line, "rm %s", fileName);
    system(line);

    // Add a newline to start the next expression off cleanly.
    showplanSpace->allocateAndCopyToAlignedSpace("\n", 1, sizeof(short));
  }

  // Destroy context and cleanup
  jit_context_destroy(context);
}
#endif /* NA_LINUX_LIBJIT */

