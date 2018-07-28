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
#ifndef SQLPARSERAUX_H
#define SQLPARSERAUX_H
/* -*++-*- 
******************************************************************************
*
* File:         SqlParserAux.cpp
* Description:  SQL Parser auxiliary methods. Extracted from sqlparser.y to
*               work around a c89 (v2.1) internal limit that shows up as
                ugen: internal: Assertion failure
                in source file 'W:\parser\SqlParser.cpp', at source line 22210
                detected in back end file 'eval.c', at line 4653
*
* Created:      4/28/94
* Language:     C++
*
*
*	Is there a parser, much bemus'd in beer,
*	A maudlin poetess, a rhyming peer,
*	A clerk, foredoom'd his father's soul to cross,
*	Who pens a stanza, when he should engross?
*		-- with my apologies to Alexander Pope
*
******************************************************************************
*/

#include <sstream>
#include "Platform.h"				// must be the first #include

#include "AllRelExpr.h"
#include "DatetimeType.h"
#include "HvTypes.h"
#include "IntervalType.h"
#include "LateBindInfo.h"
#include "ComVersionDefs.h"
#include "ParScannedTokenQueue.h"
#include "StmtDDLCreateTable.h"

class NAWString;


class PairOfUnsigned : public NABasicObject {
public:
  PairOfUnsigned(UInt32 lArg, UInt32 rArg) : left_(lArg), right_(rArg) {}
  UInt32 left  () const { return left_;  }
  UInt32 right () const { return right_; }
private:
  UInt32 left_,right_;
};

// this struct is used to pass multiple pointers from one parser
// production to another.
// Currently set to max 5 ptrs, increase size if needed.
class PtrPlaceHolder {
public:
  PtrPlaceHolder()
  {}

  PtrPlaceHolder(void * ptr1, void * ptr2=NULL, void * ptr3=NULL, 
		 void * ptr4=NULL, void * ptr5=NULL, void * ptr6=NULL)
       : ptr1_(ptr1), ptr2_(ptr2), ptr3_(ptr3), ptr4_(ptr4), ptr5_(ptr5),
	 ptr6_(ptr6)
  {}
    
  void * ptr1_;
  void * ptr2_;
  void * ptr3_;
  void * ptr4_;
  void * ptr5_;
  void * ptr6_;
};

class OldNewNames : public NABasicObject {
public:
  OldNewNames(NAString *theOld, NAString *theNew) :
      oldName_(theOld), newName_(theNew)	
    {}
  // no special dtor; the old/new strings are deleted by the
  // dtor of StmtDDLCreateTrigger
  NAString * oldName () const { return oldName_;  }
  NAString * newName () const { return newName_; }

private:
  NAString * oldName_, * newName_;
};

// prepare "nameList" with old/new transition names
//static void prepareReferencingNames(TableRefList & nameList,
				 //  StmtDDLCreateTrigger * triggerObject);

/*  This enum is used only in the sqlparser.y file to collect and
** validate consistant use of trigger scoping
*/
enum ParTriggerScopeType {
    ParTRIG_SCOPE_NONE,
    ParTRIG_SCOPE_ROW,
    ParTRIG_SCOPE_TABLE
  };


// If we need to reuse this class, STACK_LIMIT should become a variable
// (a data member passed at constructor time), or the internal stack should
// reallocate/resize as needed, copying the old stack to the new, and deleting
// the old.  Also could be template'ized to be a stack of more than just int's.
//
// For the one use to which this is being put (error 4101 stuff), a hard limit
// (that is, too many pushes LOSE data!) is acceptable.
class LimitedStack
{
  enum { STACK_LIMIT = 10, EMPTY_VALUE = 0 };
  Int32 stack_[STACK_LIMIT];
  Int32 index_;
public:
  void reset()		{ index_ = -1; }
  Int32  entries() const	{ return index_ + 1; }
  Int32  isEmpty() const	{ return index_ < 0; }
  Int32  push(Int32 i)	{ if (++index_ >= STACK_LIMIT) return index_--;
  			  stack_[index_] = i; return 0;	  // return 0: success
			}
  Int32  pop(Int32 &i)	{ if (isEmpty()) { i = EMPTY_VALUE; return -1; }
  			  i = stack_[index_--]; return 0; // return 0: success
			}
  void pop()		{ if (!isEmpty()) index_--; }
  Int32  top(Int32 &i)	{ if (isEmpty()) { i = EMPTY_VALUE; return -1; }
  			  i = stack_[index_]; return 0;   // return 0: success
			}
  Int32  operator()()	{ if (isEmpty()) return EMPTY_VALUE;
			  return stack_[index_];
			}
};

