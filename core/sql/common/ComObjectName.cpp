/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComObjectName.cpp
 * Description:  methods for classes ComObjectName and
 *               ComRoutineActionName
 *
 *
 * Created:      7/26/95
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


#define  SQLPARSERGLOBALS_NADEFAULTS            // must be first
#include <string.h>
#include "BaseTypes.h"
#include "ComASSERT.h"
#include "ComRtUtils.h"
#include "ComMPLoc.h"
#include "ComObjectName.h"
#include "ComSqlText.h"
#include "NAString.h"
#include "ComAnsiNamePart.h"
#include "ComDistribution.h"

#include "SqlParserGlobals.h"                   // must be last


// -----------------------------------------------------------------------
// definitions of methods of class ComObjectName
// -----------------------------------------------------------------------

//
// global friend functions
//

//
// ostream insertion operator
//
ostream&
operator << (ostream &out, const ComObjectName &name)
{
#ifndef NDEBUG
  out << "CLASS:  ComObjectName";

  if (NOT name.isValid())
  {
    out << "  *** INVALID NAME ***  " << endl;
  };

  out << endl;
  out << "        catalogNamePart_ = ["
      << name.catalogNamePart_.getExternalName().length() << "] "
      << name.catalogNamePart_.getExternalName() << endl;
  out << "         schemaNamePart_ = ["
      << name.schemaNamePart_.getExternalName().length()  << "] "
      << name.schemaNamePart_.getExternalName()  << endl;
  out << "         objectNamePart_ = ["
      << name.objectNamePart_.getExternalName().length()  << "] "
      << name.objectNamePart_.getExternalName()  << endl;
  out << "              nameSpace_ = " << (Int32)name.nameSpace_ << endl;
  
#endif		// NDEBUG
  return out;
}

//
// static (global) functions:
//

ComBoolean
ComRoutineActionName::extractUudfUidAndIntActNameFromFunnyIntActNameInOBJECTS_OBJECT_NAME
  ( const ComString & pr_funnyIntActNameStoredInOBJECTS_OBJECT_NAME  // in
  , ComUID          & pr_uudfUid                                     // out
  , ComString       & pr_computedIntActName                          // out
  )
{
  // Example of the format of the for-internal-use-only funny internal-format
  // action name stored in the metadata column OBJECTS.OBJECT_NAME:
  //
  //                 0123456789012345678_$ActionName

  const char *p = pr_funnyIntActNameStoredInOBJECTS_OBJECT_NAME.data();
  if (p[19] NEQ '_')
    return FALSE; // wrong parameter value
  char uidStrBuf[20];
  memcpy(uidStrBuf, p, 19);
  uidStrBuf[19] = '\0';
  ComString uidStr(uidStrBuf);
  RemoveLeadingZeros(uidStr);
  if (NOT NAStringHasOnlyDecimalDigitAsciiChars(uidStr))
    return FALSE; // wrong parameter value
  Int64 i64 = atoInt64(uidStr.data()); // in - const char * src
  if (i64 <= 0)
    return FALSE; // wrong parameter value
  pr_uudfUid = i64;
  pr_computedIntActName = &p[20];
  return TRUE;    // success
}

//
// constructors
//
#ifdef NDEBUG
  #define CHECK_INCOMPLETE
#else				// check for and display warning message
  #define CHECK_INCOMPLETE \
    { if (catalogNamePart_.isEmpty()) getExternalName(); }
#endif

ComObjectName::ComObjectName(CollHeap * h)
: nameSpace_ (COM_UNKNOWN_NAME),
  catalogNamePart_(h),
  schemaNamePart_(h),
  objectNamePart_(h),
  heap_(h),
  flags_(0)
{
}

ComObjectName::ComObjectName ( const ComObjectName &rhs
                             , CollHeap * h // default is NULL - i.e. use C/C+ runtime heap
                             )
  : nameSpace_      (rhs.getNameSpace())
  , catalogNamePart_(rhs.getCatalogNamePart(), h)
  , schemaNamePart_ (rhs.getSchemaNamePart (), h)
  , objectNamePart_ (rhs.getObjectNamePart (), h)
  , flags_          (rhs.getFlags())
  , heap_           (h)
{
}

ComObjectName::ComObjectName ( const NAString         &externalObjectName
			     , const ComAnsiNameSpace  nameSpace
                             , NABoolean               createDropAlias
                             , CollHeap * h
                             , unsigned short toInternalIdentifierFlags
			     )
: nameSpace_ (nameSpace),
  catalogNamePart_(h),
  schemaNamePart_(h),
  objectNamePart_(h),
  heap_(h),
  flags_(0)
{
  if (nameSpace EQU COM_UUDF_ACTION_NAME)
    objectNamePart_.toInternalIdentifierFlags_ |= toInternalIdentifierFlags |
      NASTRING_REGULAR_IDENT_WITH_DOLLAR_PREFIX | NASTRING_DELIM_IDENT_WITH_DOLLAR_PREFIX;
  scan(externalObjectName, createDropAlias, objectNamePart_.toInternalIdentifierFlags_);
  CHECK_INCOMPLETE;
}

// This ctor parses as much of the string as is valid,
// and returns the number of valid bytes scanned.
// Thus, 'a.@' and 'a.b.c.d' would always fail,
// but 'a.b as foo' would build object 'a.b' and return bytesScanned of 3.
//
// The caller must explicitly check for validity.
//
ComObjectName::ComObjectName ( const NAString         &externalObjectName
                             , size_t                 &bytesScanned
			     , const ComAnsiNameSpace  nameSpace
                             , CollHeap * h
			     )
