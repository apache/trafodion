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
 *****************************************************************************
 *
 * File:         <file>
 * Description:  
 *               
 *               
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"


#include <stdio.h>
#include <stdlib.h>

#include "exp_stdh.h"
#include "exp_expr.h"
#include "ExpPCode.h"
#include "ExpPCodeOptimizations.h"

void setVCLength(char * VCLen, Lng32 VCLenSize, ULng32 value);


// -----------------------------------------------------------------------
// This method returns the virtual function table pointer for an object
// with the given class ID; used by NAVersionedObject::driveUnpack().
// -----------------------------------------------------------------------
char *ex_expr::findVTblPtr(short classID)
{
  char *vtblPtr;
  switch (classID)
  {
    case exp_INPUT_OUTPUT:
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblPtr, InputOutputExpr);
#pragma warn(1506)  // warning elimination 
      break;
    case exp_AGGR:
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblPtr, AggrExpr);
#pragma warn(1506)  // warning elimination 
      break;
    case exp_LEAN_EXPR:
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblPtr, ex_expr_lean);
#pragma warn(1506)  // warning elimination 
      break;
    default:
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblPtr, ex_expr);
#pragma warn(1506)  // warning elimination 
      break;
  }
  return vtblPtr;
}

Lng32 ex_expr::initExpr()
{
  Lng32 rc = 0;

  ex_clause *currClause = clauses_;

  while( currClause != NULL )
    {
      rc = currClause->initClause();
      if (rc)
	return rc;

      currClause = currClause->getNextClause();
    }

  return rc;
}

