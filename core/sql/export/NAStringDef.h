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
* File:         NAStringDef.h (previously under /common)
* RCS:          $Id: NAStringDef.h,v 1.2 1998/07/30 06:44:34  Exp $
* Description:  This file contains the class definition for class NAString
*
* Created:      5/6/98
* Modified:     $ $Date: 1998/07/30 06:44:34 $ (GMT)
* Language:     C++
* Status:       $State: Exp $
*
*
*
****************************************************************************
*/

#ifndef __NASTRING_DEF_H__
#define __NASTRING_DEF_H__

#include "Platform.h"

#ifdef NA_STD_NAMESPACE
#include <iosfwd>
using namespace std;
#endif 

// -----------------------------------------------------------------------
//
// class NAString : derived from the source for Facebook's FBString
//                  --> adapted to take a NAMemory* in every ctor
//
// -----------------------------------------------------------------------

// $$$ NB: if we write our own (simple) c-string functionality, we can
// (almost) use NAString in the executor (see the bottom of this file
// for the two functions we need to #ifdef to allow this)
// --> goal : someday (soon?) we will no longer include the following file:
#include <string.h> 

#include "NABasicObject.h"
#include "NAStringDefGlobals.h" /* NAReference, NAStringRef */
#include "NAAssert.h"

#include "FBString.h"

#ifndef __EID
// no use for this in EID
#include "ComGuardianFileNameParts.h"
#endif
#include "Collections.h"

// -----------------------------------------------------------------------
// definitions we use to stay as compatible with the RWCString class, and
// usage of the RWCString class, as possible
// -----------------------------------------------------------------------
#define         NASize_T    size_t 
const   size_t  NA_NPOS  =  ~(size_t)0; // rw/defs.h : RW_NPOS ;  
#define         nanil       0           // rw/defs.h : rwnil      

// -----------------------------------------------------------------------
// index into a string
// -----------------------------------------------------------------------
typedef size_t    StringPos;

// -----------------------------------------------------------------------
// redundant defn (see BaseTypes.h) in order to reduce NAString's
// dependencies ...
// -----------------------------------------------------------------------
#ifndef MAXOF
#define MAXOF(X,Y) (X >= Y ? X : Y) 
#endif
#ifndef MINOF
#define MINOF(X,Y) (X <= Y ? X : Y)
#endif
#ifndef MIN_ONE /* denoting "at least one ..." */
#define MIN_ONE(X) MAXOF(X,1)
#endif




// the source someday should be changed to use only one of these --
// for now, we fudge it
#define NAPRECONDITION2(b,s) NAPRECONDITION(b,s)
#define NAPOSTCONDITION(b)   NAPRECONDITION(b,"" #b "")
#define NAASSERT(b)          NAPRECONDITION(b,"" #b "")

#define NAPRECONDITION(b,s) \
{ if (!(b)) NAAssert(s, __FILE__, __LINE__); }

// Number of bits per byte 
const size_t BITSPERBYTE = 8;  

// Check pre- and post-conditions used in 
// /export/NAStringDef.cpp and /common/rawstring.cpp
// ps: Copied from rw/defs.h
#if defined(NA_C89_VERSION3)
#include <assert.h>
#define RWPRECONDITION(a)     assert( (a) != 0 ) 
#define RWPOSTCONDITION(a)    assert( (a) != 0 )
#define RWPRECONDITION2(a,b)  assert((b, (a) !=0))
#define RWPOSTCONDITION2(a,b) assert((b, (a) !=0))
#define RWASSERT(a)           assert( (a) != 0 )
#endif

// classes in this file:
class NASubString ; // rewrite of RWCSubString, from cstring.h/cstring.cpp
class NAString ;    // rewrite of RWCString,    from cstring.h/cstring.cpp


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                             NASubString                              //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

/*
 * The NASubString class allows selected elements to be addressed.
 * There are no public constructors.
 */


class SQLEXPORT_LIB_FUNC NASubString
{
public:
  NASubString(const NASubString& sp)
         : str_(sp.str_), begin_(sp.begin_), extent_(sp.extent_) {;}

