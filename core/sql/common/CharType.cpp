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
**************************************************************************
*
* File:         CharType.cpp
* Description:  Character Type Implementation
* Created:      4/27/94
* Language:     C++
*
*
**************************************************************************
*/

#include <stdarg.h>
#include "CharType.h"
#include "ComASSERT.h"
#include "unicode_char_set.h"
#include "wstr.h"
#include "CmpCommon.h"
#include "csconvert.h"
#include "NLSConversion.h"
#include "EncodedValue.h"


static const NAString LiteralCHAR("CHAR");
static const NAString LiteralVARCHAR("VARCHAR");

static const NAString LiteralBYTE("BYTE");
static const NAString LiteralVARBYTE("VARBYTE");

const NAString CharType::LiteralSchema("SCHEMA");
const NAString CharType::LiteralCatalog("CATALOG");

static const NAString LiteralLOB("LOB");
static const NAString LiteralBLOB("BLOB");
static const NAString LiteralCLOB("CLOB");

// constructor used for CHAR length semantics only
CharType::CharType( NAMemory *h,
                    const NAString&  adtName,
		    Lng32            maxLenInBytesOrNAWchars,
		    short            maxBytesPerChar,
		    NABoolean        nullTerminated,
		    NABoolean        allowSQLnull,
		    NABoolean        isUpShifted,
		    NABoolean        isCaseInsensitive,
		    NABoolean        varLenFlag,
		    CharInfo::CharSet      cs,
		    CharInfo::Collation    co,
		    CharInfo::Coercibility ce,
		    CharInfo::CharSet      encoding,
		    Int32 vcIndLen // default is 0
		  )
      : NAType(h, adtName
		, NA_CHARACTER_TYPE
		, (maxLenInBytesOrNAWchars + (nullTerminated ? 1 : 0)) * CharInfo::minBytesPerChar(cs)
		, allowSQLnull
		, allowSQLnull ? SQL_NULL_HDR_SIZE : 0
		, varLenFlag
                // computes length of VarCharLen field (0 or 2 or 4 bytes)
                // if not passed in
                , (varLenFlag ? ((vcIndLen > 0) ? vcIndLen :
                                 (((maxLenInBytesOrNAWchars*CharInfo::minBytesPerChar(cs)) & 0xFFFF8000)
                                  ? SQL_VARCHAR_HDR_SIZE_4
                                  : SQL_VARCHAR_HDR_SIZE))
                   : 0) 
		, CharInfo::minBytesPerChar(cs)
	      ),
	qualifier_	(CHARACTER_STRING_TYPE),
	charLimitInUCS2or4chars_(maxLenInBytesOrNAWchars*CharInfo::minBytesPerChar(cs) /
                               CharInfo::maxBytesPerChar(cs)),
	bytesPerChar_	(CharInfo::maxBytesPerChar(cs)),
	nullTerminated_ (nullTerminated),
	upshifted_	(isUpShifted),
	caseinsensitive_(isCaseInsensitive),
	charSet_	(cs),
	collation_	(co),
	coercibility_	(ce)
{
  ComASSERT(adtName == LiteralCHAR || adtName == LiteralVARCHAR
            || adtName == LiteralBYTE || adtName == LiteralVARBYTE
		    || adtName == LiteralSchema || adtName == LiteralCatalog);

  if ( encoding == CharInfo::UnknownCharSet )
    encodingCharSet_ = charSet_;
  else
    encodingCharSet_ = encoding;
}

// This constructor supports SJIS and UTF8 
CharType::CharType( NAMemory *h,
                    const NAString&  adtName,
		    const CharLenInfo & maxLenInfo,
		    short            maxBytesPerChar, // is maxBytesPerChar when cs is SJIS or UTF8
		    NABoolean        nullTerminated,
		    NABoolean        allowSQLnull,
		    NABoolean        isUpShifted,
		    NABoolean        isCaseInsensitive,
		    NABoolean        varLenFlag,
		    CharInfo::CharSet      cs,
		    CharInfo::Collation    co,
		    CharInfo::Coercibility ce,
		    CharInfo::CharSet      encoding
		  )
      : NAType(h, adtName
		, NA_CHARACTER_TYPE
		, (maxLenInfo.getMaxLenInBytes() +
                    (nullTerminated ? CharInfo::minBytesPerChar(cs) : 0))
		, allowSQLnull
		, allowSQLnull ? SQL_NULL_HDR_SIZE : 0
		, varLenFlag
                , (varLenFlag ? ((maxLenInfo.getMaxLenInBytes() & 0xFFFF8000) ? SQL_VARCHAR_HDR_SIZE_4
                                                                              : SQL_VARCHAR_HDR_SIZE)
                              : 0) // computes length of VarCharLen field (0 or 2 or 4 bytes)
		, CharInfo::minBytesPerChar(cs)
	      ),
	qualifier_	(CHARACTER_STRING_TYPE),
	charLimitInUCS2or4chars_ (maxLenInfo.getMaxLenInChars()),
	bytesPerChar_	(maxBytesPerChar),
	nullTerminated_ (nullTerminated),
	upshifted_	(isUpShifted),
	caseinsensitive_(isCaseInsensitive),
	charSet_	(cs),
	collation_	(co),
	coercibility_	(ce)
{
  ComASSERT(adtName == LiteralCHAR || adtName == LiteralVARCHAR
            || adtName == LiteralBYTE || adtName == LiteralVARBYTE
		    || adtName == LiteralSchema || adtName == LiteralCatalog);

  if ( encoding == CharInfo::UnknownCharSet )
    encodingCharSet_ = charSet_;
  else
    encodingCharSet_ = encoding;
}

// -----------------------------------------------------------------------

NAString CharType::getCharSetName() const
{
  return NAString("CHARACTER SET ") + CharInfo::getCharSetName(getCharSet());
}


NAString CharType::getCharSetAsPrefix(CharInfo::CharSet cs)
{
    return NAString(SQLCHARSET_INTRODUCER_IN_LITERAL) + CharInfo::getCharSetName(cs);
}

NAString CharType::getCharSetAsPrefix() const
{
  return getCharSetAsPrefix(getCharSet());
}

NAString CharType::getCollationName() const
{
  return CharInfo::getCollationName(getCollation());
}

NAString CharType::getCollateClause(CharInfo::Collation co)
{
  return NAString("COLLATE ") + CharInfo::getCollationName(co);
}

NAString CharType::getCoercibilityText(CharInfo::Coercibility ce)
{
  return NAString("(") + CharInfo::getCoercibilityText(ce) + " coercibility)";
}

// -----------------------------------------------------------------------
// Return the SQL name for this type.
// Internal type qualifier to data type name mapping function
// Needed for supporting the SQL DESCRIBE command.
// -----------------------------------------------------------------------

NAString CharType::getSimpleTypeName() const
{
  return isVaryingLen() ? LiteralVARCHAR : LiteralCHAR;
}

NAString CharType::getTypeSQLname(NABoolean terse) const
{
  char  lenbuf[30];
  char* sp = lenbuf;

  NAString rName((isVaryingLen() ||
		  DFS2REC::isAnyVarChar(getFSDatatype()))
		 ? LiteralVARCHAR : LiteralCHAR);

  // Convert the number that denotes the length for this datatype
  if ( getCharSet() == CharInfo::UTF8 /* || ( getCharSet() == CharInfo::SJIS &&
                                           getEncodingCharSet() == CharInfo::SJIS ) */ )
  {
    if (getStrCharLimit() * getBytesPerChar() == getNominalSize() )
    {
      Int32 charLen = getStrCharLimit();
      if (charLen == 1)
        sprintf(sp, "(1 CHAR) ");
      else
        sprintf(sp, "(%d CHARS) ", charLen);
    }
    else
    {
      Int32 byteLen = getNominalSize();
      if (byteLen == 1)
        sprintf(sp, "(1 BYTE) ");
      else
        sprintf(sp, "(%d BYTES) ", byteLen);
    }
  }
  else
  sprintf(sp, "(%d) ", getStrCharLimit());
  rName += sp;
  rName += getCharSetName();

  if (isCaseinsensitive())
    rName += " NOT CASESPECIFIC ";

  if (! terse)
    getTypeSQLnull(rName, terse);

  return rName;

} // getTypeSQLname()


