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
* File:         ExpPCodeInstruction.h
* Description:  PCODE opcode list (compiler and executor) and classes and
*               enums for compiling clauses to PCODE.
* Created:      8/25/97
* Language:     C++
*
*
*
****************************************************************************
*/
#ifndef ExpPCodeInstruction_h
#define ExpPCodeInstruction_h

#include "NAStdlib.h"

// Forward internal declaractions
//
class PCIType;
class PCIAddressingModeList;
class PCIOperandList;
class PCodeInstruction;

// RMI reduction typedef's
//
typedef PCIType PCIT;
typedef PCIAddressingModeList AML;
typedef PCIOperandList OL;
typedef PCodeInstruction PCI;
typedef Int64 PCIID; 

// PCode Binary (formally referred as code) remains to be 32-bit long on
// 64-bit platform but when it contains pointers/addresses it takes two
// consecutive storge space. The same applies to the operand list as Operand
// is also a 32-bit value.
// As result some PCode instructions (PCI) would have longer PCodeBinary
// sequences and operand lists. During PCode walk through in optimization,
// address fixup, and execution, extra pointer adjustment would be needed
// for PCode instructions involving address/pointer operands.

typedef Int32 PCodeBinary;

// Pointers in PCodeBinary stream can take 1 or 2 entries
// Currently 1 on 32-bit platform and 2 on 64-bit platform
#define PCODEBINARIES_PER_PTR sizeof(char*)/sizeof(PCodeBinary)

// PCIType
//
// The PCIType class encapsulates and provides a uniform name space
// for enumerations related to PCode (and nothing more). PCIType contains 
// the Operation, AddressingMode, and Instruction enumerations and 
// static member functions for converting these enumerations to strings
// for display.
//
class PCIType {
public:

  // PCode operations -- These enumerations represent the different PCode
  // operations. These are NOT the actual byte codes that are 
  // executed. A PCode operation in conjunction with one or more
  // addressing modes and operands maps to one or more byte codes
  // that are interpreted when a expression is executed. Different PCode 
  // operations may map to the same byte codes (i.e. Op_LD_CONSTANT 
  // and Op_LD_TEMPORARY).
  //
  enum Operation {
    Op_OPDATA,
    // Moves and conversions
    Op_MOVE,
    // Nulls
    Op_NULL, Op_NOT_NULL_BRANCH, Op_NOT_NULL_BRANCH_COMP, Op_NULL_BITMAP, Op_NOT_NULL_BITMAP_BRANCH,
    Op_NULL_VIOLATION,  Op_REPLACE_NULL, Op_NULL_BYTES,
    // Relational operators
    Op_EQ, Op_LT, Op_GT, Op_LE, Op_GE, Op_NE, Op_COMP, Op_ZERO, Op_NOTZERO, 
    // Logical operators
    Op_AND, Op_OR, 
    // Arithematic operators
    Op_ADD, Op_SUB, Op_MUL, Op_DIV, Op_DIV_ROUND, Op_SUM, Op_MOD, Op_NEG,
    Op_MINMAX,
    // Bitwise operator
    Op_BITOR,
    // Hash value combination operator
    Op_HASHCOMB,
    // Encoding/decoding operators
    Op_ENCODE,
    Op_DECODE,
    // Hash function
    Op_HASH,

    // new hash function
    Op_HASH2_DISTRIB,

    // Generic functions. Actual function name(OperTypeEnum) will be stored 
    // as part of the generated pcode.
    // Number and type of operands will depend on OperTypeEnum.
    Op_GENFUNC,

    // Fill memory
    Op_FILL_MEM_BYTES,
    // Return function
    Op_RETURN,
    // Branching
    Op_BRANCH, Op_TARGET, Op_BRANCH_AND, Op_BRANCH_OR,
    // Error handling
    Op_RANGE_LOW, Op_RANGE_HIGH,
    // Evaluating clauses
    Op_CLAUSE, Op_CLAUSE_EVAL, Op_CLAUSE_BRANCH,
    // Miscellaneous
    Op_PROFILE, 
    Op_UPDATE_ROWLEN, Op_UPDATE_ROWLEN2, Op_UPDATE_ROWLEN3,
    Op_OFFSET,
    Op_HDR,              // clause header pcode
    Op_NULLIFZERO,
    Op_COPYVARROW,

    // source points to an 8 byte varchar value which is the ptr to data.
    // Length of source contains the length of data.
    Op_CONV_VC_PTR, 

    Op_NOP, Op_END
  };

  // PCode Addressing Modes -- These enumerations represent the different
  // PCode addressing modes. There are four different types of operands: 
  // integer, signed integer, unsigned integer and pointer; and three sizes: 
  // 64, 32, and 16 bit. The integer modes are used for operations that
  // are ignorant of the sign of the operand (i.e. BIT_XOR). Either
  // signed integer or unsigned integer modes are used for operations that
  // do depend on the sign of the operand (i.e. ADD).
  //
  enum AddressingMode {
    // None
    //
    AM_NONE,

    // Memory accesses
    //
    MBIN8, MBIN8U, MBIN8S, MBIN16U, MBIN16S, MBIN32U, MBIN32S, 
    MBIN64S, MBIN64U, MPTR32,
    MASCII, MDECS, MDECU, MFLT32, MFLT64, MATTR3, MATTR4, MATTR5, MATTR6,
    MBIGU, MBIGS, MUNI, MUNIV,

    // Immediate accesses
    IBIN8U, IBIN16U, IBIN32S, IBIN64S, IATTR4, IATTR3, IATTR2, IPTR
  };

  // PCode Instructions -- These enumerations represent the actual byte
  // codes that are interpreted when an expression is executed.
  // NOTE:  These byte codes are stored in modules, don't rearrange
  //        the existing numbers. Add new numbers at the end.
  enum Instruction {
    // OP data vector
    OPDATA_MPTR32_IBIN32S  = 0,
    OPDATA_MBIN16U_IBIN32S = 1,

    // Moves and conversions
    MOVE_MBIN32S              = 2,
    MOVE_MBIN32S_IBIN32S      = 3,
    MOVE_MBIN8_MBIN8_IBIN32S  = 4,
    MOVE_MBIN32U_MBIN16U      = 5,
    MOVE_MBIN32S_MBIN16U      = 6,
    MOVE_MBIN64S_MBIN16U      = 7,
    MOVE_MBIN32U_MBIN16S      = 8,
    MOVE_MBIN32S_MBIN16S      = 9,
    MOVE_MBIN64S_MBIN16S      = 10, 
    MOVE_MBIN64S_MBIN32U      = 11,
    MOVE_MBIN64S_MBIN32S      = 12,
    MOVE_MBIN64S_MDECS_IBIN32S = 13,
    MOVE_MBIN64S_MDECU_IBIN32S = 14,

