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
**************************************************************************
*
* File:         NAString.cpp
* Description:  Utility string functions (basic/exportable).
*               (See NAString2.cpp for more funx.)
* Created:      06/07/94
* Language:     C++
*
*
**************************************************************************
*/

#include <ctype.h>
#include "BaseTypes.h"
// #include "ComAnsiNamePart.h"
#include "ComASSERT.h"
#include "ComMPLoc.h"
#include "ComOperators.h"
#include "ComSmallDefs.h"
#include "str.h"
#include "ComRtUtils.h"
#include "sqlcli.h"
#include "charinfo.h"
#include "csconvert.h"
#include "nawstring.h"

#define   SQLPARSERGLOBALSCMN__INITIALIZE
#define   SQLPARSERGLOBALS_FLAGS
#include "SqlParserGlobals.h"


// Include key word header for IsSqlReservedWord().
#include "ComResWords.h"

// The timing loop in ToAnsiIdentifier below was run
// with this flag (using hashtable to check for reserved keywords)
// and without it (using strstr on one long char array to check).
// The winner is WITHOUT this flag; the strstr approach has marginally
// better average performance than the hashtable, plus it takes only
// half the time on a keyword hit, plus there is no initialization cost.
// Plus it has a smaller data size and smaller code size (smaller executable
// object/less disk space) and less algorithmic complexity.
//
// quantify'ing arkcmp's compilation of tpc-c queries shows the strstr-based
// IsSqlReservedWord taking 4.8% of total arkcmp elapsed time. A binary
// search implementation of IsSqlReservedWord shrinks this down to 0.01%
// of total arkcmp elapsed time.

#include "NAString.h"
#include "ComDistribution.h"

// Space, dquote, percent, etc, as found in Ansi 5.1,
// plus Tdm-extension of backslash.
//
// The character NON_SQL_TEXT_CHAR ('@', from our .h file) must *not*
// appear in this array:  this internal special char is used to guarantee
// a unique parseable name in internally-generated text
// (unique in that it cannot conflict with any externally legal identifier).
//
static const char specialSQL_TEXT[] = " \"%&'()*+,-./:;<=>?[]_|\\";

#include "ReservedInternalNames.cpp"

// -----------------------------------------------------------------------
// The NAString_isoMappingCS memory cache for use by routines
// ToInternalIdentifier() and ToAnsiIdentifier[2|3]() in modules
// w:/common/NAString[2].cpp.  These routines currently cannot
// access SqlParser_ISO_MAPPING directly due to the complex
// build hierarchy.
// -----------------------------------------------------------------------
static THREAD_P SQLCHARSET_CODE NAString_isoMappingCS = SQLCHARSETCODE_UNKNOWN;

// -----------------------------------------------------------------------
Lng32 NAString_getIsoMapCS()
{
  if (NAString_isoMappingCS != SQLCHARSETCODE_UNKNOWN)
    return (Lng32)NAString_isoMappingCS;
  NAString_isoMappingCS = (SQLCHARSET_CODE)ComRtGetIsoMappingEnum();
  return (Lng32)NAString_isoMappingCS;
}

// -----------------------------------------------------------------------
void NAString_setIsoMapCS(Lng32 isoMappingCS)
{
  // ComASSERT(isoMappingCS == (Lng32)SQLCHARSETCODE_ISO88591 ||
  //           isoMappingCS == (Lng32)SQLCHARSETCODE_SJIS     ||
  //           isoMappingCS == (Lng32)SQLCHARSETCODE_UTF8);
  NAString_isoMappingCS = (SQLCHARSET_CODE)isoMappingCS;
}

// -----------------------------------------------------------------------
NABoolean isUpperIsoMapCS(unsigned char c)
{
  {
    return isUpper8859_1((NAWchar)c);
  }
  return FALSE; // dead code
}

// -----------------------------------------------------------------------
NABoolean isAlphaIsoMapCS(unsigned char c)
{
  {
    return isAlpha8859_1((NAWchar)c);
  }
  return FALSE; // dead code
}

// -----------------------------------------------------------------------
NABoolean isAlNumIsoMapCS(unsigned char c)
{
  {
    return isAlNum8859_1((NAWchar)c);
  }
  return FALSE; // dead code
}

// -----------------------------------------------------------------------
void NAStringUpshiftIsoMapCS(NAString &ns)
{
  {
    ns.toUpper8859_1();
  }
}

// -----------------------------------------------------------------------
// convertNAString()
// Note that this allocates memory.
// -----------------------------------------------------------------------
char *convertNAString(const NAString& ns, CollHeap *heap, NABoolean wideNull)
{
  size_t len = ns.length();
  size_t nullSpaceLen = 0;
  char* buf;

  if (wideNull == TRUE)
     nullSpaceLen = sizeof(NAWchar);
  else
     nullSpaceLen = 1;

  if (heap)
    buf = new (heap) char[len + nullSpaceLen];
  else {
    buf = new char[len + nullSpaceLen];
    #ifndef NDEBUG
      cerr << "Possible memory leak: convertNAString called with NULL heap\n";
    #endif
  }
  str_cpy_all(buf, ns.data(), len);
  if (wideNull == TRUE)
    ((NAWchar *)buf)[len / sizeof(NAWchar)] = L'\0';
  else
    buf[len] = '\0';
  return buf;
}

// -----------------------------------------------------------------------
// Returns TRUE if the string consists entirely of whitespace
// (at least one space or tab, and nothing else),
// FALSE if string is empty (null) or contains a non-white character.
// -----------------------------------
NABoolean IsNAStringSpace(const NAString& ns)
{
  if (ns.isNull())
    return FALSE;
  return IsNAStringSpaceOrEmpty(ns);
}

// -----------------------------------
// Returns TRUE if the string consists entirely of whitespace
// (zero or more spaces or tabs, and nothing else), including none (empty str).
// -----------------------------------------------------------------------
NABoolean IsNAStringSpaceOrEmpty(const NAString& ns)
{
  StringPos len = ns.length();
  for (StringPos i = 0; i < len; i++)
    if (!isSpace8859_1((unsigned char)ns[i]))
      return FALSE;
  return TRUE;
}

// -----------------------------------------------------------------------
// Returns TRUE if the string contains only 7-bit ASCII characters or
// if the string is empty.
// -----------------------------------------------------------------------
NABoolean NAStringHasOnly7BitAsciiChars(const NAString& ns)
{
  StringPos len = ns.length();
  for (StringPos i = 0; i < len; i++)
    if ( ((unsigned char)ns[i]) > 127 )
      return FALSE;
  return TRUE;
}
// -----------------------------------------------------------------------
// Returns TRUE if the string contains only 7-bit ASCII characters
// between '0' and '9'  OR  if the string is empty.
// -----------------------------------------------------------------------
NABoolean NAStringHasOnlyDecimalDigitAsciiChars(const NAString& ns)
{
  StringPos len = ns.length();
  for (StringPos i = 0; i < len; i++)
    if ( ((unsigned char)ns[i]) < '0' OR
         ((unsigned char)ns[i]) > '9' )
      return FALSE;
  return TRUE;
}

// -----------------------------------------------------------------------
// upshift a string (no funny locale stuff, just do it)
// -----------------------------------------------------------------------
void NAStringUpshiftASCII(NAString& ns)
{
  ns.toUpper();
}

// -----------------------------------------------------------------------
// decode a number from a prefix of an NAString
// -----------------------------------------------------------------------
Lng32 NAStringToLong(const NAString &ns)
{
  Lng32 result;
  sscanf(ns.data(),"%d",&result);
  return result;
}

double NAStringToReal(const NAString &ns)
{
  float result;
  sscanf(ns.data(),"%g",&result);
  return result;
}


NAString LongToNAString(Lng32 l)
{
  char resultstr[100];
  sprintf(resultstr,"%d",l);
  return NAString(resultstr);
}

NAString UnsignedToNAString(UInt32 u)
{
  char resultstr[100];
  sprintf(resultstr,"%u",u);
  return NAString(resultstr);
}

NAString Int64ToNAString(Int64 l)
{
  char resultstr[100];
  convertInt64ToAscii(l, resultstr);
  return NAString(resultstr);
}

NAString RealToNAString(double d)
{
  char resultstr[200];
  sprintf(resultstr,"%G",d);
  return NAString(resultstr);
}

NAString &replaceAll(NAString &source, const NAString &searchFor,
                     const NAString &replaceWith)
{
  size_t indexOfReplace = NA_NPOS;
  indexOfReplace = source.index(searchFor);
  if (indexOfReplace != NA_NPOS)
    {
      // Replace all occurences of searchFor with replaceWith. When no
      // more occurences are found or end of string is reached, index()
      // will return NA_NPOS.
      while (indexOfReplace != NA_NPOS)
        {
          source.replace(indexOfReplace, searchFor.length(), 
                         replaceWith);
          // Find index of next occurence to replace.
          indexOfReplace = 
            source.index(searchFor, indexOfReplace + replaceWith.length()); 
        }
    }

  return source;
}

// ---------------------------------------------------------------------
// Hash function for NAString types in NAKeyLookup
// ---------------------------------------------------------------------
ULng32 hashKey(const NAString& str)
{
  return str.hash();
}

