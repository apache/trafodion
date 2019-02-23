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
#include "exp_clause_derived.h"
#include "exp_math_func.h"
#include "exp_function.h"
#include "ExpBitMuxFunction.h"
#include "ExpSequenceFunction.h"
#include "ExpLOB.h"
#include "wstr.h"

#define GenAssert(p, msg) if (!(p)) { NAAssert(msg, __FILE__ , __LINE__ ); };


///////////////////////////////////////////////////////////
// class ex_clause
///////////////////////////////////////////////////////////
void ex_clause::copyOperands(ex_clause* clause, Space* space)
{
  NABoolean showplan = (clause->getOperand() ?
                          clause->getOperand(0)->showplan() :
                          FALSE);

  Int32 numOperands = ((showplan) ? (2 * clause->getNumOperands()) :
                                   clause->getNumOperands());

  op_ = (AttributesPtr *)(space->allocateAlignedSpace(numOperands *
					sizeof(AttributesPtr)));

  for (Int32 i=0; i < numOperands; i++) {
    Attributes* attrOld = clause->getOperand(i);
    Attributes* attrNew = NULL;
    
    if (attrOld != NULL) {
      Int32 size = attrOld->getClassSize();
      attrNew = (Attributes*) new(space) char[size];
      memcpy((char*)attrNew, (char*)attrOld, size);
    }

    op_[i] = attrNew;
  }

  switch(clause->getType()) {
    case ex_clause::INOUT_TYPE:
    {
      char* temp;
      Lng32 len;
      ex_inout_clause* inout = (ex_inout_clause*)this;

      // Copy strings associated with this class.  The strings being with a
      // a length count of size long, followed by the string itself.  Also,
      // remember that new() will automatically allocated an aligned space, so
      // nothing additional needs to be done here.

      if (inout->getHeading()) {
        len = sizeof(Lng32) + *((Lng32*)inout->getHeading());
        temp = new(space) char[len];
        memcpy(temp, inout->getHeading(), len);
        inout->setHeading(temp);
      }

      if (inout->getName()) {
        len = sizeof(Lng32) + *((Lng32*)inout->getName());
        temp = new(space) char[len];
        memcpy(temp, inout->getName(), len);
        inout->setName(temp);
      }

      if (inout->getTableName()) {
        len = sizeof(Lng32) + *((Lng32*)inout->getTableName());
        temp = new(space) char[len];
        memcpy(temp, inout->getTableName(), len);
        inout->setTableName(temp);
      }

      if (inout->getSchemaName()) {
        len = sizeof(Lng32) + *((Lng32*)inout->getSchemaName());
        temp = new(space) char[len];
        memcpy(temp, inout->getSchemaName(), len);
        inout->setSchemaName(temp);
      }

      if (inout->getCatalogName()) {
        len = sizeof(Lng32) + *((Lng32*)inout->getCatalogName());
        temp = new(space) char[len];
        memcpy(temp, inout->getCatalogName(), len);
        inout->setCatalogName(temp);
      }

      break;
    }

    case ex_clause::FUNCTION_TYPE:
      if (clause->getClassID() == ex_clause::FUNC_RAISE_ERROR_ID) {
        ExpRaiseErrorFunction* func = (ExpRaiseErrorFunction*)this;
        const char* cName = func->getConstraintName();
        const char* tName = func->getTableName();

        // Set contraint name
        if (cName) {
          Int32 len = strlen(cName);
          char* temp = space->allocateAndCopyToAlignedSpace(cName, len, 0);
          func->setConstraintName(temp);
        }

        // Set table name
        if (tName) {
          Int32 len = strlen(tName);
          char* temp = space->allocateAndCopyToAlignedSpace(tName, len, 0);
          func->setTableName(temp);
        }
      }

      break;
  }
}


