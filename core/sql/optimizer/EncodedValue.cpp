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
 * File:         EncodedValue.C
 * Description:  Encoded Value class contains the encoded format for (possibly
 *               multi-attribute) values, used for storing histogram values.
 * Created:      June 7, 1995
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#include "Sqlcomp.h"	       /* must be first included file */
#include "EncodedValue.h"
#include "ItemColRef.h"
#include "parser.h"
#include "str.h"
#include <wchar.h>
#include "NLSConversion.h"
#include "hs_const.h"          /* for HS_MAX_BOUNDARY_LEN */
#include "wstr.h"
#include <exp_function.h>


#include <ostream>

#include "CompException.h"

#include "exp_function.h"

const EncodedValue NULL_ENCODEDVALUE (WIDE_("(NULL)")) ;
const EncodedValue UNINIT_ENCODEDVALUE (_ENCODEDVALUE_UNINIT_VALUE_) ;

//
// NOTE: This code came from encodeString() in .../common/CharType.cpp and
// was moved here because 
// (1) this coded needed to start calling ex_function_encode::encodeCollationKey()
// which is in the Executor and therefore not callable from the common library
// code and 
// (2) the places in this source file (EncodedValue.cpp) that called
// CharType.cpp's encodeString() routine were the ONLY places that needed 
// that functionality, so the encodeString() logic more properly belonged here.
//
double EncVal_encodeString(const char * str, Lng32 strLen, CharType *cType)
{
  double result;

  // the blank-padded first 8 bytes of string <str>
  unsigned char stringValues[8];

  // two long integers that, when concatenated, contain the result
  // as a 64 bit integer
  ULng32 hiResult = 0;
  ULng32 loResult = 0;

  // ---------------------------------------------------------------------
  // the encode function performs blank-padding just as the comparison
  // operator does, in order to make encoded values of all character data
  // types compatible
  // ---------------------------------------------------------------------
  // for the NCHAR prototype we want to treat nchars just like chars.
  // redo this when we do the real thing.
  if (cType->getCharSet() != CharInfo::UNICODE ) // ... used to be ... if (cType->getBytesPerChar() == 1)
    {
      // fill "stringValues" with 8 bit blanks
      unsigned char blank = (unsigned char) cType->getBlankCharacterValue();

      for (Int32 i = 0; i < 8; i++)
	stringValues[i] = blank;

    } // bytes per char == 1
  else
    {
      ComASSERT(cType->getBytesPerChar() == SQL_DBCHAR_SIZE);

      // 2/19/98: copy the blank char as byte string to stringValues[]
      // so that endian-ness will not be an issue.

      unsigned short blank = (unsigned short) cType->getBlankCharacterValue();
      for (Int32 i = 0; i < 4; i++)
	{
          str_cpy_all((char*)&stringValues[SQL_DBCHAR_SIZE*i],
		      (char*)&blank, sizeof(blank));
	}

    } // bytes per char != 1

  // NOTE: Caseinsensitive is not supported with Czech Collation
  if ((cType->isCaseinsensitive()) &&
      (NOT cType->isUpshifted()) && cType->getBytesPerChar() == 1)
    {
      // upshift and copy the first 8 bytes of str
      // (or all bytes if it is shorter than 8)
      str_cpy_convert((char *) stringValues, (char *)str, MINOF(strLen,8), -1);
    }
  else
    {
#define MAXPASSES 4
#define ENCKEYBUFLEN  (8 * (MAXPASSES+1) + 2) //Ensure Temp buffer big enough!
      UInt8 encodeKeyBuf1[ ENCKEYBUFLEN ];     //Temp buffer 
      UInt8 * tmpstr = (UInt8 *)str;
      Int32    tmpLen = strLen;
      CharInfo::Collation collation = cType->getCollation();
      if (collation != CharInfo::DefaultCollation) {
          memset(encodeKeyBuf1, 0, ENCKEYBUFLEN);
          Int16 nPasses = CollationInfo::getCollationNPasses(collation);
          ex_function_encode::encodeCollationKey( (const UInt8 *)str,
                                                  MINOF(strLen,8),
                                                  encodeKeyBuf1,
                                                  ENCKEYBUFLEN,
                                                  nPasses,
                                                  collation,
                                                  FALSE /*remove trailing spaces? */
                                                );
          tmpstr = encodeKeyBuf1;
          tmpLen = ENCKEYBUFLEN;
      }
      // copy the first 8 bytes of str (or all bytes if it is shorter than 8)
      str_cpy_all((char *) stringValues, (char *)tmpstr, MINOF(tmpLen,8));
    }

#ifdef NA_LITTLE_ENDIAN
  if (cType->getBytesPerChar() == SQL_DBCHAR_SIZE)
  {
      wc_swap_bytes((NAWchar*)stringValues, 4);
  }
#endif

  // leave the upper 12 bits (mask = 0xfff00000) of hiResult empty
  //
  // We only use the 52 fraction bits of the floating point double so that the round trip conversion
  // from decimal to double and then from double to decimal results in exactly the original decimal.
  // From Wiki "If a decimal string with at most 15 significant digits is converted to IEEE 754 double
  // precision representation and then converted back to a string with the same number of significant digits,
  // then the final string should match the original"

   // 8 bits of the first character
   hiResult += (ULng32) stringValues[0] << 12; // char 0
   hiResult += (ULng32) stringValues[1] <<  4; // char 1
   hiResult += (ULng32) stringValues[2] >>  4; // 4 bits of char 2

   loResult += (ULng32) stringValues[2] << 28; // 4 bits of char 2
   loResult += (ULng32) stringValues[3] << 20; // char 3
   loResult += (ULng32) stringValues[4] << 12; // char 4
   loResult += (ULng32) stringValues[5] <<  4; // char 5
   loResult += (ULng32) stringValues[6] >>  4; // 4 bits of char 6



  // combine the two 32 bit integers to a floating point number
  // (2**32 * hiResult + loResult)
  result = hiResult * .4294967296E10 + loResult;

  return result;
}