// ---------------------------------------------------------------------
// Look up names that start with a '$' or an '=' sign
//
// Right now, DEFINEs are simulated by environment variables
// (that might be useful for an OSS process on NSK as well)
// ---------------------------------------------------------------------
NAString LookupDefineName(const NAString &ns, NABoolean iterate)
{
  const Int32 itermax	= 100;		// detect self-referencing env vars
  Int32 iterlimit		= iterate ? itermax : 1;
  Int32 iterations	= 0;

  NAString delimIdent;
  const char *defineName = NULL;
  const char *mappedName = ns.data();

  // If the name is like $"abc", convert it to $abc and then do the lookup.
  if ((mappedName[0] == '$' OR mappedName[0] == '=') AND
      mappedName[1] == '"')
    {
      delimIdent = &mappedName[1];
      if (!ToInternalIdentifier(delimIdent, FALSE))
        {
	  delimIdent.prepend(mappedName[0]);
	  mappedName = delimIdent.data();
	}
    }

  while (mappedName AND
	 (mappedName[0] == '$' OR mappedName[0] == '=') AND
	 iterations++ < iterlimit)
    {
      defineName = mappedName;
      mappedName = getenv(&mappedName[1]);
    }

  // could raise an exception if iterations >= itermax

  if (mappedName)
    return NAString(mappedName);
  else
    // couldn't map name, return unresolved name
    return NAString(defineName);
}

// ---------------------------------------------------------------------
// Convert a NAString member of a QualifiedName, CorrName, or ColRefName
// from the canonical internal format required by Binder
// into the external delimited-identifier ANSI format.
// That is,
//	A2C		returns as	A2C
//	a2c				"a2c"
//	12C				"12C"
//	A+C				"A+C"
//	A"C"				"A""C"
//
// The required internal format is achieved by Parser (et alia) having
// previously called the companion function below, ToInternalIdentifier.
// ---------------------------------------------------------------------

// look up sqlText in the ReservedWords table; return TRUE iff id is
// an ANSI, PotentialANSI, or Tandem reserved word.
NABoolean IsSqlReservedWord(const char *sqlText)
{
  return ComResWords::isSqlReservedWord(sqlText,0);
}

NABoolean IsCIdentifier(const char *id)
{
  // trim whitespace first, if necessary
  // Note that we allow identifiers starting with an underscore
  for (size_t i=0; id[i] != 0; i++)
    {
      char c = id[i];

      if (!(c >= 'A' && c <= 'Z' ||
            c >= 'a' && c <= 'z' ||
            c == '_' ||
            c >= '0' && c <= '9' && i > 0))
        return FALSE;
    }
  return TRUE;
}

NABoolean /*NAString::*/setMPLoc()
{
  if (!SqlParser_Initialized() )
    return TRUE;
  else
    return FALSE;
}


NAString ToAnsiIdentifier(const NAString &ns, NABoolean assertShort)
{
  size_t nsLen = ns.length();

  // Zero-length INTERNAL identifiers are fabricated by Parser and Binder;
  // they're okay (it's only zero-length EXTERNAL ones that're illegal).
  if (nsLen == 0)
    return NAString();

  // Assert various checks were previously done when converting the original
  // external identifier (in some previous call to ToInternalIdentifier).
  const Int32 SMAX=2048;
  NAWString internalFormatNameInUCS2;
  ComAnsiNameToUCS2 ( ns // in  - const ComString & internalFormatName
                    , internalFormatNameInUCS2 // out - NAWString &
                    );
  if ((Int32) internalFormatNameInUCS2.length() >
      (Int32) (assertShort ? ComMAX_1_PART_INTERNAL_UCS2_NAME_LEN_IN_NAWCHARS : SMAX))
  {
    ComASSERT(0);
    return NAString();
  }

  char buf[SMAX];
  size_t len;

  ToAnsiIdentifier3(ns.data(), ns.length(), buf, SMAX, &len, NAString_getIsoMapCS());
  if (len == 0)
    return NAString();
  else
    {
      const NAString &nas = NAString(buf, len);
      return nas;
    }
}

// ---------------------------------------------------------------------
// Helper function for ToInternalIdentifier:
// put the integer count of characters scanned into the first character
// of the return string.  (We know the count will fit into a char, and we
// know the string does have a first char (is nonempty), so this is safe.)
// SqlParser uses this info for pretty syntax error messaging.
// ---------------------------------------------------------------------
static Lng32 illegalCharInIdentifier(NAString &ansiIdent,
				    size_t i, size_t countOfRemoved)
{
  ansiIdent[(size_t)0] = i + countOfRemoved;
  return -3127;
}

