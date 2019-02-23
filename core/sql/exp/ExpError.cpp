/* -*-C++-*-
****************************************************************************
*
* File:         ExpError.cpp (previously part of /executor/ex_error.cpp)
* Description:
*
* Created:      5/6/98
* Language:     C++
*
*
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
*
****************************************************************************
*/

#include "Platform.h"


#include "ExpError.h"
#include  "str.h"
#include "ComDiags.h"
#include "exp_clause_derived.h"
#include "exp_datetime.h"

// Single allocation of buf is split up to be used for opstrings,
// formatting.
// |-----buf------------------------------------|---opstrings[]---------------|--FORMATTING--|
// |--(numAModes * VALUE_TYPE_LEN) + INSTR_LEN--|--(numAModes * OPVALUE_LEN)--|--FORMATTING--]
// |--(numAModes * VALUE_TYPE_LEN) + INSTR_LEN--+--(numAModes * OPVALUE_LEN)--+--FORMATTING--]

// above equation deduced to:
// ==[(numAModes * (VALUE_TYPE_LEN + OPVALUE_LEN)) + INSTR_LEN + FORMATTING]
#define OPVALUE_LEN  200   // operand value length
#define OPTYPE_LEN   100  // operand type length
#define VALUE_TYPE_LEN  OPTYPE_LEN + OPVALUE_LEN
#define INSTR_LEN   200   // Instruction,operation,formatting length
#define FORMATTING  200   // For formatting detailed error string


class PCIType;
extern char * exClauseGetText(OperatorTypeEnum ote);
extern char * getDatatypeAsString( Int32 Datatype, NABoolean extFormat );
extern short
convertTypeToText_basic(char * text,
                        Lng32 fs_datatype,
                        Lng32 length,
                        Lng32 precision,
                        Lng32 scale,
                        rec_datetime_field datetimestart,
                        rec_datetime_field datetimeend,
                        short datetimefractprec,
                        short intervalleadingprec,
                        short upshift,
			short caseinsensitive,
                        CharInfo::CharSet charSet,
                        const char * collation_name,
                        const char * displaydatatype,
			short displayCaseSpecific);

ComDiagsArea *ExAddCondition(CollHeap* heap, ComDiagsArea** diagsArea,
			     Lng32 err, ComCondition** newCond,
			     Lng32 * intParam1,
			     Lng32 * intParam2,
			     Lng32 * intParam3,
			     const char * stringParam1,
			     const char * stringParam2,
			     const char * stringParam3)
{
  //
  // This version of ExRaiseSqlError is used by the expressions code.  In
  // addition to having slightly different parameters, it differs from the
  // above version in that it puts an error condition in the supplied
  // ComDiagsArea rather than in a copy.
  //
  // If the caller didn't pass in the address of a pointer to the
  // ComDiagsArea, there's no point in creating an error condition since the
  // caller won't receive it.  Normally this should never happen, but right
  // now it may occur until all the error processing code is in place.
  //
  if (diagsArea == NULL)
    return NULL;
  //
  // The caller did pass in the address of a pointer to the ComDiagsArea.  If
  // the pointer is NULL, we need to allocate the ComDiagsArea and put its
  // address in the pointer.  Otherwise, we will put the error condition in
  // the ComDiagsArea that was supplied.
  //
  if (*diagsArea == NULL) {
    //
    // If the heap is NULL, we can't allocate the ComDiagsArea.  This should
    // never happen.
    //
    if (heap == NULL)
      return NULL;
    *diagsArea = ComDiagsArea::allocate(heap);
  }

  ComDiagsArea *da = *diagsArea;
  ComCondition *cond = da->makeNewCondition();
  cond->setSQLCODE(err);

  if (intParam1)
    cond->setOptionalInteger(0, *intParam1);
  if (intParam2)
    cond->setOptionalInteger(1, *intParam2);
  if (intParam3)
    cond->setOptionalInteger(2, *intParam3);
  if (stringParam1)
    cond->setOptionalString(0, stringParam1);
  if (stringParam2)
    cond->setOptionalString(1, stringParam2);
  if (stringParam3)
    cond->setOptionalString(2, stringParam3);

  da->acceptNewCondition();

  if (newCond) 
    *newCond = cond;

  return *diagsArea;
}

