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
 * File:         CharInfo.cpp
 * Description:  The implementation for the CharInfo class. This class defines
 *   	 	 and provides features/information about character sets,
 *		 collations and coercibility that are supported by SQL/MX.
 *
 *
 * Created:      7/8/98
 * Modified:     $Date: 2006/11/01 01:38:09 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *****************************************************************************
 */

#include "NAWinNT.h"

#include "BaseTypes.h"
#include "charinfo.h"
#include "ComASSERT.h"
#include "ComMPLoc.h"
#include "dfs2rec.h"		// for LOCALE stuff: REC_xBYTE_LOCALE_*
#include "SQLCLIdev.h"		// for LOCALE stuff: SQLCHARSETCODE_*
#include "str.h"
#include "wstr.h"
#include "SQLTypeDefs.h"
#include "CmpMessage.h"
#include "CmpConnection.h"
#include "CmpContext.h"
#include "CmpCommon.h"
#include "CliSemaphore.h"

using namespace std;

//****************************************************************************
// CHARSET stuff
//****************************************************************************

#define IF_WIDE TRUE
#define IF_NSK FALSE

struct mapCS {
  CharInfo::CharSet cs;
  const char*       name;
  size_t	    namelen;		// len(SQLCHARSETSTRING_xxx)
  NABoolean	    supported;
  NABoolean	    fully_supported;
  Int32             minBytesPerChar;
  Int32             maxBytesPerChar;
  const char*       replacementChar;
};

// Arranged as an array, starting with CHARSET_MIN and ending with CHARSET_MAX
static const struct mapCS mapCSArray[] = {
  // CharSet enum value              string value of charset    name  supp.    fully   min / max  rep.
  //                                                            len            supp.   bytes/char char
  { /*-2*/ CharInfo::KSC5601_MP,     SQLCHARSETSTRING_KSC5601,     7, IF_NSK,  IF_NSK, 2,    2, NULL },
  { /*-1*/ CharInfo::KANJI_MP,       SQLCHARSETSTRING_KANJI,       5, IF_NSK,  IF_NSK, 2,    2, NULL },
  { /* 0*/ CharInfo::UnknownCharSet, SQLCHARSETSTRING_UNKNOWN,     9, FALSE,   FALSE,  1,    1, "?" },

  { /* 1*/ CharInfo::ISO88591,       SQLCHARSETSTRING_ISO88591,    8, TRUE,    TRUE,   1,    1, "?" },
  { /* 2*/ CharInfo::ISO88592,       SQLCHARSETSTRING_ISO88592,    8, FALSE,   FALSE,  1,    1, "?" },
  { /* 3*/ CharInfo::ISO88593,       SQLCHARSETSTRING_ISO88593,    8, FALSE,   FALSE,  1,    1, "?" },
  { /* 4*/ CharInfo::ISO88594,       SQLCHARSETSTRING_ISO88594,    8, FALSE,   FALSE,  1,    1, "?" },
  { /* 5*/ CharInfo::ISO88595,       SQLCHARSETSTRING_ISO88595,    8, FALSE,   FALSE,  1,    1, "?" },
  { /* 6*/ CharInfo::ISO88596,       SQLCHARSETSTRING_ISO88596,    8, FALSE,   FALSE,  1,    1, "?" },
  { /* 7*/ CharInfo::ISO88597,       SQLCHARSETSTRING_ISO88597,    8, FALSE,   FALSE,  1,    1, "?" },
  { /* 8*/ CharInfo::ISO88598,       SQLCHARSETSTRING_ISO88598,    8, FALSE,   FALSE,  1,    1, "?" },
  { /* 9*/ CharInfo::ISO88599,       SQLCHARSETSTRING_ISO88599,    8, FALSE,   FALSE,  1,    1, "?" },
  { /*10*/ CharInfo::SJIS,           SQLCHARSETSTRING_SJIS,        4, TRUE,    FALSE,  1,    2, "?" },
  { /*11*/ CharInfo::UCS2,           SQLCHARSETSTRING_UCS2,        4, IF_WIDE, IF_WIDE,2,    2, "\xff\xfd" },
  { /*12*/ CharInfo::EUCJP,          SQLCHARSETSTRING_EUCJP,       5, TRUE,    FALSE,  1,    3, "?" },
  { /*13*/ CharInfo::BIG5,           SQLCHARSETSTRING_BIG5,        4, TRUE,    FALSE,  1,    2, "?" },
  { /*14*/ CharInfo::GB18030,        SQLCHARSETSTRING_GB18030,     7, FALSE,   FALSE,  1,    4, "?" },
  { /*15*/ CharInfo::UTF8,           SQLCHARSETSTRING_UTF8,        4, TRUE,    TRUE,   1,    4, "\xef\xbf\xbd" },
  { /*16*/ CharInfo::KSC5601,        SQLCHARSETSTRING_MB_KSC5601, 10, TRUE,    FALSE,  1,    2, "?" },
  { /*17*/ CharInfo::GB2312,         SQLCHARSETSTRING_GB2312,      6, TRUE,    FALSE,  1,    2, "?" },
  { /*18*/ CharInfo::GBK,            SQLCHARSETSTRING_GBK,         3, TRUE,    FALSE,  1,    2, "?" },
  { /*19*/ CharInfo::BINARY,         SQLCHARSETSTRING_BINARY,      6, TRUE,    TRUE,   1,    1, "?" },
};