// ---------------------------------------------------------------------
// The inverse of ToAnsiIdentifier -- but note that this function is the one
// called first, and does some essential checking that the above relies on.
//
// The purpose of this function is to convert NAStrings containing
// Ansi-format regular or delimited identifiers to our internal format
// required by Binder (RETDesc) lookups, which is the same format as
// in the catalog metadata tables.
//
// Leading blanks are removed, and then,
// if the string begins with a double quote, then this function does:
//    - there are supposed to be double quotes surrounding the string
//      and they are removed
//    - any embedded double quotes (i.e., two consecutive dquotes)
//	are turned into just one dquote
//    - silently change tabs to spaces, just as a courtesy
//	(officially by ANSI, tabs are illegal even in delimited identifiers
//	because they are not a character in the SQL_TEXT default character set
//	specification)
//    - allow all characters to pass in delimited identifiers
//      except the @ prefix as it being used internally by the compiler
//      to generate unique table names for internal use. Please look at the
//      contents of file w:/sqlshare/ReservedInternalNames.cpp for other
//      prefix strings with embedded @ (e.g., OLD@) that are reserved for
//      internal use. @ is allowed in delimited identifiers otherwise.
//    - We now accept ^ in delimited identifiers if it is not a prefix.
//      We used to disallow ^ unless acceptCircumflex is true.
//
// If the string does not begin with a double quote, then
//    - remove trailing blanks (spaces AND tabs)
//    - verify there are no illegal characters for a REGULAR identifier
//    - uppercase the contents unless flagged not to
//    - ensure that no regular identifier matches an Ansi reserved word
//
// Return value is a SqlCode value (of a message with no parameters) if error,
// and zero (0) if no error.
// It is caller's job to insert any error condition into a ComDiags.
//
// Efficiency: this function saves on space at the cost of some time.
// The calls to RWCString.remove() probably take linear time as a function
// of string length on each call.  A faster version of this function
// would establish a transformed string in a separate buffer and then
// copy it back into the original.
//
// $$$ Kludge NLS (National Language Support) 7-APR-2007 $$$ We used
// to not accept the 7-bit ASCII characters @, /, ^, and \, in ANSI SQL
// delimited identifiers specified by customers, but we do allow
// many other 8-bit byte values between the two double quotes;
// our Japanese customers take advantage of this lack of restriction
// and put their Japanese multibyte characters in delimited identifiers.
// The MXCMP program treats the indentifier as if it contains a string
// of ISO 8859-1 characters.
//      Note that currently, the target columns in the metadata
//      tables containing internally-formated identifier has the
//      CHAR(128) CHARACTER SET ISO88591 data type.
// This kludge workaround works for most case, but there are about
// 5-10% of the Japanese Shift-JIS characters rejected by MXCMP because
// the lower byte of their two-byte multibyte characters contains binary
// value equivalent to the representation of 7-bit ASCII characters @, ^,
// or \.
//
// These restrictions have been lifted. \, @, and ^ now can appear
// within delimited identifiers.  $, @, and ^ reserved for internal
// use when they are a prefix. A few other prefixes with @ embedded
// are reserved for internal use also.
//
// We now allow the forward slash ( i.e., / ) character to appear
// within a delimited identifier.  Character / is guaranteed to be a
// standalone one-byte character in Shift-JIS and EUC-JP and any
// other character sets that is the supersets of the 7-bit ASCII
// character set (e.g., UTF-8).
// ---------------------------------------------------------------------
Lng32 ToInternalIdentifier( NAString &ansiIdent
			 , Int32 upCase
			 , NABoolean acceptCircumflex   // VO: Fix genesis solution 10-040204-2957
                         , UInt16 pv_flags      // call-by-value parameter (pv_)flags
                         )
{
  size_t i;				// unsigned: beware "i--" when (i==0)!

  // Remove leading blanks (spaces AND tabs).
  // SqlLexer/Parser do not pass in leading blanks, but we cannot trust
  // the SchemaDB caller, nor the Catman-constructed-on-the-fly names
  // of ComAnsiNamePart.
  //
  // Lines [RW] fix a RogueWave memory leak in RWCString::operator[].
  //
  const char *sptr = ansiIdent.data();			// [RW] added this line
  size_t len = ansiIdent.length();
  for (i = 0; i < len; i++)
    if (isSpace8859_1((unsigned char)*sptr))		// [RW]
      sptr++;
    else
      break;
  if (i) {
    ansiIdent.remove(0,i);
    len = ansiIdent.length();
  }

  if (len == 0)
    return -3004;		// An ident must contain at least one character

  size_t countOfRemoved = i;
  i = 0;
  // Handle double quotes or backquotes as delimited identifiers.
  // Backquotes are used for hive objects.
  // An error will be returned later if they are used for traf objects.
  NABoolean isDquote = (ansiIdent[i] == '"');
  if ((ansiIdent[i] != '"') && 
      (ansiIdent[i] != '`')) {	// REGULAR identifier

    // ANSI 5.2 SR 13 + 14 and 8.2 SR 3a say that trailing spaces are
    // insignificant in equality-testing of identifiers, so remove them
    // (and tabs as well, as a courtesy).
    //
    // This loop transforms 'ABC  ' into 'ABC' (NOT the same as delimited loop!)
    // We also know it won't be empty, thanks to the check above.
    //
    for (i = len; i > 0; ) {
      i--;
      if (!isSpace8859_1((unsigned char)ansiIdent[i]))
	break;
    }
    if (++i < len) {
      ansiIdent.remove(i);
      len = ansiIdent.length();
    }

    // ComASSERT(ComGetNameInterfaceCharSet() == SQLCHARSETCODE_UTF8);
    NABoolean has7BitAsciiCharsOnly = NAStringHasOnly7BitAsciiChars(ansiIdent);
    NABoolean isLatin1 = FALSE;
    const Int32 SMAX = 2048;
    char latin1Buf[SMAX+1];
    char *pFirstUntranslatedChar = NULL;
    if (NOT IsNAStringSpaceOrEmpty(ansiIdent) AND NOT has7BitAsciiCharsOnly)
    {
      // Check to see if ansiIdent contains English and Western European characters only (including
      // those in the upper half of the ISO88591 character set).  Note that ansiIdent contains
      // UTF-8 encoding values.
      UInt32 outLen = 0;
      UInt32 translatedCharCount = 0;
      Int32 retCode = UTF8ToLocale ( cnv_version1
                                   , (const char*) ansiIdent.data()
                                   , (const Int32) ansiIdent.length()
                                   , (const char*) latin1Buf 
                                   , (const Int32) SMAX+1
                                   , (cnv_charset) cnv_ISO88591
                                   , (char* &)     pFirstUntranslatedChar
                                   , (UInt32 *)    &outLen // unsigned int *output_data_len_p 
                                   , (const Int32) TRUE    // const int addNullAtEnd_flag
                                   , (const Int32) FALSE   // const int allow_invalids 
                                   , (UInt32 *)    &translatedCharCount // unsigned int * translated_char_cnt_p
                                   , (const char*) NULL    // const char *substitution_char_p
                                   );
      if (retCode == 0) // success - i.e., ansiIdent contains characters that can be support by ISO 8859-1
      {
        isLatin1 = TRUE;
        len = translatedCharCount;
      }
    }
    if (NOT has7BitAsciiCharsOnly AND NOT isLatin1)
    {
      // Regular identifiers can contains characters supported by ISO 8859-1 standard only.
      size_t pos = 0;
      if (pFirstUntranslatedChar != NULL AND (pFirstUntranslatedChar - ansiIdent.data()) > 0)
        pos = pFirstUntranslatedChar - ansiIdent.data();
      return illegalCharInIdentifier(ansiIdent, pos, countOfRemoved);
    }

    // First character of a regular identifier must be alpha
    // (or '\' or '$' if NSK name).
    // (User-input names under ANSI or SHORTANSI do not allow NSK format.)
    //
    i = 0;
    unsigned char c = (unsigned char)ansiIdent[i];
    if (isLatin1)
      c = (unsigned char)latin1Buf[i];

    if (isAlphaIsoMapCS(c) ||
        (c == '$' && ((pv_flags & NASTRING_REGULAR_IDENT_WITH_DOLLAR_PREFIX) != 0))) {

      // Subsequent characters must be alphanumeric or the underscore.
      //
      while (++i < len) {
        if (isLatin1)
          c = (unsigned char)latin1Buf[i];
        else
        c = (unsigned char)ansiIdent[i];
        if (NOT isAlNumIsoMapCS(c) && c != '_') {
          return illegalCharInIdentifier(ansiIdent, i, countOfRemoved);
        }
      }

      if (upCase) {
        if (isLatin1)
        {
          NAString ns = latin1Buf;
          ns.toUpper8859_1();
          memcpy(latin1Buf, ns.data(), ns.length()+1);
        }
        else
        NAStringUpshiftIsoMapCS(ansiIdent);
      }

      // Reserved words cannot be regular identifiers
      //
      if (IsSqlReservedWord(ansiIdent))
      {
        return -3128;
      }
    } else if ((c == '\\') ||
               (c == '$' && ((pv_flags & NASTRING_ALLOW_NSK_GUARDIAN_NAME_FORMAT) != 0)
                         && ((pv_flags & NASTRING_DELIM_IDENT_WITH_DOLLAR_PREFIX) == 0))) {
      // ComASSERT(NAStringHasOnly7BitAsciiChars(ansiIdent) AND NOT isLatin1);
 // For now, allow Guardian style names in ANSI mode as well.  This was the
 // old behavior since this method was not being called for Guardian names.
 // The MX Reference manual is also a bit ambiguous in this regard.
 // Need to get a resolution on this issue soon. RMW 11-20-2000.
 //
 // } else if ((c == '\\' || c == '$') &&
 //            (!SqlParser_Initialized() || SqlParser_NAMETYPE == DF_NSK)) {
      //
      // User can enter \ or $ at beginning of ident *only* if NAMETYPE NSK;
      // if SHORTANSI, their input is Ansi only (no \ or $),
      // and it is only the *output*, after SHORTANSI name resolution,
      // that may contain \ and $.
      //
      //## We should really call the left-to-right ComMPLoc ctor here,
      //## allowing only a valid MP format
      //## (one of ComMPLoc::SYS, VOL, or FILE).
      //

      // Allow the patterns:
      //  \[a-zA-Z][a-zA-Z0-9]*.$[a-zA-Z][a-zA-Z0-9]*
      //  $[a-zA-Z][a-zA-Z0-9]*
      //
      i++;
      if (c == '\\') {

        // Must start with an ascii char.
        //
        if((i >= len) || (NOT isAlphaIsoMapCS((unsigned char)ansiIdent[i++]))) {
          return illegalCharInIdentifier(ansiIdent, i - 1, countOfRemoved);
        }

        while ((i < len) && isAlNumIsoMapCS((unsigned char)ansiIdent[i])) {
          i++;
        }

        // Expecting a ".$"
        //
        if((i >= len) || ((unsigned char)ansiIdent[i++] != '.')) {
          return illegalCharInIdentifier(ansiIdent, i - 1, countOfRemoved);
        }

        if((i >= len) || ((unsigned char)ansiIdent[i++] != '$')) {
          return illegalCharInIdentifier(ansiIdent, i - 1, countOfRemoved);
        }
      }

      // After the '$'
      //
      while (i < len) {
        if (NOT isAlNumIsoMapCS((unsigned char)ansiIdent[i++])) {
          return illegalCharInIdentifier(ansiIdent, i - 1, countOfRemoved);
        }
      }

      // Note that for NSK style names, it is not necessary to call
      // IsSqlReservedWord() since there are no reserved identifiers
      // starting with \ or $.
      //
      if (upCase) {
        NAStringUpshiftIsoMapCS(ansiIdent);
      }

    } else {
      // Invalid first character.
      //
      return illegalCharInIdentifier(ansiIdent, i, countOfRemoved);
    }

    if (isLatin1)
    {
      char utf8Buf[SMAX+1];
      char * p1stUnstranslatedChar = NULL;
      UInt32 utf8StrLenInBytes = 0;
      UInt32 charCount = 0;
      Int32 returnCode = LocaleToUTF8(cnv_version1
                                      , (const char*) latin1Buf
                                      , (const Int32) len
                                      , (const char*) utf8Buf
                                      , (const Int32) SMAX+1
                                      , cnv_ISO88591
                                      , p1stUnstranslatedChar // char * &       first_untranslated_char
                                      , &utf8StrLenInBytes    // unsigned int * output_data_len_p
                                      , (const Int32)TRUE     // const int      addNullAtEnd_flag
                                      , &charCount            // unsigned int * translated_char_cnt_p
                                      );
      // Note that utf8StrLenInBytes includes the NULL terminator added at the end
      // (addNullAtEnd_flag was set to TRUE in the above call).
      // ComASSERT(returnCode == 0);
      ansiIdent = utf8Buf;
    }

  } // end REGULAR identifier
  else {

    UInt32 state = (isDquote ? 1 : 3);
    ansiIdent.remove(0,1);         // remove initial dquote
    countOfRemoved++;

    const char *sptr = ansiIdent.data();

    len = ansiIdent.length();
    if (len <= 1)     // A delimited ident must contain at least one character
        return -3004; // plus an ending double-quote.
    i = 0;
    unsigned char c = (unsigned char)ansiIdent[i];

    // Kludge NLS Notes:  When \ character is the first character in a
    //                    Japanese multibyte identifier, it is really
    //                    the standalone one-byte character and is not
    //                    the lower byte of a multibyte character.  In
    //                    Shift-JIS character set, the \ backslash is
    //                    actually displayed as and representing the
    //                    Japanese Yen money unit symbol.
    //
    //                    The $ character is guaranteed to be the
    //                    one-byte standalone character in Shift-JIS
    //                    and all other character sets that are
    //                    supersets of the 7-bit ASCII character set.

    // "\SYS.$VOL" -- special handling because '\' and '$' are not Ansi-special

    if ((c == '\\') ||
        (c == '$' && ((pv_flags & NASTRING_ALLOW_NSK_GUARDIAN_NAME_FORMAT) != 0)
                  && ((pv_flags & NASTRING_DELIM_IDENT_WITH_DOLLAR_PREFIX) == 0))) {
 // For now, allow Guardian stlye names in ANSI mode as well.  This was the
 // old behavior since this method was not being called for Guardian names.
 // The MX Reference manual is also a bit ambiguous in this regard.
 // Need to get a resolution on this issue soon. RMW 11-20-2000.
 //
 //    if ((c == '\\' || c == '$') &&
 //        (!SqlParser_Initialized() || SqlParser_NAMETYPE == DF_NSK)) {
      //
      // User can enter \ or $ at beginning of ident *only* if NAMETYPE NSK;
      // if SHORTANSI, their input is Ansi only (no \ or $),
      // and it is only the *output*, after SHORTANSI name resolution,
      // that may contain \ and $.
      //
      //## We should really call the left-to-right ComMPLoc ctor here,
      //## allowing only a valid MP format
      //## (one of ComMPLoc::SYS, VOL, or FILE).
      //

      // Allow the patterns:
      //  \[A-Z][A-Z0-9]*.$[A-Z][A-Z0-9]*
      //  $[A-Z][A-Z0-9]*
      //
      i++;
      if (c == '\\') {

        // Must start with an ascii char.
        //
        if((i >= len) || (NOT isUpperIsoMapCS((unsigned char)ansiIdent[i++]))) {
          return illegalCharInIdentifier(ansiIdent, i - 1, countOfRemoved);
        }

        while ((i < len) &&
               (isUpperIsoMapCS((unsigned char)ansiIdent[i]) ||
                isDigit8859_1((unsigned char)ansiIdent[i]))) {
          i++;
        }

        // Expecting a ".$"
        //
        if((i >= len) || ((unsigned char)ansiIdent[i++] != '.')) {
          return illegalCharInIdentifier(ansiIdent, i - 1, countOfRemoved);
        }

        if((i >= len) || ((unsigned char)ansiIdent[i++] != '$')) {
          return illegalCharInIdentifier(ansiIdent, i - 1, countOfRemoved);
        }
      }

      // After the '$'
      //
      while ((i < len) &&
             (isUpperIsoMapCS((unsigned char)ansiIdent[i]) ||
              isDigit8859_1((unsigned char)ansiIdent[i]))) {
        i++;
      }

      // Expecting a '"' character in the last position.
      //
      if (((unsigned char)ansiIdent[i] != '"') || (i != len - 1)) {
        return illegalCharInIdentifier(ansiIdent, i, countOfRemoved);
      }

      ansiIdent.remove(i, 1);
      countOfRemoved++;

    } else
    {
      //## [RW memleak -- should replace ansiIdent[i] with sptr references,
      //## refreshing sptr after every remove() or other length modification..]
      while (i < ansiIdent.length()) {

        unsigned char c = (unsigned char)ansiIdent[i];

        if (i == 0) { // first character
          if ( ( c == '^' AND NOT acceptCircumflex )
               // ---- Allow $ to appear in delimited identifers to support routine action names.
               // OR ( c == '$' AND ((pv_flags & NASTRING_DELIM_IDENT_WITH_DOLLAR_PREFIX) == 0) )
               )
            return illegalCharInIdentifier(ansiIdent, i, countOfRemoved);
          if ( NOT Get_SqlParser_Flags(ALLOW_FUNNY_IDENTIFIER) AND
               isDelimitedIdentifierReservedForInternalUse(ansiIdent.data(),
                                                           ansiIdent.length()) ) {
            for ( ; i <= ansiIdent.length(); i++ ) { // look for the first '@'
              if ( ansiIdent[i] == NON_SQL_TEXT_CHAR )
                break;
            } // for
            return illegalCharInIdentifier(ansiIdent, i, countOfRemoved);
          } // if funny identifier not allowed and specified name is funny
        } // if is first character in name

        //       Notes:       The following logic will not mess up our
        //                    Japanese customer's ANSI SQL names.  All 32
        //                    control characters, i.e., single-byte values
        //                    ranges from 0x00 through 0x1F inclusively, are
        //                    standalone characters in Shift-JIS character set
        //                    and any character sets that are supersets of the
        //                    7-bit ASCII character set.  The Tab character is
        //                    a control character.  Just keep the current
        //                    behavior unless our Japanese customers complain
        //                    about this "tab to space" conversion.
        //
        if (isSpace8859_1(c) && c != ' ') {
          ansiIdent[i] = ' ';			// tab becomes space
          c = ' ';				// tab is now  space
        }

        if (NOT isAlNumIsoMapCS(c)) {
            //
            //        Notes:       '/' is guaranteed to always be a
            //                     single-byte standalone character
            //                     in any multibyte character sets
            //                     so it is okay to disallow it; our
            //                     Japanese customer will not complain.
            //
            // JC: Fix genesis solution 10-040304-3817
            // Don't allow '/' in a delimited identifier
            //
            // ### SAP POC ### 11/21/2008 ### BEGIN
            // We now accept the forward slash in delimited names as
            // required by SAP POC. Comment out the following 3 lines of code.
            //
            // ### if (c == '/') {
            // ###   return illegalCharInIdentifier(ansiIdent, i, countOfRemoved);
            // ### }
            //
            // ### SAP POC ### 11/21/2008 ### END
            //
            //       Notes:       The restriction of @ and \ in
            //                    delimited names has been loosen.
            //
        }   // if (NOT isAlNumIsoMapCS(c))

        switch (state) {
	case 1:
          if (c == '"') {
            ansiIdent.remove(i,1);
            countOfRemoved++;
            state = 2;
          } else
            i++;
          break;
	case 2:
          if (c == '"')
            state = 1;
          else if (c != ' ')		// tab became space
            return illegalCharInIdentifier(ansiIdent, i, countOfRemoved);
          i++;
          break;
	case 3:
          if (c == '`') {
            ansiIdent.remove(i,1);
            countOfRemoved++;
            state = 4;
          } else
            i++;
          break;
	case 4:
          if (c == '`')
            state = 3;
          else if (c != ' ')		// tab became space
            return illegalCharInIdentifier(ansiIdent, i, countOfRemoved);
          i++;
          break;
	default:
          ComASSERT(FALSE); 
        }		 // switch
      }		 // while

      if ((isDquote && (state != 2)) ||
          (NOT isDquote && (state != 4)))
        return illegalCharInIdentifier(ansiIdent, i, countOfRemoved);

      // ANSI 5.2 SR 13 + 14 and 8.2 SR 3a say that trailing spaces
      // are insignificant in equality-testing of identifiers, so
      // remove them.  NB: length() and resize(i) have i one greater
      // than operator[i] positions.
      //
      // This loop transforms '" ABC  "  ' into ' ABC'.
      // We must check that   '"      "  ' is rejected as an empty string. //"
      //
      NABoolean empty = TRUE;
      for (i = ansiIdent.length(); i > 0; ) {
        --i;
        if (ansiIdent[i] != ' ') {			// tab became space
          empty = FALSE;
          break;
        }
      }
      if (empty)
        return -3004;  // A delimited ident must contain at least one character
      ansiIdent.resize(++i);
    }

  } // end DELIMITED identifier

  if (!Get_SqlParser_Flags(ALLOW_FUNNY_IDENTIFIER))
  {
    if (ansiIdent.length() > ComMAX_1_PART_INTERNAL_UTF8_NAME_LEN_IN_BYTES)
      return -3118;	// Identifier too long.

    // allocate plenty of room to avoid buffer overrun
    NAWchar internalNameInUCS2[ComMAX_1_PART_INTERNAL_UTF8_NAME_LEN_IN_BYTES + 1 + 16];
    internalNameInUCS2[0] = NAWCHR('\0');
    Int32 iErrorCode =
      ComAnsiNameToUCS2 ( (const char *) ansiIdent.data()   // in  - const char *
                        , (NAWchar *) internalNameInUCS2    // out - NAWchar * outBuf
                        , (Int32) (ComMAX_1_PART_INTERNAL_UTF8_NAME_LEN_IN_BYTES + 1 + 8) // in - outBufSizeInNAWchars
                        , FALSE // do not fill the remainder of the output buffer with spaces
                        );
    if (iErrorCode != 0 || NAWstrlen(internalNameInUCS2) == 0)
      return -13001; // An internal error occurred.  The SQL statement could not be translated.
    if (NAWstrlen(internalNameInUCS2) > ComMAX_1_PART_INTERNAL_UCS2_NAME_LEN_IN_NAWCHARS)
      return -3118;  // Identifier too long.
  }

  return 0;		// no error

} // ToInternalIdentifier

