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
#ifndef COMRESWORDS_H
#define COMRESWORDS_H

/* -*-C++-*-
*******************************************************************************
*
* File:         ComResWords.h
* Description:  All reserved words including those defined in ANSI 5.2,
*               Used by NAString (isSqlReservedWord)
* Created:      2000-07-21
*******************************************************************************
*/
#include "Platform.h"
#include "NABasicObject.h"
#include "NABoolean.h" 
#include "NAWinNT.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------

struct ComResWord;
class ComResWords;

enum { 
  FLAGSNONE_   = 0x00,          // No FLAGS

  RESWORD_     = 0x01,          // a reserved word (ANSI, Pot. ANSI or COMPAQ)

  NONRESWORD_  = 0x02,          // a non-reserved word.  These are words that
                                // should be reserved (ANSI or Pot. ANSI) but
                                // are not for various reason. See ABSOLUTE,
                                // DATA, and OPERATION in ComResWords.cpp.

  ANS_         = 0x08,          // Indicates that the word is reserved by ANSI
                                // This flag is used only for notation.

  POTANS_      = 0x10,          // Indicates that the word is potentially 
                                // reserved by ANSI. This flag is used only
                                // for notation.

  COMPAQ_      = 0x20,          // Indicates that the word is reserved by
                                // COMPAQ. This flag is used only for notation.

  ALLOWOLDNEW_ = 0x40,           // Indicates that the word is allowed if the
                                // ALLOWOLDNEW parser flag is set.  In certain
                                // contexts the words OLD and NEW are allowed
                                // as identifiers.  This flag identifies the
                                // extra words which are allowed in this
                                // context.

  // see cqd mode_special_4
  MODE_SPECIAL_4_ = 0x80
};

// ***********************************************************************
//
//  ComResWord : A description of a reserved word.
//
// ***********************************************************************

struct ComResWord
{
public:


  //void setResWord(char * resWord) { (char*)resWord_ = resWord; };

  // Accessor for the resWord_ data member.
  //
  inline const char *getResWord() const { return resWord_; };

  // Is the word reserved
  //
  inline NABoolean isReserved(UInt32 ifSetFlags) {
      return ((flags_ & RESWORD_) &&
	      !(allowOldAndNew(ifSetFlags) ));
   
  };

  // Are the reserved words OLD and NEW allowed as non-reserved
  // identifiers.
  //
  inline  NABoolean allowOldAndNew(UInt32 ifSetFlags) {
    return  (ifSetFlags & (flags_ & ALLOWOLDNEW_));
  };

  // The reserved word.
  //
  const char * resWord_;

  // Attributes of the reserved word.
  //
  UInt32 flags_;
};


// ***********************************************************************
//
//  ComResWords : A table of reserved words (ComResWord).
//
//  This class has no non-static data associated with it.  It simply
//  provides an interface to static data of the reserved word table.
//   
//    NOTE: Need to remove references to static data members for the 
//    Executor project that deals with removing globals since this class
//    if now used by the executor
//
// ***********************************************************************
class ComResWords : public NABasicObject
{
public:
  
  // Constructors.
  //
  ComResWords(const ComResWords &other, NAMemory * h=0); 
  ComResWords(NAMemory * h=0);

  // Determine if the given word is reserved. 
  //
  static NABoolean isSqlReservedWord(const char *word,
				     UInt32 ifSetFlags = 0);

private:

  // Encapulates the searching of the reserved word table.
  //
  static inline ComResWord *searchResWordTbl(ComResWord *val) {
    return binarySearch(val);
  }


  // Comparision method for searching reserved word table.
  //
  static Int32 wordCompare(const void *val1, const void *val2); 

  static ComResWord * binarySearch(ComResWord *val);
  
  // Static data associated with all instances of this class.
  //

  // The array of reserved words.
  //
  static const ComResWord resWords_[];

};

#endif /* COMRESWORDS_H */