#define SIZEOF_CS  (sizeof(mapCSArray)/sizeof(mapCS))

const char* CharInfo::getCharSetName(CharSet cs, NABoolean retUnknownAsBlank)
{
  if (cs >= CHARSET_MIN && cs <= CHARSET_MAX)
    {
      if (cs != CharInfo::UnknownCharSet)
	return mapCSArray[cs-CHARSET_MIN].name;
    }

  return retUnknownAsBlank ? "" : SQLCHARSETSTRING_UNKNOWN;
}

CharInfo::CharSet CharInfo::getCharSetEnum(const char* name)
{
  if (*name == '\0' || *name == ' ')		// fastpath: if name is empty
    return CharInfo::UnknownCharSet;		// or all blanks (begins w/ ' ')

  for (size_t i = 0; i < SIZEOF_CS; i++) {
    const mapCS *map = &mapCSArray[i];
    if (name == map->name)			// fastpath: pointers identical
      return map->cs;
    else {
      // Can't use plain old strcmp here, because we want both
      //	"SJIS"  and  "SJIS  "
      // to be matched (see smdio/CmColumnsRow.cpp).
      // ##Note that this will fail if given an Ansi delimited identifier
      // ##(in a USER-DEFINED collation name, of course) w/ an embedded space!
      // ##
      // ##As no user-defined collations are allowed in NSK Rel 1,
      // ##we are not fixing this now...
      //
      size_t len = map->namelen;
      if (strncmp(name, map->name, len) == 0 &&
          (name[len] == '\0' || name[len] == ' '))	//##fails if "My Coll"!
	return map->cs;
      }
    }

  // handle alias names for charsets, those are the exception
  const char *alias = SQLCHARSETSYNONYM_SQL_TEXT;
  size_t aliasLen = strlen(alias);

  if (strncmp(name, alias, aliasLen) == 0 &&
      (name[aliasLen] == '\0' || name[aliasLen] == ' '))
    {
      return CharInfo::SQL_TEXT;
    }

  return CharInfo::UnknownCharSet;
}

