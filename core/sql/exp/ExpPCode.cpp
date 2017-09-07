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
* File:         ExpPCode.cpp
* Description:  
*
* Created:      8/25/97
* Language:     C++
*
*
*
****************************************************************************
*/

#include "Platform.h"


#include "exp_stdh.h"
#include "str.h"
#include "exp_datetime.h"
#include "exp_expr.h"
#include "exp_function.h"
#include "ExpPCode.h"
#include "exp_tuple_desc.h"

// Uncomment the line below to debug new PCode instructions.
// #define SQLMX_DEBUG_PCIT

PCodeSegment::PCodeSegment(PCodeBinary* pcode) : NAVersionedObject() {
  str_cpy_all((char*)&eyeCatcher_, PC_eyeCatcher, PC_eyeCatcherSize);
  flags_ = 0;
  pCodeSegmentSize_ = 0;
  setPCodeBinary(pcode);
}


unsigned char PCodeSegment::getClassVersionID() {
  return 1;
}


Long PCodeSegment::pack(void *space) {
  convAddrToOffsetInPCode(space);
  pCodeSegment_.pack(space);
  return NAVersionedObject::pack(space);
}

Lng32 PCodeSegment::unpack(void *base, void * reallocator) {
  if (pCodeSegment_.unpack(base)) return -1;
  convOffsetToAddrInPCode(base);
  return NAVersionedObject::unpack(base, reallocator);
}

Int32 PCodeSegment::getPCodeSegmentSize() {
  return pCodeSegmentSize_;
}

void PCodeSegment::setPCodeSegmentSize(Int32 size) {
  pCodeSegmentSize_ = size;
}


short PCodeSegment::getClassSize() {
  Int32 trueSize = sizeof(*this) + getPCodeSegmentSize();
  return( (short)(trueSize + (trueSize % 8)) );
}

// PCodeSegment::replaceAttributesPtr
//
// Find all attributes pointers in the pcode segment which aren't referenced in
// the passed in list of clauses.  Copy these attributes pointers into the
// passed in space object and replace the old attributes pointer in the pcode
// segment with the new pointer
//
void PCodeSegment::replaceAttributesPtr(ex_clause* clauses, Space* space)
{
  Int32 addrBuf[6];

  PCodeBinary *pcode = getPCodeBinary();
  if (!pcode)
    return;

  Int32 length = *(pcode++);
  pcode += (2 * length);

  // Loop through all the PCODE instructions
  while (pcode[0] != PCIT::END) {

    // Determine if we're dealing with a PCODE instruction with an embedded addr
    Int32* addrs= PCode::getEmbeddedAddresses(pcode[0], addrBuf);

    // Only do more work if an embedded address was found *and* it's not a
    // clause pointer (since that's handled elsewhere).
    if ((addrs[0] != -1) &&
        (pcode[0] != PCIT::CLAUSE_BRANCH) &&
        (pcode[0] != PCIT::CLAUSE_EVAL))
    {
      // Go through each attribute position in pcode
      while (*addrs != -1) {
        // Assume attributes pointer is not referenced in clauses
        NABoolean found = FALSE;

        Attributes* attr = (Attributes*)GetPCodeBinaryAsPtr(pcode, *addrs);

        // It's possible that the attributes pointer is NULL.  In this case
        // there's nothing to look for and copy over.
        if (attr == NULL){
          addrs++;
          continue;
        }

        // Search the operands of all clauses to find any matching pairs.
        for (ex_clause* clause = clauses;
             clause;
             clause = clause->getNextClause())
        {
          Int32 numOperands = clause->getNumOperands();
          for (Int32 i=0; i < numOperands; i++) {
            Attributes* attrOld = clause->getOperand(i);

            // If the attribute in the pcode segment matches an attributes
            // pointer in the clause, then it was copied over correctly and
            // we don't need to do anything special.
            if (attrOld == attr) {
              found = TRUE;
              break;
            }
          }

          // No need to look further - we found it
          if (found)
            break;
        }

        // If the attributes pointer in the pcode segment was not found in the
        // operands list for all the clauses, that means the attributes pointer
        // needs to be copied and reset in the pcode segment
        if (!found) {
          Int32 size = attr->getClassSize();

          Attributes* attrNew = (Attributes*) new(space) char[size];
          memcpy((char*)attrNew, (char*)attr, size);

          // Update pcode segment with new attributes pointer
          setPtrAsPCodeBinary(pcode, *addrs, (Long)attrNew);
          // pcode[*addrs] = (PCodeBinary)attrNew;
        }

        // Advance to the next attribute postion.
        addrs++;
      }
    }
    pcode += PCode::getInstructionLength(pcode);
  }
}

// Locate the clause pointer "oldClause" in the pcode segment and replace it
// with the clause "newClause".
void PCodeSegment::replaceClauseEvalPtr(ex_clause* oldClause,
                                        ex_clause* newClause)
{
  PCodeBinary *pcode = getPCodeBinary();
  if (!pcode)
    return;

  Int32 length = *(pcode++);
  pcode += (2 * length);

  while (pcode[0] != PCIT::END) {
    if (pcode[0] == PCIT::CLAUSE_BRANCH) {
      if ((ex_clause*)*(Long*)&pcode[1 + PCODEBINARIES_PER_PTR] == oldClause) {
        setPtrAsPCodeBinary(pcode, 1 + PCODEBINARIES_PER_PTR, (Long) newClause);
        // pcode[2] = (PCodeBinary)newClause;
      }
    }
    else if (pcode[0] == PCIT::CLAUSE_EVAL) {
      if ((ex_clause*)*(Long*)&pcode[1] == oldClause) {
        setPtrAsPCodeBinary(pcode, 1, (Long) newClause);
        // pcode[1] = (PCodeBinary)newClause;
      }
    }
    pcode += PCode::getInstructionLength(pcode);
  }
}

// -----------------------------------------------------------------------
// The following 2 methods change all the addresses embedded in the pcode
// byte code to offsets (and vice versa) for packing/unpacking
//
// convAddrToOffsetInPCode uses the pcode eval driver in ex_expr::eval
// as a model.  This method traverses through the whole entire byte code
// segment and finds all addresses for the ex_clause object and translates
// the address to offset.
//
// convOffsetToAddrInPCode translates the offsets to addresses
// -----------------------------------------------------------------------
void PCodeSegment::convAddrToOffsetInPCode(void * space) {
  PCodeBinary *pcode = getPCodeBinary();
  if (!pcode)
    return;

  Int32 length = *(pcode++);
  pcode += (2 * length);

  while (pcode[0] != PCIT::END) {
    Int32 addrBuf[6];
    Int32 *addrs = PCode::getEmbeddedAddresses(pcode[0], addrBuf);
    for(Int32 i = 0; addrs[i] > 0; i++) {
      if ( (char*)pcode[addrs[i]] != NULL )
        *(Long*)&(pcode[addrs[i]]) =
                ((Space*)space)->convertToOffset((char*)*(Long*)&(pcode[addrs[i]]));
    }
    pcode += PCode::getInstructionLength(pcode);
  }
}

void PCodeSegment::convOffsetToAddrInPCode(void* base) {
  PCodeBinary *pcode = getPCodeBinary();
  if (!pcode)
    return;

  // skip over the ATP's
  Int32 length = *(pcode++);
  pcode += (2 * length);

  while (pcode[0] != PCIT::END) {
    Int32 addrBuf[6];
    Int32 *addrs = PCode::getEmbeddedAddresses(pcode[0], addrBuf);
    for(Int32 i = 0; addrs[i] > 0; i++) {
      // Zero offset is a null pointer ...
      if ( pcode[addrs[i]] != 0 )
        setPtrAsPCodeBinary(pcode, addrs[i],
                            (Long)(base) - GetPCodeBinaryAsPtr(pcode, addrs[i]));
    }
    pcode += PCode::getInstructionLength(pcode);
  }
}

// Determine if the PCODE operation is a branch or target
//
Int32 PCode::IsBranchInstruction(PCI *pci) 
{
  switch(pci->getOperation())
    {
    case PCIT::Op_BRANCH:
    case PCIT::Op_CLAUSE_BRANCH:
    case PCIT::Op_NOT_NULL_BRANCH:
    case PCIT::Op_NOT_NULL_BRANCH_COMP:
    case PCIT::Op_NOT_NULL_BITMAP_BRANCH:
    case PCIT::Op_BRANCH_AND:
    case PCIT::Op_BRANCH_OR:
      return 1;
    }
  return 0;
}

// Determine if the PCODE operation is a target.
//
Int32 PCode::IsTargetInstruction(PCI *pci) 
{
  switch(pci->getOperation())
    {
    case PCIT::Op_TARGET:
      return 1;
    }
  return 0;
}

// Determine if the PCODE operation is a CLAUSE_EVAL.
//
Int32 PCode::IsClauseEvalInstruction(PCI *pci) 
{
  switch(pci->getOperation())
    {
    case PCIT::Op_CLAUSE_EVAL:
    case PCIT::Op_CLAUSE_BRANCH:
      return 1;
    }
  return 0;
}

Int32 PCode::IsBranchOrTarget(PCI *pci) 
{
  switch(pci->getOperation())
    {
    case PCIT::Op_BRANCH:
    case PCIT::Op_CLAUSE_BRANCH:
    case PCIT::Op_BRANCH_AND:
    case PCIT::Op_BRANCH_OR:
    case PCIT::Op_NOT_NULL_BRANCH:
    case PCIT::Op_NOT_NULL_BRANCH_COMP:
    case PCIT::Op_NOT_NULL_BITMAP_BRANCH:
    case PCIT::Op_TARGET:
      return 1;
    }
  return 0;
}

//Determine if PCODE operation is a MOVE to a temporary location
//assumes Atp = 0 and Atp Index = 1  is the temporary space.

Int32 PCode::IsTemporaryStore(PCI *pci)
{
  if(((pci->getOperand(0) == 0) &&
      (pci->getOperand(1) == 1) &&
      (pci->getOperation() == PCIT::Op_MOVE) &&
      PCode::getInstruction(pci) == PCIT::MOVE_MBIN8_MBIN8_IBIN32S))
    return 1;

  return 0;
}

Int32 PCode::IsTemporaryLoad(PCI *pci)
{
  if((pci->getOperand(3) == 0) && 
     (pci->getOperand(4) == 1) &&
     (pci->getOperation() == PCIT::Op_MOVE) &&
     (PCode::getInstruction(pci) == PCIT::MOVE_MBIN8_MBIN8_IBIN32S))
    return 1;

  return 0;
}

// Determine if the PCODE operation results in a access to/from temporary space
//
Int32 PCode::IsTemporaryAccess(PCI *pci)
{
  PCIT::AddressingMode am;
  for(Int32 i=0,op=0; 
      i<pci->getNumberAddressingModes();
      i++,op+=PCIT::getNumberOperandsForAddressingMode(am))
      {
	    am = pci->getAddressingMode(i);
	    if(!PCIT::isMemoryAddressingMode(am)) 
              continue;
	    if((pci->getOperand(op) == 0)&&(pci->getOperand(op+1) == 1))
              return 1;	    
      }

  return 0;
}

Int32 PCI::temporaryStoreLoadMatch(PCI *store, PCI *load) {
  if(!PCode::IsTemporaryStore(store)) return 0;
  if(!PCode::IsTemporaryLoad(load)) return 0;

  if((store->getOperand(6) == load->getOperand(6))&&
      (store->getOperand(2) == load->getOperand(5)))
      return 1;

  return 0;
}

Int32 PCI::temporaryStoreLoadOverlap(PCI *store, PCI *load) {

  if(PCode::IsTemporaryAccess(load) == 0) 
    return 0;
  Int32 storeStart, storeLength, storeEnd, loadStart, loadLength, loadEnd;
  storeStart = store->getOperand(2);
  storeLength = store->getOperand(6);
  storeEnd = storeStart+storeLength;
  PCIT::AddressingMode am;
  for(Int32 i=0,op=0; 
      i<load->getNumberAddressingModes();
      i++,op+=PCIT::getNumberOperandsForAddressingMode(am))
      {
	    am = load->getAddressingMode(i);
	    if(!PCIT::isMemoryAddressingMode(am)) 
              continue;
            if(!((load->getOperand(op) == 0)&&(load->getOperand(op+1) == 1))) 
              continue;
            loadStart = load->getOperand(op+2);
            loadLength = load->getMemoryOperandLength(am, op);
            loadEnd = loadStart+loadLength;
            if(((loadStart >= storeStart) && (loadStart < storeEnd)) ||
            ((loadEnd > storeStart) && (loadEnd <= storeEnd)) ||
            ((loadStart < storeStart) && (storeEnd < loadEnd)))
               return 1;
      }
  return 0;
}

Int32 PCI::replaceUsesOfTarget(PCI *store, PCI *use, Int32 check) {

  // Get store PCI target info
  Int32 storTgtAtp = store->getOperand(0);
  Int32 storTgtAtpIndex = store->getOperand(1);
  Int32 storTgtStart = store->getOperand(2);
  Int32 storTgtLength = store->getOperand(6);
  Int32 storTgtEnd = storTgtStart+storTgtLength;

  // Get store PCI source info
  Int32 storSrcAtp = store->getOperand(3);
  Int32 storSrcAtpIndex = store->getOperand(4);
  Int32 storSrcStart = store->getOperand(5);

  PCIT::AddressingMode am;

  Int32 retVal = 0;  // Assume everything OK.
  
  for(Int32 i=0,op=0; i < use->getNumberAddressingModes(); i++)
    {
      am = use->getAddressingMode(i);
      if(PCIT::isMemoryAddressingMode(am)) {
        Int32 useAtp = use->getOperand(op);
        Int32 useAtpIndex = use->getOperand(op+1);
        Int32 useStart = use->getOperand(op+2);
        Int32 useLength = use->getMemoryOperandLength(am, op);
        Int32 useEnd = useStart+useLength;

        if(storTgtAtp == useAtp &&
           storTgtAtpIndex == useAtpIndex) {


          if(useLength == 0 && 
             use->getOperation() == PCIT::Op_OPDATA &&
             storTgtStart >= useStart)
            {
              // OPDATA instruction do not record the length of the
              // data they point to.  Assume that it is the same
              // length as the source.
              //
              useEnd = storTgtEnd;
            }


          if(storTgtStart >= useEnd ||
             storTgtEnd <= useStart) {
            // No overlap, do not replace but continue to check
            // next operand of the use PCI
            
          } else if (storTgtStart <= useStart &&
                     storTgtEnd >= useEnd &&
                     (i != 0 || 
                      use->getOperation() == PCIT::Op_RANGE_LOW ||
                      use->getOperation() == PCIT::Op_RANGE_HIGH) &&
                     use->getOperation() != PCIT::Op_OPDATA) {

            // use is completely within store PCI target place,
            // And this is not the first operand (result).
            // replace with store source
            //
            if(!check) {
              use->setOperand(op, storSrcAtp);
              use->setOperand(op+1, storSrcAtpIndex );
              use->setOperand(op+2, storSrcStart + (useStart - storTgtStart));
            }
            // else, just check, will return later as replaciable (retVal == 0)

            // continue to check the next operand of the use PCI.
          } else {
            // The use is within the target location of store PCI but we
            // can not eliminate this store PCI because:
            // 1). the use PCI is Op_OPDATA for CLAUSE_EVAL, or
            // 2). it is the storage place of the use PCI, such as
            //    temp = temp + ... and the store PCI is to initialize temp

            // Now, there are 3 cases that we either quit, or continue:
            // A). if we do checking and are looking at the target of use
            //     PCI, i.e. i is 0 or case 2, then return 1, as we can
            //     not eliminate the store PCI anyway;
            // B). if we do checking and are looking at a source operand
            //     of use PCI, then continue to check other operands;
            // C). if we do replacement, then we can not eliminate the
            //     store PCI because we can not replace this operand of the
            //     use PCI. So set retVal to 1, but continue as we may be
            //     able to replace other operands of the use PCI.
            if(check && i == 0) {
              return 1;    // don't even try to replace later
            } else if (!check) {
              retVal = 1;
            }
          }
                              
        }
        // else ATP and ATP index do not match, do not replace.
        //

      }

      op += PCIT::getNumberOperandsForAddressingMode(am);
    }

  return retVal;
}


PCode::PCode(CollHeap *heap, Space* space) 
  : pciList_(heap), heap_(heap), space_(space), profileCounts_(0), 
    profileTimes_(0) { 
};

PCode::~PCode() {
  profilePrint(); 
};

Int32 PCode::size() {
  PCIListIter iter(pciList_);
  Int32 size = 0;

  for(PCI *pci = iter.first(); pci; pci = iter.next())
    size += pci->getGeneratedCodeSize();

  return size;
}

#define AM_LENGTH(a) ((  a == PCIT::MATTR6) ? 6 :    \
                      (  a == PCIT::MUNIV            \
                      || a == PCIT::MATTR5) ? 5 :    \
                      (  a == PCIT::MATTR4           \
                      || a == PCIT::IATTR4) ? 4 :    \
                      (  a == PCIT::IATTR3           \
                      || a == PCIT::MATTR3) ? 3 :    \
                      (  a == PCIT::IATTR2           \
                      || a == PCIT::MBIN8            \
                      || a == PCIT::MBIN8S           \
                      || a == PCIT::MBIN8U           \
                      || a == PCIT::MBIN16U          \
                      || a == PCIT::MBIN16S          \
                      || a == PCIT::MBIN32U          \
                      || a == PCIT::MBIN32S          \
                      || a == PCIT::MBIN64U          \
                      || a == PCIT::MBIN64S          \
                      || a == PCIT::MPTR32           \
                      || a == PCIT::MASCII           \
                      || a == PCIT::MUNI             \
                      || a == PCIT::MBIGS            \
                      || a == PCIT::MBIGU            \
                      || a == PCIT::MDECS            \
                      || a == PCIT::MDECU            \
                      || a == PCIT::IBIN64S          \
                      || a == PCIT::MFLT32           \
                      || a == PCIT::MFLT64) ? 2 :    \
                      (  a == PCIT::IBIN8U           \
                      || a == PCIT::IBIN16U          \
                      || a == PCIT::IBIN32S) ? 1 :   \
                      (  a == PCIT::IPTR) ? PCODEBINARIES_PER_PTR : \
                      0)