const Int32 STACKDELTA_ENSURES_NONZERO = 100;
extern THREAD_P LimitedStack *inJoinSpec;	// can handle <STACK_LIMIT> nested Joins

char    *SQLTEXT();
charBuf *SQLTEXTCHARBUF();
Lng32    SQLTEXTCHARSET();
NAWchar *SQLTEXTW(); // in UCS-2/UTF-16
NAWcharBuf *SQLTEXTNAWCHARBUF();

CharInfo::CharSet getStringCharSet(NAString **p);
CharInfo::CharSet getStringCharSet(NAWString **p);

NABoolean charsetMismatchError(NAString **d1, NAString **d2);

inline NABoolean charsetMismatchError(NAWString **d1, NAWString **d2)
{ return charsetMismatchError((NAString **)d1, (NAString **)d2); }
inline NABoolean charsetMismatchError(NAString **d1, NAWString **d2)
{ return charsetMismatchError((NAString **)d1, (NAString **)d2); }
inline NABoolean charsetMismatchError(NAWString **d1, NAString **d2)
{ return charsetMismatchError((NAString **)d1, (NAString **)d2); }

// First emit syntax error 15001.
// Then if we're not in a parenthesized join-spec, also emit error 4101,
// 	"If $0~String0 is intended to be a further table reference in the
//	FROM clause, the preceding join search condition must be enclosed
//	in parentheses."
void checkError4101(NAString *badItemStr, ItemExpr *badPrevExpr = NULL);

// Emit warning 3169 if unknown collation,
// but emit it only once per stmt for each new unknown collation name.
NABoolean maybeEmitWarning3169(CharInfo::Collation co, const NAString &nam);

NABoolean checkError3179(const NAType *na);

typedef LIST(ItemExpr *) AllHostVarsT;
typedef LIST(ItemExpr *) AssignmentHostVarsT;
// extern LIST(ItemExpr *) AllHostVars;
// extern LIST(ItemExpr *) AssignmentHostVars;
extern THREAD_P AllHostVarsT *AllHostVars;
extern THREAD_P AssignmentHostVarsT *AssignmentHostVars;
extern THREAD_P HVArgTypeLookup *TheProcArgTypes;
extern THREAD_P NABoolean        intoClause;
extern THREAD_P NABoolean        InAssignmentSt;
extern THREAD_P NABoolean        ThereAreAssignments;

void resetHostVars();


extern THREAD_P Int32 in3GL_;

void MarkInteriorNodesAsInCompoundStmt(RelExpr *node);
NAWString* localeMBStringToUnicode(NAString* localeString, Lng32 charset, CollHeap * heap = NULL);
RelRoot *finalize(RelExpr *top, NABoolean outputVarCntValid = TRUE);

NAString * getSqlStmtStr(CharInfo::CharSet &sqlStmtCharSet, CollHeap * heap);

class ForUpdateSpec : public NABasicObject {
public:
  ForUpdateSpec(NABoolean e, NABoolean u = FALSE, ItemExpr *col = NULL)
    : explicitSpec_(e), forUpdate_(u), updateCol_(col) {}
  void finalizeUpdatability(RelExpr *top);
  NABoolean explicitSpec() { return explicitSpec_; }

private:
  NABoolean explicitSpec_;	// TRUE if FOR UPDATE or FOR READ appeared
  NABoolean forUpdate_;		// TRUE if FOR UPDATE; FALSE if FOR READ ONLY
  ItemExpr *updateCol_;		// NULL unless "FOR UPDATE OF col [, col]..."
};


NABoolean finalizeAccessOptions(RelExpr *top,
                                TransMode::AccessType at = TransMode::ACCESS_TYPE_NOT_SPECIFIED_,
                                LockMode   lm = LOCK_MODE_NOT_SPECIFIED_);

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

HostVar *makeHostVar(NAString *hvName, NAString *indName,
		     NABoolean isDynamic = FALSE);

