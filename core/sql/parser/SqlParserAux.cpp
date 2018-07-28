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
/* -*++-*- 
******************************************************************************
*
* File:         SqlParserAux.cpp
* Description:  SQL Parser auxiliary methods: a logical progression from
*		Sql Parser Gnu  to  Sql Parser Yacc  to  Sql Parser Aux.
*
*		Extracted from sqlparser.y to
*               work around a c89 (v2.1) internal limit that shows up as
                  ugen: internal: Assertion failure
                  in source file 'W:\parser\SqlParser.cpp', at source line NNN
		  detected in back end file 'eval.c', at line 4653
*               when cross-compiling SQLMX for NSK
*
* Created:      4/28/94
* Language:     C++
*
*
*
******************************************************************************
*/

#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_HOSTVARS
#define   SQLPARSERGLOBALS_LEX_AND_PARSE
#define   SQLPARSERGLOBALS_NADEFAULTS
#define   SQLPARSERGLOBALS_NAMES_AND_TOKENS
#define   SQLPARSERGLOBALS_FLAGS
#include "SqlParserGlobals.h"
#include "SqlParserAux.h"

#include <errno.h>
#include "AllItemExpr.h"
#include "MiscType.h"
#include "NumericType.h"
#include "ParserMsg.h"
#include "SqlciError.h"
#include "wstr.h"
#include "nawstring.h"


// Forward references and includes for Y.tab.h (SqlParser.h)
class ExprNodePtrList;
class ForUpdateSpec;
class IntervalQualifier;
class PairOfUnsigned;
#include "CharType.h"
#include "ComSmallDefs.h"
#include "ComTransInfo.h"
#include "ComUnits.h"
#include "HvTypes.h"
#include "ElemDDLConstraintRI.h"
#include "ElemDDLPartition.h"
#include "ElemDDLParamName.h"
#include "AllElemDDL.h"
#include "AllStmtDDL.h"
#include "RelScan.h"
#include "RelUpdate.h"
#include "RelSequence.h"
#include "StmtDMLSetTransaction.h"
#include "exp_clause_derived.h"
#include <sqlparser.h> // Angled-brackets are intentional here

const UInt32 SHORT_MAX =  32767;

char    *SQLTEXT()  {return SqlParser_CurrentParser->inputStr();}
charBuf *SQLTEXTCHARBUF()  {return SqlParser_CurrentParser->getInputcharBuf();}
Lng32    SQLTEXTCHARSET() {return SqlParser_CurrentParser->inputStrCharSet();}
NAWchar *SQLTEXTW() {return SqlParser_CurrentParser->wInputStr();}
NAWcharBuf *SQLTEXTNAWCHARBUF()  {return SqlParser_CurrentParser->getInputNAWcharBuf();}


CharInfo::CharSet getStringCharSet(NAString **p)
{ return ToStringvalWithCharSet(p)->charSet_; }

CharInfo::CharSet getStringCharSet(NAWString **p)
{ return ToStringvalWithCharSet(p)->charSet_; }

NABoolean charsetMismatchError(NAString **d1, NAString **d2)
{
  CharInfo::CharSet cs1 = getStringCharSet(d1);
  CharInfo::CharSet cs2 = getStringCharSet(d2);
  if (cs1 == cs2)
    return FALSE;

  NAString ns1("CHAR(n) CHARACTER SET ");
  NAString ns2(ns1);
  ns1 += CharInfo::getCharSetName(cs1);
  ns2 += CharInfo::getCharSetName(cs2);

  // 4034 The operation ($String0 $String1 $String2) is not allowed
  *SqlParser_Diags << DgSqlCode(-4034)
    << DgString0(ns1)
    << DgString1("||")				// string concatenation operator
    << DgString2(ns2);
  return TRUE;
}

// This function prints to the user the place in the input where an
// error occurred.  This error message is based on the contents of
// SqlParser_InputStr, which is what the parser scans, and the variable
// SqlParser_InputPos, which is an index that iterates over characters in that
// buffer as the Lexer tokenizes for the parser.
// However, if, on input, SqlParser_InputStr equals NULL, then the old
// behavior of printing the argument errtext is all that happens.
//
// NB: sqlcomp/parser.h's parser class keeps two versions of the SQL input stmt.
// Parser::inputStr() is the ascii/multibyte version and wInputStr() is
// the wide (unicode) version. Input/output requires multibyte strings (at
// least in NT 4.0) The wide (Unicode) versions of the C++ stream I/O
// functions do not work on Japanese or Chinese NT! Apparently, this is an
// accepted fact of life. All I/O must be multibyte. In-memory string handling
// can be in Unicode. So, you will see this duality in this parser. Strings
// destined for internal processing (eg, by the lexer) are in Unicode. But
// strings destined for I/O are first converted to multibyte. (tcr)

void yyerror(const char *errtext)
{
  NAWchar *inputStr = SQLTEXTW();
#ifndef NDEBUG		//## tmp code...
  if (inputStr && !getenv("YYERROR_QUIET"))
    {
      cerr << "yyerror: ";
      for (Int32 i = 0; i >= -2; i--)
        {
	  const ParScannedTokenQueue::scannedTokenInfo &tok
	    = ParScannedTokens->getScannedTokenInfo(i);
	  if (tok.tokenStrLen) 
	    {
	      NAString *ti = unicodeToChar
		(&inputStr[tok.tokenStrPos], tok.tokenStrLen,
		 CharInfo::UTF8
		 , 
		 PARSERHEAP(), TRUE);
	      cerr << "<" << (ti ? ti->data() : "") << "> ";
	      delete ti;
	    }
	  else 
	    {
	      cerr << "<" << "" << "> ";
	    }
	}
      NAString *stmt = unicodeToChar
	(inputStr, NAWstrlen(inputStr), 
	 CharInfo::UTF8
	 , 
	 PARSERHEAP(), TRUE);
      cerr << " " << (stmt ? stmt->data() : "") << endl;
      delete stmt;
    }
#endif
  
  if (inputStr || strstr(errtext, "syntax") == errtext)
    {
      if ((strcmp(errtext, "")) && 
          (strstr(errtext, "syntax") != errtext))
        *SqlParser_Diags << DgSqlCode(-SQLCI_PARSER_ERROR) << DgString0(errtext);
      else
      {
	*SqlParser_Diags << DgSqlCode(-SQLCI_SYNTAX_ERROR);
	// Point to the end of offending token,
	// knowing that the Lexer has looked ahead by 2 characters
        Int32 pos = SqlParser_CurrentParser->getLexer()->getInputPos();
	StoreSyntaxError(inputStr,pos,*SqlParser_Diags,0,
			CharInfo::UTF8
			);
      }
    }
  else
    {
      // Internal parser error -- we hope never to see this one
      *SqlParser_Diags << DgSqlCode(-SQLCI_PARSER_ERROR) << DgString0(errtext);
    }

  WeAreInACreateMVStatement = FALSE;
  WeAreInAnEmbeddedInsert = FALSE;
}


// THREAD_P LimitedStack *inJoinSpec = NULL;	// can handle <STACK_LIMIT> nested Joins

// First emit syntax error 15001.
// Then if we're not in a parenthesized join-spec, also emit error 4101,
// 	"If $0~String0 is intended to be a further table reference in the
//	FROM clause, the preceding join search condition must be enclosed
//	in parentheses."
void checkError4101(NAString *badItemStr, ItemExpr *badPrevExpr)
{
  yyerror("");

  if (!((*inJoinSpec)())) return;
  if (SqlParser_ParenDepth - (*inJoinSpec)() + STACKDELTA_ENSURES_NONZERO) return;

  if (badPrevExpr && badPrevExpr->getOperatorType() == ITM_REFERENCE) {
    const ColRefName &crn = ((ColReference *)badPrevExpr)->getColRefNameObj();

    // Is it a 3-part name at most (really a tablename intended here)?
    if (crn.getCorrNameObj().getQualifiedNameObj().fullyExpanded()) return;

    // Prepend 1, 2, or 3-part tablename to correlation name
    badItemStr->prepend(crn.getColRefAsAnsiString() + " ");
  }

  *SqlParser_Diags << DgSqlCode(-4101) << DgString0(*badItemStr);
}

// Emit warning 3169 if unknown collation,
// but emit it only once per stmt for each new unknown collation name.
NABoolean maybeEmitWarning3169(CharInfo::Collation co, const NAString &nam)
{
  if (co != CharInfo::UNKNOWN_COLLATION) return FALSE;	    // no warning
  for (Lng32 i = SqlParser_Diags->getNumber(); i; i--) {
    const ComCondition &cond = (*SqlParser_Diags)[i];
    if (cond.getSQLCODE() == +3169 &&
        strcmp(cond.getOptionalString(0), nam) == 0)
      return TRUE;					    // already emitted
  }
  *SqlParser_Diags << DgSqlCode(+3169) << DgString0(nam);
  return TRUE;
}

NABoolean checkError3179(const NAType *na)
{
  if (na->getTypeQualifier() != NA_CHARACTER_TYPE) return FALSE;  // no problem
  const CharType *ct = (const CharType *)na;
  if (ct->isCharSetAndCollationComboOK()) return FALSE;		  // no problem
  // 3179 Collation $0 is not defined on character set $1.
  *SqlParser_Diags << DgSqlCode(-3179)
    << DgString0(CharInfo::getCollationName(ct->getCollation()))
    << DgString1(CharInfo::getCharSetName(ct->getCharSet()));
  return TRUE;
}

void emitError3435( int tok_type, int cs, ParAuxCharLenSpec::ECharLenUnit parCLU )
{
  char *tok_type_name = (char *)"CHAR";
  if ( tok_type == TOK_BYTE )
       tok_type_name  = (char *)"BYTE";
  if ( tok_type == TOK_VARCHAR )
       tok_type_name  = (char *)"VARCHAR";
  if ( tok_type == TOK_LONG )
       tok_type_name  = (char *)"LONG VARCHAR";

  const char* cs_name = CharInfo::getCharSetName( (CharInfo::CharSet) cs );

  char *lenUnitName = (char *)"UNSPECIFIED";
  if ( parCLU EQU ParAuxCharLenSpec::eBYTES )
        lenUnitName = (char *)"BYTES";
  if ( parCLU EQU ParAuxCharLenSpec::eCHARACTERS )
        lenUnitName = (char *)"CHARS";

  *SqlParser_Diags << DgSqlCode(-3435) << DgString0(tok_type_name)
                                       << DgString1(cs_name)
                                       << DgString2(lenUnitName);
}
  
// LIST(ItemExpr *) AllHostVars;
// LIST(ItemExpr *) AssignmentHostVars;
THREAD_P AllHostVarsT    *AllHostVars = NULL;
THREAD_P AssignmentHostVarsT *AssignmentHostVars = NULL;
THREAD_P HVArgTypeLookup *TheProcArgTypes = NULL;
THREAD_P NABoolean        intoClause = FALSE;
THREAD_P NABoolean        InAssignmentSt = FALSE;
THREAD_P NABoolean        ThereAreAssignments;

void resetHostVars()
{
  if (AllHostVars == NULL)
    AllHostVars = new AllHostVarsT(NULL);
  AllHostVars->clear();
  TheHostVarRoles->clear();		// SqlParserGlobals.h

  // No need to "delete TheProcArgTypes;", as PARSERHEAP heap mgmt does cleanup
  TheProcArgTypes = NULL;
  InAssignmentSt =  FALSE;
  if (AssignmentHostVars == NULL)
    AssignmentHostVars = new AssignmentHostVarsT(NULL);
  AssignmentHostVars->clear();  

  intoClause = FALSE;

  ThereAreAssignments = FALSE;
}

void MarkInteriorNodesAsInCompoundStmt(RelExpr *node)
{
  node->setBlockStmt( TRUE );
  for (Int32 i=0; i < node->getArity(); i++) {
    if (node->child(i))
      MarkInteriorNodesAsInCompoundStmt(node->child(i));
  }
}

NAWString* localeMBStringToUnicode(NAString* localeString, Lng32 charset, CollHeap *heap)
{
   charBuf cbuf((unsigned char*)(localeString->data()), localeString->length());
   NAWcharBuf* wcbuf = 0;
   Int32 errorcode = 0;
   switch (charset) {
      case SQLCHARSETCODE_ISO88591:
        wcbuf = ISO88591ToUnicode(cbuf, 0, wcbuf);
        break;
      case SQLCHARSETCODE_SJIS:
	  case SQLCHARSETCODE_EUCJP:
	  case SQLCHARSETCODE_GB18030:
	  case SQLCHARSETCODE_GB2312:
	  case SQLCHARSETCODE_GBK:
	  case SQLCHARSETCODE_MB_KSC5601:
	  case SQLCHARSETCODE_BIG5:
	  case SQLCHARSETCODE_UTF8:
        wcbuf = csetToUnicode(cbuf, 0, wcbuf, charset, errorcode);
        break;
      default:
        wcbuf = ISO88591ToUnicode(cbuf, 0, wcbuf);
   }

   if ( wcbuf ) {
      NAWString* wstr = new(heap) NAWString(wcbuf->data(), wcbuf->getStrLen(), heap);
      delete wcbuf;
      return wstr;
   } else
      return new(heap) NAWString();
}

THREAD_P Int32 in3GL_ = 0;

static Lng32 findNameInList(ItemExprList &iel, const NAString &name)
{
  for (CollIndex i = 0; i < iel.entries(); i++)
    {
      if ((iel[i]->getOperatorType() == ITM_REFERENCE) &&
	  (((ColReference*)iel[i])->getColRefNameObj().getColName() 
	   == name))
	{
	  return i;
	}
    }
  return -1;
}

