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
#ifndef ITEMCOLREF_H
#define ITEMCOLREF_H
/* -*-C++-*-
******************************************************************************
*
* File:         ItemColRef.h
* Description:  Item expressions that reference columns/column ids/col groups
*               
* Created:      11/04/94
* Language:     C++
*
*
******************************************************************************
*/

#include "Platform.h"
#include <ctype.h>
#include <string.h>		// memcpy
#include "CharType.h"
#include "ItemExpr.h"
#include "Int64.h"
#include "nawstring.h"
#include "NAColumn.h"
#include "CostScalar.h"
#include "QRExprElement.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class BaseColumn;
class ColReference;
class ConstValue;
class DefaultSpecification;
class SelParameter;
class DynamicParam;
class RoutineParam;
class HostVar;
class IndexColumn;
class SelIndex;
class RoutineDesc;

// -----------------------------------------------------------------------
// Forward declarations
// -----------------------------------------------------------------------
class ColumnDescList;
class NAColumn;
class NAFileSet;
class TableDesc;

typedef ClusteredBitmap PositionSet;

// -----------------------------------------------------------------------
// A base column is allocated for each column belonging to a base table
// or table-valued stored procedure that is referenced in the query.
// This node is allocated only by the binder and is associated with
// the ValueDesc.
// -----------------------------------------------------------------------

class BaseColumn : public ItemExpr
{

public:

  BaseColumn(TableDesc * tableDesc, Lng32 columnNumber) 
     : ItemExpr(ITM_BASECOLUMN), tableDesc_(tableDesc), colNumber_(columnNumber)
  {}

  BaseColumn(const BaseColumn &column);

  virtual ~BaseColumn();

  // get the degree of this node (it is a leaf).
  virtual Int32 getArity() const;

  // virtual method to fixup tree for code generation.
  virtual ItemExpr * preCodeGen(Generator *);

  // method to do code generation
  short codeGen(Generator*);

  TableDesc *getTableDesc() const 		{ return tableDesc_; }

  Lng32 getColNumber() const 			{ return colNumber_; }

  NAColumn *getNAColumn() const;

  const NAString& getColName() const;

  const NAType& getType() const;

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const ItemExpr & other) const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // can this base column be calculated from these values/group attributes
  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
			      const GroupAttributes& newRelExprAnchorGA,
	   	              ValueIdSet& referencedInputs,
			      ValueIdSet& coveredSubExpr,
			      ValueIdSet& unCoveredExpr) const;

  virtual OrderComparison sameOrder(ItemExpr *other,
				    NABoolean askOther = TRUE);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  const NAString getTextForQuery() const;

  // return set of keyColumns referenced in the Computed Column expression
  void getUnderlyingColumnsForCC(ValueIdSet &underlyingCols);

  // deal with equivalent (usually IndexColumn) item expressions
  void addEIC(ValueId eic) { equivalentIndexCols_ += eic; }
  const ValueIdSet &getEIC() const { return equivalentIndexCols_; }
  NABoolean isEIC(ValueId id) const
    { return (getValueId() == id) OR equivalentIndexCols_.contains(id); }

  ValueIdList *getClusteringKeyCols() const;

  // is this BaseColumn a primary or partitioning key and is val a single 
  // value or a constant that can be safely coerced to BaseColumn's type?
  NABoolean isKeyColumnValue(ItemExpr& val) const;

  // is val a constant that can be safely coerced to BaseColumn's type?
  NABoolean canSafelyCoerce(ItemExpr& val) const;

  // append an ascii-version of ItemExpr into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cachewa) const;

  virtual NABoolean calculateMinMaxUecs(ColStatDescList & histograms,
					       CostScalar & minUec,
					       CostScalar & maxUec);

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

  virtual NABoolean isAColumnReference( )
  { return TRUE; }
  
  virtual QR::ExprElement getQRExprElem() const;

  const ValueId &getComputedColumnExpr() { return computedColumnExpr_; }
  void setComputedColumnExpr(const ValueId &x) { computedColumnExpr_ = x; }

private:

  // the TableId for the table to which this column belongs
  TableDesc *  tableDesc_;
  
  // Column number within the table
  Lng32 colNumber_;

  // The index columns that deliver the same value as the base column
  ValueIdSet equivalentIndexCols_;

  // If this is a computed column, the expression used to compute it (NULL otherwise)
  ValueId computedColumnExpr_;
}; // class BaseColumn

// -----------------------------------------------------------------------
// An index column object is allocated each time when an index is used
// by a rule that transforms some base table access into an index scan.
// -----------------------------------------------------------------------
class IndexColumn : public ItemExpr
{

public:

  // constructor
  IndexColumn(const NAFileSet *indexPtr,
	      Lng32 indexColumnNumber,
	      const ValueId &colDefinition);

  virtual ~IndexColumn();

  // get the degree of this node (it is a leaf).
  virtual Int32 getArity() const;

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // virtual method to fixup tree for code generation.
  virtual ItemExpr * preCodeGen(Generator *);

  // method to do code generation
  short codeGen(Generator*);
  Lng32 getOffset() const;

  const NAType& getType() const; 

  NAColumn * getNAColumn() const;

  Lng32 getIndexColNumber() const        { return indexColNumber_; }
  ValueId getDefinition() const     	{ return indexColDefinition_; }

  // misc. support functions
  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const ItemExpr & other) const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);
  virtual ValueId mapAndRewrite(ValueIdMap &map,
				NABoolean mapDownwards = FALSE);

  // get a printable string that identifies the operator
  const NAString getText() const;
  virtual void unparse(NAString &result,
                       PhaseEnum phase = OPTIMIZER_PHASE,
		       UnparseFormatEnum form = USER_FORMAT,
		       TableDesc * tabId = NULL) const;

  virtual OrderComparison sameOrder(ItemExpr *other,
				    NABoolean askOther = TRUE);

  virtual NABoolean isAColumnReference( )
  { return TRUE; }

  const NAFileSet * getNAFileSet() { return index_; }
private:

  // a pointer back to the NAFileSet structure
  const NAFileSet *index_;

  // Column number within the index
  Lng32 indexColNumber_;

  // Definition of the index column (what's in it)
  ValueId indexColDefinition_;

};

// -----------------------------------------------------------------------
// A column reference points to a column in a relational node. Constants
// and column references are the leaf nodes of an item expression.
// The ValueDesc is allocated at the same time as the BaseColumn. So 
// the ValueId must be set for this expression by calling  setValueId()
// after the allocation is done.
// -----------------------------------------------------------------------

class ColReference : public ItemExpr
{

public:

  ColReference(ColRefName *colRefName) :
    ItemExpr(ITM_REFERENCE), colRefName_(colRefName), starExpansion_(NULL),
      parent_(NULL), targetColumnClass_(USER_COLUMN) {}

  // virtual destructor
  virtual ~ColReference() {}

  // get the degree of this node (it is a leaf).
  virtual Int32 getArity() const;

  // accessor functions
  const ColRefName& getColRefNameObj() const { return *colRefName_; }
        ColRefName& getColRefNameObj()       { return *colRefName_; }
  const CorrName& getCorrNameObj() const { return colRefName_->getCorrNameObj(); }
        CorrName& getCorrNameObj()       { return colRefName_->getCorrNameObj(); }

