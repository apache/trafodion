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
 * File:         exp_eval.cpp
 * Description:  Expression evaluator, makes operands accessible, calls
 *               NULL processing methods and/or evaluation methods on clauses
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



#include "exp_stdh.h"
#include "ExpAtp.h"
#include "exp_clause_derived.h"
#include "ExpPCode.h"
#include "exp_function.h"
#include "ComSysUtils.h"
#include "exp_bignum.h"
#include "BigNumHelper.h"
#include "ExpPCodeOptimizations.h"
#include "unicode_char_set.h"
#include "wstr.h"

#include <unistd.h>
#include <sys/mman.h>
#include <fenv.h>  // floating point environment stuff

#include "exp_ovfl_ptal.h"

#include "exp_ieee.h"
double MathConvReal64ToReal64(double op1, Int16 * ov);
#include <float.h>      /* nolist */
#include <limits.h>     /* nolist */

// 3 bit left circular shift op1, and xor with op2.
// Both op1 and op2 are UInt32.
#define EXP_SHIFT_XOR(op1, op2) \
  ( ( ((op1) << 3) | ((op1) >> 29) ) ^ (op2) ) 

// Clear the sign bit and toggle one other bit accordingly.
// result is UInt32
#define EXP_CLEAR_SIGNBIT(result) \
  ( ((result) & 0x80000000) ? ((result) ^ 0x80000002) : (result) )

// macros for branch prediction hints in gcc
#define LIKELY(x)       __builtin_expect((x),1)
#define UNLIKELY(x)     __builtin_expect((x),0)


void setVCLength(char * VCLen, Int32 VCLenSize, UInt32 value);

// --------------------------------------------------------------------
// Generate 31 bit hash value by
// multiplication, shift, and xor for every 4 byte.
// --------------------------------------------------------------------
UInt32 FastHash(char *data, Int32 length) 
{
  // FastHash should be removed someday
  assert( 0 /* "FastHash should not be used!" */ );

// scramble factors.  
#define MULT_FACTOR 0x97
#define SCRAMBLE_0 MULT_FACTOR
#define SCRAMBLE_1 0x2d5
#define SCRAMBLE_2 0x1d
#define SCRAMBLE_3 0x2ec3
 
  UInt32 result;   

  if (length == 4)
    {
      // Keep this logic in sync with case PCIT::HASH_MBIN32U_MBIN32_IBIN32S
      // in ex_expr_base::evalPCode.
      return *(UInt32 *)data;
    }
  else if (length == 2)
    {
      // Keep this logic in sync with case PCIT::HASH_MBIN32U_MBIN16_IBIN32S
      // in ex_expr_base::evalPCode.
      return (UInt32) (*(UInt16 *)data);
    }

  register UInt32 *source = (UInt32 *)data;
  register UInt32 z;

  if (length == 8)
    { 
#ifdef NA_LITTLE_ENDIAN
      z = *source * MULT_FACTOR;   
      result = source[1] * SCRAMBLE_1;
#else
      z = source[1] * MULT_FACTOR;
      result = *source * SCRAMBLE_1;
#endif
      result = EXP_SHIFT_XOR(result, z); 	
      return EXP_CLEAR_SIGNBIT(result);
    }

  result = 0;
  while (length >= 4)
    { // longer than 4 bytes and not 8 bytes
      z = (*source++) * SCRAMBLE_0;
      result = EXP_SHIFT_XOR(result, z); 	   
      length -= 4;
      if (length < 4) break;

      z = (*source++) * SCRAMBLE_1;
      result = EXP_SHIFT_XOR(result, z); 	   
      length -= 4;
      if (length < 4) break;

      z = (*source++) * SCRAMBLE_2;
      result = EXP_SHIFT_XOR(result, z); 	   
      length -= 4;
      if (length < 4) break;

      z = (*source++) * SCRAMBLE_3;
      result = EXP_SHIFT_XOR(result, z); 	   
      length -= 4;
    }

  if (length == 0)
    return EXP_CLEAR_SIGNBIT(result);

  // Copy the remaining 1, 2, or 3 bytes to a Int32.
  if (length == 1)
    z = *((unsigned char *)source);
  else {
    z = *((UInt16 *)source);
    if (length == 3) 
      z = (z << 8) + ((unsigned char *)source)[2];
  }
  z *= MULT_FACTOR;
  result = EXP_SHIFT_XOR(result, z); 
  return EXP_CLEAR_SIGNBIT(result);
}

// This static array is used when a default value is not null and the 
// attribute data format is SQLMX_ALIGNED_FORMAT.
// The default value is stored in SQLMX_FORMAT always and thus there
// is an impedence mismatch between the default value format and the
// disk format it is to be used for.
// This array is 16 bytes long and can accomodate 64 added columns
// that do have a default value.
// If the added column has no default or a default of null, then this
// the nulldata pointer is set to NULL and this static array is not used.
//
static const Int32 nullBitmapData[] = { 0, 0, 0, 0 };

ex_expr::exp_return_type getDefaultValueForAddedColumn(
     UInt32 field_num, // field number whose default value is to be
                              // computed. Zero based.
     ExpTupleDesc * td,       // describes this row
     Attributes * op,
     char ** opdata,
     char ** nulldata,
     char ** varlendata,
     CollHeap * heap,
     ComDiagsArea **diagsArea) 
{
  char * defaultValue = td->getAttr((Int16)field_num)->getDefaultValue();
  Attributes::DefaultClass dc =
                           td->getAttr((Int16)field_num)->getDefaultClass();

  //
  // Default values are stored in the packed SQLMX_FORMAT.
  // This may be a different format than the attribute tuple data format.

  if ((dc == Attributes::NO_DEFAULT) ||
      ((dc == Attributes::DEFAULT_NULL) &&
       (op->getNullFlag() == 0)))
    {
      ExRaiseSqlError(heap, diagsArea, EXE_DEFAULT_VALUE_ERROR);
      return ex_expr::EXPR_ERROR;
    }

  if (op->getNullFlag())
  {
    *nulldata = defaultValue;

    if (dc == Attributes::DEFAULT_NULL)
	*nulldata = NULL;
    else if ( op->isSQLMXAlignedFormat() )
    {
      if ( ExpTupleDesc::isNullValue( defaultValue,
                                      op->getNullBitIndex(),
                                      ExpTupleDesc::SQLMX_FORMAT ) )
        *nulldata = NULL;
      else
        *nulldata = (char *)nullBitmapData;
    }
  
    // Can't use the attribute field since the attribute describes the
    // disk field and not the default value itself.
    // op->getNullIndicatorLength();
    defaultValue += ExpTupleDesc::NULL_INDICATOR_LENGTH;
  }
  
  if (op->getVCIndicatorLength() > 0)
    *varlendata = defaultValue;

  *opdata = defaultValue + op->getVCIndicatorLength();

  return ex_expr::EXPR_OK;
}

/////////////////////////////////////////////////////////
// Computes the address of the field_num field. This
// field follows one (or more) varchar fields in a row
// with old format. This is true only for tables created
// in SQL/MP. In this format, all fields
// are stored next to each other in packed format(varchar
// fields are stored without blank padding).
/////////////////////////////////////////////////////////
void computeDataPtr(char * start_data_ptr, // start of data row
		    Int32 field_num,       // field number whose address is to be
		                           // computed. Zero based.
		    ExpTupleDesc * td,     // describes this row
		    char ** opdata,
		    char ** nulldata,
		    char ** varlendata) 
{
  if (! start_data_ptr)
    {
      if (nulldata)
	*nulldata = 0;
      return;
    }

  AttributesPtr * op = td->attrs();
  char * data_ptr = start_data_ptr;
  for (Int32 i = 0; i < field_num; i++, op++)
    {
      data_ptr += ((*op)->getNullFlag() ? 
		   (*op)->getNullIndicatorLength() : 0);
      data_ptr += (*op)->getVCIndicatorLength() + 
	(*op)->getLength(data_ptr);
      
    }
  
  if (((*op)->getNullFlag()) && (nulldata))
    *nulldata = data_ptr;
  
  if (((*op)->getVCIndicatorLength() > 0) && (varlendata))
    *varlendata = data_ptr + (*op)->getNullIndicatorLength();
  
  if (opdata)
    *opdata = data_ptr + (*op)->getNullIndicatorLength() + 
      (*op)->getVCIndicatorLength();
}

// Evaluate one or more clauses from an expression
// -- Helper for PCode abstract machine
//
ex_expr::exp_return_type ex_expr::evalClauses(ex_clause *clause,
					      atp_struct *atp1, 
					      atp_struct *atp2,
					      Int32 datalen,
					      UInt32 *rowLen,
					      Int16 *lastFldIndex,
					      char * fetchedDataPtr) {
  atp_struct *atps[] = {atp1, atp2};

  // a consecutive regions of ex_clause objects (actually subclasses of
  // ex_clause objects), linked by pointers

  // allocated space on the stack for pointers to point to null indicator,
  // var len indicator and actual data. There could be a maximum of
  // MAX_OPERANDS with the first operand of each group being the result.
  // The array contains first MAX_OPERANDS entries for null indicators,
  // the next MAX_OPERANDS for vclen and the last ones for actual data.
  char *op_data[ex_clause::MAX_OPERANDS * 3];

  // allocated space for temporary, aligned operands on the stack
  // (assuming that we have no more than 4 operands and that no operand
  // using the temp space is longer than 64 bytes).
  // Using buffer of type Int64 so it starts on an 8-byte aligned offset.
  Int64 temp_buffer[ex_clause::MAX_OPERANDS][ex_clause::MAX_TEMP_OPERAND_LEN/sizeof(Int64)];

  ex_expr::exp_return_type retcode = ex_expr::EXPR_OK;

  // these next 8 locals are for SQLMX tuple format only
  UInt32      varOffset = ExpOffsetMax,
              voaOffset = 0,
              offset    = 0,
              ff;
  Int16       varIndLen;
  Int16       nullIndLen;
  NABoolean   nullable  = FALSE;
  Attributes *currAttr  = NULL;
  Attributes *prevAttr  = NULL;

  NABoolean     setLastFldIndex = FALSE;
  NABoolean     varOpFound = FALSE;
  ExpTupleDesc *tuppDesc;

  ex_expr::exp_mode mode = ex_expr::exp_UNKNOWN;

  // ---------------------------------------------------------------------
  // Loop over clauses of the expression
  // ---------------------------------------------------------------------
  while (clause)
    {
      // an array of pointers to Attributes data structures, describing
      // the data types of the operands of this expression (op[0] points
      // to the result type)
      register AttributesPtr* op = clause->getOperand();
    
      // pointer to the null indicator data for current operand
      register char ** nulldata = &op_data[0];

      // pointer to the varlen indicator for current operand
      register char ** vardata = &op_data[ex_clause::MAX_OPERANDS];

      // pointer to data for current operand
      register char ** opdata = &op_data[2 * ex_clause::MAX_OPERANDS];

      // a pointer to the actual result buffer of the expression (used in
      // case the result is calculated in the temp buffer)
      char * result_data = NULL;
      
      // initialize the op_data array to all nulls
      //   for *every* clause!
      str_pad((char*)op_data, sizeof(op_data), '\0');

      // reset the variable operand
      currAttr = NULL;

      // for each operand, incrementing i, op, and opdata
      Int16 numOperands = clause->getNumOperands();
      for (Int16 i = 0; i < numOperands;
	   i++, op++, opdata++, nulldata++, vardata++)
	{
	  // -------------------------------------------------------------
	  // get pointer to operand from its ATP, ATPIndex, and offset
	  // value
	  // -------------------------------------------------------------

	  const UInt16 atpix = (*op)->getAtpIndex();
	  const UInt16 atpnum = (*op)->getAtp();

	  if((atpix < 2) && (NOT atps[0]->getTupp(atpix).isAllocated()))
	    {
	      if(!getFixupConstsAndTemps())
		{
		  char *base;
		  if(atpix == 0) base = getConstantsArea();
		  else if(atpnum == 0) base = getTempsArea();
		  else base = getPersistentArea();

		  *opdata = base + (*op)->getOffset();
		  *nulldata = base + (*op)->getNullIndOffset();
		  *vardata = base + (*op)->getVCLenIndOffset();
		}
	      else
		{
		  *opdata = (char*)((long)((*op)->getOffset()));
		  *nulldata = (char*)((long)((*op)->getNullIndOffset()));
		  *vardata = (char*)((long)((*op)->getVCLenIndOffset()));
		}
	    }
	  else
            {
		mode = (i == 0 ? exp_WRITE : exp_READ);

		// all other ATPIndex values indicate to use tupp
		// specified by ATPIndex from the ATP specifed in atp
		register atp_struct *atp;
		
		// which atp is to be used?
		atp = atps[atpnum];

                tuppDesc = atp->getCriDesc()->getTupleDescriptor(atpix);
		
		register char * dataPtr =
		                   (atp->getTupp(atpix)).getDataPointer();

		switch( (*op)->getTupleFormat() )
		  {
		  case ExpTupleDesc::SQLMX_KEY_FORMAT:
		  case ExpTupleDesc::PACKED_FORMAT:
		    {
		      // if this is a special field (either a field following
		      // a varchar field, or a missing field), then compute
		      // its address or value.
		      // If offset is ExpOffsetMax, then this field follows a
		      // varchar field.
		      // If the offset for a non-varchar field is >= the
		      // datalen of this record, then this is a missing
		      // field.
		      // For a varchar field, if the offset is equal to the
		      // datalen, then this varchar field either is a null
		      // value or has a length of zero.
		      // A varchar field is missing if its offset is > datalen.
		      if (((*op)->isAddedCol()) &&
			  (((*op)->getOffset() == ExpOffsetMax) ||
			   ((datalen > 0) &&
			    ((((*op)->getVCIndicatorLength() > 0) &&
			      ((*op)->getOffset() > (UInt32)datalen)) ||
			     (((*op)->getVCIndicatorLength() <= 0) &&
			      ((*op)->getOffset() >= (UInt32)datalen))))))
			{
			  NABoolean defValNeeded = FALSE;
		    
			  if ((*op)->getOffset() == ExpOffsetMax)
			    {
			      // Offset is max value thus read the field number
			      // from the relative offset field.
			      // This signifies that this field follows one or
                              // more varchar fields.
                              // True for SQL/MP tables only.
			      // Needs special logic to compute the actual
			      // address of this field.
			      computeDataPtr(dataPtr, 
					     (Int32)(*op)->getRelOffset(),
                                             tuppDesc,
					     &(*opdata),
					     &(*nulldata),
					     &(*vardata));
	
                              if ((mode == exp_WRITE) && (rowLen))
                              {
                                 currAttr = *op;
                                 varOffset = (*opdata - dataPtr);
                              }

			      // if *opdata is == the end, but vardata is set
			      // then the variable field is length 0 so no
			      // default needed
			      if ((datalen > 0 ) &&
				  ((*opdata > (dataPtr + datalen)) ||
				   ((*opdata == (dataPtr + datalen)) &&
				    (*vardata == NULL))))
				defValNeeded = TRUE;
			    }
			  else
			    defValNeeded = TRUE;
		      
   			  if ((defValNeeded) && (mode == exp_READ))
			    {
			      ComDiagsArea *diagsArea = atp1->getDiagsArea();

			      retcode = 
				getDefaultValueForAddedColumn((*op)->getDefaultFieldNum(),
							      tuppDesc,
							      *op,
							      &(*opdata),
							      &(*nulldata),
							      &(*vardata),
							      getHeap(),
							      &diagsArea);

			      if (retcode == ex_expr::EXPR_ERROR)
				{
				  if (diagsArea != atp1->getDiagsArea())
				    atp1->setDiagsArea(diagsArea);
			    
				  return retcode;
				}
			    }

			  // ----------------------------------------------
			  // For SQL/MP tables, evalClauses returns the 
			  // index of the last field in the fetched record.
			  // This is used by update processing logic to set the
			  // lastFldixOldrec fld in the ModifiedFieldMap struct,
			  // for use by DP2 on backout logic. lastFldIndex is set
			  // to the index of the last column for which a value 
			  // exists in the fetched record. 
			  //----------------------------------------------
			  if ((lastFldIndex != NULL) &&
			      (! setLastFldIndex)    && // last index not set before
			      (mode == exp_WRITE)    && // result operand
			      (tuppDesc->addedFieldPresent()))
			    {
			      // see if this column is present in the fetched
			      // row.
			      char * fetchedOpPtr;
			      computeDataPtr(fetchedDataPtr, 
					     (Int32)(*op)->getDefaultFieldNum(),
					     tuppDesc,
					     &fetchedOpPtr, NULL, NULL);
			      if (fetchedOpPtr >= (fetchedDataPtr + datalen))
				{
				  // (*op)->getDefaultFieldNum is the index of
				  // the first missing column.
				  *lastFldIndex = (Int16)((*op)->getDefaultFieldNum() - 1);
				  setLastFldIndex = TRUE;
				}
			    }
			}
		      else  // not a 'special' attribute
			{
                          if ((mode == exp_WRITE)    &&
                              (rowLen)               &&
                              (*op)->isSQLPackedFormat())
                          {	
                             varOffset = (*op)->getOffset();
                             currAttr = *op;
                          }

			  if (dataPtr)
			    {
			      *opdata = dataPtr + (*op)->getOffset();
			      if ((*op)->getNullFlag())   // nullable
				*nulldata = dataPtr + (*op)->getNullIndOffset();
			      if ((*op)->getVCIndicatorLength() > 0)
				*vardata = dataPtr + (*op)->getVCLenIndOffset();
			    }
			  else
			    {
			      // missing value. Indicates a null value.
			      *nulldata = 0;
			    }
			}
		    }  // SQLMX_KEY_FORMAT, PACKED_FORMAT
		  break;

		  case ExpTupleDesc::SQLARK_EXPLODED_FORMAT:
                  {
		    if (dataPtr)
		      {
			*opdata = dataPtr + (*op)->getOffset();
			if ((*op)->getNullFlag())   // nullable
			  *nulldata = dataPtr + (*op)->getNullIndOffset();
			if ((*op)->getVCIndicatorLength() > 0)
			  *vardata = dataPtr + (*op)->getVCLenIndOffset();
		      }
		    else
		      {
			// missing value. Indicates a null value.
			*nulldata = 0;
		      }
                  }
                  break;

                  case ExpTupleDesc::SQLMX_FORMAT:
                  case ExpTupleDesc::SQLMX_ALIGNED_FORMAT:
		    {
                      ExpTupleDesc::TupleDataFormat tdf = (*op)->getTupleFormat();

		      if (dataPtr)
			{
                          UInt32 nullOff = 0;
                          NABoolean isAlignedFormat =
                                       (*op)->isSQLMXAlignedFormat();
			  offset     = (*op)->getOffset();
			  nullable   = (*op)->getNullFlag();
                          nullIndLen = (*op)->getNullIndicatorLength();
			  varIndLen  = (*op)->getVCIndicatorLength();
			  voaOffset  = (*op)->getVoaOffset();

			  if (mode == exp_WRITE)
			    { 
			      if (offset == ExpOffsetMax)
                              {
                                  // all variable length fields after the
				  // first variable length field have their
				  // offsets initialized to ExpOffsetMax so
                                  // the actual size of the variable field value
                                  // is used to calculate the offset to be
                                  // written into VOA[i] for this variable field

				  offset = varOffset;
                                  ExpTupleDesc::setVoaValue(dataPtr, 
                                                            voaOffset, 
                                                            offset,
                                                            tdf);
				  if (nullable)
				    {
                                      nullOff = (isAlignedFormat
                                                 ? (*op)->getNullIndOffset()
                                                 : offset);

                                      *nulldata = dataPtr + nullOff;

				      offset += nullIndLen;
				    }
				  *vardata = dataPtr + offset;
				  offset  += varIndLen;

				  varOffset = offset;
				  currAttr  = *op;
				}
			      else // offsets computed at compile time
				{
				  if (varIndLen > 0)
				    {
				      *vardata  = dataPtr + (*op)->getVCLenIndOffset();
				      varOffset = offset;
				      currAttr     = *op;
				      varOpFound = TRUE;
				    }
				  else if ((! varOpFound)  &&
					   rowLen          &&
                                           isAlignedFormat &&
                                           ((offset >= varOffset) ||
                                            (varOffset == ExpOffsetMax)))
				  {
                                    // In aligned format, the fixed fields are
                                    // rearranged and ordered by byte alignment
                                    // and thus offsets are not guaranteed to
                                    // be increasing.
                                    // For variable fields this is not true.
                                    varOffset = offset;
                                    currAttr = *op;
                                  }

                                  // set the VOA entry offset to the start
                                  // of this fields data
                                  ExpTupleDesc::setVoaValue(
                                                  dataPtr,
						  voaOffset,
						  offset - varIndLen - nullIndLen,
                                                  tdf
                                                  );

				  if (nullable)
                                    *nulldata = dataPtr+(*op)->getNullIndOffset();
				}

			      *opdata = dataPtr + offset;
			    }
			  else  // read mode
			    {

                              // set the compile time null indicator offset here
                              // and it will be reset if added columns present
                              // or processing a varchar
                              *nulldata = dataPtr + (*op)->getNullIndOffset();
                              
                              // if there are added columns, then there exists
                              // a tuple descriptor
                              if (tuppDesc != NULL &&
                                  tuppDesc->addedFieldPresent() )
                              {

                                ff = ExpTupleDesc::getFirstFixedOffset( dataPtr,
                                                                        tdf );

                                if ( Attributes::isDefaultValueNeeded(
                                               (*op),
                                               tuppDesc,
                                               ff,
                                               varIndLen,
                                               voaOffset,
                                               dataPtr,
                                               datalen ) )
				{
				  ComDiagsArea *diagsArea = atp1->getDiagsArea();

                                  // this sets the *nulldata, *opdata, *vardata as needed				
                                  retcode =
				    getDefaultValueForAddedColumn
				    ((*op)->getDefaultFieldNum(),
                                     tuppDesc,
				     *op,
				     &(*opdata),
				     &(*nulldata),
				     &(*vardata),
				     getHeap(),
				     &diagsArea);

				  if (retcode == ex_expr::EXPR_ERROR)
				    {
				      if (diagsArea != atp1->getDiagsArea())
					atp1->setDiagsArea(diagsArea);
			    
				      return retcode;
				    }

                                  // all pointers are set now
                                  break;
				} 
			      else 
				{ // not an added column needing special attn.
                                  // but there may be other added columns present
                                  
                                  // if variable field, then read the offset from
                                  // the data record
                                  if ( varIndLen > 0 )
                                  {
                                    offset = ExpTupleDesc::getVoaOffset(dataPtr,
                                                                        voaOffset,
                                                                        tdf);
				  
                                  }
                                  else
                                  {
                                    // use the relative offset in case there are
                                    // added columns in the data row and fields
                                    // have shifted
                                    // Relative is always true based on first
                                    // fixed fields offset.
				    offset = ff + (*op)->getRelOffset();
                                  }
                                
                                  // For nullable fields, set the nulldata ptr
                                  // and bump the offset value past the null
                                  // indicator bytes.
				  if (nullable)
				  {
                                    *nulldata = dataPtr 
                                                + (isAlignedFormat 
                                                   ? ExpAlignedFormat::getBitmapOffset(dataPtr)
                                                   : offset);
          			    offset += nullIndLen;
				  }
                                } // need defaults or not
                              } // added col present or not

                              // No added col present.
                              // if variable field, read offset from data record
                              else if ( varIndLen > 0 )
                              {
                                offset = ExpTupleDesc::getVoaOffset(dataPtr,
                                                                    voaOffset,
                                                                    tdf);                               
                                if (nullable)
                                {
                                  // null indicator does not change for aligned format
                                  // but follows data for MX (packed) format
                                  *nulldata = (dataPtr 
                                               + (isAlignedFormat 
                                                 ? (*op)->getNullIndOffset()
                                                 : offset));
                                  offset += nullIndLen;
                                }
                              }

                              // offset now correctly set even if added columns.
                              // null indicator offset correct set too.

                              // For variable length fields, set the vardata
                              // and bump the offset value past the variable
                              // length bytes.
			      if (varIndLen > 0)
                              {
			        *vardata = dataPtr + offset;
				offset  += varIndLen;
			      }
				  
			      *opdata = dataPtr + offset;
                            } // else read mode
			}
		      else    // missing value (indicates a null value)
			*nulldata = 0;
			  
		    }  // SQLMX_FORMAT, SQLMX_ALIGNED_FORMAT
		  break;

		  default:
		    {
		      ComDiagsArea *diagsArea = atp1->getDiagsArea();
		      ExRaiseSqlError(getHeap(), &diagsArea, EXE_INTERNAL_ERROR);
		      if(diagsArea != atp1->getDiagsArea())
		        atp1->setDiagsArea(diagsArea);
		      return ex_expr::EXPR_ERROR;
		    }

		  }  // switch on atp tuple format
	      }  // default atp index case

	  // -------------------------------------------------------------
	  // If the operand is nullable, set its data pointer to 0 if
	  // its NULL indicator is set or set the NULL indicator to 0
	  // and advance the data pointer to the actual data. Don't do
	  // this for the first (result) operand at this time.
	  // -------------------------------------------------------------
	  if (((*op)->getNullFlag()) &&           // nullable and
	      (*nulldata)  &&                     // value not missing
              (i > 0))                            // not result
          {
            if ( ExpTupleDesc::isNullValue( *nulldata,
                                            (*op)->getNullBitIndex(),
                                            (*op)->getTupleFormat() ) )
            {
              (*nulldata) = 0;
            }
          }

	} // for
	  
      // -----------------------------------------------------------------
      // Call the NULL processing method for this clause and, if still
      // necessary, evaluate the expression. Note that if the
      // processNulls() method can determine the result, it will return
      // EXPR_NULL and the evaluation method is not called.
      // -----------------------------------------------------------------
      ComDiagsArea *diagsArea = atp1->getDiagsArea();
      retcode = clause->useProcessNulls()
	      ? clause->processNulls(op_data, getHeap(), &diagsArea)
              : ex_expr::EXPR_OK;

      if (retcode == ex_expr::EXPR_OK)				// do the work
	retcode = clause->eval(&op_data[2 * ex_clause::MAX_OPERANDS],
			       getHeap(),
			       &diagsArea);

      if (retcode == ex_expr::EXPR_OK &&
          clause->getOperType()== ITM_CONVERT &&
          ((ex_conv_clause*)clause)->getLastVOAoffset() >0 &&
          ((ex_conv_clause*)clause)->getComputedLength() >0 )
      {
        varOffset = ((ex_conv_clause*)clause)->getComputedLength();
        currAttr = *(clause->getOperand());
      }
      else
      // Check if we have an operand that we need to keep track of the length
      // as we go.  This is a varchar attribute for SQLMX_FORMAT and
      // PACKED_FORMAT formats, and all attributes for SQLMX_ALIGNED_FORMAT
      // since fixed fields are rearranged.
      if ((currAttr && retcode == ex_expr::EXPR_OK) ||
          (currAttr                        &&
           (retcode == ex_expr::EXPR_NULL) &&
           ((currAttr->getTupleFormat() == ExpTupleDesc::PACKED_FORMAT) ||
            (currAttr->getTupleFormat() == ExpTupleDesc::SQLMX_ALIGNED_FORMAT))))
      {
        varOffset += currAttr->getLength(op_data[ex_clause::MAX_OPERANDS]);

        // Need the last attribute when we get done to adjust the row length
        // for aligned format.
        if (currAttr->isSQLMXAlignedFormat())
          prevAttr = currAttr;
      }
					     
      if (diagsArea != atp1->getDiagsArea())
	atp1->setDiagsArea(diagsArea);

      if (retcode == ex_expr::EXPR_ERROR)
	return retcode;

      // copy result data into result buffer, if generated in an aligned buffer
      if (result_data)
	{
	  str_cpy_all(result_data, (char*)temp_buffer[0], 
		      clause->getOperand(0)->getLength());
	}
 
      // advance to the next clause
      clause = clause->getNextClause();
    } // while

  if (rowLen && (varOffset != ExpOffsetMax))
  {
    currAttr = (currAttr ? currAttr : prevAttr);
    if ( currAttr )
      ex_clause::evalSetRowLength(currAttr,
                                  atps[currAttr->getAtp()],
                                  rowLen,
                                  varOffset);
    else
      *rowLen = varOffset;
  }
  return retcode;
}

static ex_expr::exp_return_type alignAndEval(ex_clause * clause,
					     char ** op_data,
					     CollHeap * heap,
					     ComDiagsArea ** diagsArea)
{
  ex_expr::exp_return_type retCode = ex_expr::EXPR_OK;

  // -------------------------------------------------------------
  // If data alignment is needed, copy the operand into
  // the aligned temp_buffer array on the stack. If this is done
  // for the result then remember the real result buffer.
  // -------------------------------------------------------------

  // an array of pointers to Attributes data structures, describing
  // the data types of the operands of this expression (op[0] points
  // to the result type)
  register AttributesPtr* op = clause->getOperand();


  // allocated space for temporary, aligned operands on the stack
  // (assuming that we have no more than 4 operands and that no operand
  // using the temp space is longer than 64 bytes).
  // Using buffer of type Int64 so it starts on an 8-byte aligned offset.
  Int64 temp_buffer[ex_clause::MAX_OPERANDS][ex_clause::MAX_TEMP_OPERAND_LEN/sizeof(Int64)];

  // pointer to data for current operand
  register char ** opdata = op_data;



  retCode = clause->eval(op_data, heap, diagsArea);
  

  return retCode;
}