RelRoot *finalize(RelExpr *top, NABoolean outputVarCntValid)
{
  RelRoot *return_top;

  if ( top == NULL )
     return NULL;

  // if the top node isn't already a root node (e.g. for unions, insert,
  // update, delete), make a new root node to hold the hostvar information
  if (top->getOperatorType() == REL_ROOT)
    return_top = (RelRoot *)top;
  else
    return_top = new (PARSERHEAP()) RelRoot(top);

  return_top->setSubRoot(TRUE);

  // If called from inside a 3GL block statement, do not set the RootFlag
  if (in3GL_ <= 0) {
    return_top->setRootFlag(TRUE);
  }
  else {
    // We only take care of input/output host variables when we have exited all
	// 3GL blocks
	return return_top;
  }
  // code in BindRelExpr and NormRelExpr rely on this isTrueRoot flag

  // Rowset::bindNode relies (indirectly) on InputVarList, so update it.

  // ---------------------------------------------------------------------
  // Add all the input host vars/parameters encountered
  // to the topmost RelRoot node's input variable list.
  // Also keep a count of the output vars/params.
  //
  // NOTE:    The hostvars are of course used somewhere else in
  //          the query. This leads to the unpleasant effect that
  //          we have parse tree nodes that are shared. If we don't
  //          want this we should consider using a LIST(ItemExpr *)
  //          in the RelRoot node instead of the ItemExpr *. Currently,
  //          the Binder does the right thing for shared ItemExprs in
  //          that it uses the already assigned ValueId.
  //
  // We iterate over the enum values in TheHostVarRoles in parallel with
  // iterating over AllHostVars.  We do this so as to extract *only* the
  // input host variables and insert only those into the RelRoot.
  // ---------------------------------------------------------------------

  return_top->outputVarCnt() = 0;
  HVArgType *argInfo;

  CollIndex i = 0;
  CollIndex j = 0;
  for (i = 0; i < AllHostVars->entries(); i++)
    {
      HostVarRole role = (*TheHostVarRoles)[(size_t)j];
      ComASSERT(role != HV_IS_INDICATOR);

      if (role == HV_IS_INPUT || role == HV_IS_DYNPARAM)
        {
          return_top->addInputVarTree((*AllHostVars)[i]);
        }
      else if (role == HV_IS_OUTPUT || role == HV_IS_INPUT_OUTPUT)
        {
	 if (role == HV_IS_INPUT_OUTPUT) {
	    NAString hvName = ((HostVar *)((*AllHostVars)[i]))->getName();
	    HostVar *hv = NULL;
	    if ( inCallStmt && ( 0 != hvName.compareTo ("")))
	    {
	      const NAString &str = ((HostVar *) ((*AllHostVars)[i]))->getIndName ();
	      if ( 0 != str.compareTo (""))
	      {
	        NAString *hvn = new (PARSERHEAP())NAString(str, PARSERHEAP ());
	        hv = makeHostVar(&hvName, hvn);
	      }
	      else
		hv = makeHostVar (&hvName, NULL);
	    }
	    else
	      hv = makeHostVar(&hvName, NULL);
	    hv->setHVInputAssignment();
            return_top->addInputVarTree(hv);
	 }
         return_top->addOutputVarTree((*AllHostVars)[i]);

         return_top->outputVarCnt()++;
         argInfo = TheProcArgTypes ?
           TheProcArgTypes->get(&((HostVar *)((*AllHostVars)[i]))->getName()) :
           NULL;
         if (argInfo) argInfo->intoCount()++;
        }

      //delete (*AllHostVars)[i];  // an orphan after this, so delete it
      //(*AllHostVars)[i] = NULL;

      // Skip past at most one following indicator
      // but not beyond the end of the array please
      if (++j < TheHostVarRoles->entries() &&
          (*TheHostVarRoles)[(size_t)j] == HV_IS_INDICATOR)
        {
          j++;
          if (role == HV_IS_OUTPUT)
            {
              if (TheProcArgTypes)
                {
                  NAString *ivName = (NAString *)&
                    ((HostVar *)((*AllHostVars)[i]))->getIndName();
                  if (!ivName->isNull())
                    {
                      argInfo = TheProcArgTypes->get(ivName);
                      if (!argInfo)
                        argInfo = new (PARSERHEAP()) HVArgType
                          (ivName, new (PARSERHEAP()) SQLUnknown(PARSERHEAP()));
                      argInfo->useCount()++;
                      argInfo->intoCount()++;
                    }
                }
            } // INTO :hv :iv  -- lookup :iv, add if necessary, incr counts
        }
    } // for-AllHostVars-loop
  assert(j >= TheHostVarRoles->entries());    // j should reach end when i does

  // The RelRoot::outputVarCnt_ returns as
  // a nonnegative count of output host variables (e.g. SELECT cols INTO :hv...)
  // or of output dynamic parameters (no syntax for this currently (ever?)),
  // or -1 for roots that don't get called here (non-topmost or non-true roots)
  // or are called as part of a DECLARE CURSOR (as in the line following).
  // Binder checks this count.
  //
  if (!outputVarCntValid) return_top->outputVarCnt() = -1;

  if (TheProcArgTypes)
    {
      LIST(HVArgType *) argdump(PARSERHEAP());
      TheProcArgTypes->dump(argdump);
      for (i = 0; i < argdump.entries(); i++)
        {

          // Warnings: if procedure parameter is unused,
          //   or if used more than once as an INTO target.
          if (!argdump[i]->useCount())
            *SqlParser_Diags << DgSqlCode(+3162)
                             << DgString0(*argdump[i]->getName());

          // We can have an output variable appear more than once
          // if we have assignment statements
          if (!ThereAreAssignments) {
            if (argdump[i]->intoCount() > 1)
              *SqlParser_Diags << DgSqlCode(+3163)
                               << DgString0(*argdump[i]->getName());
          }

#ifndef NDEBUG
          // Ansi 4.2.3 says that by default, hostvars are COERCIBLE,
          // unless they have a COLLATE clause in which case they're EXPLICIT.
          // But only column refs can be IMPLICIT.
          //
          const CharType *ct = (const CharType *)argdump[i]->getType();
          if (ct->getTypeQualifier() == NA_CHARACTER_TYPE)
            { ComASSERT(ct->getCoercibility() != CharInfo::IMPLICIT); }
#endif
        }
    }

  return return_top;
}


void ForUpdateSpec::finalizeUpdatability(RelExpr *top)
{
  ComASSERT(top->getOperatorType() == REL_ROOT);
  RelRoot *treeTop = (RelRoot *)top;

  // Ansi 13.1 SR 5a -- no updatability clause specified, but ORDER BY was.
  // See also RelRoot::isUpdatableCursor() in NormRelExpr.cpp.
  // Genesis 10-990215-7815.
  //
  if (!explicitSpec_) {
    if (treeTop->hasOrderBy())
      forUpdate_ = FALSE;
    else {
      // Tandem extension, defaulting rule --
      // if user has set this default attr to TRUE,
      // they get FOR READ ONLY (itself a Tdm-ext syntax),
      // resulting in better runtime performance.
      forUpdate_ = (CmpCommon::getDefault(READONLY_CURSOR) == DF_FALSE);
    }
  }
  treeTop->updatableSelect() = forUpdate_;
  if (updateCol_) {
    ComASSERT(explicitSpec_ && forUpdate_);
    treeTop->addUpdateColTree(updateCol_);
  }
}


NABoolean finalizeAccessOptions(RelExpr *top,
                                TransMode::AccessType at,
                                LockMode   lm)
{
  if (top->getOperatorType() == REL_TUPLE ||
      top->getOperatorType() == REL_TUPLE_LIST) {
    return (at == TransMode::ACCESS_TYPE_NOT_SPECIFIED_ && lm == LockMode::LOCK_MODE_NOT_SPECIFIED_);
  }

  // In case of an INSERT VALUES statement, this is a Tuple node.
  if (top->getOperatorType() != REL_ROOT)
    return TRUE;


  RelRoot *treeTop = (RelRoot *)top;

  if (at != TransMode::ACCESS_TYPE_NOT_SPECIFIED_) {
    if (treeTop->accessOptions().accessType() != TransMode::ACCESS_TYPE_NOT_SPECIFIED_) {
      *SqlParser_Diags << DgSqlCode(-3196);
      // Access type cannot be specified more than once.
      return FALSE;	// error
    }
    treeTop->accessOptions().accessType() = at;
  }

  if (lm != LockMode::LOCK_MODE_NOT_SPECIFIED_) {
    if (treeTop->accessOptions().lockMode() != LockMode::LOCK_MODE_NOT_SPECIFIED_) {
      *SqlParser_Diags << DgSqlCode(-3197);
      // Lock mode cannot be specified more than once.
      return FALSE;	// error
    }
    treeTop->accessOptions().lockMode() = lm;
  }

  return TRUE;		// no error
}

NAString * getSqlStmtStr(CharInfo::CharSet & refparam_targetCharSet, CollHeap * heap)
{
  NAWchar *inputStr = SQLTEXTW();
  Int32 start_pos = 0;
  if (NAWstrlen(inputStr) >= 7)
    {
      NAWchar temp[10];
      Int32 i = 0;
      while (i < 7)
	{
	  temp[i] = na_towupper(inputStr[i]);
	  i++;
	}
      if (NAWstrncmp(temp, WIDE_("DISPLAY"), 7) == 0) 
	start_pos = 7;
    }
  
  // SqlParser_CurrentParser->charset_ is the encoding charset of the
  // sql stmt under parsing now. Target is the charset that the stmt
  // will be encoded before it is sent to catman.
  // Assume the target charset is the source charset for now.
  refparam_targetCharSet = SqlParser_CurrentParser->charset_;
  
  refparam_targetCharSet = CharInfo::UTF8;
  NAString *stmt = unicodeToChar ( &inputStr[start_pos] // in - const NAWchar *
                                 , SQLTEXTNAWCHARBUF()->getStrLen() - start_pos
                                 , CharInfo::UTF8
                                 , heap
                                 );
  ParScannedInputCharset = CharInfo::UTF8;

  return stmt;
}

// The purpose of this function is to return a pointer to a HostVar object.
// This new HostVar should have a name from the given name in the arg,
// and its type should either be SQLUnknown (if no type info is available),
// or it should be obtained from TheProcArgTypes.  This depends on whether
// that list was init'd which depends on the syntax that came before the
// occurrence of the host variable for which we are making this data.
//
// If hvName is NULL, that causes an assertion failure.
// If indName is NULL, that means this host variable has no indicator
// and is not a programming error.
//
// To set the NAType of the HostVar which we return, we must
// set its own nullability flag according to whether or not there
// is an indicator in this host variable.  Is this redundant since
// that info is already in the HostVar itself?

HostVar *makeHostVar(NAString *hvName, NAString *indName, NABoolean isDynamic)
{
  ComASSERT(hvName && !hvName->isNull() && ((isDynamic) || (*hvName)[(StringPos)0] == ':'));
  ComASSERT(!indName || (!indName->isNull() && (*indName)[(StringPos)0] == ':'));

  HVArgType *argInfo = TheProcArgTypes ?
		       TheProcArgTypes->get(hvName) : NULL;
  NAType    *naType  = argInfo ?
		       argInfo->getType() : new (PARSERHEAP()) SQLUnknown(PARSERHEAP());
  ComASSERT(naType);
  naType->setNullable(!!indName);

  HostVar   *result  = indName ?
    new (PARSERHEAP()) HostVar(*hvName, *indName, naType) :
    new (PARSERHEAP()) HostVar(*hvName, naType);
  ComASSERT(result);

  if (
  #ifndef NDEBUG
      !getenv("HV_DEBUG") &&
  #endif
      (CmpCommon::context()->GetMode() == STMT_DYNAMIC) &&
      (NOT isDynamic))
    {
      // 3049 Host variables ($string0) are not allowed in dynamic compilation.
      *SqlParser_Diags << DgSqlCode(-3049) << DgString0(*hvName);
    }
  else if (!argInfo)
    {
      if (TheProcArgTypes)
        {
          // ANSI 12.3 SR 3
          // 3161 $string0 was not declared in the procedure parameter list.
          *SqlParser_Diags << DgSqlCode(-3161) << DgString0(*hvName);

          // So we won't display this errmsg again for this hv in this stmt:
          argInfo = new (PARSERHEAP()) HVArgType(hvName, naType);
          TheProcArgTypes->insert(argInfo);
          // And so we won't display warning 3162 in finalize() either:
          argInfo->useCount()++;
        }
      else
        {
          // Not in a PROCEDURE, it's okay, let the Generator coerce the
          // SQLUnknown type to some default flavor of signed int
          // (the way it does for INDICATOR variables, which you notice
          // we do *not* currently pass in the proc arg list).
        }
    }
  else
    {
      argInfo->useCount()++;
    }

  setHVorDPvarIndex ( result, hvName );
  
  return result;

  // Do **NOT** do this (remove info from our hashtable)!
  //	if (argInfo)
  //	  {
  //	    argInfo->type_ = NULL;
  //	    delete argInfo;
  //	  }
}

static
NABoolean literalToNumber(NAString *strptr, char sign, NAString *cvtstr, 
                          short &shortVal, Lng32 &longVal, Int64& i64Val, 
                          char **bigNum, Lng32& bigNumSize, size_t &strSize)
{
  NABoolean returnValue = TRUE; // assume success until proven otherwise
  //
  // Get the size of the absolute value first,
  // *then* it's safe to modify the text --
  // both the one we will store in strptr
  // and the descaled one to be converted by atoxxx (cvtstr).
  //
  strSize = cvtstr->length();
  if (sign == '-') {
    strptr->prepend(sign);
    if (strptr != cvtstr)
      cvtstr->prepend(sign);
  }

  if ((! (Get_SqlParser_Flags(ALLOW_ARB_PRECISION_LITERALS))) &&
      (CmpCommon::getDefaultNumeric(MAX_NUMERIC_PRECISION_ALLOWED)
       > MAX_HARDWARE_SUPPORTED_SIGNED_NUMERIC_PRECISION) &&
      (strSize > (ULng32)CmpCommon::getDefaultNumeric(MAX_NUMERIC_PRECISION_ALLOWED)))
    {
      *SqlParser_Diags << DgSqlCode(-3014) << DgInt0(strSize)
		       << DgInt1((Lng32)CmpCommon::getDefaultNumeric(MAX_NUMERIC_PRECISION_ALLOWED)); 
      returnValue = FALSE; // 
      return returnValue;
    }

  if (strSize < 5) {
    shortVal = atoi(*cvtstr);
  } else if (strSize < 10) {
    longVal = atol(*cvtstr);
  } else if (strSize < 19) {
    i64Val = atoInt64(*cvtstr);
  } else { // precision >= 19
    if ((CmpCommon::getDefaultNumeric(MAX_NUMERIC_PRECISION_ALLOWED) >
	 MAX_HARDWARE_SUPPORTED_SIGNED_NUMERIC_PRECISION) ||
	(Get_SqlParser_Flags(ALLOW_ARB_PRECISION_LITERALS)) 
	// callers know what they're doing, leave them alone.
	|| strSize < FLT_MAX_10_EXP) { 
      // LLONG_MAX & LLONG_MIN have 19 digits and must be allowed here.
      // There are other places in mxcmp like GenExpGenerator.cpp's
      // ExpGenerator::scaleBy10x() that do 
      //   rettree = createExprTree(str, 0, 1, retTree);
      // where str is "1000000000000000000000", a 22-digit "literal" that
      // must be allowed here also.
      // Also, just in case you ever want to use SQLNumeric here, be 
      // aware that atoInt64(LLONG_MAX) even with overflow checking can 
      // kill mxcmp with a signal 31!
      // Prepare BCD representation of number
      Lng32 largestrSize = strSize + 1; // extra byte for sign
      char *largestr = new (PARSERHEAP()) char[largestrSize];
      largestr[0] = sign;
      size_t j = (sign == '+') ? 0 : 1;
      for (size_t i = 0; i < strSize; i++)	  // strSize, not largestrSize
        largestr[i+1] = (*cvtstr)[i+j] - '0';

      // Convert BCD to Big Num representation
      bigNumSize = BigNumHelper::ConvPrecisionToStorageLengthHelper(strSize);
      *bigNum = new (PARSERHEAP()) char[bigNumSize];
      BigNumHelper::ConvBcdToBigNumWithSignHelper(largestrSize,
                                                  bigNumSize, 
                                                  largestr,
                                                  *bigNum);
      NADELETEBASIC(largestr, (PARSERHEAP()));	
    } else { // precision >= FLT_MAX_10_EXP in a user-specified literal
      // genesis 10-030220-1214 documents a cpu halt can happen here when we
      // try to support digit precisions >= 77. The R2 version of this code
      // has Gautam Das' BigNum code. It does not seem to cause a cpu halt
      // but can quietly experience an overflow and give a wrong answer!
      *SqlParser_Diags << DgSqlCode(-3165) << DgString0(*strptr);
      returnValue = FALSE; // 
    }
  }
  return returnValue;
}


// In the next several procedures, we set up ConstValue's, in whose text
// we want negative numbers to end up with minus signs but positive numbers
// without plus signs.  That is,
//	"1",'+'	-> "1"
//	"2",'-'	-> "-2"
// (Only digits, no signs, are passed in the strptr and cvtstr arguments.)
// (Note that we delete the strptr before returning:  thus we may freely
// modify the text therein.)