// In the next several procedures, we set up ConstValue's, in whose text
// we want negative numbers to end up with minus signs but positive numbers
// without plus signs.  That is,
//	"1",'+'	-> "1"
//	"2",'-'	-> "-2"
// (Only digits, no signs, are passed in the strptr and cvtstr arguments.)
// (Note that we delete the strptr before returning:  thus we may freely
// modify the text therein.)

ItemExpr *literalOfNumericPassingScale(NAString *strptr, char sign,
                                       NAString *cvtstr, size_t scale);

inline ItemExpr *literalOfNumericNoScale(NAString *strptr, char sign = '+')
{ return literalOfNumericPassingScale(strptr, sign, strptr, 0); }

ItemExpr *literalOfNumericWithScale(NAString *strptr, char sign = '+');

NABoolean literalToNumeric(NAString *strptr, double& val, char sign = '+');

NABoolean literalToDouble(NAString *strptr, double& val, 
                          NABoolean& floatP, char sign = '+');

ItemExpr *literalOfApproxNumeric(NAString *strptr, char sign = '+');

ItemExpr *literalOfInterval(NAString *strptr,
                            IntervalQualifier *qualifier,
                            char sign = '+');

ItemExpr *literalOfDate(NAString *strptr, NABoolean noDealloc = FALSE);

ItemExpr *literalOfTime(NAString *strptr);

ItemExpr *literalOfTimestamp(NAString *strptr);

// ***********************************************************************
//
//  DatetimeQualifier : A syntactic-sugar class used by the Parser only
//
// ***********************************************************************
class DatetimeQualifier : public NABasicObject
{
public:

  // Constructors
  DatetimeQualifier
  ( rec_datetime_field startField
  , UInt32 fractionPrec = 0
  ) :
  startField_(startField)
 ,fractionPrec_(fractionPrec)
 ,endField_ (startField) 
  {}

  DatetimeQualifier
  ( rec_datetime_field startField
  , rec_datetime_field endField
  , UInt32 fractionPrec = 0
   ) :
  startField_(startField)
 ,endField_ (endField) 
 ,fractionPrec_(fractionPrec)
  {}

 rec_datetime_field getStartField() { return startField_; }
 rec_datetime_field getEndField()   { return endField_; }
 UInt32 getFractionPrecision()    { return fractionPrec_; }

private:

 rec_datetime_field startField_;
 rec_datetime_field endField_ ;
 UInt32 fractionPrec_;
}; // class DatetimeQualifier

ItemExpr *literalOfDateTime(NAString *strptr, DatetimeQualifier *qualifier);

enum ParCaseIdentifierClauseType { ParCALL_CASED_IDENTIFIER_CLAUSE,
                                   ParGOTO_CASED_IDENTIFIER_CLAUSE,
                                   ParPERFORM_CASED_IDENTIFIER_CLAUSE };

void parseCasedIdentifier(ParCaseIdentifierClauseType clauseType,
                          NAString *pClauseBuffer,
                          NAString &casedIdentifier);


// Purpose: This class stores what essentially is a sequence of
// one, two, three, or more NAStrings.  Actually, just NAString*
// are stored.  Also, only up to a maximum of four strings are actually
// kept.  This has to do with the fact that this class will represent
// two and three part ANSI names and column references.
// So the code which uses this class
// will complain if the number of strings in the sequence exceeds three,
// or four, depending on where the use is.
// This class checks if more than four strings
// have been appended (although appending the ith string, for i>4,
// causes this class to delete the given NAString*!).
// Extracting the ith string (for i<=4, and note that this ith thing
// is ahead by one, since the indices begin at zero) causes this
// class to no longer manage the storage of the extracted string.
// The destructor will free any "unused" NAStrings.
// The internal data for this class shall be an unsigned to keep track
// of the number of strings appended to the sequence.  There is an
// array of four elements, each an NAString* to hold the pointers to
// the elements of this sequence.

class ShortStringSequence : public NABasicObject {
public:
  ShortStringSequence();
  ShortStringSequence(NAString*,
                      unsigned short toInternalIdentifierFlags = NASTRING_ALLOW_NSK_GUARDIAN_NAME_FORMAT);
  ~ShortStringSequence();
  enum { MAX_SIMPLENAME_PARTS = 1,
  	 MAX_QUALIFIEDNAME_PARTS = 3,
         MAX_COLREFERENCE_PARTS = 4,
	 MAX_NUM_PARTS = MAX_COLREFERENCE_PARTS };

