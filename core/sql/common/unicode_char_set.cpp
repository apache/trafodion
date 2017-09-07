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
 * File:         unicode_char_set.cpp
 * RCS:          $Id: 
 * Description:  The implementation of unicode_char_set class
 *
 *
 * Created:      7/8/98
 * Modified:     $ $Date: 1998/08/10 16:01:12 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */

#include "unicode_char_set.h"
#include "BaseTypes.h"

#include "nawstring.h"

// 4/10/98  Unicode char/string manipulations

typedef struct unicode_mapping
{
  unsigned short code1;
  unsigned short code2;
} unicode_mapping_t;

typedef struct unicode_mapping_full
{
  unsigned short code1;
  unsigned short code2[3];
} unicode_mapping_full_t;

// the following three included .h files are generated using
// the script MiscVOB/i18n/CaseMapping.pl
static const unicode_mapping_full_t unicode_lower2upper_mapping_table_full[] =
{
#include "1n_lt2u.h"
};

static const unicode_mapping_t unicode_lower2upper_mapping_table[] =
{
#include "11_lt2u.h"
};


static const unicode_mapping_t unicode_upper2lower_mapping_table[] =
{
#include "11_ut2l.h"
};


NAWchar 
binary_search(NAWchar wc, Int32 lower, Int32 upper, 
              unicode_mapping_t table[])
{
   while ( lower <= upper ) {
      Int32 middle = (lower+upper) >> 1;
      if ( table[middle].code1 == wc )
         return table[middle].code2;

      if ( table[middle].code1 < wc )
        lower = middle + 1;
      else
        upper = middle - 1;
   }
   return wc;
}

// search the lower to upper full mapping table
NAWchar* 
binary_search(NAWchar wc, Int32 lower, Int32 upper, 
              unicode_mapping_full_t table[])
{
   while ( lower <= upper ) {
      Int32 middle = (lower+upper) >> 1;
      if ( table[middle].code1 == wc )
         return (NAWchar *)table[middle].code2;

      if ( table[middle].code1 < wc )
        lower = middle + 1;
      else
        upper = middle - 1;
   }
   return NULL;
}

//
// 4/10/98  compute the Unicode upperShift function
//
NAWchar unicode_char_set::to_upper(const NAWchar x)
{
   if ( IN_RANGE(x, 0x61, 0x7a) ) { // frequently used chars checked first
#pragma nowarn(1506)   // warning elimination 
      return x - 0x61 + 0x41;
#pragma warn(1506)  // warning elimination 
   }

   if ( IN_RANGE(x, 0xe0, 0xf6) ) { // frequently used chars checked first
#pragma nowarn(1506)   // warning elimination 
      return x - 0xe0 + 0xc0;
#pragma warn(1506)  // warning elimination 
   }

   return binary_search(x, 0, 
        sizeof(unicode_lower2upper_mapping_table)/sizeof(unicode_mapping_t)-1, 
		        (unicode_mapping_t*)unicode_lower2upper_mapping_table
		       );
}

// full case mapping 
NAWchar* unicode_char_set::to_upper_full(const NAWchar x)
{
   return binary_search(x, 0, 
       sizeof(unicode_lower2upper_mapping_table_full)/sizeof(unicode_mapping_full_t)-1, 
		        (unicode_mapping_full_t*)unicode_lower2upper_mapping_table_full
		       );
}

// This method works the same way as ex_function_upper_unicode::eval()
// in exp/exp_function_upper_unicode.cpp. This method is being called
// by ConstValue::toUpper() while applying upper method on constants.
// The 'len' number of chars in 'str' are upshifted and the result
// is kept in 'upStr'.
void
unicode_char_set::to_upper(NAWchar *str, size_t len, NAWString &upStr)
{
  NAWchar* tmpWCP = NULL;

  for(size_t i = 0; i < len; ++i) {

    // search against unicode_lower2upper_mapping_table_full
    tmpWCP = unicode_char_set::to_upper_full(str[i]);

    if (tmpWCP) {
      upStr += tmpWCP[0];
      upStr += tmpWCP[1];

      if (tmpWCP[2] != (NAWchar)0) {
        upStr += tmpWCP[2];
      }
    } else {
      // a NULL return from to_upper_full()
      // search against unicode_lower2upper_mapping_table then
      upStr += unicode_char_set::to_upper(str[i]);
    }
  }
}

/*
long unicode_char_set::to_upper(NAWchar *str, long srcLen,
                                NAWchar *upStr, long maxTgtLen)
{
  NAWchar *tmpWCP = NULL;
  long tgtLen = 0, i = 0;

  for(i = 0; i < srcLen; ++i)
  {
    // search against unicode_lower2upper_mapping_table_full
    tmpWCP = unicode_char_set::to_upper_full(str[i]);

    if (tmpWCP)
    {
      if(tgtLen >= maxTgtLen - 1) return -1; 

      upStr[tgtLen++] = tmpWCP[0];
      upStr[tgtLen++] = tmpWCP[1];

      if (tmpWCP[2] != (NAWchar)0)
      {
        if(tgtLen >= maxTgtLen) return -1; 
        upStr[tgtLen++] = tmpWCP[2];
      }
    } else {
      // a NULL return from to_upper_full()
      // search against unicode_lower2upper_mapping_table then
      if(tgtLen >= maxTgtLen) return -1; 
      upStr[tgtLen++] = unicode_char_set::to_upper(str[i]);
    }
  }
  return tgtLen;
}
*/