// ex_expr::eval
//
// Interprets the expression. If there is no PCODE, evalClauses is used
// to interpret the expression. Otherwise, the PCODE is interpreted using
// a simple stack-based abstract machine.
//
// IN     : atp1, atp2 - pointers to tuple arrays
//          exHeap - memory allocation
// OUT    : 
// RETURN : The result of the expression 
//            EXPR_OK      - no errors and no return value from expression
//            EXPR_ERROR   - expression error
//            EXPR_TRUE    - the expression returned true
//            EXPR_FALSE   - the expression returned false
//            EXPR_UNKNOWN - the expression returned unknown
// EFFECTS: Executes expression which side effects a tuple or temporary or
//          computes a return value. May also change diagnostics area
//          of atps on error.
//
// NOTE   : When introducing new pcode instructions that involve an
//          an operation between signed shorter length operand and
//          unsigned longer length operand, then one of the operands
//          must be typecasted to Int64 to account for negetive values.
//          See case PCIT::LT_MBIN32S_MBIN16S_MBIN32U below for example.
//
// This variable provides 4096 bytes of null indicators.
//
static const Int64 nullData[] = {
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

//
// This table is used when comparing 2 operands.  Given the type of comparison
// to perform (1st level index) and an index indicating whether operand1 is
// less than, equal to, or greater than, operand2 (2nd level index), return
// TRUE or FALSE.
//
// Comp codes used in compTable lookup (2nd level index)
//
// if src1 >  src2  --> compCode = 2
// if src1 == src2  --> compCode = 1
// if src1 <  src2  --> compCode = 0
//

static const Int32 compTable[6][3] = {
  /* ITM_EQUAL */      {0, 1, 0},
  /* ITM_NOT_EQUAL */  {1, 0, 1},
  /* ITM_LESS */       {1, 0, 0},
  /* ITM_LESS_EQ */    {1, 1, 0},
  /* ITM_GREATER */    {0, 0, 1},
  /* ITM_GREATER_EQ */ {0, 1, 1}
};

#define FLT64ASSIGN(tgtptr, srcptr) {*((Int64*)(tgtptr)) = *((Int64*)(srcptr));}

//////////////////////////////////////////////////////////////////////////////
//
// Inlining Routines for PCODE Optimizations
//
// The following evalFast routines were designed to evaluat unique expressions
// identified at compile-time as "inlineable" exprs.  Inlined expressions
// do not call evalPCode().  Instead they call one of several evalFast methods.
// The evalPCode() method has a large setup and take-down cost because of its
// need to support all possible PCODE instructions.  It additionally has an
// instruction-switching overhead cost.  Each evalFast() method is designed to
// evaluate a particular PCODE instruction (or a couple of them).
//
// Below is a set of defines used in each evalFast() method.  PCODE_DEF is
// needed to declare the pcode bytestream pointer for each OS target.  The
// macro SETUP_EVAL_STK is used to set up the stack-based engine needed to
// evaluate the expression.  The stack created assumes that there are no
// more than 2 tuples to be processed.
//
// Also below is the routine setEvalPtr().  This routine is called at fixup
// time to initialize the evalPtr_ function pointer to point to one of the
// many fast eval methods.  It defines which PCODE instructions are inlineable.
//
// And lastly we have the set of evalFast methods.  The methods are defined as
// "evalFastXXX", where "XXX" is the enum value of the major PCODE instruction
// being processed in this inlineable expression.
//

  #define PCODE_DEF(p) PCodeBinary * pCode = p;

#ifdef _DEBUG
  #define DBGASSERT(p)  assert(p);
#else
  #define DBGASSERT(p)
#endif

  
#define SETUP_EVAL_STK(p, atp1, atp2, e)                                    \
  Long stack[6];                                                           \
  atp_struct *atps[] = { atp1, atp2 };                                      \
                                                                            \
  PCODE_DEF(p); /* Define pCode ptr */                                      \
                                                                            \
  Int32 startOffset = ((Int32)pCode[0] << 1) + 2; /* past 1st opcode */	\
                                                                            \
  stack[1] = (Long)(e->getConstantsArea()); /* Set up constants array */   \
                                                                            \
  stack[4] = (Long)atps[pCode[1]]->getTupp((Lng32)pCode[2]).getDataPointer(); \
  if (!stack[4]) {                                                          \
    DBGASSERT(0);  /* don't expect null tuples */                           \
    stack[4] = (Long)nullData;                                               \
  }                                                                         \
  if (startOffset != 4) {                                                   \
    stack[5] = (Long)atps[pCode[3]]->getTupp((Lng32)pCode[4]).getDataPointer();	\
    if (!stack[5]) {                                                        \
      DBGASSERT(0);  /* don't expect null tuples */                         \
      stack[5] = (Long)nullData;                                             \
    }                                                                       \
  }                                                                         \
                                                                            \
  pCode += (startOffset);


// Define a pointer name directly into the pcode
#define PTR_TO_PCODE(the_type,name,pcode_offset)	\
   the_type* name = (the_type*)&pCode[pcode_offset] ;

// Define a new pointer name to type ptype, and initialize its value from
// the stack (index from pCode) plus an offset (from the next pCode entry)
#define PTR_DEF_ASSIGN(ptype, name, pcode_offset)			\
   ptype* name = (ptype*)(stack[pCode[pcode_offset]] + pCode[pcode_offset+1]);

// only the base address, from the stack
#define BASE_PTR_DEF_ASSIGN(the_type, name,pcode_offset)	\
   the_type* name = (the_type*)stack[pCode[pcode_offset]] ;

// Define a new variable name of the type, and assign it a value from pCode
#define DEF_ASSIGN(the_type, name, pcode_offset)	\
  the_type name = (the_type) pCode[pcode_offset] ; 

// Same as DEF_ASSIGN but handle pointer in pcode binary
#define DEF_ASSIGN_PTR(the_type, name, pcode_offset)	\
  the_type name = *(the_type *)&(pCode[pcode_offset]) ; 

#define MOVE_INSTR(target_type, src_type) \
  *((target_type*)(stack[pCode[0]]+pCode[1])) =		\
    *((src_type*)(stack[pCode[2]]+pCode[3]));			\
  pCode += 4;							\
  break;
     
#define MOVE_CAST_INSTR(target_type, cast_type, src_type)	\
  *((target_type*)(stack[pCode[0]]+pCode[1])) =		\
    (cast_type) (*((src_type*)(stack[pCode[2]]+pCode[3])));	\
  pCode += 4;							\
  break;
     
// The length of PCodeBinary sequence of RANGE_LOW or RANGE_HIGH instruction
#define RANGE_INST_LEN 4

//
// Native Expr Routines at Runtime
//
int dbl_le_cmp (double x, double y) { return x <= y; }
int dbl_ge_cmp (double x, double y) { return x >= y; }
int dbl_lt_cmp (double x, double y) { return x < y; }
int dbl_gt_cmp (double x, double y) { return x > y; }
int dbl_eq_cmp (double x, double y) { return x == y; }
int dbl_ne_cmp (double x, double y) { return x != y; }

ex_expr::exp_return_type reportErr(atp_struct* atp1, ex_expr* expr)
{
  ComDiagsArea *diagsArea;

  diagsArea = atp1->getDiagsArea();
  ExRaiseSqlError(expr->getHeap(), &diagsArea, EXE_INTERNAL_ERROR);
  if(diagsArea != atp1->getDiagsArea())
    atp1->setDiagsArea(diagsArea);
  return ex_expr::EXPR_ERROR;
}

Int32 bignumSub(UInt16* tgt, UInt16* src1, UInt16* src2, Int32 length)
{
  NABoolean performAdd = TRUE;
  UInt16 carry = 0;
  Int32 i;

  Int32 length16 = length >> 1;

  // Get source sign bits
  char src1Sign = BIGN_GET_SIGN(src1, length);
  char src2Sign = BIGN_GET_SIGN(src2, length);

  char* origSrc1 = (char*)src1;
  char* origSrc2 = (char*)src2;

  char origSrc1Sign = src1Sign;
  char origSrc2Sign = src2Sign;

  // Temporarily clear sign bits
  BIGN_CLR_SIGN(src1, length);
  BIGN_CLR_SIGN(src2, length);

  if (TRUE) {
    if (src1Sign == src2Sign) {
      performAdd = FALSE;

      // We want to distribute the SUB operation into the signs of the
      // source operand so that everything is normalized as if we were
      // just doing an ADD and the source operands carried the sign.
      src2Sign = (src2Sign == 0) ? MSB_SET_MSK : 0;
    }
  }
  else {
    if (src1Sign != src2Sign)
      performAdd = FALSE;
  }

  if (performAdd)
  {
    // Add shorts - this is because bignum stored this way.
    for (i=0; i < length16; i++) {
      UInt32 result = (UInt32)src1[i] + src2[i] + carry;
      tgt[i] = (UInt16)(result & 0xffff);
      carry = (UInt16)(result >> 16);
    }
  }
  else
  {
    // Determine which source is bigger
    for (i = length16-1; i >= 0; i--) {
      // Skip equality
      if (src1[i] == src2[i])
        continue;

      if (src1[i] < src2[i]) {
        UInt16* temp = src1; src1 = src2; src2 = temp;
        char tempSign = src1Sign; src1Sign = src2Sign; src2Sign = tempSign;
      }

      break;
    }

    // Sub shorts - this is because bignum stored this way.
    for (i=0; i < length16; i++) {
      Int32 result = (Int32)src1[i] - src2[i] + (Int16)carry;
      tgt[i] = (UInt16) (result + (Int32)0x10000) & 0xffff;
      carry = (result < 0) ? -1 : 0;
    }
  }

  if (origSrc1Sign)
    BIGN_SET_SIGN(origSrc1, length);

  if (origSrc2Sign)
    BIGN_SET_SIGN(origSrc2, length);

  // Set target sign - will always be src1Sign, regardless of ADD/SUB
  if (src1Sign) {
    BIGN_SET_SIGN(tgt, length);
  } else {
    BIGN_CLR_SIGN(tgt, length);
  }

  // Report overflow
  return (carry != 0);
}

//
// Method used to initialize the evalPtr_ function pointer.  If the expression
// isn't marked for inlining, then evalPtr_ should be set to NULL.  Also, fast
// moves (strcpy) are evaluated directly by evalFast4().
//
void ex_expr_base::setEvalPtr( NABoolean isSamePrimary ) {
    
  static void* nativeGlobTable[] = {
    (void*)(&nullData),                  // JIT_GLOB_NULL_TABLE = 0 
    (void*)(&randomHashValues),          // JIT_GLOB_RANDOM_HASH_VALS_TABLE = 4
    (void*)(&charStringCompareWithPad),  // JIT_GLOB_COMPARE_WITH_PAD_FUNC = 8
    (void*)(&str_cmp),                   // JIT_GLOB_STRCMP_FUNC = 12
    (void*)(&str_cpy),                   // JIT_GLOB_STRCPY_FUNC = 16
    (void*)(&dbl_le_cmp),                // JIT_GLOB_DBL_LE_CMP_FUNC = 20
    (void*)(&dbl_ge_cmp),                // JIT_GLOB_DBL_GE_CMP_FUNC = 24
    (void*)(&dbl_lt_cmp),                // JIT_GLOB_DBL_LT_CMP_FUNC = 28
    (void*)(&dbl_gt_cmp),                // JIT_GLOB_DBL_GT_CMP_FUNC = 32
    (void*)(&dbl_eq_cmp),                // JIT_GLOB_DBL_EQ_CMP_FUNC = 36
    (void*)(&dbl_ne_cmp),                // JIT_GLOB_DBL_NE_CMP_FUNC = 40
    (void*)(&reportErr),                 // JIT_GLOB_REPORT_ERROR = 44
    (void*)(&bignumSub)                  // JIT_GLOB_BIG_SUB_FUNC = 48
  };

  PCodeBinary *pCode = getPCodeBinary();

  if (getPCodeNative()) {
    // evalPtr_ may have been previously set (send-top tcbs, for example,
    // share the same expr), then we can't initialize it again, since the
    // offset into the constants area where the native code resides is lost.
    // To tell if evalPtr_ hasn't been initialized yet, we check to see if it
    // still has a valid offset value within the constant's area.

    NABoolean doMProtect = FALSE ;

    if ((evalPtrOff_ >= 0) && (evalPtrOff_ <= getConstsLength())) {
       evalPtr_ = (evalPtrType)((Long)getConstantsArea() + (Long)evalPtr_);
       doMProtect = TRUE ;
    }
    else if ( ! isSamePrimary )
       doMProtect = TRUE ;

    if ( doMProtect ) {

      // Now give execute privilege to the page containing evalPtr_.  This will
      // have the side effect of giving other bytes in this page (not related
      // to native expr) execute privilege, so there may be a security risk
      // here.  Also note, this only needs to be done on Linux.
      //
      // NOTE: We could do a better job tightening the upper boundary for
      // which memory pages are executable, but it would require knowing the
      // length of the native expression and we don't currently keep that
      // info.
      //
      void* pageStart = (void*)((Long)evalPtr_ & ~(getpagesize() - 1));
      Int32 lenToAllow = (getConstantsArea() + getConstsLength()) - (char *)pageStart;
      Int32 err = mprotect(pageStart, lenToAllow,
                           PROT_READ | PROT_WRITE | PROT_EXEC);
   
      // If error occurred, do not go through with opt
      if (err != 0) {
        assert(FALSE);
        evalPtr_ = NULL;
        return;
      }
    }

    // Redefine pCode to point to the static function lookup table that can be
    // used at runtime.
    setPCodeBinary((PCodeBinary*)nativeGlobTable);

    return;
  }

  if (!getPCodeMoveFastpath() || (pCode == NULL)) {
    evalPtr_ = NULL;
    return;
  }

  Int32 length = *(Int32*)(pCode++);
  pCode += (2 * length);

  switch(pCode[0]) {
    // Comparisons
    case PCIT::EQ_MBIN32S_MBIN16S_MBIN16S:
      evalPtr_ = &ex_expr::evalFast33;
      break;

    case PCIT::EQ_MBIN32S_MBIN32S_MBIN32S:
      evalPtr_ = &ex_expr::evalFast36;
      break;

    case PCIT::EQ_MBIN32S_MBIN16U_MBIN16U:
      evalPtr_ = &ex_expr::evalFast37;
      break;

    case PCIT::EQ_MBIN32S_MBIN32U_MBIN32U:
      evalPtr_ = &ex_expr::evalFast40;
      break;

    case PCIT::EQ_MBIN32S_MASCII_MASCII:
      evalPtr_ = &ex_expr::evalFast117;
      break;

    case PCIT::LT_MBIN32S_MASCII_MASCII:
      evalPtr_ = &ex_expr::evalFast130;
      break;

    case PCIT::LE_MBIN32S_MASCII_MASCII:
      evalPtr_ = &ex_expr::evalFast131;
      break;

    case PCIT::GT_MBIN32S_MASCII_MASCII:
      evalPtr_ = &ex_expr::evalFast132;
      break;

    case PCIT::GE_MBIN32S_MASCII_MASCII:
      evalPtr_ = &ex_expr::evalFast133;
      break;

    case PCIT::EQ_MBIN32S_MBIN64S_MBIN64S:
      evalPtr_ = &ex_expr::evalFast162;
      break;

    case PCIT::NULL_TEST_MBIN32S_MATTR5_IBIN32S_IBIN32S:
      evalPtr_ = &ex_expr::evalFast262;
      break;

    case PCIT::COMP_MBIN32S_MASCII_MASCII_IBIN32S_IBIN32S_IBIN32S:
      evalPtr_ = &ex_expr::evalFast275;
      break;


    // Hash
    case PCIT::HASH_MBIN32U_MPTR32_IBIN32S_IBIN32S:
      evalPtr_ = &ex_expr::evalFast94;
      break;

    case PCIT::HASH_MBIN32U_MBIN32_IBIN32S_IBIN32S:
      evalPtr_ = &ex_expr::evalFast170;
      break;

    case PCIT::HASH_MBIN32U_MBIN16_IBIN32S_IBIN32S:
      evalPtr_ = &ex_expr::evalFast171;
      break;

    case PCIT::HASH_MBIN32U_MBIN64_IBIN32S_IBIN32S:
      evalPtr_ = &ex_expr::evalFast173;
      break;

    case PCIT::HASH2_DISTRIB:
      evalPtr_ = &ex_expr::evalFast223;
      break;


    // Moves
    case PCIT::MOVE_MBIN32U_MBIN32U:
      evalPtr_ = &ex_expr::evalFast202;
      break;

    case PCIT::MOVE_MBIN64S_MBIN64S:
      evalPtr_ = &ex_expr::evalFast203;
      break;

    default:
      evalPtr_ = NULL;
  }
}

// Moves

ex_expr::exp_return_type
ex_expr_base::evalFast4(PCodeBinary* p, atp_struct *atp1, atp_struct *atp2,ex_expr* e)
{
  SETUP_EVAL_STK(p, atp1, atp2, e); 
  #pragma refaligned 8 pCode

  PTR_DEF_ASSIGN(char,x,0);
  PTR_DEF_ASSIGN(char,y,2);

  str_cpy_all(x, y, pCode[4]);
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type
ex_expr_base::evalFast202(PCodeBinary* p,atp_struct *atp1,atp_struct *atp2,ex_expr* e)
{
  SETUP_EVAL_STK(p, atp1, atp2, e); 
  #pragma refaligned 8 pCode

  PTR_DEF_ASSIGN(Int32,x,0);
  PTR_DEF_ASSIGN(Int32,y,2);

  *x = *y;
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type
ex_expr_base::evalFast203(PCodeBinary* p,atp_struct *atp1,atp_struct *atp2,ex_expr* e)
{
  SETUP_EVAL_STK(p, atp1, atp2, e); 
  #pragma refaligned 8 pCode

  PTR_DEF_ASSIGN(Int64,x,0);
  PTR_DEF_ASSIGN(Int64,y,2);

  *x = *y;
  return ex_expr::EXPR_OK;
}


// Hash

ex_expr::exp_return_type
ex_expr_base::evalFast94(PCodeBinary* p, atp_struct *atp1,atp_struct *atp2,ex_expr* e)
{
  SETUP_EVAL_STK(p, atp1, atp2, e); 
  #pragma refaligned 8 pCode

  PTR_DEF_ASSIGN(UInt32,x,0);
  PTR_DEF_ASSIGN(char,y,2);

  *x = ExHDPHash::hash(y, pCode[4], pCode[5]);
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type
ex_expr_base::evalFast170(PCodeBinary* p,atp_struct *atp1,atp_struct *atp2,ex_expr* e)
{
  SETUP_EVAL_STK(p, atp1, atp2, e); 
  #pragma refaligned 8 pCode

  PTR_DEF_ASSIGN(UInt32,x,0);
  PTR_DEF_ASSIGN(char,y,2);

  *x = ExHDPHash::hash4(y, pCode[4]);
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type
ex_expr_base::evalFast171(PCodeBinary* p,atp_struct *atp1,atp_struct *atp2,ex_expr* e)
{
  SETUP_EVAL_STK(p, atp1, atp2, e); 
  #pragma refaligned 8 pCode

  PTR_DEF_ASSIGN(UInt32,x,0);
  PTR_DEF_ASSIGN(char,y,2);

  *x = ExHDPHash::hash2(y, pCode[4]);
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type
ex_expr_base::evalFast173(PCodeBinary* p,atp_struct *atp1,atp_struct *atp2,ex_expr* e)
{
  SETUP_EVAL_STK(p, atp1, atp2, e); 
  #pragma refaligned 8 pCode

  PTR_DEF_ASSIGN(UInt32,x,0);
  PTR_DEF_ASSIGN(char,y,2);

  *x = ExHDPHash::hash8(y, pCode[4]);
  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type
ex_expr_base::evalFast223(PCodeBinary* p,atp_struct *atp1,atp_struct *atp2,ex_expr* e)
{
  SETUP_EVAL_STK(p, atp1, atp2, e); 
  #pragma refaligned 8 pCode

  PTR_DEF_ASSIGN(UInt32,x,0);
  PTR_DEF_ASSIGN(UInt32,y,2);
  PTR_DEF_ASSIGN(UInt32,z,4);

  *x = (UInt32)(((Int64)(*y) * (Int64)(*z)) >> 32);
  return ex_expr::EXPR_OK;
}


// Comparisons

ex_expr::exp_return_type
ex_expr_base::evalFast33(PCodeBinary* p, atp_struct *atp1,atp_struct *atp2,ex_expr* e)
{
  SETUP_EVAL_STK(p, atp1, atp2, e); 
  #pragma refaligned 8 pCode

  PTR_DEF_ASSIGN(Int16,x,2);
  PTR_DEF_ASSIGN(Int16,y,4);

  return (*x == *y) ? ex_expr::EXPR_TRUE : ex_expr::EXPR_FALSE;
}

ex_expr::exp_return_type
ex_expr_base::evalFast36(PCodeBinary* p, atp_struct *atp1,atp_struct *atp2,ex_expr* e)
{
  SETUP_EVAL_STK(p, atp1, atp2, e); 
  #pragma refaligned 8 pCode

  PTR_DEF_ASSIGN(Int32,x,2);
  PTR_DEF_ASSIGN(Int32,y,4);

  return (*x == *y) ? ex_expr::EXPR_TRUE : ex_expr::EXPR_FALSE;
}

ex_expr::exp_return_type
ex_expr_base::evalFast37(PCodeBinary* p, atp_struct *atp1,atp_struct *atp2,ex_expr* e)
{
  SETUP_EVAL_STK(p, atp1, atp2, e); 
  #pragma refaligned 8 pCode

  PTR_DEF_ASSIGN(UInt16, x,2);
  PTR_DEF_ASSIGN(UInt16, y,4);

  return (*x == *y) ? ex_expr::EXPR_TRUE : ex_expr::EXPR_FALSE;
}

ex_expr::exp_return_type
ex_expr_base::evalFast40(PCodeBinary* p, atp_struct *atp1,atp_struct *atp2,ex_expr* e)
{
  SETUP_EVAL_STK(p, atp1, atp2, e); 
  #pragma refaligned 8 pCode

  PTR_DEF_ASSIGN(UInt32,x,2);
  PTR_DEF_ASSIGN(UInt32,y,4);

  return (*x == *y) ? ex_expr::EXPR_TRUE : ex_expr::EXPR_FALSE;
}

ex_expr::exp_return_type
ex_expr_base::evalFast117(PCodeBinary* p,atp_struct *atp1,atp_struct *atp2,ex_expr* e)
{
  SETUP_EVAL_STK(p, atp1, atp2, e); 
  #pragma refaligned 8 pCode

  PTR_DEF_ASSIGN(unsigned char, x,2);
  PTR_DEF_ASSIGN(unsigned char,y,4);

  if (x[0] != y[0])
    return ex_expr::EXPR_FALSE;

  return (memcmp(x, y, pCode[6])==0) ? ex_expr::EXPR_TRUE : ex_expr::EXPR_FALSE;
}

ex_expr::exp_return_type
ex_expr_base::evalFast130(PCodeBinary* p,atp_struct *atp1,atp_struct *atp2,ex_expr* e)
{
  SETUP_EVAL_STK(p, atp1, atp2, e); 
  #pragma refaligned 8 pCode

  PTR_DEF_ASSIGN(unsigned char,x,2);
  PTR_DEF_ASSIGN(unsigned char,y,4);

  if (x[0] > y[0])
    return ex_expr::EXPR_FALSE;

  return (memcmp(x, y, pCode[6])<0) ? ex_expr::EXPR_TRUE : ex_expr::EXPR_FALSE;
}

ex_expr::exp_return_type
ex_expr_base::evalFast131(PCodeBinary* p,atp_struct *atp1,atp_struct *atp2,ex_expr* e)
{
  SETUP_EVAL_STK(p, atp1, atp2, e); 
  #pragma refaligned 8 pCode

  PTR_DEF_ASSIGN(unsigned char,x,2);
  PTR_DEF_ASSIGN(unsigned char,y,4);

  if (x[0] > y[0])
    return ex_expr::EXPR_FALSE;

  return (memcmp(x, y, pCode[6])<=0) ? ex_expr::EXPR_TRUE : ex_expr::EXPR_FALSE;
}

ex_expr::exp_return_type
ex_expr_base::evalFast132(PCodeBinary* p,atp_struct *atp1,atp_struct *atp2,ex_expr* e)
{
  SETUP_EVAL_STK(p, atp1, atp2, e); 
  #pragma refaligned 8 pCode

  PTR_DEF_ASSIGN(unsigned char,x,2);
  PTR_DEF_ASSIGN(unsigned char,y,4);

  if (x[0] < y[0])
    return ex_expr::EXPR_FALSE;

  return (memcmp(x, y, pCode[6])>0) ? ex_expr::EXPR_TRUE : ex_expr::EXPR_FALSE;
}

ex_expr::exp_return_type
ex_expr_base::evalFast133(PCodeBinary* p,atp_struct *atp1,atp_struct *atp2,ex_expr* e)
{
  SETUP_EVAL_STK(p, atp1, atp2, e); 
  #pragma refaligned 8 pCode

  PTR_DEF_ASSIGN(unsigned char,x,2);
  PTR_DEF_ASSIGN(unsigned char,y,4);

  if (x[0] < y[0])
    return ex_expr::EXPR_FALSE;

  return (memcmp(x, y, pCode[6])>=0) ? ex_expr::EXPR_TRUE : ex_expr::EXPR_FALSE;
}

ex_expr::exp_return_type
ex_expr_base::evalFast162(PCodeBinary* p,atp_struct *atp1,atp_struct *atp2,ex_expr* e)
{
  SETUP_EVAL_STK(p, atp1, atp2, e); 
  #pragma refaligned 8 pCode

  PTR_DEF_ASSIGN(Int64,x,2);
  PTR_DEF_ASSIGN(Int64,y,4);

  return (*x == *y) ? ex_expr::EXPR_TRUE : ex_expr::EXPR_FALSE;
}

ex_expr::exp_return_type
ex_expr_base::evalFast262(PCodeBinary* p,atp_struct *atp1,atp_struct *atp2,ex_expr* e)
{
  SETUP_EVAL_STK(p, atp1, atp2, e); 
  #pragma refaligned 8 pCode

  UInt32 pCode3 = (pCode[7] == 1) ? *((Int32*)(stack[pCode[2]] + pCode[4]))
                                  : pCode[3];

  NABoolean isNullCheck = (pCode[8] != 0);

  NABoolean srcNull = ExpTupleDesc::isNullValue(
    (char*)(stack[pCode[2]]+pCode3),
    (Int16)pCode[6],
    (ExpTupleDesc::TupleDataFormat)pCode[5]);

  if (srcNull && isNullCheck)
    return ex_expr::EXPR_TRUE;

  if (!srcNull && !isNullCheck)
    return ex_expr::EXPR_TRUE;

  return ex_expr::EXPR_FALSE;
}

ex_expr::exp_return_type
ex_expr_base::evalFast275(PCodeBinary* p,atp_struct *atp1,atp_struct *atp2,ex_expr* e)
{
  SETUP_EVAL_STK(p, atp1, atp2, e); 
  #pragma refaligned 8 pCode

  Int32 compCode;

  const Int32* table;
  #pragma refaligned 8 table

  // Set table pointer to appropriate position based on operation
  table = &(compTable[pCode[8] - ITM_EQUAL][1]);

  // "x" is the smaller string, "y" is the bigger one
  PTR_DEF_ASSIGN(unsigned char,x,2);
  PTR_DEF_ASSIGN(unsigned char,y,4);

  // Quick first byte compare
  if (x[0] == y[0]) {
    DEF_ASSIGN(Int32,len1,6);
    DEF_ASSIGN(Int32,len2,7);
    //Int32 len1 = pCode[6];
    //Int32 len2 = pCode[7];

    compCode = memcmp(x, y, len1);

    // Fix compCode to -1, 0, or 1
    if (compCode != 0) {
      compCode = (compCode < 0) ? -1 : 1;
    }
    // If strings are equal so far and lengths are different, compare more
    else if (len1 != len2) {
      for (Int32 i=len1; i < len2; i++) {
        if (y[i] == ' ')
          continue;

        compCode = (' ' < y[i]) ? -1 : 1;
        break;
      }
    }
  }
  else
    compCode = (x[0] < y[0]) ? -1 : 1;

  return (table[compCode] ? ex_expr::EXPR_TRUE : ex_expr::EXPR_FALSE);
}

/////////////////////////////////////////////////////////////////////////////


ex_expr::exp_return_type ex_expr::evalPCodeAligned(PCodeBinary* pCode32,
                                                   atp_struct *atp1,
                                                   atp_struct *atp2,
                                                   ULng32 *rowLen)
{
  return ex_expr::EXPR_ERROR;
}

// Guidelines for pcodes involving varchar source and/or varchar targets
// 
// PCodes involving varchars have to support these row formats:
// Packed format, exploded format and aligned format.
// 
// A Varchar operand should be defined as MATTR5 with the pcode values 
// in the following order: (atp, atpindex, offset, voaOffset, length, comboLen)
// where comboLen is a 4 byte integer whose first byte represents length of 
// nullIndicator and second byte represents length of VCIndicator, in bytes.
//
// Note that there is no nullbitmap index in the above notation because pcodes 
// related to null checking will use a shorter version than MATTR5 and 
// will have nullbitmap index as a pcode value.
// 
// If there is a need to check whether the operand is a varchar or not,
// check the value of VCIndLen.
//
// If there is a need to check whether the target is disk format or not,
// check the value of VoaOffset.
//
// Steps in which varchar pcodes should be evaluated for source operands:
// ---------------------------------------------------------------------
// 
// 1. Define a ptr to the start of the source row
//     char  *src = (char *)stack[pCode[2]];
//
// 2. Get the nullIndicator and VCIndicator lengths from the combo pcode
//    Int32 comboLen = pCode[6]; 
//    char* comboPtr = (char*)&comboLen;
//    Int16 srcNullIndLen = (Int16)comboPtr[0];
//    Int16 srcVCIndLen   = (Int16)comboPtr[1];
// 
//2a. Optionally, check if it is a varchar
//    if (srcVCIndLen > 0)
//
// 3. Call inline function getVarOffset() to get the offset to the the beginning 
//    of the varchar, i.e, to the offset of VCIndLen. Increment source to that offset.  
//    src += ExpTupleDesc::getVarOffset(src,
//                                      pCode[3],       // offset
//                                      pCode[4],       // voaOff
//                                      srcVCIndLen,    // vcIndLen
//                                      srcNullIndLen   // nullIndLen
//                                     );
//    If the supplied offset is valid (exploded format or first varchar), this
//    function would simply subtract the vcIndLen and return the new offset. 
//    If it is an indirect varchar, the offset will not be valid, the actual offset
//    will be obtained from the voaOffset and optionally nullIndLen will be added.
//
// 4. Get the actual length of the varchar by calling inline function getVarLength()
//    and increment the source to that offset.
//    srcLen = ExpTupleDesc::getVarLength(src, srcVCIndLen);
//    srcStr += srcVCIndLen; 
//
// 5. If the operand was not a varchar, use the offset and length given at compile time.
//      srcStr += pCode[3];
//      srcLen = pCode[5];
//
// src points to the start of the actual varchar data,  
// srcLen has the actual length of the varchar.
//
// Steps in which varchar pcodes should be evaluated for target operands.
// ---------------------------------------------------------------------
//
// 1. If target operand is a varchar, call getTgtVarOffset() to compute
//    the target offset. For non-disk format, it would just return 
//    offset - vcIndLen. For disk format, it will compute the offset
//    based on loop variable. This function would also update the VOA entry.
//              tgt += ExpTupleDesc::getTgtVarOffset( tgt,
//                                                    tgtOffset,    
//                                                    tgtVoaOffset, 
//                                                    tgtVCIndLen,
//                                                    tgtNullIndLen,
//                                                    varOffset,
//                                                    len0
//                                                  );
//
// 2. Update the varchar length at the varchar offset and increment the tgt pointer. 
//              if (tgtVCIndLen > 0) 
//              {
//                ExpTupleDesc::setVarLength(tgt, len0, tgtVCIndLen); 
//                tgt += tgtVCIndLen;
//              }
//
// 3. Do the desired operation.
//     if (len0 > 0)
//	 str_cpy_all(tgt, &src[start], len0);

ex_expr::exp_return_type ex_expr::evalPCode(PCodeBinary* pCode32,
					    atp_struct *atp1,
					    atp_struct *atp2,
                                            Lng32 datalen,
					    ULng32 *rowLen)
{
  ComDiagsArea *diagsArea;

  PCodeBinary * pCode = pCode32;

  // Declarations for clause variables used in some cases of the
  // switch statement below. Declared here to make the compiler happy.
  //
  ex_clause *clause;
  ex_branch_clause *branchClause;

  // The return code is OK unless something happens to change it.
  //
  ex_expr::exp_return_type retCode = ex_expr::EXPR_OK;

  // Should runtime optimizations be performed
  //
  NABoolean enableRuntimeOpts = FALSE;

  // The stack used by the evaluator and a stack pointer
  //
  char *opData[3 * ex_clause::MAX_OPERANDS];
  char **opDataData = &opData[2 * ex_clause::MAX_OPERANDS];

  // stack contains addresses
  Long stack[258];

  // The first N entries in the stack are reserved for pointers to 
  // the base addresses for the tuple data that is accessed by the
  // PCODE instructions. During PCODE generation the accessed ATPs
  // and indexes are recorded and preprended along with a count
  // to the PCODE instruction sequence.
  //
  atp_struct *atps[] = { atp1, atp2 };

  // Use Long for stack
  stack[1] = (Long)(castToExExpr()->getMyConstantsArea());
  stack[2] = (Long)(castToExExpr()->getTempsArea());
  stack[3] = (Long)(castToExExpr()->getMyPersistentArea());

  Long lengthPlus4 = *(pCode) + 4;

  // Move past length.  Note, this was done before in conjunction with the
  // previous assignment, but because of a bug in the compiler which prevents
  // the refaligned pragma to be recognized in such cases, I separated the two
  // statements -
  pCode++;

  for(Long i=4; i<lengthPlus4; i++) {
    stack[i] = (Long)atps[pCode[0]]->getTupp((Lng32)pCode[1]).getDataPointer();

    // A null (C++ NULL) tuple data pointer indicates that the tuple
    // value is null (SQL NULL). This special case is handled here
    // rather than requiring every PCODE load/move instruction to
    // understand this case.
    //
    // Basically, the null tuple address is changed to point to 4096 bytes
    // of null data (from a variable above). Thus, the PCODE instructions
    // will unsuspectingly load/move this null data instead of the 
    // non-existent null data from the tuple.
    //
    // Caveats -- null pointer errors may be masked and moves are limited
    // to 4096 bytes (size of null data array).
    //
    if(!stack[i]) { 
      // Should remove this whole check when we are confident that we
      // will no longer see any null tuples
      DBGASSERT(0);  /* don't expect null tuples */
      stack[i] = (Long)nullData;
    }
    pCode += 2;
  }

  Int16  ov;	// overflow indicator

  double flt64_1;
  double flt64_2;

  // current max variable field offset
  // incremented everytime a variable field in disk format is evaluated
  Int32 varOffset = 0;
  // Keep track of write of varchars to aligned rows.  VOA values should always be increasing.
  Int32 lastVoaOffset = 0;

    PCodeBinary * pCodeOpcPtr;

  while(1) {
    PCodeBinary pCodeOpc;

    pCodeOpcPtr = pCode;
    pCodeOpc = *pCodeOpcPtr;

    pCode++;

    /**
    *** The large switch statement below results in a mis-predicted indirect
    *** branch every time we iterate through the loop.  This cost is severe.
    *** Since we know that one of the instructions has to be an END, we can
    *** directly check for that at the beginning of each iteration.  The
    *** conditional branch will always be dynamically predicted as not taken.
    *** When we do end up taking the branch, the target address will have
    *** already been calculated, and all that would happen would be 3 inserted
    *** pipeline bubbles.  Compare that to the 13+ cycles we lose each time
    *** from the mis-predicted branch. 
    **/

    if (pCodeOpc == PCIT::RETURN) {
      if (enableRuntimeOpts) {
        PCodeCfg* cfg = new(heap_) PCodeCfg(this, NULL, NULL, heap_, NULL);
        cfg->runtimeOptimize();
        NADELETE(cfg, PCodeCfg, heap_);
      }
      return retCode;
    }

    switch(pCodeOpc) {

    case PCIT::OPDATA_MBIN16U_IBIN32S:
    case PCIT::OPDATA_MPTR32_IBIN32S:
    case PCIT::OPDATA_MPTR32_IBIN32S_IBIN32S:
    case PCIT::OPDATA_MATTR5_IBIN32S:
    {
      // Loop through all opdata instructions and then fall-through to
      // eval instruction.  Note, the assumption is that opdata/eval instruction
      // sequences are contiguous - no intermediate instruction can exist
      // within this sequence.

      do {
	PTR_DEF_ASSIGN(char,src,0);

        if (pCodeOpc == PCIT::OPDATA_MPTR32_IBIN32S_IBIN32S)
        {
          // If the operand is NULL, leave 0 on the stack, otherwise a non-zero.
          NABoolean valueNull = ExpAlignedFormat::isNullValue((char *)src,
                                                              (Int16)pCode[2]);
          opData[pCode[3]] = (char *)((long)(valueNull ? 0 : 1));
          pCodeOpc = pCode[4];
          pCode += 5;
        }
        else if (pCodeOpc == PCIT::OPDATA_MATTR5_IBIN32S)
        {
          // Set up the data pointer and vclen pointer (this one instruction
          // does both) for an indirect varchar field in aligned row format.  
          //
	  BASE_PTR_DEF_ASSIGN(char, dataPtr, 0);

	  DEF_ASSIGN(Int32,comboLen,4);
          char* comboPtr = (char*)&comboLen;
          Int16 nullIndLen = (Int16)comboPtr[0];
          Int16 vcIndLen   = (Int16)comboPtr[1];

	  DEF_ASSIGN(Int32,vcLoc,5);
          Int32 dataLoc = vcLoc + ex_clause::MAX_OPERANDS;

          // If this is the first operand it is a write.
          // So set the VOA entry for this varchar.
          if(vcLoc == ex_clause::MAX_OPERANDS) {

	    DEF_ASSIGN(Int32,tgtVoaOffset,2);
        
            // VOA offsets should always be used in order.
            if(tgtVoaOffset > 0) {
              assert(tgtVoaOffset > lastVoaOffset);
              lastVoaOffset = tgtVoaOffset;
            }

            ExpTupleDesc::setVoaValue(dataPtr, tgtVoaOffset, varOffset, vcIndLen); 
            dataPtr = dataPtr + varOffset;
            
          } else {
            // Otherwise, get the offset from the VOA entry.
	    DEF_ASSIGN(UInt32, offset, 1 );
	    DEF_ASSIGN(UInt32, voaOffset, 2 );
	
            dataPtr += ExpTupleDesc::getVarOffset(dataPtr,
                                                  offset,
                                                  voaOffset,
                                                  vcIndLen,      // vcIndLen
                                                  nullIndLen     // nullIndLen
                                                  );
          }

          // Update the VCLen pointer and the data pointer of
          // the opData array.
          opData[vcLoc] = dataPtr;
          opData[dataLoc] = dataPtr + vcIndLen;

          pCodeOpc = pCode[6];
          pCode += 7;
          
        }
        else
        {
          if (pCodeOpc == PCIT::OPDATA_MBIN16U_IBIN32S)
            src = (char*)(*((UInt16*)src) == 0);

          opData[pCode[2]] = src;
          pCodeOpc = pCode[3];
          pCode += 4;
        }
      } while (pCodeOpc != PCIT::CLAUSE_EVAL);

      //
      // Do NOT break here...
      // OPDATA and CLAUSE_EVAL go together
    }

    case PCIT::CLAUSE_EVAL:
      // on 64-bit platform, pCode[0] and pCode[1] store the clause ptr
      // and pCode[2] is check null indicator, while on 32-bit platform
      // pCode[0] alone stores the clause ptr. Use PCODEBINARIES_PER_PTR to
      // handle these differences.
      clause = (ex_clause*)*(Long*)&(pCode[0]);
      diagsArea = atp1->getDiagsArea();
      if(!(pCode[PCODEBINARIES_PER_PTR] && clause->processNulls(opData)))
	retCode = alignAndEval(clause, opDataData, getHeap(), &diagsArea);

      if (diagsArea != atp1->getDiagsArea())
	atp1->setDiagsArea(diagsArea);

      if(retCode == ex_expr::EXPR_ERROR) 
	return retCode;

      pCode += 1 + PCODEBINARIES_PER_PTR;
      break;

    case PCIT::HASHCOMB_BULK_MBIN32U:
    {
      UInt32 i, combRes = 0;
      UInt32 hashVals[PCodeCfg::MAX_HASHCOMB_BULK_OPERANDS];

      UInt32 numOfOps = ((UInt32)pCode[0] - 4) >> 2;
      UInt32 count = numOfOps;

      PTR_DEF_ASSIGN(UInt32,tgt,1);

      pCode += 3; // Move to first operand

      // Hash all source operands
      do {
	DEF_ASSIGN(Int32,len1,2);
	DEF_ASSIGN(Int32,len2,6);
	DEF_ASSIGN(Int32,pos1,3);
	DEF_ASSIGN(Int32,pos2,7);

	PTR_DEF_ASSIGN(unsigned char,src1,0);
	PTR_DEF_ASSIGN(unsigned char,src2,4);

        UInt64 result = ExHDPHash::hashP(src1, src2, len1, len2);

        UInt32 h1 = (len1 == 0) ? *((UInt32*)src1) : (UInt32)(result >> 32);
        UInt32 h2 = (len2 == 0) ? *((UInt32*)src2)
                                : (UInt32)(result & 0xFFFFFFFF);

        hashVals[pos1] = h1;
        hashVals[pos2] = h2;

        count -= 2;
        pCode += 8;
      } while (count > 1);

      if (count) {
	PTR_DEF_ASSIGN(char,src1,0);

        hashVals[pCode[3]] = (pCode[2] == 0) ? *((UInt32*)src1)
	  :  ExHDPHash::hash(src1, ExHDPHash::NO_FLAGS, 
			     (Int32)pCode[2]);
        pCode += 4;
      }

      // Join all operands (j of them) one at a time;
      combRes = hashVals[0];
      for (i=1; i < numOfOps; i++)
        combRes = ((combRes << 1) | (combRes >> 31)) ^ hashVals[i];

      *tgt = combRes;

      break;
    }

    case PCIT::NOT_NULL_BRANCH_BULK:
    {
      NABoolean isNull = FALSE;
      DEF_ASSIGN(Int32,length,0);
      length--;

      Int32 btgt = length - 1;

      if (pCode[1] == 0) {
        Int32 i = 3;
        Long tgt = stack[pCode[2]];
        do {
          // Retrieve null bit and check for nullness
          isNull = (*((Int16*)(tgt + pCode[i])) != 0);
          if (isNull)
            break;

          // Move to next offset
          i++;

        } while(i < btgt);
      }
      else {
        Long tgt, i = 2;
        do {
          tgt = stack[pCode[i]];
          // Retrieve null bit and check for nullness
          isNull = (*((Int16*)(tgt + pCode[i+1])) != 0);
          if (isNull)
            break;

          // Move to next offset
          i += 2;

        } while(i < btgt);
      }

      pCode += (isNull) ? length : pCode[btgt];
      break;
    }

    case PCIT::NULL_BITMAP_BULK:
    {
      //Int32 i;
      NABoolean isNull = FALSE;
      DEF_ASSIGN(Int32,length,0);
      length--;

      Int32 btgt = length - 1;

      if (pCode[1] == 0) {
	PTR_DEF_ASSIGN(char,nullDataPtr,2);

	PTR_TO_PCODE(char, bitmapMaskPtr, 5);

        for (Int32 i=0; i < (Int32)pCode[4]; i++) {
          isNull = (nullDataPtr[i] & bitmapMaskPtr[i]);
          if (isNull)
            break;
        }
      }
      else {
        for (Int32 i=2; i < btgt; i+=3) {
	  DEF_ASSIGN(Int32,nbi,i+2);
	  PTR_DEF_ASSIGN(char,nullDataPtr,i);

          isNull = (nbi == -1) ? (*nullDataPtr != 0) :
                       ExpAlignedFormat::isNullValue(nullDataPtr, (UInt16)nbi);

          if (isNull)
            break;
        }
      }

      pCode += (isNull) ? length : pCode[btgt];
      break;
    }

    case PCIT::MOVE_BULK:
    {
      Int32 i;
      DEF_ASSIGN(Int32,length,0);
      length--;

      Long tgtBase = stack[pCode[1]];
      Long srcBase = stack[pCode[2]];

      for (i=3; i != length; i+=3) {


        Long tgt = tgtBase + pCode[i+1];
        Long src = srcBase + pCode[i+2];


        switch (pCode[i]) {
          case PCIT::MOVE_MBIN8_MBIN8_IBIN32S:
            str_cpy_all((char*)tgt, (char*)src, (Lng32)pCode[i+3]);
            i++;
            break;

          case PCIT::MOVE_MBIN64S_MBIN64S:
            *((Int64*)tgt) = *((Int64*)src);
            break;

          case PCIT::MOVE_MBIN32U_MBIN32U:
            *((UInt32*)tgt) = *((UInt32*)src);
            break;

          case PCIT::MOVE_MBIN16U_MBIN16U:
            *((UInt16*)tgt) = *((UInt16*)src);
            break;

          case PCIT::MOVE_MBIN16U_IBIN16U:
            *((UInt16*)tgt) = (UInt16)pCode[i+2];
            break;

          case PCIT::MOVE_MBIN32S_IBIN32S:
            *((Int32*)tgt) = (Int32)pCode[i+2];
            break;

          case PCIT::MOVE_MBIN8_MBIN8:
            *((char*)tgt) = *((char*)src);
            break;
        }
      }

      pCode += length;
      break;
    }

    case PCIT::CLAUSE_BRANCH:
      // On 64-bit platform pCode[0] and pCode[1] contain branch pcode stream,
      // pCode[2] and pCode[3] store ptr to target clause, while on 32-bit
      // platform, only pCode[0] and pCode[1] are used for both.
      // Use PCODEBINARIES_PER_PTR to handle both platforms
      branchClause = (ex_branch_clause*)*(Long*)&(pCode[PCODEBINARIES_PER_PTR]);
      if(branchClause->get_branch_clause() == branchClause->getNextClause())
	pCode += (Int64)pCode[0];
      else
	pCode += PCODEBINARIES_PER_PTR + PCODEBINARIES_PER_PTR;
      break;

    case PCIT::RETURN_IBIN32S:
    {
      NABoolean res = (pCode[0] == 1);

      if (enableRuntimeOpts) {
        PCodeCfg* cfg = new(heap_) PCodeCfg(this, NULL, NULL, heap_, NULL);
        cfg->runtimeOptimize();
        NADELETE(cfg, PCodeCfg, heap_);
      }

      return ((res) ? ex_expr::EXPR_TRUE : ex_expr::EXPR_FALSE);
      break;
    }

    case PCIT::MOVE_MBIN32S:
    {
      PTR_DEF_ASSIGN(Int32,resultPtr,0);
      if ( *resultPtr == 1 ) retCode = ex_expr::EXPR_TRUE;
      else retCode = ex_expr::EXPR_FALSE;
      pCode += 2;
      break;
    }

    case PCIT::MOVE_MBIN32S_IBIN32S:
    {
      PTR_DEF_ASSIGN(UInt32,tmpUIntPtr,0);
      DEF_ASSIGN(UInt32,tmpUInt,2);
      * tmpUIntPtr = tmpUInt ;
      pCode += 3;
      break;
    }
    case PCIT::MOVE_MBIN16U_IBIN16U:
    {
      PTR_DEF_ASSIGN(UInt16, tmpUShortPtr,0);
      DEF_ASSIGN(UInt16, tmpUShort,2);
      * tmpUShortPtr = tmpUShort ;
      pCode += 3;
      break;
    }
    case PCIT::MOVE_MBIN8_MBIN8_IBIN32S:
    {
      PTR_DEF_ASSIGN(char,toPtr,0);
      PTR_DEF_ASSIGN(char,fromPtr,2);
      DEF_ASSIGN(UInt32,moveLen,4);
      str_cpy_all( toPtr, fromPtr, moveLen );
      pCode += 5;
      break;
    }
    // fixed length source to fixed length target. Lengths
    // are not equal. Currently only supports targetlen > srclen.
    case PCIT::MOVE_MASCII_MASCII_IBIN32S_IBIN32S:
      {
	DEF_ASSIGN(Int32,maxTgtLen, 4);
	DEF_ASSIGN(Int32,actualSrcLen, 5);

	PTR_DEF_ASSIGN(char,tgt, 0);
	PTR_DEF_ASSIGN(char,src, 2);

        char* pad = tgt + actualSrcLen;
        Int32 padLen = (maxTgtLen - actualSrcLen);

        str_cpy_all(tgt, src, actualSrcLen);

        // add blankpad
        str_pad(pad, padLen, ' ');

        pCode += 6;
        break;
      }

    // fixed length source to fixed length target. Lengths
    // are not equal. Currently only supports targetlen > srclen.
    case PCIT::MOVE_MUNI_MUNI_IBIN32S_IBIN32S:
      {
	DEF_ASSIGN(Int32,maxTgtLen, 4);
	DEF_ASSIGN(Int32,actualSrcLen, 5);

	PTR_DEF_ASSIGN(char,tgt, 0);
	PTR_DEF_ASSIGN(char,src, 2);

        NAWchar* pad = (NAWchar*)(tgt + actualSrcLen);
        Int32 padLen = (maxTgtLen - actualSrcLen) >> 1; // Shift for dbl-byte

        str_cpy_all(tgt, src, actualSrcLen);

        // add blankpad
        wc_str_pad(pad, padLen);

        pCode += 6;
        break;
      }

    // move varchar source to varchar target
    // source or target may be indirect. 
    case PCIT::MOVE_MATTR5_MATTR5:   
      {
	DEF_ASSIGN(Int32,maxTgtLen, 3);
	Int32 srcLen, copyLen;
	BASE_PTR_DEF_ASSIGN(char,src, 5);
	BASE_PTR_DEF_ASSIGN(char,tgt, 0);

	DEF_ASSIGN(Int32,comboLen1, 4);
        char* comboPtr1 = (char*)&comboLen1;      
        Int16 tgtNullIndLen = (Int16)comboPtr1[0];
        Int16 tgtVCIndLen   = (Int16)comboPtr1[1];

	DEF_ASSIGN(Int32,comboLen2, 9);
        char* comboPtr2 = (char*)&comboLen2;      
        Int16 srcNullIndLen = (Int16)comboPtr2[0];
        Int16 srcVCIndLen   = (Int16)comboPtr2[1];

	DEF_ASSIGN(UInt32,srcOffset, 6);
	DEF_ASSIGN(UInt32,srcVoaOffset, 7);

        src += ExpTupleDesc::getVarOffset(src,
                                          srcOffset, 
                                          srcVoaOffset,
                                          srcVCIndLen,   // vcIndLen
                                          srcNullIndLen  // nullIndLen
                                         );

        srcLen = ExpTupleDesc::getVarLength(src, srcVCIndLen);
        copyLen = ((maxTgtLen >= srcLen) ? srcLen : maxTgtLen);

        src += srcVCIndLen;

	DEF_ASSIGN(Int32,tgtOffset, 1);
	DEF_ASSIGN(Int32,tgtVoaOffset, 2);
        
        // VOA offsets should always be used in order.
        if(tgtVoaOffset > 0) {
          assert(tgtVoaOffset > lastVoaOffset);
          lastVoaOffset = tgtVoaOffset;
        }
        tgt += ExpTupleDesc::getTgtVarOffset( tgt,
                                              tgtOffset,     // tgt offset
                                              tgtVoaOffset,  // tgt voa offset
                                              tgtVCIndLen,
                                              tgtNullIndLen,
                                              varOffset,
                                              copyLen
                                            );
          
        ExpTupleDesc::setVarLength(tgt, copyLen, tgtVCIndLen);  // set variable length value
        tgt += tgtVCIndLen;  // bump past the vc indicator length field
        str_cpy_all(tgt, src, copyLen);

	pCode += 10;
      }
      break;

    // move varchar source to fixed target
    // source may be indirect
    case PCIT::MOVE_MASCII_MATTR5_IBIN32S:
      {
	DEF_ASSIGN(Int32,maxTgtLen, 7);
	Int32 srcLen, copyLen, padLen = 0;
	PTR_DEF_ASSIGN(char, dst, 0);
	BASE_PTR_DEF_ASSIGN(char, src, 2);

	DEF_ASSIGN(Int32,comboLen1, 6);
        char* comboPtr1 = (char*)&comboLen1;
        Int16 srcNullIndLen = (Int16)comboPtr1[0];
        Int16 srcVCIndLen   = (Int16)comboPtr1[1];

	DEF_ASSIGN(UInt32, srcOffset, 3);
	DEF_ASSIGN(UInt32, srcVoaOffset, 4);
        src += ExpTupleDesc::getVarOffset(src,
                                          srcOffset, 
                                          srcVoaOffset,
                                          srcVCIndLen,    // vcIndLen
                                          srcNullIndLen   // nullIndLen
                                         );

        srcLen = ExpTupleDesc::getVarLength(src, srcVCIndLen);
        padLen = maxTgtLen - srcLen;

        if (padLen > 0) {
          copyLen = srcLen;
          str_pad(dst + copyLen, padLen, ' ');
        } else {
          copyLen = maxTgtLen;
        }

        // Now copy in the value ...
        str_cpy_all(dst, src + srcVCIndLen, copyLen);

	pCode += 8;
      }
      break;

    // move fixed source to varchar target
    // target may be indirect. 
    case PCIT::MOVE_MATTR5_MASCII_IBIN32S:
      {
	PTR_DEF_ASSIGN(char, src, 5);

	DEF_ASSIGN(Int32, srcLen, 7);
	DEF_ASSIGN(Int32, maxTgtLen, 3);
        Int32 copyLen = ((maxTgtLen >= srcLen) ? srcLen : maxTgtLen);

	DEF_ASSIGN(Int32, comboLen1, 4);
        char* comboPtr1 = (char*)&comboLen1;      
        Int16 tgtNullIndLen = (Int16)comboPtr1[0];
        Int16 tgtVCIndLen   = (Int16)comboPtr1[1];

	BASE_PTR_DEF_ASSIGN(char, tgt, 0);
	DEF_ASSIGN(Int32, tgtOffset, 1);
	DEF_ASSIGN(Int32,tgtVoaOffset, 2);

        // VOA offsets should always be used in order.
        if(tgtVoaOffset > 0) {
          assert(tgtVoaOffset > lastVoaOffset);
          lastVoaOffset = tgtVoaOffset;
        }

        tgt += ExpTupleDesc::getTgtVarOffset( tgt,
                                              tgtOffset,     
                                              tgtVoaOffset,  
                                              tgtVCIndLen,
                                              tgtNullIndLen,
                                              varOffset,
                                              copyLen
                                            );
                    
        ExpTupleDesc::setVarLength(tgt, copyLen, tgtVCIndLen);  
        tgt += tgtVCIndLen;  
        str_cpy_all(tgt, src, copyLen);
                                   
	pCode += 8;
      }
      break;

    // convert varchar ptr source to Int32 or Int64
    case PCIT::CONVVCPTR_MBIN32S_MATTR5_IBIN32S:
    case PCIT::CONVVCPTR_MBIN64S_MATTR5_IBIN32S:
      {
	DEF_ASSIGN(Int32,maxTgtLen, 7);
	Int32 srcLen = 0;
	PTR_DEF_ASSIGN(char, dst, 0);
	BASE_PTR_DEF_ASSIGN(char, src, 2);

	DEF_ASSIGN(Int32,comboLen1, 6);
        char* comboPtr1 = (char*)&comboLen1;
        Int16 srcNullIndLen = (Int16)comboPtr1[0];
        Int16 srcVCIndLen   = (Int16)comboPtr1[1];

	DEF_ASSIGN(UInt32, srcOffset, 3);
	DEF_ASSIGN(UInt32, srcVoaOffset, 4);
        src += ExpTupleDesc::getVarOffset(src,
                                          srcOffset, 
                                          srcVoaOffset,
                                          srcVCIndLen,    // vcIndLen
                                          srcNullIndLen   // nullIndLen
                                         );

        srcLen = ExpTupleDesc::getVarLength(src, srcVCIndLen);

	// ptr to source value is stored at src as an Int64.
	Int64 ptrVal = *(Int64*)(src + srcVCIndLen);
	char * ptrSrc = (char*)ptrVal;

	// convert source to target (Int32)
	// First try simple conversion. If that returns an error, try complex conversion.
	NABoolean neg = FALSE;
	if (ptrSrc[0] == '-')
	  {
	    neg = TRUE;
	    ptrSrc++;
	    srcLen--;
	  }

	Int64 srcNumericVal = str_atoi(ptrSrc, srcLen);
	if (srcNumericVal == -1)
	  {
	    diagsArea = atp1->getDiagsArea();

	    ex_expr::exp_return_type er =
	      convDoIt(ptrSrc, srcLen, REC_BYTE_F_ASCII, 0, 0,
		       (char*)&srcNumericVal, sizeof(Int64), 
		       (pCodeOpc == PCIT::CONVVCPTR_MBIN64S_MATTR5_IBIN32S 
			? REC_BIN64_SIGNED : REC_BIN32_SIGNED), 
			0, 0, 
		       NULL, 0,
		       heap_, &diagsArea,
		       CONV_ASCII_BIN64S,
		       NULL, 0);

            if (diagsArea != atp1->getDiagsArea())
                   atp1->setDiagsArea(diagsArea);
	    if (er == ex_expr::EXPR_ERROR) 
	      return ex_expr::EXPR_ERROR;
	  }

	if (pCodeOpc == PCIT::CONVVCPTR_MBIN64S_MATTR5_IBIN32S)
	  {
	    if (neg)
	      *(Int64*)dst = -srcNumericVal; 
	    else
	      *(Int64*)dst = srcNumericVal; 
	  }
	else
	  {
	    if (neg)
	      *(Lng32*)dst = -(Int32)srcNumericVal; 
	    else
	      *(Lng32*)dst = (Int32)srcNumericVal; 
	  }
	  
	pCode += 8;
      }
      break;

   // convert varchar ptr source to Flt32
    case PCIT::CONVVCPTR_MFLT32_MATTR5_IBIN32S:
      {
	DEF_ASSIGN(Int32,maxTgtLen, 7);
	Int32 srcLen = 0;
	PTR_DEF_ASSIGN(char, dst, 0);
	BASE_PTR_DEF_ASSIGN(char, src, 2);

	DEF_ASSIGN(Int32,comboLen1, 6);
        char* comboPtr1 = (char*)&comboLen1;
        Int16 srcNullIndLen = (Int16)comboPtr1[0];
        Int16 srcVCIndLen   = (Int16)comboPtr1[1];

	DEF_ASSIGN(UInt32, srcOffset, 3);
	DEF_ASSIGN(UInt32, srcVoaOffset, 4);
        src += ExpTupleDesc::getVarOffset(src,
                                          srcOffset, 
                                          srcVoaOffset,
                                          srcVCIndLen,    // vcIndLen
                                          srcNullIndLen   // nullIndLen
                                         );

        srcLen = ExpTupleDesc::getVarLength(src, srcVCIndLen);

	// ptr to source value is stored at src as an Int64.
	Int64 ptrVal = *(Int64*)(src + srcVCIndLen);
	char * ptrSrc = (char*)ptrVal;

	// convert source to target (Flt32). 
	// First try simple conversion. If that returns an error, try complex conversion.
	NABoolean neg = FALSE;
	if (ptrSrc[0] == '-')
	  {
	    neg = TRUE;
	    ptrSrc++;
	    srcLen--;
	  }

	double srcNumericVal = str_ftoi(ptrSrc, srcLen);
	if (srcNumericVal == -1)
	  {
	    diagsArea = atp1->getDiagsArea();
	    ex_expr::exp_return_type er =
	      convDoIt(ptrSrc, srcLen, REC_BYTE_F_ASCII, 0, 0,
		       (char*)&srcNumericVal, sizeof(double), REC_FLOAT32, 0, 0, 
		       NULL, 0,
		       heap_, &diagsArea,
		       CONV_ASCII_FLOAT64,
		       NULL, 0);
            if (diagsArea != atp1->getDiagsArea())
                   atp1->setDiagsArea(diagsArea);
	    if (er == ex_expr::EXPR_ERROR) 
	      return ex_expr::EXPR_ERROR;
	  }

	if (neg)
	  *(float*)dst = - (float)srcNumericVal; 
	else
	  *(float*)dst = (float)srcNumericVal; 

	pCode += 8;
      }
      break;

    // convert varchar ptr source to varchar tgt
    case PCIT::CONVVCPTR_MATTR5_MATTR5:
      {
	DEF_ASSIGN(Int32,maxTgtLen, 3);
	Int32 srcLen, copyLen;
	BASE_PTR_DEF_ASSIGN(char,src, 5);
	BASE_PTR_DEF_ASSIGN(char,tgt, 0);

	DEF_ASSIGN(Int32,comboLen1, 4);
        char* comboPtr1 = (char*)&comboLen1;      
        Int16 tgtNullIndLen = (Int16)comboPtr1[0];
        Int16 tgtVCIndLen   = (Int16)comboPtr1[1];

	DEF_ASSIGN(Int32,comboLen2, 9);
        char* comboPtr2 = (char*)&comboLen2;      
        Int16 srcNullIndLen = (Int16)comboPtr2[0];
        Int16 srcVCIndLen   = (Int16)comboPtr2[1];

	DEF_ASSIGN(UInt32,srcOffset, 6);
	DEF_ASSIGN(UInt32,srcVoaOffset, 7);

        src += ExpTupleDesc::getVarOffset(src,
                                          srcOffset, 
                                          srcVoaOffset,
                                          srcVCIndLen,   // vcIndLen
                                          srcNullIndLen  // nullIndLen
                                         );

        srcLen = ExpTupleDesc::getVarLength(src, srcVCIndLen);
        copyLen = ((maxTgtLen >= srcLen) ? srcLen : maxTgtLen);

	// ptr to source value is stored at src as an Int64.
	Int64 ptrVal = *(Int64*)(src + srcVCIndLen);
	char * ptrSrc = (char*)ptrVal;

	DEF_ASSIGN(Int32,tgtOffset, 1);
	DEF_ASSIGN(Int32,tgtVoaOffset, 2);
        
        // VOA offsets should always be used in order.
        if(tgtVoaOffset > 0) {
          assert(tgtVoaOffset > lastVoaOffset);
          lastVoaOffset = tgtVoaOffset;
        }
        tgt += ExpTupleDesc::getTgtVarOffset( tgt,
                                              tgtOffset,     // tgt offset
                                              tgtVoaOffset,  // tgt voa offset
                                              tgtVCIndLen,
                                              tgtNullIndLen,
                                              varOffset,
                                              copyLen
                                            );
          
	if (copyLen < srcLen)
	  {
	    diagsArea = atp1->getDiagsArea();
	    ex_expr::exp_return_type er =
	      convDoIt(ptrSrc, srcLen, REC_BYTE_F_ASCII, 0, 0,
		       (tgt+tgtVCIndLen), maxTgtLen, REC_BYTE_V_ASCII, 0, 0, 
		       tgt, tgtVCIndLen,
		       heap_, &diagsArea,
		       CONV_ASCII_F_V,
		       NULL, 0);
            if (diagsArea != atp1->getDiagsArea())
               atp1->setDiagsArea(diagsArea);
	    if (er == ex_expr::EXPR_ERROR) 
	      return ex_expr::EXPR_ERROR;
	  }
	else
	  {
	    ExpTupleDesc::setVarLength(tgt, copyLen, tgtVCIndLen);  // set variable length value
	    tgt += tgtVCIndLen;  // bump past the vc indicator length field
	    str_cpy_all(tgt, ptrSrc, copyLen);
	  }

	pCode += 10;
      }
      break;

    case PCIT::STRLEN_MBIN32U_MATTR5:
    case PCIT::STRLEN_MBIN32U_MUNIV:
    {
      PTR_DEF_ASSIGN(UInt32, tgt, 0);

      BASE_PTR_DEF_ASSIGN(char, src, 2);
      UInt32 srcLen;

      DEF_ASSIGN(Int32, comboLen, 6);
      char* comboPtr = (char*)&comboLen;
      Int16 srcNullIndLen = (Int16)comboPtr[0];
      Int16 srcVCIndLen   = (Int16)comboPtr[1];

      DEF_ASSIGN(UInt32,srcOffset, 3);
      DEF_ASSIGN(UInt32,srcVoaOffset, 4);

      src += ExpTupleDesc::getVarOffset(src,
                                        srcOffset, 
                                        srcVoaOffset,
                                        srcVCIndLen,    // vcIndLen
                                        srcNullIndLen   // nullIndLen
                                       );

      srcLen = ExpTupleDesc::getVarLength(src, srcVCIndLen);

      // Divide length by 2 if we're dealing with a UCS2 string
      *tgt = (pCodeOpc == PCIT::STRLEN_MBIN32U_MATTR5)
             ? srcLen : (srcLen >> 1);

      pCode +=  7;
      break;
    }

    case PCIT::LIKE_MBIN32S_MATTR5_MATTR5_IBIN32S_IBIN32S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0);
      Int32 retVal = 1;

      DEF_ASSIGN(Int32,comboLen1, 6);
      DEF_ASSIGN(Int32,comboLen2, 11);

      //
      // NOTE: Use "unsigned char" in the next 2 lines so that we do not
      //       get sign extension.  That way the 8-bit values pointed at
      //       can be 0 - 255 rather than only 0 - 127.
      //
      unsigned char* comboPtr1 = (unsigned char*)&comboLen1;
      unsigned char* comboPtr2 = (unsigned char*)&comboLen2;
      
      Int16 srcVCIndLen   = (Int16)comboPtr1[1];
      Int16 patVCIndLen   = (Int16)comboPtr2[1];
      Int16 srcNullIndLen = (Int16)comboPtr1[0];
      Int16 patNullIndLen = (Int16)comboPtr2[0];

      BASE_PTR_DEF_ASSIGN(char,srcStr, 2);
      BASE_PTR_DEF_ASSIGN(char,patStr, 7);

      DEF_ASSIGN(UInt32,srcOffset, 3);
      DEF_ASSIGN(UInt32,srcVoaOffset, 4);
      DEF_ASSIGN(UInt32,srcLen, 5);
      DEF_ASSIGN(UInt32,patOffset, 8);
      DEF_ASSIGN(UInt32,patVoaOffset, 9);
      DEF_ASSIGN(UInt32,patLen, 10);

      //UInt32 srcLen, patLen;

      if (srcVCIndLen > 0)
      {
        srcStr += ExpTupleDesc::getVarOffset(srcStr,
                                             srcOffset, 
                                             srcVoaOffset,
                                             srcVCIndLen,
                                             srcNullIndLen
                                            );                                           
        srcLen = ExpTupleDesc::getVarLength(srcStr, srcVCIndLen); 
        srcStr += srcVCIndLen; 
      }
      else
      {
	srcStr += srcOffset;
      }

      if (patVCIndLen > 0)
      {
        patStr += ExpTupleDesc::getVarOffset(patStr, 
                                             patOffset, 
                                             patVoaOffset,
                                             patVCIndLen,
                                             patNullIndLen
                                            );
        patLen = ExpTupleDesc::getVarLength(patStr, patVCIndLen);
        patStr += patVCIndLen;
      }
      else
      {
	patStr += patOffset;
      }
      
      // Set up pointers to offsets and lengths of all pattern strings
      //
      // NOTE: Use "unsigned char" so that offsets and lengths can be
      // up to 255.
      //
      PTR_TO_PCODE(unsigned char, pOffPtr, 12);
      PTR_TO_PCODE(unsigned char, pLenPtr, 13);

      char* tempSrc = srcStr;
      Int32 tempSrcLen = srcLen;

      Int32 numOfPatterns = comboPtr1[3];
      Int32 flags = comboPtr1[2];
      Int32 precision = comboPtr2[2];

      // ignore trailing filler blanks for UTF-8 arguments with a char limit
      if (precision)
        tempSrcLen = 
        srcLen     = lightValidateUTF8Str(tempSrc, tempSrcLen, precision, 0);

      // Iterate through each pattern string.  The length of the pattern string
      // can't exceed the remaining length of the source string to search.  As
      // pattern strings are found, shift up source string (i.e. temp string)
      // so that searching can continue with next pattern.  Also, we generate
      // pcode *only* if there is at least one pattern.

      for (Int32 i=0; i < numOfPatterns; i++) {
        Int32 pos;
        NABoolean fastCheck = FALSE;

        char* pat    = patStr + pOffPtr[i];
        Int32 patLen = pLenPtr[i];

        if (patLen > tempSrcLen) {
          retVal = 0;
          break;
        }

        // Do a fast check if flags indicate so for head of string
        if ((i == 0) && (flags & ex_like_clause_base::LIKE_HEAD))
          fastCheck = TRUE;

        // If flags indicate a search from tail of string, re-adjust pointers
        // so that a fast boundary search can be done with strcmp.
        if ((i == numOfPatterns-1) && (flags & ex_like_clause_base::LIKE_TAIL))
        {
          fastCheck = TRUE;
          tempSrc = srcStr + (srcLen - patLen);
          tempSrcLen = patLen;
        }

        // Perform search
        pos = ex_function_position::findPosition(pat, patLen, tempSrc,
                                                 tempSrcLen, fastCheck);

        // If match was not found, return false.
        if (pos == 0) {
          retVal = 0;
          break;
        }

        // Adjust pointers to source string to begin searching next pattern.
        tempSrc += (pos + patLen - 1);
        tempSrcLen -= (pos + patLen -1);
      }

      *result = retVal;

      pCode += 14;
      break;
    }

    case PCIT::POS_MBIN32S_MATTR5_MATTR5:
    {
      PTR_DEF_ASSIGN(Int32, result, 0);
      Int32 pos;

      DEF_ASSIGN(Int32, comboLen1, 6);
      DEF_ASSIGN(Int32, comboLen2, 11);
      char* comboPtr1 = (char*)&comboLen1;
      char* comboPtr2 = (char*)&comboLen2;

      Int16 patVCIndLen   = (Int16)comboPtr1[1];
      Int16 srcVCIndLen   = (Int16)comboPtr2[1];
      Int16 patNullIndLen = (Int16)comboPtr1[0];
      Int16 srcNullIndLen = (Int16)comboPtr2[0];

      BASE_PTR_DEF_ASSIGN(char, patStr, 2);
      BASE_PTR_DEF_ASSIGN(char, srcStr, 7);

      DEF_ASSIGN(UInt32,patOffset, 3);
      DEF_ASSIGN(UInt32,patVoaOffset, 4);
      DEF_ASSIGN(UInt32, patLen, 5);

      DEF_ASSIGN(UInt32,srcOffset, 8);
      DEF_ASSIGN(UInt32,srcVoaOffset, 9);
      DEF_ASSIGN(UInt32, srcLen, 10);

      if (patVCIndLen > 0)
      {
        patStr += ExpTupleDesc::getVarOffset(patStr,
                                             patOffset, 
                                             patVoaOffset,
                                             patVCIndLen,
                                             patNullIndLen
                                            );
        patLen = ExpTupleDesc::getVarLength(patStr, patVCIndLen);
        patStr += patVCIndLen;
      }
      else
      {
        patStr += patOffset;
      }

      if (srcVCIndLen > 0)
      {
        srcStr += ExpTupleDesc::getVarOffset(srcStr,
                                             srcOffset, 
                                             srcVoaOffset,
                                             srcVCIndLen,
                                             srcNullIndLen
                                            );                                           
        srcLen = ExpTupleDesc::getVarLength(srcStr, srcVCIndLen); 
        srcStr += srcVCIndLen; 
      }
      else
      {
        srcStr += srcOffset;
      }

      // Must check patLen first to ensure functionality correctness
      if (patLen == 0)
        pos = 1;
      else if (srcLen == 0)
        pos = 0;
      else
        pos =
          ex_function_position::findPosition(patStr,patLen,srcStr,srcLen,FALSE);

      *result = pos;

      pCode += 12;
      break;
    }

    case PCIT::CONCAT_MATTR5_MATTR5_MATTR5:
    {
      DEF_ASSIGN(Int32, comboLen1, 4);
      DEF_ASSIGN(Int32, comboLen2, 9);
      DEF_ASSIGN(Int32, comboLen3, 14);

      char* comboPtr1 = (char*)&comboLen1;
      char* comboPtr2 = (char*)&comboLen2;
      char* comboPtr3 = (char*)&comboLen3;

      Int16 tgtNullIndLen  = (Int16)comboPtr1[0];
      Int16 tgtVCIndLen    = (Int16)comboPtr1[1];
      Int16 src1NullIndLen = (Int16)comboPtr2[0];
      Int16 src1VCIndLen   = (Int16)comboPtr2[1];
      Int16 src2NullIndLen = (Int16)comboPtr3[0];
      Int16 src2VCIndLen   = (Int16)comboPtr3[1];

      BASE_PTR_DEF_ASSIGN(char, tgt, 0);
      BASE_PTR_DEF_ASSIGN(char, src1, 5);
      BASE_PTR_DEF_ASSIGN(char, src2, 10);
      DEF_ASSIGN(UInt32,src1Offset, 6);
      DEF_ASSIGN(UInt32,src1VoaOffset, 7);
      DEF_ASSIGN(UInt32,src1Len, 8);
      DEF_ASSIGN(UInt32,src2Offset, 11);
      DEF_ASSIGN(UInt32,src2VoaOffset, 12);
      DEF_ASSIGN(UInt32,src2Len, 13);

      if (src1VCIndLen > 0)
      {
        src1 += ExpTupleDesc::getVarOffset(src1,
					   src1Offset, 
					   src1VoaOffset,
                                             src1VCIndLen,
                                             src1NullIndLen
                                            );

        src1Len = ExpTupleDesc::getVarLength(src1, src1VCIndLen);
        src1 += src1VCIndLen;
      }
      else
      {
        src1 += src1Offset;
      }

      if (src2VCIndLen > 0)
      {
        src2 += ExpTupleDesc::getVarOffset(src2,
					   src2Offset, 
					   src2VoaOffset,
                                             src2VCIndLen,
                                             src2NullIndLen
                                            );

        src2Len = ExpTupleDesc::getVarLength(src2, src2VCIndLen);
        src2 += src2VCIndLen;
      }
      else
      {
        src2 += src2Offset; 
      }

      Int32 tgtLen  = src1Len + src2Len;
      DEF_ASSIGN(Int32, maxTgtLen, 3);
      if (tgtLen > maxTgtLen ) 
        goto Error1_;

      DEF_ASSIGN(Int32,tgtOffset, 1);
      DEF_ASSIGN(Int32,tgtVoaOffset, 2);

      if (tgtVCIndLen > 0)
      {
        // VOA offsets should always be used in order.
        if(tgtVoaOffset > 0) {
          assert(tgtVoaOffset > lastVoaOffset);
          lastVoaOffset = tgtVoaOffset;
        }

        tgt += ExpTupleDesc::getTgtVarOffset( tgt,
                                              tgtOffset, 
                                              tgtVoaOffset,
                                              tgtVCIndLen,
                                              tgtNullIndLen,
                                              varOffset,
                                              tgtLen
                                            );

        ExpTupleDesc::setVarLength(tgt, tgtLen, tgtVCIndLen); 
        tgt += tgtVCIndLen;  
      }
      else
      {
        tgt += tgtOffset;
      }

      str_cpy_all(tgt, src1, src1Len);
      str_cpy_all(&(tgt[src1Len]), src2, src2Len);

      pCode += 15;
      break;
    }

    case PCIT::SUBSTR_MATTR5_MATTR5_MBIN32S_MBIN32S:
    {
      // SUBSTRING
      // Source/target string can be in Exploded or Compressed Internal format.

      DEF_ASSIGN(UInt32, comboLen1, 4);
      DEF_ASSIGN(UInt32, comboLen2, 9);

      char* comboPtr1 = (char*)&comboLen1;
      char* comboPtr2 = (char*)&comboLen2;
      Int16 tgtNullIndLen = (Int16)comboPtr1[0];
      Int16 tgtVCIndLen   = (Int16)comboPtr1[1];
      Int16 srcNullIndLen = (Int16)comboPtr2[0];
      Int16 srcVCIndLen   = (Int16)comboPtr2[1];
      Int16 numOperands   = (Int16)comboPtr2[2];

      DEF_ASSIGN(UInt32, srcOffset, 6);
      DEF_ASSIGN(UInt32, srcVoaOffset, 7);
      DEF_ASSIGN(UInt32, srcMaxLen, 8);

      BASE_PTR_DEF_ASSIGN(char, src, 5);
      BASE_PTR_DEF_ASSIGN(char, tgt, 0);
      
      if (srcVCIndLen > 0) // is varchar
      {
        src += ExpTupleDesc::getVarOffset(src,
                                          srcOffset, 
                                          srcVoaOffset,
                                          srcVCIndLen,   
                                          srcNullIndLen);

        srcMaxLen = ExpTupleDesc::getVarLength(src, srcVCIndLen);
        src += srcVCIndLen;
      } 
      else
      {
        src += srcOffset; 
      }

      PTR_DEF_ASSIGN(Int32, startPtr, 10);
      PTR_DEF_ASSIGN(Int32, lengthPtr,12);
      Int64 start  = *startPtr; 
      Int64 length = *lengthPtr; 

      Int64 temp = ((numOperands > 0)
                    ? (start + length)
                    : ((start > (srcMaxLen + 1)) ? start : (srcMaxLen + 1)));

     // This error case must be when we have a potential overflow
     if (temp < start) {
       diagsArea = atp1->getDiagsArea();
       ExRaiseSqlError(heap_, &diagsArea, EXE_SUBSTRING_ERROR);

       if(diagsArea != atp1->getDiagsArea())
         atp1->setDiagsArea(diagsArea);
       return ex_expr::EXPR_ERROR;
     }

     Int32 len0 = 0;
     if ((start <= srcMaxLen) && (temp > 0)) {
       if (start < 1)
         start = 1;
       if (temp > (srcMaxLen + 1))
         temp = srcMaxLen + 1;
       len0 = (Int32)(temp - start);

       // The copy length can't be greater than the max target length
       DEF_ASSIGN(Int32, maxTgtLen, 3);
       if ( len0 > maxTgtLen ) len0 = maxTgtLen ;
     }

     src += (start - 1);

     DEF_ASSIGN(Int32, tgtOffset, 1);
     DEF_ASSIGN(Int32, tgtVoaOffset, 2);

     // VOA offsets should always be used in order.
     if(tgtVoaOffset > 0) {
       assert(tgtVoaOffset > lastVoaOffset);
       lastVoaOffset = tgtVoaOffset;
     }

     tgt += ExpTupleDesc::getTgtVarOffset( tgt,
                                           tgtOffset,     
                                           tgtVoaOffset,  
                                           tgtVCIndLen,
                                           tgtNullIndLen,
                                           varOffset,
                                           len0
                                         );
     if (tgtVCIndLen > 0) // if varchar
     {
       ExpTupleDesc::setVarLength(tgt, len0, tgtVCIndLen); 
       tgt += tgtVCIndLen;  
     }

     if (len0)
       str_cpy_all(tgt, src, len0);
     
     pCode += 14;
     break;
    }

    case PCIT::MOVE_MBIN16S_MBIN8S:
      MOVE_INSTR( Int16, Int8 );

    case PCIT::MOVE_MBIN16U_MBIN8U:
      MOVE_INSTR( UInt16, UInt8 );

    case PCIT::MOVE_MBIN16U_MBIN8:
      MOVE_INSTR( UInt16, UInt8 );

    case PCIT::MOVE_MBIN32S_MBIN8S:
      MOVE_INSTR( Int32, Int8 );

    case PCIT::MOVE_MBIN32U_MBIN8U:
      MOVE_INSTR( UInt32, UInt8 );

    case PCIT::MOVE_MBIN64S_MBIN8S:
      MOVE_INSTR( Int64, Int8 );

    case PCIT::MOVE_MBIN64U_MBIN8U:
      MOVE_INSTR( UInt64, UInt8 );

    case PCIT::MOVE_MBIN32U_MBIN16U:
      MOVE_INSTR( UInt32, UInt16 );

    case PCIT::MOVE_MBIN32S_MBIN16U: 
      MOVE_INSTR( Int32 , UInt16 );

    case PCIT::MOVE_MBIN64S_MBIN16U:
      MOVE_INSTR( Int64, UInt16 );

    case PCIT::MOVE_MBIN32U_MBIN16S:
      MOVE_INSTR( UInt32 , Int16 );

    case PCIT::MOVE_MBIN32S_MBIN16S:
      MOVE_INSTR( Int32 , Int16 );

    case PCIT::MOVE_MBIN64S_MBIN16S:
      MOVE_INSTR( Int64 , Int16 );

    case PCIT::MOVE_MBIN64S_MBIN32U:
      MOVE_INSTR( Int64 , UInt32 );

    case PCIT::MOVE_MBIN64S_MBIN32S:
      MOVE_INSTR( Int64 , Int32 );

    case PCIT::MOVE_MBIN8_MBIN8:
      MOVE_INSTR( unsigned char , unsigned char );

    case PCIT::MOVE_MBIN16U_MBIN16U:
      MOVE_INSTR( UInt16 , UInt16 );

    case PCIT::MOVE_MBIN32U_MBIN32U:
      MOVE_INSTR( UInt32 , UInt32 );
      
    case PCIT::MOVE_MBIN64S_MBIN64S:
      MOVE_INSTR( Int64 , Int64 );

    case PCIT::MOVE_MBIN64S_MBIN64U:
      MOVE_INSTR( Int64 , UInt64 );

    case PCIT::MOVE_MBIN64U_MBIN64S:
      MOVE_INSTR( UInt64 , Int64 );

    case PCIT::MOVE_MBIN64U_MBIN64U:
      MOVE_INSTR( UInt64 , UInt64 );

    case PCIT::MOVE_MBIN64S_MDECS_IBIN32S:
      {
	PTR_DEF_ASSIGN(char, src, 2);
	PTR_DEF_ASSIGN(Int64, tgt, 0);
	
	if (src[0] & 0200)
	  {
	    *tgt = ((src[0] & 0177) - '0');
	  }
	else
	  {
	    *tgt = (src[0] - '0');
	  }
	for (Int32 k = 1; k < pCode[4]; k++)
	  {
	    *tgt = *tgt * 10 + (src[k] - '0');
	  }

	if (src[0] & 0200)
	  {
	    *tgt = -*tgt;
	  }

	pCode += 5;
      }
      break;

    case PCIT::MOVE_MBIN64S_MDECU_IBIN32S:
      {
	PTR_DEF_ASSIGN(char, src, 2);
	PTR_DEF_ASSIGN(Int64, tgt, 0);
	
	*tgt = 0;
	for (Int32 k = 0; k < pCode[4]; k++)
	  {
	    *tgt = *tgt * 10 + (src[k] - '0');
	  }

	pCode += 5;
      }
      break;

    case PCIT::MOVE_MFLT64_MBIN16S:
      MOVE_INSTR( double , short );

    case PCIT::MOVE_MFLT64_MBIN32S:
      MOVE_INSTR( double , Int32 );

    case PCIT::MOVE_MFLT64_MBIN64S:
      MOVE_CAST_INSTR( double , double, Int64 );

    case PCIT::MOVE_MFLT64_MFLT32:
      MOVE_INSTR( double , float );

    // Generated by ExHeaderClause ... new in R2.4
    case PCIT::HDR_MPTR32_IBIN32S_IBIN32S_IBIN32S_IBIN32S_IBIN32S:
    {
      BASE_PTR_DEF_ASSIGN(char, data, 0 );
      PTR_DEF_ASSIGN(char, hdrStart , 0 );

      // Clear the header area
      str_pad(hdrStart, pCode[2], '\0');
      
      // Set the first fixed offset
      if ( pCode[4] == sizeof(Int16) )
        *((UInt16 *)hdrStart) = (UInt16)pCode[3];
      else
        *((Int32 *)hdrStart) = (Int32)pCode[3];

      // Set the bitmap offset if relevant (n/a for SQLMX_FORMAT)
      if ( pCode[5] > 0 )
        *((Int16 *)(data + pCode[6])) = (Int16)pCode[5];

      pCode += 7;

      break;
    }

    case PCIT::NULL_MBIN16U:
    {
      PTR_DEF_ASSIGN(Int16, tmpPtr, 0 );
      * tmpPtr = 0x00;
      pCode += 2;
      break;
    }
    case PCIT::NULL_BITMAP:
    {
      PTR_DEF_ASSIGN(char, bitmap, 0 );
      DEF_ASSIGN(Int16, bitIdx , 3 );
      // for aligned format's null bitmap
      switch (pCode[2])
      {
        case PCIT::NULL_BITMAP_SET:
        {
          // Either clear a bit or set one ...
          if ( pCode[4] )
	    ExpAlignedFormat::setNullBit( bitmap, bitIdx );
          else
            ExpAlignedFormat::clearNullBit( bitmap, bitIdx );

          pCode += 5;
          break;
        }
        case PCIT::NULL_BITMAP_TEST:
        {
	  assert(0);   // this code is not used
	  // in case this code is to become used: missing below are
	  //   -- return the value from isNullValue()
	  //   -- advance the pCode
	  
          // Test a bit to see if it is null or not.
          ExpAlignedFormat::isNullValue( bitmap, bitIdx );
	  
          break;
        }
      }
    }
    break;

    case PCIT::NULL_BYTES:
    {
      // for packed format
      DEF_ASSIGN(UInt32, nullIndOffset, 1 );
      DEF_ASSIGN(Int16, tmp, 2 );

      if (nullIndOffset == ExpOffsetMax)
	nullIndOffset = varOffset;

      *(Int16 *)(stack[pCode[0]] + nullIndOffset) = tmp;

     pCode += 3;
     break;
    }

    case PCIT::NULL_MBIN16U_IBIN16U:
    {
      PTR_DEF_ASSIGN(Int16, tmpPtr16 , 0);
      DEF_ASSIGN(Int16, tmp16, 2 );
      * tmpPtr16 = tmp16 ;
      pCode += 3;
      break;
    }

    case PCIT::NULL_TEST_MBIN32S_MBIN16U_IBIN32S:
      // for packed format
    {
      PTR_DEF_ASSIGN(Int16, tmpPtr16, 2);
      DEF_ASSIGN(Int16, tmp16, 4);
      PTR_DEF_ASSIGN(Int32, resultPtr, 0);
      * resultPtr = ( *tmpPtr16 == tmp16 ) ? 1 : 0 ;
      pCode += 5;
      break;
    }

    case PCIT::NOT_NULL_BRANCH_MBIN16S_IBIN32S:
      {
	PTR_DEF_ASSIGN(Int16, resultPtr, 0);

	// Not sure why we cast to Int64 ?
	if( *resultPtr == 0 ) 
          pCode += (Int64)pCode[2];
	else 
          pCode += 3;
      }
    break;
      
    case PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_IBIN32S:
      {
	PTR_DEF_ASSIGN(Int16, returnResultPtr, 0);
	PTR_DEF_ASSIGN(Int16, resultPtr, 2);
	* returnResultPtr = * resultPtr ;
	if( *resultPtr == 0 ) pCode += (Int64)pCode[4];
	else pCode += 5;
      }
    break;

      
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S:
      {
	PTR_DEF_ASSIGN(Int32, returnResultPtr32, 0);
	PTR_DEF_ASSIGN(Int16, resultPtr16, 2);
	* returnResultPtr32 = * resultPtr16 ;
	if( * resultPtr16 == 0 ) pCode += (Int64)pCode[4];
	else pCode += 5;
      }
    break;

    // nullBranchHelper - two operands
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
      // Handles all data formats
      // If either source operands are null, then target is null.
      // If neither source operands are null, then clear target null indicator.
      {                 
        // If NullIndOffset is negative, it indicates that it is an indirect
        // varchar in packed format and its positive value gives the voaOffset
        // where the actual offset can be found (read 4 bytes from VOA entry).
        // For exploded and aligned format the NullIndOffsets will never be
        // negative since the null indicator offset is known at compile time.
        UInt32 op2NullIndOff = 
	  pCode[3] < 0 ? *((Int32 *)(stack[pCode[2]]+(-pCode[3]))) : pCode[3];
        UInt32 op3NullIndOff = 
	  pCode[5] < 0 ? *((Int32 *)(stack[pCode[4]]+(-pCode[5]))) : pCode[5];

        // for target operand, if NullIndOffset is negative, use loop variable
        // varOffset to get to the next available varchar. This only applies
        // to packed format. All other formats will have valid NullIndOffset. 
        // Note that varOffset is not incremented here. It will be incremented and
        // voaOffset will be set when the actual varchar data is written.
        UInt32 op1NullIndOff = pCode[1] < 0 ? varOffset : pCode[1];
        
        // Map a group of pcodes to PCodeAttrNull struct. 
        PCodeAttrNull *attrs = (PCodeAttrNull *)&pCode[6];

        NABoolean op2Null =
          ExpTupleDesc::isNullValue((char *)stack[pCode[2]] + op2NullIndOff,
                                    (Int16)attrs->op2NullBitIndex_,
                                    (ExpTupleDesc::TupleDataFormat)attrs->fmt_.op2Fmt_);
	NABoolean op3Null =
          ExpTupleDesc::isNullValue((char *)stack[pCode[4]] + op3NullIndOff,
                                    (Int16)attrs->op3NullBitIndex_,
                                    (ExpTupleDesc::TupleDataFormat)attrs->fmt_.op3Fmt_);

	if( op2Null || op3Null )  // either one is null
        {
          ExpTupleDesc::setNullValue((char*)(stack[pCode[0]] + op1NullIndOff),
                                     (Int16)attrs->op1NullBitIndex_,
                                     (ExpTupleDesc::TupleDataFormat)attrs->fmt_.op1Fmt_);
          pCode += attrs->fmt_.size_;
        }
	else
        {
          ExpTupleDesc::clearNullValue((char*)(stack[pCode[0]] + op1NullIndOff),
                                       (Int16)attrs->op1NullBitIndex_,
                                       (ExpTupleDesc::TupleDataFormat)attrs->fmt_.op1Fmt_);
          pCode += (Int64)pCode[10];
        }
      }
    break;

    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN32S_IATTR3_IBIN32S:
      // nullBranchHelper - one operand
      // Handles all data formats
      // If source operand is null, then target is null.
      // If source operand is not null, then clear target null indicator.
      { 
        // If NullIndOffset is negative, it indicates that it is an indirect
        // varchar in packed format and its positive value gives the voaOffset
        // where the actual offset can be found (read 4 bytes from VOA entry).
        // For exploded and aligned format the NullIndOffsets will never be
        // negative since the null indicator offset is known at compile time.
        UInt32 op2NullIndOff =
	  pCode[3] < 0 ? *((Int32 *)(stack[pCode[2]]+(-pCode[3]))) : pCode[3];

        // for target operand, if NullIndOffset is negative, use loop variable
        // varOffset to get to the next available varchar. This only applies
        // to packed format. All other formats will have valid NullIndOffset. 
        // Note that varOffset is not incremented here. It will be incremented and
        // voaOffset will be set when the actual varchar data is written.
        UInt32 op1NullIndOff = pCode[1] < 0 ? varOffset : pCode[1];  
                
        PCodeAttrNull *attrs = (PCodeAttrNull *)&pCode[4];
        
        NABoolean op2Null =
          ExpTupleDesc::isNullValue((char *)stack[pCode[2]] + op2NullIndOff,
                                    (Int16)attrs->op2NullBitIndex_,
                                    (ExpTupleDesc::TupleDataFormat)attrs->fmt_.op2Fmt_);

	if( op2Null ) // check to see if operand is null
        {
          ExpTupleDesc::setNullValue((char*)(stack[pCode[0]] + op1NullIndOff),
                                     (Int16)attrs->op1NullBitIndex_,
                                     (ExpTupleDesc::TupleDataFormat)attrs->fmt_.op1Fmt_);
          pCode += attrs->fmt_.size_;
        }
	else
        {
          ExpTupleDesc::clearNullValue((char*)(stack[pCode[0]] + op1NullIndOff),
                                       (Int16)attrs->op1NullBitIndex_,
                                       (ExpTupleDesc::TupleDataFormat)attrs->fmt_.op1Fmt_);
          pCode += (Int64)pCode[7];
        }
      }
    break;

    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_MBIN32S_IATTR4_IBIN32S:
      // nullBranchHelperForComp - two operands
      {
        // If NullIndOffset is negative, it indicates that it is an indirect
        // varchar in packed format and its positive value gives the voaOffset
        // where the actual offset can be found (read 4 bytes from VOA entry).
        // For exploded and aligned format the NullIndOffsets will never be
        // negative since the null indicator offset is known at compile time.
        UInt32 op2NullIndOff =
	  pCode[3] < 0 ? *((Int32 *)(stack[pCode[2]]+(-pCode[3]))) : pCode[3];
        UInt32 op3NullIndOff =
	  pCode[5] < 0 ? *((Int32 *)(stack[pCode[4]]+(-pCode[5]))) : pCode[5];
  
        PCodeAttrNull *attrs = (PCodeAttrNull *)&pCode[6];

        NABoolean  op2Null =
          ExpTupleDesc::isNullValue((char*)(stack[pCode[2]] + op2NullIndOff),
                                    (Int16)attrs->op2NullBitIndex_,
                                    (ExpTupleDesc::TupleDataFormat)attrs->fmt_.op2Fmt_);
        NABoolean  op3Null =
          ExpTupleDesc::isNullValue((char*)(stack[pCode[4]] + op3NullIndOff),
                                    (Int16)attrs->op3NullBitIndex_,
                                    (ExpTupleDesc::TupleDataFormat)attrs->fmt_.op3Fmt_);

        Int32 rslt = (op2Null || op3Null) ? -1 : 0;

	PTR_DEF_ASSIGN(Int32, result, 0 );
	* result = rslt ;
        if (rslt == 0)
          pCode += (Int64)pCode[10];
        else
          pCode += attrs->fmt_.size_;
      }
    break;

    case PCIT::NOT_NULL_BRANCH_COMP_MBIN32S_MBIN32S_IATTR3_IBIN32S:
      // nullBranchHelperForComp - one operand
      {
        // If NullIndOffset is negative, it indicates that it is an indirect
        // varchar in packed format and its positive value gives the voaOffset
        // where the actual offset can be found (read 4 bytes from VOA entry).
        // For exploded and aligned format the NullIndOffsets will never be
        // negative since the null indicator offset is known at compile time.
        UInt32 op2NullIndOff =
	  pCode[3] < 0 ? *((Int32 *)(stack[pCode[2]]+(-pCode[3]))) : pCode[3];
  
        PCodeAttrNull *attrs = (PCodeAttrNull *)&pCode[4];

        NABoolean  op2Null =
          ExpTupleDesc::isNullValue((char*)(stack[pCode[2]] + op2NullIndOff),
                                    (Int16)attrs->op2NullBitIndex_,
                                    (ExpTupleDesc::TupleDataFormat)attrs->fmt_.op2Fmt_);

        Int32 rslt = (op2Null) ? -1 : 0;

	PTR_DEF_ASSIGN(Int32, result, 0 );
	* result = rslt ;
        if (rslt == 0)
          pCode += (Int64)pCode[7];
        else
          pCode += attrs->fmt_.size_;
      }
    break;


    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_IBIN32S_IBIN32S:
      {
	PTR_DEF_ASSIGN(Int32, resultPtr32, 0 );
	PTR_DEF_ASSIGN(Int16, resultPtr16, 2 );
	DEF_ASSIGN(Int32, val32 , 4 );

	if( * resultPtr16 == 0 ) 
	  pCode += (Int64)pCode[5];
	else 
	  {
	    * resultPtr32 = val32 ;
	    pCode += 6;
	  }
      }
    break;
      
    case PCIT::NOT_NULL_BRANCH_MBIN32S_MBIN16S_MBIN16S_IBIN32S:
      {
	PTR_DEF_ASSIGN(Int32, ptr32 , 0 );
	PTR_DEF_ASSIGN(Int16, ptr16_1, 2 );
	PTR_DEF_ASSIGN(Int16, ptr16_2, 4 );
	Int16 result = *ptr16_1 | *ptr16_2 ;

	* ptr32 = result ;
	if(result == 0) pCode += (Int64)pCode[6];
	else pCode += 7;
      }
    break;

    case PCIT::NNB_MATTR3_IBIN32S:
      // Handles all formats ...
      {
	PTR_DEF_ASSIGN(char, ptr , 0 );
	DEF_ASSIGN(Int16, val, 2);
        NABoolean nullp = ExpTupleDesc::isNullValue( ptr, val );

	if ( nullp ) 
        {
          pCode += 4;
        }
	else 
        {
	  pCode += (Int64)pCode[3];
        }
      }
    break;
      
    case PCIT::NNB_MBIN32S_MATTR3_IBIN32S_IBIN32S:
      // Handles all formats ...
      // generated by nullBranchHelperForHash
      {
	PTR_DEF_ASSIGN(Int32, retPtr, 0 );
	PTR_DEF_ASSIGN(char, ptr , 2 );
	DEF_ASSIGN(Int16, val, 4);
	DEF_ASSIGN(Int32, retVal, 5 );
        NABoolean nullp = ExpTupleDesc::isNullValue( ptr, val );

	if ( nullp ) 
        {
	  * retPtr = retVal ;
          pCode += 7;
        }
	else 
        {
	  pCode += (Int64)pCode[6];
        }
      }
    break;
      

    case PCIT::NNB_SPECIAL_NULLS_MBIN32S_MATTR3_MATTR3_IBIN32S_IBIN32S:
      // Supports all data formats.
      {
	PTR_DEF_ASSIGN(Int32, retPtr, 0 );
	PTR_DEF_ASSIGN(char, ptr1 , 2 );
	PTR_DEF_ASSIGN(char, ptr2 , 5 );
	DEF_ASSIGN(Int16, val1, 4);
	DEF_ASSIGN(Int16, val2, 7);

        NABoolean op1Null = ExpTupleDesc::isNullValue( ptr1, val1 );
        NABoolean op2Null = ExpTupleDesc::isNullValue( ptr2, val2 );

        if ( (NOT op1Null) && (NOT op2Null) )
        {
	  * retPtr = 0;
          pCode += (Int64)pCode[9];
        }
        else
        {
          // only equi case is supported. Could be extended later.
          // pCode[8] contains the operator type (ITM_EQUAL for now).
          * retPtr = op1Null && op2Null ? 1 : 0;
          pCode += 10;
        }
      }
    break;


    case PCIT::NOT_NULL_BRANCH_MBIN16S_MBIN16S_MBIN16S_IBIN32S:
      {
	PTR_DEF_ASSIGN(Int16, ptr16_0, 0);
	PTR_DEF_ASSIGN(Int16, ptr16_2, 2);
	PTR_DEF_ASSIGN(Int16, ptr16_4, 4);

	* ptr16_0 =  * ptr16_2 | * ptr16_4 ;
	if( * ptr16_0 == 0) pCode += (Int64)pCode[6];
	else pCode += 7;
      }
    break;


    case PCIT::NULL_VIOLATION_MPTR32_MPTR32_IPTR_IPTR:
    // for aligned format's null bitmap
    // Handles 1 or 2 arguments to check if null and raise an error if so.
    //   Select b, c from TFoo where cast( c as int not null) = 30
    // where c is a nullable column this gets pushed to the EID.
    {
      PTR_DEF_ASSIGN(char, ptr_0, 0);
      PTR_DEF_ASSIGN(char, ptr_2, 2);
      Attributes *arg1 = (Attributes*)(GetPCodeBinaryAsPtr(pCode, 4));
      Attributes *arg2 = (Attributes*)(GetPCodeBinaryAsPtr(pCode, 4 + PCODEBINARIES_PER_PTR));

      NABoolean arg1Null =
	ExpTupleDesc::isNullValue( ptr_0, 
				   arg1->getNullBitIndex(),
				   arg1->getTupleFormat() );
      NABoolean arg2Null =
          ( arg2 == 0
               ? FALSE
	    : ExpTupleDesc::isNullValue( ptr_2, 
                                            arg2->getNullBitIndex(),
                                            arg2->getTupleFormat() ) );

      if( arg1Null || arg2Null )
      {
        // Raise an "assigning a null value to a NOT NULL".
        diagsArea = atp1->getDiagsArea();
        ExRaiseSqlError(getHeap(), &diagsArea, EXE_ASSIGNING_NULL_TO_NOT_NULL);
        if(diagsArea != atp1->getDiagsArea())
          atp1->setDiagsArea(diagsArea);
        return ex_expr::EXPR_ERROR;
      }

      pCode += 4 + PCODEBINARIES_PER_PTR + PCODEBINARIES_PER_PTR;
      break;
    }


    case PCIT::UPDATE_ROWLEN3_MATTR5_IBIN32S:
    {
      {
	BASE_PTR_DEF_ASSIGN(char, data, 0);
	DEF_ASSIGN(UInt32, offset, 1);
	DEF_ASSIGN(Int32, comboLen1, 4);

        char* comboPtr1 = (char*)&comboLen1;
        Int16 attrNullIndLen = (Int16)comboPtr1[0];
        Int16 attrVCIndLen   = (Int16)comboPtr1[1];

	DEF_ASSIGN(Int32, voaOffset, 2 );

        offset = ExpTupleDesc::getVarOffset( data,
                                             offset, 
                                             voaOffset,
                                             attrVCIndLen,  
                                             attrNullIndLen );

        offset += ExpTupleDesc::getVarLength( data + offset,
                                              attrVCIndLen );

        offset += attrVCIndLen;     // jump past the variable length bytes


        varOffset = offset;


        if (rowLen != NULL) {

	  DEF_ASSIGN(Int32, newLen, 5 );
          // the last varchar must adjust the overall length out if
          // in aligned format
          if ( newLen > 0 )
            {
              offset = ExpAlignedFormat::adjustDataLength(data,
                                                          offset,
                                                          newLen); 
            }

          *rowLen = offset;
        }
      }
      pCode += 6;
      break;
    }

    case PCIT::COPYVARROW_MBIN8_MBIN8_IBIN32S_IBIN32S_IBIN32S_IBIN32S:
    {
      {
	PTR_DEF_ASSIGN(char, tgtData , 0 );
	PTR_DEF_ASSIGN(char, srcData , 2 );

	DEF_ASSIGN(UInt32, voaOffset, 4);
	DEF_ASSIGN(Int32, comboLen1, 5);
        char* comboPtr1 = (char*)&comboLen1;
        Int16 attrNullIndLen = (Int16)comboPtr1[0];
        Int16 attrVCIndLen   = (Int16)comboPtr1[1];

	DEF_ASSIGN(Int16, alignment, 6);
	DEF_ASSIGN(Int32, rowLength, 7);

        UInt32 copyLength = 0;

        if (*(Int32*)srcData ==-1 )  //if aliggned format header is 0xFFFFFFFF
        {// case of null instantiated row
          DBGASSERT(*(Int16*)&srcData[voaOffset] ==-1);
          copyLength = (UInt32)rowLength;
        }
        else
        {
          copyLength = ExpTupleDesc::getVarOffset(srcData,
                                                UINT_MAX,  // offset
                                                voaOffset,         // voaEntryOffset
                                                attrVCIndLen,
                                                attrNullIndLen );

          copyLength += ExpTupleDesc::getVarLength( srcData + copyLength,
                                                  attrVCIndLen );
          copyLength += attrVCIndLen;     // jump past the variable length bytes
        }

        DBGASSERT(copyLength > 0 && copyLength <=(UInt32)rowLength);
        str_cpy_all(tgtData,srcData,copyLength);
        UInt32 newLen = copyLength;

        // the last varchar must adjust the overall length out if
        // in aligned format. Aligngemne should be ExpAlignedFormat::ALIGNMENT
        if ( alignment > 0 )
        {
          newLen = ExpAlignedFormat::adjustDataLength(tgtData, copyLength, alignment, FALSE) ;
        }

        if (rowLen != NULL)
        {
          *rowLen = newLen;
        }
      }

      pCode += 8;
      break;
    }
    case PCIT::ADD_MBIN16S_MBIN16S_MBIN16S:
    {
      PTR_DEF_ASSIGN(Int16, result, 0);
      PTR_DEF_ASSIGN(Int16, operand1, 2);
      PTR_DEF_ASSIGN(Int16, operand2, 4);
      * result = *operand1 + *operand2 ;
      pCode += 6;
      break;
    }
    case PCIT::ADD_MBIN32S_MBIN32S_MBIN32S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0);
      PTR_DEF_ASSIGN(Int32, operand1, 2);
      PTR_DEF_ASSIGN(Int32, operand2, 4);
      * result = *operand1 + *operand2 ;
      pCode += 6;
      break;
    }
    case PCIT::ADD_MBIN64S_MBIN64S_MBIN64S:
    {
      PTR_DEF_ASSIGN(Int64, result, 0);
      PTR_DEF_ASSIGN(Int64, xptr, 2);
      PTR_DEF_ASSIGN(Int64, yptr, 4);
#if (!defined _DEBUG)
      Int64 x = *xptr, y = *yptr;
      Int64 z = x + y;

      *result = z;

      // Same check C/C++ compiler uses to check for overflow
      if ((((UInt64)(~(x ^ y) & (x ^ z))) >> 63) == 0) {
      } else {
        goto Error1_;
      }
#else
      * result = EXP_FIXED_OV_ADD( *xptr, *yptr, &ov );

      if (ov)
        {// overflowed
          goto Error1_;  
        }
#endif
      pCode += 6;
      break;
    }

    case PCIT::ADD_MBIN32S_MBIN16S_MBIN16S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0);
      PTR_DEF_ASSIGN(Int16, operand1, 2);
      PTR_DEF_ASSIGN(Int16, operand2, 4);
      * result = *operand1 + *operand2 ;
      pCode += 6;
      break;
    }
    case PCIT::ADD_MBIN32S_MBIN16S_MBIN32S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0);
      PTR_DEF_ASSIGN(Int16, operand1, 2);
      PTR_DEF_ASSIGN(Int32, operand2, 4);
      * result = *operand1 + *operand2 ;
      pCode += 6;
      break;
    }

    case PCIT::ADD_MBIN64S_MBIN32S_MBIN64S:
    {
      PTR_DEF_ASSIGN(Int64, result, 0);
      PTR_DEF_ASSIGN(Int32, xptr32, 2);
      PTR_DEF_ASSIGN(Int64, yptr, 4);
#if (!defined _DEBUG)

      Int64 x = (Int64) *xptr32 , y = *yptr;
      Int64 z = x + y;

      *result = z;

      // Same check C/C++ compiler uses to check for overflow
      if ((((UInt64)(~(x ^ y) & (x ^ z))) >> 63) == 0) {
      }
      else {
        goto Error1_;
      }
#else
      * result = EXP_FIXED_OV_ADD( *xptr32, *yptr, &ov );
       if (ov)
        {
          goto Error1_;
        }
#endif
      pCode += 6;
      break;
    }

	
    case PCIT::SUB_MBIN16S_MBIN16S_MBIN16S:
    {
      PTR_DEF_ASSIGN(Int16, result, 0);
      PTR_DEF_ASSIGN(Int16, operand1, 2);
      PTR_DEF_ASSIGN(Int16, operand2, 4);
      * result = *operand1 - *operand2 ;
      pCode += 6;
      break;
    }
    case PCIT::SUB_MBIN32S_MBIN32S_MBIN32S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0);
      PTR_DEF_ASSIGN(Int32, operand1, 2);
      PTR_DEF_ASSIGN(Int32, operand2, 4);
      * result = *operand1 - *operand2 ;
      pCode += 6;
      break;
    }
    case PCIT::SUB_MBIN64S_MBIN64S_MBIN64S:
    {
      PTR_DEF_ASSIGN(Int64, result, 0);
      PTR_DEF_ASSIGN(Int64, xptr, 2);
      PTR_DEF_ASSIGN(Int64, yptr, 4);
#if (!defined _DEBUG)
      Int64 x = *xptr, y = *yptr;
      Int64 z = x - y;

      *result = z;

      // Same check C/C++ compiler uses to check for overflow
      if ((((UInt64)((x ^ y) & (x ^ z))) >> 63) == 0) {
      }
      else {
        goto Error1_;
      }
#else
      * result = EXP_FIXED_OV_SUB( *xptr, *yptr, &ov);

      if (ov)
        {
          goto Error1_;
        }
#endif
      pCode += 6;
      break;
    }

    case PCIT::SUB_MBIN32S_MBIN16S_MBIN16S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0);
      PTR_DEF_ASSIGN(Int16, operand1, 2);
      PTR_DEF_ASSIGN(Int16, operand2, 4);
      * result = *operand1 - *operand2 ;
      pCode += 6;
      break;
    }
    case PCIT::SUB_MBIN32S_MBIN16S_MBIN32S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0);
      PTR_DEF_ASSIGN(Int16, operand1, 2);
      PTR_DEF_ASSIGN(Int32, operand2, 4);
      * result = *operand1 - *operand2 ;
      pCode += 6;
      break;
    }
    case PCIT::SUB_MBIN32S_MBIN32S_MBIN16S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0);
      PTR_DEF_ASSIGN(Int32, operand1, 2);
      PTR_DEF_ASSIGN(Int16, operand2, 4);
      * result = *operand1 - *operand2 ;
      pCode += 6;
      break;
    }
    case PCIT::MUL_MBIN16S_MBIN16S_MBIN16S:
    {
      PTR_DEF_ASSIGN(Int16, result, 0);
      PTR_DEF_ASSIGN(Int16, operand1, 2);
      PTR_DEF_ASSIGN(Int16, operand2, 4);
      * result = *operand1 * *operand2 ;
      pCode += 6;
      break;
    }
    case PCIT::MUL_MBIN32S_MBIN32S_MBIN32S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0);
      PTR_DEF_ASSIGN(Int32, operand1, 2);
      PTR_DEF_ASSIGN(Int32, operand2, 4);
      * result = *operand1 * *operand2 ;
      pCode += 6;
      break;
    } 
    case PCIT::MUL_MBIN64S_MBIN16S_MBIN32S:
    {
      PTR_DEF_ASSIGN(Int64, result, 0);
      PTR_DEF_ASSIGN(Int16, operand1, 2);
      PTR_DEF_ASSIGN(Int32, operand2, 4);
      * result = (Int64) *operand1 * (Int64) *operand2 ;

      pCode += 6;
      break;
    }
    case PCIT::MUL_MBIN64S_MBIN32S_MBIN32S:
    {
      PTR_DEF_ASSIGN(Int64, result, 0);
      PTR_DEF_ASSIGN(Int32, operand1, 2);
      PTR_DEF_ASSIGN(Int32, operand2, 4);
      * result = (Int64) *operand1 * (Int64) *operand2 ;
      pCode += 6;
      break;
    }
    case PCIT::MUL_MBIN64S_MBIN64S_MBIN64S:
    {
      PTR_DEF_ASSIGN(Int64, result, 0);
      PTR_DEF_ASSIGN(Int64, xptr, 2);
      PTR_DEF_ASSIGN(Int64, yptr, 4);
      * result = EXP_FIXED_OV_MUL( *xptr, *yptr, &ov);

      if (ov)
        {
          goto Error1_;
        }
      pCode += 6;
      break;
    }

    case PCIT::MUL_MBIN32S_MBIN16S_MBIN16S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0);
      PTR_DEF_ASSIGN(Int16, operand1, 2);
      PTR_DEF_ASSIGN(Int16, operand2, 4);
      * result = *operand1 * *operand2 ;
      pCode += 6;
      break;
    }
    case PCIT::MUL_MBIN32S_MBIN16S_MBIN32S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0);
      PTR_DEF_ASSIGN(Int16, operand1, 2);
      PTR_DEF_ASSIGN(Int32, operand2, 4);
      * result = *operand1 * *operand2 ;
      pCode += 6;
      break;
    }

    case PCIT::DIV_MBIN64S_MBIN64S_MBIN64S:
    {
      PTR_DEF_ASSIGN(Int64, result, 0);
      PTR_DEF_ASSIGN(Int64, xptr, 2);
      PTR_DEF_ASSIGN(Int64, yptr, 4);
#if (!defined _DEBUG)

      Int64 x = *xptr, y = *yptr;

      // Same check C/C++ compiler uses to check for overflow
      if (y != 0) {
        if ((x != 0x8000000000000000LL) || (y != -1)) {
          *result = x / y;
        }
        else {
          goto Error1_;
        }
      }
      else {
        goto Error1_;
      }
#else
      if ( *yptr == 0 )
        {// div by zero
          goto Error1_; 
        }

      * result = EXP_FIXED_OV_DIV( *xptr, *yptr, &ov);

      if (ov)
        {
          goto Error1_;
        }
#endif
      pCode += 6;
      break;
    }

    case PCIT::DIV_MBIN64S_MBIN64S_MBIN64S_ROUND:
    {
      PTR_DEF_ASSIGN(Int64, result, 0);
      PTR_DEF_ASSIGN(Int64, xptr, 2);
      PTR_DEF_ASSIGN(Int64, yptr, 4);

      if ( *yptr == 0 )
	{// div by zero
	  goto Error1_; 
	}
	
	// upscale numerator by 1 
	NABoolean upscaled;
	Int64 temp;
	DEF_ASSIGN(Int32, bitmap, 6);
	Lng32 roundingMode = (Lng32)(bitmap & 0x77);
	NABoolean divToDownscale = ((bitmap & 0x100) != 0);
	if (divToDownscale)
	  {
	    temp = *yptr / 10;
	    temp = *xptr / temp;
	    upscaled = TRUE;
	  }
	else
	  {
	    temp = EXP_FIXED_OV_MUL( *xptr,
				    10, &ov);
	    if (ov)
	      {
		// couldn't upscale. Use the original value and don't do
		// any rounding.
		upscaled = FALSE;
		temp = * xptr;
	      }
	    else
	      {
		upscaled = TRUE;
	      }
	    
	    temp = EXP_FIXED_OV_DIV(temp,
				    *yptr,
				    &ov);
	    if (ov)
	      {
	        //division by zero
		goto Error1_;
	      }
	  }

	if (upscaled)
	  {
	    short nsign = 0; // indicates negative sign when set to 1.
	    
	    // get the last digit
	    Lng32 v = (Lng32) (temp % (Int64)10);
            
            if( v < 0 ) v = -v;
            if(temp < 0) nsign = 1;
            
	    // downscale the result
	    temp = temp / (Int64)10;

	    if (roundingMode == 1)
	      {
		// ROUND HALF UP MODE
		if (v >= 5)
		  {
		    // round up the result by 1
		    temp = nsign ? temp-1 : temp+1;
		  }
	      }
	    else if (roundingMode == 2)
	      {
		// ROUND HALF EVEN MODE
		if (v > 5)
		  {
		    // round up the result by 1
		    temp = nsign ? temp-1 : temp+1;
		  }
		else if (v == 5)
		  {
		    // Roundup 'w' only if all the trailing digits following
		    // 'v' is zero and 'w' is even. If 'w' is odd, irrespective
		    // of trailing digits following 'v', 'w' is rounded up.
		    // w is second last digit.
		    Lng32 w = (Lng32) (temp % (Int64)10);
		    if ((w & 0x1) != 0)
		    {
		      // odd number, round up.
		      temp = nsign ? temp-1 : temp+1;
		    }
		    else
		    {
		      // Since 'w' is an even digit, we need to determine if
		      // all the trailing digits following 'v' is zero.
		      // If any digit following 'v' is nonzero, then it is
		      // similar to v > 5.
		      NABoolean vGT5 = FALSE;
		      if(!divToDownscale)
		      {
		        //Figure out if digits following 'v' is non zero.
		        Int64 multiplier = 100;//For digit following 'v'.
		        Int64 temp1;
                       NABoolean biggerPrecision = FALSE;
		        while(!vGT5)
		        {
			  temp1 = EXP_FIXED_OV_MUL(*xptr, multiplier, &ov);
	                  if (ov)
	                  {
	                    //end of digits.
			    // When we reach here, temp1 is overflowed over 
			    // Int64.  In this rare but possible situation, 
			    // we should try checking
			    // for additional digits using BigNum datatype.
			    biggerPrecision = TRUE;
	                    break;
	                  }
	                  temp1 = EXP_FIXED_OV_DIV(temp1, *yptr, &ov);
	                  if (ov)
	                  {
	                    //Something went wrong, lets consider
	                    //it as end of digits.
		            break;
	                  }
	                  if(temp1 % (Int64)10)
	                  {
	                    vGT5 = TRUE;
	                    break;
	                  }
		          multiplier = EXP_FIXED_OV_MUL(multiplier, 10, &ov);
	                  if (ov)
	                  {
	                    //end of digits.
	                    break;
	                  }
		        }
                       if(biggerPrecision)
                       { 
                          short rc = 0;
			  Int64 dividend = *xptr;
			  Int64 divisor  = *yptr;
                          char *op_data[3];
                          char result1[100];
                          char result2[100];
                          Int64 result3 = 0;
                          Int64 ten = 10;
                          SimpleType opST(REC_BIN64_SIGNED, sizeof(Int64), 0, 0,
		                         ExpTupleDesc::SQLMX_FORMAT,
		                         8, 0, 0, 0, Attributes::NO_DEFAULT, 0);
                          
                          BigNum opBN(16, 38, 0, 0);                          
                          
                          while(!vGT5)
		           {
                            op_data[0] = result1;
                            op_data[1] = (char *) &dividend;
                            op_data[2] = (char *) &multiplier; 
                            rc = EXP_FIXED_BIGN_OV_MUL(&opST,
                                                    &opST,
			                              op_data);
                           if (rc)
	                    {
	                      //end of digits.
	                      break;
	                    }
                           
                           op_data[0] = result2;
                           op_data[1] = result1;
                           op_data[2] = (char *) &divisor;
                           rc = EXP_FIXED_BIGN_OV_DIV(&opBN,
				                    &opST,
				                    op_data);
	                    if (rc)
	                    {
	                      //Something went wrong, lets consider
	                      //it as end of digits.
		              break;
	                    }
                           
                           op_data[0] = result2;
                           op_data[1] = (char *) &ten;
                           result3 = EXP_FIXED_BIGN_OV_MOD(&opBN,
                                                      &opST,
                                                      op_data,
                                                      &ov);
                           if (ov)
	                    {
	                      //end of digits.
	                      break;
	                    }
                            
	                    if(result3)
	                    {
	                      vGT5 = TRUE;
	                      break;
	                    }
		             
                           multiplier = EXP_FIXED_OV_MUL(multiplier, 10, &ov);
	                    if (ov)
	                    {
	                      //end of digits.
	                      break;
	                    }
		           }
                       }
		      }
		      else
		      {
		        Int64 divisor = 100;//divisor=10 corresponds to v digit.
		        Int64 temp1;
		        while(!vGT5)
		        {
			  temp1 = *yptr / divisor;
		          if(!temp1) //reached end of digits.
		          {
		            break;
		          }
			  temp1 = *xptr / temp1;
		          if(temp1 % (Int64)10)
	                  {
	                    vGT5 = TRUE;
	                    break;
	                  }
	                  divisor = EXP_FIXED_OV_MUL(divisor, 10, &ov);
	                  if (ov)
	                  {
	                    //end of digits.
	                    break;
	                  }
		        }
		      }
  		    
		      if(vGT5)
		      {
		        //irrespective of w being even number,
		        //round up the value;
		        temp = nsign ? temp-1 : temp+1;  
		      }
		    }
		  }
	      }
	  }

	* result = temp;
	pCode += 7;
      }
    break;

    // Sum of two nullable BIN32S. Do the null processing in this case
    // and then fall through to the next case to handle the actual
    // addition.
    //
    case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN32S_MBIN32S:
    {
      PTR_DEF_ASSIGN(char, nv1, 0);
      DEF_ASSIGN(Int16, off1, 2);
      PTR_DEF_ASSIGN(char, nv2, 3);
      DEF_ASSIGN(Int16, off2, 5);
      PTR_DEF_ASSIGN(Int32, target, 7);

      if ( ExpTupleDesc::isNullValue( nv2, off2 ) )
      {
        pCode += 11;
        break;
      }

      if ( ExpTupleDesc::isNullValue( nv1, off1 ) )
       {
         // Clear the target null bytes/bit ...
	 ExpTupleDesc::clearNullValue( nv1, off1 );

         // Now set the target value to 0 so it can be used in the sum ...
	 * target = 0;
        }
      
      pCode += 7;
      //
      // Do NOT break here...
      // The nulls have been processed above, so fall through to do the adding.
      // These two cases belong together...
      //
      // Sum of two BIN32S. Assume that either the operands are not nullable
      // or the null processing has been accomplished above. Sum the second
      // operand to the first. 
      //
    }
    case PCIT::SUM_MBIN32S_MBIN32S:
    {
      PTR_DEF_ASSIGN(Int32, tgtPtr, 0 );
      PTR_DEF_ASSIGN(Int32, operPtr, 2 );
      * tgtPtr += *operPtr ;
      pCode += 4;
      break;
    }
    // Sum of two nullable BIN64S. Do the null processing in this case
    // and then fall through to the next case to handle the actual
    // addition.
    //
    case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN64S:
    {
      PTR_DEF_ASSIGN(char, nv2, 3);
      DEF_ASSIGN(Int16, off2, 5);

      if ( ExpTupleDesc::isNullValue( nv2, off2 ) )
      {
        pCode += 11;
        break;
      }

      PTR_DEF_ASSIGN(char, nv1, 0);
      DEF_ASSIGN(Int16, off1, 2);
      PTR_DEF_ASSIGN(Int64, target, 7);

      if ( ExpTupleDesc::isNullValue( nv1, off1 ) )
       {
         // Clear the target null bytes/bit ...
         ExpTupleDesc::clearNullValue( nv1, off1 );

         // Now set the target value to 0 so it can be used in the sum ...
	 * target = 0;
        }
      
      pCode += 7;
      //
      // Do NOT break here...
      // The nulls have been processed above, so fall through to do the adding.
      // These two cases belong together...
      //
      // Sum of two BIN64S. Assume that either the operands are not nullable
      // or the null processing has been accomplished above. Sum the second
      // operand to the first. Check the result for overflow.
      //
    }
    case PCIT::SUM_MBIN64S_MBIN64S:
    {
      PTR_DEF_ASSIGN(Int64, result, 0);
      PTR_DEF_ASSIGN(Int64, yptr, 2);
      //#if defined NA_YOS && !defined _DEBUG
      Int64 y = *yptr;
      Int64 x = *result;

      Int64 z = x + y;

      // Same check C/C++ compiler uses to check for overflow
      if ((((UInt64)(~(x ^ y) & (x ^ z))) >> 63) == 0) {
        *result = z;
      } else {
        goto Error1_;
      }
      pCode += 4;
      break;
    }

    // Sum of a nullable BIN64S and BIN32S. Do the null processing in this
    // case and then fall through to the next case to handle the actual
    // addition.
    //
    case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN32S:
    {
      PTR_DEF_ASSIGN(char, nv2, 3);
      DEF_ASSIGN(Int16, off2, 5);

      if ( ExpTupleDesc::isNullValue( nv2, off2 ) )
      {
        pCode += 11;
        break;
      }

      PTR_DEF_ASSIGN(char, nv1, 0);
      DEF_ASSIGN(Int16, off1, 2);
      PTR_DEF_ASSIGN(Int64, target, 7);

      if ( ExpTupleDesc::isNullValue( nv1, off1 ) )
       {
         // Clear the target null bytes/bit ...
	 ExpTupleDesc::clearNullValue( nv1, off1 );

         // Now set the target value to 0 so it can be used in the sum ...
	 * target = 0;
        }
      
      pCode += 7;
      //
      // Do NOT break here...
      // The nulls have been processed above, so fall through to do the adding.
      // These two cases belong together...
      //
      // Sum of a BIN64S and BIN32S. Assume that either the operands are not
      // nullable or the null processing has been accomplished above. Sum the
      // second operand to the first. Check the result for overflow.
      //
    }
    case PCIT::SUM_MBIN64S_MBIN32S:
    {
      PTR_DEF_ASSIGN(Int64, result, 0 );
      PTR_DEF_ASSIGN(Int32, yptr, 2 );
#if (!defined _DEBUG)
      Int32 y = *yptr;
      Int64 x = *result;

      Int64 z = x + (Int64)y;

      // Same check C/C++ compiler uses to check for overflow
      if ((((UInt64)(~(x ^ y) & (x ^ z))) >> 63) == 0) {
        *result = z;
      } else {
        goto Error1_;
      }
#else
      Int64 op1 = * result;
      Int64 theResult = EXP_FIXED_OV_ADD( op1, (Int64) *yptr, &ov);
      if (ov)
	{
	  goto Error1_;
	}

      * result = theResult;
#endif
      pCode += 4;
      break;
    }


    // Sum of two nullable FLT64. Do the null processing in this case
    // and then fall through to the next case to handle the actual
    // addition.
    //
    case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MFLT64_MFLT64:
    {
      PTR_DEF_ASSIGN(char, nv2, 3);
      DEF_ASSIGN(Int16, off2, 5);

      if ( ExpTupleDesc::isNullValue( nv2, off2 ) )
      {
        pCode += 11;
        break;
      }

      PTR_DEF_ASSIGN(char, nv1, 0);
      DEF_ASSIGN(Int16, off1, 2);

      if( ExpTupleDesc::isNullValue( nv1, off1 ) )
       {
         // Clear the target null bytes/bit ...
	 ExpTupleDesc::clearNullValue( nv1, off1 );

	 PTR_DEF_ASSIGN(Int64, target, 7);

         // Now set the target value to 0 so it can be used in the sum ...
	 flt64_1 = 0;
	 * target = (Int64) flt64_1 ;  // why not just use a simple zero ?
        }

      pCode += 7;
      //
      // Do NOT break here...
      // The nulls have been processed above, so fall through to do the adding.
      // These two cases belong together...
      //
      // Sum of two FLT64 ... assume that either the operands are not nullable
      // or the null processing has been accomplished above. Sum the second
      // operand to the first. Check the result for overflow.
      //
    }
    case PCIT::SUM_MFLT64_MFLT64:
    {
      double flt64_0;
      FLT64ASSIGN(&flt64_1, (stack[pCode[0]] + pCode[1]));
      FLT64ASSIGN(&flt64_2, (stack[pCode[2]] + pCode[3]));

      // The following is an inlining of most of MathReal64Add (from exp/exp_ieee.cpp), 
      // done for performance reasons. For the SUM aggregate, in large queries we expect 
      // millions or billions of rows; the inlining makes a difference at that scale.

      unsigned int mxcsr;
      __asm__ ("stmxcsr %0" : "=m" (*&mxcsr)); 
      /* Clear the relevant bits.  */ 
      mxcsr &= ~(FE_OVERFLOW | FE_INVALID | FE_DIVBYZERO | FE_UNDERFLOW);
      /* And put them into effect.  */
      __asm__ ("ldmxcsr %0" : : "m" (*&mxcsr));
  
      flt64_0 = flt64_1 + flt64_2;
  
      unsigned int cs;
      __asm__ ("stmxcsr %0" : "=m" (*&cs));
      cs &= (FE_UNDERFLOW | FE_OVERFLOW | FE_DIVBYZERO | FE_INVALID);
      if (UNLIKELY(cs))  // if there was an error (this is unlikely)
        {
          ov = 0;
          MathEvalException(flt64_0, cs, &ov);  // don't bother inlining error path
          goto Error1_;
        }

      FLT64ASSIGN((stack[pCode[0]] + pCode[1]), &flt64_0);

      pCode += 4;
      break;
    }

    case PCIT::MINMAX_MBIN8_MBIN8_MBIN32S_IBIN32S:
      if (*((Int32*)(stack[pCode[4]]+pCode[5])) == 1) // if TRUE  
	{
	  PTR_DEF_ASSIGN(char, target, 0 );
	  PTR_DEF_ASSIGN(char, source, 2 );
	  DEF_ASSIGN(Int32, length, 6 );
	  // make the current value the new min/max value
	  str_cpy_all( target, source, length );
	}   
      pCode += 7;
      break;

      // A new PCode for hashing an 8 byte key (pCode[4] is ignored)
    case PCIT::HASH_MBIN32U_MBIN64_IBIN32S_IBIN32S:
    {
      PTR_DEF_ASSIGN(UInt32, hashValuePtr, 0 );
      PTR_DEF_ASSIGN(char, eightByteValPtr, 2 );
      * hashValuePtr = ExHDPHash::hash8( eightByteValPtr, pCode[4]);

      pCode += 6;
      break;
    }
    case PCIT::HASH_MBIN32U_MBIN32_IBIN32S_IBIN32S:
    {
      PTR_DEF_ASSIGN(UInt32, hashValuePtr, 0 );
      PTR_DEF_ASSIGN(char, fourByteValPtr, 2 );
      * hashValuePtr = ExHDPHash::hash4( fourByteValPtr, pCode[4]);

      pCode += 6;
      break;
    }
    case PCIT::HASH_MBIN32U_MBIN16_IBIN32S_IBIN32S:
    {
      PTR_DEF_ASSIGN(UInt32, hashValuePtr, 0 );
      PTR_DEF_ASSIGN(char, twoByteValPtr, 2 );
      * hashValuePtr = ExHDPHash::hash2( twoByteValPtr, pCode[4]);

      pCode += 6;
      break;
    }
    case PCIT::HASH_MBIN32U_MPTR32_IBIN32S_IBIN32S:
    {
      PTR_DEF_ASSIGN(UInt32, hashValuePtr, 0 );
      PTR_DEF_ASSIGN(char, dataPtr, 2 );
      DEF_ASSIGN(UInt32, flags, 4 );
      DEF_ASSIGN(Int32, length, 5 );
      * hashValuePtr = ExHDPHash::hash( dataPtr, flags, length );

      pCode += 6;
      break;
    }

    // varchar hash function
    case PCIT::HASH_MBIN32U_MUNIV:
    case PCIT::HASH_MBIN32U_MATTR5:
    {
      DEF_ASSIGN(UInt32, offset, 3 );
      DEF_ASSIGN(UInt32, voaOffset, 4 );
      BASE_PTR_DEF_ASSIGN(char, data, 2 );
      PTR_DEF_ASSIGN(UInt32, tgt, 0 );
      DEF_ASSIGN(UInt32, len, 5 );
      DEF_ASSIGN(Int32, comboLen, 6 );
      char* comboPtr = (char*)&comboLen;
      Int16 attrNullIndLen = (Int16)comboPtr[0];
      Int16 attrVCIndLen   = (Int16)comboPtr[1];

      if (attrVCIndLen > 0) // this check may not be needed
      {
        offset = ExpTupleDesc::getVarOffset( data,
                                             offset,
                                             voaOffset,
                                             attrVCIndLen,  
                                             attrNullIndLen );

        len = ExpTupleDesc::getVarLength( data + offset,
                                          attrVCIndLen );
        offset += attrVCIndLen;
      }

      data += offset;

      UInt32 flags = ExHDPHash::NO_FLAGS;

      if (pCodeOpc == PCIT::HASH_MBIN32U_MUNIV)
      {
        flags = ExHDPHash::SWAP_TWO;

        // skip trailing blanks
        NAWchar* wstr = (NAWchar*)data;
        len = len >> 1; // Divide len by 2 since unicode strs are dbl-bytes
        while ((len > 0) && (wstr[len-1] == unicode_char_set::space_char()))
          len--;
        len = len << 1; // Shift back to length in bytes
      }
      else {
        // skip trailing blanks
        while ((len > 0) && (data[len-1] == ' '))
          len--;
      }

      *tgt = ExHDPHash::hash(data, flags, len);

      pCode += 7;
      break;
    }

    case PCIT::FILL_MEM_BYTES:
      {
	PTR_DEF_ASSIGN(char, str, 0);
	DEF_ASSIGN(Int32, length, 2);
	DEF_ASSIGN(char, padchar, 3);
        // for all fixed length fields
	str_pad( str, length, padchar );
        pCode += 4;
        break;
      }

    case PCIT::FILL_MEM_BYTES_VARIABLE:
    {
      DEF_ASSIGN(Int32, comboLen1, 4 );
      char* comboPtr1 = (char*)&comboLen1;      
      Int16 tgtNullIndLen = (Int16)comboPtr1[0];
      Int16 tgtVCIndLen   = (Int16)comboPtr1[1];

      BASE_PTR_DEF_ASSIGN(char, tgt, 0 );
      DEF_ASSIGN(Int32, tgtOffset, 1);
      DEF_ASSIGN(Int32, tgtVoaOffset, 2);
      DEF_ASSIGN(Int32, copyLen, 5);
      DEF_ASSIGN(char, padchar, 6);

      // VOA offsets should always be used in order.
      if(tgtVoaOffset > 0) {
        assert(tgtVoaOffset > lastVoaOffset);
        lastVoaOffset = tgtVoaOffset;
      }

      tgt += ExpTupleDesc::getTgtVarOffset( tgt,
                                            tgtOffset,     // tgt offset
                                            tgtVoaOffset,  // tgt voa offset
                                            tgtVCIndLen,
                                            tgtNullIndLen,
                                            varOffset,
                                            copyLen
                                          );                        
      if (tgtVCIndLen > 0)
      {
        ExpTupleDesc::setVarLength(tgt, copyLen, tgtVCIndLen);  // set variable length value
        tgt += tgtVCIndLen;  
      }

      if (copyLen > 0)
        str_pad(tgt, copyLen, padchar );

      pCode += 7;
      break;
    }

    // EQUAL TO ("=") operations
    case PCIT::EQ_MBIN32S_MASCII_MASCII:   
      // both are fixed chars
    {
      PTR_DEF_ASSIGN(Lng32, tgt, 0);
      PTR_DEF_ASSIGN(unsigned char, src1, 2);
      PTR_DEF_ASSIGN(unsigned char, src2, 4);

      DEF_ASSIGN(Int32, length, 6);

      *tgt = (src1[0] != src2[0]) ? 0 : (memcmp(src1, src2, length) == 0);

      pCode += 7;
      break;
    }

    case PCIT::COMP_MBIN32S_MUNI_MUNI_IBIN32S_IBIN32S_IBIN32S:
    {
      Int32 i;

      PTR_DEF_ASSIGN(Lng32, res, 0 );
      PTR_DEF_ASSIGN(NAWchar, src1, 2 );
      PTR_DEF_ASSIGN(NAWchar, src2, 4 );

      DEF_ASSIGN(Int32, src1Len, 6 );
      DEF_ASSIGN(Int32, src2Len, 7 );

      NABoolean diffLens = (src1Len != src2Len);

      const Int32* table = compTable[pCode[8] - ITM_EQUAL];


      // src1 is guaranteed to be smaller or equal in length to src2, so use
      // its length with memcmp.
      Int32 compCode = wc_str_cmp(src1, src2, src1Len) + 1;

      if (diffLens && (compCode == 1)) {
        NAWchar space = unicode_char_set::space_char();
        for (i=src1Len; i < src2Len; i++) {
          if (src2[i] == space)
            continue;

          compCode = (space < src2[i]) ? 0 : 2;
          break;
        }
      }

      *res = table[compCode];

      pCode += 9;
      break;
    }

    case PCIT::COMP_MBIN32S_MASCII_MASCII_IBIN32S_IBIN32S_IBIN32S:
    {
      Int32 i, compCode;

      PTR_DEF_ASSIGN(Lng32, res, 0 );
      PTR_DEF_ASSIGN(unsigned char, src1, 2 );
      PTR_DEF_ASSIGN(unsigned char, src2, 4 );

      DEF_ASSIGN(Int32, src1Len, 6 );
      DEF_ASSIGN(Int32, src2Len, 7 );

      NABoolean diffLens = (src1Len != src2Len);

      // Set table pointer to appropriate position based on operation
      const Int32* table = &(compTable[pCode[8] - ITM_EQUAL][1]);


      // Quick first byte compare 
      if (src1[0] == src2[0]) {
        // src1 is guaranteed to be smaller or equal in length to src2, so use
        // its length with memcmp.
        compCode = memcmp(src1, src2, src1Len);

        // Fix compCode to -1, 0, or 1
        if (compCode != 0) {
          compCode = (compCode < 0) ? -1 : 1;
        }
        // If strings are equal so far and lengths are different, compare more
        else if (diffLens && (compCode == 0)) {
          for (i=src1Len; i < src2Len; i++) {
            if (src2[i] == ' ')
              continue;

            compCode = (' ' < src2[i]) ? -1 : 1;
            break;
          }
        }
      }
      else
        compCode = (src1[0] < src2[0]) ? -1 : 1;

      *res = table[compCode];

      pCode += 9;
      break;
    }

    case PCIT::COMP_MBIN32S_MUNIV_MUNIV_IBIN32S:
    case PCIT::COMP_MBIN32S_MATTR5_MATTR5_IBIN32S:
    {
      PTR_DEF_ASSIGN(Int32, res, 0 );

      // Set table pointer to appropriate position based on operation
      const Int32* table = &(compTable[pCode[12] - ITM_EQUAL][1]);


      UInt32 len1, len2;
      Int32 compCode;

      BASE_PTR_DEF_ASSIGN(char, src1Data, 2);
      BASE_PTR_DEF_ASSIGN(char, src2Data, 7);

      PTR_TO_PCODE(char, comboPtr1, 6);
      PTR_TO_PCODE(char, comboPtr2, 11);

      UInt32 src1VCIndLen = (Int32)comboPtr1[1];
      UInt32 src2VCIndLen = (Int32)comboPtr2[1];
      DEF_ASSIGN(UInt32, offset1, 3);
      DEF_ASSIGN(UInt32, offset2, 8);

      if( src1VCIndLen > 0 )  // varchar
      {
	DEF_ASSIGN(UInt32, voaOffset1, 4);
        src1Data += ExpTupleDesc::getVarOffset(src1Data,             // atp_
                                               offset1, 
                                               voaOffset1, 
                                               src1VCIndLen,         // vcIndLen
                                               (Int32)comboPtr1[0]); // nullIndLen

        len1 = ExpTupleDesc::getVarLength(src1Data, src1VCIndLen); 
        src1Data += src1VCIndLen;
      } 
      else {
	DEF_ASSIGN(UInt32, length1, 5);
        src1Data += offset1; 
        len1 = length1;
      }

      if ( src2VCIndLen > 0 )  // varchar
      {
	DEF_ASSIGN(UInt32, voaOffset2, 9);
        src2Data += ExpTupleDesc::getVarOffset(src2Data,             // atp_
                                               offset2, 
                                               voaOffset2, 
                                               src2VCIndLen,         // vcIndLen
                                               (Int32)comboPtr2[0]); // nullIndLen

        len2 = ExpTupleDesc::getVarLength(src2Data, src2VCIndLen); 
        src2Data += src2VCIndLen;
      }
      else {
	DEF_ASSIGN(UInt32, length2, 10);
        src2Data += offset2; 
        len2 = length2;
      }

      if (pCodeOpc == PCIT::COMP_MBIN32S_MUNIV_MUNIV_IBIN32S)
      {
        compCode = wcharStringCompareWithPad(
                         (NAWchar*)(src1Data), len1 >> 1,
                         (NAWchar*)(src2Data), len2 >> 1,
                         unicode_char_set::space_char());
      }
      else
      {
        compCode = charStringCompareWithPad(src1Data, len1, src2Data, len2, ' ');
      }

      *res = table[compCode];

      pCode += 13;

      break;
    }

  
    case PCIT::EQ_MBIN32S_MBIN8S_MBIN8S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(Int8, x, 2 );
      PTR_DEF_ASSIGN(Int8, y, 4 );

      *result = (*x == *y);

      pCode += 6;
      break;
    }

    case PCIT::NE_MBIN32S_MBIN8S_MBIN8S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(Int8, x, 2 );
      PTR_DEF_ASSIGN(Int8, y, 4 );

      *result = (*x != *y);

      pCode += 6;
      break;
    }

    case PCIT::LT_MBIN32S_MBIN8S_MBIN8S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(Int8, x, 2 );
      PTR_DEF_ASSIGN(Int8, y, 4 );

      *result = (*x < *y);

      pCode += 6;
      break;
    }

    case PCIT::GT_MBIN32S_MBIN8S_MBIN8S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(Int8, x, 2 );
      PTR_DEF_ASSIGN(Int8, y, 4 );

      *result = (*x > *y);

      pCode += 6;
      break;
    }

    case PCIT::LE_MBIN32S_MBIN8S_MBIN8S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(Int8, x, 2 );
      PTR_DEF_ASSIGN(Int8, y, 4 );

      *result = (*x <= *y);

      pCode += 6;
      break;
    }

    case PCIT::GE_MBIN32S_MBIN8S_MBIN8S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(Int8, x, 2 );
      PTR_DEF_ASSIGN(Int8, y, 4 );

      *result = (*x >= *y);

      pCode += 6;
      break;
    }


    case PCIT::EQ_MBIN32S_MBIN8U_MBIN8U:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(UInt8, x, 2 );
      PTR_DEF_ASSIGN(UInt8, y, 4 );

      *result = (*x == *y);

      pCode += 6;
      break;
    }

    case PCIT::NE_MBIN32S_MBIN8U_MBIN8U:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(UInt8, x, 2 );
      PTR_DEF_ASSIGN(UInt8, y, 4 );

      *result = (*x != *y);

      pCode += 6;
      break;
    }

    case PCIT::LT_MBIN32S_MBIN8U_MBIN8U:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(UInt8, x, 2 );
      PTR_DEF_ASSIGN(UInt8, y, 4 );

      *result = (*x < *y);

      pCode += 6;
      break;
    }

    case PCIT::GT_MBIN32S_MBIN8U_MBIN8U:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(UInt8, x, 2 );
      PTR_DEF_ASSIGN(UInt8, y, 4 );

      *result = (*x > *y);

      pCode += 6;
      break;
    }

    case PCIT::LE_MBIN32S_MBIN8U_MBIN8U:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(UInt8, x, 2 );
      PTR_DEF_ASSIGN(UInt8, y, 4 );

      *result = (*x <= *y);

      pCode += 6;
      break;
    }

    case PCIT::GE_MBIN32S_MBIN8U_MBIN8U:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(UInt8, x, 2 );
      PTR_DEF_ASSIGN(UInt8, y, 4 );

      *result = (*x >= *y);

      pCode += 6;
      break;
    }



    case PCIT::EQ_MBIN32S_MBIN16U_MBIN16S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(unsigned short, x, 2 );
      PTR_DEF_ASSIGN(short, y, 4 );

      *result = (*x == *y);

      pCode += 6;
      break;
    }
      
    case PCIT::EQ_MBIN32S_MBIN16S_MBIN16S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(short, x, 2 );
      PTR_DEF_ASSIGN(short, y, 4 );

      *result = (*x == *y);

      pCode += 6;
      break;
    }
      
    case PCIT::EQ_MBIN32S_MBIN16S_MBIN32S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(short, x, 2 );
      PTR_DEF_ASSIGN(Int32, y, 4 );

      *result = (*x == *y);

      pCode += 6;
      break;
    }
      
      
    case PCIT::EQ_MBIN32S_MBIN32S_MBIN32S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(Int32, x, 2 );
      PTR_DEF_ASSIGN(Int32, y, 4 );

      *result = (*x == *y);

      pCode += 6;
      break;
    }
      
    case PCIT::EQ_MBIN32S_MBIN16U_MBIN16U:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(unsigned short, x, 2 );
      PTR_DEF_ASSIGN(unsigned short, y, 4 );

      *result = (*x == *y);

      pCode += 6;
      break;
    }
      
    case PCIT::EQ_MBIN32S_MBIN16U_MBIN32U:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(unsigned short, x, 2 );
      PTR_DEF_ASSIGN(UInt32, y, 4 );

      *result = (*x == *y);

      pCode += 6;
      break;
    }
      
      
    case PCIT::EQ_MBIN32S_MBIN32U_MBIN32U: 
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(UInt32, x, 2 );
      PTR_DEF_ASSIGN(UInt32, y, 4 );

      *result = (*x == *y);

      pCode += 6;
      break;
    }
      
    case PCIT::EQ_MBIN32S_MBIN16S_MBIN32U:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(short, x, 2 );
      PTR_DEF_ASSIGN(UInt32, y, 4 );

      //typecast x to Int64 to account for negetive values
      *result = ((Int64)*x == *y);

      pCode += 6;
      break;
    }
      
    case PCIT::EQ_MBIN32S_MBIN64S_MBIN64S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(Int64, x, 2 );
      PTR_DEF_ASSIGN(Int64, y, 4 );

      *result = (*x == *y);

      pCode += 6;
      break;
    }

    // LESS THAN ("<") operations
    case PCIT::LT_MBIN32S_MASCII_MASCII:     // both are fixed chars
    {
      PTR_DEF_ASSIGN(Lng32, tgt, 0 );
      PTR_DEF_ASSIGN(unsigned char, src1, 2 );
      PTR_DEF_ASSIGN(unsigned char, src2, 4 );

      *tgt = (src1[0] > src2[0]) ? 0 : (memcmp(src1, src2, pCode[6]) < 0);

      pCode += 7;
      break;
    }

    case PCIT::LT_MBIN32S_MBIN16U_MBIN16S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(unsigned short, x, 2 );
      PTR_DEF_ASSIGN(short, y, 4 );

      *result = (*x < *y);

      pCode += 6;
      break;
    }
      
    case PCIT::LT_MBIN32S_MBIN16S_MBIN16S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(short, x, 2 );
      PTR_DEF_ASSIGN(short, y, 4 );

      *result = (*x < *y);

      pCode += 6;
      break;
    }
      
    case PCIT::LT_MBIN32S_MBIN16S_MBIN32S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(short, x, 2 );
      PTR_DEF_ASSIGN(Int32, y, 4 );

      *result = (*x < *y);

      pCode += 6;
      break;
    }
      
      
    case PCIT::LT_MBIN32S_MBIN32S_MBIN32S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(Int32, x, 2 );
      PTR_DEF_ASSIGN(Int32, y, 4 );

      *result = (*x < *y);

      pCode += 6;
      break;
    }
      
    case PCIT::LT_MBIN32S_MBIN16U_MBIN16U:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(unsigned short, x, 2 );
      PTR_DEF_ASSIGN(unsigned short, y, 4 );

      *result = (*x < *y);

      pCode += 6;
      break;
    }
      
    case PCIT::LT_MBIN32S_MBIN16U_MBIN32U:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(unsigned short, x, 2 );
      PTR_DEF_ASSIGN(UInt32, y, 4 );

      *result = (*x < *y);

      pCode += 6;
      break;
    }
      
      
    case PCIT::LT_MBIN32S_MBIN32U_MBIN32U: 
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(UInt32, x, 2 );
      PTR_DEF_ASSIGN(UInt32, y, 4 );

      *result = (*x < *y);

      pCode += 6;
      break;
    }
      
    case PCIT::LT_MBIN32S_MBIN16S_MBIN32U:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(short, x, 2 );
      PTR_DEF_ASSIGN(UInt32, y, 4 );

      //typecast x to Int64 to account for negetive values
      *result = ((Int64)*x < *y);

      pCode += 6;
      break;
    }
      
      
    case PCIT::LT_MBIN32S_MBIN64S_MBIN64S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(Int64, x, 2 );
      PTR_DEF_ASSIGN(Int64, y, 4 );

      *result = (*x < *y);

      pCode += 6;
      break;
    }

    // GREATER THAN (">") operations
    case PCIT::GT_MBIN32S_MASCII_MASCII:     // both are fixed chars
    {
      PTR_DEF_ASSIGN(Lng32, tgt, 0 );
      PTR_DEF_ASSIGN(unsigned char, src1, 2 );
      PTR_DEF_ASSIGN(unsigned char, src2, 4 );

      *tgt = (src1[0] < src2[0]) ? 0 : (memcmp(src1, src2, pCode[6]) > 0);

      pCode += 7;
      break;
    }

    case PCIT::GT_MBIN32S_MBIN16U_MBIN16S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(unsigned short, x, 2 );
      PTR_DEF_ASSIGN(short, y, 4 );

      *result = (*x > *y);

      pCode += 6;
      break;
    }
      
    case PCIT::GT_MBIN32S_MBIN16S_MBIN16S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(short, x, 2 );
      PTR_DEF_ASSIGN(short, y, 4 );

      *result = (*x > *y);

      pCode += 6;
      break;
    }
      
    case PCIT::GT_MBIN32S_MBIN16S_MBIN32S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(short, x, 2 );
      PTR_DEF_ASSIGN(Int32, y, 4 );

      *result = (*x > *y);

      pCode += 6;
      break;
    }
      
      
    case PCIT::GT_MBIN32S_MBIN32S_MBIN32S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(Int32, x, 2 );
      PTR_DEF_ASSIGN(Int32, y, 4 );

      *result = (*x > *y);

      pCode += 6;
      break;
    }
      
    case PCIT::GT_MBIN32S_MBIN16U_MBIN16U:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(unsigned short, x, 2 );
      PTR_DEF_ASSIGN(unsigned short, y, 4 );

      *result = (*x > *y);

      pCode += 6;
      break;
    }
      
    case PCIT::GT_MBIN32S_MBIN16U_MBIN32U:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(unsigned short, x, 2 );
      PTR_DEF_ASSIGN(UInt32, y, 4 );

      *result = (*x > *y);

      pCode += 6;
      break;
    }
      
      
    case PCIT::GT_MBIN32S_MBIN32U_MBIN32U: 
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(UInt32, x, 2 );
      PTR_DEF_ASSIGN(UInt32, y, 4 );

      *result = (*x > *y);

      pCode += 6;
      break;
    }
      
    case PCIT::GT_MBIN32S_MBIN16S_MBIN32U:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(short, x, 2 );
      PTR_DEF_ASSIGN(UInt32, y, 4 );

      //typecast x to Int64 to account for negetive values
      *result = ((Int64)*x > *y);

      pCode += 6;
      break;
    }
      
      
    case PCIT::GT_MBIN32S_MBIN64S_MBIN64S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(Int64, x, 2 );
      PTR_DEF_ASSIGN(Int64, y, 4 );

      *result = (*x > *y);

      pCode += 6;
      break;
    }

    // LESS THAN EQUAL TO("<=") operations   
    case PCIT::LE_MBIN32S_MASCII_MASCII:     // both are fixed chars
    {
      PTR_DEF_ASSIGN(Lng32, tgt, 0 );
      PTR_DEF_ASSIGN(unsigned char, src1, 2 );
      PTR_DEF_ASSIGN(unsigned char, src2, 4 );

      *tgt = (src1[0] > src2[0]) ? 0 : (memcmp(src1, src2, pCode[6]) <= 0);

      pCode += 7;
      break;
    }

    case PCIT::LE_MBIN32S_MBIN16U_MBIN16S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(unsigned short, x, 2 );
      PTR_DEF_ASSIGN(short, y, 4 );

      *result = (*x <= *y);

      pCode += 6;
      break;
    }
      
    case PCIT::LE_MBIN32S_MBIN16S_MBIN16S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(short, x, 2 );
      PTR_DEF_ASSIGN(short, y, 4 );

      *result = (*x <= *y);

      pCode += 6;
      break;
    }
      
    case PCIT::LE_MBIN32S_MBIN16S_MBIN32S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(short, x, 2 );
      PTR_DEF_ASSIGN(Int32, y, 4 );

      *result = (*x <= *y);

      pCode += 6;
      break;
    }
      
      
    case PCIT::LE_MBIN32S_MBIN32S_MBIN32S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(Int32, x, 2 );
      PTR_DEF_ASSIGN(Int32, y, 4 );

      *result = (*x <= *y);

      pCode += 6;
      break;
    }
      
    case PCIT::LE_MBIN32S_MBIN16U_MBIN16U:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(unsigned short, x, 2 );
      PTR_DEF_ASSIGN(unsigned short, y, 4 );

      *result = (*x <= *y);

      pCode += 6;
      break;
    }
      
    case PCIT::LE_MBIN32S_MBIN16U_MBIN32U:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(unsigned short, x, 2 );
      PTR_DEF_ASSIGN(UInt32, y, 4 );

      *result = (*x <= *y);

      pCode += 6;
      break;
    }
      
    case PCIT::LE_MBIN32S_MBIN32U_MBIN32U: 
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(UInt32, x, 2 );
      PTR_DEF_ASSIGN(UInt32, y, 4 );

      *result = (*x <= *y);

      pCode += 6;
      break;
    }
      
    case PCIT::LE_MBIN32S_MBIN16S_MBIN32U:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(short, x, 2 );
      PTR_DEF_ASSIGN(UInt32, y, 4 );

      //typecast x to Int64 to account for negetive values
      *result = ((Int64)*x <= *y);

      pCode += 6;
      break;
    }
      
      
    case PCIT::LE_MBIN32S_MBIN64S_MBIN64S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(Int64, x, 2 );
      PTR_DEF_ASSIGN(Int64, y, 4 );

      *result = (*x <= *y);

      pCode += 6;
      break;
    }

    // GREATER THAN EQUAL TO (">=") operations   
    case PCIT::GE_MBIN32S_MASCII_MASCII:      // both are fixed chars
    {
      PTR_DEF_ASSIGN(Lng32, tgt, 0 );
      PTR_DEF_ASSIGN(unsigned char, src1, 2 );
      PTR_DEF_ASSIGN(unsigned char, src2, 4 );

      *tgt = (src1[0] < src2[0]) ? 0 : (memcmp(src1, src2, pCode[6]) >= 0);

      pCode += 7;
      break;
    }

    case PCIT::GE_MBIN32S_MBIN16U_MBIN16S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(unsigned short, x, 2 );
      PTR_DEF_ASSIGN(short, y, 4 );

      *result = (*x >= *y);

      pCode += 6;
      break;
    }
      
    case PCIT::GE_MBIN32S_MBIN16S_MBIN16S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(short, x, 2 );
      PTR_DEF_ASSIGN(short, y, 4 );

      *result = (*x >= *y);

      pCode += 6;
      break;
    }
      
    case PCIT::GE_MBIN32S_MBIN16S_MBIN32S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(short, x, 2 );
      PTR_DEF_ASSIGN(Int32, y, 4 );

      *result = (*x >= *y);

      pCode += 6;
      break;
    }
      
      
    case PCIT::GE_MBIN32S_MBIN32S_MBIN32S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(Int32, x, 2 );
      PTR_DEF_ASSIGN(Int32, y, 4 );

      *result = (*x >= *y);

      pCode += 6;
      break;
    }
      
    case PCIT::GE_MBIN32S_MBIN16U_MBIN16U:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(unsigned short, x, 2 );
      PTR_DEF_ASSIGN(unsigned short, y, 4 );

      *result = (*x >= *y);

      pCode += 6;
      break;
    }
      
    case PCIT::GE_MBIN32S_MBIN16U_MBIN32U:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(unsigned short, x, 2 );
      PTR_DEF_ASSIGN(UInt32, y, 4 );

      *result = (*x >= *y);

      pCode += 6;
      break;
    }
      
      
    case PCIT::GE_MBIN32S_MBIN32U_MBIN32U: 
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(UInt32, x, 2 );
      PTR_DEF_ASSIGN(UInt32, y, 4 );

      *result = (*x >= *y);

      pCode += 6;
      break;
    }
      
    case PCIT::GE_MBIN32S_MBIN16S_MBIN32U:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(short, x, 2 );
      PTR_DEF_ASSIGN(UInt32, y, 4 );

      //typecast x to Int64 to account for negetive values
      *result = ((Int64)*x >= *y);

      pCode += 6;
      break;
    }
      
      
    case PCIT::GE_MBIN32S_MBIN64S_MBIN64S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(Int64, x, 2 );
      PTR_DEF_ASSIGN(Int64, y, 4 );

      *result = (*x >= *y);

      pCode += 6;
      break;
    }


    // NOT EQUAL TO ("<>") operations   
    case PCIT::NE_MBIN32S_MASCII_MASCII:   // both are fixed chars
    {
      PTR_DEF_ASSIGN(Lng32, tgt, 0 );
      PTR_DEF_ASSIGN(unsigned char, src1, 2 );
      PTR_DEF_ASSIGN(unsigned char, src2, 4 );

      *tgt = (src1[0] != src2[0]) ? 1 : (memcmp(src1, src2, pCode[6]) != 0);

      pCode += 7;
      break;
    }

    case PCIT::NE_MBIN32S_MBIN64S_MBIN64S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(Int64, x, 2 );
      PTR_DEF_ASSIGN(Int64, y, 4 );

      *result = (*x != *y);

      pCode += 6;
      break;
    }

    case PCIT::NE_MBIN32S_MBIN16S_MBIN16S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(short, x, 2 );
      PTR_DEF_ASSIGN(short, y, 4 );

      *result = (*x != *y);

      pCode += 6;
      break;
    }

    // EQUAL TO ZERO operations   
    case PCIT::ZERO_MBIN32S_MBIN32U:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(UInt32, x, 2 );

      *result = (*x == 0);
      break;
    }

    // NOT EQUAL TO ZERO operations   
    case PCIT::NOTZERO_MBIN32S_MBIN32U:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(UInt32, x, 2 );

      *result = (*x != 0);
      break;
    }


    case PCIT::AND_MBIN32S_MBIN32S_MBIN32S:
      {
	PTR_DEF_ASSIGN(Int32, resultPtr, 0 );
	PTR_DEF_ASSIGN(Int32, opAPtr, 2 );
	PTR_DEF_ASSIGN(Int32, opBPtr, 4 );

	* resultPtr =  *opAPtr & *opBPtr  ?  *opAPtr | *opBPtr : 0 ; 

	pCode += 6;
      }
      break;

    case PCIT::OR_MBIN32S_MBIN32S_MBIN32S:
      {
	PTR_DEF_ASSIGN(Int32, resultPtr, 0 );
	PTR_DEF_ASSIGN(Int32, opAPtr, 2 );
	PTR_DEF_ASSIGN(Int32, opBPtr, 4 );

	* resultPtr =  *opAPtr == 1 || *opBPtr == 1 ? 1 : *opAPtr | *opBPtr ; 

	pCode += 6;
      }
      break;

    case PCIT::MOD_MBIN32S_MBIN32S_MBIN32S:
    {
      PTR_DEF_ASSIGN(Int32, resultPtr, 0 );
      PTR_DEF_ASSIGN(Int32, opAPtr, 2 );
      PTR_DEF_ASSIGN(Int32, opBPtr, 4 );

      * resultPtr = *opAPtr % *opBPtr ;

      pCode += 6;
      break;
    }
    case PCIT::MOD_MBIN32U_MBIN32U_MBIN32S:
    {
      PTR_DEF_ASSIGN(UInt32, resultPtr, 0 );
      PTR_DEF_ASSIGN(UInt32, opAPtr, 2 );
      PTR_DEF_ASSIGN(Int32, opBPtr, 4 );

      * resultPtr = *opAPtr % *opBPtr ;

      pCode += 6;
      break;
    }
    case PCIT::HASHCOMB_MBIN32U_MBIN32U_MBIN32U:
    {
      PTR_DEF_ASSIGN(UInt32, tgtPtr, 0 );
      PTR_DEF_ASSIGN(UInt32, src1Ptr, 2 );
      PTR_DEF_ASSIGN(UInt32, src2Ptr, 4 );

      *tgtPtr = ( *src1Ptr << 1 | *src1Ptr >> 31 ) ^ *src2Ptr;

      pCode += 6;
      break;
    }

    case PCIT::ENCODE_MASCII_MBIN8S_IBIN32S:
      {
	PTR_DEF_ASSIGN(Int8, outputPtr, 0 );
	PTR_DEF_ASSIGN(Int8, resultPtr, 2 );
	DEF_ASSIGN(Int32, bitwiseNOT,   4 );
	Int8 result = *resultPtr;

	result ^= 0x80;

	if ( bitwiseNOT ) 
          result = ~result;

	* outputPtr = result ;

	pCode += 5;
      }
      break;

    case PCIT::ENCODE_MASCII_MBIN8U_IBIN32S:
      {
	PTR_DEF_ASSIGN(Int8, outputPtr, 0 );
	PTR_DEF_ASSIGN(Int8, resultPtr, 2 );
	DEF_ASSIGN(Int32, bitwiseNOT , 4 );
	UInt8 result = *resultPtr;

	if ( bitwiseNOT ) 
          result = ~result;

	* outputPtr = result ;

	pCode += 5;
      }
      break;

    case PCIT::ENCODE_MASCII_MBIN16S_IBIN32S:
      {
	PTR_DEF_ASSIGN(Int16, outputPtr, 0 );
	PTR_DEF_ASSIGN(Int16, resultPtr, 2 );
	DEF_ASSIGN(Int32, bitwiseNOT , 4 );
	Int16 result = *resultPtr;

	result ^= 0x8000;
#ifdef NA_LITTLE_ENDIAN
	result = reversebytes((UInt16)result);
#endif // NA_LITTLE_ENDIAN      

	if ( bitwiseNOT ) result = ~result;

	* outputPtr = result ;

	pCode += 5;
      }
      break;

    case PCIT::ENCODE_MASCII_MBIN16U_IBIN32S:
      {
	PTR_DEF_ASSIGN(Int16, outputPtr, 0 );
	PTR_DEF_ASSIGN(Int16, resultPtr, 2 );
	DEF_ASSIGN(Int32, bitwiseNOT , 4 );
	Int16 result = *resultPtr;

#ifdef NA_LITTLE_ENDIAN
	result = reversebytes((UInt16)result);
#endif // NA_LITTLE_ENDIAN      

	if ( bitwiseNOT ) result = ~result;

	* outputPtr = result ;

	pCode += 5;
      }
      break;

    case PCIT::ENCODE_MASCII_MBIN32S_IBIN32S:
      {
	PTR_DEF_ASSIGN(Int32, outputPtr, 0 );
	PTR_DEF_ASSIGN(Int32, resultPtr, 2 );
	DEF_ASSIGN(Int32, bitwiseNOT , 4 );
	Int32 result = *resultPtr;

	result ^= 0x80000000;

#ifdef NA_LITTLE_ENDIAN
	result = reversebytes((UInt32)result);
#endif // NA_LITTLE_ENDIAN      
	if ( bitwiseNOT ) result = ~result;

	* outputPtr = result ;

	pCode += 5;
      }
    break;

    case PCIT::ENCODE_MASCII_MBIN32U_IBIN32S:
      {
	PTR_DEF_ASSIGN(Int32, outputPtr, 0 );
	PTR_DEF_ASSIGN(Int32, resultPtr, 2 );
	DEF_ASSIGN(Int32, bitwiseNOT , 4 );
	Int32 result = *resultPtr;

#ifdef NA_LITTLE_ENDIAN
	result = reversebytes((UInt32)result);
#endif // NA_LITTLE_ENDIAN      
	if ( bitwiseNOT ) result = ~result;

	* outputPtr = result ;

	pCode += 5;
      }
    break;

    case PCIT::ENCODE_MASCII_MBIN64S_IBIN32S:
      {
	PTR_DEF_ASSIGN(Int64, outputPtr, 0 );
	PTR_DEF_ASSIGN(Int64, resultPtr, 2 );
	DEF_ASSIGN(Int32, bitwiseNOT , 4 );
	Int64 result = *resultPtr;

	result ^= 0x8000000000000000LL;
#ifdef NA_LITTLE_ENDIAN
	result = reversebytes(result);
#endif // NA_LITTLE_ENDIAN      
	if ( bitwiseNOT ) result = ~result;

	* outputPtr = result ;

	pCode += 5;
      }
      break;

    case PCIT::ENCODE_NXX:
       {
	 PTR_DEF_ASSIGN(char, tgt, 0 );
	 PTR_DEF_ASSIGN(char, src, 2 );
	 DEF_ASSIGN(Int32, length, 4 );
	 DEF_ASSIGN(Int32, desc, 5 );
	 if ( desc ) 
	  {
	    for (Int32 k = 0; k < length; k++)
	      tgt[k] = (char)(~(src[k]));
	  }
	else
	  {
	    str_cpy_all( tgt, src, length );
	  }
	pCode += 6;
      }
    break;

    case PCIT::ENCODE_DECS:
      {
	PTR_DEF_ASSIGN(char, tgt, 0 );
	PTR_DEF_ASSIGN(char, src, 2 );
	DEF_ASSIGN(Int32, length, 4 );
	DEF_ASSIGN(Int32, desc, 5 );
	if (src[0] & 0200) // negative
	  {
	    if ( desc )
	      {
		str_cpy_all(tgt, src, length );
	      }
	    else
	      {
		for (Int32 k = 0; k < length; k++)
		  tgt[k] = (char)(~(src[k]));
	      }
	  }
	else // positive
	  {
	    if ( desc ) 
	      {
		str_cpy_all(tgt, src, length );
		tgt[0] = (char)(~(src[0] | 0200));

		for (Int32 k = 1; k < length; k++)
		  tgt[k] = (char)(~(tgt[k]));
	      }
	    else
	      {
		str_cpy_all(tgt, src, length );

		tgt[0] |= 0200;
	      }
	  }
	
	pCode += 6;
      }
    break;

    case PCIT::DECODE_MASCII_MBIN16S_IBIN32S:
      {
	PTR_DEF_ASSIGN(Int16, outputPtr, 0 );
	PTR_DEF_ASSIGN(Int16, resultPtr, 2 );
	DEF_ASSIGN(Int32, bitwiseNOT , 4 );
	Int16 result = *resultPtr;

	if ( bitwiseNOT ) result = ~result;

#ifdef NA_LITTLE_ENDIAN
	result = reversebytes((UInt16)result);
#endif // NA_LITTLE_ENDIAN      

	result ^= 0x8000;

	* outputPtr = result ;

	pCode += 5;
      }
      break;

    case PCIT::DECODE_MASCII_MBIN16U_IBIN32S:
      {
	PTR_DEF_ASSIGN(Int16, outputPtr, 0 );
	PTR_DEF_ASSIGN(Int16, resultPtr, 2 );
	DEF_ASSIGN(Int32, bitwiseNOT , 4 );
	Int16 result = *resultPtr;

	if ( bitwiseNOT ) result = ~result;

#ifdef NA_LITTLE_ENDIAN
	result = reversebytes((UInt16)result);
#endif // NA_LITTLE_ENDIAN      

	* outputPtr = result ;

	pCode += 5;
      }
      break;

    case PCIT::DECODE_MASCII_MBIN32S_IBIN32S:
      {
	PTR_DEF_ASSIGN(Int32, outputPtr, 0 );
	PTR_DEF_ASSIGN(Int32, resultPtr, 2 );
	DEF_ASSIGN(Int32, bitwiseNOT , 4 );
	Int32 result = *resultPtr;

	if ( bitwiseNOT ) result = ~result;

#ifdef NA_LITTLE_ENDIAN
	result = reversebytes((UInt32)result);
#endif // NA_LITTLE_ENDIAN      

	result ^= 0x80000000;

	* outputPtr = result ;

	pCode += 5;
      }
    break;

    case PCIT::DECODE_MASCII_MBIN32U_IBIN32S:
      {
	PTR_DEF_ASSIGN(Int32, outputPtr, 0 );
	PTR_DEF_ASSIGN(Int32, resultPtr, 2 );
	DEF_ASSIGN(Int32, bitwiseNOT , 4 );
	Int32 result = *resultPtr;

	if ( bitwiseNOT ) result = ~result;

#ifdef NA_LITTLE_ENDIAN
	result = reversebytes((UInt32)result);
#endif // NA_LITTLE_ENDIAN      

	* outputPtr = result ;

	pCode += 5;
      }
    break;

    case PCIT::DECODE_MASCII_MBIN64S_IBIN32S:
      {
	PTR_DEF_ASSIGN(Int64, outputPtr, 0 );
	PTR_DEF_ASSIGN(Int64, resultPtr, 2 );
	DEF_ASSIGN(Int32, bitwiseNOT , 4 );
	Int64 result = *resultPtr;

	if ( bitwiseNOT ) result = ~result;

#ifdef NA_LITTLE_ENDIAN
	result = reversebytes(result);
#endif // NA_LITTLE_ENDIAN      

	result ^= 0x8000000000000000LL;
	* outputPtr = result ;

	pCode += 5;
      }
      break;

    case PCIT::DECODE_NXX:
       {
	 PTR_DEF_ASSIGN(char, tgt, 0 );
	 PTR_DEF_ASSIGN(char, src, 2 );
	 DEF_ASSIGN(Int32, length, 4 );
	 DEF_ASSIGN(Int32, desc, 5 );
	 if ( desc ) 
	  {
	    for (Int32 k = 0; k < length; k++)
	      tgt[k] = (char)(~(src[k]));
	  }
	else
	  {
	    str_cpy_all( tgt, src, length );
	  }
	pCode += 6;
      }
    break;

    case PCIT::DECODE_DECS:
      {
	PTR_DEF_ASSIGN(char, tgt, 0 );
	PTR_DEF_ASSIGN(char, src, 2 );
	DEF_ASSIGN(Int32, length, 4 );
	DEF_ASSIGN(Int32, desc, 5 );

	if (desc)
	  {
	    for (Int32 k = 0; k < length; k++)
	      tgt[k] = (char)(~(src[k]));
	  }

	if (NOT(src[0] & 0200)) 
	  {
	    for (Lng32 i = 0; i < length; i++)
	      tgt[i] = ~src[i];
	  } 
	else 
	  {
	    if (tgt != src)
	      str_cpy_all(tgt, src, length);
	    tgt[0] &= ~0200;
	  }

	pCode += 6;
      }
    break;

    case PCIT::BRANCH_INDIRECT_MBIN32S:
    {
      PTR_DEF_ASSIGN(Int32, srcPtr, 0 );
      pCode += (Int64)*srcPtr;
      break;
    }
    
    case PCIT::BRANCH:
    {
      DEF_ASSIGN(Int64, branchOffset, 0 );
      pCode += branchOffset ;
      break;
    }
    case PCIT::BRANCH_AND:
      {
        // on 64-bit the first 2 operands are pointers
	PTR_DEF_ASSIGN(Int32, tgtPtr, 2 * PCODEBINARIES_PER_PTR );
	PTR_DEF_ASSIGN(Int32, srcPtr, 2 + 2 * PCODEBINARIES_PER_PTR );
	Int32 src = *srcPtr;

        if (src == 0)
          {
	    DEF_ASSIGN_PTR(Int64, branchOffset, 0 );
	    *tgtPtr = 0;
	    pCode += branchOffset;
          }
        else
          {
	    * tgtPtr = src ;
            pCode += 4 + PCODEBINARIES_PER_PTR + PCODEBINARIES_PER_PTR;
          }
      }
    break;

    case PCIT::BRANCH_AND_CNT:
      {
	PTR_DEF_ASSIGN(Int32, tgtPtr, 2 );
	PTR_DEF_ASSIGN(Int32, srcPtr, 4 );
	Int32 src = *srcPtr;

        // Increment seen counter
        pCode[6]++;

        // Enable runtime opts if seen count equals trigger count
        enableRuntimeOpts = enableRuntimeOpts | (pCode[6] == pCode[1]);

        if (src == 0)
          {
	    DEF_ASSIGN(Int64, branchOffset, 0 );
            pCode[7]++;  // Increment taken count
	    * tgtPtr = 0;
	    pCode += branchOffset;
          }
        else
          {
	    * tgtPtr = src;
            pCode += 8;
          }
      }
    break;

    case PCIT::BRANCH_OR:
      {
        // on 64-bit the first 2 operands are pointers
	PTR_DEF_ASSIGN(Int32, tgtPtr, 2 * PCODEBINARIES_PER_PTR );
	PTR_DEF_ASSIGN(Int32, srcPtr, 2 + 2 * PCODEBINARIES_PER_PTR );
	Int32 src = *srcPtr;

        if (src == 1)
          {
	    DEF_ASSIGN_PTR(Int64, branchOffset, 0 );
	    * tgtPtr = 1;
	    pCode += branchOffset;
          }
        else
          {
	    * tgtPtr = src ;
            pCode += 4 + 2 * PCODEBINARIES_PER_PTR;
          }
      }
    break;

    case PCIT::BRANCH_OR_CNT:
      {
	PTR_DEF_ASSIGN(Int32, tgtPtr, 2 );
	PTR_DEF_ASSIGN(Int32, srcPtr, 4 );
	Int32 src = *srcPtr;

        // Increment seen counter
        pCode[6]++;

        // Enable runtime opts if seen count equals trigger count
        enableRuntimeOpts = enableRuntimeOpts | (pCode[6] == pCode[1]);

        if (src == 1)
          {
	    DEF_ASSIGN(Int64, branchOffset, 0 );
            pCode[7]++;  // Increment taken count
	    * tgtPtr = 1;
	    pCode += branchOffset;
          }
        else
          {
	    * tgtPtr = src;
            pCode += 8;
          }
      }
    break;

    case PCIT::RANGE_LOW_S64S64:
    {
      DEF_ASSIGN(Lng32, op, RANGE_INST_LEN );  // the next op
      PTR_DEF_ASSIGN(Int64, valPtr, 0 );
      PTR_TO_PCODE(Int64, minvalPtr, 2 );

      if ( *valPtr < *minvalPtr )
	{
	  goto Error1_; 
	}

      pCode += RANGE_INST_LEN;

      if (op != PCIT::RANGE_HIGH_S64S64) {
        break;
      }

      pCode++;
      //
      // Do NOT break here...
      // The next instruction is a matching RANGE_HIGH
    }

    case PCIT::RANGE_HIGH_S64S64:
    {
      PTR_DEF_ASSIGN(Int64, valPtr, 0 );
      PTR_TO_PCODE(Int64, maxvalPtr, 2 );
      if ( *valPtr > *maxvalPtr )
        {
	  goto Error1_;
        }
      pCode += RANGE_INST_LEN;
      break;
    }
    case PCIT::RANGE_LOW_S32S64:
    {
      DEF_ASSIGN(Lng32, op, RANGE_INST_LEN );  // the next op
      PTR_DEF_ASSIGN(Int32, valPtr, 0 );
      PTR_TO_PCODE(Int64, minvalPtr, 2 );

      if ( *valPtr < *minvalPtr )
	{
	  goto Error1_; 
	}

      pCode += RANGE_INST_LEN; 

      if (op != PCIT::RANGE_HIGH_S32S64) {
        break;
      }

      pCode++;
      //
      // Do NOT break here...
      // The next instruction is a matching RANGE_HIGH
    }

    case PCIT::RANGE_HIGH_S32S64:
    {
      PTR_DEF_ASSIGN(Int32, valPtr, 0 );
      PTR_TO_PCODE(Int64, maxvalPtr, 2 );
      if ( *valPtr > *maxvalPtr )
        {
	  goto Error1_;
        }
      pCode += RANGE_INST_LEN;
      break;
    }
    case PCIT::RANGE_LOW_U32S64:
    {
      DEF_ASSIGN(Lng32, op, RANGE_INST_LEN );  // the next op
      PTR_DEF_ASSIGN(UInt32, valPtr, 0 );
      PTR_TO_PCODE(Int64, minvalPtr, 2 );

      if ( *valPtr < *minvalPtr )
        {
	  goto Error1_;
        }

      pCode += RANGE_INST_LEN;

      if (op != PCIT::RANGE_HIGH_U32S64) {
        break;
      }

      pCode++;
      //
      // Do NOT break here...
      // The next instruction is a matching RANGE_HIGH
    }

    case PCIT::RANGE_HIGH_U32S64:
    {
      PTR_DEF_ASSIGN(UInt32, valPtr, 0 );
      PTR_TO_PCODE(Int64, maxvalPtr, 2 );

      if ( *valPtr > *maxvalPtr )
        {
	  goto Error1_;
        }
      pCode += RANGE_INST_LEN;
      break;
    }
    case PCIT::RANGE_LOW_S8S64:
    {
      DEF_ASSIGN(Lng32, op, RANGE_INST_LEN );  // the next op
      PTR_DEF_ASSIGN(Int8, valPtr, 0 );
      PTR_TO_PCODE(Int64, minvalPtr, 2 );

      if ( *valPtr < *minvalPtr )
	{
	  goto Error1_;
	}

      pCode += RANGE_INST_LEN;

      if (op != PCIT::RANGE_HIGH_S8S64) {
        break;
      }

      pCode++;
      //
      // Do NOT break here...
      // The next instruction is a matching RANGE_HIGH
    }

    case PCIT::RANGE_HIGH_S8S64:
    {
      PTR_DEF_ASSIGN(Int8, valPtr, 0 );
      PTR_TO_PCODE(Int64, maxvalPtr, 2 );
      if ( *valPtr > *maxvalPtr )
        {
          goto Error1_;
        }
      pCode += RANGE_INST_LEN;
      break;
    }  

    case PCIT::RANGE_LOW_S16S64:
    {
      DEF_ASSIGN(Lng32, op, RANGE_INST_LEN );  // the next op
      PTR_DEF_ASSIGN(short, valPtr, 0 );
      PTR_TO_PCODE(Int64, minvalPtr, 2 );

      if ( *valPtr < *minvalPtr )
	{
	  goto Error1_;
	}

      pCode += RANGE_INST_LEN;

      if (op != PCIT::RANGE_HIGH_S16S64) {
        break;
      }

      pCode++;
      //
      // Do NOT break here...
      // The next instruction is a matching RANGE_HIGH
    }

    case PCIT::RANGE_HIGH_S16S64:
    {
      PTR_DEF_ASSIGN(short, valPtr, 0 );
      PTR_TO_PCODE(Int64, maxvalPtr, 2 );
      if ( *valPtr > *maxvalPtr )
        {
          goto Error1_;
        }
      pCode += RANGE_INST_LEN;
      break;
    }  

    case PCIT::RANGE_LOW_U16S64:
    {
      DEF_ASSIGN(Lng32, op, RANGE_INST_LEN );  // the next op
      PTR_DEF_ASSIGN(unsigned short, valPtr, 0 );
      PTR_TO_PCODE(Int64, minvalPtr, 2 );

      if ( *valPtr < *minvalPtr )
        {
	  goto Error1_;
        }

      pCode += RANGE_INST_LEN;

      if (op != PCIT::RANGE_HIGH_U16S64) {
        break;
      }

      pCode++;
      //
      // Do NOT break here...
      // The next instruction is a matching RANGE_HIGH
    }

    case PCIT::RANGE_HIGH_U16S64:
    {
      PTR_DEF_ASSIGN(unsigned short, valPtr, 0 );
      PTR_TO_PCODE(Int64, maxvalPtr, 2 );

      if ( *valPtr > *maxvalPtr )
        {
	  goto Error1_;
        }
      pCode += RANGE_INST_LEN;
      break;
    }

    case PCIT::RANGE_LOW_U8S64:
    {
      DEF_ASSIGN(Lng32, op, RANGE_INST_LEN );  // the next op
      PTR_DEF_ASSIGN(UInt8, valPtr, 0 );
      PTR_TO_PCODE(Int64, minvalPtr, 2 );

      if ( *valPtr < *minvalPtr )
        {
	  goto Error1_;
        }

      pCode += RANGE_INST_LEN;

      if (op != PCIT::RANGE_HIGH_U8S64) {
        break;
      }

      pCode++;
      //
      // Do NOT break here...
      // The next instruction is a matching RANGE_HIGH
    }

    case PCIT::RANGE_HIGH_U8S64:
    {
      PTR_DEF_ASSIGN(UInt8, valPtr, 0 );
      PTR_TO_PCODE(Int64, maxvalPtr, 2 );

      if ( *valPtr > *maxvalPtr )
        {
	  goto Error1_;
        }
      pCode += RANGE_INST_LEN;
      break;
    }

    case PCIT::REPLACE_NULL_MATTR3_MBIN32S:
    case PCIT::REPLACE_NULL_MATTR3_MBIN32U:
    case PCIT::REPLACE_NULL_MATTR3_MBIN16S: 
    case PCIT::REPLACE_NULL_MATTR3_MBIN16U:
    {
      PTR_DEF_ASSIGN(char, target, 0 );
      PTR_DEF_ASSIGN(char, bitmap, 2 );
      DEF_ASSIGN(UInt16, idx, 4 );
      char *source;
      DEF_ASSIGN(Int32, length , 9 );

      if ( ExpTupleDesc::isNullValue( bitmap, idx ) )
      {
	PTR_DEF_ASSIGN(char, nlres, 7 );
	source = nlres;
      }
      else
      {
	PTR_DEF_ASSIGN(char, notnlres, 5 );
	source = notnlres;
      }
        
      str_cpy_all(target, source, length);
      pCode += 10;
     }
     break;

    case PCIT::ADD_MFLT64_MFLT64_MFLT64:
    {
      double flt64_0;
      PTR_DEF_ASSIGN(double, result, 0 );
      PTR_DEF_ASSIGN(double, op1, 2 );
      PTR_DEF_ASSIGN(double, op2, 4 );
      flt64_0 = MathReal64Add( *op1, *op2, &ov );
      if (ov)
      {
        goto Error1_; // overflow
      }

      * result = flt64_0;

      pCode += 6;
      break;
    }

    case PCIT::SUB_MFLT64_MFLT64_MFLT64:
    {
      double flt64_0;
      PTR_DEF_ASSIGN(double, result, 0 );
      PTR_DEF_ASSIGN(double, op1, 2 );
      PTR_DEF_ASSIGN(double, op2, 4 );
      flt64_0 = MathReal64Sub( *op1, *op2, &ov );
      if (ov) {
        goto Error1_;
      }

      * result = flt64_0;

      pCode += 6;
      break;
    }

    case PCIT::MUL_MFLT64_MFLT64_MFLT64:
    {
      double flt64_0;
      PTR_DEF_ASSIGN(double, result, 0 );
      PTR_DEF_ASSIGN(double, op1, 2 );
      PTR_DEF_ASSIGN(double, op2, 4 );
      flt64_0 = MathReal64Mul( *op1, *op2, &ov );
      if (ov) {
        goto Error1_;
      }

      * result = flt64_0;

      pCode += 6;
      break;
    }

    case PCIT::DIV_MFLT64_MFLT64_MFLT64:
    {
      double flt64_0;
      PTR_DEF_ASSIGN(double, result, 0 );
      PTR_DEF_ASSIGN(double, op1, 2 );
      PTR_DEF_ASSIGN(double, op2, 4 );
      flt64_0 = MathReal64Div( *op1, *op2, &ov );
      if (ov) {
        goto Error1_;
      }

      * result = flt64_0;

      pCode += 6;
      break;
    }
    
    case PCIT::NEGATE_MASCII_MASCII:
    {
      PTR_DEF_ASSIGN(char, result, 0 );
      PTR_DEF_ASSIGN(char, op1, 2 );
      if (*(Int8*)op1 == 1)
        *(Int8*)result = 0;
      else 
        *(Int8*)result = 1;
      pCode += 4;
      break;
    }

    case PCIT::RANGE_MFLT64:
    {
      double flt64_1;

      flt64_1 = *((double*)(stack[pCode[0]] + pCode[1]));
      // If floating point value is NAN, then return overflow result
      if (flt64_1 != flt64_1)
        {
          goto Error1_;
      }
      pCode += 2;
      break;
    }

    case PCIT::PROFILE:
      getPCodeObject()->profileCounts()[pCode[0]]++;
      pCode++;
      break;

    case PCIT::NE_MBIN32S_MFLT32_MFLT32:
    {
      // NOT EQUAL TO ("<>") operation  
      PTR_DEF_ASSIGN(float, fltPtr1, 2 );
      PTR_DEF_ASSIGN(float, fltPtr2, 4 );
      PTR_DEF_ASSIGN(Int32, result, 0 );

      * result = *fltPtr1 != *fltPtr2 ;

      pCode += 6;
      break;
    }
    case PCIT::NE_MBIN32S_MFLT64_MFLT64:
    {
      // NOT EQUAL TO ("<>") operation  
      PTR_DEF_ASSIGN(double, dblPtr1, 2 );
      PTR_DEF_ASSIGN(double, dblPtr2, 4 );
      PTR_DEF_ASSIGN(Int32, result, 0 );

      * result = *dblPtr1 != *dblPtr2 ;

      pCode += 6;
      break;
    }

    case PCIT::GT_MBIN32S_MFLT32_MFLT32:
    {
      // GREATER THAN (">") operation  
      PTR_DEF_ASSIGN(float, fltPtr1, 2 );
      PTR_DEF_ASSIGN(float, fltPtr2, 4 );
      PTR_DEF_ASSIGN(Int32, result, 0 );

      * result = *fltPtr1 > *fltPtr2 ;

      pCode += 6;
      break;
    }
    case PCIT::GT_MBIN32S_MFLT64_MFLT64:
    {
      // GREATER THAN (">") operation  
      PTR_DEF_ASSIGN(double, dblPtr1, 2 );
      PTR_DEF_ASSIGN(double, dblPtr2, 4 );
      PTR_DEF_ASSIGN(Int32, result, 0 );

      * result = *dblPtr1 > *dblPtr2 ;

      pCode += 6;
      break;
    }

    case PCIT::GE_MBIN32S_MFLT32_MFLT32:
    {
      // GREATER THAN OR EQUAL TO (">=") operation  
      PTR_DEF_ASSIGN(float, fltPtr1, 2 );
      PTR_DEF_ASSIGN(float, fltPtr2, 4 );
      PTR_DEF_ASSIGN(Int32, result, 0 );

      * result = *fltPtr1 >= *fltPtr2 ;

      pCode += 6;
      break;
    }
    case PCIT::GE_MBIN32S_MFLT64_MFLT64:
    {
      // GREATER THAN OR EQUAL TO (">=") operation  
      PTR_DEF_ASSIGN(double, dblPtr1, 2 );
      PTR_DEF_ASSIGN(double, dblPtr2, 4 );
      PTR_DEF_ASSIGN(Int32, result, 0 );

      * result = *dblPtr1 >= *dblPtr2 ;

      pCode += 6;
      break;
    }

    case PCIT::EQ_MBIN32S_MFLT32_MFLT32:
    {
      // EQUAL TO ("=") operation  
      PTR_DEF_ASSIGN(float, fltPtr1, 2 );
      PTR_DEF_ASSIGN(float, fltPtr2, 4 );
      PTR_DEF_ASSIGN(Int32, result, 0 );

      * result = *fltPtr1 == *fltPtr2 ;

      pCode += 6;
      break;
    }
    case PCIT::EQ_MBIN32S_MFLT64_MFLT64:
    {
      // EQUAL TO ("=") operation  
      PTR_DEF_ASSIGN(double, dblPtr1, 2 );
      PTR_DEF_ASSIGN(double, dblPtr2, 4 );
      PTR_DEF_ASSIGN(Int32, result, 0 );

      * result = *dblPtr1 == *dblPtr2 ;

      pCode += 6;
      break;
    }

    case PCIT::LE_MBIN32S_MFLT32_MFLT32:
    {
      // LESS THAN OR EQUAL TO ("<=") operation  
      PTR_DEF_ASSIGN(float, fltPtr1, 2 );
      PTR_DEF_ASSIGN(float, fltPtr2, 4 );
      PTR_DEF_ASSIGN(Int32, result, 0 );

      * result = *fltPtr1 <= *fltPtr2 ;

      pCode += 6;
      break;
    }
    case PCIT::LE_MBIN32S_MFLT64_MFLT64:
    {
      // LESS THAN OR EQUAL TO ("<=") operation  
      PTR_DEF_ASSIGN(double, dblPtr1, 2 );
      PTR_DEF_ASSIGN(double, dblPtr2, 4 );
      PTR_DEF_ASSIGN(Int32, result, 0 );

      * result = *dblPtr1 <= *dblPtr2 ;

      pCode += 6;
      break;
    }

    case PCIT::LT_MBIN32S_MFLT32_MFLT32:
    {
      // LESS THAN ("<") operation  
      PTR_DEF_ASSIGN(float, fltPtr1, 2 );
      PTR_DEF_ASSIGN(float, fltPtr2, 4 );
      PTR_DEF_ASSIGN(Int32, result, 0 );

      * result = *fltPtr1 < *fltPtr2 ;

      pCode += 6;
      break;
    }
    case PCIT::LT_MBIN32S_MFLT64_MFLT64:
    {
      // LESS THAN ("<") operation  
      PTR_DEF_ASSIGN(double, dblPtr1, 2 );
      PTR_DEF_ASSIGN(double, dblPtr2, 4 );
      PTR_DEF_ASSIGN(Int32, result, 0 );

      * result = *dblPtr1 < *dblPtr2 ;

      pCode += 6;
      break;
    }

    case PCIT::NULLIFZERO_MPTR32_MATTR3_MPTR32_IBIN32S:
    // copies in value from src to target, then sets / clears null if need be
    {
      PTR_DEF_ASSIGN(char, tgt, 0 );
      PTR_DEF_ASSIGN(char, tgtNull, 2 );
      PTR_DEF_ASSIGN(char, src, 5 );
      DEF_ASSIGN(Int16, indx, 4 );
      DEF_ASSIGN(Int32, srcLen, 7 );

      NABoolean resultIsNull = TRUE;
      for (Int32 i=0; i < srcLen; i++)
      {
        tgt[i] = src[i];
        if (src[i] != 0) {
          resultIsNull = FALSE;
        }
      }

      if (resultIsNull)
	ExpTupleDesc::setNullValue(tgtNull, indx);
      else
	ExpTupleDesc::clearNullValue(tgtNull, indx);

      pCode += 8;

      break;
    }

    case PCIT::GENFUNC_PCODE_1:
      {
	switch (pCode[0])
	  {
          // Obsolete
	  case ITM_NULLIFZERO:
	    {
	      PTR_DEF_ASSIGN(char, tgt, 1 );
	      PTR_DEF_ASSIGN(char, tgtNull, 3 );
	      PTR_DEF_ASSIGN(char, src, 6 );
	      DEF_ASSIGN(Int32, tgtNullIndicatorLen, 5 );
	      DEF_ASSIGN(Int32, srcLen, 8 );
	      NABoolean resultIsNull = TRUE;
	      for (Int32 i = 0; i < srcLen; i++)
		{
		  tgt[i] = src[i];
		  if (src[i] != 0)
		    {
		      resultIsNull = FALSE;
		    }
		}

	      if (resultIsNull)
                str_pad(tgtNull, tgtNullIndicatorLen, '\377');
              else
                str_pad(tgtNull, tgtNullIndicatorLen, 0);

	      pCode += 9;
	    }
	  break;

	  case ITM_RANDOMNUM:
	    {
	      DEF_ASSIGN(Int32, seed, 8 );
	      PTR_DEF_ASSIGN(UInt32, resultPtr, 1 );
	      seed++;
	      if (seed < 0)  // numeric overflow ?
		seed = seed + INT_MAX;
	      if (seed < 1) seed = 1; // in case seed == -1 or zero
	      * resultPtr = seed; 
	      pCode[8] = seed; // assign temp value back
	      pCode += 9;
	    }
	  break;

	  default:
	    goto Error2_;
	  }
      }
    break;

    case PCIT::GENFUNC_MBIN8_MBIN8_MBIN8_IBIN32S_IBIN32S:
      {
	switch (pCode[0])
	  {
	  case ITM_CONCAT:
	    {
	      PTR_DEF_ASSIGN(char, tgt, 1 );
	      PTR_DEF_ASSIGN(char, op1, 3 );
	      PTR_DEF_ASSIGN(char, op2, 5 );
	      DEF_ASSIGN(Int32, length1, 7 );
	      DEF_ASSIGN(Int32, length2, 8 );

	      str_cpy_all(tgt, op1, length1);
	      str_cpy_all(&tgt[length1], op2, length2);
	    }
	  pCode += 9;
	  break;

	  // bitand, bitor, bitxor done for int32 or Int64 operands
	  // and result. This has been validated during pcode gen for
	  // this operation.
	  case ITM_BITAND:
	    {
	      DEF_ASSIGN(Int32, oper, 7 );
	      if ( oper == REC_BIN32_UNSIGNED )
	      {
		PTR_DEF_ASSIGN(UInt32, result, 1 );
		PTR_DEF_ASSIGN(UInt32, op1, 3 );
		PTR_DEF_ASSIGN(UInt32, op2, 5 );

		* result = *op1 & *op2 ;
	      }
	      else if ( oper == REC_BIN32_SIGNED )
	      {
		PTR_DEF_ASSIGN(Int32, result, 1 );
		PTR_DEF_ASSIGN(Int32, op1, 3 );
		PTR_DEF_ASSIGN(Int32, op2, 5 );

		* result = *op1 & *op2 ;
	      }
	      else 
	      {
		PTR_DEF_ASSIGN(Int64, result, 1 );
		PTR_DEF_ASSIGN(Int64, op1, 3 );
		PTR_DEF_ASSIGN(Int64, op2, 5 );

		* result = *op1 & *op2 ;
	      }
		
	      pCode += 9;
	    }
	  break;

	  case ITM_BITOR:
	    {
	      DEF_ASSIGN(Int32, oper, 7 );
	      if ( oper == REC_BIN32_UNSIGNED )
	      {
		PTR_DEF_ASSIGN(UInt32, result, 1 );
		PTR_DEF_ASSIGN(UInt32, op1, 3 );
		PTR_DEF_ASSIGN(UInt32, op2, 5 );

		* result = *op1 | *op2 ;
	      }
	      else if ( oper == REC_BIN32_SIGNED )
	      {
		PTR_DEF_ASSIGN(Int32, result, 1 );
		PTR_DEF_ASSIGN(Int32, op1, 3 );
		PTR_DEF_ASSIGN(Int32, op2, 5 );

		* result = *op1 | *op2 ;
	      }
	      else 
	      {
		PTR_DEF_ASSIGN(Int64, result, 1 );
		PTR_DEF_ASSIGN(Int64, op1, 3 );
		PTR_DEF_ASSIGN(Int64, op2, 5 );

		* result = *op1 | *op2 ;
	      }
		
	      pCode += 9;
	    }
	  break;

	  case ITM_BITXOR:
	    {
	      DEF_ASSIGN(Int32, oper, 7 );
	      if ( oper == REC_BIN32_UNSIGNED )
	      {
		PTR_DEF_ASSIGN(UInt32, result, 1 );
		PTR_DEF_ASSIGN(UInt32, op1, 3 );
		PTR_DEF_ASSIGN(UInt32, op2, 5 );

		* result = *op1 ^ *op2 ;
	      }
	      else if ( oper == REC_BIN32_SIGNED )
	      {
		PTR_DEF_ASSIGN(Int32, result, 1 );
		PTR_DEF_ASSIGN(Int32, op1, 3 );
		PTR_DEF_ASSIGN(Int32, op2, 5 );

		* result = *op1 ^ *op2 ;
	      }
	      else 
	      {
		PTR_DEF_ASSIGN(Int64, result, 1 );
		PTR_DEF_ASSIGN(Int64, op1, 3 );
		PTR_DEF_ASSIGN(Int64, op2, 5 );

		* result = *op1 ^ *op2 ;
	      }
		
	      pCode += 9;
	    }
	  break;

	  case ITM_BITNOT:
	    {
	      DEF_ASSIGN(Int32, oper, 7 );
	      if ( oper == REC_BIN32_UNSIGNED )
	      {
		PTR_DEF_ASSIGN(UInt32, result, 1 );
		PTR_DEF_ASSIGN(UInt32, op1, 3 );

		* result = ~ *op1 ;
	      }
	      else if ( oper == REC_BIN32_SIGNED )
	      {
		PTR_DEF_ASSIGN(Int32, result, 1 );
		PTR_DEF_ASSIGN(Int32, op1, 3 );

		* result =  ~ *op1 ;
	      }
	      else 
	      {
		PTR_DEF_ASSIGN(Int64, result, 1 );
		PTR_DEF_ASSIGN(Int64, op1, 3 );

		* result = ~ *op1 ;
	      }
		
	      pCode += 9;
	    }
	  break;

	  default:
	    goto Error2_;
	  }
      }
    break;

    case PCIT::GENFUNC_MATTR5_MATTR5_IBIN32S:         // upper, lower, trim
    case PCIT::GENFUNC_MATTR5_MATTR5_MBIN32S_IBIN32S: // repeat
      {
        Int32 srcLen = 0, tgtLen = 0, i = 0;
	BASE_PTR_DEF_ASSIGN(char, tgt, 0 );
	BASE_PTR_DEF_ASSIGN(char, src, 5 );

	DEF_ASSIGN(Int32, tgtOffset, 1 );
	DEF_ASSIGN(Int32, tgtVoaOffset, 2 );

        // VOA offsets should always be used in order.
        if(tgtVoaOffset > 0) {
          assert(tgtVoaOffset > lastVoaOffset);
          lastVoaOffset = tgtVoaOffset;
        }

	DEF_ASSIGN(Int32, comboLen1, 4 );
	DEF_ASSIGN(Int32, comboLen2, 9 );
        char* comboPtr1 = (char*)&comboLen1;
        char* comboPtr2 = (char*)&comboLen2;
      
        Int16 tgtNullIndLen = (Int16)comboPtr1[0];
        Int16 tgtVCIndLen   = (Int16)comboPtr1[1];
        Int16 srcNullIndLen = (Int16)comboPtr2[0];
        Int16 srcVCIndLen   = (Int16)comboPtr2[1];
                  
	DEF_ASSIGN(Int32, offset, 6 );
	DEF_ASSIGN(Int32, voaOffset, 7 );

        src += ExpTupleDesc::getVarOffset(src,           // atp_
                                          offset, 
                                          voaOffset, 
	                                  srcVCIndLen,   // vcIndLen
                                          srcNullIndLen  // nullIndLen
                                         ); 
	if (srcVCIndLen > 0)
        {
          srcLen = ExpTupleDesc::getVarLength(src, srcVCIndLen);
        }
	else
	{
	  srcLen = pCode[8];
        }      

        // advance to beginning of data        
        src += srcVCIndLen;

	DEF_ASSIGN(Int32, opRepeat, 12 );
	DEF_ASSIGN(Int32, opx, 10 );

        Int32 subOpc = (pCodeOpc == PCIT::GENFUNC_MATTR5_MATTR5_MBIN32S_IBIN32S)
	  ? opRepeat : opx ;
        
        switch (subOpc) {

	  case ITM_UPPER:
          case ITM_LOWER:
	  {

            tgt += ExpTupleDesc::getTgtVarOffset( tgt,
                                                  tgtOffset,
                                                  tgtVoaOffset,
                                                  tgtVCIndLen,
                                                  tgtNullIndLen,
                                                  varOffset,
                                                  srcLen
                                               );
            if (tgtVCIndLen > 0) 
            {
              ExpTupleDesc::setVarLength(tgt, srcLen, tgtVCIndLen); 
              tgt += tgtVCIndLen;
            }
              
            for (Int32 i = 0; i < srcLen; i++)
	      tgt[i] = (subOpc==ITM_UPPER) ? TOUPPER(src[i]) : TOLOWER(src[i]);
              
	    pCode += 11;
	    break;
	  }

	  case ITM_RTRIM:
	  case ITM_LTRIM:
	  case ITM_TRIM:
	    {     
	      // Find how many leading characters in operand 2 correspond 
	      // to the trim character.
	      Lng32 len0 = srcLen;
	      Lng32 start = 0;   
	      
	      if (subOpc == ITM_LTRIM || subOpc == ITM_TRIM)
		{
		  while ((start < srcLen) && 
			 (src[start] == ' ')
			 )
		    {
		      start++;
		      len0--;
		    }
		}

	      // Find how many trailing characters in operand 2 correspond 
	      // to the trim character.
	      Lng32 end = srcLen;
	      if (subOpc == ITM_RTRIM || subOpc == ITM_TRIM)
		{
		  while ((end > (start)) && 
			 (src[end-1] == ' ')
			 )
		    {
		      end--;
		      len0--;
		    }
		}

              tgt += ExpTupleDesc::getTgtVarOffset( tgt,
                                                    tgtOffset,     // tgt offset
                                                    tgtVoaOffset,  // tgt voa offset
                                                    tgtVCIndLen,
                                                    tgtNullIndLen,
                                                    varOffset,
                                                    len0
                                                  );
              if (tgtVCIndLen > 0) 
              {
                ExpTupleDesc::setVarLength(tgt, len0, tgtVCIndLen); 
                tgt += tgtVCIndLen;
              }
              
	      if (len0 > 0)
	        str_cpy_all(tgt, &src[start], len0);

              pCode += 11;
	    }
	  break;

          case ITM_REPEAT:
            {
	      PTR_DEF_ASSIGN(Int32, repeatCntPtr, 10 );
	      Int32 repeatCnt = * repeatCntPtr;
              UInt32 totalSize = srcLen * repeatCnt;
	      DEF_ASSIGN(Int32, maxTargetLen, 3 );

              // If len is negative or the total bytes to write exceeds the max
              // target len, report an error.
              if ((repeatCnt < 0) || (repeatCnt * srcLen > maxTargetLen))
                goto Error1_;

              tgt += ExpTupleDesc::getTgtVarOffset( tgt,
                                                    tgtOffset,
                                                    tgtVoaOffset,
                                                    tgtVCIndLen,
                                                    tgtNullIndLen,
                                                    varOffset,
                                                    totalSize
                                                  );
              if (tgtVCIndLen > 0) 
              {
                ExpTupleDesc::setVarLength(tgt, totalSize, tgtVCIndLen); 
                tgt += tgtVCIndLen;
              }
              
              for (i=0; i < repeatCnt; i++) {
                str_cpy_all(tgt, src, srcLen);
                tgt += srcLen;
              }
              
              pCode += 13;

              break;
            }
        }

        break;
    }

    case PCIT::HASH2_DISTRIB:
      {
	PTR_DEF_ASSIGN(UInt32, resultPtr, 0 );
	PTR_DEF_ASSIGN(UInt32, hashValPtr, 2 );
	PTR_DEF_ASSIGN(UInt32, numPartsPtr, 4 );

	*resultPtr = (UInt32)((Int64)*hashValPtr * (Int64)*numPartsPtr >> 32);

        pCode += 6;
      }
      break;


    case PCIT::OFFSET_IPTR_IPTR_MBIN32S_MBIN64S_MBIN64S:
      {
	PTR_DEF_ASSIGN(Int32, indexPtr, 2 * PCODEBINARIES_PER_PTR);
	Int32 index = * indexPtr;
  
        // Lookup the indexed row in the history buffer. Compute pointers
        // to the attribute data, null indicator, and varchar indicator.
        //
        Int64 *srcData = NULL;

        {
          char *(*getRow)(void*,Int32,NABoolean,Lng32,Int32&);
          Int32 rc;

          getRow = (char *(*)(void*,Int32,NABoolean,Lng32,Int32&) )
                     GetPCodeBinaryAsPtr(pCode, 0);

          char *row = (*(getRow))((void *) GetPCodeBinaryAsPtr(pCode, PCODEBINARIES_PER_PTR),
                        index, TRUE, 0, rc);
          if(rc)
            {
              diagsArea = atp1->getDiagsArea();
              ExRaiseSqlError(heap_, &diagsArea, EXE_HISTORY_BUFFER_TOO_SMALL);
              if(diagsArea != atp1->getDiagsArea())
                atp1->setDiagsArea(diagsArea);
              return ex_expr::EXPR_ERROR;
            }

          if(row) 
            {
              srcData = (Int64 *)(row + pCode[5 + 2 * PCODEBINARIES_PER_PTR]);
            }
        }

	PTR_DEF_ASSIGN(Int64, dstData, 2 + 2 * PCODEBINARIES_PER_PTR);

        *dstData = (srcData ? *srcData : 0);

        pCode += 6 + 2 * PCODEBINARIES_PER_PTR;
      }
      break;

    case PCIT::OFFSET_IPTR_IPTR_IBIN32S_MBIN64S_MBIN64S:
      {
	DEF_ASSIGN(Int32, index, 2 * PCODEBINARIES_PER_PTR);
  
        // Lookup the indexed row in the history buffer. Compute pointers
        // to the attribute data, null indicator, and varchar indicator.
        //
        Int64 *srcData = NULL;

        {
          char *(*getRow)(void*,Int32,NABoolean,Lng32,Int32&) ;
          Int32 rc;
          getRow = (char *(*)(void*,Int32,NABoolean,Lng32,Int32&) )
                    GetPCodeBinaryAsPtr(pCode, 0);
          char *row = (*(getRow))(
                        (void *) GetPCodeBinaryAsPtr(pCode, PCODEBINARIES_PER_PTR),
                        index, TRUE, 0, rc);
          if(rc)
            {
              diagsArea = atp1->getDiagsArea();
              ExRaiseSqlError(heap_, &diagsArea, EXE_HISTORY_BUFFER_TOO_SMALL);
              if(diagsArea != atp1->getDiagsArea())
                atp1->setDiagsArea(diagsArea);
              return ex_expr::EXPR_ERROR;
            }

          if(row) 
            {
              srcData = (Int64 *)(row + pCode[4 + 2 * PCODEBINARIES_PER_PTR]);
            }
        }

	PTR_DEF_ASSIGN(Int64, dstData, 1 + 2 * PCODEBINARIES_PER_PTR);

        *dstData = (srcData ? *srcData : 0);

        pCode += 5 + 2 * PCODEBINARIES_PER_PTR;
      }
      break;

    case PCIT::COMP_MBIN32S_MBIGS_MBIGS_IBIN32S_IBIN32S:
    {
      PTR_DEF_ASSIGN(Int32, result, 0 );
      PTR_DEF_ASSIGN(UInt16,  src1, 2 );
      PTR_DEF_ASSIGN(UInt16,  src2, 4 );
      DEF_ASSIGN(Int32, len, 6 );
      Int32 len16 = len >> 1;

      // Pull out what type of comparison (e.g. ITM_EQUAL) to perform.
      DEF_ASSIGN(Int32, op, 7 );

      Int32 compCode = 1; // Assume comparison is EQ

      // Extract signs.
      char src1Sign = BIGN_GET_SIGN(src1, len);
      char src2Sign = BIGN_GET_SIGN(src2, len);

      // Clear sign bits
      BIGN_CLR_SIGN(src1, len);
      BIGN_CLR_SIGN(src2, len);

      // If the signs aren't equal, then we only need to check that both src
      // values are not 0 - otherwise we can clearly say what the result of
      // the comparison should be

      if (src1Sign != src2Sign) {
        for (Int32 i=0; i < len16; i++) {
          if ((src1[i] != 0) || (src2[i] != 0)) {
            compCode = (src1Sign == 0) ? 2 : 0;
            break;
          }
        }
      }
      else {
        // Both signs are different, so find out where the difference is
        for (Int32 i=len16-1; i >= 0; i--) {
          if (src1[i] == src2[i])
            continue;

          // Here lies the difference.  Return 0, 1, or 2 for compCode, 
          // depending on how src1[i] compares to src2[i]

          compCode = (src1[i] > src2[i]) ? ((src1Sign) ? 0 : 2)
                                         : ((src1Sign) ? 2 : 0);
          break;
        }
      }

      // Reset sign bits
      if (src1Sign)
        BIGN_SET_SIGN(src1, len);

      if (src2Sign)
        BIGN_SET_SIGN(src2, len);

      // Assign result
      *result = compTable[op - ITM_EQUAL][compCode];

      pCode += 8;
      break;
    }

    case PCIT::MOVE_MBIGS_MBIGS_IBIN32S_IBIN32S:
    {
      Int32 i;

      DEF_ASSIGN(Int32, tgtLength, 4);
      DEF_ASSIGN(Int32, srcLength, 5);

      PTR_DEF_ASSIGN(char, tgt, 0 );
      PTR_DEF_ASSIGN(char, src, 2 );

      // Get length in terms of double-words
      Int32 tgtLength16 = tgtLength >> 1;
      Int32 srcLength16 = srcLength >> 1;

      // Get source sign bit and then temporarily clear it
      char srcSign = BIGN_GET_SIGN(src, srcLength);
      BIGN_CLR_SIGN(src, srcLength);

      // If src value is too big for tgt, report overflow
      for (; srcLength16 > tgtLength16; srcLength16--) {
        if (((Int16*)src)[srcLength16 - 1] != 0) {
          // Reset source sign bit before reporting error
          if (srcSign)
            BIGN_SET_SIGN(src, srcLength);
          goto Error1_;
        }
      }

      // Only clear out tgt bits if move won't do it for you.
      if (tgtLength > srcLength)
        for (i=0; i < tgtLength16; i++)
          ((Int16*)tgt)[i] = 0;

      // Copy over shorts from bignum src
      for (i=0; i < srcLength16; i++)
        ((UInt16*)tgt)[i] = ((UInt16*)src)[i];

      if (srcSign) {
        BIGN_SET_SIGN(src, srcLength);
        BIGN_SET_SIGN(tgt, tgtLength);
      }

      pCode += 6;
      break;
    }

    case PCIT::MOVE_MBIGS_MBIN16S_IBIN32S:
    case PCIT::MOVE_MBIGS_MBIN32S_IBIN32S:
    case PCIT::MOVE_MBIGS_MBIN64S_IBIN32S:
    {
      Int32 i;

      DEF_ASSIGN(Int32, tgtLength, 4 );
      Int32 tgtLength16 = tgtLength >> 1;
      PTR_DEF_ASSIGN(UInt16,  tgt, 0 );
      PTR_DEF_ASSIGN(Int64, src, 2 );
      Int64* tgt64 = (Int64*)tgt;


      Int64 srcVal;

      if (pCodeOpc == PCIT::MOVE_MBIGS_MBIN64S_IBIN32S)
        srcVal = *src;
      else if (pCodeOpc == PCIT::MOVE_MBIGS_MBIN32S_IBIN32S)
        srcVal = *((Int32*)(src));
      else
        srcVal = *((Int16*)(src));

      NABoolean isNeg = FALSE;

      for (i=0; i < tgtLength16; i++)
        tgt[i] = 0;

      if (srcVal < 0) {
        srcVal = -srcVal;
        isNeg = TRUE;
      }

      *tgt64 = srcVal;

#ifndef NA_LITTLE_ENDIAN
      UInt16 temp1 = tgt[0];
      UInt16 temp2 = tgt[1];
      tgt[0] = tgt[3];
      tgt[1] = tgt[2];
      tgt[2] = temp2;
      tgt[3] = temp1;
#endif

      if (isNeg)
        BIGN_SET_SIGN(tgt, tgtLength);

      pCode += 5;
      break;
    }

    case PCIT::MOVE_MBIN64S_MBIGS_IBIN32S:
    {
      Int32 i;

      DEF_ASSIGN(Int32, srcLength, 4);
      Int32 srcLength16 = srcLength >> 1;

      PTR_DEF_ASSIGN(Int64, tgt, 0 );
      PTR_DEF_ASSIGN(char, src, 2 );


      // Get source sign bit and then temporarily clear it
      char srcSign = BIGN_GET_SIGN(src, srcLength);
      BIGN_CLR_SIGN(src, srcLength);

      for (i=4; i < srcLength16; i++) {
        if (((Int16*)src)[i] != 0) {
          // Reset source sign bit before reporting error
          if (srcSign)
            BIGN_SET_SIGN(src, srcLength);
          goto Error1_;
        }
      }

      *tgt = *((Int64*)src);

#ifndef NA_LITTLE_ENDIAN
      UInt16 temp1 = ((UInt16*)tgt)[0];
      UInt16 temp2 = ((UInt16*)tgt)[1];
      ((UInt16*)tgt)[0] = ((UInt16*)tgt)[3];
      ((UInt16*)tgt)[1] = ((UInt16*)tgt)[2];
      ((UInt16*)tgt)[2] = temp2;
      ((UInt16*)tgt)[3] = temp1;
#endif

      // Reset source sign bit
      if (srcSign)
        BIGN_SET_SIGN(src, srcLength);

      // If bignum was positive but that resulted in the sign bit being set
      // in the 64-bit target, then we overflowed.
      if ((srcSign == 0) && (*tgt < 0))
        goto Error1_;
      else if ((srcSign) && (*tgt < 0) && (*tgt != LLONG_MIN))
        goto Error1_;

      if ((srcSign) && (*tgt != LLONG_MIN))
        *tgt = -(*tgt);

      pCode += 5;
      break;
    }

    case PCIT::MUL_MBIGS_MBIGS_MBIGS_IBIN32S:
    {
      Int32 i, j, pos;
      UInt16 carry;

      PTR_DEF_ASSIGN(UInt16,  tgt, 0 );
      PTR_DEF_ASSIGN(UInt16,  src1, 2 );
      PTR_DEF_ASSIGN(UInt16,  src2, 4 );

      DEF_ASSIGN(Int32, tgtLength, 6 );
      DEF_ASSIGN(Int32, leftLength, 7 );
      DEF_ASSIGN(Int32, rightLength, 8 );

      Int32 leftLength16 = leftLength >> 1;
      Int32 rightLength16 = rightLength >> 1;
      Int32 tgtLength16 = tgtLength >> 1;

      // Get source sign bit and then temporarily clear it
      char src1Sign = BIGN_GET_SIGN(src1, leftLength);
      char src2Sign = BIGN_GET_SIGN(src2, rightLength);

      BIGN_CLR_SIGN(src1, leftLength);
      BIGN_CLR_SIGN(src2, rightLength);

      for (i=0; i < tgtLength16; i++)
        ((Int16*)tgt)[i] = 0;

      while ((leftLength16 > 0) && (src1[leftLength16 - 1] == 0))
        leftLength16--;

      while ((rightLength16 > 0) && (src2[rightLength16 - 1] == 0))
        rightLength16--;

      if ((leftLength16 != 0) && (rightLength16 != 0)) {
        // Multiply shorts - this is because bignum stored this way.
        for (j=0; j < rightLength16; j++) {

          for (i=0, carry=0, pos=j; i < leftLength16; i++, pos++) {
            UInt32 result = (((UInt32)src1[i]) * src2[j]) + tgt[pos] + carry;
            tgt[pos] = (UInt16)(result & 0xffff);
            carry = (UInt16)(result >> 16);
          }

          if (carry)
            tgt[pos] = carry;
        }
      }

      if (src1Sign)
	BIGN_SET_SIGN(src1, leftLength);

      if (src2Sign)
	BIGN_SET_SIGN(src2, rightLength);

      if (src1Sign != src2Sign)
        BIGN_SET_SIGN(tgt, tgtLength);

      pCode += 9;

      break;
    }

    case PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIGS_MBIGS_IBIN32S:
    case PCIT::ADD_MBIGS_MBIGS_MBIGS_IBIN32S:
    case PCIT::SUB_MBIGS_MBIGS_MBIGS_IBIN32S:
    {
      UInt16 *tgt, *src1, *src2;
      NABoolean performAdd = TRUE;
      UInt16 carry = 0;
      Int32 length;
      Int32 i;

      if (pCodeOpc == PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIGS_MBIGS_IBIN32S)
      {
	PTR_DEF_ASSIGN(UInt16,  tgtPtr, 7 );
        src1 = tgt = tgtPtr ;
	PTR_DEF_ASSIGN(UInt16,  src2Ptr, 9 );
	src2 = src2Ptr;
	DEF_ASSIGN(Int32,lenVal, 11);
	length = lenVal;
	DEF_ASSIGN(Int32, isNullable, 6 );

	if ( isNullable )
        {
          NABoolean isNull;

	  PTR_DEF_ASSIGN(Int16, tgtNull, 0 );
	  PTR_DEF_ASSIGN(Int16, srcNull, 3 );
	  DEF_ASSIGN(Int16, indxTgt, 2 );
	  DEF_ASSIGN(Int16, indxSrc, 5 );

          isNull =  indxSrc == -1  ?  *srcNull != 0  :
	    ExpAlignedFormat::isNullValue((char*)srcNull, indxSrc) ;

          if (isNull) {
            pCode += 12;
            break;
          }

          isNull =  indxTgt == -1  ?  *tgtNull != 0  :
	    ExpAlignedFormat::isNullValue((char*)tgtNull, indxTgt );

          if (isNull) {
	    if ( indxTgt == -1 )
              *tgtNull = 0;
            else
	      ExpAlignedFormat::clearNullBit((char*)tgtNull, indxTgt);

            str_pad((char*)tgt, length, 0);
          }
        }

        pCode += 5; // Re-position to align with ADD/SUB instruction.
      }
      else
      {
	PTR_DEF_ASSIGN(UInt16, tgtPtr, 0 );
	PTR_DEF_ASSIGN(UInt16, src1Ptr, 2 );
	PTR_DEF_ASSIGN(UInt16, src2Ptr, 4 );
	tgt = tgtPtr; src1 = src1Ptr; src2 = src2Ptr;
	DEF_ASSIGN(Int32,lenVal, 6);
	length = lenVal;
      }

      Int32 length16 = length >> 1;

      // Get source sign bits
      char src1Sign = BIGN_GET_SIGN(src1, length);
      char src2Sign = BIGN_GET_SIGN(src2, length);

      char* origSrc1 = (char*)src1;
      char* origSrc2 = (char*)src2;

      char origSrc1Sign = src1Sign;
      char origSrc2Sign = src2Sign;

      // Temporarily clear sign bits
      BIGN_CLR_SIGN(src1, length);
      BIGN_CLR_SIGN(src2, length);

      if (pCodeOpc == PCIT::SUB_MBIGS_MBIGS_MBIGS_IBIN32S) {
        if (src1Sign == src2Sign) {
          performAdd = FALSE;

          // We want to distribute the SUB operation into the signs of the
          // source operand so that everything is normalized as if we were
          // just doing an ADD and the source operands carried the sign.
          src2Sign = (src2Sign == 0) ? MSB_SET_MSK : 0;
        }
      }
      else {
        if (src1Sign != src2Sign)
          performAdd = FALSE;
      }

      NABoolean notEqual = FALSE; // assuming src1 equals src2 for sub
      if (performAdd)
      {
        // Add shorts - this is because bignum stored this way.
        for (i=0; i < length16; i++) {
          UInt32 result = (UInt32)src1[i] + src2[i] + carry;
          tgt[i] = (UInt16)(result & 0xffff);
          carry = (UInt16)(result >> 16);
        }
      }
      else
      {
        // Determine which source is bigger
        for (i = length16-1; i >= 0; i--) {
          // Skip equality
          if (src1[i] == src2[i])
            continue;

          notEqual = TRUE;
          if (src1[i] < src2[i]) {
            // swap src1 and src2 along with their signs so the result is
            // positive and the src1Sign would be the result sign
            UInt16* temp = src1; src1 = src2; src2 = temp;
            char tempSign = src1Sign; src1Sign = src2Sign; src2Sign = tempSign;
          }

          break;
        }

        // Sub shorts - this is because bignum stored this way.
        for (i=0; i < length16; i++) {
          Int32 result = (Int32)src1[i] - src2[i] + (Int16)carry;
          tgt[i] = (UInt16) (result + (Int32)0x10000) & 0xffff;
          carry = (result < 0) ? -1 : 0;
        }
      }

      if (origSrc1Sign)
        BIGN_SET_SIGN(origSrc1, length);

      if (origSrc2Sign)
        BIGN_SET_SIGN(origSrc2, length);

      // Set target sign - will always be src1Sign unless target is 0
      if (src1Sign && (performAdd || notEqual)) {
        BIGN_SET_SIGN(tgt, length);
      } else {
        BIGN_CLR_SIGN(tgt, length);
      }

      // Report overflow
      if (carry != 0)
        goto Error1_;

      pCode += 7;
      break;
    }

    case PCIT::SWITCH_MBIN32S_MBIN64S_MPTR32_IBIN32S_IBIN32S:
    {
      PTR_DEF_ASSIGN(UInt32, result, 0 );
      PTR_DEF_ASSIGN(Int64, src, 2 );
      DEF_ASSIGN(UInt32, flags, 7 );
      NABoolean isInList = flags & 0x01;
      DEF_ASSIGN(Int32, size, 6 );
      Int32 i;

      //
      // Pointers to tables representing 64-bit values in IN-list and 32-bit
      // pc-relative offsets for then clauses of case statement.  Tables look
      // as follows:
      //
      // In-list:  val1, val2, ..., valN
      // Case:     val1, val2, ..., valN, pcOff1, pcOff2, ..., pcOffN, pcOffDflt
      //

      PTR_DEF_ASSIGN(Int64, vals, 4 );
      Long* pcOffs = (Long*)(&vals[size]);  // i.e. the jump table


      // Assume no match
      Int32 compCode = 0;

      UInt32  hashVal = ExHDPHash::hash8((char*)src, ExHDPHash::NO_FLAGS);
      UInt32  index   = hashVal % size;

      // Compare value at index, if element at index is valid.
      for (i=0; (i < 4) && (vals[index] != PCodeCfg::INVALID_INT64); i++) {
        // Compare values to ensure match, then return TRUE if match found.
        if (vals[index] == *src) {
          compCode = 1;
          break;
        }

        // Assuming the next element is not at the end of the table, keep
        // trying for at most 4 times in total.
        if (++index >= (UInt32)size)
          break;
      }

      // Get jump table location, using default case if item not found
      if (!isInList)
        compCode = ((compCode == 0) ? pcOffs[size] : pcOffs[index]);

      *result = compCode;

      pCode += 8;
      break;
    }

    case PCIT::SWITCH_MBIN32S_MATTR5_MPTR32_IBIN32S_IBIN32S:
    {
      PTR_DEF_ASSIGN(UInt32, result, 0 );
      DEF_ASSIGN(UInt32, flags, 10 );
      NABoolean isInList = flags & 0x01;
      NABoolean ignoreSpaces = flags & 0x02;
      DEF_ASSIGN(Int32, size, 9 );
      Int32 compCode     = -1;  // Assume no match
      Int32 i;

      //
      // Pointer to table representing constant strings in IN-list and 32-bit
      // pc-relative offsets for then clauses of case statement.  Tables look
      // as follows:
      //
      // In-list:  [len1, off1], [len2, off2], ..., [lenN, offN]
      // Case:     [len1, off1], [len2, off2], ..., [lenN, offN],
      //             pcOff1, pcOff2, ..., pcOffDflt
      //

      PTR_DEF_ASSIGN(Int32, table, 7 );
      Long *pcOffs = (isInList)? NULL: (Long*)&(table[size<<1]);
        


      DEF_ASSIGN(Int32, offset, 3 ); 
      DEF_ASSIGN(Int32, voaOffset, 4 ); 
      DEF_ASSIGN(Int32, comboLen1, 6 ); 
      char* comboPtr1    = (char*)&comboLen1;
      Int16 srcNullIndLen = (Int16)comboPtr1[0];
      Int16 srcVCIndLen   = (Int16)comboPtr1[1];

      DEF_ASSIGN(UInt32, len1, 5 );
      BASE_PTR_DEF_ASSIGN(char, src1, 2 );

      if (srcVCIndLen > 0) {
        src1 += ExpTupleDesc::getVarOffset(src1,
                                           offset,
                                           voaOffset,
                                           srcVCIndLen,   // vcIndLen
                                           srcNullIndLen  // nullIndLen
                                          );

        len1 = ExpTupleDesc::getVarLength(src1, srcVCIndLen);
        src1 += srcVCIndLen;
      }
      else {
        src1 += offset; 
      }

      // skip trailing blanks if asked to
      if (!ignoreSpaces)
        while ((len1 > 0) && (src1[len1-1] == ' '))
          len1--;

      UInt32 hashVal = ExHDPHash::hash(src1, ExHDPHash::NO_FLAGS, len1);
      UInt32 index   = (hashVal % size) << 1;

      // Compare value at index, if element at index is valid.
      for (i=0; (i < 4) && (table[index] != PCodeCfg::INVALID_INT32); i++) {
        UInt32 len2 = table[index];
        char* src2  = (char*)(stack[1] + table[index + 1]);
        compCode = ((len1 != len2) || memcmp(src1, src2, len1));

        // If match is false, try next entry in table (at most 4 checks).
        if ((compCode == 0) || ((Int32)(index + 2) >= (size << 1)))
          break;

        index += 2;
      }

      if (compCode == 0)  // match
        *result = (isInList) ? 1 : pcOffs[(index >> 1)];
      else  // taking default
        *result = (isInList) ? 0 : pcOffs[size];

      pCode += 11;
      break;
    }

  case PCIT::NULL_VIOLATION_MBIN16U_MBIN16U:
  {
    PTR_DEF_ASSIGN(Int16, tmp, 0 );
    if ( *tmp != 0 )
    {
      // Raise an "assigning a null value to a NOT NULL".
      diagsArea = atp1->getDiagsArea();
      ExRaiseSqlError(getHeap(), &diagsArea, 
        EXE_ASSIGNING_NULL_TO_NOT_NULL);
      if(diagsArea != atp1->getDiagsArea())
        atp1->setDiagsArea(diagsArea);

      return ex_expr::EXPR_ERROR;
    }
    pCode += 2;
    // Do NOT break here...
    // The first null has been processed above, so fall through to 
    // process the second null.
    // These two cases belong together...
  }
  case PCIT::NULL_VIOLATION_MBIN16U:
  {
    PTR_DEF_ASSIGN(Int16, tmp, 0 );
    if ( *tmp != 0 )
    {
      // Raise an "assigning a null value to a NOT NULL".
      diagsArea = atp1->getDiagsArea();
      ExRaiseSqlError(getHeap(), &diagsArea, 
        EXE_ASSIGNING_NULL_TO_NOT_NULL);
      if(diagsArea != atp1->getDiagsArea())
        atp1->setDiagsArea(diagsArea);
      return ex_expr::EXPR_ERROR;
    }
    pCode += 2;
    break;
  }
  case PCIT::NULL_TEST_MBIN32S_MATTR5_IBIN32S_IBIN32S:
    {
      // if the flag is set, operand is an indirect varchar and must 
      // use voaOffset to get to the nullIndicator. 
      // The flag will not be set for aligned format. 
      DEF_ASSIGN(Int16, idx, 6 );
      DEF_ASSIGN(Int32, flag1, 7 );
      DEF_ASSIGN(Int32, flag2, 8 );
      DEF_ASSIGN(Int32, val, 3 );
      PTR_DEF_ASSIGN(Int32, ptr, 0 );
      UInt32 pCode3 = flag1 == 1 ? *((Int32*)(stack[pCode[2]] + pCode[4]))
                                 :  val;
 
      NABoolean srcNull =
        ExpTupleDesc::isNullValue((char*)(stack[pCode[2]]+pCode3), idx, 
				  (ExpTupleDesc::TupleDataFormat)pCode[5]);
      
      if ( srcNull && flag2 != 0 ) *ptr = 1;
      else if ( !srcNull && flag2 == 0 ) *ptr = 1;
      else *ptr = 0 ;
      pCode += 9;
      break;
    }

    case PCIT::END:
      return retCode;

    case PCIT::NOP:
      break;

    default:
      assert(pCodeOpc == -1);
      goto Error2_;
    };
  };
  
 Error1_:
  return reportOverflowError(atp1, pCodeOpcPtr, pCode, stack);
					    
					    Error2_:
  diagsArea = atp1->getDiagsArea();
  ExRaiseSqlError(heap_, &diagsArea, EXE_INTERNAL_ERROR);
  if(diagsArea != atp1->getDiagsArea())
    atp1->setDiagsArea(diagsArea);
  return ex_expr::EXPR_ERROR;
}

