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
#ifndef WSTR_H
#define WSTR_H

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

#include <string.h>
#include "Platform.h"
#include "NAWinNT.h"
#include "unicode_char_set.h"

// -----------------------------------------------------------------------
// Compare w-strings <left> and <right> (using unsigned comparison and
// SQL blank-padding semantics)
// for <length> characters.
// Return a negative value if left < right,
// return 0 if left == right,
// return a positive value if left > right.
// -----------------------------------------------------------------------
Int32 compareWcharWithBlankPadding(const NAWchar *wstr1, UInt32 len1,
                                   const NAWchar *wstr2, UInt32 len2);

// -----------------------------------------------------------------------
// Compare strings <left> and <right> (using unsigned comparison).
// for <length> characters.
// Return a negative value if left < right,
// return 0 if left == right,
// return a positive value if left > right.
// -----------------------------------------------------------------------
inline
Int32 wc_str_cmp(const NAWchar *left, const NAWchar *right, Int32 length)
{
  for ( Int32 i=0; i<length; i++ ) {
    if ( left[i] < right[i] ) return -1;
    if ( left[i] > right[i] ) return 1;
  }
  return 0;
}
 
// -----------------------------------------------------------------------
// fill string <str>  for <length> bytes with <padchar>
// -----------------------------------------------------------------------

inline
void wc_str_pad(NAWchar *str, Int32 length, 
                NAWchar padchar = unicode_char_set::SPACE)
{
  for (Int32 i=0; i<length; i++) str[i] = padchar;
}

// Swap bytes for each NAWchar in the string. 
inline
void wc_swap_bytes(NAWchar *str, Int32 length)
{
  unsigned char* ptr;
  unsigned char temp;

  if ( str == 0 || length == 0 ) return;

  for (Int32 i = 0; i < length; i++)
    {
      ptr = (unsigned char*)&str[i];
      temp = *ptr;
      *ptr = *(ptr+1);
      *(ptr+1) = temp;
    }
}

Int32 na_wstr_cpy_convert(NAWchar *tgt, NAWchar *src, Lng32 length, Int32 upshift);

#endif