double EncVal_Char_encode(const char * theValue, const NAType *theType)
{
   CharType *cType = (CharType *)theType;
   char *charBufPtr = (char *) theValue;

   if (cType->supportsSQLnull())
      charBufPtr += cType->getSQLnullHdrSize();
   if (cType->isVaryingLen())
   {
      // copy the actual length of the string into an aligned variable
      short actualLenShort;
      Lng32 actualLen;

      //      ComASSERT(sizeof(short) == cType->getVarLenHdrSize());
      if (cType->getVarLenHdrSize() == sizeof(short))
	{
	  str_cpy_all((char *) &actualLenShort, charBufPtr, sizeof(short));
	  actualLen = actualLenShort;
	}
      else
	str_cpy_all((char *) &actualLen, charBufPtr, sizeof(Lng32));
	
      return EncVal_encodeString(&charBufPtr[cType->getVarLenHdrSize()],
                                 actualLen, cType);
   }
   else // Fixed length
   {
      return EncVal_encodeString(charBufPtr, cType->getNominalSize(), cType);
   }
}
// -----------------------------------------------------------------------
//  Methods for NormValue
// -----------------------------------------------------------------------
NormValue::NormValue(const ConstValue * constant, NABoolean negate)
{
  CMPASSERT(constant);				// Genesis 10-980626-6634
  const NAType * theType = constant->getType();
  void * theValue = constant->getConstValue();

  NABoolean reset = FALSE;
  if ((CmpCommon::getDefault(::MODE_SPECIAL_1) == DF_ON) &&
      (theType->getTypeQualifier() == NA_CHARACTER_TYPE))
    {
      CharType *cType = (CharType *)theType;
      if((! cType->isCaseinsensitive()) &&
	 (cType->getBytesPerChar() == 1))
        {
          cType->setCaseinsensitive(TRUE);
          reset = TRUE;
        }
    }
    
  if (theType->getTypeQualifier() == NA_CHARACTER_TYPE)
    {
      theValue_ = EncVal_Char_encode((char *)theValue, theType);
    }
  else theValue_ = theType->encode (theValue);
  
  if (reset) 
    {
      CharType *cType = (CharType *)theType;
      cType->setCaseinsensitive(FALSE);
    }
  
  if (negate)
    theValue_ *= -1;

  isNullFlag_ = FALSE;
}

void NormValue::display (FILE *f, const char * prefix, const char * suffix,
                         CollHeap *c, char *buf) const
{
  Space * space = (Space *)c;
  char mybuf[1000];

  if (isNullFlag_)
  {
    snprintf(mybuf, sizeof(mybuf), "%sNULL%s", prefix, suffix);
    PRINTIT(f, c, space, buf, mybuf);
  }
  else if (theValue_ == _ENCODEDVALUE_UNINIT_VALUE_ )
  {
    snprintf(mybuf, sizeof(mybuf), "%szINIT%s", prefix, suffix);
    PRINTIT(f, c, space, buf, mybuf);
  }
  else
  {
    snprintf(mybuf, sizeof(mybuf), "%s%.4f%s", prefix, theValue_, suffix);
    PRINTIT(f, c, space, buf, mybuf);
  }
}

NormValueList::NormValueList(const NormValueList & nvl, NAMemory *h)
  :NAArray<NormValue>(h ? h : CmpCommon::statementHeap(),nvl.entries()),
  heap_(h ? h : CmpCommon::statementHeap())
{
  for(CollIndex i=0; i<nvl.entries(); i++)
  {
    NormValue newVal = nvl[i];
    insertAt(i, newVal);
  }
}

NormValueList & NormValueList::operator= (const NormValueList& other)
{
  if (this != &other)
  {
    for (CollIndex i = 0; i < other.entries(); i++)
    {
      NormValue newVal = other[i];
      insertAt(i, newVal);
    }
  }
  return *this;
}

NormValueList & NormValueList::operator+ (const NormValueList& other)
{
  CMPASSERT(this->entries() == other.entries());

  for (CollIndex i = 0; i < other.entries(); i++)
  {
     double newVal = this->at(i).getValue() + other[i].getValue();
     this->at(i).setValue(newVal);
  }

  return *this;
}

