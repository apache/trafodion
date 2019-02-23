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
#ifndef CHARINFO_H
#define CHARINFO_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         CharInfo.h
 * Description:  The header file for the CharInfo class. This class defines
 *   	 	 and provides features/information about character sets,
 *		 collations and coercibility that are supported by SQL/MX.
 *
 *
 * Created:      7/8/98
 * Modified:     $Date: 2007/10/09 19:38:37 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *****************************************************************************
 */


#include "Platform.h"

#include "Collections.h"	// for LIST in CollationDB
#include "NABoolean.h"
#include "Platform.h"
#include "NAWinNT.h"
#include "ComCharSetDefs.h"
#include "sql_charset_strings.h"

// Forward references
class ComMPLoc;
class QualifiedName;
class SchemaName;

// Contents of this file
class CharInfo;
class CollationInfo;
class CollationDB;


// Notice how we currently equate SQL_TEXT with ISO88591.
// This is used by StaticCompiler.cpp, in the MODULE statement.
//
//   ***HOWEVER:***							##
//   Ansi 4.2 specifies that SQL_TEXT has to be the union of all char-sets
//   implemented by an implementation. In our case, it should be UNICODE.
//   So I am not sure if we should disallow 'MODULE...NAMES ARE SQL_TEXT'
//   and instead promote '...NAMES ARE ISO88591' instead.  
//
//   Taking a UNICODE module can be done, except that we can not
//   effectively spit out the error msgs as we do not know the locale
//   into which error msg is converted
//   (NT does not have a font that would cover all UNICODE characters!).
//
//   Disallowing 'MODULE...NAMES ARE SQL_TEXT' anytime soon would cause
//   disruption to our Beta sites.
//   I propose that, whenever we do fully support a UNICODE module file,
//   then we switch the CharInfo enum SQL_TEXT to equal UNICODE instead
//   of ISO88591 (and deal with any disruption then...).


#undef UNICODE
// conflict with definition in file sqlmxevents/zmxc
#undef SQL_TEXT

#undef  MAX_CHAR_SET_STRING_LENGTH
#define MAX_CHAR_SET_STRING_LENGTH 128	// keep in sync with w:/cli/sqlcli.h
					// and with w:/common/ComSizeDefs.h
					//     ComMAX_ANSI_IDENTIFIER_INTERNAL_LEN_IN_NAWCHARS

#define MAXNPASSES 4                    // Maximum number of Passes for any collation
static const short collationNPasses[] = {2,2};
static const unsigned char collationMaxChar[] = {0x81,0x81};


class CharInfo
{
public:

   enum CharSet		// keep in sync with w:/cli/sqlcli.h enum SQLCHARSET_CODE !
   {    		// and with the charset map table in CharInfo.cpp  !
     CHARSET_MIN    = -2,
     KSC5601_MP	    = -2,  // an MX Unicode encoding could be named KSX5601.
     KANJI_MP	    = -1,  // logically equiv to SJIS, physically diff encoding.
     // the defines below are defined in common/ComCharSetDefs.h, so
     // that they can be used by other components (ODBC, utilities?)
     // without sourcing in this file
     UnknownCharSet =  SQLCHARSETCODE_UNKNOWN, //  0
     ISO88591 = SQLCHARSETCODE_ISO88591,   //  1
     ISO88592 = 2, 
     ISO88593 = 3, 
     ISO88594 = 4, 
     ISO88595 = 5,
     ISO88596 = 6, 
     ISO88597 = 7, 
     ISO88598 = 8, 
     ISO88599 = 9,
     SJIS     = SQLCHARSETCODE_SJIS,       // 10
     UNICODE  = SQLCHARSETCODE_UCS2,       // 11
     EUCJP    = SQLCHARSETCODE_EUCJP,      // 12
     BIG5     = SQLCHARSETCODE_BIG5,       // 13
     GB18030  = SQLCHARSETCODE_GB18030,    // 14
     UTF8     = SQLCHARSETCODE_UTF8,       // 15
     KSC5601  = SQLCHARSETCODE_MB_KSC5601, // 16
     GB2312   = SQLCHARSETCODE_GB2312,     // 17
     GBK      = SQLCHARSETCODE_GBK,        // 18
     BINARY   = 19,                        // 19
     //
     // synonyms
     SQL_TEXT	    = UNICODE,
     UCS2	    = UNICODE,
     DefaultCharSet = ISO88591,

     CHARSET_MAX    = 19,

