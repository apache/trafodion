/* -*-C++-*- */
#ifndef COMCHARSETDEFS_H
#define COMCHARSETDEFS_H
/* -*-C++-*-
******************************************************************************
*
* File:         ComCharSetDefs.h
* Description:  C/C++ enumerated constant and macro definitions relating to
*               I18N (e.g., Character Set related stuff)
*
* Created:      10/22/2010
* Language:     C++
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
******************************************************************************
*/

#include "Platform.h"
// No other includes, please!

#ifndef SQLCHARSET_CODE_DEFINED
#define SQLCHARSET_CODE_DEFINED
//
// enum SQLCHARSET_CODE is also defined in w:/cli/sqlcli.h.
//
// Note that the binary value of a SQLCHARSET_CODE enumerated constant
// (e.g., SQLCHARSETCODE_SJIS) must be the same as that of the corresponding
// CharInfo::CharSet enumerated constant (e.g., CharInfo::SJIS) defined in
// the header file w:/common/charinfo.h.
//
// Note that the binary values of SQLCHARSET_CODE and CharInfo::CharSet
// enumerated constants are NOT the same as the those of the corresponding
// cnv_charset enumerated constants defined in w:/common/csconvert.h.  The
// cnv_charset enumerated constants are used with the character translation
// routines defined in w:/common/csconvert.cpp.
//
// The routine declared in w:/common/NLSConversion.h and defined in
// w:/common/conversionLocale.cpp can be used to retrieve the corresponding
// cnv_charset enumerated constant from a given SQLCHARSET_CODE and
// CharInfo::CharSet enumerated constant.
//
// cnv_charset convertCharsetEnum (Int32 /* SQLCHARSET_CODE */ inCharSet);
// 
enum SQLCHARSET_CODE {
  SQLCHARSETCODE_UNKNOWN        =  0,
  SQLCHARSETCODE_ISO88591       =  1,
  SQLCHARSETCODE_KANJI          = -1,
  SQLCHARSETCODE_KSC5601        = -2,
  SQLCHARSETCODE_SJIS           = 10,
  SQLCHARSETCODE_UCS2           = 11,
  SQLCHARSETCODE_EUCJP          = 12,
  SQLCHARSETCODE_BIG5           = 13,
  SQLCHARSETCODE_GB18030        = 14,
  SQLCHARSETCODE_UTF8           = 15,
  SQLCHARSETCODE_MB_KSC5601     = 16,
  SQLCHARSETCODE_GB2312         = 17,
  SQLCHARSETCODE_GBK            = 18,

  /* specifies that the user input string is in the same charset that is
     set as the value of the ISO_MAPPING default in the defaults table.
     Cli will pass in the input string as is to mxcmp without any translation.
     It will also tell mxcmp to treat any unprefixed literals in the 
     input string as iso88591. */
  SQLCHARSETCODE_ISO_MAPPING    = 9999
};
#endif

#ifndef SQLCONVCHARSET_CODE_DEFINED
#define SQLCONVCHARSET_CODE_DEFINED
//
// enum SQLCONVCHARSET_CODE is also defined in w:/cli/sqlcli.h.
//
// Note that the binary values of SQLCONVCHARSET_CODE enumerated constants
// and the corresponding cnv_charset enumerated constants defined in
// w:/common/csconvert.h are the same.
//
// The SQLCONVCHARSET_CODE enumerated constants are used with the character
// translation routines defined in w:/cli/CliExtern.cpp.
//
enum SQLCONVCHARSET_CODE {
  SQLCONVCHARSETCODE_UNKNOWN    = 0,
  SQLCONVCHARSETCODE_UTF8       = 1,
  SQLCONVCHARSETCODE_UTF16      = 2,
  SQLCONVCHARSETCODE_UTF32      = 3,
  SQLCONVCHARSETCODE_ISO88591   = 4,
  SQLCONVCHARSETCODE_SJIS       = 5,
  SQLCONVCHARSETCODE_EUCJP      = 6,
  SQLCONVCHARSETCODE_KSC        = 7,
  SQLCONVCHARSETCODE_BIG5       = 8,
  SQLCONVCHARSETCODE_2312       = 9,
  SQLCONVCHARSETCODE_GB18030    = 10,
  SQLCONVCHARSETCODE_GBK        = 11
};
#endif

#ifndef cnv_charset_DEFINED
#define cnv_charset_DEFINED
//
// enum cnv_charset is also defined in w:/common/csconvert.h
//
// Note that the binary values of cnv_charset enumerated constants must
// match those of the corresponding SQLCONVCHARSET_CODE enumerated constants
// defined above and in w:/cli/sqlcli.h.
//
enum cnv_charset { cnv_UnknownCharSet =  0,  cnv_UTF8     = 1,
                   cnv_UTF16    = 2,         cnv_UTF32    = 3,
                   cnv_ISO88591 = 4,         cnv_SJIS     = 5,
                   cnv_EUCJP    = 6,         cnv_KSC      = 7, 
                   cnv_BIG5     = 8,         cnv_GB2312   = 9,
                   cnv_GB18030  = 10,        cnv_GBK      = 11,
                   cnv_Last_Valid_CS = 11
                  };
#endif

#endif // COMCHARSETDEFS_H
