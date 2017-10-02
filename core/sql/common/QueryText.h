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
#ifndef QUERYTEXT_H
#define QUERYTEXT_H
/* -*-C++-*-
 *****************************************************************************
 * File:         QueryText.h
 * Description:  QueryText encapsulates a SQL statement text that can be a
 *               UCS-2 or ANSI (localized) character string. 
 * Created:      7/16/2003
 * Language:     C++
 *****************************************************************************
 */
#include "NAWinNT.h"
#include "wstr.h"

class QueryText {
 public:
  // constructor
  QueryText(char* text, Lng32 charset) : text_(text), charset_(charset) {}

  // simple accessors
  Lng32 charSet() { return charset_; }
  char* text() { return text_; }
  NAWchar* wText() { return (NAWchar*)text_; }

  // other accessors
  inline Int32 canBeUsedBySqlcompTest(char** text);
  Int32 isNullText() { return !text_; }

  inline Int32 octetLength();
  inline Int32 octetLenPlusOne();
  inline Int32 length();
  NABoolean isDISPLAY()
{
  if (!text()) 
    return FALSE;
  if (charSet() == SQLCHARSETCODE_UCS2) {
    NAWchar u[100];
    na_wstr_cpy_convert(u, wText(), 7, -1);
    return na_wcsncmp(u, WIDE_("DISPLAY"), 7) == 0;
  }
  else {
    char u[100];
    str_cpy_convert(u, text(), 7, -1);
    return str_cmp(u, "DISPLAY", 7) == 0;
  }
}

  // mutators
  void setText(char *t) { text_ = t; }
  void setCharSet(Lng32 cs) { charset_ = cs; }

 private:
  char *text_;    // we don't own this memory
  Lng32  charset_;
};

inline Int32 QueryText::canBeUsedBySqlcompTest(char** text)
{
  if (charset_ == SQLCHARSETCODE_UCS2 || !text_) {
    return 0;
  }
  else {
    *text = text_;
    return 1;
  }
}

inline Int32 QueryText::octetLength()
{
  return charset_==SQLCHARSETCODE_UCS2 ? 
    na_wcslen((const NAWchar*)wText()) * 
    CharInfo::maxBytesPerChar((CharInfo::CharSet)charset_) : str_len(text());
}

inline Int32 QueryText::octetLenPlusOne()
{
  return octetLength() + CharInfo::maxBytesPerChar((CharInfo::CharSet)charset_);
}

inline Int32 QueryText::length()
{
  return charset_==SQLCHARSETCODE_UCS2 ? na_wcslen(wText()) : str_len(text());
}



#endif