// ---------------------------------------------------------------------
// Converted the external-format (quoted) string literal used by the
// user to to the internal-format string used by the user.  This routine
// assumes that the syntax of the input external-format string literal
// is already valid so it does not perform any checking
// ---------------------------------------------------------------------
#if 0 /* Needed for possible future enhancement -- see caller in CatRoutinePassThroughParamList.cpp */
void ToInternalString(NAString &internalStr, const NAString &quotedStr)
{
  const char *extStr = quotedStr.data();
  ComASSERT(strlen(extStr) >= 2 AND
            extStr[0] EQU '\'' AND
            extStr[strlen(extStr) - 1] EQU '\'');
  internalStr = "";
  if (strlen(extStr) EQU 2) return;
  for (StringPos i = 1, j = 0; i < strlen(extStr) - 1; i++, j++)
  {
    internalStr[j] = extStr[i];
    if (internalStr[j] EQU '\'')
    {
      i++;
      ComASSERT(extStr[i] EQU '\'');
    }
  }
}
#endif

// ---------------------------------------------------------------------
// Converted the internal-format string literal used by the parser to
// the external-format (quoted) string used by the user.  This routine
// assumes that the syntax of the input internal-format string is
// already valid so it does not perform any checking.  The default behavior
// is to turn each single-quote (') into a double single-quote ('') and enclose
// the entire string in single quotes ('....').  Pass in FALSE as the third
// parameter to duplicate existing single quotes without enclosing the entire
// string in single quotes.
// ---------------------------------------------------------------------
void ToQuotedString( NAString &quotedStr
                   , const NAString &internalStr
                   , NABoolean encloseInQuotes )
{
  if (encloseInQuotes) quotedStr = '\'';

  for (StringPos i = 0; i < internalStr.length(); i++)
  {
    quotedStr += internalStr[i];
    if (internalStr[i] EQU '\'') quotedStr += '\'';
  }
  if (encloseInQuotes) quotedStr += '\'';
}

