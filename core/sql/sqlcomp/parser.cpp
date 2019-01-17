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
 * File:         parser.C
 * Description:
 *               
 *               
 * Created:      8/30/1996
 * Modified:     $ $Date: 2007/03/08 02:22:32 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */


#include <ctype.h>
#include <wchar.h>
#include "NAWinNT.h"
#include "arkcmp_parser_defs.h"
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_FLAGS
#define   SQLPARSERGLOBALS_LEX_AND_PARSE
#define   SQLPARSERGLOBALS_NADEFAULTS
#define   SQLPARSERGLOBALS_NAMES_AND_TOKENS
#include "SqlParserGlobals.h"
#include "NLSConversion.h"
#include "csconvert.h"
#include "ulexer.h"

#include "CmpContext.h"
#include "CmpStatement.h"
#include "CmpErrLog.h"
#include "HvRoles.h"
#include "NAExit.h"
#include "NAMemory.h"
#include "ParserMsg.h"
#include "parser.h"
#include "QueryText.h"
#include "RelExeUtil.h"
#include "RelMisc.h"		// for RelRoot
#include "RelStoredProc.h"	// for RelInternalSP
#include "SchemaDB.h"
#include "SqlciError.h"
#include "StmtNode.h"		// for StmtQuery and for ItemColRef.h classes
#include "str.h"
#include "CompException.h"    // for CmpInternalException
#include "ComCextdecs.h"
#include "CmpSeabaseDDL.h"

#include "StmtDDLonHiveObjects.h"

#include "logmxevent.h"

ostream &operator<<(ostream &dest, const ComDiagsArea& da);


static THREAD_P NABoolean resetIsNeeded = FALSE;
void Parser::reset(NABoolean on_entry_reset_was_needed)
{
  ResetLexer();

  // On entry to parseDML means we left that proc abnormally the last time
  // (by asserting).  However, any flags would have been reset by
  // ARKCMP_EXCEPTION_EPILOGUE(), so we need not do anything here;
  // in fact, we must leave the global flags as our caller stack has set 'em.
  //
  if (on_entry_reset_was_needed) return;

  // Do this on exit from parseDML so that callers of parseDML
  // (in particular, sql_parse as called from arkcmp/cmpmod.cpp)
  // get reset, *except* if caller is managing its own reset
  // (DELAYED_RESET, e.g. in CmpMain::sqlcomp+sqlcompCleanup, which allows
  // entire compilation to retain the flags, in particular ComObjectName calls).
  //
  if (!Get_SqlParser_Flags(DELAYED_RESET))
    Set_SqlParser_Flags(0);

  if (with_clauses_)
    with_clauses_->clear();
}

ULng32 cmmHashFunc_NAString(const NAString& str)
{
  return (ULng32) NAString::hash(str);
}


Parser::Parser(const CmpContext* cmpContext) 
  : hasOlapFunctions_(NULL),
    hasTDFunctions_(NULL)
{
  cmpContext_ = const_cast<CmpContext*>(cmpContext);

  if (cmpContext_ && ((wHeap_ = cmpContext_->statementHeap()) != NULL))
  {
    hasInternalHeap_ = FALSE;
  }
  else
  {
    // set memory upper limit - currently only used to test setjmp/longjmp logic
    char* memLimitStr = getenv("MEMORY_LIMIT_PARSERSWP_UPPER_KB");
    size_t memLimit = 0;
    if (memLimitStr != NULL)
        memLimit = (size_t) 1024 * atol(memLimitStr);
    
    // Allocate a heap for the parser to prevent memory leaks.
    hasInternalHeap_ = TRUE;    
    wHeap_ = new NAHeap("Cmp Parser Heap",
                        NAMemory::DERIVED_FROM_SYS_HEAP,
                        524288,
                        memLimit);
    wHeap_->setErrorCallback(&CmpErrLog::CmpErrLogCallback);
  }

  prevParser_ = SqlParser_CurrentParser;
  SqlParser_CurrentParser = this;
  lexer = NULL;
  inputBuf_ = NULL;
  charset_ = CharInfo::UnknownCharSet;
  initialInputCharSet_ = CharInfo::UnknownCharSet;
  wInputBuf_ = NULL;
  internalExpr_ = NORMAL_TOKEN;

  modeSpecial1_ = (CmpCommon::getDefault(MODE_SPECIAL_1) == DF_ON);
  modeSpecial4_ = (CmpCommon::getDefault(MODE_SPECIAL_4) == DF_ON);

  defaultColCharset_ = CharInfo::UnknownCharSet;
  NAString cs = CmpCommon::getDefaultString(TRAF_DEFAULT_COL_CHARSET);
  if (! cs.isNull())
    {
      defaultColCharset_ = CharInfo::getCharSetEnum(cs);
    }
  hasOlapFunctions_.setHeap(wHeap_);
  hasTDFunctions_.setHeap(wHeap_);
  clearHasOlapFunctions();

  HQCKey_ = NULL;

  Lng32 initsize = 10;
  with_clauses_ =  new (wHeap_) NAHashDictionary<NAString,RelExpr>(&cmmHashFunc_NAString, initsize , TRUE, wHeap_) ;

  hiveDDLInfo_ = new (wHeap_) HiveDDLInfo();
}

Parser::~Parser()
{
  delete lexer;

  // If a heap was allocated in the Parser constructor, then delete it
  // and the memory associated with it here.
  if (hasInternalHeap_)
  {
    delete wHeap_;
  }
  else
  {
    // These buffers were allocated from the heap associated with
    // the current statement and should be deleted here.
    NADELETE(inputBuf_, charBuf, wHeap_);
    NADELETE(wInputBuf_, NAWcharBuf, wHeap_);
  }
  SqlParser_CurrentParser = prevParser_;
}
     
CmpContext* Parser::cmpContext() 
{
  return cmpContext_;
 }

size_t Parser::inputStrLen()
{
  // Note that inputBuf_->getBufSize() returns the size of the buffer not the string
  // length - The buffer could contain the string and the string's optional trailing
  // null terminator character(s) followed by any garbage data.

  if (inputBuf_ == NULL || inputBuf_->getBufSize() <= 0 || inputBuf_->getStrLen() <= 0)
    return 0;

  return (size_t)inputBuf_->getStrLen(); // rely on the correctness of getStrLen()
}

size_t Parser::wInputStrLen()
{
  // The comments in Parser::inputStrLen() method definition are also true for wInputBuf_
  if (wInputBuf_ == NULL || wInputBuf_->getBufSize() <= 0 || wInputBuf_->getStrLen() <= 0)
    return 0;

  return (size_t)wInputBuf_->getStrLen(); // rely on the correctness of getStrLen()
}

