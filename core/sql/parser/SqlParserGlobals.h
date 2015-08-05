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
#ifndef SQLPARSERGLOBALS_H
#define SQLPARSERGLOBALS_H
/* -*-C++-*-
 *****************************************************************************
 * File:         SqlParserGlobals.h
 * Description:	 Sql Parser and Lexer globals
//
 *****************************************************************************
 */


#include "NAWinNT.h"


// NOTE 1:
// These globals should really be made data members of class Parser,
// and a single instance (or a stack) of Parser should be made global.


// First define Parser globals that need to reside in common.lib
// rather than parser.lib (so that DLLs which don't use parser.lib will link)
#include "SqlParserGlobalsCmn.h"

#undef GLOB_
#undef INIT_
#ifdef SQLPARSERGLOBALS__INITIALIZE
  #define GLOB_		THREAD_P
  #define INIT_(val)	= val
#else
  #define GLOB_		extern THREAD_P
  #define INIT_(val)
#endif


#ifdef SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
  class ComDiagsArea;	// Forward reference to keep dependencies to a minimum
  #include "parser.h"
  GLOB_ ComDiagsArea		*SqlParser_Diags		INIT_(NULL);
  GLOB_ Parser			*SqlParser_CurrentParser	INIT_(NULL);
  inline CollHeap* PARSERHEAP() { return SqlParser_CurrentParser->wHeap(); }
#endif


#ifdef SQLPARSERGLOBALS_LEX_AND_PARSE
  void  ResetLexer();
  void  yyerror(const char *errtext);
  Int32   yylex(YYSTYPE *lvalp);
  Int32   yyparse();

  class ExprNode;	// Forward refs to keep #include dependencies minimal
  class ItemExpr;
  class ItemExprList;
  #include "BaseTypes.h"
  #include "charinfo.h"
  #include "DefaultConstants.h"
  #include "nawstring.h"

  // This was added to solve Genesis 10-980423-3834.
  // It extends tokval by returning not just the token value/code/enum,
  // but also the original, case-preserved text.
  struct TokvalPlusYYText {
    Int32		   tokval;	// overlays "int tokval" in %union of .y file
    const NAWchar* yytext;	// beginning of token in the input string
    Int32		   yyleng;	// length of the token
  };
  inline TokvalPlusYYText *ToTokvalPlusYYText(void *p)
  { return (TokvalPlusYYText *)p; }

  // --- old comments begin ---
  // This was added for handling non-UNICODE charsets (part of MP NCHAR work).
  // ## It might be useful to abstract it out of Lex/Parse only,
  // ## and make it available to the rest of the compiler ...
  // ## see the additional inline functions in SqlParser.y ...
  // --- old comments end ---
  struct StringvalWithCharSet {
    union {
      NAString *   stringval;	// overlays "stringval" in %union of .y file
      NAWString *  wstringval;	// overlays "wstringval" in %union of .y file
    };
    CharInfo::CharSet  charSet_;
    Int32              bytesPerChar_;
    UInt16             flags_;
    enum EStringValBitMaskType
      { eUSE_wstringval_FIELD_BIT_MASK = 0x0001  // 0 : use stringval - 1 : use wstringval
      , eHEX_STRING_LITERAL_BIT_MASK   = 0x0002  // 0 : is regular str lit - 1 : is hex str lit
      , eSTR_LIT_PREFIX_SPEC_BIT_MASK  = 0x0004  // 0 : str lit char set prefix not specified - 1 : specified
        // The following bit is used with str lit without the char set prefix - i.e. a QUOTED_STRING token - only.
      , eINFER_CS_UNKNOWN_CS_BIT_MASK  = 0x0008  // 0 : special INFER_CS UNKNOWN_CS bit is off - 1: on
      };
    NABoolean isHexStrLit() const { return (flags_ & eHEX_STRING_LITERAL_BIT_MASK) != 0; }
    NABoolean isRegStrLit() const { return ! isHexStrLit(); }
    NABoolean iswstringvalUsed() const { return (flags_ & eUSE_wstringval_FIELD_BIT_MASK) != 0; }
    NABoolean isstringvalUsed() const { return ! iswstringvalUsed(); }
    NABoolean isWithStrLitPrefix() const { return (flags_ & eSTR_LIT_PREFIX_SPEC_BIT_MASK) != 0; }
    NABoolean isWithoutStrLitPrefix() const { return ! isWithStrLitPrefix(); }
    NABoolean isInferCSUnknownCSLogicEnabled() const { return (flags_ & eINFER_CS_UNKNOWN_CS_BIT_MASK) != 0; }
    NABoolean isInferCSUnknownCSLogicDisabled() const { return ! isInferCSUnknownCSLogicEnabled(); }
    void resetflags_Bit(EStringValBitMaskType bitMask) { flags_ &= ~bitMask; }
    void setflags_Bit(EStringValBitMaskType bitMask) { flags_ |= bitMask; }
    void setflags_Bit(EStringValBitMaskType bitMask, NABoolean boolval)
    {
      if (boolval == TRUE)
        setflags_Bit(bitMask);
      else
        resetflags_Bit(bitMask);
    }
    void reset() { stringval = NULL /* or wstringval = NULL */;
      charSet_ = CharInfo::UnknownCharSet; bytesPerChar_ = 0; flags_ = 0; }
    void setInferCSUnknownCSLogicFlag() { setflags_Bit(eINFER_CS_UNKNOWN_CS_BIT_MASK); }
    void resetInferCSUnknownCSLogicFlag() { resetflags_Bit(eINFER_CS_UNKNOWN_CS_BIT_MASK); }
    void shallowCopy(const StringvalWithCharSet *rhs)
    {
      if (this == rhs)
        return;
      if (rhs->iswstringvalUsed())
        wstringval = rhs->wstringval;
      else
        stringval = rhs->stringval;
      charSet_ = rhs->charSet_;
      bytesPerChar_ = rhs->bytesPerChar_;
      flags_ = rhs->flags_;
    }
  };
  inline StringvalWithCharSet *ToStringvalWithCharSet(void *p)
  { return (StringvalWithCharSet *)p; }


  // SqlParserGlobals.h's globals cause the parser to be non-reentrant!
  // (See note 1 above for an approach to achieve reentrancy with globals.

  GLOB_ ItemExprList		   *SqlParser_ParamItemList	INIT_(NULL);
  GLOB_ Int32			    SqlParser_ParenDepth	INIT_(0);
  GLOB_ Int32			    SqlParser_WheneverClause	INIT_(FALSE);

  // this global variable returns the final parse tree
  GLOB_ ExprNode		   *TheParseTree		INIT_(NULL);

  enum SQLParserStartToken {
	  NORMAL_TOKEN=0, INTERNALEXPR_TOKEN=1, COLUMNDEF_TOKEN=2 };


