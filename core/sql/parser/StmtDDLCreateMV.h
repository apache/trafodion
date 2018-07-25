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
#ifndef STMTDDLCREATEMV_H
#define STMTDDLCREATEMV_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLCreateMV.h
 * Description:  class for parse nodes representing Create Materialized View
 *               statements
 *
 * Created:      6/11/99
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
#include "NABoolean.h"
#include "ObjectNames.h"
#include "ElemDDLColViewDefArray.h"
#include "ElemDDLDivisionClause.h"
#include "ParNameLocList.h"
#include "ParTableUsageList.h"
#include "RelExpr.h"
#include "StmtDDLNode.h"
#include "MVInfo.h"
#include "StmtDDLCreateView.h" // for ParViewUsages
#include "ElemDDLCreateMVOneAttributeTableList.h"
//#include "..\catman\CatExecCreateMV.h"

// MV Initialization types
enum MvInitializationType {
  MVINIT_EMPTY = 0,
  MVINIT_ON_CREATE,
  MVINIT_ON_REFRESH,
  MVINIT_NO_INIT,
  MVINIT_BY_USER
};

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLCreateMV;

// -----------------------------------------------------------------------
// forward declerations
// -----------------------------------------------------------------------
class CatExecCreateMV;
// -----------------------------------------------------------------------
// Create Materialized View statement
// -----------------------------------------------------------------------
class StmtDDLCreateMV : public StmtDDLNode
{

public:

  // constructor
  StmtDDLCreateMV(const QualifiedName  & MVQualName,
		  const ParNameLocList & nameLocList,
		  ElemDDLNode          * optionalMVColumnList,
		  ComMVRefreshType     refreshType,
		  ElemDDLNode		   * pAttributeTableLists, 
//		  ComMVStatus          mvStatus,
                  MvInitializationType initType,
		  ComBoolean           isRewriteEnabled,
		  ElemDDLNode          * optionalFileOptionsClause,
		  RelExpr              * queryExpression,
                  ElemDDLNode          * pOptionalOwner,
		  CollHeap             * heap = PARSERHEAP());

  // virtual destructor
  virtual ~StmtDDLCreateMV();

  // cast
  virtual StmtDDLCreateMV * castToStmtDDLCreateMV();

  // pointers to child parse nodes (getChild and setChild methods)
  enum { INDEX_MV_COLUMN_LIST = 0,
	     MV_COLUMN_LIST = INDEX_MV_COLUMN_LIST,
         INDEX_FILE_OPTIONS_CLAUSE,
	     FILE_OPTIONS_CLAUSE = INDEX_FILE_OPTIONS_CLAUSE,
         INDEX_QUERY_EXPRESSION,
	     QUERY_EXPRESSION = INDEX_QUERY_EXPRESSION,
         MAX_STMT_DDL_CREATE_MV_ARITY };

	friend class CatExecCreateMV;

  //
  // accessors
  //

  // Return the internal MVInfo object
  inline  MVInfoForDDL * getMVInfo() { return theMVInfo_ ; }

  virtual Int32 getArity() const;

  virtual ExprNode * getChild(Lng32 index);

  inline CollIndex getCurViewColNum() const;  // for Binder use only

  inline const ParNameLocList & getNameLocList() const;   // XXX
  inline       ParNameLocList & getNameLocList();         // XXX

  // returns the UDF usage list
  inline const LIST(OptUDFInfo *) &getUDFList() const;
  inline       LIST(OptUDFInfo *) &getUDFList();

  // returns a list of locations of names appearing in
  // the statement input string.  The list helps with
  // the computing of the MV text.

  inline ElemDDLNode	  * getOptionalColumnNames();
  inline ElemDDLNode	  * getFileOptions();
  inline ElemDDLStoreOptKeyColumnList  * getStoreByOption() const;

  // returns the pointer pointing to the (optional) sub-tree
  // for the file options

  inline const RelExpr * getQueryExpression() const;
  inline       RelExpr * getQueryExpression();

  // returns the pointer pointing to the parse sub-tree
  // representing the query expression in the MV definition.
  
  // ------------------------------------------------------------------
  // Get (internally stored) various positions within the input string
  // ------------------------------------------------------------------
  inline const StringPos getEndPosition() const;

        // returns the ending position (the position of the
        // last character) of the statement (within the input
        // string)
  
  inline const StringPos getStartPosition() const;

        // returns the starting position (the position of the
        // first character) of the statement (within the
        // input string)
  
  inline const StringPos getEndOfOptionalColumnListPosition() const;

        // returns the end position (the position of the "after the last"
        // character) of the "optional MV column list" clause (within the
        // input string)