: nameSpace_ (nameSpace),
  catalogNamePart_(h),
  schemaNamePart_(h),
  objectNamePart_(h),
  heap_(h),
  flags_(0)
{
  if (nameSpace EQU COM_UUDF_ACTION_NAME)
    objectNamePart_.toInternalIdentifierFlags_ |=
      NASTRING_REGULAR_IDENT_WITH_DOLLAR_PREFIX | NASTRING_DELIM_IDENT_WITH_DOLLAR_PREFIX;
  bytesScanned = 1;	// special initial value!
  scan(externalObjectName, bytesScanned, FALSE/*createDropAlias*/,
       objectNamePart_.toInternalIdentifierFlags_);
  // Do *not* do this; caller must check isValid() on return:  CHECK_INCOMPLETE;
}

//
// Initializing constructor - If parameter  format  contains
// the value ComAnsiNamePart::EXTERNAL_FORMAT (the default),
// the three *NamePart parameters must contain the name parts in external
// format (the format used by ANSI SQL users).  If parameter
// format  contains the value ComAnsiNamePart::INTERNAL_FORMAT,
// the parameters ...NamePart must contain the name parts in
// internal format (the format stored in the Metadata Dictionary tables).
//
// If an input name part is illegal, the constructed object is empty.
//
ComObjectName::ComObjectName ( const NAString  &catalogNamePart
                             , const NAString  &schemaNamePart
                             , const NAString  &objectNamePart
                             , const ComAnsiNameSpace  nameSpace
                             , const ComAnsiNamePart::formatEnum  format
                             , CollHeap * h
                             )
: nameSpace_(nameSpace),
  catalogNamePart_(h),
  schemaNamePart_(h),
  objectNamePart_(h),
  heap_(h),
  flags_(0)
{
  if (nameSpace EQU COM_UUDF_ACTION_NAME)
    objectNamePart_.toInternalIdentifierFlags_ |=
      NASTRING_REGULAR_IDENT_WITH_DOLLAR_PREFIX | NASTRING_DELIM_IDENT_WITH_DOLLAR_PREFIX;

  switch (format)
  {
  case ComAnsiNamePart::EXTERNAL_FORMAT :
    {
      catalogNamePart_ = catalogNamePart;
      schemaNamePart_  = schemaNamePart;
      ComAnsiNamePart obj ( objectNamePart
                          , ComAnsiNamePart::EXTERNAL_FORMAT
                          , h
                          , objectNamePart_.toInternalIdentifierFlags_
                          );
      objectNamePart_  = obj;
      break;
    }

  case ComAnsiNamePart::INTERNAL_FORMAT :
    {
      ComAnsiNamePart cat(catalogNamePart, ComAnsiNamePart::INTERNAL_FORMAT);
      ComAnsiNamePart sch(schemaNamePart,  ComAnsiNamePart::INTERNAL_FORMAT);
      ComAnsiNamePart obj(objectNamePart,  ComAnsiNamePart::INTERNAL_FORMAT,
                          h, objectNamePart_.toInternalIdentifierFlags_);
      catalogNamePart_ = cat;
      schemaNamePart_  = sch;
      objectNamePart_  = obj;
      break;
    }

  default :
    ABORT("");
  }

  clearIfInvalid();
  CHECK_INCOMPLETE;
}

// This method constructs ComObjectName directly from input strings.  Use this 
// constructor only if the input strings have already been parsed, such as
// calling from CatExecCreateAlias, CatExecDropalias.  The last param
// doNotParse is used to differentiate from the previous constructor.
//
ComObjectName::ComObjectName ( const NAString  &catalogNamePart
                             , const NAString  &schemaNamePart
                             , const NAString  &objectNamePart
                             , const ComAnsiNameSpace  nameSpace
                             , NABoolean doNotParse
                             )
: nameSpace_(nameSpace),
  flags_(0)
{
  if (doNotParse)  // This constructor does not parse input string
  {
    ComAnsiNamePart cat; 
    cat.setExternalName(catalogNamePart);
    cat.setInternalName(catalogNamePart); 
    catalogNamePart_ = cat;
    ComAnsiNamePart sch;
    sch.setExternalName(schemaNamePart);
    sch.setInternalName(schemaNamePart);
    schemaNamePart_ = sch;
    ComAnsiNamePart obj;
    obj.setExternalName(objectNamePart);
    obj.setInternalName(objectNamePart);
    if (nameSpace EQU COM_UUDF_ACTION_NAME)
      obj.toInternalIdentifierFlags_ |=
        NASTRING_REGULAR_IDENT_WITH_DOLLAR_PREFIX | NASTRING_DELIM_IDENT_WITH_DOLLAR_PREFIX;
    objectNamePart_ = obj;
  }
} // end of ComObjectName::ComObjectName

ComObjectName::ComObjectName ( const ComAnsiNamePart &objectNamePart
			     , const ComAnsiNameSpace  nameSpace
			     )
: objectNamePart_ (objectNamePart)
, nameSpace_      (nameSpace)
, flags_(0)
{
  if (nameSpace EQU COM_UUDF_ACTION_NAME)
    objectNamePart_.toInternalIdentifierFlags_ |=
      NASTRING_REGULAR_IDENT_WITH_DOLLAR_PREFIX | NASTRING_DELIM_IDENT_WITH_DOLLAR_PREFIX;

  // clearIfInvalid() not necessary
  CHECK_INCOMPLETE;	// this will always trigger (no cat+sch passed)
}