    // Nulls
    NULL_MBIN16U                                    = 16,
    NULL_MBIN16U_IBIN16U                            = 17,
    NULL_TEST_MBIN32S_MBIN16U_IBIN32S               = 18,
    NOT_NULL_BRANCH_MBIN16S_MBIN16S_IBIN32S         = 19,
    NOT_NULL_BRANCH_MBIN16S_MBIN16S_MBIN16S_IBIN32S = 20,
    NULL_VIOLATION_MBIN16U                          = 21,
    NULL_VIOLATION_MBIN16U_MBIN16U                  = 22,

    INSTRUCTION_FILLER_23           = 23,
    INSTRUCTION_FILLER_24           = 24,
    INSTRUCTION_FILLER_25           = 25,
    INSTRUCTION_FILLER_26           = 26,
    INSTRUCTION_FILLER_27           = 27, 

    ZERO_MBIN32S_MBIN32U       = 31,
    NOTZERO_MBIN32S_MBIN32U    = 32,

    // Relational operations
    EQ_MBIN32S_MBIN16S_MBIN16S = 33,
    EQ_MBIN32S_MBIN16S_MBIN32S = 34,

    INSTRUCTION_FILLER_35 = 35,

    EQ_MBIN32S_MBIN32S_MBIN32S = 36,
    EQ_MBIN32S_MBIN16U_MBIN16U = 37,
    EQ_MBIN32S_MBIN16U_MBIN32U = 38,

    INSTRUCTION_FILLER_39 = 39,

    EQ_MBIN32S_MBIN32U_MBIN32U = 40, 

    EQ_MBIN32S_MBIN16S_MBIN32U = 41,

    INSTRUCTION_FILLER_42 = 42,

    LT_MBIN32S_MBIN16S_MBIN16S = 43,
    LT_MBIN32S_MBIN16S_MBIN32S = 44,

    INSTRUCTION_FILLER_45 = 45,

    LT_MBIN32S_MBIN32S_MBIN32S = 46,
    LT_MBIN32S_MBIN16U_MBIN16U = 47,
    LT_MBIN32S_MBIN16U_MBIN32U = 48,

    INSTRUCTION_FILLER_49 = 49,

    LT_MBIN32S_MBIN32U_MBIN32U = 50, 
    LT_MBIN32S_MBIN16S_MBIN32U = 51,

    INSTRUCTION_FILLER_52 = 52,
 
    GT_MBIN32S_MBIN16S_MBIN16S = 53,
    GT_MBIN32S_MBIN16S_MBIN32S = 54,

    INSTRUCTION_FILLER_55 = 55,

    GT_MBIN32S_MBIN32S_MBIN32S = 56,
    GT_MBIN32S_MBIN16U_MBIN16U = 57,
    GT_MBIN32S_MBIN16U_MBIN32U = 58,

    INSTRUCTION_FILLER_59 = 59,

    GT_MBIN32S_MBIN32U_MBIN32U = 60, 
    GT_MBIN32S_MBIN16S_MBIN32U = 61,

    INSTRUCTION_FILLER_62 = 62,
 
    // Boolean operations
    AND_MBIN32S_MBIN32S_MBIN32S = 63,
    OR_MBIN32S_MBIN32S_MBIN32S  = 64,

    // Arithematic operations
    ADD_MBIN16S_MBIN16S_MBIN16S = 65, 
    ADD_MBIN32S_MBIN32S_MBIN32S = 66, 
    ADD_MBIN64S_MBIN64S_MBIN64S = 67, 
    SUB_MBIN16S_MBIN16S_MBIN16S = 68, 
    SUB_MBIN32S_MBIN32S_MBIN32S = 69, 
    SUB_MBIN64S_MBIN64S_MBIN64S = 70,
    MUL_MBIN16S_MBIN16S_MBIN16S = 71,
    MUL_MBIN32S_MBIN32S_MBIN32S = 72, 
    MUL_MBIN64S_MBIN64S_MBIN64S = 73,
    SUM_MBIN32S_MBIN32S                        = 77, 

    INSTRUCTION_FILLER_78        = 78,

    SUM_MBIN64S_MBIN64S                        = 79,

    INSTRUCTION_FILLER_80        = 80,

    MOD_MBIN32S_MBIN32S_MBIN32S                = 83,
    MINMAX_MBIN8_MBIN8_MBIN32S_IBIN32S         = 84,

    // Encoding operations
    ENCODE_MASCII_MBIN16S_IBIN32S = 87,
    ENCODE_MASCII_MBIN16U_IBIN32S = 88,
    ENCODE_MASCII_MBIN32S_IBIN32S = 89,
    ENCODE_MASCII_MBIN32U_IBIN32S = 90, 
    ENCODE_MASCII_MBIN64S_IBIN32S = 91,
    ENCODE_DATETIME               = 92,
    ENCODE_NXX                    = 93,

    // Hash functions
    HASH_MBIN32U_MPTR32_IBIN32S_IBIN32S   = 94,

    // Branch
    BRANCH     = 95,
    BRANCH_I32 = 96,

    // Errors
    RANGE_LOW_S32S64  = 97,
    RANGE_HIGH_S32S64 = 98, 
    RANGE_LOW_U32S64  = 99,
    RANGE_HIGH_U32S64 = 100,
    RANGE_LOW_S64S64  = 101,
    RANGE_HIGH_S64S64 = 102, 
    RANGE_LOW_S16S64  = 103,
    RANGE_HIGH_S16S64 = 104,
    RANGE_LOW_U16S64  = 105,
    RANGE_HIGH_U16S64 = 106,

    // Miscellaneous
    CLAUSE         = 107,
    CLAUSE_EVAL    = 108,
    CLAUSE_BRANCH  = 109,
    PROFILE        = 110,
    NULL_VIOLATION = 111,
    UPDATE_ROWLEN  = 112,
    NOP            = 113,

    // End
    END = 114,

    // Fill memory
    FILL_MEM_BYTES = 115,

    MOVE_MBIN16U_IBIN16U = 116,

    EQ_MBIN32S_MASCII_MASCII = 117,

    // PCode instructions added after SQL/MX FCS:

    // arith pcode instructions.
    ADD_MBIN32S_MBIN16S_MBIN16S = 118,
    ADD_MBIN32S_MBIN16S_MBIN32S = 119,

    INSTRUCTION_FILLER_120 = 120,

    ADD_MBIN64S_MBIN32S_MBIN64S = 121,

    INSTRUCTION_FILLER_121 = 122,

    SUB_MBIN32S_MBIN16S_MBIN16S = 123,
    SUB_MBIN32S_MBIN16S_MBIN32S = 124,
    SUB_MBIN32S_MBIN32S_MBIN16S = 125,

    MUL_MBIN32S_MBIN16S_MBIN16S = 126,
    MUL_MBIN32S_MBIN16S_MBIN32S = 127,

    INSTRUCTION_FILLER_128 = 128,

    DIV_MBIN64S_MBIN64S_MBIN64S = 129,
    LT_MBIN32S_MASCII_MASCII    = 130,
    LE_MBIN32S_MASCII_MASCII    = 131,
    GT_MBIN32S_MASCII_MASCII    = 132,
    GE_MBIN32S_MASCII_MASCII    = 133,

