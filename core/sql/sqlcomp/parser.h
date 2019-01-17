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
#ifndef PARSER_H
#define PARSER_H

#include "ItemExpr.h"
#include "ItemExprList.h"
#include "NLSConversion.h"
#include "ulexer.h"
#include "SQLCLIdev.h"
#include "Collections.h"
#include "stringBuf.h"
#include "charinfo.h"
#include "QCache.h"

// forward refs
class CmpContext;
class ElemDDLColDef;
class StmtNode;
class QueryText;

class Parser
{
public:

  Parser(const CmpContext *cmpContext);
  virtual ~Parser();

  CmpContext *cmpContext();
  NAHeap *wHeap() { return wHeap_; }
  
  // locale-to-unicode conversion in parseDML requires buffer len (tcr)
  Int32 parseDML(const char *str, Int32 len,
	       CharInfo::CharSet charset,
	       ExprNode ** node,
	       Int32 token = 0,
	       ItemExprList * enl = NULL);
  // widens str to UNICODE for parsing, uses localized str for error handling

  // locale-to-unicode conversion in parseDML (for odbc/mx unicode support)
  Int32 parseDML(QueryText& txt,
               ExprNode ** node,
               Int32 token = 0,
               ItemExprList * enl = NULL);
  // widens str to UNICODE for parsing, uses localized str for error handling

  Int32 parse_w_DML(const NAWchar *str, Int32 len,
	       ExprNode ** node,
	       Int32 token = 0,
	       ItemExprList * enl = NULL
                  );
  // narrows str to locale for error handling, uses wide str for parsing

  ExprNode *parseDML(const char *str, Int32 len, CharInfo::CharSet charset);

/////////////////////////////////////////////////////////////////////
//
// The following procedures take as input a string containing
// a SQL expression and return the parse tree for it.
// 
// Parameters(ItemExpr's) could also be passed to this procedure to be
// replaced at the desired place in the string.
// An arithmetic value is specified by @A<n>, and a boolean value
// is specified as @B<n> in the string, where <n> is the parameter
// number.  It is 1-based.
// 
// For example,
//    createExprTree("@A1 + 1", 1, item_expr)
// would return a tree:
//
//       |-------------------|
//       |  BiArith node(+)  |
//       |-------------------|
//           /        \
//          /          \
// |------------|    |-----------|
// | item_expr  |    |ConstValue |
// |            |    | (value=1) |
// |------------|    |-----------|
//  the parameter     node generated
//   tree             by parser
//
//
// To pass in more than 6 parameters, insert them in a comma list
// and pass it.
//
/////////////////////////////////////////////////////////////////////

ExprNode *getExprTree(	  const char * str,
			  UInt32 strlength = 0,
			  CharInfo::CharSet strCharSet = CharInfo::UTF8,
			  Int32 num_params = 0,
			  ItemExpr * p1 = NULL,
			  ItemExpr * p2 = NULL,
			  ItemExpr * p3 = NULL,
			  ItemExpr * p4 = NULL,
			  ItemExpr * p5 = NULL,
			  ItemExpr * p6 = NULL,
			  ItemExprList * paramItemList = NULL,
	       		  Int32 internal_expr = FALSE);	// pass this as TRUE if
			   				// you know it is going
							// to be an ItemExpr

ItemExpr *getItemExprTree(const char * str,
			  UInt32 strlength = 0,
			  CharInfo::CharSet strCharSet = CharInfo::UTF8,
			  Int32 num_params = 0,
			  ItemExpr * p1 = NULL,
			  ItemExpr * p2 = NULL,
			  ItemExpr * p3 = NULL,
			  ItemExpr * p4 = NULL,
			  ItemExpr * p5 = NULL,
			  ItemExpr * p6 = NULL,
			  ItemExprList * paramItemList = NULL);

  // wide versions of the above functions; used by catman to 
  // process unicode-encoded column default value strings.
ExprNode *get_w_ExprTree(const NAWchar * str,
			  UInt32 strlength = 0,
			  Int32 num_params = 0,
			  ItemExpr * p1 = NULL,
			  ItemExpr * p2 = NULL,
			  ItemExpr * p3 = NULL,
			  ItemExpr * p4 = NULL,
			  ItemExpr * p5 = NULL,
			  ItemExpr * p6 = NULL,
			  ItemExprList * paramItemList = NULL,
	       		  Int32 internal_expr = FALSE);	// pass this as TRUE if
			   				// you know it is going
							// to be an ItemExpr

ItemExpr *get_w_ItemExprTree(const NAWchar * str,
			  UInt32 strlength = 0,
			  Int32 num_params = 0,
			  ItemExpr * p1 = NULL,
			  ItemExpr * p2 = NULL,
			  ItemExpr * p3 = NULL,
			  ItemExpr * p4 = NULL,
			  ItemExpr * p5 = NULL,
			  ItemExpr * p6 = NULL,
			  ItemExprList * paramItemList = NULL);