ex_clause::ex_clause(clause_type type, 
		     OperatorTypeEnum oper_type,
		     short num_operands, 
		     Attributes ** op,
		     Space * space) 
  : NAVersionedObject(type), nextClause_(NULL), op_(NULL)
{
  clauseType_  = type;
  operType_    = oper_type;
  numOperands_ = num_operands;
  pciLink_     = NULL;
  nextClause_  = (ExClausePtr)NULL;
  flags_       = 0;
  //  instruction_   = -1;
  instrArrayIndex_ = -1;

  str_pad(fillers_, sizeof(fillers_), '\0');

  // Further qualify these types...
  //
  if ((type == ex_clause::FUNCTION_TYPE) ||
      (type == ex_clause::LIKE_TYPE) ||
      (type == ex_clause::MATH_FUNCTION_TYPE) ||
      (type == ex_clause::AGGREGATE_TYPE))
    {
      switch (oper_type)
	{
	case ITM_LIKE: 
	  setClassID(LIKE_CLAUSE_CHAR_ID);
	  break;
	case ITM_LIKE_DOUBLEBYTE: 
	  setClassID(LIKE_CLAUSE_DOUBLEBYTE_ID);
	  break;
        case ITM_REGEXP:
          setClassID(REGEXP_CLAUSE_CHAR_ID);
          break;
	case ITM_ASCII: 
	case ITM_CODE_VALUE: 
	case ITM_UNICODE_CODE_VALUE: 
	case ITM_NCHAR_MP_CODE_VALUE: 
	  setClassID(FUNC_ASCII_ID);
	  break;
	case ITM_CHAR:
	case ITM_NCHAR_MP_CHAR:
	case ITM_UNICODE_CHAR:
	  setClassID(FUNC_CHAR_ID);
	  break;
	case ITM_CHAR_LENGTH:
	  setClassID(FUNC_CHAR_LEN_ID);
	  break;
	case ITM_CHAR_LENGTH_DOUBLEBYTE:
	  setClassID(FUNC_CHAR_LEN_DOUBLEBYTE_ID);
	  break;
	case ITM_CONVERTFROMHEX:
	case ITM_CONVERTTOHEX:
	  setClassID(FUNC_CVT_HEX_ID);
	  break;
	case ITM_OCTET_LENGTH:
	  setClassID(FUNC_OCT_LEN_ID);
	  break;
	case ITM_POSITION:
	  setClassID(FUNC_POSITION_ID);
	  break;
	case ITM_POSITION_DOUBLEBYTE:
	  setClassID(FUNC_POSITION_DOUBLEBYTE_ID);
	  break;
	case ITM_SPLIT_PART:
	  setClassID(FUNC_SPLIT_PART_ID);
	  break;
	case ITM_CONCAT:
	  setClassID(FUNC_CONCAT_ID);
	  break;
	case ITM_REPEAT:
	  setClassID(FUNC_REPEAT_ID);
	  break;
	case ITM_REPLACE:
	  setClassID(FUNC_REPLACE_ID);
	  break;
	case ITM_SUBSTR:
	  setClassID(FUNC_SUBSTR_ID);
	  break;
	case ITM_SUBSTR_DOUBLEBYTE:
	  setClassID(FUNC_SUBSTR_DOUBLEBYTE_ID);
	  break;
	case ITM_TRIM:
	  setClassID(FUNC_TRIM_ID);
	  break;
	case ITM_TRANSLATE:
	  setClassID(FUNC_TRANSLATE_ID);
	  break;
	case ITM_TRIM_DOUBLEBYTE:
	  setClassID(FUNC_TRIM_DOUBLEBYTE_ID);
	  break;
	case ITM_LOWER:
	  setClassID(FUNC_LOWER_ID);
	  break;
	case ITM_UPPER:
	  setClassID(FUNC_UPPER_ID);
	  break;
	case ITM_UPPER_UNICODE:
	  setClassID(FUNC_UPPER_UNICODE_ID);
	  break;
	case ITM_LOWER_UNICODE:
	  setClassID(FUNC_LOWER_UNICODE_ID);
	  break;
	case ITM_UNIX_TIMESTAMP:
	  setClassID(FUNC_UNIX_TIMESTAMP_ID);
	  break;
	case ITM_SLEEP:
	  setClassID(FUNC_SLEEP_ID);
	  break;
	case ITM_CURRENT_TIMESTAMP:
	  setClassID(FUNC_CURRENT_TIMESTAMP_ID);
	  break;
	case ITM_CURRENT_TIMESTAMP_RUNNING:
	  setClassID(FUNC_CURRENT_TIMESTAMP_ID);
	  break;
	case ITM_COMP_ENCODE:
	case ITM_COMP_DECODE:
	  setClassID(FUNC_ENCODE_ID);
	  break;
	case ITM_EXPLODE_VARCHAR:
	  setClassID(FUNC_EXPLODE_VARCHAR_ID);
	  break;
	case ITM_HASH:
	  setClassID(FUNC_HASH_ID);
	  break;
	case ITM_HIVE_HASH:
	  setClassID(FUNC_HIVEHASH_ID);
	  break;
	case ITM_HASHCOMB:
	  setClassID(FUNC_HASHCOMB_ID);
	  break;
	case ITM_HIVE_HASHCOMB:
	  setClassID(FUNC_HIVEHASHCOMB_ID);
	  break;
	case ITM_HDPHASH:
	  setClassID(FUNC_HDPHASH_ID);
	  break;
	case ITM_HDPHASHCOMB:
	  setClassID(FUNC_HDPHASHCOMB_ID);
	  break;
	case ITM_BITMUX:
	  setClassID(FUNC_BITMUX_ID);
	  break;
	case ITM_REPLACE_NULL:
	  setClassID(FUNC_REPLACE_NULL_ID);
	  break;
	case ITM_MOD:
	  setClassID(FUNC_MOD_ID);
	  break;
	case ITM_MASK_SET:
	case ITM_MASK_CLEAR:
	  setClassID(FUNC_MASK_ID);
	  break;
	case ITM_SHIFT_RIGHT:
	case ITM_SHIFT_LEFT:
	  setClassID(FUNC_SHIFT_ID);
	  break;
	case ITM_ABS:
	  setClassID(FUNC_ABS_ID);
	  break;
	case ITM_RETURN_TRUE:
	case ITM_RETURN_FALSE:
	case ITM_RETURN_NULL:
	  setClassID(FUNC_BOOL_ID);
	  break;
	case ITM_CONVERTTIMESTAMP:
	  setClassID(FUNC_CONVERTTIMESTAMP_ID);
	  break;
	case ITM_DATEFORMAT:
	  setClassID(FUNC_DATEFORMAT_ID);
	  break;
	case ITM_DAYOFWEEK:
	  setClassID(FUNC_DAYOFWEEK_ID);
	  break;
	case ITM_EXTRACT:
	case ITM_EXTRACT_ODBC:
	  setClassID(FUNC_EXTRACT_ID);
	  break;
	case ITM_JULIANTIMESTAMP:
	  setClassID(FUNC_JULIANTIMESTAMP_ID);
	  break;
	case ITM_EXEC_COUNT:
	  setClassID(FUNC_EXEC_COUNT_ID);
	  break;
	case ITM_CURR_TRANSID:
	  setClassID(FUNC_CURR_TRANSID_ID);
	  break;
        case ITM_SHA1:
          setClassID(FUNC_SHA1_ID);
          break;
        case ITM_SHA2_224:
        case ITM_SHA2_256:
        case ITM_SHA2_384:
        case ITM_SHA2_512:
          setClassID(FUNC_SHA2_ID);
          break;
        case ITM_MD5:
          setClassID(FUNC_MD5_ID);
          break; 
        case ITM_CRC32:
          setClassID(FUNC_CRC32_ID);
          break; 
	case ITM_ISIPV4:
	case ITM_ISIPV6:
	  setClassID(FUNC_ISIP_ID);
	  break;
        case ITM_INET_ATON:
          setClassID(FUNC_INETATON_ID);
          break;
        case ITM_INET_NTOA:
          setClassID(FUNC_INETNTOA_ID);
          break;
	case ITM_USER:
	case ITM_USERID:
	case ITM_AUTHTYPE:
	case ITM_AUTHNAME:
	  setClassID(FUNC_USER_ID);
	  break;
	case ITM_CURRENT_USER:
	case ITM_SESSION_USER:
	  setClassID(FUNC_ANSI_USER_ID);
	  break;
	case ITM_VARIANCE:
	  setClassID(FUNC_VARIANCE_ID);
	  break;
	case ITM_STDDEV:
	  setClassID(FUNC_STDDEV_ID);
	  break;
	case ITM_RAISE_ERROR:
	  setClassID(FUNC_RAISE_ERROR_ID);
	  break;
	case ITM_RANDOMNUM:
	  setClassID(FUNC_RANDOMNUM_ID);
	  break;
	case ITM_RAND_SELECTION:
	  setClassID(FUNC_RAND_SELECTION_ID);
	  break;
	case ITM_PROGDISTRIB:
	  setClassID(FUNC_PROGDISTRIB_ID);
	  break;
	case ITM_PROGDISTRIBKEY:
	  setClassID(FUNC_PROGDISTKEY_ID);
	  break;
	case ITM_PAGROUP:
	  setClassID(FUNC_PAGROUP_ID);
	  break;
	case ITM_HASH2_DISTRIB:
	  setClassID(FUNC_HASH2_DISTRIB_ID);
	  break;
	case ITM_UNPACKCOL:
	  setClassID(FUNC_UNPACKCOL_ID);
	  break;
	case ITM_PACK_FUNC:
	  setClassID(FUNC_PACK_ID);
	  break;
	case ITM_ROWSETARRAY_SCAN:
	  setClassID(FUNC_ROWSETARRAY_SCAN_ID);
	  break;
	case ITM_ROWSETARRAY_ROWID:
	  setClassID(FUNC_ROWSETARRAY_ROW_ID);
	  break;
	case ITM_ROWSETARRAY_INTO:
	  setClassID(FUNC_ROWSETARRAY_INTO_ID);
	  break;
	case ITM_RANGE_LOOKUP:
	  setClassID(FUNC_RANGE_LOOKUP_ID);
	  break;
	case ITM_OFFSET:
	  setClassID(FUNC_OFFSET_ID);
	  break;
	case ITM_DEGREES:
	case ITM_PI:
	case ITM_RADIANS:
	case ITM_ROUND:
	case ITM_SCALE_TRUNC:
	case ITM_ACOS:
	case ITM_ASIN:
	case ITM_ATAN:
	case ITM_ATAN2:
	case ITM_CEIL:
	case ITM_COS:
	case ITM_COSH:
	case ITM_EXP:
	case ITM_FLOOR:
	case ITM_LOG:
	case ITM_LOG10:
	case ITM_LOG2:
	case ITM_SIN:
	case ITM_SINH:
	case ITM_SQRT:
	case ITM_TAN:
	case ITM_TANH:
	case ITM_EXPONENT:
	case ITM_POWER:
	  setClassID(FUNC_MATH_ID);
	  break;
	case ITM_BITAND:
	case ITM_BITOR:
	case ITM_BITXOR:
	case ITM_BITNOT:
	case ITM_BITEXTRACT:
	case ITM_CONVERTTOBITS:
	  setClassID(FUNC_BIT_OPER_ID);
	  break;
	case ITM_ONE_ROW:
	  setClassID(AGGR_ONE_ROW_ID);
	  break;
	case ITM_ANY_TRUE_MAX:
	  setClassID(AGGR_ANY_TRUE_MAX_ID);
	  break;
	case ITM_AGGR_MIN_MAX:
	  setClassID(AGGR_MIN_MAX_ID);
	  break;
	case ITM_AGGR_GROUPING_FUNC:
	  setClassID(AGGR_GROUPING_ID);
	  break;
	case ITM_CURRENTEPOCH:
	case ITM_VSBBROWTYPE:
	case ITM_VSBBROWCOUNT:
	  setClassID(FUNC_GENERICUPDATEOUTPUT_ID);
	  break;
	case ITM_INTERNALTIMESTAMP:
	  setClassID(FUNC_INTERNALTIMESTAMP_ID);
	  break;
	case ITM_UNIQUE_EXECUTE_ID:
	  setClassID(FUNC_UNIQUE_EXECUTE_ID_ID);
	  break;
	case ITM_GET_TRIGGERS_STATUS:
	  setClassID(FUNC_GET_TRIGGERS_STATUS_ID);
	  break;
	case ITM_GET_BIT_VALUE_AT:
	  setClassID(FUNC_GET_BIT_VALUE_AT_ID);
	  break;
	case ITM_IS_BITWISE_AND_TRUE:
	  setClassID(FUNC_IS_BITWISE_AND_TRUE);
	  break;
	case ITM_NULLIFZERO:
	  setClassID(FUNC_NULLIFZERO);
	  break;
	case ITM_NVL:
	  setClassID(FUNC_NVL);
	  break;
        case ITM_JSONOBJECTFIELDTEXT:
	  setClassID(FUNC_JSON_ID);
	  break;
	case ITM_QUERYID_EXTRACT:
	  setClassID(FUNC_QUERYID_EXTRACT);
	  break;
	case ITM_UNIQUE_ID:
	case ITM_UNIQUE_ID_SYS_GUID:
	case ITM_UNIQUE_SHORT_ID:
	  setClassID(FUNC_UNIQUE_ID);
	  break;
	case ITM_ROWNUM:
	  setClassID(FUNC_ROWNUM);
	  break;
	case ITM_HBASE_COLUMN_LOOKUP:
	  setClassID(FUNC_HBASE_COLUMN_LOOKUP);
	  break;
	case ITM_HBASE_COLUMNS_DISPLAY:
	  setClassID(FUNC_HBASE_COLUMNS_DISPLAY);
	  break;
	case ITM_HBASE_COLUMN_CREATE:
	  setClassID(FUNC_HBASE_COLUMN_CREATE);
	  break;
	case ITM_TOKENSTR:
	  setClassID(FUNC_TOKENSTR_ID);
	  break;
	case ITM_REVERSE:
	  setClassID(FUNC_REVERSE_ID);
	  break;
	case ITM_CAST_TYPE:
	  setClassID(FUNC_CAST_TYPE);
	  break;
	case ITM_SEQUENCE_VALUE:
	  setClassID(FUNC_SEQUENCE_VALUE);
	  break;
	case ITM_PIVOT_GROUP:
	  setClassID(FUNC_PIVOT_GROUP);
	  break;
	case ITM_HEADER:
	  setClassID(FUNC_HEADER);
	  break;
	case ITM_HBASE_TIMESTAMP:
	  setClassID(FUNC_HBASE_TIMESTAMP);
	  break;
	case ITM_HBASE_VERSION:
	  setClassID(FUNC_HBASE_VERSION);
	  break;
	case ITM_SOUNDEX:
	  setClassID(FUNC_SOUNDEX_ID);
	  break;
        case ITM_AES_ENCRYPT:
          setClassID(FUNC_AES_ENCRYPT);
          break;
        case ITM_AES_DECRYPT:
          setClassID(FUNC_AES_DECRYPT);
          break;
        case ITM_ENCODE_BASE64:
        case ITM_DECODE_BASE64:
          setClassID(FUNC_BASE64_ENC_DEC);
          break;
 	default:
	  GenAssert(0, "ex_clause: Unknown Class ID.");
	  break;
	}
    }
  else if (type == ex_clause::LOB_TYPE)
    {
      switch (oper_type)
	{
	case ITM_LOBINSERT:
	  setClassID(LOB_INSERT);
	  break;
	case ITM_LOBSELECT:
	  setClassID(LOB_SELECT);
	  break;
	case ITM_LOBDELETE:
	  setClassID(LOB_DELETE);
	  break;
	case ITM_LOBUPDATE:
	  setClassID(LOB_UPDATE);
	  break;
	case ITM_LOBCONVERT:
	  setClassID(LOB_CONVERT);
	  break;
	case ITM_LOBCONVERTHANDLE:
	  setClassID(LOB_CONVERTHANDLE);
	  break;
	case ITM_SUBSTR:
	  setClassID(LOB_FUNC_SUBSTR);
	  break;
	default:
	  GenAssert(0, "ex_clause: Unknown Class ID.");
	  break;
	}
    }

  clauseNum_ = 0;
  numberBranchTargets_ = 0;

  /* Make sure that all operands have valid values for atp, atp_index
     and offset.                                                      */
  if (op) {

    short numOperands = (op[0]->showplan() ? num_operands*2 : num_operands);

    if (space)
      
      op_ = (AttributesPtr *)(space->allocateAlignedSpace(numOperands *
							sizeof(AttributesPtr)));
	
    else
      //      op_ = (AttributesPtr *)(new char[numOperands * sizeof(AttributesPtr)]);
      GenAssert(0, "Internal Error: must pass the space pointer.");

      
    Lng32 i = 0;
    Attributes *attr = NULL;
    for (i=0; i<num_operands;i++) {
      if (! op[i])
        continue;

      if ((op[i]->getAtp() < 0) ||
	  (op[i]->getAtpIndex() < 0) ||
	  (op[i]->getTupleFormat() == ExpTupleDesc::UNINITIALIZED_FORMAT))
	GenAssert(0, "Internal Error: Operand attributes are not valid.");

      if (space)
	attr = op[i]->newCopy(space);
      else
	//	op_[i] = op[i]->newCopy();
	GenAssert(0, "Internal Error: must pass the space pointer.");
  
      attr->setAtp(op[i]->getAtp());
      attr->setAtpIndex(op[i]->getAtpIndex());
      attr->setOffset(op[i]->getOffset());
      attr->setRelOffset(op[i]->getRelOffset());
      attr->setVoaOffset(op[i]->getVoaOffset());
      attr->setNullBitIndex(op[i]->getNullBitIndex());
      attr->setNextFieldIndex(op[i]->getNextFieldIndex());
      attr->setTupleFormat(op[i]->getTupleFormat());
      attr->setDefaultFieldNum(op[i]->getDefaultFieldNum());

      if (attr->getNullFlag()) {
	if(i == 0) flags_ |= ANY_OUTPUT_NULLABLE;
	else flags_ |= ANY_INPUT_NULLABLE;
      }
      op_[i] = attr;
    }

    if (op_[0]->showplan())
      {
	for (i = num_operands; i < numOperands; i++)
	  {
	    if (space)
	      op_[i] = op[i]->newCopy(space);
	    else
	      op_[i] = op[i]->newCopy();
	  }
      }
  }
  else
    op_ = 0;
};