NABoolean Parser::fixupParserInputBufAndAppendSemicolon()
{
  const unsigned char nullchar('\0');
  const unsigned char spacechar(' ');
  const unsigned char semicolon(';');
  const unsigned char minuschar('-');

  if (inputBuf_ == NULL || inputBuf_->getBufSize() <= 0 || inputStrLen() <= 0)
  {
    NADELETE(inputBuf_, charBuf, wHeap());
    inputBuf_ = new(wHeap()) charBuf(16 /* int newBufSize */, wHeap());
    inputBuf_->data()[0] = semicolon; inputBuf_->setStrLen(1);
    inputBuf_->data()[1] = nullchar;
    return TRUE; // inputBuf_ is (re)allocated
  }

  // Note that inputBuf_->getBufSize() returns the size of the buffer
  // (including the trailing null characters)
  Int32 bufferSize = inputBuf_->getBufSize();
  unsigned char *s = inputBuf_->data();

  // exclude the trailing null characters from the count
  Int32 initialInputStrLen = inputStrLen();
  Int32 i = initialInputStrLen - 1; // position of the last elements in the buffer

  // replace trailing white space with null character and
  while (i >= 0 && (isSpace8859_1(s[i]) || s[i] == nullchar))
    s[i--] = nullchar;

  // special case! Check for the terminating -- and replace them with null characters
  if ( i >= 1 && s[i] == minuschar && s[i-1] == minuschar)
  {
    s[i--] = nullchar; // replace minus with null character
    s[i--] = nullchar; // replace minus with null character
    // replace white space before the -- with null characters
    while (i >= 0 && (isSpace8859_1(s[i]) || s[i] == nullchar))
      s[i--] = nullchar;
  }

  // if there are multiple trailing white space and semicolons,
  // keep the leftmost semicolon  and replace the characters to
  // the right of that semicolon with null characters
  if (i >= 0)
  {
    Int32 ix = i; Int32 nullcharPos = -1; Int32 semicolonPos = -1;
    for (; ix >= 0 && (isSpace8859_1(s[ix]) || s[ix] == semicolon || s[ix] == nullchar); ix--)
    {
      if (s[ix] == semicolon)
        semicolonPos = ix;
      else if (s[ix] == nullchar)
        nullcharPos = ix;
    }
    if (semicolonPos != -1) // semicolon found
    {
      if (nullcharPos != -1 && nullcharPos < semicolonPos) // null chars before the semicolon
      {
        s[nullcharPos] = semicolon; // replace the null char with a semicolon
        semicolonPos = nullcharPos;
      }
      inputBuf_->setStrLen(semicolonPos+1);
      if (semicolonPos+1 < bufferSize)
      {
        s[semicolonPos+1] = nullchar;
      }
      else if (semicolonPos+1 == bufferSize)
      {
        // cannot append a null char to the existing buffer because there is
        // no more room left in the buffer - Allocate a new (bigger) buffer.
        charBuf *pNewCharBuf = new(wHeap()) charBuf(bufferSize + 16 /* Int32 newBufferSize */, wHeap());
        memcpy((void*)pNewCharBuf->data(), (const void *)inputBuf_->data(), bufferSize/*in_bytes*/); 
        pNewCharBuf->data()[bufferSize] = nullchar;
        pNewCharBuf->setStrLen(bufferSize);
        NADELETE(inputBuf_, charBuf, wHeap());
        inputBuf_ = pNewCharBuf;
        return TRUE; // inputBuf_ is (re)allocated
      }
      return FALSE;
    }
  }

  if (i == -1)
  {
    if (bufferSize >= 2) // buffer has enough room for a semicolon and a null character
    {
      s[0] = semicolon; inputBuf_->setStrLen(1);
      s[1] = nullchar;
      return FALSE;
    }
    else
    {
      NADELETE(inputBuf_, charBuf, wHeap());
      inputBuf_ = new(wHeap()) charBuf(16 /* int newBufSize */, wHeap());
      s = inputBuf_->data(); // note that inputBuf_->data() now points to a new location
      s[0] = semicolon; inputBuf_->setStrLen(1);
      s[1] = nullchar;
      return TRUE; // inputBuf_ is (re)allocated
    }
  }

  if (i >= 0 && s[i] != semicolon)
  {
    if (i + 2 < bufferSize)
    {
      // There is enough room to add a semicolon and a null terminator
      s[i+1] = semicolon; inputBuf_->setStrLen(i+2);
      s[i+2] = nullchar;
      return FALSE;
    }

    // --- Do not have enough space in the buffer to add/append a semicolon ---
    // Allocate new buffer

    NAString newInputStr(wHeap());
    newInputStr.append((const char *)s, (size_t)(i+1)); // i+1 == inputStrLen()
    newInputStr.append(semicolon);
    // NAString is always null terminated and the null terminator is excluded from
    // the count returned by the NAString length() method.
    Int32 newInputStrLen = (Int32)newInputStr.length();
    unsigned char * pNewInputStr = (unsigned char *)newInputStr.data();
    NADELETE(inputBuf_, charBuf, wHeap());
    inputBuf_ = new(wHeap()) charBuf ( newInputStrLen + 16 // int newBufferSize
                                     , wHeap()
                                     );
    // !!! IMPORTANT !!! inputBuf_ and inputBuf_->data() now have new pointer values
    // Note that inputBuf_->getBufSize() is now == newInputStr.length() + 16
    s = inputBuf_->data();
    memcpy((void*)s, (void*)pNewInputStr, (size_t)(newInputStrLen + 1));
    inputBuf_->setStrLen(newInputStrLen);
    inputBuf_->data()[inputBuf_->getStrLen()] = nullchar;
    
    return TRUE; // inputBuf_ is (re)allocated
  }
  return FALSE;
}

NABoolean Parser::fixupParserWInputBufAndAppendSemicolon()
{
  const NAWchar nullchar(0);
  const NAWchar spacechar(' ');
  const NAWchar semicolon(';');
  const NAWchar minuschar('-');

  if (wInputBuf_ == NULL || wInputBuf_->getBufSize() <= 0 || wInputStrLen() <= 0)
  {
    NADELETE(wInputBuf_, NAWcharBuf, wHeap());
    wInputBuf_ = new(wHeap()) NAWcharBuf(16 /* int newBufSize */, wHeap());
    wInputBuf_->data()[0] = semicolon; wInputBuf_->setStrLen(1);
    wInputBuf_->data()[1] = nullchar;
    return TRUE; // wInputBuf_ is (re)allocated
  }

  // Note that wInputBuf_->getBufSize() returns the size of the buffer
  // (including the trailing null characters)
  Int32 bufferSize = wInputBuf_->getBufSize();
  NAWchar *s = wInputBuf_->data();

  // exclude the trailing null characters from the count
  Int32 initialWInputStrLen = wInputStrLen();
  Int32 i = initialWInputStrLen - 1; // position of the last elements in the buffer

  // replace trailing white space with null character and
  while (i >= 0 && (isSpace8859_1(s[i]) || s[i] == nullchar))
    s[i--] = nullchar;

  // special case! Check for the terminating -- and replace them with null characters
  if ( i >= 1 && s[i] == minuschar && s[i-1] == minuschar)
  {
    s[i--] = nullchar; // replace minus with null character
    s[i--] = nullchar; // replace minus with null character
    // replace white space before the -- with null characters
    while (i >= 0 && (isSpace8859_1(s[i]) || s[i] == nullchar))
      s[i--] = nullchar;
  }

  // if there are multiple trailing white space and semicolons,
  // keep the leftmost semicolon  and replace the characters to
  // the right of that semicolon with null characters
  if (i >= 0)
  {
    Int32 ix = i; Int32 nullcharPos = -1; Int32 semicolonPos = -1;
    for (; ix >= 0 && (isSpace8859_1(s[ix]) || s[ix] == semicolon || s[ix] == nullchar); ix--)
    {
      if (s[ix] == semicolon)
        semicolonPos = ix;
      else if (s[ix] == nullchar)
        nullcharPos = ix;
    }
    if (semicolonPos != -1) // semicolon found
    {
      if (nullcharPos != -1 && nullcharPos < semicolonPos) // null chars before the semicolon
      {
        s[nullcharPos] = semicolon; // replace the null char with a semicolon
        semicolonPos = nullcharPos;
      }
      wInputBuf_->setStrLen(semicolonPos+1);
      if (semicolonPos+1 < bufferSize)
      {
        s[semicolonPos+1] = nullchar;
      }
      else if (semicolonPos+1 == bufferSize)
      {
        // cannot append a null char to the existing buffer because there is
        // no more room left in the buffer - Allocate a new (bigger) buffer.
        NAWcharBuf *pNewNAWCharBuf =
          new(wHeap()) NAWcharBuf(bufferSize + 4 /* Int32 newBufferSize */, wHeap());
        NAWstrncpy(pNewNAWCharBuf->data(), wInputBuf_->data(), bufferSize/*in_NAWchars*/); 
        pNewNAWCharBuf->data()[bufferSize/*in_NAWchars*/] = nullchar;
        pNewNAWCharBuf->setStrLen(bufferSize/*in_NAWchars*/);
        NADELETE(wInputBuf_, NAWcharBuf, wHeap());
        wInputBuf_ = pNewNAWCharBuf;
        return TRUE; // wInputBuf_ is (re)allocated
      }
      return FALSE;
    }
  }

  if (i == -1)
  {
    if (bufferSize >= 2) // buffer has enough room for a semicolon and a null character
    {
      s[0] = semicolon; wInputBuf_->setStrLen(1);
      s[1] = nullchar;
      return FALSE;
    }
    else
    {
      NADELETE(wInputBuf_, NAWcharBuf, wHeap());
      wInputBuf_ = new(wHeap()) NAWcharBuf(16 /* int newBufSize */, wHeap());
      s = wInputBuf_->data(); // note that wInputBuf_->data() now points to a new location
      s[0] = semicolon; wInputBuf_->setStrLen(1);
      s[1] = nullchar;
      return TRUE; // wInputBuf_ is (re)allocated
    }
  }

  if (i >= 0 && s[i] != semicolon)
  {
    if (i + 2 < bufferSize)
    {
      // There is enough room to add a semicolon and a null terminator
      s[i+1] = semicolon; wInputBuf_->setStrLen(i+2);
      s[i+2] = nullchar;
      return FALSE;
    }

    // --- Do not have enough space in the buffer to add/append a semicolon ---
    // Allocate a new buffer

    NAWString newInputStr(wHeap());
    newInputStr.append((const NAWchar *)s, (size_t)inputStrLen());
    newInputStr.append(semicolon);
    // NAWString is always NAWchar null terminated and the null terminator is
    // excluded from the count returned by the NAWString length() method.
    Int32 newInputStrLen = (Int32)newInputStr.length();
    const NAWchar * pNewInputStr = newInputStr.data();
    NADELETE(wInputBuf_, NAWcharBuf, wHeap());
    wInputBuf_ = new(wHeap()) NAWcharBuf ( newInputStrLen + 16 // int newBufSize
                                         , wHeap()
                                         );
    // !!! IMPORTANT !!! wInputBuf_ and wInputBuf_->data() now have new pointer values
    // Note that wInputBuf_->getBufSize() is now == newInputStr.length() + 16
    s = wInputBuf_->data();
    NAWstrncpy(s, pNewInputStr, (size_t)(newInputStrLen + 1));
    wInputBuf_->setStrLen(newInputStrLen);
    wInputBuf_->data()[wInputBuf_->getStrLen()] = nullchar;
    
    return TRUE; // wInputBuf_ is (re)allocated
  }
  return FALSE;
}