  // parse the column definition, called from internal stored procedure
  // component ( i.e. CmpStoredProc.C )
  
  ElemDDLColDef*  parseColumnDefinition(const char* str, size_t strLen, CharInfo::CharSet strCharSet);

  // part of interface to Unicode lexer
  yyULexer *getLexer() { return lexer; }
  Int32 yylex(YYSTYPE *lvalp ) 
  { 
    Int32 retCode = lexer ? lexer->yylex(lvalp) : 0;
    addTokenToNormalizedString(retCode);
    return retCode;
  }
  const NAWchar* YYText() { return lexer ? lexer->YYText() : WIDE_(""); }
  Int32 YYLeng() { return lexer ? lexer->YYLeng() : 0; }

  char *inputStr() { return inputBuf_ ? (char*)(inputBuf_->data()) : NULL; }
  charBuf *getInputcharBuf() { return inputBuf_; }
  size_t inputStrLen(); // size (in bytes) of inputBuf_ with trailing null characters excluded from the count
  CharInfo::CharSet inputStrCharSet() { return inputStr() == NULL ? CharInfo::UnknownCharSet : charset_; }
  NAWchar *wInputStr() { return wInputBuf_ ? wInputBuf_->data() : NULL; }
  NAWcharBuf *getInputNAWcharBuf() { return wInputBuf_; }
  size_t wInputStrLen(); // size (in NAWchars) of wInputBuf_ with trailing null characters excluded from the count

  NABoolean fixupParserInputBufAndAppendSemicolon(); // returns TRUE if new inputBuf_ is (re)allocated
  NABoolean fixupParserWInputBufAndAppendSemicolon(); // returns TRUE if new wInputBuf_ is (re)allocated

  NABoolean CharHereIsDoubleQuote(StringPos p) {
    return wInputStrLen() > p && wInputStr()[p] == NAWCHR('"');
  }

  // This replaces the global variable SqlParser_InputStr in arkcmp.
  // SqlParser_InputStr is bad news to recursive parser calls. For
  // example, when arkcmp executes "create table t(a char not null)", it
  // calls CatalogManager::executeDDL() which calls CatCommand() which
  // parses the above statment and then calls CatCommand::execute()
  // which eventually calls CatAddNotNullConstraint() which calls the
  // parser again to process "alter table t add constraint blah check
  // (a is not null)". The partially unicode-enabled parser does the
  // unicode conversion of the input string very late: just before parsing.
  // This conversion may require memory allocation and deallocation. The
  // end result can be a ComASSERT() failure and possibly an arkcmp crash.
  // (tcr)

  void reset(NABoolean on_entry_reset_was_needed = FALSE);

  // set to oneof: NORMAL_TOKEN=0, INTERNALEXPR_TOKEN=1, COLUMNDEF_TOKEN=2;
  // used by the catalog manager for scanning/parsing odd stuff like:
  // "CAST('<minvalue>' AS CHAR(n))" (tcr)
  Int32 internalExpr_;

  // the original client locale's character set; used by ulexer to convert
  // unicode string literals back to their original multibyte char form.
  CharInfo::CharSet charset_;
  CharInfo::CharSet initialInputCharSet_;

  // if this is not set to UnknownCharSet, then it is used during col create if one
  // is not explicitly specified.
  CharInfo::CharSet defaultColCharset_;

  CharInfo::CharSet defaultColCharset() { return defaultColCharset_;}

  void setmodeSpecial1(NABoolean v) { modeSpecial1_ = v; }
  NABoolean modeSpecial1() { return modeSpecial1_; }
  void setmodeSpecial4(NABoolean v) { modeSpecial4_ = v; }
  NABoolean modeSpecial4() { return modeSpecial4_; }
 