// ---------------------------------------------------------------------
// bsearchStrcmp() is used by bsearch() within tokIsFuncOrParenKeyword()
// to compare two strings.
static Int32 bsearchStrcmp(const void *s1, const void *s2)
{
  return (strcmp((char*)s1, *((char**)s2)));
}

// Used by PrettifySqlText() -- depends on its having upcased unquoted tokens.
static NABoolean tokIsFuncOrParenKeyword(const NAString &sqlText,
					 size_t pos, size_t prevpos)
{
  NAString tok(" ");				// space in front
  tok += &sqlText.data()[pos];
  ComASSERT(tok[tok.length()-1] == ' ');	// and space after

  if (tok == " AND " || tok == " OR ")
    return FALSE;

  // Derived table correlation names are not keywords, but we want to treat
  // them like a paren-keyword (no space between word and lparen):
  //	SELECT COUNT(*) FROM (SELECT ...) corr(colRename,...)
  //	SELECT COUNT(*) FROM (SELECT ...) AS corr(colRename,...)
  // But we want a space in this context:
  //	CREATE VIEW vw(col) AS (SELECT ...);
  if (pos >= 2 && sqlText[pos - 1] == ' ')
    {
      if (sqlText[pos - 2] == ')' && tok != " FROM " && tok != " AS ")
        return TRUE;
      if (pos >= 5)
	if (sqlText[pos - 5] == ')' &&
	    sqlText[pos - 4] == ' ' &&
	    sqlText[pos - 3] == 'A' &&
	    sqlText[pos - 2] == 'S')
	  return TRUE;
    }

  // Ansi reserved-word function names, tandem-extensions, and other keywords.
  // These must be in alphabetical order.  Order is checked for DEBUG builds.
  // There must also be a trailing space for each keyword.
  // Stored procedure names (e.g. EXPLAIN) are deliberately not in this list
  // nor are such tokens as CHECK, PRIMARY KEY, REFERENCES, VALUES, ...
  // Some expression names (e.g. CASE, COALESCE, NULLIF) are.
  //
  static const char *keywords[] =
  {
	"ABS ",                // Tandem-extension
	"ACOS ",               // Tandem-extension
	"ASC ",                // Collation name
	"ASCII ",              // Tandem-extension
	"ASIN ",               // Tandem-extension
	"ATAN ",               // Tandem-extension
	"ATAN2 ",              // Tandem-extension
	"AVG ",                // ANSI
	"BIT ",                // Datatype with scales/precisions/length
	"BIT_LENGTH ",         // ANSI
	"CASE ",               // ANSI
	"CAST ",               // ANSI
	"CEILING ",            // Tandem-extension
	"CHAR ",               // Datatype with scales/precisions/length
	"CHARACTER ",          // Datatype with scales/precisions/length
	"CHARACTER_LENGTH ",   // ANSI
	"CHAR_LENGTH ",        // ANSI
	"COALESCE ",           // ANSI
	"CODE_VALUE ",         // Tandem-extension
	"CONCAT ",             // Tandem-extension
	"CONVERT ",            // ANSI
	"CONVERTFROMHEX ",     // Tandem-extension
	"CONVERTTIMESTAMP ",   // Tandem-extension
	"CONVERTTOHEX ",       // Tandem-extension
	"COS ",                // Tandem-extension
	"COSH ",               // Tandem-extension
	"COUNT ",              // ANSI
	"CRC32 ",              // Trafodion extension
	"CURDATE ",            // Tandem-extension
	"CURRENT ",            // ANSI
	"CURRENT_DATE ",       // ANSI
	"CURRENT_TIME ",       // ANSI
	"CURRENT_TIMESTAMP ",  // ANSI
	"CURRENT_USER ",       // ANSI
	"CURTIME ",            // Tandem-extension
	"DATEFORMAT ",         // Tandem-extension
	"DAY ",                // Datatype with scales/precisions/length
	"DAYNAME ",            // Tandem-extension
	"DAYOFMONTH ",         // Tandem-extension
	"DAYOFWEEK ",          // Tandem-extension
	"DAYOFYEAR ",          // Tandem-extension
	"DEC ",                // Datatype with scales/precisions/length
	"DECIMAL ",            // Datatype with scales/precisions/length
	"DEGREES ",            // Tandem-extension
	"DESC ",               // Collation name
	"ENCODE_KEY ",         // Tandem-extension
	"EXP ",                // Tandem-extension
	"EXTEND ",             // ANSI
	"EXTERNAL ",           // Collation name
	"EXTRACT ",            // ANSI
	"FIRSTDAYOFYEAR ",     // Tandem-extension
	"FLOAT ",              // Datatype with scales/precisions/length
	"FLOOR ",              // Tandem-extension
        "GROUP_CONCAT",        // MySQL-extension
	"HASHPARTFUNC ",       // Tandem-extension
	"HOUR ",               // Datatype with scales/precisions/length
	"JSON_OBJECT_FIELD_TEXT" //json_object_field_text
	"JULIANTIMESTAMP ",    // Tandem-extension
	"LCASE ",              // Tandem-extension
	"LOCATE ",             // Tandem-extension
	"LOG ",                // Tandem-extension
	"LOG10 ",              // Tandem-extension
	"LOWER ",              // ANSI
	"LPAD ",               // Tandem-extension
	"LTRIM ",              // Tandem-extension
	"MAX ",                // ANSI
	"MD5 ",                // Trafodion extension
	"MIN ",                // ANSI
	"MINUTE ",             // Datatype with scales/precisions/length
	"MOD ",                // Tandem-extension
	"MONTH ",              // Datatype with scales/precisions/length
	"MONTHNAME ",          // Tandem-extension
	"NCHAR ",              // Datatype with scales/precisions/length
	"NOW ",                // Tandem-extension
	"NULLIF ",             // ANSI
	"NUMERIC ",            // Datatype with scales/precisions/length
	"OCTET_LENGTH ",       // ANSI
	"OS_USERID ",          // Tandem-extension
	"PI ",                 // Tandem-extension
	"PIC 9 ",              // Cobol datatype directly supported by SQLMX DDL
	"PICTURE 9 ",          // Cobol datatype directly supported by SQLMX DDL
	"POSITION ",           // ANSI
	"POWER ",              // Tandem-extension
	"QUARTER ",            // Tandem-extension
	"RADIANS ",            // Tandem-extension
	"RAND ",               // Tandem-extension
	"REPEAT ",             // Tandem-extension
	"ROUND ",              // Tandem-extension
	"ROUNDROBINPARTFUNC ", // Tandem-extension
	"RPAD ",               // Tandem-extension
	"RTRIM ",              // Tandem-extension
	"SECOND ",             // Datatype with scales/precisions/length
	"SESSION_USER ",       // ANSI
	"SHA ",                // Trafodion extension
	"SHA1 ",               // Trafodion extension
	"SHA2 ",               // Trafodion extension
	"SIGN ",               // Tandem-extension
	"SIN ",                // Tandem-extension
	"SINH ",               // Tandem-extension
	"SQRT ",               // Tandem-extension
	"STDDEV ",             // Tandem-extension
	"SUBSTRING ",          // ANSI
	"SUM ",                // ANSI
	"SYS_GUID ",           // Oracle-extension
	"TAN ",                // Tandem-extension
	"TANH ",               // Tandem-extension
	"TIME ",               // Datatype with scales/precisions/length
	"TIMESTAMP ",          // Datatype with scales/precisions/length
	"TRANSLATE ",          // ANSI
	"TRIM ",               // ANSI
	"TRUNCATE ",           // Tandem-extension
	"UCASE ",              // Tandem-extension
	"UPPER ",              // ANSI
	"UPSHIFT ",            // ANSI
	"USER ",               // ANSI
	"VARCHAR ",            // Datatype with scales/precisions/length
	"VARIANCE ",           // Tandem-extension
	"VARNCHAR ",           // Datatype with scales/precisions/length
	"VARYING ",            // Datatype with scales/precisions/length
	"WEEK ",               // Tandem-extension
	"YEAR ",               // Datatype with scales/precisions/length
  };

#ifdef _DEBUG
  // Only check the order of the above keywords in debug mode.
  static NABoolean checked_order = FALSE;
  if (!checked_order)
  {
    for (Int32 i = 1; i < (sizeof(keywords) / sizeof(keywords[0])); i++)
      {
        if (::strcmp(keywords[i], keywords[i - 1]) <= 0)
          {
             char err_buf[128];
             sprintf(err_buf, "keywords %s and %s are out of order",
                      keywords[i], keywords[i-1]);
             ABORT(err_buf);
          }
      }
    checked_order = TRUE;
  }
#endif

  // Return true if this is a keyword
  if (bsearch(tok.data() + 1, keywords, (sizeof(keywords) / sizeof(keywords[0])),
              sizeof(char*), bsearchStrcmp))
    return TRUE;

  // PICTURE or PIC (Cobol datatype directly supported by SQLMX DDL).
  if (prevpos) prevpos--;
  NAString prevtok(&sqlText.data()[prevpos]);
  NABoolean pic = FALSE;
  if (prevtok.length() > 9)
    {
      prevtok.remove(9);
      pic = prevtok == " PICTURE ";
    }
  if (!pic && prevtok.length() > 5)
    {
      prevtok.remove(5);
      pic = prevtok == " PIC ";
    }
  if (pic)
    {
      if (tok == " B "  || tok == " X " ||
          tok == " S9 " || tok == " S 9 " || tok == " SV9 " || tok == " V9 ")
	return TRUE;
    }
  if (tok == " V9 ")	// PICTURE S V9(nnn)
    {
      if (prevtok.length() > 3)
	{
	  prevtok.remove(3);
	  if (prevtok == " S ")
	    {
	      if (prevpos >= 8)
		{
		  prevtok = &sqlText.data()[prevpos-8];
		  prevtok.remove(9);
		  if (prevtok == " PICTURE ") return TRUE;
		}
	      if (prevpos >= 4)
		{
		  prevtok = &sqlText.data()[prevpos-4];
		  prevtok.remove(5);
		  if (prevtok == " PIC ") return TRUE;
		}
	    }
	}
    }

  return FALSE;
}

