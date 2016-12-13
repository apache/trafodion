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
******************************************************************************
*
* File:         ObjectNames.C
* Description:  External SQL object names
*
* Created:      10/25/95
* Language:     C++
*
*
******************************************************************************
*/


#define  SQLPARSERGLOBALS_FLAGS	// must precede all #include's
#define  SQLPARSERGLOBALS_CONTEXT_AND_DIAGS	// must be first
#define  SQLPARSERGLOBALS_NADEFAULTS		// must be first

#include "NAWinNT.h"				// platform-independent stuff

#include "ObjectNames.h"
#include "ComMPLoc.h"
#include "ComRtUtils.h"
#include "SchemaDB.h"

#include "BindWA.h"
#include "ItemColRef.h"
#include "parser.h"
#include "RelScan.h"
#include "StmtNode.h"

#include "CmpSeabaseDDL.h"

#include "ComSmallDefs.h"


#include "SqlParserGlobals.h"			// must be last

#if !defined(__EID) && !defined(ARKFS_OPEN)
#include "ComResWords.h"
#endif
// -----------------------------------------------------------------------
// Context variable for "getXxxAsAnsiString()" methods simplifies the
// calling interface (we don't have to pass a boolean all over the place).
//
// The Parser has made all identifiers into a canonical form:
// regular ones are uppercased, with no enclosing quotes;
// delimited ones have internal double-doublequotes reduced to one dquote,
// and also with no enclosing quotes.
//
// This allows all equality tests (in particular, those done by NAKeyLookup)
// to work correctly.  Output of identifiers, in error messages or in a
// SQL DESCRIBE statement (in the Generator), uses getXxxAsAnsiString()
// to properly re-delimit any delimited identifiers.
// Note that such outputting is only going to be done once per object,
// so there's no need to really optimize it.
// See ToAnsiIdentifier() in NAString.C for details of this operation.
// -----------------------------------------------------------------------
static THREAD_P NABoolean formatAsAnsiIdentifier = FALSE;
#define FORMAT(s) formatAsAnsiIdentifier ? ToAnsiIdentifier(s) : s
#define FORMAT_NO_ASSERT(s) formatAsAnsiIdentifier ? ToAnsiIdentifier(s, FALSE) : s

// -----------------------------------------------------------------------
// Function GetAnsiNameCharSet()
// -----------------------------------------------------------------------
CharInfo::CharSet GetAnsiNameCharSet()
{
  return OBJECTNAMECHARSET;
}

// -----------------------------------------------------------------------
// Methods for class CatalogName
// -----------------------------------------------------------------------

// See .h file comments re Genesis 10-970902-0878.
void CatalogName::setNamePosition(StringPos namePos, NABoolean setDelimited)
{
  namePos_ = namePos;
  isDelimited_ = setDelimited;
}

// -----------------------------------------------------------------------
// Methods for class QualifiedName
// -----------------------------------------------------------------------

NABoolean QualifiedName::operator==(const NAString& simpleName) const
{
  // This method is just syntactic sugar to compare if QualifiedName is empty
  CMPASSERT(simpleName == (const char *)"");
  // Ignore the name space attribute - i.e. use this name space.
  QualifiedName rhs(simpleName);
  rhs.setObjectNameSpace(this->getObjectNameSpace());
  return *this == rhs;
}

QualifiedName & QualifiedName::operator=(const QualifiedName& rhs)
{
  if (&rhs == this) return *this;
  setCatalogName (rhs.getCatalogName ());
  setSchemaName  (rhs.getSchemaName  ());
  setObjectName  (rhs.getObjectName  ());
  setObjectNameSpace (rhs.getObjectNameSpace());
  setNamePosition(rhs.getNamePosition(), FALSE);
  setIsDelimited (rhs.isDelimited    ());
  flagbits_          = rhs.flagbits_ ;
  return *this;
}

NAString QualifiedName::getQualifiedNameAsString(NABoolean formatForDisplay,
						 NABoolean externalDisplay) const
{
  // Preallocate a result buffer that'll be big enough most of the time
  // (so += won't reallocate+copy most of the time).
  NAString result((NASize_T)40, CmpCommon::statementHeap());

  // The object name can be empty if it's an ambiguous column reference
  // (in a join), e.g.  In that case, do NOT prepend "cat.sch." to colrefname
  // which would yield the incorrect string "cat.sch..col".
  //
  if (NOT getObjectName().isNull()) 
  {
    // If volatile, only output the object part of the external name.
    // cat/sch names are internal.
    if ((formatForDisplay) &&
	(isVolatile()))
      {
	result += FORMAT(getObjectName());
      }
    else
      {
	if (NOT getCatalogName().isNull() && NOT externalDisplay) 
	  {
	    if ((SqlParser_NADefaults_Glob != NULL) AND
		(SqlParser_NADefaults_Glob->NAMETYPE_ == DF_SHORTANSI) AND
		(*getCatalogName().data() == '\\'))
	      {
		formatAsAnsiIdentifier = FALSE;
	      }
	    result = FORMAT(getCatalogName());
	    result += ".";
	    CMPASSERT(NOT getSchemaName().isNull());
	  }
	
	if (NOT getSchemaName().isNull())
	  {
	    result += FORMAT(getSchemaName());
	    result += ".";
	  }
	result += FORMAT(getObjectName());
      }
  }
  
  return result;
}

//LCOV_EXCL_START /* : cnu -- not used on Linux */
// -----------------------------------------------------------------------
// makeSafeFilenamePart() and
// QualifiedName::getQualifiedNameAsAnsiNTFilenameString()
//
// Genesis 10-990113-5782
// Make sure that the end result of this function results in a valid
// file name for the system -- whether NT or OSS.
//
// Convert any invalid NT filename characters to underscores.
//
// A valid NT (or OSS) filename may be up to 255 characters including spaces.
// The characters
//	\/:*?<>|"
// are not allowed in the filename at all.
// Furthermore, a POSIX shell (MKS KornShell on NT, OSS on NSK)
// treats the following specially, so they do not make good chars for filenames:
//	$%&();' and space
//
// SQL identifiers can be up to 128 characters long in regular or
// delimited format.
//
// Regular identifiers begin with A-Z or a-z and can contain digits 0-9 or
// underscores.  Regular identifiers are not case sensitive.  Reserved words
// may not be used in a regular identifier.
//
// Delimited identifiers always begin with a " character, and can contain
// both upper and lowercase Latin-1 characters, as well as
//	\/|<>*?:"
//	$%&();' and space
//	+-=[],.
//	digits 0-9 and underscore
//
// This routine will convert all non-alphanumeric chars
// to the underscore character, and result in a filename which when treated
// as an Ansi qualified name (a MODULE name), is composed only of regular
// identifiers -- no "quoting" needed.
//
// The length of a module name that might get generated
// could be a string of over 255 characters.  128 for each of the three parts.
// 2 characters for each part if delimited plus the period separators not
// counting any quotes which are embedded and doubled which might be
// singled again.
//
// There might still be problems because NT is not case sensitive but
// SQL is where delimited identifiers are concerned and that is not being
// addressed here.  We're just going to assume that we won't have two
// catalogs and schemas that are the same delimited if case is not considered
// that will cause problems in NT.  We will be replacing all characters
// that cause the name to be delimited with underscore.
//
// If we need to truncate any one of the three parts (not generally the
// user supplied portion (mainly just the module name portion), then we
// will need to verify that the truncated result does not result in a
// reserved word.  Might possibly happen depending on how we decide to
// handle the case where the length of 2 of the parts nears the 255
// character file name limit for NT and we decide to truncate one part
// down to a size that might result in a reserved word.
// -----------------------------------------------------------------------
//
static void makeSafeFilenamePart(NAString &ns, const NAString &prefix)
{
  size_t len = ns.length();
  for (size_t i=0; i<len; i++) {
    if (!isalnum(ns[i]) && ns[i] != '_')
      ns[i] = '_';
  }

  if (!len || !isalpha(ns[(size_t)0]))	// must begin with an alphabetic
    ns.prepend(prefix);
}

