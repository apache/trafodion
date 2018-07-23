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
#ifndef PARKEYWORDS_H
#define PARKEYWORDS_H
/* -*-C++-*-
*******************************************************************************
*
* File:         ParKeyWords.h
* Description:  All keywords including those defined in ANSI 5.2,
*               Used by ulexer
*******************************************************************************
*/

#include "NABasicObject.h"
#define   SQLPARSERGLOBALS_FLAGS
#include "SqlParserGlobals.h"	// Parser Flags
#include "NAWinNT.h"		// for NAWchar, WIDE_(), etc.
#include "NABoolean.h" 

#ifndef SQLPARSER_H
#include <sqlparser.h>   // Angled brackets are intentional here
#define SQLPARSER_H
#endif

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ParKeyWord;
class ParKeyWords;

enum { 
  FLAGSNONE_   = 0x000,         // No FLAGS

  FIRST_       = 0x001,         // can appear as first word of compound token
                                // eg. BROWSE in BROWSE ACCESS -> 
                                // TOK_BROWSE_ACCESS

  SECOND_      = 0x002,         // can appear as second word of compound token
                                // eg. ACCESS in BROWSE ACCESS -> 
                                // TOK_BROWSE_ACCESS

  THIRD_       = 0x004,         // can appear as third word of compound token
                                // eg. ONLY in FOR READ ONLY ->
                                // TOK_FOR_READ_ONLY

  RESWORD_     = 0x008,         // a reserved word (ANSI, Pot. ANSI or COMPAQ)

  NONRESWORD_  = 0x010,         // a non-reserved word.  These are words that
                                // should be reserved (ANSI or Pot. ANSI) but
                                // are not for various reason. See ABSOLUTE,
                                // DATA, and OPERATION in ComResWords.cpp.

  NONRESTOKEN_ = 0x020,         // a non-reserved token.  These are words that
                                // can be used as a token or an identifier 
                                // depending on the context.  This flag is 
                                // used only for notation.  The Parser also 
                                // maintains this list.

  ALLOWOLDNEW_ = 0x080,         // Indicates that the word is allowed if the
                                // ALLOWOLDNEW parser flag is set.  In certain
                                // contexts the words OLD and NEW are allowed
                                // as identifiers.  This flag identifies the
                                // extra words which are allowed in this
                                // context.

  ANS_         = 0x100,         // Indicates that the word is reserved by ANSI
                                // This flag is used only for notation.

  POTANS_      = 0x200,         // Indicates that the word is potentially 
                                // reserved by ANSI. This flag is used only
                                // for notation.

  COMPAQ_      = 0x400,         // Indicates that the word is reserved by
                                // COMPAQ. This flag is used only for notation.

  CONDITIONAL_RES_ = 0x800      // Indicates that the word is reserved 
                                // conditionally.                                 
};

// ***********************************************************************
//
//  ParKeyWord : A description of a key word.
//
// ***********************************************************************
class ParKeyWord : public NABasicObject
{
public:

  // Constructors.
  //
  ParKeyWord(const ParKeyWord &other, NAMemory * h=0); 
  ParKeyWord(NAMemory * h=0) {};
  ParKeyWord(const char *kwd, Int32 code, UInt32 flags, NAMemory * h=0);
  
  // Destructor
  //
  virtual ~ParKeyWord() {};

  // Is this key word allowed in the first, second or third position
  // of a compound token.
  //
  inline NABoolean canBeFirst () const { return flags_ & FIRST_; };
  inline NABoolean canBeSecond() const { return flags_ & SECOND_; };
  inline NABoolean canBeThird () const { return flags_ & THIRD_; };

  // Accessor methods.
  //
  inline const char *getKeyWord() const { return keyword_; };
  inline Int32 getTokenCode() const { return tokenCode_; };

  // Is the word an identifier.
  //
  inline NABoolean isIdentifier() const {
    return (tokenCode_ == IDENTIFIER);
  };

  // Is the word reserved.
  //
  inline NABoolean isReserved() const { 
      return ((flags_ & RESWORD_) &&
	      !(allowOldAndNew() && (flags_ & ALLOWOLDNEW_)));
  };

  inline NABoolean isConditionallyReserved() const {
    return ( flags_ & (CONDITIONAL_RES_ | RESWORD_ ) );
  };

  NABoolean useAsIdentifierInNonSpecialMode() const;

private:
  
  // Are the keywords OLD and NEW allowed as identifiers.
  //
  inline static NABoolean allowOldAndNew(void) {
    return Get_SqlParser_Flags(ALLOW_OLD_AND_NEW_KEYWORD) ? TRUE : FALSE;
  }
  

  // The keyword.
  //
  const char *keyword_;

  // The token representing the keyword.
  //
  Int32 tokenCode_;

  // Attributes of the key word.
  //
  UInt32 flags_;
};

// ***********************************************************************
//
//  ParKeyWords : A table of keywords (ParKeyWord).
//
//  This class has no non-static data associated with it.  It simple
//  provides an interface to static data of the keyword table.
//
// ***********************************************************************
class ParKeyWords : public NABasicObject
{
public:
  
  // Constructor.
  //
  ParKeyWords(const ParKeyWords &other, NAMemory * h=0); 
  ParKeyWords(NAMemory * h=0);

  // Initialize the key word table.  This will load in any changes to
  // the table and sort it if these things have not already been done.
  //
  static void initKeyWordTable(void);

  // Find the given keyword in the key word table.  If it is not
  // found, return a pointer to the identifier keyword (see identWord_
  // below).
  //
  static const ParKeyWord *lookupKeyWord(NAWchar *id);

  // Check if key word table had been sorted
  //
  static const NABoolean keyWordTableSorted(void) {
    return keyWordTableSorted_;
  }

private:

  // Comparision method for searching keyword table.
  //
  static Int32 keyCompare(const void *keyval, const void *datum); 
  
  // Encapulates the searching of the keyword table.
  //
  static inline ParKeyWord *searchKeyWordTbl(ParKeyWord *key) {
    return (ParKeyWord *)bsearch(key,
				 keyWords_,
				 numEntries_,
				 sizeof(ParKeyWord),
				 keyCompare);
  }

  // Static data associated with all instances of this class.
  //

  // The array of keywords.
  //
  static ParKeyWord keyWords_[];

  // The default IDENTIFIER keyword.  Used by lookupKeyWord() if the
  // given word is not found in the keyword table.
  //
  static ParKeyWord identWord_;

  // The number of entries in the keyword table.
  //
  static size_t numEntries_;

  // The max length of all the keywords.
  //
  static size_t maxKeyWordLength_;

  // Indicates if the keyword table has been sorted.
  //
  static NABoolean keyWordTableSorted_;
};

#endif // PARKEYWORDS_H