  void append(NAString*);
  NAString* extract(UInt32);

  // returns position of first character in name
  const StringPos getPosition() const	  { return seqPos_; }
  // returns position of last character in name
  const StringPos getEndPosition() const  { return seqEndPos_; }
  // returns number of characters in name
  const size_t getNameLength() const	  { return seqEndPos_ - seqPos_ + 1; }

  // returns the number of parts in the sequence, that is, number of appends
  // that have been done plus the number of elements introduced by
  // the constructor (either zero or one)
  UInt32 numParts() const		  { return numParts_; }

  NABoolean isValid() const		  { return numParts_ <= MAX_NUM_PARTS; }

  void invalidate();

private:
  const ParScannedTokenQueue::scannedTokenInfo& getTokInfo(NAString*);

  NAString   *seq[MAX_NUM_PARTS];
  size_t     seqPos_;
  size_t     seqPosStartOffset_;
  size_t     seqEndPos_;
  UInt32   numParts_;
public:
  UInt16 toInternalIdentifierFlags_;
};


// For error messages
NAWString badNameFromStrings(ShortStringSequence *names);

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

QualifiedName * qualifiedNameFromStrings(ShortStringSequence *names);

SchemaName * schemaNameFromStrings(ShortStringSequence *names);

CorrName * corrNameFromStrings(ShortStringSequence *names);

ColRefName *colRefNameFromStrings(ShortStringSequence *names);

short preprocessHiveDDL(const NAString &catalogName, 
                        Parser::HiveDDLInfo *hiveDDLInfo);

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
                              Int32 upCase = TRUE,
                              NABoolean acceptCircumflex = FALSE // VO: Fix genesis solution 10-040204-2957
                             ,UInt16 toInternalIdentifierFlags = NASTRING_ALLOW_NSK_GUARDIAN_NAME_FORMAT // same as pv_flags in ToInternalIdentifier()
                             );


// Class PicStream
// ===============
// This class provides useful operations for parsing cobol PIC clauses.
// The intention is for a client function to employ this class and its
// members (as well as inherited streambuf operations) to parse TOK_PICTURE
// tokens.

// PicStream(char*) initializes this streambuff to be the
//       buffer consisting of the given char buffer.  No putting shall
//       be allowed.
// skipWhite() advances the get pointer over zero or more chars
//       of input until either EOF has been achieved or a non-white character
//       is the (new) current character.  This is a void function.
// skipCount(char) returns an unsigned integer telling the
//       count gleaned from a pattern such as `XX(3)' (four), or `999999'
//       six.  The char parameter tells what kind of character (X or 9)
//       is the `pattern.'  This function assertion fails if
//       upon entry the current character does not match the given argument.
// skipPicture() expects the next input to be either `PIC'
//       or `PICTURE'.  Case doesn't matter.  This routine reads
//       over that token and any following white space. It is an assertion
//       failure (and SqlLexer problem) if `PIC' or `PICTURE' is not found.

class PicStream : public stringbuf {
public:
          PicStream         (char* buffer);
 void     skipWhite         ();
  // return TRUE if successful, return FALSE if overflow
 NABoolean skipCount         (UInt32 *result, const char pattern, NABoolean isCharType = FALSE);
 void     skipPicture       ();
 void 
   mystossc() 
 {
   if (this->gptr() < this->egptr()) 
     this->gbump(1);
   else 
     this->uflow();
 }
};

inline
PicStream::PicStream (char *buffer) : stringbuf(buffer)
{}

inline
void PicStream::skipWhite()
{
  while( sgetc() != EOF && !isgraph(sgetc())) stossc();
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
			 UInt32 *precisionPtr, UInt32 *scalePtr,
			 NABoolean *hasSignPtr);


// DISPLAY_STYLE is used in the cobol pic_type related grammar/code.
enum DISPLAY_STYLE { STYLE_DISPLAY, STYLE_LEADING_SIGN, STYLE_UPSHIFT,
                     STYLE_COMP };

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
		  const NABoolean      isCaseinsensitive);


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
ItemExpr *convertINvaluesToOR(ItemExpr *lhs, ItemExpr *rhs);


// quantified_predicate : value_expression_list '=' quantifier rel_subquery
ItemExpr *makeQuantifiedComp(ItemExpr *lhs,
                             OperatorTypeEnum compOpType,
                             Int32 quantifierTok,
                             RelExpr *subquery);