  void pushHasOlapFunctions(NABoolean v) { hasOlapFunctions_.insert( v ); }
  NABoolean topHasOlapFunctions() { return hasOlapFunctions_[hasOlapFunctions_.entries()-1]; }
  void setTopHasOlapFunctions( NABoolean v) { hasOlapFunctions_[hasOlapFunctions_.entries()-1] = v; }
  NABoolean popHasOlapFunctions() { return hasOlapFunctions_.removeAt( hasOlapFunctions_.entries() - 1 ); }
  void clearHasOlapFunctions() {hasOlapFunctions_.clear();}
  Int32 hasOlapFunctionsEntries() { return hasOlapFunctions_.entries(); }
  void pushHasTDFunctions(NABoolean v) { hasTDFunctions_.insert( v ); }
  NABoolean topHasTDFunctions() { return hasTDFunctions_[hasTDFunctions_.entries()-1]; }
  void setTopHasTDFunctions( NABoolean v) { hasTDFunctions_[hasTDFunctions_.entries()-1] = v; }
  NABoolean popHasTDFunctions() { return hasTDFunctions_.removeAt( hasTDFunctions_.entries() - 1 ); }
  void clearHasTDFunctions() {hasTDFunctions_.clear();}
  Int32 hasTDFunctionsEntries() { return hasTDFunctions_.entries(); }

  HQCParseKey* getHQCKey()  { return HQCKey_; }
  
  void setHQCKey(HQCParseKey* k)  { HQCKey_ = k;  }
  
  void addTokenToNormalizedString(Int32 & tokCod) 
    { if(HQCKey_)HQCKey_->addTokenToNormalizedString(tokCod); }

  void FixupForUnaryNegate(BiArith* itm)
    { if(HQCKey_)HQCKey_->FixupForUnaryNegate(itm); }

  void collectItem4HQC(ItemExpr* itm)
    { if(HQCKey_)HQCKey_->collectItem4HQC(itm); }
  
  void setIsHQCCacheable(NABoolean b)
    { if(HQCKey_)HQCKey_->setIsCacheable(b);  }

  NABoolean isHQCCacheable()
    { return HQCKey_?HQCKey_->isCacheable():FALSE;  }

  NABoolean hasWithDefinition(NAString* key)
    { if(with_clauses_->contains(key) ) return TRUE;
      else return FALSE;
    }

  void insertWithDefinition(NAString* key,  RelExpr* val)
    {
       with_clauses_->insert(key,val);
    }

  RelExpr * getWithDefinition(NAString *key)
    {
       return with_clauses_->getFirstValue(key);
    }

  //////////////////////////////////////////////////////////////////////////
  // class HiveDDLInfo
  // this class contains various fields and info that is needed to process
  // a hive ddl statement. These fields are set during the parsing phase
  // and are processed after return from parser. 
  // That is done in method processHiveDDL.
  //////////////////////////////////////////////////////////////////////////
  class HiveDDLInfo 
  {
  public:
    enum ESSD // Explain/Showplan/Showshape/Display
      {
        NONE_      = 0,
        EXPLAIN_   = 1,
        SHOWPLAN_  = 2,
        SHOWSHAPE_ = 3,
        DISPLAY_   = 4,
      };

    HiveDDLInfo()
    {
      init();
    }

    void init()
    {
      disableDDLcheck_ = FALSE;
      checkForDDL_ = FALSE;
      foundDDL_ = FALSE;
      ddlObjectType_ = 0;
      ddlOperation_ = 0;
      ifExistsOrNotExists_ = FALSE;
      ddlNamePos_ = 0;
      ddlNameLen_ = 0;
      backquotedDelimFound_ = FALSE;
      essd_ = NONE_;
      essdQueryStartPos_ = 0;
    }

    void setValues(NABoolean checkForDDL,
                   Int32 ddlOperation, Int32 ddlObjectType,
                   NABoolean ifExistsOrNotExists = FALSE)
    {
      checkForDDL_         = checkForDDL;
      ddlOperation_        = ddlOperation;
      ddlObjectType_       = ddlObjectType;
      ifExistsOrNotExists_ = ifExistsOrNotExists;
    }

    void setFoundDDL(NABoolean v) 
    {
      foundDDL_ = v;
    }

    // in some cases, parser should not do hive ddl check.
    // This may happen for internal parsing, for ex, for view expansion,
    // or internal MD ddl compiles.
    NABoolean disableDDLcheck_;

    // this is set when create/drop/alter ddl keyword is seen.
    // It is later used to see if the specified name is a hive name
    // (catalog is HIVE).
    NABoolean checkForDDL_;

    // set if specified name is a valid hive name
    NABoolean foundDDL_;

    // StmtDDLonHiveObjects::Operation
    Int32 ddlOperation_;