  // star expansion functions
  void setStarExpansion(const ColumnDescList *columnList)
  {
    starExpansion_ = columnList;
  }
  const ColumnDescList *getStarExpansion() 	{ return starExpansion_; }

  void setParent(RelExpr *parent) { parent_ = parent; }
  RelExpr * getParent() const     { return parent_;   }

  // MV --
  inline void setTargetColumnClass(ColumnClass colClass) { targetColumnClass_ = colClass; }
  inline ColumnClass getTargetColumnClass() const { return targetColumnClass_; }

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const ItemExpr & other) const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // get a printable string that identifies the operator
  const NAString getText() const;

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}
private:

  // the parser and binder work with textual column references
  ColRefName *colRefName_;

  // if the column name is '*' or 'table.*', the binder expands it
  const ColumnDescList * starExpansion_;

  // pointer to parent node
  RelExpr *parent_;

  // MV --
  // Force this column to be added to the RETDesc as a system column.
  ColumnClass targetColumnClass_;

}; // class ColReference

// -----------------------------------------------------------------------
// A constant item operator refers to a constant value.
// It is either an integer, a string value or a boolean constant
// -----------------------------------------------------------------------

class ConstValue : public ItemExpr
{
public:

  enum IsNullEnum { IS_NOT_NULL = FALSE, IS_NULL = TRUE, IS_NULL_WAS_DEFAULT };
  // constructor for an untyped NULL constant
  ConstValue();
       
  // constructor for a numeric constant
  ConstValue(Lng32 intval,NAMemory * outHeap = CmpCommon::statementHeap());

  // constructor for a string constant
  ConstValue(const NAString& strval,
           enum CharInfo::CharSet charSet=CharInfo::DefaultCharSet,
           enum CharInfo::Collation collation=CharInfo::DefaultCollation,
           enum CharInfo::Coercibility coercibility=CharInfo::COERCIBLE,
           NAMemory * outHeap = CmpCommon::statementHeap()
        );

  ConstValue(const NAString& strval,
           NABoolean isCaseInSensitive,
           enum CharInfo::CharSet charSet=CharInfo::DefaultCharSet,
           enum CharInfo::Collation collation=CharInfo::DefaultCollation,
           enum CharInfo::Coercibility coercibility=CharInfo::COERCIBLE,
           NAMemory * outHeap = CmpCommon::statementHeap()
        );

  // constructor for a wide (unicode) string constant
  ConstValue(const NAWString& strval,
           enum CharInfo::CharSet charSet=CharInfo::UNICODE,
           enum CharInfo::Collation collation=CharInfo::DefaultCollation,
           enum CharInfo::Coercibility coercibility=CharInfo::COERCIBLE,
           NAMemory * outHeap = CmpCommon::statementHeap(),
           enum CharInfo::CharSet strLitPrefixCharSet=CharInfo::UnknownCharSet
        );

  ConstValue(const NAWString& strval,
           NABoolean isCaseInSensitive,
           enum CharInfo::CharSet charSet=CharInfo::UNICODE,
           enum CharInfo::Collation collation=CharInfo::DefaultCollation,
           enum CharInfo::Coercibility coercibility=CharInfo::COERCIBLE,
           NAMemory * outHeap = CmpCommon::statementHeap(),
           enum CharInfo::CharSet strLitPrefixCharSet=CharInfo::UnknownCharSet
        );


// constructor for a string constant with unknown charset (both the
// single-byte and double-byte string values are known)
   ConstValue(
            NAString strval, NAWString wstrval,
            enum CharInfo::Collation collation=CharInfo::DefaultCollation,
            enum CharInfo::Coercibility coercibility=CharInfo::COERCIBLE,
            NAMemory * outHeap = CmpCommon::statementHeap()
         );
  
  // Supply a type, a buffer containing the packed value,
  // the size of the buffer and , optionally, the string
  // for the literal
  ConstValue(const NAType * type,
             void * value,
             Lng32 value_len, 
	     NAString * literal = NULL,
             NAMemory * outHeap = CmpCommon::statementHeap());

  /*soln:10-050710-9594 begin */
  /* Same as above constructor along with string constants */
  ConstValue(const NAType * type, void * value, Lng32 value_len, NAString *lstrval, NAWString *wstrval,
	     NAString * literal = NULL,  NAMemory * outHeap = CmpCommon::statementHeap(), IsNullEnum isNull = IS_NOT_NULL);
  /*soln:10-050710-9594 end */

  // constructor to create extremal values (with the option of allowing
  // or excluding NULL)
  ConstValue(const NAType * type, 
	     const NABoolean wantMinValue,
	     const NABoolean allowNull,
             NAMemory * outHeap = CmpCommon::statementHeap());

  ConstValue(OperatorTypeEnum otype, ConstValue *other,
             NAMemory * outHeap = CmpCommon::statementHeap());

  // for query caching, we need a simple ConstValue copy constructor
  // to be used in creating a ConstantParameter given a ConstValue.
  // we'd like to use one of the above constructors, but they expose
  // internal details of ConstValue that we want to hide and ignore.
  ConstValue(const ConstValue& s, NAHeap *h);

  // just like the previous copy constructor but does pointer copies only;
  // ie, no new heap allocations.
  ConstValue(const ConstValue& s);

  // virtual destructor
  virtual ~ConstValue();

  // change the character set for a string value.
  const NAType* pushDownType(NAType& newType,
                      enum NABuiltInTypeEnum defaultQualifier);

  // perform safe type cast (return NULL ptr for illegal casts)
  virtual ConstValue *castToConstValue(NABoolean &  negate );

  // get the degree of this node (it is a leaf).
  virtual Int32 getArity() const;

  // A method that returns for "user-given" input values.
  // These are values that are either constants, host variables, parameters,
  // or even values that are sensed from the environment such as 
  // current time, the user name, etcetera. 
  virtual NABoolean isAUserSuppliedInput() const;

  // Method for changing the type of the constant
  void changeType(const NAType * newType) { type_ = newType; }

  // Method for changing the string constant 
  void changeStringConstant(const NAString* newStringConstant);
  
  // Accessor function for the type.
  const NAType * getType() const	  { return type_; }
  
  // return true iff I am a string literal with unknown character set
  NABoolean hasUnknownCharSet();

  void * getConstValue() const		  { return value_; }
  Lng32 getStorageSize() const		  { return storageSize_; }
  Lng32 getSize() const;

  // get constant value in string format

  // The boolean argument indicates whether the text returned
  // has been transformed (i.e., all \0 chars become "\0").
  NAString getConstStr(NABoolean transformNeeded = TRUE) const;
    
  short isNull() const			 { return isNull_ ? -1 : 0; }
  NABoolean isNullWasDefaultSpec() const { return isNull_==IS_NULL_WAS_DEFAULT;}
  void setWasDefaultSpec()		 { if (isNull_)
					     isNull_ = IS_NULL_WAS_DEFAULT;
					 }
  NABoolean isAFalseConstant() const;

  NABoolean isExactNumeric() const;

  // You must call canGetExactNumericValue() to determine if safe to call
  // one of the getExactNumericValue methods.
  //
  NABoolean canGetExactNumericValue() const;