ItemExpr *literalOfNumericPassingScale(NAString *strptr, char sign,
                                       NAString *cvtstr, size_t scale)
{
  short rc = 0;

  ItemExpr *returnValue = NULL;
  //
  // Get the size of the absolute value first,
  // *then* it's safe to modify the text --
  // both the one we will store in ConstValue's text (strptr)
  // and the descaled one to be converted by atoxxx (cvtstr).
  //
  size_t strSize = cvtstr->length();
  NABoolean createSignedDatatype = 
    ((CmpCommon::getDefault(TRAF_CREATE_SIGNED_NUMERIC_LITERAL)) == DF_ON);
  if (sign == '-') {
    createSignedDatatype = TRUE;
    strptr->prepend(sign);
    if (strptr != cvtstr)
      cvtstr->prepend(sign);
  }

  if ((! (Get_SqlParser_Flags(ALLOW_ARB_PRECISION_LITERALS))) &&
      (CmpCommon::getDefaultNumeric(MAX_NUMERIC_PRECISION_ALLOWED)
       > MAX_HARDWARE_SUPPORTED_SIGNED_NUMERIC_PRECISION) &&
      (strSize > (ULng32)CmpCommon::getDefaultNumeric(MAX_NUMERIC_PRECISION_ALLOWED)))
    {
      *SqlParser_Diags << DgSqlCode(-3014) << DgInt0(strSize)
		       << DgInt1((Lng32)CmpCommon::getDefaultNumeric(MAX_NUMERIC_PRECISION_ALLOWED)); 
      return NULL;
    }

  NABoolean createTinyLiteral = 
    ((CmpCommon::getDefault(TRAF_CREATE_TINYINT_LITERAL)) == DF_ON);
  NABoolean createLargeintUnsignedLiteral =
    ((CmpCommon::getDefault(TRAF_LARGEINT_UNSIGNED_IO)) == DF_ON);
  
  char numericVal[8];
  short datatype = -1;
  Lng32 length = -1;
  if ((createTinyLiteral) && (strSize < 3)) {
    datatype = (createSignedDatatype ? REC_BIN8_SIGNED : REC_BIN8_UNSIGNED);
    length = sizeof(Int8);
  } else if (strSize < 5) {
    datatype = (createSignedDatatype ? REC_BIN16_SIGNED : REC_BIN16_UNSIGNED);
    length = sizeof(short);
  } else if (strSize < 10) {
    datatype = (createSignedDatatype ? REC_BIN32_SIGNED : REC_BIN32_UNSIGNED);
    length = sizeof(Lng32);
  } else if (strSize <= 19) {
    datatype = (createSignedDatatype || !createLargeintUnsignedLiteral ? REC_BIN64_SIGNED : REC_BIN64_UNSIGNED);
    length = sizeof(Int64);
  } else if ((strSize == 20) && (!createSignedDatatype) && (createLargeintUnsignedLiteral)) {
    datatype = REC_BIN64_UNSIGNED;
    length = sizeof(Int64);
  }    
    
  if (datatype != -1) {
    rc = convDoIt((char*)cvtstr->data(),
		  (Lng32)cvtstr->length(),
		  REC_BYTE_F_ASCII,
		  0, 0,
		  numericVal,
		  length,
		  datatype,
		  0, 0,
		  NULL, 0, 
		  PARSERHEAP(), 
		  NULL,
		  CONV_UNKNOWN);
    if (rc == 0) {
      if ((CmpCommon::getDefault(LIMIT_MAX_NUMERIC_PRECISION) == DF_ON) &&
	  (strSize == 19))
	returnValue = 
	  new (PARSERHEAP()) ConstValue
	  (new (PARSERHEAP()) SQLLargeInt
	    (PARSERHEAP(), (Lng32)scale, 
             (UInt16) 0, // 64-bit
	     TRUE,
	     FALSE),
	   (void *)numericVal,
	   (UInt32) length,
	   strptr);
      else
	returnValue = 
	  new (PARSERHEAP()) ConstValue
	  (new (PARSERHEAP()) SQLNumeric
	   (PARSERHEAP(), length,
	    (Lng32)strSize, // precision
	    (Lng32)scale, 
	    createSignedDatatype,
	    FALSE),
	   (void *)numericVal,
	   (UInt32) length,
	   strptr);
    }
    else if (strSize < 19) {
      *SqlParser_Diags << DgSqlCode(-8411);
      return NULL;
    }
    else
      datatype = -1;
  } // datatype != -1

  if (datatype == -1) {
    if ((CmpCommon::getDefaultNumeric(MAX_NUMERIC_PRECISION_ALLOWED) >
	 MAX_HARDWARE_SUPPORTED_SIGNED_NUMERIC_PRECISION) ||
	(Get_SqlParser_Flags(ALLOW_ARB_PRECISION_LITERALS)) 
        // callers know what they're doing, leave them alone.
        || strSize < FLT_MAX_10_EXP) { 
      // LLONG_MAX & LLONG_MIN have 19 digits and must be allowed here.
      // There are other places in mxcmp like GenExpGenerator.cpp's
      // ExpGenerator::scaleBy10x() that do 
      //   rettree = createExprTree(str, 0, 1, retTree);
      // where str is "1000000000000000000000", a 22-digit "literal" that
      // must be allowed here also.
      // Also, just in case you ever want to use SQLNumeric here, be 
      // aware that atoInt64(LLONG_MAX) even with overflow checking can 
      // kill mxcmp with a signal 31!
      // Prepare BCD representation of number
      Lng32 largestrSize = strSize + 1; // extra byte for sign
      char *largestr = new (PARSERHEAP()) char[largestrSize];
      largestr[0] = sign;
      size_t j = (sign == '+') ? 0 : 1;
      for (size_t i = 0; i < strSize; i++)	  // strSize, not largestrSize
        largestr[i+1] = (*cvtstr)[i+j] - '0';

      // Convert BCD to Big Num representation
      Lng32 bigNumSize = BigNumHelper::ConvPrecisionToStorageLengthHelper(strSize);
      char *bigNumData = new (PARSERHEAP()) char[bigNumSize];
      BigNumHelper::ConvBcdToBigNumWithSignHelper(largestrSize,
                                                  bigNumSize, 
                                                  largestr,
                                                  bigNumData);
    
      returnValue = new (PARSERHEAP()) ConstValue
        (new (PARSERHEAP()) SQLBigNum(PARSERHEAP(), strSize, 
                                      scale,
                                      TRUE,
                                      TRUE,
                                      FALSE),
         (void *) bigNumData,
         bigNumSize,
         strptr);
      NADELETEBASIC(largestr, (PARSERHEAP()));	
      NADELETEBASIC(bigNumData, (PARSERHEAP()));
    } else { // precision >= FLT_MAX_10_EXP in a user-specified literal
      // genesis 10-030220-1214 documents a cpu halt can happen here when we
      // try to support digit precisions >= 77. The R2 version of this code
      // has Gautam Das' BigNum code. It does not seem to cause a cpu halt
      // but can quietly experience an overflow and give a wrong answer!
      *SqlParser_Diags << DgSqlCode(-3165) << DgString0(*strptr);
    }
  }
  //
  // Delete the original string (strptr),
  // but leave cvtstr alone (either a temporary, or identical to strptr)!
  delete strptr;
  return returnValue;
}


static Int32
removeDecimalPointReturnScale(NAString *strptr, NAString& tmpstr)
{
  // remove the decimal point, if any, from tmpstr
  size_t i, j, dot=0, strSize = strptr->length();
  for (i=j=dot; i < strSize; i++)
    if ((*strptr)[i] != '.')
      tmpstr[j++] = (*strptr)[i];
    else
      dot = j;			// Remember the position of the decimal point
  tmpstr.resize(j);             // adjust the size
  // return number's scale
  return Int32(strSize - dot -1); 
}


NABoolean literalToNumeric(NAString *strptr, double &val, char sign)
{
  // Create a new string after removing the decimal point, if any.
  NAString tmpstr(*strptr, strptr->length());
  size_t strSize;
  Int32 scale = removeDecimalPointReturnScale(strptr, tmpstr);

  // convert number into a double
  short shortVal; Lng32 longVal = 0; Int64 i64Val;
  char *bigNum = NULL; Lng32 bigNumSize;
  NABoolean result = literalToNumber
    (strptr, sign, &tmpstr, shortVal, longVal, i64Val, &bigNum,
     bigNumSize, strSize);
  if (result) {
    if (strSize < 5)
      val = shortVal * pow(10, -scale);
    else if (strSize < 10) 
      val = longVal * pow(10, -scale);
    else if (strSize < 19) 
      val = convertInt64ToDouble(i64Val) * pow(10, -scale);
    else {
      ComASSERT(BigNumHelper::ConvBigNumWithSignToInt64Helper
                (bigNumSize, bigNum, (void*)&i64Val,FALSE) == 0);
      val = convertInt64ToDouble(i64Val) * pow(10, -scale);
      NADELETEBASIC(bigNum, (PARSERHEAP()));
    }
  }
  delete strptr;
  return result;
}


ItemExpr *literalOfNumericWithScale(NAString *strptr, char sign)
{
  //
  // Create a new string after removing the decimal point, if any.
  //
  size_t strSize = strptr->length();
  NAString tmpstr(*strptr, strSize);
  size_t i, j, dot;
  for (i=j=dot=0; i < strSize; i++)
    if ((*strptr)[i] != '.')
      tmpstr[j++] = (*strptr)[i];
    else
      dot = j;			// Remember the position of the decimal point
  tmpstr.resize(j);             // adjust the size
  return literalOfNumericPassingScale(strptr, sign, &tmpstr, strSize - dot - 1);
}

NABoolean literalToDouble(NAString *strptr, double& val, 
                          NABoolean& floatP, char sign)
{
  NABoolean returnValue = FALSE; // assume failure until proven otherwise
  //
  // Compute the precision of the mantissa.  In other words, count the number
  // of significant digits in the mantissa.  Leading zeroes are not
  // significant.
  //
  UInt32 mantissaPrecision = 0;
  const char* s = *strptr;
  while ((*s != '\0') && (*s == '0'))
    s++;
  while ((*s != '\0') && (*s != 'e') && (*s != 'E')) {
    if (isdigit(*s++))
      mantissaPrecision++;
  }

  // Now get the size of the exponent
  Lng32 expValue = 0;
  if ((*s == 'e') || (*s == 'E')) {
    s++;
    expValue = atol(s);
  }

  // Having done that scanning, *now* it is safe to modify the text.
  //
  if (sign == '-') strptr->prepend(sign);

  floatP = FALSE;

  // check mantissa digits and exponent values to decide which type to build
  // and ensure some basic values are correct.
  // Overflow's and undflow's are caught at runtime in convDoIt()
  // under (case CONV_FLOAT64_FLOAT32:)
  if (mantissaPrecision < 8 &&             // real ?
      expValue < FLT_MAX_10_EXP &&
      expValue > -FLT_MAX_10_EXP)
  {
    floatP = TRUE;
  }
  else if (mantissaPrecision >= 20 ||
           expValue > DBL_MAX_10_EXP ||
           expValue < -DBL_MAX_10_EXP)
  {
    *SqlParser_Diags << DgSqlCode(-3165) << DgString0(*strptr);
    return returnValue;
  }

  // now try converting the string and see if there were problems
  char *endPtr;
  errno=0;
  val = strtod(*strptr, &endPtr);

  Int32 numErr=0;

  numErr = errno;

  // There are some anomalies with floating point operations on
  // NSK.  For instance strtod can produce double values that are
  // out-of-range with respect to the defines (DBL_MAX and
  // DBL_MIN).  Make range checking consistent with the executor
  // (see exp_conv.cpp convDoit, CONV_FLOAT64_FLOAT64).
  //
  if((val < -DBL_MAX) ||
     (val > DBL_MAX) ||
     (val != 0 && val < DBL_MIN && val > -DBL_MIN)) {
    numErr = ERANGE;
    val = 0;
  }
          

  // check for underflow and overflow
  if ( ((val == 0) &&
        ((numErr == ERANGE) ||                // underflo
         (strptr->data() == endPtr))) ||      // not a number
       (val == -HUGE_VAL) ||           // overflow
       (val ==  HUGE_VAL) )            // overflow
    *SqlParser_Diags << DgSqlCode(-3166) << DgString0(*strptr);
  else
    returnValue = TRUE; // success

  return returnValue;
}

ItemExpr *literalOfApproxNumeric(NAString *strptr, char sign)
{
  short rc = 0;
  ItemExpr *returnValue = NULL;

  if (sign == '-') 
    strptr->prepend(sign);

  double doubleVal;
  rc = convDoIt((char*)strptr->data(),
		(Lng32)strptr->length(),
		REC_BYTE_F_ASCII,
		0, 0,
		(char*)&doubleVal,
		sizeof(double),
		REC_FLOAT64,
		0, 0,
		NULL, 0, 
		PARSERHEAP(), 
		NULL,
		CONV_UNKNOWN);
  if (rc != 0)
    {
      *SqlParser_Diags << DgSqlCode(-3166) << DgString0(*strptr);;     
      return NULL;
    }

  returnValue = new (PARSERHEAP()) ConstValue (new (PARSERHEAP())
					       SQLDoublePrecision(PARSERHEAP(), FALSE),
					       (void *)&doubleVal,
					       sizeof(double),
					       strptr);
  if (returnValue == NULL)
    *SqlParser_Diags << DgSqlCode(-2006);     // out of memory error
  
  return returnValue;
}

ItemExpr *literalOfInterval(NAString *strptr,
                            IntervalQualifier *qualifier,
                            char sign)
{
  ItemExpr *returnValue = NULL;
  IntervalValue intervalValue(*strptr,
                              qualifier->getStartField(),
                              qualifier->getLeadingPrecision(),
                              qualifier->getEndField(),
                              qualifier->getFractionPrecision(),
                              sign);
  IntervalType *intervalType = new (PARSERHEAP())
		  SQLInterval(PARSERHEAP(), FALSE,
			      qualifier->getStartField(),
			      qualifier->getLeadingPrecision(),
			      qualifier->getEndField(),
			      qualifier->getFractionPrecision());
  strptr->prepend("'");
  strptr->append ("' ");
  strptr->append (intervalType->getIntervalQualifierAsString());
  if (! intervalValue.isValid())
    *SqlParser_Diags << DgSqlCode(-3044) << DgString0(*strptr);
  else 
    {
     if (!intervalType->isSupportedType())  // issue a warning
     {
      *SqlParser_Diags << DgSqlCode(3044) << DgString0(*strptr);
     }
     if (sign == '-') 
     {
      strptr->prepend("-");
     }
     strptr->prepend("INTERVAL ");
     returnValue = new (PARSERHEAP()) ConstValue(
                        intervalType,
                        (void *) intervalValue.getValue(),
                       intervalValue.getValueLen(),
                        strptr);
    }
  delete strptr;
  //assert (returnValue);	//don't assert; caller checks value for NULL
  return returnValue;
}


ItemExpr *literalOfDate(NAString *strptr, NABoolean noDealloc)
{
  ItemExpr *returnValue = NULL;
  UInt32 fractionPrec;
  DatetimeValue dtValue(*strptr, REC_DATE_YEAR, REC_DATE_DAY, fractionPrec,
                        (CmpCommon::getDefault(USE_OLD_DT_CONSTRUCTOR) == DF_ON));
  if ((! dtValue.isValid()) &&
      (CmpCommon::getDefault(MARIAQUEST_PROCESS) == DF_OFF))
     *SqlParser_Diags << DgSqlCode(-3045) << DgString0(*strptr);
  else
    returnValue = new (PARSERHEAP()) ConstValue (new (PARSERHEAP())
		    SQLDate(PARSERHEAP(), FALSE),
		    (void *) dtValue.getValue(),
		    dtValue.getValueLen(),
		    strptr);
  if (NOT noDealloc)
    delete strptr;
  //assert (returnValue);	//don't assert; caller checks value for NULL
  return returnValue;
}


