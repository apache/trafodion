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
******************************************************************************
*
* File:         GenItemFunc.C
* Description:  Function expressions
*               
*               
* Created:      5/17/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

/////////////////////////////////////////////////////////////////////
//
// Contents:
//    
//   BuiltinFunction::codeGen()
//   Case::codeGen()
//   Cast::codeGen()
//   RaiseError::codeGen()
//   CompEncode::codeGen()
//   Hash::codeGen()
//   HashComb::codeGen()
//   HashDistPartHash::codeGen()
//   HashDistPartHashComb::codeGen()
//   HiveHash::codeGen()
//   HiveHashComb::codeGen()
//   Trim::codeGen
//   Tup::codeGen
//   UDFunction::codeGen()
//
//////////////////////////////////////////////////////////////////////

#include "Generator.h"
#include "GenExpGenerator.h"
#include "exp_function.h"
#include "ExpLOB.h"
#include "exp_datetime.h"
#include "exp_math_func.h"
#include "CharType.h"
#include "NumericType.h"
#include "RelMisc.h"
#include "ItemFuncUDF.h"

short BuiltinFunction::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  Space * space = generator->getSpace();
  
  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;
  
  ex_clause * function_clause = 0;
  
  switch (getOperatorType())
    {
    case ITM_CONVERTTOBITS:
      {
	function_clause =
	  new(generator->getSpace()) ExFunctionBitOper(getOperatorType(), 
						       (1+getArity()),
						       attr, space);
      }
    break;
    case ITM_REGEXP:
      {
        function_clause = 
          new(generator->getSpace()) ExRegexpClauseChar(getOperatorType(), (1+getArity()),
                                                       attr, space);
      }
    break; 
    case ITM_LIKE:
      {

        const CharType& likeSource = 
           (CharType&)((child(0)->getValueId()).getType());

        const CharType& likePattern =
           (CharType&)((child(1)->getValueId()).getType());

        switch ( likeSource.getCharSet() )
        {
          case CharInfo::ISO88591:
          case CharInfo::UTF8:
          case CharInfo::BINARY:
          // case CharInfo::SJIS:  // Uncomment this if we ever support SJIS
           function_clause =
               new(generator->getSpace()) ex_like_clause_char(getOperatorType()
                                                   ,1+getArity()
                                                   ,attr
                                                   ,space
                                                   );

           // Consider generating pcode for this.  Certain criteria must be
           // met, however.  If pattern string is a constant, with no escape
           // characters, and we're dealing with a fixed-length string (should
           // be see constants are exploded), then we met the minimum criteria.
           // Pcode generation for LIKE will do other checks.

           if ((child(1)->castToItemExpr()->getOperatorType() == ITM_CONSTANT)&&
               (likePattern.getFSDatatype() == REC_BYTE_F_ASCII) &&
               (getArity() <= 2))
           {
             char* pat = (char*)
               ((ConstValue*)(child(1)->castToItemExpr()))->getConstValue();

             // Pass in constant string for analysis during pcode generation.
             ((ex_like_clause_base*)(function_clause))->setPatternStr(pat);
           }
           else
             function_clause->setNoPCodeAvailable(TRUE);

           break;
          case CharInfo::UCS2:
           function_clause = new(generator->getSpace()) 
                                   ex_like_clause_doublebyte(ITM_LIKE_DOUBLEBYTE
                                                   ,1+getArity()
                                                   ,attr
                                                   ,space
                                                   );
           break;
          default:
	    GenAssert(0, "unknown value for bytes-per-char.");
        }
      }

      break;
      
    case ITM_SLEEP:
      {
	function_clause =
	  new(generator->getSpace()) ex_function_sleep(getOperatorType(),(1+getArity()), 
							 attr, space);
      }
      
      break;
    case ITM_UNIX_TIMESTAMP:
      {
	function_clause =
	  new(generator->getSpace()) ex_function_unixtime(getOperatorType(), (1+getArity()),
							 attr, space);
      }
      
      break;
    case ITM_CURRENT_TIMESTAMP:
    case ITM_CURRENT_TIMESTAMP_RUNNING:
      {
	function_clause =
	  new(generator->getSpace()) ex_function_current(getOperatorType(), 
							 attr, space);
      }
      
      break;

    // MV,
    case ITM_CURRENTEPOCH:
    case ITM_VSBBROWTYPE:
    case ITM_VSBBROWCOUNT:
      {
	function_clause =
	  new(generator->getSpace()) ExFunctionGenericUpdateOutput(getOperatorType(), 
							 attr, space);
      }      
      break;

	// ++ Triggers -
    case ITM_INTERNALTIMESTAMP:
      {
	function_clause =
	  new(generator->getSpace()) ExFunctionInternalTimestamp(getOperatorType(), 
							 attr, space);
      }
      
      break;

    case ITM_UNIQUE_EXECUTE_ID:
      {
	function_clause =
	  new(generator->getSpace()) ex_function_unique_execute_id(getOperatorType(), 
							 attr, space);
      }
      
      break;

   case ITM_GET_TRIGGERS_STATUS:
      {
	function_clause =
	  new(generator->getSpace()) ex_function_get_triggers_status(getOperatorType(), 
							 attr, space);
      }
      
      break;

   case ITM_GET_BIT_VALUE_AT:
      {
	function_clause =
	  new(generator->getSpace()) ex_function_get_bit_value_at(getOperatorType(), 
							 attr, space);
      }
      
      break;
      // -- Triggers -

   case ITM_IS_BITWISE_AND_TRUE:
      {
	function_clause =
	  new(generator->getSpace()) ex_function_is_bitwise_and_true(getOperatorType(), 
							 attr, space);
      }
      
      break;

   case ITM_SHA1:
      {
         function_clause =
           new(generator->getSpace()) ExFunctionSha(getOperatorType(),
                                                         attr, space);
      }
     
      break;

   case ITM_SHA2_256:
   case ITM_SHA2_224:
   case ITM_SHA2_384:
   case ITM_SHA2_512:
      {
          Lng32 mode = 0;
          switch (getOperatorType()) {
          case ITM_SHA2_256:
              mode = 256;
              break;

          case ITM_SHA2_224:
              mode = 224;
              break;

          case ITM_SHA2_384:
              mode = 384;
              break;

          case ITM_SHA2_512:
              mode = 512;
              break;
          }
         function_clause =
           new(generator->getSpace()) ExFunctionSha2(getOperatorType(),
                                                         attr, space, mode);
      }
     
      break;

   case ITM_MD5:
      {
         function_clause =
           new(generator->getSpace()) ExFunctionMd5(getOperatorType(),
                                                         attr, space);
      }
     
      break;

   case ITM_CRC32:
      {
         function_clause =
           new(generator->getSpace()) ExFunctionCrc32(getOperatorType(),
                                                         attr, space);
      }
     
      break;

   case ITM_ISIPV4:
   case ITM_ISIPV6:
      {
	function_clause =
	  new(generator->getSpace()) ExFunctionIsIP(getOperatorType(), 
							 attr, space);
      }
      
      break;
    case ITM_INET_ATON:
      {
        function_clause =
          new(generator->getSpace()) ExFunctionInetAton(getOperatorType(),
                                                         attr, space);
      }

      break;

    case ITM_INET_NTOA:
      {
        function_clause =
          new(generator->getSpace()) ExFunctionInetNtoa(getOperatorType(),
                                                         attr, space);
      }

      break;

    // -- MV
    case ITM_CONCAT:
      {
	function_clause =
	  new(generator->getSpace()) ex_function_concat(getOperatorType(),
							attr, space);
      }
      
      break;
      
    case ITM_SUBSTR:
      {

	if (getValueId().getType().isLob())
	  {
	   function_clause = new(generator->getSpace()) 
		ExpLOBfuncSubstring(getOperatorType(), 
				    (1+getArity()), attr, space);
	   break;
	  }
	

        const CharType& substringOperandType = 
           (CharType&)((child(0)->getValueId()).getType());

        switch ( substringOperandType.getCharSet() )
        {
          case CharInfo::ISO88591:
          case CharInfo::UTF8:
          case CharInfo::BINARY:
          // case CharInfo::SJIS:  // Uncomment this if we ever support SJIS
	   function_clause = new(generator->getSpace()) 
		ex_function_substring(getOperatorType(), 
				      (1+getArity()), attr, space);
            break;
     
          case CharInfo::UCS2:
	   function_clause = new(generator->getSpace()) 
		ex_function_substring_doublebyte(ITM_SUBSTR_DOUBLEBYTE,
				(1+getArity()), attr, space
					     );
            break;

          default:
	    GenAssert(0, "unknown value for bytes-per-char.");
        }
      }
      
      break;
      
    case ITM_LOWER:
      {
        const CharType& lowerOperandType = 
		(CharType&)((child(0)->getValueId()).getType());

        if ( lowerOperandType.getCharSet() != CharInfo::UNICODE )
	  function_clause = new(generator->getSpace()) 
		ex_function_lower(getOperatorType(), attr, space);
        else
	  function_clause = new(generator->getSpace()) 
	   ex_function_lower_unicode(ITM_LOWER_UNICODE, attr, space);
      }
      
      break;
      
    case ITM_UPPER:
      {
        const CharType& upperOperandType = 
           (CharType&)((child(0)->getValueId()).getType());

        if ( upperOperandType.getCharSet() != CharInfo::UNICODE )
	   function_clause = new(generator->getSpace()) 
		ex_function_upper(ITM_UPPER, attr, space);
        else
	    function_clause = new(generator->getSpace()) 
		ex_function_upper_unicode(ITM_UPPER_UNICODE, attr, space);
      }
      
      break;
      
    case ITM_CHAR_LENGTH:
      {

        const CharType& charLenOperandType = 
		(CharType&)(child(0)->getValueId().getType());

        switch ( charLenOperandType.getCharSet() )
        {
          case CharInfo::ISO88591:
          case CharInfo::UTF8:
          case CharInfo::BINARY:
          // case CharInfo::SJIS: // Uncomment this if we ever support SJIS
	    function_clause = new(generator->getSpace()) 
		ex_function_char_length(ITM_CHAR_LENGTH, attr, space);
            break;
          case CharInfo::UCS2:
	    function_clause = new(generator->getSpace()) 
		ex_function_char_length_doublebyte(ITM_CHAR_LENGTH_DOUBLEBYTE,
					        attr, space);
            break;
          default:
	    GenAssert(0, "unknown value for bytes-per-char.");
        }
      }
      
      break;
      
    case ITM_OCTET_LENGTH:
      {
	function_clause =
	  new(generator->getSpace()) ex_function_oct_length(getOperatorType(),
							    attr, space);
      }
      
      break;
      
    case ITM_POSITION:
      {
        const CharType& patternOperandType = 
		(CharType&)(child(0)->getValueId().getType());

        switch ( patternOperandType.getCharSet() )
        {
          case CharInfo::ISO88591:
          case CharInfo::UTF8:
          case CharInfo::BINARY:
          // case CharInfo::SJIS: // Uncomment this if we ever support SJIS
	   function_clause = new(generator->getSpace()) 
             ex_function_position(ITM_POSITION, attr, space,
                                  (getArity() + 1));

	    ((ex_function_position *)function_clause)->setCollation(((PositionFunc*)this)->getCollation());

           break;

          case CharInfo::UCS2:
	   function_clause = new(generator->getSpace()) 
             ex_function_position_doublebyte(ITM_POSITION_DOUBLEBYTE, attr, space,
                                             (getArity() + 1));

           break;

           default:
	    GenAssert(0, "unknown value for bytes-per-char.");
         }
      }
      
      break;
      
    case ITM_REPLACE:
      {
	function_clause =
	  new(generator->getSpace()) ExFunctionReplace(getOperatorType(),
						       attr, space);

	const NAType &type1 = 
	  child(0)->castToItemExpr()->getValueId().getType();
	const NAType &type2 = 
	  child(1)->castToItemExpr()->getValueId().getType();
	const NAType &type3 = 
	  child(2)->castToItemExpr()->getValueId().getType();

	if ((type1.getTypeQualifier() == NA_CHARACTER_TYPE) &&
	    (type2.getTypeQualifier() == NA_CHARACTER_TYPE))
	  {
	    const CharType &cType1 = (CharType&)type1;
	    const CharType &cType2 = (CharType&)type2;
	    const CharType &cType3 = (CharType&)type3;

	    CharInfo::Collation coll1= cType1.getCollation();
    	    CharInfo::Collation coll2= cType2.getCollation();
    	    CharInfo::Collation coll3= cType3.getCollation();

	    CMPASSERT(coll1 == coll2 && coll1 == coll3);
	    if (CollationInfo::isSystemCollation(coll1))
            {
	      ((ExFunctionReplace*)function_clause)->setCollation(coll1);

	      Lng32 len = 0;
	      ItemExpr * tmpEncode;
	      const NAType * typ;
	      const CharType * ctyp;
	      
	      tmpEncode =  
		  new(generator->wHeap()) 
		  CompEncode(child(0),FALSE, -1, CollationInfo::Search);

	      typ= tmpEncode->synthesizeType();
	      ctyp = (CharType *)typ;
	      len = ctyp->getNominalSize();

	      ((ExFunctionReplace*)function_clause)->setArgEncodedLen((Int16)len, 0);

	      tmpEncode = 
		  new(generator->wHeap()) 
		  CompEncode(child(1),FALSE, -1, CollationInfo::Search);
	      
	      typ = tmpEncode->synthesizeType();
	      ctyp = (CharType *)typ;
	      len = ctyp->getNominalSize();

	      ((ExFunctionReplace*)function_clause)->setArgEncodedLen((Int16)len, 1);

	    }
	    else
	    {
	      NABoolean doCIcomp = 
		((cType1.isCaseinsensitive()) && (cType2.isCaseinsensitive()));
  	    
	      if ((doCIcomp) && (NOT cType1.isUpshifted()))
		((ExFunctionReplace*)function_clause)->
		  setCaseInsensitiveOperation(TRUE);
	    }
	  }
      }
    break;

    case ITM_REPEAT:
      {
	function_clause =
	  new(generator->getSpace()) ExFunctionRepeat(getOperatorType(),
						       attr, space);
      }
    break;

    case ITM_RETURN_TRUE:
    case ITM_RETURN_FALSE:
    case ITM_RETURN_NULL:
      {
	function_clause =
	  new(generator->getSpace()) ex_function_bool(getOperatorType(),
						      attr, space);
      }
      break;
      
    case ITM_INVERSE:
      {
	MapInfo * inv_map_info 
	  = generator->getMapInfo(getValueId());
	
 	MapInfo * child_map_info 
	  = generator->getMapInfo(child(0)->castToItemExpr()->getValueId());
	
	(inv_map_info->getAttr())->copyLocationAttrs(child_map_info->getAttr());
      }
      break;
      
    case ITM_CONVERTTIMESTAMP:
      {
	function_clause =
	  new(generator->getSpace()) ex_function_converttimestamp(getOperatorType(),
                                                                  attr, space);
      }
      break;
      
    case ITM_DAYOFWEEK:
      {
	function_clause =
	  new(generator->getSpace()) ex_function_dayofweek(getOperatorType(),
                                                           attr, space);
      }
      break;
      
    case ITM_JULIANTIMESTAMP:
      {
	function_clause =
	  new(generator->getSpace()) ex_function_juliantimestamp(getOperatorType(),
                                                                 attr, space);
      }
      break;

    case ITM_EXEC_COUNT:
      {
	function_clause =
	  new(generator->getSpace()) ex_function_exec_count(getOperatorType(),
							    attr, space);
      }
      break;

    case ITM_CURR_TRANSID:
      {
	function_clause =
	  new(generator->getSpace()) ex_function_curr_transid(getOperatorType(),
							      attr, space);
	GenAssert(0, "eval method is missing for curr_transid function");
      }
      break;

    case ITM_USER:
    case ITM_USERID:
    case ITM_AUTHNAME:
    case ITM_AUTHTYPE:
      {
	function_clause =
	  new(generator->getSpace()) ex_function_user(getOperatorType(),
						      attr, space);
      }
      break;
      
    case ITM_NULLIFZERO:
      {
	function_clause =
	  new(generator->getSpace()) ex_function_nullifzero(getOperatorType(),
						      attr, space);
      }
      break;
      
    case ITM_NVL:
      {
	function_clause =
	  new(generator->getSpace()) ex_function_nvl(getOperatorType(),
						     attr, space);
      }
      break;
    case ITM_JSONOBJECTFIELDTEXT:
    {
	  function_clause =
	    new(generator->getSpace()) ex_function_json_object_field_text(getOperatorType(),
						     attr, space);
    }
    break;
      
    case ITM_QUERYID_EXTRACT:
      {
	function_clause =
	  new(generator->getSpace()) 
	  ex_function_queryid_extract(getOperatorType(),
				      attr, space);
      }
      break;

    case ITM_TOKENSTR:
      {
	function_clause =
	  new(generator->getSpace()) ExFunctionTokenStr(getOperatorType(), 
							attr, space);
      }
    break;

    case ITM_REVERSE:
      {
	function_clause =
	  new(generator->getSpace()) ExFunctionReverseStr(getOperatorType(), 
                                                          attr, space);
      }
    break;

    case ITM_UNIQUE_ID:
    case ITM_UNIQUE_ID_SYS_GUID:
    case ITM_UNIQUE_SHORT_ID:
      {
	function_clause =
	  new(generator->getSpace()) ExFunctionUniqueId(getOperatorType(),
							attr,
							space);
      }
    break;
 
    case ITM_ROWNUM:
      {
	function_clause =
	  new(generator->getSpace()) ExFunctionRowNum(getOperatorType(),
							attr,
							space);
      }
    break;

    case ITM_SOUNDEX:
    {
        function_clause =
            new(generator->getSpace()) ExFunctionSoundex(getOperatorType(),
                    attr,
                    space);
    }
    break;

    case ITM_AES_ENCRYPT:
    {
      function_clause =
        new(generator->getSpace()) ExFunctionAESEncrypt(getOperatorType(),
                                                        attr,
                                                        space,
                                                        getArity(),
                                                        CmpCommon::getDefaultNumeric(BLOCK_ENCRYPTION_MODE));
      break;
    }

    case ITM_AES_DECRYPT:
    {
      function_clause =
        new(generator->getSpace()) ExFunctionAESDecrypt(getOperatorType(),
                                                        attr,
                                                        space,
                                                        getArity(),
                                                        CmpCommon::getDefaultNumeric(BLOCK_ENCRYPTION_MODE));
      break;
    }

    case ITM_ENCODE_BASE64:
    case ITM_DECODE_BASE64:
      {
        function_clause =
          new(generator->getSpace()) ExFunctionBase64EncDec
          (getOperatorType(),
           attr,
           space,
           (getOperatorType() == ITM_ENCODE_BASE64 ? TRUE : FALSE));
      }
      break;

    case ITM_SPLIT_PART:
    {
      function_clause = 
            new (generator->getSpace()) ex_function_split_part(getOperatorType(), attr, space);
      break;
    }

    default:
      break;
    }
  
  if (function_clause)
    generator->getExpGenerator()->linkClause(this, function_clause);
  
  return 0;
}

