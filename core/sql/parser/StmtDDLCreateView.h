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
#ifndef STMTDDLCREATEVIEW_H
#define STMTDDLCREATEVIEW_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLCreateView.h
 * Description:  class for parse nodes representing Create View
 *               statements
 *
 *               Also contains definitions of classes describing
 *               the view usages information.
 *
 *
 * Created:      1/5/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ComASSERT.h"
#include "ComOperators.h"
#include "ComSmallDefs.h"
#include "ElemDDLColViewDefArray.h"
#include "ElemDDLLocation.h"
#include "NABoolean.h"
#include "ObjectNames.h"
#include "ParNameLocList.h"
#include "ParTableUsageList.h"
#include "RelExpr.h"
#include "StmtDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLCreateView;
class ParViewColTablesUsage;
class ParViewColTablesUsageList;
class ParViewColTableColsUsage;
class ParViewColTableColsUsageList;
class ParViewTableColsUsage;
class ParViewTableColsUsageList;
class ParViewUsages;

extern NABoolean ParIsTracingViewUsages();

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// definition of class ParViewTableColsUsage
// -----------------------------------------------------------------------

class ParViewTableColsUsage : public NABasicObject
{
public:

  // default constructor
  ParViewTableColsUsage(CollHeap * h=PARSERHEAP());

  // initialize constructor
  ParViewTableColsUsage(const ColRefName &colName, CollHeap * h=PARSERHEAP());

  // copy ctor
  ParViewTableColsUsage (const ParViewTableColsUsage & orig, 
                         CollHeap * h=PARSERHEAP()) ; // not written

  // virtual destructor
  virtual ~ParViewTableColsUsage();

  //
  // operator
  //

  NABoolean operator==(const ParViewTableColsUsage &rhs) const;
  
  //
  // accessors
  //
  
  inline const NAString & getColumnName() const;

        // returns the referenced column name in internal format.

  inline const ExtendedQualName & getTableQualName() const;

        // returns the referenced table name in QualifedName format.

  //
  // mutators
  //

  inline void setColumnName(const NAString &colName);
  inline void setTableName(const ExtendedQualName &tableQualName);

private:
  
  ExtendedQualName tableName_;
  NAString      columnName_;
  
}; // class ParViewTableColsUsage

// -----------------------------------------------------------------------
// definition of class ParViewTableColsUsageList
// -----------------------------------------------------------------------

class ParViewTableColsUsageList : private LIST(ParViewTableColsUsage *)
{
public:

  // constructor
  ParViewTableColsUsageList(CollHeap *heap = PARSERHEAP());

        // heap specifies the heap to allocate space for objects
        // pointed by the elements in this list.

  // virtual destructor
  virtual ~ParViewTableColsUsageList();

  // operators
  inline const ParViewTableColsUsage & operator[](CollIndex index) const;
  inline       ParViewTableColsUsage & operator[](CollIndex index);

  //
  // accessors
  //
  
  inline CollIndex entries() const;

  inline const ParViewTableColsUsage * const find(const ColRefName &
                                                    colName) const;

  ParViewTableColsUsage * const find(const ColRefName &colName);

        // returns the pointer pointing to the ParViewTableColsUsage
        // element in the list containing the specified colName;
        // returns the NULL pointer value if not found.

  //
  // mutator
  //
  
  NABoolean insert(const ColRefName &columnName);

        // 1. If columeName is not in the list, inserts it to the end of
        //    the list and return TRUE.
        // 2. If the columnName is already in the list, returns FALSE.

private:

  ParViewTableColsUsageList(const ParViewTableColsUsageList &);  // DO NOT USE
  ParViewTableColsUsageList & operator=
    (const ParViewTableColsUsageList &);                         // DO NOT USE

  //
  // heap to allocate space for objects pointed by elements in the list.
  //
  CollHeap * heap_;

}; // class ParViewTableColsUsageList

// -----------------------------------------------------------------------
// View Column Tables Usage
// -----------------------------------------------------------------------
class ParViewColTablesUsage : public NABasicObject
{
public:

  //
  // constructors
  //

  ParViewColTablesUsage(CollHeap * h=PARSERHEAP());
  ParViewColTablesUsage(const CollIndex usingViewColNum,
                        const ExtendedQualName &usedObjName,
                        CollHeap * h=PARSERHEAP());
  // copy ctor
  ParViewColTablesUsage (const ParViewColTablesUsage & orig, CollHeap * h=PARSERHEAP()) ; 

