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
* File:         BindItemExpr.C
* Description:  Item expressions (binder-related methods)
* Created:      09/31/94
* Language:     C++
*
*
* 	Canst thou bind the sweet influences of Pleiades,
* 	or loose the bands of Orion?
*		    -- Job 38:31
*
******************************************************************************
*/

#define  SQLPARSERGLOBALS_FLAGS	// must precede all #include's
#define  SQLPARSERGLOBALS_NADEFAULTS

#include "Platform.h"
#include "NAWinNT.h"
#include "nawstring.h"
#include "AllItemExpr.h"
#include "BindWA.h"
#include "ComOperators.h"
#include "GroupAttr.h"			// QSTUFF
#include "parser.h"
#include "ParNameLocList.h"
#include "Rel3GL.h"
#include "RelMisc.h"
#include "RelScan.h"
#include "RelUpdate.h"
#include "Sqlcomp.h"
#include "StmtDDLAddConstraintCheck.h"
#include "StmtDDLCreateTrigger.h"
#include "StmtDDLCreateView.h"
#include "StmtDDLCreateMV.h"
#include "ex_error.h"
#include "exp_like.h"
#include "ItemColRef.h"
#include "TriggerDB.h"
#include "UdrErrors.h"
#include "nchar_mp.h"
#include "CmpStatement.h"
#include "CmpCommon.h"
#include "Analyzer.h"
#include "ControlDB.h"
#include "OptimizerSimulator.h"
#include "RelSequence.h"
#include "ItemSample.h"
#include "SqlParserGlobals.h"
#include "RelSequence.h"
#include "ComSqlId.h"

#include "ItemFuncUDF.h"
#include "CmpSeabaseDDL.h"
#include "QCache.h"

#include "TrafDDLdesc.h"
#include "exp_datetime.h"

#include <stack>


// defined in SynthType.cpp
extern
void emitDyadicTypeSQLnameMsg(Lng32 sqlCode,
			      const NAType &op1,
			      const NAType &op2,
			      const char *str1 = NULL,
			      const char *str2 = NULL,
			      ComDiagsArea * diagsArea = NULL,
                              const Lng32 int1 = -999999);

// defined in parser/SqlParserAux.h
extern
ItemExpr *literalOfDate(NAString *strptr, NABoolean noDealloc = FALSE);

#define BINDITEMEXPR_STMTCHARSET CharInfo::UTF8

// -----------------------------------------------------------------------
// utility functions
// -----------------------------------------------------------------------

inline static ValueId createValueDesc(BindWA *bindWA,
				      ItemExpr *expr,
				      const NAType *type)
{
  // See CMPASSERT in RelRoot::bindNode --
  // put there, instead of here, for performance (assert only once per query).
  return ValueDesc::create(expr, type, bindWA->wHeap());
}

// Work for Genesis 10-971028-7413 (Ansi 7.1 SR 1, plus Ansi 8 "predicates").
//
// According to Ansi, the keyword NULL in a value expression (in DML)
// may appear *only* in:
//  - the VALUES row-val-ctor(s) of an INSERT
//  - a result of a CASE
//  - operand 1 of a CAST
// As extensions, since they're harmless and possibly useful,
// we also allow the NULL kwd to appear:
//  - as a select-list item, e.g. SELECT col1,NULL,col2 FROM tbl;
//  - in a IS [NOT] NULL predicate, e.g. WHERE NULL IS NULL;
//  - in any non-Ansi function, e.g. EXPLAIN
//  - in various (many) internal functions
//
// Here (throughout BindItemExpr), we explicitly disallow the NULL kwd in:
//  - comparison predicates (BiRelat), e.g. col>=NULL, col IN (x,NULL,y)
//  - quantified comparison predicates e.g. col>=ANY(subqry), NULL IN (subqry)
//  - IN predicates (e.g. see above two lines!)
//  - BETWEEN and LIKE predicates (see ItemFunc.h ctors, and Function::bindNode)
//  - all Ansi functions, including aggregates, e.g. UPPER(NULL), SUM(NULL)
//  - arithmetic operations, e.g. NULL*col, -NULL
//
// **Note** that when we support the MATCH and OVERLAPS predicates,
// they'll need to disallow the kwd too (emulate BETWEEN and LIKE,
// their ::allowsSQLnullArg, ::isAPredicate, and ::bindNode methods).
//
//
// The keyword DEFAULT is even more restricted in Ansi syntax, so the mechanism
// for disallowing it is different and simpler:
// it's by default disallowed everywhere except where explicitly enabled
// via binder context and Insert node context -- in an INSERT VALUES list only.
//
// According to Ansi,
//   INSERT INTO t(a,b,c) VALUES(DEFAULT,DEFAULT+1,SUBSTRING(DEFAULT FROM 2))
// is not legal (DEFAULT for column A is, but not in the other 2 expressions),
// but as a Tandem extension we allow arithmetic and functions.
// (Note that RelRoot will catch ..VALUES(..SUM(DEFAULT).. with err 4015.)
//
enum errSQLnullChild
       { FUNCTION_ = -4097, ARITH_ = -4098, PREDICATE_ = -4099,
         DEFAULT_SPEC_ = -4096,		  // see DefaultSpecification::bindNode
    	 DEFAULT_BOUND_AS_NULL_ = -4095,  // see ItemExpr::bindNode
	 CHILDREN_OK_ = 0
       };

static errSQLnullChild checkForSQLnullChild(const ItemExpr *ie,
					    NABoolean SQLnullIsLegal,
					    errSQLnullChild err)
{
  // Shallow (nonrecursive) testing of children,
  // except for ItemLists whose arity 2 really implements an n-ary list.
  Int32 arity = ie->getArity();
  for (Int32 i=0; i<arity; i++) {
    ItemExpr *ieChild = ie->child(i);
    if (!ieChild) break;		// could be optional children eg. STDDEV

    if (ieChild->getOperatorType() == ITM_CONSTANT) {
      if (!SQLnullIsLegal && ((ConstValue *)ieChild)->isNull()) {

	if (err == DEFAULT_BOUND_AS_NULL_ &&
	    ((ConstValue *)ieChild)->isNullWasDefaultSpec())
	  ((ConstValue *)ieChild)->setText("DEFAULT");
        return err;
      }
    }
    else if (ieChild->getOperatorType() == ITM_DEFAULT_SPECIFICATION) {
      if (err == PREDICATE_)		// Tandem extension (see comments)
	return DEFAULT_SPEC_;
    }
    else if (ieChild->getOperatorType() == ITM_ITEM_LIST) {
      errSQLnullChild ret = checkForSQLnullChild(ieChild, SQLnullIsLegal, err);
      if (ret) return ret;
    }
  }
  return CHILDREN_OK_;			// no naughty children
}
static NABoolean checkForSQLnullChild(BindWA *bindWA,
				      ItemExpr *ie,
				      NABoolean SQLnullIsLegal	= FALSE,
				      errSQLnullChild err 	= PREDICATE_,
				      NABoolean isUnaryNegate	= FALSE)
{
  // Some Function classes implement SQL predicates (BETWEEN, LIKE, MATCH, OVERLAPS).
  if (ie->isAPredicate()) err = PREDICATE_;

  err = checkForSQLnullChild(ie, SQLnullIsLegal, err);

  if (err) {
  NAString unparsed(bindWA->wHeap());

    if (isUnaryNegate)			// instead of "0 - NULL"
      unparsed = (err == DEFAULT_SPEC_) ? "(-DEFAULT)" : "(-NULL)";
    else if (ie->getOperatorType() == ITM_POSITION) unparsed = "POSITION or LOCATE";
    else if (ie->getOperatorType() == ITM_SUBSTR)   unparsed = "SUBSTRING";
    else if (ie->getOperatorType() == ITM_TRIM)     unparsed = "TRIM, LTRIM or RTRIM";
    else if (ie->getOperatorType() == ITM_LOWER)    unparsed = "LOWER or LCASE";
    else if (ie->getOperatorType() == ITM_UPPER)    unparsed = "UPPER or UCASE";
    else if (ie->getOperatorType() == ITM_NVL)    unparsed = "NVL";
    else if (ie->getOperatorType() == ITM_QUERYID_EXTRACT)    unparsed = "QUERYID_EXTRACT";
    else {
      ie->unparse(unparsed, DEFAULT_PHASE, USER_FORMAT_DELUXE);
      if (unparsed[(size_t)0] != '(') unparsed = "(" + unparsed + ")";
    }

    // 4095     A DEFAULT which resolves to NULL is not allowed in $0~String0.
    // 4096     A DEFAULT spec is allowed only in an INSERT's VALUES list.
    // 4097-99  A NULL operand cannot appear in func/oper/pred $0~String0.
    *CmpCommon::diags() << DgSqlCode(err) << DgString0(unparsed);
    bindWA->setErrStatus();
    return TRUE;
  }
  return FALSE;
}

// Flatten certain types to simplify (shorten) various if-tests.
static inline OperatorTypeEnum getOperatorTypeFlat(const ItemExpr *ie)
{
  const OperatorType &e = ie->getOperator();
  return e.match(ITM_ANY_CAST) ? ITM_CAST : OperatorTypeEnum(e);
}

// If target requires upshifting, and source is not already upshifted,
// then interpose a new upshift-function node and bind it.
// It is caller's duty to determine if the upshifting is required.
//
// See also Upper::bindNode(), which removes unneeded Upper's.
// ## Future enhancement is to NOT interpose an upshift in these cases:
// ## - source is a CAST from a TIME/TIMESTAMP *in format with no AM/PM marker*,
// ##	as the source then ends up being all digits and punctuation
// ##   so upshift is unnecessary.
// ##   SynthType.cpp should probably set the isUpshifted flag of the CharType..
//
static ItemExpr *applyUpperToSource(BindWA *bindWA, ItemExpr *ie, Int32 srcIndex)
{
  OperatorTypeEnum opSelf = getOperatorTypeFlat(ie);
  CMPASSERT(opSelf == ITM_ASSIGN || ie->getOperator().match(ITM_ANY_CAST));
  ItemExpr *src = ie->child(srcIndex);
  ItemExpr *upSrc;
  OperatorTypeEnum opSrc = getOperatorTypeFlat(src);

  const CharType &ct = (CharType &)src->getValueId().getType();
  if (ct.getTypeQualifier() == NA_CHARACTER_TYPE && !ct.isUpshifted()) {

    if (opSrc == ITM_CONSTANT && (upSrc = ((ConstValue *)src)->toUpper()))
      ie->setChild(srcIndex, upSrc->bindNode(bindWA));
    else {
      CMPASSERT(opSrc != ITM_UPPER);		// otherwise, bug in SynthType
      const NAColumn *nacolSrc =
	src->getValueId().getNAColumn(TRUE/*okIfNotColumn*/);

      if (NOT nacolSrc || NOT nacolSrc->isUpshiftReqd()) {
	if (opSelf == ITM_ASSIGN) {
	  // Genesis 10-980402-1556: uppercase (or NULL) constant is ok
	  Upper *upSrc = new (bindWA->wHeap()) Upper(src);
	  upSrc->allowsSQLnullArg() = TRUE;     // internal UPPER, it's ok
	  ie->setChild(srcIndex, upSrc->bindNode(bindWA));
	}
	else {
	  // Genesis 10-980611-7115:
	  //	CAST(expr AS CHAR(n) UPSHIFT)  =>  UPPER(CAST(expr AS CHAR(n)))
	  CharType &ctSelf = (CharType &)ie->getValueId().getType();
	  ctSelf.setUpshifted(FALSE);
	  Upper *upSrc = new (bindWA->wHeap()) Upper(ie);
	  ie = upSrc->bindNode(bindWA);
	}
      } // !isUpshiftReqd in src
    }
  }

  return ie;
}


// There are two metadata tables containing view-column-usage information,
// VW_COL_USAGE and VW_COL_TBL_COLS.
//
// VW_COL_USAGE is defined in page 472 of the ANSI SQL-92 standard
// as the Information Schema view VIEW_COLUMN_USAGE.
// VW_COL_USAGE contains information about all columns referenced in
// the view definition (excluding the columns in the view column list)
// so supporting it is pretty simple.
//
// VW_COL_TBL_COLS is for internal use by the catalog manager to support
// view column security; it is only used when the view is updatable.
// Supporting VW_COL_TBL_COLS is harder because we need to collect only
// certain (not all) referenced columns relating to each view column.
// Since we don't know whether the view is updatable or not until the
// normalization phase completes, we must blindly collect information for it..
// Once we find out that the view is not updatable, we can throw this
// extra information away.  It's still not clear what information
// needs to be collected and what not.  So the code might still have
// problems, but hopefully it'll be more stable soon.  In certain
// cases the binder can find out if the view is not updatable; for
// example, a view is not updatable if its query expression a natural
// join operation; the binder can stop collecting information for
// the VW_COL_TBL_COLS right away.
//
void
BindUtil_CollectColUsageForCommonCol(BindWA *const bindWA,
                                     const NAString &commonColName,
                                     const RelExpr  *const tableRelExpr,
                                     const ItemExpr *const columnItemExpr)
{
  if (bindWA->getNameLocListPtr() == NULL) return;
  ExprNode *pUsageParseNode = bindWA->getUsageParseNodePtr();
  if (pUsageParseNode == NULL) return;

  QualifiedName tblQualName;

  if (tableRelExpr->getOperatorType() == REL_RENAME_TABLE AND
      ((RenameTable *)tableRelExpr)->isView()) // view
  {
    RenameTable *pRenameTab = (RenameTable *)tableRelExpr;
    tblQualName = pRenameTab->getTableName().getQualifiedNameObj();
  }
  else if (tableRelExpr->getOperatorType() == REL_SCAN)    // table
  {
    Scan *pScan = (Scan*)tableRelExpr;
    tblQualName = pScan->getTableName().getQualifiedNameObj();
    if (NOT tblQualName.fullyExpanded())  // is a renamed table
      return;
  }
  else return;  // does nothing

  ColRefName colRefName(commonColName, tblQualName);

  if (pUsageParseNode->getOperatorType()
      == DDL_ALTER_TABLE_ADD_CONSTRAINT_CHECK)
  {
    StmtDDLAddConstraintCheck &pn = *pUsageParseNode->castToElemDDLNode()->castToStmtDDLAddConstraintCheck();
    ParCheckConstraintColUsageList &cul = pn.getColumnUsageList();
    cul.insert(colRefName,
               bindWA->getCurrentScope()->context()->inSelectList());
  }
  else if (pUsageParseNode->getOperatorType() == DDL_CREATE_VIEW)
  {
    StmtDDLCreateView &cvpn = *pUsageParseNode->castToElemDDLNode()->castToStmtDDLCreateView();
    ParViewTableColsUsageList &vcul = cvpn.getViewUsages().
                                      getViewTableColsUsageList();
    vcul.insert(colRefName);

    // The view is not updatable because it has common columns.
    // This is one of a few cases that the binder knows whether the
    // view is not updatable.  See comments for method
    // ParViewUsages::isViewSurelyNotUpdatable() for more information.
    cvpn.getViewUsages().setViewIsSurelyNotUpdatableFlag();
  }
  return;

} // BindUtil_CollectColUsageForCommonCol()

//
// Returns FALSE if colRefName does not contain adequate information.
// Returns TRUE  if colRefName contains enough information to be added
//                             to a column-usage list.
//
static NABoolean
BindUtil_IsColUsgInfoOk(const ColRefName &colRefName)
{
  if (colRefName.isFabricated())
  {
    //
    // column-usage information for common columns (e.g., in
    // a natural join operation) has already been collected by
    // routine BindUtil_CollectColUsageForCommonCol().
    //
    return FALSE;
  }

  const QualifiedName &qualName = colRefName.getCorrNameObj().
                                  getQualifiedNameObj();

  if (colRefName.getColName().isNull() OR
      qualName.getObjectName().isNull())
  {
    //
    // col name in GroupBy node created by parser
    // for Select Distinct; for example:
    //
    //   create view v as select distinct * from t;
    //
    return FALSE;
  }

  if (qualName.getCatalogName().isNull() OR
      qualName.getSchemaName().isNull())
  {
    //
    // ( query_expr ) corr_name
    //
    // For example:
    //
    //   create view v as select * from (select c from t) x(d);
    //
    return FALSE;
  }

  return TRUE;
}

static NABoolean BindUtil_CollectColumnUsageInfo(BindWA *bindWA,
                                                 const ColRefName &colRefName,
                                                 const ValueId vid,
                                                 RelExpr * parent)
{
  ExprNode *pUsageParseNode = bindWA->getUsageParseNodePtr();
  if (pUsageParseNode == NULL)
  {
    return FALSE;  // no, do not continue to collect usage information
  }

  if (pUsageParseNode->getOperatorType()
      == DDL_ALTER_TABLE_ADD_CONSTRAINT_CHECK)
  {
    StmtDDLAddConstraintCheck &pn = *pUsageParseNode
      ->castToElemDDLNode()->castToStmtDDLAddConstraintCheck();
    ParCheckConstraintColUsageList &cul = pn.getColumnUsageList();
    if (BindUtil_IsColUsgInfoOk(colRefName))
    {
      cul.insert(colRefName,
                 bindWA->getCurrentScope()->context()->inSelectList());
    }
    return TRUE;
  }
  else if (pUsageParseNode->getOperatorType() == DDL_CREATE_VIEW)
  {
    StmtDDLCreateView &cvpn = *bindWA->getCreateViewParseNode();
    ParViewTableColsUsageList &vcul = cvpn.getViewUsages().
                                      getViewTableColsUsageList();

    if (BindUtil_IsColUsgInfoOk(colRefName))
    {
      vcul.insert(colRefName);
    }

    if (cvpn.isProcessingViewColList())
    {
      ParViewColTableColsUsageList &vctcul = cvpn.getViewUsages().
                                             getViewColTableColsUsageList();

      if (NOT colRefName.isFabricated() AND  // com col info already collected
          NOT cvpn.getViewUsages().isViewSurelyNotUpdatable())
      {
        const QualifiedName &qualName = colRefName.getCorrNameObj().
                                        getQualifiedNameObj();

        if (BindUtil_IsColUsgInfoOk(colRefName))
        {
          vctcul.insert(cvpn.getCurViewColNum(), colRefName);
        }
        else if (qualName.getObjectName().isNull() AND
                 NOT colRefName.getCorrNameObj().getCorrNameAsString().isNull()
                 AND NOT colRefName.getColName().isNull() AND
                 cvpn.isItmColRefInColInRowVals() AND
                 parent == cvpn.getQueryExpression())
        {
          // renamed_table.column case
          // for example: x.d in the following statement
          // create view v as select * from (select c from t) x(d);
          // needs to map to t.c (a persistent object)

          // traverses down the parse tree, skipping the RenameTable
          // parse nodes, until reaching a RenameTable parse node for
          // a referenced view or (hopefully) a base table.

          // QSTUFF
          // at this time we know that the view is updatable
          // previously an updatable view ended up to be a RelRoot
          // followed by a sequence of (Rename,RelRoot) pairs.
          // At the bottom of the view we either find a Scan
          // or a "RenameTable" inserted by view expansion.
          // When allowing emdedded generic updates we may see
          // the following combination at the bottom of a
          // view (RenameTable,RelRoot,GenericUpdate). If we
          // encounter this combination we need to skip the
          // GenericUpdate to get to the base table scan or
          // RenameTable inserted by view expansion below
          // the GenericUpdate.
          // RelExpr *result = parent->child(0);  // table in FROM clause

          NABoolean isTopLevelUpdateInView =
             (parent->getGroupAttr()->isEmbeddedUpdate() && (NOT bindWA->inViewExpansion()));

          // in case of a top level update within a view definition we really
          // want the RETDesc of the generic update as it contains both, the
          // valueids of the scan table and the valueids of the update base table.
          RelExpr *result = parent->getViewScanNode(isTopLevelUpdateInView);

          if (result)
          {
            const ValueIdList & updateValueIds =
                              bindWA->getUpdateToScanValueIds().getTopValues();
            const ValueIdList & scanValueIds =
                              bindWA->getUpdateToScanValueIds().getBottomValues();
            ValueId mappedVid = vid;

            for (CollIndex i = 0; i < updateValueIds.entries(); i++){
              if (vid == updateValueIds[i]){
                mappedVid = scanValueIds[i];
                break;
              }
            }

            // gets the fully-qualified column name (of the referenced
            // view or base table) if it exists.
            ColumnNameMap *cnm = result->getRETDesc()->findColumn(mappedVid);
            // QSTUFF

            if (cnm AND
                NOT cnm->getColumnDesc()->getColRefNameObj().isFabricated())
            {
              vctcul.insert(cvpn.getCurViewColNum(),
                            cnm->getColumnDesc()->getColRefNameObj());
            }
          } // if (result)
        } // else if (rename-table.column-name AND ...)
      } // if (column-name is not fabricated AND ...)

      if (cvpn.isItmColRefInColInRowVals() AND
          parent == cvpn.getQueryExpression())
      {
        // only increment the counter if the column in the bindRowValues()
        // is a column reference or a star column references; for example,
        //   create view v as select a, t1.*, b+2 from t1,t2;
        // in the above example, when we process a (or t1.*) here, we
        // need to increment the counter here. When we process b (in the
        // expression b+2), we do not increment the counter (the counter will
        // be incremented later in routine ItemExpr::convertToValueIdList().
        cvpn.incrCurViewColNum();
      }
    }
    return TRUE;
  }
  return FALSE;
} // BindUtil_CollectColumnUsageInfo()

static void BindUtil_UpdateNameLocForColRef(BindWA *bindWA,
                                            ColRefName &colRefName,
                                            ColumnNameMap *colNameMap,
                                            RelExpr *parent)
{
  ParNameLocList *pNameLocList = bindWA->getNameLocListPtr();
  if (pNameLocList == NULL) return;
  CMPASSERT(NOT colRefName.isStar());
  CMPASSERT(colNameMap);

  // note that the name of a common column (e.g., in a natural
  // join operation) may not be qualified so qualName and
  // qualNameExpanded may be empty.
  const ColRefName &colRefNameExpanded =
    colNameMap->getColumnDesc()->getColRefNameObj();
  const QualifiedName &qualNameExpanded =
    colRefNameExpanded.getCorrNameObj().getQualifiedNameObj();

  CorrName &corrName = colRefName.getCorrNameObj();
  QualifiedName &qualName = corrName.getQualifiedNameObj();

  ParNameLoc *pNameLoc = pNameLocList->getNameLocPtr(
			     colRefName.getNamePosition());
  // -- Triggers
  //CMPASSERT(pNameLoc);
  if (pNameLoc == NULL)
	  return;

  // Note that qualName can be empty when column is a common
  // column (e.g. in a natural join operation).  Common columns
  // is handled in routine joinCommonColumns().
  //
  if (qualName.fullyExpanded())
  {
    pNameLoc->setExpandedName(colRefName.getColRefAsAnsiString());
  }
  else if (qualNameExpanded.fullyExpanded())
  {
    // Expand colRefName's qualified name, without modifying any overlying
    // correlation name; and if no overlying corr name, also expand the
    // name loc to four-part col ref ("c.s.t.col").
    //
    qualName.setObjectName(qualNameExpanded.getObjectName());
    if (NOT qualName.getObjectName().isNull())
      qualName.applyDefaults(qualNameExpanded);
    if (corrName.getCorrNameAsString().isNull())
    {
      corrName.setCorrName(colRefNameExpanded.getCorrNameObj().getCorrNameAsString());
      pNameLoc->setExpandedName(colRefNameExpanded.getColRefAsAnsiString());
    }
  }
  else if (!colRefNameExpanded.getCorrNameObj().getCorrNameAsString().isNull())
  {
    pNameLoc->setExpandedName(colRefNameExpanded.getColRefAsAnsiString());
  }

  // Note that colRefNameExpanded might not be fully qualified in
  // certain cases (for example, the column is a derived column,
  // not a catalog object).  These should not be put into the usage
  // list.  The routine BindUtil_CollectColumnUsageInfo will take
  // care of these cases.
  //
  // Example:  The following query generates a derived column:
  // create view v as select a from (select x + y from t1) as t(a)

  BindUtil_CollectColumnUsageInfo(bindWA, colRefNameExpanded,
                                  colNameMap->getValueId(), parent);

} // BindUtil_UpdateNameLocForColRef()

static void BindUtil_UpdateNameLocForStarExpansion
				      (BindWA *bindWA,
				       const ColumnDescList &colDescList,
				       const StringPos starPos,
                                       RelExpr *parent)
{
  ParNameLocList *pNameLocList = bindWA->getNameLocListPtr();
  if (pNameLocList == NULL) return;
  if (starPos)
  {
    ParNameLoc *pNameLoc = pNameLocList->getNameLocPtr(starPos);
    CMPASSERT(pNameLoc);
    // tries to handle the following case
    // create view v as select * from (select * from (values (1), (2)) x
    NABoolean isStarWithoutColNames = FALSE;
    for (CollIndex i = 0; i < colDescList.entries(); i++)
    {
      if (colDescList[i]->getColRefNameObj().getColName().isNull())
      {
        isStarWithoutColNames = TRUE;
        break;
      }
    }
    if (isStarWithoutColNames)
      pNameLoc->setExpandedName("*");
    else
      pNameLoc->setExpandedName(colDescList.getColumnDescListAsString());
  }

  for (CollIndex i = 0; i < colDescList.entries(); i++)
    if (NOT BindUtil_CollectColumnUsageInfo(bindWA,
    					    colDescList[i]->getColRefNameObj(),
                                            colDescList[i]->getValueId(),
                                            parent))
      break;

} // BindUtil_UpdateNameLocForStarExpansion()

// -----------------------------------------------------------------------
// member functions for class ItemExpr
// -----------------------------------------------------------------------
void ItemExpr::bindChildren(BindWA *bindWA)
{
  Int32 savedCurrChildNo = currChildNo();
  for (Int32 i = 0; i < getArity(); i++, currChildNo()++) {
   
    if (child(i) != NULL) {
      ItemExpr *boundExpr = child(i)->bindNode(bindWA);
      if (bindWA->errStatus()) 
        return;
      child(i) = boundExpr;
    }
      
  }
  currChildNo() = savedCurrChildNo;
}

// Within the loop here, currChildNo() is an index or position.
// After the loop (it does not get reset), it is an index one past the max,
// i.e. it is the degree of this ItemExpr -- which will equal arity for all(?)
// items except ItemList (arity 2, arbitrary degree).
// If this ItemExpr returns a node different than 'this' in its bindNode,
// its bindNode must explicitly save this currChildNo degree if needed.
//
void ItemExpr::bindSelf(BindWA *bindWA)
{
  if (nodeIsBound()) return;

  // defaultSpecBoundAsSQLnull is needed only for our Tandem extension
  // of allowing DEFAULT to appear in item expressions
  // *except* when the DEFAULT value is NULL:
  //   INSERT INTO t(a,b) VALUES(DEFAULT,DEFAULT+1)
  //   -- DEFAULT for A is ok always, even if A is DEFAULT NULL,
  //   -- DEFAULT+1 for B is ok, *unless* B is DEFAULT NULL.
  // Couldn't catch the disallowed NULL+1 earlier in checkForSQLnullChild
  // because didn't know DEFAULT was NULL then, but no big deal, since
  // DEFAULT+1 and its ilk are allowed only as a Tandem extension anyway.
  //
  NABoolean defaultSpecBoundAsSQLnull = FALSE;

  for (Int32 i = 0; i < getArity(); i++, currChildNo()++) {
   
    ItemExpr *boundExpr = child(i)->bindNode(bindWA);
    if (bindWA->errStatus()) 
      return;
    if (! boundExpr)
      {
        bindWA->setErrStatus();
        return;
      }

    if (boundExpr->getOperatorType() == ITM_CONSTANT &&
        ((ConstValue *)boundExpr)->isNullWasDefaultSpec())
      defaultSpecBoundAsSQLnull = TRUE;

    child(i) = boundExpr;
      
  }

  //##? Should   getOperatorType() != ITM_CAST		?##
  //##? be	!getOperator().match(ITM_ANY_CAST)	?##
  if (defaultSpecBoundAsSQLnull)
    if (getOperatorType() != ITM_ASSIGN		 &&
        getOperatorType() != ITM_CAST		 &&
        getOperatorType() != ITM_EXPLODE_VARCHAR &&  // Genesis 10-980402-1556
        getOperatorType() != ITM_ITEM_LIST)
      checkForSQLnullChild(bindWA, this, FALSE, DEFAULT_BOUND_AS_NULL_);

  markAsBound();
} // ItemExpr::bindSelf()


  // Relaxation on an item expression is necessary if some of its
  // character-typed children are associated with character set values such
  // that the ANSI character type matching rule is violated for the expression.
  //
  // Relaxation can be done for an item expression, if it is either
  // 1) a character hostvar with UCS2 charset, or
  // 2) a SQL string function that returns an UCS2 result, and all
  //    character-typed operands of the function are relaxable.
  //
  // To perform the relaxation for a non-Translate relexable expression, add
  // a translate node (UCS2->ISO88591) on top of it. For a translate node,
  // simply remove the expression.
  //
  // Assume hostvar a, b and c are defined as
  //   char CHARACTER SET IS UCS2 a[10]
  //   char CHARACTER SET IS UCS2 b[1]
  //   char CHARACTER SET IS ISO88591 c[1]
  //
  // Examples of relexable item expressions:
  //    :a        // :a is an UCS2 hostvar
  //    :b        // :b is an UCS2 hostvar
  //    trim(:a)  // trim(:a) produces an UCS2 resulta, :a is relaxable.
  //    upper(:a) // upper(:a) produces an UCS2 result, :a is relaxable.
  //    repeat(:b, 10) // repeat(:b, 10) produces an UCS2 result,
  //                   // :b (the only character operand of the function)
  //                   // is relaxable.
  //
  // Examples of non-relexable item expressions:
  //    :c                        // :c is not an UCS2 host variable.
  //    concat(:a, 'b')           // 'b' is not a relaxable child of concat().
  //    replace(:a, _ucs2'b', _ucs2'z') // both _ucs2'b' and _ucs2'z'are not
  //                                    // relaxable.
  //

// A generic routine to relax ANSIcharacter-type matching rule for Static Inputs
//
// Always return this.
ItemExpr* ItemExpr::tryToRelaxCharTypeMatchRules(BindWA *bindWA)
{

  Int32 ucs2s = 0;
  CharInfo::CharSet cs = CharInfo::UnknownCharSet;

  // determine whether the ANSI character type matching rule is violated
  for (Int32 i = 0; i < getArity(); i++) {

    const NAType& type = child(i)->getValueId().getType();

    if ( type.getTypeQualifier() == NA_CHARACTER_TYPE )
    {
       switch (((const CharType&)type).getCharSet())
       {
        case CharInfo::UNICODE :
          ucs2s++;
          break;

        case CharInfo::KANJI_MP:
        case CharInfo::KSC5601_MP:
                 // can not perform relaxation on non-ISO88591 charsets.
          return this;

        case CharInfo::ISO88591:
        default:
          if (cs == CharInfo::UnknownCharSet)
             cs = ((CharType&)type).getCharSet();
          break;
       }
    }
  }

  if ( cs != CharInfo::UnknownCharSet && ucs2s > 0 )
   // ANSI rule is violated, try to perform relaxation
    return performRelaxation(cs, bindWA);
  else
   // relaxation not needed
    return this;
}

// Always return this.
ItemExpr* ItemExpr::performRelaxation(CharInfo::CharSet cs, BindWA *bindWA)
{
  // perofrm relaxation on any relaxable child
  for (Int32 i = 0; i < (Int32) currChildNo(); i++) {
     if ( ((ItemExpr*)child(i)) -> isCharTypeMatchRulesRelaxable() )
     {
        // for R2 FCS, the target is fixed at 88591. With Phase II work, it is
        // possible to set the target charset in Translator cstr.

	ItemExpr * newTranslateChild =
           new (bindWA->wHeap()) Translate(child(i), Translate::UNICODE_TO_ISO88591);

        newTranslateChild = newTranslateChild->bindNode(bindWA);
	if (bindWA->errStatus())
            return this;

	setChild(i, newTranslateChild);
     }
  }
  return this;
}


Int32 find_translate_type( CharInfo::CharSet src_cs,   // Source charset 
                           CharInfo::CharSet dest_cs ) // Destination charset 
{
     Int32 tran_type  = Translate::UNKNOWN_TRANSLATION;

     switch( dest_cs )
     {
        case CharInfo::ISO88591:
             switch( src_cs )
             {
                case CharInfo::UCS2:
                     tran_type = Translate::UNICODE_TO_ISO88591;
                     break;
                case CharInfo::UTF8:
                     tran_type = Translate::UTF8_TO_ISO88591;
                     break;
                case CharInfo::SJIS:
                     // tran_type = Translate::SJIS_TO_ISO88591;
                     break;
             }
             break;
        case CharInfo::UCS2:
             switch( src_cs )
             {
                case CharInfo::UTF8:
                     tran_type = Translate::UTF8_TO_UCS2;
                     break;
                case CharInfo::ISO88591:
                     tran_type = Translate::ISO88591_TO_UNICODE;
                     break;
                case CharInfo::SJIS:
                     tran_type = Translate::SJIS_TO_UCS2;
                     break;
             }
             break;
        case CharInfo::UTF8:
             switch( src_cs )
             {
                case CharInfo::UCS2:
                     tran_type = Translate::UCS2_TO_UTF8;
                     break;
                case CharInfo::ISO88591:
                     tran_type = Translate::ISO88591_TO_UTF8;
                     break;
                case CharInfo::SJIS:
                     tran_type = Translate::SJIS_TO_UTF8;
                     break;
             }
             break;
        case CharInfo::SJIS:
             switch( src_cs )
             {
                case CharInfo::UCS2:
                     tran_type = Translate::UCS2_TO_SJIS;
                     break;
                case CharInfo::ISO88591:
                     // tran_type = Translate::ISO88591_TO_SJIS;
                     break;
                case CharInfo::UTF8:
                     tran_type = Translate::UTF8_TO_SJIS;
                     break;
             }
             break;
     }
     return tran_type;
}

//
// For each character child node (of node pointed to by 'this') that has
// a different character set than the 'cs' argument, we must do something
// to convert the child to the 'cs' character set.  
//
// For children that are character string constants, we can do the actual
// translation here in the Compiler (UTF8->UCS2 or UCS2->UTF8).  If we
// later want to do something similar for the SJIS or ISO88591 configurations,
// we might have to restrict this and do the translation here in the compiler
// ONLY IF the string is all ASCII characters or something like that.
// For Seaquest platform (using the True ISO88591 configuration), the
// translation is between ISO88591 and UCS2, and we issue error messages
// when the translation fails.
//
// For children that are Translate nodes, we may be able to *delete* the
// Translate node in order to get the correct character set.
//
// For some child nodes that are functions, we may be able to push the
// translation operation down to the children of those functions.  We 
// expecially want to do this if the children of those functions are
// string constant nodes.
//
// For all other cases, we insert a Translate node between the 'this'
// node and the child node.
//
// Always return this.
//
ItemExpr* ItemExpr::performImplicitCasting(CharInfo::CharSet cs, BindWA *bindWA)
{
  if ( getOperatorType() == ITM_INSTANTIATE_NULL ||
       getOperatorType() == ITM_BITMUX )           // Don't want translate node
     return this;                                  // below one of these.


  ItemExpr *result = this;

  // If we are dealing with an expression that already has been assigned a ValueId,
  // it may be shared with other ItemExpr trees, therefore we need to make a copy
  // and can't modify it or its children.
  // Note that when we call this method from tryToDoImplicitCasting(), "this"
  // is an expression that does not yet have a value id. In this case, the
  // method will return "this", but maybe with modified children.
  if (getValueId() != NULL_VALUE_ID)
     result = copyTopNode();

  for (Int32 i = 0; i < getArity(); i++)
  {
     // initialize the result's child, assuming for now that it won't change
     if (result != this)
        result->setChild(i, getChild(i));

     const NAType& type = child(i)->getValueId().getType();
     if ( type.getTypeQualifier() != NA_CHARACTER_TYPE )
        continue; // Skip non-char types

     CharInfo::CharSet child_cs = ((const CharType&)type).getCharSet() ;
     if ( child_cs != CharInfo::ISO88591 && child_cs != CharInfo::UNICODE &&
          child_cs != CharInfo::UTF8     && child_cs != CharInfo::SJIS )
        continue;

     // If child is already in desired charset, then skip it.
     if ( child_cs == cs )
        continue;

     CharInfo::CharSet  desired_cs    = cs;

     enum cnv_charset eCnvCS = convertCharsetEnum(desired_cs);
     enum cnv_charset eCnvChild_cs = convertCharsetEnum(child_cs);

     Int32 tran_type = find_translate_type( child_cs, desired_cs );

     if ( tran_type == Translate::UNKNOWN_TRANSLATION )
        continue;

     // Remove a Translate node when appropriate (insteading of adding
     // another one to undo the existing one.)
     //
     OperatorTypeEnum chld_opertyp = child(i)->getOperatorType();
     if ( chld_opertyp == ITM_TRANSLATE )
     {
        Translate * Trans_node = (Translate *) ( child(i)->castToItemExpr() );
        ItemExpr * grandchild = Trans_node->child(0);

        const NAType& gr_type = grandchild->getValueId().getType();
        CharInfo::CharSet gr_child_cs = ((const CharType&)gr_type).getCharSet() ;

        if ( gr_child_cs == cs ) // If true, delete Translate node!
        {
           // Make the copy become this Translate Node's child
           result->setChild(i, grandchild);
           continue; //Go on to deal with next child
        }

     }  // end: if child(i) is an ITM_TRANSLATE

     else if ( chld_opertyp == ITM_CONSTANT )
     {
        NABoolean    cv_is_NULL ;
        Lng32         cv_StorageSize ;
        const char * cv_ConstValue;
        ValueId      cv_ValueId ;

        ConstValue * new_cv ;
          
        ConstValue * cv ;
        cv = (ConstValue*)(child(i)->castToItemExpr());
        cv_is_NULL = cv->isNull();
        cv_StorageSize = cv->getStorageSize() - cv->getType()->getVarLenHdrSize();
        cv_ConstValue  = (const char *)(cv->getConstValue());
        cv_ValueId     = cv->getValueId();

        if ( cv_is_NULL )
        {
           new_cv = new (bindWA->wHeap()) ConstValue() ;
        }
        else
        {
           // First find length of constant string in characters.
           // Also find length of result string in bytes.
           //
           Int32 byte_offset = 0;
           Int32 cv_len_in_chars = 0;
           Int32 rslt_len_in_bytes = 0;
           while ( byte_offset < cv_StorageSize )
           {
              Int32 firstCharLenInBuf;
              UInt32 UCS4value;
              firstCharLenInBuf = LocaleCharToUCS4(&cv_ConstValue[byte_offset],
                                                   cv_StorageSize - byte_offset,
                                                   &UCS4value,
                                                   eCnvChild_cs);
              if(firstCharLenInBuf < 0)
                {
                  *CmpCommon::diags() << DgSqlCode(-2109)
                                      << DgString0(CharInfo::getCharSetName(child_cs))
                                      << DgString1("UCS4") 
                                      << DgInt0(cv_len_in_chars)
                                      << DgInt1((Int32)(byte_offset));
                   bindWA->setErrStatus();
                   break;
              }
              byte_offset += firstCharLenInBuf;
              cv_len_in_chars++;

              Int16 tmpBuf[4];
              Int32 len_in_bytes = UCS4ToLocaleChar(&UCS4value,
                                                    (char *)tmpBuf, 8,
                                                    eCnvCS);
              if (len_in_bytes < 0)
                {
                  *CmpCommon::diags() << DgSqlCode(-2109)
                                      << DgString0(CharInfo::getCharSetName(child_cs))
                                      << DgString1(CharInfo::getCharSetName(desired_cs))
                                      << DgInt0(cv_len_in_chars)
                                      << DgInt1((Int32)(byte_offset));
                  bindWA->setErrStatus();
                   break;
              }
              rslt_len_in_bytes += len_in_bytes;
           }
           //
           // Now we know the exact length of the output buffer and
           // the length of the string (in chars!)
           //
           Int32 out_length = rslt_len_in_bytes +
                              ( (desired_cs == CharInfo::UNICODE) ?  2 : 1 );
           char * tmpbufr = new(bindWA->wHeap()) char[out_length];

           Int32 rslt_len = 0;

           byte_offset = 0;
           while ( byte_offset < cv_StorageSize )
           {
              Int32 firstCharLenInBuf;
              UInt32 UCS4value;
              firstCharLenInBuf = LocaleCharToUCS4(&cv_ConstValue[byte_offset],
                                                   cv_StorageSize - byte_offset,
                                                   &UCS4value,
                                                   eCnvChild_cs);
              if(firstCharLenInBuf < 0)
                   break; // Error would have already been reported above

              byte_offset += firstCharLenInBuf;
              Int32 len_in_bytes = UCS4ToLocaleChar(&UCS4value,
                                                    tmpbufr + rslt_len,
                                                    8, eCnvCS);
              if (len_in_bytes < 0)
                   break; // Error would have already been reported above

              rslt_len += len_in_bytes;
           }

           if ( desired_cs == CharInfo::UCS2 )
           {
             NAWString tmpStr;
             // Instead of using
             //         NAWString tmpStr = (NAWchar *)tmpbufr;
             // use NAWString::append() method in case tmpbufr contains
             // many consecultive embedded binary zero bytes that
             // look like the NAWchar NULL terminator(s).
             tmpStr.append((NAWchar *)tmpbufr, (size_t)cv_len_in_chars);
             NADELETEBASIC(tmpbufr, bindWA->wHeap()); tmpbufr = NULL;
             new_cv = new (bindWA->wHeap()) ConstValue( tmpStr,
                    CharInfo::UNICODE, CharInfo::DefaultCollation, CharInfo::COERCIBLE );
           }
           else
           {
              *(tmpbufr + rslt_len) = 0; // Add a trailing Null
              NAString tmpStr = tmpbufr;
              NADELETEBASIC(tmpbufr, bindWA->wHeap()); tmpbufr = NULL;
              new_cv = new (bindWA->wHeap()) ConstValue( tmpStr,
                    desired_cs, CharInfo::DefaultCollation, CharInfo::COERCIBLE);
           }
        }

        ItemExpr   * new_chld_ie = new_cv->bindNode(bindWA);
        if ( cv_is_NULL )
        {
           CharType myCharType = (const CharType&)type;
           Int32 bytesPerCh = myCharType.getBytesPerChar();
           NAType * newType = new (HEAP) SQLChar(HEAP, 
                           ( cv_ValueId == NULL_VALUE_ID )
                           ? 0 : type.getNominalSize()/bytesPerCh,
                             TRUE, FALSE, FALSE, FALSE,
                             cs,                         // The target charset
                             CharInfo::DefaultCollation,
                             CharInfo::COERCIBLE );
           ValueId theId = new_cv->getValueId();
           theId.coerceType(*newType);
           new_cv->changeType(newType);
        }

        ConstValue* new_chld_cv = dynamic_cast<ConstValue*>(new_chld_ie);

        CURRENTQCACHE->getHQC()
             ->collectBinderRetConstVal4HQC
                 ((ConstValue*)(child(i)->castToItemExpr()), new_chld_cv);
        
        result->setChild(i, new_chld_ie);

        continue; //Go on to deal with next child

     }    // end:  ITM_CONSTANT

     else if ( child(i)->getArity() > 0 ) // If it has child nodes
     {
        if ( child(i)->shouldPushTranslateDown(cs) >= 0 )
        {
           result->setChild(i, child(i)->performImplicitCasting(cs, bindWA));
           continue; //Go on to deal with next child

        }  // end: if ( child(i)->shouldPushTranslateDown(cs) >= 0 )
     }     // end: if ( child(i)->getArity() > 0 )

     ItemExpr * newTranslateChild =
        new (bindWA->wHeap()) Translate(child(i), tran_type );

     newTranslateChild = newTranslateChild->bindNode(bindWA);
     if (bindWA->errStatus())
        return this;
     result->setChild(i, newTranslateChild);

  }  //end of for loop (for each child node)

  if ( result->getOperatorType() == ITM_CAST )
  {
     // Must fix up the Cast's target type field
     const NAType *targetType = ((Cast *)result)->getType();
     NAType * newCastType = targetType->newCopy(bindWA->wHeap());
     CharType * newChCastType = (CharType *) newCastType;
     newChCastType->setCharSet(cs) ;
     newChCastType->setBytesPerChar(CharInfo::maxBytesPerChar(cs)) ;
     ((Cast *)result)->changeType(newCastType);
  }

  if (result != this && nodeIsBound() && getValueId() != NULL_VALUE_ID)
  {
     // if this node is already bound (not just marked as bound, but completely
     // bound with a value id assigned), then bind the new result as well
     result->bindNode(bindWA);
  }

  return result;
}

//
// This method tries to determine whether or not it is a good idea
// to push the Translate operation down to the children of the 'this'
// node.  Returns:
//        + num   if this is a good idea
//        0       if this is not a good idea
//        - num   if this is a bad idea (or not even possible)
//
Int32 ItemExpr::shouldPushTranslateDown(CharInfo::CharSet chrset) const
{
   Int32 Goodness = 1; // Good if number of Translate Nodes would not increase
   NABoolean sawChildWithDiffCS = FALSE;

   // Exclude all ItemExprs with arity > 1 that possibly produce character
   // output, have children with possibly a character type, and have at least
   // one of the following:
   //
   //  a) A fixed output charset, so pushing the translate down would
   //     have no effect. Examples: CAST, ENCODE.
   //  b) The semantics of the operator may depend on the character set.
   //     Examples: MIN, MAX, LIKE.
   //  c) the operator isn't well-enough understood to push the translate
   //     down below it. Example: ITM_USER_DEF_FUNCTION.
   //  d) Other reasons.

   // For any ItemExpr do not push a CAST node down the ItemExpr tree
   // if the ItemExpr itself is the output of a groupby. This is needed
   // because the components of a grouped expression are likely not available
   // above the groupby. Pushing the CAST down a group expression forces
   // the generator to look for parts of the group exptresion in nodes where
   // they are typically not available.
   if (isGroupByExpr())
     return -1 ;

   switch (getOperatorType())
     {
     case ITM_BITMUX:                // d) mixes separate item expressions together
     case ITM_MAX:                   // b) ordering, also may eliminate data
     case ITM_MIN:                   // b) ordering, also may eliminate data
     case ITM_MAX_ORDERED:           // b) ordering, also may eliminate data
     case ITM_MIN_ORDERED:           // b) ordering, also may eliminate data
     case ITM_USER_DEF_FUNCTION:     // c) may or may not depend on charset
     case ITM_COMP_ENCODE:           // a), b) output is binary disguised as ISO
     case ITM_COMP_DECODE:           // a), b) output is binary disguised as ISO
     case ITM_MOVING_MAX:            // b) possibly different orderings
     case ITM_MOVING_MIN:            // b) possibly different orderings
     case ITM_SCALAR_MIN:            // b) ordering, also may eliminate data
     case ITM_SCALAR_MAX:            // b) ordering, also may eliminate data
     case ITM_OLAP_MAX:              // b) ordering, also may eliminate data
     case ITM_OLAP_MIN:              // b) ordering, also may eliminate data
     case ITM_CONVERTTOHEX:          // b) operator depends on encoding
     case ITM_CONVERTFROMHEX:        // b) produces a specific encoding
     case ITM_TOKENSTR:              // b) implementation assumes all same 
     case ITM_SUBSTR_DOUBLEBYTE:     // b) operator specific to UCS2
     case ITM_LIKE_DOUBLEBYTE:       // b) operator specific to UCS2
     case ITM_UPPER_UNICODE:         // b) operator specific to UCS2
     case ITM_LOWER_UNICODE:         // b) operator specific to UCS2
     case ITM_REPLACE_UNICODE:       // b) operator specific to UCS2
     case ITM_INSTANTIATE_NULL:      // d) caused test failures
     case ITM_QUERYID_EXTRACT:       // a) output is always ISO88591
     case ITM_NARROW:                // b) NARROW may want to catch conversion errors
     case ITM_CONVERT:               // a) internal node, too late to do ICAT
     case ITM_CAST:                  // a) output is of a specific charset
     case ITM_CAST_CONVERT:          // a) internal node, too late to do ICAT
     case ITM_CAST_TYPE:
     case ITM_DATEFORMAT:
     case ITM_REVERSE:
       return -1;

     case ITM_LEFT:                  // b) counts characters
     case ITM_RIGHT:                 // b) counts characters
     case ITM_LIKE:                  // b) counts characters
     case ITM_REGEXP:                  // b) counts characters
     case ITM_SUBSTR:                // b) counts characters
     case ITM_REPLACE:               // b) counts characters
     case ITM_INSERT_STR:            // b) counts characters
       {
         // Some operators produce subtle differences when used on UTF-16 surrogate
         // pairs instead of 4-byte UTF-8 characters. Don't open that can of worms.
         const NAType&     myType    = getValueId().getType();
         CharInfo::CharSet myCharset = CharInfo::UnknownCharSet;

         if (myType.getTypeQualifier() == NA_CHARACTER_TYPE)
           myCharset = ((const CharType &) myType).getCharSet();

         if ((chrset    == CharInfo::UCS2 &&
              myCharset == CharInfo::UTF8)
             ||
             (myCharset == CharInfo::UCS2 &&
              chrset    == CharInfo::UTF8))
           return -1;
       }

     default:
       ; // go on, it's ok to push the translate operator down
     }

   for (Int32 ii = 0 ; ii < getArity(); ii++ )
   {
      const NAType& type = child(ii)->getValueId().getType();
      if ( type.getTypeQualifier() != NA_CHARACTER_TYPE )
         continue; // Skip non-char types

      if ( ((const CharType&)type).getCharSet() == chrset )
         continue; // Skip children with desired character set

      sawChildWithDiffCS = TRUE;

      switch ( child(ii)->getOperatorType() )
      {
         case ITM_CONSTANT: // Node should be changed to new character set.
            Goodness++ ;    // and may allow optimizer to eval at Compile time
            continue;
         case ITM_TRANSLATE:
            Goodness++ ; // Good since node would be removed if we did push down.
            continue;

         case ITM_BASECOLUMN:
         case ITM_VALUEIDUNION:
         case ITM_ROW_SUBQUERY:
         case ITM_IN_SUBQUERY:
            Goodness-- ; // Bad since we would add a Translate node for this.
            continue;

         default:
         Int32 chld_goodness = child(ii)->shouldPushTranslateDown(chrset) ;
         if ( chld_goodness < 0 )
            Goodness = 0; // Cannot push down any lower (at least for now)
         else if ( chld_goodness > 1 )
            Goodness++ ;
         continue;
      }
   }
   if (sawChildWithDiffCS == FALSE )
      Goodness = -1;     //Pushing Translate down is not possible
   return ( Goodness );
}

// A special routine to relax ANSI character-type matching rule for binary comparison
// operators for Static Inputs, due to special internal representation for such
// operators.
//
// Aleays return this.
ItemExpr* BiRelat::tryToRelaxCharTypeMatchRules(BindWA *bindWA)
{
   ItemExpr *x = (ItemExpr*)child(0);
   ItemExpr *xy= (ItemExpr*)child(1);

   // if both children are not list type, just relax both
   if (x-> getOperatorType() != ITM_ITEM_LIST &&
        xy-> getOperatorType() != ITM_ITEM_LIST
      )
     return ItemExpr::tryToRelaxCharTypeMatchRules(bindWA);

   // handle the comparison bwt two lists
   if ( x-> getOperatorType() == ITM_ITEM_LIST &&
        xy-> getOperatorType() == ITM_ITEM_LIST )
   {
      NAMemory *heap = CmpCommon::statementHeap();

      // collect the leaves from the two trees into two lists
      ExprValueIdList* leafList1 =
          ((ItemList*)(x)) -> collectLeaves(heap);
      ExprValueIdList* leafList2 =
          ((ItemList*)(xy)) -> collectLeaves(heap);

      // can not relax if both lists are not of same length
      if ( leafList1 -> entries() != leafList2 -> entries() )
         return this;

      // relax by pairs
      for ( Int32 i=0; i<leafList1 -> entries(); i++ ) {
         if ( performRelaxation((*leafList1)[i], (*leafList2)[i], bindWA) == FALSE )
           return this; // If one pair can not be made to be type-compatible, return
                        // right away. The type synthesise code will flag the
                        // type mismatch error.
      }
   }
   return this;
}

// This routine is a helper function to BiRelat::tryToRelaxCharTypeMatchRules().
// It linearizes the item expression tree by collecting the address of leave
// nodes into a list. The list is returned.
ExprValueIdList*
ItemList::collectLeaves(CollHeap* heap, ExprValueIdList* list)
{
   if ( list == NULL )
      list = new (heap) ExprValueIdList(heap);

   for ( Int32 i=0; i<getArity(); i++ ) {

     ItemExpr* c = this->child(i);
     if ( c )  {

       if ( ((ItemExpr*)child(i)) -> getOperatorType() != ITM_ITEM_LIST )
          list->insert(&child(i));
       else 
          return ((ItemList*)((ItemExpr*)child(i))) -> collectLeaves(heap, list);
     
     }
   }

   return list;
}

void ItemList::setResolveIncompleteTypeStatus(NABoolean x)
{
   for ( Int32 i=0; i<getArity(); i++ ) {
       ItemExpr* child = this->child(i);
       if ( child )
         child -> setResolveIncompleteTypeStatus(x);
   }
}

// This routine is a helper function to BiRelat::tryToRelaxCharTypeMatchRules().
// It performs the relaxation.
NABoolean
ItemExpr::performRelaxation(ExprValueId* ie1, ExprValueId* ie2, BindWA *bindWA)
{
   const NAType *operand1 = &(ie1->getValueId()).getType();
   const NAType *operand2 = &(ie2->getValueId()).getType();

   if ( operand1 -> getTypeQualifier() != NA_CHARACTER_TYPE ||
        operand2 -> getTypeQualifier() != NA_CHARACTER_TYPE
      )
     return TRUE;

   const CharType *charOp1 = (CharType*)operand1;
   const CharType *charOp2 = (CharType*)operand2;

   // pointer to the node to be translated (relaxed)
   ExprValueId* nodeToTranslatePtr = NULL;

   if (  (ie1->getPtr()) -> isCharTypeMatchRulesRelaxable() &&
         charOp2->getCharSet() == CharInfo::ISO88591
      )
   {
      nodeToTranslatePtr = ie1; // the left node should be translated
   } else
   if ( (ie2->getPtr()) -> isCharTypeMatchRulesRelaxable() &&
        charOp1->getCharSet() == CharInfo::ISO88591
      )
   {
      nodeToTranslatePtr = ie2; // the right node should be translated
   }

   if ( nodeToTranslatePtr != NULL )
   {
      ItemExpr * newTranslateChild =
         new (CmpCommon::statementHeap()) Translate(
                 nodeToTranslatePtr->getPtr(),
                 Translate::UNICODE_TO_ISO88591
                                           );

      newTranslateChild = newTranslateChild->bindNode(bindWA);
      if (bindWA->errStatus())
         return FALSE;

      *nodeToTranslatePtr = newTranslateChild;

      return TRUE;
   }

   return FALSE;
}

// special relax code because we want to handle Code_Value(:ucs_hv)
ItemExpr* CodeVal::tryToRelaxCharTypeMatchRules(BindWA *bindWA)
{
   if ( (getOperatorType() == ITM_ASCII || getOperatorType() == ITM_CODE_VALUE)
       && ((ItemExpr*)child(0)) -> isCharTypeMatchRulesRelaxable()
      )
   {
     return performRelaxation(CharInfo::ISO88591, bindWA);
   }
   return this;
}

// special relax code because we want to handle
// Translate(:ucs_hv using ISO88591TOUCS2)
ItemExpr* Translate::tryToRelaxCharTypeMatchRules(BindWA *bindWA)
{
   if ( getTranslateMapTableId() == Translate::ISO88591_TO_UNICODE &&
        ((ItemExpr*)child(0)) -> isCharTypeMatchRulesRelaxable()
      )
   {
     // just return the child(0) because this translate is not
     // necessary and we want to avoid double 1-to-1 translation
     return (ItemExpr*)child(0);
   }
   return this;
}

// special "relaxable" test for assign. Return true if the target is
// an UCS2 hostvar and the source is ISO88591.
NABoolean Assign::isRelaxCharTypeMatchRulesPossible()
{
   if ( getTarget().getType().getTypeQualifier() == NA_CHARACTER_TYPE &&
        getSource().getType().getTypeQualifier() == NA_CHARACTER_TYPE &&
        getSource().getItemExpr() -> isCharTypeMatchRulesRelaxable()
       )
   {
      const CharType& sourceCT = (const CharType&)getSource().getType();
      const CharType& targetCT = (const CharType&)getTarget().getType();

      if ( sourceCT.getCharSet() == CharInfo::UNICODE &&
           targetCT.getCharSet() == CharInfo::ISO88591
         )
        return TRUE;
   }
   return FALSE;
}

ItemExpr* Assign::tryToRelaxCharTypeMatchRules(BindWA *bindWA)
{
   return performRelaxation(CharInfo::ISO88591, bindWA);
}

// A generic routine to attempt Implicit Casting/Translation of
// child nodes to the character set required by the context.
//
// Always return this.
//
ItemExpr* ItemExpr::tryToDoImplicitCasting(BindWA *bindWA)
{
  ItemExpr *result = this;
  enum {iUCS2 = 0, iISO = 1, iUTF8 = 2, iSJIS = 3, iGBK = 4, iUNK = 5};
  Int32 Literals_involved[6] = { 0, 0, 0, 0, 0, 0};
  Int32 nonLiterals_involved[6] = { 0, 0, 0, 0, 0, 0 };
  Int32 charsets_involved[6] = { 0, 0, 0, 0, 0, 0 };
  Int32 charsetsCount = 0;
  CharInfo::CharSet cs          = CharInfo::UnknownCharSet;
  CharInfo::CharSet curr_chld_cs= CharInfo::UnknownCharSet;
  CharInfo::CharSet chld0_cs    = CharInfo::UnknownCharSet;
  OperatorTypeEnum  chld0_opType     = ITM_FIRST_ITEM_OP;
  OperatorTypeEnum  curr_chld_opType = ITM_FIRST_ITEM_OP;

  Int32 arity = getArity();
  if (arity <= 0)       // This method works only if there are children
     return this;
  //
  // First we must determine the best target character set to use
  // given the context.
  //
  // Step 1: Determine if we have children with different character set attributes
  //
  for (Int32 i = 0; i < arity; i++) {
    
    const NAType& type = child(i)->getValueId().getType();
    if ( type.getTypeQualifier() == NA_CHARACTER_TYPE )
      {
        curr_chld_cs = ((const CharType&)type).getCharSet();
        if ( i==0 ) chld0_cs = curr_chld_cs;  // Remember this one

        Int16 cur_chld_cs_ndx = iUNK;

        switch ( curr_chld_cs )
          {
          case CharInfo::UNICODE :
            cur_chld_cs_ndx = iUCS2;
            break;

          case CharInfo::ISO88591:
            cur_chld_cs_ndx = iISO;
            break;

          case CharInfo::UTF8:
            cur_chld_cs_ndx = iUTF8;
            break;

          case CharInfo::SJIS:
            cur_chld_cs_ndx = iSJIS;
            break;

          case CharInfo::GBK:
            cur_chld_cs_ndx = iGBK;
            break;

            //case CharInfo::KANJI_MP:
            //case CharInfo::KSC5601_MP:
          default:
            break; // Can not translate these currently.
          }
        charsets_involved[cur_chld_cs_ndx]++;

        OperatorTypeEnum curr_chld_opType = child(i)->getOperatorType();
        if (i == 0 ) chld0_opType = curr_chld_opType;  // Remember this one

        if ( curr_chld_opType == ITM_CONSTANT )
          Literals_involved[cur_chld_cs_ndx] += 1 ;
        else
          nonLiterals_involved[cur_chld_cs_ndx] += 1 ;
      }
      
  }

  for (Int32 j = 0; j < iUNK; j++)
  {
    if (charsets_involved[j] > 0)
      charsetsCount++;
  }

  if (charsetsCount > 1)
  {
    // Now choose the best character set for the translations

    cs = CharInfo::ISO88591;

    if ( ! CanChild0BeImplicitlyCast() )
    {
       // Looks like an Assign operation, so use child 0's cs
       cs = chld0_cs;
    }
    else 
    {
       if ( nonLiterals_involved[iUCS2] > 0 )
          cs = CharInfo::UCS2;
       else if ( nonLiterals_involved[iUTF8] > 0 )
          cs = CharInfo::UTF8;
       else if ( nonLiterals_involved[iSJIS] > 0 )
          cs = CharInfo::SJIS;
       else if ( Literals_involved[iUCS2] > 0 )
          cs = CharInfo::UCS2;
       else if ( Literals_involved[iUTF8] > 0 )
          cs = CharInfo::UTF8;
       else if ( Literals_involved[iSJIS] > 0 )
          cs = CharInfo::SJIS;
       else if ( Literals_involved[iGBK] > 0 )
          cs = CharInfo::GBK;

       //
       // Now, we may be able to optimize by translating the 1st child
       // rather than the 2nd.  For now, we consider only the case
       // when there are exactly 2 children (e.g. WHERE predicate)
       //
       if ( ( cs == chld0_cs ) &&  ( arity == 2 ) &&
               ( curr_chld_opType != ITM_TRANSLATE ) &&
               ( charsetsCount == (charsets_involved[iUCS2] + charsets_involved[iUTF8] + charsets_involved[iGBK]) ) )
       {
          if ( chld0_opType == ITM_TRANSLATE )
             cs = curr_chld_cs;  //...because we will eliminate a translate op
          else
          if ( CanChild0BeImplicitlyCast()      &&
             ( chld0_opType != ITM_BASECOLUMN ) )
          {
             if ( child(0)->shouldPushTranslateDown( curr_chld_cs ) >
                  child(1)->shouldPushTranslateDown( cs ) )
             {  // If translating to curr_chld_cs is more beneficial
                cs = curr_chld_cs;
             }
          }
       }
    }

    result = performImplicitCasting(cs, bindWA);
  }
  else if ( getOperatorType() == ITM_CAST )
  {
     const NAType& chldType = child(0)->getValueId().getType();
     if ( chldType.getTypeQualifier() == NA_CHARACTER_TYPE )
     {
        CharInfo::CharSet chld_cs = ((const CharType&)chldType).getCharSet();

        const NAType *desiredType = ((Cast *)this)->getType();
        if ( desiredType->getTypeQualifier() == NA_CHARACTER_TYPE )
        {
           CharInfo::CharSet Desired_cs = ((const CharType*)desiredType)->getCharSet();
           /*
           * this is a special handling for jira 1720, only used in a bulkload scenario
           * that is, when user set the HIVE_FILE_CHARSET to 'gbk', it means the data saved in hive
           * table is encoded as GBK. Trafodion default all Hive data charset as 'UTF8', so 
           * this will allow the auto charset converting to happen during bulk load
           * the reason is:
           * hive scan will mark the source column as GBK when HIVE_FILE_CHARSET is set to GBK
           * which is the only value it can be 
           * So the bind will invoke this implicit casting method to check if an auto charset 
           * converting is needed. 
           * In the hive scan, it does not set the tgtCharSetSpecified field, so in order to 
           * force it to perform a translate, add a checking here
           */
           if( (chld_cs != Desired_cs) && CmpCommon::getDefaultString(HIVE_FILE_CHARSET) == "GBK" )
              result = performImplicitCasting( Desired_cs, bindWA );
           else if ( (chld_cs != Desired_cs) && ( ! ((Cast *)this)->tgtCharSetSpecified() ) )
           {
              //
              // Looks like user said CAST( ... as [var]char(NNN) ) 
              // without specifying charset.
              // Leave the child's charset alone and change desired type to match it.
              NAType * newType = desiredType->newCopy(bindWA->wHeap());
              CharType * newCType = (CharType *) newType;
              Int32 child_bpc = CharInfo::maxBytesPerChar(chld_cs);
              Int32 child_charLimit = ((const CharType&)chldType).getStrCharLimit();
              Int32 Desired_charLimit = ((const CharType*)desiredType)->getStrCharLimit();
  
              newCType->setCharSet(chld_cs) ;
              newCType->setBytesPerChar(child_bpc) ;
              newCType->setEncodingCharSet( ((const CharType&)chldType).getEncodingCharSet() );
              if ( chld_cs == CharInfo::UNICODE )
                  newCType->setDataStorageSize( Desired_charLimit * child_bpc );
              if ( (Desired_cs == CharInfo::ISO88591) &&
                   (chld_cs    == CharInfo::UTF8)
                 )
              {
                  // Assume user meant [var]char(nnn CHARS)
                  newCType->setDataStorageSize( Desired_charLimit * child_bpc );
              }
              ((Cast *)this)->changeType(newType); // Change the Cast's target type!

              if (getValueId() != NULL_VALUE_ID)
                  getValueId().changeType(newType);
              return this;
           }
           else if ( chld_cs != Desired_cs )  // New CharSet specified
           {
              result = performImplicitCasting( Desired_cs, bindWA );
           }
        }
     }
  }
  else if ( getOperatorType() == ITM_TRANSLATE )
  {
     CharInfo::CharSet Required_cs = CharInfo::UnknownCharSet;
     Translate *parent_tran = (Translate *) this;
     switch( parent_tran->getTranslateMapTableId() )
     {
        case Translate::ISO88591_TO_UNICODE:
        case Translate::ISO88591_TO_UTF8:
        // case Translate::ISO88591_TO_SJIS:
             Required_cs = CharInfo::ISO88591;
             break;
        case Translate::SJIS_TO_UNICODE:
        case Translate::SJIS_TO_UCS2:
        case Translate::SJIS_TO_UTF8:
        // case Translate::SJIS_TO_ISO88591:
             Required_cs = CharInfo::SJIS;
             break;
        case Translate::UTF8_TO_UCS2:
        case Translate::UTF8_TO_SJIS:
        case Translate::UTF8_TO_ISO88591:
             Required_cs = CharInfo::UTF8;
             break;
        case Translate::UNICODE_TO_ISO88591:
        case Translate::UNICODE_TO_SJIS:
        case Translate::UCS2_TO_SJIS:
        case Translate::UCS2_TO_UTF8:
             Required_cs = CharInfo::UNICODE;
             break;
	case Translate::GBK_TO_UTF8:
	     Required_cs = CharInfo::GBK;
             break;
        default:
             break;
     }
     if ( Required_cs == CharInfo::UnknownCharSet )
        return this; // Cannot do anything for this situation

     if ( child(0)->getOperatorType() == ITM_TRANSLATE ) // 1st child ?
     {
        const NAType& type = child(0)->child(0)->getValueId().getType();
        CharInfo::CharSet gr_chld_cs = ((const CharType&)type).getCharSet();
        if ( gr_chld_cs == Required_cs )
        {
           // We have an undesired TRANSLATE node, so we will remove it.

           ItemExpr *grnd_child = child(0)->child(0)->castToItemExpr();

           result->setChild(0, grnd_child);
        }
     }
     else if ( child(0)->getOperatorType() != ITM_BASECOLUMN )
     {
        const NAType& type = child(0)->getValueId().getType();
        if ( type.getTypeQualifier() == NA_CHARACTER_TYPE )
        {
           CharInfo::CharSet chld_cs = ((const CharType&)type).getCharSet();
           //
           // If the child is NOT of the required character set we need to
           // do Implicit Casting UNLESS the child is ISO88591 and the
           // Required_cs is a superset of ISO88591
           //
           if ( ( chld_cs != Required_cs ) &&
              ! ( ( chld_cs == CharInfo::ISO88591 ) && ( Required_cs == CharInfo::UTF8 ) )
              )
           {
              result = performImplicitCasting(Required_cs, bindWA); 
           }
        }
     }
     // else it is an ITM_BASECOLUMN.  We choose to give user an error,
     // so here we just return.

  }  // end of:  if ( getOperatorType() == ITM_TRANSLATE )

  // assumption is that we call this method before assigning a value id, and that
  // performImplicitCasting returns the object it has been called on
  CMPASSERT(result == this && getValueId() == NULL_VALUE_ID);

  return result; // No automatic translations to do.
}

ItemExpr *ItemExpr::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) return this;
  bindSelf(bindWA);
  if (bindWA->errStatus()) return this;

  ItemExpr* exp = this;

  // A quick way to determine whether we should worry about relaxation.
  // Only comparison and assign operators, SQL string functions are the
  // candidates.
  if ( isRelaxCharTypeMatchRulesPossible() )
  {
     // may return this or an modified expression. In either case, need to
     // perform the type synthesization.
     exp=tryToRelaxCharTypeMatchRules(bindWA);
     CMPASSERT(exp);
  }

  if ( getArity() > 0 || getOperatorType() == ITM_VALUEIDUNION)
  {
     // Might be possible to do some automatic translation of character sets
     //
     if (getOperatorType() != ITM_ITEM_LIST) // Ignore LISTs - no context yet!
     {
        if ( CmpCommon::getDefault(ALLOW_IMPLICIT_CHAR_CASTING) == DF_ON )
        {
           //User wants this (despite being rather non-ANSI standard)
           //so we will try to do so.

          exp = tryToDoImplicitCasting(bindWA);
          CMPASSERT(exp);
        }
     }
  }

  const NAType *type = exp->synthTypeWithCollateClause(bindWA);
  if (!type) return exp;
  setValueId(createValueDesc(bindWA, exp, type));
  return exp;
} // ItemExpr::bindNode()

//////////////////////////////////////////////////
// This function binds an item expression tree.
// After bindNode() returns, the completeness of
// the type of the expression is checked. Pushing
// down desired complete type may take place.
//////////////////////////////////////////////////

ItemExpr *ItemExpr::bindNodeRoot(BindWA *bindWA)
{
  ItemExpr* exp = bindNode(bindWA);

  if ( getResolveIncompleteTypeStatus() == FALSE )
      return exp;

// if the valudId is invalid or we are not bind the true root, 
// forget the pushdown type business
  if ( exp ==0 || exp->getValueId() == NULL_VALUE_ID ||
       bindWA->isBindTrueRoot() == FALSE
     )
     return exp;

  const NAType* type = &(exp->getValueId()).getType();

  if ( type->getTypeQualifier() == NA_CHARACTER_TYPE &&
       ((CharType*)type)->getCharSet() == CharInfo::UnknownCharSet
     )
  {

   // deal with cases where the item expression tree can not determine
   // the charset attribute by itself.
   // Example: select upper('abcd') from t;
   // Solution: we force ISO88591 throughout the tree here.
     const NAType* desired = CharType::desiredCharType(CharInfo::ISO88591);

     (exp->getValueId()).coerceType(*desired, NA_CHARACTER_TYPE);
     type = &(exp->getValueId()).getType();

// We only give one shot here.
     if ( type->getTypeQualifier() == NA_CHARACTER_TYPE &&
          ((CharType*)type)->getCharSet() == CharInfo::UnknownCharSet
        )
     {
        return 0;
     }
  }

  return exp;

} // ItemExpr::bindNodeRoot()

ItemExpr* ItemExpr::_bindNodeRoot(BindWA *bindWA)
{
  return 0;
}

ItemExpr * ItemExpr::foldConstants(BindWA *bindWA)
{
  // a shortcut to constant folding for use in the binder
  ItemExpr *result = foldConstants(CmpCommon::diags(), TRUE);
  if (CmpCommon::diags()->mainSQLCODE() < 0)
    bindWA->setErrStatus();
  return result;
}

ItemExpr * ItemExpr::bindUDFsOrSubqueries(BindWA *bindWA)
{

  // Method to bind only a UDF or Subquery node in an expr Tree. This
  // is used for assign expression in update to make sure we know the
  // the degree of the UDF/Subquery before we create assignment lists.
  
  switch (getOperatorType())
  {
    case ITM_ROW_SUBQUERY:
    {
      DefaultToken allowMultiDegreeTok =
                    CmpCommon::getDefault(ALLOW_MULTIDEGREE_SUBQ_IN_SELECTLIST);

      if (allowMultiDegreeTok == DF_ON ||
          allowMultiDegreeTok == DF_SYSTEM)
        return bindNode(bindWA); 
      else
        return this; // don't do anything.
      break;
    }
            
    case ITM_USER_DEF_FUNCTION:
      return bindNode(bindWA); 
      break;
            
    default:
      // Walk the rest of the tree.
      for (Int32 chld=0; chld < getArity(); chld++)
      {
        child(chld) = child(chld)->bindUDFsOrSubqueries(bindWA); 
      }
  }
  return this;
}

ItemExpr *AnsiUSERFunction::bindNode(BindWA *bindWA)
{
  if (bindWA->inDDL() && (bindWA->inCheckConstraintDefinition()))
  {
    StmtDDLAddConstraintCheck *pCkC = bindWA->getUsageParseNodePtr()
                                    ->castToElemDDLNode()
                                    ->castToStmtDDLAddConstraintCheck();
    *CmpCommon::diags() << DgSqlCode(-4132);
    bindWA->setErrStatus();
    return this;
  }

  if (nodeIsBound())
    return getValueId().getItemExpr();
  const NAType *type = synthTypeWithCollateClause(bindWA);
  if (!type) return this;

  // Multiple references to the USER function should return the same
  // user ID in one query, no matter which process it is being evaluated
  // (sqlci/esp/dp2).
  // So all occurrences of CURRENT USER functions are treated as input
  // values and are given the same value id.  Similarly, all occurrences
  // of SESSION USER function are given the same value id (but different
  // from that of the CURRENT USER).
  //
  ItemExpr * ie = ItemExpr::bindUserInput(bindWA,type,getText());
  if (bindWA->errStatus())
    return this;

  // add this value id to BindWA's input function list.
  bindWA->inputFunction().insert(getValueId());

  return ie;
}

ItemExpr *MonadicUSERFunction::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();

  // For now user(x) is allowed only in the top most select list
  // check that first, or else give an error

  BindScope * currScope = bindWA->getCurrentScope();
  BindContext *context = currScope->context();

  if (!(context->inSelectList()))
  {
    *CmpCommon::diags() << DgSqlCode(-4310)
			<< DgString0("USER(x)");
    bindWA->setErrStatus();
    return NULL;
  }

  // Check for case like select (select user(1) ...).
  // or select * from t1, (select user(x) from t1) t3 etc.
  // Here the user function is in the select list of a
  // sub-query and in join, hence is not allowed.
  // Also it is not allowed at any other place example orderBy
  BindScope *prevScope   = NULL;

  while (currScope)
  {
    BindContext *currContext = currScope->context();
    if (currContext->inSubquery() ||
      currContext->inOrderBy() ||
      currContext->inExistsPredicate() ||
      currContext->inGroupByClause() ||
      currContext->inWhereClause() ||
      currContext->inHavingClause() ||
      currContext->inUnion() ||
      currContext->inJoin()       )
    {
 	*CmpCommon::diags() << DgSqlCode(-4310)
			    << DgString0("USER(x)");
	bindWA->setErrStatus();
	return NULL;
    }
    prevScope = currScope;
    currScope = bindWA->getPreviousScope(prevScope);
  }

  bindSelf(bindWA);
  if (bindWA->errStatus()) return this;

  unBind();

  return ItemExpr::bindNode(bindWA);
} // MonadicUSERFunction::bindNode


// -----------------------------------------------------------------------
// BiRelat & Function classes set context flags, then call ItemExpr::bindNode
// (they are directly derived subclasses of ItemExpr; safe to invoke this)
// -----------------------------------------------------------------------

ItemExpr *BiRelat::bindNode(BindWA *bindWA)
{
  if (checkForSQLnullChild(bindWA, this, getSpecialNulls())) return this;
  ItemExpr *save = bindWA->getCurrentScope()->context()->inMultaryPred();
  bindWA->getCurrentScope()->context()->inMultaryPred() = this;

  //changes for HistIntRed

  //save the current state of inRangePred_ in the binder
  //context so that we can set it back after binding this
  //node in case we reset it the if statement below
  NABoolean inRangePredSave = bindWA->getCurrentScope()->context()->inRangePred();

  //get the operator
  OperatorTypeEnum oper = getOperatorType();

  //check if the operator implies a range predicate
  //this would be if the operator is <, >, <=, >=
  if((oper == ITM_LESS)||
     (oper == ITM_LESS_EQ)||
     (oper == ITM_GREATER)||
     (oper == ITM_GREATER_EQ))
    {
      bindWA->getCurrentScope()->context()->inRangePred() = TRUE;
    };
  bindWA->getCurrentScope()->context()->inPredicate() = TRUE;

  // this will bind/type-propagate all children.
  //bindChildren(bindWA);
  ItemExpr *boundExpr = ItemExpr::bindNode(bindWA);

  //set inRangePred_ in the context to original value
  bindWA->getCurrentScope()->context()->inRangePred() = inRangePredSave;
  bindWA->getCurrentScope()->context()->inMultaryPred() = save;

  bindWA->getCurrentScope()->context()->inPredicate() = FALSE;

  if (bindWA->errStatus()) 
    return this;

  if (!handleIncompatibleComparison(bindWA))
    return NULL;

  // ------
  // for dynamic histogram compression:
  // ----------------------------------
  // if the node relates two basecolumns mark the columns
  // as having a join predicate
  // The hasJoinPred flag is used by dynamic histogram compression
  // to pick the correct version of compressed histograms
  if((child(0)->getOperatorType()==ITM_BASECOLUMN) &&
     (child(1)->getOperatorType()==ITM_BASECOLUMN))
  {
    // get the table descriptor for each column
    TableDesc * tabdesc1 = ((BaseColumn *) getChild(0))->getTableDesc();
    TableDesc * tabdesc2 = ((BaseColumn *) getChild(1))->getTableDesc();

    // if table descriptors don't match these columns are from different
    // tables i.e. join predicate
    // This avoids a scenario like select * from t1 where t1.a = t1.b
    if(tabdesc1 != tabdesc2)
    {
      ((BaseColumn *) getChild(0))->getNAColumn()->setHasJoinPred();
      ((BaseColumn *) getChild(1))->getNAColumn()->setHasJoinPred();
    }
  }

  if ((child(0)->getOperatorType() == ITM_BASECOLUMN) ||
      (child(1)->getOperatorType() == ITM_BASECOLUMN))
    {
      const NAType &type1 = 
	child(0)->castToItemExpr()->getValueId().getType();
      const NAType &type2 = 
	child(1)->castToItemExpr()->getValueId().getType();

      if ((type1.getTypeQualifier() == NA_CHARACTER_TYPE) &&
	  (type2.getTypeQualifier() == NA_CHARACTER_TYPE))
	{
	  const CharType &cType1 = (CharType&)type1;
	  const CharType &cType2 = (CharType&)type2;
	  
	  if (cType1.isCaseinsensitive() != cType2.isCaseinsensitive())
	    {
	      if ((child(0)->getOperatorType() == ITM_BASECOLUMN) &&
		  (cType1.isCaseinsensitive()))
		{
		  ItemExpr * newChild = 
		    new (bindWA->wHeap()) Convert(child(0));
		  newChild->bindNode(bindWA);
		  setChild(0, newChild);
		}

	      if ((child(1)->getOperatorType() == ITM_BASECOLUMN) &&
		  (cType2.isCaseinsensitive()))
		{
		  ItemExpr * newChild = 
		    new (bindWA->wHeap()) Convert(child(1));
		  newChild->bindNode(bindWA);
		  setChild(1, newChild);
		}
	    }
	}

      if ( child(0)->getOperatorType() == ITM_CONSTANT ||
           child(1)->getOperatorType() == ITM_CONSTANT )
      {
        NABoolean checkRebind = FALSE;
        Int32 cvExprIndex;
        if ( child(1)->getOperatorType() == ITM_CONSTANT AND
             oper == ITM_EQUAL )
        {
            checkRebind = TRUE;
            cvExprIndex = 1;
        } else {
           if ( child(0)->getOperatorType() == ITM_CONSTANT AND
                oper == ITM_EQUAL )
           {
             checkRebind = TRUE;
             cvExprIndex = 0;
           }
        }
  
        if ( checkRebind == TRUE ) {
            ConstValue* cvExpr = (ConstValue*)(getChild(cvExprIndex));
            if ( cvExpr->getValueId().getType().getTypeQualifier() == NA_CHARACTER_TYPE
                 AND
                 cvExpr-> isRebindNeeded() == TRUE ) {
               cvExpr -> unBind();
               cvExpr->bindNode(bindWA); // rebind to populate the cache or
                                         // share the valueId of an identical
                                         // string literal
               setChild(cvExprIndex, cvExpr);
               cvExpr->setRebindNeeded(FALSE);
            }
        }
      }
    }

  // 
  return boundExpr;
} // BiRelat::bindNode()

void BiRelat::synthTypeAndValueId(NABoolean redriveTypeSynthesisFlag, 
				  NABoolean redriveChildTypeSynthesis)
{
  ItemExpr::synthTypeAndValueId(redriveTypeSynthesisFlag,
				redriveChildTypeSynthesis);

  if ((child(0)->getOperatorType() == ITM_BASECOLUMN) ||
      (child(1)->getOperatorType() == ITM_BASECOLUMN))
    {
      const NAType &type1 = 
	child(0)->castToItemExpr()->getValueId().getType();
      const NAType &type2 = 
	child(1)->castToItemExpr()->getValueId().getType();

      if ((type1.getTypeQualifier() == NA_CHARACTER_TYPE) &&
	  (type2.getTypeQualifier() == NA_CHARACTER_TYPE))
	{
	  const CharType &cType1 = (CharType&)type1;
	  const CharType &cType2 = (CharType&)type2;
	  
	  if (cType1.isCaseinsensitive() != cType2.isCaseinsensitive())
	    {
	      if ((child(0)->getOperatorType() == ITM_BASECOLUMN) &&
		  (cType1.isCaseinsensitive()))
		{
		  ItemExpr * newChild = new HEAP Convert(child(0));
		  newChild->synthTypeAndValueId(redriveTypeSynthesisFlag,
						redriveChildTypeSynthesis);
		  setChild(0, newChild);
		}

	      if ((child(1)->getOperatorType() == ITM_BASECOLUMN) &&
		  (cType2.isCaseinsensitive()))
		{
		  ItemExpr * newChild = new HEAP Convert(child(1));
		  newChild->synthTypeAndValueId(redriveTypeSynthesisFlag,
						redriveChildTypeSynthesis);
		  setChild(1, newChild);
		}
	    }
	}
    }
  
}

static ItemExpr * ItemExpr_handleIncompatibleComparison(
     BindWA *bindWA,
     ItemExpr * thisPtr,
     ItemExpr * op1, ItemExpr * op2,
     ItemExpr * &newOp1, ItemExpr * &newOp2)
{
  const NAType &type1 = op1->castToItemExpr()->getValueId().getType();
  const NAType &type2 = op2->castToItemExpr()->getValueId().getType();

  // lob cannot be in a predicate
  if (type1.isLob() || type2.isLob())
  {
    *CmpCommon::diags() << DgSqlCode(-4322);
    bindWA->setErrStatus();
    return NULL;  // error
  }

  // binary types can be compared with all other datatypes
  if ((DFS2REC::isBinaryString(type1.getFSDatatype())) ||
      (DFS2REC::isBinaryString(type2.getFSDatatype())))
    {
      return thisPtr;
    }
 
  // Check if we are to allow certain incompatible comparisons
  if (CmpCommon::getDefault(ALLOW_INCOMPATIBLE_OPERATIONS) == DF_ON)
  {
    // if left and right operands are incompatible, convert
    // one of them to the other type.
    // The check for the conditions under which this is allowed
    // has already been done in BiRelat::synthesizeType.

    Int32 srcOpIndex = -1;  //index of src child
    Int32 tgtOpIndex = -1;  //index of tgt child
    Int32 conversion = 0; //0 = no conversion
                        //1 = cast char to numeric
                        //2 = cast char to date
                        //3 = cast date to numeric
                        //4 = cast numeric to interval
                        //5 = cast numeric to date

    //check if:
    //1. Comparing numeric to a character type
    //2. Comparing date column to a character string literal
    //3. Comparing date to numeric
    //4. Comparing interval to numeric
    //6. Comparing date to char of form: DD-MON-YYYY

    //check for numeric to character comparison
    if ((type1.getTypeQualifier() == NA_CHARACTER_TYPE) &&
	(type2.getTypeQualifier() == NA_NUMERIC_TYPE))
    {
      // convert op1(char) to numeric type
      srcOpIndex = 0;
      tgtOpIndex = 1;
      conversion = 1;
    }
    else
    if ((type1.getTypeQualifier() == NA_NUMERIC_TYPE) &&
        (type2.getTypeQualifier() == NA_CHARACTER_TYPE))
    {
      // convert op2(character) to numeric type
      srcOpIndex = 1;
      tgtOpIndex = 0;
      conversion = 1;
    }

    //check for date to character literal comparison
    if (((type1.getTypeQualifier() == NA_CHARACTER_TYPE) &&
	 (op1->getOperatorType() == ITM_CONSTANT) &&
	 (type2.getTypeQualifier() == NA_DATETIME_TYPE)) ||
	((type1.getTypeQualifier() == NA_DATETIME_TYPE) &&
	 (type2.getTypeQualifier() == NA_CHARACTER_TYPE) &&
	 (op2->getOperatorType() == ITM_CONSTANT)))
    {
      NABoolean op1IsChar = 
	(type1.getTypeQualifier() == NA_CHARACTER_TYPE);

      // only a specific char const pattern is being supported right now.
      // Pattern is:   DD-MON-YYYY.
      // Check for that.
      ConstValue * cv = 
	(op1IsChar ? (ConstValue*)op1->castToItemExpr()
	 : (ConstValue*)op2->castToItemExpr());

      if (cv->getStorageSize() == strlen("DD-MON-YYYY"))
	{
	  char * str = (char*)cv->getRawText()->data();
	  if ((str[2] == '-') && (str[6] == '-'))
	    {
	      if (op1IsChar)
		{
		  //convert op1(char literal) to date
		  srcOpIndex = 0;
		  tgtOpIndex = 1;
		}
	      else
		{
		  //convert op2(char literal) to date
		  srcOpIndex = 1;
		  tgtOpIndex = 0;
		}

	      if (op1IsChar)
		{
		  if (type2.getPrecision() == SQLDTCODE_DATE)
		    conversion = 6;
		  else if (type2.getPrecision() == SQLDTCODE_TIMESTAMP)
		    conversion = 7;
		}
	      else
		{
		  if (type1.getPrecision() == SQLDTCODE_DATE)
		    conversion = 6;
		  else if (type1.getPrecision() == SQLDTCODE_TIMESTAMP)
		    conversion = 7;
		}
	    }
	}
    }

    // check for date to character comparison that was not covered by
    // the previous condition.
    if (conversion == 0)
      {
	if ((type1.getTypeQualifier() == NA_CHARACTER_TYPE) &&
	    (type2.getTypeQualifier() == NA_DATETIME_TYPE))
	  {
	    //convert op1(char literal) to date
	    srcOpIndex = 0;
	    tgtOpIndex = 1;
	    conversion = 2;
	  }
	else
	  if ((type1.getTypeQualifier() == NA_DATETIME_TYPE) &&
	      (type2.getTypeQualifier() == NA_CHARACTER_TYPE))
	    {
	      //convert op2(char literal) to date
	      srcOpIndex = 1;
	      tgtOpIndex = 0;
	      conversion = 2;
	    }
      }

    // check for date to numeric comparison.
    // if one child is a column and the other child is not, then
    // convert the non-column child to the type of the column child.
    // This would result in a CAST node not being added on top of the
    // column which could then be used as a key predicate.
    // If both children are columns or both are non-columns, then convert 
    // datetime to numeric.
    if ((type1.getTypeQualifier() == NA_DATETIME_TYPE) &&
	(type2.getTypeQualifier() == NA_NUMERIC_TYPE))
      {
	if (((op1->getOperatorType() == ITM_REFERENCE) ||
	     (op1->getOperatorType() == ITM_BASECOLUMN)) &&
	    (NOT ((op2->getOperatorType() == ITM_REFERENCE) ||
		  (op2->getOperatorType() == ITM_BASECOLUMN))))
	  {
	    // op1 is column, op2 is not.
	    // convert op2(numeric) to the type of op1 (datetime)
	    srcOpIndex = 1;
	    tgtOpIndex = 0;
	    conversion = 5;
	  }
	else
	  {
	    // op2 is column and op1 is not,
	    // or both children are columns, or both children are non-columns.
	    // Convert op1(datetime) to numeric type
	    srcOpIndex = 0;
	    tgtOpIndex = 1;
	    conversion = 3;
	  }
      }
    else if ((type1.getTypeQualifier() == NA_NUMERIC_TYPE) &&
	     (type2.getTypeQualifier() == NA_DATETIME_TYPE))
      {
	if (((op2->getOperatorType() == ITM_REFERENCE) ||
	     (op2->getOperatorType() == ITM_BASECOLUMN)) &&
	    (NOT ((op1->getOperatorType() == ITM_REFERENCE) ||
		  (op1->getOperatorType() == ITM_BASECOLUMN))))
	  {
	    // op2 is column, op1 is not.
	    // convert op1(numeric) to the type of op2 (datetime)
	    srcOpIndex = 0;
	    tgtOpIndex = 1;
	    conversion = 5;
	  }
	else
	  {
	    // op1 is column and op2 is not,
	    // or both children are columns, or both children are non-columns.
	    // Convert op2(datetime) to numeric type(op1)
	    srcOpIndex = 1;
	    tgtOpIndex = 0;
	    conversion = 3;
	  }
    }

    //check for interval to numeric comparison
    if ((type1.getTypeQualifier() == NA_INTERVAL_TYPE) &&
	(type2.getTypeQualifier() == NA_NUMERIC_TYPE))
    {
      // convert op2 to interval type
      srcOpIndex = 1;
      tgtOpIndex = 0;
      conversion = 4;
    }
    else
    if ((type1.getTypeQualifier() == NA_NUMERIC_TYPE) &&
        (type2.getTypeQualifier() == NA_INTERVAL_TYPE))
    {
      // convert op1 to interval type
      srcOpIndex = 0;
      tgtOpIndex = 1;
      conversion = 4;
    }
    else
    if ((type1.getTypeQualifier() == NA_DATETIME_TYPE) &&
	(type2.getTypeQualifier() == NA_DATETIME_TYPE) &&
        (type1.getPrecision() != type2.getPrecision()))
    {
      conversion = 8;
      if (type1.getPrecision() == SQLDTCODE_TIMESTAMP)
        {
          // convert op2 to timestamp
          srcOpIndex = 1;
          tgtOpIndex = 0;
        }
      else
        {
          // convert op1 to timestamp
          srcOpIndex = 0;
          tgtOpIndex = 1;
        }
    }

    ItemExpr * newOp = NULL;

    switch (conversion)
    {
      case 1:
        //doing a char to numeric conversion
        // convert to double precision. This will handle all precision,
	// scale and type specified in the char value.
	newOp =
	  new (bindWA->wHeap())
	  Cast((srcOpIndex == 0 ? op1 : op2),
	    new (bindWA->wHeap())
	    SQLDoublePrecision(bindWA->wHeap(), (srcOpIndex == 0 ? op1 : op2)->castToItemExpr()->getValueId().getType().supportsSQLnull()));
    	newOp = newOp->bindNode(bindWA);
        break;

      case 2:
        //doing a char to date conversion
        newOp =
	  new (bindWA->wHeap())
	  Cast((srcOpIndex == 0 ? op1 : op2),
	       (tgtOpIndex == 0 ? op1 : op2)->castToItemExpr()->getValueId().getType().newCopy(bindWA->wHeap()));
    	newOp = newOp->bindNode(bindWA);
        break;

      case 3:
	{
	  //doing a date to numeric conversion
	  newOp =
	    new (bindWA->wHeap())
	    Cast((srcOpIndex == 0 ? op1 : op2),
		 new (bindWA->wHeap())
		 SQLLargeInt(bindWA->wHeap(), TRUE,
			     (srcOpIndex == 0 ? op1 : op2)->castToItemExpr()->
			     getValueId().getType().supportsSQLnull()));
	  newOp = newOp->bindNode(bindWA);
	}
        break;

      case 4:
	{
	  //doing a numeric to interval conversion
	  const NumericType&  numeric  = 
	    (NumericType&)(srcOpIndex == 0 ? op1 : op2)->castToItemExpr()->getValueId().getType();
	  const IntervalType& interval = 
	    (IntervalType&)(tgtOpIndex == 0 ? op1 : op2)->castToItemExpr()->getValueId().getType();
	  Lng32 maxDigits = (numeric.getMagnitude() + 9) / 10;
	  maxDigits = MINOF(maxDigits, 
			    SQLInterval::MAX_LEADING_PRECISION);
	  
	  SQLInterval * newInterval =  
	    new(bindWA->wHeap()) SQLInterval(
                 bindWA->wHeap(),
		 numeric.supportsSQLnull(),
		 interval.getEndField(),
		 maxDigits,
		 interval.getEndField(),
		 0);
	  
	  newOp =
	    new (bindWA->wHeap())
	    Cast((srcOpIndex == 0 ? op1 : op2), newInterval);
	  newOp = newOp->bindNode(bindWA);
	}
        break;

      case 5:
        //doing a numeric to date conversion
        newOp =
	  new (bindWA->wHeap())
	  Cast((srcOpIndex == 0 ? op1 : op2),
	       (tgtOpIndex == 0 ? op1 : op2)->castToItemExpr()->getValueId().getType().newCopy(bindWA->wHeap()));
    	newOp = newOp->bindNode(bindWA);
        break;

    case 6:
    case 7:
      // doing char to date formatting.
      newOp =
	new (bindWA->wHeap())
	Format((srcOpIndex == 0 ? op1 : op2), "DD-MON-YYYY", TRUE);
      newOp = newOp->bindNode(bindWA);

      if (conversion == 7)
	{
	  newOp =
	    new (bindWA->wHeap())
	    Cast(newOp,
		 (tgtOpIndex == 0 ? op1 : op2)->castToItemExpr()->getValueId().getType().newCopy(bindWA->wHeap()));
	  newOp = newOp->bindNode(bindWA);
	}
      break;
      
    case 8:
      newOp =
	new (bindWA->wHeap())
	Cast((srcOpIndex == 0 ? op1 : op2),
	     (tgtOpIndex == 0 ? op1 : op2)->castToItemExpr()->getValueId().getType().newCopy(bindWA->wHeap()));
      newOp = newOp->bindNode(bindWA);
      break;
     
      default:
        break;
    }

    if(bindWA->errStatus()) 
      return NULL;

    newOp1 = NULL;
    newOp2 = NULL;
    if (newOp)
      {
	if (srcOpIndex == 0)
	  newOp1 = newOp;
	else
	  newOp2 = newOp;
      }
  }

  return thisPtr;
}


// if we are allowing certain incompatible comparisons handle them.
// currently the following incompatible comparisons are supported:
// 1. Numeric and Character
// 2. Date and Character literal
BiRelat * BiRelat::handleIncompatibleComparison(BindWA *bindWA)
{
  ItemExpr * newChild0 = NULL;
  ItemExpr * newChild1 = NULL;
  ItemExpr * result = 
    ItemExpr_handleIncompatibleComparison(
	 bindWA,
	 this,
	 child(0)->castToItemExpr(), child(1)->castToItemExpr(),
	 newChild0, newChild1);
  if (! result)
    return NULL;

  if(bindWA->errStatus()) 
    return NULL;
  
  if (newChild0)
    setChild(0, newChild0);
  if (newChild1)
    setChild(1, newChild1);

  return this;
}

ItemExpr *KeyRangeCompare::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) return this;

  CollIndex i = 0;


  // In the case of CLUSTERING KEY option (child(0) == NULL),
  // see if the objectName exists in the RetDesc. This is done to support
  // queries of the form - "select * from t_4a1 as X where
  //           Key_Range_Compare (CLUSTERING KEY >= (1,2) on table X);".
  // The X within the KeyRangeCompare resolves to t_4a1.
  //
  // In the case of PARTITIONING KEY option use the object name that
  // the user provides.
  // This is to support the requirements from Online Populate Index, where
  // KEY_RANGE_COMAPRE is done to see which partition a row belongs
  // to in the index. The key values are obtained from the Audit Image of the
  // table. Example:
  // select case
  // when KEY_RANGE_COMPARE(PARTITIONING KEY(X.C2, X.C3) < ('l',1000)
  //					ON INDEX_TABLE t_indx)
  //	THEN 0
  //    ELSE -1
  //	END
  // from (SELECT c2,c3 from table(INTERPRET_AS_ROW(:tableAuditImage,....))
  //  AS X(C2,C3)).
  //

  if (child(0) == NULL)
    {
      TableNameMap *exposedTableName = NULL;
      LIST(TableNameMap*) xtnmList(bindWA->wHeap());
      bindWA->getTablesInScope(xtnmList, NULL);

      if ((getObjectName().isEmpty()) && (xtnmList.entries() == 1))
      { // if no table was specified and there is only one table in scope
	// then use that table.
	exposedTableName = xtnmList[0];
      }
      else
      {
	RETDesc * retDesc = bindWA->getCurrentScope()->getRETDesc();
	exposedTableName = retDesc->getXTNM().get(&objectName_);

	if (!exposedTableName)
	{
	  // Table specified for KeyRangeCompare is not in scope.
	  retDesc->getTableList(xtnmList, NULL);
	  NAString fmtdList(bindWA->wHeap());
	  RETDesc::formatTableList(xtnmList, &fmtdList, TRUE);  // include partition names, if present

	  NAString objectNameAsString;
	  if (objectName_.hasPartnClause())
	    objectNameAsString = objectName_.getExposedNameAsStringWithPartitionNames();
	  else
	    objectNameAsString = objectName_.getExposedNameAsString();

	  *CmpCommon::diags() << DgSqlCode(-4332)
			      << DgTableName(objectNameAsString)
			      << DgString0(fmtdList);
	  bindWA->setErrStatus();
	  return this;
	}
      } // table name was specified or we have more than one table in scope.

      setObjectName(exposedTableName->getTableName());
    }

  // Obtain the NATable for the objectName.
  NATable *naTable = bindWA->getNATable(objectName_);
  if (bindWA->errStatus())
    return this;
  naTable->decrReferenceCount(); // refcount need not be incremented for this use of natable

  // Key_Range_Compare disallowed on views.
  if (naTable->getViewText() != NULL)
    {
      *CmpCommon::diags() << DgSqlCode(-4334);
     bindWA->setErrStatus();
     return this;
    }

  // The constructor sets the specialNulls_ flag to TRUE
  // Initialize the direction vector here. Set it up
  // later in the procedure.
  IntegerList *directionVector = new(bindWA->wHeap()) IntegerList();

  // Setup the LHS of the comparision predicate
  // Currently, we require the users to specify the LHS for
  // partition keys, but not for clustering keys.
  // child0 will be NULL only for the clustering key comparision.
   if (child(0) == NULL)
    {
      // Get the Clustering Key Columns.
      const NAColumnArray & clustKeyColumns =
	naTable->getClusteringIndex()->getIndexKeyColumns();

      CorrName qualifiedName(objectName_, bindWA->wHeap());
      // since these CKs colReferences are created in the Binder
      // don't fix the name in LocList.
      qualifiedName.setNamePosition(0);

      // Create an item list and set it as the LHS.
      ItemExpr *clustKeyColList = new (bindWA->wHeap())
	ColReference(new(bindWA->wHeap()) ColRefName
		     (clustKeyColumns[0]->getColName(),
		      qualifiedName,
		      bindWA->wHeap()));

      for (i = 1; i < clustKeyColumns.entries(); i++)
	clustKeyColList = new (bindWA->wHeap())
	  ItemList(clustKeyColList, (ItemExpr *) new (bindWA->wHeap())
		   ColReference(new(bindWA->wHeap()) ColRefName
				(clustKeyColumns[i]->getColName(),
				 qualifiedName,
				 bindWA->wHeap())));

      // Set that as child(0)
      child(0) = clustKeyColList;

      // Setup the direction vector.
      for (i=0; i < clustKeyColumns.entries(); i++)
      {
	if (clustKeyColumns.isAscending(i))
	  directionVector->insert(1);
	else
	  directionVector->insert(-1);
      }
    }
  else
    {
      const PartitioningFunction *partFunc =
	naTable->getClusteringIndex()->getPartitioningFunction();

      if (partFunc->isATableHashPartitioningFunction())
	{
	  // Table is Hash or Hash2 partitioned. Partitioning Keys
	  // comparison is not allowed on hash partitioned tables.
	  *CmpCommon::diags() << DgSqlCode(-4331)
			      << DgTableName(objectName_.getExposedNameAsAnsiString());
	  bindWA->setErrStatus();
	  return this;
	}

      // Partition Key is being specified.
      const NAColumnArray &partKeyCols =
	naTable->getClusteringIndex()->getPartitioningKeyColumns();

      if(!verifyPartitioningKeys(bindWA,
				 child(0),
				  partKeyCols,
				  bindWA->wHeap()))
	{
	  bindWA->setErrStatus();
	  return this;
	}

      for (CollIndex i2=0; i2 < partKeyCols.entries(); i2++)
	{
	  if (partKeyCols.isAscending(i2))
	    directionVector->insert(1);
	  else
	    directionVector->insert(-1);
	}
    }

  setDirectionVector(directionVector);

  // KeyRangeCompare inherits from BiRelat
  return BiRelat::bindNode(bindWA);
}

NABoolean KeyRangeCompare::verifyPartitioningKeys(BindWA *bindWA,
						  ItemExpr *tree,
						   const NAColumnArray &partKeyCols,
						  CollHeap *heap)
{
  ExprValueIdList *list = new (heap) ExprValueIdList(heap);

  if (tree->getOperatorType() != ITM_ITEM_LIST)
    {
      ExprValueId exprTree(tree);
      // There is only one element.
      list->insert(&exprTree);
    }
  else
    {
      list = ((ItemList *)tree)->collectLeaves(heap, list);
    }

  const ULng32 num = list->entries();

  if (partKeyCols.entries() != num) {
    // 4042 The operands of a comparison predicate must be of equal degree.
    *CmpCommon::diags() << DgSqlCode(-4335)
			<< DgInt0((Lng32) num)
			<< DgInt1((Lng32) partKeyCols.entries())
			<< DgTableName(objectName_.getExposedNameAsAnsiString());
    return FALSE;
  }


  // Make sure that the types of items in tree match the partKeyCols types.
  // We do this by temporarily creating a cast node and binding it.
  for (ULng32 i = 0; i < num; i++)
    {
      ItemExpr *cast = new (bindWA->wHeap()) Cast((*list)[i]->getPtr(),
						  partKeyCols[i]->getType(),
						  ITM_CAST);
      cast = cast->bindNode(bindWA);
      if (bindWA->errStatus())
	return FALSE;

      delete cast;

    }
  return TRUE;
}

ItemExpr *Function::bindNode(BindWA *bindWA)
{
  if (checkForSQLnullChild(bindWA, this, allowsSQLnullArg(), FUNCTION_))
    return this;

  ItemExpr *save = bindWA->getCurrentScope()->context()->inMultaryPred();
  bindWA->getCurrentScope()->context()->inMultaryPred() = this;
  ItemExpr *boundExpr = ItemExpr::bindNode(bindWA);
  bindWA->getCurrentScope()->context()->inMultaryPred() = save;
  return boundExpr;
} // Function::bindNode()


ItemExpr *Overlaps::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();

  bindChildren(bindWA);
  if (bindWA->errStatus())
    return this;
  //Syntax Rules:
  // 1) ... 2)...
  // 3)...
  //   Case: 
  //   a) If the declared type is INTERVAL, then the precision of the declared type 
  //      shall be such that the interval can be added to the datetime data type of 
  //      the first column of the <row value predicand>.
  //   b) If the declared type is a datetime data type, then it shall be comparable
  //      with the datetime data type of the first column of the <row value predicand>.
  const NAType &type1 =
    child(1)->castToItemExpr()->getValueId().getType();

  if (type1.getTypeQualifier() == NA_INTERVAL_TYPE)
  {
    ItemExpr * newChild = new (bindWA->wHeap())    
      BiArith(ITM_PLUS, child(0), child(1));
    child(1) = newChild->bindNode(bindWA);
    if (bindWA->errStatus())
      return this;
  }

  const NAType &type3 =
    child(3)->castToItemExpr()->getValueId().getType();
  if (type3.getTypeQualifier() == NA_INTERVAL_TYPE)
  {
    ItemExpr * newChild = new (bindWA->wHeap())    
      BiArith(ITM_PLUS, child(2), child(3));
    child(3) = newChild->bindNode(bindWA);
    if (bindWA->errStatus())
      return this;
  }



  BuiltinFunction::bindNode(bindWA);
  if (bindWA->errStatus())
    return this;

  return getValueId().getItemExpr();
}


ItemExpr *Between::bindNode(BindWA *bindWA)
{
  //changes for HistIntRed

  //save the current state of inRangePred_ in the binder
  //context so that we can set it back after binding this
  //node in case we reset it the if statement below
  NABoolean inRangePredSave = bindWA->getCurrentScope()->context()->inRangePred();
  bindWA->getCurrentScope()->context()->inRangePred() = TRUE;

  ItemExpr * boundExpr = BuiltinFunction::bindNode(bindWA);

  if((boundExpr) &&
     (!bindWA->errStatus()) &&
     (!handleIncompatibleComparison(bindWA)))
    return NULL;

  //set inRangePred_ in the context to original value
  bindWA->getCurrentScope()->context()->inRangePred() = inRangePredSave;

  // Apply the substr transformation, if possible
  if (child(0)->getOperatorType() == ITM_SUBSTR AND
      child(1)->getOperatorType() == ITM_CONSTANT AND
      child(2)->getOperatorType() == ITM_CONSTANT AND
      CmpCommon::getDefault(SUBSTRING_TRANSFORMATION) == DF_ON)
  {
     ItemExpr * bt = checkAndApplySubstrTransformation();
     if ( bt ) {
       boundExpr = bt->bindNode(bindWA);
     }
  }

  return boundExpr;
}

// Check out the possibilty of SUBSTR optimization, which
// convert the predicate
//
//   substr(PK_column_fix_char, 1, n) between literal1 and literal2
//
// into the following
//
//   PK_column_fix_char between c1 and c2 
//
//   where c1 = literal1 paded with character 0x00
//                     and
//         c2 = literal2 paded with character 0xff
//
// The transformation is applied when the following is true
//
// 1. PK_column_fix_char is leading key fixed ISO88591 char column;
// 2. strlen(literal1) = strlen(literal2) = length of substr();
// 3. 1 <= n <= column length of PK_column_fix_char
//
// The method returns the transformed between expr if the transformation
// can be done. Otherwise, a NULL is returned.
//
ItemExpr* Between::checkAndApplySubstrTransformation()
{
   ItemExpr* substr = child(0)->castToItemExpr();
   ItemExpr* literal1 = child(1)->castToItemExpr();
   ItemExpr* literal2 = child(2)->castToItemExpr();

   NAMemory *heap = CmpCommon::statementHeap();

   Int32 column_len = 0; 
   NABoolean col_support_null = FALSE;
   Lng32  substr_len = 0;

   // check substr() first
   ItemExpr* c = substr->child(0)->castToItemExpr();
   BaseColumn* baseCol = NULL;

   // skip the CAST operator, if any 
   if ( c->getOperatorType()==ITM_CAST )
      baseCol = (BaseColumn*)(c->child(0)->castToItemExpr());
   else
      baseCol = (BaseColumn*)c;

   // c in SUBSTR(c, a, b) must be a basecolumn
   if ( baseCol->getOperatorType()==ITM_BASECOLUMN )
   {
     const NAType &coltype = (baseCol->getValueId()).getType();

     // c should be a leading key base column with fixed ISO88591 char data type
     if ( coltype.getTypeQualifier() != NA_CHARACTER_TYPE ||
          ((const CharType&)(coltype)).getCharSet() != CharInfo::ISO88591 ||
          !DFS2REC::isSQLFixedChar(((const CharType&)(coltype)).getFSDatatype())
      )
       return NULL;

     // c must be a key column
     const NAColumn* naCol = baseCol->getNAColumn();
     if (naCol==NULL || !(naCol->isPrimaryKey())) return NULL;


     const NATable* naTable = naCol->getNATable();
     const NAColumnArray & clustKeyColumns =
                   naTable->getClusteringIndex()->getIndexKeyColumns();

     // c must be a leading key column
     if ( clustKeyColumns.entries() < 1 ||
          clustKeyColumns[0]->getPosition() != baseCol->getColNumber() )
       return NULL;

/*
     const ValueIdList keyCols = 
        baseCol->getTableDesc()->getClusteringIndex()->getClusteringKeyCols();

     if ( keyCols.entries() == 0 || 
          keyCols[0].getItemExpr()->getOperatorType() != ITM_INDEXCOLUMN )
       return NULL;

     IndexColumn* indexCol = (IndexColumn*)(keyCols[0].getItemExpr());
     if ( indexCol->getIndexColNumber() != baseCol->getColNumber() )
       return NULL;
*/

     column_len = ((const CharType&)(coltype)).getStrCharLimit();

     // Get the number of characters returned by the SUBSTR 
     substr_len = 
       ((const CharType&)((substr->getValueId()).getType())).getStrCharLimit();

     // Column length must be at least substr_len long
     if ( column_len < substr_len )
       return NULL;

     col_support_null = coltype.supportsSQLnullLogical();
   } else
       return NULL;

   // The 2nd argument of substr should be 1
   if ( substr->child(1)->getOperatorType()==ITM_CONSTANT ) {

      const ConstValue* cv1 = 
         (const ConstValue*)(substr->child(1)->castToItemExpr());

      if ( cv1->canGetExactNumericValue() AND
           cv1->getExactNumericValue() != 1 ) 
         return NULL;
   } else
      return NULL;

   // The 3rd argument of substr should be >= 1.
   if ( substr->child(1)->getOperatorType()==ITM_CONSTANT ) {

      const ConstValue* cv2 =
         (const ConstValue*)(substr->child(2)->castToItemExpr());

      if ( cv2->canGetExactNumericValue() AND
           cv2->getExactNumericValue() < 1 )
         return NULL;
   } else
      return NULL;

   // literal1 must be of character type and its length must be
   // equal to that of the synthesized SUBSTR().
   const NAType &literal1_type = 
        literal1->castToItemExpr()->getValueId().getType();

   if ( literal1_type.getTypeQualifier() != NA_CHARACTER_TYPE ||
        substr_len != ((const CharType&)literal1_type).getStrCharLimit() 
      )
      return NULL;

   // literal2 must be of character type and its length must be
   // equal to that of the synthesized SUBSTR().
   const NAType &literal2_type = 
        literal2->castToItemExpr()->getValueId().getType();

   if ( literal2_type.getTypeQualifier() != NA_CHARACTER_TYPE ||
        substr_len != ((const CharType&)literal2_type).getStrCharLimit() 
      )
     return NULL;


   // Now modify the between tree
   // ----------------------------

   // Figure out the number of character 0x00 or 0xff to pad
   Int32 chars_to_pad = column_len - substr_len;

   // modify the Construct the new constant 
   ConstValue* cv = (ConstValue*)literal1;
   NAString cstr1(*(cv->getRawText()));
   cstr1.append((const char)0x00, chars_to_pad);
   ConstValue *c1 = new (heap) ConstValue(cstr1, heap);

   cv = (ConstValue*)literal2;
   NAString cstr2(*(cv->getRawText()));
   cstr2.append((const char)0xff, chars_to_pad);

   ConstValue *c2 = new (heap) ConstValue(cstr2, heap);

   child(0)= baseCol;
   child(1)= c1;
   child(2)= c2;

   // make sure this gets bound again
   unBind();

   return this;
}

// -----------------------------------------------------------------------
// member functions for class BitOperFunc
// -----------------------------------------------------------------------
ItemExpr *BitOperFunc::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();

  return BuiltinFunction::bindNode(bindWA);
} // BitOperFunc::bindNode()

Between * Between::handleIncompatibleComparison(BindWA * bindWA)
{
  ItemExpr * newChild0 = NULL;
  ItemExpr * newChild1 = NULL;
  ItemExpr * newChild2 = NULL;
  ItemExpr * result = 
    ItemExpr_handleIncompatibleComparison(
	 bindWA,
	 this,
	 child(0)->castToItemExpr(), child(1)->castToItemExpr(),
	 newChild0, newChild1);
  if (! result)
    return NULL;

  if(bindWA->errStatus()) 
    return NULL;
  
  result = 
    ItemExpr_handleIncompatibleComparison(
	 bindWA,
	 this,
	 child(0)->castToItemExpr(), child(2)->castToItemExpr(),
	 newChild0, newChild2);
  if (! result)
    return NULL;

  if(bindWA->errStatus()) 
    return NULL;
  
  if (newChild0)
    setChild(0, newChild0);
  if (newChild1)
    setChild(1, newChild1);
  if (newChild2)
    setChild(2, newChild2);
  
  return this;
}

ItemExpr *BuiltinFunction::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    {
      // 10-100719-1893
      // Look for the replacementExpr if the builtinFunction is already
      // bound. Solves the issue where we forget that we have transformed
      // the function. For example NVL() -> CASE().

      ItemExpr * retExpr = getValueId().getItemExpr()->getReplacementExpr();
      if (retExpr != NULL) 
        return retExpr;
      else
        return getValueId().getItemExpr();
    }

  //////////////////////////////////////////////////////////////////////
  // Due to the many problems with rand() implementations we decided to
  // disable this method for R2.0. The last known problem with rand()
  // caused data corruption.
  // We will re-enable rand() after it get fixed.
  // Also we provide special CQD to allow R1.8 users who may already used
  // rand() and have scripts depending on it to use it in R2.0.
  // We need also to make sure we are only disabling user rands (binder
  // phase rands) and not  system generated rands.
  ///////////////////////////////////////////////////////////////////////
  if ( getOperatorType() == ITM_RANDOMNUM )
    {
      // if this builtinfunction is a RandomNum(), then it is safe to cast
      // the this pointer to (Random *). 
      if (CmpCommon::getDefault(ALLOW_RAND_FUNCTION) != DF_ON AND
	  QueryAnalysis::Instance() AND
	  QueryAnalysis::Instance()->getCompilerPhase() == QueryAnalysis::BINDER)
	{
	  *CmpCommon::diags() << DgSqlCode(-4313);
	  bindWA->setErrStatus();
	  return NULL; // error
	}
    }

  // this will bind/type-propagate all children.
  bindChildren(bindWA);
  if (bindWA->errStatus()) 
    return this;

  if ( getOperatorType() != ITM_BETWEEN )
    {
      // Reverify inputs
      // This is to deal with cases where a builtin function requires one 
      // input, but the input given is a subquery with degree greater than 1 or
      // a MultiValuedFunction(MVF). In this case the parser would not flag it,
      // since we don't know the degree of those until after bind time.
      // Here we check again..
      
      // We don't need to do this for between because comparison expandsions
      // will detect it.
      
      // XXX If there are builtin functions that requires two inputs and we 
      // want to allow specifying just an mvf or subquery of degree 2, we need
      // to change the parser...
      
      Int32 childDegree = 0;
      Subquery * subq = NULL;
      UDFunction * udf = NULL;
      Int32 origArity = getArity();
      
      for (Int32 chld=0; chld < origArity; chld++)
        {
       
          switch (child(chld)->getOperatorType())
            {
            case ITM_ROW_SUBQUERY:
              subq = (Subquery *) child(chld)->castToItemExpr();
              childDegree += subq->getSubquery()->getDegree();
              break;
            
            case ITM_USER_DEF_FUNCTION:
              udf = (UDFunction *) child(chld)->castToItemExpr();
              childDegree += udf->getRoutineDesc()->getOutputColumnList().entries();
              break;
            
            default:
              childDegree += 1;
              break;
            }
          
        }
      
      if (childDegree > origArity) 
      {
        NAString upperFunc(getText(), bindWA->wHeap());
        
        upperFunc.toUpper();
        *CmpCommon::diags() << DgSqlCode(-4479) << DgString0(upperFunc) 
                            << DgInt1(origArity) << DgInt2(childDegree);
        
        bindWA->setErrStatus();
        return NULL;
      }
    } // if ITM_BETWEEN

  ItemExpr * retExpr = NULL;
  NABoolean useCase = FALSE;
  ItemExpr * ie = NULL;
  switch (getOperatorType())
    {

    case ITM_ISIPV4:
    case ITM_ISIPV6:
    case ITM_MD5:
    case ITM_CRC32:
    case ITM_SHA1:
    case ITM_SOUNDEX:
    case ITM_SHA2_224:
    case ITM_SHA2_256:
    case ITM_SHA2_384:
    case ITM_SHA2_512:
      {
         break;
      }
    case ITM_NULLIFZERO:
      {
	// binder has already verified that child is numeric
	const NumericType &type_op1 = (NumericType&)
	  (child(0)->castToItemExpr()->getValueId().getType());

	if ((type_op1.isComplexType()) ||
	    (NOT (type_op1.isExact())) ||
	    (type_op1.isDecimal()))
	  {
	    Parser parser(bindWA->currentCmpContext());

	    retExpr =
	      parser.getItemExprTree(
		   "CASE WHEN @A1 <> 0 then @A1 ELSE NULL END;",
		   0, BINDITEMEXPR_STMTCHARSET, 1, child(0));
	  }
	break;
      }

    case ITM_NVL:
      {
	// if my children's attributes EXCEPT for nullability are not the
	// same as mine, use a CASE stmt.
	const NAType &typ1 =
	  child(0)->castToItemExpr()->getValueId().getType();
	const NAType &typ2 =
	  child(1)->castToItemExpr()->getValueId().getType();

	NABoolean useCase = FALSE;
	if (NOT typ1.isCompatible(typ2))
	  useCase = TRUE;
	else if ((child(0)->castToItemExpr()->isASubquery()) ||
	    (child(1)->castToItemExpr()->isASubquery()))
	  {
	    useCase = TRUE;
	  }
	else if (typ1.getTypeQualifier() == NA_CHARACTER_TYPE)
	  {
	    //
	    // For character types, if the collation is not the same for
	    // the two arguments to NVL, then revert to a CASE stmt.
	    //
	    const CharType &cTyp1 = (CharType&)typ1;
	    if ( cTyp1.getCollation() != ((CharType&)typ2).getCollation() )
	      useCase = TRUE;
	  }
	if ( ( NOT useCase) && (NOT typ1.supportsSQLnull()) )
	  {
	    retExpr = child(0)->castToItemExpr();
	    break;    // That's all that this NVL needs.
	  }
	if ( NOT useCase )
	  {
	    if (typ2.supportsSQLnull())
	      useCase = TRUE;
            // convert NVL to CASE for all CHAR types, including fixed CHARs.
            // This is necessary because query caching does limited type
            // synthesization when replacing a fixed char literal with a varchar
            // typed parameter. In fact, if NVL appears inside
            //       "select nvl(t.c1, 'a') from t"
            // the type of nvl() is assumed to be that of t.c1 if no conversion
            // is done. If t.c1 is fixed char, then type will mismatch when
            // the execution of the cached plan delivers a varchar value. The
            // length value will appear as 1st two bytes of the output value!
            //
	    else if ((DFS2REC::isAnyCharacter(typ1.getFSDatatype())) ||
		     (DFS2REC::isAnyCharacter(typ2.getFSDatatype())))
	      useCase = TRUE;
	    else
	      {
		// typ1 is nullable and typ2 is non-nullable.
		// create a new typ1 with the same null attr as typ2
		// and the same coercibilit as typ2.
		NAType * newTyp1 = typ1.newCopy(bindWA->wHeap());
		newTyp1->resetSQLnullFlag();
		if (newTyp1->getTypeQualifier() == NA_CHARACTER_TYPE)
		  ((CharType*)newTyp1)->setCoercibility(((CharType&)typ2).getCoercibility());
		if (NOT(*newTyp1 == typ2))
		  {
		    ie = Function::bindNode(bindWA);
		    if (bindWA->errStatus()) 
		      return NULL;

		    if (*newTyp1 == getValueId().getType())
		      {
			// Left child is the same type as result.
			// Insert a cast node to convert right child to
			// result type.
			child(1) = new (bindWA->wHeap())
			  Cast(child(1), &getValueId().getType());
			child(1)->bindNode(bindWA);
		      }
		    else
		      useCase = TRUE;
		  }
		delete newTyp1;
	      }

	  }

	if (useCase)
	  {
	    Parser parser(bindWA->currentCmpContext());

	    retExpr =
	      parser.getItemExprTree(
		   "CASE WHEN @A1 is null then @A2 ELSE @A1 END;",
		   0, BINDITEMEXPR_STMTCHARSET, 2, child(0), child(1));
	  }

	break;
      }
    case ITM_JSONOBJECTFIELDTEXT:
    {
        break;
    }
    case ITM_QUERYID_EXTRACT:
      {
        // type cast any params
	ValueId vid1 = child(0)->getValueId();
	SQLChar c1(NULL, ComSqlId::MAX_QUERY_ID_LEN);
	vid1.coerceType(c1, NA_CHARACTER_TYPE);
        
        ValueId vid2 = child(1)->getValueId();
	SQLChar c2(NULL, 40, FALSE);
	vid2.coerceType(c2, NA_CHARACTER_TYPE);

	const CharType &typ1 = (CharType&)child(0)->getValueId().getType();
	const CharType &typ2 = (CharType&)child(1)->getValueId().getType();
	
        if (typ1.getTypeQualifier() != NA_CHARACTER_TYPE || typ2.getTypeQualifier() != NA_CHARACTER_TYPE)
        {
          // 4043 The operand of a $0~String0 function must be character.
          *CmpCommon::diags() << DgSqlCode(-4043) << DgString0(getTextUpper());
          bindWA->setErrStatus();
          return NULL;
        }
        CharInfo::CharSet chld_cs = ((const CharType&)typ1).getCharSet();
        if ( chld_cs == CharInfo::UNICODE )
        {
           child(0) =
             new (bindWA->wHeap()) Translate(child(0), Translate::UNICODE_TO_ISO88591);
           child(0) = child(0)->bindNode(bindWA);
        }
        chld_cs = ((const CharType&)typ2).getCharSet();
        if ( chld_cs == CharInfo::UNICODE )
        {
           child(1) =
             new (bindWA->wHeap()) Translate(child(1), Translate::UNICODE_TO_ISO88591);
           child(1) = child(1)->bindNode(bindWA);
        }
        if (bindWA->errStatus())
          return NULL;
        // Right child is the string containing the query id attribute that
	// needs to be extracted. 
	// Upshift it so it could be compared at runtime.
	child(1) = new (bindWA->wHeap()) Upper(child(1));
	child(1) = child(1)->bindNode(bindWA);
        if (bindWA->errStatus())
          return NULL;
      }
    break;

    case ITM_ASCII:
      {
         // Since the ASCII(<str_expr>) function requires an ISO88591 string arg,
         // but we want to allow unprefixed string literals (which may be UCS2
         // when running on a system that is using the Unicode Config),
         // we need the following code.
      }
    break;

    case ITM_AES_ENCRYPT:
    case ITM_AES_DECRYPT:
    case ITM_ENCODE_BASE64:
    case ITM_DECODE_BASE64:
      break;
    default:
      {
      }
      break;
    } // switch

  if (retExpr)
    {
      // Make sure the original expression is bound
      ie = Function::bindNode(bindWA);
      if (bindWA->errStatus()) 
        return NULL;


      // then bind the replacement expression
      ie = retExpr->bindNode(bindWA);
      if (bindWA->errStatus())
	return NULL;
    }
  else
    {
      if (getOperatorType() == ITM_EXTRACT || 
          getOperatorType() == ITM_EXTRACT_ODBC)
        {
          if (isAUserSuppliedInput())
            {
              ItemExpr * extractChild = NULL;
              if ( child(0) && (child(0)->getOperatorType() == ITM_CAST) && 
                   child(0)->child(0) )
                extractChild = child(0)->child(0)->castToItemExpr();
              else if (child(0))
                extractChild = child(0)->castToItemExpr();

              const NAType *type = synthTypeWithCollateClause(bindWA);
              if (!type) return this;

              char xFld[2]; str_itoa(((Extract*) this)->getExtractField(),xFld);
              char xVid[7]; str_itoa(extractChild->getValueId(),xVid);
              NAString functionText = getText() + xFld + "-" + xVid;
              ie = ItemExpr::bindUserInput(bindWA,type,functionText);
              if (bindWA->errStatus())
                return this;

              // add this value id to BindWA's input function list.
              bindWA->inputFunction().insert(getValueId());
              return ie;
            }
        } // !extract

      ie = Function::bindNode(bindWA);
      if (bindWA->errStatus()) 
        return NULL;
    } // !retExpr

  //////////////////////////////////////////////////////////////////////
  // Put a special Cast node on top of each child node that is a column.
  // Do not do this if this function is a Cast function.
  // This special Cast function casts the child to its existing
  // data attributes. Later, if the child is replaced by a VEG and
  // resolved to a type with different attribute in preCodeGen,
  // this cast will convert it to its original data attributes.
  // If the child was not resolved to a different type, then precodegen
  // ignores this cast and replaces it with the original child.
  ///////////////////////////////////////////////////////////////////////
  NABoolean doCastFix = TRUE;
  if ((doCastFix) && (protectFromVEGs()))
    {
      for (Int32 i = 0; i < ie->getArity(); i++)
	{
	  if ((child(i)) &&
	      ((child(i)->getOperatorType() == ITM_REFERENCE) ||
	       (child(i)->getOperatorType() == ITM_BASECOLUMN)))
	    {
	      ItemExpr * newChild =
		new (bindWA->wHeap()) Cast(ie->child(i),
					   &ie->child(i)->getValueId().getType());

	      // mark this cast so we do not generate code for it if its attrs
	      // are the same as its child's.
	      ((Cast*)newChild)->setMatchChildType(TRUE);

	      newChild = newChild->bindNode(bindWA);
	      if (bindWA->errStatus()) return NULL;
	      ie->setChild(i, newChild);
	    }
	}
    }

  // 10-100719-1893
  // set the replacementExpr so that we remember any substitution done
  // for example a NVL() expression gets translated to a CASE expression
  // and we want to remember this so that any references to the same NVL()
  // gets replaced correctly. AVG(NVL()) is a good example as the AVG() itself
  // gets translated to SUM(NVL())/COUNT(NVL()). If we don't remember the
  // CASE translation of the NVL(), then only the SUM() reference to the NVL()
  // expression getting translated correctly. 

  setReplacementExpr(ie);

  return ie;
}


ItemExpr *Upper::bindNode(BindWA *bindWA)
{
  // solution 10-040212-3234: the upshift transformation is happening
  // in code generator. This is too late. Suppose a relational operator
  // is outputting the Upper ItemExpression node. Now suddenly, it has
  // to output the child. This presents a few problems. Perhaps, the
  // transformatin can't happen no later than the normalization when the
  // outputs are decided.

  if (nodeIsBound())
    return getValueId().getItemExpr();

  // Upper inherits from BuiltinFunction .. Function .. ItemExpr.
  ItemExpr *boundExpr = BuiltinFunction::bindNode(bindWA);
  if (bindWA->errStatus()) return this;

  // If our child is already upshifted, we can remove this Upper from the tree.
  // BuiltinFunction::bindNode might have added a Cast node underneath Upper
  // with the MATCH_CHILD_TYPE flag set. We need to remove it too
  // else we can get into trouble with unmapped ValueIds at code generation
  // time. (e.g. If a hash join equi-join predicate refers to the Cast, the
  // Cast expression will be in the join child's characteristic outputs but
  // when we generate the join predicate we skip the Cast looking for the
  // underlying column. That might not be in the join child's characteristic
  // outputs.)
  if (boundExpr == this) {
    CMPASSERT(getArity() == 1);
    ValueId opVid = child(0)->getValueId();
    ItemExpr * child0ie = opVid.getItemExpr();
    if (child0ie->getOperatorType() == ITM_CAST)
      {
        Cast * child0ieCast = (Cast *)child0ie;
        if (child0ieCast->matchChildType()) 
          // we need to remove the Cast too
          opVid = child0ieCast->child(0)->getValueId();
      }
    const CharType &ct = (const CharType &)opVid.getType();
    CMPASSERT(ct.getTypeQualifier() == NA_CHARACTER_TYPE);
    if (ct.isUpshifted()) setValueId(opVid);
  }
  return getValueId().getItemExpr();
}

ItemExpr *Abs::bindNode(BindWA *bindWA)
{
  // solution 1438 in Bugzilla: If my child is unsigned 
  // then no Abs function is necessary. It seems safer to 
  // remove the Abs from the query tree as early as possible.
  // Runtime eval method expects child of Abs to be signed,
  // so if this transformation is not made we get a runtime
  // internal error.

  if (nodeIsBound())
    return getValueId().getItemExpr();

  // Abs inherits from MathFunc .. BuiltinFunction .. Function .. ItemExpr.
  ItemExpr *boundExpr = MathFunc::bindNode(bindWA);
  if (bindWA->errStatus()) return this;

  // If our child is unsigned, we can remove this Abs from the tree.
  if (boundExpr == this) {
    CMPASSERT(getArity() == 1);
    ValueId opVid = child(0)->getValueId();
    const NumericType &nt = (const NumericType &)opVid.getType();
    CMPASSERT(nt.getTypeQualifier() == NA_NUMERIC_TYPE);
    if (nt.isUnsigned()) setValueId(opVid);
  }
  return getValueId().getItemExpr();
}

ItemExpr *CharFunc::bindNode(BindWA *bindWA)
{
  switch ( charSet_ )
  {
    case CharInfo::UNICODE:
      setOperatorType(ITM_UNICODE_CHAR);
      break;

    case CharInfo::KANJI_MP:
    case CharInfo::KSC5601_MP:
      setOperatorType(ITM_NCHAR_MP_CHAR);
      break;

    default:
      break;
  }

  if (!CharInfo::isCharSetSupported(charSet_)) {
    // 3010 Character set $0~string0 is not yet supported.
    // 4062 The preceding error actually occurred in function $0~String0.
    *CmpCommon::diags() << DgSqlCode(-3010)
      << DgString0(CharInfo::getCharSetName(charSet_));
  NAString unparsed(bindWA->wHeap());
    unparse(unparsed, DEFAULT_PHASE, USER_FORMAT_DELUXE);
    *CmpCommon::diags() << DgSqlCode(-4062) << DgString0(unparsed);
    bindWA->setErrStatus();
    return NULL;
  }

  // CharFunc inherits from BuiltinFunction .. Function .. ItemExpr.
  return BuiltinFunction::bindNode(bindWA);
}

ItemExpr *Concat::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();

  // this will bind/type-propagate all children.
  bindChildren(bindWA);
  if (bindWA->errStatus()) 
    return this;

  if (CmpCommon::getDefault(ALLOW_INCOMPATIBLE_OPERATIONS) == DF_ON)
  {
    const NAType &type1 = 
      child(0)->castToItemExpr()->getValueId().getType();
    const NAType &type2 = 
      child(1)->castToItemExpr()->getValueId().getType();
    
    // allow DATE || CHAR (in mode_special_1)
    // allow NUMERIC || CHAR for all
    Int32 srcChildIndex = -1;
    Int32 convType = -1;
    if ((type1.getTypeQualifier() == NA_CHARACTER_TYPE) &&
        (type2.getTypeQualifier() == NA_DATETIME_TYPE))
      {
        // convert child(1)(DATETIME) to char type
        srcChildIndex = 1;
        convType = 1;
      }
    else if ((type1.getTypeQualifier() == NA_DATETIME_TYPE) &&
             (type2.getTypeQualifier() == NA_CHARACTER_TYPE))
      {
        // convert child(1)(DATETIME) to char type
        srcChildIndex = 0;
        convType = 1;
      }
    else if ((type1.getTypeQualifier() == NA_NUMERIC_TYPE) &&
             (type2.getTypeQualifier() == NA_CHARACTER_TYPE))
      {
        // convert child(0)(NUMERIC) to char type
        srcChildIndex = 0;
        convType = 2;
      }
    else if ((type1.getTypeQualifier() == NA_CHARACTER_TYPE) &&
             (type2.getTypeQualifier() == NA_NUMERIC_TYPE))
      {
        // convert child(1)(NUMERIC) to char type
        srcChildIndex = 1;
        convType = 2;
      }
    
    if ((srcChildIndex >= 0) &&
        ((convType == 2) ||
         ((convType == 1) &&
          (CmpCommon::getDefault(MODE_SPECIAL_1) == DF_ON))))         
      {
        Lng32 dLen = 0;
        if (convType == 1)
          {
            DatetimeType &dtType = (DatetimeType&)
              child(srcChildIndex)->castToItemExpr()->getValueId().getType();
            dLen = dtType.getDisplayLength();
          }
        else if (convType == 2)
          {
            NumericType &nType = (NumericType&)
              child(srcChildIndex)->castToItemExpr()->getValueId().getType();
            dLen =
              nType.getDisplayLength(nType.getFSDatatype(),
                                     nType.getNominalSize(),
                                     nType.getPrecision(),
                                     nType.getScale(),
                                     0);
          }
        
        ItemExpr * newChild = NULL;
        if (convType == 1)
          {
            newChild =
              new (bindWA->wHeap())
              Cast(child(srcChildIndex),
                   new (bindWA->wHeap())
                   SQLChar(bindWA->wHeap(), dLen,
                           child(srcChildIndex)->castToItemExpr()->
                           getValueId().getType().supportsSQLnull()));
            newChild = newChild->bindNode(bindWA);
            if (bindWA->errStatus())
              return this;
          }
        else if (convType == 2)
          {
            Parser parser(bindWA->currentCmpContext());
            char buf[128];
            
            sprintf(buf, "CAST(CAST(@A1 AS VARCHAR(%d)) AS VARCHAR(%d))",
                    dLen, dLen);
            newChild = 
              parser.getItemExprTree(buf, strlen(buf), BINDITEMEXPR_STMTCHARSET, 1, child(srcChildIndex));
            
            newChild = newChild->bindNode(bindWA);
            if (bindWA->errStatus()) 
              return this;
          }
        
        setChild(srcChildIndex, newChild);
      }
  }

  // Concat inherits from BuiltinFunction .. Function .. ItemExpr.
  BuiltinFunction::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return this;

  return getValueId().getItemExpr();
}

ItemExpr *UnLogic::bindNode(BindWA *bindWA)
{
  bindWA->getCurrentScope()->context()->inPredicate() = TRUE;

  // Check for a DEFAULT child -- 'DEFAULT IS NULL' is illegal
  ItemExpr *itm = this;
  if (!checkForSQLnullChild(bindWA, this, TRUE/*Tdm extension allows NULL arg*/))
    itm = ItemExpr::bindNode(bindWA);

  bindWA->getCurrentScope()->context()->inPredicate() = FALSE;
  return itm;
}

ItemExpr * ExtractOdbc::bindNode(BindWA * bindWA)
{
  if (nodeIsBound()) return this;

  bindSelf(bindWA);
  if (bindWA->errStatus()) return this;

  // if my child is a char/varchar, insert a Cast node
  // to convert from char to timestamp.
  if (child(0)->castToItemExpr()->getValueId().getType().getTypeQualifier()
      == NA_CHARACTER_TYPE)
    {
      ItemExpr * newChild =
	new (bindWA->wHeap()) Cast(child(0), new (bindWA->wHeap())
				   SQLTimestamp(bindWA->wHeap(), TRUE));
      setChild(0, newChild);
    }
  unBind();

  allowsSQLnullArg() = TRUE;  // do not raise Nullability type errors during call to bindNode
                              // This is being done so that the same error is raised for YEAR(NULL)
                              // type syntax.
  ItemExpr * ie = BuiltinFunction::bindNode(bindWA);
  allowsSQLnullArg() = FALSE; // reset the nullability flag to not allowing NULLs.
  return ie ;
}

ItemExpr * DateFormat::quickDateFormatOpt(BindWA * bindWA)
{
  const NAType *naType0 = &child(0)->getValueId().getType();

  if ((CmpCommon::getDefault(MODE_SPECIAL_4) == DF_ON) &&
      (child(0)->getOperatorType() == ITM_CONSTANT) &&
      (formatStr_ == "DD-MON-YYYY") &&
      (NOT naType0->isVaryingLen()) &&
      (naType0->getFSDatatype() == REC_BYTE_F_ASCII) &&
      (formatStr_.length() == naType0->getNominalSize()))
    {
      NABoolean canXfrm = TRUE;
      ConstValue * cNode = (ConstValue*)child(0)->castToItemExpr();
      NAString cv = *cNode->getRawText();
      const char * str = cv.data();
      if (NOT ((str[2] == '-') && (str[6] == '-')))
        canXfrm = FALSE;

      NAString dv;
      if (canXfrm)
        {
          dv.append(&str[7], 4);
          dv += "-";
          NAString mon(&str[3], 3);
          mon.toUpper();
          if (mon == "JAN")
            dv += "01";
          else if (mon == "FEB")
            dv += "02";
          else if (mon == "MAR")
            dv += "03";
          else if (mon == "APR")
            dv += "04";
          else if (mon == "MAY")
            dv += "05";
          else if (mon == "JUN")
            dv += "06";
          else if (mon == "JUL")
            dv += "07";
          else if (mon == "AUG")
            dv += "08";
          else if (mon == "SEP")
            dv += "09";
          else if (mon == "OCT")
            dv += "10";
          else if (mon == "NOV")
            dv += "11";
          else if (mon == "DEC")
            dv += "12";
          else
            canXfrm = FALSE;
        }

      ItemExpr * newNode = NULL;
      if (canXfrm)
        {
          dv += "-";
          dv.append(&str[0], 2);

          // nuke the Format node and return a DATE constant
          newNode = literalOfDate(&dv, TRUE);
          if (newNode)
            {
              newNode = newNode->bindNode(bindWA);
              if (bindWA->errStatus())
                {
                  canXfrm = FALSE;
                }
            }
          else
            {
              canXfrm = FALSE;
            }
        }
      
      if ((canXfrm) && (newNode))
        {
          return newNode;
        }
      
      SqlParser_Diags->clear();
      bindWA->resetErrStatus();
    }

  return NULL;
}

ItemExpr * Format::bindNode(BindWA * bindWA)
{
  bindChildren(bindWA);
  if (bindWA->errStatus()) 
    return this;

  const NAType *naType0 = &child(0)->getValueId().getType();
  const NumericType * nType0 = NULL;

  NABoolean formatX = FALSE;
  NABoolean format9 = FALSE;
  NABoolean formatExtract = FALSE;
  Lng32 dotPos = 0;
  NABoolean formatNumericAsX = FALSE;
  NABoolean formatStringAsX  = FALSE;

  if ((formatStr_ == "HH24") ||
      (formatStr_ == "D") ||
      (formatStr_ == "MM") ||
      (formatStr_ == "YYYY"))
    {
      if (CmpCommon::getDefault(MODE_SPECIAL_4) == DF_OFF)
        {
          *CmpCommon::diags() << DgSqlCode(-4065) << DgString0(formatStr_)
                              << DgString1(formatType_ == FORMAT_GENERIC ? "FORMAT" 
                                           : (formatType_ == FORMAT_TO_CHAR ? "TO_CHAR" : "TO_DATE"));
          bindWA->setErrStatus();
          return this;
        }

      if ((formatType_ != FORMAT_TO_CHAR)  &&
	  (naType0->getTypeQualifier() != NA_DATETIME_TYPE))
        {
          *CmpCommon::diags() << DgSqlCode(-4071) << DgString0("TO_CHAR");
          bindWA->setErrStatus();
          return this;
        }

      if ((naType0->getPrecision() != SQLDTCODE_TIMESTAMP) &&
          (naType0->getPrecision() != SQLDTCODE_TIME))
        {
          *CmpCommon::diags() << DgSqlCode(-4072) << DgString0("TO_CHAR") << DgString1("time");;
          bindWA->setErrStatus();
          return this;
	}
      
      formatExtract = TRUE;
    }
  else
    {
      formatX = TRUE;
      for (CollIndex i = 0; i < formatStr_.length(); i++)
	{
	  if ((formatStr_.data()[i] != 'X') &&
	      (formatStr_.data()[i] != 'x'))
	    formatX = FALSE;
	}
      if (formatX)
	{
	  if ((naType0->getTypeQualifier() == NA_CHARACTER_TYPE) &&
	      ((Lng32)(formatStr_.length()) != naType0->getNominalSize()))
	    formatStringAsX = TRUE;
	  else if ((naType0->getTypeQualifier() == NA_NUMERIC_TYPE) &&
		   ((nType0 = ((NumericType*)naType0))->isExact()) &&
		   (NOT nType0->isBigNum()) &&
		   (nType0->getScale() == 0))
	    formatNumericAsX = TRUE;
	  else
	    formatX = FALSE;
	}

      if (NOT formatX)
        {
          format9 = TRUE;
          // check if format is of the form:   99999[.99]
          Lng32 numDots = 0;
          dotPos = -1;
          for (CollIndex i = 0; i < formatStr_.length(); i++)
            {
              if ((formatStr_.data()[i] != '9') &&
                  (formatStr_.data()[i] != '.'))
                format9 = FALSE;
              else if (formatStr_.data()[i] == '.')
                {
                  numDots++;
                  dotPos = i;
                }
            }
          if ((format9) && (numDots > 1))
            format9 = FALSE;
        }
    }

  ItemExpr * newIE= NULL;
  if ((formatX) || (format9) || (formatExtract))
    {
      Parser parser(bindWA->currentCmpContext());
      char buf[200];
      buf[0] = 0;

      if (formatNumericAsX)
	{
	  Lng32 dLen =
	    nType0->getDisplayLength(nType0->getFSDatatype(),
				     nType0->getNominalSize(),
				     nType0->getPrecision(),
				     nType0->getScale(),
				     0);

	  if ((Lng32)(formatStr_.length()) < dLen)
	    {
	      sprintf(buf, "SUBSTRING(CAST(@A1 as CHAR(%d)), %d, " PFSZ ")", 
		      dLen, 1, formatStr_.length());
	    }
	  else if ((Lng32)(formatStr_.length()) >= dLen)
	    {
	      // format using Cast
	      sprintf(buf, "CAST(@A1 as CHAR(" PFSZ "))", formatStr_.length());
	    }
	  
	}
      else if (formatStringAsX)
	{
	  if ((Lng32)(formatStr_.length()) < naType0->getNominalSize())
	    {
	      sprintf(buf, "SUBSTRING(@A1, %d, " PFSZ ")", 1, formatStr_.length());
	    }
	  else if ((Lng32)(formatStr_.length()) > naType0->getNominalSize())
	    {
	      // format using Cast
	      sprintf(buf, "CAST(@A1 as CHAR(" PFSZ "))", formatStr_.length());
	    }
	}
      else if (format9)
        {
          if (dotPos == -1)
            dotPos = 0;
          else
            dotPos = formatStr_.length() - dotPos -1;
          sprintf(buf, "CAST(@A1 as NUMERIC(%d,%d))", (Lng32)formatStr_.length(), dotPos);
        }
      else if (formatExtract)
        {
          if (formatStr_ == "YYYY")
            sprintf(buf, "CAST(EXTRACT (YEAR FROM @A1) AS CHAR(4))");
          else if (formatStr_ == "MM")
            sprintf(buf, "CAST(EXTRACT (MONTH FROM @A1) AS CHAR(2))");
          else if (formatStr_ == "HH24")
            sprintf(buf, "CAST(EXTRACT (HOUR FROM @A1) AS CHAR(2))");
          else if (formatStr_ == "D")
            sprintf(buf, "CAST(DAYOFWEEK(@A1) AS CHAR(2));");
          else
            {
              bindWA->setErrStatus();
              return NULL;
            }
        }
       else
	{
	  bindWA->setErrStatus();
	  return NULL;
	}

      if (strlen(buf) > 0)
	{
	  newIE = parser.getItemExprTree(buf, strlen(buf), BINDITEMEXPR_STMTCHARSET, 1, child(0));
	  if (! newIE)
	    {
	      bindWA->setErrStatus();
	      return NULL;
	    }
	  
	  newIE = newIE->bindNode(bindWA);
	  if (bindWA->errStatus())
	    return NULL;
	}
    }
  else
    {
      if (CmpCommon::getDefault(MODE_SPECIAL_1) == DF_OFF)
        {
          *CmpCommon::diags() << DgSqlCode(-4065) << DgString0(formatStr_)
                              << DgString1(formatType_ == FORMAT_GENERIC ? "FORMAT" 
                                           : (formatType_ == FORMAT_TO_CHAR ? "TO_CHAR" : "TO_DATE"));
          bindWA->setErrStatus();
          return this;
        }
      
      // In mode_special_1, ignore this format and return the child pointer.
      newIE = child(0);
    }
  
  return newIE;
}

///////////////////////////////////////////////////////
// various error checks, details below.
//////////////////////////////////////////////////////
NABoolean DateFormat::errorChecks(Lng32 frmt, BindWA *bindWA, 
                                  const NAType* opType)
{
  Lng32 error = 0;

  NABoolean toChar  = (formatType_ == FORMAT_TO_CHAR);
  NABoolean toDate  = (formatType_ == FORMAT_TO_DATE);
  NABoolean toTime  = (formatType_ == FORMAT_TO_TIME);
  NABoolean df  = ExpDatetime::isDateFormat(frmt);
  NABoolean tf  = ExpDatetime::isTimeFormat(frmt);
  NABoolean tsf = ExpDatetime::isTimestampFormat(frmt);
  NABoolean nf  = ExpDatetime::isNumericFormat(frmt);
  NABoolean ef  = ExpDatetime::isExtraFormat(frmt);
  NABoolean ms4 = (CmpCommon::getDefault(MODE_SPECIAL_4) == DF_ON);
  
  if (NOT (df || tf || tsf || nf || ef))
    {
      // format must be date, time, timestamp or numeric
      error = 1; // error 4065
    }
  else if ((NOT ms4) && nf)
    {
      // format can only be numeric in mode_special_4
      error = 1; // error 4065
    }

  if (toDate && NOT (df || tsf))
    {
      // TO_DATE requires date format or timestamp format
      // unless we are in mode_special_4 in which case
      // numeric format is accepted
      if (NOT (ms4 && nf))
        error = 1; // error 4065
    }

  if (!error && toTime)
    {
      if (NOT tf)
        {
          // TO_TIME requires time format
          error = 1; // error 4065
        }
      // source must be datetime containing time field or a character string
      else if (((opType->getTypeQualifier() == NA_DATETIME_TYPE) &&
                (opType->getPrecision() == SQLDTCODE_DATE)) ||
               (opType->getTypeQualifier() != NA_CHARACTER_TYPE))
        {
          error = 9; // error 3415
        }
    }

  if (!error && toChar)
    {
      // source must be datetime with to_char function
      if (opType->getTypeQualifier() != NA_DATETIME_TYPE)
        error = 2; // error 4071

      // cannot convert date source to time format
      else if (tf && (opType->getPrecision() == SQLDTCODE_DATE))
        error = 3; // error 4072
      
      // cannot convert time source to date format or timestamp format
      // for TO_CHAR only (for DATEFORMAT it is OK)
      else if ((df || tsf) && (!wasDateformat_) && (opType->getPrecision() == SQLDTCODE_TIME))
        error = 8; // error 4072
    }

  if (!error && toDate)
    {
      // source must be char or numeric with to_date
      if ((opType->getTypeQualifier() != NA_CHARACTER_TYPE) &&
          (opType->getTypeQualifier() != NA_NUMERIC_TYPE))
        error = 4; //error 4043

      // source can only be numeric in mode_special_4
      else if ((NOT ms4) && (opType->getTypeQualifier() != NA_CHARACTER_TYPE))
        error = 4; // error 4043

      // operand must be numeric with nf (numeric format)
      else if (ms4 && nf && (opType->getTypeQualifier() != NA_NUMERIC_TYPE))
        {
          error = 7; // error 4045
        }

      // numeric must be exact with scale of 0
      else if (ms4 && (opType->getTypeQualifier() == NA_NUMERIC_TYPE))
        {
          if (NOT ((NumericType*)opType)->isExact())
            error = 5; // error 4046
          else if (NOT ((NumericType*)opType)->getScale() == 0)
            error = 6; // error 4047
        }
    }

  if (error)
    {
      switch (error)
        {
        case 1: 
          {
            *CmpCommon::diags() << DgSqlCode(-4065) << DgString0(formatStr_)
                                << DgString1((toChar ? "TO_CHAR" : 
                                              (toDate ? "TO_DATE" : "TO_TIME")));
            bindWA->setErrStatus();
          }
          break;

        case 2:
          {
            *CmpCommon::diags() << DgSqlCode(-4071) << DgString0(wasDateformat_ ? "DATEFORMAT" : "TO_CHAR");
            bindWA->setErrStatus();
          }
          break;

        case 3:
          {
            *CmpCommon::diags() << DgSqlCode(-4072) << DgString0("TO_CHAR") << DgString1("time");
            bindWA->setErrStatus();
          }
          break;

        case 4:
          {
            *CmpCommon::diags() << DgSqlCode(-4043) << DgString0("TO_DATE");
            bindWA->setErrStatus();
          }
          break;
          
        case 5:
          {
            *CmpCommon::diags() << DgSqlCode(-4046) << DgString0("TO_DATE");
            bindWA->setErrStatus();
          }
          break;
          
        case 6:
          {
            *CmpCommon::diags() << DgSqlCode(-4047) << DgString0("TO_DATE");
            bindWA->setErrStatus();
          }
          break;

        case 7:
          {
            *CmpCommon::diags() << DgSqlCode(-4045) << DgString0("TO_DATE");
            bindWA->setErrStatus();
          }
          break;

        case 8:
          {
            *CmpCommon::diags() << DgSqlCode(-4072) << DgString0("TO_CHAR") << DgString1("date");
            bindWA->setErrStatus();
          }
          break;

        case 9:
          {
            *CmpCommon::diags() << DgSqlCode(-3415) << DgString0("TO_TIME")
                                << DgString1(" It must be a datetime datatype containing the time field or a character datatype.");
            bindWA->setErrStatus();
          }
          break;

        } // switch

      return TRUE;
    }
  
  return FALSE;
}

// used for TO_DATE, TO_CHAR, TO_TIME, DATEFORMAT functions.
DateFormat::DateFormat(ItemExpr *val1Ptr, const NAString &formatStr,
                       Lng32 formatType, NABoolean wasDateformat)
     : CacheableBuiltinFunction(ITM_DATEFORMAT,
                                1, val1Ptr),
       formatStr_(formatStr),
       wasDateformat_(wasDateformat),
       formatType_(formatType),
       frmt_(-1),
       origString_(""),
       dateFormat_(DATE_FORMAT_NONE)
{ 
  allowsSQLnullArg() = FALSE; 

  if (formatStr_ == "SYYYYMM")
    formatStr_ = "YYYYMM";
  else if (formatStr_ == "HH:MI:SS")
    formatStr_ = "HH24:MI:SS";

  frmt_ = ExpDatetime::getDatetimeFormat(formatStr_.data());
}

ItemExpr * DateFormat::bindNode(BindWA * bindWA)
{
  if (checkForSQLnullChild(bindWA, this, allowsSQLnullArg(), FUNCTION_))
    return this;

  bindChildren(bindWA);
  if (bindWA->errStatus()) 
    return this;

  const NAType *naType0 = &child(0)->getValueId().getType();
  const DatetimeType* operand = (DatetimeType *)naType0;
  const NumericType * nType0 = NULL;

  // if the date time format was not specified in TO_CHAR, supply a
  // default now based on the datatype of the first operand
  if (frmt_ == ExpDatetime::DATETIME_FORMAT_UNSPECIFIED)
    {
      if ((naType0->getTypeQualifier() == NA_DATETIME_TYPE) &&
          (operand->getPrecision() == SQLDTCODE_TIME))
        frmt_ = ExpDatetime::DATETIME_FORMAT_TS4;
      else
        frmt_ = ExpDatetime::DATETIME_FORMAT_DEFAULT;
      formatStr_ = ExpDatetime::getDatetimeFormatStr(frmt_);
    }

  // a quick optimization for the date format.
  ItemExpr *newNode = quickDateFormatOpt(bindWA);
  if (newNode)
    return newNode;

  if (errorChecks(frmt_, bindWA, naType0))
    {
      return this;
    }

  dateFormat_ = DateFormat::DATE_FORMAT_NONE;

  if (ExpDatetime::isDateTimeFormat(frmt_))
    {
      // if DATEFORMAT function was specified, then time portion of the
      // format depends on the operand. Date portion remains the same as
      // what was specified (DEFAULT, USA, EUROPEAN).
      if ((wasDateformat_) &&
          (naType0->getTypeQualifier() == NA_DATETIME_TYPE))
        {
          if (operand->getPrecision() == SQLDTCODE_TIMESTAMP)
            {
              if (frmt_ == ExpDatetime::DATETIME_FORMAT_DEFAULT)
                frmt_ = ExpDatetime::DATETIME_FORMAT_TS3;// YYYY-MM-DD HH24:MI:SS
              else if (frmt_ == ExpDatetime::DATETIME_FORMAT_USA)
                frmt_ = ExpDatetime::DATETIME_FORMAT_TS7;// MM/DD/YYYY HH24:MI:SS AM|PM
              else if (frmt_ == ExpDatetime::DATETIME_FORMAT_EUROPEAN)
                frmt_ = ExpDatetime::DATETIME_FORMAT_TS10;// DD.MM.YYYY HH24:MI:SS
            }
        }

       if (ExpDatetime::isTimestampFormat(frmt_))
        dateFormat_ = DateFormat::TIMESTAMP_FORMAT_STR;
      else if (ExpDatetime::isTimeFormat(frmt_))
        dateFormat_ = DateFormat::TIME_FORMAT_STR;
      else
        dateFormat_ = DateFormat::DATE_FORMAT_STR;

     if (naType0->getTypeQualifier() == NA_NUMERIC_TYPE)
        {
          // convert number to char before formatting.
          // Length of target char is equal to formatStr_.
          ItemExpr * newChild =
            new (bindWA->wHeap())
            Cast(child(0),
                 new (bindWA->wHeap())
                 SQLChar(bindWA->wHeap(), formatStr_.length(),
                         child(0)->castToItemExpr()->
                         getValueId().getType().supportsSQLnull()));
          setChild(0, newChild->bindNode(bindWA));
          naType0 = &child(0)->getValueId().getType();
        }
    }
  else if (ExpDatetime::isNumericFormat(frmt_))
    {
      dateFormat_ = DateFormat::TIME_FORMAT_STR;

      if (naType0->getTypeName() != LiteralLargeInt)
        {
          // convert to largeint. We have already verified that
          // child is exact with scale of 0.
          ItemExpr * newChild =
            new (bindWA->wHeap())
            Cast(child(0),
                 new (bindWA->wHeap())
                 SQLLargeInt(bindWA->wHeap(), TRUE,
                             child(0)->castToItemExpr()->
                             getValueId().getType().supportsSQLnull()));
          setChild(0, newChild->bindNode(bindWA));
        }
    }
  else if (ExpDatetime::isExtraFormat(frmt_))
    {
      dateFormat_ = DateFormat::TIME_FORMAT_STR;
    }
  else
    {
      CMPASSERT(FALSE); // should not reach here
    }

  // if source is a timestamp and target is date or time, extract
  // date or time part from source before formatting.
  ItemExpr * newChild = NULL;
  if (naType0->getPrecision() == SQLDTCODE_TIMESTAMP)
    {
      if (ExpDatetime::isTimeFormat(frmt_))
        {
          newChild =
            new (bindWA->wHeap())
            Cast(child(0),
                 new (bindWA->wHeap())
                 SQLTime(bindWA->wHeap(), child(0)->castToItemExpr()->
                         getValueId().getType().supportsSQLnull(),
                         0));
        }
      else if (ExpDatetime::isDateFormat(frmt_))
        {
          newChild =
            new (bindWA->wHeap())
            Cast(child(0),
                 new (bindWA->wHeap())
                 SQLDate(bindWA->wHeap(), child(0)->castToItemExpr()->
                         getValueId().getType().supportsSQLnull()));
        }
      
      if (newChild)
        setChild(0, newChild->bindNode(bindWA));
    }

  BuiltinFunction::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return this;

  return getValueId().getItemExpr();  
}

ItemExpr *Trim::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();

  // this will bind/type-propagate all children.
  bindChildren(bindWA);
  if (bindWA->errStatus()) 
    return this;

  // child0 is trim operand.
  // child1 is expression to be trimmed.
  // if trim operand is null, set it based on trim expression.
  // if trim expr is binary string, trim operand becomes '\0'.
  // Otherwise it becomes ' '
  if (child(0) == NULL)
    {
      const NAType &type1 = 
        child(1)->castToItemExpr()->getValueId().getType();
       
      ItemExpr * trimOper = NULL;
      if (DFS2REC::isBinaryString(type1.getFSDatatype()))
        trimOper = new (PARSERHEAP()) SystemLiteral(NAString('\0'), CharInfo::ISO88591);
      else
        trimOper = new (PARSERHEAP()) SystemLiteral(" ", WIDE_(" "));

      trimOper = trimOper->bindNode(bindWA);
      if (bindWA->errStatus())
        return this;
      setChild(0, trimOper);
    }

  if (CmpCommon::getDefault(ALLOW_INCOMPATIBLE_OPERATIONS) == DF_ON)
    {
      // if argument is numeric, convert it to string.
      const NAType &type1 = 
        child(1)->castToItemExpr()->getValueId().getType();
      if (type1.getTypeQualifier() == NA_NUMERIC_TYPE)
        {
          const NumericType &numeric  = (NumericType&)type1;
          Lng32 dLen =
            numeric.getDisplayLength(numeric.getFSDatatype(),
                                     numeric.getNominalSize(),
                                     numeric.getPrecision(),
                                     numeric.getScale(),
                                     0);
          
          ItemExpr * newChild =
            new (bindWA->wHeap())
            Cast(child(1),
                 new (bindWA->wHeap())
                 SQLChar(bindWA->wHeap(), dLen, type1.supportsSQLnull()));
          
          newChild = newChild->bindNode(bindWA);
          if (bindWA->errStatus())
            return this;
          setChild(1, newChild);
        }
    }

  // Trim inherits from BuiltinFunction .. Function .. ItemExpr.
  BuiltinFunction::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return this;

  return getValueId().getItemExpr();
}

// -----------------------------------------------------------------------
// ItemList (which is not ItemExprList!) has arity 2 but arbitrary degree
// (number of list elements), which we capture via this currChildNo()
// propagation.
// -----------------------------------------------------------------------

ItemExpr *ItemList::bindNode(BindWA *bindWA)
{
  ItemList *save = bindWA->getCurrentScope()->context()->inItemList();
  bindWA->getCurrentScope()->context()->inItemList() = this;
  if (save) currChildNo() = save->currChildNo();

  ItemExpr::bindNode(bindWA);

  if (save) save->currChildNo() = currChildNo();
  bindWA->getCurrentScope()->context()->inItemList() = save;

  return this;
} // ItemList::bindNode()

// -----------------------------------------------------------------------
// A method for binding host variables, dynamic parameters and constants.
// -----------------------------------------------------------------------

ItemExpr *ItemExpr::bindUserInput(BindWA *bindWA,
				  const NAType *type,
				  const NAString &fabricatedName)
{
  AssignmentStHostVars *varPtr;
  OperatorTypeEnum opTyp = getOperatorType();

  // If we have a SET statement of compound statements, we return the value id
  // stored in the list assignmentStHostVars_ in bindWA. Such list contains the
  // latest value id assigned to this variable. If it is not there, we bind this ItemExpr
  if ( (opTyp == ITM_HOSTVAR) &&
       (bindWA->getAssignmentStArea()) && (bindWA->getAssignmentStArea()->getAssignmentStHostVars()) &&
       (varPtr = bindWA->getAssignmentStArea()->getAssignmentStHostVars()->findVar(fabricatedName))) {
    ValueId vid = varPtr->currentValueId();
    if ((vid != NULL_VALUE_ID) && !(((HostVar *)this)->isHVInputAssignment())) {
      varPtr->currentValueId().getItemExpr()->previousHostVar() = TRUE;
      varPtr->currentValueId().getItemExpr()->previousName() = ((HostVar *)this)->getName();
      //markAsBound();
      return varPtr->currentValueId().getItemExpr();
    }
  }

  if (nodeIsBound())
    return getValueId().getItemExpr();

  CMPASSERT(isAUserSuppliedInput()) ;

  CMPASSERT(bindWA);
  // Currently we disallow parameters or hostvars in DDL.
  if ( bindWA->inDDL() AND ( getOperatorType() == ITM_DYN_PARAM OR
			     getOperatorType() == ITM_HOSTVAR ) ) {
    *CmpCommon::diags() << DgSqlCode(-3186) << DgString0(fabricatedName);
    bindWA->setErrStatus();
    return this;
  }


  const CharType *ct = (type && type->getTypeQualifier() == NA_CHARACTER_TYPE) ?
		       (const CharType*)type : NULL;

  if (opTyp == ITM_DYN_PARAM AND getText() == "?") {
    DynamicParam* orig = ((DynamicParam*)this)->getOriginal();
    if (orig == NULL) {
      // we're an original dynamic parameter.
      setValueId(createValueDesc(bindWA, this, type));
    }
    else {
      // dynamic parameters must preserve their valueids.
      // otherwise, the result can be uninitialized variables at runtime.
      // dynamic parameters are initialized in the root fragment.
      if (orig->getValueId() == NULL_VALUE_ID)
        orig->setValueId(createValueDesc(bindWA, this, type));
      setValueId(orig->getValueId());
    }
  }
  else if (opTyp == ITM_CONSTANT  AND getText() == "NULL") {
    setValueId(createValueDesc(bindWA, this, type));
  }
  else if (opTyp == ITM_CACHE_PARAM) {
    setValueId(createValueDesc(bindWA, this, type));
    bindWA->addInputValue(fabricatedName, getValueId());
  }
  else if (ct && ct->getCharSet() == CharInfo::UnknownCharSet)
    // Do not reuse valueId on an UnknownCharSet const value.
    setValueId(createValueDesc(bindWA, this, type));
  else {
    NAString fabName(fabricatedName, bindWA->wHeap());
    if (ct) {
      // Collations might be coerced later, in type synth,
      // going up the parse tree.  We need to add this discriminant info!
      char coco[30];
      sprintf(coco, " %d,%d", ct->getCollation(), ct->getCoercibility());
      fabName += coco;
    }
    ColumnNameMap *xcnmEntry = bindWA->findInputValue(fabName);
    if (!xcnmEntry) {
      setValueId(createValueDesc(bindWA, this, type));
      bindWA->addInputValue(fabName, getValueId());
    }
    else {
      if (type &&
          type->getTypeQualifier() == NA_CHARACTER_TYPE)
      {
        if (*type == xcnmEntry->getValueId().getType())
          {
           setValueId(xcnmEntry->getValueId());
          }
          else
          {
            // CR 10-010314-1733
            // For a given constant, the fabricated name needs to be distinct from
            // other names as this name is used as the hash value.
            // The name is initialized using the initial
            // value set by constructor based on the length of the string.
            // Occasionally, if the length and the initial value are same
            // then two different constants may be initialized to the same name: this
            // may cause problems such as type mismatch.

            // To prevent this, we create two random digits and append to
            // the name. Since these names are on statement heap, the probability of
            // two different constants having the same name is almost zero.

            setValueId(createValueDesc(bindWA, this, type));
            char fab[3];
            memset(fab, 0, 3);
            sprintf(fab, "%d", rand() % 100);
            fab[2] = '\0';
            fabName += fab;
            bindWA->addInputValue(fabName,getValueId());
          }
          // else continue binding this item expression
      }
      else
      {
        setValueId(xcnmEntry->getValueId());
        CMPASSERT(!type ||
		  type->getTypeQualifier() == NA_UNKNOWN_TYPE ||
		  *type == getValueId().getType());
      }
    }
  }
  //
  // All user inputs **except constants** are treated as
  // outer references in the current scope.
  //
  if (opTyp != ITM_CONSTANT)
    bindWA->getCurrentScope()->addOuterRef(getValueId());

  bindSelf(bindWA);
  if (bindWA->errStatus()) return this;

  return getValueId().getItemExpr();
} // ItemExpr::bindUserInput()

// -----------------------------------------------------------------------
// Convert a backbone of certain nodes (usually AND nodes) into a value id
// list and get rid of the backbone (NOTE: this does not strictly
// require a left-linear or right-linear backbone, any shape tree with
// 0 or more nodes of type 'backboneType' on its top will work)
// -----------------------------------------------------------------------
Int32 ItemExpr::convertToValueIdList(ValueIdList &vl,
				   BindWA *bindWA,
				   OperatorTypeEnum backboneType,
                                   RelExpr *parent)
{
  // use local stack to implement recursive calls
  stack<ItemExpr*> stk;
  ItemExpr *current_ie;

  stk.push (this);

  while (!stk.empty())
  {
    current_ie = stk.top();
    stk.pop();

    if (current_ie->getOperatorType() == backboneType)
    {
       if (current_ie->child(1))
          stk.push(current_ie->child(1));
       if (current_ie->child(0))
          stk.push(current_ie->child(0));
    }
  else
    {
      // ok, we've reached a non-backbone node, bind the node and insert its
      // value id into the list.
      ItemExpr *boundExpr = current_ie;
      if (!bindWA)
	current_ie->synthTypeAndValueId();
      else
	{
          StmtDDLCreateView *pcvn = NULL;
          if (bindWA->inViewDefinition())
	    {
	      pcvn = bindWA->getCreateViewParseNode();
	      if (pcvn->isProcessingViewColList() &&
		  parent == pcvn->getQueryExpression())
		{
		  if (current_ie->containsSubquery())
		    pcvn->getViewUsages().setViewIsSurelyNotUpdatableFlag();
		  if (current_ie->getOperatorType() == ITM_REFERENCE)
		    {
		      pcvn->setItmColRefInColInRowValsFlag(TRUE);
		      ((ColReference*) current_ie)->setParent(parent);
		    }
		  else
		    pcvn->setItmColRefInColInRowValsFlag(FALSE);
	      }
	    }

	  BindScope *currScope = bindWA->getCurrentScope();
	  if (current_ie->inGroupByOrdinal())
	    {
	      currScope->context()->inGroupByOrdinal() = TRUE;
	    }
          if (current_ie->isGroupByExpr())
	    {
	      currScope->context()->inGroupByExpr() = TRUE;
	    }
	  boundExpr = (ItemExpr *) (current_ie->bindNodeRoot(bindWA));
	  if (current_ie->inGroupByOrdinal())
	    {
	      currScope->context()->inGroupByOrdinal() = FALSE;
	    }
          if (current_ie->isGroupByExpr())
	    {
	      currScope->context()->inGroupByExpr() = FALSE;
	    }

	  if (!boundExpr || bindWA->errStatus())
            return TRUE;		// error

          if (pcvn &&
              pcvn->isProcessingViewColList() &&
              parent == pcvn->getQueryExpression() &&
              NOT pcvn->isItmColRefInColInRowVals())
	    { // see notes in BindUtil_CollectColumnUsageInfo()
	      pcvn->incrCurViewColNum();
	    }
	  else
	  if (parent &&
	      bindWA->getCurrentScope()->context()->counterForRowValues())
	    {
	      bindWA->getCurrentScope()->context()->counterForRowValuesIncr();
	    }
	}

      if (current_ie->getOperatorType() == ITM_REFERENCE &&
          ((ColReference*) current_ie)->getColRefNameObj().isStar() && 
          ((ColReference*) current_ie)->getStarExpansion())
	{
	  ((ColReference*) current_ie)->getStarExpansion()->getValueIdList(vl);
	}
      else
	{
	  vl.insert(boundExpr->getValueId());
	}


      if (bindWA && (boundExpr->getValueId() != NULL_VALUE_ID) &&
	  (boundExpr->getValueId().getType().isLob()))
      {
	BindScope *currScope = bindWA->getCurrentScope();

	BindContext *currContext = currScope->context();
	if (currContext->inOrderBy() ||
	    currContext->inExistsPredicate() ||
	    currContext->inGroupByClause() ||
	    currContext->inHavingClause() ||
	    currContext->inUnion() ||
	    currContext->inJoin()
	  )
	{
	  *CmpCommon::diags() << DgSqlCode(-4322);
	  bindWA->setErrStatus();
	  return TRUE;  // error
	}
      }

    // If the valueId of the column corresponds to the USER(x) function
    // and appears at any place other than the outer select list, then give
    // an error. If the user(x) function appears directly in the query
    // then these errors will be caught while binding the user function itself.
    // This check is for case where column derived from user(x) is being
    // used in the query. This will have to be caught after it has got its valuId
    // and that valueId corresponds to the user(x) function.

    if (bindWA && (boundExpr->getOperatorType() == ITM_USER))
    {
      BindScope *prevScope = NULL;
      BindScope *currScope = bindWA->getCurrentScope();

      while (currScope)
      {
	BindContext *currContext = currScope->context();
	if (currContext->inSubquery() ||
	    currContext->inOrderBy() ||
	    currContext->inExistsPredicate() ||
	    currContext->inGroupByClause() ||
	    currContext->inWhereClause() ||
	    currContext->inHavingClause() ||
	    currContext->inUnion() ||
	    currContext->inJoin()
	  )
	{
 	    *CmpCommon::diags() << DgSqlCode(-4310)
				<< DgString0("USER(x)");
	    bindWA->setErrStatus();
	    return TRUE;  // error
	}
	prevScope = currScope;
	currScope = bindWA->getPreviousScope(prevScope);
      }
    }
    else if ( bindWA &&
              (bindWA->getCurrentScope()->context()->inOrderBy() ||
               bindWA->getCurrentScope()->context()->inGroupByClause()) )
    {
      // Temporary fix till random is supported in ORDER BY, GROUP BY
      // For now do not allow random in order by clause, Group By
      // and distinct.  
      if (boundExpr->containsOpType(ITM_RANDOMNUM)) {
        *CmpCommon::diags() << DgSqlCode(-4313);
        bindWA->setErrStatus();
        return TRUE; // error
      }
    }
  }
 }

 if (stk.empty())
   return FALSE;

  return TRUE;
} // ItemExpr::convertToValueIdList()

//
// MACROS used by SEMI-NON-RECURSIVE version of ItemExpr::convertToValueIdSet(...) below
//
#define AVR_STATE0 0
#define AVR_STATE1 1
#define AVR_STATE2 2

// -----------------------------------------------------------------------
// Convert a backbone of certain nodes (usually AND nodes) into a value id
// set and get rid of the backbone (NOTE: this does not strictly
// require a left-linear or right-linear backbone, any shape tree with
// 0 or more nodes of type 'backboneType' on its top will work)
// -----------------------------------------------------------------------
Int32 ItemExpr::convertToValueIdSet(ValueIdSet &vs,
				  BindWA *bindWA,
				  OperatorTypeEnum backboneType,
				  NABoolean tfmSubq,
				  NABoolean flattenLists)
{
  //
  // convertToValueIdSet() used to be called recursively not just
  // for all the items in an expression but for all the items in the node
  // tree for an entire query. Consequently, we must eliminate the recursive
  // calls to convertToValueIdSet() by keeping the information needed by
  // each "recursive" level in the HEAP and using a "while" loop to look
  // at each node in the tree in the same order as the old recursive technique
  // would have done.
  // The information needed by each "recursive" level is basically just
  // * a pointer to what node (ItemExpr *) to look at next, and
  // * a "state" value that tells us where we are in the convertToValueIdSet()
  //   code for the ItemExpr node that we are currently working on
  //
  ARRAY( ItemExpr * ) IEarray(HEAP, 10) ; //Initially 10 elements (no particular reason to choose 10)
  ARRAY( Int16 )      state(HEAP, 10)   ; //These ARRAYs will grow automatically as needed.)

  Int32  currIdx    = 0           ;
  IEarray.insertAt( currIdx, this       ) ; //Initialize first array element
  state.insertAt(   currIdx, AVR_STATE0 ) ;

  Int32 status = FALSE ;

  while( currIdx >= 0 )
  {
    ItemExpr * currIE = IEarray[currIdx] ;

    if ( currIE->getOperatorType() == backboneType )
    {
      switch ( state[currIdx] )
      {
        case AVR_STATE0:
          // this is a backbone node, recurse and then delete the backbone node
          CMPASSERT( currIE->getArity() == 2 );

          state.insertAt( currIdx, AVR_STATE1 ) ;
          currIdx++ ;                               //"Recurse" down to child 0
          state.insertAt(   currIdx, AVR_STATE0 ) ; // and start that child's state at 0
          IEarray.insertAt( currIdx, currIE->child(0) ) ;

          // if (status) return status; Commented out since return will be done later
          continue ;

        case AVR_STATE1:
          state.insertAt( currIdx, AVR_STATE2 ) ;
          currIdx++ ;                               //"Recurse" down to child 1
          state.insertAt(   currIdx, AVR_STATE0 ) ; // and start that child's state at 0
          IEarray.insertAt( currIdx, currIE->child(1) ) ;

          // if (status) return status; Commented out since return will be done later
          continue ;

        case AVR_STATE2:
          state.insertAt( currIdx, AVR_STATE0 ) ; // We are done processing 'currIE'
          break;
      }

      // If user had specified selectivity for the compound predicate, then make 
      // sure the child predicates get proportionate share of the selectivity.
      if( currIE->isSelectivitySetUsingHint() )
      {
        double newSelFactor = sqrt( currIE->getSelectivityFactor() );

        currIE->child(0)->setSelectivitySetUsingHint();
        currIE->child(0)->setSelectivityFactor( newSelFactor );

        currIE->child(1)->setSelectivitySetUsingHint();
        currIE->child(1)->setSelectivityFactor( newSelFactor );
      }
    }
    else
    {
      // ok, we've reached a non-backbone node, bind the node and insert its
      // value id into the set.
      ItemExpr *boundExpr = currIE;
      if (!bindWA)
         currIE->synthTypeAndValueId() ;
      else
      {
        boundExpr = (ItemExpr *) currIE->bindNodeRoot(bindWA) ;
        if (bindWA->errStatus()) 
          return TRUE;
        if (! boundExpr)
          {
            bindWA->setErrStatus();
            return TRUE;             // error
          }

        if ( bindWA->getCurrentScope()->context()->inOrderBy() ||
             bindWA->getCurrentScope()->context()->inGroupByClause() )
        {
          // Temporary fix till random is supported in ORDER BY, GROUP BY
          // For now do not allow random in ORDER BY clause, GROUP BY
          // and DISTINCT.
          if (boundExpr->containsOpType(ITM_RANDOMNUM))
          {
            *CmpCommon::diags() << DgSqlCode(-4313);
            bindWA->setErrStatus();
            return TRUE; // error
          }
        }

        if ( bindWA && (boundExpr->getValueId() != NULL_VALUE_ID) &&
             ( boundExpr->getValueId().getType().isLob() ) )
        {
          BindScope *currScope = bindWA->getCurrentScope();

          BindContext *currContext = currScope->context();
          if (currContext->inOrderBy() ||
              currContext->inExistsPredicate() ||
              currContext->inGroupByClause() ||
              currContext->inHavingClause() ||
              currContext->inUnion() ||
              currContext->inJoin()
            )
          {
            *CmpCommon::diags() << DgSqlCode(-4322);
            bindWA->setErrStatus();
            return TRUE; // error
          }
        }
      }

      if ( currIE->getOperatorType() == ITM_REFERENCE &&
          ((ColReference *)currIE)->getColRefNameObj().isStar()  &&
          ((ColReference *)currIE)->getStarExpansion())
      {
        const ColumnDescList& cl = *(((ColReference *)currIE)->getStarExpansion());
        for (CollIndex i = 0; i < cl.entries(); i++)
          vs.insert(cl[i]->getValueId());
      }
      else if ( tfmSubq && backboneType == ITM_AND
                && currIE->containsSubquery() )
      {
        // If *all* my children are MVPs, insert my transform (not me).
        //
        // If a "raw-mode" retry could transform *any* MVPs,
        // insert the raw transform (containing all my subqueries) *AND*
        // insert me too.
        //
        // See dissectOutSubqueries in NormItemExpr.cpp for rationale.
        //
        ItemExpr *tfm = currIE->transformMultiValuePredicate(FALSE/*do NOT flatten*/);
        if (tfm)
          tfm->convertToValueIdSet(vs, bindWA, backboneType, FALSE);
        else
        {
          if (!bindWA) // do not do this at bindtime
            tfm = currIE->transformMultiValuePredicate(FALSE, ANY_CHILD_RAW);
          if (tfm)
            tfm->convertToValueIdSet(vs, bindWA, backboneType, FALSE);
          vs.insert(boundExpr->getValueId());
        }

        if(tfm && ( tfm->getOperatorType() == ITM_AND))
        {
          tfm->synthTypeAndValueId(TRUE);

          // check for the added prefix predicate
          ItemExpr * leftChildOfAND = (ItemExpr *)tfm->child(0);

          if((leftChildOfAND->isARangePredicate()) &&
             ((BiRelat *)leftChildOfAND)->isDerivedFromMCRP())
          {
            BiRelat * addedComparison = (BiRelat *) leftChildOfAND;
            addedComparison->translateListOfComparisonsIntoValueIds();
          }
        }
      }
      else if ( ( flattenLists == TRUE )
                && ( currIE->getOperatorType() == ITM_ITEM_LIST ) )
      {
        status = currIE->convertToValueIdSet(vs, bindWA, ITM_ITEM_LIST, 
                                             tfmSubq, flattenLists);
        if (status) return status;
      }
      else 
      {
        vs.insert(boundExpr->getValueId());
      }
    }
    if ( state[currIdx] == AVR_STATE0 )
       currIdx-- ;       // Return to the parent node & continue working on it

  } // end of while( currIdx >= 0 )

  return status ;
} // ItemExpr::convertToValueIdSet

// -----------------------------------------------------------------------
// member functions for class Aggregate
// -----------------------------------------------------------------------

ItemExpr *Aggregate::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();

  if(isOLAP()) {
    ItemExpr *olap = transformOlapFunction( bindWA );
    if(!olap) {
      return NULL;
    }

    return olap->bindNode(bindWA);
  }

  // Genesis 10-000222-6892:
  // An AVG is really "SUM / COUNT_NONULL";
  // the fix saves the outer Agg Scope (bindWA->outerAggScope())from the first part (SUM)
  // to be applied to the second part (COUNT).
  //
  // VARIANCE and STDDEV are likewise compositions of primitive aggr-funcs.
  // See Variance::bindNode() and subaggBindNode()
  // where the outer Agg Scope (bindWA->outerAggScope())is saved + reapplied in a similar fashion.
  //
  // Lines which have been CLONED or COPIED are marked "AggBind#n".

  if (origOpType() != ITM_AVG)      // AggBind#1 support
    bindWA->outerAggScope() = NULL;

  else if (getOperatorType() == ITM_AVG) {

    ItemExpr *sum = new (bindWA->wHeap())
      Aggregate(ITM_SUM,          child(0), isDistinct(), ITM_AVG, '!');
    ItemExpr *cnt = new (bindWA->wHeap())
      Aggregate(ITM_COUNT_NONULL, child(0), isDistinct(), ITM_AVG, '!');
    ItemExpr *avg = new (bindWA->wHeap())
      BiArith(ITM_DIVIDE, sum, cnt);
    avg->setOrigOpType(ITM_AVG);

    bindWA->outerAggScope() = NULL;			// SUM may change this for CNT

    // 
    // NOTE: Removed code from right here.  That code was calling bindNode
    // on the *sum and *cnt nodes.  The code was not needed since the 
    // following avg->bindNode() operation will call bindChildren() to bind
    // the children.  Furthermore, now that we are supporting OLAP functions,
    // the bindNode() operation on each of the children may return a pointer
    // to a different node (a ITM_NOTCOVERED node) than the original child
    // nodes.  bindChildren() will handle that properly, whereas the code
    // that has been deleted from here was ignoring the possibility that
    // sum->bindNode() and/or cnt->bindNode() might return different ptrs.
    // 

    avg = avg->bindNode(bindWA);
    if (bindWA->errStatus()) return NULL;

    markAsBound();
    setValueId(avg->getValueId());
    return getValueId().getItemExpr();		// AVG node is never seen again

  } // top-level AVG

  BindScope   *currScope = bindWA->getCurrentScope();
  BindContext *context   = currScope->context();

  if (context->inAggregate()) {
    // 4009 An aggregate is not allowed inside an aggregate.
    *CmpCommon::diags() << DgSqlCode(-4009);
    bindWA->setErrStatus();
    return NULL;
  }

  // an aggregate is not allowed to be referenced as a group by expr.
  if (bindWA->getCurrentScope()->context()->inGroupByExpr()) {
    *CmpCommon::diags() << DgSqlCode(-4197)
                        << DgString0("GROUP BY");
    bindWA->setErrStatus();
    return NULL;
  }

  // an aggregate is not allowed to be referenced as a group by ordinal.
  if (bindWA->getCurrentScope()->context()->inGroupByOrdinal()) {
    *CmpCommon::diags() << DgSqlCode(-4185);
    bindWA->setErrStatus();
    return NULL;
  }

  context->inAggregate() = TRUE;

  CMPASSERT(NOT context->colRefInAgg());
  CMPASSERT(NOT context->outerColRefInAgg() ||
	    origOpType() == ITM_STDDEV || origOpType() == ITM_VARIANCE);

  if (checkForSQLnullChild(bindWA, this, FALSE, FUNCTION_)) return this;


  // The Aggregates are bound in the environment (RETDesc) of
  // the child of the OLAP Sequence if one exists
  //
  RelSequence *sequenceNode = (RelSequence *)currScope->getSequenceNode();
  RETDesc *currentRETDesc = NULL;
  NABoolean isOLAPSequence = FALSE;

  // An empty requiredOrder() indicates that this Sequence is for OLAP.
  // (may want to have a better way of indicating this.
  //
  if (sequenceNode && 
      (((const RelSequence *)sequenceNode)->requiredOrder().entries() == 0)) {

    currentRETDesc = currScope->getRETDesc();
   
    currScope->setRETDesc(sequenceNode->child(0)->getRETDesc());
    isOLAPSequence = TRUE;
  }

  if ((CmpCommon::getDefault(MODE_SPECIAL_1) == DF_ON) &&
      (getOperatorType() == ITM_SUM))
    {
      // bind/type-propagate all children.
      bindChildren(bindWA);
      if (bindWA->errStatus()) 
	return this;

      const NAType &type1 = 
	child(0)->castToItemExpr()->getValueId().getType();

      if (type1.getTypeQualifier() == NA_CHARACTER_TYPE)
	{
	  // convert to double precision. This will handle all precision,
	  // scale and type specified in the char value.
	  ItemExpr * newChild =
	    new (bindWA->wHeap())
	    Cast(child(0),
		 new (bindWA->wHeap())
		 SQLDoublePrecision(bindWA->wHeap(), type1.supportsSQLnull()));
	  setChild(0, newChild->bindNode(bindWA));
	}

    }

  ItemExpr::bindNode(bindWA);	// Aggregate is directly derived from ItemExpr
  
  // Restore current RETDesc.
  if(isOLAPSequence) {
    currScope->setRETDesc(currentRETDesc);
  }

  if (bindWA->errStatus()) return NULL;

  // Now that we allow UDFs and subqueries as inputs to aggregates, we need
  // to make sure they have a degree of 1.

  CollIndex childDegree = 0;
  Subquery * subq = NULL;
  UDFunction * udf = NULL;
  Int32 origArity = getArity();

  for (Int32 chld=0; chld < origArity; chld++)
  {
    switch (child(chld)->getOperatorType())
    {
       case ITM_ROW_SUBQUERY:
       case ITM_USER_DEF_FUNCTION:
       {
         subq = (Subquery *) child(chld)->castToItemExpr();
         udf = (UDFunction *) child(chld)->castToItemExpr();
         Lng32 myDegree = 
                       (child(chld)->getOperatorType() == ITM_ROW_SUBQUERY)?
                       subq->getSubquery()->getDegree() :
                       udf->getRoutineDesc()->getOutputColumnList().entries(); 
         childDegree += myDegree;
           // If the subquery or MVF was the only given input, and it has a 
           // degree of 2, assign child1 of the aggregate to be the 
           // second element of the subquery/MVF output.
           // This will allow us to use a subquery or MVF to specify all 
           // inputs to STDDEV or VARIANCE
         if ((childDegree == 2) && (getArity() == 1))
         {
           ItemExprList *mDegreeList = (ItemExprList *) new(bindWA->wHeap())
                                     ItemExprList(child(chld)->castToItemExpr()
                                     ,bindWA->wHeap());

           ItemExpr * ie = mDegreeList->convertToItemExpr();
           ie->bindNode(bindWA);
           if (bindWA->errStatus()) return this;

           ValueIdList mDegreeVlist;
           ie->convertToValueIdList(mDegreeVlist, bindWA, ITM_ITEM_LIST);

           child(1) = mDegreeVlist[1].getItemExpr(); 
           // we do not need to adjust the arity, since the aggregate
           // arity method looks at how many child pointers are not null.
         }
       }
       break;

       default:
         childDegree += 1;
         break;
    }
  }
  if (getOperatorType() == ITM_STDDEV || getOperatorType() == ITM_VARIANCE)
  {
    if (childDegree < 1 || childDegree > 2 )
    {
      *CmpCommon::diags() << DgSqlCode(-4077) << DgString0(getTextUpper());
      bindWA->setErrStatus();
      return NULL;
    }
  }
  else
  {
      // We don't want to test the ITM_ONEROW since it is an internal 
      // constructs and will often have a subquery or UDF as a child 
      // The subquery or UDF outputs will eventually get a separate ITM_ONEROW
      // per output 
    if (childDegree != 1  && getOperatorType() != ITM_ONEROW )
    {
      *CmpCommon::diags() << DgSqlCode(-4476) << DgString0(getTextUpper());
      bindWA->setErrStatus();
      return NULL;
    }
  }

  context->colRefInAgg() = FALSE;
  context->inAggregate() = FALSE;

  // Return now to Variance::bindNode() after binding child of the initial
  // STDDEV or VARIANCE.  The important thing is that returning now preserves
  // the BindContext, in particular outerColRefInAgg() and (outer)aggScope.
  //
  if (getOperatorType() == ITM_STDDEV || getOperatorType() == ITM_VARIANCE)
    return getValueId().getItemExpr();

  // If second or subsequent part of any other decomposed aggregate like AVG,
  // reestablish the outer agg scope (stored in bindWA->outerAggScope())
  if (bindWA->outerAggScope()) {			// reestablish AggBind#1
    context->outerColRefInAgg() = TRUE;			//
    context->aggScope() = bindWA->outerAggScope();	// reestablish AggBind#2
  }

  if (getOperatorType() == ITM_COUNT_NONULL &&
      !isDistinct() &&
      !child(0)->getValueId().getType().supportsSQLnullLogical() &&
      !(bindWA->inDDL() && bindWA->isBindingMvRefresh()) ) {
    context->outerColRefInAgg() = FALSE;
    context->aggScope() = NULL;
    ItemExpr *countStar = new (bindWA->wHeap()) SystemLiteral(1);
    countStar = countStar->bindNode(bindWA);
    if (bindWA->errStatus()) return NULL;
    setChild(0, countStar);
    setOperatorType(ITM_COUNT);
  }

  // Outer column reference
  if (context->outerColRefInAgg()) {				// AggBind#1
    currScope->addOuterRef(getValueId());			// AggBind#2
    context->aggScope()->addLocalRef(getValueId());		// AggBind#3
    context->outerColRefInAgg() = FALSE;			// AggBind#4
    bindWA->outerAggScope() = context->aggScope();		// save for next time, maybe
    CMPASSERT(context->aggScope() != currScope);
  }
  // No column reference, i.e. constant ref:  COUNT(*) internally is COUNT(1)
  else if (context->aggScope() == NULL)
    context->aggScope() = currScope;
  else
    CMPASSERT(context->aggScope() == currScope);    // Local column reference

  // If this is an aggregate for a scalar GroupByAgg make it nullable
  // if it isn't already and if it isn't a count
  if (context->aggScope()->context()->inScalarGroupBy()) {

    // Mark the aggregate expr as one that is computed by a scalar GroupByAgg
    setInScalarGroupBy();

    if (getOperatorType() != ITM_COUNT &&
        getOperatorType() != ITM_COUNT_NONULL) {
      // Note that synthesizeNullableType() returns self if already nullable
      const NAType* nullableType =
	getValueId().getType().synthesizeNullableType(bindWA->wHeap());
      getValueId().changeType(nullableType);
    }
  }

  if (isDistinct())
    setDistinctValueId(child(0)->getValueId());

  // This may introduce a GroupByAgg in RelRoot::bindNode().
  ValueId aggrId = context->aggScope()->addUnresolvedAggregate(getValueId());

  // May have found an equivalent aggregate.  If so, make sure we use
  // the replacement.
  //
  setValueId(aggrId);

  context->aggScope() = NULL;					  // AggBind#6

  // If there is an Sequence operator for OLAP functions, then add
  // this non-OLAP Aggregate to the outputs of the Sequence operator.
  // All outputs of the Sequence operator have an NotCovered on top of
  // them
  //
  if(isOLAPSequence &&
     !context->inOtherSequenceFunction() &&
     // no need to add aggregate to sequenced columns if it is in having clasue
     // and we don't want to add NotCovered on top of it
     !context->inHavingClause()) 
  {
    ItemExpr *notCov = new (bindWA->wHeap()) NotCovered (this);
    ItemExpr *boundExpr = notCov->bindNode(bindWA);
    if (bindWA->errStatus()) return NULL;
    
    // Add the aggregate to the sequenced columns.  These represent the 
    // outputs of the Sequence Operator.  More outputs will be added
    // when the Sequence operator is bound.
    //
    sequenceNode->addSequencedColumn(boundExpr->getValueId());
    
    return boundExpr;

  } else {
    return getValueId().getItemExpr();
  }
} // Aggregate::bindNode()

// -----------------------------------------------------------------------
// Variance::bindNode()
//
// Variance is a subclass of Aggregate.
// This class implements the compiler side of the Variance and Stddev
// aggregates.  This new class redefines the {con,de}structor, the bindNode
// method, and the getText method.  The other methods are NEVER called
// for this class and have (for now) been redefined to ASSERT if called.
//
// The bindNode method does some type checking and error reporting and then
// replaces the Variance node with a tree of nodes rooted by a ScalarVariance
// node.  This new tree is bound and returned as the result of bindNode of
// Variance.  Because of this, the Variance node should never appear after
// binding.  The Variance is translated in the following way:
//
// (unweighted)
// Variance(<expr>) ->
//
// ScalarVariance(SUM(Cast(<expr>) * Cast(<expr>)),
//                SUM(Cast(<expr>)),
//                Cast(COUNT(Cast(<expr>))))
//
// (weighted)
// Variance(<expr>, <weight>) ->
//
// ScalarVariance(SUM(Cast(<expr>) * Cast(<expr>) * Cast(<weight>)),
//                SUM(Cast(<expr>) * Cast(<weight>)),
//                SUM(Cast(<weight>)))
//
// All Cast expressions are cast to Nullable SQLDoublePrecision.
// Each occurrence of a common subexpression is constructed once, then reused.
// -----------------------------------------------------------------------

static Aggregate *subaggBindNode(BindWA *bindWA,
				 BindScope *currScope,
				 BindScope *outerAggScope,
				 Aggregate *&agg,		// INOUT
				 const Aggregate *origAgg,
				 ValueId distinctId)
{
  agg->setOriginalChild(origAgg->getOriginalChild());
  agg->setOrigOpType(origAgg->origOpType());

  if (outerAggScope) {						// AggBind#1
    BindContext *context = currScope->context();
    CMPASSERT(context->aggScope() == NULL ||
    	      context->aggScope() == outerAggScope);
    context->aggScope() = outerAggScope;			// AggBind#6,1
    context->outerColRefInAgg() = TRUE;				// AggBind#4,1
  }

  agg = (Aggregate *)agg->bindNode(bindWA);			// INOUT pointer
  if (bindWA->errStatus()) return NULL;

  Aggregate *AggTmp  = agg;

  //
  // Now that we support OLAP functions, the agg->bindNode() operation above
  // may have returned a pointer to an ITM_NOTCOVERED node.  If so, we
  // need to check its child(0) node rather than the ITM_NOTCOVERED node.
  //
  if (AggTmp->getOperatorType() == ITM_NOTCOVERED )
      AggTmp = (Aggregate *)(ItemExpr *)AggTmp->child(0);

  CMPASSERT(AggTmp->isAnAggregate());

  if (distinctId != NULL_VALUE_ID)
    AggTmp->setDistinctValueId(distinctId);

  return agg; //Return ptr to original node (NOTCOVERED node, if there is one)
}

ItemExpr *Variance::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();

  if (isOLAP())
  {
    if (getArity() > 1) 
    {
      *CmpCommon::diags() << DgSqlCode(-4078) << DgString0(getTextUpper());
      bindWA->setErrStatus();
      return NULL;
    }
    ItemExpr *olap = Aggregate::bindNode(bindWA);
    if (! olap || bindWA->errStatus()) return NULL;
    
    setValueId(olap->getValueId());
    return getValueId().getItemExpr();
  }
    // Variance/Stddev must have either one or two arguments.
  // The optional second argument is the weighting parameter.
  //
  if (getArity() < 1 || getArity() > 2) {
    *CmpCommon::diags() << DgSqlCode(-4077) << DgString0(getTextUpper());
    bindWA->setErrStatus();
    return NULL;
  }

  // The steps to bind this node are:
  //
  // 1) Bind the children of this node.
  // 2) Check the datatypes of the children.
  // 3) Construct the replacement ItemExpr tree.
  // 4) Bind the newly constucted tree.
  // 5) return this as the result of binding Variance.

  // Variance is directly derived from class Aggregate.
  //
  ItemExpr *result;
  ItemExpr *tmp = Aggregate::bindNode(bindWA);
  if (bindWA->errStatus()) return NULL;

  // Needed to do this check after binding our inputs in case we had 
  // a MVF or subquery > 1 as an input. 
  const Int32 weighted = (getArity() >= 2);

  // Variance/Stddev does not support both Distinct and Weighted simultaneously.
  //
  if (weighted && isDistinct()) {
    *CmpCommon::diags() << DgSqlCode(-4078) << DgString0(getTextUpper());
    bindWA->setErrStatus();
    return NULL;
  }

  //
  // Now that we support OLAP functions, the bindNode() operation above
  // may have returned a pointer to an ITM_NOTCOVERED node.  If so, we
  // need to work with its child(0) node rather than the ITM_NOTCOVERED node.
  //
  if (tmp->getOperatorType() == ITM_NOTCOVERED )
     tmp = tmp->child(0);

  ItemExpr *boundChild0 = tmp->child(0);
  ItemExpr *boundChild1 = tmp->child(1);

  BindScope   *currScope     = bindWA->getCurrentScope();
  BindContext *context       = currScope->context();
  BindScope   *outerAggScope = NULL;
  if (context->outerColRefInAgg()) {				// AggBind#1
    outerAggScope = context->aggScope();
    CMPASSERT(outerAggScope);
  }

  // Get the types of the children determined by binding.
  // Make sure they are NUMERIC or INTERVAL (later)
  //
  const NAType& oper0 = boundChild0->getValueId().getType();
  const NAType& oper1 = weighted ? boundChild1->getValueId().getType() : oper0;

  // Only NUMERIC datatypes are supported. INTERVAL will be supported later.
  //
  if (oper0.getTypeQualifier() != NA_NUMERIC_TYPE ||
      oper1.getTypeQualifier() != NA_NUMERIC_TYPE) {
    *CmpCommon::diags() << DgSqlCode(-4079) << DgString0(getTextUpper());
    bindWA->setErrStatus();
    return NULL;
  }

  // Cast the children nodes to this type.
  // This will force the calculations to be done using
  // double precision floating point.
  // Assumes that the type propogation will make the types
  // of the child of the ScalarVariance node all double precision floats.
  //
  const NAType *desiredType = new (bindWA->wHeap()) SQLDoublePrecision(bindWA->wHeap(), TRUE);

  // Cast the first child to the desired type.
  // This is the itemExpr which should be distinct.
  //
  ItemExpr *variChild = new (bindWA->wHeap()) Cast(boundChild0, desiredType);
  ValueId distinctId = isDistinct() ? boundChild0->getValueId() : NULL_VALUE_ID;

  // Cast the second child (if it exists) to the desired type.
  // If it does not exist, it is set to NULL, and should not be used.
  //
  ItemExpr *variWeight = weighted
    ? new (bindWA->wHeap()) Cast(boundChild1, desiredType)
    : NULL;

  // The weighted child is the ItemExpr
  //   (Cast(<expr>) * Cast(<weight>)) 				-- weighted
  //   (Cast(<expr>)) 						-- unweighted
  //
  ItemExpr *weightedChild = weighted
    ? new (bindWA->wHeap()) BiArith(ITM_TIMES,
				    variChild,
				    variWeight)
    : variChild;

  // The Sum of Val Squared is the ItemExpr
  //   (SUM(Cast(<expr>) * Cast(<expr>) * Cast(<weight>)))	-- weighted
  //   (SUM(Cast(<expr>) * Cast(<expr>)))			-- unweighted
  //
  Aggregate *sumOfValSquared =
    new (bindWA->wHeap())
    Aggregate(ITM_SUM,
	      new (bindWA->wHeap())
	      BiArith(ITM_TIMES,
		      variChild,
		      weightedChild),
	      isDistinct());

  result = subaggBindNode(bindWA, currScope, outerAggScope,
  			  sumOfValSquared, this, distinctId);
  if (bindWA->errStatus()) return NULL;

  // Sum of Val is the ItemExpr (SUM(Cast(<expr>) * Cast(<weight>)))
  // for the weighted case and (SUM(Cast(<expr>)) for the unweighted case.
  //
  Aggregate *sumOfVal =
    new (bindWA->wHeap())
    Aggregate(ITM_SUM,
	      weightedChild,
	      isDistinct());

  result = subaggBindNode(bindWA, currScope, outerAggScope,
  			  sumOfVal, this, distinctId);
  if (bindWA->errStatus()) return NULL;

  // Count of Val is the ItemExpr (SUM(Cast(<weight>))) for the
  // weighted case and (Cast(COUNT(Cast(<expr>)))) for the unweighted case.
  //
  Aggregate *countOfVal =
    		 weighted
		 ? new (bindWA->wHeap()) Aggregate(ITM_SUM,
						   variWeight,
						   isDistinct())
		 : new (bindWA->wHeap()) Aggregate(ITM_COUNT_NONULL,
						   weightedChild,
						   isDistinct());

  result = subaggBindNode(bindWA, currScope, outerAggScope,
  			  countOfVal, this, distinctId);
  if (bindWA->errStatus()) return NULL;

  // The result is the ItemExpr
  //
  // (ScalarVariance(SUM(Cast(<expr>) * Cast(<expr>) * Cast(<weight>)),
  //                 SUM(Cast(<expr>) * Cast(<weight>)),
  //                 SUM(Cast(<weight>))))
  // for the weighted case and
  //
  // (ScalarVariance(SUM(Cast(<expr>) * Cast(<expr>)),
  //                 SUM(Cast(<expr>)),
  //                 Cast(COUNT(<expr>))))
  // for the unweighted case.
  //
  // Bind the newly constructed ItemExpr Tree and return it as the
  // result of binding the Variance node.  The Variance node should
  // not be seen after this point.

  result = new (bindWA->wHeap())
    ScalarVariance(getOperatorType(),
		   sumOfValSquared,
		   sumOfVal,
		   weighted
		     ? (ItemExpr *)countOfVal
		     : (ItemExpr *)new (bindWA->wHeap())
		     		   Cast(countOfVal, desiredType)
		  );
  result->setOrigOpType(origOpType());

  result = result->bindNode(bindWA);
  if (bindWA->errStatus()) return NULL;

  // Make sure that the assumptions about type propagation producing the
  // desired types are correct.
  //
  NumericType &type_ret =
    (NumericType &)(result->getValueId().getType());

  NumericType &type_op0 =
    (NumericType &)(result->child(0)->getValueId().getType());

  NumericType &type_op1 =
    (NumericType &)(result->child(1)->getValueId().getType());

  NumericType &type_op2 =
    (NumericType &)(result->child(2)->getValueId().getType());

  CMPASSERT(type_ret.getTypeQualifier() == NA_NUMERIC_TYPE &&
	    type_op0.getTypeQualifier() == NA_NUMERIC_TYPE &&
	    type_op1.getTypeQualifier() == NA_NUMERIC_TYPE &&
	    type_op2.getTypeQualifier() == NA_NUMERIC_TYPE &&
	    !type_ret.isExact() &&
	    !type_op0.isExact() &&
	    !type_op1.isExact() &&
	    !type_op2.isExact() &&
	    type_ret.getBinaryPrecision() == SQL_DOUBLE_PRECISION &&
	    type_op0.getBinaryPrecision() == SQL_DOUBLE_PRECISION &&
	    type_op1.getBinaryPrecision() == SQL_DOUBLE_PRECISION &&
	    type_op2.getBinaryPrecision() == SQL_DOUBLE_PRECISION);

  setValueId(result->getValueId());
  return getValueId().getItemExpr();
} // Variance::bindNode()

// -----------------------------------------------------------------------
// member functions for class PivotGroup
// -----------------------------------------------------------------------

ItemExpr *PivotGroup::bindNode(BindWA *bindWA)
{
  ItemExpr * result = NULL;
  
  if (nodeIsBound())
    return getValueId().getItemExpr();
  
  result = Aggregate::bindNode(bindWA);
  if (! result || bindWA->errStatus()) 
    return NULL;
  
  return result;
}

// -----------------------------------------------------------------------
// member functions for class BiArith
// -----------------------------------------------------------------------

ItemExpr *BiArith::bindNode(BindWA *bindWA)
{
  ItemExpr * result = NULL;

  if (nodeIsBound())
    return getValueId().getItemExpr();

  if (checkForSQLnullChild(bindWA, this, FALSE, ARITH_, isUnaryNegate()))
    return this;

  // Unary negate "-x" is represented as binary minus "0 - x".
  // Need to change this to "INTERVAL '0...0' qualifier - x" if x is INTERVAL.
  if (isUnaryNegate()) {
    CMPASSERT(getOperatorType() == ITM_MINUS);

    child(1) = child(1)->bindNode(bindWA);
    if (bindWA->errStatus()) return this;

    const NAType *naType = &child(1)->getValueId().getType();

    if (naType->getTypeQualifier() == NA_UNKNOWN_TYPE) {
      // Emitting errmsg here is necessary for Tandem extension to allow use of
      // INSERT INTO t VALUES(..., -DEFAULT, ...)
      // and because here we've bypassed the checking in ItemExpr::bindSelf,
      // by doing our own child() processing.
      //
      // SELECT -?param FROM t  strictly speaking deserves an error message.
      // We're going to do what the user intended, i.e. to allow it
      // (the synthType will implicitly CAST the param to some numeric type).
      // Genesis 10-981110-4799.
      //
      if (child(1)->getOperatorType() != ITM_DYN_PARAM &&
          !checkForSQLnullChild(bindWA, this, FALSE, ARITH_, isUnaryNegate()))
        CMPASSERT(FALSE);
    }
    else {

      if (naType->supportsSQLnull()) {
	// E.g., in "SELECT -CAST(99 AS INTERVAL DAY) ...",
	// child1's type is nullable (the CAST), but the child0 we're about
	// to fabricate needs to have a NONnullable type.
        NAType *t = naType->newCopy(bindWA->wHeap());
	t->setNullable(FALSE);
	naType = t;
      }

      if (naType->getTypeQualifier() == NA_INTERVAL_TYPE) {
	// NAType : IntervalType : SQLInterval : IntervalQualifier
	const IntervalQualifier *qualifier = (IntervalQualifier *)naType;
        NAString *literal;
 Lng32 tmpvallen = 20;
	char tmpval[20];
	CMPASSERT(tmpvallen >= qualifier->getTotalSize());
	qualifier->getZeroValue(tmpval, &tmpvallen, &literal, bindWA->wHeap());

	// Replace ConstValue NUMERIC "0" (from Parser) with INTERVAL "0 units"
	delete child(0).getPtr();
	child(0) = new (bindWA->wHeap()) SystemLiteral(qualifier,
						    (void *)tmpval,
						    tmpvallen,
						    literal);
      }
    }

    setIsUnaryNegate(FALSE);
  }

  // See the NOTE on side effects of rounding mode on datetime arithmetic
  // in common/OperTypeEnum.h
  // Disable rounding on arithmetic operations added to implement
  // DATE_TRUNC(). Usually these operations are added by Generator while
  // adjusting scales and Generator propagates OrigOpType these operators.
  const OperatorTypeEnum srcOrigOpType = origOpType();
  if (srcOrigOpType >= ITM_DATE_TRUNC_YEAR &&
      srcOrigOpType <= ITM_TSI_WEEK)
    setIgnoreSpecialRounding();

  // bind/type-propagate all children.
  bindChildren(bindWA);
  if (bindWA->errStatus()) 
    return this;

   Int32 childDegree = 0;
   Subquery * subq = NULL;
   UDFunction * udf = NULL;
   Int32 origArity = getArity();
      
   for (Int32 chld=0; chld < origArity; chld++)
   {
     if (child(chld)->getOperatorType() == ITM_USER_DEF_FUNCTION)
     {       
       udf = (UDFunction *) child(chld)->castToItemExpr();
       childDegree += udf->getRoutineDesc()->getOutputColumnList().entries();
     }
     else
       childDegree += 1;
   }
      
   if (childDegree > origArity) 
   {
     NAString upperFunc(getText(), bindWA->wHeap());
     
     upperFunc.toUpper();
     *CmpCommon::diags() << DgSqlCode(-4479) << DgString0(upperFunc) 
                           << DgInt1(origArity) << DgInt2(childDegree);
     
     bindWA->setErrStatus();
     return NULL;
   }

  // If special1 mode is on, then <datetime> +|- <number> and
  // <interval> +|- are allowed.
  //
  // <number> is the interval value equivalent of the least sig field 
  // of <datetime> for datetime computation.
  const NAType *naType0 = &child(0)->getValueId().getType();
  const NAType *naType1 = &child(1)->getValueId().getType();
  if (isDateMathFunction() &&
      ((naType0->getTypeQualifier() == NA_DATETIME_TYPE) &&
       (naType1->getTypeQualifier() != NA_INTERVAL_TYPE) 	))
    {
      *CmpCommon::diags() << DgSqlCode(-4116)
      			  << DgString0("DATE_ADD or DATE_SUB");
      bindWA->setErrStatus();
      return this;
    }

  if ((((naType0->getTypeQualifier() == NA_DATETIME_TYPE) &&
	(naType1->getTypeQualifier() == NA_NUMERIC_TYPE)) ||
       ((naType0->getTypeQualifier() == NA_INTERVAL_TYPE) &&
	(naType1->getTypeQualifier() == NA_NUMERIC_TYPE)) ||
       ((naType0->getTypeQualifier() == NA_NUMERIC_TYPE) &&
	(naType1->getTypeQualifier() == NA_INTERVAL_TYPE)) ) ||
      
      (isDateMathFunction() &&
       (naType0->getTypeQualifier() == NA_DATETIME_TYPE) &&
       (naType1->getTypeQualifier() == NA_INTERVAL_TYPE)) ||
      
      ((CmpCommon::getDefault(ALLOW_INCOMPATIBLE_OPERATIONS) == DF_ON) &&
       (((naType0->getTypeQualifier() == NA_INTERVAL_TYPE) &&
         (naType1->getTypeQualifier() == NA_INTERVAL_TYPE)) ||
        ((DFS2REC::isCharacterString(naType0->getFSDatatype())) &&
         (naType1->getTypeQualifier() == NA_NUMERIC_TYPE)) ||
        ((naType0->getTypeQualifier() == NA_NUMERIC_TYPE) &&
         (DFS2REC::isCharacterString(naType1->getFSDatatype()))) ||
        ((naType0->getTypeQualifier() == NA_DATETIME_TYPE) &&
         (naType1->getTypeQualifier() == NA_DATETIME_TYPE)))))
    {
      // if datetime +|- numeric, then cast numeric to interval type.
      // Validate that this is being done for
      // the appropriate numeric type(exact with scale of zero).
      if ((naType0->getTypeQualifier() == NA_DATETIME_TYPE) &&
	  (naType1->getTypeQualifier() == NA_NUMERIC_TYPE))
	{
	  const DatetimeType* datetime = (DatetimeType*)naType0;
	  const NumericType*  numeric  = (NumericType*)naType1;
	  if (((getOperatorType() == ITM_PLUS) ||
	       (getOperatorType() == ITM_MINUS)) &&
	      ((numeric->isExact()) &&
	       (NOT numeric->isBigNum()) &&
	       (numeric->getScale() == 0)))
	    {	  
	      Lng32 maxDigits = (numeric->getMagnitude() + 9) / 10;
	      maxDigits = MINOF(maxDigits, 
				SQLInterval::MAX_LEADING_PRECISION);
	      SQLInterval * interval =  
		new(bindWA->wHeap()) SQLInterval(
                     bindWA->wHeap(),
		     naType1->supportsSQLnull(),
		     datetime->getEndField(),
		     maxDigits,
		     datetime->getEndField(),
		     0);
	      
	      ItemExpr * newChild = 
		new(bindWA->wHeap()) Cast(child(1), interval);
	      setChild(1, newChild->bindNode(bindWA));
	      if (bindWA->errStatus())
		return this;
	    }
	}
	// if datetime +|- interval, then cast datetime to timestamp type.

     else if ((naType0->getTypeQualifier() == NA_DATETIME_TYPE) &&
              (naType1->getTypeQualifier() == NA_INTERVAL_TYPE))
	{
	  const DatetimeType* datetime = (DatetimeType*)naType0;
	  const IntervalType*  interval  = (IntervalType*)naType1;
	  if (interval->getEndField() > REC_DATE_DAY)
	    {	  
	   SQLTimestamp * ts = new(bindWA->wHeap()) SQLTimestamp ( bindWA->wHeap(), naType0->supportsSQLnull(),
                               interval->getFractionPrecision() > datetime->getFractionPrecision()
                               ?interval->getFractionPrecision()
                               :datetime->getFractionPrecision());
	   	      
	   ItemExpr * newChild = 
		new(bindWA->wHeap()) Cast(child(0), ts);
	      setChild(0, newChild->bindNode(bindWA));
	      if (bindWA->errStatus())
		return this;
	    }
	}
      else if ((naType0->getTypeQualifier() == NA_INTERVAL_TYPE) &&
	       (naType1->getTypeQualifier() == NA_NUMERIC_TYPE))
	{
	  const IntervalType* interval = (IntervalType*)naType0;
	  const NumericType*  numeric  = (NumericType*)naType1;
	  if ((numeric->isExact()) &&
	      (NOT numeric->isBigNum()) &&
	      (numeric->getScale() == 0) &&
	      (interval->getFractionPrecision() == 0) &&
	      ((getOperatorType() == ITM_PLUS) ||
	       (getOperatorType() == ITM_MINUS)))
	    {	  
	      Lng32 maxDigits = (numeric->getMagnitude() + 9) / 10;
	      maxDigits = MINOF(maxDigits, 
				SQLInterval::MAX_LEADING_PRECISION);
	      SQLInterval * newInterval =  
		new(bindWA->wHeap()) SQLInterval(
                     bindWA->wHeap(),
		     numeric->supportsSQLnull(),
		     interval->getEndField(),
		     maxDigits,
		     interval->getEndField(),
		     0);

	      ItemExpr * newChild = 
		new(bindWA->wHeap()) Cast(child(1), newInterval);
	      setChild(1, newChild->bindNode(bindWA));
	      if (bindWA->errStatus())
		return this;
	    }
	}
      else if ((naType0->getTypeQualifier() == NA_NUMERIC_TYPE) &&
	       (naType1->getTypeQualifier() == NA_INTERVAL_TYPE))
	{
	  if(isDateMathFunction()){
	    // 4035 cannot cast between types.
		*CmpCommon::diags() << DgSqlCode(-4035)
      			  << DgString0("NUMERIC") << DgString1("DATE");
	    bindWA->setErrStatus();
	    return this;
	  }
	  
	  const IntervalType* interval = (IntervalType*)naType1;
	  const NumericType*  numeric  = (NumericType*)naType0;
	  if ((numeric->isExact()) &&
	      (NOT numeric->isBigNum()) &&
	      (numeric->getScale() == 0) &&
	      (interval->getFractionPrecision() == 0) &&
	      ((getOperatorType() == ITM_PLUS) ||
	       (getOperatorType() == ITM_MINUS)))
	    {	  
	      Lng32 maxDigits = (numeric->getMagnitude() + 9) / 10;
	      maxDigits = MINOF(maxDigits, 
				SQLInterval::MAX_LEADING_PRECISION);
	      SQLInterval * newInterval =  
		new(bindWA->wHeap()) SQLInterval(bindWA->wHeap(),
		     numeric->supportsSQLnull(),
		     interval->getEndField(),
		     maxDigits,
		     interval->getEndField(),
		     0);

	      ItemExpr * newChild = 
		new(bindWA->wHeap()) Cast(child(0), newInterval);
	      setChild(0, newChild->bindNode(bindWA));
	      if (bindWA->errStatus())
		return this;
	    }
	}
      else if ((naType0->getTypeQualifier() == NA_INTERVAL_TYPE) &&
	       (naType1->getTypeQualifier() == NA_INTERVAL_TYPE))
	{
	  const IntervalType* interval1 = (IntervalType*)naType0;
	  const IntervalType* interval2 = (IntervalType*)naType1;
	  if ((getOperatorType() == ITM_DIVIDE) &&
	      (interval1->getFractionPrecision() == 0) &&
	      (interval2->getFractionPrecision() == 0))
	    {	  
	      const Int16 DisAmbiguate = 0;
	      ItemExpr * newChild =
		new (bindWA->wHeap())
		Cast(child(0),
		     new (bindWA->wHeap())
		     SQLNumeric(bindWA->wHeap(), TRUE,
				interval1->getTotalPrecision(),
				0,
				DisAmbiguate, // added for 64bit proj.
				child(0)->castToItemExpr()->
				getValueId().getType().supportsSQLnull()));
	      setChild(0, newChild->bindNode(bindWA));
	      if (bindWA->errStatus())
		return this;

	      newChild =
		new (bindWA->wHeap())
		Cast(child(1),
		     new (bindWA->wHeap())
		     SQLNumeric(bindWA->wHeap(), TRUE,
				interval2->getTotalPrecision(),
				0,
				DisAmbiguate, // added for 64bit proj.
				child(1)->castToItemExpr()->
				getValueId().getType().supportsSQLnull()));
	      setChild(1, newChild->bindNode(bindWA));
	      if (bindWA->errStatus())
		return this;
	    }
	}
      else if ((naType0->getTypeQualifier() == NA_DATETIME_TYPE) &&
	       (naType1->getTypeQualifier() == NA_DATETIME_TYPE) &&
               (getOperatorType() == ITM_MINUS))
	{
          // timestamp(0) - date          =  diff in days
          // date - date                  = diff in days
          //
          // In mode_special_4,
          // Column of DATE datatype is internally created as TIMESTAMP(0)
          // and their diff is in days.
          // timestamp(0) - timestamp(0)  = diff in days
	  const DatetimeType* datetime1 = (DatetimeType*)naType0;
	  const DatetimeType* datetime2 = (DatetimeType*)naType1;
          if (((datetime1->getSubtype() == DatetimeType::SUBTYPE_SQLTimestamp) &&
               (datetime2->getSubtype() == DatetimeType::SUBTYPE_SQLDate)) ||
              ((datetime1->getSubtype() == DatetimeType::SUBTYPE_SQLDate) &&
               (datetime2->getSubtype() == DatetimeType::SUBTYPE_SQLDate)) ||
              ((CmpCommon::getDefault(MODE_SPECIAL_4) == DF_ON) &&
               (datetime1->getSubtype() == DatetimeType::SUBTYPE_SQLTimestamp) &&
               (datetime2->getSubtype() == DatetimeType::SUBTYPE_SQLTimestamp) &&
               (datetime1->getFractionPrecision() == 0) &&
               (datetime2->getFractionPrecision() == 0)))
            {
              ItemExpr * newChild = NULL;
              if (datetime1->getSubtype() == DatetimeType::SUBTYPE_SQLTimestamp)
                {
                  newChild = new (bindWA->wHeap())
                    Cast(child(0),
                         new (bindWA->wHeap())
                         SQLDate(bindWA->wHeap(), datetime1->supportsSQLnull()));
                  setChild(0, newChild->bindNode(bindWA));
                  if (bindWA->errStatus())
                    return this;
                }
 
              if (datetime2->getSubtype() == DatetimeType::SUBTYPE_SQLTimestamp)
                {
                  newChild = new (bindWA->wHeap())
                    Cast(child(1),
                         new (bindWA->wHeap())
                         SQLDate(bindWA->wHeap(), datetime2->supportsSQLnull()));
                  setChild(1, newChild->bindNode(bindWA));
                  if (bindWA->errStatus())
                    return this;
                }
	    }
	}
      else
	{
	  Int32 srcChildIndex = -1;
	  Int32 otherChildIndex = -1;
	  if (naType0->getTypeQualifier() == NA_CHARACTER_TYPE)
	    {
	      srcChildIndex = 0;
	      otherChildIndex = 1;
	    }
	  else
	    {
	      srcChildIndex = 1;
	      otherChildIndex = 0;
	    }

	  const NumericType &numeric = (NumericType&)
	    child(otherChildIndex)->getValueId().getType();

	  if ((numeric.isExact()) &&
	      (NOT numeric.isBigNum()) &&
	      (numeric.getScale() == 0))
	    {
	      //doing a char to numeric conversion
	      // convert to largeint.
	      ItemExpr * newChild =
		new (bindWA->wHeap())
		Cast(child(srcChildIndex),
		     new (bindWA->wHeap())
		     SQLLargeInt(bindWA->wHeap(), TRUE,
				 child(srcChildIndex)->castToItemExpr()->
				 getValueId().getType().supportsSQLnull()));
	      setChild(srcChildIndex, newChild->bindNode(bindWA));
	    }
	  else
	    {
	      // convert to double precision. This will handle all precision,
	      // scale and type specified in the char value.
	      ItemExpr * newChild =
		new (bindWA->wHeap())
		Cast(child(srcChildIndex),
		     new (bindWA->wHeap())
		     SQLDoublePrecision(bindWA->wHeap(), child(srcChildIndex)->castToItemExpr()->getValueId().getType().supportsSQLnull()));
	      setChild(srcChildIndex, newChild->bindNode(bindWA));
	    }
	}
    }
  
  // (BiArith is a directly derived subclass of ItemExpr; safe to invoke this)
  result = ItemExpr::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return this;

  NAType * result_type = (NAType *)(&(getValueId().getType()));
  if ((result_type->getTypeQualifier() == NA_NUMERIC_TYPE) &&
      (getRoundingMode()) && 
      (getOperatorType() == ITM_DIVIDE) &&
      (((NumericType*)result_type)->isExact()) &&
      (result_type->getFSDatatype() != REC_BIN64_SIGNED))
    {
      // rounded division are only supported for Int64 datatypes.
      // Make the result to be NUMERIC(MAX_PRECISION, original_result_scale).
      // Save the current result attribute (attr_result).
      // Converted rounded result to the original result datatype.
      
      const Int16 DisAmbiguate = 0;
      NAType * orig_result_type = result_type->newCopy(bindWA->wHeap());
      result_type = new(bindWA->wHeap()) 
	SQLNumeric(bindWA->wHeap(), TRUE, 
		   MAX_NUMERIC_PRECISION,
		   result_type->getScale(),
                   DisAmbiguate,
		   result_type->supportsSQLnull());
      getValueId().changeType(result_type);

      // convert result back to its original type
      result = new(bindWA->wHeap()) Cast(result, orig_result_type);
      result = result->bindNode(bindWA);
      if (bindWA->errStatus()) 
	return this;
    }
  
  return result;

} // BiArith::bindNode()

// -----------------------------------------------------------------------
// member functions for class UnArith
// -----------------------------------------------------------------------

ItemExpr *UnArith::bindNode(BindWA *bindWA)
{
  ItemExpr * result = NULL;

  if (nodeIsBound())
    return getValueId().getItemExpr();

  CMPASSERT(getOperatorType() == ITM_NEGATE);

  child(0) = child(0)->bindNode(bindWA);
  if (bindWA->errStatus()) 
    return this;

  result = ItemExpr::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return this;

  return result;

} // UnArith::bindNode()

// -----------------------------------------------------------------------
// member functions for class Assign
// -----------------------------------------------------------------------
// Helper function
// Soln:10-040915-9806 This function is used to find the mapping between
// the exposed name and the base column name. This was initially introduced
// to find the mapping between the view column name and the base column
// name to report proper names in error messages.
const NAString& findMappedName(ValueId vid,BindWA *bindWA)
{
   ValueIdList colList;
   RETDesc *myRetDesc = bindWA->getCurrentScope()->getRETDesc();
   myRetDesc->getValueIdList(colList);
   for (CollIndex i = 0; i < colList.entries(); i++)
   {
      if(colList[i].getNAColumn() == vid.getNAColumn())
          return myRetDesc->findColumn(colList[i])->getColRefNameObj().getColName();
   }
   return vid.getNAColumn()->getColName();
}

// -----------------------------------------------------------------------
// member functions for class Assign
// -----------------------------------------------------------------------

ItemExpr *Assign::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();

#ifndef NDEBUG
  if (getenv("ASSIGN_DEBUG")) {
    NAString unparsed(bindWA->wHeap());
    unparse(unparsed);
    cout << getValueId() << ":	" << unparsed << endl;
  }
#endif  

  ItemExpr *boundExpr = NULL;
  ItemExpr *boundExpr_0, *boundExpr_1 ;

  boundExpr_0 = child(0)->bindNode(bindWA);
  if (bindWA->errStatus())
      return this;
  child(0) = boundExpr_0;
 
  if (child(1))
    {
      boundExpr_1 = child(1)->bindNode(bindWA);
      if (bindWA->errStatus())
        return this;
      child(1) = boundExpr_1;
    }
 
 
  NABuiltInTypeEnum targetType =  child(0)->castToItemExpr()->getValueId().getType().getTypeQualifier() ;
  if  (targetType == NA_LOB_TYPE)
    {  
      if (child(1))
        {
          NABuiltInTypeEnum sourceType =  child(1)->castToItemExpr()->getValueId().getType().getTypeQualifier() ; 
          //If it's a dynamic param with unknown type or if it is a 
          // character type, trasnform the insert.
          if ((((child(1)->getOperatorType() == ITM_DYN_PARAM) ||(child(1)->getOperatorType() == ITM_ROWSETARRAY_SCAN))  && sourceType == NA_UNKNOWN_TYPE)  || sourceType == NA_CHARACTER_TYPE)
            {
              ValueId vid1 = child(1)->castToItemExpr()->getValueId();  
              // Add a stringToLob node
              ItemExpr *newChild;
              const NAType &desiredType = child(0)->getValueId().getType();
              SQLBlob &lobType = (SQLBlob&)desiredType;
              short fs_datatype = child(0)->castToItemExpr()->getValueId().getType().getFSDatatype();

              NAType * newType = NULL;

              double lob_input_limit_for_batch = CmpCommon::getDefaultNumeric(LOB_INPUT_LIMIT_FOR_BATCH)*1024;
                  double lob_size = lobType.getLobLength();
              if (fs_datatype == REC_CLOB) {
                newType = new (bindWA->wHeap()) SQLClob(bindWA->wHeap(), lob_input_limit_for_batch < lob_size ? lob_input_limit_for_batch : lob_size,
                         lobType.getLobStorage(),
                         TRUE, FALSE, FALSE,
                         lob_input_limit_for_batch < lob_size ? lob_input_limit_for_batch : lob_size);
              }
              else {
                newType = new (bindWA->wHeap()) SQLBlob(bindWA->wHeap(),lob_input_limit_for_batch < lob_size ? lob_input_limit_for_batch : lob_size ,
                                             lobType.getLobStorage(), 
                                             TRUE, FALSE, FALSE, 
                                             lob_input_limit_for_batch < lob_size ? lob_input_limit_for_batch : lob_size);
              }
             
              CMPASSERT(lob_input_limit_for_batch < INT_MAX);
              vid1.coerceType(*newType, NA_LOB_TYPE); 
              if (bindWA->getCurrentScope()->context()->inUpdate())
                {
                  newChild =  new (bindWA->wHeap()) LOBupdate( vid1.getItemExpr(), child(0), NULL,LOBoper::STRING_, FALSE,TRUE);
                }
              else
                {
                  newChild =  new (bindWA->wHeap()) LOBinsert( vid1.getItemExpr(),NULL, LOBoper::STRING_, FALSE,TRUE); 
                }   
              newChild->bindNode(bindWA);
              if (bindWA->errStatus())
                return boundExpr; 
              setChild(1, newChild);
            }
        }
    }
  // Assign is a directly derived subclass of ItemExpr; safe to invoke this
  boundExpr = ItemExpr::bindNode(bindWA);
  if (bindWA->errStatus())
      return boundExpr;
	    

  //
  if (isUserSpecified()) {
    //
    // Ensure the target is a column;
    // and that it is a user column (4013).
    //
    const NAColumn *nacolTgt = child(0).getNAColumn();
    if (nacolTgt->isSystemColumn()) {
      *CmpCommon::diags() << DgSqlCode(-4013)
      			  << DgColumnName(nacolTgt->getColName());
      bindWA->setErrStatus();
      return boundExpr;
    }
    //
    // ## For first release,
    // ## ensure it is not a clustering column being updated (4033)
    // ## (when we do allow it we of course must ensure, just as insert does,
    // ## that the resulting row does not violate PK uniqueness constraint).
    //
    if (bindWA->getCurrentScope()->context()->inUpdate())
      if (nacolTgt->isClusteringKey() ||
	  nacolTgt->isPrimaryKey()) 
      {
	if (CmpCommon::getDefault(UPDATE_CLUSTERING_OR_UNIQUE_INDEX_KEY) == DF_OFF)
	{
	  *CmpCommon::diags() << DgSqlCode(-4033)
	      		<< DgColumnName(findMappedName(child(0).getValueId(),bindWA));
	  bindWA->setErrStatus();
	  return boundExpr;
	}
      }

    //
    // If target requires upshifting, and source is not already upshifted,
    // then interpose a new upshift-function node and bind it.
    // This need not be done if not user-specified, i.e. if default value,
    // for CatMan should be storing upshift columns' default values already
    // upshifted.
    //
    if (nacolTgt->isUpshiftReqd()) {
      boundExpr = applyUpperToSource(bindWA, boundExpr, 1);
      if (bindWA->errStatus()) return boundExpr;
    }

    // QSTUFF
    // For ON ROLLBACK assignment statements we need to ensure that the
    // respective column is not a clustering key or index column and that
    // the respective column size is of fixed size since we can't
    // handle variable length columns during abort.
    if (onRollback()) {

      NAString str0(bindWA->wHeap());
      if      (nacolTgt->isIndexKey())        str0 = "Index Key";
      else if (nacolTgt->isPrimaryKey())      str0 = "Primary Key";
      else if (nacolTgt->isClusteringKey())   str0 = "Clustering Key";
      else if (nacolTgt->isPartitioningKey()) str0 = "Partitioning Key";
      // if (nacolTgt->isReferencedForHistogram())	str0 = "Histogram Reference";

      if (!str0.isNull()) {
        *CmpCommon::diags() << DgSqlCode(-4177)
          << DgString0(str0)
	  << DgColumnName(nacolTgt->getColName());
        bindWA->setErrStatus();
        return boundExpr;
      }

      if (DFS2REC::isAnyVarChar(nacolTgt->getType()->getFSDatatype())) {
        *CmpCommon::diags() << DgSqlCode(-4178)
          << DgColumnName(nacolTgt->getColName());
        bindWA->setErrStatus();
        return boundExpr;
      }
      // test for not null on rollback
      if (nacolTgt->getType()->supportsSQLnullLogical()) {
        *CmpCommon::diags() << DgSqlCode(-4209)
          << DgColumnName(nacolTgt->getColName());
        bindWA->setErrStatus();
        return boundExpr;
      }
    } // QSTUFF

  } // isUserSpecified
 
 targetType =  child(0)->castToItemExpr()->getValueId().getType().getTypeQualifier() ;
 
  if ((NOT child(0)->getValueId().getType().
       isCompatible(child(1)->getValueId().getType())) &&
      (CmpCommon::getDefault(ALLOW_INCOMPATIBLE_OPERATIONS) == DF_ON) &&
      ((child(1)->getOperatorType() != ITM_CONSTANT) ||
       (NOT ((ConstValue *) child(1).getPtr() )->isNull())))
    {
      // target type is not the same as source type.
      // Add an explicit CAST node.
      // All supported incompatible conversions will be handled by CAST.
      ItemExpr * newChild = 
	new(bindWA->wHeap()) Cast(child(1),
				  &child(0)->getValueId().getType());
      newChild->bindNode(bindWA);
      if (bindWA->errStatus())
	return boundExpr;
      setChild(1, newChild);
    }
    
 
  // If we assign a numeric type and the source has a larger scale then
  // the target we cast the source to reduce the scale (truncate).
  // We also cast (truncate) if we deal with char and the source is larger
  // than the target.
  targetType =  child(0)->castToItemExpr()->getValueId().getType().getTypeQualifier() ;
  if (targetType == NA_CHARACTER_TYPE) {
    Lng32 sourceLength = ((CharType&)(child(1)->getValueId().getType())).getStrCharLimit();
    Lng32 targetLength = ((CharType&)(child(0)->getValueId().getType())).getStrCharLimit();
    Lng32 sourceLength_bytes = ((CharType&)(child(1)->getValueId().getType())).getNominalSize();
    Lng32 targetLength_bytes = ((CharType&)(child(0)->getValueId().getType())).getNominalSize();
    if ( (targetLength < sourceLength) || (targetLength_bytes < sourceLength_bytes) ){
      ItemExpr *newChild;
      
      // if the targetLength is smaller than sourceLength, and since the
      // target type is of character type, make sure to set the
      // checkTruncationError flag in the cast node if this is an insert
      // or an update command. If MODE_SPECIAL_1 is on, then turn off the
      // checkTruncationError flag. Also, turn off string truncation warnings
      // in this case.
      
      OperatorTypeEnum opType = bindWA->getCurrentScope()->context()
	->inUpdateOrInsert();
      if ((opType == REL_UPDATE) || (opType == REL_INSERT))
	{
	  NABoolean specialMode = 
	    (CmpCommon::getDefault(MODE_SPECIAL_1) == DF_ON);

          NABoolean checkForTrunc = TRUE;
          NABoolean noStringTruncWarn = FALSE;
          if (specialMode)
            {
              checkForTrunc = FALSE;
              noStringTruncWarn = TRUE;
            }
          else
            {
              if (CmpCommon::getDefault(TRAF_STRING_AUTO_TRUNCATE) == DF_ON)
                {
                  checkForTrunc = FALSE;
                  noStringTruncWarn = TRUE;
                  if (CmpCommon::getDefault(TRAF_STRING_AUTO_TRUNCATE_WARNING) == DF_ON)
                    noStringTruncWarn = FALSE;
                }
            }

	  newChild = new(bindWA->wHeap()) Cast(child(1),
					       &child(0)->getValueId().getType(),
					       ITM_CAST,
                                               checkForTrunc,
                                               noStringTruncWarn);
	}
      else
	newChild = new (bindWA->wHeap()) Cast(child(1),
					      &child(0)->getValueId().getType());
      setChild(1, newChild->bindNode(bindWA));
      if (bindWA->errStatus())
	return boundExpr;
    }
  }
  else if (targetType == NA_NUMERIC_TYPE) {
    NumericType *source = (NumericType*) &child(1)->getValueId().getType();
    NumericType *target = (NumericType*) &child(0)->getValueId().getType();
    if (target->getScale() < source->getScale()) {
      ItemExpr * newChild = new(bindWA->wHeap()) Cast(child(1),
						      &child(0)->getValueId().getType());
      setChild(1, newChild->bindNode(bindWA));
      if (bindWA->errStatus())
	return boundExpr;
    }
  }
  else if (targetType == NA_DATETIME_TYPE) {
    DatetimeIntervalCommonType *source =
      (DatetimeIntervalCommonType*) &child(1)->getValueId().getType();
    DatetimeIntervalCommonType *target =
      (DatetimeIntervalCommonType*) &child(0)->getValueId().getType();
    if (target->getFractionPrecision() < source->getFractionPrecision()) {
      ItemExpr * newChild = new(bindWA->wHeap()) Cast(child(1),
						      &child(0)->getValueId().getType());
      setChild(1, newChild->bindNode(bindWA));
      if (bindWA->errStatus())
	return boundExpr;
    }
  }
  
  if (!child(0)->getValueId().getType().supportsSQLnull() &&
      child(1)->getOperatorType() == ITM_CONSTANT &&
      ( (ConstValue *) child(1).getPtr() )->isNull())
    {
      // - Triggers
      // Check if this table has before triggers on it.
      // If so - don't create the error. Maybe a before trigger will fix this
      // NULL value for us. If not - the error will be caught in execution.
      // Don't check on the temp table, so the error will not be on the wrong table.
      const NAColumn *nacol = child(0)->getValueId().getNAColumn(TRUE/*no err*/);
      if (nacol != NULL)
      {
        if (nacol->getNATable()->getSpecialType() !=
                    ExtendedQualName::TRIGTEMP_TABLE)
	{
	  QualifiedName table(*nacol->getTableName(), bindWA->wHeap());
          if ( nacol->getNATable()->getSpecialType() ==
                                      ExtendedQualName::GHOST_TABLE)
              table.setIsGhost(TRUE);
	  ComOperation op = COM_UNKNOWN_IUD;
	  switch (bindWA->getCurrentScope()->context()->inUpdateOrInsert())
	    {
	    case REL_INSERT: op = COM_INSERT;
	      break;
	    case REL_UPDATE: op = COM_UPDATE;
	      break;
	    default        : CMPASSERT(FALSE);
	    }
	  BeforeAndAfterTriggers *allTriggers =
	    bindWA->getSchemaDB()->getTriggerDB()->getTriggers(table, op, bindWA);
	  if (bindWA->errStatus())
	    return boundExpr;
	  if ((allTriggers==NULL) || (allTriggers->getBeforeTriggers()==NULL))
	    {
	      // No before triggers found. Go ahead with the error.
	      // 4122 NULL cannot be assigned to NOT NULL column $column.
	      NAString colname(nacol->getFullColRefNameAsAnsiString());
		  *CmpCommon::diags() << DgSqlCode(-4122) << DgColumnName(colname);
	      bindWA->setErrStatus();
	    }
	}
      }
      else
      {
	  NAString colname("");
	  *CmpCommon::diags() << DgSqlCode(-4122) << DgColumnName(colname);
	  bindWA->setErrStatus();
      }
    }

  return boundExpr;
} // Assign::bindNode()



// -----------------------------------------------------------------------
// member functions for class Cast
// -----------------------------------------------------------------------

ItemExpr *Cast::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();

  // Cast inherits from BuiltinFunction .. Function .. ItemExpr.
  ItemExpr *boundExpr = BuiltinFunction::bindNode(bindWA);
  if (bindWA->errStatus())
    return boundExpr;

  if (getType()->getTypeQualifier() == NA_CHARACTER_TYPE &&
      ((CharType *)getType())->isUpshifted())
    boundExpr = applyUpperToSource(bindWA, boundExpr, 0);

  // currently const folding is not being done if cast to BINARY datatype
  if (DFS2REC::isBinaryString(getType()->getFSDatatype()))
    setConstFoldingDisabled(TRUE);

// COMMENTED OUT -- causing problems in Generator key-building --	     ##
// FIX LATER -- for now, just catch this problem at run-time instead of compile,
// via Executor error 8421 ...
//
//  if (!getType()->supportsSQLnull() &&
//      child(0)->getOperatorType() == ITM_CONSTANT &&
//      ( (ConstValue *) child(0).getPtr() )->isNull()) {
//    // 4123 NULL cannot be cast to a NOT NULL datatype.
//    *CmpCommon::diags() << DgSqlCode(-4123);
//    bindWA->setErrStatus();
//  }

  // in mode_special_1, if a character datatype is being converted
  // to a numeric type, then treat an empty string or a string with all spaces
  // to be the same as the value 0.
  if ((CmpCommon::getDefault(MODE_SPECIAL_1) == DF_ON) &&
      (child(0)->castToItemExpr()->getValueId().getType().getTypeQualifier() == NA_CHARACTER_TYPE) &&
      (getType()->getTypeQualifier() == NA_NUMERIC_TYPE))
    {
      setTreatAllSpacesAsZero(TRUE);
    }

  return boundExpr;
} // Cast::bindNode()

// -----------------------------------------------------------------------
// member functions for class Like
// -----------------------------------------------------------------------

ItemExpr *PatternMatchingFunction::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();

  NABoolean savedInPred = bindWA->getCurrentScope()->context()->inPredicate();
  bindWA->getCurrentScope()->context()->inPredicate() = TRUE;

  // this will bind/type-propagate all children.
  bindChildren(bindWA);
  if (bindWA->errStatus()) 
    return this;

  //=================================================================
  //10-040212-3209-begin
  // Like inherits from Function .. ItemExpr.
  //10-040212-3209-end

  // case 10-020314-7397
  // BuiltInFunction::bindNode() seems to add a cast node on top of
  // all base columns and this results in a full table scan
  // for queries like
  // select * from t where a like 'abc%';
  // hence like will inherit from Function not BuiltInFunction
  //==================================================================
  ItemExpr *boundExpr = Function::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return this;

  // update both operands if case insensitive comparions
  // are to be done.
  const NAType &type1 = 
    child(0)->castToItemExpr()->getValueId().getType();
  const NAType &type2 = 
    child(1)->castToItemExpr()->getValueId().getType();
  
  if ((type1.getTypeQualifier() == NA_CHARACTER_TYPE) &&
      (type2.getTypeQualifier() == NA_CHARACTER_TYPE))
    {
      const CharType &cType1 = (CharType&)type1;
      const CharType &cType2 = (CharType&)type2;
      
      NABoolean doCIcomp = 
      	((cType1.isCaseinsensitive()) && (cType2.isCaseinsensitive()));
      ItemExpr * newChild = NULL;
      if (doCIcomp) 
        {
          if (NOT cType1.isUpshifted())
            {
              newChild = new (bindWA->wHeap()) Upper(child(0));
              newChild->bindNode(bindWA);
              setChild(0, newChild);
            }
          if (NOT cType2.isUpshifted())
            {
              newChild = new (bindWA->wHeap()) Upper(child(1));
              newChild->bindNode(bindWA);
              setChild(1, newChild);
            }
          // for CaseInSensitive escape character, Soln 10-080310-1225
          if (getArity()>2)  
            {
              const NAType &type3 = 
                child(2)->castToItemExpr()->getValueId().getType();
              if (type3.getTypeQualifier() == NA_CHARACTER_TYPE)
              {
                const CharType &cType3 = (CharType&)type3;
                if (NOT cType3.isUpshifted())
                  {
                    newChild = new (bindWA->wHeap()) Upper(child(2));
                    newChild->bindNode(bindWA);
                    setChild(2, newChild);
                  }
              }
            }  
        }
 
    }

  bindWA->getCurrentScope()->context()->inPredicate() = savedInPred;

  return applyBeginEndKeys(bindWA, boundExpr, bindWA->wHeap());

} // PatternMatchingFunction::bindNode()

NABoolean PatternMatchingFunction::beginEndKeysApplied(CollHeap *heap)
{
  // Called by optimizer, long after binding (thus bindNode has already
  // called the common method applyBeginEndKeys and done the appropriate
  // node allocations there).  Calling applyBeginEndKeys again in optimizer
  // rather than setting a flag in the Like object was chosen so we don't
  // have to analyze the survivability of a new flag across optimizer's
  // copyTopNode and rules assigning object members here and there.

  return beginEndKeysApplied_;

} // PatternMatchingFunction::beginEndKeysApplied()

ItemExpr *Regexp::applyBeginEndKeys(BindWA *bindWA, ItemExpr *boundExpr,
				  CollHeap *heap)
{
  return boundExpr;
}

ItemExpr *Like::applyBeginEndKeys(BindWA *bindWA, ItemExpr *boundExpr,
				  CollHeap *heap)
{
  CMPASSERT((bindWA && boundExpr) || (!bindWA && !boundExpr));
  CMPASSERT(heap);

  // Assert that Like::bindNode, and importantly, Like::synthesizeType,
  // has been called -- the latter ensures all LIKE arguments are of
  // compatible/coercible character string type.
  CMPASSERT(nodeIsBound());

  // Now we want to optimize the common case where the pattern is a literal
  // and so is the optional escape character, if one.
  // Note that we don't care about the match value (first argument) here.
  // Help the optimizer by ANDing another predicate above this node,
  // letting the optimizer estimate selectivity and avoid a full table scan.
  //	mv LIKE 'ab%yz'  ->  mv >= 'ab{0}' AND mv LIKE 'ab%yz' AND mv < 'ac{0}'
  //	mv LIKE 'ab_yz'  ->  mv >= 'ab{0}' AND mv LIKE 'ab_yz' AND mv < 'ac{0}'
  // where {0} is a sequence of enough ascii-zero characters to fill the
  // literal comparand out to the same length as mv.
  //
  // Further, LIKE processing can be optimized away entirely:
  //   	mv LIKE 'ab%'    ->  mv >= 'ab{0}' AND mv < 'ac{0}'
  //    ('ab%', 'ab%%', 'ab%%%' -- all can have the LIKE optimized away.)
  // Note that 'ab_' CANNOT have the LIKE optimized away.
  //
  // mv LIKE 'ab'	->  mv = 'ab' [if mv is CHAR type]
  // mv LIKE '%'	->  if mv is nullable return IS_NOT_NULL else return TRUE
  //
  // We do not optimize case like:
  //	mv LIKE 'ab'	if mv is of VARCHAR type
  //
  // The following begin with wildcards; no optimization is possible:
  //	mv LIKE '%yz' , mv LIKE '_yz' , mv LIKE '_'	-- no change
  //
  // Some of this code copied from ex_like_clause::eval().

  ItemExpr *matchExpr = child(0)->castToItemExpr();		// arity 1
  ItemExpr *strExpr = child(1)->castToItemExpr();		// arity 2

  const ConstValue *patternNode, *escapeNode = NULL;

  NABoolean specialMode = (CmpCommon::getDefault(MODE_SPECIAL_1) == DF_ON);

  if (specialMode)
  {
    if(strExpr->getOperatorType() == ITM_UPPER)
      strExpr = strExpr->child(0);
  }

  patternNode = (ConstValue *)strExpr->castToItemExpr();	// arity 2
  NABoolean optimizeLike =
    patternNode->getOperatorType() == ITM_CONSTANT && !patternNode->isNull();
  if (getArity() > 2)
  {
    ItemExpr *escapeExpr = child(2)->castToItemExpr();
    if (specialMode)
    {
      if(escapeExpr->getOperatorType() == ITM_UPPER)
	escapeExpr = escapeExpr->child(0);
    }
    escapeNode = (ConstValue *)escapeExpr->castToItemExpr();	// arity 3
    optimizeLike = optimizeLike &&
      escapeNode->getOperatorType() == ITM_CONSTANT && !escapeNode->isNull();
  }

  // The following comment is added based on the inspection discussion for fix
  // for 10-040127-4126 (NF: LIKE with ESCAPE kanji char '_' gets assertion failure ).
  //
  // optimizerLike = true means both the pattern and the escape clause contain constants
  // are not NULL. It is possible that this optimization step is bypassed when either
  // clause contains a non-constant expression with all components are constants. After
  // constant-folding, the expression evaluates to a constant value and the optimization
  // step can be performed, if this function is called (occasionally from the Analyzer
  // or the Optimizer). If this function is not called, then some error, if any, will be
  // caught during run-time.
  //
  //
  // On the other hand, the cost of calling binder after const-folding, the infrequent
  // use of such constant expressions in real-world queries, and the error will get
  // caught during run-time, this defect can be considered as a "limitation or regret"
  // of the current architecture. It is a defect nice-to-be-fixed but the overhead of
  // fixing it and its consequence can be very  big  for us to bear.
  //
  if (optimizeLike)
  {
    // Seems like a weird way to cast, but only way to do in on NT C++
    const CharType *matchCharType = (const CharType *)
				    &matchExpr->getValueId().getType();
    // Already ensured by Like::synthesizeType, but what the heck...

    CMPASSERT(matchCharType->getTypeQualifier() == NA_CHARACTER_TYPE);

    // 2/15/98: remove the following assertion.
    //CMPASSERT(matchCharType->getBytesPerChar() == 1);

    const char *escapeChar = NULL;
    Int32 escapeChar_len = 0;
    if (escapeNode)
    {
      CharInfo::CharSet cs = matchCharType->getCharSet();
      escapeChar_len = escapeNode->getRawText()->length();
      escapeChar = escapeNode->getRawText()->data();

      if      (  CharInfo::isSingleByteCharSet(cs) && escapeChar_len == 1) /*ok*/;
      else if (! CharInfo::isSingleByteCharSet(cs) && escapeChar_len == 2) /*ok*/;
      else
      {
	*CmpCommon::diags() << DgSqlCode(EXE_INVALID_ESCAPE_CHARACTER,
					 DgSqlCode::ERROR_);
	if (bindWA)
	  bindWA->setErrStatus();
	return boundExpr;
      }
    } // if (escapeNode)

    //
    // We currently cannot optimize if CZECH collation is involved
    // because the Optimizer doesn't understand various things about
    // CZECH collation ... such as H < CH < I.
    //
    // Since all arguments to LIKE must have the same collation (or
    // they would not be comparable), we need to check only one
    // of the arguments.
    //
    CharInfo::Collation Coll = matchCharType->getCollation();
    if ( Coll == CharInfo::CZECH_COLLATION )
    {
      return boundExpr; // Cannot optimize
    }

  // 2/15/98: prepare the right version of underscore,
  // percent any bytesPerChar info.

   const char* underscoreChar;
   UInt16 underscoreChar_len = BYTES_PER_NAWCHAR;
   NAWchar wideUnderscoreChar;
   switch (matchCharType->getCharSet())
   {
     case CharInfo::UNICODE:
       underscoreChar = (char*)L"_";
       break;

     case CharInfo::KANJI_MP:
       wideUnderscoreChar = kanji_char_set::underscore_char();
       underscoreChar = (char*)&wideUnderscoreChar;
       break;

     case CharInfo::KSC5601_MP:
       wideUnderscoreChar = ksc5601_char_set::underscore_char();
       underscoreChar = (char*)&wideUnderscoreChar;
       break;

     case CharInfo::ISO88591:
     default:
       underscoreChar = (char*)"_";
       underscoreChar_len = 1;
       break;
   }

   const char* percentChar;
   UInt16 percentChar_len = BYTES_PER_NAWCHAR;
   NAWchar widePercentChar;
   switch (matchCharType->getCharSet())
   {
     case CharInfo::UNICODE:
       percentChar = (char*)L"%";
       break;

     case CharInfo::KANJI_MP:
       widePercentChar = kanji_char_set::percent_char();
       percentChar = (char*)&widePercentChar;
       break;

     case CharInfo::KSC5601_MP:
       widePercentChar = ksc5601_char_set::percent_char();
       percentChar = (char*)&widePercentChar;
       break;

     case CharInfo::ISO88591:
     default:
       percentChar = (char*)"%";
       percentChar_len = 1;
       break;
   }

   const char* pattern_str = 0;
   Int32 pattern_str_len = 0;

   pattern_str = patternNode->getRawText() -> data();
   pattern_str_len = patternNode->getRawText()->length();

    CharInfo::CharSet cs = matchCharType->getCharSet();


    LikePatternString patternString( pattern_str,
				     pattern_str_len, cs,
				     escapeChar, escapeChar_len,
				     underscoreChar, underscoreChar_len,
				     percentChar, percentChar_len
				   );

    LikePattern pattern(patternString, heap, cs);

    if (pattern.error())
    {
      if(pattern.error() == EXE_INVALID_CHARACTER)
      {
        *CmpCommon::diags() << DgSqlCode(pattern.error(), DgSqlCode::ERROR_)
          << DgString0(CharInfo::getCharSetName(cs))
          << DgString1("LIKE PATTERN"); 
      }
      else
        *CmpCommon::diags() << DgSqlCode(pattern.error(), DgSqlCode::ERROR_);

      // fix for 10-040127-4126.
      // Like::beginEndKeysApplied(CollHeap *heap) actually supplies
      // a NULL bindWA and NULL boundExpr when calling this method during the ANALYSIS
      // phase. Need to check the nullness of bindWA before call its members.
      if (bindWA)
        bindWA->setErrStatus();
      return boundExpr;
    }

    if (bindWA)
    {
      // we are here from the Binder and not from the Optimizer
      // so calculate the total number of non_wildcard characters
      // including underscores and set them in the Like expression.
      // This will be used later in the Optimizer to estimate the
      // selectivity
      setNumberOfNonWildcardChars(patternString);

      // do all equality and '%' transformations, only if the call is from
      // the Binder.

      // We first check if the predicate like '%' is being applied to a
      // not_nullable column. If the column does not allow NULLS, then
      // the like % is transformed to '*' or TRUE. Similarly predicates such 
      // as col LIKE '____'(N underscores) for not nullable columns of length 
      // N are converted to TRUE.
      // For nullable columns, the LIKE predicate is transformed to IS_NOT_NULL.

      Int32 BytesInNonWildCardChars = getBytesInNonWildcardChars();
      if (BytesInNonWildCardChars == 0)
      {
	if (( pattern.getNextHeader() &&
	    !pattern.getNextHeader()->getNextHeader() &&
	    (pattern.getNextHeader()->getType() == LikePatternStringIterator:: PERCENT) &&
            ((pattern.getType() != LikePatternStringIterator:: UNDERSCORE) ||  // like '%'
              (matchCharType->getNominalSize() >= pattern.getLength()) &&
              (!matchCharType->isVaryingLen())))  // like '___%', but not like '___%_'  col len >= # of underscores
	    ||
	    (!pattern.getNextHeader() &&
	    (pattern.getType() == LikePatternStringIterator:: UNDERSCORE) &&
            (!matchCharType->isVaryingLen()) &&
            (!CharInfo::isVariableWidthMultiByteCharSet(cs)) &&
            // like '___', column must be same length as pattern
	    (matchCharType->getStrCharLimit() == pattern.getLength())))
	{
	  ItemExpr * result = NULL;
	  if (!(matchExpr->getValueId().getType().supportsSQLnullLogical()) )
	    result = new (heap) BoolVal(ITM_RETURN_TRUE);
	  else
	  {
           //10-061019-9936 -Begin
           // For nullable columns, the LIKE predicate is transformed as shown
           // in case statement below.

           Parser parser(bindWA->currentCmpContext());
           ItemExpr * itmtrue = NULL, *itmnull = NULL;
           char buf[200];
           buf[0] = 0;
           itmnull = new(heap) BoolVal(ITM_RETURN_NULL);
           itmtrue = new(heap) BoolVal(ITM_RETURN_TRUE);
           strcpy(buf, "CASE WHEN @A1 is not null then @A2 ELSE @A3 END;");

           result = parser.getItemExprTree(buf,strlen(buf), BINDITEMEXPR_STMTCHARSET, 3, matchExpr, itmtrue,itmnull);

           //10-061019-9936 -End
	 }
	  boundExpr = result;

	  // Now bind the newly created expression, and return
	  boundExpr = boundExpr->bindNode(bindWA);
	  return boundExpr;
	}
      } // if (BytesInNonWildCardChars == 0)

      // Then check for cases - col like 'ab'. These will be converted to
      // equality predicates.

      // If it is a CHAR type and pattern_str_len contains the total number
      // of characters in the like pattern
      // Get number of non-wild-characters in the string. If they are equal, then
      // that implies there are no wild card characters.
      // Then transform the like predicate as follows:
      // Col LIKE 'ab' can be transformed to FALSE if Col is not nullable,
      // and (Col a CHAR(n) with n <> 2, or if Col is a VARCHAR(n) with n < 2)
      // if col is a CHAR type then it is transormed to an equality predicate

      if (pattern_str_len == BytesInNonWildCardChars)
      {
	// set a flag in the expression to indicate that the pattern is a
	// string literal (col LIKE 'ab'). This will be used later to set
	// the selectivity of LIKE predicate, if for some reason the LIKE
	// predicate could not be transformed.
	setPatternAStringLiteral();

	// take care of all cases if the column is VARCHAR type
	if (matchCharType->getTypeName() == "VARCHAR")
	{
	  if ( ( !matchCharType->supportsSQLnullLogical()  ) &&
	       (matchCharType->getNominalSize() < BytesInNonWildCardChars ) )
	  {
	    // if the col is VARCHAR type and the length of the col is
	    // less than the length of the pattern, return FALSE
	    ItemExpr * result = new (heap) BoolVal(ITM_RETURN_FALSE);
	    boundExpr = result;

	    // Now bind the newly created equality expression, and return
	    boundExpr = boundExpr->bindNode(bindWA);
	    return boundExpr;
	  }
	  else
	  {
	    // length of the column is equal to or greater than the  length
	    // of the pattern. Ideally we should have been able to convert the
	    // LIKE predicate to an equality predicate of exact length, but
	    // because of a bug in equality predicates on VARCHAR columns, we
	    // will set the default selectivity equal to 1/UEC. The bug is
	    // followed by Sol: 10-050412-6599. Once this problem is fixed
	    // the LIKE predicate should be transformed to
	    // col LIKE 'ab' -> col = 'ab' and char_length(col) = 2
	    // As of now we shall not do anything right now, and take care
	    // of it later while estimating cardinality.

	    return boundExpr;
	  }
	} // col LIKE 'ab' on a VARCHAR column
	else
	{
	  if ( !matchCharType->supportsSQLnullLogical()  &&
	       (matchCharType->getNominalSize() != BytesInNonWildCardChars ) )
	  {
	    // col is not nullable, is CHAR type but the length of the col
	    // is not same as the LIKE pattern length. Hence return FALSE
	    ItemExpr * result = new (heap) BoolVal(ITM_RETURN_FALSE);
	    boundExpr = result;

	    // Now bind the newly created equality expression, and return
	    boundExpr = boundExpr->bindNode(bindWA);
	    return boundExpr;

	  }
	  else
	  {
	    if (matchCharType->getNominalSize() == BytesInNonWildCardChars )
	    {
	      // col is the same length as the pattern, so transform the
	      // predicate into an equality predicate
	      // Left child of the expression would be matchExpr, which is the left
	      // child of Like pred, which is the column
	      // Right child of the equality expression will be the Like pattern
	      // which is a literal. Type of the literl is same as the type of
	      // the column it is being equated to.

	      ItemExpr * child1 = new (heap) SystemLiteral(
					     NAString(pattern_str, pattern_str_len),
					     matchCharType->getCharSet(),
					     matchCharType->getCollation(),
					     matchCharType->getCoercibility()
								);
	      if ((specialMode) && (matchCharType->isCaseinsensitive()))
		child1 = new (bindWA->wHeap()) Upper(child1);

	      BiRelat *eqExpr =
			new (heap) BiRelat(ITM_EQUAL,
					     matchExpr,
					     child1,
					     FALSE,	  // specialNulls flag
					     FALSE
					  );

	      eqExpr->setOriginalLikeExprId(getValueId() );

	      boundExpr = eqExpr;

	      // Now bind the newly created equality expression, and return
	      boundExpr = boundExpr->bindNode(bindWA);

	      return boundExpr;
	    } // matchCharType->getNominalSize() == BytesInNonWildCardChars
	  } // length of col <> pattern length, and col is NULLable
	} // col is CHAR (dataType.getTypeName() <> "VARCHAR")

	// for all other cases, return without any transformation. Apply default
	// selectivity later.
	return boundExpr;

      } // if (pattern_str_len == BytesInNonWildCardChars)
    } // end of all the transformations for special cases

    if (pattern.getType() == LikePatternStringIterator::NON_WILDCARD &&
	pattern.getClauseLength() > 0)
    {
      // If called by beginEndKeysApplied, return non-NULL pointer,
      // so beginEndKeysApplied() will return TRUE.
      if (!bindWA) return this;

      // Test if pattern ends with percent and has no underscores --
      // 'ab%', 'ab%%', 'ab%%%', ..., but not
      // 'ab', 'ab_', 'ab%yz', 'ab%_', 'ab%_yz', ... --
      // If so then we can optimize the LIKE away entirely.
      //
      if (pattern.getLength() == pattern.getClauseLength() &&
	  pattern.getNextHeader() &&
	  pattern.getNextHeader()->getType() == LikePatternStringIterator::
	  					PERCENT &&
	  !pattern.getNextHeader()->getNextHeader())
      {
	    CMPASSERT(!pattern.getNextClause() &&
		      !pattern.getNextHeader()->getNextClause());
	    CMPASSERT(pattern.getNextHeader()->getLength() ==
		      pattern.getNextHeader()->getClauseLength());
	// boundExpr==this, so do not delete it!
	boundExpr = NULL;
      }

      // Get the prefix, e.g. "ab", and the minimal key value, e.g. '\0'
      NAString prefix(pattern.getPattern(),
		      pattern.getClauseLength(),
		      heap);
      Lng32 zeroChar = matchCharType->getMinSingleCharacterValue();

      // Get the maximum length of the comparands.
      // Note that there's another silly case we don't bother optimizing
      // but just allow here (the if-test, after which we widen matchLen):
      //   m < p  (where m = mv length, p = pattern non-wild prefix length)
      // could be optimized to
      //   if mv is null return NULL else return FALSE
      //

      // matchLen must be number of BYTES, not SQL CHAR's
      size_t matchLen = matchCharType->getNominalSize();

      if (matchLen < prefix.length()) matchLen = prefix.length();

      // Zero-pad into prefixZ, e.g. for mv type VARCHAR(4), make "ab\0\0"
      char *prefixZ = new (heap) char[matchLen];
      byte_str_cpy(prefixZ, matchLen, prefix.data(), prefix.length(),
		   (char)zeroChar);

      ItemExpr *child1 = new (heap) SystemLiteral(
			      NAString(prefixZ, matchLen),
			      matchCharType->getCharSet(),
			      matchCharType->getCollation(),
			      matchCharType->getCoercibility()
						);
      if ((specialMode) && (matchCharType->isCaseinsensitive()))
	child1 = new (bindWA->wHeap()) Upper(child1);

      ItemExpr *beginKey =
		new (heap) BiRelat(ITM_GREATER_EQ, matchExpr,
				     child1,
				     FALSE,	  // specialNulls flag
				     FALSE // derivative from Like
				  );
      ((BiRelat*)beginKey)->setAddedForLikePred(TRUE);

      // Now set the selectivity that should be applied for this
      // predicate.

      // Selectivity from like would depend on the number of non_wildcard
      // characters in the Like string.
      // Selectivities are computed as follows:
      // ab% -> >= ab and < c. The two range predicates are applied the usual way
      // but the final selectivity is based on the default selectivity of like
      // predicates adjusted based on the number of non-wildcard characters. This
      // selectivity is applied to the first range predicate. Selectivity of
      // the second range predicate is set to 1.
      // a%b -> >= a and < b and like %b. The selectivities in this case are
      // computed and applied similar to the previous case. This implies, that
      // we shall compute the selectivity based on the number of non-wildcard
      // characters. Apply that to the first range predicate and would use
      // selectivity equal to 1 for the two remaining predicates (> b and like %b)

      double selectivity = computeSelForNonWildcardChars();

      // If user had specified selectivity for original LIKE predicate via selectivity
      // hint, then store that as LikeSelectivity on BiRelat predicate and unset the 
      // selecitivity hint on the original LIKE predicate.
      if(isSelectivitySetUsingHint())
      {
        selectivity = getSelectivityFactor();
        beginKey->setSelectivitySetUsingHint();
        beginKey->setSelectivityFactor(selectivity);
        setSelectivitySetUsingHint(FALSE);
        setSelectivityFactor(-1);
      }

      BiRelat *br = (BiRelat *) beginKey;
      br->setLikeSelectivity(selectivity);
      // Like pred has non_wildcard beginning and ends in %
      // Later we will collapse histogram into one interval if this flag is set.
      // In this simple case, it is better to not flag this BiRelat as 
      // originating from LIKE so that we get a better histogram on it. 
      // We may lose some knowledge of correlation between begin/end keys 
      // but it is better to have 2 unrelated birelats with good stats than 
      // correlated begin/end preds with a single interval histogram. JIRA 2512
      if(boundExpr)
        br->setOriginalLikeExprId(getValueId());

      // Compute the value following the beginKey prefix:
      // If beginKey == 'ab', this will return 'ac';
      // if 'a\377', then 'b'; if '\377\377', then '' and FALSE;
      // if can't compute next key (due to multibyte chars or nondefault
      // collating sequence), then it returns '' and FALSE.
      // If we get a TRUE return, then build endKey predicate.
      //

      NABoolean foundNextKey = FALSE;

      if ( matchCharType->getCharSet() == CharInfo::ISO88591 )
      {
	  foundNextKey = matchCharType->computeNextKeyValue(prefix);
	  byte_str_cpy(prefixZ, matchLen, prefix.data(), prefix.length(),
		       (char)zeroChar);
      }
      else if ( matchCharType->getCharSet() == CharInfo::UCS2 )
      {
	  NAWString prefixW((NAWchar*)pattern.getPattern(),
			    pattern.getClauseLength()>>1
			   );
	  foundNextKey = matchCharType->computeNextKeyValue(prefixW);
	  byte_str_cpy(prefixZ, matchLen,
		       (char*)prefixW.data(), prefixW.length()<<1,
		       (char)zeroChar
		      );
      }
      else if ( matchCharType->getCharSet() == CharInfo::UTF8 )
      {
	  foundNextKey = matchCharType->computeNextKeyValue_UTF8(prefix);
	  byte_str_cpy(prefixZ, matchLen, prefix.data(), prefix.length(),
		       (char)zeroChar);
      }

      if ( foundNextKey )
      {
	ItemExpr *child1 = new (heap) SystemLiteral(
				NAString(prefixZ, matchLen),
				matchCharType->getCharSet(),
				matchCharType->getCollation(),
				matchCharType->getCoercibility()
						  );
	if ((specialMode) && (matchCharType->isCaseinsensitive()))
	  child1 = new (bindWA->wHeap()) Upper(child1);

	ItemExpr *endKey =
	    new (heap) BiRelat(ITM_LESS, matchExpr,
				     child1,
				     FALSE,	  // specialNulls flag
				     FALSE	  // partKeyPred flag
			      );

	BiRelat *br = (BiRelat *) endKey;
	br->setAddedForLikePred(TRUE);

	// set selectivity of the second range predicate equal to 1.0
	br->setLikeSelectivity(1.0);
        if(boundExpr)
          br->setOriginalLikeExprId(getValueId());


	if (boundExpr)
	  boundExpr = new (heap) BiLogic(ITM_AND, boundExpr, endKey);
	else
	  boundExpr = endKey;
      } // foundNextKey

      if (boundExpr)
	boundExpr = new (heap) BiLogic(ITM_AND, beginKey, boundExpr);
      else
	boundExpr = beginKey;

      CMPASSERT(bindWA);
      boundExpr = boundExpr->bindNode(bindWA);

      NADELETEBASIC(prefixZ, heap);
      
      beginEndKeysApplied_ = TRUE;

    } // pattern has a non-wild prefix
    else
    {
      // pattern has a wild card prefix. At this point see if the user
      // has used the old CQD HIST_DEFAULT_SEL_FOR_LIKE_WILDCARD. If he has
      // then set a flag here to indicate that the optimizer should use the
      // old CQD as the default selectivity for '%ab type cases. This is to
      // maintain upward compatibility for the compiler

      if (bindWA)
      {
	// check the CQD list set by the user to see if HIST_DEFAULT_SEL_FOR_LIKE_WILDCARD
	// has been set

        ControlDB *cdb = ActiveControlDB();

	for (CollIndex i = 0; i < cdb->getCQDList().entries(); i++)
	{
	   ControlQueryDefault *cqd = cdb->getCQDList()[i];
	   if (cqd->getAttrEnum() == HIST_DEFAULT_SEL_FOR_LIKE_WILDCARD)
	   {
	     oldDefaultSelForLikeWildCardUsed_ = TRUE;
	     break;
	   }
	} // done checking for HIST_DEFAULT_SEL_FOR_LIKE_WILDCARD
      }
    }
  } // optimizeLike: pattern && escape are non-null constants

  return boundExpr;
} // Like::applyBeginEndKeys()

// -----------------------------------------------------------------------
// member functions for class Case
// -----------------------------------------------------------------------

ItemExpr *Case::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();

  //
  // Check whether the CASE statement is in the following form:
  //
  // CASE val0 WHEN val1 THEN result1 WHEN val2 THEN result2 ...
  //
  // If so, convert it to the following form:
  //
  // CASE WHEN val0 = val1 THEN result1 WHEN val0 = val2 THEN result2 ...
  //
  ItemExpr *caseOperand = removeCaseOperand();
  if (caseOperand) {
    caseOperand = caseOperand->bindNode(bindWA);
    if (bindWA->errStatus())
      return this;
    ItemExpr *ifThenElse = child(0);
    CMPASSERT(ifThenElse->getOperatorType() == ITM_IF_THEN_ELSE);
    //
    // The ELSE clause may be a NULL pointer if this is part of a CASE
    // statement created by the generator.
    //
    do {
      ifThenElse->child(0) = new (bindWA->wHeap())
	BiRelat(ITM_EQUAL, caseOperand,	ifThenElse->child(0));
      ifThenElse = ifThenElse->child(2);
    } while (ifThenElse AND ifThenElse->getOperatorType() == ITM_IF_THEN_ELSE);
  }

  if (CmpCommon::getDefault(ALLOW_INCOMPATIBLE_OPERATIONS) == DF_ON)
    {
      // if operands are incompatible, insert cast node to convert them
      // to a common datatype.
      // Right now, only done for CHAR and NUMERICs.
      ItemExpr *ifThenElse = child(0);
      ItemExpr * thenClause = ifThenElse->child(1)->castToItemExpr();
      NABoolean done = FALSE;
      NABoolean charFound = FALSE;
      NABoolean numericFound = FALSE;
      Lng32 dLen = 0;
      Lng32 thenClauseNum = 1;
      while ((ifThenElse) && (NOT done))
        {
          thenClause = thenClause->bindNode(bindWA);
          if (bindWA->errStatus())
            return this;
      
          ifThenElse->setChild(thenClauseNum, thenClause);
      
          if ((thenClause->getOperatorType() == ITM_CONSTANT) &&
              ((ConstValue *)thenClause)->isNull())
            {
              // do nothing
            }
          else if (DFS2REC::isCharacterString(thenClause->getValueId().getType().getFSDatatype()))
            {
              if (thenClause->getValueId().getType().getNominalSize() > dLen)
                dLen = thenClause->getValueId().getType().getNominalSize();
              charFound = TRUE;
            }
          else if (thenClause->getValueId().getType().getTypeQualifier() 
                   == NA_NUMERIC_TYPE)
            {
              NumericType &numeric = (NumericType&)
                thenClause->getValueId().getType();
              Lng32 numericDLen =
                numeric.getDisplayLength(numeric.getFSDatatype(),
                                         numeric.getNominalSize(),
                                         numeric.getPrecision(),
                                         numeric.getScale(),
                                         0);
              
              if (numericDLen > dLen)
                dLen = numericDLen;
              numericFound = TRUE;
            }
          else
            {
              done = TRUE;
            }
      
          if (thenClauseNum == 2)
            ifThenElse = NULL;
          else
            {
              if (ifThenElse->child(2)->getOperatorType() == ITM_IF_THEN_ELSE)
                {
                  ifThenElse = ifThenElse->child(2);
                  thenClause = ifThenElse->child(1);
                  thenClauseNum = 1;
                }
              else
                {
                  // this is the else clause
                  thenClause = ifThenElse->child(2);
                  thenClauseNum = 2;
                }
            }
        } // while
  
      if ((NOT done) && (charFound) && (numericFound))
        {
          ifThenElse = child(0);
          thenClause = ifThenElse->child(1)->castToItemExpr();
          thenClauseNum = 1;
          while (ifThenElse)
            {
              if (thenClause->getValueId().getType().getTypeQualifier() 
                  == NA_NUMERIC_TYPE)
                {
                  // cast to character
                  thenClause =
                    new (bindWA->wHeap())
                    Cast(thenClause,
                         new (bindWA->wHeap())
                         SQLChar(bindWA->wHeap(), dLen,
                                 thenClause->
                                 getValueId().getType().supportsSQLnull()));
                  
                  thenClause = thenClause->bindNode(bindWA);
                  if (bindWA->errStatus())
                    return this;
                  
                  ifThenElse->setChild(thenClauseNum, thenClause);
                }
              
              if (thenClauseNum == 2)
                ifThenElse = NULL;
              else
                {
                  if (ifThenElse->child(2)->getOperatorType() == ITM_IF_THEN_ELSE)
                    {
                      ifThenElse = ifThenElse->child(2);
                      thenClause = ifThenElse->child(1);
                      thenClauseNum = 1;
                    }
                  else
                    {
                      // this is the else clause
                      thenClause = ifThenElse->child(2);
                      thenClauseNum = 2;
                    }
                }
            } // while
        }
    } // allow incompatible operations
  
  // Case inherits from BuiltinFunction .. Function .. ItemExpr.
  ItemExpr *boundExpr = BuiltinFunction::bindNode(bindWA);

  // Fix for "BR0094.txt", here and in ItemExpr::synthTypeAndValueId() --
  // If we are "CASE(select..from..) WHEN..ELSE..",
  // make sure our result type is NULLABLE, as the subq may produce zero rows.
  if (caseOperand && caseOperand->isASubquery()) {
    ValueId vid = boundExpr->getValueId();
    const NAType* nullableType =
      vid.getType().synthesizeNullableType(bindWA->wHeap());
    vid.changeType(nullableType);
    CMPASSERT(boundExpr->getOperatorType() == ITM_CASE);
    ((Case *)boundExpr)->caseOperandWasNullable() = TRUE;
  }

  return boundExpr;
} // Case::bindNode()

// -----------------------------------------------------------------------
// member functions for class ColReference
//
// (but first, a BindWA method called by ColReference::bindNode below,
// and by Natural Join binding code in BindRelExpr.C)
// -----------------------------------------------------------------------

// **FOR DML:**
//
// Mark column as of interest to the Optimizer (particularly ScanOptimizer.C).
// Optimizer will expect:
//
// - Full stats (a ColStats header and as many HistInts as there are)
// from each referenced column's *single-column* histogram, and
// will want full stats from any *multi-column* histogram
// that contains a referenced column (for MDAM).
//
// - Of columns that are not referenced in this query, those that belong to
// an index (any one) must have short stats (a ColStats header, no intervals)
// from their *single-column* histogram.  As there will always be at least
// one key column in a table (SYSKEY at the least), each table will end up
// having at least one ColStat, even if no refd cols ("SELECT c FROM t;"),
// which is another Optimizer assumption.
//
// - All other columns -- those not deemed referenced by the criteria below
// which also are not index keys -- the Optimizer has no use for any stats.
//
// The FetchHistograms function (in /ustat) uses this
// is-referenced flag, along with the columns' is-indexkey flags,
// and applies the rules above to deliver the minimum required stats.
//
// **FOR DDL -- actually only for CREATE VIEW:**
//
// On the first reference to a column, anywhere in the query per se
// (i.e. excluding constraints), add the column reference to the
// view-basetablecolumn list needed for one of the Ansi metadata tables.
// On any reference to a column within the top select-list of the view query,
// add to the viewcolumn-basetablecolumn list needed by CatMan to enforce
// the complicated REFERENCES privilege.
//

/*
Update: 09/14/2009

Columns are marked �REFERENCED_FOR_MULTI_INTERVAL_HISTOGRAM� which require full histograms. 
Full histogram means detailed histogram data which includes all the histogram interval data. 
For columns to be marked under this category, they should be part of one of the following groups:

- Key columns
- Where Clause
- Join predicate

Columns are marked "REFERENCED_FOR_SINGLE_INTERVAL_HISTOGRAM" which require only single 
interval histograms. For columns to be marked under this category, they should be part 
of one of the following groups:

- Union clause
- GroupBy Clause
- Having Clause

Columns are marked "REFERENCED_ANYWHERE" which do not fall into either of the above categories.
*/

void BindWA::markAsReferencedColumn(const ColumnDesc *cd, 
                                    NABoolean groupByRefForSingleIntHist)
{ 
  if (cd->getViewFileName())
  {
    setColumnRefsInStoi(cd->getViewFileName(),cd->getViewColPosition());
  }

  markAsReferencedColumn(cd->getValueId(), groupByRefForSingleIntHist); 
}

void BindWA::markAsReferencedColumn(const ValueId &vid,
                                    NABoolean markGroupByForSingleInt)
{
  BindContext *context = getCurrentScope()->context();

  // Pay attention only to the query per se, not to extra bits brought in from
  // the metadata.
  if (context->inAnyConstraint()) return;

  // If ColReference refers to a union of colrefs, or to an aggregate function,
  // or an instantiate-null, or whatever, no need to do anything --
  // the underlying colrefs will already have been marked by earlier binding.
  //
  NAColumn *nacol = vid.getNAColumn(TRUE/*okIfNotColumn*/);

   if (!nacol) 
  {
	if (vid.getItemExpr()->getOperatorType() == ITM_VALUEIDUNION) 
	{
	  ValueIdUnion *valIdUnion = (ValueIdUnion *)vid.getItemExpr();

	  NABoolean groupByRefForSingleIntHist = FALSE;

	  // If the GroupBy is due to UNION DISTINCT, do not mark grouping columns
	  // as referenced for histogram
	  if (valIdUnion->isTrueUnion() )
		groupByRefForSingleIntHist = TRUE;

	  for (CollIndex i = 0; i < valIdUnion->getSources().entries(); i++) 
	  {
		markAsReferencedColumn(valIdUnion->getSources()[i], groupByRefForSingleIntHist);
	  }
	}
	else
	{
	  ValueIdSet leafValues;

	  ItemExpr *tempPred = vid.getItemExpr();
	  if(!tempPred)
	    return;
	  
	  tempPred->findAll(ITM_BASECOLUMN, leafValues, TRUE, TRUE);

	  for ( ValueId id = leafValues.init();
		leafValues.next( id );
		leafValues.advance( id ) )
	  {
	      markAsReferencedColumn(id);
	  }
	}
	return;
  }

  const NATable * naTable = nacol->getNATable();

  if ( !naTable->isHiveTable() ) {
    NAString fileName( naTable->getViewText() ?
                    (NAString)naTable->getViewFileName() :
                    naTable->getClusteringIndex()->
                        getFileSetName().getQualifiedNameAsString(),
                    wHeap());

    setColumnRefsInStoi(fileName.data(),nacol->getPosition());
  }

  if (inDDL()||context->inOrderBy()) return;

  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) )
  {
	if ( (nacol->isPartitioningKey() ) &&
		   (nacol->isUserColumn() ) &&
		   (nacol->getNATable()->getSpecialType() == ExtendedQualName::NORMAL_TABLE) )
	   nacol->setReferencedForMultiIntHist();
  }

  // column references that are in a prdicate - WHERE, HAVING,
  // column references that are in a GROUP BY. This does not include 
  // GroupBy created implicitly because of UNION DISTINCT
  // common columns in a NATURAL join,
  // column references that are in a join predicate
  // are marked as referenced for histogram

  BindScope *scope = getCurrentScope();

    while (scope) {
    BindContext *context = scope->context();
    if (context->inWhereClause()  ||
        context->inHavingClause() ||
        context->inJoinPred() ||
        // If the join has not been fully bound, the joinPred would not have
        // been set and the predicates would still exist as joinPredTree
        (context->inJoin() && context->inJoin()->getJoinPredTree()) ||
        context->inGroupByClause() ||
        context->inUnion())
     {
      //if column participates in a join pred mark it, since this
      //information is later used for reducing the number of histogram
      //intervals
      if (context->inJoinPred() || (context->inJoin() && context->inJoin()->getJoinPredTree()))
      	nacol->setHasJoinPred();

      //if column participates in a range pred mark it, since this
      //information is later used for reducing the number of histogram
      //intervals
      if (context->inRangePred())
    	nacol->setHasRangePred();
      
      // if it has already been marked referenced for histogram, we will not
      // reduce its scope, hence return
      // isReferencedForHistogram is set to TRUE if histogram is marked for either single interval
      // or full interval. If the histogram is marked for single interval, but another context
      // require it to be full histogram, we upgrade
      if (nacol->isReferencedForMultiIntHist())
        return;
      
      // the column is referenced in a predicate, used to determine
      // whether histograms should be fetched for this column reference
      if(markGroupByForSingleInt ||
         context->inGroupByClause() || 
         context->inHavingClause()  ||
         context->inUnion())
	nacol->setReferencedForSingleIntHist();
      else
	nacol->setReferencedForMultiIntHist();

      return;
    }
    scope = getPreviousScope(scope);
  }

  // column is referenced anywhere in a query, used by unpack
  nacol->setReferenced();

} // BindWA::markAsReferencedColumn()

ItemExpr *ColReference::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) 
    {
      if (getColRefNameObj().isStar())
        return this;

      BindScope *bindScope;
      ColumnNameMap *xcnmEntry = bindWA->findColumn(getColRefNameObj(), bindScope);
      if (bindScope != bindWA->getCurrentScope() &&
          (bindWA->getCurrentScope()->context()->inOlapOrderBy() || 
           bindWA->getCurrentScope()->context()->inOlapPartitionBy()))
        {
          *CmpCommon::diags() << DgSqlCode(-4391);
          bindWA->setErrStatus();
          return this;
        }    
      
      // this return has been there for a long time.
      // No idea what the code below it is doing since it will never be reached.
      return getValueId().getItemExpr();
      
      // In case the first time this Colreference was seen it was on
      // left side of a set clause
      NAColumn *nacol = getValueId().getNAColumn(TRUE/*okIfNotColumn*/);
      const NATable * naTable = nacol->getNATable();
      NAString fileName( naTable->getViewText() ?
                         (NAString)naTable->getViewFileName() :
                         naTable->getClusteringIndex()->
                         getFileSetName().getQualifiedNameAsString(),
                         bindWA->wHeap());
      
      bindWA->setColumnRefsInStoi(fileName.data(),nacol->getPosition());
      
    }
  
  // In mode_special_4,
  // if name is of the form:   IDENTIFIER.NEXTVAL or IDENTIFIER.CURRVAL,
  // then change it to:  seqnum(identifier, next) or seqnum(identifier, current)
  // If name is: ROWNUM, change it to ROWNUM() function.
  if (CmpCommon::getDefault(MODE_SPECIAL_4) == DF_ON) 
    {
      ColRefName &colRefName = getColRefNameObj();
      CorrName &cn = getCorrNameObj();
      
      const NAString &catName = cn.getQualifiedNameObj().getCatalogName();	       
      const NAString &schName = cn.getQualifiedNameObj().getSchemaName();		
      const NAString &objName = cn.getQualifiedNameObj().getObjectName();		
      const NAString &colName = colRefName.getColName();

      if (((catName.isNull()) &&
          (schName.isNull()) &&
          ((colName == "NEXTVAL") ||
           (colName == "CURRVAL"))) ||
          (((catName.isNull()) &&
            (schName.isNull()) &&
            (objName.isNull())) &&
           (colName == "ROWNUM")))
        {
          ItemExpr * itemExpr = NULL;

          if (colName == "ROWNUM")
            {
              itemExpr = new(bindWA->wHeap()) RowNumFunc();
            }
          else
            {
              CorrName seqName(objName);
              seqName.setSpecialType(ExtendedQualName::SG_TABLE);
              
              itemExpr = 
                new(bindWA->wHeap()) SequenceValue(seqName, 
                                                   (colName == "NEXTVAL" ? FALSE : TRUE),
                                                   (colName == "NEXTVAL" ? TRUE : FALSE));
            }

          itemExpr = itemExpr->bindNode(bindWA);
          if (bindWA->errStatus()) 
            return this;
          ValueId valId = itemExpr->getValueId();
          setValueId(valId);

          bindSelf(bindWA);
          return itemExpr;
        }
    }

  // override schema
  if ( ( bindWA->overrideSchemaEnabled() ) 
       // do not override if no column name
    && ( ! getColRefNameObj().getColName().isNull() )  
    // do not override if in a constraint (required override should have been done)
    && ( ! bindWA->getCurrentScope()->context()->inCheckConstraint() )) {
       bindWA->doOverrideSchema(getCorrNameObj());
    }

  // fix 0-061115-0532 (query cache didn't handle select with embedded
  // update correctly). New/Old corr. names are recorded here in bindWA.
  // 
  NABoolean hasSeenNewOrOldName = FALSE;
  if ( getCorrNameObj().getQualifiedNameObj().getObjectName() == "NEW" ) {
    bindWA->appendCorrNameToken('N');
    hasSeenNewOrOldName = TRUE;
  } else {
    if ( getCorrNameObj().getQualifiedNameObj().getObjectName() == "OLD" ) {
      bindWA->appendCorrNameToken('O'); 
      hasSeenNewOrOldName = TRUE;
    }
  }

  // See if UDF_SUBQ_IN_AGGS_AND_GBYS is enabled. It is enabled if the
  // default is ON, or if the default is SYSTEM and ALLOW_UDF is ON.
  NABoolean udfSubqInAggGrby_Enabled = FALSE;
  DefaultToken udfSubqTok = CmpCommon::getDefault(UDF_SUBQ_IN_AGGS_AND_GBYS);
  if ((udfSubqTok == DF_ON) ||
      (udfSubqTok == DF_SYSTEM))
    udfSubqInAggGrby_Enabled = TRUE;
  
  BindScope *currScope = bindWA->getCurrentScope();
  if (getColRefNameObj().isStar()) {			// "*" or "CORR.*"

    if ( hasSeenNewOrOldName == TRUE )
      bindWA->appendCorrNameToken('*'); 

    RETDesc* resultTable = currScope->getRETDesc();
    CorrName corrName = getCorrNameObj();
    const ColumnDescList *colList;

    if (resultTable == NULL) { // for example values(sas_score("HHH",t.*));
      NAString nam("*", bindWA->wHeap());
      NAString fmtdList(bindWA->wHeap());
      LIST(TableNameMap*) xtnmList(bindWA->wHeap());
      bindWA->getTablesInScope(xtnmList, &fmtdList);	// Tables in all scopes
      if(fmtdList.isNull())
        fmtdList = "NONE";

      *CmpCommon::diags() << DgSqlCode(-4002)
        << DgColumnName(nam)
        << DgTableName(getCorrNameObj().getExposedNameAsAnsiString())
        << DgString0(fmtdList)
        << DgString1(bindWA->getDefaultSchema().getSchemaNameAsAnsiString());
      bindWA->setErrStatus();
      return this;
    }
    if (collateClause()) {
      // 4034 The operation (T.* COLLATE coll-name) is not allowed.
      NAString nam("*", bindWA->wHeap());
      if (corrName != "")
        nam.prepend(corrName.getExposedNameAsAnsiString() + ".");
      *CmpCommon::diags() << DgSqlCode(-4034)
        << DgString0(nam)
	<< DgString1("COLLATE")
	<< DgString2(CharInfo::getCollationName(collateClause()->collation_));
      bindWA->setErrStatus();
      return this;
    }
    if (corrName == "")
    {
      colList = resultTable->getColumnList();
      if(CmpCommon::getDefault(DISPLAY_DIVISION_BY_COLUMNS) == DF_ON)
      {
    	ColumnDescList *divColList = new(bindWA->wHeap())
    			  ColumnDescList(bindWA->wHeap());

    	for (CollIndex i=0; i<colList->entries(); i++)
    		  divColList->insert(colList->at(i));

    	const ColumnDescList *sysColList = resultTable->getSystemColumnList();
    	for (CollIndex i=0; i<sysColList->entries(); i++)
    	{
    		ColumnDesc *colDesc = sysColList->at(i);
    		NAColumn *nacol = colDesc->getValueId().getNAColumn(TRUE);
    		if(nacol->isDivisioningColumn())
    			divColList->insert(colDesc);
    	}
    	colList = divColList;
      }
    }
    else {
      colList = resultTable->getQualColumnList(corrName);

      if (!colList || !colList->entries()) {
	// 4010 There are no user columns with the qualifier.
	*CmpCommon::diags() << DgSqlCode(-4010)
			  << DgTableName(corrName.getExposedNameAsAnsiString());
	bindWA->setErrStatus();
	return this;
      }
    }

    // -- MVs
    // Remove from the list columns that are system added.
    CMPASSERT(resultTable!=NULL &&  colList!=NULL);
    if (!getColRefNameObj().getStarWithSystemAddedCols())
    {
      ColumnDescList *minimalColList = new(bindWA->wHeap())
	ColumnDescList(bindWA->wHeap());
      for (CollIndex i=0; i<colList->entries(); i++)
      {
	ColumnDesc *colDesc = colList->at(i);
	NAColumn *col = colDesc->getValueId().getNAColumn(TRUE);
	if (col==NULL || !col->isMvSystemAddedColumn())
	    minimalColList->insert(colDesc);
      }
      colList = minimalColList;
    }

    //
    // If this is in a GROUP BY clause, mark each column as a grouping column.
    // (Our extension to ANSI)
    //
    if (currScope->context()->inGroupByClause() && 
        (!udfSubqInAggGrby_Enabled ||
         !currScope->context()->inUDFunction())) {
      for (CollIndex i = 0; i < colList->entries(); i++)
      {
        (*colList)[i]->setGroupedFlag();
        if ((*colList)[i]->getValueId().getItemExpr()
                             ->containsOpType(ITM_RANDOMNUM))
        {
          // Temporary fix till random is supported in ORDER BY, GROUP BY
          // For now do not allow random in ORDER BY clause, GROUP BY
          // and DISTINCT.
          *CmpCommon::diags() << DgSqlCode(-4313);
          bindWA->setErrStatus();
          return this;
        }
      }
    }

    ColumnDescList *collapseStar = NULL;
    if (currScope->context()->inSelectList()) {
	  CMPASSERT( (corrName != "") || // -- Triggers
		         (bindWA->getPreviousScope(currScope) != NULL));
      if (corrName == "" &&
	  bindWA->getPreviousScope(currScope)->	    // simply contained
		     context()->inExistsPredicate()) {
	//
	// ANSI 7.9 SR 3a applies to "*" (simply contained only!), not "T.*":
	// - "exists (select * from (select a,a from t) x)" is equivalent to
	//   "exists (select 1 from (select a,a from t) x)" (arbitrary literal).
	// Note that since
	// - "exists (select * from (...group by a,e having...) x)" is equiv to
	//   "exists (select 1 from (...group by a,e having...) x)",
	// we do not require that columns b,c,d be grouping columns.
	//
	// So to bind this case ("*" simply contained in "exists"),
	// we just expand the * into the first column in the list,
	// not checking for duplicate/ambiguous column references.
	// This collapsing to a degree-one (scalar) result makes
	// BindRelExpr.C/bindRowValues() happy.
	// Later on, Subquery::transformNode will remove the unnecessary
	// selected column from the characteristic output.
	//
	collapseStar = new (bindWA->wHeap()) ColumnDescList(bindWA->wHeap());
	collapseStar->insert((*colList)[0]);
	colList = collapseStar;
      } else {		// not bare "*" simply contained by an Exists pred
	//
	// ANSI 7.9 SR 3b + 4:  replace "*" and "T.*" with a sequence of
	// column references; referenced columns cannot be ambiguous (6.4 SR 4).
	// Unnamed columns should actually have unique implementation-dependent
	// names so must not be considered ambiguous.
	//
	for (CollIndex i = 0; i < colList->entries(); i++) {
	  ColumnDesc *columnDesc = (*colList)[i];
	  if (NOT columnDesc->getColRefNameObj().isEmpty()) {	// named column
	    ColumnNameMap *xcnmEntry = bindWA->findColumn(*columnDesc);
	    if (xcnmEntry->isDuplicate()) {
              // 4011: Ambiguous star column reference.
              *CmpCommon::diags() << DgSqlCode(-4011)
		 << DgColumnName(columnDesc->getColRefNameObj().
		 		             getColRefAsAnsiString());
	      bindWA->setErrStatus();
	      return this;
	    }
	  }	// named column
	}	// for-loop
	//
	// If the table is a grouped table, each column in the select list
	// must be a grouping column.
	//
	if (resultTable->isGrouped()) {
	  for (CollIndex i = 0; i < colList->entries(); i++) {
	    ColumnDesc *columnDesc = (*colList)[i];
	    if (NOT columnDesc->isGrouped()) {
              // 4012: col must be grouping col; on this tbl star ref is illegal
              *CmpCommon::diags() << DgSqlCode(-4012)
		 << DgColumnName(columnDesc->getColRefNameObj().
					     getColRefAsAnsiString());
	      bindWA->setErrStatus();
	      return this;
	    }
	  }
	}		// isGrouped
      }			// not bare "*" simply contained by an Exists pred
    }			// inSelectList
    //
    setStarExpansion(colList);
    // The collapseStar case is not a real column ref, so skip these two things:
    if (!collapseStar) {
      BindUtil_UpdateNameLocForStarExpansion(bindWA, *colList,
					     getColRefNameObj().getNamePosition(),
                                             getParent());
      for (CollIndex i = 0; i < colList->entries(); i++)
	bindWA->markAsReferencedColumn((*colList)[i]);
    }

    bindSelf(bindWA);
    return this;
  } 		// ColReference::bindNode -- reference to "*" or "CORR.*"

  Lng32 sqlCode = 0;
  BindScope *bindScope;
  ColumnNameMap *xcnmEntry = bindWA->findColumn(getColRefNameObj(), bindScope);

  // When nametype is SHORTANSI in RETDesc::addColumnDesc() columns
  // are fully qualified before being inserted into into xcnm_.insert().
  // For a statement  like :
  //            Select sys_vol_subvol.table.column
  //            from sys_vol_subvol.table;
  // or
  //            set schema 'sys_vol_subvol';
  //            select table.column
  //            from table;
  // xcnm_.insert() inserts \sys.vol.subvol.table.column.
  // But in the above bindWA->findColumn() it is still looking for
  // sys_vol_subvol.table.column, so in the below changes it looks for
  // \sys.vol.subvol.table.column and finds it successfully.
  // Fix for CR-10-000719-1267.




  if ((xcnmEntry == NULL) &&
      (NOT getColRefNameObj().getCorrNameObj().getQualifiedNameObj().getObjectName().isNull()) &&
      (CmpCommon::context()->sqlSession()->volatileSchemaInUse()))
    {
      CorrName newCorrName = 
	CmpCommon::context()->sqlSession()->getVolatileCorrName
	(getColRefNameObj().getCorrNameObj());

      newCorrName.applyDefaults(bindWA, bindWA->getDefaultSchema());
      if (bindWA->errStatus())
	return NULL;		
      
      ColRefName *cstColRefName = NULL;
      cstColRefName = new(bindWA->wHeap())
		      ColRefName(getColRefNameObj().getColName(),
				 newCorrName, bindWA->wHeap());

      xcnmEntry = bindWA->findColumn(*cstColRefName, bindScope);
      if (xcnmEntry)
	getColRefNameObj().getCorrNameObj() = newCorrName;
    }

  NAString colRefStr(  xcnmEntry ?
  		       xcnmEntry->getColRefNameObj().getColRefAsAnsiString()
                       :
		       getColRefNameObj().getColRefAsAnsiString(),
                       bindWA->wHeap()) ;

  // VO, Genesis solution 10-040107-2237:
  //     If the column WAS specified as delimited, but
  //     looks like a regular identifier, then add the quotes
  if ( getColRefNameObj().isDelimited()      &&     // colRef WAS   "FOO"
       colRefStr[(StringPos)0] != '"'               // colRef IS    FOO
     )
    colRefStr = NAString('"') + colRefStr + NAString('"');

  if ( xcnmEntry == NULL ||
       xcnmEntry->isQualifiedColumnAmbiguous() ||

       ( xcnmEntry->isDuplicate() &&
	 NOT getColRefNameObj().isQualified() )

     )
  {
    NAString fmtdList(bindWA->wHeap());
    LIST(TableNameMap*) xtnmList(bindWA->wHeap());

    if (xcnmEntry == NULL) {
      if (getCorrNameObj() == "")
	sqlCode = -4001;  // col not found.
      else if (!bindWA->findCorrName(getCorrNameObj(), bindScope))
	sqlCode = -4002;  // corr.col not found. table "corr" not exposed.
      else
	sqlCode = -4003;  // corr.col not a col of specified table "corr".
      bindWA->getTablesInScope(xtnmList, &fmtdList);	// Tables in all scopes
      //10-031030-0943 -begin
      //If the fmtdList is empty then dont give a blank
      //string  fill it with "NONE" so that the error message is meaningful
      if(fmtdList.isNull())
        fmtdList = "NONE";
      //10-031030-0943 -end
    }
    else {
      sqlCode = -4004;    // col is ambiguous.
      bindScope->getTablesInScope(xtnmList, &fmtdList);	// Tables in ambig scope
    }

    // Genesis case 10-971208-5113
    // Tandem extension allows an ORDER BY column to be absent from the SELECT
    // list except when aggregation or GROUP BY are involved. Of course, those
    // columns must be in the tables exposed.
    //
    // If we are getting -4001,-4002 or -4003 when binding an ORDER BY column,
    // a couple of scenarios are possible:
    //
    // 1. There is no aggregation or GROUP BY in the query, and the column is
    //    really not in tables exposed (in this case, xtnmList.entries() > 0).
    // 2. There is aggregation or GROUP BY in the query, and the column is not
    //    found in the SELECT-list.
    //
    if ( (sqlCode == -4001 || sqlCode == -4002 || sqlCode == -4003) &&
          currScope->context()->inOrderBy() &&
          currScope->getRETDesc()->isGrouped() )
    {
      sqlCode = (!xtnmList.entries()) ? -4120 : -4121;
      *CmpCommon::diags() << DgSqlCode(sqlCode)
			  << DgColumnName(colRefStr)
			  << DgString0(fmtdList);
    }
    // genesis case 10-031030-7250:"NE:R2 MX1013 INSERT/SELECT not able to
    // handle ORDER BY with all columns". We want to reject insert-selects
    // of the form "insert into t(a) select a from s order by b" because
    // they can cause lots of block splits. Ideally, we want the source to
    // be in the same clustering key sequence as the target.
    else if ((sqlCode == -4001 || sqlCode == -4002 || sqlCode == -4003) &&
             currScope->context()->inOrderBy() &&
             currScope->context()->inInsert()) {
      *CmpCommon::diags() << DgSqlCode(-4135) << DgColumnName(colRefStr);
    }
    else
    {
    *CmpCommon::diags() << DgSqlCode(sqlCode)
      << DgColumnName(colRefStr)
      << DgTableName(getCorrNameObj().getExposedNameAsAnsiString())
      << DgString0(fmtdList)
      << DgString1(bindWA->getDefaultSchema().getSchemaNameAsAnsiString());

    // Genesis 10-970902-0878:
    // user typed "foo" when they meant 'foo',
    // or "FOO" when they meant 'FOO'.
    //
    if (getCorrNameObj() == "")		     // sqlCode could be -4001 or -4004
      if (colRefStr[(StringPos)0] == '"')
      {
        NAString literalStr(colRefStr, bindWA->wHeap());
	literalStr[(StringPos)0] = '\'';
	literalStr[literalStr.length()-1] = '\'';	// literalStr is 'foo'
	*CmpCommon::diags() << DgSqlCode(4104)
                            << DgColumnName(colRefStr)
                            << DgString0(literalStr);
      }
    } // endif (sqlCode == -4001 ... && currScope->context()->inOrderBy())

  } // xcnmEntry error (col not found, or duplicate/ambiguous)

  // Genesis 10-970929-8459:
  //   'SELECT * FROM ta JOIN tb ON a=b,c;'
  //   is perfectly legal unambiguous Ansi, but ambiguous for SQL/MX
  //	 because our Tandem-extension allowing Sql-row-value-constructor
  //   does not require parens around a value list;
  //   thus 'c' in example above is parsed as a column ref
  //   but user may well have (here, they did!) intended it as a table ref.
  //
  // Hence here we emit 4101 in addition to the preceding errmsg.
  //   So in this case we misinterpret legal Ansi syntax, emitting an error.
  //	 That's unfortunate, but fixing SqlParser productions for search_cond
  //   is prohibitive at this time.  Also note that
  //   'SELECT * FROM ta JOIN tb ON a,a2=b,b2,c;'
  //   is not legal Ansi, but likewise ambiguous for our parser.
  //	 'SELECT * FROM ta JOIN tb ON (a,a2=b,b2),c;' -- legal-Ansi, unambig-Tdm
  //	 'SELECT * FROM ta JOIN tb ON a,a2=(b,b2),c;' -- legal-Ansi, unambig-Tdm
  //	 'SELECT * FROM c, ta JOIN tb ON a,a2=b,b2;'  -- unambig-Tdm
  //   (The last has the side-effect of reordering the output *-list.)
  //
  // 4101: If $0~String0 is intended to be a further table reference
  //   in the FROM clause, the preceding join search condition must be
  //   enclosed in parentheses.
  //   (Or the rightmost row-value-ctor must be parenthesized.
  //	  Or the table ref must come *first* in the list of FROM tbl-refs.
  //    But this is all too wordy for a single error message!)
  //
  // Note that if 'c' in the above query is unambiguously found in scope
  // (column of 'ta' or 'tb'), then we need to first emit error 4042, then 4101.
  //
  if (getColRefNameObj().getCorrNameObj().getQualifiedNameObj().getCatalogName()
      .isNull()) {  // a 4-part name can only be a colref, so 4101 doesn't apply

    // Does this colref appear in the rightmost arg of a BiRelat or Function
    // which appears rightmost in the join pred?
    //	  Note that  'mPred->containsRightmost(this)'
    //	  could NOT replace the mPredChNo lines below; e.g. in a case like
    //	  'SELECT * FROM ta JOIN tb ON a=b,c,d;'
    //	  -- c is not rightmost in the list but it is cause for error 4101.
    //
    BindContext *context = bindWA->getCurrentScope()->context();
    ItemExpr *jPred = context->inJoinPred();
    ItemExpr *mPred = context->inMultaryPred();
    Lng32 mPredChNo  = mPred? mPred->currChildNo() : 0;
    if (jPred->containsRightmost(mPred) &&	// BiR/Func is rtmost in JoinPrd
        mPredChNo &&				// RHS of BiRelat/Function
       (mPredChNo >= mPred->getArity()-1 ||	// absolute rightmost item
	!mPred->child(mPredChNo+1)))	{	// effective rightmost item

      // Does this colref appear in a list, at a position greater than
      // the degree of the LHS comparand list?
      //
      ItemExpr *iList = context->inItemList();
      Lng32 iListChNo  = iList? iList->currChildNo() : 0;
      Lng32 mPredPrevDegree = mPred->child(mPredChNo-1)->currChildNo();
      if (iListChNo && iListChNo >= mPredPrevDegree) {	// degree, not arity!

	if (!sqlCode) {
	  // The operands of a comparison predicate must be of equal degree.
	  // Error emitted here because we'll be setting errStatus and our
	  // caller won't be calling SynthType.C where this usually appears.
	  //
	  sqlCode = -4042;
	  *CmpCommon::diags() << DgSqlCode(sqlCode);
	}
	*CmpCommon::diags() << DgSqlCode(-4101) << DgString0(colRefStr);

	// Rather than emit these errors, we could take this "col" ref
	// and the remainder of the iList and rewrite them as tbl refs (Scans),
	// attaching them to the parent query tree in the proper place.
	// Then we could achieve full Ansi syntax conformance.
      }
    }
  }

  if (sqlCode) {
#ifndef NDEBUG					// ##tmp
    // The following debug code is often useful when debugging 
    // internal queries when the metadata changes. Just set the
    // environment variable in a debug build to see the output.
    if (getenv("COLREFERENCE_DEBUG")) {
      BindContext *ctxt = bindWA->getCurrentScope()->context();
      char ii = ctxt->inItemList() ?    'i' : ' ';
      char jj = ctxt->inJoinPred() ?    'j' : ' ';
      char mm = ctxt->inMultaryPred() ? 'm' : ' ';
      Int32 iia = ii == ' ' ? -99 : ctxt->inItemList()->getArity();
      Int32 iic = ii == ' ' ? -99 : ctxt->inItemList()->currChildNo();
      Int32 jja = jj == ' ' ? -99 : ctxt->inJoinPred()->getArity();
      Int32 jjc = jj == ' ' ? -99 : ctxt->inJoinPred()->currChildNo();
      Int32 mma = mm == ' ' ? -99 : ctxt->inMultaryPred()->getArity();
      Int32 mmc = mm == ' ' ? -99 : ctxt->inMultaryPred()->currChildNo();
      cout << getColRefNameObj().getColRefAsAnsiString()
        << "	(" << ii << " " << iia << " " << iic << ") "
        << "	(" << jj << " " << jja << " " << jjc << ") "
        << "	(" << mm << " " << mma << " " << mmc << ") "
        << endl;
    }
#endif // NDEBUG
    bindWA->setErrStatus();
    return this;
  }

  if (NULL == xcnmEntry)
  {
    bindWA->setErrStatus();
    return this;
  }

  // Continue with no-error, non-star column reference.
  ValueId valId = xcnmEntry->getValueId();
  setValueId(valId);	// not bound yet, but this makes more informative errmsg
  			// if ColReference::getText() or unparse() is used

  const NAType *xcnmType = &valId.getType();
  const NAType *thisType = synthTypeWithCollateClause(bindWA, xcnmType);

  if (thisType != xcnmType) {
    if (!thisType) return this;

    // We have a new type because an explicit COLLATE clause was specified
    // in the query (e.g., SELECT charColumn COLLATE SJIS FROM ...)
    // We must now CAST(BaseColumn AS xxx COLLATE zzz).
    // Yes, we must do this even if BaseColumn's collation is IMPLICITly "zzz".
    // Yes, we must CAST, not changeType, as that would change the BaseColumn
    // from IMPLICIT to EXPLICIT for *all* ColRef's!
    //
    // Compare propagateCoAndCoToChildren() in SynthType.cpp.
    //
    ItemExpr *itemExpr = valId.getItemExpr();
    itemExpr = new (bindWA->wHeap()) Cast(itemExpr, thisType);
    itemExpr = itemExpr->bindNode(bindWA);
    if (bindWA->errStatus()) return this;
    valId = itemExpr->getValueId();
    setValueId(valId);
  }

  const NAType &naType = valId.getType();
  if (!naType.isSupportedType() && !bindScope->context()->inSelectList()) {
    *CmpCommon::diags() << DgSqlCode(-1010);
    bindWA->setErrStatus();
    return this;
   }

  // If the column reference is in a GROUP BY, mark it as a grouping column.
  //
  if ((currScope->context()->inGroupByClause()) AND
      (bindScope == currScope) AND
      (!udfSubqInAggGrby_Enabled ||
       (!currScope->context()->inUDFunction())))
     xcnmEntry->getColumnDesc()->setGroupedFlag();
  //
  // If a local column reference is in a HAVING clause or in the select list of
  // a grouped table, or an outer reference is in a subquery that is in a
  // HAVING clause or in the select list of a grouped table, the column
  // reference must be a grouping column or be specified within an aggregate.
  //
  if (bindScope->context()->inHavingClause() OR (
      bindScope->context()->inSelectList() AND
      bindScope->getRETDesc()->isGrouped() AND
      (NOT bindScope->context()->inGroupByOrdinal())
     ))
    if (NOT xcnmEntry->getColumnDesc()->isGrouped() AND
	//NOT (currScope->context()->inAggregate() || currScope->context()->inUDFunction())) {
	NOT (currScope->context()->inAggregate() )) {
      // 4005: col must be grouping col or specified within an aggregate
      *CmpCommon::diags() << DgSqlCode(-4005)
                 << DgColumnName(getColRefNameObj().getColRefAsAnsiString());
      bindWA->setErrStatus();
      return this;
    }
  // If the bindScope's table later on turns into a grouped table
  // (no groupby columns exist, but if an aggregate on a column of that table,
  //  as an outer ref, will turn the table into a grouped table of one group),
  // then this nonaggregated column reference will become illegal, by
  // ANSI 7.9 SR 7.  Mark this here and check later in RelRoot::bindNode().
  // Also added check for the case where we have a subquery inside an
  // aggregate.
  if (bindScope->context()->inSelectList() AND
      NOT bindScope->getRETDesc()->isGrouped() AND
      NOT (currScope->context()->inAggregate() OR
           (udfSubqInAggGrby_Enabled AND
            (bindScope->context()->inAggregate() AND
             bindScope->context()->inSubquery()))) AND
      NOT bindScope->context()->unaggColRefInSelectList())
    bindScope->context()->unaggColRefInSelectList() = valId.getItemExpr();
  //
  // ANSI 6.5 SR 4 states that,
  // "If an outer reference is in an aggregate, it must be the only column
  // reference in the aggregate."
  // As an extension (the second if-test here), we say that all column refs
  // in an aggregate must come from the same scope.
  //
  if (currScope->context()->outerColRefInAgg() ||
      ((bindScope != currScope) && currScope->context()->colRefInAgg()))
    if (currScope->context()->aggScope() != bindScope) {
      // 4006: within aggregate all col refs must be from same scope
      *CmpCommon::diags() << DgSqlCode(-4006);
      bindWA->setErrStatus();
      return this;
    }
  if (currScope->context()->inAggregate()) {
    currScope->context()->colRefInAgg() = TRUE;
    currScope->context()->aggScope() = bindScope;	// outer OR local/curr
  }
  // If the column reference is an outer reference, add it to the outer
  // references list unless the outer reference is in an aggregate.  If it's
  // in an aggregate, the aggregate will be added to the outer references list
  // when the aggregate node has been bound.
  //
  if (bindScope != currScope) {
    if (currScope->context()->inAggregate())
      currScope->context()->outerColRefInAgg() = TRUE;
    else
      currScope->addOuterRef(valId);
  }

  if (bindScope != currScope) 
  {
    //Paramaters and outer references are not supported with rank function.
    if (currScope->context()->inTDFunction())
    {
      *CmpCommon::diags() << DgSqlCode(-4369);
      bindWA->setErrStatus();
      return this;
    }
    //Paramaters and outer references in the PARTITION BY or ORDER BY clause of a window function are not supported.
    if (currScope->context()->inOlapOrderBy() || 
        currScope->context()->inOlapPartitionBy())
    {
      *CmpCommon::diags() << DgSqlCode(-4391);
      bindWA->setErrStatus();
      return this;
    }
  }
  //4391

  BindUtil_UpdateNameLocForColRef(bindWA, getColRefNameObj(), xcnmEntry,
				  getParent());
  if (!currScope->context()->inComputedColumnExpr())
    bindWA->markAsReferencedColumn(xcnmEntry->getColumnDesc());

  bindSelf(bindWA);
  return valId.getItemExpr();
} // ColReference::bindNode()

// -----------------------------------------------------------------------
// member functions for class ConstValue
// -----------------------------------------------------------------------

ItemExpr *ConstValue::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();
  const NAType *type = synthTypeWithCollateClause(bindWA);
  if (!type) return this;

  // Fabricate a name for the constant such that its type prefixes the value.
  NAString typeName(type->getTypeSQLname(), bindWA->wHeap());
  size_t len = typeName.length();

  // If numeric constant, allocate space to hold the scale identifier.
  // Scale is implicit for exact numeric value and is not stored
  // with the constant value. So it is needed to differentiate between
  // two constants which 'look' the same except for their scale.
  // For example, 2 and .2 are both smallint, with length of 1,
  // still are different constants.
  char scale_val[4];			// max 2 digits of scale + 1 for null
  char *scale_buf = NULL;
  size_t scale_len = 0;
  if (type->getTypeQualifier() == NA_NUMERIC_TYPE) {
    scale_buf = scale_val;
    str_itoa(((NumericType *)type)->getScale(), scale_buf);
    scale_len = strlen(scale_buf);
  }

  size_t value_len = getStorageSize();

  char *buf = new char[len + scale_len + 2*value_len + 1];
  memset(buf,0, len + scale_len + 2*value_len +1);
  memcpy(buf,typeName.data(),len);

  if (scale_buf) {
    memcpy(&buf[len], scale_buf, scale_len);
    len += scale_len;
  }

  // Now encode the actual value into the fabricated name such that no
  // null bytes appear (because the name will be used as a hash key
  // and RogueWave will use C string comparison) --
  // so we precede every value byte with a tag byte.

  char *bufp = &buf[len];
  char *valp = (char *)value_;
  len += 2*value_len;
  while (value_len--)
  {
    if (*valp)
    {
      *bufp++ = 'n';		// tag byte
      *bufp++ = *valp++;
    }
    else
    {
      *bufp++ = 'z';		// tag byte
      *bufp++ = 'z';		// embedded null does not appear!
      valp++;
    }
  }

  buf[len++] = '\0';
  NAString fabricatedName(buf,len,bindWA->wHeap());
  delete [] buf;
  ItemExpr * result = ItemExpr::bindUserInput(bindWA,type,fabricatedName);
  ConstValue* cv = dynamic_cast<ConstValue*>(result);
  CURRENTQCACHE->getHQC()
             ->collectBinderRetConstVal4HQC(this, cv);
             
  return result;
} // ConstValue::bindNode()

// -----------------------------------------------------------------------
// member functions for class DefaultSpecification
// -----------------------------------------------------------------------

ItemExpr *DefaultSpecification::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();

  // If immediately contained in an Insert, and binding the source VALUES list
  BindScope   *scope   = bindWA->getCurrentScope();
  BindContext *context = scope->context();
  if (context->inInsert() &&
      context->updateOrInsertScope() == scope &&
      context->counterForRowValues()) {

    Insert *insert = (Insert *)context->updateOrInsertNode();
    if (insert &&
        insert->getOperatorType() == REL_UNARY_INSERT &&
        insert->canBindDefaultSpecification()) {

      const char *defaultValueStr =
	insert->getColDefaultValue(bindWA, *context->counterForRowValues());

      // If column has NO DEFAULT, then getColDefaultValue() emitted error -4107
      // and set bindWA errstatus.
      if (!defaultValueStr) return NULL;

      // The DEFAULT specification replaces itself with a ConstValue
      // whose value is the default value for the column corresponding
      // to this position in the source tuple and target column list.  E.g.,
      //   INSERT INTO T(C,B,A) VALUES(1,2,DEFAULT)
      // the DEFAULT == position 3 == *context->counterForRowValues()
      // and getColDefaultValue(3) gets the default literal for column A.
      //
      // After this, the DefaultSpecification node is not seen again.
      //

      // Set the special parser flag to allow IDENTITY as a function.

      ULng32 savedParserFlags = Get_SqlParser_Flags (0xFFFFFFFF);
      Set_SqlParser_Flags(ALLOW_VOLATILE_SCHEMA_IN_TABLE_NAME);

      Parser parser(bindWA->currentCmpContext());

      ItemExpr *defaultValueExpr =
        parser.getItemExprTree(defaultValueStr);

      Assign_SqlParser_Flags (savedParserFlags);

      // It is possible to have a SQL/MP default value that SQL/MX
      // cannot parser.  In these case SQL/MX is not compatible with
      // SQL/MP and an error is reported.
      //
      if(!defaultValueExpr)
        {
          bindWA->setErrStatus();
          return NULL;
        }

      ItemExpr *boundExpr = NULL;

      boundExpr = defaultValueExpr->bindNode(bindWA);

      if (bindWA->errStatus()) return NULL;

      if (defaultValueExpr->getOperatorType() == ITM_SEQUENCE_VALUE)
        {
          insert->setSystemGeneratesIdentityValue(TRUE);
        }

      // Remember the fact that the literal used to be a DEFAULT spec
      if (boundExpr->getOperatorType() == ITM_CONSTANT)
        ((ConstValue *)boundExpr)->setWasDefaultSpec();

      boundExpr->setWasDefaultClause(TRUE);

      setValueId(boundExpr->getValueId());
      return getValueId().getItemExpr();
    }
  }

  // 4096 A DEFAULT specification is allowed only when simply contained
  //	  in the VALUES list of an INSERT.
  *CmpCommon::diags() << DgSqlCode(-4096);
  bindWA->setErrStatus();
  return NULL;

} // DefaultSpecification::bindNode()

// -----------------------------------------------------------------------
// member functions for class SleepFunction 
// -----------------------------------------------------------------------

ItemExpr *SleepFunction::bindNode(BindWA *bindWA)
{

  if (bindWA->inDDL() && (bindWA->inCheckConstraintDefinition()))
  {
	StmtDDLAddConstraintCheck *pCkC = bindWA->getUsageParseNodePtr()
                                    ->castToElemDDLNode()
                                    ->castToStmtDDLAddConstraintCheck();
    *CmpCommon::diags() << DgSqlCode(-4131);
    bindWA->setErrStatus();
    return this;
  }

  if (nodeIsBound())
    return getValueId().getItemExpr();
  const NAType *type = synthTypeWithCollateClause(bindWA);
  if (!type) return this;

  ItemExpr * ie = ItemExpr::bindUserInput(bindWA,type,getText());
  if (bindWA->errStatus())
    return this;

  // add this value id to BindWA's input function list.
  bindWA->inputFunction().insert(getValueId());

  return ie;
} // SleepFunction::bindNode()

// -----------------------------------------------------------------------
// member functions for class UnixTimestamp
// -----------------------------------------------------------------------

ItemExpr *UnixTimestamp::bindNode(BindWA *bindWA)
{

  if (bindWA->inDDL() && (bindWA->inCheckConstraintDefinition()))
  {
	StmtDDLAddConstraintCheck *pCkC = bindWA->getUsageParseNodePtr()
                                    ->castToElemDDLNode()
                                    ->castToStmtDDLAddConstraintCheck();
    *CmpCommon::diags() << DgSqlCode(-4131);
    bindWA->setErrStatus();
    return this;
  }

  if (nodeIsBound())
    return getValueId().getItemExpr();
  const NAType *type = synthTypeWithCollateClause(bindWA);
  if (!type) return this;

  ItemExpr * ie = ItemExpr::bindUserInput(bindWA,type,getText());
  if (bindWA->errStatus())
    return this;

  // add this value id to BindWA's input function list.
  bindWA->inputFunction().insert(getValueId());

  return ie;
} // UnixTimestamp::bindNode()

// -----------------------------------------------------------------------
// member functions for class CurrentTimestamp
// -----------------------------------------------------------------------

ItemExpr *CurrentTimestamp::bindNode(BindWA *bindWA)
{

  if (bindWA->inDDL() && (bindWA->inCheckConstraintDefinition()))
  {
	StmtDDLAddConstraintCheck *pCkC = bindWA->getUsageParseNodePtr()
                                    ->castToElemDDLNode()
                                    ->castToStmtDDLAddConstraintCheck();
    *CmpCommon::diags() << DgSqlCode(-4131);
    bindWA->setErrStatus();
    return this;
  }

  if (nodeIsBound())
    return getValueId().getItemExpr();
  const NAType *type = synthTypeWithCollateClause(bindWA);
  if (!type) return this;
  //
  // ANSI requires that multiple references to CURRENT_DATE, CURRENT_TIME,
  // or CURRENT_TIMESTAMP in the same SQL statement be effectively evaluated
  // simultaneously, so all CurrentTimestamp functions are treated as input
  // values and are given the same value id.
  //
  ItemExpr * ie = ItemExpr::bindUserInput(bindWA,type,getText());
  if (bindWA->errStatus())
    return this;

  // add this value id to BindWA's input function list.
  bindWA->inputFunction().insert(getValueId());

  return ie;
} // CurrentTimestamp::bindNode()


//++Triggers

// -----------------------------------------------------------------------
// member functions for class UniqueExecuteId
// -----------------------------------------------------------------------
ItemExpr *UniqueExecuteId::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();
  const NAType *type = synthesizeType();
  if (!type) {
    bindWA->setErrStatus();
    return this;
  }

  //
  // functions of this type are treated as input
  // values and are given the same value id.
  //
  ItemExpr * ie = ItemExpr::bindUserInput(bindWA,type,getText());
  if (bindWA->errStatus())
    return this;

  // add this value id to BindWA's input function list.
  bindWA->inputFunction().insert(getValueId());

  return ie;
} // UniqueExecuteId::bindNode()

// -----------------------------------------------------------------------
// member functions for class GetTriggersStatus
// -----------------------------------------------------------------------
ItemExpr *GetTriggersStatus::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();
  const NAType *type = synthesizeType();
  if (!type) {
    bindWA->setErrStatus();
    return this;
  }

  //
  // functions of this type are treated as input
  // values and are given the same value id.
  //
  ItemExpr * ie = ItemExpr::bindUserInput(bindWA,type,getText());
  if (bindWA->errStatus())
    return this;

  // add this value id to BindWA's input function list.
  bindWA->inputFunction().insert(getValueId());

  return ie;
} // GetTriggersStatus::bindNode()

//--Triggers

// -----------------------------------------------------------------------
// member functions for class CurrentTimestampRunning
// -----------------------------------------------------------------------

ItemExpr *CurrentTimestampRunning::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();
  const NAType *type = synthTypeWithCollateClause(bindWA);
  if (!type) return this;

  ItemExpr *boundExpr = ItemExpr::bindNode(bindWA);
  return boundExpr;
} // CurrentTimestampRunning::bindNode()

// -----------------------------------------------------------------------
// member functions for class Parameter
// -----------------------------------------------------------------------

ItemExpr *Parameter::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();
  if (bindWA->getCurrentScope()->context()->inTDFunction())
  {
    //Paramaters and outer references are not supported with rank function.
    *CmpCommon::diags() << DgSqlCode(-4369);
    bindWA->setErrStatus();
    return this;
  }
  if (bindWA->getCurrentScope()->context()->inOlapOrderBy() ||
      bindWA->getCurrentScope()->context()->inOlapPartitionBy())
  {
    //Paramaters and outer references in the PARTITION BY or ORDER BY clause of a window function are not supported.
    *CmpCommon::diags() << DgSqlCode(-4391);
    bindWA->setErrStatus();
    return this;
  }

  if ( bindWA->bindingCall () && ITM_DYN_PARAM == getOperatorType ())
  {
    // Are we in a trigger?
    // This needs to be ahead of trying access host areas
    if (bindWA->isInTrigger()) {
      *CmpCommon::diags() <<  DgSqlCode(-11046);
      bindWA->setErrStatus ();
      return this;
    }

    // We do not allow Rowsets in CALL yet.
    if ( bindWA->getHostArraysArea()->hasDynamicRowsets() )
    {
      *CmpCommon::diags() <<
      DgSqlCode(-UDR_BINDER_NO_ROWSET_IN_CALL);
      bindWA->setErrStatus ();
      return this;
    }

    setPMOrdPosAndIndex(bindWA->getCurrParamMode(),
                        (Int32) bindWA->getCurrOrdinalPosition(),
                        getHVorDPIndex());
    bindWA->addHVorDPToSPDups(this);

    if (!bindWA->getDupWarning() && bindWA->checkMultiOutSPParams(this))
    {
      *CmpCommon::diags()
        << DgSqlCode(UDR_BINDER_MULTI_HOSTVAR_OR_DP_IN_PARAMS)
        << DgString0(((DynamicParam *) this)->getName())
        << DgTableName(bindWA->getCurrSPName().getQualifiedNameAsString());

      bindWA->setDupWarning (TRUE);
    }

    // CLI support for CALL stmt OUT params
    if ( COM_INPUT_COLUMN == bindWA->getCurrParamMode () ||
         COM_INOUT_COLUMN == bindWA->getCurrParamMode () )
    {
      bindWA->getSpInParams().insert ( this );
    } // if INPUT or INOUT
    if ( COM_OUTPUT_COLUMN == bindWA->getCurrParamMode () ||
         COM_INOUT_COLUMN == bindWA->getCurrParamMode () )
    {
      // OUT param suppport, CLI will use this
      // During RelRoot::bindNode this is copied into the RelRoot's
      // private area
      bindWA->getSpOutParams().insert( this );
    } // if OUTPUT or INOUT
  } // binding a CALL statement

  const NAType *type = synthTypeWithCollateClause(bindWA);
  if (!type) return this;

  if(getOperatorType() == ITM_ROUTINE_PARAM) 
  {
      // Don't want to send the ROUTINE_PARAMs through bindUserInput
      // as they are used as fake inputs, but real outputs. Thus they
      // cannot be a UserInput
    setValueId(createValueDesc(bindWA, this, type));
    bindSelf(bindWA);
    if (bindWA->errStatus()) return this;
    return getValueId().getItemExpr();
  } else
    return ItemExpr::bindUserInput(bindWA,type,getText());
} // Parameter::bindNode()

// -----------------------------------------------------------------------
// member functions for class HostVar
// -----------------------------------------------------------------------

ItemExpr *HostVar::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();

  const NAType *type = synthTypeWithCollateClause(bindWA);
  if (!type) return this;

  if ( bindWA->bindingCall ())
  {
    // Are we in a trigger
    if (bindWA->isInTrigger()) {
      *CmpCommon::diags() <<  DgSqlCode(-11046);
      bindWA->setErrStatus ();
      return this;
    }

    // No rowsets as CALL parameters yet
    if ( NA_ROWSET_TYPE == getType()->getTypeQualifier() )
    {
      *CmpCommon::diags() <<
      DgSqlCode(-UDR_BINDER_NO_ROWSET_IN_CALL);
      bindWA->setErrStatus ();
      return this;
    }

    setPMOrdPosAndIndex(bindWA->getCurrParamMode(),
                        (Int32) bindWA->getCurrOrdinalPosition(),
                        hvIndex_);
    bindWA->addHVorDPToSPDups(this);

    if (!bindWA->getDupWarning() && bindWA->checkMultiOutSPParams(this))
    {
      *CmpCommon::diags()
        << DgSqlCode(UDR_BINDER_MULTI_HOSTVAR_OR_DP_IN_PARAMS)
        << DgString0(getName())
        << DgTableName(bindWA->getCurrSPName().getQualifiedNameAsString());

      bindWA->setDupWarning(TRUE);
    }

    // CLI support for CALL stmt OUT params
    if ( COM_INPUT_COLUMN == bindWA->getCurrParamMode () ||
         COM_INOUT_COLUMN == bindWA->getCurrParamMode () )
    {
      bindWA->getSpInParams().insert ( this );
    } // if INPUT or INOUT

    if ( COM_OUTPUT_COLUMN == bindWA->getCurrParamMode () ||
         COM_INOUT_COLUMN == bindWA->getCurrParamMode () )
    {
      // OUT param suppport, CLI will use this
      // During RelRoot::bindNode this is copied into the RelRoot's
      // private area
      bindWA->getSpOutParams().insert( this );
    } // if OUTPUT or INOUT
  } // if bindingCall
  else
  {
    // We can be here during RelRoot::bindNode also. At this point
    // bindingCall () will be FALSE, but we still need to set
    // the variable's PMOrdPos etc.
    // Also, if we come here during a post-bind phase
    // and the bindWA's HVorDP list is empty, 'h' below will be NULL
    HostVar *h = (HostVar *) bindWA->getHVorDPFromSPDups (this);
    if (h)
    {
      setPMOrdPosAndIndex (h->getParamMode (),
                           h->getOrdinalPosition (),
                           h->getHVorDPIndex ());
    }
  }

  ItemExpr * ie = ItemExpr::bindUserInput(bindWA, type, getText());

  return ie;
} // HostVar::bindNode()

// -----------------------------------------------------------------------
// member functions for class RenameCol
// -----------------------------------------------------------------------

ItemExpr *RenameCol::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();
  //
  // Bind the child nodes.
  //
  bindSelf(bindWA);
  if (bindWA->errStatus()) return this;
 
  // We don't allow rename of MVF or Subq by default
  // If the CQD is turned on, we will allow a query like this
  // 
  // select mvf() as x from t1;
  //
  // and we will silently pick the first output from the mvf() to be
  // associated with x. Any other outputs from the mvf() will be ingored.
  //
  // similarly for a subquery:
  // 
  // select (select a,b from t1) as x, b from t2;
  //
  // will associate the a from the subquery's select list with x, and b will
  // quitely be ignored.


  if ( CmpCommon::getDefault(ALLOW_RENAME_OF_MVF_OR_SUBQ) == DF_OFF )
  {

    // Since UDFs can return more than one output, we have to check for that
    // here, and if it does, disallow it. 
    if (child(0)->getOperatorType() == ITM_USER_DEF_FUNCTION)
    {
      UDFunction *udf = (UDFunction *) child(0)->castToItemExpr();
      if (udf->getRoutineDesc() && 
          udf->getRoutineDesc()->getOutputColumnList().entries() > 1)
      {
        *CmpCommon::diags() << DgSqlCode(-4478);
        bindWA->setErrStatus();
        return this;
      }
    
    } 
    // Since we now allow Subqueries to return multiple columns, 
    // we have to check for that here, and if it does, disallow it. 
    else if (child(0)->getOperatorType() == ITM_ROW_SUBQUERY)
    {
  
      Subquery *subq = (Subquery *) child(0)->castToItemExpr();
      if (subq->getSubquery()->getDegree() > 1)
      {
        *CmpCommon::diags() << DgSqlCode(-4477);
        bindWA->setErrStatus();
        return this;
      }
    }
  }

  setValueId(child(0)->getValueId());
  return getValueId().getItemExpr();
}

// -----------------------------------------------------------------------
// member functions for class PositionFunc
// -----------------------------------------------------------------------

ItemExpr *PositionFunc::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();

  // this will bind/type-propagate all children.
  bindChildren(bindWA);
  if (bindWA->errStatus()) 
    return this;

  // update both operands if case insensitive comparions
  // are to be done.
  const NAType &type1 = 
    child(0)->castToItemExpr()->getValueId().getType();
  const NAType &type2 = 
    child(1)->castToItemExpr()->getValueId().getType();
  
  if ((type1.getTypeQualifier() == NA_CHARACTER_TYPE) &&
      (type2.getTypeQualifier() == NA_CHARACTER_TYPE))
    {
      const CharType &cType1 = (CharType&)type1;
      const CharType &cType2 = (CharType&)type2;
      
      NABoolean doCIcomp = 
	((cType1.isCaseinsensitive()) && (cType2.isCaseinsensitive()));
      
      ItemExpr * newChild = NULL;
      if ((doCIcomp) &&
	  (NOT cType1.isUpshifted()))
	{
	  newChild = new (bindWA->wHeap()) Upper(child(0));
	  setChild(0, newChild);
	}
      
      if ((doCIcomp) &&
	  (NOT cType2.isUpshifted()))
	{
	  newChild = new (bindWA->wHeap()) Upper(child(1));
	  setChild(1, newChild);
	}
    }

  // if third(start position) and fourth(occurence) child operands are
  // specified, then convert them to INT.
  if (child(2))
    {
      ValueId vid3 = child(2)->getValueId();
      SQLInt si(NULL);

      vid3.coerceType(si, NA_NUMERIC_TYPE);

      const NAType &type3 = vid3.getType();

      if (type3.getTypeQualifier() != NA_NUMERIC_TYPE) {
        // 4053 The third operand of a POSITION function must be numeric.
        *CmpCommon::diags() << DgSqlCode(-4053) << DgString0(getTextUpper());
        bindWA->setErrStatus();
        return NULL;
      }
      
      if (((NumericType&)type3).getScale() != 0) {
        // 4047 The third operand of a POSITION function must have a scale of 0.
        *CmpCommon::diags() << DgSqlCode(-4047) << DgString0(getTextUpper());
        bindWA->setErrStatus();
        return NULL;
      }

      if (type3.getFSDatatype() != REC_BIN32_SIGNED)
        {
          ItemExpr * newChild =
            new (bindWA->wHeap())
            Cast(child(2), 
                 new (bindWA->wHeap())
                 SQLInt(bindWA->wHeap(), TRUE, type3.supportsSQLnull()));
          newChild = newChild->bindNode(bindWA);
          setChild(2, newChild);
        }
    }

  if (child(3))
    {
      ValueId vid4 = child(3)->getValueId();
      SQLInt si(NULL);

      vid4.coerceType(si, NA_NUMERIC_TYPE);

      const NAType &type4 = vid4.getType();

      if (type4.getTypeQualifier() != NA_NUMERIC_TYPE) {
        // 4053 The third operand of a POSITION function must be numeric.
        *CmpCommon::diags() << DgSqlCode(-4053) << DgString0(getTextUpper());
        bindWA->setErrStatus();
        return NULL;
      }
      
      if (((NumericType&)type4).getScale() != 0) {
        // 4047 The third operand of a POSITION function must have a scale of 0.
        *CmpCommon::diags() << DgSqlCode(-4047) << DgString0(getTextUpper());
        bindWA->setErrStatus();
        return NULL;
      }

      if (type4.getFSDatatype() != REC_BIN32_SIGNED)
        {
          ItemExpr * newChild =
            new (bindWA->wHeap())
            Cast(child(3), 
                 new (bindWA->wHeap())
                 SQLInt(bindWA->wHeap(), TRUE, type4.supportsSQLnull()));
          newChild = newChild->bindNode(bindWA);
          setChild(3, newChild);
        }
    }

  BuiltinFunction::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return this;

  return getValueId().getItemExpr();
} // PositionFunc::bindNode()

// -----------------------------------------------------------------------
// member functions for class Replace
// -----------------------------------------------------------------------

ItemExpr *Replace::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();

  // this will bind/type-propagate all children.
  bindChildren(bindWA);
  if (bindWA->errStatus()) 
    return this;

  // update both operands if case insensitive comparions
  // are to be done.
  const NAType &type1 = 
    child(0)->castToItemExpr()->getValueId().getType();
  const NAType &type2 = 
    child(1)->castToItemExpr()->getValueId().getType();
  
  if ((type1.getTypeQualifier() == NA_CHARACTER_TYPE) &&
      (type2.getTypeQualifier() == NA_CHARACTER_TYPE))
    {
      const CharType &cType1 = (CharType&)type1;
      const CharType &cType2 = (CharType&)type2;
      
      NABoolean doCIcomp = 
	((cType1.isCaseinsensitive()) && (cType2.isCaseinsensitive()));
      
      ItemExpr * newChild = NULL;
      /*      if ((doCIcomp) &&
	  (NOT cType1.isUpshifted()))
	{
	  newChild = new (bindWA->wHeap()) Upper(child(0));
	  setChild(0, newChild);
	}
	*/

      if ((doCIcomp) &&
	  (NOT cType2.isUpshifted()))
	{
	  newChild = new (bindWA->wHeap()) Upper(child(1));
	  setChild(1, newChild);
	  
	}
    }

  BuiltinFunction::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return this;

  return getValueId().getItemExpr();
} // Replace::bindNode()

// -----------------------------------------------------------------------
// member functions for class CharLength
// -----------------------------------------------------------------------

ItemExpr *CharLength::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();

  // this will bind/type-propagate all children.
  bindChildren(bindWA);
  if (bindWA->errStatus()) 
    return this;

  const NAType &type1 = 
    child(0)->castToItemExpr()->getValueId().getType();
  
  if (type1.getTypeQualifier() == NA_NUMERIC_TYPE)
    {
      ItemExpr * newChild = new (bindWA->wHeap()) 
        Trim((Int32)Trim::TRAILING,
             new (PARSERHEAP()) SystemLiteral(" ", WIDE_(" ")), child(0));

      setChild(0, newChild);
    }

  BuiltinFunction::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return this;
  
  return getValueId().getItemExpr();
} // CharLength::bindNode()

// -----------------------------------------------------------------------
// member functions for class OctetLength
// -----------------------------------------------------------------------

ItemExpr *OctetLength::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();

  // this will bind/type-propagate all children.
  bindChildren(bindWA);
  if (bindWA->errStatus()) 
    return this;

  const NAType &type1 = 
    child(0)->castToItemExpr()->getValueId().getType();
  
  if (type1.getTypeQualifier() == NA_NUMERIC_TYPE)
    {
      ItemExpr * newChild = new (bindWA->wHeap()) 
        Trim((Int32)Trim::TRAILING,
             new (PARSERHEAP()) SystemLiteral(" ", WIDE_(" ")), child(0));

      setChild(0, newChild);
    }

  BuiltinFunction::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return this;
  
  return getValueId().getItemExpr();
} // OctetLength::bindNode()

// -----------------------------------------------------------------------
// member functions for class SelIndex
// -----------------------------------------------------------------------

ItemExpr *SelIndex::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();
  //
  // Bind the child nodes.
  //
  bindSelf(bindWA);
  if (bindWA->errStatus())
    return this;

  BindScope * currScope = bindWA->getCurrentScope();
  if ((currScope->context()->inGroupByClause()) ||
      (currScope->context()->inHavingClause()))
    {
      // the real value id pointing to the select list element
      // will be set during phase2 of groupby ordinal transformation.
      // See RelRoot::transformGroupByWithOrdinalPhase2().

      // create a dummy type of type unknown.
      NAType * type = new(bindWA->wHeap()) SQLUnknown(bindWA->wHeap());
      setValueId(createValueDesc(bindWA, this, type));

      if ((bindWA->inViewDefinition()) &&
	  (getExprInGrbyClause()))
	{
	  // this will expand names used in the groupby clause
	  // so they could be used during view create processing.
	  getExprInGrbyClause()->bindNode(bindWA);
	}

      return this;
    }

  //
  // Check that the select list index is within the allowable range.
  //
  const CollIndex i = getSelIndex();
  RETDesc *resultTable = bindWA->getCurrentScope()->getRETDesc();
  if (i < 1 || i > resultTable->getDegree())
  {
    // 4007: select list index out of range.
    *CmpCommon::diags() << DgSqlCode(-4007) << DgInt0(i) << DgInt1(resultTable->getDegree());
    bindWA->setErrStatus();
    return this;
  }
  setValueId(resultTable->getValueId(i - 1));
  return getValueId().getItemExpr();
}

// -----------------------------------------------------------------------
// member functions for class Subquery
// -----------------------------------------------------------------------

ItemExpr *Subquery::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();

  BindContext *context = bindWA->getCurrentScope()->context();

  // See if UDF_SUBQ_IN_AGGS_AND_GBYS is enabled. It is enabled if the
  // default is ON, or if the default is SYSTEM and ALLOW_UDF is ON.
  NABoolean udfSubqInAggGrby_Enabled = FALSE;
  DefaultToken udfSubqTok = CmpCommon::getDefault(UDF_SUBQ_IN_AGGS_AND_GBYS);
  if ((udfSubqTok == DF_ON) ||
      (udfSubqTok == DF_SYSTEM))
    udfSubqInAggGrby_Enabled = TRUE;
  
  if (!udfSubqInAggGrby_Enabled)
  {

    if (context->inAggregate()) {
      // 4008: A subquery is not allowed inside an aggregate.
      *CmpCommon::diags() << DgSqlCode(-4008);
      bindWA->setErrStatus();
      return this;
    }
  
    // a subquery is not allowed to be referenced as a group by ordinal.
    if (context->inGroupByOrdinal()) {
      *CmpCommon::diags() << DgSqlCode(-4185);
      bindWA->setErrStatus();
      return NULL;
    }
  }

  // subquery is not allowed in the join predicate of Full Outer Join.
  if (context->inJoinPred() &&
      (context->inJoin()->getOperatorType() == REL_FULL_JOIN))
    {
      *CmpCommon::diags() << DgSqlCode(-4339);
      bindWA->setErrStatus();
      return NULL;
    }

  if (getArity() && checkForSQLnullChild(bindWA, this)) return this;

  // Bind the child nodes.
  //
  bindSelf(bindWA);
  if (bindWA->errStatus())
    return this;

  //
  // Bind the subquery tree
  //
  context = bindWA->getCurrentScope()->context();
  NABoolean orig = context->inSubquery();
  NABoolean origRow = context->inRowSubquery();
  context->inSubquery() = TRUE;
  context->inRowSubquery() = (isARowSubquery());

  tableExpr_ = getSubquery()->bindNode(bindWA);

  context->inSubquery() = orig;
  context->inRowSubquery() = origRow;
  if (bindWA->errStatus())
    return this;
  
  // QSTUFF
  // we don't allow streams in subqueries.
  if (tableExpr_->getGroupAttr()->isStream()){
    *CmpCommon::diags() << DgSqlCode(-4168);
    bindWA->setErrStatus();
    return this;
  }
  
  // we don't allow destructive selects or embedded inserts in subqueries.
  // The SeqGenSubquery updating the SG Table and returning the
  // next value is an exception. 
  if (1 && 
      ((tableExpr_->getGroupAttr()->isEmbeddedUpdateOrDelete()) 
       || (tableExpr_->getGroupAttr()->isEmbeddedInsert())         
       || (bindWA->isEmbeddedIUDStatement()))
      )
    {
      NAString type;
      if (tableExpr_->getGroupAttr()->isEmbeddedUpdate())
	type = "UPDATE";
      else
	{
	  if (tableExpr_->getGroupAttr()->isEmbeddedInsert())
	    type = "INSERT";
	  else
	    type = "DELETE";
	}
      
      *CmpCommon::diags()
	<< DgSqlCode(-4167)
	<< DgString0(type);
      bindWA->setErrStatus();
      return this;
    }
  // QSTUFF
  
  // Create a ValueDesc for this ItemExpr.
  //
  const NAType *type = synthTypeWithCollateClause(bindWA);
  if (!type) return this;
  setValueId(createValueDesc(bindWA, this, type));
  return getValueId().getItemExpr();
} // Subquery::bindNode()

ItemExpr *QuantifiedComp::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();

  ItemExpr *boundExpr = Subquery::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return this;

  // if left child is incompatible with right child, insert a node
  // to convert left to right.
  if ((CmpCommon::getDefault(ALLOW_INCOMPATIBLE_OPERATIONS) == DF_ON) &&
      (createdFromINlist()))
    {
      const NAType &type1 = 
	child(0)->castToItemExpr()->getValueId().getType();
      const NAType &type2 = 
	getSubquery()->selectList()->castToItemExpr()->getValueId().getType();

      if (type1.getTypeQualifier() != type2.getTypeQualifier())
	{
	  if ((DFS2REC::isCharacterString(type1.getFSDatatype())) &&
	      (type2.getTypeQualifier() == NA_NUMERIC_TYPE))
	    {
	      // only supporting char lhs at this time. Add more later.
	      ItemExpr * newChild =
		new (bindWA->wHeap())
		Cast(child(0),
		     new (bindWA->wHeap())
		     SQLDoublePrecision(bindWA->wHeap(),
			  child(0)->castToItemExpr()->getValueId().
			  getType().supportsSQLnull()));
	      newChild = newChild->bindNode(bindWA);
	      if(bindWA->errStatus()) 
		return NULL;
	      
	      if(newChild)
		setChild(0, newChild);
	    }
	  else
	    {
	      emitDyadicTypeSQLnameMsg(-4041, type1, type2);
	      bindWA->setErrStatus();
	      return NULL;
	    }
	}
    }  

  return boundExpr;
}

ItemExpr *Substring::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();

  // this will bind/type-propagate all children.
  bindChildren(bindWA);
  if (bindWA->errStatus()) 
    return this;

  if (CmpCommon::getDefault(MODE_SPECIAL_1) == DF_ON)
    {
      // allow substring on exact, binary numeric operand with scale of zero.
      const NAType &type1 = 
	child(0)->castToItemExpr()->getValueId().getType();

      if (type1.getTypeQualifier() == NA_NUMERIC_TYPE)
	{
	  NumericType &nType = (NumericType&)type1;
	  if ((nType.isExact()) &&
	      (nType.getScale() == 0) &&
	      (nType.isSimpleType()))
	    {
	      Parser parser(bindWA->currentCmpContext());
	      char buf[1000];
	      
	      Lng32 dlen =
		nType.getDisplayLength(nType.getFSDatatype(),
				       nType.getNominalSize(),
				       nType.getPrecision(),
				       nType.getScale(),
				       0);
	      ItemExpr * parseTree ;
	      // right justify the string representation of numeric operand 
	      // and then do substring.
	      if (getNumChildren() == 2)
	      {
		sprintf(buf, "SUBSTRING(SPACE(%d - CHAR_LENGTH(CAST(@A1 AS VARCHAR(%d)))) || CAST(@A1 AS VARCHAR(%d)), @A2)",
		      dlen, dlen, dlen);
		parseTree = parser.getItemExprTree(buf, strlen(buf), BINDITEMEXPR_STMTCHARSET, 2, child(0), child(1));
	      }
	      else
	      {
		CMPASSERT(getNumChildren() == 3);
		sprintf(buf, "SUBSTRING(SPACE(%d - CHAR_LENGTH(CAST(@A1 AS VARCHAR(%d)))) || CAST(@A1 AS VARCHAR(%d)), @A2, @A3)",
			dlen, dlen, dlen);
		parseTree = parser.getItemExprTree(buf, strlen(buf), BINDITEMEXPR_STMTCHARSET, 3, child(0), child(1), child(2));
	      }
	      
	      parseTree = parseTree->bindNode(bindWA);
	      if (bindWA->errStatus()) 
		return NULL;
	      else
		return parseTree;
	    }
	}
      else if (type1.getTypeQualifier() == NA_DATETIME_TYPE)
	{
	  // Convert stored date to numeric and then substring.
	  // Numeric value of a date is: (YYYY-1900)*10000 + (MM*100) + DD
	  // Then cast this numeric value as CHAR(7) before doing 
	  // the substring.
	  DatetimeType &dtType = (DatetimeType&)type1;
	  if (dtType.getPrecision() == SQLDTCODE_DATE)
	    {
	      // Cast DATE to INT
	      ItemExpr * newChild =
		new (bindWA->wHeap())
		Cast(child(0), 
		     new (bindWA->wHeap())
		     SQLInt(bindWA->wHeap(), TRUE, type1.supportsSQLnull()));
	      newChild = newChild->bindNode(bindWA);

	      // Cast INT to CHAR(7).
	      newChild =
		new (bindWA->wHeap())
		Cast(newChild,
		     new (bindWA->wHeap())
		     SQLChar(bindWA->wHeap(), 7, type1.supportsSQLnull()));
	      newChild = newChild->bindNode(bindWA);
	      setChild(0, newChild);
	    }
	}
    }

  // Substring inherits from BuiltinFunction .. Function .. ItemExpr.
  BuiltinFunction::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return this;

  return getValueId().getItemExpr();
}

// -----------------------------------------------------------------------
// member functions for class Exists
// -----------------------------------------------------------------------

ItemExpr *Exists::bindNode(BindWA *bindWA)
{
  BindContext *context = bindWA->getCurrentScope()->context();
  NABoolean orig = context->inExistsPredicate();
  context->inExistsPredicate() = TRUE;
  ItemExpr *boundExpr = Subquery::bindNode(bindWA);
  context->inExistsPredicate() = orig;
  return boundExpr;
} // Exists::bindNode()



// -----------------------------------------------------------------------
// member functions for class UDFunction
// -----------------------------------------------------------------------
ItemExpr *UDFunction::bindNode(BindWA *bindWA)
{
  NARoutine *udf = 0, *udfAction = 0;

  if (nodeIsBound())
    return getValueId().getItemExpr();

  ItemExpr *boundExpr = NULL;

  // IS req 8: Check if UDF is in certain query contexts.
  // No error check on return, these use NAList/NACollection and NAAbort if out of range.
  BindScope *curScope = bindWA->getCurrentScope();
  BindContext *curContext = bindWA->getCurrentScope()->context();
  curContext->inUDFunction() = TRUE;

  // UDFs not allowed in argument list of a sequence function.
  if (curContext->inSequenceFunction())
  {
    *CmpCommon::diags() << DgSqlCode(-4461)
                        << DgString0(functionName_.getExternalName());
    bindWA->setErrStatus();
    return this;
  }

  // UDFs not allowed in ORDER BY clause of OLAP window function.
  if (((curScope->getSequenceNode() &&
         ((RelRoot *)curScope->getSequenceNode())->getHasOlapFunctions()) ||
       (curScope->getSequenceNode() &&
         ((RelSequence *)curScope->getSequenceNode())->getHasOlapFunctions()) ||
       (curContext->inOtherSequenceFunction())) &&
      curContext->inOrderBy())
  {
    *CmpCommon::diags() << DgSqlCode(-4462)
                        << DgString0(functionName_.getExternalName());
    bindWA->setErrStatus();
    return this;
  }
  // UDFs not allowed in ON clause of a full outer join.
  if (curContext->inJoin() &&
      curContext->inJoin()->isFullOuterJoin() &&
      curContext->inJoinPred())
  {
    *CmpCommon::diags() << DgSqlCode(-4463)
                        << DgString0(functionName_.getExternalName());
    bindWA->setErrStatus();
    return this;
  }
  // UDFs not allowed in WHEN clause of an AFTER trigger.
  if (bindWA->getUsageParseNodePtr() &&
      bindWA->getUsageParseNodePtr()->getOperatorType() == DDL_CREATE_TRIGGER)
  {
    StmtDDLCreateTrigger *trigger = (StmtDDLCreateTrigger *) bindWA->getUsageParseNodePtr();
    if (trigger->isAfter() && 
        (curContext->inPredicate() || curContext->inRangePred()))
    {
      *CmpCommon::diags() << DgSqlCode(-4464)
                        << DgString0(functionName_.getExternalName());
      bindWA->setErrStatus();
      return this;
    }
  }
  // UDFs not allowed in WHERE clause of an DELETE [FIRST N] query.
  if (curContext->deleteNode() && curContext->inWhereClause() && curContext->firstN())
  {
    *CmpCommon::diags() << DgSqlCode(-4465)
                        << DgString0(functionName_.getExternalName());
    bindWA->setErrStatus();
    return this;
  }
  // UDFs not allowed in WHERE clause of an UPDATE [FIRST N] query.
  if (curContext->inUpdate() && curContext->inWhereClause() && curContext->firstN())
  {
    *CmpCommon::diags() << DgSqlCode(-4466)
                        << DgString0(functionName_.getExternalName());
    bindWA->setErrStatus();
    return this;
  }
  // UDFs not allowed in WHERE clause of an INSERT .. SELECT [FIRST N] query.
  if (bindWA->isInsertSelectStatement()) 
  {
    BindScope *prevScope = bindWA->getPreviousScope(curScope);  
    if (prevScope) // Must use previous scope for insert/select.
    {
      BindContext *prevContext = prevScope->context();
      if (prevContext && 
          curContext->inWhereClause() && prevContext->firstN())
      {
        *CmpCommon::diags() << DgSqlCode(-4473)
                            << DgString0(functionName_.getExternalName());
        bindWA->setErrStatus();
        return this;
      }
    }
  }
  // UDFs not allowed in check constraint.
  if (curContext->inCheckConstraint())
  {
    *CmpCommon::diags() << DgSqlCode(-4470)
                        << DgString0(functionName_.getExternalName());
    bindWA->setErrStatus();
    return this;
  }
  
  // Check for NARoutine for this UDF in cache (NARoutineDB)
  NAString func = functionName_.getExternalName();
  NAString action = "";
  NAHeap *heap = CmpCommon::statementHeap();

  NAString dftUdfLoc = ActiveSchemaDB()->getDefaults().getValue(UDF_METADATA_SCHEMA);
  NAString dftUdfCat, dftUdfSch;

  ComObjectName functionName1(functionName_);
  ComObjectName functionName2(functionName_);

  size_t index = dftUdfLoc.first('.');
  if (index > 1 && index < dftUdfLoc.length()) {
    dftUdfCat = dftUdfLoc(0, index);
    dftUdfSch = dftUdfLoc(index+1, dftUdfLoc.length()-index-1);
    if (dftUdfSch.first('.') != NA_NPOS) // A delimited name was used, such that
    {                                     // we can't be sure assign was correct.
       dftUdfCat = CmpSeabaseDDL::getSystemCatalogStatic().data(); 
       dftUdfSch = SEABASE_UDF_SCHEMA; // If there is a '.' in Sch, set to well known cat.sch
                                       // Without this the QualifiedName creation in NARoutineDBKey
    }                                  // below may assert.
  }


  // Find UDF in cache or metadata
  TrafDesc *udfMetadata = NULL;
  TrafDesc *oldUdfMetadata = NULL; // not used, just to get code to compile
  CmpSeabaseDDL cmpSBD(heap);

  try 
  {
    if (CmpCommon::getDefault(COMP_BOOL_191) == DF_OFF) // temporary switch for
    {                                                   // real and old metadata.
      Int32 catSchNameChosen = 1; // Will be set to 1 or 2 based on
                                // which cat.sch action is found in.


      // Set functionName1 to current cat.sch - unless cat.sch specified.
      // If catalog not specified, add current catalog to name.
      if (functionName1.getCatalogNamePartAsAnsiString() == "")
        functionName1.setCatalogNamePart(
          bindWA->getDefaultSchema().getCatalogName());
      // If schema not specified, add current schema to name.
      if (functionName1.getSchemaNamePartAsAnsiString() == "")
        functionName1.setSchemaNamePart(
          bindWA->getDefaultSchema().getSchemaName());
       // Set functionName2 to default UDF cat.sch.
      functionName2.setCatalogNamePart(dftUdfCat);
      functionName2.setSchemaNamePart(dftUdfSch);

      QualifiedName functionName1AsQualName(functionName1, heap);
      QualifiedName functionName2AsQualName(functionName2, heap);
    // in open source, only the SEABASE catalog is allowed.
    // Return an error if some other catalog is being used.
    if ((NOT functionName1AsQualName.isSeabase()) && (NOT functionName1AsQualName.isHive()))
      {
	*CmpCommon::diags()
	  << DgSqlCode(-1002)
	  << DgCatalogName(functionName1AsQualName.getCatalogName());
	
	bindWA->setErrStatus();
	return NULL;
      }

      // Check the NARoutine cache for NARoutine first.
      // 1. Look for UDF in current or specified cat/schema.
      
      NARoutineDBKey functionKey1(functionName1, heap);
      udf = bindWA->getSchemaDB()->getNARoutineDB()->get(bindWA, &functionKey1);
      catSchNameChosen = 1;

      // 2. If UDF not found in cache w/ current or spec'd cat.sch AND
      //    cat.sch NOT specified in query, look in cache for default cat.sch.
      if (NULL == udf && 
          functionName_.getSchemaNamePartAsAnsiString() == "")
      {
        // Look in NARoutine cache with default cat.sch.
        NARoutineDBKey functionKey2(functionName2, heap);
        udf = bindWA->getSchemaDB()->getNARoutineDB()->get(bindWA, &functionKey2);
        catSchNameChosen = 2;
      }
      
      // 3. If UDF not found in NARoutine cache, look up in metadata.
      if (NULL == udf)
      {
        udfMetadata =  cmpSBD.getSeabaseRoutineDesc(
				       functionName1AsQualName.getCatalogName(),
				       functionName1AsQualName.getSchemaName(),
				       functionName1AsQualName.getObjectName());
        catSchNameChosen = 1;
      }
      // 4. If UDF not found in current or specified cat.sch AND
      //    cat.sch NOT specified in query, look up in metadata with
      //    default cat.sch.
      if (NULL == udf && NULL == udfMetadata && 
          functionName_.getSchemaNamePartAsAnsiString() == "")
      {
        // Look for UDF in default cat.schema if cat.sch not 
        // spec'd on command line.
        udfMetadata =  cmpSBD.getSeabaseRoutineDesc(
				       functionName2AsQualName.getCatalogName(),
				       functionName2AsQualName.getSchemaName(),
				       functionName2AsQualName.getObjectName());

        catSchNameChosen = 2;
      }
      if (catSchNameChosen == 1) 
      { 

        functionName_.setCatalogNamePart(functionName1.getCatalogNamePartAsAnsiString());
        functionName_.setSchemaNamePart(functionName1.getSchemaNamePartAsAnsiString());
      }
      else
      { 
        functionName_.setCatalogNamePart(functionName2.getCatalogNamePartAsAnsiString());
        functionName_.setSchemaNamePart(functionName2.getSchemaNamePartAsAnsiString());
      }
      // Remove -1001 through -1004 (not found errors) - the binder reports these.
      CollIndex i;
      for (Int32 err=-1004; err<=-1001; err++)
        while ((i=CmpCommon::diags()->returnIndex(err)) != NULL_COLL_INDEX)
          CmpCommon::diags()->deleteError(i);
      // Remove -1389 (not found error) 
      while ((i=CmpCommon::diags()->returnIndex(-1389)) != NULL_COLL_INDEX)
        CmpCommon::diags()->deleteError(i);
     }
    else 
    {
        *CmpCommon::diags() << DgSqlCode(-4222)
                      << DgString0("UDF");
        bindWA->setErrStatus();
        return this;
        // we are now guaranteed that COMP_BOOL_191 is OFF for closed source
    }
  }
  catch ( ... )
  {
    // Print out something??
    //CatExceptionTypeEnum eType = e.getExceptionType();
    *CmpCommon::diags() << DgSqlCode(-4457)
                        << DgString0(functionName_.getExternalName())
                        << DgString1("Exception occurred during UDF lookup");
  }
  // Set expanded UDF name for triggers, MVs, ...
  ParNameLocList *pNameLocList = bindWA->getNameLocListPtr();
  if (pNameLocList)
  {
    ParNameLoc *pNameLoc
      = pNameLocList->getNameLocPtr(getNamePosOfUdfInQuery());
    if (pNameLoc)
      pNameLoc->setExpandedName(functionName_.getExternalName());
  }

  if (NULL == udf) // UDF/UUDF not found in NARoutine cache.
  {
    // IS req 5: Check if UDF/UUDF name found in metadata, if not output error(s).
    if ((CmpCommon::getDefault(COMP_BOOL_191) == DF_OFF &&
         NULL == udfMetadata)                              ||
        (CmpCommon::getDefault(COMP_BOOL_191) == DF_ON &&
         (NULL == oldUdfMetadata 
          ) ))
    {
      *CmpCommon::diags() << DgSqlCode(-4450)
                          << DgString0(functionName_.getExternalName());
      bindWA->setErrStatus();
      return this;
    }

    // IS req 6: Check ROUTINE_TYPE column of ROUTINES table.
    // Emit error if invalid type.
    ComRoutineType udrType = COM_UNKNOWN_ROUTINE_TYPE;
    if (CmpCommon::getDefault(COMP_BOOL_191) == DF_OFF)
      {
        udrType = udfMetadata->routineDesc()->UDRType ;
      }

    if (udrType != COM_SCALAR_UDF_TYPE &&
        udrType != COM_UNIVERSAL_UDF_TYPE)
    {
      *CmpCommon::diags() << DgSqlCode(-4457)
                          << DgString0(functionName_.getExternalName())
                          << DgString1("Only scalar or universal user-defined functions are supported");
      bindWA->setErrStatus();
      return this;
    }

    NAHeap *routineHeap;
    if (bindWA->getSchemaDB()->getNARoutineDB()->cachingMetaData()) 
    {
      // Create new heap for this NARoutine.  The reason for this is to be able to
      // track the size of this object.  Otherwise we might use the context heap.
      const Lng32 size = 16 * 1024;  // The initial size
      routineHeap = new CTXTHEAP NAHeap("NARoutine Heap", (NAHeap *)CTXTHEAP, size);
    }
    // If not caching, put NARoutine on statement heap.
    else routineHeap=CmpCommon::statementHeap(); 

    // IS req 3, 7.3. Instantiate NARoutine object.  
    // NARoutine data members will be assigned from udfMetadata.
    Int32 errors=0;
    NAString empty="";
    if (CmpCommon::getDefault(COMP_BOOL_191) == DF_OFF)
      udf = new (routineHeap)
                   NARoutine(functionName_,
                             udfMetadata, 
                             bindWA,
                             errors,
                             routineHeap);
    if ( NULL == udf || errors != 0)
    {
      bindWA->setErrStatus();
      return this;
    }

    // Add NARoutine to the NARoutineDB cache.
    if (bindWA->getSchemaDB()->getNARoutineDB()->cachingMetaData()) 
      bindWA->getSchemaDB()->getNARoutineDB()->put(udf);
  } // if (NULL == udf ) -- NARoutine not in cache.

  // Create the routineDesc and initialize it with the NARoutine for the
  // UDF. Action proceesing handled later.  See below.
  RoutineDesc *rdesc = new (bindWA->wHeap()) RoutineDesc(bindWA, udf); 
  if (rdesc == NULL)
  {
     *CmpCommon::diags() << DgSqlCode(-4457)
                         << DgString0(functionName_.getExternalName())
                         << DgString1("Internal Error creating the RoutineDesc");
     bindWA->setErrStatus();
     return this;
  }
  setRoutineDesc(rdesc); // Assign the RoutineDesc pointer in UDFunction.

  // Ideally it would be nice to call Function::bindNode() right here
  // to bind the children (inputs) to the function, but that would 
  // invoke UDFunction::synthType() which requires the outputs to be
  // setup. Instead we will make sure the inputs are bound by the call
  // to setInOrOutParam()

  // Create item list for input arguments to be stored.
  // In the Action case we might need more, but they will be 
  // allocated when we insert into the List.
  ItemExprList * inParams = new(CmpCommon::statementHeap())
                                ItemExprList(udf->getInParamCount(), 
                                             CmpCommon::statementHeap());
    Int32 minUdfInputs = 0;
    Int32 maxUdfInputs = udf->getInParamCount();
    // Check input and output arguments from the ROUTINE_PARAMS table.
    // Get expected num of inputs by checking if an argument is optional.
    // If inputs from ROUTINE_PARAMS do not match input arguments from 
    // function, emit error.
    //
    NABoolean foundOptional = FALSE;
    for (Int32 i=0; i<maxUdfInputs; i++)
    {
      // Check if parameter specified as optional.  'i' should never
      // be out of range - based on assignment, however if it is, getColumn()
      // via NAList::operator[] will issue NAAbort().
      if (udf->getInParams()[i]->isOptional())
        foundOptional = TRUE;
      else if (foundOptional == TRUE)
      {
        // An optional argument cannot be followed by a required argument.
        *CmpCommon::diags() << DgSqlCode(-4457)
                            << DgString0(functionName_.getExternalName())
                            << DgString1("User-defined functions cannot have an optional argument followed by a required argument");
        bindWA->setErrStatus();
        return this;
      }
      // If input not optional, then it is required, so increment minimum expected inputs.
      else
        minUdfInputs++;
    }

    // Make sure that outputs are not optional.
    for (Int32 i=0; i<udf->getOutParamCount(); i++)
    {
      // Check to make sure output not spec'd as optional.  'i' should never
      // be out of range - based on assignment, however if it is, getColumn()
      // via NAList::operator[] will issue NAAbort().
      if (udf->getOutParams()[i]->isOptional())
      {
        // An optional argument MUST be an input.
        *CmpCommon::diags() << DgSqlCode(-4457)
                            << DgString0(functionName_.getExternalName())
                            << DgString1("User-defined functions cannot have an optional argument defined as an output");
        bindWA->setErrStatus();
        return this;
      }
    }

  // IS req 7.2. If this is not a UUDR, then check that inputs = expected.
  // Cannot use the rdesc to check for UUDF as the action hasn't been 
  // created yet.  For universal functions, do not check inputs/outputs
  // from the metadata - we rely only on action metadata info.
  if (!udf->isUniversal())
  {
    // Non-deterministic UDFs not allowed in the query expression of a 
    // CREATE VIEW statement with CHECK OPTION.  Note that this check is 
    // somewhat redundant since a CHECK OPTION must be used with an updatable
    // view, but a UDF cannot be part of an updatable view.  However, this
    // latter condition is only found at execution time.  Action determinism
    // is checked later.
    if (!udf->isDeterministic() &&
        bindWA->getUsageParseNodePtr() &&
        bindWA->getUsageParseNodePtr()->getOperatorType() == DDL_CREATE_VIEW)
    {
      StmtDDLCreateView *view = (StmtDDLCreateView *) bindWA->getUsageParseNodePtr();
      if (view->isWithCheckOptionSpecified())
      {
        *CmpCommon::diags() << DgSqlCode(-4467)
                            << DgString0(functionName_.getExternalName());
        bindWA->setErrStatus();
        return this;
      }
    }

    // IS req 8: Multi-valued (MV) UDFs are not allowed in TRANSPOSE clause.
    if (curContext->inTransposeClause() &&  
        udf->getOutParamCount() > 1 )
    {
      *CmpCommon::diags() << DgSqlCode(-4468)
                          << DgString0(functionName_.getExternalName());
      bindWA->setErrStatus();
      return this;
    }

    // IS req 7: Process information from the ROUTINE_PARAMS, 
    // REPLICAS, and TEXT metadata tables.

    // Set up routine params in routine descriptor.
    for (Int32 i=0; i<udf->getInParamCount(); i++)
    {
      // This will update the input params lists in the rdesc to 
      // reflect those of the UDF.
      if (rdesc->createRoutineParam(bindWA, i, udf, 
                                    &(rdesc->getUDFInParamColumnList()), 
                                    &(rdesc->getUDFOutputColumnList()) ) == FALSE)
      {
        bindWA->setErrStatus();
        return FALSE;
      }
    }

    // Bind all the children, so we will know their output degree.
    for (Int32 i=0; i<getNumChildren(); i++)
    {
      child(i).getPtr()->bindNode(bindWA);  // If 'i' out of range, will CMPASSERT.
      if (bindWA->errStatus()) return FALSE;
    }
    // Process UDF input arguments.  If an input argument is a subquery or UDF,
    // it may have multiple outputs.  In that case, we add to the inputs of the
    // UDF here by creating ValueIdProxy class for each added argument.
    Int32 numInputs = 0;
    for (Int32 i=0; i<getNumChildren(); i++)
    {
      ItemExpr *cmdArg = child(i).getPtr(); // If 'i' out of range, will CMPASSERT.

      // Function arguments are in descending order.
      Int32 childOutputDegree = child(i).getPtr()->getOutputDegree();
      for (Int32 j=0; j<childOutputDegree; j++)
      {
        if (childOutputDegree == 1) // The argument only represents one input.
          //setInOrOutParam(rdesc, cmdArg, numInputs, COM_INPUT_COLUMN, bindWA); 
          setInOrOutParam(rdesc, cmdArg, numInputs, COM_INPUT_COLUMN, inParams, bindWA); 
        else
        {
          // We create a ValueIdProxy for each element in the subquery's
          // select list or for each output parameter of a multi-valued UDF. 
          // The first one of these will be marked to be transformed. This 
          // allows us to get the correct degree of statements containing 
          // MV UDFs or subqueries with degree > 1 at bind time. 
          ValueIdProxy *proxyOutput = 
            new (CmpCommon::statementHeap())
                 ValueIdProxy(cmdArg->getValueId(), 
                              cmdArg->getOutputItem(j)->getValueId(),
                              j, j==0 /* transform first derived child */ );
          //setInOrOutParam(rdesc, proxyOutput, numInputs+j,
          setInOrOutParam(rdesc, proxyOutput, numInputs+j,
                          COM_INPUT_COLUMN, inParams, bindWA);
        }
        
        if (bindWA->errStatus()) return this;
      }
      numInputs += childOutputDegree;
    }

    // Check that number of inputs to UDF is correct.
    if (numInputs < minUdfInputs || 
        numInputs > maxUdfInputs)
    {
      *CmpCommon::diags() << DgSqlCode(-4452)
                          << DgString0(functionName_.getExternalName())
                          << DgInt0(minUdfInputs)
                          << DgInt1(getNumChildren());
      bindWA->setErrStatus();
      return this;
    }

    // IS req 7.2. Check that there is at least one output parameter
    // specified by metadata.
    if (udf->getOutParamCount() == 0)
    {
      *CmpCommon::diags() << DgSqlCode(-4457)
                          << DgString0(functionName_.getExternalName())
                          << DgString1("User-defined functions must have at least one registered output value");
      bindWA->setErrStatus();
      return this;
    }
    else 
    {
        // Check output arguments against information in the COLS table.
       for (Int32 i=0; i<udf->getOutParamCount(); i++)
       {   
         // This will update the output params lists in the rdesc to 
         // reflect those of the UDF.
         if (rdesc->createRoutineParam(bindWA, i+maxUdfInputs, udf, 
                                       &(rdesc->getUDFInParamColumnList()), 
                                       &(rdesc->getUDFOutputColumnList())) == FALSE)
         {
           bindWA->setErrStatus();
           return this;
         }
         // Function arguments are in descending order.
         setInOrOutParam(rdesc, rdesc->getOutputColumnList()[i].getItemExpr(), 
                         i, COM_OUTPUT_COLUMN, inParams, bindWA); 
         if (bindWA->errStatus()) return this;
       }
    }
  } // end if (!udf->isUniversal() 


  //
  // Proceed to save the UDF referenced name.
  // Obtain the UDF usages list.
  // But, first check to see if this UDF reference already exists in the UDF Info List.
  //
 
  if (bindWA->getUsageParseNodePtr())
  {
    LIST(OptUDFInfo *) *udfList = NULL;
    
    switch (bindWA->getUsageParseNodePtr()->getOperatorType())
    {
      case DDL_CREATE_TRIGGER:
      {
        udfList = &bindWA->getUDFList();
    	break;
      }
      case DDL_CREATE_VIEW:
      {
        // Set the UDF list for view processing
        StmtDDLCreateView *view = (StmtDDLCreateView *) bindWA->getUsageParseNodePtr();
        CMPASSERT(view);
        udfList = &view->getUDFList();
    	break;
      }
      case DDL_CREATE_MV:
      {
        // Set the UDF list for materialized view processing
        StmtDDLCreateMV *mv = (StmtDDLCreateMV *) bindWA->getUsageParseNodePtr();
        CMPASSERT(mv);
        udfList = &mv->getUDFList();
    	break;
      }
      default:
    	break;
    }
    
    if (udfList != NULL)
    {
      OptUDFInfo *udfInfo = NULL;
      bool isUDFReferenced = false;
      ULng32 numUdfs = udfList->entries();
      
      for (ULng32 udfIndex = 0; udfIndex < numUdfs; udfIndex++)
        if ((*udfList)[udfIndex]->getUDFExternalName().compareTo(functionName_.getExternalName()) == 0)
        {
          isUDFReferenced = true;
          break;
        }

      // If this is a new UDF reference, create and save on list
      if (!isUDFReferenced)
      {
        OptUDFInfo *udfInfo = new (bindWA->wHeap()) OptUDFInfo(udf->getRoutineID(), 
                                                               functionName_, 
                                                               bindWA->wHeap());
        udfList->insert(udfInfo);
      }
    }  
  }

  // Since our inParams list now accurately reflects our real inputs,
  // we need to update the children's array as it no longer reflects 
  // reality. For example a UDF with a T.* input, would at this point
  // have the real columns that will be passed to the UDF at runtime in
  // the inParams list, but the children array only contains the T.* entry.
  // Since T.* never gets bound, it causes trouble if anyone tries to get 
  // the valueId or type from that child later on, so we get rid of it.

  // At this point the children represents the parameters that will
  // be passed to the actual function during execution.
  // inputVars_ contains the uncasted inputs that nodes above us in 
  // the tree can produce..

  children().clear();


  for (UInt32 paramPos=0; paramPos<inParams->entries(); paramPos++)
    {
  
      children().insertAt(paramPos, inParams->at(paramPos));
    }

  // Save off a copy of the chilren array 
  ARRAY(ExprValueId) copyOfChildren(HEAP);
  for (Int32 i=0; i< getArity(); i++)
    copyOfChildren.insertAt(i, child(i));

  ItemExpr *retExpr = Function::bindNode(bindWA);

  // Now check to see if Function::bindNode() made any changes to the children
  // if so we need to update our inputVars array.

  // Make sure the number of parameters didn't change
  CCMPASSERT( copyOfChildren.entries() == getArity() );

  for (Int32 i=0; i < getArity(); i++)
  {
    // If Function::bindNode() changed the child, update our inputVars 
    if (child(i)->castToItemExpr() != copyOfChildren[i]->castToItemExpr())
    {
      // If the new child still contains the old valueId, we don't need to
      // do anything, else we have to remove the original child value from
      // the inputVars and add in the new one. If the new one is a  CAST,
      // we are going to use the child of the CAST as the input value that
      // will be used as characteristic input for the IsolatedScalarUDF.
      if (! child(i)->castToItemExpr()->referencesTheGivenValue(
                    copyOfChildren[i]->getValueId()), TRUE)
      {
        if (inputVars_.contains(copyOfChildren[i]->getValueId()))
          inputVars_ -= copyOfChildren[i]->getValueId();
        else if ((copyOfChildren[i]->getOperatorType() == ITM_CAST) &&
                 (inputVars_.contains(copyOfChildren[i]->child(0)->getValueId())))
          inputVars_ -= copyOfChildren[i]->child(0)->getValueId();
        else
          CCMPASSERT(0); // do we have a multilevel CAST??

        if ((child(i)->getOperatorType() == ITM_CAST) ) 
          inputVars_ +=  child(i)->child(0)->getValueId();
        else
          inputVars_ +=  child(i)->getValueId();
      }
    }
  }
    
  curContext->inUDFunction() = FALSE;
 
  // add the routine to the UdrStoiList.  The UdrStoi list is used
  // to check valid privileges
  LIST(OptUdrOpenInfo *) udrList = bindWA->getUdrStoiList ();

  // See if UDF already exists
  NABoolean udrReferenced = FALSE;
  for (ULng32 stoiIndex = 0; stoiIndex < udrList.entries(); stoiIndex++)
  {
    if ( 0 ==
         udrList[stoiIndex]->getUdrName().compareTo(
                           udf->getSqlName().getQualifiedNameAsAnsiString()
                                                  )
       )
    {
      udrReferenced = TRUE;
      break;
    }
  }

  // UDF has not been defined, go ahead an add one
  if ( FALSE == udrReferenced )
  {
    SqlTableOpenInfo *udrStoi = new (bindWA->wHeap ())SqlTableOpenInfo ();
    udrStoi->setAnsiName ( convertNAString(
                           udf->getSqlName().getQualifiedNameAsAnsiString(),
                           bindWA->wHeap ())
                         );

    OptUdrOpenInfo *udrOpenInfo = new (bindWA->wHeap ())
      OptUdrOpenInfo( udrStoi
                    , udf->getSqlName().getQualifiedNameAsAnsiString()
                    , udf
                    );
    bindWA->getUdrStoiList().insert(udrOpenInfo);
  }

  delete inParams; // clean up the memory used 

  return retExpr;
} // UDFunction::bindNode()

void
UDFunction::setInOrOutParam (RoutineDesc *routine,
                             ItemExpr *argument,
                             Int32 position,
                             ComColumnDirection paramMode,
                             ItemExprList *inParams,
                             BindWA *bindWA)
{
  // This function is based on CallSP::setInOrOutParam().
  CollIndex ordinalPosition = position;

  if (argument == NULL)
  {
     *CmpCommon::diags() << DgSqlCode(-4457)
                         << DgString0(functionName_.getExternalName())
                         << DgString1("Internal error in setInOrOutParam()");
      bindWA->setErrStatus();
      return;
  } // if argument == NULL


  // Obtain the type of the function argument by binding it.
  ItemExpr *boundExpr = argument->bindNode(bindWA);
  if (bindWA->errStatus()) return;

  // If this is supposed to be an input, make checks.
  if (COM_INPUT_COLUMN == paramMode)
  {
     if (routine == NULL)
     {
       *CmpCommon::diags() << DgSqlCode(-4457)
                           << DgString0(functionName_.getExternalName())
                           << DgString1("Internal error in setInOrOutParam(): routine is NULL.");
        bindWA->setErrStatus();
        return;
     } // if routine == NULL

     const ValueIdList &formalParams = routine->getInParamColumnList();
     if (ordinalPosition >= formalParams.entries())
     {
       *CmpCommon::diags() << DgSqlCode(-4457)
                           << DgString0(functionName_.getExternalName())
                           << DgString1("Internal error in setInOrOutParam(): index position out of range.");
        bindWA->setErrStatus();
        return;
     } // if routine == NULL

     // Obtain the type that was read from metadata.
     const ValueId column = formalParams[ordinalPosition];
     const NAType &paramType = column.getType();

     ValueId inputTypeId = boundExpr->getValueId();

     // If function argument is character type, get detailed info.
     if (inputTypeId.getType().getTypeQualifier() == NA_CHARACTER_TYPE)
     {
       const CharType* stringLiteral = (CharType*)&(inputTypeId.getType());

       if(CmpCommon::wantCharSetInference())
       {
         const CharType* desiredType =
           CharType::findPushDownCharType(((CharType&)paramType).getCharSet(),
                                       stringLiteral, 0);
         if ( desiredType )
           inputTypeId.coerceType((NAType&)*desiredType, NA_CHARACTER_TYPE);
       }
     }
     // Get final type of function argument.
     const NAType &inputType = inputTypeId.getType();

   
    // Do type checking,
    // If it is not a compatible type report an error
    if (!( NA_UNKNOWN_TYPE == inputType.getTypeQualifier() ||
           paramType.isCompatible(inputType) ||
           boundExpr->getOperatorType() == ITM_DYN_PARAM
          )
      )
    {
      // Error, data types dont match
      if (!routine->isUUDFRoutine())
        // Create error for user-defined function.
        *CmpCommon::diags() << DgSqlCode(-4455)
                            << DgInt0((Lng32) ordinalPosition+1)
                            << DgString0(functionName_.getExternalName())
                            << DgString1(inputType.getTypeSQLname(TRUE))
                            << DgString2(paramType.getTypeSQLname(TRUE));
      else
      {
        // Create error for action.
        NAString actionName = "UNKNOWN";
        if (routine->getActionNARoutine() &&
            routine->getActionNARoutine()->getActionName())
          actionName = routine->getActionNARoutine()->getActionName()->data();
        *CmpCommon::diags() << DgSqlCode(-4456)
                            << DgInt0((Lng32) ordinalPosition+1)
                            << DgString0(actionName.data())
                            << DgString1(functionName_.getExternalName())
                            << DgString2(inputType.getTypeSQLname(TRUE))
                            << DgString3(paramType.getTypeSQLname(TRUE));
      }
      bindWA->setErrStatus();
      return;
    } // if NOT isCompatible

    // Store valueId in list.
    inputVars_ += boundExpr->getValueId();

    if (!hasSubquery_)
      hasSubquery_ = boundExpr->containsSubquery();
    
    if ( ! ( paramType == inputType) )
    {
      // Create a Cast node to cast the argument from the function call
      // to the type specified by the metadata (if the two are compatible.)
      Cast *castExpr = new (bindWA->wHeap()) Cast(boundExpr, &paramType,
                                                ITM_CAST, TRUE);
      ItemExpr *boundCast = castExpr->bindNode(bindWA);
      if (bindWA->errStatus()) return;
      // Add to input ItemExpr list.
      inParams->insert(boundCast);
    } 
    else
    {
      // Add to input ItemExpr list.
      inParams->insert(boundExpr);
    }

  } else if (COM_OUTPUT_COLUMN == paramMode)
  {
    outputVars_.insert (boundExpr->getValueId ());
  } // if OUTPUT

} // UDFunction::setInOrOutParam()

// -----------------------------------------------------------------------
// member functions for class ValueIdUnion
// -----------------------------------------------------------------------

ItemExpr *ValueIdUnion::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();

  if ((CmpCommon::getDefault(ALLOW_INCOMPATIBLE_OPERATIONS) == DF_ON) &&
      (isTrueUnion()) &&
      (entries() == 2))
    {
      // allow CHAR || NUMERIC, NUMERIC || CHAR
      const NAType &type1 = getLeftSource().getType();
      const NAType &type2 = getRightSource().getType();

      Int32 srcChildIndex = -1;
      Int32 otherChildIndex = -1;
      Int32 convType = -1;
      if ((type1.getTypeQualifier() == NA_NUMERIC_TYPE) &&
	  (DFS2REC::isCharacterString(type2.getFSDatatype())))
	{
	  // convert leftSource(NUMERIC) to char type
	  srcChildIndex = 0;
	  otherChildIndex = 1;
	  if ((getSource(otherChildIndex).getItemExpr()->getOperatorType() == 
	       ITM_DATEFORMAT) &&
	      (((DateFormat*)(getSource(otherChildIndex).getItemExpr()))->getDateFormat() ==
	       DateFormat::TIME_FORMAT_STR))
	    {
	      // in special1 mode, if one side of a union is numeric and
	      // other side is FORMAT clause, then the numeric needs to
	      // be formatted as well.
	      convType = 1;
	    }
	  else
	    convType = 2;
	}
      else if ((type1.getTypeQualifier() == NA_CHARACTER_TYPE) &&
	       (type2.getTypeQualifier() == NA_NUMERIC_TYPE))
	{
	  // convert rightSource(NUMERIC) to char type
	  srcChildIndex = 1;
	  otherChildIndex = 0;

	  if ((getSource(otherChildIndex).getItemExpr()->getOperatorType() == 
	       ITM_DATEFORMAT) &&
	      (((DateFormat*)(getSource(otherChildIndex).getItemExpr()))->getDateFormat() ==
	       DateFormat::TIME_FORMAT_STR))
	    {
	      // in special1 mode, if one side of a union is numeric and
	      // other side is FORMAT clause, then the numeric needs to
	      // be formatted as well.
	      convType = 1;
	    }
	  else
	    convType = 2;
	}
      
      if (srcChildIndex >= 0)
	{
	  ItemExpr * newChild = NULL;
	  if (convType == 1)
	    {
	      DateFormat * df =
		((DateFormat*)getSource(otherChildIndex).getItemExpr());
	      ConstValue * cv = (ConstValue*)(df->child(1)->castToItemExpr());
	      newChild =
		new (bindWA->wHeap())
		Format(getSource(srcChildIndex).getItemExpr(),
		       NAString((char*)(cv->getConstValue()), 
				cv->getStorageSize()),
		       FALSE);
	      
	    }
	  else if (convType == 2)
	    {
	      Lng32 dLen = 0;

	      NumericType &nType = (NumericType&)
		getSource(srcChildIndex).getType();
	      dLen =
		nType.getDisplayLength(nType.getFSDatatype(),
				       nType.getNominalSize(),
				       nType.getPrecision(),
				       nType.getScale(),
				       0);
	      
	      newChild =
		new (bindWA->wHeap())
		Cast(getSource(srcChildIndex).getItemExpr(),
		     new (bindWA->wHeap())
		     SQLChar(bindWA->wHeap(), dLen,
			     getSource(srcChildIndex).getType().
			     supportsSQLnull()));
	    }

	  newChild = newChild->bindNode(bindWA);
	  if (bindWA->errStatus())
	    return this;

	  setSource(srcChildIndex, newChild->getValueId());
	}
    }

  // ValueIdUnion is a direct derived subclass of ItemExpr; 
  // safe to invoke this
  ItemExpr *boundExpr = ItemExpr::bindNode(bindWA);
  if (bindWA->errStatus())
    return boundExpr;
  setResult(getValueId());
  return getValueId().getItemExpr();
} // ValueIdUnion::bindNode()

//
// This method is part of the Implicit Casting And Translation feature
// which allows the user to UNION a
// column of any supported character set with a column of any other 
// supported character set, providing that one CS can be translated
// to the other without errors by throwing in a Translate node to
// translate to the most general character set.
//
ItemExpr * ValueIdUnion::tryToDoImplicitCasting( BindWA *bindWA )
{
  const NAType *entryType ;
  CharInfo::CharSet savedMostGeneral_CS = CharInfo::UnknownCharSet;
  Int32 savedMaxLen = 0;
  NABoolean savedAllowNull = FALSE;
  NABoolean savedUpShifted = TRUE;
  NABoolean savedCaseInsensitive = TRUE;
  CharInfo::Collation savedCollation = CharInfo::DefaultCollation;

  enum {iUCS2 = 0, iISO = 1, iUTF8 = 2, iSJIS = 3, iUNK = 4};
  Int32 charsets_involved[5] = { 0, 0, 0, 0, 0 };
  CollIndex i = 0;

  //
  // Count the number of entries for each supported character set.
  // If we don't have at least two different ones, we just return.
  //
  for (i = 0; i < entries(); i++) {
     entryType = &getSource(i).getType();
     if (entryType->getTypeQualifier() == NA_CHARACTER_TYPE)
     {
        const CharType * ctI = (CharType *) entryType;
        Int16 cur_chld_cs_ndx = iUNK;

        Int16 maxLen = ctI->getMaxLenInBytesOrNAWChars();
        if ( savedMaxLen < maxLen )
             savedMaxLen = maxLen ;
        if ( ctI->supportsSQLnullLogical() )
           savedAllowNull = TRUE;
        if ( ! ctI->isUpshifted() )
           savedUpShifted = FALSE;
        if ( ! ctI->isCaseinsensitive() )
           savedCaseInsensitive = FALSE;

        switch ( ctI->getCharSet() )
        {
         case CharInfo::UNICODE :
           cur_chld_cs_ndx = iUCS2;
           if ( savedMostGeneral_CS != CharInfo::UNICODE )
           {
                savedMostGeneral_CS = CharInfo::UCS2;
                savedCollation      = ctI->getCollation();
           }
           break;
         case CharInfo::UTF8:
           cur_chld_cs_ndx = iUTF8;
           if ( ( savedMostGeneral_CS == CharInfo::UnknownCharSet ) ||
                ( savedMostGeneral_CS == CharInfo::SJIS           ) ||
                ( savedMostGeneral_CS == CharInfo::ISO88591       ) )
           {
                savedMostGeneral_CS = CharInfo::UTF8;
                savedCollation      = ctI->getCollation();
           }
           break;
         case CharInfo::SJIS:
           cur_chld_cs_ndx = iSJIS;
           if ( ( savedMostGeneral_CS == CharInfo::UnknownCharSet ) ||
                ( savedMostGeneral_CS == CharInfo::ISO88591 ) )
           {
                savedMostGeneral_CS = CharInfo::SJIS;
                savedCollation      = ctI->getCollation();
           }
           break;
         case CharInfo::ISO88591:
           cur_chld_cs_ndx = iISO;
           if ( savedMostGeneral_CS == CharInfo::UnknownCharSet )
           {
                savedMostGeneral_CS = CharInfo::ISO88591;
                savedCollation      = ctI->getCollation();
           }
           break;
         //case CharInfo::KANJI_MP:
         //case CharInfo::KSC5601_MP:
         default:
           break; // Can not translate these currently.
        }
        charsets_involved[cur_chld_cs_ndx]++;
     }
  }

  Int32 charsetsCount = 0;
  for (Int32 j = 0; j < iUNK; j++)
    if (charsets_involved[j] > 0)
      charsetsCount++;
  if ( charsetsCount <= 1) return this;
  CMPASSERT( savedMostGeneral_CS != CharInfo::UnknownCharSet ); // Shouldn't happen since charsetsCount > 1
  CMPASSERT( getValueId() == NULL_VALUE_ID ); // call this before assigning a value id

  CharType * MostGeneralType = new( bindWA->wHeap() )
                                   SQLVarChar(bindWA->wHeap(), savedMaxLen,
                                              savedAllowNull,
                                              savedUpShifted,
                                              savedCaseInsensitive,
                                              savedMostGeneral_CS,
                                              savedCollation,
                                              CharInfo::COERCIBLE,
                                              savedMostGeneral_CS
                                             );
  //
  // OK, we have entries for at least two different character sets.
  // So, we must insert some Translate nodes.  Just to keep it
  // simple, we always translate to the Most General charset
  //
  for (i = 0; i < entries(); i++) {
    entryType = &getSource(i).getType();
    if ( entryType->getTypeQualifier() == NA_CHARACTER_TYPE )
    {
        const CharType* ctI = (CharType *) entryType;
        if ( ctI->getCharSet() != savedMostGeneral_CS )
        {
           // Do Implicit Cast to the Most General Character Set
           //
           Int32 iTranslateFromTo = find_translate_type( ctI->getCharSet(),
                                                         savedMostGeneral_CS );
           if ( iTranslateFromTo == Translate::UNKNOWN_TRANSLATION )
              continue; // Just skip this entry!

           ValueId vidi = getSource(i);
           ItemExpr * newTranslateChild = NULL;
           ItemExpr * ieChild = vidi.getItemExpr();

           if ( (ieChild->getOperatorType() == ITM_CONSTANT)  &&
                ((ConstValue *)ieChild)->isNull() )  // If NULL, create new child with needed charset
           {   
              newTranslateChild = new(bindWA->wHeap()) ConstValue();
              newTranslateChild->bindNode(bindWA);
              (newTranslateChild->getValueId()).changeType(MostGeneralType);
           }   
           else // Must insert a Translate node
           {   
              newTranslateChild =
                 new (bindWA->wHeap()) Translate(vidi.getItemExpr(), iTranslateFromTo );

              newTranslateChild = newTranslateChild->bindNode(bindWA);
              if (bindWA->errStatus())
                 return this;
           }   

           changeSource(i, ValueDesc::create(newTranslateChild,
                                             MostGeneralType, bindWA->wHeap()) );
        }
     }
  }

  return this;
}

// This method returns the length of the padding string (the third argument to
// an LPAD or RPAD function, for e.g. the '00' in LPAD(<colname>, max-len, '00'))
// if the padding string is not a varchar. The method returns -2 is there is an error
// and -1 if the length is a varchar or for some other reason the length cannot
// be determined.
Int32 ZZZBinderFunction::getPadStringLength (ItemExpr* padExpr, BindWA* bindWA)
{
  Int32 padStringLength = -1;
  if (padExpr)
  {
    ItemExpr * tempPadString =
      padExpr->castToItemExpr()->bindNode(bindWA);
    if (bindWA->errStatus()) return -2;

    const NAType &typ1 = tempPadString->getValueId().getType();
    if ((typ1.getTypeQualifier() == NA_CHARACTER_TYPE) &&
        (!typ1.isVaryingLen())) {
          const CharType &ctyp1 = (CharType &) typ1;
          padStringLength = typ1.getNominalSize()/ctyp1.getBytesPerChar() ;
        }
  }
  return padStringLength ;
}

// This method returns the maximum length (the second argument to
// an LPAD or RPAD function, for e.g. the <max-len> in LPAD(<colname>, <max-len>, '00'))
// if the second argument is a constant that is not null. 
// The method returns -2 is there is an error
// and -1 if max-len is an expression or null.
Int32 ZZZBinderFunction::getPadLength (ItemExpr* padLengthExpr, BindWA* bindWA)
{
  Int32 padLengthMax = -1;
  if (padLengthExpr)
  {
    ItemExpr * tempPadLength = padLengthExpr->bindNode(bindWA);
    if (bindWA->errStatus()) return -2;

    NumericType &tempType = (NumericType&)tempPadLength->getValueId().getType();
    if(! (tempType.isExact() && tempType.getScale() <=0 ))
      {
	*CmpCommon::diags() << DgSqlCode(-4047);
	bindWA->setErrStatus();
	return -2;
      }

     NABoolean negate;
    if ((tempPadLength->getOperatorType() == ITM_CONSTANT) &&
        (tempPadLength->castToConstValue(negate)))
    {
      ConstValue * cv = tempPadLength->castToConstValue(negate);
      if (! cv->isNull())
      {
	if (cv->canGetExactNumericValue())
	{ 
	   padLengthMax = (Int32) cv->getExactNumericValue();
           if ( padLengthMax > CmpCommon::getDefaultNumeric(TRAF_MAX_CHARACTER_COL_LENGTH))
             {
               *CmpCommon::diags() << DgSqlCode(-4129)
                                   << DgString0(getTextUpper());
               bindWA->setErrStatus();
               return -2;
             } 
        }
      }
    }  // padLength is a constant
  }
  return padLengthMax ;
}

NABoolean ZZZBinderFunction::isPadWithSpace (ExprValueId& padExpr, CharInfo::CharSet cs)
{

  if ((cs != CharInfo::UNICODE)&&(cs != CharInfo::ISO88591))
  {
    return FALSE;  // the optimization to use CAST for RPAD is done
    // only for these two charsets.
  }

  if (padExpr == NULL) // if this expression is null then we pad with blank space
    return TRUE ;

  CharInfo::CharSet padExprCS = CharInfo::UnknownCharSet;
  if(!padExpr->castToItemExpr()->nodeIsBound())
  {
    CCMPASSERT(FALSE);
    return FALSE;
  }
  const NAType& padExprType = padExpr->castToItemExpr()->getValueId().getType();
  if ( padExprType.getTypeQualifier() == NA_CHARACTER_TYPE )
    padExprCS = ((CharType&) padExprType).getCharSet();
  if (padExprCS != cs)
  {
    if ( CmpCommon::getDefault(ALLOW_IMPLICIT_CHAR_CASTING) == DF_OFF )
      return FALSE; // if charset of arg1 and arg3 are different then do not
      // the RPAD to CAST optimization. We let existing code handle the error path.
  }

  if ((padExpr->castToItemExpr()->getOperatorType() == ITM_CONSTANT) &&
      (!((ConstValue *)padExpr->castToItemExpr())->getText().isNull()))
  {
     NAString padString(
       ((ConstValue *)padExpr->castToItemExpr())->getConstStr(FALSE));
     Int32 i = 0;
     NABoolean foundSingleQuote = FALSE;
     for (const char *s = padString.data(); *s; s++)
     {
       i++;
       if ((!foundSingleQuote)&&(*s != '\'')) // loop through 
         continue;  // the prefix _UCS2 or _ISO88591
       else if ((!foundSingleQuote))
       {
        foundSingleQuote = TRUE; // found the leading single quote.
        continue;
       }
       if ((i == (padString.length())) && (*s == '\'')) // trailing single quote
         continue;
       if (*s != ' ')
         return FALSE;
     }
     return foundSingleQuote;
  }
  return FALSE ;

}

// -----------------------------------------------------------------------
// member functions for class ZZZBinderFunction
// -----------------------------------------------------------------------

ItemExpr *ZZZBinderFunction::bindNode(BindWA *bindWA)
{
  Parser parser(bindWA->currentCmpContext());
  char buf[2500];
  buf[0] = 0;
  NABoolean resetRealBigNum = FALSE;

  ItemExpr *parseTree = NULL, *boundTree = NULL;

    // Need to check to see if we have any parameters that are MVFs or 
    // subqueries of degree > 1.
    // 
    // Some functions like LPAD and RPAD can take 2 or 3 parameters.
    // Here we can choose to if we are given the 2 parameter version, but
    // one of those parameters is a subquer of degree 2 or an MVF that returns
    // 2 outputs.
    
    Lng32 childDegree  = 0;
    Lng32 origArity = getArity();
    
    for (Lng32 idx = 0; idx < origArity; idx++) 
    {
      // Break when we have no more parameters
      // This can happen since we have some function with optional params.
      // getArity() may sometime return the maximum allowed, instead of actual
      // This code will also only allow that expansion to happen if the
      // subquery or MVF is the last given parameter.
      if (child(idx) == NULL) break;
      
      ItemExpr *chldExpr = child(idx)->castToItemExpr();
      switch (chldExpr->getOperatorType()) 
      {
        case ITM_ROW_SUBQUERY:
        case ITM_USER_DEF_FUNCTION:
        {
          // Need to bind the subquery to find its degree
          child(idx) = child(idx)->castToItemExpr()->bindNode(bindWA);
          if (bindWA->errStatus()) return this;
          
          Subquery *subq = (Subquery *) child(idx)->castToItemExpr();
          UDFunction * udf = (UDFunction *) child(idx)->castToItemExpr();
          Lng32 myDegree = (chldExpr->getOperatorType() == ITM_ROW_SUBQUERY) ? 
            subq->getSubquery()->getDegree() :
            udf->getRoutineDesc()->getOutputColumnList().entries(); 
          
          childDegree += myDegree;
          
          // Only allow expansion for LPAD/RPAD
          // for the two parameter case
          if ( myDegree > 1 && 
               ((getOperatorType() == ITM_LPAD) || 
                (getOperatorType() == ITM_RPAD)) &&
               origArity == 2)
          {
            // Expand the child() array
            // First shift any existing ones over
            for (Lng32 shiftIdx = origArity-1; shiftIdx > idx; shiftIdx--)
            {
              child(shiftIdx+myDegree) = child(shiftIdx);
            }
            
            ItemExprList *mDegreeList = (ItemExprList *) new(bindWA->wHeap())
              ItemExprList(child(idx)->castToItemExpr()
                           ,bindWA->wHeap());
            
            ItemExpr * ie = mDegreeList->convertToItemExpr();
            ie->bindNode(bindWA);
            if (bindWA->errStatus()) return this;
            
            ValueIdList mDegreeVlist;
            ie->convertToValueIdList(mDegreeVlist, bindWA, ITM_ITEM_LIST);
            
            for (Lng32 expIdx = 0; expIdx < myDegree; expIdx++)
            {
              
              child(idx + expIdx) = mDegreeVlist[expIdx].getItemExpr(); 
            }
            
            // bump the index to point to the next non processed param
            idx += myDegree-1;
            // We don't need to anything special to increase the arity
            // since the assignment inside the for loop above does it for
            // us. The BuiltinFunction::getArity() method looks at the number
            // of children in its child array.
            
          }
        }
        break;
        default:
          childDegree += 1;
          break;
      }
    }
    
    if ((childDegree > origArity ) && 
        (! ( childDegree == 3 && origArity == 2 && 
             ((getOperatorType() == ITM_LPAD) || 
              (getOperatorType() == ITM_RPAD))))
        )
    {
      // Error
      NAString upperFunc(getText(), bindWA->wHeap());
      
      upperFunc.toUpper();
      *CmpCommon::diags() << DgSqlCode(-4479) << DgString0(upperFunc) 
                          << DgInt1(origArity) << DgInt2(childDegree);
      bindWA->setErrStatus();
      return this;
    }

  // fix 10-040621-7164. Make sure that the child expression is not dynamic param
  // before verifying the type of the child. Otherwise, the verification may fail
  // prematually.
  //
  // This is because bindNode() is called before ::synthesizeType() and the
  // dynamic parameter's type will not be set until ::synthesizType() is called.
  // If we emit error here, then we miss the chance to assign a solid type to the
  // dynamic parameter child.

  //
  // We do not have to apply the "is dynamic param" check if only the nullness
  // We do not have to apply the "is dynamic param" check if only the nullness
  // attribute of the actual type is checked, since a dynamic paramete is
  // always nullable.
  //
  // This fix applies the "is dynamic param" check to the implementation of SQL function
  // LEFT() and RIGHT(). We do not do this for QUARTER() because a cast is needed
  // if a dynamic param is involved (see R2.0 Ref. Manual on MXCI, section Type
  // Assignment for Parameters).
  switch (getOperatorType())
    {
    case ITM_DATE_TRUNC_YEAR:
      {
        if (enforceDateOrTimestampDatatype(bindWA,0,2))
          return this;

        //Cast to DATETIME YEAR first to pick up only the year.
        strcpy(buf, "CAST(CAST(@A1 AS DATETIME YEAR) AS TIMESTAMP) ;");
      }
      break;

    case ITM_DATE_TRUNC_MONTH:
      {
        if (enforceDateOrTimestampDatatype(bindWA,0,2))
          return this;

        //Get first day of year and then add in the months.
        strcpy(buf, "CAST(CAST(@A1 AS DATETIME YEAR) AS TIMESTAMP) + "
                    "CAST(MONTH(@A1)-1 AS INTERVAL MONTH);");
      }
      break;

    case ITM_DATE_TRUNC_DAY:
      {
        if (enforceDateOrTimestampDatatype(bindWA,0,2))
          return this;

        //Note: Cast to DATE first to zero out all time fields
        strcpy(buf, "CAST(CAST(@A1 AS DATE) AS TIMESTAMP);");
      }
      break;

    case ITM_DATE_TRUNC_HOUR:
      {
        if (enforceDateOrTimestampDatatype(bindWA,0,2))
          return this;

        //Note: Cast to DATE to zero out all time fields.  Cast to TIMESTAMP in case DATE was supplied.
        strcpy(buf,
               "CAST( CAST(@A1 AS DATE) AS TIMESTAMP) + "
               "CAST( HOUR( CAST(@A1 AS TIMESTAMP) ) AS INTERVAL HOUR);");
      }
      break;

    case ITM_DATE_TRUNC_MINUTE:
      {
        if (enforceDateOrTimestampDatatype(bindWA,0,2))
          return this;

        strcpy(buf, "DATE_TRUNC('HOUR',@A1) + "
                    "CAST(MINUTE(CAST(@A1 AS TIMESTAMP)) AS INTERVAL MINUTE);");
      }
      break;

    case ITM_DATE_TRUNC_SECOND:
      {
        if (enforceDateOrTimestampDatatype(bindWA,0,2))
          return this;

        strcpy(buf, "DATE_TRUNC('MINUTE',@A1) + "
                    "CAST( CAST( SECOND(CAST(@A1 AS TIMESTAMP)) AS SMALLINT) "
                    "AS INTERVAL SECOND);");
      }
      break;

    case ITM_DATE_TRUNC_CENTURY:
      {
        if (enforceDateOrTimestampDatatype(bindWA,0,2)) 
          return this;

        strcpy(buf, "CAST( CAST(@A1 AS DATETIME YEAR) AS TIMESTAMP ) - "
                    "CAST( MOD(YEAR(@A1),100) AS INTERVAL YEAR(4)  );");
      }
      break;

    case ITM_DATE_TRUNC_DECADE:
      {
        if (enforceDateOrTimestampDatatype(bindWA,0,2)) 
          return this;

        strcpy(buf, "CAST( CAST(@A1 AS DATETIME YEAR) AS TIMESTAMP ) - "
                    "CAST( MOD(YEAR(@A1),10) AS INTERVAL YEAR(4)   );");
      }
      break;

    case ITM_DATEDIFF_YEAR:
    case ITM_TSI_YEAR:
      {
        if (enforceDateOrTimestampDatatype(bindWA,0,2))
          return this;
        if (enforceDateOrTimestampDatatype(bindWA,1,3))
          return this;
        strcpy(buf, "CAST( YEAR(@A2) - YEAR(@A1) AS INT) ;");
      }
      break;

    case ITM_DATEDIFF_MONTH:
    case ITM_TSI_MONTH:
      {
        if (enforceDateOrTimestampDatatype(bindWA,0,2))
          return this;
        if (enforceDateOrTimestampDatatype(bindWA,1,3))
          return this;
        strcpy(buf, "CAST( (YEAR(@A2)*12 + MONTH(@A2)) - "
                          "(YEAR(@A1)*12 + MONTH(@A1)) AS INT) ;");
      }
      break;

    case ITM_MONTHS_BETWEEN:
      {
        if (enforceDateOrTimestampDatatype(bindWA,0,1))
          return this;
        if (enforceDateOrTimestampDatatype(bindWA,1,2))
          return this;
	strcpy(buf, "CASE WHEN DAY (@A1) = DAY (@A2) THEN (YEAR(@A1)*12 + MONTH(@A1) - (YEAR(@A2)*12 + MONTH(@A2))) ELSE CAST((CAST(@A1 AS DATE) - CAST(@A2 AS DATE)) AS NUMERIC(18,6))/31 END");
      }
      break;

    case ITM_DATEDIFF_DAY:
    case ITM_TSI_DAY:
      {
        if (enforceDateOrTimestampDatatype(bindWA,0,2))
          return this;
        if (enforceDateOrTimestampDatatype(bindWA,1,3))
          return this;
        strcpy(buf, "CAST( CAST(@A2 AS DATE) - CAST(@A1 AS DATE) AS INT );");
      }
      break;

    case ITM_DATEDIFF_HOUR:
    case ITM_TSI_HOUR:
      {
        if (enforceDateOrTimestampDatatype(bindWA,0,2))
          return this;
        if (enforceDateOrTimestampDatatype(bindWA,1,3))
          return this;
        strcpy(buf,
              "CAST( (JULIANTIMESTAMP(DATE_TRUNC('HOUR',@A2)) - "
                    " JULIANTIMESTAMP(DATE_TRUNC('HOUR',@A1))) / (1000000*3600) "
                    " AS INT);");
      }
      break;

    case ITM_DATEDIFF_MINUTE:
    case ITM_TSI_MINUTE:
      {
        if (enforceDateOrTimestampDatatype(bindWA,0,2))
          return this;
        if (enforceDateOrTimestampDatatype(bindWA,1,3))
          return this;
        strcpy(buf,
               "CAST( (JULIANTIMESTAMP(DATE_TRUNC('MINUTE',@A2)) - "
                     " JULIANTIMESTAMP(DATE_TRUNC('MINUTE',@A1))) / (1000000*60)"
                     " AS INT);");
      }
      break;

    case ITM_DATEDIFF_SECOND:
    case ITM_TSI_SECOND:
      {
        if (enforceDateOrTimestampDatatype(bindWA,0,2))
          return this;
        if (enforceDateOrTimestampDatatype(bindWA,1,3))
          return this;
        strcpy(buf, "CAST( "
                    "(JULIANTIMESTAMP(DATE_TRUNC('SECOND',@A2)) - "
                    " JULIANTIMESTAMP(DATE_TRUNC('SECOND',@A1))) / 1000000"
                    " AS INT);");
      }
      break;

    case ITM_DATEDIFF_QUARTER:
    case ITM_TSI_QUARTER:
      {
        if (enforceDateOrTimestampDatatype(bindWA,0,2))
          return this;
        if (enforceDateOrTimestampDatatype(bindWA,1,3))
          return this;
        strcpy(buf, "CAST( ("
                    "((YEAR(@A2)*12) + ((QUARTER(@A2)-1)*3)) - "
                    "((YEAR(@A1)*12) + ((QUARTER(@A1)-1)*3)) ) / 3 AS INT);");
      }
      break;

    case ITM_DATEDIFF_WEEK:
    case ITM_TSI_WEEK:
      {
        if (enforceDateOrTimestampDatatype(bindWA,0,2))
          return this;
        if (enforceDateOrTimestampDatatype(bindWA,1,3))
          return this;
        strcpy(buf, "CAST(("
                    "(CAST(@A2 AS DATE) - CAST(DAYOFWEEK(@A2)-1 AS INTERVAL DAY)) - "
                    "(CAST(@A1 AS DATE) - CAST(DAYOFWEEK(@A1)-1 AS INTERVAL DAY))"
                    ") / 7 AS INT);");
      }
      break;

    case ITM_LAST_DAY:
      {
        if (enforceDateOrTimestampDatatype(bindWA,0,1))
          return this;

        strcpy(buf, "@A1 - CAST( DAY(@A1) -1 AS INTERVAL DAY) + INTERVAL '1' MONTH - INTERVAL '1' DAY;");
      }
      break;

    case ITM_NEXT_DAY:
      {
	// Make sure that child(0) is of date or timestamp datatype.
        if (enforceDateOrTimestampDatatype(bindWA,0,1))
          return this;

	// make sure child(1) is of string type
	ItemExpr * tempBoundTree =
	  child(1)->castToItemExpr()->bindNode(bindWA);
	if (bindWA->errStatus()) 
	  return this;

	if (tempBoundTree->getValueId().getType().getTypeQualifier() !=
	    NA_CHARACTER_TYPE)
	  {
	    *CmpCommon::diags() << DgSqlCode(-4116) << DgString0(getTextUpper());
	    bindWA->setErrStatus();
	    return NULL;
	  }

	strcpy(buf, "@A1 + CAST( MOD( CASE WHEN UPPER(@A2) = 'MONDAY' THEN -5 WHEN UPPER(@A2) = 'TUESDAY' THEN -4 WHEN UPPER(@A2) = 'WEDNESDAY' THEN -3 WHEN UPPER(@A2) = 'THURSDAY' THEN -2 WHEN UPPER(@A2) = 'FRIDAY' THEN -1 WHEN UPPER(@A2) = 'SATURDAY' THEN  0 WHEN UPPER(@A2) = 'SUNDAY' THEN  1 ELSE NULL END -  DAYOFWEEK(@A1), 7) + 7 AS INTERVAL DAY);");
      }
      break;

    case ITM_YEARWEEK:
      {
        // human-readable week format, 100*year + week
        strcpy(buf, "CAST( (YEAR(@A1) * 100 + WEEK(@A1)) AS NUMERIC(6,0));");
      }
      break;

    case ITM_YEARWEEKD:
      {
        // dense week format, 54*year + 0-based week
        // (see week function documentation, week value can range from 1 to 54)
        strcpy(buf, "CAST( (YEAR(@A1) * 54 + (WEEK(@A1) - 1)) AS NUMERIC(6,0));");
      }
      break;

    case ITM_COALESCE:
      {
	bindChildren(bindWA);
	if (bindWA->errStatus()) 
	  return this;

	ExprValueId eVid(child(0)->castToItemExpr());
	ItemExprTreeAsList coalesceList(&eVid, ITM_ITEM_LIST);
	
	IfThenElse * firstITE = NULL;
	IfThenElse * currITE = NULL;
	for (CollIndex i = 0; i < (CollIndex)coalesceList.entries(); i++) 
	  {
	    // Specifically prohibit the last operand from being explicit NULL
	    if ( (i+1) == (CollIndex)coalesceList.entries() ) {
	        if ( (coalesceList[i])->getOperatorType() == ITM_CONSTANT ) {
	           if ( ((ConstValue *)coalesceList[i])->isNull() ) {
	              *CmpCommon::diags() << DgSqlCode(-3416) << DgString0("COALESCE");
	              bindWA->setErrStatus();
	              return this;
	           }
	        }
	    }
	    ItemExpr * v = 
	      new(bindWA->wHeap()) UnLogic(ITM_IS_NOT_NULL, coalesceList[i]);
	    IfThenElse * ite = new(bindWA->wHeap()) IfThenElse(v, coalesceList[i], NULL);
	    if (firstITE == NULL)
	      firstITE = ite;
	    if (currITE != NULL)
	      {
		currITE->setElse(ite);
	      }
	    currITE = ite;
	  }
	currITE->setElse(new(bindWA->wHeap()) SystemLiteral());
	parseTree = new(bindWA->wHeap()) Case(NULL, firstITE);
      }
    break;

    case ITM_DAYNAME:
      {
	// find the nullability of child
	//coverity[returned_pointer]
	ItemExpr * tempBoundTree =
	  child(0)->castToItemExpr()->bindNode(bindWA);
	if (bindWA->errStatus()) return this;

	NABoolean childIsNullable = FALSE;
	if (tempBoundTree->getValueId().getType().supportsSQLnull())
	  {
	    childIsNullable = TRUE;
	  }

	if (childIsNullable)
	  strcpy(buf, "CASE WHEN @A1 IS NULL THEN NULL ELSE ");
	else
	  strcpy(buf, "");

	strcat (buf, "CASE DAYOFWEEK(@A1) WHEN 1 THEN 'Sunday' WHEN 2 THEN 'Monday' WHEN 3 THEN 'Tuesday' WHEN 4 THEN 'Wednesday' WHEN 5 THEN 'Thursday' WHEN 6 THEN 'Friday' WHEN 7 THEN 'Saturday' ELSE 'ERROR' END");

	if (childIsNullable)
	  strcat(buf, " END;");
	else
	  strcat(buf, ";");
      }
    break;

    case ITM_DECODE:
      {
	Int32 savedCurrChildNo = currChildNo();
	
	for (Int32 i = 0; i < getArity(); i++, currChildNo()++) 
	{
	  ItemExpr *boundExpr = child(i)->bindNode(bindWA);
	  child(i) = boundExpr;
	}
	
	currChildNo() = savedCurrChildNo;

	if (bindWA->errStatus()) 
	  return this;

	ExprValueId eVid(child(0)->castToItemExpr());
	ItemExprTreeAsList decodeList(&eVid, ITM_ITEM_LIST);
	
	const NAType &typ1 = eVid->getValueId().getType();
	NABoolean noNulls = FALSE;
	
	if ((getArity() == 1) && (NOT typ1.supportsSQLnull()))  {
	  noNulls = true;  // main expression cannot be null so a simplier expression can be built
	}

	IfThenElse * firstITE = NULL;
	IfThenElse * currITE = NULL;

	Int32 numExprs = decodeList.entries();
        
	if (numExprs < 3) {
	  *CmpCommon::diags() << DgSqlCode(-15001) << DgString0(")");
	  bindWA->setErrStatus();
	  return this;
	} 

	BiRelat *v;
	for (CollIndex i = 1; i < (CollIndex)numExprs-1; i++) 
	{
	  v = new(bindWA->wHeap())BiRelat(ITM_EQUAL,decodeList[0] ,decodeList[i]);
	  v->setSpecialNulls(TRUE);
	  IfThenElse * ite = new(bindWA->wHeap()) IfThenElse(v, decodeList[i+1], NULL);
	  if (firstITE == NULL)
	    firstITE = ite;
	  
	  if (currITE != NULL)
	  {
	    currITE->setElse(ite);
	  }
	  
	  currITE = ite;
	  i++;  // must increment two each loop
	}
	
	if (numExprs % 2 == 0 )		 // even number.  final argument is an else
	  currITE->setElse( decodeList[numExprs-1]);
	else
	  currITE->setElse(new(bindWA->wHeap()) SystemLiteral());
	
	parseTree = new(bindWA->wHeap()) Case(NULL, firstITE);
	parseTree->setOrigOpType(getOperatorType());
      }
    break;

    case ITM_DAYOFYEAR:
      {
	strcpy(buf, "CAST(CAST(@A1 AS DATE) - FIRSTDAYOFYEAR(@A1) AS INT) + 1;");
      }
    break;

    case ITM_FIRSTDAYOFYEAR:
      {
	strcpy(buf, "CAST(CAST(@A1 AS DATETIME YEAR) AS DATE);");
      }
    break;

    case ITM_DAYOFMONTH:
      {
	// Make sure that the child is of date datatype.
	ItemExpr * tempBoundTree =
	  child(0)->castToItemExpr()->bindNode(bindWA);
	if (bindWA->errStatus()) 
          return this;

	if (tempBoundTree->getValueId().getType().getTypeQualifier() !=
	    NA_DATETIME_TYPE)
	  {
	    // 4071 The operand of a DAYOFMONTH function must be a datetime.
	    *CmpCommon::diags() << DgSqlCode(-4071) << DgString0(getTextUpper());
	    bindWA->setErrStatus();
	    return this;
	  }

        parseTree = new(bindWA->wHeap()) 
          ExtractOdbc(REC_DATE_DAY, child(0), TRUE);
      }
      break;

    case ITM_GREATEST:
      {
	strcpy(buf, "CASE WHEN @A1 is NULL or @A2 is null then NULL WHEN @A1 > @A2 then @A1 else @A2 END;");
      }
    break;

   case ITM_LEAST:
      {
	strcpy(buf, "CASE WHEN @A1 is NULL or @A2 is null then NULL WHEN @A1 < @A2 then @A1 else @A2 END;");
      }
    break;

    case ITM_INSERT_STR:
      {
	// Make sure that the third child(length) is of unsigned numeric with
	// scale of zero.
	ItemExpr * tempBoundTree =
	  child(2)->castToItemExpr()->bindNode(bindWA);
	if (bindWA->errStatus()) return this;
        
        // fix case 10-031103-2610, soln 10-031103-0997: make sure length arg
        // in function "INSERT(s1, start, length, s2)" is not null.
        if (tempBoundTree->getOperatorType() == ITM_CONSTANT &&
            ((ConstValue *)tempBoundTree)->isNull()) {
          *CmpCommon::diags() << DgSqlCode(-4097) << DgString0("INSERT");
          bindWA->setErrStatus();
          return this;
        }
        
	//
	// Type cast any param.
	//
	SQLInt nType(FALSE);
	ValueId vid = child(2)->getValueId();
	vid.coerceType(nType, NA_NUMERIC_TYPE);

	NABoolean errorLength = FALSE;
	if (tempBoundTree->getValueId().getType().getTypeQualifier() !=
	    NA_NUMERIC_TYPE)
	  {
	    errorLength = TRUE;
	  }

	const NumericType &ntyp =
	  (NumericType &) tempBoundTree->getValueId().getType();

	if ((NOT ntyp.isExact()) || (ntyp.getScale() != 0))
	  errorLength = TRUE;

	if (errorLength)
	  {
	    *CmpCommon::diags() << DgSqlCode(-4053) << DgString0(getTextUpper());
	    bindWA->setErrStatus();
	    return this;
	  }

	// INSERT (str1(@A1), start(@A2), length(@A3), str2(@A4))
	// Returns a string where <length> characters have been deleted from
	// <str1> beginning at <start> and where <str2> has been inserted
	// into <str1>, beginning at <start>

        // make sure replacement expression handles any null args correctly
        strcpy(buf, "CASE WHEN @A1 IS NULL THEN NULL"
               " WHEN @A2 IS NULL THEN NULL"
               " WHEN @A3 IS NULL THEN NULL"
               " WHEN @A4 IS NULL THEN NULL ELSE ");

	// Get the characters before <start>
	strcat(buf, "(LEFT(@A1, CAST(@A2 AS INT UNSIGNED) - 1)");

	// append <str2>
	strcat(buf, "|| @A4 ");

	// and finally append the 'rightmost' characters from str1 after
	// skipping (start + length) characters.
	// If the number of 'rightmost' characters is a negative,
	// then insert '0' characters.
        strcat(buf, "|| CASE WHEN (CHAR_LENGTH(@A1) - (@A2 + CAST(@A3 AS INT UNSIGNED)) + 1) > 0 THEN RIGHT(@A1, CHAR_LENGTH(@A1) - (@A2 + CAST(@A3 AS INT UNSIGNED)) + 1) ELSE RIGHT(@A1, 0) END) END;");
      }
    break;

    case ITM_LPAD:
      {
        Int32 padStringLength = 0;
        if (child(2))
        {
          padStringLength = getPadStringLength(child(2)->castToItemExpr(), bindWA);
          if (bindWA->errStatus()) return this;
        }

        if (child(2) == NULL)
          {
            // pad with spaces
            // NOTE: The outer SUBSTRING operation would seem to be unnecessary
            // in some cases, but it ensures the SQL compiler always knows that
            // the maximum length of the return string is the user-specified value.

            strcpy(buf,
                  "SUBSTRING( "
                  "CASE WHEN @A1 IS NULL THEN NULL "
                  "WHEN CHAR_LENGTH(@A1) >= @A2 THEN @A1 "
                  "ELSE SUBSTRING( SPACE(@A2, _UNKNOWN_), 1, "
                                  " @A2 - CHAR_LENGTH(@A1) ) || @A1 "
                  "END, 1, @A2 ) ;");
          }
        else if (padStringLength == 1)
        {
          // when the third param has a length = 1 then the we do not need to
          // use the complicated case expression in the else branch a few lines 
          // below. When the padding string length is known to be 1 we can use
          // an case expression very similar to the situation when child(2) is null.

          strcpy(buf,
                  "SUBSTRING( "
                  "CASE WHEN @A1 IS NULL THEN NULL "
                  "WHEN CHAR_LENGTH(@A1) >= @A2 THEN @A1 "
                  "ELSE SUBSTRING( "
                    "REPEAT(@A3, @A2), 1, @A2 - CHAR_LENGTH(@A1)) || @A1 "
                  "END, 1, @A2 ) ;");
        }
        else 
        {
          // pad with the third param.
          Int32 lpadLength = getPadLength(child(1)->castToItemExpr(), bindWA);
          if (bindWA->errStatus()) return this;

          // If result size needed is less than some threshold, use fast path
          if ((lpadLength > 0) && (lpadLength <= 1024)) {
            strcpy(buf,
                  "SUBSTRING( "
                  "CASE WHEN @A1 IS NULL OR @A3 IS NULL THEN NULL "
                  "WHEN CHAR_LENGTH(@A1) >= @A2 THEN @A1 "
                  "ELSE SUBSTRING( REPEAT(@A3, @A2), 1, "
                    "@A2 - CHAR_LENGTH(@A1)) || @A1 "
                  "END, 1, @A2 );");
          }
          else
          {
            // NOTES:
            // (1) The outer SUBSTRING operation would seem to be unnecessary
            //     in some cases, but it ensures the SQL compiler always knows
            //     that the maximum length of the return string is the
            //     user-specified value (if the user specified a constant.)
            // (2) The 2nd WHEN prevents a negative REPEAT count and is
            //     also required by definition of this padding function.
            // (3) The 3rd WHEN prevents division by 0 later on and also
            //     ensures we catch overly large count (@A2) values at compile
            //     time if possible ... by sending @A2 directly to REPEAT
            //     without doing any CASTing or arithmetic on it (because
            //     doing those causes the value of @A2 to be resolved at
            //     runtime instead of compile time.)

            strcpy(buf,
                  "SUBSTRING( "
                  "CASE WHEN @A1 IS NULL OR @A3 IS NULL THEN NULL "
                  "WHEN CHAR_LENGTH(@A1) >= @A2 THEN @A1 "
                  "WHEN CHAR_LENGTH(@A3) <= 1 THEN "
                  "SUBSTRING( REPEAT(@A3, @A2), 1, "
                    "@A2 - CHAR_LENGTH(@A1)) || @A1 "
                  "ELSE SUBSTRING( "
                        "REPEAT(@A3,"
                              "CAST((@A2 - CHAR_LENGTH(@A1))/CHAR_LENGTH(@A3) "
                                "+ 1 AS INT)"
                              "), "
                        " 1, "
                        " @A2 - CHAR_LENGTH(@A1) ) || @A1 "
                  "END, 1, @A2 );");

            if (lpadLength > -1)
            {
              parseTree = parser.getItemExprTree(buf, strlen(buf), BINDITEMEXPR_STMTCHARSET, 3, child(0), child(1), child(2));
              buf[0] = 0;

              // The max length of repeat is "length of lpad" * "padding string
              // length" * "bytes per char".  The "length of lpad" is stored,
              // and when the type is synthesized, the length will be multiplied
              // by the nominal size of the pattern string.  Also note, both the
              // pattern string and the column must be the same type - an error
              // would occur otherwise at compile-time.

              Int32 maxRepeatLength = lpadLength;

              if (parseTree && parseTree->getOperatorType() == ITM_SUBSTR)
              {
                NAList<Lng32> childNumList(CmpCommon::statementHeap(),7);
                NAList<OperatorTypeEnum>
                  opTypeList(CmpCommon::statementHeap(),7);

                childNumList.insertAt(0,0);
                opTypeList.insertAt(0,ITM_CASE);

                childNumList.insertAt(1,0);
                opTypeList.insertAt(1,ITM_IF_THEN_ELSE);

                childNumList.insertAt(2,2);
                opTypeList.insertAt(2,ITM_IF_THEN_ELSE);

                childNumList.insertAt(3,2);
                opTypeList.insertAt(3,ITM_IF_THEN_ELSE);

                childNumList.insertAt(4,2); opTypeList.insertAt(4,ITM_CONCAT);
                childNumList.insertAt(5,0); opTypeList.insertAt(5,ITM_SUBSTR);
                childNumList.insertAt(6,0); opTypeList.insertAt(6,ITM_REPEAT);

                Repeat*  repNode =
                  (Repeat *) parseTree->getParticularItemExprFromTree(
                                                childNumList, opTypeList) ;
                if (repNode && (maxRepeatLength > 1))
                   repNode->setMaxLength(maxRepeatLength);

              } // end of block that is looking for the problematic Repeat node
              else
              {
                bindWA->setErrStatus();  // couldn't parse lpad replacement str
                return this;
              }
            }  // lpad has a fixed max length
          }
        } // third param is has more than one character or is a varchar
      }  // end of LPAD case
    break;

    case ITM_MONTHNAME:
      {
	// find the nullability of child
	ItemExpr * tempBoundTree =
	  child(0)->castToItemExpr()->bindNode(bindWA);
	if (bindWA->errStatus()) return this;

	NABoolean childIsNullable = FALSE;
	if (tempBoundTree->getValueId().getType().supportsSQLnull())
	  {
	    childIsNullable = TRUE;
	  }

	if (childIsNullable)
	  strcpy(buf, "CASE WHEN @A1 IS NULL THEN NULL ELSE ");
	else
	  strcpy(buf, "");

	strcat (buf, "CASE MONTH(@A1) WHEN 1 THEN 'January' WHEN 2 THEN 'February' WHEN 3 THEN 'March' WHEN 4 THEN 'April' WHEN 5 THEN 'May' WHEN 6 THEN 'June' WHEN 7 THEN 'July' WHEN 8 THEN 'August' WHEN 9 THEN 'September' WHEN 10 THEN 'October' WHEN 11 THEN 'November' WHEN 12 THEN 'December' ELSE 'Error' END");

	if (childIsNullable)
	  strcat(buf, " END;");
	else
	  strcat(buf, ";");
      }
    break;

    case ITM_ODBC_LENGTH:
      {
	strcpy(buf, "CHAR_LENGTH(RTRIM(@A1));");
      }
    break;

    case ITM_QUARTER:
      {
	// Make sure that the child is of datetime datatype.
	ItemExpr * tempBoundTree =
	  child(0)->castToItemExpr()->bindNode(bindWA);
	if (bindWA->errStatus()) return this;

	if (tempBoundTree->getValueId().getType().getTypeQualifier() !=
	    NA_DATETIME_TYPE)
	  {
	    // 4071 The operand of a QUARTER function must be a datetime.
	    *CmpCommon::diags() << DgSqlCode(-4071) << DgString0(getTextUpper());
	    bindWA->setErrStatus();
	    return this;
	  }

	NABoolean childIsNullable = FALSE;
	if (tempBoundTree->getValueId().getType().supportsSQLnull())
	  {
	    childIsNullable = TRUE;
	  }

	if (childIsNullable)
	  strcpy(buf, "CASE WHEN @A1 IS NULL THEN NULL ELSE ");
	else
	  strcpy(buf, "");

	strcat (buf, "CASE WHEN MONTH(@A1) >= 1 AND MONTH(@A1) <= 3 THEN 1 WHEN MONTH(@A1) >= 4 AND MONTH(@A1) <= 6 THEN 2 WHEN MONTH(@A1) >= 7 AND MONTH(@A1) <= 9 THEN 3 WHEN MONTH(@A1) >= 10 AND MONTH(@A1) <= 12 THEN 4 ELSE 0 END");

	if (childIsNullable)
	  strcat(buf, " END;");
	else
	  strcat(buf, ";");

      }
    break;

    case ITM_RIGHT:
    case ITM_LEFT:

      {
	// LEFT(<str>(@A1), <count>(A2))
	// RIGHT(<str>(@A1), <count>(A2))

	// verify that the COUNT is specified as an INT with no scale.
	ItemExpr * tempBoundTree =
	  child(1)->castToItemExpr()->bindNode(bindWA);
	if (bindWA->errStatus()) return this;

	const NAType &typ1 = tempBoundTree->getValueId().getType();

        if ( tempBoundTree->getOperatorType() != ITM_DYN_PARAM ) {
           const NumericType &ntyp1 = (NumericType &) typ1;
           if ((typ1.getTypeQualifier() != NA_NUMERIC_TYPE) ||
               (NOT ntyp1.isExact()))
             {
               // 4046 Count must be exact numeric.
               *CmpCommon::diags() << DgSqlCode(-4046) << DgString0(getTextUpper());
               bindWA->setErrStatus();
               return this;
             }

           if (ntyp1.getScale() != 0)
             {
               // 4047 Count must have a scale of 0.
               *CmpCommon::diags() << DgSqlCode(-4047) << DgString0(getTextUpper());
               bindWA->setErrStatus();
               return this;
             }
        }

        if ( getOperatorType() == ITM_RIGHT )
	  // The case expression is needed for cases where the length supplied
          // exceeds the length of the string; in this case we want to return 
          // the whole string. SUBSTR of a 0 or negative value doesn't do that.
          strcpy(buf, "SUBSTRING(@A1 FROM "
                      "CASE WHEN(CHAR_LENGTH(@A1) - CAST(@A2 AS INT UNSIGNED) + 1) > 1 "
                      "THEN (CHAR_LENGTH(@A1) - CAST(@A2 AS INT UNSIGNED) + 1) ELSE 1 END);");
        else
	  strcpy(buf, "SUBSTRING(@A1 FROM 1 FOR @A2);"); // LEFT()

      }
    break;

    case ITM_RPAD:
      {
        Int32 padStringLength = 0;
        Int32 rpadLength = -3;  // -2, -1 and 0 have a distinct meaning, see 
        // comment near function definition.

        if (child(2))
        {
          child(2) = child(2)->castToItemExpr()->bindNode(bindWA);
          if (bindWA->errStatus()) return this;
          padStringLength = getPadStringLength(child(2)->castToItemExpr(), bindWA);
          if (bindWA->errStatus()) return this;
        }

        if (child(1))
        {
          rpadLength = getPadLength(child(1)->castToItemExpr(), bindWA);
          if (bindWA->errStatus()) return this;
        }

        ItemExpr * tempBoundTree =
	  child(0)->castToItemExpr()->bindNode(bindWA);
	if (bindWA->errStatus()) return this;
        CharInfo::CharSet cs = CharInfo::UnknownCharSet;
        const NAType& type = tempBoundTree->getValueId().getType();
        if ( type.getTypeQualifier() == NA_CHARACTER_TYPE )
	  cs = ((CharType&)type).getCharSet();

        NABoolean padWithSpace = FALSE;

        // If the character set is ISO88591, ISO mapping is checked 
        // to get the correct character set in the current configuration
        if (cs == CharInfo::ISO88591)
        {	
          CharInfo::CharSet isoMappingCS = (CharInfo::CharSet)SqlParser_ISO_MAPPING;
          padWithSpace = isPadWithSpace(child(2),isoMappingCS);
        }
        else
          padWithSpace = isPadWithSpace(child(2),cs);

        if ((rpadLength > 0) && padWithSpace)
        {
          // use CAST to implement RPAD of blank space
          // remember to turn off string trunc. warnings.
          if (cs == CharInfo::ISO88591)
            sprintf(buf, "CAST(@A1 AS CHAR(%d) CHARACTER SET ISO88591) ;", rpadLength);
          else if (cs == CharInfo::UTF8)
            sprintf(buf, "CAST(@A1 AS CHAR(%d BYTES) CHARACTER SET UTF8) ;", rpadLength);
          else if (cs == CharInfo::SJIS /* && encodingCharSet == CharInfo::SJIS */)
            sprintf(buf, "CAST(@A1 AS CHAR(%d BYTES) CHARACTER SET SJIS) ;", rpadLength);
          else 
          {
            sprintf(buf, "CAST(@A1 AS CHAR(%d) CHARACTER SET UCS2) ;", rpadLength);
          }

          if ( rpadLength > CmpCommon::getDefaultNumeric(TRAF_MAX_CHARACTER_COL_LENGTH))
          {
             //Note: We claim error occurred in "REPEAT" here just so we get a consistent
             //error message regardless of whether or not the CAST optimization is used.
            *CmpCommon::diags() << DgSqlCode(-4129) << DgString0("REPEAT");
             *CmpCommon::diags() << DgSqlCode(-4062) << DgString0("RPAD");
             bindWA->setErrStatus();
             return this;
          }

          parseTree = parser.getItemExprTree(buf, strlen(buf), BINDITEMEXPR_STMTCHARSET, 1, child(0));
          buf[0] = 0;
          if (parseTree && parseTree->getOperatorType() == ITM_CAST)
          {
            ((Cast*) parseTree)->setNoStringTruncationWarnings(TRUE);
          }
        }
	else if (child(2) == NULL)
	{
	  // pad with spaces
	  // NOTE: The outer SUBSTRING operation would seem to be unnecessary
	  // in some cases, but it ensures the SQL compiler always knows that
	  // the max length of the return string is the user-specified value.

	  strcpy(buf, 
                "SUBSTRING( "
                "CASE WHEN @A1 IS NULL THEN NULL "
                "WHEN CHAR_LENGTH(@A1) >= @A2 THEN @A1 "
                "ELSE @A1 || SUBSTRING( SPACE(@A2, _UNKNOWN_), 1, "
                                " @A2 - CHAR_LENGTH(@A1) ) "
                "END, 1, @A2 ) ;");
	}
        else if (padStringLength == 1)
        {
          // when the third param has a length = 1 then the we do not need to
          // use the complicated case expression in the else branch a few lines 
          // below. When the padding string length is known to be 1 we can use
          // a case expression very similar to that used when child(2) is null.
            strcpy(buf,
                    "SUBSTRING( "
                    "CASE WHEN @A1 IS NULL THEN NULL "
                    "WHEN CHAR_LENGTH(@A1) >= @A2 THEN @A1 "
                    "ELSE @A1 || REPEAT(@A3, @A2) "
                    "END, 1, @A2 ) ;");

        }
	else
	{
          // pad with the third param.

          Int32 rpadLength = getPadLength(child(1)->castToItemExpr(), bindWA);
          if (bindWA->errStatus()) return this;

          // If result size needed is less than some threshold, use fast path
          if ((rpadLength > 0) && (rpadLength <= 1024)) {
            strcpy(buf,
                  "SUBSTRING( "
                  "CASE WHEN @A1 IS NULL OR @A3 IS NULL THEN NULL "
                  "WHEN CHAR_LENGTH(@A1) >= @A2 THEN @A1 "
                  "ELSE @A1 || REPEAT(@A3, @A2) "
                  "END, 1, @A2 );");
          }
          else
          {
            // NOTES:
            // (1) The outer SUBSTRING operation would seem to be unnecessary
            //     in some cases, but it ensures the SQL compiler always knows
            //     that the maximum length of the return string is the
            //     user-specified value (if the user specified a constant.)
            // (2) The 2nd WHEN prevents division by 0 later on and also
            //     ensures we catch overly large count (@A2) values at compile
            //     time if possible ... by sending @A2 directly to REPEAT
            //     without doing any CASTing or arithmetic on it (because
            //     doing those causes the value of @A2 to be resolved at
            //     runtime instead of compile time.)
            // (3) The 3rd WHEN prevents a negative REPEAT count and is
            //     also required by definition of this padding function.

            strcpy(buf,
                  "SUBSTRING( "
                  "CASE WHEN @A1 IS NULL OR @A3 IS NULL THEN NULL "
                  "WHEN CHAR_LENGTH(@A3) <= 1 THEN @A1 || REPEAT(@A3, @A2) "
                  "WHEN CHAR_LENGTH(@A1) >= @A2 THEN @A1 "
                  "ELSE @A1 || REPEAT(@A3, CAST(( "
                    "@A2 - CHAR_LENGTH(@A1))/CHAR_LENGTH(@A3) + 1 AS INT)) "
                  "END, 1, @A2 );");

            if (rpadLength > -1)
            {
              parseTree = parser.getItemExprTree(buf, strlen(buf), BINDITEMEXPR_STMTCHARSET, 3, child(0), child(1), child(2));
              buf[0] = 0;

              // The max length of repeat is "length of rpad" * "padding string
              // length" * "bytes per char".  The "length of rpad" is stored,
              // and when the type is synthesized, the length will be multiplied
              // by the nominal size of the pattern string.  Also note, both the
              // pattern string and the column must be the same type - an error
              // would occur otherwise at compile-time.

              Int32 maxRepeatLength = rpadLength;

              if (parseTree && parseTree->getOperatorType() == ITM_SUBSTR)
              {
                NAList<Lng32> childNumList(CmpCommon::statementHeap(),6);
                NAList<OperatorTypeEnum>
                  opTypeList(CmpCommon::statementHeap(),6);

                childNumList.insertAt(0,0);
                opTypeList.insertAt(0,ITM_CASE);

                childNumList.insertAt(1,0);
                opTypeList.insertAt(1,ITM_IF_THEN_ELSE);

                childNumList.insertAt(2,2);
                opTypeList.insertAt(2,ITM_IF_THEN_ELSE);

                childNumList.insertAt(3,2);
                opTypeList.insertAt(3,ITM_IF_THEN_ELSE);

                childNumList.insertAt(4,2); opTypeList.insertAt(4,ITM_CONCAT);
                childNumList.insertAt(5,1); opTypeList.insertAt(5,ITM_REPEAT);

                Repeat*  repNode =
                  (Repeat *) parseTree->getParticularItemExprFromTree(
                                                childNumList, opTypeList) ;
                if (repNode && (maxRepeatLength > 1))
                 repNode->setMaxLength(maxRepeatLength);

              } // end of block that is looking for the problematic Repeat node
              else
              {
                bindWA->setErrStatus();  // couldn't parse rpad replacement str
                return this;
              }
            }  // rpad has a fixed max length
	  }  // padding string of rpad has more than 1 character 
        }
      }
    break;

    case ITM_SIGN:
      {
	ItemExpr * tempBoundTree =
	  child(0)->castToItemExpr()->bindNode(bindWA);
	if (bindWA->errStatus()) return this;

	//
	// Type cast any param.
	//
	SQLInt nType(FALSE);
	ValueId vid = tempBoundTree->castToItemExpr()->getValueId();
	vid.coerceType(nType, NA_NUMERIC_TYPE);

	const NAType &typ1 = vid.getType();
        if (typ1.getTypeQualifier() != NA_NUMERIC_TYPE)
          {
            // 4045 Operand must be numeric.
            *CmpCommon::diags() << DgSqlCode(-4045) << DgString0(getTextUpper());
            bindWA->setErrStatus();
            return this;
          }

	// find the nullability of child
	NABoolean childIsNullable = FALSE;
	if (tempBoundTree->getValueId().getType().supportsSQLnull())
	  {
	    childIsNullable = TRUE;
	  }

	if (childIsNullable)
	  strcpy(buf, "CASE WHEN @A1 IS NULL THEN NULL ELSE ");
	else
	  strcpy(buf, "");

	strcat(buf, "CASE WHEN @A1 < 0 THEN -1 WHEN @A1 = 0 THEN 0 ELSE 1 END");

	if (childIsNullable)
	  strcat(buf, " END;");
	else
	  strcat(buf, ";");
      }
    break;

    case ITM_SPACE:
      {
	strcpy(buf, "REPEAT ( @A1 , @A2 );");
      }
    break;

    case ITM_USER:
    case ITM_AUTHNAME:
    case ITM_AUTHTYPE:
    {
	ItemExpr * tempBoundTree =
	  child(0)->castToItemExpr()->bindNode(bindWA);
	if (bindWA->errStatus())
	  return this;

	if (tempBoundTree->getValueId().getType().getTypeQualifier() !=
		NA_NUMERIC_TYPE)
	{
		strcpy(buf,
		       "cast(substring(@A1, 1, position(',' in @A1)-1) as smallint) * 256 + cast(substring(@A1, position(',' in @A1)+1, char_length(@A1) - position(',' in @A1)) as smallint)");
	}
	else
	{
		buf[0] = 0;
		parseTree = child(0);
	}
	
    }
    break;

    case ITM_WEEK:
      {
        // Essentially we need to process the following case statement to
        // achieve WEEK functionality. However, the case statement is split
        // into pieces to access the divide operator node.
        // "CAST((DAYOFYEAR(@A1) - 1 + DAYOFWEEK(FIRSTDAYOFYEAR(@A1)) - 1)/7 
        //  AS INT) + 1;"
        strcpy(buf, "(DAYOFYEAR(@A1) - 1 + DAYOFWEEK(FIRSTDAYOFYEAR(@A1)) - 1)/7");        
        
        ItemExpr *tempExpr = NULL;
	if (strlen(buf) > 0)
	  {
	    tempExpr = parser.getItemExprTree(buf, strlen(buf), BINDITEMEXPR_STMTCHARSET, 1, child(0));
	    if (! tempExpr)
	      {
		bindWA->setErrStatus();
		return this;
	      }

	    // Disable rounding for DATE TIME type of operations.
	    if (tempExpr->getOperatorType() == ITM_DIVIDE)
	      {
		((BiArith*)tempExpr)->setIgnoreSpecialRounding();
	      }
            
            strcpy(buf, "CAST(@A1 AS INT) + 1;");
            
            parseTree = parser.getItemExprTree(buf, strlen(buf), BINDITEMEXPR_STMTCHARSET, 1, tempExpr);

	    parseTree->setOrigOpType(getOperatorType());
	
	    buf[0] = 0;
	  }
      }
    break;

    case ITM_NULLIF:
      {
        strcpy(buf, "CASE WHEN @A1 = @A2 THEN NULL ELSE @A1 END;");
      }
    break;

    case ITM_ZEROIFNULL:
      {
	// find the nullability of child
	ItemExpr * tempBoundTree =
	  child(0)->castToItemExpr()->bindNode(bindWA);
	if (bindWA->errStatus()) return this;

	//
	// Type cast any param.
	//
	SQLInt nType(FALSE);
	ValueId vid = tempBoundTree->castToItemExpr()->getValueId();
	vid.coerceType(nType, NA_NUMERIC_TYPE);

	if (tempBoundTree->getValueId().getType().getTypeQualifier() != NA_NUMERIC_TYPE)
	  {
	    // 4045 must be numeric.
	    *CmpCommon::diags() << DgSqlCode(-4045) << DgString0(getTextUpper());
	    bindWA->setErrStatus();
	    return this;
	  }

	if ((tempBoundTree->getValueId().getType().supportsSQLnull()) ||
	    (tempBoundTree->isASubquery()))
	  {
	    strcpy(buf, "CASE WHEN @A1 IS NULL then 0 ELSE @A1 END;");
	  }
	else
	  {
	    // if child is not nullable, then no need to do zeroifnull.
	    // Point to child.
	    parseTree = child(0)->castToItemExpr();
	  }
      }
    break;
    case ITM_SUBSTR:
      {
          //
          // Perhaps someone, someday, will figure out how to make
          // sqlparser.y invoke the Substring() constructor and pass
          // it a constant of 1 for the start argument...without
          // causing crashes of mxcmp.  Until then, we will
          // translate it to old SUBSTRING syntax here.
          //
          strcpy(buf, "SUBSTRING(@A1 FROM 1 FOR @A2);");
      }
    break;

    case ITM_ROUND:
      {
	//Do initial argument validations. 
	// First operand is the value to be rounded. Cannot be float or
	// bignums.
	// 
	// Second operand is the number of digits after decimal point to be
	// rounded to.
	// Third operand is the rounding mode. Must be a constant.
	//

	NABoolean useOptimizedRound = TRUE;

	//Processing of first operand.
	ItemExpr * tempBoundTree = child(0)->castToItemExpr()->bindNode(bindWA);
	if (bindWA->errStatus()) return this;

	if (tempBoundTree->getValueId().getType().getTypeQualifier() == NA_UNKNOWN_TYPE)
	  child(0)->getValueId().coerceType(NA_NUMERIC_TYPE);
	
	if (tempBoundTree->getValueId().getType().getTypeQualifier() != NA_NUMERIC_TYPE)
	  {
	    // 4059 The first operand must be numeric.
	    *CmpCommon::diags() << DgSqlCode(-4059) << DgString0("ROUND");
	    bindWA->setErrStatus();
	    return this;
	  }
	
	NumericType &type_op1 =
	  (NumericType&)tempBoundTree->getValueId().getType();
	if ((NOT type_op1.isExact()) ||
	    (type_op1.isComplexType()))
	  {
	    useOptimizedRound = FALSE;
	    // Only supporting ROUND of exact numerics with max precision
	    // of MAX_NUMERIC_PRECISION (no floats or bignums).
	    //
	    // 4059 The first operand must be numeric.
	    //	    *CmpCommon::diags() << DgSqlCode(-4070) << DgString0("ROUND");
	    //bindWA->setErrStatus();
	    //return this;
	  }

	//second operand.
 Lng32 roundTo = -1;
	if (child(1) == NULL)
	  {
	    // round all digits after the decimal point
	    roundTo = 0;
	  }
	else
	  {
	    //argument validation for second argument.
	    ItemExpr *secondOpExpr = child(1)->castToItemExpr(); 
	    secondOpExpr->bindNode(bindWA);
	    if (bindWA->errStatus()) 
	      return this;
	    
	    if (secondOpExpr->getValueId().getType().getTypeQualifier() != NA_NUMERIC_TYPE)
	      {
		*CmpCommon::diags() << DgSqlCode(-4052) << DgString0("ROUND");
		bindWA->setErrStatus();
		return this;
	      }
	    
	    NumericType &tempType = (NumericType&)secondOpExpr->getValueId().getType();
	    if(! (tempType.isExact() && tempType.getScale() <=0 ))
	      {
		*CmpCommon::diags() << DgSqlCode(-4047) << DgString0("ROUND");
		bindWA->setErrStatus();
		return this;
	      }

	    if (secondOpExpr->getOperatorType() == ITM_CONSTANT)
	      {
		roundTo = 
		  (Lng32)((ConstValue*)secondOpExpr)->getExactNumericValue();

		Lng32 maxPrecision = MAX_NUMERIC_PRECISION;
		if (type_op1.isComplexType())
		  maxPrecision = CmpCommon::getDefaultNumeric(MAX_NUMERIC_PRECISION_ALLOWED);

		if (roundTo > maxPrecision)
		  {
		    *CmpCommon::diags() << DgSqlCode(-4054) << DgString0("ROUND") << DgInt0(maxPrecision);
		    bindWA->setErrStatus();
		    return this;
		  }
	      }
	    else
	      {
		useOptimizedRound = FALSE;
		//		*CmpCommon::diags() << DgSqlCode(-4052) << DgString0("ROUND");
		//		bindWA->setErrStatus();
		//		return this;
	      }
	  }

	short roundingMode = 1;
	if( child(2) != NULL )
	  {
	    //argument validation for second argument.
	    ItemExpr *thirdOpExpr = child(2)->castToItemExpr(); 
	    thirdOpExpr->bindNode(bindWA);
	    if (bindWA->errStatus()) 
	      return this;
	    
	    if ((thirdOpExpr->getOperatorType() != ITM_CONSTANT) ||
		(thirdOpExpr->getValueId().getType().getTypeQualifier() != NA_NUMERIC_TYPE) ||
		(useOptimizedRound == FALSE))
	      {
		*CmpCommon::diags() << DgSqlCode(-4053) << DgString0("ROUND");
		bindWA->setErrStatus();
		return this;
	      }
	    
	    NumericType &tempType = (NumericType&)thirdOpExpr->getValueId().getType();
	    if(! (tempType.isExact() && tempType.getScale() <=0 ))
	      {
		*CmpCommon::diags() << DgSqlCode(-4047) << DgString0("ROUND");
		bindWA->setErrStatus();
		return this;
	      }

	    roundingMode = 
	      (short)((ConstValue*)thirdOpExpr)->getExactNumericValue();
	    if ((roundingMode < 0) ||
		(roundingMode > 2))
	      {
		*CmpCommon::diags() << DgSqlCode(-4053) << DgString0("ROUND");
		bindWA->setErrStatus();
		return this;
	      }
	  }
        
	if (useOptimizedRound)
	  {
	    // divide the value by 10 raised to the power of the number of
	    // digits to be rounded off.
	    // If roundTo(second) param is a const, create a literal instead
	    // of an expression.
	    ItemExpr * divExpr = NULL;
	    Int64 denom = 0;
	    buf[0] = 0;
	    if ((roundTo >= 0) &&
		(type_op1.getScale() > roundTo))
	      {
		denom = 1;
	 Int32 i = (type_op1.getScale() - roundTo);
		while (i > 0)
		  {
		    denom = denom * 10;
		    i--;
		  }
		
		str_sprintf(buf, "@A1 / %ld", denom);
	      }
	    
	    if (strlen(buf) > 0)
	      {
		divExpr = parser.getItemExprTree(buf, strlen(buf), BINDITEMEXPR_STMTCHARSET, 2, child(0), child(1));
		if (! divExpr)
		  {
		    bindWA->setErrStatus();
		    return this;
		  }
		
		// indicate that this division need to do rounding.
		if (divExpr->getOperatorType() == ITM_DIVIDE)
		  {
		    ((BiArith*)divExpr)->setRoundingMode(roundingMode);
		    ((BiArith*)divExpr)->setDivToDownscale(TRUE);
		  }
		
		// multiply by 10 ** (number of digits rounded off) to
		// get back to the original scale.
		str_sprintf(buf, "cast(@A1 * %ld as numeric(%d,%d))", 
                  denom, MAX_NUMERIC_PRECISION, MAXOF(type_op1.getScale(), roundTo));
		parseTree = parser.getItemExprTree(buf, strlen(buf), BINDITEMEXPR_STMTCHARSET, 2, divExpr, child(1));
		if (! parseTree)
		  {
		    bindWA->setErrStatus();
		    return this;
		  }
		buf[0] = 0;
	      }
	    else
	      parseTree = child(0)->castToItemExpr();
	  }
	else
	  {
	    //The round function is implemented using other existing math functions.
	    //Typically ROUND(expr,num) will involve the following:Ex: ROUND(34.58,1)
	    //1. Move the decimal point num+1 places by multiplying by num+1.
	    //   Example: 34.58 * power(10,num+1) = 3458.00
	    //2. MOD(result,10)=x determines if round-up is required. In the case of 
	    //   inexact numbers, round-down or round-even is also performed.
	    //   If x < 5 then no change. if x > 5 then round up.
	    //   if x = 5 then round even.
	    //   Example:MOD(3458.00, 10) = 8 means round up.
	    //3. Based on roundup,round down, round even, round the number at numth
	    //   position.
	    //   Example: floor(34.58 * power(10, num)) = 345.00 + 1 = 346.00
	    //4. Then move the decimal point by multiplying or dividing by
	    //   multiples of 10.
	    //   Example: 346.00 * power(10, -1) = 34.60.
	    //5. In the case of inexact numbers, round even is performed if required.
	    
	    //ROUND processing:              
	    if( type_op1.isExact() && //MOD does not support inexact expressions.
	        !( CmpCommon::getDefault(COMP_BOOL_15) == DF_ON &&
	           type_op1.isBigNum() ) ) // exclude runtime BigNum case
            {
	      if(child(1) == NULL)//In ROUND(expr,num), num is optional, defaults to 0.
              { 
		if(type_op1.getScale() <=0)//no need to perform round in this case.
		  parseTree = child(0)->castToItemExpr();
                else
                {
                  if(type_op1.isBigNum()) 
                    strcpy(buf,"case when SIGN(@A1) < 0 then case when (cast(@A1 * 10 as numeric(128,0))-(10 * cast(cast((@A1 * 10)as numeric(128,0))/10  as numeric(128,0)))) > -5 then cast(@A1 as numeric(128,0)) when (cast(@A1 * 10 as numeric(128,0))-(10 * cast(cast((@A1 * 10)as numeric(128,0))/10  as numeric(128,0)))) <=-5 then cast(@A1 as numeric(128,0)) - 1 END else case when (cast(@A1 * 10 as numeric(128,0))-(10 * cast(cast((@A1 * 10)as numeric(128,0))/10  as numeric(128,0)))) < 5 then cast(@A1 as numeric(128,0)) when (cast(@A1 * 10 as numeric(128,0))-(10 * cast(cast((@A1 * 10)as numeric(128,0))/10  as numeric(128,0)))) >=5 then cast(@A1 as numeric(128,0)) +SIGN(@A1) END END");
                  else
		    strcpy(buf, "case when MOD(cast((abs(@A1) * 10) as largeint), 10) < 5 then cast(@A1 as largeint) when MOD(cast((abs(@A1) * 10) as largeint), 10) >=5 then cast(@A1 as largeint) +SIGN(@A1) END");
                }
	      }
	      else{
                if(type_op1.getScale() <= 0){
                  if(type_op1.isBigNum()) 
                    strcpy(buf, "case SIGN(@A1) when -1 then case SIGN(@A2+1)when 1 then @A1 ELSE case when (cast(@A1/cast(power(10, abs(@A2+1)) as numeric(128,0)) as numeric(128,0))-(10 * cast(cast(@A1/cast(power(10, abs(@A2+1)) as numeric(128,0)) as numeric(128,0))/10 as numeric(128,0)))) > -5 then cast((@A1/cast(power(10,abs(@A2)) as numeric(128,0))) as numeric(128,0))* cast(power(10,abs(@A2)) as numeric(128,0)) ELSE (cast((@A1/cast(power(10,abs(@A2)) as numeric(128,0))) as numeric(128,0)) + SIGN(@A1)) * cast(power(10,abs(@A2)) as numeric(128,0)) END END ELSE case SIGN(@A2+1)when 1 then @A1 ELSE case when (cast(@A1/cast(power(10, abs(@A2+1)) as numeric(128,0)) as numeric(128,0))-(10 * cast(cast(@A1/cast(power(10, abs(@A2+1)) as numeric(128,0)) as numeric(128,0))/10 as numeric(128,0)))) < 5 then cast((@A1/cast(power(10,abs(@A2)) as numeric(128,0))) as numeric(128,0))* cast(power(10,abs(@A2)) as numeric(128,0)) ELSE (cast((@A1/cast(power(10,abs(@A2)) as numeric(128,0))) as numeric(128,0)) + SIGN(@A1)) * cast(power(10,abs(@A2)) as numeric(128,0)) END END END");
                  else
		    strcpy(buf, "case SIGN(@A2+1)when 1 then case when MOD(cast(abs(@A1) * cast(power(10, @A2+1) as largeint) as largeint), 10) < 5 then cast((@A1 * cast(power(10,@A2) as largeint)) as largeint) / cast(power(10,abs(@A2)) as largeint) when MOD(cast(abs(@A1) * cast(power(10, @A2+1) as largeint) as largeint), 10) >=5 then (cast((@A1 * cast(power(10,@A2) as largeint)) as largeint) +SIGN(@A1)) / cast(power(10,@A2) as largeint) END ELSE case when MOD(cast(abs(@A1) /cast(power(10, abs(@A2+1)) as largeint) as largeint), 10) < 5 then cast((@A1/cast(power(10,abs(@A2)) as largeint)) as largeint)* cast(power(10,abs(@A2)) as largeint) when MOD(cast(abs(@A1) /cast(power(10, abs(@A2+1)) as largeint) as largeint), 10) >=5 then (cast((@A1/cast(power(10,abs(@A2)) as largeint)) as largeint)+ SIGN(@A1)) * cast(power(10,abs(@A2)) as largeint) END END");
                }
		else{
                      NumericType *typeTemp;
                      if(type_op1.isBigNum())
                      {
                       // In the below case statement, cast the result to 
                       // resultPrecision + 1. This is done for cases that 
                       // result in consuming additional precision.
                       // For example, round(99.00, -2) = 100.00 
                       const Int16 DisAmbiguate = 0;
                       typeTemp = new(bindWA->wHeap()) SQLNumeric(bindWA->wHeap(), type_op1.isSigned(),
                                                                   MINOF(type_op1.getPrecision()+1,128),
                                                                   type_op1.getScale(),
                                                                   DisAmbiguate // added for 64bit proj.
                                                                  );
                        
                       NAString nstr = "";
		        typeTemp->getMyTypeAsText(&nstr, FALSE);
		        char *text = convertNAString(nstr, bindWA->wHeap()); 
		        sprintf(buf, "cast(case SIGN(@A1) when -1 then case SIGN(@A2+1) when 1 then case when (cast(@A1 * cast(power(10, @A2+1) as numeric(128,0)) as numeric(128,0))-(10 * cast(cast(@A1 * cast(power(10, @A2+1) as numeric(128,0))as numeric(128,0))/10  as numeric(128,0)))) > -5 then cast((@A1 * cast(power(10,@A2) as numeric(128,0))) as numeric(128,0)) / cast(power(10,abs(@A2)) as numeric(128,0)) ELSE (cast((@A1 * cast(power(10,@A2) as numeric(128,0))) as numeric(128,0)) + SIGN(@A1) ) / cast(power(10,@A2) as numeric(128,0)) END ELSE case when (cast(@A1/cast(power(10, abs(@A2+1)) as numeric(128,0)) as numeric(128,0))-(10 * cast(cast(@A1/cast(power(10, abs(@A2+1)) as numeric(128,0)) as numeric(128,0))/10 as numeric(128,0)))) > -5 then cast((@A1/cast(power(10,abs(@A2)) as numeric(128,0))) as numeric(128,0))* cast(power(10,abs(@A2)) as numeric(128,0)) ELSE (cast((@A1/cast(power(10,abs(@A2)) as numeric(128,0))) as numeric(128,0))+ SIGN(@A1)) * cast(power(10,abs(@A2)) as numeric(128,0)) END END ELSE case SIGN(@A2+1) when 1 then case when (cast(@A1 * cast(power(10, @A2+1) as numeric(128,0)) as numeric(128,0))-(10 * cast(cast(@A1 * cast(power(10, @A2+1) as numeric(128,0))as numeric(128,0))/10  as numeric(128,0)))) < 5 then cast((@A1 * cast(power(10,@A2) as numeric(128,0))) as numeric(128,0)) / cast(power(10,abs(@A2)) as numeric(128,0)) ELSE (cast((@A1 * cast(power(10,@A2) as numeric(128,0))) as numeric(128,0)) + SIGN(@A1) ) / cast(power(10,@A2) as numeric(128,0)) END ELSE case when (cast(@A1/cast(power(10, abs(@A2+1)) as numeric(128,0)) as numeric(128,0))-(10 * cast(cast(@A1/cast(power(10, abs(@A2+1)) as numeric(128,0)) as numeric(128,0))/10 as numeric(128,0)))) < 5 then cast((@A1/cast(power(10,abs(@A2)) as numeric(128,0))) as numeric(128,0))* cast(power(10,abs(@A2)) as numeric(128,0)) ELSE (cast((@A1/cast(power(10,abs(@A2)) as numeric(128,0))) as numeric(128,0))+ SIGN(@A1)) * cast(power(10,abs(@A2)) as numeric(128,0)) END END END as %s)", text);
                      }
                      else
                      {
                        if(type_op1.getScale() <=18)
		         {
                          // In the below case statement, cast the result to 
                          // resultPrecision + 1. This is done for cases that 
                          // result in consuming additional precision.
                          // For example, round(99.00, -2) = 100.00
                          typeTemp = (NumericType*)type_op1.newCopy(bindWA->wHeap());
                          typeTemp->setPrecision(MINOF(type_op1.getPrecision()+1,18));
                          typeTemp->setScale(type_op1.getScale());
      		  
		          //precision is 10 or bigger, then UNSIGNED cannot be used for NUMERIC type.
		          if((typeTemp->getPrecision() >= 10 ) &&  typeTemp->isUnsigned())
		            typeTemp->makeSigned();

		          NAString nstr = "";
		          typeTemp->getMyTypeAsText(&nstr, FALSE);
		          char *text = convertNAString(nstr, bindWA->wHeap()); 
		          sprintf(buf, "cast(case SIGN(@A2+1)when 1 then case when MOD(cast(abs(@A1) * cast(power(10, @A2+1) as largeint) as largeint), 10) < 5 then cast((@A1 * cast(power(10,@A2) as largeint)) as largeint) / cast(power(10,abs(@A2)) as largeint) when MOD(cast(abs(@A1) * cast(power(10, @A2+1) as largeint) as largeint), 10) >=5 then (cast((@A1 * cast(power(10,@A2) as largeint)) as largeint) +SIGN(@A1)) / cast(power(10,@A2) as largeint) END ELSE case when MOD(cast(abs(@A1) /cast(power(10, abs(@A2+1)) as largeint) as largeint), 10) < 5 then cast((@A1/cast(power(10,abs(@A2)) as largeint)) as largeint)* cast(power(10,abs(@A2)) as largeint) when MOD(cast(abs(@A1) /cast(power(10, abs(@A2+1)) as largeint) as largeint), 10) >=5 then (cast((@A1/cast(power(10,abs(@A2)) as largeint)) as largeint)+ SIGN(@A1)) * cast(power(10,abs(@A2)) as largeint) END END as %s)", text);
                        }
                        else
                        {
                           //If the scale of type_op1 is greater than 18,
                           //skip the casting of final result expression.
                           sprintf(buf, "case SIGN(@A2+1)when 1 then case when MOD(cast(abs(@A1) * cast(power(10, @A2+1) as largeint) as largeint), 10) < 5 then cast((@A1 * cast(power(10,@A2) as largeint)) as largeint) / cast(power(10,abs(@A2)) as largeint) when MOD(cast(abs(@A1) * cast(power(10, @A2+1) as largeint) as largeint), 10) >=5 then (cast((@A1 * cast(power(10,@A2) as largeint)) as largeint) +SIGN(@A1)) / cast(power(10,@A2) as largeint) END ELSE case when MOD(cast(abs(@A1) /cast(power(10, abs(@A2+1)) as largeint) as largeint), 10) < 5 then cast((@A1/cast(power(10,abs(@A2)) as largeint)) as largeint)* cast(power(10,abs(@A2)) as largeint) when MOD(cast(abs(@A1) /cast(power(10, abs(@A2+1)) as largeint) as largeint), 10) >=5 then (cast((@A1/cast(power(10,abs(@A2)) as largeint)) as largeint)+ SIGN(@A1)) * cast(power(10,abs(@A2)) as largeint) END END");
                        }
                      }
		    }
	      }
             // Once we are done bignum round processing above, we need to 
             // make sure not to introduce realbignum if the type_op1 is not
             // real BigNum. Using CAST(x as numeric(128,0)) will automatically
             // make the operand realbignum. 
             if(type_op1.isBigNum() &&
              (!((SQLBigNum &)type_op1).isARealBigNum()))
             {
                resetRealBigNum = TRUE;
             }
	    }
	    else {
              ItemExpr * v =
                new(bindWA->wHeap()) MathFunc(ITM_ROUND, child(0), child(1));

              boundTree = v->bindNode(bindWA);
              if (bindWA->errStatus())
                return this;
	    } 	
	  }
      }
    
    break;
    case ITM_CURRNT_USER:
      {
	strcpy(buf, "TRANSLATE(CURRNT_USR_INTN USING ISO88591ToUCS2);");
      }
    break;
    case ITM_SESSN_USER:
      {
	strcpy(buf, "TRANSLATE(SESSN_USR_INTN USING ISO88591ToUCS2);");
      }
    break;
    case ITM_CONVERTTOHX:
      {
	strcpy(buf, "TRANSLATE(CONVERTTOHX_INTN(@A1) USING ISO88591ToUCS2);");
      }
    break;

    case ITM_SCALE_TRUNC:
      {
	ItemExpr *tempBoundTree = child(0)->castToItemExpr()->bindNode(bindWA); 
	if (bindWA->errStatus()) 
	  return this;

	if (tempBoundTree->getValueId().getType().getTypeQualifier() == NA_NUMERIC_TYPE)
	  {
	    NumericType &type_op1 =
	      (NumericType&)tempBoundTree->getValueId().getType();
	    
	    if ((NOT type_op1.isExact()) ||
		(type_op1.isComplexType()))
	      {
		// 4059 The first operand must be numeric.
		*CmpCommon::diags() << DgSqlCode(-4070) << DgString0("ROUND");
		bindWA->setErrStatus();
		return this;
	      }
	    
	    Lng32 truncVal = 
	      (Lng32)((ConstValue*)child(1)->castToItemExpr())->getExactNumericValue();
	    
	    str_sprintf(buf, "CAST(@A1 as NUMERIC(%d, %d));",
			type_op1.getPrecision(), truncVal);
	  }
	else if (tempBoundTree->getValueId().getType().getTypeQualifier() == NA_DATETIME_TYPE)
	  {
	    // for now, just return the child.
	    boundTree = tempBoundTree;
	  }
	else
	  {
	    // 4059 The first operand must be numeric.
	    *CmpCommon::diags() << DgSqlCode(-4059) << DgString0("TRUNC");
	    bindWA->setErrStatus();
	    return this;
	  }
      }
    break;

    case ITM_TO_NUMBER:
      {
	ItemExpr *tempBoundTree = child(0)->castToItemExpr()->bindNode(bindWA); 
	if (bindWA->errStatus()) 
	  return this;

        if (CmpCommon::getDefault(MODE_SPECIAL_4) == DF_OFF)
          {
            if (tempBoundTree->getValueId().getType().getTypeQualifier() != NA_CHARACTER_TYPE)
              {
                // 4043 The first operand must be numeric.
                *CmpCommon::diags() << DgSqlCode(-4043) << DgString0("TO_NUMBER");
                bindWA->setErrStatus();
                return this;
              }
          }

	NAType &type_op1 =
	  (NAType&)tempBoundTree->getValueId().getType();
	
	str_sprintf(buf, "CAST(@A1 as NUMERIC(%d));",
		    type_op1.getNominalSize());
      }
    break;

    case ITM_TO_TIMESTAMP:
      {
	ItemExpr *tempBoundTree = child(0)->castToItemExpr()->bindNode(bindWA); 
	if (bindWA->errStatus()) 
	  return this;

	str_sprintf(buf, "CAST(@A1 as TIMESTAMP(6));");
      }
    break;

    case ITM_CURRENT_TIMESTAMP_UTC:
      {
	if (bindWA->currentCmpContext()->gmtDiff() > 0)
	  str_sprintf(buf, "CURRENT_TIMESTAMP + INTERVAL '%4d' MINUTE(4);",
		      bindWA->currentCmpContext()->gmtDiff());
	else if (bindWA->currentCmpContext()->gmtDiff() < 0)
	  str_sprintf(buf, "CURRENT_TIMESTAMP - INTERVAL '%4d' MINUTE(4);",
		      -bindWA->currentCmpContext()->gmtDiff());
	else
	  strcpy(buf, "CURRENT_TIMESTAMP");
      }
    break;

    case ITM_CURRENT_TIME_UTC:
      {
	str_sprintf(buf, "CAST(CURRENT_TIMESTAMP_UTC AS TIME);");
      }
    break;

    case ITM_GROUPING_ID:
      {
        *CmpCommon::diags() << DgSqlCode(-3242)
                            << DgString0("GROUPING_ID function must be specified in the select list of a GROUP BY ROLLUP statement.");
        bindWA->setErrStatus();
        return this;
      }
      break;

    case ITM_TO_BINARY:
      {
	ItemExpr *child0 = child(0)->castToItemExpr()->bindNode(bindWA); 
	if (bindWA->errStatus()) 
	  return this;

        Lng32 binLen = 0;
        if (child(1) != NULL)
          {
            ItemExpr *child1 = child(1)->castToItemExpr()->bindNode(bindWA);
            if (bindWA->errStatus()) 
              return this;

            // parser has already validated that child1 is an unsigned const.
            NABoolean negate;
            ConstValue * cv = child1->castToConstValue(negate);
            binLen = (Lng32)cv->getExactNumericValue();
          }

        const NAType &type0 = child0->getValueId().getType();
        if (binLen == 0)
          binLen = type0.getNominalSize();
        SQLBinaryString *bin = 
          new(bindWA->wHeap()) SQLBinaryString(bindWA->wHeap(),
                                               binLen, type0.supportsSQLnull(),
                                               type0.isVaryingLen());

        Cast *cast = new(bindWA->wHeap()) Cast(child0, bin);
        parseTree = cast;
      }
      break;

    case ITM_TO_CHAR:
      {
	ItemExpr *child0 = child(0)->castToItemExpr()->bindNode(bindWA); 
	if (bindWA->errStatus()) 
	  return this;
        
        ItemExpr* ie = NULL;
        const NAType &type0 = child0->getValueId().getType();
        if (type0.getTypeQualifier() == NA_DATETIME_TYPE)
          {
            ie = new(bindWA->wHeap()) DateFormat
              (child0, "UNSPECIFIED", DateFormat::FORMAT_TO_CHAR);
          }
        else
          {
            Lng32 len = type0.getDisplayLength();
            SQLVarChar *vc = new(bindWA->wHeap()) SQLVarChar
              (bindWA->wHeap(), len, type0.supportsSQLnull());
            
            ie = new(bindWA->wHeap()) Cast(child0, vc);
          }
        
        parseTree = ie;
      }
      break;

    ////////////////////////////////////////////////////////////////////////
    // OVERLAY ( src_str PLACING replace_str FROM start_pos [ FOR length ]
    // is equivalent to:
    // If 'FOR length' is specified:
    //   SUBSTRING(src_str FROM 1 FOR start_pos - 1 ) 
    //   || replace_str 
    //   || SUBSTRING(src_str FROM start_pos + length )
    // Otherwise:
    //   SUBSTRING(src_str FROM 1 FOR start_pos - 1 ) 
    //   || replace_str 
    //   || SUBSTRING(src_str FROM start_pos + CHAR_LENGTH(replace_str) )
    //
    //
    // STUFF (srcStr, startPos, length, replaceStr)
    /////////////////////////////////////////////////////////////////////////
    case ITM_OVERLAY:
      {
	bindChildren(bindWA);
	if (bindWA->errStatus()) 
	  return this;

        NAString funcName = (overlayFuncWasStuff() ? "STUFF" : getTextUpper());

        NAString errReason;
        if (child(0)->getValueId().getType().getTypeQualifier() != NA_CHARACTER_TYPE)
          {
            errReason = "Source string specified in function " + funcName + " must be of character datatype.";
	    *CmpCommon::diags() << DgSqlCode(-3242)
                                << DgString0(errReason);
	    bindWA->setErrStatus();
	    return this;
          }

        if (child(1)->getValueId().getType().getTypeQualifier() != NA_CHARACTER_TYPE)
          {
            errReason = "Replacement string specified in function " + funcName + " must be of character datatype.";
	    *CmpCommon::diags() << DgSqlCode(-3242)
                                << DgString0(errReason);
	    bindWA->setErrStatus();
	    return this;
          }

        if (child(2))
          {
            const NumericType &ntyp2 =
              (NumericType &) child(2)->getValueId().getType();
            if (NOT ((ntyp2.getTypeQualifier() == NA_NUMERIC_TYPE) &&
                     (ntyp2.isExact()) && (ntyp2.getScale() == 0)))
              {
                errReason = "Start position of replacement string specified in function " + funcName + " must be of numeric datatype with scale of 0.";
                *CmpCommon::diags() << DgSqlCode(-3242)
                                    << DgString0(errReason);
                bindWA->setErrStatus();
                return this;
              }

            // this error will be caught at execution time based on actual
            // values
            errReason = "Start position of replacement string specified in function " + funcName + " cannot be less than or equal to zero.";
            RaiseError *ire = 
              new (bindWA->wHeap()) 
              RaiseError((Lng32)EXE_STMT_NOT_SUPPORTED, "", "", errReason, 
                         &child(2)->getValueId().getType());
            ire->bindNode(bindWA);
            if (bindWA->errStatus())
              return this;
            
            setChild(4, ire);
          }

        if (child(3))
          {
            const NumericType &ntyp3 =
              (NumericType &) child(3)->getValueId().getType();
            if (NOT ((ntyp3.getTypeQualifier() == NA_NUMERIC_TYPE) &&
                     (ntyp3.isExact()) && (ntyp3.getScale() == 0)))
              {
                errReason = "Number of characters to replace specified in function " + funcName + " must be of numeric datatype with scale of 0.";
                *CmpCommon::diags() << DgSqlCode(-3242)
                                    << DgString0(errReason);
                bindWA->setErrStatus();
                return this;
              }

            // this error will be caught at execution time based on actual
            // values
            errReason = "Number of characters to replace specified in function " + funcName + " cannot be less than zero.";
            RaiseError *ire = 
              new (bindWA->wHeap()) 
              RaiseError((Lng32)EXE_STMT_NOT_SUPPORTED, "", "", errReason, 
                         &child(3)->getValueId().getType());
            ire->bindNode(bindWA);
            if (bindWA->errStatus())
              return this;
            
            setChild(5, ire);
          }

        if (child(3))
          // @A5(child4) and @A6(child5) are RaiseError operators that will
          // be evaluated at runtime if that error condition occurs.
          strcpy(buf, "SUBSTRING(@A1 FROM 1 FOR case when @A3 <= 0 then @A5 else @A3 end - 1) || @A2 || SUBSTRING (@A1 FROM @A3 + case when @A4 < 0 THEN @A6 else @A4 end); "); 
        else
          strcpy(buf, "SUBSTRING(@A1 FROM 1 FOR case when @A3 <= 0 then @A5 else @A3 end - 1) || @A2 || SUBSTRING (@A1 FROM @A3 + char_length(@A2) ); "); 
      }
      break;
      
    default:
      {
	bindWA->setErrStatus();
	return this;
      }
    }
  
  if (CURRENTSTMT->getItemExprOrigOpTypeCounter() == 0)
    CURRENTSTMT->setItemExprOrigOpTypeBeingBound(getOperatorType());
  
  (CURRENTSTMT->getItemExprOrigOpTypeCounter())++;

  if (strlen(buf) > 0)
    {
      parseTree = parser.getItemExprTree(buf, strlen(buf), BINDITEMEXPR_STMTCHARSET, 6, child(0), child(1), child(2), child(3), child(4), child(5));
    }

 if (parseTree) {

    switch (getOperatorType())
      {
      case ITM_UNICODE_CODE_VALUE:
      case ITM_NCHAR_MP_CODE_VALUE:
	{
	  parseTree = new(bindWA->wHeap()) CodeVal(getOperatorType(), parseTree);
	}
	break;

      case ITM_AUTHNAME:
      case ITM_AUTHTYPE:
      case ITM_USER:
	{
	  parseTree = new(bindWA->wHeap()) MonadicUSERFunction(parseTree,getOperatorType());
	}
	break;
      case ITM_ROUND:
       {
         if(resetRealBigNum)
          resetRealBigNumFlag(parseTree);
       }
       break;

      } // switch

    boundTree = parseTree->bindNode(bindWA);
    if (bindWA->errStatus()) boundTree = NULL;
  }

  //origOpTypeCounter()--;
  (CURRENTSTMT->getItemExprOrigOpTypeCounter())--;

  // once out of the scope of any operator set the type to default
  //if (origOpTypeCounter() == 0)
  //  origOpTypeBeingBound() = NO_OPERATOR_TYPE;

  if (CURRENTSTMT->getItemExprOrigOpTypeCounter() == 0)
    CURRENTSTMT->setItemExprOrigOpTypeBeingBound(NO_OPERATOR_TYPE);

  // NOTE: Don't call unparse() below if we don't need to.  It may blow up
  // since getSubquery() could return NULL if the subquery was discarded.
  //
  if ((boundTree == NULL) && (CURRENTSTMT->getItemExprOrigOpTypeCounter() == 0) ) {
    NAString orig(bindWA->wHeap());

    // For functions whose arity is more than 1, am removing
    // the use of unparse since it does not do the right thing.
    //if (parseTree) parseTree->unparse(orig,DEFAULT_PHASE,USER_FORMAT_DELUXE);
    if (getArity() > 1)
      orig = getTextUpper();
    else
      unparse(orig, DEFAULT_PHASE, USER_FORMAT_DELUXE);

    // 4062 The preceding error actually occurred in function $0~String0.
    *CmpCommon::diags() << DgSqlCode(-4062) << DgString0(orig);
    bindWA->setErrStatus();
  }

  return boundTree;
}

ItemExpr *ZZZBinderFunction::tryToUndoBindTransformation(ItemExpr *expr)
{
  // Given as input an expression produced by ZZZBinderFunction::bindNode(),
  // return an equivalent ZZZBinderFunction ItemExpr or NULL for cases
  // that are not yet supported. This is used for a) unparsing such
  // functions, and b) validating some item expressions.

  ItemExpr *result = NULL;
  ItemExpr *op1 = expr;
  ItemExpr *op2 = NULL;

  switch (expr->origOpType())
    {
    case ITM_DATE_TRUNC_SECOND:
      {
        // Form is date_trunc(date_trunc('hour', <arg>) + ...) + ...
        // ==> Remove the top + and fall through to next case
        if (op1 &&
            op1->getOperatorType() == ITM_PLUS)
          op1 = op1->child(0);
        else
          op1 = NULL;
      }
      // fall through to next case

    case ITM_DATE_TRUNC_MINUTE:
      {
        // Form is date_trunc('hour', <arg>) + ...
        // ==> Remove the top + and fall through to next case
        if (op1 &&
            op1->getOperatorType() == ITM_PLUS)
          op1 = op1->child(0);
        else
          op1 = NULL;
      }
      // fall through to next case
      
    case ITM_DATE_TRUNC_MONTH:
    case ITM_DATE_TRUNC_HOUR:
    case ITM_DATE_TRUNC_CENTURY:
    case ITM_DATE_TRUNC_DECADE:
      {
        // form is cast(cast(<arg>)) +/- ...
        // ==> Remove the top + or - and fall through to next case
        if (op1 &&
            (op1->getOperatorType() == ITM_PLUS ||
             op1->getOperatorType() == ITM_MINUS))
          op1 = op1->child(0);
        else
          op1 = NULL;
      }
      // fall through to next case
      
    case ITM_DATE_TRUNC_YEAR:
    case ITM_DATE_TRUNC_DAY:
      {
        // form is cast(cast(<arg>))
        if (op1 &&
            op1->getOperatorType() == ITM_CAST)
          {
            op1 = op1->child(0);
            if (op1->getOperatorType() == ITM_CAST)
              op1 = op1->child(0);
          }
        else
          op1 = NULL;

        if (op1)
          result = new(CmpCommon::statementHeap())
            ZZZBinderFunction(expr->origOpType(),op1);
      }
      break;
      
    case ITM_DATEDIFF_YEAR:
    case ITM_DATEDIFF_QUARTER:
    case ITM_DATEDIFF_MONTH:
      {
        // different forms (in prefix notation) - we find <arg1> and <arg2>
        // and ignore the parts shown as ellipsis
        // year:    cast(-(extract(<arg2>),
        //                 extract(<arg1>)))
        // quarter: cast(/(-(+(*(extract(<arg2>),
        //                       ...),
        //                     ...),
        //                   +(*(extract(<arg1>),
        //                       ...),
        //                     ...))))
        // month:   cast(-(+(*(extract(<arg2>),
        //                     ...),
        //                   ...),
        //                 +(*(extract(<arg1>),
        //                     ...),
        //                   ...)))
        if (op1->getOperatorType() == ITM_CAST)
          {
            if (NULL != expr && expr->origOpType() == ITM_DATEDIFF_QUARTER &&
                op1->child(0)->getOperatorType() == ITM_DIVIDE)
              op1 = op1->child(0)->child(0); // the minus operator
              //    cast   /         -
            else if (op1->child(0)->getOperatorType() == ITM_MINUS)
              op1 = op1->child(0); // the minus operator
              //    cast   -

            if (NULL != expr && expr->origOpType() == ITM_DATEDIFF_YEAR)
              {
                if (op1->child(0)->getOperatorType() == ITM_EXTRACT ||
                    op1->child(0)->getOperatorType() == ITM_EXTRACT_ODBC)
                  {
                    op2 = op1->child(0)->child(0);
                    //     -   extract   <arg2>
                    op1 = op1->child(1)->child(0);
                    //     -   extract   <arg1>
                  }
              }
            else if (op1->child(0)->getOperatorType() == ITM_PLUS &&
                     op1->child(0)->child(0)->getOperatorType() == ITM_TIMES &&
                     (op1->child(0)->child(0)->child(0)->getOperatorType() == ITM_EXTRACT ||
                      op1->child(0)->child(0)->child(0)->getOperatorType() == ITM_EXTRACT_ODBC) &&
                     op1->child(1)->getOperatorType() == ITM_PLUS &&
                     op1->child(1)->child(0)->getOperatorType() == ITM_TIMES &&
                     (op1->child(1)->child(0)->child(0)->getOperatorType() == ITM_EXTRACT ||
                      op1->child(1)->child(0)->child(0)->getOperatorType() == ITM_EXTRACT_ODBC))
              {
                op2 = op1->child(0)->child(0)->child(0)->child(0);
                //     -      +         *      extract   <arg2>
                op1 = op1->child(1)->child(0)->child(0)->child(0);
                //     -      +         *      extract   <arg1>
              }
            else
              op1 = NULL;
          }
        if (op1 && op2)
          result = new(CmpCommon::statementHeap())
            ZZZBinderFunction(expr->origOpType(),op1,op2);
      }
      break;

    case ITM_DATEDIFF_WEEK:
      {
        // form:    cast(/(-(-(cast(<arg2>),...),
        //                   -(cast(<arg1>),...)))))
        if (op1->getOperatorType() == ITM_CAST &&
            op1->child(0)->getOperatorType() == ITM_DIVIDE &&
            op1->child(0)->child(0)->getOperatorType() == ITM_MINUS)
          {
            op1 = op1->child(0)->child(0);
            //    cast   /          -

            if (op1->child(0)->getOperatorType() == ITM_MINUS &&
                op1->child(0)->child(0)->getOperatorType() == ITM_CAST &&
                op1->child(1)->getOperatorType() == ITM_MINUS &&
                op1->child(1)->child(0)->getOperatorType() == ITM_CAST)
              {
                op2 = op1->child(0)->child(0)->child(0);
                //     -     -       cast      <arg2>
                op1 = op1->child(1)->child(0)->child(0);
                //     -     -       cast      <arg1>
              }
          }
        else
          op1 = NULL;
        if (op1 && op2)
          result = new(CmpCommon::statementHeap())
            ZZZBinderFunction(expr->origOpType(),op1,op2);
      }
      break;

    case ITM_YEARWEEK:
    case ITM_YEARWEEKD:
      {
        // form: cast(+(*(extract_odbc(<arg1>),...),...))
        if (op1->getOperatorType() == ITM_CAST &&
            op1->child(0)->getOperatorType() == ITM_PLUS &&
            op1->child(0)->child(0)->getOperatorType() == ITM_TIMES &&
            op1->child(0)->child(0)->child(0)->getOperatorType() == ITM_EXTRACT_ODBC)
          {
            op1 = op1->child(0)->child(0)->child(0)->child(0);
            //    cast   +         *       extract   <arg1>
            result = new(CmpCommon::statementHeap())
              ZZZBinderFunction(expr->origOpType(), op1);
          }
      }
      break;

    default:
      break;
    }

  return result;  
}

// returns true if there is an error
bool ZZZBinderFunction::enforceDateOrTimestampDatatype(BindWA * bindWA, CollIndex childIndex, int operand)
{
  // Make sure that the child is of date or timestamp datatype.
  ItemExpr * tempBoundTree =
    child(childIndex)->castToItemExpr()->bindNode(bindWA);
  if (bindWA->errStatus()) 
    return true;

  bool error = (tempBoundTree->getValueId().getType().getTypeQualifier() !=
                NA_DATETIME_TYPE);
  if (!error)
    {
      DatetimeType *dtOper = 
	  &(DatetimeType&)tempBoundTree->getValueId().getType();
      error = ((dtOper->getPrecision() != SQLDTCODE_TIMESTAMP) &&
	       (dtOper->getPrecision() != SQLDTCODE_DATE));
    }

  if (error)
    {
      // 4182 Function $0~String0 operand $0~Int0 must be of type $1~String1.
      *CmpCommon::diags() << DgSqlCode(-4182) 
                          << DgString0(getTextUpper())
                          << DgInt0(operand)
                          << DgString1("DATE or TIMESTAMP");
      bindWA->setErrStatus();
      return true;
    }

  setChild(childIndex, tempBoundTree);
  return false;  // no error
}

//-------------------------------------------------------------------------
//
// member functions for class ItmSequenceFunction
//
//-------------------------------------------------------------------------
ItemExpr *ItmSequenceFunction::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    if (bindWA->getCurrentScope()->getAllSequenceFunctions().contains(getValueId()))
      bindWA->getCurrentScope()->getUnresolvedSequenceFunctions() += getValueId();
    return getValueId().getItemExpr();
  }
  
  if (isOlapFunction())
  {
    ItmSeqOlapFunction * olap = (ItmSeqOlapFunction * )this;
    if ((olap->isFrameStartUnboundedPreceding() && //olap->getframeStart() == -INT_MAX && 
         olap->isFrameEndUnboundedPreceding() ) || //olap->getframeEnd() == -INT_MAX) || 
        (olap->isFrameStartUnboundedFollowing() && //olap->getframeStart() ==  INT_MAX && 
         olap->isFrameEndUnboundedFollowing()) || //olap->getframeEnd() ==  INT_MAX) || 
        (olap->getframeStart() > olap->getframeEnd()))
    {
      //The specified window frame clause is not valid.
      *CmpCommon::diags() << DgSqlCode(-4342);
      bindWA->setErrStatus();
    } 

    if (!olap->isFrameStartUnboundedPreceding() && //olap->getframeStart() != -INT_MAX &&
        !olap->isFrameEndUnboundedFollowing() && //olap->getframeEnd()   != INT_MAX &&
        olap->getframeEnd() - olap->getframeStart() >  
        (Lng32)CmpCommon::getDefaultNumeric(OLAP_MAX_FIXED_WINDOW_FRAME))
    {
      //Maximum Window frame size exceeded.
      *CmpCommon::diags() << DgSqlCode(-4347)
			  << DgInt0((Lng32)CmpCommon::getDefaultNumeric(OLAP_MAX_FIXED_WINDOW_FRAME));
      bindWA->setErrStatus();
    }

    if ( olap->getframeEnd() > 0 &&
        bindWA->getCurrentScope()->context()->inPrecOlapFunction())
    {
      //Nesting Window functions with FOLLOWING clause is not supported.
      *CmpCommon::diags() << DgSqlCode(-4348);
      bindWA->setErrStatus();
    }

    if (olap->getframeStart() <= 0)
    {
      bindWA->getCurrentScope()->context()->inPrecOlapFunction() = TRUE;
    }

  }

  RelSequence * seqNode = 
    (RelSequence *)bindWA->getCurrentScope()->getSequenceNode();

  bindWA->getCurrentScope()->context()->inSequenceFunction() = TRUE;

  if (seqNode && !isOLAP())
  {
    if (isTDFunction() && 
        ((const RelSequence *)seqNode)->requiredOrder().entries() != 0)
    {
      setIsTDFunction(FALSE);
    }

    if(isTDFunction() )
    {
      if (bindWA->getCurrentScope()->context()->inTDFunction() && 
            getOperatorType() == ITM_RUNNING_RANK)
      {//Nesting rank functions is not supported. 
        *CmpCommon::diags() << DgSqlCode(-4368);
        bindWA->setErrStatus();
        return this;
      }

      if  (!bindWA->getCurrentScope()->context()->inSelectList() &&
          !bindWA->getCurrentScope()->context()->inQualifyClause())
      { //Rank can be placed only in the select list or the qualify clause
        *CmpCommon::diags() << DgSqlCode(-4364);
        bindWA->setErrStatus();
        return this;
      }

      if (getOperatorType() == ITM_RUNNING_RANK)
      {
        bindWA->getCurrentScope()->context()->inTDFunction() = TRUE;
      }

      ValueIdList change = seqNode->getPartitionChange();

      if (change.entries() != 0 && getOperatorType() == ITM_RUNNING_RANK)
      {
        setOlapPartitionBy(change.rebuildExprTree(ITM_ITEM_LIST));
        
        CMPASSERT(getOlapPartitionBy());

        ItemExpr * tdSeq = transformTDFunction(bindWA);
        if (! tdSeq)
	    return NULL;

        return tdSeq->bindNode(bindWA);
      }
    }
    else
    {
      if (bindWA->getCurrentScope()->context()->inTDFunction())
      { //Using rank function and sequence functions together in the same query scope is not supported
        *CmpCommon::diags() << DgSqlCode(-4367);
        bindWA->setErrStatus();
        return this;
      }
    }
  }

  BindScope::HasOlapFunctionsEnum olap = isOLAP() ? BindScope::OLAP_: BindScope::NONOLAP_;

  if ( bindWA->getCurrentScope()->getHasOlapSeqFunctions() == BindScope::OLAPUNKNOWN_ )
  {
    bindWA->getCurrentScope()->setHasOlapSeqFunctions(olap);
  } 
  else
  {
    if (bindWA->getCurrentScope()->getHasOlapSeqFunctions() != olap)
    {
       *CmpCommon::diags() << DgSqlCode(-4345);
       bindWA->setErrStatus();
       return NULL;
    }
  }

  if (bindWA->getCurrentScope()->context()->inHavingClause() OR (
      bindWA->getCurrentScope()->context()->inSelectList() AND
      bindWA->getCurrentScope()->getRETDesc()->isGrouped() AND
      ! isOLAP()
     ))  {
       //
       //  4109: We are in a SELECT or HAVING, and the sequence function is not
       //        inside an aggregate (fixes Genesis case 10-990823-0045)
       //
      if (NOT bindWA->getCurrentScope()->context()->inAggregate()) {
        NAString unparsed(bindWA->wHeap());
              unparse(unparsed, DEFAULT_PHASE, USER_FORMAT_DELUXE);
              // 4109: sequence function placed incorrectly
      *CmpCommon::diags() << DgSqlCode(-4109) << DgString0(unparsed);
              bindWA->setErrStatus();
              return this;
      }
  }

  // Capture the current set of sequence functions in this scope.
  // Do not add those sequence functions which are below this node.
  //
  ValueIdSet seqFuncs =
    bindWA->getCurrentScope()->getUnresolvedSequenceFunctions();

 // Check for invalid nesting of the THIS sequence function
 // inside ROWS SINCE and some other sequence function.
 // All other nestings are allowed.
 //
 NABoolean savedRowsSince             = bindWA->getCurrentScope()->context()->inRowsSince();
 NABoolean savedOtherSequenceFunction = bindWA->getCurrentScope()->context()->inOtherSequenceFunction();
 switch (getOperatorType())
 {
   case ITM_ROWS_SINCE:
     bindWA->getCurrentScope()->context()->inRowsSince() = TRUE;
                             // The next line fixes Genesis case 10-990301-0576.
     bindWA->getCurrentScope()->context()->inOtherSequenceFunction() = FALSE;
     break;

   case ITM_THIS:
     if (bindWA->getCurrentScope()->context()->inRowsSince() &&
         bindWA->getCurrentScope()->context()->inOtherSequenceFunction())
     {
       // Inside a ROWS SINCE, the xxx function contained an invalid reference
       // to the THIS function.
       *CmpCommon::diags() << DgSqlCode(-4108);
       bindWA->setErrStatus();
     }
     if (NOT bindWA->getCurrentScope()->context()->inRowsSince())
     {
       // THIS can be used only Inside a ROWS SINCE
       *CmpCommon::diags() << DgSqlCode(-4220);
       bindWA->setErrStatus();
     }
     break;

   default:
     bindWA->getCurrentScope()->context()->inOtherSequenceFunction() = TRUE;
     break;
 }

 // The sequence functions are bound in the environment (RETDesc) of
 // the child of the Sequence
 //

 RelExpr *sequenceNode = bindWA->getCurrentScope()->getSequenceNode();
 RETDesc *currentRETDesc = NULL;

 if (sequenceNode) {

   currentRETDesc = bindWA->getCurrentScope()->getRETDesc();
   
   bindWA->getCurrentScope()->setRETDesc(sequenceNode->child(0)->getRETDesc());
 }


  // ItmSequencefunction is directly derived from BuiltinFunction;
  // safe to invoke this
  //
  BuiltinFunction::bindNode(bindWA);

  if (sequenceNode) {

    bindWA->getCurrentScope()->setRETDesc(currentRETDesc);

  }

  if (bindWA->errStatus()) return NULL;

  bindWA->getCurrentScope()->context()->inTDFunction() = FALSE;

  bindWA->getCurrentScope()->context()->inPrecOlapFunction() = FALSE;
 
  ValueId equivId, finalVid;
  if (CmpCommon::getDefault(COMP_BOOL_203) == DF_ON) {
    equivId = bindWA->getCurrentScope()->getEquivalentItmSequenceFunction(getValueId());
  }

  // Add value id to list of sequence functions in this scope.
  // Ignore those sequence functions which are below this node.
  // We only want the root sequence functions.
  //
  if (CmpCommon::getDefault(COMP_BOOL_203) == DF_ON)
    finalVid = equivId;
  else
    finalVid = getValueId();

  seqFuncs += finalVid ;
  bindWA->getCurrentScope()->getUnresolvedSequenceFunctions() = seqFuncs; 
  bindWA->getCurrentScope()->getAllSequenceFunctions() += finalVid; 


  //
  //  Reset the nesting flags.
  //
  bindWA->getCurrentScope()->context()->inOtherSequenceFunction() = savedOtherSequenceFunction;
  bindWA->getCurrentScope()->context()->inRowsSince()             = savedRowsSince;

  // Reset other context flags.
  bindWA->getCurrentScope()->context()->inSequenceFunction() = FALSE;

  if (CmpCommon::getDefault(COMP_BOOL_203) == DF_ON)
    return equivId.getItemExpr();
  else
    return getValueId().getItemExpr();
}

//---------------------------------------------------------------------------
//
// member functions for class HbaseColumnLookup
//
//---------------------------------------------------------------------------
ItemExpr *HbaseColumnLookup::bindNode(BindWA *bindWA)
{
  ItemExpr * boundExpr = NULL;

  if (nodeIsBound())
    return getValueId().getItemExpr();

  // Binds self; Binds children; ColumnLookup::synthesize();
  boundExpr = Function::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;

  // Add this hbase column to column usage info of this hbase table.
  ItemExpr * ie = child(0)->castToItemExpr();
  NAColumn * nacol = NULL;
  if (ie->getOperatorType() == ITM_REFERENCE)
    {
      ColReference * colR = (ColReference*)ie;
      nacol = colR->getValueId().getNAColumn(TRUE/*okIfNotColumn*/);
    }
  else if (ie->getOperatorType() == ITM_BASECOLUMN)
    {
      BaseColumn * baseC = (BaseColumn*)ie;
      nacol = baseC->getNAColumn();
    }

  const NATable * naTable = nacol->getNATable();
  bindWA->hbaseColUsageInfo()->insert
    ((QualifiedName*)&naTable->getTableName(), &hbaseCol_);
  
  return boundExpr;
}

//---------------------------------------------------------------------------
//
// member functions for class HbaseColumnsDisplay
//
//---------------------------------------------------------------------------
ItemExpr *HbaseColumnsDisplay::bindNode(BindWA *bindWA)
{
  ItemExpr * boundExpr = NULL;

  if (nodeIsBound())
    return getValueId().getItemExpr();

  // Binds self; Binds children; ColumnDisplay::synthesize();
  boundExpr = Function::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;

  // Add this hbase column to column usage info of this hbase table.
  ItemExpr * ie = child(0)->castToItemExpr();
  NAColumn * nacol = NULL;
  if (ie->getOperatorType() == ITM_REFERENCE)
    {
      ColReference * colR = (ColReference*)ie;
      nacol = colR->getValueId().getNAColumn(TRUE/*okIfNotColumn*/);
    }
  else if (ie->getOperatorType() == ITM_BASECOLUMN)
    {
      BaseColumn * baseC = (BaseColumn*)ie;
      nacol = baseC->getNAColumn();
    }

  const NATable * naTable = nacol->getNATable();
  if (csl() == NULL)
    {
      NAString nas("*");
      bindWA->hbaseColUsageInfo()->insert
	((QualifiedName*)&naTable->getTableName(), &nas);
    }
  else
    {
      for (Lng32 i = 0; i < csl()->entries(); i++)
	{
	  NAString * nas = (NAString*)(*csl())[i];

	  bindWA->hbaseColUsageInfo()->insert
	    ((QualifiedName*)&naTable->getTableName(), nas);
	}
    }

  return boundExpr;
}

//---------------------------------------------------------------------------
//
// member functions for class HbaseColumnCreate
//
//---------------------------------------------------------------------------
ItemExpr *HbaseColumnCreate::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();

  ItemExpr * boundExpr = NULL;
  short numEntries = hccol_->entries();
  colValMaxLen_ = 0;
  NAType * firstType = NULL;
  NAType * resultType = NULL;
  NABoolean resultNull = FALSE;
  colNameMaxLen_ = 0;
  for (short i = 0; i < numEntries; i++)
    {
      HbaseColumnCreateOptions * hcco = (*hccol_)[i];
      HbaseColumnCreate::HbaseColumnCreateOptions::ConvType co = HbaseColumnCreate::HbaseColumnCreateOptions::NONE;

      ItemExpr * colName = hcco->colName();
      colName = colName->bindNode(bindWA);
      if (bindWA->errStatus()) 
        return NULL;
      
      // type cast any params
      ValueId vid1 = colName->getValueId();
      SQLVarChar c1(NULL, CmpCommon::getDefaultNumeric(HBASE_MAX_COLUMN_NAME_LENGTH));
      vid1.coerceType(c1, NA_CHARACTER_TYPE);
      
      hcco->setColName(colName);

      ItemExpr * colValue = hcco->colVal();
      colValue = colValue->bindNode(bindWA);
      if (bindWA->errStatus()) 
	return NULL;

      // type cast any params
      ValueId vid2 = colValue->getValueId();
      SQLVarChar c2(NULL, CmpCommon::getDefaultNumeric(HBASE_MAX_COLUMN_VAL_LENGTH));
      vid2.coerceType(c2, NA_CHARACTER_TYPE);

      hcco->setColVal(colValue);

      const NAType &typeColName = 
	colName->castToItemExpr()->getValueId().getType();

      const NAType &typeColVal = 
	colValue->castToItemExpr()->getValueId().getType();

      if (colNameMaxLen_ < typeColName.getNominalSize())
	colNameMaxLen_ = typeColName.getNominalSize();

      if (i == 0) // first entry
	{
	  co = hcco->convType();
	  firstType = (NAType*)hcco->naType();
	  if (firstType)
	    resultType = firstType;
	  else
	    resultType = &(NAType&)typeColVal;
	  if (resultType->getTypeQualifier() != NA_CHARACTER_TYPE)
	    {
	      *CmpCommon::diags() << DgSqlCode(-4221)
				  << DgString0("COLUMN_CREATE(list format)")
				  << DgString1("character type");
	      bindWA->setErrStatus();
	      return NULL;
	    }

	  colValMaxLen_ = resultType->getNominalSize();
	  resultNull = resultType->supportsSQLnull();
	} // if first entry
      else
	{
	  if ((co != hcco->convType()) ||
	      (! firstType && hcco->naType()) ||
	      (firstType && ! hcco->naType()) ||
	      (firstType && hcco->naType() && (NOT (*firstType == *hcco->naType()))))
	    {
	      *CmpCommon::diags() << DgSqlCode(-4221)
			       << DgString0("COLUMN_CREATE(list format)")
			       << DgString1("compatible");

	      bindWA->setErrStatus();
	      return NULL;
	    }
	} // else

      if (co == HbaseColumnCreate::HbaseColumnCreateOptions::NONE)
	{
	  if (colValMaxLen_ < typeColVal.getNominalSize())
	    colValMaxLen_ = typeColVal.getNominalSize();

	  if (typeColVal.supportsSQLnull())
	    resultNull = TRUE;
	}
    } // for

  resultNull = TRUE;
  NAType * childResultType = new(bindWA->wHeap()) SQLVarChar(bindWA->wHeap(), colValMaxLen_,
							     resultNull);
  
  Lng32 totalLen = 0;
  totalLen += sizeof(numEntries) + sizeof(colNameMaxLen_) 
    + sizeof(short)/*VCLenIndicatorSize*/ + sizeof(colValMaxLen_);
  
  for (Lng32 i = 0; i < numEntries; i++)
    {
      HbaseColumnCreateOptions * hcco = (*hccol_)[i];

      NAType * cnType = new(bindWA->wHeap()) SQLVarChar(bindWA->wHeap(), colNameMaxLen_, FALSE);
      ItemExpr * cnChild =
	new (bindWA->wHeap()) Cast(hcco->colName(), cnType);
      cnChild = cnChild->bindNode(bindWA);
      hcco->setColName(cnChild);
      totalLen += cnChild->getValueId().getType().getTotalSize();

      ItemExpr * newChild =
	new (bindWA->wHeap()) Cast(hcco->colVal(), childResultType);
      newChild = newChild->bindNode(bindWA);
      hcco->setColVal(newChild);
      totalLen += newChild->getValueId().getType().getTotalSize();      
    }

  resultType_ = new(bindWA->wHeap()) SQLVarChar(bindWA->wHeap(), totalLen, FALSE);
  
  // Binds self; Binds children; ColumnCreate::synthesize();
  boundExpr = Function::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;

  return boundExpr;
}


// -----------------------------------------------------------------------
// member functions for class Loboper, LOBinsert, LOBselect, LOBdelete
// -----------------------------------------------------------------------

ItemExpr *LOBinsert::bindNode(BindWA *bindWA)
{
  ItemExpr * boundExpr = NULL;

  if (nodeIsBound())
    return getValueId().getItemExpr();

  // Binds self; Binds children; LOBoper::synthesize();
  boundExpr = LOBoper::bindNode(bindWA);
  if (bindWA->errStatus()) return NULL;
 
  return boundExpr;
} // LOBinsert::bindNode()

ItemExpr *LOBselect::bindNode(BindWA *bindWA)
{
  ItemExpr * boundExpr = NULL;

  if (nodeIsBound())
    return getValueId().getItemExpr();


  // For now LOBselect is allowed only in the top most select list
  // check that first, or else give an error


  // Binds self; Binds children; LOBoper::synthesize();
  boundExpr = LOBoper::bindNode(bindWA);
  if (bindWA->errStatus()) return NULL;
 
  return boundExpr;
} // LOBselect::bindNode()



ItemExpr *SequenceValue::bindNode(BindWA *bindWA)
{
  ItemExpr * boundExpr = NULL;

  if (nodeIsBound())
    return getValueId().getItemExpr();

  // Binds self; Binds children; SequenceValue::synthesize();
  boundExpr = Function::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;

  ULng32 savedParserFlags = Get_SqlParser_Flags (0xFFFFFFFF);
  Set_SqlParser_Flags(ALLOW_VOLATILE_SCHEMA_IN_TABLE_NAME);

  // Obtain the NATable for the seq object.
  naTable_ = bindWA->getNATable(seqCorrName_);
  if (bindWA->errStatus())
    return this;

  // BindWA keeps list of sequence generators used, so privileges can be checked.
  bindWA->insertSeqVal(this);

  Assign_SqlParser_Flags (savedParserFlags);

  return boundExpr;
}

ItemExpr *HbaseTimestamp::bindNode(BindWA *bindWA)
{
  ItemExpr * boundExpr = NULL;

  CMPASSERT(col_);

  if (nodeIsBound())
    return getValueId().getItemExpr();

  col_ = col_->bindNode(bindWA);
  if (! col_ || bindWA->errStatus())
    return NULL;

  CMPASSERT(col_->getOperatorType() == ITM_BASECOLUMN);

  NAColumn * nac = ((BaseColumn*)col_)->getNAColumn();
  if (! nac)
    return NULL;

  colName_ = nac->getColName();

  NAType * tsValsType = 
    new (bindWA->wHeap()) SQLVarChar(bindWA->wHeap(), sizeof(Int64), FALSE);
  tsVals_ = 
    new (bindWA->wHeap()) NATypeToItem(tsValsType);
  
  tsVals_ = tsVals_->bindNode(bindWA);
  if (! tsVals_ || bindWA->errStatus())
    return NULL;

  // Binds self; Binds children; HbaseTimestamp::synthesize();
  boundExpr = Function::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;
  
  return boundExpr;
}

ItemExpr *HbaseTimestampRef::bindNode(BindWA *bindWA)
{
  ItemExpr * boundExpr = NULL;

  CMPASSERT(col_);

  if (nodeIsBound())
    return getValueId().getItemExpr();

  col_ = col_->bindNode(bindWA);
  if (! col_ || bindWA->errStatus())
    return NULL;

  CMPASSERT(col_->getOperatorType() == ITM_BASECOLUMN);

  BaseColumn * bc = (BaseColumn*)col_;

  if ((bc->getTableDesc()->getNATable()->isHiveTable()) ||
      (bc->getTableDesc()->getNATable()->isSQLMXAlignedTable()))
    {
      if (bc->getTableDesc()->getNATable()->isHiveTable())
        *CmpCommon::diags() << DgSqlCode(-3242)
                            << DgString0("hbase_timestamp or hbase_version cannot be used on a Hive table.");
      else
        *CmpCommon::diags() << DgSqlCode(-3242)
                            << DgString0("hbase_timestamp or hbase_version cannot be used on an aligned format table.");
      
      bindWA->setErrStatus();
      return NULL;
    }

  if (bc->getTableDesc()->hbaseTSList().entries() == 0)
    {
      for (CollIndex i = 0; i < bc->getTableDesc()->getColumnList().entries(); i++) 
        {
          ItemExpr *baseCol = bc->getTableDesc()->getColumnList()[i].getItemExpr();
          HbaseTimestamp * hbtCol = 
            new (bindWA->wHeap()) HbaseTimestamp(baseCol);
          hbtCol->bindNode(bindWA);
          if (bindWA->errStatus()) 
            return NULL;
          bc->getTableDesc()->hbaseTSList().insert(hbtCol->getValueId());
        }
    }

  ValueId valId = bc->getTableDesc()->hbaseTSList()[bc->getColNumber()];
  setValueId(valId);
  
  bindSelf(bindWA);
  if (bindWA->errStatus()) 
    return NULL;
  
  return valId.getItemExpr();
}

ItemExpr *HbaseVersion::bindNode(BindWA *bindWA)
{
  ItemExpr * boundExpr = NULL;

  CMPASSERT(col_);

  if (nodeIsBound())
    return getValueId().getItemExpr();

  col_ = col_->bindNode(bindWA);
  if (! col_ || bindWA->errStatus())
    return NULL;

  CMPASSERT(col_->getOperatorType() == ITM_BASECOLUMN);

  NAColumn * nac = ((BaseColumn*)col_)->getNAColumn();
  if (! nac)
    return NULL;

  colName_ = nac->getColName();

  NAType * tsValsType = 
    new (bindWA->wHeap()) SQLVarChar(bindWA->wHeap(), sizeof(Int64), FALSE);
  tsVals_ = 
    new (bindWA->wHeap()) NATypeToItem(tsValsType);
  
  tsVals_ = tsVals_->bindNode(bindWA);
  if (! tsVals_ || bindWA->errStatus())
    return NULL;

  // Binds self; Binds children; HbaseVersion::synthesize();
  boundExpr = Function::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;
  
  return boundExpr;
}

ItemExpr *HbaseVersionRef::bindNode(BindWA *bindWA)
{
  ItemExpr * boundExpr = NULL;

  CMPASSERT(col_);

  if (nodeIsBound())
    return getValueId().getItemExpr();

  col_ = col_->bindNode(bindWA);
  if (! col_ || bindWA->errStatus())
    return NULL;

  CMPASSERT(col_->getOperatorType() == ITM_BASECOLUMN);

  BaseColumn * bc = (BaseColumn*)col_;

  if ((bc->getTableDesc()->getNATable()->isHiveTable()) ||
      (bc->getTableDesc()->getNATable()->isSQLMXAlignedTable()))
    {
      if (bc->getTableDesc()->getNATable()->isHiveTable())
        *CmpCommon::diags() << DgSqlCode(-3242)
                            << DgString0("hbase_timestamp or hbase_version cannot be used on a Hive table.");
      else
        *CmpCommon::diags() << DgSqlCode(-3242)
                            << DgString0("hbase_timestamp or hbase_version cannot be used on an aligned format table.");
      
      bindWA->setErrStatus();
      return NULL;
    }

  if (bc->getTableDesc()->hbaseVersionList().entries() == 0)
    {
      for (CollIndex i = 0; i < bc->getTableDesc()->getColumnList().entries(); i++) 
        {
          ItemExpr *baseCol = bc->getTableDesc()->getColumnList()[i].getItemExpr();
          HbaseVersion * hbtCol = 
            new (bindWA->wHeap()) HbaseVersion(baseCol);
          hbtCol->bindNode(bindWA);
          if (bindWA->errStatus()) 
            return NULL;
          bc->getTableDesc()->hbaseVersionList().insert(hbtCol->getValueId());
        }
    }

  ValueId valId = bc->getTableDesc()->hbaseVersionList()[bc->getColNumber()];
  setValueId(valId);
  
  bindSelf(bindWA);
  if (bindWA->errStatus()) 
    return NULL;
  
  return valId.getItemExpr();
}

ItemExpr *RowNumFunc::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return getValueId().getItemExpr();

  // For now user(x) is allowed only in the top most select list
  // check that first, or else give an error

  BindScope * currScope = bindWA->getCurrentScope();
  BindContext *context = currScope->context();

  if (!(context->inSelectList()))
    //      (! context->inWhereClause()))
  {
    *CmpCommon::diags() << DgSqlCode(-4311)
			<< DgString0("ROWNUM");
    bindWA->setErrStatus();
    return NULL;
  }

  // Check for case like select (select user(1) ...).
  // or select * from t1, (select user(x) from t1) t3 etc.
  // Here the user function is in the select list of a
  // sub-query and in join, hence is not allowed.
  // Also it is not allowed at any other place example orderBy
  BindScope *prevScope   = NULL;

  while (currScope)
  {
    BindContext *currContext = currScope->context();
    if (currContext->inSubquery() ||
        currContext->inOrderBy() ||
        currContext->inExistsPredicate() ||
        currContext->inGroupByClause() ||
        currContext->inGroupByOrdinal() ||
        currContext->inWhereClause() ||
        currContext->inHavingClause() ||
        currContext->inUnion() ||
        currContext->inJoin()       )
      {
 	*CmpCommon::diags() << DgSqlCode(-4311)
			    << DgString0("ROWNUM");
	bindWA->setErrStatus();
	return NULL;
      }
    prevScope = currScope;
    currScope = bindWA->getPreviousScope(prevScope);
  }

  return BuiltinFunction::bindNode(bindWA);
} // RowNumFunc::bindNode

NABoolean ItemExpr::canBeUsedInGBorOB(NABoolean setErr)
{
  Int32 arity = getArity();
  for (Int32 i=0; i<arity; i++) 
    {
      ItemExpr *ieChild = child(i);
      if (NOT ieChild->canBeUsedInGBorOB(setErr))
        return FALSE;
    }
  
  return TRUE;
}

NABoolean RowNumFunc::canBeUsedInGBorOB(NABoolean setErr)
{
  // cannot be used in a group by or order by clause.
  if (setErr)
    {
      *CmpCommon::diags() << DgSqlCode(-4311)
                          << DgString0("ROWNUM");
    }

  return FALSE;
}