ex_clause::~ex_clause()
{
}

// -----------------------------------------------------------------------
// This method returns the virtual function table pointer for an object
// with the given class ID; used by NAVersionedObject::driveUnpack().
// -----------------------------------------------------------------------
char *ex_clause::findVTblPtr(short classID)
{
  char *vtblPtr;
  switch (classID)
    {
    case ex_clause::COMP_TYPE:
      GetVTblPtr(vtblPtr, ex_comp_clause);
      break;
    case ex_clause::CONV_TYPE:
      GetVTblPtr(vtblPtr, ex_conv_clause);
      break;
    case ex_clause::UN_LOGIC_TYPE:
      GetVTblPtr(vtblPtr, ex_unlogic_clause);
      break;
    case ex_clause::ARITH_TYPE:
      GetVTblPtr(vtblPtr, ex_arith_clause);
      break;
    case ex_clause::ARITH_SUM_TYPE:
      GetVTblPtr(vtblPtr, ex_arith_sum_clause);
      break;
    case ex_clause::ARITH_COUNT_TYPE:
      GetVTblPtr(vtblPtr, ex_arith_count_clause);
      break;
    case ex_clause::LIKE_CLAUSE_CHAR_ID:
      GetVTblPtr(vtblPtr, ex_like_clause_char);
      break;
    case ex_clause::LIKE_CLAUSE_DOUBLEBYTE_ID:
      GetVTblPtr(vtblPtr, ex_like_clause_doublebyte);
      break;
    case ex_clause::REGEXP_CLAUSE_CHAR_ID:
            GetVTblPtr(vtblPtr, ExRegexpClauseChar);
      break;
    case ex_clause::FUNC_ASCII_ID:
      GetVTblPtr(vtblPtr, ExFunctionAscii);
      break;
    case ex_clause::FUNC_CHAR_ID:
      GetVTblPtr(vtblPtr, ExFunctionChar);
      break;
    case ex_clause::FUNC_CHAR_LEN_ID:
      GetVTblPtr(vtblPtr, ex_function_char_length);
      break;
    case ex_clause::FUNC_CHAR_LEN_DOUBLEBYTE_ID:
      GetVTblPtr(vtblPtr, ex_function_char_length_doublebyte);
      break;
    case ex_clause::FUNC_CVT_HEX_ID:
      GetVTblPtr(vtblPtr, ExFunctionConvertHex);
      break;
    case ex_clause::FUNC_OCT_LEN_ID:
      GetVTblPtr(vtblPtr, ex_function_oct_length);
      break;
    case ex_clause::FUNC_POSITION_ID:
      GetVTblPtr(vtblPtr, ex_function_position);
      break;
    case ex_clause::FUNC_POSITION_DOUBLEBYTE_ID:
      GetVTblPtr(vtblPtr, ex_function_position_doublebyte);
      break;
    case ex_clause::FUNC_SPLIT_PART_ID:
      GetVTblPtr(vtblPtr, ex_function_split_part);
      break;
    case ex_clause::FUNC_CONCAT_ID:
      GetVTblPtr(vtblPtr, ex_function_concat);
      break;
    case ex_clause::FUNC_REPEAT_ID:
      GetVTblPtr(vtblPtr, ExFunctionRepeat);
      break;
    case ex_clause::FUNC_REPLACE_ID:
      GetVTblPtr(vtblPtr, ExFunctionReplace);
      break;
    case ex_clause::FUNC_SUBSTR_ID:
      GetVTblPtr(vtblPtr, ex_function_substring);
      break;
    // 12/23/97: added for unicode
    case ex_clause::FUNC_SUBSTR_DOUBLEBYTE_ID:
      GetVTblPtr(vtblPtr, ex_function_substring_doublebyte);
      break;
    case ex_clause::FUNC_TRIM_ID:
      GetVTblPtr(vtblPtr, ex_function_trim_char);
      break;
    case ex_clause::FUNC_TRANSLATE_ID:
      GetVTblPtr(vtblPtr, ex_function_translate);
      break;
    // 12/29/97: added for unicode
    case ex_clause::FUNC_TRIM_DOUBLEBYTE_ID:
      GetVTblPtr(vtblPtr, ex_function_trim_doublebyte);
      break;
    case ex_clause::FUNC_LOWER_ID:
      GetVTblPtr(vtblPtr, ex_function_lower);
      break;
    case ex_clause::FUNC_UPPER_ID:
      GetVTblPtr(vtblPtr, ex_function_upper);
      break;
    // 12/17/97: added for unicode UPPER()
    case ex_clause::FUNC_UPPER_UNICODE_ID:
      GetVTblPtr(vtblPtr, ex_function_upper_unicode);
      break;
    // 12/17/97: added for unicode LOWER()
    case ex_clause::FUNC_LOWER_UNICODE_ID:
      GetVTblPtr(vtblPtr, ex_function_lower_unicode);
      break;
    case ex_clause::FUNC_SLEEP_ID:
      GetVTblPtr(vtblPtr, ex_function_sleep);
      break;
    case ex_clause::FUNC_UNIX_TIMESTAMP_ID:
      GetVTblPtr(vtblPtr, ex_function_unixtime);
      break;
    case ex_clause::FUNC_CURRENT_TIMESTAMP_ID:
      GetVTblPtr(vtblPtr, ex_function_current);
      break;
    case ex_clause::FUNC_INTERNALTIMESTAMP_ID:
      GetVTblPtr(vtblPtr, ExFunctionInternalTimestamp);
      break;
    case ex_clause::FUNC_GENERICUPDATEOUTPUT_ID:
      GetVTblPtr(vtblPtr, ExFunctionGenericUpdateOutput);
      break;
    case ex_clause::FUNC_UNIQUE_EXECUTE_ID_ID:
      GetVTblPtr(vtblPtr, ex_function_unique_execute_id);
      break;
    case ex_clause::FUNC_GET_TRIGGERS_STATUS_ID:
      GetVTblPtr(vtblPtr, ex_function_get_triggers_status);
      break;
    case ex_clause::FUNC_GET_BIT_VALUE_AT_ID:
      GetVTblPtr(vtblPtr, ex_function_get_bit_value_at);
      break;
    case ex_clause::FUNC_IS_BITWISE_AND_TRUE:
      GetVTblPtr(vtblPtr, ex_function_is_bitwise_and_true);
      break;
    case ex_clause::FUNC_ENCODE_ID:
      GetVTblPtr(vtblPtr, ex_function_encode);
      break;
    case ex_clause::FUNC_EXPLODE_VARCHAR_ID:
      GetVTblPtr(vtblPtr, ex_function_explode_varchar);
      break;
    case ex_clause::FUNC_HASH_ID:
      GetVTblPtr(vtblPtr, ex_function_hash);
      break;
    case ex_clause::FUNC_HASHCOMB_ID:
      GetVTblPtr(vtblPtr, ExHashComb);
      break;
    case ex_clause::FUNC_HDPHASH_ID:
      GetVTblPtr(vtblPtr, ExHDPHash);
      break;
    case ex_clause::FUNC_HDPHASHCOMB_ID:
      GetVTblPtr(vtblPtr, ExHDPHashComb);
      break;
    case ex_clause::FUNC_BITMUX_ID:
      GetVTblPtr(vtblPtr, ExpBitMuxFunction);
      break;
    case ex_clause::FUNC_REPLACE_NULL_ID:
      GetVTblPtr(vtblPtr, ex_function_replace_null);
      break;
    case ex_clause::FUNC_MOD_ID:
      GetVTblPtr(vtblPtr, ex_function_mod);
      break;
    case ex_clause::FUNC_MASK_ID:
      GetVTblPtr(vtblPtr, ex_function_mask);
      break;
    case ex_clause::FUNC_SHIFT_ID:
      GetVTblPtr(vtblPtr, ExFunctionShift);
      break;
    case ex_clause::FUNC_ABS_ID:
      GetVTblPtr(vtblPtr, ex_function_abs);
      break;
    case ex_clause::FUNC_BOOL_ID:
      GetVTblPtr(vtblPtr, ex_function_bool);
      break;
    case ex_clause::FUNC_CONVERTTIMESTAMP_ID:
      GetVTblPtr(vtblPtr, ex_function_converttimestamp);
      break;
    case ex_clause::FUNC_DATEFORMAT_ID:
      GetVTblPtr(vtblPtr, ex_function_dateformat);
      break;
    case ex_clause::FUNC_DAYOFWEEK_ID:
      GetVTblPtr(vtblPtr, ex_function_dayofweek);
      break;
    case ex_clause::FUNC_EXTRACT_ID:
      GetVTblPtr(vtblPtr, ex_function_extract);
      break;
    case ex_clause::FUNC_JULIANTIMESTAMP_ID:
      GetVTblPtr(vtblPtr, ex_function_juliantimestamp);
      break;
    case ex_clause::FUNC_EXEC_COUNT_ID:
      GetVTblPtr(vtblPtr, ex_function_exec_count);
      break;
    case ex_clause::FUNC_CURR_TRANSID_ID:
      GetVTblPtr(vtblPtr, ex_function_curr_transid);
      break;
    case ex_clause::FUNC_USER_ID:
      GetVTblPtr(vtblPtr, ex_function_user);
      break;
    case ex_clause::FUNC_ANSI_USER_ID:
      GetVTblPtr(vtblPtr, ex_function_ansi_user);
      break;
    case ex_clause::FUNC_VARIANCE_ID:
      GetVTblPtr(vtblPtr, ExFunctionSVariance);
      break;
    case ex_clause::FUNC_STDDEV_ID:
      GetVTblPtr(vtblPtr, ExFunctionSStddev);
      break;
    case ex_clause::FUNC_RAISE_ERROR_ID:
      GetVTblPtr(vtblPtr, ExpRaiseErrorFunction);
      break;
    case ex_clause::FUNC_RANDOMNUM_ID:
      GetVTblPtr(vtblPtr, ExFunctionRandomNum);
      break;
    case ex_clause::FUNC_RAND_SELECTION_ID:
      GetVTblPtr(vtblPtr, ExFunctionRandomSelection);
      break;
    case ex_clause::FUNC_PROGDISTRIB_ID:
      GetVTblPtr(vtblPtr, ExProgDistrib);
      break;
    case ex_clause::FUNC_PROGDISTKEY_ID:
      GetVTblPtr(vtblPtr, ExProgDistribKey);
      break;
    case ex_clause::FUNC_PAGROUP_ID:
      GetVTblPtr(vtblPtr, ExPAGroup);
      break;
    case ex_clause::FUNC_HASH2_DISTRIB_ID:
      GetVTblPtr(vtblPtr, ExHash2Distrib);
      break;
    case ex_clause::FUNC_HEADER:
      GetVTblPtr(vtblPtr, ExHeaderClause);
      break;
    case ex_clause::FUNC_UNPACKCOL_ID:
      GetVTblPtr(vtblPtr, ExUnPackCol);
      break;
    case ex_clause::FUNC_PACK_ID:
      GetVTblPtr(vtblPtr, ExFunctionPack);
      break;
    case ex_clause::FUNC_RANGE_LOOKUP_ID:
      GetVTblPtr(vtblPtr, ExFunctionRangeLookup);
      break;
    case ex_clause::FUNC_OFFSET_ID:
      GetVTblPtr(vtblPtr, ExpSequenceFunction);
      break;
    case ex_clause::FUNCTION_TYPE:
      GetVTblPtr(vtblPtr, ex_function_clause);
      break;
    case ex_clause::FUNC_MATH_ID:
      GetVTblPtr(vtblPtr, ExFunctionMath);
      break;
    case ex_clause::FUNC_BIT_OPER_ID:
      GetVTblPtr(vtblPtr, ExFunctionBitOper);
      break;
    case ex_clause::BOOL_RESULT_TYPE:
      GetVTblPtr(vtblPtr, bool_result_clause);
      break;
    case ex_clause::BOOL_TYPE:
      GetVTblPtr(vtblPtr, ex_bool_clause);
      break;
    case ex_clause::BRANCH_TYPE:
      GetVTblPtr(vtblPtr, ex_branch_clause);
      break;
    case ex_clause::INOUT_TYPE:
      GetVTblPtr(vtblPtr, ex_inout_clause);
      break;
    case ex_clause::NOOP_TYPE:
      GetVTblPtr(vtblPtr, ex_noop_clause);
      break;
    case ex_clause::AGGR_ONE_ROW_ID:
      GetVTblPtr(vtblPtr, ex_aggr_one_row_clause);
      break;
    case ex_clause::AGGR_ANY_TRUE_MAX_ID:
      GetVTblPtr(vtblPtr, ex_aggr_any_true_max_clause);
      break;
    case ex_clause::AGGR_MIN_MAX_ID:
      GetVTblPtr(vtblPtr, ex_aggr_min_max_clause);
      break;
    case ex_clause::AGGR_GROUPING_ID:
      GetVTblPtr(vtblPtr, ExFunctionGrouping);
      break;
    case ex_clause::AGGREGATE_TYPE:
      GetVTblPtr(vtblPtr, ex_aggregate_clause);
      break;
    case ex_clause::FUNC_ROWSETARRAY_SCAN_ID:
      GetVTblPtr(vtblPtr, ExRowsetArrayScan)
      break;
    case ex_clause::FUNC_ROWSETARRAY_ROW_ID:
      GetVTblPtr(vtblPtr, ExRowsetArrayRowid);
      break;
    case ex_clause::FUNC_ROWSETARRAY_INTO_ID:
      GetVTblPtr(vtblPtr, ExRowsetArrayInto);
      break;
    case ex_clause::FUNC_NULLIFZERO:
      GetVTblPtr(vtblPtr, ex_function_nullifzero);
      break;
    case ex_clause::FUNC_NVL:
      GetVTblPtr(vtblPtr, ex_function_nvl);
      break;
    case ex_clause::FUNC_JSON_ID:
      GetVTblPtr(vtblPtr, ex_function_json_object_field_text);
      break;
    case ex_clause::FUNC_QUERYID_EXTRACT:
      GetVTblPtr(vtblPtr, ex_function_queryid_extract);
      break;
    case ex_clause::FUNC_TOKENSTR_ID:
      GetVTblPtr(vtblPtr, ExFunctionTokenStr);
      break;
    case ex_clause::FUNC_REVERSE_ID:
      GetVTblPtr(vtblPtr, ExFunctionReverseStr);
      break;
    case ex_clause::LOB_INSERT:
      GetVTblPtr(vtblPtr, ExpLOBinsert);
      break;
    case ex_clause::LOB_SELECT:
      GetVTblPtr(vtblPtr, ExpLOBselect);
      break;
    case ex_clause::LOB_DELETE:
      GetVTblPtr(vtblPtr, ExpLOBdelete);
      break;
    case ex_clause::LOB_UPDATE:
      GetVTblPtr(vtblPtr, ExpLOBupdate);
      break;
    case ex_clause::LOB_CONVERT:
      GetVTblPtr(vtblPtr, ExpLOBconvert);
      break;
    case ex_clause::LOB_CONVERTHANDLE:
      GetVTblPtr(vtblPtr, ExpLOBconvertHandle);
      break;
    case ex_clause::LOB_FUNC_SUBSTR:
      GetVTblPtr(vtblPtr, ExpLOBfuncSubstring);
      break;
    case ex_clause::FUNC_HIVEHASH_ID:
      GetVTblPtr(vtblPtr, ex_function_hivehash);
      break;
    case ex_clause::FUNC_HIVEHASHCOMB_ID:
      GetVTblPtr(vtblPtr, ExHiveHashComb);
      break;
    case ex_clause::FUNC_UNIQUE_ID:
      GetVTblPtr(vtblPtr, ExFunctionUniqueId);
      break;
    case ex_clause::FUNC_ROWNUM:
      GetVTblPtr(vtblPtr, ExFunctionRowNum);
      break;
    case ex_clause::FUNC_HBASE_COLUMN_LOOKUP:
      GetVTblPtr(vtblPtr, ExFunctionHbaseColumnLookup);
      break;
    case ex_clause::FUNC_HBASE_COLUMNS_DISPLAY:
      GetVTblPtr(vtblPtr, ExFunctionHbaseColumnsDisplay);
      break;
    case ex_clause::FUNC_HBASE_COLUMN_CREATE:
      GetVTblPtr(vtblPtr, ExFunctionHbaseColumnCreate);
      break;
    case ex_clause::FUNC_CAST_TYPE:
      GetVTblPtr(vtblPtr, ExFunctionCastType);
      break;
    case ex_clause::FUNC_SEQUENCE_VALUE:
      GetVTblPtr(vtblPtr, ExFunctionSequenceValue);
      break;
    case ex_clause::FUNC_PIVOT_GROUP:
      GetVTblPtr(vtblPtr, ex_pivot_group_clause);
      break;
    case ex_clause::FUNC_HBASE_TIMESTAMP:
      GetVTblPtr(vtblPtr, ExFunctionHbaseTimestamp);
      break;
    case ex_clause::FUNC_HBASE_VERSION:
      GetVTblPtr(vtblPtr, ExFunctionHbaseVersion);
      break;
    case ex_clause::FUNC_SHA1_ID:
      GetVTblPtr(vtblPtr, ExFunctionSha);
      break;
    case ex_clause::FUNC_SHA2_ID:
      GetVTblPtr(vtblPtr, ExFunctionSha2);
      break;
    case ex_clause::FUNC_MD5_ID:
      GetVTblPtr(vtblPtr, ExFunctionMd5);
      break;
    case ex_clause::FUNC_CRC32_ID:
      GetVTblPtr(vtblPtr, ExFunctionCrc32);
      break;
    case ex_clause::FUNC_ISIP_ID:
      GetVTblPtr(vtblPtr, ExFunctionIsIP);
      break;
    case ex_clause::FUNC_INETATON_ID:
      GetVTblPtr(vtblPtr, ExFunctionInetAton);
      break;
    case ex_clause::FUNC_INETNTOA_ID:
      GetVTblPtr(vtblPtr, ExFunctionInetNtoa);
      break;
    case ex_clause::FUNC_SOUNDEX_ID:
      GetVTblPtr(vtblPtr, ExFunctionSoundex);
      break;
    case ex_clause::FUNC_AES_ENCRYPT:
      GetVTblPtr(vtblPtr, ExFunctionAESEncrypt);
      break;
    case ex_clause::FUNC_AES_DECRYPT:
      GetVTblPtr(vtblPtr, ExFunctionAESDecrypt);
      break;
    case ex_clause::FUNC_BASE64_ENC_DEC:
      GetVTblPtr(vtblPtr, ExFunctionBase64EncDec);
      break;
    default:
      GetVTblPtr(vtblPtr, ex_clause);
      break;
    }
  return vtblPtr;
}