    LE_MBIN32S_MBIN16S_MBIN16S  = 134,
    LE_MBIN32S_MBIN16S_MBIN32S  = 135,

    INSTRUCTION_FILLER_136  = 136,

    LE_MBIN32S_MBIN32S_MBIN32S  = 137,
    LE_MBIN32S_MBIN16U_MBIN16U  = 138,
    LE_MBIN32S_MBIN16U_MBIN32U  = 139,

    INSTRUCTION_FILLER_140  = 140,

    LE_MBIN32S_MBIN32U_MBIN32U  = 141, 
    LE_MBIN32S_MBIN16S_MBIN32U  = 142,

    INSTRUCTION_FILLER_143  = 143,

    GE_MBIN32S_MBIN16S_MBIN16S  = 144,
    GE_MBIN32S_MBIN16S_MBIN32S  = 145,

    INSTRUCTION_FILLER_146  = 146,

    GE_MBIN32S_MBIN32S_MBIN32S  = 147,
    GE_MBIN32S_MBIN16U_MBIN16U  = 148,
    GE_MBIN32S_MBIN16U_MBIN32U  = 149,

    INSTRUCTION_FILLER_150  = 150,

    GE_MBIN32S_MBIN32U_MBIN32U  = 151, 
    GE_MBIN32S_MBIN16S_MBIN32U  = 152,

    INSTRUCTION_FILLER_153  = 153,
    INSTRUCTION_FILLER_154  = 154,
    INSTRUCTION_FILLER_155  = 155,
 
    // Hash value combination operation
    HASHCOMB_MBIN32U_MBIN32U_MBIN32U  = 156,

    EQ_MBIN32S_MBIN16U_MBIN16S    = 157,
    LT_MBIN32S_MBIN16U_MBIN16S    = 158,
    LE_MBIN32S_MBIN16U_MBIN16S    = 159,
    GT_MBIN32S_MBIN16U_MBIN16S    = 160,
    GE_MBIN32S_MBIN16U_MBIN16S    = 161,

    EQ_MBIN32S_MBIN64S_MBIN64S    = 162,
    LT_MBIN32S_MBIN64S_MBIN64S    = 163,
    LE_MBIN32S_MBIN64S_MBIN64S    = 164,
    GT_MBIN32S_MBIN64S_MBIN64S    = 165,
    GE_MBIN32S_MBIN64S_MBIN64S    = 166,
    NE_MBIN32S_MBIN64S_MBIN64S    = 167,

    // short circuit branch instructions
    BRANCH_AND                    = 168,
    BRANCH_OR                     = 169,

    // Hash functions
    HASH_MBIN32U_MBIN32_IBIN32S_IBIN32S   = 170,
    HASH_MBIN32U_MBIN16_IBIN32S_IBIN32S   = 171,
    //HASH_MBIN32U_MPTR32_IBIN32S_IBIN32S = 172,
    HASH_MBIN32U_MBIN64_IBIN32S_IBIN32S   = 173,

    ENCODE_DECS                   = 176,

    GENFUNC_PCODE_1               = 181,

    INSTRUCTION_FILLER_182   = 182,
    MUL_MBIN64S_MBIN16S_MBIN32S   = 183,
    MUL_MBIN64S_MBIN32S_MBIN32S   = 184,

    INSTRUCTION_FILLER_185  = 185,

    MOD_MBIN32U_MBIN32U_MBIN32S            = 186,

    NOT_NULL_BRANCH_MBIN32S_MBIN16S_MBIN16S_IBIN32S = 187,
    NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S = 188,
    NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S_IBIN32S = 189,
    NOT_NULL_BRANCH_MBIN16S_IBIN32S = 190,
    GENFUNC_MBIN8_MBIN8_MBIN8_IBIN32S_IBIN32S = 191,

    INSTRUCTION_FILLER_192 = 192,

    MOVE_MFLT64_MBIN16S = 193,
    MOVE_MFLT64_MFLT32  = 194,

    INSTRUCTION_FILLER_195 = 195,

    NE_MBIN32S_MBIN16S_MBIN16S    = 196,

    INSTRUCTION_FILLER_197 = 197,
    INSTRUCTION_FILLER_198 = 198,
    INSTRUCTION_FILLER_199 = 199,
    
    // Standard move instructions
    MOVE_MBIN8_MBIN8 = 200,
    MOVE_MBIN16U_MBIN16U = 201,
    MOVE_MBIN32U_MBIN32U = 202,
    MOVE_MBIN64S_MBIN64S = 203,

    ADD_MFLT64_MFLT64_MFLT64 = 204, 
    SUB_MFLT64_MFLT64_MFLT64 = 205, 
    MUL_MFLT64_MFLT64_MFLT64 = 206, 
    DIV_MFLT64_MFLT64_MFLT64 = 207, 

    INSTRUCTION_FILLER_208 = 208,

    SUM_MFLT64_MFLT64 = 209,
    RANGE_MFLT64 = 210,
    
    // IEEE float comparisons
    NE_MBIN32S_MFLT64_MFLT64 = 211,
    NE_MBIN32S_MFLT32_MFLT32 = 212,

    GT_MBIN32S_MFLT64_MFLT64 = 213,
    GT_MBIN32S_MFLT32_MFLT32 = 214,

    GE_MBIN32S_MFLT64_MFLT64 = 215,
    GE_MBIN32S_MFLT32_MFLT32 = 216,

    EQ_MBIN32S_MFLT64_MFLT64 = 217,
    EQ_MBIN32S_MFLT32_MFLT32 = 218,

    LE_MBIN32S_MFLT64_MFLT64 = 219,
    LE_MBIN32S_MFLT32_MFLT32 = 220,

    LT_MBIN32S_MFLT64_MFLT64 = 221,
    LT_MBIN32S_MFLT32_MFLT32 = 222,

    HASH2_DISTRIB            = 223,

    // To support the aligned record format.
    NULL_BITMAP              = 224,        // main null bitmap manipulation
    NULL_BITMAP_SET          = 225,        // subcode to set a null bit
    NULL_BITMAP_CLEAR        = 226,        // subcode to clear a null bit
    NULL_BITMAP_TEST         = 227,        // subcode to test a null bit
    
    OPDATA_MPTR32_IBIN32S_IBIN32S                      = 228,
    NULL_TEST_BIT_MBIN32S_MBIN32S_IBIN32S              = 229,
    NNBB_MPTR32_IBIN16S_IBIN32S                        = 230,

    OPDATA_MATTR5_IBIN32S                              = 231,
    INSTRUCTION_FILLER_232 = 232,
    INSTRUCTION_FILLER_233 = 233,
    INSTRUCTION_FILLER_234 = 234,
    INSTRUCTION_FILLER_235 = 235,
    INSTRUCTION_FILLER_236 = 236,

    DIV_MBIN64S_MBIN64S_MBIN64S_ROUND = 237,

    NULL_VIOLATION_MPTR32_MPTR32_IPTR_IPTR       = 238,