// This method is written based on the implememtation contained in
// EncVal_encodeString(const char * str, Lng32 strLen, CharType *cType).
NAString* CharType::convertToString(double v, NAMemory * h) const
{

   // From EncVal_encodeString(), a CHAR string is encoded into a 
   // double as follows.

   // we only use the 52 fraction bits of the floating point double so that the round trip conversion 
   // from decimal to double and then from double to decimal results in exactly the original decimal.
   // From Wiki "If a decimal string with at most 15 significant digits is converted to IEEE 754 double 
   // precision representation and then converted back to a string with the same number of significant digits, 
   // then the final string should match the original"
   // =======================================================
   // 8 bits of the first character
   // hiResult += (ULng32) stringValues[0] << 12; // char 0
   // hiResult += (ULng32) stringValues[1] <<  4; // char 1
   // hiResult += (ULng32) stringValues[2] >>  4; // 4 bits of char 2
   // loResult += (ULng32) stringValues[2] << 28; // 4 bits of char 2
   // loResult += (ULng32) stringValues[3] << 20; // char 3
   // loResult += (ULng32) stringValues[4] << 12; // char 4
   // loResult += (ULng32) stringValues[5] <<  4; // char 5
   // loResult += (ULng32) stringValues[6] >>  4; // 4 bits of char 6


    // combine the two 32 bit integers to a floating point number
    // (2**32 * hiResult + loResult)
    // result = hiResult * .4294967296E10 + loResult;
    // =======================================================

    // Here we reverse the above steps to generate an 8-byte string.
    
    ULng32 hiResult = 0;
    ULng32 loResult = 0;

    hiResult = ((UInt64)v) >> 32;
    loResult = ((UInt64)v) % (UInt64)(.4294967296E10);

    // including the leading and trading single quote
    char bits[9];
    bits[0] = bits[8] = '\'';

    // hiResult: 
    //   bit 20-13: char 0
    //   bit 12-5:  char 1
    //   bit  4-1:  high 6-bits of char 2

    bits[1] = (hiResult >> 12) & 0xff;
    bits[2] = (hiResult >> 4)  & 0xff;
    bits[3] = (hiResult << 4)  & 0xf0;

    // loResult:
    //   bit 32-29: low 4-bits of char 2
    //   bit 28-21: char 3
    //   bit 20-13: char 4
    //   bit 12-5:  char 5
    //   bit 4-1:   high 4-bits of char 6

    bits[3] |= (loResult >> 28);
    bits[4] =  (loResult >> 20) & 0xff;
    bits[5] =  (loResult >> 12) & 0xff;
    bits[6] =  (loResult >> 4) & 0xff;
    bits[7] =  (loResult << 4) & 0xff;

    // compute the actual string length as any character
    // decoded could be a \0.

    Int32 i = 0;
    for ( i=1; i<=7; i++ )
      if ( bits[i] == 0 ) {
        bits[i] = '\'';
        break;
      }
 
    return new (h) NAString(bits, i+1, h);
}

//----------------------------------------------
// Compute the collation and the coercibility.
// See Ansi 4.2.3, tables 2 and 3.
// See the ES for details.
//
// Returns FALSE for "syntax error" case,
//   in which case newCO returns as UNKNOWN_COLLATION
//   (and newCE as EXPLICIT (defensive programming: but we could change this)).
//
// Returns TRUE otherwise (legal Ansi cases),
//   in which case newCO *may* return as UNKNOWN_COLLATION
//   and newCE as NO_COLLATING_SEQUENCE.
//
// So, if being called just for type synthesis
//   (CharType::synthesizeType, defined below),
//   per Ansi 4.2.3 table TWO, you only need to weed out syntax errors --
//   the function result FALSE is all you need to check.
//
// If being called for comparability checking
//   (CharType::isComparable, defined in ../optimizer/SynthType.cpp),
//   per Ansi 4.2.3 table THREE, you need to flag both
//   a) syntax errors, and b) non-propatable collations --
//   both are covered by checking the newCO result UNKNOWN_COLLATION.
//
//----------------------------------------------
NABoolean
CharType::computeCoAndCo(const CharType& other,
			 CharInfo::Collation& newCO,
			 CharInfo::Coercibility& newCE) const
{
   if ( getCoercibility() == CharInfo::COERCIBLE )
   {
     ComASSERT( getCollation() != CharInfo::UNKNOWN_COLLATION );
     newCE = other.getCoercibility();
     newCO = other.getCollation();
     return TRUE;
   }
   if ( other.getCoercibility() == CharInfo::COERCIBLE )
   {
     ComASSERT( other.getCollation() != CharInfo::UNKNOWN_COLLATION );
     newCE = getCoercibility();
     newCO = getCollation();
     return TRUE;
   }

   NABoolean sameCEdiffCO = ( getCoercibility() == other.getCoercibility() &&
			      getCollation()    != other.getCollation() );

   if ( getCoercibility() == CharInfo::IMPLICIT )
   {
     if ( sameCEdiffCO ) {
       newCE = CharInfo::NO_COLLATING_SEQUENCE;
       newCO = CharInfo::UNKNOWN_COLLATION;
     } else {
       newCE = other.getCoercibility();
       newCO = other.getCollation();
     }
     return TRUE;
   }

   if ( getCoercibility() == CharInfo::EXPLICIT )
   {
     newCE = CharInfo::EXPLICIT;
     if ( sameCEdiffCO ) {
       newCO = CharInfo::UNKNOWN_COLLATION;
       return FALSE;				// syntax error
     } else {
       newCO = getCollation();
       return TRUE;
     }
   }

   if ( getCoercibility() == CharInfo::NO_COLLATING_SEQUENCE )
   {
     if ( other.getCoercibility() == CharInfo::EXPLICIT ) {
       newCE = CharInfo::EXPLICIT;
       newCO = other.getCollation();
     } else {
       newCE = CharInfo::NO_COLLATING_SEQUENCE;
       newCO = CharInfo::UNKNOWN_COLLATION;
     }
     return TRUE;
   }

   ComASSERT(FALSE);
   return FALSE;
}