short Abs::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  Space * space = generator->getSpace();
  
  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ex_clause * function_clause =
    new(generator->getSpace()) ex_function_abs(getOperatorType(), 
					       attr, space);

  generator->getExpGenerator()->linkClause(this, function_clause);
 
  return 0;
}

short CharFunc::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  Space * space = generator->getSpace();
  
  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ex_clause * function_clause =
    new(generator->getSpace()) ExFunctionChar(getOperatorType(), 
					       attr, space);

  generator->getExpGenerator()->linkClause(this, function_clause);
 
  return 0;
}

short CodeVal::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  Space * space = generator->getSpace();
  
  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ex_clause * function_clause =
    new(generator->getSpace()) ExFunctionAscii(getOperatorType(), 
					       attr, space);

  generator->getExpGenerator()->linkClause(this, function_clause);
 
  return 0;
}

short ConvertHex::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  Space * space = generator->getSpace();
  
  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ex_clause * function_clause =
    new(generator->getSpace()) ExFunctionConvertHex(getOperatorType(), 
						    attr, space);

  generator->getExpGenerator()->linkClause(this, function_clause);
 
  return 0;
}

short AggrMinMax::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  Space * space = generator->getSpace();
  
  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;
  
  ex_clause * function_clause = 
    new(generator->getSpace()) ex_aggr_min_max_clause(getOperatorType(),
						      (short) (1+getArity()),
						      attr, generator->getSpace());
  
  generator->getExpGenerator()->linkClause(this, function_clause);
  
  return 0;
}