  // Method to convert a ConstValue into an exact number,
  // result must be multiplied by 10**-scale
  Int64 getExactNumericValue(Lng32 &scale) const;

  Int64 getExactNumericValue() const
  { Lng32 scale;
    Int64 result = getExactNumericValue(scale);
    CMPASSERT(scale >= 0);
    while (scale--) result /= 10;
    return result;
  }

  // same, but getting an approximate numeric value
  NABoolean canGetApproximateNumericValue() const;
  double getApproximateNumericValue() const;

  // get offsets of the three fields stored in the data buffer
  void getOffsetsInBuffer(int &nullIndOffset,
                          int &vcLenOffset,
                          int &dataOffset);

  // Are two ConstValues equal to each other???
  virtual NABoolean operator == (const ItemExpr& other) const;
  
  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const ItemExpr & other) const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // can this base column be calculated from these values/group attributes
  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
			      const GroupAttributes& newRelExprAnchorGA,
	   	              ValueIdSet& referencedInputs,
			      ValueIdSet& coveredSubExpr,
			      ValueIdSet& unCoveredExpr) const;

  // method to do code generation
  short codeGen(Generator*);

  // return a string that identifies the operator. same as getText() except 
  // we prepend char set to string literals. This is to fix genesis case 
  // 10-040616-0347 "NF: query cache does not work properly on || for certain 
  // character set". If we change ConstValue::getText() for this fix, we risk
  // breaking QA tests due to expected output diffs.
  virtual const NAString getText4CacheKey() const;

  // get a printable string that identifies the operator
  const NAString getTextForQuery() const;
  const NAString getTextForQuery(UnparseFormatEnum form) const;

  // get a printable string that identifies the operator
  const NAString getText() const;
  virtual void unparse(NAString &result,
		       PhaseEnum phase = OPTIMIZER_PHASE,
		       UnparseFormatEnum form = USER_FORMAT,
		       TableDesc * tabId = NULL) const;

  // get raw, unquoted text
  const NAString *getRawText() const;

  // Genesis 10-980402-1556 (see Binder)
  ConstValue * toUpper(CollHeap *h = CmpCommon::statementHeap());

  // mutate text -- use with extreme caution -- only before an unparse/getText
  // for purposes of an error message, after which we throw in the towel anyway!
  void setText(const char *t)	{ *text_ = t; }

  // can this ConstValue be safely coerced into this target type?
  NABoolean canBeSafelyCoercedTo(const NAType& target);

  // does this ItemExpr (dis)qualify query to be cacheable after this phase?
  virtual NABoolean isCacheableExpr(CacheWA& cwa);

  // is this an empty string?
  NABoolean isEmptyString() const;

  virtual NABoolean calculateMinMaxUecs(ColStatDescList & histograms,
					       CostScalar & minUec,
					       CostScalar & maxUec);

  // is this a system generated const value?
  NABoolean isSystemProvided() const { return isSystemSupplied_; };

  // change literal of a cacheable query into ConstantParameter
  virtual ItemExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA);

  NABoolean isRebindNeeded() { return rebindNeeded_; };
  void setRebindNeeded(NABoolean x) { rebindNeeded_ = x; };

  NABoolean isStrLitWithCharSetPrefixSpecified() const { return isStrLitWithCharSetPrefix_; }
  // TRUE if is a string literal appearing together with a character set prefix - This data
  // member is for use with the DEFAULT string literal CLAUSE in a DDL column definition only.
  void setStrLitWithCharSetPrefixFlag(NABoolean x) { isStrLitWithCharSetPrefix_ = x; }

  // provide an interface to the SB and MB strings prepared by the parser.
  // Use with caution! At least need to check the null-ness.
  NAString* getLocaleString() { return locale_strval; };
  NAWString* getLocaleWString() { return locale_wstrval; };
  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

  virtual QR::ExprElement getQRExprElem() const;

  // computeHashValues() computes a hash value for a constant for use
  // in skew busters and most frequent value search.
  // Hash value is created only for character and numeric column types.
  UInt32 computeHashValue(const NAType& columnType);

  // does the value of this constant (if char) has trailing blanks
  NABoolean valueHasTrailingBlanks();
  
private:
  void initCharConstValue(const NAString&,
            enum CharInfo::CharSet charSet,
            enum CharInfo::Collation collation,
            enum CharInfo::Coercibility coercibility,
            NABoolean isCaseInSensitive = FALSE,
            NAMemory * outHeap = CmpCommon::statementHeap()
         );

   void initCharConstValue(const NAWString&,
            enum CharInfo::CharSet charSet,
            enum CharInfo::Collation collation,
            enum CharInfo::Coercibility coercibility,
            NABoolean isCaseInSensitive = FALSE,
            NAMemory * outHeap = CmpCommon::statementHeap(),
            enum CharInfo::CharSet strLitPrefixCharSet = CharInfo::UnknownCharSet
         );

private:

  // this indicates whether the constant is a NULL constant
  IsNullEnum isNull_;

  // type information about the constant.
  const NAType * type_;
  
  // contains the packed representation for the constant value.
  // it is used by the expression generator
  void * value_;       // untyped storage for the bit pattern
  Lng32   storageSize_; // size of the buffer anchored in value_

  // contains the text used for specifying the constant value.
  // it is captured by the parser.
  NAString *text_;
  // Indicates whether the ConstValue constructor stored a valid SQL literal
  // in *text_(TRUE) or text_ was passed by the caller (FALSE) and may or may
  // not be a valid literal.
  // NOTE: In the long term, we should validate or fix all cases and get
  // rid of this data member. Also note that FALSE doesn't mean that text_
  // is an invalid SQL literal, it just means we are keeping the older
  // code that doesn't always store valid literals.
  NABoolean textIsValidatedSQLLiteralInUTF8_;

  NABoolean rebindNeeded_; // TRUE if the string const value is originally
                           // set with an unknown charset value.

  NABoolean isStrLitWithCharSetPrefix_; // TRUE if is a str lit with charset prefix - This
  // member is used with the DEFAULT string literal CLAUSE in a DDL column definition only.

 protected:
  NABoolean isSystemSupplied_; // true iff a system-supplied literal
  NAString* locale_strval;
  NAWString* locale_wstrval;
}; // class ConstValue

typedef ConstValue* ConstValuePtrT;

// -----------------------------------------------------------------------
// A SystemLiteral is a system-supplied ConstValue. It's sole purpose in
// life is to facilitate identification of system-supplied literals so that
// query caching does not change SystemLiteral(s) into ConstantParameter(s)
// -----------------------------------------------------------------------
class SystemLiteral : public ConstValue
{
 public:
  // constructor for a system-supplied untyped NULL constant
  SystemLiteral() : ConstValue() { isSystemSupplied_ = TRUE; }
       
  // constructor for a system-supplied numeric constant
  SystemLiteral(Lng32 intval) 
    : ConstValue(intval) { isSystemSupplied_ = TRUE; }

  // constructor for a system-supplied string constant
  SystemLiteral
    (const NAString& strval,
     enum CharInfo::CharSet charSet=CharInfo::DefaultCharSet,
     enum CharInfo::Collation collation=CharInfo::DefaultCollation,
     enum CharInfo::Coercibility coercibility=CharInfo::COERCIBLE)
    : ConstValue(strval, charSet, collation, coercibility)
    { isSystemSupplied_ = TRUE; }