ComDiagsArea *ExRaiseSqlError(CollHeap* heap, ComDiagsArea** diagsArea,
                              ExeErrorCode err, ComCondition** cond,
			      Lng32 * intParam1,
			      Lng32 * intParam2,
			      Lng32 * intParam3,
			      const char * stringParam1,
			      const char * stringParam2,
			      const char * stringParam3)
{
  return ExAddCondition(heap, diagsArea, - (Lng32) err, cond,
			intParam1, intParam2, intParam3, 
			stringParam1, stringParam2, stringParam3);
}
 
ComDiagsArea *ExRaiseSqlError(CollHeap* heap, ComDiagsArea** diagsArea,
              Lng32 err,
              Lng32 * intParam1,
              Lng32 * intParam2,
              Lng32 * intParam3,
              const char * stringParam1,
              const char * stringParam2,
              const char * stringParam3)
{
  return ExAddCondition(heap, diagsArea, err, NULL,
             intParam1, intParam2, intParam3, 
             stringParam1, stringParam2, stringParam3);
}

ComDiagsArea *ExRaiseSqlWarning(CollHeap* heap, ComDiagsArea** diagsArea,
                              ExeErrorCode err, ComCondition** cond,
                              Lng32 * intParam1,
                              Lng32 * intParam2,
                              Lng32 * intParam3,
                              const char * stringParam1,
                              const char * stringParam2,
                              const char * stringParam3)
{
  return ExAddCondition(heap, diagsArea,  (Lng32) err, cond,
                        intParam1, intParam2, intParam3,
                        stringParam1, stringParam2, stringParam3);
}
ComDiagsArea *ExRaiseSqlWarning(CollHeap* heap, ComDiagsArea** diagsArea,
                                ExeErrorCode err, ComCondition** cond)
{
  return ExAddCondition(heap, diagsArea, (Lng32) err, cond, NULL, NULL, NULL);
}

ComDiagsArea *ExRaiseFunctionSqlError(CollHeap* heap, 
				      ComDiagsArea** diagsArea,
				      ExeErrorCode err, 
				      NABoolean derivedFunction,
				      OperatorTypeEnum origOperType,
				      ComCondition** cond)
{
  ExRaiseSqlError(heap, diagsArea, err);

  if (derivedFunction)
    {
      **diagsArea << DgSqlCode(-EXE_MAPPED_FUNCTION_ERROR);
      **diagsArea << DgString0(exClauseGetText(origOperType));
    }
  return *diagsArea;
}

Int32 convertToHexAscii(char *src, Int32 srcLength, char *result,
                      Int32 maxResultSize)
{
  const char HexArray[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                             'A', 'B', 'C', 'D', 'E', 'F'};
  
  if( src == NULL || result == NULL || srcLength<=0  || maxResultSize <= 0) 
    return 1;
  //Each byte = 2 chars to represent hex + '0x'+'\0'.
  if( ((srcLength * 2) + 3) > maxResultSize)
    return 1;
  
  char *srcTemp = src;
  Int32 upper4bits, lower4bits;
  result[0] = '0';
  result[1] = 'x';
  char *resultTemp = &result[2];

  //Since source length may be a odd value, it is not possible to
  //convert between little or big endian. We just convert the 
  //memory into hex and put it in the string. 
  for( Int32 i = 0; i < srcLength; i++ )
  {
    lower4bits = (*srcTemp) & 0x0F;
    upper4bits = (*srcTemp) & 0xF0;
    upper4bits>>= 4;
    resultTemp[2 * i] = HexArray[upper4bits];
    resultTemp[(2 * i) + 1 ] = HexArray[lower4bits];
    srcTemp++;
  }
  result[(srcLength * 2)+2] = '\0';
  return 0;
}


