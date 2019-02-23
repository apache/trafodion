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
* File:         conversionHex.cpp
* Description:  hexa string conversion routines
*
* Created:      5/14/03
* Language:     C++
*
*
*
******************************************************************************
*/

#include "conversionHex.h"
#include "NAString.h"
#include "nawstring.h"
#include "ComASSERT.h"
#include "SQLTypeDefs.h"
#include "str.h"
#include "csconvert.h"

// a helper function converting a hexdecimal digit to its value
static
unsigned short getHexDigitValue(NAWchar wc)
{
   if ( isDigit8859_1(wc) )
      return (unsigned short)wc - '0';
   else {
      if ( 'A' <= wc AND wc <= 'F' )
         return (unsigned short)wc - 'A' + 10;
      else
         return (unsigned short)wc - 'a' + 10;
   }
}

// JQ
// spaces are allowed in hexadecimal format string literals
// these spaces have to be removed
//
static NAWString *removeWSpaces(const NAWchar *s, Int32& len, NAWchar quote, CollHeap *heap)
{
  NAWString *r = new (heap) NAWString(heap);
  Int32 tmpLen = 0;
  if (!s || len <= 0) return r;

  for (Int32 x = 0; x < len; x++)
    {
      if (s[x] == quote)
        {
        // prematurely end the process
          break;
        }
      if (s[x] != L' ') {
        ++tmpLen;
        r->append(s[x]);
      }
    }
  len = tmpLen;
  return r;
}

// a helper function converting a hexdecimal digit to a single-byte string
static NAString * 
convHexToChar(const NAWchar *s, Int32 inputLen, CharInfo::CharSet cs, CollHeap* heap)
{
  ComASSERT((inputLen % SQL_DBCHAR_SIZE) == 0);
  NAString *r = new (heap) NAString(heap);
  if (!s || inputLen <= 0) return r;

  unsigned char upper4Bits;
  unsigned char lower4Bits;

  for (Int32 i = 0; i < inputLen; i=i+2) {
    if (isHexDigit8859_1(s[i]) AND isHexDigit8859_1(s[i+1])) {

      upper4Bits = getHexDigitValue(s[i]);
      lower4Bits = getHexDigitValue(s[i+1]);

      char c = (upper4Bits << 4) | lower4Bits;
      r->append(c);
    } else { 
      NADELETE(r, NAString, heap);
      return NULL;
    }
  }
  return r;
}

// a helper function converting a hexdecimal digit to a double-byte string
static NAWString * 
convHexToWChar(const NAWchar *s, Int32 inputLen, CharInfo::CharSet cs, CollHeap* heap)
{
  if ( cs == CharInfo::UNICODE )
  {
    NAWString *r = new (heap) NAWString(heap);
    if (!s || inputLen <= 0) return r;

    assert((inputLen % 4) == 0);
  
    for (Int32 i = 0; i < inputLen; i=i+4) {
      if ( isHexDigit8859_1(s[i])   AND isHexDigit8859_1(s[i+1]) AND
           isHexDigit8859_1(s[i+2]) AND isHexDigit8859_1(s[i+3]) )
      {
        unsigned short first4Bits  = getHexDigitValue(s[i]);
  	unsigned short second4Bits = getHexDigitValue(s[i+1]);
  	unsigned short third4Bits  = getHexDigitValue(s[i+2]);
  	unsigned short fourth4Bits = getHexDigitValue(s[i+3]);
  
        NAWchar wc = (first4Bits << 12) | (second4Bits << 8) | (third4Bits << 4) | fourth4Bits;
        r->append(wc);
      }
      else {
        NADELETE(r, NAWString, heap);
        return NULL;
      }
    }
  
    if (! CharInfo::checkCodePoint(r->data(), r->length(), cs) ) {
      NADELETE(r, NAWString, heap);
      return NULL;
    }

    return r;
  }
  return NULL;
}

// verify whether a hexadecimal string is in valid format.
static NABoolean isValidHexFormat(const NAWchar *str, Int32 len, CharInfo::CharSet cs)
{
  // specified by  _charsetname'([0-9, a-f, A-F])*'

  // ISO88591:               hex digits per char = 2
  // UCS2/KSC5601/KANJI:     hex digits per char = 4
  Int32 hexPerChar = 2 * CharInfo::minBytesPerChar(cs);

  // The following while loop recognizes regular expression:
  // space* [non-space]\{hexPerChar\} space*
  // 
  // Examples of legal hexdecimal literals. 
  //       x' 98 FF  F0' (ISO88591)
  //       x' 98FF F0' (ISO88591)
  //       _ucs2 x' 98FF 3dF0' (UCS2)
  // 
  // Examples of illegal hexdecimal literals. 
  //       x' 98F F  F0' (ISO88591)
  //       _ucs2 x'9FF 3dF0' (UCS2)
  Int32 i = 0;
  while ( i<len ) {
     if ( str[i] != L' ' ) {
        // at least hexPerChar non-space chars should be present, starting at i
        for ( Int32 j=0; j<hexPerChar; j++ )
        {
          if ( i >= len || ! isHexDigit8859_1(str[i++]) ) {
             // invalid format
             return FALSE;
          } 
        }
     } else i++;
  }
  return TRUE;
}


hex_conversion_code verifyAndConvertHex(const NAWchar *str, Int32 len, NAWchar quote,
                   CharInfo::CharSet cs, CollHeap* heap, void*& result)
{
  if ( CharInfo::isHexFormatSupported(cs) == FALSE )
    return NOT_SUPPORTED;

  if ( isValidHexFormat(str, len, cs) == FALSE )
    return INVALID;

  if ( heap == 0 )
    return CONV_FAILED;

  NAWString *tmpStr = removeWSpaces(str, len, quote, heap);

  // convert to actual string literal
  hex_conversion_code ok = INVALID_CODEPOINTS;
  switch ( cs ) {
    case CharInfo::KANJI_MP:
    case CharInfo::KSC5601_MP:
    case CharInfo::ISO88591:
    case CharInfo::UTF8:
    case CharInfo::BINARY:
      {
        Int32   StrLength = (Int32)(tmpStr->length());
        result = convHexToChar(tmpStr->data(), StrLength, cs, heap);
        if (result ) {
           ok = SINGLE_BYTE; // Assume good data for now
           if (cs == CharInfo::UTF8) {
              // Verify UTF8 code point values are valid
              Int32   iii = 0;
              Int32   rtnv = 0;
              NAString* reslt = (NAString*)result;
              UInt32  UCS4 = 0;
              StrLength = StrLength/2;  // Orig StrLength was for hex-ASCII string
              while ( iii < StrLength )
              {
                 rtnv = LocaleCharToUCS4( &(reslt->data()[iii]), StrLength - iii,
                                          &UCS4, cnv_UTF8 );
                 if (rtnv == CNV_ERR_INVALID_CHAR)
                 {
                    ok = INVALID_CODEPOINTS; // Return error
                    break;
                 }
                 iii += rtnv;
              }
           }
        }
      }
      break;

    case CharInfo::UNICODE:
      {
        result = convHexToWChar(tmpStr->data(), (Int32)(tmpStr->length()), cs, heap);
        if (result) ok = DOUBLE_BYTE;
      }
      break;

    default:
      ok = INVALID;
      break;
  }
  return ok;
}