// Local MACROs for constructing the instruction map
//
#define I6(inst,am1,am2,am3,am4,am5,am6,op) {(((Int64)PCIT::inst<<36)|((Int64)PCIT::am1<<30)|((Int64)PCIT::am2<<24)|((Int64)PCIT::am3<<18)|((Int64)PCIT::am4<<12)|((Int64)PCIT::am5<<6)|((Int64)PCIT::am6)),PCIT::op,"" # op "",AM_LENGTH(PCIT::am1)+AM_LENGTH(PCIT::am2)+AM_LENGTH(PCIT::am3)+AM_LENGTH(PCIT::am4)+AM_LENGTH(PCIT::am5)+AM_LENGTH(PCIT::am6)+1,6}
#define I5(inst,am1,am2,am3,am4,am5,op) {(((Int64)PCIT::inst<<36)|((Int64)PCIT::am1<<30)|((Int64)PCIT::am2<<24)|((Int64)PCIT::am3<<18)|((Int64)PCIT::am4<<12)|((Int64)PCIT::am5<<6)),PCIT::op,"" # op "",AM_LENGTH(PCIT::am1)+AM_LENGTH(PCIT::am2)+AM_LENGTH(PCIT::am3)+AM_LENGTH(PCIT::am4)+AM_LENGTH(PCIT::am5)+1,5}
#define I4(inst,am1,am2,am3,am4,op) {(((Int64)PCIT::inst<<36)|((Int64)PCIT::am1<<30)|((Int64)PCIT::am2<<24)|((Int64)PCIT::am3<<18)|((Int64)PCIT::am4<<12)),PCIT::op,"" # op "",AM_LENGTH(PCIT::am1)+AM_LENGTH(PCIT::am2)+AM_LENGTH(PCIT::am3)+AM_LENGTH(PCIT::am4)+1,4}
#define I3(inst,am1,am2,am3,op) {(((Int64)PCIT::inst<<36)|((Int64)PCIT::am1<<30)|((Int64)PCIT::am2<<24)|((Int64)PCIT::am3<<18)),PCIT::op,"" # op "",AM_LENGTH(PCIT::am1)+AM_LENGTH(PCIT::am2)+AM_LENGTH(PCIT::am3)+1,3}
#define I2(inst,am1,am2, op) {(((Int64)PCIT::inst<<36)|((Int64)PCIT::am1<<30)|((Int64)PCIT::am2<<24)),PCIT::op, "" # op "",AM_LENGTH(PCIT::am1)+AM_LENGTH(PCIT::am2)+1,2}
#define I1(inst,am1,op) {(((Int64)PCIT::inst<<36)|((Int64)PCIT::am1<<30)),PCIT::op,"" # op "",AM_LENGTH(PCIT::am1)+1,1}
#define I0(inst,op) {(((Int64)PCIT::inst<<36)),PCIT::op,"" # op "",1,0}
#define IopCodeNotInUse(op) {0,op,"opCodeNotInUse",1,0}
  // create the instruction map array once per process
  // the size of the (# of operations) * (# of addressing modes)
  static const PCIMap opcodeMap[] = {
  I2(Op_OPDATA,MPTR32,IBIN32S,OPDATA_MPTR32_IBIN32S),       // Instruction 0
  I2(Op_OPDATA,MBIN16U,IBIN32S,OPDATA_MBIN16U_IBIN32S),     // Instruction 1

  I1(Op_MOVE,MBIN32S,MOVE_MBIN32S),                         // Instruction 2
  I2(Op_MOVE,MBIN32S,IBIN32S,MOVE_MBIN32S_IBIN32S),         // Instruction 3
  I3(Op_MOVE,MBIN8,MBIN8,IBIN32S,MOVE_MBIN8_MBIN8_IBIN32S), // Instruction 4
  I2(Op_MOVE,MBIN32U,MBIN16U,MOVE_MBIN32U_MBIN16U),         // Instruction 5
  I2(Op_MOVE,MBIN32S,MBIN16U,MOVE_MBIN32S_MBIN16U),         // Instruction 6
  I2(Op_MOVE,MBIN64S,MBIN16U,MOVE_MBIN64S_MBIN16U),         // Instruction 7
  I2(Op_MOVE,MBIN32U,MBIN16S,MOVE_MBIN32U_MBIN16S),         // Instruction 8
  I2(Op_MOVE,MBIN32S,MBIN16S,MOVE_MBIN32S_MBIN16S),         // Instruction 9
  I2(Op_MOVE,MBIN64S,MBIN16S,MOVE_MBIN64S_MBIN16S),         // Instruction 10
  I2(Op_MOVE,MBIN64S,MBIN32U,MOVE_MBIN64S_MBIN32U),         // Instruction 11
  I2(Op_MOVE,MBIN64S,MBIN32S,MOVE_MBIN64S_MBIN32S),         // Instruction 12
  I3(Op_MOVE,MBIN64S,MDECS,IBIN32S,MOVE_MBIN64S_MDECS_IBIN32S),  // Instruction 13
  I3(Op_MOVE,MBIN64S,MDECU,IBIN32S,MOVE_MBIN64S_MDECU_IBIN32S),  // Instruction 14

  IopCodeNotInUse(15),                                      // Instruction 15 not in use

  I1(Op_NULL,MBIN16U,NULL_MBIN16U),                         // Instruction 16
  I2(Op_NULL,MBIN16U,IBIN16U,NULL_MBIN16U_IBIN16U),         // Instruction 17
  I3(Op_NULL,MBIN32S,MBIN16U,IBIN32S,NULL_TEST_MBIN32S_MBIN16U_IBIN32S),  // Instruction 18
  I3(Op_NOT_NULL_BRANCH,MBIN16S,MBIN16S,IBIN32S,NOT_NULL_BRANCH_MBIN16S_MBIN16S_IBIN32S),  // Instruction 19
  I4(Op_NOT_NULL_BRANCH,MBIN16S,MBIN16S,MBIN16S,IBIN32S,NOT_NULL_BRANCH_MBIN16S_MBIN16S_MBIN16S_IBIN32S),  // Instruction 20
  I1(Op_NULL_VIOLATION,MBIN16U,NULL_VIOLATION_MBIN16U),     // Instruction 21
  I2(Op_NULL_VIOLATION,MBIN16U,MBIN16U,NULL_VIOLATION_MBIN16U_MBIN16U),  // Instruction 22

  IopCodeNotInUse(23), 
  IopCodeNotInUse(24), 
  IopCodeNotInUse(25), 
  IopCodeNotInUse(26), 
  IopCodeNotInUse(27), 
  IopCodeNotInUse(28),                                      // Instruction 28 not in use
  IopCodeNotInUse(29),                                      // Instruction 29 not in use
  IopCodeNotInUse(30),                                      // Instruction 30 not in use

  I2(Op_ZERO,MBIN32S,MBIN32U,ZERO_MBIN32S_MBIN32U),         // Instruction 31
  I2(Op_NOTZERO,MBIN32S,MBIN32U,NOTZERO_MBIN32S_MBIN32U),   // Instruction 32

  I3(Op_EQ,MBIN32S,MBIN16S,MBIN16S,EQ_MBIN32S_MBIN16S_MBIN16S),  // Instruction 33
  I3(Op_EQ,MBIN32S,MBIN16S,MBIN32S,EQ_MBIN32S_MBIN16S_MBIN32S),  // Instruction 34

  IopCodeNotInUse(35), 

  I3(Op_EQ,MBIN32S,MBIN32S,MBIN32S,EQ_MBIN32S_MBIN32S_MBIN32S),  // Instruction 36
  I3(Op_EQ,MBIN32S,MBIN16U,MBIN16U,EQ_MBIN32S_MBIN16U_MBIN16U),  // Instruction 37
  I3(Op_EQ,MBIN32S,MBIN16U,MBIN32U,EQ_MBIN32S_MBIN16U_MBIN32U),  // Instruction 38

  IopCodeNotInUse(39),

  I3(Op_EQ,MBIN32S,MBIN32U,MBIN32U,EQ_MBIN32S_MBIN32U_MBIN32U),  // Instruction 40
  I3(Op_EQ,MBIN32S,MBIN16S,MBIN32U,EQ_MBIN32S_MBIN16S_MBIN32U),  // Instruction 41

  IopCodeNotInUse(42), 

  I3(Op_LT,MBIN32S,MBIN16S,MBIN16S,LT_MBIN32S_MBIN16S_MBIN16S),  // Instruction 43
  I3(Op_LT,MBIN32S,MBIN16S,MBIN32S,LT_MBIN32S_MBIN16S_MBIN32S),  // Instruction 44

  IopCodeNotInUse(45),

  I3(Op_LT,MBIN32S,MBIN32S,MBIN32S,LT_MBIN32S_MBIN32S_MBIN32S),  // Instruction 46
  I3(Op_LT,MBIN32S,MBIN16U,MBIN16U,LT_MBIN32S_MBIN16U_MBIN16U),  // Instruction 47
  I3(Op_LT,MBIN32S,MBIN16U,MBIN32U,LT_MBIN32S_MBIN16U_MBIN32U),  // Instruction 48

  IopCodeNotInUse(49),

  I3(Op_LT,MBIN32S,MBIN32U,MBIN32U,LT_MBIN32S_MBIN32U_MBIN32U),  // Instruction 50
  I3(Op_LT,MBIN32S,MBIN16S,MBIN32U,LT_MBIN32S_MBIN16S_MBIN32U),  // Instruction 51

  IopCodeNotInUse(52),

  I3(Op_GT,MBIN32S,MBIN16S,MBIN16S,GT_MBIN32S_MBIN16S_MBIN16S),  // Instruction 53
  I3(Op_GT,MBIN32S,MBIN16S,MBIN32S,GT_MBIN32S_MBIN16S_MBIN32S),  // Instruction 54

  IopCodeNotInUse(55),

  I3(Op_GT,MBIN32S,MBIN32S,MBIN32S,GT_MBIN32S_MBIN32S_MBIN32S),  // Instruction 56
  I3(Op_GT,MBIN32S,MBIN16U,MBIN16U,GT_MBIN32S_MBIN16U_MBIN16U),  // Instruction 57
  I3(Op_GT,MBIN32S,MBIN16U,MBIN32U,GT_MBIN32S_MBIN16U_MBIN32U),  // Instruction 58

  IopCodeNotInUse(59),

  I3(Op_GT,MBIN32S,MBIN32U,MBIN32U,GT_MBIN32S_MBIN32U_MBIN32U),  // Instruction 60
  I3(Op_GT,MBIN32S,MBIN16S,MBIN32U,GT_MBIN32S_MBIN16S_MBIN32U),  // Instruction 61

  IopCodeNotInUse(62),

  I3(Op_AND,MBIN32S,MBIN32S,MBIN32S,AND_MBIN32S_MBIN32S_MBIN32S),  // Instruction 63
  I3(Op_OR,MBIN32S,MBIN32S,MBIN32S,OR_MBIN32S_MBIN32S_MBIN32S),  // Instruction 64

  I3(Op_ADD,MBIN16S,MBIN16S,MBIN16S,ADD_MBIN16S_MBIN16S_MBIN16S),  // Instruction 65
  I3(Op_ADD,MBIN32S,MBIN32S,MBIN32S,ADD_MBIN32S_MBIN32S_MBIN32S),  // Instruction 66
  I3(Op_ADD,MBIN64S,MBIN64S,MBIN64S,ADD_MBIN64S_MBIN64S_MBIN64S),  // Instruction 67
  I3(Op_SUB,MBIN16S,MBIN16S,MBIN16S,SUB_MBIN16S_MBIN16S_MBIN16S),  // Instruction 68
  I3(Op_SUB,MBIN32S,MBIN32S,MBIN32S,SUB_MBIN32S_MBIN32S_MBIN32S),  // Instruction 69
  I3(Op_SUB,MBIN64S,MBIN64S,MBIN64S,SUB_MBIN64S_MBIN64S_MBIN64S),  // Instruction 70
  I3(Op_MUL,MBIN16S,MBIN16S,MBIN16S,MUL_MBIN16S_MBIN16S_MBIN16S),  // Instruction 71
  I3(Op_MUL,MBIN32S,MBIN32S,MBIN32S,MUL_MBIN32S_MBIN32S_MBIN32S),  // Instruction 72
  I3(Op_MUL,MBIN64S,MBIN64S,MBIN64S,MUL_MBIN64S_MBIN64S_MBIN64S),  // Instruction 73

  IopCodeNotInUse(74),                                      // Instruction 74 not in use
  IopCodeNotInUse(75),                                      // Instruction 75 not in use
  IopCodeNotInUse(76),                                      // Instruction 76 not in use

  I2(Op_SUM,MBIN32S,MBIN32S,SUM_MBIN32S_MBIN32S),                  // Instruction 77
  
  IopCodeNotInUse(78),

  I2(Op_SUM,MBIN64S,MBIN64S,SUM_MBIN64S_MBIN64S),                  // Instruction 79

  IopCodeNotInUse(80),
  IopCodeNotInUse(81),                                      // Instruction 81 not in use
  IopCodeNotInUse(82),                                      // Instruction 82 not in use

  I3(Op_MOD,MBIN32S,MBIN32S,MBIN32S,MOD_MBIN32S_MBIN32S_MBIN32S),  // Instruction 83
  I4(Op_MINMAX,MBIN8,MBIN8,MBIN32S,IBIN32S,MINMAX_MBIN8_MBIN8_MBIN32S_IBIN32S),  // Instruction 84

  IopCodeNotInUse(85),                                      // Instruction 85 not in use
  IopCodeNotInUse(86),                                      // Instruction 86 not in use

  I3(Op_ENCODE,MASCII,MBIN16S,IBIN32S,ENCODE_MASCII_MBIN16S_IBIN32S),  // Instruction 87
  I3(Op_ENCODE,MASCII,MBIN16U,IBIN32S,ENCODE_MASCII_MBIN16U_IBIN32S),  // Instruction 88
  I3(Op_ENCODE,MASCII,MBIN32S,IBIN32S,ENCODE_MASCII_MBIN32S_IBIN32S),  // Instruction 89
  I3(Op_ENCODE,MASCII,MBIN32U,IBIN32S,ENCODE_MASCII_MBIN32U_IBIN32S),  // Instruction 90
  I3(Op_ENCODE,MASCII,MBIN64S,IBIN32S,ENCODE_MASCII_MBIN64S_IBIN32S),  // Instruction 91

  IopCodeNotInUse(92),

  I4(Op_ENCODE,MASCII,MASCII,IBIN32S,IBIN32S,ENCODE_NXX),              // Instruction 93
  I4(Op_HASH,MBIN32U,MPTR32,IBIN32S,IBIN32S,HASH_MBIN32U_MPTR32_IBIN32S_IBIN32S),  // Instruction 94
  I1(Op_BRANCH,IBIN32S,BRANCH),                                    // Instruction 95

  IopCodeNotInUse(96),                                      // Instruction 96 not in use

  I2(Op_RANGE_LOW,MBIN32S,IBIN64S,RANGE_LOW_S32S64),               // Instruction 97
  I2(Op_RANGE_HIGH,MBIN32S,IBIN64S,RANGE_HIGH_S32S64),             // Instruction 98
  I2(Op_RANGE_LOW,MBIN32U,IBIN64S,RANGE_LOW_U32S64),               // Instruction 99
  I2(Op_RANGE_HIGH,MBIN32U,IBIN64S,RANGE_HIGH_U32S64),             // Instruction 100
  I2(Op_RANGE_LOW,MBIN64S,IBIN64S,RANGE_LOW_S64S64),               // Instruction 101
  I2(Op_RANGE_HIGH,MBIN64S,IBIN64S,RANGE_HIGH_S64S64),             // Instruction 102
  I2(Op_RANGE_LOW,MBIN16S,IBIN64S,RANGE_LOW_S16S64),               // Instruction 103
  I2(Op_RANGE_HIGH,MBIN16S,IBIN64S,RANGE_HIGH_S16S64),             // Instruction 104
  I2(Op_RANGE_LOW,MBIN16U,IBIN64S,RANGE_LOW_U16S64),               // Instruction 105
  I2(Op_RANGE_HIGH,MBIN16U,IBIN64S,RANGE_HIGH_U16S64),             // Instruction 106

  IopCodeNotInUse(107),                                      // Instruction 107 not in use

  I2(Op_CLAUSE_EVAL,IPTR,IBIN32S,CLAUSE_EVAL),                  // Instruction 108
  I2(Op_CLAUSE_BRANCH,IPTR,IPTR,CLAUSE_BRANCH),              // Instruction 109

  I0(Op_PROFILE,PROFILE),                                          // Instruction 110

  IopCodeNotInUse(111),                                      // Instruction 111 not in use
  IopCodeNotInUse(112),                                      // Instruction 112 not in use

  I0(Op_NOP,NOP),                                                  // Instruction 113
  I0(Op_END,END),                                                  // Instruction 114

  I3(Op_FILL_MEM_BYTES,MASCII,IBIN32S,IBIN8U,FILL_MEM_BYTES),      // Instruction 115

  I2(Op_MOVE,MBIN16U,IBIN16U,MOVE_MBIN16U_IBIN16U),                // Instruction 116

  I4(Op_EQ,MBIN32S,MASCII,MASCII,IBIN32S,EQ_MBIN32S_MASCII_MASCII),  // Instruction 117

  I3(Op_ADD,MBIN32S,MBIN16S,MBIN16S,ADD_MBIN32S_MBIN16S_MBIN16S),    // Instruction 118
  I3(Op_ADD,MBIN32S,MBIN16S,MBIN32S,ADD_MBIN32S_MBIN16S_MBIN32S),    // Instruction 119

  IopCodeNotInUse(120),

  I3(Op_ADD,MBIN64S,MBIN32S,MBIN64S,ADD_MBIN64S_MBIN32S_MBIN64S),    // Instruction 121

  IopCodeNotInUse(122),

  I3(Op_SUB,MBIN32S,MBIN16S,MBIN16S,SUB_MBIN32S_MBIN16S_MBIN16S),    // Instruction 123
  I3(Op_SUB,MBIN32S,MBIN16S,MBIN32S,SUB_MBIN32S_MBIN16S_MBIN32S),    // Instruction 124
  I3(Op_SUB,MBIN32S,MBIN32S,MBIN16S,SUB_MBIN32S_MBIN32S_MBIN16S),    // Instruction 125

  I3(Op_MUL,MBIN32S,MBIN16S,MBIN16S,MUL_MBIN32S_MBIN16S_MBIN16S),    // Instruction 126
  I3(Op_MUL,MBIN32S,MBIN16S,MBIN32S,MUL_MBIN32S_MBIN16S_MBIN32S),    // Instruction 127

  IopCodeNotInUse(128),

  I3(Op_DIV,MBIN64S,MBIN64S,MBIN64S,DIV_MBIN64S_MBIN64S_MBIN64S),    // Instruction 129
  I4(Op_LT,MBIN32S,MASCII,MASCII,IBIN32S,LT_MBIN32S_MASCII_MASCII),  // Instruction 130
  I4(Op_LE,MBIN32S,MASCII,MASCII,IBIN32S,LE_MBIN32S_MASCII_MASCII),  // Instruction 131
  I4(Op_GT,MBIN32S,MASCII,MASCII,IBIN32S,GT_MBIN32S_MASCII_MASCII),  // Instruction 132
  I4(Op_GE,MBIN32S,MASCII,MASCII,IBIN32S,GE_MBIN32S_MASCII_MASCII),  // Instruction 133

  I3(Op_LE,MBIN32S,MBIN16S,MBIN16S,LE_MBIN32S_MBIN16S_MBIN16S),  // Instruction 134
  I3(Op_LE,MBIN32S,MBIN16S,MBIN32S,LE_MBIN32S_MBIN16S_MBIN32S),  // Instruction 135

  IopCodeNotInUse(136),

  I3(Op_LE,MBIN32S,MBIN32S,MBIN32S,LE_MBIN32S_MBIN32S_MBIN32S),  // Instruction 137
  I3(Op_LE,MBIN32S,MBIN16U,MBIN16U,LE_MBIN32S_MBIN16U_MBIN16U),  // Instruction 138
  I3(Op_LE,MBIN32S,MBIN16U,MBIN32U,LE_MBIN32S_MBIN16U_MBIN32U),  // Instruction 139

  IopCodeNotInUse(140),

  I3(Op_LE,MBIN32S,MBIN32U,MBIN32U,LE_MBIN32S_MBIN32U_MBIN32U),  // Instruction 141
  I3(Op_LE,MBIN32S,MBIN16S,MBIN32U,LE_MBIN32S_MBIN16S_MBIN32U),  // Instruction 142

  IopCodeNotInUse(143),

  I3(Op_GE,MBIN32S,MBIN16S,MBIN16S,GE_MBIN32S_MBIN16S_MBIN16S),  // Instruction 144
  I3(Op_GE,MBIN32S,MBIN16S,MBIN32S,GE_MBIN32S_MBIN16S_MBIN32S),  // Instruction 145

  IopCodeNotInUse(146),

  I3(Op_GE,MBIN32S,MBIN32S,MBIN32S,GE_MBIN32S_MBIN32S_MBIN32S),  // Instruction 147
  I3(Op_GE,MBIN32S,MBIN16U,MBIN16U,GE_MBIN32S_MBIN16U_MBIN16U),  // Instruction 148
  I3(Op_GE,MBIN32S,MBIN16U,MBIN32U,GE_MBIN32S_MBIN16U_MBIN32U),  // Instruction 149

  IopCodeNotInUse(150),

  I3(Op_GE,MBIN32S,MBIN32U,MBIN32U,GE_MBIN32S_MBIN32U_MBIN32U),  // Instruction 151
  I3(Op_GE,MBIN32S,MBIN16S,MBIN32U,GE_MBIN32S_MBIN16S_MBIN32U),  // Instruction 152

  IopCodeNotInUse(153),
  IopCodeNotInUse(154), 
  IopCodeNotInUse(155), 

  I3(Op_HASHCOMB,MBIN32U,MBIN32U,MBIN32U,HASHCOMB_MBIN32U_MBIN32U_MBIN32U),  // Instruction 156
  I3(Op_EQ,MBIN32S,MBIN16U,MBIN16S,EQ_MBIN32S_MBIN16U_MBIN16S),  // Instruction 157
  I3(Op_LT,MBIN32S,MBIN16U,MBIN16S,LT_MBIN32S_MBIN16U_MBIN16S),  // Instruction 158
  I3(Op_LE,MBIN32S,MBIN16U,MBIN16S,LE_MBIN32S_MBIN16U_MBIN16S),  // Instruction 159
  I3(Op_GT,MBIN32S,MBIN16U,MBIN16S,GT_MBIN32S_MBIN16U_MBIN16S),  // Instruction 160
  I3(Op_GE,MBIN32S,MBIN16U,MBIN16S,GE_MBIN32S_MBIN16U_MBIN16S),  // Instruction 161

  I3(Op_EQ,MBIN32S,MBIN64S,MBIN64S,EQ_MBIN32S_MBIN64S_MBIN64S),  // Instruction 162
  I3(Op_LT,MBIN32S,MBIN64S,MBIN64S,LT_MBIN32S_MBIN64S_MBIN64S),  // Instruction 163
  I3(Op_LE,MBIN32S,MBIN64S,MBIN64S,LE_MBIN32S_MBIN64S_MBIN64S),  // Instruction 164
  I3(Op_GT,MBIN32S,MBIN64S,MBIN64S,GT_MBIN32S_MBIN64S_MBIN64S),  // Instruction 165
  I3(Op_GE,MBIN32S,MBIN64S,MBIN64S,GE_MBIN32S_MBIN64S_MBIN64S),  // Instruction 166
  I3(Op_NE,MBIN32S,MBIN64S,MBIN64S,NE_MBIN32S_MBIN64S_MBIN64S),  // Instruction 167

  I4(Op_BRANCH_AND,IPTR,IPTR,MBIN32S,MBIN32S,BRANCH_AND),  // Instruction 168
  I4(Op_BRANCH_OR,IPTR,IPTR,MBIN32S,MBIN32S,BRANCH_OR),    // Instruction 169

  I4(Op_HASH,MBIN32U,MBIN32S,IBIN32S,IBIN32S,HASH_MBIN32U_MBIN32_IBIN32S_IBIN32S),  // Instruction 170
  I4(Op_HASH,MBIN32U,MBIN16S,IBIN32S,IBIN32S,HASH_MBIN32U_MBIN16_IBIN32S_IBIN32S),  // Instruction 171
  //  I4(Op_HASH,MBIN32U,MPTR32,IBIN32S,IBIN32S,HASH_MBIN32U_MPTR32_IBIN32S_IBIN32S),  // Instruction 172
  IopCodeNotInUse(172),
  I4(Op_HASH,MBIN32U,MBIN64S,IBIN32S,IBIN32S,HASH_MBIN32U_MBIN64_IBIN32S_IBIN32S),//8byte 

  IopCodeNotInUse(174),                                      // Instruction 174 not in use
  IopCodeNotInUse(175),                                      // Instruction 175 not in use

  I4(Op_ENCODE,MASCII,MDECS,IBIN32S,IBIN32S,ENCODE_DECS),             // Instruction 176

  IopCodeNotInUse(177),                                      // Instruction 177 not in use
  IopCodeNotInUse(178),                                      // Instruction 178 not in use
  IopCodeNotInUse(179),                                      // Instruction 179 not in use
  IopCodeNotInUse(180),                                      // Instruction 180 not in use

  I6(Op_GENFUNC,IBIN32S,MBIN8,MBIN8,IBIN32S,MBIN8,IBIN32S,GENFUNC_PCODE_1),  // Instruction 181

  IopCodeNotInUse(182), 

  I3(Op_MUL,MBIN64S,MBIN16S,MBIN32S,MUL_MBIN64S_MBIN16S_MBIN32S),  // Instruction 183
  I3(Op_MUL,MBIN64S,MBIN32S,MBIN32S,MUL_MBIN64S_MBIN32S_MBIN32S),  // Instruction 184

  IopCodeNotInUse(185),

  I3(Op_MOD,MBIN32U,MBIN32U,MBIN32S,MOD_MBIN32U_MBIN32U_MBIN32S),  // Instruction 186
  I4(Op_NOT_NULL_BRANCH,MBIN32S,MBIN16S,MBIN16S,IBIN32S,
      NOT_NULL_BRANCH_MBIN32S_MBIN16S_MBIN16S_IBIN32S),   // Instruction 187
  I3(Op_NOT_NULL_BRANCH,MBIN32S,MBIN16S,IBIN32S,
      NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S),           // Instruction 188
  I4(Op_NOT_NULL_BRANCH,MBIN32S,MBIN16S,IBIN32S,IBIN32S,
      NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S_IBIN32S),   // Instruction 189
  I2(Op_NOT_NULL_BRANCH,MBIN16S,IBIN32S,NOT_NULL_BRANCH_MBIN16S_IBIN32S),  // Instruction 190
  I6(Op_GENFUNC,IBIN32S,MBIN8,MBIN8,MBIN8,IBIN32S,IBIN32S,
      GENFUNC_MBIN8_MBIN8_MBIN8_IBIN32S_IBIN32S),         // Instruction 191

  IopCodeNotInUse(192), 

  I2(Op_MOVE,MFLT64,MBIN16S,MOVE_MFLT64_MBIN16S),         // Instruction 193
  I2(Op_MOVE,MFLT64,MFLT32,MOVE_MFLT64_MFLT32),           // Instruction 194

  IopCodeNotInUse(195),

  I3(Op_NE,MBIN32S,MBIN16S,MBIN16S,NE_MBIN32S_MBIN16S_MBIN16S),    // Instruction 196

  IopCodeNotInUse(197),
  IopCodeNotInUse(198),                                      // Instruction 198 not in use
  IopCodeNotInUse(199),                                      // Instruction 199 not in use

  I2(Op_MOVE,MBIN8,MBIN8,MOVE_MBIN8_MBIN8),               // Instruction 200
  I2(Op_MOVE,MBIN16U,MBIN16U,MOVE_MBIN16U_MBIN16U),       // Instruction 201
  I2(Op_MOVE,MBIN32U,MBIN32U,MOVE_MBIN32U_MBIN32U),       // Instruction 202
  I2(Op_MOVE,MBIN64S,MBIN64S,MOVE_MBIN64S_MBIN64S),       // Instruction 203

  I3(Op_ADD,MFLT64,MFLT64,MFLT64,ADD_MFLT64_MFLT64_MFLT64),  // Instruction 204
  I3(Op_SUB,MFLT64,MFLT64,MFLT64,SUB_MFLT64_MFLT64_MFLT64),  // Instruction 205
  I3(Op_MUL,MFLT64,MFLT64,MFLT64,MUL_MFLT64_MFLT64_MFLT64),  // Instruction 206
  I3(Op_DIV,MFLT64,MFLT64,MFLT64,DIV_MFLT64_MFLT64_MFLT64),  // Instruction 207

  IopCodeNotInUse(208),

  I2(Op_SUM,MFLT64,MFLT64,SUM_MFLT64_MFLT64),                // Instruction 209
  I1(Op_RANGE_LOW,MFLT64,RANGE_MFLT64),                      // Instruction 210

  I3(Op_NE,MBIN32S,MFLT64,MFLT64,NE_MBIN32S_MFLT64_MFLT64),  // Instruction 211
  I3(Op_NE,MBIN32S,MFLT32,MFLT32,NE_MBIN32S_MFLT32_MFLT32),  // Instruction 212

  I3(Op_GT,MBIN32S,MFLT64,MFLT64,GT_MBIN32S_MFLT64_MFLT64),  // Instruction 213
  I3(Op_GT,MBIN32S,MFLT32,MFLT32,GT_MBIN32S_MFLT32_MFLT32),  // Instruction 214

  I3(Op_GE,MBIN32S,MFLT64,MFLT64,GE_MBIN32S_MFLT64_MFLT64),  // Instruction 215
  I3(Op_GE,MBIN32S,MFLT32,MFLT32,GE_MBIN32S_MFLT32_MFLT32),  // Instruction 216

  I3(Op_EQ,MBIN32S,MFLT64,MFLT64,EQ_MBIN32S_MFLT64_MFLT64),  // Instruction 217
  I3(Op_EQ,MBIN32S,MFLT32,MFLT32,EQ_MBIN32S_MFLT32_MFLT32),  // Instruction 218

  I3(Op_LE,MBIN32S,MFLT64,MFLT64,LE_MBIN32S_MFLT64_MFLT64),  // Instruction 219
  I3(Op_LE,MBIN32S,MFLT32,MFLT32,LE_MBIN32S_MFLT32_MFLT32),  // Instruction 220

  I3(Op_LT,MBIN32S,MFLT64,MFLT64,LT_MBIN32S_MFLT64_MFLT64),  // Instruction 221
  I3(Op_LT,MBIN32S,MFLT32,MFLT32,LT_MBIN32S_MFLT32_MFLT32),  // Instruction 222

  I3(Op_HASH2_DISTRIB,MBIN32U,MBIN32U,MBIN32U,HASH2_DISTRIB),  // Instruction 223

  I4(Op_NULL_BITMAP,MPTR32,IBIN32S,IBIN16U,IBIN32S,NULL_BITMAP),    // Instruction 224

  IopCodeNotInUse(225),                                      // Instruction 225 not in use
  IopCodeNotInUse(226),                                      // Instruction 226 not in use
  IopCodeNotInUse(227),                                      // Instruction 227 not in use

  I3(Op_OPDATA,MPTR32,IBIN32S,IBIN32S,OPDATA_MPTR32_IBIN32S_IBIN32S),   // Instruction 228

  IopCodeNotInUse(229),                                      // Instruction 229 not in use
  IopCodeNotInUse(230),                                      // Instruction 230 not in use

  I2(Op_OPDATA,MATTR5,IBIN32S,OPDATA_MATTR5_IBIN32S),                   // Instruction 231

  IopCodeNotInUse(232),
  IopCodeNotInUse(233),
  IopCodeNotInUse(234),
  IopCodeNotInUse(235),
  IopCodeNotInUse(236),

  I4(Op_DIV_ROUND,MBIN64S,MBIN64S,MBIN64S,IBIN32S,
    DIV_MBIN64S_MBIN64S_MBIN64S_ROUND),                                // Instruction 237
  I4(Op_NULL_VIOLATION,MPTR32,MPTR32,IPTR,IPTR,
    NULL_VIOLATION_MPTR32_MPTR32_IPTR_IPTR),                     // Instruction 238

  I5(Op_OFFSET,IPTR,IPTR,MBIN32S,MBIN64S,MBIN64S,
    OFFSET_IPTR_IPTR_MBIN32S_MBIN64S_MBIN64S),                   // Instruction 239
  I5(Op_OFFSET,IPTR,IPTR,IBIN32S,MBIN64S,MBIN64S,
    OFFSET_IPTR_IPTR_IBIN32S_MBIN64S_MBIN64S),                   // Instruction 240

  I4(Op_ADD,MBIGS,MBIGS,MBIGS,IBIN32S,ADD_MBIGS_MBIGS_MBIGS_IBIN32S),  // Instruction 241
  I4(Op_SUB,MBIGS,MBIGS,MBIGS,IBIN32S,SUB_MBIGS_MBIGS_MBIGS_IBIN32S),  // Instruction 242
  I6(Op_MUL,MBIGS,MBIGS,MBIGS,IBIN32S,IBIN32S,IBIN32S,MUL_MBIGS_MBIGS_MBIGS_IBIN32S),  // Instruction 243
  I4(Op_MOVE,MBIGS,MBIGS,IBIN32S,IBIN32S,MOVE_MBIGS_MBIGS_IBIN32S_IBIN32S),  // Instruction 244
  I3(Op_MOVE,MBIGS,MBIN64S,IBIN32S,MOVE_MBIGS_MBIN64S_IBIN32S),        // Instruction 245
  I3(Op_MOVE,MBIN64S,MBIGS,IBIN32S,MOVE_MBIN64S_MBIGS_IBIN32S),        // Instruction 246

  I5(Op_NOT_NULL_BRANCH,MBIN32S,MBIN32S,MBIN32S,IATTR4,IBIN32S,
    NOT_NULL_BRANCH_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S),           // Instruction 247
  I4(Op_NOT_NULL_BRANCH,MBIN32S,MBIN32S,IATTR3,IBIN32S,
    NOT_NULL_BRANCH_MBIN32S_MBIN32S_IATTR3_IBIN32S),                   // Instruction 248
  I5(Op_NOT_NULL_BRANCH_COMP,MBIN32S,MBIN32S,MBIN32S,IATTR4,IBIN32S,
    NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S),      // Instruction 249
  I4(Op_NOT_NULL_BRANCH_COMP,MBIN32S,MBIN32S,IATTR3,IBIN32S,
    NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S),              // Instruction 250

  I4(Op_MOVE,MASCII,MASCII,IBIN32S,IBIN32S,MOVE_MASCII_MASCII_IBIN32S_IBIN32S),  // Instruction 251

  IopCodeNotInUse(252),
  IopCodeNotInUse(253),
  IopCodeNotInUse(254),
  IopCodeNotInUse(255),
  IopCodeNotInUse(256),
  IopCodeNotInUse(257),
  IopCodeNotInUse(258),
  IopCodeNotInUse(259),
  IopCodeNotInUse(260),
  IopCodeNotInUse(261),

  I4(Op_NULL,MBIN32S,MATTR5,IBIN32S,IBIN32S,NULL_TEST_MBIN32S_MATTR5_IBIN32S_IBIN32S),    // Instruction 262
  I4(Op_NE,MBIN32S,MASCII,MASCII,IBIN32S,NE_MBIN32S_MASCII_MASCII),    // Instruction 263

  I0(Op_RETURN,RETURN),    // Instruction 264
  I1(Op_RETURN,IBIN32S,RETURN_IBIN32S),  // Instruction 265

  I1(Op_HASHCOMB,MBIN32U,HASHCOMB_BULK_MBIN32U),  // Instruction 266
  I0(Op_NOT_NULL_BRANCH,NOT_NULL_BRANCH_BULK),    // Instruction 267
  I0(Op_MOVE,MOVE_BULK),    // Instruction 268
  I0(Op_NULL_BITMAP,NULL_BITMAP_BULK),    // Instruction 269

  I5(Op_COMP,MBIN32S,MBIGS,MBIGS,IBIN32S,IBIN32S,
    COMP_MBIN32S_MBIGS_MBIGS_IBIN32S_IBIN32S),    // Instruction 270 obsolete

  I2(Op_SUM,MBIN64S,MBIN32S,SUM_MBIN64S_MBIN32S),  // Instruction 271

  IopCodeNotInUse(272),

  I6(Op_BRANCH_AND,IBIN32S,IBIN32S,MBIN32S,MBIN32S,IBIN32S,IBIN32S,BRANCH_AND_CNT),  // Instruction 273
  I6(Op_BRANCH_OR,IBIN32S,IBIN32S,MBIN32S,MBIN32S,IBIN32S,IBIN32S,BRANCH_OR_CNT),  // Instruction 274

  I6(Op_COMP,MBIN32S,MASCII,MASCII,IBIN32S,IBIN32S,IBIN32S,
      COMP_MBIN32S_MASCII_MASCII_IBIN32S_IBIN32S_IBIN32S),  // Instruction 275

  IopCodeNotInUse(276),                                      // obsolete Instruction 276
  IopCodeNotInUse(277), 
  IopCodeNotInUse(278), 
  IopCodeNotInUse(279), 

  I6(Op_COMP,MBIN32S,MATTR4,MATTR4,IBIN32S,IBIN32S,IBIN32S,
    COMP_MBIN32S_MATTR4_MATTR4_IBIN32S_IBIN32S_IBIN32S),  // Instruction 280
  I6(Op_COMP,MBIN32S,MUNI,MUNI,IBIN32S,IBIN32S,IBIN32S,
    COMP_MBIN32S_MUNI_MUNI_IBIN32S_IBIN32S_IBIN32S),  // Instruction 281
  I4(Op_MOVE,MUNI,MUNI,IBIN32S,IBIN32S,MOVE_MUNI_MUNI_IBIN32S_IBIN32S),  // Instruction 282

  IopCodeNotInUse(283),                                      // Instruction 283

  I2(Op_MOVE,MATTR5,MATTR5,MOVE_MATTR5_MATTR5),    // Instruction 284
  I4(Op_COMP,MBIN32S,MATTR5,MATTR5,IBIN32S,COMP_MBIN32S_MATTR5_MATTR5_IBIN32S),    // Instruction 285
  I2(Op_HASH,MBIN32U,MATTR5,HASH_MBIN32U_MATTR5),    // Instruction 286
  I4(Op_GENFUNC,MATTR5,MATTR5,MBIN32S,MBIN32S,SUBSTR_MATTR5_MATTR5_MBIN32S_MBIN32S),    // Instruction 287
  I3(Op_GENFUNC,MATTR5,MATTR5,IBIN32S,GENFUNC_MATTR5_MATTR5_IBIN32S),    // Instruction 288

  // BigNum, map is not available for now.
  I3(Op_MOVE,MBIGS,MBIN32S,IBIN32S,MOVE_MBIGS_MBIN32S_IBIN32S),         // Instruction 289
  I3(Op_MOVE,MBIGS,MBIN16S,IBIN32S,MOVE_MBIGS_MBIN16S_IBIN32S),         // Instruction 290 

  IopCodeNotInUse(291),                                      // Instruction 291
  IopCodeNotInUse(292),                                      // Instruction 292

  I4(Op_COMP,MBIN32S,MUNIV,MUNIV,IBIN32S,COMP_MBIN32S_MUNIV_MUNIV_IBIN32S),    // Instruction 293
  I2(Op_HASH,MBIN32U,MUNIV,HASH_MBIN32U_MUNIV),    // Instruction 294

  I2(Op_MOVE,MFLT64,MBIN32S,MOVE_MFLT64_MBIN32S),    // Instruction 295
  I2(Op_MOVE,MFLT64,MBIN64S,MOVE_MFLT64_MBIN64S),    // Instruction 296

  I2(Op_GENFUNC,MBIN32U,MATTR5,STRLEN_MBIN32U_MATTR5),    // Instruction 297
  I2(Op_GENFUNC,MBIN32U,MUNIV,STRLEN_MBIN32U_MUNIV),    // Instruction 298
  I4(Op_GENFUNC,MATTR5,MATTR5,MBIN32S,IBIN32S,
       GENFUNC_MATTR5_MATTR5_MBIN32S_IBIN32S),    // Instruction 299
  I3(Op_GENFUNC,MBIN32S,MATTR5,MATTR5,POS_MBIN32S_MATTR5_MATTR5),    // Instruction 300
  I5(Op_GENFUNC,MBIN32S,MATTR5,MATTR5,IBIN32S,IBIN32S,
       LIKE_MBIN32S_MATTR5_MATTR5_IBIN32S_IBIN32S),    // Instruction 301
  I2(Op_MOVE,MBIN16U,MBIN8,MOVE_MBIN16U_MBIN8),    // Instruction 302

  // Header clause
  I6(Op_HDR,MPTR32,IBIN32S,IBIN32S,IBIN32S,IBIN32S,IBIN32S,
    HDR_MPTR32_IBIN32S_IBIN32S_IBIN32S_IBIN32S_IBIN32S), // Instruction 303

  // Special nulls for all data formats
  I5(Op_NOT_NULL_BRANCH,MBIN32S,MATTR3,MATTR3,IBIN32S,IBIN32S,
    NNB_SPECIAL_NULLS_MBIN32S_MATTR3_MATTR3_IBIN32S_IBIN32S), // Instruction 304

  I4(Op_NULLIFZERO,MPTR32,MATTR3,MPTR32,IBIN32S,
     NULLIFZERO_MPTR32_MATTR3_MPTR32_IBIN32S),               // Instruction 305

  I2(Op_NOT_NULL_BRANCH,MATTR3,IBIN32S, NNB_MATTR3_IBIN32S), // Instruction 306

  // Null helper for hash
  I4(Op_NOT_NULL_BRANCH,MBIN32S,MATTR3,IBIN32S,IBIN32S, // Instruction 307
    NNB_MBIN32S_MATTR3_IBIN32S_IBIN32S),

  I5(Op_REPLACE_NULL, MBIN32S, MATTR3, MBIN8, MBIN8, IBIN32S,
    REPLACE_NULL_MATTR3_MBIN32S),                               // Instruction 308
  I5(Op_REPLACE_NULL, MBIN32U, MATTR3, MBIN8, MBIN8, IBIN32S,
    REPLACE_NULL_MATTR3_MBIN32U),                               // Instruction 309
  I5(Op_REPLACE_NULL, MBIN16S, MATTR3, MBIN8, MBIN8, IBIN32S,
    REPLACE_NULL_MATTR3_MBIN16S),                               // Instruction 310
  I5(Op_REPLACE_NULL, MBIN16U, MATTR3, MBIN8, MBIN8, IBIN32S,
    REPLACE_NULL_MATTR3_MBIN16U),                               // Instruction 311

  I5(Op_SUM,MATTR3,MATTR3,IBIN32S,MBIN32S,MBIN32S,
    SUM_MATTR3_MATTR3_IBIN32S_MBIN32S_MBIN32S),     // Instruction 312
  I5(Op_SUM,MATTR3,MATTR3,IBIN32S,MBIN64S,MBIN64S,
    SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN64S),     // Instruction 313
  I5(Op_SUM,MATTR3,MATTR3,IBIN32S,MBIN64S,MBIN32S,
    SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN32S),     // Instruction 314
  I5(Op_SUM,MATTR3,MATTR3,IBIN32S,MFLT64,MFLT64,
    SUM_MATTR3_MATTR3_IBIN32S_MFLT64_MFLT64),       // Instruction 315

  I2(Op_UPDATE_ROWLEN3,MATTR5,IBIN32S,UPDATE_ROWLEN3_MATTR5_IBIN32S),   // Instruction 316

  I3(Op_FILL_MEM_BYTES,MATTR5,IBIN32S,IBIN32S,FILL_MEM_BYTES_VARIABLE), // Instruction 317

  I3(Op_MOVE,MASCII,MATTR5,IBIN32S,MOVE_MASCII_MATTR5_IBIN32S),         // Instruction 318
  I3(Op_MOVE,MATTR5,MASCII,IBIN32S,MOVE_MATTR5_MASCII_IBIN32S),         // Instruction 319
  I5(Op_COMP,MBIN32S,MATTR5,MPTR32,IBIN32S,IBIN32S,
     SWITCH_MBIN32S_MATTR5_MPTR32_IBIN32S_IBIN32S),                     // Instruction 320
  I5(Op_COMP,MBIN32S,MBIN64S,MPTR32,IBIN32S,IBIN32S,
     SWITCH_MBIN32S_MBIN64S_MPTR32_IBIN32S_IBIN32S),                    // Instruction 321
  I1(Op_BRANCH,MBIN32S,BRANCH_INDIRECT_MBIN32S),                        // Instruction 322
  I3(Op_GENFUNC,MATTR5,MATTR5,MATTR5,CONCAT_MATTR5_MATTR5_MATTR5),      // Instruction 323
  I6(Op_SUM,MATTR3,MATTR3,IBIN32S,MBIGS,MBIGS,IBIN32S,
     SUM_MATTR3_MATTR3_IBIN32S_MBIGS_MBIGS_IBIN32S),                    // Instruction 324
 
  I2(Op_NULL_BYTES,MPTR32,IBIN32S,NULL_BYTES),                           // Instruction 325
  I6(Op_COPYVARROW, MBIN8, MBIN8, IBIN32S, IBIN32S, IBIN32S,IBIN32S,
     COPYVARROW_MBIN8_MBIN8_IBIN32S_IBIN32S_IBIN32S_IBIN32S),		// Instruction 326
  I3(Op_CONV_VC_PTR,MBIN32S,MATTR5,IBIN32S,CONVVCPTR_MBIN32S_MATTR5_IBIN32S),        // Instruction 327
  I3(Op_CONV_VC_PTR,MFLT32,MATTR5,IBIN32S,CONVVCPTR_MFLT32_MATTR5_IBIN32S),        // Instruction 328
  I2(Op_CONV_VC_PTR,MATTR5,MATTR5,CONVVCPTR_MATTR5_MATTR5),       // Instruction 329
  I3(Op_CONV_VC_PTR,MBIN64S,MATTR5,IBIN32S,CONVVCPTR_MBIN64S_MATTR5_IBIN32S),        // Instruction 330

  I3(Op_DECODE,MASCII,MBIN16S,IBIN32S,DECODE_MASCII_MBIN16S_IBIN32S),  // Instruction 331
  I3(Op_DECODE,MASCII,MBIN16U,IBIN32S,DECODE_MASCII_MBIN16U_IBIN32S),  // Instruction 332
  I3(Op_DECODE,MASCII,MBIN32S,IBIN32S,DECODE_MASCII_MBIN32S_IBIN32S),  // Instruction 333
  I3(Op_DECODE,MASCII,MBIN32U,IBIN32S,DECODE_MASCII_MBIN32U_IBIN32S),  // Instruction 334
  I3(Op_DECODE,MASCII,MBIN64S,IBIN32S,DECODE_MASCII_MBIN64S_IBIN32S),  // Instruction 335
  I4(Op_DECODE,MASCII,MASCII,IBIN32S,IBIN32S,DECODE_DATETIME),         // Instruction 336
  I4(Op_DECODE,MASCII,MASCII,IBIN32S,IBIN32S,DECODE_NXX),              // Instruction 337
  I4(Op_DECODE,MASCII,MDECS,IBIN32S,IBIN32S,DECODE_DECS),              // Instruction 338

  I2(Op_NEG,MASCII,MASCII,NEGATE_MASCII_MASCII),                       // Instruction 339

  I3(Op_ENCODE,MASCII,MBIN8S,IBIN32S,ENCODE_MASCII_MBIN8S_IBIN32S),  // Instruction 340
  I3(Op_ENCODE,MASCII,MBIN8U,IBIN32S,ENCODE_MASCII_MBIN8U_IBIN32S),  // Instruction 341

  I3(Op_DECODE,MASCII,MBIN8S,IBIN32S,DECODE_MASCII_MBIN8S_IBIN32S),  // Instruction 342
  I3(Op_DECODE,MASCII,MBIN8U,IBIN32S,DECODE_MASCII_MBIN8U_IBIN32S),  // Instruction 343
 
  I2(Op_MOVE,MBIN16S,MBIN8S,MOVE_MBIN16S_MBIN8S),                    // Instruction 344
  I2(Op_MOVE,MBIN16U,MBIN8U,MOVE_MBIN16U_MBIN8U),                    // Instruction 345
  I2(Op_MOVE,MBIN32S,MBIN8S,MOVE_MBIN32S_MBIN8S),                    // Instruction 346
  I2(Op_MOVE,MBIN32U,MBIN8U,MOVE_MBIN32U_MBIN8U),                    // Instruction 347
  I2(Op_MOVE,MBIN64S,MBIN8S,MOVE_MBIN64S_MBIN8S),                    // Instruction 348
  I2(Op_MOVE,MBIN64U,MBIN8U,MOVE_MBIN64U_MBIN8U),                    // Instruction 349

  I3(Op_EQ,MBIN32S,MBIN8S,MBIN8S,EQ_MBIN32S_MBIN8S_MBIN8S),          // Instruction 350
  I3(Op_NE,MBIN32S,MBIN8S,MBIN8S,NE_MBIN32S_MBIN8S_MBIN8S),          // Instruction 351
  I3(Op_LT,MBIN32S,MBIN8S,MBIN8S,LT_MBIN32S_MBIN8S_MBIN8S),          // Instruction 352
  I3(Op_GT,MBIN32S,MBIN8S,MBIN8S,GT_MBIN32S_MBIN8S_MBIN8S),          // Instruction 353
  I3(Op_LE,MBIN32S,MBIN8S,MBIN8S,LE_MBIN32S_MBIN8S_MBIN8S),          // Instruction 354
  I3(Op_GE,MBIN32S,MBIN8S,MBIN8S,GE_MBIN32S_MBIN8S_MBIN8S),          // Instruction 355

  I3(Op_EQ,MBIN32S,MBIN8U,MBIN8U,EQ_MBIN32S_MBIN8U_MBIN8U),          // Instruction 356
  I3(Op_NE,MBIN32S,MBIN8U,MBIN8U,NE_MBIN32S_MBIN8U_MBIN8U),          // Instruction 357
  I3(Op_LT,MBIN32S,MBIN8U,MBIN8U,LT_MBIN32S_MBIN8U_MBIN8U),          // Instruction 358
  I3(Op_GT,MBIN32S,MBIN8U,MBIN8U,GT_MBIN32S_MBIN8U_MBIN8U),          // Instruction 359
  I3(Op_LE,MBIN32S,MBIN8U,MBIN8U,LE_MBIN32S_MBIN8U_MBIN8U),          // Instruction 360
  I3(Op_GE,MBIN32S,MBIN8U,MBIN8U,GE_MBIN32S_MBIN8U_MBIN8U),          // Instruction 361

  I2(Op_MOVE,MBIN64S,MBIN64U,MOVE_MBIN64S_MBIN64U),                  // Instruction 362
  I2(Op_MOVE,MBIN64U,MBIN64S,MOVE_MBIN64U_MBIN64S),                  // Instruction 363
  I2(Op_MOVE,MBIN64U,MBIN64U,MOVE_MBIN64U_MBIN64U),                  // Instruction 364

  I2(Op_RANGE_LOW,MBIN8S,IBIN64S,RANGE_LOW_S8S64),               // Instruction 365
  I2(Op_RANGE_HIGH,MBIN8S,IBIN64S,RANGE_HIGH_S8S64),             // Instruction 366
  I2(Op_RANGE_LOW,MBIN8U,IBIN64S,RANGE_LOW_U8S64),               // Instruction 367
  I2(Op_RANGE_HIGH,MBIN8U,IBIN64S,RANGE_HIGH_U8S64),             // Instruction 368

  I2(Op_MOVE,MBIN8S,MBIN16S,MOVE_MBIN8S_MBIN16S),                    // Instruction 369
  I2(Op_MOVE,MBIN8U,MBIN16U,MOVE_MBIN8U_MBIN16U),                    // Instruction 370

  I2(Op_MOVE,MBIN8U,MBIN16S,MOVE_MBIN8U_MBIN16S),                    // Instruction 371
  I2(Op_MOVE,MBIN8S,MBIN16U,MOVE_MBIN8S_MBIN16U),                    // Instruction 372

  };