NormValueList & NormValueList::operator- (const NormValueList& other)
{
  CMPASSERT(this->entries() == other.entries());

  for (CollIndex i = 0; i < other.entries(); i++)
  {
     double newVal = this->at(i).getValue() - other[i].getValue();
     this->at(i).setValue(newVal);
  }

  return *this;
}

NormValueList & NormValueList::operator* (const double factor)
{
  for (CollIndex i = 0; i < entries(); i++)
  {
     double newVal = this->at(i).getValue() * factor;
     this->at(i).setValue(newVal);
  }

  return *this;
}

void NormValueList::round ()
{
  for (CollIndex i = 0; i < entries(); i++)
  {
     CostScalar newVal = this->at(i).getValue();
     newVal = newVal.round();
     this->at(i).setValue(newVal.getValue());
  }
}

//
// A help data structure to sort the columns by column positions in descending order. 
// Used by MCSB to compute the run-time hash function correctly. 
// See HashDistPartHash::preCodeGen() for detail on the part key column list is converted
// to the partExpression. 
//
struct pos_index  
{
    void setPos(Int32 x) { pos_ = x; }
    void setIdx(Int32 x) { idx_ = x; }

    Int32 getPos() { return pos_; }
    Int32 getIdx() { return idx_; }

    Int32 pos_;
    Int32 idx_;
};

Int32 comparePosIdx(const void* p1, const void* p2)
{
  pos_index* pi1 = (pos_index*)p1;
  pos_index* pi2 = (pos_index*)p2;

  if ( pi1->pos_ == pi2->pos_ )
    return 0;
  if ( pi1->pos_ > pi2->pos_ )
    return -1;
  else 
    return 1;
}



// A helper method to compute a run-time hash value for a composite
// value.
// 
// The ith component of the composite value is represented as follows,
// depending on the type of the component value.
//
// SQL Integer types:  in NormValue_[i]
// NUMERIC types:      in NormValue_[i]
// CHAR types:         in cvPtrs[i]

UInt32 EncodedValue::computeRunTimeHashValue(const NAColumnArray & colArray, const NAWchar * boundary, ConstValue* cvPtrs[])
{
  UInt32 hashValueForCurCol = 0, hashValueForColGrp = 0;
  NABoolean useHashValue = TRUE;

  const NormValueList *nvl = getValueList();
  CollIndex colEntries = nvl->entries();

  // If not all columns of a multi-column group fit in the boundary value,
  // return 0 as the final hash value.
  if (colArray.entries() != colEntries)
     return 0;

  // Re-arrage the colArray in the descending order of col positions. The result 
  // is saved in a pos_index array below
  pos_index* posIndexArray = 
         (pos_index*)(new (STMTHEAP) char[sizeof(pos_index)*colArray.entries()]);


  for(CollIndex k=0; k<colEntries; k++) {
     posIndexArray[k].setPos(colArray[k]->getPosition());
     posIndexArray[k].setIdx(k);
  }

  // Sort based on the column position numbers.
  qsort(posIndexArray, colArray.entries(), sizeof(pos_index), comparePosIdx);

  for(CollIndex j=0; j<colEntries; j++)
  {
    // Indirectly find out the corresponding actual NAColumn and constantValue, for the jth
    // column position in decending order.  
    CollIndex i = posIndexArray[j].getIdx();

    const NAType * type = colArray[i]->getType();

    useHashValue = type->useHashRepresentation();

    NormValue nV = nvl->at(i);

    hashValueForCurCol = 0;

    if(nV.isNull()) {

      hashValueForCurCol = ExHDPHash::nullHashValue;

    } else 
    if( !useHashValue ) 
    {
      char data[10]; Int32 len = 0;
      UInt32 flags = ExHDPHash::NO_FLAGS;

      EncodedValue ev(nV.getValue());

      ev.outputToBufferToComputeRTHash(type, data, len, flags);
      hashValueForCurCol = ExHDPHash::hash(data, flags, len);

    } else if (type->getTypeQualifier() == NA_NUMERIC_TYPE && 
             type->getTypeName() == LiteralNumeric) 
    {
      hashValueForCurCol = nV.computeHashForNumeric((SQLNumeric*)type);
    } else if ( type->getTypeQualifier() == NA_CHARACTER_TYPE && cvPtrs ) {

      ConstValue* cv = cvPtrs[i];

     if ( cv ) {

         if ( type->getTypeQualifier() == NA_CHARACTER_TYPE &&
              ((CharType*)type)->isCaseinsensitive() &&
              ((CharType*)type)->getCharSet() != CharInfo::UNICODE
            )
            cv = cv->toUpper(HISTHEAP);

         hashValueForCurCol = cv->computeHashValue(*type);
      }

    } else {
      // arbitrarily make up a hash value. SB code will not form a SB
      // plan when one of the join columns is of a SQL type (i.e. a float)
      // not capable of being SB processed.
      hashValueForCurCol = 0;
    }

    //
    // Combine the current hash value into the final hash value
    //
    if(j == 0) // First time, directly copy the hash value to the result
      hashValueForColGrp = hashValueForCurCol;
    else
    {
      // The following code is from ExHDPHashComb::eval() method in exp_function.cpp
      // and shouldnt be changed.
      hashValueForColGrp = ((hashValueForColGrp << 1) | (hashValueForColGrp >> 31));
      hashValueForColGrp = hashValueForColGrp ^ hashValueForCurCol;
    }
  }
  NADELETEBASIC(posIndexArray, STMTHEAP);

  return  hashValueForColGrp;
}