  // constructor for a system-supplied wide (unicode) string constant
  SystemLiteral
    (const NAWString& strval,
     enum CharInfo::CharSet charSet=CharInfo::UNICODE,
     enum CharInfo::Collation collation=CharInfo::DefaultCollation,
     enum CharInfo::Coercibility coercibility=CharInfo::COERCIBLE)
    : ConstValue(strval, charSet, collation, coercibility)
    { isSystemSupplied_ = TRUE; }

  // constructor for a system-supplied string constant that its character 
  // set is unknow at this point. Binder will determine the character set.
  SystemLiteral
    (const NAString& strval,
     const NAWString& wstrval,
     enum CharInfo::Collation collation=CharInfo::DefaultCollation,
     enum CharInfo::Coercibility coercibility=CharInfo::COERCIBLE)
    : ConstValue(strval, wstrval, collation, coercibility)
    { isSystemSupplied_ = TRUE; }

  // Supply a type, a buffer containing the packed value,
  // the size of the buffer and , optionally, the string
  // for the literal (system-supplied version)
  SystemLiteral(const NAType *type, void *value, Lng32 value_len, 
                NAString *literal = NULL)
    : ConstValue(type, value, value_len, literal) 
    { isSystemSupplied_ = TRUE; }

  // constructor to create extremal values with the option of allowing
  // or excluding NULL (system-supplied version)
  SystemLiteral
    (const NAType *type, const NABoolean wantMinValue,
     const NABoolean allowNull)
    : ConstValue(type, wantMinValue, allowNull)
    { isSystemSupplied_ = TRUE; }
}; 

// -----------------------------------------------------------------------
// The DEFAULT keyword, allowed only in the VALUES list in an INSERT,
// is really just a placeholder till Binder can replace it with the correct
// ConstValue (or reject it as a syntax error).
// -----------------------------------------------------------------------
class DefaultSpecification : public ItemExpr
{
public:

  DefaultSpecification()
    : ItemExpr(ITM_DEFAULT_SPECIFICATION)
    {}
       
  virtual ~DefaultSpecification() 	     {}

  virtual Int32 getArity() const		     { return 0; }

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // If for any reason copyTopNode() is called before bindNode then the call
  // goes to ItemExpr::copyTopNode() and it does'nt handle (by design) NULL
  // derivedNode. DefaultSpecification::copyTopNode() gets called during
  // inlining of triggers and when there is recursion involved, it happens
  // before the binder has finished. So we need to provide an override.
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // Safety net -- these methods should never be called because we should
  // never get so far as to use them.
  virtual const NAType * synthesizeType()    { CMPASSERT(FALSE); return NULL; }
  short codeGen(Generator*)		     { CMPASSERT(FALSE); return -1; }

  const NAString getText() const	     { return "DEFAULT"; }

private:

}; // class DefaultSpecification

// -----------------------------------------------------------------------
// A host variable used in a static SQL statement.
// -----------------------------------------------------------------------

class HostVar : public ItemExpr
{

public:

  enum PrototypeType { INVALID, QUALIFIEDNAME };

  // constructors for hostvars without, and with, an indicator host variable
  HostVar(const NAString & varName, 
  	  const NAType  *type, 
          NABoolean isSystemGen = FALSE) 
   : ItemExpr(ITM_HOSTVAR), varName_(varName, CmpCommon::statementHeap()), 
     varType_(type), isSystemGenerated_(isSystemGen), 
     isEnvVar_(FALSE), isDefine_(FALSE), 
     isParam_(FALSE), isCachedParam_(FALSE),
     prototypeType_(INVALID),
     prototypeValue_(CmpCommon::statementHeap()), 
     indicatorName_(CmpCommon::statementHeap()),
     rowsetInfo_(0),
     specialSyntaxType_(ExtendedQualName::NORMAL_TABLE),
     paramMode_ (COM_UNKNOWN_DIRECTION),
     ordinalPosition_ (0),
     hvIndex_ (0),
     heading_(CmpCommon::statementHeap()),
     tablename_(CmpCommon::statementHeap())
  { CMPASSERT(!varName_.isNull()); }

  // This one take OperatorTypeEnum otype as a parameter.
  HostVar(OperatorTypeEnum otype,
	  const NAString & varName, 
  	  const NAType  *type, 
          NABoolean isSystemGen = FALSE) 
   : ItemExpr(otype ), varName_(varName, CmpCommon::statementHeap()), 
     varType_(type), isSystemGenerated_(isSystemGen), 
     isEnvVar_(FALSE), isDefine_(FALSE), 
     isParam_(FALSE), isCachedParam_(FALSE),
     prototypeType_(INVALID),
     prototypeValue_(CmpCommon::statementHeap()), 
     indicatorName_(CmpCommon::statementHeap()),
     rowsetInfo_(0),
     specialSyntaxType_(ExtendedQualName::NORMAL_TABLE),
     paramMode_ (COM_UNKNOWN_DIRECTION),
     ordinalPosition_ (0),
     hvIndex_ (0),
     heading_(CmpCommon::statementHeap()),
     tablename_(CmpCommon::statementHeap())
  { CMPASSERT(!varName_.isNull()); }


  HostVar(const NAString & varName, 
          const NAString & indicatorName, 
  	  const NAType *type, 
          NABoolean isSystemGen = FALSE) 
   : ItemExpr(ITM_HOSTVAR), varName_(varName, CmpCommon::statementHeap()), 
     varType_(type), isSystemGenerated_(isSystemGen), 
     isEnvVar_(FALSE), isDefine_(FALSE), 
     isParam_(FALSE), isCachedParam_(FALSE),
     prototypeType_(INVALID),
     indicatorName_(indicatorName, CmpCommon::statementHeap()),
     prototypeValue_(CmpCommon::statementHeap()),
     rowsetInfo_(0),
     specialSyntaxType_(ExtendedQualName::NORMAL_TABLE),
     paramMode_ (COM_UNKNOWN_DIRECTION),
     ordinalPosition_ (0),
     hvIndex_ (0),
     heading_(CmpCommon::statementHeap()),
     tablename_(CmpCommon::statementHeap())
  { 
    CMPASSERT(!varName_.isNull());
    CMPASSERT(!indicatorName_.isNull());
  }

  // copy ctor
  HostVar(const HostVar&);

  virtual ~HostVar() {}

  // get the degree of this node (it is a leaf).
  virtual Int32 getArity() const;
  
  const NAString& getName() const	   { return varName_; }
  const NAType* getType() const 	   { return varType_; }
  const NAString& getIndName() const	   { return indicatorName_; }
  
  NABoolean isSystemGenerated() const      { return isSystemGenerated_; }

  NABoolean isEnvVar() const		   { return isEnvVar_; }
  void setIsEnvVar(NABoolean isEV = TRUE)  { isEnvVar_ = isEV; }

  NABoolean isDefine() const		   { return isDefine_; }
  void setIsDefine(NABoolean isDef = TRUE)  { isDefine_ = isDef; }

  NABoolean isParam() const		   { return isParam_; }
  void setIsParam(NABoolean isParam = TRUE)  { isParam_ = isParam; }

  NABoolean isCachedParam() const	    { return isCachedParam_; }
  void setIsCachedParam(NABoolean isCachedParam = TRUE)  
  { isCachedParam_ = isCachedParam; }