// -----------------------------------------------------------------------
// Type synthesis for binary operators
// -----------------------------------------------------------------------
const NAType* CharType::synthesizeType(NATypeSynthRuleEnum synthRule,
				       const NAType& operand1,
				       const NAType& operand2,
				       CollHeap* h,
				       UInt32 *flags) const
{
  //
  // If the second operand's type synthesis rules have higher precedence than
  // this operand's rules, use the second operand's rules.
  //
  if (operand2.getSynthesisPrecedence() > getSynthesisPrecedence())
    return operand2.synthesizeType(synthRule, operand1, operand2, h, flags);
  //
  // If either operand is not character, the expression is invalid.
  //
  if (operand1.getTypeQualifier() != NA_CHARACTER_TYPE ||
      operand2.getTypeQualifier() != NA_CHARACTER_TYPE)
    return NULL;

  const CharType& op1 = (CharType &) operand1;
  const CharType& op2 = (CharType &) operand2;

  //
  // Charsets must be equal.
  //

  // Charset inference.  If either is UnknownCharSet, we continue.
  // The synthesized type with unknown charset attribute allows us to
  // continue synthesization upwards.  At some point in the process,
  // we are able to determine the real charset. Then a push down process
  // will drive the real charset downwards.
  //
  if ( op1.getCharSet() != op2.getCharSet() &&
     op1.getCharSet() != CharInfo::UnknownCharSet &&
     op2.getCharSet() != CharInfo::UnknownCharSet
   )
     return NULL;

  Lng32 res_len_in_Chars;
  Lng32 res_nominalSize;
  CharInfo::Collation    co;
  CharInfo::Coercibility ce;
  NABoolean caseinsensitive = FALSE;
  NABoolean makeTypeVarchar = FALSE;

  switch (synthRule) {
  case SYNTH_RULE_UNION:
    res_len_in_Chars = MAXOF( op1.getStrCharLimit(), op2.getStrCharLimit() );
    res_nominalSize = MAXOF( op1.getNominalSize(), op2.getNominalSize());
    if (!(DFS2REC::isAnyVarChar(op1.getFSDatatype()) ||
          DFS2REC::isAnyVarChar(op2.getFSDatatype())))
    {
      if((flags && ((*flags & NAType::MAKE_RESULT_VARCHAR) != 0)) && 
         ( (op1.getStrCharLimit() != op2.getStrCharLimit()) ||
           (op1.getNominalSize()  != op2.getNominalSize()) ) )
      {
        makeTypeVarchar = TRUE;
      }
    }


    // See Ansi SQL92 7.10 SR 10(b)(ii), which in turn refers to
    // the rules of subclause 9.3 "set operation result data types",
    // of which Ansi 9.3 SR 3(a) applies.
    if (!op1.computeCoAndCo(op2, co, ce))
      return NULL;

    caseinsensitive = op1.isCaseinsensitive() AND
                            op2.isCaseinsensitive();
    break;
  case SYNTH_RULE_CONCAT:
    res_len_in_Chars = op1.getStrCharLimit() + op2.getStrCharLimit();
    res_nominalSize = op1.getNominalSize() + op2.getNominalSize();

    // See Ansi 6.13 SR 4(b).
    if (!op1.computeCoAndCo(op2, co, ce))
      return NULL;
    // caller sets case insensitive flag: Concat::syntheSiZeType()
    break;
  default:
    return NULL;
  }

  NABoolean null = op1.supportsSQLnull() OR op2.supportsSQLnull();
  NABoolean upshift = op1.isUpshifted() AND op2.isUpshifted();

  CharLenInfo res_CharLenInfo(res_len_in_Chars,
                              res_nominalSize);
      
  if (DFS2REC::isAnyVarChar(op1.getFSDatatype()) OR
      DFS2REC::isAnyVarChar(op2.getFSDatatype()))
    makeTypeVarchar = TRUE;
  
  NABoolean makeTypeBinary = FALSE;
  if (DFS2REC::isBinaryString(op1.getFSDatatype()) OR
      DFS2REC::isBinaryString(op2.getFSDatatype()))
    makeTypeBinary = TRUE;

  if (makeTypeBinary)
    return new(h) SQLBinaryString(h, res_nominalSize, null, makeTypeVarchar);
  else if (makeTypeVarchar)
    return new(h) SQLVarChar(h, res_CharLenInfo, null, upshift, caseinsensitive,
			     op1.getCharSet(), co, ce);
  else
    return new(h) SQLChar(h, res_CharLenInfo, null, upshift, caseinsensitive, FALSE,
			  op1.getCharSet(), co, ce);
} // synthesizeType()


// ---------------------------------------------------------------------
// A method which tells if a conversion error can occur when converting
// a value of this type to the target type.
// ---------------------------------------------------------------------
NABoolean CharType::errorsCanOccur (const NAType& target, NABoolean lax) const
{
  if (!NAType::errorsCanOccur(target) &&
      getNominalSize() <= target.getNominalSize())
    {return FALSE;}
  else
    {return TRUE;}
}

// ---------------------------------------------------------------------
// keyValue INPUT is the string representation of the current value, then
// keyValue is OUTPUT as the string rep of the very NEXT value, and RETURN TRUE.
// If we're already at the maximum value, keyValue returns empty, RETURN FALSE.
//
// We assume caller has removed trailing blanks if desired, and
// we do not pad out the returned value with minimal characters ('\0' chars).
// See Like::applyBeginEndKeys for the full treatment of trailing zeroes
// in the really correct way, for which this is just a helper function.
//
// ## We need to fix this for (a) multibyte characters, (b) collating seqs!
// ---------------------------------------------------------------------
NABoolean CharType::computeNextKeyValue(NAString &keyValue) const
{
  ComASSERT(getBytesPerChar() == 1);

  for (size_t i = keyValue.length(); i--; i)
	{
	  unsigned char c = keyValue[i];
	  if (c < UCHAR_MAX)
	    {
	      keyValue[i] = ++c;  // NOT keyValue[i]++: NAString is signed char
	      break;
	    }
	  keyValue.remove(i);
  }

  return keyValue.length();
}

NABoolean CharType::computeNextKeyValue_UTF8(NAString &keyValue) const
{
  ComASSERT(getBytesPerChar() == 4);

  for (size_t i = keyValue.length(); i--; i)
  {
     unsigned char c = keyValue[i];
     if ( (c & 0xC0) == 0x80 ) // If not first byte in a char,
        continue;              // keep going back by one byte at a time

     unsigned int UCS4val = 0; // LocaleCharToUCS4 requires "unsigned int"
     int charLenIn = LocaleCharToUCS4( &keyValue[i], 4, &UCS4val, cnv_UTF8 );
     if ( (charLenIn > 0) && (UCS4val < 0x1FFFFF) ) // Less than max char ?
     {
        char tmpBuf[10] ;
        UCS4val++;
        int charLenOut = UCS4ToLocaleChar( &UCS4val, tmpBuf, 10, cnv_UTF8 );
        tmpBuf[charLenOut] = '\0';
        //
        // Replace character with next character
        //
        keyValue.remove(i);
        keyValue.insert(i, tmpBuf);
        break;
     }
     else keyValue.remove(i);
  }

  return keyValue.length();
}

NABoolean CharType::computeNextKeyValue(NAWString &keyValue) const
{
  ComASSERT(getBytesPerChar() == SQL_DBCHAR_SIZE);

  NAWchar maxValue = (NAWchar)CharType::getMaxSingleCharacterValue();

  for (size_t i = keyValue.length(); i--; i)
	{
	  NAWchar c = keyValue[i];

#ifdef NA_LITTLE_ENDIAN
#ifdef IS_MP
          if ( CharInfo::is_NCHAR_MP(getCharSet()) )
              wc_swap_bytes(&c, 1);
#endif
#endif

	  if (c < maxValue)
	    {
              c += 1;
#ifdef NA_LITTLE_ENDIAN
#ifdef IS_MP
          if ( CharInfo::is_NCHAR_MP(getCharSet()) )
              wc_swap_bytes(&c, 1);
#endif
#endif
	      keyValue[i] = c;  // NOT keyValue[i]++: NAWString is signed char
	      break;
	    }
	  keyValue.remove(i);
	}

  return keyValue.length();
}