void ExConvertErrorToString(CollHeap* heap, 
		            ComDiagsArea** diagsArea,
                            char *src,
                            Int32 srcLength,
                            Int16 srcType,
                            Int32 srcScale,
                            char *result,
                            Int32 maxResultSize
                            )
{
  ex_expr::exp_return_type retCode = ex_expr::EXPR_OK;

  Int32 vcharLen = 0;
  Int32 counter;
  Int32 errorMark    = ComDiagsArea::INVALID_MARK_VALUE;
  Int32 warningMark  = ComDiagsArea::INVALID_MARK_VALUE;
  Int32 errorMark1; 
  Int32 warningMark1;
    
  if (DFS2REC::isBinaryString(srcType)) {
    if(convertToHexAscii(src, srcLength, result, maxResultSize) == 0 ){
      return;
    }

    result[0] = '?';
    result[1] = '\0';
    
    return;
  }

  if(*diagsArea){
    errorMark = (*diagsArea)->getNumber(DgSqlCode::ERROR_);
    warningMark = (*diagsArea)->getNumber(DgSqlCode::WARNING_);
  }
           
  retCode = convDoIt( src,
                      srcLength,             //source length
                      srcType,               //source type
                      0,                     //source precision
                      srcScale,              //source scale
                      result,                //target
                      maxResultSize,         //targetlength
                      REC_BYTE_V_ASCII,      //target type
                      0,                     //target precision
                      SQLCHARSETCODE_UTF8,   //target scale
                      (char*)&vcharLen,      //vcharlength 
                      sizeof(vcharLen),      //vchar length size
                      heap,
                      diagsArea,
                      CONV_UNKNOWN,
                      0,
                      CONV_CONTROL_LOOPING | CONV_ALLOW_INVALID_CODE_VALUE);
    
  //Before proceeding, delete any errors and warnings that may have
  //been introduced by convDoIt from above.
  if(*diagsArea){
    errorMark1 = (*diagsArea)->getNumber(DgSqlCode::ERROR_);
    warningMark1 = (*diagsArea)->getNumber(DgSqlCode::WARNING_);
    counter = errorMark1 - errorMark;
    while(counter){
      (*diagsArea)->deleteError(errorMark1 - counter);
      counter--;
    }
    counter = warningMark1 - warningMark;
    while(counter){
      (*diagsArea)->deleteWarning(warningMark1 - counter);
      counter--;
    }
  }

  if (retCode == ex_expr::EXPR_OK ){
    // need to zero terminate the result buffer to printf error string
    if (vcharLen < maxResultSize - 1)
      result[vcharLen] = '\0';
    else
      result[maxResultSize - 1] = '\0';
    return;
  }

  //Once we reach this point, all we can do is just dump the source memory.
  if(convertToHexAscii(src, srcLength, result, maxResultSize) == 0 ){
    return;
  }

  // Once we reach this point, there is nothing we can do much.
  // Attempt to display "-E" indicating error, else NULL.
  if(maxResultSize >=3){
    result[0] = '-';
    result[1] = 'E';
    result[2] = '\0';
  }
  else
    result[0] = '\0';
  return;
}