NABoolean CharInfo::isCharSetSupported(CharSet cs)
{
  if (cs >= CHARSET_MIN && cs <= CHARSET_MAX)
    {
      // Special for running regress/fullstack/TEST001 on NSK:
#ifdef _DEBUG
      if (IF_WIDE == FALSE &&
          mapCSArray[cs-CHARSET_MIN].maxBytesPerChar > 1 &&	// SJIS or UNICODE
          getenv("NCHAR_SJIS_DEBUG"))
        return TRUE;
#endif
      return mapCSArray[cs-CHARSET_MIN].supported;
    }

  return FALSE;
}

NABoolean CharInfo::isCharSetFullySupported(CharSet cs)
{
  if (cs >= CHARSET_MIN && cs <= CHARSET_MAX)
    {
      // Special for running regress/fullstack/TEST001 on NSK:
#ifdef _DEBUG
      if (IF_WIDE == FALSE &&
          mapCSArray[cs-CHARSET_MIN].maxBytesPerChar > 1 &&	// SJIS or UNICODE
          getenv("NCHAR_SJIS_DEBUG"))
        return TRUE;
#endif
      return mapCSArray[cs-CHARSET_MIN].fully_supported;
    }

  return FALSE;
}

NABoolean CharInfo::isHexFormatSupported(CharSet cs) {
  return ( (cs == CharInfo::ISO88591) || (cs == CharInfo::UNICODE) || 
           (cs == CharInfo::UTF8) ||
           (is_NCHAR_MP(cs)) ||
           (cs == CharInfo::BINARY)
         );
}

NABoolean CharInfo::isTerminalCharSetSupported(CharSet cs) {
  return ((cs == CharInfo::ISO88591) || (cs == CharInfo::SJIS)   ||
          (cs == CharInfo::EUCJP)    || (cs == CharInfo::BIG5)   ||
          (cs == CharInfo::GB18030)  || (cs == CharInfo::GB2312) ||
                                        (cs == CharInfo::GBK   ) ||
          (cs == CharInfo::KSC5601)  || (cs == CharInfo::UTF8)) ;
}

NABoolean CharInfo::isMsgCharSetSupported(CharSet cs) {
  return ( (cs == CharInfo::UTF8) || (cs == CharInfo::UNICODE) );
}

// see TESTCHARSET in CmpMain.cpp
void CharInfo::toggleCharSetSupport(CharSet cs)
{
#ifdef _DEBUG		
    size_t i;
    for (i = 0; i < SIZEOF_CS; i++)
      if (cs == mapCSArray[i].cs)
	break;
    cerr << "toggleCharSetSupport: " << getCharSetName(cs) << " ";
    if (cs == UnknownCharSet || i >= SIZEOF_CS)
      cerr << "*not* toggled: "<< (Int32)cs << ", " << i << endl;
    else {
      cerr << "toggled from " << mapCSArray[i].supported
		    << " to " << !mapCSArray[i].supported << endl;
      NABoolean *nonconstSupported = (NABoolean *)&mapCSArray[i].supported;
      *nonconstSupported = !*nonconstSupported;
    }
#endif
}

// for R2 FCS. 
CharInfo::CharSet CharInfo::getEncoding(const CharInfo::CharSet x)
{
   switch (x)
   {
     case CharInfo::ISO88591:
     case CharInfo::UNICODE:
        return x;
        break;

     case CharInfo::SJIS:
     case CharInfo::KANJI_MP:
     case CharInfo::KSC5601_MP:
        return CharInfo::ISO88591;
        break;

     default:
        return x;
        break;
   }
}

Int32 CharInfo::minBytesPerChar(CharSet cs)
{
  ComASSERT(cs >= CHARSET_MIN && cs <= CHARSET_MAX);

  return mapCSArray[cs-CHARSET_MIN].minBytesPerChar;
}

Int32 CharInfo::maxBytesPerChar(CharSet cs)
{
  ComASSERT(cs >= CHARSET_MIN && cs <= CHARSET_MAX);

  return mapCSArray[cs-CHARSET_MIN].maxBytesPerChar;
}