  NABoolean isSystemGeneratedOutputHV() const;

  NAString& getPrototypeValue() 	   { return prototypeValue_; }
  const NAString& getPrototypeValue() const { return prototypeValue_; }
  NABoolean hasPrototypeValue() const	   { return !prototypeValue_.isNull(); }
  void setPrototypeValue(const NAString &s){ prototypeValue_ = s; }

  PrototypeType getPrototypeType() const   { return prototypeType_; }
  NABoolean isPrototypeValid() const	   { return prototypeType_ != INVALID; }
  void setPrototypeType(PrototypeType type){ prototypeType_ = type; }

  // A virtual method that returns for "user-given" input values.
  // These are values that are either constants, host variables, parameters,
  // or even values that are sensed from the environment such as 
  // current time, the user name, etc:
  // Always TRUE for a HostVar.
  //
  virtual NABoolean isAUserSuppliedInput() const;

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  const NAType * pushDownType(NAType& desiredType,
                        enum NABuiltInTypeEnum defaultQualifier);

  // a virtual function for checking whether this node is 
  // ANSI character-type matching rule relaxable
  NABoolean isCharTypeMatchRulesRelaxable();

  // virtual method to fixup tree for code generation.
  virtual ItemExpr * preCodeGen(Generator *);

  // method to do code generation
  short codeGen(Generator*);

  void setSpecialType(ExtendedQualName::SpecialTableType type) 
    { specialSyntaxType_=type; }
  ExtendedQualName::SpecialTableType getSpecialType() const
  { return specialSyntaxType_; }

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const ItemExpr & other) const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // get a printable string that identifies the operator
  const NAString getText() const;

  inline ULng32 getRowsetInfo()           {return rowsetInfo_;}
  inline void setRowsetInfo (ULng32 rowsetInfo)
			    {rowsetInfo_ = rowsetInfo;}

  // Constants used by rowsetInfo_. They get populated in sqlparser.y 
  // to indicate what they are in a rowset query
  enum RowsetEnum {
    USE_TOTAL_ROWSET_SIZE             = 0x0001,  // Indicates to use whole
                                                 // rowset size in DP2
    HV_ROWSET_FOR_INPUT_SIZE          = 0x0002,  // This is the host var in
                                                 // ROWSET FOR INPUT SIZE <var>
    HV_ROWSET_FOR_OUTPUT_SIZE         = 0x0004,  // This is the host var in
                                                 // ROWSET FOR OUTPUT SIZE <var>
    HV_ROWSET_LOCAL_SIZE              = 0x0008,  // This is the host var in
                                                 // ROWSET <var> ( <list >)
    HV_INPUT_ASSIGNMENT               = 0x0010,  // This is an input host variable
                                                 // used in an assignment statement
    HV_ROWWISE_ROWSET_INPUT_ROWLEN    = 0x0020,  // max length of each input
                                                 // row in the rowwise rowset
                                                 // input buffer
    HV_ROWWISE_ROWSET_INPUT_BUFFER    = 0x0040,  // this hostvar/param contains
                                                 // the address of rowwise 
                                                 // rowwise rowset input buffer
    HV_ROWWISE_ROWSET_PARTN_NUM       = 0x0080,  // this param contains the
                                                 // partition number where
                                                 // this rwrs buffer need
                                                 // to be shipped to
    HV_ROW_IN_ROWWISE_ROWSET          = 0x0100,  // this hvar/param is part 
                                                 // of the row that will be
                                                 // moved in to the rowwise 
                                                 // rowset buffer
    NOT_A_FLAG                        = 0x8000   // We put other information
                                                 // that is not a flag
    
  }; 

  inline void setUseTotalRowsetSize() {
    rowsetInfo_ |= USE_TOTAL_ROWSET_SIZE;
  }

  inline void setHVRowsetForInputSize() {
    rowsetInfo_ |= HV_ROWSET_FOR_INPUT_SIZE;
  }

  inline void setHVRowsetForOutputSize() {
    rowsetInfo_ |= HV_ROWSET_FOR_OUTPUT_SIZE;
  }

  inline void setHVRowsetLocalSize() {
    rowsetInfo_ |= HV_ROWSET_LOCAL_SIZE;
  }

  inline void setHVInputAssignment() {
    rowsetInfo_ |= HV_INPUT_ASSIGNMENT;
  }

  inline NABoolean isHVInputAssignment() {
    return ((rowsetInfo_ & HV_INPUT_ASSIGNMENT) != 0);
  }

  // does this entire ItemExpr qualify query to be cacheable after this phase?
  virtual NABoolean isCacheableExpr(CacheWA& cwa);

 // Raj P - 01/2001
  // Parameter  mode, ordinal position and variable index
  // needed for (java) stored procedures
  ComColumnDirection getParamMode () const {return paramMode_;};
  Int32 getOrdinalPosition () const {return ordinalPosition_;};
  Int32 getHVorDPIndex () const { return hvIndex_;}
  virtual void setPMOrdPosAndIndex( ComColumnDirection paramMode,
				    Int32 ordinalPosition,
				    Int32 index);

  // Param heading and tablename needed here for JDBC/WLI, when dynamic rowsets are used.
  const NAString &getParamHeading() const {  return heading_; }

  void setParamHeading(NAString head) 
  { 
    heading_ = head; 
  }

  const NAString &getParamTablename() const { return tablename_; }

  void setParamTablename(NAString tablename) 
  { 
    tablename_ = tablename; 
  }

private:

  // name (in embedded host language syntax) of the variable and possibly indic
  NAString varName_;
  NAString indicatorName_;

  // SQLtype of the variable
  const NAType *varType_;

  // a value, always character string, always in SQL syntax
  NAString prototypeValue_;
  PrototypeType prototypeType_;

  NABoolean isEnvVar_;

  NABoolean isDefine_;

  NABoolean isParam_;

  NABoolean isCachedParam_;

  NABoolean isSystemGenerated_;	// TRUE => internally generated host var
				// should not impact histogram statistics

  ULng32 rowsetInfo_;    // Contains rowset information. Can be populated
                                // by constants in rowset enum

  ExtendedQualName::SpecialTableType specialSyntaxType_;

  // Raj P - 01/2001
  // Parameter  mode, ordinal position and variable index
  // needed for (java) stored procedures
  ComColumnDirection paramMode_;
  Int32 ordinalPosition_;
  Int32 hvIndex_;
   // In certain cases, the hostvar is assigned non-datatype attributes of
  // the target column when it is typed.
  // For internal use only, used to implement the 'temporary' blob/clob
  // solution from JDBC/WLI.
  // It is assigned the tablename and column heading of the target
  // column when used as the source of an insert (insert...values (?[10])) or
  // an update (update...set col = ?[10]) query.
  // Array parameters are converted to hostvars during the early part of binding
  // so hostvar needs these datamembers. They will be used only for dynamic rowsets 
  // from JDBC/WLI. See similar comment below for DynamicParams.
  NAString heading_;
  NAString tablename_;
}; // class HostVar


// -----------------------------------------------------------------------
// Parameter is the base class of DynamicParam and ConstantParameter
// -----------------------------------------------------------------------
class Parameter : public ItemExpr
{
public:
  // constructor
  Parameter(OperatorTypeEnum otype) 
    : ItemExpr(otype) {}

