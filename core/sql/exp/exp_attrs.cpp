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


#include "exp_stdh.h"
#include "exp_attrs.h"
#include "exp_clause_derived.h"
#include "exp_bignum.h"
#include "str.h"
#include "NLSConversion.h"

Int32 Attributes::getStorageLength(){return -1;};
Int32 Attributes::getDefaultValueStorageLength(){return -1;};
Int32 Attributes::getLength(){return -1;};
Attributes * Attributes::newCopy(){return 0;};
Attributes * Attributes::newCopy(CollHeap *){return 0;};
void Attributes::copyAttrs(Attributes * /*source_*/){};

Attributes * SimpleType::newCopy()
{
  SimpleType * new_copy = new SimpleType();
  *new_copy = *this;
  return new_copy;
}

Attributes * SimpleType::newCopy(CollHeap * heap)
{
  SimpleType * new_copy = new(heap) SimpleType();
  *new_copy = *this;
  return new_copy;
}

void SimpleType::copyAttrs(Attributes *source_) // copy source attrs to this.
{
  *this = *(SimpleType *)source_;
}

Attributes::Attributes(short complex_type) : 
  NAVersionedObject(AttribAnchorID)
{
  datatype_ = -1;
  
  nullFlag_ = 0;
  nullIndicatorLength_ = 2; // the default
  
  vcIndicatorLength_ = 2; // for now
 
  offset_   = ExpOffsetMax;
  atpindex_ = 0;
  atp_      = 0;
  
  nullIndOffset_  = ExpOffsetMax;
  vcLenIndOffset_ = ExpOffsetMax;
  voaOffset_      = ExpOffsetMax;
  relOffset_      = 0;
  nextAttrIdx_    = ExpOffsetMax;
  rowsetSize_     = 0;
  rowsetInfo_     = 0;
  nullBitIdx_     = -1;

  tdf_ = ExpTupleDesc::UNINITIALIZED_FORMAT;
  alignment_ = 1;    // no alignment
  
  defClass_ = NO_DEFAULT;
  defaultValue_ = 0;
  defaultFieldNum_ = 0x0ffff ; // initialize to an invalid field num

  flags_  = 0;
  flags2_ = 0;

  if (!complex_type)    
    setClassID(SimpleTypeID);
  else {
    flags_ |= COMPLEX_TYPE;

    setClassID(ComplexTypeID);
  }
  setBulkMoveable(TRUE);

  str_pad(fillers_, sizeof(fillers_), '\0');
}

// -----------------------------------------------------------------------
// This method returns the virtual function table pointer for an object
// with the given class ID; used by NAVersionedObject::driveUnpack().
// -----------------------------------------------------------------------
char *Attributes::findVTblPtr(short classID)
{
  char *vtblPtr;
  switch (classID)
    {
    case ShowplanID:
      GetVTblPtr(vtblPtr, ShowplanAttributes);
      break;
    case SimpleTypeID:
      GetVTblPtr(vtblPtr, SimpleType);
      break;
    case BigNumID:
      GetVTblPtr(vtblPtr, BigNum);
      break;
    case ComplexTypeID:
    default:
      GetVTblPtr(vtblPtr, ComplexType);
      break;
    }
  return vtblPtr;
}

Long Attributes::pack(void * space)
{
  defaultValue_.pack(space);

  return NAVersionedObject::pack(space);
}

Lng32 Attributes::unpack(void * base, void * reallocator)
{
  if (defaultValue_.unpack(base)) return -1;
  return NAVersionedObject::unpack(base, reallocator);
}

void Attributes::fixup(Space * /*space*/,
                       char * constantsArea,
		       char * tempsArea,
		       char * persistentArea,
		       short fixupConstsAndTemps,
		       NABoolean spaceCompOnly)
{
  if ((! fixupConstsAndTemps) || (spaceCompOnly))
    return;
  
  char *area;
  
  if ((atp_ == 0) && (atpindex_ == 0))
    area = constantsArea;
  else if((atp_ == 0) && (atpindex_ == 1))
    area = tempsArea;
  else if((atp_ == 1) && (atpindex_ == 1))
    area = persistentArea;
  else
    return;

#if 1
  assert( area == (char *)0 );
    
#else /* FOLLOWING CODE SHOULD NOT BE NEEDED */
  offset_ = (uLong)(area + offset_);
  if (getNullFlag())   // nullable 
     nullIndOffset_ = (ULng32)(area + nullIndOffset_);
  if (getVCIndicatorLength() > 0)
    vcLenIndOffset_ = (ULng32)(area + vcLenIndOffset_);
#endif

}