#undef I
#undef I0
#undef I1
#undef I2
#undef I4
#undef I5
#undef I6
#undef IopCodeNotInUse

// Set the position within pCode where an embedded address resides.
Int32 *PCode::getEmbeddedAddresses(Int32 opcode, Int32 addr[]) {
  //static int addr[6];
    switch(opcode) {
      case PCIT::CLAUSE_EVAL:
        addr[0] = 1;
        addr[1] = -1;
        break;

      case PCIT::CLAUSE_BRANCH:
        addr[0] = 3;  // address of branch clause starts from pcode[3]
        addr[1] = -1;
        break;

    case PCIT::NULL_VIOLATION_MPTR32_MPTR32_IPTR_IPTR:
      addr[0] = 5;
      addr[1] = 7;  // address of 2nd attributes struct starts from pcode[7]
      addr[2] = -1;
      break;

    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_MBIN16S_IBIN32S:
      addr[0] = 7;
      addr[1] = 9;  // should be pcode[9], but don't know if this opcode is used
      addr[2] = -1;
      break;

    default:
      addr[0] = -1;
      break;
    }
  return addr;
}

Int32 PCode::getInstructionLength(PCodeBinary * pcode) {
  PCodeBinary opcode = pcode[0];

  if(opcode < 0 || opcode >= PCIT::LAST_PCODE_INSTR)
  {
    char tmpbuf[100];
    str_sprintf(tmpbuf, "Invalid opcode: %d\n", opcode);
    NAAssert(tmpbuf, __FILE__ , __LINE__ );
  }

  if ((opcode == PCIT::HASHCOMB_BULK_MBIN32U) ||
      (opcode == PCIT::NOT_NULL_BRANCH_BULK) ||
      (opcode == PCIT::MOVE_BULK) ||
      (opcode == PCIT::NULL_BITMAP_BULK))
    return pcode[1];

  Int32 len = opcodeMap[opcode].length;
  return len;
}