short AggrGrouping::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  Space * space = generator->getSpace();
  
  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;
  
  GenAssert((rollupGroupIndex_ >= 0), 
            "AggrGrouping::codeGen. rollupGroupIndex_ must be >= 0.");

  ex_clause * function_clause = 
    new(generator->getSpace()) ExFunctionGrouping(getOperatorType(),
                                                  1,
                                                  attr, 
                                                  rollupGroupIndex_,
                                                  generator->getSpace());
  
  generator->getExpGenerator()->linkClause(this, function_clause);
  
  return 0;
}

short AnsiUSERFunction::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  Space * space = generator->getSpace();
  
  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;
  
  ex_clause * function_clause = NULL;
  
  function_clause =
    new(generator->getSpace()) ex_function_ansi_user(getOperatorType(),
						     attr, space);

  if (function_clause)
    generator->getExpGenerator()->linkClause(this, function_clause);
  
  return 0;
}

short BoolResult::codeGen(Generator * generator)
{
  Attributes ** attr;

  child(0)->codeGen(generator);
  
  ItemExpr *rightMost = child(0);

  while (rightMost->getOperatorType() == ITM_ITEM_LIST)
    rightMost = rightMost->child(1);

  if (generator->getExpGenerator()->genItemExpr(this, &attr, 1, -1) == 1)
    return 0;
  
  attr[0]->copyLocationAttrs(
       generator->getMapInfo(rightMost->getValueId())->getAttr());
  
  ex_clause * function_clause =
    new(generator->getSpace()) bool_result_clause(getOperatorType(),
						  attr, generator->getSpace()
						  );
  
  // this is the last clause.
  function_clause->setLastClause();
  generator->getExpGenerator()->linkClause(this, function_clause);
 
  MapInfo * map_info = generator->addMapInfo(getValueId(), NULL);
  map_info->codeGenerated();

  return 0;
}

short BitOperFunc::codeGen(Generator * generator)
{
  Attributes ** attr;

  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ex_clause * function_clause = NULL;
  function_clause =
    new(generator->getSpace()) ExFunctionBitOper(getOperatorType(), 
						 (1+getArity()),
						 attr, generator->getSpace());
  
  generator->getExpGenerator()->linkClause(this, function_clause);

  return 0;
}

short IfThenElse::codeGen(Generator *generator) {
  return BuiltinFunction::codeGen(generator);

  //  Attributes **attr;
  //
  //  if (generator->getExpGenerator()->genItemExpr(this, &attr, 1, 0) == 1)
  //    return 0;
  //  
  //  return 0;
}

short RaiseError::codeGen(Generator *generator) {
  Attributes ** attr;
  
  if (generator->getExpGenerator()->genItemExpr(this, &attr, 1 + getArity(), -1) == 1)
    return 0;
  
  const char * constraintName = NULL;
  const char * tableName  = NULL;
  const char * optionalStr = NULL;

  if (!getConstraintName().isNull()) {
    constraintName = generator->getSpace()->AllocateAndCopyToAlignedSpace(
         getConstraintName(), 0);
  }
  
  if (!getTableName().isNull()) {
    tableName  = generator->getSpace()->AllocateAndCopyToAlignedSpace(
         getTableName(), 0);
  }
  
  if (!optionalStr_.isNull()) {
    optionalStr  = generator->getSpace()->allocateAndCopyToAlignedSpace(
         optionalStr_.data(), optionalStr_.length(), 0);
  }
  
  // make raiseError a field in this class. TBD.
  NABoolean raiseError = ((getSQLCODE() > 0) ? TRUE : FALSE);
  ex_clause * function_clause =
    new(generator->getSpace()) ExpRaiseErrorFunction (attr, 
						      generator->getSpace(),
						      (raiseError ? getSQLCODE() : - getSQLCODE()),
						      raiseError,
						      constraintName,
						      tableName,
                                                      (getArity()==1) ? TRUE : FALSE,  // -- Triggers
                                                      optionalStr);
  
  generator->getExpGenerator()->linkClause(this, function_clause);
  
  return 0;
}

