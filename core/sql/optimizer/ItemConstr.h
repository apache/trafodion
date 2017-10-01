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
#ifndef ITEMCONSTR_H
#define ITEMCONSTR_H
/* -*-C++-*-
******************************************************************************
*
* File:         ItemConstr.h
* Description:  Item expressions dealing with table constraints. These item
*               expressions are mainly used in the constraints_ field of
*               group attributes.
*
* Created:      12/20/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "ItemExpr.h"
#include "ObjectNames.h"

// forward references
class BindWA;
class NAColumn;
class UpdateColumns;

// contents of this file
class Constraint;
class OptConstraint;
class CheckConstraint;
class CardConstraint;
class UniqueOptConstraint;
class FuncDependencyConstraint;
class UniqueConstraint;
class RefConstraint;
class AbstractRIConstraint;
class ComplementaryRIConstraint;

class CheckConstraintList;
class RefConstraintList;
class AbstractRIConstraintList;

// -----------------------------------------------------------------------
// Parent class for all Ansi constraints (all of which have Ansi names, natch).
//
// Derived from ItemExpr solely because subclass CheckConstraint needs it and
// a) I couldn't get this multiple inheritance working:
//	class Constraint : public NABasicObject ...
//	class CheckConstraint : public ItemExpr, public Constraint ...
// b) I have no time to split CheckConstraint in two, a la Unique[Opt]Constraint
//	class Constraint : public NABasicObject ...
//	class CheckConstraint : public Constraint ...
//	class CheckOptConstraint : public ItemExpr ...
//	  CheckOptConstraint(const CheckConstraint &CheckConstraint) ...
// Thus we're wasting a little space in all the other Ansi constraints; too bad.
// -----------------------------------------------------------------------
class Constraint : public ItemExpr
{
public:

  Constraint(OperatorTypeEnum itmType, const QualifiedName &cName, CollHeap * h=0)
       : ItemExpr(itmType),
         constraintName_(cName, h),
         colSignature_(h)
  {}

  // copy ctor
  Constraint (const Constraint & cons, CollHeap * h=0)
       : ItemExpr(cons.getOperatorType(), cons.child(0), cons.child(1)),
         constraintName_(cons.constraintName_, h),
         referencedCols_(cons.referencedCols_),
         colSignature_(cons.colSignature_, h)
  {}

  const QualifiedName &getConstraintName() const { return constraintName_; }

  typedef SET(Lng32) ColSignature;	// unordered set of column positions
  //##should be SUBARRAY(long)?  NABitVector?
  //##SUBARRAY(NAColumn *), with the array being NATable's NAColumnArray?
  //##
  //##cf AbstractRIConstraint::constraintOverlapsUpdateCols for required methods
  //##perhaps should be a separate class (reusable) with its own new methods?

  typedef ARRAY(NAColumn *) KeyColumns;	// ordered array of columns

  static void makeColSignature(const ValueIdSet &assigns, ColSignature &outsig);
  static void makeColSignature(const KeyColumns &columns, ColSignature &outsig);

protected:

  const ColSignature &colSignature() const { return colSignature_; }

private:

  QualifiedName constraintName_;

  // A fast way to know all the BaseColumn ItemExpr's referenced "interestingly"
  // by this ItemExpr
  // (see MarkAsReferencedColumn method elsewhere in the Binder).
  // These are *bound* columns.
  ValueIdSet referencedCols_;

  // Used in RI to choose only pertinent constraints when possible
  // (ignoring irrelevant ones makes execution of the ins/upd/del faster).
  // These are *unbound* columns.
  //
  // ## To be used on CHECK constraints too at some future point, not just
  // ## the simpler RI case...
  ColSignature colSignature_;

}; // Constraint


// -----------------------------------------------------------------------
// Optimizer constraint
//
// This is the counterpart to the ANSI constraint that is defined through
// DDL operations. An optimizer constraint is synthesized from other
// constraints and information in a RelExpr tree.
// -----------------------------------------------------------------------

class OptConstraint : public ItemExpr
{
public:

  OptConstraint(OperatorTypeEnum itmType) : ItemExpr(itmType) {}
  virtual ~OptConstraint() {}

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);
  virtual Int32 getArity() const = 0;
};

// -----------------------------------------------------------------------
// Check constraint
//
// Unbound instances (text only, no ValueIds/none of ItemExpr used)
// are attached to the appropriate NATable (which, as a generic description
// of a table by name, cannot have ValueIds on it).
// Bound instances are attached to TableDescs only (one TD per table reference
// in the query, ok).
// -----------------------------------------------------------------------
class CheckConstraint : public Constraint
{
  // ITM_CHECK_CONSTRAINT
public:

  // Constructor for table check constraint
  CheckConstraint(const QualifiedName &cName, const NAString &text, CollHeap * h=0)
       : Constraint(ITM_CHECK_CONSTRAINT, cName, h),		// check constraint name
         constraintText_(text, h),
         isViewWithCheckOption_(FALSE),
         cascadingView_(h)
  {}

  // Constructor for view with check option
  CheckConstraint(const QualifiedName &vName,
  		  const QualifiedName &cascadingView,
                  CollHeap * h=0)
       : Constraint(ITM_CHECK_CONSTRAINT, vName, h),		// view name
         constraintText_(h),
         cascadingView_(cascadingView, h),			// topmost WCO view
         isViewWithCheckOption_(TRUE)
  {}

  // copy ctor
  CheckConstraint (const CheckConstraint & chek, CollHeap * h=0)
       : Constraint (chek, h),
         constraintText_(chek.constraintText_, h),
         cascadingView_(chek.cascadingView_, h),
         isViewWithCheckOption_(chek.isViewWithCheckOption_)
  {}

  virtual ~CheckConstraint() {}
  virtual Int32 getArity() const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);
  void unparse(NAString &result,PhaseEnum phase,UnparseFormatEnum form,
               TableDesc * tabId = NULL) const;
  const NAString getText() const;

  const NAString &getConstraintText() const { return constraintText_; }

  NABoolean isViewWithCheckOption() const   { return isViewWithCheckOption_; }
  void setViewWithCheckOption(NABoolean t)  { isViewWithCheckOption_ = t; }
  const QualifiedName &getCascadingViewName() const { return cascadingView_; }
  NABoolean isTheCascadingView() const 	    { return isViewWithCheckOption_ &&
  						getConstraintName() ==
						getCascadingViewName();
					    } // i.e. "isTopmostWCOview()"
private:

  NAString constraintText_;

  // This flag discriminates between the run-time errors
  // 'integrity constraint violation' and 'with check option violation'.
  NABoolean isViewWithCheckOption_;

  // Name of the topmost view in the view creation stack which was defined
  // with ''with check option'.  May or may not be the same as vName.
  // See the BindWA::bindView() caller.
  QualifiedName cascadingView_;

}; // CheckConstraint

class CheckConstraintList : public LIST(CheckConstraint *)
{
public:
  CheckConstraintList(CollHeap* h/*=0*/) : LIST(CheckConstraint *)(h) {}
  // copy ctor
  CheckConstraintList(const CheckConstraintList & cclist, CollHeap *h) :
    LIST(CheckConstraint *)(cclist, h) {}

  virtual ~CheckConstraintList() {}
};