static NAWcharBuf* parserCharSetToUTF16(const charBuf& inCharBuf,
                                        CollHeap*      heap, 
                                        NAWcharBuf*&   outNAWcharBuf,
                                        Int32          inStrCharSet,
                                        Int32 &        outErrorCode,
                                        NABoolean      addNullAtEnd = TRUE,
                                        Int32 *        outCharCount = NULL,
                                        Int32 *        outErrorByteOff = NULL)
{
  NAWcharBuf * result = NULL;
  Int32 iCharCount = 0;
  Int32 iErrorByteOff = 0;
  if (outCharCount == NULL) outCharCount = &iCharCount;
  if (outErrorByteOff == NULL) outErrorByteOff = &iErrorByteOff;

  result = csetToUnicode(inCharBuf, heap, outNAWcharBuf, inStrCharSet,
                         outErrorCode, addNullAtEnd, outCharCount, outErrorByteOff);
  if (outErrorCode)
  {
    if(outErrorCode == CNV_ERR_INVALID_CHAR)
      {
      *CmpCommon::diags() << DgSqlCode(-2109)
                          << DgString0(CharInfo::getCharSetName((CharInfo::CharSet)inStrCharSet))
                          << DgString1("UCS2")
                          << DgInt0(*outCharCount) << DgInt1(*outErrorByteOff); 
      }
    else if(outErrorCode == CNV_ERR_BUFFER_OVERRUN)
      *CmpCommon::diags() << DgSqlCode(-2110)
                          << DgString0(CharInfo::getCharSetName((CharInfo::CharSet)inStrCharSet))
                          << DgString1("UCS2"); 
    else
      PARSERASSERT(false);
    outNAWcharBuf = NULL;
    return NULL;
  }

  return result;
}

charBuf* parserUTF16ToCharSet(const NAWcharBuf& pr_UTF16StrBuf,
                              CollHeap*         heap, 
                              charBuf*&         pr_pOutCharSetStrBuf,
                              Int32             inStrCharSet,
                              Int32 &           outErrorCode,
                              NABoolean         addNullAtEnd,           // default is TRUE
                              NABoolean         allowInvalidCodePoint,  // default is TRUE
                              Int32 *           outCharCount,           // default is NULL
                              Int32 *           outErrorByteOff)        // default is NULL
{
  charBuf * result = NULL;
  Int32 iCharCount = 0;
  Int32 iErrorByteOff = 0;
  if (outCharCount == NULL) outCharCount = &iCharCount;
  if (outErrorByteOff == NULL) outErrorByteOff = &iErrorByteOff;
  result = unicodeTocset(pr_UTF16StrBuf,
                         heap,
                         pr_pOutCharSetStrBuf,
                         inStrCharSet,
                         outErrorCode,
                         addNullAtEnd,
                         allowInvalidCodePoint,
                         outCharCount,
                         outErrorByteOff);
  if (outErrorCode)
  {
    if (outErrorCode == CNV_ERR_INVALID_CHAR)
      {
      *CmpCommon::diags() << DgSqlCode(-2109)
                          << DgString0(CharInfo::getCharSetName((CharInfo::CharSet)inStrCharSet))
                          << DgString1("UCS2")
                          << DgInt0(*outCharCount) << DgInt1(*outErrorByteOff);
      }
    else if (outErrorCode == CNV_ERR_BUFFER_OVERRUN)
      *CmpCommon::diags() << DgSqlCode(-2110)
                          << DgString0(CharInfo::getCharSetName((CharInfo::CharSet)inStrCharSet))
                          << DgString1("UCS2"); 
    else
      PARSERASSERT(false);
    pr_pOutCharSetStrBuf = NULL;
    return NULL;
  }

  return result;
}

// Prescan the string to detect conditions which would cause our SqlParser.y
// to either infinitely loop or read past the end of the input buffer and
// thus access-violate.
// Although sqlci/InputStmt.C does this same checking, it cannot do so for
// quoted text as seen in a (hostvar) prototype value.  This code should remain
// here, and the (much more complicated) code in InputStmt removed, IMHO.
//
static NABoolean stringScanWillTerminateInParser(const NAWchar *str,
                                                 Int32 internalExpr, Int32 sLen)
{
  // Encoded strings from GenRfork's buildEncodeTree
  // (calling via GenExpGenerator.h's createExprTree) are weird,
  // often having embedded squotes that DO terminate in Parser, so return OK.
  //
  if (internalExpr) return TRUE;

  NAWchar quote_seen = NAWCHR('\0');
  enum seen { NOT_SEEN, SEEN, SEEN_EMBEDDED };
  seen semicolon = NOT_SEEN;
  Int32 len = 0;
  for (const NAWchar *s = str; len < sLen; s++, len++)
    {
      if (quote_seen)
        if (*s == quote_seen) 
          quote_seen = NAWCHR('\0');
        else 
          { /*consume quoted character*/ }
      else if (*s == NAWCHR('\'') || *s == NAWCHR('"'))
        quote_seen = *s;
      else if (semicolon == SEEN && !NAWisspace(*s))	// && *s != NAWCHR(';')
        { semicolon = SEEN_EMBEDDED; break; }
      else if (*s == NAWCHR(';'))
        semicolon = SEEN;
    }
  if (quote_seen)
    {
      // Unmatched quote
      *SqlParser_Diags << DgSqlCode(-SQLCI_INPUT_MISSING_QUOTE) 
                       << DgWString0(NAWString(WIDE_("\n")) + str);
      return FALSE;
    }

 // compound statements use ';' as a statement separator, so
 // we cannot simply reject unquoted semicolons as an error.

  return TRUE;
}