NABoolean CharType::isEncodingNeeded() const
{
  switch (getCharSet()) {
#if 0 /* SJIS NOT SUPPORTED YET */
    case CharInfo::SJIS:
      if (getEncodingCharSet() == CharInfo::SJIS)
        return FALSE; // treated as a stream of 8-bit bytes
      else if (getEncodingCharSet() == CharInfo::UCS2) // this option is not supported - i.e., does not work yet
        return TRUE; // encoding is needed because of byte-order
      return FALSE;  // make a guess
#endif

    case CharInfo::ISO88591:
    case CharInfo::UTF8:
        return FALSE; // treated as a stream of 8-bit bytes

    case CharInfo::UNICODE: // encoding is needed because of byte-order
      return TRUE;

    case CharInfo::KANJI_MP:    // Since KANJI/KSC5601 data in MP is stored
    case CharInfo::KSC5601_MP:  // as typed in, there is no need to encode.
    default:
      return FALSE;
  }
}

// -- Blank and min and max permissible values for a single character
// ## These too will need to be changed to handle different collating sequences
// ## and character sets and such ...

Lng32 CharType::getBlankCharacterValue() const
{
  switch (getCharSet()) {
    case CharInfo::UNICODE:
      return unicode_char_set::space_char();
    default:
      return Lng32(' ');		//##NCHAR: is this correct for Kanji etc?
  }
}

Lng32 CharType::getMinSingleCharacterValue() const
{
  return 0;
}

Lng32 CharType::getMaxSingleCharacterValue() const
{
  //##NCHAR:  This switches-within-switch-stmt is not optimally maintainable.
  //##	      Best way to implement this would likely be to augment the
  //##		CharInfo.cpp charset-map-struct and the CollationInfo class...


  switch (getCharSet()) {
    case CharInfo::ISO88591:
      switch (getCollation()) {
	case CharInfo::DefaultCollation: return UCHAR_MAX;
#if 0 /* SJIS NOT SUPPORTED YET */
	case CharInfo::SJIS_COLLATION:   return UCHAR_MAX;  //##NCHAR: [note 1]
#endif
	case CharInfo::CZECH_COLLATION:   
          return collationMaxChar[CollationInfo::getCollationParamsIndex(CharInfo::CZECH_COLLATION)];  
	case CharInfo::CZECH_COLLATION_CI:   
          return collationMaxChar[CollationInfo::getCollationParamsIndex(CharInfo::CZECH_COLLATION_CI)];  
      } // switch (getCollation())
      break;

    case CharInfo::UTF8:
      return UCHAR_MAX;
      break;

    case CharInfo::BINARY:
      return UCHAR_MAX;
      break;

#if 0 /* SJIS NOT SUPPORTED YET */
    case CharInfo::SJIS:
      if (getEncodingCharSet() == CharInfo::SJIS)
        return UCHAR_MAX;
#endif
      // assume getEncodingCharSet() == CharInfo::UCS2 // this option is not supported - i.e., does not work yet
      // fall through
    case CharInfo::UNICODE:
      switch (getCollation()) {
	case CharInfo::DefaultCollation: return 0xffff; // max UCS-2 character
	case CharInfo::SJIS_COLLATION:   return 0xfcfc; // max SJIS 2-byte char
      }
      break;
  } // switch (getCharSet())

  ComASSERT(FALSE);
  return INT_MIN;
}

NABoolean CharType::isCharSetAndCollationComboOK() const
{
  if ( getCollation() == CharInfo::CZECH_COLLATION &&
       charSet_ != CharInfo::ISO88591 )
     return FALSE;

  // An unknown collation is allowed if not used in any dyadic operation
  // or comparison -- the parser just issues a warning for it.
  return getCollation() == CharInfo::UNKNOWN_COLLATION ||
         getMaxSingleCharacterValue() != INT_MIN;
}

// -----------------------------------------------------------------------
// Equality comparison
// -----------------------------------------------------------------------
NABoolean CharType::compareCharAttrs(const NAType& other) const
{
  return caseinsensitive_ == ((CharType&)other).caseinsensitive_ &&
    nullTerminated_ == ((CharType&)other).nullTerminated_ &&
    upshifted_	 == ((CharType&)other).upshifted_ &&
    charSet_	 == ((CharType&)other).charSet_ &&
    collation_	 == ((CharType&)other).collation_ &&
    coercibility_	 == ((CharType&)other).coercibility_ &&
    encodingCharSet_ == ((CharType&)other).encodingCharSet_ ;
}

NABoolean CharType::operator==(const NAType& other) const
{
  return NAType::operator==(other) &&
    compareCharAttrs(other);
}

// -----------------------------------------------------------------------
// Check compatibility for string types.
// -----------------------------------------------------------------------
NABoolean CharType::isCompatible(const NAType& other, UInt32 * flags) const
{
  return (NAType::isCompatible(other, flags) &&
	  getCharSet() == ((CharType&)other).getCharSet() &&
	  getCharSet() != CharInfo::UnknownCharSet);
}
// -----------------------------------------------------------------------
// Check compatibility for string types allowing UnknownCharSet
// compatible if either is an unknown character set.
// -----------------------------------------------------------------------
NABoolean CharType::isCompatibleAllowUnknownCharset(const NAType& other) const
{
  return (NAType::isCompatible(other) &&
	  (getCharSet() == CharInfo::UnknownCharSet ||
	  ((CharType&)other).getCharSet() == CharInfo::UnknownCharSet));
}

NABoolean CharType::createSQLLiteral(const char * buf,
                                     NAString *&stringLiteral,
                                     NABoolean &isNull,
                                     CollHeap *h) const
{
  if (NAType::createSQLLiteral(buf, stringLiteral, isNull, h))
    return TRUE;

  // Generate a string literal in a target charset (usually UTF8)
  // of the form _<type charset>'<string in target charset>'
  // TBD: - consider using hex literal instead of unprintable characters
  NABoolean result = TRUE;
  NAString *resultLiteral = new(h) NAString(getCharSetAsPrefix());
  const char *valPtr = buf + getSQLnullHdrSize();
  Int32 valLen = 0;
  CharInfo::CharSet sourceCS = getCharSet();
  char *tempBuf = NULL;

  if (getVarLenHdrSize() == 0)
    valLen = getNominalSize();
  else 
    {
      if (getVarLenHdrSize() == 2)
        valLen = *((Int16 *) valPtr);
      else if (getVarLenHdrSize() == 4)
        valLen = *((Int32 *) valPtr);
      else
        ComASSERT(FALSE);

      valPtr += getVarLenHdrSize();
    }

  if (sourceCS == CharInfo::BINARY)
    *resultLiteral += " X";

  *resultLiteral += "'";

  switch (sourceCS)
    {
    case CharInfo::BINARY:
      {
        Lng32 hexlen = str_computeHexAsciiLen(valLen);
        
        tempBuf = new(h) char[hexlen+1];
        Lng32 rc = str_convertToHexAscii(valPtr, valLen,
                                         tempBuf, hexlen+1,
                                         TRUE /*append null*/);
        *resultLiteral += tempBuf;
      }
      break;

    case CharInfo::UTF8:
      *resultLiteral += NAString(valPtr, valLen);
      break;

    case CharInfo::ISO88591:
      {
        // try it the easy way, for all ASCII chars
        NABoolean allAscii = TRUE;
        unsigned char *ucharBuf = (unsigned char *) valPtr;
        for (Int32 i=0; i<valLen && allAscii; i++)
          if (ucharBuf[i] > 127)
            allAscii = FALSE;
        if (allAscii)
          {
            *resultLiteral += NAString((char *) valPtr, valLen);
            break;
          }
      }
      // otherwise fall through to the next case
          
    case CharInfo::UNICODE:
      {
        char *firstUntranslatedChar = NULL;
        unsigned int outputLength = 0;
        Int32 tempBufSize = CharInfo::getMaxConvertedLenInBytes(sourceCS,
                                                                valLen,
                                                                CharInfo::UTF8);
        tempBuf = new(h) char[tempBufSize];

        int retCode = LocaleToUTF8(cnv_version1,
                                   valPtr,
                                   valLen,
                                   tempBuf,
                                   tempBufSize,
                                   convertCharsetEnum(sourceCS),
                                   firstUntranslatedChar,
                                   &outputLength);
        if (retCode != 0)
          result = FALSE;

        *resultLiteral += NAString(tempBuf, outputLength);
      }
      break;

    default:
      // non-supported character set
      assert(FALSE);
      break;
    } // end case

  // check for any quotes (skip the first one) and duplicate them
  size_t firstQuotePos = resultLiteral->first('\'');
  size_t otherQuotePos = firstQuotePos + 1;
  while ((otherQuotePos = resultLiteral->first('\'', otherQuotePos)) != NA_NPOS)
    {
      resultLiteral->replace(otherQuotePos, 1, "''");
      otherQuotePos += 2;
    }

  *resultLiteral += "'";
  if (tempBuf)
    NADELETEBASIC(tempBuf, h);
  stringLiteral = resultLiteral;

  return result;
}


