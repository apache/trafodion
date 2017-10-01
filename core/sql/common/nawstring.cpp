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

#include "nawstring.h"
#include "NLSConversion.h"
#include "charinfo.h"

#define MULTITHREAD_LOCK

#include "NAAssert.h"
#include "str.h"

/*
 * Initialize for non 16-bit Windows DLL versions of the libary.
 * Otherwise, the data must be managed by the instance manager.
 */

#if !defined(__DLL__) || !defined(__WIN16__)

// Static member variable initialization:
const size_t          NAWString::initialCapac     = 15;
const size_t          NAWString::resizeInc        = 16;
const size_t          NAWString::freeboard        = 15;

#endif

void NAWString::initFromSingleByteString(Lng32 charset, const char* str, size_t N, NAMemory *h)
{
   assert(str!=nanil);
   NAWchar buf[N+1];
   Lng32 wcs = LocaleStringToUnicode(charset, (char*)str, N, buf, N, FALSE);
   buf[wcs] = 0;
   append(buf, wcs);
}

void NAWString::initFromVariableWidthMultiByteString(Lng32 charset,
                                                     const char* str,
                                                     size_t N,
                                                     NAMemory *h)
{
   Int32 errorcode;
   assert(str!=nanil);
   // if h is uninitialized, then use the (derived) string class's
   // default h instead
   NAMemory * realHeap = (h == NASTRING_UNINIT_HEAP_PTR) ?
                this->defaultHeapPtr () : h ;
   charBuf strCharBuf((unsigned char*)str, N /* str len in bytes */);
   NAWcharBuf *pWcharBuf = NULL; // request for new NAWcharBuf(fer) allocation
   pWcharBuf = csetToUnicode(strCharBuf, realHeap, pWcharBuf, charset, errorcode);
   size_t Nwchars = pWcharBuf->getStrLen(); // str len in number of NAWchar's
   append(pWcharBuf->data(), Nwchars);
   NADELETE(pWcharBuf, NAWcharBuf, realHeap);
}

NAWString::NAWString(Lng32 charset, const char* str, NAMemory *h)
  :fbwstring_((h == NASTRING_UNINIT_HEAP_PTR)?this->defaultHeapPtr () : h)
{
  // if h is uninitialized, then use the (derived) string class's
  // default h instead
  NAMemory * realHeap = (h == NASTRING_UNINIT_HEAP_PTR) ?
               this->defaultHeapPtr () : h ;
  
  size_t N = strlen(str);
  if (CharInfo::isVariableWidthMultiByteCharSet((CharInfo::CharSet)charset))
    initFromVariableWidthMultiByteString(charset, str, N, realHeap);
  else
    initFromSingleByteString(charset, str, N, realHeap);
}

NAWString::NAWString(Lng32 charset, const char* str, size_t N, NAMemory *h)
  :fbwstring_((h == NASTRING_UNINIT_HEAP_PTR)?this->defaultHeapPtr () : h)
{
  // if h is uninitialized, then use the (derived) string class's
  // default h instead
  NAMemory * realHeap = (h == NASTRING_UNINIT_HEAP_PTR) ?
               this->defaultHeapPtr () : h ;
  if (CharInfo::isVariableWidthMultiByteCharSet((CharInfo::CharSet)charset))
    initFromVariableWidthMultiByteString(charset, str, N, realHeap);
  else
    initFromSingleByteString(charset, str, N, realHeap);
}

NAWString::NAWString(const NAWchar* wstr, size_t N, NAMemory *h)
  :fbwstring_((FBWString::value_type*)wstr , N, (h == NASTRING_UNINIT_HEAP_PTR)?this->defaultHeapPtr () : h)
{
   assert(wstr!=nanil);
}

NAWString::NAWString(const NAWString& ws, NAMemory *h)
  :fbwstring_(ws.fbwstring_, (h == NASTRING_UNINIT_HEAP_PTR)?this->defaultHeapPtr () : h)
{
}

NAWString::NAWString(const NAWchar* wstr, NAMemory *h)
  :fbwstring_((FBWString::value_type*)wstr, na_wcslen(wstr), (h == NASTRING_UNINIT_HEAP_PTR)?this->defaultHeapPtr () : h)
{
  assert(wstr!=nanil);
}

NAWString::NAWString(NAWchar wc, NAMemory *h)
  :fbwstring_(1, (FBWString::value_type)wc, (h == NASTRING_UNINIT_HEAP_PTR)?this->defaultHeapPtr () : h)
{
}

NAWString::NAWString(NAMemory *h)
  :fbwstring_((h == NASTRING_UNINIT_HEAP_PTR)?this->defaultHeapPtr () : h)
{
}

NAWString::~NAWString() 
{
}


NAWString& NAWString::operator=(const NAWchar* wstr)
{
  fbwstring_.assign((FBWString::value_type*)wstr);
  return *this;
}

// Remove at most n1 characters from self beginning at pos,
// and replace them with the first n2 characters of cs.
NAWString&
NAWString::replace(size_t pos, size_t n1, const NAWchar* cs, size_t n2)
{
  assert(!(pos > length())); // NAWString::replace: position > length()
  fbwstring_.replace(pos, n1, (FBWString::value_type*)cs, n2);
  return *this;
}

NAWString operator+(const NAWString& s1, const NAWString& s2)
{
 // Use the special concatenation constructor:
  return NAWString(s1.data(), s1.length(), s2.data(), s2.length(), s1.heap());
}

// Special constructor to initialize with the concatenation of a1 and a2:
NAWString::NAWString(const NAWchar* a1, size_t N1, const NAWchar* a2, size_t N2, NAMemory *h)
:fbwstring_((FBWString::value_type*)a1, N1+N2, (h == NASTRING_UNINIT_HEAP_PTR) ? this->defaultHeapPtr() : h)
{
  fbwstring_.replace(N1, N2, (FBWString::value_type*)a2, N2);
}