Int32 CharInfo::getFSTypeFixedChar(CharSet cs)
{
  if (cs == UCS2)
    return REC_BYTE_F_DOUBLE;

  return REC_BYTE_F_ASCII;
}


Int32 CharInfo::getFSTypeANSIChar(CharSet cs)
{
  if (cs == UCS2)
    return REC_BYTE_V_ANSI_DOUBLE;

  return REC_BYTE_V_ANSI;
}

const char* CharInfo::getReplacementCharacter(CharSet cs)
{
  ComASSERT(cs >= CHARSET_MIN && cs <= CHARSET_MAX);

  return mapCSArray[cs-CHARSET_MIN].replacementChar;
}

NABoolean CharInfo::isVariableWidthMultiByteCharSet(CharSet cs)
{
  if (cs >= CHARSET_MIN && cs <= CHARSET_MAX)
    {
      return (mapCSArray[cs-CHARSET_MIN].minBytesPerChar !=
              mapCSArray[cs-CHARSET_MIN].maxBytesPerChar);
    }

  return FALSE;
}

NABoolean 
CharInfo::checkCodePoint(const NAWchar* inputStr, Int32 inputLen, CharInfo::CharSet cs)
{
  if (!inputStr || (inputLen <= 0) ) return TRUE;

  if (cs == CharInfo::UNICODE) {
     for (Int32 i = 0; i < inputLen; i++) {
       if (!unicode_char_set::isValidUCS2CodePoint(inputStr[i]))
          return FALSE;
     }
     return TRUE;
  }
  return FALSE;
}



//****************************************************************************
// COLLATION stuff:  CollationInfo methods
//
//	The design here, particularly the 4-part name, is because we need to
//	allow the Parser to lookup names, and it has no notion of
//	applying defaults to a possibly qualified identifier
//	(and, builtin/predefined system collations are 1-part and must take
//	precedence!).  This should probably be changed so that Parser need
//	do no lookup at all, that collation names be resolved later, in Binder!
//****************************************************************************

CollationInfo::CollationInfo(CollHeap *h,
			     CharInfo::Collation co,
			     const char *name,
			     CollationFlags flags,
			     size_t *siz)	  /* array[SIZEARRAY_SIZE] */
: co_(co), flags_(flags)
{
  ComASSERT(name);
  namelen_ = strlen(name);	// allowed to be 0 if siz[] not passed in
  if (siz) {
    size_t cnt = siz[0];
    ComASSERT(cnt >= 0 && cnt < MAX_NAME_PARTS);
    ComASSERT(namelen_ > 0 && namelen_ == siz[1]);
    for (size_t off = 0; off < OFFSETARRAY_SIZE; off++) {
      synonymOffset_[off] = (off < cnt) ? siz[off+2] : 0;
    }
  } else
    synonymOffset_[0] = synonymOffset_[1] = synonymOffset_[2] = 0;
  if (flags_ & NO_ALLOC_AND_COPY_IN_CTOR)
    name_ = name;
  else {
    name_ = new (h) char[namelen_+1];
    strcpy((char *)name_, name);
  }
}




void CollationInfo::display() const
{
}
//
CollationDB::CollationDB(CollHeap *h)
  : CollationDBSupertype(h), heap_(h), refreshNeeded_(TRUE)
{
    if (this == CharInfo::builtinCollationDB_) return;
    if (cmpCurrentContext != NULL)
       cmpCurrentContext->getCollationDBList()->insert(this);
}

CollationDB::CollationDB(CollHeap *h, const CollationInfo *co, size_t count)
  : CollationDBSupertype(h), heap_(h), refreshNeeded_(!!count)
{ 
   while (count--) CollationDBSupertype::insert(co++);
   if (this == CharInfo::builtinCollationDB_) return;
   if (cmpCurrentContext != NULL)
      cmpCurrentContext->getCollationDBList()->insert(this);
}

CollationDB::~CollationDB()
{ 
   if (this == CharInfo::builtinCollationDB_) return;
   clearAndReset();
   cmpCurrentContext->getCollationDBList()->remove(this);
}