ItemExpr *makeBetween(ItemExpr *x, ItemExpr *y, ItemExpr *z, Int32 tok);
// Triggers
// static NABoolean isInvalidSignalSqlstateValue(NAString *SqlState);


// Change the <sqltext> arg of a CQD from
//	SET SCHEMA X.Y;		-- unquoted: Tandem syntax extension
// into
//	SET SCHEMA 'X.Y';	-- string literal: Ansi syntax, MX canonical fmt
//
// This needs to be called ONLY for:
// - SET cqd's (not DECLARE cqd's), and
//   - the SET cqd's unquoted (non-string-literal) variants, or
//   - or if we are otherwise rewriting the user input text
//
ControlQueryDefault *normalizeDynamicCQD(const char *attrName,
                                         const NAString &attrValue);

// return the relexpr tree that evaluates an empty compound statement
RelExpr* getEmptyCSRelTree();

// return the relexpr tree that evaluates a IF statement. Empty 
// branches are removed. If both branches are empty, a NULL is returned. 
// Note the condition argument will be deleted if both theBranch and
// elseBranch are NULL.
RelExpr*
getIfRelExpr(ItemExpr* condition, RelExpr* thenBranch, RelExpr* elseBranch);

// EJF L4J - dynamic CQD not allowed inside Compound Statements
NABoolean beginsWith(char *sqltext, const char *kwd);

// Used to set variable index for Host Vars and Dynamic params,
// Currently used by CALL <procedure> statement
void setHVorDPvarIndex ( ItemExpr *, NAString * );

//ct-bug-10-030102-3803 Begin
void conditionalDelimit(NAString &, const NAString &);
//ct-bug-10-030102-3803 End

// Get the default max column width from the defaults table for LONG VARCHAR/WVARCHAR.
// LONG_MAX is returned if the parser is invoked by the preprocessor. This is 
// because the value that can be obtained on NT in the context of ETK may be different
// from that in the defaults table on NSK.
Lng32 getDefaultMaxLengthForLongVarChar(CharInfo::CharSet cs);

// Get the default min column width from the defaults table for LONG VARCHAR/WVARCHAR.
// 0 is returned if the parser is invoked by the preprocessor. This is 
// because the value that can be obtained on NT in the context of EKT may be different
// from that in the defaults table on NSK.
Lng32 getDefaultMinLengthForLongVarChar(CharInfo::CharSet cs);

// Get the charset inference setting from the defaults table. The defVal's content 
// is updated with the INFER_CHARSET entry in the table if the parser is not 
// invoked by the preprocessor and the  INFER_CHARSET CQD is turned on.
// If the parser is invoked by the preprocessor, the function returns FALSE 
// and defval is not changed.
NABoolean getCharSetInferenceSetting(NAString& defval);

// Purpose: Contains the specified length of the character-related
// data type and its unit (e.g., CHARACTERS or BYTES).
class ParAuxCharLenSpec : public NABasicObject
{
public:
  enum ECharLenUnit { eCHAR_LEN_UNIT_NOT_SPECIFIED, eCHARACTERS, eBYTES };
  ParAuxCharLenSpec(UInt32 len, ECharLenUnit eUnit) : uiCharLen_(len) , eCharLenUnit_(eUnit) { }
  virtual ~ParAuxCharLenSpec() { }
  UInt32 getCharLen() const { return uiCharLen_; }
  ECharLenUnit getCharLenUnit() const { return eCharLenUnit_; }
  void setCharLenUnit(ECharLenUnit u) {eCharLenUnit_ = u;}

  NABoolean isCharLenUnitSpecified() const { return(eCharLenUnit_ NEQ eCHAR_LEN_UNIT_NOT_SPECIFIED); }
private:
  ParAuxCharLenSpec(); // DO NOT USE
  UInt32 uiCharLen_;
  ECharLenUnit eCharLenUnit_;
};

void emitError3435( int, int, ParAuxCharLenSpec::ECharLenUnit );

// return TRUE iff user wants to allow RAND function
NABoolean allowRandFunction();

RelExpr * getTableExpressionRelExpr(
     RelExpr *fromClause,
     ItemExpr *whereClause,
     RelExpr *sampleClause,
     RelExpr *transClause,
     ItemExpr *seqByClause,
     ItemExpr *groupByClause,
     ItemExpr *havingClause,
     ItemExpr *qualifyClause = NULL,
     NABoolean hasTDFunctions = FALSE,
     NABoolean hasOlapFunctions = FALSE);



