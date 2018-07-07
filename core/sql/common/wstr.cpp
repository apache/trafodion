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
 * File:         wstr.C
 * RCS:          $Id: wstr.cpp,v 1.2 1998/08/10 15:34:01  Exp $
 * Description:  
 *               
 * Modified:     $Date: 2001/09/8 15:34:01 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *****************************************************************************
 */

#include <limits>
#include <errno.h>
#include "NAAssert.h"
#include "NAWinNT.h" // where the headers are declared. Should be moved
                     // to wstr.h later
#include "wstr.h"

#ifndef NULL
#define NULL    0
#endif


// inline. because nawstring class needs it
/*
unsigned wcslen (const NAWchar * wstr)
{
   const NAWchar* s;
   for (s = wstr; *s; ++s);
   return s - wstr;
}
*/

/*
int iswspace (NAWchar wc)
{
   return ( wc == L' ' );
}
*/

// Compares two Wchar strings using SQL blank-padding semantics.
// Returns +ve if the first string is greater, 0 if equal, -ve if
// the second string is greater.
Int32 compareWcharWithBlankPadding(const NAWchar *wstr1, UInt32 len1,
                                   const NAWchar *wstr2, UInt32 len2)
{
  UInt32 shorterLen = (len1 < len2 ? len1 : len2);
  Int32 rc = memcmp(wstr1,wstr2,shorterLen*sizeof(NAWchar));
  if (rc == 0)
    {
      if (len1 < len2)
        {
          // compare the rest of wstr2 with blanks
          while ((shorterLen < len2) && (wstr2[shorterLen] == L' '))
            shorterLen++;

          if (shorterLen < len2)
            {
              if (wstr2[shorterLen] > L' ')
                rc = -1;
              else
                rc = 1;
            }
        }
      else if (len1 > len2)
        {
          // compare the rest of wstr1 with blanks
          while ((shorterLen < len1) && (wstr1[shorterLen] == L' '))
            shorterLen++;

          if (shorterLen < len1)
            {
              if (wstr1[shorterLen] > L' ')
                rc = 1;
              else
                rc = -1;
            }
        }            
    }
  return rc;
}

// Do a char-by-char comparison (including nulls) up to the length of the
// shorter string or the first difference. If the strings are equal up to the
// length of the shorter one, we consider the longer one to be greater and
// return -1 if the short string is the first string, and 1 if it is the
// second string. We can't return the positive or negative value of the first
// character of the remainder of the longer string because it may be a null,
// which would result in 0 being returned and interpreted as if the strings
// were equal.
Int32 na_wcsnncmp (const NAWchar *wstr1, UInt32 len1,
                   const NAWchar *wstr2, UInt32 len2)
{
  UInt32 shorterLen = (len1 < len2 ? len1 : len2);
  UInt32 currCharPos = 0;

  while (currCharPos++ < shorterLen && *wstr1++ == *wstr2++)
    ;

  if (currCharPos > shorterLen)
    {
      // First shorterLen chars were equal.
      if (len1 < len2)
        return -1;
      else if (len1 > len2)
        return 1;
      else
        return 0;
    }
  else
    return *--wstr1 - *--wstr2;
//    return (*(const UInt16*)--wstr1 - *(const UInt16*)--wstr2);
}

Int32 na_wcsncmp (const NAWchar * wstr1, const NAWchar * wstr2, UInt32 len)
{
   if ( len == 0 ) return 0;

   for (Int32 i=0; i<len; i++ )
   {
     if ( *wstr1 != *wstr2 )
        return ( *wstr1 - *wstr2 );

     if ( *wstr1 == 0 ) break;

     wstr1++; wstr2++; ;
   }

   return 0;
}

Int32 na_wcscmp (const NAWchar * wstr1, const NAWchar * wstr2)
{
   while (*wstr1 == *wstr2++)
		if (*wstr1++ == 0)
			return (0);
	return (*wstr1 - *--wstr2);
}

#if 0 /* As of 8/30/2011, there are no callers in SQ SQL. */
Int32 na_wcsicmp (const NAWchar * wstr1, const NAWchar * wstr2)
{
   while (na_towlower(*wstr1) == na_towlower(*wstr2++))
		if (*wstr1++ == 0)
			return (0);
	return (*wstr1 - *--wstr2);
}
#endif /* As of 8/30/2011, there are no callers in SQ SQL. */

Int32 na_wcsincmp (const NAWchar * wstr1, const NAWchar * wstr2, UInt32 len)
{
  if ( len == 0 ) return 0;

  for ( Int32 i = 0; static_cast<UInt32>(i) < len; i++, wstr1++, wstr2++ )
  {
    // na_towlower() only lowers letters specified in 7-bit US ASCII
    if ( na_towlower(*wstr1) != na_towlower(*wstr2) )
      return ( static_cast<Int32>(static_cast<UInt32>(*wstr1)) -
               static_cast<Int32>(static_cast<UInt32>(*wstr2)) );

    if ( *wstr1 == 0 ) break;
  }

  return 0;
}

NAWchar *na_wcscat (NAWchar* wstr1, const NAWchar* wstr2)
{
   if ( wstr1 == NULL || wstr2 == NULL ) return wstr1;
   na_wcscpy(wstr1+na_wcslen(wstr1), wstr2);
   return wstr1;
}

// copies src to tgt for length bytes and upshifts, if upshift <> 0,
// else downshifts. Src and Tgt may point to the same location.
Int32 na_wstr_cpy_convert(NAWchar *tgt, NAWchar *src, Lng32 length, Int32 upshift)
{
  assert((tgt && src) || !length);
 
  for (Lng32 i = 0; i < length; i++) {
    if (upshift)
      tgt[i] = na_towupper(src[i]);
    else
      tgt[i] = na_towlower(src[i]);
  }
  return 0;
}