ItemExpr *literalOfTime(NAString *strptr)
{
  ItemExpr *returnValue = NULL;
  UInt32 fractionPrec = 0;
  DatetimeValue dtValue(*strptr, REC_DATE_HOUR, REC_DATE_SECOND, fractionPrec,
                        (CmpCommon::getDefault(USE_OLD_DT_CONSTRUCTOR) == DF_ON));
  if ((! dtValue.isValid()) &&
      (CmpCommon::getDefault(MARIAQUEST_PROCESS) == DF_OFF))
      *SqlParser_Diags << DgSqlCode(-3046) << DgString0(*strptr);
  else
    returnValue = new (PARSERHEAP()) ConstValue (new (PARSERHEAP())
		    SQLTime(PARSERHEAP(), FALSE, fractionPrec),
		    (void *) dtValue.getValue(),
		    dtValue.getValueLen(),
		    strptr);
  delete strptr;
  //assert (returnValue);	//don't assert; caller checks value for NULL
  return returnValue;
}
//
// This routine handles MP-style datetime literals; some of these literals map to ANSI
// DATE, TIME or TIMESTAMP, in which case the corresponding NAType is generated.
//
ItemExpr *literalOfDateTime(NAString *strptr, DatetimeQualifier *qualifier)
{
  ItemExpr *returnValue = NULL;
  UInt32 fractionPrec = qualifier->getFractionPrecision();
  DatetimeValue    dtValue(*strptr,
                           qualifier->getStartField(),
                           qualifier->getEndField(),
                           fractionPrec,                   // returned value
                           (CmpCommon::getDefault(USE_OLD_DT_CONSTRUCTOR) == DF_ON));

  DatetimeType  *dtType = DatetimeType::constructSubtype(       // This call is necessary to insure that we return
                               FALSE,                           // a standard DateTime, if possible.
                               qualifier->getStartField(),
                               qualifier->getEndField(),
                               fractionPrec,
                               PARSERHEAP()
                               );
  if (!dtType)
  {
   *SqlParser_Diags << DgSqlCode(-3158) << DgString0(""); // Error - invalid datetime
  }
  else
  {
   strptr->prepend("'");
   strptr->append ("'");
   strptr->append (dtType->getDatetimeQualifierAsString(TRUE));
   if (dtType->checkValid(SqlParser_Diags))
   {
    if (!dtValue.isValid())
    {
    *SqlParser_Diags << DgSqlCode(-3158) << DgString0(*strptr); // Error - invalid datetime
    }
    else
    {
    if (!dtType->isSupportedType())
    { 
    *SqlParser_Diags << DgSqlCode(3158) << DgString0(*strptr); // Warning - invalid datetime
    }
    strptr->prepend("DATETIME");
    returnValue = new (PARSERHEAP()) ConstValue(
                    dtType,
                    (void *) dtValue.getValue(),
                    dtValue.getValueLen(),
                    strptr);
    }
   }
  }
  delete strptr;
  //assert (returnValue);       //don't assert; caller checks value for NULL
  return returnValue;
  }




ItemExpr *literalOfTimestamp(NAString *strptr)
{
  ItemExpr *returnValue = NULL;
  UInt32 fractionPrec = 0;
  DatetimeValue dtValue(*strptr, REC_DATE_YEAR, REC_DATE_SECOND, fractionPrec,
                        (CmpCommon::getDefault(USE_OLD_DT_CONSTRUCTOR) == DF_ON));
  if ((! dtValue.isValid()) &&
      (CmpCommon::getDefault(MARIAQUEST_PROCESS) == DF_OFF))
     *SqlParser_Diags << DgSqlCode(-3047) << DgString0(*strptr);
  else
    returnValue = new (PARSERHEAP())
      ConstValue (new (PARSERHEAP())
                  SQLTimestamp (PARSERHEAP(), FALSE, fractionPrec),
                  (void *) dtValue.getValue(),
                  dtValue.getValueLen(),
                  strptr);
  delete strptr;
  //assert (returnValue);	//don't assert; caller checks value for NULL
  return returnValue;
}


void parseCasedIdentifier(ParCaseIdentifierClauseType clauseType,
                          NAString *pClauseBuffer,
                          NAString &casedIdentifier)
{
  assert(pClauseBuffer);
  NAString workBuf(*pClauseBuffer);
  StringPos invalidCharPos;

  switch (clauseType)
  {
   case ParCALL_CASED_IDENTIFIER_CLAUSE :
     {
     //  remove leading reserved word CALL from workBuf
     StringPos firstBlankPos = 0, startPos = 0;
     firstBlankPos = IndexOfFirstWhiteSpace(workBuf, startPos);
     ComASSERT(firstBlankPos != NA_NPOS);
     NAString firstTok(workBuf(startPos, firstBlankPos-startPos/*length*/));
     casedIdentifier = workBuf;
     TrimNAStringSpace(casedIdentifier,TRUE,FALSE);

     break;
     }
   case ParGOTO_CASED_IDENTIFIER_CLAUSE :
     {
     //  remove leading reserved word GOTO or GO TO from workBuf
     StringPos firstBlankPos, startPos = 0;
     firstBlankPos = IndexOfFirstWhiteSpace(workBuf, startPos);
     ComASSERT(firstBlankPos != NA_NPOS);
     NAString firstTok(workBuf(startPos, firstBlankPos-startPos/*length*/));
     startPos = IndexOfFirstNonWhiteSpace(workBuf, firstBlankPos);  // skips blank(s)
     if ( firstTok == "GO" ) // either "GO TO" or "GOTO" 
       { // skips GO
       StringPos secondBlankPos = IndexOfFirstWhiteSpace(workBuf, startPos);
       assert(secondBlankPos != NA_NPOS);
	   // skips TO
       startPos = IndexOfFirstNonWhiteSpace(workBuf, secondBlankPos);  // skips blank(s)
       }

     // gets cased identifier
     casedIdentifier = workBuf(startPos,workBuf.length()-startPos/*length*/);

     break;
     }
   case ParPERFORM_CASED_IDENTIFIER_CLAUSE :
     {
     //  remove leading reserved word PERFORM from workBuf
     //  please note that PERFORM is not a reserved word.
     StringPos firstBlankPos, startPos = 0;
     firstBlankPos = IndexOfFirstWhiteSpace(workBuf, startPos);
     ComASSERT(firstBlankPos != NA_NPOS);
     NAString firstTok(workBuf(startPos, firstBlankPos-startPos/*length*/));
     startPos = IndexOfFirstNonWhiteSpace(workBuf, firstBlankPos);  // skips blank(s)

     // gets cased identifier
     casedIdentifier = workBuf(startPos,workBuf.length()-startPos/*length*/);

     if (IdentifyMyself::GetMyName() != I_AM_COBOL_PREPROCESSOR) 
       {
	 *SqlParser_Diags << DgSqlCode(-3133); // PERFORM is valid only in COBOL
	 return;
       }

     break;
     }

   default :
     {
	 *SqlParser_Diags << DgSqlCode(-4000); // internal error
	 return;
	 }
   } // end of switch (clauseType)

  // If the running process is the C preprocessor, checks to make sure that 
  // workBuf contains a valid C identifier; otherwise, issues an error message. 
  // Please note that workBuf may contain a delimited identifier.

  // If the running process is the COBOL preprocessor, checks to make sure that
  // workBuf contains a valid COBOL work; otherwise, issues an error message.

  // If workBuf contains a regular identifier, makes sure that it is not a 
  // reserverd word.

  switch (IdentifyMyself::GetMyName())  
  {
    case I_AM_C_PREPROCESSOR:
	  {
          if (!IsCIdentifier(casedIdentifier.data()))
            {
	      //Illegal character in identifier $0~string0.
              *SqlParser_Diags << DgSqlCode(-3127)
		               << DgString0("C identifier " + casedIdentifier);
	      return;
            } 
	  break;
	  }
    case I_AM_COBOL_PREPROCESSOR:
	  {
          *SqlParser_Diags << DgSqlCode(-4222)
                           << DgString0("COBOL embedded SQL");
          return;
	  }
    default:
          {
          // error, should not have come here
	  *SqlParser_Diags << DgSqlCode(-4000); // internal error
	  break;
          }
  } // end of switch

} // end of parseCasedIdentifier


// deletes all members of seq[]  (recall in C++ delete NULL does nothing)
ShortStringSequence::~ShortStringSequence()
{
   for (UInt32 i=0; i!=MAX_NUM_PARTS; i++)
     delete seq[i];
} // destructor

// Sets up numParts_ to 0.  seq[] is all NULLs.
// Cannot determine seqPos_ and seqEndPos_.  Sets them to 0.
ShortStringSequence::ShortStringSequence()
  : numParts_(0),
    toInternalIdentifierFlags_(NASTRING_ALLOW_NSK_GUARDIAN_NAME_FORMAT),
    seqPos_(0),
    seqPosStartOffset_(0),
    seqEndPos_(0)
{
  for (UInt32 i=0; i!=MAX_NUM_PARTS; i++)
    seq[i] = NULL;
} // default constructor

// numParts_ gets 1.  seq[] is { arg, NULL, NULL, ... }.
ShortStringSequence::ShortStringSequence(NAString *arg,
                                         unsigned short toInternalIdenfifierFlags)
{
  toInternalIdentifierFlags_ = toInternalIdenfifierFlags;
  numParts_ = 1;
  seq[0] = arg;
  for (UInt32 i=numParts_; i!=MAX_NUM_PARTS; i++)
    seq[i] = NULL;

  // initialize seqPos_, seqEndPos_ with the info from the ParScannedTokenQueue.
  //
  const ParScannedTokenQueue::scannedTokenInfo &tokInfo = getTokInfo(arg);
  if (isValid())
  {
    seqPos_    = tokInfo.tokenStrPos;
    seqEndPos_ = tokInfo.tokenStrPos + tokInfo.tokenStrLen - 1;
    seqPosStartOffset_ = tokInfo.tokenStrOffset ;
  }

} // ShortStringSequence ctor

// Increments numParts_.  If numParts_ < MAX_NUM_PARTS, then sets seq[numParts_]
// to be a copy of arg.  If numParts_ >= MAX_NUM_PARTS, just the
// incrementing is all we need do (if this behavior seems odd, note the
// term "Short" in the name of this class).
void ShortStringSequence::append(NAString* arg)
{
  if (++numParts_ > MAX_NUM_PARTS)
  {
    invalidate();
    delete arg;
    return;
  }
  seq[numParts_-1] = arg;

  // update seqEndPos_ with the information from the ParScannedTokenQueue.
  //
  const ParScannedTokenQueue::scannedTokenInfo &tokInfo = getTokInfo(arg);
  if (isValid())
  {
    seqEndPos_ = tokInfo.tokenStrPos + tokInfo.tokenStrLen - 1;
    //
    // If this ShortStringSequence had a previous multi-byte component,
    // then tokInfo.tokenStrPos has extra bytes added to it which should NOT
    // be counted ar part of the length (in UCS2 chars) of the string, so
    // we must subtract off any such extra bytes that were added.
    //
    seqEndPos_ -= tokInfo.tokenStrOffset - seqPosStartOffset_ ;
  }

} // ShortStringSequence::append

// For i>=MAX_NUM_PARTS assertion fail.
// Otherwise return the pointer, seq[i], and set seq[i] to NULL.
NAString* ShortStringSequence::extract(UInt32 i)
{
  assert(i<MAX_NUM_PARTS);
  NAString  *resultValue = seq[i];
  seq[i] = NULL;
  return resultValue;
} // ShortStringSequence::extract

void ShortStringSequence::invalidate()
{
  // mark it as invalid:
  numParts_ = MAX_NUM_PARTS + 1;
  yyerror("Invalid SQL identifier");

  /*
    All callers of ShortStringSequence methods (ctor and append)
    must follow with a call to isValid, and if it isn't, then a YYABORT.
    This is because the following (commented out) does not work!
	// emulate YYABORT here, with
	// assert (*not* COMassert!)
	// to break out of parser
	assert(FALSE);
  */
}

const ParScannedTokenQueue::scannedTokenInfo&
ShortStringSequence::getTokInfo(NAString *arg)
{
  // The parser may look ahead one or more tokens.
  // So the most recently scanned token (with 0th index)
  // may not relate to the string pointed to by 'arg';
  // if not, then -1th indexed token (the previously scanned token) should.

  const ParScannedTokenQueue::scannedTokenInfo *tokInfoPtr = NULL;
  NAWchar *inputStr = SQLTEXTW();
  Int32 downTo = -1;
  for (Int32 i = 0; i >= downTo; i--)
    {
      const ParScannedTokenQueue::scannedTokenInfo &tokInfo
	= ParScannedTokens->getScannedTokenInfo(i);

      // is the i'th tok a valid SQL identifier?
      // offset is subtracted off because in THIS case it is the Wide copy

      size_t idx = tokInfo.tokenStrPos - tokInfo.tokenStrOffset;
      NAString *iTok = unicodeToChar(&inputStr[idx],
                                     tokInfo.tokenStrLen,
                                     (Lng32) ComGetNameInterfaceCharSet(), // SQLCHARSETCODE_UTF8
                                     PARSERHEAP());

      // This identifier string needs to be converted back to
      // ISO88591 because it may be passed to catman code to
      // determine if the named object exists. When catman 
      // becomes Unicode-enabled, this ISO88591 conversion
      // can go away. This comment also applies to other 
      // unicodeToChar() call sites here & elsewhere.
      if (!iTok)
        break;

      TrimNAStringSpace(*iTok);
      if (iTok->isNull() || tokInfo.tokenIsComment)
      {
        downTo--;
        delete iTok;
        continue;
      }
      
      if (!ToInternalIdentifier ( *iTok
                                , TRUE // upCase - default is TRUE
                                , TRUE // ^ ok?  - default is FLASE
                                , toInternalIdentifierFlags_
                                )
          && *arg == *iTok)
      {
        delete iTok;
        return tokInfo;
      }

      delete iTok;
      tokInfoPtr = &tokInfo;
    }

  invalidate();		// one or both tokens is an invalid SQL identifier
  return *tokInfoPtr;
  
} // ShortStringSequence::getTokInfo

// For error messages
NAWString badNameFromStrings(ShortStringSequence *names)
{
  assert(names);
  NAWString result(SQLTEXTW());
  result.remove(names->getEndPosition()+1);
  result.remove(0, names->getPosition());   // pos BEFORE the name began
  delete names;
  return result;
}

// This function is used in xxxNameFromStrings() functions below. 
void getNamePart(NAString &xxxName, ShortStringSequence *names,
                 UInt32&index)
{
  if (index>0) {
    NAString namePtr (*(names->extract(--index)));
    xxxName = namePtr;
  }
}

static NABoolean CharHereIsaDoubleQuote(StringPos p) {
  return SqlParser_CurrentParser && 
    SqlParser_CurrentParser->CharHereIsDoubleQuote(p);
}

// This function knows that if there is only one
// name it is the table name, two names, table and schema, all three
// then table, schema, and catalog.  And this is what it returns:
// a CorrName object whose corrName_ string is empty and whose
// QualifiedName fields are filled in with table, schema, and catalog.
//
// The input argument must not be NULL.  It must also have at least
// one, and not more than three (> 3 is an assertion error), name parts.
// Each of the names is extracted and then placed into a CorrName object
// that is returned.  A NULL return value implies that there was an error.

QualifiedName * qualifiedNameFromStrings(ShortStringSequence *names)
{
  assert(names);
  UInt32 index = names->numParts();
  assert(index>0);

  NAString  tblName;
  NAString  schName;
  NAString  catName;

  getNamePart(tblName,names,index);
  getNamePart(schName,names,index);
  getNamePart(catName,names,index);

  if (index) {
    // ~String0 is an invalid qualified name 
    *SqlParser_Diags << DgSqlCode(-3011)
      << DgWString0(badNameFromStrings(names));
    return NULL;
  }

  StringPos startPos = names->getPosition();
  delete names;
  QualifiedName *result = new (PARSERHEAP()) 
    QualifiedName(tblName, schName, catName, PARSERHEAP());
  ComASSERT(result);
  result->setNamePosition(startPos, CharHereIsaDoubleQuote(startPos));
  return result;
}

