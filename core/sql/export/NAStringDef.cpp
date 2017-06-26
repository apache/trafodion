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
****************************************************************************
*
* File:         NAStringDef.cpp (previously under /common)
* Description:
*
* Created:      5/6/98
* Language:     C++
*
*
*
****************************************************************************
*/

#include "Platform.h"


#include "NAAssert.h"

#include <iostream>

// -----------------------------------------------------------------------
//
// class NAString : derived from the source for FaceBook FBString
//                  --> adapted to take a NAMemory* in every ctor
//
// -----------------------------------------------------------------------

#include "NAStringDef.h"
#include "NAMemory.h"
#include "str.h"
#include <stdarg.h>

#include "str.h"
#include <fstream>
#ifdef NA_STD_NAMESPACE
using namespace std;
#endif // NA_STD_NAMESPACE

// we're using the multithread-safe versions here
#define MULTITHREAD_LOCK
#include <ctype.h>
#include <stdlib.h>

const UInt32 NA_HASH_SHIFT = 5;

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                              NAString                               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


NAString::NAString (NAMemory *h)                
  :fbstring_((h == NASTRING_UNINIT_HEAP_PTR) ? this->defaultHeapPtr() : h)
{}

NAString::NAString (NASize_T ic, NAMemory *h) 
  :fbstring_((h == NASTRING_UNINIT_HEAP_PTR) ? this->defaultHeapPtr() : h)
{
  fbstring_.reserve(ic);
}

NAString::NAString (const NASubString & substr, NAMemory *h)
  :fbstring_(substr.startData(), substr.length(), (h == NASTRING_UNINIT_HEAP_PTR) ? this->defaultHeapPtr() : h) {}


NAString::NAString (const NAString & S, NAMemory *h)
  :fbstring_(S.fbstring_, (h == NASTRING_UNINIT_HEAP_PTR) ? this->defaultHeapPtr() : h)      
{
  // if we're supposed to be on the same heap, then we can share
}

NAString::NAString (const char * cs, NAMemory *h)
  :fbstring_(cs, (h == NASTRING_UNINIT_HEAP_PTR) ? this->defaultHeapPtr() : h)        
{
  assert(cs!=nanil);
}

NAString::NAString (const char * cs, size_t N, NAMemory *h)
  :fbstring_(cs, N, (h == NASTRING_UNINIT_HEAP_PTR) ? this->defaultHeapPtr() : h) 
{
  assert(cs!=nanil);
} 

NAString::NAString (char c, size_t N, NAMemory *h)
  :fbstring_(N, c, (h == NASTRING_UNINIT_HEAP_PTR) ? this->defaultHeapPtr() : h)    
{}

// these three ctors are all the same
NAString::NAString(char c, NAMemory *h)
  :fbstring_((h == NASTRING_UNINIT_HEAP_PTR) ? this->defaultHeapPtr() : h)
{
  fbstring_.push_back(c);
}

NAString::NAString(unsigned char c, NAMemory *h)
  :fbstring_((h == NASTRING_UNINIT_HEAP_PTR) ? this->defaultHeapPtr() : h)
{
  fbstring_.push_back(char(c));
}

NAString::NAString(signed char c, NAMemory *h)
  :fbstring_((h == NASTRING_UNINIT_HEAP_PTR) ? this->defaultHeapPtr() : h)
{
  fbstring_.push_back(char(c));
}

NAString::NAString(const FBString & fbs, NAMemory *h)
  :fbstring_(fbs, (h == NASTRING_UNINIT_HEAP_PTR) ? this->defaultHeapPtr() : h)
{}

NAString::~NAString()
{}

NAString&
NAString::operator=(const char * cs)
{
  fbstring_ = cs;
  return *this;
}

NAString&
NAString::operator=(const NAString& str)
{
  fbstring_ = str.fbstring_;
  return *this;
}

/********************** Member Functions *************************/