// ------------------------------------------------------------------------
// processHiveDDL
//
// This method is called if a hive ddl statement is seen during parsing.
// When that is detected, information is set in HiveDDLInfo 
// and parsing phase errors out.
// This is needed to avoid enhancing the parser with hive ddl syntax.
// 
// For example:
//  create table hive.hive.t (a int) stored as sequencefile;
// Traf parser does not undertand 'stored as sequencefile' syntax.
// As soon as 'hive.hive.t' is detected, all relevant information is 
// stored in HiveDDLInfo class and parsing phase is terminated.
// This method then creates the needed structures so the create stmt could
// be passed on to hive api layer.
// 
// Return:  'node' contains the generated tree.
//          TRUE, if all ok.
//          FALSE, if error.
// -------------------------------------------------------------------------
NABoolean Parser::processHiveDDL(Parser::HiveDDLInfo * hiveDDLInfo,
                                 ExprNode** node)
{
  NABoolean rc = CmpSeabaseDDL::setupQueryTreeForHiveDDL
    (hiveDDLInfo,
     inputStr(),
     (CharInfo::CharSet)inputStrCharSet(),
     CmpCommon::getDefaultString(CATALOG),
     CmpCommon::getDefaultString(SCHEMA),
     node);

  TheHostVarRoles->clear();
  return rc;
}

// Parser::parseSQL is a private helper function that encapsulates most of
// the work that used to be done in Parser::parseDML. It avoids duplicating
// code shared by parseDML and parse_w_DML.
//   requires: Parser.inputStr() and Parser.wInputStr() are well-defined
//   modifies: node, 
//             Parser.{lexer,internalExpr_}
//             SqlParser_Diags, SqlParser_ParamItemList,
//             SqlParser_Flags, TheParseTree, ParScannedTokens, AllHostVars,
//	       common/SqlParserGlobals*.h LEX/PARSE globals
//   effects : parses the SQL statement whose text is given by 
//               Parser.inputStr() and Parser.wInputStr()
//               uses wInputStr() for lexing and parsing
//               uses inputStr() for error reporting
//             returns 0 if all OK, 1 otherwise
Int32 Parser::parseSQL
(ExprNode    **node,         // (OUT): parse tree if all OK
 Int32          internalExpr,  // (IN) : NORMAL_TOKEN, INTERNALEXPR_TOKEN, etc
 ItemExprList *paramItemList)// (IN) : assigned to SqlParser_ParamItemList 
{

// set the SQL text to the event logging area if the buffer there
// is empty
  cmpCurrentContext->setLogmxEventSqlText( wInputStr() );

  // Set parser globals here
  // if (ParScannedTokens == NULL)
    ParScannedTokens = new(wHeap()) ParScannedTokenQueue();
  // if (TheHostVarRoles == NULL)
    TheHostVarRoles = new(wHeap()) HostVarRole_vec(wHeap());

  // End of setting parser globals

  // The parameter internal_expr indicates that this expression
  // was created internally (e.g., by binder or generator) and is being
  // parsed to get back the corresponding parse tree.
  // The root of this tree will be the corresponding ExprNode,
  // NOT necessarily a StmtNode (which tops the tree for all normal 
  // SQL statements).
  internalExpr_ = internalExpr;
  if (internalExpr == INTERNALEXPR_TOKEN) { 
    // Set flag to indicate that we are parsing an internal expression
    // so that arbitrary precision exact numeric literals are accepted
    // by the SqlParserAux.cpp literalOfNumericPassingScale function.
    Set_SqlParser_Flags(ALLOW_ARB_PRECISION_LITERALS);
  }

#ifndef NDEBUG
  // Define this env var to the usual 1 to display all sqltext input except
  // internal expressions (casts from the generator) and
  // object-name parsing from check constraint binding.
  const char *dbg = getenv("SQLCOMP_DEBUG");
  if (!dbg) {
    // Set this to ascii '1' (or other digit) if debugging in MSDEV when
    // you don't have the env var defined (e.g. a static compile).
    static const char overrideEnv = '\0';
    dbg = &overrideEnv;
  }
  if (dbg && *dbg != '\0' && *dbg != '0')
    if (!internalExpr || *dbg != '1')		// internal-expr's
      {
        NAString tmp(inputStr());
        tmp.remove(6);
        if (tmp != "TABLE " || *dbg == '9')	// check constraint binding
          {
            NAString pretty(inputStr());
            PrettifySqlText(pretty);
            LineBreakSqlText(pretty);
            cout << pretty << endl;
          }
      }
#endif

  // if using special DDL or requesing DDL for SQL/MP objects, generate 
  // DDLExpr node now. 
  if (!internalExpr)
  {
    if (processSpecialDDL(inputStr(),
                          inputStrLen(),
                          NULL,
                          (CharInfo::CharSet)inputStrCharSet(),
                          node))
      {
      
        // Either an error or special DDL found
	TheHostVarRoles->clear();

	if (*node == NULL)
	  return 1; // error
	else
	  return 0; // special DDL found and node has been generated
      }
  }

  // Rewrite the utility commands into internal stored procedure commands.
  ExprNode * utilISPNode = NULL;
  if (!internalExpr)
  {
    parseUtilISPCommand(inputStr(),
                        inputStrLen(),
                        (CharInfo::CharSet)inputStrCharSet(),
                        &utilISPNode);
  }

  // Mark the compiler's common diags area:
  // This is because compiler might call the other routines in compiler 
  // for ExprNode constructors that might put in the errors into 
  // CmpCommon::diags() area. So the CmpCommon::diags()
  // is marked here and at the end merged into the SqlParser_Diags
  // (the diags area maintained by parser).
  //
  Lng32 diagsMark = CmpCommon::diags()->mark();
  Lng32 initialErrCnt = SqlParser_Diags->getNumber(DgSqlCode::ERROR_);
  Int32 parseError = 1;			// error

  // This static flag will be TRUE on entry if a previous yyparse ComASSERTed
  // (longjmp'd), which the try block below does *NOT* catch...
  if ( cmpContext() )
  {
      if ( cmpContext()->getParserResetIsNeeded() )    reset( TRUE );
      else cmpContext()->setParserResetIsNeeded( TRUE );
  }
  else
  {
      if ( resetIsNeeded )       reset( TRUE );
      else resetIsNeeded = TRUE ;
  }

  // SqlParser_Diags is initialized elsewhere, not here.
  SqlParser_NADefaults_Glob =
    ActiveSchemaDB()->getDefaults().getSqlParser_NADefaults();
  SqlParser_ParamItemList = paramItemList;
  SqlParser_ParenDepth = 0;
  SqlParser_WheneverClause = FALSE;
  TheParseTree = NULL;

  // SqlParser_Flags is *not* initialized prior to calling yyparse,
  // it's only reset to zero *afterwards*.
  // This allows Binder/Catman/DDL-Rfork to set flags before calling Parser.

  // Only internal *module* is trusted, not internal mdf...
  if (cmpContext() &&
      ((cmpContext()->internalCompile() == CmpContext::INTERNAL_MODULENAME)||
       (cmpContext()->statement() && cmpContext()->statement()->isSMDRecompile())))
    Set_SqlParser_Flags(ALLOW_SPECIALTABLETYPE);
    
  if ( internalExpr == INTERNALEXPR_TOKEN )
    Set_SqlParser_Flags(ALLOW_UNKNOWN_CHARSET);

  try
    {      
      if (wInputStr() &&
          stringScanWillTerminateInParser(wInputStr(), internalExpr, 
                                          wInputStrLen()))
        {
          // convert str to Unicode
          delete lexer;
          lexer = new yyULexer(wInputStr(), wInputStrLen());
          parseError = yyparse();		// yyparse returns 0 if success
        }
      else
        if ( cmpContext() ) cmpContext()->setParserResetIsNeeded( FALSE );
        else resetIsNeeded = FALSE;

      if (!parseError &&
          initialErrCnt < SqlParser_Diags->getNumber(DgSqlCode::ERROR_))
        parseError = 1;			// error
    }
  catch(EHBreakException&)
    {
      cerr << "Parser exception :" << endl;
      cerr << *SqlParser_Diags;
      NAExit(1);
    }  
  catch(...)
    {
      parseError = 1;			// error
    }                    

  // Should be impossible to satisfy this test, but just in case...
  if (parseError &&
      initialErrCnt >= SqlParser_Diags->getNumber(DgSqlCode::ERROR_))
    yyerror("");			// call before any reinit/reset

  // This marking and moving seems hokey to me now:
  // Here we're moving diags that were inserted into common after the mark
  // (i.e., by compiler components during this parse) --
  // moving those diags into the parser area,
  // to follow diags put into there during this parse.
  // Then we copy all the parser diags back to the common diags
  // appending after the original mark (to which common diags were rewound).
  // Seems like we could forgo the diags mark above and the rewindAndMerge here,
  // doing just the mergeAfter, with no loss of information.
  //
  // It is the common diags that end up getting displayed.
  //
  CmpCommon::diags()->rewindAndMergeIfDifferent(diagsMark, SqlParser_Diags);
  CmpCommon::diags()->mergeAfter(*SqlParser_Diags);

  // Reinitialize our globals (failing to do this sometimes results in 
  // spurious error messages!)

  if ( cmpContext() )
    {
      if ( cmpContext()->getParserResetIsNeeded() )
      {
         reset();
         cmpContext()->setParserResetIsNeeded( FALSE );
      }
    }
  else
    {
       if ( resetIsNeeded )  { reset() ; resetIsNeeded = FALSE ; }
    }

  if (parseError)
    {
      delete TheParseTree; 
      TheParseTree = NULL;
    }

  // if this query generated a utilISPNode but is also recognized by sql
  // parser, then use the sql parser generated node.
  // If sql parser doesn't recognize it, clear diags area and return the
  // utilISP node.
  if (utilISPNode)
    {
      if (parseError)
	{
	  CmpCommon::diags()->clear();
	  *node = utilISPNode;
	  parseError = 0;
	}
      else
	{
	  delete utilISPNode;
	  *node = TheParseTree;
	}
    }
  else if (SqlParser_CurrentParser->hiveDDLInfo_->foundDDL_)
    {
      // if a hive ddl object was found during parsing, generate ddl expr tree.
      // foundDDL_ could be set during successful parsing as well as for
      // a query which gave a syntax error.
      if (TheParseTree)
        delete TheParseTree; 
      TheParseTree = NULL;
      *node = NULL;
      
      if ((processHiveDDL(SqlParser_CurrentParser->hiveDDLInfo_, node)) &&
          (*node != NULL))
        parseError = 0; // hive DDL found and node has been generated
      else
        parseError = 1; // error
    }
  else if (SqlParser_CurrentParser->hiveDDLInfo_->backquotedDelimFound_)
    {
      // backquote delim identifier only valid for hive objects.
      if (TheParseTree)
        delete TheParseTree; 
      TheParseTree = NULL;
      parseError = 1;
    }
  else
    {
      *node = TheParseTree;
    }

  return parseError;
}