short Case::codeGen(Generator * generator)
{
  ///////////////////////////////////////////////////////////////////
  // A case statement of the form:
  //   CASE CO
  //     WHEN C1 THEN R1
  //     WHEN C2 THEN R2
  //     ...
  //     ELSE EO 
  //   END
  //
  // gets transformed into:
  //  CASE
  //    WHEN CO = C1 THEN R1
  //    WHEN CO = C2 THEN R2
  //    ...
  //    ELSE EO
  //  END
  //
  // If the case operand (CO) is empty, then the previous
  // transformation is not done. Binder/Type propagator already
  // has checked to make sure that the Ci's are in the form of
  // a search condition.
  //
  // The case statement then further gets transformed into:
  // 
  //    ((CO = C1) AND (R1)) OR
  //    ((CO = C2) AND (R2)) OR
  //    ...                  OR
  //    (EO)
  // 
  // This transformation evaluates the 'when' part (CO = Ci) first
  // and if that is true, then the corresponding Ri is evaluated.
  // The logical statement then branches out using boolean short
  // circuit evaluation. The else part (EO) is evaluated if all
  // when conditions fail. The location of each Ri and EO is 'fixed' up
  // at code generation time so that they all point to the same
  // buffer.
  //
  // Later, move to preCodeGen.
  /////////////////////////////////////////////////////////////////////

  Attributes ** attr;

  // add the case node to the map table
  if (generator->getExpGenerator()->genItemExpr(this, &attr, 1, 0) == 1)
    return 0;

  Attributes * case_map_info_attr = generator->getMapInfo(getValueId())->getAttr();
  
  ItemExpr * rootNode = 0;

  // Set flag indicating that a case stmt was generated
  generator->getExpGenerator()->setCaseStmtGenerated(TRUE);

  if ( CmpCommon::getDefault(COMP_BOOL_91) == DF_OFF ) {
    // Do the codegen for the CASE Logic here in the codegen method,
    // rather than translating to an AND/OR tree and calling codegen
    // on that.  The advantage of this is that we can reuse the
    // MapTable for short circuit processing.  Using the AND/OR tree
    // method can lead to excessive memory use and procesing time due
    // to repeatedly creating and deleting map tables.

    // This code generates code in the same structure as the AND/OR
    // method.  It might be possible to generate more efficient code
    // given the global view of the CASE statement.  But for now, lets
    // just mimic the old method.

    ValueIdList whens;
    ValueIdList thens;

    // Collect all the WHEN and THEN clauses in lists.
    //
    ItemExpr *ifThenElse = child(0);
    for (;
         (ifThenElse != NULL) AND
           (ifThenElse->getOperatorType() == ITM_IF_THEN_ELSE);
         ifThenElse = ifThenElse->child(2))
      {
        ItemExpr * when_part = ifThenElse->child(0);
        ItemExpr * then_part = new(generator->wHeap()) Cast(ifThenElse->child(1),
                                                            &(getValueId().getType()));

        then_part->bindNode(generator->getBindWA());
      
        // set the buffer location of all the convert nodes that were added
        // in the last step to be the same as that of the case node. This
        // will make all result values to be moved to the same location.
        MapInfo * map_info 
          = generator->addMapInfo(then_part->getValueId(),
                                  case_map_info_attr);
        (map_info->getAttr())->copyLocationAttrs(case_map_info_attr);   
      
        ItemExpr *caseOperand = removeCaseOperand();

        if (caseOperand != NULL) 
          {
            when_part = new(generator->wHeap()) BiRelat(ITM_EQUAL, caseOperand, when_part);
          }
      
        // Then part must return TRUE after moving result to the proper location.
        // It must return TRUE so that the AND/OR processing will work properly.
        //
        then_part = new(generator->wHeap()) BoolVal(ITM_RETURN_TRUE, then_part);

        when_part->bindNode(generator->getBindWA());
        then_part->bindNode(generator->getBindWA());

        whens.insert(when_part->getValueId());
        thens.insert(then_part->getValueId());

    } // for


    // If there is an ELSE part, process it as one of the THEN clauses.
    //
    if (ifThenElse)
      {
        // Move the ELSE value into the same location as the THENs.
        //
        ItemExpr * else_part = new(generator->wHeap()) Cast(ifThenElse, 
                                                            &(getValueId().getType()));
      
        else_part->bindNode(generator->getBindWA());
        MapInfo * map_info = 
          generator->addMapInfo(else_part->getValueId(),
                                case_map_info_attr);
      
        (map_info->getAttr())->copyLocationAttrs(case_map_info_attr);
      
        else_part =  new(generator->wHeap()) BoolVal(ITM_RETURN_TRUE, else_part); 

        else_part->bindNode(generator->getBindWA());

        thens.insert(else_part->getValueId());
    }
    

    // Disable the Common Expression Detection Cache.
    //
    generator->getExpGenerator()->incrementLevel();

    // A MapTable used to do the Short Circuit processing of the WHEN
    // clauses.
    //
    MapTable *whenscMapTable = generator->appendAtEnd(NULL);

    // A MapTable used to do the Short Circuit processing of the THEN
    // clauses.
    //
    MapTable *thenscMapTable = generator->appendAtEnd(NULL);
    MapTable *mt = generator->unlinkLast();
    GenAssert(mt == thenscMapTable, "Unexpected map table");

    Space * space = generator->getSpace();
    
    // Attributes for AND/OR Branching and AND/OR BiLogic.
    //
    Attributes ** branch_attr = new(generator->wHeap()) Attributes * [2];
    Attributes ** allAttr = new(generator->wHeap()) Attributes * [3];
    Attributes ** prevAttr = new(generator->wHeap()) Attributes * [3];
    
    // Must remember previous Branch Clause.
    //
    ex_branch_clause *orBranchClause = NULL;

    // For all the WHEN/THEN pairs.
    //
    for(CollIndex i = 0; i < whens.entries(); i++) {

      ItemExpr *when_part = whens[i].getItemExpr();
      ItemExpr *then_part = thens[i].getItemExpr();
      
      // Create an AND node and bind it.  Just used to get a ValueId.
      // Will not call codegen in this AND node.
      //
      ItemExpr *andNode = new(generator->wHeap()) BiLogic(ITM_AND, when_part, then_part);
      andNode->bindNode(generator->getBindWA());

      // Create a temporary space for the result of the AND clause.
      MapInfo *mapInfoAndNode = 
        generator->getExpGenerator()->addTemporary(andNode->getValueId(), NULL);

      when_part->preCodeGen(generator);
      generator->getExpGenerator()->setClauseLinked(FALSE);
      when_part->codeGen(generator);

      // generate boolean short circuit code
      // Add the AND Branch clause to skip the THEN if the WHEN is FALSE.
      //
      allAttr[0] = mapInfoAndNode->getAttr();
      allAttr[1] = generator->getAttr(when_part);
      generator->getExpGenerator()->setClauseLinked(FALSE);

      branch_attr[0] = allAttr[0]->newCopy(generator->wHeap());
      branch_attr[0]->copyLocationAttrs(allAttr[0]);

      branch_attr[1] = allAttr[1]->newCopy(generator->wHeap());
      branch_attr[1]->copyLocationAttrs(allAttr[1]);

      branch_attr[0]->resetShowplan();

      ex_branch_clause  *andBranchClause 
        = new(space) ex_branch_clause(ITM_AND, branch_attr, space);
  
      generator->getExpGenerator()->linkClause(0, andBranchClause);
      
      // Use the Short Circuit Map Table when processing the THEN Clause.
      //
      generator->appendAtEnd(thenscMapTable);
      then_part->preCodeGen(generator);
      
      // Disable the Common Expression Detection Cache.
      //
      generator->getExpGenerator()->incrementLevel();
      generator->getExpGenerator()->setClauseLinked(FALSE);
      then_part->codeGen(generator);
      generator->getExpGenerator()->setClauseLinked(FALSE);
      generator->getExpGenerator()->decrementLevel();

      ItemExpr *rightMost = then_part;
      
      // Find the right most part of the THEN clause.
      // (Can this really be a list in the context of a CASE statement?)
      //
      while (rightMost->getOperatorType() == ITM_ITEM_LIST)
        rightMost = rightMost->child(1)->castToItemExpr();

      allAttr[2] = generator->
        getMapInfo(rightMost->getValueId())->getAttr();

      // Add the REAL Boolean AND Clause.  The AND between the WHEN
      // and the TRUE (of the THEN)
      //

      allAttr[0]->resetShowplan();

      ex_bool_clause * andClause =
        new(space) ex_bool_clause(ITM_AND, allAttr, space);

      generator->getExpGenerator()->linkClause(this, andClause);

      andBranchClause->set_branch_clause((ex_clause *)andClause);

      // Remove Short Circuit Map Table.  The code generated for the
      // THEN clause should not be available for processing the WHEN
      // clauses.
      mt = generator->unlinkLast();
      GenAssert(mt == thenscMapTable, "Unexpected map table");
      
      // Clear the codegen flag in the Short Circuit MapTable.  The
      // items codegened for one THEN clause should not be available
      // for the next THEN clause.
      //
      thenscMapTable->resetCodeGen();

      // Create the real Boolean OR Clause between this AND and the
      // previous AND or previous OR of ANDS.  The first time thru the
      // loop there is no previous so only do this after the first
      // iteration.
      if(i > 0) {
        // Add OR

        prevAttr[2] = mapInfoAndNode->getAttr();

        prevAttr[0]->resetShowplan();

        ex_bool_clause * orClause =
          new(space) ex_bool_clause(ITM_OR, prevAttr, space);

        generator->getExpGenerator()->linkClause(this, orClause);

        orBranchClause->set_branch_clause((ex_clause *)orClause);

      }

      // If there is another CLAUSE to process after this one (either
      // another WHEN/THEN pair or an ELSE clause), add the OR Branch
      // clause to branch around WHEN/THENs (ELSE) once one of the THENs has
      // been taken.
      //
      if(i < (thens.entries() - 1)) {
        
        // Add OR Branch Create a dummy OR node and bind it.  Just
        // used to get a ValueId.  Will not call codegen in this OR
        // node.  (note that the OR is not between the right values.
        // It is okay since it is just used as a placeholder.)
        //
        ItemExpr *orNode = new(generator->wHeap()) BiLogic(ITM_OR, andNode, 
                                                           andNode);
        orNode->bindNode(generator->getBindWA());

        // Create temp space for the result of the OR clause.
        MapInfo *mapInfoOrNode = 
          generator->getExpGenerator()->addTemporary(orNode->getValueId(), NULL);
      
        // On the first time, the OR is between two ANDs,  after that it is between
        // the previous OR (OR of ANDS) and the current AND.
        //
        if(i > 0) {
          prevAttr[1] = prevAttr[0];
        } else {
          prevAttr[1] = mapInfoAndNode->getAttr();
        }
        prevAttr[0] = mapInfoOrNode->getAttr();

        branch_attr[0] = prevAttr[0]->newCopy(generator->wHeap());
        branch_attr[0]->copyLocationAttrs(prevAttr[0]);

        branch_attr[1] = prevAttr[1]->newCopy(generator->wHeap());
        branch_attr[1]->copyLocationAttrs(prevAttr[1]);

        generator->getExpGenerator()->setClauseLinked(FALSE);
        
        branch_attr[0]->resetShowplan();
        orBranchClause  = new(space) ex_branch_clause(ITM_OR, branch_attr, space);

        generator->getExpGenerator()->linkClause(0, orBranchClause);
      }
    }

    // If there is an ELSE clause, generate code for the ELSE clause
    // and OR it into the rest.
    if(thens.entries() > whens.entries()) {
      ItemExpr *else_part = thens[whens.entries()].getItemExpr();

      // Use the Short Circuit Map Table for the ELSE clause.
      //
      generator->appendAtEnd(thenscMapTable);
      else_part->preCodeGen(generator);

      generator->getExpGenerator()->incrementLevel();
      generator->getExpGenerator()->setClauseLinked(FALSE);
      else_part->codeGen(generator);
      generator->getExpGenerator()->decrementLevel();
    
      prevAttr[2] = generator->getAttr(else_part);
      generator->getExpGenerator()->setClauseLinked(FALSE);

      prevAttr[0]->resetShowplan();

      ex_bool_clause * orClause =
        new(space) ex_bool_clause(ITM_OR, prevAttr, space);

      generator->getExpGenerator()->linkClause(this, orClause);
    
      orBranchClause->set_branch_clause((ex_clause *)orClause);
    
      mt = generator->unlinkLast();
      GenAssert(mt == thenscMapTable, "Unexpected map table");
      
      thenscMapTable->resetCodeGen();
    }
    generator->getExpGenerator()->setClauseLinked(FALSE);

    //    delete thenscMapTable;
    generator->appendAtEnd(thenscMapTable);
    generator->removeLast();

    mt = generator->getLastMapTable();
    GenAssert(mt == whenscMapTable, "Unexpected map table");
    generator->removeLast();

    generator->getExpGenerator()->decrementLevel();

    return 0;

  }
  
  char * str = new(generator->wHeap()) char[100];
  
  ItemExpr *ifThenElse = child(0);
  for (;
       (ifThenElse != NULL) AND
       (ifThenElse->getOperatorType() == ITM_IF_THEN_ELSE);
       ifThenElse = ifThenElse->child(2))
    {
      ItemExpr * when_part = ifThenElse->child(0);
      ItemExpr * then_part = new(generator->wHeap()) Cast(ifThenElse->child(1),
				      &(getValueId().getType()));
      
      then_part->bindNode(generator->getBindWA());
      
      // set the buffer location of all the convert nodes that were added
      // in the last step to be the same as that of the case node. This
      // will make all result values to be moved to the same location.
      MapInfo * map_info 
	= generator->addMapInfo(then_part->getValueId(),
					       case_map_info_attr);
      (map_info->getAttr())->copyLocationAttrs(case_map_info_attr);   
      
      ItemExpr *caseOperand = removeCaseOperand();
      if (caseOperand != NULL)
	strcpy(str, "@A1 = @A2 AND @B3");
      else
	strcpy(str, "@B2 AND @B3");
      
      ItemExpr * andNode 
	= generator->getExpGenerator()->createExprTree(str, 0, 3,
						       caseOperand,
						       when_part,
						       new(generator->wHeap())
						       BoolVal(ITM_RETURN_TRUE, then_part));
      
      if (rootNode)
	{
	  rootNode = generator->getExpGenerator()->createExprTree("@B1 OR @B2", 0, 2,
								  rootNode, andNode);
	}
      else
	{
	  rootNode = andNode;
	}
    } // for
  
  // now add the else part, if present
  if (ifThenElse)
    {
      ItemExpr * conv_node = new(generator->wHeap()) Cast(ifThenElse, 
				      &(getValueId().getType()));
      
      conv_node->bindNode(generator->getBindWA());
      MapInfo * map_info = 
	generator->addMapInfo(conv_node->getValueId(),
					   case_map_info_attr);
      
      (map_info->getAttr())->copyLocationAttrs(case_map_info_attr);
      
      rootNode = new(generator->wHeap()) BiLogic(ITM_OR, rootNode,
			     new(generator->wHeap()) BoolVal(ITM_RETURN_TRUE, 
					 conv_node)); 
    }

  NADELETEBASIC( str, generator->wHeap()) ;

  GenAssert(rootNode,"Case::codeGen: Expected ELSE condition"); // ported back from R2

  #ifdef _DEBUG
    if (getenv("GEN_FUNC_DEBUG"))
      {
        NAString unparsed;
	unparse(unparsed);
	cout << "(a)" << unparsed << endl;

	unparsed = "";
	rootNode->unparse(unparsed);
	cout << "(b)" << unparsed << endl;
      }
  #endif
  
  rootNode->bindNode(generator->getBindWA());

  GenAssert(rootNode,"Case::codeGen: Expected ELSE condition");
  // generate child
  rootNode->preCodeGen(generator);
  rootNode->codeGen(generator);
  
  return 0;
}

short Cast::codeGen(Generator * generator)
{

  Attributes ** attr;

  ExpGenerator * eg = generator->getExpGenerator();
  
  if (eg->genItemExpr(this, &attr, (1+getArity()), -1) == 1)
    return 0;

  // if temp space is needed for this operation, set it.
  if (attr[0]->isComplexType())
    {
      eg->addTempsLength(((ComplexType *)attr[0])->setTempSpaceInfo(getOperatorType(),
								    eg->getTempsLength()));
    }
  ex_conv_clause * conv_clause;
  if(attr[0]->getNullFlag())  //if target is nullable
    conv_clause = new(generator->getSpace()) ex_conv_clause(getOperatorType(), attr,
						    generator->getSpace(),
						    1 + getArity(), 
  						    checkTruncationError(),
                                                    reverseDataErrorConversionFlag_,
                                                    noStringTruncationWarnings(),
                                                    convertNullWhenError());
  else
    conv_clause = new(generator->getSpace()) ex_conv_clause(getOperatorType(), attr,
                                                    generator->getSpace(),
                                                    1 + getArity(), 
                                                    checkTruncationError(),
                                                    reverseDataErrorConversionFlag_,
                                                    noStringTruncationWarnings(),
                                                    FALSE);

  conv_clause->setTreatAllSpacesAsZero(treatAllSpacesAsZero());

  if ( CmpCommon::getDefault(MARIAQUEST_PROCESS) == DF_ON ) 
  {
    conv_clause->setAllowSignInInterval(TRUE);
    conv_clause->setNoDatetimeValidation(TRUE);
  }
  
  conv_clause->setAllowSignInInterval(allowSignInInterval());
  conv_clause->setSrcIsVarcharPtr(srcIsVarcharPtr());

  generator->getExpGenerator()->linkClause(this, conv_clause); 

  return 0;
}