    // added IPTR_ for pointer to function GetHistoryRowOLAP. 
    // note that the indexes are different on 64-bit mode
    // pCode[0] will contain pointer to the function, 
    // pCode[1] will point to Tcb to get to the history buffer
    // pCode[2] , pCode[3] --index (MBIN32S)/ 
    // pCode[4] pCode[5] -- result
    // pCode[6] pCode[7] -- source
    OFFSET_IPTR_IPTR_MBIN32S_MBIN64S_MBIN64S = 239,  
    // pCode[0] will contain pointer to the function, 
    // pCode[1] will point to Tcb to get to the history buffer
    // pCode[2] --index (IBIN32S)/ 
    // pCode[3] pCode[4] -- result
    // pCode[5] pCode[6] -- source
    OFFSET_IPTR_IPTR_IBIN32S_MBIN64S_MBIN64S = 240,  
                                                            
    ADD_MBIGS_MBIGS_MBIGS_IBIN32S = 241,
    SUB_MBIGS_MBIGS_MBIGS_IBIN32S = 242,
    MUL_MBIGS_MBIGS_MBIGS_IBIN32S = 243,
    MOVE_MBIGS_MBIGS_IBIN32S_IBIN32S = 244,
    MOVE_MBIGS_MBIN64S_IBIN32S = 245,
    MOVE_MBIN64S_MBIGS_IBIN32S = 246,

    // null branch processing for indirect varchars
    NOT_NULL_BRANCH_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S   = 247,
    NOT_NULL_BRANCH_MBIN32S_MBIN32S_IATTR3_IBIN32S           = 248,
    NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S   = 249,
    NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S           = 250,

    // move for fixed char
    MOVE_MASCII_MASCII_IBIN32S_IBIN32S                       = 251,

    INSTRUCTION_FILLER_252 = 252,
    INSTRUCTION_FILLER_253 = 253,
    INSTRUCTION_FILLER_254 = 254,
    INSTRUCTION_FILLER_255 = 255,
    INSTRUCTION_FILLER_256 = 256,
    INSTRUCTION_FILLER_257 = 257,
    INSTRUCTION_FILLER_258 = 258,
    INSTRUCTION_FILLER_259 = 259,
    INSTRUCTION_FILLER_260 = 260,
    INSTRUCTION_FILLER_261 = 261,

    // null test
    NULL_TEST_MBIN32S_MATTR5_IBIN32S_IBIN32S                 = 262,

    NE_MBIN32S_MASCII_MASCII                                 = 263,

    RETURN = 264,
    RETURN_IBIN32S = 265,

    HASHCOMB_BULK_MBIN32U = 266,
    NOT_NULL_BRANCH_BULK = 267,
    MOVE_BULK = 268,
    NULL_BITMAP_BULK = 269,

    COMP_MBIN32S_MBIGS_MBIGS_IBIN32S_IBIN32S = 270,

    SUM_MBIN64S_MBIN32S                        = 271,

    INSTRUCTION_FILLER_272 = 272,

    // short circuit counted branch instructions
    BRANCH_AND_CNT                             = 273,
    BRANCH_OR_CNT                              = 274,

    // Comparison for fixed
    COMP_MBIN32S_MASCII_MASCII_IBIN32S_IBIN32S_IBIN32S = 275,

    INSTRUCTION_FILLER_276 = 276,
    INSTRUCTION_FILLER_277 = 277,
    INSTRUCTION_FILLER_278 = 278,
    INSTRUCTION_FILLER_279 = 279,

    // Unicode support
    COMP_MBIN32S_MATTR4_MATTR4_IBIN32S_IBIN32S_IBIN32S    = 280,
    COMP_MBIN32S_MUNI_MUNI_IBIN32S_IBIN32S_IBIN32S        = 281,
    MOVE_MUNI_MUNI_IBIN32S_IBIN32S                        = 282,

    HASH_MBIN32U_MATTR4_IBIN32S                            = 283,

    // Varchar support
    MOVE_MATTR5_MATTR5                                    = 284,
    COMP_MBIN32S_MATTR5_MATTR5_IBIN32S                    = 285,
    HASH_MBIN32U_MATTR5                                   = 286,
    SUBSTR_MATTR5_MATTR5_MBIN32S_MBIN32S                  = 287,
    GENFUNC_MATTR5_MATTR5_IBIN32S                         = 288,

    // Bignum support
    MOVE_MBIGS_MBIN32S_IBIN32S = 289,
    MOVE_MBIGS_MBIN16S_IBIN32S = 290,
    SUM_MBIN16U_MBIN16U_MBIGS_MBIN64S        = 291,
    SUM_MBIN16U_MBIN16U_MBIGS_MBIGS        = 292,

    // Varchar Unicode support
    COMP_MBIN32S_MUNIV_MUNIV_IBIN32S                      = 293,
    HASH_MBIN32U_MUNIV                                    = 294,

    // Moves
    MOVE_MFLT64_MBIN32S = 295,
    MOVE_MFLT64_MBIN64S = 296,

    // String length functions
    STRLEN_MBIN32U_MATTR5 = 297,
    STRLEN_MBIN32U_MUNIV = 298,

    // Other string functions
    GENFUNC_MATTR5_MATTR5_MBIN32S_IBIN32S = 299,
    POS_MBIN32S_MATTR5_MATTR5 = 300,
    LIKE_MBIN32S_MATTR5_MATTR5_IBIN32S_IBIN32S = 301,

    MOVE_MBIN16U_MBIN8 = 302,

    // Header clause instruction used when writing
    // a data record in disk format(both Packed & Aligned).
    HDR_MPTR32_IBIN32S_IBIN32S_IBIN32S_IBIN32S_IBIN32S = 303,

    NNB_SPECIAL_NULLS_MBIN32S_MATTR3_MATTR3_IBIN32S_IBIN32S    = 304,

    NULLIFZERO_MPTR32_MATTR3_MPTR32_IBIN32S            = 305,

    NNB_MATTR3_IBIN32S                                 = 306,

    // null helper for hash 
    NNB_MBIN32S_MATTR3_IBIN32S_IBIN32S                 = 307,

    REPLACE_NULL_MATTR3_MBIN32S                        = 308,
    REPLACE_NULL_MATTR3_MBIN32U                        = 309,
    REPLACE_NULL_MATTR3_MBIN16S                        = 310,
    REPLACE_NULL_MATTR3_MBIN16U                        = 311,

    SUM_MATTR3_MATTR3_IBIN32S_MBIN32S_MBIN32S          = 312,
    SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN64S          = 313,
    SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN32S          = 314,
    SUM_MATTR3_MATTR3_IBIN32S_MFLT64_MFLT64            = 315,

    UPDATE_ROWLEN3_MATTR5_IBIN32S                      = 316,

    FILL_MEM_BYTES_VARIABLE                            = 317,

    // move for fixed/varchars
    MOVE_MASCII_MATTR5_IBIN32S                         = 318,
    MOVE_MATTR5_MASCII_IBIN32S                         = 319,