ex_expr::exp_return_type ex_clause::processNulls(char *null_data[],
						 CollHeap *heap,
						 ComDiagsArea **diagsArea)
{
  
  for (short i = 1; i < getNumOperands(); i++)
    {
      // if value is missing, then it is a null value.
      // Move it to result and return. 
      if ((getOperand(i)->getNullFlag()) &&    // nullable
	  (! null_data[i]))                    // missing value 
	{
	  // This test is only needed in derived ex_conv_clause::processNulls;
	  // (! get_operand(0)->getNullFlag()) should be impossible here.
	  //
	  // if (! get_operand(0)->getNullFlag()) // target not nullable
	  //   {
	  //     // Attempt to put NULL into a column with a
	  //     // NOT NULL NONDROPPABLE constraint.
	  //     // ## Need to supply name of constraint and name of table here.
	  //     ExRaiseSqlError(heap, diagsArea, EXE_TABLE_CHECK_CONSTRAINT);
	  //     return ex_expr::EXPR_ERROR;
	  //   }

	  // move null value to result.
          ExpTupleDesc::setNullValue( null_data[0],
                                      getOperand(0)->getNullBitIndex(),
                                      getOperand(0)->getTupleFormat() );
	  return ex_expr::EXPR_NULL;
	}
    }
  
  // move 0 to the null bytes of result
  if (getOperand(0)->getNullFlag())
    {
      ExpTupleDesc::clearNullValue( null_data[0],
                                    getOperand(0)->getNullBitIndex(),
                                    getOperand(0)->getTupleFormat() );
    }
  
  return ex_expr::EXPR_OK;
}

Long ex_clause::packClause(void * space, Lng32 /*size*/)
{
  if (op_) {
    if (op_[0]->showplan()) {
      for (Lng32 i=numOperands_; i < 2 * numOperands_; i++)
        op_[i].pack(space);
    }
    for (Lng32 i=0; i < numOperands_; i++) 
      op_[i].pack(space);
  }
  op_.packShallow(space);
  return NAVersionedObject::pack(space);
}

Long ex_clause::pack(void * space)
{
  return packClause(space, sizeof(ex_clause));
}

Lng32 ex_clause::unpackClause(void *base, void * reallocator)
{
  if (op_) {
    if (op_.unpackShallow(base)) return -1;
    for (Lng32 i=0; i < numOperands_; i++) {
      if (op_[i].unpack(base, reallocator)) return -1;
    }
    if (op_[0]->showplan()) {
	  for (Lng32 i=numOperands_; i < 2 * numOperands_; i++) {
        if (op_[i].unpack(base, reallocator)) return -1;
      }
    }
  }
  return NAVersionedObject::unpack(base, reallocator);
}

Lng32 ex_clause::unpack(void *base, void * reallocator)
{
  return unpackClause(base, reallocator);
}