  //
  // virtual destructor
  //
  
  virtual ~ParViewColTablesUsage();

  //
  // operator
  //

  NABoolean operator==(const ParViewColTablesUsage &rhs) const;

  //
  // accessors
  //
  
  inline const CollIndex getUsingViewColumnNumber() const;
  inline const ExtendedQualName & getUsedObjectName() const;

  //
  // mutators
  //
  
  inline void setUsingViewColumnNumber(const CollIndex usingColNum);
  inline void setUsedObjectName(const ExtendedQualName &usedObjName);
  
private:
  
  CollIndex     usingViewColumnNumber_;
  ExtendedQualName usedObjectName_;
  
}; // class ParViewColTablesUsage

// -----------------------------------------------------------------------
// View Column Table Columns Usage List
// -----------------------------------------------------------------------
class ParViewColTablesUsageList : private LIST(ParViewColTablesUsage *)
{
public:

  //
  // constructor
  //

  ParViewColTablesUsageList(CollHeap *heap = PARSERHEAP());

        // heap specifies the heap to allocate space for objects
        // pointed by the elements in this list.
  
  //
  // virtual destructor
  //
  
  virtual ~ParViewColTablesUsageList();

  //
  // operators
  //
  
  inline const ParViewColTablesUsage & operator[](CollIndex index)
                const;
  inline       ParViewColTablesUsage & operator[](CollIndex index);

  //
  // accessors
  //
  
  inline CollIndex entries() const;

  const  ParViewColTablesUsage * const find(const CollIndex usingViewColNum)
                const;
  ParViewColTablesUsage * const find(const CollIndex usingViewColNum);

        // returns the pointer pointing to the ParViewColTablesUsage
        // element in the list containing the specified usingViewColNum.
        // returns the NULL pointer value if not found.

  //
  // mutator
  //
  
  NABoolean insert(const CollIndex usingViewColumnNumber,
                   const ExtendedQualName &usedObjectName);

        // inserts to the list and returns TRUE if not in the list;
        // otherwise, returns FALSE.

private:

  ParViewColTablesUsageList(const ParViewColTablesUsageList &);  // DO NOT USE
  ParViewColTablesUsageList & operator=
    (const ParViewColTablesUsageList &);                         // DO NOT USE
  //
  // heap to allocate space for objects pointed by elements in the list.
  //
  CollHeap * heap_;

}; // class ParViewColTablesUsageList

// -----------------------------------------------------------------------
// View Column Table Columns Usage
// -----------------------------------------------------------------------
class ParViewColTableColsUsage : public NABasicObject
{
public:

  //
  // constructors
  //

  ParViewColTableColsUsage(CollHeap * h=PARSERHEAP());
  ParViewColTableColsUsage(const CollIndex usingViewColumnNumber,
                           const ColRefName &usedObjectColumnName,
                           CollHeap * h=PARSERHEAP());
  // copy ctor
  ParViewColTableColsUsage (const ParViewColTableColsUsage & orig, CollHeap * h=PARSERHEAP()) ; 

  //
  // virtual destructor
  //
  
  virtual ~ParViewColTableColsUsage();

  //
  // operator
  //

  NABoolean operator==(const ParViewColTableColsUsage &rhs) const;

  //
  // accessors
  //
  
  inline const CollIndex getUsingViewColumnNumber() const;
  inline const ColRefName & getUsedObjectColumnName() const;

  //
  // mutators
  //
  
  inline void setUsingViewColumnNumber(const CollIndex colNum);
  inline void setUsedObjectColumnName(const ColRefName &colRefName);

private:
  
  CollIndex  usingViewColumnNumber_;
  ColRefName usedObjectColumnName_;
  
}; // class ParViewColTableColsUsage

// -----------------------------------------------------------------------
// View Column Table Columns Usage List
// -----------------------------------------------------------------------
class ParViewColTableColsUsageList : private LIST(ParViewColTableColsUsage *)
{
public:

  //
  // constructor
  //

  ParViewColTableColsUsageList(CollHeap *heap = PARSERHEAP());

        // heap specifies the heap to allocate space for objects
        // pointed by the elements in this list.
  
  //
  // virtual destructor
  //
  
  virtual ~ParViewColTableColsUsageList();

  //
  // operators
  //
  
  inline const ParViewColTableColsUsage & operator[](CollIndex index)
                const;
  inline       ParViewColTableColsUsage & operator[](CollIndex index);