/**
*** This routine is only called when a divide by zero or overflow occurred
*** during expression evaluation in evalPCode().  Any time a new pCode
*** instruction is inserted which could result in such an exception, this
*** routine needs to be modified.  The pCode pointer passed in is assumed to
*** be at the first operand position passed the opcode.
**/
ex_expr::exp_return_type ex_expr_base::reportOverflowError(
					      atp_struct *atp1, 
					      PCodeBinary* pCodeOpcPtr,
					      PCodeBinary* pCode,
					      Long* stack)
{
  ComDiagsArea *diagsArea = atp1->getDiagsArea();

  char* op1;
  char* op2;
  ExeErrorCode ovfl;
  Int32 instr;

  // Retrieve the pCode opcode associated with this failure.  Because certain
  // instructions fall into others during expression evaluation, we need to be
  // careful to assign instr as is expected when reporting the error.

  if (*pCodeOpcPtr == PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIN64S_MBIN64S)
    instr = PCIT::SUM_MBIN64S_MBIN64S;
  else if (*pCodeOpcPtr == PCIT::SUM_MATTR3_MATTR3_IBIN32S_MFLT64_MFLT64)
    instr = PCIT::SUM_MFLT64_MFLT64;
  else if (*pCodeOpcPtr == PCIT::SUM_MATTR3_MATTR3_IBIN32S_MBIGS_MBIGS_IBIN32S)
    instr = PCIT::ADD_MBIGS_MBIGS_MBIGS_IBIN32S;
  else
    instr = *(pCode-1);

  switch (instr) {
    case PCIT::ADD_MBIGS_MBIGS_MBIGS_IBIN32S:
    case PCIT::SUB_MBIGS_MBIGS_MBIGS_IBIN32S:
    case PCIT::ADD_MBIN64S_MBIN64S_MBIN64S:
    case PCIT::ADD_MBIN64S_MBIN32S_MBIN64S:
    case PCIT::SUB_MBIN64S_MBIN64S_MBIN64S:
    case PCIT::MUL_MBIN64S_MBIN64S_MBIN64S:
    case PCIT::ADD_MFLT64_MFLT64_MFLT64:
    case PCIT::SUB_MFLT64_MFLT64_MFLT64:
    case PCIT::MUL_MFLT64_MFLT64_MFLT64:
      op1 = ((char*)(stack[pCode[2]] + pCode[3]));
      op2 = ((char*)(stack[pCode[4]] + pCode[5]));
      ovfl = EXE_NUMERIC_OVERFLOW;

      break;

    case PCIT::SUM_MBIN64S_MBIN64S:
    case PCIT::SUM_MFLT64_MFLT64:
      op1 = ((char*)(stack[pCode[0]] + pCode[1]));
      op2 = ((char*)(stack[pCode[2]] + pCode[3]));
      ovfl = EXE_NUMERIC_OVERFLOW;

      break;

    case PCIT::DIV_MBIN64S_MBIN64S_MBIN64S:
    case PCIT::DIV_MBIN64S_MBIN64S_MBIN64S_ROUND:
      op1 = ((char*)(stack[pCode[2]] + pCode[3]));
      op2 = ((char*)(stack[pCode[4]] + pCode[5]));

      if (*((Int64*)(stack[pCode[4]] + pCode[5])) == 0) {
        ovfl = EXE_DIVISION_BY_ZERO;
      }
      else {
        ovfl = EXE_NUMERIC_OVERFLOW;
      }

      break;

    case PCIT::DIV_MFLT64_MFLT64_MFLT64:
      op1 = ((char*)(stack[pCode[2]] + pCode[3]));
      op2 = ((char*)(stack[pCode[4]] + pCode[5]));

      if (*((double *)(stack[pCode[4]] + pCode[5])) == 0) {
        ovfl = EXE_DIVISION_BY_ZERO;
      }
      else {
        ovfl = EXE_NUMERIC_OVERFLOW;
      }

      break;

    case PCIT::RANGE_MFLT64:
      op1 = ((char*)(stack[pCode[0]] + pCode[1]));
      op2 = NULL;
      ovfl = EXE_NUMERIC_OVERFLOW;
      break;

    case PCIT::RANGE_LOW_S64S64:
      op1 = ((char*)(stack[pCode[0]] + pCode[1]));
      op2 = (char*)(&pCode[2]);

      if (((*((Int64*)(stack[pCode[0]]+pCode[1]))) < 0) &&
          ((*(Int64*)&pCode[2]) >= 0))
      { // conversion of -ve num to unsigned field
        ovfl = EXE_UNSIGNED_OVERFLOW;
      }
      else {
        ovfl = EXE_NUMERIC_OVERFLOW;
      }

      break;

    case PCIT::RANGE_LOW_S32S64:
      op1 = ((char*)(stack[pCode[0]] + pCode[1]));
      op2 = (char*)(&pCode[2]);

      if (((*((Int32*)(stack[pCode[0]]+pCode[1]))) < 0) &&
          ((*(Int64*)&pCode[2]) >= 0))
      { // conversion of -ve num to unsigned field
        ovfl = EXE_UNSIGNED_OVERFLOW;
      }
      else {
        ovfl = EXE_NUMERIC_OVERFLOW;
      }

      break;

    case PCIT::RANGE_LOW_S16S64:
      op1 = ((char*)(stack[pCode[0]] + pCode[1]));
      op2 = (char*)(&pCode[2]);

      if (((*((short*)(stack[pCode[0]]+pCode[1]))) < 0) &&
          ((*(Int64*)&pCode[2]) >= 0))
      { // conversion of -ve num to unsigned field
        ovfl = EXE_UNSIGNED_OVERFLOW;
      }
      else {
        ovfl = EXE_NUMERIC_OVERFLOW;
      }

      break;

    case PCIT::RANGE_LOW_S8S64:
      op1 = ((char*)(stack[pCode[0]] + pCode[1]));
      op2 = (char*)(&pCode[2]);

      if (((*((Int8*)(stack[pCode[0]]+pCode[1]))) < 0) &&
          ((*(Int64*)&pCode[2]) >= 0))
      { // conversion of -ve num to unsigned field
        ovfl = EXE_UNSIGNED_OVERFLOW;
      }
      else {
        ovfl = EXE_NUMERIC_OVERFLOW;
      }

      break;

    case PCIT::RANGE_LOW_U32S64:
    case PCIT::RANGE_LOW_U16S64:
    case PCIT::RANGE_LOW_U8S64:
    case PCIT::RANGE_HIGH_S64S64:
    case PCIT::RANGE_HIGH_S32S64:
    case PCIT::RANGE_HIGH_U32S64:
    case PCIT::RANGE_HIGH_S16S64:
    case PCIT::RANGE_HIGH_U16S64:
    case PCIT::RANGE_HIGH_S8S64:
    case PCIT::RANGE_HIGH_U8S64:
      op1 = ((char*)(stack[pCode[0]] + pCode[1]));
      op2 = (char*)(&pCode[2]);
      ovfl = EXE_NUMERIC_OVERFLOW;

      break;

    case PCIT::MOVE_MBIN64S_MBIGS_IBIN32S:
      op2 = ((char*)(stack[pCode[2]] + pCode[3]));
      ExRaiseDetailSqlError(getHeap(), &diagsArea, EXE_NUMERIC_OVERFLOW, op2,
                            pCode[4], REC_NUM_BIG_SIGNED, 0,
                            REC_BIN64_SIGNED, 0);
      if(diagsArea != atp1->getDiagsArea())
        atp1->setDiagsArea(diagsArea);
      return ex_expr::EXPR_ERROR;
      break;

    case PCIT::MOVE_MBIGS_MBIGS_IBIN32S_IBIN32S:
      op2 = ((char*)(stack[pCode[2]] + pCode[3]));
      ExRaiseDetailSqlError(getHeap(), &diagsArea, EXE_NUMERIC_OVERFLOW, op2,
                            pCode[5], REC_NUM_BIG_SIGNED, 0,
                            REC_NUM_BIG_SIGNED,0);
      if(diagsArea != atp1->getDiagsArea())
        atp1->setDiagsArea(diagsArea);
      return ex_expr::EXPR_ERROR;
      break;

    case PCIT::GENFUNC_MATTR5_MATTR5_MBIN32S_IBIN32S:
      ExRaiseFunctionSqlError(getHeap(), &diagsArea, EXE_STRING_OVERFLOW,FALSE);
      if(diagsArea != atp1->getDiagsArea())
        atp1->setDiagsArea(diagsArea);
      return ex_expr::EXPR_ERROR;
      break;

    case PCIT::CONCAT_MATTR5_MATTR5_MATTR5:
      ExRaiseFunctionSqlError(getHeap(), &diagsArea, EXE_STRING_OVERFLOW,FALSE);
      return ex_expr::EXPR_ERROR;

    default:
      ExRaiseSqlError(heap_,
                      &diagsArea,
                      EXE_INTERNAL_ERROR);

      if(diagsArea != atp1->getDiagsArea())
        atp1->setDiagsArea(diagsArea);
      return ex_expr::EXPR_ERROR;
  }

  ExRaiseDetailSqlError(getHeap(),
                        &diagsArea,
                        ovfl,
                        (PCIT::Instruction)instr,
                        op1,
                        op2);

  if(diagsArea != atp1->getDiagsArea())
    atp1->setDiagsArea(diagsArea);
  return ex_expr::EXPR_ERROR;
}

ex_expr::exp_return_type ex_clause::eval(char * /*op_data*/[],
					 CollHeap*,
					 ComDiagsArea**)
{
  // default eval method does nothing and returns OK
  return ex_expr::EXPR_OK;
};

