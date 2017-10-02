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
* File:         ExpPCode.h
* Description:  
*
* Created:      8/25/97
* Language:     C++
*
*
*
****************************************************************************
*/
#ifndef ExpPCode_h
#define ExpPCode_h

// ExpPCode.h (PCodeInstructionMap, PCode)
//

// PCodeInstruction and PCI list classes
//
#include "ExpPCodeInstruction.h"
#include "ExpPCodeList.h"
#include "exp_tuple_desc.h"

// This pcode version number is attached to the beginning of the pcode byte
// code.  Checking of version compatibility is done at fixup time
// Increment this counter everytime pcode instructions are added/deleted/
// modified.  PCode is now stored on disk, so we need to know whether the
// current executables can be used with the pcode stored in previously-
// compiled user programs

#define PC_eyeCatcher "PC 1"
#define PC_eyeCatcherSize 4     // number of BYTES
#define PC_fillerSize 16   //for a total of 16 * sizeof(Int32) bytes filler

//Opcode map defines
#define OPCODE_MAP_FIRSTSIX_BITS  0x000000000000003F

#ifdef _DEBUG
#include <stdio.h>
#endif

struct pc_eye_catcher {
  char name_[4];
};

// Internal forward declarations
//
class PCode;
struct PCodeInstructionMap;

// External forward declarations
//
// Attributes - Pointers to Attributes appear in the function declarations
// of the helper functions that are used to generate PCode for common
// operations (i.e. loadValue, loadNullInd, etc). These functions must access
// the Attributes class to figure out what PCI's to perform.
//
// Space - Pointers to space apprear in the function declarations 
// of the helper functions that are used to generate PCode for common
// operations (i.e. loadValue, loadNullInd, etc). These functions must use
// the Space object to allocate space for the PCI's that are stored with
// clauses and expressions.
//
class ex_clause;
class Attributes;

// RMI reduction typedef's
//
typedef PCodeInstructionMap PCIMap;

#ifndef uLong
  #define uLong ULng32
#endif 

// PCodeInstructionMap
//
// The PCodeInstructionMap class is used to hold pairs of corresponding
// instructions and opcodes. This class purposely operates like a structure. 
// All of the members are public and only convenient constructors are 
// provided.
//
struct  PCodeInstructionMap {
public:
  Int64 instruction;
  Int32 opcode;
  const char * opcodeString;
  Int32 length;
  Int32 numAmodes;
};

// class PCodeSegment
//
// this class contains the actual byte code 

class PCodeSegment : public NAVersionedObject {
public:
  PCodeSegment(PCodeBinary* pcode = 0);

  PCodeBinary *getPCodeBinary() {return (PCodeBinary*)pCodeSegment_.getPointer();}
  void setPCodeBinary(PCodeBinary* pcode) {pCodeSegment_ = pcode;}

  // Takes pointer out of PCodeBinary sequences
  // Now a macro, see exp/ExpPCodeInstruction.h
  // Long getPCodeBinaryAsPtr(PCodeBinary *pcode, Int32 idx)

  // Adds pointer to PCodeBinary sequences and advance idx
  Int32 setPtrAsPCodeBinary(PCodeBinary *pcode, Int32 idx, Long ptr)
    {
      *(Long*)&(pcode[idx]) = ptr;
      return (sizeof(ptr)/sizeof(PCodeBinary));
    }

  unsigned char getClassVersionID();
  void populateImageVersionIDArray() {
    setImageVersionID(0, getClassVersionID());
  }
  // Checks version of pcode byte string, not PCodeSegment object. 
  NABoolean versionOK () { return (str_cmp (eyeCatcher_.name_, PC_eyeCatcher, PC_eyeCatcherSize) == 0); }
  Int32 getPCodeSegmentSize();
  void setPCodeSegmentSize(Int32 size);
  short getClassSize();

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  void replaceAttributesPtr(ex_clause* clauses, Space* space);
  void replaceClauseEvalPtr(ex_clause* oldClause, ex_clause* newClause);