  //
  // accessors
  //
  
  inline CollIndex entries() const;

  inline
  const ParViewColTableColsUsage * const find(const CollIndex usingViewColNum)
                const;
        ParViewColTableColsUsage * const find(const CollIndex usingViewColNum);

        // returns the pointer pointing to the ParViewColTableColsUsage
        // element in the list containing the specified usingViewColNum.
        // Returns the NULL pointer value if not found.

  inline
  const ParViewColTableColsUsage * const find(const CollIndex usingViewColNum,
                                              const ColRefName &usedColRefName)
                const;
        ParViewColTableColsUsage * const find(const CollIndex usingViewColNum,
                                              const ColRefName &usedColRefName);

        // returns the pointer pointing to the ParViewColTableColsUsage
        // element in the list containing the specified usingViewColNum,
        // the usedColRefName.getCorrNameObj().getQualifiedNameObj(), and
        // the usedColRefName.getColName() values.  Returns the NULL
        // pointer value if not found.

  const CollIndex getIndex(const CollIndex usingViewColumnNumber) const;
  
        // returns the index to the first entry of the list of elements
        // containing the usingViewColumnNumber; returns NULL_COLL_INDEX
        // if not found.

  const CollIndex getIndexOfNextElem(const CollIndex
                                     usingViewColumnNumber) const;

        // returns the index to the first entry of the list of elements
        // containing the using-view-column-number that is greater than
        // usingViewColumnNumber; returns NULL_COLL_INDEX if
        // usingViewColumnNumber is the biggest value so far (or if the
        // list is currently empty).
  
  //
  // mutators
  //
  
  void clear();

        // destroys all elements in the list.

  NABoolean insert(const CollIndex usingViewColumnNumber,
                   const ColRefName &usedObjectColumnName);

        // inserts to the list and returns TRUE if not in the list;
        // otherwise, returns FALSE.

private:

  ParViewColTableColsUsageList
    (const ParViewColTableColsUsageList &); // DO NOT USE
  ParViewColTableColsUsageList & operator=
    (const ParViewColTableColsUsageList &); // DO NOT USE

  //
  // heap to allocate space for objects pointed by elements in the list.
  //
  CollHeap * heap_;

}; // class ParViewColTableColsUsageList

// -----------------------------------------------------------------------
// View Usages
// -----------------------------------------------------------------------
class ParViewUsages : public NABasicObject
{
public:

  ParViewUsages(CollHeap * heap = PARSERHEAP());

  virtual ~ParViewUsages();

  //
  // accessors
  //
  
  inline       CollIndex getCurViewColNum() const;  // for Binder use only

  inline const ExtendedQualName *getUsedTableNamePtr
                (const ExtendedQualName &tableName) const;

  inline const ParViewTableColsUsageList & getViewTableColsUsageList() const;
  inline       ParViewTableColsUsageList & getViewTableColsUsageList();

  inline const ParTableUsageList & getViewTableUsageList() const;
  inline       ParTableUsageList & getViewTableUsageList();

  inline const ParViewColTablesUsageList & getViewColTablesUsageList() const;
  inline       ParViewColTablesUsageList & getViewColTablesUsageList();

// KSKSKS

  const ParViewColTableColsUsageList & getViewColTableColsUsageList()
                const;
        ParViewColTableColsUsageList & getViewColTableColsUsageList();

// KSKSKS

  inline       NABoolean isItmColRefInColInRowVals() const;  // Binder use only

  inline       NABoolean isViewSurelyNotUpdatable() const;   // Binder use only

        // Please note the we don't know whether view is updatable or not
        // until we are done with the Normalization phase.  This method,
        // therefore, cannot provide accurrate information.  Without knowing
        // for sure if the view is not updatable, the binder has to collect
        // the view-column-table-columns-usage information even though this
        // information is not needed if the view is not updatable.  Collecting
        // this information is not an easy task because non-updatable view
        // can be pretty complex.  However, in certain cases, the binder can
        // find out that a view is not updatable; for example, the query
        // expression contains natural join operation.  To make the binder's
        // task simpler, the binder will stop collection the view-column-
        // table-columns-usage information as soon as it knows.

  //
  // mutators
  //

  inline       void setCurViewColNum(const CollIndex curVwColNum);  // Binder

  inline       void setItmColRefInColInRowValsFlag(const NABoolean  // Binder
                                                   isItmColRef);    // use only

