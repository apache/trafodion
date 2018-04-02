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
//
**********************************************************************/
#ifndef PARAM_H
#define PARAM_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         Param.h
 * RCS:          $Id: Param.h,v 1.6 1998/07/20 07:27:28  Exp $
 * Description:  
 *               
 *               
 * Created:      4/15/95
 * Modified:     $ $Date: 1998/07/20 07:27:28 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */


#include "Int64.h"
#include "NAString.h"
#include "charinfo.h"

class SetParam;

class Param {
  char * name;
  char * value;
  char * display_value; // single/multi-byte version of Unicode param value, 
                        // for display
  char * converted_value;
  CharInfo::CharSet cs; // charset of a char-type value. For non-char ones, cs 
                        // take the value of CharInfo::UnknownCharSet 
  
  char * nullValue_;	// a two byte buffer containing the null value (-1)
  NABoolean inSingleByteForm_; // whether the value is in single-byte form. 
                               // 
                               // Here is how the value is set:
                               //  value passed-in            inSingleByteForm
                               //        char*                       TRUE
                               //        NAWchar*                    FALSE 

  NABoolean isQuotedStrWithoutCharSetPrefix_;  // set to TRUE in w:/sqlci/sqlci_yacc.y
                                               // if the parameter value is a string
                                               // literal (i.e., quoted string) AND
                                               // the string literal does not have a
                                               // string literal character set prefix;
                                               // otherwise, this data member is set
                                               // to FALSE.
  NAWchar * utf16StrLit_;            // When isQuotedStrWithoutCharSetPrefix_ is TRUE,
                                     // this data member points to the UTF16 string
                                     // literal equivalent to the specified quoted
                                     // string parameter; otherwise, this data member
                                     // is set to NULL.
  CharInfo::CharSet termCS_;   // When isQuotedStrWithoutCharSetPrefix_ is TRUE, this
                               // data member contains the TERMINAL_CHARSET CQD
                               // setting at the time the SET PARAM command was
                               // executed; otherwise, this data member is set to
                               // CharInfo:UnknownCharSet.

public:
  Param(char * name_, char * value_, CharInfo::CharSet cs = CharInfo::UnknownCharSet);
  Param(char * name_, NAWchar * value_, CharInfo::CharSet cs = CharInfo::UNICODE);
  Param(char * name_, SetParam *);
  ~Param();

  static NAString getExternalName(const char * name)
  					{return NAString("?") + name;}
  NAString getExternalName() const	{return NAString("?") + name;}
  char * getName() const		{return name;}
  char * getValue() const		{return value;}
  char * getDisplayValue(CharInfo::CharSet = CharInfo::ISO88591); 
                            // get the display version of the value. 
                            // returns value data member if non UNICODE params,
                            // and returns the converted for Unicode params. 
                            // The argument is for the desired display charset.
  char * getConvertedValue()		{return converted_value;}
  CharInfo::CharSet getCharSet() { return cs; };
  NABoolean isInSingleByteForm() { return inSingleByteForm_; }; 

  short convertValue(SqlciEnv *, short targetType, Lng32 &targetLength,
		     Lng32 targetPrecision, Lng32 targetScale, 
                     Lng32 vcIndLen,
                     ComDiagsArea*&diags);
  void setName(const char * name_);

  void setValue(const char*, CharInfo::CharSet cs = CharInfo::UnknownCharSet);
  void setValue(const NAWchar*, CharInfo::CharSet cs = CharInfo::UNICODE);
  void setValue(SetParam*);

  short contains(const char * value) const;

  char * getNullValue()			{return nullValue_;}
  short isNull() const			{return nullValue_ ? -1 : 0;}
  void makeNull();

  NAWchar * getUTF16StrLit()            { return utf16StrLit_; }
  void      setUTF16StrLit(NAWchar *utf16Str);

  CharInfo::CharSet getTermCharSet() const                   { return termCS_; }
  void              setTermCharSet(CharInfo::CharSet termCS) { termCS_ = termCS; }

  NABoolean isQuotedStrWithoutCharSetPrefix() const { return isQuotedStrWithoutCharSetPrefix_; }

protected:
  void resetValue_(); // remove the existing value, and converted_value. 
                      // set nullValue_ to 0.

  // helper functions to set the value
  void setValue_(const char * value_, CharInfo::CharSet cs);
  void setValue_(const NAWchar * value_, CharInfo::CharSet cs);
};

#endif