char * getDatatypeAsString(Int32 datatype, NABoolean extFormat = false )
{
switch (datatype)
  {
  // When you add new datatype in /common/dfs2rec.h, don't
  // forget add new case here. Otherwise, showplan won't display it.

  case REC_BIN8_SIGNED: return extFormat? (char *)"TINYINT SIGNED":(char *)"REC_BIN8_SIGNED";
  case REC_BIN8_UNSIGNED: return extFormat? (char *)"TINYINT UNSIGNED":(char *)"REC_BIN8_UNSIGNED";
  case REC_BIN16_SIGNED: return extFormat? (char *)"SMALLINT SIGNED":(char *)"REC_BIN16_SIGNED";
  case REC_BIN16_UNSIGNED: return extFormat? (char *)"SMALLINT UNSIGNED":(char *)"REC_BIN16_UNSIGNED";
  case REC_BIN32_SIGNED: return extFormat? (char *)"INTEGER SIGNED":(char *)"REC_BIN32_SIGNED";
  case REC_BIN32_UNSIGNED: return extFormat? (char *)"INTEGER UNSIGNED":(char *)"REC_BIN32_UNSIGNED";
  case REC_BIN64_SIGNED: return extFormat? (char *)"LARGEINT":(char *)"REC_BIN64_SIGNED";
  case REC_BIN64_UNSIGNED: return extFormat? (char *)"LARGEINT UNSIGNED":(char *)"REC_BIN64_UNSIGNED";
  case REC_BPINT_UNSIGNED: return extFormat? (char *)"BIT PRECISION INTEGER":(char *)"REC_BPINT_UNSIGNED";

  case REC_IEEE_FLOAT32: return extFormat? (char *)"IEEE FLOAT":(char *)"REC_IEEE_FLOAT32";
  case REC_IEEE_FLOAT64: return extFormat? (char *)"IEEE DOUBLE PRECISION":(char *)"REC_IEEE_FLOAT64";

  case REC_DECIMAL_UNSIGNED: return extFormat? (char *)"DECIMAL UNSIGNED":(char *)"REC_DECIMAL_UNSIGNED";
  case REC_DECIMAL_LS: return extFormat? (char *)"DECIMAL SIGNED":(char *)"REC_DECIMAL_LS";
  case REC_DECIMAL_LSE: return extFormat? (char *)"DECIMAL SIGNED":(char *)"REC_DECIMAL_LSE";
  case REC_NUM_BIG_UNSIGNED: return extFormat? (char *)"NUMERIC":(char *)"REC_NUM_BIG_UNSIGNED";
  case REC_NUM_BIG_SIGNED: return extFormat? (char *)"NUMERIC":(char *)"REC_NUM_BIG_SIGNED";

  case REC_BYTE_F_ASCII: return extFormat? (char *)"CHAR":(char *)"REC_BYTE_F_ASCII";
  case REC_NCHAR_F_UNICODE: return extFormat? (char *)"NCHAR":(char *)"REC_NCHAR_F_UNICODE";

  case REC_BYTE_V_ASCII: return extFormat? (char *)"VARCHAR":(char *)"REC_BYTE_V_ASCII";
  case REC_NCHAR_V_UNICODE: return extFormat? (char *)"NCHAR VARYING":(char *)"REC_NCHAR_V_UNICODE";
  case REC_BYTE_V_ASCII_LONG: return extFormat? (char *)"VARCHAR":(char *)"REC_BYTE_V_ASCII_LONG";

  case REC_BYTE_V_ANSI: return extFormat? (char *)"VARCHAR":(char *)"REC_BYTE_V_ANSI";
  case REC_BYTE_V_ANSI_DOUBLE: return extFormat? (char *)"VARCHAR":(char *)"REC_BYTE_V_ANSI_DOUBLE";

  case REC_SBYTE_LOCALE_F: return extFormat? (char *)"CHAR":(char *)"REC_SBYTE_LOCALE_F";
  case REC_MBYTE_LOCALE_F: return extFormat? (char *)"CHAR":(char *)"REC_MBYTE_LOCALE_F";
  case REC_MBYTE_F_SJIS: return extFormat? (char *)"CHAR":(char *)"REC_MBYTE_F_SJIS";
  case REC_MBYTE_V_SJIS: return extFormat? (char *)"VARCHAR":(char *)"REC_MBYTE_V_SJIS";

  case REC_DATETIME: return extFormat? (char *)"DATETIME":(char *)"REC_DATETIME";

  case REC_INT_YEAR: return extFormat? (char *)"INTERVAL YEAR":(char *)"REC_INT_YEAR";
  case REC_INT_MONTH: return extFormat? (char *)"INTERVAL MONTH":(char *)"REC_INT_MONTH";
  case REC_INT_YEAR_MONTH: return extFormat? (char *)"INTERVAL YEAR TO MONTH":(char *)"REC_INT_YEAR_MONTH";
  case REC_INT_DAY: return extFormat? (char *)"INTERVAL DAY":(char *)"REC_INT_DAY";
  case REC_INT_HOUR: return extFormat? (char *)"INTERVAL HOUR":(char *)"REC_INT_HOUR";
  case REC_INT_DAY_HOUR: return extFormat? (char *)"INTERVAL DAY TO HOUR":(char *)"REC_INT_DAY_HOUR";
  case REC_INT_MINUTE: return extFormat? (char *)"INTERVAL MINUTE":(char *)"REC_INT_MINUTE";
  case REC_INT_HOUR_MINUTE: return extFormat? (char *)"INTERVAL HOUR TO MINUTE":(char *)"REC_INT_HOUR_MINUTE";
  case REC_INT_DAY_MINUTE: return extFormat? (char *)"INTERVAL DAY TO MINUTE":(char *)"REC_INT_DAY_MINUTE";
  case REC_INT_SECOND: return extFormat? (char *)"INTERVAL SECOND":(char *)"REC_INT_SECOND";
  case REC_INT_MINUTE_SECOND: return extFormat? (char *)"INTERVAL MINUTE TO SECOND":(char *)"REC_INT_MINUTE_SECOND";
  case REC_INT_HOUR_SECOND: return extFormat? (char *)"INTERVAL HOUR TO SECOND":(char *)"REC_INT_HOUR_SECOND";
  case REC_INT_DAY_SECOND: return extFormat? (char *)"INTERVAL DAY TO SECOND":(char *)"REC_INT_DAY_SECOND";
  case REC_INT_FRACTION: return extFormat? (char *)"INTERVAL FRACTION":(char *)"REC_INT_FRACTION";
  case REC_BLOB: return extFormat? (char *)"BLOB":(char *)"REC_BLOB";
  case REC_CLOB: return extFormat? (char *)"CLOB":(char *)"REC_CLOB";
  case REC_BOOLEAN: return extFormat ? (char *)"BOOLEAN" : (char *)"REC_BOOLEAN";

  case REC_BINARY_STRING: return extFormat ? (char *)"BINARY" : (char *)"REC_BINARY_STRING";
  case REC_VARBINARY_STRING: return extFormat ? (char *)"VARBINARY" : (char *)"REC_VARBINARY_STRING";

  // When you add new datatype in /common/dfs2rec.h, don't
  // forget add new case here. Otherwise, showplan won't display it.
  default: return extFormat? (char *)"UNKNOWN":(char *)"add datatype in getDatatypeAsString()";
  }
}