SQLChar::SQLChar(NAMemory *h,
                 Lng32 maxLen,
		 NABoolean allowSQLnull,
		 NABoolean isUpShifted,
		 NABoolean isCaseInsensitive,
		 NABoolean varLenFlag,
		 CharInfo::CharSet cs,
		 CharInfo::Collation co,
		 CharInfo::Coercibility ce,
		 CharInfo::CharSet encoding
		 )
     : CharType(h, varLenFlag ? LiteralVARCHAR : LiteralCHAR,
		maxLen, CharInfo::maxBytesPerChar(cs),
		FALSE, allowSQLnull, isUpShifted, isCaseInsensitive,
		varLenFlag, cs, co, ce,
		encoding)
{}

SQLChar::SQLChar(NAMemory *h,
                 const CharLenInfo & maxLenInfo,
		 NABoolean allowSQLnull,
		 NABoolean isUpShifted,
		 NABoolean isCaseInsensitive,
		 NABoolean varLenFlag,
		 CharInfo::CharSet cs,
		 CharInfo::Collation co,
		 CharInfo::Coercibility ce,
		 CharInfo::CharSet encoding
		 )
     : CharType(h, varLenFlag ? LiteralVARCHAR : LiteralCHAR,
		maxLenInfo, CharInfo::maxBytesPerChar(cs),
		FALSE, allowSQLnull, isUpShifted, isCaseInsensitive,
		varLenFlag, cs, co, ce,
		encoding)
{}

SQLVarChar::SQLVarChar(NAMemory *h,
                       Lng32 maxLen,
		       NABoolean allowSQLnull,
		       NABoolean isUpShifted,
		       NABoolean isCaseInsensitive,
		       CharInfo::CharSet cs,
		       CharInfo::Collation co,
		       CharInfo::Coercibility ce,
		       CharInfo::CharSet encoding,
                       Lng32 vcIndLen
		      )
    : CharType(h, LiteralVARCHAR,
	       maxLen, CharInfo::maxBytesPerChar(cs),
	       FALSE, allowSQLnull, isUpShifted, isCaseInsensitive,
	       TRUE, cs, co, ce,
	       encoding, vcIndLen),
      clientDataType_(collHeap()),  // Get heap from NABasicObject. Can't allocate on stack.
      wasHiveString_(FALSE)
{}

SQLVarChar::SQLVarChar(NAMemory *h, 
                       const CharLenInfo & maxLenInfo,
		       NABoolean allowSQLnull,
		       NABoolean isUpShifted,
		       NABoolean isCaseInsensitive,
		       CharInfo::CharSet cs,
		       CharInfo::Collation co,
		       CharInfo::Coercibility ce,
		       CharInfo::CharSet encoding
		      )
    : CharType(h, LiteralVARCHAR,
	       maxLenInfo, CharInfo::maxBytesPerChar(cs),
	       FALSE, allowSQLnull, isUpShifted, isCaseInsensitive,
	       TRUE, cs, co, ce,
	       encoding),
      clientDataType_(collHeap()),  // Get heap from NABasicObject. Can't allocate on stack.
      wasHiveString_(FALSE)
{}

// -----------------------------------------------------------------------
// Equality comparison
// -----------------------------------------------------------------------
NABoolean SQLVarChar::operator==(const NAType& other) const
{
  NABoolean compResult = FALSE; // false, failed. true, passed.

  // if this or other was originally a hive string type, then
  // do not compare lengths.
  if ((wasHiveString()) ||
      ((other.getTypeName() == LiteralVARCHAR) &&
       (((SQLVarChar&)other).wasHiveString())))
    compResult = equalIgnoreLength(other);
  else
    compResult = NAType::operator==(other);

  if (NOT compResult)
    return FALSE;

  return compareCharAttrs(other);
}

// -----------------------------------------------------------------------
// The SQL/C Preprocessor rewrites a VARCHAR declaration (Ansi 19.4)
// as an ANSIVARCHAR datatype token (Tandem-internal extension),
// which is actually this ANSIChar datatype --
// FIXED char, with nul-terminator, *not* VAR char with a length prefix.
// In fact, the varLenFlag param is unused:  lots of things break
// in cli + rfork if a CharType is both nul-terminated and has a vc-header.
// -----------------------------------------------------------------------
ANSIChar::ANSIChar(NAMemory *h,
                   Lng32 maxLen,
		   NABoolean allowSQLnull,
		   NABoolean isUpShifted,
		   NABoolean varLenFlag,	// *unused*
		   CharInfo::CharSet cs,
		   CharInfo::Collation co,
		   CharInfo::Coercibility ce,
		   CharInfo::CharSet encoding
		  )
     : CharType(h, LiteralCHAR,
		maxLen, CharInfo::maxBytesPerChar(cs),
		TRUE, allowSQLnull, isUpShifted, FALSE, FALSE, cs, co, ce,
		encoding)
//##
//## : CharType(varLenFlag ? LiteralVARCHAR : LiteralCHAR,
//##		maxLen, CharInfo::maxBytesPerChar(cs),
//##		TRUE, allowSQLnull, isUpShifted, varLenFlag, cs, co, ce,
//##		tokNCHARinParser)
{}

short ANSIChar::getFSDatatype() const
{
  switch ( getBytesPerChar() ) {
  case 1:
    return REC_BYTE_V_ANSI;
  case 2:
#ifdef IS_MP
    if ( CharInfo::is_NCHAR_MP(getCharSet()) )
       return REC_BYTE_V_ANSI;
    else
#endif
       return REC_BYTE_V_ANSI_DOUBLE;
  default:
    ComASSERT(FALSE);
    return REC_BYTE_V_ANSI;
  }
}

short SQLChar::getFSDatatype() const
{
  switch ( getBytesPerChar() ) {
  case 1:
    return REC_BYTE_F_ASCII;
  case 2:
#if 0 /* NCHAR_MP and SJIS are NOT supported yet */
    if ( CharInfo::is_NCHAR_MP(getCharSet()) || ( getCharSet() == CharInfo::SJIS &&
                                                  getEncodingCharSet() == CharInfo::SJIS ) )
       return REC_BYTE_F_ASCII;
    else
#endif
       return REC_BYTE_F_DOUBLE;
  case 4:
    if (getCharSet() == CharInfo::UTF8)
      return REC_BYTE_F_ASCII;
    // fall through
  default:
    ComASSERT(FALSE);
    return REC_BYTE_F_ASCII;
  }
}