// -----------------------------------------------------------------------
// Lower/upper bound on the cardinality of a relational expression
//
// A construct internal to the optimizer, not an external ANSI notion,
// so no need to subclass this one from class Constraint
// -----------------------------------------------------------------------

class CardConstraint : public OptConstraint
{
  // ITM_CARD_CONSTRAINT
public:

  CardConstraint(Cardinality lowerBound = 0,
		 Cardinality upperBound = INFINITE_CARDINALITY)
    : OptConstraint(ITM_CARD_CONSTRAINT),
      lowerBound_(lowerBound),
      upperBound_(upperBound) {}

  virtual ~CardConstraint() {}

  // get the degree of this node (it is a leaf).
  virtual Int32 getArity() const;

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // accessor functions
  Cardinality getLowerBound() const { return lowerBound_; }
  Cardinality getUpperBound() const { return upperBound_; }
  void setLowerBound(Cardinality v) { lowerBound_ = v; }
  void setUpperBound(Cardinality v) { upperBound_ = v; }

  // get a printable string that identifies the operator
  const NAString getText() const;
  void unparse(NAString &result,PhaseEnum phase,UnparseFormatEnum form,
               TableDesc * tabId = NULL) const;

private:

  Cardinality lowerBound_;
  Cardinality upperBound_;

}; // CardConstraint


