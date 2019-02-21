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
* File:         ULexer.cpp
* Description:  Unicode lexical scanner for arkcmp SQL parser
*
*
* Created:      4/15/98
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "Platform.h"

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Use a reserved UCS-2 character (but not the last one) as EOF substitute
#define WEOF (NAWchar)(0xFFEF)
#include  "arkcmp_parser_defs.h"
#undef    SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS

//  Need parser global flags to determine if parsing (lexing) SQL/MP
//  Stored Text. MP Allows double quotes (as well as single quotes)
//  for string literals.  MP Allows a space after the char set name in
//  string literals.
//
#undef    SQLPARSERGLOBALS_FLAGS
#define   SQLPARSERGLOBALS_FLAGS
#undef    SQLPARSERGLOBALS_LEX_AND_PARSE
#define   SQLPARSERGLOBALS_LEX_AND_PARSE
#undef    SQLPARSERGLOBALS_NADEFAULTS
#define   SQLPARSERGLOBALS_NADEFAULTS
#undef    SQLPARSERGLOBALS_NAMES_AND_TOKENS
#define   SQLPARSERGLOBALS_NAMES_AND_TOKENS
#include "SqlParserGlobals.h"

// Forward references and includes for Y.tab.h (sqlparser.h)
class DatetimeQualifier;
class ExprNodePtrList;
class ForUpdateSpec;
class IntervalQualifier;
class OldNewNames;
class PairOfUnsigned;
class PipelineDefPtrList;  // Forward references for MV classes.
class PipelineDef;         // MV --
class QualNamePtrList;
class NRowsClause;
class CatchupClause;
class PipelineClause;
class DeltaDefinitionPtrList;
class DeltaDefinition;
class IncrementalRefreshOption;
class RecomputeRefreshOption;
class DeltaOptions;
class DeltaDefLogs;  
class DeltaDefRangeLog; 
class DeltaDefIUDLog;
class IUDStatistics;
class IntegerList;


#include "CharType.h"
#include "charinfo.h"
#include "conversionHex.h"
#include "ComSmallDefs.h"
#include "ComTransInfo.h"
#include "ComUnits.h"
#include "HvTypes.h"
#include "ElemDDLConstraintRI.h"
#include "ElemDDLParamName.h"
#include "ElemDDLPartition.h"
#include "ElemDDLPassThroughParamDef.h"
#include "ElemDDLQualName.h" // OZ
#include "ElemDDLColRefArray.h"
#include "RelScan.h"
#include "RelUpdate.h"
#include "ItemLog.h"
#include "StmtDMLSetTransaction.h"
#include "StmtDDLAlterMV.h" // (using an enum defined in that file)
#include "StmtDDLAlterMVRGroup.h" // Mv refresh groups  ( OZ ) - new enum
#include "ElemDDLCreateMVOneAttributeTableList.h" // MV OZ 
#include "ElemDDLFileAttrRangeLog.h"
#include "ElemDDLFileAttrMvsAllowed.h"
#include "ElemDDLFileAttrMvAudit.h"
#include "SqlParserAux.h"
#include "StmtDDLCreateMV.h"
#include "ElemDDLHbaseOptions.h"
#include "StmtDDLCommentOn.h"

// Need the definition of the Parsers Union.  If this is not defined,
// sqlparser.h will only define the Tokens.
//
#ifndef INCLUDE_UNION
#define INCLUDE_UNION
#endif
#include "sqlparser.h"   // Angled brackets are intentional here
#define SQLPARSER_H

extern Int32 tokval;                      // defined by yacc, for lex only
extern THREAD_P NABoolean HexStringLiteralNotAllowed;
extern THREAD_P NABoolean turnUnknownCharSetToISO88591;

#include "ParKeyWords.h"
#include "ulexer.h"
#include "wstr.h"
#include "str.h"
#include "CliSemaphore.h"

#ifdef DEBUG
//NGG For the time being  #define dbgprintf(x)	printf(x,yytext_)
  #define dbgprintf(x)	
  #define DBGMSG(x)	x
#else
  #define dbgprintf(x)
  #define DBGMSG(x)	NULL
#endif

#define YY_END_OF_BUFFER_CHAR 0

typedef struct yy_buffer_state *YY_BUFFER_STATE;

typedef UInt32 yy_size_t;

struct yy_buffer_state
{
  NAWchar *yy_ch_buf;   /* input buffer */
  NAWchar *yy_buf_pos;  /* current position in input buffer */

  /* Number of characters read into yy_ch_buf, not including EOB
   * characters.
   */
  Int32 yy_n_chars;
  Int32 yy_ch_count;
};

// Report a fatal error.
#define YY_EXIT_FAILURE 2
/* UR2-CNTNSK */
#define YY_FATAL_ERROR(msg) \
  do { fprintf(stderr, "%s\n", msg); exit(YY_EXIT_FAILURE); } while(0)
//#endif

THREAD_P LimitedStack *inJoinSpec = NULL;       // can handle <STACK_LIMIT> nested Joins

void yyULexer::yyULexer_ctor(const NAWchar *str, Int32 charCount)
{
  input_pos_ = 0;
  yy_init_ = 0;
  yy_U_debug_ = 0;

  YY_BUFFER_STATE b = yy_current_buffer_ = new (PARSERHEAP()) yy_buffer_state;
// UR2-CNTNSK
  if ( !b ) YY_FATAL_ERROR( "out of dynamic memory in yyULexer()" );

  if (inJoinSpec == NULL)
    inJoinSpec = new LimitedStack;

  /* yy_ch_buf has to be 2 characters longer than the size given because
   * we need to put in 2 end-of-buffer characters.
   */
  Int32 buf_size = charCount * BYTES_PER_NAWCHAR;
  b->yy_ch_buf = b->yy_buf_pos = yy_c_buf_p_ = new (PARSERHEAP()) NAWchar[charCount+2];
  if ( ! b->yy_ch_buf )
    // UR2-CNTNSK
    YY_FATAL_ERROR( "out of dynamic memory in yyULexer()" );

  b->yy_n_chars = charCount;

  // copy str because yylex will modify it during scanning
  memcpy(b->yy_ch_buf, str, buf_size);
  b->yy_ch_buf[charCount] = b->yy_ch_buf[charCount+1] = YY_END_OF_BUFFER_CHAR;

  yy_load_buffer_state();

  returnAllChars_ = FALSE;

  // Initialize the key word table.  This will load in any changes to
  // the table and sort it if these things have not already been done.
  //
  if (!ParKeyWords::keyWordTableSorted())
    {
      CLISemaphore * sema = GetCliGlobals()->getSemaphore();
      sema->get();

      ParKeyWords::initKeyWordTable();

      sema->release();
    }
}

yyULexer::yyULexer(const NAWchar *str, Int32 charCount)
{ yyULexer_ctor(str, charCount); }

yyULexer::yyULexer(const NAWchar *str, size_t charCount)
{
  CMPASSERT(charCount <= INT_MAX);
  yyULexer_ctor(str, (Int32)charCount);
}


yyULexer::~yyULexer()
{
  if ( yy_current_buffer_ )
    {
      if ( yy_current_buffer_->yy_ch_buf ) 
        NADELETEBASIC(yy_current_buffer_->yy_ch_buf, PARSERHEAP());

      NADELETE(yy_current_buffer_, yy_buffer_state, PARSERHEAP());
      yy_current_buffer_ = 0;
    }
}


void yyULexer::yy_load_buffer_state()
{
  yy_n_chars_ = yy_current_buffer_->yy_n_chars;

  yytext_ = yy_c_buf_p_ = yy_current_buffer_->yy_buf_pos;
  yy_hold_char_ = *yy_c_buf_p_;
}


// Unicode character classification functions. 
// We may need an NSK version of these functions.

// For now, we only support ISO 8859-1 for token char set
inline static Int32 U_isdigit(NAWchar c)
{
// UR2-CNTNSK
  return (L'0' <= c && c <= L'9');
}

inline static Int32 U_isalphaund(NAWchar c)
{
  return isAlpha8859_1(c) || c == L'_';
}

inline static Int32 U_isalnumund(NAWchar c)
{
  return isAlNum8859_1(c) || c == L'_';
}

inline static Int32 U_isspace(NAWchar c)
{
  return isSpace8859_1(c);
}