  NASubString& operator=(const char*);         // Assignment to char*
  NASubString& operator=(const NAString&);    // Assignment to NAString
  NASubString& operator=(const NASubString&); // Assignment to NASubString
  char&         operator()(size_t i);           // Index with optional bounds checking
  char&         operator[](size_t i);           // Index with bounds checking

  char          operator()(size_t i) const ;

  char          operator[](size_t i) const ;

  
  const char*   startData() const;      // Replaces data(). See definition below.
  // startData() will remain undocumented.  Please don't even ask.
  // Use at your own risk. It may be deprecated in the future.

  // DON'T USE THE FUNCTION BELOW!
  
  const char*   data() const;
  // This member is deprecated and will be removed in a future version.
  // It remains public only to maintain source compatibility.
  // Since NASubString works by referencing the NAString's data,
  // if you attempt to directly use the data() member of the NAString,
  // you will very likely be surprised by the result, which will be null
  // terminated not at the extent of the substring,
  // but at the end of the NAString.

  size_t        length() const          {return extent_;}
  size_t        start() const           {return begin_;}
  void          toLower();              // Convert self to lower-case
  void          toUpper();              // Convert self to upper-case

  // For detecting null substrings:
  NABoolean     isNull() const          {return begin_==NA_NPOS;}
  Int32           operator!() const       {return begin_==NA_NPOS;}

protected:

  void          subStringError(size_t, size_t, size_t) const;
  void          assertElement(size_t i) const;  // Verifies i is valid index

private:

  // NB: the only constructor is private:
  NASubString(const NAString & s, size_t start, size_t len);

  NAString*    str_;           // Referenced string
  size_t              begin_;   // Index of starting character
  size_t              extent_;// Length of NASubString

friend
SQLEXPORT_LIB_FUNC
NABoolean operator==(const NASubString& s1, const NASubString& s2);

friend
SQLEXPORT_LIB_FUNC
NABoolean operator==(const NASubString& s1, const NAString& s2);

friend
SQLEXPORT_LIB_FUNC
NABoolean operator==(const NASubString& s1, const char* s2);

friend class NAString;

};



//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                              NAString                                //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
//
// NB: classes deriving from NAString can change just the
// 'defaultHeapPtr()' method in order to define a default heap.  In this
// way, it should be easy to create a "ParseString" or "OptString" class
// whose underlying char* array is always allocated in the right place.
//
//////////////////////////////////////////////////////////////////////////

class SQLEXPORT_LIB_FUNC NAString : public NABasicObject 
{
public:

  // Often when an NAString is created, no heap pointer is specified;
  // in this case, we need to use a default value.
  virtual NAMemory * defaultHeapPtr () const
  { return NABasicObject::systemHeapPtr() ; } // fyi, this value is 0x0

  // used to identify NAMemory *'s that weren't specified by the NAString ctor call
  enum { nastring_uninit_heap_ptr = 0x0002 } ;
#define NASTRING_UNINIT_HEAP_PTR ((NAMemory *)NAString::nastring_uninit_heap_ptr)
#define NAWSTRING_UNINIT_HEAP_PTR NASTRING_UNINIT_HEAP_PTR

  enum stripType    {leading = 0x1, trailing = 0x2, both = 0x3};
  enum caseCompare  {exact, ignoreCase};
  enum scopeType    {one, all};

  typedef folly::basic_fbstring<char> FBString;  

  NAString(NAMemory *h=NASTRING_UNINIT_HEAP_PTR)                           ;
  NAString(NASize_T ic, NAMemory *h=NASTRING_UNINIT_HEAP_PTR)              ;
  NAString(const NAString & s, NAMemory *h=NASTRING_UNINIT_HEAP_PTR)       ;
  NAString(const NASubString& SS, NAMemory *h=NASTRING_UNINIT_HEAP_PTR)    ;
  NAString(const char * a, NAMemory *h=NASTRING_UNINIT_HEAP_PTR)           ;
  NAString(const char * a, size_t N, NAMemory *h=NASTRING_UNINIT_HEAP_PTR) ;