     // for internal use only
     ISO_MAPPING_CODE = SQLCHARSETCODE_ISO_MAPPING // 9999
   };

   enum Collation    { UNKNOWN_COLLATION    = 0,
		       DefaultCollation	    = 1,
		       SJIS_COLLATION	    = SJIS,  // to prevent coding bugs!// SJIIS= 10
		       FIRST_SYS_COLLATION  =100,   //system collations start at 101 to Last_sys_collation
		       CZECH_COLLATION      =101,
                       CZECH_COLLATION_CI   =102,
		       LAST_SYS_COLLATION   ,
		       FIRST_USER_DEFINED_COLLATION = 1000
		     };

   enum Coercibility { NO_COLLATING_SEQUENCE = 0, COERCIBLE, IMPLICIT, EXPLICIT
		     };

   static CharSet  	getCharSetEnum(const char* name);
   static const char*	getCharSetName(CharSet cs,
   				       NABoolean retUnkAsBlank = FALSE);
   static NABoolean	isCharSetSupported(CharSet cs);
   static NABoolean	isCharSetSupported(const char* name)
   			{ return isCharSetSupported(getCharSetEnum(name)); }

   static NABoolean	isCharSetFullySupported(CharSet cs);
   static NABoolean	isCharSetFullySupported(const char* name)
   			{ return isCharSetFullySupported(getCharSetEnum(name)); }

   static NABoolean	isOnlySingleByteCharacters(CharSet cs);
   static NABoolean	isOnlySingleByteCharacters(const char* name)
   			{ return isOnlySingleByteCharacters(getCharSetEnum(name)); }

   static NABoolean     isModuleCharSetSupported(CharSet cs)
                        { return cs == CharInfo::ISO88591; };

   static NABoolean     isHexFormatSupported(CharSet cs);
   static NABoolean     isHexFormatSupported(const char* name)
			{ return isHexFormatSupported(getCharSetEnum(name)); }

   static NABoolean	isTerminalCharSetSupported(CharSet cs);
   static NABoolean	isTerminalCharSetSupported(const char* name)
			{ return isTerminalCharSetSupported(getCharSetEnum(name)); }

   static NABoolean     isMsgCharSetSupported(CharSet cs);
   static NABoolean	isMsgCharSetSupported(const char* name)
			{ return isMsgCharSetSupported(getCharSetEnum(name)); }

   static void		toggleCharSetSupport(CharSet cs); // debugging only
   static Int32	        minBytesPerChar(CharSet cs);
   static Int32	        maxBytesPerChar(CharSet cs);
   static Int32	        bytesPerChar(CharSet cs)
                        { return maxBytesPerChar(cs); }

   static NABoolean	isSingleByteCharSet(CharSet cs)
			{ return maxBytesPerChar(cs) == 1
			    || cs == CharInfo::UTF8 // is variable-length/width multi-byte char-set but treat it as a C/C++ string
			    ; }
   static NABoolean     isVariableWidthMultiByteCharSet(CharSet cs);

   static NABoolean	is_NCHAR_MP(CharSet cs)
			{ return cs == KANJI_MP || cs == KSC5601_MP; }

   static Int32         getFSTypeFixedChar(CharSet cs);
   static Int32         getFSTypeVarChar(CharSet cs);
   static Int32         getFSTypeANSIChar(CharSet cs);

   static const char*   getReplacementCharacter(CharSet cs); // replacement for untranslatable chars

   // get the encoding charset for a (logical/SQL) charset
   static CharInfo::CharSet getEncoding(const CharInfo::CharSet);


   static Collation	getCollationEnum(const char *name,
					 NABoolean formatNSK = FALSE,
   					 size_t maxlen = 0);
   static const char*	getCollationName(Collation co,
   					 NABoolean retUnkAsBlank = FALSE);
   static Int32	        getCollationFlags(Collation co);
   static NABoolean	isCollationUserDefined(Collation co)
   { return co >= FIRST_USER_DEFINED_COLLATION; }	// watch out for UNKNOWN_COLLATION!


   static const char*	getCoercibilityText(Coercibility ce);
   static NABoolean	compareCoercibility(Coercibility ce1, Coercibility ce2);

   // check if the code point value for each character in the input string is
   // a valid UCS2 character
   static NABoolean checkCodePoint(const NAWchar *inputStr, Int32 inputLen, CharInfo::CharSet cs);