// -----------------------------------------------------------------------
// Uniqueness of a combination of columns/expressions in a table:
// a) from OptLogRelExpr.C (internal, like CardConstraint).
// -----------------------------------------------------------------------
class UniqueOptConstraint : public OptConstraint
{
  // ITM_UNIQUE_OPT_CONSTRAINT
public:

  // ctor for unique constraints discovered by the optimizer
  UniqueOptConstraint(const ValueIdSet &uniqueCols)
    : OptConstraint(ITM_UNIQUE_OPT_CONSTRAINT), uniqueCols_(uniqueCols) {}

  // convert the other kind of u.c. into this internal kind
  //##useful extra info for Optimizer at some future point...

  UniqueOptConstraint(const UniqueConstraint &uniqueConstraint);

  virtual ~UniqueOptConstraint() {}

  // get the degree of this node (it is a leaf).
  virtual Int32 getArity() const;

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // accessor functions
  ValueIdSet &uniqueCols() { return uniqueCols_; }

  // get a printable string that identifies the operator
  const NAString getText() const;
  void unparse(NAString &result,PhaseEnum phase,UnparseFormatEnum form,
               TableDesc * tabId = NULL) const;

private:

  ValueIdSet uniqueCols_;

}; // UniqueOptConstraint

// -----------------------------------------------------------------------
// Functional dependency of a set of columns from another set of columns.
// For a definition of functional dependencies see
// http://www.acm.org/sigmod/dblp/db/journals/tods/Bernstein76.html
// or section 4.18 of the SQL99 "Foundation".
// -----------------------------------------------------------------------
class FuncDependencyConstraint : public OptConstraint
{
public:

  // ctor for unique constraints discovered by the optimizer
  FuncDependencyConstraint(const ValueIdSet &determiningCols,
			   const ValueIdSet &dependentCols)
    : OptConstraint(ITM_FUNC_DEPEND_CONSTRAINT),
      determiningCols_(determiningCols),
      dependentCols_(dependentCols) {}

  virtual ~FuncDependencyConstraint() {}

  // get the degree of this node (it is a leaf).
  virtual Int32 getArity() const;

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // accessor functions
  const ValueIdSet & getDeterminingCols() const  { return determiningCols_; }
  const ValueIdSet & getDependentCols() const      { return dependentCols_; }

  // create functional dependencies for a group from one of its child
  // RelExprs (adds new functional dependency constraints to ga of parent)
  static void synthFunctionalDependenciesFromChild(
       GroupAttributes &ga,
       const RelExpr *child,
       NABoolean createNewDependencies);

  // reduce a set of columns such that it contains no entries that are
  // functionally dependent on the other elements of the set
  void minimizeUniqueCols(ValueIdSet &uniqueCols);

  // get a printable string that identifies the operator
  const NAString getText() const;
  void unparse(NAString &result,PhaseEnum phase,UnparseFormatEnum form,
               TableDesc * tabId = NULL) const;

private:

  ValueIdSet determiningCols_;
  ValueIdSet dependentCols_;
};

// -----------------------------------------------------------------------
// Internal check constraint, e.g. a predicate used in synthesizing
// physical properties like a sort order. These may be needed to validate
// that a given sort order does indeed satisfy a requirement.
// -----------------------------------------------------------------------
class CheckOptConstraint : public OptConstraint
{
  // ITM_CHECK_OPT_CONSTRAINT
public:

  // ctor for unique constraints discovered by the optimizer
  CheckOptConstraint(const ValueIdSet &checkPreds)
    : OptConstraint(ITM_CHECK_OPT_CONSTRAINT), checkPreds_(checkPreds) {}

  // convert the other kind of u.c. into this internal kind
  //##useful extra info for Optimizer at some future point...

  virtual ~CheckOptConstraint();

  // get the degree of this node (it is a leaf).
  virtual Int32 getArity() const;

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // accessor functions
  const ValueIdSet &getCheckPreds() { return checkPreds_; }