inline static Int32 U_isAsciiAlpha(NAWchar c)
{
  return (c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z');    
}

inline static Int32 U_isAsciiAlNum(NAWchar c)
{
  return (c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z') ||
         (c >= L'0' && c <= L'9');
}

inline static Int32 U_isAsciiAlNumUnd(NAWchar c)
{
  return U_isAsciiAlNum(c) || c == L'_';
}

inline static Int32 U_isAsciiAlNumUndHyphen(NAWchar c)
{
  // The hyphen should be allowed only in the COBOL preprocessing mode! ##
  return U_isAsciiAlNumUnd(c) || c == L'-';
}


// The old (flex-based) lexer was required to recognize the following
// compound tokens:
//   FOR BROWSE
//   FOR CLEAN
//   FOR READ
//   FOR REPEATABLE
//   FOR SERIALIZABLE
//   FOR STABLE
//   NO DEFAULT
//   NOT IN
//   NOT BETWEEN
//   REPEATABLE ACCESS
//   SERIALIZABLE ACCESS
//   UNION JOIN
// You can get rid of these compound tokens only if sqlparser.y
// does not get additional shift-reduce/reduce-reduce conflicts.
// The flex-based lexer was also recognizing the following Cobol tokens:
//   [Cc][Aa][Ll][Ll]{B}{CASED_IDENTIFIER}
//   [Gg][Oo]{B}?[Tt][Oo]{B}{CASED_IDENTIFIER}
//   [Pp][Ee][Rr][Ff][Oo][Rr][Mm]{B}{CASED_IDENTIFIER}
//   [Pp][Ii][Cc]([Tt][Uu][Rr][Ee])?{B}(([Ss]?{NINESPEC})|{XSPEC})
// where
//   B          [ \t\n]+
//   NINES      ((9(\({B}?[0-9]+{B}?\))?)+)
//   XSPEC      (([Xx](\({B}?[0-9]+{B}?\))?)+)
//   NINESPEC   ({NINES}|({NINES}?[Vv]{NINES}?))
//   CASED_IDENTIFIER     [A-Za-z0-9_][A-Za-z0-9_-]*
// In addition, we need to recognize the new compound tokens:
//   _[Uu][Nn][Ii][Cc][Oo][Dd][Ee]'([^']|"''")*'
//   _[Ii][Ss][Oo]8859[1-9]'([^']|"''")*'

/////////////////////////////////////////////////////////////////////
// The Trafodion keyword table has move to parser/ParKeyWords.{h,cpp}.
// See this file for instructions on how to add new key words.
/////////////////////////////////////////////////////////////////////

inline Int32 yyULexer::endOfBuffer() 
{  return currChar_ >= &yy_current_buffer_->yy_ch_buf[yy_n_chars_] ||
    yy_n_chars_ <= 0;
}


inline NAWchar yyULexer::peekChar() 
{
  if (endOfBuffer()) 
    return WEOF;
  else
    return *currChar_;
}

static NAWString *removeConsecutiveWQuotes(const NAWchar *s,
                                           Int32 len,
                                           NAWchar quote,
                                           YYSTYPE *lvalp)
{
  lvalp->stringval_with_charset.charSet_ = CharInfo::UCS2;
  lvalp->stringval_with_charset.bytesPerChar_ = BYTES_PER_NAWCHAR;
  lvalp->stringval_with_charset.setflags_Bit(StringvalWithCharSet::eUSE_wstringval_FIELD_BIT_MASK);

  NAWString *r = new (PARSERHEAP()) NAWString(PARSERHEAP());
  if (!s || len <= 0) return r; // lvalp->wstringval = lvalp->stringval_with_charset.wstringval = r;

  for (Int32 x = 0; x < len; x++)
    {
      if (s[x] == quote)
        {
          if (s[x+1] != quote) break;
          x++;
        }
      r->append(s[x]);
    }
  return r; // lvalp->wstringval = lvalp->stringval_with_charset.wstringval = r;
}


// convert a unicode wide char to a multibyte char
static Int32 my_wctomb(char *s, Int32 s_len, NAWchar wc, CharInfo::CharSet cs)
{
/* UR2-CNTNSK */
  return UnicodeStringToLocale(cs, &wc, 1, s, s_len, FALSE /* no null terminator*/, FALSE /* in substitute */);
}


// The argument literalPrefixCS is useful when the parser's character set is Unicode. 
// In this case, we really need to know how to translate a non-UCS2 literal back to its
// single-/multi-byte form.
static NAString *removeConsecutiveQuotes(const NAWchar *s,
                                         Int32 len,
                                         NAWchar quote, 
                                         YYSTYPE *lvalp,
                                         CharInfo::CharSet literalPrefixCS
                                         )
{
  NAString *r = new (PARSERHEAP()) NAString(PARSERHEAP());

  NAString inferCharSetSetting;
  NABoolean isInferCharSetEnabled(getCharSetInferenceSetting(inferCharSetSetting));

  lvalp->stringval_with_charset.resetflags_Bit(StringvalWithCharSet::eUSE_wstringval_FIELD_BIT_MASK); // i.e., use stringval field
  lvalp->stringval_with_charset.setflags_Bit ( StringvalWithCharSet::eSTR_LIT_PREFIX_SPEC_BIT_MASK
                                             , literalPrefixCS != CharInfo::UnknownCharSet
                                             );
  lvalp->stringval_with_charset.setflags_Bit ( StringvalWithCharSet::eINFER_CS_UNKNOWN_CS_BIT_MASK
                                             , ( isInferCharSetEnabled
                                                 && literalPrefixCS == CharInfo::UnknownCharSet // the QUOTED_STRING case
                                                 && turnUnknownCharSetToISO88591 == FALSE )
                                             );

  //
  // If no input, we can just return (r) at this point.
  // If len == 1 AND literalPrefixCS is CharInfo::UnknownCharSet
  // then s will point to ending quote mark, so the input is
  // actually a zero-length quoted string and, in this case, we
  // want to treat it as an ISO88591 zero-length quoted string, so
  // again we just return (r).
  //
  if (!s || len <= 0 ||
         ( (len == 1) && (literalPrefixCS == CharInfo::UnknownCharSet) )
     )
  {
    CharInfo::CharSet cs = literalPrefixCS;
    if (cs == CharInfo::UnknownCharSet)
      cs = CharInfo::ISO88591; // an empty string can have any character set attribute
    lvalp->stringval_with_charset.charSet_ = cs;
    lvalp->stringval_with_charset.bytesPerChar_ = CharInfo::maxBytesPerChar(cs);
    return r; // lvalp->stringval = lvalp->stringval_with_charset.stringval = r;
  }

  CharInfo::CharSet inputCS = (CharInfo::CharSet)SqlParser_CurrentParser->charset_;
  CharInfo::CharSet targetCS = inputCS;


  if (literalPrefixCS != CharInfo::UnknownCharSet) // the TOK_SBYTE_LITERAL case
  {
    targetCS = literalPrefixCS;
    PARSERASSERT(targetCS == CharInfo::ISO88591 || targetCS == CharInfo::UTF8
                 || targetCS == CharInfo::BINARY);
    //
    // Note that an _ISO88591'string-literal' may contain Western European characters
    // (i.e., 0 <= code_points < 255).
  }
  else // literalPrefixCS == CharInfo::UnknownCharSet - i.e., the QUOTED_STRING case
  {
    PARSERASSERT(targetCS == CharInfo::UTF8);

    // Check if the string is all ASCII characters (i.e., 0 <= code_points <= 127)
    {
       NABoolean containsOnlyASCII = na_wcs_has_only_ascii_chars(s, len/*in_NAWchars*/);
       //
       // If all ASCII characters, then mark the quoted string as ISO88591
       // instead of UTF8 as that will be (A) simpler for any translations or
       // comparisons later and (B) more backward compatible (hence avoiding
       // many potential regression test failures.)
       //
       if ( containsOnlyASCII == TRUE )
          targetCS = CharInfo::ISO88591;
    }
  } // string literal without character set prefix (i.e. QUOTED_STRING)

  NAString* pTempStr =  NULL;
  pTempStr = unicodeToChar(const_cast<NAWchar*>(s), len,
                           static_cast<Lng32>(targetCS), PARSERHEAP());
  if ( pTempStr == NULL ) // conversion failed
  {
    // The string argument contains characters that cannot be converted.
    *CmpCommon::diags() << DgSqlCode(-8413);
    delete r;
    return NULL; // lvalp->stringval = lvalp->stringval_with_charset.stringval = NULL;
  }
  else // conversion was successful
  {
    // --- Remove consecutive quotes ---

    Int32 iConvStrLitLen = pTempStr->length();
    unsigned char quoteChar = (unsigned char)quote; // ok to cast - quote is 0x0022 or 0x0027
    unsigned char *pConvStrLit = (unsigned char *)pTempStr->data();
    for (Int32 i = 0; i < iConvStrLitLen; i++)
    {
      if (pConvStrLit[i] == quoteChar)
      {
        if (pConvStrLit[i+1] != quoteChar)
          break; // pConvStrLit[i] contains the terminating delimiting quote
        i++;
      }
      r->append(pConvStrLit[i]);
    } // for

    if ( literalPrefixCS == CharInfo::UnknownCharSet ) // the QUOTED_STRING case
      PARSERASSERT( targetCS == CharInfo::UTF8 || ( targetCS == CharInfo::ISO88591 &&
                                                    NAStringHasOnly7BitAsciiChars(*pTempStr)));
    delete pTempStr;
    pTempStr = NULL;

    lvalp->stringval_with_charset.charSet_ = targetCS;
    lvalp->stringval_with_charset./*max*/bytesPerChar_ = CharInfo::maxBytesPerChar(targetCS);

    return r; // lvalp->stringval = lvalp->stringval_with_charset.stringval = r;
  } //  pTempStr != NULL

  delete r;
  return NULL; // lvalp->stringval = lvalp->stringval_with_charset.stringval = NULL;

}

static NAString *removeHostVarQuotes(const NAWchar *s, Int32 len)
{
  // :"aB" becomes :aB
  // :"a""B" becomes :a""B
  // not :a"B which is what it SHOULD become according to Ansi syntax 12.3 + 5.4
  // but who cares?
  // First, no host language currently allows doublequotes in its identifiers;
  // second, no ambiguity will result if users use only the Ansi-standard syntax
  // of :hv in procedure param decls instead of Tdm-extension omitting colon.
  // Fix this ("" to become ") in future only if some language needs it!

  NAString *r = new (PARSERHEAP()) NAString(PARSERHEAP());
  if (!s || len <= 0 || s[0] != NAWchar(':')) return r;

  CharInfo::CharSet targetCS = (CharInfo::CharSet)SqlParser_CurrentParser->charset_;
  if ( targetCS == CharInfo::UNICODE ) targetCS = CharInfo::ISO88591;

  // copy ':'
  char ch[8]; Int32 x, y, count = my_wctomb(ch, 8, s[0], targetCS);
  if (count > 8) count = 8;
  for (y=0; y < count; y++) r->append(ch[y]);

  for (x = 2; // elide first "
       x < len; x++) 
    {
      if (s[x] == NAWchar('"'))
        {
          if (s[x+1] != NAWchar('"')) break;
          x++;
        }
      if (!s[x])
        r->append('\0');
      else 
        {
          count = my_wctomb(ch, 8, s[x], targetCS);
          if (count == 0) {
            NADELETE(r, NAString, PARSERHEAP());
            return NULL;
          }
          if (count > 4) count = 4;
          for (y=0; y < count; y++) r->append(ch[y]);
        }
    }
  return r;
}


// encapsulate lexer actions associated with recognizing one of
// {IDENTIFIER, Trafodion keyword, compound keyword, compound Cobol token,
//  approx numeric, exact numeric with scale, exact numeric no scale}:
//  - updating the ParScannedToken queue
//  - setting the yylval token value
//  - echoing the token string if debugging is on
//
Int32 yyULexer::setStringval(Int32 tokCod, const char *dbgstr, YYSTYPE *lvalp)
{
  addTokenToGlobalQueue();
  dbgprintf(dbgstr);

  Lng32 targetMBCS = (Lng32)ParScannedInputCharset;

  lvalp->stringval = unicodeToChar
    (YYText(),YYLeng(),targetMBCS,PARSERHEAP());

  if (
      (tokCod == DELIMITED_IDENTIFIER || tokCod == IDENTIFIER ||
       tokCod == BACKQUOTED_IDENTIFIER)
      && targetMBCS != CharInfo::ISO88591
      )
  {
    NAString* tempstr = lvalp->stringval; // targetMBCS == ParScannedInputCharset
    if (tempstr == NULL)
      return invalidStrLitNonTranslatableChars(lvalp);

    Int32 TSLen = (Int32)tempstr->length();
    Int32 YYLen = (Int32)YYLeng();
    if(TSLen != YYLen){  // need offset of ORIGINAL string
      ParScannedTokens->updateInputLen(TSLen);
      ParScannedTokenOffset = ParScannedTokenOffset + TSLen - YYLen;
    }
  }
  return tokCod;
}

Int32 yyULexer::setTokval(Int32 tokCod, const char *dbgstr, YYSTYPE *lvalp)
{
  addTokenToGlobalQueue();
  dbgprintf(dbgstr);
  lvalp->tokval_plus_yytext.yytext = YYText();
  lvalp->tokval_plus_yytext.yyleng = YYLeng();
  lvalp->tokval = tokCod;
  return tokCod;
}

Int32 yyULexer::aStringLiteralWithCharSet(CharInfo::CharSet cs,
					const NAWchar *str, 
                                        Int32 len,
                                        NAWchar quote,
                                        YYSTYPE *lvalp)
{
  addTokenToGlobalQueue();
  dbgprintf(DBGMSG("String literal %s\n"));
  if ( len != 0 &&
       ParScannedInputCharset != CharInfo::ISO88591 &&
       str[0] != 255 ) { // do not check if processing the mask
      NAString* tempstr = unicodeToChar
        (str, len, ParScannedInputCharset,PARSERHEAP());
      if (tempstr == NULL)
        return invalidStrLitNonTranslatableChars(lvalp);
      ParScannedTokenOffset += ((Int32)tempstr->length() - len);
      ParScannedTokens->updateInputLen((Int32)tempstr->length());
  }

  // a "mini-cache" to avoid proc call, for perf
  static THREAD_P CharInfo::CharSet cachedCS  = CharInfo::UnknownCharSet;
  static THREAD_P Int32 cachedBPC = 1;
  if (cachedCS != cs) {
    cachedCS = cs;
    cachedBPC = CharInfo::maxBytesPerChar(cs);
  }

  lvalp->stringval_with_charset.charSet_ = cs;
  lvalp->stringval_with_charset.bytesPerChar_ = cachedBPC;
  lvalp->stringval_with_charset.setflags_Bit ( StringvalWithCharSet::eSTR_LIT_PREFIX_SPEC_BIT_MASK
                                             , cs != CharInfo::UnknownCharSet
                                             );

  // What we are really asking is whether the cs is multi-single-byte
  // or multi-double-byte.
  if (cachedBPC == 1 || CharInfo::is_NCHAR_MP(cs)
      || cs == CharInfo::UTF8
      || cs == CharInfo::SJIS 
      || cs == CharInfo::UnknownCharSet
      ) {
    lvalp->stringval = removeConsecutiveQuotes(str, len, quote, lvalp, cs);
    PARSERASSERT(lvalp->stringval_with_charset.isstringvalUsed());
    return (lvalp->stringval) ? TOK_SBYTE_LITERAL : invalidStrLitNonTranslatableChars(lvalp);
  }
  else {
    lvalp->wstringval = removeConsecutiveWQuotes(str, len, quote, lvalp);
    lvalp->stringval_with_charset.setflags_Bit(StringvalWithCharSet::eUSE_wstringval_FIELD_BIT_MASK);
    return (lvalp->wstringval) ? TOK_MBYTE_LITERAL : invalidStrLitNonTranslatableChars(lvalp);
  }
}

Int32 yyULexer::aHexStringLiteralWithCharSet(CharInfo::CharSet cs,
					const NAWchar *str, 
                                        Int32 len,
                                        NAWchar quote,
                                        YYSTYPE *lvalp)
{
  addTokenToGlobalQueue();
  dbgprintf(DBGMSG("String literal %s\n"));

  CMPASSERT(cs != CharInfo::UnknownCharSet);

  // a "mini-cache" to avoid proc call, for perf
  static THREAD_P CharInfo::CharSet cachedCS  = CharInfo::UnknownCharSet;
  static THREAD_P Int32 cachedBPC = 1;
  if (cachedCS != cs) {
    cachedCS = cs;
    cachedBPC = CharInfo::maxBytesPerChar(cs);
  }

  lvalp->stringval_with_charset.charSet_ = cs;
  lvalp->stringval_with_charset.bytesPerChar_ = cachedBPC;
  lvalp->stringval_with_charset.setflags_Bit ( StringvalWithCharSet::eSTR_LIT_PREFIX_SPEC_BIT_MASK
                                             , cs != CharInfo::UnknownCharSet
                                             );
  lvalp->stringval_with_charset.setflags_Bit(StringvalWithCharSet::eHEX_STRING_LITERAL_BIT_MASK);

  void* result = NULL;
                         
  enum hex_conversion_code code = 
     verifyAndConvertHex(str, len, quote, cs, PARSERHEAP(), result);

  switch ( code ) {

     case SINGLE_BYTE:
     {
        NAString* s = (NAString*)result;
        if (s) {
          lvalp->stringval = s;
          PARSERASSERT(lvalp->stringval_with_charset.isstringvalUsed());
          return TOK_SBYTE_LITERAL;
        }
     }
     break;
                           
     case DOUBLE_BYTE:
     {
        NAWString* ws = (NAWString*)result;
        if (ws) {
          lvalp->wstringval = ws;
          lvalp->stringval_with_charset.setflags_Bit(StringvalWithCharSet::eUSE_wstringval_FIELD_BIT_MASK);
          return TOK_MBYTE_LITERAL;
        }
                           
     }
     break;

     case NOT_SUPPORTED:
        // Error 3401
        // Hexadecimal representation of string literals associated
        // with the specified character set not yet supported.
         *SqlParser_Diags << DgSqlCode(-3401) <<
                             DgString0(CharInfo::getCharSetName(cs));
         return invalidHexStrLit(lvalp);
         break;

     case INVALID_CODEPOINTS:
        // 3400: invalid code points
       *SqlParser_Diags << DgSqlCode(-3400) << DgString0(CharInfo::getCharSetName(cs));
       break;

  case INVALID:
    // 3402: invalid hex format
   	*SqlParser_Diags << DgSqlCode(-3402) 
                     << DgString0(CharInfo::getCharSetName(cs));
    break;

  case CONV_FAILED:
  default:
    return invalidHexStrLit(lvalp);
    break;
  }

  //  // specified by  _charsetname'([0-9, a-f, A-F])*'
  //
  //  // ISO88591:               hex digits per char = 2
  //  // UCS2/KSC5601/KANJI:     hex digits per char = 4
  //  int hexPerChar = 2 * CharInfo::maxBytesPerChar(cs);
  //
  //  // The following while loop recognizes regular expression:
  //  // space* [non-space]\{hexPerChar\} space*
  //  // 
  //  // Examples of legal hexdecimal literals. 
  //  //       x' 98 FF  F0' (ISO88591)
  //  //       x' 98FF F0' (ISO88591)
  //  //       _ucs2 x' 98FF 3dF0' (UCS2)
  //  // 
  //  // Examples of illegal hexdecimal literals. 
  //  //       x' 98F F  F0' (ISO88591)
  //  //       _ucs2 x'9FF 3dF0' (UCS2)
  //  int i = 0;
  //  while ( i<len ) {
  //     if ( str[i] != L' ' ) {
  //        // at least hexPerChar non-space chars should be present, starting at i
  //        for ( int j=0; j<hexPerChar; j++ )
  //        {
  //          if ( i >= len || ! isHexDigit8859_1(str[i++]) ) {
  //             // invalid format
  //   	     *SqlParser_Diags << DgSqlCode(-3402) 
  //                              << DgString0(CharInfo::getCharSetName(cs));
  //   	     return invalidHexStrLit(lvalp);
  //          } 
  //        }
  //     } else i++;
  //  }
  //
  //  // remove spaces
  //  const NAWchar *tmpStr = removeWSpaces(str, len, quote)->data();
  //
  //  // convert to actual string literal
  //  switch ( cs ) {
  //    case CharInfo::KANJI_MP:
  //    case CharInfo::KSC5601_MP:
  //    case CharInfo::ISO88591:
  //      {
  //        NAString* s = convHexToChar(tmpStr, len, cs, PARSERHEAP());
  //        if (s) {
  //          yylval.stringval = s;
  //          return TOK_SBYTE_LITERAL;
  //        }
  //      }
  //      break;
  //
  //    case CharInfo::UNICODE:
  //      {
  //        NAWString* ws = convHexToWChar(tmpStr, len, cs, PARSERHEAP());
  //        if (ws) {
  //          yylval.wstringval = ws;
  //          return TOK_MBYTE_LITERAL;
  //        }
  //      }
  //      break;
  //
  //    default:
  //      break;
  //  }

  return invalidHexStrLit(lvalp);
}

//
// construct a string literal which can be either in the standard
// ANSI quoted format (single quoted ''), the MP format (double quoted ""), 
// or the hexdecimal format ( x'').  ANSI or hexdecimal format can be optionally
// prefixed with a character set name
//
inline Int32 yyULexer::constructStringLiteralWithCharSet(NABoolean isHex, 
                                                       CharInfo::CharSet cs,
                                                       YYSTYPE *lvalp,
                                                       NAWchar quote)
{
  NAWchar cc;

  undoBeforeAction();
  advance();				// past the first '
  Int32 introducerLen = YYLengNow();
  while ((cc=peekAdvance()) != WEOF)
    {
      if (cc == quote)
        if ( isHex == FALSE AND (cc=peekChar()) == quote)
          advance();
        else
          {
            doBeforeAction();
            if ( isHex ) {

              if ( HexStringLiteralNotAllowed == TRUE ) {
                // disable hex string literal if necessary (e.g., in DDL)
                HexStringLiteralNotAllowed = FALSE;
                *SqlParser_Diags << DgSqlCode(-1243);
                return prematureEOF(lvalp);
              }

              // a char string in potential hexdecimal form 
              return aHexStringLiteralWithCharSet(
                                                  cs, 
                                                  YYText() + introducerLen,
                                                  YYLeng() - introducerLen - 1,
                                                  quote,
                                                  lvalp);
            } else
              // a national char string specified by
              //  _charsetname'([^']|"''")*'
              return aStringLiteralWithCharSet(
                                               cs, 
                                               YYText() + introducerLen,
                                               YYLeng() - introducerLen - 1,
                                               quote,
                                               lvalp);
          }
    }
  return prematureEOF(lvalp);
}

// Use this method when a Trafodion Reserved word which is reserved for
// future use is encountered as an identifier.
//
inline Int32 yyULexer::anSQLMXReservedWord(YYSTYPE *lvalp)
{ 
  // Could return an error here, but instead go ahead and return
  // the identifier.  Will be caught later in transformIdentifier()
  //
  return setStringval(IDENTIFIER, DBGMSG("Identifier %s\n"), lvalp); 
}

inline Int32 yyULexer::anIdentifier(YYSTYPE *lvalp)
{ 
  return setStringval(IDENTIFIER, DBGMSG("Identifier %s\n"), lvalp); 
}

inline Int32 yyULexer::anSQLMXKeyword(Int32 tokCod, YYSTYPE *lvalp)
{ 
  return setTokval(tokCod, DBGMSG("Trafodion keyword %s\n"), lvalp);
}

inline Int32 yyULexer::aCompoundKeyword(Int32 tokCod, YYSTYPE *lvalp)
{ 
  return setTokval(tokCod, DBGMSG("Compound keyword %s\n"), lvalp);
}

inline Int32 yyULexer::aCobolToken(Int32 tokCod, YYSTYPE *lvalp)
{ 
  return setStringval(tokCod, DBGMSG("Compound Cobol token %s\n"), lvalp); 
}

inline Int32 yyULexer::anApproxNumber(YYSTYPE *lvalp)
{ 
  return setStringval(NUMERIC_LITERAL_APPROX, 
                      DBGMSG("Approx numeric literal %s\n"),
                      lvalp);
}

inline Int32 yyULexer::exactWithScale(YYSTYPE *lvalp)
{ 
  return setStringval(NUMERIC_LITERAL_EXACT_WITH_SCALE, 
                      DBGMSG("Numeric literal with scale %s\n"),
                      lvalp);
}


inline Int32 yyULexer::exactNoScale(YYSTYPE *lvalp)
{ 
  return setStringval(NUMERIC_LITERAL_EXACT_NO_SCALE, 
                      DBGMSG("Numeric literal with no scale %s\n"),
                      lvalp);
}

inline Int32 yyULexer::prematureEOF(YYSTYPE *lvalp)
{ 
  doBeforeAction();
  return setTokval(0, DBGMSG("Premature EOF in <%s>\n"), lvalp);
}

inline Int32 yyULexer::invalidHexStrLit(YYSTYPE *lvalp)
{
  doBeforeAction();
  return setTokval(0, DBGMSG("Invalid hexadecimal representation of a string literal.\n"), lvalp);
}

inline Int32 yyULexer::invalidStrLitNonTranslatableChars(YYSTYPE *lvalp)
{
  doBeforeAction();
  return setTokval(0, DBGMSG("Invalid use of non-translatable characters in a string literal.\n"), lvalp);
}

inline Int32 yyULexer::invalidHostVarNonTranslatableChars(YYSTYPE *lvalp)
{
  doBeforeAction();
  return setTokval(0, DBGMSG("Invalid use of non-translatable characters in a host variable.\n"), lvalp);
}

// This is here just because it's such a common idiom
Int32 yyULexer::eitherCompoundOrSimpleKeyword(
                                            NABoolean isCompound,
                                            Int32 tokcodCompound,
                                            Int32 tokcodSimple,
                                            NAWchar *end1,
                                            NAWchar holdChar1,
                                            YYSTYPE *lvalp)
{
  if (isCompound)
    {
      // un-null-terminate 1st kwd of the compound
      *end1 = holdChar1;
      doBeforeAction();
      return aCompoundKeyword(tokcodCompound, lvalp);
    }
  else
    {
      // retract to end of kwd1 and return a simple keyword
      retractToMark(end1);
      return anSQLMXKeyword(tokcodSimple, lvalp);
    }
}

Int32 yyULexer::notCompoundKeyword(const ParKeyWord *key, NAWchar &holdChar,
                                   YYSTYPE *lvalp)
{
  // Check to see if this token is an identifier
  //
  if (key->isIdentifier()) {
    if (key->isReserved()) {

      // If it is a reserved word, issue an error message.
      // (Are there any case when an identifier is allowed to be a
      // reserved word?)
      // Currently, anSQLMXReservedWord() behaves just like anIdentifier(),
      // so no error message is produced.  The error is caught down stream
      // by transformIdentifier(), or toAnsiIdentifier().
      //
      return anSQLMXReservedWord(lvalp);
    } else {

      // Otherwise it is simply an Identifier.
      //
      return anIdentifier(lvalp);
    }
  }

  // If it can not be a compound word, then it must be a keyword token.
  //
  if (!key->canBeFirst()) return anSQLMXKeyword(key->getTokenCode(), lvalp);
  // compound tokens have whitespace in between
  if (!U_isspace(holdChar)) return anSQLMXKeyword(key->getTokenCode(), lvalp);
  return 0;     // it IS a compound keyword
}

Int32 yyULexer::yylex(YYSTYPE *lvalp) 
{
  Int32 tokIfNotCompound;
  NAWchar cc, holdChar1, holdChar2, holdChar3;
  NAWchar *end1, *end2, *endp = NULL, *beginRun2, *beginRun3;
  CharInfo::CharSet cs;

  // Key word objects for the current token.  Three words for possible
  // 2-word or 3-word compound token.
  //
  const ParKeyWord *keyWordEntry1, *keyWordEntry2, *keyWordEntry3;

  if (SqlParser_CurrentParser->internalExpr_)
    {
      static const Int32 SqlParser_starting_token[] = 
        { 0, TOK_INTERNAL_EXPR, TOK_INTERNAL_COLUMN_DEFINITION };
      Int32 temp = SqlParser_starting_token
        [SqlParser_CurrentParser->internalExpr_];
      SqlParser_CurrentParser->internalExpr_ = NORMAL_TOKEN; // reset it
      return temp;                      // parse from anything-goes start state
    }
  
  if (returnAllChars_)
    {
      startRun();

      /*      cc=peekChar();
      advance();
      while (((cc=peekAdvance()) != WEOF) &&
	     (cc != ';'))
	{
	}
	*/
      cc = peekChar();
      while ((cc != WEOF) &&
	     (cc != ';'))
	{
	  advance();
	  cc = peekChar();
	}
      
      returnAllChars_ = FALSE;

      doBeforeAction();
      return setStringval(ANY_STRING, DBGMSG("any string %s\n"), lvalp);
    }

  if ( yy_init_ )
    {
      yy_init_ = 0;
      yy_load_buffer_state();
    }

  // Set the default charset for all returns except when overridden by
  // aStringLiteralWithCharSet() above...
  lvalp->stringval_with_charset.reset();
  lvalp->stringval_with_charset.charSet_ = CharInfo::DefaultCharSet;
  lvalp->stringval_with_charset.bytesPerChar_ = CharInfo::maxBytesPerChar(lvalp->stringval_with_charset.charSet_);

  while ( 1 )           /* loops until end-of-file is reached */
    {
      startRun();

      switch (cc=peekChar()) 
        {
        case L'_': {
          // an identifier prefix specified by _
          advance();
          do { advance(); } while (U_isalnumund(cc=peekChar()));
          doBeforeAction();

          // In Trafodion the only valid quote characters is '\''
          //
          NAWchar quote = NAWchar('\'');


          // JQ
          // Determine if hexadecimal character string representation
          // by looking ahead two characters 
          // if they are space followed by L'X'/L'x', then yes;
          // otherwise no.
          endp = mark();
          NABoolean isHexStringLiteral = FALSE;
          if (cc == L' '){
            advance();
            cc = peekChar();
            if ( (cc == L'X') || (cc == L'x') ){
              isHexStringLiteral = TRUE;
              advance();
              cc = peekChar();
            }
            else {
              retractToMark(endp);
            }
          }

          if (cc == L'\'' || cc == quote)
            {
              // The leading quote character encountered.  For Trafodion
              // text, this will always be a single quote.
              //
              quote = cc;

              NAWString preserveCase(YYText());
              yyToUpper();
              yyToNarrow();
              const char *csName = &yynarrow_[1];	// past the _ introducer
              cs = CharInfo::getCharSetEnum(csName);
              if (CharInfo::isCharSetFullySupported(cs))
                {
                  if (cs == CharInfo::BINARY)
                    isHexStringLiteral = TRUE;
                  return constructStringLiteralWithCharSet(isHexStringLiteral, cs, lvalp, quote);
                } 
              else if (cs != CharInfo::UnknownCharSet)
                {
                  // 3010 Character set $0~string0 is not yet supported.
                  *SqlParser_Diags << DgSqlCode(-3010) << DgString0(csName);
                  return prematureEOF(lvalp);
                }
              NAWstrcpy(yytext_, preserveCase);
            }
          // fall thru from if and nested-if above

          // two more characters have beed advanced for hex format string literals than 
          // regular qualified string literels, have to retract.
          if (isHexStringLiteral) {
            retractToMark(endp);
          }

          // Look for the current word in the keyword table.  If it is
          // not found, an IDENTIFIER keyword entry will be used.
          //
          keyWordEntry1 = ParKeyWords::lookupKeyWord(yytext_);

          if (tokIfNotCompound=notCompoundKeyword(keyWordEntry1, cc, lvalp))
            {
              if (tokIfNotCompound == IDENTIFIER && U_isspace(cc))
                {
                  NAWString preserveCase(YYText());
                  yyToUpper();
                  yyToNarrow();
                  const char *csName = &yynarrow_[1];	// past the _ introducer
                  cs = CharInfo::getCharSetEnum(csName);
                  if (cs != CharInfo::UnknownCharSet)
                    {
                      endp = mark();
                      undoBeforeAction();
                      while ((cc=peekChar()) != WEOF && U_isspace(cc))
                        advance();
                      if (cc == L'\'' || cc == quote)
                        {
                          // Emit a warning suggesting they may have meant:
                          //	_charset'literal'   instead of    _cs 'lit'
                          // Here we do NOT want to use "csName"
                          // because we want to retain the _ introducer.
                          *SqlParser_Diags << DgSqlCode(+3159)
                                           << DgString0(yynarrow_);
                        }
                      retractToMark(endp);
                    }
                  NAWstrcpy(yytext_, preserveCase);
                }
              return tokIfNotCompound;
            }
          else
            {
              Int32 BadCompoundKeywordPrefix=0;
              assert(BadCompoundKeywordPrefix);
            }
        }
          break;
        case L'\'':
          // 'string' specified by '([^']|"''")*'
          advance();
          while ((cc=peekAdvance()) != WEOF)
            {
              if (cc == L'\'')
                if ((cc= peekChar()) == L'\'')
                  advance();
                else
		  {
		    doBeforeAction();
		    addTokenToGlobalQueue();
		    dbgprintf(DBGMSG("Quoted string literal %s\n"));
		    
		    if ( ParScannedInputCharset != CharInfo::ISO88591 ) {
		      NAString* tempstr = unicodeToChar
			(YYText(),YYLeng(), ParScannedInputCharset,PARSERHEAP());
		      if (tempstr == NULL)
			return invalidStrLitNonTranslatableChars(lvalp);
		      ParScannedTokenOffset += ((Int32)tempstr->length() - (Int32)YYLeng());
		      ParScannedTokens->updateInputLen((Int32)tempstr->length());
		    }                   
		    lvalp->stringval = removeConsecutiveQuotes
		      ( YYText()+1, YYLeng()-1, WIDE_('\''), lvalp
		      , CharInfo::UnknownCharSet // string literal without character set prefix
		      );
		    return (lvalp->stringval) ? QUOTED_STRING : invalidStrLitNonTranslatableChars(lvalp);
		  }
            }
          return prematureEOF(lvalp);
        case L'C': case L'c':
          // identifier prefix specified by [Cc]
          do { advance(); } while (U_isalnumund(holdChar1=peekChar()));
          doBeforeAction();

          // Look for the current word in the keyword table.  If it is
          // not found, an IDENTIFIER keyword entry will be used.
          //
          keyWordEntry1 = ParKeyWords::lookupKeyWord(yytext_);

          if (tokIfNotCompound=notCompoundKeyword(keyWordEntry1, holdChar1, lvalp))
            return tokIfNotCompound;
          // else, a possible compound Trafodion Cobol token
          // save first token end
          end1 = mark();
          // skip whitespace
          do { advance(); } while (U_isspace(cc=peekChar()));
          // check 2nd part of compound Cobol token
          switch(keyWordEntry1->getTokenCode()) {
          default: 
            // an identifier beginning with [Cc]
            return anIdentifier(lvalp);
          case TOK_CALL:

            // Raj P - 7/200
            // If we are not in the whenever statement and it is a CALL
            // it must be a CALL to a (java) stored procedure
            if ( !SqlParser_WheneverClause )
              {
                // This is not a compound statement
                tokIfNotCompound = anIdentifier(lvalp);
                return (keyWordEntry1->getTokenCode());
              }
 
            // check 2nd part of compound Cobol token
            if (U_isAsciiAlNumUnd(cc)) {

              // scan CASED_IDENTIFIER
              do { advance(); cc=peekChar(); }
              while (SqlParser_WheneverClause ? U_isAsciiAlNumUndHyphen(cc)
                     : U_isAsciiAlNumUnd(cc));
              // un-null-terminate 1st kwd
              *end1 = holdChar1;
              doBeforeAction();
              // a compound token specified by
              // [Cc][Aa][Ll][Ll]{B}{CASED_IDENTIFIER}
              return aCobolToken(CALL_CASED_IDENTIFIER, lvalp);
            }
            else 
              // an identifier specified by [Cc][Aa][Ll][Ll]
              return anIdentifier(lvalp);
            break;
            // QSTUFF
          case TOK_CURSOR:
            if (!U_isAsciiAlNumUnd(cc)) {
              // CURSOR is a simple keyword. 
              return anSQLMXKeyword(keyWordEntry1->getTokenCode(), lvalp);
            }
            else { 
              beginRun2 = mark();
              do { advance(); } while (U_isalnumund(cc=peekChar()));
              // null-terminate 2nd part
              holdChar2 = cc;
              setCurrChar(0);
              // is 2nd kwd part of a compound token?
              //
              // Look for the second word in the keyword table.  If it
              // is not found, an IDENTIFIER keyword entry will be
              // used.
              //
              keyWordEntry2 = ParKeyWords::lookupKeyWord(beginRun2);
              // restore null-termination of 2nd part.
              setCurrChar(holdChar2);
              switch (keyWordEntry2->getTokenCode()) {
              default: 
                // CURSOR is a simple keyword. 
                // retract to end of kwd1.
                retractToMark(end1);
                return anSQLMXKeyword(keyWordEntry1->getTokenCode(), lvalp);
              case TOK_WITH:
                // un-null-terminate 1st kwd
                *end1 = holdChar1;
                // skip whitespace
                while (U_isspace(cc=peekChar())) advance();
                // check 3rd part of 3-part compound token
                if (!U_isAsciiAlpha(cc)) {
                  // 3rd part is not a keyword.
                  // return CURSOR as a simple keywork 
                  retractToMark(end1);
                  return anSQLMXKeyword(keyWordEntry1->getTokenCode(), lvalp);
                }
                // scan 3rd part
                beginRun3 = mark();
                while (U_isalnumund(cc=peekChar())) advance();
                // null-terminate 3rd part
                holdChar3 = cc;
                setCurrChar(0);
                // is 3rd kwd part of a compound token?
                //
                // Look for the third word in the keyword
                // table.  If it is not found, an IDENTIFIER
                // keyword entry will be used.
                //
                keyWordEntry3 = ParKeyWords::lookupKeyWord(beginRun3);

                // restore null-termination of 3rd part.
                setCurrChar(holdChar3);
                if (!keyWordEntry3->canBeThird() ||
                    keyWordEntry3->getTokenCode() != TOK_HOLD) {
                  // suffix is not part of a 3-part compound token,
                  // ie, CURSOR
                  // return 1st part as a 2-part compound token.
                  retractToMark(end1);
                  return anSQLMXKeyword(keyWordEntry1->getTokenCode(), lvalp);
                }
                else { 
                  doBeforeAction();
                  return aCompoundKeyword(TOK_CURSOR_WITH_HOLD, lvalp);
                }
              case TOK_WITHOUT:
                // un-null-terminate 1st kwd
                *end1 = holdChar1;
                // skip whitespace
                while (U_isspace(cc=peekChar())) advance();
                // check 3rd part of 3-part compound token
                if (!U_isAsciiAlpha(cc)) {
                  // 3rd part is not a keyword.
                  // return CURSOR as a simple keywork 
                  retractToMark(end1);
                  return anSQLMXKeyword(keyWordEntry1->getTokenCode(), lvalp);
                }
                // scan 3rd part
                beginRun3 = mark();
                while (U_isalnumund(cc=peekChar())) advance();
                // null-terminate 3rd part
                holdChar3 = cc;
                setCurrChar(0);
                // is 3rd kwd part of a compound token?
                //
                // Look for the third word in the keyword
                // table.  If it is not found, an IDENTIFIER
                // keyword entry will be used.
                //
                keyWordEntry3 = ParKeyWords::lookupKeyWord(beginRun3);

                // restore null-termination of 3rd part.
                setCurrChar(holdChar3);
                if (!keyWordEntry3->canBeThird() ||
                    keyWordEntry3->getTokenCode() != TOK_HOLD) {
                  // suffix is not part of a 3-part compound token,
                  // ie, CURSOR
                  // return 1st part as a 2-part compound token.
                  retractToMark(end1);
                  return anSQLMXKeyword(keyWordEntry1->getTokenCode(), lvalp);
                }
                else { 
                  doBeforeAction();
                  return aCompoundKeyword(TOK_CURSOR_WITHOUT_HOLD, lvalp);
                }
              }       // switch (keyWordEntry2->getTokenCode())
            }     // if !U_isAsciiAlNumUnd(cc) ... else ....

          case TOK_CLEANUP:
            if (!U_isAsciiAlNumUnd(cc)) {
              // CLEANUP is a simple keyword. 
              return anSQLMXKeyword(keyWordEntry1->getTokenCode(), lvalp);
            }
            else { 
              beginRun2 = mark();
              do { advance(); } while (U_isalnumund(cc=peekChar()));
              // null-terminate 2nd part
              holdChar2 = cc;
              setCurrChar(0);
              // is 2nd kwd part of a compound token?
              //
              // Look for the second word in the keyword table.  If it
              // is not found, an IDENTIFIER keyword entry will be
              // used.
              //
              keyWordEntry2 = ParKeyWords::lookupKeyWord(beginRun2);
              // restore null-termination of 2nd part.
              setCurrChar(holdChar2);
              switch (keyWordEntry2->getTokenCode()) {
              case TOK_OBSOLETE:
                {
                  return eitherCompoundOrSimpleKeyword
                    (keyWordEntry2->getTokenCode() == TOK_OBSOLETE,
                     TOK_CLEANUP_OBSOLETE,
                     keyWordEntry1->getTokenCode(),
                     end1, holdChar1, lvalp);
                   
                }
                break;

              default: 
                {
                  // CLEANUP is a simple keyword. 
                  // retract to end of kwd1.
                  retractToMark(end1);
                  return anSQLMXKeyword(keyWordEntry1->getTokenCode(), lvalp);
                }
                break;
              }       // switch (keyWordEntry2->getTokenCode())
            }     // if !U_isAsciiAlNumUnd(cc) ... else ....
            
          }     // switch (keyWordEntry1->getTokenCode())
          // control should not reach here. but if it does, we may be
          // seeing an identifier beginning with letter [Cc]
          return anIdentifier(lvalp);
          break;
        case L'G': case L'g':
          // identifier prefix specified by [Gg]
          do { advance(); } while (U_isalnumund(cc=peekChar()));
          holdChar1 = cc;
          doBeforeAction();

          // Look for the current word in the keyword table.  If it is
          // not found, an IDENTIFIER keyword entry will be used.
          //
          keyWordEntry1 = ParKeyWords::lookupKeyWord(yytext_);

          if (tokIfNotCompound=notCompoundKeyword(keyWordEntry1, holdChar1, lvalp))
            return tokIfNotCompound;
          // else, a possible compound Trafodion Cobol token
          // save first token end
          end1 = mark();
          // skip whitespace
          do { advance(); } while (U_isspace(cc=peekChar()));
          // check 2nd part of compound token
          if (!U_isAsciiAlpha(cc))
            {
              // 2nd part is not a keyword. 
              // return 1st token as a Trafodion keyword.
              return anSQLMXKeyword(keyWordEntry1->getTokenCode(), lvalp);
            }
          else if (keyWordEntry1->getTokenCode() == TOK_GO)
            // 2nd part may be a keyword
            {
              // scan 2nd part
              beginRun2 = mark();
              do { advance(); } while (U_isalnumund(cc=peekChar()));
              // null-terminate 2nd part
              holdChar2 = cc;
              setCurrChar(0);
              // is 2nd kwd part of a compound token?
              //
              // Look for the second word in the keyword table.  If it
              // is not found, an IDENTIFIER keyword entry will be
              // used.
              //
              keyWordEntry2 = ParKeyWords::lookupKeyWord(beginRun2);

              // restore null-termination of 2nd part.
              setCurrChar(holdChar2);
              if (keyWordEntry2->getTokenCode() == TOK_TO && U_isspace(cc))
                {
                  // scanned [Gg][Oo]{B}?[Tt][Oo]{B}
                  while (U_isspace(cc)) cc=peekAdvance();

                  // Drop thru to GOTO case below.
                }
              else
                {
                  // suffix is not part of a compound token.
                  // return 1st part as a keyword.
                  return anSQLMXKeyword(keyWordEntry1->getTokenCode(), lvalp);
                }
            }
          if ((keyWordEntry1->getTokenCode() == TOK_GOTO ||
               keyWordEntry1->getTokenCode() == TOK_GO)
              && U_isAsciiAlNumUnd(cc))
            {
              // scan CASED_IDENTIFIER
              do { advance(); } while (U_isAsciiAlNumUndHyphen(peekChar()));
              // un-null-terminate 1st kwd
              *end1 = holdChar1;
              doBeforeAction();
              // a compound token specified by
              // [Gg][Oo]{B}?[Tt][Oo]{B}{CASED_IDENTIFIER}
              return aCobolToken(GOTO_CASED_IDENTIFIER, lvalp);
            }
          else
            {
              // an identifier specified by [Gg][Oo]{B}?[Tt][Oo]
              return anIdentifier(lvalp);
            }
          break;
        case L'N': case L'n': {
          advance();
          end1 = mark();

          // In Trafodion the only valid quote characters is '\''
          //
          NAWchar quote = NAWchar('\'');

          NABoolean isHexRep = FALSE;

          {

            // MX text. Determine if a hexadecimal character string 
            // representation follows.
            if (peekChar() == L' '){
              advance();
              cc = peekChar();
              if ( (cc == L'X') || (cc == L'x') ){
                isHexRep = TRUE;
                advance();
              }
              else {
                retractToMark(end1);
              }
            }
          }

          if ((peekChar() == L'\'' || peekChar() == quote)) 
            {
              // N'string' specified by N'([^']|"''")*', or
              // N X'hex digit string'

              // The leading quote character encountered.  For Trafodion
              // text, this will always be a single quote.
              //
              quote = peekChar();

              return constructStringLiteralWithCharSet(
                                                       isHexRep,
                                                       SqlParser_NATIONAL_CHARSET,
                                                       lvalp,
                                                       quote);

            }
          else // identifier or keyword prefix specified by [Nn]
            {
              retractToMark(end1);

              while (U_isalnumund(holdChar1=peekChar())) advance();
              doBeforeAction();

              // Look for the current word in the keyword table.  If
              // it is not found, an IDENTIFIER keyword entry will be
              // used.
              //
              keyWordEntry1 = ParKeyWords::lookupKeyWord(yytext_);

              if (tokIfNotCompound=notCompoundKeyword(keyWordEntry1, holdChar1, lvalp))
                return tokIfNotCompound;
              // else, a possible compound Trafodion keyword
              // save first token end
              end1 = mark();
              // skip whitespace
              do { advance(); } while (U_isspace(cc=peekChar()));
              // check 2nd part of compound token
              if (!U_isAsciiAlpha(cc))
                {
                  // 2nd part is not a keyword. 
                  // return 1st token as a Trafodion keyword.
                  return anSQLMXKeyword(keyWordEntry1->getTokenCode(), lvalp);
                }
              else // 2nd part may be a keyword
                {
                  // scan 2nd part
                  beginRun2 = mark();
                  do { advance(); } while (U_isalnumund(cc=peekChar()));
                  // null-terminate 2nd part
                  holdChar2 = cc;
                  setCurrChar(0);
                  // is 2nd kwd part of a compound token?
                  //
                  // Look for the second word in the keyword table.
                  // If it is not found, an IDENTIFIER keyword entry
                  // will be used.
                  //
                  keyWordEntry2 = ParKeyWords::lookupKeyWord(beginRun2);

                  // restore null-termination of 2nd part.
                  setCurrChar(holdChar2);
                  if (!keyWordEntry2->canBeSecond())
                    {
                      // suffix is not part of a compound token.
                      // return 1st part as a keyword.
                      return anSQLMXKeyword(keyWordEntry1->getTokenCode(), lvalp);
                    }
                  else // 2nd kwd may be part of a compound token
                    {
                      switch (keyWordEntry1->getTokenCode())
                        {
                        case TOK_NO:
                          return eitherCompoundOrSimpleKeyword
                            (keyWordEntry2->getTokenCode() == TOK_DEFAULT ||
                             keyWordEntry2->getTokenCode() == TOK_PARTITION ||
                             keyWordEntry2->getTokenCode() == TOK_PARTITIONS ||
                             keyWordEntry2->getTokenCode() == TOK_LOAD,
                             ((keyWordEntry2->getTokenCode() == TOK_DEFAULT)
                              ? TOK_NO_DEFAULT
                              : ((keyWordEntry2->getTokenCode() == TOK_LOAD)
                                 ? TOK_NO_LOAD
                                 : ((keyWordEntry2->getTokenCode() == TOK_PARTITION)
                                    ? TOK_NO_PARTITION
                                    : TOK_NO_PARTITIONS))),
                             keyWordEntry1->getTokenCode(),
                             end1, holdChar1, lvalp);
                          break;
                        case TOK_NOT:
                          return eitherCompoundOrSimpleKeyword
                            (keyWordEntry2->getTokenCode() == TOK_IN ||
                             keyWordEntry2->getTokenCode() == TOK_BETWEEN || 
                             keyWordEntry2->getTokenCode() == TOK_DROPPABLE || 
                             keyWordEntry2->getTokenCode() == TOK_CASESPECIFIC ||
			     keyWordEntry2->getTokenCode() == TOK_ENFORCED,

                             ((keyWordEntry2->getTokenCode() == TOK_IN )
                              ? TOK_NOT_IN
                              : ((keyWordEntry2->getTokenCode() == TOK_BETWEEN)
                                 ? TOK_NOT_BETWEEN
                                 : ((keyWordEntry2->getTokenCode() == TOK_DROPPABLE)
                                   ? TOK_NOT_DROPPABLE
				     : ((keyWordEntry2->getTokenCode() == TOK_CASESPECIFIC)
					? TOK_NOT_CASESPECIFIC
					  : TOK_NOT_ENFORCED)))),
                             keyWordEntry1->getTokenCode(),
                             end1, holdChar1, lvalp);
                          break;
                        default:
                          Int32 BadCompoundKeywordPrefix=0;
                          assert(BadCompoundKeywordPrefix);
                        }
                    }
                }

            }
          break;
        }
        case L'P': case L'p':
          // identifier prefix specified by [Pp]
          do { advance(); } while (U_isalnumund(holdChar1=peekChar()));
          doBeforeAction();

          // Look for the current word in the keyword table.  If it is
          // not found, an IDENTIFIER keyword entry will be used.
          //
          keyWordEntry1 = ParKeyWords::lookupKeyWord(yytext_);

          if (tokIfNotCompound=notCompoundKeyword(keyWordEntry1, holdChar1, lvalp))
            {
              if (tokIfNotCompound == TOK_PICTURE)
                return anIdentifier(lvalp); 
              else
                return tokIfNotCompound;
            }
          // else, a possible compound Trafodion Cobol token
          // save first token end
          end1 = mark();
          // skip whitespace
          do { advance(); } while (U_isspace(cc=peekChar()));
          // check 2nd part of compound Cobol token
	  if (keyWordEntry1->getTokenCode() == TOK_PRIMARY)
	    {
              // scan 2nd part
              beginRun2 = mark();
              do { advance(); } while (U_isalnumund(cc=peekChar()));
              // null-terminate 2nd part
              holdChar2 = cc;
              setCurrChar(0);
              // is 2nd kwd part of a compound token?
              //
              // Look for the second word in the keyword table.  If it
              // is not found, an IDENTIFIER keyword entry will be
              // used.
              //
              keyWordEntry2 = ParKeyWords::lookupKeyWord(beginRun2);

              // restore null-termination of 2nd part.
              setCurrChar(holdChar2);
              if (!keyWordEntry2->canBeSecond())
                {
                  // suffix is not part of a compound token.
                  // return 1st part as a keyword.
                  return anSQLMXKeyword(keyWordEntry1->getTokenCode(), lvalp);
                }
              else // 2nd kwd may be part of a compound token
                {
		  return eitherCompoundOrSimpleKeyword
		    (keyWordEntry2->getTokenCode() == TOK_INDEX,
		     TOK_PRIMARY_INDEX,
		     keyWordEntry1->getTokenCode(),
		     end1, holdChar1, lvalp);
		}
	    }
          else if (keyWordEntry1->getTokenCode() == TOK_PERFORM &&
              SqlParser_WheneverClause &&
              U_isAsciiAlNumUnd(cc))
            {
              // scan CASED_IDENTIFIER
              do { advance(); } while (U_isAsciiAlNumUndHyphen(peekChar()));
              // un-null-terminate 1st kwd
              *end1 = holdChar1;
              doBeforeAction();
              // a compound token specified by
              // [Pp][Ee][Rr][Ff][Oo][Rr][Mm]{B}{CASED_IDENTIFIER}
              return aCobolToken(PERFORM_CASED_IDENTIFIER, lvalp);
            }
          else if (keyWordEntry1->getTokenCode() == TOK_PICTURE)
            {
              if (cc == L'X' || cc == L'x')
                {
                  // enough for a valid Cobol PICTURE token
                  // un-null-terminate 1st kwd: PICTURE
                  *end1 = holdChar1;
                  // scan for ([Xx](\({B}?[0-9]+{B}?\))?)+
                  while (cc == L'X' || cc == L'x')
                    {
                      advance();
                      endp = mark(); // remember end of XSPEC
                      // scan for \({B}?[0-9]+{B}?\)
                      if ((cc=peekChar()) == L'(')
                        {
                          advance();
                          while (U_isspace(cc=peekChar())) advance();
                          if (!U_isdigit(cc)) break;
                          // scan for [0-9]+{B}?\)
                          while (U_isdigit(cc=peekChar())) advance();
                          while (U_isspace(cc=peekChar())) advance();
                         
                          if ( U_isAsciiAlpha(cc) ) {
                            // allow PIC X(10 characters)

                            NAWchar* bp_char = mark();
                            do { advance(); } 
                            while (U_isAsciiAlpha(cc=peekChar()));

                            // null-terminate 
                            holdChar2 = cc;
                            setCurrChar(0);
                            const ParKeyWord *keyWordEntry = 
                              ParKeyWords::lookupKeyWord(bp_char);

                            // restore null-termination 
                            setCurrChar(holdChar2);

                            if ( keyWordEntry == NULL ||
                                 keyWordEntry->getTokenCode() != TOK_CHARACTERS )
                              break ; // error for now. 
                            // Later on, allow code_units (UTF16)
                          
                            while (U_isspace(cc=peekChar())) advance();
                          }

                          if (cc != L')') break;
                          // remember end of XSPEC
                          advance(); endp = mark();
                          cc = peekChar();
                        }
                    }
                  // retract to end of last good XSPEC
                  retractToMark(endp);
                  // a compound token specified by
                  // [Pp][Ii][Cc]([Tt][Uu][Rr][Ee])?{B}
                  // ([Xx](\({B}?[0-9]+{B}?\))?)+
                  doBeforeAction();
                  return aCobolToken(keyWordEntry1->getTokenCode(), lvalp);
                }
              else if (cc == L'S' || cc == L's')
                {
                  // a compound token prefix specified by
                  // [Pp][Ii][Cc]([Tt][Uu][Rr][Ee])?{B}[Ss]
                  advance();
                  if ((cc=peekChar()) == L'9')
                    {
                      // enough for a valid Cobol PICTURE token
                      // un-null-terminate 1st kwd: PICTURE
                      *end1 = holdChar1;
                      // scan for (9(\({B}?[0-9]+{B}?\))?)+
                      while (cc == L'9')
                        {
                          advance();
                          endp = mark(); // remember end of NINES
                          // scan for \({B}?[0-9]+{B}?\)
                          if ((cc=peekChar()) == L'(')
                            {
                              advance();
                              while (U_isspace(cc=peekChar())) advance();
                              if (!U_isdigit(cc)) break;
                              // scan for [0-9]+{B}?\)
                              while(U_isdigit(cc)) cc=peekAdvance();
                              while(U_isspace(cc)) cc=peekAdvance();
                              if (cc != L')') break;
                              // remember end of NINES
                              endp = mark();
                              cc = peekChar();
                            }
                        }
                      if (cc == L'V' || cc == L'v')
                        {
                          // a compound token prefix specified by
                          // [Pp][Ii][Cc]([Tt][Uu][Rr][Ee])?{B}
                          // [Ss]{NINES}[Vv]
                          // where NINES is (9(\({B}?[0-9]+{B}?\))?)+
                          advance();
                          endp = mark();
                          cc = peekChar();
                          // scan for (9(\({B}?[0-9]+{B}?\))?)+
                          while (cc == L'9')
                            {
                              advance();
                              endp = mark(); // remember end of NINES
                              // scan for \({B}?[0-9]+{B}?\)
                              if ((cc=peekChar()) == L'(')
                                {
                                  advance();
                                  while (U_isspace(cc=peekChar())) 
                                    advance();
                                  if (!U_isdigit(cc)) break;
                                  // scan for [0-9]+{B}?\)
                                  while(U_isdigit(cc)) cc=peekAdvance();
                                  while(U_isspace(cc)) cc=peekAdvance();
                                  if (cc != L')') break;
                                  // remember end of NINES
                                  endp = mark();
                                  cc = peekChar();
                                }
                            }
                        }
                      // retract to end of last NINES
                      retractToMark(endp);
                      // a compound token specified by
                      // [Pp][Ii][Cc]([Tt][Uu][Rr][Ee])?{B}
                      // [Ss]({NINES}|({NINES}[Vv]{NINES}?))
                      // where NINES is (9(\({B}?[0-9]+{B}?\))?)+
                      doBeforeAction();
                      return aCobolToken(keyWordEntry1->getTokenCode(), lvalp);
                    }
                  else if (cc == L'V' || cc == L'v')
                    {
                      // enough for a valid Cobol PICTURE token
                      // un-null-terminate 1st kwd: PICTURE
                      *end1 = holdChar1;
                      // a compound token prefix specified by
                      // [Pp][Ii][Cc]([Tt][Uu][Rr][Ee])?{B}[Ss][Vv]
                      advance();
                      endp = mark(); // remember end of NINESPEC
                      cc = peekChar();
                      // scan for (9(\({B}?[0-9]+{B}?\))?)+
                      while (cc == L'9')
                        {
                          advance();
                          endp = mark(); // remember end of NINES
                          // scan for \({B}?[0-9]+{B}?\)
                          if ((cc=peekChar()) == L'(')
                            {
                              advance();
                              while (U_isspace(cc=peekChar())) advance();
                              if (!U_isdigit(cc)) break;
                              // scan for [0-9]+{B}?\)
                              while(U_isdigit(cc)) cc=peekAdvance();
                              while(U_isspace(cc)) cc=peekAdvance();
                              if (cc != L')') break;
                              // remember end of NINES
                              endp = mark();
                              cc = peekChar();
                            }
                        }
                      // retract to end of last NINESPEC
                      retractToMark(endp);
                      // a compound token specified by
                      // [Pp][Ii][Cc]([Tt][Uu][Rr][Ee])?{B}
                      // [Ss][Vv]{NINES}?
                      // where NINES is (9(\({B}?[0-9]+{B}?\))?)+
                      doBeforeAction();
                      return aCobolToken(keyWordEntry1->getTokenCode(), lvalp);
                    }
                  else
                    {
                      // an identifier specified by
                      // [Pp][Ii][Cc]([Tt][Uu][Rr][Ee])? 
                      return anIdentifier(lvalp);
                    }
                }
              else if (cc == L'9')
                {
                  // a compound token prefix specified by
                  // [Pp][Ii][Cc]([Tt][Uu][Rr][Ee])?{B}9

                  // enough for a valid Cobol PICTURE token
                  // un-null-terminate 1st kwd: PICTURE
                  *end1 = holdChar1;

                  // scan for (9(\({B}?[0-9]+{B}?\))?)+
                  while (cc == L'9')
                    {
                      advance();
                      endp = mark(); // remember end of NINES
                      // scan for \({B}?[0-9]+{B}?\)
                      if ((cc=peekChar()) == L'(')
                        {
                          advance();
                          while (U_isspace(cc=peekChar())) advance();
                          if (!U_isdigit(cc)) break;
                          // scan for [0-9]+{B}?\)
                          while(U_isdigit(cc)) cc=peekAdvance();
                          while(U_isspace(cc)) cc=peekAdvance();
                          if (cc != L')') break;
                          // remember end of NINES
                          endp = mark();
                          cc = peekChar();
                        }
                    }
                  if (cc == L'V' || cc == L'v')
                    {
                      // a compound token prefix specified by
                      // [Pp][Ii][Cc]([Tt][Uu][Rr][Ee])?{B}{NINES}[Vv]
                      // where NINES is (9(\({B}?[0-9]+{B}?\))?)+
                      advance();
                      endp = mark();
                      cc = peekChar();
                      // scan for (9(\({B}?[0-9]+{B}?\))?)+
                      while (cc == L'9')
                        {
                          advance();
                          endp = mark(); // remember end of NINES
                          // scan for \({B}?[0-9]+{B}?\)
                          if ((cc=peekChar()) == L'(')
                            {
                              advance();
                              while (U_isspace(cc=peekChar())) 
                                advance();
                              if (!U_isdigit(cc)) break;
                              // scan for [0-9]+{B}?\)
                              while(U_isdigit(cc)) cc=peekAdvance();
                              while(U_isspace(cc)) cc=peekAdvance();
                              if (cc != L')') break;
                              // remember end of NINES
                              endp = mark();
                              cc = peekChar();
                            }
                        }
                    }
                  // retract to end of last NINES
                  retractToMark(endp);
                  // a compound token specified by
                  // [Pp][Ii][Cc]([Tt][Uu][Rr][Ee])?{B}
                  // ({NINES}|({NINES}[Vv]{NINES}?))
                  // where NINES is (9(\({B}?[0-9]+{B}?\))?)+
                  doBeforeAction();
                  return aCobolToken(keyWordEntry1->getTokenCode(), lvalp);
                }
              else if (cc == L'V' || cc == L'v')
                {
                  // a compound token prefix specified by
                  // [Pp][Ii][Cc]([Tt][Uu][Rr][Ee])?{B}[Vv]
                  advance();
                  // enough for a valid Cobol PICTURE token
                  // un-null-terminate 1st kwd: PICTURE
                  *end1 = holdChar1;
                  endp = mark();
                  cc = peekChar();
                  // scan for (9(\({B}?[0-9]+{B}?\))?)+
                  while (cc == L'9')
                    {
                      advance();
                      endp = mark(); // remember end of NINES
                      // scan for \({B}?[0-9]+{B}?\)
                      if ((cc=peekChar()) == L'(')
                        {
                          advance();
                          while (U_isspace(cc=peekChar())) 
                            advance();
                          if (!U_isdigit(cc)) break;
                          // scan for [0-9]+{B}?\)
                          while(U_isdigit(cc)) cc=peekAdvance();
                          while(U_isspace(cc)) cc=peekAdvance();
                          if (cc != L')') break;
                          // remember end of NINES
                          endp = mark();
                          cc = peekChar();
                        }
                    }
                  // retract to end of last NINES
                  retractToMark(endp);
                  // a compound token specified by
                  // [Pp][Ii][Cc]([Tt][Uu][Rr][Ee])?{B}[Vv]{NINES}?
                  // where NINES is (9(\({B}?[0-9]+{B}?\))?)+
                  doBeforeAction();
                  return aCobolToken(keyWordEntry1->getTokenCode(), lvalp);
                }
              else 
                {
                  // an identifier specified by
                  // [Pp][Ii][Cc]([Tt][Uu][Rr][Ee])? 
                  return anIdentifier(lvalp);
                }
            }
           else if (keyWordEntry1->getTokenCode() == TOK_PARTITION)
            {
              // scan 2nd part
              beginRun2 = mark();
              do { advance(); } while (U_isalnumund(cc=peekChar()));
              // null-terminate 2nd part
              holdChar2 = cc;
              setCurrChar(0);
              keyWordEntry2 = ParKeyWords::lookupKeyWord(beginRun2);

              // restore null-termination of 2nd part.
              setCurrChar(holdChar2);
              if(keyWordEntry2->getTokenCode() == TOK_BY)
                {
                  *end1 = holdChar1;
                  doBeforeAction();
                  return aCompoundKeyword(TOK_PARTITION_BY, lvalp);
                }
              else
                {
                  // suffix is not part of a compound token.
                  // return 1st part as a keyword.
                   retractToMark(end1);
                  return anSQLMXKeyword(keyWordEntry1->getTokenCode(), lvalp);
                }
            }
          else
            {
              // it's not part of a compound Cobol token.
              // return 1st token as a Trafodion identifier.
              return anIdentifier(lvalp);
            }
          break;
        case L'"':
          // "delimited identifier" specified by \"([^\"]|"\"\"")*\"
          // " (To help editors that have trouble counting quotes.
          //
          // Or a string literal if parsing/lexing SQL/MP Stored text.
          //
          advance();
          while ((cc=peekAdvance()) != WEOF)
            {
              if (cc == L'"')
                if ((cc= peekChar()) == L'"')
                  advance();
                else
                  {
                    doBeforeAction();
                      // In Trafodion text, double quoted strings are
                      // delimited identifiers.
                      //
                      return setStringval(DELIMITED_IDENTIFIER, 
                                          DBGMSG("Delimited identifier %s\n"),
                                          lvalp);
                  }
            }
          return prematureEOF(lvalp);
        case L'`':
          // "delimited identifier" enclosed within backquotes (`)
          //
          advance();
          while ((cc=peekAdvance()) != WEOF)
            {
              if (cc == L'`')
                if ((cc= peekChar()) == L'`')
                  advance();
                else
                  {
                    doBeforeAction();
                      // In Trafodion text, double quoted strings are
                      // delimited identifiers.
                      //
                      return setStringval(BACKQUOTED_IDENTIFIER, 
                                          DBGMSG("Backquoted identifier %s\n"),
                                          lvalp);
                  }
            }
          return prematureEOF(lvalp);
        case L'.':
          advance();
          if (U_isdigit(cc=peekChar()))
            {
              // numeric prefix specified by \.{digit}+
              do { advance(); } while (U_isdigit(cc=peekChar()));
              end1 = mark();
              if (cc == L'E' || cc == L'e')
                {
                  // approx numeric specified by 
                  // \.{digit}+[Ee][+-]?{digit}+
                  advance();
                  cc = peekAdvance();
                  if ((cc == L'+' || cc == L'-') && U_isdigit(peekChar()))
                    {
                      while (U_isdigit(peekChar())) advance();
                      doBeforeAction();
                      return anApproxNumber(lvalp);
                    }
                  else if (U_isdigit(cc))
                    {
                      // approx numeric specified by 
                      // \.{digit}+[Ee]{digit}+
                      while (U_isdigit(peekChar())) advance();
                      doBeforeAction();
                      return anApproxNumber(lvalp);
                    }
                  else
                    {
                      // exact_numeric_with_scale specified by \.{digit}+
                      retractToMark(end1);
                      doBeforeAction();
                      return exactWithScale(lvalp);
                    }
                }
              else
                {
                  // exact_numeric_with_scale specified by \.{digit}+
                  doBeforeAction();
                  return exactWithScale(lvalp);
                }
            }
          else if (cc == L'*')
            {
              // '.*'
              advance();
              doBeforeAction();
              return setTokval(TOK_DOT_STAR, DBGMSG("STAR (*)\n"), lvalp);
            }
          else 
            {
              // '.'
              doBeforeAction();
              return setTokval(yytext_[0], DBGMSG("DOT (.)\n"), lvalp);
            }
          break;
        case L'|':
          advance();
          if (peekChar() == L'|')
            {
              // concatenation operator: ||
              advance();
              doBeforeAction();
              return setTokval(TOK_CONCATENATION, DBGMSG("Concatenation %s\n"), lvalp);
            }
          else
            {
              // separator |
              doBeforeAction();
              return setTokval(TOK_BITOR_OPERATOR, DBGMSG("Separator %s\n"), lvalp);
	      //              return setTokval(yytext_[0], DBGMSG("Separator %s\n"), lvalp);
            }
          break;
        case L'>':
          advance();
          if ((cc=peekChar()) == L'=')
            {
              // greater or equal: >=
              advance();
              doBeforeAction();
              return setTokval(TOK_GREATER_EQUAL, DBGMSG("Greater or equal %s\n"), lvalp);
            }
          else if (cc == L'>') { 
            // optimizer hint end specified by >>
            advance();
            doBeforeAction();
            return setTokval(TOK_END_HINT, DBGMSG("Hint end %s\n"), lvalp);
          }
          else
            {
              // greater: >
              doBeforeAction();
              return setTokval(yytext_[0], DBGMSG("Greater %s\n"), lvalp);
            }
          break;
        case L'!':
          advance();
          if ((cc=peekChar()) == L'=')
            {
              // not equal: !=
              advance();
              doBeforeAction();
              return setTokval(TOK_NOT_EQUAL, DBGMSG("Not equal %s\n"), lvalp);
            }
          else
            {
              doBeforeAction();
              return setTokval(yytext_[0], DBGMSG("Not %s\n"), lvalp);
            }
          break;
        case L'<':
          advance();
          if ((cc=peekChar()) == L'=')
            {
              // less or equal: <=
              advance();
              doBeforeAction();
              return setTokval(TOK_LESS_EQUAL, DBGMSG("Less or equal %s\n"), lvalp);
            }
          else if (cc == L'>')
            {
              // not equal: <>
              advance();
              doBeforeAction();
              return setTokval(TOK_NOT_EQUAL, DBGMSG("Not equal %s\n"), lvalp);
            }
          else if (cc == L'<') { // MX comment or optimizer hint begin
            advance();
            cc = peekChar();
            if (cc == L'+') { // optimizer hint begin specified by <<+
              advance();
              doBeforeAction();
              return setTokval(TOK_BEGIN_HINT, DBGMSG("Hint begin %s\n"), lvalp);
            }
            else { // MX comment specified by \<\<\+[^\0\>]*\>\>
              while (cc != L'\0') {
                if (cc == WEOF) 
                  return -1;
                advance();
                if (cc == L'>' && peekChar() == L'>') {
                  advance();
                  break;
                }
                cc = peekChar();
              }
              doBeforeAction();
              addTokenToGlobalQueue(TRUE);
              // toss it away
            }
          }
          else
            {
              // less: <
              doBeforeAction();
              return setTokval(yytext_[0], DBGMSG("Less %s\n"), lvalp);
            }
          break;
        case L'*':
          advance();
          if ((cc=peekChar()) == L'*')
            {
              // exponentiation: **
              advance();
              doBeforeAction();
              return setTokval(TOK_EXPONENTIATE, 
                               DBGMSG("Exponentiation operator %s\n"), lvalp);
            }
          else if (cc == L'/') { 
            // optimizer hint end specified by */
            advance();
            doBeforeAction();
            return setTokval(TOK_END_HINT, DBGMSG("Hint end %s\n"), lvalp);
          }
          else
            {
              // times: *
              doBeforeAction();
              return setTokval(yytext_[0], DBGMSG("Separator %s\n"), lvalp);
            }
          break;
        case L':':
          advance();
          if ((cc=peekChar()) == L'"')
            {
              // a host variable specified by :\"([^\"]|(\"\"))*\"
              advance();
              while ((cc=peekAdvance()) != WEOF)
                {
                  if (cc == L'"')
                    if ((cc= peekChar()) == L'"')
                      advance();
                    else
                      {
                        doBeforeAction();
                        addTokenToGlobalQueue();
                        dbgprintf(DBGMSG("Host variable %s\n"));
                        lvalp->stringval = removeHostVarQuotes
                          (YYText(), YYLeng());
                        return (lvalp->stringval) ? HOSTVAR : invalidHostVarNonTranslatableChars(lvalp);
                      }
                }
              return prematureEOF(lvalp);
            }
          else if (U_isalphaund(cc))
            {
              // a host variable specified by :[A-Za-z_][A-Za-z0-9_]*
              do { advance(); } while (U_isalnumund(peekChar()));
              doBeforeAction();
              return setStringval(HOSTVAR, DBGMSG("Host variable %s\n"), lvalp);
            }
          else if ((SqlParser_CurrentParser->modeSpecial4()) &&
                   (U_isdigit(cc)))
            {
              // hostvar specified as :<num>
              do { advance(); } while (U_isdigit(peekChar()));
              doBeforeAction();
              return setStringval(HOSTVAR, DBGMSG("Host variable %s\n"), lvalp);
            }
          else
            {
              /* illegal character seen, not inside a
               * delimited identifier; will cause parser
               * to emit syntax error message
               */
              doBeforeAction();
              return setTokval(NON_SQLTEXT_CHARACTER, 
                               DBGMSG("Non-SQLTEXT character <%s>\n"), lvalp);
            }
          break;
        case L'?':
          advance();
          if (U_isalphaund(peekChar()))
            {
              // dynamic parameter specified by \?[A-Za-z_][A-Za-z0-9_]*
              do { advance(); } while (U_isalnumund(peekChar()));
              doBeforeAction();
              addTokenToGlobalQueue();
              dbgprintf(DBGMSG("Dynamic parameter %s\n"));
              lvalp->stringval = 
                unicodeToChar(YYText()+1,YYLeng()-1,CharInfo::ISO88591,
                              PARSERHEAP());
              return PARAMETER;
            }
          else
            {
              // dynamic parameter specified by \?
              doBeforeAction();
              addTokenToGlobalQueue();
              dbgprintf(DBGMSG("Dynamic parameter %s\n"));
              lvalp->stringval = new (PARSERHEAP()) NAString("", PARSERHEAP());
              return PARAMETER;
            }
          break;
        case L'@':
          advance();
          if ((cc=peekAdvance()) == L'A' && U_isdigit(peekChar()))
            {
              // internal arith placeholder specified by \@[A][0-9]+
              do { advance(); } while (U_isdigit(peekChar()));
              doBeforeAction();
              addTokenToGlobalQueue();
              dbgprintf(DBGMSG("Internal arith placeholder %s\n"));
              Lng32 x = NAWwcstol(YYText()+2);
              if ((*SqlParser_ParamItemList)[x-1])
                lvalp->item = (*SqlParser_ParamItemList)[x-1]
                  ->castToItemExpr();
              else
                lvalp->item = 0;
              return ARITH_PLACEHOLDER;
            }
          else if (cc == L'B' && U_isdigit(peekChar()))
            {
              // internal boolean placeholder specified by \@[B][0-9]+
              do { advance(); } while (U_isdigit(peekChar()));
              doBeforeAction();
              addTokenToGlobalQueue();
              dbgprintf(DBGMSG("Internal boolean placeholder %s\n"));
              Lng32 x = NAWwcstol(YYText()+2);
              if ((*SqlParser_ParamItemList)[x-1])
                lvalp->item = (*SqlParser_ParamItemList)[x-1]
                  ->castToItemExpr();
              else
                lvalp->item = 0;
              return BOOL_PLACEHOLDER;
            }
          else
            {
              /* illegal character seen, not inside a
               * delimited identifier; will cause parser
               * to emit syntax error message
               */
              doBeforeAction();
              return setTokval(NON_SQLTEXT_CHARACTER, 
                               DBGMSG("Non-SQLTEXT character <%s>\n"), lvalp);
            }
          break;
        case L'\\':
          advance();
          if (U_isAsciiAlpha(peekChar()))
            {
              // Allow underscores in guardian stlye names, even
              // though they are not allowed.  The illegal characters
              // will be caught later.
              //
              do { advance(); } while (U_isAsciiAlNumUnd((cc=peekChar())));
              if (cc != L'.')
                {
                  doBeforeAction();
                  // NSK system name only; e.g., \FIGARO
                  Int32 c = setStringval(BACKSLASH_SYSTEM_NAME,
                                       DBGMSG("BACKSLASH_SYSTEM_NAME name %s\n"),
                                         lvalp);
                  lvalp->stringval->toUpper();
                  return c;
                }
              else
                if (peekAdvance() == L'.' )
                {
                  if (peekChar() == L'$')
                  {
                    advance();
                    if (U_isAsciiAlpha(peekAdvance()))
                    {
                      // NSK system volume specified by \\{G}\.\${G}
                      // where G is [a-zA-Z][a-zA-Z0-9]*
                      //                  do { advance(); } while (U_isAsciiAlNum(peekChar()));
                      while (U_isAsciiAlNumUnd(peekChar()))
                        {
                          advance();
                        }
  		   
                      doBeforeAction();
                      Int32 c = setStringval(SYSTEM_VOLUME_NAME,
                                          DBGMSG("NSK sys-vol name %s\n"),
                                             lvalp);
                      lvalp->stringval->toUpper();
                      return c;
                    }
                    else
                    {
                      // invalid NSK name
                      doBeforeAction();
                      return setTokval(NON_SQLTEXT_CHARACTER,
                                      DBGMSG("Invalid NSK sys-vol name <%s>\n"), lvalp);
                    }
                  }
                  else
                  if (U_isdigit(peekAdvance()))
                  {
                    while (U_isdigit(peekChar()))
                      {
                        advance();
                      }
  		  
                    doBeforeAction();
                    Int32 c = setStringval(SYSTEM_CPU_IDENTIFIER,
                                        DBGMSG("SYSTEM CPU Idenitfier %s\n"),
                                             lvalp);
                    lvalp->stringval->toUpper();
                    return c;
                  }
                  else
                  {
                    // invalid NSK name
                    doBeforeAction();
                    return setTokval(NON_SQLTEXT_CHARACTER,
                                    DBGMSG("Invalid SYSTEM CPU Idenitfier <%s>\n"), lvalp);
                  }
                }
                else
                  {
                    // invalid NSK name
                    doBeforeAction();
                    return setTokval(NON_SQLTEXT_CHARACTER,
                                     DBGMSG("Invalid NSK sys-vol name <%s>\n"), lvalp);
                  }
            }
          else
            { 
              // invalid NSK name
              doBeforeAction();
              return setTokval(NON_SQLTEXT_CHARACTER,
                               DBGMSG("Invalid NSK sys-vol name <%s>\n"), lvalp);
 
            }
          break;
        case L'$':
          advance();
          if (U_isAsciiAlpha(peekChar()))
            {
              // Environment variable or NSK volume (DAM) name
              // of the form \${G}
              // where G is [a-zA-Z][a-zA-Z0-9]*
              //
              // Allow underscores in guardian stlye names, even
              // though they are not allowed.  The illegal characters
              // will be caught later.
              //
              do { advance(); } while (U_isAsciiAlNumUnd((cc=peekChar())));
              doBeforeAction();
              // note, returned 'identifier' includes leading dollar sign
              // parser code for ENV variables strips it off.
              Int32 c = setStringval(DOLLAR_IDENTIFIER,
                                   DBGMSG("DOLLAR IDENTIFIER name %s\n"),
                                     lvalp);
              lvalp->stringval->toUpper();
              return c;
            }
          else
            {
              // not a valid NSK name or environment variable
              doBeforeAction();
              return setTokval(NON_SQLTEXT_CHARACTER,
                               DBGMSG("Invalid NSK sys-vol name <%s>\n"), lvalp);
            }
          break;
	case L'(':
          {
	    if (NOT SqlParser_CurrentParser->modeSpecial1())
	      {
                /* externalize FORMAT functionality to regular users */
                do { advance(); } while (U_isspace(peekChar()));  // skip whitespace
                beginRun2 = mark();
                while (U_isalnumund(holdChar1=peekChar())) advance();  // end of word
                setCurrChar(0);
                keyWordEntry1 = ParKeyWords::lookupKeyWord(beginRun2);
	        setCurrChar(holdChar1);
                if (keyWordEntry1->getTokenCode() == TOK_FORMAT)  // (FORMAT...
                  {
                    // FORMAT is not a reserved word. Make sure it is used in FORMAT 'string'
                    do { advance(); } while (U_isspace(holdChar2=peekChar())); // advance and skip whitespace
                    if (holdChar2 == '\'')
                      {
                        // it is '(' in (FORMAT '...
                        retractToMark(beginRun2);
                        doBeforeAction();
                        SqlParser_ParenDepth++;
                        return aCompoundKeyword(TOK_LPAREN_BEFORE_FORMAT, lvalp);
                      }
                  }
	        else if (keyWordEntry1->getTokenCode() == TOK_DATE) // (DATE...
                  {
                    while (U_isspace(holdChar2=peekChar())) advance(); // skip whitespace
                    if (holdChar2 == '(') // '(' in (DATE (...
                      {
                        retractToMark(beginRun2);
                        doBeforeAction();
                        SqlParser_ParenDepth++;
                        return aCompoundKeyword(TOK_LPAREN_BEFORE_DATE_AND_LPAREN, lvalp);
                      }
                    else if (holdChar2 == ',')
                      {
                        do {  advance(); } while (U_isspace(peekChar())); // advance and skip whitespace
                        beginRun3 = mark();
                        while (U_isalnumund(holdChar3=peekChar())) advance(); // end of word
                        setCurrChar(0);
                        keyWordEntry2 = ParKeyWords::lookupKeyWord(beginRun3);
                        setCurrChar(holdChar3);
                        if (keyWordEntry2->getTokenCode() == TOK_FORMAT) // '(' in (DATE, FORMAT...
                          {
                            retractToMark(beginRun2);
                            doBeforeAction();
                            SqlParser_ParenDepth++;
                            return aCompoundKeyword(TOK_LPAREN_BEFORE_DATE_COMMA_AND_FORMAT, lvalp);
                          }
                      }
                  }
      		// A separator specified by [(]
                retractToMark(beginRun2);
                doBeforeAction();
                SqlParser_ParenDepth++;
                return setTokval(yytext_[0], DBGMSG("Separator %s\n"), lvalp);
                /* end of enternalize FORMAT */
                /* old code
		// A separator specified by [(]
		advance();
		doBeforeAction();
		SqlParser_ParenDepth++;
		return setTokval(yytext_[0], DBGMSG("Separator %s\n"), lvalp);
                */
	      }

            SqlParser_ParenDepth++;
	    
	    // skip whitespace
	    do 
	      { 
		advance(); 
	      }
	    while (U_isspace(cc=peekChar()));

	    beginRun2 = mark();
	    while (U_isalnumund(holdChar1=peekChar())) 
	      advance();
	    setCurrChar(0);
	    keyWordEntry1 = ParKeyWords::lookupKeyWord(beginRun2);
	    setCurrChar(holdChar1);
	    
	    if (keyWordEntry1->getTokenCode() == TOK_NAMED)
	      {
		retractToMark(beginRun2);
		doBeforeAction();
		return aCompoundKeyword(TOK_LPAREN_BEFORE_NAMED, lvalp);
	      }
	    else if (keyWordEntry1->getTokenCode() == TOK_DATE)
	      {
		// skip whitespace
		while (U_isspace(holdChar1=peekChar()))
		  { 
		    advance(); 
		  }

		if (holdChar1 == '(')
		  {
		    retractToMark(beginRun2);
		    doBeforeAction();
		    return aCompoundKeyword(TOK_LPAREN_BEFORE_DATE_AND_LPAREN, lvalp);
		  }
		else if (holdChar1 == ',')
		  {
		    // skip whitespace
		    do 
		      { 
			advance(); 
		      }
		    while (U_isspace(holdChar1=peekChar()));
		    
		    beginRun3 = mark();
		    while (U_isalnumund(holdChar1=peekChar())) 
		      advance();
		    setCurrChar(0);
		    keyWordEntry1 = ParKeyWords::lookupKeyWord(beginRun3);
		    setCurrChar(holdChar1);

		    retractToMark(beginRun2);
		    doBeforeAction();
		    if (keyWordEntry1->getTokenCode() == TOK_FORMAT)
		      return aCompoundKeyword(TOK_LPAREN_BEFORE_DATE_COMMA_AND_FORMAT, lvalp);
		    else
		      return setTokval(yytext_[0], DBGMSG("Separator %s\n"), lvalp);
		  }
		else
		  {
		    retractToMark(beginRun2);
		    doBeforeAction();

		    return setTokval(yytext_[0], DBGMSG("Separator %s\n"), lvalp);
		  }
	      }
	    else if (keyWordEntry1->getTokenCode() == TOK_FORMAT)
	      {
		retractToMark(beginRun2);
		doBeforeAction();
		return aCompoundKeyword(TOK_LPAREN_BEFORE_FORMAT, lvalp);
	      }
	    else 
	      {  
		retractToMark(beginRun2);
		doBeforeAction();

		if ((keyWordEntry1->getTokenCode() == TOK_DECIMAL) ||
		    (keyWordEntry1->getTokenCode() == TOK_NUMERIC) ||
		    (keyWordEntry1->getTokenCode() == TOK_INTEGER) ||
		    (keyWordEntry1->getTokenCode() == TOK_CHAR   ) ||
		    (keyWordEntry1->getTokenCode() == TOK_CHARACTER) ||
		    (keyWordEntry1->getTokenCode() == TOK_SMALLINT))
		  {
		    return aCompoundKeyword(TOK_LPAREN_BEFORE_DATATYPE, lvalp);
		  }
		else
		  {
		    return setTokval(yytext_[0], DBGMSG("Separator %s\n"), lvalp);
		  }
	      }
	  }
        case L'=': case L'+': case L',': case L')':
        case L'{': case L'}': 
        case L'[': case L']':
	    case L'&': case L'^': case L'~':
          // A separator specified by [=+,()\$\{\}\[\]&^~]
          advance();
          doBeforeAction();
          if (yytext_[0] == L'(')
            SqlParser_ParenDepth++;
          else if (yytext_[0] == L')')
            SqlParser_ParenDepth--;
          return setTokval(yytext_[0], DBGMSG("Separator %s\n"), lvalp);
        case L'-': 
		  advance();
          if (peekChar() == L'-')
            {
			// SQL comment specified by --
			    while (cc != L'\0') 
                  {
                    if (cc == WEOF) 
                      return -1;
                    advance();
                    if (cc == L'\n' || cc == L'\r') // EOL
                      {
                        break;
                      }
                    cc = peekChar();
                  }
                doBeforeAction();
                addTokenToGlobalQueue(TRUE);
                // toss it away
			}
			else
			{
              // A separator specified by -
              doBeforeAction();
              return setTokval(yytext_[0], DBGMSG("Separator %s\n"), lvalp);
            }
			break;
        case L'/':
          advance();
          if (peekChar() == L'*')
            {
              // C comment specified by \/\*[^\0\*]*\*\/ or
              // optimizer hint begin specified by \/\*\+
              advance();
              cc = peekChar();
              if (cc == L'+') { // optimizer hint begin specified by \/\*\+
                advance();
                doBeforeAction();
                return setTokval(TOK_BEGIN_HINT, DBGMSG("Hint begin %s\n"), lvalp);
              }
              else { // C comment specified by \/\*[^\0\*]*\*\/
                while (cc != L'\0') 
                  {
                    if (cc == WEOF) 
                      return -1;
                    advance();
                    if (cc == L'*' && peekChar() == L'/')
                      {
                        advance();
                        break;
                      }
                    cc = peekChar();
                  }
                doBeforeAction();
                addTokenToGlobalQueue(TRUE);
                // toss it away
              }
            }
          else
            {
              // A separator specified by /
              doBeforeAction();
              return setTokval(yytext_[0], DBGMSG("Separator %s\n"), lvalp);
            }
          break;
  
        case L';':
          // sql separator symbol
          advance();
          doBeforeAction();
          return setTokval(L';', DBGMSG("The separator symbol %s\n"), lvalp);
  
        case NAWchar('\0'):
          // sql end symbol
          doBeforeAction();
          return setTokval(0, DBGMSG("The end symbol %s\n"), lvalp);
  
          /*  Removing the next two case stmts and replacing with the
              previous case stmt. The next 2 case stmts cause parser problem
              and is the result of an incomplete merge from \tcr_fcs0902
              to \main path. The next two stmts are to be restored after
              completely merging that path to main.

              case L';':
              // ';' is a separtor
              advance();
              doBeforeAction();
              return setTokval(yytext_[0], DBGMSG("A separator %s\n"), lvalp);
              */

        case L'0': case L'1': case L'2': case L'3': case L'4':
        case L'5': case L'6': case L'7': case L'8': case L'9':
          // numeric prefix specified by {digit}+
          do { advance(); } while (U_isdigit(cc=peekChar()));
          end1 = mark();
          if ((cc=peekAdvance()) == L'.' && U_isdigit(peekChar()))
            {
              // numeric prefix specified by {digit}+\.{digit}+
              while (U_isdigit(cc=peekChar())) advance();
              end2 = mark();
              if (cc == L'E' || cc == L'e')
                {
                  // approx numeric specified by 
                  // {digit}+\.{digit}+[Ee][+-]?{digit}+
                  advance();
                  cc = peekAdvance();
                  if ((cc == L'+' || cc == L'-') && U_isdigit(peekChar()))
                    {
                      while (U_isdigit(peekChar())) advance();
                      doBeforeAction();
                      return anApproxNumber(lvalp);
                    }
                  else if (U_isdigit(cc))
                    {
                      // approx numeric specified by 
                      // {digit}+\.{digit}+[Ee]{digit}+
                      while (U_isdigit(peekChar())) advance();
                      doBeforeAction();
                      return anApproxNumber(lvalp);
                    }
                  else
                    {
                      // exact_numeric_with_scale specified by 
                      // {digit}+\.{digit}+
                      retractToMark(end2);
                      doBeforeAction();
                      return exactWithScale(lvalp);
                    }
                }
              else 
                {
                  // exact numeric with scale specified by
                  // {digit}+\.{digit}+
                  doBeforeAction();
                  return exactWithScale(lvalp);
                }
            }
          else if (cc == L'.' && !(U_isdigit(peekChar())))
            {
              // exact numeric with scale specified by {digit}+\.
              doBeforeAction();
              return exactWithScale(lvalp);
            }
          else if (cc == L'E' || cc == L'e')
            {
              // approx numeric specified by 
              // {digit}+[Ee][+-]?{digit}+
              cc = peekAdvance();
              if ((cc == L'+' || cc == L'-') && U_isdigit(peekChar()))
                {
                  while (U_isdigit(peekChar())) advance();
                  doBeforeAction();
                  return anApproxNumber(lvalp);
                }
              else if (U_isdigit(cc))
                {
                  // approx numeric specified by 
                  // {digit}+[Ee]{digit}+
                  while (U_isdigit(peekChar())) advance();
                  doBeforeAction();
                  return anApproxNumber(lvalp);
                }
              else
                {
                  // exact_numeric_no_scale specified by {digit}+
                  retractToMark(end1);
                  doBeforeAction();
                  return exactNoScale(lvalp);
                }
            }
          else
            {
              // exact_numeric_no_scale specified by {digit}+
              retractToMark(end1);
              doBeforeAction();
              return exactNoScale(lvalp);
            }
          break;
        case L' ': case L'\t': case L'\n': case L'\r': case L'\f': case L'\v':
          // white space specified by [ \t\n\r\f\v]+
          do { advance(); } while (U_isspace(peekChar()));
          doBeforeAction();
          addTokenToGlobalQueue();
          // no action means toss away white space
          break;
	case L'X':
	case L'x':
	  endp = mark();
	  advance();
	  cc = peekChar();
	  if ( cc == L'\'' ){
	      return constructStringLiteralWithCharSet(TRUE, 
						       SqlParser_DEFAULT_CHARSET,
                                                       //CharInfo::BINARY,
						       lvalp,
						       L'\'');
	  }
	  else { 
	    retractToMark(endp);
	    cc = peekChar();
	  }
        case L'A': case L'B':            case L'D': case L'E':
        case L'F':            case L'H': case L'I': case L'J':
        case L'K': case L'L': case L'M':            case L'O': 
        case L'Q': case L'R': case L'S': case L'T': 
        case L'U': case L'V': case L'W': case L'Y': 
        case L'Z':
        case L'a': case L'b':            case L'd': case L'e':
        case L'f':            case L'h': case L'i': case L'j':
        case L'k': case L'l': case L'm':            case L'o': 
        case L'q': case L'r': case L's': case L't': 
        case L'u': case L'v': case L'w': case L'y': 
        case L'z':
          // identifier prefix specified by [A-Za-z]
          do { advance(); } while (U_isalnumund(holdChar1=peekChar()));
          doBeforeAction();

          // Look for the current word in the keyword table.  If it is
          // not found, an IDENTIFIER keyword entry will be used.
          //
          keyWordEntry1 = ParKeyWords::lookupKeyWord(yytext_);

	  if ((keyWordEntry1->getTokenCode() == TOK_DATE) ||
	      (keyWordEntry1->getTokenCode() == TOK_TIME))
	    {
	      cc = holdChar1;
	      // skip white space
	      while (U_isspace(cc))
		{
		  advance();
		  cc = peekChar();
		}

	      if (cc == L'\'')
		{
		  if (keyWordEntry1->getTokenCode() == TOK_DATE)
		    return aCompoundKeyword(TOK_DATE_BEFORE_QUOTE, lvalp);
		  else
		    return aCompoundKeyword(TOK_TIME_BEFORE_QUOTE, lvalp);
		}
	      else
		return anSQLMXKeyword(keyWordEntry1->getTokenCode(), lvalp);
	    }

          if (tokIfNotCompound=notCompoundKeyword(keyWordEntry1, holdChar1, lvalp))
            return tokIfNotCompound;
          // else, a possible compound Trafodion keyword
          // save first token end
          end1 = mark();
          // skip whitespace
	  do { advance(); } while (U_isspace(cc=peekChar()));
          // check 2nd part of compound token
          if (!U_isAsciiAlpha(cc))
            {
              // 2nd part is not a keyword. 
              // return 1st token as a Trafodion keyword.
              return anSQLMXKeyword(keyWordEntry1->getTokenCode(), lvalp);
            }
          else // 2nd part may be a keyword
            {
              // scan 2nd part
              beginRun2 = mark();
              do { advance(); } while (U_isalnumund(cc=peekChar()));
              // null-terminate 2nd part
              holdChar2 = cc;
              setCurrChar(0);
              // is 2nd kwd part of a compound token?
              //
              // Look for the second word in the keyword table.  If it
              // is not found, an IDENTIFIER keyword entry will be
              // used.
              //
              keyWordEntry2 = ParKeyWords::lookupKeyWord(beginRun2);

              // restore null-termination of 2nd part.
              setCurrChar(holdChar2);
              if (!keyWordEntry2->canBeSecond())
                {
                  // suffix is not part of a compound token.
                  // return 1st part as a keyword.
                  return anSQLMXKeyword(keyWordEntry1->getTokenCode(), lvalp);
                }
              else // 2nd kwd may be part of a compound token
                {
                  switch (keyWordEntry1->getTokenCode())
                    {
                    case TOK_FOR:
                      switch (keyWordEntry2->getTokenCode())
                        {
                        default: 
                          // FOR is a simple keyword. 
                          // retract to end of kwd1.
                          retractToMark(end1);
                          return anSQLMXKeyword(keyWordEntry1->getTokenCode(), lvalp);
                        case TOK_LIBRARY:
                          *end1 = holdChar1;
                           doBeforeAction();
                           return aCompoundKeyword(TOK_FOR_LIBRARY, lvalp);
                        case TOK_ROLE:
                          *end1 = holdChar1;
                           doBeforeAction();
                           return aCompoundKeyword(TOK_FOR_ROLE, lvalp);
                        case TOK_USER:
                          *end1 = holdChar1;
                           doBeforeAction();
                           return aCompoundKeyword(TOK_FOR_USER, lvalp);
			case TOK_MAXRUNTIME:
                        case TOK_REPEATABLE:
                        case TOK_SERIALIZABLE:
                        case TOK_SKIP:				// QSTUFF
                          // un-null-terminate 1st kwd
                          *end1 = holdChar1;
                          doBeforeAction();
                          // FOR <kwd2> is a compound kwd
                          return aCompoundKeyword
                            (keyWordEntry2->getTokenCode() == TOK_REPEATABLE
                             ? TOK_FOR_REPEATABLE
                             : (keyWordEntry2->getTokenCode() == TOK_SKIP
                                ? TOK_FOR_SKIP             // QSTUFF
                                : (keyWordEntry2->getTokenCode() == TOK_SERIALIZABLE
                                   ? TOK_FOR_SERIALIZABLE : TOK_FOR_MAXRUNTIME)), lvalp);
                      
                        case TOK_READ:
                          // un-null-terminate 1st kwd
                          *end1 = holdChar1;
                          // skip whitespace
                          while (U_isspace(cc=peekChar())) advance();
                          // check 3rd part of 3-part compound token
                          if (!U_isAsciiAlpha(cc)) {
                            // 3rd part is not a keyword.
                            // return FOR READ as a compound token.
                            return aCompoundKeyword(TOK_FOR_READ, lvalp);
                          }
                          // scan 3rd part
                          beginRun3 = mark();
                          while (U_isalnumund(cc=peekChar())) advance();
                          // null-terminate 3rd part
                          holdChar3 = cc;
                          setCurrChar(0);
                          // is 3rd kwd part of a compound token?
                          //
                          // Look for the third word in the keyword
                          // table.  If it is not found, an IDENTIFIER
                          // keyword entry will be used.
                          //
                          keyWordEntry3 = ParKeyWords::lookupKeyWord(beginRun3);

                          // restore null-termination of 3rd part.
                          setCurrChar(holdChar3);
                          if (!keyWordEntry3->canBeThird() ||
                              keyWordEntry3->getTokenCode() != TOK_ONLY) {
                            // suffix is not part of a 3-part compound token,
                            // ie, FOR READ blah
                            // return 1st part as a 2-part compound token.
                            retractToMark(beginRun3);
                            doBeforeAction();
                            return aCompoundKeyword(TOK_FOR_READ, lvalp);
                          }
                          else { 
                            // FOR READ ONLY is a compound kwd
                            doBeforeAction();
                            return aCompoundKeyword(TOK_FOR_READ_ONLY, lvalp);
                          }
			}
                      break;
                    case TOK_INITIALIZE:
		      if (keyWordEntry2->getTokenCode() == TOK_MAINTAIN)
			return eitherCompoundOrSimpleKeyword
			  (keyWordEntry2->getTokenCode() == TOK_MAINTAIN,
			   TOK_INITIALIZE_MAINTAIN,
			   keyWordEntry1->getTokenCode(),
			   end1, holdChar1, lvalp);
		      else if (keyWordEntry2->getTokenCode() == TOK_SQL)
			return eitherCompoundOrSimpleKeyword
			  (keyWordEntry2->getTokenCode() == TOK_SQL,
			   TOK_INITIALIZE_SQL,
			   keyWordEntry1->getTokenCode(),
			   end1, holdChar1, lvalp);
                      else {
                          // INITIALIZE is a simple keyword.
                          // retract to end of kwd1.
                          retractToMark(end1);
                          return anSQLMXKeyword(keyWordEntry1->getTokenCode(), lvalp);
                        }
                      break;
                    case TOK_LOCK:
                      return eitherCompoundOrSimpleKeyword
                        ((SqlParser_CurrentParser->modeSpecial1() &&
			  keyWordEntry2->getTokenCode() == TOK_ROW),
                         TOK_LOCK_ROW,
                         keyWordEntry1->getTokenCode(),
                         end1, holdChar1, lvalp);
                      break;
                    case TOK_ON:
                      return eitherCompoundOrSimpleKeyword(
                              keyWordEntry2->getTokenCode() == TOK_COMMIT,
                              TOK_ON_COMMIT,
                              keyWordEntry1->getTokenCode(),
                              end1, holdChar1, lvalp);
                      break;
                    case TOK_REINITIALIZE:
		      if (keyWordEntry2->getTokenCode() == TOK_MAINTAIN)
			return eitherCompoundOrSimpleKeyword
			  (keyWordEntry2->getTokenCode() == TOK_MAINTAIN,
			   TOK_REINITIALIZE_MAINTAIN,
			   keyWordEntry1->getTokenCode(),
			   end1, holdChar1, lvalp);
                      break;
                    case TOK_REPEATABLE:
                      return eitherCompoundOrSimpleKeyword
                        (keyWordEntry2->getTokenCode() == TOK_ACCESS ||
                         keyWordEntry2->getTokenCode() == TOK_READ,

                         keyWordEntry2->getTokenCode() == TOK_ACCESS
                         ? TOK_REPEATABLE_ACCESS : TOK_REPEATABLE_READ,
                         keyWordEntry1->getTokenCode(),		
                         end1, holdChar1, lvalp);		
                      break;		
                    case TOK_REPLICATE:		
                      return eitherCompoundOrSimpleKeyword		
                        (keyWordEntry2->getTokenCode() == TOK_PARTITION, 		
                         TOK_REPLICATE_PARTITION,
                         keyWordEntry1->getTokenCode(),
                         end1, holdChar1, lvalp);
                      break;
                    case TOK_SAMPLE:
                      switch (keyWordEntry2->getTokenCode())
                        {
                        case TOK_FIRST:
                        case TOK_PERIODIC:
                        case TOK_RANDOM:
                          // un-null-terminate 1st kwd
                          *end1 = holdChar1;
                          doBeforeAction();
                          // SAMPLE <kwd2> is a compound kwd
                          return aCompoundKeyword
                            (keyWordEntry2->getTokenCode() == TOK_FIRST
                             ? TOK_SAMPLE_FIRST
                             : (keyWordEntry2->getTokenCode() == TOK_PERIODIC
                                ? TOK_SAMPLE_PERIODIC
                                : TOK_SAMPLE_RANDOM), lvalp);
                        default:
                          // SAMPLE is a simple keyword.
                          // retract to end of kwd1.
                          retractToMark(end1);
                          return anSQLMXKeyword(keyWordEntry1->getTokenCode(), lvalp);
                        }
                      break;
                    case TOK_SEQUENCE:
                      return eitherCompoundOrSimpleKeyword(
							   keyWordEntry2->getTokenCode() == TOK_BY,
							   TOK_SEQUENCE_BY,
							   keyWordEntry1->getTokenCode(),
							   end1, holdChar1, lvalp);
                      break;
                    case TOK_SHOWDDL:
                      if (keyWordEntry2->getTokenCode() == TOK_COMPONENT)
                        return eitherCompoundOrSimpleKeyword
                          (keyWordEntry2->getTokenCode() == TOK_COMPONENT,
                           TOK_SHOWDDL_COMPONENT,
                           keyWordEntry1->getTokenCode(),
                           end1, holdChar1, lvalp);
                      else if (keyWordEntry2->getTokenCode() == TOK_ROLE)
                        return eitherCompoundOrSimpleKeyword
                          ( keyWordEntry2->getTokenCode() == TOK_ROLE,
                            TOK_SHOWDDL_ROLE,
                            keyWordEntry1->getTokenCode(),
                            end1, holdChar1, lvalp);
                      else if (keyWordEntry2->getTokenCode() == TOK_LIBRARY)
                        return eitherCompoundOrSimpleKeyword
                          ( keyWordEntry2->getTokenCode() == TOK_LIBRARY,
                            TOK_SHOWDDL_LIBRARY,
                            keyWordEntry1->getTokenCode(),
                            end1, holdChar1, lvalp);
                      else if (keyWordEntry2->getTokenCode() == TOK_SEQUENCE)
                        return eitherCompoundOrSimpleKeyword
                          ( keyWordEntry2->getTokenCode() == TOK_SEQUENCE,
                            TOK_SHOWDDL_SEQUENCE,
                            keyWordEntry1->getTokenCode(),
                            end1, holdChar1, lvalp);
                      else if (keyWordEntry2->getTokenCode() == TOK_USER)
                        return eitherCompoundOrSimpleKeyword
                          ( keyWordEntry2->getTokenCode() == TOK_USER,
                            TOK_SHOWDDL_USER,
                            keyWordEntry1->getTokenCode(),
                            end1, holdChar1, lvalp);
                      break;
                    case TOK_SERIALIZABLE:
                      return eitherCompoundOrSimpleKeyword
                        (keyWordEntry2->getTokenCode() == TOK_ACCESS,
                         TOK_SERIALIZABLE_ACCESS,
                         keyWordEntry1->getTokenCode(),
                         end1, holdChar1, lvalp);
                      break;
                    case TOK_SKIP:				// QSTUFF
                      if (keyWordEntry2->getTokenCode() == TOK_CONFLICT){
			// un-null-terminate 1st kwd
			*end1 = holdChar1;
			// skip whitespace
			while (U_isspace(cc=peekChar())) advance();
			// check 3rd part of 3-part compound token
			if (!U_isAsciiAlpha(cc)) {
			  // 3rd part is not a keyword.
			  // return SKIP CONFLICT as a compound token.
			  return aCompoundKeyword(TOK_SKIP_CONFLICT_ACCESS, lvalp);
			}
			// scan 3rd part
			beginRun3 = mark();
			while (U_isalnumund(cc=peekChar())) advance();
			// null-terminate 3rd part
			holdChar3 = cc;
			setCurrChar(0);
			// is 3rd kwd part of a compound token?
			//
			// Look for the third word in the keyword
			// table.  If it is not found, an IDENTIFIER
			// keyword entry will be used.
			//
			keyWordEntry3 = ParKeyWords::lookupKeyWord(beginRun3);

			// restore null-termination of 3rd part.
			setCurrChar(holdChar3);
			if (keyWordEntry3->getTokenCode() != TOK_ACCESS) {
			  retractToMark(beginRun3);
			  doBeforeAction();
			  return aCompoundKeyword(TOK_SKIP_CONFLICT_ACCESS, lvalp);
			}
			else { 
			  // SKIP CONFLICT ACCESS a compound kwd
			  doBeforeAction();
			  return aCompoundKeyword(TOK_SKIP_CONFLICT_ACCESS, lvalp);
			}
                      } else {
                        // SKIP is a simple keyword. 
                        // retract to end of kwd1.
                        retractToMark(end1);
                        return anSQLMXKeyword(keyWordEntry1->getTokenCode(), lvalp);
                      }
                      break;
                    case TOK_UNION:
                      return eitherCompoundOrSimpleKeyword
                        (keyWordEntry2->getTokenCode() == TOK_JOIN,
                         TOK_UNION_JOIN,
                         keyWordEntry1->getTokenCode(),
                         end1, holdChar1, lvalp);
                      break;
                    case TOK_UPDATE:
                      return eitherCompoundOrSimpleKeyword(
							   keyWordEntry2->getTokenCode() == TOK_LOB,
							   TOK_UPDATE_LOB,
							   keyWordEntry1->getTokenCode(),
							   end1, holdChar1, lvalp);
                      break;
                    default:
                      Int32 BadCompoundKeywordPrefix=0;
                      assert(BadCompoundKeywordPrefix);
                    }
                }
            }
          break;
        default:
          if (U_isalphaund(cc))
            {
              // identifier prefix specified by {letter|_}
              do { advance(); } while (U_isalnumund(peekChar()));
              doBeforeAction();
              // an identifier specified by {letter|_}{letter|_|digit}*
              return anIdentifier(lvalp);
            }
          else if (U_isspace(cc))
            {
              // toss away white space
              do { advance(); } while (U_isspace(peekChar()));
              doBeforeAction();
              addTokenToGlobalQueue();
            }
          else if (endOfBuffer())
            {
              // buffer is empty. make sure we start a new run.
              yytext_ = mark();
              break;
            }
          else
            {
              /* illegal character seen, not inside a
               * delimited identifier; will cause parser
               * to emit syntax error message
               */
              advance();
              doBeforeAction();
              return setTokval(NON_SQLTEXT_CHARACTER,
                               DBGMSG("Non-SQLTEXT character <%s>\n"), lvalp);
            }
          break;
        }

      // check for end of buffer
      if (endOfBuffer()) {
        // sqlparser.y's semantic actions for create view statements
        // call ParNameLocList.cpp's ParSetTextEndPos which uses
        // ParScannedTokens to check for end of the create view stmt; so,
        // feed updateParScannedKludge with a zero len string at endOfBuffer,
        // otherwise, create view stmts may get false syntax errors.
        startRun();
        doBeforeAction();
        return setTokval(0, DBGMSG("The end symbol %s\n"), lvalp);
      }
    }
  return 0;
}


void yyULexer::reset()
{
  YY_BUFFER_STATE b = yy_current_buffer_;
  if (b)
    {
      // we have only one buffer. retract to beginning of that buffer. 
      b->yy_buf_pos = b->yy_ch_buf;
      yy_load_buffer_state();
    }
}


Int32 yyULexer::getInputPos()
{
  return currChar_ - yy_current_buffer_->yy_ch_buf - input_pos_ - 1;
}


void yyULexer::setInputPos(Int32 i)
{
  input_pos_ = i;
}

// The syntax error handler relies on this code to get at the last 2 tokens
// preceding the error point.
void yyULexer::addTokenToGlobalQueue(NABoolean isComment)
{
  ParScannedTokens->insert(ParScannedTokenPos,
                          YYLeng(),
                          ParScannedTokenOffset,
                          isComment);
  ParScannedTokenPos += YYLeng();
}

NABoolean yyULexer::isDynamicParameter(Int32 tokCod)
{
    return (tokCod == PARAMETER);
}

NABoolean yyULexer::isLiteral4HQC(Int32 tokCod) 
{
     if(tokCod == NUMERIC_LITERAL_EXACT_NO_SCALE
        ||tokCod == NUMERIC_LITERAL_EXACT_WITH_SCALE
        ||tokCod == NUMERIC_LITERAL_APPROX
        ||tokCod == TOK_MBYTE_LITERAL
        ||tokCod == TOK_SBYTE_LITERAL
        ||tokCod == QUOTED_STRING
        ||tokCod == TOK_NULL)
        return TRUE;
    else
        return FALSE;
}