  // these three ctors are all the same
  NAString(char c, NAMemory *h=NASTRING_UNINIT_HEAP_PTR)           ;
  NAString(unsigned char c, NAMemory *h=NASTRING_UNINIT_HEAP_PTR)  ;
  NAString(signed char c, NAMemory *h=NASTRING_UNINIT_HEAP_PTR)    ;
  NAString(const FBString & fbs, NAMemory *h=NASTRING_UNINIT_HEAP_PTR) ;

#ifndef __EID
// not supposed to be in executor-in-DP2
  NAString(const ComGuardianFileNamePart & e, NAMemory *h=NASTRING_UNINIT_HEAP_PTR)
    :fbstring_(e.castToConstChar(), e.length(), 
                      (h == NASTRING_UNINIT_HEAP_PTR) ? this->defaultHeapPtr() : h)
    {}
#endif

  NAString(char c, size_t N, NAMemory *h=NASTRING_UNINIT_HEAP_PTR)         ;
  virtual ~NAString() ;

  // Type conversion:
  operator const char*() const {return fbstring_.data();}
#ifndef __EID
// not supposed to be in executor-in-DP2
  operator const ComNodeName() const {return ComNodeName(fbstring_.data());}
  operator const ComVolumeName() const {return ComVolumeName(fbstring_.data());}
#endif

  // Assignment:
  NAString&    operator=(const char*);         // Replace string
  NAString&    operator=(const NAString&);    // Replace string
 
#ifndef __EID
// not supposed to be in executor-in-DP2
  NAString&    operator=(const ComGuardianFileNamePart & e)
    {return operator= (e.castToConstChar());};
#endif
  NAString&    operator=(const char c)
    {return operator= (NAString(c, heap()));};

  NAString&    operator+=(const char*);        // Append string.

  NAString&    operator+=(const NAString& s);

  NAString& operator+=(const char c);

#ifndef __EID
// not supposed to be in executor-in-DP2
  NAString& operator+=(const ComGuardianFileNamePart & e)
    {return operator+= (e.castToConstChar());};
#endif

  
  // Indexing operators:
  char&         operator[](size_t);             // Indexing with bounds checking

  char&         operator()(size_t);             // Indexing with optional bounds checking
  NASubString   operator()(size_t start, size_t len);           // Sub-string operator

/*
# if !defined(RW_NO_STL)
  NASubString  match(const RWCRExpr& re);                                // Match the RE
  NASubString  match(const RWCRExpr& re, size_t start);              // Match the RE
# endif
*/

  NASubString   subString(const char* pat, size_t start=0, caseCompare=exact);
#ifndef RW_NO_CONST_OVERLOAD

  char          operator[](size_t) const;

  char          operator()(size_t) const;
  NASubString   operator()(size_t start, size_t len) const;
/*
# if !defined(RW_NO_STL)
  NASubString    match(const RWCRExpr& pat) const; // Match the RE
  NASubString    match(const RWCRExpr& pat, size_t start) const; // Match the RE
# endif
*/
  NASubString   subString(const char* pat, size_t start=0, caseCompare=exact) const;
  NASubString   strip(stripType s=trailing, char c=' ') const;
#endif

                // Non-static member functions:

  NAString&     append(const char* cs);

  NAString&     append(const char* cs, size_t N);

  NAString&     append(const NAString& s);
 
  NAString&     append(const NAString& s, size_t N);
  
  NAString&     append(const char c, size_t rep=1);   // Append c rep times

#ifndef __EID
// not supposed to be in executor-in-DP2
  NAString&     append(const ComGuardianFileNamePart & e)
    {return append (e.castToConstChar());}
#endif

  //RWspace       binaryStoreSize() const         {return length()+sizeof(size_t);}
  // total memory allocated, including the NAStringRef object in front of
  // the string data and the null!
  size_t  getAllocatedSize() const    
  {
    return fbstring_.get_alloc_size();       
  }

  size_t        capacity() const                {return fbstring_.capacity();}
  size_t        capacity(size_t N);
  void          clear();
#ifndef RW_NO_LOCALE
  Int32           collate(const char* cs) const   {return ::strcoll(data(), cs);}

  Int32           collate(const NAString& st) const;

#endif
  Int32           compareTo(const char* cs,      caseCompare cmp = exact) const;
  Int32           compareTo(const NAString& st, caseCompare cmp = exact) const;