ItemExpr *processINlist(ItemExpr *lhs, ItemExpr *rhs);

QualifiedName * processVolatileDDLName(QualifiedName * inName,
				       NABoolean validateVolatileName,
				       NABoolean updateVolatileName);
const NABoolean validateVolatileSchemaName(NAString &schName);
SchemaName * processVolatileSchemaName(SchemaName *schName,
				       NABoolean validateVolatileName,
				       NABoolean updateVolatileName);

RelExpr * processReturningClause(RelExpr * insert, 
				 UInt32 returningType);

// Process the ascii characters for an Int64.
// Ensure that the number is not larger than
// the maximum allowed for an Int64
NABoolean validateSGOption(NABoolean positive,
			   NABoolean negAllowed,
			   char * value,
			   const char * optionName,
			   const char * objectType);

/*
// INSERT2000 COLUMN FIX STARTS HERE
// class RearrangeValueExprList is only applicable for INSERT statements.
// Objects of this class would temporarily store values in ParserHeap
// A double link list of objects is created with "tail" acting as the
// last link in the chain.
// Example: INSERT INTO TABLE T (A,B,C,D,E) VALUES (1,2,3,4,5);
// The first node would contain ItemExpr value corresponding to "1"
// and the last node would correspond to "5". This is happening due to the 
// left-reucrssion nature of the grammar.
// Once all the elements are collected it is now sent to ItemList
// The order of call would be:
//	ItemList (4,5);
//	ItemList (3,NULL);
//	ItemList (2,NULL);
//	ItemList (1,NULL);
	
class RearrangeValueExprList 
{
   public:
	static RearrangeValueExprList *tail;
    ItemExpr *value;
	
	RearrangeValueExprList *next;
	RearrangeValueExprList *prev;
	
	ItemExpr* Store_ValueExprList(ItemExpr *i, ItemExpr *j);	
    ItemExpr* Return_ValueExprList();

	RearrangeValueExprList();		
	~RearrangeValueExprList();		
};

// class MultiValueExprList is only applicable for INSERT statements.
// The objects from this class is only used when a single INSERT statement
// is having multiple rows inserted.
// Consider the following example:
// INSERT INTO TABLE T (A,B,C,D,E) VALUES (1,2,3,4,5), (6,7,8,9,0);
// After the first row is received it has to be sent to ItemList which in turn 
// return a ItemExpr pointer which is later on required in the grammar.
// This returned pointer is stored before passing on the second row to
// ItemExpr. This helps in preserving the order of rows being inserted.

class MultiValueExprList 
{
   public:
	static MultiValueExprList *tail;
    ItemExpr *value;
	
	MultiValueExprList *next;
	MultiValueExprList *prev;
	
	void Store_MultiValueExprList(ItemExpr *i);	
    ItemExpr* Return_MultiValueExprList();

	MultiValueExprList();		
	~MultiValueExprList();		
};

// INSERT2000 COLUMN FIX ENDS HERE
// */

// ----------------------------------------------------------------------------
// Class: TableTokens
//
// This is a helper class that gathers tokens from different create table
// productions to make setting values in the create table parser node more
// uniform.
//
// Class contains three members:
//   type_ - describe the type of table as specified by the create table
//           start tokens
//   options_ - describe how data is loaded for create table statements
//           that load data
//   ifNotExistsSet - A table token that tells the create code to ignore
//           already exists errors.
// ----------------------------------------------------------------------------           
class TableTokens : public NABasicObject
{
public:
  // Type of tables available from create table start tokens
  enum TableType
    { TYPE_REGULAR_TABLE = 0,
      TYPE_EXTERNAL_TABLE,
      TYPE_IMPLICIT_EXTERNAL_TABLE,
      TYPE_SET_TABLE,
      TYPE_MULTISET_TABLE,
      TYPE_VOLATILE_TABLE,
      TYPE_VOLATILE_TABLE_MODE_SPECIAL1,
      TYPE_VOLATILE_SET_TABLE,
      TYPE_VOLATILE_MULTISET_TABLE,
      TYPE_GHOST_TABLE,
    };

  // load/in memory options
  enum TableOptions
    { OPT_NONE,
      OPT_LOAD,
      OPT_NO_LOAD,
      OPT_IN_MEM,
      OPT_LOAD_WITH_DELETE
    };