const char * getOperTypeEnumAsString(Int16 /*OperatorTypeEnum*/ ote)
{
  switch (ote)
    {
    // Note, this list is arranged in the same order as the types
    // appear in common/OperTypeEnum.h, please keep the same order
    // when adding new types
    case ITM_AND: return "ITM_AND";
    case ITM_OR: return "ITM_OR";

    // unary logic operators
    case ITM_NOT: return "ITM_NOT";
    case ITM_IS_TRUE: return "ITM_IS_TRUE";
    case ITM_IS_FALSE: return "ITM_IS_FALSE";
    case ITM_IS_NULL: return "ITM_IS_NULL";
    case ITM_IS_NOT_NULL: return "ITM_IS_NOT_NULL";
    case ITM_IS_UNKNOWN: return "ITM_IS_UNKNOWN";
    case ITM_IS_NOT_UNKNOWN: return "ITM_IS_NOT_UNKNOWN";

    // binary comparison operators
    case ITM_EQUAL: return "ITM_EQUAL";
    case ITM_NOT_EQUAL: return "ITM_NOT_EQUAL";
    case ITM_LESS: return "ITM_LESS";
    case ITM_LESS_EQ: return "ITM_LESS_EQ";
    case ITM_GREATER: return "ITM_GREATER";
    case ITM_GREATER_EQ: return "ITM_GREATER_EQ";

    // unary arithmetic operators
    case ITM_NEGATE: return "ITM_NEGATE";
    case ITM_INVERSE: return "ITM_INVERSE";
    // binary arithmetic operators
    case ITM_PLUS: return "ITM_PLUS";
    case ITM_MINUS: return "ITM_MINUS";
    case ITM_TIMES: return "ITM_TIMES";
    case ITM_DIVIDE: return "ITM_DIVIDE";
    case ITM_EXPONENT: return "ITM_EXPONENT";

    // aggregate functions
    case ITM_AVG: return "ITM_AVG";
    case ITM_MAX: return "ITM_MAX";
    case ITM_MIN: return "ITM_MIN";
    case ITM_MAX_ORDERED: return "ITM_MAX_ORDERED";
    case ITM_MIN_ORDERED: return "ITM_MIN_ORDERED";
    case ITM_SUM: return "ITM_SUM";
    case ITM_COUNT: return "ITM_COUNT";
    case ITM_COUNT_NONULL: return "ITM_COUNT_NONULL";
    case ITM_STDDEV: return "ITM_STDDEV";
    case ITM_VARIANCE: return "ITM_VARIANCE";
    case ITM_BASECOL: return "ITM_BASECOL";

    case ITM_ONE_ROW: return "ITM_ONE_ROW";
    case ITM_ONEROW: return "ITM_ONEROW";
    case ITM_ONE_TRUE: return "ITM_ONE_TRUE";
    case ITM_ANY_TRUE: return "ITM_ANY_TRUE";
    case ITM_ANY_TRUE_MAX: return "ITM_ANY_TRUE_MAX";
    case ITM_MAX_INCL_NULL: return "ITM_MAX_INCL_NULL";

    case ITM_PIVOT_GROUP: return "ITM_PIVOT_GROUP";

    case ITM_AGGR_MIN_MAX: return "ITM_AGGR_MIN_MAX";
    case ITM_AGGR_GROUPING_FUNC: return "ITM_AGGR_GROUPING_FUNC";

    // custom functions
    case ITM_USER_DEF_FUNCTION: return "ITM_USER_DEF_FUNCTION";
    case ITM_BETWEEN: return "ITM_BETWEEN";
    case ITM_LIKE: return "ITM_LIKE";
    case ITM_REGEXP: return "ITM_REGEXP";
    case ITM_UNIX_TIMESTAMP: return "ITM_UNIX_TIMESTAMP";
    case ITM_SLEEP: return "ITM_SLEEP";
    case ITM_CURRENT_TIMESTAMP: return "ITM_CURRENT_TIMESTAMP";
    case ITM_CURRENT_USER: return "ITM_CURRENT_USER";
    case ITM_SESSION_USER: return "ITM_SESSION_USER";
    case ITM_USER: return "ITM_USER";
    case ITM_AUTHNAME: return "ITM_AUTHNAME";
    case ITM_AUTHTYPE: return "ITM_AUTHTYPE";

    case ITM_BOOL_RESULT: return "ITM_BOOL_RESULT";
    case ITM_NO_OP: return "ITM_NO_OP";

    case ITM_CASE: return "ITM_CASE";
    case ITM_IF_THEN_ELSE: return "ITM_IF_THEN_ELSE";
    case ITM_RETURN_TRUE: return "ITM_RETURN_TRUE";
    case ITM_RETURN_FALSE: return "ITM_RETURN_FALSE";
    case ITM_RETURN_NULL: return "ITM_RETURN_NULL";
    case ITM_COMP_ENCODE: return "ITM_COMP_ENCODE";
    case ITM_COMP_DECODE: return "ITM_COMP_DECODE";
    case ITM_HASH: return "ITM_HASH";
    case ITM_REPLACE_NULL: return "ITM_REPLACE_NULL";
    case ITM_PACK_FUNC: return "ITM_PACK_FUNC";
    case ITM_BITMUX: return "ITM_BITMUX";
    case ITM_OVERLAPS: return "ITM_OVERLAPS";
    case ITM_RAISE_ERROR: return "ITM_RAISE_ERROR";

    case ITM_USERID: return "ITM_USERID";

    // sequence functions
    case ITM_DIFF1            : return "ITM_DIFF1";
    case ITM_DIFF2            : return "ITM_DIFF2";
    case ITM_LAST_NOT_NULL    : return "ITM_LAST_NOT_NULL";
    case ITM_MOVING_COUNT     : return "ITM_MOVING_COUNT";
    case ITM_MOVING_SUM       : return "ITM_MOVING_SUM";
    case ITM_MOVING_AVG       : return "ITM_MOVING_AVG";
    case ITM_MOVING_MAX       : return "ITM_MOVING_MAX";
    case ITM_MOVING_MIN       : return "ITM_MOVING_MIN";
    case ITM_MOVING_SDEV      : return "ITM_MOVING_SDEV";
    case ITM_MOVING_VARIANCE  : return "ITM_MOVING_VARIANCE";
    case ITM_OFFSET           : return "ITM_OFFSET";
    case ITM_RUNNING_COUNT    : return "ITM_RUNNING_COUNT";
    case ITM_ROWS_SINCE       : return "ITM_ROWS_SINCE";
    case ITM_RUNNING_SUM      : return "ITM_RUNNING_SUM";
    case ITM_RUNNING_AVG      : return "ITM_RUNNING_AVG";
    case ITM_RUNNING_MAX      : return "ITM_RUNNING_MAX";
    case ITM_RUNNING_MIN      : return "ITM_RUNNING_MIN";
    case ITM_RUNNING_SDEV     : return "ITM_RUNNING_SDEV";
    case ITM_RUNNING_VARIANCE : return "ITM_RUNNING_VARIANCE";
    case ITM_THIS             : return "ITM_THIS";
    case ITM_NOT_THIS         : return "ITM_NOT_THIS";

    // flow control
    case ITM_DO_WHILE         : return "ITM_DO_WHILE";
    case ITM_BLOCK            : return "ITM_BLOCK";
    case ITM_WHILE            : return "ITM_WHILE";

    // scalar min/max

    case ITM_SCALAR_MIN       : return "ITM_SCALAR_MIN";
    case ITM_SCALAR_MAX       : return "ITM_SCALAR_MAX";

    case ITM_CURRENT_TIMESTAMP_RUNNING: return "ITM_CURRENT_TIMESTAMP_RUNNING";

    // numeric functions
    case ITM_ABS: return "ITM_ABS";
    case ITM_CEIL: return "ITM_CEIL";
    case ITM_COS: return "ITM_COS";
    case ITM_COSH: return "ITM_COSH";
    case ITM_FLOOR: return "ITM_FLOOR";
    case ITM_LOG: return "ITM_LOG";
    case ITM_LOG10: return "ITM_LOG10";
    case ITM_LOG2: return "ITM_LOG2";
    case ITM_MOD: return "ITM_MOD";
    case ITM_POWER: return "ITM_POWER";
    case ITM_ROUND: return "ITM_ROUND";
    case ITM_SIGN: return "ITM_SIGN";
    case ITM_SIN: return "ITM_SIN";
    case ITM_SINH: return "ITM_SINH";
    case ITM_SQRT: return "ITM_SQRT";
    case ITM_TAN: return "ITM_TAN";
    case ITM_TANH: return "ITM_TANH";
    case ITM_ROUND_ROBIN: return "ITM_ROUND_ROBIN";
    case ITM_ACOS: return "ITM_ACOS";
    case ITM_ASIN: return "ITM_ASIN";
    case ITM_ATAN: return "ITM_ATAN";
    case ITM_ATAN2: return "ITM_ATAN2";
    case ITM_DEGREES: return "ITM_DEGREES";
    case ITM_EXP: return "ITM_EXP";
    case ITM_PI: return "ITM_PI";
    case ITM_RADIANS: return "ITM_RADIANS";
    case ITM_SCALE_TRUNC: return "ITM_SCALE_TRUNC";
    case ITM_MASK_CLEAR: return "ITM_MASK_CLEAR";
    case ITM_MASK_SET: return "ITM_MASK_SET";
    case ITM_SHIFT_RIGHT: return "ITM_SHIFT_RIGHT";
    case ITM_SHIFT_LEFT: return "ITM_SHIFT_LEFT";
    case ITM_BITAND: return "ITM_BITAND";
    case ITM_BITOR: return "ITM_BITOR";
    case ITM_BITXOR: return "ITM_BITXOR";
    case ITM_BITNOT: return "ITM_BITNOT";
    case ITM_BITEXTRACT: return "ITM_BITEXTRACT";

    // string functions
    case ITM_TRUNC: return "ITM_TRUNC";
    case ITM_ASCII: return "ITM_ASCII";
    case ITM_CODE_VALUE: return "ITM_CODE_VALUE";
    case ITM_POSITION: return "ITM_POSITION";
    case ITM_CHAR_LENGTH: return "ITM_CHAR_LENGTH";
    case ITM_INSERT_STR: return "ITM_INSERT_STR";
    case ITM_OCTET_LENGTH: return "ITM_OCTET_LENGTH";
    case ITM_LOWER: return "ITM_LOWER";
    case ITM_LPAD: return "ITM_LPAD";
    case ITM_LTRIM: return "ITM_LTRIM";
    case ITM_REPLACE: return "ITM_REPLACE";
    case ITM_RPAD: return "ITM_RPAD";
    case ITM_RTRIM: return "ITM_RTRIM";
    case ITM_SOUNDEX: return "ITM_SOUNDEX";
    case ITM_SUBSTR: return "ITM_SUBSTR";
    case ITM_TRIM: return "ITM_TRIM";
    case ITM_UPPER: return "ITM_UPPER";
    case ITM_CHAR: return "ITM_CHAR";
    case ITM_CONCAT: return "ITM_CONCAT";
    case ITM_UNPACKCOL: return "ITM_UNPACKCOL";
    case ITM_EXPLODE_VARCHAR: return "ITM_EXPLODE_VARCHAR";
    case ITM_REPEAT: return "ITM_REPEAT";
    case ITM_RIGHT: return "ITM_RIGHT";
    case ITM_CONVERTTOBITS: return "ITM_CONVERTTOBITS";
    case ITM_CONVERTTOHEX: return "ITM_CONVERTTOHEX";
    case ITM_CONVERTFROMHEX: return "ITM_CONVERTFROMHEX";
    case ITM_TOKENSTR: return "ITM_TOKENSTR";
    case ITM_REVERSE: return "ITM_REVERSE";

    // UNICODE/DOUBLEBYTE charsets built-in functions
    case ITM_SUBSTR_DOUBLEBYTE: return "ITM_SUBSTR_DOUBLEBYTE";
    case ITM_TRIM_DOUBLEBYTE: return "ITM_TRIM_DOUBLEBYTE";
    case ITM_CHAR_LENGTH_DOUBLEBYTE: return "ITM_CHAR_LENGTH_DOUBLEBYTE";
    case ITM_POSITION_DOUBLEBYTE: return "ITM_POSITION_DOUBLEBYTE";
    case ITM_LIKE_DOUBLEBYTE: return "ITM_LIKE_DOUBLEBYTE";
    case ITM_UPPER_UNICODE: return "ITM_UPPER_UNICODE";
    case ITM_LOWER_UNICODE: return "ITM_LOWER_UNICODE";
    case ITM_REPEAT_UNICODE: return "ITM_REPEAT_UNICODE";
    case ITM_REPLACE_UNICODE: return "ITM_REPLACE_UNICODE";
    case ITM_UNICODE_CODE_VALUE: return "ITM_UNICODE_CODE_VALUE";
    case ITM_NCHAR_MP_CODE_VALUE: return "ITM_NCHAR_MP_CODE_VALUE";
    // translate function
    case ITM_TRANSLATE: return "ITM_TRANSLATE";

    case ITM_UNICODE_CHAR: return "ITM_UNICODE_CHAR";
    case ITM_NCHAR_MP_CHAR: return "ITM_NCHAR_MP_CHAR";

    // RowSet expression functions
    case ITM_ROWSETARRAY_SCAN: return "ITM_ROWSETARRAY_SCAN";
    case ITM_ROWSETARRAY_ROWID: return "ITM_ROWSETARRAY_ROWID";
    case ITM_ROWSETARRAY_INTO : return "ITM_ROWSETARRAY_INTO";

    case ITM_LEFT: return "ITM_LEFT";
    case ITM_SPACE: return "ITM_SPACE";
    case ITM_ODBC_LENGTH: return "ITM_ODBC_LENGTH";

    // datetime functions
    case ITM_CONVERTTIMESTAMP: return "ITM_CONVERTTIMESTAMP";
    case ITM_DATEFORMAT: return "ITM_DATEFORMAT";
    case ITM_DAYOFWEEK: return "ITM_DAYOFWEEK";
    case ITM_EXTRACT: return "ITM_EXTRACT";
    case ITM_INITCAP: return "ITM_INITCAP";
    case ITM_JULIANTIMESTAMP: return "ITM_JULIANTIMESTAMP";
    case ITM_EXTRACT_ODBC: return "ITM_EXTRACT_ODBC";
    case ITM_DAYNAME: return "ITM_DAYNAME";
    case ITM_MONTHNAME: return "ITM_MONTHNAME";
    case ITM_QUARTER: return "ITM_QUARTER";
    case ITM_WEEK: return "ITM_WEEK";
    case ITM_DAYOFYEAR: return "ITM_DAYOFYEAR";
    case ITM_FIRSTDAYOFYEAR: return "ITM_FIRSTDAYOFYEAR";
    case ITM_INTERNALTIMESTAMP: return "ITM_INTERNALTIMESTAMP";
    // misc. functions
    case ITM_NARROW: return "ITM_NARROW";
    case ITM_INTERVAL: return "ITM_INTERVAL";
    case ITM_INSTANTIATE_NULL: return "ITM_INSTANTIATE_NULL";
    case ITM_INCREMENT: return "ITM_INCREMENT";
    case ITM_DECREMENT: return "ITM_DECREMENT";
    case ITM_GREATER_OR_GE: return "ITM_GREATER_OR_GE";
    case ITM_LESS_OR_LE: return "ITM_LESS_OR_LE";
    case ITM_RANGE_LOOKUP: return "ITM_RANGE_LOOKUP";
    case ITM_DECODE: return "ITM_DECODE";
    case ITM_HDPHASHCOMB: return "ITM_HDPHASHCOMB";
    case ITM_RANDOMNUM: return "ITM_RANDOMNUM";
    case ITM_PROGDISTRIB: return "ITM_PROGDISTRIB";
    case ITM_HASHCOMB: return "ITM_HASHCOMB";
    case ITM_HDPHASH: return "ITM_HDPHASH";
    case ITM_EXEC_COUNT: return "ITM_EXEC_COUNT";
    case ITM_CURR_TRANSID: return "ITM_CURR_TRANSID";
    case ITM_NOTCOVERED: return "ITM_NOTCOVERED";
    case ITM_BALANCE: return "ITM_BALANCE";
    case ITM_RAND_SELECTION: return "ITM_RAND_SELECTION";
    case ITM_PROGDISTRIBKEY: return "ITM_PROGDISTRIBKEY";
    case ITM_PAGROUP: return "ITM_PAGROUP";
    case ITM_HASH2_DISTRIB: return "ITM_HASH2_DISTRIB";

    case ITM_HEADER: return "ITM_HEADER";

    case ITM_LOBINSERT: return "ITM_LOBINSERT";
    case ITM_LOBSELECT: return "ITM_LOBSELECT";
    case ITM_LOBDELETE: return "ITM_LOBDELETE";
    case ITM_LOBUPDATE: return "ITM_LOBUPDATE";
    case ITM_LOBCONVERT: return "ITM_LOBCONVERT";
    case ITM_LOBCONVERTHANDLE: return "ITM_LOBCONVERTHANDLE";

    case ITM_UNIQUE_EXECUTE_ID: return "ITM_UNIQUE_EXECUTE_ID";
    case ITM_GET_TRIGGERS_STATUS: return "ITM_GET_TRIGGERS_STATUS";
    case ITM_GET_BIT_VALUE_AT: return "ITM_GET_BIT_VALUE_AT";
    case ITM_CURRENTEPOCH: return "ITM_CURRENTEPOCH";
    case ITM_VSBBROWTYPE: return "ITM_VSBBROWTYPE";
    case ITM_VSBBROWCOUNT: return "ITM_VSBBROWCOUNT";
    case ITM_IS_BITWISE_AND_TRUE: return "ITM_IS_BITWISE_AND_TRUE";

    case ITM_NULLIFZERO: return "ITM_NULLIFZERO";
    case ITM_NVL: return "ITM_NVL";

    case ITM_JSONOBJECTFIELDTEXT: return "ITM_JSONOBJECTFIELDTEXT";

    // subqueries
    case ITM_ROW_SUBQUERY: return "ITM_ROW_SUBQUERY";
    case ITM_IN_SUBQUERY: return "ITM_IN_SUBQUERY";
    case ITM_IN: return "ITM_IN";
    case ITM_EXISTS: return "ITM_EXISTS";
    case ITM_NOT_EXISTS: return "ITM_NOT_EXISTS";
    case ITM_EQUAL_ALL: return "ITM_EQUAL_ALL";
    case ITM_EQUAL_ANY: return "ITM_EQUAL_ANY";
    case ITM_NOT_EQUAL_ALL: return "ITM_NOT_EQUAL_ALL";
    case ITM_NOT_EQUAL_ANY: return "ITM_NOT_EQUAL_ANY";
    case ITM_LESS_ALL: return "ITM_LESS_ALL";
    case ITM_LESS_ANY: return "ITM_LESS_ANY";
    case ITM_GREATER_ALL: return "ITM_GREATER_ALL";
    case ITM_GREATER_ANY: return "ITM_GREATER_ANY";
    case ITM_LESS_EQ_ALL: return "ITM_LESS_EQ_ALL";
    case ITM_LESS_EQ_ANY: return "ITM_LESS_EQ_ANY";
    case ITM_GREATER_EQ_ALL: return "ITM_GREATER_EQ_ALL";
    case ITM_GREATER_EQ_ANY: return "ITM_GREATER_EQ_ANY";

    case ITM_WILDCARD_EQ_NE: return "ITM_WILDCARD_EQ_NE";

    // renaming, conversion, assignment
    case ITM_RENAME_COL: return "ITM_RENAME_COL";
    case ITM_CONVERT: return "ITM_CONVERT";
    case ITM_CAST: return "ITM_CAST";
    case ITM_ASSIGN: return "ITM_ASSIGN";

    // convert an NA-type to an item expression
    case ITM_NATYPE: return "ITM_NATYPE";

    // do a cast but adjust target length based
    // on operand (used by ODBC)
    case ITM_CAST_CONVERT: return "ITM_CAST_CONVERT";

    case ITM_CAST_TYPE: return "ITM_CAST_TYPE";

    // for OperatorType::match() of ItemExpr::origOpType()
    case ITM_ANY_AGGREGATE: return "ITM_ANY_AGGREGATE";

    // to match Cast, Cast_Convert, Instantiate_Null, Narrow
    case ITM_ANY_CAST: return "ITM_ANY_CAST";

    // item expressions describing constraints
    case ITM_CHECK_CONSTRAINT: return "ITM_CHECK_CONSTRAINT";
    case ITM_CARD_CONSTRAINT: return "ITM_CARD_CONSTRAINT";
    case ITM_UNIQUE_CONSTRAINT: return "ITM_UNIQUE_CONSTRAINT";
    case ITM_REF_CONSTRAINT: return "ITM_REF_CONSTRAINT";
    case ITM_UNIQUE_OPT_CONSTRAINT: return "ITM_UNIQUE_OPT_CONSTRAINT";
    case ITM_FUNC_DEPEND_CONSTRAINT: return "ITM_FUNC_DEPEND_CONSTRAINT";

    // list of item expressions
    case ITM_ITEM_LIST: return "ITM_ITEM_LIST";

    // leaf nodes of item expressions
    case ITM_CONSTANT: return "ITM_CONSTANT";
    case ITM_REFERENCE: return "ITM_REFERENCE";
    case ITM_BASECOLUMN: return "ITM_BASECOLUMN";
    case ITM_INDEXCOLUMN: return "ITM_INDEXCOLUMN";
    case ITM_HOSTVAR: return "ITM_HOSTVAR";
    case ITM_DYN_PARAM: return "ITM_DYN_PARAM";
    case ITM_SEL_INDEX: return "ITM_SEL_INDEX";
    case ITM_VALUEIDREF: return "ITM_VALUEIDREF";
    case ITM_VALUEIDUNION: return "ITM_VALUEIDUNION";
    case ITM_VEG: return "ITM_VEG";
    case ITM_VEG_PREDICATE: return "ITM_VEG_PREDICATE";
    case ITM_VEG_REFERENCE: return "ITM_VEG_REFERENCE";
    case ITM_DEFAULT_SPECIFICATION: return "ITM_DEFAULT_SPECIFICATION";
    case ITM_SAMPLE_VALUE: return "ITM_SAMPLE_VALUE";
    case ITM_CACHE_PARAM: return "ITM_CACHE_PARAM";

    // Item expressions for transactions
    case ITM_SET_TRANS_ISOLATION_LEVEL: return "ITM_SET_TRANS_ISOLATION_LEVEL";
    case ITM_SET_TRANS_ACCESS_MODE: return "ITM_SET_TRANS_ACCESS_MODE";
    case ITM_SET_TRANS_DIAGS: return "ITM_SET_TRANS_DIAGS";
    case ITM_SET_TRANS_ROLLBACK_MODE: return "ITM_SET_TRANS_ROLLBACK_MODE";
    case ITM_SET_TRANS_AUTOABORT_INTERVAL: return "ITM_SET_TRANS_AUTOABORT_INTERVAL";
    case ITM_SET_TRANS_MULTI_COMMIT: return "ITM_SET_TRANS_MULTI_COMMIT";

    case ITM_LAST_ITEM_OP: return "ITM_LAST_ITEM_OP";
    case ITM_UNIQUE_ID: return "ITM_UNIQUE_ID";
    case ITM_UNIQUE_ID_SYS_GUID: return "ITM_UNIQUE_ID_SYS_GUID";
    case ITM_UNIQUE_SHORT_ID: return "ITM_UNIQUE_SHORT_ID";
    case ITM_ROWNUM: return "ITM_ROWNUM";
    case ITM_HBASE_COLUMN_LOOKUP: return "ITM_HBASE_COLUMN_LOOKUP";
    case ITM_HBASE_COLUMNS_DISPLAY: return "ITM_HBASE_COLUMNS_DISPLAY";
    case ITM_HBASE_COLUMN_CREATE: return "ITM_HBASE_COLUMN_CREATE";
    case ITM_HBASE_TIMESTAMP: return "ITM_HBASE_TIMESTAMP";
    case ITM_HBASE_VERSION: return "ITM_HBASE_VERSION";

    case ITM_SEQUENCE_VALUE: return "ITM_SEQUENCE_VALUE";

    case ITM_ENCODE_BASE64: return "ITM_ENCODE_BASE64";
    case ITM_DECODE_BASE64: return "ITM_DECODE_BASE64";

    // Note, this list is arranged in the same order as the types
    // appear in common/OperTypeEnum.h, please keep the same order
    // when adding new types
    default: 
      {
	cout << "OperatorType must be added to getOperTypeEnumAsString()"
	     << ote << endl;
	return "Add To getOperTypeEnumAsString()";
      }
    }
}