  // copy ctor
  Parameter(const Parameter& orig)
    : ItemExpr(orig) {}

  virtual ~Parameter() {}

  // get the degree of this node (it is a leaf).
  virtual Int32 getArity() const { return 0; }

  // A method that returns true for "user-given" input values.
  // These are values that are either constants, host variables, parameters,
  // or even values that are sensed from the environment such as 
  // current time, the user name, etcetera. 
  virtual NABoolean isAUserSuppliedInput() const { return TRUE; }

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  virtual ItemExpr * preCodeGen(Generator * generator);

  // method to do code generation
  short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                                 CollHeap* outHeap = 0);
}; // class Parameter

// ConstantParameter is used to represent each constant
// parameter of a cacheable query that has been normalizedForCache.
class ConstantParameter : public Parameter
{
  ConstValue *val_;  // the constant value we're treating as a cache parameter
  NAType     *type_; // same as val_'s type except it's nullable
  PositionSet *posns_;// set of positions that share this ConstantParameter

 public:
  // create a ConstantParameter
  ConstantParameter(const ConstValue& v, NAMemory *h, 
                    NABoolean quantizeLen=FALSE, UInt32 p=0);

  // create a ConstantParameter with a given NAType
  ConstantParameter(ConstValue* v, NAMemory *h, const NAType* typ, UInt32 p=0);

  // copy constructor
  ConstantParameter(const ConstantParameter& cp, NAHeap *h);

  // free our allocated memory 
  virtual ~ConstantParameter();

  // append an ascii-version of ConstantParameter into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cachewa) const;

  Lng32 getSize() const;

  // get a printable string that identifies the operator
  const NAString getText() const;

  const NAType* getType() const { return type_; }

  ConstValue * getConstVal() const { return val_; }

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const ItemExpr & other) const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                                 CollHeap* outHeap = 0);

  virtual ConstantParameter *castToConstantParameter() { return this; }

  NABoolean matches(ConstValue *val) const;

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

  virtual QR::ExprElement getQRExprElem() const;

  void addPosition(UInt32 p) { *posns_ += p; }
  PositionSet *getPositionSet() { return posns_; }
};

// Selectivity represents a query cache entry's SelParameter's selectivity.
// Selectivity helps determine when two cache entries match. Two cache entries'
// SelParameter's match iff their types & selectivities match.
class Selectivity {
 public:
  Selectivity(CostScalar s) : val_(s) {}
  Selectivity(const Selectivity &s) : val_(s.val_) {}

  virtual ~Selectivity() {}
  
  // return true iff two selectivities match or are within some 
  // tolerance specified by the CQD QUERY_CACHE_SELECTIVITY_TOLERANCE
  NABoolean operator==(const Selectivity& other) const;

  CostScalar getValue() const { return val_; };

 private:
  CostScalar val_; 
};

struct ParamType {
  NAType      *type_; // type of that ConstantParameter
  PositionSet *posns_;// set of positions that share this ConstantParameter
  ParamType() : type_(0), posns_(NULL) {}
  ParamType(NAType *t, PositionSet *r) : type_(t), posns_(r) {}
};

// SelParamType is the type & selectivity of a cache entry's SelParameter
struct SelParamType {
  NAType      *type_; // type of that SelParameter
  Selectivity sel_;   // selectivity of that SelParameter
  PositionSet *posns_;// set of positions that share this SelParameter
  SelParamType() : sel_(CostScalar(0)), type_(0), posns_(NULL) {}
  SelParamType(NAType *t, const Selectivity& s, PositionSet *r) 
    : sel_(s), type_(t), posns_(r) {}
};

// SelParameter is used to represent a cacheable query's selection parameter
class SelParameter : public ConstantParameter
{
  Selectivity sel_;  // the selectivity part
 public:
  // create a SelParameter 
  SelParameter(ConstValue* v, NAMemory *h, const NAType* typ,
               const Selectivity s, UInt32 p)
    : ConstantParameter(v, h, typ, p), sel_(s) {}

  // copy constructor
  SelParameter(const SelParameter& cp, NAHeap *h) 
    : ConstantParameter(cp, h), sel_(cp.sel_) {}

  // free our allocated memory 
  virtual ~SelParameter() {}

  // accessor
  const Selectivity getSelectivity() const { return sel_; }

  // append an ascii-version of SelParameter into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cachewa) const;

  virtual const SelParameter *castToSelParameter() const { return this; }

  ConstValue * castToConstValue(NABoolean & negate_it);
};

// -----------------------------------------------------------------------
// A parameter supplied in a dynamic SQL statement.
// -----------------------------------------------------------------------

class DynamicParam : public Parameter
{

public:

  // constructors (for unnamed, named, and named/unnamed with indicator)
  DynamicParam(CollHeap * h=0) 
    : Parameter(ITM_DYN_PARAM), paramName_(h),
      indicatorName_(h),
      heading_(h),
      tablename_(h),
      paramMode_ (COM_UNKNOWN_DIRECTION),
      ordinalPosition_ (0),
      udrFormalParamName_(h),
      dpIndex_ (0),
      rowsetSize_(0),
      rowsetInfo_(0),
      original_(0)
  {}

  DynamicParam(const NAString & paramName, CollHeap * h=0) 
    : Parameter(ITM_DYN_PARAM), paramName_(paramName, h),
      indicatorName_(h),
      heading_(h),
      tablename_(h),
      paramMode_ (COM_UNKNOWN_DIRECTION),
      ordinalPosition_ (0),
      udrFormalParamName_(h),
      dpIndex_ (0),
      rowsetSize_(0),
      rowsetInfo_(0),
      original_(0)
  {}

  DynamicParam(const NAString & paramName, 
               const NAString & indicatorName, 
               CollHeap * h=0) 
    : Parameter(ITM_DYN_PARAM), paramName_(paramName, h), 
      indicatorName_(indicatorName, h),
      heading_(h),
      tablename_(h),
      paramMode_ (COM_UNKNOWN_DIRECTION),
      ordinalPosition_ (0),
      udrFormalParamName_(h),
      dpIndex_ (0),
      rowsetSize_(0),
      rowsetInfo_(0),
      original_(0)
  {}

  // copy ctor
  DynamicParam (const DynamicParam & orig, CollHeap * h=0)
       : Parameter(orig), 
         paramName_(orig.paramName_, h), 
         indicatorName_(orig.indicatorName_, h),
	 heading_(orig.heading_, h),
         tablename_(orig.tablename_, h),
	 paramMode_ (orig.paramMode_),
         ordinalPosition_ (orig.ordinalPosition_),
         udrFormalParamName_(orig.udrFormalParamName_, h),
	 dpIndex_ (orig.dpIndex_),
	 rowsetSize_ (orig.rowsetSize_),
         rowsetInfo_(orig.rowsetInfo_),
         original_(orig.original_)
  {}

  DynamicParam* getOriginal() const { return original_; }

  void setOriginal (DynamicParam* o) { original_ = o; }

  const NAString& getName() const	            
  { 
    return paramName_; 
  }

  const NAString& getIndicatorName() const 
  {
    return indicatorName_;
  }

  const NAString &getParamHeading() const
  {
    return heading_;
  }