ComObjectName::ComObjectName ( const ComAnsiNamePart  &schemaNamePart
                             , const ComAnsiNamePart  &objectNamePart
			     , const ComAnsiNameSpace  nameSpace
			     )
: schemaNamePart_ (schemaNamePart)
, objectNamePart_ (objectNamePart)
, nameSpace_      (nameSpace)
, flags_(0)
{
  if (nameSpace EQU COM_UUDF_ACTION_NAME)
    objectNamePart_.toInternalIdentifierFlags_ |=
      NASTRING_REGULAR_IDENT_WITH_DOLLAR_PREFIX | NASTRING_DELIM_IDENT_WITH_DOLLAR_PREFIX;

  clearIfInvalid();
  CHECK_INCOMPLETE;	// this will always trigger (no cat passed)
}

ComObjectName::ComObjectName ( const ComAnsiNamePart  &catalogNamePart
                             , const ComAnsiNamePart  &schemaNamePart 
                             , const ComAnsiNamePart  &objectNamePart
			     , const ComAnsiNameSpace  nameSpace
			     )
: catalogNamePart_ (catalogNamePart)
, schemaNamePart_  (schemaNamePart)
, objectNamePart_  (objectNamePart)
, nameSpace_       (nameSpace)
, flags_           (0)
{
  if (nameSpace EQU COM_UUDF_ACTION_NAME)
    objectNamePart_.toInternalIdentifierFlags_ |=
      NASTRING_REGULAR_IDENT_WITH_DOLLAR_PREFIX | NASTRING_DELIM_IDENT_WITH_DOLLAR_PREFIX;

  clearIfInvalid();
  CHECK_INCOMPLETE;
}

// a protected member -
// The following constructor is invoked by a constructor of class CatRoutineActionName
ComObjectName::ComObjectName( const ComUID    &uudfUid     // UID of UUDF associating with UDF action
                            , const NAString  &catalogNamePart
                            , const NAString  &schemaNamePart
                            , const NAString  &routineActionNamePart
                            , const ComAnsiNameSpace  nameSpace       // must be COM_UUDF_ACTION_NAME
                            , const ComAnsiNamePart::formatEnum  format // default is EXTERNAL_FORMAT
                            , ComBoolean       parseInputNameParts      // default is TRUE
                            , CollHeap * h                              // default is 0 (i.e., NULL)
                            )
  : nameSpace_ (nameSpace),
    catalogNamePart_(h),
    schemaNamePart_(h),
    objectNamePart_(h),
    heap_(h),
    flags_(0)
{
  if (nameSpace NEQ COM_UUDF_ACTION_NAME)
  {
    clear();
    return;
  }

  initialize ( uudfUid
             , catalogNamePart
             , schemaNamePart
             , routineActionNamePart
             , nameSpace
             , format
             , parseInputNameParts
             );

} // ComObjectName::ComObjectName()

//
// virtual destructor
//
ComObjectName::~ComObjectName () {}

//
// virtual cast methods
//

//virtual
const ComRoutineActionName * ComObjectName::castToComRoutineActionName() const
{
  return NULL;
}

// virtual
ComRoutineActionName * ComObjectName::castToComRoutineActionName()
{
  return NULL;
}

//
// assignment operator
//
ComObjectName &
ComObjectName::operator= (const NAString &rhsObjectName)
{
  clear();
  scan(rhsObjectName);
  return *this;
}

ComObjectName &
ComObjectName::operator= (const ComObjectName &rhsObjectName)
{
  this->setCatalogNamePart(rhsObjectName.getCatalogNamePart());
  this->setSchemaNamePart(rhsObjectName.getSchemaNamePart());
  this->setObjectNamePart(rhsObjectName.getObjectNamePart());
  this->setNameSpace(rhsObjectName.getNameSpace());
  return *this;
}

//
// accessor
//

const NAString &
ComObjectName::getCatalogNamePartAsAnsiString(NABoolean v) const
{
  if (v == FALSE)
    return catalogNamePart_.getExternalName();
  else
    return catalogNamePart_.getInternalName();
}

const NAString &
ComObjectName::getSchemaNamePartAsAnsiString(NABoolean v) const
{
  if (v == FALSE)
    return schemaNamePart_.getExternalName();
  else
    return schemaNamePart_.getInternalName();
}

const NAString &
ComObjectName::getObjectNamePartAsAnsiString(NABoolean v) const
{
  if (v == FALSE)
    return objectNamePart_.getExternalName();
  else
    return objectNamePart_.getInternalName();
}


//
// returns an empty string if this object is empty; otherwise,
// returns the ANSI SQL object name, in external-format (the
// format used by ANSI SQL users).  If the catalog name part
// does not exist (in this object), it will not be included
// in the returned object name (in external format).  If the
// schema name part does not exist, only the object name part
// (in external format) is returned.
//
/* virtual */ NAString
ComObjectName::getExternalName(NABoolean formatForDisplay,
			       NABoolean displayedExternally) const
{
  #ifndef NDEBUG
  Int32 ok = 0;
  #endif
  NAString extObjName(getObjectNamePartAsAnsiString());

  if ((NOT isVolatile()) ||
      (NOT formatForDisplay))
    {
      if (NOT schemaNamePart_.isEmpty())
	{
	  extObjName = getSchemaNamePartAsAnsiString() + 
	    NAString(ComSqlText.getPeriod()) + extObjName;
	  
	  if (NOT catalogNamePart_.isEmpty() && NOT displayedExternally)
	    {
#ifndef NDEBUG
	      ok = 1;
#endif
	      extObjName = getCatalogNamePartAsAnsiString() +
		NAString(ComSqlText.getPeriod()) + extObjName;
	    }
	}
    }

  return extObjName;
}