  inline       void setViewIsSurelyNotUpdatableFlag();   // for Binder use only

        // Please see comments for method isViewSurelyNotUpdatable()

private:
  
  ParTableUsageList            usedTableNameList_;
  ParViewTableColsUsageList    usedColRefList_;
  ParViewColTablesUsageList    viewColTablesUsageList_;
  ParViewColTableColsUsageList viewColTableColsUsageList_;

  //
  // information about the order number of the view column
  // in the view column list (the order number of the leftmost
  // column is 0).  This field is set to NULL_COLL_INDEX if
  // not processing view column list.  This data member
  // is used during the Binding phase only.
  //

  CollIndex curViewColNum_;

  //
  // The following data member is used in the Binding phase to
  // find out whether the current column in the bindRowValues()
  // is an ITM_REFERENCE (either a column reference or a star
  // column reference) or not (e.g., a complex item expression).
  //

  NABoolean isItmColRefInColInRowVals_;

  //
  // see comments for method isViewSurelyNotUpdatable()
  //

  NABoolean isViewSurelyNotUpdatable_;

}; // class ParViewUsages

// -----------------------------------------------------------------------
// Create View statement
// -----------------------------------------------------------------------
class StmtDDLCreateView : public StmtDDLNode
{

public:

  // constructor
  StmtDDLCreateView(const QualifiedName & viewName,
                    const ParNameLocList & nameLocList,
                    ElemDDLNode * optionalViewColumnList,
                    ElemDDLNode * optionalLocationClause,
                    RelExpr     * queryExpression,
                    ElemDDLNode * optionalWithCheckOption,
                    ComCreateViewBehavior   createViewBehavior,
                   ElemDDLNode *pOptionalOwner, 
                   CollHeap    * heap = PARSERHEAP());

  // virtual destructor
  virtual ~StmtDDLCreateView();

  // cast
  virtual StmtDDLCreateView * castToStmtDDLCreateView();

  // pointers to child parse nodes (getChild and setChild methods)
  enum { INDEX_VIEW_COLUMN_LIST = 0,
	       VIEW_COLUMN_LIST = INDEX_VIEW_COLUMN_LIST,
         INDEX_LOCATION_CLAUSE,
               LOCATION_CLAUSE = INDEX_LOCATION_CLAUSE,
         INDEX_QUERY_EXPRESSION,
	       QUERY_EXPRESSION = INDEX_QUERY_EXPRESSION,
         INDEX_WITH_CHECK_OPTION,
	       WITH_CHECK_OPTION = INDEX_WITH_CHECK_OPTION,
         INDEX_VIEW_OWNER,
               VIEW_OWNER = INDEX_VIEW_OWNER,
         MAX_STMT_DDL_CREATE_VIEW_ARITY };

  //
  // accessors
  //

  virtual Int32 getArity() const;

  inline NABoolean getIsUpdatable() const	{ return isUpdatable_;  }
  inline NABoolean getIsInsertable() const	{ return isInsertable_; }

  inline ComLevels getCheckOptionLevel() const;

        // returns the Check Option level when the With Check
        // Option clause appears in the Create View statement
        // (the default is COM_CASCADED_LEVEL); otherwise,
        // returns COM_UNKNOWN_LEVEL.

  virtual ExprNode * getChild(Lng32 index);

  inline CollIndex getCurViewColNum() const;  // for Binder use only

  inline const StringPos getEndPosition() const;

        // returns the ending position (the position of the
        // last character) of the statement (within the input
        // string)
  
  inline const NAString & getLocation() const;

        // returns the location name specified in the LOCATION
        // clause/phrase associating with the view; returns an
        // empty string if the LOCATION clause does not appear.
  
  inline ElemDDLLocation::locationNameTypeEnum getLocationNameType() const;

        // returns the type of the location name (e.g., an OSS
        // path name, a Guardian device name, an OSS environment
        // variable name, etc.)  If LOCATION clause does not
        // appear, the returned value has no meaning.
  inline const NABoolean isCreateView() const;
  inline const NABoolean isCreateSystemView() const;
  inline const NABoolean isCreateOrReplaceView() const;
  inline const NABoolean isCreateOrReplaceViewCascade() const;
  inline const ComCreateViewBehavior getCreateViewBehavior() const;
  inline const ParNameLocList & getNameLocList() const;
  inline       ParNameLocList & getNameLocList();

        // returns a list of locations of names appearing in
        // the statement input string.  The list helps with
        // the computing of the view text.