const char * PCode::getOpcodeString(Int32 opcode)
{
  if(opcode < 0 || opcode >= PCIT::LAST_PCODE_INSTR)
  {
    char tmpbuf[100];
    str_sprintf(tmpbuf, "Invalid opcode: %d\n", opcode);
    NAAssert(tmpbuf, __FILE__ , __LINE__ );
  }

  return opcodeMap[opcode].opcodeString;
}

PCIT::Instruction PCode::getInstruction(PCI *pci) {
  
  if (pci->getOperation() == PCIT::Op_TARGET)  //Target instructions are just placeholders.
    return(PCIT::NOP);

  Int32 shift = 36;
  Int64 code = pci->getOperation() ;
  code = code << shift;
  Int32 i = 0;
  for(i=0; i<pci->getNumberAddressingModes(); i++) {
    shift -= 6;
    code |= ((Int64)pci->getAddressingMode(i)) << shift;
  }
  for(i=0; i<PCIT::LAST_PCODE_INSTR; i++) {
           if(opcodeMap[i].instruction == code) {

#ifdef SQLMX_DEBUG_PCIT
              if (getenv("SQLMX_DEBUG_PCIT")) {
	          cerr << " Instruction " << (PCIT::Instruction)opcodeMap[i].opcode
	          << endl;
              }
#endif
              return (PCIT::Instruction)opcodeMap[i].opcode;
          } 
      } 
#ifdef _DEBUG
  fprintf(stderr, 
          "ERROR(getInstruction) -- Unknown instruction, opcode = %d\n", 
          pci->getOperation());
#endif // _DEBUG
  ExpAssert(0, "Unknown PCode Instruction");

  return(PCIT::NOP);
};