  // get a printable string that identifies the operator
  const NAString getText() const;
  void unparse(NAString &result,PhaseEnum phase,UnparseFormatEnum form,
               TableDesc * tabId = NULL) const;

private:

  ValueIdSet checkPreds_;

}; // CheckOptConstraint

// -----------------------------------------------------------------------
//
// Auxiliary classes and definitions used by the
// referential integrity constraints, UniqueConstraint and RefConstraint.
//
//	Integrity without knowledge is weak and useless, and
//	knowledge without integrity is dangerous and dreadful.
//	        -- Samuel Johnson, _Rasselas_
//
// -----------------------------------------------------------------------

class AbstractRIConstraint : public Constraint
{
public:

  AbstractRIConstraint(OperatorTypeEnum itmType,
  		       const QualifiedName &cName,
		       const QualifiedName &tName,
		       CollHeap* heap)
   : Constraint(itmType, cName, heap), defTableName_(&tName), keyColumns_(heap) {}

  // copy ctor
  AbstractRIConstraint (const AbstractRIConstraint & aric, CollHeap * h=0) :
       Constraint(aric, h),
       defTableName_(aric.defTableName_),
       keyColumns_(aric.keyColumns_, h)
  {}

  virtual ~AbstractRIConstraint();
  virtual Int32 getRefConstraints(BindWA *bindWA,
				const ColSignature &updateCols,
				RefConstraintList &resultList) = 0;

  virtual void resetAfterStatement() = 0;

  const QualifiedName &getDefiningTableName() const { return *defTableName_; }

  void setKeyColumns(const struct TrafConstrntsDesc*, CollHeap*);

  const KeyColumns &keyColumns() const { return keyColumns_; }

  AbstractRIConstraint *findConstraint(BindWA *bindWA,
				       const ComplementaryRIConstraint &riInfo)
				      const;

protected:

  NABoolean constraintOverlapsUpdateCols(BindWA *bindWA,
 					 const ColSignature &updateCols) const;

  // Pointer to name of the table which defined this constraint
  // (pointer to within an NATable)
  const QualifiedName *defTableName_;

  KeyColumns keyColumns_;

}; // AbstractRIConstraint

class ComplementaryRIConstraint : public NABasicObject
{
public:

  // This class represents the complementing half of an RI constraint --
  // the UC referenced by an FK, or one (of perhaps many) FK referencing a UC

  ComplementaryRIConstraint(const QualifiedName &cName,
  			    const QualifiedName &tName,
                            CollHeap * h=0)
       : constraintName_(cName, h), tableName_(tName, h), keyColumns_(NULL) {}

  // copy ctor
  ComplementaryRIConstraint (const ComplementaryRIConstraint & cric, CollHeap * h=0)
       : constraintName_(cric.constraintName_, h),
         tableName_(cric.tableName_, h),
         keyColumns_(cric.keyColumns_)
  {}

  // needed by Collections template for reasons not now apparent:
  ComplementaryRIConstraint (CollHeap * h=0) :
       constraintName_("",h), tableName_("",h), keyColumns_(NULL) {}
  NABoolean operator==(const ComplementaryRIConstraint& c)
			{ return constraintName_ == c.constraintName_; }

  // This method resets any pointers to the 'other table''s data after
  // each statement. For example: the KeyColumns_ points to the keyColumns_ 
  // from the NATable of the 'other table'. This pointer may not be valid
  // if the NATable cache of the 'other table' is refreshed. 
  void resetAfterStatement();

  // Let the data be freely available to the RI classes using this helper class
  //private:

  // A subset of fields as in AbstractRIConstraint, but *tweaked* a lil differnt
  // Conceptually, this is the same, just less baggage/overhead hopefully.
  //
  // keyColumns_ is used by RefConstraint but not needed by UniqueConstraint

  const Constraint::KeyColumns *keyColumns() const { return keyColumns_; }

  QualifiedName constraintName_;	// same as Constraint
  QualifiedName tableName_;		// cf. AbstractRIConstraint's pointer
  const Constraint::KeyColumns *keyColumns_;	// cf. AbstractRIConstraint's

  const QualifiedName &getConstraintName() const { return constraintName_; }
  const QualifiedName &getTableName() const { return tableName_; }

}; // ComplementaryRIConstraint