  inline const RelExpr * getQueryExpression() const;
  inline       RelExpr * getQueryExpression();

        // returns the pointer pointing to the parse sub-tree
        // representing the query expression in the view
        // definition.
  
  inline const StringPos getStartPosition() const;

        // returns the starting position (the position of the
        // first character) of the statement (within the
        // input string)
  
  inline const ElemDDLColViewDefArray & getViewColDefArray() const;
  inline       ElemDDLColViewDefArray & getViewColDefArray();

        // returns an array of pointers pointing to the Column
        // Name parse nodes.  If the View Column List phrase
        // is not specified in the Create View statement,
        // returns an empty array.

  inline const NAString getViewName() const;

        // returns the name of the defined view.

  inline const QualifiedName & getViewNameAsQualifiedName() const;
  inline       QualifiedName & getViewNameAsQualifiedName();

        // returns the name of the defined view in
        // QualifiedName format

  inline const ParViewUsages & getViewUsages() const;
  inline       ParViewUsages & getViewUsages();

        // returns the UDF usage list
  inline const LIST(OptUDFInfo *) &getUDFList() const;
  inline       LIST(OptUDFInfo *) &getUDFList();

        // returns the view usages information after the parse
        // node is bound; returns an empty object otherwise.
  
  inline NABoolean isItmColRefInColInRowVals() const;  // for Binder use only

  inline NABoolean isLocationSpecified() const;

        // returns TRUE if the location clause/phrase appears;
        // returns FALSE otherwise.

  inline NABoolean isProcessingViewColList() const;

        // This method is used during the Binding phase to help
        // with collecting view column usage information

  inline NABoolean isWithCheckOptionSpecified() const;

        // returns FALSE if the With ... Check Option clause
        // is not specified in the Create View Statement;
        // returns TRUE otherwise.

  inline NABoolean isOwnerSpecified() const;

        // returns TRUE if the BY <owner> phrase appears
        // in the Create statement; returns FALSE otherwise.

  inline const ElemDDLGrantee *getOwner() const;
        // returns pointer to the optional "by owner"
        // (in the form of an ElemDDLGrantee).

  //
  // mutators
  //
  
  void incrCurViewColNum();

  inline void resetCurViewColNum();

  virtual void setChild(Lng32 index, ExprNode * pChildNode);

  inline void setCurViewColNum(const CollIndex curVwCol);  // for Binder use

  inline void setItmColRefInColInRowValsFlag(const NABoolean  // for Binder
                                             isItmColRef);    // use only
  inline void setEndPosition(const StringPos endPos);

        // sets the ending position (the position of the
        // last character) of the statement (within the
        // input string)
  
  inline void setStartPosition(const StringPos startPos);

        // sets the starting position (the position of the
        // first character) of the statement (within the
        // input string)

  const NABoolean createIfNotExists() const { return createIfNotExists_; }
  void setCreateIfNotExists(NABoolean v) { createIfNotExists_ = v; }

  //
  // method for binding
  //
  
  ExprNode * bindNode(BindWA *bindWAPtr);

  // method for collecting information
  void synthesize();

        // collects information in the parse sub-tree and
        // copy/move them to the current parse node.

  //
  // methods for tracing
  //
  
  virtual const NAString displayLabel1() const;
  virtual const NAString displayLabel2() const;
  virtual const NAString displayLabel3() const;
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;


private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  //
  // default constructor
  //
  
  StmtDDLCreateView();  // DO NOT USE

  //
  // assignment operator
  //

  StmtDDLCreateView & operator==(const StmtDDLCreateView &rhs);  // DO NOT USE 
  
  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------

  QualifiedName viewQualName_;

  //
  // information extracted from View Column List parse sub-tree
  //

  ElemDDLColViewDefArray columnDefArray_;

  //
  // information extracted from the optional LOCATION clause
  //

  NABoolean isLocationClauseSpec_;
  NAString locationName_;
  ElemDDLLocation::locationNameTypeEnum locationNameType_;

  ComCreateViewBehavior createViewBehavior_;

  //
  // information derived from With Check Option (child) parse node
  //

  ComLevels checkOptionLevel_;

  //
  // information about the position of the name within the input
  // string (to help with computing the view text)
  //

  ParNameLocList nameLocList_;
  StringPos startPos_;
  StringPos endPos_;

  //
  // Updatable/insertable flags, set by Bind/Transform step
  //