const PCIMap &PCode::getMapEntry (Int32 i) {

 if ( ((char*)(&opcodeMap[i])) < (((char *)opcodeMap) + sizeof(opcodeMap))) {
   return opcodeMap[i];
 }
 else {
   return opcodeMap[PCIT::LAST_PCODE_INSTR - 1];
 }
}

Int32 PCode::getOpCodeMapElements(Int32 opcode, PCIT::Operation &operation,
                                  PCIT::AddressingMode am[], Int32 &numAModes )
{
  if( (opcode == PCIT::END) || (opcode < 0) || (opcode >=PCIT::LAST_PCODE_INSTR))
    return 1;

  Int64 instruction = opcodeMap[opcode].instruction >>
                      ( 36 - (opcodeMap[opcode].numAmodes * 6));
  for( Int32 count = opcodeMap[opcode].numAmodes; count > 0; count--)
  {
    am[count-1] =
                (PCIT::AddressingMode)(instruction & OPCODE_MAP_FIRSTSIX_BITS);
    instruction >>= 6;
  }
  operation = (PCIT::Operation)instruction;
  numAModes = opcodeMap[opcode].numAmodes;
  return 0;
}

Int32 PCode::isInstructionRangeType( PCIT::Instruction instruction)
{
  switch(instruction)
  {
    case PCIT::RANGE_LOW_S8S64:
    case PCIT::RANGE_HIGH_S8S64:
    case PCIT::RANGE_LOW_U8S64:
    case PCIT::RANGE_HIGH_U8S64:
    case PCIT::RANGE_LOW_S16S64:
    case PCIT::RANGE_HIGH_S16S64:
    case PCIT::RANGE_LOW_U16S64:
    case PCIT::RANGE_HIGH_U16S64:
    case PCIT::RANGE_LOW_S32S64:
    case PCIT::RANGE_HIGH_S32S64:
    case PCIT::RANGE_LOW_U32S64:
    case PCIT::RANGE_HIGH_U32S64:
    case PCIT::RANGE_LOW_S64S64:
    case PCIT::RANGE_HIGH_S64S64:
      return 1;
    default:
      return 0;
  };
}

// LCOV_EXCL_START
Int32 PCode::profileInit() {
  if(profileCounts_) delete [] profileCounts_;
  if(profileTimes_) delete [] profileTimes_;
  profileCounts_ = new Int32[size()+1];
  profileTimes_ = new Int32[size()+1];

  for(Int32 i=0; i<size()+1; i++) {
    profileCounts_[i] = 0;
    profileTimes_[i] = 0;
  }

  return 0;
}
// LCOV_EXCL_STOP
Int32 PCode::profilePrint() {
  PCIListIter iter(pciList_);

  if(!profileCounts_) return 0;
  // LCOV_EXCL_START
  Int32 pciNum = 0;
  for(PCI *pci = iter.first(); pci; pci = iter.next()) {
    if(pci->getOperation() == PCIT::Op_PROFILE) {
      pci = iter.next();
      continue;
    }
    pciNum++;
  }
  // LCOV_EXCL_STOP
  return 0;
}
// LCOV_EXCL_START
//buffer should be 32 bytes
void PCIT::operandString(Int32 operand, char* buffer) {
  // str_sprintf(buffer, "%8.8X", operand);
  str_sprintf(buffer, "%09d", operand);
}

void PCode::print(PCIList code) {
#ifdef _DEBUG
  PCIListIter iter(code);
  for(PCI *pci = iter.first(); pci; pci = iter.next()) {
    pci->print();
    fprintf(stderr, "\n");
  }
#endif
}
#ifdef _DEBUG
void PCI::print() {
    char buffer[32];
    PCIT::operandString(operation_, buffer);
    fprintf(stderr, "%p,%3d,%d %s(", 
	    this, 
	    getCodePosition(),
	    getGeneratedCodeSize(),
	    buffer);
    if(getNumberAddressingModes() > 0)
      fprintf(stderr, "%s", 
	      PCIT::addressingModeString(getAddressingMode(0)));
    Int32 i=1;
    for(; i<getNumberAddressingModes(); i++)
      fprintf(stderr, ",%s", 
	     PCIT::addressingModeString(getAddressingMode(i)));
    fprintf(stderr, ") ");
    for(i=0; i<getNumberOperands(); i++) {
      PCIT::operandString(getOperand(i), buffer);
      fprintf(stderr, "%s ", buffer);
    }
};
#endif

void PCode::displayContents(PCIList code, Space *space) {
  char buf[256];
  char operandBuf[32];
  //  str_sprintf(buf, "  PCode:\n");

  PCIListIter iter(code);
  PCI * pci = iter.first();

  while (pci != NULL) {

    char tbuf[256];
    str_sprintf(buf, "    PCI(%09d)[%3d,%d] %s(", pci, pci->getCodePosition(),
	    pci->getGeneratedCodeSize(),
	    PCIT::operationString(pci->getOperation()));

    if(pci->getNumberAddressingModes() > 0) {
      str_sprintf(tbuf, "%s", 
	      PCIT::addressingModeString(pci->getAddressingMode(0)));
      str_cat(buf, tbuf, buf);
    }
    
    Int32 i=1;
    for(; i<pci->getNumberAddressingModes(); i++) {
      str_sprintf(tbuf, ",%s", 
	      PCIT::addressingModeString(pci->getAddressingMode(i)));
      str_cat(buf, tbuf, buf);
    }
    str_cat(buf, ") ", buf);
    for(i=0; i<pci->getNumberOperands(); i++) {
      PCIT::operandString(pci->getOperand(i), operandBuf);
      str_sprintf(tbuf, "%s ", operandBuf);
      str_cat(buf, tbuf, buf);
    }
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
   
    pci = iter.next();
  }
  
  str_sprintf(buf, "\n");
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
};

void PCode::displayContents(PCodeBinary* pCode, Space *space) {
  char buf[256];

  // skip over the ATP's
  Int32 length = *(pCode++);
  
  str_sprintf(buf, "Num ATP/ATPIndex pairs: %d", length);
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
  for (Int32 i = 0; i < length; i++)
    {
      str_sprintf(buf, "Pair #%d: atp = %d, atpindex = %d", 
		  i+1, pCode[0], pCode[1]);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      pCode += 2;
    }
  
  while (pCode[0] != PCIT::END)
    {
      Int32 opcode = *(pCode++);
      
      str_sprintf(buf, "    %s ", 
		  PCIT::instructionString(PCIT::Instruction(opcode)));
      
      Lng32 pcodeLength = PCode::getInstructionLength(pCode-1) - 1;
      
      char tbuf[256];
      char operandBuf[32];
      for (Int32 i=0; i<pcodeLength; i++) 
	{
	  PCIT::operandString(pCode[i], operandBuf);
	  str_sprintf(tbuf, "%s ", operandBuf);
	  str_cat(buf, tbuf, buf);
	}
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      pCode += pcodeLength;
    } // while
  
  str_sprintf(buf, "\n");
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
}

Int32 PCode::dumpContents(PCIList code, char * buf, Int32 bufLen) {
  char operandBuf[32];
  Int32 len = 0;

  PCIListIter iter(code);
  PCI * pci = iter.first();

  buf[0] = 0;
  while (pci != NULL) {

    char tbuf[256];
    str_sprintf(tbuf, "PCI(%09d)[%3d,%d] %s(", pci, pci->getCodePosition(),
	    pci->getGeneratedCodeSize(),
	    PCIT::operationString(pci->getOperation()));
    len += str_len(tbuf);
    if (len >= bufLen - 2)
      return len - str_len(tbuf);
    str_cat(buf, tbuf, buf);

    if(pci->getNumberAddressingModes() > 0) {
      str_sprintf(tbuf, "%s", 
	      PCIT::addressingModeString(pci->getAddressingMode(0)));
      len += str_len(tbuf);
      if (len >= bufLen - 2)
        return len - str_len(tbuf);
      str_cat(buf, tbuf, buf);
    }
    
    Int32 i=1;
    for(; i<pci->getNumberAddressingModes(); i++) {
      str_sprintf(tbuf, ",%s", 
	      PCIT::addressingModeString(pci->getAddressingMode(i)));
      len += str_len(tbuf);
      if (len >= bufLen - 2)
        return len - str_len(tbuf);
      str_cat(buf, tbuf, buf);
    }
    str_cat(buf, ") ", buf);
    for(i=0; i<pci->getNumberOperands(); i++) {
      PCIT::operandString(pci->getOperand(i), operandBuf);
      str_sprintf(tbuf, "%s ", operandBuf);
      len += str_len(tbuf);
      if (len >= bufLen - 2)
        return len - str_len(tbuf);
      str_cat(buf, tbuf, buf);
    }
    pci = iter.next();
  }
  
  buf[len] = '#';
  buf[len+1] = 0;
  return len;
};

void PCode::dumpContents(PCodeBinary* pCode, char* buf, Int32 bufLen) {
  char tbuf[256];
  Int32 len = 0;

  // skip over the ATP's
  Int32 length = *(pCode++);
  
  buf[0] = 0;
  str_sprintf(tbuf, "Num ATP/ATPIndex pairs: %d", length);
  len += str_len(tbuf);
  if (len >= bufLen - 2)
    return;
  str_cat(buf, tbuf, buf);
  for (Int32 i = 0; i < length; i++)
    {
      str_sprintf(tbuf, "Pair #%d: atp = %d, atpindex = %d", 
		  i+1, pCode[0], pCode[1]);
      len += str_len(tbuf);
      if (len >= bufLen - 2)
        return;
      str_cat(buf, tbuf, buf);
      pCode += 2;
    }
  
  while (pCode[0] != PCIT::END)
    {
      Int32 opcode = *(pCode++);
      
      str_sprintf(tbuf, "    %s ", 
		  PCIT::instructionString(PCIT::Instruction(opcode)));
      len += str_len(tbuf);
      if (len >= bufLen - 2)
        break;
      str_cat(buf, tbuf, buf);
      
      Lng32 pcodeLength = PCode::getInstructionLength(pCode-1) - 1;
      
      char operandBuf[32];
      for (Int32 i=0; i<pcodeLength; i++) 
	{
	  PCIT::operandString(pCode[i], operandBuf);
	  str_sprintf(tbuf, "%s ", operandBuf);
          len += str_len(tbuf);
          if (len >= bufLen - 2)
            break;
	  str_cat(buf, tbuf, buf);
	}
      pCode += pcodeLength;
    } // while
  
  buf[len] = '#';
  buf[len+1] = 0;
}

// LCOV_EXCL_STOP
PCodeBinary * PCode::generateCodeSegment(Int32 length,
				  Int32 *atpCode,
				  Int32 *atpIndexCode,
				  Int32 *codeSize) {

  // Generate the code
  //
  Int32 pc = 0;

  PCodeBinary *code = new(space_) PCodeBinary[size() + 1 + (1 + 2 * length)];
  *codeSize = sizeof(PCodeBinary) * (size() + 1 + (1 + 2 * length));

  code[pc++] = (Int32)length;
  for(Int32 i=0; i<length; i++) {
    code[pc++] = atpCode[i];
    code[pc++] = atpIndexCode[i];
  }
  PCIListIter iter(pciList_);
  for(PCI *pci = iter.first(); pci; pci = iter.next()) {
    if(pci->getOperation() == PCIT::Op_TARGET) 
      continue;
    code[pc++] = getInstruction(pci);
    for(Int32 i=0; i<pci->getNumberOperands(); i++)
      code[pc++] = pci->getOperand(i);
  }
  code[pc++] = PCIT::END;

  return code;
}


PCIT::AddressingMode PCIT::getMemoryAddressingMode(Int32 datatype)
{
  switch(datatype)
    {
    case REC_BIN8_UNSIGNED: return PCIT::MBIN8U; break;
    case REC_BIN8_SIGNED:   return PCIT::MBIN8S; break;

    case REC_BIN16_UNSIGNED: 
    case REC_BPINT_UNSIGNED: return PCIT::MBIN16U; break;

    case REC_BIN16_SIGNED:   return PCIT::MBIN16S; break;
    case REC_BIN32_UNSIGNED: return PCIT::MBIN32U; break;
    case REC_BIN32_SIGNED:   return PCIT::MBIN32S; break;
    case REC_BIN64_UNSIGNED: return PCIT::MBIN64U; break;
    case REC_BIN64_SIGNED:   return PCIT::MBIN64S; break;
    case REC_IEEE_FLOAT32:   return PCIT::MFLT32; break;
    case REC_IEEE_FLOAT64:   return PCIT::MFLT64; break;
    case REC_DECIMAL_LSE:    return PCIT::MDECS; break;
    case REC_DECIMAL_UNSIGNED: return PCIT::MDECU; break;

    case REC_BOOLEAN:        return PCIT::MASCII; break;
    case REC_BYTE_F_ASCII: return PCIT::MASCII; break;

    case REC_BYTE_V_ASCII: return PCIT::MATTR5; break;
    case REC_NCHAR_F_UNICODE: return PCIT::MUNI; break;
    case REC_NCHAR_V_UNICODE: return PCIT::MUNIV; break;
    case REC_NUM_BIG_UNSIGNED: return PCIT::MBIGU; break;
    case REC_NUM_BIG_SIGNED: return PCIT::MBIGS; break;
    };
  return PCIT::AM_NONE;
}

Int16 PCIT::getDataTypeForMemoryAddressingMode(PCIT::AddressingMode am)
{
  switch(am)
  {
    case PCIT::MBIN8S: return REC_BIN8_SIGNED; break;
    case PCIT::MBIN8U: return REC_BIN8_UNSIGNED; break;
    case PCIT::IBIN16U:
    case PCIT::MBIN16U: return REC_BIN16_UNSIGNED; break;
    case PCIT::MBIN16S: return REC_BIN16_SIGNED; break;
    case PCIT::MBIN32U: return REC_BIN32_UNSIGNED; break;
    case PCIT::IBIN32S: 
    case PCIT::MBIN32S: return REC_BIN32_SIGNED; break;
    case PCIT::IBIN64S:
    case PCIT::MBIN64S: return REC_BIN64_SIGNED; break;
    case PCIT::MBIN64U: return REC_BIN64_UNSIGNED; break;
    case PCIT::MFLT32: return REC_IEEE_FLOAT32; break;
    case PCIT::MFLT64: return REC_IEEE_FLOAT64; break;
    case PCIT::MDECS: return REC_DECIMAL_LSE; break;
    case PCIT::MDECU: return REC_DECIMAL_UNSIGNED; break;
    case PCIT::MASCII: return REC_BYTE_F_ASCII; break;
    case PCIT::MBIGU: return REC_NUM_BIG_UNSIGNED; break;
    case PCIT::MBIGS: return REC_NUM_BIG_SIGNED; break;
  };
  return -1;
}


Int32 PCIT::getNumberOperandsForAddressingMode(PCIT::AddressingMode am)
{
  switch(am)
    {
    case PCIT::MATTR6:
      return 7;
    case PCIT::MUNIV:
    case PCIT::MATTR5:
      return 6;
    case PCIT::MATTR4:
      return 5;
    case PCIT::MATTR3:
      return 4;
    case PCIT::IATTR4:
      return 4;
    case PCIT::IATTR3:
      return 3;

    case PCIT::IATTR2:
      return 2;

    case PCIT::MBIN8:
    case PCIT::MBIN8S:
    case PCIT::MBIN8U:
    case PCIT::MBIN16U:
    case PCIT::MBIN16S:
    case PCIT::MBIN32U:
    case PCIT::MBIN32S:
    case PCIT::MBIN64U:
    case PCIT::MBIN64S:
    case PCIT::MFLT32:
    case PCIT::MFLT64:
    case PCIT::MPTR32:
    case PCIT::MDECS:
    case PCIT::MDECU:
    case PCIT::MASCII:
    case PCIT::MUNI:
    case PCIT::MBIGS:
    case PCIT::MBIGU:
      return 3;

    case PCIT::IBIN64S:
      return 2;

    case PCIT::IBIN8U:
    case PCIT::IBIN16U:
    case PCIT::IBIN32S:
      return 1;
    case PCIT::IPTR:
      return PCODEBINARIES_PER_PTR;
    };
  return 0;
}