NAString
ComObjectName::getExternalNameWithNameSpace(NABoolean formatForDisplay) const
{
  NAString extObjName;
  const ComRoutineActionName *pActName = castToComRoutineActionName();
  if (pActName EQU NULL)
  {
    extObjName += getExternalName(formatForDisplay);
  }
  else // is a ComRoutineActionName
  {
    const ComObjectName uudfName = pActName->getUudfComObjectName();
    if (uudfName.isValid())
    {
      extObjName += "[Universal Function ";
      extObjName += uudfName.getExternalName(formatForDisplay);
      extObjName += "] ";
    }
    extObjName += getExternalName(formatForDisplay);
  }

  if (getNameSpace() != COM_TABLE_NAME)
  {
    NAString nameSpaceName;
    switch(getNameSpace())
    {
      case COM_TRIGTEMP_TABLE_NAME: 
	nameSpaceName = " (TrigTemp)";
	break;
      case COM_IUD_LOG_TABLE_NAME:
	nameSpaceName = " (IudLog)";
	break;
      case COM_RANGE_LOG_TABLE_NAME:
	nameSpaceName = " (RangeLog)";
	break;
      case COM_GHOST_TABLE_NAME: 
	nameSpaceName = " (GhostTable)";
	break;
      case COM_GHOST_INDEX_NAME: 
	nameSpaceName = " (GhostIndex)";
	break;
      case COM_EXCEPTION_TABLE_NAME: 
	nameSpaceName = " (Exception)";
	break;
      case COM_GHOST_IUD_LOG_TABLE_NAME:
	nameSpaceName = " (GhostIudLog)";
	break;
      case COM_SEQUENCE_GENERATOR_NAME:
	nameSpaceName = " (SG Table)";
	break;
      case COM_UDF_NAME:
	nameSpaceName = " (User-defined Function)";
	break;
      case COM_UUDF_ACTION_NAME:
	nameSpaceName = " (Routine Action)";
	break;
      default:;
    }
    extObjName += nameSpaceName;
  }

  return extObjName;
}

//
// mutators
//

void ComObjectName::applyDefaults( const ComAnsiNamePart &catalogNamePart
                                 , const ComAnsiNamePart &schemaNamePart
                                 )
{
  if (getSchemaNamePart().isEmpty())
    setSchemaNamePart(schemaNamePart);
  if (getCatalogNamePart().isEmpty())
    setCatalogNamePart(catalogNamePart);
}

//
// Reset data members
//

// virtual
void ComObjectName::clear()
{
  catalogNamePart_ = "";
  schemaNamePart_  = "";
  objectNamePart_  = "";
  if (castToComRoutineActionName() NEQ NULL)
    objectNamePart_.toInternalIdentifierFlags_ = ( NASTRING_REGULAR_IDENT_WITH_DOLLAR_PREFIX |
                                                   NASTRING_DELIM_IDENT_WITH_DOLLAR_PREFIX );
  else
    objectNamePart_.toInternalIdentifierFlags_ = NASTRING_ALLOW_NSK_GUARDIAN_NAME_FORMAT;
  nameSpace_       = COM_UNKNOWN_NAME;
  flags_           = 0;
}

// virtual
void ComObjectName::clearIfInvalid()
{
  if (NOT catalogNamePart_.isEmpty())
  {
    if (schemaNamePart_.isEmpty() OR objectNamePart_.isEmpty())
      clear();
    // else okay
  }
  else // catalog name part is empty
  {
    if (NOT schemaNamePart_.isEmpty() AND objectNamePart_.isEmpty())
      clear();
    // else okay
  }
}

//
// protected methods
//