SchemaName * schemaNameFromStrings(ShortStringSequence *names)
{
  assert(names);
  UInt32 index = names->numParts();
  assert(index>0);

  NAString  schName;
  NAString  catName;

  getNamePart(schName,names,index);
  getNamePart(catName,names,index);

  if (index) {
    // ~String0 is an invalid qualified name 
    *SqlParser_Diags << DgSqlCode(-3011)
      << DgWString0(badNameFromStrings(names));
    return NULL;
  }

  StringPos startPos = names->getPosition();
  delete names;
  SchemaName *result = new (PARSERHEAP()) 
    SchemaName(schName, catName, PARSERHEAP());
  ComASSERT(result);
  result->setNamePosition(startPos, CharHereIsaDoubleQuote(startPos));
  return result;
}

// This method detects that a hive object is being processed and it
// sets the field foundHiveDDL_ in passed in hiveDDLInfo object.
// This field is later used to process hive DDL.
short preprocessHiveDDL(const NAString &catalogName, 
                        Parser::HiveDDLInfo *hiveDDLInfo)
{
  // In some cases, parser is called to process statements.
  // For ex, binder calls parser during view expansion in dml queries.
  // Or internal MD definitions are processed during process startup.
  // In these internal cases, skip special hive ddl processing 
  // Flag disableDDLcheck _ will be set  in these cases.
  if ((NOT hiveDDLInfo->disableDDLcheck_) &&
      (hiveDDLInfo->checkForDDL_) &&
      (NOT hiveDDLInfo->foundDDL_))
    {
      hiveDDLInfo->checkForDDL_ = FALSE;
      if (((NOT catalogName.isNull()) &&
           (catalogName.compareTo
            (HIVE_SYSTEM_CATALOG) == 0)) ||
          ((catalogName.isNull()) &&
           (CmpCommon::getDefaultString(CATALOG).compareTo
            (HIVE_SYSTEM_CATALOG) == 0)))
        {
          hiveDDLInfo->foundDDL_ = TRUE;
          hiveDDLInfo->userSpecifiedStmt_ = 
            NAString(SqlParser_CurrentParser->inputStr(), 
                     SqlParser_CurrentParser->inputStrLen());
        }
      else if (hiveDDLInfo->backquotedDelimFound_)
        {
          yyerror(""); // emit syntax error
        }
    }  

  return 0;
}

// if the schemaName part inName contains volatile schema prefix, then
// return an error. Don't do this if volatile schema prefix is allowed
// for internal queries.
// If validateVolatileName is set, then validate that:
//      -- volatile schema exists
//      -- name is a one or 2 part name
//      -- if 2-part name, then the schPart is the currentUserName.
// If updateVolatileName is set and the name is validate to be a volatile
// name, then change the schema name to be the current volatile schema
// name.
const NABoolean validateVolatileSchemaName(NAString &schName)
{
  return CmpCommon::context()->sqlSession()->
    validateVolatileSchemaName(schName);
}

SchemaName * processVolatileSchemaName(SchemaName *schName,
				       NABoolean validateVolatileName,
				       NABoolean updateVolatileName)
{
  SchemaName *result = schName;
  
  if (validateVolatileName)
    {
      //if ((!schName) ||
      //  (! validateVolatileSchemaName(*schName)))
      //return NULL;
    }

  if (updateVolatileName)
    {
      result = CmpCommon::context()->sqlSession()->updateVolatileSchemaName();
    }

  return result;
}

QualifiedName * processVolatileDDLName(QualifiedName * inName,
				       NABoolean validateVolatileName,
				       NABoolean updateVolatileName)
{
  QualifiedName *result = inName;

  if (NOT inName->getSchemaName().isNull())
    {
      if (! CmpCommon::context()->sqlSession()->
	  validateVolatileQualifiedSchemaName(*inName))
	return NULL;
    }

  if (validateVolatileName)
    {
      if (! CmpCommon::context()->sqlSession()->validateVolatileQualifiedName(*inName))
	return NULL;
    }

  if (updateVolatileName)
    {
      result = CmpCommon::context()->sqlSession()->updateVolatileQualifiedName(*inName);
    }

  return result;
}

CorrName * corrNameFromStrings(ShortStringSequence *names)
{
  QualifiedName *qn = qualifiedNameFromStrings(names);
  if (!qn) return NULL;

  CorrName *result;

  // ##SQLMP-SYNTAX-KLUDGE##
  //
  // ## Temporarily ##??, till we get ANSI name mapping working for
  // SQL/MP tables, this code stuffs the tablename into the corr name.
  //
  // MP-style queries may look like this:
  //   "SELECT T1.A, Y.B, T3.C FROM \Q.$R.S.T1, \N.$O.P.T2 Y, \K.$L.M.T3"
  // which uses non-ANSI defaulting rules for correlation names.
  //
  // If in NSK nametype mode, here we supply implicit corr names AS IF
  // the user had input
  //   "SELECT T1.A, Y.B, T3.C FROM \Q.$R.S.T1 T1, \N.$O.P.T2 Y, \K.$L.M.T3 T3"
  //
  // These implicit corr names are overridden if the user supplies
  // an explicit corr name -- by setCorrName() in rule "table_name as_clause" --
  // as in the "T2 Y" in the example here.
  //
  // ## See GenericUpdate::bindNode(), which UNDOES this temporary corr.

  const NAString &tblName = qn->getObjectName();

  result = new (PARSERHEAP()) CorrName(*qn, PARSERHEAP());

  ComASSERT(result);

  result->setNamePosition(qn->getNamePosition(),
                          CharHereIsaDoubleQuote(qn->getNamePosition()));
  return result;
}


ColRefName *colRefNameFromStrings(ShortStringSequence *names)
{
  assert(names);
  UInt32 index = names->numParts();
  assert(index>0);

  NAString  colName;
  NAString  tblName;
  NAString  schName;
  NAString  catName;

  getNamePart(colName,names,index);
  getNamePart(tblName,names,index);
  getNamePart(schName,names,index);
  getNamePart(catName,names,index);

  if (index) {
    // ~String0 is an invalid colref name 
    *SqlParser_Diags << DgSqlCode(-3002)
      << DgWString0(badNameFromStrings(names));
    return NULL;
  }

  StringPos startPos = names->getPosition();
  delete names;
  ColRefName *result = new (PARSERHEAP())
    ColRefName(colName, CorrName(tblName, PARSERHEAP(), schName, catName), PARSERHEAP());
  ComASSERT(result);
  result->setNamePosition(startPos, CharHereIsaDoubleQuote(startPos));
  return result;
}


// The purpose of this function is to convert NAStrings that contain
// delimited identifiers as detected by SqlLexer
// to a format we can use internally.
//
// If a string begins with a double quote, then
// this function takes an NAString and:
//    - there are supposed to be double quotes surrounding the string
//      and they are removed
//    - any embedded double quotes (i.e., two consecutive dquotes)
//	are turned into just one dquote
//
// If the string does not begin with a double quote, then the
// only transformation is to make all the contents upper case.
//
// Efficiency: this function saves on space at the cost of some time.
// The calls to NAString.remove() probably take linear time as a function
// of string length on each call.  A faster version of this function
// would establish a transformed string in a separate buffer and then
// copy it back into the original.

NABoolean transformIdentifier(NAString& delimIdent, 
                              Int32 upCase,
                              NABoolean acceptCircumflex    // VO: Fix genesis solution 10-040204-2957
                             ,UInt16 toInternalIdentifierFlags
                             )
{
  NAString origIdent(delimIdent);
  Lng32 sqlcode = ToInternalIdentifier(delimIdent, upCase, acceptCircumflex, toInternalIdentifierFlags);

  if (sqlcode) {
    // 3004 A delimited identifier must contain at least one character.
    // 3118 Identifier too long.
    // 3127 Illegal character in identifier $0~string0.
    // 3128 $1~string1 is a reserved word.
    *SqlParser_Diags << DgSqlCode(sqlcode) 
                     << DgString0(origIdent)
                     << DgString1(delimIdent);

    if (sqlcode > 0) sqlcode = -sqlcode;
    if (sqlcode != -3118) {
      if (sqlcode == -3127) {
        Int32 i = -delimIdent[(size_t)0];	// count of scanned chars
        if (i > 0) i = -i;
        i += ((Int32)origIdent.length()) - 1;
        if (i > 0) {
          SqlParser_CurrentParser->getLexer()->setInputPos(i);
          // point to the illegal character
        }
      }
      yyerror("");			// emit syntax error 15001
    }

    return TRUE;
  }

  return FALSE;		// no error
}


void PicStream::skipPicture()
{
   NAString   theIdentifier;
   while (isalpha(sgetc())) theIdentifier.append(toupper(sbumpc()));
   skipWhite();
   NAString string1 = "PIC";
   NAString string2 = "PICTURE";
   assert(theIdentifier == string1 || theIdentifier == string2);
}

// The skipCount function is basically a loop that on each pass
// either gets a single character, or gets a single
// character followed by some parens with an enclosed integer, and optionally
// followed by a character length unit identifier ('CHARACTERS' for R2.0).
// We assertion fail on a syntax error --- this should be okay since
// the SqlLexer.l is guaranteed (read: is supposed to) give us only
// valid strings to parse in the first place and not doing so is a
// programming error.

NABoolean PicStream::skipCount (UInt32*result, const char pattern, NABoolean isCharType)
{
   assert(toupper(sgetc()) == pattern);
   UInt32   total = 0;
   do {
      stossc();
      if (sgetc() == '(') {
         // Next we advance over the characters in a pattern
         // of `( <unsigned-int> )' where there may be white space after
         // the left parens and white space before the right parens.  It is
         // an assertion fail (and ostensibly a SqlLexer.l error) if this
         // pattern is not encountered.  The unsigned int is parsed
         // and its value added to total.
         stossc();
         skipWhite();
         assert(isdigit(sgetc()));
         UInt32 val = 0;
         do {
            val = (val*10) + sbumpc() - '0';
	    if(val > SHORT_MAX)
	      return FALSE;
	    
         } while (isdigit(sgetc()));
         skipWhite();
         char ch = sbumpc();

         if ( isCharType == TRUE && ( ch=='C' || ch=='c' ) ) {

            char len_unit_array[11]; // len("CHARACTERS") = 10
            len_unit_array[0] = ch;  // store 'c'
            Int32 n = sgetn(len_unit_array+1, 9); // get the rest of "haracters"
            len_unit_array[10] = 0;
            assert(n == 9 && strcasecmp(len_unit_array, "CHARACTERS") == 0);

            skipWhite();
            ch = sbumpc();
         }

         assert(ch==')');
         total += val;
      }
      else
         total++;
      if(total > SHORT_MAX)
	return FALSE;
   } while ( toupper(sgetc()) == pattern);
   *result = total;
   return TRUE;
}


// parsePicClause() accepts a char* and either
// assertion fails (fundamentally due to a SqlLexer.l error), or,
// yields the following data about a PIC clause:  string or not string,
// precision (length), scale,  signed or not signed.  Of course,
// scale and signedness only apply to the case of `not string.'
//
// The basic formats which this function parses are:
// PX, P9, PV9, P9V9, PS9, PSV9, PS9V9
// where P is the `PICTURE' part, X is a cobol-x's clause, 9 is a cobol-9's
// clause, and S and V are the letter S and V (upper or lower case),
// respectively.

// return TRUE if successful, FALSE if overflow
NABoolean parsePicClause(NAString *picClauseBuffer, NABoolean  *isStringPtr,
			 UInt32*precisionPtr, UInt32*scalePtr,
			 NABoolean *hasSignPtr)
{
   assert(picClauseBuffer);
   assert(precisionPtr);
   assert(isStringPtr);

   UInt32   frontPart=0;  // if a format is 99V999 then front=2, back=3
   UInt32   backPart=0;

   // We intentionally "cast away" const in the next line.
   // We need the char* pointer but the NAString::data() is defined as const.
   // Don't worry, we don't modify the string, and it wouldn't matter anyway.
   PicStream   s( (char *) picClauseBuffer->data());
   s.skipPicture();

   if (toupper(s.sgetc()) == 'X') {

      if(s.skipCount(precisionPtr, 'X', TRUE) == FALSE)
	return FALSE;

      *isStringPtr  = TRUE;
   }
   else {
      assert(hasSignPtr);
      assert(scalePtr);

      // Syntax of just 'S' or 'V' or 'SV' with no 9's are caught
      // by the picNAType() function.

      if (toupper(s.sgetc()) == 'S') {
         *hasSignPtr = TRUE;
         s.stossc();
      }
      else
         *hasSignPtr = FALSE;

      if (s.sgetc() == '9')
	if( s.skipCount(&frontPart, '9') == FALSE)
	  return FALSE;

      if (toupper(s.sgetc()) == 'V') {
         s.stossc();
         if (s.sgetc() == '9')
            if( s.skipCount(&backPart, '9') == FALSE)
	      return FALSE;
      }

      // We know this is the numeric case.
      // Also, frontPart + backPart = precision and backPart = scale.
      *precisionPtr   = frontPart + backPart;
      *scalePtr       = backPart;
      *isStringPtr    = FALSE;
   }
   assert(s.sgetc() == EOF);
   return TRUE;
}


// This function is used in the productions that handle the COBOL style
// PIC type declarations.  It accepts some parameters gleaned from the
// PIC syntax and either issues an error message, returning NULL,
// or returns a pointer to a newly allocated NAType appropriate
// to the given parameters.

NAType *picNAType(const NABoolean      isString, 
                  const DISPLAY_STYLE  style,
                  const UInt32       precision,
                  const UInt32       scale,
                  const NABoolean      hasSign,
                  const CharInfo::CharSet charset,
                  const CharInfo::Collation collation,
                  const CharInfo::Coercibility coerc,
                  const NAString &     picClauseBuffer,
		  const NABoolean      isCaseinsensitive)
{
   NAType    *returnValue=NULL;

   if (isString) {
      assert(precision > 0);
      switch (style) {
        case STYLE_DISPLAY:
          {
            CharInfo::CharSet eEncodingCharSet = charset;
            Int32 maxLenInBytes = precision;
            Int32 characterLimit = precision;
            Int32 maxBytesPerChar = CharInfo::maxBytesPerChar(charset);
            returnValue = new (PARSERHEAP())
              //            SQLChar(precision,TRUE,FALSE,isCaseinsensitive,FALSE,charset,collation,coerc);
              SQLChar(PARSERHEAP(), CharLenInfo(characterLimit, maxLenInBytes),
                      TRUE,FALSE,isCaseinsensitive,FALSE,
                      charset,collation,coerc,eEncodingCharSet);
            assert(returnValue);
          }
          break;
        case STYLE_UPSHIFT:
	    returnValue = new (PARSERHEAP())
  		SQLChar(PARSERHEAP(), precision,TRUE,TRUE,isCaseinsensitive,FALSE,charset,collation,coerc);
            assert(returnValue);
            break;
        case STYLE_LEADING_SIGN:
	    // PIC X types cannot have leading signs, or any signs.
            *SqlParser_Diags << DgSqlCode(-3038);   
            break;  
        case STYLE_COMP:
	    // PIC X types do not have any COMP representation.
            *SqlParser_Diags << DgSqlCode(-3039);
            break;  
        default:  assert(FALSE);
      }
   }
   else if (precision == 0)
     //  You can't have a precision of zero.  Add a '9' to
     //  the PICTURE clause.
         *SqlParser_Diags << DgSqlCode(-3040);
   else
      switch (style) {
        case STYLE_DISPLAY:
        case STYLE_LEADING_SIGN:
           if (scale >= 10  && !hasSign) {
	     // If scale is greater than or equal to 10, UNSIGNED
             // is invalid for a numeric or decimal type specification.
                *SqlParser_Diags << DgSqlCode(-3041);
              break;
           }
           else if (precision > 18)
           {
              // Precision of PIC 9 types, $0~string0, cannot exceed 18.
              *SqlParser_Diags << DgSqlCode(-3037)
                               << DgString0(picClauseBuffer.data());
              break;
           }
           else
              returnValue = new (PARSERHEAP())
		SQLDecimal(PARSERHEAP(), precision,scale,hasSign);
           assert(returnValue);
           break;
        case STYLE_UPSHIFT:
	  // Upshift for a numeric type is invalid.
             *SqlParser_Diags << DgSqlCode(-3042);
           break;  
        case STYLE_COMP:
           if (scale >= 10  && !hasSign) {
	      // If scale is greater than or equal to 10, UNSIGNED
              // is invalid for a numeric or decimal type specification.
              *SqlParser_Diags << DgSqlCode(-3041);
              break;
           }
           else if (precision > 18)
	      // Precision greater than eighteen invalid for a COMP
	      // numeric type.
              *SqlParser_Diags << DgSqlCode(-3043);
           else {
              const Int16 DisAmbiguate = 0; // added for 64bit project
              returnValue = new (PARSERHEAP())
		SQLNumeric(PARSERHEAP(), hasSign, precision, scale, DisAmbiguate);
              assert(returnValue);
           }
           break;
        default:  assert(FALSE);
      }
   return returnValue;
}