  inline const StringPos getStartOfFileOptionsPosition() const;
  inline const StringPos getEndOfFileOptionsPosition() const;

        // returns the start/end positions of the "optional file options" 
        // clause (within the input string)

  inline const StringPos getEndOfSelectColumnListPosition() const;

        // returns the end position (the position of the "after the last"
        // character) of the "select column list" of the MV stmt (within
        // the input string)

  inline const StringPos getStartOfMVQueryPosition() const;

        // returns the start position of the MV query (just before SELECT)
        // (within the input string)

  // ------------------------------------------------------------------

  inline const ElemDDLColViewDefArray & getMVColDefArray() const; // XXX
  inline       ElemDDLColViewDefArray & getMVColDefArray();       // XXX

        // returns an array of pointers pointing to the Column Name parse
        // nodes.  If the Optional MV Column Name List phrase is not
        // specified in the Create View statement, returns an empty array.

  inline const NAString getMVName() const;

        // returns the name of the defined MV.

  inline const QualifiedName & getMVNameAsQualifiedName() const;
  inline       QualifiedName & getMVNameAsQualifiedName();

        // returns the name of the defined MV in QualifiedName format

  inline const ParViewUsages & getViewUsages() const;
  inline       ParViewUsages & getViewUsages();

        // returns the view usages information after the parse
        // node is bound; returns an empty object otherwise.

  ElemDDLCreateMVOneAttributeTableList * getIgnoreChangesList() 
										{ return pIgnoreChangesList_; }

  
  inline NABoolean isItmColRefInColInRowVals() const;  // for Binder use only

  inline NABoolean isProcessingViewColList() const;

        // This method is used during the Binding phase to help
        // with collecting view column usage information

  ComMvAuditType getMvAuditType() {return mvAuditType_;}
  ULng32 getCommitEachNRows() {return commitEachNRows_;}

  ComBoolean getIsStoreByOptionSpecified() {return isStoreByClauseSpecified_;}

  inline NABoolean isOwnerSpecified() const;

        // returns TRUE if the BY <owner> phrase appears
        // in the Create statement; returns FALSE otherwise.

  inline const ElemDDLGrantee *getOwner() const;
        // returns pointer to the optional "by owner"
        // (in the form of an ElemDDLGrantee).

  inline NABoolean isDivisionClauseSpecified() const;

        // returns TRUE if the DIVISION BY clause appears;
        // returns FALSE otherwise.

  inline ElemDDLDivisionClause::divisionTypeEnum getDivisionType() const;
  inline ItemExprList * getDivisionExprList();
  inline ElemDDLDivisionClause * getDivisionClause();

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

  inline void setEndOptionalColumnListPosition(const StringPos endPos);

        // sets the end position (the position of the "after the last"
        // character) of the "optional view column list" clause (within the
        // input string)

  inline void setFileOptionsPositions(const StringPos startPos,
				      const StringPos endPos);

        // sets the start + end positions of the "optional file options" 
        // clause (within the input string)

  inline void setEndSelectColumnListPosition(const StringPos endPos);

        // sets the end position (the position of the "after the last"
        // character) of the "select column list" clause (within the
        // input string)

  inline void setStartOfMVQueryPosition(const StringPos startPos);

        // sets the start position of the MV query (within the input string)

  inline const ElemDDLColRefArray & getPartitionKeyColRefArray() const;
  inline       ElemDDLColRefArray & getPartitionKeyColRefArray();
        // get the MV partition columns specified by the user

  //
  // method for binding
  //
  
	ExprNode * bindNode(BindWA *bindWAPtr);

	// method for collecting information
	void synthesize();

	// collects information in the parse sub-tree and
	// copy/move them to the current parse node.

	// CHANGES CLAUSE
	void processAttributeTableLists();
	NABoolean statementHasAttributeTableLists();

	NABoolean hasIgnoreChangesList() {return !!pIgnoreChangesList_;}

	NABoolean isPartitionByClauseSpecified () { return isPartitionByClauseSpecified_; }
        NABoolean isPartitionDefinitionSpecified () { return isPartitionDefinitionSpecified_; }

	// check FILE OPTIONS sub tree. 
	void ensureCorrectMVFileOptions();
	void checkFileOption(ElemDDLNode * pTableOption);
	void checkStoreByClause(ElemDDLStoreOpt*);
	void checkFileAttributeClause(ElemDDLFileAttrClause*);
	void checkFileAttribute(ElemDDLFileAttr*);
	void checkLocationClause(ElemDDLLocation*);
	void checkDivisionByClause(ElemDDLDivisionClause *);

	void checkPartitionDefinitionclause(ElemDDLPartitionClause*);
	void checkMVFileAttributeClause(ElemDDLMVFileAttrClause *);