// The following method is invoked by an initializing constructor and one of the
// CatRoutineActionName::set() method.
NABoolean
ComObjectName::initialize ( const ComUID    &uudfUid     // UID of UUDF associating with UDF action
                          , const NAString  &catalogNamePart
                          , const NAString  &schemaNamePart
                          , const NAString  &routineActionNamePart
                          , const ComAnsiNameSpace  nameSpace       // must be COM_UUDF_ACTION_NAME
                          , const ComAnsiNamePart::formatEnum  format // default is EXTERNAL_FORMAT
                          , ComBoolean       parseInputNameParts      // default is TRUE
                          )
{
  if (nameSpace NEQ COM_UUDF_ACTION_NAME)
  {
    clear();
    return FALSE;
  }
  nameSpace_ = nameSpace;

  if (parseInputNameParts)
  {
    if (format EQU ComAnsiNamePart::EXTERNAL_FORMAT)
    {
      catalogNamePart_ = catalogNamePart;
      schemaNamePart_  = schemaNamePart;
      ComRoutineActionNamePart actNP;
      actNP.setUudfUID(uudfUid);
      actNP.setExternalName(routineActionNamePart);
      ComSInt32 status = 0; // < 0 = error -- > 0 length of internal name
      NAString workName(routineActionNamePart);
      status = (ComSInt32)
        ToInternalIdentifier ( workName  // in/out
                             , TRUE      // in - NABoolean upCase           - default is TRUE
                             , FALSE     // in - NABoolean acceptCircumflex - default is FALSE
                             , actNP.toInternalIdentifierFlags_
                             );
      if (status > 0)
      {
        actNP.setInternalName(workName);
        actNP.getRoutineActionNameStoredInOBJECTS_OBJECT_NAME(objectNamePart_);
      }
    }
    else // INTERNAL_FORMAT
    {
      ComAnsiNamePart catNP(catalogNamePart, ComAnsiNamePart::INTERNAL_FORMAT);
      ComAnsiNamePart schNP(schemaNamePart,  ComAnsiNamePart::INTERNAL_FORMAT);
      ComRoutineActionNamePart actNP2(uudfUid,
                                      routineActionNamePart,
                                      ComAnsiNamePart::INTERNAL_FORMAT);
      if (actNP2.isValid())
      {
        catalogNamePart_ = catNP;
        schemaNamePart_  = schNP;
        actNP2.getRoutineActionNameStoredInOBJECTS_OBJECT_NAME(objectNamePart_);
      }
    }
    ComObjectName::clearIfInvalid();
    return ComObjectName::isValid();
  }
  else // DO NOT parseInputNameParts
  {
    // Assume that the input names are legal;
    // if they are not, we have a problem ...
    if (format EQU ComAnsiNamePart::EXTERNAL_FORMAT)
    {
      NAString workName2;

      catalogNamePart_.setExternalName(catalogNamePart);
      workName2 = catalogNamePart;
      ToInternalIdentifier(workName2);
      catalogNamePart_.setInternalName(workName2);

      schemaNamePart_.setExternalName(schemaNamePart);
      workName2 = schemaNamePart;
      ToInternalIdentifier(workName2);
      schemaNamePart_.setInternalName(workName2);

      ComRoutineActionNamePart actNP3;
      actNP3.setUudfUID(uudfUid);
      actNP3.setExternalName(routineActionNamePart);
      ToInternalIdentifier ( workName2 // in/out
                           , TRUE      // in - NABoolean upCase           - default is TRUE
                           , FALSE     // in - NABoolean acceptCircumflex - default is FALSE
                           , actNP3.toInternalIdentifierFlags_
                           );
      actNP3.setInternalName(workName2);
      actNP3.getRoutineActionNameStoredInOBJECTS_OBJECT_NAME(objectNamePart_);
    }
    else // INTERNAL_FORMAT
    {
      catalogNamePart_.setInternalName(catalogNamePart);
      catalogNamePart_.setExternalName(ToAnsiIdentifier(catalogNamePart,
                                                        FALSE/*assertShort*/));
      schemaNamePart_.setInternalName(schemaNamePart);
      schemaNamePart_.setExternalName(ToAnsiIdentifier(schemaNamePart,
                                                       FALSE/*assertShort*/));
      ComRoutineActionNamePart actNP4;
      actNP4.setUudfUID(uudfUid);
      actNP4.setInternalName(routineActionNamePart);
      actNP4.setExternalName(ToAnsiIdentifier(routineActionNamePart,
                                              FALSE/*assertShort*/));
      actNP4.getRoutineActionNameStoredInOBJECTS_OBJECT_NAME(objectNamePart_);
    } // INTERNAL_FORMAT
    return TRUE; // no checking - assume everything went smoothly
  } // DO NOT parseInputNameParts
}

//
// private methods
//

//
// Scans (parses) the specified external-format ANSI SQL object name.
//
NABoolean
ComObjectName::scan( const NAString &externalObjectName
                    ,NABoolean createDropAlias
                    ,unsigned short toInternalIdentifierFlags
                   )
{
  size_t bytesScanned = 0;	// initialize this to 'off' for scan()
  return scan(externalObjectName, bytesScanned, 
              createDropAlias, toInternalIdentifierFlags);
}

