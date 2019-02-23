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
 * File:         CliExpExchange.cpp
 * Description:  Move data in and out of user host variables
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

#ifdef _DEBUG
#include <assert.h>
#endif

#include "exp_stdh.h"
#include "exp_clause_derived.h"
#include "ex_stdh.h"
#include "cli_stdh.h"
#include "exp_datetime.h"
#include "exp_interval.h"
#include "exp_expr.h"
#include "ExRLE.h"

// defined in exp_eval.C
void computeDataPtr(char * start_data_ptr, // start of data row
                    Lng32 field_num, // field number whose address is to be
                    // computed. Zero based.
                    ExpTupleDesc * td,// describes this row
                    char ** opdata,
                    char ** nulldata,
                    char ** varlendata);

static Lng32 getIntervalCode(short datatype)
{
  switch (datatype) {
  case REC_INT_YEAR:          return SQLINTCODE_YEAR;
  case REC_INT_MONTH:         return SQLINTCODE_MONTH;
  case REC_INT_YEAR_MONTH:    return SQLINTCODE_YEAR_MONTH;
  case REC_INT_DAY:           return SQLINTCODE_DAY;
  case REC_INT_HOUR:          return SQLINTCODE_HOUR;
  case REC_INT_DAY_HOUR:      return SQLINTCODE_DAY_HOUR;
  case REC_INT_MINUTE:        return SQLINTCODE_MINUTE;
  case REC_INT_HOUR_MINUTE:   return SQLINTCODE_HOUR_MINUTE;
  case REC_INT_DAY_MINUTE:    return SQLINTCODE_DAY_MINUTE;
  case REC_INT_SECOND:        return SQLINTCODE_SECOND;
  case REC_INT_MINUTE_SECOND: return SQLINTCODE_MINUTE_SECOND;
  case REC_INT_HOUR_SECOND:   return SQLINTCODE_HOUR_SECOND;
  case REC_INT_DAY_SECOND:    return SQLINTCODE_DAY_SECOND;
  default:
    break;
  }
  return 0;
}

static void setRowNumberInCli(ComDiagsArea * diagsArea, Lng32 rowNum, Lng32 rowsetSize)
{
  if ((rowsetSize > 0) && diagsArea) {
    diagsArea->setAllRowNumber(rowNum);
  }
}

ex_expr::exp_return_type InputOutputExpr::describeOutput(void * output_desc_,
							 UInt32 flags)
{
  NABoolean isOdbc = isODBC(flags);
  NABoolean isDbtr = isDBTR(flags);
  NABoolean isIFIO  = isInternalFormatIO(flags);

  ex_clause *clause = getClauses();
  Descriptor * output_desc = (Descriptor *)output_desc_;
  short entry = 0;

#ifdef _DEBUG
  short tempAtp = -1;  // For testing 
#endif  

  Lng32 prevEntryStartOffset = 0;
  Lng32 currAlignedOffset = 0;
  Lng32 firstEntryStartAlignment = 0;

  Lng32 length = -1;
  NABoolean firstEntryProcessed = FALSE;

  while (clause)
    {
      if (clause->getType() == ex_clause::INOUT_TYPE)
        {
          Attributes * operand = clause->getOperand(0);
	  if(isCall() && output_desc->isDescTypeWide())
	    entry = ((ex_inout_clause *)clause)->getParamIdx();
	  else
	    entry++;

          output_desc->setDescItem(entry, SQLDESC_TYPE_FS, operand->getDatatype(),
                                   0);

	  length = operand->getLength();
	  
          if ((operand->getDatatype() >= REC_MIN_INTERVAL) &&
              (operand->getDatatype() <= REC_MAX_INTERVAL)) {
            //
            // According to ANSI, interval info is stored in the following
            // descriptor items:
            //
            // DATETIME_INTERVAL_CODE = qualifier code
            // PRECISION = fractional precision
            // DATETIME_INTERVAL_PRECISION = leading precision
            //
            output_desc->setDescItem(entry,
                                     SQLDESC_DATETIME_CODE,
                                     getIntervalCode(operand->getDatatype()),
                                     0);
            output_desc->setDescItem(entry,
                                     SQLDESC_PRECISION,
                                     operand->getScale(),
                                     0);
            output_desc->setDescItem(entry, SQLDESC_SCALE, 0, 0);
            output_desc->setDescItem(entry,
                                     SQLDESC_INT_LEAD_PREC,
                                     operand->getPrecision(),
                                     0);
	    if (NOT isIFIO)
	      {
		Lng32 displaySize =
		  ExpInterval::getDisplaySize(operand->getDatatype(),
					      operand->getPrecision(),
					      operand->getScale());
		length = displaySize;
	      }
          } else if (operand->getDatatype() == REC_DATETIME) {
            //
            // According to ANSI, datetime info is stored in the following
            // descriptor items:
            //
            // DATETIME_INTERVAL_CODE = qualifier code
            // PRECISION = fractional precision
            //
            output_desc->setDescItem(entry,
                                     SQLDESC_DATETIME_CODE,
                                     operand->getPrecision(),
                                     0);
            output_desc->setDescItem(entry,
                                     SQLDESC_PRECISION,
                                     operand->getScale(),
                                     0);
            output_desc->setDescItem(entry, SQLDESC_SCALE, 0, 0);
            output_desc->setDescItem(entry, SQLDESC_INT_LEAD_PREC, 0, 0);

	    if (((NOT isOdbc) || (output_desc->rowwiseRowsetV2() == FALSE)) &&
		(NOT isIFIO))
	      {
		Lng32 displaySize =
		  ExpDatetime::getDisplaySize(operand->getPrecision(),
					      operand->getScale());
		length = displaySize;
	      }

          } else {
            output_desc->setDescItem(entry, SQLDESC_DATETIME_CODE, 0, 0);
            output_desc->setDescItem(entry,
                                     SQLDESC_PRECISION,
                                     operand->getPrecision(),
                                     0);
            output_desc->setDescItem(entry,
                                     SQLDESC_SCALE,
                                     operand->getScale(),
                                     0);
            output_desc->setDescItem(entry, SQLDESC_INT_LEAD_PREC, 0, 0);
          }
	  
          // Use SQLDESC_CHAR_SET_NAM (one-part name) for charset

	  if ( DFS2REC::isAnyCharacter(operand->getDatatype()) || 
               (operand->getDatatype() == REC_BLOB) || 
               (operand->getDatatype() == REC_CLOB )) 
            {
              output_desc->setDescItem(entry, SQLDESC_CHAR_SET_NAM, 0, 
                                       (char*)CharInfo::getCharSetName(operand->getCharSet()));

              // reset the length for Unicode
            if ( operand->getCharSet() == CharInfo::UNICODE ||
                 CharInfo::is_NCHAR_MP(operand->getCharSet())
                 )
	      {
		length = operand->getLength()/SQL_DBCHAR_SIZE;
	      }

	    if (operand->isCaseinsensitive())
              output_desc->setDescItem(entry, SQLDESC_CASEINSENSITIVE, 1, 0);
	      
	    if (operand->getCollation() != CharInfo::DefaultCollation)
              output_desc->setDescItem(entry, SQLDESC_COLLATION,
                                       operand->getCollation(), 0);
	  }

          output_desc->setDescItem(entry, SQLDESC_NULLABLE,
                                   operand->getNullFlag(), 0);
          output_desc->setDescItem(entry, SQLDESC_NAME, 0, 
                                   ((ex_inout_clause *)clause)->getName());
          if (((ex_inout_clause *)clause)->getHeading() != 0)
            output_desc->setDescItem(entry, SQLDESC_HEADING, 0, 
                                    ((ex_inout_clause *)clause)->getHeading());

	  if (((ex_inout_clause *)clause)->getTableName() != 0)
            output_desc->setDescItem(entry, SQLDESC_TABLE_NAME, 0, 
                                    ((ex_inout_clause *)clause)->getTableName());

	  if (((ex_inout_clause *)clause)->getSchemaName() != 0)
            output_desc->setDescItem(entry, SQLDESC_SCHEMA_NAME, 0, 
                                    ((ex_inout_clause *)clause)->getSchemaName());
	  if (((ex_inout_clause *)clause)->getCatalogName() != 0)
            output_desc->setDescItem(entry, SQLDESC_CATALOG_NAME, 0, 
                                    ((ex_inout_clause *)clause)->getCatalogName());

	  
          output_desc->setDescItem(entry, SQLDESC_VAR_PTR, 0, 0);
          output_desc->setDescItem(entry, SQLDESC_IND_PTR, 0, 0);
          output_desc->setDescItem(entry, SQLDESC_VAR_DATA, 0, 0);
          output_desc->setDescItem(entry, SQLDESC_IND_DATA, 0, 0);
	  
          if (operand->getRowsetSize() > 0) {
            output_desc->setDescItem(0, SQLDESC_ROWSET_SIZE, 
                                     operand->getRowsetSize(), 0);
            output_desc->setDescItem(entry, SQLDESC_ROWSET_VAR_LAYOUT_SIZE,
                                     operand->getLength(), 0);
            output_desc->setDescItem(entry, SQLDESC_ROWSET_IND_LAYOUT_SIZE,
                                     operand->getNullIndicatorLength(),
                                     0);
          }
	  else {
            output_desc->setDescItem(0, SQLDESC_ROWSET_SIZE, 
                                     0, 0);
            output_desc->setDescItem(entry, SQLDESC_ROWSET_VAR_LAYOUT_SIZE,
                                     0, 0);
            output_desc->setDescItem(entry, SQLDESC_ROWSET_IND_LAYOUT_SIZE,
                                     0,
                                     0);
	  }
	  
	  short paramMode = ((ex_inout_clause *)clause)->getParamMode();
	  short paramIdx = ((ex_inout_clause *)clause)->getParamIdx();
	  short ordPos = ((ex_inout_clause *)clause)->getOrdPos();

	  BriefAssertion((paramMode == PARAMETER_MODE_INOUT) ||
			 (paramMode == PARAMETER_MODE_OUT)||
			 (paramMode == PARAMETER_MODE_UNDEFINED),
			 "invalid param mode");
	  BriefAssertion(paramIdx >= 0, "invalid param index");
	  BriefAssertion(ordPos >= 0, "invalid param ordinal position");

	  // handle old plan
	  if(paramMode == PARAMETER_MODE_UNDEFINED){
	    paramMode = PARAMETER_MODE_OUT;
	  }
	  output_desc->setDescItem(entry, SQLDESC_PARAMETER_MODE, paramMode, 0);
	  output_desc->setDescItem(entry, SQLDESC_PARAMETER_INDEX, paramIdx, 0);
	  output_desc->setDescItem(entry, SQLDESC_ORDINAL_POSITION, ordPos, 0);

	  if(isCall()){
	    short paramMode = ((ex_inout_clause *)clause)->getParamMode();
	    short paramIdx = ((ex_inout_clause *)clause)->getParamIdx();
	    short ordPos = ((ex_inout_clause *)clause)->getOrdPos();
	    output_desc->setDescItem(entry, SQLDESC_PARAMETER_MODE,
				     paramMode, 0);
	    output_desc->setDescItem(entry, SQLDESC_PARAMETER_INDEX,
				     paramIdx, 0);
	    output_desc->setDescItem(entry, SQLDESC_ORDINAL_POSITION,
				     ordPos, 0);
	  }
	  else{
	    output_desc->setDescItem(entry, SQLDESC_PARAMETER_MODE,
				     PARAMETER_MODE_OUT, 0);
	    output_desc->setDescItem(entry, SQLDESC_PARAMETER_INDEX,
				     entry, 0);
	    output_desc->setDescItem(entry, SQLDESC_ORDINAL_POSITION,
				     0, 0);
          }

	  output_desc->setDescItem(entry, SQLDESC_LENGTH, length, 0);

	  // Set up length of null indicator and varcharlen indicator values
	  // in the descriptor.
	  //
          output_desc->setDescItem(entry, SQLDESC_IND_LENGTH,
                                   operand->getNullIndicatorLength(), 0);
          output_desc->setDescItem(entry, SQLDESC_VC_IND_LENGTH,
                                   operand->getVCIndicatorLength(), 0);

	  // Get offsets to data, null indicator and varchar len indicator
	  // in the actual row and set them in the descriptor.
	  //
	  // This is being done for sqlark_exploded_format only.
	  // The offsets that are set in descriptor assume that the returned
	  // row to user is a single contiguous aligned row even though internally
	  // that row could be represented by multiple fragments, each with
	  // different atp index.
	  // 
	  // These values are used by caller(mxcs) to set up input and output
	  // data pointers so that they are aligned and set up the same way
	  // as the actual row in cli.
	  //
	  // These values cannot be set by external callers.
	  //

	  Lng32 dataOffset = -1;
	  Lng32 nullIndOffset = -1;
	  Lng32 currEntryStartOffset = -1;
	  Lng32 currEntryStartAlignment = -1;

	  Lng32 outputDatalen = -1;
	  if (output_desc->getDescItem(entry, SQLDESC_OCTET_LENGTH, &outputDatalen, NULL, 0, NULL, 0, NULL, 0) == ERROR)
            return ex_expr::EXPR_ERROR;

	  Lng32 alignment = 1;
	  if (operand->getNullIndOffset() >= 0)
	    {
	      if (operand->getTupleFormat() == ExpTupleDesc::SQLARK_EXPLODED_FORMAT)
		alignment = operand->getNullIndicatorLength();
	      
	      currAlignedOffset = ADJUST(currAlignedOffset, alignment);
	      nullIndOffset = currAlignedOffset;
	      
	      currEntryStartOffset = currAlignedOffset;
	      currEntryStartAlignment = alignment;

	      currAlignedOffset += operand->getNullIndicatorLength();
	    }
	  
	  if (operand->getVCLenIndOffset() >= 0)
	    {
	      if (operand->getTupleFormat() == ExpTupleDesc::SQLARK_EXPLODED_FORMAT)
		alignment = operand->getVCIndicatorLength();
	      currAlignedOffset = ADJUST(currAlignedOffset, alignment);
	      dataOffset = currAlignedOffset;

	      if (currEntryStartOffset == -1)
		{
		  currEntryStartOffset = currAlignedOffset;
		  currEntryStartAlignment = alignment;
		}

	      currAlignedOffset += operand->getVCIndicatorLength() + outputDatalen;
	    }
	  else
	    {
	      if (operand->getTupleFormat() == ExpTupleDesc::SQLARK_EXPLODED_FORMAT)
		alignment = operand->getDataAlignmentSize();
	      currAlignedOffset = ADJUST(currAlignedOffset, alignment);
	      dataOffset = currAlignedOffset;

	      if (currEntryStartOffset == -1)
		{
		  currEntryStartOffset = currAlignedOffset;
		  currEntryStartAlignment = alignment;
		}

	      currAlignedOffset += outputDatalen;
	    }

	  if (NOT firstEntryProcessed)
	    {
	      firstEntryProcessed = TRUE;
	      firstEntryStartAlignment = currEntryStartAlignment;
	    }

	  output_desc->setDescItemInternal(entry, SQLDESC_DATA_OFFSET,
					   dataOffset, 0);
	  output_desc->setDescItemInternal(entry, SQLDESC_NULL_IND_OFFSET,
					   nullIndOffset, 0);
	  
	  output_desc->setDescItemInternal(entry-1, SQLDESC_ALIGNED_LENGTH,
					   currEntryStartOffset - prevEntryStartOffset, 0);

	  prevEntryStartOffset = currEntryStartOffset;

#ifdef _DEBUG
// Start testing logic  *****
	  //
	  // Set up the descriptor rowwise offsets to enable bulk move when possible.
	  // The offset is only valid for one tuple. Set the values to -1 for all
	  // the other tuples. 
	  //
          if (tempAtp == -1)
            tempAtp = operand->getAtpIndex();
          if (tempAtp == operand->getAtpIndex()){
	    output_desc->setDescItem(entry, SQLDESC_ROWWISE_VAR_OFFSET, operand->getOffset(), 0);
	    output_desc->setDescItem(entry, SQLDESC_ROWWISE_IND_OFFSET, operand->getNullIndOffset(), 0);
	  }
          else{
	    output_desc->setDescItem(entry, SQLDESC_ROWWISE_VAR_OFFSET, -1, 0);
	    output_desc->setDescItem(entry, SQLDESC_ROWWISE_IND_OFFSET, -1, 0);
	  }
// End testing logic  *****
#endif
        }
      clause = clause->getNextClause();
    }

  if (firstEntryProcessed)
    {
      currAlignedOffset = ADJUST(currAlignedOffset, firstEntryStartAlignment);
      output_desc->setDescItemInternal(entry, SQLDESC_ALIGNED_LENGTH,
				       currAlignedOffset - prevEntryStartOffset, 0);
    }

  if(!(isCall() && output_desc->isDescTypeWide())
     || (entry > output_desc->getUsedEntryCount()))
    output_desc->setUsedEntryCount(entry);

  return ex_expr::EXPR_OK;
}

//
// This method will evaluate the characteristics of the operand and descriptor
// to determine if a value can be bulk moved given rowwise rowset version 1 
// bulk move rules. The method also has the side affect of setting the varPtr
// parameter to the proper location in the descriptor's data buffer for this
// items bulk move start address.
//
Descriptor::BulkMoveStatus Descriptor::checkBulkMoveStatusV1(
     short entry, Attributes * op,
     Long &varPtr,
     NABoolean isInputDesc,
     NABoolean isRWRS,
     NABoolean isInternalFormatIO)
{
#ifdef _DEBUG
  //  if (! getenv("DOSLOWBULKMOVE"))
  //    return BULK_MOVE_OFF;
  if (getenv("BULKMOVEOFF"))
    return BULK_MOVE_OFF;
  else if (getenv("BULKMOVEDISALLOWED"))
    return BULK_MOVE_DISALLOWED;
#endif

  desc_struct  &descItem =  desc[entry - 1]; // Zero base

  // if any of the first set of conditions is true, bulk move cannot
  // be done for any value of this row. Turn off bulk move completely.
  if ((rowsetSize > 0 )                           ||
      //      ((NOT rowwiseRowset()) && 
      (descItem.var_ptr == 0)                     ||
      (op->getVCIndicatorLength() > 0)            ||
      (DFS2REC::isAnyVarChar(op->getDatatype()))  ||
      (((DFS2REC::isDateTime(op->getDatatype()))    ||
	(DFS2REC::isInterval(op->getDatatype()))) && 
       (NOT isInternalFormatIO))           ||
      (op->isAddedCol()) ||
      (!op->isBulkMoveable())
      )
    return BULK_MOVE_OFF;

  // if any of these conditions are true, the bulk move for this
  // value cannot be done. Turn off bulk move for this value.
  //
  // If descriptor item is binary with precision > 0, which means that
  // it was declared as a NUMERIC datatype, then it needs to
  // be validated at input and output time. Cannot do bulk move in
  // that case.
  // At input, always validate it.
  // At output, only validate if descItem's precision is greater than
  // zero, and less than operand's precision or op's precision is zero.
  //
  // Bulk move for char datatype is only done if both source and
  // target are single byte charsets and the target doesn't enforce
  // character limits.
  else if ((descItem.datatype != op->getDatatype())    ||
	   ((DFS2REC::isAnyCharacter(op->getDatatype())) &&
	    ((descItem.charset != op->getCharSet()) ||
	     (NOT CharInfo::isSingleByteCharSet((CharInfo::CharSet)descItem.charset)) ||
             op->getPrecision() /*char limit*/ > 0)) ||
	   ((NOT DFS2REC::isAnyCharacter(op->getDatatype())) &&
	    (descItem.scale != op->getScale())) || 
	   (descItem.length != op->getLength())        ||
	   ((descItem.datatype >= REC_MIN_BINARY_NUMERIC) &&
	    (descItem.datatype <= REC_MAX_BINARY_NUMERIC) &&
	    (((isInputDesc) &&
	      (descItem.precision > 0)) ||
	     ((NOT isInputDesc) &&
	      (descItem.precision > 0) &&
	      ((op->getPrecision() == 0) ||
	       (op->getPrecision() > descItem.precision))))) ||
	   (op->getNullFlag())                         ||
	   (descItem.nullable != 0))
    return BULK_MOVE_DISALLOWED;

  // bulk move could be done for this value.
  else
    {
      varPtr = descItem.var_ptr;
      return BULK_MOVE_OK;
    }
}  // end Descriptor::checkBulkMoveStatusV1

//
// This method will return the proper location in the 
// descriptor's data buffer for this entry based on
//
Long Descriptor::getRowsetVarPtr(short entry)
{
  Long          varPtr   = 0;
  desc_struct  &descItem =  desc[entry - 1]; // Zero base

  //
  // Set the varPtr to the appropriate place.
  // 
  if (descItem.nullable != 0)
    varPtr = descItem.ind_ptr;
  else
    varPtr = descItem.var_ptr;

  return varPtr;

}  // end getRowsetVarPtr