  //change all the addresses embedded in PCode to offsets for packing
  void convAddrToOffsetInPCode(void *space);
  //reverse
  void convOffsetToAddrInPCode(void *base);

  NABoolean containsClauseEval() { return (flags_ & CONTAINS_CLAUSE_EVAL) != 0; };
  void setContainsClauseEval(NABoolean v)
  {
    (v ? flags_ |= CONTAINS_CLAUSE_EVAL : flags_ &= ~CONTAINS_CLAUSE_EVAL);
  }

private:
  enum 
  {
    // if set, indicates that there is atleast one clause_eval
    // pcode instruction. A clause_eval pcode instr is evaluated
    // by called a non-pcode ex_clause::eval method at runtime.
    CONTAINS_CLAUSE_EVAL = 0x0001
  };

  pc_eye_catcher                eyeCatcher_;	// 00-03

  UInt32                         flags_;	// 04-07

  Int32                          filler_[PC_fillerSize];	// 08-71

  Int32                          pCodeSegmentSize_;	// 72-75
  char                           fillerPCodeSegmentSize_[4]; // 76-79

  Int32Ptr                       pCodeSegment_;	// 80-87

};



// PCode
//
// The PCode class is used to hold a list of PCode Instructions (PCIs). The
// PCI's typically represent the code for either a single clause 
// (exp_clause) or an expression (exp_expr) which has a list of clauses. 
// PCode encapsulates access to the PCI's and also a set of helper functions
// which perform common operations such as generating code for loading
// attributes or NULL indicators.  In addition, the PCode class has minimal
// support for storing profile information associated with a list of PCI's.
//
// Most of the helper functions are static member functions and don't need
// direct access to a PCode object. They are included in the PCode class
// in order to provide one-stop shopping and a uniform name space for the
// functions that operate on PCI's.
//
//
class  PCode {
public:

  // Construction/Destruction
  //
  PCode(CollHeap *heap, Space *space);
  ~PCode();

  // Manipulating the PCodeInstruction List
  //
  void setPCIList(PCIList pciList) { pciList_ = pciList; };
  PCIList getPCIList() { return pciList_; };

  // Accessors
  //
  Int32 size();
  static PCIT::Instruction getInstruction(PCI *pci);
  const PCIMap &getMapEntry (Int32 i);
  static Int32 getOpCodeMapElements(Int32 opcode,
                                  PCIT::Operation &operation,
                                  PCIT::AddressingMode am[],
                                  Int32 &numAModes);
  
  static Int32 isInstructionRangeType( PCIT::Instruction instruction);

  // Generating byte code
  // 
  PCodeBinary * generateCodeSegment(Int32 length,
				Int32 *atpCode,
				Int32 *atpIndexCode,
				Int32 *codeSize);

  // Translating PCI's to ID's and vice versa. Each PCI is identified 
  // by a unique ID. Currently, this is simply the address of the PCI.
  // This works because the PCI's are ALWAYS manipulated on non-intrusive
  // lists which refer to the PCI's by pointer and the PCI's are allocated
  // from a Space object which doesn't garbage collect.
  //
  static PCIID getId(PCI *pci) { return (PCIID)pci; };
  static PCI* getPCI(PCIID pciId) { return (PCI*)pciId; };

  // Profiling
  //
  Int32 profileInit();
  Int32 profilePrint();
  Int32 isProfilingOn() { return profileCounts_ != 0; };
  Int32 *profileCounts() { return profileCounts_; };
  Int32 *profileTimes() { return profileTimes_; };

  static Int32 IsBranchOrTarget(PCI *);
  static Int32 IsTemporaryStore(PCI *);
  static Int32 IsTemporaryLoad(PCI *);
  static Int32 IsTemporaryAccess(PCI *);
  static Int32 ComputeTemporaryAccess(PCI *, Int32&, int&, Int32);
  static Int32 IsBranchInstruction(PCI *);
  static Int32 IsTargetInstruction(PCI *);
  static Int32 IsClauseEvalInstruction(PCI *);
  static Int32 *getEmbeddedAddresses(Int32 opcode, Int32 addr[]);
  static Int32 getInstructionLength(PCodeBinary * pcode);
  static const char * getOpcodeString(Int32 opcode);
  