short SQLVarChar::getFSDatatype() const
{
  switch ( getBytesPerChar() ) {
  case 1:
    return REC_BYTE_V_ASCII;
  case 2:
#if 0 /* NCHAR_MP and SJIS are NOT supported yet */
    if ( CharInfo::is_NCHAR_MP(getCharSet()) || ( getCharSet() == CharInfo::SJIS &&
                                                  getEncodingCharSet() == CharInfo::SJIS ) )
       return REC_BYTE_V_ASCII;
    else
#endif
       return REC_BYTE_V_DOUBLE;
  case 4:
    if (getCharSet() == CharInfo::UTF8)
      return REC_BYTE_V_ASCII;
    // fall through
  default:
    ComASSERT(FALSE);
    return REC_BYTE_V_ASCII;
  }
}

short CharType::getFSDatatype() const
{
  ComASSERT(FALSE);
  return -1;
}

Lng32 CharType::getPrecisionOrMaxNumChars() const
{
  if ( (charLimitInUCS2or4chars_ > 0) && 
       (charSet_ == CharInfo::UTF8)   &&
       (charLimitInUCS2or4chars_ < getDataStorageSize() ) )
     return charLimitInUCS2or4chars_ ;
  return 0;  // no limits for now
}

Lng32 CharType::getScaleOrCharset() const
{
  return (Lng32) getCharSet();
}

void CharType::generateTextThenSetDisplayDataType ( CharInfo::CharSet cs   // in
                                                  , NAString & ddt         // in/out
                                                  )
{
  if ( (cs NEQ CharInfo::UTF8 /* AND cs NEQ CharInfo::SJIS */)  OR  ddt.isNull() )
    return;  // only do this for CHARACTER SET SJIS and UTF8

  ddt += "(";

  UInt32 charLimit = getStrCharLimit();
  UInt32 sizeInBytes = getNominalSize();

  if ( charLimit * getBytesPerChar() == sizeInBytes )
  {
    ddt += LongToNAString(charLimit);
    if (charLimit EQU 1)
      ddt += " CHAR";
    else
      ddt += " CHARS";
  }
  else
  {
    ddt += LongToNAString(sizeInBytes);
    if (sizeInBytes EQU 1)
      ddt += " BYTE";
    else
      ddt += " BYTES";
  }

  ddt += ") CHARACTER SET ";

  ddt += CharInfo::getCharSetName(cs);

  setDisplayDataType(ddt);
}

// -----------------------------------------------------------------------
// Print function for debugging
// -----------------------------------------------------------------------
void CharType::print(FILE *ofd, const char *indent)
{
#ifdef TRACING_ENABLED 
  fprintf(ofd,"%s%s\n",indent,getTypeSQLname());
#endif
} // CharType::print()

// -----------------------------------------------------------------------
// A method for generating the hash key.
// SQL builtin types should return getTypeSQLName()
// -----------------------------------------------------------------------
NAString* CharType::getKey(CollHeap* h) const
{
  return new (h) NAString(getTypeSQLname(), h);
}

void CharType::minMaxRepresentableValue(void* bufPtr,
                                        Lng32* bufLen,
                                        NAString ** stringLiteral,
                                        NABoolean isMax,
                                        CollHeap* h) const
{
  Int32 i;
  Int32 vcLenHdrSize = getVarLenHdrSize();
  char *valPtr       = reinterpret_cast<char *>(bufPtr) + vcLenHdrSize;
  Int32 valBufLen    = getNominalSize();
  Int32 valLen       = valBufLen; // output length of min/max value
  char minmax_char;
  wchar_t minmax_wchar;

  ComASSERT(*bufLen >= vcLenHdrSize + valBufLen);

  switch (getCharSet())
  {
  case CharInfo::ISO88591:
  case CharInfo::SJIS:
  case CharInfo::BINARY:
    if (isMax)
      minmax_char = (char)getMaxSingleCharacterValue();
    else
      minmax_char = (char)getMinSingleCharacterValue();
    memset(valPtr, minmax_char, valBufLen);
    break;

  case CharInfo::UTF8:
    if (isMax)
      valLen = fillWithMaxUTF8Chars(valPtr,
                                    valBufLen,
                                    getPrecisionOrMaxNumChars());
    else
      valLen = fillWithMinUTF8Chars(valPtr,
                                    valBufLen,
                                    getPrecisionOrMaxNumChars());
    break;

  case CharInfo::UCS2:
    if (isMax)
      minmax_wchar = (wchar_t)getMaxSingleCharacterValue();
    else
      minmax_wchar = (wchar_t)getMinSingleCharacterValue();
#ifdef NA_LITTLE_ENDIAN
    wc_swap_bytes(&minmax_wchar, 1);
#endif // NA_LITTLE_ENDIAN
    valBufLen /= SQL_DBCHAR_SIZE;
    for (i=0 ;i < valBufLen; i++)
      ((wchar_t *)valPtr)[i] = minmax_wchar;
    break;

  default:
    ComASSERT(FALSE);
  }

  // copy the output value length into the varchar len header
  if (vcLenHdrSize == sizeof(short))
    {
      short vc_len = (short) valLen;
      str_cpy_all((char *)bufPtr, (char *)&vc_len, vcLenHdrSize);
    }
  else if (vcLenHdrSize == sizeof(Int32))
    {
      Int32 vc_len = (Int32) valLen;
      str_cpy_all((char *)bufPtr, (char *)&vc_len, vcLenHdrSize);
    }
  else
    ComASSERT(vcLenHdrSize == 0);

  if (stringLiteral)
    {
      NABoolean isNull = FALSE;
      //the bufPtr passed in point to the postion after null header
      //createSQLLiteral requires the input pointer to be the begin of the buffer
      NABoolean res = createSQLLiteral((const char *) bufPtr  - getSQLnullHdrSize(), *stringLiteral, isNull, h);
      assert(res);
    }
}

// -- Min and max permissible values for a CHAR string
// ## These too will need to be changed to handle different collating sequences
// ## and character sets and multibyte chars and such ...

void SQLChar::minRepresentableValue(void* bufPtr, Lng32* bufLen,
				    NAString ** stringLiteral,
				    CollHeap* h) const
{
  minMaxRepresentableValue(bufPtr, bufLen, stringLiteral, FALSE, h);
}

void SQLChar::maxRepresentableValue(void* bufPtr, Lng32* bufLen,
				    NAString ** stringLiteral,
				    CollHeap* h) const
{
  minMaxRepresentableValue(bufPtr, bufLen, stringLiteral, TRUE, h);
}

//  encoding of the max char value
double SQLChar::getMaxValue() const
{
  EncodedValue dummyVal(0.0);

  double encodedval = dummyVal.minMaxValue(this, FALSE);

  return encodedval;
}

//  encoding of the min char value
double SQLChar::getMinValue() const
{
  EncodedValue dummyVal(0.0);

  double encodedval = dummyVal.minMaxValue(this, TRUE);

  return encodedval;
}


double SQLChar::encode (void *bufPtr) const
{
#if 1  /* Stub */
return 0;
#else
  /* Following code moved into EncVal_Char_encode() in .../optimizer/EncodedValue.cpp */
  /* since the optimizer was the only place that needed it. */
  char *charBufPtr = (char *) bufPtr;
  if (supportsSQLnull())
     charBufPtr += getSQLnullHdrSize();
  return encodeString(charBufPtr,getNominalSize());
#endif
}

// -- Min and max permissible values for a VARCHAR string
// ## These too will need to be changed to handle different collating sequences
// ## and character sets and multibyte chars and such ...

void SQLVarChar::minRepresentableValue(void* bufPtr, Lng32* bufLen,
				       NAString ** stringLiteral,
				       CollHeap* h) const
{
  minMaxRepresentableValue(bufPtr, bufLen, stringLiteral, FALSE, h);
}

void SQLVarChar::maxRepresentableValue(void* bufPtr, Lng32* bufLen,
				       NAString ** stringLiteral,
				       CollHeap* h) const
{
  minMaxRepresentableValue(bufPtr, bufLen, stringLiteral, TRUE, h);
}