//
// This method will evaluate the characteristics of the operand and descriptor
// to determine if a value can be bulk moved given rowwise rowset version 2 
// bulk move rules. The method also has the side affect of setting the varPtr
// parameter to the proper location in the descriptor's data buffer for this
// items bulk move start address.
//
Descriptor::BulkMoveStatus Descriptor::checkBulkMoveStatusV2(
     short entry, Attributes * op,
     Long &varPtr,
     NABoolean isInputDesc,
     NABoolean isOdbc,
     NABoolean isRWRS,
     NABoolean isInternalFormatIO)
{
#ifdef _DEBUG
  //  if (! getenv("DOSLOWBULKMOVE"))
  //    return BULK_MOVE_OFF;
  if (getenv("BULKMOVEOFF"))
    return BULK_MOVE_OFF;
  else if (getenv("BULKMOVEDISALLOWED"))
    return BULK_MOVE_DISALLOWED;
#endif

  desc_struct  &descItem =  desc[entry - 1]; // Zero base

  // if any of the first set of conditions is true, bulk move cannot
  // be done for any value of this row. Turn off bulk move completely.
  if ((NOT isRWRS) &&
      ((rowsetSize > 0 )       ||
       (descItem.var_ptr == 0) ||
       (op->isAddedCol()))
      )
    return BULK_MOVE_OFF;

  // If any of these conditions are true, bulk move for this
  // value cannot be done. Turn off bulk move for this value.
  //
  else if (
	// Check to make sure the descriptor and operand have the same data type.
	   (descItem.datatype != op->getDatatype())    
	// Check to make sure the descriptor and operand character sets are the same.
	 ||  ((DFS2REC::isAnyCharacter(op->getDatatype())) && (descItem.charset != op->getCharSet())) 
	// Check to make sure the descriptor and operand scale is the same.    
	 ||  ((NOT DFS2REC::isAnyCharacter(op->getDatatype())) 
               && (NOT DFS2REC::isDateTime(op->getDatatype())) 
              && (NOT DFS2REC::isInterval(op->getDatatype()))
              && (descItem.scale != op->getScale()))
	// Check special case where descriptor is data/time. In that case the scale is stored in the precision.
	// Check to make sure the descriptor and operand scale is the same.    
           ||  ((DFS2REC::isDateTime(op->getDatatype()) 
                 || DFS2REC::isInterval(op->getDatatype())) 
                && (descItem.precision != op->getScale()))
        // Check to make sure the descriptor and operand lengths are the same.
         ||  (descItem.length != op->getLength())
	// Check if descriptor item is binary with precision > 0, which means that
        //   it was declared as a NUMERIC datatype, then it needs to be validated 
        //   at input and output time. Cannot do bulk move in that case.  At input, 
	//   always validate it.  At output, only validate if descItem's precision 
	//   is greater than zero, and less than operand's precision or op's 
	//   precision is zero.
         ||
	   ((descItem.datatype >= REC_MIN_BINARY_NUMERIC) &&
            (descItem.datatype <= REC_MAX_BINARY_NUMERIC) &&
	    (((isInputDesc) &&
	      (NOT isRWRS) &&
	      (descItem.precision > 0)) ||
	     ((NOT isInputDesc) && (descItem.precision > 0)
	      && ((op->getPrecision() == 0) || (op->getPrecision() > descItem.precision)))
	     )
	    )
	// Check to make sure the descriptor and operand both have null flags.
         ||
           (op->getNullFlag() && !descItem.ind_ptr)
	// Check to make sure the descriptor and operand both do not have null flags.
         ||
           (!op->getNullFlag() && descItem.ind_ptr)
	// Check to make sure the descriptor and operand both not nullable if this is for input values.
	   // || 
	   //  (isInputDesc && (op->getNullFlag() || descItem.nullable != 0))
	// Check to make sure the operand data type is not a binary precision integer.
         ||
	     (op->getDatatype() == REC_BPINT_UNSIGNED)
	// Check to make sure the operand is bulk movable.
         ||
	   (op->isBulkMoveable() == FALSE)
	// Check to make sure if the data type is date/time that the request is coming from odbc.
         ||
	   ((DFS2REC::isDateTime(op->getDatatype())) && (isOdbc == FALSE) && (NOT isInternalFormatIO))
	// Check to make sure the the operand is not an interval
         ||
	   ((DFS2REC::isInterval(op->getDatatype())) && (NOT isInternalFormatIO))
	// Check to make sure that if the descriptor data type is varcha, then it is an ANSI varchar.
         ||
	   ((DFS2REC::isAnyVarChar(descItem.datatype))
       && !(descItem.datatype == REC_BYTE_V_ASCII || descItem.datatype == REC_NCHAR_V_UNICODE))
     )
    {
    // MXCS must always propertly set up the descriptor to allow bulk move unless the
    // data type is interval. An interval datatype is returned in external format. 
    //BriefAssertion( !((isOdbc == TRUE) && ((DFS2REC::isInterval(op->getDatatype()) == FALSE)))
    //              ,  "Only bulk move allowed for rowwise rowsets type 2 and MXCS");
    return BULK_MOVE_DISALLOWED;
    }  // end else if

  //
  // If nullable, make sure null indicator offsets match
  //
  Int32 opIndOffset   = 0;
  Int32 descIndOffset = 0;
  if (descItem.nullable != 0)
    {
    if (op->getVCIndicatorLength())
      opIndOffset = op->getNullIndOffset() - op->getVCLenIndOffset();
    else
      opIndOffset = op->getNullIndOffset() - op->getOffset();
    descIndOffset = descItem.ind_ptr - descItem.var_ptr;
    if (opIndOffset != descIndOffset)
      {
      // MXCS must always set up the descriptor such that the indicator and var pointers 
      // correctly aligned. 
      //BriefAssertion((isOdbc == FALSE) , "Indicator/var pointer alignment problem for rowwise rowset type 2 with MXCS");
	if ((NOT isRWRS) || (NOT isOdbc))
	  return BULK_MOVE_DISALLOWED;
      }
    }

  //
  // Bulk move can be done for this value.
  // Set the varPtr to the appropriate place.
  // 
  if (descItem.nullable != 0)
    varPtr = descItem.ind_ptr;
  else
    varPtr = descItem.var_ptr;

  return BULK_MOVE_OK;
}  // end checkBulkMoveStatusV2



void InputOutputExpr::setupBulkMoveInfo(void * desc_, CollHeap * heap,
					NABoolean isInputDesc,
					UInt32 flags)
{
  NABoolean isOdbc = isODBC(flags);
  NABoolean isDbtr = isDBTR(flags);
  NABoolean isRwrs = isRWRS(flags);
  NABoolean isIFIO  = (isInternalFormatIO(flags) ||
		       (isDbtr && isRwrs));

  ex_clause  * clause      = getClauses();
  Descriptor * desc = (Descriptor *)desc_;
  short entry = 0;
  Long firstDescStartPtr = -1;
  Long currDescStartPtr = 0;
  Long currExeStartOffset = 0;
  short currFirstEntryNum = 0;
  Long descVarPtr;
  Lng32 currLength = 0;
  Long opOffset = 0;
  short currAtpIndex = 0;
  short bmEntry = 0;
  NABoolean currIsVarchar = FALSE;
  NABoolean currIsNullable = FALSE;

  if (desc->bulkMoveDisabled())
    return;

  if (getNumEntries() == 0)
    {
      desc->setBulkMoveDisabled(TRUE);
      return;
    }

  // bulk move disabled for wide descriptors.
  if (isCall() && desc->isDescTypeWide())
    {
      desc->setBulkMoveDisabled(TRUE);
      return;
    }

  desc->setBulkMoveDisabled(FALSE);

  desc->setDoSlowMove(FALSE);

  NABoolean bulkMoveInfoAllocated = FALSE;
  while (clause) 
    {
      if (clause->getType() == ex_clause::INOUT_TYPE) 
	{
	  entry++;
	  Attributes * op = clause->getOperand(0);
          
	  if (((ex_inout_clause*)clause)->excludeFromBulkMove())
	    {
	      // this clause has been excluded from bulk move.
	      // It could be used to get rowwise rowset specific
	      // information, like rowset buffer length, max size, etc.
	      // Skip this clause.
	      // move to the next entry
	      goto next_clause;
	    }

	  //   firstDescStartPtr  - Represents a pointer to the first entry
          if (entry == 1)
	    firstDescStartPtr  = desc->getRowsetVarPtr(entry);

	  Descriptor::BulkMoveStatus bms = 
	    desc->checkBulkMoveStatus(entry, op, descVarPtr, isInputDesc, 
				      isOdbc, isRwrs, isIFIO);
	  
	  if (bms == Descriptor::BULK_MOVE_OFF)
	    {   // It is not possible to do a bulk move
	      desc->deallocBulkMoveInfo();
	      desc->setBulkMoveDisabled(TRUE);
	      return;
	    }
	  else if (bms == Descriptor::BULK_MOVE_DISALLOWED)
	    {
	      // Bulk move is not allowed for this case, do slow move using convDoIt.
	      // Mark the descriptor indicating slow move. This indicator
	      // will be used in input and outputValues method to call
	      // convDoIt.
	      desc->setDoSlowMove(entry, TRUE);

	      // Also set this flag in the descriptor.
	      desc->setDoSlowMove(TRUE);
	      
	      // move to the next entry
	      goto next_clause;
	    }
	  else
	    {
	      // bulk move could be done. Reset 'slow move' flag.
	      desc->setDoSlowMove(entry, FALSE);

	      bmEntry++;

	      // if first time, allocate bulkMoveInfo.
	      if (NOT bulkMoveInfoAllocated)
		{
		  desc->allocBulkMoveInfo();
		  bulkMoveInfoAllocated = TRUE;
		}
	    }

          NABoolean opIsVarchar = FALSE;
          
          if (op->getDatatype() >= REC_MIN_V_CHAR_H &&
  	      op->getDatatype() <= REC_NCHAR_V_UNICODE)
            opIsVarchar = TRUE;

          if (op->getNullFlag() != 0)
            opOffset = (Long)op->getNullIndOffset();
          else
	    {
            if (opIsVarchar)
              opOffset = (Long)op->getVCLenIndOffset();
            else
  	      opOffset = (Long)op->getOffset();
	    }

	  if (op->getAtpIndex() == 0)                     // constant
	    opOffset += (Long)getConstantsArea();
	  else if (op->getAtpIndex() == 1)                // temp
	    opOffset += (Long)getTempsArea();

	  if ((isInputDesc) && (isOdbc) && (isRwrs))
	    {
	      descVarPtr = opOffset;

	      if (firstDescStartPtr < 0)
		firstDescStartPtr  = descVarPtr;
	    }

	  //
	  // The following code builds "blocks" of entries that can be bulk moved. That is, 
	  // one or move entries are grouped together in blocks if the entries are adjacent
	  // to each other in both descriptor and operand.
	  //
	  // The variables involved and their purpose are as follows:
	  //
	  //   bmEntry            - Keeps track of the number of operands that have been added 
	  //                        to the bulk move list
	  //   opOffset           - Holds the offset value of the current bulk move entry
	  //   firstDescStartPtr  - Represents a pointer to the first entry.
	  //                        When data is moved, the offset to the bulk move entry
          //                        in the descriptor's data will be calculated as an
	  //                        offset from this pointer.
	  //   currDescStartPtr   - Represents a pointer to the current bulk move entry 
	  //                        descriptor var pointer.
	  //   currExeStartOffset - Holds the offset to the current bulk move block of entries.
	  //   currLength         - Holds the length of the current move block of entries.
	  //   currfirstEntryNum  - Represents the entry number of the first entry in the current
	  //                        block of entries.
	  //
	  // After all entries have been processed, the logic detects the last bulk move,
	  // and adds it to the list of bulk moves.
	  //
	  
	  if (bmEntry == 1) // first time
	    {
	      currDescStartPtr   = descVarPtr;
	      currAtpIndex       = op->getAtpIndex();
	      currExeStartOffset = opOffset;

              if (op->getNullFlag() != 0)
                currLength = (op->getOffset() - op->getNullIndOffset()) + op->getLength();
              else if (opIsVarchar)
	        currLength = op->getLength() + op->getVCIndicatorLength();
              else
	        currLength = op->getLength();

	      currFirstEntryNum  = 1;
              if (!isInputDesc) 
              {
                currIsVarchar = opIsVarchar;
                currIsNullable = op->getNullFlag();
              }
	    }
	  else if ((op->getAtpIndex() != currAtpIndex) ||
		   (opOffset < currExeStartOffset) ||
		   (descVarPtr < currDescStartPtr) ||
		   ((descVarPtr - currDescStartPtr) !=
		    (opOffset - currExeStartOffset)) ||
                   (!isInputDesc && (opIsVarchar || currIsVarchar)))
		   // (descVarPtr - currDescStartPtr) != currLength)
	    {
	      desc->bulkMoveInfo()->
		addEntry(currLength,
			 (((desc->rowwiseRowset()) &&
			   (NOT desc->rowwiseRowsetDisabled())) ?
			  
			  // this one is an offset
			  (char*)(currDescStartPtr - firstDescStartPtr) :

			  (char*)currDescStartPtr),
			 currAtpIndex,
			 (currAtpIndex <= 1 ? TRUE : FALSE),
			 currExeStartOffset,
			 currFirstEntryNum,
			 (short)(bmEntry-1),
                         currIsVarchar,
                         currIsNullable);
			 
	      currDescStartPtr   = descVarPtr;
	      currAtpIndex       = op->getAtpIndex();
	      currExeStartOffset = opOffset;
              if (op->getNullFlag() != 0)
                currLength       = (op->getOffset() - op->getNullIndOffset()) + op->getLength();
              else if (opIsVarchar)
	        currLength = op->getLength() + op->getVCIndicatorLength();
              else
	        currLength = op->getLength();
	      currFirstEntryNum = bmEntry;
              if (!isInputDesc) 
              {
                currIsVarchar = opIsVarchar;
                currIsNullable = op->getNullFlag();
              }
	    }
	  else
	    {
              if (op->getNullFlag() != 0)
                currLength = opOffset - currExeStartOffset + 
                             (op->getOffset() - op->getNullIndOffset()) + op->getLength();
              else if (opIsVarchar)
	        currLength = opOffset - currExeStartOffset + op->getLength() + op->getVCIndicatorLength();
              else
      	        currLength = opOffset - currExeStartOffset + op->getLength();
	    }
	}

next_clause:      
      clause = clause->getNextClause();
    } // loop over clauses
  
  if ((desc->bulkMoveInfo()) &&
      (bmEntry > 0)) // found at least one entry which does bulk move.
    desc->bulkMoveInfo()->
      addEntry(currLength,
	       (((desc->rowwiseRowset()) &&
		 (NOT desc->rowwiseRowsetDisabled())) ?
		
		// this one is an offset
		(char*)(currDescStartPtr - firstDescStartPtr) :
		
		(char*)currDescStartPtr),
	       currAtpIndex,
	       (currAtpIndex <= 1 ? TRUE : FALSE),
	       currExeStartOffset,
	       currFirstEntryNum, bmEntry,
               currIsVarchar,
               currIsNullable);
  
  desc->setBulkMoveSetup(TRUE);

  if (getenv("BULKMOVE") && getenv("BULKMOVEINFO") &&
      desc->bulkMoveInfo())
    {
      if (isInputDesc)
	cout << "InputDesc, Used Entries " << 
	  desc->bulkMoveInfo()->usedEntries() << endl;
      else
	cout << "OutputDesc, Used Entries " << 
	  desc->bulkMoveInfo()->usedEntries() << endl;

      for (UInt32 i = 0; i < desc->bulkMoveInfo()->usedEntries(); i++)
	{
	  cout << "Entry #" << i+1 << endl;
	  cout << "desc ptr = " << 
	    desc->bulkMoveInfo()->getDescDataPtr(i)
	       << ", exe offset = " << desc->bulkMoveInfo()->getExeOffset(i)
	       << ", length = " << desc->bulkMoveInfo()->getLength(i) 
	       << ", atpindex = " << desc->bulkMoveInfo()->getExeAtpIndex(i) 
	       << endl;
	  cout << "firstEntryNum = " <<
	    desc->bulkMoveInfo()->getFirstEntryNum(i) << 
	    ", lastEntryNum = " <<  
	    desc->bulkMoveInfo()->getLastEntryNum(i) << endl;
	  cout << endl;
	}
    }
}