void
EncodedValue::outputToBufferToComputeRTHash(
         const NAType* naType, 
         char* data, // output buffer to hold the data to be hashed
         Int32& len, // length of the data
         UInt32& flags // flags to be used during hash
         ) const
{
      double x = getDblValue();
      flags = ExHDPHash::NO_FLAGS;
      switch (naType->getFSDatatype()) {
        case REC_BIN8_UNSIGNED:
        case REC_BOOLEAN:
           len = 1;
           { UInt8 y = (UInt8)x; memcpy(data, &y, len); }
           break;
        case REC_BIN8_SIGNED:
           len = 1;
           { Int8 y = (Int8)x; memcpy(data, &y, len); }
           break;
        case REC_BIN16_UNSIGNED:
           len = 2;
           flags =ExHDPHash::SWAP_TWO;
           { unsigned short y = (unsigned short)x; memcpy(data, &y, len); }
           break;
        case REC_BIN16_SIGNED:
           len = 2;
           flags =ExHDPHash::SWAP_TWO;
           { short y = (short)x; memcpy(data, &y, len); }
           break;
         case REC_BIN32_UNSIGNED:
           len = 4;
           flags =ExHDPHash::SWAP_FOUR;
           { UInt32 y = (UInt32)x; memcpy(data, &y, len); }
           break;
        case REC_BIN32_SIGNED:
           len = 4;
           flags =ExHDPHash::SWAP_FOUR;
           { Int32 y = (Int32)x; memcpy(data, &y, len); }
           break;
        case REC_BIN64_SIGNED:
           len = 8;
           flags =ExHDPHash::SWAP_EIGHT;
           { Int64 y = (Int64)x; len = 8; memcpy(data, &y, len); }
           break;
        case REC_BIN64_UNSIGNED:
           len = 8;
           flags =ExHDPHash::SWAP_EIGHT;
           { UInt64 y = (UInt64)x; len = 8; memcpy(data, &y, len); }
           break;
        default:
          len = 0; // For column types not supported by SB, we just
                   // skip the column value here.
       }
}

// -----------------------------------------------------------------------
//  Methods for EncodedValue
// -----------------------------------------------------------------------

// It's very important to make sure that something that's not really NULL
// doesn't have a double-encoding that's so close to NULL that it might
// be sorted == or > than NULL (!)

void EncodedValue::enforceNullMechanism()
{
  if ( (value_.getValue() > _ENCODEDVALUE_CLOSE_TO_NULL_) AND NOT value_.isNull() )
    value_.setValue (_ENCODEDVALUE_CLOSE_TO_NULL_) ;
}

void EncodedValue::addANormValue(EncodedValue * thisPtr,
				 ItemExpr * exprPtr,
				 NABoolean negate)
{
  if(exprPtr->doesExprEvaluateToConstant(TRUE))
    {
  	NABoolean neg = FALSE;
  	ConstValue * cv = exprPtr->castToConstValue(neg);
  	NormValue value (cv, neg); // *might* generate NULL
  	if ( value.getValue() == _ENCODEDVALUE_NULL_VALUE_ )
  	   value.setNull() ; // explicitly set NULL flag
  	thisPtr->setValue (value) ;
  	return;
    }
    ABORT("EncodedValue::addANormValue encountered an illegal expression");
  return;

} // EncodedValue::addANormValue()

double 
EncodedValue::minMaxValue(const NAType *pType, const NABoolean wantMin)
{
  Lng32 len = pType->getTotalSize();
  char *buf = new char[len + 2+2];
  char *pt = buf;

  if (pType->supportsSQLnullPhysical())
    {
      Lng32 nullHdrSize = pType->getSQLnullHdrSize();
      pt = &buf[nullHdrSize];
      buf[0] = buf[1] = '\0';
      len -= nullHdrSize;
    }



  if (wantMin)
    pType->minRepresentableValue(pt, &len, NULL, CmpCommon::statementHeap());
  else
    pType->maxRepresentableValue(pt, &len, NULL, CmpCommon::statementHeap());

  double dblVal ;
  if (pType->getTypeQualifier() == NA_CHARACTER_TYPE)
       dblVal = EncVal_Char_encode(buf, pType);
  else
       dblVal = pType->encode(buf);

  delete [] buf;
  return dblVal;
}

EncodedValue::EncodedValue (const EncodedValue & other, NAMemory * h)
: valueList_(NULL), heap_(h ? h : CmpCommon::statementHeap())
{
  const NormValueList * nvl = other.getValueList();
  if(nvl)
  {
    valueList_ = new (heap_) NormValueList(nvl->entries(), heap_);
    *valueList_ = *nvl;
  }
  else
    this->setValue (other.value_);
}