// : value_expression_list TOK_IN '(' value_expression_list ')'
// Convert "v IN (v1, v2 ...)" to "v=v1 OR v=v2 ...".
//
// This transformation is somewhat arbitrary (it creates a left-deep
// tree, instead of a right-deep or bushy or ...), and could easily be
// improved -- i.e., removing duplicates.
//
// NB: The left-deep nature of the resulting OR-tree is expected by
// histogram-manipulation code in /optimizer/ColStatDesc.cpp,
// CSDL::estimateCardinality().  If you change this transformation, then
// please change the code there, too.  (Or at least talk to the owner of
// the histogram code.)
ItemExpr *convertINvaluesToOR(ItemExpr *lhs, ItemExpr *rhs)
{
  NABoolean err = FALSE;
  if (lhs->getOperatorType() == ITM_ITEM_LIST)
    err = TRUE;
  else if (lhs->getOperatorType() == ITM_ROW_SUBQUERY)
    {
      RelExpr *subq = ((RowSubquery *)lhs)->getSubquery();
      CMPASSERT( subq->getOperatorType() == REL_ROOT );

      RelRoot *sq = (RelRoot *)subq;
      RelExpr *theChild;
      theChild = sq->child(0);
  
      if (sq->getCompExprTree() &&	// what if no compExprTree()?
          sq->getCompExprTree()->getOperatorType() == ITM_ITEM_LIST)
        err = TRUE;
      else if (theChild->getOperatorType() == REL_TUPLE &&
               ((Tuple *)theChild)->tupleExprTree()->getOperatorType() == 
                                       ITM_ITEM_LIST)
        err = TRUE;
    }
  if (err)
    {
      // 3147 The left operand of an IN predicate whose right operand is
      //      a value list must be scalar (degree of one).
      *SqlParser_Diags << DgSqlCode(-3147);
      return NULL;
    }

  ExprValueId rightListId = rhs;

  ItemExprTreeAsList *enl = new (PARSERHEAP())
    ItemExprTreeAsList(&rightListId, ITM_ITEM_LIST);

  ItemExpr *result = NULL;
  CollIndex nEnlEntries = (CollIndex) enl->entries();
  for (Lng32 i = 0; i < nEnlEntries ; i++)
    {
      BiRelat *eqpred = new (PARSERHEAP()) BiRelat(ITM_EQUAL, lhs, (*enl)[i]);
      eqpred->setCreatedFromINlist(TRUE);
      if (!result)
	result = eqpred;
      else
	result = new (PARSERHEAP()) BiLogic(ITM_OR, result, eqpred);
    }

  if (result && result->getOperatorType() == ITM_OR)
    ((BiLogic*)result)->setCreatedFromINlist(TRUE);
  return result;
}


// quantified_predicate : value_expression_list '=' quantifier rel_subquery
ItemExpr *makeQuantifiedComp(ItemExpr *lhs,
                             OperatorTypeEnum compOpType,
                             Int32 quantifierTok,
                             RelExpr *subquery)
{
  assert(quantifierTok == TOK_ALL || quantifierTok == TOK_ANY);
  NABoolean all = (quantifierTok == TOK_ALL);

  OperatorTypeEnum resultOpType = NO_OPERATOR_TYPE;
  switch (compOpType)
    {
    case ITM_EQUAL:
         resultOpType 	= all ? ITM_EQUAL_ALL      : ITM_EQUAL_ANY;      break;
    case ITM_LESS:
         resultOpType 	= all ? ITM_LESS_ALL       : ITM_LESS_ANY;       break;
    case ITM_GREATER:
         resultOpType 	= all ? ITM_GREATER_ALL    : ITM_GREATER_ANY;    break;
    case ITM_NOT_EQUAL:
	 resultOpType 	= all ? ITM_NOT_EQUAL_ALL  : ITM_NOT_EQUAL_ANY;  break;
    case ITM_LESS_EQ:
	 resultOpType 	= all ? ITM_LESS_EQ_ALL    : ITM_LESS_EQ_ANY;    break;
    case ITM_GREATER_EQ:
	 resultOpType	= all ? ITM_GREATER_EQ_ALL : ITM_GREATER_EQ_ANY; break;
    default:
	 assert(FALSE);
    }
  return new (PARSERHEAP()) QuantifiedComp(resultOpType, lhs, subquery, FALSE);
}

///////////////////////////////////////////////////////////////////////////
// this method converts an IN list to a VALUES subquery or
// converts it to an OR predicate.
// Conversion to VALUES SQ is done
// if all IN list elements are constants or parameters and
// the number of elements are greater than 100. For less than
// 100, it might be better to use the IN predicate so mdam could
// be chosen. The number 100 is experimental and may change.
//   <value> IN (<val1>, ...., <valN>)  gets converted to
//   <value> IN (VALUES(<val1>), ..., (<valN))
// Otherwise, IN list is converted to ORs.
//
// IN list is also converted to ORs if all consts do not have compatible
// types. (like, all numerics, or all characters...). This is done since
// we don't (yet) support incompatible types within a TupleList.
//
// processINlist should be moved to Binder. Will do that later.
//
///////////////////////////////////////////////////////////////////////////
ItemExpr *processINlist(ItemExpr *lhs, ItemExpr *rhs)
{
  ItemExpr * retItemExpr = NULL;
  Lng32 defaultsLimit = ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_22);
  if (rhs->castToItemExpr()->getOperatorType() == ITM_ITEM_LIST)
    {
      ItemList * il = (ItemList*)rhs;
      if ((defaultsLimit > 0) && // if defaultsLimit == 0 the the feature is turned OFF.
          (il->doesExprEvaluateToConstant(FALSE, FALSE)) &&
	  (il->numOfItems() > defaultsLimit)
         )
	{
	  // insert a convert node on top of each child item expr
	  ItemExpr * currIL = il;
	  NABoolean incompatibleTypes = FALSE;
	  NABoolean negate;
          ConstValue* prevCVExpr = currIL->child(0)->castToConstValue(negate);

	  NABuiltInTypeEnum prevEnum = ( prevCVExpr == NULL ) ? 
             NA_UNKNOWN_TYPE : prevCVExpr->getType()->getTypeQualifier()  ;

          ConstValue* currCVExpr = NULL;
          CollIndex index = 0;

	  while (1)
	    {
	      currCVExpr = currIL->child(index)->castToConstValue(negate);

	      if ( currCVExpr ) {
                NABuiltInTypeEnum currEnum = 
                       currCVExpr->getType()->getTypeQualifier();
	        if ( prevEnum == NA_UNKNOWN_TYPE ) {
                  prevEnum = currEnum;
                } else {
	          if (currEnum != prevEnum) {
		    incompatibleTypes = TRUE;
                    break;
                  }
                }
              } else {
                if (currIL->child(index)->castToItemExpr()->getOperatorType()
                  != ITM_DYN_PARAM) 
                {
                  incompatibleTypes = TRUE;
                  break;
                }
              }

	      Convert * cnv = new(PARSERHEAP()) Convert(currIL->child(index));
	      currIL->setChild(index, cnv);

              if ( index == 1 ) break;

	      if (currIL->child(1)->castToItemExpr()->getOperatorType() 
		  == ITM_ITEM_LIST)
		currIL = currIL->child(1);
	      else
                index = 1; 
            }

	  if (NOT incompatibleTypes)
	    {
	      // convert to VALUES subq.
	      TupleList * tl = new(PARSERHEAP()) TupleList(il);
          tl->setCreatedForInList(TRUE);
	      RelRoot * rr = new (PARSERHEAP()) RelRoot(tl);
	      retItemExpr = 
		makeQuantifiedComp(lhs, ITM_EQUAL, TOK_ANY, rr); 
	      if (retItemExpr)
		((QuantifiedComp*)retItemExpr)->setCreatedFromINlist(TRUE);
	    }
	}
    }
  
  if (! retItemExpr)
    retItemExpr = convertINvaluesToOR(lhs, rhs);

  return retItemExpr;
}

ItemExpr *makeBetween(ItemExpr *x, ItemExpr *y, ItemExpr *z, Int32 tok)
{
  ItemExpr *result = new (PARSERHEAP()) Between(x, y, z);
  if (tok == TOK_NOT_BETWEEN)
    result = new (PARSERHEAP()) UnLogic(ITM_NOT, result);
  return result;
}

// Change the <sqltext> arg of a CQD from
//	SET SCHEMA X.Y;		-- unquoted: Tandem syntax extension
//	SET MPLOC $V.SV;	-- MPLOC:    Tandem syntax extension
//	SET MPLOC '$V.SV';	-- MPLOC:    Tandem syntax extension
// into
//	SET SCHEMA 'X.Y';	-- string literal: Ansi syntax, MX canonical fmt
//	SET MP_SUBVOLUME '$V.SV';  -- string lit:  Tdm ext, MX canonical format
//	SET MP_SUBVOLUME '$V.SV';  -- string lit:  Tdm ext, MX canonical format
//
// This needs to be called ONLY for:
// - SET cqd's (not DECLARE cqd's), and
//   - the SET cqd's unquoted (non-string-literal) variants, or
//   - or if we are otherwise rewriting the user input text
//     (e.g. the syntactic sugar of "SET MPLOC" --
//	there is no NADefaults attribute of MPLOC --
//	NADefaults parses a multi-part MP_SUBVOLUME instead).
//
ControlQueryDefault *normalizeDynamicCQD(const char *attrName,
                                         const NAString &attrValue)
{
  size_t len = attrValue.length() + 2;		// assume only two quotes
  NAString quotedValue(len);
  ToQuotedString(quotedValue, attrValue);

  len += 4 + strlen(attrName) + 2;
  NAString tmpSQLTEXT(len);
  tmpSQLTEXT  = "SET ";
  tmpSQLTEXT += attrName;
  tmpSQLTEXT += quotedValue + ";";
  return new (PARSERHEAP())
    ControlQueryDefault(
      tmpSQLTEXT,
      CharInfo::UTF8
      , attrName, attrValue, TRUE/*dynamic*/);
}

// return the relexpr tree that evaluates an empty compound statement
RelExpr* getEmptyCSRelTree()
{
     ItemExpr *tupleExpr = new (PARSERHEAP()) ConstValue();
     RelExpr *tuple = new (PARSERHEAP()) Tuple(tupleExpr);
     ItemExpr *predicate = new (PARSERHEAP()) BoolVal(ITM_RETURN_FALSE);
     tuple->addSelPredTree(predicate);
     return tuple;
}

RelExpr* 
getIfRelExpr(ItemExpr* condition, RelExpr* thenBranch, RelExpr* elseBranch)
{
   if ( thenBranch == NULL && elseBranch == NULL ) {
    delete condition;
    return NULL;
   }
  NABoolean thenBranchAloneIsNull = FALSE ;
  NABoolean elseBranchAloneIsNull = FALSE ;

   if ( thenBranch == NULL ) {
      thenBranch = getEmptyCSRelTree();
      thenBranchAloneIsNull = TRUE ;
   }

   if (elseBranch == NULL) {
     elseBranch = getEmptyCSRelTree();
     elseBranchAloneIsNull = TRUE ;
   }
      

   Union *u = new (PARSERHEAP())
          Union(thenBranch, elseBranch, NULL, condition);
   u->setUnionForIF();


   if ( thenBranchAloneIsNull ) {
      u->setCondEmptyIfThen();
   }

   if (elseBranchAloneIsNull) {
     u->setCondEmptyIfElse();
   }
   
   return u;
}

// EJF L4J - dynamic CQD not allowed inside Compound Statements
NABoolean beginsWith(char *sqltext, const char *kwd)
{
  // Assumes Prettify has been called, so only one space (at most) btw tokens.
  // If this is called more than once, the second time in the text might begin
  // with a delimiting space.
  NAString sqlText(sqltext);
  size_t i = 0;
  size_t flen = sqlText.length();

  if (!sqlText.isNull())  {
    while (i < flen)  {
      if (isspace((unsigned char)sqlText[size_t(0)]))  // For VS2003
        sqlText.remove(0, 1);
      else
        i = flen;
      }
    }

  size_t len = strlen(kwd);
  if (sqlText.length() > len) {
    NAString tmp(sqlText);
    tmp.remove(len);
    if (tmp == kwd)
      return TRUE;
    }

  return FALSE;
}

void setHVorDPvarIndex(ItemExpr * expr, NAString *name)
{
  // This method sets the var index value to HV or DP
  // in sequential order. The duplicate HV or DP will be eliminated
  // during 'bind' phase.
  OperatorTypeEnum opType = expr->getOperatorType();

  if (inCallStmt)
  {
    if (ITM_HOSTVAR == opType)
      ((HostVar *)expr)->setPMOrdPosAndIndex(COM_INPUT_COLUMN,
                                             1,
                                             currVarIndex++);
    else
      ((DynamicParam *)expr)->setPMOrdPosAndIndex(COM_INPUT_COLUMN,
                                                  1,
                                                  currVarIndex++);
  }
}

//ct-bug-10-030102-3803 Begin
void  conditionalDelimit(NAString &tmpName, const NAString &tmp)
{
   if(tmp.contains(".",NAString::exact) || tmp.contains("*",NAString::exact))
   {
       tmpName.append("\"",1);
       tmpName.append(tmp.data(),tmp.length());
       tmpName.append("\"",1);
   }
   else
   {
       tmpName.append(tmp.data(),tmp.length());
   }
}
//ct-bug-10-030102-3803 End
Lng32 getDefaultMaxLengthForLongVarChar(CharInfo::CharSet cs)
{
  if (IdentifyMyself::IsPreprocessor() == TRUE )
     return INT_MAX;

  switch(cs) {
  case CharInfo::UNICODE:
    return (Lng32)CmpCommon::getDefaultNumeric(MAX_LONG_WVARCHAR_DEFAULT_SIZE);
    break;

  default:
    return (Lng32)CmpCommon::getDefaultNumeric(MAX_LONG_VARCHAR_DEFAULT_SIZE);
    break;
  }
}

Lng32 getDefaultMinLengthForLongVarChar(CharInfo::CharSet cs)
{
  if (IdentifyMyself::IsPreprocessor() == TRUE )
     return 0;

  switch(cs) {
  case CharInfo::UNICODE:
    return (Lng32)CmpCommon::getDefaultNumeric(MIN_LONG_WVARCHAR_DEFAULT_SIZE);
    break;

  default:
    return (Lng32)CmpCommon::getDefaultNumeric(MIN_LONG_VARCHAR_DEFAULT_SIZE);
    break;
  }
}