size_t NAWString::adjustCapacity(size_t nc)
{
  size_t ic = getInitialCapacity();
  if (nc<=ic) return ic;
  size_t rs = getResizeIncrement();
  return (nc - ic + rs - 1) / rs * rs + ic;
}

// Converted the internal-format string literal used by the parser to
// the external-format (quoted) string used by the user. Encloses internalStr
// in single quotes and changes embedded quotes to 2 consecutive quotes
NAWString NAWString::ToQuotedWString()
{
  if (length() == 0 || (length() == 1 && (NAWchar)fbwstring_[0] == L'\0'))
    return NAWString(L"\'\'", heap());

  NAWString quotedStr(L'\'', heap());

  for (StringPos i = 0; i < length(); i++)
    {
      quotedStr += (*this)[i];
      if ((*this)[i] == NAWchar('\'')) quotedStr += NAWchar('\'');
    }
  quotedStr += NAWchar('\'');

  return quotedStr;
}

// The following method was derived from method NAString::index() defined in w:/export/NAStringDef.cpp
// so when one fixes a bug in NAString::index(), one should fix this NAWString::index version also.
size_t NAWString::index(const NAWchar* pattern, size_t patLen, size_t startIndex, NAString::caseCompare cmp) const
{
  if ( pattern == NULL || patLen == 0 || *pattern == static_cast<NAWchar>(0) ) 
    return startIndex;

  size_t slen = length();
  if (slen < startIndex + patLen)
    return NA_NPOS;

  slen -= startIndex + patLen;
  const NAWchar* sp = data() + startIndex;
  if (cmp == NAString::exact)
  {
    NAWchar first = *pattern;
    for (size_t i = 0; i <= slen; ++i)
      if (sp[i] == first && NAWstrncmp(sp+i+1, pattern+1, patLen-1) == 0)
        return i + startIndex;
  }
  else // NAString::ignoreCase
  {
    // na_towlower() only lowers letters specified in 7-bit US ASCII
    NAWchar firstCharInLowerCase = na_towlower(*pattern);
    for (size_t ii = 0; ii <= slen; ii++)
      if (na_towlower(sp[ii]) == firstCharInLowerCase &&
	  NAWstrincmp(sp+ii+1, pattern+1, patLen-1) == 0)
	return ii + startIndex;
  }

  return NA_NPOS;
}

// -----------------------------------------------------------------------
// Adapted from w:/common/NAString.cpp
// Returns TRUE if the string consists entirely of whitespace
// (zero or more spaces or tabs, and nothing else), including none (empty str).
// -----------------------------------------------------------------------


// -----------------------------------------------------------------------
// newNAWcharBuffer()
//
// Adapted from convertNAString() in w:/common/NAString.cpp
//
// Allocates space (including an extra NAWchar for the NULL character
// terminator) in the specified heap or the process C/C++ runtime
// heap (if the specified heap pointer is a NULL pointer) and then
// DEEP-copies the contents of the NAWString parameter naws to the
// newly allocated buffer (including the NULL character terminator).
//
// It is the caller responsibility to deallocate the returned the
// newly allocated buffer when the buffer is no longer needed.
// Use the NADELETEBASIC(returned_NAWchar_star_pointer, heap_pointer)
// call (C macro expansion/invocation) to deallocate the buffer.
// -----------------------------------------------------------------------
NAWchar * newNAWcharBuffer(const NAWString& naws, CollHeap *heap)
{
  size_t len = naws.length();
  NAWchar* buf = NULL;

  if (heap)
    buf = new (heap) NAWchar[len + 1];
  else
  {
    buf = new NAWchar[len + 1];
  }
  NAWstrncpy(buf, naws.data(), len);
  buf[len] = NAWCHR('\0');
  return buf;
}

// -----------------------------------------------------------------------
// newNAWcharBufferContainingAnEmptyNAWString()
//
// Adapted from copyString() in w:/sqlcat/realReadArk.cpp
//
// Allocates space for one or two NAWchar's to contain the
// NAWchar NULL character (respresenting on empty string)
// in the specified heap or the process C/C++ runtime
// heap (if the specified heap pointer is a NULL pointer)
// and then set the contents of newly allocated buffer to
// the NAWchar NULL character encoding value.
//
// It is the caller responsibility to deallocate the returned the
// newly allocated buffer when the buffer is no longer needed.
// Use the NADELETEBASIC(returned_NAWchar_star_pointer, heap_pointer)
// call (C macro expansion/invocation) to deallocate the buffer.
// -----------------------------------------------------------------------
NAWchar * newNAWcharBufferContainingAnEmptyString(CollHeap *heap)
{
  NAWchar* buf = NULL;

  if (heap)
    buf = new (heap) NAWchar[1];
  else
  {
    buf = new NAWchar[1];
  }

  *buf = NAWCHR('\0');

  return buf;
}

// -----------------------------------------------------------------------
// Remove whitespace (spaces and tabs) from front or back or both
// -----------------------------------------------------------------------
void TrimNAWStringSpace(NAWString& ns, NAString::stripType eStripType) // default is NAString::trailing
{
  StringPos i;

  if (eStripType == NAString::trailing || eStripType == NAString::both) {
    if (i = ns.length()) {			// assign followed by compare
      for ( ; i--; )
        if (!isSpace8859_1(ns[i]))
          break;
      ns.remove(++i);
    }
  }

  if (eStripType == NAString::leading || eStripType == NAString::both) {
    for (i=0; i<ns.length(); i++)
      if (!isSpace8859_1(ns[i]))
        break;
    if (i)
      ns.remove(0, i);
  }
}