NAString&
NAString::append(const char c, size_t rep)
{
  fbstring_.append(rep, c);
  return *this;
}

// Change the string capacity, returning the new capacity
size_t
NAString::capacity(size_t nc)
{
  if (nc > length() && nc != capacity())
    clone(nc);

  assert(capacity() >= length());
  return capacity();
}

// Erase the contents of a string
void
NAString::clear(void)
{
  fbstring_.clear();
}

// String comparisons
Int32
NAString::compareTo(const char* cs2, caseCompare cmp) const
{
  assert(cs2!=nanil);
  const char* cs1 = data();
  size_t len = length();
  size_t i = 0;
  if (cmp == exact) {
    for (; cs2[i]; ++i) {
      if (i == len) return -1;
      if (cs1[i] != cs2[i]) return ((cs1[i] > cs2[i]) ? 1 : -1);
    }
  } else {                  // ignore case
    for (; cs2[i]; ++i) {
      if (i == len) return -1;
#pragma nowarn(1506)   // warning elimination 
      char c1 = tolower((unsigned char)cs1[i]);
#pragma warn(1506)  // warning elimination 
#pragma nowarn(1506)   // warning elimination 
      char c2 = tolower((unsigned char)cs2[i]);
#pragma warn(1506)  // warning elimination 
      if (c1 != c2) return ((c1 > c2)? 1 : -1);
    }
  }
  return (i < len) ? 1 : 0;
}

Int32
NAString::compareTo(const NAString& str, caseCompare cmp) const
{
  const char* s1 = data();
  const char* s2 = str.data();
  size_t len = str.length();
  if (length() < len) len = length();
  if (cmp == exact) {
    return fbstring_.compare(str.fbstring_);
  } else {
    size_t i = 0;
    for (; i < len; ++i) {
#pragma nowarn(1506)   // warning elimination 
      char c1 = tolower((unsigned char)s1[i]);
#pragma warn(1506)  // warning elimination 
#pragma nowarn(1506)   // warning elimination 
      char c2 = tolower((unsigned char)s2[i]);
#pragma warn(1506)  // warning elimination 
      if (c1 != c2) return ((c1 > c2)? 1 : -1);
    }
  }
  // strings are equal up to the length of the shorter one.
  if (length() == str.length()) return 0;
  return (length() > str.length())? 1 : -1;
}


NAString
NAString::copy() const
{
  NAString temp(*this);	// Has increased reference count
  temp.clone();			// Distinct copy
  return temp;
}

int
NAString::extract(int begin, int end, NAString & target) const
{
  if(end >= length() || begin < 0 ||end < begin) 
      return -1;

  for(int i = begin; i <= end; i++)
      target.append((*this)[i]);

  return end - begin;
}


UInt32 
NAString::hashFoldCase() const
{
  UInt32 hv = (UInt32)length();    // Mix in the string length.
  UInt32 i  = hv;
  const unsigned char* p = (const unsigned char*) data();
  while (i--) {
    mash(hv, toupper(*p));
    ++p;
  }
  return hv;
}

void
NAString::mash(UInt32& hash, UInt32 chars) const
{
  hash = (chars ^
       ((hash << NA_HASH_SHIFT) |
        (hash >> (BITSPERBYTE*sizeof(UInt32) - NA_HASH_SHIFT))));
}

/*
 * Return a case-sensitive hash value.
 */
UInt32
NAString::hash() const
{
  UInt32 hv       = (UInt32)length(); // Mix in the string length.
  UInt32 i        = length()*sizeof(char)/sizeof(UInt32);
  const UInt32* p = (const UInt32*)data();
  {
    while (i--)
      mash(hv, *p++);			// XOR in the characters.
  }
  // XOR in any remaining characters:
  if ((i = length()*sizeof(char)%sizeof(UInt32)) != 0) {
    UInt32 h = 0;
    const char* c = (const char*)p;
    while (i--) 
      h = ((h << BITSPERBYTE*sizeof(char)) | *c++);
    mash(hv, h);
  }
  return hv;
}