const NAString QualifiedName::getQualifiedNameAsAnsiNTFilenameString() const
{
  // Preallocate a result buffer that'll be big enough most of the time
  // (so += won't reallocate+copy most of the time).
  NAString result((NASize_T)40, CmpCommon::statementHeap());

  NAString catName(CmpCommon::statementHeap());
  NAString schName(CmpCommon::statementHeap());
  NAString objName(CmpCommon::statementHeap());

  formatAsAnsiIdentifier = TRUE;	// put quotes on delimited identifiers

  if ( NOT getCatalogName().isNull() ) {
    catName = FORMAT(getCatalogName());
    makeSafeFilenamePart(catName, "SQLMX_DEFAULT_CATALOG_");
  }
  if ( NOT getSchemaName().isNull() ) {
    schName = FORMAT(getSchemaName());
    makeSafeFilenamePart(schName, "SQLMX_DEFAULT_SCHEMA_");
  }
  if ( NOT getObjectName().isNull() ) {
    objName = FORMAT(getObjectName());
  }
  makeSafeFilenamePart(objName, "SQLMX_DEFAULT_FILE_");

  formatAsAnsiIdentifier = FALSE;	// reset to initial value

  size_t totlen = catName.length() + schName.length() + objName.length() + 2;

  if ( totlen > 255 ) {					// need to truncate
    // +1 so round off doesn't give us less than what we need to chop
    size_t chopLen = totlen - 255 + 1;  
             
    if ( catName.length() - chopLen/2 <= 0 )		// cat too short
      schName.remove( schName.length() - chopLen );
    else if ( schName.length() - chopLen/2 <= 0 )	// sch too short
      catName.remove( catName.length() - chopLen );
    else {						// chop from both
      // remember position starts at 0 and length is 1 more
      chopLen /= 2;
      catName.remove( catName.length() - chopLen - 1 );
      schName.remove( schName.length() - chopLen - 1 );
    }
  }

  if (NOT catName.isNull()) {
    result = catName;
    result += ".";
  }
  if (NOT schName.isNull()) {
    result += schName;
    result += ".";
  }
  result += objName;

  return result;
}
//LCOV_EXCL_STOP /* : cnu -- not used on Linux */

const NAString QualifiedName::getQualifiedNameAsAnsiString(NABoolean formatForDisplay,
							   NABoolean externalDisplay) const
{
  formatAsAnsiIdentifier = TRUE;
  NAString result(getQualifiedNameAsString(formatForDisplay, externalDisplay), 
                  CmpCommon::statementHeap());
  formatAsAnsiIdentifier = FALSE;
  return result;
}

NABoolean SchemaName::matchDefaultPublicSchema()
{
  if (schemaName_.isNull())
    return TRUE;

  NABoolean matched = FALSE;

  // get default schema
  NAString defCat;
  NAString defSch;
  ActiveSchemaDB()->getDefaults().getCatalogAndSchema(defCat, defSch);
  ToInternalIdentifier(defCat);
  ToInternalIdentifier(defSch);

  // get public schema
  NAString publicSchema = ActiveSchemaDB()->getDefaults().getValue(PUBLIC_SCHEMA_NAME);
  ComSchemaName pubSchema(publicSchema);
  NAString pubCat = pubSchema.getCatalogNamePart().getInternalName();
  NAString pubSch = pubSchema.getSchemaNamePart().getInternalName();

  // if catalog was not specified
  NAString catName = (catalogName_.isNull()? defCat:catalogName_);
  pubCat = (pubCat.isNull()? defCat:pubCat);

  if (((catName==defCat) && (schemaName_==defSch)) ||
      ((catName==pubCat) && (schemaName_==pubSch)))
    matched = TRUE;

  return matched;
}

const NAString SchemaName::getSchemaNameAsAnsiString() const
{
  QualifiedName q("X", getSchemaName(), getCatalogName());
  NAString result(q.getQualifiedNameAsAnsiString(), CmpCommon::statementHeap());
  CMPASSERT(result[result.length()-1] == 'X');
  result.remove(result.length()-2);		// remove the '.X'
  return result;
}


// ODBC SHORTANSI -- the actual MPLoc is encoded in the schName
// using underscore delimiters, i.e. "systemName_volumeName_subvolName".
// We make a real MPLoc out of that.
NABoolean QualifiedName::applyShortAnsiDefault(NAString& catName,
					       NAString& schName) const
{
  ComMPLoc loc;
  loc.parse(schName, ComMPLoc::SUBVOL, TRUE/*SHORTANSI*/);

  if (loc.isValid(ComMPLoc::SUBVOL)) {
    catName = loc.getSysDotVol();
    schName = loc.getSubvolName();
    return TRUE;
  }
  //else schName actually was a real Ansi schema,
  //so we return cat & schName unchanged.
  //## Is this the desired behavior, or does this represent an undetected error?
  return FALSE;
}

