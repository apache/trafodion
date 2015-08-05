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
#ifndef COMSQLTEXT_H
#define COMSQLTEXT_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComSqlText.h
 * Description:  class represents the current SQL_TEXT which only supports
 *               the ANSI SQL ANSI_GRAPHIC character set.  The work on this
 *               is not complete yet.
 *
 * Created:      7/28/95
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "ComOperators.h"
#include "ComSmallDefs.h"
#include "NABoolean.h"
#include "NAString.h"

#if 0 /* Not needed since ComSqlTextHandle object has no data members */
// -----------------------------------------------------------------------
// definition of class ComSqlTextHandleInitializer
// -----------------------------------------------------------------------

class ComSqlTextHandleInitializer
{
public:

  ComSqlTextHandleInitializer();

private:

  // counter to make sure the initialization is done once
  //
  static UInt32 count_;

};
#endif

// -----------------------------------------------------------------------
// definition of class ComSqlTextHandle
// -----------------------------------------------------------------------

class ComSqlTextHandle
{
public:

  ComSqlTextHandle() {};

  inline char getDoubleQuote() const;
  inline char getPeriod() const;
  inline char getSpace() const;
  inline char getUnderscore() const;
  
  NABoolean isDigit(const char &aChar) const;
  NABoolean isDoubleQuote(const char &aChar) const;
  NABoolean isIdentifierPart(const char &aChar) const;
  NABoolean isIdentifierPartOrUnderscore(const char &aChar) const;
  NABoolean isIdentifierStart(const char &aChar) const;
  NABoolean isNonDoubleQuoteChar(const char &aChar) const;
  NABoolean isPeriod(const char &aChar) const;
  NABoolean isSimpleLatinLetter(const char &aChar) const;
  NABoolean isSimpleLatinLowerCaseLetter(const char &aChar) const;
  NABoolean isSimpleLatinUpperCaseLetter(const char &aChar) const;
  NABoolean isSpace(const char &aChar) const;
  NABoolean isSqlLangChar(const char &aChar) const;
  NABoolean isSqlSpecialChar(const char &aChar) const;
  NABoolean isUnderscore(const char &aChar) const;

private:

};

//
// ComSqlText is defined in ComSqlText.C
//
extern ComSqlTextHandle ComSqlText;

#if 0 /* Not needed since ComSqlTextHandle object will be fully initialized at compile time */
//
// Make sure the global object ComSqlText
// is initialized before it is used
//
static ComSqlTextHandleInitializer ComSqlTextHandleInit;
#endif

// -----------------------------------------------------------------------
// definitions of inline methods
// -----------------------------------------------------------------------


inline char
ComSqlTextHandle::getDoubleQuote() const
{
  return '"';
}

inline char
ComSqlTextHandle::getPeriod() const
{
  return '.';
}

inline char
ComSqlTextHandle::getSpace() const
{
  return ' ';
}

inline char
ComSqlTextHandle::getUnderscore() const
{
  return '_';
}

#endif // COMSQLTEXT_H
