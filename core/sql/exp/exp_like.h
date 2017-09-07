#ifndef EXP_LIKE_H
#define EXP_LIKE_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         <file>
 * Description:
 *
 * Created:      10/17/95
 * Language:     C++
 *
 *
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
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#include "ExpError.h"

#include "BaseTypes.h"
#include "NAHeap.h"
#include "csconvert.h"
#include "NLSConversion.h"

class  LikePatternString : public NABasicObject
{
public:
  //
  // Constructor.
  //
  LikePatternString
  ( const char* pattern
  , UInt16 patternLen
  , CharInfo::CharSet patternCS = CharInfo::ISO88591
  , const char* escapeChar = NULL
  , UInt16      escapeChar_len = 0
  , const char* underscoreChar = "_"
  , UInt16      underscoreChar_len = 1
  , const char* percentChar = "%"
  , UInt16      percentChar_len = 1
  )
  : pattern_(pattern)
  , patternLen_(patternLen)
  , endOfPattern_(&pattern[patternLen])
  , patternCS_(patternCS)
  , bytesPerChar_(CharInfo::maxBytesPerChar(patternCS))
  , underscoreChar_(underscoreChar)
  , percentChar_(percentChar)
  , escapeChar_(escapeChar)
  , escapeChar_len_(escapeChar_len)
  , percentChar_len_(percentChar_len)
  , underscoreChar_len_(underscoreChar_len)
  {}

  ~LikePatternString() {}
  //
  // Accessor functions.
  //
  CharInfo::CharSet getPatternCharSet() const { return patternCS_ ; }

  UInt16 getLength() const { return patternLen_; }
private:
  const char* pattern_;
  const UInt16 patternLen_;
  CharInfo::CharSet patternCS_ ;
  const char* endOfPattern_;
  const UInt16 bytesPerChar_;
  const UInt16 escapeChar_len_;
  const UInt16 underscoreChar_len_;
  const UInt16 percentChar_len_; 
  const char* underscoreChar_;
  const char* percentChar_;
  const char* escapeChar_;
  friend class LikePatternStringIterator;

};

class  LikePatternStringIterator
{
public:
  //
  // Construct an iterator that points to the first character in the given
  // pattern.
  //
  LikePatternStringIterator
  ( const LikePatternString& pattern
  )
  : pattern_(pattern)
  , currentChar_(pattern.pattern_)
  {
    determineCharType();
  }
  ~LikePatternStringIterator() {}
  //
  // Determine the current character's classification.
  //
  enum CharType
  { END_OF_PATTERN
  , NON_WILDCARD
  , UNDERSCORE
  , PERCENT
  , CharType_ERROR
  };
  void determineCharType()
  {
    if (currentChar_ >= pattern_.endOfPattern_)
      charType_ = END_OF_PATTERN;
    else if ( (pattern_.escapeChar_ != NULL) AND
              thisCharIsEqualTo(pattern_.escapeChar_, pattern_.escapeChar_len_) ) {
      currentChar_ += pattern_.escapeChar_len_;
      if (currentChar_ >= pattern_.endOfPattern_) {
        //
        // A pattern cannot end with an escape character.  Raise a data
        // exception--invalid escape sequence.
        //
        charType_ = CharType_ERROR;
      } else if ( thisCharIsEqualTo(pattern_.escapeChar_, pattern_.escapeChar_len_) OR
                  thisCharIsEqualTo(pattern_.underscoreChar_, pattern_.underscoreChar_len_) OR
                  thisCharIsEqualTo(pattern_.percentChar_, pattern_.percentChar_len_)
                )
        charType_ = NON_WILDCARD; // Declare normal wildcard char as NON wildcard!
      else {
        //
        // The escape character was not followed by another escape character,
        // underscore, or percent character.  Raise a data exception--invalid
        // escape sequence.
        //
        charType_ = CharType_ERROR;
      }
    } else if ( thisCharIsEqualTo(pattern_.underscoreChar_, pattern_.underscoreChar_len_) )
      charType_ = UNDERSCORE;
    else if ( thisCharIsEqualTo(pattern_.percentChar_, pattern_.percentChar_len_) )
      charType_ = PERCENT;
    else
      charType_ = NON_WILDCARD;
  }
  //
  // Accessor functions.
  //
  operator CharType() const 		{ return charType_; }

  const char* getCurrentChar()
  {
    return currentChar_;
  }

  Int16 getChar(char* ch) const
  {
    Int16 chrLen = pattern_.bytesPerChar_;
    switch (pattern_.patternCS_) {
      case CharInfo::ISO88591   :
      case CharInfo::UCS2       :
      case CharInfo::KANJI_MP   :
      case CharInfo::KSC5601_MP :
        break;
      default :  // For all multi-byte character sets
        cnv_charset cnv_cs = convertCharsetEnum (pattern_.patternCS_);
        UInt32 UCS4val = 0;
        chrLen = LocaleCharToUCS4( currentChar_, 
                                   pattern_.bytesPerChar_,
                                   &UCS4val, cnv_cs);
        if (chrLen <= 0) chrLen = 1; //Prevent going wild.
        break;
    }
    for (UInt16 i = 0; i < chrLen ; i++)
      ch[i] = currentChar_[i];
    return( chrLen );
  }