// Name parts return in string parameters, defaulted if not yet present;
// this object is not modified.
// Function return value is the number of names that match the default,
// {0, 1, 2} = {no matches, catalog matches, catalog&schema match}.
//
// If NAMETYPE is NSK, the SchemaDB puts the current MPLOC into the defCatSch;
// so this method has to handle **only one** tiny NSK naming detail.
//
Int32 QualifiedName::extractAndDefaultNameParts(const SchemaName& defCatSch
					     , NAString& catName 	// OUT
					     , NAString& schName	// OUT
					     , NAString& objName	// OUT
                                             ) const
{
  catName = getCatalogName();
  schName = getSchemaName();
  objName = getObjectName();
  CMPASSERT(NOT objName.isNull());		// no default for this!

  {
    if (catName.isNull()) {
      if((ActiveSchemaDB()->getDefaults().schSetToUserID()) &&
         (SqlParser_NAMETYPE == DF_SHORTANSI))
      {
        // If NAMETYPE is SHORTANSI and  schema is not set 
        // in DEFAULTS table or by user, for dml, catName  
        // gets \SYS.$VOL from set or default MPLOC. 
        catName =  SqlParser_MPLOC.getSysDotVol();
      }
      else
      {
      // If NAMETYPE NSK, catName will become  \SYS.$VOL
        catName = defCatSch.getCatalogName();
      }
    }
    else if (SqlParser_NAMETYPE == DF_NSK &&
	     *catName.data() == '$' &&
	     SqlParser_MPLOC.hasSystemName()) {
      // If user specified only a $VOL, fill in the current default \SYS.
      catName.prepend(SqlParser_MPLOC.getSystemName() + ".");
    }

    if (schName.isNull()) {
      if((ActiveSchemaDB()->getDefaults().schSetToUserID()) &&
         (SqlParser_NAMETYPE == DF_SHORTANSI))
      {
        // If NAMETYPE is SHORTANSI and  schema is not set 
        // in DEFAULTS table or by user, for dml, schName  
        // gets subvol from set or default MPLOC. 
        schName =  SqlParser_MPLOC.getSubvolName();
      }
      else
        schName = defCatSch.getSchemaName();
    }
  }

  CMPASSERT(NOT catName.isNull());
  CMPASSERT(NOT schName.isNull());

  Int32 defaultMatches = 0;
  if (catName == defCatSch.getCatalogName()) {
    defaultMatches++;
    if (schName == defCatSch.getSchemaName())
      defaultMatches++;
  }

  // Next comes support for table name resolution for SHORTANSI nametype, 
  // implemented as internal feature for ODBC use only. 
  //
  // For the name resolution of table name when nametype is SHORTANSI, 
  // check is made to see if schName contains an '_', thus ensuring
  // the method applyShortAnsiDefault is called only once.
  // Correct syntax for schName is "systemName_volumeName_subvolName"
  // of the MPLoc where the objName is actually located.
  // Complete syntax checking is done inside applyShortAnsiDefault.
  //
  if (SqlParser_NAMETYPE == DF_SHORTANSI && schName.first('_') != NA_NPOS) {
    applyShortAnsiDefault(catName, schName);
  }

  return defaultMatches;
}

// Applies the default catalog & schema to *this -- if either name part of this
// is empty, it is filled in (updated) with the default.
//
Int32 QualifiedName::applyDefaults(const SchemaName& defCatSch)
{
  return extractAndDefaultNameParts( defCatSch 
				   , catalogName_
                                   , schemaName_
                                   , objectName_
                                   );
}

// This function copies the above applyDefaults() function
// and includes checking for the object existence and 
// search in the public schema if necessary.
// This is used to replace the above function when 
// we need to consider PUBLIC_SCHEMA_NAME
Int32 QualifiedName::applyDefaultsValidate(const SchemaName& defCatSch,
                                         ComAnsiNameSpace nameSpace)
{
  // need to try public schema if it is specified
  // and the object schema is not specified
  NAString publicSchema = "";
  CmpCommon::getDefault(PUBLIC_SCHEMA_NAME, publicSchema, FALSE);
  ComSchemaName pubSchema(publicSchema);
  NAString pubSchemaIntName = "";
  if ( getSchemaName().isNull() && 
       !pubSchema.isEmpty() )
  {
    pubSchemaIntName = pubSchema.getSchemaNamePart().getInternalName();
  }

  Int32 ret = extractAndDefaultNameParts( defCatSch 
				                              , catalogName_
                                      , schemaName_
                                      , objectName_
                                   );

  // try public schema if the table does not exist 
  if (!pubSchemaIntName.isNull())
  {
    *(CmpCommon::diags()) << DgSqlCode(-4222)
                          << DgString0("Public Access Schema");
  }

  return ret;
}

// Constructor that parses a 1-, 2-, or 3-part external (Ansi) name string
// and optionally applies default catalog and schema to it.
// Use this on a string gotten from a trusted source (Sql Catalog), because
// if it doesn't parse, the ctor cannot return an error so it CMPASSERTs.
//
// This code cloned for CorrName::applyPrototype below.
//
QualifiedName::QualifiedName(const NAString &ansiString, 
                             Int32 minNameParts,
                             CollHeap * h, 
                             BindWA *bindWA) :
     SchemaName(h),
     objectName_(h),
     objectNameSpace_(COM_UNKNOWN_NAME),
     flagbits_(0)
{
  if (HasMPLocPrefix(ansiString.data())) {
    ComMPLoc loc(ansiString);
    catalogName_ = loc.getSysDotVol();
    schemaName_ = loc.getSubvolName();
    objectName_ = loc.getFileName();
  }
  else
  {
    CmpContext *cmpContext = bindWA ? bindWA->currentCmpContext() : NULL;
    Parser parser(cmpContext);
    NAString ns("TABLE " + ansiString + ";", CmpCommon::statementHeap());
#pragma nowarn(1506)   // warning elimination 
    // save the current parserflags setting
    ULng32 savedParserFlags = Get_SqlParser_Flags (0xFFFFFFFF);
    StmtQuery *stmt = (StmtQuery *)parser.parseDML(ns, ns.length(), GetAnsiNameCharSet());
    // Restore parser flags settings 
    Set_SqlParser_Flags (savedParserFlags);
#pragma warn(1506)  // warning elimination 
    if (stmt) {
      CMPASSERT(stmt->getOperatorType() == STM_QUERY);
      *this = stmt->getQueryExpression()->getScanNode()->getTableName().getQualifiedNameObj();
      delete stmt;
    } else {
      // It is possible for the parser to get errors when parsing SQL/MP
      // stored text.  The caller is expected to check the contents of
      // this QualifiedName.
      //
      return;
    }
  }

  Int32 nameParts = 0;
  if (minNameParts > 0) {
    nameParts = getCatalogName() != "" ? 3 :
		getSchemaName()  != "" ? 2 :
		getObjectName()  != "" ? 1 : 0;
    CMPASSERT(nameParts >= minNameParts);
  }

  if (bindWA && nameParts < 3)
    applyDefaults(bindWA->getDefaultSchema());
} // end of QualifiedName::QualifiedName 

QualifiedName::QualifiedName(const ComObjectName &comObjNam, CollHeap * h)
: SchemaName (comObjNam.getSchemaNamePart ().getInternalName(),
	      comObjNam.getCatalogNamePart().getInternalName(), h),
  objectName_(comObjNam.getObjectNamePart ().getInternalName(), h),
  objectNameSpace_(comObjNam.getNameSpace()),
  flagbits_(0)
{}