  NABoolean isUpdatable_;
  NABoolean isInsertable_;

  //
  // view usages information
  //

  ParViewUsages viewUsages_;

  //
  // pointers to child parse nodes
  //
 
  ElemDDLNode * pViewColumnList_;
  ElemDDLNode * pLocationClause_;
  RelExpr     * pQueryExpression_;
  ElemDDLNode * pWithCheckOption_;

  ElemDDLGrantee * pOwner_;

  // create if view does not exist. Otherwise just return.
  NABoolean createIfNotExists_;

  // All UDF's referenced in a statement
  // --------------------------------------------------------------------
  LIST(OptUDFInfo *) udfList_;

}; // class StmtDDLCreateView


// -----------------------------------------------------------------------
// definitions of inline methods for class ParViewTableColsUsage
// -----------------------------------------------------------------------

//
// accessors
//

inline const NAString &
ParViewTableColsUsage::getColumnName() const
{
  return columnName_;
}

inline const ExtendedQualName &
ParViewTableColsUsage::getTableQualName() const
{
  return tableName_;
}

//
// mutators
//

inline void
ParViewTableColsUsage::setColumnName(const NAString &colName)
{
  columnName_ = colName;
}

inline void
ParViewTableColsUsage::setTableName(const ExtendedQualName &tableQualName)
{
  tableName_ = tableQualName;
}

// -----------------------------------------------------------------------
// definitions of inline methods for class ParViewTableColsUsageList
// -----------------------------------------------------------------------

//
// operators
//

inline const ParViewTableColsUsage &
ParViewTableColsUsageList::operator[](CollIndex index) const
{
  return *(LIST(ParViewTableColsUsage *)::operator[](index));
}

inline ParViewTableColsUsage &
ParViewTableColsUsageList::operator[](CollIndex index)
{
  return *(LIST(ParViewTableColsUsage *)::operator[](index));
}

//
// accessors
//

inline CollIndex
ParViewTableColsUsageList::entries() const
{
  return LIST(ParViewTableColsUsage *)::entries();
}

inline const ParViewTableColsUsage * const
ParViewTableColsUsageList::find(const ColRefName &colName) const
{
  return ((ParViewTableColsUsageList *)this)->find(colName);
}

// -----------------------------------------------------------------------
// definitions of inline methods for class ViewColTablesUsage
// -----------------------------------------------------------------------

//
// accessors
//

inline const CollIndex
ParViewColTablesUsage::getUsingViewColumnNumber() const
{
  return usingViewColumnNumber_;
}

inline const ExtendedQualName &
ParViewColTablesUsage::getUsedObjectName() const
{
  return usedObjectName_;
}

//
// mutators
//

inline void
ParViewColTablesUsage::setUsingViewColumnNumber(const CollIndex usingColNum)
{
  usingViewColumnNumber_ = usingColNum;
}

inline void
ParViewColTablesUsage::setUsedObjectName(const ExtendedQualName &usedObjName)
{
  usedObjectName_ = usedObjName;
}

// -----------------------------------------------------------------------
// definitions of inline methods for class ParViewColTablesUsageList
// -----------------------------------------------------------------------

inline CollIndex
ParViewColTablesUsageList::entries() const
{
  return LIST(ParViewColTablesUsage *)::entries();
}

inline const ParViewColTablesUsage &
ParViewColTablesUsageList::operator[](CollIndex index) const
{
  return *(LIST(ParViewColTablesUsage *)::operator[](index));
}

inline ParViewColTablesUsage &
ParViewColTablesUsageList::operator[](CollIndex index)
{
  return *(LIST(ParViewColTablesUsage *)::operator[](index));
}

// -----------------------------------------------------------------------
// definitions of inline methods for class ViewColTableColsUsage
// -----------------------------------------------------------------------

//
// accessors
//

inline const CollIndex
ParViewColTableColsUsage::getUsingViewColumnNumber() const
{
  return usingViewColumnNumber_;
}

inline const ColRefName &
ParViewColTableColsUsage::getUsedObjectColumnName() const
{
  return usedObjectColumnName_;
}

//
// mutators
//

inline void
ParViewColTableColsUsage::setUsingViewColumnNumber(const CollIndex colNum)
{
  usingViewColumnNumber_ = colNum;
}

inline void
ParViewColTableColsUsage::setUsedObjectColumnName(const ColRefName &colRefName)
{
  usedObjectColumnName_ = colRefName;
}