// LCOV_EXCL_START
// lean expressions are not used not tested
void ex_expr_lean::displayContents(Space * space, short mode,
				   const char * displayStr,
				   ULng32 flag)
{
  char buf[200];

  str_sprintf(buf, "Expression: %s", displayStr);
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
  str_sprintf(buf, "  Total Expr Len: %d, Consts Area Len: %d", getLength(), getConstsLength());
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
  str_sprintf(buf, "  Temps Area Len: %d, Persistent Area Len: %d", getTempsLength(), persistentLength());
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
  str_sprintf(buf, "  PCode Expr Length: %d\n", getPCodeSize());
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
  
  if (flag & 0x00000002)
    {
      // Clear the fixup flag, since the expression has not been fixed up
      // (pCodeGenerate undoes, the fixup of Const and Temp offsets.)
      //
      setFixupConstsAndTemps(0);

      if (getPCodeBinary())
	{
	  str_sprintf(buf, "  PCode:\n");
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	  //	  PCode::displayContents(getPCodeObject()->getPCIList(), space);

	  PCodeBinary *pCode = getPCodeBinary();

	  // skip over the ATP's
	  Int32 length = *(pCode++);
	  pCode += (2 * length);
  
	  while (pCode[0] != PCIT::END)
	    {
	      Int32 opcode = *(pCode++);

	      str_sprintf(buf, "    %s ", PCIT::instructionString(PCIT::Instruction(opcode)));

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
    }
}
// LCOV_EXCL_STOP
// make a 'lean' copy of 'this' and point it back to itself.  Supports all
// expressions.  Do not copy clauses_ or pCodeObject_.  Caller is responsible
// for proper deallocation of memory.
void ex_expr::makeLeanCopyAll(Space * space)
{
  if (getPCodeSegment() != NULL) {
    Int32 pCodeSize = getPCodeSize();
    NABoolean evalFlag = pCode_->containsClauseEval();
    PCodeBinary* pCode = getPCodeBinary();
    PCodeBinary* code;

    pCode_ = new(space) PCodeSegment();
    code = (PCodeBinary*) (new(space) char[pCodeSize]);

    str_cpy_all((char*)code, (char*)pCode, pCodeSize);

    pCode_.getPointer()->setPCodeSegmentSize(pCodeSize);
    pCode_.getPointer()->setContainsClauseEval(evalFlag);

    setPCodeBinary(code);
  }

  if (constantsArea_) {
    char* area = space->allocateAlignedSpace(constantsAreaLength_);
    str_cpy_all(area, constantsArea_, constantsAreaLength_);
    setConstantsArea(area);
  } 

  if (tempsArea_) {
    char* area = space->allocateAlignedSpace(tempsAreaLength_);
    setTempsArea(area);
  }

  if (persistentArea_) {
    char* area = space->allocateAlignedSpace(persistentLength_);
    persistentArea_ = area;
  }

  if (persistentInitializationArea_) {
    char* area = space->allocateAlignedSpace(persistentLength_);
    str_cpy_all(area, persistentInitializationArea_, persistentLength_);
    persistentInitializationArea_ = area;
  }
}

// make a 'lean' copy of 'this' to 'other'.
// Do not copy clauses_ or pCodeObject_.
// LCOV_EXCL_START
void ex_expr::makeLeanCopy(ex_expr_lean * other, Space * space)
{
  //  *((ex_expr_base*)other) = *((ex_expr_base*)this);

  PCodeBinary * pCodeS = (PCodeBinary*) 
    space->allocateAlignedSpace(sizeof(PCodeBinary) + 
				pCode_->getPCodeSegmentSize());
  str_cpy_all((char*)&pCodeS[1], (char*)pCode_->getPCodeBinary(),
	      pCode_->getPCodeSegmentSize());
  pCodeS[0] = pCode_->getPCodeSegmentSize();
  other->setPCodeLean(pCodeS);
  //  other->setPCodeSize(pCode_->getPCodeSegmentSize());

  other->constantsAreaLength_ = constantsAreaLength_;
  other->tempsAreaLength_ = tempsAreaLength_;
  other->persistentLength_ = persistentLength_;

  if (constantsArea_)
    {
      other->setConstantsArea(space->allocateAlignedSpace(constantsAreaLength_));
      str_cpy_all(other->getConstantsArea(), constantsArea_,
		  constantsAreaLength_);
    }
 
  if (tempsArea_)
    {
      other->setTempsArea(space->allocateAlignedSpace(tempsAreaLength_));
    }
  
  if (persistentArea_)
    {
      other->persistentArea_ = space->allocateAlignedSpace(persistentLength_);
    }
}
// LCOV_EXCL_STOP
void ex_expr::displayContents(Space * space, short mode, const char * displayStr,ULng32 flag)
{
  char buf[100] = {0};	// Soln 10-041117-1848

  ULng32 exprLen = getLength();

  str_sprintf(buf, "Expression: %s", displayStr);
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
  str_sprintf(buf, "Expr Len: %d, Consts Len: %d", exprLen, getConstsLength());
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
  str_sprintf(buf, "flags_ = %x ",flags_ );
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  if (flag & 0x00000004)
    {
      Int32 i = 1;
      ex_clause * clause = getClauses();
      while (clause)
	{
	  clause->displayContents(space, 0, i, constantsArea_);
	  
	  clause = clause->getNextClause();
	  i++;
	}
    }
  
  if (flag & 0x00000002)
    {
      if (getPCodeBinary())
        {
          str_sprintf(buf, "  PCode:\n");
          space->allocateAndCopyToAlignedSpace(buf, str_len(buf),sizeof(short));
          str_sprintf(buf, "PCode Expr Length: %d", getPCodeSize());
          space->allocateAndCopyToAlignedSpace(buf, str_len(buf),sizeof(short));

          PCodeCfg* cfg = new(heap_) PCodeCfg(this, heap_);
          cfg->generateShowPlan(getPCodeBinary(), space);
          NADELETE(cfg, PCodeCfg, heap_);
        }
      else if ((flag & 0x00000010) &&
	       (getPCodeBinary()))
	{
	  str_sprintf(buf, "  PCode:\n");
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	  str_sprintf(buf, "PCode Expr Length: %d", getPCodeSize());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	  
	  PCode::displayContents(getPCodeBinary(), space);
	}
      else
	{
	  // Clear the fixup flag, since the expression has not been fixed up
	  // (pCodeGenerate undoes, the fixup of Const and Temp offsets.)
	  //
	  setFixupConstsAndTemps(0);
	  
	  // re-generate pcode for showplan
	  setPCodeGenCompile(0);
	  setPCodeObject(0);
	  pCode_ = 0;
	  Space tempSpace;  //create pcode stuff in a different space
	  //the space in PCode::displayContents is used for
	  //creating text for the showplan
	  UInt32 f = 0;
	  ex_expr_base::setForShowplan(f, TRUE);
	  pCodeGenerate(&tempSpace, &tempSpace, f);
	  
	  if(getPCodeObject())
	    {
		  // LCOV_EXCL_START
	      str_sprintf(buf, "  PCode:\n");
	      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	      str_sprintf(buf, "PCode Expr Length: %d", getPCodeSize());
	      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	      
	      PCode::displayContents(getPCodeObject()->getPCIList(), space);
	      // LCOV_EXCL_STOP
	    }
	}
    }
}

char *ex_expr::getPersistentData(Int32 offset)
{
  return persistentArea_ + offset;
}

// error denoted by negative return code
int ex_expr::formatARow2(const char** srcFldsPtr,
                         const int* srcFldsLength,
                         const int * srcFieldsConvIndex,
                         char* formattedRow,
                         int& formattedRowLength,
                         int numAttrs,
                         AttributesPtr * attrs,
                         ExpTupleDesc *tupleDesc,
                         UInt16 firstFixedOffset,
                         UInt16 bitmapEntryOffset,
                         UInt16 bitmapOffset,
                         NABoolean sysKeyTable,
                         CollHeap *heap,
                         ComDiagsArea **diags)
{

  formattedRowLength = tupleDesc->tupleDataLength();  //need to fix this later for other types
  //char *sourceData = (char*)asciiRow;
  //char *sourceDataEnd =(char*)asciiRow + rowLength;
  //assert (sourceDataEnd);


  //set the headers in tuple.
  ExpTupleDesc::setFirstFixedOffset(formattedRow,
                                firstFixedOffset,
                                tupleDesc->getTupleDataFormat());

  //Bitmap offset value is zero if bitmap is not preset. But bitmap offset
  //still exists.
  ExpAlignedFormat::setBitmapOffset(formattedRow + bitmapEntryOffset,
                                    - bitmapOffset);


  char *targetData = NULL;

  //for varchars
  UInt32  vcActualLen = 0;
  UInt32  voaOffset = 0;
  UInt32  vOffset = 0;
  Int32   vcLenIndOffset = 0;
  bool    firstvc = true;
  char paddedTimestampVal[27] = "2000-01-01 00:00:00.000000" ;
  

  int startCol = 0;
  if (sysKeyTable)
  {
    startCol =1;
  }
  for(int index= startCol; index < numAttrs ; index++)
  {
    //Attributes * attr = myTdb().getOutputAttr(index);
    Attributes * attr = attrs[index];
    //loop thru all the columns in the row. For each column,
    //find its corresponding offset in ascii row. Corresponding
    //offset is identified and converted.

    char * srcColData = (char*)srcFldsPtr[index - startCol];
    int  srcColLen = srcFldsLength[index - startCol];

    //set null bit map if column is nullable.
    //if ( attr->getDefaultClass() == Attributes::DEFAULT_NULL )
      // ExpAlignedFormat::setNullValue( formattedRow, attr->getNullBitIndex() );

    if (! srcColLen )
    {
        ExpAlignedFormat::setNullValue( formattedRow, attr->getNullBitIndex() );
        continue;
    }

    if((!firstvc)  && attr->getVCIndicatorLength() > 0)
      targetData = (char*) formattedRow + vOffset;
    else
      targetData = (char*) formattedRow + attr->getOffset();

    if (attr->getDatatype() == REC_DATETIME && srcColLen == 10)
      {
        srcColData = &(paddedTimestampVal[0]);
        srcColLen = 26;
      }

    ex_expr::exp_return_type err = convDoIt(srcColData,
                                            srcColLen,
                                            REC_BYTE_F_ASCII,
                                            0,0,
                                            targetData,
                                            attr->getLength(),
                                            attr->getDatatype(),
                                            attr->getPrecision(),
                                            attr->getScale(),
                                            (char*)&vcActualLen,
                                            sizeof(vcActualLen),
                                            heap,
                                            diags,
                                            (ConvInstruction)srcFieldsConvIndex[index]
                                            );

    if(err == ex_expr::EXPR_ERROR) return -1;

    //update var fields parameters and adjust for subsequent var columns.
    if(attr->getVCIndicatorLength() > 0)
    {
      if(firstvc)
      {
          voaOffset = attr->getVoaOffset();
          vOffset = attr->getOffset();
          vcLenIndOffset = attr->getVCLenIndOffset();
      }
      //obtain actual len of varchar field and update VCIndicatorLength
      setVCLength(formattedRow + vcLenIndOffset,
                              attr->getVCIndicatorLength(), vcActualLen);

      //update voa offset location with value of offset
      ExpTupleDesc::setVoaValue(formattedRow, voaOffset,
                                vcLenIndOffset, tupleDesc->getTupleDataFormat());

      //update all offset vars for next varchar column
      ExpAlignedFormat::incrVoaOffset(voaOffset);
      vcLenIndOffset += (vcActualLen + attr->getVCIndicatorLength());
      vOffset = vcLenIndOffset + attr->getVCIndicatorLength(); //vcIndLen remains same.
      firstvc = false;

      //update formatted row length to reflect actual len
      formattedRowLength = vcLenIndOffset;
    }

  }

  //finally adjust the data length to alignment size. See header file for more info.
  formattedRowLength = ExpAlignedFormat::adjustDataLength(formattedRow,
                                                          formattedRowLength,
                                                          ExpAlignedFormat::ALIGNMENT,
                                                          TRUE);
  return 0;
};

void AggrExpr::displayContents(Space * space, short mode, const char * displayStr,
                               ULng32 flag)
{
  char buf[100];

  if (initExpr_)
    {
      str_sprintf(buf, "%s::initExpr_", displayStr);

      initExpr_->displayContents(space, mode, buf, flag);
    }

  if (finalNullExpr_)
    {
      str_sprintf(buf, "%s::finalNullExpr_", displayStr);

      finalNullExpr_->displayContents(space, mode, buf, flag);
    }

  if (finalExpr_)
    {
      str_sprintf(buf, "%s::finalExpr_", displayStr);

      finalExpr_->displayContents(space, mode, buf, flag);
    }

  if (groupingExpr_)
    {
      str_sprintf(buf, "%s::groupingExpr_", displayStr);
      
      groupingExpr_->displayContents(space, mode, buf, flag);
    }

  if (perrecExpr_)
    {
      str_sprintf(buf, "%s::perrecExpr_", displayStr);

      perrecExpr_->displayContents(space, mode, buf, flag);
    }

}