    // Indirect branch related instructions
    SWITCH_MBIN32S_MATTR5_MPTR32_IBIN32S_IBIN32S       = 320,
    SWITCH_MBIN32S_MBIN64S_MPTR32_IBIN32S_IBIN32S      = 321,
    BRANCH_INDIRECT_MBIN32S                            = 322,

    CONCAT_MATTR5_MATTR5_MATTR5                        = 323,
    SUM_MATTR3_MATTR3_IBIN32S_MBIGS_MBIGS_IBIN32S      = 324,

    NULL_BYTES                                         = 325,   

    // COPYVARROW_MBIN8_MBIN8_IBIN32S_IBIN32S_IBIN32S_IBIN32S
    // pCode[0], pCode[1] --result
    // pCode[2], pCode[3] -- source
    // pCode[4] --lastVoaOffset
    // pcode[5] --lastVCIndicatorLength
    // pCode[6] --lastNULLIndicatorLength
    // pCode[7] --row length
    COPYVARROW_MBIN8_MBIN8_IBIN32S_IBIN32S_IBIN32S_IBIN32S     = 326,

    CONVVCPTR_MBIN32S_MATTR5_IBIN32S                   = 327,
    CONVVCPTR_MFLT32_MATTR5_IBIN32S                   = 328,
    CONVVCPTR_MATTR5_MATTR5                  = 329,
    CONVVCPTR_MBIN64S_MATTR5_IBIN32S                   = 330,

    // Decoding operations
    DECODE_MASCII_MBIN16S_IBIN32S = 331,
    DECODE_MASCII_MBIN16U_IBIN32S = 332,
    DECODE_MASCII_MBIN32S_IBIN32S = 333,
    DECODE_MASCII_MBIN32U_IBIN32S = 334, 
    DECODE_MASCII_MBIN64S_IBIN32S = 335,
    DECODE_DATETIME               = 336,
    DECODE_NXX                    = 337,
    DECODE_DECS                   = 338,

    // unary operations
    NEGATE_MASCII_MASCII          = 339,

    // tinyint operations
    ENCODE_MASCII_MBIN8S_IBIN32S  = 340,
    ENCODE_MASCII_MBIN8U_IBIN32S  = 341,
    DECODE_MASCII_MBIN8S_IBIN32S  = 342,
    DECODE_MASCII_MBIN8U_IBIN32S  = 343,
    MOVE_MBIN16S_MBIN8S           = 344,
    MOVE_MBIN16U_MBIN8U           = 345,
    MOVE_MBIN32S_MBIN8S           = 346,
    MOVE_MBIN32U_MBIN8U           = 347,
    MOVE_MBIN64S_MBIN8S           = 348,
    MOVE_MBIN64U_MBIN8U           = 349,

    EQ_MBIN32S_MBIN8S_MBIN8S      = 350,
    NE_MBIN32S_MBIN8S_MBIN8S      = 351,
    LT_MBIN32S_MBIN8S_MBIN8S      = 352,
    GT_MBIN32S_MBIN8S_MBIN8S      = 353,
    LE_MBIN32S_MBIN8S_MBIN8S      = 354,
    GE_MBIN32S_MBIN8S_MBIN8S      = 355,

    EQ_MBIN32S_MBIN8U_MBIN8U      = 356,
    NE_MBIN32S_MBIN8U_MBIN8U      = 357,
    LT_MBIN32S_MBIN8U_MBIN8U      = 358,
    GT_MBIN32S_MBIN8U_MBIN8U      = 359,
    LE_MBIN32S_MBIN8U_MBIN8U      = 360,
    GE_MBIN32S_MBIN8U_MBIN8U      = 361,

    // largeint unsigned operations
    MOVE_MBIN64S_MBIN64U          = 362,
    MOVE_MBIN64U_MBIN64S          = 363,
    MOVE_MBIN64U_MBIN64U          = 364,

    RANGE_LOW_S8S64               = 365,
    RANGE_HIGH_S8S64              = 366,
    RANGE_LOW_U8S64               = 367,
    RANGE_HIGH_U8S64              = 368,

    MOVE_MBIN8S_MBIN16S           = 369,
    MOVE_MBIN8U_MBIN16U           = 370,
    MOVE_MBIN8U_MBIN16S           = 371,
    MOVE_MBIN8S_MBIN16U           = 372,

    //***************************************************************
    // Add new PCODE instructions immediately above this comment!!!!!
    //***************************************************************

    // this should have the highest number of the PCODE instructions
    LAST_PCODE_INSTR
  };

  // Following enum is needed by Native Expressions code to resolve
  // an ambiguity with an LLVM object that is also named Instruction.
  typedef enum Instruction enum_PCI ;

  // 64-bit - see notes above at PCodeBinary define.
  typedef Int32 Operand; 

  static PCIType::AddressingMode getMemoryAddressingMode(Int32 datatype);
  static Int32 isMemoryAddressingMode(PCIType::AddressingMode am);
  static Int32 getNumberOperandsForAddressingMode(PCIType::AddressingMode am);
  static Int32 getOperandLengthForAddressingMode(PCIType::AddressingMode am);
  static Int16 getDataTypeForMemoryAddressingMode(PCIType::AddressingMode am);

  // Helper functions for displaying the enumerations
  //
  static const char *operationString(PCIType::Operation op);
  static const char *addressingModeString(PCIType::AddressingMode am);
  
  //buf should be 32 bytes
  static void operandString(Int32 operand, char* buf); 

  static char *instructionString(PCIType::Instruction instr);

};

// PCIAddressingModeList (AML for short)
//
// The AML class is a container class for a set of 0 to 3 addressing modes.
// A convenient constructor is provided for creating an AML with
// up to 3 addressing modes. Accessors for querying the number and modes
// are provided. Once an AML is created, the entries are read-only.
//
// An independent class for a set of addressing modes is used (rather than
// a generic container) for a couple of reasons. Since this code runs at
// execution time (possibly in EID), no standard containers (i.e. RW) can
// be used. Instead of writing a generic template container, I 
// chose to implement the simple AML and OL classes. This also allows simpler
// code and better type checking since the AML and OL classes are geared 
// with convenient constructors and robust type checking.
//
// The AML and OL classes are only meant to be used to simply argument
// passing to the PCI constructors. Nothing more.
//
class PCIAddressingModeList {
public:
  // Constructor
  //
  PCIAddressingModeList(PCIType::AddressingMode am1 =PCIType::AM_NONE,
			PCIType::AddressingMode am2 =PCIType::AM_NONE,
			PCIType::AddressingMode am3 =PCIType::AM_NONE,
			PCIType::AddressingMode am4 =PCIType::AM_NONE,
			PCIType::AddressingMode am5 =PCIType::AM_NONE,
			PCIType::AddressingMode am6 =PCIType::AM_NONE) {
    numberAddressingModes_ = 0;
    if(am1 != PCIType::AM_NONE) numberAddressingModes_++;
    if(am2 != PCIType::AM_NONE) numberAddressingModes_++;
    if(am3 != PCIType::AM_NONE) numberAddressingModes_++;
    if(am4 != PCIType::AM_NONE) numberAddressingModes_++;
    if(am5 != PCIType::AM_NONE) numberAddressingModes_++;
    if(am6 != PCIType::AM_NONE) numberAddressingModes_++;
    addressingMode_[0] = am1;
    addressingMode_[1] = am2;
    addressingMode_[2] = am3;
    addressingMode_[3] = am4;
    addressingMode_[4] = am5;
    addressingMode_[5] = am6;
  };
  