char * exClauseGetText(OperatorTypeEnum ote)
{
  char * itmText = (char *)getOperTypeEnumAsString(ote);
  
  // strip the ITM_ prefix
  if ((str_len(itmText) > 4) && (str_cmp(itmText, "ITM_", 4) == 0))
    return &itmText[4];
  else
    return itmText;
} // exClausegetText()

void ex_clause::displayContents(Space * space, const char * displayStr,
				Int32 clauseNum, char * constsArea)
{
  return displayContents(space, displayStr, clauseNum, constsArea, 0);
}

void ex_clause::displayContents(Space * space, const char * displayStr,
				Int32 clauseNum, char * constsArea, 
                                UInt32 clauseFlags,
                                Int16 instruction,
                                const char * instrText)
{
  char buf[100];
  if (displayStr)
    {
      str_sprintf(buf, "  Clause #%d: %s", clauseNum, displayStr); 
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }

  str_sprintf(buf, "    OperatorTypeEnum = %s(%d), NumOperands = %d",
	  getOperTypeEnumAsString(operType_), operType_, numOperands_);
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  str_sprintf(buf, "    ex_clause::flags_ = %x ",flags_ );
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  if (displayStr)
    {
      str_sprintf(buf, "    %s::flags_ = %x ", displayStr, clauseFlags);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }

  if (noPCodeAvailable())
    {
      str_sprintf(buf, "    PCODE  = not supported ");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
  else
    {
      str_sprintf(buf, "    PCODE  = supported ");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }

  if (instruction >= 0)
    {
      if (instrText)
        str_sprintf(buf, "    instruction: %s(%d), instrArrayIndex_: %d", 
                    instrText, instruction, instrArrayIndex_);
      else
        str_sprintf(buf, "    instruction: UNKNOWN(%d), instrArrayIndex_: %d", 
                    instruction, instrArrayIndex_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
  
  if (numOperands_ == 0)
    return;

  if (numOperands_ > 0)
  {
   NABoolean showplan = getOperand(0)->showplan();

   for (Int32 i = 0; i < numOperands_; i++)
   {
   getOperand(i)->displayContents(space, i,
                                   constsArea,
                                   (showplan 
                                   ? getOperand(i+numOperands_) 
                                   : NULL));
   str_sprintf(buf, "\n");
   space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
  }
 }
}

/////////////////////////////////////////////////////////

// Derived clauses
/////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////
// class ex_arith_clause
///////////////////////////////////////////////////////////
ex_arith_clause::ex_arith_clause(OperatorTypeEnum oper_type,
				 Attributes ** attr,
				 Space * space,
				 short arithRoundingMode,
				 NABoolean divToDownscale) 
     : ex_clause (ex_clause::ARITH_TYPE, oper_type, 
                  (oper_type == ITM_NEGATE ? 2 : 3), attr, space),
       flags_(0)
{
  setAugmentedAssignOperation(TRUE);
  arithRoundingMode_ = (char)arithRoundingMode;


  if (divToDownscale)
    setDivToDownscale(TRUE);

  if (attr)
    setInstruction();
}


ExRegexpClauseChar::ExRegexpClauseChar(OperatorTypeEnum oper_type, 
			    short num_operands,
			    Attributes ** attr,
			    Space * space)
: ExRegexpClauseBase(oper_type, num_operands,attr,space)
{
}

ex_arith_clause::ex_arith_clause(clause_type type,
                                 OperatorTypeEnum oper_type,
                                 Attributes ** attr,
                                 Space * space)
                                 : ex_clause(type, oper_type, 3, attr, space),
				   arithRoundingMode_(0),
				   flags_(0)
 
{
  setAugmentedAssignOperation(TRUE);
  setInstruction(); 
}

ex_arith_sum_clause::ex_arith_sum_clause(OperatorTypeEnum oper_type,
					 Attributes ** attr,
					 Space * space)
           : ex_arith_clause (ex_clause::ARITH_SUM_TYPE, oper_type, attr, 
			      space)
{
}

ex_arith_count_clause::ex_arith_count_clause(OperatorTypeEnum oper_type,
					     Attributes ** attr,
					     Space * space)
           : ex_arith_clause (ex_clause::ARITH_COUNT_TYPE, oper_type, attr, 
			      space)
{
}

///////////////////////////////////////////////////////////
// class ex_comp_clause
///////////////////////////////////////////////////////////
ex_comp_clause::ex_comp_clause(OperatorTypeEnum oper_type,
			       Attributes ** attr,
			       Space * space,
			       ULng32 flags)
     : ex_clause (ex_clause::COMP_TYPE, oper_type, 3, attr, space),
       flags_(0),
       rollupColumnNum_(-1)
{
  if(flags) 
    setSpecialNulls();
  setInstruction();
}
 
///////////////////////////////////////////////////////////
// class ex_conv_clause
///////////////////////////////////////////////////////////
ex_conv_clause::ex_conv_clause(OperatorTypeEnum oper_type,
			       Attributes ** attr,
			       Space * space,
			       short num_operands, NABoolean checkTruncErr,
                               NABoolean reverseDataErrorConversionFlag,
                               NABoolean noStringTruncWarnings,
                               NABoolean convertToNullWhenErrorFlag)
     : ex_clause (ex_clause::CONV_TYPE, oper_type, num_operands, attr, space),
       lastVOAoffset_(0),
       lastVcIndicatorLength_(0),
       lastNullIndicatorLength_(0),
       computedLength_(0),
       alignment_(0),
       flags_(0)
{
  if (oper_type == ITM_NARROW)
    // Narrow reports conversion errors via a variable instead of a
    // SQL diagnostic -- so in this case we want to handle NULLs ourselves 
    setProcessNulls();   
  if (checkTruncErr)
    setCheckTruncationFlag();

  if (reverseDataErrorConversionFlag)
    flags_ |= REVERSE_DATA_ERROR_CONVERSION_FLAG;

  if (noStringTruncWarnings)
    setNoTruncationWarningsFlag();
  
  if (convertToNullWhenErrorFlag)
    flags_ |= CONV_TO_NULL_WHEN_ERROR;

  setInstruction(); 
}

///////////////////////////////////////////////////////////
// class ex_inout_clause
///////////////////////////////////////////////////////////
ex_inout_clause::ex_inout_clause(OperatorTypeEnum oper_type, 
				 Attributes ** attr, Space * space)
     : ex_clause (ex_clause::INOUT_TYPE, oper_type, 1, attr, space)
{
  name = 0;
  heading_ = 0;
  //  convHVClause_ = 0;
  flags_ = 0;
}

///////////////////////////////////////////////////////////
// class bool_result_clause
///////////////////////////////////////////////////////////
bool_result_clause::bool_result_clause(OperatorTypeEnum oper_type, 
				       Attributes ** attr,
				       Space * space)
     : ex_clause (ex_clause::BOOL_RESULT_TYPE, oper_type, 1, attr, space)
{
}

///////////////////////////////////////////////////////////
// class ex_branch_clause
///////////////////////////////////////////////////////////
ex_branch_clause::ex_branch_clause(OperatorTypeEnum oper_type,
				   Attributes ** attr, Space * space)
     : ex_clause (ex_clause::BRANCH_TYPE, oper_type, 2, attr, space),
       saved_next_clause(NULL),
       branch_clause(NULL)
{
}

ex_branch_clause::ex_branch_clause(OperatorTypeEnum oper_type,
				   Space * space)
     : ex_clause (ex_clause::BRANCH_TYPE, oper_type, 0, NULL, space),
       saved_next_clause(NULL),
       branch_clause(NULL)
{
}

///////////////////////////////////////////////////////////
// class ex_bool_clause
///////////////////////////////////////////////////////////
ex_bool_clause::ex_bool_clause(OperatorTypeEnum oper_type,
			       Attributes ** attr,
			       Space * space)
     : ex_clause (ex_clause::BOOL_TYPE, oper_type, 3, attr, space)
{
}

///////////////////////////////////////////////////////////
// class ex_unlogic_clause
///////////////////////////////////////////////////////////
ex_unlogic_clause::ex_unlogic_clause(OperatorTypeEnum oper_type, 
				     Attributes ** attr,
				     Space * space)
     : ex_clause (ex_clause::UN_LOGIC_TYPE, oper_type, 2, attr, space)
{
}

///////////////////////////////////////////////////////////
// class ex_aggregate_clause
///////////////////////////////////////////////////////////
ex_aggregate_clause::ex_aggregate_clause(OperatorTypeEnum oper_type,
					 short num_operands,
					 Attributes ** attr,
					 Space * space)
: ex_clause (ex_clause::AGGREGATE_TYPE, oper_type, num_operands, attr, space)
{
}

///////////////////////////////////////////////////////////
// class ex_noop_clause
///////////////////////////////////////////////////////////
ex_noop_clause::ex_noop_clause()
     : ex_clause (ex_clause::NOOP_TYPE, ITM_CONVERT, 0, 0, 0)
{
}

/////////////////////////////////////////////////////////
// class ex_function_clause
/////////////////////////////////////////////////////////
ex_function_clause::ex_function_clause(OperatorTypeEnum oper_type,
				       short num_operands,
				       Attributes ** attr,
				       Space * space)
     : ex_clause (ex_clause::FUNCTION_TYPE, oper_type, num_operands, attr,
		  space),
       origFunctionOperType_(oper_type)
{
  setDerivedFunction(FALSE);
}
  
/////////////////////////////////////////////////////////
// class ex_like_clause_char
/////////////////////////////////////////////////////////
ex_like_clause_char::ex_like_clause_char(OperatorTypeEnum oper_type,
                               short num_operands,
                               Attributes ** attr,
                               Space * space)
: ex_like_clause_base (oper_type, num_operands, attr, space)
{
}

ex_like_clause_doublebyte::ex_like_clause_doublebyte(OperatorTypeEnum oper_type,
                               short num_operands,
                               Attributes ** attr,
                               Space * space)
: ex_like_clause_base (oper_type, num_operands, attr, space)
{
}

/////////////////////////////////////////////////////////////
// Methods to display Contents
/////////////////////////////////////////////////////////////
void ex_aggr_one_row_clause::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  ex_clause::displayContents(space, "ex_aggr_one_row_clause", clauseNum, constsArea);
}

void ex_aggr_any_true_max_clause::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  ex_clause::displayContents(space, "ex_aggr_any_true_max_clause", clauseNum, constsArea);
}
void ex_aggr_min_max_clause::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  ex_clause::displayContents(space, "ex_aggr_min_max_clause", clauseNum, constsArea);
}