// -----------------------------------------------------------------------
// Uniqueness of a combination of columns in a table:
// b) from DDL (including PRIMARY KEY, a type of UNIQUE constraint).
// -----------------------------------------------------------------------
class UniqueConstraint : public AbstractRIConstraint
{
  // ITM_UNIQUE_CONSTRAINT
public:

  friend class RefConstraint;	//RefConstraint::getRefConstraints();

  UniqueConstraint(const QualifiedName &cName,
                   const QualifiedName &tName,
		   CollHeap* heap,
		   NABoolean PK = FALSE)
       : AbstractRIConstraint(ITM_UNIQUE_CONSTRAINT, cName, tName, heap),
         refConstraintsReferencingMe_(heap),
         isPrimaryKey_(PK) {}

  // copy ctor
  UniqueConstraint (const UniqueConstraint & unic, CollHeap * h=0)
       : AbstractRIConstraint (unic, h),
         refConstraintsReferencingMe_(unic.refConstraintsReferencingMe_, h),
         isPrimaryKey_(unic.isPrimaryKey_)
  {}

  virtual ~UniqueConstraint();
  virtual void resetAfterStatement();
  virtual Int32 getRefConstraints(BindWA *bindWA,
				const ColSignature &updateCols,
				RefConstraintList &resultList);
  void setRefConstraintsReferencingMe(const struct TrafConstrntsDesc*,
                                    CollHeap*, BindWA*);
  NABoolean hasRefConstraintsReferencingMe()
  { return NOT refConstraintsReferencingMe_.isEmpty() ; };

  const NABoolean isPrimaryKeyConstraint() { return isPrimaryKey_; }

  Lng32 getNumRefConstraintsReferencingMe()
  {
    return refConstraintsReferencingMe_.entries();
  }

  const ComplementaryRIConstraint *getRefConstraintReferencingMe(Lng32 i)
  {
    if ((hasRefConstraintsReferencingMe()) &&
	((i >= 0) && i <= refConstraintsReferencingMe_.entries()))
    return refConstraintsReferencingMe_[i];
    
    return NULL;
  }
private:

  // List of pointers to ComplementaryRIConstraint.
  LIST(ComplementaryRIConstraint*) refConstraintsReferencingMe_;	// zero or more

  NABoolean isPrimaryKey_;

}; // UniqueConstraint

// -----------------------------------------------------------------------
// Referential constraint:  One table references another.
// The first table's DDL defines a constraint saying
//   FOREIGN KEY (mycol1,..,mycoln) REFERENCES othertable (othercolx,..)
// where the other table's list of columns is actually a set previously defined
// as a UNIQUE constraint on the other table.
//
// The first table is known variously as the
//   child, detail, referencing, FK/foreign key table
// while the second is the
//   parent, master, referenced, PK/primary key, or UC/unique constraint table.
//
// The whole scheme with the ComplementaryRIConstraint's array of columns
// "parallel" to the defining constraint's columns assumes that constraint info
// is stored in CatMan (or at least by NATable) in the
// manner described for this example:
//   CREATE TABLE U ... CONSTRAINT U0 UNIQUE(V,W,X)    [ or PRIMARY KEY(V,W,X) ]
//   CREATE TABLE F ... CONSTRAINT R1 FOREIGN KEY(G,H,I) REFERENCES U(X,W,V)
//	U0 keycols [1.V 2.W 3.X]
//	R1 keycols [1.I 2.H 3.G] i.e. positions relative to the unique key,
//	i.e. *not* [1.G 2.H 3.I]!
// (These positions may be zero-based; I don't know; this is illustrative only.)
// -----------------------------------------------------------------------
class RefConstraint : public AbstractRIConstraint
{
  // ITM_REF_CONSTRAINT
public:

  friend class UniqueConstraint;	//UniqueConstraint::getRefConstraints();

  RefConstraint(const QualifiedName &cName, const QualifiedName &tName,
  		const QualifiedName &cRefd, const QualifiedName &tRefd,
		CollHeap* heap)
       : AbstractRIConstraint(ITM_REF_CONSTRAINT, cName, tName, heap),
         uniqueConstraintReferencedByMe_(cRefd, tRefd, heap),
         matchPartial_(FALSE), isEnforced_(TRUE)
  { otherTableName_ = &uniqueConstraintReferencedByMe_.tableName_; }