  // Accessors
  //
  Int32 getNumberAddressingModes() const { return numberAddressingModes_; };
  PCIType::AddressingMode getAddressingMode(Int32 mode) const
  { return addressingMode_[mode]; };
  
private:
  // numberAddressingMode_ - Indicates the number of addressing modes that
  // are valid (0-6). Is set automatically by the constructor and is
  // read-only otherwise. Can be queried with the getNumberAddressingModes()
  // method.
  //
  Int32 numberAddressingModes_;

  // addressingMode_ - A vector of the addressing modes. Is set automatically
  // by the constructor and is read-only otherwise. Can be queried with the
  // getAddressingMode() accessor.
  //
  PCIType::AddressingMode addressingMode_[6];

};

// PCIOperandList (OL for short)
//
// The OL class is a container class for a set of 0 to 20 operands.
// Convenient constructors are provided for creating an OL with up to
// 9 operands. Accessors for querying the number and operands are provided.
// Once an OL is created, the entries are read-only.
//
// See PCIAddressingMode comments above. 
//
class PCIOperandList {
public:
  // Constructors
  //
  PCIOperandList() { initList(0); };
  PCIOperandList(PCIType::Operand arg1) { initList(1, arg1); };

  PCIOperandList(Int64 arg1) { 
    numberOperands_ = 2;
    *((Int64*)operand_) = arg1;
  };

  PCIOperandList(Int64 arg1, PCIType::Operand arg2) {
    numberOperands_ = 3;
    *((Int64*)&operand_[0]) = arg1;
    operand_[2] = arg2;
  };
  PCIOperandList(Int64 arg1, Int64 arg2) {
    numberOperands_ = 4;
    *((Int64*)&operand_[0]) = arg1;
    *((Int64*)&operand_[2]) = arg2;
  };

  PCIOperandList(PCIType::Operand arg1, PCIType::Operand arg2) { initList(2, arg1, arg2); };
  PCIOperandList(PCIType::Operand arg1, PCIType::Operand arg2, PCIType::Operand arg3) { 
    initList(3, arg1, arg2, arg3); 
  };
  PCIOperandList(PCIType::Operand arg1, PCIType::Operand arg2, PCIType::Operand arg3, Int32 arg4) { 
    initList(4, arg1, arg2, arg3, arg4); 
  };
  PCIOperandList(PCIType::Operand arg1, PCIType::Operand arg2, PCIType::Operand arg3, Int64 arg4) { 
    initList(5, arg1, arg2, arg3, 0, 0); 
    numberOperands_ = 5;
    *((Int64*)&operand_[3]) = arg4;
  };

  PCIOperandList(PCIType::Operand arg1, PCIType::Operand arg2, PCIType::Operand arg3, PCIType::Operand arg4, PCIType::Operand arg5) 
  { 
    initList(5, arg1, arg2, arg3, arg4, arg5); 
  };
  PCIOperandList(PCIType::Operand arg1, PCIType::Operand arg2, PCIType::Operand arg3, PCIType::Operand arg4, PCIType::Operand arg5, 
			    PCIType::Operand arg6) { 
    initList(6, arg1, arg2, arg3, arg4, arg5, arg6); 
  };
  PCIOperandList(PCIType::Operand arg1, PCIType::Operand arg2, PCIType::Operand arg3, PCIType::Operand arg4, PCIType::Operand arg5, 
			    PCIType::Operand arg6, PCIType::Operand arg7) { 
    initList(7, arg1, arg2, arg3, arg4, arg5, arg6, arg7); 
  };

  PCIOperandList(Int64 arg1, Int64 arg2, PCIType::Operand arg3, PCIType::Operand arg4, PCIType::Operand arg5, 
			    PCIType::Operand arg6, PCIType::Operand arg7, PCIType::Operand arg8) { 
    numberOperands_ = 10;
    *((Int64*)&operand_[0]) = arg1; *((Int64*)&operand_[2]) = arg2;
    operand_[4] = arg3; operand_[5] = arg4; operand_[6] = arg5;
    operand_[7] = arg6; operand_[8] = arg7; operand_[9] = arg8;
  };
  PCIOperandList(PCIType::Operand arg1, PCIType::Operand arg2, PCIType::Operand arg3, PCIType::Operand arg4, PCIType::Operand arg5, 
			    PCIType::Operand arg6, Int64 arg7, Int64 arg8) { 
    numberOperands_ = 10;
    operand_[0] = arg1; operand_[1] = arg2; operand_[2] = arg3;
    operand_[3] = arg4; operand_[4] = arg5; operand_[5] = arg6;
    *((Int64*)&operand_[6]) = arg7; *((Int64*)&operand_[8]) = arg8;
  };

  PCIOperandList(PCIType::Operand arg1, PCIType::Operand arg2, PCIType::Operand arg3, PCIType::Operand arg4, PCIType::Operand arg5, 
			    PCIType::Operand arg6, PCIType::Operand arg7, PCIType::Operand arg8) { 
    initList(8, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); 
  };

  PCIOperandList(Int64 arg1, Int64 arg2, PCIType::Operand arg3, PCIType::Operand arg4, PCIType::Operand arg5, 
			    PCIType::Operand arg6, PCIType::Operand arg7, PCIType::Operand arg8, PCIType::Operand arg9) { 
    numberOperands_ = 11;
    *((Int64*)&operand_[0]) = arg1; *((Int64*)&operand_[2]) = arg2;
    operand_[4] = arg3; operand_[5] = arg4; operand_[6] = arg5;
    operand_[7] = arg6; operand_[8] = arg7; operand_[9] = arg8;
    operand_[10] = arg9;
  }; 