// This is cloned from sqlcomp/parser.C's stringScanWillTerminateInParser,
// though it serves a different purpose.
// It compresses multiple blanks into a single space, and optionally uppercases
// (it does not, of course, do these things within quoted text, for either
// ' or " quoting). //"
//
Lng32 PrettifySqlText(NAString &sqlText, const char *nationalCharSetName)
{
  #define prevResultChar  (result.length() ? result[result.length() - 1] : '\0')
  #define prevResultCharIs(c)  (prevResultChar == c)

  // Either this is NOT passed in (null pointer), OR it has the Ansi character
  // for charset introducer, the underscore.
  ComASSERT(!nationalCharSetName || *nationalCharSetName == '_');

  NAString result;
  enum TokType { SPACE, ALPHA, DELIM, DIGIT, LPAREN, PUNC, UNARYOP }
    toktype = SPACE, prevtoktype = SPACE;
  char prev = ' ';				// will remove leading blanks
  char quote_seen = '\0';
  size_t alphapos = 0, prevalphapos = 0;

  for (const char *s = sqlText.data(); *s; s++)
    {
      char curr = *s;
      if (quote_seen)
	if (*s == quote_seen)
	  quote_seen = '\0';
	else
	  { /*consume quoted character*/ }
      else if (*s == '\'' || *s == '"')
	{
	  quote_seen = *s;
	  if (toktype != DELIM)		// initial, not embedded, quote
	    {
              // Put a space in front of initial quote,
	      // unless it is a national, bit, or hex string literal (Ansi 5.3)
	      if (prev != ' ')
	        if (*s == '"')
	          result.append(" ");
	        else if (prev != '_' && strchr(specialSQL_TEXT, prev))
	          result.append(" ");
	        else if (prev == 'N' && nationalCharSetName)
		  if (result.length() >= 2)
		    if (result[result.length()-2] == ' ' ||
		        result[result.length()-2] == '(') {
		      // Here we have ' N' or '(N' preceding an initial squote.
		      // Replace the N with the actual cs name
		      // (which the caller must provide with the correct
		      // underscore introducer, e.g. "_KANJI").
		      result.remove(result.length() - 1);
		      result.append(nationalCharSetName);
		    }
	      toktype = DELIM;		// delim-ident or string literal...
	    }
	}
      else if (isSpace8859_1((unsigned char)*s))
        curr = ' ';			// convert unquoted tab/newline to space

      if (quote_seen || *s == '\'' || *s == '"') // in quotes or on ending quote
	{
	  result.append(s, 1);
	}
      else if (curr == ' ')
	{
	  if (prev == ' ')
	    { /*throw away the subsequent spaces; remain in whatever toktype*/ }
	  else
	    {
	      result.append(" ");	// append ourself, a space
	      prevtoktype = toktype;
	      toktype = SPACE;
	    }
	}
      else				// unquoted and not a space
        {
	  TokType efftoktype = toktype != SPACE ? toktype : prevtoktype;

	  // Put a space before the first letter of an identifier/hostvar/param,
	  // before the first digit of a number (but not a digit in an ident),
	  // before the first lparen of a series of lparens.
	  //
          NABoolean isInLatin1ExtendedHalf = FALSE;
          NAWchar tmpBuf[10];
          if ((UInt32)curr >= 0x80) // is not a 7-bit ASCII character
          {
            char * p1stUnstranslatedChar = NULL;
            UInt32 iOutLenInBytesIncludingNull = 0;
            UInt32 iTranslatedCharCount = 0;
            Int32 cnvErrStatus = LocaleToUTF16
              ( cnv_version1                 // in  - const enum cnv_version version
              , s                            // in  - const char *in_bufr
              , strlen(s)                    // in  - const int in_len
              , (const char *) tmpBuf        // out - const char *out_bufr
              , 10*BYTES_PER_NAWCHAR         // in  - const int out_bufr_size_in_bytes
              , cnv_UTF8                     // in  - enum cnv_charset charset of source
              , p1stUnstranslatedChar        // out - char * & first_untranslated_char
              , &iOutLenInBytesIncludingNull // out - unsigned int *output_data_len_p
              , 0                            // in  - const int cnv_flags
              , (Int32) TRUE                 // in  - const int addNullAtEnd_flag
              , &iTranslatedCharCount        // out - unsigned int * translated_char_cnt_p
              , 1                            // in  - unsigned int max_chars_to_convert
              );
            // NOTE: No errors should be possible -- string has been converted before.
            // ComASSERT(cnvErrStatus == 0 && iTranslatedCharCount == 1);
            if (cnvErrStatus EQU 0 AND (UInt32)tmpBuf[0] >= 0x80 AND (UInt32)tmpBuf[0] <= 0xFF)
            {
              isInLatin1ExtendedHalf = TRUE;
              curr = (unsigned char)tmpBuf[0];
              s++; // The character stored in curr requires two bytes in UTF-8 encoding value
            }
          }
	  if (isAlphaIsoMapCS((unsigned char)curr) || curr == '_' || curr == ':' ||
              curr == '?' || curr == '$'   || curr == '\\')  // non-Ansi '\nsk.$vol' extension
	    {
	      if (toktype != ALPHA)
	        {
		  toktype = ALPHA;

		  // 123.E+10  and  123. E+10  are numeric.
		  // ABC.E+10  is  ABC.E + 10  (efftoktype is alpha, thus E is).
		  // Note: need to test both prevResultChar and efftoktype here
		  // since rparen sets what can become our efftoktype to DIGIT;
		  // probably unnecessary since
		  // A)E+10    is not legal (derived-table-rename plus number?).
		  if (curr == 'E' || curr == 'e')
		    if (s[1] == '+' || s[1] == '-' || isDigit8859_1((unsigned char)s[1]))
		      if (isDigit8859_1((unsigned char)prevResultChar) || prevResultChar == '.')
			if (efftoktype == DIGIT)
			  {
			    // Avoid getting into toktype PUNC next loop iter:
			    if (s[1] == '+' || s[1] == '-')
			      {
			        result.append("E");
				curr = *++s;
			      }
			    toktype = DIGIT;
			  }

		  if (toktype == ALPHA)
		    {
		      if (prev != ' ') result.append(" ");
		      prevalphapos = alphapos;
		      alphapos = result.length();
		    }
		}
	    }
	  else if (isDigit8859_1((unsigned char)curr))
	    {
	      if (toktype != DIGIT && toktype != ALPHA)
	        {
		  if (prev != ' ') result.append(" ");
		  toktype = DIGIT;
		}
	    }
	  else if (curr == '(')
	    {
	      if (toktype != LPAREN)
	        {
		  if (prev != ' ') result.append(" ");
		  toktype = LPAREN;
		  if (prevResultCharIs(' '))  // note: wrong to test (prev==' ')
		    if (tokIsFuncOrParenKeyword(result, alphapos, prevalphapos))
		      result.remove(result.length() - 1);
		}
	    }
	  else if (curr == '.')
	    {
	      if (efftoktype == ALPHA || efftoktype == DELIM)
	        {
		  if (prevResultCharIs(' '))  // note: wrong to test (prev==' ')
		    result.remove(result.length() - 1);
		}
	      else if (toktype != DIGIT)	// not efftoktype!
		{
		  if (prev != ' ') result.append(" ");
		  toktype = DIGIT;
		}
	    }
	  else if (curr == ')' || curr == ',' || curr == ';')
	    {
	      // Remove any preceding space for this kind of punctuation.
	      if (prevResultCharIs(' '))    // note: wrong to test (prev==' ')
	        result.remove(result.length() - 1);

	      // Set prevtoktype for UNARYOP determination.
	      // Note that input of    (alpha)-1,(digit)+-1,isp()- -1
	      // will thus display as  (alpha) - 1, (digit) + -1, isp () - -1
	      prevtoktype = curr == ')' ? DIGIT : SPACE;
	      toktype = SPACE;
	    }
	  else
	    {
	      // Currently not dealing with Sql characters '&', '[', ']'
	      // because we don't expect them to appear in the text this
	      // procedure ever encounters.
	      if (toktype != PUNC)
	        {
		  // Put a space before first of a new series of punc
		  // and before subsequent unary minuses in a series of uminus
		  // (i.e. "x + - -y", not "x + --y", as "--" is Sql comment).
		  // Note: wrong to test (prev=='-'); see curr/prev set below.
		  if (prev != ' ' ||
		     (curr == '-' && prevResultCharIs('-')))
		    result.append(" ");

		  // '+ or -' is unary if it follows a reserved word, a comma,
		  // an LPAREN (but not an rparen!), or any PUNC or UNARYOP.
		  // E.g., select +1 + -2 from t where -3 < -col - (-5 + 6) - 7;
		  if (curr == '+' || curr == '-')
		    {
		      if (efftoktype == ALPHA)
			{
			  NAString tok;
			  tok += &result.data()[alphapos];
			  tok[tok.length()-1] = 0;
			  toktype = IsSqlReservedWord(tok.data()) ? UNARYOP : PUNC;
			}
		      else
			toktype = (efftoktype != DELIM && efftoktype != DIGIT) ?
				  UNARYOP : PUNC;
		    }
		  else
		    {
		      toktype = PUNC;

		      // If "CREATE ... LOCATION /G/directory ...",
		      // need to treat from the slash thru the next space
		      // as if they were quoted -- don't insert spaces
		      // and don't uppercase.
		      if (curr == '/' && efftoktype == ALPHA)
		        {
			  NAString tok(&result.data()[alphapos]);
			  if (tok == "LOCATION ")
			    {
			      tok = result;
			      tok.remove(7);
			      if (tok == "CREATE ")
			        quote_seen = ' ';	// will consume till ' '
			    }
			}
		    }
		}
	      else if (curr == '+' || curr == '-')
	        {
		  // Put a space before a unary operator.
		  if (!prevResultCharIs(' ')) result.append(" ");
		  toktype = UNARYOP;
		}
	    }

	  // Preceding spaces have been dealt with; now add the current char.
	  if (isInLatin1ExtendedHalf)
	  {
	    NAString ns(curr);
	    ns.toUpper8859_1();
	    curr = (char)ns.data()[0];
	
	    char utf8Buf[20];
	    char * p1stUnstranslatedChar = NULL;
	    UInt32 utf8OutLenInBytesIncludingNull = 0;
	    UInt32 charCount = 0;
	    Int32 returnCode = LocaleToUTF8
	      ( cnv_version1
	      , (const char*) ns.data()         // source
	      , (const Int32) ns.length()       // source len in bytes - should be 1
	      , (const char*) utf8Buf           // output buffer for target
	      , (const Int32) 20                // output buffer size in bytes
	      , cnv_ISO88591                    // source char set
	      , p1stUnstranslatedChar           // char * &       first_untranslated_char
	      , &utf8OutLenInBytesIncludingNull // unsigned int * output_data_len_p in bytes including '\0' terminator
	      , (const Int32)TRUE               // const int      addNullAtEnd_flag
	      , &charCount                      // unsigned int * translated_char_cnt_p - should be 1
	      );
	    ComASSERT(returnCode == 0 && charCount == 1 && utf8OutLenInBytesIncludingNull == 3);

	    // Exclude the NULL terminator added at the end (addNullAtEnd_flag was set to TRUE in the above call)
	    // from the count.
	    UInt32 utf8StrLenInBytes = 0;
	    if ((Int32)utf8OutLenInBytesIncludingNull >= CharInfo::minBytesPerChar(CharInfo::UTF8))
	      utf8StrLenInBytes = utf8OutLenInBytesIncludingNull - CharInfo::minBytesPerChar(CharInfo::UTF8);

            ComASSERT(utf8StrLenInBytes > 0);
	    result.append(utf8Buf, utf8StrLenInBytes);
	  }
	  else if ((UInt32)curr < 0x80) // is a 7-bit ASCII character
	  {
	    curr = (char)toupper(curr);
	    result.append(&curr, 1);
          }
	  // This kind of punctuation does not want spaces following it --
	  // except, do not collapse "* *" (as in "CONTROL TABLE * * RESET;")
	  // into "**" exponentiation operator.
	  //   (Note that '?' does not appear here:  Tdm named parameters
	  //   must have the name immediately following the '?', to distinguish
	  //   from Ansi unnamed params.  Also, Tdm '\nsk.$vol' punc is never
	  //   allowed with trailing spaces on input, so no need here to
	  //   scan ahead and remove it.)
	  // Remain in same toktype (space/lparen/alpha/etc).

	  if (curr == '.' || curr == '(' || curr == ':' ||
	      toktype == PUNC || toktype == UNARYOP)
	    {
	      if (curr != '*')
		while (isSpace8859_1((unsigned char)s[1])) s++;	// throw away following spaces

	      // Set curr to space, which next sets prev to space, which
	      // in the next loop iter will prevent a space from being appended.
	      // Thus, CAT.SCH.TBL.FLTCOL > 1.5 + -7 will display correctly.
	      if (toktype != PUNC) curr = ' ';
	    }

	} // unquoted and not a space

      prev = curr;

    } // loop over sqlText

  sqlText = result;

  if (quote_seen) {
    return -15005;	// Unmatched quote
  }

  size_t len = sqlText.length();
  if (len) {
    len--;
    if (sqlText[len] == ' ') sqlText.remove(len--);	// trim final space
//  These lines commented out because space already removed before ';' above...
//  if (sqlText[len] == ';' && len--)
//    if (sqlText[len] == ' ') sqlText.remove(len,1);	// trim space before ';'
  }

  return 0;		// no error

} // PrettifySqlText