  NABoolean     contains(const char* pat,      caseCompare cmp = exact) const;

  NABoolean     contains(const NAString& pat, caseCompare cmp = exact) const;

  NAString      copy() const;
  //Extract part of this string and append it to target
  int extract(int begin, int end, NAString & target) const;
  inline const char*   toCharStar() const {return fbstring_.data();} ;
  const char*   data() const {return fbstring_.data();}
  size_t        first(char c) const { return fbstring_.find_first_of(c);}
  size_t        first(char c, size_t i) const { return fbstring_.find_first_of(c, i);}
  size_t        first(const char* cs) const { return fbstring_.find_first_of(cs); }
  size_t        first(const char* cs, size_t N) const{return fbstring_.find_first_of(cs, N);}

  //splite this string into words by delim, memory for words are allocated in heap of elems,
  NAList<NAString> & split(char delim, NAList<NAString> & elems);
  
  UInt32      hash(caseCompare cmp) const;
  UInt32      hash() const;
  UInt32      hashFoldCase() const;
  void        mash(UInt32& hash, UInt32 chars) const;

  // index methods return:
  //   NA_NPOS if not found.
  //   0 based index, if found.
  size_t        index(const char* pat, size_t i=0, caseCompare cmp = exact)
                      const;
  
  size_t        index(const NAString& s, size_t i=0, caseCompare cmp = exact)
                      const;

  size_t        index(const char* pat, size_t patlen, size_t i,
                      caseCompare cmp) const;

  size_t        index(const NAString& s, size_t patlen, size_t i,
                      caseCompare cmp) const;

  NAString&     insert(size_t pos, const char*);

  NAString&     insert(size_t pos, const char*, size_t extent);

  NAString&     insert(size_t pos, const NAString&);

  NAString&     insert(size_t pos, const NAString&, size_t extent);
  NABoolean     isAscii() const;

  NABoolean     isNull() const                  {return 0 == fbstring_.size();}

  size_t        last(char c) const              {return fbstring_.find_last_of(c);}

  size_t        last(char c,size_t i) const     {return fbstring_.find_last_of(c, i);}

  size_t        length() const                  {return fbstring_.size();}
#ifndef RW_NO_LOCALE
  size_t        mbLength() const;       // multibyte length, or NA_NPOS on error
#endif

 NAString&     prepend(const char*);                   // Prepend a character string

  NAString&     prepend(const char* cs, size_t N);

  NAString&     prepend(const NAString& s);

  NAString&     prepend(const NAString& s, size_t N);
  NAString&     prepend(char c, size_t rep=1);  // Prepend c rep times
  Int32      readFile(ifstream&);                     // Read to EOF or null character.
  Int32      readLine(ifstream&);   // Read to EOF or newline.
  Int32      readToDelim(ifstream&, char delim='\n'); // Read to EOF or delimitor.

  NAString&     remove(size_t pos);                     // Remove pos to end of string

  void adjustMemory(size_t tot);  // adjust the capacity to tot 
                                  // if tot is smaller or equal
                                  // to the current capacity, 
                                 // do nothing.

  NAString&     remove(size_t pos, size_t n);           // Remove n chars starting at pos

  NAString&     replace(size_t pos, size_t n, const char*);
  NAString&     replace(size_t pos, size_t n, const char*, size_t);
 
  NAString&     replace(size_t pos, size_t n, const NAString&);

  NAString&     replace(size_t pos, size_t n, const NAString&, size_t);

  void          resize(size_t N);                       // Truncate or add blanks as necessary.

  NASubString   strip(stripType s=trailing, char c=' ');
  
  void          toLower();                              // Change self to lower-case
  void          toUpper();                              // Change self to upper-case
  void          toUpper8859_1();              // ISO 8859_1 alphabet upper-case
  
  NABoolean format(const char* formatTemplate...);
  
  // useful for supplying hash functions to template hash collection ctors:
  static UInt32       hash(const NAString&);

  NAMemory * heap() const { return fbstring_.heap(); }
protected:

  // Special concatenation constructor:
  NAString (const char *a1, size_t N1, const char * a2, size_t N2, NAMemory *h = NASTRING_UNINIT_HEAP_PTR) ;
  void                  assertElement(size_t) const;    // Index in range
  void                  clobber(size_t nc);             // Remove old contents

  void                  cow();                          // Do copy on write as needed
  
  void                  cow(size_t nc);                 // Do copy on write as needed
  
  void                  initChar(char, NAMemory *h);    // Initialize from char
public:
  static char* buildBuffer(const char* formatTemplate, va_list args);
  
private:

  void          clone();          // Make self a distinct copy

  void          clone(size_t nc); // Make self a distinct copy w. capacity nc

  FBString fbstring_;
friend
SQLEXPORT_LIB_FUNC 
NAString operator+(const NAString& s1, const NAString& s2);

friend
SQLEXPORT_LIB_FUNC
NAString operator+(const NAString& s,  const char* cs);

friend
SQLEXPORT_LIB_FUNC
NAString operator+(const NAString& s,  const char c);

friend
SQLEXPORT_LIB_FUNC
NAString operator+(const char* cs, const NAString& s);



friend
SQLEXPORT_LIB_FUNC
NABoolean operator==(const NAString& s1, const char* s2);


friend class NASubString;
  
};

typedef NAString* NAStringPtr;

SQLEXPORT_LIB_FUNC NAString toLower(const NAString&);   // Return lower-case version of argument.


SQLEXPORT_LIB_FUNC NAString toUpper(const NAString&);   // Return upper-case version of argument.


NA_EIDPROC
inline    UInt32 rwhash(const NAString& s) { return s.hash(); }
NA_EIDPROC
inline    UInt32 rwhash(const NAString* s) { return s->hash(); }
#ifndef RW_NO_LOCALE
NAString strXForm(const NAString&);  // strxfrm() interface
#endif


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                               Inlines                                //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

inline void NAString::cow()
{ clone(); }

inline void NAString::cow(size_t nc)
{ clone(nc); }

inline NAString& NAString::append(const char* cs)
{ fbstring_.append(cs); return *this; }

inline NAString& NAString::append(const char* cs, size_t N)
{ fbstring_.append(cs, N); return *this; }

inline NAString& NAString::append(const NAString& s)
{ fbstring_.append(s.fbstring_); return *this; }

inline NAString& NAString::append(const NAString& s, size_t N)
{ fbstring_.append(s.data(), N); return *this; }

inline NAString& NAString::operator+=(const char* cs)
{ fbstring_.append(cs, strlen(cs)); return *this; }

inline NAString& NAString::operator+=(const NAString& s)
{ fbstring_.append(s.data(), s.length()); return *this; }

inline NAString& NAString::operator+=(const char c)
{ fbstring_.append(1, c); return *this; }

#ifndef RW_NO_LOCALE
inline Int32 NAString::collate(const NAString& st) const
{ return ::strcoll(data(), st.data()); }
#endif

inline NABoolean NAString::contains(const NAString& pat, caseCompare cmp) const
{ return index(pat.data(), pat.length(), (size_t)0, cmp) != NA_NPOS; }

inline NABoolean NAString::contains(const char* s, caseCompare cmp) const
{ return index(s, strlen(s), (size_t)0, cmp) != NA_NPOS; }

inline size_t NAString::index(const char* s, size_t i, caseCompare cmp) const
{ return index(s, strlen(s), i, cmp); }

inline size_t NAString::index(const NAString& s, size_t i, caseCompare cmp) const
{ return index(s.data(), s.length(), i, cmp); }

inline size_t NAString::index(const NAString& pat, size_t patlen, size_t i, caseCompare cmp) const
{ return index(pat.data(), patlen, i, cmp); }

inline NAString& NAString::insert(size_t pos, const char* cs)
{ return replace(pos, 0, cs, strlen(cs)); }

inline NAString& NAString::insert(size_t pos, const char* cs, size_t N)
{ return replace(pos, 0, cs, N); }

inline NAString& NAString::insert(size_t pos, const NAString& cstr)
{ return replace(pos, 0, cstr.data(), cstr.length()); }