  // Helper functions to generate PCode segments for common operations
  //
  // Loading addresses of attributes (and VC and NULL indicators)
  //
  static PCIList loadVoaAddress(Attributes *attr, CollHeap *heap);
  static PCIList loadVoaValue(Attributes *attr, CollHeap *heap);
  static PCIList loadVCLenIndAddress(Attributes *attr, CollHeap *heap);
  static PCIList loadNullIndAddress(Attributes *attr, CollHeap *heap);
  static PCIList loadAddress(Attributes *attr, CollHeap *heap);
  static PCIList loadAddress(Attributes *attr, uLong offset, CollHeap *heap);

  // Loading attributes (and VC and NULL indicators)
  //
  static PCIList loadOpDataNullBitmapAddress(Attributes *attr, Int32 loc,CollHeap *heap);
  static PCIList loadOpDataNullAddress(Attributes *attr, Int32 loc,CollHeap *heap);
  static PCIList loadOpDataVCAddress(Attributes *attr, Int32 loc,CollHeap *heap);
  static PCIList loadOpDataDataAddress(Attributes *attr, Int32 loc, CollHeap *heap);
  static PCIList loadOpDataAddress(Attributes *attr, Int32 offset, Int32 loc, 
                                   CollHeap *heap);
  static PCIList loadOpDataNullBitmap(Attributes *attr, Int32 loc, CollHeap *heap);
  static PCIList loadOpDataNull(Attributes *attr, Int32 loc, CollHeap *heap);
  static PCIList loadVCLenIndValue(Attributes *attr, CollHeap *heap);
  static PCIList loadNullIndValue(Attributes *attr, CollHeap *heap);
  static PCIList loadValue(Attributes *attr, CollHeap *heap);
  static PCIList loadValue(Attributes *attr, uLong offset, CollHeap *heap,
			   PCIType::AddressingMode =PCIType::AM_NONE);

  // Storing attributes (and VC and NULL indicators)
  //

  // For SQLMX_ALIGNED_FORMAT all offsets are shorts rather than longs.
  static PCIList storeShortVoa(Attributes *attr, CollHeap *heap);
  static PCIList storeShortValue(UInt32      value,
                                 Attributes *attr, 
                                 UInt32      voaOffset,
                                 CollHeap   *heap);

  static PCIList storeVoa(Attributes *attr, CollHeap *heap);
  static PCIList storeVoaValue(Attributes *attr,
			       uLong voaOffset,
			       uLong value,
                               CollHeap *heap,
			       short varOnly = 0);
  static PCIList storeValue(Int32 value, Attributes *attr, 
                            CollHeap *heap);
  static PCIList storeValue(Int32 value, Attributes *attr, 
                            uLong offset, CollHeap *heap);
  static PCIList updateRowLen(Attributes *attr, CollHeap *heap, UInt32 f);

  // Moving attributes (and NULL indicators)
  //
  static PCIList moveValue(Attributes *dst, Attributes *src, CollHeap *heap);
  static PCIList moveVarcharValue(Attributes *dst, Attributes *src, CollHeap *heap);
  static PCIList moveVarcharFixedValue(Attributes *dst, Attributes *src, CollHeap *heap);
  static PCIList moveFixedVarcharValue(Attributes *dst, Attributes *src, CollHeap *heap);
  static PCIList convertVarcharPtrToTarget(Attributes *dst,Attributes *src,CollHeap  *heap);

  static PCIList copyVarRow(Attributes *dst,
                            Attributes *src,
                            UInt32 lastVOAoffset,
                            Int16 lastVcIndicatorLength,
                            Int16 lastNullIndicatorLength,
                            Int16 alignment,
                            CollHeap *heap);

  static PCIID zeroFillNullValue(Attributes *dst, 
                                 PCIList& code,
                                 PCIID notNullBranch,
                                 Int32 genUncondJump = -1);