UInt32
NAString::hash(caseCompare cmp) const
{
  return (cmp==exact) ? hash() : hashFoldCase();
}


static Int32
rwMemiEqual(const char* p, const char* q, size_t N)
{
  while (N--)
  {
    if (tolower((unsigned char)*p) != tolower((unsigned char)*q))
      return FALSE;
    p++; q++;
  }
  return TRUE;
}

// Pattern Matching:

size_t
NAString::index(const char* pattern,	// Pattern to search for
		 size_t plen,		// Length of pattern
		 size_t startIndex,	// Starting index from which to start
		 caseCompare cmp) const	// What type of case-comparison
{
  assert(pattern!=nanil);
  if (cmp == exact) {
    return fbstring_.find(pattern, startIndex, plen);
  } else {
    FBString tmp = fbstring_;
    register char* p = tmp.begin();
    register size_t N = tmp.length();
    while ( N-- ) { *p = tolower((unsigned char)*p); p++;}
    FBString pat(pattern);
    p = pat.begin();
    N = pat.length();
    while ( N-- ) { *p = tolower((unsigned char)*p); p++;}
    return tmp.find(pat.data(), startIndex, pat.length());
  }
}

// Prepend characters to self:
NAString&
NAString::prepend(char c, size_t rep)
{
  fbstring_.insert(0, rep, c);
  return *this;
}

// Remove at most n1 characters from self beginning at pos,
// and replace them with the first n2 characters of cs.
NAString&
NAString::replace(size_t pos, size_t n1, const char* cs, size_t n2)
{
  fbstring_.replace(pos, n1, cs, n2);
  return *this;
}

void NAString::adjustMemory(size_t tot)
{
  if (capacity() >= tot) return;
  fbstring_.reserve(tot);
}

// Truncate or add blanks as necessary
void
NAString::resize(size_t N)
{
  fbstring_.resize(N, ' ');			// Shrank; truncate the string
}

NAList<NAString> & NAString::split(char delim, NAList<NAString> & elems)
{
  int begin = 0;
  int end = 0;
  elems.clear();
  end = fbstring_.find_first_of(delim, begin);
  //For string like "w1|w2|w3|", devided by '|' we get 4 words
  //"w1", "w2", "w3", ""
  while(end >= begin)
  {
     NAString word(fbstring_.data() + begin,  end - begin);
     elems.insert(word);
     begin = end + 1;
     end = fbstring_.find_first_of(delim, begin);
  }
  if(fbstring_.size() > 0)
  {
    NAString end_word(fbstring_.data()+begin, fbstring_.size()-begin);
    elems.insert(end_word);
  }
  
  return elems;
}

Int32 NAString::readFile(ifstream& in) // Read to EOF or null character.
{
    char c;
    Int32 char_count = 0;
    fbstring_ = "";
    c = in.get();
    while(c != '\0' && c!=EOF)
    {
        fbstring_.append(1, c);
        c = in.get();
        char_count++;
    }
    return char_count;
}

Int32 NAString::readLine(ifstream& in)   // Read to EOF or newline.
{
    return readToDelim(in, '\n');
}

Int32 NAString::readToDelim(ifstream& in, char delim) // Read to EOF or delimitor.
{
    char c;
    Int32 char_count = 0;
    fbstring_ = "";
    c = in.get();
    while(c != '\0' && c!=delim && c!=EOF)
    {
        fbstring_.append(1, c);
        c = in.get();
        char_count++;
    }
    return char_count;
}