// See comments for ComMPLoc::getMPName(int *lenArray).
// The array size of 5 is CollationInfo::MAX_NAME_PARTS + 1.
//
const NAString QualifiedName::getQualifiedNameAsAnsiString(
					    size_t *lenArray /* array[5] */
					    ) const
{
  lenArray[0] = lenArray[1] = lenArray[2] = lenArray[3] = lenArray[4] = 0;
  NAString n( getQualifiedNameAsAnsiString(), CmpCommon::statementHeap());

  lenArray[1] = n.length();	// [1] = length of full name
  size_t a = 1;
  size_t i = 0;
  const char *s = n.data();
  for (NABoolean quoted = FALSE; *s; i++, s++) {
    if (*s == '"')
      quoted = !quoted;
    if (*s == '.' && !quoted) {
      lenArray[++a] = i + 1;	// [2] thru [4] = offsets of trailing name parts
    }
  }
  lenArray[0] = a;		// [0] = the count of size_t's being returned
   				// (length + up to 3 offsets)
				// (aka the number of nameparts)
  return n;			// Return the external-format string as well.
}

// If any parts of the name were defaulted (input param = nbr)
//  and NAMETYPE was specified as other than ANSI
//    return FALSE;  else return TRUE.

NABoolean QualifiedName::verifyAnsiNameQualification(
			 Int32 nbrPartsDefaulted) const
{
  if (nbrPartsDefaulted > 0)
    if ((SqlParser_NADefaults_Glob != NULL) AND
        (SqlParser_NADefaults_Glob->NAMETYPE_ != DF_ANSI))
      return FALSE;

  return TRUE;

}

NABoolean QualifiedName::isHive(const NAString &catName) 
{
  NAString hiveDefCatName = "";
  CmpCommon::getDefault(HIVE_CATALOG, hiveDefCatName, FALSE);
  hiveDefCatName.toUpper();
  
  if (CmpCommon::getDefault(MODE_SEAHIVE) == DF_ON &&
      ((NOT catName.isNull()) &&
       ((catName == HIVE_SYSTEM_CATALOG) ||
	(catName == hiveDefCatName))))
    return TRUE;

  return FALSE;
}

NABoolean QualifiedName::isHive() const
{
  return isHive(getCatalogName());
}

NABoolean QualifiedName::isHbase(const NAString &catName) 
{
  return CmpSeabaseDDL::isHbase(catName);
}

NABoolean QualifiedName::isHbase() const
{
  return isHbase(getCatalogName());
}

NABoolean QualifiedName::isSeabase(const NAString &catName) 
{
  return CmpSeabaseDDL::isSeabase(catName);
}

NABoolean QualifiedName::isSeabase() const
{
  return isSeabase(getCatalogName());
}

NABoolean QualifiedName::isSeabaseMD() const
{
  return CmpSeabaseDDL::isSeabaseMD(
				     getCatalogName(), getSchemaName(), getObjectName());
}

NABoolean QualifiedName::isSeabasePrivMgrMD() const
{
  return CmpSeabaseDDL::isSeabasePrivMgrMD
    (getCatalogName(), getSchemaName());
}

NABoolean QualifiedName::isHbaseMappedName() const
{
  return ((isSeabase(getCatalogName())) &&
          (ComIsHbaseMappedSchemaName(getSchemaName())));
}

// -----------------------------------------------------------------------
// Methods for class CorrName
// -----------------------------------------------------------------------

ULng32 CorrName::hash() const
{
  if (corrName_ != (const char *)"") {
    if (formatAsAnsiIdentifier)
       return ToAnsiIdentifier(corrName_).hash();
    else
       return corrName_.hash();
  } else
    return getQualifiedNameObj().hash();
}


CorrName & CorrName::operator = (const CorrName & rhs)
{
  if (this==&rhs) return *this ;

  qualName_          = rhs.qualName_ ;
  corrName_          = rhs.corrName_ ;
  //ct-bug-10-030102-3803 -Begin
  ugivenName_        = rhs.ugivenName_ ;
  //ct-bug-10-030102-3803 -End
  bound_             = rhs.bound_ ;
  flagbits_          = rhs.flagbits_ ;
  prototype_         = rhs.prototype_ ;
  defaultMatchCount_ = rhs.defaultMatchCount_ ; 

  return *this ;
}

void CorrName::setCorrName(const NAString& corrName)
{
  // This used to be called only by Parser, and these asserts worked fine.
  // Now called by RI and IM binding, these asserts are no longer valid.
  // 
  //   CMPASSERT(corrName != "");		// parameter nonempty
  //   CMPASSERT(corrName_ == ""		// prev name empty
  //	    // kludge for SQLMP(NSK) name; initial (non-ANSI) release only ##
  //	    || getQualifiedNameObj().getCatalogName().data()[0] == '\\'
  //	   );
  corrName_ = corrName;
}
//ct-bug-10-030102-3803 -Begin
void CorrName::setUgivenName(const NAString& ugivenName)
{
  ugivenName_ = ugivenName;
}
//ct-bug-10-030102-3803 -End

