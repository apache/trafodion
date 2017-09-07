/* -*-C++-*-
 *****************************************************************************
 *
 * File:         nchar_mp.h
 * RCS:          $Id: 
 * Description:  The implementation of kanji_char_set and ksc5601_char_set class
 *               
 *               
 * Created:      12/01/2003
 * Modified:     $ $Date: 2007/10/09 19:38:38 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
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

#ifndef _NCHAR_MP_H
#define _NCHAR_MP_H


#include <string.h>
#include <limits.h>

#include "Platform.h"

#include "NAWinNT.h"
#include "NABoolean.h"

////////////////////////////////////////////////////////////////////
// A simple class devoting to the concept of MP KANJI charset 
////////////////////////////////////////////////////////////////////

//LCOV_EXCL_START : cnu  -- As of 8/30/2011, not used in SQ SQL.

class kanji_char_set 
{
public:

   kanji_char_set() {};

   virtual ~kanji_char_set() {};

   static NAWchar space_char() { return ' '; };

   static NAWchar null_char() {return 0;};

static NAWchar underscore_char() 
{ 
#ifdef NA_LITTLE_ENDIAN
// need to flip the two bytes because KANJI_MP data is in multi-byte 
// (big endian order) format
   return 0x5181;
#else
   return 0x8151; 
#endif
};

static NAWchar percent_char() 
{ 
#ifdef NA_LITTLE_ENDIAN
// need to flip the two bytes because KANJI_MP data is in multi-byte 
// (big endian order) format
   return 0x9381;
#else
   return 0x8193;
#endif
};

static NAWchar maxCharValue() { return USHRT_MAX; };

static short bytesPerChar() { return 2; };

private:

};
//LCOV_EXCL_STOP : cnu


////////////////////////////////////////////////////////////////////
// A simple class devoting to the concept of MP KSC5601 charset 
////////////////////////////////////////////////////////////////////

//LCOV_EXCL_START : cnu  -- As of 8/30/2011, not used in SQ SQL.

class ksc5601_char_set 
{
public:

   ksc5601_char_set() {};

   virtual ~ksc5601_char_set() {};

   static NAWchar space_char() { return ' '; };

   static NAWchar null_char() {return 0;};

static NAWchar underscore_char() 
{ 
#ifdef NA_LITTLE_ENDIAN
// need to flip the two bytes because KANJI_MP data is in multi-byte 
// (big endian order) format
   return 0xDFA3;
#else
   return 0xA3DF;
#endif
};

static NAWchar percent_char() 
{ 
#ifdef NA_LITTLE_ENDIAN
// need to flip the two bytes because KANJI_MP data is in multi-byte 
// (big endian order) format
   return 0xA5A3;
#else
   return 0xA3A5;
#endif
};

   static NAWchar maxCharValue() { return USHRT_MAX; };

static short bytesPerChar() { return 2; };

private:

};
//LCOV_EXCL_STOP : cnu


#endif