    // StmtDDLonHiveObjects::ObjectType
    Int32 ddlObjectType_;

    // TRUE: if 'if exists' is specified for drop or truncate,
    //       or if 'if not exists' is specified for create.
    // FALSE: otherwise
    NABoolean ifExistsOrNotExists_;

    // position and length of hive name within the input string.
    Int32 ddlNamePos_;
    Int32 ddlNameLen_;

    // set if backquoted delimited name is seen (  `abc` ).
    // Valid for hive names only. 
    NABoolean backquotedDelimFound_;

    // 1, if explain query. 2, if showplan. 3, if showshape. 4, if display.
    Int32 essd_;
    NAString essdOptions_;
    Int32 essdQueryStartPos_;

    // hive ddl stmt passed in by user.
    // For direct ddl, like "drop table t...", or "alter table t..", this 
    // contains the whole statement.
    // For passthru ddl sent in via "process hive ddl 'drop table...'",
    // this contains the contents of the single quoted string ('drop table...')
    NAString userSpecifiedStmt_;
  };
  HiveDDLInfo * hiveDDLInfo_;

private:

  HQCParseKey* HQCKey_;
  
  // See notes in .C file.
  CmpContext  *cmpContext_;
  Parser      *prevParser_;

  NAHeap *wHeap_;             // Pointer to the NAHeap 
  NABoolean hasInternalHeap_; // Did Parser allocate this heap?

  // private methods for internal usage.

  // parseUtilISPCommand parse the input query for utility keyword and 
  // generate a StmtQuery ( RelRoot ( RelInternalSP ) ) tree. The tree
  // is generated in this routine to bypass the arkcmp parser, because
  // the utility stored procedure will parse the parameter. Since there
  // might be quoted strings in the parameters, arkcmp parser can't parse
  // the parameters, it might destroy the parameters. This routine returns
  // TRUE in the case of utility keyword found and tree generated. 
  // FALSE otherwise.
  NABoolean parseUtilISPCommand(const char* commandText,
                                size_t commandTextLen,
                                CharInfo::CharSet commandCharSet,
                                ExprNode** node);

  // parse input query for a Rel1 NSK DDL, UPDATE STATISTICS or special
  // CAT API requests. Create DDL Expr here
  // instead of letting it go thru the MX parser.
  NABoolean processSpecialDDL(const char* commandText,
                              size_t commandTextLen,
                              ExprNode * childNode,
                              CharInfo::CharSet commandTextCharSet,
                              ExprNode** node);
							  
  // see comments in parser.cpp file.
  NABoolean processHiveDDL(HiveDDLInfo * hiveDDLInfo, ExprNode** node);

  Int32 parseSQL(ExprNode ** node,
               Int32 token = 0,
               ItemExprList * enl = NULL);

  void ResetLexer(void);             
  yyULexer *lexer;

  charBuf    *inputBuf_;
  NAWcharBuf *wInputBuf_;

  NABoolean modeSpecial1_;
  NABoolean modeSpecial4_;

  LIST(NABoolean )  hasOlapFunctions_;
  LIST(NABoolean )  hasTDFunctions_;
  /* 
   * hashmap to save WITH clause definition 
   * key is the name of the with clause
   * value is the RelExpr structure
   */
  NAHashDictionary<NAString,RelExpr> *with_clauses_;

};

#define PARSERASSERT(b) \
	if (!(b)) { ParserAssertInternal( " " # b " ", __FILE__,__LINE__); }
#define PARSERABORT(b)  \
	if (!(b)) { ParserAbortInternal( " " # b " ", __FILE__,__LINE__); }

void ParserAssertInternal(const char*, const char*, Int32);
void ParserAbortInternal(const char*, const char*, Int32);

// The parsing routine which the preprocessor must call. 
Int32 sql_parse(const char* str, Int32 len, CharInfo::CharSet charset,
                StmtNode **stmt_node_ptr_ptr);

charBuf* parserUTF16ToCharSet(const NAWcharBuf& pr_UTF16StrBuf,
                              CollHeap*         pp_Heap, 
                              charBuf*&         pr_pOutCharSetStrBuf,
                              Int32             pv_iCharSet,
                              Int32 &           pr_iErrorcode,
                              NABoolean         pv_bAddNullAtEnd = TRUE,
                              NABoolean         pv_bAllowInvalidCodePoint = TRUE,
                              Int32 *           pp_iCharCount = NULL,
                              Int32 *           pp_iErrorByteOff = NULL);

#endif // PARSER_H