//
// This method assumes that the parameter  externalObjectName  only
// contains the external-format object name.  The syntax of an
// object name is
//
//   [ [ <catalog-name-part> ] . <schema-name-part> . ] <object-name-part>
//
// An object name part must be specified; the other name parts are optional.
// Use CmpCommon::applyDefaults(ComObjectName&) to fill in cat + sch.
//
// ## It might be a good idea to extend this method with a
// ## minimum-number-of-nameparts-required.
//
// The method returns the number of bytes scanned via the parameter
// bytesScanned.  If the scanned object name is illegal, bytesScanned
// contains the number of bytes examined when the name is determined
// to be invalid.
//
// If the specified external-format object name is valid, this method
// returns TRUE and decomposes the specified ANSI SQL name part into
// data members catalogNamePart_, schemaNamePart_, and objectNamePart_.
//
// If the specified name is invalid:
//   if what's invalid is trailing stuff AND bytesScanned was passed in nonzero,
//	return TRUE, with data members updated with the scan result
//	preceding the junk
//	(see the bytesScanned ctor for why this is useful!);
//   else,
//	this method returns FALSE and does not change the data members;
//
NABoolean
ComObjectName::scan( const NAString &externalObjectName
                    ,size_t &bytesScanned
                    ,NABoolean createDropAlias
                    ,unsigned short toInternalIdentifierFlags
                   )
{
  size_t count;
  size_t externalObjectNameLen = externalObjectName.length();
  size_t trailingJunkIsOk = bytesScanned;
  bytesScanned = 0;

  // ---------------------------------------------------------------------
  // Scan the leftmost ANSI SQL name part.
  // ---------------------------------------------------------------------

  count = trailingJunkIsOk;
  ComAnsiNamePart part1(externalObjectName, count, 0,
                        createDropAlias
                        , FALSE                     // NABoolean acceptCircumflex
                        , toInternalIdentifierFlags // unsigned short
                        );
  bytesScanned += count;
  if (NOT part1.isValid())
    return FALSE;

  if (bytesScanned == externalObjectNameLen)
  {
    obj__:
      objectNamePart_  = part1;
      return TRUE;			// "obj"
  }

  // Get past the period separator
  if (externalObjectName[bytesScanned++] != '.') {
    if (trailingJunkIsOk) { bytesScanned--; goto obj__; }
    return FALSE;
  }

  // ---------------------------------------------------------------------
  // Scan the next ANSI SQL name part
  // ---------------------------------------------------------------------

  Int32 remainingLen = externalObjectNameLen - bytesScanned;
  NAString remainingName = externalObjectName(bytesScanned, remainingLen);
  count = trailingJunkIsOk;
  ComAnsiNamePart part2(remainingName, count, 0, 
                        createDropAlias
                        , FALSE                     // NABoolean acceptCircumflex
                        , toInternalIdentifierFlags // unsigned short
                        );
  bytesScanned += count;
  if (NOT part2.isValid())
    return FALSE;

  if (bytesScanned == externalObjectNameLen)
  {
    sch_obj__:
    schemaNamePart_  = part1;
    objectNamePart_  = part2;
    return TRUE;			// "sch.obj"
  }

  if (externalObjectName[bytesScanned++] != '.') {
    if (trailingJunkIsOk) { bytesScanned--; goto sch_obj__; }
    return FALSE;
  }

  // ---------------------------------------------------------------------
  // Scan the last ANSI SQL name part
  // ---------------------------------------------------------------------

  remainingLen = externalObjectNameLen - bytesScanned;
  remainingName = externalObjectName(bytesScanned, remainingLen);
  count = trailingJunkIsOk;
  ComAnsiNamePart part3(remainingName, count, 0,
                        createDropAlias
                        , (nameSpace_ == COM_MODULE_NAME) // NABoolean acceptCircumflex
                        , toInternalIdentifierFlags       // unsigned short
                        );
  bytesScanned += count;
  if (NOT part3.isValid())
    return FALSE;

  if (bytesScanned == externalObjectNameLen || trailingJunkIsOk)
  {
    //cat_sch_obj__:
      catalogNamePart_ = part1;
      schemaNamePart_  = part2;
      objectNamePart_  = part3;
      return TRUE;			// "cat.sch.obj"
  }

  // The specified external-format object name contains some extra 
  // trailing characters -- illegal.
  //
  return FALSE;

} // ComObjectName::scan()

// ----------------------------------------------------------------------------
// Test scaffolding
// ----------------------------------------------------------------------------
#if 0
  	#ifndef NDEBUG
	void ComObjTest(const NAString &e)	//## undo .h kludge!
	{
	  ComObjectName o;
	  size_t z;
	  NABoolean b = o.scan(e,z);
	  NAString ee("<"); ee += e; ee += ">";
	  NAString oo("<"); oo += o.getExternalName(); oo += ">";
	  printf("%-16s %-16s %d %d\n", ee.data(), oo.data(), b, z);
	}
	#endif

  	#ifndef NDEBUG
 Int32 test = 0;
	if (test) {
	  ComObjTest("a.b.c");		//ok
	  ComObjTest(" A . B . C ");	//ok
	  ComObjTest("");
	  ComObjTest("a.b.c.");
	  ComObjTest(" A . B . C . ");
	  ComObjTest("a.b.c.d");
	  ComObjTest(" A . B . C . D");
	  ComObjTest(".b.c");
	  ComObjTest("a..c");
	  ComObjTest("a.b.");
	  ComObjTest("  . B . C ");
	  ComObjTest(" A .  . C ");
	  ComObjTest(" A . B .  ");
	  ComObjTest("");
	  ComObjTest("a");		//ok
	  ComObjTest(" a ");		//ok
	  ComObjTest("a.b");		//ok
	  ComObjTest(" a . b ");	//ok
	  ComObjTest("");
	  ComObjTest("a.1.c");
	  ComObjTest("a.zone.c");
	  ComObjTest("z_ne.\"b@2\".c");
	  ComObjTest("\"ZONE\".\"zone\".\"z.z,y+y\"");	//ok
	  ComObjTest("a.\"b.c");
	  ComObjTest("a. \"	 \" .c");
	  ComObjTest("");
	  ComObjTest("...");
	  ComObjTest("a...");
	  ComObjTest("a.b...");
	}
	#endif
#endif // test scaffolding

// -----------------------------------------------------------------------
// definitions of methods of class ComObjectName
// -----------------------------------------------------------------------

//
// global friend functions
//

//
// ostream insertion operator
//
ostream&
operator << (ostream &out, const ComRoutineActionName &name)
{
  return out;
}

//
// constructors
//