void CollationDB::display() const
{
}

void CollationDB::Display()
{
  CollationDBList *CDBlist = cmpCurrentContext->getCollationDBList();
  CollIndex i, n = CDBlist->entries();
  for (i = 0; i < n; i++)
    (*CDBlist)[i]->display();
}

//****************************************************************************
// COLLATION stuff:  CollationDB data and methods
//
// Collations may be simple 1-part names for system predefined collations;
// they are also allowed to be user-defined, hence qualified names
// (3-part Ansi, or 4-part NSK).
//
//	The insert methods and their static data are implemented in
//	../optimizer/SchemaDB.cpp instead of here, because
//	one method uses a QualifiedName, which is defined in ../optimizer --
//	#include and DLL-link problems occur if we try to implement
//	here in ../common.
//	The self-maintaining CDB-chain and the static CharInfo::getCollation*()
//	caller interface make this work.
//****************************************************************************

inline
CollationDB * CollationDB::nextCDB() const
{
  // If this is in the CDB chain [should always be true -- defensive prog'ing],
  // return the next CDB in the chain, if there is one.
  CollationDBList *CDBlist = cmpCurrentContext->getCollationDBList();

  CollIndex i = CDBlist->index((CollationDB *)this);
  if (i != NULL_COLL_INDEX)				// [defensive prog'ing]
    for (CollIndex n = CDBlist->entries(); ++i < n; )
      if ((*CDBlist)[i] && (*CDBlist)[i] != this)		// [defensive prog'ing]
        return (*CDBlist)[i];

  return NULL;
}

const CollationInfo* CollationDB::getCollationInfo(CharInfo::Collation co) const
{
  CollIndex i, n;
  n = entries();
  for (i = 0; i < n; i++)
    if (co == at(i)->co_)
      return at(i);

  CollationDB *next = nextCDB();
  return next ? next->getCollationInfo(co) : NULL;
}

const char* CollationDB::getCollationName(CharInfo::Collation co,
					  NABoolean retUnknownAsBlank) const
{
  if (co != CharInfo::UNKNOWN_COLLATION) {
    const CollationInfo *ci = getCollationInfo(co);
    if (ci) return ci->name_;
  }

  return retUnknownAsBlank ? "" : SQLCOLLATIONSTRING_UNKNOWN;
}

Int32 CollationDB::getCollationFlags(CharInfo::Collation co) const
{
  const CollationInfo *ci = getCollationInfo(co);
  if (ci) return ci->flags_;

  return CollationInfo::ALL_NEGATIVE_SYNTAX_FLAGS;
}

// We need the namlen arg here, unlike CharInfo::getCharSetEnum(),
// because whereas CHARSETs are SQL simple identifiers
// (must begin with a Latin letter, a regular not "delimited" identifier),
// COLLATIONs are SQL identifiers
// (can be delimited and contain spaces).
//
// So we must check that any spaces are *trailing* spaces only.
//
// We can't use plain old strcmp here, because we want both
//	"SJIS"  and  "SJIS  "
// to be matched.  The public caller CharInfo::getCollationEnum()
// inputs to us a correct namlen in either case (4, for the SJIS example).
//
CharInfo::Collation CollationDB::getCollationEnum(const char* name,
						  NABoolean formatNSK,
						  size_t namlen) const
{
  ComASSERT(namlen);

  CollIndex n = entries();
  for (CollIndex i = 0; i < n; i++) {
    const CollationInfo *map = at(i);

    if (name == map->name_)			// fastpath: pointers identical
      return map->co_;

    // If we want NSK format and this i'th name is not NSK, or
    // if we don't want NSK fmt and this name is NSK,
    // then skip this name.
    // Exception is that the builtin collations are always compared.
    if (map->co_ >= CharInfo::FIRST_USER_DEFINED_COLLATION &&
        formatNSK XOR HasMPLocPrefix(map->name_))
      continue;

    size_t off = 0;
    for (size_t j = 0; ; j++) {
      size_t len = map->namelen_ - off;
      if (len == namlen) {
        if (strncmp(name, &map->name_[off], len) == 0)
	  return map->co_;
        else break;
      }
      if (len < namlen) break;
      if (j == CollationInfo::OFFSETARRAY_SIZE) break;
      off = map->synonymOffset_[j];
      if (off == 0) break;
    } // loop j

  } // loop i

  CollationDB *next = nextCDB();
  return next ? next->getCollationEnum(name, formatNSK, namlen)
  	      : CharInfo::UNKNOWN_COLLATION;

} // CollationDB::getCollationEnum()