//This static function calculates and allocates buffer for the printf-like formatted string,
//and the formatted string is copied to the buffer.
char* NAString::buildBuffer(const char* formatTemplate, va_list args)
{
  // calculate needed bytes to allocate memory from system heap.
  int bufferSize = 20049;
  int msgSize = 0;
  //buffer is managed by this static function
  //the allocated memory is shared by all NAString objects.
  static THREAD_P char *buffer = NULL;
  va_list args2;
  va_copy(args2, args);
  bool secondTry = false;
  if (buffer == NULL)
      buffer = new char[bufferSize];
  // For messages shorter than the initial 20K limit, a single pass through
  // the loop will suffice.
  // For longer messages, 2 passes will do it. The 2nd pass uses a buffer of the
  // size returned by the call to vsnprintf. If a second pass is necessary, we
  // pass the copy of the va_list we made above, because the first call changes
  // the state of the va_list, and can cause a crash if reused.
  while(1)
  {
      msgSize = vsnprintf(buffer, bufferSize, formatTemplate, secondTry ? args2 : args);
      if ( msgSize < 0 ){
          //there is error, try second time
          if(!secondTry){
              secondTry = TRUE;
              continue;
          }
          else
            return NULL;//error second time
      }
      else if(msgSize < bufferSize) {
          // Buffer is large enough - we're good.
          break; 
      }
      else { 
          // Buffer is too small, reallocate it and go through the loop again.
          bufferSize = msgSize + 1; // Use correct size now.
          delete [] buffer;
          buffer = new char[bufferSize];
          secondTry = true;
      }
  }
  va_end(args2);
  return buffer;
}

NABoolean NAString::format(const char* formatTemplate...)
{
  NABoolean retcode;
  va_list args ;
  va_start(args, formatTemplate);

  char * buffer = buildBuffer(formatTemplate, args);
  if(buffer) {
    fbstring_ = buffer;
    retcode = TRUE;
  }
  else {
    fbstring_ = "";
    retcode = FALSE;
  }  
  va_end(args);
  return retcode;
}

// Return a substring of self stripped at beginning and/or end

NASubString NAString::strip (NAString::stripType st, char c)
{
  size_t start = 0;		// Index of first character
  size_t end = length();	// One beyond last character
  const char* direct = data();	// Avoid a dereference w dumb compiler

  assert((Int32)st != 0);
  if (st & leading)
    while (start < end && direct[start] == c)
      ++start;
  if (st & trailing)
    while (start < end && direct[end-1] == c)
      --end;
  if (end == start) start = end = NA_NPOS;  // make the null substring
  return NASubString(*this, start, end-start);
}


NASubString NAString::strip(NAString::stripType st, char c) const
{
  // Just use the "non-const" version, adjusting the return type:
  return ((NAString*)this)->strip(st,c);
}


// Change self to lower-case
void
NAString::toLower()
{
  cow();
  register size_t N = length();
  register char* p = fbstring_.begin();
#pragma nowarn(1506)   // warning elimination 
  while ( N-- ) { *p = tolower((unsigned char)*p); p++;}
#pragma warn(1506)  // warning elimination 
}

// Change self to upper case
void
NAString::toUpper()
{
  cow();
  register size_t N = length();
  register char* p = fbstring_.begin();
#pragma nowarn(1506)   // warning elimination 
  while ( N-- ) { *p = toupper((unsigned char)*p); p++;}
#pragma warn(1506)  // warning elimination 
}

// Change self to upper case
void
NAString::toUpper8859_1()
{
  cow();
  register size_t N = length();
  register unsigned char* p = (unsigned char *)(fbstring_.begin());
  while ( N-- ) 
  { 
    if ((isLower8859_1((unsigned char) *p)) &&
        (*p != 0xff) &&    // small y with diaeresis has no upcase equiv in 8859-1
        (*p != 0xdf))      // small german sharp s has no upcase euqiv in 8859-1
    {
#pragma nowarn(1506)   // warning elimination 
      *p = *p - 32; // convert to uppercase equivalent
#pragma warn(1506)  // warning elimination 
    }
    p++;
  }
}