Int32 PCIT::getOperandLengthForAddressingMode(PCIT::AddressingMode am)
{
  switch(am)
    {
    case PCIT::MBIGS:
    case PCIT::MBIGU:
    case PCIT::MUNI:
    case PCIT::MUNIV:
    case PCIT::MATTR6:
    case PCIT::MATTR5:
    case PCIT::MATTR4:
    case PCIT::MATTR3:
    case PCIT::MPTR32:
    case PCIT::MASCII:
    case PCIT::MDECS:
    case PCIT::MDECU:
    case PCIT::MBIN8S:
    case PCIT::MBIN8U:
    case PCIT::IBIN8U:
    case PCIT::IATTR4:
    case PCIT::IATTR3:
    case PCIT::IATTR2:
      return 1;
    case PCIT::MBIN16U:
    case PCIT::MBIN16S:
    case PCIT::IBIN16U:
      return 2;
    case PCIT::MBIN32U:
    case PCIT::MBIN32S:
    case PCIT::MFLT32:
    case PCIT::IBIN32S:
      return 4;
    case PCIT::MBIN64U:
    case PCIT::MBIN64S:
    case PCIT::MFLT64:
    case PCIT::IBIN64S:
      return 8;
    case PCIT::IPTR:
      return sizeof(char*);
    default :
      NAAssert("Invalid addressing mode passed to PCIT::getOperandLengthForAddressingMode", __FILE__ , __LINE__ );
      return 0;

    };
}

Int32 PCIT::isMemoryAddressingMode(PCIT::AddressingMode am)
{
  switch(am)
    {
    case PCIT::MBIN8:
    case PCIT::MBIN8U:
    case PCIT::MBIN8S:
    case PCIT::MBIN16U:
    case PCIT::MBIN16S:
    case PCIT::MBIN32U:
    case PCIT::MBIN32S:
    case PCIT::MBIN64U:
    case PCIT::MBIN64S:
    case PCIT::MFLT32:
    case PCIT::MFLT64:
    case PCIT::MPTR32:
    case PCIT::MDECS:
    case PCIT::MDECU:
    case PCIT::MASCII:
    case PCIT::MBIGS:
    case PCIT::MBIGU:
    case PCIT::MATTR3:
    case PCIT::MATTR4:
    case PCIT::MATTR5:
    case PCIT::MATTR6:
    case PCIT::MUNI:
    case PCIT::MUNIV:
      return 1;
    };
  return 0;
}

const char *PCIT::operationString(PCIT::Operation op) {
  switch(op) {
    /*  case PCIT::Op_ATP_VCX: return("ATP_VCX");
  case PCIT::Op_LEA_ATP: return("LEA_ATP");
  case PCIT::Op_LD_ATP: return("LD_ATP");
  case PCIT::Op_LD_UNALIGNED_ATP: return("LD_UNALIGNED_ATP");
  case PCIT::Op_MV_ATP_N: return("MV_N");*/
  // LCOV_EXCL_START
  case PCIT::Op_OPDATA: return("OPDATA");

  case PCIT::Op_MOVE: return("MOVE");
  case PCIT::Op_NULL: return("NULL");
  case PCIT::Op_NOT_NULL_BRANCH: return("NOT_NULL_BRANCH");
  case PCIT::Op_NOT_NULL_BRANCH_COMP: return("NOT_NULL_BRANCH_COMP");
  case PCIT::Op_NOT_NULL_BITMAP_BRANCH: return("NOT_NULL_BITMAP_BRANCH");
  case PCIT::Op_NULL_BITMAP: return("NULL_BITMAP");
  case PCIT::Op_NULL_BYTES:  return("NULL_BYTES");
  case PCIT::Op_NULL_VIOLATION: return("NULL_VIOLATION");

  case PCIT::Op_UPDATE_ROWLEN:  return("UPDATE_ROWLEN");
  case PCIT::Op_UPDATE_ROWLEN2:  return("UPDATE_ROWLEN2");
  case PCIT::Op_UPDATE_ROWLEN3:  return("UPDATE_ROWLEN3");

  case PCIT::Op_EQ: return("EQ");
  case PCIT::Op_LT: return("LT");
  case PCIT::Op_GT: return("GT");
  case PCIT::Op_LE: return("LE");
  case PCIT::Op_GE: return("GE");
  case PCIT::Op_NE: return("NE");
  case PCIT::Op_COMP: return("COMP");
  case PCIT::Op_ZERO: return("ZERO");
  case PCIT::Op_NOTZERO: return("NOTZERO");

  case PCIT::Op_AND: return("AND");
  case PCIT::Op_OR: return("OR");
  // LCOV_EXCL_STOP
  case PCIT::Op_SUM: return("SUM");
  case PCIT::Op_MINMAX: return("MINMAX");
  case PCIT::Op_ADD: return("ADD");
  case PCIT::Op_SUB: return("SUB");
  case PCIT::Op_MUL: return("MUL");
  case PCIT::Op_DIV: return("DIV");
  case PCIT::Op_DIV_ROUND: return("DIV_ROUND");
  case PCIT::Op_MOD: return("MOD");

  case PCIT::Op_BITOR: return("BITOR");

  case PCIT::Op_HASHCOMB: return("HASHCOMB");

  case PCIT::Op_ENCODE: return("ENCODE");
  case PCIT::Op_DECODE: return("DECODE");

  case PCIT::Op_GENFUNC: return("GENFUNC");
  // LCOV_EXCL_START
  case PCIT::Op_HASH: return("HASH");
  case PCIT::Op_FILL_MEM_BYTES: return("FILL_MEM_BYTES");
  case PCIT::Op_RETURN: return("RETURN");

  case PCIT::Op_BRANCH: return("BRANCH");
  case PCIT::Op_BRANCH_AND: return("BRANCH_AND");
  case PCIT::Op_BRANCH_OR: return("BRANCH_OR");
  case PCIT::Op_TARGET: return("TARGET");
  // LCOV_EXCL_STOP
  case PCIT::Op_RANGE_LOW: return("RANGE_LOW");
  case PCIT::Op_RANGE_HIGH: return("RANGE_HIGH");
  // LCOV_EXCL_START
  case PCIT::Op_REPLACE_NULL: return("REPLACE_NULL");

  case PCIT::Op_CLAUSE: return("CLAUSE");
  case PCIT::Op_CLAUSE_BRANCH: return("CLAUSE_BRANCH");
  case PCIT::Op_CLAUSE_EVAL: return("CLAUSE_EVAL");

  case PCIT::Op_PROFILE: return("PROFILE");

  case PCIT::Op_NOP: return("NOP");

  case PCIT::Op_HASH2_DISTRIB: return("HASH2_DISTRIB");

  case PCIT::Op_OFFSET: return("OFFSET");

  case PCIT::Op_HDR: return("HEADER");

  case PCIT::Op_COPYVARROW: return("COPYVARROW");

  case PCIT::Op_CONV_VC_PTR: return("CONV_VC_PTR");

  case PCIT::Op_END: return("END");
  // LCOV_EXCL_STOP
  default: return("UNKNOWN OPERATION");
  }
}

const char *PCIT::addressingModeString(PCIT::AddressingMode am) {
  switch(am) {
  case PCIT::MBIN8: return("MBIN8");
  case PCIT::MBIN8S: return("MBIN8S");
  case PCIT::MBIN8U: return("MBIN8U");
  case PCIT::MBIN16U: return("MBIN16U");
  case PCIT::MBIN16S: return("MBIN16S");
  case PCIT::MBIN32U: return("MBIN32U");
  case PCIT::MBIN32S: return("MBIN32S");
  case PCIT::MBIN64U: return("MBIN64U");
  case PCIT::MBIN64S: return("MBIN64S");
  case PCIT::MFLT32: return("MFLT32");
  case PCIT::MFLT64: return("MFLT64");
  case PCIT::MPTR32: return("MPTR32");
  case PCIT::MDECS: return("MDECS");
  case PCIT::MDECU: return("MDECU");
  case PCIT::MASCII: return("MASCII");
  case PCIT::MBIGS: return("MBIGS");
  case PCIT::MBIGU: return("MBIGU");

  case PCIT::IBIN8U: return("IBIN8U");
  case PCIT::IBIN16U: return("IBIN16U");
  case PCIT::IBIN64S: return("IBIN64S");
  // LCOV_EXCL_START
  case PCIT::IBIN32S: return("IBIN32S");
  case PCIT::IATTR4: return("IATTR4");
  case PCIT::IATTR3: return("IATTR3");
  case PCIT::IATTR2: return("IATTR2");

  case PCIT::MATTR3: return("MATTR3");
  case PCIT::MATTR4: return("MATTR4");
  // LCOV_EXCL_STOP
  case PCIT::MATTR5: return("MATTR5");
  // LCOV_EXCL_START
  case PCIT::MATTR6: return("MATTR6");

  case PCIT::MUNI: return("MUNI");
  case PCIT::MUNIV: return("MUNIV");
  // LCOV_EXCL_STOP

  case PCIT::IPTR: return("IPTR");

  default: return("UNKNOWN ADDRESSING MODE");
  }
};


char *PCIT::instructionString(PCIT::Instruction instr) {
  return (char *)PCode::getOpcodeString(instr);
}
Int32 PCI::getGeneratedCodeSize() {
  switch(getOperation()) {
  case PCIT::Op_TARGET:
    return 0;
    break;
  }

  return 1 + getNumberOperands();
}

Int32 PCI::getMemoryOperandLength(PCIT::AddressingMode am, Int32 op)
{
  switch(PCode::getInstruction(this)) {
    case PCIT::OPDATA_MPTR32_IBIN32S:
      return 0;
    case PCIT::HASH_MBIN32U_MPTR32_IBIN32S_IBIN32S:
      if (am == PCIT::MBIN32U)
        return 4;
      if (am == PCIT::MPTR32)
        return getOperand(7);
      return 0;
    case PCIT::ENCODE_MASCII_MBIN8S_IBIN32S:
    case PCIT::ENCODE_MASCII_MBIN8U_IBIN32S:
    case PCIT::DECODE_MASCII_MBIN8S_IBIN32S:
    case PCIT::DECODE_MASCII_MBIN8U_IBIN32S:
     return 1; 
    case PCIT::ENCODE_MASCII_MBIN16S_IBIN32S:
    case PCIT::ENCODE_MASCII_MBIN16U_IBIN32S:
    case PCIT::DECODE_MASCII_MBIN16S_IBIN32S:
    case PCIT::DECODE_MASCII_MBIN16U_IBIN32S:
     return 2; 
    case PCIT::ENCODE_MASCII_MBIN32S_IBIN32S:
    case PCIT::ENCODE_MASCII_MBIN32U_IBIN32S:
    case PCIT::DECODE_MASCII_MBIN32S_IBIN32S:
    case PCIT::DECODE_MASCII_MBIN32U_IBIN32S:
      return 4;
    case PCIT::ENCODE_MASCII_MBIN64S_IBIN32S:
    case PCIT::DECODE_MASCII_MBIN64S_IBIN32S:
      return 8;
    case PCIT::FILL_MEM_BYTES:
      return getOperand(getNumberOperands()-2);
    default:
      if (am == PCIT::MBIN8) {
        // Some PCI instructions have multiple MBIN8 operands with potentially
        // different lengths, and so it's important to associate the right
        // operand with the right length.  From what I can tell, however,
        // CONCAT is the only one. 
        if ((getOperation() == PCIT::Op_GENFUNC) &&
            (getOperand(0) == ITM_CONCAT) && (op == 4)) {

          // Length of 1st operand is second to last PCI operand
          return getOperand(getNumberOperands()-2);
        }
        
        // Otherwise the length should be the last one.
        return getOperand(getNumberOperands()-1);
      }
      else if ((am == PCIT::MBIGU) || (am == PCIT::MBIGS)) {
        return getOperand(getNumberOperands()-1);
      }
      return PCIT::getOperandLengthForAddressingMode(am);
  }
}

// Load the target null bitmap address so it can be set if needed.
PCIList PCode::loadOpDataNullBitmapAddress(Attributes *attr,
                                           Int32         loc,
                                           CollHeap   *heap)
{
  return loadOpDataAddress(attr, attr->getNullIndOffset(), loc, heap);
}

PCIList PCode::loadOpDataNullAddress(Attributes *attr, Int32 loc, CollHeap *heap)
{
  return loadOpDataAddress(attr, attr->getNullIndOffset(), loc, heap);
}

PCIList PCode::loadOpDataVCAddress(Attributes *attr, Int32 loc, CollHeap *heap)
{
  if(attr->isIndirectVC())
    {
      // For indirect varchars, use the OPDATA_MATTR5_IBIN32 instruction.
      // This will set both the vclen pointer and the data pointer in the
      // op_data array.
      UInt32 comboLen = 0;
      char* comboPtr = (char*)&comboLen;

      // Use combo fields for specifying vc/null lengths
      comboPtr[0] = (char)attr->getNullIndicatorLength();
      comboPtr[1] = (char)attr->getVCIndicatorLength();

      PCIList pciList(heap);
      AML aml(PCIT::MATTR5, PCIT::IBIN32S);
      OL ol(attr->getAtp(), attr->getAtpIndex(), attr->getOffset(),
            attr->getVoaOffset(), attr->getLength(), comboLen, 
            ex_clause::MAX_OPERANDS + loc);
      PCI pci(PCIT::Op_OPDATA, aml, ol);
      pciList.append(pci);
      return pciList;
    }
  else
    {
      return loadOpDataAddress(attr, attr->getVCLenIndOffset(), 
                               ex_clause::MAX_OPERANDS + loc, heap);
    }
}

PCIList PCode::loadOpDataDataAddress(Attributes *attr, Int32 loc, CollHeap *heap)
{
  return loadOpDataAddress(attr, (Int32)attr->getOffset(), 
                           2 * ex_clause::MAX_OPERANDS + loc, heap);
}

PCIList PCode::loadOpDataAddress(Attributes *attr, Int32 offset, Int32 loc, 
                                 CollHeap *heap)
{
  PCIList pciList(heap);
  AML aml(PCIT::MPTR32, PCIT::IBIN32S);
  OL ol(attr->getAtp(), attr->getAtpIndex(), offset, loc);
  PCI pci(PCIT::Op_OPDATA, aml, ol);
  pciList.append(pci);
  return pciList;
}

PCIList PCode::loadOpDataNullBitmap(Attributes *attr, Int32 loc, CollHeap *heap)
{
  PCIList pciList(heap);
  AML aml(PCIT::MPTR32, PCIT::IBIN32S, PCIT::IBIN32S);
  OL ol(attr->getAtp(), attr->getAtpIndex(), attr->getNullIndOffset(),
        attr->getNullBitIndex(), loc);
  PCI pci(PCIT::Op_OPDATA, aml, ol);
  pciList.append(pci);
  return pciList;
}

PCIList PCode::loadOpDataNull(Attributes *attr, Int32 loc, CollHeap *heap)
{
  PCIList pciList(heap);
  AML aml(PCIT::MBIN16U, PCIT::IBIN32S);
  OL ol(attr->getAtp(), attr->getAtpIndex(), attr->getNullIndOffset(), loc);
  PCI pci(PCIT::Op_OPDATA, aml, ol);
  pciList.append(pci);
  return pciList;
}

PCIList PCode::storeValue(Int32 value, Attributes *attr, CollHeap *heap)
{
  return storeValue(value, attr, attr->getOffset(), heap);
}

PCIList PCode::storeValue(Int32 value, Attributes *attr, UInt32 offset, 
                          CollHeap *heap)
{
  if ( attr->isSQLMXAlignedFormat() )
    return( storeShortValue( (Int32) value, attr, offset, heap ) );

  PCIList pciList(heap);

  AML aml(PCIT::MBIN32S, PCIT::IBIN32S);
  OL ol(attr->getAtp(), attr->getAtpIndex(), (Int32)offset, value);
  PCI pci(PCIT::Op_MOVE, aml, ol);
  pciList.append(pci);
  return pciList;
}

PCIList PCode::storeShortValue(UInt32      value,
                               Attributes *attr,
                               UInt32      offset, 
                               CollHeap   *heap)
{
  PCIList pciList(heap);

  AML aml(PCIT::MBIN16U, PCIT::IBIN16U);
  OL ol(attr->getAtp(), attr->getAtpIndex(), (Int32)offset, (Int16)value);
  PCI pci(PCIT::Op_MOVE, aml, ol);
  pciList.append(pci);
  return pciList;
}

PCIList PCode::storeShortVoa(Attributes *attr, CollHeap *heap)
{
  PCIList pciList(heap);

  // The voaOffset data member is only set for both of the SQL/MX disk
  // formats for the first fixed field and all variable length fields.
  // Other data formats have the voaOffset initialized to ExpOffsetMax.
  if ((attr->isSQLMXDiskFormat()           &&
       (attr->getVoaOffset() != ExpOffsetMax) &&
       (attr->getOffset() != ExpOffsetMax)))
  {
    pciList.append(storeShortValue(
                            attr->getOffset() - attr->getVCIndicatorLength(),
                            attr,
                            attr->getVoaOffset(),
                            heap
                            ));
  }
  return pciList;
}

PCIList PCode::storeVoa(Attributes *attr, CollHeap *heap)
{
  // The voaOffset data member is only set for SQLMX and SQLMX_ALIGNED format.
  // Other data formats have the voaOffset initialized to ExpOffsetMax.
  // The voa offset is 0 for the first fixed field offset and this is handled
  // by a separate clause now ExHeaderClause.
  if ( (attr->getVoaOffset() > 0)             &&
       (attr->getVoaOffset() != ExpOffsetMax) &&
       (attr->getOffset() != ExpOffsetMax)    &&
       attr->isSQLMXDiskFormat() )
  {
    if ( attr->isSQLMXAlignedFormat() )
      return( storeShortVoa( attr, heap ) );
    else
      return storeVoaValue(attr,
                           attr->getVoaOffset(),
                           (attr->getOffset() -
                            attr->getVCIndicatorLength() -
                            attr->getNullIndicatorLength()),
                           heap);
  }

  PCIList pciList(heap);

  return pciList;
}