//Detailed error support for pcode expression evaluation.
ComDiagsArea *ExRaiseDetailSqlError(CollHeap* heap, 
				    ComDiagsArea** diagsArea,
				    ExeErrorCode err, 
				    Int32 pciInst,
                                    char *op1,
                                    char *op2,
                                    char *op3)
{
  if (diagsArea == NULL)
    return NULL;
  
  if (*diagsArea == NULL){
  
    if (heap == NULL)
      return NULL;
    *diagsArea = ComDiagsArea::allocate(heap);
  }
  
  PCIT::Operation operation;
  PCIT::AddressingMode am[6];
  Int32 numAModes;
  Int32 rangeInst = PCode::isInstructionRangeType((PCIT::Instruction)pciInst)? 1: 0;

  if(PCode::getOpCodeMapElements( pciInst, operation, am, numAModes ) ){
    
    ExRaiseSqlError(heap, diagsArea, err);
    return *diagsArea;
  }

  // Single allocation of buf is split up to be used for opstrings,
  // formatting. See defines at beginning of this header file 
  // for details.
  char *buf = new (heap)
    char[(numAModes * (VALUE_TYPE_LEN + OPVALUE_LEN)) + INSTR_LEN + FORMATTING];
  
  if( !buf ){
    ExRaiseSqlError(heap, diagsArea, err);
    return *diagsArea;
  }
  char *opStrings = &buf[(numAModes * VALUE_TYPE_LEN) + INSTR_LEN ];
  char *buf1 = // assign FORMATTING part of address.
      &buf[(numAModes * (VALUE_TYPE_LEN + OPVALUE_LEN)) + INSTR_LEN];

  switch( numAModes ){
    case 1:
      ExConvertErrorToString(heap,
                             diagsArea,
                             op1,
                             PCIT::getOperandLengthForAddressingMode(am[0]),
                             PCIT::getDataTypeForMemoryAddressingMode(am[0]),
                             0,
                             (char*)&opStrings[0],
                             OPVALUE_LEN);
    
     //construct a formated string and assign to diags area
      **diagsArea << DgSqlCode(-err);
           
      if(rangeInst){
        str_sprintf( buf, " Source Value:%s not within limits of Target Type:%s(%s).",
                    &opStrings[0],
                    getDatatypeAsString(PCIT::getDataTypeForMemoryAddressingMode(am[0]),true),
                    PCIT::addressingModeString(am[0]));
      }
      else{
        str_sprintf( buf, " Operand Type:%s(%s)  Operand Value:%s.",
                   getDatatypeAsString(PCIT::getDataTypeForMemoryAddressingMode(am[0]),true),
                   PCIT::addressingModeString(am[0]), &opStrings[0]);
      }
      
      str_sprintf( buf1, " Instruction:%s Operation:%s.",
                   PCIT::instructionString((PCIT::Instruction)pciInst),
                   PCIT::operationString(operation));
      str_cat(buf, buf1, buf);
      **diagsArea<<DgString0(buf);
          
      break;

    case 3:
    case 4:
      //since we are only interested in left and right operands, just overright 
      //am modes to be processed by case 2 below.
      am[0] = am[1];
      am[1] = am[2];

      //do not break here. Flow down to case 2.
    case 2:
      ExConvertErrorToString(heap,
                             diagsArea,
                             op1,
                             PCIT::getOperandLengthForAddressingMode(am[0]),
                             PCIT::getDataTypeForMemoryAddressingMode(am[0]),
                             0,
                             (char*)&opStrings[0],
                             OPVALUE_LEN);

      ExConvertErrorToString(heap,
                             diagsArea,
                             op2,
                             PCIT::getOperandLengthForAddressingMode(am[1]),
                             PCIT::getDataTypeForMemoryAddressingMode(am[1]),
                             0,
                             (char*)&opStrings[OPVALUE_LEN],
                             OPVALUE_LEN);
      
      //construct a formated string and assign to diags area
      **diagsArea << DgSqlCode(-err);
      str_sprintf( buf, " %s Type:%s(%s) %s Value:%s",
                   rangeInst? "Source":"Operand1",
                   getDatatypeAsString(PCIT::getDataTypeForMemoryAddressingMode(am[0]),true),
                   PCIT::addressingModeString(am[0]),
                   rangeInst? "Source":"Operand1", &opStrings[0]);
      str_sprintf( buf1, " %s Type:%s(%s)%s%s Value:%s.",
                   rangeInst? "Target":"Operand2",
                   getDatatypeAsString(PCIT::getDataTypeForMemoryAddressingMode(am[1]),true),
                   PCIT::addressingModeString(am[1]),
                   rangeInst? (opStrings[OPVALUE_LEN] == '-' ? " Min ": " Max "):" ", 
                   rangeInst? "Target":"Operand2", &opStrings[OPVALUE_LEN]);
      str_cat(buf, buf1, buf);
      str_sprintf( buf1, " Instruction:%s Operation:%s.",
                   PCIT::instructionString((PCIT::Instruction)pciInst),
                   PCIT::operationString(operation));
      str_cat(buf, buf1, buf);
      **diagsArea<<DgString0(buf);
      break;

    //Needs to be enhanced for other types. Just dump what ever we have.
    default: 
      ExRaiseSqlError(heap, diagsArea, err);
  };
  
  NADELETEBASICARRAY(buf, (heap));
  return *diagsArea;
}
 