// We come here from syntax like
//	SELECT * FROM :hv PROTOTYPE 'cat.sch.tbl';  -- host variable, static SQL
//	TABLE $ev;				    -- env var, static or dynam
// (Internally, both host vars and env vars are HostVar objects.)
//
// If there's an environment variable, get its value at time of compilation,
// and stick that into the internal prototype value.  Generator will need
// to save these env var name/value pairs in the generated code;
// Executor needs to use the saved compile-time value of any name that is not
// defined at run-time.
//
// If there's a prototype value, parse it as an actual table name
// (1, 2, or 3-part name) and overwrite the bogus values in *this with the
// parsed prototype value.  Our caller, applyDefaults, will overwrite any
// remaining blank name parts.  Generator needs to save the host var names
// for actual input at run-time, and to save the prototype values for
// similarity check at run-time.
//
// We avoid re-parsing and re-overwriting this CorrName by checking/setting
// its bound state.  Note that we do not rely on its prototype's bound state
// because that is a separate, pointed at object: some Binder subroutines
// make local copies of CorrNames, and any apply methods invoked on the locals
// would not be propagated to the caller's originals: relying on the then True
// value of the prototype's bound state would be fallacious.
//
// If no error in proto, node is bound and bindWA errStatus unchanged
// (note that not having a proto is not an error).
// If error in proto, node is left unbound and bindWA errStatus is set.
//
void CorrName::applyPrototype(BindWA *bindWA)
{
  if (nodeIsBound()) return;
  HostVar *proto = getPrototype();
  if (!proto) {
    markAsBound();
    return;
  }

  // CMPASSERT(this == proto->getPrototypeTarget());
  CMPASSERT(!proto->getName().isNull());
  if (proto->isEnvVar()) {
    char *value = getenv(proto->getName());
    if (!value) {
      // Environment variable has no defined value.
      *CmpCommon::diags() << DgSqlCode(-4086) << DgString0(proto->getName());
      bindWA->setErrStatus();
      return;
    }
    // upcase value returned by getenv
#pragma nowarn(1506)   // warning elimination 
    Int32 len = strlen(value);
#pragma warn(1506)  // warning elimination 
    char * ucValue = new (bindWA->wHeap()) char[len+1];
    str_cpy_convert(ucValue, value, len, -1/*upshift*/);
    ucValue[len] = 0;
    proto->getPrototypeValue() = ucValue;
    // do not free "ucValue" here, it is still used in "proto"
    // to prevent Coverity RESOURCE_LEAK error, add the following
    // coverity[leaked_storage]
  }

  // defines can not be used on linux platform
  if (proto->isDefine()) {
      *CmpCommon::diags() << DgSqlCode(-4155) << DgString0(proto->getName());
      bindWA->setErrStatus();
      return;
  }

  CMPASSERT(!proto->getPrototypeValue().isNull());

  // Some of the following code is cloned from the QualifiedName ctor above
  Parser parser(bindWA->currentCmpContext());
  NAString ns("TABLE " + proto->getPrototypeValue() + ";",
              CmpCommon::statementHeap());
#pragma nowarn(1506)   // warning elimination 
  // save the current parserflags setting
  ULng32 savedParserFlags = Get_SqlParser_Flags (0xFFFFFFFF);
  StmtQuery *stmt = (StmtQuery *)parser.parseDML(ns, ns.length(), GetAnsiNameCharSet());
  // Restore parser flags settings 
  Set_SqlParser_Flags (savedParserFlags);
#pragma warn(1506)  // warning elimination 
  if (stmt) {
    CMPASSERT(stmt->getOperatorType() == STM_QUERY);
    CorrName &protoCorrName = 
      		stmt->getQueryExpression()->getScanNode()->getTableName();

    // Unless the hostvar type was forced directly,
    // copy the special table type to me, the prototype may be a
    // resource fork
    if (getSpecialType() == ExtendedQualName::NORMAL_TABLE)
      setSpecialType(protoCorrName);

    // This if-test prevents pathologies such as
    //	   SELECT col FROM :hv1 PROTOTYPE ':hv2 PROTOTYPE ''tbl''';
    // but allows
    //	   SELECT col FROM :hv1 PROTOTYPE '$ev';
    //
    // (The assertion below ensures that only host var syntax allows prototypes,
    // that you can't say ... FROM $ev PROTOTYPE '...'.)
    //
    HostVar *proto2 = protoCorrName.getPrototype();
    CMPASSERT(!proto2 || !proto->isEnvVar());
    if (!proto2 || proto2->isEnvVar()) {

      if (proto2) {
        // Here if proto2->isEnvVar.
	// Recurse (once only; the assertion above ensures that)
	// to get the value of the var, parse it and all.
	protoCorrName.applyPrototype(bindWA);
	if (bindWA->errStatus()) return;
      }

      #ifndef NDEBUG
	if (getenv("HV_DEBUG"))
	  cout << "HostVar/Prototype: parsed (" 
	       << (Int32)proto->nodeIsBound() << ") "
	       << proto->getName() << " " 
	       << protoCorrName.getExposedNameAsAnsiString() << endl;
      #endif

      // assert *before* overwriting with 0
	//      CMPASSERT(!getQualifiedNameObj().getNamePosition());
      getQualifiedNameObj() = protoCorrName.getQualifiedNameObj();

      proto->setPrototypeType(HostVar::QUALIFIEDNAME);

      // mark that we, the trusted CorrName, are bound
      markAsBound();
    }
  }

  if (!nodeIsBound()) {
    // Clear parser syntax error and emit "Prototype value not valid"
    #ifndef NDEBUG
      if (!getenv("HV_DEBUG"))
    #endif
    CmpCommon::diags()->clear();
    *CmpCommon::diags() << DgSqlCode(-4087) 
      << DgString0(proto->getPrototypeValue());
    bindWA->setErrStatus();
  }

  delete stmt;

} // applyPrototype

// First, always calls applyPrototype -- so may fill in one or more of 
// this's name parts -- hence this method is not const.
//
// Second, delegates to QualifiedName::extractAndDefaultNameParts 
// to fill in the return name parts --
// *EXCEPT* in the case that this CorrName contains a correlation name,
// masking a qualified name like "c.s.t", -- then we always return 0
// and the name parts are *not* filled in with defaults (they're meaningless).
//
Int32 CorrName::extractAndDefaultNameParts(BindWA *bindWA,
					 const SchemaName& defCatSch,
					 NAString& catName,	// OUT
					 NAString& schName,	// OUT
					 NAString& objName)	// OUT
{
  // For performance, avoid redoing work if we've done it before
  if (defaultMatchCount_ < 0) {		// initially -1
  
    applyPrototype(bindWA);
    // if (!nodeIsBound()) return -1;	// caller will check bindWA-errStatus()

    if (getCorrNameAsString() == "") {	// CorrName is "qual", not "corr"
      CMPASSERT(NOT isFabricated());  	// only corr names can be fabricated
      defaultMatchCount_ = getQualifiedNameObj().applyDefaults(defCatSch);
      // override schema, if default schema is "from" schema
      // increase count so columns like t.a will be valid
      if ( bindWA->overrideSchemaEnabled()
        && (bindWA->getOsFromSchema() == defCatSch.getSchemaName()) 
        && (bindWA->getOsToSchema() == getQualifiedNameObj().getSchemaName()) )
        defaultMatchCount_++;
    } else
      defaultMatchCount_ = 0;		// "corr" masks "qual", always return 0
  }

  catName = getQualifiedNameObj().getCatalogName();		// OUT
  schName = getQualifiedNameObj().getSchemaName();		// OUT
  objName = getQualifiedNameObj().getObjectName();		// OUT
  
  return defaultMatchCount_;
}

// Same as above except *ALWAYS* delegates to QN::eADNParts,
// *even if* this CorrName contains a correlation name string.
// (And thus it's not safe to set the defaultMatchCount_ member.)
//
Int32 CorrName::applyDefaults(BindWA *bindWA,
			    const SchemaName& defCatSch)
{
  if (defaultMatchCount_ >= 0) return defaultMatchCount_;	// for perf

  applyPrototype(bindWA);
  // if (!nodeIsBound()) return -1;	// caller will check bindWA-errStatus()

  if (isFabricated()) return (defaultMatchCount_ = 0);	// assignment, not ==

  Int32 retval = getQualifiedNameObj().applyDefaults(defCatSch);
  if (retval == -1)         // Only NSK platform will return -1
   bindWA->setErrStatus();
  return retval;
}

NABoolean CorrName::isHive() const
{
  return getQualifiedNameObj().isHive();
}

NABoolean CorrName::isInHiveDefaultSchema() const
{
  return (isHive() &&
          getQualifiedNameObj().getSchemaName() ==
                                  HIVE_SYSTEM_SCHEMA);
}

NABoolean CorrName::isSeabase() const
{
  return getQualifiedNameObj().isSeabase();
}