// parseDML widens the locale-based str and scans & parses it
Int32 Parser::parseDML(const char *instr, Int32 inlen,
                       CharInfo::CharSet charset,
                       ExprNode **node,
                       Int32 internalExpr, 
                       ItemExprList *paramItemList)
{
  initialInputCharSet_ = charset;

  if (charset == CharInfo::UCS2 && wInputBuf_ != NULL)
  {
    PARSERASSERT(wInputBuf_->data() != (NAWchar *)instr);
    PARSERASSERT((inlen & 1) == 0); // inlen must be an even number
    NADELETE(wInputBuf_, NAWcharBuf, wHeap());
    wInputBuf_ = NULL;
  }
  else if (charset != CharInfo::UCS2 && inputBuf_ != NULL)
  {
    PARSERASSERT(inputBuf_->data() != (unsigned char *)instr);
    NADELETE(inputBuf_, charBuf, wHeap());
    inputBuf_ = NULL;
  }

  Int32 len = 0;
  if (charset == CharInfo::UCS2)
  {
    NADELETE(wInputBuf_, NAWcharBuf, wHeap());
    size_t wInputBufSizeInNAWchars = (size_t)((inlen/*in_bytes*/ + 16) / BYTES_PER_NAWCHAR);
    wInputBuf_ = new(wHeap()) NAWcharBuf(wInputBufSizeInNAWchars, wHeap());
    wInputBuf_->setStrLen/*in_NAWchars*/(inlen/*in_bytes*/ / BYTES_PER_NAWCHAR);
    NAWstrncpy(wInputBuf_->data(), (const NAWchar *)instr, wInputBuf_->getStrLen());
    wInputBuf_->data()[wInputBuf_->getStrLen()] = 0;
    fixupParserWInputBufAndAppendSemicolon();
    len = wInputStrLen(); /* in NAWchars */

    if (inputBuf_ != NULL)
    {
      NADELETE(inputBuf_, charBuf, wHeap());
      inputBuf_ = NULL;
    }
  }
  else
  {
    NADELETE(inputBuf_, charBuf, wHeap());
    inputBuf_ = new(wHeap()) charBuf ( inlen + 16 // buffer size in bytes
                                     , wHeap()
                                     );
    memcpy((void *)inputBuf_->data(), (void *)instr, inlen/*in_bytes*/);
    inputBuf_->setStrLen(inlen);
    inputBuf_->data()[inlen] = 0;
    fixupParserInputBufAndAppendSemicolon();
    len = inputStrLen(); /* in bytes */

    if (wInputBuf_ != NULL)
    {
      NADELETE(wInputBuf_, NAWcharBuf, wHeap());
      wInputBuf_ = NULL;
    }
  }

  Int32 errorcode = 0;
  Int32 charCount = 0;
  Int32 errorByteOff = 0;


  if (inputBuf_) {
    switch (charset) {
    case SQLCHARSETCODE_ISO88591:
      // inputBuf_ was already allocated and fixed up at the beginning of the routine
      wInputBuf_ = ISO88591ToUnicode(*inputBuf_, wHeap(), wInputBuf_);
      break;
    case SQLCHARSETCODE_UCS2:
      // wInputBuf_ was already allocated and fixed up at the beginning of the routine
      // inputBuf_ == NULL
      inputBuf_ = unicodeToISO88591(*wInputBuf_, wHeap(), inputBuf_);
      break;
    case SQLCHARSETCODE_EUCJP:
    case SQLCHARSETCODE_SJIS:
    case SQLCHARSETCODE_GB18030:
    case SQLCHARSETCODE_GB2312:
    case SQLCHARSETCODE_GBK:
    case SQLCHARSETCODE_MB_KSC5601:
    case SQLCHARSETCODE_BIG5:
    case SQLCHARSETCODE_UTF8:
      // inputBuf_ was already allocated and fixed up at the beginning of the routine
      // wInputBuf_ == NULL
      wInputBuf_ = parserCharSetToUTF16(*inputBuf_, wHeap(), wInputBuf_, charset,
                                 errorcode, TRUE, &charCount, &errorByteOff);
      if (errorcode) return 1;
      break;
    default:
      { Int32 CharsetNotSupported=0; PARSERASSERT(CharsetNotSupported); }
      break;
    }
  }
  charset_ = charset; // needed by lexer

  if (wInputStrLen() > 0 && charset_ != CharInfo::UTF8)
  {
    charset_ = CharInfo::UTF8;
    NADELETE(inputBuf_, charBuf, wHeap());
    inputBuf_ = NULL; // must be set to NULL for the following call to work correctly
    inputBuf_ = parserUTF16ToCharSet ( *wInputBuf_, wHeap(), inputBuf_, charset_, errorcode
                                     , TRUE   // NABoolean addNullAtEnd
                                     , FALSE  // NABoolean allowInvalidCodePoint
                                     );
    if (errorcode) return 1;
  }
  ParScannedInputCharset = charset_;

  if (inputStr() != NULL && inputStrLen() > 0)
    fixupParserInputBufAndAppendSemicolon();
  if (wInputStr() != NULL && wInputStrLen() > 0)
    fixupParserWInputBufAndAppendSemicolon();


  // scan & parse it
  return parseSQL(node, internalExpr, paramItemList);
}