char&
NAString::operator[](size_t i)
{
   assertElement(i); 
   cow();
   return fbstring_[i];
}

// Check to make a string index is in range
void
NAString::assertElement(size_t i) const
{
  if (i>=length() ) // NA_NPOS is > any legal length()
  {
    assert (i < length()) ;                                     
  }
}

/********************** Protected functions ***********************/

// Special constructor to initialize with the concatenation of a1 and a2:
NAString::NAString(const char* a1, size_t N1, const char* a2, size_t N2, NAMemory *h )
  //:fbstring_(a1, N1+N2, (h == NASTRING_UNINIT_HEAP_PTR) ? this->defaultHeapPtr() : h)
  :fbstring_((h == NASTRING_UNINIT_HEAP_PTR) ? this->defaultHeapPtr() : h)
{
  fbstring_ = FBString(a1, N1) + FBString(a2, N2);
  //fbstring_.replace(N1, N2, a2, N2);
}

void NAString::clobber(size_t nc)
{
  fbstring_.clear();
  fbstring_.reserve(nc);
}

// Make self a distinct copy; 
// preserve previous contents
void
NAString::clone()
{
  //indirectly call mutable_data()
  //to make self distinct
  fbstring_.begin();
}

// Make self a distinct copy with capacity of at least nc;
// preserve previous contents
void
NAString::clone(size_t nc)
{
  fbstring_.reserve(nc);
}

/****************** Related global functions ***********************/

SQLEXPORT_LIB_FUNC
NABoolean 
operator==(const NAString& s1, const char* s2)
{
  const char* data = s1.data();
  size_t len = s1.length();
  size_t i;
  for (i = 0; s2[i]; ++i)
    if (data[i] != s2[i] || i == len) return FALSE;
  return (i == len);
}

// Return a lower-case version of str:
SQLEXPORT_LIB_FUNC
NAString 
toLower(const NAString& str)
{
  register size_t N = str.length();
  NAString temp((char)0, N);
  register const char* uc = str.data();
  register       char* lc = (char*)temp.data();
  // Guard against tolower() being a macro:
#pragma nowarn(1506)   // warning elimination 
  while( N-- ) { *lc++ = tolower((unsigned char)*uc); uc++; }
#pragma warn(1506)  // warning elimination 
  return temp;
}

// Return an upper-case version of str:
SQLEXPORT_LIB_FUNC
NAString 
toUpper(const NAString& str)
{
  register size_t N = str.length();
  NAString temp((char)0, N);
  register const char* uc = str.data();
  register       char* lc = (char*)temp.data();
  // Guard against toupper() being a macro:
#pragma nowarn(1506)   // warning elimination 
  while( N-- ) { *lc++ = toupper((unsigned char)*uc); uc++; }
#pragma warn(1506)  // warning elimination 
  return temp;
}

SQLEXPORT_LIB_FUNC
NAString 
operator+(const NAString& s, const char* cs)
{
  // Use the special concatenation constructor:
  return NAString(s.data(), s.length(), cs, strlen(cs), s.heap());
}            

SQLEXPORT_LIB_FUNC
NAString 
operator+(const NAString& s, const char c)
{
  // Use the special concatenation constructor:
  NAString temp(c);
  return NAString(s.data(), s.length(), temp.data(), temp.length(), s.heap());
}            

SQLEXPORT_LIB_FUNC
NAString 
operator+(const char* cs, const NAString& s)
{
  // Use the special concatenation constructor:
  return NAString(cs, strlen(cs), s.data(), s.length(), s.heap());
}

SQLEXPORT_LIB_FUNC
NAString 
operator+(const NAString& s1, const NAString& s2)
{
  // Use the special concatenation constructor:
  return NAString(s1.data(), s1.length(), s2.data(), s2.length(), s1.heap());
}