  void setParamHeading(NAString head)
  {
    heading_ = head;
  }

  const NAString &getParamTablename() const
  {
    return tablename_;
  }

  void setParamTablename(NAString tablename)
  {
    tablename_ = tablename;
  }

  ULng32 getRowsetSize() const	            
  { 
    return rowsetSize_; 
  }

  void setRowsetSize( const ULng32 size)
  {
     rowsetSize_ = size;
  }

  ULng32 getRowsetInfo() const	            
  { 
    return rowsetInfo_; 
  }

  void setRowsetInfo( const ULng32 info)
  {
     rowsetInfo_ = info;
  }

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const ItemExpr & other) const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // get a printable string that identifies the operator
  const NAString getText() const;

  // does this entire ItemExpr qualify query to be cacheable after this phase?
  virtual NABoolean isCacheableExpr(CacheWA& cwa);
    
  // Methods to support CALL statement processing
  ComColumnDirection getParamMode () const {return paramMode_;};
  Int32 getOrdinalPosition () const {return ordinalPosition_;};
  Int32 getHVorDPIndex () const { return dpIndex_;}
  virtual void setPMOrdPosAndIndex( ComColumnDirection paramMode,
				    Int32 ordinalPosition,
				    Int32 index);
  void setUdrFormalParamName(const NAString &name)
  {
    udrFormalParamName_ = name;
  }
  const NAString &getUdrFormalParamName() const
  {
    return udrFormalParamName_;
  }

  const NAType * pushDownType(NAType& desiredType, enum NABuiltInTypeEnum defaultQualifier);

  inline void setDPRowsetForInputSize() {
    rowsetInfo_ |= HostVar::HV_ROWSET_FOR_INPUT_SIZE;
  }


  inline NABoolean isDPRowsetForInputSize() {
    return ((rowsetInfo_ & HostVar::HV_ROWSET_FOR_INPUT_SIZE) != 0);
  }

  inline void setRowwiseRowsetInputMaxRowlen() {
    rowsetInfo_ |= HostVar::HV_ROWWISE_ROWSET_INPUT_ROWLEN;
  }

  inline NABoolean isRowwiseRowsetInputMaxRowlen() {
    return ((rowsetInfo_ & HostVar::HV_ROWWISE_ROWSET_INPUT_ROWLEN) != 0);
  }

  inline void setRowwiseRowsetInputBuffer() {
    rowsetInfo_ |= HostVar::HV_ROWWISE_ROWSET_INPUT_BUFFER;
  }

  inline NABoolean isRowwiseRowsetInputBuffer() {
    return ((rowsetInfo_ & HostVar::HV_ROWWISE_ROWSET_INPUT_BUFFER) != 0);
  }

  inline void setRowwiseRowsetPartnNum() {
    rowsetInfo_ |= HostVar::HV_ROWWISE_ROWSET_PARTN_NUM;
  }

  inline NABoolean isRowwiseRowsetPartnNum() {
    return ((rowsetInfo_ & HostVar::HV_ROWWISE_ROWSET_PARTN_NUM) != 0);
  }

  inline void setRowInRowwiseRowset() {
    rowsetInfo_ |= HostVar::HV_ROW_IN_ROWWISE_ROWSET;
  }

  inline NABoolean isRowInRowwiseRowset() {
    return ((rowsetInfo_ & HostVar::HV_ROW_IN_ROWWISE_ROWSET) != 0);
  }

private:

  // the name of the param ("" for unnamed params) and the indicator (if any)
  NAString paramName_;
  NAString indicatorName_;

  // In certain cases, the param is assigned non-datatype attributes of
  // the target column when it is typed.
  // For internal use only, used to implement the 'temporary' blob/clob
  // solution from JDBC/WLI.
  // It is assigned the tablename and column heading of the target
  // column when used as the source of an insert (insert...values (?)) or
  // an update (update...set col = ?) query.
  // This information is returned during describe time and used by the caller
  // to find out if this param was assigned to a blob/clob'ed column.
  // See sqlparser.y for the special heading generated for a BLOB/CLOB col.
  NAString heading_;
  NAString tablename_;

  ULng32 rowsetSize_;
  ULng32 rowsetInfo_;

  // The following are only used when compiling a CALL statement:
  //
  //   parameter mode - IN, OUT, or INOUT. UNDEFINED for non-CALL
  //   statements.
  //
  //   parameter ordinal position - Formal parameter position. Values
  //   start at 1. 0 for non-CALL statements.
  //
  //   parameter index - An ordering of parameters according to their
  //   position in the SQL text. Values start at 1. 0 for non-CALL
  //   statements.
  // 
  //   formal parameter name - only set when a single dynamic
  //   parameter is used as a CALL statement IN or INOUT argument. If
  //   the formal parameter name is set and this dynamic parameter
  //   object is unnamed, then when the CALL statement is DESCRIBEd we
  //   return the formal parameter name in the SQLDESC_NAME descriptor
  //   entry rather than an empty name. Only JDBC and ODBC use these
  //   formal names currently. The purpose is to allow clients to set
  //   CALL statement input values by name when a given input argument
  //   is represented by a single dynamic parameter in the CALL
  //   statement text. This bookkeeping is not done for OUT parameters
  //   because DESCRIBE names for OUT parameters come from the RelExpr
  //   object's RETDesc not from the dynamic parameter instances
  //   themselves.
  //
  ComColumnDirection paramMode_;
  Int32 ordinalPosition_;
  Int32 dpIndex_;
  NAString udrFormalParamName_;

  DynamicParam* original_; // null or param we were copyTopNode from
}; // class DynamicParam

// -----------------------------------------------------------------------
// A fake parameter used to represent the routines formal intputs and
// actual output
// -----------------------------------------------------------------------
class RoutineParam : public Parameter
{

public:

  // Constructurs
  RoutineParam(CollHeap * h=0) 
    : Parameter(ITM_ROUTINE_PARAM), paramName_(h),
      paramMode_(COM_UNKNOWN_DIRECTION),
      ordinalPosition_(0),
      optionalParam_(FALSE),
      isCacheable_(FALSE),
      paramType_(0),
      rdesc_(0)
  {
      memset(argumentType_, 0, sizeof(argumentType_));
  }

  RoutineParam(const NAString & paramName, const NAType *type, CollHeap * h=0) 
    : Parameter(ITM_ROUTINE_PARAM), paramName_(paramName, h),
      paramMode_(COM_UNKNOWN_DIRECTION),
      ordinalPosition_(0),
      optionalParam_(FALSE),
      isCacheable_(FALSE),
      paramType_(type),
      rdesc_(0)
  {
      memset(argumentType_, 0, sizeof(argumentType_));
  }

  RoutineParam(const NAString & paramName, const NAType *type, Lng32 pos, 
      ComColumnDirection direction, RoutineDesc *rdesc, CollHeap * h=0) 
    : Parameter(ITM_ROUTINE_PARAM), paramName_(paramName, h),
      paramMode_(direction),
      ordinalPosition_(pos),
      optionalParam_(FALSE),
      isCacheable_(FALSE),
      paramType_(type),
      rdesc_(rdesc)
  {
      memset(argumentType_, 0, sizeof(argumentType_));
  }