  // copy ctor
  RefConstraint (const RefConstraint & refc, CollHeap * h=0)
       : AbstractRIConstraint (refc, h),
         uniqueConstraintReferencedByMe_(refc.uniqueConstraintReferencedByMe_, h),
         otherTableName_(refc.otherTableName_),
         matchPartial_(refc.matchPartial_),
	 isEnforced_(refc.isEnforced_)
  {}
  virtual ~RefConstraint();
  virtual void resetAfterStatement() 
  {uniqueConstraintReferencedByMe_.resetAfterStatement();};

  virtual Int32 getRefConstraints(BindWA *bindWA,
				const ColSignature &updateCols,
				RefConstraintList &resultList);

  inline NABoolean selfRef() const;
  inline NABoolean isaForeignKeyinTableBeingUpdated() const;
  inline NABoolean referencesTableBeingUpdated() const;

  const QualifiedName &getOtherTableName() const
  {
    return *otherTableName_;
  } 

  inline void getMyKeyColumns(LIST(Lng32)& colPositions) const;
  void getOtherTableKeyColumns(BindWA *bindWA, LIST(Lng32)& colPositions) const;

  void getMatchOptionPredicateText(NAString &text, 
				   NAString *corrName) const;
  
  void getPredicateText(NAString &text, NAString *corrName) const;

  NABoolean isRINeededForUpdatedColumns(UpdateColumns *UpdatedColumns);

  NABoolean getIsEnforced() const {return isEnforced_ ;} ;
  void setIsEnforced(NABoolean val) {isEnforced_ = val ;} ;

  const QualifiedName& getUniqueConstraintName() const
  {
    return uniqueConstraintReferencedByMe_.constraintName_;
  }

  const ComplementaryRIConstraint& getUniqueConstraintReferencedByMe() const
  {
    return uniqueConstraintReferencedByMe_;
  }

private:

  void getPredicateText(NAString &text,
  		        const QualifiedName &tblName,
			const KeyColumns &keyColumns,
                        NAString *corrName = NULL) const;
  
// The instigating table has a UniqueConstraint which this RefC is referencing;
// tell this RefC that its defining table is "the other" table
// relative to the table being ins/upd/del (the instigating table).

  void setOtherTableName()
  {
    // Assertion to catch any impossible/weird persistence bug in SchemaDB/NATable
    CMPASSERT(otherTableName_ == &getDefiningTableName() || otherTableName_ == &uniqueConstraintReferencedByMe_.tableName_ ||
   	      selfRef());
    otherTableName_ = &getDefiningTableName();
  }

// Tell this RefC that the "the other" table is the table referenced by 
// its defining table. 
  void resetOtherTableName()
  {
    // Assertion to catch any impossible/weird persistence bug in SchemaDB/NATable
    CMPASSERT(otherTableName_ == &getDefiningTableName() || otherTableName_ == &uniqueConstraintReferencedByMe_.tableName_ ||
   	      selfRef());
    otherTableName_ = &uniqueConstraintReferencedByMe_.tableName_;
  }

  void KeyColumnsToPositionsList( LIST(Lng32)& colPositions, 
								  const KeyColumns& keyColumns) const;

  ComplementaryRIConstraint uniqueConstraintReferencedByMe_;	// exactly one

  // Pointer to the "other" table relative to the table being ins/upd/del.
  // Initially points to the referenced table but can be set the other way
  // (see the inline setOtherTableName comments below)
  const QualifiedName *otherTableName_;

  NABoolean matchPartial_;				//## set() this later
  NABoolean isEnforced_;

}; // RefConstraint


class AbstractRIConstraintList : public LIST(AbstractRIConstraint *)
{
public:
  AbstractRIConstraintList(CollHeap* h/*=0*/) : LIST(AbstractRIConstraint *)(h) {}
  // copy ctor
  AbstractRIConstraintList (const AbstractRIConstraintList & arilist, CollHeap * h)
    : LIST(AbstractRIConstraint *)(arilist, h) {}

  virtual ~AbstractRIConstraintList() {}