NAWchar *na_wcsncpy (NAWchar * dest, const NAWchar * src, UInt32 n)
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

// inline. nawstring needs it.
//
//NAWchar *wcsncpy (NAWchar * dest, const NAWchar * src, unsigned int n)
//{
//  if (n != 0) {
//
//    NAWchar *d = dest;
//    const NAWchar *s = src;
//
//    do {
//      if ((*d++ = *s++) == 0) {
//         /* NUL pad the remaining n-1 bytes */
//         while (--n != 0)
//           *d++ = 0;
//         break;
//      }
//   } while (--n != 0);
// }
// return (dest);
//}


//NAWchar towupper (NAWchar wc)
//{
//   if ( L'a' <= wc && wc <= L'z' )
//      return wc - L'a' + L'A';
//   else 
//      return wc;
//}

//NAWchar towlower (NAWchar wc)
//{
//   if ( L'A' <= wc && wc <= L'Z' )
//      return wc - L'A' + L'a';
//   else 
//      return wc;
//}

NAWchar *na_wcschr (const NAWchar * wstr, NAWchar wc)
{
   NAWchar* p = (NAWchar*)wstr;

   if ( wc == 0 )
     return p+na_wcslen(wstr);

   while ( *p != (NAWchar)0 )
      if ( wc == *p )
         return p;
      else 
         p++;

   return NULL;
}

// like na_wcschr, except this variant skips over any text
// that is within matched parentheses (note that it will never
// find a left parenthesis as a consequence)
NAWchar *na_wcschrSkipOverParenText (const NAWchar * wstr, NAWchar wc)
{
   NAWchar* p = (NAWchar*)wstr;

   if ( wc == 0 )
     return p+na_wcslen(wstr);

   int ignoringTextDepth = 0;
   while ( *p != (NAWchar)0 )
     {
       if (ignoringTextDepth > 0)
         {
           if (*p == L')')
             ignoringTextDepth--;
           p++;
         }
       else if (*p == L'(')
         {
           ignoringTextDepth++;
           p++;
         }
       else if ( wc == *p )
         return p;
       else 
         p++;
     }

   return NULL;
}

NAWchar *na_wcsrchr (const NAWchar * wstr, NAWchar wc)
{
   NAWchar* p = (NAWchar*)wstr;

   if ( p == NULL ) return NULL;
   else p += na_wcslen(p);

   while ( p >= wstr )
      if ( wc == *p )
         return p;
      else
         p--;

   return NULL;
}

Int64 na_wcstoll (const NAWchar * wstr)
{
// pattern recognized: [ws]*[+|-]*[digit]*

 
   Int16 sign = 1;
   Int64 res = 0;
  
   while ( wstr && *wstr == L' ' ) wstr++;

   if ( wstr )
   {
     if (*wstr == L'-') {
        sign = -1; 
        wstr++;
     } else
     if (*wstr == L'+')
        wstr++;
   }

   while ( wstr && (L'0' <= *wstr && *wstr <= L'9') ) {
      res = res*10 + *wstr - L'0';
      wstr++;
   }

   return res * sign;
}

Lng32 na_wcstol (const NAWchar * wstr)
{
  // Get the value.
  Int64 val = na_wcstoll(wstr);
  
  // If the value is outside 32-bit range, set errno and return upper/lower bound.
  if (val > numeric_limits<Lng32>::max())
    {
      errno = ERANGE;
      return numeric_limits<Lng32>::max();
    }
  else if (val < numeric_limits<Lng32>::min())
    {
      errno = ERANGE;
      return numeric_limits<Lng32>::min();
    }
    
  return (Lng32)na_wcstoll(wstr);
}

double na_wcstod(const NAWchar *, NAWchar **)
{
   // not implemented!
   return (double)0;
}

NAWchar *na_wmemchr(const NAWchar *ws, NAWchar wc, Int32 n) 
{
   if (n != 0) {
     const NAWchar *p = ws;
     do {
        if (*p++ == wc)
           return (NAWchar*)p;
      } while (--n != 0);
   }
   return (NULL);
}

size_t na_wcstombs(char * p, const NAWchar* wp, size_t mx_p)
{
   if ( p == NULL ) return mx_p;
   size_t n =0;

   while (mx_p-- > 0 && (*p++ =(char)*wp)) {
     wp++; n++;
  } 
  return n;
}

size_t na_mbstowcs(NAWchar* wp, const char* p,  size_t mx_wp)
{
   if ( wp == NULL ) return mx_wp;
   size_t n =0;

   while (mx_wp-- > 0 && (*wp++ =(NAWchar)(unsigned char)*p)) {
     p++; n++;
  } 
  return n;
}

NAWchar* na_wcswcs(const NAWchar* wp1, const NAWchar* wp2)
{ 
   if ( wp2 == NULL || *wp2 == (NAWchar)0 ) 
      return (NAWchar*)wp1;

   Int32 len2 = na_wcslen(wp2);
   Int32 len1 = na_wcslen(wp1) - len2 + 1;

   // brute force search of wp2 in wp1
   for (Int32 i=0; i<len1; i++ ) {
      if ( na_wcsncmp(wp1+i, wp2, len2) == 0 )
         return (NAWchar*)(wp1+i);
   }

   return NULL;
}

#include "swscanf.cpp"
#include "swsprintf.cpp"