	void extractMvAttributesFromFileOptions();

  //
  // methods for tracing
  //
  
  virtual const NAString displayLabel1() const;
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;

  virtual NABoolean explainSupported() { return TRUE; }

private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  //
  // default constructor
  //
  
  StmtDDLCreateMV();  // DO NOT USE

  //
  // assignment operator
  //

  StmtDDLCreateMV & operator==(const StmtDDLCreateMV &rhs);  // DO NOT USE 
  
  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------

  QualifiedName MVQualName_;

  ComMVRefreshType refreshType_;
  ElemDDLNode * pAttributeTableLists_;
  ElemDDLCreateMVOneAttributeTableList * pIgnoreChangesList_;

  //
  // information extracted from View Column List parse sub-tree
  //

  ElemDDLColViewDefArray columnDefArray_;
  
  //
  // information about the position of the name within the input
  // string (to help with computing the view text)
  //

  ParNameLocList nameLocList_;
  StringPos startPos_;
  StringPos endPos_;

  StringPos endOfOptionalColumnList_;
  StringPos startOfFileOptions_;
  StringPos endOfFileOptions_;
  StringPos startOfMVQuery_;
  StringPos endOfSelectColumnList_;

  //
  //  The MVInfo internal object
  //
  MVInfoForDDL * theMVInfo_ ;

  //
  // view usages information
  //

  ParViewUsages viewUsages_;

  //
  // indicators for indicating a clause has already been specified
  //
  
  NABoolean isAttrClauseSpecified_;
  NABoolean isLocationClauseSpecified_; 
  NABoolean isDivisionByClauseSpecified_;
  ElemDDLDivisionClause * pDivisionByClauseParseNode_;
  NABoolean isPartitionDefinitionSpecified_;     // for any HASH(2) PARTITION clauses
  NABoolean isPartitionByClauseSpecified_;       // for HASH(2) PARTITION BY clause
  NABoolean isStoreByClauseSpecified_; 
  NABoolean	isMVAttributeClauseSpecified_;

  // MVATTRIBUTES
  ComMvAuditType mvAuditType_;
  ULng32 commitEachNRows_;
  
  //
  // pointers to child parse nodes
  //
 
  ElemDDLNode			*pMVColumnList_;
  ElemDDLNode			*pFileOptions_;
  ElemDDLStoreOptKeyColumnList	*pStoreByOption_;
  RelExpr			*pQueryExpression_;

  // PARTITION BY clause
  ElemDDLColRefArray             partitionKeyColRefArray_;

  ElemDDLGrantee * pOwner_;

  // --------------------------------------------------------------------
  // All UDF's referenced in a statement
  // --------------------------------------------------------------------
  LIST(OptUDFInfo *) udfList_;

}; // class StmtDDLCreateMV


// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLCreateMV
// -----------------------------------------------------------------------

//
// accessors
//

inline QualifiedName &
StmtDDLCreateMV::getMVNameAsQualifiedName()
{
  return MVQualName_;
}

inline const QualifiedName & 
StmtDDLCreateMV::getMVNameAsQualifiedName() const 
{
  return MVQualName_;
}

inline CollIndex
StmtDDLCreateMV::getCurViewColNum() const
{
  return getViewUsages().getCurViewColNum();
}

inline const StringPos
StmtDDLCreateMV::getEndPosition() const
{
  return endPos_;
}

inline const ParNameLocList &
StmtDDLCreateMV::getNameLocList() const
{
  return nameLocList_;
}

inline ParNameLocList &
StmtDDLCreateMV::getNameLocList()
{
  return nameLocList_;
}


inline  ElemDDLNode * 
StmtDDLCreateMV::getOptionalColumnNames()
{
  return pMVColumnList_;
}

inline  ElemDDLNode * 
StmtDDLCreateMV::getFileOptions()
{
  return pFileOptions_;
}

inline ElemDDLStoreOptKeyColumnList * 
StmtDDLCreateMV::getStoreByOption() const
{
  return pStoreByOption_;
}


inline const RelExpr *
StmtDDLCreateMV::getQueryExpression() const
{
  return pQueryExpression_;
}

inline RelExpr *
StmtDDLCreateMV::getQueryExpression()
{
  return pQueryExpression_;
}

inline const StringPos
StmtDDLCreateMV::getStartPosition() const
{
  return startPos_;
}

inline const ElemDDLColViewDefArray &
StmtDDLCreateMV::getMVColDefArray() const
{
  return columnDefArray_;
}

inline ElemDDLColViewDefArray &
StmtDDLCreateMV::getMVColDefArray()
{
  return columnDefArray_;
}