  Int32 getRefConstraints(BindWA *bindWA,
			const ValueIdSet &assigns,
			RefConstraintList &resultList) const;
  void resetAfterStatement();
};

class RefConstraintList : public LIST(RefConstraint *)
{
public:
  RefConstraintList(CollHeap* h/*=0*/) : LIST(RefConstraint *)(h) {}
  // copy ctor
  RefConstraintList (const RefConstraintList & rclist, CollHeap * h)
    : LIST(RefConstraint *)(rclist, h) {}
  virtual ~RefConstraintList() {}

  RefConstraintList *getNeededRIs(UpdateColumns *UpdatedColumns, CollHeap *heap);
};

// -----------------------------------------------------------------------
// Foreign Key side of a Referential constraint (optimizer version)
// the optimizer version uses Value Ids and has fewer data members than the 
// class RefConstraint.
// This class is used to represent a Referential constraint used from 
// Normalizer onwards.
// This object is constructed during Scan::synthLogProp() in the Normalizer
// As the parent nodes of the scan have their logical properties synthesized
// this constraint flows up until it is either (a) matched with its 
// corresponding uniqueness constraint (in which case the matched flag is set)
// or (b) its foreign key(FK) characteristic is destroyed. The foreign key 
// characteristic is said to be destroyed when the valueids that make up
// the FK are no longer part of the outputs of a node. This will happen
// when the FK columns are  not part of char. outputs of a node. This means 
// that either 
// (1) there is no join on the FK columns up above
// (2) Nodes such as Union, transpose, outer join has altered the value 
// of the FK columns
// If the FK characteristic is destroyed we simply stop flowing this constraint
// up the query tree.
// A match is made between the RefOptConstraint and the corresponding
// ComplementaryRefOptConstraint when 
// (a) the name attribute in the two objects are identical
//and (b) each (ForeignKey col, UniqueKey col) pair is related by an equality 
//    join predicate
//and (c) there is no other join predicate between the referenced and refencing
//tables.
// Since valueids are used to enforce conditions (b) and (c) above, if there 
// are 2 intances of the same unique key table in a query, it is possible to
// match them separately in the same query.
// -----------------------------------------------------------------------
class RefOptConstraint : public OptConstraint
{
  // ITM_REF_OPT_CONSTRAINT
public:

  // ctor for unique constraints discovered by the optimizer
  RefOptConstraint(const ValueIdList &fkCols, const QualifiedName &ukCName, 
		    CollHeap * h=NULL)
    : OptConstraint(ITM_REF_OPT_CONSTRAINT), fkCols_(fkCols), 
      uniqueConstraintName_(ukCName, h), isMatched_(FALSE) {}

  RefOptConstraint(const RefOptConstraint &riOptConstr)
    : OptConstraint (ITM_REF_OPT_CONSTRAINT),
         fkCols_(riOptConstr.fkCols_),
	 uniqueConstraintName_(riOptConstr.uniqueConstraintName_),
	 isMatched_(riOptConstr.isMatched_)
  {}

  virtual ~RefOptConstraint() {}

  // get the degree of this node (it is a leaf).
  virtual Int32 getArity() const;

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = NULL);

  // accessor functions
  ValueIdList &foreignKeyCols() { return fkCols_; }
  const QualifiedName &uniqueConstraintName() const 
  { return uniqueConstraintName_ ;}

  // get a printable string that identifies the operator
  virtual const NAString getText() const;
  virtual void unparse(NAString &result,PhaseEnum phase,UnparseFormatEnum form,
               TableDesc * tabId = NULL) const;

  const NABoolean isMatched() const 
  { return isMatched_ ;}

  void setIsMatched(NABoolean val)
  { isMatched_ = val;}

private:

  ValueIdList fkCols_;
  // Name of Unique Constraint that I (this Ref Constraint) points to.
  // Used for matching purpose at the Join node that eventually
  // sees the two sides of this Ref constraint together for the first 
  // time
  QualifiedName uniqueConstraintName_;
  NABoolean isMatched_;
}; // RefOptConstraint