// SQL/MX Regression Test Support
// Calculate an increased max output line length to accommodate schema names
// longer than 'SCH', that are used when regression test suites are executed
// concurrently.  The number of characters that schemaName is longer than
// 'SCH' is referred to as the "excess character count" in the following
// description.  The increase beyond maxLineLen is calculated as the excess
// character count times the number of times schemaName occurs in the search
// substring of sqlText starting at pos.  The search substring length is
// increased by the excess character count each time schemaName is found in
// the search substring.
//
size_t adjustedMaxLen(const NAString &sqlText, size_t pos, size_t maxLineLen,
                      const char *schemaName)
{
  const size_t SCH_LEN = 3;  // Number of chars in 'SCH'
  size_t schemaNameLen = schemaName ? strlen(schemaName) : 0;
  if (schemaNameLen > SCH_LEN)
  {
    size_t excessCharCount = schemaNameLen - SCH_LEN;
    size_t sqlTextLen = sqlText.length();
    size_t maxSearchPos = pos + maxLineLen - 1;
    size_t occurs = 0;
    while (TRUE)
      {
        if (maxSearchPos >= sqlTextLen)
          maxSearchPos = sqlTextLen - 1;
        pos = sqlText.index(schemaName, pos, NAString::ignoreCase);
        if ((pos == NA_NPOS) || (pos > maxSearchPos))
          break;
        occurs++;
        pos += schemaNameLen;
        maxSearchPos += excessCharCount;
      }
    return maxLineLen + (occurs * excessCharCount);
  }
  else
    return maxLineLen;
}

