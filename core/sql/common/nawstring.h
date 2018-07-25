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
#ifndef NAWSTRING_H
#define NAWSTRING_H
/* -*-C++-*-
******************************************************************************
*
* File:         NAWString.h
* RCS:          $Id: nawstring.h,v 1.2 1998/08/10 15:33:50  Exp $
* Description:  common definitions of basic data types
*
* Created:      4/27/94
* Modified:     $Date: 2001/03/13 15:33:
* Language:     C++
* Status:       $State: Exp $
*
*
******************************************************************************
*/

#include "Platform.h"
#include "NAWinNT.h"
#include "NAStringDef.h"

// -----------------------------------------------------------------------
// a Unicode string datatype
// -----------------------------------------------------------------------

// Note NAWString always put a NAWchar null at the end of the string. So we 
// count in the trailing NAWchar null here.
#define NAW_TO_NASTRING(w)	(const char *)w.data(), (w.length()+1) * BYTES_PER_NAWCHAR

class NAWString : public NABasicObject
{
public:
  
   virtual NAMemory * defaultHeapPtr () const
   { return NABasicObject::systemHeapPtr() ; } // fyi, this value is 0x0

#if  __SIZEOF_WCHAR_T__ == 2
   typedef folly::basic_fbstring<char16_t> FBWString;
#else
   typedef folly::basic_fbstring<NAWchar> FBWString;
#endif

   NAWString(Lng32 charset, const char* str, NAMemory *h=NAWSTRING_UNINIT_HEAP_PTR);
   NAWString(Lng32 charset, const char* str, size_t, NAMemory *h=NAWSTRING_UNINIT_HEAP_PTR);
   NAWString(const NAWchar*, size_t, NAMemory *h=NAWSTRING_UNINIT_HEAP_PTR);
   NAWString(const NAWchar*, NAMemory *h=NAWSTRING_UNINIT_HEAP_PTR);
   NAWString(const NAWString &, NAMemory *h=NAWSTRING_UNINIT_HEAP_PTR);
   NAWString(NAWchar, NAMemory *h=NAWSTRING_UNINIT_HEAP_PTR);
   NAWString(NAMemory *h=NAWSTRING_UNINIT_HEAP_PTR);
   ~NAWString();

   NABoolean operator==(const NAWchar * s2) const
   { return fbwstring_.compare((FBWString::value_type*)s2) == 0; }

   operator const NAWchar*() const
   { return (const NAWchar*)fbwstring_.data(); }

   NAWString&   operator=(const NAWString& rhs) // Replace string
   { fbwstring_.replace(0, length(), (FBWString::value_type*)rhs.data(), rhs.length()); return *this; }
   NAWString&   operator=(const NAWchar*);      // Replace string
   NAWString&   operator+=(NAWchar wc)
   { fbwstring_ += (FBWString::value_type)wc; return *this; }

   NAWchar&     operator[](size_t i) { return (NAWchar&)fbwstring_[i]; } ;

   size_t       index(const NAWchar* pat, size_t patLen, size_t i=0, NAString::caseCompare cmp = NAString::exact) const;
   size_t       index(const NAWString& pat, size_t i=0, NAString::caseCompare cmp = NAString::exact) const
   { return index(pat.data(), pat.length(), i, cmp); }

   NAWString&   append(const NAWString& s)
   { fbwstring_.replace(length(), 0, (FBWString::value_type*)s.data(), s.length());return *this; }

   NAWString&   append(const NAWchar* cs, size_t N)
   { fbwstring_.replace(length(), 0, (FBWString::value_type*)cs, N); return *this;}

   NAWString&   remove(size_t pos)
   { fbwstring_.replace(pos, length()-pos, nanil, 0);return *this; }

   // Remove n chars starting at pos
   NAWString&   remove(size_t pos, size_t n)  
   { fbwstring_.replace(pos, n, nanil, 0); return *this;}

   NAWString& replace(size_t pos, size_t n1, const NAWchar* cs, size_t n2);
   const NAWchar* data() const { return (NAWchar*)fbwstring_.data(); }

   size_t length() const {  return fbwstring_.length(); }