EncodedValue::EncodedValue (const NormValueList& nvl, NAMemory * h)
: valueList_(NULL), heap_(h ? h : CmpCommon::statementHeap())

{
    valueList_ = new (heap_) NormValueList(nvl.entries(), heap_);
    *valueList_ = nvl;

    this->setValue (nvl[0].getValue());
}

EncodedValue::EncodedValue (double val)
: valueList_(NULL), heap_(HISTHEAP)
{
  NormValue value (val) ;  // will never generate NULL
  this->setValue (value) ; // filters out non-NULLs with NULL double-value
}

EncodedValue::EncodedValue (ItemExpr *expr,
                            NABoolean negate)
: valueList_(NULL), heap_(HISTHEAP)
{
  addANormValue(this,expr,negate);
}

void
EncodedValue::constructorFunction (const NAWchar * theValue,
                                   const NAColumnArray &columns, 
                                   NABoolean okToReportErrors,
                                   ConstValue* cvPtrs[])
{
  // Some notes about error reporting for this function:
  //
  // The error reporting for this function is a little strange
  // and should be re-engineered when we can imagine a better
  // design for it.
  //
  // This function is called from two very different contexts.
  //
  // One is from the constructors of global objects, to provide
  // convenient encoded constants. Being global objects, this
  // call is made as a result of global constructor calls before
  // the C++ main for the process is invoked. As such, we cannot
  // depend on other global objects being constructed. So, for
  // example, we cannot depend on CmpCommon::diags() being
  // initialized, as C++ makes no guarantees about the order in
  // which global objects are created. Too, it does no good to
  // throw a C++ exception as there would be nothing to catch it
  // and process it. So, if an error happens in this code path,
  // we'll simply assert.
  //
  // The other is in the course of histogram processing. Histograms
  // have been read in, and now we want to encode the boundary
  // values in the histogram. These might be stale or corrupted
  // so that condition has to be detected. When detected, this
  // routine raises a warning in CmpCommon::diags(), and then
  // throws a C++ exception.
  //
  // This warning processing has to be done carefully. The way
  // it works is that lower level routines report errors into 
  // CmpCommon::diags(). This routine checks for such errors and
  // if it sees any, it throws them away, replacing them with
  // a warning in CmpCommon::diags(). It then throws a C++
  // exception which is typically caught by HSHistogrmCursor::fetch 
  // (ustat/hs_read.cpp). We use default histograms in that case.
  //
  // Why do we throw away the errors? We do this because of the
  // way the Normalizer handles CmpCommon::diags(). During 
  // synthesise logical properties processing, histograms may
  // be read and processed. (Note: They can be read and processed
  // from other phases as well, such as table analysis.) The
  // Normalizer checks for errors in CmpCommon::diags(), and if
  // found, retries compilation. On the retry, CmpCommon::diags()
  // will be cleared, and the histograms code will simply use
  // a default histogram. So, if there is any error in
  // CmpCommon::diags(), we will lose the histogram warnings
  // generated in this method.
  //
  // Note that if there is already an error in CmpCommon::diags()
  // when this method is called, we'll lose the histogram warnings
  // anyway. Sigh.

  Lng32 mark = okToReportErrors ? CmpCommon::diags()->mark() : -1;

  // Find the first non-blank char.
  const NAWchar *item = theValue;
  while (*item == L' ')
    item++;

  if ( *item != L'(' ) // must be '('
    {
      if (okToReportErrors)
        {
          *CmpCommon::diags() << DgSqlCode(CATALOG_HISTOGRM_HISTINTS_TABLES_CONTAIN_BAD_VALUE)
             << DgWString0(theValue) 
             << DgString1(columns[0]->getFullColRefNameAsAnsiString());
          CmpInternalException("Bad Interval Boundary", __FILE__ , __LINE__).throwException();
        }
      else
        {
          CMPASSERT(FALSE); // developer needs to fix the bug
        } 
    }

  item++;

  NAWchar *next;
  NABoolean boundaryValueTruncated = FALSE;

  const Int32 BOUNDARY_LEN = HS_MAX_BOUNDARY_LEN + 10; // +10 just in case
  NAWchar buf[BOUNDARY_LEN];

  NormValue val;
  double dblVal;

  // If read from cache.
  if (*item == L'=')
    {
#ifndef NDEBUG
      //this is supposedly dead code
      //putting the assert here to see
      //if we ever get to this part of
      //the code
      CMPASSERT(FALSE);
#endif //NDEBUG
      item++;
      switch (*item)
      {
      case L'N':
	{
	  val.setNull(); // NULL flag explicitly set
	  break;
	}
      case L'L':
	{ // generate min as default.
	  dblVal = minMaxValue(columns[0]->getType(), TRUE);
	  val.setValue(dblVal); // will never generate NULL
	  break;
	}
      case L'H':
	{ // generate max as default.
	  dblVal = minMaxValue(columns[0]->getType(), FALSE);
	  val.setValue(dblVal); // will never generate NULL
	  break;
	}
      default:
	{
	  dblVal = na_wcstod(item,NULL);
	  val.setValue(dblVal); // will never generate NULL
	}
      }
      this->setValue (val) ; // filters out non-NULLs with NULL double-value
      return;
    }

  // If read from histogram tables.
  while (*item == L' ')
    item++;
  next = (NAWchar *)item;

  CollIndex entries = columns.entries() ;
  if(entries == 0)
    entries = 1;

  // CMPASSERT (entries == 1) ;
  // this assertion's not true! when reading in MCH's, we have multiple
  // columns --> however, for the EncodedValue we can only encode one of
  // them, so just ignore the others

  for (CollIndex i = 0 ; i < entries; i++)
  {
    // Check if a quoted item first.
    // Need to skip quotes within quotes and find the
    // corresponding right quote.
    if (*next++ == L'\'')
    {
      while ((*next != L'\0') &&
             ((*next++ != L'\'') || (*next++ == L'\'')))
        ;
      next--;
    }

    // Scan for delimiter (either ',' or ')' from next instead of item, because
    // in the case of a string, next has been advanced to the closing quote while
    // item still points to the beginning of the string. Scanning for a comma
    // starting at item may find a comma that is part of the string.
    // Note: In the case of INTERVAL literals, we might have a nasty SECOND(m,n)
    // qualifier at the end. We don't want to mistake a possible comma within such
    // a qualifier for our delimiter, so we have to use na_wcschrSkipOverParenText
    // instead of na_wcschr to search for the comma.
    if ( i == entries-1 OR entries==0 ) // sometimes columns is an empty list
      next = na_wcsrchr(next, L')') ;
    else  // it's an MCH
      {
    	NAWchar* nextSave = next;
        next = na_wcschrSkipOverParenText(next, L',');
        if ( next == NULL )
          {
            // Number of components of boundary value is less than the number of
            // cols in the MCH; they must have not all fit in the max boundary size.
            next = na_wcsrchr(nextSave, L')');
            boundaryValueTruncated = TRUE;
          }
      }

    if ( next == NULL ) // should never happen!
      {
        if (okToReportErrors)
          {
            CmpCommon::diags()->rewind(mark,TRUE); // get rid of any diags we may have added
            *CmpCommon::diags() 
             << DgSqlCode(CATALOG_HISTOGRM_HISTINTS_TABLES_CONTAIN_BAD_VALUE) 
             << DgWString0(theValue) 
             << DgString1(columns[i]->getFullColRefNameAsAnsiString());
            CmpInternalException("Bad Interval Boundary", __FILE__ , __LINE__).throwException();
          }
        else
          {
            CMPASSERT(FALSE);  // developer needs to fix the bug
          }
      }
 
    Lng32 len = BOUNDARY_LEN; 
    Lng32 storageSize = 0;
    Lng32 offset = 0;

    switch(*item)
    {
    case L'>':
      { // generate min as default.
	dblVal = minMaxValue(columns[i]->getType(), TRUE);
	val.setValue(dblVal); // will never generate NULL
        break;
      }
    case L'<':
      { // generate max as default.
	dblVal = minMaxValue(columns[i]->getType(), FALSE);
	val.setValue(dblVal); // will never generate NULL
	break;
      }

    default:
      {
	if ((item[0] == L'N') &&
	    (item[1] == L'U') &&
	    (item[2] == L'L') &&
	    (item[3] == L'L'))
	{
	  val.setNull();
	  break;
	}

        // Parser assumes CmpCommon::diags() is initialized and available
        // so okToReportErrors better be true in this code path.
        CMPASSERT(okToReportErrors);

	// invoke parser to parse the char string and generate a ConstValue
        Parser parser(CmpCommon::context());

                         // Leave space for both semi-colon and null
                         // next points to the next char after the value
        Int32 numChars = MINOF(BOUNDARY_LEN-2,na_wcslen(item)-na_wcslen(next));
  
        // Genesis solution 10-031101-0981
        // When fetching histogram for UNICODE column we need to prefix
        // the buffer to be analyzed by parser with _USC2
        const NAType* colType = columns[i]->getType();
        Int32 prefixLen = 0;
        if ( colType->getTypeQualifier() == NA_CHARACTER_TYPE ) 
        {
           switch (((CharType *)colType)->getCharSet()) {
             case CharInfo::UNICODE:
                prefixLen = 5;
                na_wcsncpy(buf,WIDE_("_UCS2"), prefixLen);
                break;
             case CharInfo::UTF8:
                prefixLen = 5;
                na_wcsncpy(buf,WIDE_("_UTF8"), prefixLen);
                break;
/*********                          Uncomment if we ever support SJIS 
             case CharInfo::SJIS:
                prefixLen = 5;
                na_wcsncpy(buf,WIDE_("_SJIS"), prefixLen);
                break;
*/
             case CharInfo::KANJI_MP:
                prefixLen = 6;
                na_wcsncpy(buf,WIDE_("_KANJI"), prefixLen);
                break;
             case CharInfo::KSC5601_MP:
                prefixLen = 8;
                na_wcsncpy(buf,WIDE_("_KSC5601"), prefixLen);
                break;
             default: // no prefix is needed (ISO88591)
                break;
           }
        }
        na_wcsncpy(buf+prefixLen,item,numChars);
        buf[numChars+prefixLen] = L';';
        buf[numChars+prefixLen+1] = L'\0';
       


	NABoolean negate = FALSE;

	ItemExpr *ie = parser.get_w_ItemExprTree(buf);
	ConstValue * constVal = ie ? ie->castToConstValue(negate) : NULL;

        // Genesis 10-980626-6634, 10-040322-4395 and 10-090206-9004
        // ALM 5956 -- don't do the type check for MCH. Overflowed MCH boundary
        //             value may skip 1 or more column values, so pair-wise type 
        //             matching may not work.
        if (!constVal || 
             (entries == 1 &&
              constVal->getType()->getTypeQualifier() != colType->getTypeQualifier()))
        {
          CmpCommon::diags()->rewind(mark,TRUE);  // get rid of any diags parser may have added
          *CmpCommon::diags() 
              << DgSqlCode(CATALOG_HISTOGRM_HISTINTS_TABLES_CONTAIN_BAD_VALUE) 
              << DgWString0(theValue) 
              << DgString1(columns[i]->getFullColRefNameAsAnsiString());

          CmpInternalException("Bad Interval Boundary", __FILE__ , __LINE__).throwException();
        }

        // For a case-insensitive char type, it is necessary to upper-case the 
        // boundary value first before compute the hash. This is to mimic the
        // run-time behavior where values of such char type are always 
        // upper shifted before case-insensitive comparison, being hashed or
        // encoded.
        // skip the upshifting for Unicode characters, as that is how these
        // are treated in USTAT. 10-090225-9553
        if ((colType->getTypeQualifier() == NA_CHARACTER_TYPE) && 
            ((CharType*)colType)->isCaseinsensitive() &&
            (((CharType*)colType)->getCharSet() != CharInfo::UNICODE))
          constVal = constVal->toUpper(HISTHEAP);

        if (colType->getTypeQualifier() == NA_CHARACTER_TYPE)
        {
          CharInfo::Collation Col_Coll ;
          Col_Coll = ((CharType*)colType)->getCollation();

          if (Col_Coll != CharInfo::DefaultCollation) {
             ((CharType *)(constVal->getType()))->setCollation(Col_Coll);
          }
        }

	val = NormValue (constVal, negate);
         
        // populate the ith entry in cvPtrs[]
        if ( cvPtrs ) cvPtrs[i] = constVal;

	break;
      }
    }

    if(entries == 1)
      this->setValue(val); // filters out non-NULLs with NULL double-value
    else
    {
      if(!valueList_)
        valueList_ = new (heap_) NormValueList(entries);
      valueList_->insertAt(i,val);
    }

    // If boundaryValueTruncated is true, we have just processed the last
    // component of the MCH boundary value that was present.
    if (i == (entries - 1) || boundaryValueTruncated)
      return;

    // next should now point to ','
    item = next + 1;
    while (*item == L' ')
      item++;
    next = (NAWchar *)item;

    // Check if trailing part has been truncated due to size limit.
    if ((next[0] == L'.') &&
        (next[1] == L'.'))
      return;
  } // For loop

  return;
}