short CastType::codeGen(Generator * generator)
{
  if (makeNullable_)
    return Cast::codeGen(generator);

  Attributes ** attr;

  ExpGenerator * eg = generator->getExpGenerator();
  
  if (eg->genItemExpr(this, &attr, (1+getArity()), -1) == 1)
    return 0;

  ExFunctionCastType * ct =
	  new(generator->getSpace()) ExFunctionCastType(getOperatorType(), attr,
							generator->getSpace());

  generator->getExpGenerator()->linkClause(this, ct); 

  return 0;
}

short CompEncode::codeGen(Generator * generator)
{
  Attributes ** attr;

  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;


  CharInfo::Collation collation = encodedCollation_;

  ex_function_encode * function_clause =
    new(generator->getSpace()) ex_function_encode(getOperatorType(),
						  attr, generator->getSpace(), 
						  collation,
						  descFlag_, 
						  collationType_
						  );
  
  // don't need data alignment for the operand since it is treated
  // as a byte stream to encode the value.
  function_clause->getOperand(1)->dontNeedDataAlignment();

  // encode as case insensitive if needed.
  if (caseinsensitiveEncode_)
    {
      function_clause->setCaseInsensitive(TRUE);
    }
  
  if (regularNullability_)
    {
      function_clause->setRegularNullability(TRUE);
    }

  if (isDecode())
    function_clause->setIsDecode(TRUE);

  generator->getExpGenerator()->linkClause(this, function_clause);

  if (CollationInfo::isSystemCollation(collation))
  //test implies child is of character type and has a system collation
  {
    short nPasses = CollationInfo::getCollationNPasses(collation);
    
    GenAssert(function_clause->getOperand(1)->getLength() * nPasses
            <= function_clause->getOperand(0)->getLength(),
            "Not enough storage allocated for encode");
  }
  else
  {
    // Runtime assumes that the target is large enough to hold the source.
    //
    if (NOT regularNullability_)
      {
	GenAssert(function_clause->getOperand(1)->getLength() +
                  function_clause->getOperand(1)->getNullIndicatorLength()
		  <= function_clause->getOperand(0)->getLength(),
		  "Not enough storage allocated for encode");
      }
  }

  return 0;
}
 
short CompDecode::codeGen(Generator * generator)
{
  Attributes ** attr;

  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  CharInfo::Collation collation = encodedCollation_;

  ex_function_encode * function_clause =
    new(generator->getSpace()) ex_function_encode(getOperatorType(),
						  attr, generator->getSpace(), 
						  collation,
						  descFlag_, 
						  collationType_
						  );
  
  // don't need data alignment for the operand since it is treated
  // as a byte stream to encode the value.
  function_clause->getOperand(1)->dontNeedDataAlignment();

  // encode as case insensitive if needed.
  if (caseinsensitiveEncode_)
    {
      function_clause->setCaseInsensitive(TRUE);
    }
  
  if (regularNullability_)
    {
      function_clause->setRegularNullability(TRUE);
    }

  function_clause->setIsDecode(TRUE);

  generator->getExpGenerator()->linkClause(this, function_clause);

  if (0) //CollationInfo::isSystemCollation(collation))
    //test implies child is of character type and has a system collation
    {
      short nPasses = CollationInfo::getCollationNPasses(collation);
      
      GenAssert(function_clause->getOperand(1)->getLength() * nPasses
		<= function_clause->getOperand(0)->getLength(),
		"Not enough storage allocated for decode");
    }
  else if (0)
    {
      // Runtime assumes that the target is large enough to hold the source.
      //
      if (NOT regularNullability_)
	{
	  GenAssert(function_clause->getOperand(1)->getStorageLength() 
		    <= function_clause->getOperand(0)->getLength(),
		    "Not enough storage allocated for decode");
	}
    }
  
  return 0;
}
 
short ExplodeVarchar::codeGen(Generator * generator)
{
  Attributes ** attr;

  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ex_clause * explode_clause =
    new(generator->getSpace()) ex_function_explode_varchar(getOperatorType(),
							   2,
							   attr, generator->getSpace(),
							   forInsert_);
  
  generator->getExpGenerator()->linkClause(this, explode_clause);

  return 0;
}

short Hash::codeGen(Generator * generator)
{
  Attributes ** attr;

  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ex_clause * function_clause =
    new(generator->getSpace()) ex_function_hash(getOperatorType(),
						attr, generator->getSpace());
  
  // don't need data alignment for the operand since it is treated
  // as a byte stream to hash the value.
  function_clause->getOperand(1)->dontNeedDataAlignment();

  generator->getExpGenerator()->linkClause(this, function_clause);

  return 0;
}

short HashComb::codeGen(Generator * generator)
{
  Attributes ** attr;

  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ex_clause * function_clause =
    new(generator->getSpace()) ExHashComb(getOperatorType(),
                                          attr, generator->getSpace());
  
  generator->getExpGenerator()->linkClause(this, function_clause);

  return 0;
}

short HiveHashComb::codeGen(Generator * generator)
{
  Attributes ** attr;

  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ex_clause * function_clause =
    new(generator->getSpace()) ExHiveHashComb(getOperatorType(),
                                          attr, generator->getSpace());

  generator->getExpGenerator()->linkClause(this, function_clause);

  return 0;
}