void ex_pivot_group_clause::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  ex_clause::displayContents(space, "ex_pivot_group_clause", clauseNum, constsArea);
}

void ExFunctionGrouping::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  ex_clause::displayContents(space, "ExFunctionGrouping", clauseNum, constsArea);

  char buf[100];
  str_sprintf(buf, "    rollupGroupIndex_ = %d\n",
              rollupGroupIndex_);
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
}

void ex_arith_clause::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  setInstruction();

  char buf[100];
  str_sprintf(buf, "  Clause #%d: ex_arith_clause", clauseNum);
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  if (arithRoundingMode_ != 0)
    {
      str_sprintf(buf, "    arithRoundingMode_ = %d, divToScale = %d",
		  (short)arithRoundingMode_, (getDivToDownscale() ? 1 : 0));
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }

  ex_clause::displayContents(space, (const char *)NULL, clauseNum, constsArea, 0,
                             ex_arith_clause::getInstruction(getInstrArrayIndex()),
                             ex_arith_clause::getInstructionStr(getInstrArrayIndex()));
}

void ex_arith_sum_clause::displayContents(Space * space, 
					  const char * /*displayStr*/, 
					  Int32 clauseNum, char * constsArea)
{
  setInstruction();
  ex_clause::displayContents(space, "ex_arith_sum_clause", clauseNum, constsArea);
}