inline NAString& NAString::insert(size_t pos, const NAString& cstr, size_t N)
{ return replace(pos, 0, cstr.data(), MINOF(N, cstr.length())); }

inline NAString& NAString::prepend(const char* cs)
{ return replace(0, 0, cs, strlen(cs)); }

inline NAString& NAString::prepend(const char* cs, size_t N)
{ return replace(0, 0, cs, N); }

inline NAString& NAString::prepend(const NAString& s)
{ return replace(0, 0, s.data(), s.length()); }

inline NAString& NAString::prepend(const NAString& s, size_t N)
{ return replace(0, 0, s.data(), MINOF(N, s.length())); }

inline NAString& NAString::remove(size_t pos)
{ return replace(pos, length()-pos, nanil, 0); }

inline NAString& NAString::remove(size_t pos, size_t n)
{ return replace(pos, n, nanil, 0); }

inline NAString& NAString::replace(size_t pos, size_t n, const char* cs)
{ return replace(pos, n, cs, strlen(cs)); }

inline NAString& NAString::replace(size_t pos, size_t n, const NAString& cstr)
{ return replace(pos, n, cstr.data(), cstr.length()); }

inline NAString& NAString::replace(size_t pos, size_t n1, const NAString& cstr, size_t n2)
{ return replace(pos, n1, cstr.data(), MINOF(cstr.length(),n2)); }

inline char& NAString::operator()(size_t i)
{ 
#ifndef NDEBUG
  assertElement(i); 
#endif
  cow();
  return fbstring_[i];
}

inline char NAString::operator[](size_t i) const
{ assertElement(i); return fbstring_[i]; }

inline char NAString::operator()(size_t i) const
{
#ifndef NDEBUG
  assertElement(i); 
#endif
  return fbstring_[i];
}

#ifndef __EID
// not supposed to be in executor-in-DP2
inline NAString operator+(const NAString& s, const ComGuardianFileNamePart & e)
  {return operator+(s, e.castToConstChar());};

inline NAString operator+(const ComGuardianFileNamePart & e, const NAString& s )
  {return operator+(e.castToConstChar(), s);};

inline NAString operator+(const ComGuardianFileNamePart & e, const char * cs )
  {return operator+(NAString(e), cs);};

inline NAString operator+(const char * cs, const ComGuardianFileNamePart & e )
  {return operator+(cs, NAString(e));};
#endif

///////////////////////////////////////////////////////////////////////////////
//
// NASubString::startData()
//
// This member replaces data().
// startData() will remain undocumented.  Please don't even ask.
// Use at your own risk. It may be deprecated in the future.
//
// Since NASubString works by referencing the NAString's data,
// if you attempt to directly use the data() member of the NAString,
// you will very likely be surprised by the result, which will be null
// terminated not at the extent of the substring,
// but at the end of the NAString.
//
///////////////////////////////////////////////////////////////////////////////

inline const char* NASubString::startData() const
{ return str_->data() + begin_; }

// DON'T USE THE FUNCTION BELOW!
// This member is deprecated and will be removed in a future version.
// It remains public only to maintain source compatibility.
inline const char* NASubString::data() const
{ return str_->data() + begin_; }

//------------------------------------------------------------------------------

// Access to elements of sub-string with bounds checking
inline char NASubString::operator[](size_t i) const
{ assertElement(i); return (*str_)[begin_+i]; }

inline char NASubString::operator()(size_t i) const
{
#ifndef NDEBUG
   assertElement(i);
#endif
   return (*str_)[begin_+i];
}

// String Logical operators:
NA_EIDPROC
inline NABoolean        operator==(const NAString& s1, const NAString& s2)
                                  { return ((s1.length() == s2.length()) &&
                                    !memcmp(s1.data(), s2.data(), s1.length())); }

NA_EIDPROC
inline NABoolean        operator< (const NAString& s1, const NAString& s2)
                                  { return s1.compareTo(s2)< 0;}

NA_EIDPROC
inline NABoolean        operator!=(const NAString& s1, const NAString& s2)
                                  { return !(s1 == s2); }

NA_EIDPROC
inline NABoolean        operator> (const NAString& s1, const NAString& s2)
                                  { return s1.compareTo(s2)> 0;}