/* static */ UInt32
NAString::hash(const NAString& str)
{
  return str.hash();
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                             NASubString                             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

/*
 * A zero lengthed substring is legal.  It can start
 * at any character.  It is considered to be "pointing"
 * to just before the character.
 *
 * A "null" substring is a zero lengthed substring that
 * starts with the nonsense index NA_NPOS.  It can
 * be detected with the member function isNull().
 */

// Private constructor 
NASubString::NASubString(const NAString & str, size_t start, size_t nextent)
: str_((NAString*)&str),
  begin_(start),
  extent_(nextent)
{
#ifndef NDEBUG
  size_t N = str.length();

  // Allow zero lengthed and null substrings:
  if ((start==NA_NPOS && nextent!=0) || (start!=NA_NPOS && start+nextent>N))
    subStringError(N, start, nextent);
#endif
}

// Sub-string operator
NASubString 
NAString::operator()(size_t start, size_t len) 
{
  return NASubString(*this, start, len);
}

/*
 * Returns a substring matching "pattern", or the null substring 
 * if there is no such match.  It would be nice if this could be yet another 
 * overloaded version of operator(), but this would result in a type
 * conversion ambiguity with operator(size_t, size_t). 
 */
NASubString
NAString::subString(const char* pattern, size_t startIndex, caseCompare cmp)
{
  assert(pattern!=nanil);
  size_t len = strlen(pattern);
  size_t i = index(pattern, len, startIndex, cmp);
  return NASubString(*this, i, i == NA_NPOS ? 0 : len);
}
char&
NASubString::operator[](size_t i)
{
   assertElement(i); 
   str_->cow();
   return (*str_)[begin_+i];
}

char&
NASubString::operator()(size_t i)
{ 
#ifndef NDEBUG
   assertElement(i);
#endif
   str_->cow();
   return (*str_)[begin_+i];
}

NASubString
NAString::operator()(size_t start, size_t len) const
{
  return NASubString(*this, start, len);
}

NASubString
NAString::subString(const char* pattern, size_t startIndex, caseCompare cmp) const
{
  assert(pattern!=nanil);
  size_t len = strlen(pattern);
  size_t i = index(pattern, len, startIndex, cmp);
  return NASubString(*this, i, i == NA_NPOS ? 0 : len);
}

NASubString&
NASubString::operator=(const NAString& str) 
{
  if( !isNull() ) {
    size_t len = str.length();
    str_->replace(begin_, extent_, str.data(), len);
    extent_ = len;
  }
  return *this;
}

NASubString&
NASubString::operator=(const NASubString& s) 
{
  if( !isNull() ) {
    str_->replace(begin_, extent_, s.data(), s.length());
    extent_ = s.length();
  }
  return *this;
}

NASubString&
NASubString::operator=(const char* cs)
{
  if (!isNull() ) {
    size_t len = strlen(cs);
    str_->replace(begin_, extent_, cs, len);
    extent_ = len;
  }
  return *this;
}

SQLEXPORT_LIB_FUNC
NABoolean 
operator==(const NASubString& ss, const char* cs)
{
  assert(cs!=nanil);

  if ( ss.isNull() ) return *cs =='\0'; // Two null strings compare equal

  assert(ss.begin_+ss.extent_<=ss.str_->length());

  const char* data = ss.str_->data() + ss.begin_;
  size_t i;
  for (i = 0; cs[i]; ++i)
    if (cs[i] != data[i] || i == ss.extent_) return FALSE;
  return (i == ss.extent_);
}

SQLEXPORT_LIB_FUNC
NABoolean 
operator==(const NASubString& ss, const NAString& s)
{
  if (ss.isNull()) return s.isNull(); // Two null strings compare equal.
  if (ss.extent_ != s.length()) return FALSE;
  return !memcmp(ss.str_->data() + ss.begin_, s.data(), ss.extent_);
}

SQLEXPORT_LIB_FUNC
NABoolean 
operator==(const NASubString& s1, const NASubString& s2)
{
  if (s1.isNull()) return s2.isNull();
  if (s1.extent_ != s2.extent_) return FALSE;
  return !memcmp(s1.str_->data()+s1.begin_, s2.str_->data()+s2.begin_, s1.extent_);
}

// Convert self to lower-case
void
NASubString::toLower()
{
  if(!isNull())
  {				// Ignore null substrings
     str_->cow();
     register char* p = (char*)(str_->data() + begin_); // Cast away constness
     size_t N = extent_;
#pragma nowarn(1506)   // warning elimination 
     while( N-- ) { *p = tolower((unsigned char)*p); p++;}
#pragma warn(1506)  // warning elimination 
  }
}

// Convert self to upper-case
void
NASubString::toUpper()
{
  if(!isNull())
  {				// Ignore null substrings
     str_->cow();
     register char* p = (char*)(str_->data() + begin_); // Cast away constness
     size_t N = extent_;
#pragma nowarn(1506)   // warning elimination 
     while( N-- ) { *p = toupper((unsigned char)*p); p++;}
#pragma warn(1506)  // warning elimination 
  }
}

void
NASubString::subStringError(size_t sr, size_t start, size_t n) const
{
  assert (FALSE) ; 
}

void
NASubString::assertElement(size_t i) const
{
  if (i>=length() ) // NA_NPOS is > any legal length()
  {
    assert ( i < length() ) ;
  }
}


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                           LOCALE RELATED                             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

NABoolean
NAString::isAscii() const
{
  const char* cp = data();
  for (size_t i = 0; i < length(); ++i)
    if (cp[i] & ~0x7F)
      return FALSE;
  return TRUE;
}

#ifndef RW_NO_LOCALE

NAString 
strXForm(const NAString& cstr)
{
  // Get the size required to transform the string;
  // cast to "char*" necessary for Sun cfront:
  size_t N = ::strxfrm(NULL, (char*)cstr.data(), 0) + 1;

  NAString temp((char)0, N);

  // Return null string in case of failure:
  if ( ::strxfrm((char*)temp.data(), (char*)cstr.data(), N) >= N )
    return NAString();

  return temp;
}

size_t
NAString::mbLength() const 
{
  const char* cp = data();
  size_t i = 0;
  size_t len = 0;
  mblen((const char*)0, MB_CUR_MAX);  // clear static state (bleah!)
  while (i < length() && cp[i]) {
    Int32 l = mblen(cp+i, MB_CUR_MAX);
    if (l <= 0) return NA_NPOS;
    i += l;
    ++len;
  }
  if (i > length()) return NA_NPOS; // incomplete last char
  return len;
}

#endif /* RW_NO_LOCALE */

/*
 * Warning: certain implementations of iostreams have been
 * found to reset the stream width after ostream::write() is called.
 * In the function below, the characters are output directly through
 * the streambuf.
 */

SQLEXPORT_LIB_FUNC
ostream& 
operator<<(ostream& os, const NAString& s)
{
  // ANSI replaced opfx/osfx to implicit calls in the ctor/dtor 
  // of the sentry class
  if (!os.good()) return os;

  ostream::sentry opfx(os);
  if(opfx)
  {
    size_t len = s.length();
    size_t wid = os.width();
    wid = (len < wid) ? wid - len : 0;
    Lng32 flags = os.flags();
#pragma nowarn(1506)   // warning elimination 
    os.width(wid);
#pragma warn(1506)  // warning elimination 
    if (wid && !(flags & ios::left))
      os << "";  // let the ostream fill
#pragma nowarn(1506)   // warning elimination 
    os.rdbuf()->sputn((char*)s.data(), s.length());
#pragma warn(1506)  // warning elimination 
    if (wid && (flags & ios::left))
      os << "";  // let the ostream fill


  // no need to destroy sentry: it will go out of scope.
  }
  return os;
}

