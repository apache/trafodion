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
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComSchemaName.C
 * Description:  methods for class ComSchemaName
 *
 * Created:      9/12/95
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */


#define  SQLPARSERGLOBALS_NADEFAULTS		// first

#include <string.h>
#include "ComASSERT.h"
#include "ComMPLoc.h"
#include "ComSchemaName.h"
#include "ComSqlText.h"
#include "NAString.h"

#include "SqlParserGlobals.h"			// last


//
// constructors
//

//
// default constructor
//
ComSchemaName::ComSchemaName () 
{
}

//
// initializing constructor
//
ComSchemaName::ComSchemaName (const NAString &externalSchemaName)
{
  scan(externalSchemaName);
}

//
// initializing constructor
//
ComSchemaName::ComSchemaName (const NAString &externalSchemaName,
                              size_t &bytesScanned)
{
  scan(externalSchemaName, bytesScanned);
}

//
// initializing constructor
//
ComSchemaName::ComSchemaName (const ComAnsiNamePart &schemaNamePart)
: schemaNamePart_ (schemaNamePart)
{
}

//
// initializing constructor
//
ComSchemaName::ComSchemaName (const ComAnsiNamePart &catalogNamePart,
                              const ComAnsiNamePart &schemaNamePart)
: catalogNamePart_ (catalogNamePart)
, schemaNamePart_ (schemaNamePart)
{
  // "cat." is invalid
  if (NOT catalogNamePart_.isEmpty() AND schemaNamePart_.isEmpty())
    clear();
}


//
// virtual destructor
//
ComSchemaName::~ComSchemaName ()
{
}


//
// assignment operator
//
ComSchemaName &ComSchemaName::operator= (const NAString &rhsSchemaName)
{
  clear();
  scan(rhsSchemaName);
  return *this;
}

// 
// accessors
//
const NAString &
ComSchemaName::getCatalogNamePartAsAnsiString(NABoolean) const
{
  return catalogNamePart_.getExternalName();
}

const NAString &
ComSchemaName::getSchemaNamePartAsAnsiString(NABoolean) const
{
  return schemaNamePart_.getExternalName();
}

NAString
ComSchemaName::getExternalName(NABoolean) const
{
  NAString extSchemaName;
  #ifndef NDEBUG
  Int32 ok = 0;
  #endif

  if (NOT schemaNamePart_.isEmpty())
  {
    if (NOT catalogNamePart_.isEmpty())
    { 
  #ifndef NDEBUG
      ok = 1;
  #endif
      extSchemaName = getCatalogNamePartAsAnsiString() + "." +
      		      getSchemaNamePartAsAnsiString();
    }
    else
    {
      extSchemaName = getSchemaNamePartAsAnsiString();
    }
  }

  #ifndef NDEBUG
    if (!ok) 
      cerr << "Warning: incomplete ComSchemaName " << extSchemaName << endl;
  #endif
  return extSchemaName;
}

//
// mutators
//

//
// Resets data members
//
void ComSchemaName::clear()
{
  catalogNamePart_.clear();
  schemaNamePart_.clear();
}


//
//  private methods
//
//
// Scans (parses) input external-format schema name.
//
NABoolean
ComSchemaName::scan(const NAString &externalSchemaName)
{
  size_t bytesScanned;
  return scan(externalSchemaName, bytesScanned);
}

//
// Scans (parses) input external-format schema name.
//
// This method assumes that the parameter  externalSchemaName  only
// contains the external-format schema name.  The syntax of an
// schema name is
//
//   [ <catalog-name-part> ] . <schema-name-part>
//
// A schema name part must be specified; the catalog name part is optional.
//
// The method returns the number of bytes scanned via the parameter
// bytesScanned.  If the scanned schema name is illegal, bytesScanned
// contains the number of bytes examined when the name is determined
// to be invalid.
//
// If the specified external-format schema name is valid, this method
// returns TRUE and saves the parsed ANSI SQL name part into data
// members catalogNamePart_ and schemaNamePart_; otherwise, it returns
// FALSE and does not changes the contents of the data members.
//
NABoolean
ComSchemaName::scan(const NAString &externalSchemaName,
                    size_t &bytesScanned)
{
  size_t count;
  size_t externalSchemaNameLen = externalSchemaName.length();
  bytesScanned = 0;

  #define COPY_VALIDATED_STRING(x)	\
		      ComAnsiNamePart(x, ComAnsiNamePart::INTERNAL_FORMAT)

  if (( SqlParser_Initialized() && SqlParser_NAMETYPE == DF_NSK)       ||
      (!SqlParser_Initialized() && *externalSchemaName.data() == '\\')) {
    ComMPLoc loc(externalSchemaName);
    switch (loc.getFormat()) {
      case ComMPLoc::SUBVOL:
		catalogNamePart_ = COPY_VALIDATED_STRING(loc.getSysDotVol());
		schemaNamePart_  = COPY_VALIDATED_STRING(loc.getSubvolName());
		bytesScanned = externalSchemaNameLen;
		return TRUE;

      case ComMPLoc::FILE:
		if (!loc.hasSubvolName()) {
		  catalogNamePart_ = "";
		  schemaNamePart_  = COPY_VALIDATED_STRING(loc.getFileName());
		  bytesScanned = externalSchemaNameLen;
		  return TRUE;
		}
    }
  }

  // Each ComAnsiNamePart ctor below must be preceded by "count = 0;"
  // -- see ComAnsiNamePart.cpp, and for a better scan implementation,
  //    see ComObjectName::scan() + ComObjectName(bytesScanned) ctor.

  // ---------------------------------------------------------------------
  // Scan the leftmost ANSI SQL name part.
  // ---------------------------------------------------------------------

  count = 0;
  ComAnsiNamePart part1(externalSchemaName, count);
  bytesScanned += count;
  if (NOT part1.isValid())
    return FALSE;

  if (bytesScanned >= externalSchemaNameLen)
  {
    ComASSERT(bytesScanned == externalSchemaNameLen);
    schemaNamePart_  = part1;
    return TRUE;					// "sch"
  }

  // Get past the period separator
  if (NOT ComSqlText.isPeriod(externalSchemaName[bytesScanned++]))
    return FALSE;

  // ---------------------------------------------------------------------
  // Scan the last ANSI SQL name part
  // ---------------------------------------------------------------------

#pragma nowarn(1506)   // warning elimination 
  Int32 remainingLen = externalSchemaNameLen - bytesScanned;
#pragma warn(1506)  // warning elimination 
  NAString remainingName = externalSchemaName(bytesScanned, remainingLen);
  count = 0;
  ComAnsiNamePart part2(remainingName, count);
  bytesScanned += count;
  if (NOT part2.isValid())
    return FALSE;

  if (bytesScanned == externalSchemaNameLen)
  {
    catalogNamePart_ = part1;
    schemaNamePart_  = part2;
    return TRUE;					// "cat.sch"
  }

  // The specified external-format object name contains some extra
  // trailing characters -- illegal.
  //
  return FALSE;

} // ComSchemaName::scan()



void ComSchemaName::setDefinitionSchemaName (const COM_VERSION version)
{
}