NABoolean getCharSetInferenceSetting(NAString& defVal)
{
  if (IdentifyMyself::IsPreprocessor() == TRUE )
    return FALSE;

  return (CmpCommon::getDefault(INFER_CHARSET, defVal) == DF_ON);
}

NABoolean allowRandFunction()
{
  if (IdentifyMyself::IsPreprocessor() == TRUE 
     )
    return TRUE;
  else
    return CmpCommon::getDefault(ALLOW_RAND_FUNCTION) == DF_ON;
}
    
RelExpr * getTableExpressionRelExpr(
     RelExpr *fromClause,
     ItemExpr *whereClause,
     RelExpr *sampleClause,
     RelExpr *transClause,
     ItemExpr *seqByClause,
     ItemExpr *groupByClause,
     ItemExpr *havingClause,
     ItemExpr *qualifyClause,
     NABoolean hasTDFunctions,
     NABoolean hasOlapFunctions)
{
  if (qualifyClause && seqByClause)
  {
    *SqlParser_Diags << DgSqlCode(-4360); 
    return NULL;
  }

  if(hasOlapFunctions && seqByClause)
  {
    *SqlParser_Diags << DgSqlCode(-4345);
    return NULL;
  }
  // childPtr is the current child node
  // as the tree is built.  At the end
  // it is the root of the table expression tree.
  //
  RelExpr *childPtr = fromClause;
  NABoolean groupByClauseProcessed = FALSE;

  
  // add where clause as a selection pred
  //
  if (whereClause)
    childPtr->addSelPredTree(whereClause);

   if(groupByClause || havingClause)
   { 
    // we are making this change so that for the following query the groupby is 
    // associlated with the last select statement and not the entire unioned result.
    //  select a from t union select a from t order by 1 group by 1;
    if ((CmpCommon::getDefault(MODE_SPECIAL_1) == DF_ON) &&
      ((childPtr->getOperatorType() == REL_GROUPBY) && // group by for union distinct
	childPtr->child(0) && (childPtr->child(0)->getOperatorType() == REL_ROOT) &&
	childPtr->child(0)->child(0) &&
	  (childPtr->child(0)->child(0)->getOperatorType() == REL_UNION) &&
	 childPtr->child(0)->child(0)->child(1) &&
	 (childPtr->child(0)->child(0)->child(1)->getOperatorType() == REL_ROOT) &&
	 childPtr->child(0)->child(0)->child(1)->child(0)) 
	 ||   // union all case is below
	  ((childPtr->getOperatorType() == REL_UNION) && 
	    childPtr->child(1) && (childPtr->child(1)->getOperatorType() == REL_ROOT) &&
	    childPtr->child(1)->child(0)))
    {
      RelExpr * unionGroupByChild ;
      RelExpr * unionChild;
      if (childPtr->getOperatorType() == REL_GROUPBY)
      {
	unionGroupByChild = childPtr->child(0)->child(0)->child(1)->child(0);
	unionChild = childPtr->child(0)->child(0)->child(1);
      }
      else
      {
	unionGroupByChild = childPtr->child(1)->child(0);
	unionChild = childPtr->child(1);
      }

      RelExpr * unionGrby ;
	if(unionGroupByChild->getOperatorType() != REL_GROUPBY)
	{
	  unionGrby = new (PARSERHEAP())
	  GroupByAgg(unionGroupByChild,
		    REL_GROUPBY,
		    groupByClause);
	// add having clause as a selection pred
	unionGrby->addSelPredTree(havingClause);
	if (childPtr->getOperatorType() == REL_GROUPBY)
	  childPtr->child(0)->child(0)->child(1)->child(0) = unionGrby;
	else
	  childPtr->child(1)->child(0) = unionGrby;
	}
	else
	{
	  yyerror("");
	}
      groupByClauseProcessed = TRUE;
    }
  }
  
  // Add the optional sample clause.
  //
  if(sampleClause) {
    sampleClause->setChild(0, childPtr);
    childPtr = sampleClause;
  }
  
  // Add chain of transpose clauses
  //
  if(transClause) {
    RelExpr *chain = transClause;
    while(chain->child(0))
      chain = chain->child(0);
    chain->setChild(0,childPtr);
    childPtr = transClause;
  }
  
  // Add the optional sequence by clause.
  // Used with sequence functions.
  //
  if(seqByClause) {
    childPtr = new (PARSERHEAP())
      RelSequence(childPtr, seqByClause);
  }
  else if( hasTDFunctions ) {  
    RelSequence *seqNode = new (PARSERHEAP()) RelSequence(childPtr,NULL);    
    seqNode->setHasTDFunctions(hasTDFunctions);
    childPtr = seqNode;
  }
  
  if (!hasTDFunctions)
  {
    if (qualifyClause)
    { //Using Qualify clause without using rank function in the query is not allowed.
      *SqlParser_Diags << DgSqlCode(-4363);
      return NULL;
    }
    if((groupByClause || havingClause) && (NOT groupByClauseProcessed))
    { 
      childPtr = new (PARSERHEAP())
	GroupByAgg(childPtr,
		   REL_GROUPBY,
		   groupByClause);
      // add having clause as a selection pred
      childPtr->addSelPredTree(havingClause);

      if (groupByClause)
        ((GroupByAgg*)childPtr)->setIsRollup(groupByClause->isGroupByRollup());
    }

    // sequence node goes right below rel root
    if( !seqByClause && hasOlapFunctions )
    {
      RelSequence *seqNode = new (PARSERHEAP()) RelSequence(childPtr,NULL);
      seqNode->setHasOlapFunctions(hasOlapFunctions);
      childPtr = seqNode;
    }
  } // !hasTDFunctions
  else
  {
    if (!seqByClause)
    { 
      // for TD rank, the group by becomes the partition by
      if (groupByClause)
      {
	((RelSequence *)childPtr)->
          setPartitionBy(groupByClause->copyTree(CmpCommon::statementHeap()));
      }
      if (qualifyClause)
      {
	childPtr->addSelPredTree(qualifyClause);
      }
    }
  }

  return childPtr;
}
RelExpr * processReturningClause(RelExpr * re, UInt32 returningType)
{
  // disable returning clause if in a compound statement.
  if (in3GL_)
    return NULL;

  Insert * insert = (Insert *)re;

  NAString nas("SYSKEY");
  ColRefName * newColRefName = new(PARSERHEAP())
    ColRefName(nas, PARSERHEAP());
  if (newColRefName == NULL) 
    return NULL;
  ColReference * cr = new (PARSERHEAP()) ColReference(newColRefName);

  // this flag sets up the insert operator to return rows.
  // Maybe it should be renamed to indicate that inserted rows are
  // being returned.
  insert->setMtsStatement(TRUE);
  
  RelRoot * root = new (PARSERHEAP())
    RelRoot(insert, 
	    TransMode::ACCESS_TYPE_NOT_SPECIFIED_, 
	    LockMode::LOCK_MODE_NOT_SPECIFIED_, 
	    REL_ROOT, cr);
  
  if ((insert->child(0)) &&
      (insert->child(0)->getOperatorType() != REL_TUPLE))
    {
      // indicate that [last 1] is needed. LAST 1 is indicated by -3.
      if (root)
	root->setFirstNRows(-3);
    }

  return root;
}

// Process the sequence generator options.
// Ensure that the number is not negative and not larger than
// the maximum allowed for an Int64
NABoolean validateSGOption(NABoolean positive,
			   NABoolean negAllowed,
			   char * numStr,
			   const char * optionName,
			   const char * objectType)
{
  Lng32 strLen = (Lng32)(strlen(numStr));
  Int64  theValue;
  Lng32 convErrFlag = ex_conv_clause::CONV_RESULT_OK;

  // if the option is a negative number and negative numbers are not allowed,
  // prepare a diagnostic and return.
  // In the future, negatives may be allowed.
  if (!positive && !negAllowed)
      {
       *SqlParser_Diags << DgSqlCode(-1572)
        << DgString0(optionName)
        << DgString1(objectType);
        return FALSE;
      }

  // Mark the diagnostics.
  // Any possible diagnostics added by convDoIt
  // will be removed.
  Lng32 markValue = SqlParser_Diags->mark();

  /* for char(n), we limit the value of n to be no more than
     2^63-1, so convert it to 64bit signed first, then check 
     to make sure the converted value is not negative. */

  ex_expr::exp_return_type result = 
         convDoIt(numStr,               /*source*/
	          strLen,               /*sourceLen*/
		  REC_BYTE_F_ASCII,     /*sourceType*/
		  0,                    /*sourcePrecision*/
		  0,                    /*sourceScale*/
	          (char *)&theValue,    /*target*/
	          sizeof(theValue),     /*targetLen*/
	          REC_BIN64_SIGNED,     /*targetType*/
	          0,                    /*targetPrecision*/
	          0,                    /*targetScale*/
                  NULL,                 /*varCharLen*/
                  0,                    /*varCharLenSize*/
	          PARSERHEAP(),         /*heap*/
	          &SqlParser_Diags,     /*diagsArea*/
		  CONV_ASCII_BIN64S,    /*index*/
                  &convErrFlag,         /*dataConversionErrorFlag*/
	          0                     /*flags*/);

      switch(convErrFlag){
      case ex_conv_clause::CONV_RESULT_ROUNDED_DOWN:
      case ex_conv_clause::CONV_RESULT_ROUNDED_DOWN_TO_MAX:
      case ex_conv_clause::CONV_RESULT_ROUNDED_UP:
      case ex_conv_clause::CONV_RESULT_ROUNDED_UP_TO_MIN:
      case ex_conv_clause::CONV_RESULT_ROUNDED:
      case ex_conv_clause::CONV_RESULT_FAILED:
        result = ex_expr::EXPR_ERROR;
	break;
      default:
        break;
      }
      
      if (result == ex_expr::EXPR_ERROR)
      {
	// Rewind any errors added by convDoIt
       SqlParser_Diags->rewind(markValue);

       // Set the correct error
       *SqlParser_Diags << DgSqlCode(-1576)
        << DgString0(optionName)
        << DgString1(objectType);
	return FALSE;
      }

      return TRUE;
}


/*
//  INSERT2000 COLUMN FIX STARTS HERE
//  Please refer to SqlParserAux.h for class details and comments.

RearrangeValueExprList * RearrangeValueExprList::tail = NULL;

RearrangeValueExprList::RearrangeValueExprList() 
{
	next = NULL;
	prev = NULL;
	value = NULL;
}

// start to return the value_expression_list in reverse order.
ItemExpr* RearrangeValueExprList::Return_ValueExprList()
{
	RearrangeValueExprList *tmp;
	tmp = new (PARSERHEAP()) RearrangeValueExprList();
	if(tmp->tail->prev != NULL) { // redundant check
		tmp->value = new (PARSERHEAP()) ItemList(tmp->tail->prev->value, tmp->tail->value);
		(tmp->tail->prev->prev != NULL) ? 
			(tmp->tail = tmp->tail->prev->prev) : (tmp->tail = NULL);
	}

    while (tmp->tail != NULL) {
		tmp->value = new (PARSERHEAP()) ItemList(tmp->tail->value, tmp->value);
		(tmp->tail->prev != NULL) ? 
			(tmp->tail = tmp->tail->prev) : (tmp->tail = NULL);
	}
      
	tmp->tail = NULL;
	return tmp->value;

}

// ( ItemExpr -2, ItemExpr 0 )
ItemExpr* RearrangeValueExprList::Store_ValueExprList(ItemExpr *j, ItemExpr *i)   
{
	if (j != NULL) 	{
		RearrangeValueExprList *newNode;
		newNode = new (PARSERHEAP()) RearrangeValueExprList();
		if (newNode->tail != NULL) {
			// this condition will be TRUE only for the statement like follow:
			// insert into table_name values (1,2,3),(4,5,6),(7,8,9);
			// call the code to store call the Return_ValueExprList
			// Store the final tmp->value in a list.
			MultiValueExprList *tmp;
			tmp = new (PARSERHEAP()) MultiValueExprList();
			tmp->value = newNode->Return_ValueExprList();
			tmp->Store_MultiValueExprList(tmp->value);

			// this would be the next new row
			newNode->value = j;
			newNode->tail = newNode;			
		}
		else {  // the first node. 		
			newNode->value = j;
			newNode->tail = newNode;
		}
	}

	if (i != NULL) 	{
		RearrangeValueExprList *newNode2;
		newNode2 = new (PARSERHEAP()) RearrangeValueExprList();
		if (newNode2->tail != NULL)	{
			newNode2->tail->next = newNode2;
			newNode2->prev = newNode2->tail;
			newNode2->value = i;
			newNode2->tail = newNode2;
		}
		else {
			// this code should not be executed since
			// "i" will never be the first node.. failsafe.
			newNode2->value = i;
			newNode2->tail = newNode2;
		}
	}
	
	// Expecting some value.
	// Correct value assigned later.
	return i;
}

RearrangeValueExprList::~RearrangeValueExprList()
{
  // setting free resources gives fragmentation error. remove this?
}


MultiValueExprList * MultiValueExprList::tail = NULL;

MultiValueExprList::MultiValueExprList() 
{
	next = NULL;
	prev = NULL;
	value = NULL;
}

void MultiValueExprList::Store_MultiValueExprList(ItemExpr *i)
{
	if (i != NULL) 	{
		MultiValueExprList *newNode;
		newNode = new (PARSERHEAP()) MultiValueExprList();
		if (newNode->tail != NULL) {
			newNode->tail->next = newNode;
			newNode->prev = newNode->tail;
			newNode->value = i;
			newNode->tail = newNode;
		}
		else {
			newNode->value = i;
			newNode->tail = newNode;
		}
	}
}

ItemExpr* MultiValueExprList::Return_MultiValueExprList()
{
	MultiValueExprList *tmpNode;
	tmpNode = new (PARSERHEAP()) MultiValueExprList();
	if (tmpNode->tail != NULL)	{
		tmpNode = tmpNode->tail;
		(tmpNode->tail->prev != NULL) ?
			(tmpNode->tail = tmpNode->tail->prev) : (tmpNode->tail = NULL) ;
	}
	return tmpNode->value;
}

MultiValueExprList::~MultiValueExprList()
{
  // setting free resources gives fragmentation error. 
}
*/

// INSERT2000 COLUMN FIX ENDS HERE

ItemExpr *buildUdfExpr(NAString *udfName,
                       NAString *fixedInput,
                       ItemExpr *valueList)
{
  NAString udfNameUpper = *udfName;
  udfNameUpper.toUpper();

  // Construct a 3-part UDF name
  QualifiedName *qualifiedName = new (PARSERHEAP())
    QualifiedName(udfNameUpper,
                  "", // schema name
                  "", // catalog name
                  PARSERHEAP());
  
  // Create a RelExpr
  IsolatedScalarUDF *udfFunc = new (PARSERHEAP())
    IsolatedScalarUDF(*qualifiedName, PARSERHEAP());
  
  // Give the RelExpr a pointer to the input and output
  // parameters. The output datatype is not known yet but
  // still a dummy ItemExpr is required.
  ConstValue *dummyOutVal = new (PARSERHEAP()) ConstValue(0);
  if (valueList)
  {
    ItemList *inputsAndReturn = new (PARSERHEAP())
      ItemList(valueList, dummyOutVal);
    udfFunc->setProcAllParamsTree(inputsAndReturn);
  }
  else
  {
    udfFunc->setProcAllParamsTree(dummyOutVal);
  }
  
  // Give the RelExpr a pointer to the fixed input string
  //if (fixedInput)
    // XXX Ignoring this for now
    //udfFunc->setFixedInput(*fixedInput);
  
  // Package the RelExpr as a scalar subquery
  RelRoot *root = new (PARSERHEAP()) RelRoot(udfFunc);
  ItemExpr *result = new (PARSERHEAP()) RowSubquery(root);

  return result;
}

// -----------------------------------------------------------------------
// SqlParserAux_buildUdfOptimizationHint
// -----------------------------------------------------------------------