// -----------------------------------------------------------------------
// definitions of inline methods for class ParViewColTableColsUsageList
// -----------------------------------------------------------------------

//
// operators
//

inline const ParViewColTableColsUsage &
ParViewColTableColsUsageList::operator[](CollIndex index) const
{
  return *(LIST(ParViewColTableColsUsage *)::operator[](index));
}

inline ParViewColTableColsUsage &
ParViewColTableColsUsageList::operator[](CollIndex index)
{
  return *(LIST(ParViewColTableColsUsage *)::operator[](index));
}

//
// accessors
//

inline CollIndex
ParViewColTableColsUsageList::entries() const
{
  return LIST(ParViewColTableColsUsage *)::entries();
}

inline const ParViewColTableColsUsage * const
ParViewColTableColsUsageList::find(const CollIndex usingViewColNum) const
{
  return ((ParViewColTableColsUsageList *)this)->find(usingViewColNum);
}

inline const ParViewColTableColsUsage * const
ParViewColTableColsUsageList::find(const CollIndex usingViewColNum,
                                   const ColRefName &usedColRefName) const
{
  return ((ParViewColTableColsUsageList *)this)->find(usingViewColNum,
                                                      usedColRefName);
}

// -----------------------------------------------------------------------
// definitions of inline methods for class ParViewUsages
// -----------------------------------------------------------------------

//
// accessors
//

inline CollIndex
ParViewUsages::getCurViewColNum() const
{
  return curViewColNum_;
}

inline const ExtendedQualName *
ParViewUsages::getUsedTableNamePtr(const ExtendedQualName &tableName) const
{
  return usedTableNameList_.find(tableName);
}

inline const ParViewTableColsUsageList &
ParViewUsages::getViewTableColsUsageList() const
{
  return usedColRefList_;
}

inline ParViewTableColsUsageList &
ParViewUsages::getViewTableColsUsageList()
{
  return usedColRefList_;
}

inline const ParTableUsageList &
ParViewUsages::getViewTableUsageList() const
{
  return usedTableNameList_;
}

inline ParTableUsageList &
ParViewUsages::getViewTableUsageList()
{
  return usedTableNameList_;
}

inline const ParViewColTablesUsageList &
ParViewUsages::getViewColTablesUsageList() const
{
  return viewColTablesUsageList_;
}

inline ParViewColTablesUsageList &
ParViewUsages::getViewColTablesUsageList()
{
  return viewColTablesUsageList_;
}


#if 0  // KSKSKS

inline const ParViewColTableColsUsageList &
ParViewUsages::getViewColTableColsUsageList() const
{
  return viewColTableColsUsageList_;
}

inline ParViewColTableColsUsageList &
ParViewUsages::getViewColTableColsUsageList()
{
  return viewColTableColsUsageList_;
}

#endif  // KSKSKS


inline NABoolean
ParViewUsages::isItmColRefInColInRowVals() const
{
  return isItmColRefInColInRowVals_;
}

inline NABoolean
ParViewUsages::isViewSurelyNotUpdatable() const
{
  return isViewSurelyNotUpdatable_;
}

//
// mutators
//

inline void
ParViewUsages::setCurViewColNum(const CollIndex curVwCol)
{
  curViewColNum_ = curVwCol;
}

inline void
ParViewUsages::setItmColRefInColInRowValsFlag(const NABoolean isItmColRef)
{
  isItmColRefInColInRowVals_ = isItmColRef;
}

inline void
ParViewUsages::setViewIsSurelyNotUpdatableFlag()
{
  isViewSurelyNotUpdatable_ = TRUE;
}

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLCreateView
// -----------------------------------------------------------------------

//
// accessors
//

inline QualifiedName &
StmtDDLCreateView::getViewNameAsQualifiedName()
{
  return viewQualName_;
}

inline const QualifiedName &
StmtDDLCreateView::getViewNameAsQualifiedName() const
{
  return viewQualName_;
}

inline ComLevels
StmtDDLCreateView::getCheckOptionLevel() const
{
  return checkOptionLevel_;
}

inline CollIndex
StmtDDLCreateView::getCurViewColNum() const
{
  return getViewUsages().getCurViewColNum();
}

inline const StringPos
StmtDDLCreateView::getEndPosition() const
{
  return endPos_;
}

inline const NAString &
StmtDDLCreateView::getLocation() const
{
  return locationName_;
}