  RoutineParam(NAColumn *col, Lng32 pos, 
      RoutineDesc *rdesc, CollHeap * h=0) 
    : Parameter(ITM_ROUTINE_PARAM), paramName_(col == NULL ? "" : col->getColName() , h),
      paramMode_(col == NULL ? COM_UNKNOWN_DIRECTION : col->getColumnMode()),
      ordinalPosition_(pos),
      optionalParam_(col == NULL ? FALSE : col->isOptional()),
      isCacheable_(FALSE),
      paramType_(col == NULL ? NULL : col->getType()),
      rdesc_(rdesc)
  {
      memset(argumentType_, 0, sizeof(argumentType_));
      if (col != NULL)
        strncpy(argumentType_, col->getRoutineParamType(), 2);
  }

  RoutineParam(const NAString & paramName, const NAType *type, Lng32 pos, 
      ComColumnDirection direction, char * argType, RoutineDesc *rdesc, CollHeap * h=0) 
    : Parameter(ITM_ROUTINE_PARAM), paramName_(paramName, h),
      paramMode_(direction),
      ordinalPosition_(pos),
      optionalParam_(FALSE),
      isCacheable_(FALSE),
      paramType_(type),
      rdesc_(rdesc)
  {
      memset(argumentType_, 0, sizeof(argumentType_));
      strncpy(argumentType_, argType, 2); 
  }

  RoutineParam(RoutineDesc * rdesc, 
               Lng32 position, 
               CollHeap * h=0) 
    : Parameter(ITM_ROUTINE_PARAM), paramName_(h), 
      paramMode_(COM_UNKNOWN_DIRECTION),
      ordinalPosition_(position),
      optionalParam_(FALSE),
      isCacheable_(FALSE),
      paramType_(0),
      rdesc_(rdesc)
  {
      memset(argumentType_, 0, sizeof(argumentType_));
  }

  // copy ctor
  RoutineParam (const RoutineParam & orig, CollHeap * h=0)
       : Parameter(orig), 
         paramName_(orig.paramName_, h), 
	 paramMode_(orig.paramMode_),
	 paramType_(orig.paramType_),
	 optionalParam_(orig.optionalParam_),
         ordinalPosition_(orig.ordinalPosition_),
         isCacheable_(orig.isCacheable_),
         rdesc_(orig.rdesc_)
  {
      memset(argumentType_, 0, sizeof(argumentType_));
  }

  const NAString& getName() const	            
  { 
    return paramName_; 
  }

  void setName(NAString & name) 
  { 
    paramName_ = name; 
  }

  const NAType * getType() const	            
  { 
    return paramType_; 
  }

  void setType(NAType *type) 
  { 
    paramType_ = type; 
  }

  const char * getArgType() const	            
  { 
    return argumentType_; 
  }

  void setArgType(char *type) 
  { 
    strncpy(argumentType_ , type,3); 
  }
  const NABoolean optionalParam() const	            
  { 
    return optionalParam_; 
  }

  void setOptionalParam(NABoolean optional) 
  { 
    optionalParam_ = optional; 
  }

  // does this entire ItemExpr qualify query to be cacheable after this phase?
  virtual NABoolean isCacheableExpr(CacheWA& cwa)
  { 
    return isCacheable_; 
  }

  virtual NABoolean isAUserSuppliedInput() const { return FALSE; }

  void setIsCacheableExpr(NABoolean cacheable) 
  { 
    isCacheable_ = cacheable; 
  }

  ComColumnDirection getParamMode() const	            
  { 
    return paramMode_; 
  }

  void setParamMode(ComColumnDirection direction) 
  { 
    paramMode_ = direction; 
  }

  const Int32 getOrdinalPos() const	            
  { 
    return ordinalPosition_; 
  }

  void setOrdinalPos(Int32 pos) 
  { 
    ordinalPosition_ = pos; 
  }

  const RoutineDesc * getRoutineDesc() const	            
  { 
    return rdesc_; 
  }

  void setRoutineDesc(RoutineDesc * rdesc)
  { 
    rdesc_ = rdesc; 
  }

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const ItemExpr & other) const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // get a printable string that identifies the operator
  const NAString getText() const;

    
  // Methods to support CALL statement processing
  Int32 getOrdinalPosition () const {return ordinalPosition_;};

  // redefine pushDownType()
  const NAType * pushDownType(NAType& desiredType,
                    enum NABuiltInTypeEnum defaultQualifier)
  { return &desiredType; }



private:

  NAString           paramName_;
  const NAType      *paramType_;
  NABoolean          optionalParam_;
  NABoolean          isCacheable_;
  ComColumnDirection paramMode_;
  Int32                ordinalPosition_;
  RoutineDesc       *rdesc_;
  char               argumentType_[3];

}; // class RoutineParam

// -----------------------------------------------------------------------
// A select list index used in an ORDER BY clause.
// -----------------------------------------------------------------------

class SelIndex : public ItemExpr
{

public:

  SelIndex(ULng32 i, ItemExpr * exprInGrbyClause = NULL) 
       : ItemExpr(ITM_SEL_INDEX), 
	 selIndex_(i),
	 exprInGrbyClause_(exprInGrbyClause),
	 renamedColNameInGrbyClause_(FALSE)
  {}

  // virtual destructor
  virtual ~SelIndex() {}

  // get the degree of this node (it is a leaf).
  virtual Int32 getArity() const;

  // accessor functions
  ULng32 getSelIndex() const { return selIndex_; }
  void setSelIndex(ULng32 idx) { selIndex_ = idx; }
  const ItemExpr * getExprInGrbyClause() const { return exprInGrbyClause_; }
  ItemExpr * getExprInGrbyClause() { return exprInGrbyClause_; }
  NABoolean renamedColNameInGrbyClause() 
  { return renamedColNameInGrbyClause_; }
  void setRenamedColNameInGrbyClause(NABoolean val) 
  {renamedColNameInGrbyClause_ = val;}

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // get a printable string that identifies the operator
  const NAString getText() const;

private:

  // An index into the select list.  The range is 1..n, where n is the number
  // of expressions in the select list.
  ULng32 selIndex_;

  // points to the expression in the groupby clause that got replaced
  // by this SelIndex. See RelRoot::transformGroupByWithOrdinalPhase1
  // for details. Used to compute view text.
  ItemExpr * exprInGrbyClause_;

  // If a grouby or having clause contains a name that is both a 
  // renamed colname and a base column name then we have
  // decided that the base column will win. Grouping will be done 
  // according to the base column and not the renamed col.
  // If this SelIndex is being used to point to a renamed column in the 
  // GroupBy clause then renamedColNameInGrbyClause_
  // will be TRUE. If an ordinal is present in this clause
  // then renamedColNameInGrbyClause_ will be FALSE.
  // Replacement of renamedcols with a select index is done in 
  // transformGroupByWithOrdinalPhase1. Then in GroupByAgg::bindNode()
  // we replace the select index with a colreference to the
  // base column if we detect that renamed column is the same as an
  // existing base column.
  // select a + 1 as b from t group by b ; (create table t (a int, b int) ;)
  // This statement should raise an error since the select list contains columns
  // that are not in the group by clause.
  NABoolean renamedColNameInGrbyClause_;
}; // class SelIndex

#endif /* ITEMCOLREF_H */