// --------------------------------------------------------------
// member functions for HashDistPartHash operator 
// Hash Function used by Hash Partitioning. This function cannot change
// once Hash Partitioning is released!  Defined for all data types,
// returns a 32 bit non-nullable hash value for the data item.
//--------------------------------------------------------------
short HashDistPartHash::codeGen(Generator * generator)
{
  Attributes ** attr;

  if (generator->getExpGenerator()->
      genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ex_clause * hashClause =
    new(generator->getSpace()) ExHDPHash(getOperatorType(),
                                         attr, generator->getSpace());
  
  // don't need data alignment for the operand since it is treated
  // as a byte stream to hash the value.
  hashClause->getOperand(1)->dontNeedDataAlignment();

  generator->getExpGenerator()->linkClause(this, hashClause);

  return 0;
}

// --------------------------------------------------------------
// member functions for HashDistPartHashComp operator 
// This function is used to combine two hash values to produce a new
// hash value. Used by Hash Partitioning. This function cannot change
// once Hash Partitioning is released!  Defined for all data types,
// returns a 32 bit non-nullable hash value for the data item.
// --------------------------------------------------------------
//
short HashDistPartHashComb::codeGen(Generator * generator)
{
  Attributes ** attr;

  if (generator->getExpGenerator()->
      genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ex_clause * hashCombClause =
    new(generator->getSpace()) ExHDPHashComb(getOperatorType(),
                                             attr, generator->getSpace());
  
  generator->getExpGenerator()->linkClause(this, hashCombClause);

  return 0;
}

short HiveHash::codeGen(Generator * generator)
{
  Attributes ** attr;

  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ex_clause * function_clause =
    new(generator->getSpace()) ex_function_hivehash(getOperatorType(),
                                                    attr, generator->getSpace());

  // don't need data alignment for the operand since it is treated
  // as a byte stream to hive hash the value.
  function_clause->getOperand(1)->dontNeedDataAlignment();

  generator->getExpGenerator()->linkClause(this, function_clause);

  return 0;
}

short PivotGroup::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  Space * space = generator->getSpace();
  
  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;
  
  ex_clause * function_clause = 
    new(generator->getSpace()) ex_pivot_group_clause(getOperatorType(),
                                                     (short) (1+getArity()),
                                                     attr, 
                                                     (char*)delim_.data(),
                                                     maxLen_,
                                                     orderBy_,
                                                     generator->getSpace());
  
  generator->getExpGenerator()->linkClause(this, function_clause);
  
  return 0;
}

short ReplaceNull::codeGen(Generator * generator)
{
  Attributes ** attr;

  if (generator->getExpGenerator()->
      genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ex_clause * function_clause =
    new(generator->getSpace()) 
    ex_function_replace_null(getOperatorType(), attr, generator->getSpace());
  
  generator->getExpGenerator()->linkClause(this, function_clause);
  return 0;
}

short MathFunc::codeGen(Generator * generator)
{
  Attributes ** attr;

  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;


  ex_clause * function_clause = NULL;
  function_clause =
    new(generator->getSpace()) ExFunctionMath(getOperatorType(), 
					      (1+getArity()),
					      attr, generator->getSpace());
  
  
  generator->getExpGenerator()->linkClause(this, function_clause);

  return 0;
}

short Modulus::codeGen(Generator * generator)
{
  Attributes ** attr;

  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ex_clause * function_clause =
    new(generator->getSpace()) ex_function_mod(getOperatorType(),
					       attr, generator->getSpace());
  
  generator->getExpGenerator()->linkClause(this, function_clause);

  return 0;
}


short Narrow::codeGen(Generator * generator)
{
  return Cast::codeGen(generator);
}


short Mask::codeGen(Generator * generator)
{
  Attributes ** attr;

  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ex_clause * function_clause =
    new(generator->getSpace()) ex_function_mask(getOperatorType(),
                                                attr, generator->getSpace());
  
  generator->getExpGenerator()->linkClause(this, function_clause);

  return 0;
}

short Shift::codeGen(Generator * generator)
{
  Attributes ** attr;

  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ex_clause * function_clause =
    new(generator->getSpace()) ExFunctionShift(getOperatorType(),
                                               attr, generator->getSpace());
  
  generator->getExpGenerator()->linkClause(this, function_clause);

  return 0;
}

short NoOp::codeGen(Generator * generator)
{
  Attributes ** attr;

  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ex_clause * function_clause =
    new(generator->getSpace()) ex_noop_clause();

  // this is the last clause.
  function_clause->setLastClause();
  generator->getExpGenerator()->linkClause(0, function_clause);
 
  return 0;
}

short Translate::codeGen(Generator * generator)
{
  Attributes ** attr;
  NABoolean unicodeToUnicode = FALSE;
  Int16 translateFlags = 0;
  
  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;
  
  Int32 convType = CONV_UNKNOWN;
  switch ( map_table_id_ ) {
     case ISO88591_TO_UNICODE:
	convType = CONV_ASCII_UNICODE_V;
	break;
     case SJIS_TO_UNICODE:
	convType = CONV_SJIS_F_UNICODE_V;
	break;
     case UNICODE_TO_SJIS:
	convType = CONV_UNICODE_F_SJIS_V;
	break;
     case SJIS_TO_UCS2:
	convType = CONV_SJIS_F_UCS2_V;
	break;
     case UTF8_TO_UCS2:
	convType = CONV_UTF8_F_UCS2_V;
        unicodeToUnicode = TRUE;
	break;
     case UCS2_TO_SJIS:
	convType = CONV_UCS2_F_SJIS_V;
	break;
     case UCS2_TO_UTF8:
	convType = CONV_UCS2_F_UTF8_V;
        unicodeToUnicode = TRUE;
	break;
     case GBK_TO_UTF8:
        convType = CONV_GBK_F_UTF8_V;
        break;
     case UNICODE_TO_ISO88591:
	convType = CONV_UNICODE_F_ASCII_V;
	break;
     case KANJI_MP_TO_ISO88591:
     case KSC5601_MP_TO_ISO88591:
	convType = CONV_ASCII_F_V;
	break;
  }

  if (CmpCommon::getDefault(TRANSLATE_ERROR) == DF_OFF ||
      (unicodeToUnicode &&
       CmpCommon::getDefault(TRANSLATE_ERROR_UNICODE_TO_UNICODE) == DF_OFF))
    translateFlags |= ex_function_translate::TRANSLATE_FLAG_ALLOW_INVALID_CODEPOINT;

  ex_clause * function_clause = 
	new(generator->getSpace()) ex_function_translate(
			         getOperatorType(),
				 attr, 
				 generator->getSpace(),
                                 convType,
                                 translateFlags
				);

  generator->getExpGenerator()->linkClause(this, function_clause);
  
  return 0;
}

short Trim::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;
  
  ex_clause * function_clause = 0;

  const CharType& trimSourceType = 
        (CharType&)((child(0)->getValueId()).getType());

  const CharType& trimCharT = 
        (CharType&)((child(1)->getValueId()).getType());

  switch ( trimSourceType.getCharSet() ) 
  {
    case CharInfo::ISO88591:
    case CharInfo::UTF8:
    case CharInfo::BINARY:
    // case CharInfo::SJIS: // Uncomment this if we ever support SJIS
      function_clause = new(generator->getSpace()) 
		ex_function_trim_char(ITM_TRIM,
				 attr, 
				 generator->getSpace(), getTrimMode()
				);
      ((ex_function_trim_char *)function_clause)->setCollation(getCollation());
      
      if (CollationInfo::isSystemCollation(getCollation()))
      {
        Lng32 len = 0;
	const CharType * ctyp ;
	      
	const NAType & typ1 = child(1)->getValueId().getType();
        ctyp = (CharType *)&typ1;
    
	len = CompEncode::getEncodedLength(getCollation(),
					  CollationInfo::Search,
					  ctyp->getNominalSize(),
					  ctyp->supportsSQLnull());

        ((ex_function_trim_char*)function_clause)->setSrcStrEncodedLength((Int16)len);

	const NAType & typ0= child(0)->getValueId().getType();
        ctyp = (CharType *)&typ0;

	len = CompEncode::getEncodedLength(getCollation(),
					CollationInfo::Search,
					ctyp->getNominalSize(),
					ctyp->supportsSQLnull());

        ((ex_function_trim_char*)function_clause)->setTrimCharEncodedLength((Int16)len);
      }

      // pcode is only supported if the trim char is a single byte literal with
      // length of 1 and value of ' '
      if (NOT ((child(0)->castToItemExpr()->getOperatorType() == ITM_CONSTANT) &&
	       (trimSourceType.getFSDatatype() == REC_BYTE_F_ASCII) &&
	       (trimSourceType.getNominalSize() == 1) &&
	       (str_cmp((char*)((ConstValue*)(child(0)->castToItemExpr()))->getConstValue(), " ", 1) == 0)))
	{
	  function_clause->setNoPCodeAvailable(TRUE);
	}
      break;

    case CharInfo::UCS2:
      function_clause = new(generator->getSpace()) 
		ex_function_trim_doublebyte(ITM_TRIM_DOUBLEBYTE,
				 attr, 
				 generator->getSpace(), getTrimMode()
				);
     break;

    default:
     GenAssert(0, "unknown value for bytes-per-char.");
  }

  generator->getExpGenerator()->linkClause(this, function_clause);
  
  return 0;
}

short DateFormat::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  if (generator->getExpGenerator()->genItemExpr(this,
                                                &attr,
                                                1 + getArity(),
                                                -1) == 1)
    return 0;

  ex_clause * function_clause = 
    new(generator->getSpace()) ex_function_dateformat(getOperatorType(),
                                                      attr, 
                                                      generator->getSpace(),
                                                      getExpDatetimeFormat());
  generator->getExpGenerator()->linkClause(this, function_clause);
  
  return 0;
}

short Extract::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  if (generator->getExpGenerator()->genItemExpr(this,
                                                &attr,
                                                1 + getArity(),
                                                -1) == 1)
    return 0;
  
  ex_clause * function_clause = 
    new(generator->getSpace()) ex_function_extract(getOperatorType(),
                                                   attr, 
                                                   generator->getSpace(),
                                                   getExtractField());
  generator->getExpGenerator()->linkClause(this, function_clause);
  
  return 0;
}

short RangeLookup::codeGen(Generator * generator)
{
  Attributes ** attr2;
  Attributes ** attr3;
  Lng32 arity = getArity();
  Lng32 keysLen = splitKeysLen();
  Attributes *splitKeyAttr;
  char *constKeyArray;
  ConstValue *constValSplitKeys;

  // generate code for child (allocates an array with 2 Attributes pointers)
  if (generator->getExpGenerator()->genItemExpr(this,
                                                &attr2,
                                                1 + arity,
                                                -1) == 1)
    return 0;

  // now allocate a third child for this clause, which is a constant
  // array of all the encoded start key values
  Int32 numAttrs = arity+2;
  Int32 numAttrsShowPlan = (generator->getExpGenerator()->getShowplan() ? numAttrs : 0);

  attr3 = new(generator->wHeap()) Attributes * [numAttrs + numAttrsShowPlan];

  // copy 2 Attribute pointers and 2 showplan attribute pointers to array with 3 pointers
  str_cpy_all((char *) attr3,
	      (const char *) attr2,
	      sizeof(Attributes *) * (numAttrs-1));
  if (numAttrsShowPlan)
    {
      str_cpy_all((char *) &attr3[numAttrs],
		  (const char *) &attr2[numAttrs-1],
		  sizeof(Attributes *) * (numAttrs-1));
    }

  // make a ConstValue that is a huge character column with all the
  // split keys in it, then generate code for it and get its Attributes

  constKeyArray = new(generator->wHeap()) char [keysLen];
  // now fill the allocated constant space with the data  
  copySplitKeys(constKeyArray, keysLen);

  constValSplitKeys = new (generator->wHeap()) ConstValue(
       new (generator->wHeap()) SQLChar(generator->wHeap(), keysLen,FALSE),
       constKeyArray,
       keysLen,
       NULL,
       generator->wHeap());
  constValSplitKeys->bindNode(generator->getBindWA());
  constValSplitKeys->codeGen(generator);

  // code generation is done, now get the Attributes info back
  MapInfo * map_info = generator->getMapInfoAsIs(
       constValSplitKeys->getValueId());
  GenAssert(map_info, "failed to generate long constant for split keys");
  splitKeyAttr = map_info->getAttr();

  // copy last Attributes pointer
  attr3[numAttrs-1] = splitKeyAttr;

  // ...and showplan equivalent, if needed
  if (numAttrsShowPlan)
    {
      attr3[numAttrs + numAttrsShowPlan - 1] = new (generator->wHeap())
	ShowplanAttributes(
	     constValSplitKeys->getValueId(),
	     convertNAString("SplitKeysForRangeRepartitioning",
			     generator->wHeap()));
    }

  // now that we prepared all the inputs, add the clause itself
  ex_clause * function_clause = 
    new(generator->getSpace()) ExFunctionRangeLookup(
	 attr3, 
	 generator->getSpace(),
	 getNumOfPartitions(),
	 getEncodedBoundaryKeyLength());
  generator->getExpGenerator()->linkClause(this, function_clause);
  
  return 0;
}

short UDFunction::codeGen(Generator * /* generator */)
{
  GenAssert(0, "UDFunction::codeGen. We don't have any!");
  
  return -1;
}
  
short
ScalarVariance::codeGen(Generator *generator)
{
  Attributes **attr;
  Space *space = generator->getSpace();
  
  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;
  
  ex_clause *function_clause = 0;

  switch(getOperatorType())
    {
    case ITM_VARIANCE:
      function_clause =	new(space) ExFunctionSVariance(attr, space);
      break;
    case ITM_STDDEV:
      function_clause =	new(space) ExFunctionSStddev(attr, space);
      break;
    default:
      GenAssert(0,"ScalarVariance: Unknown operator");
      break;
    }

  if (function_clause)
    generator->getExpGenerator()->linkClause(this, function_clause);
  
  return 0;
}