// 4/10/98  compute the Unicode toLower function
NAWchar unicode_char_set::to_lower(const NAWchar x)
{
  if ( IN_RANGE(x, 0x41, 0x5a) ) { // frequently used chars checked first
#pragma nowarn(1506)   // warning elimination 
     return  x + 0x61 - 0x41;
#pragma warn(1506)  // warning elimination 
  }

  if ( IN_RANGE(x, 0xc0, 0xd6) ) { // frequently used chars checked first
#pragma nowarn(1506)   // warning elimination 
     return x + 0xe0 - 0xc0;
#pragma warn(1506)  // warning elimination 
  }

   return binary_search(x, 0, sizeof(unicode_upper2lower_mapping_table)/sizeof(unicode_mapping_t)-1, 
		        (unicode_mapping_t*)unicode_upper2lower_mapping_table
		       );
}

//
// UTF-8 related functions
//

Int32 IndexOfLastByteOfUTF8CharAtOrBeforePos (const unsigned char *utf8Str,
                                              const Int32 utf8StrLenInBytes,
                                              const Int32 bytePos)
{
  if (utf8Str == NULL || utf8StrLenInBytes <= 0 || bytePos < 0 || bytePos >= utf8StrLenInBytes)
    return -1; // error
  if (IS_7_BIT_ASCII_IN_UTF8_CHAR(utf8Str[bytePos]))
    return bytePos;
  Int32 indexOf1stByteOfUtf8Char = IndexOfFirstByteOfUTF8CharAtOrBeforePos(utf8Str, utf8StrLenInBytes, bytePos);
  if (indexOf1stByteOfUtf8Char < 0) // could not find the first byte in a UTF-8 character in the string
    return -1; // cannot tell if this is the last byte
  Int32 byteCountForUtf8Char = UTF8CharLenInBytes(utf8Str[indexOf1stByteOfUtf8Char]);
  if (byteCountForUtf8Char <= 0) // error
    return -1; // cannot tell if this is the last byte
  Int32 indexOfLastByteOfUtf8Char = indexOf1stByteOfUtf8Char + byteCountForUtf8Char - 1;
  if (indexOfLastByteOfUtf8Char == bytePos)
    return bytePos;
  if (indexOf1stByteOfUtf8Char > 0)
    return IndexOfLastByteOfUTF8CharAtOrBeforePos(utf8Str, utf8StrLenInBytes,
                                                  indexOf1stByteOfUtf8Char - 1 /*bytePos*/);
  return -1; // error
}

Int32 UTF8CharLenInBytes(const unsigned char firstByteOfTheUtf8Char)
{
  if (IS_NOT_1ST_BYTE_IN_UTF8_CHAR(firstByteOfTheUtf8Char))
    return 0; // error
  if (IS_7_BIT_ASCII_IN_UTF8_CHAR(firstByteOfTheUtf8Char))
    return 1;
  if (((firstByteOfTheUtf8Char & 0xE0) >> 5) == 0x06)
    return 2;
  if (((firstByteOfTheUtf8Char & 0xF0) >> 4) == 0x0E)
    return 3;
  if (((firstByteOfTheUtf8Char & 0xF8) >> 3) == 0x1E)
    return 4;
//if (((firstByteOfTheUtf8Char & 0xFC) >> 2) == 0x3E)
//  return 5;
//if (((firstByteOfTheUtf8Char & 0xFE) >> 1) == 0x7E)
//  return 6;
//if (firstByteOfTheUtf8Char                 == 0xFE)
//  return 7;
//if (firstByteOfTheUtf8Char                 == 0xFF)
//  return 8;
  return 0; // error
}

Int32 IndexOfFirstByteOfUTF8CharAtOrBeforePos(const unsigned char *utf8Str,
                                              const Int32 utf8StrLenInBytes,
                                              const Int32 bytePos)
{
  if (utf8Str == NULL || utf8StrLenInBytes <= 0 || bytePos < 0 || bytePos >= utf8StrLenInBytes)
    return -1; // error
  if (IS_7_BIT_ASCII_IN_UTF8_CHAR(utf8Str[bytePos]))
    return bytePos;
  Int32 i = bytePos;
  while (i >= 0 && IS_NOT_1ST_BYTE_IN_UTF8_CHAR(utf8Str[i]))
    i--;
  if (i >= 0 && IS_1ST_BYTE_IN_UTF8_CHAR(utf8Str[i]))
    return i;
  return -1; // error
}