PCIList PCode::storeVoaValue(Attributes *attr, UInt32 offset,
			     UInt32 value, CollHeap *heap,
			     short varOnly) {
  PCIList pciList(heap);

  if (attr->isSQLMXDiskFormat() &&
      (!varOnly || (varOnly && (attr->getVCIndicatorLength() > 0))))
  {
    pciList.append(PCode::storeValue(value, attr, (Int32)offset, heap));
  }
  return pciList;
}

PCIList PCode::updateRowLen(Attributes *attr, CollHeap *heap, UInt32 flags)
{
  PCIList pciList(heap);

  if (attr->getVCIndicatorLength() > 0 &&
      attr->isSQLMXDiskFormat())
  {
    UInt32 comboLen1 = 0;
    char* comboPtr1 = (char*)&comboLen1;    
    comboPtr1[0] = (char)attr->getNullIndicatorLength(),
    comboPtr1[1] = (char)attr->getVCIndicatorLength();

    UInt32 alignment =  (attr->isSQLMXAlignedFormat() &&
                        (attr->getNextFieldIndex() == ExpOffsetMax))
                        ? ExpAlignedFormat::ALIGNMENT : 0;

    // Use a length of -1 to indicate that this instruction in effect
    // operates on the whole row.  If the attr length is used, then we
    // are subject to copy propagation and other pcode optimizations.
    // If this happens then we might end up computing the rowlen for some
    // other tuple that might not even be a row in Aligned Format.
    AML aml(PCIT::MATTR5, PCIT::IBIN32S);
    OL ol(attr->getAtp(), attr->getAtpIndex(), (Int32)attr->getOffset(),
          (Int32)attr->getVoaOffset(), -1, comboLen1, 
          alignment);
          
    PCI pci(PCIT::Op_UPDATE_ROWLEN3, aml, ol);
    pciList.append(pci);
  }

  return pciList;
}

// Move fixed source to fixed destination.
PCIList PCode::moveValue(Attributes *dst, Attributes *src, CollHeap *heap)
{
  if (dst->getLength() == src->getLength())
    {
      PCIList code(heap);
      
      AML aml(PCIT::MBIN8,PCIT::MBIN8,PCIT::IBIN32S);
      OL ol(dst->getAtp(), dst->getAtpIndex(), (Int32)dst->getOffset(),
	    src->getAtp(), src->getAtpIndex(), (Int32)src->getOffset(),
	    (Int32)dst->getLength());
      PCI pci(PCIT::Op_MOVE, aml, ol);
      code.append(pci);
      
      return code;
    }
  else
    {
      PCIList code(heap);
      
      AML aml(PCIT::getMemoryAddressingMode(dst->getDatatype()),
              PCIT::getMemoryAddressingMode(src->getDatatype()),
	      PCIT::IBIN32S,PCIT::IBIN32S);
      OL ol(dst->getAtp(), dst->getAtpIndex(), (Int32)dst->getOffset(),
	    src->getAtp(), src->getAtpIndex(), (Int32)src->getOffset(),
	    (Int32)dst->getLength(),
	    (Int32)src->getLength());
      PCI pci(PCIT::Op_MOVE, aml, ol);
      code.append(pci);

      return code;
    }
}

PCIList PCode::copyVarRow(Attributes *dst,
                          Attributes *src,
                          UInt32 lastVOAoffset,
                          Int16 lastVcIndicatorLength,
                          Int16 lastNullIndicatorLength,
                          Int16 alignment,
                          CollHeap *heap)
{
  PCIList pciList(heap);
  if (lastVOAoffset > 0 && lastVcIndicatorLength > 0 )
  {
    UInt32 comboLen1 = 0;
    char* comboPtr1 = (char*)&comboLen1;
    comboPtr1[0] = (char)lastNullIndicatorLength,
    comboPtr1[1] = (char)lastVcIndicatorLength;

    AML aml(PCIT::MBIN8, PCIT::MBIN8, PCIT::IBIN32S, PCIT::IBIN32S, PCIT::IBIN32S,PCIT::IBIN32S);
    OL ol(dst->getAtp(), dst->getAtpIndex(), (Int32)dst->getOffset(),
          src->getAtp(), src->getAtpIndex(), (Int32)src->getOffset(),
          (Int32)lastVOAoffset,
          (Int32)comboLen1,
          (Int32)alignment,
          (Int32) ((SimpleType*)src)->getLength());

    PCI pci(PCIT::Op_COPYVARROW, aml, ol);
    pciList.append(pci);
  }
  return pciList;
}
// Move varchar source to varchar destination.
// source or destination can be "indirect"
// For disk format, it handles setting the VOA in the target.
PCIList PCode::moveVarcharValue(Attributes *dst,
                                Attributes *src,
                                CollHeap   *heap)
{
  PCIList code(heap);

  // Handles all varchar's in disk format - ones with a fixed offset and
  // "indirect" varchar's where the offset must be read from the VOA.
  // Handles setting the VOA in the target for the varchar being moved too.

  UInt32 comboLen1 = 0;
  UInt32 comboLen2 = 0;
  char* comboPtr1 = (char*)&comboLen1;
  char* comboPtr2 = (char*)&comboLen2;

  // Use combo fields for specifying vc/null lengths of both operands
  comboPtr1[0] = (char)dst->getNullIndicatorLength(),
  comboPtr1[1] = (char)dst->getVCIndicatorLength();
  comboPtr2[0] = (char)src->getNullIndicatorLength(),
  comboPtr2[1] = (char)src->getVCIndicatorLength();

  AML aml(PCIT::MATTR5,PCIT::MATTR5);
  OL ol(dst->getAtp(), dst->getAtpIndex(), dst->getOffset(),
          dst->getVoaOffset(), dst->getLength(), comboLen1,
        src->getAtp(), src->getAtpIndex(), src->getOffset(),
          src->getVoaOffset(), src->getLength(), comboLen2);
  PCI pci(PCIT::Op_MOVE, aml, ol);
  code.append(pci);

  return code;
}

// Move varchar source to fixed destination.
// source or destination can be "indirect"
// For disk format, it handles setting the VOA in the target.
PCIList PCode::moveVarcharFixedValue(Attributes *dst,
                                     Attributes *src,
                                     CollHeap   *heap)
{
  PCIList code(heap);

  UInt32 comboLen1 = 0;
  char* comboPtr1 = (char*)&comboLen1;

  // Use combo fields for specifying vc/null lengths of both operands
  comboPtr1[0] = (char)src->getNullIndicatorLength(),
  comboPtr1[1] = (char)src->getVCIndicatorLength();

  AML aml(PCIT::MASCII,PCIT::MATTR5,PCIT::IBIN32S);
  OL ol(dst->getAtp(), dst->getAtpIndex(), (Int32)dst->getOffset(),
        src->getAtp(), src->getAtpIndex(), (Int32)src->getOffset(),
        (Int32)src->getVoaOffset(), src->getLength(), comboLen1,
        (Int32)dst->getLength());
  PCI pci(PCIT::Op_MOVE, aml, ol);
  code.append(pci);

  return code;
}

// Move fixed source to varchar destination.
// source or destination can be "indirect"
// For disk format, it handles setting the VOA in the target.
PCIList PCode::moveFixedVarcharValue(Attributes *dst,
                                     Attributes *src,
                                     CollHeap   *heap)
{
  PCIList code(heap);

  UInt32 comboLen1 = 0;
  char* comboPtr1 = (char*)&comboLen1;

  // Use combo fields for specifying vc/null lengths of both operands
  comboPtr1[0] = (char)dst->getNullIndicatorLength();
  comboPtr1[1] = (char)dst->getVCIndicatorLength();

  AML aml(PCIT::MATTR5,PCIT::MASCII,PCIT::IBIN32S);
  OL ol(dst->getAtp(), dst->getAtpIndex(), (Int32)dst->getOffset(),
        (Int32)dst->getVoaOffset(), (Int32)dst->getLength(), comboLen1,
        src->getAtp(), src->getAtpIndex(), (Int32)src->getOffset(),
        (Int32)src->getLength());
  PCI pci(PCIT::Op_MOVE, aml, ol);
  code.append(pci);

  return code;
}

// Convert varchar ptr source to destination.
// source is a varchar ptr.
PCIList PCode::convertVarcharPtrToTarget(Attributes *dst,
					  Attributes *src,
					  CollHeap   *heap)
{
  PCIList code(heap);

  UInt32 comboLen1 = 0;
  char* comboPtr1 = (char*)&comboLen1;
  UInt32 comboLen2 = 0;
  char* comboPtr2 = (char*)&comboLen2;

  // Use combo fields for specifying vc/null lengths of both operands
  comboPtr1[0] = (char)src->getNullIndicatorLength(),
  comboPtr1[1] = (char)src->getVCIndicatorLength();
  if (dst->getDatatype() == REC_BYTE_V_ASCII)
    {  
      comboPtr2[0] = (char)dst->getNullIndicatorLength();
      comboPtr2[1] = (char)dst->getVCIndicatorLength();
    }

  if (dst->getDatatype() == REC_FLOAT32)
    {
      AML aml(PCIT::MFLT32,PCIT::MATTR5,PCIT::IBIN32S);
      OL ol(dst->getAtp(), dst->getAtpIndex(), (Int32)dst->getOffset(),
	    src->getAtp(), src->getAtpIndex(), (Int32)src->getOffset(),
	    (Int32)src->getVoaOffset(), src->getLength(), comboLen1,
	    (Int32)dst->getLength());
      PCI pci(PCIT::Op_CONV_VC_PTR, aml, ol);
      code.append(pci);
    }
  else if (dst->getDatatype() == REC_BIN32_SIGNED)
    {
      AML aml(PCIT::MBIN32S,PCIT::MATTR5,PCIT::IBIN32S);
      OL ol(dst->getAtp(), dst->getAtpIndex(), (Int32)dst->getOffset(),
	    src->getAtp(), src->getAtpIndex(), (Int32)src->getOffset(),
	    (Int32)src->getVoaOffset(), src->getLength(), comboLen1,
	    (Int32)dst->getLength());
      PCI pci(PCIT::Op_CONV_VC_PTR, aml, ol);
      code.append(pci);
    }
  else if (dst->getDatatype() == REC_BIN64_SIGNED)
    {
      AML aml(PCIT::MBIN64S,PCIT::MATTR5,PCIT::IBIN32S);
      OL ol(dst->getAtp(), dst->getAtpIndex(), (Int32)dst->getOffset(),
	    src->getAtp(), src->getAtpIndex(), (Int32)src->getOffset(),
	    (Int32)src->getVoaOffset(), src->getLength(), comboLen1,
	    (Int32)dst->getLength());
      PCI pci(PCIT::Op_CONV_VC_PTR, aml, ol);
      code.append(pci);
    }
  else if (dst->getDatatype() == REC_BYTE_V_ASCII)
    {
      AML aml(PCIT::MATTR5,PCIT::MATTR5);
      OL ol(dst->getAtp(), dst->getAtpIndex(), dst->getOffset(),
          dst->getVoaOffset(), dst->getLength(), comboLen2,
        src->getAtp(), src->getAtpIndex(), src->getOffset(),
          src->getVoaOffset(), src->getLength(), comboLen1);
      PCI pci(PCIT::Op_CONV_VC_PTR, aml, ol);
      code.append(pci);
    }

  return code;
}

// notNullBranch, when specified, indicates a (previous) code location of
// a branch instruction that wants to skip this code (in case
// the value is not NULL). nulBranch is the code location of the final
// branch that gets generated here to take after the zero fill is done.
//
PCIID PCode::generateJumpAndBranch(Attributes *dst, 
				   PCIList& code,
				   PCIID notNullBranch,
				   Int32 genUncondJump)
{
  PCIID nulBranch = 0;

  // generate an unconditional jump over the code that handles the
  // not-NULL case
  if (genUncondJump)
    {
      AML aml(PCIT::IBIN32S);
      OL ol((PCIType::Operand)0);
      PCI pci(PCIT::Op_BRANCH, aml, ol);
      code.append(pci);
      nulBranch = code.getTailId();
    }

  // jump target for the case that we move a non-NULL value
  if (notNullBranch)
    {
      AML aml;
      OL ol((Int64) notNullBranch);
      PCI pci(PCIT::Op_TARGET, aml, ol);
      code.append(pci);
    }

  return nulBranch;
}

// Adds code to zero-fill the data portion of a NULL value.
// notNullBranch, when specified, indicates a (previous) code location of
// a branch instruction that wants to skip this code (in case
// the value is not NULL). nulBranch is the code location of the final
// branch that gets generated here to take after the zero fill is done.
//
PCIID PCode::zeroFillNullValue(Attributes *dst, 
			       PCIList& code,
			       PCIID notNullBranch,
			       Int32 genUncondJump)
{
  PCIID nulBranch = 0;

  // Zero fill the destination.

  // If the destination is a variable length field, then do this separately.
  // The voa offset will be written here too rather than generate a separate
  // PCI to do this.
  if (dst->isVariableLength())
  {
    UInt32 comboLen1 = 0;
    char* comboPtr1 = (char*)&comboLen1;    
    comboPtr1[0] = (char)dst->getNullIndicatorLength(),
    comboPtr1[1] = (char)dst->getVCIndicatorLength();

    UInt32 len;
    // if going to disk format, clearing vcIndLen is sufficient
    if (dst->isSQLMXDiskFormat()  ||
        dst->isTrafodionDiskFormat())
      len = 0;
    else
      len = dst->getLength();

    AML aml(PCIT::MATTR5, PCIT::IBIN32S, PCIT::IBIN32S);
    OL ol(dst->getAtp(), dst->getAtpIndex(), (Int32)dst->getOffset(),
          (Int32)dst->getVoaOffset(), dst->getLength(), comboLen1,
          len, 0);

    PCI pci(PCIT::Op_FILL_MEM_BYTES, aml, ol);

    code.append(pci);
  }
  else
  {
    // For fixed fields, there could be padding before the data offset but 
    // not needed in the calculation here. 
    AML aml(PCIT::MASCII, PCIT::IBIN32S, PCIT::IBIN8U);
    OL ol(dst->getAtp(), dst->getAtpIndex(),dst->getOffset(),dst->getLength(),0);
    PCI pci(PCIT::Op_FILL_MEM_BYTES, aml, ol);
    code.append(pci);
  }

  nulBranch = generateJumpAndBranch(dst, code, notNullBranch, genUncondJump);


  return nulBranch;
}

PCIList PCode::isNull(Attributes *attrDst,
                      Attributes *attrSrc, 
                      CollHeap* heap)
{
  PCIList pciList(heap);

  // For aligned format the source nullness can be checked directly using
  // the null bit index for the source operand.
  // For exploded format or packed format the voa offset must be retrieved
  // to get to the start of the field and then the 2 bytes of nullness can
  // be checked there.
  Int32 flag = ((attrSrc->isIndirectVC() && !attrSrc->isSQLMXAlignedFormat())
                ? 1 : 0);

  AML aml(PCIT::MBIN32S, PCIT::MATTR5, PCIT::IBIN32S, PCIT::IBIN32S);
  OL ol(attrDst->getAtp(), attrDst->getAtpIndex(), (Int32)attrDst->getOffset(),
        attrSrc->getAtp(), attrSrc->getAtpIndex(),
        (Int32)attrSrc->getNullIndOffset(), attrSrc->getVoaOffset(),
        attrSrc->getTupleFormat(), attrSrc->getNullBitIndex(),
        flag, -1);
  PCI pci(PCIT::Op_NULL, aml, ol);
  pciList.append(pci);

  return pciList;
}

PCIList PCode::isNotNull(Attributes *attrDst,
                         Attributes *attrSrc, 
                         CollHeap   *heap)
{
  PCIList pciList(heap);


  // For aligned format the source nullness can be checked directly using
  // the null bit index for the source operand.
  // For exploded format or packed format the voa offset must be retrieved
  // to get to the start of the field and then the 2 bytes of nullness can
  // be checked there.
  Int32 flag = ((attrSrc->isIndirectVC() && !attrSrc->isSQLMXAlignedFormat())
                ? 1 : 0);

  AML aml(PCIT::MBIN32S, PCIT::MATTR5, PCIT::IBIN32S, PCIT::IBIN32S);
  OL ol(attrDst->getAtp(), attrDst->getAtpIndex(), (Int32)attrDst->getOffset(),
        attrSrc->getAtp(), attrSrc->getAtpIndex(),
        (Int32)attrSrc->getNullIndOffset(), attrSrc->getVoaOffset(),
        attrSrc->getTupleFormat(), attrSrc->getNullBitIndex(),
        flag, 0);
  PCI pci(PCIT::Op_NULL, aml, ol);
  pciList.append(pci);

  return pciList;
}

// Generate PCI's necessary before clause execution. Currently, this
// only covers TARGET PCI's that need to be generated because of
// branch clauses.
//
void PCode::preClausePCI(ex_clause *clause, PCIList& code) {

  // If this clause is a target of a branch clause, then insert a 
  // TARGET PCI for each branch clause that targets this clause. 
  // Use this clause's address as the target operand for now. This
  // will be fixed up in expr::pCodeGenerate() by matching BRANCH-TARGET
  // pairs which have the same operand.
  //
  if(clause->isBranchTarget()) {
    for(Int32 i=0; i<clause->getNumberBranchTargets(); i++)
    {
      AML aml;
      OL ol((Int64) clause);
      PCI pci(PCIT::Op_TARGET, aml, ol);
      code.append(pci);
    }
  }
}

// Generate PCI's necessary after clause execution.
//
void PCode::postClausePCI(ex_clause *clause, PCIList& code) {
}