// Called by SHOWDDL command (CmpDescribe.C).
// Inserts linebreaks at word boundaries in order to keep tokens whole --
// to make it easier for a user to cut SHOWDDL output text and paste it
// into SQLCI as a new command.
//
// The optional schemaName argument provides SQL/MX regression test
// support.  When a schemaName is provided, the maximum output line
// length is adjusted so that lines are broken in the same logical place
// they would be if the deafult schema was 'SCH'.
//
size_t LineBreakSqlText(NAString &sqlText,
			NABoolean showddlView,
			size_t maxlen,
		        size_t pfxlen,
			size_t pfxinitlen,
			char pfxchar,
			const char * schemaName,
			NABoolean commentOut)
{
  if (commentOut && pfxchar != '-')
  {
    // Make sure that the prefixes have enough room
    // for the leading "--" comment prefix.
    if (pfxlen < 2)
      pfxlen += (2 - pfxlen);
    if (pfxinitlen < 2)
      pfxinitlen += (2 - pfxinitlen);
  }
  // The initial line can be indented differently from subsequent lines.
  if (maxlen == 0 || maxlen <= pfxlen) maxlen = pfxlen + 1;
  size_t maxinitlen = (maxlen <= pfxinitlen) ? pfxinitlen + 1 : maxlen;
  maxlen -= pfxlen;
  maxinitlen -= pfxinitlen;
  size_t maxcurrlen = adjustedMaxLen(sqlText, 0, maxinitlen, schemaName);

  NAString result(pfxchar, pfxinitlen);
  NAString pfx(pfxchar, pfxlen);
  if (commentOut && pfxchar != '-')
  {
    result[(size_t)0] = '-';
    result[(size_t)1] = '-';
    pfx[(size_t)0] = '-';
    pfx[(size_t)1] = '-';
  }
  pfx.prepend("\n");

  NABoolean showddlViewAS = FALSE;
  char quote_seen = '\0';
  size_t cnt = 0, space = 0, dot[3];		// C.S.T.COL ref has max 3 dots
  size_t sqlNextPos = 0;
  dot[0] = 0;
  dot[1] = 0;
  sqlText += "\n";				// sentinel (newline, for "--")
  for (const char *s = sqlText.data(); *s; s++)
    {
      if (quote_seen)
	if (*s == quote_seen)
	  quote_seen = '\0';
	else
	  { /*consume quoted character*/ }
      else if (*s == '\'' || *s == '"')
        quote_seen = *s;
      else if (*s == '-' && s[1] == '-')	// SQL comment: "--" to eol
        quote_seen = '\n';

      result.append(s, 1);
      cnt++;
      sqlNextPos++;
      if (!quote_seen)
        if (isSpace8859_1((unsigned char)*s))                        // sentinel ensures we get here
	  {
	    if (showddlView)			// look for keyword "AS"
	      if ((space && result.length() - space == 3) ||
		  (!space && cnt == 3))
		if (result[result.length()-3] == 'A' &&
		    result[result.length()-2] == 'S')
		  showddlViewAS = TRUE;

	    if (cnt < maxcurrlen)
	      {
		space = result.length();	// thus space > pfxlen
	      }
	    else
	      {
		if (cnt > maxcurrlen + 1 && space > 0)	// linebreak on space
		  {
		    result.replace(space - 1, 1, pfx);
		    cnt = result.length() - space - pfxlen;
		    space = result.length();
		    Int32 i=0;
		    for (; i<3; i++)
		      if (!dot[i]) break;
		      else dot[i] += pfxlen;		// replaced 1 with p+1
		    maxcurrlen = adjustedMaxLen(sqlText, sqlNextPos - cnt,
		                                maxlen, schemaName);
		    // fall through to while loop
		  }

		while (cnt > maxcurrlen + 1)
		  {
		    NABoolean dotfound = FALSE;
		    size_t dotdiff;
                    Int32 i = 0;
		    for (i=0; i<3; i++)
		      if (!dot[i]) break;
		      else
		        {
			  dotdiff = dot[i] - (result.length() - cnt);
			  if (dotdiff <= maxcurrlen) { dotfound = TRUE; break; }
			}
		    if (dotfound)    // linebreak after dot
		      {		     // and don't forget to dot your i's!
		        result.insert(dot[i], pfx);
			cnt -= dotdiff;
			dot[i] = 0;
			for ( ; i--; ) dot[i] += pfxlen + 1;	// inserted p+1
		      }
		    else	    // linebreak after too-long unspaced token
		      {
			result.remove(result.length() - 1);
			result += pfx;
			cnt = space = 0;
		      }
		    maxcurrlen = adjustedMaxLen(sqlText, sqlNextPos - cnt, maxlen,
						schemaName);
		  } // while still too long

		if (cnt >= maxcurrlen)
		  {
		    result.remove(result.length() - 1);
		    result += pfx;
		    cnt = space = 0;
		    maxcurrlen = adjustedMaxLen(sqlText, sqlNextPos, maxlen,
						schemaName);
		  }
		else if (cnt && result[result.length() - 1] == ' ')
		  space = result.length();

	      } // need to linebreak

	    dot[0] = 0;

	    if (showddlViewAS)			// it was the keyword "AS"
	      {
                size_t i;
		for (i = result.length() - 1;
		     result[i] == ' ' || result[i] == pfxchar; i--)
		  result.remove(i);
		if (result[i] != '\n') result += "\n";	// newline, no pfx chars

		if (result.length() <= maxinitlen + 1)
		  pfxlen = pfxinitlen;		// still the first line

		NAString queryText(++s);	// get past the current space
		cnt = LineBreakSqlText(queryText, FALSE,
				       maxlen+pfxlen, pfxlen+4, pfxlen+2,
				       pfxchar, schemaName);
		result += queryText;
		s = &sqlText.data()[sqlText.length() - 1];  // will exit loop
	      }

	  } // unquoted space

	else if ((*s == '.') &&
                 (!isDigit8859_1((unsigned char)s[1]))) // ignore dots in numeric constants
	  {
	    dot[2] = dot[1];
	    dot[1] = dot[0];
	    dot[0] = result.length();
	  } // unquoted dot

    } // loop over sqlText

  sqlText = result;
  TrimNAStringSpace(sqlText, FALSE, TRUE);    // remove trailing sentinel char
  return cnt;

} // LineBreakSqlText

void GetSimplePosixFilename(NAString &filename, NABoolean doLower)
{
  // Remove any preceding directory path
  const char *fslash = strrchr(filename, '/'),
             *bslash = strrchr(filename, '\\'),
             *dirpathpunc;
  if (fslash && bslash)
    dirpathpunc = fslash > bslash ? fslash : bslash;
  else
    dirpathpunc = fslash ? fslash : bslash;
  if (dirpathpunc) filename = ++dirpathpunc;

  #ifdef NA_CASE_INSENSITIVE_FILENAMES
    if (doLower) filename.toLower();
  #endif

} // GetSimplePosixFilename

void FUNNY_ANSI_IDENT_REMOVE_PREFIX(NAString &str, const char *pfx)
{
  // Say str is "PACKED__@T" and pfx is PACKED__@
  str.remove(1, strlen(pfx));		// str is now "T" (dquotes kept)
  ToInternalIdentifier(str);		// str is now T
  str = ToAnsiIdentifier(str);		// str remains T
  assert(!str.isNull());
}

NAString Latin1StrToUTF8(const NAString & latin1Str, NAMemory * heap)
{
  if (latin1Str.isNull())
    return NAString();

  char buffer[3008]; // allocate a few extra bytes to make me feel better
  char * target = &buffer[0];
  bool isBufferAllocatedFromProcessHeap(FALSE);
  Lng32 targetBufferLen = (Lng32)(latin1Str.length() * 4 /* SQL_UTF8_CHAR_MAXSIZE */ + 2);
  if ( targetBufferLen > 3000 )
  {
    isBufferAllocatedFromProcessHeap = TRUE;
    target = new (heap) char[targetBufferLen + 2]; // allocate a couple extra bytes ...
  }
    
  char * p1stUnstranslatedChar = NULL;
  UInt32 utf8StrLenInBytes = 0;
  UInt32 charCount = 0;  // number of characters translated/converted
  Int32 errorCode = LocaleToUTF8 ( cnv_version1
                               , latin1Str.data()          // in  - const char *   srcStr
                               , (Int32)latin1Str.length() // in  - const int      srcStrLen
                               , (const char*)target       // out - const char *   bufferForTargetStr
                               , (Int32)targetBufferLen    // in  - const in       targetBufferSizeInBytes
                               , cnv_ISO88591              // in  - cnv_charset    srcCharset
                               , p1stUnstranslatedChar     // out - char* &        first_untranslated_char
                               , &utf8StrLenInBytes        // out - unsigned int * output_data_len_p 
                               , (const Int32)TRUE         // in  - const int      addNullAtEnd_flag
                               , &charCount                // out - unsigned int * translated_char_cnt_p
                               );
  // Exclude the NULL terminator added to the end (addNullAtEnd_flag was set to TRUE in the above call)
  // from the count.
  if ((Int32)utf8StrLenInBytes >= CharInfo::minBytesPerChar(CharInfo::UTF8))
    utf8StrLenInBytes -= (UInt32)CharInfo::minBytesPerChar(CharInfo::UTF8);
  else
    utf8StrLenInBytes = 0;
  NAString result;
  if (utf8StrLenInBytes > 0)
    result.append(target, (size_t)utf8StrLenInBytes);
  if (isBufferAllocatedFromProcessHeap)
    NADELETEBASIC(target, heap);
  return result;
}

// -----------------------------------------------------------------------
// StatementHeap-related stuff
// -----------------------------------------------------------------------

static NAMemory *TheStatementHeap = NASTRING_UNINIT_HEAP_PTR;

