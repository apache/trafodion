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
#ifndef NAWINNT_H
#define NAWINNT_H
/* -*-C++-*-
******************************************************************************
*
* File:         NAWinNT.h
* Description:  Abstractions of WINNT idioms for platform independence
*               Later expanded to provide APIs relating to NAWchar data type
*               (an NAWchar object contains Unicode UCS-2 code points)
*
*
*
******************************************************************************
*/

#include "Platform.h"

// In the names below, "NA" of course stands for "NA" ("nice abstraction")
// and "W" stands for "wide".

#include <ctype.h>

// We generally use NAWchar as the C data type for wide characters. 
#define NAWchar	wchar_t

#ifndef BYTES_PER_NAWCHAR
#define BYTES_PER_NAWCHAR 2
#endif

#define WIDE_(q)  L ## q  // q is a character constant or string constant
#define NAWCHR(q) ((NAWchar)L ## q)         // q is a C character constanst - ex: 'A'
#define NAWSTR(q) ((const NAWchar*)L ## q)  // q is a C string constant - ex: "abc"

// The following declaration is defined in w:/common/wstr.h.
//
// Source files calling NAWstrpad() and other NAW...() macros/functions
// need to source in the header file wstr.h (besides this header file).
// We avoid sourcing in more header files into this simple header NAWinNT.h
// file due to the complexity of the build of the SQL Engine products.
//
// inline void wc_str_pad(NAWchar *str, Int32 length,
//                                   NAWchar padchar = unicode_char_set::SPACE);


#define NAWisspace	na_iswspace
#define NAWtolower	na_towlower
#define NAWtoupper	na_towupper
#define NAWstrcat	na_wcscat
#define NAWstrchr	na_wcschr
#define NAWstrcmp	na_wcscmp
#define NAWstricmp	na_wcsicmp
#define NAWstrincmp	na_wcsincmp
#define NAWstrcpy	na_wcscpy
#define NAWstrlen	na_wcslen
#define NAWstrncpy	na_wcsncpy
#define NAWsscanf	na_swscanf
#define NAWstrncmp      na_wcsncmp
#define NAWstrpad       wc_str_pad
#define NAWsprintf      na_wsprintf
#define NAWwcstol       na_wcstol

// inline wchar functions
inline
UInt32 na_wcslen (const NAWchar * wstr)
{
   const NAWchar* s;
   for (s = wstr; *s; ++s);
   return s - wstr;
}

inline
Int32 na_iswspace (NAWchar wc)
{
   return ( wc == NAWCHR(' ') );
}

NAWchar *  na_wcscat (NAWchar*, const NAWchar*);
Int32      na_wcscmp (const NAWchar *, const NAWchar *);
Int32      na_wcsicmp (const NAWchar *, const NAWchar *);
Int32      na_wcsncmp (const NAWchar *, const NAWchar *, UInt32);
Int32      na_wcsincmp (const NAWchar *, const NAWchar *, UInt32);
NAWchar *  na_wcsncpy (NAWchar * dest, const NAWchar * src, UInt32 n);

// We define these functions on Linux because we have chosen to use the
// short version of wchar_t (i.e., the 2-byte-size NAWchar data type)
// and these functions will help us not to call the system ones.
//
// Functions na_wcstombs(), na_mbstowcs(), and na_wcswcs are defined in
// the source file w:/common/wstr.cpp.
// 
size_t    na_wcstombs(char *, const NAWchar*, size_t);
size_t    na_mbstowcs(NAWchar*, const char*,  size_t);
NAWchar * na_wcswcs(const NAWchar*, const NAWchar*);

// This compares two strings that can have embedded nulls.
Int32      na_wcsnncmp (const NAWchar *wstr1, UInt32 len1,
                                   const NAWchar *wstr2, UInt32 len2);

inline
NAWchar *na_wcscpy (NAWchar * dest, const NAWchar * src)
{
   NAWchar* q = dest;

   for (; (*q = *src) != '\0'; ++src, ++q);

   return dest;
}

inline
Int32 na_wcs_has_only_ascii_chars (const NAWchar * src, size_t srcLenInNAWchars)
{
   for( size_t i = 0; i < srcLenInNAWchars ; i++ )
      if ( src[i] > 0x7F )
        return 0; // FALSE

   return 1; // TRUE
}

/*
inline
NAWchar *na_wcsncpy (NAWchar * dest, const NAWchar * src, unsigned int n)
{
  if (n != 0) {

    NAWchar *d = dest;
    const NAWchar *s = src;

    do {
      if ((*d++ = *s++) == 0) {
         // NUL pad the remaining n-1 bytes
         while (--n != 0)
           *d++ = 0;
         break;
      }
   } while (--n != 0);
 }
 return (dest);
}
*/

inline
NAWchar na_towupper (NAWchar wc)
{
   if ( NAWCHR('a') <= wc && wc <= NAWCHR('z') )
      return wc - NAWCHR('a') + NAWCHR('A');
   else
      return wc;
}

inline
NAWchar na_towlower (NAWchar wc)
{
   if ( NAWCHR('A') <= wc && wc <= NAWCHR('Z') )
      return wc - NAWCHR('A') + NAWCHR('a');
   else
      return wc;
}

Int64 na_wcstoll (const NAWchar *);
Lng32 na_wcstol (const NAWchar *);
NAWchar *na_wcschr (const NAWchar *, NAWchar);
NAWchar *na_wcschrSkipOverParenText (const NAWchar *, NAWchar);
NAWchar *na_wcsrchr (const NAWchar *, NAWchar);
Int32 na_wsprintf(NAWchar *buffer, const NAWchar *format, ... );
NAWchar *na_wmemchr(const NAWchar *ws, NAWchar wc, Int32 n); // used by swsprintf

Int32 na_swscanf(const NAWchar *buffer, const NAWchar *format, ... );
double   na_wcstod (const NAWchar *, NAWchar **); // used by swscanf


  #define T_TEXT(q)	WIDE_(q)        // q is "string"


#endif /* NAWINNT_H */