//****************************************************************************
// COLLATION stuff:  CharInfo methods
//****************************************************************************

#define STATIC_STR	CollationInfo::NO_ALLOC_AND_COPY_IN_CTOR
#define STATIC_NEG	CollationInfo::ALL_NEGATIVE_PLUS_STATIC
static const CollationInfo mapCOArray[] = {
  CollationInfo(NULL, CharInfo::DefaultCollation,   SQLCOLLATIONSTRING_DEFAULT,
  			STATIC_STR),
  CollationInfo(NULL, CharInfo::CZECH_COLLATION,   SQLCOLLATIONSTRING_CZECH,
  			STATIC_STR),
  CollationInfo(NULL, CharInfo::CZECH_COLLATION_CI,   SQLCOLLATIONSTRING_CZECH_CI,
  			STATIC_STR),
  CollationInfo(NULL, CharInfo::UNKNOWN_COLLATION,  SQLCOLLATIONSTRING_UNKNOWN,
  			STATIC_NEG)
};

#define SIZEOF_CO (sizeof(mapCOArray)/sizeof(CollationInfo))

const CollationDB *CharInfo::builtinCollationDB_ = NULL;

CharInfo::Collation CharInfo::getCollationEnum(const char* name,
					       NABoolean formatNSK,
					       size_t namlen)
{
  if (namlen == 0)
    namlen = strlen(name);
  else {
    const char *n = &name[namlen-1];
    for (;
         name < n && (*n == ' ' || *n == '\0');
	 n--)
      ;
    namlen = (*n == ' ' || *n == '\0') ? 0 : n - name + 1;
  }
  if (namlen == 0)				// fastpath: if name is empty
    return CharInfo::UNKNOWN_COLLATION;

  // Collapse any nonzero formatNSK to single bit, for XOR
  return builtinCollationDB()->getCollationEnum(name, !!formatNSK, namlen);
}

const char* CharInfo::getCollationName(Collation co,
				       NABoolean retUnknownAsBlank)
{
  return builtinCollationDB()->getCollationName(co, retUnknownAsBlank);
}

Int32 CharInfo::getCollationFlags(Collation co)
{
  return builtinCollationDB()->getCollationFlags(co);
}

//****************************************************************************
// COERCIBILITY stuff
//****************************************************************************

const char* CharInfo::getCoercibilityText(Coercibility ce)
{
  // These are not keywords, not tokens, not part of Ansi syntax.
  // They are part of Ansi concepts; cf. Ansi 4.2.3.
  switch (ce) {
    case IMPLICIT:		return "implicit";
    case EXPLICIT:		return "explicit";
    case COERCIBLE:		return "coercible";
    case NO_COLLATING_SEQUENCE:	return "no-collating-sequence";
    default:			return "unknown";
  }
}