// parseDML widens the locale-based str and scans & parses it
Int32 Parser::parseDML(QueryText& txt,
                     ExprNode **node,
                     Int32 internalExpr, 
                     ItemExprList *paramItemList)
{
  initialInputCharSet_ = (CharInfo::CharSet)txt.charSet();

  // set up input string buffer. avoid SqlParser globals. (tcr)
  NADELETE(inputBuf_, charBuf, wHeap());
  NADELETE(wInputBuf_, NAWcharBuf, wHeap());

  inputBuf_ = NULL; // Set both buffers to NULL. Otherwise the conversion
                    // routines below will assume they are valid and 
                    // write on deleted memory.
  wInputBuf_ = NULL;

  Int32 len = txt.octetLength();
  while (len > 0 && txt.text()[len - 1] == 0) // exclude trailing null characters from the count
    len--;
  charset_ = (CharInfo::CharSet)txt.charSet(); // needed by lexer

  Int32 errorcode = 0;
  Int32 charCount = 0;
  Int32 errorByteOff = 0;


  switch ((SQLCHARSET_CODE)charset_) {
  case SQLCHARSETCODE_ISO88591:
    inputBuf_ = new (wHeap()) charBuf((unsigned char*)txt.text(), len); // shallow copy
    wInputBuf_ = ISO88591ToUnicode(*inputBuf_, wHeap(), wInputBuf_);
#ifndef NDEBUG
    if ( getenv("UCS2_SQL_TEXT_DEBUG") ) {
       charset_ = CharInfo::UNICODE;
    }
#endif
    break;
  case SQLCHARSETCODE_UCS2:
    wInputBuf_ = new (wHeap()) NAWcharBuf(txt.length() + 4, wHeap());
    NAWstrncpy(wInputBuf_->data(), txt.wText(), txt.length());
    wInputBuf_->data()[txt.length()] = NAWCHR('\0');
    wInputBuf_->setStrLen(txt.length());
    break;
  case SQLCHARSETCODE_EUCJP:
  case SQLCHARSETCODE_SJIS:
  case SQLCHARSETCODE_GB18030:
  case SQLCHARSETCODE_GB2312:
  case SQLCHARSETCODE_GBK:
  case SQLCHARSETCODE_MB_KSC5601:
  case SQLCHARSETCODE_BIG5:
  case SQLCHARSETCODE_UTF8:
    inputBuf_ = new (wHeap()) charBuf((unsigned char*)txt.text(), len);
    wInputBuf_ = parserCharSetToUTF16(*inputBuf_, wHeap(), wInputBuf_, charset_,
                               errorcode, TRUE, &charCount, &errorByteOff);
    if (errorcode) return 1;
    break;
  default:
    { Int32 CharsetNotSupported=0; PARSERASSERT(CharsetNotSupported); }
    break;
  }

  //*****************************************************************
  // Do NOT #ifdef or comment out this end-of-string (len  ';'  '\0') code,
  // without an extremely valid reason!
  // At least 3 submits to Redfish have been delayed due to bugs from missing
  // nul-terminator on sqltext, in ODBC and DDOL regression tests!
  //*****************************************************************
  fixupParserWInputBufAndAppendSemicolon();

  if (wInputStrLen() > 0 && initialInputCharSet_ != CharInfo::UTF8)
  {
    charset_ = CharInfo::UTF8;
    NADELETE(inputBuf_, charBuf, wHeap());
    inputBuf_ = NULL; // must be set to NULL for the following call to work correctly
    inputBuf_ = parserUTF16ToCharSet ( *wInputBuf_, wHeap(), inputBuf_, charset_, errorcode
                                     , TRUE   // NABoolean addNullAtEnd
                                     , FALSE  // NABoolean allowInvalidCodePoint
                                     );
    if (errorcode) return 1;
    ParScannedInputCharset = charset_;
  } // if  (wInputStrLen() > 0 && initialInputCharSet_ != CharInfo::UTF8)

  if (inputStr() != NULL && inputStrLen() > 0)
    fixupParserInputBufAndAppendSemicolon();
  if (wInputStr() != NULL && wInputStrLen() > 0)
    fixupParserWInputBufAndAppendSemicolon();


  // scan & parse it
  return parseSQL(node, internalExpr, paramItemList);
}

// str is a unicode-encoded SQL statement (or stmt fragment);
// scan and parse str; narrow str to the given charset when doing
// other text processing stuff, such as, error reporting, etc
Int32 Parser::parse_w_DML(const NAWchar *instr, Int32 inlen,
		     ExprNode **node,
		     Int32 internalExpr, 
		     ItemExprList *paramItemList
                        )
{
  initialInputCharSet_ = CharInfo::UCS2;

  if (wInputBuf_ != NULL)
  {
    PARSERASSERT(wInputBuf_->data() != instr);
    NADELETE(wInputBuf_, NAWcharBuf, wHeap());
  }
  wInputBuf_ = new (wHeap()) NAWcharBuf ( inlen + 4 // extra space for semicolon and null characters
                                        , wHeap()
                                        );
  NAWstrncpy(wInputBuf_->data(), instr, inlen);
  // Fill the remaining with null characters
  wInputBuf_->zeroOutBuf(inlen/*Int32 startPos*/);
  wInputBuf_->setStrLen(inlen);
  fixupParserWInputBufAndAppendSemicolon();

  // set up input string buffer. avoid SqlParser globals. (tcr)
  NADELETE(inputBuf_, charBuf, wHeap());
  inputBuf_ = NULL;

  charset_ = CharInfo::UCS2; // needed by lexer
  if (wInputStrLen() > 0)
  {
    charset_ = CharInfo::UTF8; // needed by lexer
    Int32 errorcode = 0;
    NADELETE(inputBuf_, charBuf, wHeap());
    inputBuf_ = NULL; // set to NULL to ask parserUTF16ToCharSet to allocate a new buffer
    inputBuf_ = parserUTF16ToCharSet ( *wInputBuf_, wHeap(), inputBuf_, charset_, errorcode
                                     , TRUE   // NABoolean addNullAtEnd
                                     , FALSE  // NABoolean allowInvalidCodePoint
                                     );
    if (errorcode) return 1;
  }
  ParScannedInputCharset = charset_;

  if (inputStr() != NULL && inputStrLen() > 0)
    fixupParserInputBufAndAppendSemicolon();
  if (wInputStr() != NULL && wInputStrLen() > 0)
    fixupParserWInputBufAndAppendSemicolon();


  // scan & parse it
  return parseSQL(node, internalExpr, paramItemList);
}

ExprNode *Parser::parseDML(const char *str, Int32 len, CharInfo::CharSet charset)
{
  ExprNode *node = NULL;
  parseDML(str, len, charset, &node, 0, NULL);
  return node;
}

ExprNode *Parser::getExprTree(const char * str,
			      UInt32 strlength,
			      CharInfo::CharSet strCharSet,
			      Int32 num_params,
			      ItemExpr * p1,
			      ItemExpr * p2,
			      ItemExpr * p3,
			      ItemExpr * p4,
			      ItemExpr * p5,
			      ItemExpr * p6,
			      ItemExprList * otherParams,
			      Int32 internal_expr)   // getItemExprTree is caller
{
  char *newstr;
  SQLParserStartToken token = 
    (internal_expr ? INTERNALEXPR_TOKEN : NORMAL_TOKEN);

  // If strlength is passed in, use it so non-null-terminated strings
  // can be passed to parser.
  size_t newlen = ((strlength > 0) ? strlength : strlen(str));
  // Exclude trailing null characters from the count
  while (newlen > 0 && str[newlen - 1] == 0)
    newlen--;

  if (newlen >=2 && str[newlen-1] == ';' && str[newlen] == 0)
    {
      newstr = (char *)str;	  // it really is const but C++ doesn't know it
    }
  else
    {
      // add a semicolon and a null character to the end of str (required by the parser)
      newstr = new(wHeap()) char[newlen + 1 + 1];
      str_cpy_all(newstr, str, newlen);
      newstr[newlen]   = ';' ;
      newstr[newlen+1] = '\0';
      newlen++;
    }
  
  ExprNode *node = NULL;
  ItemExprList *paramItemList = NULL;
  
  // num_params refers only to the 6 params passed as separate arguments, and
  // does not include the number of entries in otherParams. Either the indivicual
  // parameters, the list, or both may be used (but typically only one or the
  // other will).
  if (num_params > 0 || (otherParams && otherParams->entries() > 0))
    {
      paramItemList = new(wHeap()) ItemExprList(wHeap());
      if (num_params >= 1)    paramItemList->insert(p1);
      if (num_params >= 2)    paramItemList->insert(p2);
      if (num_params >= 3)    paramItemList->insert(p3);
      if (num_params >= 4)    paramItemList->insert(p4);
      if (num_params >= 5)    paramItemList->insert(p5);
      if (num_params >= 6)    paramItemList->insert(p6);

      if (otherParams && otherParams->entries() > 0)
        paramItemList->insert(*otherParams);
    }

  // parseDML method resets all SqlParser_Flags.
  // save the current SqlParser_Flags and restore them after parse step.
  ULng32 saved_SqlParser_Flags = SqlParser_Flags;

  parseDML(newstr, newlen, strCharSet, &node, token, paramItemList);
  delete paramItemList;
  
  // restore the saved SqlParser_Flags 
  Set_SqlParser_Flags(saved_SqlParser_Flags);

  if (newstr != str)
    NADELETEBASIC(newstr, wHeap());

  return node;
}