   // Convert the int value returned by MBCS_DEFAULTCHARSET_()
   // to MX enum value.  See Guardian Procedure Calls Ref Manual.
   static CharSet  	getCharSetEnumFromNSK_MBCS(Int32 n)
   {
     switch (n) {
       case 1:	return KANJI_MP;
       case 12:	return KSC5601_MP;
       default:	return UnknownCharSet;	// we don't support other magic numbers
     }					// like Hangul, Big5, Chinese PC
   }

   // Obtain the character set locale information about locale machine.
   // The result is one of the SQLCHARSETCODE_xxx values defined in SQLCLI.h,
   // or "UNKNOWN" for a unknown locale.
   // Used by the sql_id and Formatter classes.
   static Lng32		findLocaleCharSet();
   static const char*	getLocaleCharSetAsString();
   static Int32		getTargetCharTypeFromLocale();

   // check whether the client character set (e.g., the cs of a hostvar) is
   // assignment compatible with the MX one (e.g., the cs of a column). 
   static NABoolean isAssignmentCompatible(CharSet clientCS, CharSet mxCS) 
   {
     return clientCS == mxCS ||
            (clientCS == CharInfo::UNICODE && // relaxation
             mxCS == CharInfo::ISO88591); 
   };

  // for an arbitrary string encoded in "sourceCS", with length
  // "sourceLenInBytes", what is the max. length in bytes of this
  // string after converting it to "targetCS"?
  static Int32 getMaxConvertedLenInBytes(CharSet sourceCS, 
                                         Int32   sourceLenInBytes,
                                         CharSet targetCS);

  static const CollationDB *builtinCollationDB();
 

private:
friend class CollationDB;			// needs to access builtinCDB_

   static const char*	const localeCharSet_;
   static const CollationDB   *builtinCollationDB_;

}; // CharInfo

// For the convenience of SqlParser.y, and ItemExpr::bindNode()
struct CollationAndCoercibility {
  CharInfo::Collation           collation_;
  CharInfo::Coercibility        coercibility_;
};


class CollationInfo : public NABasicObject
{
friend class CollationDB;  // for robust security/validity: see private methods!

public:
  enum CollationFlags {
    // external (governing SQL syntax and what MX supports) flags
    NO_FLAGS			= 0,
    NO_PAD			= 0x1,		// Ansi NO PAD (vs. PAD SPACE)
    ORDERED_CMP_ILLEGAL		= 0x10,		   // MP collations in MX-NSK-R1
    EQ_NE_CMP_ILLEGAL		= 0x20,		   // MP collations in MX-NSK-R1
    ALL_CMP_ILLEGAL		= EQ_NE_CMP_ILLEGAL | ORDERED_CMP_ILLEGAL,
    ALL_NEGATIVE_SYNTAX_FLAGS	= 0x0FFFFFFF,

    // internal (class-implementation) flags
    NO_ALLOC_AND_COPY_IN_CTOR	= 0x10000000,

    ALL_NEGATIVE_PLUS_STATIC	= ALL_NEGATIVE_SYNTAX_FLAGS | NO_ALLOC_AND_COPY_IN_CTOR
  };
  enum CollationMisc { MAX_NAME_PARTS   = 4,
		       SIZEARRAY_SIZE   = MAX_NAME_PARTS + 1,
		       OFFSETARRAY_SIZE = MAX_NAME_PARTS - 1
		     };

  enum Pass 
  {
    FirstPass	=0,
    SecondPass	=1,
    ThirdPass	=2,
    FourthPass   =3
  };

  enum CollationType 
  {
   
    Sort	=0, // Used for sort. when the collation type is Sort, the encoded 
		    // value of a nullable column is prepended with 2 bytes 
		    // indicating whether the value is null or not null
		    
    Compare	=1, // Used to compare two character strings. The main diffrence 
		    // with the sort type is that the encoded value of a null value 
		    // is actually a null value
    
    Search	=2  // used to do string search
  };

  enum SortDirection 
  {
    DefaultDir	=0,
    Ascending	=1,
    Descending	=2
  };


  CollationInfo(CollHeap *h, CharInfo::Collation co, const char *name,
		CollationFlags flags = NO_FLAGS,
		size_t *sizArray = NULL);	  /* array[SIZEARRAY_SIZE] */

  ~CollationInfo()
  { 
    if (!(flags_ | NO_ALLOC_AND_COPY_IN_CTOR)) delete (char*)name_; 
  }

  CharInfo::Collation	getCollationEnum() const	{ return co_; }
  const char *		getCollationName() const	{ return name_; }
  Int32			getCollationFlags() const	{ return flags_; }