inline ElemDDLLocation::locationNameTypeEnum
StmtDDLCreateView::getLocationNameType() const
{
  return locationNameType_;
}

inline const ParNameLocList &
StmtDDLCreateView::getNameLocList() const
{
  return nameLocList_;
}

inline ParNameLocList &
StmtDDLCreateView::getNameLocList()
{
  return nameLocList_;
}

inline const NABoolean 
StmtDDLCreateView::isCreateView() const
{
  if(createViewBehavior_ == COM_CREATE_VIEW_BEHAVIOR)
    return TRUE;
  else 
    return FALSE;
}

inline const NABoolean 
StmtDDLCreateView::isCreateSystemView() const
{
  if(createViewBehavior_ == COM_CREATE_SYSTEM_VIEW_BEHAVIOR)
    return TRUE;
  else 
    return FALSE;

}

inline const NABoolean 
StmtDDLCreateView::isCreateOrReplaceView() const
{
  if(createViewBehavior_ == COM_CREATE_OR_REPLACE_VIEW_BEHAVIOR)
    return TRUE;
  else 
    return FALSE;

}

inline const NABoolean 
StmtDDLCreateView::isCreateOrReplaceViewCascade() const
{
  if(createViewBehavior_ == COM_CREATE_OR_REPLACE_VIEW_CASCADE_BEHAVIOR)
    return TRUE;
  else 
    return FALSE;

}

inline const ComCreateViewBehavior
StmtDDLCreateView::getCreateViewBehavior() const
{
  return createViewBehavior_;
}

inline const RelExpr *
StmtDDLCreateView::getQueryExpression() const
{
  return pQueryExpression_;
}

inline RelExpr *
StmtDDLCreateView::getQueryExpression()
{
  return pQueryExpression_;
}

inline const StringPos
StmtDDLCreateView::getStartPosition() const
{
  return startPos_;
}

inline const ElemDDLColViewDefArray &
StmtDDLCreateView::getViewColDefArray() const
{
  return columnDefArray_;
}

inline ElemDDLColViewDefArray &
StmtDDLCreateView::getViewColDefArray()
{
  return columnDefArray_;
}

inline const NAString
StmtDDLCreateView::getViewName() const
{
  return viewQualName_.getQualifiedNameAsAnsiString();
}

inline const ParViewUsages &
StmtDDLCreateView::getViewUsages() const
{
  return viewUsages_;
}

inline ParViewUsages &
StmtDDLCreateView::getViewUsages()
{
  return viewUsages_;
}

inline const LIST(OptUDFInfo *) &
StmtDDLCreateView::getUDFList() const
{
  return udfList_;
}

inline LIST(OptUDFInfo *) &
StmtDDLCreateView::getUDFList()
{
  return udfList_;
}

inline NABoolean
StmtDDLCreateView::isItmColRefInColInRowVals() const
{
  return getViewUsages().isItmColRefInColInRowVals();
}

// is location clause/phrase specified?
inline NABoolean
StmtDDLCreateView::isLocationSpecified() const
{
  return isLocationClauseSpec_;
}

inline NABoolean
StmtDDLCreateView::isProcessingViewColList() const
{
  return getCurViewColNum() NEQ NULL_COLL_INDEX;
}

inline NABoolean
StmtDDLCreateView::isWithCheckOptionSpecified() const
{
  if (pWithCheckOption_ NEQ NULL)
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

inline NABoolean
StmtDDLCreateView::isOwnerSpecified() const
{
  return pOwner_ ? TRUE : FALSE;
}

inline const ElemDDLGrantee *
StmtDDLCreateView::getOwner() const
{
  return pOwner_;
} 

//
// mutators
//

inline void
StmtDDLCreateView::resetCurViewColNum()
{
  getViewUsages().setCurViewColNum(NULL_COLL_INDEX);
}

inline void
StmtDDLCreateView::setCurViewColNum(const CollIndex curVwCol)
{
  getViewUsages().setCurViewColNum(curVwCol);
}

inline void
StmtDDLCreateView::setEndPosition(const StringPos endPos)
{
  endPos_ = endPos;
}

inline void
StmtDDLCreateView::setItmColRefInColInRowValsFlag(const NABoolean isItmColRef)
{
  getViewUsages().setItmColRefInColInRowValsFlag(isItmColRef);
}

inline void
StmtDDLCreateView::setStartPosition(const StringPos startPos)
{
  startPos_ = startPos;
}

#endif // STMTDDLCREATEVIEW_H