inline const NAString
StmtDDLCreateMV::getMVName() const
{
  return MVQualName_.getQualifiedNameAsAnsiString();
}

inline const ParViewUsages &
StmtDDLCreateMV::getViewUsages() const
{
  return viewUsages_;
}

inline ParViewUsages &
StmtDDLCreateMV::getViewUsages()
{
  return viewUsages_;
}

inline const LIST(OptUDFInfo *) &
StmtDDLCreateMV::getUDFList() const
{
  return udfList_;
}

inline LIST(OptUDFInfo *) &
StmtDDLCreateMV::getUDFList()
{
  return udfList_;
}

inline NABoolean
StmtDDLCreateMV::isItmColRefInColInRowVals() const
{
  return getViewUsages().isItmColRefInColInRowVals();
}

inline NABoolean
StmtDDLCreateMV::isProcessingViewColList() const
{
  return getCurViewColNum() NEQ NULL_COLL_INDEX;
}

//
// mutators
//

inline void
StmtDDLCreateMV::resetCurViewColNum()
{
  getViewUsages().setCurViewColNum(NULL_COLL_INDEX);
}

inline void
StmtDDLCreateMV::setCurViewColNum(const CollIndex curVwCol)
{
  getViewUsages().setCurViewColNum(curVwCol);
}

inline void
StmtDDLCreateMV::setEndPosition(const StringPos endPos)
{
  endPos_ = endPos;
}

inline void
StmtDDLCreateMV::setItmColRefInColInRowValsFlag(const NABoolean isItmColRef)
{
  getViewUsages().setItmColRefInColInRowValsFlag(isItmColRef);
}
inline void
StmtDDLCreateMV::setStartPosition(const StringPos startPos)
{
  startPos_ = startPos;
}
inline 
const StringPos StmtDDLCreateMV::getEndOfOptionalColumnListPosition() const
{
  return endOfOptionalColumnList_;
}
inline 
const StringPos StmtDDLCreateMV::getEndOfSelectColumnListPosition() const
{
  return endOfSelectColumnList_;
}
inline 
const StringPos StmtDDLCreateMV::getStartOfMVQueryPosition() const
{
  return startOfMVQuery_;
}
inline const StringPos StmtDDLCreateMV::getStartOfFileOptionsPosition() const
{
  return startOfFileOptions_;
}
inline const StringPos StmtDDLCreateMV::getEndOfFileOptionsPosition() const
{
  return endOfFileOptions_;
}


inline void 
StmtDDLCreateMV::setEndOptionalColumnListPosition(const StringPos endPos)
{
  endOfOptionalColumnList_ = endPos;
}

inline void 
StmtDDLCreateMV::setFileOptionsPositions(const StringPos startPos,
					 const StringPos endPos)
{
  startOfFileOptions_ = startPos;
  endOfFileOptions_ = endPos;
}

inline void 
StmtDDLCreateMV::setEndSelectColumnListPosition(const StringPos endPos)
{
  endOfSelectColumnList_ = endPos;
}
inline void 
StmtDDLCreateMV::setStartOfMVQueryPosition(const StringPos startPos)
{
  startOfMVQuery_ = startPos;
}

// get column name list in partition by clause
inline const ElemDDLColRefArray &
StmtDDLCreateMV::getPartitionKeyColRefArray() const
{
  return partitionKeyColRefArray_;
}

// get column name list in partition by clause
inline ElemDDLColRefArray &
StmtDDLCreateMV::getPartitionKeyColRefArray()
{
  return partitionKeyColRefArray_;
}

inline NABoolean
StmtDDLCreateMV::isOwnerSpecified() const
{
  return pOwner_ ? TRUE : FALSE;
}

inline const ElemDDLGrantee *
StmtDDLCreateMV::getOwner() const
{
  return pOwner_;
}

inline NABoolean StmtDDLCreateMV::isDivisionClauseSpecified() const
{
  return isDivisionByClauseSpecified_;
}

inline ElemDDLDivisionClause::divisionTypeEnum StmtDDLCreateMV::getDivisionType() const
{
  return ( pDivisionByClauseParseNode_ EQU NULL
           ? ElemDDLDivisionClause::UNKNOWN_DIVISION_TYPE
           : pDivisionByClauseParseNode_->getDivisionType() );
}

inline ElemDDLDivisionClause * StmtDDLCreateMV::getDivisionClause()
{
  return pDivisionByClauseParseNode_;
}

inline ItemExprList * StmtDDLCreateMV::getDivisionExprList()
{
  return ( pDivisionByClauseParseNode_ EQU NULL
           ? NULL
           : pDivisionByClauseParseNode_->getDivisionExprList() );
}

#endif // STMTDDLCREATEMV_H