void ex_arith_count_clause::displayContents(Space * space, 
					    const char * /*displayStr*/, 
					    Int32 clauseNum, char * constsArea)
{
  setInstruction();
  ex_clause::displayContents(space, "ex_arith_count_clause", clauseNum, constsArea);
}

void ex_bool_clause::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  ex_clause::displayContents(space, "ex_bool_clause", clauseNum, constsArea);
}

void bool_result_clause::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  ex_clause::displayContents(space, "bool_result_clause", clauseNum, constsArea);
}

void ex_branch_clause::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  char buf[100];
  str_sprintf(buf, "  Clause #%d: ex_branch_clause", clauseNum);
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  str_sprintf(buf, "    OperatorTypeEnum = %s(%d), NumOperands = %d",
          getOperTypeEnumAsString(getOperType()), getOperType(),
                                  getNumOperands());
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  str_sprintf(buf, "    flags_ = %x ", getAllFlags());
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  if (noPCodeAvailable())
    {
      str_sprintf(buf, "    PCODE  = not supported ");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
  else
    {
      str_sprintf(buf, "    PCODE  = supported ");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
  str_sprintf(buf, "    branch to = #%d ",branch_clause->clauseNum());
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  if (getNumOperands() == 0)
    return;

  if (getNumOperands() > 0)
  {
   NABoolean showplan = getOperand(0)->showplan();

   for (Int32 i = 0; i < getNumOperands(); i++)
   {
   getOperand(i)->displayContents(space, i,
                                   constsArea,
                                   (showplan
                                   ? getOperand(i+getNumOperands())
                                   : NULL));
   str_sprintf(buf, "\n");
   space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
   }
  }
}

void ex_comp_clause::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  setInstruction();

  char buf[100];
  str_sprintf(buf, "  Clause #%d: ex_comp_clause", clauseNum);
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  str_sprintf(buf, "    ex_comp_clause::rollupColumnNum_ = %d", rollupColumnNum_);
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  str_sprintf(buf, "    ex_comp_clause::flags_ = %x", flags_);
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  ex_clause::displayContents(space, (const char *)NULL, clauseNum, constsArea, 
                             0,
                             ex_comp_clause::getInstruction(getInstrArrayIndex()),                             
                             ex_comp_clause::getInstructionStr(getInstrArrayIndex()));

}

void ex_conv_clause::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  setInstruction();
  ex_clause::displayContents(space, "ex_conv_clause", clauseNum, constsArea, 
                             flags_,
                             ex_conv_clause::getInstruction(getInstrArrayIndex()),
                             ex_conv_clause::getInstructionStr(getInstrArrayIndex()));
}

void ex_function_clause::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  ex_clause::displayContents(space, "ex_function_clause", clauseNum, constsArea);
}
void ex_function_abs::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  ex_clause::displayContents(space, "ex_function_abs", clauseNum, constsArea);
}

void ExFunctionMath::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  ex_clause::displayContents(space, "ExFunctionMath", clauseNum, constsArea);
}

void ExFunctionBitOper::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  ex_clause::displayContents(space, "ExFunctionBitOper", clauseNum, constsArea);
}
void ex_inout_clause::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  ex_clause::displayContents(space, "ex_inout_clause", clauseNum, constsArea);
  //  cout << "Name  = " << getName() << endl;
}

void ex_noop_clause::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  ex_clause::displayContents(space, "ex_noop_clause", clauseNum, constsArea);
}

void ex_unlogic_clause::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  ex_clause::displayContents(space, "ex_unlogic_clause", clauseNum, constsArea);
}

void ex_like_clause_char::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  ex_clause::displayContents(space, "ex_like_clause_char", clauseNum, constsArea);
}

void ExRegexpClauseChar::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  ex_clause::displayContents(space, "ExRegexpClauseChar", clauseNum, constsArea);
}

void ex_like_clause_doublebyte::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  ex_clause::displayContents(space, "ex_like_clause_doublebyte", clauseNum, constsArea);
}

void ExFunctionHbaseTimestamp::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  char buf[100];
  str_sprintf(buf, "  Clause #%d: ExFunctionHbaseTimestamp", clauseNum);
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  str_sprintf(buf, "    colIndex_ = %d", colIndex_);
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  ex_clause::displayContents(space, (const char *)NULL, clauseNum, constsArea);
}

void ExFunctionHbaseVersion::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  char buf[100];
  str_sprintf(buf, "  Clause #%d: ExFunctionHbaseVersion", clauseNum);
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  str_sprintf(buf, "    colIndex_ = %d", colIndex_);
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  ex_clause::displayContents(space, (const char *)NULL, clauseNum, constsArea);
}

void ex_function_dateformat::displayContents(Space * space, const char * /*displayStr*/, Int32 clauseNum, char * constsArea)
{
  char buf[100];
  str_sprintf(buf, "  Clause #%d: ex_function_dateformat", clauseNum);
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  str_sprintf(buf, "    dateformat_ = %d", dateformat_);
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  ex_clause::displayContents(space, (const char *)NULL, clauseNum, constsArea);
}

// Function to compare two strings. 
Int32 charStringCompareWithPad(char* in_s1, Int32 length1, 
                                          char* in_s2, Int32 length2, 
                                          char padChar)
{
  unsigned char * s1 = (unsigned char *)in_s1;
  unsigned char * s2 = (unsigned char *)in_s2;

   Lng32 compare_len;
   Int32 compare_code;
   
   if (length1 > length2)
     compare_len = length2;
   else
     compare_len = length1;

   compare_code = str_cmp(in_s1, in_s2, compare_len);
   
   if ((compare_code == 0) && (length1 != length2))
     {
       if (length1 > length2)
         {
           Int32 j = compare_len;
           
           while ((j < length1) && (compare_code == 0))
             {
               if (s1[j] < padChar )
                 compare_code = -1;
               else
                 if (s1[j] > padChar )
                   compare_code = 1;
               j++;
             }
         }
       else
         {
           Int32 j = compare_len;
           
           while ((j < length2) && (compare_code == 0))
             {
               if (s2[j] < padChar )
                 compare_code = 1;
               else
                 if (s2[j] > padChar )
                   compare_code = -1;
               j++;
             }
         }
     }

   //return 0,1,-1 values, not the positive, 0, negative
   if (compare_code > 0)
     compare_code = 1;
   if (compare_code < 0)
     compare_code = -1;
   return compare_code;
}

Int32 wcharStringCompareWithPad(NAWchar* s1, Int32 length1, 
                                           NAWchar* s2, Int32 length2, 
                                           NAWchar space)
{
  Lng32 compare_len;
  Int32 compare_code;
	
  if (length1 > length2)
    compare_len = length2;
  else
    compare_len = length1;

  compare_code = wc_str_cmp(s1, s2, compare_len);
	
  if ((compare_code == 0) && (length1 != length2))
    {
      if (length1 > length2)
        {
          Int32 j = compare_len;
 	
          while ((j < length1) && (compare_code == 0))
            {
	      if (s1[j] < space )
	        compare_code = -1;
	      else
	        if (s1[j] > space )
	  	  compare_code = 1;
	      j++;
	    }
        }
	else
	{
	  Int32 j = compare_len;
		
	  while ((j < length2) && (compare_code == 0))
	    {
	      if (s2[j] < space )
	        compare_code = 1;
	      else
	        if (s2[j] > space )
	    	  compare_code = -1;
	      j++;
	    }
        }
    }
    //return 0,1,-1 values, not the positive, 0, negative
     if (compare_code > 0)
       compare_code = 1;
     if (compare_code < 0)
       compare_code = -1;
    return compare_code;
}