#endif

#ifdef SQLPARSERGLOBALS_NAMES_AND_TOKENS
  // For computing view text, check constraint search condition text, etc.
  //
  // Flex sometimes scans ahead, so input_pos doesn't always point to
  // the scanned token -- ParScannedTokenPos is used by Lexer to keep track.
  //				   -- ParScannedTokenOffset tracks wide character offsets
  // ParScannedTokens is a circular queue with info on recently scanned tokens.
  //
  #include "ParNameLocList.h"
  #include "ParScannedTokenQueue.h"
  GLOB_ ParNameLocList		*ParNameLocListPtr		INIT_(NULL);
  GLOB_ ParNameLocList		*ParNameSavedLocListPtr		INIT_(NULL);
  GLOB_ ParNameLocList		*ParNameCTLocListPtr		INIT_(NULL);
  GLOB_ ParNameLocList		*ParNameDivByLocListPtr		INIT_(NULL);
  GLOB_ Int32			 ParScannedTokenPos		INIT_(0);
  GLOB_ Int32			 ParScannedTokenOffset		INIT_(0);
  GLOB_ Lng32		 ParScannedInputCharset		INIT_(CharInfo::UTF8);
  GLOB_ ParScannedTokenQueue	 *ParScannedTokens              INIT_(NULL);

  GLOB_ Int32                      WeAreInACreateMVStatement	 INIT_(FALSE);
  GLOB_ Int32                      ThisIsTheFirstMVQuerySelect	 INIT_(TRUE);
  
  //   Keep position of the end-of optional view column list
  GLOB_ Int32                      ParEndOfOptionalColumnListPos   INIT_(0);
  //   Keep position of the begining-of optional file options list
  GLOB_ Int32                      ParBeginingOfFileOptionsListPos INIT_(0);
  //   Keep position of the end-of optional file options list
  GLOB_ Int32                      ParEndOfFileOptionsListPos      INIT_(0);
  //   Keep position of the begining-of the MV query
  GLOB_ Int32                      ParBeginingOfMVQueryPos         INIT_(0);
  //   Keep position of the end-of the select column list 
  GLOB_ Int32                      ParEndOfSelectColumnListPos     INIT_(0);


  //   Keep position of the begining-of the query in a 'create table as'
  GLOB_ Int32                      ParBeginingOfCreateTableQueryPos INIT_(0);

  // Keep position of beginning of table attributes in a 'create table as'
  GLOB_ Int32                      ParBeginingOfCreateTableAsAttrList INIT_(0);

  // Keep position of end of table attributes in a 'create table as'
  GLOB_ Int32                      ParEndOfCreateTableAsAttrList INIT_(0);


  GLOB_ NABoolean                inCallStmt                     INIT_(FALSE);
  GLOB_ Int32                      currVarIndex                   INIT_(1);
  GLOB_ NABoolean                  inRSProxyStmt                  INIT_(FALSE);

  // For Embedded Insert testing of a Cursor
  GLOB_ Int32                      WeAreInAnEmbeddedInsert    INIT_(FALSE);

  // For LRU and Embedded Operation (for now DELETE)
  GLOB_ Int32                      WeAreInALRUOperation    INIT_(FALSE);

#endif

#ifdef SQLPARSERGLOBALS_HOSTVARS	// HvRoles.h SqlParser.y scXlat.cpp
  #include "HvRoles.h"
  GLOB_ HostVarRole_vec		 *TheHostVarRoles          INIT_(NULL);

#endif

#undef GLOB_
#undef INIT_

#include "NAString.h"
#include "nawstring.h"

#endif /* SQLPARSERGLOBALS_H */



