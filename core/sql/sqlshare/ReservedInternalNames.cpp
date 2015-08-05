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
#ifndef RESERVED_INTERNAL_NAMES_CPP
#define RESERVED_INTERNAL_NAMES_CPP

#include <string.h>

//
// Used by the following file:
// sql/common/NAString.cpp
//

//
// returns true if the name is an internal and/or generated name;
// returns false otherwise.
//
static Int32 isDelimitedIdentifierReservedForInternalUse
( const char *delimitedIdentWithoutTheLeadingQuote,
  size_t identLen )
  // delimitedIdentWithoutTheLeadingQuote may or may not be '\0' terminated.
  // Rely on indentLen to get the length of delimitedIdentWithoutTheLeadingQuote
  // Do not rely on the strlen(delimitedIdentWithoutTheLeadingQuote) call.
  // The logic in this routine also handles internal-format name correctly.
{
  // Prefixes of names reserved for internal use
  //
  // Names with @ funny prefix are not included in the
  // array to save space and to improve efficiency.
  //
  // "@BR_...",
  // "@CURRENT_EPOCH"
  // "@EPOCH",
  // "@EXECID",        // used to be "EXECID@"
  // "@F",             // used to be "F@"
  // "@GOP",           // used to be "GOP@"
  // "@ISLAST",        // used to be "ISLAST@"
  // "@LNN_@BR_...",
  // "@MAINTAIN_CONTROL_INFO@",
  // "@MAINTAIN_SCHEMA@",
  // "@NEW",
  // "@OLD",
  // "@OP",            // used to be "OP@"
  // "@ROW_COUNT",     // used to be "ROW_COUNT@"
  // "@ROW_TYPE",      // used to be "ROW_TYPE@"
  // "@SORT_KEY_...",  // where ... is the orginal column name
  // "@SYSKEY",
  // "@..."
  //
  // Note that the SQL/MX Engine scanner, parser, binder, etc.
  // also support regular identifiers with pattern @{A|B}[0-9]+
  // for internal use.
  //
  // Using the following ordered list to search for reserved
  // prefixes may be inefficient when the list becomes
  // long.  We also need to remember to update the list
  // whenever we introduce a new prefix.
  //
  // Starting from 4/7/2007, please use @ prefix for any new
  // internal and/or generated names to improve efficiency.
  //
  const char * reservedNamePrefixes[] =
    {
      "DELTA@",
      "INDIRECT@",
      "KEYTAG@",
      "NEW@",
      "OLD@",
      "PACKED__@",
      "PACKED__IDU__@",
      ""
    };

  if (identLen == 0)
    return false;

  char firstChar = delimitedIdentWithoutTheLeadingQuote[0];
  if (firstChar == '@')
    return true;

  const char *  *p = &reservedNamePrefixes[0];
  size_t len = 0;
  char prefix1stChar;
  for ( ; (prefix1stChar = **p) <= firstChar && prefix1stChar != '\0'; p++ )
  {
    if ( prefix1stChar == firstChar )
    {
      len = strlen(*p);
      if ( identLen >= len  &&
           strncmp(delimitedIdentWithoutTheLeadingQuote, *p, len) == 0 )
        return true;
    }
  } // for

  return false;

} // isDelimitedIdentifierReservedForInternalUse()

#endif // RESERVED_INTERNAL_NAMES_CPP