// -----------------------------------------------------------------------
//  Given an upper and lower bound, represented in multi-attribute
//  encoded version where:
//    lowBound    = (x1, y1, ...)
//    *this value = (x2, y2, ...)
//    upperBound  = (x3, y3, .. )
//
//  this method returns a ratio stating where this value lies.
//
//  ratio = *thisvalue - lowBound
//          ---------------------
//          upperBound - lowBound
//
//        = x2 - x1 + (y2-y1)/(Maxy-Miny) + (z2-z1)/(Maxz-Minz)*#(Maxy-Miny) ...
//         -------------------------------------------------------------------
//          x3 - x1 + (y3-y1)/(Maxy-Miny) + (z3-z1)/(Maxz-Minz)*#(Maxy-Miny) ...
//
//
// -----------------------------------------------------------------------
double EncodedValue::ratio (const EncodedValue & lowBound,
			    const EncodedValue & upperBound) const
{
  // NB: ignore the comments above -- we only have 1 column in our
  // EncodedValues now, so this function is simpler than what's described

  double retval = -1.0 ;

  double hi = upperBound.getDblValue() ;
  double lo = lowBound.getDblValue() ;
  double me = this->getDblValue() ;

  if ( hi > lo ) // implies hi-lo != 0 -- avoid div-by-zero!
  {
    CMPASSERT(hi>=me);
    CMPASSERT(lo<=me);
    if      ( me == hi ) 
      retval = 1.0 ;
    else if ( me == lo ) 
      retval = 0.0 ;
    else
      retval = (me-lo)/(hi-lo);
  }

  // if someone passes the parameters in incorrectly, we should
  // still return a reasonable result!
  else if ( hi < lo )
    retval = ratio (upperBound,lowBound) ;

  else // hi == lo
    CMPASSERT(FALSE) ; // misuse of this function!

  CMPASSERT(retval != -1.0) ;

  // make sure we didn't generate any invalid numbers!

  CMPASSERT ( NOT isnan(retval) ) ;

  return retval;
}