NABoolean CorrName::isSeabaseMD() const
{
  return getQualifiedNameObj().isSeabaseMD();
}

NABoolean CorrName::isSeabasePrivMgrMD() const
{
  return getQualifiedNameObj().isSeabasePrivMgrMD();
}

NABoolean CorrName::isHbase() const
{
  return getQualifiedNameObj().isHbase();
}

NABoolean CorrName::isHbaseCell() const
{
  if ((getQualifiedNameObj().isHbase()) &&
      (getQualifiedNameObj().getSchemaName() == "_CELL_"))
    return TRUE;
  else
    return FALSE;
}

NABoolean CorrName::isHbaseRow() const
{
  if ((getQualifiedNameObj().isHbase()) &&
      (getQualifiedNameObj().getSchemaName() == "_ROW_"))
    return TRUE;
  else
    return FALSE;
}

NABoolean CorrName::isHbaseMap() const
{
  return getQualifiedNameObj().isHbaseMappedName();
}

NAString CorrName::getCorrNameAsString() const
{
  return FORMAT(corrName_);
}

//## inline?
NAString CorrName::getQualifiedNameAsString(NABoolean formatForDisplay) const
{
  return getQualifiedNameObj().getQualifiedNameAsString(formatForDisplay);
}

//## inline?
const NAString CorrName::getText() const
{
  NAString result = getExtendedQualNameObj().getText();
  if (getCorrNameAsString() != "")
    result += NAString(" ") + ToAnsiIdentifier(getCorrNameAsString());
  return result;
}

// See ANSI 6.3.
// Fabricated names do not get exposed.
const NAString CorrName::getExposedNameAsString
(NABoolean debug,
 NABoolean formatForDisplay) const
{
  if (isFabricated() AND NOT debug)
    return "";
  else if (NOT getCorrNameAsString().isNull())
    return getCorrNameAsString();
  else
    return getQualifiedNameAsString(formatForDisplay);		//##forceAnsi
}

const NAString CorrName::getExposedNameAsAnsiString
(NABoolean debug,
 NABoolean formatForDisplay) const
{
  formatAsAnsiIdentifier = TRUE;
  NAString result(getExposedNameAsString(debug, formatForDisplay), 
    CmpCommon::statementHeap());
  formatAsAnsiIdentifier = FALSE;
  return result;
}

const NAString CorrName::getExposedNameAsStringWithPartitionNames() const
{
    if (isFabricated())
    return "";
  else if (NOT getCorrNameAsString().isNull())
    return getCorrNameAsString();
  else
    return getExtendedQualNameObj().getExtendedQualifiedNameAsString();
}

// This method was the == operator before the partnClause was added
// Since we need to skip PartnClause when comparing corrnames that are
// contained in a ColRefName, we are placing the comparision of all
// other data members in this method. This method will be called
// by the new == operator.
NABoolean CorrName::isEqualWithPartnClauseSkipped(const CorrName& other) const
{
  if (isFabricated() ^ other.isFabricated())
    return FALSE;

  NABoolean result;
  if (!corrName_.isNull()) {
    if (!other.corrName_.isNull())
      result = corrName_ == other.corrName_;
    else {
      QualifiedName thisQual(corrName_);
      result = thisQual == other.getQualifiedNameObj();
    }
  } else if (!other.corrName_.isNull()) {
      QualifiedName otherQual(other.corrName_);
      result = getQualifiedNameObj() == otherQual;
  } else {
      result = getQualifiedNameObj() == other.getQualifiedNameObj();
  }
  //  // The names must match, and
  //  // the special types must match OR one of them must be "normal" --
  //  // thus in "delete from TABLE (RESOURCE_FORK %s) where rfsection = ?;",
  //  // unbound column "rfsection"'s corrname is initially empty/default/normal
  //  // but it will hereby compare equal to the RETDesc xcnm "rfsection" which
  //  // is marked "special/rfork type".
  //  return result && (getSpecialType() == other.getSpecialType()	||
  // 		        getSpecialType() == NORMAL_TABLE		||
  // 		        NORMAL_TABLE     == other.getSpecialType()
  //		       );
  //
  // The preceding is foolishly, overly rigorous -- it would only ever catch
  // a situation where we had two distinct special types in the same query,
  // or if we were reusing (cached) NATables across queries --
  // neither of which we are currently doing.
  // So, the names must match, and that's all!
  
  return result;
}
// Real comparison operator
//
// Are you adding an extra field to be compared here (like specialType was)?
// Then modify CorrName::isEmpty() and RETDesc.C and TableNameMap.C accordingly
// (emulate the setSpecialType() logic).
//
NABoolean CorrName::operator==(const CorrName& other) const
{
  // if two corrnames have different Partition clauses then they are different.
  // This is needed because we are caching natables for which PARTITION or LOCATION 
  // clause is specified.
  if (getPartnClause() != other.getPartnClause())
    return FALSE;

  return isEqualWithPartnClauseSkipped(other);
}


PartitionClause & PartitionClause::operator=(const PartitionClause & rhs)
{
  if (this==&rhs) return *this ;

  partName_          = rhs.partName_ ;
  beginPartNumber_   = rhs.beginPartNumber_ ;
  endPartNumber_     = rhs.endPartNumber_ ;
  locationName_	     = rhs.locationName_;

  partnNameSpecified_  = rhs.partnNameSpecified_;
  partnNumSpecified_   = rhs.partnNumSpecified_;
  partnRangeSpecified_ = rhs.partnRangeSpecified_;

  return *this ;
}
// -----------------------------------------------------------------------
// Methods for class ExtendedQualName
// -----------------------------------------------------------------------

NABoolean ExtendedQualName::operator==(const ExtendedQualName& other) const
{
  if ((getQualifiedNameObj() != other.getQualifiedNameObj()) ||
      (getPartnClause() != other.getPartnClause()))
  {
    // different name
    return FALSE;
  }
 
  SpecialTableType thisType = getSpecialType();
  SpecialTableType otherType = other.getSpecialType();

  if (thisType == otherType)
  {
    // name & special-type are the same
    return TRUE;
  }
  
  // We reach this point when the only difference between the two objects
  // is in the special-type.
  //
  // The special-type MV_TABLE is used only as a security mechanism.
  // Directly updating Materialized Views if prohibited unless using this
  // special-type (see GenericUpdate::bindNode()), while directly selecting
  // from it is approved even under NORMAL_TABLE special-type. Thus, if the
  // two objects refer to the same name, and the only change is the
  // special-type (MV_TABLE vs. NORMAL_TABLE), they shuold still considered
  // equal. This will lead creation of only one NATable object for the MV
  // object.

  if ((thisType == NORMAL_TABLE && otherType == MV_TABLE) ||
      (thisType == MV_TABLE && otherType == NORMAL_TABLE))
  {
    return TRUE;
  }

  return FALSE;
}