// UnPackCol::codeGen() ----------------------------
// UnPackCol is implemented by the ExUnPackCol function_clause.
//
short
UnPackCol::codeGen(Generator *generator)
{
  Attributes **attr;
  Space *space = generator->getSpace();
  
  if (generator->getExpGenerator()->
      genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ex_clause *function_clause = NULL;

  switch (child(0)->getValueId().getType().getTypeQualifier()) {
  case NA_CHARACTER_TYPE:
    // A packed column....
    //
    function_clause = new(space) ExUnPackCol(attr, 
                                             space,
                                             width_,
                                             base_,
                                             nullsPresent_);
    break;
  case NA_NUMERIC_TYPE:
    // A virtually packed SYSKEY column...
    //
    function_clause = new(space) ex_arith_clause(ITM_PLUS, attr, space, 
						 0, FALSE);
    break;
  default:
    GenAssert(0, "UnPackCol::codeGen() Unknown type");
    break;
  }

  if (function_clause)
    generator->getExpGenerator()->linkClause(this, function_clause);
  
  return 0;
}

// RowsetArrayScan::codeGen() ----------------------------
// RowsetArrayScan is implemented by the ExRowsetArrayScan function_clause.
//
short
RowsetArrayScan::codeGen(Generator *generator)
{
  Attributes **attr;
  ex_clause * function_clause = 0;
  Space *space = generator->getSpace();

  if (generator->getExpGenerator()->genItemExpr(this, &attr, 
                                                (1 + getArity()), -1) == 1)
    return 0;
  
  switch (getOperatorType()) {
  case ITM_ROWSETARRAY_SCAN:
    function_clause = new(space) ExRowsetArrayScan(attr, 
                                                   space,
                                                   maxNumElem_,
                                                   elemSize_,
                                                   elemNullInd_);
    break;

  case ITM_ROWSETARRAY_ROWID:
    function_clause = new(space) ExRowsetArrayRowid(attr, 
                                                    space,
                                                    maxNumElem_);
    break;

  default:
    GenAssert(0,"ScalarVariance: Unknown operator");
    break;
  }

  if (function_clause)
    generator->getExpGenerator()->linkClause(this, function_clause);
  
  return 0;
}

short
RowsetArrayInto::codeGen(Generator *generator)
{
  Attributes **attr;
  ex_clause * function_clause = 0;
  Space *space = generator->getSpace();

  if (generator->getExpGenerator()->genItemExpr(this, &attr, 
                                                (1 + getArity()), -1) == 1)
    return 0;

  function_clause = new(space) ExRowsetArrayInto(attr, 
                                                 space,
                                                 maxNumElem_,
                                                 elemSize_,
                                                 elemNullInd_);

  if(function_clause)
    generator->getExpGenerator()->linkClause(this,function_clause);

  return 0;
}

short RandomNum::codeGen(Generator *generator)
{
  Attributes **attr;
  Space *space = generator->getSpace();
  
  /*
  // convert seed, if present, to 'unsigned int'.
  if (getArity() == 1)
    {
      ItemExpr * newChild = 
	new (generator->wHeap()) 
	Cast(child(0), 
	     new (generator->wHeap()) SQLInt(generator->wHeap(), FALSE, FALSE));
      newChild = newChild->bindNode(generator->getBindWA());
      newChild->preCodeGen(generator);

      setChild(0, newChild);
    }
    */

  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;
  ex_clause *function_clause = new (space) ExFunctionRandomNum(ITM_RANDOMNUM,
							       (1+getArity()),
							       simpleRandom_,
                                                               attr, 
                                                               space);
  
  if (function_clause)
    generator->getExpGenerator()->linkClause(this, function_clause);
  
  return 0;
}

short HashDistrib::codeGen(Generator *generator)
{
  GenAssert(0, "HashDistrib::codeGen. Should use ProgDistrib or Hash2Distrib");
  return -1;
}

short Hash2Distrib::codeGen(Generator *generator)
{ 
  Attributes **attr;
  Space *space = generator->getSpace();

  if (generator->getExpGenerator()->
      genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ex_clause *function_clause = new (space) ExHash2Distrib(attr, space);

  if (function_clause)
    generator->getExpGenerator()->linkClause(this, function_clause);

  return 0;
}

short ProgDistrib::codeGen(Generator *generator)
{
  Attributes **attr;
  Space *space = generator->getSpace();
  
  if (generator->getExpGenerator()->
      genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;
  
  ex_clause *function_clause = new (space) ExProgDistrib(attr, space);;

  if (function_clause)
    generator->getExpGenerator()->linkClause(this, function_clause);
  
  return 0;
}

short ProgDistribKey::codeGen(Generator *generator)
{
  Attributes **attr;
  Space *space = generator->getSpace();
  
  if (generator->getExpGenerator()->
      genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;
  
  ex_clause *function_clause = new (space) ExProgDistribKey(attr, space);

  if (function_clause)
    generator->getExpGenerator()->linkClause(this, function_clause);
  
  return 0;
}

short PAGroup::codeGen(Generator *generator)
{
  Attributes **attr;
  Space *space = generator->getSpace();
  
  if (generator->getExpGenerator()->
      genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;
  
  ex_clause *function_clause = new (space) ExPAGroup(attr, space);

  if (function_clause)
    generator->getExpGenerator()->linkClause(this, function_clause);
  
  return 0;
}

#include "NumericType.h"

short PackFunc::codeGen(Generator* generator)
{
  Attributes** attr;
  Space* space = generator->getSpace();

  if(generator->getExpGenerator()->genItemExpr(this,&attr,(1 + getArity()),-1) == 1)
    return 0;

  // If format info is not valid, synthesize them from child's format info.
  if(!isFormatInfoValid_)
  {
    const NAType* columnType = &child(0)->getValueId().getType();
    deriveFormatInfoFromUnpackType(columnType);
  }

  ex_clause* function_clause = new(space) ExFunctionPack(attr,
                                                         space,
                                                         width_,
                                                         base_,
                                                         nullsPresent_);
  if(function_clause)
    generator->getExpGenerator()->linkClause(this,function_clause);

  return 0;
}

//
// From MAX (child(0), child(1)),
// Generates a CASE expression of the form CASE WHEN child(0) > child(1) THEN child(0) ELSE child(1).
//

short ItmScalarMinMax::codeGen(Generator* generator)
{
 const char *str = NULL;

 switch (getOperatorType()) {

 case ITM_SCALAR_MAX:              // SCALAR_MAX(5, 6) ==> 6; SCALAR_MAX(5, NULL) ==> 5; SCALAR_MAX(NULL, 6) ==> 6
    str = "CASE WHEN @A1 IS NULL OR @A2 > @A1 THEN @A2 ELSE @A1 END";
    break;

 case ITM_SCALAR_MIN:              // SCALAR_MIN(5, 6) ==> 5; SCALAR_MIN (5, NULL) ==> 5; SCALAR_MIN (NULL, 6) ==> 6
    str = "CASE WHEN @A1 IS NULL OR @A2 < @A1 THEN @A2 ELSE @A1 END";
    break;

 default:
    break;

 }  //switch on getOperatorType()

ItemExpr *newExpr = generator->getExpGenerator()->createExprTree(str, 0, 2, child(0), child(1));
newExpr = new (generator->wHeap()) Cast (newExpr, getValueId().getType().newCopy(generator->wHeap()));

getValueId().replaceItemExpr(newExpr);  // I am now an ITM_CASE
newExpr->synthTypeAndValueId(TRUE); 
setReplacementExpr(newExpr);

newExpr->preCodeGen(generator);
newExpr->codeGen(generator);

return 0;
}

short HbaseColumnLookup::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  Space * space = generator->getSpace();
  
  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  std::string colFam;
  std::string colName;
  ExFunctionHbaseColumnLookup::extractColFamilyAndName(
						       hbaseCol_.data(), -1, FALSE, colFam, colName);

  if (colFam.empty())
    {
      GenAssert(0, "Must specify the column family in the COLUMN_LOOKUP function");
    }

  NAString qualColName = colFam.data();
  qualColName += ":";
  qualColName += colName.data();

  ExFunctionHbaseColumnLookup * cl =
    new(generator->getSpace()) ExFunctionHbaseColumnLookup
    (getOperatorType(), 
     attr, 
     qualColName.data(),
     space);

  if (cl)
    generator->getExpGenerator()->linkClause(this, cl);
  
  return 0;
}

short HbaseColumnsDisplay::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  Space * space = generator->getSpace();
  
  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  char * colNames = NULL;
  NAString allCols;
  if ((csl()) && (csl()->entries() > 0))
    {
      for (Lng32 i = 0; i < csl()->entries(); i++)
	{
	  NAString * nas = (NAString*)(*csl())[i];

	  short colNameLen = nas->length();
	  allCols.append((char*)&colNameLen, sizeof(colNameLen));
	  
	  allCols += *nas;
	}

      colNames = 
      	generator->getSpace()->allocateAndCopyToAlignedSpace(allCols.data(), allCols.length());

    }

  ExFunctionHbaseColumnsDisplay * cd =
    new(generator->getSpace()) ExFunctionHbaseColumnsDisplay
    (getOperatorType(), 
     attr, 
     (csl() ? csl()->entries() : 0),
     colNames,
     space);

  if (cd)
    generator->getExpGenerator()->linkClause(this, cd);

  generator->getExpGenerator()->setPCodeMode( ex_expr::PCODE_NONE );

  return 0;
}

short HbaseColumnCreate::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  Space * space = generator->getSpace();
  
  if (generator->getExpGenerator()->genItemExpr(this, &attr, 1, -1) == 1)
    return 0;

  short numEntries = hccol_->entries();

  Lng32 colValuesLen = 0;
  NAString colNames;
  NAString colValues;

  ValueIdList colCreateVIDlist;

  for (Lng32 i = 0; i < numEntries; i++)
    {
      HbaseColumnCreateOptions * hcco = (*hccol_)[i];

      hcco->colName()->preCodeGen(generator);
      hcco->colVal()->preCodeGen(generator);

      const NAType &childType = hcco->colVal()->getValueId().getType();

      if (NOT childType.supportsSQLnull())
	{
	  NAType *newType= childType.newCopy(generator->wHeap());
	  newType->setNullable(TRUE);
	  ItemExpr * ne = new (generator->wHeap()) Cast(hcco->colVal(), 
							 newType);
	  ne = ne->bindNode(generator->getBindWA());
	  ne->preCodeGen(generator);
	  hcco->setColVal(ne);
	}

      colCreateVIDlist.insert(hcco->colName()->getValueId());
      colCreateVIDlist.insert(hcco->colVal()->getValueId());
    }
									
  ULng32 tupleLength = 0;
  short colValVCIndLen = 0;
  generator->getExpGenerator()->processValIdList(colCreateVIDlist,
						 ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
						 tupleLength,
						 attr[0]->getAtp(),
						 attr[0]->getAtpIndex(),
						 NULL,
						 ExpTupleDesc::SHORT_FORMAT,
						 attr[0]->getOffset() + 
						 (sizeof(numEntries) + sizeof(colNameMaxLen_) +
						  sizeof(colValVCIndLen) + sizeof(colValMaxLen_)));
			
  for (Lng32 i = 0; i < numEntries; i++)
    {
      HbaseColumnCreateOptions * hcco = (*hccol_)[i];

      hcco->colName()->codeGen(generator);
      hcco->colVal()->codeGen(generator);

      const NAType &childType = hcco->colVal()->getValueId().getType();
      colValVCIndLen = (childType.isVaryingLen() ?
                        childType.getVarLenHdrSize() : 0);
    }
			 
  ExFunctionHbaseColumnCreate * cl =
    new(generator->getSpace()) ExFunctionHbaseColumnCreate
    (getOperatorType(), 
     attr, 
     numEntries,
     colNameMaxLen_,
     colValMaxLen_,
     colValVCIndLen,
     generator->getSpace());

  generator->getExpGenerator()->linkClause(this, cl);
  
  return 0;
}


short LOBinsert::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  Space * space = generator->getSpace();
  
  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  char * loc = NULL;
  if (NOT lobStorageLocation().isNull())
    {
      //      loc = space->AllocateAndCopyToAlignedSpace(lobStorageLocation(), 0);
    }

  ExpLOBinsert * li =
    new(generator->getSpace()) ExpLOBinsert
    (getOperatorType(), 
     getArity()+1,
     attr, 
     objectUID_,
     (short)insertedTableSchemaName().length(),
     (char*)insertedTableSchemaName().data(),
     space);
  
  if (obj_ == LOBoper::STRING_)
    li->setFromString(TRUE);
  else if (obj_ == LOBoper::FILE_)
    li->setFromFile(TRUE);
  else if (obj_ == LOBoper::LOAD_)
    li->setFromLoad(TRUE);
  else if (obj_ == LOBoper::LOB_)
    {
      if (lobStorageType() != Lob_External_HDFS_File)
        li->setFromLob(TRUE);
      else
        li->setFromLobExternal(TRUE);
    }
  else if (obj_ == LOBoper::EXTERNAL_)
    li->setFromExternal(TRUE);
  else if (obj_ ==LOBoper::BUFFER_)
    li->setFromBuffer(TRUE);
  else if(obj_ == LOBoper::EMPTY_LOB_)
    li->setFromEmpty(TRUE);

  if (CmpCommon::getDefault(LOB_LOCKING) == DF_ON)
    li->setLobLocking(TRUE);
  else
    li->setLobLocking(FALSE);

  li->lobNum() = lobNum();
  li->setLobStorageType(lobStorageType());
  li->setLobStorageLocation((char*)lobStorageLocation().data());
  li->setLobSize(lobSize());
  li->setLobMaxSize(getLobMaxSize());
  li->setLobMaxChunkMemSize(getLobMaxChunkMemSize());
  li->setLobGCLimit(getLobGCLimit());
  li->setLobHdfsServer((char *)getLobHdfsServer().data());
  li->setLobHdfsPort(getLobHdfsPort());
  generator->getExpGenerator()->linkClause(this, li);
  
  return 0;
}

short LOBdelete::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  Space * space = generator->getSpace();
  
  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ExpLOBdelete * ld =
    new(generator->getSpace()) ExpLOBdelete(getOperatorType(), 
					    attr, 
					    space);
  
  ld->getOperand(0)->setAtp(ld->getOperand(1)->getAtp());
  ld->getOperand(0)->setAtpIndex(ld->getOperand(1)->getAtpIndex());

  ld->lobNum() = lobNum();
  ld->setLobStorageType(lobStorageType());
  ld->setLobStorageLocation((char*)lobStorageLocation().data());
  ld->setLobHdfsServer((char *)getLobHdfsServer().data());
  ld->setLobHdfsPort(getLobHdfsPort());
  generator->getExpGenerator()->linkClause(this, ld);
 
  return 0;
}

