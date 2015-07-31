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
#ifndef COM_SIZE_DEFS_H
#define COM_SIZE_DEFS_H

#include "Platform.h"
// No other includes, please!

// All values should be declared as enums. Individual enum literals should
// start with ComMAX to indicate that the definition is common, and that it is
// a max length.


// A Japanese character takes 2 bytes in UCS2, but it takes 3 bytes in UTF8.
// Add (length_in_NAWchar_s / 2) extra bytes to be on the safe side.
// We may want to change the computing formula from multiplying the length
// (in NAWchar elements) by 4 instead of 3.5 if we want to be really safe.
//
//#define COM_COMPUTE_UTF8_BYTE_LEN_FROM_UCS2_NAWCHAR_LEN(length_in_NAWchar_s) \
//  ( length_in_NAWchar_s*(2/*BYTES_PER_NAWCHAR*/+1) + length_in_NAWchar_s/2 )
//
// 2010/12/14 - Decide to use 4 instead of 3.5 as the multiplier to be on the safe side.
//
#define COM_COMPUTE_UTF8_BYTE_LEN_FROM_UCS2_NAWCHAR_LEN(length_in_NAWchar_s) \
  ( length_in_NAWchar_s * 4 /*SQL_UTF8_CHAR_SIZE in BYTES*/ )

//-----------------------------------------------------------------------------
//
//  ANSI (SQL) identifiers and names and other names like UDF parameter names
//
//-----------------------------------------------------------------------------

enum
{

    // ---------------------------------------------------------------------
    // Max length of a 1-part ANSI (SQL) identifier (or another kind of
    // name like cursor or parameter name), in INTERNAL format (i.e., what
    // we store in the metadata columns).
    // ---------------------------------------------------------------------

    // 128 bytes (for internal-format names stored in ISO88591 metadata columns), or
    // 128 NAWchar (or ucs2_t or _ucs2_t) elements (for internal-format names stored
    // in CHARACTER SET UCS2 metadata columns).
    //
    // In the SQL Engine C/C++ or JAVA code, we can use an array of 129 NAWchar elements
    // to store the 1-part UCS2-encoding internal-format name, but most of time we still
    // use the C char data type (e.g., an array of char elements) to store the names.
    // We will translate the names (in ISO88591 or UCS2 format) to UTF8 encoding values
    // before storing them into the arrays of char elements or C string objects.
    // The maximun length, in bytes, of a 1-part UTF8-encoding internal-format name is
    // 512 for the worst case.  Note that a Japanese character takes 2 bytes in UCS2, but
    // it may require 3 bytes in UTF8 encoding format.
    //
    // Note that ComMAX_ANSI_IDENTIFIER_INTERNAL_LEN is the old enumerated constant.
    // We keep the (old) name, but change its value from 128 to 512.

    ComMAX_1_PART_INTERNAL_ISO88591_NAME_LEN         = 128 // for names stored in ISO88591 metadata columns
  , ComMAX_1_PART_INTERNAL_UCS2_NAME_LEN_IN_NAWCHARS = ComMAX_1_PART_INTERNAL_ISO88591_NAME_LEN
  , ComMAX_1_PART_INTERNAL_UCS2_NAME_LEN_IN_BYTES    = ComMAX_1_PART_INTERNAL_UCS2_NAME_LEN_IN_NAWCHARS * 2 /*BYTES_PER_NAWCHAR*/
                                                           // for names stored in UCS2 metadata columns
                                                           // Keep this macro definition and the corresponding
                                                           // one in w:/smdio/CmTableDefs.h in sync.
  , ComMAX_1_PART_INTERNAL_UTF8_NAME_LEN_IN_BYTES    = COM_COMPUTE_UTF8_BYTE_LEN_FROM_UCS2_NAWCHAR_LEN
                                                       ( ComMAX_1_PART_INTERNAL_UCS2_NAME_LEN_IN_NAWCHARS )
  , ComMAX_ANSI_IDENTIFIER_INTERNAL_LEN              = ComMAX_1_PART_INTERNAL_UTF8_NAME_LEN_IN_BYTES