// default constructor
ComRoutineActionName::ComRoutineActionName(CollHeap * h) // default of h is NULL - i.e. use C/C++ runtime heap
  : ComObjectName         (h)
  , routineActionNamePart_(h)
  , uudfComObjectName_    (h)
{
}

// copy constructor
ComRoutineActionName::ComRoutineActionName( const ComRoutineActionName &rhs
                                          , CollHeap * h // default of h is NULL - i.e. use C/C++ runtime heap
                                          ) 
  : ComObjectName         (rhs, h)
  , routineActionNamePart_(rhs.getRoutineActionNamePart(), h)
  , uudfComObjectName_    (rhs.getUudfComObjectName(), h)
{
}

// initializing constructors

ComRoutineActionName::ComRoutineActionName( const ComUID   & uudfUid  // e.g., UID of UUDF sas_score
                                          , const NAString & uudfNameAsAnsiName
                                          , const NAString & routineActionNameAsAnsiName
                                          , CollHeap * h // default of h is NULL - i.e. use C/C++ runtime heap
                                          )
  : ComObjectName         (h)
  , routineActionNamePart_(h)
  , uudfComObjectName_    ( uudfNameAsAnsiName
                          , COM_UDF_NAME // const ComAnsiNameSpace nameSpace
                          , FALSE        // NABoolean              createDropAlias = FALSE (the default)
                          , h            // CollHeap *             h
                          )
{
  set(uudfUid, uudfNameAsAnsiName, routineActionNameAsAnsiName);
}

ComRoutineActionName::ComRoutineActionName( const ComUID        & uudfUid  // e.g., UID of UUDF sas_score
                                          , const ComObjectName & uudfObjName
                                          , const NAString      & catalogNamePart
                                          , const NAString      & schemaNamePart
                                          , const NAString      & routineActionNamePart
                                          , const ComAnsiNamePart::formatEnum  format
                                          , ComBoolean       parseInputNameParts  // default is TRUE
                                          , CollHeap * h     // default of h is 0 (i.e., NULL)
                                          )
  : ComObjectName ( uudfUid
                  , catalogNamePart
                  , schemaNamePart
                  , routineActionNamePart
                  , COM_UUDF_ACTION_NAME
                  , format
                  , parseInputNameParts
                  , h
                  )
  , routineActionNamePart_(h)
  , uudfComObjectName_(uudfObjName, h)
{
  initializeRoutineActionNamePart ( uudfUid
                                  , uudfObjName
                                  , catalogNamePart
                                  , schemaNamePart
                                  , routineActionNamePart
                                  , format
                                  , parseInputNameParts
                                  );
}

//
// virtual destructor
//

ComRoutineActionName::~ComRoutineActionName()
{
}

//
// virtual cast methods
//

// virtual
const ComRoutineActionName * ComRoutineActionName::castToComRoutineActionName() const
{
  return this;
}

// virtual
ComRoutineActionName * ComRoutineActionName::castToComRoutineActionName()
{
  return this;
}

//
// accessors
//

const NAString &
ComRoutineActionName::getRoutineActionNamePartAsAnsiString(NABoolean) const
{
  return routineActionNamePart_.getExternalName();
}

//
// returns an empty string if this object is empty; otherwise,
// returns the qualified routine action name, in external-format
// (the format used by ANSI SQL users).  If the catalog name part
// does not exist (in this object), it will not be included
// in the returned qualified routine action name. If the schema
// name part does not exist, only the rightmost name part (i.e.,
// the routine action name part) - in external format - is
// returned.
//
/* virtual */ NAString
ComRoutineActionName::getExternalName(NABoolean formatForDisplay,
                                      NABoolean displayedExternally) const
{
  ComASSERT(NOT isVolatile());
#ifdef _DEBUG
  Int32 ok = 0;
#endif

  NAString extActName(getRoutineActionNamePartAsAnsiString());

  if (NOT formatForDisplay)
  {
    if (NOT getSchemaNamePart().isEmpty())
    {
      extActName = getSchemaNamePartAsAnsiString()
        + NAString(ComSqlText.getPeriod())
        + extActName;

      if (NOT getCatalogNamePart().isEmpty() AND
          NOT displayedExternally)
      {
#ifdef _DEBUG
        ok = 1;
#endif
        extActName = getCatalogNamePartAsAnsiString()
          + NAString(ComSqlText.getPeriod())
          + extActName;
      }
    }
  }

#ifdef _DEBUG
  if (NOT ok)
  {
    if (extActName.isNull())
      cerr << "ERROR: empty ComRoutineActionName" << endl;
    // else
    //   cerr << "WARNING: incomplete ComRoutineActionName " << extActName << endl;
  }
#endif
  return extActName;
}


//
// mutators
//