short LOBupdate::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  Space * space = generator->getSpace();
  
  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ExpLOBupdate * lu =
    new(generator->getSpace()) ExpLOBupdate
    (getOperatorType(), 
     (1+getArity()),
     attr, 
     objectUID_,
     (short)updatedTableSchemaName().length(),
     (char*)updatedTableSchemaName().data(),
     space);
  
 if (append_)
    lu->setIsAppend(TRUE);
  
  if (obj_ == LOBoper::STRING_)
    lu->setFromString(TRUE);
  else if (obj_ == LOBoper::FILE_)
    lu->setFromFile(TRUE);
  else if (obj_ == LOBoper::LOB_)
    lu->setFromLob(TRUE);
  else if (obj_ == LOBoper::EXTERNAL_)
    lu->setFromExternal(TRUE);
  else if (obj_ == LOBoper::BUFFER_)
    lu->setFromBuffer(TRUE);
  else if(obj_ == LOBoper::EMPTY_LOB_)
    lu->setFromEmpty(TRUE);
  if (CmpCommon::getDefault(LOB_LOCKING) == DF_ON)
    lu->setLobLocking(TRUE);
  else
    lu->setLobLocking(FALSE);
  lu->lobNum() = lobNum();
  lu->setLobStorageType(lobStorageType());
  lu->setLobStorageLocation((char*)lobStorageLocation().data());
  lu->setLobMaxSize(getLobMaxSize());
  lu->setLobMaxChunkMemSize(getLobMaxChunkMemSize());
  lu->setLobGCLimit(getLobGCLimit());
  lu->setLobHdfsServer((char *)getLobHdfsServer().data());
  lu->setLobHdfsPort(getLobHdfsPort());
  generator->getExpGenerator()->linkClause(this, lu);
 
  return 0;
}

short LOBselect::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  Space * space = generator->getSpace();
  
  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ExpLOBselect * ls =
    new(generator->getSpace()) ExpLOBselect(getOperatorType(), 
					    attr, 
					    space);
  ls->lobNum() = lobNum();
  ls->setLobStorageType(lobStorageType());
  ls->setLobStorageLocation((char*)lobStorageLocation().data());
  ls->setLobHdfsServer((char *)getLobHdfsServer().data());
  ls->setLobHdfsPort(getLobHdfsPort());
  generator->getExpGenerator()->linkClause(this, ls);
 
  return 0;
}

short LOBconvertHandle::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  Space * space = generator->getSpace();
  
  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ExpLOBconvertHandle * lu =
    new(generator->getSpace()) ExpLOBconvertHandle(getOperatorType(), 
						   attr, 
						   space);
  
  if (obj_ == STRING_)
    lu->setToString(TRUE);
  else if (obj_ == LOB_)
    lu->setToLob(TRUE);

  lu->lobNum() = lobNum();
  lu->setLobStorageType(lobStorageType());
  lu->setLobStorageLocation((char*)lobStorageLocation().data());
  lu->setLobHdfsServer((char *)getLobHdfsServer().data());
  lu->setLobHdfsPort(getLobHdfsPort());
  generator->getExpGenerator()->linkClause(this, lu);
 
  return 0;
}

short LOBconvert::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  Space * space = generator->getSpace();
  
  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ExpLOBconvert * lc =
    new(generator->getSpace()) ExpLOBconvert(getOperatorType(), 
					     attr, 
					     space);
  
  if (obj_ == STRING_)
    lc->setToString(TRUE);
  else if (obj_ == LOB_)
    lc->setToLob(TRUE);
 
  lc->lobNum() = lobNum();
  lc->setLobStorageType(lobStorageType());
  lc->setLobStorageLocation((char*)lobStorageLocation().data());
  generator->getExpGenerator()->linkClause(this, lc);
  lc->setConvertSize(getTgtSize());
  lc->setLobHdfsServer((char *)getLobHdfsServer().data());
  lc->setLobHdfsPort(getLobHdfsPort());
  return 0;
}


short SequenceValue::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  Space * space = generator->getSpace();
  
  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;
  Int64 origCacheSize = naTable_->getSGAttributes()->getSGCache();
  Lng32 cacheSize = CmpCommon::getDefaultNumeric(TRAF_SEQUENCE_CACHE_SIZE);
  if (cacheSize > 0)
    {
      ((SequenceGeneratorAttributes*)naTable_->getSGAttributes())->setSGCache(cacheSize);
    }

  ExFunctionSequenceValue * sv =
    new(generator->getSpace()) ExFunctionSequenceValue
    (getOperatorType(), 
     attr, 
     *naTable_->getSGAttributes(),
     space);

  sv->setRetryNum(CmpCommon::getDefaultLong(TRAF_SEQUENCE_RETRY_TIMES));

  if (cacheSize > 0)
    ((SequenceGeneratorAttributes*)naTable_->getSGAttributes())->setSGCache(origCacheSize);

  if (sv)
    generator->getExpGenerator()->linkClause(this, sv);
  
  if (currVal_)
    sv->setIsCurr(TRUE);

  return 0;
}

short HbaseTimestamp::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  MapInfo * hbtMapInfo = generator->getMapInfoAsIs(getValueId());
  if (hbtMapInfo && hbtMapInfo->isCodeGenerated())
    return 0;

  Space * space = generator->getSpace();
  
  setChild(0, tsVals_);

  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ExFunctionHbaseTimestamp * hbt =
    new(generator->getSpace()) ExFunctionHbaseTimestamp
    (getOperatorType(), 
     attr, 
     colIndex_,
     space);

  if (hbt)
    generator->getExpGenerator()->linkClause(this, hbt);

  setChild(0, NULL);

  hbtMapInfo->codeGenerated();

  return 0;
}

short HbaseTimestampRef::codeGen(Generator * generator)
{
  GenAssert(0, "HbaseTimestampRef::codeGen. Should not reach here.");

  return 0;
}

short HbaseVersion::codeGen(Generator * generator)
{
  Attributes ** attr;
  
  MapInfo * hbtMapInfo = generator->getMapInfoAsIs(getValueId());
  if (hbtMapInfo && hbtMapInfo->isCodeGenerated())
    return 0;

  Space * space = generator->getSpace();
  
  setChild(0, tsVals_);

  if (generator->getExpGenerator()->genItemExpr(this, &attr, (1 + getArity()), -1) == 1)
    return 0;

  ExFunctionHbaseVersion * hbt =
    new(generator->getSpace()) ExFunctionHbaseVersion
    (getOperatorType(), 
     attr, 
     colIndex_,
     space);

  if (hbt)
    generator->getExpGenerator()->linkClause(this, hbt);

  setChild(0, NULL);

  hbtMapInfo->codeGenerated();

  return 0;
}

short HbaseVersionRef::codeGen(Generator * generator)
{
  GenAssert(0, "HbaseVersionRef::codeGen. Should not reach here.");

  return 0;
}