   NAWString ToQuotedWString();

friend
   NAWString operator+(const NAWString& s1, const NAWString& s2);

friend
NABoolean operator==(const NAWString& s1, const NAWString& s2);

friend
NABoolean operator< (const NAWString& s1, const NAWString& s2);

friend
NABoolean operator> (const NAWString& s1, const NAWString& s2);

protected:
   NAWString(const NAWchar* a1, size_t N1, 
             const NAWchar* a2, size_t N2, NAMemory *h);

   void initFromSingleByteString(Lng32 charset, const char* str, size_t N, NAMemory *h);
   void initFromVariableWidthMultiByteString(Lng32 charset, const char* str, size_t N, NAMemory *h);

private:

  // ====================================================================
  // Capacity, increment and max waste are all expressed in UCS2 units
  // ====================================================================
  size_t  capacity() const {return fbwstring_.capacity()/BYTES_PER_NAWCHAR;}
  static size_t         adjustCapacity(size_t nc);
  static size_t         initialCapacity(size_t ic = 15);        // Initial allocation Capacity
  static size_t         maxWaste(size_t mw = 15);               // Max empty space before reclaim
  static size_t         resizeIncrement(size_t ri = 16);        // Resizing increment

  static size_t         getInitialCapacity()    {return initialCapac;}

  static size_t         getResizeIncrement()    {return resizeInc;}

  static size_t         getMaxWaste()           {return freeboard;}

 static const size_t initialCapac;           // Initial allocation Capacity
 static const size_t resizeInc;              // Resizing increment
 static const size_t freeboard;              // Max empty space before reclaim

  NAMemory *    heap() const { return fbwstring_.heap() ; }

  FBWString fbwstring_;
}; //NAWString

// NAWString Logical operators:
inline NABoolean operator==(const NAWString& s1, const NAWString& s2)
                           { return s1.fbwstring_ == s2.fbwstring_; }

inline NABoolean operator< (const NAWString& s1, const NAWString& s2)
                           { return s1.fbwstring_ < s2.fbwstring_; }
//#endif // :cnu

inline NABoolean operator!=(const NAWString& s1, const NAWString& s2)
                           { return !(s1 == s2); }

inline NABoolean operator> (const NAWString& s1, const NAWString& s2)
                           { return s1.fbwstring_ > s2.fbwstring_; }

inline NABoolean operator<=(const NAWString& s1, const NAWString& s2)
                           { return !(s1 > s2); }

inline NABoolean operator>=(const NAWString& s1, const NAWString& s2)
                           { return !(s1 < s2); }


// -----------------------------------------------------------------------
// Returns TRUE if the string consists entirely of whitespace
// (zero or more spaces or tabs, and nothing else), including none (empty str).
// -----------------------------------------------------------------------
NABoolean IsNAWStringSpaceOrEmpty(const NAWString& naws);

// -----------------------------------------------------------------------
// Remove whitespace (spaces and tabs) from front or back or both
// -----------------------------------------------------------------------
void TrimNAWStringSpace(NAWString& naws, NAString::stripType s = NAString::trailing);

// -----------------------------------------------------------------------
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
NAWchar * newNAWcharBuffer(const NAWString& naws, CollHeap *heap = NULL);

// -----------------------------------------------------------------------
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
NAWchar * newNAWcharBufferContainingAnEmptyString(CollHeap *heap = NULL);

// // -----------------------------------------------------------------------
// // A simple API (abeit inefficient) to convert a C string from ISO88591
// // encoding format to UCS2/UTF16 encoding format.  The specified work
// // heap is used for allocating temporary storage buffers during the
// // execution of the routine - The default is to use the process heap.
// //
// // The returned NAWString object always uses the C/C++ runtime process
// // heap (regardless whether the working heap is specified or not).
// //
// // This function relates to the function Latin1StrToUTF8 declared in
// // the header file w:/common/NAString.h.
// //
// // 3/21/2011  Comment out the function because it is not currently used.
// // -----------------------------------------------------------------------
// NAWString Latin1StrToUCS2(const NAString & latin1Str, CollHeap * workHeap = NULL);
#endif 