NAString ExtendedQualName::getExtendedQualifiedNameAsString() const
{

  if (hasPartnClause())
  {
    // Preallocate a result buffer that'll be big enough most of the time
    // (so += won't reallocate+copy most of the time).
    NAString result((NASize_T)65, CmpCommon::statementHeap());
    result = getQualifiedNameObj().getQualifiedNameAsString();
    if (!(getPartnClause().getPartitionName().isNull()))
    {
      result += " : PARTITION NAME ";
      result += getPartnClause().getPartitionName();
    }
    else if (getPartnClause().getPartitionNumber() > 0)
    {
      result += " : PARTITION NUMBER ";
      char buf[10];
      sprintf(buf, "%d", getPartnClause().getPartitionNumber());
      result += buf;

      if (getPartnClause().getEndPartitionNumber() > 0)
	{
	  char buf[12];
	  sprintf(buf, ",%d", getPartnClause().getEndPartitionNumber());
	  result += buf;
	}
    }
    else if (!(getPartnClause().getLocationName().isNull()))
    {
      result += " : LOCATION NAME ";
      result += getPartnClause().getLocationName();
    }
    return result ;
  }
  else
    return getQualifiedNameObj().getQualifiedNameAsString();
  
}

// -----------------------------------------------------------------------
// Methods for class ColRefName
// -----------------------------------------------------------------------

NABoolean ColRefName::operator==(const ColRefName& other) const
{
  return (isStar()         == other.isStar() &&
  	  getColName()     == other.getColName() &&
	  getCorrNameObj().isEqualWithPartnClauseSkipped(other.getCorrNameObj()));
}


NABoolean ColRefName::operator!=(const ColRefName& other) const
{
  return NOT operator==(other);
}

NABoolean ColRefName::isQualified() const
{
  return corrName_ != "";
}

const NAString ColRefName::getColRefAsString(NABoolean debug,
					     NABoolean formatForDisplay) const
{
  // Call static method to return "col", "corr.col", or "cat.sch.tbl.col"
  NAString tmp(FORMAT_NO_ASSERT(getColName()), CmpCommon::statementHeap());
  return getColRefAsString(tmp, 
			   corrName_.getExposedNameAsString
			   (debug,
			    formatForDisplay));
}

const NAString ColRefName::getColRefAsAnsiString(NABoolean debug,
						 NABoolean formatForDisplay) const
{
  formatAsAnsiIdentifier = TRUE;
  NAString result(getColRefAsString(debug, formatForDisplay), 
		  CmpCommon::statementHeap());
  formatAsAnsiIdentifier = FALSE;
  return result;
}

const NAString ColRefName::getColNameAsAnsiString() const
{
  NAString tempColHeading(CmpCommon::statementHeap()); 
  tempColHeading = ToAnsiIdentifier(getColName());
  char buff[ComAnsiNamePart::MAX_IDENTIFIER_EXT_LEN];
  char *finalptr = buff;
  const char *initptr = tempColHeading.data();
  size_t len = tempColHeading.length();
  // remove the double double-quotes in case of delimited identifiers.
  // Also change any embedded doubled double quotes to single double quote.
  // This will change a name of the form "a""b"  to    a"b
  size_t i = 0;
  size_t f = 0;
  if (initptr[i] == '"')
    {
      i++;
      while (i < (len - 1))
	{
	  finalptr[f] = initptr[i];
	  if ((initptr[i] == '"') &&
	      (initptr[i+1] == '"'))
	    i++;
	  i++;
	  f++;
	}
      finalptr[f] = '\0';
      tempColHeading = buff;
    } // delimited name

  return tempColHeading;
}

// -----------------------------------------------------------------------
// -- Triggers
// Methods for class TableRefName
// -----------------------------------------------------------------------

// Find the table names in the specified RETDesc.
// Returns TRUE if found.
NABoolean TableRefName::lookupTableName(RETDesc *retDesc)
{
	if (retDesc == NULL)
		return FALSE;

	if (getColumnList() != NULL)    // Have we already found this one?
		return FALSE;
									// Lookup the table name in the RETDesc
	ColumnDescList *columnList = retDesc->getQualColumnList(getTableCorr());
	if (columnList == NULL)
		return FALSE;

	setColumnList(columnList);      // Save the pointer to the ColumnDescList
	return TRUE;                    // One reference less to find.
}

// Add to the RETDesc of the current BindScope new columns, 
// each with the information found by lookupTableName(), but with the refName
// as a correlation name to the table.
void TableRefName::bindRefColumns(BindWA *bindWA) const
{
	if (getColumnList() == NULL)
		return;    // REFERENCING an unused transition name.

	RETDesc *currentRETDesc = bindWA->getCurrentScope()->getRETDesc();
	// Create a new CorrName for the table, with the refName as a correllation
	// name. Since it's the same table for all columns, take the table name
	// from the first column.
	const ColumnDesc *firstCol = getColumnList()->at(0);
	CorrName updatedCorrName(firstCol->getColRefNameObj().getCorrNameObj());
	updatedCorrName.setCorrName(getRefName());

	// Insert this CorrName into the XTNM of the current BindScope.
//	bindWA->getCurrentScope()->getXTNM()->insertNames(bindWA,updatedCorrName);
	
	for (CollIndex i=0; i<getColumnList()->entries(); i++) 
	{
		// For each column of the table
		const ColumnDesc *currentCol = getColumnList()->at(i);			
		// Create a new column ref, with the updated corellation name.
		ColRefName renamedColumn(currentCol->getColRefNameObj().getColName(), 
								 updatedCorrName);

		// Add it to the RETDesc of the current scope.
		currentRETDesc->addColumn(bindWA, 
								  renamedColumn, 
								  currentCol->getValueId(), 
								  USER_COLUMN, 
								  currentCol->getHeading());
	}
}

// -----------------------------------------------------------------------
// Methods for class TableRefList
// -----------------------------------------------------------------------

// Default Ctor, make the LIST allocated it's internal data from the
// statement heap instead of the system heap.
TableRefList::TableRefList(CollHeap *heap) :
	LIST(TableRefName)(heap)
	{}

const TableRefName *TableRefList::findTable(const CorrName& tableCorr) const
{
	for (CollIndex i=0; i<entries(); i++)
	{
		if (at(i).getTableCorr() == tableCorr)
			return &at(i);
	}
	return NULL;
}

// -----------------------------------------------------------------------
// Additional non-inline functions for all ``ObjectNames'' classes
// -----------------------------------------------------------------------

//LCOV_EXCL_START : dpm
// Display/print, for debugging.

void CatalogName::display()   const { print(); }	
void CorrName::display()      const { print(); }	
void ColRefName::display()    const { print(); }	
void ExtendedQualName::display()      const { print(); }	