NA_EIDPROC
inline NABoolean        operator<=(const NAString& s1, const NAString& s2)
                                  { return s1.compareTo(s2)<=0;}

NA_EIDPROC
inline NABoolean        operator>=(const NAString& s1, const NAString& s2)
                                  { return s1.compareTo(s2)>=0;}

NA_EIDPROC
inline NABoolean        operator!=(const NAString& s1, const char* s2)
                                  { return !(s1 == s2); }

NA_EIDPROC
inline NABoolean        operator< (const NAString& s1, const char* s2)
                                  { return s1.compareTo(s2)< 0; }

NA_EIDPROC
inline NABoolean        operator> (const NAString& s1, const char* s2)
                                  { return s1.compareTo(s2)> 0; }

NA_EIDPROC
inline NABoolean        operator<=(const NAString& s1, const char* s2)
                                  { return s1.compareTo(s2)<=0; }

NA_EIDPROC
inline NABoolean        operator>=(const NAString& s1, const char* s2)
                                  { return s1.compareTo(s2)>=0; }

NA_EIDPROC
inline NABoolean        operator==(const char* s1, const NAString& s2)
                                  { return (s2 == s1); }

NA_EIDPROC
inline NABoolean        operator!=(const char* s1, const NAString& s2)
                                  { return !(s2 == s1); }

NA_EIDPROC
inline NABoolean        operator< (const char* s1, const NAString& s2)
                                  { return s2.compareTo(s1)> 0; }

NA_EIDPROC
inline NABoolean        operator> (const char* s1, const NAString& s2)
                                  { return s2.compareTo(s1)< 0; }

NA_EIDPROC
inline NABoolean        operator<=(const char* s1, const NAString& s2)
                                  { return s2.compareTo(s1)>=0; }

NA_EIDPROC
inline NABoolean        operator>=(const char* s1, const NAString& s2)
                                  { return s2.compareTo(s1)<=0; }

// SubString Logical operators:
NA_EIDPROC
inline NABoolean operator==(const NAString& s1,    const NASubString& s2)
                           { return (s2 == s1); }

NA_EIDPROC
inline NABoolean operator==(const char* s1,        const NASubString& s2)
                           { return (s2 == s1); }

NA_EIDPROC
inline NABoolean operator!=(const NASubString& s1, const char* s2)
                           { return !(s1 == s2); }

NA_EIDPROC
inline NABoolean operator!=(const NASubString& s1, const NAString& s2)
                           { return !(s1 == s2); }

NA_EIDPROC
inline NABoolean operator!=(const NASubString& s1, const NASubString& s2)
                           { return !(s1 == s2); }

NA_EIDPROC
inline NABoolean operator!=(const NAString& s1,    const NASubString& s2)
                           { return !(s2 == s1); }

NA_EIDPROC
inline NABoolean operator!=(const char* s1,        const NASubString& s2)
                           { return !(s2 == s1); }

#ifndef __EID
// not supposed to be in executor-in-DP2
inline NABoolean operator==(const NAString& s1, const ComGuardianFileNamePart& s2)
                           {return (s1 == s2.castToConstChar());}
inline NABoolean operator==(const ComGuardianFileNamePart& s1, const NAString& s2)
                           {return (s2 == s1.castToConstChar());}
inline NABoolean operator==(const NAString& s1, const ComNodeName& s2)
                           {return (s1 == s2.castToConstChar());}
inline NABoolean operator==(const ComNodeName& s1, const NAString& s2)
                           {return (s2 == s1.castToConstChar());}
inline NABoolean operator==(const NAString& s1, const char c)
                           {return (s1.length() == 1 && *s1.data() == c);}
#endif

// $$$NB: besides usage of c-string-lib functions inside the NAString code,
// if we #ifdef the following two functions then we ought to be able to 
// use NAString objects inside the executor ...

// input/output
SQLEXPORT_LIB_FUNC istream&  operator>>(istream& str   , NAString& cstr);

SQLEXPORT_LIB_FUNC ostream&  operator<<(ostream& str   , const NAString& cstr);

#endif /* __NASTRING_DEF_H__ */