COMPARE_RESULT NormValue::compare (const NormValue &other) const
{
  if ( *this > other )
    return (MORE) ;
  else if ( *this < other )
    return (LESS) ;
  else
    return (SAME) ;
}

COMPARE_RESULT NormValueList::compare (const NormValueList * other) const
{
  COMPARE_RESULT result = SAME;
  CollIndex i = 0;
  while(result == SAME && i < entries())
  {
    result = this->at(i).compare(other->at(i));
    i++;
  }
  return result;
}

COMPARE_RESULT EncodedValue::compare (const EncodedValue &other) const
{
  COMPARE_RESULT result = SAME;
  const NormValueList * nvl = other.getValueList();
  if(valueList_ && nvl)
    result = valueList_->compare(nvl);
  else if(valueList_ && !nvl)
    result = valueList_->at(0).compare(other.getValue());
  else if(!valueList_ && nvl)
    result = getValue().compare(nvl->at(0));
  else
    result = getValue().compare(other.getValue());
  return result;
}

void EncodedValue::display (FILE *f, const char * prefix, const char * suffix,
                            CollHeap *c, char *buf) const
{
  Space * space = (Space *)c;
  char mybuf[1000];

  snprintf(mybuf, sizeof(mybuf), "%s( ", prefix);
  PRINTIT(f, c, space, buf, mybuf);

  if(valueList_)
  {
      CollIndex i = 0;
      CollIndex noOfCols = valueList_->entries();
      while(i < noOfCols)
      {
        valueList_->at(i).display(f, prefix, suffix, c, buf);
        snprintf(mybuf, sizeof(mybuf), "%s,", prefix);
        i++;
      };
  }
  else
    getValue().display(f, prefix, suffix, c, buf);

  snprintf(mybuf, sizeof(mybuf), " )%s", suffix);
  PRINTIT(f, c, space, buf, mybuf);
}