void CatalogName::print(FILE* ofd, const char* indent, const char* title) const
{
#ifndef NDEBUG
  fprintf(ofd,"catalog=%s%s",
	  getCatalogName().data(),
	  indent);	// just to prevent warnings
  if (strcmp(title,"")) fprintf(ofd,"\n");
#endif
}

void SchemaName::print(FILE* ofd, const char* indent, const char* title) const
{
#ifndef NDEBUG
  fprintf(ofd,"c=%s s=%s%s",
	  getCatalogName().data(),
	  getSchemaName().data(),
	  indent);	// just to prevent warnings
  if (strcmp(title,"")) fprintf(ofd,"\n");
#endif
}

void QualifiedName::print(FILE* ofd, const char* indent, const char* title) const
{
#ifndef NDEBUG
  fprintf(ofd,"c=%s s=%s o=%s%s",
	  getCatalogName().data(),
	  getSchemaName().data(),
	  getObjectName().data(),
	  indent);	// just to prevent warnings
  if (strcmp(title,"")) fprintf(ofd,"\n");
#endif
}

NABoolean QualifiedName::isHistograms() const
{
   return (getObjectName() == HBASE_HIST_NAME);
}

NABoolean QualifiedName::isHistogramIntervals() const
{
   return (getObjectName() == HBASE_HISTINT_NAME);
}


void ExtendedQualName::print(FILE* ofd, const char* indent, const char* title) const
{
#ifndef NDEBUG
  fprintf(ofd,"%s ", getLocationName().data());
  getQualifiedNameObj().print(ofd, indent, "");
  if (strcmp(title,"")) fprintf(ofd,"\n");
#endif
}

void CorrName::print(FILE* ofd, const char* indent, const char* title) const
{
#ifndef NDEBUG
  fprintf(ofd,"%s ", getExposedNameAsString().data());
  if (isFabricated())
    fprintf(ofd,"(%s) ", getExposedNameAsString(TRUE).data());
  getQualifiedNameObj().print(ofd, indent, "");
  if (strcmp(title,"")) fprintf(ofd,"\n");
#endif
}

void ColRefName::print(FILE* ofd, const char* indent, const char* title,
		       NABoolean brief) const
{
#ifndef NDEBUG
  if (strcmp(title, "")) fprintf(ofd,"%s ", title);
  fprintf(ofd,"%s", getColRefAsString().data());
  if (isFabricated())
    fprintf(ofd," (%s)", getColRefAsString(TRUE).data());
  if (brief) return;
  fprintf(ofd," ");
  getCorrNameObj().print(ofd, indent, "");
  if (strcmp(title,"")) fprintf(ofd,"\n");
#endif
}
//LCOV_EXCL_STOP : dpm

// ++MV
ComAnsiNameSpace ExtendedQualName::convSpecialTableTypeToAnsiNameSpace( const SpecialTableType type )
{
  switch (type)
  {
  case  NORMAL_TABLE:
  case  LOAD_TABLE:
  case  ISP_TABLE:
  case  MV_TABLE:
  case  PARTITION_TABLE:
  case  TRIGTEMP_TABLE:
  case  RESOURCE_FORK:
  case  VIRTUAL_TABLE:
  case  MVS_UMD:
    return COM_TABLE_NAME;
  case  INDEX_TABLE:
    return COM_INDEX_NAME;
  case  IUD_LOG_TABLE:
    return COM_IUD_LOG_TABLE_NAME;
  case  RANGE_LOG_TABLE:
    return COM_RANGE_LOG_TABLE_NAME;
  case SCHEMA_SECURITY_TABLE:
    return COM_SCHEMA_LABEL_NAME;
  case  EXCEPTION_TABLE:
    return COM_EXCEPTION_TABLE_NAME;
  case GHOST_TABLE:
  case GHOST_MV_TABLE:
    return COM_GHOST_TABLE_NAME;
  case GHOST_INDEX_TABLE:
    return COM_GHOST_INDEX_NAME;
  case  GHOST_IUD_LOG_TABLE:
    return COM_GHOST_IUD_LOG_TABLE_NAME;
  case SG_TABLE:
    return COM_SEQUENCE_GENERATOR_NAME;
  case LIBRARY_TABLE:   // not really a table, but that is what we call 3 part names
    return COM_LIBRARY_NAME; 
  default:
    return COM_UNKNOWN_NAME;
  }
}

ComAnsiNameSpace ExtendedQualName::getAnsiNameSpace() const
{
  return convSpecialTableTypeToAnsiNameSpace(getSpecialType());
}

// In the display tool, it is usefull to know the special type also.
const NAString ExtendedQualName::getSpecialTypeName() const
{
  NAString result;

  switch (getSpecialType())
  {
    case  MV_TABLE:
      result += "MV";
      break;

    case  TRIGTEMP_TABLE:
      result += "TrigTemp";
      break;
    case  INDEX_TABLE:
      result += "Index";
      break;

    case  IUD_LOG_TABLE:
      result += "IudLog";
      break;

    case  RANGE_LOG_TABLE:
      result += "RangeLog";
      break;

    case  EXCEPTION_TABLE:
      result += "Exception";
      break;

    case  GHOST_TABLE:
      result += "GhostTable";
      break;

    case  GHOST_INDEX_TABLE:
      result += "GhostIndex";
      break;

    case  GHOST_MV_TABLE:
      result += "GhostMV";
      break;

    case  LIBRARY_TABLE:
      result += "Library";
      break;

    default: ;
  }

  return result;
}

const NAString ExtendedQualName::getText() const
{
  NAString result(getQualifiedNameObj().getQualifiedNameAsAnsiString(TRUE), 
                  CmpCommon::statementHeap());
  return result;
}

const NAString ExtendedQualName::getTextWithSpecialType() const
{
  NAString tableType(getSpecialTypeName());

  if (tableType == "")
    return getText();
  else
  {
    NAString tableName(getText());
    tableName += " (";
    tableName += tableType;
    tableName += ")";
    return tableName;
  }
}

// --MV
// For NAKeyLookup

ULng32 hashKey(const QualifiedName& name) { return name.hash(); }
ULng32 hashKey(const ExtendedQualName& name) { return name.hash(); }
ULng32 hashKey(const CorrName&      name) { return name.hash(); }
ULng32 hashKey(const ColRefName&    name) { return name.hash(); }



// For TaskMonitor display
ostream& operator<< (ostream& out, TaskMonitor t)
{
// return the time in microseconds. The unit indicator ("ms") should be "us".
// But we right now we keep it as "ms".

  out.fixed;
  out.precision(6);

  return out<< "Time = " <<
  ((double) t.timer()) / CLOCKS_PER_SEC
  << " us (microsecond)" <<
//#if defined(NA_LINUX)
   //"\tET = " << out.fixed << out.precision(6) << t.elapsed_time() << " s" << 
   "\tET = " << t.elapsed_time() << " s" << 
//#endif
  " \tCounts = "<<t.count()<<" \tGoodCnts = "<<t.goodcount();
}