//Detailed error support for clause expression evaluation.
ComDiagsArea *ExRaiseDetailSqlError(CollHeap* heap, 
				    ComDiagsArea** diagsArea,
				    ExeErrorCode err, 
				    ex_clause *clause,
                                    char *op_data[])
{
  if (diagsArea == NULL)
    return NULL;
  
  if (*diagsArea == NULL){
  
    if (heap == NULL)
      return NULL;
    *diagsArea = ComDiagsArea::allocate(heap);
  }
 
  Int16 numOperands = clause->getNumOperands();
  Attributes *op;
  Int16 i; 
  
  // Single allocation of buf is split up to be used for opstrings,
  // formatting. See defines at beginning of this header file 
  // for details. 
  char *buf = new (heap)
    char[(numOperands * (VALUE_TYPE_LEN + OPVALUE_LEN)) + INSTR_LEN + FORMATTING];
  
  if( !buf ){
    ExRaiseSqlError(heap, diagsArea, err);
    return *diagsArea;
  }
  char *opStrings = &buf[(numOperands * VALUE_TYPE_LEN) + INSTR_LEN ];
  // assign FORMATTING part of address.
  char *buf1 = &buf[(numOperands * (VALUE_TYPE_LEN + OPVALUE_LEN)) + INSTR_LEN];
 
  for (i = 1; i < numOperands; i++){
    op = clause->getOperand(i);
    ExConvertErrorToString(heap,
                           diagsArea,
                           (char*)op_data[i],
                           op->getLength(),
                           op->getDatatype(),
                           op->getScale(),
                           (char*)&opStrings[(i-1)*OPVALUE_LEN],
                           OPVALUE_LEN);

  }

  //initialize buf before entering the loop.
  buf[0] = '\0';

  for(i = 1; i< numOperands; i++){
    op = clause->getOperand(i);
    str_sprintf(buf1, " Operand%d Type:%s(%s) Operand%d Value:%s",i,
                        getDatatypeAsString(op->getDatatype(), true),
                        getDatatypeAsString(op->getDatatype(), false),
                        i,
                        &opStrings[(i-1)*OPVALUE_LEN]);
    str_cat(buf, buf1, buf);
  }
  if(numOperands)
  {
    str_sprintf(buf1,".");
    str_cat(buf, buf1, buf);
  }
  str_sprintf(buf1, " Clause Type:%d Clause number:%d Operation:%s.",
                     clause->getType(), clause->clauseNum(),
                     exClauseGetText(clause->getOperType()));
  str_cat(buf, buf1, buf);
  
  **diagsArea << DgSqlCode(-err);
  **diagsArea<<DgString0(buf);
  
  NADELETEBASICARRAY(buf, (heap));
  return *diagsArea;
}