  TableTokens(TableType type, NABoolean ifNotExistsSet)
  : type_(type),
    options_ (OPT_NONE),
    ifNotExistsSet_(ifNotExistsSet)
  {}

  TableType getType() { return type_; }
  void setType(TableType t) { type_ = t; }

  NABoolean isVolatile()
   { return (type_ == TYPE_VOLATILE_TABLE ||
             type_ == TYPE_VOLATILE_TABLE_MODE_SPECIAL1 ||
             type_ == TYPE_VOLATILE_SET_TABLE ||
             type_ == TYPE_VOLATILE_MULTISET_TABLE); }
  TableOptions getOptions() { return options_; }
  NABoolean ifNotExistsSet() { return ifNotExistsSet_; }

  void setOptions( TableOptions load) { options_ = load; }
  void setTableTokens(StmtDDLCreateTable *pNode);

private:
  TableType    type_;
  TableOptions options_;
  NABoolean    ifNotExistsSet_;
};

// -----------------------------------------------------------------------
// Declarations of global functions
// -----------------------------------------------------------------------

ItemExpr *buildUdfExpr(NAString *udfName,
                       NAString *fixedInput,
                       ItemExpr *valueList);

ElemDDLNode *
SqlParserAux_buildUdfOptimizationHint ( Int32       tokvalStage     // in
                                      , Int32       tokvalResource  // in
                                      , ComSInt32 cost            // in
                                      );

StmtDDLNode *
SqlParserAux_buildAlterFunction ( QualifiedName * ddl_qualified_name                       // in - deep copy
                                , ElemDDLNode   * optional_alter_passthrough_inputs_clause // in
                                , ElemDDLNode   * optional_add_passthrough_inputs_clause   // in
                                , ElemDDLNode   * optional_create_function_attribute_list  // in
                                );

StmtDDLNode *
SqlParserAux_buildAlterAction ( QualifiedName * ddl_qualified_name_of_uudf               // in - deep copy
                              , QualifiedName * ddl_qualified_name_of_action             // in - deep copy
                              , ElemDDLNode   * optional_alter_passthrough_inputs_clause // in
                              , ElemDDLNode   * optional_add_passthrough_inputs_clause   // in
                              , ElemDDLNode   * optional_create_function_attribute_list  // in
                              );

StmtDDLNode *
SqlParserAux_buildAlterTableMappingFunction
  ( QualifiedName * ddl_qualified_name_of_table_mapping_udf  // in - deep copy
  , ElemDDLNode   * optional_alter_passthrough_inputs_clause // in
  , ElemDDLNode   * optional_add_passthrough_inputs_clause   // in
  , ElemDDLNode   * optional_create_function_attribute_list  // in
  );

StmtDDLNode *
SqlParserAux_buildDropAction ( QualifiedName * ddl_qualified_name_of_uudf   // in - deep copy
                             , QualifiedName * ddl_qualified_name_of_action // in - deep copy
                             , NABoolean       optional_cleanup             // in
                             , ComDropBehavior optional_drop_behavior       // in
                             , NABoolean       optional_validate            // in
                             , NAString      * optional_logfile             // in - deep copy
                             );

StmtDDLNode *
SqlParserAux_buildDropRoutine ( ComRoutineType  drop_routine_type_tokens  // in
                              , QualifiedName * ddl_qualified_name_of_udf // in - deep copy
                              , NABoolean       optional_cleanup          // in
                              , ComDropBehavior optional_drop_behavior    // in
                              , NABoolean       optional_validate         // in
                              , NAString      * optional_logfile          // in - deep copy
                              , NABoolean       optional_if_exists        // in
                              );

ElemDDLNode *
SqlParserAux_buildAlterPassThroughParamDef
  ( UInt32                   passthrough_param_position      // in
  , ElemDDLNode *                  passthrough_input_value         // in - shallow copy
  , ComRoutinePassThroughInputType optional_passthrough_input_type // in
  );

RelExpr *
SqlParserAux_buildDescribeForFunctionAndAction
  ( CorrName * actual_routine_name_of_udf_or_uudf  // in - deep copy
  , CorrName * optional_showddl_action_name_clause // in - deep copy
  , Lng32       optional_showddlroutine_options     // in
  );

#endif /* SQLPARSERAUX_H */