double SQLVarChar::encode (void *bufPtr) const
{
#if 1  /* Stub */
return 0;
#else
  /* Following code moved into EncVal_Char_encode() in .../optimizer/EncodedValue.cpp */
  /* since the optimizer was the only place that needed it. */
  char *charBufPtr = (char *) bufPtr;
  if (supportsSQLnull())
     charBufPtr += getSQLnullHdrSize();
  // copy the actual length of the string into an aligned variable
  short actualLen;
  ComASSERT(sizeof(short) == getVarLenHdrSize());
  str_cpy_all((char *) &actualLen,charBufPtr,sizeof(short));

  return encodeString(&charBufPtr[getVarLenHdrSize()],actualLen);
#endif
}

THREAD_P NABoolean pushDownRequiredByODBC = TRUE;

// This function finds the chartype to be pushed down among a set S of
// char types and a default char type.
// Arguments: 0th: the default char type;
//            1st: the first known char type;
//            2nd: the second known char type;
//               ... ...
// It returns
//    0 if each chartype in S is of a known charset;
//    the default chartype if all chartypes in S are of unknown charset;
//    the first chartype in S with known charset;
//
const CharType*
CharType::findPushDownCharType(CharInfo::CharSet cs,
			       const CharType* first, ...)
{
  if ( pushDownRequiredByODBC == FALSE )
     return 0;

  va_list ap;
  va_start(ap, first);

  const CharType* ctp = first;
  const CharType* sampleCharType = 0;

  if ( first == NULL )
     return desiredCharType(cs);

  Int32 total = 0;
  Int32 ct = 0;

  do
  {
     total++;

     if ( ctp->getCharSet() != CharInfo::UnknownCharSet ) {

        ct++;
        if ( sampleCharType == 0 )
           sampleCharType = ctp;
     }

  } while ( (ctp = (CharType*)va_arg(ap, void*)) );

  va_end(ap);

  if ( ct == total )
     return 0;

  if ( sampleCharType )
    return sampleCharType;
  else
    return desiredCharType(cs);
}

const CharType* CharType::desiredCharType(enum CharInfo::CharSet cs)
{
  static SQLChar unicodeChar(NULL, 0, TRUE, FALSE, FALSE, FALSE, CharInfo::UNICODE);
  static SQLChar latin1Char(NULL, 0, TRUE, FALSE, FALSE, FALSE, CharInfo::ISO88591);
  static SQLChar sjisChar(NULL, 0, TRUE, FALSE, FALSE, FALSE, CharInfo::SJIS /* ... old kludge ... CharInfo::ISO88591 */, CharInfo::DefaultCollation, CharInfo::COERCIBLE, CharInfo::SJIS/*encoding*/);
  // static SQLChar sjisUnicodeChar(NULL, 0, TRUE, FALSE, FALSE, FALSE, CharInfo::SJIS, CharInfo::DefaultCollation, CharInfo::COERCIBLE, CharInfo::UNICODE/*encoding*/);
  static SQLChar utf8Char(NULL, 0, TRUE, FALSE, FALSE, FALSE, CharInfo::UTF8 /* ... old kludge ... CharInfo::ISO88591 */, CharInfo::DefaultCollation, CharInfo::COERCIBLE, CharInfo::UTF8/*encoding*/);

  switch ( cs ) {
    case CharInfo::UNICODE:
      return &unicodeChar;

    case CharInfo::UTF8:
      return &utf8Char;

#if 0 /* SJIS NOT SUPPORTED YET */
    case CharInfo::SJIS:
      // ??? if (getEncodingCharSet() == CharInfo::SJIS)
      return &sjisChar;
      // ??? else if (getEncodingCharSet() == CharInfo:UCS2)
      // ???   return &sjisUnicodeChar;
      // ??? else
      // ???   return &sjisChar;
#endif

    case CharInfo::ISO88591:
    default:
      return &latin1Char;
  }
  return 0;
}

// round length up to next bigger quantum step
static Lng32 quantize(Lng32 len)
{
  static const Lng32 quanLen[] = { 8, 16, 32, 64, 128, 256, 512, 1024 };
  Lng32 x, limit=sizeof(quanLen) / sizeof(quanLen[0]);
  if (len <= 0) { return quanLen[0]; }
  if (len >= quanLen[limit-1]) { return len; }
  for (x=0; x<limit-1; x++) {
    if (len <= quanLen[x]) { return quanLen[x]; }
  }
  return len;
}

// A virtual function to return a char equivalent of this type
NAType* SQLVarChar::equivalentCharType(NAMemory *h, NABoolean quantizeLen)
{
  Lng32 len_in_chars ;
  Lng32 len_in_bytes ;

  len_in_chars = quantizeLen ? quantize(getStrCharLimit()) : getStrCharLimit();
  len_in_bytes = quantizeLen ? quantize(getNominalSize()) : getNominalSize() ;

  CharLenInfo CLInfo( len_in_chars, len_in_bytes );
  return new(h) SQLChar(h, CLInfo, supportsSQLnull(), isUpshifted(),
                        isCaseinsensitive(), FALSE,
                        getCharSet(), getCollation(), getCoercibility());
}

// A virtual function to return a varchar equivalent of this type
NAType* SQLChar::equivalentVarCharType(NAMemory *h, NABoolean quantizeLen)
{
  Lng32 len_in_chars ;
  Lng32 len_in_bytes ;

  len_in_chars = quantizeLen ? quantize(getStrCharLimit()) : getStrCharLimit();
  len_in_bytes = quantizeLen ? quantize(getNominalSize()) : getNominalSize() ;

  CharLenInfo CLInfo( len_in_chars, len_in_bytes );
  return new(h) SQLVarChar(h, CLInfo, supportsSQLnull(), isUpshifted(),
			   isCaseinsensitive(),
			   getCharSet(), getCollation(), getCoercibility());
}

// A virtual function to return a varchar equivalent of this type
NAType* ANSIChar::equivalentVarCharType(NAMemory *h, NABoolean quantizeLen)
{
  Lng32 len = quantizeLen ? quantize(getStrCharLimit()) : getStrCharLimit();
  return new(h) SQLVarChar(h, len, supportsSQLnull(), isUpshifted(),
			   isCaseinsensitive(),
			   getCharSet(), getCollation(), getCoercibility());
}

short SQLLongVarChar::getFSDatatype() const
{
   if ( getCharSet() == CharInfo::UNICODE )
      return REC_BYTE_V_DOUBLE;
   else
      return REC_BYTE_V_ASCII_LONG;
}

short SQLLongVarChar::getTrueFSDatatype() const
{
   return REC_BYTE_V_ASCII_LONG;
}


//////////////////////////////
// class SQLlob
//////////////////////////////
SQLlob::SQLlob(NAMemory *h, 
	       NABuiltInTypeEnum  ev,
	       Int64 lobLength, 
	       LobsStorage ls,
	       NABoolean allowSQLnull,
	       NABoolean inlineIfPossible,
	       NABoolean externalFormat,
	       Lng32 extFormatLen
		)
  : NAType(h, (ev == NA_LOB_TYPE ? LiteralLOB : LiteralLOB)
	    , ev
	    , extFormatLen
	    , allowSQLnull
	    , allowSQLnull ? SQL_NULL_HDR_SIZE : 0
	    , TRUE
	    , SQL_VARCHAR_HDR_SIZE_4
	    ),
    inlineIfPossible_(inlineIfPossible),
    externalFormat_(externalFormat),
    extFormatLen_(extFormatLen),
    lobStorage_(ls),
    lobLength_(lobLength)
{
  if (externalFormat_)
    lobStorage_ = Lob_External_HDFS_File;
  else
    lobStorage_ = Lob_HDFS_File;
}