NABoolean
ComRoutineActionName::set ( const ComUID   &uudfUid  // e.g., UID of UUDF sas_score
                          , const NAString &uudfNameAsAnsiName
                          , const NAString &routineActionNameAsAnsiName
                          )
{
  // set the uudfComObjectName_ data member
  ComObjectName uudfObjName(uudfNameAsAnsiName, COM_UDF_NAME);
  uudfComObjectName_ = uudfObjName;

  // Use the kludge object to decompose the 1-, 2-, or 3-part input routine action name.
  ComObjectName kludge( routineActionNameAsAnsiName
                      , COM_UUDF_ACTION_NAME // ComAnsiNameSpace
                      , FALSE // NABoolean createDropAlias - not applicable
                      );
  if (kludge.isValid())
  {
    ComRoutineActionNamePart actNP( uudfUid
                                  , kludge.getObjectNamePart().getInternalName()
                                  , ComAnsiNamePart::INTERNAL_FORMAT
                                  );
    if (actNP.isValid())
    {
      ComAnsiNamePart objNP;
      NABoolean isOK =
        actNP.getRoutineActionNameStoredInOBJECTS_OBJECT_NAME( objNP  // out
                                                             , TRUE   // ComBoolean performCheck
                                                             );
      if (isOK)
      {
        setCatalogNamePart(kludge.getCatalogNamePart());
        setSchemaNamePart(kludge.getSchemaNamePart());
        setObjectNamePart(objNP);
        routineActionNamePart_ = actNP;
        setNameSpace(COM_UUDF_ACTION_NAME);
      }
    }
  }
  clearIfInvalid();
  return isValid();
}

NABoolean
ComRoutineActionName::set ( const ComUID        & uudfUid                // e.g., UID of UUDF sas_score
                          , const ComObjectName & uudfObjName            // in
                          , const NAString      & catalogNamePart        // in
                          , const NAString      & schemaNamePart         // in
                          , const NAString      & routineActionNamePart  // in
                          , const ComAnsiNamePart::formatEnum format     // in - default is EXTERNAL_FORMAT
                          , ComBoolean parseInputNameParts               // in - default is TRUE
                          )
{
  clear();
  NABoolean isOkay = ComObjectName::initialize ( uudfUid
                                               , catalogNamePart
                                               , schemaNamePart
                                               , routineActionNamePart
                                               , COM_UUDF_ACTION_NAME
                                               , format
                                               , parseInputNameParts
                                               );
  if (NOT isOkay)
    return FALSE;

  return initializeRoutineActionNamePart ( uudfUid
                                         , uudfObjName
                                         , catalogNamePart
                                         , schemaNamePart
                                         , routineActionNamePart
                                         , format
                                         , parseInputNameParts
                                         );
}

//
// helpers
//

// virtual
NABoolean ComRoutineActionName::isEmpty() const
{
  return (routineActionNamePart_.isEmpty() AND
          getSchemaNamePart().isEmpty() AND
          getCatalogNamePart().isEmpty());
}

// virtual
NABoolean ComRoutineActionName::isValid() const
{
  return (NOT routineActionNamePart_.isEmpty());
}

NABoolean ComRoutineActionName::initializeRoutineActionNamePart
          ( const ComUID        & uudfUid                 // e.g., UID of UUDF sas_score
          , const ComObjectName & uudfObjName             // in
          , const NAString      & catalogNamePart         // in
          , const NAString      & schemaNamePart          // in
          , const NAString      & routineActionNamePart   // in
          , const ComAnsiNamePart::formatEnum format      // in - default is EXTERNAL_FORMAT
          , ComBoolean            parseInputNameParts     // in - default is TRUE
          )
{
  uudfComObjectName_ = uudfObjName;
  if (uudfComObjectName_.isValid() AND
      uudfComObjectName_.getNameSpace() EQU COM_UNKNOWN_NAME)
  {
    uudfComObjectName_.setNameSpace(COM_UDF_NAME);
  }

  if (parseInputNameParts)
  {
    ComRoutineActionNamePart actNP(uudfUid,
                                   routineActionNamePart,
                                   format);
    if (actNP.isValid())
      routineActionNamePart_ = actNP;
    setNameSpace(COM_UUDF_ACTION_NAME);
    clearIfInvalid();
    return isValid();
  }
  else // DO NOT check - assume all input parameters are valid
  {
    routineActionNamePart_.setUudfUID(uudfUid);
    if (format EQU ComAnsiNamePart::EXTERNAL_FORMAT)
    {
      routineActionNamePart_.setExternalName(routineActionNamePart);
      NAString actName(routineActionNamePart);
      // ToInternalIdentifer() does do the checking, but if the input
      // parameters are valid, there should be no errors except for
      // the performance penalty.
      ToInternalIdentifier ( actName   // in/out
                           , TRUE      // in - NABoolean upCase           - default is TRUE
                           , FALSE     // in - NABoolean acceptCircumflex - default is FALSE
                           , routineActionNamePart_.toInternalIdentifierFlags_
                           );
      routineActionNamePart_.setInternalName(actName);
    }
    else // INTERNAL_FORMAT
    {
      routineActionNamePart_.setInternalName(routineActionNamePart);
      routineActionNamePart_.setExternalName(ToAnsiIdentifier(routineActionNamePart,
                                                              FALSE/*assertShort*/));
    } // INTERNAL_FORMAT
    setNameSpace(COM_UUDF_ACTION_NAME);
    return TRUE; // no checking - assume that everthing is okay
  } // DO NOT check
}

//
// Reset data members
//

// virtual
void ComRoutineActionName::clear()
{
  ComObjectName::clear();
  routineActionNamePart_.clear();
  uudfComObjectName_.clear();
}

// virtual
void ComRoutineActionName::clearIfInvalid()
{
  if (NOT getCatalogNamePart().isEmpty())
  {
    if (getSchemaNamePart().isEmpty() OR routineActionNamePart_.isEmpty())
      clear();
    // else okay
  }
  else// catalog name part is empty
  {
    if (NOT getSchemaNamePart().isEmpty() AND routineActionNamePart_.isEmpty())
      clear();
    // else okay
  }
}

//
// End of File ComObjectName.cpp
//