PCIID PCode::nullBranchHelper(AttributesPtr *attrs,
			      Attributes *attrA, 
			      Attributes *attrB,
			      PCIList& code) {

  PCIID nulBranch = 0;
  Attributes *tgt = attrs[0];

  // If there are two nullable inputs...
  //
  if(attrB)
    {
      // If the output is nullable, add the NULL branch instructions,
      //
       
      if(tgt->getNullFlag())
      {
        PCodeTupleFormats tpf(tgt->getTupleFormat(), 
                              attrA->getTupleFormat(),
                              attrB->getTupleFormat());
        
        Int32 tgtNullIndOff = tgt->getNullIndOffset();
        Int32 attrANullIndOff = attrA->getNullIndOffset();
        Int32 attrBNullIndOff = attrB->getNullIndOffset();
       
        // For source operands:
        // For indirect varchars in packed format use the negated voa offset.
        // Then at runtime the code knows to get the offset value out of the
        // voa entry and jump to the actual value to test the nullness of the
        // field.
        // For aligned format the null indicator offset is used since the
        // null bitmap is statically known.
        // For exploded format the varchar field offset is known at compile
        // time and the attribute is not marked as 'indirect'.
        if (attrA->isIndirectVC() && !attrA->isSQLMXAlignedFormat()) 
          attrANullIndOff = -( (Int32)attrA->getVoaOffset() );
        if (attrB->isIndirectVC() && !attrB->isSQLMXAlignedFormat()) 
          attrBNullIndOff = -( (Int32)attrB->getVoaOffset() );

        // For target operand, a negative NullIndOffset indicates that 
        // it is an indirect varchar in packed format. In this case, the runtime
        // will use the loop variable varOffset. All other formats will
        // have valid NullIndOffset. 
          
        // If either of the input args are null, then output is null.
        AML aml(PCIT::MBIN32S,PCIT::MBIN32S,PCIT::MBIN32S,PCIT::IATTR4,PCIT::IBIN32S);
        OL ol(tgt->getAtp(), tgt->getAtpIndex(), tgtNullIndOff, 
              attrA->getAtp(), attrA->getAtpIndex(), attrANullIndOff, 
              attrB->getAtp(), attrB->getAtpIndex(), attrBNullIndOff, 
              EXPAND_PCODEATTRNULL3(*(UInt32 *)&tpf, tgt, attrA, attrB),
              0);
              
        PCI pci(PCIT::Op_NOT_NULL_BRANCH, aml, ol);
        code.append(pci);

        nulBranch = zeroFillNullValue(tgt, code, code.getTailId());  
      }
      //
      // Otherwise do a null violation check.
      //
      else 
	{
    	  // LCOV_EXCL_START
        if( attrA->isSQLMXAlignedFormat() || attrB->isSQLMXAlignedFormat() )
        {
          AML aml(PCIT::MPTR32,PCIT::MPTR32,PCIT::IPTR,PCIT::IPTR);
          OL ol(attrA->getAtp(), attrA->getAtpIndex(), 
                attrA->getNullIndOffset(),
                attrB->getAtp(), attrB->getAtpIndex(),
                attrB->getNullIndOffset(),
                (Int64)attrA, (Int64)attrB
                );
          PCI pci(PCIT::Op_NULL_VIOLATION, aml, ol);
          code.append(pci);
        }
        else 
        {
	  AML aml(PCIT::MBIN16U, PCIT::MBIN16U);
	  OL ol(attrA->getAtp(), attrA->getAtpIndex(), 
		attrA->getNullIndOffset(),
		attrB->getAtp(), attrB->getAtpIndex(),
		attrB->getNullIndOffset());
	  PCI pci(PCIT::Op_NULL_VIOLATION, aml, ol);
	  // LCOV_EXCL_STOP
	  code.append(pci);
	}
    }
    }
  //
  // Otherwise, if there is only one nullable input...
  //
  else if(attrA)
    {
      // If the output is nullable, add the NULL branch instruction
      //
      if(tgt->getNullFlag())
      {
        PCodeTupleFormats tpf(tgt->getTupleFormat(), 
                              attrA->getTupleFormat());
                              
        Int32 tgtNullIndOff = tgt->getNullIndOffset();
        Int32 attrANullIndOff = attrA->getNullIndOffset();
       
        // For indirect varchars in packed format use the negated voa offset.
        // Then at runtime the code knows to get the offset value out of the
        // voa entry and jump to the actual value to test the nullness of the
        // field.
        // For aligned format the null indicator offset is used since the
        // null bitmap is statically known.
        // For exploded format the varchar field offset is known at compile
        // time and the attribute is not marked as 'indirect'.
        if (attrA->isIndirectVC() && !attrA->isSQLMXAlignedFormat()) 
          attrANullIndOff = -( (Int32)attrA->getVoaOffset() );
          
        // If the input arg is null, then output is null.
        AML aml(PCIT::MBIN32S,PCIT::MBIN32S,PCIT::IATTR3,PCIT::IBIN32S);
        OL ol(tgt->getAtp(), tgt->getAtpIndex(), tgtNullIndOff, 
              attrA->getAtp(), attrA->getAtpIndex(), attrANullIndOff, 
              EXPAND_PCODEATTRNULL2(*(UInt32 *)&tpf, tgt, attrA),
              0);
              
        PCI pci(PCIT::Op_NOT_NULL_BRANCH, aml, ol);
        code.append(pci);
 
        nulBranch = zeroFillNullValue(tgt, code, code.getTailId());
      }
      //
      // otherwise do a null violation check.
      //
      else
	{
        if( attrA->isSQLMXAlignedFormat() )
        {
          AML aml(PCIT::MPTR32,PCIT::MPTR32,PCIT::IPTR,PCIT::IPTR);
          OL ol(attrA->getAtp(),attrA->getAtpIndex(),attrA->getNullIndOffset(),
                attrA->getAtp(),attrA->getAtpIndex(),attrA->getNullIndOffset(),
                (Int64)attrA,
                (Int64)0
                );
          PCI pci(PCIT::Op_NULL_VIOLATION, aml, ol);
          code.append(pci);
        }
        else 
        {
	  AML aml(PCIT::MBIN16U);
	  OL ol(attrA->getAtp(), attrA->getAtpIndex(), 
		attrA->getNullIndOffset());
	  PCI pci(PCIT::Op_NULL_VIOLATION, aml, ol);
	  code.append(pci);
        }
	}
    }

  return nulBranch;
}

PCIID PCode::nullBranchHelperForComp(AttributesPtr *attrs,
				     Attributes *attrA, 
				     Attributes *attrB,
				     NABoolean isSpecialNulls,
				     PCIList & code) {
  
  PCIID nulBranch = 0;
  Attributes *tgt = attrs[0];

  if (isSpecialNulls)
    {
      PCIID id;

      // Support the case where 1 or both operands are nullable
      // and this is an equi predicate.
      // This check was done when pcode was generated.

      //
      // If only one operand is nullable, then the result is 0 (because we're
      // only dealing with ITM_EQUAL right now).  Generate a null branch
      // check, and then fall-through to a move of 0 into the result.
      //
      if(!attrs[1]->getNullFlag() || !attrs[2]->getNullFlag())
      {
        //
        // Currently there is no NOT_NULL_BRANCH instruction that supports the
        // aligned row format which simply checks for NULL in the source operand
        // without setting a tgt/result.  Rather than create a new instruction
        // at this time
        // , use an existing NOT_NULL_BRANCH instruction that will be
        // slightly less efficient.
        //

        // First add NOT_NULL_BRANCH
        Attributes* src = (attrs[1]->getNullFlag() ? attrs[1] : attrs[2]);

        PCodeTupleFormats tpf(tgt->getTupleFormat(), src->getTupleFormat());
        Int32 srcNullIndOff = src->getNullIndOffset();

        // for indirect varchars, use the negated voaOffset.
        if (src->isIndirectVC() && !src->isSQLMXAlignedFormat())
          srcNullIndOff = -( (Int32)src->getVoaOffset() );

        // If the input arg is null, then output is null.
        AML aml(PCIT::MBIN32S, PCIT::MBIN32S, PCIT::IATTR3,PCIT::IBIN32S);
        OL ol(tgt->getAtp(), tgt->getAtpIndex(), tgt->getOffset(),
              src->getAtp(), src->getAtpIndex(), srcNullIndOff,
              EXPAND_PCODEATTRNULL2(*(UInt32 *)&tpf, tgt, src),
              0);

        PCI pci(PCIT::Op_NOT_NULL_BRANCH_COMP, aml, ol);
        code.append(pci);

        id = code.getTailId();

        // Now add store of 0 to result
        AML aml2(PCIT::MBIN32S, PCIT::IBIN32S);
        OL ol2(tgt->getAtp(), tgt->getAtpIndex(), tgt->getOffset(), 0);
        PCI pci2(PCIT::Op_MOVE, aml2, ol2);
        code.append(pci2);
      }
      else
      {
        // only support the case where both operands are nullable
        // and this is an equi predicate.
        // This check was done when pcode was generated.
        AML aml(PCIT::MBIN32S, PCIT::MATTR3, PCIT::MATTR3, 
                PCIT::IBIN32S, PCIT::IBIN32S);
        OL ol(tgt->getAtp(), tgt->getAtpIndex(), (Int32)tgt->getOffset(),
              attrA->getAtp(), attrA->getAtpIndex(), attrA->getNullIndOffset(),
              attrA->getNullBitIndex(),
              attrB->getAtp(), attrB->getAtpIndex(), attrB->getNullIndOffset(),
              attrB->getNullBitIndex(),
              ITM_EQUAL,
              0);
        PCI pci(PCIT::Op_NOT_NULL_BRANCH, aml, ol);
        code.append(pci);

        id = code.getTailId();
      }

      nulBranch = generateJumpAndBranch(tgt, code, id);
    }

  // If there are two nullable inputs...
  //
  else if (attrB)
    {
      assert(!tgt->isIndirectVC());

      PCodeTupleFormats tpf(tgt->getTupleFormat(), 
                            attrA->getTupleFormat(),
                            attrB->getTupleFormat());
                            
      Int32 attrANullIndOff = attrA->getNullIndOffset();
      Int32 attrBNullIndOff = attrB->getNullIndOffset();
       
      // For indirect varchars in packed format use the negated voa offset.
      // Then at runtime the code knows to get the offset value out of the
      // voa entry and jump to the actual value to test the nullness of the
      // field.
      // For aligned format the null indicator offset is used since the
      // null bitmap is statically known.
      // For exploded format the varchar field offset is known at compile
      // time and the attribute is not marked as 'indirect'.
      if (attrA->isIndirectVC() && !attrA->isSQLMXAlignedFormat()) 
        attrANullIndOff = -( (Int32)attrA->getVoaOffset() );
      if (attrB->isIndirectVC() && !attrB->isSQLMXAlignedFormat()) 
        attrBNullIndOff = -( (Int32)attrB->getVoaOffset() );
          
      // If either of the input args are null, then output is null.
      AML aml(PCIT::MBIN32S,PCIT::MBIN32S,PCIT::MBIN32S,PCIT::IATTR4,PCIT::IBIN32S);
      OL ol(tgt->getAtp(), tgt->getAtpIndex(), tgt->getOffset(),  
            attrA->getAtp(), attrA->getAtpIndex(), attrANullIndOff, 
            attrB->getAtp(), attrB->getAtpIndex(), attrBNullIndOff, 
            EXPAND_PCODEATTRNULL3(*(UInt32 *)&tpf, tgt, attrA, attrB),
            0);

      PCI pci(PCIT::Op_NOT_NULL_BRANCH_COMP, aml, ol);
      code.append(pci);
      nulBranch = generateJumpAndBranch(tgt, code, code.getTailId());
    }
  //
  // Otherwise, if there is only one nullable input...
  //
  else if (attrA)
    {
      PCodeTupleFormats tpf(tgt->getTupleFormat(), attrA->getTupleFormat());

      Int32 attrANullIndOff = attrA->getNullIndOffset();

      // For indirect varchars in packed format use the negated voa offset.
      // Then at runtime the code knows to get the offset value out of the
      // voa entry and jump to the actual value to test the nullness of the
      // field.
      // For aligned format the null indicator offset is used since the
      // null bitmap is statically known.
      // For exploded format the varchar field offset is known at compile
      // time and the attribute is not marked as 'indirect'.
      if (attrA->isIndirectVC() && !attrA->isSQLMXAlignedFormat()) 
        attrANullIndOff = -( (Int32)attrA->getVoaOffset() );

      // If the input is null, then output is null.
      AML aml(PCIT::MBIN32S,PCIT::MBIN32S,PCIT::IATTR3,PCIT::IBIN32S);
      OL ol(tgt->getAtp(), tgt->getAtpIndex(), tgt->getOffset(),  
            attrA->getAtp(), attrA->getAtpIndex(), attrANullIndOff,  
            EXPAND_PCODEATTRNULL2(*(UInt32 *)&tpf, tgt, attrA),
            0);

      PCI pci(PCIT::Op_NOT_NULL_BRANCH_COMP, aml, ol);
      code.append(pci);
      nulBranch = generateJumpAndBranch(attrs[0], code, code.getTailId());
    }

  return nulBranch;
}

PCIID PCode::nullBranchHelperForHash(AttributesPtr *attrs,
				     Attributes *attrA, 
				     Attributes *attrB,
				     PCIList & code) {
  
  PCIID nulBranch = 0;

  Attributes *tgt = attrs[0];

  // This will handle all formats of source data ...
  AML aml(PCIT::MBIN32S, PCIT::MATTR3, PCIT::IBIN32S, PCIT::IBIN32S);
  OL ol(tgt->getAtp(), tgt->getAtpIndex(), tgt->getOffset(),
	attrA->getAtp(), attrA->getAtpIndex(), attrA->getNullIndOffset(),
        attrA->getNullBitIndex(),
        ex_clause::NULL_HASH, 0);

  PCI pci(PCIT::Op_NOT_NULL_BRANCH, aml, ol);
  code.append(pci);
  nulBranch = generateJumpAndBranch(tgt, code, code.getTailId());

  return nulBranch;
}

PCIID PCode::nullBranch(ex_clause *clause,
			PCIList& code,
			AttributesPtr *attrs)
{
  // Rowsets cannot be null.
  if ((attrs[0]->getRowsetSize() > 0) || (attrs[1]->getRowsetSize() > 0)) {
    return 0;
  }

  // If NULL is not relevant for this operation, there is nothing to do here.
  // Skip this check for hash operation because though the nullRelevant is 
  // turned off for eval_clause reasons, it is needed here to store a constant
  // hash value for nulls.
  // In hash's eval_clause, hash function's destination is not nullable
  // so turning on nullRelevant flag causes eval_clause to write to
  // opdata[0] which is not setup since it was skipped in the first place. 
  if(!clause->isNullRelevant()) 
  {
    if ((clause->getType() == ex_clause::FUNCTION_TYPE) &&
        ((((ex_function_clause*)clause)->origFunctionOperType() == ITM_HASH) ||
        (((ex_function_clause*)clause)->origFunctionOperType() == ITM_HDPHASH)))
    {
      // null is relevant for hash if we are generating pcode.
    } 
    else 
    {
      return 0;
    }
  }

  // If none of the operands are nullable, there is nothing to do here.
  //
  if(!clause->isAnyOperandNullable()) 
    return 0;

  // Find the nullable inputs
  //
  PCIID nulBranch = 0;
  Attributes *attrA = NULL, *attrB = NULL;
  if(clause->isAnyInputNullable())
    {
      for(Int32 i=1; i<clause->getNumOperands(); i++) {
	if(attrs[i]->getNullFlag()) {
	  if(!attrA) {
	    attrA = attrs[i];
	  } else if(!attrB)
	    attrB = attrs[i];
	  else {
	    // Null Branch: Too many nullable operands
            assert(!attrA || !attrB);
	  }
	}
      }
    }
  if(attrA) 
    {
      if ((clause->getType() == ex_clause::COMP_TYPE) ||
          (clause->getType() == ex_clause::LIKE_TYPE))
	nulBranch = nullBranchHelperForComp(attrs, attrA, attrB, 
					    (clause->isSpecialNulls() != 0),
					    code);
      else if ((clause->getType() == ex_clause::FUNCTION_TYPE) &&
        ((((ex_function_clause*)clause)->origFunctionOperType() == ITM_HASH) ||
        (((ex_function_clause*)clause)->origFunctionOperType() == ITM_HDPHASH)))
      {
	nulBranch = nullBranchHelperForHash(attrs, attrA, attrB, code);
      }
      else
	nulBranch = nullBranchHelper(attrs, attrA, attrB, code);
    }
  //
  // Otherwise, if there are no nullable inputs, but the output is nullable, 
  // set the output to not null.
  //
  Attributes *tgt = attrs[0];
  if(!attrA && clause->isAnyOutputNullable())
  {
    // Check if we need to handle the null bitmap for the aligned row format.
    if ( tgt->isSQLMXAlignedFormat() )
    {
      AML aml( PCIT::MPTR32,          // ptr to target null bitmap to clear
               PCIT::IBIN32S,         // flag - set or test
               PCIT::IBIN16U,         // null bit index
               PCIT::IBIN32S          // value to set to (on/off)
               );
      OL ol(tgt->getAtp(), tgt->getAtpIndex(), tgt->getNullIndOffset(),
            PCIT::NULL_BITMAP_SET, (Int16)tgt->getNullBitIndex(), 0);
      PCI pci(PCIT::Op_NULL_BITMAP, aml, ol);
      code.append(pci);
    }
    else if ( tgt->isSQLMXFormat() ) 
    {
      AML aml( PCIT::MPTR32,          // ptr to target null bitmap to clear
               PCIT::IBIN32S          // value to set to (on/off)
               );
      OL ol(tgt->getAtp(), tgt->getAtpIndex(), tgt->getNullIndOffset(), 0);
      PCI pci(PCIT::Op_NULL_BYTES, aml, ol);
      code.append(pci);
    }
    else // exploded format
    {
      AML aml(PCIT::MBIN16U);
      OL ol(tgt->getAtp(), tgt->getAtpIndex(), tgt->getNullIndOffset());
      PCI pci(PCIT::Op_NULL, aml, ol);
      code.append(pci);
    }
  }

  return nulBranch;
}