  // for debugging
  void			display() const;

  static NABoolean isSystemCollation(const CharInfo::Collation collation) 
  {
    return (collation > CharInfo::FIRST_SYS_COLLATION && collation < CharInfo::LAST_SYS_COLLATION);
  }
 
  static short  getCollationParamsIndex(const CharInfo::Collation collation) 
  {
    return (collation - CharInfo::FIRST_SYS_COLLATION -1);
  }

  inline static short  getCollationNPasses(const CharInfo::Collation collation) 
  {
    return collationNPasses[getCollationParamsIndex( collation) ];
  }

  inline static unsigned char  getCollationMaxChar(const CharInfo::Collation collation) 
  {
    return collationMaxChar[getCollationParamsIndex( collation) ];
  }


 

private:

  void setFlags(CollationFlags f)			{ flags_ |= f; }
  void clrFlags(CollationFlags f)			{ flags_ &= ~f; }

  // See ComMPLoc::getMPName(size_t *) 
  // and QualifiedName::getQualifiedNameAsAnsiString(size_t *)
  // -- an MP name can have up to 4 parts (MAX_NAME_PARTS),
  // and an Ansi name up to 3.
  // Hence, here we have:
  // - one fixed string for the whole name,
  // - the length of the whole,
  //   and, where the caller (CollationDB::insert methods)
  //   has determined that valid synonyms exist based on current defaults,
  // - offsets into the string pointing to those synonyms
  //   (an offset value of zero means no synonym!).
  //
  // E.g., for name "\X.$Y.Z.W", if default MPLoc was "\X.$Y.DIFFSV",
  //		     012345678
  // here the namelen would of course be 9,
  // and the three offsets would be 3, 6, and 0.
  // Put another way, the string starting at name_[3] is "$Y.Z.W",
  // the string at name_[6] is "Z.W",
  // both of which are equivalent to the full name, GIVEN THE DEFAULTS.
  // The string at name_[8] is "W", which by the given defaults
  // is equivalent to the different full name "\X.$Y.DIFFSV.W";
  // hence the third offset is 0 in this example.
  //
  CharInfo::Collation	co_;
  Int32			flags_;
  const char*		name_;
  size_t		namelen_;
  size_t		synonymOffset_[OFFSETARRAY_SIZE];

}; // CollationInfo


typedef LIST(const CollationInfo *) CollationDBSupertype;
typedef LIST(CollationDB *)   CollationDBList;

class CollationDB : private CollationDBSupertype
{
public:

  CollationDB(CollHeap *h);

  CollationDB(CollHeap *h, const CollationInfo *co, size_t count);

  ~CollationDB();

  void clearAndReset()
  {
    for (CollIndex i = entries(); i--; ) {
      delete at(i);
      at(i) = NULL;	// (just in case)
    }
    clear();
  }

  // for debugging only
  void			display() const;
  static void		Display();

  NABoolean &refreshNeeded()	{ return refreshNeeded_; }

  // The insert methods assume you've already done an unsuccessful lookup.
  //
  // They return a newly generated CharInfo::Collation value for the
  // user-defined collation just inserted.

  CharInfo::Collation insert(ComMPLoc &loc,
			     const ComMPLoc *defaultMPLoc,
			     CollationInfo::CollationFlags
			     		    flags=CollationInfo::NO_FLAGS);

  CharInfo::Collation insert(QualifiedName &qn,
			     const SchemaName *defaultSchema,
			     CollationInfo::CollationFlags
					    flags=CollationInfo::NO_FLAGS);

private:
friend class CharInfo;	// its static funx should be the only callers of these:

  CharInfo::Collation insert(const char *nam,
			     size_t *sizArray,	    /* array[SIZEARRAY_SIZE] */
			     CollationInfo::CollationFlags flags,
			     Int32 defaultMatchCount);
  inline
  CollationDB *		nextCDB() const;
 
  const CollationInfo *	getCollationInfo(CharInfo::Collation co) const;

  CharInfo::Collation	getCollationEnum(const char *name,
					 NABoolean formatNSK,
  					 size_t namlen) const;
  const char *		getCollationName(CharInfo::Collation co,
  					 NABoolean retUnkAsBlank) const;

  Int32			getCollationFlags(CharInfo::Collation co) const;

  // data members
  CollHeap *			heap_;
  NABoolean			refreshNeeded_;

  static Lng32			nextUserCo_;
}; // CollationDB


#endif //CHARINFO_H