// -----------------------------------------------------------------------
// Unique constraint (UC) side of a Referential Constraint (optimizer version)
// The optimizer version uses Value Ids and has fewer data members than
// UniqueConstraint or ComplimentaryRIConstraint.
// Unique constraint side of Referential constraint used from Normalizer onwards.
// This object is constructed during Scan::synthLogProp() in the Normalizer.
// As the parent nodes of the scan have their logical properties synthesized
// this constraint flows up until either
// (a) the rows produced by the scan are filtered by a predicate 
// or otherwise reduced by a groupby
// or (b) the UC columns are not needed by parent nodes and are therefore not part
// of char. outputs. This means that there is no join on the UC columns up above
// or (c) the UC columns are altered by a node such as an Union or outer join such
// that the value ids of the UC cols are no longer outputs of this node
// In situations (a), (b) and (c) this constraint can no longer be matched
// with its corresponding RefOptConstraint and is not flowed up the query tree
// anymore.
// -----------------------------------------------------------------------
class ComplementaryRefOptConstraint : public OptConstraint
{
  // ITM_COMP_REF_OPT_CONSTRAINT
public:

  // Constructor
  ComplementaryRefOptConstraint(const ValueIdList &ucCols,
				const QualifiedName &cName, 
				RelExpr * tabPtr, 
				TableDesc * tabDesc, 
                                CollHeap * h=NULL,
                                NABoolean isMatchedForElimination=FALSE)
       : OptConstraint(ITM_COMP_REF_OPT_CONSTRAINT), 
         ucCols_(ucCols), 
	 constraintName_(cName,h), // uniqueness constraint name
	 tabPtr_(tabPtr),
	 tabDesc_(tabDesc),
         isMatchedForElimination_(FALSE)
  {}

  // copy ctor
  ComplementaryRefOptConstraint(const ComplementaryRefOptConstraint & constr, 
				  CollHeap * h=0)
       : OptConstraint (ITM_COMP_REF_OPT_CONSTRAINT),
         ucCols_(constr.ucCols_),
	 constraintName_(constr.constraintName_),
	 tabPtr_(constr.tabPtr_),
	 tabDesc_(constr.tabDesc_),
         isMatchedForElimination_(constr.isMatchedForElimination_)
  {}

  virtual ~ComplementaryRefOptConstraint() {}
  virtual Int32 getArity() const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = NULL);
  virtual void unparse(NAString &result,PhaseEnum phase,UnparseFormatEnum form,
               TableDesc * tabId = NULL) const;
  virtual const NAString getText() const;

  // accessor functions
  ValueIdList &uniqueKeyCols() { return ucCols_; }
  const QualifiedName &constraintName() const 
  { return constraintName_ ;}

  const RelExpr *getReferencedTable() const 
  { return tabPtr_ ;}

  const TableDesc *getTableDesc() const 
  { return tabDesc_ ;}

  TableDesc *getTableDesc()
  { return tabDesc_ ;}

  void setReferencedTable(RelExpr * ptr)
  { tabPtr_ = ptr;}

  void setIsMatchedForElimination(NABoolean val)
  { isMatchedForElimination_ = val; }
  
  NABoolean getIsMatchedForElimination()
  { return isMatchedForElimination_; }
private:

  ValueIdList ucCols_;
  QualifiedName constraintName_;
  RelExpr * tabPtr_;
  TableDesc * tabDesc_;
  NABoolean isMatchedForElimination_;

}; // ComplementaryRefOptConstraint


// -----------------------------------------------------------------------
// Inlines
// -----------------------------------------------------------------------

// Is defining (referencing) table the same as the referenced table?
//
inline NABoolean RefConstraint::selfRef() const
{ return getDefiningTableName() == uniqueConstraintReferencedByMe_.tableName_; }

// Tell whether setOtherTableName() was called
//
inline NABoolean RefConstraint::isaForeignKeyinTableBeingUpdated() const
{ return (otherTableName_ == &uniqueConstraintReferencedByMe_.tableName_) ;}

inline NABoolean RefConstraint::referencesTableBeingUpdated() const
{ return !isaForeignKeyinTableBeingUpdated(); }

inline void RefConstraint::getMyKeyColumns(LIST(Lng32)& colPositions) const
{ KeyColumnsToPositionsList(colPositions, keyColumns()); }


#endif /* ITEMCONSTR_H */