const NAString EncodedValue::getText(NABoolean parenthesized, NABoolean showFractionalPart) const
{
  char cp [100];  // max double value ~= 1.16e77, hence 100 bytes is plenty.
  char *s = cp;
  char *t = s;

  if ( parenthesized == TRUE ) {
    sprintf (s, "( ");
    t += strlen(s);
  }

  if ( getValue().isNull() )
    sprintf (t, "NULL");
  else {
    if ( showFractionalPart == TRUE )
       sprintf (t, "%.4f", getDblValue());
    else
       sprintf (t, "%.0f", getDblValue());
  }

  if ( parenthesized == TRUE ) {
    t = s + strlen(s);
    sprintf (t, " )");
  }

  return cp;
}

UInt32 EncodedValue::computeHashForNumeric(SQLNumeric* nt)
{
  return getValue().computeHashForNumeric(nt);
}

UInt32 NormValue::computeHashForNumeric(SQLNumeric* nt)
{
   CMPASSERT(nt);
   Lng32 len = nt->getNominalSize();
   CMPASSERT(len >= 0);

   Lng32 longTemp;
   ULng32 usLongTemp;
   short shrtTemp;
   unsigned short usShrtTemp;

   double x = getValue();

    char result[8];

    // Fix 10-091117-6417. 
    // Warning: some of the following conversions can be lossy when the 
    // scale is relatively big.  For example, 2.9998611111111111 
    // will become 2.9998611111111112 after converting to Int64,
    // where 2.9998611111111111 is of type NUMERIC(18, 16). When that value
    // is skewed, we will miss it in the skew buster.

    UInt32 flags = ExHDPHash::NO_FLAGS;

    if ( len <= 2 ) {
      flags = ExHDPHash::SWAP_TWO;
      if ( nt->isUnsigned()) {
         usShrtTemp = (unsigned short)(x*pow(10.0, nt->getScale()));
         memcpy(result, (char*)&usShrtTemp, len);
      } else {
         shrtTemp = (short)(x*pow(10.0, nt->getScale()));
         memcpy(result, (char*)&shrtTemp, len);
      }
    } else
    if ( len <= 4 ) {
      flags = ExHDPHash::SWAP_FOUR;
      if ( nt->isUnsigned()) {
         usLongTemp = (ULng32)(x*pow(10.0, nt->getScale()));
         memcpy(result, (char*)&usLongTemp, len);
      } else {
         longTemp = (Lng32)(x*pow(10.0, nt->getScale()));
         memcpy(result, (char*)&longTemp, len);
      }
    } else {
      flags = ExHDPHash::SWAP_EIGHT;
       Int64 int64Temp = (Int64) (x * pow(10.0, nt->getScale()));
       memcpy(result, (char*)&int64Temp, len);
    }

    UInt32 hash =  ExHDPHash::hash(result, flags, len);
    return hash;
}