//Detailed error support for conversions, especially for use in convdoit.
ComDiagsArea *ExRaiseDetailSqlError(CollHeap* heap, 
				    ComDiagsArea** diagsArea,
				    ExeErrorCode err, 
				    char *src,
                                    Int32 srcLength,
                                    Int16 srcType,
                                    Int32 srcScale,
                                    Int16 tgtType,
                                    UInt32 flags,
                                    Int32 tgtLength,
                                    Int32 tgtScale,
                                    Int32 tgtPrecision,
                                    Int32 srcPrecision)
{
  //if looping situation, no need to proceed further, return back.
  if(flags & CONV_CONTROL_LOOPING)
    return NULL;

  NABoolean intermediate;

  if( flags & CONV_INTERMEDIATE_CONVERSION )
    intermediate = TRUE;
  else
    intermediate = FALSE;

  if (diagsArea == NULL)
    return NULL;
  
  if (*diagsArea == NULL){
  
    if (heap == NULL)
      return NULL;
    *diagsArea = ComDiagsArea::allocate(heap);
  }
  
  //Allocate buf once, for formatting and opstring[1].
  // |--------formatting--------|--opstring[1]------|
  char *buf = new (heap) char[FORMATTING+ OPVALUE_LEN] ;
  if( !buf ){
    ExRaiseSqlError(heap, diagsArea, err);
    return *diagsArea;
  }
  char *opString = &buf[FORMATTING];
    
  ExConvertErrorToString(heap,
                         diagsArea,
                         src,
                         srcLength,
                         srcType,
                         srcScale,
                         opString,
                         OPVALUE_LEN);

  char srcDatatypeDetail[200];
  char tgtDatatypeDetail[200];
  char srcTypeAsText[1000];
  char tgtTypeAsText[100];
  srcTypeAsText[0] = 0;
  tgtTypeAsText[0] = 0;

  if (srcPrecision != -1)
    {
      rec_datetime_field startField;
      rec_datetime_field endField;
      ExpDatetime::getDatetimeFields(srcPrecision, startField, endField);
      convertTypeToText_basic(srcTypeAsText, 
                              srcType, srcLength, srcPrecision, srcScale,
                              startField, endField, 0, 0, 0, 0,
                              (CharInfo::CharSet)srcScale,
                              NULL, NULL, 0);
      strcpy(srcDatatypeDetail, getDatatypeAsString(srcType, false));
    }
  else if ((DFS2REC::isCharacterString(srcType)) &&
           (srcLength >= 0) &&
           (srcScale > 0) &&
           (srcPrecision == -1))
    str_sprintf(srcDatatypeDetail, "%s,%d BYTES,%s", 
                getDatatypeAsString(srcType, false), 
                srcLength,
                (srcScale == CharInfo::ISO88591 ? "ISO88591" : "UTF8"));
  else
    strcpy(srcDatatypeDetail, getDatatypeAsString(srcType, false));

  if (tgtLength != -1)
    {
      rec_datetime_field startField;
      rec_datetime_field endField;
      ExpDatetime::getDatetimeFields(tgtPrecision, startField, endField);

      convertTypeToText_basic(tgtTypeAsText, 
                              tgtType, tgtLength, tgtPrecision, tgtScale,
                              startField, endField, 0, 0, 0, 0, 
                              (CharInfo::CharSet)tgtScale,
                              NULL, NULL, 0);
      strcpy(tgtDatatypeDetail, getDatatypeAsString(tgtType, false));
    }
  else if ((DFS2REC::isCharacterString(tgtType)) &&
           (tgtLength >= 0) &&
           (tgtScale > 0))
    str_sprintf(tgtDatatypeDetail, "%s,%d %s,%s", 
                getDatatypeAsString(tgtType, false), 
                tgtPrecision ? tgtPrecision : tgtLength,
                tgtPrecision ? "CHARS" : "BYTES",
                (tgtScale == CharInfo::ISO88591 ? "ISO88591" : "UTF8"));
  else
    strcpy(tgtDatatypeDetail, getDatatypeAsString(tgtType, false));

  str_sprintf(buf,
              " %s of Source Type:%s(%s) Source Value:%s to Target Type:%s(%s).",
              intermediate? "Intermediate conversion" : "Conversion",
              (strlen(srcTypeAsText) == 0 ? getDatatypeAsString(srcType,true) :
               srcTypeAsText),              
              srcDatatypeDetail,
              opString,
              (strlen(tgtTypeAsText) == 0 ? getDatatypeAsString(tgtType,true) :
               tgtTypeAsText),
              tgtDatatypeDetail);
  
  **diagsArea << DgSqlCode(-err);
  **diagsArea<<DgString0(buf);
  
  NADELETEBASICARRAY(buf, (heap));
  return *diagsArea;
}

//////////////////////////////////////////////////////////////////
////
//// A helper function to show buffer in HEX
////
//// ///////////////////////////////////////////////////////////////

char *stringToHex(char * out, Int32 outLen, char * in, Int32 inLen)
{

  Int32 hexLen = (outLen / 2) -1 ;
  if (inLen < hexLen) 
     hexLen = inLen;
  if (hexLen < 0)
     hexLen = 0;
  if (outLen > 0)
     out[0] = '\0';
  char hex[3];
  for(int i = 0; i < hexLen; i++)
  {
    snprintf(hex, sizeof(hex), "%02x", (unsigned char)in[i]);
    strcat(out,hex);
  }
  return out;
}