  UInt16 getBytesPerChar() const { return pattern_.bytesPerChar_; }

  //
  // Return TRUE if the current character is equal to the given character.
  //
  NABoolean thisCharIsEqualTo(const char* ch, const UInt16 ch_len) const
  {
    if ( currentChar_ + ch_len > pattern_.endOfPattern_ )
      return FALSE;
    for (UInt16 i = 0; i < ch_len ; i++)
      if (currentChar_[i] != ch[i])
        return FALSE;
    return TRUE;
  }
  //
  // Advance to the next character in the pattern.
  //

  void operator += (Int32 numBytesToAdd)
  {
    currentChar_ += numBytesToAdd;
  }

private:

  const LikePatternString& pattern_;
  
  const char* currentChar_;
  CharType charType_;

};

/******************************************************************************

LikePattern

A LikePatternString must be converted to a LikePattern object before the
pattern can be compared with a text string for a possible match.  A LikePattern
is a chain of LikePatternHeader objects.  A LikePatternHeader represents a part
of the pattern before the next percent character.  It matches text of a
specific length.  A LikePatternHeader is a chain of LikePatternClause objects.
A LikePatternClause represents a homogenous part of the pattern that is either
a string of non-wildcard characters or a string of underscores.  Notice that,
in the inheritance hierarchy we have:
  LikePatternClause
    LikePatternHeader
      LikePattern

Note that the clause and header EACH has its own length_ and getLength().
You can cast a header to its baseclass (LPClause) or use the header's
getClauseLength() method.

LikePatternString "" is represented as:

  LikePatternHeader  LikePatternClause
  +------------+     +---------------------+
  | Length = 0 |---->| Type = NON_WILDCARD |
  +------------+     | Pattern = ""        |
		     | Length = 0          |
		     +---------------------+

LikePatternString "%" is represented as:

  LikePatternHeader  LikePatternClause
  +------------+     +---------------------+
  | Length = 0 |---->| Type = PERCENT      |
  +------------+     | Pattern = ""        |
	|            | Length = 0          |
	|            +---------------------+
	|
	V            LikePatternClause
  +------------+     +---------------------+
  | Length = 0 |---->| Type = PERCENT      |
  +------------+     | Pattern = ""        |
		     | Length = 0          |
		     +---------------------+

LikePatternString "%%ABC__%%%DEF%" is represented as:

  LikePatternHeader  LikePatternClause
  +------------+     +---------------------+
  | Length = 0 |---->| Type = PERCENT      |
  +------------+     | Pattern = ""        |
	|            | Length = 0          |
	|            +---------------------+
	|
	V            LikePatternClause           LikePatternClause
  +------------+     +---------------------+     +---------------------+
  | Length = 5 |---->| Type = NON_WILDCARD |---->| Type = UNDERSCORE   |
  +------------+     | Pattern = "ABC"     |     | Pattern = "__"      |
	|            | Length = 3          |     | Length = 2          |
	|            +---------------------+     +---------------------+
	|
	V            LikePatternClause
  +------------+     +---------------------+
  | Length = 3 |---->| Type = NON_WILDCARD |
  +------------+     | Pattern = "DEF"     |
	|            | Length = 3          |
	|            +---------------------+
	|
	V            LikePatternClause
  +------------+     +---------------------+
  | Length = 0 |---->| Type = PERCENT      |
  +------------+     | Pattern = ""        |
		     | Length = 0          |
		     +---------------------+

******************************************************************************/

class  LikePatternClause : public NABasicObject
{
public:
  //
  // Constructor.
  //
  LikePatternClause
  ( LikePatternStringIterator::CharType charType
  , char* patternBuf
  )
  : charType_(charType)
  , pattern_(patternBuf)
  , length_(0)
  , previousClause_(NULL)
  , nextClause_(NULL)
  , co_(CharInfo::DefaultCollation)
  , encodedPattern_(NULL)
  {}

  ~LikePatternClause() {}
  //
  // Accessor functions.
  //
  LikePatternStringIterator::CharType getType() const { return charType_; }
  char* getPattern() const 			{ return pattern_; }
  UInt16 getLength() const 		{ return length_; }
  void setLength(UInt16 length) 
    { length_ = length; }

  LikePatternClause* getNextClause() const 	{ return nextClause_; }
  LikePatternClause* getPreviousClause() const 	{ return previousClause_; }
  //
  // Set the clause's character classification.  It indicates the type of
  // character that will be stored in the clause.
  //
  void setType(LikePatternStringIterator::CharType charType)
  {
    charType_ = charType;
  }
  //
  // Append the given character to the clause.
  //
  void append(const LikePatternStringIterator& i)
  {
    length_ += i.getChar(getPattern() + getLength());
  }
  //
  // Append the given clause to the current clause.
  //
  void append(LikePatternClause* clause)
    {
      nextClause_ = clause;
      nextClause_->previousClause_ = this;
    }
  //
  // Return TRUE if the clause matches the given text.
  //
  NABoolean matches(const char* text); 