ex_expr::exp_return_type InputOutputExpr::doBulkMove(atp_struct * atp,
						     void * desc_,
						     char * tgtRowPtr,
						     NABoolean isInputDesc)
{
  Descriptor * desc = (Descriptor *)desc_;
  BulkMoveInfo * bmi = desc->bulkMoveInfo();

  if (!bmi)
    return ex_expr::EXPR_OK;
    
  for (Int32 i = 0; i < (Lng32)bmi->usedEntries(); i++)
    {
      char * dataPtr;
      if (NOT bmi->isExeDataPtr(i))
	{
	  // compute ptr from offset
	  if ((isInputDesc) && (tgtRowPtr))
	    dataPtr = tgtRowPtr + bmi->getExeOffset(i);
	  else
	    dataPtr = 
	      atp->getTupp(bmi->getExeAtpIndex(i)).getDataPointer() + 
	      bmi->getExeOffset(i);
	}
      else
	// it is already a pointer. Use it.
	dataPtr = bmi->getExeDataPtr(i);

      if (isInputDesc)
	{
          char * srcPtr = NULL;
          if (desc->rowwiseRowset())
            srcPtr = (char*)(desc->getCurrRowPtrInRowwiseRowset() + bmi->getDescDataPtr(i));
          else 
            srcPtr = bmi->getDescDataPtr(i);

          str_cpy_all(dataPtr, srcPtr, bmi->getLength(i));
	}
      else
	{
          char *destPtr =  NULL;
          if ((desc->rowwiseRowset()) && (NOT desc->rowwiseRowsetDisabled()))
            destPtr = (char*)(desc->getCurrRowPtrInRowwiseRowset() + bmi->getDescDataPtr(i));
          else
            destPtr = bmi->getDescDataPtr(i);

	  str_cpy_all(destPtr, dataPtr, (Lng32)bmi->getLength(i));
	}
    }

  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type
InputOutputExpr::outputValues(atp_struct *atp,
                              void * output_desc_, 
                              CollHeap *heap,
                              UInt32 flags)
{
  NABoolean isOdbc = isODBC(flags);
  NABoolean isIFIO  = isInternalFormatIO(flags);
  
  ex_clause  * clause      = getClauses();
  Descriptor * output_desc = (Descriptor *)output_desc_;
  NABoolean useParamIdx = isCall() && output_desc->isDescTypeWide();
  Attributes * operand     = 0;
  ComDiagsArea * diagsArea = atp->getDiagsArea();
  
  char * source = 0;
  char * sourceVCLenInd = 0;
  char * sourceNullInd = 0;
  Lng32   sourceLen = 0; // Avoid possible uninitialized variable when saving in savedSourceLen.
  Lng32   sourceNumProcessed = -1;
  Lng32   sourceCompiledWithRowsets = FALSE; // True if there is a SELECT .. INTO <rowset> only
  Lng32   tempSource;
  
  char * target;
  char * realTarget;   // Added to show real start position of target, 
  // varchars adjust this position
  short  targetType;
  Lng32   targetLen;
  Lng32   targetRowsetSize;  // Greater than zero if there is an output rowset in the descriptor,
                            // this can happen in a SELECT .. INTO <rowset> and FETCH INTO <rowset>
  Lng32   targetScale;
  Lng32   targetPrecision;
  char * targetVarPtr = NULL;
  char * targetIndPtr = NULL;
  char * targetVCLen  = NULL;
  short  targetVCLenSize = 0;
  
  // Size of an entry of the indicator array
  Lng32   indSize;
  
  Long   tempTarget   = 0;
  
  char * dataPtr = 0;
  short entry = 0;
  
  if (!useParamIdx &&
    (getNumEntries() != output_desc->getUsedEntryCount()) &&
      (!output_desc->thereIsACompoundStatement())) 
    {
      ExRaiseSqlError(heap, &diagsArea, CLI_STMT_DESC_COUNT_MISMATCH);
      if (diagsArea != atp->getDiagsArea())
	atp->setDiagsArea(diagsArea);
      return ex_expr::EXPR_ERROR;
    }
  
  // If bulk move has not been disabled before, then check to see if bulk
  // move could be done. This is done by comparing attributes of the 
  // executor items and the descriptor entries. See method
  // Descriptor::checkBulkMoveStatus() for details on when bulk move will
  // be disabled.
  // If bulk move is possible then set up bulk move info, if not already
  // done.
  if (NOT output_desc->bulkMoveDisabled())
    {
      if (NOT output_desc->bulkMoveSetup())
	{
	  if (output_desc->rowwiseRowset())
	    output_desc->setRowwiseRowsetDisabled(FALSE);

#ifdef _DEBUG
	  if (getenv("DISABLE_ROWWISE_ROWSET"))
	    output_desc->setRowwiseRowsetDisabled(TRUE);
#endif

	  setupBulkMoveInfo(output_desc, heap, FALSE /* output desc*/, flags);
	  if (
	      // rowwise rowsets V1 are only supported if bulk move
	      // is being done for all items.
              (((output_desc->rowwiseRowset()) &&
	       ((output_desc->bulkMoveDisabled()) ||
	        (output_desc->doSlowMove()))
		) 
              && (output_desc->rowwiseRowsetV1() == TRUE)
	      )
            ||
	    (   (output_desc->rowwiseRowset())
              &&
		((output_desc->bulkMoveDisabled()))
              &&
                (output_desc->rowwiseRowsetV2() == TRUE)
	     )
	    )
	    {
	      output_desc->setRowwiseRowsetDisabled(TRUE);

	      if (NOT output_desc->bulkMoveDisabled()) // do slow move
		{
		  // now set up bulk move as a non-rowwise rowset move
		  setupBulkMoveInfo(output_desc, heap, FALSE /* output desc*/,
				    flags);
		}
	      else
		{
		  output_desc->deallocBulkMoveInfo();
		  output_desc->setBulkMoveDisabled(TRUE);
		}
	    } // rowwise rowset
	}
      


      if (NOT output_desc->bulkMoveDisabled())
	{
	  // bulk move is enabled and setup has been done.
	  // Do bulk move now.
	  doBulkMove(atp, output_desc, NULL, FALSE /* output desc */);

	  if (NOT output_desc->doSlowMove())
	    return ex_expr::EXPR_OK;
	}
    }
  
  // Note that every output operands must participate in rowsets, or none of
  // them. That is, you cannot mix rowset host variables and simple host
  // variables for output purposes (INTO clause).
  Int32 totalNumOfRowsets = 0;

  // Number of warnings found so far. This variable keeps track of the total number of 
  // warnings reported so far and is used to set up the indicator if 
  // we find a string overflow warning reported by the latest condDoit() call 
  // for the output.
  Lng32 numOfWarningsFoundSoFar = 0;
  
  while (clause) {
    if (clause->getType() == ex_clause::INOUT_TYPE) {
      entry++;

      if(useParamIdx){
        entry = ((ex_inout_clause *)clause)->getParamIdx();

        // avoid going off the end of the output descriptor (this can only
        // happen for wide descriptors, as for non-wide descriptors we would
        // have checked the descriptor size earlier; mxosrvr uses wide 
        // descriptors all the time though so we have to check here)
        if (entry /* 1-based */ > output_desc->getUsedEntryCount()) {
          ExRaiseSqlError(heap, &diagsArea, CLI_STMT_EXCEEDS_DESC_COUNT);
          if (diagsArea != atp->getDiagsArea())
            atp->setDiagsArea(diagsArea);
          return ex_expr::EXPR_ERROR;
        }
      }

      if ((NOT output_desc->bulkMoveDisabled()) &&
	  (NOT output_desc->doSlowMove(entry)))
	goto next_clause;

      operand = clause->getOperand(0);
 
      sourceCompiledWithRowsets = (operand->getRowsetSize() > 0);
      
      if (sourceCompiledWithRowsets) {
	totalNumOfRowsets++;
      }
      
      switch (operand->getAtpIndex()) {
      case 0:
        // constant
        source          = getConstantsArea() + operand->getOffset();
        sourceNullInd   = getConstantsArea() + operand->getNullIndOffset();
        sourceVCLenInd  = getConstantsArea() + operand->getVCLenIndOffset();
        break;
	
      case 1:
        // temp
        source          = getTempsArea() + operand->getOffset();
        sourceNullInd   = getTempsArea() + operand->getNullIndOffset();
        sourceVCLenInd  = getTempsArea() + operand->getVCLenIndOffset();

	if (operand->getNullFlag() &&
            ExpTupleDesc::isNullValue( sourceNullInd,
                                       operand->getNullBitIndex(),
                                       operand->getTupleFormat() ))
        {
          sourceNullInd = 0; // it is a null value
	}
	break;
	
      default:
        dataPtr = (atp->getTupp(operand->getAtpIndex())).getDataPointer();
        if (operand->getOffset() < 0) {
          // Offset is negative. This indicates that this offset
          // is the negative of field number in a base table
          // and this field follows one or more varchar fields.
          // True for SQL/MP tables only. Needs special logic
          // to compute the actual address of this field.
          computeDataPtr(dataPtr, 
                         - ((Lng32) operand->getOffset()),
                         atp->getCriDesc()->getTupleDescriptor(operand->getAtpIndex()),
                         &(source),
                         &(sourceNullInd),
                         &(sourceVCLenInd));
        }
        else {
          if (dataPtr) {
            source = dataPtr + operand->getOffset();
	    
            if (operand->getNullFlag()) {
              sourceNullInd = dataPtr + operand->getNullIndOffset();
	      
              if (ExpTupleDesc::isNullValue( sourceNullInd,
                                             operand->getNullBitIndex(),
                                             operand->getTupleFormat() ))
              {
		sourceNullInd = 0; // it is a null value
              }
            }
            if (operand->getVCIndicatorLength())
              sourceVCLenInd = dataPtr + operand->getVCLenIndOffset();
          }
          else
            {
              sourceNullInd  = 0;
              sourceVCLenInd = 0;
            }
        }
      } // switch atpIndex
      
      // For rowsets, here we extract the number of elements in it. This
      // only happens for the SELECT .. INTO clause
      if (source && sourceCompiledWithRowsets) {
        if (::convDoIt(source,
                       sizeof(Lng32),
                       REC_BIN32_SIGNED,
                       0,
                       0,
                       (char *)&tempSource,
                       sizeof(Lng32),
                       REC_BIN32_SIGNED,
                       0,
                       0,
                       0,
                       0,
                       heap,
                       &diagsArea) != ex_expr::EXPR_OK)
	  {
	    if (diagsArea != atp->getDiagsArea())
	      atp->setDiagsArea(diagsArea);	    
	    
	    return ex_expr::EXPR_ERROR;
	  }
	
        // Increment the location for the next element.
        source += sizeof(Lng32);
	
	// The offsets for rowset SQLVarChar attributes are set as follows. 
	// offset_ points to the start of the rowset info followed by four bytes 
	// of the rowset size. nullIndOffset_ (if nullIndicatorLength_ is not set
	// to 0) points to (offset_+4). vcLenIndOffset_ (if vcIndicatorLength_ 
	// is not set to 0) points to (offset_+nullIndicatorLength_). The first 
	// data value stars at (offset_+nullIndicatorLength_+vcIndicatorLength_)
	// or (offset_+vcIndicatorLength_) depending on whether nullIndicatorLength_
	// is valid.
	// Note vcIndicatorLength_ is set to sizeof(short) for rowset SQLVarChars.
	sourceNullInd = source;
        if (operand->getNullFlag()) {
	  source += operand->getNullIndicatorLength();
        }
	
        if (operand->getVCIndicatorLength() > 0) { 
	  source += operand->getVCIndicatorLength(); 
        }
	//Now source points to the start of the first data value of the rowset.
	
        if (tempSource < 0) {
          tempSource = 1;
	}
	
	// We have determined the number of elements in the rowset
        sourceNumProcessed = tempSource;
	
      } else {
        // No rowset currently being processed, i.e. only one value to process
	sourceNumProcessed = 1;
      }
      
      
      // Get the size of an entry of the indicator array
      output_desc->getDescItem(entry, SQLDESC_ROWSET_IND_LAYOUT_SIZE, &indSize,
                               0, 0, 0, 0);
      
      output_desc->getDescItem(entry, SQLDESC_TYPE_FS, &tempTarget, 0, 0, 0, 0);
      targetType = (short)tempTarget;
      
      output_desc->getDescItem(entry, SQLDESC_ROWSET_SIZE, &targetRowsetSize, 0, 0, 0, 0);

      targetLen = 0;

      
      output_desc->getDescItem(entry, SQLDESC_OCTET_LENGTH, &targetLen, 0, 0, 0, 0);
      
      if ((targetType >= REC_MIN_INTERVAL) &&
          (targetType <= REC_MAX_INTERVAL)) {
        output_desc->getDescItem(entry, SQLDESC_INT_LEAD_PREC,
                                 &targetPrecision, 0, 0, 0, 0);

	// SQLDESC_PRECISION gets the fractional precision.
        output_desc->getDescItem(entry, SQLDESC_PRECISION,
                                 &targetScale, 0, 0, 0, 0);
      }
      else if (targetType == REC_DATETIME) {
        output_desc->getDescItem(entry, SQLDESC_DATETIME_CODE,
                                 &targetPrecision, 0, 0, 0, 0);
        output_desc->getDescItem(entry, SQLDESC_PRECISION,
                                 &targetScale, 0, 0, 0, 0);
      } else {
        output_desc->getDescItem(entry, SQLDESC_PRECISION,
                                 &targetPrecision, 0, 0, 0, 0);
        output_desc->getDescItem(entry, SQLDESC_SCALE,
                                 &targetScale, 0, 0, 0, 0);
      }

      CharInfo::CharSet targetCharSet = CharInfo::UnknownCharSet;
      NABoolean treatAnsivByteAsWideChar = FALSE;
      if ((targetType >= REC_MIN_CHARACTER ) &&
          (targetType <= REC_MAX_CHARACTER)) {
	 // charset can be retrieved as a long INTERNALLY
	 // using programatic interface by calling getDescItem(),
	 // and can only be retrieved as a character string EXTERNALLY
	 // using "get descriptor" syntax.

         Lng32 temp_char_set;
         output_desc->getDescItem(entry, SQLDESC_CHAR_SET,
                                 &temp_char_set, 0, 0, 0, 0);
         targetCharSet = (CharInfo::CharSet)temp_char_set;

         if ( targetType == REC_BYTE_V_ANSI &&
              CharInfo::is_NCHAR_MP(targetCharSet))
            treatAnsivByteAsWideChar = TRUE;

         // store charset in scale for convDoIt call
         targetScale = targetCharSet;
      }
      
      // Now convert each element
      for (Lng32 RowNum = 0; RowNum < sourceNumProcessed; RowNum++) {

	// save current source and target datatype attributes since
	// they may get changed later in this method.
	// Restore them for the next iteration of this for loop.
	// Do this only for rowsets, for non-rowsets this loop is executed
	// only once.
	Lng32   savedSourceLen;
	short  savedTargetType = 0;
	Lng32   savedTargetPrecision = 0;
	Lng32   savedTargetScale = 0;
	Lng32   savedTargetLen = 0;
	if (targetRowsetSize > 0)
	  {
	    savedSourceLen = sourceLen;

	    savedTargetType = targetType;
	    savedTargetPrecision = targetPrecision;
	    savedTargetScale = targetScale;
	    savedTargetLen = targetLen;
	  }

	char * rowsetSourcePosition = source;
        if (sourceCompiledWithRowsets)
          output_desc->setDescItem(0, SQLDESC_ROWSET_HANDLE, RowNum, 0);
        output_desc->getDescItem(entry, SQLDESC_IND_PTR, &tempTarget,
                                 0, 0, 0, 0);

	// Need to decide if we are in rowwise rowsets and adjust the temptarget appropriately
        if (tempTarget && output_desc->rowwiseRowset() && (NOT output_desc->rowwiseRowsetDisabled()))
	  tempTarget = tempTarget + output_desc->getCurrRowOffsetInRowwiseRowset();
         
        targetIndPtr = (char *)tempTarget;
	
        if ((operand->getVCIndicatorLength()) && (sourceVCLenInd)) {
          if (operand->getVCIndicatorLength() == sizeof(short))
            sourceLen = *(short*)sourceVCLenInd;
          else
            sourceLen = *(Lng32 *)sourceVCLenInd;
        }
        else
          sourceLen = operand->getLength();
	
        output_desc->getDescItem(entry, SQLDESC_VAR_PTR,
                                 &tempTarget, 0, 0, 0, 0);

	// Need to decide if we are in rowwise rowsets and adjust the temptarget appropriately
        if (tempTarget && output_desc->rowwiseRowset() && (NOT output_desc->rowwiseRowsetDisabled()))
	  tempTarget = tempTarget + output_desc->getCurrRowOffsetInRowwiseRowset();
	
        NABoolean nullMoved = FALSE;
	
        if (!tempTarget)
        {
          if (operand->getNullFlag() &&
              ((! sourceNullInd) ||
               ExpTupleDesc::isNullValue( sourceNullInd,
                                          operand->getNullBitIndex(),
                                          operand->getTupleFormat() )))
	    {
	      output_desc->setDescItem(entry, SQLDESC_IND_DATA, -1, NULL);
	      nullMoved = TRUE;
	    }
	  else
	    output_desc->setDescItem(entry, SQLDESC_IND_DATA, 0, NULL);
	}
	else {
	  if (targetIndPtr) { // null indicator specified in target
	    target = targetIndPtr;
	    realTarget = target;
            
	    if (operand->getNullFlag() &&
                ((! sourceNullInd) ||
                 ExpTupleDesc::isNullValue( sourceNullInd,
                                            operand->getNullBitIndex(),
                                            operand->getTupleFormat() )))
	      {
		// sourceNullInd is missing. This indicates a null value.
		// Move null to the indicator.
		if (targetRowsetSize > 0) {
		  for (Int32 i = 0; i < indSize; i++) {
		    targetIndPtr[i] = '\377';
		  }
		}
		else {
		  targetIndPtr[0] = '\377';
		  targetIndPtr[1] = '\377';
		}
		nullMoved = TRUE;
	      }
	    else {
	      // move 'no null' to target indicator
	      if (targetRowsetSize > 0) {
		for (Int32 i = 0; i < indSize; i++) {
		  targetIndPtr[i] = 0;
		}
	      }
	      else {
		targetIndPtr[0] = 0;
		targetIndPtr[1] = 0;
	      }
	    }
	  }
	  else {
	    // no indicator specified.
	    // Return error if a null value.
	    if (operand->getNullFlag() && (!sourceNullInd))
	      {
		ExRaiseSqlError(heap, &diagsArea,
				EXE_MISSING_INDICATOR_VARIABLE);
		if (diagsArea != atp->getDiagsArea())
		  atp->setDiagsArea(diagsArea);
		return ex_expr::EXPR_ERROR;
	      }
	  }
	}
	
	if (! nullMoved) {
	  targetVarPtr = (char *)tempTarget;
	  
	  if (targetVarPtr) {
	    // bound column. Move value to user area.
	    target     = targetVarPtr;
	    realTarget = target;
	  }
	  else {
	    // unbound column, move into temp buffer to add VC length
	    target = new (heap) char[targetLen + 10]; // 10 extra bytes should do
	    realTarget = target;
	  }
	  
	  Lng32  tempDataConversionErrorFlag = ex_conv_clause::CONV_RESULT_OK;
	  
	  if ((DFS2REC::isSQLVarChar(targetType)) ||
	      (DFS2REC::isLOB(targetType))) {
	    targetVCLen = target;
	    if (targetVarPtr) {
#ifdef _DEBUG
	      assert(SQL_VARCHAR_HDR_SIZE == sizeof(short));
#endif
	      // user host vars have VC len of 4 if the targetLen is
	      // more than 32768. Else, VC len is 2.
	      if (targetLen & 0xFFFF8000)
	        targetVCLenSize = sizeof(Lng32);
	      else
	        targetVCLenSize = sizeof(short);
	    }
	    else
	      targetVCLenSize = sizeof(Lng32);   // desc entries have VC len 4
	    target = &target[targetVCLenSize];
	  }
	  else
	    targetVCLenSize = 0;

	  ex_expr::exp_return_type rc = ex_expr::EXPR_OK;
	  NABoolean implicitConversion = FALSE;

	  short sourceType = operand->getDatatype();
          CharInfo::CharSet sourceCharSet = operand->getCharSet();

	  // if the source and target are not compatible, then do implicit
	  // conversion if skipTypeCheck is set.
	  // Furthermore, if restrictedSkipTypeCheck is set, then only
	  // convert for the conversions that are externally supported.
	  // See usage of default IMPLICIT_HOSTVAR_CONVERSION in generator
	  // (GenExpGenerator.cpp) for restricted conversions.
	  if (NOT areCompatible(sourceType, targetType))
	    {
	      if ((NOT skipTypeCheck()) ||
		  ((restrictedSkipTypeCheck()) && 
		   (NOT implicitConversionSupported(sourceType,targetType))))
		{
		  ExRaiseSqlError(heap, &diagsArea,EXE_CONVERT_NOT_SUPPORTED);
		  if (diagsArea != atp->getDiagsArea())
		    atp->setDiagsArea(diagsArea);	    
		  return ex_expr::EXPR_ERROR;
		}

	      implicitConversion = TRUE;
	   } else {
              if ( DFS2REC::isAnyCharacter(sourceType) &&
                   NOT CharInfo::isAssignmentCompatible(targetCharSet, sourceCharSet)
                 ) 

              {
	        ExRaiseSqlError(heap, &diagsArea, CLI_ASSIGN_INCOMPATIBLE_CHARSET);

                if (diagsArea != atp->getDiagsArea())
	            atp->setDiagsArea(diagsArea);	    

                *diagsArea 
		<< DgString0(CharInfo::getCharSetName((CharInfo::CharSet)targetCharSet))
		<< DgString1(CharInfo::getCharSetName((CharInfo::CharSet)sourceCharSet));

	        return ex_expr::EXPR_ERROR;
              }
            }
	
	  // if the target is REC_DECIMAL_LS and the source is NOT
	  // REC_DECIMAL_LSE, convert the source to REC_DECIMAL_LSE
	  // first. This conversion already took place during compilation
	  // for rowsets
	  char * intermediate = NULL;
	  Lng32 sourceScale = operand->getScale();
	  Lng32 sourcePrecision = operand->getPrecision();

	  if ((targetType == REC_DECIMAL_LS) &&
	      (operand->getDatatype() != REC_DECIMAL_LSE) &&
  	      (targetRowsetSize == 0)) {
	    
            if ( source ) {
	      intermediate = new (heap) char[targetLen - 1];
	      if (::convDoIt(source,
			     sourceLen,
			     sourceType,
			     sourcePrecision,
			     sourceScale,
			     intermediate,
			     targetLen - 1,
			     REC_DECIMAL_LSE,
			     targetPrecision,
			     targetScale,
			     NULL,
			     0,
			     heap,
			     &diagsArea) !=  ex_expr::EXPR_OK) {
		if (diagsArea != atp->getDiagsArea())
		  atp->setDiagsArea(diagsArea);            
		
		NADELETEBASIC(intermediate, heap);
		if (! targetVarPtr)
		  NADELETEBASIC(realTarget, heap);
		return ex_expr::EXPR_ERROR;
	      };
	      
	      source = intermediate;
	      sourceLen = targetLen - 1;
	    } 
	    
	    sourceType = REC_DECIMAL_LSE;
	  }
	  
	  // 5/18/98: added to handle SJIS encoding charset case.
	  // This is used in ODBC where the target character set is still
	  // ASCII but the actual character set used is SJIS.
	  //
	  if ( (sourceType == REC_NCHAR_F_UNICODE ||
		sourceType == REC_NCHAR_V_UNICODE )
	       )
	    {

           if ( targetCharSet == CharInfo::SJIS ) 
		{
		  switch ( targetType ) {
		  case REC_BYTE_F_ASCII:
		    targetType = REC_MBYTE_F_SJIS;
		    break;
		    
		  case REC_BYTE_V_ANSI:
		  case REC_BYTE_V_ASCII_LONG:
		  case REC_BYTE_V_ASCII:
		    targetType = REC_MBYTE_V_SJIS;
		    break;
		    
		  default:
		    break;
		  }
		}
	  } 
	  //////////////////////////////////////////////////////////////////
	  // Conversion for exact numerics is done before doing scaling.
	  // The scale is implicit so a value 1234.56 is stored as 123456.
	  // While converting a value that does downscaling, we may run
	  // into overflow errors which would not be there after scaling.
	  // For example,
	  // a NUMERIC(6,2) type with value 1234.56 is stored as 123456.
	  // Converting this to a target type of SMALLINT will return an
	  // overflow error since 123456 will not fit in 2 bytes. However
	  // after scaling and truncation, this value will become 1234
	  // that will fit into SMALLINT. ANSI allows truncated results to
	  // be returned.
	  // So...if the source scale is greater than target scale for an
	  // exact numeric conversion, then first convert the source to
	  // largeint(this is the biggest exact numeric type that we
	  // support) and downscale it there. Then convert this scaled 
	  // largeint to the target type.
	  //////////////////////////////////////////////////////////////////
	  if ((sourceScale > targetScale) &&
	      (isExactNumeric(sourceType)) &&
	      (isExactNumeric(targetType)))
	    {
	      if (DFS2REC::isBigNum(sourceType))
		// max numeric precision of 128 is stored as 57 bytes.
		intermediate = new (heap) char[57];
	      else
		intermediate = new (heap) char[8];
              if ( source ) 
		{
		  // convert to largeint
		  if (convDoIt(source,
			       sourceLen,
			       sourceType,
			       sourcePrecision,
			       sourceScale,
			       intermediate,
			       (DFS2REC::isBigNum(sourceType) ? 57 : 8),
			       (DFS2REC::isBigNum(sourceType) 
				? REC_NUM_BIG_SIGNED : REC_BIN64_SIGNED),
			       targetPrecision,
			       targetScale,
			       NULL,
			       0,
			       heap,
			       &diagsArea) !=  ex_expr::EXPR_OK) 
		    {
		      if (diagsArea != atp->getDiagsArea())
			atp->setDiagsArea(diagsArea);            
		      
		      NADELETEBASIC(intermediate, heap);
		      return ex_expr::EXPR_ERROR;
		    }
		  
		  // scale it
		  if (scaleDoIt(
		       intermediate,
		       (DFS2REC::isBigNum(sourceType) ? 57 : 8),
		       (DFS2REC::isBigNum(sourceType) 
			? REC_NUM_BIG_SIGNED : REC_BIN64_SIGNED),
		       sourceScale,
		       targetScale,
                       sourceType,
		       heap) != ex_expr::EXPR_OK) 
		    {
		      ExRaiseSqlError(heap, &diagsArea, EXE_NUMERIC_OVERFLOW);
		      if (diagsArea != atp->getDiagsArea())
			atp->setDiagsArea(diagsArea);            
		      
		      NADELETEBASIC(intermediate, heap);
		      return ex_expr::EXPR_ERROR;
		    }
		}
	      
	      // if the target is REC_DECIMAL_LS, convert the source to 
	      // REC_DECIMAL_LSE first. Conversion to LS is currently
	      // only supported if source is LSE.
	      if (targetType == REC_DECIMAL_LS)
		{
                  if ( source ) 
		    {
		      char * intermediate2 = new (heap) char[targetLen - 1];
		      if (::convDoIt(intermediate,
				     8,
				     REC_BIN64_SIGNED,
				     targetPrecision,
				     targetScale,
				     intermediate2,
				     targetLen - 1,
				     REC_DECIMAL_LSE,
				     targetPrecision,
				     targetScale,
				     NULL,
				     0,
				     heap,
				     &diagsArea) !=  ex_expr::EXPR_OK) {
			if (diagsArea != atp->getDiagsArea())
			  atp->setDiagsArea(diagsArea);            
			
			NADELETEBASIC(intermediate, heap);
			NADELETEBASIC(intermediate2, heap);
			return ex_expr::EXPR_ERROR;
		      };
		      
		      NADELETEBASIC(intermediate, heap);
		      intermediate = intermediate2;
		      sourceLen = targetLen - 1;
		    }
		  
		  sourceType = REC_DECIMAL_LSE;
		} // targetType == REC_DECIMAL_LS
	      else
		{
		  sourceLen = (DFS2REC::isBigNum(sourceType) ? 57 : 8);
		  sourceType = (DFS2REC::isBigNum(sourceType) 
				? REC_NUM_BIG_SIGNED : REC_BIN64_SIGNED);
		}
	      
              if ( source )
		source = intermediate;
	      
	      // no more scaling is to be done.
	      sourceScale = 0;
	      targetScale = 0;
	    } // exact numeric with scaling
	  
	  // if incompatible conversions are to be done,
	  // and the target is  REC_BYTE_V_ANSI, 
	  // and the source is not a string type, convert the source to 
	  // REC_BYTE_V_ASCII first. Conversion to V_ANSI is currently
	  // only supported if source is a string type.
	  if (implicitConversion)
	    {
	      if ((targetType == REC_BYTE_V_ANSI) &&
		  ((sourceType < REC_MIN_CHARACTER) ||
		   (sourceType > REC_MAX_CHARACTER)))
		{
		  if ( source ) 
		    {
		      intermediate = new (heap) char[targetLen];
		      short intermediateLen;
		      if (::convDoIt(source,
				     sourceLen,
				     sourceType,
				     sourcePrecision,
				     sourceScale,
				     intermediate,
				     targetLen,
				     REC_BYTE_V_ASCII,
				     targetPrecision,
				     targetScale,
				     (char*)&intermediateLen,
				     sizeof(short),
				     heap,
				     &diagsArea) !=  ex_expr::EXPR_OK) {
			if (diagsArea != atp->getDiagsArea())
			  atp->setDiagsArea(diagsArea);            
			
			NADELETEBASIC(intermediate, heap);
			return ex_expr::EXPR_ERROR;
		      };
		      
		      sourceType = REC_BYTE_F_ASCII;
		      sourceLen = intermediateLen;
		      sourceScale = 0;
		      source = intermediate;
		    }
		} // targetType == REC_BYTE_V_ANSI

	      if (((targetType == REC_DATETIME) ||
		   (DFS2REC::isInterval(targetType))) &&
		  (DFS2REC::isAnyCharacter(sourceType)))
		{
		  targetType = REC_BYTE_F_ASCII;
		  targetScale = 0;
		  targetPrecision = 0;
		}
	    } // implicit conversion
	  
	  ConvInstruction case_index = CONV_UNKNOWN;
	  ULng32 convFlags = 0;
	  if ((source) &&
	      (NOT implicitConversion) &&
	      (((sourceType == REC_DATETIME) ||
		((sourceType >= REC_MIN_INTERVAL) &&
		 (sourceType <= REC_MAX_INTERVAL))) &&
	       (NOT isIFIO)))
	    {
	      // this is a datetime or interval conversion.
	      // Convert the internal datetime/interval to its
	      // external string format. The conversion follows the
	      // same rules as "CAST(datetime/interval TO CHAR)".

	      // If the two interval types are not the same, then first
	      // convert source to target interval type, and then convert
	      // to its string external representation.
	      
	      if (sourceType == REC_DATETIME)
		{
		  if ((sourcePrecision != targetPrecision) ||
		      (sourceScale != targetScale))
		    {
		      // source and target must have the same datetime_code,
		      // they must both be date, time or timestamp. 
		      ExRaiseSqlError(heap, &diagsArea,
				      EXE_CONVERT_NOT_SUPPORTED);
		      if (diagsArea != atp->getDiagsArea())
			atp->setDiagsArea(diagsArea);	    
		      return ex_expr::EXPR_ERROR;
		    }
		  else
		    {
		      // For CALL stmts with wide desc, don't change target
		      // type to ASCII if rowwise rowsets are set by caller.
		      // Caller expects the value in internal format even though
		      // we do not use bulk move for copying.
                      // Also applicable to ODBC calls.
                      if ( ((NOT isOdbc) &&
                            (! isCall() ||
                             ! output_desc->isDescTypeWide() ||
                             ! output_desc->rowwiseRowsetV2())) ||
                           (isOdbc &&
                            ! output_desc->rowwiseRowsetV1() &&
                            ! output_desc->rowwiseRowsetV2()) )
                        {
                          targetType = REC_BYTE_F_ASCII;
                          targetPrecision = 0; // TBD $$$$ add target max chars later
                          targetScale = SQLCHARSETCODE_ISO88591; // assume target charset is ASCII-compatible
                        }
		    }
		}
	      else if ((sourceType == targetType) &&
		       (sourcePrecision == targetPrecision) &&
		       (sourceScale == targetScale))
		{
		  // 'interval' source and target match all attributes.
		  // Convert source(internal) to string target(external) format
		  targetType = REC_BYTE_F_ASCII;
                  targetPrecision = 0; // TBD $$$$ add target max chars later
                  targetScale = SQLCHARSETCODE_ISO88591; // assume target charset is ASCII compatible
		}
	      else 
		{
		  // interval type and source/target do not match.
		  // Convert source to target interval type.
		  short intermediateLen = SQL_LARGE_SIZE;
		  intermediate = new (heap) char[intermediateLen];
		  if (::convDoIt(source,
				 sourceLen,
				 sourceType,
				 sourcePrecision,
				 sourceScale,
				 intermediate,
				 intermediateLen,
				 targetType,
				 targetPrecision,
				 targetScale,
				 NULL,
				 sizeof(short),
				 heap,
				 &diagsArea) !=  ex_expr::EXPR_OK) 
		    {
		      if (diagsArea != atp->getDiagsArea())
			atp->setDiagsArea(diagsArea);            
		      
		      NADELETEBASIC(intermediate, heap);
		      return ex_expr::EXPR_ERROR;
		    };

		  sourceType = targetType;
		  sourceLen = intermediateLen;
		  sourceScale = targetScale;
		  sourcePrecision = targetPrecision;
		  source = intermediate;
		  
		  targetType = REC_BYTE_F_ASCII;
		  targetScale = 0;
		  targetPrecision = 0;
		}
	      
	      // datetime and interval values are left blank padded.
	      convFlags |= CONV_LEFT_PAD;
	    }

          if ( source ) 
	    {
              if ( treatAnsivByteAsWideChar == TRUE )
              {
                  // We substract one here so that the first byte in the
                  // last wide char can be filled with single-byte 
                  // null by convDoIt(). 
                  // 
                  // Note the target length records the number of octets
                  // in target hostvar. The value is even because
                  // KANJI/KSC hostvars are wide characters.
                  targetLen--;
              }
	      rc =  convDoIt (source,
	      	        sourceLen,
			        sourceType,
			        sourcePrecision,
		  	        sourceScale,
			        target,
			        targetLen,
			        targetType,
			        targetPrecision,
			        targetScale,
			        targetVCLen,
			        targetVCLenSize,
			        heap,
			        &diagsArea,
			        case_index,
			        0,
			        convFlags);
	      if (rc == ex_expr::EXPR_ERROR) {
		if (diagsArea != atp->getDiagsArea())
		  atp->setDiagsArea(diagsArea);            
		
		NADELETEBASIC(intermediate, heap);
		if (!targetVarPtr)
		  NADELETEBASIC(realTarget, heap);
		return ex_expr::EXPR_ERROR;
	      }

              if ( treatAnsivByteAsWideChar == TRUE )
              {
                  // Make the second byte in the last wide char
                  // a null so that the last wide char contains a wide-char 
                  // null.
                  target[strlen(target)+1] = 0;
              }

              if (diagsArea ) {
  
   	         // Indicator var is also used to test for truncated output value.
   	         // The Indicator variable is set to the length of the string in the 
   	         // database if the character value is truncated..

                 // numOfWarningsFoundSoFar is the number of warnings we know exists 
                 // before the above convDoIt() is called. If warning EXE_STRING_OVERFLOW 
                 // exists as a result of the convDoIt() call, it will be recorded in  
                 // diagsArea->warnings_ array, in the range 
                 // [numOfStringOverflowWarningsFoundSoFar, warnings_.entries()). The 
                 // method diagsArea->containsWarning(x, y) will tell us if it is true or 
                 // not. If it is true, then we update the indicator.
   	         if ( DFS2REC::isAnyCharacter(targetType) &&
   		   DFS2REC::isAnyCharacter(operand->getDatatype()) &&
                   diagsArea -> containsWarning( numOfWarningsFoundSoFar, 
                                                 EXE_STRING_OVERFLOW))
   		   {

   		     if (targetIndPtr)
   		       {
   		         short * tempVCInd = (short *) targetIndPtr;
   			  if (DFS2REC::isANSIVarChar(sourceType)) {
			    if ( CharInfo::maxBytesPerChar(sourceCharSet) == 2 ) {
				*tempVCInd = (short) NAWstrlen((NAWchar *)source);  
				// we are guaranteed that source is
				// null-terminated as the previous call to convdoit() 
				//would have raised an error otherwise
			    }  
			    else {
				*tempVCInd = (short) strlen(source);
			    }
			}  // if ANSIVarChar
			else {
			  *tempVCInd = (short)sourceLen;

			  if ( CharInfo::maxBytesPerChar(sourceCharSet) == 2 )
			    *tempVCInd /= SQL_DBCHAR_SIZE;
			}
		      }
   		  
   		       if (diagsArea != atp->getDiagsArea())
   			 atp->setDiagsArea(diagsArea);
   		     } // character datatype

                 numOfWarningsFoundSoFar = diagsArea->getNumber(DgSqlCode::WARNING_);
               }
	      
	      NADELETEBASIC(intermediate, heap);
	      
	      // up- or down-scale target.
	      if (targetType >= REC_MIN_NUMERIC && 
		  targetType <= REC_MAX_NUMERIC &&
		  sourceScale != targetScale &&
		  ::scaleDoIt(
		       target,
		       targetLen,
		       targetType,
		       sourceScale,
		       targetScale,
                       sourceType,
		       heap) != ex_expr::EXPR_OK) 
		{
		  ExRaiseSqlError(heap, &diagsArea, EXE_NUMERIC_OVERFLOW);
		  if (diagsArea != atp->getDiagsArea())
		    atp->setDiagsArea(diagsArea);            
		  
		  if (!targetVarPtr)
		    NADELETEBASIC(realTarget, heap);
		  return ex_expr::EXPR_ERROR;
		}
	      
	      if (!targetVarPtr) {
		// unbound column. Remember it in executor memory.
		// This value will(probably) be retrieved later by
		// a call to GetDescItem with SQLDESC_VAR_DATA.
		Lng32 length;
		if (targetVCLenSize > 0)
		  length = 0;
		else
		  length = targetLen;
		output_desc->setDescItem(entry, SQLDESC_VAR_DATA,
					 length , realTarget);
		//sourceLen+targetVCLenSize, realTarget); 
		NADELETEBASIC(realTarget, heap);
	      }
	    }
	} // not NULL moved
	
       if ( source != NULL ) {

	 // Increment the location for the next element.
 	 if (targetRowsetSize > 0) {
	   source = rowsetSourcePosition;
	 }
	 source += operand->getStorageLength();

	 if ((operand->getVCIndicatorLength() > 0) && (sourceVCLenInd))
	   sourceVCLenInd += operand->getStorageLength();
	 if ((operand->getNullFlag()) && (sourceNullInd))
	   sourceNullInd  += operand->getStorageLength();
       }  

       if (targetRowsetSize > 0)
	 {
	   // restore the original datatype attributes.
	   sourceLen = savedSourceLen;

	   targetType = savedTargetType;
	   targetPrecision = savedTargetPrecision;
	   targetScale = savedTargetScale;
	   targetLen = savedTargetLen;
	 }

      } // end of for (RowNum < sourceNumProcessed) loop
    } // clause is INOUT type

next_clause:
    clause = clause->getNextClause();
  } // loop over clauses
  
  if (totalNumOfRowsets)
    output_desc->setDescItem(0, SQLDESC_ROWSET_NUM_PROCESSED,
			     sourceNumProcessed, 0);
  
  return ex_expr::EXPR_OK;
}


ex_expr::exp_return_type InputOutputExpr::describeInput(void * input_desc_,
							void * rwrs_info,
							UInt32 flags)
{
  NABoolean isOdbc = isODBC(flags);
  NABoolean isDbtr = isDBTR(flags);
  NABoolean isRwrs = isRWRS(flags);
  NABoolean isIFIO  = (isInternalFormatIO(flags) ||
		       (isDbtr && isRwrs));

  RWRSInfo   * rwrsInfo  = (RWRSInfo *)rwrs_info;

  ex_clause *clause = getClauses();
  Descriptor * input_desc = (Descriptor *)input_desc_;
  
  short entry = 0;
  input_desc->setDescItem(0, SQLDESC_ROWSET_SIZE, // ROWSET_SIZE is a header field and 
				    0, 0);        // occurs once per descriptor, not once per
 // desc item. Its value is 0 if no rowsets are present, otherwise its value is 
 // rowset size of any one of the all rowset input variables in the descriptor. 
 // For dynamic SQl, where describe is used, rowsets sizes must all be the same
  // so the MIN of all input sizes is not calculated.
  short dataType; 

  Lng32 prevEntryStartOffset = 0;
  Lng32 currAlignedOffset = 0;
  Lng32 firstEntryStartAlignment = 0;

  Lng32 length = -1;

  NABoolean firstEntryProcessed = FALSE;

  while (clause)
    {

      if (clause->getType() == ex_clause::INOUT_TYPE)
        {
          Attributes * operand = clause->getOperand(0);
	  dataType  = operand->getDatatype();

	  if(isCall() && input_desc->isDescTypeWide())
	    entry = ((ex_inout_clause *)clause)->getParamIdx();
	  else
	    entry++;

          input_desc->setDescItem(entry, SQLDESC_TYPE_FS, dataType, 0);
	  length = operand->getLength();

          // Record the display length for DATETIME or INTERVAL operand since either is
          // represented as a character array externally. The value of this variable is
          // used to set the SQLDESC_LENGTH field for the descriptor.  The same value 
          // will be used to set the SQLDESC_ROWSET_VAR_LAYOUT_SIZE field if the input
          // is a rowset and its element type is DATETIME or INTERVAL.
          Lng32 displayLength = 0; 
	
          if ((dataType >= REC_MIN_INTERVAL) &&
              (dataType <= REC_MAX_INTERVAL)) {
            //
            // According to ANSI, interval info is stored in the following
            // descriptor items:
            //
            // DATETIME_INTERVAL_CODE = qualifier code
            // PRECISION = fractional precision
            // DATETIME_INTERVAL_PRECISION = leading precision
            //
            input_desc->setDescItem(entry,
                                    SQLDESC_DATETIME_CODE,
                                    getIntervalCode(dataType),
                                    0);
            input_desc->setDescItem(entry,
                                    SQLDESC_PRECISION,
                                    operand->getScale(),
                                    0);
            input_desc->setDescItem(entry, SQLDESC_SCALE, 0, 0);
            input_desc->setDescItem(entry,
                                    SQLDESC_INT_LEAD_PREC,
                                    operand->getPrecision(),
                                    0);
	    if (NOT isIFIO)
	      {
		displayLength =
		  ExpInterval::getDisplaySize(dataType,
					      operand->getPrecision(),
					      operand->getScale());
		
		length = displayLength;
	      }
          } else if (dataType == REC_DATETIME) {
            //
            // According to ANSI, datetime info is stored in the following
            // descriptor items:
            //
            // DATETIME_INTERVAL_CODE = qualifier code
            // PRECISION = fractional precision
            //
            input_desc->setDescItem(entry,
                                    SQLDESC_DATETIME_CODE,
                                    operand->getPrecision(),
                                    0);
            input_desc->setDescItem(entry,
                                    SQLDESC_PRECISION,
                                    operand->getScale(),
                                    0);
            input_desc->setDescItem(entry, SQLDESC_SCALE, 0, 0);
            input_desc->setDescItem(entry, SQLDESC_INT_LEAD_PREC, 0, 0);

	    if (NOT isIFIO)
	      {
		displayLength =
		  ExpDatetime::getDisplaySize(operand->getPrecision(),
					      operand->getScale());
		length = displayLength;
	      }

          } else {
            input_desc->setDescItem(entry, SQLDESC_DATETIME_CODE, 0, 0);
            input_desc->setDescItem(entry,
                                    SQLDESC_PRECISION,
                                    operand->getPrecision(),
                                    0);
            input_desc->setDescItem(entry,
                                    SQLDESC_SCALE,
                                    operand->getScale(),
                                    0);
            input_desc->setDescItem(entry, SQLDESC_INT_LEAD_PREC, 0, 0);
          }
	
          // Use SQLDESC_CHAR_SET_NAM (one-part name) for charset
	  if (((dataType >= REC_MIN_CHARACTER) &&
               (dataType <= REC_MAX_CHARACTER)) )
            {
	    input_desc->setDescItem(entry, SQLDESC_CHAR_SET_NAM, 0, 
	      (char*)CharInfo::getCharSetName(operand->getCharSet()));

            // For Unicode/KSC5601/KANJI, set the length in number of characters 
            if ( CharInfo::maxBytesPerChar(operand->getCharSet()) == SQL_DBCHAR_SIZE )
	      length = operand->getLength()/SQL_DBCHAR_SIZE;
	  }

          input_desc->setDescItem(entry, SQLDESC_NULLABLE, 
                                  operand->getNullFlag(), 0);
          input_desc->setDescItem(entry, SQLDESC_NAME, 0, 
                                  ((ex_inout_clause *)clause)->getName());

          if (((ex_inout_clause *)clause)->getHeading() != 0)
            input_desc->setDescItem(entry, SQLDESC_HEADING, 0, 
                                    ((ex_inout_clause *)clause)->getHeading());

	  if (((ex_inout_clause *)clause)->getTableName() != 0)
            input_desc->setDescItem(entry, SQLDESC_TABLE_NAME, 0, 
                                    ((ex_inout_clause *)clause)->getTableName());

	  if (((ex_inout_clause *)clause)->getSchemaName() != 0)
            input_desc->setDescItem(entry, SQLDESC_SCHEMA_NAME, 0, 
                                    ((ex_inout_clause *)clause)->getSchemaName());
	  if (((ex_inout_clause *)clause)->getCatalogName() != 0)
            input_desc->setDescItem(entry, SQLDESC_CATALOG_NAME, 0, 
                                    ((ex_inout_clause *)clause)->getCatalogName());

          input_desc->setDescItem(entry, SQLDESC_VAR_PTR, 0, 0);
          input_desc->setDescItem(entry, SQLDESC_IND_PTR, 0, 0);
          input_desc->setDescItem(entry, SQLDESC_VAR_DATA, 0, 0);
          input_desc->setDescItem(entry, SQLDESC_IND_DATA, 0, 0);
	  
          if (operand->getRowsetSize() > 0) {
	      input_desc->setDescItem(0, SQLDESC_ROWSET_SIZE, 
                                      operand->getRowsetSize(), 0);

            // Use the operand length if the operand is not INTERVAL/DATETIME.
            if ( displayLength == 0 ) 
               displayLength = operand->getLength();
	    // Describe Input will set values for VarLayoutSize that are compatible
	    // with C type (i.e null-terminated) hostvariables. For fixed length chars
	    // ANSI varchars, decimal, datetime and interval types
	    // varLayoutSize will be set to OCTET_LENGTH + 1 (+2 for unicode). 
	    // Users are expected to override this value, if their arraysize is different.

	    if (DFS2REC::isAnyCharacter(dataType) && !DFS2REC::isSQLVarChar(dataType)) {
	      // all charcter types other than tandem varchar
	      displayLength += CharInfo::maxBytesPerChar(operand->getCharSet());
	    }
	    else if ((dataType >= REC_MIN_DECIMAL && dataType <= REC_MAX_DECIMAL) ||
		     (dataType >= REC_MIN_INTERVAL && dataType <= REC_MAX_INTERVAL_MP) ||
		     (dataType == REC_DATETIME)) {
	      // We do not expect to see dataType == REC_DECIMAL_LS (with first byte for sign)
	      // as the compiler currently generates REC_DECIMAL_LSE (leading sign embedded) as
	      // the type for signed decimal columns. If the compiler ever changes is default type 
	      // for decimal signed columns to REC_DECIMAL_LS, then the following must be true
	      // RowsetVarLayoutSize = Precison + 2;
	      // If Length = Precision + 1; then the following assertion can be removed and 
	      // the behaviour will be correct. If Length = Precision, then when removing
	      // the following assertion add one to displayLength for the REC_DECIMAL_LS datatype.
	      BriefAssertion(dataType != REC_DECIMAL_LS, "RowsetVarLayoutSize must account for the sign byte");
	      displayLength++;

	    }
	    
            input_desc->setDescItem(entry, SQLDESC_ROWSET_VAR_LAYOUT_SIZE,
                                    displayLength, 0);

            input_desc->setDescItem(entry, SQLDESC_ROWSET_IND_LAYOUT_SIZE,
                                    operand->getNullIndicatorLength(), 0);
          }
	  else {
            input_desc->setDescItem(entry, SQLDESC_ROWSET_VAR_LAYOUT_SIZE,
				    0, 0);
            input_desc->setDescItem(entry, SQLDESC_ROWSET_IND_LAYOUT_SIZE,
				    0, 0);
	    
	  }

	  short paramMode = ((ex_inout_clause *)clause)->getParamMode();
	  short paramIdx = ((ex_inout_clause *)clause)->getParamIdx();
	  short ordPos = ((ex_inout_clause *)clause)->getOrdPos();

	  BriefAssertion((paramMode == PARAMETER_MODE_IN)||
			 (paramMode == PARAMETER_MODE_INOUT)||
			 (paramMode == PARAMETER_MODE_UNDEFINED),
			 "invalid param mode");
	  BriefAssertion(paramIdx >= 0, "invalid param index");
	  BriefAssertion(ordPos >= 0, "invalid param ordinal position");

	  // handle old plan
	  if(paramMode == PARAMETER_MODE_UNDEFINED){
	    paramMode = PARAMETER_MODE_IN;
	  }
	  if(paramIdx == 0){ 
	    paramIdx = entry;
	  }
	  if(ordPos == 0){
	    ordPos = entry;
	  }
	  input_desc->setDescItem(entry, SQLDESC_PARAMETER_MODE, paramMode, 0);
	  input_desc->setDescItem(entry, SQLDESC_PARAMETER_INDEX, paramIdx, 0);
	  input_desc->setDescItem(entry, SQLDESC_ORDINAL_POSITION, ordPos, 0);

	  // End


	  if(isCall()){
	    short paramMode = ((ex_inout_clause *)clause)->getParamMode();
	    short paramIdx = ((ex_inout_clause *)clause)->getParamIdx();
	    short ordPos = ((ex_inout_clause *)clause)->getOrdPos();
	    input_desc->setDescItem(entry, SQLDESC_PARAMETER_MODE,
				    paramMode, 0);
	    input_desc->setDescItem(entry, SQLDESC_PARAMETER_INDEX,
				    paramIdx, 0);
	    input_desc->setDescItem(entry, SQLDESC_ORDINAL_POSITION,
				    ordPos, 0);
	  }
	  else{
	    input_desc->setDescItem(entry, SQLDESC_PARAMETER_MODE,
				    PARAMETER_MODE_IN, 0);
	    input_desc->setDescItem(entry, SQLDESC_PARAMETER_INDEX,
				    entry, 0);
	    input_desc->setDescItem(entry, SQLDESC_ORDINAL_POSITION,
				    0, 0);
	  }

	  input_desc->setDescItem(entry, SQLDESC_LENGTH, length, 0);

	  // Set up length of null indicator and varcharlen indicator values
	  // in the descriptor.
	  //
          input_desc->setDescItem(entry, SQLDESC_IND_LENGTH,
				  operand->getNullIndicatorLength(), 0);
          input_desc->setDescItem(entry, SQLDESC_VC_IND_LENGTH,
				  operand->getVCIndicatorLength(), 0);
	  
	  // Get offsets to data, null indicator and varchar len indicator
	  // in the actual row and set them in the descriptor.
	  //
	  // This is being done for sqlark_exploded_ and packed formats only.
	  // The offsets that are set in descriptor assume that the returned
	  // row to user is a single contiguous aligned row even though internally
	  // that row could be represented by multiple fragments, each with
	  // different atp index.
	  // 
	  // These values are used by caller(mxcs) to set up input and output
	  // data pointers so that they are aligned and set up the same way
	  // as the actual row in cli.
	  //
	  // These values cannot be set by external callers.
	  //
	  // If RWRS buffer is being input, then do this for data params only.
	  // 
	  if ((rwrsInfo ) &&
	      ((entry == rwrsInfo->rwrsInputSizeIndex()) ||
	       (entry == rwrsInfo->rwrsMaxInputRowlenIndex()) ||
	       (entry == rwrsInfo->rwrsBufferAddrIndex()) ||
	       (entry == rwrsInfo->rwrsPartnNumIndex())))
	    {
	      // control params, skip them.
	      goto next_clause;
	    }

	  Lng32 dataOffset = -1;
	  Lng32 nullIndOffset = -1;
	  Lng32 currEntryStartOffset = -1;
	  Lng32 currEntryStartAlignment = -1;

	  Lng32 inputDatalen = -1;
	  if (input_desc->getDescItem(entry, SQLDESC_OCTET_LENGTH, &inputDatalen, NULL, 0, NULL, 0, NULL, 0) == ERROR)
            return ex_expr::EXPR_ERROR;

	  Lng32 alignment = 1;
	  if (operand->getNullIndOffset() >= 0)
	    {
	      if (operand->getTupleFormat() == ExpTupleDesc::SQLARK_EXPLODED_FORMAT)
		alignment = operand->getNullIndicatorLength();
	      
	      currAlignedOffset = ADJUST(currAlignedOffset, alignment);
	      nullIndOffset = currAlignedOffset;
	      
	      currEntryStartOffset = currAlignedOffset;
	      currEntryStartAlignment = alignment;

	      currAlignedOffset += operand->getNullIndicatorLength();
	    }
	  
	  if (operand->getVCLenIndOffset() >= 0)
	    {
	      if (operand->getTupleFormat() == ExpTupleDesc::SQLARK_EXPLODED_FORMAT)
		alignment = operand->getVCIndicatorLength();
	      currAlignedOffset = ADJUST(currAlignedOffset, alignment);
	      dataOffset = currAlignedOffset;

	      if (currEntryStartOffset == -1)
		{
		  currEntryStartOffset = currAlignedOffset;
		  currEntryStartAlignment = alignment;
		}

	      currAlignedOffset += operand->getVCIndicatorLength() + inputDatalen;
	    }
	  else
	    {
	      if (operand->getTupleFormat() == ExpTupleDesc::SQLARK_EXPLODED_FORMAT)
		alignment = operand->getDataAlignmentSize();
	      currAlignedOffset = ADJUST(currAlignedOffset, alignment);
	      dataOffset = currAlignedOffset;

	      if (currEntryStartOffset == -1)
		{
		  currEntryStartOffset = currAlignedOffset;
		  currEntryStartAlignment = alignment;
		}

	      currAlignedOffset += inputDatalen;
	    }

	  if (NOT firstEntryProcessed)
	    {
	      firstEntryProcessed = TRUE;
	      firstEntryStartAlignment = currEntryStartAlignment;
	    }

	  input_desc->setDescItemInternal(entry, SQLDESC_DATA_OFFSET,
					  dataOffset, 0);
	  input_desc->setDescItemInternal(entry, SQLDESC_NULL_IND_OFFSET,
					  nullIndOffset, 0);
	  
	  input_desc->setDescItemInternal(entry-1, SQLDESC_ALIGNED_LENGTH,
					  currEntryStartOffset - prevEntryStartOffset, 0);

	  prevEntryStartOffset = currEntryStartOffset;
        } // INOUT clause

next_clause:
      clause = clause->getNextClause();
    }

  if (firstEntryProcessed)
    {
      currAlignedOffset = ADJUST(currAlignedOffset, firstEntryStartAlignment);
      input_desc->setDescItemInternal(entry, SQLDESC_ALIGNED_LENGTH,
				       currAlignedOffset - prevEntryStartOffset, 0);
    }

  if(!(isCall() && input_desc->isDescTypeWide())
     || (entry > input_desc->getUsedEntryCount()))
    input_desc->setUsedEntryCount(entry);

  return ex_expr::EXPR_OK;
}



/* If you are adding code to InputValues that will may cause an error to generated
(i.e. an SQL error with some SQLCODE value) then please read on.
You must make two decision about the error
1. Can this error be raised when rowset input data is being copied in? 
If YES, is the error data dependent? A data dependent error is one which can occur
for row i of the rowset but not on row (i+1). In other words it is not an error that 
applies to the whole statement but only to a specific row in the rowsets. If the 
answer to both these questions is YES please place the following line of code at some
point after the error is in the diagsarea

  setRowNumberInCli(diagsArea, RowNum, targetRowsetSize) ;

2. If the answer the previous question(s) were YES, then read on. Now please
judge if this error can be considered Nonfatal for rowsets, i.e. does it make 
sense to continue execution with next rowset row or should we stop right away.
If you decide that it is a Nonfatal error (look in the method below for examples)
then place the following four lines of code after the call to setRowNumberInCli.

    if (tolerateNonFatalError && diagsArea->canAcceptMoreErrors())
      continue;
    else
      return ex_expr::EXPR_ERROR;
*/
// error path not taken . This is only if something bad happens in NVT
ex_expr::exp_return_type
InputOutputExpr::inputSingleRowValue(atp_struct *atp,
				     void * inputDesc_,
				     char * tgtRowPtr,
				     CollHeap * heap,
				     UInt32 flags)
{
  NABoolean isOdbc = isODBC(flags);
  NABoolean isDbtr = isDBTR(flags);
  NABoolean isRwrs = isRWRS(flags);
  NABoolean isIFIO  = (isInternalFormatIO(flags) ||
		       (isDbtr && isRwrs));

  ex_clause * clause = getClauses();
  Descriptor * inputDesc = (Descriptor *)inputDesc_;
  Attributes * operand = 0;
  ComDiagsArea *diagsArea = atp->getDiagsArea();
  
  char * target = 0;
  char * targetVCLenInd = 0;
  char * targetNullInd = 0;
  char * source = 0;
  Lng32   sourceLen;
  char * sourceIndPtr = 0;
  short  sourceType;
  Lng32   sourcePrecision;
  Lng32   sourceScale;
  char *    tempSource   = 0;
  Lng32   tempSourceType   = 0;
  Lng32   indData;
  short  entry = 0;
  char * intermediate = NULL;
  short  savedSourceType = 0;
  Lng32   savedSourcePrecision = 0;
  Lng32   savedSourceScale = 0;
  Lng32   savedSourceLen = 0;
  ExeErrorCode   errorCode = EXE_OK;

  // if bulk move has not been disabled before, then check to see if bulk
  // move could be done. This is done by comparing attributes of the 
  // executor items and the descriptor entries. See method
  // Descriptor::isBulkMovePossible() for details on when bulk move will
  // be disabled.
  // If bulk move is possible then set up bulk move info, if not already
  // done.
  if (NOT inputDesc->bulkMoveDisabled())
    {
      if (NOT inputDesc->bulkMoveSetup())
	{
	  setupBulkMoveInfo(inputDesc, heap, TRUE /* input desc */, flags);
	}
      
      if (NOT inputDesc->bulkMoveDisabled())
	{
	  // bulk move is enabled and setup has been done.
	  // Do bulk move now.
	  doBulkMove(atp, inputDesc, tgtRowPtr, TRUE /* input desc */);

	  if (NOT inputDesc->doSlowMove())
	    return ex_expr::EXPR_OK;
	}
    }
           
  CharInfo::CharSet sourceCharSet = CharInfo::UnknownCharSet;
  CharInfo::CharSet targetCharSet = CharInfo::UnknownCharSet;
 
  while (clause) 
    {
      if (clause->getType() == ex_clause::INOUT_TYPE) 
	{
	  entry++;
	  
	  if ((NOT inputDesc->bulkMoveDisabled()) &&
	      (NOT inputDesc->doSlowMove(entry)))
	    goto next_clause;
	  
	  operand = clause->getOperand(0);
	  
	  if (operand->isBlankHV()) 
	    {
	      clause = clause->getNextClause();
	      continue;
	    }

	  char * dataPtr = 
	    (tgtRowPtr ? tgtRowPtr : 
	     (atp->getTupp(operand->getAtpIndex())).getDataPointer());
	  
	  target = (char *)(dataPtr + operand->getOffset());
	  
	  if (operand->getVCIndicatorLength() > 0)
	    targetVCLenInd = (char *)(dataPtr + operand->getVCLenIndOffset());
	  
	  if (operand->getNullFlag())
	    targetNullInd = (char*)(dataPtr + operand->getNullIndOffset());
	  
	  inputDesc->getDescItem(entry, SQLDESC_TYPE_FS, &tempSourceType, 0, 0, 0, 0);
	  sourceType = (short) tempSourceType;
	  
	  if ((sourceType >= REC_MIN_INTERVAL) &&
	      (sourceType <= REC_MAX_INTERVAL)) {
	    inputDesc->getDescItem(entry, SQLDESC_INT_LEAD_PREC,
				   &sourcePrecision, 0, 0, 0, 0);
	    
	    // SQLDESC_PRECISION gets the fractional precision
	    inputDesc->getDescItem(entry, SQLDESC_PRECISION,
				   &sourceScale, 0, 0, 0, 0);
	  } 
	  else if (sourceType == REC_DATETIME) {
	    inputDesc->getDescItem(entry, SQLDESC_DATETIME_CODE, 
				   &sourcePrecision, 0, 0, 0, 0);
	    inputDesc->getDescItem(entry, SQLDESC_PRECISION,
				   &sourceScale, 0, 0, 0, 0);
	  } else {
	    inputDesc->getDescItem(entry, SQLDESC_PRECISION,
				   &sourcePrecision, 0, 0, 0, 0);
	    inputDesc->getDescItem(entry, SQLDESC_SCALE,
				   &sourceScale, 0, 0, 0, 0);
	  };
	  
	  if ((sourceType >= REC_MIN_CHARACTER) &&
	      (sourceType <= REC_MAX_CHARACTER)) {
	    
	    Lng32 temp_char_set;
	    inputDesc->getDescItem(
		 entry, SQLDESC_CHAR_SET, &temp_char_set,
		 NULL, 0, NULL, 0);
	    sourceCharSet = (CharInfo::CharSet)temp_char_set;
            sourceScale = sourceCharSet;
	  }
	  
	  // 5/18/98: added to handle SJIS encoding charset case.
	  // This is used in ODBC where the target character set is still
	  // ASCII but the actual character set used is SJIS.
	  //
	  short targetType = operand->getDatatype();
	  targetCharSet = operand->getCharSet();
	  
	  if ( (targetType == REC_NCHAR_F_UNICODE ||
		targetType == REC_NCHAR_V_UNICODE )
	       )
	    {
	      // charset can be retrieved as a long INTERNALLY
	      // using programatic interface by calling getDescItem(),
	      // and can only be retrieved as a character string EXTERNALLY
	      // using "get descriptor" syntax.
	      
	      if ( sourceCharSet == CharInfo::SJIS ) 
		{
		  switch ( sourceType ) {
		  case REC_BYTE_F_ASCII:
		    sourceType = REC_MBYTE_F_SJIS;
		    break;
		    
		  case REC_BYTE_V_ANSI:
		  case REC_BYTE_V_ASCII_LONG:
		  case REC_BYTE_V_ASCII:
		    sourceType = REC_MBYTE_V_SJIS;
		    break;
		    
		  default:
		    break;
		  }
		}
	    }
	  
	  // The following is redefined at a later point for variable length 
	  // strings
	  inputDesc->getDescItem(entry, SQLDESC_OCTET_LENGTH, &sourceLen, 0, 0, 0, 
				 0);
	  
	  inputDesc->getDescItem(entry, SQLDESC_IND_DATA, &indData,
				 0, 0, 0, 0);
	  if (indData == 0) 
	    {
	      inputDesc->getDescItem(entry, SQLDESC_IND_PTR, &tempSource, 
				     0, 0, 0, 0);
	    }
	  else
	    tempSource = (char * )&indData;
	  
	  if ((isOdbc) && (isRwrs))
	    {
	      if (operand->getNullIndOffset() >= 0)
		sourceIndPtr = (char*)
		  (inputDesc->getCurrRowPtrInRowwiseRowset()
		   + operand->getNullIndOffset());
	      else
		sourceIndPtr = NULL;
	    }
	  else
	    sourceIndPtr = tempSource;

	  NABoolean nullMoved = FALSE;
	  if (sourceIndPtr) 
	    {
	      short temp = 0;
	      str_cpy_all((char *)&temp, sourceIndPtr, operand->getNullIndicatorLength());
	      if (temp > 0)
		{
		  // invalid null indicator value.
		  // Valid indicator values are 0(not null value) and
		  // -ve (null value).
		  errorCode = EXE_CONVERT_STRING_ERROR;
		  goto error_return;
		}
	      
	      if (operand->getNullFlag()) 
		{
		  nullMoved = (temp < 0);   //if null indicator is a negative
		  //number, the value is null
		  
		  str_cpy_all (targetNullInd, sourceIndPtr,
			       operand->getNullIndicatorLength());
		}
	      else 
		{
		  if (temp < 0) 
		    {
		      // input is a null value
		      // error. Trying to move a null input value to a 
		      // non-nullable buffer.
		      errorCode = EXE_ASSIGNING_NULL_TO_NOT_NULL;
		      goto error_return;
		    }
		}
	    }
	  else 
	    {
	      // indicator not specified.
	      if (operand->getNullFlag()) 
		{
		  // move 0 (non-null value) to first two bytes of buffer.
		  str_pad(targetNullInd, operand->getNullIndicatorLength(), '\0');
		}
	    }

	  if ((isOdbc) && (isRwrs))
	    {
	      if (operand->getOffset() >= 0)
		source = (char*)(inputDesc->getCurrRowPtrInRowwiseRowset()
				 + operand->getOffset());
	      else
		source = NULL;
	    }
	  else
	    source = inputDesc -> getVarData(entry);

	  if (!source) 
	    {
	      continue;
	    }
	  
	  if (DFS2REC::isSQLVarChar(sourceType))
	    {
	      // the first 2 bytes of data are actually the variable 
	      // length indicator
	      short VCLen;
	      str_cpy_all((char *) &VCLen, source, sizeof(short));
	      sourceLen = (Lng32) VCLen;
	      source = &source[sizeof(short)];
#ifdef _DEBUG
	      assert(SQL_VARCHAR_HDR_SIZE == sizeof(short));
#endif
	    }
	  
	  // 12/22/97: added for Unicode Vchar.
	  if (! nullMoved) {
	    
	    ex_expr::exp_return_type retcode = ex_expr::EXPR_OK;  
	    Lng32 warningMark;
	    if (diagsArea)
	      warningMark= diagsArea->getNumber(DgSqlCode::WARNING_);
	    else
	      warningMark = 0;
	    
	    NABoolean implicitConversion = FALSE;
	    // if the source and target are not compatible, then do implicit
	    // conversion if skipTypeCheck is set.
	    // Furthermore, if restrictedSkipTypeCheck is set, then only
	    // convert for the conversions that are externally supported.
	    // See usage of default IMPLICIT_HOSTVAR_CONVERSION in generator
	    // (GenExpGenerator.cpp) for restricted conversions.
	    
	    if (NOT areCompatible(sourceType, targetType))
	      {
		if ((NOT skipTypeCheck()) ||
		    ((restrictedSkipTypeCheck()) && 
		     (NOT implicitConversionSupported(sourceType,
						      operand->getDatatype()))))
		  {
		    errorCode = EXE_CONVERT_NOT_SUPPORTED;
		    goto error_return;
		  }
		
		implicitConversion = TRUE;
	      } 
	    else 
	      {
		// check charset for source and target if they are CHARACTER types.
		if ( DFS2REC::isAnyCharacter(sourceType) &&
		     NOT CharInfo::isAssignmentCompatible(sourceCharSet, targetCharSet)
		     )                  
		  {
		    ExRaiseSqlError(heap, &diagsArea, CLI_ASSIGN_INCOMPATIBLE_CHARSET);
		    
		    if (diagsArea != atp->getDiagsArea())
		      atp->setDiagsArea(diagsArea);
		    
		    *diagsArea
		      << DgString0(CharInfo::getCharSetName((CharInfo::CharSet)sourceCharSet))
		      << DgString1(CharInfo::getCharSetName((CharInfo::CharSet)targetCharSet));
		    
		    return ex_expr::EXPR_ERROR;
		  }
	      }
	    
	    // Check length limit for KANJI/KSC5601/UCS2 ANSI VARCHAR (i.e., there 
	    // should be a wchar_t typed NULL somewhere in the range [1, sourceLen/2]
	    // in the source. We have to do this for KANJI/KSC because their FS type
	    // codes are shared with the ISO88591 CHARACTER types and the conversion 
	    // routines for ISO88591 ANSI VARCHAR in convDoIt() can not verify the 
	    // wchar_t nulls. 
	    // 
	    // We also check the ANSI Unicode source here because we then can safely 
	    // use the NAWstrlen() function to obtain the length in the next IF block to 
	    // check code points. 
	    if ((sourceType == REC_BYTE_V_ANSI && 
		 CharInfo::is_NCHAR_MP(sourceCharSet)) ||
		sourceType == REC_NCHAR_V_ANSI_UNICODE  
		)
	      {
		Lng32 i = 0;
		Int32 sourceLenInWchar = sourceLen/SQL_DBCHAR_SIZE;
		wchar_t* sourceInWchar = (wchar_t*)source;
		while ((i < sourceLenInWchar) && (sourceInWchar[i] != 0))
		  i++;
		
		if ( i == sourceLenInWchar ) 
		  {
		    errorCode = EXE_MISSING_NULL_TERMINATOR;
		    goto error_return;
		  };
	      }
	    
	    
	    if ( DFS2REC::isAnyCharacter(sourceType) &&
		 sourceCharSet == CharInfo::UNICODE ) 
	      {
		
		Lng32 realSourceLen = sourceLen;
		
		if (sourceType == REC_NCHAR_V_ANSI_UNICODE)  
		  {
		    realSourceLen = NAWstrlen((wchar_t*)source) * SQL_DBCHAR_SIZE;
		    
		    if ( realSourceLen > sourceLen )
		      realSourceLen = sourceLen ;
		  }
		realSourceLen /= SQL_DBCHAR_SIZE;
		
		if ( CharInfo::checkCodePoint((wchar_t*)source, realSourceLen,
					      CharInfo::UNICODE ) == FALSE )
		  {
		    // Error code 3400 falls in CLI error code area, but it is
		    // a perfect fit to use here (as an exeutor error).
		    ExRaiseSqlError(heap, &diagsArea, (ExeErrorCode)3400);
		    
		    if (diagsArea != atp->getDiagsArea())
		      atp->setDiagsArea(diagsArea);
		    
		    *diagsArea << DgString0(SQLCHARSETSTRING_UNICODE);
		    
		    return ex_expr::EXPR_ERROR;
		  }
	      }
	    
	    // if incompatible conversions are to be done,
	    // and the source is REC_BYTE_V_ANSI, 
	    // and the target is not a string type, convert the source to 
	    // REC_BYTE_F_ASCII first. Conversion from V_ANSI is currently
	    // only supported if target is a string type.
	    if ((implicitConversion) &&
		((sourceType == REC_BYTE_V_ANSI) &&
		 ((targetType < REC_MIN_CHARACTER) ||
		  (targetType > REC_MAX_CHARACTER))))
	      {
		intermediate = new (heap) char[sourceLen];
		if (::convDoIt(source,
			       sourceLen,
			       sourceType,
			       sourcePrecision,
			       sourceScale,
			       intermediate,
			       sourceLen,
			       REC_BYTE_F_ASCII,
			       sourcePrecision,
			       sourceScale,
			       NULL,
			       0,
			       heap,
			       &diagsArea) !=  ex_expr::EXPR_OK) 
		  {
		    NADELETEBASIC(intermediate, heap);
		    errorCode = EXE_OK;
		    goto error_return;
		  };
		
		sourceType = REC_BYTE_F_ASCII;
		source = intermediate;
	      } // sourceType == REC_BYTE_V_ANSI
	    
	    if ((sourceType >= REC_MIN_BINARY_NUMERIC) &&
		(sourceType <= REC_MAX_BINARY_NUMERIC) &&
		(sourceType == operand->getDatatype()) &&
		(sourcePrecision > 0) &&
		(sourcePrecision == operand->getPrecision()))
	      {
		// validate source precision. Make the source precision
		// zero, that will trigger the validation. See method
		// checkPrecision in exp_conv.cpp.
		sourcePrecision = 0;
	      }
	    
	    ULng32 convFlags = 0;
	    if ((NOT implicitConversion) &&
		(((sourceType == REC_DATETIME) ||
		  ((sourceType >= REC_MIN_INTERVAL) &&
		   (sourceType <= REC_MAX_INTERVAL))) &&
		 (NOT isIFIO)))
	      {
		// this is a datetime or interval conversion.
		// Convert the external string datetime/interval to its
		// internal format. The conversion follows the
		// same rules as "CAST(char TO datetime/interval)".
		
		// If the two interval types are not the same, then first
		// convert source from its string to its internal format,
		// and then convert to the target interval type.
		if (sourceType == REC_DATETIME)
		  {
		    if ((sourcePrecision != operand->getPrecision()) ||
			(sourceScale != operand->getScale()))
		      {
			// source and target must have the same datetime_code,
			// they must both be date, time or timestamp. 
			errorCode = EXE_CONVERT_NOT_SUPPORTED;
			goto error_return;
		      }
		    else
		      sourceType = REC_BYTE_F_ASCII;
		  }
		else if ((sourceType == targetType) &&
			 (sourcePrecision == operand->getPrecision()) &&
			 (sourceScale == operand->getScale()))
		  {
		    // 'interval' source and target match all attributes.
		    // Convert source(string external) to target(internal) format
		    sourceType = REC_BYTE_F_ASCII;
		    
		    convFlags |= CONV_ALLOW_SIGN_IN_INTERVAL;
		  }
		else
		  {
		    // interval type and source/target do not match.
		    // Convert source string interval to corresponding
		    // internal format.
		    short intermediateLen = SQL_LARGE_SIZE;
		    intermediate = new (heap) char[intermediateLen];
		    convFlags |= CONV_ALLOW_SIGN_IN_INTERVAL;
		    if (::convDoIt(source,
				   sourceLen,
				   REC_BYTE_F_ASCII,
				   0, //sourcePrecision,
				   0, //sourceScale,
				   intermediate,
				   intermediateLen,
				   sourceType,
				   sourcePrecision,
				   sourceScale,
				   NULL,
				   0,
				   heap,
				   &diagsArea,
				   CONV_UNKNOWN,
				   0,
				   convFlags) !=  ex_expr::EXPR_OK) 
		      {
			NADELETEBASIC(intermediate, heap);
			
			errorCode = EXE_OK;
			goto error_return;
		      };
		    
		    source = intermediate;
		    sourceLen = intermediateLen;
		  }
	      }

	    if (noDatetimeValidation())
	      convFlags |= CONV_NO_DATETIME_VALIDATION;
	    
	    retcode = ::convDoIt(source,
				 sourceLen,
				 sourceType,
				 sourcePrecision,
				 sourceScale,
				 target,
				 operand->getLength(),
				 operand->getDatatype(),
				 operand->getPrecision(),
				 operand->getScale(),
				 targetVCLenInd,
				 operand->getVCIndicatorLength(),
				 heap,
				 &diagsArea,
				 CONV_UNKNOWN,
				 0,
				 convFlags);
	    
	    // NADELETEBASIC(intermediate, heap);
	    if (retcode != ex_expr::EXPR_OK)
	      {
		errorCode = EXE_OK;
		goto error_return;
	      }
	    else if (diagsArea && diagsArea->mainSQLCODE() == EXE_STRING_OVERFLOW)
	      {
		Int32 warningMark2 = diagsArea->getNumber(DgSqlCode::WARNING_);
		Int32 counter = warningMark2 - warningMark;
		
		diagsArea->deleteWarning(warningMark2-counter);
		
		errorCode = EXE_STRING_OVERFLOW;
		goto error_return;
	      }
	    
	    // up- or down-scale target.
	    if ((operand->getDatatype() >= REC_MIN_NUMERIC) &&
		(operand->getDatatype() <= REC_MAX_NUMERIC) &&
		(sourceScale != operand->getScale()) &&
		::scaleDoIt(target,
			    operand->getLength(),
			    operand->getDatatype(),
			    sourceScale,
			    operand->getScale(),
                            sourceType,
			    heap) != ex_expr::EXPR_OK)
	      {
		errorCode = EXE_NUMERIC_OVERFLOW;
		goto error_return;
	      }
	  } // if (! nullMoved)
	} // if clause == IN_OUT type
      
    next_clause:
      clause = clause->getNextClause();
    }  // while(clause)
  
  return ex_expr::EXPR_OK;
  
error_return:
  if (errorCode != EXE_OK)
    ExRaiseSqlError(heap, &diagsArea, errorCode);
  if (diagsArea != atp->getDiagsArea())
    atp->setDiagsArea(diagsArea);	
  
  return ex_expr::EXPR_ERROR;
}
ex_expr::exp_return_type
InputOutputExpr::inputRowwiseRowsetValues(atp_struct *atp,
					  void * inputDesc_,
					  void * rwrs_info,
					  NABoolean tolerateNonFatalError,
					  CollHeap * heap,
					  UInt32 flags)
{
  NABoolean isOdbc = isODBC(flags);
  NABoolean isDbtr = isDBTR(flags);

  ex_clause * clause = getClauses();
  Descriptor * inputDesc = (Descriptor *)inputDesc_;
  RWRSInfo   * rwrsInfo  = (RWRSInfo *)rwrs_info;

  Attributes * operand = 0;
  ComDiagsArea *diagsArea = atp->getDiagsArea();
  Lng32   targetRowsetSize = -1;
  Lng32   dynamicRowsetSize = -1;
  Lng32   rwrsInputBuffer = -1;
  char * source = NULL;
  char * target = NULL;
  short entry = 0;
  ExeErrorCode   errorCode = EXE_OK;
  Lng32 intParam1 = 0;
  Lng32 intParam2 = 0;
  Lng32 intParam3 = 0;

  Lng32 numRowsProcessed = 0;
  char * tgtRowAddr = rwrsInfo->getRWRSInternalBufferAddr();
  char ** rwrsInternalBufferAddrLoc = NULL;
  char * rwrsMaxInputRowlenLoc = NULL;

  Lng32 inputRowsetSize = -1;
  Lng32 maxInputRowlen  = -1;
  char  * bufferAddr      = NULL;
  Lng32 partnNum        = -1;

  // input RWRS are always V2.
  inputDesc->setRowwiseRowsetV2(TRUE);

  while (clause) 
    {
      if (clause->getType() == ex_clause::INOUT_TYPE) 
	{
	  entry++;
	  
	  if ((entry == rwrsInfo->rwrsInputSizeIndex()) ||
	      (entry == rwrsInfo->rwrsMaxInputRowlenIndex()) ||
	      (entry == rwrsInfo->rwrsBufferAddrIndex()) ||
	      (entry == rwrsInfo->rwrsPartnNumIndex()))
	    {
	      operand = clause->getOperand(0);

	      target = 
		(char *)((atp->getTupp(operand->getAtpIndex())).
			 getDataPointer() + operand->getOffset());

	      // find the number of rows in the input rowset
	      source = inputDesc->getVarData(entry);
	      if (entry == rwrsInfo->rwrsInputSizeIndex())
		{
                  if (source)
                    {
                      // do the conversion
                      str_cpy_all(target, source, sizeof(Lng32));
                    }
                  else
                    {
                      *(Lng32*)target = inputDesc->getRowwiseRowsetSize();
                    }

                  inputRowsetSize = *((Lng32 *) target);
		}
	      else if (entry == rwrsInfo->rwrsMaxInputRowlenIndex())
		{
                  if (source)
                    {
                      // do the conversion
                      str_cpy_all(target, source, sizeof(Lng32));
                    }
                  else
                    {
                      *(Lng32*)target = inputDesc->getRowwiseRowsetRowLen();
                    }
                  
                  maxInputRowlen = *((Lng32 *) target);
                  
                  rwrsMaxInputRowlenLoc = target;
                }
	      else if (entry == rwrsInfo->rwrsBufferAddrIndex())
		{
                  if (source)
                    {
                      // do the conversion
                      str_cpy_all(target, source, sizeof(Long));
                    }
                  else
                    {
                      *(Long*)target = inputDesc->getRowwiseRowsetPtr();
                    }

		  bufferAddr = *((char **) target);

		  // We need to send the address of the rowwise rowset
		  // buffer to other nodes below the root.
		  // It will be determined later in this procedure whether
		  // we can send the buffer address of the application's
		  // rowwise rowset buffer or if we first need to move data 
		  // to our internal buffer and send the addr of the internal
		  // buffer.
		  rwrsInternalBufferAddrLoc = (char **)target;
		}
	      else if (entry == rwrsInfo->rwrsPartnNumIndex())
		{
		  partnNum = *((Lng32 *) target);
		}
	
	      // skip this INOUT clause during rowset bulk move processing.
	      ((ex_inout_clause*)clause)->setExcludeFromBulkMove(TRUE);
 	    } // rwrs info
	} // if inout clause

      clause = clause->getNextClause();
    } // while clause
  
  if ((inputRowsetSize < 0) ||
      (inputRowsetSize > rwrsInfo->rwrsMaxSize()))
    {
      //raise error
      errorCode = (ExeErrorCode)30038; //EXE_RW_ROWSET_SIZE_OUTOF_RANGE;
      goto error_return;
    }

  if (maxInputRowlen < 0)
    {
      //raise error
      errorCode = (ExeErrorCode)30039; //EXE_RW_ROWSET_ROWLEN_OUTOF_RANGE;
      goto error_return;
    }


  if (inputRowsetSize == 0)
    return ex_expr::EXPR_OK;

  if (rwrsInfo->rwrsIsCompressed() && rwrsInfo->dcompressInMaster())
    {
      // if not already allocated, allocate a buffer to decompress user buf.
      Lng32 dcomBufLen = maxInputRowlen * rwrsInfo->rwrsMaxSize();
      if ((rwrsInfo->getRWRSDcompressedBufferAddr() == NULL) ||
	  (rwrsInfo->getRWRSDcompressedBufferLen() < dcomBufLen))
	{
	  if (rwrsInfo->getRWRSDcompressedBufferAddr())
	    {
	      NADELETEBASIC((char *)rwrsInfo->getRWRSDcompressedBufferAddr(), heap);
	    }

	  char * dcomBuf = new(heap) char[dcomBufLen];
	  rwrsInfo->setRWRSDcompressedBufferAddr(dcomBuf);
	  rwrsInfo->setRWRSDcompressedBufferLen(dcomBufLen);
	}

      // dcompress/decode the user buffer into the dcompressedBuffer.
      // Length of buffer is in the first 4 bytes of bufferAddr.
      // Pass the pointer to the length bytes to ExDecode.
      // Pass the buffer length plus size of length bytes.
      Lng32 compressedBuflen = *(Lng32 *)bufferAddr + sizeof(Lng32);
      unsigned char * compressedBuf = (unsigned char *)(bufferAddr);
      Int32 dcompressedBuflen = -1;
      Lng32 param1 = 0;
      Lng32 param2 = 0;
      Lng32 rc = ExDecode(compressedBuf, compressedBuflen, 
			 (unsigned char *)rwrsInfo->getRWRSDcompressedBufferAddr(),
			 &dcompressedBuflen, param1, param2);
      if (rc)
	{
	  // error.
	  errorCode = (ExeErrorCode)30045;
	  intParam1 = rc;
	  intParam2 = param1;
	  intParam3 = param2;
	  goto error_return;
	}

      if (dcompressedBuflen > rwrsInfo->getRWRSDcompressedBufferLen())
	{
	  // error.
	  errorCode = (ExeErrorCode)30046;
	  intParam1 = dcompressedBuflen;
	  intParam2 = rwrsInfo->getRWRSDcompressedBufferLen();
	  goto error_return;
	}
      
      // change user buffer addr to point to the decompressed buffer
      bufferAddr = rwrsInfo->getRWRSDcompressedBufferAddr();
    }

  // find out if user rwrs buffer could be passed in to the unpack method.
  // This could be done :
  //   -- bulk move is not disabled and there are no slow bulk moves
  //   -- user row layout is the same as the internal row 
  //   -- there is no mismatch in user and internal column definitions
  //   -- columns in user row could be moved with one bulk move to the
  //      internal buffer
  // if these conditions are met, then user buffer address, user row length
  // and number of rows are passed on to the unpack buffer.
  if (NOT inputDesc->bulkMoveDisabled())
    {
      if (NOT inputDesc->bulkMoveSetup())
	{
	  rwrsInfo->setUseUserRWRSBuffer(FALSE);

	  setupBulkMoveInfo(inputDesc, heap, TRUE /* input desc */, flags);
      
	  if ((NOT inputDesc->bulkMoveDisabled()) &&
	      (NOT inputDesc->doSlowMove()) &&
	      (inputDesc->bulkMoveInfo()->usedEntries() == 1))
	    {
	      rwrsInfo->setUseUserRWRSBuffer(TRUE);
	    }
	}
    }

  if (rwrsInfo->useUserRWRSBuffer())
    {
      inputDesc->setDescItem(0, SQLDESC_ROWWISE_ROWSET_SIZE, 
			     inputRowsetSize, 0);
      inputDesc->setDescItem(0, SQLDESC_ROWWISE_ROWSET_ROW_LEN, 
			     maxInputRowlen, 0);
      inputDesc->setDescItem(0, SQLDESC_ROWWISE_ROWSET_PTR, 
			     (Long)bufferAddr, 0);
      inputDesc->setDescItem(0, SQLDESC_ROWWISE_ROWSET_PARTN_NUM, 
			     partnNum, 0);

      inputDesc->setDescItem(0, SQLDESC_ROWSET_NUM_PROCESSED,
			     inputRowsetSize, NULL);

      // move the address of the user rowwise rowset buffer so it could
      // be passed on to other operators.
      *rwrsInternalBufferAddrLoc = bufferAddr;
      
      // move the max length of each user row so it could be passed on to
      // other operators.
      *rwrsMaxInputRowlenLoc = maxInputRowlen;
    }
  else
    {
      // error path not taken . This is only if something bad happens in NVT
      if (isDbtr)
	{
	  // rowwise rowsets from dbtr *must* use the optimized input
	  // path. Something is not setup correctly, if it doesn't.
	  // Return an error.
	  errorCode = (ExeErrorCode)30041;
	  goto error_return;
	}

      inputDesc->setDescItem(0, SQLDESC_ROWWISE_ROWSET_SIZE, 
			     inputRowsetSize, 0);
      inputDesc->setDescItem(0, SQLDESC_ROWWISE_ROWSET_ROW_LEN, 
			     maxInputRowlen, 0);
      inputDesc->setDescItem(0, SQLDESC_ROWWISE_ROWSET_PTR, 
			     (Long)bufferAddr, 0);

      while (numRowsProcessed < inputRowsetSize)
	{
	  inputDesc->setDescItem(0, SQLDESC_ROWSET_NUM_PROCESSED,
				 numRowsProcessed, NULL);
	  
	  setIsRWRS(flags, TRUE);
	  if (inputSingleRowValue(atp, inputDesc, (char*)tgtRowAddr, heap,
				  flags)
	      == ex_expr::EXPR_ERROR)
	    {
	      errorCode = EXE_OK;
	      goto error_return;
	    }
	  
	  numRowsProcessed++;
	  
	  tgtRowAddr += rwrsInfo->rwrsMaxInternalRowlen();
	}
  
      // reset rows processed
      inputDesc->setDescItem(0, SQLDESC_ROWSET_NUM_PROCESSED,
			     0, NULL);

      // move the address of the internal rowwise rowset buffer so it could
      // be passed on to other operators.
      *rwrsInternalBufferAddrLoc = 
	rwrsInfo->getRWRSInternalBufferAddr();
      
      // move the max length of each row so it could be passed on to other
      // operators.
      *rwrsMaxInputRowlenLoc =
	rwrsInfo->rwrsMaxInternalRowlen();
    } // use internal buffer

  return ex_expr::EXPR_OK;
  
error_return:
  if (errorCode != EXE_OK)
    ExRaiseSqlError(heap, &diagsArea, errorCode,
		    NULL,
		    (intParam1 != 0 ? &intParam1 : NULL),
		    (intParam2 != 0 ? &intParam2 : NULL),
		    (intParam3 != 0 ? &intParam3 : NULL));
  if (diagsArea != atp->getDiagsArea())
    atp->setDiagsArea(diagsArea);	
  return ex_expr::EXPR_ERROR;
}

ex_expr::exp_return_type
InputOutputExpr::inputValues(atp_struct *atp,
                             void * inputDesc_,
			     NABoolean tolerateNonFatalError,
                             CollHeap * heap,
                             UInt32 flags) {
  NABoolean isOdbc = isODBC(flags);
  NABoolean isIFIO  = isInternalFormatIO(flags);

  ex_clause * clause = getClauses();
  Descriptor * inputDesc = (Descriptor *)inputDesc_;
  Attributes * operand = 0;
  ComDiagsArea *diagsArea = atp->getDiagsArea();
  
  char * target = 0;
  char * targetVCLenInd = 0;
  char * targetNullInd = 0;
  Lng32   targetRowsetSize;
  Lng32   dynamicRowsetSize = 0; // Specified with ROWSET FOR INPUT SIZE <var>
                                // or some other rowset syntax; <var> can 
                                // change at run time
  char * source = 0;
  Lng32   sourceLen;
  char * sourceIndPtr = 0;
  short  sourceType;
  Lng32   sourcePrecision;
  Lng32   sourceScale;
  char *    tempSource   = 0;
  Lng32   tempSourceType   = 0;
  Lng32   indData;
  short entry = 0;
  char * intermediate = NULL;
  Lng32 rowsetVarLayoutSize;
  Lng32 rowsetIndLayoutSize;
  short  savedSourceType = 0;
  Lng32   savedSourcePrecision = 0;
  Lng32   savedSourceScale = 0;
  Lng32   savedSourceLen = 0;



  // We changed the code here, because the previous version uses
  // isCall() && isWideDescType() as a condition. isCall() returns FALSE when
  // there is no paramter index in the expression. So the error was
  // raised when there is no inputs and there is outputs.
  // We relax the bypass condition. until we have a fix in the generator that
  // can put a flag in the root to describe whether it is CALL statement.

  NABoolean useParamIdx = FALSE;
  
  if(!inputDesc) {
    if(getNumEntries() > 0)
    {
      ExRaiseSqlError(heap, &diagsArea, CLI_STMT_DESC_COUNT_MISMATCH);
      if (diagsArea != atp->getDiagsArea())
	atp->setDiagsArea(diagsArea);
      return ex_expr::EXPR_ERROR;
    }
    else
      return ex_expr::EXPR_OK;
  }
  else{
    useParamIdx = isCall() && inputDesc->isDescTypeWide();
    NABoolean byPassCheck = inputDesc->isDescTypeWide();
    if (!byPassCheck && !inputDesc->thereIsACompoundStatement() &&
	getNumEntries() != inputDesc->getUsedEntryCount())
    {
      ExRaiseSqlError(heap, &diagsArea, CLI_STMT_DESC_COUNT_MISMATCH);
      if (diagsArea != atp->getDiagsArea())
	atp->setDiagsArea(diagsArea);
      return ex_expr::EXPR_ERROR;
    }
  }
  
  if (inputDesc && inputDesc->rowwiseRowset())
    {
      ExRaiseSqlError(heap, &diagsArea, CLI_ROWWISE_ROWSETS_NOT_SUPPORTED);
      if (diagsArea != atp->getDiagsArea())
	atp->setDiagsArea(diagsArea);
      return ex_expr::EXPR_ERROR;
    }

  // If bulk move has not been disabled before, then check to see if bulk
  // move could be done. This is done by comparing attributes of the 
  // executor items and the descriptor entries. See method
  // Descriptor::checkBulkMoveStatus() for details on when bulk move will
  // be disabled.
  // If bulk move is possible then set up bulk move info, if not already
  // done.
  if (NOT inputDesc->bulkMoveDisabled())
    {
      if (NOT inputDesc->bulkMoveSetup())
	{
	  setupBulkMoveInfo(inputDesc, heap, TRUE /* input desc */, flags);
	}
      
      if (NOT inputDesc->bulkMoveDisabled())
	{
	  // bulk move is enabled and setup has been done.
	  // Do bulk move now.
	  doBulkMove(atp, inputDesc, NULL, TRUE /* input desc */);

	  if (NOT inputDesc->doSlowMove())
	    return ex_expr::EXPR_OK;
	}
    }
           
  CharInfo::CharSet sourceCharSet = CharInfo::UnknownCharSet;
  CharInfo::CharSet targetCharSet = CharInfo::UnknownCharSet;
 

  // Note that some of the input operands may participate in rowsets, while
  // other may not. That is, simple host variables and rowset host variables
  // can be mixed for input purposes.
  while (clause) {
    if (clause->getType() == ex_clause::INOUT_TYPE) {
      entry++;

      if ((NOT inputDesc->bulkMoveDisabled()) &&
	  (NOT inputDesc->doSlowMove(entry)))
	goto next_clause;

      if(useParamIdx){
	entry = ((ex_inout_clause *)clause)->getParamIdx();
      }

      operand = clause->getOperand(0);
      
      if (operand->isBlankHV()) {
        clause = clause->getNextClause();
	continue;
      }
      
      target = 
        (char *)((atp->getTupp(operand->getAtpIndex())).getDataPointer() +
                 operand->getOffset());
      
      if (operand->getVCIndicatorLength() > 0)
        targetVCLenInd =
          (char *)((atp->getTupp(operand->getAtpIndex())).getDataPointer() + 
                   operand->getVCLenIndOffset());
      
      if (operand->getNullFlag())
        targetNullInd = 
          (atp->getTupp(operand->getAtpIndex())).getDataPointer() 
          + operand->getNullIndOffset();
      
      inputDesc->getDescItem(entry, SQLDESC_TYPE_FS, &tempSourceType, 0, 0, 0, 0);
      sourceType = (short) tempSourceType;
      

      if ((sourceType >= REC_MIN_INTERVAL) &&
          (sourceType <= REC_MAX_INTERVAL)) {
        inputDesc->getDescItem(entry, SQLDESC_INT_LEAD_PREC,
                               &sourcePrecision, 0, 0, 0, 0);

	// SQLDESC_PRECISION gets the fractional precision
        inputDesc->getDescItem(entry, SQLDESC_PRECISION,
                               &sourceScale, 0, 0, 0, 0);
      } 
      else if (sourceType == REC_DATETIME) {
        inputDesc->getDescItem(entry, SQLDESC_DATETIME_CODE, 
                               &sourcePrecision, 0, 0, 0, 0);
        inputDesc->getDescItem(entry, SQLDESC_PRECISION,
                               &sourceScale, 0, 0, 0, 0);
      } else {
        inputDesc->getDescItem(entry, SQLDESC_PRECISION,
                               &sourcePrecision, 0, 0, 0, 0);
        inputDesc->getDescItem(entry, SQLDESC_SCALE,
                               &sourceScale, 0, 0, 0, 0);
      };

      if ((sourceType >= REC_MIN_CHARACTER) &&
          (sourceType <= REC_MAX_CHARACTER)) {

        Lng32 temp_char_set;
        inputDesc->getDescItem(
                entry, SQLDESC_CHAR_SET, &temp_char_set,
		NULL, 0, NULL, 0);
        sourceCharSet = (CharInfo::CharSet)temp_char_set;
        sourceScale = temp_char_set;
      }
      
      // 5/18/98: added to handle SJIS encoding charset case.
      // This is used in ODBC where the target character set is still
      // ASCII but the actual character set used is SJIS.
      //
      short targetType = operand->getDatatype();
      targetCharSet = operand->getCharSet();
      
      if ( (targetType == REC_NCHAR_F_UNICODE ||
	    targetType == REC_NCHAR_V_UNICODE )
           )
        {
	   // charset can be retrieved as a long INTERNALLY
	   // using programatic interface by calling getDescItem(),
	   // and can only be retrieved as a character string EXTERNALLY
	   // using "get descriptor" syntax.

           if ( sourceCharSet == CharInfo::SJIS ) 
	    {
	      switch ( sourceType ) {
	      case REC_BYTE_F_ASCII:
		sourceType = REC_MBYTE_F_SJIS;
		break;
		
	      case REC_BYTE_V_ANSI:
	      case REC_BYTE_V_ASCII_LONG:
	      case REC_BYTE_V_ASCII:
		sourceType = REC_MBYTE_V_SJIS;
		break;
		
	      default:
		break;
	      }
	    }
        }
      
      //	ex_expr::exp_return_type retcode = ex_expr::EXPR_OK;  
      // long warningMark;
      //	if (diagsArea)
      //	  warningMark= diagsArea->getNumber(DgSqlCode::WARNING_);
      //	else
      //	  warningMark = 0;
      
      
      // The following is redefined at a later point for variable length 
      // strings
      inputDesc->getDescItem(entry, SQLDESC_OCTET_LENGTH, &sourceLen, 0, 0, 0, 
                             0);
      
      // Verify that the operand deals with the same rowset as specified
      // in the descriptor. This check was added since the pre-processor
      // have had generated different information for the module defintion
      // file and the cpp file.
      
      // Find rowset size, if any
      targetRowsetSize = operand->getRowsetSize();
      
      if (targetRowsetSize > 0) {	
	// dynamicRowsetSize is specified with ROWSET FOR INPUT SIZE <var>
	// or some other rowset syntax; <var> can change at run time
	if (dynamicRowsetSize > targetRowsetSize) {
	  ExRaiseSqlError(heap, &diagsArea, EXE_ROWSET_INDEX_OUTOF_RANGE);
	  if (diagsArea != atp->getDiagsArea()) {
	    atp->setDiagsArea(diagsArea);	    
	    return ex_expr::EXPR_ERROR;
	  }
	}
	
	if (dynamicRowsetSize > 0) {
	  targetRowsetSize = dynamicRowsetSize;
	}
	
        // do the conversion
        source = (char *)&targetRowsetSize;
        if (::convDoIt(source,
                       sizeof(Lng32),
                       REC_BIN32_SIGNED,
                       0,
                       0,
                       target,
                       sizeof(Lng32),
                       REC_BIN32_SIGNED,
                       0,
                       0,
                       0,
                       0
                       ) != ex_expr::EXPR_OK)
	  {
	    if (diagsArea != atp->getDiagsArea())
	      atp->setDiagsArea(diagsArea);	    
	    
	    return ex_expr::EXPR_ERROR;
	  }
	
        // Increment the location for the next element. 
	target += sizeof(Lng32); 
        if (operand->getNullFlag()) { 
	  targetNullInd = target;
          target += operand->getNullIndicatorLength();
        }
	
        if (operand->getVCIndicatorLength() > 0) { 
	  target += operand->getVCIndicatorLength();
        } 
	// save current source and target datatype attributes since
	// they may get changed later in this method.
	// Restore them for the next iteration of this for loop.
	// Do this only for rowsets, for non-rowsets this loop is executed
	// only once.
	savedSourceType = sourceType;
	savedSourcePrecision = sourcePrecision;
	savedSourceScale = sourceScale;
	savedSourceLen = sourceLen;
      } // if (targetRowsetSize > 0)
      else {
	inputDesc->getDescItem(entry, SQLDESC_ROWSET_VAR_LAYOUT_SIZE, &rowsetVarLayoutSize, 
	                       0, 0, 0, 0);
	if (rowsetVarLayoutSize > 0) {
	  ExRaiseSqlError(heap, &diagsArea, EXE_ROWSET_SCALAR_ARRAY_MISMATCH);
	  if (diagsArea != atp->getDiagsArea()) {
	    atp->setDiagsArea(diagsArea);	    
	    return ex_expr::EXPR_ERROR;
	    // Raises an error when query was compiled with scalar param 
	    // and array value is passed in at runtime. When query is compiled 
	    // with array params and some (not all) values are scalars at runtime
	    // we do not raise an error and we duplicate the scalar value.
	  }
	}
      } // else (targetRowsetSize > 0)
      
      Lng32 numToProcess = ((targetRowsetSize > 0) ? targetRowsetSize : 1);

      // Define and clear case indexes potentially used for rowsets
      ConvInstruction index1 = CONV_UNKNOWN;
      ConvInstruction index2 = CONV_UNKNOWN;
      ConvInstruction index3 = CONV_UNKNOWN;
      
      for (Lng32 RowNum = 0; RowNum < numToProcess; RowNum++) 
        {

	  // Increment the location for the next element.
          if ((targetRowsetSize > 0) && (RowNum > 0)) {
            target += operand->getStorageLength();
            if (operand->getVCIndicatorLength() > 0)
              targetVCLenInd += operand->getStorageLength();
            if (operand->getNullFlag())
              targetNullInd  += operand->getStorageLength();

	    // restore the original datatype attributes.
	      sourceType = savedSourceType;
	      sourcePrecision = savedSourcePrecision;
	      sourceScale = savedSourceScale;
	      sourceLen = savedSourceLen;
          }

	  
          if (targetRowsetSize > 0)
            inputDesc->setDescItem(0, SQLDESC_ROWSET_HANDLE, RowNum, 0);
          inputDesc->getDescItem(entry, SQLDESC_IND_DATA, &indData,
                                 0, 0, 0, 0);
          if (indData == 0) {
            inputDesc->getDescItem(entry, SQLDESC_IND_PTR, &tempSource, 
                                   0, 0, 0, 0);
	    inputDesc->getDescItem(entry, SQLDESC_ROWSET_IND_LAYOUT_SIZE, &rowsetIndLayoutSize, 
	                       0, 0, 0, 0);
	    if ((!tempSource) && (rowsetIndLayoutSize > 0)) {
	      ExRaiseSqlError(heap, &diagsArea, EXE_ROWSET_VARDATA_OR_INDDATA_ERROR);
	      if (diagsArea != atp->getDiagsArea())
		  atp->setDiagsArea(diagsArea);	    	
	      return ex_expr::EXPR_ERROR;
	    }
	  }
          else
            tempSource = (char *)&indData;

          sourceIndPtr = tempSource;
	  
          NABoolean nullMoved = FALSE;
          if (sourceIndPtr) {
	    short temp = 0;
	    str_cpy_all((char *)&temp, sourceIndPtr, operand->getNullIndicatorLength());
	    if (inputDesc && inputDesc->thereIsACompoundStatement()  
	        && (temp > 0)) {
	      // We may be processing input data with invalid indicators if we
	      // are in an IF statement
	      temp = 0;
	    }
	    if (temp > 0)
	      {
		// invalid null indicator value.
		// Valid indicator values are 0(not null value) and
		// -ve (null value).
		ExRaiseSqlError(heap, &diagsArea, EXE_CONVERT_STRING_ERROR);
		if (diagsArea != atp->getDiagsArea())
		  atp->setDiagsArea(diagsArea);	
		
		setRowNumberInCli(diagsArea, RowNum, targetRowsetSize) ;
		if (tolerateNonFatalError && diagsArea->canAcceptMoreErrors())
		  continue;
		else
		  return ex_expr::EXPR_ERROR;
	      }
	    
            if (operand->getNullFlag()) {
              nullMoved = (temp < 0);   //if null indicator is a negative
	      //number, the value is null
	      
              str_cpy_all (targetNullInd, sourceIndPtr,
			   operand->getNullIndicatorLength());
            }
            else {
              if (temp < 0) {
                // input is a null value
                // error. Trying to move a null input value to a 
                // non-nullable buffer.
		ExRaiseSqlError(heap, &diagsArea, EXE_ASSIGNING_NULL_TO_NOT_NULL);
		if (diagsArea != atp->getDiagsArea())
		  atp->setDiagsArea(diagsArea);	
		
		setRowNumberInCli(diagsArea, RowNum, targetRowsetSize) ;
                if (tolerateNonFatalError && diagsArea->canAcceptMoreErrors())
		  continue;
		else
		  return ex_expr::EXPR_ERROR;
              }
            }
          }
          else {
            // indicator not specified.
            if (operand->getNullFlag()) {
              // move 0 (non-null value) to first two bytes of buffer.
              str_pad(targetNullInd, operand->getNullIndicatorLength(), '\0');
            }
          }
	  
          source = inputDesc -> getVarData(entry);
          if (!source) {
	    continue;
	  }

          if (DFS2REC::isSQLVarChar(sourceType)) {
            Lng32 vcIndLen = inputDesc->getVarIndicatorLength(entry);
            sourceLen = ExpTupleDesc::getVarLength(source, vcIndLen);
            source = &source[vcIndLen];
          }

	  // 12/22/97: added for Unicode Vchar.
          if (! nullMoved) {
	    
	    ex_expr::exp_return_type retcode = ex_expr::EXPR_OK;  
	    Lng32 warningMark;
	    if (diagsArea)
	      warningMark= diagsArea->getNumber(DgSqlCode::WARNING_);
	    else
	      warningMark = 0;
	    
	    NABoolean implicitConversion = FALSE;
	    // if the source and target are not compatible, then do implicit
	    // conversion if skipTypeCheck is set.
	    // Furthermore, if restrictedSkipTypeCheck is set, then only
	    // convert for the conversions that are externally supported.
	    // See usage of default IMPLICIT_HOSTVAR_CONVERSION in generator
	    // (GenExpGenerator.cpp) for restricted conversions.

	    if (NOT areCompatible(sourceType, targetType))
	      {
		if ((NOT skipTypeCheck()) ||
		    ((restrictedSkipTypeCheck()) && 
		     (NOT implicitConversionSupported(sourceType,
						      operand->getDatatype()))))
                {
	           ExRaiseSqlError(heap, &diagsArea,EXE_CONVERT_NOT_SUPPORTED);
	           if (diagsArea != atp->getDiagsArea())
	             atp->setDiagsArea(diagsArea);
		   
		   setRowNumberInCli(diagsArea, RowNum, targetRowsetSize) ;
	           if (tolerateNonFatalError && diagsArea->canAcceptMoreErrors())
		    continue;
		   else
		    return ex_expr::EXPR_ERROR;
		}

		implicitConversion = TRUE;

	      } else {
                 // check charset for source and target if they are CHARACTER types.
                 if ( DFS2REC::isAnyCharacter(sourceType) &&
                      NOT CharInfo::isAssignmentCompatible(sourceCharSet, targetCharSet)
                    )                  
                 {
                      ExRaiseSqlError(heap, &diagsArea, CLI_ASSIGN_INCOMPATIBLE_CHARSET);

                      if (diagsArea != atp->getDiagsArea())
                          atp->setDiagsArea(diagsArea);

                      *diagsArea
                        << DgString0(CharInfo::getCharSetName((CharInfo::CharSet)sourceCharSet))
                        << DgString1(CharInfo::getCharSetName((CharInfo::CharSet)targetCharSet));
			
			setRowNumberInCli(diagsArea, RowNum, targetRowsetSize) ;
                        if (tolerateNonFatalError && diagsArea->canAcceptMoreErrors())
			  continue;
			else
			  return ex_expr::EXPR_ERROR;
                 }
              }

	    
	    // In the case of VARCHARS, we zero out remaining elements
            if (targetRowsetSize > 0) {
	      Int32 i;
	      if ((sourceType >= REC_MIN_V_N_CHAR_H) 
		  && (sourceType <= REC_MAX_V_N_CHAR_H)) {
		for (i = strlen(source); i < operand->getLength(); i++) {
		  target[i] = 0;
		}
	      }
	      if (DFS2REC::isSQLVarChar(sourceType)) { 
                for (i = sourceLen; i < operand->getLength(); i++) {
		  target[i] = 0;
		}
	      }
	    } 
           
            // Check length limit for KANJI/KSC5601/UCS2 ANSI VARCHAR (i.e., there 
            // should be a NAWchar typed NULL somewhere in the range [1, sourceLen/2]
            // in the source. We have to do this for KANJI/KSC because their FS type
            // codes are shared with the ISO88591 CHARACTER types and the conversion 
            // routines for ISO88591 ANSI VARCHAR in convDoIt() can not verify the 
            // NAWchar nulls. 
            // 
            // We also check the ANSI Unicode source here because we then can safely 
            // use the NAWstrlen() function to obtain the length in the next IF block to 
            // check code points. 
            if ((sourceType == REC_BYTE_V_ANSI && CharInfo::is_NCHAR_MP(sourceCharSet)) ||
                sourceType == REC_NCHAR_V_ANSI_UNICODE  
               )
            {
               Lng32 i = 0;
               Int32 sourceLenInWchar = sourceLen/SQL_DBCHAR_SIZE;
               NAWchar* sourceInWchar = (NAWchar*)source;
               while ((i < sourceLenInWchar) && (sourceInWchar[i] != 0))
                 i++;

               if ( i == sourceLenInWchar ) {
                 ExRaiseSqlError(heap, &diagsArea, EXE_MISSING_NULL_TERMINATOR);

		 if (diagsArea != atp->getDiagsArea())
		   atp->setDiagsArea(diagsArea);	    
		
		 setRowNumberInCli(diagsArea, RowNum, targetRowsetSize) ;
                 if (tolerateNonFatalError && diagsArea->canAcceptMoreErrors())
		  continue;
		 else
		  return ex_expr::EXPR_ERROR;
               };
            }


            if ( DFS2REC::isAnyCharacter(sourceType) &&
                 sourceCharSet == CharInfo::UNICODE ) 
            {

               Lng32 realSourceLen = sourceLen;

               if (sourceType == REC_NCHAR_V_ANSI_UNICODE)  {
                  realSourceLen = NAWstrlen((NAWchar*)source) * SQL_DBCHAR_SIZE;

                  if ( realSourceLen > sourceLen )
                     realSourceLen = sourceLen ;
               }
               realSourceLen /= SQL_DBCHAR_SIZE;

               if ( CharInfo::checkCodePoint((NAWchar*)source, realSourceLen,
                                          CharInfo::UNICODE ) == FALSE )
               {
                 // Error code 3400 falls in CLI error code area, but it is
                 // a perfect fit to use here (as an exeutor error).
                 ExRaiseSqlError(heap, &diagsArea, (ExeErrorCode)3400);

                 if (diagsArea != atp->getDiagsArea())
                   atp->setDiagsArea(diagsArea);

                 *diagsArea << DgString0(SQLCHARSETSTRING_UNICODE);

		 setRowNumberInCli(diagsArea, RowNum, targetRowsetSize) ;
                 if (tolerateNonFatalError && diagsArea->canAcceptMoreErrors())
		  continue;
		 else
		  return ex_expr::EXPR_ERROR;
               }
            }
	    
	    // if incompatible conversions are to be done,
	    // and the source is REC_BYTE_V_ANSI, 
	    // and the target is not a string type, convert the source to 
	    // REC_BYTE_F_ASCII first. Conversion from V_ANSI is currently
	    // only supported if target is a string type.
	    if ((implicitConversion) &&
		((sourceType == REC_BYTE_V_ANSI) &&
		 ((targetType < REC_MIN_CHARACTER) ||
		  (targetType > REC_MAX_CHARACTER))))
	      {
		intermediate = new (heap) char[sourceLen];

                // Initialize index for this convDoIt call, if needed
                if (index1 == CONV_UNKNOWN) {
                  ex_conv_clause tempClause;
                  index1 = tempClause.findInstruction(sourceType,
                                                      sourceLen,
                                                      REC_BYTE_F_ASCII,
                                                      sourceLen,
                                                      0);
                }

		if (::convDoIt(source,
			       sourceLen,
			       sourceType,
			       sourcePrecision,
			       sourceScale,
			       intermediate,
			       sourceLen,
			       REC_BYTE_F_ASCII,
			       sourcePrecision,
			       sourceScale,
			       NULL,
			       0,
			       heap,
			       &diagsArea,
                               index1) !=  ex_expr::EXPR_OK) {
		  if (diagsArea != atp->getDiagsArea())
		    atp->setDiagsArea(diagsArea);            
		  
		  NADELETEBASIC(intermediate, heap);
		  setRowNumberInCli(diagsArea, RowNum, targetRowsetSize) ;
		  if (tolerateNonFatalError && diagsArea->canAcceptMoreErrors())
		    continue;
		  else
		    return ex_expr::EXPR_ERROR;
		};
		
		sourceType = REC_BYTE_F_ASCII;
		source = intermediate;
	      } // sourceType == REC_BYTE_V_ANSI
	    
	    if ((sourcePrecision > 0) &&
		((sourceType >= REC_MIN_BINARY_NUMERIC) &&
                 (sourceType <= REC_MAX_BINARY_NUMERIC) &&
                 (sourceType == operand->getDatatype()) &&
                 (sourcePrecision == operand->getPrecision()) ||
                 (DFS2REC::isAnyCharacter(sourceType) && !suppressCharLimitCheck())))
	      {
		// Validate source precision. Make the source precision
		// zero, that will trigger the validation. See method
		// checkPrecision in exp_conv.cpp. This also applies to
                // source chars or varchars, where we don't trust the
                // precision (max number of chars) in the value
		sourcePrecision = 0;
	      }

	    ULng32 convFlags = 0;
	    if ((NOT implicitConversion) &&
		(((sourceType == REC_DATETIME) ||
		  ((sourceType >= REC_MIN_INTERVAL) &&
		   (sourceType <= REC_MAX_INTERVAL))) &&
		 (NOT isIFIO)))
	      {
		// this is a datetime or interval conversion.
		// Convert the external string datetime/interval to its
		// internal format. The conversion follows the
		// same rules as "CAST(char TO datetime/interval)".
		
		// If the two interval types are not the same, then first
		// convert source from its string to its internal format,
		// and then convert to the target interval type.
		if (sourceType == REC_DATETIME)
		  {
		    if ((sourcePrecision != operand->getPrecision()) ||
			(sourceScale != operand->getScale()))
		      {
			// source and target must have the same datetime_code,
			// they must both be date, time or timestamp. 
			ExRaiseSqlError(heap, &diagsArea,
					EXE_CONVERT_NOT_SUPPORTED);
			if (diagsArea != atp->getDiagsArea())
			  atp->setDiagsArea(diagsArea);	  
			
			setRowNumberInCli(diagsArea, RowNum, targetRowsetSize) ;
			if (tolerateNonFatalError && diagsArea->canAcceptMoreErrors())
			  continue;
			else
			  return ex_expr::EXPR_ERROR;
		      }
		    else
                      {
                        sourceType = REC_BYTE_F_ASCII;
                        sourcePrecision = 0;
                        sourceScale = SQLCHARSETCODE_ISO88591; // assume target charset is ASCII-compatible

                        convFlags |= CONV_NO_HADOOP_DATE_FIX;
                      }
		  }
		else if ((sourceType == targetType) &&
			 (sourcePrecision == operand->getPrecision()) &&
			 (sourceScale == operand->getScale()))
		  {
		    // 'interval' source and target match all attributes.
		    // Convert source(string external) to target(internal) format
		    sourceType = REC_BYTE_F_ASCII;
                    sourcePrecision = 0;  // TBD $$$$ add source max chars later
                    sourceScale = SQLCHARSETCODE_ISO88591; // assume target charset is ASCII-compatible
		    
		    convFlags |= CONV_ALLOW_SIGN_IN_INTERVAL;
		  }
		else
		  {
		    // interval type and source/target do not match.
		    // Convert source string interval to corresponding
		    // internal format.
		    short intermediateLen = SQL_LARGE_SIZE;
		    intermediate = new (heap) char[intermediateLen];
		    convFlags |= CONV_ALLOW_SIGN_IN_INTERVAL;

                    // Initialize index for this convDoIt call, if needed
                    if (index2 == CONV_UNKNOWN) {
                      ex_conv_clause tempClause;
                      index2 = tempClause.findInstruction(REC_BYTE_F_ASCII,
                                                          sourceLen,
                                                          sourceType,
                                                          intermediateLen,
                                                          0 - sourceScale);
                    }

		    if (noDatetimeValidation())
		      convFlags |= CONV_NO_DATETIME_VALIDATION;
		      
		    if (::convDoIt(source,
				   sourceLen,
				   REC_BYTE_F_ASCII,
				   0, //sourcePrecision,
				   0, //sourceScale,
				   intermediate,
				   intermediateLen,
				   sourceType,
				   sourcePrecision,
				   sourceScale,
				   NULL,
				   0,
				   heap,
				   &diagsArea,
				   index2,
				   0,
				   convFlags) !=  ex_expr::EXPR_OK) 
		      {
			if (diagsArea != atp->getDiagsArea())
			  atp->setDiagsArea(diagsArea);            
			
			NADELETEBASIC(intermediate, heap);
			setRowNumberInCli(diagsArea, RowNum, targetRowsetSize) ;
			if (tolerateNonFatalError && diagsArea->canAcceptMoreErrors())
			  continue;
			else
			  return ex_expr::EXPR_ERROR;
		      };

		    source = intermediate;
		    sourceLen = intermediateLen;
		  }
	      }

            // Initialize index for this convDoIt call, if needed
            if (index3 == CONV_UNKNOWN) {
              ex_conv_clause tempClause;
              index3 = tempClause.findInstruction(sourceType,
                                                  sourceLen,
                                                  operand->getDatatype(),
                                                  operand->getLength(),
                                                  sourceScale -
                                                    operand->getScale());
            }

	    if (noDatetimeValidation())
	      convFlags |= CONV_NO_DATETIME_VALIDATION;
            if ((sourceType==REC_BLOB) || (sourceType ==REC_CLOB))
	    {
	      // the first 4 bytes of data are actually the variable 
	      // length indicator
	      Lng32 VCLen;
	      str_cpy_all((char *) &VCLen, source, sizeof(Int32));
	      sourceLen = (Lng32) VCLen;
	      source = &source[sizeof(Int32)];
	    }
            retcode = ::convDoIt(source,
				 sourceLen,
				 sourceType,
				 sourcePrecision,
				 sourceScale,
				 target,
				 operand->getLength(),
				 operand->getDatatype(),
				 operand->getPrecision(),
				 operand->getScale(),
				 targetVCLenInd,
				 operand->getVCIndicatorLength(),
				 heap,
				 &diagsArea,
				 index3,
				 0,
				 convFlags);

	    // NADELETEBASIC(intermediate, heap);
	    if (retcode != ex_expr::EXPR_OK)
	      {
		if (diagsArea != atp->getDiagsArea())
		  atp->setDiagsArea(diagsArea);	    
		
		setRowNumberInCli(diagsArea, RowNum, targetRowsetSize) ;
		if (tolerateNonFatalError && diagsArea->canAcceptMoreErrors())
		  continue;
		else
		  return ex_expr::EXPR_ERROR;
	      }
	    else if (diagsArea && diagsArea->mainSQLCODE() == EXE_STRING_OVERFLOW)
	      {
		Int32 warningMark2 = diagsArea->getNumber(DgSqlCode::WARNING_);
		Int32 counter = warningMark2 - warningMark;
		
		diagsArea->deleteWarning(warningMark2-counter);
		ExRaiseSqlError(heap, &diagsArea, EXE_STRING_OVERFLOW);
		
		if (diagsArea != atp->getDiagsArea())
		  atp->setDiagsArea(diagsArea);	

		setRowNumberInCli(diagsArea, RowNum, targetRowsetSize) ;
		if (tolerateNonFatalError && diagsArea->canAcceptMoreErrors())
		  continue;
		else
		  return ex_expr::EXPR_ERROR;
	      }

	    // If the query contains ROWSET FOR INPUT SIZE <var> or
	    // ROWSET <var> ( <list> ) then we get the number of entries 
	    // from <var>
            if (operand->getHVRowsetForInputSize() || 
	        operand->getHVRowsetLocalSize()) {
              short targetType = operand->getDatatype();
              switch (targetType) {
              case REC_BIN8_SIGNED :
                if (*((Int8 *) target) <= 0) {
                  //raise error
                  ExRaiseSqlError(heap, &diagsArea, EXE_ROWSET_NEGATIVE_SIZE);
		  if (diagsArea != atp->getDiagsArea()) 
		    atp->setDiagsArea(diagsArea);	    
                  return ex_expr::EXPR_ERROR;
                }
                else {
		  dynamicRowsetSize = *((Int8 *) target);
		  break;
                }
              case REC_BIN8_UNSIGNED :
                if (*((UInt8 *) target) == 0) {
                  //raise error
                  ExRaiseSqlError(heap, &diagsArea, EXE_ROWSET_NEGATIVE_SIZE);
		  if (diagsArea != atp->getDiagsArea()) 
		    atp->setDiagsArea(diagsArea);	    
                  return ex_expr::EXPR_ERROR;
                }
                else {
		  dynamicRowsetSize = *((UInt8 *) target);
		  break;
                }
		
              case REC_BIN16_SIGNED :
                if (*((short *) target) <= 0) {
                  //raise error
                  ExRaiseSqlError(heap, &diagsArea, EXE_ROWSET_NEGATIVE_SIZE);
		  if (diagsArea != atp->getDiagsArea()) 
		    atp->setDiagsArea(diagsArea);	    
                  return ex_expr::EXPR_ERROR;
                }
                else {
		  dynamicRowsetSize = *((short *) target);
		  break;
                }
              case REC_BIN16_UNSIGNED :
                if (*((unsigned short *) target) == 0) {
                  //raise error
                  ExRaiseSqlError(heap, &diagsArea, EXE_ROWSET_NEGATIVE_SIZE);
		  if (diagsArea != atp->getDiagsArea()) 
		    atp->setDiagsArea(diagsArea);	    
                  return ex_expr::EXPR_ERROR;
                }
                else {
		  dynamicRowsetSize = *((unsigned short *) target);
		  break;
                }
		
              case REC_BIN32_SIGNED :
                if (*((Lng32 *) target) <= 0) {
                  //raise error
                  ExRaiseSqlError(heap, &diagsArea, EXE_ROWSET_NEGATIVE_SIZE);
		  if (diagsArea != atp->getDiagsArea()) 
		    atp->setDiagsArea(diagsArea);	    
                  return ex_expr::EXPR_ERROR;
                }
                else {
		  dynamicRowsetSize = *((Lng32 *) target);         //long is the same as int
		  break;
                }
		
              case REC_BIN32_UNSIGNED :
                if (*((ULng32 *) target) == 0) {   // unsigned long is the same as unsigned int
                  //raise error
                  ExRaiseSqlError(heap, &diagsArea, EXE_ROWSET_NEGATIVE_SIZE);
		  if (diagsArea != atp->getDiagsArea()) 
		    atp->setDiagsArea(diagsArea);	    
                  return ex_expr::EXPR_ERROR;
                }
                else {
		  dynamicRowsetSize = *((ULng32 *) target);
		  break;
                }
                
              default:
                //raise error
                ExRaiseSqlError(heap, &diagsArea,EXE_ROWSET_WRONG_SIZETYPE);
		if (diagsArea != atp->getDiagsArea()) 
		  atp->setDiagsArea(diagsArea);	    
                return ex_expr::EXPR_ERROR;
              }
	    }
	    
            // up- or down-scale target, if there is a difference
            // in scale, but be careful to ignore scale used as charset
            if ((operand->getDatatype() >= REC_MIN_NUMERIC) &&
                (operand->getDatatype() <= REC_MAX_NUMERIC) &&
                (sourceScale != operand->getScale()) &&
                ::scaleDoIt(target,
                            operand->getLength(),
                            operand->getDatatype(),
                            sourceScale,
                            operand->getScale(),
                            sourceType,
                            heap) != ex_expr::EXPR_OK)
	      {
		ExRaiseSqlError(heap, &diagsArea, EXE_NUMERIC_OVERFLOW);
		if (diagsArea != atp->getDiagsArea())
		  atp->setDiagsArea(diagsArea);	    
		
		setRowNumberInCli(diagsArea, RowNum, targetRowsetSize) ;
		if (tolerateNonFatalError && diagsArea->canAcceptMoreErrors())
		  continue;
		else
		  return ex_expr::EXPR_ERROR;
	      }
	    } // if (! nullMoved)
          } // loop over rowset elements (RowNum)
        } // if clause == IN_OUT type
    
next_clause:
    clause = clause->getNextClause();
  }  // while(clause)
  
  return ex_expr::EXPR_OK;
}

Lng32 InputOutputExpr::getCompiledOutputRowsetSize(atp_struct *atp)
{
  ex_clause  * clause = getClauses();
  Attributes * operand;
  Lng32   sourceRowsetSize = 0;

  // Note that every output operands must participate in rowsets, or none of
  // them. That is, you cannot mix rowset host variables and simple host
  // variables for output purposes (INTO clause).

  if (clause) {
    operand = clause->getOperand(0);
    sourceRowsetSize = operand->getRowsetSize();
  }

  return sourceRowsetSize;
}

ex_expr::exp_return_type InputOutputExpr::addDescInfoIntoStaticDesc
(Descriptor * desc, NABoolean isInput)
{
  ex_clause *clause = getClauses();
  short entry = 0;

  if(isCall() && desc->isDescTypeWide()){
    while(clause){
      if(clause->getType() == ex_clause::INOUT_TYPE){
	entry = ((ex_inout_clause *)clause)->getParamIdx();
	if(entry > desc->getUsedEntryCount())
	  continue;
	desc->setDescItem(entry, SQLDESC_NAME, 0, 
	  ((ex_inout_clause *)clause)->getName());
	short paramMode = ((ex_inout_clause *)clause)->getParamMode();
	short ordPos = ((ex_inout_clause *)clause)->getOrdPos();
	desc->setDescItem(entry, SQLDESC_PARAMETER_MODE, paramMode, 0);
	desc->setDescItem(entry, SQLDESC_PARAMETER_INDEX, entry, 0);
	desc->setDescItem(entry, SQLDESC_ORDINAL_POSITION, ordPos, 0);
      }
	clause = clause->getNextClause();
    }
  }
  else if(isCall() && !desc->isDescTypeWide()){
    while (clause && entry < desc->getUsedEntryCount()){
      if (clause->getType() == ex_clause::INOUT_TYPE){
	entry++;
	desc->setDescItem(entry, SQLDESC_NAME, 0, 
			  ((ex_inout_clause *)clause)->getName());
	short paramMode = ((ex_inout_clause *)clause)->getParamMode();
	short paramIdx = ((ex_inout_clause *)clause)->getParamIdx();
	short ordPos = ((ex_inout_clause *)clause)->getOrdPos();
	desc->setDescItem(entry, SQLDESC_PARAMETER_MODE, paramMode, 0);
	desc->setDescItem(entry, SQLDESC_PARAMETER_INDEX, paramIdx, 0);
	desc->setDescItem(entry, SQLDESC_ORDINAL_POSITION, ordPos, 0);
      }
      clause = clause->getNextClause();
    }
  }
  else{
    while (clause && entry < desc->getUsedEntryCount()){
      if (clause->getType() == ex_clause::INOUT_TYPE){
	entry++;
	desc->setDescItem(entry, SQLDESC_NAME, 0, 
			  ((ex_inout_clause *)clause)->getName());
	if(isInput)
	  desc->setDescItem(entry, SQLDESC_PARAMETER_MODE,
			    PARAMETER_MODE_IN, 0);
	else
	  desc->setDescItem(entry, SQLDESC_PARAMETER_MODE,
			    PARAMETER_MODE_OUT, 0);
	desc->setDescItem(entry, SQLDESC_PARAMETER_INDEX, entry, 0);
	desc->setDescItem(entry, SQLDESC_ORDINAL_POSITION, 0, 0);
      }
      clause = clause->getNextClause();
    }
  }
  
  return ex_expr::EXPR_OK;
}

Lng32 InputOutputExpr::getMaxParamIdx()
{
  Lng32 maxParamIdx = 0;
  ex_clause *clause = getClauses();
  while(clause){
    if(clause->getType() == ex_clause::INOUT_TYPE){
      Lng32 paramIdx = ((ex_inout_clause *)clause)->getParamIdx();
      if(paramIdx > maxParamIdx)
	maxParamIdx = paramIdx;
    }
    clause = clause->getNextClause();
  }
  return maxParamIdx;
}