// "Which coercibility wins?"
// Returns 0 if they're equal, 1 if the first one wins, 2 if the second.
// This follows the strict Ansi precedence of
//      COERCIBLE < IMPLICIT < NO_COLLATING_SEQUENCE < EXPLICIT
//
// ## (As an aside, note that CharType::computeCoAndCo()
// ## could be pulled out into a static CharInfo:: method placed here.)
//
Int32 CharInfo::compareCoercibility(CharInfo::Coercibility ce1,
				  CharInfo::Coercibility ce2)
{
  if (ce1 == ce2) return 0;

  if (ce1 == CharInfo::COERCIBLE) return 2;		// 1 yields to 2
  if (ce2 == CharInfo::COERCIBLE) return 1;		// 2 yields to 1

  if (ce1 == CharInfo::EXPLICIT) return 1;
  if (ce2 == CharInfo::EXPLICIT) return 2;

  if (ce1 == CharInfo::NO_COLLATING_SEQUENCE) return 1;
  if (ce2 == CharInfo::NO_COLLATING_SEQUENCE) return 2;

  ComASSERT(FALSE);			// ceN IMPLICIT already handled above!
  return -1;
}


//****************************************************************************
// LOCALE stuff
//****************************************************************************

const char* const CharInfo::localeCharSet_ = NULL;

Lng32 CharInfo::findLocaleCharSet()
{
  return SQLCHARSETCODE_ISO88591;


}

const char* CharInfo::getLocaleCharSetAsString()
{
   if (!localeCharSet_) {
      switch ( findLocaleCharSet() ) {
        case SQLCHARSETCODE_ISO88591:
          return SQLCHARSETSTRING_ISO88591;

        case SQLCHARSETCODE_UCS2:
          return SQLCHARSETSTRING_UNICODE;

        case SQLCHARSETCODE_SJIS:
          return SQLCHARSETSTRING_SJIS;

        default:
          return SQLCHARSETSTRING_UNKNOWN;
     }
   } else
     return localeCharSet_;

   return SQLCHARSETSTRING_UNKNOWN;
}

Int32 CharInfo::getTargetCharTypeFromLocale()
{


  return REC_SBYTE_LOCALE_F;
}

 

Int32 CharInfo::getMaxConvertedLenInBytes(CharSet sourceCS, 
                                          Int32   sourceLenInBytes,
                                          CharSet targetCS)
{
  if (sourceCS == targetCS)
    {
      // trivial case, no conversion
      return sourceLenInBytes;
    }
  else if (targetCS == UTF8)
    {
      // optimize some cases where we can exploit knowledge about
      // the UTF-8 encoding

      if (sourceCS == UCS2)
        {
          // Worst case is each 2 byte UCS2 char resulting in a 3 byte
          // UTF8 char. Note that no UCS2 char requires a 4 byte UTF8
          // representation.
          // 2 UTF-16 surrogate pairs (4 bytes) will be translated to
          // a 4-byte UTF-8 character, which is not the worst case.
          return 3 * sourceLenInBytes/2;
        }

      if (sourceCS == ISO88591)
        {
          // Worst case is all upper-half ISO characters, which
          // result in 2 byte UTF-8 characters. No ISO8859-1 character
          // takes up more than 2 bytes in UTF-8.
          return 2 * sourceLenInBytes;
        }
    }

  // General case, assume max number of chars in input,
  // and all of them convert to the longest output char.
  // NOTE: This also works for UTF8 to UCS2 conversions
  // and back, even though those conversions really
  // treat UCS2 as UTF16.
  return ((sourceLenInBytes/minBytesPerChar(sourceCS)) *
          maxBytesPerChar(targetCS));
}

const CollationDB *CharInfo::builtinCollationDB()
{
   if (CharInfo::builtinCollationDB_ != NULL)
      return CharInfo::builtinCollationDB_;
   globalSemaphore.get(); 
   if (CharInfo::builtinCollationDB_ != NULL) 
   {
      globalSemaphore.release();
      return CharInfo::builtinCollationDB_;
   }
   CharInfo::builtinCollationDB_ = new CollationDB(NULL, mapCOArray, SIZEOF_CO);
   globalSemaphore.release();
   return CharInfo::builtinCollationDB_;
}