  PCIOperandList(PCIType::Operand arg1, PCIType::Operand arg2, PCIType::Operand arg3, PCIType::Operand arg4, PCIType::Operand arg5, 
			    PCIType::Operand arg6, PCIType::Operand arg7, PCIType::Operand arg8, PCIType::Operand arg9) { 
    initList(9, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
  }; 
  PCIOperandList(PCIType::Operand arg1, PCIType::Operand arg2, PCIType::Operand arg3, PCIType::Operand arg4, PCIType::Operand arg5, 
			    PCIType::Operand arg6, PCIType::Operand arg7, PCIType::Operand arg8, PCIType::Operand arg9, PCIType::Operand arg10){
    initList(10, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10); 
  };

  PCIOperandList(Int64 arg1, Int64 arg2, PCIType::Operand arg3, PCIType::Operand arg4, PCIType::Operand arg5, 
			    PCIType::Operand arg6, PCIType::Operand arg7, PCIType::Operand arg8, PCIType::Operand arg9, PCIType::Operand arg10,
			    PCIType::Operand arg11){
    numberOperands_ = 13;
    *((Int64*)&operand_[0]) = arg1; *((Int64*)&operand_[2]) = arg2;
    operand_[4] = arg3; operand_[5] = arg4; operand_[6] = arg5;
    operand_[7] = arg6; operand_[8] = arg7; operand_[9] = arg8;
    operand_[10] = arg9; operand_[11] = arg10; operand_[12] = arg11;
  };

  PCIOperandList(PCIType::Operand arg1, PCIType::Operand arg2, PCIType::Operand arg3, PCIType::Operand arg4, PCIType::Operand arg5, 
			    PCIType::Operand arg6, PCIType::Operand arg7, PCIType::Operand arg8, PCIType::Operand arg9, PCIType::Operand arg10,
			    PCIType::Operand arg11){
    initList(11, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
	     arg11); 
  };
  PCIOperandList(PCIType::Operand arg1, PCIType::Operand arg2, PCIType::Operand arg3, PCIType::Operand arg4, PCIType::Operand arg5, 
			    PCIType::Operand arg6, PCIType::Operand arg7, PCIType::Operand arg8, PCIType::Operand arg9, PCIType::Operand arg10,
			    PCIType::Operand arg11, PCIType::Operand arg12){
    initList(12, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
	     arg11, arg12); 
  };
  PCIOperandList(PCIType::Operand arg1, PCIType::Operand arg2, PCIType::Operand arg3, PCIType::Operand arg4, PCIType::Operand arg5, 
			    PCIType::Operand arg6, PCIType::Operand arg7, PCIType::Operand arg8, PCIType::Operand arg9, PCIType::Operand arg10,
			    PCIType::Operand arg11, PCIType::Operand arg12, PCIType::Operand arg13){
    initList(13, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
	     arg11, arg12, arg13); 
  };
  
  PCIOperandList(PCIType::Operand arg1, PCIType::Operand arg2, PCIType::Operand arg3, PCIType::Operand arg4, PCIType::Operand arg5, 
			    PCIType::Operand arg6, PCIType::Operand arg7, PCIType::Operand arg8, PCIType::Operand arg9, PCIType::Operand arg10,
                            PCIType::Operand arg11, PCIType::Operand arg12, PCIType::Operand arg13, PCIType::Operand arg14){
    initList(14, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
             arg11, arg12, arg13, arg14); 
  };

  PCIOperandList(PCIType::Operand arg1, PCIType::Operand arg2, PCIType::Operand arg3, PCIType::Operand arg4, PCIType::Operand arg5, 
			    PCIType::Operand arg6, PCIType::Operand arg7, PCIType::Operand arg8, PCIType::Operand arg9, PCIType::Operand arg10,
                            PCIType::Operand arg11, PCIType::Operand arg12, PCIType::Operand arg13, PCIType::Operand arg14, PCIType::Operand arg15){
    initList(15, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
             arg11, arg12, arg13, arg14, arg15); 
  };

  PCIOperandList(PCIType::Operand arg1, PCIType::Operand arg2, PCIType::Operand arg3, PCIType::Operand arg4, PCIType::Operand arg5, 
			    PCIType::Operand arg6, PCIType::Operand arg7, PCIType::Operand arg8, PCIType::Operand arg9, PCIType::Operand arg10,
                            PCIType::Operand arg11, PCIType::Operand arg12, PCIType::Operand arg13, PCIType::Operand arg14,
                            PCIType::Operand arg15, PCIType::Operand arg16){
    initList(16, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
             arg11, arg12, arg13, arg14, arg15, arg16); 
  };

  PCIOperandList(PCIType::Operand arg1, PCIType::Operand arg2, PCIType::Operand arg3, PCIType::Operand arg4, PCIType::Operand arg5, 
			    PCIType::Operand arg6, PCIType::Operand arg7, PCIType::Operand arg8, PCIType::Operand arg9, PCIType::Operand arg10,
                            PCIType::Operand arg11, PCIType::Operand arg12, PCIType::Operand arg13, PCIType::Operand arg14,
                            PCIType::Operand arg15, PCIType::Operand arg16, PCIType::Operand arg17){
    initList(17, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
             arg11, arg12, arg13, arg14, arg15, arg16, arg17); 
  };

  PCIOperandList(PCIType::Operand arg1, PCIType::Operand arg2, PCIType::Operand arg3, PCIType::Operand arg4, PCIType::Operand arg5, 
			    PCIType::Operand arg6, PCIType::Operand arg7, PCIType::Operand arg8, PCIType::Operand arg9, PCIType::Operand arg10,
                            PCIType::Operand arg11, PCIType::Operand arg12, PCIType::Operand arg13, PCIType::Operand arg14,
                            PCIType::Operand arg15, PCIType::Operand arg16, PCIType::Operand arg17, PCIType::Operand arg18){
    initList(18, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
             arg11, arg12, arg13, arg14, arg15, arg16, arg17, arg18); 
  };

  PCIOperandList(PCIType::Operand arg1, PCIType::Operand arg2, PCIType::Operand arg3, PCIType::Operand arg4, PCIType::Operand arg5, 
			    PCIType::Operand arg6, PCIType::Operand arg7, PCIType::Operand arg8, PCIType::Operand arg9, PCIType::Operand arg10,
                            PCIType::Operand arg11, PCIType::Operand arg12, PCIType::Operand arg13, PCIType::Operand arg14, 
                            PCIType::Operand arg15, PCIType::Operand arg16, PCIType::Operand arg17, PCIType::Operand arg18,
                            PCIType::Operand arg19){
    initList(19, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
             arg11, arg12, arg13, arg14, arg15, arg16, arg17, arg18, arg19); 
  };

  PCIOperandList(PCIType::Operand arg1, PCIType::Operand arg2, PCIType::Operand arg3, PCIType::Operand arg4, PCIType::Operand arg5, 
			    PCIType::Operand arg6, PCIType::Operand arg7, PCIType::Operand arg8, PCIType::Operand arg9, PCIType::Operand arg10,
                            PCIType::Operand arg11, PCIType::Operand arg12, PCIType::Operand arg13, PCIType::Operand arg14, 
                            PCIType::Operand arg15, PCIType::Operand arg16, PCIType::Operand arg17, PCIType::Operand arg18,
                            PCIType::Operand arg19, PCIType::Operand arg20){
    initList(20, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
             arg11, arg12, arg13, arg14, arg15, arg16, arg17, arg18, arg19,
             arg20); 
  };

  // Accessors
  //
  Int32 getNumberOperands() { return numberOperands_; };
  PCIType::Operand getOperand(Int32 op) { return operand_[op]; };
    
private:
  // Helper for constuctors
  //
  
  void initList(Int32 numberOperands, 
		PCIType::Operand op1 =0, PCIType::Operand op2 =0, PCIType::Operand op3 =0, PCIType::Operand op4 =0, 
		PCIType::Operand op5 =0, PCIType::Operand op6 =0, PCIType::Operand op7 =0, PCIType::Operand op8 =0, 
		PCIType::Operand op9 =0, PCIType::Operand op10 =0, PCIType::Operand op11 =0, PCIType::Operand op12 =0,
		PCIType::Operand op13 =0, PCIType::Operand op14 =0, PCIType::Operand op15 =0, PCIType::Operand op16 =0,
                PCIType::Operand op17 =0, PCIType::Operand op18 =0, PCIType::Operand op19 =0, PCIType::Operand op20 =0) {
    numberOperands_ = numberOperands;
    operand_[0] = op1; operand_[1] = op2; operand_[2] = op3; 
    operand_[3] = op4; operand_[4] = op5; operand_[5] = op6;
    operand_[6] = op7; operand_[7] = op8; operand_[8] = op9;
    operand_[9] = op10; operand_[10] = op11; operand_[11] = op12;
    operand_[12] = op13; operand_[13] = op14; operand_[14] = op15;
    operand_[15] = op16; operand_[16] = op17; operand_[17] = op18;
    operand_[18] = op19; operand_[19] = op20;
  };

  // numberOperands_ - Indicates the number of operands that are valid (0-6).
  // Is set automatically by the constructor and is read-only otherwise.
  // Can be queried with the getNumberOperands() method.
  //
  Int32 numberOperands_;

  // operand_ - A vector of operands. Is set automatically by the 
  // constructor and is read-only otherwise. Can be queried with the
  // getOperand() accessor.
  //
  PCIType::Operand operand_[20];

};

// PCodeInstruction (PCI for short)
//
// The PCodeInstruction class is used to encapsulate a PCODE operation which
// consists of an operation, 0 to 3 addressing modes, and 0 to 6 operands.
// PCI's are NOT the actual byte codes that are interpreted at execution
// time, but one level higher. It may be helpful to think of PCI's as
// analogous to assembly code and the byte codes that are intpreted at 
// execution time as analogous to machine code.
// 
//
class PCodeInstruction {
public:
  // Constructors
  //
  PCodeInstruction(PCIType::Operation operation) {
    AML aml;
    OL ol;
    init(operation, aml, ol);
  };

