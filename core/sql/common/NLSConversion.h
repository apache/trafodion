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
 * File:         NLSConversion.h
 * RCS:          $Id: 
 * Description:  The header file of a set of conversion functions
 *               
 *               
 * Created:      7/8/98
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#ifndef _NASTRINGCONVERSION_H
#define _NASTRINGCONVERSION_H

#include "Platform.h"

#if !defined(MODULE_DEBUG) 

#include "BaseTypes.h"
#include "NAWinNT.h"
#include "NAHeap.h"
#include "stringBuf.h"
#include "csconvert.h"
#include "charinfo.h"

class NAString;
class NAWString;

NAWcharBuf* 
ISO88591ToUnicode(const charBuf& iso88591String, CollHeap *heap, 
                  NAWcharBuf*& unicodeString, NABoolean addNullAtEnd = TRUE);

charBuf* 
unicodeToISO88591(const NAWcharBuf& unicodeString, CollHeap* heap, 
                  charBuf*& iso88591String, NABoolean addNullAtEnd = TRUE,
                  NABoolean allowInvalidCodePoint = TRUE);

NAWcharBuf* 
csetToUnicode(const charBuf& csetString, CollHeap *heap, 
                  NAWcharBuf*& unicodeString, Int32 cset, Int32 &errorcode,
                  NABoolean addNullAtEnd = TRUE, Int32 *charCount=NULL, Int32 *errorByteOff=NULL);

charBuf* 
unicodeTocset(const NAWcharBuf& unicodeString, CollHeap* heap, 
                  charBuf*& csetString, Int32 cset, Int32 &errorcode, NABoolean addNullAtEnd = TRUE,
                  NABoolean allowInvalidCodePoint = TRUE, Int32 *charCount=NULL, Int32 *errorByteOff=NULL);

NAWcharBuf* 
sjisToUnicode(const charBuf& sjisString, CollHeap *heap, 
              NAWcharBuf*& unicodeString, NABoolean addNullAtEnd = TRUE);

charBuf* 
unicodeToSjis(const NAWcharBuf& unicodeString, CollHeap *heap, 
              charBuf*& sjisString, NABoolean addNullAtEnd = TRUE,
                  NABoolean allowInvalidCodePoint = TRUE);

charBuf* 
unicodeToUtf8(const NAWcharBuf& unicodeString, CollHeap *heap, 
              charBuf*& utf8String, NABoolean addNullAtEnd = TRUE,
                  NABoolean allowInvalidCodePoint = TRUE);

NAWcharBuf*
ksc5601ToUnicode(const charBuf& ksc5601String, CollHeap *heap,
              NAWcharBuf*& unicodeString, NABoolean addNullAtEnd = TRUE);

charBuf*
unicodeToKsc5601(const NAWcharBuf& unicodeString, CollHeap *heap,
              charBuf*& ksc5601String, NABoolean addNullAtEnd = TRUE,
                  NABoolean allowInvalidCodePoint = TRUE);

Int32 unicodeToSjisChar(char *sjis, NAWchar wc);

cnv_charset convertCharsetEnum (Int32/*i.e. enum CharInfo::CharSet*/ inset);

const char* getCharsetAsString(Int32/*i.e. enum CharInfo::CharSet*/ charset);

Lng32 UnicodeStringToLocale(Lng32/*i.e. enum CharInfo::CharSet*/ charset,
                  const NAWchar* wstr, Lng32 wstrLen, char* buf, Lng32 bufLen, NABoolean addNullAtEnd = TRUE,
                  NABoolean allowInvalidCodePoint = TRUE);


Lng32 LocaleStringToUnicode(Lng32/*i.e. enum CharInfo::CharSet*/ charset,
                  const char* str, Lng32 strLen, NAWchar* wstrBuf, Lng32 wstrBufLen, NABoolean addNullAtEnd = TRUE);

Int32 localeConvertToUTF8(char* source,
                  Lng32 sourceLen,
                  char* target,
                  Lng32  targetLen,
                  Lng32 charset, // enum cnv_charset type
                  CollHeap *heap = 0,
                  Lng32  *charCount = NULL,
                  Lng32  *errorByteOff = NULL);