ElemDDLNode *
SqlParserAux_buildUdfOptimizationHint ( Int32       tokvalStage     // in
                                      , Int32       tokvalResource  // in
                                      , ComSInt32 cost            // in
                                      )
{
  ElemDDLUdfOptimizationHint * pNode = NULL;
  ComUdfOptimizationHintKind kind = COM_UDF_INITIAL_CPU_COST;
  if (tokvalStage EQU TOK_INITIAL)
  {
    switch (tokvalResource)
    {
    case TOK_CPU:     kind = COM_UDF_INITIAL_CPU_COST;     break;
    case TOK_IO:      kind = COM_UDF_INITIAL_IO_COST;      break;
    case TOK_MESSAGE: kind = COM_UDF_INITIAL_MESSAGE_COST; break;
    default:          return pNode;                        break;
    }
  }
  else if (tokvalStage EQU TOK_NORMAL)
  {
    switch (tokvalResource)
    {
    case TOK_CPU:     kind = COM_UDF_NORMAL_CPU_COST;     break;
    case TOK_IO:      kind = COM_UDF_NORMAL_IO_COST;      break;
    case TOK_MESSAGE: kind = COM_UDF_NORMAL_MESSAGE_COST; break;
    default:          return pNode;                       break;
    }
  }
  else
    return pNode;
  pNode = new (PARSERHEAP()) ElemDDLUdfOptimizationHint(kind);
  pNode->setCost(cost);
  return pNode;
}

// -----------------------------------------------------------------------
// SqlParserAux_buildAlterFunction
// -----------------------------------------------------------------------

StmtDDLNode *
SqlParserAux_buildAlterFunction ( QualifiedName * ddl_qualified_name_of_udf                // in - deep copy
                                , ElemDDLNode   * optional_alter_passthrough_inputs_clause // in
                                , ElemDDLNode   * optional_add_passthrough_inputs_clause   // in
                                , ElemDDLNode   * optional_function_attribute_list         // in
                                )
{
  QualifiedName noRoutineActionName(PARSERHEAP());
  StmtDDLAlterRoutine *pNode66 = new (PARSERHEAP())
    StmtDDLAlterRoutine
    ( COM_UDF_NAME                             // in - function name space
    , *ddl_qualified_name_of_udf               // in - deep copy
    , noRoutineActionName                      // in - deep copy
    , COM_UNKNOWN_ROUTINE_TYPE                 // in - either scalar or universal function
    , optional_alter_passthrough_inputs_clause // in - shallow copy
    , optional_add_passthrough_inputs_clause   // in - shallow copy
    , optional_function_attribute_list         // in - shallow copy
    , PARSERHEAP()
    );
  pNode66->synthesize();
  return pNode66;
}

// -----------------------------------------------------------------------
// SqlParserAux_buildAlterAction
// -----------------------------------------------------------------------

StmtDDLNode *
SqlParserAux_buildAlterAction ( QualifiedName * ddl_qualified_name_of_uudf               // in - deep copy
                              , QualifiedName * ddl_qualified_name_of_action             // in - deep copy
                              , ElemDDLNode   * optional_alter_passthrough_inputs_clause // in
                              , ElemDDLNode   * optional_add_passthrough_inputs_clause   // in
                              , ElemDDLNode   * optional_function_attribute_list         // in
                              )
{
  StmtDDLAlterRoutine *pNode77 = new (PARSERHEAP()) StmtDDLAlterRoutine
    ( COM_UUDF_ACTION_NAME                     // in - routine action name space
    , *ddl_qualified_name_of_uudf              // in - deep copy
    , *ddl_qualified_name_of_action            // in - deep copy
    , COM_ACTION_UDF_TYPE                      // in - routine action type
    , optional_alter_passthrough_inputs_clause // in - shallow copy
    , optional_add_passthrough_inputs_clause   // in - shallow copy
    , optional_function_attribute_list         // in - shallow copy
    , PARSERHEAP()
    );
  pNode77->synthesize();
  return pNode77;
}

// -----------------------------------------------------------------------
// SqlParserAux_buildAlterTableMappingFunction
// -----------------------------------------------------------------------

StmtDDLNode *
SqlParserAux_buildAlterTableMappingFunction
  ( QualifiedName * ddl_qualified_name_of_table_mapping_udf  // in - deep copy
  , ElemDDLNode   * optional_alter_passthrough_inputs_clause // in
  , ElemDDLNode   * optional_add_passthrough_inputs_clause   // in
  , ElemDDLNode   * optional_function_attribute_list         // in
  )
{
  QualifiedName noRoutineActionName(PARSERHEAP());
  StmtDDLAlterRoutine *pNode88 = new (PARSERHEAP()) StmtDDLAlterRoutine
    ( COM_TABLE_NAME // table mapping function name belongs to table name space
    , *ddl_qualified_name_of_table_mapping_udf // in - deep copy
    , noRoutineActionName                      // in - deep copy
    , COM_TABLE_UDF_TYPE                       // in - ComRoutineType
    , optional_alter_passthrough_inputs_clause // in - shallow copy
    , optional_add_passthrough_inputs_clause   // in - shallow copy
    , optional_function_attribute_list         // in - shallow copy
    , PARSERHEAP()
    );
  pNode88->synthesize();
  return pNode88;
}

// -----------------------------------------------------------------------
// SqlParserAux_buildDropAction
// -----------------------------------------------------------------------

StmtDDLNode *
SqlParserAux_buildDropAction ( QualifiedName * ddl_qualified_name_of_uudf   // in - deep copy
                             , QualifiedName * ddl_qualified_name_of_action // in - deep copy
                             , NABoolean       optional_cleanup             // in
                             , ComDropBehavior optional_drop_behavior       // in
                             , NABoolean       optional_validate            // in
                             , NAString      * optional_logfile             // in - deep copy
                             )
{
  // If CLEANUP, VALIDATE, or LOG option specified,
  // ALLOW_SPECIALTABLETYPE must also be specified
  if ( (optional_cleanup  OR  optional_validate  OR  optional_logfile)  AND
       NOT Get_SqlParser_Flags(ALLOW_SPECIALTABLETYPE) )
  {
    return NULL; // Error: internal syntax only!
  }

  NAString *pLogFile = NULL;
  if (optional_logfile NEQ NULL) // logfile specified
  {
    pLogFile = new (PARSERHEAP()) NAString ( optional_logfile->data()
                                           , PARSERHEAP()
                                           );
  }
  ddl_qualified_name_of_uudf->setObjectNameSpace(COM_UDF_NAME);
  ddl_qualified_name_of_action->setObjectNameSpace(COM_UUDF_ACTION_NAME);
  return new (PARSERHEAP()) StmtDDLDropRoutine
    ( COM_ACTION_UDF_TYPE            // in - ComRoutineType
    , *ddl_qualified_name_of_uudf    // in - QualifiedName * - deep copy
    , *ddl_qualified_name_of_action  // in - QualifiedName * - deep copy
    , optional_drop_behavior         // in - ComDropBehavior
    , optional_cleanup               // in - NABoolean       - for CLEANUP  mode set to TRUE
    , optional_validate              // in - NABoolean       - for VALIDATE mode set to FALSE (?)
    , pLogFile                       // in - NAString *      - shallow copy
    , PARSERHEAP()
    );
  // Do not delete pLogFile because we did a shallow copy
}

// -----------------------------------------------------------------------
// SqlParserAux_buildDropRoutine
// -----------------------------------------------------------------------

StmtDDLNode *
SqlParserAux_buildDropRoutine ( ComRoutineType  drop_routine_type_tokens  // in
                              , QualifiedName * ddl_qualified_name_of_udf // in - deep copy
                              , NABoolean       optional_cleanup          // in
                              , ComDropBehavior optional_drop_behavior    // in
                              , NABoolean       optional_validate         // in
                              , NAString      * optional_logfile          // in - deep copy
                              , NABoolean       optional_if_exists        // in
                              )
{
  // If CLEANUP, VALIDATE, or LOG option specified,
  // ALLOW_SPECIALTABLETYPE must also be specified
  if ( (optional_cleanup  OR  optional_validate  OR  optional_logfile)  AND
       NOT Get_SqlParser_Flags(ALLOW_SPECIALTABLETYPE) )
  {
    return NULL; // Error: internal syntax only!
  }

  if (drop_routine_type_tokens EQU COM_ACTION_UDF_TYPE)
  {
    return NULL; // Error: illegal syntax!
  }

  QualifiedName noRoutineActionName(PARSERHEAP());

  NAString *pLogFile = NULL;
  if (optional_logfile NEQ NULL) // logfile specified
  {
    pLogFile = new (PARSERHEAP()) NAString ( optional_logfile->data()
                                           , PARSERHEAP()
                                           );
  }

  switch (drop_routine_type_tokens)
  {
  case COM_PROCEDURE_TYPE:
  case COM_TABLE_UDF_TYPE:
    ddl_qualified_name_of_udf->setObjectNameSpace(COM_TABLE_NAME);
    break;
  case COM_ACTION_UDF_TYPE:
    ComASSERT(FALSE);
    break;
  case COM_UNKNOWN_ROUTINE_TYPE: // either scalar or universal
  default:
    ddl_qualified_name_of_udf->setObjectNameSpace(COM_UDF_NAME);
    break;
  } // switch

  StmtDDLNode *pNode99 = new (PARSERHEAP()) StmtDDLDropRoutine
    ( drop_routine_type_tokens       // in - ComRoutineType
    , *ddl_qualified_name_of_udf     // in - QualifiedName & - deep copy
    , noRoutineActionName            // in - QualifiedName & - deep copy
    , optional_drop_behavior         // in - ComDropBehavior
    , optional_cleanup               // in - NABoolean       - for CLEANUP  mode set to TRUE
    , optional_validate              // in - NABoolean       - for VALIDATE mode set to FALSE (?)
    , pLogFile                       // in - NAString *      - shallow copy
    , PARSERHEAP()
    );

  pNode99->castToStmtDDLDropRoutine()->setDropIfExists(optional_if_exists);

  // Do not delete pLogFile because we did a shallow copy
  return pNode99;
}

// -----------------------------------------------------------------------
// SqlParserAux_buildAlterPassThroughParamDef
// -----------------------------------------------------------------------

ElemDDLNode *
SqlParserAux_buildAlterPassThroughParamDef
  ( UInt32                   passthrough_param_position      // in
  , ElemDDLNode *                  passthrough_input_value         // in - shallow copy
  , ComRoutinePassThroughInputType optional_passthrough_input_type // in
  )
{
  ElemDDLPassThroughParamDef *pNode3 =
    passthrough_input_value->castToElemDDLPassThroughParamDef();
  pNode3->setParamPosition(passthrough_param_position);
  pNode3->setPassThroughInputType(optional_passthrough_input_type);
  pNode3->setPassThroughParamDefKind(ElemDDLPassThroughParamDef::eALTER_PASS_THROUGH_INPUT);
  return pNode3;
}

// -----------------------------------------------------------------------
// SqlParserAux_buildDescribeForFunctionAndAction
// -----------------------------------------------------------------------

RelExpr *
SqlParserAux_buildDescribeForFunctionAndAction
  ( CorrName * actual_routine_name_of_udf_or_uudf  // in - deep copy
  , CorrName * optional_showddl_action_name_clause // in - deep copy
  , Lng32 optional_showddlroutine_options           // in
  )
{
  actual_routine_name_of_udf_or_uudf->getQualifiedNameObj().setObjectNameSpace(COM_UDF_NAME);
  Describe * pDescribe = NULL;
  if (optional_showddl_action_name_clause NEQ NULL) // routine action name
  {
    optional_showddl_action_name_clause->getQualifiedNameObj().setObjectNameSpace(COM_UUDF_ACTION_NAME);
    pDescribe = new (PARSERHEAP())
      Describe ( SQLTEXT()
               , *optional_showddl_action_name_clause // in - const CorrName & - deep copy
               , Describe::SHOWDDL_
               , COM_UUDF_ACTION_NAME                 // in - ComAnsiNameSpace labelAnsiNameSpace_
               , optional_showddlroutine_options      // in - long optional_showddlroutine_options
               );
    pDescribe->setUudfQualName(actual_routine_name_of_udf_or_uudf->getQualifiedNameObj()); // deep copy
  }
  else // function name
  {
    pDescribe = new (PARSERHEAP())
      Describe ( SQLTEXT()
               , *actual_routine_name_of_udf_or_uudf // in - const CorrName & - deep copy
               , Describe::SHOWDDL_
               , COM_UDF_NAME                        // in - ComAnsiNameSpace labelAnsiNameSpace_
               , optional_showddlroutine_options     // in - long optional_showddlroutine_options
               );
  }
  return new (PARSERHEAP()) RelRoot ( pDescribe
                                    , REL_ROOT
                                    , new (PARSERHEAP()) ColReference ( new (PARSERHEAP())
                                                                        ColRefName ( TRUE
                                                                                   , PARSERHEAP()
                                                                                   )
                                                                      )
                                    );
}

// ----------------------------------------------------------------------------
// method:: TableTokens::setTableTokens
//
// Method that sets appropriate values in the createTableNode parser tree
// based on options described in this class.
//
// in:  StmtDDLCreateTable *pNode - pointer to the create table parse tress
//
// ----------------------------------------------------------------------------
void
TableTokens::setTableTokens(StmtDDLCreateTable *pNode)
{
  pNode->setCreateIfNotExists(ifNotExistsSet());

  switch (type_)
  {
    case TableTokens::TYPE_REGULAR_TABLE:
    case TableTokens::TYPE_GHOST_TABLE:
      break;

    case TableTokens::TYPE_EXTERNAL_TABLE:
      pNode->setIsExternal(TRUE);
      break;

    case TableTokens::TYPE_IMPLICIT_EXTERNAL_TABLE:
      pNode->setIsExternal(TRUE);
      pNode->setIsImplicitExternal(TRUE);
      break;

    case TableTokens::TYPE_SET_TABLE:
      pNode->setInsertMode(COM_SET_TABLE_INSERT_MODE);
      break;

    case TableTokens::TYPE_MULTISET_TABLE:
      pNode->setInsertMode(COM_MULTISET_TABLE_INSERT_MODE);
      break;

    case TableTokens::TYPE_VOLATILE_TABLE:
      pNode->setIsVolatile(TRUE);
      pNode->setProcessAsExeUtil(TRUE);
      break;

    case TableTokens::TYPE_VOLATILE_TABLE_MODE_SPECIAL1:
    case TableTokens::TYPE_VOLATILE_SET_TABLE:
      pNode->setIsVolatile(TRUE);
      pNode->setProcessAsExeUtil(TRUE);
      pNode->setInsertMode(COM_SET_TABLE_INSERT_MODE);
      break;

    case TableTokens::TYPE_VOLATILE_MULTISET_TABLE:
      pNode->setProcessAsExeUtil(TRUE);
      pNode->setInsertMode(COM_MULTISET_TABLE_INSERT_MODE);
      break;

    default:
      NAAbort("TableTokens - TypeAttr", __LINE__, "internal logic error");
      break;
  }

  switch (options_)
  {
    case TableTokens::OPT_NONE:
      break;

    case TableTokens::OPT_LOAD:
      pNode->setLoadIfExists(TRUE);
      break;

    case TableTokens::OPT_NO_LOAD:
      pNode->setNoLoad(TRUE);
      break;

    case TableTokens::OPT_IN_MEM:
      pNode->setNoLoad(TRUE);
      pNode->setInMemoryObjectDefn(TRUE);
      break;

    case TableTokens::OPT_LOAD_WITH_DELETE:
      pNode->setLoadIfExists(TRUE);
      pNode->setDeleteData(TRUE);
      break;

    default:
      NAAbort("TableTokens - LoadAttr", __LINE__, "internal logic error");
      break;
  }
}

//
// End of File
//