    // ---------------------------------------------------------------------
    // Max length of a 1-part ANSI (SQL) identifier (or another kind of
    // names like cursor or parameter name), in EXTERNAL format (i.e., what
    // the users see; e.g., in error message text).
    // ---------------------------------------------------------------------

    // 258 bytes (for ISO88591 encoding format) or 258 NAWchar elements (for UCS2 encoding format)
    // or 514 bytes (for UTF8 encoding format) for the worst case scenarios.
    //
    // We add 2 elements for delimiting 7-bit ASCII double-quote characters (encoding value 0x22).
    // For ISO88591 and UCS2 encoding cases, we double the internal name length to handle the worst
    // case where all 128 characters in the internal-format name are a 7-bit ASCII double-quote
    // characters.  For UTF8 encoding case, the 514 max-size-in-bytes (defined below) can handle the
    // case where the internal-format name contains 128 7-bit ASCII double quote characters without
    // any problems.
    //
    // Note that ComMAX_ANSI_IDENTIFIER_EXTERNAL_LEN  is the old enumerated constant.
    // We keep the (old) name, but change its value from 258 to 514.

  , ComMAX_1_PART_EXTERNAL_ISO88591_NAME_LEN          = (2 * ComMAX_1_PART_INTERNAL_ISO88591_NAME_LEN) + 2
  , ComMAX_1_PART_EXTERNAL_UCS2_NAME_LEN_IN_NAWCHARS  = (2 * ComMAX_1_PART_INTERNAL_UCS2_NAME_LEN_IN_NAWCHARS) + 2
  , ComMAX_1_PART_EXTERNAL_UCS2_NAME_LEN_IN_BYTES     = ComMAX_1_PART_EXTERNAL_UCS2_NAME_LEN_IN_NAWCHARS * 2 /*BYTES_PER_NAWCHAR*/
  , ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES     = ComMAX_1_PART_INTERNAL_UTF8_NAME_LEN_IN_BYTES + 2
  , ComMAX_ANSI_IDENTIFIER_EXTERNAL_LEN               = ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES

    // ---------------------------------------------------------------------
    // Max length of a 2-part ANSI (SQL) name, in EXTERNAL format (i.e.,
    // what the users see; e.g., a fully-qualified schema name [including
    // the catalog name part] in error message text).  The 7-bit ASCII
    // period separator has the count of 1.  We do not support extra spaces
    // around the period separator. 514*2+1 = 1029
    // ---------------------------------------------------------------------

  , ComMAX_2_PART_EXTERNAL_ISO88591_NAME_LEN         = (2 * ComMAX_1_PART_EXTERNAL_ISO88591_NAME_LEN) + 1
  , ComMAX_2_PART_EXTERNAL_UCS2_NAME_LEN_IN_NAWCHARS = (2 * ComMAX_1_PART_EXTERNAL_UCS2_NAME_LEN_IN_NAWCHARS) + 1
  , ComMAX_2_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES    = (2 * ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES) + 1

    // ---------------------------------------------------------------------
    // Max length of a 3-part ANSI (SQL) name, in EXTERNAL format.
    // ---------------------------------------------------------------------

    // 776 bytes (for ISO88591 encoding format) or 776 NAWchar elements (for UCS2 encoding format)
    // or 1544 bytes for the worst case (for UTF8 encoding format) on Linux platform
    // or 1472 bytes for the worst case (for UTF8 encoding format) on Windows NT platform.
    //
    // We include 4 elements (into the count) for the 7-bit ASCII period name-separators.
    // We do not allow any spaces around the period name-separators (i.e., we do not
    // include these extra spaces into the count) so please avoid including extra spaces
    // around the period name separators into the 3-part name.
    //
    // Note that ComMAX_ANSI_NAME_EXTERNAL_LEN is the old enumerated constant.
    // We keep the (old) name, but change its value from 776 to 1544 (or 1472 for NT).

  , ComMAX_3_PART_EXTERNAL_ISO88591_NAME_LEN          = (3 * ComMAX_1_PART_EXTERNAL_ISO88591_NAME_LEN) + 2
  , ComMAX_3_PART_EXTERNAL_UCS2_NAME_LEN_IN_NAWCHARS  = (3 * ComMAX_1_PART_EXTERNAL_UCS2_NAME_LEN_IN_NAWCHARS) + 2
  , ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES     = (3 * ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES) + 2
  , ComMAX_ANSI_NAME_EXTERNAL_LEN                     = ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES

    // We do not define the ComMAX_ANSI_NAME_INTERNAL_LEN enum constant because
    // there is no such thing as a 3-part ANSI name in internal format.

    // ---------------------------------------------------------------------
    // Max length of a 4-part ANSI (SQL) name, in EXTERNAL format.
    // Example: a 4-part column name like cat.sch.tab.col
    // ---------------------------------------------------------------------

    // We include 3 elements (into the count) for the 7-bit ASCII period name-separators.
    // We do not allow any spaces around the period name-separators (i.e., we do not
    // include these extra spaces into the count) so please avoid including extra spaces
    // around the period name separators into the 4-part name.

  , ComMAX_4_PART_EXTERNAL_ISO88591_NAME_LEN         = (4 * ComMAX_1_PART_EXTERNAL_ISO88591_NAME_LEN) + 3
  , ComMAX_4_PART_EXTERNAL_UCS2_NAME_LEN_IN_NAWCHARS = (4 * ComMAX_1_PART_EXTERNAL_UCS2_NAME_LEN_IN_NAWCHARS) + 3
  , ComMAX_4_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES    = (4 * ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES) + 3

    // ---------------------------------------------------------------------
    // Max length of a 1-part routine action name, in INTERNAL format.
    // ---------------------------------------------------------------------

    // Routine action name, internal format - Max size is 80 UCS2 elements.
    // This is the format stored in ROUTINE_ACTIONS.ACTION_NAME metadata column; e.g., $ACTION
    // The format of the for-internal-use-only routine action name stored in OBJECTS.OBJECT_NAME
    // is: <uudf-uid>_<internal-routine-action-name>; e.g., 01234567890123456789_$ACTION
    // This special for-internal-use-only routine action name format is needed to support
    // different actions with the same name when used with universal user-defined functions;
    // e.g., the $ACTION's in sas_put(T1.C1, '$ACTION') and sas_score('$ACTION', T2.C2) are
    // not the same - sas_put and sas_score in the example are UUDF's with different UUDF UID's.
    // Note that ComMAX_ROUTINE_ACTION_NAME_INTERNAL_LEN is the old enumerated constant.
    // We keep the (old) name, but change its value from 80 to 320 (i.e., 4 * 80).

  , ComMAX_ROUTINE_ACTION_1_PART_INTERNAL_ISO88591_NAME_LEN         = 80
  , ComMAX_ROUTINE_ACTION_1_PART_INTERNAL_UCS2_NAME_LEN_IN_NAWCHARS = 80
  , ComMAX_ROUTINE_ACTION_1_PART_INTERNAL_UTF8_NAME_LEN_IN_BYTES    = COM_COMPUTE_UTF8_BYTE_LEN_FROM_UCS2_NAWCHAR_LEN(80)
  , ComMAX_ROUTINE_ACTION_NAME_INTERNAL_LEN = ComMAX_ROUTINE_ACTION_1_PART_INTERNAL_UTF8_NAME_LEN_IN_BYTES


};

// Guardian names and name parts
enum
{
    // Individual Guardian name part, 8 characters
    ComMAX_GUARDIAN_NAME_PART_LEN                     = 8
    // Fully qualified disk file name, 35 characters
  , ComMAX_FULLY_QUALIFIED_GUARDIAN_NAME_LEN          = (4 * ComMAX_GUARDIAN_NAME_PART_LEN) + 3
    // Fully qualified file name of any kind. Refer to KFNAME (in Enscribe) for explanation.
  , ComMAX_EXTERNAL_GUARDIAN_FNAME_LEN                = 47
};

// The maxinum length in bytes (allowed) of the string appearing in the DETAIL
// clause in Register Component and Create Component Privilege statements.
#define ComMAX_COMPONENT_DETAIL_STRING_LEN_IN_BYTES     80
// Other component-privilege-related limits
#define ComMAX_COMPONENT_PRIV_ABBREV_STRING_LEN_IN_BYTES 2

//
// The size of an array
//
#define occurs(array) (sizeof(array)/sizeof(array[0]))

#endif