  PCodeInstruction(PCIType::Operation operation, AML &aml) {
    OL ol;
    init(operation, aml, ol);
  };

  PCodeInstruction(PCIType::Operation operation, AML &aml, OL &ol) { 
    init(operation, aml, ol);
  }

  PCodeInstruction(const PCodeInstruction *pci) { copy(pci); };

  // Accessors
  //
  Int32 getNumberAddressingModes() { return numberAddressingModes_; };
  Int32 getNumberOperands() { return numberOperands_; };
  void setNumberOperands(Int32 numOperands) { numberOperands_ = numOperands; };

  void replaceAddressingModesAndOperands(AML &aml, OL &ol) {
    numberAddressingModes_ = aml.getNumberAddressingModes();
    Int32 i = 0;

    for(; i<numberAddressingModes_; i++)
      addressingMode_[i] = aml.getAddressingMode(i);

    numberOperands_ = ol.getNumberOperands();

    for(i=0; i<numberOperands_; i++)
      operand_[i] = ol.getOperand(i);
  };

  PCIType::Operation getOperation() { return operation_; };
  PCIType::AddressingMode getAddressingMode(Int32 am) 
    { return addressingMode_[am]; };
  Long getLongOperand(Int32 op) { return *(Long*)&operand_[op]; };
  PCIType::Operand getOperand(Int32 op) { return operand_[op]; };
  void setLongOperand(Int32 op, Long val) { *(Long*)&operand_[op] = val; };
  void setOperand(Int32 op, PCIType::Operand val) { operand_[op] = val; };

  Int32 getCodePosition() const { return codePosition_; };
  void setCodePosition(Int32 codePosition) { codePosition_ = codePosition; };

  Int32 getStackChange();
  Int32 getGeneratedCodeSize();

  // Helpers
  //
  static Int32 temporaryStoreLoadMatch(PCodeInstruction *store, PCodeInstruction *load);
  static Int32 temporaryStoreLoadOverlap(PCodeInstruction *store, PCodeInstruction *load);
  static Int32 replaceUsesOfTarget(PCodeInstruction *store, PCodeInstruction *use, Int32 check);
  Int32 getMemoryOperandLength(PCIType::AddressingMode, Int32 operand);

  // Display
  //
  void print();

private:
  // Private helper for constructors
  //
  
  void init(PCIType::Operation operation, AML &aml, OL &ol) {
    operation_ = operation;
    numberAddressingModes_ = aml.getNumberAddressingModes();
    Int32 i = 0;

    for(; i<numberAddressingModes_; i++)
      addressingMode_[i] = aml.getAddressingMode(i);

    numberOperands_ = ol.getNumberOperands();

    for(i=0; i<numberOperands_; i++)
      operand_[i] = ol.getOperand(i);
  };

  // Private helper for constructors
  //
  
  void copy(const PCodeInstruction *pci) {
    operation_ = pci->operation_;
    numberAddressingModes_ = pci->numberAddressingModes_;
    numberOperands_ = pci->numberOperands_;

    Int32 i = 0;
    for(; i<numberAddressingModes_; i++)
      addressingMode_[i] = pci->addressingMode_[i];

    for(i=0; i<numberOperands_; i++)
      operand_[i] = pci->operand_[i];
  };

  // operation_ - Indicates the type of operation (i.e. load, store, etc).
  // Is set by the constructor and is read-only otherwise. Can be queried
  // using the getOperation() accessor.
  //
  PCIType::Operation operation_;

  // addressingMode_ - Vector of addressing modes for this PCI. Is set
  // by the constructor and is read-only otherwise. Can be queried using
  // the getAddressingMode() accessor.
  //
  PCIType::AddressingMode addressingMode_[6];

  // operand_ - Vector of operands for this PCI. Is set by the constructor.
  // Can be queried using the getOperand() accessor and modified using the
  // setOperand() method. However, only existing operands can be changed.
  //
  PCIType::Operand operand_[20];

  // numberAddressingModes_ - Indicates the number of valid addressing modes.
  // Is set automatically by the constructor and is read-only otherwise. 
  // Can be queried with the getNumberAddressingModes() accessor.
  //
  Int32 numberAddressingModes_;

  // numberOperands_ - Indicates the number of valid operands.
  // Is set automatically by the constructor and is read-only otherwise. 
  // Can be queried with the getNumberOperands() accessor.
  //
  Int32 numberOperands_;

  // codePosition_ - Indicates integer offset of the beggining of this
  // PCI in the generated byte code stream. Can be queried using the
  // getCodePosition() accesor and set using the setCodePosition() method.
  //
  Int32 codePosition_;

};

// A general method to take pointers out of PCode binary stream
// Takes pointer out of PCodeBinary sequences
#define GetPCodeBinaryAsPtr(pcode, idx) (*(Long*)&(pcode[idx]))

#endif  // ExpPCodeInstruction_h
