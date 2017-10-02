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
#ifndef __NASTRING_DEF_GLOBALS_H__
#define __NASTRING_DEF_GLOBALS_H__
/* -*-C++-*-
******************************************************************************
*
* File:         NAStringDefGlobals.h
* Description:  This file contains some globally usable NAString definitions;
*               in particular, things that NAMemory needs to know.
*
* Created:      6/10/1998
* Language:     C++
*
*
*
*
****************************************************************************** */

// -----------------------------------------------------------------------
//
// This header file contains a few of the declarations for class
// NAString.
//
// It is a separate file in order to facilitate the use of NAString
// in multiple system components.
//
// DO NOT ADD DEPENDENCIES TO THIS FILE!!!!! 
// DO NOT #include ANY FILES EXCEPT THE DLLEXPORTDEFINES ONE!!!!!
//
// -----------------------------------------------------------------------

#include "Platform.h"
// #include "ComCharSetDefs.h" // includes definition of enum SQLCHARSET_CODE

// classes in this file:
class NAReference ; // rewrite of RWReference,  from ref.h/ref.cpp
class NAStringRef ; // rewrite of RWCStringRef, from cstring.h/cstring.cpp

// forward declarations 
class RWMutex ;
class NAMemory ;
class NASubString ;
class NAString ;
class NAWString ;

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                             NAReference                              //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

class NAReference 
{
protected:
  UInt32        refs_;          // (1 less than) number of references 
public:
  enum NAReferenceFlag { STATIC_INIT };
  
  NAReference(Int32 initRef = 0) : refs_((UInt32)initRef-1) { }
  NAReference(NAReferenceFlag) { }  // leave refs_ alone
  UInt32        references() const      {return refs_+1;}
  void            setRefCount(UInt32 r) {refs_ = r-1;}

  // these two methods can be found at the end of file NAStringDef.cpp
  void     addReference();
  UInt32 removeReference();
};




//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                             NAStringRef                              //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

/*
 * This is the dynamically allocated part of a NAString.
 * It maintains a reference count.
 * There are no public member functions.
 */

class NAStringRef : public NAReference 
{
private: // not protected -- we don't expect anyone to ever inherit from NAStringRef

  // -----------------------------------------------------------------------------
  // This is the only NAStringRef ctor -- it is used to initialize the
  // ref-counted "null string" reps on various heaps (this change is
  // necessary to minimize memory leaks & memory usage of the NAString
  // class).
  //
  // This constructor, and operator new() below, should only be used by NAMemory. 
  enum ctor_type { INVALID_CTOR_USE, NULL_CTOR }; 

  inline NAStringRef (ctor_type ctor, NAMemory * heap)  
  {  
    // need to set all data members as appropriate for a NULL string on this
    // heap (i.e, all values, except the refCount, should be set to zero)
    heap_     = heap ;

    capacity_ = 0 ;
    nchars_   = 0 ;
    // charset_ = SQLCHARSETCODE_UNKNOWN;
    data()[0] = '\0' ;
    data()[1] = '\0' ; // UR2. to handle NULL NAWString.
    this->setRefCount (1) ;
  }

  // In order to use the above ctor with NAMemory the way we want, we need
  // to make sure that we don't use the globally-defined new.  So we write
  // for NAStringRef.
  static inline void* operator new (size_t, void* loc) 
  {
    return loc ;  // yes, it's really this simple!
  }

  // -----------------------------------------------------------------------------


  static NAStringRef* getRep(const NAString * string, size_t capac, size_t nchar, NAMemory *h);

  // capac and nchar are in number of wide chars (UCS2)
  static NAStringRef* getWCSRep(const NAWString * wstring, size_t capac, size_t nchar, NAMemory *h);

  // depending on the heap ptr, returns the corresponding null-string-ref ptr
  static NAStringRef* nullNAStringRef (NAMemory * h) ;

  void  unLink(); // disconnect from a stringref, maybe delete it

  size_t        length   () const {return nchars_;}
  size_t        capacity () const {return capacity_;}
  char* data             () const {return (char*)(this+1);}
  // SQLCHARSET_CODE charset () const {return charset_;}

  char& operator[](size_t i)       {return ((char*)(this+1))[i];}
  char  operator[](size_t i) const {return ((char*)(this+1))[i];}

  size_t        first    (char       ) const;
  size_t        first    (char,size_t) const;
  size_t        first    (const char*) const;
  size_t        first    (const char*, size_t) const;
  UInt32      hash     (           ) const;
  UInt32      hashFoldCase (       ) const;
  size_t        last     (char       ) const;
  size_t        last     (char,size_t) const;

#ifndef RW_NO_LOCALE
  Int32           collate(const char*) const;
#endif

  NAMemory *    heap_ ;

  size_t        capacity_;      // Max string length (excluding null)
  size_t        nchars_;        // String length (excluding terminating null)
  // SQLCHARSET_CODE  charset_; // see enum SQLCHARSET_CODE definition in sqlcli.h

friend class NAString;
friend class NASubString;
friend class NAMemory;
friend class NAWString;

private:
  NAStringRef () ;
  NAStringRef (const NAStringRef &) ;
};


#endif /* __NASTRING_DEF_GLOBALS_H__ */