Int32 UTF8ConvertToLocale(char* source,
                        Lng32 sourceLen,
                        char* target,
                        Lng32  targetLen,
                        Lng32 charset, // enum cnv_charset type
                        CollHeap *heap = 0,
                        Lng32  *charCount = NULL,
                        Lng32  *errorByteOff = NULL);

// -----------------------------------------------------------------------
// ComputeWidthInBytesOfMbsForDisplay:
//
// Returns the display width (in bytes) that is the closest to the
// specified maximum display width (in bytes) without chopping the
// rightmost multi-byte characters into two parts so that we do not
// encounter the situation where the first part of the multi-byte
// character is in the current display line and the other part of
// the character is in the next display line.
//
// If encounters an error, return the error code (a negative number)
// define in w:/common/csconvert.h.
//
// In parameter pv_eCharSet contains the character set attribute
// of the input string passed in via the parameter pp_cMultiByteStr.
//
// The out parameter pr_iNumOfTranslatedChars contains the number of
// the actual (i.e., UCS4) characters translated.
//
// The out parameter pr_iNumOfNAWchars contains the number of UCS2
// characters (NAwchar[acters]) instead of the number of the actual
// (i.e., UCS4) characters.
//
// Note that classes NAMemory and CollHeap are the same except for
// the names.
// -----------------------------------------------------------------------
Int32 ComputeWidthInBytesOfMbsForDisplay ( const char * pp_cpMbs                // in
                                         , const Int32 pv_iMbsLenInBytes        // in
                                         , const Int32 pv_iMaxDisplayLenInBytes // in
                                         , const CharInfo::CharSet  pv_eCharSet // in
                                         , Int32 &    pr_iNumOfTranslatedChars  // out - number of chars translated
                                         , Int32 &    pr_iNumOfNAWchars         // out - width in NAWchar(acters)
                                         , NAMemory * heap = NULL               // in  - default is process heap
                                         );

// -----------------------------------------------------------------------
// ComputeStrLenInNAWchars:
//
// Returns the length of the input string (in the specified character set)
// in number of NAWchar(acters) - Note that a UTF16 character (i.e., a
// surrogate pair) will have a count of 2 NAWchar(acters).
//
// Return an error code (a negative number) if encounters an error.  The
// error code values are defined in w:/common/csconvert.h.
// -----------------------------------------------------------------------
Int32 ComputeStrLenInNAWchars (const char * pStr,
                               const Int32  strLenInBytes,
                               const CharInfo::CharSet cs,
                               NAMemory *   workspaceHeap);

// -----------------------------------------------------------------------
// ComputeStrLenInUCS4chars:
//
// Returns the actual (i.e., UCS4) character count of the input string
// (in the specified character set) in the actual (i.e., UCS4) characters.
// Return an error code (a negative number) if encounters an error.  The
// error code values are defined in w:/common/csconvert.h.
// -----------------------------------------------------------------------
Int32 ComputeStrLenInUCS4chars (const char * pStr,
                                const Int32 strLenInBytes,
                                const CharInfo::CharSet cs);

// convert a Unicode string back to char
class NAMemory;
NAString *unicodeToChar(const NAWchar *s, Int32 len, Lng32 charset, 
                        NAMemory *h=NULL, NABoolean allowInvalidChar = FALSE);

// convert a char string to Unicode
NAWString *charToUnicode(Lng32 charset, const char *s, Int32 len, 
                        NAMemory *h=NULL);
NAWString *charToUnicode(Lng32 charset, const char *s, NAMemory *h=NULL);

// convert a char string to another char string (in a different character set);
// if both target and source char sets are the same, do a deep copy.
NAString *charToChar(Lng32 targetCS, const char *s, Int32 sLenInBytes, Lng32 sourceCS, 
                     NAMemory *h=NULL, NABoolean allowInvalidChar = FALSE);

#else

#include <stdio.h>

typedef unsigned short NAWchar;
typedef NAWchar NAWchar;
typedef char CollHeap;
#define NABoolean Int32
#define TRUE 1
#define FALSE 0
#define NADELETEBASIC(buf_, heap_)
#define NADELETE(buf_, T_, heap_)
#include "stringBuf.h"

void * operator new(size_t size, CollHeap *s)
{
   void * result = ::operator new(size);
   return result;
}

#endif

#endif