  static PCIID generateJumpAndBranch(Attributes *dst, 
                                     PCIList& code,
                                     PCIID notNullBranch,
                                     Int32 genUncondJump = -1);
  
  // Testing for NULL or NOT NULL
  //
  static PCIList isNull(Attributes *attrDst, Attributes *attrSrc, 
                        CollHeap *heap);
  static PCIList isNotNull(Attributes *attrDst, Attributes *attrSrc, 
                           CollHeap *heap);

  // Generating code to handle NULL in NULL out operations
  //
  static PCIID nullBranch(ex_clause*, PCIList&, AttributesPtr*);
  static PCIID nullBranchHelper(AttributesPtr *attrs,
                                Attributes *attrA, 
                                Attributes *attrB,
                                PCIList& code);

  static PCIID nullBranchHelperForComp(AttributesPtr *attrs,
                                       Attributes *attrA, 
                                       Attributes *attrB,
                                       NABoolean isSpecialNulls,
                                       PCIList& code);
  
  static PCIID nullBranchHelperForHash(AttributesPtr *attrs,
                                       Attributes *attrA, 
                                       Attributes *attrB,
                                       PCIList& code);
  
  // Pre/Post generate startup and cleanup PCI's
  //
  static void preClausePCI(ex_clause *clause, PCIList& code);
  static void postClausePCI(ex_clause *clause, PCIList& code);

  // Print PCODE (can only be used in Master EXE)
  //
  static void print(PCIList pciList);

  // Used by SHOWPLAN to display PCI's
  //
  static void displayContents(PCIList pciList, Space *space);
  static void displayContents(PCodeBinary* pCode, Space *space);

  // for debug
  static Int32 dumpContents(PCIList pciList, char *buf, Int32 bufLen);
  static void dumpContents(PCodeBinary* pCode, char *buf, Int32 bufLen);
 
private:
  // pciList_ - The list of PCI's for this PCode object.
  //
  PCIList pciList_;

  // space_ - The allocator to use when operating with the PCode object.
  //
  CollHeap *heap_;
  Space *space_;

  Int32 *profileCounts_, *profileTimes_;

};

// For null processing, this struct stores the three tuple formats 
// as chars. It also computes the size of the pcode. Size of this 
// struct has to be Int32 so that it can be passed as a parameter. 
typedef struct PCodeTupleFormats {
  char op1Fmt_;
  char op2Fmt_;
  char op3Fmt_;
  char size_;
  // do not add anything here. Size needs to be 32 bits. 

  // constructors
  PCodeTupleFormats(char op1Fmt, char op2Fmt, char op3Fmt) {
    op1Fmt_ = op1Fmt;
    op2Fmt_ = op2Fmt;
    op3Fmt_ = op3Fmt;
    // 3 operands x (atp, offset, nullbitIndx) + tupleFormat + branch slot
    size_ = (3 * 3) + 1 + 1;   
   }
  PCodeTupleFormats(char op1Fmt, char op2Fmt) {
    op1Fmt_ = op1Fmt;
    op2Fmt_ = op2Fmt;
    op3Fmt_ = NULL;
    // 2 operands x (atp, offset, nullbitIndx) + tupleFormat + branch slot
    size_ = (2 * 3) + 1 + 1;
  }

} PCodeTupleFormats;

// The runtime code maps the address of a pcode to this struct
// to enable direct symbolic access to its fields. The EXPAND_ 
// macros are kept here to ensure they are in sync with the fields defined
// in this struct. 
typedef struct{
  PCodeTupleFormats fmt_;
  Int32 op1NullBitIndex_;
  Int32 op2NullBitIndex_;
  Int32 op3NullBitIndex_;
  
#define EXPAND_PCODEATTRNULL3(tpf, op1, op2, op3)  \
  tpf, op1->getNullBitIndex(), op2->getNullBitIndex(), op3->getNullBitIndex()

#define EXPAND_PCODEATTRNULL2(tpf, op1, op2)  \
  tpf, op1->getNullBitIndex(), op2->getNullBitIndex()

}PCodeAttrNull;

#endif