ShowplanAttributes::ShowplanAttributes(Lng32 valueId, char * text)
{
  setClassID(ShowplanID);
  valueId_ = valueId;
  if (text)
    {
      if (str_len(text) < sizeof(text_))
	str_cpy(text_, text, str_len(text)+1,'\0');
      else
	{
	  memset(text_, 0, sizeof(text_));
	  str_cpy_all(text_, text, sizeof(text_) - 4);
	  str_cat(text_, " ...",text_);
	}
    }
  else
    text_[0] = 0;

  memset(fillers_, 0, sizeof(fillers_));
}

ShowplanAttributes::~ShowplanAttributes()
{}

Attributes * ShowplanAttributes::newCopy()
{
  ShowplanAttributes * new_copy = new ShowplanAttributes(valueId(), text());
  *new_copy = *this;
  return new_copy;
}

Attributes * ShowplanAttributes::newCopy(CollHeap * heap)
{
  ShowplanAttributes * new_copy = new(heap) ShowplanAttributes(valueId(), text());
  *new_copy = *this;
  return new_copy;
}

void Attributes::displayContents(Space * space, Int32 operandNum,
				 char * constsArea, Attributes * spAttr)
{
  char buf[250];
  char r[15];

  if (operandNum == 0)
    str_cpy(r, " (result)",str_len(" (result)")+1,'\0');
  else
    r[0] = 0;
  
  str_sprintf(buf, "    Operand #%d%s:", operandNum, r);
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  str_sprintf(buf, "      Datatype = %s(%d), Length = %d, Null Flag = %d",
	  getDatatypeAsString(getDatatype()), getDatatype(), getLength(), getNullFlag());
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  if ((getDatatype() == REC_BLOB) ||
      (getDatatype() == REC_CLOB))
    {
     
      
      Int64 ll = getLength();
      if (isLengthInKB())
        ll = ll*1024;
      //      Int64 ll = (Int64)getPrecision() * 1000 + (Int64)getScale();
      str_sprintf(buf, "      LobLength = %ld bytes", ll);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }

  str_sprintf(buf, "      Precision = %d, Scale = %d, Collation = %d, flags_ = %x",
              getPrecision(), getScale(), getCollation(), flags_);

  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  str_cpy(buf, "      Tuple Data Format = ", 
          str_len("      Tuple Data Format = ")+1,'\0');

  switch (getTupleFormat())
    {
    case ExpTupleDesc::UNINITIALIZED_FORMAT: 
      str_cat(buf, "UNINITIALIZED_FORMAT", buf);
      break;

    case ExpTupleDesc::PACKED_FORMAT: 
      str_cat(buf, "PACKED_FORMAT", buf);
      break;

    case ExpTupleDesc::SQLMX_KEY_FORMAT: 
      str_cat(buf, "SQLMX_KEY_FORMAT", buf);
      break;

    case ExpTupleDesc::SQLARK_EXPLODED_FORMAT: 
      str_cat(buf, "SQLARK_EXPLODED_FORMAT", buf);
      break;

    case ExpTupleDesc::SQLMX_FORMAT: 
      str_cat(buf, "SQLMX_FORMAT", buf);
      break;

    case ExpTupleDesc::SQLMX_ALIGNED_FORMAT: 
      str_cat(buf, "SQLMX_ALIGNED_FORMAT", buf);
      break;

    default:
      str_cat(buf, "Unrecognized format", buf);
      break;

    } // switch tuple format

  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  if (isAddedCol())
    {
      str_sprintf(buf, "      DefaultFieldNum = %d",getDefaultFieldNum());
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
    
  char constOrTemp[150];
  if ((getAtp()) == 0 && (getAtpIndex() == 0))
    {
      str_cpy(constOrTemp, " (Constant)",
              str_len(" (Constant)")+1,'\0');
    }
  else if ((getAtp() == 0) && (getAtpIndex() == 1))
    str_cpy(constOrTemp, " (Temporary)",
            str_len(" (Temporary)")+1,'\0');
  else if ((getAtp() == 1) && (getAtpIndex() == 1))
    str_cpy(constOrTemp, " (Persistent)",
            str_len(" (Persistent)")+1,'\0');
  else if (getAtpIndex() == 0)
    str_cpy(constOrTemp, " !!!ERROR!!! - Invalid (Atp,AtpIndex)",
            str_len(" !!!ERROR!!! - Invalid (Atp,AtpIndex)")+1,'\0');
  else
    str_cpy(constOrTemp, " ", str_len(" ")+1,'\0');

  str_sprintf(buf, "      Atp = %d, AtpIndex = %d%s",
	  getAtp(), getAtpIndex(), constOrTemp);
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  str_sprintf(buf, "      Offset = %d, NullIndOffset = %d, VClenIndOffset = %d",
	  (getOffset() == ExpOffsetMax ? -1 : (Lng32)getOffset()),
	  (getNullIndOffset() == ExpOffsetMax ?  -1 : getNullIndOffset()),
	  (getVCLenIndOffset() == ExpOffsetMax ? -1 : getVCLenIndOffset()));
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  if ((getTupleFormat() == ExpTupleDesc::SQLMX_FORMAT) ||
      (getTupleFormat() == ExpTupleDesc::SQLMX_ALIGNED_FORMAT))
    {
      str_sprintf(buf, "      RelOffset = %d, VoaOffset = %d, NullBitIdx = %d",
                  getRelOffset(),
                  (getVoaOffset() == ExpOffsetMax ? -1 : getVoaOffset()),
                  getNullBitIndex());
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }

  str_sprintf(buf, "      NullIndLength = %d, VClenIndLength = %d",
	      getNullIndicatorLength(), getVCIndicatorLength());
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  if ((getRowsetSize() > 0) ||
      (getRowsetInfo()))
    {
      str_sprintf(buf, "      rowsetSize_ = %d, rowsetInfo_ = %x",
		  getRowsetSize(), getRowsetInfo());
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
  
  if (spAttr)
    {
      str_sprintf(buf, "      ValueId = %d",
	      ((ShowplanAttributes *)spAttr)->valueId());
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      str_sprintf(buf, "      Text = %s",
	      (((ShowplanAttributes *)spAttr)->text() ? 
	       ((ShowplanAttributes *)spAttr)->text() : ""));
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
}

ExpDatetime * 
Attributes::castToExpDatetime()
{
  return NULL;
} 

// ---------------------------------------------------------------------
// Method for comparing if two Attributes are equal.
// ---------------------------------------------------------------------
NABoolean Attributes::operator==(const Attributes& other) const
{
  Attributes thisAttr  = (Attributes&)(*this); // to make 'this' a non-const
  Attributes otherAttr = (Attributes&)other;

  if (datatype_ == otherAttr.datatype_ &&
      nullFlag_ == otherAttr.nullFlag_ &&
      defClass_ == otherAttr.defClass_ &&
      (thisAttr.upshift() == otherAttr.upshift()) &&
      tdf_ == otherAttr.tdf_ &&
      ( (alignment_ > 0 && otherAttr.alignment_ > 0) 
	? (alignment_ == otherAttr.alignment_ ) : TRUE ) &&
      ( (nullFlag_ != 0) 
	? (nullIndicatorLength_ == otherAttr.nullIndicatorLength_ &&
	   vcIndicatorLength_ == otherAttr.vcIndicatorLength_ ) : TRUE)
      )
    return TRUE;
  else
    return FALSE;
}

NABoolean SimpleType::operator==(const Attributes& other) const
{
  if (Attributes::operator==(other))
    {
      SimpleType thisAttr  = (SimpleType&)(*this);
      SimpleType otherAttr = (SimpleType&)other;

      if (length_ == otherAttr.length_ &&
	  precision_ == otherAttr.precision_ &&
	  (DFS2REC::isAnyCharacter(thisAttr.getDatatype())
	   ? (thisAttr.getCharSet() == otherAttr.getCharSet())
	   : (scale_ == otherAttr.scale_)) &&
	  ((thisAttr.getDefaultClass() == DEFAULT_NULL || 
	    thisAttr.getDefaultClass() == NO_DEFAULT ||
	    thisAttr.getDefaultClass() == DEFAULT_UUID ||
	    thisAttr.getDefaultClass() == DEFAULT_CURRENT_UT ||
	    thisAttr.getDefaultClass() == DEFAULT_CURRENT) ||
	   (thisAttr.getDefaultValue() && otherAttr.getDefaultValue() &&
	    (str_cmp(thisAttr.getDefaultValue(), 
		     otherAttr.getDefaultValue(),
		     (thisAttr.getNullIndicatorLength() +  
		      thisAttr.getVCIndicatorLength() + 
		      length_) ) == 0)
	    ) )
	  )
	return TRUE;
      else
	return FALSE;
    }
  else
    return FALSE;
}

NABoolean ComplexType::operator==(const Attributes& other) const
{
  if (Attributes::operator==(other))
    {
      ComplexType &thisAttr  = (ComplexType&)(*this);
      ComplexType &otherAttr = (ComplexType&)other;

      if ((thisAttr.complexDatatype_ == otherAttr.complexDatatype_) &&
	  (thisAttr.getLength() == otherAttr.getLength()) &&
	  (thisAttr.getPrecision() == otherAttr.getPrecision()) &&
	  (thisAttr.getScale() == otherAttr.getScale()))
	return TRUE;
      else
	return FALSE;
    }
  else
    return FALSE;
}

NABoolean isAddedColumn(Attributes * srcAttr,
                        NABoolean tableHasAddedColumns,
                        NABoolean tableHasVariableColumns,
                        ULng32 offsetOfFirstFixedFieldInRec,
                        ULng32 recordLength,
                        char * recordPtr
                        )
{
   // Check if this is an added column.
   // There are 4 cases to check for:
   // (1) The column is a variable column and the offset to the first
   //     fixed field is greater than the offset
   //     to VOA entry for this column.
   // (2) This is a fixed column and its offset is greater than the
   //     length of the row in, and there are no varchar
   //     columns in the roq
   // (3) This is a fixed column and its offset is greater than
   //     the offset for the first variable length column in the audit
   //     row image.
   // (4) This is a fixed column, but there are no previous fixed fields
   //     in the row
   if (((srcAttr->isAddedCol()) || (tableHasAddedColumns)) &&
       (((srcAttr->getVCIndicatorLength() > 0) &&
         (srcAttr->getVoaOffset() >= offsetOfFirstFixedFieldInRec)) ||
        (((!tableHasVariableColumns) &&
          ((offsetOfFirstFixedFieldInRec + srcAttr->getRelOffset()) >=
                  recordLength)) ||
         ((tableHasVariableColumns) &&
          ((offsetOfFirstFixedFieldInRec + srcAttr->getRelOffset()) >=
            *(ULng32 *)(recordPtr + sizeof(Lng32)))) ||
         (tableHasVariableColumns &&
          (srcAttr->getVCIndicatorLength() == 0) &&
          ((offsetOfFirstFixedFieldInRec == sizeof(Lng32)) ||
           (offsetOfFirstFixedFieldInRec == 0))))))
      return TRUE;

   return FALSE;
}

// Return number of bytes of the first character in buf. SJIS should be 1 or
// 2. UTF8 should be 1 to 4 (byte). UCS2 is 1 (we use wchar for UCS2 data. So
// it is 1, not 2).
Int32 Attributes::getFirstCharLength(const char              *buf,
                                           Int32             buflen,
                                           CharInfo::CharSet cs)
{
  UInt32 UCS4value;
  UInt32 firstCharLenInBuf;

  // The buffer explain send to string function includes character 0,
  // treat it as single byte character.
  if( cs == CharInfo::ISO88591 ||
      cs == CharInfo::UCS2 ||
      cs == CharInfo::BINARY ||
      buf[0] == 0)
  {
      firstCharLenInBuf = 1;
  }
  else
  {
    firstCharLenInBuf =
      LocaleCharToUCS4(buf, buflen, &UCS4value, convertCharsetEnum(cs));
  }
  return firstCharLenInBuf;
}

// Find the number of character at the offset in buf.
Int32 Attributes::convertOffsetToChar(const char        *buf,
                                      Int32             offset, 
                                      CharInfo::CharSet cs)
{
  if (cs == CharInfo::ISO88591 || cs == CharInfo::UCS2 || cs == CharInfo::BINARY)
     return(offset);

  Int32 firstCharLenInBuf;
  UInt32 UCS4value;

  cnv_charset charset = convertCharsetEnum(cs);

  Int32 numberOfChar = 0;
  Int32 i = 0;

  while(i < offset)
  {
    firstCharLenInBuf = LocaleCharToUCS4(&buf[i],
                                         offset - i,
                                         &UCS4value,
                                         charset);
    if(firstCharLenInBuf < 0) return firstCharLenInBuf;

    i += firstCharLenInBuf;
    ++numberOfChar;
  }
  return numberOfChar;
}

// Return number of bytes used by the characters in buf preceding the Nth char.
// Return an error if we encounter a character that is not valid in the cs 
// character set.
Int32 Attributes::convertCharToOffset (const char        *buf,
                                       Int32             numOfChar,
                                       Int32             maxBufLen,
                                       CharInfo::CharSet cs)
{
  if (cs == CharInfo::ISO88591 || cs == CharInfo::UCS2 || cs == CharInfo::BINARY)
     return((numOfChar <= maxBufLen) ? numOfChar - 1 : maxBufLen);

  Int32 firstCharLenInBuf;
  UInt32 UCS4value;

  cnv_charset charset = convertCharsetEnum(cs);

  // Number of character in string functions start from 1, not 0. 1 means
  // the first character in the string. Offset start from 0. The offset of
  // the first character in a string is 0.
  Int32 count = 1;
  Int32 offset = 0;

  while(count < numOfChar && offset < maxBufLen)
  {
    firstCharLenInBuf = LocaleCharToUCS4(&buf[offset],
                                           maxBufLen - offset,
                                           &UCS4value,
                                           charset);

    if(firstCharLenInBuf < 0) return firstCharLenInBuf;

    offset += firstCharLenInBuf;
    ++count;
  }
  return offset;
}

Int32 Attributes::getCharLengthInBuf
    (const char        *buf,
     const char        *endOfBuf,
     char              *charLengthInBuf,
     CharInfo::CharSet cs)
{
  Int32 numberOfCharacterInBuf;

  if (cs == CharInfo::ISO88591 || cs == CharInfo::BINARY || cs == CharInfo::UCS2)
  {
    numberOfCharacterInBuf = endOfBuf - buf;
    if(charLengthInBuf != NULL)
    {
      for(Int32 i = 0; i < numberOfCharacterInBuf; i ++)
        charLengthInBuf[i] = 1;
    }
    return numberOfCharacterInBuf;
  }

  Int32 firstCharLenInBuf;
  UInt32 UCS4value;
  cnv_charset charset = convertCharsetEnum(cs);

  // For SJIS, it is impossible to get the length of the last character
  // from right. Scan the string from the beginning and save the vales to
  // an array.
  // For example: SJIS string (x'5182828251') and (x'51828251'), the last
  // character in the first string is 2-byte, double-byte "2". The last
  // character in the second string is 1 byte, single-byte "Q".

  size_t len = endOfBuf - buf;
  numberOfCharacterInBuf = 0;

  while(len > 0)
  {
    firstCharLenInBuf = LocaleCharToUCS4 (buf, len, &UCS4value, charset);

    if (firstCharLenInBuf <= 0)
      return CNV_ERR_INVALID_CHAR;
    else
    {
      if(charLengthInBuf != NULL)
      {
        charLengthInBuf[numberOfCharacterInBuf] = (char)firstCharLenInBuf;
      }

      numberOfCharacterInBuf++;
      buf += firstCharLenInBuf;
      len -= firstCharLenInBuf;
    }
  }
  return numberOfCharacterInBuf;
}

Int32 Attributes::trimFillerSpaces
     (const char* buf, Int32 precision, Int32 maxBufLen, CharInfo::CharSet cs)
{
#if 0 /* All callers have already checked this for speed reasons. May need this if SJIS supported later. */
  if (cs == CharInfo::UTF8)
#endif
  {
     if ( precision > 0 )
     {
        Int32 endOff = lightValidateUTF8Str(buf, maxBufLen, precision, FALSE);
        if (endOff >= 0)   // If no error
            return (endOff);
        // else bad UTF8 chars will get detected later by caller
     }
  }
  return (maxBufLen);
}