ExprNode *Parser::get_w_ExprTree(const NAWchar * str, // strCharSet should be CharInfo::UCS2
			      UInt32 strlength,
			      Int32 num_params,
			      ItemExpr * p1,
			      ItemExpr * p2,
			      ItemExpr * p3,
			      ItemExpr * p4,
			      ItemExpr * p5,
			      ItemExpr * p6,
			      ItemExprList * /*paramItemList not used*/,
			      Int32 internal_expr)   // getItemExprTree is caller
{
  NAWchar *newstr;
  SQLParserStartToken token = 
    (internal_expr ? INTERNALEXPR_TOKEN : NORMAL_TOKEN);

  // If strlength is passed in, use it so non-null-terminated strings
  // can be passed to parser.
  size_t newlen = ((strlength > 0) ? strlength : NAWstrlen(str));
  // Exclude trailing null characters from the count
  while (newlen > 0 && str[newlen - 1] == 0)
    newlen--;

  if (newlen >= 2 && str[newlen-1] == NAWCHR(';') && str[newlen] == 0)
    newstr = (NAWchar *)str; // it really is const but C++ doesn't know it
  else
    {
      // add a semicolon and a null character to the end of str (required by the parser)
      newstr = new(wHeap()) NAWchar[newlen + 1 + 1];
      NAWstrncpy(newstr, str, newlen);
      newstr[newlen]   = NAWCHR(';') ;
      newstr[newlen+1] = NAWCHR('\0');
      newlen++;
    }
  
  ExprNode *node = NULL;
  ItemExprList *paramItemList = NULL;
  
  if (num_params > 0)
    {
      paramItemList = new(wHeap()) ItemExprList(wHeap());
      /**(num_params >= 1)**/ paramItemList->insert(p1);
      if (num_params >= 2)    paramItemList->insert(p2);
      if (num_params >= 3)    paramItemList->insert(p3);
      if (num_params >= 4)    paramItemList->insert(p4);
      if (num_params >= 5)    paramItemList->insert(p5);
      if (num_params >= 6)    paramItemList->insert(p6);
    }

    parse_w_DML(newstr, newlen, &node, token, paramItemList);
    delete paramItemList;
  
  if (newstr != str)
    NADELETEBASIC(newstr, wHeap());

  return node;
}

ItemExpr *Parser::getItemExprTree(const char * str,
				  UInt32 len,
				  CharInfo::CharSet strCharSet,
				  Int32 num_params,
				  ItemExpr * p1,
				  ItemExpr * p2,
				  ItemExpr * p3,
				  ItemExpr * p4,
				  ItemExpr * p5,
				  ItemExpr * p6,
				  ItemExprList * paramItemList)
{
  ExprNode *et = getExprTree(str,len,strCharSet,num_params,p1,p2,p3,p4,p5,p6,paramItemList,
  			     INTERNALEXPR_TOKEN);

  PARSERASSERT(et == NULL || 
  		(et->getOperatorType() >= ITM_FIRST_ITEM_OP &&
		 et->getOperatorType() <= ITM_LAST_ITEM_OP));
  return (ItemExpr *)et;
}

ItemExpr *Parser::get_w_ItemExprTree(const NAWchar * str,
				  UInt32 len,
				  Int32 num_params,
				  ItemExpr * p1,
				  ItemExpr * p2,
				  ItemExpr * p3,
				  ItemExpr * p4,
				  ItemExpr * p5,
				  ItemExpr * p6,
				  ItemExprList * paramItemList)
{
  ExprNode *et = get_w_ExprTree(str,len,num_params,p1,p2,p3,p4,p5,p6,paramItemList,
  			     INTERNALEXPR_TOKEN);

  PARSERASSERT(et == NULL || 
  		(et->getOperatorType() >= ITM_FIRST_ITEM_OP &&
		 et->getOperatorType() <= ITM_LAST_ITEM_OP));
  return (ItemExpr *)et;
}

ElemDDLColDef* Parser::parseColumnDefinition(const char* str, size_t strLen, CharInfo::CharSet strCharSet)
{
  ExprNode* node;
  // If strLen is passed in, use it so non-null-terminated strings can be passed to parser.
  Int32 len = (Int32)(strLen > 0 ? strLen : strlen(str)) + 2;
  char* newStr = new(wHeap()) char[len];
  sprintf(newStr, "%s;", str);
  parseDML(newStr, len, strCharSet, &node, COLUMNDEF_TOKEN, NULL);
  // parseDML is expected to always return, should not jump to other places.
  // so the following delete will always be performed.
  NADELETEBASIC(newStr, wHeap());

  return (ElemDDLColDef*)node;
}

NABoolean Parser::parseUtilISPCommand(const char* command, size_t cmdLen, CharInfo::CharSet cmdCharSet, ExprNode** node)
{
  if (cmdLen == 0)
    cmdLen = strlen(command);
  Int32 inStrLen = cmdLen;
  // Exclude trailing null characters from the count
  while (inStrLen > 0 && command[inStrLen - 1] == 0)
    inStrLen--;

  static const char* UtilISPToken[] =
  { "PURGEDATA", "POPULATE", "RECOVER", "REFRESH", "UPGRADE", "DOWNGRADE", "VALIDATE", "TRANSFORM","" };

  static const char* UtilISPName[] =
  { "sp_purgedata", "sp_populate", "sp_recover",  "sp_refresh", "sp_SchLevel", "sp_SchLevel", "sp_validate", "sp_transform","" };

  static const char* displayString = "DISPLAY";
  static const char* tokenDelimiter=" \t\r\n\0";

  NABoolean displayFound = FALSE;
  char* tempStr = new (wHeap()) char[inStrLen + 1 ];
  memcpy((void *)tempStr, (void *)command, (size_t)inStrLen);
  tempStr[inStrLen] = 0;

  char* p = strtok(tempStr, tokenDelimiter);

  // Upshift the token before comparing
  unsigned char *puc = (unsigned char *)p;
  for ( ; *puc != '\0'; puc++)
    *puc = (unsigned char)(TOUPPER(*puc));

  if (p && _stricmp(p, displayString)== 0 )
  {
    displayFound = TRUE;
    p = strtok(NULL, tokenDelimiter);
  }

  NABoolean utilISPFound = FALSE;
  NABoolean isPurgedata = FALSE;
  Int32 index = 0;
  if (p)
  {
    if (displayFound)
    {
      // Upshift the token before comparing
      puc = (unsigned char *)p;
      for ( ; *puc != '\0'; puc++)
        *puc = (unsigned char)(TOUPPER(*puc));
    }
    for ( index=0; !utilISPFound && strlen(UtilISPToken[index]) > 0 ; index++ )
      if ( _stricmp(p, UtilISPToken[index]) == 0 )
	{
	  utilISPFound = TRUE;

	  if (_stricmp(p, "PURGEDATA") == 0)
	    isPurgedata = TRUE;
	}
  }

  if (node)
    *node = NULL;

  if ( utilISPFound )
  {
    *node = NULL;
  } // utilISPFound

  NADELETEBASIC(tempStr, wHeap());
  return utilISPFound;
}