  CharInfo::Collation getCollation() const
  { return co_; }

  void setCollation(CharInfo::Collation collation)
    { co_ = collation; }

  unsigned char* getEncodedPattern()
    { return encodedPattern_; }

  void setEncodedPattern(unsigned char* encodedPattern)
    { encodedPattern_ = encodedPattern; }

private:
  //
  // The character type can be either NON_WILDCARD, UNDERSCORE, or PERCENT.
  //
  LikePatternStringIterator::CharType charType_;
  char* pattern_;
  UInt16 length_;
  LikePatternClause* nextClause_;
  CharInfo::Collation co_;
  unsigned char *encodedPattern_;

  // LikePatternHeader::matchesR need check the pattern header from the end
  // to the beginning. previousClause_ will have the information about the
  // previous (left) character in the header.
  LikePatternClause* previousClause_;

};

class  LikePatternHeader : public LikePatternClause
{
public:
  //
  // Constructor.
  //
  LikePatternHeader
  ( LikePatternStringIterator::CharType charType
  , char* patternBuf
  , CollHeap* exHeap
  )
  : LikePatternClause(charType, patternBuf)
  , length_(0)
  , nextHeader_(NULL)
  , exHeap_(exHeap)
  , error_(EXE_OK)
  , encodedHeader_(NULL)
  {
    lastClause_ = this; // VC++ doesn't like "this" in init list above
  }

  //
  // Destructor.
  //
 ~LikePatternHeader()
  {
    // If we have allocated space for encodedHeader, deallocate it.
     if(this->getEncodedHeader())
       NADELETEBASIC(this->getEncodedHeader(), getExHeap());
    //
    // Delete all clauses attached to the header.
    //
    LikePatternClause* clause = getNextClause();
    while (clause != NULL) {
      lastClause_ = clause;
      clause = clause->getNextClause();
      delete lastClause_;
    }
  }
  //
  // Accessor functions.
  //
  LikePatternClause* getLastClause() const 	{ return lastClause_; }
  UInt16 getLength() const 		{ return length_; }
  UInt16 getClauseLength() const
  		{ return ((LikePatternClause *)this)->getLength(); }
  LikePatternHeader* getNextHeader() const 	{ return nextHeader_; }
  CollHeap* getExHeap() const 			{ return exHeap_; }
  //
  // Append the given character to the header.
  //
  void append(const LikePatternStringIterator& i)
  {
    if (i != getLastClause()->getType()) {
      if (getLastClause()->getLength() > 0)
        addNewClause(i);
      else
        getLastClause()->setType(i);
    }
    getLastClause()->append(i);
  }
  //
  // Append a new clause to the header.
  //
  void addNewClause(LikePatternStringIterator::CharType charType)
  {
    length_ += getLastClause()->getLength();
    getLastClause()->append(new(exHeap_) LikePatternClause(charType,
                                                           getPattern() +
                                                             getLength()));
    lastClause_ = getLastClause()->getNextClause();
  }
  //
  // Terminate the clause chain with a NULL pointer and compute the header
  // length.
  //
  void endClauses()
  {
    length_ += getLastClause()->getLength();
  }
  //
  // Append the given header to the current header.
  //
  void append(LikePatternHeader* header) { nextHeader_ = header; }
  //
  // Return TRUE if the header matches the beginning of the given text.
  //
NABoolean matches(const char* text, 
                  Int32 &headerMatchLen, 
                  CharInfo::CharSet cs = CharInfo::ISO88591);

NABoolean matchesR(const char* text, const char* &endText,
                   CharInfo::CharSet cs = CharInfo::ISO88591);

  ExeErrorCode error() const { return error_; }

  void setError(ExeErrorCode exeErrorCode)
    { error_ = exeErrorCode; }

  unsigned char* getEncodedHeader()
    { return encodedHeader_; }

  void setEncodedHeader(unsigned char *encodedHeader)
    { encodedHeader_ = encodedHeader; }

private:
  LikePatternClause* lastClause_;
  UInt16 length_;
  LikePatternHeader* nextHeader_;
  CollHeap* exHeap_;

  // Like pattern may include INVALID_ESCAPE_SEQUENCE and INVALID_CHARACTER
  // since R2.4 (with multi-byte character support). Change error_ from
  // NABoolean to ExeErrorCode.
  ExeErrorCode error_;
  unsigned char *encodedHeader_;

};

class  LikePattern : public LikePatternHeader
{
public:
  //
  // Constructor.
  //
  LikePattern
  ( const LikePatternString& pattern
  , CollHeap* exHeap
  , CharInfo::CharSet cs = CharInfo::ISO88591
  , CharInfo::Collation co = CharInfo::DefaultCollation
  );
  //
  // Destructor.
  //
 ~LikePattern();
  //
  // Return TRUE if the pattern is invalid.
  //
  ExeErrorCode error() const { return error_; }
  //
  // Return TRUE if the pattern matches the given text string.
  //
  NABoolean matches(const char* text, UInt16 textLen,
                    CharInfo::CharSet cs = CharInfo::ISO88591);

private:
  ExeErrorCode error_;

};

#endif