// ---------------------------------------------------------------------
// A method which tells if a conversion error can occur when converting
// a value of this type to the target type.
// ---------------------------------------------------------------------
NABoolean SQLlob::errorsCanOccur (const NAType& target, NABoolean lax) const
{
  if (!NAType::errorsCanOccur(target))
    {return FALSE;}
  else
    {return TRUE;}
}

NAString SQLlob::getTypeSQLname(NABoolean terse) const
{
  NAString rName = "LOB";

  getTypeSQLnull(rName, terse);

  return rName;

} // getTypeSQLname()

/////////////////////////////////////
// class SQLBlob
/////////////////////////////////////
SQLBlob::SQLBlob(NAMemory *h, 
  Int64 blobLength, 
  LobsStorage lobStorage,
  NABoolean allowSQLnull,
  NABoolean inlineIfPossible,
  NABoolean externalFormat,
  Lng32 extFormatLen
)
  : SQLlob(h, NA_LOB_TYPE,
	   blobLength,
	   lobStorage,
	   allowSQLnull,
	   inlineIfPossible,
	   externalFormat,
	   extFormatLen)
{
  setCharSet(CharInfo::ISO88591);//lobhandle can only be in ISO format
}

NAType *SQLBlob::newCopy(NAMemory* h) const
{ return new(h) SQLBlob(*this,h); }

// -----------------------------------------------------------------------
// Type synthesis for binary operators
// -----------------------------------------------------------------------
const NAType* SQLBlob::synthesizeType(NATypeSynthRuleEnum synthRule,
				      const NAType& operand1,
				      const NAType& operand2,
				      CollHeap* h,
				      UInt32 *flags) const
{
  if (operand1.getFSDatatype() != REC_BLOB ||
      operand2.getFSDatatype() != REC_BLOB)
    return NULL;
  
  SQLBlob& op1 = (SQLBlob &) operand1;
  SQLBlob& op2 = (SQLBlob &) operand2;

  NABoolean null = op1.supportsSQLnull() OR op2.supportsSQLnull();

  if (synthRule == SYNTH_RULE_UNION)
    {
      return new(h) SQLBlob(h, MAXOF(op1.getLobLength(), op2.getLobLength()),
			    op1.getLobStorage(),
			    null);
    }

  return NULL;
}

/////////////////////////////////////
// class SQLClob
/////////////////////////////////////
SQLClob::SQLClob(NAMemory *h, 
  Int64 clobLength, 
  LobsStorage lobStorage,
  NABoolean allowSQLnull,
  NABoolean inlineIfPossible,
  NABoolean externalFormat,
  Lng32 extFormatLen
)
  : SQLlob(h, NA_LOB_TYPE,
	   clobLength,
	   lobStorage,
	   allowSQLnull,
	   inlineIfPossible,
	   externalFormat,
	   extFormatLen)
{
  setCharSet(CharInfo::ISO88591); //lob handle can only be in this format
}

NAType *SQLClob::newCopy(NAMemory* h) const
{ return new(h) SQLClob(*this,h); }

// -----------------------------------------------------------------------
// Type synthesis for binary operators
// -----------------------------------------------------------------------
const NAType* SQLClob::synthesizeType(NATypeSynthRuleEnum synthRule,
				      const NAType& operand1,
				      const NAType& operand2,
				      CollHeap* h,
				      UInt32 *flags) const
{
  if (operand1.getFSDatatype() != REC_CLOB ||
      operand2.getFSDatatype() != REC_CLOB)
    return NULL;
  
  SQLClob& op1 = (SQLClob &) operand1;
  SQLClob& op2 = (SQLClob &) operand2;

  NABoolean null = op1.supportsSQLnull() OR op2.supportsSQLnull();

  if (synthRule == SYNTH_RULE_UNION)
    {
      return new(h) SQLClob(h, MAXOF(op1.getLobLength(), op2.getLobLength()),
			    op1.getLobStorage(),
			    null);
    }

  return NULL;
}

// ----------------------------------------------------------------------
// class SQLBinaryString
// ----------------------------------------------------------------------
SQLBinaryString::SQLBinaryString(NAMemory *h,
                                 Lng32 maxLen,
                                 NABoolean allowSQLnull,
                                 NABoolean varLenFlag
		 )
     : CharType(h, varLenFlag ? LiteralVARCHAR : LiteralCHAR,
		maxLen, 1,
		FALSE, allowSQLnull, FALSE, FALSE,
		varLenFlag, 
                CharInfo::BINARY,
                CharInfo::DefaultCollation, CharInfo::COERCIBLE,
		CharInfo::UnknownCharSet)
{}

//  encoding of the max char value
double SQLBinaryString::getMaxValue() const
{
  EncodedValue dummyVal(0.0);

  double encodedval = dummyVal.minMaxValue(this, FALSE);

  return encodedval;
}

//  encoding of the min char value
double SQLBinaryString::getMinValue() const
{
  EncodedValue dummyVal(0.0);

  double encodedval = dummyVal.minMaxValue(this, TRUE);

  return encodedval;
}

NABoolean SQLBinaryString::isCompatible(const NAType& other, UInt32 * flags) const
{
  return NAType::isCompatible(other, flags);
}

NAString SQLBinaryString::getTypeSQLname(NABoolean terse) const
{
  char  lenbuf[30];
  char* sp = lenbuf;

  NAString rName(isVaryingLen() ? "VARBINARY" : "BINARY");

  sprintf(sp, "(%d) ", getStrCharLimit());
  rName += sp;

  if (! terse)
    getTypeSQLnull(rName, terse);

  return rName;
}

// -----------------------------------------------------------------------
// Type synthesis for binary operators
// -----------------------------------------------------------------------
const NAType* SQLBinaryString::synthesizeType(NATypeSynthRuleEnum synthRule,
                                            const NAType& operand1,
                                            const NAType& operand2,
                                            CollHeap* h,
                                            UInt32 *flags) const
{
  //
  // If the second operand's type synthesis rules have higher precedence than
  // this operand's rules, use the second operand's rules.
  //
  if (operand2.getSynthesisPrecedence() > getSynthesisPrecedence())
    return operand2.synthesizeType(synthRule, operand1, operand2, h, flags);
 
  Lng32 res_nominalSize = 0;
  NABoolean makeTypeVarchar = FALSE;

  switch (synthRule) {
  case SYNTH_RULE_UNION:
    res_nominalSize = MAXOF( operand1.getNominalSize(), operand2.getNominalSize());
    break;
  case SYNTH_RULE_CONCAT:
    res_nominalSize = operand1.getNominalSize() + operand2.getNominalSize();
    break;
  default:
    return NULL;
  }

  NABoolean null = operand1.supportsSQLnull() OR operand2.supportsSQLnull();
  if (DFS2REC::isAnyVarChar(operand1.getFSDatatype()) OR
      DFS2REC::isAnyVarChar(operand2.getFSDatatype()))
    makeTypeVarchar = TRUE;
  
  return new(h) SQLBinaryString(h, res_nominalSize, null, makeTypeVarchar);
} // synthesizeType()

void SQLBinaryString::minRepresentableValue(void* bufPtr, Lng32* bufLen,
                                            NAString ** stringLiteral,
                                            CollHeap* h) const
{
  minMaxRepresentableValue(bufPtr, bufLen, stringLiteral, FALSE, h);
}

void SQLBinaryString::maxRepresentableValue(void* bufPtr, Lng32* bufLen,
                                            NAString ** stringLiteral,
                                            CollHeap* h) const
{
  minMaxRepresentableValue(bufPtr, bufLen, stringLiteral, TRUE, h);
}