// ------------------------------------------------------------------------
// processSpecialDDL:
//
// If the request is a "special DDL request", go ahead a generate the
// DDLExpr node 
//
// Special DDL requests consist of:
//   UPDATE STATISTICS
//   HIVE DDL request
//
// return TRUE: if a special DDL request or error.
//            : if error, node returned is NULL.
// return FALSE: if need to call SQL/MX parser after return from here.
// -------------------------------------------------------------------------
NABoolean Parser::processSpecialDDL(const char* inputStr, size_t inputStrLen,
                                    ExprNode * childNode,
                                    CharInfo::CharSet inputStrCS, 
                                    ExprNode** node)
{    
  if (cmpContext() && cmpContext()->internalCompile())
    return FALSE;

  PARSERASSERT(inputStrCS != CharInfo::UCS2);

  NABoolean ustat = FALSE;   // will be TRUE if the special DDL is for Update Statistics

  if (inputStrLen == 0)
    inputStrLen = strlen(inputStr);
  Int32 newStrLen = inputStrLen;
  // Exclude trailing null characters from the count
  while (newStrLen > 0 && inputStr[newStrLen-1] == 0)
    newStrLen--;

  CharInfo::CharSet inputStrCharSet = inputStrCS;
  // Fix up input string:
  //   Get rid of leading blanks
  //   Strip off the leading "DISPLAY" if found
  //   Strip off the leading "PROCEDURE procname (...)" if found
  NAString ns(wHeap());
  if (inputStr != NULL)
    ns.append(inputStr, (size_t)newStrLen);

  // skip leading blanks
  ns = ns.strip(NAString::leading, ' ');

  // if first token is display, skip it. Remember that it was a display.
  NABoolean displayFound = FALSE;
  size_t position = ns.index("DISPLAY", 0, NAString::ignoreCase);
  if (position == 0)
    {
      // found DISPLAY. Remember it and skip it.
      displayFound = TRUE;
      ns = ns(7, ns.length()-7);
      ns = ns.strip(NAString::leading, ' ');
    }

  // Now go and see if request is a special DDL request
  NABoolean specialDDL = FALSE;
  NABoolean xnNeeded = FALSE;

  // Check for UPDATE STATISTICS
  if (ns.index("UPDATE", 0, NAString::ignoreCase) == 0)
    {
      NAString nstemp = ns;
      nstemp = nstemp(6, nstemp.length()-6); // skip over UPDATE
      nstemp = nstemp.strip(NAString::leading, ' ');
      if (nstemp.index("STATISTICS", 0, NAString::ignoreCase) == 0)
	{
	  specialDDL = TRUE; // UPDATE STATISTICS
          ustat = TRUE;

	  // do not start Xn at runtime. 
	  xnNeeded = FALSE;
	}
    }
  else if (childNode)
    {
      ustat = FALSE;
      specialDDL = TRUE;
      xnNeeded = FALSE;
    }

  // If a special DDL is found, go ahead and create a DDLExpr node
  if (specialDDL)
    {
      *node = NULL;
      DDLExpr * ddlExpr = new(CmpCommon::statementHeap()) 
	DDLExpr(childNode, (char *)ns.data(), inputStrCharSet, 
                CmpCommon::statementHeap());
      RelExpr *queryExpr = new(CmpCommon::statementHeap())
	RelRoot(ddlExpr);

      ddlExpr->xnNeeded() = xnNeeded;

      ddlExpr->specialDDL() = TRUE;

      // Indicate whether the special DDL is an Update Stats
      ddlExpr->isUstat() = ustat;

      // indicate that this is the root for the entire query
      ((RelRoot *) queryExpr)->setRootFlag(TRUE);
      
      if (displayFound)
	((RelRoot *)queryExpr)->setDisplayTree(TRUE);
      
      StmtQuery* query = new(wHeap())StmtQuery(queryExpr);

      *node = query;

      return TRUE;
    }
  
  return FALSE;
}

void Parser::ResetLexer(void)
{
  if (lexer) lexer->reset();
  ParScannedTokenPos = 0;
  ParScannedTokenOffset = 0;
  ParScannedInputCharset = SQLCHARSETCODE_UTF8;
  ParNameLocListPtr  = NULL;
}

void HQCParseKey::addTokenToNormalizedString(Int32 & tokCod)
{
  if(SqlParser_CurrentParser->getLexer()->isLiteral4HQC(tokCod))
  {
     keyText_ += "#np# ";
     NAString* literal = unicodeToChar(SqlParser_CurrentParser->YYText(), SqlParser_CurrentParser->YYLeng(), (Lng32)ParScannedInputCharset, heap_);
     CMPASSERT(literal);
     getParams().getNPLiterals().insert(*literal);
  }
  else
  {  
     NAString* tok = unicodeToChar(SqlParser_CurrentParser->YYText(), SqlParser_CurrentParser->YYLeng(), (Lng32)ParScannedInputCharset, heap_);
     if(tok) {
        //for first token which is select/insert/update/delete, it might be HQC cacheable.
        tok->toLower(); //make case insensitive
        if(nOfTokens_ == 0 
           && ( strncmp(tok->data(), "select", 6) == 0
              //HQC does not cache insert statement as SQC already did this before bind. 
              //SQC does strict NAType checking on constants while HQC does not for Insert,  
              //this will cause inconsistency. 
              //Fix launchpad bug 1421374
              //||strncmp(tok->data(), "insert", 6) == 0
              ||strncmp(tok->data(), "update", 6) == 0
              ||strncmp(tok->data(), "delete", 6) == 0) 
          )  
          setIsCacheable(TRUE);
        
         if(SqlParser_CurrentParser->getLexer()->isDynamicParameter(tokCod)) {
            NABoolean FoundInList = FALSE;
            for(CollIndex i = 0; i < HQCDynParamMap_.entries(); i++) {
                if(HQCDynParamMap_[i].original_ ==  *tok)
                {
                    keyText_ += HQCDynParamMap_[i].normalized_ + " ";
                    FoundInList = TRUE;
                    break;
                }
            }
            if(!FoundInList) {
                NAString param = "?";
                param += "param" + UnsignedToNAString(HQCDynParamMap_.entries()+1);
                keyText_ += param + " ";
                HQCDynParamMap_.insert(HQCDParamPair(*tok, param));
            }
            //not support dynamic parameter
            setIsCacheable(FALSE);
         }
         else
           keyText_ += *tok + " ";
     }
   }
   nOfTokens_++;
   isStringNormalized_ = FALSE;
}

/* JWP
//KSKSKS
NAWchar *Parser::wInputStr()
{ 
  Int32 i;
  static NAWchar *temp2 = (NAWchar *)  111111111;  // 0x069F68C7
  static NAWchar *temp3 = (NAWchar *) 1412509744;  // 0x54313030

  if (wInputBuf_ != NULL)
    {
    if (wInputBuf_->data() == NULL)
       i = 20;
    else if (   wInputBuf_->data() <= (NAWchar *) temp2 
             || wInputBuf_->data() >= (NAWchar *) temp3
            )
               i = 21;
    return wInputBuf_->data();
    }
  else
   return NULL;
}
//KSKSKS
*/

Int32 yylex(YYSTYPE *lvalp)
{
  return SqlParser_CurrentParser ? SqlParser_CurrentParser->yylex(lvalp) : 0;
}

void ParserAssertInternal(const char* condition, const char* file, Int32 num)
{
  // Put the internal error into the diags area if there is one

  *SqlParser_Diags << DgSqlCode(-3000) << DgInt0(num) <<
    DgString0(condition) << DgString1(file);

  CmpInternalException(condition, file , num).throwException();
}

void ParserAbortInternal(const char* condition, const char* file, Int32 num)
{
  cerr << "Internal error (" << condition << ") at " 
       << file << ", line " << num << ", aborting."
       << endl;
  throw EHBreakException(file, num);
}


// -----------------------------------------------------------------------
// The parsing routine which the preprocessor must call,
// as well as arkcmp/cmpmod.cpp routines.
// -----------------------------------------------------------------------
Int32 sql_parse(const char* str, Int32 len, CharInfo::CharSet charset,
	      StmtNode **stmt_node_ptr_ptr
	      /***, SqlParser_Flags_Enum flags ***/)
{
  ExprNode *node;
  Int32 result = 0;

  Parser *parser = new Parser(cmpCurrentContext);
  try {
    result = parser->parseDML(str, len, charset, &node, 0, NULL);
  } catch (...) {
    delete parser;
    throw;  // rethrow the exception
  }
  delete parser;

  *stmt_node_ptr_ptr = (StmtNode*)node;

  return result;
}

